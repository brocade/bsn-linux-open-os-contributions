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
#include "fpin.h"

struct fpin_global global;
struct list_head els_marginal_list_head;

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

	if ((global.fctxp_device_rfd = open("/dev/fctxpd", O_RDONLY)) < 0) {
		FPIN_CLOG("NO Device found\n");
		exit(0);
	}

	fpin_payload = calloc(1, fpin_payload_sz);
	if (fpin_payload == NULL) {
		FPIN_CLOG(" No Mem to alloc\n");
		pthread_exit(NULL);
	}

	for ( ; ; ) {
		FPIN_ILOG("Waiting for ELS...\n");
		maxfd = global.fctxp_device_rfd + 1;
		FD_SET(global.fctxp_device_rfd, &readfs);
		poll:
		ret = select(maxfd, &readfs, NULL, NULL, NULL);
		if (!ret) {
			perror("read");
                        FPIN_ELOG("select failed  ret = %d\n", ret);
                        break;
		}
		
		ret = read(global.fctxp_device_rfd, fpin_payload, fpin_payload_sz);
		FPIN_DLOG("Got a new request\n");
		if (ret < FC_PAYLOAD_MAXLEN) {
			perror("read");
			FPIN_ELOG("Read failed to read ret = %d 0x%x\n", ret,(char *)fpin_payload->payload);
			 goto poll;
		}

		/* Push the frame to appropriate frame list */
		fpin_handle_els_frame(fpin_payload);
	}

	if (fpin_payload != NULL) {
		free(fpin_payload);
		fpin_payload = NULL;
	}

	close(global.fctxp_device_rfd);
}

void dump_err(int func, int err) {
	if (func == 1) {
		switch(err) {
			case EAGAIN:
				FPIN_CLOG("Pthread_create: Insuffecient Resources\n");
				break;
			case EINVAL:
				FPIN_CLOG("Pthread_create: Invalid settings in attr.\n");
				break;
			case EPERM:
				FPIN_CLOG("Pthread_create: No permission to set the scheduling "
							"policy and parameters specified in attr.\n");
				break;
			default:
				FPIN_ELOG("Pthread_create: Unknown Error\n");
				break;
		}
	} else if (func == 2) {
		switch(err) {
			case EDEADLK:
				FPIN_CLOG("Pthread_join: A deadlock was detected.\n");
				break;
			case EINVAL:
				FPIN_CLOG("Pthread_join: Thread is not joinable\n");
				break;
			case ESRCH:
				FPIN_CLOG("Pthread_join: Thread ID not found.\n");
				break;
			default:
				FPIN_ELOG("Pthread_create: Unknown Error\n");
				break;
		}
	}
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
PCFRT:
	ret = pthread_create(&fpin_receiver_thread_id, NULL,
				fpin_fabric_notification_receiver, NULL);
	if (ret != 0) {
		FPIN_ELOG("Failed to create receiver thread, err %d, retrying\n", ret);
		retries--;
		if (retries)
			goto PCFRT;

		FPIN_CLOG("Failed to create receiver thread, err %d, exiting\n", ret);
		dump_err(1, ret);
		exit (ret);
	}

	retries = 3;
PCFCT:
	ret = pthread_create(&fpin_consumer_thread_id, NULL,
				fpin_els_li_consumer, NULL);
	if (ret != 0) {
		FPIN_ELOG("Failed to create consumer thread, err %d, retrying\n", ret);
		retries--;
		if (retries)
			goto PCFCT;

		FPIN_CLOG("Failed to create receiver thread, err %d, exiting\n", ret);
		dump_err(1, ret);
		exit (ret);
	}

	retries = 3;
PJFRT:
	ret = pthread_join(fpin_receiver_thread_id, NULL);
	if (ret != 0) {
		FPIN_ELOG("Failed to join consumer thread, err %d, retrying\n", ret);
		retries--;
		if (retries)
			goto PJFRT;

		FPIN_CLOG("Failed to join receiver thread, err %d, exiting\n", ret);
		dump_err(2, ret);
		exit (ret);
	}
	retries = 3;
PJFCT:
	ret = pthread_join(fpin_consumer_thread_id, NULL);
	if (ret != 0) {
		FPIN_ELOG("Failed to join consumer thread, err %d, retrying\n", ret);
		retries--;
		if (retries)
			goto PJFCT;

		FPIN_CLOG("Failed to join receiver thread, err %d, exiting\n", ret);
		dump_err(2, ret);
		exit (ret);
	}

	exit (0);
}
