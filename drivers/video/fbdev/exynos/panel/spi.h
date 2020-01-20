/*
 * linux/drivers/video/fbdev/exynos/panel/spi.h
 *
 * Header file for Samsung Common LCD Driver.
 *
 * Copyright (c) 2016 Samsung Electronics
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

int panel_spi_read_data(struct spi_device *, u8 , u8 *, int);
int panel_spi_write_data(struct spi_device *, const u8 *, int);
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
