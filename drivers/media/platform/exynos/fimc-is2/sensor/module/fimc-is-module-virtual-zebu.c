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
#include "fimc-is-dt.h"
#include "fimc-is-module-virtual-zebu.h"

#define SENSOR_NAME "VIRTUAL_ZEBU"
#define DEFAULT_SENSOR_WIDTH	640
#define DEFAULT_SENSOR_HEIGHT	480
#define SENSOR_MEMSIZE DEFAULT_SENSOR_WIDTH * DEFAULT_SENSOR_HEIGHT


static struct fimc_is_sensor_cfg config_virtual_zebu[] = {
	/* 640x480@30fps */
	FIMC_IS_SENSOR_CFG(DEFAULT_SENSOR_WIDTH, DEFAULT_SENSOR_HEIGHT, 30, 0, 0, CSI_DATA_LANES_4),
};

static struct fimc_is_vci vci_virtual_zebu[] = {
	{
		.pixelformat = V4L2_PIX_FMT_SBGGR10,
		.config = {{0, HW_FORMAT_RAW10}, {1, HW_FORMAT_UNKNOWN}, {2, HW_FORMAT_UNKNOWN}, {3, 0}}
	}, {
		.pixelformat = V4L2_PIX_FMT_SBGGR12,
		.config = {{0, HW_FORMAT_RAW10}, {1, HW_FORMAT_UNKNOWN}, {2, HW_FORMAT_UNKNOWN}, {3, 0}}
	}, {
		.pixelformat = V4L2_PIX_FMT_SBGGR16,
		.config = {{0, HW_FORMAT_RAW10}, {1, HW_FORMAT_UNKNOWN}, {2, HW_FORMAT_UNKNOWN}, {3, 0}}
	}
};

static int sensor_virtual_zebu_open(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	pr_info("%s\n", __func__);
	return 0;
}
static int sensor_virtual_zebu_close(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	pr_info("%s\n", __func__);
	return 0;
}
static int sensor_virtual_zebu_registered(struct v4l2_subdev *sd)
{
	pr_info("%s\n", __func__);
	return 0;
}

static void sensor_virtual_zebu_unregistered(struct v4l2_subdev *sd)
{
	pr_info("%s\n", __func__);
}

static const struct v4l2_subdev_internal_ops internal_ops = {
	.open = sensor_virtual_zebu_open,
	.close = sensor_virtual_zebu_close,
	.registered = sensor_virtual_zebu_registered,
	.unregistered = sensor_virtual_zebu_unregistered,
};

static int sensor_virtual_zebu_init(struct v4l2_subdev *subdev, u32 val)
{
	return 0;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init = sensor_virtual_zebu_init
};

static int sensor_virtual_zebu_s_stream(struct v4l2_subdev *subdev, int enable)
{
	int ret = 0;
	struct fimc_is_module_enum *sensor;

	pr_info("%s\n", __func__);

	sensor = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (!sensor) {
		err("sensor is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (enable) {
		ret = CALL_MOPS(sensor, stream_on, subdev);
		if (ret < 0) {
			err("s_duration is fail(%d)", ret);
			goto p_err;
		}
	} else {
		ret = CALL_MOPS(sensor, stream_off, subdev);
		if (ret < 0) {
			err("s_duration is fail(%d)", ret);
			goto p_err;
		}
	}

p_err:
	return 0;
}

static int sensor_virtual_zebu_s_param(struct v4l2_subdev *subdev, struct v4l2_streamparm *param)
{
	int ret = 0;
	struct fimc_is_module_enum *sensor;
	struct v4l2_captureparm *cp;
	struct v4l2_fract *tpf;
	u64 duration;

	FIMC_BUG(!subdev);
	FIMC_BUG(!param);

	pr_info("%s\n", __func__);

	cp = &param->parm.capture;
	tpf = &cp->timeperframe;

	if (!tpf->denominator) {
		err("denominator is 0");
		ret = -EINVAL;
		goto p_err;
	}

	if (!tpf->numerator) {
		err("numerator is 0");
		ret = -EINVAL;
		goto p_err;
	}

	duration = (u64)(tpf->numerator * 1000 * 1000 * 1000) /
					(u64)(tpf->denominator);

	sensor = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (!sensor) {
		err("sensor is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = CALL_MOPS(sensor, s_duration, subdev, duration);
	if (ret) {
		err("s_duration is fail(%d)", ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int sensor_virtual_zebu_s_format(struct v4l2_subdev *subdev, struct v4l2_mbus_framefmt *fmt)
{
	/* TODO */
	return 0;
}

static const struct v4l2_subdev_video_ops video_ops = {
	.s_stream = sensor_virtual_zebu_s_stream,
	.s_parm = sensor_virtual_zebu_s_param
};

static const struct v4l2_subdev_pad_ops pad_ops = {
	.set_fmt = sensor_virtual_zebu_s_format
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
	.video = &video_ops,
	.pad = &pad_ops
};

int sensor_virtual_zebu_stream_on(struct v4l2_subdev *subdev)
{
	return 0;
}

int sensor_virtual_zebu_stream_off(struct v4l2_subdev *subdev)
{
	return 0;
}

/*
 * @ brief
 * frame duration time
 * @ unit
 * nano second
 * @ remarks
 */
int sensor_virtual_zebu_s_duration(struct v4l2_subdev *subdev, u64 duration)
{
	return 0;
}

int sensor_virtual_zebu_g_min_duration(struct v4l2_subdev *subdev)
{
	return 0;
}

int sensor_virtual_zebu_g_max_duration(struct v4l2_subdev *subdev)
{
	return 0;
}

int sensor_virtual_zebu_s_exposure(struct v4l2_subdev *subdev, u64 exposure)
{
	return 0;
}

int sensor_virtual_zebu_g_min_exposure(struct v4l2_subdev *subdev)
{
	return 0;
}

int sensor_virtual_zebu_g_max_exposure(struct v4l2_subdev *subdev)
{
	return 0;
}

int sensor_virtual_zebu_s_again(struct v4l2_subdev *subdev, u64 sensitivity)
{
	return 0;
}

int sensor_virtual_zebu_g_min_again(struct v4l2_subdev *subdev)
{
	return 0;
}

int sensor_virtual_zebu_g_max_again(struct v4l2_subdev *subdev)
{
	return 0;
}

int sensor_virtual_zebu_s_dgain(struct v4l2_subdev *subdev)
{
	return 0;
}

int sensor_virtual_zebu_g_min_dgain(struct v4l2_subdev *subdev)
{
	return 0;
}

int sensor_virtual_zebu_g_max_dgain(struct v4l2_subdev *subdev)
{
	return 0;
}

struct fimc_is_sensor_ops module_virtual_zebu_ops = {
	.stream_on	= sensor_virtual_zebu_stream_on,
	.stream_off	= sensor_virtual_zebu_stream_off,
	.s_duration	= sensor_virtual_zebu_s_duration,
	.g_min_duration	= sensor_virtual_zebu_g_min_duration,
	.g_max_duration	= sensor_virtual_zebu_g_max_duration,
	.s_exposure	= sensor_virtual_zebu_s_exposure,
	.g_min_exposure	= sensor_virtual_zebu_g_min_exposure,
	.g_max_exposure	= sensor_virtual_zebu_g_max_exposure,
	.s_again	= sensor_virtual_zebu_s_again,
	.g_min_again	= sensor_virtual_zebu_g_min_again,
	.g_max_again	= sensor_virtual_zebu_g_max_again,
	.s_dgain	= sensor_virtual_zebu_s_dgain,
	.g_min_dgain	= sensor_virtual_zebu_g_min_dgain,
	.g_max_dgain	= sensor_virtual_zebu_g_max_dgain
};

#ifdef CONFIG_OF
static int sensor_virtual_zebu_power_setpin(struct platform_device *pdev,
	struct exynos_platform_fimc_is_module *pdata)
{
	return 0;
}
#endif

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
int sensor_virtual_zebu_vision_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct v4l2_subdev *subdev_module;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor *device;
	struct sensor_open_extended *ext;
	static bool probe_retried = false;
	struct exynos_platform_fimc_is_sensor_module *pdata;
	struct device *dev;

	if (!fimc_is_dev)
		goto probe_defer;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("core device is not yet probed");
		return -EPROBE_DEFER;
	}

	dev = &client->dev;

#ifdef CONFIG_OF
	fimc_is_sensor_module_parse_dt(pdev, sensor_virtual_zebu_power_setpin);
#endif
	pdata = dev->platform_data;
	device = &core->sensor[SENSOR_VIRTUAL_ZEBU_INSTANCE];

	subdev_module = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_module) {
		err("subdev_module is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	/* virtual_zebu */
	module = &device->module_enum[atomic_read(&device->module_count)];
	atomic_inc(&device->module_count);
	clear_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
	module->sensor_id = SENSOR_VIRTUAL_ZEBU_NAME;
	module->subdev = subdev_module;
	module->device = SENSOR_VIRTUAL_ZEBU_INSTANCE;
	module->ops = &module_virtual_zebu_ops;
	module->client = client;
	module->active_width = DEFAULT_SENSOR_WIDTH;
	module->active_height = DEFAULT_SENSOR_HEIGHT;
	module->pixel_width = module->active_width;
	module->pixel_height = module->active_height;
	module->max_framerate = 30;
	module->position = SENSOR_POSITION_FRONT;
	module->mode = CSI_MODE_CH0_ONLY;
	module->lanes = CSI_DATA_LANES_4;
	module->bitwidth = 10;
	module->sensor_maker = "SLSI";
	module->sensor_name = "virtual_zebu";
	module->setfile_name = "setfile_virtual_zebu.bin";
	module->cfgs = ARRAY_SIZE(config_virtual_zebu);
	module->cfg = config_virtual_zebu;
	module->private_data = kzalloc(sizeof(struct fimc_is_module_virtual_zebu), GFP_KERNEL);
	if (!module->private_data) {
		err("private_data is NULL");
		ret = -ENOMEM;
		kfree(subdev_module);
		goto p_err;
	}

	ext = &module->ext;
	ext->mipi_lane_num = module->lanes;
	ext->I2CSclk = I2C_L0;
	ext->sensor_con.product_name = SENSOR_VIRTUAL_ZEBU_NAME;
	ext->sensor_con.peri_type = SE_I2C;
	ext->sensor_con.peri_setting.i2c.channel = SENSOR_CONTROL_I2C1;
	ext->sensor_con.peri_setting.i2c.slave_address = 0;
	ext->sensor_con.peri_setting.i2c.speed = 400000;

	ext->from_con.product_name = FROMDRV_NAME_NOTHING;

	ext->companion_con.product_name = PREPROCESSOR_NAME_NOTHING;

	if (client) {
		v4l2_i2c_subdev_init(subdev_module, client, &subdev_ops);
		subdev_module->internal_ops = &internal_ops;
	} else {
		v4l2_subdev_init(subdev_module, &subdev_ops);
	}

	v4l2_set_subdevdata(subdev_module, module);
	v4l2_set_subdev_hostdata(subdev_module, device);
	snprintf(subdev_module->name, V4L2_SUBDEV_NAME_SIZE, "sensor-subdev.%d", module->sensor_id);

p_err:
	probe_info("%s(%d)\n", __func__, ret);
	return ret;

probe_defer:
	if (probe_retried) {
		err("probe has already been retried!!");
		BUG();
	}

	probe_retried = true;
	err("core device is not yet probed");
	return -EPROBE_DEFER;
}

static int sensor_virtual_zebu_vision_remove(struct i2c_client *client)
{
	int ret = 0;
	return ret;
}

#ifdef CONFIG_OF
static const struct of_device_id exynos_fimc_is_sensor_virtual_zebu_vision_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-sensor-virtual_zebu-vision",
	},
	{},
};
#endif

static const struct i2c_device_id sensor_virtual_zebu_vision_idt[] = {
	{ SENSOR_NAME, 0 },
};

static struct i2c_driver sensor_virtual_zebu_vision_driver = {
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = exynos_fimc_is_sensor_virtual_zebu_vision_match
#endif
	},
	.probe	= sensor_virtual_zebu_vision_probe,
	.remove	= sensor_virtual_zebu_vision_remove,
	.id_table = sensor_virtual_zebu_vision_idt
};

static int __init sensor_virtual_zebu_load(void)
{
        return i2c_add_driver(&sensor_virtual_zebu_vision_driver);
}

static void __exit sensor_virtual_zebu_unload(void)
{
        i2c_del_driver(&sensor_virtual_zebu_vision_driver);
}

module_init(sensor_virtual_zebu_load);
module_exit(sensor_virtual_zebu_unload);
#endif /* CONFIG_I2C || CONFIG_I2C_MODULE */

int sensor_virtual_zebu_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct v4l2_subdev *subdev_module;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor *device;
	struct sensor_open_extended *ext;
	static bool probe_retried = false;
	struct exynos_platform_fimc_is_module *pdata;
	struct device *dev;

	if (!fimc_is_dev)
		goto probe_defer;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("core device is not yet probed");
		return -EPROBE_DEFER;
	}

	dev = &pdev->dev;

#ifdef CONFIG_OF
	fimc_is_sensor_module_parse_dt(pdev, sensor_virtual_zebu_power_setpin);
#endif
	pdata = dev_get_platdata(dev);
	device = &core->sensor[pdata->id];

	subdev_module = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_module) {
		err("subdev_module is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	/* S5Kvirtual_zebu */
	module = &device->module_enum[atomic_read(&device->module_count)];
	atomic_inc(&device->module_count);
	module->pdata = pdata;
	module->dev = dev;
	module->sensor_id = SENSOR_NAME_VIRTUAL_ZEBU;
	module->subdev = subdev_module;
	module->device = pdata->id;
	module->client = NULL;
	module->ops = &module_virtual_zebu_ops;
	module->active_width = DEFAULT_SENSOR_WIDTH;
	module->active_height = DEFAULT_SENSOR_HEIGHT;
	module->pixel_width = module->active_width;
	module->pixel_height = module->active_height;
	//module->pixel_width = module->active_width + 16;
	//module->pixel_height = module->active_height + 10;
	module->max_framerate = 30;
	module->position = pdata->position;
	module->mode = CSI_MODE_CH0_ONLY;
	module->lanes = CSI_DATA_LANES_4;
	module->vcis = ARRAY_SIZE(vci_virtual_zebu);
	module->vci = vci_virtual_zebu;
	module->sensor_maker = "SLSI";
	module->sensor_name = "virtual_zebu";
	module->setfile_name = "setfile_virtual_zebu.bin";
	module->cfgs = ARRAY_SIZE(config_virtual_zebu);
	module->cfg = config_virtual_zebu;
	module->private_data = kzalloc(sizeof(struct fimc_is_module_virtual_zebu), GFP_KERNEL);
	if (!module->private_data) {
		err("private_data is NULL");
		ret = -ENOMEM;
		kfree(subdev_module);
		goto p_err;
	}

	ext = &module->ext;
	ext->mipi_lane_num = module->lanes;
	ext->I2CSclk = 0;

	ext->sensor_con.product_name = SENSOR_VIRTUAL_ZEBU_NAME;
	ext->sensor_con.peri_type = SE_I2C;
	ext->sensor_con.peri_setting.i2c.channel = pdata->sensor_i2c_ch;
	ext->sensor_con.peri_setting.i2c.slave_address = pdata->sensor_i2c_addr;
	ext->sensor_con.peri_setting.i2c.speed = 400000;

	ext->actuator_con.product_name = ACTUATOR_NAME_NOTHING;
	ext->flash_con.product_name = FLADRV_NAME_NOTHING;
	ext->from_con.product_name = FROMDRV_NAME_NOTHING;
	ext->preprocessor_con.product_name = PREPROCESSOR_NAME_NOTHING;
	ext->ois_con.product_name = OIS_NAME_NOTHING;

	v4l2_subdev_init(subdev_module, &subdev_ops);

	v4l2_set_subdevdata(subdev_module, module);
	v4l2_set_subdev_hostdata(subdev_module, device);
	snprintf(subdev_module->name, V4L2_SUBDEV_NAME_SIZE, "sensor-subdev.%d", module->sensor_id);

p_err:
	probe_info("%s(%d)\n", __func__, ret);
	return ret;

probe_defer:
	if (probe_retried) {
		err("probe has already been retried!!");
		BUG();
	}

	probe_retried = true;
	err("core device is not yet probed");
	return -EPROBE_DEFER;
}

#ifdef CONFIG_OF
static int sensor_virtual_zebu_remove(struct platform_device *pdev)
{
	int ret = 0;
	return ret;
}

static const struct of_device_id exynos_fimc_is_sensor_virtual_zebu_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-sensor-virtual-zebu",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fimc_is_sensor_virtual_zebu_match);

static struct platform_driver sensor_virtual_zebu_driver = {
	.probe  = sensor_virtual_zebu_probe,
	.remove = sensor_virtual_zebu_remove,
	.driver = {
		.name   = SENSOR_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = exynos_fimc_is_sensor_virtual_zebu_match,
	}
};

module_platform_driver(sensor_virtual_zebu_driver);
#else
static int __init fimc_is_sensor_module_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&sensor_virtual_zebu_driver);
	if (ret)
		err("platform_driver_register failed: %d\n", ret);

	return ret;
}

static void __exit fimc_is_sensor_module_exit(void)
{
	platform_driver_unregister(&sensor_virtual_zebu_driver);
}
module_init(fimc_is_sensor_module_init);
module_exit(fimc_is_sensor_module_exit);
#endif
MODULE_AUTHOR("Gilyeon lim");
MODULE_DESCRIPTION("Sensor virtual_zebu driver");
MODULE_LICENSE("GPL v2");

