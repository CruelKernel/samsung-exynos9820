/*
 * sec_debug_first_kmsg.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */
#include <linux/uaccess.h>
#include <linux/proc_fs.h>

#include <linux/sec_debug.h>
#include <linux/kthread.h>

extern void register_first_kmsg_hook_func(void (*)(const char *, size_t));
static char *first_kmsg_base;
static long first_kmsg_size;
static int ready;
static char *curr_ptr;
size_t max;

/* return true: in case ptr exceed buffer */
static inline bool check_eob(char *buf, size_t size)
{
	if (((size_t)buf + size) > max)
		return true;
	else
		return false;
}

/*
 * hook kernel log to (first_kmsg_base + curr_ptr)
 * until first_kmsg_base + first_kmsg_size(2MB)
 */
static void sec_debug_first_kmsg_hook(const char *buf, size_t size)
{
	if (unlikely(check_eob(curr_ptr, size) != true )) {
		memcpy(curr_ptr, buf, size);
		curr_ptr += size;
	}

	return;
}

int __init sec_first_kmsg_init(void)
{
	first_kmsg_size = sec_debug_get_buf_size(SDN_MAP_FIRST_KMSG);
	first_kmsg_base = (char *)phys_to_virt(sec_debug_get_buf_base(SDN_MAP_FIRST_KMSG));

	max = (size_t)(first_kmsg_base + first_kmsg_size);
	curr_ptr = first_kmsg_base;

	pr_info("%s: base: %p, size: 0x%lx\n", __func__, first_kmsg_base, first_kmsg_size);

	if (!first_kmsg_base || !first_kmsg_size)
		return 0;

	memset(first_kmsg_base, 0, first_kmsg_size);
	register_first_kmsg_hook_func(sec_debug_first_kmsg_hook);

	ready = 1;

	return 0;
}
early_initcall(sec_first_kmsg_init);

static ssize_t sec_first_kmsg_read(struct file *file, char __user *buf,
				  size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if (!ready) {
		pr_err("first_kmsg is not ready\n");
		return 0;
	}

	if (pos >= first_kmsg_size)
		return 0;

	count = min(len, (size_t)(first_kmsg_size - pos));

	if (copy_to_user(buf, first_kmsg_base + pos, count)) {
		pr_crit("%s: failed to copy to user\n", __func__);
		return -EFAULT;
	}

	*offset += count;

	return count;
}

static const struct file_operations first_kmsg_file_ops = {
	.owner = THIS_MODULE,
	.read = sec_first_kmsg_read,
};

static int __init sec_first_kmsg_late_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("first_kmsg", S_IFREG | 0440,
			    NULL, &first_kmsg_file_ops);
	if (!entry) {
		pr_err("%s: failed to create proc entry\n", __func__);
		return 0;
	}
	proc_set_size(entry, first_kmsg_size);
	pr_info("%s: success to create proc entry\n", __func__);

	return 0;
}
late_initcall(sec_first_kmsg_late_init);
