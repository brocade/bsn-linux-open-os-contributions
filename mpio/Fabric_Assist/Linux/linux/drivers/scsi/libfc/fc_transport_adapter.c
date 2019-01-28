/*******************************************************************
 * This file is part of the Broadcom Linux Device Driver for         *
 * Fibre Channel transport adpapter.                                *
 * Copyright (C) 2017-2018 Broadcom. All Rights Reserved. The term *
 * Broadcom							   *
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
#include <linux/version.h>
#include <linux/semaphore.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <scsi/fc/fc_transport_adapter.h>

struct fpin_els_info {
	struct list_head    els_list;
	fpin_payload_t fpin_payload;
};

struct fpin_support_hba {
	struct list_head    hba_list;
	int (*fc_flush_pending_io)(struct hba_port_wwn_info *port_info);
};

LIST_HEAD(fc_transport_els_list);
LIST_HEAD(hba_support_fpin_list);
static DEFINE_SPINLOCK(els_lock);

#define FCTXPD_IS_OPEN 0x1
static unsigned long fctxpd_status;	/* bitmapped status byte.	*/

DECLARE_WAIT_QUEUE_HEAD(fctxpd_readq);

int update_els_frame(uint64_t hba_pwwn, void *payload)
{
	struct fpin_els_info *pinfo = NULL;
	unsigned long flags = 0;

	if (payload != NULL) {

		pinfo = kmalloc(sizeof(*pinfo), GFP_KERNEL);
		if (!pinfo)
			return -ENOMEM;
		pinfo->fpin_payload.hba_wwn = hba_pwwn;
		pinfo->fpin_payload.length = FC_PAYLOAD_MAXLEN;
		memcpy(pinfo->fpin_payload.payload, payload,
			FC_PAYLOAD_MAXLEN);
		spin_lock_irqsave(&els_lock, flags);
		list_add_tail(&pinfo->els_list, &fc_transport_els_list);
		spin_unlock_irqrestore(&els_lock, flags);
		wake_up_interruptible(&fctxpd_readq);
		return 0;
	}

	return -EINVAL;
}
EXPORT_SYMBOL_GPL(update_els_frame);

static long
fctxpd_ioctl(struct file *filp, unsigned int cmd_in, unsigned long arg)
{
	void __user *p = (void __user *)arg;
	struct hba_port_wwn_info port;
	struct fpin_support_hba *p_fhba;
	struct list_head *r;
	struct list_head *q;

	switch (cmd_in) {
	case FCTXPD_FAILBACK_IO:
		if (!access_ok(VERIFY_READ, p,
			sizeof(struct hba_port_wwn_info)))
			return -EFAULT;

		if (__copy_from_user(&port, p,
			sizeof(struct hba_port_wwn_info)))
			return -EFAULT;

		list_for_each_safe(r, q, &hba_support_fpin_list) {
			p_fhba = list_entry(r, struct fpin_support_hba,
					hba_list);
			if (p_fhba->fc_flush_pending_io(&port))
				return 1;
		}
		break;
	default:
		break;
	}
	return 0;
}

static bool
is_data_available(void)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&els_lock, flags);

	if (list_empty(&fc_transport_els_list)) {
		spin_unlock_irqrestore(&els_lock, flags);
		return false;
	}
	spin_unlock_irqrestore(&els_lock, flags);
	return true;
}

static unsigned int fctxpd_poll(struct file *filp, poll_table *wait)
{
	unsigned int ret = 0;

	poll_wait(filp, &fctxpd_readq, wait);
	if (is_data_available())
		ret |= POLLIN | POLLRDNORM;
	return ret;
}

static int fctxpd_open(struct inode *inode, struct file *file)
{
	if (fctxpd_status & FCTXPD_IS_OPEN)
		return -EBUSY;

	fctxpd_status |=  FCTXPD_IS_OPEN;
	return 0;
}

static int fctxpd_close(struct inode *inode, struct file *file)
{
	fctxpd_status = 0;
	return 0;
}


static ssize_t fctxpd_read(struct file *filp,
			char *buffer, size_t len, loff_t *offs)
{
	int ret = 0;
	struct fpin_els_info *pinfo = NULL;
	unsigned long flags = 0;

	if (len >  sizeof(fpin_payload_t))
		return -EINVAL;

	spin_lock_irqsave(&els_lock, flags);
	if (!list_empty(&fc_transport_els_list)) {
		pinfo = list_first_entry(&fc_transport_els_list,
			struct fpin_els_info, els_list);
		spin_unlock_irqrestore(&els_lock, flags);
		ret = copy_to_user(buffer, &pinfo->fpin_payload, len);
		/*Delete the data from the list once it is
		 *pushed to the user
		 */
		spin_lock_irqsave(&els_lock, flags);
		list_del(&pinfo->els_list);
		spin_unlock_irqrestore(&els_lock, flags);
		kfree(pinfo);
		pinfo = NULL;
		return len;
	}
	spin_unlock_irqrestore(&els_lock, flags);
	return ret;
}

/*
 * Define the LPFC driver file operations.
 */

static const struct file_operations fctxpd_fops = {
owner: THIS_MODULE,
read : fctxpd_read,
poll : fctxpd_poll,
open : fctxpd_open,
unlocked_ioctl:fctxpd_ioctl,
release : fctxpd_close,
};

static struct miscdevice fctxpd_dev = {
	FCTXPD_MINOR,
	"fctxpd",
	&fctxpd_fops
};

int fc_register_els(int (*fc_reg_flush_pending_io)
			(struct hba_port_wwn_info *port_info))
{
	struct fpin_support_hba *p_fhba = NULL;

	p_fhba = kmalloc(sizeof(*p_fhba), GFP_ATOMIC);
	if (!p_fhba)
		return -ENOMEM;
	p_fhba->fc_flush_pending_io = fc_reg_flush_pending_io;
	list_add_tail(&p_fhba->hba_list, &hba_support_fpin_list);
	return 0;
}
EXPORT_SYMBOL(fc_register_els);
/*
 * Create the Character device which FCTXPD will use
 */
static int __init
fctxpd_dev_init(void)
{
	int ret = 0;

	ret = misc_register(&fctxpd_dev);
	if (ret)
		return ret;
	return 0;
}
static void __exit
fctxpd_dev_cleanup(void)
{
	struct fpin_els_info *pinfo = NULL;
	struct fpin_support_hba *p_fhba = NULL;
	unsigned long flags = 0;

	spin_lock_irqsave(&els_lock, flags);
	while (!list_empty(&fc_transport_els_list)) {
		pinfo = list_first_entry(&fc_transport_els_list,
			struct fpin_els_info, els_list);
		list_del(&pinfo->els_list);
		kfree(pinfo);
		pinfo = NULL;
	}
	spin_unlock_irqrestore(&els_lock, flags);
	while (!list_empty(&hba_support_fpin_list)) {
		p_fhba = list_first_entry(&hba_support_fpin_list,
			struct fpin_support_hba, hba_list);
		list_del(&p_fhba->hba_list);
		kfree(p_fhba);
		p_fhba = NULL;
	}

	misc_deregister(&fctxpd_dev);
}

module_init(fctxpd_dev_init);
module_exit(fctxpd_dev_cleanup);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Broadcom");
