/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 * Kimyung Lee <kernel.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PANEL_SPI_DEVICE_H__
#define __PANEL_SPI_DEVICE_H__

#include <linux/kernel.h>
#include <linux/miscdevice.h>

#include "panel.h"

#define DRIVER_NAME "panel_spi"
#define PANEL_SPI_DRIVER_NAME "panel_spi_driver"
#define MAX_PANEL_SPI_RX_BUF	(2048)

enum {
	PANEL_SPI_CTRL_DATA_RX = 0,
	PANEL_SPI_CTRL_DATA_TX = 1,
	PANEL_SPI_CTRL_RESET = 2,
	PANEL_SPI_CTRL_CMD = 3,
	PANEL_SPI_STATUS_1 = 4,
	PANEL_SPI_STATUS_2 = 5,
};
struct panel_spi_dev;

struct spi_drv_ops {
	int (*read)(struct panel_spi_dev *spi_dev, const u8 cmd, u8 *buf, int size);
	int (*write)(struct spi_device *spi, u32 addr, const u8 *buf, int size);
	int (*ctl)(struct spi_device *spi, int msg);
	int (*cmd)(struct panel_spi_dev *spi_dev, const u8 *wbuf, int wsize, u8 *rbuf, int rsize);
	int (*setparam)(struct panel_spi_dev *spi_dev, const u8 *wbuf, int wsize);
};

struct panel_spi_dev {
	struct miscdevice dev;
	struct spi_drv_ops *ops;
	struct spi_device *spi;
	u8 *setparam_buffer;
	u32 setparam_buffer_size;
	u8 *read_buf_data;
	u8 read_buf_cmd;
	u32 read_buf_size;
	u32 speed_hz;
};

struct spi_data {
	int spi_addr;
	u32 speed_hz;
};

int panel_spi_drv_probe(struct panel_device *panel, struct spi_data *spi_data);

#endif
