/*
 *sec_debug_reset_reason.c
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
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sec_debug.h>
#include <linux/sec_class.h>
#include <linux/soc/samsung/exynos-soc.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/stacktrace.h>
#include <linux/uaccess.h>
#include <linux/memblock.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>

unsigned int reset_reason = RR_N;

static const char *regs_bit[][8] = {
	{ "RSVD0", "TSD", "TIMEOUT", "LDO3OK", "PWRHOLD", "RSVD5", "RSVD6", "UVLOB" }, /* PWROFFSRC */
	{ "PWRON", "JIGONB", "ACOKB", "MRST", "ALARM1", "ALARM2", "SMPL", "WTSR" }, /* PWRONSRC */
}; /* S2MPS18 */

static const char *dword_regs_bit[][32] = {
	{ "CLUSTER1_DBGRSTREQ0", "CLUSTER1_DBGRSTREQ1", "CLUSTER1_DBGRSTREQ2", "CLUSTER1_DBGRSTREQ3",
	  "CLUSTER0_DBGRSTREQ0", "CLUSTER0_DBGRSTREQ1", "CLUSTER0_DBGRSTREQ2", "CLUSTER0_DBGRSTREQ3",
	  "CLUSTER1_WARMRSTREQ0", "CLUSTER1_WARMRSTREQ1", "CLUSTER1_WARMRSTREQ2", "CLUSTER1_WARMRSTREQ3",
	  "CLUSTER0_WARMRSTREQ0", "CLUSTER0_WARMRSTREQ1", "CLUSTER0_WARMRSTREQ2", "CLUSTER0_WARMRSTREQ3",
	  "PINRESET", "RSVD17", "RSVD18", "CHUB_CPU_WDTRESET",
	  "CORTEXM23_APM_WDTRESET", "CORTEXM23_APM_SYSRESET", "VTS_CPU_WDTRESET", "CLUSTER1_WDTRESET",
  	  "CLUSTER0_WDTRESET", "AUD_CA7_WDTRESET", "SSS_WDTRESET", "RSVD27",
	  "WRESET", "SWRESET", "RSVD30", "RSVD31"
	}, /* RST_STAT */
}; /* EXYNOS 9810 */

struct outbuf {
	char buf[SZ_1K];
	int index;
	int already;
};

static struct outbuf extra_buf;
static struct outbuf pwrsrc_buf;

static void write_buf(struct outbuf *obuf, int len, const char *fmt, ...)
{
	va_list list;
	char *base;
	int rem, ret;

	base = obuf->buf;
	base += obuf->index;

	rem = sizeof(obuf->buf);
	rem -= obuf->index;

	if (rem <= 0)
		return;

	if ((len > 0) && (len < rem))
		rem = len;

	va_start(list, fmt);
	ret = vsnprintf(base, rem, fmt, list);
	if (ret)
		obuf->index += ret;

	va_end(list);
}

static int __init sec_debug_set_reset_reason(char *arg)
{
	get_option(&arg, &reset_reason);
	return 0;
}

early_param("sec_debug.reset_reason", sec_debug_set_reset_reason);

static int set_debug_reset_reason_proc_show(struct seq_file *m, void *v)
{
	if (reset_reason == RR_S)
		seq_puts(m, "SPON\n");
	else if (reset_reason == RR_W)
		seq_puts(m, "WPON\n");
	else if (reset_reason == RR_D)
		seq_puts(m, "DPON\n");
	else if (reset_reason == RR_K)
		seq_puts(m, "KPON\n");
	else if (reset_reason == RR_M)
		seq_puts(m, "MPON\n");
	else if (reset_reason == RR_P)
		seq_puts(m, "PPON\n");
	else if (reset_reason == RR_R)
		seq_puts(m, "RPON\n");
	else if (reset_reason == RR_B)
		seq_puts(m, "BPON\n");
	else if (reset_reason == RR_T)
		seq_puts(m, "TPON\n");
	else
		seq_puts(m, "NPON\n");

	return 0;
}

bool is_target_reset_reason(void)
{
	if (reset_reason == RR_K ||
		reset_reason == RR_D || 
		reset_reason == RR_P ||
		reset_reason == RR_S)
		return true;

	return false;
}

static int sec_debug_reset_reason_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, set_debug_reset_reason_proc_show, NULL);
}

static const struct file_operations sec_debug_reset_reason_proc_fops = {
	.open = sec_debug_reset_reason_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int sec_debug_reset_reason_store_lastkmsg_proc_show(struct seq_file *m, void *v)
{
	if (is_target_reset_reason())
		seq_puts(m, "1\n");
	else
		seq_puts(m, "0\n");

	return 0;
}

static int sec_debug_reset_reason_store_lastkmsg_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_debug_reset_reason_store_lastkmsg_proc_show, NULL);
}

static const struct file_operations sec_debug_reset_reason_store_lastkmsg_proc_fops = {
	.open = sec_debug_reset_reason_store_lastkmsg_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*
 * proc/pwrsrc
 * OFFSRC (from PWROFF - 32) + ONSRC (from PWR - 32) + RSTSTAT (from RST - 32)
 * regs_bit (max 8) / dword_regs_bit (max 32)
 * total max : 48
 */
static int sec_debug_reset_reason_pwrsrc_show(struct seq_file *m, void *v)
{
	int i;
	char val[32] = {0, };
	unsigned long tmp;

	if (pwrsrc_buf.already)
		goto out;

	memset(&pwrsrc_buf, 0, sizeof(pwrsrc_buf));

	memset(val, 0, 32);
	get_bk_item_val_as_string("PWROFF", val);
	tmp = simple_strtol(val, NULL, 0);

	write_buf(&pwrsrc_buf, 0, "OFFSRC:");
	if (!tmp)
		write_buf(&pwrsrc_buf, 0, " -");
	else
		for (i = 0; i < 8; i++)
			if (tmp & (1 << i))
				write_buf(&pwrsrc_buf, 0, " %s", regs_bit[0][i]);

	write_buf(&pwrsrc_buf, 0, " /");

	memset(val, 0, 32);
	get_bk_item_val_as_string("PWR", val);
	tmp = simple_strtol(val, NULL, 0);

	write_buf(&pwrsrc_buf, 0, " ONSRC:");
	if (!tmp)
		write_buf(&pwrsrc_buf, 0, " -");
	else
		for (i = 0; i < 8; i++)
			if (tmp & (1 << i))
				write_buf(&pwrsrc_buf, 0, " %s", regs_bit[1][i]);

	write_buf(&pwrsrc_buf, 0, " /");

	memset(val, 0, 32);
	get_bk_item_val_as_string("RST", val);
	tmp = simple_strtol(val, NULL, 0);

	write_buf(&pwrsrc_buf, 0, " RSTSTAT:");
	if (!tmp)
		write_buf(&pwrsrc_buf, 0, " -");
	else
		for (i = 0; i < 32; i++)
			if (tmp & (1 << i))
				write_buf(&pwrsrc_buf, 0, " %s", dword_regs_bit[0][i]);

	pwrsrc_buf.already = 1;
out:
	seq_printf(m, pwrsrc_buf.buf);

	return 0;
}

static int sec_debug_reset_reason_pwrsrc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_debug_reset_reason_pwrsrc_show, NULL);
}

static const struct file_operations sec_debug_reset_reason_pwrsrc_proc_fops = {
	.open = sec_debug_reset_reason_pwrsrc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#define BBP_STR_LEN (64)

static void handle_bug_string(char *buf, char *src)
{
	int idx = 0, len, i;

	len = strlen(src);
	if (BBP_STR_LEN < len)
		len = BBP_STR_LEN;

	for (i = 0; i < len; i++) {
		if (src[i] == '/')
			idx = i;
	}

	if (idx)
		strncpy(buf, &(src[idx + 1]), len - idx);
	else
		strncpy(buf, src, len);
}

static void handle_bus_string(char *buf, char *src)
{
	int idx = 0, len, max = 2, cnt = 0, i;

	len = strlen(src);
	if (BBP_STR_LEN < len)
		len = BBP_STR_LEN;

	for (i = 0; i < len; i++) {
		if (src[i] == '/') {
			idx = i;
			cnt++;

			if (cnt == max)
				goto out;
		}
	}

out:
	if (idx)
		strncpy(buf, src, idx);
	else
		strncpy(buf, src, len);
}

enum pnc_str {
	PNC_STR_IGNORE = 0,
	PNC_STR_UNRECV = 1,
	PNC_STR_REST = 2,
};

static int handle_panic_string(char *src)
{
	if (!src)
		return PNC_STR_IGNORE;

	if (!strncmp(src, "Fatal", 5))
		return PNC_STR_IGNORE;
	else if (!strncmp(src, "ITMON", 5))
		return PNC_STR_IGNORE;
	else if (!strncmp(src, "Unrecoverable", 13))
		return PNC_STR_UNRECV;

	return PNC_STR_REST;
}

/*
 * proc/extra
 * RSTCNT (32) + PC (256) + LR (256) (MAX: 544)
 * + BUG (256) + BUS (256) => get only 64 (BBP_STR_LEN) (MAX: 672)
 * + PANIC (256) => get only 64 (MAX: 736)
 * + SMU (256) => get only 64 (MAX: 800)
 * total max : 800
 */
static int sec_debug_reset_reason_extra_show(struct seq_file *m, void *v)
{
	ssize_t ret = 0;
	char *rstcnt, *pc, *lr;
	char *bug, *bus, *pnc, *smu;
	char buf_bug[BBP_STR_LEN] = {0, };
	char buf_bus[BBP_STR_LEN] = {0, };

	if (extra_buf.already)
		goto out;

	memset(&extra_buf, 0, sizeof(extra_buf));

	rstcnt = get_bk_item_val("RSTCNT");
	pc = get_bk_item_val("PC");
	lr = get_bk_item_val("LR");

	bug = get_bk_item_val("BUG");
	bus = get_bk_item_val("BUS");
	pnc = get_bk_item_val("PANIC");
	smu = get_bk_item_val("SMU");

	write_buf(&extra_buf, 0, "RCNT:");
	if (rstcnt && strnlen(rstcnt, MAX_ITEM_VAL_LEN))
		write_buf(&extra_buf, 0, " %s /", rstcnt);
	else
		write_buf(&extra_buf, 0, " - /");

	write_buf(&extra_buf, 0, " PC:");
	if (pc && strnlen(pc, MAX_ITEM_VAL_LEN))
		write_buf(&extra_buf, 0, " %s", pc);
	else
		write_buf(&extra_buf, 0, " -");

	write_buf(&extra_buf, 0, " LR:");
	if (lr && strnlen(lr, MAX_ITEM_VAL_LEN))
		write_buf(&extra_buf, 0, " %s", lr);
	else
		write_buf(&extra_buf, 0, " -");

	/* BUG */
	if (bug && strnlen(bug, MAX_ITEM_VAL_LEN)) {
		handle_bug_string(buf_bug, bug);

		write_buf(&extra_buf, 0, " BUG: %s", buf_bug);
	}

	/* BUS */
	if (bus && strnlen(bus, MAX_ITEM_VAL_LEN)) {
		handle_bus_string(buf_bus, bus);

		write_buf(&extra_buf, 0, " BUS: %s", buf_bus);
	}

	/* PANIC */
	ret = handle_panic_string(pnc);
	if (ret == PNC_STR_UNRECV) {
		if (smu && strnlen(smu, MAX_ITEM_VAL_LEN))
			write_buf(&extra_buf, BBP_STR_LEN, " SMU: %s", smu);
		else
			write_buf(&extra_buf, BBP_STR_LEN, " PANIC: %s", pnc);
	} else if (ret == PNC_STR_REST) {
		if (strnlen(pnc, MAX_ITEM_VAL_LEN))
			write_buf(&extra_buf, BBP_STR_LEN, " PANIC: %s", pnc);
	}

	extra_buf.already = 1;

out:
	seq_printf(m, extra_buf.buf);

	return 0;
}

static int sec_debug_reset_reason_extra_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_debug_reset_reason_extra_show, NULL);
}

static const struct file_operations sec_debug_reset_reason_extra_proc_fops = {
	.open = sec_debug_reset_reason_extra_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init sec_debug_reset_reason_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("reset_reason", 0222, NULL, &sec_debug_reset_reason_proc_fops);
	if (!entry)
		return -ENOMEM;

	entry = proc_create("store_lastkmsg", 0222, NULL, &sec_debug_reset_reason_store_lastkmsg_proc_fops);
	if (!entry)
		return -ENOMEM;

	entry = proc_create("extra", 0222, NULL, &sec_debug_reset_reason_extra_proc_fops);
	if (!entry)
		return -ENOMEM;

	entry = proc_create("pwrsrc", 0222, NULL, &sec_debug_reset_reason_pwrsrc_proc_fops);
	if (!entry)
		return -ENOMEM;

//	dev = sec_device_create(NULL, "sec_reset_reason");
//	ret = sysfs_create_group(&dev->kobj, &sec_reset_reason_attr_group);
//	if (ret)
//		pr_err("%s : could not create sysfs noden", __func__);

	return 0;
}
device_initcall(sec_debug_reset_reason_init);
