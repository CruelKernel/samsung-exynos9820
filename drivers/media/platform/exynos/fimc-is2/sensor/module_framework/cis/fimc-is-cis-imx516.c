/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd
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
#include "fimc-is-cis-imx516.h"
#include "fimc-is-cis-imx516-setA.h"
#include "fimc-is-vender-specific.h"
#include "fimc-is-helper-i2c.h"

#define SENSOR_NAME "IMX516"

#define SENSOR_EXTAREA_INTG_0_A_0 0x2124
#define SENSOR_EXTAREA_INTG_0_A_1 0x2125
#define SENSOR_EXTAREA_INTG_0_A_2 0x2126
#define SENSOR_EXTAREA_INTG_0_A_3 0x2127

#define SENSOR_EXTAREA_INTG_0_B_0 0x2128
#define SENSOR_EXTAREA_INTG_0_B_1 0x2129
#define SENSOR_EXTAREA_INTG_0_B_2 0x212A
#define SENSOR_EXTAREA_INTG_0_B_3 0x212B

#define SENSOR_LASER_CONTROL_REG_1 0x2144
#define SENSOR_LASER_CONTROL_REG_2 0x2145
#define SENSOR_LASER_OFF  0xAA
#define SENSOR_LASER_ON   0x00

#define SENSOR_HMAX_UPPER 0x0800
#define SENSOR_HMAX_LOWER 0x0801

#define SENSOR_DEPTH_MAP_NUMBER 0x151C
#define SENSOR_FPS_30		33333333
#define SENSOR_FPS_24		41666666
#define SENSOR_FPS_20		50000000
#define SENSOR_FPS_15		66666666
#define SENSOR_FPS_10		100000000
#define SENSOR_FPS_5		200000000
#define SENOSR_FPS_REGISTER_1	0x2112
#define SENOSR_FPS_REGISTER_2	0x2113

static const struct v4l2_subdev_ops subdev_ops;

static const u32 *sensor_imx516_global;
static u32 sensor_imx516_global_size;
static const u32 ***sensor_imx516_setfiles;
static const u32 **sensor_imx516_uid;
static const u32 *sensor_imx516_max_uid_num;
static const u32 **sensor_imx516_setfile_sizes;
static const u16 ***sensor_imx516_setfiles_fps;

int sensor_imx516_mode = -1;
u16 sensor_imx516_HMAX;
u32 sensor_imx516_frame_duration;

#define REAR_TX_DEFAULT_FREQ 100750 /* KHz */

#ifdef USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION
static const struct cam_tof_sensor_mode **sensor_imx516_tx_freq_sensor_mode;
static const u32 *sensor_imx516_tx_freq_sensor_mode_size;
static const int **sensor_imx516_verify_sensor_mode;
static const int *sensor_imx516_verify_sensor_mode_size;
u32 sensor_imx516_rear_tx_freq = REAR_TX_DEFAULT_FREQ;

static int sensor_imx516_cis_set_tx_clock(struct v4l2_subdev *subdev)
{
	struct fimc_is_cis *cis = NULL;
	struct fimc_is_device_sensor *device;
	const struct cam_tof_sensor_mode *cur_tx_sensor_mode;
	int found = -1, i = 0;
	u32 sensor_uid = 0;
	u32 uid_index = 0;
	int ret = 0;

	device = (struct fimc_is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	if (device == NULL) {
		err("device is NULL");
		return -1;
	}

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	if (cis == NULL) {
		err("cis is NULL");
		return -1;
	}

	ret = sensor_imx516_cis_get_uid_index(cis, sensor_imx516_mode, &uid_index);
	if (ret < 0) {
		return -1;
	}

	cur_tx_sensor_mode = &sensor_imx516_tx_freq_sensor_mode[sensor_imx516_mode][uid_index];

	if (cur_tx_sensor_mode->mipi_channel_size == 0 ||
		cur_tx_sensor_mode->mipi_channel == NULL) {
		dbg_sensor(1, "skip select mipi channel\n");
		return -1;
	}

	found = fimc_is_vendor_select_mipi_by_rf_channel(cur_tx_sensor_mode->mipi_channel,
				cur_tx_sensor_mode->mipi_channel_size);

	if (found != -1) {
		if (found < cur_tx_sensor_mode->sensor_setting_size) {
			sensor_cis_set_registers(subdev,
				cur_tx_sensor_mode->sensor_setting[found].setting,
				cur_tx_sensor_mode->sensor_setting[found].setting_size);
			sensor_imx516_rear_tx_freq = cur_tx_sensor_mode->sensor_setting[found].tx_freq;
			info("%s - tx setting %d freq %d\n", __func__, found, sensor_imx516_rear_tx_freq);
		} else {
			err("sensor setting size is out of bound");
		}
	}

	return 0;
}
#endif

int sensor_imx516_cis_set_frame_duration(struct v4l2_subdev *subdev, u32 duration);

int sensor_imx516_cis_get_uid_index(struct fimc_is_cis *cis, u32 mode, u32 *uid_index)
{
	struct fimc_is_core *core;
	struct fimc_is_vender_specific *specific = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	u32 sensor_uid = 0;
	int i = 0;

	if (mode >= SENSOR_IMX516_MODE_MAX || mode == -1) {
		err("sensor mode is out of bound (%d)", mode);
		return -1;
	}

	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		probe_info("core device is not yet probed");
		return -1;
	}

	specific = core->vender.private_data;
	if (sensor_peri->module->position == SENSOR_POSITION_REAR_TOF) {
		sensor_uid = specific->rear_tof_uid[mode];
	} else {  // SENSOR_POSITION_FRONT_TOF
		sensor_uid = specific->front_tof_uid[mode];
	}

	for (i = 0; i < sensor_imx516_max_uid_num[mode]; i++) {
		if (sensor_uid == sensor_imx516_uid[mode][i]) {
			*uid_index = i;
			break;
		}
	}

	if (i == sensor_imx516_max_uid_num[mode]) {
		err("sensor_imx516_cis_mode_change can't find uid(uid : %0xd) mode (%d)", sensor_uid, mode);
		return -1;
	}

	return 0;
}

int sensor_imx516_cis_check_rev(struct fimc_is_cis *cis)
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

	info("sensor_imx516_cis_check_rev start ");
	ret = fimc_is_sensor_read8(client, 0x0000, &rev);
	info("sensor_imx516_cis_check_rev 0x0000 %x ", rev);
	ret = fimc_is_sensor_read8(client, 0x0001, &rev);
	info("sensor_imx516_cis_check_rev 0x0001 %x ", rev);
	ret = fimc_is_sensor_read8(client, 0x0002, &rev);
	info("sensor_imx516_cis_check_rev 0x0002 %x ", rev);
	ret = fimc_is_sensor_read8(client, 0x0003, &rev);
	info("sensor_imx516_cis_check_rev 0x0003 %x ", rev);
	ret = fimc_is_sensor_read8(client, 0x0019,&rev);
	info("sensor_imx516_cis_check_rev 0x0019 %x ", rev);
	if (ret < 0) {
		err("fimc_is_sensor_read8 fail, (ret %d)", ret);
		ret = -EAGAIN;
	} else {
		cis->cis_data->cis_rev = rev;
	}
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	return ret;
}

/* CIS OPS */
int sensor_imx516_cis_init(struct v4l2_subdev *subdev)
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

	ret = sensor_imx516_cis_check_rev(cis);
	if (ret < 0) {
#ifdef USE_CAMERA_HW_BIG_DATA
		sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
		if (sensor_peri)
			fimc_is_sec_get_hw_param(&hw_param, sensor_peri->module->position);
		if (hw_param)
			hw_param->i2c_sensor_err_cnt++;
#endif
		warn("sensor_imx516_check_rev is fail when cis init");
		cis->rev_flag = true;
	}

	cis->cis_data->product_name = cis->id;
	cis->cis_data->cur_width = SENSOR_IMX516_MAX_WIDTH;
	cis->cis_data->cur_height = SENSOR_IMX516_MAX_HEIGHT;
	cis->cis_data->low_expo_start = 33000;
	cis->need_mode_change = false;
	sensor_imx516_frame_duration = 0;
	sensor_imx516_mode = -1;

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_imx516_cis_log_status(struct v4l2_subdev *subdev)
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
	pr_err("[SEN:DUMP] IMX516 *************************\n");
	fimc_is_sensor_read16(client, 0x0001, &data16);
	pr_err("[SEN:DUMP] manufacturer ID(%x)\n", data16);
	fimc_is_sensor_read8(client, SENSOR_DEPTH_MAP_NUMBER, &data8);
	pr_err("[SEN:DUMP] frame counter (%x)\n", data8);
	fimc_is_sensor_read8(client, 0x1001, &data8);
	pr_err("[SEN:DUMP] mode_select(%x)\n", data8);

#if 0
	pr_err("[SEN:DUMP] global \n");
	sensor_cis_dump_registers(subdev, sensor_imx516_global, sensor_imx516_global_size);
	pr_err("[SEN:DUMP] mode \n");
	sensor_cis_dump_registers(subdev, sensor_imx516_setfiles[0], sensor_imx516_setfile_sizes[0]);
#endif
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	pr_err("[SEN:DUMP] *******************************\n");

p_err:
	return ret;
}

int sensor_imx516_cis_set_global_setting(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	/* setfile global setting is at camera entrance */
	info("sensor_imx516_cis_set_global_setting");
	ret = sensor_cis_set_registers(subdev, sensor_imx516_global, sensor_imx516_global_size);
	if (ret < 0) {
		err("sensor_imx516_set_registers fail!!");
		goto p_err;
	}

	dbg_sensor(1, "[%s] global setting done\n", __func__);

p_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);
	return ret;
}

int sensor_imx516_get_HMAX_value(const u32 *regs, const u32 size)
{
	int i = 0;

	FIMC_BUG(!regs);

	sensor_imx516_HMAX = 0;
	for (i = 0; i < size; i += I2C_NEXT) {
		if (regs[i + I2C_ADDR] == SENSOR_HMAX_UPPER) {
			sensor_imx516_HMAX |= (regs[i + I2C_DATA] << 8);
		} else if (regs[i + I2C_ADDR] == SENSOR_HMAX_LOWER) {
			sensor_imx516_HMAX |= regs[i + I2C_DATA];
			return 0;
		}
	}
	return 0;
}

int sensor_imx516_cis_mode_change(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct fimc_is_cis *cis = NULL;
	struct fimc_is_vender_specific *specific = NULL;
	//int index = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	u32 sensor_uid = 0;
	u32 uid_index = 0;

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (mode >= SENSOR_IMX516_MODE_MAX) {
		err("invalid mode(%d)!!", mode);
		ret = -EINVAL;
		return ret;
	}

	/* If check_rev fail when cis_init, one more check_rev in mode_change */
	if (cis->rev_flag == true) {
		cis->rev_flag = false;
		ret = sensor_cis_check_rev(cis);
		if (ret < 0) {
			err("sensor_imx516_check_rev is fail");
			return ret;
		}
	}

	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		probe_info("core device is not yet probed");
		ret = -EINVAL;
		return ret;
	}

	specific = core->vender.private_data;
	if (sensor_peri->module->position == SENSOR_POSITION_REAR_TOF) {
#ifdef USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION
		sensor_imx516_rear_tx_freq = REAR_TX_DEFAULT_FREQ;
#endif
		sensor_uid = specific->rear_tof_uid[mode];
	} else {  // SENSOR_POSITION_FRONT_TOF
		sensor_uid = specific->front_tof_uid[mode];
	}

	ret = sensor_imx516_cis_get_uid_index(cis, mode, &uid_index);
	if (ret < 0) {
		return ret;
	}

	info("sensor_imx516_cis_mode_change pos %d sensor_uid %x mode %d index %d 0x%x",
		sensor_peri->module->position, sensor_uid, mode, uid_index, sensor_imx516_uid[mode][uid_index]);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	ret = sensor_cis_set_registers(subdev, sensor_imx516_setfiles[mode][uid_index], sensor_imx516_setfile_sizes[mode][uid_index]);
	if (ret < 0) {
		err("sensor_imx516_set_registers fail!!");
		goto p_err;
	}

	sensor_imx516_get_HMAX_value(sensor_imx516_setfiles[mode][uid_index], sensor_imx516_setfile_sizes[mode][uid_index]);
	sensor_imx516_mode = mode;
p_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);
	return ret;
}

int sensor_imx516_cis_wait_streamoff(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;
	u32 wait_cnt = 0, time_out_cnt = 250;
	u8 sensor_fcount = 0;

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	if (unlikely(!cis)) {
		err("cis is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;
	if (unlikely(!cis_data)) {
		err("cis_data is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	usleep_range(33000, 33000);
	I2C_MUTEX_LOCK(cis->i2c_lock);
	ret = fimc_is_sensor_read8(client, SENSOR_DEPTH_MAP_NUMBER, &sensor_fcount);
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	if (ret < 0) {
		err("i2c transfer fail addr(%x), val(%x), ret = %d\n", SENSOR_DEPTH_MAP_NUMBER, sensor_fcount, ret);
		ret = -EINVAL;
		goto p_err;
	}

	/*
	 * stream on (0x00 ~ 0xFE), stream off (0xFF)
	 */
	while (sensor_fcount != 0xFF) {
		I2C_MUTEX_LOCK(cis->i2c_lock);
		ret = fimc_is_sensor_read8(client, SENSOR_DEPTH_MAP_NUMBER, &sensor_fcount);
		I2C_MUTEX_UNLOCK(cis->i2c_lock);

		if (ret < 0) {
			err("i2c transfer fail addr(%x), val(%x), ret = %d\n", SENSOR_DEPTH_MAP_NUMBER, sensor_fcount, ret);
			ret = -EINVAL;
			goto p_err;
		}

		usleep_range(CIS_STREAM_OFF_WAIT_TIME, CIS_STREAM_OFF_WAIT_TIME);
		wait_cnt++;

		if (wait_cnt >= time_out_cnt) {
			err("[MOD:D:%d] %s, time out, wait_limit(%d) > time_out(%d), sensor_fcount(%d)",
					cis->id, __func__, wait_cnt, time_out_cnt, sensor_fcount);
			ret = -EINVAL;
			goto p_err;
		}

		dbg_sensor(1, "[MOD:D:%d] %s, sensor_fcount(%d), (wait_limit(%d) < time_out(%d))\n",
				cis->id, __func__, sensor_fcount, wait_cnt, time_out_cnt);
	}

p_err:
	return ret;
}

int sensor_imx516_cis_wait_streamon(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;
	u32 wait_cnt = 0, time_out_cnt = 250;
	u8 sensor_fcount = 0;

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	if (unlikely(!cis)) {
	    err("cis is NULL");
	    ret = -EINVAL;
	    goto p_err;
	}

	cis_data = cis->cis_data;
	if (unlikely(!cis_data)) {
	    err("cis_data is NULL");
	    ret = -EINVAL;
	    goto p_err;
	}

	client = cis->client;
	if (unlikely(!client)) {
	    err("client is NULL");
	    ret = -EINVAL;
	    goto p_err;
	}

	I2C_MUTEX_LOCK(cis->i2c_lock);
	ret = fimc_is_sensor_read8(client, SENSOR_DEPTH_MAP_NUMBER, &sensor_fcount);
	I2C_MUTEX_UNLOCK(cis->i2c_lock);
	if (ret < 0) {
		err("i2c transfer fail addr(%x), val(%x), ret = %d\n", SENSOR_DEPTH_MAP_NUMBER, sensor_fcount, ret);
		ret = -EINVAL;
		goto p_err;
	}

	/*
	 * Read sensor frame counter (sensor_fcount address = 0x0005)
	 * stream on (0x00 ~ 0xFE), stream off (0xFF)
	 */
	while (sensor_fcount == 0xff) {
		usleep_range(CIS_STREAM_ON_WAIT_TIME, CIS_STREAM_ON_WAIT_TIME);
		wait_cnt++;

		I2C_MUTEX_LOCK(cis->i2c_lock);
		ret = fimc_is_sensor_read8(client, SENSOR_DEPTH_MAP_NUMBER, &sensor_fcount);
		I2C_MUTEX_UNLOCK(cis->i2c_lock);

		if (ret < 0) {
			err("i2c transfer fail addr(%x), val(%x), ret = %d\n", SENSOR_DEPTH_MAP_NUMBER, sensor_fcount, ret);
			ret = -EINVAL;
			goto p_err;
		}

		if (wait_cnt >= time_out_cnt) {
			err("[MOD:D:%d] %s, Don't sensor stream on and time out, wait_limit(%d) > time_out(%d), sensor_fcount(%d)",
				cis->id, __func__, wait_cnt, time_out_cnt, sensor_fcount);
			ret = -EINVAL;
			goto p_err;
		}

		dbg_sensor(1, "[MOD:D:%d] %s, sensor_fcount(%d), (wait_limit(%d) < time_out(%d))\n",
				cis->id, __func__, sensor_fcount, wait_cnt, time_out_cnt);
	}

p_err:
	return ret;
}

int sensor_imx516_cis_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	struct fimc_is_module_enum *module;
	cis_shared_data *cis_data;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct exynos_platform_fimc_is_module *pdata;

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
	module = sensor_peri->module;

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);
	/* fps setting */
	if (sensor_imx516_frame_duration != 0)
		sensor_imx516_cis_set_frame_duration(subdev, sensor_imx516_frame_duration);

	/* Sensor stream on */
	info("sensor_imx516_cis_stream_on %d", module->position);

	I2C_MUTEX_LOCK(cis->i2c_lock);
#ifdef USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION
	if (module->position == SENSOR_POSITION_REAR_TOF) {
		sensor_imx516_cis_set_tx_clock(subdev);
	}
#endif
	fimc_is_sensor_write8(client, 0x1001, 0x01);
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	cis_data->stream_on = true;

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

	usleep_range(200, 200);
	ret = pdata->gpio_cfg(module, SENSOR_SCENARIO_ADDITIONAL_POWER, GPIO_SCENARIO_ON);
	if (ret) {
		clear_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
		err("gpio_cfg is fail(%d)", ret);
		goto p_err;
	}

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_imx516_cis_stream_off(struct v4l2_subdev *subdev)
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
	info("sensor_imx516_cis_stream_off");
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

int sensor_imx516_cis_set_exposure_time(struct v4l2_subdev *subdev, struct ae_param *target_exposure)
{
	#define AE_MAX 0x1D4C0
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
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

	if (target_exposure->long_val) {
		info("initial setting is skip");
		goto p_err;
	}

	value_32 = target_exposure->short_val * 120;
	if (value_32 > AE_MAX) {
		value_32 = AE_MAX;
	} else if (value_32 < sensor_imx516_HMAX) {
		value_32 = sensor_imx516_HMAX;
	}

	info("sensor_imx516_cis_set_exposure_time org %d value %x sensor_imx516_HMAX %x",
		target_exposure->short_val, value_32, sensor_imx516_HMAX);

	I2C_MUTEX_LOCK(cis->i2c_lock);
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
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

p_err:
	return ret;
}

int sensor_imx516_cis_set_frame_duration(struct v4l2_subdev *subdev, u32 duration)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	u16 cur_fps = 0;
	const u16 *fps_list = NULL;
	u8 fps = 0;
	u32 uid_index = 0;

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

	if (duration == 0) {
		err("duration is 0");
		return ret;
	}

	if (sensor_imx516_mode == -1) {
		sensor_imx516_frame_duration = duration;
		err("need sensor setting (%d)", duration);
		ret = -EINVAL;
		goto p_err;
	}

	sensor_imx516_frame_duration = duration;

	ret = sensor_imx516_cis_get_uid_index(cis, sensor_imx516_mode, &uid_index);
	if (ret < 0) {
		return ret;
	}

	fps_list = sensor_imx516_setfiles_fps[sensor_imx516_mode][uid_index];
	I2C_MUTEX_LOCK(cis->i2c_lock);
	fimc_is_sensor_write8(client, 0x0102, 0x01);

	info("%s : set %d", __func__, duration);
	switch (duration) {
		case SENSOR_FPS_5:
			cur_fps = fps_list[5];
			fps = cur_fps & 0xFF;
			fimc_is_sensor_write8(client, SENOSR_FPS_REGISTER_2, fps);
			fps = cur_fps >> 8;
			fimc_is_sensor_write8(client, SENOSR_FPS_REGISTER_1, fps);
			break;
		case SENSOR_FPS_10:
			cur_fps = fps_list[4];
			fps = cur_fps & 0xFF;
			fimc_is_sensor_write8(client, SENOSR_FPS_REGISTER_2, fps);
			fps = cur_fps >> 8;
			fimc_is_sensor_write8(client, SENOSR_FPS_REGISTER_1, fps);
			break;
		case SENSOR_FPS_15:
			cur_fps = fps_list[3];
			fps = cur_fps & 0xFF;
			fimc_is_sensor_write8(client, SENOSR_FPS_REGISTER_2, fps);
			fps = cur_fps >> 8;
			fimc_is_sensor_write8(client, SENOSR_FPS_REGISTER_1, fps);
			break;
		case SENSOR_FPS_20:
			cur_fps = fps_list[2];
			fps = cur_fps & 0xFF;
			fimc_is_sensor_write8(client, SENOSR_FPS_REGISTER_2, fps);
			fps = cur_fps >> 8;
			fimc_is_sensor_write8(client, SENOSR_FPS_REGISTER_1, fps);
			break;
		case SENSOR_FPS_24:
			cur_fps = fps_list[1];
			fps = cur_fps & 0xFF;
			fimc_is_sensor_write8(client, SENOSR_FPS_REGISTER_2, fps);
			fps = cur_fps >> 8;
			fimc_is_sensor_write8(client, SENOSR_FPS_REGISTER_1, fps);
			break;
		case SENSOR_FPS_30:
			cur_fps = fps_list[0];
			fps = cur_fps & 0xFF;
			fimc_is_sensor_write8(client, SENOSR_FPS_REGISTER_2, fps);
			fps = cur_fps >> 8;
			fimc_is_sensor_write8(client, SENOSR_FPS_REGISTER_1, fps);
			break;
		default:
			err("Unsupported fps : %d", duration);
			break;
	}

	fimc_is_sensor_write8(client, 0x0102, 0x00);
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

p_err:
	return ret;
}

int sensor_imx516_cis_set_laser_control(struct v4l2_subdev *subdev, u32 onoff)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
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

	info("sensor_imx516_cis_set_laser_control %d", onoff);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	if (onoff) {
		fimc_is_sensor_write8(client, SENSOR_LASER_CONTROL_REG_1, SENSOR_LASER_ON);
		fimc_is_sensor_write8(client, SENSOR_LASER_CONTROL_REG_2, SENSOR_LASER_ON);
	} else {
		fimc_is_sensor_write8(client, SENSOR_LASER_CONTROL_REG_1, SENSOR_LASER_OFF);
		fimc_is_sensor_write8(client, SENSOR_LASER_CONTROL_REG_2, SENSOR_LASER_OFF);
	}
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

p_err:
	return ret;
}

int sensor_imx516_cis_set_vcsel_current(struct v4l2_subdev *subdev, u32 value)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	cis_shared_data *cis_data;
	struct i2c_client *client;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	u8 value8 = (u8)value;

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
	FIMC_BUG(!sensor_peri);
	module = sensor_peri->module;

	cis_data = cis->cis_data;
	if (cis_data->stream_on == false) {
		err("sensor_imx516_cis_set_vcsel_current fail!! sensor is off ");
		goto p_err;
	}

	info("sensor_imx516_cis_set_vcsel_current %d", value);

	I2C_MUTEX_LOCK(cis->i2c_lock);

	/* for setting IPD_OFFSET */
	fimc_is_sensor_write8(client, 0x0403, 0x20);
	fimc_is_sensor_write8(client, 0x0405, 0x00);
	fimc_is_sensor_write8(client, 0x0407, 0x26);
	fimc_is_sensor_write8(client, 0x0526, 0x02);
	fimc_is_sensor_write8(client, 0x0527, 0x0D);
	fimc_is_sensor_write8(client, 0x0528, 0xC0);  /* IPD OFFSET */
	fimc_is_sensor_write8(client, 0x0401, 0x01);
	fimc_is_sensor_write8(client, 0x0400, 0x01);
	fimc_is_sensor_write8(client, 0x0401, 0x00);

	/* for setting VCSEL current */
	fimc_is_sensor_write8(client, 0x0403, 0x20);
	fimc_is_sensor_write8(client, 0x0405, 0x00);
	fimc_is_sensor_write8(client, 0x0407, 0x26);
	fimc_is_sensor_write8(client, 0x0526, 0x02);
	fimc_is_sensor_write8(client, 0x0527, 0x08);
	fimc_is_sensor_write8(client, 0x0528, value8);
	fimc_is_sensor_write8(client, 0x0401, 0x01);
	fimc_is_sensor_write8(client, 0x0400, 0x01);
	fimc_is_sensor_write8(client, 0x0401, 0x00);

	I2C_MUTEX_UNLOCK(cis->i2c_lock);
	usleep_range(5000, 5000);

p_err:
	return ret;
}

int sensor_imx516_cis_get_vcsel_photo_diode(struct v4l2_subdev *subdev, u16 *value)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	u8 value8_1 = 0, value8_0 = 0;

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

	I2C_MUTEX_LOCK(cis->i2c_lock);

	fimc_is_sensor_read8(client, 0x0588, &value8_1); /*0x058B for APC2_CHECK_DATA[9:8]*/
	fimc_is_sensor_read8(client, 0x058F, &value8_0); /* 0x0582 for APC2_CHECK_DATA[7:0] */

	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	*value = (((value8_1 & 0x30) << 4) | value8_0);
	info("sensor_imx516_cis_get_vcsel_photo_diode value8_1 0x%x value8_0 0x%x => (0x%x)%d", value8_1, value8_0, *value, *value);

p_err:
	return ret;
}

#if defined(USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION) || defined(USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION_SYSFS_ENABLE)
int sensor_imx516_cis_get_tx_freq(struct v4l2_subdev *subdev, u32 *value)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
	FIMC_BUG(!sensor_peri);
	module = sensor_peri->module;

	if (module->position == SENSOR_POSITION_REAR_TOF) {
#ifdef USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION_SYSFS_ENABLE
		info("%s freq variation disabled, rear tx freq %d", __func__, REAR_TX_DEFAULT_FREQ);
		*value = REAR_TX_DEFAULT_FREQ;
#else
		info("%s rear tx freq %d", __func__, sensor_imx516_rear_tx_freq);
		*value = sensor_imx516_rear_tx_freq;
#endif
	} else { //SENSOR_POSITION_FRONT_TOF
		info("%s front tx freq none ", __func__);
	}

	return ret;
}
#endif

static struct fimc_is_cis_ops cis_ops_imx516 = {
	.cis_init = sensor_imx516_cis_init,
	.cis_log_status = sensor_imx516_cis_log_status,
	.cis_set_global_setting = sensor_imx516_cis_set_global_setting,
	.cis_mode_change = sensor_imx516_cis_mode_change,
	.cis_stream_on = sensor_imx516_cis_stream_on,
	.cis_stream_off = sensor_imx516_cis_stream_off,
	.cis_set_exposure_time = sensor_imx516_cis_set_exposure_time,
	.cis_wait_streamoff = sensor_imx516_cis_wait_streamoff,
	.cis_wait_streamon = sensor_imx516_cis_wait_streamon,
	.cis_set_laser_control = sensor_imx516_cis_set_laser_control,
	.cis_set_laser_current = sensor_imx516_cis_set_vcsel_current,
	.cis_get_laser_photo_diode = sensor_imx516_cis_get_vcsel_photo_diode,
	.cis_set_frame_duration = sensor_imx516_cis_set_frame_duration,
#if defined(USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION) || defined(USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION_SYSFS_ENABLE)
	.cis_get_tof_tx_freq = sensor_imx516_cis_get_tx_freq,
#endif
};

static int cis_imx516_probe(struct i2c_client *client,
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

	sensor_peri = find_peri_by_cis_id(device, SENSOR_NAME_IMX516);
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

	cis->id = SENSOR_NAME_IMX516;
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
	cis->cis_ops = &cis_ops_imx516;

	if (of_property_read_string(dnode, "setfile", &setfile)) {
		err("setfile index read fail(%d), take default setfile!!", ret);
		setfile = "default";
	}

	if (strcmp(setfile, "default") == 0 || strcmp(setfile, "setA") == 0) {
		probe_info("%s setfile_A\n", __func__);
		sensor_imx516_global = sensor_imx516_setfile_A_Global;
		sensor_imx516_global_size = ARRAY_SIZE(sensor_imx516_setfile_A_Global);
		sensor_imx516_setfiles = sensor_imx516_setfiles_A;
		sensor_imx516_uid = sensor_imx516_setfiles_uid;
		sensor_imx516_max_uid_num = sensor_imx516_setfiles_uid_num;
		sensor_imx516_setfile_sizes = sensor_imx516_setfile_A_sizes;
		sensor_imx516_setfiles_fps = sensor_imx516_setfile_FPS_A;
#ifdef USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION
		sensor_imx516_tx_freq_sensor_mode = sensor_imx516_setfile_A_tof_sensor_mode;
		sensor_imx516_tx_freq_sensor_mode_size = sensor_imx516_setfile_A_verify_sensor_mode_size;
		sensor_imx516_verify_sensor_mode = sensor_imx516_setfile_A_verify_sensor_mode;
		sensor_imx516_verify_sensor_mode_size = sensor_imx516_setfile_A_verify_sensor_mode_size;
#endif
	}
#if 0
	else if (strcmp(setfile, "setB") == 0) {
		probe_info("%s setfile_B\n", __func__);
		sensor_imx516_global = sensor_imx516_setfile_B_Global;
		sensor_imx516_global_size = ARRAY_SIZE(sensor_imx516_setfile_B_Global);
		sensor_imx516_setfiles = sensor_imx516_setfiles_B;
		sensor_imx516_setfile_sizes = sensor_imx516_setfile_B_sizes;
	}
#endif
	else {
		err("%s setfile index out of bound, take default (setfile_A)", __func__);
		sensor_imx516_global = sensor_imx516_setfile_A_Global;
		sensor_imx516_global_size = ARRAY_SIZE(sensor_imx516_setfile_A_Global);
		sensor_imx516_setfiles = sensor_imx516_setfiles_A;
		sensor_imx516_uid = sensor_imx516_setfiles_uid;
		sensor_imx516_max_uid_num = sensor_imx516_setfiles_uid_num;
		sensor_imx516_setfile_sizes = sensor_imx516_setfile_A_sizes;
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
#ifdef USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION
	{
		int i = 0, j = 0, mode = 0, index = 0;
		for (mode = 0; mode < SENSOR_IMX516_MODE_MAX; mode++){
			for (i = 0; i < sensor_imx516_verify_sensor_mode_size[mode]; i++) {
				for (j = 0; j < sensor_imx516_max_uid_num[mode]; j++) {
					if (sensor_imx516_verify_sensor_mode[mode][i] == sensor_imx516_uid[mode][j]) {
						index = j;
						break;
					}
				}

				if (fimc_is_vendor_verify_mipi_channel(sensor_imx516_tx_freq_sensor_mode[mode][index].mipi_channel,
							sensor_imx516_tx_freq_sensor_mode[mode][index].mipi_channel_size)) {
					panic("wrong mipi channel");
					break;
				}
			}
		}
	}
#endif
	probe_info("%s done\n", __func__);

p_err:
	return ret;
}

static const struct of_device_id sensor_cis_imx516_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-cis-imx516",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sensor_cis_imx516_match);

static const struct i2c_device_id sensor_cis_imx516_idt[] = {
	{ SENSOR_NAME, 0 },
	{},
};

static struct i2c_driver sensor_cis_imx516_driver = {
	.probe	= cis_imx516_probe,
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sensor_cis_imx516_match,
		.suppress_bind_attrs = true,
	},
	.id_table = sensor_cis_imx516_idt
};

static int __init sensor_cis_imx516_init(void)
{
	int ret;

	probe_info("sensor_cis_imx516_init");
	ret = i2c_add_driver(&sensor_cis_imx516_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			sensor_cis_imx516_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_cis_imx516_init);
