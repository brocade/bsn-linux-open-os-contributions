#ifndef __FPIN_H__
#define __FPIN_H__

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <limits.h>
#include <dirent.h>
#include <libudev.h>
#include <libdevmapper.h>
#include <sysexits.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
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
#define CMD_LEN			128
#define DEV_NAME_LEN	128
#define TGT_NAME_LEN	64
#define DEV_NODE_LEN	32
#define WWN_LEN			32
#define PORT_ID_LEN		8
#define UUID_LEN		128
#define SYS_PATH_LEN	512

#define	IO_FLUSH_ATTR	"abort_outstanding_io"

#define NLMSG_MIN_TYPE		0x10	/* < 0x10: reserved control messages */
#define SCSI_TRANSPORT_MSG		NLMSG_MIN_TYPE + 1
#define SCSI_NL_VERSION				1
#define SCSI_NL_MARGINAL_PATH  0x0002

/* scsi_nl_hdr->magic value */
#define SCSI_NL_MAGIC				0xA1B2

/* scsi_nl_hdr->transport value */
#define SCSI_NL_TRANSPORT			0
#define SCSI_NL_TRANSPORT_FC			1
#define SCSI_NL_MAX_TRANSPORTS			2

struct scsi_nl_hdr {
	uint8_t version;
	uint8_t transport;
	uint16_t magic;
	uint16_t msgtype;
	uint16_t msglen;
} __attribute__((aligned(sizeof(uint64_t))));
struct fc_nl_event {
	struct scsi_nl_hdr snlh;		/* must be 1st element ! */
	uint64_t seconds;
	uint64_t vendor_id;
	uint16_t host_no;
	uint16_t event_datalen;
	uint32_t event_num;
	uint32_t event_code;
	uint32_t event_data;
} __attribute__((aligned(sizeof(uint64_t))));

enum scsi_fpin_event {
        SCSI_FPIN_SUPPORT = 1,
        SCSI_FPIN_LINK_INTEGRITY,
        SCSI_FPIN_LINK_CONGESTION,
        SCSI_HOST_VMID,
};
struct scsi_fpin_hdr {
        uint64_t port_wwn;
        uint64_t t_port_wwn;
        enum scsi_fpin_event event;
} __attribute__((aligned(sizeof(uint64_t))));

/* Global Structure to store all globals*/
struct fpin_global {
        int             fctxp_device_rfd;
};

struct impacted_dev_list
{
	char dev_node[DEV_NAME_LEN];
	char dev_name[DEV_NAME_LEN];
	char dev_serial_id[UUID_LEN];
	char dev_pwwn[WWN_LEN];
	char sd_path[SYS_PATH_LEN];
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
	char tgt_pwwn[WWN_LEN];
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
	char hba_syspath[SYS_PATH_LEN];
	struct impacted_port_wwn_list impacted_ports;
};

/* ELS frame Handling functions */
int fpin_fetch_dm_lun_data(struct wwn_list *list,
			struct list_head *dm_list_head,
			struct list_head *impacted_dev_list_head, struct udev *udev);
void fpin_dm_fail_path(struct udev *udev, struct list_head *dm_list_head,
				struct list_head *impacted_dev_list_head,
				struct wwn_list *list);

int fpin_populate_dm_lun(struct list_head *dm_list_head,
			struct list_head *impacted_dev_list_head,
			struct udev *udev, struct list_head *target_head);

/* Target Related Functions */
int fpin_dm_insert_target(struct list_head *tgt_head,
			char *target, char *tgt_wwn);
int fpin_dm_find_target(struct list_head *tgt_head,
			char *target, char **tgt_wwn);
void fpin_dm_display_target(struct list_head *tgt_head);
void fpin_dm_free_target(struct list_head *tgt_head);
int fpin_dm_populate_target(struct wwn_list *list,
		 struct list_head *tgt_list, struct udev *udev);
void fpin_dm_free_dev(struct  list_head *sd_head);
void fpin_free_dm(struct list_head *dm_head);

/* WWN Related Functions */
int fpin_els_wwn_exists(struct wwn_list *list, char *port_wwn_buf);
void fpin_els_free_wwn_list(struct wwn_list *list);

/* Util Functions */
int sysfs_read_line(const char *dir, const char *file, char *buf, size_t len);
#endif
