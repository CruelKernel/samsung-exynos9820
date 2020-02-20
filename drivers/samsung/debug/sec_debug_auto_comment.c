/*
 * sec_debug_auto_comment.c
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

#include <linux/kernel.h>
#include <linux/file.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kallsyms.h>
#include <linux/memblock.h>
#include <linux/sec_debug.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <asm/sections.h>

static struct sec_debug_auto_comm_log_idx ac_idx[SEC_DEBUG_AUTO_COMM_BUF_SIZE];
static struct sec_debug_auto_comment *auto_comment_info;
static char *auto_comment_buf;

void sec_debug_auto_comment_log_disable(int type)
{
	atomic_inc(&(ac_idx[type].logging_disable));
}

void sec_debug_auto_comment_log_once(int type)
{
	if (atomic64_read(&(ac_idx[type].logging_entry)))
		sec_debug_auto_comment_log_disable(type);
	else
		atomic_inc(&(ac_idx[type].logging_entry));
}

static inline void sec_debug_hook_auto_comm(int type, const char *buf, size_t size)
{
	struct sec_debug_auto_comm_buf *p = &auto_comment_info->auto_comm_buf[type];
	unsigned int offset = p->offset;

	if (atomic64_read(&(ac_idx[type].logging_disable)))
		return;

	if (offset + size > SZ_4K)
		return;

	if (init_data[type].max_count &&
	    (atomic64_read(&(ac_idx[type].logging_count)) > init_data[type].max_count))
		return;

	if (!(auto_comment_info->fault_flag & 1 << type)) {
		auto_comment_info->fault_flag |= 1 << type;
		if (init_data[type].prio_level == PRIO_LV5) {
			auto_comment_info->lv5_log_order |= type << auto_comment_info->lv5_log_cnt * 4;
			auto_comment_info->lv5_log_cnt++;
		}
		auto_comment_info->order_map[auto_comment_info->order_map_cnt++] = type;
	}

	atomic_inc(&(ac_idx[type].logging_count));

	memcpy(p->buf + offset, buf, size);
	p->offset += (unsigned int)size;
}

static void sec_debug_auto_comment_init_print_buf(void)
{
	auto_comment_buf = (char *)phys_to_virt(sec_debug_get_buf_base(SDN_MAP_AUTO_COMMENT));
	auto_comment_info = (struct sec_debug_auto_comment *)sec_debug_get_debug_base(SDN_MAP_AUTO_COMMENT);

	memset(auto_comment_info, 0, sizeof(struct sec_debug_auto_comment));

	register_set_auto_comm_buf(sec_debug_hook_auto_comm);

	pr_info("%s: done\n", __func__);
}

static ssize_t sec_reset_auto_comment_proc_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if (!auto_comment_buf) {
		pr_err("%s : buffer is null\n", __func__);
		return -ENODEV;
	}

	if (reset_reason >= RR_R && reset_reason <= RR_N) {
		pr_err("%s : reset_reason %d\n", __func__, reset_reason);
		return -ENOENT;
	}

	if (pos >= AC_SIZE) {
		pr_err("%s : pos 0x%llx\n", __func__, pos);
		return 0;
	}

	if (strncmp(auto_comment_buf, "@ Ramdump", 9)) {
		pr_err("%s : no data in auto_comment\n", __func__);
		return 0;
	}

	count = min(len, (size_t)(AC_SIZE - pos));
	if (copy_to_user(buf, auto_comment_buf + pos, count))
		return -EFAULT;

	*offset += count;
	return count;
}

static const struct file_operations sec_reset_auto_comment_proc_fops = {
	.owner = THIS_MODULE,
	.read = sec_reset_auto_comment_proc_read,
};

static int __init sec_debug_auto_comment_init(void)
{
	struct proc_dir_entry *entry;
	int i;

	pr_info("%s: start\n", __func__);

	sec_debug_auto_comment_init_print_buf();

	if (auto_comment_info) {
		auto_comment_info->header_magic = AC_MAGIC;
		auto_comment_info->tail_magic = AC_TAIL_MAGIC;
	}

	for (i = 0; i < SEC_DEBUG_AUTO_COMM_BUF_SIZE; i++) {
		atomic_set(&(ac_idx[i].logging_entry), 0);
		atomic_set(&(ac_idx[i].logging_disable), 0);
		atomic_set(&(ac_idx[i].logging_count), 0);
	}

	entry = proc_create("auto_comment", S_IWUGO, NULL,
			    &sec_reset_auto_comment_proc_fops);

	if (!entry)
		return -ENOMEM;

	proc_set_size(entry, AC_SIZE);

	pr_info("%s: done\n", __func__);

	return 0;
}
late_initcall(sec_debug_auto_comment_init);
