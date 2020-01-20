/*
 * Samsung Exynos5 SoC series Actuator driver
 *
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
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

#include "fimc-is-actuator-zc533.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-core.h"
#include "fimc-is-helper-actuator-i2c.h"

#include "interface/fimc-is-interface-library.h"
//#include "fimc-is-api-common.h"

#ifdef CONFIG_USE_VENDER_FEATURE
#include "fimc-is-sec-define.h"
#endif

#define ACTUATOR_NAME		"ZC533"

#define REG_POWER_DOWN	0x02 // Default: 0x00, W, [0] = PD
#define REG_POSITION_MSB	0x03 // Default: 0x00, R/W, [1:0] = DIN[9:8]
#define REG_POSITION_LSB	0x04 // Default: 0x00, R/W, [7:0] = DIN[7:0]
#define REG_DRIVE_MODE		0x05 // Default: 0x00, R/W, [2:0] = DM[2:0]
#define REG_DSC_GAIN		0x06 // Default: 0x00, R/W, [7:0] = VDP[7:0]
#define REG_AIP_DATA		0x07 // Default: 0x00, R/W, [7:0] = AIPD[7:0]
#define REG_AIP_TIME		0x08 // Default: 0x00, R/W, [1:0] = AIPT[1:0]

#define DEF_ZC533_FIRST_POSITION		320
#define DEF_ZC533_SECOND_POSITION		420
#define DEF_ZC533_FIRST_DELAY_MS		15
#define DEF_ZC533_SECOND_DELAY_MS	0

extern struct fimc_is_lib_support gPtr_lib_support;
extern struct fimc_is_sysfs_actuator sysfs_actuator;

int sensor_zc533_init(struct i2c_client *client, struct fimc_is_caldata_list_zc533 *cal_data)
{
	int ret = 0;
	u8 i2c_data[2];

	if (!cal_data) {
		/* Normal Operation Mode
		* 0 : Normal Operation
		* 1 : Power Down
		*/
		i2c_data[0] = REG_POWER_DOWN;
		i2c_data[1] = 0x00;
		ret = fimc_is_sensor_addr8_write8(client, i2c_data[0], i2c_data[1]);
		if (ret < 0)
			goto p_err;

		/*
		 * Drive mode setting
		 * 0x00: Direct Mode, 0x02: DSC2, 0x04: DSC4, 0x05: DSC5, 0x06: DSC6
		 */
		i2c_data[0] = REG_DRIVE_MODE;
		i2c_data[1] = 0x06; /* DSC6 */
		ret = fimc_is_sensor_addr8_write8(client, i2c_data[0], i2c_data[1]);
		if (ret < 0)
			goto p_err;

		/*
		 * DSC Gain
		 */
		i2c_data[0] = REG_DSC_GAIN;
		i2c_data[1] = 0x64; /* 100 x 0.1 = 10 */
		ret = fimc_is_sensor_addr8_write8(client, i2c_data[0], i2c_data[1]);
		if (ret < 0)
			goto p_err;

		udelay(50);

	} else {
		/* Normal Operation Mode
		* 0 : Normal Operation
		* 1 : Power Down
		*/
		i2c_data[0] = REG_POWER_DOWN;
		i2c_data[1] = 0x00;
		ret = fimc_is_sensor_addr8_write8(client, i2c_data[0], i2c_data[1]);
		if (ret < 0)
			goto p_err;

		/*
		 * Drive mode setting
		 * 0x00: Direct Mode, 0x02: DSC2, 0x04: DSC4, 0x05: DSC5, 0x06: DSC6
		 */
		i2c_data[0] = REG_DRIVE_MODE;
		i2c_data[1] = cal_data->operating_mode;
		ret = fimc_is_sensor_addr8_write8(client, i2c_data[0], i2c_data[1]);
		if (ret < 0)
			goto p_err;

		/*
		 * DSC Gain
		 */
		i2c_data[0] = REG_DSC_GAIN;
		i2c_data[1] = cal_data->gain;
		ret = fimc_is_sensor_addr8_write8(client, i2c_data[0], i2c_data[1]);
		if (ret < 0)
			goto p_err;

		udelay(50);
	}

p_err:
	return ret;
}

static int sensor_zc533_write_position(struct i2c_client *client, u32 val)
{
	int ret = 0;
	u8 val_high = 0, val_low = 0;

	FIMC_BUG(!client);

	if (!client->adapter) {
		err("Could not find adapter!\n");
		ret = -ENODEV;
		goto p_err;
	}

	if (val > ZC533_POS_MAX_SIZE) {
		err("Invalid af position(position : %d, Max : %d).\n",
					val, ZC533_POS_MAX_SIZE);
		ret = -EINVAL;
		goto p_err;
	}

	/*
	 * val_high is position DIN[9:8],
	 * val_low is position DIN[7:0]
	 */
	val_high = (val & 0x300) >> 8;
	val_low = (val & 0x00FF);

	ret = fimc_is_sensor_addr_data_write16(client, REG_POSITION_MSB, val_high, val_low);
	if (ret < 0)
		goto p_err;

	return ret;

p_err:
	ret = -ENODEV;
	return ret;
}

static int sensor_zc533_valid_check(struct i2c_client * client)
{
	int i;

	FIMC_BUG(!client);

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

static void sensor_zc533_print_log(int step)
{
	int i;

	if (step > 0) {
		dbg_actuator("initial position ");
		for (i = 0; i < step; i++) {
			dbg_actuator("position(%d), delay(%d)\n",
					sysfs_actuator.init_positions[i], sysfs_actuator.init_delays[i]);
		}
		dbg_actuator(" setting");
	}
}

static int sensor_zc533_init_position(struct i2c_client *client,
		struct fimc_is_actuator *actuator)
{
	int i;
	int ret = 0;
	int init_step = 0;

	init_step = sensor_zc533_valid_check(client);

	if (init_step > 0) {
		for (i = 0; i < init_step; i++) {
			ret = sensor_zc533_write_position(client, sysfs_actuator.init_positions[i]);
			if (ret < 0)
				goto p_err;

			mdelay(sysfs_actuator.init_delays[i]);
		}

		actuator->position = sysfs_actuator.init_positions[i];

		sensor_zc533_print_log(init_step);

	} else {
		ret = sensor_zc533_write_position(client, DEF_ZC533_FIRST_POSITION);
		if (ret < 0)
			goto p_err;

		mdelay(DEF_ZC533_FIRST_DELAY_MS);

		ret = sensor_zc533_write_position(client, DEF_ZC533_SECOND_POSITION);
		if (ret < 0)
			goto p_err;

		mdelay(DEF_ZC533_SECOND_DELAY_MS);

		actuator->position = DEF_ZC533_SECOND_POSITION;

		dbg_actuator("initial position %d, %d setting\n", DEF_ZC533_FIRST_POSITION, DEF_ZC533_SECOND_POSITION);
		dbg_actuator("initial delays %d, %d setting\n", DEF_ZC533_FIRST_DELAY_MS, DEF_ZC533_SECOND_DELAY_MS);
	}

p_err:
	return ret;
}

int sensor_zc533_actuator_init(struct v4l2_subdev *subdev, u32 val)
{
	int ret = 0;
	struct fimc_is_actuator *actuator;
	struct fimc_is_caldata_list_zc533 *cal_data = NULL;
	struct i2c_client *client = NULL;
	long cal_addr;
#ifdef DEBUG_ACTUATOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);

	dbg_actuator("%s\n", __func__);

	actuator = (struct fimc_is_actuator *)v4l2_get_subdevdata(subdev);
	if (!actuator) {
		err("actuator is not detect!\n");
		goto p_err;
	}

	client = actuator->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	/* EEPROM AF calData address */
	cal_addr = gPtr_lib_support.minfo->kvaddr_cal[SENSOR_POSITION_REAR] + EEPROM_OEM_BASE;

	cal_data = (struct fimc_is_caldata_list_zc533 *)(cal_addr);

	/* Read into EEPROM data or default setting */
	ret = sensor_zc533_init(client, cal_data);
	if (ret < 0)
		goto p_err;

	ret = sensor_zc533_init_position(client, actuator);
	if (ret < 0)
		goto p_err;

#ifdef DEBUG_ACTUATOR_TIME
	do_gettimeofday(&end);
	pr_info("[%s] time %lu us", __func__, (end.tv_sec - st.tv_sec) * 1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_zc533_actuator_get_status(struct v4l2_subdev *subdev, u32 *info)
{
	int ret = 0;
	struct fimc_is_actuator *actuator = NULL;
	struct i2c_client *client = NULL;
#ifdef DEBUG_ACTUATOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	dbg_actuator("%s\n", __func__);

	FIMC_BUG(!subdev);
	FIMC_BUG(!info);

	actuator = (struct fimc_is_actuator *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!actuator);

	client = actuator->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	/* If you need check the position value, use this */
#if 0
	/* Read REG_POSITION_MSB(0x03) DIN[9:8] and REG_POSITION_LSB(0x04) DIN[7:0] */
	ret = fimc_is_sensor_addr8_read8(client, REG_POSITION_MSB, &val);
	data = (val & 0x0300);
	ret = fimc_is_sensor_addr8_read8(client, REG_POSITION_LSB, &val);
	data |= (val & 0x00ff);
#endif

#if 0
	ret = fimc_is_sensor_addr8_read8(client, REG_STATUS, &val);

	*info = ((val & 0x1) == 0) ? ACTUATOR_STATUS_NO_BUSY : ACTUATOR_STATUS_BUSY;
#else
	*info = ACTUATOR_STATUS_NO_BUSY;
#endif
#ifdef DEBUG_ACTUATOR_TIME
	do_gettimeofday(&end);
	pr_info("[%s] time %lu us", __func__, (end.tv_sec - st.tv_sec) * 1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_zc533_actuator_set_position(struct v4l2_subdev *subdev, u32 *info)
{
	int ret = 0;
	struct fimc_is_actuator *actuator;
	struct i2c_client *client;
	u32 position = 0;
#ifdef DEBUG_ACTUATOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);
	FIMC_BUG(!info);

	actuator = (struct fimc_is_actuator *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!actuator);

	client = actuator->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	position = *info;
	if (position > ZC533_POS_MAX_SIZE) {
		err("Invalid af position(position : %d, Max : %d).\n",
					position, ZC533_POS_MAX_SIZE);
		ret = -EINVAL;
		goto p_err;
	}

	/* position Set */
	ret = sensor_zc533_write_position(client, position);
	if (ret <0)
		goto p_err;
	actuator->position = position;

	dbg_actuator("%s: position(%d)\n", __func__, position);

#ifdef DEBUG_ACTUATOR_TIME
	do_gettimeofday(&end);
	pr_info("[%s] time %lu us", __func__, (end.tv_sec - st.tv_sec) * 1000000 + (end.tv_usec - st.tv_usec));
#endif
p_err:
	return ret;
}

static int sensor_zc533_actuator_g_ctrl(struct v4l2_subdev *subdev, struct v4l2_control *ctrl)
{
	int ret = 0;
	u32 val = 0;

	switch(ctrl->id) {
	case V4L2_CID_ACTUATOR_GET_STATUS:
		ret = sensor_zc533_actuator_get_status(subdev, &val);
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

static int sensor_zc533_actuator_s_ctrl(struct v4l2_subdev *subdev, struct v4l2_control *ctrl)
{
	int ret = 0;

	switch(ctrl->id) {
	case V4L2_CID_ACTUATOR_SET_POSITION:
		ret = sensor_zc533_actuator_set_position(subdev, &ctrl->value);
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

static const struct v4l2_subdev_core_ops core_ops = {
	.init = sensor_zc533_actuator_init,
	.g_ctrl = sensor_zc533_actuator_g_ctrl,
	.s_ctrl = sensor_zc533_actuator_s_ctrl,
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
};

static int sensor_zc533_actuator_probe(struct i2c_client *client,
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

	FIMC_BUG(!fimc_is_dev);
	FIMC_BUG(!client);

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
	if (!test_bit(FIMC_IS_SENSOR_PROBE, &device->state)) {
		err("sensor device is not yet probed");
		ret = -EPROBE_DEFER;
		goto p_err;
	}

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
		goto p_err;
	}

	/* This name must is match to sensor_open_extended actuator name */
	actuator->id = ACTUATOR_NAME_ZC533;
	actuator->subdev = subdev_actuator;
	actuator->device = sensor_id;
	actuator->client = client;
	actuator->position = 0;
	actuator->max_position = ZC533_POS_MAX_SIZE;
	actuator->pos_size_bit = ZC533_POS_SIZE_BIT;
	actuator->pos_direction = ZC533_POS_DIRECTION;
	actuator->i2c_lock = NULL;
	actuator->need_softlanding = 0;
	actuator->actuator_ops = NULL;

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

static const struct of_device_id sensor_actuator_zc533_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-actuator-zc533",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sensor_actuator_zc533_match);

static const struct i2c_device_id sensor_actuator_zc533_idt[] = {
	{ ACTUATOR_NAME, 0 },
	{},
};

static struct i2c_driver sensor_actuator_zc533_driver = {
	.probe  = sensor_zc533_actuator_probe,
	.driver = {
		.name	= ACTUATOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sensor_actuator_zc533_match,
		.suppress_bind_attrs = true,
	},
	.id_table = sensor_actuator_zc533_idt,
};

static int __init sensor_actuator_zc533_init(void)
{
	int ret;

	ret = i2c_add_driver(&sensor_actuator_zc533_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			sensor_actuator_zc533_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_actuator_zc533_init);
