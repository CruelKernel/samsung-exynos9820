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
#include <linux/firmware.h>
#include <linux/string.h>
#include <linux/debug-snapshot.h>

#include <soc/samsung/exynos-adv-tracer-misc.h>
#include <soc/samsung/exynos-adv-tracer-ipc.h>

struct adv_tracer_misc_plugin_info {
	char name[8];
	unsigned int phy_base_addr;
	struct list_head list;
	struct adv_tracer_plugin ch_info;
};

struct plugin_misc_info {
	struct platform_device *pdev;
	struct list_head plugin_list;
} plugin_misc;

int adv_tracer_misc_plugin_load(const char *name, const char *release_plugin_name)
{
	struct adv_tracer_misc_plugin_info *node;
	struct adv_tracer_ipc_cmd cmd;
	int ret;

	list_for_each_entry(node, &plugin_misc.plugin_list, list) {
		if (!strncmp(name, node->name, strlen(name))) {
			cmd.cmd_raw.cmd = EAT_IPC_CMD_FRM_LOAD_BINARY;
			cmd.buffer[1] = node->phy_base_addr;
			cmd.buffer[2] = *(unsigned int *)node->name;
			if (release_plugin_name)
				cmd.buffer[3] = *(unsigned int *)release_plugin_name;

			ret = adv_tracer_ipc_send_data_polling(EAT_FRM_CHANNEL, &cmd);
			if (ret)
				return -1;

			if (release_plugin_name)
				adv_tracer_ipc_release_channel_by_name(release_plugin_name);

			return 0;
		}
	}

	return -1;
}

int adv_tracer_misc_ipc_request_channel(const char *name, void *handler)
{
	struct adv_tracer_misc_plugin_info *plugin_node;
	struct device_node *np;
	int ret;

	list_for_each_entry(plugin_node, &plugin_misc.plugin_list, list) {
		if (!strncmp(name, plugin_node->name, strlen(name))) {
			np = plugin_node->ch_info.np;
			ret = adv_tracer_ipc_request_channel(np, (ipc_callback)handler,
					&plugin_node->ch_info.id, &plugin_node->ch_info.len);
			if (ret < 0)
				return -1;
			return 0;
		}
	}

	return -1;
}

static int adv_tracer_misc_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *plugin_info_np;
	struct device_node *plugin_np;
	const char *plugin_str;
	int ret, i, count;
	struct adv_tracer_misc_plugin_info *plugin;
	const struct firmware *fw;
	void *firmware_vaddr;
	unsigned int firmware_paddr, total_size;

	dev_info(&pdev->dev, "[EAT+] %s\n", __func__);
	INIT_LIST_HEAD(&plugin_misc.plugin_list);

	/* check DSS region */
	plugin_str = of_get_property(np, "save-fw-dss-region-name", NULL);
	if (!plugin_str) {
		dev_err(&pdev->dev, "failed to get dss region name\n");
		ret = -ENODEV;
		goto err_misc_info;
	}
	firmware_vaddr = (void *)dbg_snapshot_get_item_vaddr((char *)plugin_str);
	firmware_paddr = dbg_snapshot_get_item_paddr((char *)plugin_str);
	total_size = 0;
	if (!firmware_vaddr || !firmware_paddr) {
		dev_err(&pdev->dev, "failed to get dss(%s) addr\n", plugin_str);
		ret = -ENODEV;
		goto err_misc_info;
	}

	/* find plugin-info node */
	plugin_info_np = of_get_child_by_name(np, "plugin-info");
	if (!plugin_info_np) {
		dev_err(&pdev->dev, "failed to get plugin_info node\n");
		ret = -ENODEV;
		goto err_misc_info;
	}
	plugin_misc.pdev = pdev;

	/* get plugin list count */
	ret = of_property_count_strings(plugin_info_np, "plugin-list");
	if (ret < 0) {
		dev_err(&pdev->dev, "faild to get plugin-list\n");
		ret = -ENODEV;
		goto err_misc_info;
	}
	count = ret;

	/* load plugin binary to DRAM */
	for (i = 0; i < count; i++) {
		ret = of_property_read_string_index(plugin_info_np, "plugin-list",
						i, (const char **)&plugin_str);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to get plugin_str\n");
			continue;
		}

		plugin_np = of_get_child_by_name(plugin_info_np, plugin_str);
		if (!plugin_np) {
			dev_err(&pdev->dev, "failed to get %s node, count:%d\n",
								plugin_str, count);
			continue;
		}

		plugin = kzalloc(sizeof(struct adv_tracer_misc_plugin_info), GFP_KERNEL);
		if (!plugin) {
			dev_err(&pdev->dev, "failed to get memory region of adv_tracer_misc_plugin_info\n");
			of_node_put(plugin_np);
			continue;
		}

		/* get plugin name */
		plugin_str = of_get_property(plugin_np, "plugin-name", NULL);
		if (!plugin_str) {
			dev_err(&pdev->dev, "failed to get plugin-name\n");
			of_node_put(plugin_np);
			kfree(plugin);
			continue;
		}
		strncpy(plugin->name, plugin_str, sizeof(unsigned int));

		/* get firmware binary location */
		plugin_str = (char *)of_get_property(plugin_np, "plugin-fw-name", NULL);
		if (!plugin_str) {
			dev_err(&pdev->dev, "failed to get plugin-fw-name\n");
			of_node_put(plugin_np);
			kfree(plugin);
			continue;
		}

		/* copy binary to DRAM */
		ret = request_firmware(&fw, plugin_str, &pdev->dev);
		if (ret) {
			dev_err(&pdev->dev, "fail to get %s firmware binary\n", plugin_str);
			of_node_put(plugin_np);
			kfree(plugin);
			continue;
		}
		memcpy(firmware_vaddr + total_size, fw->data, fw->size);
		plugin->phy_base_addr = firmware_paddr + total_size;
		total_size += (unsigned int)fw->size;
		plugin->ch_info.np = (void *)plugin_np;
		release_firmware(fw);

		/* add plugin to list */
		list_add(&plugin->list, &plugin_misc.plugin_list);
		dev_info(&pdev->dev, "success to copy binary %s\n", plugin->name);
	}

	dev_info(&pdev->dev, "[EAT-] %s\n", __func__);
	return 0;


err_misc_info:
	return ret;
}

static int adv_tracer_misc_remove(struct platform_device *pdev)
{
	struct adv_tracer_misc_plugin_info *node, *next_node;

	list_for_each_entry_safe(node, next_node, &plugin_misc.plugin_list, list) {
		list_del(&node->list);
		kfree(node);
	}

	return 0;
}

static const struct of_device_id adv_tracer_misc_match[] = {
	{
		.compatible = "samsung,exynos-adv-tracer-misc",
	},
	{},
};

static struct platform_driver adv_tracer_misc_drv = {
	.probe		= adv_tracer_misc_probe,
	.remove		= adv_tracer_misc_remove,
	.driver		= {
		.name	= "exynos-adv-tracer-misc",
		.owner	= THIS_MODULE,
		.of_match_table = adv_tracer_misc_match,
	},
};

static int __init adv_tracer_misc_init(void)
{
	return platform_driver_register(&adv_tracer_misc_drv);
}
late_initcall_sync(adv_tracer_misc_init);
