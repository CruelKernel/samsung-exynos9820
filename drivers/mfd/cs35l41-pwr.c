/*
 * Power Management support for Cirrus Logic CS35L41 codec
 *
 * Copyright 2017 Cirrus Logic
 *
 * Author:	David Rhodes	<david.rhodes@cirrus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/firmware.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/power_supply.h>
#include <linux/fs.h>
#include <linux/ktime.h>
#include <linux/of_device.h>

#include <linux/mfd/cs35l41/core.h>
#include <linux/mfd/cs35l41/registers.h>
#include <linux/mfd/cs35l41/power.h>

static int cs35l41_pwr_probe(struct platform_device *pdev)
{
	struct cs35l41_data *cs35l41 = dev_get_drvdata(pdev->dev.parent);
	struct device_node *pwr_params;
	unsigned int reg, target_temp = 0, exit_temp = 0;
	bool right_channel_amp, global_enable;
	int ret;

	regmap_read(cs35l41->regmap, 0x00000000, &reg);
	dev_info(&pdev->dev,
		"Prince Power Driver probe, Dev ID = %x\n", reg);

	right_channel_amp = of_property_read_bool(cs35l41->dev->of_node,
					"cirrus,right-channel-amp");

	ret = cirrus_pwr_amp_add(cs35l41->regmap, right_channel_amp);

	if (ret < 0) {
		dev_info(&pdev->dev,
			"cirrus_pwr device not ready, probe deferred\n");
		return -EPROBE_DEFER;
	}

	pwr_params = of_get_child_by_name(cs35l41->dev->of_node,
						"cirrus,pwr-params");
	if (pwr_params) {
		global_enable = of_property_read_bool(pwr_params,
						"cirrus,pwr-global-enable");
		ret = of_property_read_u32(pwr_params, "cirrus,pwr-target-temp",
					&reg);
		if (ret >= 0)
			target_temp = reg;

		ret = of_property_read_u32(pwr_params, "cirrus,pwr-exit-temp",
					&reg);
		if (ret >= 0)
			exit_temp = reg;

		cirrus_pwr_set_params(global_enable, right_channel_amp,
					target_temp, exit_temp);
	}
	of_node_put(pwr_params);

	return 0;
}

static int cs35l41_pwr_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver cs35l41_pwr_driver = {
	.driver = {
		.name = "cs35l41-pwr",
		.owner = THIS_MODULE,
	},
	.probe = cs35l41_pwr_probe,
	.remove = cs35l41_pwr_remove,
};
module_platform_driver(cs35l41_pwr_driver);
