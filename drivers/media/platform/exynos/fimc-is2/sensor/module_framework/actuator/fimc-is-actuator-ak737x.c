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

#include "fimc-is-actuator-ak737x.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-core.h"

#include "fimc-is-helper-i2c.h"

#include "interface/fimc-is-interface-library.h"

extern struct fimc_is_sysfs_actuator sysfs_actuator;

#define AK737X_DEFAULT_FIRST_POSITION		120
#define AK737X_DEFAULT_FIRST_DELAY			2000

static int sensor_ak737x_write_position(struct i2c_client *client, u32 val)
{
	int ret = 0;
	u8 val_high = 0, val_low = 0;

	WARN_ON(!client);

	if (!client->adapter) {
		err("Could not find adapter!\n");
		ret = -ENODEV;
		goto p_err;
	}

	if (val > AK737X_POS_MAX_SIZE) {
		err("Invalid af position(position : %d, Max : %d).\n",
					val, AK737X_POS_MAX_SIZE);
		ret = -EINVAL;
		goto p_err;
	}

	val_high = (val & 0x01FF) >> 1;
	val_low = (val & 0x0001) << 7;

	ret = fimc_is_sensor_addr8_write8(client, AK737X_REG_POS_HIGH, val_high);
	if (ret < 0)
		goto p_err;
	ret = fimc_is_sensor_addr8_write8(client, AK737X_REG_POS_LOW, val_low);
	if (ret < 0)
		goto p_err;

p_err:
	return ret;
}

static int sensor_ak737x_valid_check(struct i2c_client *client)
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

static void sensor_ak737x_print_log(int step)
{
	int i;

	if (step > 0) {
		dbg_actuator("initial position ");
		for (i = 0; i < step; i++)
			dbg_actuator(" %d", sysfs_actuator.init_positions[i]);
		dbg_actuator(" setting");
	}
}

static int sensor_ak737x_init_position(struct i2c_client *client,
		struct fimc_is_actuator *actuator)
{
	int i;
	int ret = 0;
	int init_step = 0;

	init_step = sensor_ak737x_valid_check(client);

	if (init_step > 0) {
		for (i = 0; i < init_step; i++) {
			ret = sensor_ak737x_write_position(client, sysfs_actuator.init_positions[i]);
			if (ret < 0)
				goto p_err;

			mdelay(sysfs_actuator.init_delays[i]);
		}

		actuator->position = sysfs_actuator.init_positions[i];

		sensor_ak737x_print_log(init_step);

	} else {
		/* wide, tele camera uses previous position at initial time */
		if (actuator->device == 1 || actuator->position == 0)
			actuator->position = actuator->vendor_first_pos;

		ret = sensor_ak737x_write_position(client, actuator->position);
		if (ret < 0)
			goto p_err;

		usleep_range(actuator->vendor_first_delay, actuator->vendor_first_delay + 10);

		dbg_actuator("initial position %d setting\n", actuator->vendor_first_pos);
	}

p_err:
	return ret;
}

int sensor_ak737x_actuator_init(struct v4l2_subdev *subdev, u32 val)
{
	int ret = 0;
	int i = 0;
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

	u32 product_id_list[AK737X_MAX_PRODUCT_LIST] = {0, };
	u32 product_id_len = 0;
	u8 product_id = 0;
	const u32 *product_id_spec;

	struct device *dev;
	struct device_node *dnode;

	WARN_ON(!subdev);

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

	product_id_spec = of_get_property(dnode, "vendor_product_id", &product_id_len);
	if (!product_id_spec)
		err("vendor_product_id num read is fail(%d)", ret);

	product_id_len /= (unsigned int)sizeof(*product_id_spec);

	ret = of_property_read_u32_array(dnode, "vendor_product_id", product_id_list, product_id_len);
	if (ret)
		err("vendor_product_id read is fail(%d)", ret);

	I2C_MUTEX_LOCK(actuator->i2c_lock);

	if (product_id_len < 2 || (product_id_len % 2) != 0
		|| product_id_len > AK737X_MAX_PRODUCT_LIST) {
		err("[%s] Invalid product_id in dts\n", __func__);
		ret = -EINVAL;
		goto p_err;
	}

	for (i = 0; i < product_id_len; i += 2) {
		ret = fimc_is_sensor_addr8_read8(client, product_id_list[i], &product_id);
		if (ret < 0) {
#ifdef USE_CAMERA_HW_BIG_DATA
			device = v4l2_get_subdev_hostdata(subdev);
			if (device)
				fimc_is_sec_get_hw_param(&hw_param, device->position);
			if (hw_param)
				hw_param->i2c_af_err_cnt++;
#endif
			goto p_err;
		}

		pr_info("[%s][%d] dt[addr=%x,id=%x], module id=%x\n",
				__func__, actuator->device, product_id_list[i], product_id_list[i+1], product_id);

		if (product_id_list[i+1] == product_id) {
			actuator->vendor_product_id = product_id_list[i+1];
			break;
		}
	}

	if (i == product_id_len) {
		err("[%s] Invalid product_id in module\n", __func__);
		ret = -EINVAL;
		goto p_err;
	}

	/* ToDo: Cal init data from FROM */

	if (actuator->vendor_use_sleep_mode) {
		/* Go sleep mode */
		ret = fimc_is_sensor_addr8_write8(client, 0x02, 32);
	} else {
		ret = sensor_ak737x_init_position(client, actuator);
		if (ret < 0)
			goto p_err;

		/* Go active mode */
		ret = fimc_is_sensor_addr8_write8(client, 0x02, 0);
		if (ret < 0)
			goto p_err;
	}

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

int sensor_ak737x_actuator_get_status(struct v4l2_subdev *subdev, u32 *info)
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

int sensor_ak737x_actuator_set_position(struct v4l2_subdev *subdev, u32 *info)
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
	if (position > AK737X_POS_MAX_SIZE) {
		err("Invalid af position(position : %d, Max : %d).\n",
					position, AK737X_POS_MAX_SIZE);
		ret = -EINVAL;
		goto p_err;
	}

	/* position Set */
	ret = sensor_ak737x_write_position(client, position);
	if (ret < 0)
		goto p_err;
	actuator->position = position;

	dbg_actuator("%s [%d]: position(%d)\n", __func__, actuator->device, position);

#ifdef DEBUG_ACTUATOR_TIME
	do_gettimeofday(&end);
	pr_info("[%s] time %lu us", __func__, (end.tv_sec - st.tv_sec) * 1000000 + (end.tv_usec - st.tv_usec));
#endif
p_err:
	I2C_MUTEX_UNLOCK(actuator->i2c_lock);
	return ret;
}

static int sensor_ak737x_actuator_g_ctrl(struct v4l2_subdev *subdev, struct v4l2_control *ctrl)
{
	int ret = 0;
	u32 val = 0;

	switch (ctrl->id) {
	case V4L2_CID_ACTUATOR_GET_STATUS:
		ret = sensor_ak737x_actuator_get_status(subdev, &val);
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

static int sensor_ak737x_actuator_s_ctrl(struct v4l2_subdev *subdev, struct v4l2_control *ctrl)
{
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_ACTUATOR_SET_POSITION:
		ret = sensor_ak737x_actuator_set_position(subdev, &ctrl->value);
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
static int sensor_ak737x_actuator_set_active(struct v4l2_subdev *subdev, int enable)
{
	int ret = 0;
	struct fimc_is_actuator *actuator;
	struct i2c_client *client = NULL;
	struct fimc_is_module_enum *module;

	WARN_ON(!subdev);

	actuator = (struct fimc_is_actuator *)v4l2_get_subdevdata(subdev);
	WARN_ON(!actuator);

	if (!actuator->vendor_use_sleep_mode) {
		warn("There is no 'use_af_sleep_mode' in DT");
		return 0;
	}

	module = actuator->sensor_peri->module;
	pr_info("%s [%d]=%d\n", __func__, actuator->device, enable);

	client = actuator->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	I2C_MUTEX_LOCK(actuator->i2c_lock);

	if (enable) {
		sensor_ak737x_init_position(client, actuator);

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
	.init = sensor_ak737x_actuator_init,
	.g_ctrl = sensor_ak737x_actuator_g_ctrl,
	.s_ctrl = sensor_ak737x_actuator_s_ctrl,
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
};

static struct fimc_is_actuator_ops actuator_ops = {
#ifdef USE_AF_SLEEP_MODE
	.set_active = sensor_ak737x_actuator_set_active,
#endif
};

int sensor_ak737x_actuator_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret = 0;
	struct fimc_is_core *core = NULL;
	struct v4l2_subdev *subdev_actuator = NULL;
	struct fimc_is_actuator *actuator = NULL;
	struct fimc_is_device_sensor *device = NULL;
	u32 sensor_id = 0;
	u32 first_pos = 0;
	u32 first_delay = 0;
	bool vendor_use_sleep_mode = false;
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

	if (of_property_read_bool(dnode, "vendor_use_sleep_mode"))
		vendor_use_sleep_mode = true;

	ret = of_property_read_u32(dnode, "vendor_first_pos", &first_pos);
	if (ret) {
		first_pos = AK737X_DEFAULT_FIRST_POSITION;
		err("vendor_first_pos read is fail(%d)", ret);
	}

	ret = of_property_read_u32(dnode, "vendor_first_delay", &first_delay);
	if (ret) {
		first_delay = AK737X_DEFAULT_FIRST_DELAY;
		err("vendor_first_delay read is fail(%d)", ret);
	}

	ret = of_property_read_u32(dnode, "id", &sensor_id);
	if (ret)
		err("id read is fail(%d)", ret);

	probe_info("%s sensor_id(%d)\n", __func__, sensor_id);

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

	actuator->id = ACTUATOR_NAME_AK737X;
	actuator->subdev = subdev_actuator;
	actuator->device = sensor_id;
	actuator->client = client;
	actuator->position = 0;
	actuator->max_position = AK737X_POS_MAX_SIZE;
	actuator->pos_size_bit = AK737X_POS_SIZE_BIT;
	actuator->pos_direction = AK737X_POS_DIRECTION;
	actuator->i2c_lock = NULL;
	actuator->need_softlanding = 0;
	actuator->actuator_ops = &actuator_ops;

	actuator->vendor_product_id = AK737X_PRODUCT_ID_AK7371; // AK737X - initial product_id : AK7371
	actuator->vendor_first_pos = first_pos;
	actuator->vendor_first_delay = first_delay;
	actuator->vendor_use_sleep_mode = vendor_use_sleep_mode;

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

static int sensor_ak737x_actuator_remove(struct i2c_client *client)
{
	int ret = 0;

	return ret;
}

static const struct of_device_id exynos_fimc_is_ak737x_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-actuator-ak737x",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fimc_is_ak737x_match);

static const struct i2c_device_id actuator_ak737x_idt[] = {
	{ ACTUATOR_NAME, 0 },
	{},
};

static struct i2c_driver actuator_ak737x_driver = {
	.driver = {
		.name	= ACTUATOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = exynos_fimc_is_ak737x_match
	},
	.probe	= sensor_ak737x_actuator_probe,
	.remove	= sensor_ak737x_actuator_remove,
	.id_table = actuator_ak737x_idt
};
module_i2c_driver(actuator_ak737x_driver);
