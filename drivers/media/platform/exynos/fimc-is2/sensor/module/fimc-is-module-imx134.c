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
#include "fimc-is-module-imx134.h"

#define SENSOR_NAME "IMX134"

static struct fimc_is_sensor_cfg config_imx134[] = {
	/* 3280X2458@30fps */
	FIMC_IS_SENSOR_CFG(3280, 2458, 30, 15, 0, CSI_DATA_LANES_4),
	/* 3280X1846@30fps */
	FIMC_IS_SENSOR_CFG(3280, 1846, 30, 11, 1, CSI_DATA_LANES_4),
	/* 3280X2458@24fps */
	FIMC_IS_SENSOR_CFG(3280, 2458, 24, 12, 2, CSI_DATA_LANES_4),
	/* 3280X1846@24fps */
	FIMC_IS_SENSOR_CFG(3280, 1846, 24, 9, 3, CSI_DATA_LANES_4),
	/* 1936X1450@24fps */
	FIMC_IS_SENSOR_CFG(1936, 1450, 24, 12, 4, CSI_DATA_LANES_4),
	/* 1936X1090@24fps */
	FIMC_IS_SENSOR_CFG(1936, 1090, 24, 9, 5, CSI_DATA_LANES_4),
	/* 816X460@120fps */
	FIMC_IS_SENSOR_CFG(816, 460, 120, 7, 6, CSI_DATA_LANES_4),
	/* 1640X924@60fps */
	FIMC_IS_SENSOR_CFG(1640, 924, 60, 6, 7, CSI_DATA_LANES_4),
};

static int sensor_imx134_init(struct v4l2_subdev *subdev, u32 val)
{
	int ret = 0;
	struct fimc_is_module_enum *module;

	FIMC_BUG(!subdev);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);

	pr_info("[MOD:D:%d] %s(%d)\n", module->id, __func__, val);

	return ret;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init = sensor_imx134_init
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops
};

#ifdef CONFIG_OF
static int sensor_imx134_power_setpin(struct device *dev)
{
	return 0;
}
#endif /* CONFIG_OF */

int sensor_imx134_probe(struct i2c_client *client,
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

	device = &core->sensor[SENSOR_IMX134_INSTANCE];

	subdev_module = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_module) {
		probe_err("subdev_module is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	module = &device->module_enum[atomic_read(&device->module_count)];
	atomic_inc(&device->module_count);
	module->sensor_id = SENSOR_NAME_IMX134;
	module->subdev = subdev_module;
	module->device = SENSOR_IMX134_INSTANCE;
	module->ops = NULL;
	module->client = client;
	module->active_width = 3264;
	module->active_height = 2448;
	module->pixel_width = module->active_width + 16;
	module->pixel_height = module->active_height + 10;
	module->max_framerate = 300;
	module->position = SENSOR_POSITION_REAR;
	module->mode = CSI_MODE_CH0_ONLY;
	module->lanes = CSI_DATA_LANES_4;
	module->bitwidth = 10;
	module->sensor_maker = "SONY";
	module->sensor_name = "IMX134";
	module->setfile_name = "setfile_imx134.bin";
	module->cfgs = ARRAY_SIZE(config_imx134);
	module->cfg = config_imx134;
	module->ops = NULL;
	module->private_data = NULL;
#ifdef CONFIG_OF
	module->power_setpin = sensor_imx134_power_setpin;
#endif

	ext = &module->ext;
	ext->mipi_lane_num = module->lanes;
	ext->I2CSclk = I2C_L0;

	ext->sensor_con.product_name = SENSOR_NAME_IMX134;
	ext->sensor_con.peri_type = SE_I2C;
	ext->sensor_con.peri_setting.i2c.channel = SENSOR_CONTROL_I2C0;
	ext->sensor_con.peri_setting.i2c.slave_address = 0x34;
	ext->sensor_con.peri_setting.i2c.speed = 400000;

	ext->actuator_con.product_name = ACTUATOR_NAME_AK7345;//ACTUATOR_NAME_NOTHING;
	ext->actuator_con.peri_type = SE_I2C;
	ext->actuator_con.peri_setting.i2c.channel = SENSOR_CONTROL_I2C1;
	ext->actuator_con.peri_setting.i2c.slave_address = 0x34;
	ext->actuator_con.peri_setting.i2c.speed = 400000;

#ifdef CONFIG_LEDS_MAX77804
	ext->flash_con.product_name = FLADRV_NAME_MAX77693;
#endif
#ifdef CONFIG_LEDS_LM3560
	ext->flash_con.product_name = FLADRV_NAME_LM3560;
#endif
#ifdef CONFIG_LEDS_SKY81296
	ext->flash_con.product_name = FLADRV_NAME_SKY81296;
#endif
#ifdef CONFIG_LEDS_KTD2692
	ext->flash_con.product_name = FLADRV_NAME_KTD2692;
#endif
	ext->flash_con.peri_type = SE_GPIO;
	ext->flash_con.peri_setting.gpio.first_gpio_port_no = 1;
	ext->flash_con.peri_setting.gpio.second_gpio_port_no = 2;

	ext->from_con.product_name = FROMDRV_NAME_NOTHING;

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

