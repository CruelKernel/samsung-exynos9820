/*
 *
 * drivers/media/tdmb/tdmb_i2c.c
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
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>

#include "tdmb.h"

struct i2c_client *i2c_dmb;
static int tdmb_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct tdmb_i2c_dev *tdmb_dev = NULL;

	i2c_dmb = client;
	DPRINTK("tdmbi2c_probe() i2c_dmb : 0x%p\n", i2c_dmb);

	tdmb_dev = kzalloc(sizeof(*tdmb_dev), GFP_KERNEL);
	if (!tdmb_dev)
		return -ENOMEM;

	tdmb_dev->client = client;

	i2c_set_clientdata(client, tdmb_dev);

	return 0;
}

static const struct of_device_id tdmb_i2c_match_table[] = {
	{.compatible = "samsung,tdmb_i2c"},
	{}
};

static const struct i2c_device_id tdmb_i2c_id[] = {
	{"tdmbi2c", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, tdmb_i2c_id);

static int tdmb_i2c_remove(struct i2c_client *client)
{
	i2c_set_clientdata(client, NULL);
	return 0;
}

static struct i2c_driver tdmb_i2c_driver = {
	.driver = {
		.name	= "tdmbi2c",
		.of_match_table = tdmb_i2c_match_table,
		.owner	= THIS_MODULE,
	},
	.probe	= tdmb_i2c_probe,
	.remove	= tdmb_i2c_remove,
	.id_table = tdmb_i2c_id,
};

static int __init tdmb_i2c_init(void)
{
	return i2c_add_driver(&tdmb_i2c_driver);
}

static void __exit tdmb_i2c_exit(void)
{
	i2c_del_driver(&tdmb_i2c_driver);
}

unsigned long tdmb_get_if_handle(void)
{
	DPRINTK("%s : i2c_dmb 0x%p\n", __func__, i2c_dmb);
	return (unsigned long)i2c_dmb;
}
EXPORT_SYMBOL_GPL(tdmb_get_if_handle);

module_init(tdmb_i2c_init);
module_exit(tdmb_i2c_exit);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("TDMB I2C Driver");
MODULE_LICENSE("GPL v2");
