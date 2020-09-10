// SPDX-License-Identifier: GPL-2.0
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

#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/workqueue.h>

#if defined(CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif /* CONFIG_OF */

#define DRIVER_NAME "panel_spi"
#define MAX_PANEL_SPI_RX_BUF	(256)
enum {
	PANEL_SPI_CTRL_DATA_RX = 0,
	PANEL_SPI_CTRL_DATA_TX = 1,
};
static struct spi_device *panel_spi;
static DEFINE_MUTEX(panel_spi_lock);
int panel_spi_write_data(struct spi_device *spi, const u8 *cmd, int size)
{
	size_t i;
	int status;
	u8 ctrl_data = PANEL_SPI_CTRL_DATA_TX;
	struct spi_message msg;
	struct spi_transfer xfer[] = {
		{ .bits_per_word = 1, .len = 1, .tx_buf = &ctrl_data, },
		{ .bits_per_word = 8, .len = size, .tx_buf = cmd, },
	};

	if (unlikely(!spi || !cmd)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	pr_debug("%s, addr 0x%02X len %d\n", __func__, cmd[0], size);

	spi_message_init(&msg);
	for (i = 0; i < ARRAY_SIZE(xfer); i++)
		if (xfer[i].len > 0)
			spi_message_add_tail(&xfer[i], &msg);
	status = spi_sync(spi, &msg);
	if (status < 0) {
		pr_err("%s, failed to spi_sync %d\n", __func__, status);
		return status;
	}

	return 0;
}

int panel_spi_read_data(struct spi_device *spi, u8 addr, u8 *buf, int size)
{
	unsigned int i;
	int status;
	u8 ctrl_data = PANEL_SPI_CTRL_DATA_RX;
#ifdef CONFIG_SUPPORT_PANEL_SPI_SHORT_TEST
	u8 dummy_rx_buf[1];
#endif
	u8 rx_buf[MAX_PANEL_SPI_RX_BUF];
	struct spi_message msg;
	struct spi_transfer xfer[] = {
		{ .bits_per_word = 1, .len = 1, .tx_buf = &ctrl_data, },
		{ .bits_per_word = 8, .len = 1, .tx_buf = &addr, },
		{ .bits_per_word = 8, .len = size, .rx_buf = rx_buf, },
#ifdef CONFIG_SUPPORT_PANEL_SPI_SHORT_TEST
		{ .bits_per_word = 1, .len = 1, .rx_buf = dummy_rx_buf, },
#endif
	};

	if (unlikely(!spi || !buf)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	pr_debug("%s, addr 0x%02X len %d\n", __func__, addr, size);

	if (size > ARRAY_SIZE(rx_buf)) {
		pr_err("%s, read length(%d) should be less than %ld\n",
				__func__, size, ARRAY_SIZE(rx_buf));
		return -EINVAL;
	}

	memset(rx_buf, 0, sizeof(rx_buf));

	spi_message_init(&msg);
	for (i = 0; i < (unsigned int)ARRAY_SIZE(xfer); i++)
		if (xfer[i].len > 0)
			spi_message_add_tail(&xfer[i], &msg);

	status = spi_sync(spi, &msg);
	if (status < 0) {
		pr_err("%s, failed to spi_sync %d\n", __func__, status);
		return status;
	}

	for (i = 0; i < size; i++) {
		buf[i] = rx_buf[i];
		pr_debug("%s, rx_buf[%d] 0x%02X\n", __func__, i, buf[i]);
	}

	return size;
}

static int panel_spi_probe_dt(struct spi_device *spi)
{
	struct device_node *nc = spi->dev.of_node;
	int rc;
	u32 value;

	rc = of_property_read_u32(nc, "bits-per-word", &value);
	if (rc) {
		dev_err(&spi->dev, "%s has no valid 'bits-per-word' property (%d)\n",
				nc->full_name, rc);
		return rc;
	}
	spi->bits_per_word = value;

	rc = of_property_read_u32(nc, "spi-max-frequency", &value);
	if (rc) {
		dev_err(&spi->dev, "%s has no valid 'spi-max-frequency' property (%d)\n",
				nc->full_name, rc);
		return rc;
	}
	spi->max_speed_hz = value;

	return 0;
}

static int panel_spi_probe(struct spi_device *spi)
{
	int ret;

	if (unlikely(!spi)) {
		pr_err("%s, invalid spi\n", __func__);
		return -EINVAL;
	}

	ret = panel_spi_probe_dt(spi);
	if (ret < 0) {
		dev_err(&spi->dev, "%s, failed to parse device tree, ret %d\n",
				__func__, ret);
		return ret;
	}

	ret = spi_setup(spi);
	if (ret < 0) {
		dev_err(&spi->dev, "%s, failed to setup spi, ret %d\n", __func__, ret);
		return ret;
	}

	panel_spi = spi;

	return 0;
}

static int panel_spi_remove(struct spi_device *spi)
{
	return 0;
}

static const struct of_device_id panel_spi_match_table[] = {
	{   .compatible = "panel_spi",},
	{}
};

static struct spi_driver panel_spi_driver = {
	.driver = {
		.name		= DRIVER_NAME,
		.owner		= THIS_MODULE,
		.of_match_table = panel_spi_match_table,
	},
	.probe		= panel_spi_probe,
	.remove		= panel_spi_remove,
};

struct spi_device *of_find_panel_spi_by_node(struct device_node *node)
{
	//return (spi->dev.of_node == node) ? panel_spi : NULL;
	return panel_spi;
}
EXPORT_SYMBOL(of_find_panel_spi_by_node);

static int __init panel_spi_init(void)
{
	return spi_register_driver(&panel_spi_driver);
}
subsys_initcall(panel_spi_init);

static void __exit panel_spi_exit(void)
{
	spi_unregister_driver(&panel_spi_driver);
}
module_exit(panel_spi_exit);

MODULE_DESCRIPTION("spi driver for panel");
MODULE_LICENSE("GPL");
