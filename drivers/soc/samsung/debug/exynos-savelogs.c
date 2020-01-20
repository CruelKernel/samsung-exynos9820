/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>
#include <linux/debug-snapshot.h>
#include <linux/debug-snapshot-helper.h>

struct savelogs_desc {
	unsigned int kernel_log_size;
	unsigned long kernel_log_base;
	unsigned long kernel_ptr;
	unsigned int platform_log_size;
	unsigned long platform_log_base;
	unsigned long platform_ptr;
	struct dentry *debugfs_dir;
};

static struct savelogs_desc exynos_savelogs_desc = { 0, };

static ssize_t exynos_savelogs_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	return 0;
}

static ssize_t exynos_savelogs_kernel_read(struct file *file,
				char __user *user_buf, size_t count,
				loff_t *ppos)
{
	unsigned long curr_ptr;
	unsigned long prev_ptr;
	unsigned long base_ptr;
	unsigned long last_ptr;
	ssize_t read_cnt = 0;
	ssize_t read_cnt2 = 0;
	ssize_t ret;

	curr_ptr = dbg_snapshot_get_item_curr_ptr("log_kernel");
	if (!curr_ptr) {
		pr_info("EXYNOS SAVELOGS: %s() no log_kernel item\n", __func__);
		return 0;
	}

	prev_ptr = exynos_savelogs_desc.kernel_ptr;
	last_ptr = exynos_savelogs_desc.kernel_log_base +
					exynos_savelogs_desc.kernel_log_size;
	if (prev_ptr < curr_ptr) {
		read_cnt = curr_ptr - prev_ptr;
		ret = copy_to_user(user_buf, (const void *)prev_ptr, curr_ptr - prev_ptr);
		if (ret == read_cnt) {
			read_cnt = 0;
		} else {
			read_cnt -= ret;
			exynos_savelogs_desc.kernel_ptr = prev_ptr + read_cnt;
		}
	} else {
		read_cnt = last_ptr - prev_ptr;
		ret = copy_to_user(user_buf, (const void *)prev_ptr, last_ptr - prev_ptr);
		if (read_cnt == ret) {
			read_cnt = 0;
		} else {
			read_cnt -= ret;
			if (!ret) {
				base_ptr = exynos_savelogs_desc.kernel_log_base;
				read_cnt2 = curr_ptr - base_ptr;
				ret = copy_to_user(user_buf + read_cnt,
						(const void *)base_ptr,
						curr_ptr - base_ptr);
				if (read_cnt2 == ret)
					read_cnt2 = 0;
				else
					read_cnt2 -= ret;
				exynos_savelogs_desc.kernel_ptr = base_ptr + read_cnt2;
			} else {
				exynos_savelogs_desc.kernel_ptr = prev_ptr + read_cnt;
			}
		}
		read_cnt += read_cnt2;
	}

	return read_cnt;
}

static ssize_t exynos_savelogs_platform_read(struct file *file,
				char __user *user_buf, size_t count,
				loff_t *ppos)
{
	unsigned long curr_ptr;
	unsigned long prev_ptr;
	unsigned long base_ptr;
	unsigned long last_ptr;
	ssize_t read_cnt = 0;
	ssize_t read_cnt2 = 0;
	ssize_t ret;

	curr_ptr = dbg_snapshot_get_item_curr_ptr("log_platform");
	if (!curr_ptr) {
		pr_info("EXYNOS SAVELOGS: %s() no log_platform item\n", __func__);
		return 0;
	}

	prev_ptr = exynos_savelogs_desc.platform_ptr;
	last_ptr = exynos_savelogs_desc.platform_log_base +
					exynos_savelogs_desc.platform_log_size;
	if (prev_ptr < curr_ptr) {
		read_cnt = curr_ptr - prev_ptr;
		ret = copy_to_user(user_buf, (const void *)prev_ptr, curr_ptr - prev_ptr);
		if (ret == read_cnt) {
			read_cnt = 0;
		} else {
			read_cnt -= ret;
			exynos_savelogs_desc.platform_ptr = prev_ptr + read_cnt;
		}
	} else {
		read_cnt = last_ptr - prev_ptr;
		ret = copy_to_user(user_buf, (const void *)prev_ptr, last_ptr - prev_ptr);
		if (read_cnt == ret) {
			read_cnt = 0;
		} else {
			read_cnt -= ret;
			if (!ret) {
				base_ptr = exynos_savelogs_desc.platform_log_base;
				read_cnt2 = curr_ptr - base_ptr;
				ret = copy_to_user(user_buf + read_cnt,
						(const void *)base_ptr,
						curr_ptr - base_ptr);
				if (read_cnt2 == ret)
					read_cnt2 = 0;
				else
					read_cnt2 -= ret;
				exynos_savelogs_desc.platform_ptr = base_ptr + read_cnt2;
			} else {
				exynos_savelogs_desc.platform_ptr = prev_ptr + read_cnt;
			}
		}
		read_cnt += read_cnt2;
	}

	return read_cnt;
}

static const struct file_operations exynos_savelogs_kernel_fops = {
	.open	= simple_open,
	.read	= exynos_savelogs_kernel_read,
	.write	= exynos_savelogs_write,
	.llseek	= default_llseek,
};

static const struct file_operations exynos_savelogs_platform_fops = {
	.open	= simple_open,
	.read	= exynos_savelogs_platform_read,
	.write	= exynos_savelogs_write,
	.llseek	= default_llseek,
};

static int __init exynos_savelogs_init(void)
{

	pr_info("EXYNOS SAVELOGS: %s() called\n", __func__);

	/* check dss enabled */
	exynos_savelogs_desc.kernel_log_size =
			dbg_snapshot_get_item_size("log_kernel");
	exynos_savelogs_desc.kernel_log_base =
			dbg_snapshot_get_item_vaddr("log_kernel");
	if (!exynos_savelogs_desc.kernel_log_size ||
			!exynos_savelogs_desc.kernel_log_base) {
		pr_info("EXYNOS SAVELOGS: %s() no log_kernel information\n", __func__);
		goto savelogs_out;
	}

	exynos_savelogs_desc.platform_log_size =
			dbg_snapshot_get_item_size("log_platform");
	exynos_savelogs_desc.platform_log_base =
			dbg_snapshot_get_item_vaddr("log_platform");
	if (!exynos_savelogs_desc.platform_log_size ||
			!exynos_savelogs_desc.platform_log_base) {
		pr_info("EXYNOS SAVELOGS: %s() no log_platform information\n", __func__);
		goto savelogs_out;
	}

	exynos_savelogs_desc.kernel_ptr =
			exynos_savelogs_desc.kernel_log_base;
	exynos_savelogs_desc.platform_ptr =
			exynos_savelogs_desc.platform_log_base;

	/* create debugfs dir & file */
	exynos_savelogs_desc.debugfs_dir =
				debugfs_create_dir("exynos_savelogs", NULL);
	if (!exynos_savelogs_desc.debugfs_dir) {
		pr_err("EXYNOS SAVELOGS: %s() cannot create debugfs dir\n", __func__);
		goto savelogs_out;
	}
	debugfs_create_file("kernel", 0644,
			exynos_savelogs_desc.debugfs_dir,
			NULL, &exynos_savelogs_kernel_fops);
	debugfs_create_file("platform", 0644,
			exynos_savelogs_desc.debugfs_dir,
			NULL, &exynos_savelogs_platform_fops);



	pr_info("EXYNOS SAVELOGS: %s() exit normally\n", __func__);

savelogs_out:
	return 0;
}
late_initcall_sync(exynos_savelogs_init);
