/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * Samsung TN debugging code
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/sec_debug.h>

static char *last_kmsg_buffer;
static size_t last_kmsg_size;

void sec_debug_save_last_kmsg(unsigned char *head_ptr, unsigned char *curr_ptr, size_t buf_size)
{
	size_t size;
	unsigned char *magickey_addr;

	if (!head_ptr || !curr_ptr || head_ptr == curr_ptr) {
		pr_err("%s: no data\n", __func__);
		return;
	}

	if ((curr_ptr - head_ptr) <= 0) {
		pr_err("%s: invalid args\n", __func__);
		return;
	}
	size = (size_t)(curr_ptr - head_ptr);

	magickey_addr = head_ptr + buf_size - (size_t)0x08;

	/* provide previous log as last_kmsg */
	if (*((unsigned long long *)magickey_addr) == SEC_LKMSG_MAGICKEY) {
		pr_info("%s: sec_log buffer is full\n", __func__);
		last_kmsg_size = (size_t)SZ_2M;
		last_kmsg_buffer = kzalloc(last_kmsg_size, GFP_NOWAIT);
		if (last_kmsg_size && last_kmsg_buffer) {
			memcpy(last_kmsg_buffer, curr_ptr, last_kmsg_size - size);
			memcpy(last_kmsg_buffer + (last_kmsg_size - size), head_ptr, size);
			pr_info("%s: succeeded\n", __func__);
		} else {
			pr_err("%s: failed\n", __func__);
		}
	} else {
		pr_info("%s: sec_log buffer is not full\n", __func__);
		last_kmsg_size = size;
		last_kmsg_buffer = kzalloc(last_kmsg_size, GFP_NOWAIT);
		if (last_kmsg_size && last_kmsg_buffer) {
			memcpy(last_kmsg_buffer, head_ptr, last_kmsg_size);
			pr_info("%s: succeeded\n", __func__);
		} else {
			pr_err("%s: failed\n", __func__);
		}
	}
}

static ssize_t sec_last_kmsg_read(struct file *file, char __user *buf,
				  size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if (pos >= last_kmsg_size)
		return 0;

	count = min(len, (size_t)(last_kmsg_size - pos));
	if (copy_to_user(buf, last_kmsg_buffer + pos, count))
		return -EFAULT;

	*offset += count;
	return count;
}

static const struct file_operations last_kmsg_file_ops = {
	.owner = THIS_MODULE,
	.read = sec_last_kmsg_read,
};

static int __init sec_last_kmsg_late_init(void)
{
	struct proc_dir_entry *entry;

	if (!last_kmsg_buffer)
		return 0;

	entry = proc_create("last_kmsg", S_IFREG | 0444,
			    NULL, &last_kmsg_file_ops);
	if (!entry) {
		pr_err("%s: failed to create proc entry\n", __func__);
		return 0;
	}

	pr_info("%s: success to create proc entry\n", __func__);

	proc_set_size(entry, last_kmsg_size);
	return 0;
}

late_initcall(sec_last_kmsg_late_init);
