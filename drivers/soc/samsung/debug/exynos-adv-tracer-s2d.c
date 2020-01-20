/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
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
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/debug-snapshot.h>

#include <soc/samsung/exynos-adv-tracer-ipc.h>

enum s2d_ipc_cmd {
	eS2D_IPC_CMD_SET_ALL_BLK = 1,
	eS2D_IPC_CMD_GET_ALL_BLK,
	eS2D_IPC_CMD_SET_ENABLE,
	eS2D_IPC_CMD_GET_ENABLE,
};

struct plugin_s2d_info {
	struct adv_tracer_plugin *s2d_dev;
	struct platform_device *pdev;
	unsigned int enable;
	unsigned int all_block;
} plugin_s2d;

int adv_tracer_s2d_get_enable(void)
{
	struct adv_tracer_ipc_cmd cmd;
	int ret = 0;

	cmd.cmd_raw.cmd = eS2D_IPC_CMD_GET_ENABLE;
	ret = adv_tracer_ipc_send_data(plugin_s2d.s2d_dev->id, (struct adv_tracer_ipc_cmd *)&cmd);
	if (ret < 0) {
		dev_err(&plugin_s2d.pdev->dev, "s2d ipc cannot get enable\n");
		return ret;
	}
	plugin_s2d.enable = cmd.buffer[1];

	return 0;
}

int adv_tracer_s2d_set_enable(int en)
{
	struct adv_tracer_ipc_cmd cmd;
	int ret = 0;

	cmd.cmd_raw.cmd = eS2D_IPC_CMD_SET_ENABLE;
	cmd.buffer[1] = en;
	ret = adv_tracer_ipc_send_data(plugin_s2d.s2d_dev->id, &cmd);
	if (ret < 0) {
		dev_err(&plugin_s2d.pdev->dev, "s2d ipc cannot enable setting\n");
		return ret;
	}
	plugin_s2d.enable = en;
	return 0;
}

int adv_tracer_s2d_get_blk_info(void)
{
	volatile struct adv_tracer_ipc_cmd cmd;
	int ret = 0;

	cmd.cmd_raw.cmd = eS2D_IPC_CMD_GET_ALL_BLK;
	ret = adv_tracer_ipc_send_data(plugin_s2d.s2d_dev->id, (struct adv_tracer_ipc_cmd *)&cmd);
	if (ret < 0) {
		dev_err(&plugin_s2d.pdev->dev, "s2d ipc cannot select blk\n");
		return ret;
	}
	plugin_s2d.all_block = cmd.buffer[1];

	return 0;
}


int adv_tracer_s2d_set_blk_info(unsigned int all)
{
	struct adv_tracer_ipc_cmd cmd;
	int ret = 0;

	cmd.cmd_raw.cmd = eS2D_IPC_CMD_SET_ALL_BLK;
	cmd.buffer[1] = all;
	ret = adv_tracer_ipc_send_data(plugin_s2d.s2d_dev->id, &cmd);
	if (ret < 0) {
		dev_err(&plugin_s2d.pdev->dev, "s2d ipc cannot select blk\n");
		return ret;
	}
	plugin_s2d.all_block = all;

	return 0;
}

static int adv_tracer_s2d_handler(struct adv_tracer_ipc_cmd *cmd, unsigned int len)
{
	return 0;
}

static ssize_t s2d_enable_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	unsigned long val = simple_strtoul(buf, NULL, 10);

	adv_tracer_s2d_set_enable(!!val);

	return size;
}

static ssize_t s2d_enable_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, 10, "%sable\n", plugin_s2d.enable ? "en" : "dis");
}

static ssize_t s2d_block_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	unsigned long val = simple_strtoul(buf, NULL, 16);

	adv_tracer_s2d_set_blk_info(!!val);

	return size;
}

static ssize_t s2d_block_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, 10, "%s\n", plugin_s2d.all_block ? "all" : "cpus");
}

static DEVICE_ATTR_RW(s2d_enable);
static DEVICE_ATTR_RW(s2d_block);

static struct attribute *adv_tracer_s2d_sysfs_attrs[] = {
	&dev_attr_s2d_enable.attr,
	&dev_attr_s2d_block.attr,
	NULL,
};
ATTRIBUTE_GROUPS(adv_tracer_s2d_sysfs);

static int adv_tracer_s2d_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct adv_tracer_plugin *s2d = NULL;
	int ret;

	dev_info(&pdev->dev, "[EAT+] %s\n", __func__);

	s2d = devm_kzalloc(&pdev->dev, sizeof(struct adv_tracer_plugin), GFP_KERNEL);
	if (!s2d) {
		dev_err(&pdev->dev, "can not allocate mem for s2d\n");
		ret = -ENOMEM;
		goto err_s2d_info;
	}

	plugin_s2d.pdev = pdev;
	plugin_s2d.s2d_dev = s2d;

	ret = adv_tracer_ipc_request_channel(node, (ipc_callback)adv_tracer_s2d_handler,
				&s2d->id, &s2d->len);
	if (ret < 0) {
		dev_err(&pdev->dev, "s2d ipc request fail(%d)\n",ret);
		ret = -ENODEV;
		goto err_sysfs_probe;
	}

	ret = adv_tracer_s2d_get_enable();
	if (ret < 0)
		goto err_sysfs_probe;
	dev_info(&pdev->dev, "S2D %sabled\n", plugin_s2d.enable ? "en" : "dis");

	ret = dbg_snapshot_get_debug_level_reg();
	if (plugin_s2d.enable) {
	       if (ret == DSS_DEBUG_LEVEL_LOW)
		       plugin_s2d.all_block = 0;
	       else
		       plugin_s2d.all_block = 1;

		adv_tracer_s2d_set_blk_info(plugin_s2d.all_block);
		dev_info(&pdev->dev, "S2D block: %s\n",
				plugin_s2d.all_block ? "All" : "CPU");
	}

	platform_set_drvdata(pdev, s2d);
	ret = sysfs_create_groups(&pdev->dev.kobj, adv_tracer_s2d_sysfs_groups);
	if (ret) {
		dev_err(&pdev->dev, "fail to register sysfs.\n");
		return ret;
	}

	dev_info(&pdev->dev, "[EAT-] %s\n", __func__);
	return 0;

err_sysfs_probe:
	kfree(s2d);
err_s2d_info:
	return ret;
}

static int adv_tracer_s2d_remove(struct platform_device *pdev)
{
	struct adv_tracer_plugin *s2d = platform_get_drvdata(pdev);

	adv_tracer_ipc_release_channel(s2d->id);
	kfree(s2d);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id adv_tracer_s2d_match[] = {
	{
		.compatible = "samsung,exynos-adv-tracer-s2d",
	},
	{},
};

static struct platform_driver adv_tracer_s2d_drv = {
	.probe          = adv_tracer_s2d_probe,
	.remove         = adv_tracer_s2d_remove,
	.driver         = {
		.name   = "exynos-adv-tracer-s2d",
		.owner  = THIS_MODULE,
		.of_match_table = adv_tracer_s2d_match,
	},
};

static int __init adv_tracer_s2d_init(void)
{
	return platform_driver_register(&adv_tracer_s2d_drv);
}
subsys_initcall(adv_tracer_s2d_init);
