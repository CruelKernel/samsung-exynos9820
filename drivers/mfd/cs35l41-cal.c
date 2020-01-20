/*
 * Calibration support for Cirrus Logic CS35L41 codec
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
#include <asm/io.h>
#include <linux/firmware.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/power_supply.h>
#include <linux/fs.h>
#include <linux/of_device.h>

#include <linux/mfd/cs35l41/core.h>
#include <linux/mfd/cs35l41/registers.h>
#include <linux/mfd/cs35l41/calibration.h>
#include <linux/mfd/cs35l41/wmfw.h>

static int cs35l41_cal_probe(struct platform_device *pdev)
{
	struct cs35l41_data *cs35l41 = dev_get_drvdata(pdev->dev.parent);
	unsigned int temp;
	bool right_channel_amp;
	const char *dsp_part_name;
	int ret;

	regmap_read(cs35l41->regmap, 0x00000000, &temp);
	dev_info(&pdev->dev, "Prince Calibration Driver probe, Dev ID = %x\n", temp);

	right_channel_amp = of_property_read_bool(cs35l41->dev->of_node,
					"cirrus,right-channel-amp");
	ret = of_property_read_string(cs35l41->dev->of_node,
						"cirrus,dsp-part-name",
						&dsp_part_name);
	if (ret < 0)
		dsp_part_name = "cs35l41";

	ret = cirrus_cal_amp_add(cs35l41->regmap, right_channel_amp,
							dsp_part_name);

	if (ret < 0) {
		dev_info(&pdev->dev,
			"cirrus_cal device not ready, probe deferred\n");
		return -EPROBE_DEFER;
	}

	return 0;
}

static int cs35l41_cal_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver cs35l41_cal_driver = {
	.driver = {
		.name = "cs35l41-cal",
		.owner = THIS_MODULE,
	},
	.probe = cs35l41_cal_probe,
	.remove = cs35l41_cal_remove,
};
module_platform_driver(cs35l41_cal_driver);
