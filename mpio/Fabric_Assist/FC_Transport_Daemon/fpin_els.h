#ifndef __FPIN_ELS_H__
#define __FPIN_ELS_H__

#include <pthread.h>
#include <endian.h>
#include <arpa/inet.h>

#include "list.h"

/* max ELS frame Size */
#define FC_PAYLOAD_MAXLEN   2048

/* ELS Frame, TBD: Use FPIN ELS once standardized */
#define ELS_CMD_FPIN 0x16
#define ELS_CMD_MPD 0x82
#define ELS_CMD_CJN 0x83

/*
 * Union of all possible formats
 */
typedef union wwn {
	/*
	 * Big TBD: Take care of 32 and 64 B arch. Used this type temporarily
	 * as this is how the struct is sent on wire now.
	 */
	uint32_t	words[2];
} wwn_t;

struct els_marginal_list {
	wwn_t hba_pwwn;
	char payload[FC_PAYLOAD_MAXLEN];
	struct list_head els_frame;
};

/* --- FPIN --- */

/* Notification Descriptor Tags */
typedef enum fpin_notification_descriptor_tag {
	eFPIN_NOTIFICATION_DESCRIPTOR_UNASSIGNED				= 0x00000000,
	eFPIN_NOTIFICATION_DESCRIPTOR_REGISTER_TAG				= 0x00020000,
	eFPIN_NOTIFICATION_DESCRIPTOR_LINK_INTEGRITY_TAG		= 0x00020001,
	eFPIN_NOTIFICATION_DESCRIPTOR_DELIVERY_TAG				= 0x00020002,
	eFPIN_NOTIFICATION_DESCRIPTOR_CONGESTION_TAG			= 0x00020003,
	eFPIN_NOTIFICATION_DESCRIPTOR_TRANS_DELAY_TAG			= 0x00020004
} fpin_notification_descriptor_tag_e;

/* Congestion Notification Event Types */
typedef enum fpin_congestion_notification_event_type {
	eFPIN_CONGESTION_NOTIFICATION_EVENT_TYPE_NONE			= 0x00,
	eFPIN_CONGESTION_NOTIFICATION_EVENT_TYPE_LOST_CREDIT	= 0x01,
	eFPIN_CONGESTION_NOTIFICATION_EVENT_TYPE_CREDIT_STALL	= 0x02,
	eFPIN_CONGESTION_NOTIFICATION_EVENT_TYPE_OVERSUBSCRIPTION \
															= 0x03
} fpin_congestion_notification_event_type_e;

/* FPIN ELS Header */
typedef struct fpin_els_header {
	uint32_t                                els_code;		/* ELS Command Code */
	uint32_t                                length;			/* Descriptor Length */
	fpin_notification_descriptor_tag_e      tag;			/* Tag */
} fpin_els_header_t;

/* Notification Port List */
typedef struct fpin_notification_port_list {
	uint32_t            count;								/* Number of port names in 'port_name_list' */
	wwn_t               port_name_list[0];					/* List of N_Port Names (Port WWN) */
} fpin_notification_port_list_t;

typedef struct fpin_link_integrity_notification {
	fpin_notification_descriptor_tag_e  tag;				/* eFPIN_NOTIFICATION_DESCRIPTOR_LINK_INTEGRITY_TAG */
	uint32_t                            length;				/* Length in bytes */
	wwn_t                               detecting_port_wwn;	/* Detecting F/N_Port Name (Port WWN) */
	wwn_t                               attached_port_wwn;	/* Attached F/N_Port Name (Port WWN) */
	fpin_notification_port_list_t       port_list;			/* Event data (Port List) */
} fpin_link_integrity_notification_t;

typedef struct fpin_link_integrity_request_els {
	uint32_t							els_code;			/* ELS command code */
	uint32_t							desc_length;			/* Length of 'linkIntegrityDesc' in bytes */
	fpin_link_integrity_notification_t	li_desc;	/* Link Integrity Descriptor */
} fpin_link_integrity_request_els_t;

/* FPIN Payload received from HBA driver */
typedef struct fpin_payload {
	wwn_t hba_wwn;
	uint32_t length; //2048 for now
	char payload[0];
} fpin_payload_t;

void *fpin_els_li_consumer();
#endif
