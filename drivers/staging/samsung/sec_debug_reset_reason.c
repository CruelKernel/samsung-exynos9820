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
#include <linux/sec_sysfs.h>
#include <linux/soc/samsung/exynos-soc.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/stacktrace.h>

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

static int sec_debug_reset_reason_pwrsrc_show(struct seq_file *m, void *v)
{
	ssize_t size = 0;
	int i;
	char buf[SZ_1K], val[32] = {0, };
	unsigned long tmp;

	memset(val, 0, 32);
	get_bk_item_val_as_string("PWROFF", val);
	tmp = simple_strtol(val, NULL, 0);
	size += sprintf((char *)(buf + size), "OFFSRC:");
	if (!tmp)
		size += sprintf((char *)(buf + size), " -");
	else
		for (i = 0; i < 8; i++)
			if (tmp & (1 << i))
				size += sprintf((char *)(buf + size), " %s", regs_bit[0][i]);

	size += sprintf((char *)(buf + size), " /");

	memset(val, 0, 32);
	get_bk_item_val_as_string("PWR", val);
	tmp = simple_strtol(val, NULL, 0);
	size += sprintf((char *)(buf + size), " ONSRC:");
	if (!tmp)
		size += sprintf((char *)(buf + size), " -");
	else
		for (i = 0; i < 8; i++)
			if (tmp & (1 << i))
				size += sprintf((char *)(buf + size), " %s", regs_bit[1][i]);

	size += sprintf((char *)(buf + size), " /");

	memset(val, 0, 32);
	get_bk_item_val_as_string("RST", val);
	tmp = simple_strtol(val, NULL, 0);
	size += sprintf((char *)(buf + size), " RSTSTAT:");
	if (!tmp)
		size += sprintf((char *)(buf + size), " -");
	else
		for (i = 0; i < 32; i++)
			if (tmp & (1 << i))
				size += sprintf((char *)(buf + size), " %s", dword_regs_bit[0][i]);

	seq_printf(m, buf);

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

static int sec_debug_reset_reason_extra_show(struct seq_file *m, void *v)
{
	ssize_t size = 0;
	char buf[SZ_1K];
	char *pc;
	char *lr;
	char *rstcnt;

	pc = get_bk_item_val("PC");
	lr = get_bk_item_val("LR");
	rstcnt = get_bk_item_val("RSTCNT");

	size += sprintf((char *)(buf + size), "RSTCNT:");
	if (rstcnt && strnlen(rstcnt, MAX_ITEM_VAL_LEN)) {
		size += sprintf((char *)(buf + size), " %s", rstcnt);
		size += sprintf((char *)(buf + size), " /");
	}
	else {
		size += sprintf((char *)(buf + size), " - /");
	}

	size += sprintf((char *)(buf + size), " PC:");
	if (pc && strnlen(pc, MAX_ITEM_VAL_LEN))
		size += sprintf((char *)(buf + size), " %s", pc);
	else
		size += sprintf((char *)(buf + size), " -");

	size += sprintf((char *)(buf + size), " LR:");
	if (lr && strnlen(lr, MAX_ITEM_VAL_LEN))
		size += sprintf((char *)(buf + size), " %s", lr);
	else
		size += sprintf((char *)(buf + size), " -");

	seq_printf(m, buf);

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
