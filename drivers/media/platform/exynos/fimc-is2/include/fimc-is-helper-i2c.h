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

#ifndef fimc_is_helper_i2c_H
#define fimc_is_helper_i2c_H

#include <linux/i2c.h>

//#define FIMC_IS_VIRTUAL_MODULE

#ifdef FIMC_IS_VIRTUAL_MODULE
#define i2c_info(fmt, ...) \
	printk(KERN_DEBUG "[^^]"fmt, ##__VA_ARGS__)
#else
#define i2c_info(fmt, ...)
#endif

/* FIXME: make it dynamic parsing */
#define I2C_NEXT  3
#define I2C_BYTE  2
#define I2C_DATA  1
#define I2C_ADDR  0

/* setfile I2C write option */
#define I2C_MODE_BASE	0xF0000000
#define I2C_MODE_BURST_ADDR	(1 + I2C_MODE_BASE)
#define I2C_MODE_BURST_DATA	(2 + I2C_MODE_BASE)
#define I2C_MODE_DELAY	(3 + I2C_MODE_BASE)

int fimc_is_i2c_transfer(struct i2c_adapter *adapter, struct i2c_msg *msg, u32 size);
int fimc_is_sensor_addr8_read8(struct i2c_client *client,
	u8 addr, u8 *val);
int fimc_is_sensor_read8(struct i2c_client *client,
	u16 addr, u8 *val);
int fimc_is_sensor_read16(struct i2c_client *client,
	u16 addr, u16 *val);
int fimc_is_sensor_write(struct i2c_client *client,
	u8 *buf, u32 size);
int fimc_is_sensor_addr8_write8(struct i2c_client *client,
	u8 addr, u8 val);
int fimc_is_sensor_write8(struct i2c_client *client,
	u16 addr, u8 val);
int fimc_is_sensor_write8_array(struct i2c_client *client,
	u16 addr, u8 *val, u32 num);
int fimc_is_sensor_write16(struct i2c_client *client,
	u16 addr, u16 val);
int fimc_is_sensor_write16_array(struct i2c_client *client,
	u16 addr, u16 *val, u32 num);
int fimc_is_sensor_write16_burst(struct i2c_client *client,
	u16 addr, u16 *val, u32 num);
#endif
