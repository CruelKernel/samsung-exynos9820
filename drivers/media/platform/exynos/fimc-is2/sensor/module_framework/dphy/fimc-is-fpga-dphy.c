/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <mach/exynos-fimc-is2-sensor.h>

#include "fimc-is-hw.h"
#include "fimc-is-core.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-resourcemgr.h"
#include "fimc-is-dt.h"
#include "fimc-is-fpga-dphy.h"

#include "fimc-is-helper-i2c.h"

#define DPHY_NAME "FPGA_DPHY"

struct i2c_client *fpga_dphy_client;

int fpga_dphy_init(struct v4l2_subdev *subdev, u32 val)
{
	int ret = 0;

	printk("set dphy addr %#x\n", fpga_dphy_client->addr);
	ret = fimc_is_sensor_write8(fpga_dphy_client, 30, 0x1);
	if (ret) {
		err("fpga_dphy_client set fail, 30\n");
	}

	usleep_range(1000, 1000);
	ret = fimc_is_sensor_write8(fpga_dphy_client, 38, 0x18);
	if (ret) {
		err("fpga_dphy_client set fail, 38\n");
	}

	printk("%s\n", __func__);

	return ret;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init = fpga_dphy_init
};

static const struct v4l2_subdev_video_ops video_ops;

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
	.video = &video_ops
};

int fpga_dphy_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;

	fpga_dphy_client = client;

	probe_info("%s(%d)\n", __func__, ret);
	return ret;
}

static int fpga_dphy_remove(struct i2c_client *client)
{
	int ret = 0;
	return ret;
}

static const struct of_device_id exynos_fimc_is_fpga_dphy_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-fpga-dphy",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fimc_is_fpga_dphy_match);

static const struct i2c_device_id fpga_dphy_idt[] = {
	{ DPHY_NAME, 0 },
	{},
};

static struct i2c_driver fpga_dphy_driver = {
	.driver = {
		.name	= DPHY_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = exynos_fimc_is_fpga_dphy_match
	},
	.probe	= fpga_dphy_probe,
	.remove	= fpga_dphy_remove,
	.id_table = fpga_dphy_idt
};
module_i2c_driver(fpga_dphy_driver);
