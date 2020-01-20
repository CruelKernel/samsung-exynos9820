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

#ifndef FIMC_IS_DEVICE_4E6_H
#define FIMC_IS_DEVICE_4E6_H

#define SENSOR_S5K4E6_INSTANCE	1
#define SENSOR_S5K4E6_NAME	SENSOR_NAME_S5K4E6

struct fimc_is_module_4e6 {
	u16		vis_duration;
	u16		frame_length_line;
	u32		line_length_pck;
	u32		system_clock;
};
int sensor_4e6_vision_probe(struct i2c_client *client,
	const struct i2c_device_id *id);
int sensor_4e6_probe(struct platform_device *pdev);
#endif
