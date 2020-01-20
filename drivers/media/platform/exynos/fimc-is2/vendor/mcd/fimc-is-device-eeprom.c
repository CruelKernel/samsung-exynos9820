
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
#include <exynos-fimc-is-sensor.h>
#include "fimc-is-core.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-resourcemgr.h"
#include "fimc-is-hw.h"
#include "fimc-is-device-eeprom.h"
#include "fimc-is-vender-specific.h"

#define DRIVER_NAME "fimc_is_eeprom_i2c"
#define DRIVER_NAME_REAR "rear-eeprom-i2c"
#define DRIVER_NAME_FRONT "front-eeprom-i2c"
#define DRIVER_NAME_REAR2 "rear2-eeprom-i2c"
#define DRIVER_NAME_FRONT2 "front2-eeprom-i2c"
#define DRIVER_NAME_REAR3 "rear3-eeprom-i2c"
#define DRIVER_NAME_FRONT3 "front3-eeprom-i2c"

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

int sensor_eeprom_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct fimc_is_core *core;
	static bool probe_retried = false;
	struct fimc_is_device_eeprom *eeprom;
	struct fimc_is_vender_specific *specific;

	if (!fimc_is_dev)
		goto probe_defer;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core)
		goto probe_defer;

	specific = core->vender.private_data;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		probe_err("No I2C functionality found\n");
		return -ENODEV;
	}

	if (id->driver_data >= ROM_ID_REAR && id->driver_data < ROM_ID_MAX) {
		specific->eeprom_client[id->driver_data] = client;
		specific->rom_valid[id->driver_data] = true;
	} else {
		probe_err("rear eeprom device is failed!");
		return -ENODEV;
	}

	eeprom = kzalloc(sizeof(struct fimc_is_device_eeprom), GFP_KERNEL);
	if (!eeprom) {
		probe_err("fimc_is_device_eeprom is NULL");
		return -ENOMEM;
	}

	eeprom->client = client;
	eeprom->core = core;
	eeprom->driver_data = id->driver_data;

	i2c_set_clientdata(client, eeprom);

	if (client->dev.of_node) {
		if(fimc_is_vendor_rom_parse_dt(client->dev.of_node, eeprom->driver_data)) {
			probe_err("parsing device tree is fail");
			kfree(eeprom);
			return -ENODEV;
		}
	}

	probe_info("%s %s[%ld]: fimc_is_sensor_eeprom probed!\n",
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

static int sensor_eeprom_remove(struct i2c_client *client)
{
	int ret = 0;

	return ret;
}

#ifdef CONFIG_OF
static const struct of_device_id exynos_fimc_is_sensor_eeprom_match[] = {
	{
		.compatible = "samsung,rear-eeprom-i2c", .data = (void *)ROM_ID_REAR
	},
	{
		.compatible = "samsung,front-eeprom-i2c", .data = (void *)ROM_ID_FRONT
	},
	{
		.compatible = "samsung,rear2-eeprom-i2c", .data = (void *)ROM_ID_REAR2
	},
	{
		.compatible = "samsung,front2-eeprom-i2c", .data = (void *)ROM_ID_FRONT2
	},
	{
		.compatible = "samsung,rear3-eeprom-i2c", .data = (void *)ROM_ID_REAR3
	},
	{
		.compatible = "samsung,front3-eeprom-i2c", .data = (void *)ROM_ID_FRONT3
	},
	{},
};
#endif

static const struct i2c_device_id sensor_eeprom_idt[] = {
	{ DRIVER_NAME_REAR, ROM_ID_REAR },
	{ DRIVER_NAME_FRONT, ROM_ID_FRONT },
	{ DRIVER_NAME_REAR2, ROM_ID_REAR2 },
	{ DRIVER_NAME_FRONT2, ROM_ID_FRONT2 },
	{ DRIVER_NAME_REAR3, ROM_ID_REAR3 },
	{ DRIVER_NAME_FRONT3, ROM_ID_FRONT3 },
	{},
};

static struct i2c_driver sensor_eeprom_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = exynos_fimc_is_sensor_eeprom_match
#endif
	},
	.probe	= sensor_eeprom_probe,
	.remove	= sensor_eeprom_remove,
	.id_table = sensor_eeprom_idt
};

static int __init sensor_eeprom_load(void)
{
        return i2c_add_driver(&sensor_eeprom_driver);
}

static void __exit sensor_eeprom_unload(void)
{
        i2c_del_driver(&sensor_eeprom_driver);
}

module_init(sensor_eeprom_load);
module_exit(sensor_eeprom_unload);

MODULE_AUTHOR("Kyoungho Yun");
MODULE_DESCRIPTION("Camera eeprom driver");
MODULE_LICENSE("GPL v2");
