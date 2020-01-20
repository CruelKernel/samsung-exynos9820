/*
 * Samsung Exynos5 SoC series Actuator driver
 *
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include "fimc-is-helper-actuator-i2c.h"

int fimc_is_sensor_data_read16(struct i2c_client *client,
		u16 *val)
{
	int ret = 0;
	struct i2c_msg msg[1];
	u8 wbuf[2] = {0, 0};

	if (!client->adapter) {
		pr_err("Could not find adapter!\n");
		ret = -ENODEV;
		goto p_err;
	}

	/* 1. I2C operation for reading data */
	msg[0].addr = client->addr;
	msg[0].flags = I2C_M_RD; /* write : 0, read : 1 */
	msg[0].len = 2;
	msg[0].buf = wbuf;

	ret = fimc_is_i2c_transfer(client->adapter, msg, 1);
	if (ret < 0) {
		pr_err("i2c treansfer fail");
		goto p_err;
	}

	*val = ((wbuf[0] << 8) | wbuf[1]);

	i2c_info("I2CR16(%d) : 0x%08X\n", client->addr, *val);

	return 0;
p_err:
	return ret;
}

int fimc_is_sensor_data_write16(struct i2c_client *client,
		u8 val_high, u8 val_low)
{
	int ret = 0;
	struct i2c_msg msg[1];
	u8 wbuf[2];

	if (!client->adapter) {
		pr_err("Could not find adapter!\n");
		ret = -ENODEV;
		goto p_err;
	}

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 2;
	msg->buf = wbuf;
	wbuf[0] = val_high;
	wbuf[1] = val_low;
	ret = fimc_is_i2c_transfer(client->adapter, msg, 1);
	if (ret < 0) {
		pr_err("i2c treansfer fail(%d)", ret);
		goto p_err;
	}

	i2c_info("I2CR08(%d) : 0x%04X%04X\n", client->addr, val_high, val_low);
	return 0;
p_err:
	return ret;
}

int fimc_is_sensor_addr_data_write16(struct i2c_client *client,
		u8 addr, u8 val_high, u8 val_low)
{
	int ret = 0;
	struct i2c_msg msg[1];
	u8 wbuf[3];

	if (!client->adapter) {
		pr_err("Could not find adapter!\n");
		ret = -ENODEV;
		goto p_err;
	}

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 3;
	msg->buf = wbuf;
	wbuf[0] = addr;
	wbuf[1] = val_high;
	wbuf[2] = val_low;

	ret = fimc_is_i2c_transfer(client->adapter, msg, 1);
	if (ret < 0) {
		pr_err("i2c treansfer fail(%d)\n", ret);
		goto p_err;
	}

	i2c_info("I2CW16(%d) : 0x%04X%04X\n", client->addr, val_high, val_low);

	return 0;
p_err:
	return ret;
}
