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

#include "fimc-is-helper-ois-i2c.h"
#include "fimc-is-helper-i2c.h"

int fimc_is_ois_write(struct i2c_client *client,
		u16 addr, u8 data)
{
	int ret = 0;
	struct i2c_msg msg[1];
	u8 wbuf[3] = {0,};

	wbuf[0] = (addr & 0xff00) >> 8;
	wbuf[1] = (addr & 0xff);
	wbuf[2] = data;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 3;
	msg->buf = wbuf;

	ret = fimc_is_i2c_transfer(client->adapter, msg, 1);
	if (ret < 0) {
		pr_err("i2c treansfer fail(%d)", ret);
		ret = -EIO;
		return ret;
	}
	i2c_info("I2CR08(%d) : 0x%04X%04X\n", client->addr, addr, val);

	return ret;
}

int fimc_is_ois_read(struct i2c_client *client,
		u16 addr, u8 *data)
{
	int ret = 0;
	struct i2c_msg msg[2];
	u8 txbuf[2];
	u8 rxbuf[1] = {0,};

	*data = 0;
	txbuf[0] = (addr & 0xFF00) >> 8;
	txbuf[1] = (addr & 0xFF);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = txbuf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = rxbuf;

	ret = fimc_is_i2c_transfer(client->adapter, msg, 2);
	if (ret < 0) {
		pr_err("i2c register read failed");
		ret = -EIO;
		return ret;
	}

	*data = rxbuf[0];
	return ret;
}

int fimc_is_ois_read_multi(struct i2c_client *client,
		u16 addr, u8 *data, size_t size)
{
	int ret;
	u8 rxbuf[256] = {0,};
	u8 wbuf[2] = {0,};
	struct i2c_msg msg[2];

	wbuf[0] = (addr & 0xff00) >> 8;
	wbuf[1] = (addr & 0xff);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = wbuf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = size;
	msg[1].buf = rxbuf;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret < 0) {
		pr_err("i2c read failed");
		return -EIO;
	}

	i2c_info("%s: rxbuf[0](%d), rxbuf[1](%d)\n", __func__,
			rxbuf[0], rxbuf[1]);

	memcpy(data, rxbuf, size);

	return ret;
}

int fimc_is_ois_write_multi(struct i2c_client *client, u16 addr,
		u8 *data, int size)
{
	int ret = 0;
	int i = 0;
	struct i2c_msg msg[2];
	u8 wbuf[258] = {0,};

	wbuf[0] = (addr & 0xFF00) >> 8;
	wbuf[1] = (addr & 0xFF);

	for(i = 0; i < size - 2; i++)
		wbuf[i + 2] = *(data + i);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = size;
	msg[0].buf = wbuf;

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret < 0) {
		pr_err("multi i2c write failed");
		ret = -EIO;
	} else {
		ret = 0;
	}

	return ret;
}

