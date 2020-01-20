/*
 * @file mst_ctrl.c
 * @brief MST stop control node
 * Copyright (c) 2015, Samsung Electronics Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mman.h>
#include <linux/random.h>
#include <linux/init.h>
#include <linux/raw.h>
#include <linux/tty.h>
#include <linux/capability.h>
#include <linux/ptrace.h>
#include <linux/device.h>
#include <linux/highmem.h>
#include <linux/crash_dump.h>
#include <linux/backing-dev.h>
#include <linux/bootmem.h>
#include <linux/splice.h>
#include <linux/pfn.h>
#include <linux/export.h>
#include <linux/seq_file.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#include <linux/delay.h>

/* Mar 9, 2016, limit the length of string to 1 byte: "0" or "1"*/
#define MST_CTRL_CMD_LEN ((size_t)1)

extern void mst_ctrl_of_mst_hw_onoff(bool on);

ssize_t mst_ctrl_write(struct file *file, const char __user *buffer,
		       size_t size, loff_t *offset)
{

	unsigned long mode;
	char *string;

	if (size == 0)
		return 0;

	printk(KERN_ERR " %s\n", __FUNCTION__);

	/* size includes space for NULL, so size will be "number of buffer char. + 1" */
	string = kmalloc(MST_CTRL_CMD_LEN + 1, GFP_KERNEL);
	if (string == NULL) {
		printk(KERN_ERR "%s failed kmalloc\n", __func__);
		return 0;
	}

	if (copy_from_user(string, buffer, MST_CTRL_CMD_LEN) != 0) {
		printk(KERN_ERR "%s failed copy_from_user\n", __func__);
		kfree(string);
		return 0;
	}

	string[MST_CTRL_CMD_LEN] = '\0';

	if (kstrtoul(string, 0, &mode)) {
		memset(string, 0, MST_CTRL_CMD_LEN);
		kfree(string);
		return 0;
	};

	memset(string, 0, MST_CTRL_CMD_LEN);
	kfree(string);

	printk(KERN_ERR "id: %d\n", (int)mode);

	switch (mode) {
	case 1:
		printk(KERN_ERR " %s -> Notify secure world that NFS starts.\n",
		       __FUNCTION__);
		mst_ctrl_of_mst_hw_onoff(0);
		printk(KERN_ERR
		       " %s -> nfc_status is on / mst_status is off.\n",
		       __FUNCTION__);
		break;
	case 0:
		printk(KERN_ERR " %s -> Notify secure world that NFS ends.\n",
		       __FUNCTION__);
		mst_ctrl_of_mst_hw_onoff(1);
		printk(KERN_ERR " %s -> nfc_status is off.\n", __FUNCTION__);
		break;
	default:
		printk(KERN_ERR " %s -> Invalid mst operations\n",
		       __FUNCTION__);
		break;
	}

	return size;
}

static int mst_ctrl_read(struct seq_file *m, void *v)
{
	return 0;
}

static int mst_ctrl_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, mst_ctrl_read, NULL);
}

long mst_ctrl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	/*
	 * Switch according to the ioctl called
	 */
	switch (cmd) {
	case 1:
		printk(KERN_ERR
		       " %s -> [ioctl] Notify secure world that NFS starts.\n",
		       __FUNCTION__);
		mst_ctrl_of_mst_hw_onoff(0);
		printk(KERN_ERR
		       " %s -> [ioctl] nfc_status is on / mst_status is off.\n",
		       __FUNCTION__);
		break;
	case 0:
		printk(KERN_ERR
		       " %s -> [ioctl] Notify secure world that NFS ends.\n",
		       __FUNCTION__);
		mst_ctrl_of_mst_hw_onoff(1);
		printk(KERN_ERR " %s -> [ioctl] nfc_status is off.\n",
		       __FUNCTION__);
		break;
	default:
		printk(KERN_ERR " %s -> [ioctl] Invalid mst ctrl operations\n",
		       __FUNCTION__);
		return -1;
		break;
	}

	return 0;
}

const struct file_operations mst_ctrl_fops = {
	.open = mst_ctrl_open,
	.release = single_release,
	.read = seq_read,
	.write = mst_ctrl_write,
	.unlocked_ioctl = mst_ctrl_ioctl,
};
