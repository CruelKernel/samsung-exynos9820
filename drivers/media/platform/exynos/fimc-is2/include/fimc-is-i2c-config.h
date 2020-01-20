/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_I2C_CONFIG_H
#define FIMC_IS_I2C_CONFIG_H

#include <linux/i2c.h>

/* I2C PIN MODE */
enum {
	I2C_PIN_STATE_OFF = 0,
	I2C_PIN_STATE_ON,
};

int fimc_is_i2c_pin_config(struct i2c_client *client, int state);
int fimc_is_i2c_pin_control(struct fimc_is_module_enum *module, u32 scenario, u32 value);
#endif
