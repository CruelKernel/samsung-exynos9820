/*
 *	Copyright(c) 2013 FCI Inc. All Rights Reserved
 *
 *	File name : fc8080_i2c.c
 *
 *	Description : i2c interface source file
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *	History :
 *	----------------------------------------------------------------------
 */
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/mutex.h>

#include "fci_types.h"
#include "fc8080_regs.h"
#include "fci_oal.h"
#include "tdmb.h"

#define CHIP_ADDR       0x58
#define I2C_M_FCIRD 1
#define I2C_M_FCIWR 0
#define I2C_MAX_SEND_LENGTH 500
struct i2c_ts_driver {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct work;
};
struct i2c_client *fc8080_i2c;

static DEFINE_MUTEX(lock);

static s32 i2c_bulkread(HANDLE handle, u8 chip, u16 addr, u8 *data, u16 length)
{
	int res;
	struct i2c_msg rmsg[2];
	unsigned char i2c_data[2];

	rmsg[0].addr = CHIP_ADDR;
	rmsg[0].flags = I2C_M_FCIWR;
	rmsg[0].len = 2;
	rmsg[0].buf = i2c_data;
	i2c_data[0] = (addr >> 8) & 0xff;
	i2c_data[1] = (addr) & 0xff;

	rmsg[1].addr = CHIP_ADDR;
	rmsg[1].flags = I2C_M_FCIRD;
	rmsg[1].len = length;
	rmsg[1].buf = data;
	res = i2c_transfer(fc8080_i2c->adapter, &rmsg[0], 2);

	return res;
}


static s32 i2c_bulkwrite(HANDLE handle, u8 chip, u16 addr, u8 *data, u16 length)
{
	int res;
	struct i2c_msg wmsg;
	unsigned char i2c_data[I2C_MAX_SEND_LENGTH];

	if ((length + 2) > I2C_MAX_SEND_LENGTH)
		return -ENODEV;

	wmsg.addr = CHIP_ADDR;
	wmsg.flags = I2C_M_FCIWR;
	wmsg.len = length + 2;
	wmsg.buf = i2c_data;
	i2c_data[0] = (addr >> 8) & 0xff;
	i2c_data[1] = (addr) & 0xff;
	memcpy(&i2c_data[2], data, length);

	res = i2c_transfer(fc8080_i2c->adapter, &wmsg, 1);

	return res;
}

static s32 i2c_dataread(HANDLE handle, u8 chip, u16 addr, u8 *data, u16 length)
{
	return i2c_bulkread(handle, chip, addr, data, length);
}

s32 fc8080_i2c_init(HANDLE handle, unsigned long param)
{
	static unsigned long fc8080_i2c_temp;

	fc8080_i2c_temp = param;
	fc8080_i2c = (struct i2c_client *)fc8080_i2c_temp;
	DPRINTK("%s : 0x%p\n", __func__, fc8080_i2c);

	return BBM_OK;
}

s32 fc8080_i2c_byteread(HANDLE handle, u16 addr, u8 *data)
{
	s32 res;

	mutex_lock(&lock);
	res = i2c_bulkread(handle, (u8) CHIP_ADDR, addr, data, 1);
	mutex_unlock(&lock);

	return res;
}

s32 fc8080_i2c_wordread(HANDLE handle, u16 addr, u16 *data)
{
	s32 res;

	mutex_lock(&lock);
	res = i2c_bulkread(handle, (u8) CHIP_ADDR, addr, (u8 *) data, 2);
	mutex_unlock(&lock);

	return res;
}

s32 fc8080_i2c_longread(HANDLE handle, u16 addr, u32 *data)
{
	s32 res;

	mutex_lock(&lock);
	res = i2c_bulkread(handle, (u8) CHIP_ADDR, addr, (u8 *) data, 4);
	mutex_unlock(&lock);

	return res;
}

s32 fc8080_i2c_bulkread(HANDLE handle, u16 addr, u8 *data, u16 length)
{
	s32 res;

	mutex_lock(&lock);
	res = i2c_bulkread(handle, (u8) CHIP_ADDR, addr, data, length);
	mutex_unlock(&lock);

	return res;
}

s32 fc8080_i2c_bytewrite(HANDLE handle, u16 addr, u8 data)
{
	s32 res;

	mutex_lock(&lock);
	res = i2c_bulkwrite(handle, (u8) CHIP_ADDR, addr, (u8 *) &data, 1);
	mutex_unlock(&lock);

	return res;
}

s32 fc8080_i2c_wordwrite(HANDLE handle, u16 addr, u16 data)
{
	s32 res;

	mutex_lock(&lock);
	res = i2c_bulkwrite(handle, (u8) CHIP_ADDR, addr, (u8 *) &data, 2);
	mutex_unlock(&lock);

	return res;
}

s32 fc8080_i2c_longwrite(HANDLE handle, u16 addr, u32 data)
{
	s32 res;

	mutex_lock(&lock);
	res = i2c_bulkwrite(handle, (u8) CHIP_ADDR, addr, (u8 *) &data, 4);
	mutex_unlock(&lock);

	return res;
}

s32 fc8080_i2c_bulkwrite(HANDLE handle, u16 addr, u8 *data, u16 length)
{
	s32 res;

	mutex_lock(&lock);
	res = i2c_bulkwrite(handle, (u8) CHIP_ADDR, addr, data, length);
	mutex_unlock(&lock);

	return res;
}

s32 fc8080_i2c_dataread(HANDLE handle, u16 addr, u8 *data, u32 length)
{
	s32 res;

	mutex_lock(&lock);
	res = i2c_dataread(handle, (u8) CHIP_ADDR, addr, data, length);
	mutex_unlock(&lock);

	return res;
}

s32 fc8080_i2c_deinit(HANDLE handle)
{
	return BBM_OK;
}

