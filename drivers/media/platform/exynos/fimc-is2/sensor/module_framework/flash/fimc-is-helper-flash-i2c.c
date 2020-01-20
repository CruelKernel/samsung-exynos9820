/*
 * Samsung Exynos5 SoC series Flash driver
 *
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include "fimc-is-helper-flash-i2c.h"

int fimc_is_flash_data_write(struct i2c_client *client,
	u8 addr, u8 val)
{
	int ret = 0;

	ret = i2c_smbus_write_byte_data(client, addr, val);
	if (ret < 0) {
		pr_err("i2c write is fail");
		return ret;
	}

	i2c_info("I2C(0x%x): reg(0x%x), val(0x%x)\n", __func__,
		client->addr, reg, val);

	return ret;
}

int fimc_is_flash_data_read(struct i2c_client *client,
	u8 addr, u8 *val)
{
	int ret = 0;

	ret = i2c_smbus_read_byte_data(client, addr);
	if (ret < 0) {
		pr_err("i2c read is fail");
		return ret;
	}

	*val = ret;
	ret = 0;

	i2c_info("I2C(0x%x): reg(0x%x), val(0x%x)\n", __func__,
		client->addr, reg, *val);

	return ret;
}

int fimc_is_flash_updata_reg(struct i2c_client *client,
	u8 addr, u8 data, u8 mask)
{
	int ret = 0;
	u8 temp;
	u8 val;

	ret = fimc_is_flash_data_read(client, addr, &temp);
	if (ret < 0)
		return ret;

	val = (data & mask) | (temp & (~mask));
	ret = fimc_is_flash_data_write(client, addr, val);
	if (ret < 0)
	    return ret;

	i2c_info("I2C(0x%x): addr(0x%x), val(0x%x)\n", __func__,
		client->addr, addr, val);

	return ret;
}
