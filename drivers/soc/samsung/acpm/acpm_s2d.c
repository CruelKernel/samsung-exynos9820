/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <soc/samsung/acpm_ipc_ctrl.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/sched/clock.h>
#include <linux/debug-snapshot.h>
#include <soc/samsung/exynos-pmu.h>

#define RESET_SEQUENCER_CONFIGURATION		(0x0500)

#define EXYNOS_S2D_DBG_PREFIX	"EXYNOS-S2D-DBG: "
#define BUF_SIZE		32

#ifdef CONFIG_EXYNOS_REBOOT
extern void dfd_set_dump_gpr(int en);
extern void little_reset_control(int en);
extern void big_reset_control(int en);
#else
#define dfd_set_dump_gpr(a)            do { } while(0)
#define little_reset_control(a)                do { } while(0)
#define big_reset_control(a)           do { } while(0)
#endif

#define DEFAULT_S2D_BLOCK		(0xFFFFF3BF)

struct scan2dram_info {
	struct device_node *np;
	unsigned int ch;
	unsigned int size;

	struct dentry *den;
	struct file_operations fops;
};

static int s2d_en = 0;
static u32 s2d_sel_block = DEFAULT_S2D_BLOCK;
static struct scan2dram_info *acpm_s2d;
static struct dentry *s2d_dbg_root;

void exynos_acpm_set_s2d_enable(int en)
{
	if (en) {
		dfd_set_dump_gpr(0);
		little_reset_control(0);
		big_reset_control(0);
		exynos_pmu_write(RESET_SEQUENCER_CONFIGURATION, 0x0);
	}

	s2d_en = en;
}

void exynos_acpm_select_s2d_blk(u32 sel)
{
	s2d_sel_block = sel;
}

int exynos_acpm_s2d_update_en(void)
{
	struct ipc_config config;
	unsigned int cmd[4] = {0,};
	unsigned long long before, after, latency;
	int ret;

	config.cmd = cmd;
	config.response = true;
	config.indirection = false;
	config.cmd[1] = exynos_ss_get_item_paddr("log_s2d");
	if (s2d_en)
		config.cmd[2] = s2d_sel_block;
	else
		config.cmd[2] = 0;

	before = sched_clock();
	ret = acpm_ipc_send_data(acpm_s2d->ch, &config);
	after = sched_clock();
	latency = after - before;
	if (ret)
		pr_err("%s: latency = %llu ret = %d",
				__func__, latency, ret);

	return ret;
}

static ssize_t exynos_s2d_en_dbg_read(struct file *file, char __user *user_buf,
		size_t count, loff_t *ppos)
{
	ssize_t ret;
	char buf[BUF_SIZE] = {0,};

	ret = snprintf(buf, BUF_SIZE, "%s : %d\n", "Scan2dram enable", s2d_en);
	if (ret < 0)
		return ret;

	return simple_read_from_buffer(user_buf, count, ppos, buf, ret);
}

static ssize_t exynos_s2d_en_dbg_write(struct file *file, const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	ssize_t ret;
	char buf[BUF_SIZE];

	ret = simple_write_to_buffer(buf, sizeof(buf) - 1, ppos, user_buf, count);
	if (ret < 0)
		return ret;

	if (buf[0] == '0') {
		exynos_acpm_set_s2d_enable(0);
		exynos_acpm_s2d_update_en();
	} else if (buf[0] == '1') {
		exynos_acpm_set_s2d_enable(1);
		exynos_acpm_s2d_update_en();
	}

	return ret;
}

static int exynos_acpm_s2d_probe(struct platform_device *pdev)
{
	int ret;

	acpm_s2d = kzalloc(sizeof(struct scan2dram_info), GFP_KERNEL);
	if (!acpm_s2d) {
		pr_err("%s %s: can not allocate mem for acpm_s2d\n",
				EXYNOS_S2D_DBG_PREFIX, __func__);
		ret = -ENOMEM;
		goto err_s2d_info;
	}

	acpm_s2d->np = of_find_node_by_name(NULL, "acpm_s2d");
	if (!acpm_s2d->np) {
		pr_err("%s %s: can not get node for acpm_s2d\n",
				EXYNOS_S2D_DBG_PREFIX, __func__);
		ret = -ENODEV;
		goto err_dbgfs_probe;
	}

	ret = acpm_ipc_request_channel(acpm_s2d->np, NULL,
			&acpm_s2d->ch, &acpm_s2d->size);
	if (ret < 0) {
		pr_err("%s %s: acpm_s2d ipc request fail ret = %d\n",
				EXYNOS_S2D_DBG_PREFIX, __func__, ret);
		ret = -ENODEV;
	}

	exynos_acpm_s2d_update_en();

	s2d_dbg_root = debugfs_create_dir("scan2dram", NULL);
	if (!s2d_dbg_root) {
		pr_err("%s %s: could not create debugfs root dir\n",
				EXYNOS_S2D_DBG_PREFIX, __func__);
		ret = -ENOMEM;
		goto err_dbgfs_probe;
	}

	acpm_s2d->fops.open = simple_open;
	acpm_s2d->fops.read = exynos_s2d_en_dbg_read;
	acpm_s2d->fops.write = exynos_s2d_en_dbg_write;
	acpm_s2d->fops.llseek = default_llseek;
	acpm_s2d->den = debugfs_create_file("enable", 0644, s2d_dbg_root,
			NULL, &acpm_s2d->fops);

	platform_set_drvdata(pdev, acpm_s2d);

	return 0;

err_dbgfs_probe:
	kfree(acpm_s2d);
err_s2d_info:
	return ret;
}

static int exynos_acpm_s2d_remove(struct platform_device *pdev)
{
	struct scan2dram_info *acpm_s2d = platform_get_drvdata(pdev);

	acpm_ipc_release_channel(acpm_s2d->np, acpm_s2d->ch);
	debugfs_remove_recursive(s2d_dbg_root);
	kfree(acpm_s2d);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id exynos_acpm_s2d_match[] = {
	{
		.compatible = "samsung,exynos-acpm-s2d",
	},
	{},
};

static struct platform_driver exynos_acpm_s2d_drv = {
	.probe          = exynos_acpm_s2d_probe,
	.remove         = exynos_acpm_s2d_remove,
	.driver         = {
		.name   = "exynos_acpm_s2d",
		.owner  = THIS_MODULE,
		.of_match_table = exynos_acpm_s2d_match,
	},
};

static int __init exynos_acpm_s2d_init(void)
{
	return platform_driver_register(&exynos_acpm_s2d_drv);
}
late_initcall(exynos_acpm_s2d_init);

static int __init s2d_enable(char *str)
{
	s2d_en = 1;
	return 1;
}
__setup("s2d_enable", s2d_enable);
