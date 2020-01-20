/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_FPGA_DPHY_H
#define FIMC_IS_FPGA_DPHY_H

#include <linux/i2c.h>

int fpga_dphy_probe(struct i2c_client *client,
	const struct i2c_device_id *id);
int fpga_dphy_init(struct v4l2_subdev *subdev, u32 val);

#endif

