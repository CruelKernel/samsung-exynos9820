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

#ifndef FIMC_IS_DEVICE_IMX219_H
#define FIMC_IS_DEVICE_IMX219_H

#define SENSOR_IMX219_INSTANCE	1
#define SENSOR_IMX219_NAME	SENSOR_NAME_IMX219

struct fimc_is_module_imx219 {
	u16		vis_duration;
	u16		frame_length_line;
	u32		line_length_pck;
	u32		system_clock;
};

int sensor_imx219_probe(struct i2c_client *client,
	const struct i2c_device_id *id);

#endif
