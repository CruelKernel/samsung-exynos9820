/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef fimc_is_device_actuator_i2c_H
#define fimc_is_device_actuator_i2c_H

#include "fimc-is-helper-i2c.h"

int fimc_is_sensor_data_read16(struct i2c_client *client,
		u16 *val);
int fimc_is_sensor_data_write16(struct i2c_client *client,
		u8 val_high, u8 val_low);
int fimc_is_sensor_addr_data_write16(struct i2c_client *client,
	u8 addr, u8 val_high, u8 val_low);
#endif
