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
#include <linux/poll.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <scsi/fc/fc_transport_adapter.h>

static const struct file_operations fctxpd_fops;
static int fctxpd_open(struct inode *inode, struct file *file);
static int fctxpd_close(struct inode *inode, struct file *file);
static ssize_t fctxpd_read(struct file *file, char *buf, size_t size,
				loff_t *off);
static unsigned int fctxpd_poll(struct file *file, poll_table *wait);

struct els_list {
	struct list_head    active_els;
	fpin_payload_t *fpin_payload;
};

struct list_head    fc_transport_els_list;
static DEFINE_SPINLOCK(els_lock);

#define LOCK(x, y)	spin_lock_irqsave((x), (y))
#define UNLOCK(x, y)	spin_unlock_irqrestore((x), (y))
#define LOCK_INIT(x)	spin_lock_init((x))
#define FCTXPD_IS_OPEN 0x1
static unsigned long fctxpd_status;	/* bitmapped status byte.	*/

DECLARE_WAIT_QUEUE_HEAD(fctxpd_readq);

int update_els_frame(uint64_t hba_pwwn, void *payload)
{
	struct els_list *plist = NULL;
	unsigned long flags = 0;
	int fpin_payload_sz = sizeof(fpin_payload_t) + FC_PAYLOAD_MAXLEN;

	if (payload != NULL) {

		plist = kmalloc(sizeof(*plist), GFP_ATOMIC);
		if (!plist)
			return -ENOMEM;
		plist->fpin_payload = kmalloc(fpin_payload_sz, GFP_ATOMIC);
		if (plist->fpin_payload == NULL) {
			kfree(plist);
			return -ENOMEM;
		}
		plist->fpin_payload->hba_wwn = hba_pwwn;
		plist->fpin_payload->length = FC_PAYLOAD_MAXLEN;
		memcpy(plist->fpin_payload->payload, payload,
			FC_PAYLOAD_MAXLEN);
		LOCK(&els_lock, flags);
		list_add_tail(&plist->active_els, &fc_transport_els_list);
		UNLOCK(&els_lock, flags);
		wake_up_interruptible(&fctxpd_readq);
		return 0;
	}

	return -ENOMEM;
}
EXPORT_SYMBOL_GPL(update_els_frame);
/*
 * Define the LPFC driver file operations.
 */

static const struct file_operations fctxpd_fops = {
owner: THIS_MODULE,
read : fctxpd_read,
poll : fctxpd_poll,
open : fctxpd_open,
release : fctxpd_close,
};

static bool
will_read_block(void)
{
	unsigned long flags = 0;

	LOCK(&els_lock, flags);

	if (list_empty(&fc_transport_els_list)) {
		UNLOCK(&els_lock, flags);
		return false;
	}
	UNLOCK(&els_lock, flags);
	return true;
}
static unsigned int fctxpd_poll(struct file *filp, poll_table *wait)
{
	unsigned int ret = 0;

	poll_wait(filp, &fctxpd_readq, wait);
	if (will_read_block())
		ret |= POLLIN | POLLRDNORM;
	return ret;
}
static int fctxpd_open(struct inode *inode, struct file *file)
{
	try_module_get(THIS_MODULE);
	if (fctxpd_status & FCTXPD_IS_OPEN)
		return -EBUSY;
	/* TBD: Register for FPIN-LI Registration here */
	fctxpd_status |=  FCTXPD_IS_OPEN;
	return 0;
}

static int fctxpd_close(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE);
	fctxpd_status = 0;
	return 0;
}


static ssize_t fctxpd_read(struct file *filp,
			char *buffer, size_t len, loff_t *offs)
{
	int ret = 0;
	struct els_list *plist = NULL;
	unsigned long flags = 0;

	LOCK(&els_lock, flags);
	if (!list_empty(&fc_transport_els_list)) {
		plist = list_first_entry(&fc_transport_els_list,
			struct els_list, active_els);
		ret = copy_to_user(buffer, plist->fpin_payload, len);
		list_del(&plist->active_els);
		kfree(plist->fpin_payload);
		kfree(plist);
		plist = NULL;
		ret = len;
	}
	UNLOCK(&els_lock, flags);
	return ret;
}

static struct miscdevice fctxpd_dev = {
	FCTXPD_MINOR,
	"fctxpd",
	&fctxpd_fops
};

/*
 * Create the Character device which FCTXPD will use
 */
static int __init
fctxpd_dev_init(void)
{
	int ret = 0;

	ret = misc_register(&fctxpd_dev);
	if (ret)
		return -ENODEV;
	/* Initialize the Sempahore */
	INIT_LIST_HEAD(&fc_transport_els_list);
	LOCK_INIT(&els_lock);
	return 0;
}
static void __exit
fctxpd_dev_cleanup(void)
{
	misc_deregister(&fctxpd_dev);
}

module_init(fctxpd_dev_init);
module_exit(fctxpd_dev_cleanup);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Broadcom");
