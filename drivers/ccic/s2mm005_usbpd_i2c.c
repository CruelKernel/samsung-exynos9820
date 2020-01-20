/*
 * driver/ccic/s2mm005-i2c.c - S2MM005 USBPD I2C device driver
 *
 * Copyright (C) 2017 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#include <linux/ccic/s2mm005_usbpd.h>

int s2mm005_read_byte(const struct i2c_client *i2c, u16 reg, u8 *val, u16 size)
{
	int ret, i2c_retry; u8 wbuf[2];
	struct i2c_msg msg[2];
	struct s2mm005_data *usbpd_data = i2c_get_clientdata(i2c);

	mutex_lock(&usbpd_data->i2c_mutex);
	i2c_retry = 0;
	msg[0].addr = i2c->addr;
	msg[0].flags = i2c->flags;
	msg[0].len = 2;
	msg[0].buf = wbuf;
	msg[1].addr = i2c->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = size;
	msg[1].buf = val;

	wbuf[0] = (reg & 0xFF00) >> 8;
	wbuf[1] = (reg & 0xFF);

	do {
		ret = i2c_transfer(i2c->adapter, msg, ARRAY_SIZE(msg));
	} while (ret < 0 &&  i2c_retry++ < 5);

	if (ret < 0)
		dev_err(&i2c->dev, "i2c read16 fail reg:0x%x error %d\n", reg, ret);

	mutex_unlock(&usbpd_data->i2c_mutex);

	return ret;
}

int s2mm005_write_byte(const struct i2c_client *i2c, u16 reg, u8 *val, u16 size)
{
	int ret, i2c_retry; u8 buf[258] = {0,};
	struct i2c_msg msg[1];
	struct s2mm005_data *usbpd_data = i2c_get_clientdata(i2c);

	if (size > 256) {
		pr_err("I2C error, over the size %d", size);
		return -EIO;
	}

	mutex_lock(&usbpd_data->i2c_mutex);
	i2c_retry = 0;
	msg[0].addr = i2c->addr;
	msg[0].flags = 0;
	msg[0].len = size+2;
	msg[0].buf = buf;

	buf[0] = (reg & 0xFF00) >> 8;
	buf[1] = (reg & 0xFF);
	memcpy(&buf[2], val, size);

	do {
		ret = i2c_transfer(i2c->adapter, msg, 1);
	} while (ret < 0 &&  i2c_retry++ < 5);

	if (ret < 0)
		dev_err(&i2c->dev, "i2c write fail reg:0x%x error %d\n", reg, ret);

	mutex_unlock(&usbpd_data->i2c_mutex);

	return ret;
}

void s2mm005_sel_write(struct i2c_client *i2c, u16 reg, u8 *buf, i2c_slv_cmd_t cmd)
{
	u8 W_DATA[8] = {0,};

	switch (cmd) {
	case SEL_WRITE_BYTE:
	case SEL_WRITE_WORD:
	case SEL_WRITE_LONG:
		break;
	default:
		dev_err(&i2c->dev, "%s, cmd is worng cmd(%d)\n", __func__, cmd);
		return;
	}

	W_DATA[0] = 0x02;
	W_DATA[1] = cmd;
	W_DATA[2] = reg & 0xFF;
	W_DATA[3] = (reg & 0xFF00) >> 8;
	memcpy(&W_DATA[4], buf, cmd);

	s2mm005_write_byte(i2c, REG_I2C_SLV_CMD, W_DATA, cmd + 4);
}

void s2mm005_sel_read(struct i2c_client *i2c, u16 reg, u8 *buf, i2c_slv_cmd_t cmd)
{
	u8 W_DATA[4] = {0,};
	u8 size;

	switch (cmd) {
	case SEL_READ_BYTE:
	case SEL_READ_WORD:
	case SEL_READ_LONG:
		size = cmd >> 8;
		break;
	default:
		dev_err(&i2c->dev, "%s, cmd is worng cmd(%d)\n", __func__, cmd);
		return;
	}

	W_DATA[0] = 0x02;
	W_DATA[1] = cmd;
	W_DATA[2] = reg & 0xFF;
	W_DATA[3] = (reg & 0xFF00) >> 8;
	s2mm005_write_byte(i2c, S2MM005_REG_I2C_CMD, W_DATA, 4);

	s2mm005_read_byte(i2c, S2MM005_REG_I2C_RESPONSE, buf, size);
}

void s2mm005_int_clear(struct s2mm005_data *usbpd_data)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 val = 0;

	dev_info(usbpd_data->dev, "%s : -- clear clear -- \n", __func__);
	val = 0x01;
	s2mm005_write_byte(i2c, S2MM005_REG_I2C_CMD, &val, 1);
}

void s2mm005_system_reset(struct s2mm005_data *usbpd_data)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 R_DATA[2];
	u8 W_DATA[2];

	s2mm005_sel_read(i2c, 0x1064, R_DATA, SEL_READ_WORD);

	/* SYSTEM RESET */
	W_DATA[0] = R_DATA[0];
	W_DATA[1] = R_DATA[1];

	s2mm005_sel_write(i2c, 0x1068, W_DATA, SEL_WRITE_WORD);
}
