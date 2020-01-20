/*
 * Samsung Exynos5 SoC series Flash driver
 *
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>

#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>

#include "fimc-is-flash.h"
#include "fimc-is-flash-rt5033.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-core.h"

static int flash_rt5033_init(struct v4l2_subdev *subdev, u32 val)
{
	int ret = 0;
	struct fimc_is_flash *flash;

	FIMC_BUG(!subdev);

	flash = (struct fimc_is_flash *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!flash);

	/* TODO: init flash driver */
	flash->flash_data.mode = CAM2_FLASH_MODE_OFF;
	flash->flash_data.intensity = 100; /* TODO: Need to figure out min/max range */
	flash->flash_data.firing_time_us = 1 * 1000 * 1000; /* Max firing time is 1sec */
	flash->flash_data.flash_fired = false;

	gpio_request_one(flash->flash_gpio, GPIOF_OUT_INIT_LOW, "CAM_FLASH_GPIO_OUTPUT");
	gpio_free(flash->flash_gpio);
	gpio_request_one(flash->torch_gpio, GPIOF_OUT_INIT_LOW, "CAM_TORCH_GPIO_OUTPUT");
	gpio_free(flash->torch_gpio);

	return ret;
}

static int sensor_rt5033_flash_control(struct v4l2_subdev *subdev, enum flash_mode mode, u32 intensity)
{
	int ret = 0;
	struct fimc_is_flash *flash = NULL;

	FIMC_BUG(!subdev);

	flash = (struct fimc_is_flash *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!flash);

	if (mode == CAM2_FLASH_MODE_OFF) {
		ret = control_flash_gpio(flash->flash_gpio, 0);
		if (ret)
			err("capture flash off fail");
		ret = control_flash_gpio(flash->torch_gpio, 0);
		if (ret)
			err("torch flash off fail");
	} else if (mode == CAM2_FLASH_MODE_SINGLE) {
		ret = control_flash_gpio(flash->flash_gpio, intensity);
		if (ret)
			err("capture flash on fail");
	} else if (mode == CAM2_FLASH_MODE_TORCH) {
		ret = control_flash_gpio(flash->torch_gpio, intensity);
		if (ret)
			err("torch flash on fail");
	} else {
		err("Invalid flash mode");
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

int flash_rt5033_s_ctrl(struct v4l2_subdev *subdev, struct v4l2_control *ctrl)
{
	int ret = 0;
	struct fimc_is_flash *flash = NULL;

	FIMC_BUG(!subdev);

	flash = (struct fimc_is_flash *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!flash);

	switch(ctrl->id) {
	case V4L2_CID_FLASH_SET_INTENSITY:
		/* TODO : Check min/max intensity */
		if (ctrl->value < 0) {
			err("failed to flash set intensity: %d\n", ctrl->value);
			ret = -EINVAL;
			goto p_err;
		}
		flash->flash_data.intensity = ctrl->value;
		break;
	case V4L2_CID_FLASH_SET_FIRING_TIME:
		/* TODO : Check min/max firing time */
		if (ctrl->value < 0) {
			err("failed to flash set firing time: %d\n", ctrl->value);
			ret = -EINVAL;
			goto p_err;
		}
		flash->flash_data.firing_time_us = ctrl->value;
		break;
	case V4L2_CID_FLASH_SET_FIRE:
		ret =  sensor_rt5033_flash_control(subdev, flash->flash_data.mode, ctrl->value);
		if (ret) {
			err("sensor_rt5033_flash_control(mode:%d, val:%d) is fail(%d)",
					(int)flash->flash_data.mode, ctrl->value, ret);
			goto p_err;
		}
		break;
	default:
		err("err!!! Unknown CID(%#x)", ctrl->id);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init = flash_rt5033_init,
	.s_ctrl = flash_rt5033_s_ctrl,
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
};

static int __init flash_rt5033_probe(struct device *dev, struct i2c_client *client)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct v4l2_subdev *subdev_flash;
	struct fimc_is_device_sensor *device;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct fimc_is_flash *flash;
	u32 sensor_id = 0;
	struct device_node *dnode;

	FIMC_BUG(!fimc_is_dev);
	FIMC_BUG(!dev);

	dnode = dev->of_node;

	ret = of_property_read_u32(dnode, "id", &sensor_id);
	if (ret) {
		err("id read is fail(%d)", ret);
		goto p_err;
	}

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		probe_info("core device is not yet probed");
		ret = -EPROBE_DEFER;
		goto p_err;
	}

	device = &core->sensor[sensor_id];
	if (!device) {
		err("sensor device is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	sensor_peri = find_peri_by_flash_id(device, FLADRV_NAME_RT5033);
	if (!sensor_peri) {
		probe_info("sensor peri is net yet probed");
		return -EPROBE_DEFER;
	}

	flash = &sensor_peri->flash;
	if (!flash) {
		err("flash is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	subdev_flash = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_flash) {
		err("subdev_flash is NULL");
		ret = -ENOMEM;
		goto p_err;
	}
	sensor_peri->subdev_flash = subdev_flash;

	flash = &sensor_peri->flash;
	flash->id = FLADRV_NAME_RT5033;
	flash->subdev = subdev_flash;
	flash->client = client;

	flash->flash_gpio = of_get_named_gpio(dnode, "flash-gpio", 0);
	if (!gpio_is_valid(flash->flash_gpio)) {
		dev_err(dev, "failed to get PIN_RESET\n");
		return -EINVAL;
	}

	flash->torch_gpio = of_get_named_gpio(dnode, "torch-gpio", 0);
	if (!gpio_is_valid(flash->torch_gpio)) {
		dev_err(dev, "failed to get PIN_RESET\n");
		return -EINVAL;
	}

	flash->flash_data.mode = CAM2_FLASH_MODE_OFF;
	flash->flash_data.intensity = 100; /* TODO: Need to figure out min/max range */
	flash->flash_data.firing_time_us = 1 * 1000 * 1000; /* Max firing time is 1sec */

	if (client)
		v4l2_i2c_subdev_init(subdev_flash, client, &subdev_ops);
	else
		v4l2_subdev_init(subdev_flash, &subdev_ops);

	v4l2_set_subdevdata(subdev_flash, flash);
	v4l2_set_subdev_hostdata(subdev_flash, device);
	snprintf(subdev_flash->name, V4L2_SUBDEV_NAME_SIZE, "flash-subdev.%d", flash->id);

p_err:
	return ret;
}

static int flash_rt5033_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
	struct device *dev;

	FIMC_BUG(!client);

	dev = &client->dev;

	ret = flash_rt5033_probe(dev, client);
	if (ret < 0) {
		probe_err("flash rt5033 probe fail(%d)\n", ret);
		goto p_err;
	}

	probe_info("%s(i2c slave addr(%x) done\n", __func__, client->addr);

p_err:
	return ret;
}

static int __init flash_rt5033_platform_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev;

	FIMC_BUG(!pdev);

	dev = &pdev->dev;

	ret = flash_rt5033_probe(dev, NULL);
	if (ret < 0) {
		probe_err("flash rt5033 probe fail(%d)\n", ret);
		goto p_err;
	}

	probe_info("%s done\n", __func__);

p_err:
	return ret;
}

static const struct of_device_id exynos_fimc_is_sensor_flash_rt5033_match[] = {
	{
		.compatible = "samsung,sensor-flash-rt5033",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fimc_is_sensor_flash_rt5033_match);

/* register I2C driver */
static const struct i2c_device_id flash_rt5033_i2c_idt[] = {
	{ "RT5033", 0 },
	{},
};

static struct i2c_driver sensor_flash_rt5033_i2c_driver = {
	.probe	= flash_rt5033_i2c_probe,
	.driver = {
		.name	= "FIMC-IS-SENSOR-FLASH-RT5033-I2C",
		.owner	= THIS_MODULE,
		.of_match_table = exynos_fimc_is_sensor_flash_rt5033_match
		.suppress_bind_attrs = true,
	}
	.id_table = flash_rt5033_i2c_idt,
};

static int __init sensor_flash_rt5033_i2c_init(void)
{
	int ret;

	ret = i2c_add_driver(&sensor_flash_rt5033_i2c_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			sensor_flash_rt5033_i2c_driver.driver.name, ret);

	return ret;
};
late_initcall_sync(sensor_flash_rt5033_i2c_init);

/* register platform driver */
static struct platform_driver sensor_flash_rt5033_platform_driver = {
	.driver = {
		.name   = "FIMC-IS-SENSOR-FLASH-RT5033-PLATFORM",
		.owner  = THIS_MODULE,
		.of_match_table = exynos_fimc_is_sensor_flash_rt5033_match,
	}
};

static int __init sensor_flash_rt5033_platform_init(void)
{
	int ret;

	ret = platform_driver_probe(&sensor_flash_rt5033_platform_driver,
				flash_rt5033_platform_probe);
	if (ret)
		err("failed to probe %s driver: %d\n",
			sensor_flash_rt5033_platform_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_flash_rt5033_platform_init);
