/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>

#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>

#include "fimc-is-device-ischain.h"
#include "fimc-is-control-sensor.h"
#include "fimc-is-device-sensor-peri.h"

static int preproc_itf_set_af(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionSetAfInfoStr_73C3 *param)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct v4l2_subdev *subdev;
	int ret = 0;

	BUG_ON(!itf);
	BUG_ON(!param);
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, preprocessor_inferface);
	subdev = sensor_peri->subdev_preprocessor;
	BUG_ON(!subdev);

	/* info("%s called\n", __func__); */

	ret = CALL_PREPROPOPS(sensor_peri->preprocessor, preprocessor_set_af, subdev, param);
	if (ret) {
		err("preprocessor_set_af fail");
		goto p_err;
	}

p_err:
	return ret;
}

/* C73C3_BlockOnOff() */
static int preproc_itf_enable_block(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionEnableBlockStr *param)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct v4l2_subdev *subdev;
	int ret = 0;

	BUG_ON(!itf);
	BUG_ON(!param);
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, preprocessor_inferface);
	subdev = sensor_peri->subdev_preprocessor;
	BUG_ON(!subdev);

	/* info("%s called\n", __func__); */

	ret = CALL_PREPROPOPS(sensor_peri->preprocessor, preprocessor_enable_block, subdev, param);
	if (ret) {
		err("preprocessor_enable_block fail");
		goto p_err;
	}

p_err:
	return ret;
}

static int preproc_itf_set_lsc_3aa(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionSetLsc3aaInfoStr *param)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct v4l2_subdev *subdev;
	int ret = 0;

	BUG_ON(!itf);
	BUG_ON(!param);
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, preprocessor_inferface);
	subdev = sensor_peri->subdev_preprocessor;
	BUG_ON(!subdev);

	/* info("%s called\n", __func__); */

	ret = CALL_PREPROPOPS(sensor_peri->preprocessor, preprocessor_set_lsc_3aa, subdev, param);
	if (ret) {
		err("preprocessor_set_lsc_3aa fail");
		goto p_err;
	}

p_err:
	return ret;
}

static int preproc_itf_set_wdr_ae(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionSetWdrAeInfoStr *param)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct v4l2_subdev *subdev;
	int ret = 0;

	BUG_ON(!itf);
	BUG_ON(!param);
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, preprocessor_inferface);
	subdev = sensor_peri->subdev_preprocessor;
	BUG_ON(!subdev);

	/* info("%s called\n", __func__); */

	ret = CALL_PREPROPOPS(sensor_peri->preprocessor, preprocessor_wdr_ae, subdev, param);
	if (ret) {
		err("preprocessor_wdr_ae fail");
		goto p_err;
	}

p_err:
	return ret;
}

static int preproc_itf_set_motion(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionSetMotionInfoStr *param)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct v4l2_subdev *subdev;
	int ret = 0;

	BUG_ON(!itf);
	BUG_ON(!param);
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, preprocessor_inferface);
	subdev = sensor_peri->subdev_preprocessor;
	BUG_ON(!subdev);

	/* info("%s called\n", __func__); */

	ret = CALL_PREPROPOPS(sensor_peri->preprocessor, preprocessor_set_motion, subdev, param);
	if (ret) {
		err("preprocessor_set_motion fail");
		goto p_err;
	}

p_err:
	return ret;
}

static int preproc_itf_set_bypass(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionSetBypassInfoStr *param)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct v4l2_subdev *subdev;
	int ret = 0;

	BUG_ON(!itf);
	BUG_ON(!param);
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, preprocessor_inferface);
	subdev = sensor_peri->subdev_preprocessor;
	BUG_ON(!subdev);

	/* info("%s called\n", __func__); */

	ret = CALL_PREPROPOPS(sensor_peri->preprocessor, preprocessor_set_bypass, subdev, param);
	if (ret) {
		err("preprocessor_set_bypass fail");
		goto p_err;
	}

p_err:
	return ret;
}

static int preproc_itf_set_seamless_mode(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionSetSeamlessModeInfoStr *param)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct v4l2_subdev *subdev;
	int ret = 0;

	BUG_ON(!itf);
	BUG_ON(!param);
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, preprocessor_inferface);
	subdev = sensor_peri->subdev_preprocessor;
	BUG_ON(!subdev);

	/* info("%s called\n", __func__); */

	ret = CALL_PREPROPOPS(sensor_peri->preprocessor, preprocessor_set_seamless_mode, subdev, param);
	if (ret) {
		err("preprocessor_set_seamless_mode fail");
		goto p_err;
	}

p_err:
	return ret;
}

static int preproc_itf_set_drc(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionSetDrcInfoStr *param)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct v4l2_subdev *subdev;
	int ret = 0;

	BUG_ON(!itf);
	BUG_ON(!param);
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, preprocessor_inferface);
	subdev = sensor_peri->subdev_preprocessor;
	BUG_ON(!subdev);

	/* info("%s called\n", __func__); */

	ret = CALL_PREPROPOPS(sensor_peri->preprocessor, preprocessor_set_drc, subdev, param);
	if (ret) {
		err("preprocessor_set_drc fail");
		goto p_err;
	}

p_err:
	return ret;
}

static int preproc_itf_set_ir(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionSetIrInfoStr *param)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct v4l2_subdev *subdev;
	int ret = 0;

	BUG_ON(!itf);
	BUG_ON(!param);
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, preprocessor_inferface);
	subdev = sensor_peri->subdev_preprocessor;
	BUG_ON(!subdev);

	/* info("%s called\n", __func__); */

	ret = CALL_PREPROPOPS(sensor_peri->preprocessor, preprocessor_set_ir, subdev, param);
	if (ret) {
		err("preprocessor_set_ir fail");
		goto p_err;
	}

p_err:
	return ret;
}

static int preproc_itf_set_roi(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionSetRoiInfoStr *param)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct v4l2_subdev *subdev;
	int ret = 0;

	BUG_ON(!itf);
	BUG_ON(!param);
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, preprocessor_inferface);
	subdev = sensor_peri->subdev_preprocessor;
	BUG_ON(!subdev);

	/* info("%s called\n", __func__); */

	ret = CALL_PREPROPOPS(sensor_peri->preprocessor, preprocessor_set_roi, subdev, param);
	if (ret) {
		err("preprocessor_set_roi fail");
		goto p_err;
	}

p_err:
	return ret;
}

static int preproc_itf_set_power_mode(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionPowerModeCfgStr *param)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct v4l2_subdev *subdev;
	int ret = 0;

	BUG_ON(!itf);
	BUG_ON(!param);
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, preprocessor_inferface);
	subdev = sensor_peri->subdev_preprocessor;
	BUG_ON(!subdev);

	/* info("%s called\n", __func__); */

	ret = CALL_PREPROPOPS(sensor_peri->preprocessor, preprocessor_set_power_mode, subdev, param);
	if (ret) {
		err("preprocessor_set_power_mode fail");
		goto p_err;
	}

p_err:
	return ret;
}

static int preproc_itf_change_config(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionChangeCfgStr *param)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct v4l2_subdev *subdev;
	int ret = 0;

	BUG_ON(!itf);
	BUG_ON(!param);
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, preprocessor_inferface);
	subdev = sensor_peri->subdev_preprocessor;
	BUG_ON(!subdev);

	/* info("%s called\n", __func__); */

	ret = CALL_PREPROPOPS(sensor_peri->preprocessor, preprocessor_change_config, subdev, param);
	if (ret) {
		err("preprocessor_change_config fail");
		goto p_err;
	}

p_err:
	return ret;
}

static int preproc_itf_read_af_stat(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionReadAfStatFromSpiStr *param)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct v4l2_subdev *subdev;
	int ret = 0;

	BUG_ON(!itf);
	BUG_ON(!param);
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, preprocessor_inferface);
	subdev = sensor_peri->subdev_preprocessor;
	BUG_ON(!subdev);

	/* info("%s called\n", __func__); */

	ret = CALL_PREPROPOPS(sensor_peri->preprocessor, preprocessor_read_af_stat, subdev, param);
	if (ret) {
		err("preprocessor_read_af_stat fail");
		goto p_err;
	}

p_err:
	return ret;
}

static int preproc_itf_read_af_reg(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionReadAfReg *param)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct v4l2_subdev *subdev;
	int ret = 0;

	BUG_ON(!itf);
	BUG_ON(!param);
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, preprocessor_inferface);
	subdev = sensor_peri->subdev_preprocessor;
	BUG_ON(!subdev);

	ret = CALL_PREPROPOPS(sensor_peri->preprocessor, preprocessor_read_af_reg, subdev, param);
	if (ret) {
		err("preprocessor_read_af_reg fail");
		goto p_err;
	}

p_err:
	return ret;
}

int preproc_itf_get_ae_gains(struct fimc_is_preprocessor_interface *itf, u32 **preproc_auto_exposure)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
        cis_shared_data *cis_data = NULL;
        int ret = 0;

	BUG_ON(!itf);
        sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, preprocessor_inferface);
        cis_data = sensor_peri->cis.cis_data;
        *preproc_auto_exposure = (u32 *)cis_data->preproc_auto_exposure;
        
        BUG_ON(!cis_data);

        return ret;
}

int init_preprocessor_interface(struct fimc_is_preprocessor_interface *itf)
{
	int ret = 0;

	memset(itf, 0x0, sizeof(struct fimc_is_preprocessor_interface));

	itf->magic = PREPROCESSOR_INTERFACE_MAGIC;

	itf->preproc_itf_ops.set_af = preproc_itf_set_af;
	itf->preproc_itf_ops.enable_block = preproc_itf_enable_block;
	itf->preproc_itf_ops.set_lsc_3aa = preproc_itf_set_lsc_3aa;
	itf->preproc_itf_ops.set_wdr_ae = preproc_itf_set_wdr_ae;
	itf->preproc_itf_ops.set_motion = preproc_itf_set_motion;
	itf->preproc_itf_ops.set_bypass = preproc_itf_set_bypass;
	itf->preproc_itf_ops.set_seamless_mode = preproc_itf_set_seamless_mode;
	itf->preproc_itf_ops.set_drc = preproc_itf_set_drc;
	itf->preproc_itf_ops.set_ir = preproc_itf_set_ir;
	itf->preproc_itf_ops.set_roi = preproc_itf_set_roi;
	itf->preproc_itf_ops.set_power_mode = preproc_itf_set_power_mode;
	itf->preproc_itf_ops.change_config = preproc_itf_change_config;
	itf->preproc_itf_ops.read_af_stat = preproc_itf_read_af_stat;
	itf->preproc_itf_ops.read_af_reg = preproc_itf_read_af_reg;
    	itf->preproc_itf_ops.get_ae_gains = preproc_itf_get_ae_gains;

	return ret;
}

