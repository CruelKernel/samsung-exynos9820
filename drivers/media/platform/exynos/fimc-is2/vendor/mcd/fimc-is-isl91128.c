#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include "fimc-is-core.h"
#include "fimc-is-i2c-config.h"

#define DEVICE_NAME "fimc-is-isl91128"
/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is core functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

int isl91128_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct fimc_is_core *core;
	static bool probe_retried = false;
	u32 voltage_option = 0;
	struct device *dev;
	struct device_node *dnode;
	int ret;

	if (!fimc_is_dev)
		goto probe_defer;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core)
		goto probe_defer;

	dev = &client->dev;
	dnode = dev->of_node;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		probe_err("No I2C functionality found\n");
		return -ENODEV;
	}

	ret = of_property_read_u32(dnode, "voltage_option", &voltage_option);
	if (ret < 0) {
		probe_info("%s : voltage_option get fail", dev_driver_string(&client->dev));
		voltage_option = 0;
	}

	fimc_is_i2c_pin_config(client, I2C_PIN_STATE_ON);

	if (voltage_option == 1) { /* set 3.35V */
		probe_info("%s : output level is 3.35V", dev_driver_string(&client->dev));
		ret = fimc_is_sensor_addr8_write8(client, 0x00, 0x1D);
		ret = fimc_is_sensor_addr8_write8(client, 0x00, 0x9E);
	} else {
		probe_info("%s : output level is 3.3V", dev_driver_string(&client->dev));
	}

	ret = fimc_is_sensor_addr8_write8(client, 0x01, 0x30);
	if (ret < 0) {
		probe_err("i2c write fail %d", ret);
		goto probe_defer;
	}

	probe_info("%s %s[%ld]: probed!\n",
		dev_driver_string(&client->dev), dev_name(&client->dev), id->driver_data);

	return 0;

probe_defer:
	if (probe_retried) {
		probe_err("probe has already been retried!!");
	}

	probe_retried = true;
	probe_err("core device is not yet probed");
	return -EPROBE_DEFER;
}

static const struct of_device_id fimc_is_isl91128_match[] = {
	{
		.compatible = "samsung,fimc-is-isl91128",
	},
	{},
};
MODULE_DEVICE_TABLE(of, fimc_is_isl91128_match);

static const struct i2c_device_id fimc_is_isl91128_idt[] = {
	{ DEVICE_NAME, 0 },
	{},
};

static struct i2c_driver fimc_is_isl91128_driver = {
	.probe	= isl91128_probe,
	.driver = {
		.name	= DEVICE_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = fimc_is_isl91128_match,
		.suppress_bind_attrs = true,
	},
	.id_table = fimc_is_isl91128_idt
};

static int __init fimc_is_isl91128_init(void)
{
	int ret;

	ret = i2c_add_driver(&fimc_is_isl91128_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			fimc_is_isl91128_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(fimc_is_isl91128_init);

