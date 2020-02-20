/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * Samsung TN debugging code
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/bootmem.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/sched/clock.h>
#ifdef CONFIG_NO_BOOTMEM
#include <linux/memblock.h>
#endif
#include <linux/sec_debug.h>

#ifdef CONFIG_KNOX_KAP
extern int boot_mode_security;
#endif

/*
 * Example usage: sec_log=256K@0x45000000
 * In above case, log_buf size is 256KB and its base address is
 * 0x45000000 physically. Actually, *(int *)(base - 8) is log_magic and
 * *(int *)(base - 4) is log_ptr. So we reserve (size + 8) bytes from
 * (base - 8).
 */
#define LOG_MAGIC 0x4d474f4c	/* "LOGM" */

#ifdef CONFIG_SEC_DEBUG_TSP_LOG
static unsigned int *sec_tsp_log_ptr;
static char *sec_tsp_log_buf;
static unsigned int sec_tsp_log_size;

static int sec_tsp_raw_data_index;
static char *sec_tsp_raw_data_buf;
static unsigned int sec_tsp_raw_data_size;

static int sec_tsp_command_history_index;
static char *sec_tsp_command_history_buf;
static unsigned int sec_tsp_command_history_size;

static int __init sec_tsp_log_setup(char *str)
{
	unsigned int size = memparse(str, &str);
	unsigned long base = 0;
	unsigned int *sec_tsp_log_mag;
	/* If we encounter any problem parsing str ... */
	if (!size || size != roundup_pow_of_two(size) ||
	    *str != '@' || kstrtoul(str + 1, 0, &base))
		goto out;

#ifdef CONFIG_NO_BOOTMEM
	if (memblock_is_region_reserved(base - 8, size + 8) ||
	    memblock_reserve(base - 8, size + 8)) {
#else
	if (reserve_bootmem(base - 8, size + 8, BOOTMEM_EXCLUSIVE)) {
#endif
		pr_err("%s: failed reserving size %d at base 0x%lx\n",
		       __func__, size, base);
		goto out;
	}

	sec_tsp_log_mag = phys_to_virt(base) - 8;
	sec_tsp_log_ptr = phys_to_virt(base) - 4;
	sec_tsp_log_buf = phys_to_virt(base);
	sec_tsp_log_size = size;

	if (*sec_tsp_log_mag != LOG_MAGIC) {
		pr_info("%s: no old log found\n", __func__);
		*sec_tsp_log_ptr = 0;
		*sec_tsp_log_mag = LOG_MAGIC;
	}
	return 1;
out:
	return 0;
}
__setup("sec_tsp_log=", sec_tsp_log_setup);

static int sec_tsp_log_timestamp(unsigned long idx)
{
	/* Add the current time stamp */
	char tbuf[50];
	unsigned int tlen;
	unsigned long long t;
	unsigned long nanosec_rem;

	t = local_clock();
	nanosec_rem = do_div(t, 1000000000);
	tlen = sprintf(tbuf, "[%5lu.%06lu] ", (unsigned long)t,
		       nanosec_rem / 1000);

	/* Overflow buffer size */
	if (idx + tlen > sec_tsp_log_size - 1) {
		tlen = scnprintf(&sec_tsp_log_buf[0],
				 tlen + 1, "%s", tbuf);
		*sec_tsp_log_ptr = tlen;
	} else {
		tlen = scnprintf(&sec_tsp_log_buf[idx], tlen + 1, "%s", tbuf);
		*sec_tsp_log_ptr += tlen;
	}

	return *sec_tsp_log_ptr;
}

static int sec_tsp_raw_data_timestamp(unsigned long idx)
{
	/* Add the current time stamp */
	char tbuf[50];
	unsigned int tlen;
	unsigned long long t;
	unsigned long nanosec_rem;

	t = local_clock();
	nanosec_rem = do_div(t, 1000000000);
	tlen = snprintf(tbuf, sizeof(tbuf), "[%5lu.%06lu] ",
			(unsigned long)t,
			nanosec_rem / 1000);

	/* Overflow buffer size */
	if (idx + tlen > sec_tsp_raw_data_size - 1) {
		tlen = scnprintf(&sec_tsp_raw_data_buf[0], tlen + 1, "%s", tbuf);
		sec_tsp_raw_data_index = tlen;
	} else {
		tlen = scnprintf(&sec_tsp_raw_data_buf[idx], tlen + 1, "%s", tbuf);
		sec_tsp_raw_data_index += tlen;
	}

	return sec_tsp_raw_data_index;
}

#define TSP_BUF_SIZE 512
void sec_debug_tsp_log(char *fmt, ...)
{
	va_list args;
	char buf[TSP_BUF_SIZE];
	int len = 0;
	unsigned long idx;
	unsigned long size;

	/* In case of sec_tsp_log_setup is failed */
	if (!sec_tsp_log_size)
		return;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	idx = *sec_tsp_log_ptr;
	size = strlen(buf);

	idx = sec_tsp_log_timestamp(idx);

	/* Overflow buffer size */
	if (idx + size > sec_tsp_log_size - 1) {
		len = scnprintf(&sec_tsp_log_buf[0],
				size + 1, "%s\n", buf);
		*sec_tsp_log_ptr = len;
	} else {
		len = scnprintf(&sec_tsp_log_buf[idx], size + 1, "%s\n", buf);
		*sec_tsp_log_ptr += len;
	}
}
EXPORT_SYMBOL(sec_debug_tsp_log);

void sec_debug_tsp_raw_data(char *fmt, ...)
{
	va_list args;
	char buf[TSP_BUF_SIZE];
	int len = 0;
	unsigned int idx;
	unsigned long size;

	/* In case of sec_tsp_log_setup is failed */
	if (!sec_tsp_raw_data_size || !sec_tsp_raw_data_buf)
		return;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	idx = sec_tsp_raw_data_index;
	size = strlen(buf);

	idx = sec_tsp_raw_data_timestamp(idx);

	/* Overflow buffer size */
	if (idx + size > sec_tsp_raw_data_size - 1) {
		len = scnprintf(&sec_tsp_raw_data_buf[0],
				size + 1, "%s\n", buf);
		sec_tsp_raw_data_index = len;
	} else {
		len = scnprintf(&sec_tsp_raw_data_buf[idx], size + 1, "%s\n", buf);
		sec_tsp_raw_data_index += len;
	}
}
EXPORT_SYMBOL(sec_debug_tsp_raw_data);

void sec_debug_tsp_log_msg(char *msg, char *fmt, ...)
{
	va_list args;
	char buf[TSP_BUF_SIZE];
	int len = 0;
	unsigned int idx;
	size_t size;
	size_t size_dev_name;

	/* In case of sec_tsp_log_setup is failed */
	if (!sec_tsp_log_size)
		return;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	idx = *sec_tsp_log_ptr;
	size = strlen(buf);
	size_dev_name = strlen(msg);

	idx = sec_tsp_log_timestamp(idx);

	/* Overflow buffer size */
	if (idx + size + size_dev_name + 3 + 1 > sec_tsp_log_size) {
		len = scnprintf(&sec_tsp_log_buf[0],
				size + size_dev_name + 3 + 1,
				"%s : %s", msg, buf);
		*sec_tsp_log_ptr = len;
	} else {
		len = scnprintf(&sec_tsp_log_buf[idx],
				size + size_dev_name + 3 + 1,
				"%s : %s", msg, buf);
		*sec_tsp_log_ptr += len;
	}
}
EXPORT_SYMBOL(sec_debug_tsp_log_msg);

void sec_debug_tsp_raw_data_msg(char *msg, char *fmt, ...)
{
	va_list args;
	char buf[TSP_BUF_SIZE];
	int len = 0;
	unsigned int idx;
	size_t size;
	size_t size_dev_name;

	/* In case of sec_tsp_log_setup is failed */
	if (!sec_tsp_raw_data_size || !sec_tsp_raw_data_buf)
		return;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	idx = sec_tsp_raw_data_index;
	size = strlen(buf);
	size_dev_name = strlen(msg);

	idx = sec_tsp_raw_data_timestamp(idx);

	/* Overflow buffer size */
	if (idx + size + size_dev_name + 3 + 1 > sec_tsp_raw_data_size) {
		len = scnprintf(&sec_tsp_raw_data_buf[0],
				size + size_dev_name + 3 + 1, "%s : %s", msg, buf);
		sec_tsp_raw_data_index = len;
	} else {
		len = scnprintf(&sec_tsp_raw_data_buf[idx],
				size + size_dev_name + 3 + 1, "%s : %s", msg, buf);
		sec_tsp_raw_data_index += len;
	}
}
EXPORT_SYMBOL(sec_debug_tsp_raw_data_msg);

void sec_debug_tsp_command_history(char *buf)
{
	int len = 0;
	unsigned int idx;
	size_t size;

	/* In case of sec_tsp_log_setup is failed */
	if (!sec_tsp_command_history_size || !sec_tsp_command_history_buf)
		return;

	idx = sec_tsp_command_history_index;
	size = strlen(buf);

	/* Overflow buffer size */
	if (idx + size + 1 > sec_tsp_command_history_size) {
		len = scnprintf(&sec_tsp_command_history_buf[0],
			size + 1, "%s ", buf);
		sec_tsp_command_history_index = len;
	} else {
		len = scnprintf(&sec_tsp_command_history_buf[idx],
			size + 1, "%s ", buf);
		sec_tsp_command_history_index += len;
	}
}
EXPORT_SYMBOL(sec_debug_tsp_command_history);

void sec_tsp_raw_data_clear(void)
{
	if (!sec_tsp_raw_data_size || !sec_tsp_raw_data_buf)
		return;

	sec_tsp_raw_data_index = 0;
	memset(sec_tsp_raw_data_buf, 0x00, sec_tsp_raw_data_size);
}
EXPORT_SYMBOL(sec_tsp_raw_data_clear);

static ssize_t sec_tsp_log_write(struct file *file,
				 const char __user *buf,
				 size_t count, loff_t *ppos)
{
	char *page = NULL;
	ssize_t ret;
	int new_value;

	if (!sec_tsp_log_buf)
		return 0;

	ret = -ENOMEM;
	if (count >= PAGE_SIZE)
		return ret;

	ret = -ENOMEM;
	page = (char *)get_zeroed_page(GFP_KERNEL);
	if (!page)
		return ret;

	ret = -EFAULT;
	if (copy_from_user(page, buf, count))
		goto out;

	ret = -EINVAL;
	if (sscanf(page, "%u", &new_value) != 1) {
		pr_info("%s\n", page);
		/* print tsp_log to sec_tsp_log_buf */
		sec_debug_tsp_log("%s", page);
	}
	ret = count;
out:
	free_page((unsigned long)page);

	return ret;
}

static ssize_t sec_tsp_raw_data_write(struct file *file,
				      const char __user *buf,
				      size_t count, loff_t *ppos)
{
	char *page = NULL;
	ssize_t ret;
	int new_value;

	if (!sec_tsp_raw_data_buf)
		return 0;

	ret = -EINVAL;
	if (count >= PAGE_SIZE)
		return ret;

	ret = -ENOMEM;
	page = (char *)get_zeroed_page(GFP_KERNEL);
	if (!page)
		return ret;

	ret = -EFAULT;
	if (copy_from_user(page, buf, count))
		goto out;

	ret = -EINVAL;
	if (sscanf(page, "%u", &new_value) != 1) {
		pr_info("%s\n", page);
		sec_debug_tsp_raw_data("%s", page);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

static ssize_t sec_tsp_log_read(struct file *file, char __user *buf,
				size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if (!sec_tsp_log_buf)
		return 0;

	if (pos >= *sec_tsp_log_ptr)
		return 0;

	count = min(len, (size_t)(*sec_tsp_log_ptr - pos));
	if (copy_to_user(buf, sec_tsp_log_buf + pos, count))
		return -EFAULT;
	*offset += count;
	return count;
}

static ssize_t sec_tsp_raw_data_read(struct file *file, char __user *buf,
				     size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if (!sec_tsp_raw_data_buf)
		return 0;

	if (pos >= sec_tsp_raw_data_index)
		return 0;

	count = min(len, (size_t)(sec_tsp_raw_data_index - pos));
	if (copy_to_user(buf, sec_tsp_raw_data_buf + pos, count))
		return -EFAULT;
	*offset += count;
	return count;
}

static ssize_t sec_tsp_command_history_read(struct file *file, char __user *buf,
					size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if (!sec_tsp_command_history_buf)
		return 0;

	if (pos >= sec_tsp_command_history_index)
		return 0;

	count = min(len, (size_t)(sec_tsp_command_history_index - pos));
	if (copy_to_user(buf, sec_tsp_command_history_buf + pos, count))
		return -EFAULT;
	*offset += count;
	return count;
}

static const struct file_operations tsp_msg_file_ops = {
	.owner = THIS_MODULE,
	.read = sec_tsp_log_read,
	.write = sec_tsp_log_write,
	.llseek = generic_file_llseek,
};

static const struct file_operations tsp_raw_data_file_ops = {
	.owner = THIS_MODULE,
	.read = sec_tsp_raw_data_read,
	.write = sec_tsp_raw_data_write,
	.llseek = generic_file_llseek,
};

static const struct file_operations tsp_command_history_file_ops = {
	.owner = THIS_MODULE,
	.read = sec_tsp_command_history_read,
	.llseek = generic_file_llseek,
};

static int __init sec_tsp_log_late_init(void)
{
	struct proc_dir_entry *entry;

	if (!sec_tsp_log_buf)
		return 0;

	entry = proc_create("tsp_msg", S_IFREG | 0444,
			    NULL, &tsp_msg_file_ops);
	if (!entry) {
		pr_err("%s: failed to create proc entry\n", __func__);
		return 0;
	}

	proc_set_size(entry, sec_tsp_log_size);

	return 0;
}
late_initcall(sec_tsp_log_late_init);

static int __init sec_tsp_raw_data_late_init(void)
{
	struct proc_dir_entry *entry2;

	if (!sec_tsp_raw_data_buf)
		return 0;

	entry2 = proc_create("tsp_raw_data", S_IFREG | 0444,
			     NULL, &tsp_raw_data_file_ops);
	if (!entry2) {
		pr_err("%s: failed to create proc entry of tsp_raw_data\n", __func__);
		return 0;
	}

	proc_set_size(entry2, sec_tsp_raw_data_size);

	return 0;
}
late_initcall(sec_tsp_raw_data_late_init);

static int __init sec_tsp_command_history_late_init(void)
{
	struct proc_dir_entry *entry;

	if (!sec_tsp_command_history_buf)
		return 0;

	entry = proc_create("tsp_cmd_hist", S_IFREG | 0444,
			NULL, &tsp_command_history_file_ops);
	if (!entry) {
		pr_err("%s: failed to create proc entry of tsp_command_history\n", __func__);
		return 0;
	}

	proc_set_size(entry, sec_tsp_command_history_size);

	return 0;
}
late_initcall(sec_tsp_command_history_late_init);

#define SEC_TSP_RAW_DATA_BUF_SIZE	(50 * 1024)	/* 50 KB */
static int __init __init_sec_tsp_raw_data(void)
{
	char *vaddr;

	sec_tsp_raw_data_size = SEC_TSP_RAW_DATA_BUF_SIZE;
	vaddr = kmalloc(sec_tsp_raw_data_size, GFP_KERNEL);

	if (!vaddr) {
		pr_info("%s: ERROR! init failed!\n", __func__);
		return -ENOMEM;
	}

	sec_tsp_raw_data_buf = vaddr;

	pr_info("%s: init done\n", __func__);

	return 0;
}
fs_initcall(__init_sec_tsp_raw_data);	/* earlier than device_initcall */

#define SEC_TSP_COMMAND_HISTORY_BUF_SIZE	(10 * 1024)	/* 10 KB */
static int __init __init_sec_tsp_command_history(void)
{
	char *vaddr;

	sec_tsp_command_history_size = SEC_TSP_COMMAND_HISTORY_BUF_SIZE;
	vaddr = kmalloc(sec_tsp_command_history_size, GFP_KERNEL);

	if (!vaddr) {
		pr_info("%s: ERROR! init failed!\n", __func__);
		return -ENOMEM;
	}

	sec_tsp_command_history_buf = vaddr;

	pr_info("%s: init done\n", __func__);

	return 0;
}
fs_initcall(__init_sec_tsp_command_history);	/* earlier than device_initcall */

#endif /* CONFIG_SEC_DEBUG_TSP_LOG */

