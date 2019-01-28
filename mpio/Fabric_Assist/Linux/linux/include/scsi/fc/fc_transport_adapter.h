/*******************************************************************
 * This file is part of the Emulex Linux Device Driver for         *
 * Fibre Channel Host Bus Adapters.                                *
 * Copyright (C) 2017-2018 Broadcom. All Rights Reserved. The term *
 * Broadcom
 * Copyright (C) 2004-2016 Emulex.  All rights reserved.           *
 * EMULEX and SLI are trademarks of Emulex.                        *
 * www.broadcom.com                                                *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of version 2 of the GNU General       *
 * Public License as published by the Free Software Foundation.    *
 * This program is distributed in the hope that it will be useful. *
 * ALL EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND          *
 * WARRANTIES, INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY,  *
 * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT, ARE      *
 * DISCLAIMED, EXCEPT TO THE EXTENT THAT SUCH DISCLAIMERS ARE HELD *
 * TO BE LEGALLY INVALID.  See the GNU General Public License for  *
 * more details, a copy of which can be found in the file COPYING  *
 * included with this package.                                     *
 *******************************************************************/
#ifndef __FC_TRANSPORT_ADAPTER_H__
#define __FC_TRANSPORT_ADAPTER_H__

#define FCTXPD_DRIVER_NAME	"fctxpt"
#define FC_PAYLOAD_MAXLEN	2048

//int fctxpd_dev_init(void);
void fctxpd_dev_register(void);
int update_els_frame(uint64_t pwwn, void *payload);

typedef struct fpin_payload {
	uint64_t hba_wwn;
	uint32_t length; //2048 for now
	char payload[FC_PAYLOAD_MAXLEN];
} fpin_payload_t;

struct hba_port_wwn_info {
	uint64_t initiator_hba_wwn;
	uint64_t target_hba_wwn;
	uint64_t hba_ctxt;
};

#define FCTXPD_FAILBACK_IO  _IOWR('N', 0x1, struct hba_port_wwn_info)
int fc_register_els(int (*fc_reg_flush_pending_io)
			(struct hba_port_wwn_info *port_info));
#endif
