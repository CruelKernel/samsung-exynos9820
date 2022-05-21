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
#include "fimc-is-interface-sensor.h"
#if defined(CONFIG_CAMERA_PAFSTAT)
#include "pafstat/fimc-is-hw-pafstat.h"
#endif
#include "fimc-is-vender-specific.h"

static u8 rta_static_data[STATIC_DATA_SIZE];
static u8 ddk_static_data[STATIC_DATA_SIZE];

/* helper functions */
static struct fimc_is_module_enum *get_subdev_module_enum(struct fimc_is_sensor_interface *itf)
{
	struct fimc_is_device_sensor_peri *sensor_peri;

	if (unlikely(!itf)) {
		err("invalid sensor interface");
		return NULL;
	}

	FIMC_BUG_NULL(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri,
			sensor_interface);
	if (!sensor_peri) {
		err("failed to get sensor_peri");
		return NULL;
	}

	return sensor_peri->module;
}

static struct fimc_is_device_sensor *get_device_sensor(struct fimc_is_sensor_interface *itf)
{
	struct fimc_is_module_enum *module;
	struct v4l2_subdev *subdev_module;

	module = get_subdev_module_enum(itf);
	if (unlikely(!module)) {
		err("failed to get sensor_peri's module");
		return NULL;
	}

	subdev_module = module->subdev;
	if (!subdev_module) {
		err("module's subdev was not probed");
		return NULL;
	}

	return (struct fimc_is_device_sensor *)
			v4l2_get_subdev_hostdata(subdev_module);
}

static struct fimc_is_device_csi *get_subdev_csi(struct fimc_is_sensor_interface *itf)
{
	struct fimc_is_device_sensor *device;

	device = get_device_sensor(itf);
	if (!device) {
		err("failed to get sensor device");
		return NULL;
	}

	return (struct fimc_is_device_csi *)
			v4l2_get_subdevdata(device->subdev_csi);
}

struct fimc_is_actuator *get_subdev_actuator(struct fimc_is_sensor_interface *itf)
{
	struct fimc_is_device_sensor_peri *sensor_peri;

	if (unlikely(!itf)) {
		err("invalid sensor interface");
		return NULL;
	}

	FIMC_BUG_NULL(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri,
			sensor_interface);
	if (!sensor_peri) {
		err("failed to get sensor_peri");
		return NULL;
	}

	return (struct fimc_is_actuator *)
			v4l2_get_subdevdata(sensor_peri->subdev_actuator);

}

int sensor_get_ctrl(struct fimc_is_sensor_interface *itf,
			u32 ctrl_id, u32 *val)
{
	int ret = 0;
	struct fimc_is_device_sensor *device = NULL;
	struct fimc_is_module_enum *module = NULL;
	struct v4l2_subdev *subdev_module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct v4l2_control ctrl;

	if (unlikely(!itf)) {
		err("interface in is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);
	FIMC_BUG(!val);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	module = sensor_peri->module;
	if (unlikely(!module)) {
		err("module in is NULL");
		module = NULL;
		goto p_err;
	}

	subdev_module = module->subdev;
	if (!subdev_module) {
		err("module is not probed");
		subdev_module = NULL;
		goto p_err;
	}

	device = v4l2_get_subdev_hostdata(subdev_module);
	if (unlikely(!device)) {
		err("device in is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ctrl.id = ctrl_id;
	ctrl.value = -1;
	ret = fimc_is_sensor_g_ctrl(device, &ctrl);
	*val = (u32)ctrl.value;
	if (ret < 0) {
		err("err!!! ret(%d), return_value(%d)", ret, *val);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

/* if target param has only one value(such as CIS_SMIA mode or flash_intensity),
   then set value to long_val, will be ignored short_val */
int set_interface_param(struct fimc_is_sensor_interface *itf,
			enum itf_cis_interface mode,
			enum itf_param_type target,
			u32 index,
			u32 long_val,
			u32 short_val)
{
	int ret = 0;
	u32 val[MAX_EXPOSURE_GAIN_PER_FRAME] = {0};

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	if (mode == ITF_CIS_SMIA) {
		val[EXPOSURE_GAIN_INDEX] = long_val;
		val[LONG_EXPOSURE_GAIN_INDEX] = 0;
		val[SHORT_EXPOSURE_GAIN_INDEX] = 0;
	} else if (mode == ITF_CIS_SMIA_WDR) {
		val[EXPOSURE_GAIN_INDEX] = 0;
		val[LONG_EXPOSURE_GAIN_INDEX] = long_val;;
		val[SHORT_EXPOSURE_GAIN_INDEX] = short_val;
	} else {
		pr_err("[%s] invalid mode (%d)\n", __func__, mode);
		ret = -EINVAL;
		goto p_err;
	}

	if (index >= NUM_FRAMES) {
		pr_err("[%s] invalid frame index (%d)\n", __func__, index);
		ret = -EINVAL;
		goto p_err;
	}

	switch (target) {
	case ITF_CIS_PARAM_TOTAL_GAIN:
		itf->total_gain[EXPOSURE_GAIN_INDEX][index] = val[EXPOSURE_GAIN_INDEX];
		itf->total_gain[LONG_EXPOSURE_GAIN_INDEX][index] = val[LONG_EXPOSURE_GAIN_INDEX];
		itf->total_gain[SHORT_EXPOSURE_GAIN_INDEX][index] = val[SHORT_EXPOSURE_GAIN_INDEX];
		dbg_sensor(1, "%s: total gain[%d] %d %d %d\n", __func__, index,
				val[EXPOSURE_GAIN_INDEX],
				val[LONG_EXPOSURE_GAIN_INDEX],
				val[SHORT_EXPOSURE_GAIN_INDEX]);
		dbg_sensor(1, "%s: total gain [0]:%d [1]:%d [2]:%d\n", __func__,
				itf->total_gain[LONG_EXPOSURE_GAIN_INDEX][0],
				itf->total_gain[LONG_EXPOSURE_GAIN_INDEX][1],
				itf->total_gain[LONG_EXPOSURE_GAIN_INDEX][2]);
		break;
	case ITF_CIS_PARAM_ANALOG_GAIN:
		itf->analog_gain[EXPOSURE_GAIN_INDEX][index] = val[EXPOSURE_GAIN_INDEX];
		itf->analog_gain[LONG_EXPOSURE_GAIN_INDEX][index] = val[LONG_EXPOSURE_GAIN_INDEX];
		itf->analog_gain[SHORT_EXPOSURE_GAIN_INDEX][index] = val[SHORT_EXPOSURE_GAIN_INDEX];
		dbg_sensor(1, "%s: again[%d] %d %d %d\n", __func__, index,
				val[EXPOSURE_GAIN_INDEX],
				val[LONG_EXPOSURE_GAIN_INDEX],
				val[SHORT_EXPOSURE_GAIN_INDEX]);
		dbg_sensor(1, "%s: again [0]:%d [1]:%d [2]:%d\n", __func__,
				itf->analog_gain[LONG_EXPOSURE_GAIN_INDEX][0],
				itf->analog_gain[LONG_EXPOSURE_GAIN_INDEX][1],
				itf->analog_gain[LONG_EXPOSURE_GAIN_INDEX][2]);
		break;
	case ITF_CIS_PARAM_DIGITAL_GAIN:
		itf->digital_gain[EXPOSURE_GAIN_INDEX][index] = val[EXPOSURE_GAIN_INDEX];
		itf->digital_gain[LONG_EXPOSURE_GAIN_INDEX][index] = val[LONG_EXPOSURE_GAIN_INDEX];
		itf->digital_gain[SHORT_EXPOSURE_GAIN_INDEX][index] = val[SHORT_EXPOSURE_GAIN_INDEX];
		dbg_sensor(1, "%s: dgain[%d] %d %d %d\n", __func__, index,
				val[EXPOSURE_GAIN_INDEX],
				val[LONG_EXPOSURE_GAIN_INDEX],
				val[SHORT_EXPOSURE_GAIN_INDEX]);
		dbg_sensor(1, "%s: dgain [0]:%d [1]:%d [2]:%d\n", __func__,
				itf->digital_gain[LONG_EXPOSURE_GAIN_INDEX][0],
				itf->digital_gain[LONG_EXPOSURE_GAIN_INDEX][1],
				itf->digital_gain[LONG_EXPOSURE_GAIN_INDEX][2]);
		break;
	case ITF_CIS_PARAM_EXPOSURE:
		itf->exposure[EXPOSURE_GAIN_INDEX][index] = val[EXPOSURE_GAIN_INDEX];
		itf->exposure[LONG_EXPOSURE_GAIN_INDEX][index] = val[LONG_EXPOSURE_GAIN_INDEX];
		itf->exposure[SHORT_EXPOSURE_GAIN_INDEX][index] = val[SHORT_EXPOSURE_GAIN_INDEX];
		dbg_sensor(1, "%s: expo[%d] %d %d %d\n", __func__, index,
				val[EXPOSURE_GAIN_INDEX],
				val[LONG_EXPOSURE_GAIN_INDEX],
				val[SHORT_EXPOSURE_GAIN_INDEX]);
		dbg_sensor(1, "%s: expo [0]:%d [1]:%d [2]:%d\n", __func__,
				itf->exposure[LONG_EXPOSURE_GAIN_INDEX][0],
				itf->exposure[LONG_EXPOSURE_GAIN_INDEX][1],
				itf->exposure[LONG_EXPOSURE_GAIN_INDEX][2]);
		break;
	case ITF_CIS_PARAM_FLASH_INTENSITY:
		itf->flash_intensity[index] = long_val;
		break;
	default:
		pr_err("[%s] invalid CIS_SMIA mode (%d)\n", __func__, mode);
		ret = -EINVAL;
		goto p_err;
		break;
	}

p_err:
	return ret;
}

int get_interface_param(struct fimc_is_sensor_interface *itf,
			enum itf_cis_interface mode,
			enum itf_param_type target,
			u32 index,
			u32 *long_val,
			u32 *short_val)
{
	int ret = 0;
	u32 val[MAX_EXPOSURE_GAIN_PER_FRAME] = {0};

	FIMC_BUG(!itf);
	FIMC_BUG(!long_val);
	FIMC_BUG(!short_val);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	if (index >= NUM_FRAMES) {
		pr_err("[%s] invalid frame index (%d)\n", __func__, index);
		ret = -EINVAL;
		goto p_err;
	}

	switch (target) {
	case ITF_CIS_PARAM_TOTAL_GAIN:
		val[EXPOSURE_GAIN_INDEX] = itf->total_gain[EXPOSURE_GAIN_INDEX][index];
		val[LONG_EXPOSURE_GAIN_INDEX] = itf->total_gain[LONG_EXPOSURE_GAIN_INDEX][index];
		val[SHORT_EXPOSURE_GAIN_INDEX] = itf->total_gain[SHORT_EXPOSURE_GAIN_INDEX][index];
		dbg_sensor(2, "%s: total gain[%d] %d %d %d\n", __func__, index,
			val[EXPOSURE_GAIN_INDEX],
			val[LONG_EXPOSURE_GAIN_INDEX],
			val[SHORT_EXPOSURE_GAIN_INDEX]);
		break;
	case ITF_CIS_PARAM_ANALOG_GAIN:
		val[EXPOSURE_GAIN_INDEX] = itf->analog_gain[EXPOSURE_GAIN_INDEX][index];
		val[LONG_EXPOSURE_GAIN_INDEX] = itf->analog_gain[LONG_EXPOSURE_GAIN_INDEX][index];
		val[SHORT_EXPOSURE_GAIN_INDEX] = itf->analog_gain[SHORT_EXPOSURE_GAIN_INDEX][index];
		dbg_sensor(2, "%s: again[%d] %d %d %d\n", __func__, index,
			val[EXPOSURE_GAIN_INDEX],
			val[LONG_EXPOSURE_GAIN_INDEX],
			val[SHORT_EXPOSURE_GAIN_INDEX]);
		break;
	case ITF_CIS_PARAM_DIGITAL_GAIN:
		val[EXPOSURE_GAIN_INDEX] = itf->digital_gain[EXPOSURE_GAIN_INDEX][index];
		val[LONG_EXPOSURE_GAIN_INDEX] = itf->digital_gain[LONG_EXPOSURE_GAIN_INDEX][index];
		val[SHORT_EXPOSURE_GAIN_INDEX] = itf->digital_gain[SHORT_EXPOSURE_GAIN_INDEX][index];
		dbg_sensor(2, "%s: dgain[%d] %d %d %d\n", __func__, index,
			val[EXPOSURE_GAIN_INDEX],
			val[LONG_EXPOSURE_GAIN_INDEX],
			val[SHORT_EXPOSURE_GAIN_INDEX]);
		break;
	case ITF_CIS_PARAM_EXPOSURE:
		val[EXPOSURE_GAIN_INDEX] = itf->exposure[EXPOSURE_GAIN_INDEX][index];
		val[LONG_EXPOSURE_GAIN_INDEX] = itf->exposure[LONG_EXPOSURE_GAIN_INDEX][index];
		val[SHORT_EXPOSURE_GAIN_INDEX] = itf->exposure[SHORT_EXPOSURE_GAIN_INDEX][index];
		dbg_sensor(2, "%s: exposure[%d] %d %d %d\n", __func__, index,
			val[EXPOSURE_GAIN_INDEX],
			val[LONG_EXPOSURE_GAIN_INDEX],
			val[SHORT_EXPOSURE_GAIN_INDEX]);
		break;
	case ITF_CIS_PARAM_FLASH_INTENSITY:
		val[EXPOSURE_GAIN_INDEX] = itf->flash_intensity[index];
		val[LONG_EXPOSURE_GAIN_INDEX] = itf->flash_intensity[index];
		break;
	default:
		pr_err("[%s] invalid CIS_SMIA mode (%d)\n", __func__, mode);
		ret = -EINVAL;
		goto p_err;
		break;
	}

	if (mode == ITF_CIS_SMIA) {
		*long_val = val[EXPOSURE_GAIN_INDEX];
	} else if (mode == ITF_CIS_SMIA_WDR) {
		*long_val = val[LONG_EXPOSURE_GAIN_INDEX];
		*short_val = val[SHORT_EXPOSURE_GAIN_INDEX];
	} else {
		pr_err("[%s] invalid mode (%d)\n", __func__, mode);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

u32 get_vsync_count(struct fimc_is_sensor_interface *itf);

u32 get_frame_count(struct fimc_is_sensor_interface *itf)
{
	u32 frame_count = 0;
#if !defined(CONFIG_USE_SENSOR_GROUP)
	struct fimc_is_device_sensor *device = NULL;
	struct fimc_is_module_enum *module = NULL;
	struct v4l2_subdev *subdev_module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
#endif

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

#if !defined(CONFIG_USE_SENSOR_GROUP)
	if (itf->otf_flag_3aa == true) {
		frame_count = get_vsync_count(itf);
	} else {
		/* Get 3AA active count */
		sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
		FIMC_BUG(!sensor_peri);

		module = sensor_peri->module;
		if (unlikely(!module)) {
			err("module in is NULL");
			module = NULL;
			return 0;
		}

		subdev_module = module->subdev;
		if (!subdev_module) {
			err("module is not probed");
			subdev_module = NULL;
			return 0;
		}

		device = v4l2_get_subdev_hostdata(subdev_module);
		if (unlikely(!device)) {
			err("device in is NULL");
			return 0;
		}

		frame_count = device->ischain->group_3aa.fcount;
	}
#else
	frame_count = get_vsync_count(itf);
#endif

	/* Frame count have to start at 1 */
	if (frame_count == 0)
		frame_count = 1;

	return frame_count;
}

struct fimc_is_sensor_ctl *get_sensor_ctl_from_module(struct fimc_is_sensor_interface *itf,
							u32 frame_count)
{
	u32 index = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG_NULL(!itf);
	FIMC_BUG_NULL(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG_NULL(!sensor_peri);

	index = frame_count % CAM2P0_UCTL_LIST_SIZE;

	return &sensor_peri->cis.sensor_ctls[index];
}

camera2_sensor_uctl_t *get_sensor_uctl_from_module(struct fimc_is_sensor_interface *itf,
							u32 frame_count)
{
	struct fimc_is_sensor_ctl *sensor_ctl = NULL;

	FIMC_BUG_NULL(!itf);
	FIMC_BUG_NULL(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_ctl = get_sensor_ctl_from_module(itf, frame_count);

	/* TODO: will be moved to report_sensor_done() */
	sensor_ctl->sensor_frame_number = frame_count;

	return &sensor_ctl->cur_cam20_sensor_udctrl;
}

void set_sensor_uctl_valid(struct fimc_is_sensor_interface *itf,
						u32 frame_count)
{
	struct fimc_is_sensor_ctl *sensor_ctl = NULL;

	FIMC_BUG_VOID(!itf);
	FIMC_BUG_VOID(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_ctl = get_sensor_ctl_from_module(itf, frame_count);

	sensor_ctl->is_valid_sensor_udctrl = true;
}

int get_num_of_frame_per_one_3aa(struct fimc_is_sensor_interface *itf,
				u32 *num_of_frame);
int set_exposure(struct fimc_is_sensor_interface *itf,
		enum itf_cis_interface mode,
		u32 long_exp,
		u32 short_exp)
{
	int ret = 0;
	u32 frame_count = 0;
	camera2_sensor_uctl_t *sensor_uctl;
	u32 i = 0;
	u32 num_of_frame = 1;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	frame_count = get_frame_count(itf);
	ret = get_num_of_frame_per_one_3aa(itf, &num_of_frame);

	for (i = 0; i < num_of_frame; i++) {
		sensor_uctl = get_sensor_uctl_from_module(itf, frame_count + i);
		FIMC_BUG(!sensor_uctl);

		/* set exposure */
		sensor_uctl->exposureTime = fimc_is_sensor_convert_us_to_ns(long_exp);
		if (mode == ITF_CIS_SMIA_WDR) {
			sensor_uctl->longExposureTime = fimc_is_sensor_convert_us_to_ns(long_exp);
			sensor_uctl->shortExposureTime = fimc_is_sensor_convert_us_to_ns(short_exp);
		}
		set_sensor_uctl_valid(itf, frame_count);
	}

	return ret;
}

int set_gain_permile(struct fimc_is_sensor_interface *itf,
		enum itf_cis_interface mode,
		u32 long_total_gain, u32 short_total_gain,
		u32 long_analog_gain, u32 short_analog_gain,
		u32 long_digital_gain, u32 short_digital_gain)
{
	int ret = 0;
	u32 frame_count = 0;
	camera2_sensor_uctl_t *sensor_uctl;
	u32 i = 0;
	u32 num_of_frame = 1;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	frame_count = get_frame_count(itf);

	ret = get_num_of_frame_per_one_3aa(itf, &num_of_frame);

	for (i = 0; i < num_of_frame; i++) {
		sensor_uctl = get_sensor_uctl_from_module(itf, frame_count + i);
		FIMC_BUG(!sensor_uctl);

		/* set exposure */
		if (mode == ITF_CIS_SMIA) {
			sensor_uctl->sensitivity = DIV_ROUND_UP(long_total_gain, 10);

			sensor_uctl->analogGain = long_analog_gain;
			sensor_uctl->digitalGain = long_digital_gain;

			set_sensor_uctl_valid(itf, frame_count);
		} else if (mode == ITF_CIS_SMIA_WDR) {
			sensor_uctl->sensitivity = DIV_ROUND_UP(long_total_gain, 10);

			/* Caution: short values are setted at analog/digital gain */
			sensor_uctl->analogGain = short_analog_gain;
			sensor_uctl->digitalGain = short_digital_gain;
			sensor_uctl->longAnalogGain = long_analog_gain;
			sensor_uctl->shortAnalogGain = short_analog_gain;
			sensor_uctl->longDigitalGain = long_digital_gain;
			sensor_uctl->shortDigitalGain = short_digital_gain;

			set_sensor_uctl_valid(itf, frame_count);
		} else {
			pr_err("invalid cis interface mode (%d)\n", mode);
			ret = -EINVAL;
		}
	}

	return ret;
}

/* new APIs */
int request_reset_interface(struct fimc_is_sensor_interface *itf,
				u32 exposure,
				u32 total_gain,
				u32 analog_gain,
				u32 digital_gain)
{
	int ret = 0;
	u32 i = 0;
	u32 end_index = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	dbg_sensor(1, "[%s] exposure(%d), total_gain(%d), a-gain(%d), d-gain(%d)\n", __func__,
			exposure, total_gain, analog_gain, digital_gain);

	itf->vsync_flag = false;
	end_index = itf->otf_flag_3aa == true ? NEXT_NEXT_FRAME_OTF : NEXT_NEXT_FRAME_DMA;

	for (i = 0; i <= end_index; i++) {
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_TOTAL_GAIN, i, total_gain, total_gain);
		if (ret < 0)
			pr_err("[%s] set_interface_param TOTAL_GAIN fail(%d)\n", __func__, ret);
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_ANALOG_GAIN, i, analog_gain, analog_gain);
		if (ret < 0)
			pr_err("[%s] set_interface_param ANALOG_GAIN fail(%d)\n", __func__, ret);
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_DIGITAL_GAIN, i, digital_gain, digital_gain);
		if (ret < 0)
			pr_err("[%s] set_interface_param DIGITAL_GAIN fail(%d)\n", __func__, ret);
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_EXPOSURE, i, exposure, exposure);
		if (ret < 0)
			pr_err("[%s] set_interface_param EXPOSURE fail(%d)\n", __func__, ret);
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_FLASH_INTENSITY, i, 0, 0);
		if (ret < 0)
			pr_err("[%s] set_interface_param FLASH_INTENSITY fail(%d)\n", __func__, ret);
	}

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	fimc_is_sensor_set_cis_uctrl_list(sensor_peri,
			exposure,
			exposure,
			total_gain, total_gain,
			analog_gain, analog_gain,
			digital_gain, digital_gain);

	memset(sensor_peri->cis.cis_data->auto_exposure, 0, sizeof(sensor_peri->cis.cis_data->auto_exposure));

	return ret;
}

int get_calibrated_size(struct fimc_is_sensor_interface *itf,
			u32 *width,
			u32 *height)
{
	int ret = 0;
	struct fimc_is_module_enum *module = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!width);
	FIMC_BUG(!height);

	module = get_subdev_module_enum(itf);
	FIMC_BUG(!module);

	*width = module->pixel_width;
	*height = module->pixel_height;

	pr_debug("%s, width(%d), height(%d)\n", __func__, *width, *height);

	return ret;
}

int get_bayer_order(struct fimc_is_sensor_interface *itf,
			u32 *bayer_order)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!bayer_order);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	*bayer_order = sensor_peri->cis.bayer_order;

	return ret;
}

u32 get_min_exposure_time(struct fimc_is_sensor_interface *itf)
{
	int ret = 0;
	u32 exposure = 0;

	FIMC_BUG(!itf);

	ret = sensor_get_ctrl(itf, V4L2_CID_SENSOR_GET_MIN_EXPOSURE_TIME, &exposure);
	if (ret < 0 || exposure == 0) {
		err("err!!! ret(%d), return_value(%d)", ret, exposure);
		goto p_err;
	}

	dbg_sensor(2, "%s:(%d:%d) min exp(%d)\n", __func__, get_vsync_count(itf), get_frame_count(itf), exposure);

p_err:
	return exposure;
}

u32 get_max_exposure_time(struct fimc_is_sensor_interface *itf)
{
	int ret = 0;
	u32 exposure = 0;

	FIMC_BUG(!itf);

	ret = sensor_get_ctrl(itf, V4L2_CID_SENSOR_GET_MAX_EXPOSURE_TIME, &exposure);
	if (ret < 0 || exposure == 0) {
		err("err!!! ret(%d), return_value(%d)", ret, exposure);
		goto p_err;
	}

	dbg_sensor(2, "%s:(%d:%d) max exp(%d)\n", __func__, get_vsync_count(itf), get_frame_count(itf), exposure);

p_err:
	return exposure;
}

u32 get_min_analog_gain(struct fimc_is_sensor_interface *itf)
{
	int ret = 0;
	u32 again = 0;

	FIMC_BUG(!itf);

	ret = sensor_get_ctrl(itf, V4L2_CID_SENSOR_GET_MIN_ANALOG_GAIN, &again);
	if (ret < 0 || again == 0) {
		err("err!!! ret(%d), return_value(%d)", ret, again);
		goto p_err;
	}

	dbg_sensor(2, "%s:(%d:%d) min analog gain(%d)\n", __func__, get_vsync_count(itf), get_frame_count(itf), again);

p_err:
	return again;
}

u32 get_max_analog_gain(struct fimc_is_sensor_interface *itf)
{
	int ret = 0;
	u32 again = 0;

	FIMC_BUG(!itf);

	ret = sensor_get_ctrl(itf, V4L2_CID_SENSOR_GET_MAX_ANALOG_GAIN, &again);
	if (ret < 0 || again == 0) {
		err("err!!! ret(%d), return_value(%d)", ret, again);
		goto p_err;
	}

	dbg_sensor(2, "%s:(%d:%d) max analog gain(%d)\n", __func__, get_vsync_count(itf), get_frame_count(itf), again);

p_err:
	return again;
}

u32 get_min_digital_gain(struct fimc_is_sensor_interface *itf)
{
	int ret = 0;
	u32 dgain = 0;

	FIMC_BUG(!itf);

	ret = sensor_get_ctrl(itf, V4L2_CID_SENSOR_GET_MIN_DIGITAL_GAIN, &dgain);
	if (ret < 0 || dgain == 0) {
		err("err!!! ret(%d), return_value(%d)", ret, dgain);
		goto p_err;
	}

	dbg_sensor(2, "%s:(%d:%d) min digital gain(%d)\n", __func__, get_vsync_count(itf), get_frame_count(itf), dgain);

p_err:
	return dgain;
}

u32 get_max_digital_gain(struct fimc_is_sensor_interface *itf)
{
	int ret = 0;
	u32 dgain = 0;

	FIMC_BUG(!itf);

	ret = sensor_get_ctrl(itf, V4L2_CID_SENSOR_GET_MAX_DIGITAL_GAIN, &dgain);
	if (ret < 0 || dgain == 0) {
		err("err!!! ret(%d), return_value(%d)", ret, dgain);
		goto p_err;
	}

	dbg_sensor(2, "%s:(%d:%d) max digital gain(%d)\n", __func__, get_vsync_count(itf), get_frame_count(itf), dgain);

p_err:
	return dgain;
}


u32 get_vsync_count(struct fimc_is_sensor_interface *itf)
{
	struct fimc_is_device_csi *csi;

	FIMC_BUG(!itf);

	csi = get_subdev_csi(itf);
	FIMC_BUG(!csi);

	return atomic_read(&csi->fcount);
}

u32 get_vblank_count(struct fimc_is_sensor_interface *itf)
{
	struct fimc_is_device_csi *csi;

	FIMC_BUG(!itf);

	csi = get_subdev_csi(itf);
	FIMC_BUG(!csi);

	return atomic_read(&csi->vblank_count);
}

bool is_vvalid_period(struct fimc_is_sensor_interface *itf)
{
	struct fimc_is_device_csi *csi;

	FIMC_BUG_FALSE(!itf);

	csi = get_subdev_csi(itf);
	FIMC_BUG_FALSE(!csi);

	return atomic_read(&csi->vvalid) <= 0 ? false : true;
}

int request_exposure(struct fimc_is_sensor_interface *itf,
			u32 long_exposure,
			u32 short_exposure)
{
	int ret = 0;
	u32 i = 0;
	u32 end_index = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	dbg_sensor(1, "[%s](%d:%d) long_exposure(%d), short_exposure(%d)\n", __func__,
		get_vsync_count(itf), get_frame_count(itf), long_exposure, short_exposure);

	end_index = (itf->otf_flag_3aa == true ? NEXT_NEXT_FRAME_OTF : NEXT_NEXT_FRAME_DMA);

	i = (itf->vsync_flag == false ? 0 : end_index);
	for (; i <= end_index; i++) {
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_EXPOSURE, i, long_exposure, short_exposure);
		if (ret < 0) {
			pr_err("[%s] set_interface_param EXPOSURE fail(%d)\n", __func__, ret);
			goto p_err;
		}
	}
	itf->vsync_flag = true;

	/* set exposure */
	if (itf->otf_flag_3aa == true) {
		ret = set_exposure(itf, itf->cis_mode, long_exposure, short_exposure);
		if (ret < 0) {
			pr_err("[%s] set_exposure fail(%d)\n", __func__, ret);
			goto p_err;
		}
	}

	/* store exposure for use initial AE */
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	if (!sensor_peri) {
		err("[%s] sensor_peri is NULL", __func__);
		return -EINVAL;
	}

	if (sensor_peri->cis.use_initial_ae) {
		sensor_peri->cis.last_ae_setting.long_exposure = long_exposure;
		sensor_peri->cis.last_ae_setting.exposure = short_exposure;
	}
p_err:
	return ret;
}

int adjust_exposure(struct fimc_is_sensor_interface *itf,
			u32 long_exposure,
			u32 short_exposure,
			u32 *available_long_exposure,
			u32 *available_short_exposure,
			fimc_is_sensor_adjust_direction adjust_direction)
{
	/* NOT IMPLEMENTED YET */
	int ret = -1;

	dbg_sensor(1, "[%s] NOT IMPLEMENTED YET\n", __func__);

	return ret;
}

int get_next_frame_timing(struct fimc_is_sensor_interface *itf,
			u32 *long_exposure,
			u32 *short_exposure,
			u32 *frame_period,
			u64 *line_period)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!long_exposure);
	FIMC_BUG(!short_exposure);
	FIMC_BUG(!frame_period);
	FIMC_BUG(!line_period);

	ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_EXPOSURE, NEXT_FRAME, long_exposure, short_exposure);
	if (ret < 0)
		pr_err("[%s] get_interface_param EXPOSURE fail(%d)\n", __func__, ret);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	*frame_period = sensor_peri->cis.cis_data->frame_time;
	*line_period = sensor_peri->cis.cis_data->line_readOut_time;

	dbg_sensor(2, "%s:(%d:%d) exp(%d, %d), frame_period %d, line_period %lld\n", __func__,
		get_vsync_count(itf), get_frame_count(itf),
		*long_exposure, *short_exposure, *frame_period, *line_period);

	return ret;
}

int get_frame_timing(struct fimc_is_sensor_interface *itf,
			u32 *long_exposure,
			u32 *short_exposure,
			u32 *frame_period,
			u64 *line_period)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!long_exposure);
	FIMC_BUG(!short_exposure);
	FIMC_BUG(!frame_period);
	FIMC_BUG(!line_period);

	ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_EXPOSURE, CURRENT_FRAME, long_exposure, short_exposure);
	if (ret < 0)
		pr_err("[%s] get_interface_param EXPOSURE fail(%d)\n", __func__, ret);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	*frame_period = sensor_peri->cis.cis_data->frame_time;
	*line_period = sensor_peri->cis.cis_data->line_readOut_time;

	dbg_sensor(2, "%s:(%d:%d) exp(%d, %d), frame_period %d, line_period %lld\n", __func__,
		get_vsync_count(itf), get_frame_count(itf),
		*long_exposure, *short_exposure, *frame_period, *line_period);

	return ret;
}

int request_analog_gain(struct fimc_is_sensor_interface *itf,
			u32 long_analog_gain,
			u32 short_analog_gain)
{
	int ret = 0;
	u32 i = 0;
	u32 end_index = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	dbg_sensor(1, "[%s](%d:%d) long_analog_gain(%d), short_analog_gain(%d)\n", __func__,
		get_vsync_count(itf), get_frame_count(itf), long_analog_gain, short_analog_gain);

	end_index = (itf->otf_flag_3aa == true ? NEXT_NEXT_FRAME_OTF : NEXT_NEXT_FRAME_DMA);

	i = (itf->vsync_flag == false ? 0 : end_index);
	for (; i <= end_index; i++) {
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_ANALOG_GAIN, i, long_analog_gain, short_analog_gain);
		if (ret < 0) {
			pr_err("[%s] set_interface_param EXPOSURE fail(%d)\n", __func__, ret);
			goto p_err;
		}
	}

p_err:
	return ret;
}

int request_gain(struct fimc_is_sensor_interface *itf,
		u32 long_total_gain,
		u32 long_analog_gain,
		u32 long_digital_gain,
		u32 short_total_gain,
		u32 short_analog_gain,
		u32 short_digital_gain)
{
	int ret = 0;
	u32 i = 0;
	u32 end_index = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	dbg_sensor(1, "[%s](%d:%d) long_total_gain(%d), short_total_gain(%d)\n", __func__,
		get_vsync_count(itf), get_frame_count(itf), long_total_gain, short_total_gain);
	dbg_sensor(1, "[%s](%d:%d) long_analog_gain(%d), short_analog_gain(%d)\n", __func__,
		get_vsync_count(itf), get_frame_count(itf), long_analog_gain, short_analog_gain);
	dbg_sensor(1, "[%s](%d:%d) long_digital_gain(%d), short_digital_gain(%d)\n", __func__,
		get_vsync_count(itf), get_frame_count(itf), long_digital_gain, short_digital_gain);

	end_index = (itf->otf_flag_3aa == true ? NEXT_NEXT_FRAME_OTF : NEXT_NEXT_FRAME_DMA);

	i = (itf->vsync_flag == false ? 0 : end_index);
	for (; i <= end_index; i++) {
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_TOTAL_GAIN, i, long_total_gain, short_total_gain);
		if (ret < 0) {
			pr_err("[%s] set_interface_param EXPOSURE fail(%d)\n", __func__, ret);
			goto p_err;
		}
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_ANALOG_GAIN, i, long_analog_gain, short_analog_gain);
		if (ret < 0) {
			pr_err("[%s] set_interface_param EXPOSURE fail(%d)\n", __func__, ret);
			goto p_err;
		}
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_DIGITAL_GAIN, i, long_digital_gain, short_digital_gain);
		if (ret < 0) {
			pr_err("[%s] set_interface_param EXPOSURE fail(%d)\n", __func__, ret);
			goto p_err;
		}
	}

	/* set gain permile */
	if (itf->otf_flag_3aa == true) {
		ret = set_gain_permile(itf, itf->cis_mode,
				long_total_gain, short_total_gain,
				long_analog_gain, short_analog_gain,
				long_digital_gain, short_digital_gain);
		if (ret < 0) {
			pr_err("[%s] set_gain_permile fail(%d)\n", __func__, ret);
			goto p_err;
		}
	}

	/* store gain for use initial AE */
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	if (!sensor_peri) {
		err("[%s] sensor_peri is NULL", __func__);
		return -EINVAL;
	}

	if (sensor_peri->cis.use_initial_ae) {
		sensor_peri->cis.last_ae_setting.long_analog_gain = long_analog_gain;
		sensor_peri->cis.last_ae_setting.long_digital_gain = long_digital_gain;
		sensor_peri->cis.last_ae_setting.analog_gain = short_analog_gain;
		sensor_peri->cis.last_ae_setting.digital_gain = short_digital_gain;
	}
p_err:
	return ret;
}

int request_sensitivity(struct fimc_is_sensor_interface *itf,
		u32 sensitivity)
{
	int ret = 0;
	u32 frame_count = 0;
	camera2_sensor_uctl_t *sensor_uctl = NULL;
	u32 i = 0;
	u32 num_of_frame = 1;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	frame_count = get_frame_count(itf);

	ret = get_num_of_frame_per_one_3aa(itf, &num_of_frame);

	/* set sensitivity  */
	if (itf->otf_flag_3aa == true) {
		for (i = 0; i < num_of_frame; i++) {
			sensor_uctl = get_sensor_uctl_from_module(itf, frame_count + i);
			FIMC_BUG(!sensor_uctl);

			sensor_uctl->sensitivity = sensitivity;
		}
	}

	return ret;
}

int set_previous_dm(struct fimc_is_sensor_interface *itf)
{
	int ret = 0;
	u32 frame_count = 0;
	u32 index = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct fimc_is_sensor_ctl *module_ctl = NULL;
	struct camera2_sensor_ctl *sensor_ctl = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	frame_count = get_frame_count(itf);

	/* set previous values */
	index = (frame_count - 1) % CAM2P0_UCTL_LIST_SIZE;
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	module_ctl = &sensor_peri->cis.sensor_ctls[index];
	sensor_ctl = &module_ctl->cur_cam20_sensor_ctrl;

	index = (frame_count + 1) % EXPECT_DM_NUM;
	if(sensor_ctl->sensitivity != 0 && module_ctl->valid_sensor_ctrl == true) {
		if(sensor_ctl->sensitivity != sensor_peri->cis.expecting_sensor_dm[index].sensitivity) {
			sensor_peri->cis.expecting_sensor_dm[index].sensitivity = sensor_ctl->sensitivity;
		}
	}

	if(sensor_ctl->exposureTime != 0 && module_ctl->valid_sensor_ctrl == true) {
		if(sensor_ctl->exposureTime != sensor_peri->cis.expecting_sensor_dm[index].exposureTime) {
			sensor_peri->cis.expecting_sensor_dm[index].exposureTime = sensor_ctl->exposureTime;
		}
	}

	return ret;
}

int adjust_analog_gain(struct fimc_is_sensor_interface *itf,
			u32 desired_long_analog_gain,
			u32 desired_short_analog_gain,
			u32 *actual_long_gain,
			u32 *actual_short_gain,
			fimc_is_sensor_adjust_direction adjust_direction)
{
	/* NOT IMPLEMENTED YET */
	int ret = -1;

	dbg_sensor(1, "[%s] NOT IMPLEMENTED YET\n", __func__);

	return ret;
}

int get_next_analog_gain(struct fimc_is_sensor_interface *itf,
			u32 *long_analog_gain,
			u32 *short_analog_gain)
{
	int ret = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(!long_analog_gain);
	FIMC_BUG(!short_analog_gain);

	ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_ANALOG_GAIN, NEXT_FRAME, long_analog_gain, short_analog_gain);
	if (ret < 0)
		pr_err("[%s] get_interface_param ANALOG_GAIN fail(%d)\n", __func__, ret);

	dbg_sensor(2, "[%s](%d:%d) long_analog_gain(%d), short_analog_gain(%d)\n", __func__,
		get_vsync_count(itf), get_frame_count(itf),
		*long_analog_gain, *short_analog_gain);

	return ret;
}

int get_analog_gain(struct fimc_is_sensor_interface *itf,
			u32 *long_analog_gain,
			u32 *short_analog_gain)
{
	int ret = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(!long_analog_gain);
	FIMC_BUG(!short_analog_gain);

	ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_ANALOG_GAIN, CURRENT_FRAME, long_analog_gain, short_analog_gain);
	if (ret < 0)
		pr_err("[%s] get_interface_param ANALOG_GAIN fail(%d)\n", __func__, ret);

	dbg_sensor(2, "[%s](%d:%d) long_analog_gain(%d), short_analog_gain(%d)\n", __func__,
		get_vsync_count(itf), get_frame_count(itf),
		*long_analog_gain, *short_analog_gain);

	return ret;
}

int get_next_digital_gain(struct fimc_is_sensor_interface *itf,
				u32 *long_digital_gain,
				u32 *short_digital_gain)
{
	int ret = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(!long_digital_gain);
	FIMC_BUG(!short_digital_gain);

	ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_DIGITAL_GAIN, NEXT_FRAME, long_digital_gain, short_digital_gain);
	if (ret < 0)
		pr_err("[%s] get_interface_param DIGITAL_GAIN fail(%d)\n", __func__, ret);

	dbg_sensor(2, "[%s](%d:%d) long_digital_gain(%d), short_digital_gain(%d)\n", __func__,
		get_vsync_count(itf), get_frame_count(itf),
		*long_digital_gain, *short_digital_gain);

	return ret;
}

int get_digital_gain(struct fimc_is_sensor_interface *itf,
			u32 *long_digital_gain,
			u32 *short_digital_gain)
{
	int ret = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(!long_digital_gain);
	FIMC_BUG(!short_digital_gain);

	ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_DIGITAL_GAIN, CURRENT_FRAME, long_digital_gain, short_digital_gain);
	if (ret < 0)
		pr_err("[%s] get_interface_param DIGITAL_GAIN fail(%d)\n", __func__, ret);

	dbg_sensor(2, "[%s](%d:%d) long_digital_gain(%d), short_digital_gain(%d)\n", __func__,
		get_vsync_count(itf), get_frame_count(itf),
		*long_digital_gain, *short_digital_gain);

	return ret;
}

bool is_actuator_available(struct fimc_is_sensor_interface *itf)
{
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG_FALSE(!itf);
	FIMC_BUG_FALSE(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	return test_bit(FIMC_IS_SENSOR_ACTUATOR_AVAILABLE, &sensor_peri->peri_state);
}

bool is_flash_available(struct fimc_is_sensor_interface *itf)
{
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG_FALSE(!itf);
	FIMC_BUG_FALSE(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	dbg_flash("flash available (%d)\n",
		test_bit(FIMC_IS_SENSOR_FLASH_AVAILABLE, &sensor_peri->peri_state));
	return test_bit(FIMC_IS_SENSOR_FLASH_AVAILABLE, &sensor_peri->peri_state);
}

bool is_companion_available(struct fimc_is_sensor_interface *itf)
{
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG_FALSE(!itf);
	FIMC_BUG_FALSE(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	return test_bit(FIMC_IS_SENSOR_PREPROCESSOR_AVAILABLE, &sensor_peri->peri_state);
}

bool is_ois_available(struct fimc_is_sensor_interface *itf)
{
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG_FALSE(!itf);
	FIMC_BUG_FALSE(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	return test_bit(FIMC_IS_SENSOR_OIS_AVAILABLE, &sensor_peri->peri_state);
}

bool is_aperture_available(struct fimc_is_sensor_interface *itf)
{
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG_FALSE(!itf);
	FIMC_BUG_FALSE(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	return test_bit(FIMC_IS_SENSOR_APERTURE_AVAILABLE, &sensor_peri->peri_state);
}

int get_sensor_frame_timing(struct fimc_is_sensor_interface *itf,
			u32 *pclk,
			u32 *line_length_pck,
			u32 *frame_length_lines,
			u32 *max_margin_cit)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri;

	FIMC_BUG(!itf);
	FIMC_BUG(!pclk);
	FIMC_BUG(!line_length_pck);
	FIMC_BUG(!frame_length_lines);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	*pclk = sensor_peri->cis.cis_data->pclk;
	*line_length_pck = sensor_peri->cis.cis_data->line_length_pck;
	*frame_length_lines = sensor_peri->cis.cis_data->frame_length_lines;
	*max_margin_cit = sensor_peri->cis.cis_data->cur_coarse_integration_time_step;

	dbg_sensor(2, "[%s](%d:%d) pclk(%d), line_length_pck(%d), frame_length_lines(%d), max_margin_cit(%d)\n",
		__func__, get_vsync_count(itf), get_frame_count(itf),
		*pclk, *line_length_pck, *frame_length_lines, *max_margin_cit);

	return ret;
}

int get_sensor_cur_size(struct fimc_is_sensor_interface *itf,
			u32 *cur_width,
			u32 *cur_height)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!cur_width);
	FIMC_BUG(!cur_height);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	*cur_width = sensor_peri->cis.cis_data->cur_width;
	*cur_height = sensor_peri->cis.cis_data->cur_height;

	return ret;
}

int get_sensor_max_fps(struct fimc_is_sensor_interface *itf,
			u32 *max_fps)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!max_fps);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	*max_fps = sensor_peri->cis.cis_data->max_fps;

	return ret;
}

int get_sensor_cur_fps(struct fimc_is_sensor_interface *itf,
			u32 *cur_fps)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!cur_fps);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	if (sensor_peri->cis.cis_data->cur_frame_us_time != 0) {
		*cur_fps = (u32)((1 * 1000 * 1000) / sensor_peri->cis.cis_data->cur_frame_us_time);
	} else {
		pr_err("[%s] cur_frame_us_time is ZERO\n", __func__);
		ret = -1;
	}

	return ret;
}

int get_hdr_ratio_ctl_by_again(struct fimc_is_sensor_interface *itf,
			u32 *ctrl_by_again)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!ctrl_by_again);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	*ctrl_by_again = sensor_peri->cis.hdr_ctrl_by_again;

	return ret;
}

int get_sensor_use_dgain(struct fimc_is_sensor_interface *itf,
			u32 *use_dgain)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!use_dgain);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	*use_dgain = sensor_peri->cis.use_dgain;

	return ret;
}

int set_alg_reset_flag(struct fimc_is_sensor_interface *itf,
			bool executed)
{
	int ret = 0;
	struct fimc_is_sensor_ctl *sensor_ctl = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_ctl = get_sensor_ctl_from_module(itf, get_frame_count(itf));

	if (sensor_ctl == NULL) {
		err("get_sensor_ctl_from_module fail!!\n");
		return -1;
	}

	sensor_ctl->alg_reset_flag = executed;

	return ret;
}

/*
 * TODO: need to implement getting C2, C3 stat data
 * This sensor interface returns done status of getting sensor stat
 */
int get_sensor_hdr_stat(struct fimc_is_sensor_interface *itf,
		enum itf_cis_hdr_stat_status *status)
{
	int ret = 0;

	info("%s", __func__);

	return ret;
}

/*
 * TODO: For example, 3AA thumbnail result shuld be applied to sensor in case of IMX230
 */
int set_3a_alg_res_to_sens(struct fimc_is_sensor_interface *itf,
		struct fimc_is_3a_res_to_sensor *sensor_setting)
{
	int ret = 0;

	info("%s", __func__);

	return ret;
}

int get_sensor_initial_aperture(struct fimc_is_sensor_interface *itf,
			u32 *aperture)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	WARN_ON(!itf);
	WARN_ON(!aperture);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	*aperture = sensor_peri->cis.aperture_num;

	return ret;
}

int set_initial_exposure_of_setfile(struct fimc_is_sensor_interface *itf,
				u32 expo)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	dbg_sensor(1, "[%s] init expo (%d)\n", __func__, expo);

	sensor_peri->cis.cis_data->low_expo_start = expo;

	return ret;
}

#ifdef USE_NEW_PER_FRAME_CONTROL
int set_num_of_frame_per_one_3aa(struct fimc_is_sensor_interface *itf,
				u32 *num_of_frame)
{
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!num_of_frame);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	sensor_peri->cis.cis_data->num_of_frame = *num_of_frame;

	dbg_sensor(1, "[%s] num_of_frame (%d)\n", __func__, *num_of_frame);

	return 0;
}

int get_num_of_frame_per_one_3aa(struct fimc_is_sensor_interface *itf,
				u32 *num_of_frame)
{
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!num_of_frame);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	*num_of_frame = sensor_peri->cis.cis_data->num_of_frame;

	return 0;
}

int reserved0(struct fimc_is_sensor_interface *itf,
				bool reserved0)
{
	return 0;
}

int reserved1(struct fimc_is_sensor_interface *itf,
				u32 *reserved1)
{
	return 0;
}
#else

int set_video_mode_of_setfile(struct fimc_is_sensor_interface *itf,
				bool video_mode)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	dbg_sensor(1, "[%s] video mode (%d)\n", __func__, video_mode);

	sensor_peri->cis.cis_data->video_mode = video_mode;

	return ret;
}

int get_num_of_frame_per_one_3aa(struct fimc_is_sensor_interface *itf,
				u32 *num_of_frame)
{
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	u32 max_fps = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(!num_of_frame);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	max_fps = sensor_peri->cis.cis_data->max_fps;

	/* TODO: SDK should know how many frames are needed to execute one 3a. */
	*num_of_frame = NUM_OF_FRAME_30FPS;

	if (sensor_peri->cis.cis_data->video_mode == true) {
		if (max_fps >= 300) {
			*num_of_frame = 10; /* TODO */
		} else if (max_fps >= 240) {
			*num_of_frame = NUM_OF_FRAME_240FPS;
		} else if (max_fps >= 120) {
			*num_of_frame = NUM_OF_FRAME_120FPS;
		} else if (max_fps >= 60) {
			*num_of_frame = NUM_OF_FRAME_60FPS;
		}
	}

	return 0;
}

int get_offset_from_cur_result(struct fimc_is_sensor_interface *itf,
				u32 *offset)
{
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	u32 max_fps = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(!offset);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	max_fps = sensor_peri->cis.cis_data->max_fps;

	/* TODO: SensorSdk delivers the result of the 3AA by taking into account this parameter. */
	*offset = 0;

	if (sensor_peri->cis.cis_data->video_mode == true) {
		if (max_fps >= 300) {
			*offset = 1;
		} else if (max_fps >= 240) {
			*offset = 1;
		} else if (max_fps >= 120) {
			*offset = 1;
		} else if (max_fps >= 60) {
			*offset = 0;
		}
	}

	return 0;
}
#endif

int set_cur_uctl_list(struct fimc_is_sensor_interface *itf)
{
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	if (fimc_is_vender_wdr_mode_on(sensor_peri->cis.cis_data)) {
		fimc_is_sensor_set_cis_uctrl_list(sensor_peri,
			sensor_peri->cis.cur_sensor_uctrl.longExposureTime,
			sensor_peri->cis.cur_sensor_uctrl.shortExposureTime,
			sensor_peri->cis.cur_sensor_uctrl.sensitivity,
			0,
			sensor_peri->cis.cur_sensor_uctrl.longAnalogGain,
			sensor_peri->cis.cur_sensor_uctrl.shortAnalogGain,
			sensor_peri->cis.cur_sensor_uctrl.longDigitalGain,
			sensor_peri->cis.cur_sensor_uctrl.shortDigitalGain);
	} else {
		fimc_is_sensor_set_cis_uctrl_list(sensor_peri,
			sensor_peri->cis.cur_sensor_uctrl.exposureTime,
			0,
			sensor_peri->cis.cur_sensor_uctrl.sensitivity,
			0,
			sensor_peri->cis.cur_sensor_uctrl.analogGain,
			0,
			sensor_peri->cis.cur_sensor_uctrl.digitalGain,
			0);
	}

	return 0;
}

int apply_sensor_setting(struct fimc_is_sensor_interface *itf)
{
	struct fimc_is_device_sensor *device = NULL;
	struct fimc_is_module_enum *module = NULL;
	struct v4l2_subdev *subdev_module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	module = sensor_peri->module;
	if (unlikely(!module)) {
		err("module in is NULL");
		module = NULL;
		goto p_err;
	}

	subdev_module = module->subdev;
	if (!subdev_module) {
		err("module is not probed");
		subdev_module = NULL;
		goto p_err;
	}

	device = v4l2_get_subdev_hostdata(subdev_module);
	FIMC_BUG(!device);

	/* sensor control */
	fimc_is_sensor_ctl_frame_evt(device);

p_err:
	return 0;
}

int request_reset_expo_gain(struct fimc_is_sensor_interface *itf,
				u32 long_expo,
				u32 long_tgain,
				u32 long_again,
				u32 long_dgain,
				u32 short_expo,
				u32 short_tgain,
				u32 short_again,
				u32 short_dgain)
{
	int ret = 0;
	u32 i = 0;
	u32 end_index = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	dbg_sensor(1, "[%s] long exposure(%d), total_gain(%d), a-gain(%d), d-gain(%d)\n", __func__,
			long_expo, long_tgain, long_again, long_dgain);
	dbg_sensor(1, "[%s] short exposure(%d), total_gain(%d), a-gain(%d), d-gain(%d)\n", __func__,
			short_expo, short_tgain, short_again, short_dgain);

	end_index = itf->otf_flag_3aa == true ? NEXT_NEXT_FRAME_OTF : NEXT_NEXT_FRAME_DMA;

	for (i = 0; i <= end_index; i++) {
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_TOTAL_GAIN, i, long_tgain, short_tgain);
		if (ret < 0)
			pr_err("[%s] set_interface_param TOTAL_GAIN fail(%d)\n", __func__, ret);
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_ANALOG_GAIN, i, long_again, short_again);
		if (ret < 0)
			pr_err("[%s] set_interface_param ANALOG_GAIN fail(%d)\n", __func__, ret);
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_DIGITAL_GAIN, i, long_dgain, short_dgain);
		if (ret < 0)
			pr_err("[%s] set_interface_param DIGITAL_GAIN fail(%d)\n", __func__, ret);
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_EXPOSURE, i, long_expo, short_expo);
		if (ret < 0)
			pr_err("[%s] set_interface_param EXPOSURE fail(%d)\n", __func__, ret);
	}

	itf->vsync_flag = true;

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	fimc_is_sensor_set_cis_uctrl_list(sensor_peri,
			long_expo,
			short_expo,
			long_tgain, short_tgain,
			long_again, short_again,
			long_dgain, short_dgain);

	memset(sensor_peri->cis.cis_data->auto_exposure, 0, sizeof(sensor_peri->cis.cis_data->auto_exposure));

	sensor_peri->cis.cis_data->auto_exposure[CURRENT_FRAME].exposure = long_expo;
	sensor_peri->cis.cis_data->auto_exposure[CURRENT_FRAME].analog_gain = long_again;
	sensor_peri->cis.cis_data->auto_exposure[CURRENT_FRAME].digital_gain = long_dgain;
	sensor_peri->cis.cis_data->auto_exposure[CURRENT_FRAME].long_exposure = long_expo;
	sensor_peri->cis.cis_data->auto_exposure[CURRENT_FRAME].long_analog_gain = long_again;
	sensor_peri->cis.cis_data->auto_exposure[CURRENT_FRAME].long_digital_gain = long_dgain;
	sensor_peri->cis.cis_data->auto_exposure[CURRENT_FRAME].short_exposure = short_expo;
	sensor_peri->cis.cis_data->auto_exposure[CURRENT_FRAME].short_analog_gain = short_again;
	sensor_peri->cis.cis_data->auto_exposure[CURRENT_FRAME].short_digital_gain = short_dgain;

	sensor_peri->cis.cis_data->auto_exposure[NEXT_FRAME].exposure = long_expo;
	sensor_peri->cis.cis_data->auto_exposure[NEXT_FRAME].analog_gain = long_again;
	sensor_peri->cis.cis_data->auto_exposure[NEXT_FRAME].digital_gain = long_dgain;
	sensor_peri->cis.cis_data->auto_exposure[NEXT_FRAME].long_exposure = long_expo;
	sensor_peri->cis.cis_data->auto_exposure[NEXT_FRAME].long_analog_gain = long_again;
	sensor_peri->cis.cis_data->auto_exposure[NEXT_FRAME].long_digital_gain = long_dgain;
	sensor_peri->cis.cis_data->auto_exposure[NEXT_FRAME].short_exposure = short_expo;
	sensor_peri->cis.cis_data->auto_exposure[NEXT_FRAME].short_analog_gain = short_again;
	sensor_peri->cis.cis_data->auto_exposure[NEXT_FRAME].short_digital_gain = short_dgain;

	return ret;
}

int set_sensor_info_mode_change(struct fimc_is_sensor_interface *itf,
		u32 long_expo,
		u32 long_again,
		u32 long_dgain,
		u32 expo,
		u32 again,
		u32 dgain)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);

	sensor_peri->cis.mode_chg_expo = expo;
	sensor_peri->cis.mode_chg_again = again;
	sensor_peri->cis.mode_chg_dgain = dgain;
	sensor_peri->cis.mode_chg_long_expo = long_expo;
	sensor_peri->cis.mode_chg_long_again = long_again;
	sensor_peri->cis.mode_chg_long_dgain = long_dgain;

	dbg_sensor(1, "[%s] mode_chg_expo(%d), again(%d), dgain(%d)\n", __func__,
			sensor_peri->cis.mode_chg_expo,
			sensor_peri->cis.mode_chg_again,
			sensor_peri->cis.mode_chg_dgain);
	dbg_sensor(1, "[%s] mode_chg_long_expo(%d), long_again(%d), long_dgain(%d)\n", __func__,
			sensor_peri->cis.mode_chg_long_expo,
			sensor_peri->cis.mode_chg_long_again,
			sensor_peri->cis.mode_chg_long_dgain);

	return ret;
}

int update_sensor_dynamic_meta(struct fimc_is_sensor_interface *itf,
		u32 frame_count,
		camera2_ctl_t *ctrl,
		camera2_dm_t *dm,
		camera2_udm_t *udm)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	u32 index = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(!ctrl);
	FIMC_BUG(!dm);
	FIMC_BUG(!udm);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	index = frame_count % EXPECT_DM_NUM;
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);

	dm->sensor.exposureTime = sensor_peri->cis.expecting_sensor_dm[index].exposureTime;
	dm->sensor.frameDuration = sensor_peri->cis.cis_data->cur_frame_us_time * 1000;
	dm->sensor.sensitivity = sensor_peri->cis.expecting_sensor_dm[index].sensitivity;
	if (sensor_peri->cis.cis_data->cur_lownoise_mode == FIMC_IS_CIS_LN4)
		dm->sensor.rollingShutterSkew = sensor_peri->cis.cis_data->rolling_shutter_skew * 4;
	else if (sensor_peri->cis.cis_data->cur_lownoise_mode == FIMC_IS_CIS_LN2)
		dm->sensor.rollingShutterSkew = sensor_peri->cis.cis_data->rolling_shutter_skew * 2;
	else
		dm->sensor.rollingShutterSkew = sensor_peri->cis.cis_data->rolling_shutter_skew;

	udm->sensor.analogGain = sensor_peri->cis.expecting_sensor_udm[index].analogGain;
	udm->sensor.digitalGain = sensor_peri->cis.expecting_sensor_udm[index].digitalGain;
	udm->sensor.longExposureTime = sensor_peri->cis.expecting_sensor_udm[index].longExposureTime;
	udm->sensor.shortExposureTime = sensor_peri->cis.expecting_sensor_udm[index].shortExposureTime;
	udm->sensor.longAnalogGain = sensor_peri->cis.expecting_sensor_udm[index].longAnalogGain;
	udm->sensor.shortAnalogGain = sensor_peri->cis.expecting_sensor_udm[index].shortAnalogGain;
	udm->sensor.longDigitalGain = sensor_peri->cis.expecting_sensor_udm[index].longDigitalGain;
	udm->sensor.shortDigitalGain = sensor_peri->cis.expecting_sensor_udm[index].shortDigitalGain;

	dbg_sensor(1, "[%s]: expo(%lld), duration(%lld), sensitivity(%d), rollingShutterSkew(%lld)\n",
			__func__, dm->sensor.exposureTime,
			dm->sensor.frameDuration,
			dm->sensor.sensitivity,
			dm->sensor.rollingShutterSkew);
	dbg_sensor(1, "[%s]: udm expo[%lld, %lld], dgain[%d, %d] again[%d, %d]\n",
			__func__,
			udm->sensor.longExposureTime, udm->sensor.shortExposureTime,
			udm->sensor.longDigitalGain, udm->sensor.shortDigitalGain,
			udm->sensor.longAnalogGain, udm->sensor.shortAnalogGain);

	return ret;
}

int copy_sensor_ctl(struct fimc_is_sensor_interface *itf,
			u32 frame_count,
			camera2_shot_t *shot)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	u32 index = 0;
	struct fimc_is_sensor_ctl *sensor_ctl;
	cis_shared_data *cis_data = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	index = frame_count % EXPECT_DM_NUM;
	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	sensor_ctl = &sensor_peri->cis.sensor_ctls[index];
	cis_data = sensor_peri->cis.cis_data;

	FIMC_BUG(!cis_data);

	sensor_ctl->ctl_frame_number = 0;
	sensor_ctl->valid_sensor_ctrl = false;
	sensor_ctl->is_sensor_request = false;

	if (shot != NULL) {
#if defined(CONFIG_CAMERA_PDP)
		cis_data->is_data.paf_mode = shot->uctl.isModeUd.paf_mode;
		cis_data->is_data.wdr_mode = shot->uctl.isModeUd.wdr_mode;
		cis_data->is_data.disparity_mode = shot->uctl.isModeUd.disparity_mode;
#endif
		sensor_ctl->ctl_frame_number = shot->dm.request.frameCount;
		sensor_ctl->cur_cam20_sensor_ctrl = shot->ctl.sensor;
#if defined (CONFIG_CAMERA_USE_MCU)
		if (sensor_peri->mcu && sensor_peri->mcu->ois) {
			sensor_peri->mcu->ois->ois_mode = shot->ctl.lens.opticalStabilizationMode;
			sensor_peri->mcu->ois->coef = (u8)shot->uctl.lensUd.oisCoefVal;
		}
#else
		if (sensor_peri->subdev_ois) {
			sensor_peri->ois->ois_mode = shot->ctl.lens.opticalStabilizationMode;
			sensor_peri->ois->coef = (u8)shot->uctl.lensUd.oisCoefVal;
		}
#endif
		cis_data->video_mode = shot->ctl.aa.vendor_videoMode;

		if (shot->uctl.isModeUd.wdr_mode == CAMERA_WDR_ON ||
				shot->uctl.isModeUd.wdr_mode == CAMERA_WDR_AUTO ||
				shot->uctl.isModeUd.wdr_mode == CAMERA_WDR_AUTO_LIKE ||
				shot->uctl.isModeUd.wdr_mode == CAMERA_WDR_AUTO_3P ||
				shot->uctl.isModeUd.wdr_mode == CAMERA_WDR_AUTO_3P_VIDEO)
			itf->cis_mode = ITF_CIS_SMIA_WDR;
		else
			itf->cis_mode = ITF_CIS_SMIA;

		/* set frame rate : Limit of max frame duration
		 * Frame duration is set by
		 * 1. Manual sensor control
		 *	 - For HAL3.2
		 *	 - AE_MODE is OFF and ctl.sensor.frameDuration is not 0.
		 * 2. Target FPS Range
		 *	 - For backward compatibility
		 *	 - In case of AE_MODE is not OFF and aeTargetFpsRange[0] is not 0,
		 *	   frame durtaion is 1000000us / aeTargetFpsRage[0]
		 */
		if ((shot->ctl.aa.aeMode == AA_AEMODE_OFF)
			|| (shot->ctl.aa.mode == AA_CONTROL_OFF)) {
			sensor_ctl->valid_sensor_ctrl = true;
			sensor_ctl->is_sensor_request = true;
		} else if (shot->ctl.aa.aeTargetFpsRange[1] != 0) {
			u32 duration_us = 1000000 / shot->ctl.aa.aeTargetFpsRange[1];
			sensor_ctl->cur_cam20_sensor_udctrl.frameDuration = fimc_is_sensor_convert_us_to_ns(duration_us);

			/* qbuf min, max fps value */
			sensor_peri->cis.min_fps = shot->ctl.aa.aeTargetFpsRange[0];
			sensor_peri->cis.max_fps = shot->ctl.aa.aeTargetFpsRange[1];
		}
	}

	return ret;
}

int get_module_id(struct fimc_is_sensor_interface *itf, u32 *module_id)
{
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct fimc_is_module_enum *module = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!module_id);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	module = sensor_peri->module;
	if (unlikely(!module)) {
		err("module in is NULL");
		module = NULL;
		goto p_err;
	}

	*module_id = module->sensor_id;

p_err:
	return 0;

}

int get_module_position(struct fimc_is_sensor_interface *itf,
				enum exynos_sensor_position *module_position)
{
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct fimc_is_module_enum *module = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!module_position);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	module = sensor_peri->module;
	if (unlikely(!module)) {
		err("module in is NULL");
		module = NULL;
		goto p_err;
	}

	*module_position = module->position;

p_err:
	return 0;

}

int set_sensor_3a_mode(struct fimc_is_sensor_interface *itf,
				u32 mode)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	if (mode > 1) {
		err("invalid mode(%d)\n", mode);
		return -1;
	}

	/* 0: OTF, 1: M2M */
	itf->otf_flag_3aa = mode == 0 ? true : false;

	if (itf->otf_flag_3aa == false) {
		ret = fimc_is_sensor_init_sensor_thread(sensor_peri);
		if (ret) {
			err("fimc_is_sensor_init_sensor_thread is fail(%d)", ret);
			return ret;
		}
	}

	return 0;
}

int get_initial_exposure_gain_of_sensor(struct fimc_is_sensor_interface *itf,
	u32 *long_expo,
	u32 *long_again,
	u32 *long_dgain,
	u32 *short_expo,
	u32 *short_again,
	u32 *short_dgain)
{
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	if (!itf) {
		err("[%s] fimc_is_sensor_interface is NULL", __func__);
		return -EINVAL;
	}

	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	if (!sensor_peri) {
		err("[%s] sensor_peri is NULL", __func__);
		return -EINVAL;
	}

	if (sensor_peri->cis.use_initial_ae) {
		*long_expo = sensor_peri->cis.init_ae_setting.long_exposure;
		*long_again = sensor_peri->cis.init_ae_setting.long_analog_gain;
		*long_dgain = sensor_peri->cis.init_ae_setting.long_digital_gain;
		*short_expo = sensor_peri->cis.init_ae_setting.exposure;
		*short_again = sensor_peri->cis.init_ae_setting.analog_gain;
		*short_dgain = sensor_peri->cis.init_ae_setting.digital_gain;
	} else {
		*long_expo = sensor_peri->cis.cis_data->low_expo_start;
		*long_again = 1000;
		*long_dgain = 1000;
		*short_expo = sensor_peri->cis.cis_data->low_expo_start;
		*short_again = 1000;
		*short_dgain = 1000;
		dbg_sensor(1, "%s: called at not enabled last_ae, use default low exposure setting", __func__);
	}

	dbg_sensor(1, "%s: sensorid(%d),long(%d-%d-%d), shot(%d-%d-%d)\n", __func__,
		sensor_peri->module->sensor_id,
		*long_expo, *long_again, *long_dgain, *short_expo, *short_again, *short_dgain);

	return 0;
}

u32 set_adjust_sync(struct fimc_is_sensor_interface *itf, u32 setsync)
{
	u32 ret = 0;
	struct fimc_is_device_sensor *device = NULL;
	struct fimc_is_module_enum *module = NULL;
	struct v4l2_subdev *subdev_module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	module = sensor_peri->module;
	subdev_module = module->subdev;
	device = v4l2_get_subdev_hostdata(subdev_module);

	/* sensor control */
	fimc_is_sensor_ctl_adjust_sync(device, setsync);

	return ret;
}

u32 request_frame_length_line(struct fimc_is_sensor_interface *itf, u32 framelengthline)
{
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	cis_shared_data *cis_data = NULL;
	u32 frameDuration = 0;

	FIMC_BUG(!itf);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	cis_data = sensor_peri->cis.cis_data;
	FIMC_BUG(!cis_data);
	frameDuration = fimc_is_sensor_convert_ns_to_us(sensor_peri->cis.cur_sensor_uctrl.frameDuration);
#ifdef CAMERA_REAR2
	cis_data->min_frame_us_time = MAX(frameDuration, cis_data->min_sync_frame_us_time + framelengthline);
#endif

	dbg_sensor(1, "[%s] frameDuration(%d), framelengthline(%d), min_frame_us_time(%d)\n",
		__func__, frameDuration, framelengthline, cis_data->min_frame_us_time);

	return 0;
}

/* In order to change a current CIS mode when an user select the WDR (long and short exposure) mode or the normal AE mo */
int change_cis_mode(struct fimc_is_sensor_interface *itf,
		enum itf_cis_interface cis_mode)
{
	int ret = 0;

#if 0
	/* Change get cis_mode to copy sensor_ctl */
	info("cis mode : %d -> %d", itf->cis_mode, cis_mode);
	itf->cis_mode = cis_mode;
#endif

	return ret;
}

int start_of_frame(struct fimc_is_sensor_interface *itf)
{
	int ret = 0;
	u32 i = 0;
	u32 end_index = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	dbg_sensor(1, "%s: !!!!!!!!!!!!!!!!!!!!!!!\n", __func__);
	end_index = itf->otf_flag_3aa == true ? NEXT_NEXT_FRAME_OTF : NEXT_NEXT_FRAME_DMA;

	for (i = 0; i < end_index; i++) {
		if (itf->cis_mode == ITF_CIS_SMIA) {
			itf->total_gain[EXPOSURE_GAIN_INDEX][i] = itf->total_gain[EXPOSURE_GAIN_INDEX][i + 1];
			itf->analog_gain[EXPOSURE_GAIN_INDEX][i] = itf->analog_gain[EXPOSURE_GAIN_INDEX][i + 1];
			itf->digital_gain[EXPOSURE_GAIN_INDEX][i] = itf->digital_gain[EXPOSURE_GAIN_INDEX][i + 1];
			itf->exposure[EXPOSURE_GAIN_INDEX][i] = itf->exposure[EXPOSURE_GAIN_INDEX][i + 1];
		} else if (itf->cis_mode == ITF_CIS_SMIA_WDR){
			itf->total_gain[LONG_EXPOSURE_GAIN_INDEX][i] = itf->total_gain[LONG_EXPOSURE_GAIN_INDEX][i + 1];
			itf->total_gain[SHORT_EXPOSURE_GAIN_INDEX][i] = itf->total_gain[SHORT_EXPOSURE_GAIN_INDEX][i + 1];
			itf->analog_gain[LONG_EXPOSURE_GAIN_INDEX][i] = itf->analog_gain[LONG_EXPOSURE_GAIN_INDEX][i + 1];
			itf->analog_gain[SHORT_EXPOSURE_GAIN_INDEX][i] = itf->analog_gain[SHORT_EXPOSURE_GAIN_INDEX][i + 1];
			itf->digital_gain[LONG_EXPOSURE_GAIN_INDEX][i] = itf->digital_gain[LONG_EXPOSURE_GAIN_INDEX][i + 1];
			itf->digital_gain[SHORT_EXPOSURE_GAIN_INDEX][i] = itf->digital_gain[SHORT_EXPOSURE_GAIN_INDEX][i + 1];
			itf->exposure[LONG_EXPOSURE_GAIN_INDEX][i] = itf->exposure[LONG_EXPOSURE_GAIN_INDEX][i + 1];
			itf->exposure[SHORT_EXPOSURE_GAIN_INDEX][i] = itf->exposure[SHORT_EXPOSURE_GAIN_INDEX][i + 1];
		} else {
			pr_err("[%s] in valid cis_mode (%d)\n", __func__, itf->cis_mode);
			ret = -EINVAL;
			goto p_err;
		}

		itf->flash_intensity[i] = itf->flash_intensity[i + 1];
		itf->flash_mode[i] = itf->flash_mode[i + 1];
		itf->flash_firing_duration[i] = itf->flash_firing_duration[i + 1];
	}

	itf->flash_mode[i] = CAM2_FLASH_MODE_OFF;
	itf->flash_intensity[end_index] = 0;
	itf->flash_firing_duration[i] = 0;

	/* Flash setting */
	ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_FLASH_INTENSITY, end_index, 0, 0);
	if (ret < 0)
		pr_err("[%s] set_interface_param FLASH_INTENSITY fail(%d)\n", __func__, ret);
	/* TODO */
	/*
	if (itf->flash_itf_ops) {
		(*itf->flash_itf_ops)->on_start_of_frame(itf->flash_itf_ops);
		(*itf->flash_itf_ops)->set_next_flash(itf->flash_itf_ops, itf->flash_intensity[NEXT_FRAME]);
	}
	*/

p_err:
	return ret;
}

int end_of_frame(struct fimc_is_sensor_interface *itf)
{
	int ret = 0;
	u32 end_index = 0;
	u32 long_total_gain = 0;
	u32 short_total_gain = 0;
	u32 long_analog_gain = 0;
	u32 short_analog_gain = 0;
	u32 long_digital_gain = 0;
	u32 short_digital_gain = 0;
	u32 long_exposure = 0;
	u32 short_exposure = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	dbg_sensor(1, "%s: !!!!!!!!!!!!!!!!!!!!!!!\n", __func__);
	end_index = itf->otf_flag_3aa == true ? NEXT_NEXT_FRAME_OTF : NEXT_NEXT_FRAME_DMA;

	if (itf->vsync_flag == true) {
		/* TODO: sensor timing test */

		if (itf->otf_flag_3aa == false) {
			/* set gain */
			ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_TOTAL_GAIN,
						end_index, &long_total_gain, &short_total_gain);
			if (ret < 0)
				pr_err("[%s] get TOTAL_GAIN fail(%d)\n", __func__, ret);
			ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_ANALOG_GAIN,
						end_index, &long_analog_gain, &short_analog_gain);
			if (ret < 0)
				pr_err("[%s] get ANALOG_GAIN fail(%d)\n", __func__, ret);
			ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_DIGITAL_GAIN,
						end_index, &long_digital_gain, &short_digital_gain);
			if (ret < 0)
				pr_err("[%s] get DIGITAL_GAIN fail(%d)\n", __func__, ret);

			ret = set_gain_permile(itf, itf->cis_mode,
						long_total_gain, short_total_gain,
						long_analog_gain, short_analog_gain,
						long_digital_gain, short_digital_gain);
			if (ret < 0) {
				pr_err("[%s] set_gain_permile fail(%d)\n", __func__, ret);
				goto p_err;
			}

			/* set exposure */
			ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_EXPOSURE,
						end_index, &long_exposure, &short_exposure);
			if (ret < 0)
				pr_err("[%s] get EXPOSURE fail(%d)\n", __func__, ret);

			ret = set_exposure(itf, itf->cis_mode, long_exposure, short_exposure);
			if (ret < 0) {
				pr_err("[%s] set_exposure fail(%d)\n", __func__, ret);
				goto p_err;
			}
		}
	}

	/* TODO */
	/*
	if (itf->flash_itf_ops) {
		(*itf->flash_itf_ops)->on_end_of_frame(itf->flash_itf_ops);
	}
	*/

p_err:
	return ret;
}

int apply_frame_settings(struct fimc_is_sensor_interface *itf)
{
	/* NOT IMPLEMENTED YET */
	int ret = -1;

	err("NOT IMPLEMENTED YET\n");

	return ret;
}

/* end of new APIs */

/* APERTURE interface */
int set_aperture_value(struct fimc_is_sensor_interface *itf, int value)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	WARN_ON(!itf);
	WARN_ON(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	dbg_aperture("[%s] aperture value(%d)\n", __func__, value);

	if (sensor_peri->mcu && sensor_peri->mcu->aperture) {
		sensor_peri->mcu->aperture->start_value = value;

		if (value != sensor_peri->mcu->aperture->cur_value) {
			sensor_peri->mcu->aperture->new_value = value;
			sensor_peri->mcu->aperture->step = APERTURE_STEP_PREPARE;
		}
	}

	return ret;
}

int get_aperture_value(struct fimc_is_sensor_interface *itf, struct fimc_is_apature_info_t *param)
{
	int ret = 0;
	int aperture_value = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct fimc_is_module_enum *module = NULL;
	struct v4l2_subdev *subdev_module = NULL;
	struct fimc_is_device_sensor *device = NULL;
	struct fimc_is_core *core = NULL;
	struct fimc_is_vender_specific *specific = NULL;

	WARN_ON(!itf);
	WARN_ON(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	module = sensor_peri->module;
	WARN_ON(!module);

	subdev_module = module->subdev;
	WARN_ON(!subdev_module);

	device = v4l2_get_subdev_hostdata(subdev_module);
	WARN_ON(!device);

	core = device->private_data;
	WARN_ON(!core);

	specific = core->vender.private_data;
	WARN_ON(!specific);

	if (sensor_peri->mcu && sensor_peri->mcu->aperture)
		aperture_value = sensor_peri->mcu->aperture->cur_value;
	else
		aperture_value = 0;

	param->cur_value = aperture_value;
	param->zoom_running = specific->zoom_running;

	dbg_aperture("[%s] aperture value(%d), zoom_running(%d)\n",
			__func__, aperture_value, specific->zoom_running);

	return ret;
}

/* Flash interface */
int set_flash(struct fimc_is_sensor_interface *itf,
		u32 frame_count, u32 flash_mode, u32 intensity, u32 time)
{
	int ret = 0;
	struct fimc_is_sensor_ctl *sensor_ctl = NULL;
	camera2_flash_uctl_t *flash_uctl = NULL;
	enum flash_mode mode = CAM2_FLASH_MODE_OFF;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_ctl = get_sensor_ctl_from_module(itf, frame_count);
	flash_uctl = &sensor_ctl->cur_cam20_flash_udctrl;

	sensor_ctl->flash_frame_number = frame_count;

	if (intensity == 0) {
		mode = CAM2_FLASH_MODE_OFF;
	} else {
		switch (flash_mode) {
		case CAM2_FLASH_MODE_OFF:
		case CAM2_FLASH_MODE_SINGLE:
		case CAM2_FLASH_MODE_TORCH:
			mode = flash_mode;
			break;
		default:
			err("[%s] unknown scene_mode(%d)\n", __func__, flash_mode);
			break;
		}
	}

	flash_uctl->flashMode = mode;
	flash_uctl->firingPower = intensity;
	flash_uctl->firingTime = time;

	dbg_flash("[%s] frame count %d,  mode %d, intensity %d, firing time %lld\n", __func__,
			frame_count,
			flash_uctl->flashMode,
			flash_uctl->firingPower,
			flash_uctl->firingTime);

	sensor_ctl->valid_flash_udctrl = true;

	return ret;
}

int request_flash(struct fimc_is_sensor_interface *itf,
				u32 mode,
				bool on,
				u32 intensity,
				u32 time)
{
	int ret = 0;
	u32 i = 0;
	u32 end_index = 0;
	u32 vsync_cnt = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	vsync_cnt = get_vsync_count(itf);

	dbg_flash("[%s](%d) request_flash, mode(%d), on(%d), intensity(%d), time(%d)\n",
			__func__, vsync_cnt, mode, on, intensity, time);

	ret = get_num_of_frame_per_one_3aa(itf, &end_index);
	if (ret < 0) {
		pr_err("[%s] get_num_of_frame_per_one_3aa fail(%d)\n", __func__, ret);
		goto p_err;
	}

	for (i = 0; i < end_index; i++) {
		if (mode == CAM2_FLASH_MODE_TORCH && on == false) {
			dbg_flash("[%s](%d) pre-flash off, mode(%d), on(%d), intensity(%d), time(%d)\n",
				__func__, vsync_cnt, mode, on, intensity, time);
			/* pre-flash off */
			sensor_peri->flash->flash_ae.pre_fls_ae_reset = true;
			sensor_peri->flash->flash_ae.frm_num_pre_fls = vsync_cnt + 1;
		} else if (mode == CAM2_FLASH_MODE_SINGLE && on == true) {
			dbg_flash("[%s](%d) main on-off, mode(%d), on(%d), intensity(%d), time(%d)\n",
				__func__, vsync_cnt, mode, on, intensity, time);

			sensor_peri->flash->flash_data.mode = mode;
			sensor_peri->flash->flash_data.intensity = intensity;
			sensor_peri->flash->flash_data.firing_time_us = time;
			/* main-flash on off*/
			sensor_peri->flash->flash_ae.main_fls_ae_reset = true;
			sensor_peri->flash->flash_ae.frm_num_main_fls[0] = vsync_cnt + 1;
			sensor_peri->flash->flash_ae.frm_num_main_fls[1] = vsync_cnt + 2;
		} else {
			/* pre-flash on & flash off */
			ret = set_flash(itf, vsync_cnt + i, mode, intensity, time);
			if (ret < 0) {
				pr_err("[%s] set_flash fail(%d)\n", __func__, ret);
				goto p_err;
			}
		}
	}

p_err:
	return ret;
}

int request_flash_expo_gain(struct fimc_is_sensor_interface *itf,
			struct fimc_is_flash_expo_gain *flash_ae)
{
	int ret = 0;
	int i = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	for (i = 0; i < 2; i++) {
		sensor_peri->flash->flash_ae.expo[i] = flash_ae->expo[i];
		sensor_peri->flash->flash_ae.tgain[i] = flash_ae->tgain[i];
		sensor_peri->flash->flash_ae.again[i] = flash_ae->again[i];
		sensor_peri->flash->flash_ae.dgain[i] = flash_ae->dgain[i];
		sensor_peri->flash->flash_ae.long_expo[i] = flash_ae->long_expo[i];
		sensor_peri->flash->flash_ae.long_tgain[i] = flash_ae->long_tgain[i];
		sensor_peri->flash->flash_ae.long_again[i] = flash_ae->long_again[i];
		sensor_peri->flash->flash_ae.long_dgain[i] = flash_ae->long_dgain[i];
		sensor_peri->flash->flash_ae.short_expo[i] = flash_ae->short_expo[i];
		sensor_peri->flash->flash_ae.short_tgain[i] = flash_ae->short_tgain[i];
		sensor_peri->flash->flash_ae.short_again[i] = flash_ae->short_again[i];
		sensor_peri->flash->flash_ae.short_dgain[i] = flash_ae->short_dgain[i];
		dbg_flash("[%s] expo(%d, %d, %d), again(%d, %d, %d), dgain(%d, %d, %d)\n",
			__func__,
			sensor_peri->flash->flash_ae.expo[i],
			sensor_peri->flash->flash_ae.long_expo[i],
			sensor_peri->flash->flash_ae.short_expo[i],
			sensor_peri->flash->flash_ae.again[i],
			sensor_peri->flash->flash_ae.long_again[i],
			sensor_peri->flash->flash_ae.short_again[i],
			sensor_peri->flash->flash_ae.dgain[i],
			sensor_peri->flash->flash_ae.long_dgain[i],
			sensor_peri->flash->flash_ae.short_dgain[i]);
	}

	return ret;
}

int update_flash_dynamic_meta(struct fimc_is_sensor_interface *itf,
		u32 frame_count,
		camera2_ctl_t *ctrl,
		camera2_dm_t *dm,
		camera2_udm_t *udm)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!ctrl);
	FIMC_BUG(!dm);
	FIMC_BUG(!udm);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);

	if (sensor_peri->flash) {
		dm->flash.flashMode = sensor_peri->flash->flash_data.mode;
		dm->flash.firingPower = sensor_peri->flash->flash_data.intensity;
		dm->flash.firingTime = sensor_peri->flash->flash_data.firing_time_us;
		if (sensor_peri->flash->flash_data.flash_fired)
			dm->flash.flashState = FLASH_STATE_FIRED;
		else
			dm->flash.flashState = FLASH_STATE_READY;

		dbg_flash("[%s]: mode(%d), power(%d), time(%lld), state(%d)\n",
				__func__, dm->flash.flashMode,
				dm->flash.firingPower,
				dm->flash.firingTime,
				dm->flash.flashState);
	}

	return ret;
}

static struct fimc_is_framemgr *get_csi_vc_framemgr(struct fimc_is_device_csi *csi, u32 ch)
{
	struct fimc_is_subdev *fimc_is_subdev_vc;
	struct fimc_is_framemgr *framemgr = NULL;

	if (ch >= CSI_VIRTUAL_CH_MAX) {
		err("VC(%d of %d) is out-of-range", ch, CSI_VIRTUAL_CH_MAX);
		return NULL;
	}

#if !defined(CONFIG_USE_SENSOR_GROUP)
	if (ch == CSI_VIRTUAL_CH_0) {
		framemgr = csi->framemgr;
	} else {
#endif
		fimc_is_subdev_vc = csi->dma_subdev[ch];
		if (!fimc_is_subdev_vc ||
				!test_bit(FIMC_IS_SUBDEV_START, &fimc_is_subdev_vc->state)) {
			err("[%d] vc(%d) subdev is not started", csi->instance, ch);
			return NULL;
		}

		framemgr = GET_SUBDEV_FRAMEMGR(fimc_is_subdev_vc);
#if !defined(CONFIG_USE_SENSOR_GROUP)
	}
#endif
	return framemgr;
}

int get_vc_dma_buf(struct fimc_is_sensor_interface *itf,
		enum itf_vc_buf_data_type request_data_type,
		u32 frame_count,
		u32 *buf_index,
		u64 *buf_addr)
{
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_csi *csi;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	struct fimc_is_subdev *subdev;
	unsigned long flags;
	int ret;
	int ch;

	WARN_ON(!buf_addr);
	WARN_ON(!buf_index);

	*buf_addr = 0;
	*buf_index = 0;

	sensor = get_device_sensor(itf);
	if (!sensor) {
		err("failed to get sensor device");
		return -ENODEV;
	}

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);

	switch (request_data_type) {
	case VC_BUF_DATA_TYPE_SENSOR_STAT1:
	case VC_BUF_DATA_TYPE_SENSOR_STAT2:
		for (ch = CSI_VIRTUAL_CH_1; ch < CSI_VIRTUAL_CH_MAX; ch++) {
			if (sensor->cfg->output[ch].type == VC_TAILPDAF)
				break;
		}
		break;
	case VC_BUF_DATA_TYPE_GENERAL_STAT1:
		for (ch = CSI_VIRTUAL_CH_1; ch < CSI_VIRTUAL_CH_MAX; ch++) {
			if (sensor->cfg->output[ch].type == VC_PRIVATE)
				break;
		}
		break;
	case VC_BUF_DATA_TYPE_GENERAL_STAT2:
		for (ch = CSI_VIRTUAL_CH_1; ch < CSI_VIRTUAL_CH_MAX; ch++) {
			if (sensor->cfg->output[ch].type == VC_MIPISTAT)
				break;
		}
		break;
	default:
		err("invalid data type(%d)", request_data_type);
		return -EINVAL;
	}

	if (ch == CSI_VIRTUAL_CH_MAX) {
		err("requested stat. type(%d) is not supported with current config",
								request_data_type);
		return -EINVAL;
	}

	framemgr = get_csi_vc_framemgr(csi, ch);
	if (!framemgr) {
		err("failed to get framemgr");
		return -ENXIO;
	}

	subdev = csi->dma_subdev[ch];

	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_30, flags);
	if (!framemgr->frames) {
		merr("framemgr was already closed", sensor);
		ret = -EINVAL;
		goto err_get_framemgr;
	}

	frame = find_frame(framemgr, FS_FREE, frame_fcount, (void *)(ulong)frame_count);
	if (frame) {
		/* cache invalidate */
		CALL_BUFOP(subdev->pb_subdev[frame->index],
				sync_for_cpu,
				subdev->pb_subdev[frame->index],
				0,
				subdev->pb_subdev[frame->index]->size,
				DMA_FROM_DEVICE);

		*buf_addr = frame->kvaddr_buffer[0];
		*buf_index = frame->index;

		trans_frame(framemgr, frame, FS_PROCESS);
	} else {
		err("failed to get a frame: fcount: %d", frame_count);
		ret = -EINVAL;
		goto err_invalid_frame;
	}

	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_30, flags);

	dbg_sensor(2, "[%s]: ch: %d, index: %d, framecount: %d, addr: 0x%llx\n",
			__func__, ch, *buf_index, frame_count, *buf_addr);

	return 0;

err_invalid_frame:
err_get_framemgr:
	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_30, flags);

	return ret;
}

int put_vc_dma_buf(struct fimc_is_sensor_interface *itf,
		enum itf_vc_buf_data_type request_data_type,
		u32 index)
{
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_csi *csi;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	unsigned long flags;
	int ch;

	sensor = get_device_sensor(itf);
	if (!sensor) {
		err("failed to get sensor device");
		return -ENODEV;
	}

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);

	switch (request_data_type) {
	case VC_BUF_DATA_TYPE_SENSOR_STAT1:
	case VC_BUF_DATA_TYPE_SENSOR_STAT2:
		for (ch = CSI_VIRTUAL_CH_1; ch < CSI_VIRTUAL_CH_MAX; ch++) {
			if (sensor->cfg->output[ch].type == VC_TAILPDAF)
				break;
		}
		break;
	case VC_BUF_DATA_TYPE_GENERAL_STAT1:
		for (ch = CSI_VIRTUAL_CH_1; ch < CSI_VIRTUAL_CH_MAX; ch++) {
			if (sensor->cfg->output[ch].type == VC_PRIVATE)
				break;
		}
		break;
	case VC_BUF_DATA_TYPE_GENERAL_STAT2:
		for (ch = CSI_VIRTUAL_CH_1; ch < CSI_VIRTUAL_CH_MAX; ch++) {
			if (sensor->cfg->output[ch].type == VC_MIPISTAT)
				break;
		}
		break;
	default:
		err("invalid data type(%d)", request_data_type);
		return -EINVAL;
	}

	if (ch == CSI_VIRTUAL_CH_MAX) {
		err("requested stat. type(%d) is not supported with current config",
								request_data_type);
		return -EINVAL;
	}

	framemgr = get_csi_vc_framemgr(csi, ch);
	if (!framemgr) {
		err("failed to get framemgr");
		return -ENXIO;
	}

	if (index >= framemgr->num_frames) {
		err("index(%d of %d) is out-of-range", index, framemgr->num_frames);
		return -ENOENT;
	}

	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_31, flags);
	if (!framemgr->frames) {
		framemgr_x_barrier_irqr(framemgr, FMGR_IDX_31, flags);
		merr("framemgr was already closed", sensor);
		return -EINVAL;
	}

	frame = &framemgr->frames[index];
	trans_frame(framemgr, frame, FS_FREE);

	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_31, flags);

	dbg_sensor(1, "[%s]: ch: %d, index: %d\n", __func__, ch, index);

	return 0;
}

int get_vc_dma_buf_info(struct fimc_is_sensor_interface *itf,
		enum itf_vc_buf_data_type request_data_type,
		struct vc_buf_info_t *buf_info)
{
	struct fimc_is_module_enum *module;
	struct v4l2_subdev *subdev_module;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_csi *csi;
	int ch;
	struct fimc_is_subdev *subdev;

	memset(buf_info, 0, sizeof(struct vc_buf_info_t));
	buf_info->stat_type = VC_STAT_TYPE_INVALID;
	buf_info->sensor_mode = VC_SENSOR_MODE_INVALID;

	module = get_subdev_module_enum(itf);
	if (!module) {
		err("failed to get sensor_peri's module");
		return -ENODEV;
	}

	subdev_module = module->subdev;
	if (!subdev_module) {
		err("module's subdev was not probed");
		return -ENODEV;
	}

	sensor = v4l2_get_subdev_hostdata(subdev_module);
	if (!sensor) {
		err("failed to get sensor device");
		return -ENODEV;
	}

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);
	if (!csi) {
		err("failed to get csi device");
		return -ENODEV;
	}

	switch (request_data_type) {
	case VC_BUF_DATA_TYPE_SENSOR_STAT1:
	case VC_BUF_DATA_TYPE_SENSOR_STAT2:
		for (ch = CSI_VIRTUAL_CH_1; ch < CSI_VIRTUAL_CH_MAX; ch++) {
			if (sensor->cfg->output[ch].type == VC_TAILPDAF)
				break;
		}
		break;
	case VC_BUF_DATA_TYPE_GENERAL_STAT1:
		for (ch = CSI_VIRTUAL_CH_1; ch < CSI_VIRTUAL_CH_MAX; ch++) {
			if (sensor->cfg->output[ch].type == VC_PRIVATE)
				break;
		}
		break;
	case VC_BUF_DATA_TYPE_GENERAL_STAT2:
		for (ch = CSI_VIRTUAL_CH_1; ch < CSI_VIRTUAL_CH_MAX; ch++) {
			if (sensor->cfg->output[ch].type == VC_MIPISTAT)
				break;
		}
		break;
	default:
		err("invalid data type(%d)", request_data_type);
		return -EINVAL;
	}

	if (ch == CSI_VIRTUAL_CH_MAX) {
		err("requested stat. type(%d) is not supported with current config",
								request_data_type);
		return -EINVAL;
	}

	subdev = csi->dma_subdev[ch];
	if (!subdev) {
		err("failed to get subdev device");
		return -ENODEV;
	}

	buf_info->stat_type = module->vc_extra_info[request_data_type].stat_type;
	buf_info->sensor_mode = module->vc_extra_info[request_data_type].sensor_mode;
	buf_info->width = subdev->output.width;
	buf_info->height = subdev->output.height;
	buf_info->element_size = module->vc_extra_info[request_data_type].max_element;

#if defined(CONFIG_CAMERA_PAFSTAT)
	switch (buf_info->stat_type) {
	case VC_STAT_TYPE_PAFSTAT_FLOATING:
		pafstat_hw_g_floating_size(&buf_info->width, &buf_info->height, &buf_info->element_size);
		break;
	case VC_STAT_TYPE_PAFSTAT_STATIC:
		pafstat_hw_g_static_size(&buf_info->width, &buf_info->height, &buf_info->element_size);
		break;
	}
#endif

	dbg_sensor(2, "VC buf (req_type(%d), stat_type(%d), width(%d), height(%d), element(%d byte))\n",
		request_data_type, buf_info->stat_type, buf_info->width, buf_info->height, buf_info->element_size);

	return 0;
}

int get_vc_dma_buf_max_size(struct fimc_is_sensor_interface *itf,
		enum itf_vc_buf_data_type request_data_type,
		u32 *width,
		u32 *height,
		u32 *element_size)
{
	struct fimc_is_sensor_vc_extra_info *vc_extra_info;
	struct fimc_is_module_enum *module;

	module = get_subdev_module_enum(itf);
	if (unlikely(!module)) {
		err("failed to get sensor_peri's module");
		return -ENODEV;
	}

	if ((request_data_type <= VC_BUF_DATA_TYPE_INVALID) ||
		(request_data_type >= VC_BUF_DATA_TYPE_MAX)) {
		err("invalid data type(%d)", request_data_type);
		return -EINVAL;
	}

	if (module->vc_extra_info[request_data_type].stat_type == VC_BUF_DATA_TYPE_INVALID) {
		err("module doesn't support this stat. type(%d)", request_data_type);
		return -EINVAL;
	}

	vc_extra_info = &module->vc_extra_info[request_data_type];

	*width = vc_extra_info->max_width;
	*height = vc_extra_info->max_height;
	*element_size = vc_extra_info->max_element;

	dbg_sensor(2, "VC max buf (type(%d), width(%d), height(%d),element(%d byte))\n",
		request_data_type, *width, *height, *element_size);

	return (*width) * (*height) * (*element_size);
}

#ifdef CAMERA_REAR2_SENSOR_SHIFT_CROP
int get_sensor_shifted_num(struct fimc_is_sensor_interface *itf,
		u32 *sensor_shifted_num)
{
	struct fimc_is_device_sensor_peri *sensor_peri;

	if (!itf) {
		err("invalid sensor interface");
		return -EINVAL;
	}

	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri,
			sensor_interface);
	if (!sensor_peri) {
		err("failed to get sensor_peri");
		return -EINVAL;
	}

	*sensor_shifted_num = sensor_peri->cis.cis_data->sensor_shifted_num;

	return 0;
}
#endif

int csi_reserved(struct fimc_is_sensor_interface *itf)
{
	return 0;
}

int set_long_term_expo_mode(struct fimc_is_sensor_interface *itf,
		struct fimc_is_long_term_expo_mode *long_term_expo_mode)
{
	int ret = 0;
	int i = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	WARN_ON(!itf);
	WARN_ON(itf->magic != SENSOR_INTERFACE_MAGIC);
	WARN_ON(!long_term_expo_mode);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	sensor_peri->cis.long_term_mode.sen_strm_off_on_enable = long_term_expo_mode->sen_strm_off_on_enable;
	sensor_peri->cis.long_term_mode.frm_num_strm_off_on_interval = long_term_expo_mode->frm_num_strm_off_on_interval;

	if (sensor_peri->cis.long_term_mode.sen_strm_off_on_enable) {
		for (i = 0; i < 2; i++) {
			sensor_peri->cis.long_term_mode.expo[i] = long_term_expo_mode->expo[i];
			sensor_peri->cis.long_term_mode.tgain[i] = long_term_expo_mode->tgain[i];
			sensor_peri->cis.long_term_mode.again[i] = long_term_expo_mode->again[i];
			sensor_peri->cis.long_term_mode.dgain[i] = long_term_expo_mode->dgain[i];
			sensor_peri->cis.long_term_mode.frm_num_strm_off_on[i] = long_term_expo_mode->frm_num_strm_off_on[i];
		}

		sensor_peri->cis.long_term_mode.frame_interval = long_term_expo_mode->frm_num_strm_off_on_interval;
		sensor_peri->cis.long_term_mode.lemode_set.lemode = sensor_peri->cis.long_term_mode.sen_strm_off_on_enable;
	}

	dbg_sensor(1, "[%s]: expo[0](%d), expo[1](%d), again[0](%d), again[1](%d), "
		KERN_CONT "dgain[0](%d), again[1](%d), interval(%d)\n", __func__,
		long_term_expo_mode->expo[0], long_term_expo_mode->expo[1],
		long_term_expo_mode->again[0], long_term_expo_mode->again[1],
		long_term_expo_mode->dgain[0], long_term_expo_mode->dgain[1],
		long_term_expo_mode->frm_num_strm_off_on_interval);

	return ret;
}

int set_low_noise_mode(struct fimc_is_sensor_interface *itf, u32 mode)
{
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	cis_shared_data *cis_data = NULL;

	WARN_ON(!itf);
	WARN_ON(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	cis_data = sensor_peri->cis.cis_data;
	WARN_ON(!cis_data);

	if (mode < FIMC_IS_CIS_LOWNOISE_MODE_MAX)
		cis_data->cur_lownoise_mode = mode;
	else
		cis_data->cur_lownoise_mode = FIMC_IS_CIS_LNOFF;

	dbg_sensor(1, "[%s] mode(%d), cur_lownoise_mode(%d)\n",
		__func__, mode, cis_data->cur_lownoise_mode);

	return 0;
}

int get_sensor_max_dynamic_fps(struct fimc_is_sensor_interface *itf,
			u32 *max_dynamic_fps)
{
	int ret = 0;
	struct fimc_is_device_sensor *sensor = NULL;
	enum fimc_is_ex_mode ex_mode;

	FIMC_BUG(!itf);
	FIMC_BUG(!max_dynamic_fps);

	sensor = get_device_sensor(itf);
	if (!sensor) {
		err("failed to get sensor device");
		return -ENODEV;
	}

	ex_mode = fimc_is_sensor_g_ex_mode(sensor);
	if (ex_mode == EX_DUALFPS_960)
		*max_dynamic_fps = 960;
	else if (ex_mode == EX_DUALFPS_480)
		*max_dynamic_fps = 480;
	else
		*max_dynamic_fps = 0;

	return ret;
}

int get_static_mem(int ctrl_id, void **mem, int *size) {
	int err = 0;

	switch(ctrl_id) {
	case ITF_CTRL_ID_DDK:
		*mem = (void *)ddk_static_data;
		*size = sizeof(ddk_static_data);
		break;
	case ITF_CTRL_ID_RTA:
		*mem = (void *)rta_static_data;
		*size = sizeof(rta_static_data);
		break;
	default:
		err("invalid itf ctrl id %d", ctrl_id);
		*mem = NULL;
		*size = 0;
		err = -EINVAL;
	}

	return err;
}

int get_open_close_hint(int* opening, int* closing) {
	struct fimc_is_core *core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);

	*opening = IS_OPENING_HINT_NONE;
	*closing = IS_CLOSING_HINT_NONE;

#ifdef CONFIG_VENDER_MCD
	if (core) {
		*opening = core->vender.opening_hint;
		*closing = core->vender.closing_hint;
	}
#endif

	dbg_sensor(1, "[%s] opening(%d), closing(%d)\n", __func__, *opening, *closing);

	return 0;
}

int set_mainflash_duration(struct fimc_is_sensor_interface *itf, u32 mainflash_duration)
{
	int ret = 0;
	u32 vsync_cnt = 0;

	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	WARN_ON(!itf);
	WARN_ON(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	vsync_cnt = get_vsync_count(itf);

	if(mainflash_duration < 1) {
		dbg_flash("[%s] duration(%d) is too short\n", __func__, mainflash_duration);
		ret = -1;
		goto p_err;
	}
	else if(mainflash_duration > 5) {
		dbg_flash("[%s] duration(%d) is too long\n", __func__, mainflash_duration);
		ret = -1;
		goto p_err;
	}

	sensor_peri->flash->flash_ae.frm_num_main_fls[1] = vsync_cnt + mainflash_duration + 1;

	dbg_flash("[%s] duration(%d)\n", __func__, mainflash_duration);

p_err:
	return ret;
}

int get_sensor_state(struct fimc_is_sensor_interface *itf)
{
	struct fimc_is_device_sensor *sensor;

	sensor = get_device_sensor(itf);
	if (!sensor) {
		err("failed to get sensor device");
		return -1;
	}

	dbg("%s: sstream(%d)\n", sensor->instance, __func__, sensor->sstream);

	return sensor->sstream;
}

int get_reuse_3a_state(struct fimc_is_sensor_interface *itf,
				u32 *position, u32 *ae_exposure, u32 *ae_deltaev, bool is_clear)
{
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	if (sensor_peri->reuse_3a_value) {
		if (sensor_peri->actuator) {
			if (position != NULL)
				*position = sensor_peri->actuator->position;
		}

		if (ae_exposure != NULL)
			*ae_exposure = sensor_peri->cis.ae_exposure;

		if (ae_deltaev != NULL)
			*ae_deltaev = sensor_peri->cis.ae_deltaev;

		if (is_clear)
			sensor_peri->reuse_3a_value = false;

		return true;
	}

	return false;
}

int set_reuse_ae_exposure(struct fimc_is_sensor_interface *itf,
				u32 ae_exposure, u32 ae_deltaev)
{
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	dbg_sensor(1, "[%s] set ae_exposure (%d)\n", __func__, ae_exposure);

	sensor_peri->cis.ae_exposure = ae_exposure;
	sensor_peri->cis.ae_deltaev = ae_deltaev;

	return 0;
}

int dual_reserved_0(struct fimc_is_sensor_interface *itf)
{
	return 0;
}

int dual_reserved_1(struct fimc_is_sensor_interface *itf)
{
	return 0;
}

int set_paf_param(struct fimc_is_sensor_interface *itf,
		struct paf_setting_t *regs, u32 regs_size)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
	int ret;

	if (!itf) {
		err("invalid sensor interface");
		return -EINVAL;
	}

	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri,
			sensor_interface);
	if (!sensor_peri) {
		err("failed to get sensor_peri");
		return -ENODEV;
	}

	/* PDP */
	if (IS_ENABLED(CONFIG_CAMERA_PDP)) {
		if (!sensor_peri->pdp || !sensor_peri->subdev_pdp) {
			err("invalid PDP state");
			return -EINVAL;
		}

		ret = CALL_PDPOPS(sensor_peri->pdp, set_param,
				sensor_peri->subdev_pdp,
				regs, regs_size);
	/* PAFSTAT */
	} else if (IS_ENABLED(CONFIG_CAMERA_PAFSTAT)) {
		if (!sensor_peri->pafstat || !sensor_peri->subdev_pafstat) {
			err("invalid PAFSTAT state");
			return -EINVAL;
		}

		ret = CALL_PAFSTATOPS(sensor_peri->pafstat, set_param,
				sensor_peri->subdev_pafstat,
				regs, regs_size);
	} else {
		err("no PAF HW");
		return -ENODEV;
	}

	return ret;
}

int get_paf_ready(struct fimc_is_sensor_interface *itf, u32 *ready)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
	int ret;

	if (!itf) {
		err("invalid sensor interface");
		return -EINVAL;
	}

	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri,
			sensor_interface);
	if (!sensor_peri) {
		err("failed to get sensor_peri");
		return -ENODEV;
	}

	/* PDP */
	if (IS_ENABLED(CONFIG_CAMERA_PDP)) {
		if (!sensor_peri->pdp || !sensor_peri->subdev_pdp) {
			err("invalid PDP state");
			return -EINVAL;
		}

		ret = CALL_PDPOPS(sensor_peri->pdp, get_ready,
				sensor_peri->subdev_pdp, ready);

	/* PAFSTAT */
	} else if (IS_ENABLED(CONFIG_CAMERA_PAFSTAT)) {
		if (!sensor_peri->pafstat || !sensor_peri->subdev_pafstat) {
			err("invalid PAFSTAT state");
			return -EINVAL;
		}

		ret = CALL_PAFSTATOPS(sensor_peri->pafstat, get_ready,
				sensor_peri->subdev_pafstat, ready);
	} else {
		err("no PAF HW");
		return -ENODEV;
	}

	return ret;
}

int register_paf_notifier(struct fimc_is_sensor_interface *itf,
			enum itf_vc_stat_type type,
			paf_notifier_t notifier, void *data)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
	int ret;

	if (!itf) {
		err("invalid sensor interface");
		return -EINVAL;
	}

	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri,
			sensor_interface);
	if (!sensor_peri) {
		err("failed to get sensor_peri");
		return -ENODEV;
	}

	/* PDP */
	if (IS_ENABLED(CONFIG_CAMERA_PDP)) {
		if (!sensor_peri->pdp || !sensor_peri->subdev_pdp) {
			err("invalid PDP state");
			return -EINVAL;
		}

		ret = CALL_PDPOPS(sensor_peri->pdp, register_notifier,
				sensor_peri->subdev_pdp,
				type, notifier, data);
	/* PAFSTAT */
	} else if (IS_ENABLED(CONFIG_CAMERA_PAFSTAT)) {
		if (!sensor_peri->pafstat || !sensor_peri->subdev_pafstat) {
			err("invalid PAFSTAT state");
			return -EINVAL;
		}

		ret = CALL_PAFSTATOPS(sensor_peri->pafstat, register_notifier,
				sensor_peri->subdev_pafstat,
				type, notifier, data);
	} else {
		err("no PAF HW");
		return -ENODEV;
	}

	return ret;
}

int unregister_paf_notifier(struct fimc_is_sensor_interface *itf,
			enum itf_vc_stat_type type,
			paf_notifier_t notifier)
{
	struct fimc_is_device_sensor_peri *sensor_peri;
	int ret;

	if (!itf) {
		err("invalid sensor interface");
		return -EINVAL;
	}

	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct fimc_is_device_sensor_peri,
			sensor_interface);
	if (!sensor_peri) {
		err("failed to get sensor_peri");
		return -ENODEV;
	}

	/* PDP */
	if (IS_ENABLED(CONFIG_CAMERA_PDP)) {
		if (!sensor_peri->pdp || !sensor_peri->subdev_pdp) {
			err("invalid PDP state");
			return -EINVAL;
		}

		ret = CALL_PDPOPS(sensor_peri->pdp, unregister_notifier,
				sensor_peri->subdev_pdp,
				type, notifier);
	/* PAFSTAT */
	} else if (IS_ENABLED(CONFIG_CAMERA_PAFSTAT)) {
		if (!sensor_peri->pafstat || !sensor_peri->subdev_pafstat) {
			err("invalid PAFSTAT state");
			return -EINVAL;
		}

		ret = CALL_PAFSTATOPS(sensor_peri->pafstat, unregister_notifier,
				sensor_peri->subdev_pafstat,
				type, notifier);
	} else {
		err("no PAF HW");
		return -ENODEV;
	}

	return ret;
}

int paf_reserved(struct fimc_is_sensor_interface *itf)
{
	return -EINVAL;
}

int init_sensor_interface(struct fimc_is_sensor_interface *itf)
{
	int ret = 0;

	itf->magic = SENSOR_INTERFACE_MAGIC;
	itf->vsync_flag = false;

	/* Default scenario is OTF */
	itf->otf_flag_3aa = true;
	/* TODO: check cis mode */
	itf->cis_mode = ITF_CIS_SMIA_WDR;
	/* OTF default is 3 frame delay */
	itf->diff_bet_sen_isp = itf->otf_flag_3aa ? DIFF_OTF_DELAY : DIFF_M2M_DELAY;

	/* struct fimc_is_cis_interface_ops */
	itf->cis_itf_ops.request_reset_interface = request_reset_interface;
	itf->cis_itf_ops.get_calibrated_size = get_calibrated_size;
	itf->cis_itf_ops.get_bayer_order = get_bayer_order;
	itf->cis_itf_ops.get_min_exposure_time = get_min_exposure_time;
	itf->cis_itf_ops.get_max_exposure_time = get_max_exposure_time;
	itf->cis_itf_ops.get_min_analog_gain = get_min_analog_gain;
	itf->cis_itf_ops.get_max_analog_gain = get_max_analog_gain;
	itf->cis_itf_ops.get_min_digital_gain = get_min_digital_gain;
	itf->cis_itf_ops.get_max_digital_gain = get_max_digital_gain;

	itf->cis_itf_ops.get_vsync_count = get_vsync_count;
	itf->cis_itf_ops.get_vblank_count = get_vblank_count;
	itf->cis_itf_ops.is_vvalid_period = is_vvalid_period;

	itf->cis_itf_ops.request_exposure = request_exposure;
	itf->cis_itf_ops.adjust_exposure = adjust_exposure;

	itf->cis_itf_ops.get_next_frame_timing = get_next_frame_timing;
	itf->cis_itf_ops.get_frame_timing = get_frame_timing;

	itf->cis_itf_ops.request_analog_gain = request_analog_gain;
	itf->cis_itf_ops.request_gain = request_gain;

	itf->cis_itf_ops.adjust_analog_gain = adjust_analog_gain;
	itf->cis_itf_ops.get_next_analog_gain = get_next_analog_gain;
	itf->cis_itf_ops.get_analog_gain = get_analog_gain;

	itf->cis_itf_ops.get_next_digital_gain = get_next_digital_gain;
	itf->cis_itf_ops.get_digital_gain = get_digital_gain;

	itf->cis_itf_ops.is_actuator_available = is_actuator_available;
	itf->cis_itf_ops.is_flash_available = is_flash_available;
	itf->cis_itf_ops.is_companion_available = is_companion_available;
	itf->cis_itf_ops.is_ois_available = is_ois_available;
	itf->cis_itf_ops.is_aperture_available = is_aperture_available;

	itf->cis_itf_ops.get_sensor_frame_timing = get_sensor_frame_timing;
	itf->cis_itf_ops.get_sensor_cur_size = get_sensor_cur_size;
	itf->cis_itf_ops.get_sensor_max_fps = get_sensor_max_fps;
	itf->cis_itf_ops.get_sensor_cur_fps = get_sensor_cur_fps;
	itf->cis_itf_ops.get_hdr_ratio_ctl_by_again = get_hdr_ratio_ctl_by_again;
	itf->cis_itf_ops.get_sensor_use_dgain = get_sensor_use_dgain;
	itf->cis_itf_ops.get_sensor_initial_aperture = get_sensor_initial_aperture;
	itf->cis_itf_ops.set_alg_reset_flag = set_alg_reset_flag;
	itf->cis_ext_itf_ops.get_sensor_hdr_stat = get_sensor_hdr_stat;
	itf->cis_ext_itf_ops.set_3a_alg_res_to_sens = set_3a_alg_res_to_sens;

	itf->cis_itf_ops.set_initial_exposure_of_setfile = set_initial_exposure_of_setfile;

#ifdef USE_NEW_PER_FRAME_CONTROL
	itf->cis_itf_ops.reserved0 = reserved0;
	itf->cis_itf_ops.set_num_of_frame_per_one_3aa = set_num_of_frame_per_one_3aa;
	itf->cis_itf_ops.reserved1 = reserved1;
#else
	itf->cis_itf_ops.set_video_mode_of_setfile = set_video_mode_of_setfile;
	itf->cis_itf_ops.get_num_of_frame_per_one_3aa = get_num_of_frame_per_one_3aa;
	itf->cis_itf_ops.get_offset_from_cur_result = get_offset_from_cur_result;
#endif

	itf->cis_itf_ops.set_cur_uctl_list = set_cur_uctl_list;

	/* TODO: What is diff with apply_frame_settings at event_ops */
	itf->cis_itf_ops.apply_sensor_setting = apply_sensor_setting;

	/* reset exposure and gain for flash */
	itf->cis_itf_ops.request_reset_expo_gain = request_reset_expo_gain;

	itf->cis_itf_ops.set_sensor_info_mode_change = set_sensor_info_mode_change;
	itf->cis_itf_ops.update_sensor_dynamic_meta = update_sensor_dynamic_meta;
	itf->cis_itf_ops.copy_sensor_ctl = copy_sensor_ctl;
	itf->cis_itf_ops.get_module_id = get_module_id;
	itf->cis_itf_ops.get_module_position = get_module_position;
	itf->cis_itf_ops.set_sensor_3a_mode = set_sensor_3a_mode;
	itf->cis_itf_ops.get_initial_exposure_gain_of_sensor = get_initial_exposure_gain_of_sensor;
	itf->cis_ext_itf_ops.change_cis_mode = change_cis_mode;

	/* struct fimc_is_cis_event_ops */
	itf->cis_evt_ops.start_of_frame = start_of_frame;
	itf->cis_evt_ops.end_of_frame = end_of_frame;
	itf->cis_evt_ops.apply_frame_settings = apply_frame_settings;

	/* Actuator interface */
	itf->actuator_itf.soft_landing_table.enable = false;
	itf->actuator_itf.position_table.enable = false;
	itf->actuator_itf.initialized = false;

	itf->actuator_itf_ops.set_actuator_position_table = set_actuator_position_table;
	itf->actuator_itf_ops.set_soft_landing_config = set_soft_landing_config;
	itf->actuator_itf_ops.set_position = set_position;
	itf->actuator_itf_ops.get_cur_frame_position = get_cur_frame_position;
	itf->actuator_itf_ops.get_applied_actual_position = get_applied_actual_position;
	itf->actuator_itf_ops.get_prev_frame_position = get_prev_frame_position;
	itf->actuator_itf_ops.set_af_window_position = set_af_window_position; /* AF window value for M2M AF */
	itf->actuator_itf_ops.get_status = get_status;

	/* Flash interface */
	itf->flash_itf_ops.request_flash = request_flash;
	itf->flash_itf_ops.request_flash_expo_gain = request_flash_expo_gain;
	itf->flash_itf_ops.update_flash_dynamic_meta = update_flash_dynamic_meta;

	/* Aperture interface */
	itf->aperture_itf_ops.set_aperture_value = set_aperture_value;
	itf->aperture_itf_ops.get_aperture_value = get_aperture_value;

	itf->paf_itf_ops.set_paf_param = set_paf_param;
	itf->paf_itf_ops.get_paf_ready = get_paf_ready;
	itf->paf_itf_ops.register_paf_notifier = register_paf_notifier;
	itf->paf_itf_ops.unregister_paf_notifier = unregister_paf_notifier;
	itf->paf_itf_ops.reserved[0] = paf_reserved;
	itf->paf_itf_ops.reserved[1] = paf_reserved;
	itf->paf_itf_ops.reserved[2] = paf_reserved;
	itf->paf_itf_ops.reserved[0] = paf_reserved;

	/* MIPI-CSI interface */
	itf->csi_itf_ops.get_vc_dma_buf = get_vc_dma_buf;
	itf->csi_itf_ops.put_vc_dma_buf = put_vc_dma_buf;
	itf->csi_itf_ops.get_vc_dma_buf_info = get_vc_dma_buf_info;
	itf->csi_itf_ops.get_vc_dma_buf_max_size = get_vc_dma_buf_max_size;
#ifdef CAMERA_REAR2_SENSOR_SHIFT_CROP
	itf->csi_itf_ops.get_sensor_shifted_num = get_sensor_shifted_num;
#endif
	itf->csi_itf_ops.reserved[0] = csi_reserved;
	itf->csi_itf_ops.reserved[1] = csi_reserved;
	itf->csi_itf_ops.reserved[2] = csi_reserved;
#ifndef CAMERA_REAR2_SENSOR_SHIFT_CROP
	itf->csi_itf_ops.reserved[3] = csi_reserved;
#endif

	/* CIS ext2 interface */
	/* Long Term Exposure mode(LTE mode) interface */
	itf->cis_ext2_itf_ops.set_long_term_expo_mode = set_long_term_expo_mode;
	itf->cis_ext2_itf_ops.set_low_noise_mode = set_low_noise_mode;
	itf->cis_ext2_itf_ops.get_sensor_max_dynamic_fps = get_sensor_max_dynamic_fps;
	itf->cis_ext2_itf_ops.get_static_mem = get_static_mem;
	itf->cis_ext2_itf_ops.get_open_close_hint = get_open_close_hint;
	itf->cis_ext2_itf_ops.set_mainflash_duration = set_mainflash_duration;
	itf->cis_ext2_itf_ops.set_previous_dm = set_previous_dm;
	itf->cis_ext_itf_ops.set_adjust_sync = set_adjust_sync;
	itf->cis_ext_itf_ops.request_frame_length_line = request_frame_length_line;
	itf->cis_ext_itf_ops.request_sensitivity = request_sensitivity;

	/* Sensor dual sceanrio interface */
	itf->dual_itf_ops.get_sensor_state = get_sensor_state;
	itf->dual_itf_ops.get_reuse_3a_state = get_reuse_3a_state;
	itf->dual_itf_ops.set_reuse_ae_exposure = set_reuse_ae_exposure;
	itf->dual_itf_ops.reserved[0] = dual_reserved_0;
	itf->dual_itf_ops.reserved[1] = dual_reserved_1;

	return ret;
}

