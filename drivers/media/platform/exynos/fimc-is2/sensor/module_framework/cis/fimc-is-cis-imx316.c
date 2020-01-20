/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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
#include "fimc-is-param.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-resourcemgr.h"
#include "fimc-is-dt.h"
#include "fimc-is-cis-imx316.h"
#include "fimc-is-cis-imx316-setA.h"
#include "fimc-is-vender-specific.h"
#include "fimc-is-helper-i2c.h"

#define SENSOR_NAME "IMX316"
/* #define DEBUG_imx316_PLL */

#define SENSOR_EXTAREA_INTG_0_A_0 0x2110
#define SENSOR_EXTAREA_INTG_0_A_1 0x2111
#define SENSOR_EXTAREA_INTG_0_A_2 0x2112
#define SENSOR_EXTAREA_INTG_0_A_3 0x2113

#define SENSOR_EXTAREA_INTG_0_B_0 0x2114
#define SENSOR_EXTAREA_INTG_0_B_1 0x2115
#define SENSOR_EXTAREA_INTG_0_B_2 0x2116
#define SENSOR_EXTAREA_INTG_0_B_3 0x2117

#define SENSOR_HMAX_UPPER 0x0800
#define SENSOR_HMAX_LOWER 0x0801

static const struct v4l2_subdev_ops subdev_ops;

static const u32 *sensor_imx316_global;
static u32 sensor_imx316_global_size;
static const u32 **sensor_imx316_setfiles;
static const u32 *sensor_imx316_uid;
static u32 sensor_imx316_max_uid_num;
static const u32 *sensor_imx316_setfile_sizes;
static u32 sensor_imx316_max_setfile_num;

u16 sensor_imx316_HMAX;

void sensor_imx316_set_integration_max_margin(u32 mode, cis_shared_data *cis_data)
{
}

void sensor_imx316_cis_data_calculation(const struct sensor_pll_info_compact *pll_info, cis_shared_data *cis_data)
{
}

void sensor_imx316_cis_data_calc(struct v4l2_subdev *subdev, u32 mode)
{
}

int sensor_imx316_wait_stream_off_status(cis_shared_data *cis_data)
{
	int ret = 0;
	u32 timeout = 0;

	FIMC_BUG(!cis_data);

#define STREAM_OFF_WAIT_TIME 250
	while (timeout < STREAM_OFF_WAIT_TIME) {
		if (cis_data->is_active_area == false &&
				cis_data->stream_on == false) {
			pr_debug("actual stream off\n");
			break;
		}
		timeout++;
	}

	if (timeout == STREAM_OFF_WAIT_TIME) {
		pr_err("actual stream off wait timeout\n");
		ret = -1;
	}
	return ret;
}

int sensor_imx316_cis_check_rev(struct fimc_is_cis *cis)
{
	int ret = 0;
	u8 rev = 0;
	struct i2c_client *client;

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		return ret;
	}

	I2C_MUTEX_LOCK(cis->i2c_lock);
#if 0
	/* Specify OPT Page Address for READ */
	fimc_is_sensor_write8(client, 0x0A02,  0x7F);

	/* Turn ON OTP Read MODE */
	fimc_is_sensor_write8(client, 0x0A00,  0x01);

	/* Check status */
	fimc_is_sensor_read8(client, 0x0A01,  &rev);
#endif
	info("sensor_imx316_cis_check_rev start ");
	ret = fimc_is_sensor_read8(client, 0x0000, &rev);
	info("sensor_imx316_cis_check_rev 0x0000 %x ", rev);
	ret = fimc_is_sensor_read8(client, 0x0001, &rev);
	info("sensor_imx316_cis_check_rev 0x0001 %x ", rev);
	ret = fimc_is_sensor_read8(client, 0x0002, &rev);
	info("sensor_imx316_cis_check_rev 0x0002 %x ", rev);
	ret = fimc_is_sensor_read8(client, 0x0003, &rev);
	info("sensor_imx316_cis_check_rev 0x0003 %x ", rev);
	ret = fimc_is_sensor_read8(client, 0x0019,&rev);
	info("sensor_imx316_cis_check_rev 0x0019 %x ", rev);
	if (ret < 0) {
		err("fimc_is_sensor_read8 fail, (ret %d)", ret);
		I2C_MUTEX_UNLOCK(cis->i2c_lock);
		return ret;
	}
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	cis->cis_data->cis_rev = rev;
	return 0;
}

/* CIS OPS */
int sensor_imx316_cis_init(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	cis_setting_info setinfo;
#ifdef USE_CAMERA_HW_BIG_DATA
	struct cam_hw_param *hw_param = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
#endif

	setinfo.param = NULL;
	setinfo.return_value = 0;

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	if (!cis) {
		err("cis is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	FIMC_BUG(!cis->cis_data);
	memset(cis->cis_data, 0, sizeof(cis_shared_data));
	cis->rev_flag = false;

	ret = sensor_imx316_cis_check_rev(cis);
	if (ret < 0) {
#ifdef USE_CAMERA_HW_BIG_DATA
		sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
		if (sensor_peri)
			fimc_is_sec_get_hw_param(&hw_param, sensor_peri->module->position);
		if (hw_param)
			hw_param->i2c_sensor_err_cnt++;
#endif
		warn("sensor_imx316_check_rev is fail when cis init");
		cis->rev_flag = true;
		ret = 0;
	}

	cis->cis_data->product_name = cis->id;
	cis->cis_data->cur_width = SENSOR_IMX316_MAX_WIDTH;
	cis->cis_data->cur_height = SENSOR_IMX316_MAX_HEIGHT;
	cis->cis_data->low_expo_start = 33000;
	cis->need_mode_change = false;

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_imx316_cis_log_status(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client = NULL;
	u8 data8 = 0;
	u16 data16 = 0;

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	if (!cis) {
		err("cis is NULL");
		ret = -ENODEV;
		goto p_err;
	}

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -ENODEV;
		goto p_err;
	}

	I2C_MUTEX_LOCK(cis->i2c_lock);
	pr_err("[SEN:DUMP] IMX316 *************************\n");
	fimc_is_sensor_read16(client, 0x0001, &data16);
	pr_err("[SEN:DUMP] manufacturer ID(%x)\n", data16);
	fimc_is_sensor_read8(client, 0x152C, &data8);
	pr_err("[SEN:DUMP] frame counter (%x)\n", data8);
	fimc_is_sensor_read8(client, 0x1001, &data8);
	pr_err("[SEN:DUMP] mode_select(%x)\n", data8);

#if 0
	pr_err("[SEN:DUMP] global \n");
	sensor_cis_dump_registers(subdev, sensor_imx316_global, sensor_imx316_global_size);
	pr_err("[SEN:DUMP] mode \n");
	sensor_cis_dump_registers(subdev, sensor_imx316_setfiles[0], sensor_imx316_setfile_sizes[0]);
#endif
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	pr_err("[SEN:DUMP] *******************************\n");

p_err:
	return ret;
}

/* Input
 *	hold : true - hold, flase - no hold
 * Output
 *      return: 0 - no effect(already hold or no hold)
 *		positive - setted by request
 *		negative - ERROR value
 */
int sensor_imx316_cis_group_param_hold(struct v4l2_subdev *subdev, bool hold)
{
	int ret = 0;
	return ret;
}

int sensor_imx316_cis_set_global_setting(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct sensor_open_extended *ext_info;

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);

	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
	module = sensor_peri->module;
	ext_info = &module->ext;

	I2C_MUTEX_LOCK(cis->i2c_lock);
	/* setfile global setting is at camera entrance */
	info("sensor_imx316_cis_set_global_setting");
	ret = sensor_cis_set_registers(subdev, sensor_imx316_global, sensor_imx316_global_size);
	if (ret < 0) {
		err("sensor_imx316_set_registers fail!!");
		goto p_err;
	}

	dbg_sensor(1, "[%s] global setting done\n", __func__);

p_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);
	return ret;
}

int sensor_imx316_get_HMAX_value(const u32 *regs, const u32 size)
{
	int i = 0;

	FIMC_BUG(!regs);

	sensor_imx316_HMAX = 0;
	for (i = 0; i < size; i += I2C_NEXT) {
		if (regs[i + I2C_ADDR] == SENSOR_HMAX_UPPER) {
			sensor_imx316_HMAX |= (regs[i + I2C_DATA] << 8);
		} else if (regs[i + I2C_ADDR] == SENSOR_HMAX_LOWER) {
			sensor_imx316_HMAX |= regs[i + I2C_DATA];
			return 0;
		}
	}
	return 0;
}

int sensor_imx316_cis_mode_change(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct fimc_is_cis *cis = NULL;
	struct fimc_is_vender_specific *specific = NULL;
	//int index = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct fimc_is_module_enum *module;
	struct sensor_open_extended *ext_info;
	struct exynos_platform_fimc_is_module *pdata;
	u32 sensor_uid = 0;
	int i;

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (mode > sensor_imx316_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		ret = -EINVAL;
		goto p_err;
	}

	/* If check_rev fail when cis_init, one more check_rev in mode_change */
	if (cis->rev_flag == true) {
		cis->rev_flag = false;
		ret = sensor_cis_check_rev(cis);
		if (ret < 0) {
			err("sensor_imx316_check_rev is fail");
			goto p_err;
		}
	}

	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
	module = sensor_peri->module;
	ext_info = &module->ext;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		probe_info("core device is not yet probed");
		ret = -EINVAL;
		goto p_err;
	}

	specific = core->vender.private_data;
	if (sensor_peri->module->position == SENSOR_POSITION_REAR_TOF) {
		sensor_uid = specific->rear_tof_uid;
	} else {  // SENSOR_POSITION_FRONT_TOF
		sensor_uid = specific->front_tof_uid;
	}

	for (i = 0; i < sensor_imx316_max_uid_num; i++) {
		if (sensor_uid == sensor_imx316_uid[i]) {
			mode = i;
			break;
		}
	}

	if (i == sensor_imx316_max_uid_num) {
		err("sensor_imx316_cis_mode_change can't find uid set");
		ret = -EINVAL;
		goto p_err;
	}

	info("sensor_imx316_cis_mode_change pos %d sensor_uid %x mode %d 0x%x",
		sensor_peri->module->position, sensor_uid, mode, sensor_imx316_uid[mode]);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	ret = sensor_cis_set_registers(subdev, sensor_imx316_setfiles[mode], sensor_imx316_setfile_sizes[mode]);
	if (ret < 0) {
		err("sensor_imx316_set_registers fail!!");
		goto p_err;
	}

	/* VCSEL ON  */
	pdata = module->pdata;
	if (!pdata) {
		clear_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
		err("pdata is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (!pdata->gpio_cfg) {
		clear_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
		err("gpio_cfg is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = pdata->gpio_cfg(module, SENSOR_SCENARIO_ADDITIONAL_POWER, GPIO_SCENARIO_ON);
	if (ret) {
		clear_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
		err("gpio_cfg is fail(%d)", ret);
		goto p_err;
	}

	sensor_imx316_get_HMAX_value(sensor_imx316_setfiles[mode], sensor_imx316_setfile_sizes[mode]);
p_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);
	return ret;
}

/* TODO: Sensor set size sequence(sensor done, sensor stop, 3AA done in FW case */
int sensor_imx316_cis_set_size(struct v4l2_subdev *subdev, cis_shared_data *cis_data)
{
	int ret = 0;
	return ret;
}

int sensor_imx316_cis_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
	FIMC_BUG(!sensor_peri);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	if (ret < 0)
		err("group_param_hold_func failed at stream on");

	/* Sensor stream on */
	info("sensor_imx316_cis_stream_on");
	fimc_is_sensor_write8(client, 0x1001, 0x01);
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	cis_data->stream_on = true;

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_imx316_cis_stream_off(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;
	do_gettimeofday(&st);
#endif

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	/* Sensor stream off */
	info("sensor_imx316_cis_stream_off");
	fimc_is_sensor_write8(client, 0x1001, 0x00);
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	cis_data->stream_on = false;

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_imx316_cis_set_exposure_time(struct v4l2_subdev *subdev, struct ae_param *target_exposure)
{
	#define AE_MAX 0x1D4C0
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	u8 value;
	u32 value_32;

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}


	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
	module = sensor_peri->module;

	value_32 = target_exposure->short_val * 120;
	if (value_32 > AE_MAX) {
		value_32 = AE_MAX;
	} else if (value_32 < sensor_imx316_HMAX) {
		value_32 = sensor_imx316_HMAX;
	}

	info("sensor_imx316_cis_set_exposure_time org %x value %x",
		target_exposure->short_val * 120, value_32);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	if (sensor_peri->module->position == SENSOR_POSITION_REAR_TOF) { /* dual mode - rear tof */
		value = value_32 & 0xFF;
		fimc_is_sensor_write8(client, SENSOR_EXTAREA_INTG_0_A_3, value);
		fimc_is_sensor_write8(client, SENSOR_EXTAREA_INTG_0_B_3, value);
		value = (value_32 & 0xFF00) >> 8;
		fimc_is_sensor_write8(client, SENSOR_EXTAREA_INTG_0_A_2, value);
		fimc_is_sensor_write8(client, SENSOR_EXTAREA_INTG_0_B_2, value);
		value = (value_32 & 0xFF0000) >> 16;
		fimc_is_sensor_write8(client, SENSOR_EXTAREA_INTG_0_A_1, value);
		fimc_is_sensor_write8(client, SENSOR_EXTAREA_INTG_0_B_1, value);
		value = (value_32 & 0xFF000000) >> 24;
		fimc_is_sensor_write8(client, SENSOR_EXTAREA_INTG_0_A_0, value);
		fimc_is_sensor_write8(client, SENSOR_EXTAREA_INTG_0_B_0, value);
	} else { /* single mode - front tof */
		value = value_32 & 0xFF;
		fimc_is_sensor_write8(client, SENSOR_EXTAREA_INTG_0_A_3, value);
		value = (value_32 & 0xFF00) >> 8;
		fimc_is_sensor_write8(client, SENSOR_EXTAREA_INTG_0_A_2, value);
		value = (value_32 & 0xFF0000) >> 16;
		fimc_is_sensor_write8(client, SENSOR_EXTAREA_INTG_0_A_1, value);
		value = (value_32 & 0xFF000000) >> 24;
		fimc_is_sensor_write8(client, SENSOR_EXTAREA_INTG_0_A_0, value);
	}

p_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);
	return ret;
}

int sensor_imx316_cis_get_min_exposure_time(struct v4l2_subdev *subdev, u32 *min_expo)
{
	int ret = 0;
	return ret;
}

int sensor_imx316_cis_get_max_exposure_time(struct v4l2_subdev *subdev, u32 *max_expo)
{
	int ret = 0;
	return ret;
}

int sensor_imx316_cis_adjust_frame_duration(struct v4l2_subdev *subdev,
						u32 input_exposure_time,
						u32 *target_duration)
{
	int ret = 0;
	return ret;
}

int sensor_imx316_cis_set_frame_duration(struct v4l2_subdev *subdev, u32 frame_duration)
{
	int ret = 0;
	return ret;
}

int sensor_imx316_cis_set_frame_rate(struct v4l2_subdev *subdev, u32 min_fps)
{
	int ret = 0;
	return ret;
}

u32 sensor_imx316_cis_calc_again_code(u32 permile)
{
	return 1024 - (1024000 / permile);
}

u32 sensor_imx316_cis_calc_again_permile(u32 code)
{
	return 1024000 / (1024 - code);
}

int sensor_imx316_cis_adjust_analog_gain(struct v4l2_subdev *subdev, u32 input_again, u32 *target_permile)
{
	int ret = 0;
	return ret;
}

int sensor_imx316_cis_set_analog_gain(struct v4l2_subdev *subdev, struct ae_param *again)
{
	int ret = 0;
	return ret;
}

int sensor_imx316_cis_get_analog_gain(struct v4l2_subdev *subdev, u32 *again)
{
	int ret = 0;
	return ret;
}

int sensor_imx316_cis_get_min_analog_gain(struct v4l2_subdev *subdev, u32 *min_again)
{
	int ret = 0;
	return ret;
}

int sensor_imx316_cis_get_max_analog_gain(struct v4l2_subdev *subdev, u32 *max_again)
{
	int ret = 0;
	return ret;
}

int sensor_imx316_cis_set_digital_gain(struct v4l2_subdev *subdev, struct ae_param *dgain)
{
	int ret = 0;
	return ret;
}

int sensor_imx316_cis_get_digital_gain(struct v4l2_subdev *subdev, u32 *dgain)
{
	int ret = 0;
	return ret;
}

int sensor_imx316_cis_get_min_digital_gain(struct v4l2_subdev *subdev, u32 *min_dgain)
{
	int ret = 0;
	return ret;
}

int sensor_imx316_cis_get_max_digital_gain(struct v4l2_subdev *subdev, u32 *max_dgain)
{
	int ret = 0;
	return ret;
}

static struct fimc_is_cis_ops cis_ops_imx316 = {
	.cis_init = sensor_imx316_cis_init,
	.cis_log_status = sensor_imx316_cis_log_status,
	.cis_group_param_hold = sensor_imx316_cis_group_param_hold,
	.cis_set_global_setting = sensor_imx316_cis_set_global_setting,
	.cis_mode_change = sensor_imx316_cis_mode_change,
	.cis_set_size = sensor_imx316_cis_set_size,
	.cis_stream_on = sensor_imx316_cis_stream_on,
	.cis_stream_off = sensor_imx316_cis_stream_off,
	.cis_set_exposure_time = sensor_imx316_cis_set_exposure_time,
	.cis_get_min_exposure_time = sensor_imx316_cis_get_min_exposure_time,
	.cis_get_max_exposure_time = sensor_imx316_cis_get_max_exposure_time,
	.cis_adjust_frame_duration = sensor_imx316_cis_adjust_frame_duration,
	.cis_set_frame_duration = sensor_imx316_cis_set_frame_duration,
	.cis_set_frame_rate = sensor_imx316_cis_set_frame_rate,
	.cis_adjust_analog_gain = sensor_imx316_cis_adjust_analog_gain,
	.cis_set_analog_gain = sensor_imx316_cis_set_analog_gain,
	.cis_get_analog_gain = sensor_imx316_cis_get_analog_gain,
	.cis_get_min_analog_gain = sensor_imx316_cis_get_min_analog_gain,
	.cis_get_max_analog_gain = sensor_imx316_cis_get_max_analog_gain,
	.cis_set_digital_gain = sensor_imx316_cis_set_digital_gain,
	.cis_get_digital_gain = sensor_imx316_cis_get_digital_gain,
	.cis_get_min_digital_gain = sensor_imx316_cis_get_min_digital_gain,
	.cis_get_max_digital_gain = sensor_imx316_cis_get_max_digital_gain,
	.cis_compensate_gain_for_extremely_br = sensor_cis_compensate_gain_for_extremely_br,
	.cis_wait_streamoff = sensor_cis_wait_streamoff,
	.cis_wait_streamon = sensor_cis_wait_streamon,
	.cis_data_calculation = sensor_imx316_cis_data_calc,
};

static int cis_imx316_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
	struct fimc_is_core *core = NULL;
	struct v4l2_subdev *subdev_cis = NULL;
	struct fimc_is_cis *cis = NULL;
	struct fimc_is_device_sensor *device = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	u32 sensor_id;
	char const *setfile;
	struct device *dev;
	struct device_node *dnode;

	FIMC_BUG(!client);
	FIMC_BUG(!fimc_is_dev);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		probe_info("core device is not yet probed");
		return -EPROBE_DEFER;
	}

	dev = &client->dev;
	dnode = dev->of_node;

	ret = of_property_read_u32(dnode, "id", &sensor_id);
	if (ret) {
		err("sensor id read is fail(%d)", ret);
		goto p_err;
	}

	probe_info("%s sensor id %d\n", __func__, sensor_id);

	device = &core->sensor[sensor_id];

	sensor_peri = find_peri_by_cis_id(device, SENSOR_NAME_IMX316);
	if (!sensor_peri) {
		probe_info("sensor peri is net yet probed");
		return -EPROBE_DEFER;
	}

	cis = &sensor_peri->cis;
	if (!cis) {
		err("cis is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	subdev_cis = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_cis) {
		probe_err("subdev_cis is NULL");
		ret = -ENOMEM;
		goto p_err;
	}
	sensor_peri->subdev_cis = subdev_cis;

	cis->id = SENSOR_NAME_IMX316;
	cis->subdev = subdev_cis;
	cis->device = 0;
	cis->client = client;
	sensor_peri->module->client = cis->client;
	cis->ctrl_delay = N_PLUS_TWO_FRAME;
#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
	cis->mipi_clock_index_cur = CAM_MIPI_NOT_INITIALIZED;
	cis->mipi_clock_index_new = CAM_MIPI_NOT_INITIALIZED;
#endif
	cis->cis_data = kzalloc(sizeof(cis_shared_data), GFP_KERNEL);
	if (!cis->cis_data) {
		err("cis_data is NULL");
		ret = -ENOMEM;
		goto p_err;
	}
	cis->cis_ops = &cis_ops_imx316;

	if (of_property_read_string(dnode, "setfile", &setfile)) {
		err("setfile index read fail(%d), take default setfile!!", ret);
		setfile = "default";
	}

	if (strcmp(setfile, "default") == 0 || strcmp(setfile, "setA") == 0) {
		probe_info("%s setfile_A\n", __func__);
		sensor_imx316_global = sensor_imx316_setfile_A_Global;
		sensor_imx316_global_size = ARRAY_SIZE(sensor_imx316_setfile_A_Global);
		sensor_imx316_setfiles = sensor_imx316_setfiles_A;
		sensor_imx316_uid = sensor_imx316_setfiles_uid;
		sensor_imx316_max_uid_num = ARRAY_SIZE(sensor_imx316_setfiles_uid);
		sensor_imx316_setfile_sizes = sensor_imx316_setfile_A_sizes;
		sensor_imx316_max_setfile_num = ARRAY_SIZE(sensor_imx316_setfiles_A);
	}
#if 0
	else if (strcmp(setfile, "setB") == 0) {
		probe_info("%s setfile_B\n", __func__);
		sensor_imx316_global = sensor_imx316_setfile_B_Global;
		sensor_imx316_global_size = ARRAY_SIZE(sensor_imx316_setfile_B_Global);
		sensor_imx316_setfiles = sensor_imx316_setfiles_B;
		sensor_imx316_setfile_sizes = sensor_imx316_setfile_B_sizes;
		sensor_imx316_max_setfile_num = ARRAY_SIZE(sensor_imx316_setfiles_B);
	}
#endif
	else {
		err("%s setfile index out of bound, take default (setfile_A)", __func__);
		sensor_imx316_global = sensor_imx316_setfile_A_Global;
		sensor_imx316_global_size = ARRAY_SIZE(sensor_imx316_setfile_A_Global);
		sensor_imx316_setfiles = sensor_imx316_setfiles_A;
		sensor_imx316_uid = sensor_imx316_setfiles_uid;
		sensor_imx316_max_uid_num = ARRAY_SIZE(sensor_imx316_setfiles_uid);
		sensor_imx316_setfile_sizes = sensor_imx316_setfile_A_sizes;
		sensor_imx316_max_setfile_num = ARRAY_SIZE(sensor_imx316_setfiles_A);
	}

	/* belows are depend on sensor cis. MUST check sensor spec */
	cis->bayer_order = OTF_INPUT_ORDER_BAYER_GR_BG;

	if (of_property_read_bool(dnode, "sensor_f_number")) {
		ret = of_property_read_u32(dnode, "sensor_f_number", &cis->aperture_num);
		if (ret) {
			warn("f-number read is fail(%d)",ret);
		}
	} else {
		cis->aperture_num = F2_4;
	}

	probe_info("%s f-number %d\n", __func__, cis->aperture_num);

	cis->use_dgain = true;
	cis->hdr_ctrl_by_again = false;

	ret = of_property_read_string(dnode, "setfile", &setfile);
	if (ret) {
		err("setfile index read fail(%d), take default setfile!!", ret);
		setfile = "default";
	}

	cis->use_initial_ae = of_property_read_bool(dnode, "use_initial_ae");
	probe_info("%s use initial_ae(%d)\n", __func__, cis->use_initial_ae);

	v4l2_i2c_subdev_init(subdev_cis, client, &subdev_ops);
	v4l2_set_subdevdata(subdev_cis, cis);
	v4l2_set_subdev_hostdata(subdev_cis, device);
	snprintf(subdev_cis->name, V4L2_SUBDEV_NAME_SIZE, "cis-subdev.%d", cis->id);

	probe_info("%s done\n", __func__);

p_err:
	return ret;
}

static const struct of_device_id sensor_cis_imx316_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-cis-imx316",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sensor_cis_imx316_match);

static const struct i2c_device_id sensor_cis_imx316_idt[] = {
	{ SENSOR_NAME, 0 },
	{},
};

static struct i2c_driver sensor_cis_imx316_driver = {
	.probe	= cis_imx316_probe,
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sensor_cis_imx316_match,
		.suppress_bind_attrs = true,
	},
	.id_table = sensor_cis_imx316_idt
};

static int __init sensor_cis_imx316_init(void)
{
	int ret;

	probe_info("sensor_cis_imx316_init");
	ret = i2c_add_driver(&sensor_cis_imx316_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			sensor_cis_imx316_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_cis_imx316_init);
