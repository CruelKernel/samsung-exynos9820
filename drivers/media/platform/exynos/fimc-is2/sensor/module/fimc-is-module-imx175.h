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

#ifndef FIMC_IS_DEVICE_IMX175_H
#define FIMC_IS_DEVICE_IMX175_H

#define SENSOR_IMX175_INSTANCE	0
#define SENSOR_IMX175_NAME	SENSOR_NAME_IMX175

int sensor_imx175_probe(struct i2c_client *client,
	const struct i2c_device_id *id);

#endif
