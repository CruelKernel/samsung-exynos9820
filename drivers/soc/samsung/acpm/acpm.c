/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/firmware.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/debug-snapshot.h>
#include <linux/soc/samsung/exynos-soc.h>
#include <linux/sched/clock.h>

#include "acpm.h"
#include "acpm_ipc.h"
#include "../cal-if/fvmap.h"
#include "fw_header/framework.h"

static int ipc_done;
static unsigned long long ipc_time_start;
static unsigned long long ipc_time_end;

static struct acpm_info *exynos_acpm;

static int acpm_send_data(struct device_node *node, unsigned int check_id,
		struct ipc_config *config);

static int handle_dynamic_plugin(struct device_node *node, unsigned int id, unsigned int attach)
{
	struct ipc_config config;
	unsigned int cmd[4] = {0, };
	int ret = 0;

	config.cmd = cmd;
	if (attach)
		config.cmd[0] = (1 << ACPM_IPC_PROTOCOL_DP_A);
	else
		config.cmd[0] = (1 << ACPM_IPC_PROTOCOL_DP_D);

	config.cmd[0] |= id << ACPM_IPC_PROTOCOL_ID;

	config.response = true;
	config.indirection = false;

	ret = acpm_send_data(node, id, &config);

	config.cmd = NULL;

	if (!ret) {
		if (attach)
			pr_info("[ACPM] plugin(id = %d) attach done!\n", id);
		else
			pr_info("[ACPM] plugin(id = %d) detach done!\n", id);
	}

	return ret;
}

static void firmware_load(void *base, const char *firmware, int size)
{
	memcpy(base, firmware, size);
}

static int firmware_update(struct device *dev, void *fw_base, const char *fw_name)
{
	const struct firmware *fw_entry = NULL;
	int err;

	dev_info(dev, "Loading %s firmware ... ", fw_name);
	err = request_firmware(&fw_entry, fw_name, dev);
	if (err || !fw_entry) {
		dev_err(dev, "firmware request FAIL \n");
		return err;
	}

	firmware_load(fw_base, fw_entry->data, fw_entry->size);

	dev_info(dev, "OK \n");

	release_firmware(fw_entry);

	return 0;
}

static int plugins_init(void)
{
	struct plugin *plugins;
	int i, len, ret = 0;
	unsigned int plugin_id;
	char name[50];
	const char *fw_name = NULL;
	void __iomem *fw_base_addr;
	struct device_node *node, *child;
	const __be32 *prop;
	unsigned int offset;

	plugins = (struct plugin *)(acpm_srambase + acpm_initdata->plugins);

	for (i = 0; i < acpm_initdata->num_plugins; i++) {
		if ((plugins[i].is_attached == 0 && plugins[i].stay_attached == 1) ||
			(plugins[i].is_attached == 1 && plugins[i].stay_attached == 0)) {

			if (plugins[i].is_attached == 0 && plugins[i].stay_attached == 1) {
				fw_name = (const char *)(acpm_srambase + plugins[i].fw_name);
				if (!plugins[i].fw_name) {
					dev_err(exynos_acpm->dev, "fw_name missing on plugins %d.\n", plugins[i].id);
					return -ENOENT;
				}

				fw_base_addr = acpm_srambase + (plugins[i].base_addr & ~0x1);
				if (!plugins[i].base_addr) {
					dev_err(exynos_acpm->dev, "base_addr missing on plugins %d.\n", plugins[i].id);
					return -ENOENT;
				}

				/* Override fw_name from acpm firmware if it is described on DT. */
				node = of_get_child_by_name(exynos_acpm->dev->of_node, "dynamic-plugins");
				for_each_child_of_node(node, child) {
					prop = of_get_property(child, "plugin-id", &len);
					if (!prop)
						continue;
					plugin_id = be32_to_cpup(prop);

					if (plugin_id != i)
						continue;

					if (of_property_read_string(child, "fw-name", &fw_name)) {
						dev_err(exynos_acpm->dev, "fw name of plugin %d is invalid in DT.\n", i);
						return -EINVAL;
					}
				}

				strncpy(name, fw_name, 50);
				name[49]= 0;

				firmware_update(exynos_acpm->dev, fw_base_addr, name);
			}

			ret = handle_dynamic_plugin(exynos_acpm->dev->of_node, plugins[i].id, plugins[i].stay_attached);
			if (ret < 0)
				dev_err(exynos_acpm->dev, "plugin attach/detach:%u fail! plugin_id:%d, ret:%d",
						plugins[i].stay_attached, plugins[i].id, ret);

			if (fw_name && strstr(fw_name, "dvfs"))
				fvmap_init(fw_base_addr + plugins[i].size);

		} else if (plugins[i].is_attached == 1 && plugins[i].stay_attached == 1) {
			fw_name = (const char *)(acpm_srambase + plugins[i].fw_name);

			if (plugins[i].fw_name && fw_name &&
					(strstr(fw_name, "DVFS") || strstr(fw_name, "dvfs"))) {

				fw_base_addr = acpm_srambase + (plugins[i].base_addr & ~0x1);
				prop = of_get_property(exynos_acpm->dev->of_node, "fvmap_offset", &len);
				if (prop) {
					offset = be32_to_cpup(prop);
					fvmap_init(fw_base_addr + offset);
				}
			}
		}
	}

	return ret;
}

static int debug_log_level_get(void *data, unsigned long long *val)
{

	return 0;
}

static int debug_log_level_set(void *data, unsigned long long val)
{
	acpm_fw_log_level((unsigned int)val);

	return 0;
}

static int debug_ipc_loopback_test_get(void *data, unsigned long long *val)
{
	struct ipc_config config;
	int ret = 0;
	unsigned int cmd[4] = {0, };

	config.cmd = cmd;
	config.cmd[0] = (1 << ACPM_IPC_PROTOCOL_TEST);
	config.cmd[0] |= 0x3 << ACPM_IPC_PROTOCOL_ID;

	config.response = true;
	config.indirection = false;

	ipc_time_start = sched_clock();
	ret = acpm_send_data(exynos_acpm->dev->of_node, 3, &config);
	ipc_time_end = sched_clock();

	if (!ret)
		*val = ipc_time_end - ipc_time_start;
	else
		*val = 0;

	config.cmd = NULL;

	return 0;
}

static int debug_ipc_loopback_test_set(void *data, unsigned long long val)
{

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_log_level_fops,
		debug_log_level_get, debug_log_level_set, "%llu\n");
DEFINE_SIMPLE_ATTRIBUTE(debug_ipc_loopback_test_fops,
		debug_ipc_loopback_test_get, debug_ipc_loopback_test_set, "%llu\n");

static void acpm_debugfs_init(struct acpm_info *acpm)
{
	struct dentry *den;

	den = debugfs_create_dir("acpm_framework", NULL);
	debugfs_create_file("ipc_loopback_test", 0644, den, acpm, &debug_ipc_loopback_test_fops);
	debugfs_create_file("log_level", 0644, den, NULL, &debug_log_level_fops);
}

void *memcpy_align_4(void *dest, const void *src, unsigned int n)
{
	unsigned int *dp = dest;
	const unsigned int *sp = src;
	int i;

	if ((n % 4))
		BUG();

	n = n >> 2;

	for (i = 0; i < n; i++)
		*dp++ = *sp++;

	return dest;
}

void acpm_enter_wfi(void)
{
	struct ipc_config config;
	int ret = 0;
	unsigned int cmd[4] = {0, };

	if (exynos_acpm->enter_wfi)
		return;

	config.cmd = cmd;
	config.response = true;
	config.indirection = false;
	config.cmd[0] = 1 << ACPM_IPC_PROTOCOL_STOP;

	ret = acpm_send_data(exynos_acpm->dev->of_node, ACPM_IPC_PROTOCOL_STOP, &config);

	config.cmd = NULL;

	if (ret) {
		pr_err("[ACPM] acpm enter wfi fail!!\n");
	} else {
		pr_err("[ACPM] wfi done\n");
		exynos_acpm->enter_wfi++;
	}
}

u32 exynos_get_peri_timer_icvra(void)
{
       return (EXYNOS_PERI_TIMER_MAX - __raw_readl(exynos_acpm->timer_base + EXYNOS_TIMER_APM_TCVR)) & EXYNOS_PERI_TIMER_MAX;
}

void exynos_acpm_timer_clear(void)
{
	writel(exynos_acpm->timer_cnt, exynos_acpm->timer_base + EXYNOS_TIMER_APM_TCVR);
}

void exynos_acpm_reboot(void)
{
	u32 soc_id, revision;

	acpm_ipc_set_waiting_mode(BUSY_WAIT);

	soc_id = exynos_soc_info.product_id;
	revision = exynos_soc_info.revision;

	if (!(soc_id == EXYNOS9810_SOC_ID && revision < EXYNOS_MAIN_REV_1))
		acpm_enter_wfi();
}

static int acpm_send_data(struct device_node *node, unsigned int check_id,
		struct ipc_config *config)
{
	unsigned int channel_num, size;
	int ret = 0;
	int timeout_flag;
	unsigned int id = 0;

	if (!acpm_ipc_request_channel(node, NULL,
				&channel_num, &size)) {
		ipc_done = -1;

		ipc_time_start = sched_clock();
		ret = acpm_ipc_send_data(channel_num, config);

		if (config->cmd[0] & ACPM_IPC_PROTOCOL_DP_CMD) {
			id = config->cmd[0] & ACPM_IPC_PROTOCOL_IDX;
			id = id	>> ACPM_IPC_PROTOCOL_ID;
			ipc_done = id;
		} else if (config->cmd[0] & (1 << ACPM_IPC_PROTOCOL_STOP)) {
			ipc_done = ACPM_IPC_PROTOCOL_STOP;
		} else if (config->cmd[0] & (1 << ACPM_IPC_PROTOCOL_TEST)) {
			id = config->cmd[0] & ACPM_IPC_PROTOCOL_IDX;
			id = id	>> ACPM_IPC_PROTOCOL_ID;
			ipc_done = id;
		} else {
			id = config->cmd[0] & ACPM_IPC_PROTOCOL_IDX;
			id = id	>> ACPM_IPC_PROTOCOL_ID;
			ipc_done = id;
		}

		/* Response interrupt waiting */
		UNTIL_EQUAL(ipc_done, check_id, timeout_flag);

		if (timeout_flag)
			ret = -ETIMEDOUT;

		acpm_ipc_release_channel(node, channel_num);
	} else {
		pr_err("%s ipc request_channel fail, id:%u, size:%u\n",
				__func__, channel_num, size);
		ret = -EBUSY;
	}

	return ret;
}

static int acpm_probe(struct platform_device *pdev)
{
	struct acpm_info *acpm;
	struct device_node *node = pdev->dev.of_node;
	struct resource *res;
	int ret = 0;

	dev_info(&pdev->dev, "acpm probe\n");

	if (!node) {
		dev_err(&pdev->dev, "driver doesnt support"
				"non-dt devices\n");
		return -ENODEV;
	}

	acpm = devm_kzalloc(&pdev->dev,
			sizeof(struct acpm_info), GFP_KERNEL);
	if (IS_ERR(acpm))
		return PTR_ERR(acpm);
	if (!acpm)
		return -ENOMEM;

	acpm->dev = &pdev->dev;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "timer_apm");
	acpm->timer_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(acpm->timer_base))
		dev_info(&pdev->dev, "could not find timer base addr \n");

		if (of_property_read_u32(node, "peritimer-cnt", &acpm->timer_cnt))
		pr_warn("No matching property: peritiemr_cnt\n");

	exynos_acpm = acpm;

	acpm_debugfs_init(acpm);

	exynos_acpm_timer_clear();
	return ret;
}

static int acpm_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id acpm_match[] = {
	{ .compatible = "samsung,exynos-acpm" },
	{},
};

static struct platform_driver samsung_acpm_driver = {
	.probe	= acpm_probe,
	.remove	= acpm_remove,
	.driver	= {
		.name = "exynos-acpm",
		.owner	= THIS_MODULE,
		.of_match_table	= acpm_match,
	},
};

static int __init exynos_acpm_init(void)
{
	return platform_driver_register(&samsung_acpm_driver);
}
arch_initcall_sync(exynos_acpm_init);

static int __init exynos_acpm_binary_update(void)
{
	int ret;

	acpm_ipc_set_waiting_mode(BUSY_WAIT);

	ret = plugins_init();

	return ret;
}
fs_initcall_sync(exynos_acpm_binary_update);
