/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * JiHoon Kim <jihoonn.kim@samsung.com>
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PANEL_SPI_H__
#define __PANEL_SPI_H__
#include <linux/spi/spi.h>
#include <linux/of_device.h>

int panel_spi_read_data(struct spi_device *spi, u8 addr, u8 *buf, int size);
int panel_spi_write_data(struct spi_device *spi, const u8 *cmd, int size);
#ifdef CONFIG_OF
struct spi_device *of_find_panel_spi_by_node(struct device_node *node);
#else
static inline struct spi_device *
of_find_panel_spi_by_node(struct device_node *node)
{
	return NULL;
}
#endif
#endif /* __PANEL_SPI_H__ */
