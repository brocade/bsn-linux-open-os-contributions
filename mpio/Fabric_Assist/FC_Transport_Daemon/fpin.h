#ifndef __FPIN_H__
#define __FPIN_H__

#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/netlink.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <limits.h>
#include <dirent.h>
#include <libudev.h>
#include <libdevmapper.h>
#include <syslog.h>
#include "fpin_els.h"

#ifdef FPIN_DEBUG
#define FPIN_DLOG(fmt...) syslog(LOG_DEBUG, fmt);
#define FPIN_ILOG(fmt...) syslog(LOG_INFO, fmt);
#define FPIN_ELOG(fmt...) syslog(LOG_ERR, fmt);
#define FPIN_CLOG(fmt...) syslog(LOG_CRIT, fmt);
#else
#define FPIN_DLOG(fmt...)
#define FPIN_ILOG(fmt...)
#define FPIN_ELOG(fmt...)
#define FPIN_CLOG(fmt...)
#endif



/* Linked List to store sd and dm mapping */
#define DM_PARAMS_SIZE	4096
#define DEV_NAME_LEN	10
#define TGT_NAME_LEN	20
#define DEV_NODE_LEN	32
#define WWN_LEN			32
#define PORT_ID_LEN		8
#define UUID_LEN		128
#define SYS_PATH_LEN	512

/*IOCTL to fail all pending IOs on HBA port */
struct hba_port_wwn_info {
        uint64_t initiator_hba_wwn;
        uint64_t target_hba_wwn;
        uint64_t hba_ctxt;
};

#define FCTXPD_FAILBACK_IO  _IOWR('N', 0x1, struct hba_port_wwn_info)

/* Global Structure to store all globals*/
struct fpin_global {
        int             fctxp_device_rfd;
};

struct impacted_dev_list
{
	char dev_node[DEV_NAME_LEN];
	char dev_name[DEV_NAME_LEN];
	char dev_serial_id[UUID_LEN];
	struct list_head dev_list_head;
};

struct dm_dev_list
{
	char dm_dev_node[DEV_NAME_LEN];
	char dm_name[DEV_NAME_LEN];
	char dm_uuid[UUID_LEN];
	struct list_head dm_list_head;
};

struct target_list
{
	char target[TGT_NAME_LEN];
	struct list_head target_head;
};

/* Structure to store WWNs of HBA port and affected PWWNs */
struct impacted_port_wwn_list
{
	char impacted_port_wwn[WWN_LEN];
	struct list_head impacted_port_wwn_head;
};

/* For 1 hba_wwn, we will have a list of impacted
 * port WWNs.
 */
struct wwn_list
{
	char hba_wwn[WWN_LEN];
	struct impacted_port_wwn_list impacted_ports;
};

/* ELS frame Handling functions */
int fpin_fetch_dm_lun_data(struct wwn_list *list, struct list_head *dm_list_head,
				struct list_head *impacted_dev_list_head);
void fpin_dm_fail_path(struct list_head *dm_list_head,
				struct list_head *impacted_dev_list_head,
				struct hba_port_wwn_info *port_info);

int fpin_populate_dm_lun(struct list_head *dm_list_head,
			struct list_head *impacted_dev_list_head,
			struct list_head *target_head);

/* Target Related Functions */
int fpin_dm_insert_target(struct list_head *tgt_head, char *target);
void fpin_dm_display_target(struct list_head *tgt_head);
int fpin_dm_find_target(struct list_head *tgt_head, char *target);
void fpin_dm_free_target(struct list_head *tgt_head);
int fpin_dm_populate_target(struct wwn_list *list,
				 struct list_head *tgt_list);

/* WWN Related Functions */
int fpin_els_wwn_exists(struct wwn_list *list, char *port_wwn_buf);
void fpin_els_free_wwn_list(struct wwn_list *list);

/* Util Functions */
int sysfs_read_line(const char *dir, const char *file, char *buf, size_t len);
#endif
