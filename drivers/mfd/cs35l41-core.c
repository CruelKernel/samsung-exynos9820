/*
 * Core MFD support for Cirrus Logic CS35L41 codec
 *
 * Copyright 2017 Cirrus Logic
 *
 * Author:	David Rhodes	<david.rhodes@cirrus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/mfd/core.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <sound/cs35l41.h>
#include <linux/mfd/cs35l41/core.h>

#define CS35L41_MFD_SYSFS_CLASS_NAME "cirrus"

static const struct mfd_cell cs35l41_devs[] = {
	{ .name = "cs35l41-codec", },
	{ .name = "cs35l41-cal", },
	{ .name = "cs35l41-bd", },
	{ .name = "cs35l41-pwr", },
};

static const char * const cs35l41_supplies[] = {
	"VA",
	"VP",
};

int cs35l41_dev_init(struct cs35l41_data *cs35l41)
{
	int ret, i;

	dev_set_drvdata(cs35l41->dev, cs35l41);
	dev_info(cs35l41->dev, "Prince MFD core probe\n");

	if (dev_get_platdata(cs35l41->dev))
		memcpy(&cs35l41->pdata, dev_get_platdata(cs35l41->dev),
		       sizeof(cs35l41->pdata));

	for (i = 0; i < ARRAY_SIZE(cs35l41_supplies); i++)
		cs35l41->supplies[i].supply = cs35l41_supplies[i];

	cs35l41->num_supplies = ARRAY_SIZE(cs35l41_supplies);

	ret = devm_regulator_bulk_get(cs35l41->dev, cs35l41->num_supplies,
					cs35l41->supplies);
	if (ret != 0) {
		dev_err(cs35l41->dev,
			"Failed to request core supplies: %d\n",
			ret);
		return ret;
	}

	ret = regulator_bulk_enable(cs35l41->num_supplies, cs35l41->supplies);
	if (ret != 0) {
		dev_err(cs35l41->dev,
			"Failed to enable core supplies: %d\n", ret);
		return ret;
	}

	/* returning NULL can be an option if in stereo mode */
	cs35l41->reset_gpio = devm_gpiod_get_optional(cs35l41->dev, "reset",
							GPIOD_OUT_LOW);
	if (IS_ERR(cs35l41->reset_gpio)) {
		ret = PTR_ERR(cs35l41->reset_gpio);
		cs35l41->reset_gpio = NULL;
		if (ret == -EBUSY) {
			dev_info(cs35l41->dev,
				 "Reset line busy, assuming shared reset\n");
		} else {
			dev_err(cs35l41->dev,
				"Failed to get reset GPIO: %d\n", ret);
			goto err;
		}
	}

	if (cs35l41->reset_gpio) {
		usleep_range(1000, 1100);
		gpiod_set_value_cansleep(cs35l41->reset_gpio, 1);
	}

	usleep_range(2000, 2100);

	ret = mfd_add_devices(cs35l41->dev, PLATFORM_DEVID_AUTO, cs35l41_devs,
				ARRAY_SIZE(cs35l41_devs),
				NULL, 0, NULL);
	if (ret) {
		dev_err(cs35l41->dev, "Failed to add subdevices: %d\n", ret);
		ret = -EINVAL;
		goto err;
	}

	return 0;

err:
	regulator_bulk_disable(cs35l41->num_supplies, cs35l41->supplies);
	return ret;
}

int cs35l41_dev_exit(struct cs35l41_data *cs35l41)
{
	mfd_remove_devices(cs35l41->dev);
	regulator_bulk_disable(cs35l41->num_supplies, cs35l41->supplies);
	return 0;
}
