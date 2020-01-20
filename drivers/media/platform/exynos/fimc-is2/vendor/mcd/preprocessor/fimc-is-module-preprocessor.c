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
#include "fimc-is-module-preprocessor.h"

#include "fimc-is-interface-library.h"
#include "fimc-is-cis.h"

#define FIMC_IS_DEV_COMP_DEV_NAME "73C3"

extern struct fimc_is_lib_support gPtr_lib_support;

#if 0
static int __used sensor_preproc_read_sysreg(struct v4l2_subdev *subdev, preproc_setting_info *setinfo)
{
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;
	int i;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	fimc_is_comp_i2c_write_2(client, 0xFCFC, 0x2000);

	for (i = 0; i < C73C3_NUM_OF_DBG_SYS_REG; i++) {
		if (preproc_dbg_sysreg_adrs[i][0] == 0x02) {
			u16 reg_val;

			if (fimc_is_comp_i2c_read_2(client, (u16)(preproc_dbg_sysreg_adrs[i][1] & 0x0000FFFF), &reg_val) == true)
				info("[PREPROC-C3] %s: %s = 0x%08x\n", __func__, preproc_dbg_sysreg_name[i], reg_val);
			else
				err("[PREPROC-C3] ReadByte_Reg2Data2() error of %s!", preproc_dbg_sysreg_name[i]);
		} else if (preproc_dbg_sysreg_adrs[i][0] == 0x04) {
			u32 reg_val;

			if (fimc_is_comp_i2c_read_4(client, (u16)(preproc_dbg_sysreg_adrs[i][1] & 0x0000FFFF), &reg_val) == true)
				info("[PREPROC-C3] %s: %s = 0x%08x\n", __func__, preproc_dbg_sysreg_name[i], reg_val);
			else
				err("[PREPROC-C3] ReadByte_Reg2Data2() error of %s!", preproc_dbg_sysreg_name[i]);
		} else {
		}
	}

	fimc_is_comp_i2c_write_2(client, 0xFCFC, 0x4000);

	return 0;
}
#endif

static int sensor_preproc_stream_on(struct v4l2_subdev *subdev)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	is_shared_data *is_data;
	struct i2c_client *client;
	cis_shared_data *cis_data;
	int ret = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;
	companion_data = &cis_data->companion_data;

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	CHECK_INIT_N_RET(module_preproc->initialized);

	if (companion_data->stream_on) {
		info("Companion already stream on\n");
		return 0;
	}

	ret = module_preproc->cmd_func[COMP_CMD_STREAM_ON](subdev, NULL, cis_data);
	if (ret) {
		err("STREAM_ON CMD failed");
		return ret;
	}

	return ret;
}

static int sensor_preproc_stream_off(struct v4l2_subdev *subdev)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	is_shared_data *is_data;
	cis_shared_data *cis_data;
	struct i2c_client *client;
	int ret = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;
	companion_data = &cis_data->companion_data;

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	CHECK_INIT_N_RET(module_preproc->initialized);

	ret = module_preproc->cmd_func[COMP_CMD_STREAM_OFF](subdev, NULL, cis_data);
	if (ret) {
		err("STREAM_OFF CMD failed");
		return ret;
	}

	return ret;
}

int sensor_preproc_get_mode(struct v4l2_subdev *subdev,
		struct fimc_is_device_sensor *device, u16 *mode_select)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_device_preproc *device_preproc;
	struct fimc_is_module_enum *module;
	const struct fimc_is_preproc_cfg *cfg_table, *select = NULL;
	u32 cfgs, mode = 0;
	u32 width, height, framerate;
	u32 approximate_value = UINT_MAX;
	int deviation;

	if (unlikely(!subdev)) {
		err("subdev_preprocessor is NULL\n");
		return -EINVAL;
	}

	if (unlikely(!device)) {
		err("device_sensor is NULL\n");
		return -EINVAL;
	}

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	if (unlikely(!preprocessor)) {
		merr("preprocessor is NULL\n", device);
		return -EINVAL;
	}

	device_preproc = preprocessor->device_preproc;
	if (unlikely(!device_preproc)) {
		merr("device_preproc is NULL\n", device);
		return -EINVAL;
	}

	module = device_preproc->module;
	if (unlikely(!module)) {
		merr("module_enum is NULL\n", device);
		return -EINVAL;
	}

	cfg_table = preprocessor->cfg;
	cfgs = preprocessor->cfgs;
	width = device->image.window.width;
	height = device->image.window.height;
	framerate = device->image.framerate;

	/* find sensor mode by w/h and fps range */
	for (mode = 0; mode < cfgs; mode++) {
		if ((cfg_table[mode].width == width) && (cfg_table[mode].height == height)) {
			deviation = cfg_table[mode].framerate - framerate;
			if (deviation == 0) {
				/* You don't need to find another sensor mode */
				select = &cfg_table[mode];
				break;
			} else if ((deviation > 0) && approximate_value > abs(deviation)) {
				/* try to find framerate smaller than previous */
				approximate_value = abs(deviation);
				select = &cfg_table[mode];
			}
		}
	}

	if (!select) {
		merr("preproc mode(%dx%d@%dfps) is not found", device, width, height, framerate);
		return -EINVAL;
	}

	minfo("preproc mode(%dx%d@%d) = %d\n", device,
			select->width,
			select->height,
			select->framerate,
			select->mode);

	*mode_select = select->mode;

	return 0;
}

static int sensor_preproc_mode_change(struct v4l2_subdev *subdev,
		struct fimc_is_device_sensor *device)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	cis_shared_data *cis_data;
	struct i2c_client *client;
	int ret = 0;

	BUG_ON(!subdev);
	BUG_ON(!device);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	CHECK_INIT_N_RET(module_preproc->initialized);
	
	ret = module_preproc->cmd_func[COMP_CMD_MODE_CHANGE](subdev, NULL, cis_data);
	if (ret) {
		err("MODE_CHANGE CMD failed");
		return ret;
	}

	return ret;
}

static int sensor_preproc_set_af(struct v4l2_subdev *subdev, void *param)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	cis_shared_data *cis_data;
	struct i2c_client *client;
	preproc_setting_info setinfo = {param, 0};
	int ret = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	CHECK_INIT_N_RET(module_preproc->initialized);

	ret = module_preproc->cmd_func[COMP_CMD_AF_INFO](subdev, &setinfo, cis_data);
	if (ret) {
		err("SET_AF CMD failed");
	}

	return ret;
}

static int sensor_preproc_enable_block(struct v4l2_subdev *subdev, void *param)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	cis_shared_data *cis_data;
	struct i2c_client *client;
	preproc_setting_info setinfo = {param, 0};
	int ret = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	CHECK_INIT_N_RET(module_preproc->initialized);

	ret = module_preproc->cmd_func[COMP_CMD_BLOCK_ON_OFF](subdev, &setinfo, cis_data);
	if (ret) {
		err("ENABLE_BLOCK CMD failed");
	}

	return ret;
}

static int sensor_preproc_set_drc(struct v4l2_subdev *subdev, void *param)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	cis_shared_data *cis_data;
	struct i2c_client *client;
	preproc_setting_info setinfo = {param, 0};
	int ret = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	CHECK_INIT_N_RET(module_preproc->initialized);

	ret = module_preproc->cmd_func[COMP_CMD_DRC_SET_INFO](subdev, &setinfo, cis_data);
	if (ret) {
		err("SET_DRC CMD failed");
	}

	return ret;
}

static int sensor_preproc_set_ir(struct v4l2_subdev *subdev, void *param)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	cis_shared_data *cis_data;
	struct i2c_client *client;
	preproc_setting_info setinfo = {param, 0};
	int ret = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	CHECK_INIT_N_RET(module_preproc->initialized);

	ret = module_preproc->cmd_func[COMP_CMD_IR_SET_INFO](subdev, &setinfo, cis_data);
	if (ret) {
		err("SET_IR CMD failed");
	}

	return ret;
}

static int sensor_preproc_set_lsc_3aa(struct v4l2_subdev *subdev, void *param)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	cis_shared_data *cis_data;
	struct i2c_client *client;
	preproc_setting_info setinfo = {param, 0};
	int ret = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	CHECK_INIT_N_RET(module_preproc->initialized);

	ret = module_preproc->cmd_func[COMP_CMD_LSC_SET_3AA_INFO](subdev, &setinfo, cis_data);
	if (ret) {
		err("SET_LSC_3AA CMD failed");
	}

	return ret;
}

static int sensor_preproc_wdr_ae(struct v4l2_subdev *subdev, void *param)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	cis_shared_data *cis_data;
	struct i2c_client *client;
	preproc_setting_info setinfo = {param, 0};
	int ret = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	CHECK_INIT_N_RET(module_preproc->initialized);

	ret = module_preproc->cmd_func[COMP_CMD_WDR_SET_AE_INFO](subdev, &setinfo, cis_data);
	if (ret) {
		err("WDR_AE CMD failed");
	}

	return ret;
}

static int sensor_preproc_set_motion(struct v4l2_subdev *subdev, void *param)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	cis_shared_data *cis_data;
	struct i2c_client *client;
	preproc_setting_info setinfo = {param, 0};
	int ret = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	CHECK_INIT_N_RET(module_preproc->initialized);

	ret = module_preproc->cmd_func[COMP_CMD_MOTION_SET_INFO](subdev, &setinfo, cis_data);
	if (ret) {
		err("SET_MOTION CMD failed");
	}

	return ret;
}

static int sensor_preproc_set_bypass(struct v4l2_subdev *subdev, void *param)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	cis_shared_data *cis_data;
	struct i2c_client *client;
	preproc_setting_info setinfo = {param, 0};
	int ret = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	CHECK_INIT_N_RET(module_preproc->initialized);

	ret = module_preproc->cmd_func[COMP_CMD_BYPASS_SET_INFO](subdev, &setinfo, cis_data);
	if (ret) {
		err("SET_BYPASS CMD failed");
	}

	return ret;
}

static int sensor_preproc_set_le_mode(struct v4l2_subdev *subdev, void *param)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	cis_shared_data *cis_data;
	struct i2c_client *client;
	preproc_setting_info setinfo = {param, 0};
	int ret = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	CHECK_INIT_N_RET(module_preproc->initialized);

	ret = module_preproc->cmd_func[COMP_CMD_LEMODE_SET_INFO](subdev, &setinfo, cis_data);
	if (ret) {
		err("SET_LE_MODE CMD failed");
	}

	return ret;
}

static int sensor_preproc_set_seamless_mode(struct v4l2_subdev *subdev, void *param)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	cis_shared_data *cis_data;
	struct i2c_client *client;
	preproc_setting_info setinfo = {param, 0};
	int ret = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	CHECK_INIT_N_RET(module_preproc->initialized);

	ret = module_preproc->cmd_func[COMP_CMD_SEAMLESSMODE_SET_INFO](subdev, &setinfo, cis_data);
	if (ret) {
		err("SEAMLESS_MODE CMD failed");
	}

	return ret;
}

static int sensor_preproc_set_roi(struct v4l2_subdev *subdev, void *param)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	cis_shared_data *cis_data;
	struct i2c_client *client;
	preproc_setting_info setinfo = {param, 0};
	int ret = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	CHECK_INIT_N_RET(module_preproc->initialized);

	ret = module_preproc->cmd_func[COMP_CMD_ROI_SET_INFO](subdev, &setinfo, cis_data);
	if (ret) {
		err("SET_ROI CMD failed");
	}

	return ret;
}

static int sensor_preproc_set_power_mode(struct v4l2_subdev *subdev, void *param)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	cis_shared_data *cis_data;
	struct i2c_client *client;
	preproc_setting_info setinfo = {param, 0};
	int ret = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	CHECK_INIT_N_RET(module_preproc->initialized);

	ret = module_preproc->cmd_func[COMP_CMD_POWER_MODE](subdev, &setinfo, cis_data);
	if (ret) {
		err("POWER_MODE CMD failed");
	}

	return ret;
}

static int sensor_preproc_change_config(struct v4l2_subdev *subdev, void *param)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	cis_shared_data *cis_data;
	struct i2c_client *client;
	preproc_setting_info setinfo = {param, 0};
	int ret = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	CHECK_INIT_N_RET(module_preproc->initialized);

	ret = module_preproc->cmd_func[COMP_CMD_CHANGE_CONFIG](subdev, &setinfo, cis_data);
	if (ret) {
		err("CHANGE_CONFIG CMD failed");
	}

	return ret;
}

static int sensor_preproc_debug(struct v4l2_subdev *subdev)
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

static int sensor_preproc_wait_s_input(struct v4l2_subdev *subdev)
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

	info("preproc finished(wait : %d)", waiting);

p_err:
	return ret;
}

static int sensor_preproc_read_af_stat(struct v4l2_subdev *subdev, void *param)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	cis_shared_data *cis_data;
	struct i2c_client *client;
	preproc_setting_info setinfo = {param, 0};
	int ret = 0;
	int buf_index = 0;
	u8 *buf = NULL;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	CHECK_INIT_N_RET(module_preproc->initialized);

	for (buf_index = 0; buf_index < PREPROC_PDAF_BUF_NUM; buf_index++) {
		if (module_preproc->pdaf_buf[buf_index] == NULL) {
			err("af read buffer not allocated.");
			goto exit;
		}
	}

	buf = module_preproc->pdaf_buf[module_preproc->pdaf_count++ % PREPROC_PDAF_BUF_NUM]; 

	C73C3_ReadAfStatFromSpi(preprocessor->spi, &setinfo, cis_data, &buf);

exit:
	return ret;
}

static int sensor_preproc_read_af_reg(struct v4l2_subdev *subdev, void *param)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	cis_shared_data *cis_data;
	struct i2c_client *client;
	preproc_setting_info setinfo = {param, 0};
	int ret = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;

	client = preprocessor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		return -EINVAL;
	}

	CHECK_INIT_N_RET(module_preproc->initialized);

	ret = module_preproc->cmd_func[COMP_CMD_READ_AF_REG](subdev,
		&setinfo, cis_data);
	if (ret) {
		err("READ_AF_REG CMD failed");
	}

	return ret;
}

static int sensor_preproc_init(struct v4l2_subdev *subdev, u32 val){
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct fimc_is_device_sensor *device;
	struct fimc_is_module_enum *module;
	struct i2c_client *client = NULL;
	cis_shared_data *cis_data;
	preproc_setting_info setinfo = {NULL, 0};
	int ret = 0;
	int buf_index = 0;

	BUG_ON(!subdev);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	if (!preprocessor) {
		err("preprocessor is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	device = (struct fimc_is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	if (!device) {
		err("device sensor is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(device->subdev_module);
	BUG_ON(!module);

	client = preprocessor->client;
	
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	BUG_ON(!module_preproc);

	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;
	BUG_ON(!sensor_peri->cis.cis_data);

	module_preproc->cis_data = sensor_peri->cis.cis_data;
	cis_data = module_preproc->cis_data;
	module_preproc->pdaf_count = 0;

	fimc_is_get_preproc_cfg(&sensor_peri->preprocessor->cfg,
							&sensor_peri->preprocessor->cfgs, cis_data);

	for (buf_index = 0; buf_index < PREPROC_PDAF_BUF_NUM; buf_index++) {
		if (module_preproc->pdaf_buf[buf_index] == NULL) {
			module_preproc->pdaf_buf[buf_index] = \
						(u8 *)kmalloc(sizeof(u8) * MAX_PDAF_STATS_SIZE , GFP_KERNEL);
			if (!module_preproc->pdaf_buf[buf_index]) {
				err("mem alloc failed. (%u bytes), buf_index = %d", MAX_PDAF_STATS_SIZE , buf_index);
				return -ENOMEM;
			}
		}
	}
	
	init_comp_func_73c3(module_preproc->cmd_func, cis_data, false, sensor_peri->cis.id);
	
	ret = module_preproc->cmd_func[COMP_CMD_INIT](subdev, &setinfo, cis_data);
	if (ret) {
		err("INIT CMD failed");

		for (buf_index = 0; buf_index < PREPROC_PDAF_BUF_NUM; buf_index++) {
			if (module_preproc->pdaf_buf[buf_index]) {
				kfree((void *)module_preproc->pdaf_buf[buf_index]);
				module_preproc->pdaf_buf[buf_index] = NULL;
			}
		}
		return ret;
	}

	module_preproc->initialized = true;

p_err:
	return ret;
}

int sensor_preproc_deinit(struct v4l2_subdev *subdev)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	struct fimc_is_device_sensor *device;
	struct fimc_is_module_enum *module;
	struct i2c_client *client = NULL;
	preproc_setting_info setinfo = {NULL, 0};
	int buf_index = 0;
	int ret = 0;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	if (!preprocessor) {
		err("preprocessor is NULL");
		ret = -EINVAL;
		goto exit;
	}
	
	device = (struct fimc_is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	if (!device) {
		err("device sensor is NULL");
		ret = -EINVAL;
		goto exit;
	}

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(device->subdev_module);
	BUG_ON(!module);

	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	BUG_ON(!module_preproc);

	CHECK_INIT_N_RET(module_preproc->initialized);

	client = preprocessor->client;

	/* Block all accesses to preprocessor driver after this */
	module_preproc->initialized = false;

	for (buf_index = 0; buf_index < PREPROC_PDAF_BUF_NUM; buf_index++) {
		if (module_preproc->pdaf_buf[buf_index]) {
			kfree((void *)module_preproc->pdaf_buf[buf_index]);
			module_preproc->pdaf_buf[buf_index] = NULL;
		}
	}

	ret = module_preproc->cmd_func[COMP_CMD_DEINIT](subdev, &setinfo, module_preproc->cis_data);
	if (ret) {
		err("DEINIT CMD failed");
		ret = -EINVAL;
		goto exit;
	}

exit:
	return ret;
}

int sensor_preproc_s_format(struct v4l2_subdev *subdev, struct fimc_is_device_sensor *device)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_module_preprocessor *module_preproc;
	struct i2c_client *client = NULL;
	cis_shared_data *cis_data;
	int ret = 0;
	u16 compmode;

	BUG_ON(!subdev);
	BUG_ON(!device);

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	BUG_ON(!preprocessor);
	BUG_ON(!preprocessor->private_data);
	
	client = preprocessor->client;
	BUG_ON(!client);
	
	module_preproc = (struct fimc_is_module_preprocessor *)preprocessor->private_data;
	cis_data = module_preproc->cis_data;

	ret = sensor_preproc_get_mode(subdev, device, &compmode);
	if (ret) {
		merr("preproc search mode is fail (%d)\n", device, ret);
		ret = -EINVAL;
		goto exit;
	}

	cis_data->companion_data.config_idx = compmode;

	if (module_preproc->cmd_func[COMP_CMD_UPDATE_SUPPORT_MODE](subdev, NULL, cis_data) != 0) {
		err("COMP_CMD_UPDATE_SUPPORT_MODE failed");
		ret = -EINVAL;
		goto exit;
	}

exit:
	return ret;
}

static struct fimc_is_preprocessor_ops preprocessor_ops = {
	.preprocessor_stream_on = sensor_preproc_stream_on,
	.preprocessor_stream_off = sensor_preproc_stream_off,
	.preprocessor_mode_change = sensor_preproc_mode_change,
	.preprocessor_debug = sensor_preproc_debug,
	.preprocessor_wait_s_input = sensor_preproc_wait_s_input,
	.preprocessor_set_af = sensor_preproc_set_af,
	.preprocessor_enable_block = sensor_preproc_enable_block,
	.preprocessor_set_lsc_3aa = sensor_preproc_set_lsc_3aa,
	.preprocessor_wdr_ae = sensor_preproc_wdr_ae,
	.preprocessor_set_motion = sensor_preproc_set_motion,
	.preprocessor_set_bypass = sensor_preproc_set_bypass,
	.preprocessor_set_le_mode = sensor_preproc_set_le_mode,
	.preprocessor_set_seamless_mode = sensor_preproc_set_seamless_mode,
	.preprocessor_set_drc = sensor_preproc_set_drc,
	.preprocessor_set_ir = sensor_preproc_set_ir,
	.preprocessor_set_roi = sensor_preproc_set_roi,
	.preprocessor_set_power_mode = sensor_preproc_set_power_mode,
	.preprocessor_change_config = sensor_preproc_change_config,
	.preprocessor_read_af_stat = sensor_preproc_read_af_stat,
	.preprocessor_read_af_reg = sensor_preproc_read_af_reg,
	.preprocessor_deinit = sensor_preproc_deinit,
	.preprocessor_s_format = sensor_preproc_s_format,
};

static const struct v4l2_subdev_core_ops core_ops = {
	.init = sensor_preproc_init,
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
};

static int sensor_preproc_probe(struct platform_device *pdev)
{
	int ret = 0;
	int gpio_pdaf = 0;
	struct fimc_is_core *core= NULL;
	struct v4l2_subdev *subdev_preprocessor = NULL;
	struct fimc_is_preprocessor *preprocessor = NULL;
	struct fimc_is_module_preprocessor *module_preproc = NULL;
	struct fimc_is_device_sensor *device = NULL;
	struct device_node *dnode;
	struct device *dev;
	u32 sensor_id = 0;
	u32 product_name = 0;
	int preproc_irq = 0;

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

	ret = of_property_read_u32(dnode, "id", &sensor_id);
	if (ret) {
		err("id read is fail(%d)", ret);
		goto p_err;
	}

	ret = of_property_read_u32(dnode, "product_name", &product_name);
	if (ret) {
		err("product_name read is fail(%d)", ret);
		goto p_err;
	}

	device = &core->sensor[sensor_id];
	if (!device) {
		err("sensor device is NULL");
		return -EPROBE_DEFER;
	}
	probe_info("device module id = %d \n",device->pdata->id);

	preprocessor = kzalloc(sizeof(struct fimc_is_preprocessor), GFP_KERNEL);
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

	preprocessor->device_preproc = &core->preproc;
	preprocessor->preprocessor_ops = &preprocessor_ops;

	/* This name must is match to sensor_open_extended actuator name */
	preprocessor->id = product_name;
	preprocessor->subdev = subdev_preprocessor;
	preprocessor->device = sensor_id;
	preprocessor->client = core->client0;
	preprocessor->spi = core->spi1.device;
	preprocessor->i2c_lock = NULL;

	module_preproc = kzalloc(sizeof(struct fimc_is_module_preprocessor), GFP_KERNEL);
	if (!module_preproc) {
		err("module_preproc is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	preprocessor->private_data = module_preproc;

	/* Check IRQ */
	gpio_pdaf = of_get_named_gpio(dnode, "gpio_pdaf", 0);
	if (!gpio_is_valid(gpio_pdaf)) {
		err("failed to get preproc pdaf gpio");
		ret = -ENODEV;
		goto p_err;
	}

	preproc_irq = gpio_to_irq(gpio_pdaf);
	if (preproc_irq < 0) {
		err("irq=%d for gpio_pdaf=%d not correct", preproc_irq, gpio_pdaf);
		ret = -ENODEV;
		goto p_err;
	}

	gPtr_lib_support.intr_handler[ID_GENERAL_INTR_PREPROC_PDAF].irq = preproc_irq;

	device->subdev_preprocessor = subdev_preprocessor;
	device->preprocessor = preprocessor;

	v4l2_subdev_init(subdev_preprocessor, &subdev_ops);
	v4l2_set_subdevdata(subdev_preprocessor, preprocessor);
	v4l2_set_subdev_hostdata(subdev_preprocessor, device);
	snprintf(subdev_preprocessor->name, V4L2_SUBDEV_NAME_SIZE, "preprop-subdev.%d", sensor_id);

	ret = v4l2_device_register_subdev(&device->v4l2_dev, subdev_preprocessor);
	if (ret) {
		merr("v4l2_device_register_subdev is fail(%d)", device, ret);
		goto p_err;
	}

	probe_info("%s done\n", __func__);

	return ret;

p_err:
	if (preprocessor)
		kfree(preprocessor);

	if (subdev_preprocessor)
		kfree(subdev_preprocessor);

	if (module_preproc)
		kfree(module_preproc);

	return ret;
}

static int sensor_preproc_remove(struct platform_device *pdev)
{
	int ret = 0;

	info("%s\n", __func__);

	return ret;
}

#ifdef CONFIG_OF
static const struct of_device_id exynos_fimc_device_preproc_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-device-companion0",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fimc_device_preproc_match);

static struct platform_driver fimc_is_device_preproc_driver = {
	.probe		= sensor_preproc_probe,
	.remove		= sensor_preproc_remove,
	.driver = {
		.name	= FIMC_IS_DEV_COMP_DEV_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = exynos_fimc_device_preproc_match,
	}
};

#else
static struct platform_device_id fimc_is_device_preproc_driver_ids[] = {
	{
		.name		= FIMC_IS_DEV_COMP_DEV_NAME,
		.driver_data	= 0,
	},
	{},
};
MODULE_DEVICE_TABLE(platform, fimc_is_device_preproc_driver_ids);

static struct platform_driver fimc_is_device_preproc_driver = {
	.probe		= sensor_preproc_probe,
	.remove		= __devexit_p(sensor_preproc_remove),
	.id_table	= fimc_is_device_preproc_driver_ids,
	.driver	  = {
		.name	= FIMC_IS_DEV_COMP_DEV_NAME,
		.owner	= THIS_MODULE,
	}
};
#endif

static int __init sensor_preproc_module_init(void)
{
	int ret = platform_driver_register(&fimc_is_device_preproc_driver);
	if (ret)
		err("platform_driver_register failed: %d\n", ret);

	return ret;
}
late_initcall(sensor_preproc_module_init);

static void __exit sensor_preproc_module_exit(void)
{
	platform_driver_unregister(&fimc_is_device_preproc_driver);
}
module_exit(sensor_preproc_module_exit);

MODULE_AUTHOR("Wonjin LIM<wj.lim@samsung.com>");
MODULE_DESCRIPTION("Exynos FIMC_IS_DEVICE_COMPANION driver");
MODULE_LICENSE("GPL");
