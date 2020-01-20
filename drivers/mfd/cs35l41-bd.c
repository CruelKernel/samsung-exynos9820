/*
 * Big-data logging support for Cirrus Logic CS35L41 codec
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
#include <linux/mfd/cs35l41/big_data.h>
#include <linux/mfd/cs35l41/wmfw.h>

static int cs35l41_bd_probe(struct platform_device *pdev)
{
	struct cs35l41_data *cs35l41 = dev_get_drvdata(pdev->dev.parent);
	unsigned int temp;
	bool right_channel_amp;
	int ret;

	regmap_read(cs35l41->regmap, 0x00000000, &temp);
	dev_info(&pdev->dev,
		"Prince Big Data Driver probe, Dev ID = %x\n", temp);

	right_channel_amp = of_property_read_bool(cs35l41->dev->of_node,
					"cirrus,right-channel-amp");

	ret = cirrus_bd_amp_add(cs35l41->regmap, right_channel_amp);

	if (ret < 0) {
		dev_info(&pdev->dev,
			"cirrus_bd device not ready, probe deferred\n");
		return -EPROBE_DEFER;
	}

	return 0;
}

static int cs35l41_bd_remove(struct platform_device *pdev)
{
	return 0;
}

struct platform_driver cs35l41_bd_driver = {
	.driver = {
		.name = "cs35l41-bd",
		.owner = THIS_MODULE,
	},
	.probe = cs35l41_bd_probe,
	.remove = cs35l41_bd_remove,
};
module_platform_driver(cs35l41_bd_driver);
