/*
 * Samsung Exynos5 SoC series Actuator driver
 *
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>

#include "fimc-is-actuator-ak7371.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-core.h"

#include "fimc-is-helper-i2c.h"

#include "interface/fimc-is-interface-library.h"

#define ACTUATOR_NAME		"AK7371"

extern struct fimc_is_sysfs_actuator sysfs_actuator;

#define AK7371_DEFAULT_FIRST_POSITION		120
#define AK7371_DEFAULT_FIRST_DELAY			2000

static int sensor_ak7371_write_position(struct i2c_client *client, u32 val)
{
	int ret = 0;
	u8 val_high = 0, val_low = 0;

	WARN_ON(!client);

	if (!client->adapter) {
		err("Could not find adapter!\n");
		ret = -ENODEV;
		goto p_err;
	}

	if (val > AK7371_POS_MAX_SIZE) {
		err("Invalid af position(position : %d, Max : %d).\n",
					val, AK7371_POS_MAX_SIZE);
		ret = -EINVAL;
		goto p_err;
	}

	val_high = (val & 0x01FF) >> 1;
	val_low = (val & 0x0001) << 7;

	ret = fimc_is_sensor_addr8_write8(client, AK7371_REG_POS_HIGH, val_high);
	if (ret < 0)
		goto p_err;
	ret = fimc_is_sensor_addr8_write8(client, AK7371_REG_POS_LOW, val_low);
	if (ret < 0)
		goto p_err;

	return ret;

p_err:
	ret = -ENODEV;
	return ret;
}

static int sensor_ak7371_valid_check(struct i2c_client * client)
{
	int i;

	WARN_ON(!client);

	if (sysfs_actuator.init_step > 0) {
		for (i = 0; i < sysfs_actuator.init_step; i++) {
			if (sysfs_actuator.init_positions[i] < 0) {
				warn("invalid position value, default setting to position");
				return 0;
			} else if (sysfs_actuator.init_delays[i] < 0) {
				warn("invalid delay value, default setting to delay");
				return 0;
			}
		}
	} else
		return 0;

	return sysfs_actuator.init_step;
}

static void sensor_ak7371_print_log(int step)
{
	int i;

	if (step > 0) {
		dbg_actuator("initial position ");
		for (i = 0; i < step; i++) {
			dbg_actuator(" %d", sysfs_actuator.init_positions[i]);
		}
		dbg_actuator(" setting");
	}
}

static int sensor_ak7371_init_position(struct i2c_client *client,
		struct fimc_is_actuator *actuator)
{
	int i;
	int ret = 0;
	int init_step = 0;

	init_step = sensor_ak7371_valid_check(client);

	if (init_step > 0) {
		for (i = 0; i < init_step; i++) {
			ret = sensor_ak7371_write_position(client, sysfs_actuator.init_positions[i]);
			if (ret < 0)
				goto p_err;

			mdelay(sysfs_actuator.init_delays[i]);
		}

		actuator->position = sysfs_actuator.init_positions[i];

		sensor_ak7371_print_log(init_step);

	} else {
		if (actuator->position > 0) {
			ret = sensor_ak7371_write_position(client, actuator->position);
			if (ret < 0)
				goto p_err;

			usleep_range(actuator->vendor_first_delay, actuator->vendor_first_delay + 10);

			dbg_actuator("initial position %d setting\n", actuator->vendor_first_pos);
		} else {
			ret = sensor_ak7371_write_position(client, actuator->vendor_first_pos);
			if (ret < 0)
				goto p_err;

			usleep_range(actuator->vendor_first_delay, actuator->vendor_first_delay + 10);

			actuator->position = actuator->vendor_first_pos;

			dbg_actuator("initial position %d setting\n", actuator->vendor_first_pos);
		}
	}

p_err:
	return ret;
}

int sensor_ak7371_actuator_init(struct v4l2_subdev *subdev, u32 val)
{
	int ret = 0;
	u8 product_id = 0;
	struct fimc_is_actuator *actuator;
	struct i2c_client *client = NULL;
#ifdef USE_CAMERA_HW_BIG_DATA
	struct cam_hw_param *hw_param = NULL;
	struct fimc_is_device_sensor *device = NULL;
#endif
#ifdef DEBUG_ACTUATOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	struct device *dev;
	struct device_node *dnode;

	WARN_ON(!subdev);

	dbg_actuator("%s\n", __func__);

	actuator = (struct fimc_is_actuator *)v4l2_get_subdevdata(subdev);
	WARN_ON(!actuator);

	client = actuator->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	dev = &client->dev;
	dnode = dev->of_node;

	ret = of_property_read_u32(dnode, "vendor_first_pos", &actuator->vendor_first_pos);
	if (ret) {
		err("vendor_first_pos read is fail(%d)", ret);
		//goto p_err;
	}

	ret = of_property_read_u32(dnode, "vendor_first_delay", &actuator->vendor_first_delay);
	if (ret) {
		err("vendor_first_delay read is fail(%d)", ret);
		//goto p_err;
	}

	I2C_MUTEX_LOCK(actuator->i2c_lock);
	ret = fimc_is_sensor_addr8_read8(client, 0x03, &product_id);

	if (ret < 0) {
#ifdef USE_CAMERA_HW_BIG_DATA
		device = v4l2_get_subdev_hostdata(subdev);
		if (device) {
			fimc_is_sec_get_hw_param(&hw_param, device->position);
		}
		if (hw_param)
			hw_param->i2c_af_err_cnt++;
#endif
		goto p_err;
	}

#ifdef USE_AF_SLEEP_MODE
	/* Go sleep mode */
	ret = fimc_is_sensor_addr8_write8(client, 0x02, 32);
#else
	/* ToDo: Cal init data from FROM */

	ret = sensor_ak7371_init_position(client, actuator);
	if (ret <0)
		goto p_err;

	/* Go active mode */
	ret = fimc_is_sensor_addr8_write8(client, 0x02, 0);
	if (ret <0)
		goto p_err;
#endif

	/* ToDo */
	/* Wait Settling(>20ms) */
	/* SysSleep(30/MS_PER_TICK, NULL); */

#ifdef DEBUG_ACTUATOR_TIME
	do_gettimeofday(&end);
	pr_info("[%s] time %lu us", __func__, (end.tv_sec - st.tv_sec) * 1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	I2C_MUTEX_UNLOCK(actuator->i2c_lock);
	return ret;
}

int sensor_ak7371_actuator_get_status(struct v4l2_subdev *subdev, u32 *info)
{
	int ret = 0;
	struct fimc_is_actuator *actuator = NULL;
	struct i2c_client *client = NULL;
	enum fimc_is_actuator_status status = ACTUATOR_STATUS_NO_BUSY;
#ifdef DEBUG_ACTUATOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	dbg_actuator("%s\n", __func__);

	WARN_ON(!subdev);
	WARN_ON(!info);

	actuator = (struct fimc_is_actuator *)v4l2_get_subdevdata(subdev);
	WARN_ON(!actuator);

	client = actuator->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	/*
	 * The info is busy flag.
	 * But, this module can't get busy flag.
	 */
	status = ACTUATOR_STATUS_NO_BUSY;
	*info = status;

#ifdef DEBUG_ACTUATOR_TIME
	do_gettimeofday(&end);
	pr_info("[%s] time %lu us", __func__, (end.tv_sec - st.tv_sec) * 1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_ak7371_actuator_set_position(struct v4l2_subdev *subdev, u32 *info)
{
	int ret = 0;
	struct fimc_is_actuator *actuator;
	struct i2c_client *client;
	u32 position = 0;
#ifdef DEBUG_ACTUATOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	WARN_ON(!subdev);
	WARN_ON(!info);

	dbg_actuator("%s\n", __func__);

	actuator = (struct fimc_is_actuator *)v4l2_get_subdevdata(subdev);
	WARN_ON(!actuator);

	client = actuator->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	I2C_MUTEX_LOCK(actuator->i2c_lock);
	position = *info;
	if (position > AK7371_POS_MAX_SIZE) {
		err("[%d] Invalid af position(position : %d, Max : %d).\n",
					actuator->device, position, AK7371_POS_MAX_SIZE);
		ret = -EINVAL;
		goto p_err;
	}

	/* position Set */
	ret = sensor_ak7371_write_position(client, position);
	if (ret <0)
		goto p_err;
	actuator->position = position;

	dbg_actuator("%s [%d]: position(%d)\n", __func__, actuator->device, position);

	/* Add the setting delay for preventing i2c fail issue.
	 * If this device is not sharing the i2c bus with others,
	 * this delay could be removed.
	 */
	if (actuator->id == ACTUATOR_NAME_AK7371)
		usleep_range(600, 610);

#ifdef DEBUG_ACTUATOR_TIME
	do_gettimeofday(&end);
	pr_info("[%s] time %lu us", __func__, (end.tv_sec - st.tv_sec) * 1000000 + (end.tv_usec - st.tv_usec));
#endif
p_err:
	I2C_MUTEX_UNLOCK(actuator->i2c_lock);
	return ret;
}

static int sensor_ak7371_actuator_g_ctrl(struct v4l2_subdev *subdev, struct v4l2_control *ctrl)
{
	int ret = 0;
	u32 val = 0;

	switch(ctrl->id) {
	case V4L2_CID_ACTUATOR_GET_STATUS:
		ret = sensor_ak7371_actuator_get_status(subdev, &val);
		if (ret < 0) {
			err("err!!! ret(%d), actuator status(%d)", ret, val);
			ret = -EINVAL;
			goto p_err;
		}
		break;
	default:
		err("err!!! Unknown CID(%#x)", ctrl->id);
		ret = -EINVAL;
		goto p_err;
	}

	ctrl->value = val;

p_err:
	return ret;
}

static int sensor_ak7371_actuator_s_ctrl(struct v4l2_subdev *subdev, struct v4l2_control *ctrl)
{
	int ret = 0;

	switch(ctrl->id) {
	case V4L2_CID_ACTUATOR_SET_POSITION:
		ret = sensor_ak7371_actuator_set_position(subdev, &ctrl->value);
		if (ret) {
			err("failed to actuator set position: %d, (%d)\n", ctrl->value, ret);
			ret = -EINVAL;
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

#ifdef USE_AF_SLEEP_MODE
static int sensor_ak7371_actuator_set_active(struct v4l2_subdev *subdev, int enable)
{
	int ret = 0;
	struct fimc_is_actuator *actuator;
	struct i2c_client *client = NULL;
	struct fimc_is_module_enum *module;

	WARN_ON(!subdev);

	actuator = (struct fimc_is_actuator *)v4l2_get_subdevdata(subdev);
	WARN_ON(!actuator);

	module = actuator->sensor_peri->module;
	pr_info("%s [%d]=%d\n", __func__, actuator->device, enable);

	client = actuator->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	I2C_MUTEX_LOCK(actuator->i2c_lock);

	if (enable) {
		sensor_ak7371_init_position(client, actuator);

		/* Go active mode */
		ret = fimc_is_sensor_addr8_write8(client, 0x02, 0);
		if (ret < 0)
			goto p_err;

	} else {
		/* Go sleep mode */
		ret = fimc_is_sensor_addr8_write8(client, 0x02, 32);
		if (ret < 0)
			goto p_err;
	}

p_err:
	I2C_MUTEX_UNLOCK(actuator->i2c_lock);
	return ret;
}
#endif

static const struct v4l2_subdev_core_ops core_ops = {
	.init = sensor_ak7371_actuator_init,
	.g_ctrl = sensor_ak7371_actuator_g_ctrl,
	.s_ctrl = sensor_ak7371_actuator_s_ctrl,
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
};

static struct fimc_is_actuator_ops actuator_ops = {
#ifdef USE_AF_SLEEP_MODE
	.set_active = sensor_ak7371_actuator_set_active,
#endif
};

static int sensor_ak7371_actuator_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret = 0;
	struct fimc_is_core *core= NULL;
	struct v4l2_subdev *subdev_actuator = NULL;
	struct fimc_is_actuator *actuator = NULL;
	struct fimc_is_device_sensor *device = NULL;
	u32 sensor_id = 0;
	struct device *dev;
	struct device_node *dnode;

	WARN_ON(!fimc_is_dev);
	WARN_ON(!client);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("core device is not yet probed");
		ret = -EPROBE_DEFER;
		goto p_err;
	}

	dev = &client->dev;
	dnode = dev->of_node;

	ret = of_property_read_u32(dnode, "id", &sensor_id);
	if (ret) {
		err("id read is fail(%d)", ret);
		goto p_err;
	}

	device = &core->sensor[sensor_id];

	actuator = kzalloc(sizeof(struct fimc_is_actuator), GFP_KERNEL);
	if (!actuator) {
		err("actuator is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	subdev_actuator = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_actuator) {
		err("subdev_actuator is NULL");
		ret = -ENOMEM;
		kfree(actuator);
		goto p_err;
	}

	/* This name must is match to sensor_open_extended actuator name */
	if (sensor_id == 2)
		actuator->id = ACTUATOR_NAME_AK7371_DUAL;
	else
		actuator->id = ACTUATOR_NAME_AK7371;

	actuator->subdev = subdev_actuator;
	actuator->device = sensor_id;
	actuator->client = client;
	actuator->position = 0;
	actuator->max_position = AK7371_POS_MAX_SIZE;
	actuator->pos_size_bit = AK7371_POS_SIZE_BIT;
	actuator->pos_direction = AK7371_POS_DIRECTION;
	actuator->i2c_lock = NULL;
	actuator->need_softlanding = 0;
	actuator->actuator_ops = &actuator_ops;

	actuator->vendor_first_pos = AK7371_DEFAULT_FIRST_POSITION;
	actuator->vendor_first_delay = AK7371_DEFAULT_FIRST_DELAY;

	device->subdev_actuator[sensor_id] = subdev_actuator;
	device->actuator[sensor_id] = actuator;

	v4l2_i2c_subdev_init(subdev_actuator, client, &subdev_ops);
	v4l2_set_subdevdata(subdev_actuator, actuator);
	v4l2_set_subdev_hostdata(subdev_actuator, device);

	snprintf(subdev_actuator->name, V4L2_SUBDEV_NAME_SIZE, "actuator-subdev.%d", actuator->id);

p_err:
	probe_info("%s done\n", __func__);
	return ret;
}

static const struct of_device_id sensor_actuator_ak7371_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-actuator-ak7371",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sensor_actuator_ak7371_match);

static const struct i2c_device_id sensor_actuator_ak7371_idt[] = {
	{ ACTUATOR_NAME, 0 },
	{},
};

static struct i2c_driver sensor_actuator_ak7371_driver = {
	.probe  = sensor_ak7371_actuator_probe,
	.driver = {
		.name	= ACTUATOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sensor_actuator_ak7371_match,
		.suppress_bind_attrs = true,
	},
	.id_table = sensor_actuator_ak7371_idt,
};

static int __init sensor_actuator_ak7371_init(void)
{
	int ret;

	ret = i2c_add_driver(&sensor_actuator_ak7371_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			sensor_actuator_ak7371_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_actuator_ak7371_init);
