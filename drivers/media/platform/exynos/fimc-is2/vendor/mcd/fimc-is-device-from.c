/*
 * driver for FIMC-IS FROM SPI
 *
 * Copyright (c) 2011, Samsung Electronics. All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/spi/spi.h>

#include "fimc-is-core.h"
#include "fimc-is-regs.h"

int fimc_is_spi_write(struct fimc_is_spi *spi, u32 addr, u8 *data, size_t size)
{
	int ret = 0;
	u8 tx_buf[512];
	size_t i = 0;

	tx_buf[0] = 0x02; /* write cmd */
	tx_buf[1] = (addr & 0xFF0000) >> 16; /* address */
	tx_buf[2] = (addr & 0xFF00) >> 8; /* address */
	tx_buf[3] = (addr & 0xFF); /* address */

	for (i = 0; i < size; i++) {
		tx_buf[i + 4] = *(data+ i);
	}

	ret = spi_write(spi->device, &tx_buf[0], i + 4);
	if (ret)
		err("spi sync error - can't read data");

	return ret;
}

int fimc_is_spi_write_enable(struct fimc_is_spi *spi)
{
	int ret = 0;
	u8 tx_buf;

	tx_buf = 0x06; /* write enable cmd */

	ret = spi_write(spi->device, &tx_buf, 1);
	if (ret)
		err("spi sync error - can't read data");

	return ret;
}

int fimc_is_spi_write_disable(struct fimc_is_spi *spi)
{
	int ret = 0;
	u8 tx_buf;

	tx_buf = 0x04; /* write disable cmd */

	ret = spi_write(spi->device, &tx_buf, 1);
	if (ret)
		err("spi sync error - can't read data");

	return ret;
}

int fimc_is_spi_read_status_bit(struct fimc_is_spi *spi, u8 *buf)
{
	unsigned char req_data;
	int ret;

	struct spi_transfer t_c;
	struct spi_transfer t_r;
	struct spi_message m;

	memset(&t_c, 0x00, sizeof(t_c));
	memset(&t_r, 0x00, sizeof(t_r));

	req_data = 0x05; /* read status cmd */

	t_c.tx_buf = &req_data;
	t_c.len = 1;
	t_c.cs_change = 1;
	t_c.bits_per_word = 8;

	t_r.rx_buf = buf;
	t_r.len = 1;
	t_r.cs_change = 0;
	t_r.bits_per_word = 8;

	spi_message_init(&m);
	spi_message_add_tail(&t_c, &m);
	spi_message_add_tail(&t_r, &m);

	ret = spi_sync(spi->device, &m);
	if (ret) {
		err("spi sync error - can't read data");
		return -EIO;
	} else {
		return 0;
	}
}

int fimc_is_spi_erase_block(struct fimc_is_spi *spi, u32 addr)
{
	int ret = 0;
	u8 tx_buf[4];

	tx_buf[0] = 0xD8; /* erase cmd */
	tx_buf[1] = (addr & 0xFF0000) >> 16; /* address */
	tx_buf[2] = (addr & 0xFF00) >> 8; /* address */
	tx_buf[3] = (addr & 0xFF); /* address */

	ret = spi_write(spi->device, &tx_buf[0], 4);
	if (ret)
		err("spi sync error - can't read data");

	return ret;
}

int fimc_is_spi_erase_sector(struct fimc_is_spi *spi, u32 addr)
{
	int ret = 0;
	u8 tx_buf[4];

	tx_buf[0] = 0x20; /* erase cmd */
	tx_buf[1] = (addr & 0xFF0000) >> 16; /* address */
	tx_buf[2] = (addr & 0xFF00) >> 8; /* address */
	tx_buf[3] = (addr & 0xFF); /* address */

	ret = spi_write(spi->device, &tx_buf[0], 4);
	if (ret)
		err("spi sync error - can't read data");

	return ret;
}

int fimc_is_spi_read_module_id(struct fimc_is_spi *spi, void *buf, u16 addr, size_t size)
{
	unsigned char req_data[4] = { 0x90,  };
	int ret;

	struct spi_transfer t_c;
	struct spi_transfer t_r;

	struct spi_message m;

	memset(&t_c, 0x00, sizeof(t_c));
	memset(&t_r, 0x00, sizeof(t_r));

	req_data[1] = (addr & 0xFF00) >> 8;
	req_data[2] = (addr & 0xFF);

	t_c.tx_buf = req_data;
	t_c.len = 4;
	t_c.cs_change = 1;
	t_c.bits_per_word = 32;

	t_r.rx_buf = buf;
	t_r.len = (u32)size;

	spi_message_init(&m);
	spi_message_add_tail(&t_c, &m);
	spi_message_add_tail(&t_r, &m);

	spi->device->max_speed_hz = 48000000;
	ret = spi_sync(spi->device, &m);
	if (ret) {
		err("spi sync error - can't read data");
		return -EIO;
	} else {
		return 0;
	}
}

MODULE_DESCRIPTION("FIMC-IS FROM SPI driver");
MODULE_LICENSE("GPL");
