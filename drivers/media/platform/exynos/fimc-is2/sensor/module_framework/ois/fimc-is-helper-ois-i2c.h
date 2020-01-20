/*
 * Samsung Exynos5 SoC series ois i2c driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef fimc_is_device_ois_i2c_H
#define fimc_is_device_ois_i2c_H

#include <linux/i2c.h>

int fimc_is_ois_write(struct i2c_client *client,
		u16 addr, u8 data);
int fimc_is_ois_read(struct i2c_client *client,
		u16 addr, u8 *data);

int fimc_is_ois_write_multi(struct i2c_client *client, u16 addr,
		u8 *data, int size);
int fimc_is_ois_read_multi(struct i2c_client *client, u16 addr, u8 *data, size_t size);
#endif

