/*
 * I2C bus interface to Cirrus Logic CS35L41 codec
 *
 * Copyright 2017 Cirrus Logic
 *
 * Author:	David Rhodes	<david.rhodes@cirrus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/of.h>

#include <linux/mfd/cs35l41/registers.h>
#include "sound/cs35l41.h"
#include "cs35l41.h"


static struct regmap_config cs35l41_regmap_i2c = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
	.reg_format_endian = REGMAP_ENDIAN_BIG,
	.val_format_endian = REGMAP_ENDIAN_BIG,
	.max_register = CS35L41_LASTREG,
	.reg_defaults = cs35l41_reg,
	.num_reg_defaults = ARRAY_SIZE(cs35l41_reg),
	.volatile_reg = cs35l41_volatile_reg,
	.readable_reg = cs35l41_readable_reg,
	.cache_type = REGCACHE_RBTREE,
};

static const struct of_device_id cs35l41_of_match[] = {
	{.compatible = "cirrus,cs35l40"},
	{.compatible = "cirrus,cs35l41"},
	{},
};
MODULE_DEVICE_TABLE(of, cs35l41_of_match);

static const struct i2c_device_id cs35l41_id_i2c[] = {
	{"cs35l40", 0},
	{"cs35l41", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, cs35l41_id_i2c);

static int cs35l41_i2c_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct cs35l41_platform_data *pdata = dev_get_platdata(dev);
	const struct regmap_config *regmap_config = &cs35l41_regmap_i2c;
	struct cs35l41_data *cs35l41;
	int ret;

	cs35l41 = devm_kzalloc(dev, sizeof(struct cs35l41_data), GFP_KERNEL);

	if (cs35l41 == NULL)
		return -ENOMEM;

	cs35l41->dev = dev;
	cs35l41->irq = client->irq;
	cs35l41->pdata = pdata;

	i2c_set_clientdata(client, cs35l41);
	cs35l41->regmap = devm_regmap_init_i2c(client, regmap_config);
	if (IS_ERR(cs35l41->regmap)) {
		ret = PTR_ERR(cs35l41->regmap);
		dev_err(cs35l41->dev, "Failed to allocate register map: %d\n",
			ret);
		return ret;
	}

	return cs35l41_dev_init(cs35l41);
}

static int cs35l41_i2c_remove(struct i2c_client *client)
{
	struct cs35l41_data *cs35l41 = i2c_get_clientdata(client);

	return cs35l41_dev_exit(cs35l41);
}

static struct i2c_driver cs35l41_i2c_driver = {
	.driver = {
		.name		= "cs35l41",
		.of_match_table = cs35l41_of_match,
	},
	.id_table	= cs35l41_id_i2c,
	.probe		= cs35l41_i2c_probe,
	.remove		= cs35l41_i2c_remove,
};

module_i2c_driver(cs35l41_i2c_driver);

MODULE_DESCRIPTION("I2C CS35L41 driver");
MODULE_AUTHOR("David Rhodes, Cirrus Logic Inc, <david.rhodes@cirrus.com>");
MODULE_LICENSE("GPL");
