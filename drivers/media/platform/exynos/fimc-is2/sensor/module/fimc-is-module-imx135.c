/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

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
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include <exynos-fimc-is-sensor.h>
#include "fimc-is-hw.h"
#include "fimc-is-core.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-resourcemgr.h"
#include "fimc-is-module-imx135.h"

#define SENSOR_NAME "IMX135"

static struct fimc_is_sensor_cfg config_imx135[] = {
	/* 4144x3106@30fps */
	FIMC_IS_SENSOR_CFG(4144, 3106, 30, 23, 0, CSI_DATA_LANES_4),
	/* 4144x2332@30fps */
	FIMC_IS_SENSOR_CFG(4144, 2332, 30, 18, 1, CSI_DATA_LANES_4),
	/* 1936x1450@24fps */
	FIMC_IS_SENSOR_CFG(1936, 1450, 24, 9, 2, CSI_DATA_LANES_4),
	/* 1936x1090@24fps */
	FIMC_IS_SENSOR_CFG(1936, 1090, 24, 7, 3, CSI_DATA_LANES_4),
	/* 1024x576@120fps */
	FIMC_IS_SENSOR_CFG(1024, 576, 120, 9, 4, CSI_DATA_LANES_4),
	/* 2072x1166@60fps */
	FIMC_IS_SENSOR_CFG(2072, 1166, 60, 18, 5, CSI_DATA_LANES_4),
};

static int sensor_imx135_init(struct v4l2_subdev *subdev, u32 val)
{
	int ret = 0;
	struct fimc_is_module_enum *module;

	FIMC_BUG(!subdev);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);

	pr_info("[MOD:D:%d] %s(%d)\n", module->id, __func__, val);

	return ret;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init = sensor_imx135_init
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops
};

#ifdef CONFIG_OF
static int sensor_imx135_power_setpin(struct device *dev)
{
	return 0;
}
#endif /* CONFIG_OF */

int sensor_imx135_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct v4l2_subdev *subdev_module;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor *device;
	struct sensor_open_extended *ext;

	FIMC_BUG(!fimc_is_dev);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		probe_err("core device is not yet probed");
		return -EPROBE_DEFER;
	}

	device = &core->sensor[SENSOR_IMX135_INSTANCE];

	subdev_module = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_module) {
		probe_err("subdev_module is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	module = &device->module_enum[atomic_read(&device->module_count)];
	atomic_inc(&device->module_count);
	module->sensor_id = SENSOR_NAME_IMX135;
	module->subdev = subdev_module;
	module->device = SENSOR_IMX135_INSTANCE;
	module->ops = NULL;
	module->client = client;
	module->active_width = 4128;
	module->active_height = 3096;
	module->pixel_width = module->active_width + 16;
	module->pixel_height = module->active_height + 10;
	module->max_framerate = 120;
	module->sensor_maker = "SONY";
	module->sensor_name = "IMX135";
	module->setfile_name = "setfile_imx135.bin";
	module->cfgs = ARRAY_SIZE(config_imx135);
	module->cfg = config_imx135;
	module->private_data = NULL;
	module->lanes = CSI_DATA_LANES_4;
	module->bitwidth = 10;
#ifdef CONFIG_OF
	module->power_setpin = sensor_imx135_power_setpin;
#endif

	ext = &module->ext;
	memset(ext, 0x0, sizeof(struct sensor_open_extended));
	ext->mipi_lane_num = 4;
	ext->sensor_con.product_name = 0;
	ext->sensor_con.peri_type = SE_I2C;
	//ext->sensor_con.peri_setting.i2c.channel = sensor_info->i2c_channel;
	//ext->sensor_con.peri_setting.i2c.slave_address = sensor_info->sensor_slave_address;
	ext->sensor_con.peri_setting.i2c.speed = 400000;

	ext->actuator_con.product_name = ACTUATOR_NAME_AK7345;
	ext->actuator_con.peri_type = SE_I2C;
	//ext->actuator_con.peri_setting.i2c.channel = sensor_info->actuator_i2c;

	//ext->flash_con.product_name = sensor_info->flash_id;
	ext->flash_con.peri_type = SE_GPIO;
	//ext->flash_con.peri_setting.gpio.first_gpio_port_no = sensor_info->flash_first_gpio;
	//ext->flash_con.peri_setting.gpio.second_gpio_port_no = sensor_info->flash_second_gpio;

	ext->from_con.product_name = FROMDRV_NAME_NOTHING;
	ext->mclk = 0;
	ext->mipi_lane_num = 0;
	ext->mipi_speed = 0;
	ext->fast_open_sensor = 0;
	ext->self_calibration_mode = 0;
	ext->I2CSclk = I2C_L0;

	ext->preprocessor_con.product_name = PREPROCESSOR_NAME_NOTHING;

	if (client) {
		v4l2_i2c_subdev_init(subdev_module, client, &subdev_ops);
	} else {
		v4l2_subdev_init(subdev_module, &subdev_ops);
	}

	v4l2_set_subdevdata(subdev_module, module);
	v4l2_set_subdev_hostdata(subdev_module, device);
	snprintf(subdev_module->name, V4L2_SUBDEV_NAME_SIZE, "sensor-subdev.%d", module->id);

p_err:
	probe_info("%s(%d)\n", __func__, ret);
	return ret;
}
