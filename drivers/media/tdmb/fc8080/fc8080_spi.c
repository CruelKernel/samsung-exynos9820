/*
 *	Copyright(c) 2013 FCI Inc. All Rights Reserved
 *
 *	File name : fc8080_spi.c
 *
 *	Description : spi interface source file
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
 *	History :
 *	----------------------------------------------------------------------
 */

#include <linux/input.h>
#include <linux/spi/spi.h>
#include <linux/cache.h>

#include "fci_types.h"
#include "fc8080_regs.h"
#include "fci_oal.h"
#include "fc8080_spi.h"

#include "tdmb.h"

#define SPI_BMODE       0x00
#define SPI_WMODE       0x04
#define SPI_LMODE       0x08
#define SPI_RD_THRESH   0x30
#define SPI_RD_REG      0x20
#define SPI_READ        0x40
#define SPI_WRITE       0x00
#define SPI_AINC        0x80

#define CHIPID          0
#define DRIVER_NAME "fc8080_spi"

unsigned long fc8080_spi;
static u8 tx_data[32] __cacheline_aligned;

static DEFINE_MUTEX(lock);

int fc8080_spi_write_then_read(
	struct spi_device *spi
	, u8 *txbuf
	, u16 tx_length
	, u8 *rxbuf
	, u16 rx_length)
{
	s32 res;

	struct spi_message message;
	struct spi_transfer	x;

	spi_message_init(&message);
	memset(&x, 0, sizeof(x));

	spi_message_add_tail(&x, &message);

	x.tx_buf = txbuf;
	x.rx_buf = txbuf;
	x.len = tx_length + rx_length;

	res = spi_sync(spi, &message);

	memcpy(rxbuf, x.rx_buf + tx_length, rx_length);

	return res;
}

int fc8080_spi_write_then_burstread(
	struct spi_device *spi
	, u8 *txbuf
	, u16 tx_length
	, u8 *rxbuf
	, u16 rx_length)
{
	s32 res;

	struct spi_message	message;
	struct spi_transfer	x;

	spi_message_init(&message);
	memset(&x, 0, sizeof(x));

	spi_message_add_tail(&x, &message);

	x.tx_buf = txbuf;
	x.rx_buf = rxbuf;
	x.len = tx_length + rx_length;

	res = spi_sync(spi, &message);

	return res;
}

static s32 spi_bulkread(HANDLE handle, u16 addr, u8 command, u8 *data,
			u16 length)
{
	s32 res = BBM_OK;

	tx_data[0] = (u8) (addr & 0xff);
	tx_data[1] = (u8) ((addr >> 8) & 0xff);
	tx_data[2] = (u8) ((command & 0xfc) | CHIPID);
	tx_data[3] = (u8) (length & 0xff);

	res = fc8080_spi_write_then_read(
		(struct spi_device *)fc8080_spi, &tx_data[0], 4, &data[0], length);

	if (res) {
		print_log(0, "fc8080_spi_bulkread fail : %d\n", res);
		return BBM_NOK;
	}

	return BBM_OK;
}

static s32 spi_bulkwrite(HANDLE handle, u16 addr, u8 command, u8 *data,
			u16 length)
{
	s32 res = BBM_OK;
	s32 i = 0;

	tx_data[0] = (u8) (addr & 0xff);
	tx_data[1] = (u8) ((addr >> 8) & 0xff);
	tx_data[2] = (u8) ((command & 0xfc) | CHIPID);
	tx_data[3] = (u8) (length & 0xff);

	for (i = 0; i < length; i++)
		tx_data[4+i] = data[i];

	res = fc8080_spi_write_then_read(
		(struct spi_device *)fc8080_spi, &tx_data[0], length+4, NULL, 0);

	if (res) {
		print_log(0, "fc8080_spi_bulkwrite fail : %d\n", res);
		return BBM_NOK;
	}

	return BBM_OK;
}

static s32 spi_dataread(HANDLE handle, u16 addr, u8 command, u8 *data,
			u32 length)
{
	s32 res = BBM_OK;

	tx_data[0] = (u8) (addr & 0xff);
	tx_data[1] = (u8) ((addr >> 8) & 0xff);
	tx_data[2] = (u8) ((command & 0xfc) | CHIPID);
	tx_data[3] = (u8) (length & 0xff);

	res = fc8080_spi_write_then_burstread(
		(struct spi_device *)fc8080_spi, &tx_data[0], 4, &data[0], length);

	if (res) {
		print_log(0, "fc8080_spi_dataread fail : %d\n", res);
		return BBM_NOK;
	}

	return BBM_OK;
}

s32 fc8080_spi_init(HANDLE handle, unsigned long param)
{
	fc8080_spi = param;

	DPRINTK("%s\n", __func__);

	return BBM_OK;
}

s32 fc8080_spi_byteread(HANDLE handle, u16 addr, u8 *data)
{
	s32 res;
	u8 command = SPI_READ;

	mutex_lock(&lock);
	res = spi_bulkread(handle, addr, command, data, 1);
	mutex_unlock(&lock);

	return res;
}

s32 fc8080_spi_wordread(HANDLE handle, u16 addr, u16 *data)
{
	s32 res;
	u8 command = SPI_READ | SPI_AINC;

	mutex_lock(&lock);
	res = spi_bulkread(handle, addr, command, (u8 *) data, 2);
	mutex_unlock(&lock);

	return res;
}

s32 fc8080_spi_longread(HANDLE handle, u16 addr, u32 *data)
{
	s32 res;
	u8 command = SPI_READ | SPI_AINC;

	mutex_lock(&lock);
	res = spi_bulkread(handle, addr, command, (u8 *) data, 4);
	mutex_unlock(&lock);

	return res;
}

s32 fc8080_spi_bulkread(HANDLE handle, u16 addr, u8 *data, u16 length)
{
	s32 i;
	u16 x, y;
	s32 res = BBM_OK;
	u8 command = SPI_READ | SPI_AINC;

	x = length / 255;
	y = length % 255;

	mutex_lock(&lock);
	for (i = 0; i < x; i++, addr += 255)
		res |= spi_bulkread(handle, addr, command, &data[i * 255], 255);
	if (y)
		res |= spi_bulkread(handle, addr, command, &data[x * 255], y);
	mutex_unlock(&lock);

	return res;
}

s32 fc8080_spi_bytewrite(HANDLE handle, u16 addr, u8 data)
{
	s32 res;
	u8 command = SPI_WRITE;

	mutex_lock(&lock);
	res = spi_bulkwrite(handle, addr, command, (u8 *) &data, 1);
	mutex_unlock(&lock);

	return res;
}

s32 fc8080_spi_wordwrite(HANDLE handle, u16 addr, u16 data)
{
	s32 res;
	u8 command = SPI_WRITE;

	if ((addr & 0xff00) != 0x0f00)
		command |= SPI_AINC;

	mutex_lock(&lock);
	res = spi_bulkwrite(handle, addr, command, (u8 *) &data, 2);
	mutex_unlock(&lock);

	return res;
}

s32 fc8080_spi_longwrite(HANDLE handle, u16 addr, u32 data)
{
	s32 res;
	u8 command = SPI_WRITE | SPI_AINC;

	mutex_lock(&lock);
	res = spi_bulkwrite(handle, addr, command, (u8 *) &data, 4);
	mutex_unlock(&lock);

	return res;
}

s32 fc8080_spi_bulkwrite(HANDLE handle, u16 addr, u8 *data, u16 length)
{
	s32 i;
	u16 x, y;
	s32 res = BBM_OK;
	u8 command = SPI_WRITE | SPI_AINC;

	x = length / 255;
	y = length % 255;

	mutex_lock(&lock);
	for (i = 0; i < x; i++, addr += 255)
		res |= spi_bulkwrite(handle, addr, command, &data[i * 255],
					255);
	if (y)
		res |= spi_bulkwrite(handle, addr, command, &data[x * 255], y);
	mutex_unlock(&lock);

	return res;
}

s32 fc8080_spi_dataread(HANDLE handle, u16 addr, u8 *data, u32 length)
{
	s32 res;
	u8 command = SPI_READ | SPI_RD_THRESH;

	mutex_lock(&lock);
	res = spi_dataread(handle, addr, command, data, length);
	mutex_unlock(&lock);

	return res;
}

s32 fc8080_spi_deinit(HANDLE handle)
{
	fc8080_spi = 0;
	return BBM_OK;
}

