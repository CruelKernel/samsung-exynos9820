/*
 * amsung Exynos5 SoC series Sensor driver
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
#include <linux/of_gpio.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include <exynos-fimc-is-sensor.h>
#include "fimc-is-hw.h"
#include "fimc-is-core.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-resourcemgr.h"
#include "fimc-is-dt.h"

#include "fimc-is-helper-i2c.h"
#include "fimc-is-companion.h"
#include "fimc-is-device-companion.h"

#define FIMC_IS_DEV_COMP_DEV_NAME "73C3"

static const struct fimc_is_companion_cfg config_comp3_sensor_2l1_rear[] = {
	FIMC_IS_COMPANION_CFG(4032, 3024, 30, 0), /* 12Mp 4:3 (2PD Mode1) */
	FIMC_IS_COMPANION_CFG(4032, 2268, 30, 1), /* 12Mp 16:9 (2PD Mode1) */
	FIMC_IS_COMPANION_CFG(3024, 3024, 30, 2), /* 12Mp 16:9 (2PD Mode1) */
	FIMC_IS_COMPANION_CFG(4032, 2268, 60, 3), /* UHD 16:9 (2PD Mode2) */
	FIMC_IS_COMPANION_CFG(2016, 1134, 120, 4), /* FHD 16:9 (2PD Mode2) */
	FIMC_IS_COMPANION_CFG(2016, 1134, 30, 5), /* FHD 16:9 (2PD Mode2) */
	FIMC_IS_COMPANION_CFG(2016, 1134, 240, 6), /* HD 16:9 (2PD Mode2) */
	FIMC_IS_COMPANION_CFG(2016, 1512, 30, 7), /* 4:3 (2PD Mode2) */
	FIMC_IS_COMPANION_CFG(1504, 1504, 30, 8), /* 1:1 (2PD Mode2) */
	FIMC_IS_COMPANION_CFG(1008, 756, 120, 9), /* Fast AE 4:3 (2PD Mode2) */
};

static const struct fimc_is_companion_cfg config_comp3_sensor_2l1_front[] = {
	FIMC_IS_COMPANION_CFG(2608, 1960, 30, 11), /* Full mode */
	FIMC_IS_COMPANION_CFG(652, 488, 112, 12), /* 4x4 Binning mode */
};

static const struct fimc_is_companion_cfg *config_comp3_sensor_2l1[] = {
	config_comp3_sensor_2l1_rear,
	config_comp3_sensor_2l1_front,
};

static const u32 config_comp3_sensor_2l1_sizes[] = {
	ARRAY_SIZE(config_comp3_sensor_2l1_rear),
	ARRAY_SIZE(config_comp3_sensor_2l1_front),
};

int fimc_is_comp_stream_on(struct v4l2_subdev *subdev)
{
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;
	int ret = 0;
	u16 companion_id = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	fimc_is_comp_i2c_read(client, 0x0, &companion_id);
	dbg_preproc("Companion name: 0x%04x\n", companion_id);

	ret = fimc_is_comp_i2c_write(client, 0x6800, 1);
	if (ret) {
		err("i2c write fail");
	}

	fimc_is_comp_i2c_read(client, 0x6800, &companion_id);
	info("Companion stream on (%#x)\n", companion_id);
p_err:
	return ret;
}

int fimc_is_comp_stream_off(struct v4l2_subdev *subdev)
{
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;
	int ret = 0;
	u16 companion_id = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = fimc_is_comp_i2c_write(client, 0x6800, 0);
	if (ret) {
		err("i2c write fail");
	}

	fimc_is_comp_i2c_read(client, 0x6800, &companion_id);
	info("Companion stream off (%#x)\n", companion_id);
p_err:
	return ret;
}

int fimc_is_comp_mode_change(struct v4l2_subdev *subdev,
		struct fimc_is_device_sensor *device)
{
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;
	int ret = 0;
	u16 compmode, compdata;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	if (unlikely(!device)) {
		err("device_sensor is NULL");
		return -EINVAL;
	}

	ret = fimc_is_comp_get_mode(subdev, device, &compmode);
	if (ret) {
		merr("Companion search mode is fail (%d)\n", device, ret);
		return ret;
	}

	ret = fimc_is_comp_i2c_write(client, 0x0042, compmode);
	if (ret) {
		err("i2c write fail");
	}

	fimc_is_comp_i2c_read(client, 0x0042, &compdata);
	dbg_preproc("direct Companion mode: 0x%04x, %s\n", compdata, __func__);

	return ret;
}

int fimc_is_comp_debug(struct v4l2_subdev *subdev)
{
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;
	int ret = 0;
	u16 companion_id = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	fimc_is_comp_i2c_read(client, 0x6800, &companion_id);
	info("Companion stream off status: 0x%04x, %s\n", companion_id, __func__);
	fimc_is_comp_i2c_read(client, 0x11B0, &companion_id);
	info("DBG_COUNTER_0 : 0x%04x, %s\n", companion_id, __func__);
	fimc_is_comp_i2c_read(client, 0x11B2, &companion_id);
	info("DBG_COUNTER_1 : 0x%04x, %s\n", companion_id, __func__);
	fimc_is_comp_i2c_read(client, 0x11B4, &companion_id);
	info("DBG_COUNTER_2 : 0x%04x, %s\n", companion_id, __func__);
	fimc_is_comp_i2c_read(client, 0x11BA, &companion_id);
	info("DBG_COUNTER_5 : 0x%04x, %s\n", companion_id, __func__);
	fimc_is_comp_i2c_read(client, 0x11BC, &companion_id);
	info("DBG_COUNTER_6 : 0x%04x, %s\n", companion_id, __func__);
	fimc_is_comp_i2c_read(client, 0x11BE, &companion_id);
	info("DBG_COUNTER_7 : 0x%04x, %s\n", companion_id, __func__);
	fimc_is_comp_i2c_read(client, 0x11C0, &companion_id);
	info("DBG_COUNTER_8 : 0x%04x, %s\n", companion_id, __func__);
	fimc_is_comp_i2c_read(client, 0x11C6, &companion_id);
	info("DBG_COUNTER_11 : 0x%04x, %s\n", companion_id, __func__);
	fimc_is_comp_i2c_read(client, 0x11CA, &companion_id);
	info("DBG_COUNTER_13 : 0x%04x, %s\n", companion_id, __func__);
	fimc_is_comp_i2c_read(client, 0x11CC, &companion_id);
	info("DBG_COUNTER_14 : 0x%04x, %s\n", companion_id, __func__);
	fimc_is_comp_i2c_read(client, 0x11D0, &companion_id);
	info("DBG_COUNTER_15 : 0x%04x, %s\n", companion_id, __func__);
	fimc_is_comp_i2c_read(client, 0x0, &companion_id);
	info("Companion validation: 0x%04x\n", companion_id);

p_err:
	return ret;
}

int fimc_is_comp_wait_s_input(struct v4l2_subdev *subdev)
{
	int ret = 0;
	int waiting = 0;
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_device_preproc *device_preproc = NULL;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);


	device_preproc = preprocessor->device_preproc;
	if (!test_bit(FIMC_IS_PREPROC_OPEN, &device_preproc->state)) {
		err("preprocessor is not ready\n");
		BUG();
	}

	while (!test_bit(FIMC_IS_PREPROC_S_INPUT, &device_preproc->state)) {
		usleep_range(1000, 1000);
		if (waiting++ >= MAX_PREPROC_S_INPUT_WAITING) {
			err("preprocessor s_input is not finished\n");
			ret = -EBUSY;
			goto p_err;
		}
	}

	dbg_preproc("preproc finished(wait : %d)", waiting);

p_err:
	return ret;
}

int sensor_comp_init(struct v4l2_subdev *subdev, u32 val){
	int ret =0;
	return ret;
}

static struct fimc_is_preprocessor_ops preprocessor_ops = {
	.preprocessor_stream_on = fimc_is_comp_stream_on,
	.preprocessor_stream_off = fimc_is_comp_stream_off,
	.preprocessor_mode_change = fimc_is_comp_mode_change,
	.preprocessor_debug = fimc_is_comp_debug,
	.preprocessor_wait_s_input = fimc_is_comp_wait_s_input,
};

static const struct v4l2_subdev_core_ops core_ops = {
	.init = sensor_comp_init,
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
};

static int fimc_is_comp_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct fimc_is_core *core= NULL;
	struct v4l2_subdev *subdev_preprocessor = NULL;
	struct fimc_is_preprocessor *preprocessor = NULL;
	struct fimc_is_device_sensor *device = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct fimc_is_resourcemgr *resourcemgr = NULL;
	struct fimc_is_module_enum *module_enum = NULL;
	struct device_node *dnode;
	struct device *dev;
	u32 mindex = 0, mmax = 0;
	u32 node_id = 0;
	u32 position = 0;

	BUG_ON(!pdev);
	BUG_ON(!fimc_is_dev);

	dev = &pdev->dev;
	dnode = dev->of_node;

	if (fimc_is_dev == NULL) {
		warn("fimc_is_dev is not yet probed(preprop)");
		pdev->dev.init_name = FIMC_IS_DEV_COMP_DEV_NAME;
		ret =  -EPROBE_DEFER;
		goto p_err;
	}

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("core device is not yet probed");
		ret = -EPROBE_DEFER;
		goto p_err;
	}

	ret = of_property_read_u32(dnode, "id", &node_id);
	if (ret) {
		err("id read is fail(%d)", ret);
		goto p_err;
	}

	ret = of_property_read_u32(dnode, "position", &position);
	if (ret) {
		err("id read is fail(%d)", ret);
		goto p_err;
	}

	probe_info("%s position %d\n", __func__, position);

	device = &core->sensor[node_id];
	if (!device) {
		err("sensor device is NULL");
		ret = -ENOMEM;
		goto p_err;
	}
	probe_info("device module id = %d \n",device->pdata->id);

	resourcemgr = device->resourcemgr;
	module_enum = device->module_enum;

	mmax = atomic_read(&device->module_count);
	for (mindex = 0; mindex < mmax; mindex++) {
		if (module_enum[mindex].position == position) {
			sensor_peri = (struct fimc_is_device_sensor_peri *)module_enum[mindex].private_data;
			if (!sensor_peri) {
				return -EPROBE_DEFER;
			}
			probe_info("sensor id : %d \n", module_enum[mindex].sensor_id);
			break;
		}
	}

	preprocessor = &sensor_peri->preprocessor;
	if (!preprocessor) {
		err("preprocessor is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	subdev_preprocessor = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_preprocessor) {
		err("subdev_preprocessor is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	sensor_peri->subdev_preprocessor = subdev_preprocessor;

	preprocessor->device_preproc = &core->preproc;
	preprocessor->preprocessor_ops = &preprocessor_ops;

	/* This name must is match to sensor_open_extended actuator name */
	preprocessor = &sensor_peri->preprocessor;
	preprocessor->id = PREPROCESSOR_NAME_73C3;
	preprocessor->subdev = subdev_preprocessor;
	preprocessor->device = node_id;
	preprocessor->client = core->client0;
	preprocessor->cfg = config_comp3_sensor_2l1[position];
	preprocessor->cfgs = config_comp3_sensor_2l1_sizes[position];

	v4l2_subdev_init(subdev_preprocessor, &subdev_ops);
	v4l2_set_subdevdata(subdev_preprocessor, preprocessor);
	v4l2_set_subdev_hostdata(subdev_preprocessor, device);
	snprintf(subdev_preprocessor->name, V4L2_SUBDEV_NAME_SIZE, "preprop-subdev.%d", node_id);

	ret = v4l2_device_register_subdev(&device->v4l2_dev, subdev_preprocessor);
	if (ret) {
		merr("v4l2_device_register_subdev is fail(%d)", device, ret);
		goto p_err;
	}

	set_bit(FIMC_IS_SENSOR_COMPANION_AVAILABLE, &sensor_peri->peri_state);

p_err:
	return ret;
}

static int fimc_is_comp_remove(struct platform_device *pdev)
{
	int ret = 0;

	info("%s\n", __func__);

	return ret;
}

#ifdef CONFIG_OF
static const struct of_device_id exynos_fimc_device_companion_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-device-companion0",
	},
	{
		.compatible = "samsung,exynos5-fimc-is-device-companion1",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fimc_device_companion_match);

static struct platform_driver fimc_is_device_companion_driver = {
	.probe		= fimc_is_comp_probe,
	.remove		= fimc_is_comp_remove,
	.driver = {
		.name	= FIMC_IS_DEV_COMP_DEV_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = exynos_fimc_device_companion_match,
	}
};

#else
static struct platform_device_id fimc_is_device_companion_driver_ids[] = {
	{
		.name		= FIMC_IS_DEV_COMP_DEV_NAME,
		.driver_data	= 0,
	},
	{},
};
MODULE_DEVICE_TABLE(platform, fimc_is_device_companion_driver_ids);

static struct platform_driver fimc_is_device_companion_driver = {
	.probe		= fimc_is_comp_probe,
	.remove		= __devexit_p(fimc_is_comp_remove),
	.id_table	= fimc_is_device_companion_driver_ids,
	.driver	  = {
		.name	= FIMC_IS_DEV_COMP_DEV_NAME,
		.owner	= THIS_MODULE,
	}
};
#endif

static int __init fimc_is_comp_init(void)
{
	int ret = platform_driver_register(&fimc_is_device_companion_driver);
	if (ret)
		err("platform_driver_register failed: %d\n", ret);

	return ret;
}
late_initcall(fimc_is_comp_init);

static void __exit fimc_is_comp_exit(void)
{
	platform_driver_unregister(&fimc_is_device_companion_driver);
}
module_exit(fimc_is_comp_exit);

MODULE_AUTHOR("Wonjin LIM<wj.lim@samsung.com>");
MODULE_DESCRIPTION("Exynos FIMC_IS_DEVICE_COMPANION driver");
MODULE_LICENSE("GPL");
