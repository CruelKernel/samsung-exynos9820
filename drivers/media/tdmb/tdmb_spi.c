/*
 *
 * drivers/media/tdmb/tdmb_spi.c
 *
 * tdmb driver
 *
 * Copyright (C) (2011, Samsung Electronics)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/of_gpio.h>

#include "tdmb.h"

struct spi_device *spi_dmb;

static int tdmbspi_probe(struct spi_device *spi)
{
	int ret;

	spi_dmb = spi;

	DPRINTK("%s()\n", __func__);

	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 8;
	ret = spi_setup(spi);
	if (ret < 0) {
		DPRINTK("spi_setup() fail ret : %d\n", ret);
		return ret;
	}

	return 0;
}

static const struct of_device_id tdmb_spi_match_table[] = {
	{.compatible = "tdmb_spi_comp"},
	{}
};

static int tdmbspi_remove(struct spi_device *spi)
{
	return 0;
}

static struct spi_driver tdmbspi_driver = {
	.driver = {
		.name	= "tdmbspi",
		.of_match_table = tdmb_spi_match_table,
		.owner	= THIS_MODULE,
	},
	.probe	= tdmbspi_probe,
	.remove	= tdmbspi_remove,
};

static int __init tdmb_spi_init(void)
{
	return spi_register_driver(&tdmbspi_driver);
}

static void __exit tdmb_spi_exit(void)
{
	spi_unregister_driver(&tdmbspi_driver);
}

unsigned long tdmb_get_if_handle(void)
{
	DPRINTK("%s\n", __func__);
	return (unsigned long)spi_dmb;
}
EXPORT_SYMBOL_GPL(tdmb_get_if_handle);

late_initcall(tdmb_spi_init);
module_exit(tdmb_spi_exit);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("TDMB SPI Driver");
MODULE_LICENSE("GPL v2");
