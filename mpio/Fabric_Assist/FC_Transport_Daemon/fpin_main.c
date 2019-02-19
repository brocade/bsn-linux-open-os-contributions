/*
 * Copyright (c) 2018 Broadcom Inc.
 *
 * FPIN daemon code.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * Authors:
 *      Ganesh Murugesan <ganesh.murugesan@broadcom.com>
 *      Ganesh Pai <ganesh.pai@broadcom.com>
 */

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include "fpin.h"
#include <linux/sockios.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/ethtool.h>
#include <linux/if_vlan.h>


struct fpin_global global;
struct list_head els_marginal_list_head;
static int fcm_fc_socket;
#define DEF_RX_BUF_SIZE		4096

/* 
 * Thread to listen for ELS frames from driver. on receiving the frame payload,
 * push the payload to a list, and notify the fpin_els_li_consumer thread to
 * process it. Once consumer thread is notified, return to listen for more ELS
 * frames from driver.
 */
void *fpin_fabric_notification_receiver()
{
	int ret = -1; 
	int maxfd = 0;
	fd_set readfs;
	fpin_payload_t *fpin_payload = NULL;
	int fpin_payload_sz = sizeof(fpin_payload_t) + FC_PAYLOAD_MAXLEN;
	int fd, rc;
	struct sockaddr_nl fc_local;
	unsigned char buf[DEF_RX_BUF_SIZE];
	int offset =0;

	fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_SCSITRANSPORT);
	if (fd < 0) {
		FPIN_ELOG("fc socket error %d", fd);
		pthread_exit(NULL);
	}
	memset(&fc_local, 0, sizeof(fc_local));
	fc_local.nl_family = AF_NETLINK;
	fc_local.nl_groups = ~0;
	fc_local.nl_pid = getpid();
	rc = bind(fd, (struct sockaddr *)&fc_local, sizeof(fc_local));
	if (rc == -1) {
		FPIN_ELOG("fc socket bind error %d\n", rc);
		close(fd);
		exit(EX_NOINPUT);
	}
	global.fctxp_device_rfd = fd;
	fpin_payload = calloc(1, fpin_payload_sz);
	if (fpin_payload == NULL) {
		FPIN_CLOG(" No Mem to alloc\n");
		exit(EX_IOERR);
	}

	for ( ; ; ) {
		FPIN_ILOG("Waiting for ELS...\n");
		ret = read(global.fctxp_device_rfd, buf, DEF_RX_BUF_SIZE);
		FPIN_DLOG("Got a new request\n");

		/* Push the frame to appropriate frame list */
		offset= sizeof(struct nlmsghdr) + sizeof(struct fc_nl_event);
		offset = offset - 4;
		memcpy(fpin_payload, &buf[offset], fpin_payload_sz);
		fpin_handle_els_frame(fpin_payload);
	}

	if (fpin_payload != NULL) {
		free(fpin_payload);
		fpin_payload = NULL;
	}

	close(global.fctxp_device_rfd);
}

/*
 * FPIN daemon main(). Sleeps on read until an FPIn ELS frame is recieved from
 * HBA driver.
 */
int
main(int argc, char *argv[])
{

	int ret = -1, retries = 3;
	pthread_t fpin_receiver_thread_id;
	pthread_t fpin_consumer_thread_id;
	/* Daemonize */
	//fpin_daemonize();

	/*
	 *	A thread to recieve notifications from fabric and process them.
	 */
	setlogmask (LOG_UPTO (LOG_INFO));
	openlog("FCTXPTD", LOG_PID, LOG_USER);
	INIT_LIST_HEAD(&els_marginal_list_head);
	ret = pthread_create(&fpin_receiver_thread_id, NULL,
				fpin_fabric_notification_receiver, NULL);
	if (ret != 0) {
		FPIN_CLOG("pthread_create failed receiver thread, err %d, %s\n", ret, strerror(errno));
		exit (ret);
	}

	ret = pthread_create(&fpin_consumer_thread_id, NULL,
				fpin_els_li_consumer, NULL);
	if (ret != 0) {
		FPIN_CLOG("pthread_create failed for receiver thread, err %d, %s\n", ret, strerror(errno));
		exit (ret);
	}

	ret = pthread_join(fpin_receiver_thread_id, NULL);
	if (ret != 0) {
		retries--;
		FPIN_CLOG("pthread_join failed for reciever thread, err %d, %s\n", ret, strerror(errno));
		exit (ret);
	}
	ret = pthread_join(fpin_consumer_thread_id, NULL);
	if (ret != 0) {
		FPIN_CLOG("pthread_join failed for consumer thread, err %d, %s\n", ret, strerror(errno));
		exit (ret);
	}

	exit (0);
}
