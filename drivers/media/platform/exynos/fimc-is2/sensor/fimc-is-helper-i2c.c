/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>

#include "fimc-is-helper-i2c.h"

#ifdef FIMC_IS_VIRTUAL_MODULE
static u8 emul_reg[1024*20] = {0};
static bool init_reg = false;

int i2c_emulator(struct i2c_adapter *adapter, struct i2c_msg *msg, u32 type)
{
	int ret = 0;
	int addr = 0;

	FIMC_BUG(!adapter);
	FIMC_BUG(!msg);

	if (init_reg == false) {
		memset(emul_reg, 0x1, sizeof(emul_reg));
		init_reg = true;
	}

	if (type == 2) { /* READ */
		if (msg[0].len == 1) {
			addr = *(msg[0].buf);
		} else if (msg[0].len == 2) {
			addr = (msg[0].buf[0] << 8 | msg[0].buf[1]);
		}

		if (msg[1].len == 1) {
			msg[1].buf[0] = emul_reg[addr];
		} else if (msg[1].len == 2) {
			msg[1].buf[0] = emul_reg[addr];
			msg[1].buf[1] = emul_reg[addr+1];
		}
	} else if (type == 1) { /* WRITE */
		if (msg[0].len == 2) {
			addr = msg[0].buf[0];
			emul_reg[addr] = msg[0].buf[1];
		} else if (msg[0].len == 3) {
			addr = (msg[0].buf[0] << 8 | msg[0].buf[1]);
			emul_reg[addr] = msg[0].buf[2];
		} else if (msg[0].len == 4) {
			addr = (msg[0].buf[0] << 8 | msg[0].buf[1]);
			emul_reg[addr] = msg[0].buf[2];
			emul_reg[addr+1] = msg[0].buf[3];
		}
	} else {
		pr_err("err, unknown type(%d)\n", type);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}
#endif

int fimc_is_i2c_transfer(struct i2c_adapter *adapter, struct i2c_msg *msg, u32 size)
{
	int ret = 0;
#ifdef FIMC_IS_VIRTUAL_MODULE
	ret = i2c_emulator(adapter, msg, size);
#else
	ret = i2c_transfer(adapter, msg, size);
#endif

	return ret;
}

int fimc_is_sensor_addr8_read8(struct i2c_client *client,
	u8 addr, u8 *val)
{
	int ret = 0;
	struct i2c_msg msg[2];
	u8 wbuf[1];

	if (!client->adapter) {
		pr_err("Could not find adapter!\n");
		ret = -ENODEV;
		goto p_err;
	}

	/* 1. I2C operation for writing. */
	msg[0].addr = client->addr;
	msg[0].flags = 0; /* write : 0, read : 1 */
	msg[0].len = 1;
	msg[0].buf = wbuf;
	wbuf[0] = addr;

	/* 2. I2C operation for reading data. */
	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = val;

	ret = fimc_is_i2c_transfer(client->adapter, msg, 2);
	if (ret < 0) {
		pr_err("i2c treansfer fail");
		goto p_err;
	}

	i2c_info("I2CR08(%d) [0x%04X] : 0x%04X\n", client->addr, addr, *val);
	return 0;
p_err:
	return ret;
}

int fimc_is_sensor_read8(struct i2c_client *client,
	u16 addr, u8 *val)
{
	int ret = 0;
	struct i2c_msg msg[2];
	u8 wbuf[2];

	if (!client->adapter) {
		pr_err("Could not find adapter!\n");
		ret = -ENODEV;
		goto p_err;
	}

	/* 1. I2C operation for writing. */
	msg[0].addr = client->addr;
	msg[0].flags = 0; /* write : 0, read : 1 */
	msg[0].len = 2;
	msg[0].buf = wbuf;
	/* TODO : consider other size of buffer */
	wbuf[0] = (addr & 0xFF00) >> 8;
	wbuf[1] = (addr & 0xFF);

	/* 2. I2C operation for reading data. */
	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = val;

	ret = fimc_is_i2c_transfer(client->adapter, msg, 2);
	if (ret < 0) {
		pr_err("i2c treansfer fail");
		goto p_err;
	}

	i2c_info("I2CR08(%d) [0x%04X] : 0x%04X\n", client->addr, addr, *val);
	return 0;
p_err:
	return ret;
}

int fimc_is_sensor_read16(struct i2c_client *client,
	u16 addr, u16 *val)
{
	int ret = 0;
	struct i2c_msg msg[2];
	u8 wbuf[2], rbuf[2] = {0, 0};

	if (!client->adapter) {
		pr_err("Could not find adapter!\n");
		ret = -ENODEV;
		goto p_err;
	}

	/* 1. I2C operation for writing. */
	msg[0].addr = client->addr;
	msg[0].flags = 0; /* write : 0, read : 1 */
	msg[0].len = 2;
	msg[0].buf = wbuf;
	/* TODO : consider other size of buffer */
	wbuf[0] = (addr & 0xFF00) >> 8;
	wbuf[1] = (addr & 0xFF);

	/* 2. I2C operation for reading data. */
	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 2;
	msg[1].buf = rbuf;

	ret = fimc_is_i2c_transfer(client->adapter, msg, 2);
	if (ret < 0) {
		pr_err("i2c treansfer fail");
		goto p_err;
	}

	*val = ((rbuf[0] << 8) | rbuf[1]);

	i2c_info("I2CR16(%d) [0x%04X] : 0x%04X\n", client->addr, addr, *val);

	return 0;
p_err:
	return ret;
}

int fimc_is_sensor_write(struct i2c_client *client,
	u8 *buf, u32 size)
{
	int ret = 0;
	int retry_count = 5;
	struct i2c_msg msg = {client->addr, 0, size, buf};

	do {
		ret = fimc_is_i2c_transfer(client->adapter, &msg, 1);
		if (likely(ret == 1))
			break;
		msleep(10);
	} while (retry_count-- > 0);

	if (ret != 1) {
		dev_err(&client->dev, "%s: I2C is not working.\n", __func__);
		return -EIO;
	}

	return 0;
}

int fimc_is_sensor_addr8_write8(struct i2c_client *client,
	u8 addr, u8 val)
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
	wbuf[0] = addr;
	wbuf[1]  = val;
	ret = fimc_is_i2c_transfer(client->adapter, msg, 1);
	if (ret < 0) {
		pr_err("i2c treansfer fail(%d)", ret);
		goto p_err;
	}

	i2c_info("I2CR08(%d) [0x%04X] : 0x%04X\n", client->addr, addr, val);
	return 0;
p_err:
	return ret;
}

int fimc_is_sensor_write8(struct i2c_client *client,
	u16 addr, u8 val)
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
	wbuf[0] = (addr & 0xFF00) >> 8;
	wbuf[1] = (addr & 0xFF);
	wbuf[2] = val;

	ret = fimc_is_i2c_transfer(client->adapter, msg, 1);
	if (ret < 0) {
		pr_err("i2c treansfer fail(%d)", ret);
		goto p_err;
	}

	i2c_info("I2CW08(%d) [0x%04X] : 0x%04X\n", client->addr, addr, val);

	return 0;
p_err:
	return ret;
}

int fimc_is_sensor_write8_array(struct i2c_client *client,
	u16 addr, u8 *val, u32 num)
{
	int ret = 0;
	struct i2c_msg msg[1];
	int i = 0;
	u8 wbuf[10];

	if (val == NULL) {
		pr_err("val array is null\n");
		ret = -ENODEV;
		goto p_err;
	}

	if (num > 4) {
		pr_err("currently limit max num is 4, need to fix it!\n");
		ret = -ENODEV;
		goto p_err;
	}

	if (!client->adapter) {
		pr_err("Could not find adapter!\n");
		ret = -ENODEV;
		goto p_err;
	}

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 2 + (num * 1);
	msg->buf = wbuf;
	wbuf[0] = (addr & 0xFF00) >> 8;
	wbuf[1] = (addr & 0xFF);
	for (i = 0; i < num; i++) {
		wbuf[(i * 1) + 2] = val[i];
	}

	ret = fimc_is_i2c_transfer(client->adapter, msg, 1);
	if (ret < 0) {
		pr_err("i2c treansfer fail(%d)", ret);
		goto p_err;
	}

	i2c_info("I2CW08(%d) [0x%04x] : 0x%04x\n", client->addr, addr, *val);

	return 0;
p_err:
	return ret;
}

int fimc_is_sensor_write16(struct i2c_client *client,
	u16 addr, u16 val)
{
	int ret = 0;
	struct i2c_msg msg[1];
	u8 wbuf[4];

	if (!client->adapter) {
		pr_err("Could not find adapter!\n");
		ret = -ENODEV;
		goto p_err;
	}

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 4;
	msg->buf = wbuf;
	wbuf[0] = (addr & 0xFF00) >> 8;
	wbuf[1] = (addr & 0xFF);
	wbuf[2] = (val & 0xFF00) >> 8;
	wbuf[3] = (val & 0xFF);

	ret = fimc_is_i2c_transfer(client->adapter, msg, 1);
	if (ret < 0) {
		pr_err("i2c treansfer fail(%d)", ret);
		goto p_err;
	}

	i2c_info("I2CW16(%d) [0x%04X] : 0x%04X\n", client->addr, addr, val);

	return 0;
p_err:
	return ret;
}

int fimc_is_sensor_write16_array(struct i2c_client *client,
	u16 addr, u16 *val, u32 num)
{
	int ret = 0;
	struct i2c_msg msg[1];
	int i = 0;
	u8 wbuf[10];

	if (val == NULL) {
		pr_err("val array is null\n");
		ret = -ENODEV;
		goto p_err;
	}

	if (num > 4) {
		pr_err("currently limit max num is 4, need to fix it!\n");
		ret = -ENODEV;
		goto p_err;
	}

	if (!client->adapter) {
		pr_err("Could not find adapter!\n");
		ret = -ENODEV;
		goto p_err;
	}

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 2 + (num * 2);
	msg->buf = wbuf;
	wbuf[0] = (addr & 0xFF00) >> 8;
	wbuf[1] = (addr & 0xFF);
	for (i = 0; i < num; i++) {
		wbuf[(i * 2) + 2] = (val[i] & 0xFF00) >> 8;
		wbuf[(i * 2) + 3] = (val[i] & 0xFF);
	}

	ret = fimc_is_i2c_transfer(client->adapter, msg, 1);
	if (ret < 0) {
		pr_err("i2c treansfer fail(%d)", ret);
		goto p_err;
	}

	i2c_info("I2CW16(%d) [0x%04x] : 0x%04x\n", client->addr, addr, *val);

	return 0;
p_err:
	return ret;
}

int fimc_is_sensor_write16_burst(struct i2c_client *client,
	u16 addr, u16 *val, u32 num)
{
	int ret = 0;
	struct i2c_msg msg[1];
	int i = 0;
	u8 *wbuf;

	if (val == NULL) {
		pr_err("val array is null\n");
		ret = -ENODEV;
		goto p_err;
	}

	if (!client->adapter) {
		pr_err("Could not find adapter!\n");
		ret = -ENODEV;
		goto p_err;
	}

	wbuf = kmalloc((2 + (num * 2)), GFP_KERNEL);
	if (!wbuf) {
		pr_err("failed to alloc buffer for burst i2c\n");
		ret = -ENODEV;
		goto p_err;
	}

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 2 + (num * 2);
	msg->buf = wbuf;
	wbuf[0] = (addr & 0xFF00) >> 8;
	wbuf[1] = (addr & 0xFF);
	for (i = 0; i < num; i++) {
		wbuf[(i * 2) + 2] = (val[(i * 2) * I2C_NEXT] & 0xFF00) >> 8;
		wbuf[(i * 2) + 3] = (val[(i * 2) * I2C_NEXT] & 0xFF);
	}

	ret = fimc_is_i2c_transfer(client->adapter, msg, 1);
	if (ret < 0) {
		pr_err("i2c treansfer fail(%d)", ret);
		goto p_err_free;
	}

	i2c_info("I2CW16(%d) [0x%04x] : 0x%04x\n", client->addr, addr, *val);
	kfree(wbuf);
	return 0;
p_err_free:
	kfree(wbuf);
p_err:
	return ret;
}
