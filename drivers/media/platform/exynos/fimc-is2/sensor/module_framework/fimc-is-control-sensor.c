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

#include "fimc-is-control-sensor.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-sensor-peri.h"

/* helper */
u64 fimc_is_sensor_convert_us_to_ns(u32 usec)
{
	u64 nsec = 0;

	nsec = (u64)usec * (u64)1000;

	return nsec;
}

u32 fimc_is_sensor_convert_ns_to_us(u64 nsec)
{
	u64 usec = 0;

	usec = nsec;
	do_div(usec, 1000);

	return (u32)usec;
}


u32 fimc_is_sensor_ctl_get_csi_vsync_cnt(struct fimc_is_device_sensor *device)
{
	struct fimc_is_device_csi *csi = NULL;

	FIMC_BUG(!device);

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(device->subdev_csi);
	if (unlikely(!csi)) {
		err("%s, csi in is NULL", __func__);
		return 0;
	}

	return atomic_read(&csi->fcount);
}

void fimc_is_sensor_ctl_update_cis_data(cis_shared_data *cis_data, camera2_sensor_uctl_t *sensor_uctrl)
{
	FIMC_BUG_VOID(!sensor_uctrl);
	FIMC_BUG_VOID(!cis_data);

	memcpy(&cis_data->auto_exposure[CURRENT_FRAME], &cis_data->auto_exposure[NEXT_FRAME], sizeof(ae_setting));
	cis_data->auto_exposure[NEXT_FRAME].exposure =
					fimc_is_sensor_convert_ns_to_us(sensor_uctrl->exposureTime);
	cis_data->auto_exposure[NEXT_FRAME].analog_gain = sensor_uctrl->analogGain;
	cis_data->auto_exposure[NEXT_FRAME].digital_gain = sensor_uctrl->digitalGain;
	cis_data->auto_exposure[NEXT_FRAME].long_exposure =
					fimc_is_sensor_convert_ns_to_us(sensor_uctrl->longExposureTime);
	cis_data->auto_exposure[NEXT_FRAME].long_analog_gain = sensor_uctrl->longAnalogGain;
	cis_data->auto_exposure[NEXT_FRAME].long_digital_gain = sensor_uctrl->longDigitalGain;
	cis_data->auto_exposure[NEXT_FRAME].short_exposure =
					fimc_is_sensor_convert_ns_to_us(sensor_uctrl->shortExposureTime);
	cis_data->auto_exposure[NEXT_FRAME].short_analog_gain = sensor_uctrl->shortAnalogGain;
	cis_data->auto_exposure[NEXT_FRAME].short_digital_gain = sensor_uctrl->shortDigitalGain;

}

void fimc_is_sensor_ctl_get_ae_index(struct fimc_is_device_sensor *device,
					u32 *expo_index,
					u32 *again_index,
					u32 *dgain_index)
{
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct fimc_is_cis *cis = NULL;

	FIMC_BUG_VOID(!device);
	FIMC_BUG_VOID(!expo_index);
	FIMC_BUG_VOID(!again_index);
	FIMC_BUG_VOID(!dgain_index);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(device->subdev_module);
	FIMC_BUG_VOID(!module);
	FIMC_BUG_VOID(!module->private_data);

	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;
	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(sensor_peri->subdev_cis);

	if (cis->ctrl_delay == N_PLUS_TWO_FRAME) {
		*expo_index = NEXT_FRAME;
		*again_index = NEXT_FRAME;
		*dgain_index = NEXT_FRAME;
	} else {
		*expo_index = NEXT_FRAME;
		*again_index = CURRENT_FRAME;
		*dgain_index = CURRENT_FRAME;
	}
}

void fimc_is_sensor_ctl_adjust_ae_setting(struct fimc_is_device_sensor *device,
					ae_setting *setting, cis_shared_data *cis_data)
{
	u32 exposure_index = 0;
	u32 again_index = 0;
	u32 dgain_index = 0;

	FIMC_BUG_VOID(!device);
	FIMC_BUG_VOID(!setting);
	FIMC_BUG_VOID(!cis_data);

	fimc_is_sensor_ctl_get_ae_index(device, &exposure_index, &again_index, &dgain_index);

	if (fimc_is_vender_wdr_mode_on(cis_data)) {
		setting->long_exposure = cis_data->auto_exposure[exposure_index].long_exposure;
		setting->long_analog_gain = cis_data->auto_exposure[again_index].long_analog_gain;
		setting->long_digital_gain = cis_data->auto_exposure[dgain_index].long_digital_gain;
		setting->short_exposure = cis_data->auto_exposure[exposure_index].short_exposure;
		setting->short_analog_gain = cis_data->auto_exposure[again_index].short_analog_gain;
		setting->short_digital_gain = cis_data->auto_exposure[dgain_index].short_digital_gain;
	} else {
		setting->exposure = cis_data->auto_exposure[exposure_index].exposure;
		setting->analog_gain = cis_data->auto_exposure[again_index].analog_gain;
		setting->digital_gain = cis_data->auto_exposure[dgain_index].digital_gain;
	}
}

void fimc_is_sensor_ctl_compensate_expo_gain(struct fimc_is_device_sensor *device,
						struct gain_setting  *adj_gain_setting,
						ae_setting *setting,
						cis_shared_data *cis_data)
{
	FIMC_BUG_VOID(!device);
	FIMC_BUG_VOID(!adj_gain_setting);
	FIMC_BUG_VOID(!setting);
	FIMC_BUG_VOID(!cis_data);

	/* Compensate gain under extremely brightly Illumination */
	if (fimc_is_vender_wdr_mode_on(cis_data)) {
		fimc_is_sensor_peri_compensate_gain_for_ext_br(device, setting->long_exposure,
								&adj_gain_setting->long_again,
								&adj_gain_setting->long_dgain);
		fimc_is_sensor_peri_compensate_gain_for_ext_br(device, setting->short_exposure,
								&adj_gain_setting->short_again,
								&adj_gain_setting->short_dgain);
	} else {
		fimc_is_sensor_peri_compensate_gain_for_ext_br(device, setting->exposure,
								&adj_gain_setting->long_again,
								&adj_gain_setting->long_dgain);
	}

}

int fimc_is_sensor_ctl_set_frame_rate(struct fimc_is_device_sensor *device,
					u32 frame_duration,
					u32 dynamic_duration)
{
	int ret = 0;
	struct v4l2_control v4l2_ctrl;
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	u32 cur_frame_duration = 0;
	u32 cur_dynamic_duration = 0;

	FIMC_BUG(!device);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(device->subdev_module);
	if (unlikely(!module)) {
		err("%s, module in is NULL", __func__);
		module = NULL;
		goto p_err;
	}
	FIMC_BUG(!module);
	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;

	cur_frame_duration = fimc_is_sensor_convert_ns_to_us(sensor_peri->cis.cur_sensor_uctrl.frameDuration);
	cur_dynamic_duration = fimc_is_sensor_convert_ns_to_us(sensor_peri->cis.cur_sensor_uctrl.dynamicFrameDuration);

	/* Set min frame rate(static) */
	if (frame_duration != 0 && cur_frame_duration != frame_duration) {
		v4l2_ctrl.id = V4L2_CID_SENSOR_SET_FRAME_RATE;
		v4l2_ctrl.value = DURATION_US_TO_FPS(frame_duration);
		ret = fimc_is_sensor_s_ctrl(device, &v4l2_ctrl);
		if (ret < 0) {
			err("[%s] SET FRAME RATE fail\n", __func__);
			goto p_err;
		}
		sensor_peri->cis.cur_sensor_uctrl.frameDuration =
				fimc_is_sensor_convert_us_to_ns(frame_duration);
	} else {
		dbg_sensor(1, "[%s] skip min frame duration(%d)\n", __func__, frame_duration);
	}

	/* Set dynamic frame duration */
	if (dynamic_duration != 0) {
		v4l2_ctrl.id = V4L2_CID_SENSOR_SET_FRAME_DURATION;
		v4l2_ctrl.value = dynamic_duration;
		ret = fimc_is_sensor_s_ctrl(device, &v4l2_ctrl);
		if (ret < 0) {
			err("[%s] SET FRAME DURATION fail\n", __func__);
			goto p_err;
		}
		sensor_peri->cis.cur_sensor_uctrl.dynamicFrameDuration =
				fimc_is_sensor_convert_us_to_ns(dynamic_duration);
	} else {
		dbg_sensor(1, "[%s] skip dynamic frame duration(%d)\n", __func__, dynamic_duration);
	}
p_err:
	return ret;
}

int fimc_is_sensor_ctl_adjust_gains(struct fimc_is_device_sensor *device,
				struct fimc_is_sensor_ctl *module_ctl,
				ae_setting *applied_ae_setting,
				struct gain_setting *adj_gain_setting)
{
	int ret = 0;
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	cis_shared_data *cis_data = NULL;
	camera2_sensor_ctl_t *sensor_ctrl = NULL;
	camera2_sensor_uctl_t *sensor_uctrl = NULL;

	u32 sensitivity = 0;
	u32 long_again = 0;
	u32 short_again = 0;
	u32 long_dgain = 0;
	u32 short_dgain = 0;

	FIMC_BUG(!device);
	FIMC_BUG(!module_ctl);
	FIMC_BUG(!applied_ae_setting);
	FIMC_BUG(!adj_gain_setting);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(device->subdev_module);
	if (unlikely(!module)) {
		err("%s, module in is NULL", __func__);
		module = NULL;
		goto p_err;
	}
	FIMC_BUG(!module);
	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;

	sensor_ctrl = &module_ctl->cur_cam20_sensor_ctrl;
	sensor_uctrl = &module_ctl->cur_cam20_sensor_udctrl;
	cis_data = sensor_peri->cis.cis_data;
	FIMC_BUG(!cis_data);

	/* detect manual sensor control or Auto mode
	 * if use manual control, apply sensitivity to ctl meta
	 * else, apply to uctl meta
	 */
	if (sensor_ctrl->sensitivity != 0 && module_ctl->valid_sensor_ctrl == true) {
		sensitivity = sensor_ctrl->sensitivity;
	} else {
		if (sensor_uctrl->sensitivity != 0) {
			sensitivity = sensor_uctrl->sensitivity;
		} else {
			err("[SSDRV] Invalid sensitivity\n");
			ret = -1;
			goto p_err;
		}
	}

	if (fimc_is_vender_wdr_mode_on(cis_data)) {
		long_again = applied_ae_setting->long_analog_gain;
		long_dgain = applied_ae_setting->long_digital_gain;
		short_again = applied_ae_setting->short_analog_gain;
		short_dgain = applied_ae_setting->short_digital_gain;
	} else {
		long_again = applied_ae_setting->analog_gain;
		long_dgain = applied_ae_setting->digital_gain;
		short_again = applied_ae_setting->analog_gain;
		short_dgain = applied_ae_setting->digital_gain;
	}

	adj_gain_setting->sensitivity = sensitivity;
	adj_gain_setting->long_again = long_again;
	adj_gain_setting->short_again = short_again;
	adj_gain_setting->long_dgain = long_dgain;
	adj_gain_setting->short_dgain = short_dgain;

p_err:
	return ret;
}

int fimc_is_sensor_ctl_set_gains(struct fimc_is_device_sensor *device,
				struct gain_setting *adj_gain_setting)
{
	int ret = 0;

	FIMC_BUG(!device);
	FIMC_BUG(!adj_gain_setting);

	/* Set gain */
	ret = fimc_is_sensor_peri_s_analog_gain(device, adj_gain_setting->long_again, adj_gain_setting->short_again);
	if (ret < 0) {
		dbg_sensor(1, "[%s] SET analog gain fail\n", __func__);
		goto p_err;
	}

	ret = fimc_is_sensor_peri_s_digital_gain(device, adj_gain_setting->long_dgain, adj_gain_setting->short_dgain);
	if (ret < 0) {
		dbg_sensor(1, "[%s] SET digital gain fail\n", __func__);
		goto p_err;
	}

p_err:
	return ret;
}

int fimc_is_sensor_ctl_update_gains(struct fimc_is_device_sensor *device,
				u32 *dm_index,
				struct gain_setting *adj_gain_setting)
{
	int ret = 0;
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	cis_shared_data *cis_data = NULL;

	FIMC_BUG(!device);
	FIMC_BUG(!dm_index);
	FIMC_BUG(!adj_gain_setting);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(device->subdev_module);
	if (unlikely(!module)) {
		err("%s, module in is NULL", __func__);
		module = NULL;
		goto p_err;
	}
	FIMC_BUG(!module);
	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;

	cis_data = sensor_peri->cis.cis_data;
	FIMC_BUG(!cis_data);

	sensor_peri->cis.cur_sensor_uctrl.sensitivity = adj_gain_setting->sensitivity;

	if (adj_gain_setting->sensitivity != 0) {
		sensor_peri->cis.cur_sensor_uctrl.analogGain = adj_gain_setting->long_again;
		sensor_peri->cis.cur_sensor_uctrl.digitalGain = adj_gain_setting->long_dgain;
		sensor_peri->cis.cur_sensor_uctrl.longAnalogGain = adj_gain_setting->long_again;
		sensor_peri->cis.cur_sensor_uctrl.longDigitalGain = adj_gain_setting->long_dgain;
		sensor_peri->cis.cur_sensor_uctrl.shortAnalogGain = adj_gain_setting->short_again;
		sensor_peri->cis.cur_sensor_uctrl.shortDigitalGain = adj_gain_setting->short_dgain;

		/* HACK
		 * as analogGain, digitalGain dm moved to udm
		 * It sentents is commented temporary
		 */
		sensor_peri->cis.expecting_sensor_dm[dm_index[0]].sensitivity = adj_gain_setting->sensitivity;

		sensor_peri->cis.expecting_sensor_udm[dm_index[0]].analogGain = adj_gain_setting->long_again;
		sensor_peri->cis.expecting_sensor_udm[dm_index[0]].digitalGain = adj_gain_setting->long_dgain;
		sensor_peri->cis.expecting_sensor_udm[dm_index[0]].longAnalogGain = adj_gain_setting->long_again;
		sensor_peri->cis.expecting_sensor_udm[dm_index[0]].longDigitalGain = adj_gain_setting->long_dgain;
		sensor_peri->cis.expecting_sensor_udm[dm_index[0]].shortAnalogGain = adj_gain_setting->short_again;
		sensor_peri->cis.expecting_sensor_udm[dm_index[0]].shortDigitalGain = adj_gain_setting->short_dgain;
	} else {
		sensor_peri->cis.expecting_sensor_dm[dm_index[0]].sensitivity = sensor_peri->cis.expecting_sensor_dm[dm_index[1]].sensitivity;

		sensor_peri->cis.expecting_sensor_udm[dm_index[0]].analogGain =
			sensor_peri->cis.expecting_sensor_udm[dm_index[1]].analogGain;
		sensor_peri->cis.expecting_sensor_udm[dm_index[0]].digitalGain =
			sensor_peri->cis.expecting_sensor_udm[dm_index[1]].digitalGain;
		sensor_peri->cis.expecting_sensor_udm[dm_index[0]].longAnalogGain =
			sensor_peri->cis.expecting_sensor_udm[dm_index[1]].longAnalogGain;
		sensor_peri->cis.expecting_sensor_udm[dm_index[0]].longDigitalGain =
			sensor_peri->cis.expecting_sensor_udm[dm_index[1]].longDigitalGain;
		sensor_peri->cis.expecting_sensor_udm[dm_index[0]].shortAnalogGain =
			sensor_peri->cis.expecting_sensor_udm[dm_index[1]].shortAnalogGain;
		sensor_peri->cis.expecting_sensor_udm[dm_index[0]].shortDigitalGain =
			sensor_peri->cis.expecting_sensor_udm[dm_index[1]].shortDigitalGain;
	}

p_err:
	return ret;
}

int fimc_is_sensor_ctl_set_exposure(struct fimc_is_device_sensor *device,
					u32 *dm_index,
					u32 long_exposure, u32 short_exposure)
{
	int ret = 0;
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	cis_shared_data *cis_data = NULL;

	FIMC_BUG(!device);
	FIMC_BUG(!dm_index);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(device->subdev_module);
	if (unlikely(!module)) {
		err("%s, module in is NULL", __func__);
		module = NULL;
		goto p_err;
	}
	FIMC_BUG(!module);
	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;

	cis_data = sensor_peri->cis.cis_data;
	FIMC_BUG(!cis_data);

	if (long_exposure != 0 && short_exposure != 0) {
		ret = fimc_is_sensor_peri_s_exposure_time(device, long_exposure, short_exposure);
		if (ret < 0) {
			err("[%s] SET exposure time fail\n", __func__);
		}

		if (fimc_is_vender_wdr_mode_on(cis_data)) {
			sensor_peri->cis.cur_sensor_uctrl.exposureTime = 0;
			sensor_peri->cis.cur_sensor_uctrl.longExposureTime = fimc_is_sensor_convert_us_to_ns(long_exposure);
			sensor_peri->cis.cur_sensor_uctrl.shortExposureTime = fimc_is_sensor_convert_us_to_ns(short_exposure);
		} else {
			sensor_peri->cis.cur_sensor_uctrl.exposureTime = fimc_is_sensor_convert_us_to_ns(short_exposure);
			sensor_peri->cis.cur_sensor_uctrl.longExposureTime = 0;
			sensor_peri->cis.cur_sensor_uctrl.shortExposureTime = 0;
		}
		sensor_peri->cis.expecting_sensor_dm[dm_index[0]].exposureTime = fimc_is_sensor_convert_us_to_ns(short_exposure);

		sensor_peri->cis.expecting_sensor_udm[dm_index[0]].longExposureTime =
			fimc_is_sensor_convert_us_to_ns(long_exposure);
		sensor_peri->cis.expecting_sensor_udm[dm_index[0]].shortExposureTime =
			fimc_is_sensor_convert_us_to_ns(short_exposure);
	} else {
		sensor_peri->cis.expecting_sensor_dm[dm_index[0]].exposureTime = sensor_peri->cis.expecting_sensor_dm[dm_index[1]].exposureTime;

		sensor_peri->cis.expecting_sensor_udm[dm_index[0]].longExposureTime =
			sensor_peri->cis.expecting_sensor_udm[dm_index[1]].longExposureTime;
		sensor_peri->cis.expecting_sensor_udm[dm_index[0]].shortExposureTime =
			sensor_peri->cis.expecting_sensor_udm[dm_index[1]].shortExposureTime;
	}

p_err:
	return ret;
}

void fimc_is_sensor_ctl_frame_evt(struct fimc_is_device_sensor *device)
{
	int ret = 0;
	u32 vsync_count = 0;
	u32 applied_frame_number = 0;
	u32 uctl_frame_index = 0;
	u32 dm_index[2];
	ae_setting applied_ae_setting;
	u32 frame_duration = 0;
	struct gain_setting adj_gain_setting;
	u32 long_exposure = 0, short_exposure = 0;

	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct fimc_is_sensor_ctl *module_ctl = NULL;
	cis_shared_data *cis_data = NULL;

	camera2_sensor_ctl_t *sensor_ctrl = NULL;
	camera2_sensor_uctl_t *sensor_uctrl = NULL;

	struct v4l2_control ctrl;

	FIMC_BUG_VOID(!device);

	vsync_count = fimc_is_sensor_ctl_get_csi_vsync_cnt(device);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(device->subdev_module);
	if (unlikely(!module)) {
		err("%s, module in is NULL", __func__);
		return;
	}
	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;

	if(vsync_count < sensor_peri->sensor_interface.diff_bet_sen_isp + 1) {
		info("%s: vsync_count(%d) < DIFF_BET_SEN_ISP + 1(%d)\n",
			__func__, vsync_count,
			sensor_peri->sensor_interface.diff_bet_sen_isp + 1);
		return;
	}

	applied_frame_number = vsync_count - sensor_peri->sensor_interface.diff_bet_sen_isp;
	uctl_frame_index = applied_frame_number % CAM2P0_UCTL_LIST_SIZE;
	/* dm_index[0] : index for update dm
	 * dm_index[1] : previous updated index for error case
	 */
	dm_index[0] = (applied_frame_number + 2) % EXPECT_DM_NUM;
	dm_index[1] = (applied_frame_number + 1) % EXPECT_DM_NUM;

	module_ctl = &sensor_peri->cis.sensor_ctls[uctl_frame_index];
	cis_data = sensor_peri->cis.cis_data;
	FIMC_BUG_VOID(!cis_data);

	if (sensor_peri->mcu && sensor_peri->mcu->aperture) {
		if (sensor_peri->mcu->aperture->step == APERTURE_STEP_PREPARE) {
			sensor_peri->mcu->aperture->step = APERTURE_STEP_MOVING;
			schedule_work(&sensor_peri->mcu->aperture->aperture_set_work);
		}
	}

	if ((module_ctl->valid_sensor_ctrl == true) ||
		(module_ctl->sensor_frame_number == applied_frame_number && module_ctl->alg_reset_flag == true)) {
		sensor_ctrl = &module_ctl->cur_cam20_sensor_ctrl;
		sensor_uctrl = &module_ctl->cur_cam20_sensor_udctrl;

		if (module_ctl->ctl_frame_number != module_ctl->sensor_frame_number) {
			dbg_sensor(1, "Sen frame number is not match ctl/sensor(%d/%d)\n",
					module_ctl->ctl_frame_number, module_ctl->sensor_frame_number);
		}

		/* update cis_date */
		fimc_is_sensor_ctl_update_cis_data(cis_data, sensor_uctrl);

		/* set applied AE setting */
		memset(&applied_ae_setting, 0, sizeof(ae_setting));
		fimc_is_sensor_ctl_adjust_ae_setting(device, &applied_ae_setting, cis_data);

		/* set frame rate : Limit of max frame duration */
		if (sensor_ctrl->frameDuration != 0 && module_ctl->valid_sensor_ctrl == true)
			frame_duration = fimc_is_sensor_convert_ns_to_us(sensor_ctrl->frameDuration);
		else
			frame_duration = fimc_is_sensor_convert_ns_to_us(sensor_uctrl->frameDuration);

		ret = fimc_is_sensor_ctl_set_frame_rate(device, frame_duration, 0);
		if (ret < 0) {
			err("[%s] frame number(%d) set frame duration fail\n", __func__, applied_frame_number);
		}

		/* Set exposureTime */
		/* TODO: WDR mode */
		if (sensor_ctrl->exposureTime != 0 && module_ctl->valid_sensor_ctrl == true) {
			long_exposure = fimc_is_sensor_convert_ns_to_us(sensor_ctrl->exposureTime);
			short_exposure = fimc_is_sensor_convert_ns_to_us(sensor_ctrl->exposureTime);
		} else {
			if (fimc_is_vender_wdr_mode_on(cis_data)) {
				long_exposure = applied_ae_setting.long_exposure;
				short_exposure = applied_ae_setting.short_exposure;
			} else {
				long_exposure = applied_ae_setting.exposure;
				short_exposure = applied_ae_setting.exposure;
			}
		}

		/* set dynamic duration */
		ctrl.id = V4L2_CID_SENSOR_ADJUST_FRAME_DURATION;
		ctrl.value = 0;
		ret = fimc_is_sensor_peri_adj_ctrl(device, MAX(long_exposure, short_exposure), &ctrl);
		if (ret < 0) {
			err("err!!! ret(%d)", ret);
		}

		sensor_uctrl->dynamicFrameDuration = fimc_is_sensor_convert_us_to_ns(ctrl.value);

		ret = fimc_is_sensor_ctl_set_frame_rate(device, 0,
						fimc_is_sensor_convert_ns_to_us(sensor_uctrl->dynamicFrameDuration));
		if (ret < 0) {
			err("[%s] frame number(%d) set frame duration fail\n", __func__, applied_frame_number);
		}

		ret =  fimc_is_sensor_ctl_set_exposure(device, dm_index, long_exposure, short_exposure);
		if (ret < 0) {
			err("[%s] frame number(%d) set exposure fail\n", __func__, applied_frame_number);
		}

		ret = fimc_is_sensor_ctl_adjust_gains(device, module_ctl, &applied_ae_setting, &adj_gain_setting);
		if (ret < 0) {
			err("[%s] frame number(%d) adjust gains fail\n", __func__, applied_frame_number);
			goto p_err;
		}

		fimc_is_sensor_ctl_compensate_expo_gain(device, &adj_gain_setting, &applied_ae_setting, cis_data);

		/* Set analog and digital gains */
		if (adj_gain_setting.long_again != 0 && adj_gain_setting.long_dgain != 0) {
			ret = fimc_is_sensor_ctl_set_gains(device, &adj_gain_setting);
		} else {
			dbg_sensor(1, "[%s] Skip to set gain (%d,%d)\n",
						__func__, adj_gain_setting.long_again, adj_gain_setting.long_dgain);
		}

		if (ret < 0) {
			err("[%s] frame number(%d) set gains fail\n", __func__, applied_frame_number);
		}

		ret = fimc_is_sensor_ctl_update_gains(device, dm_index, &adj_gain_setting);
		if (ret < 0) {
			err("[%s] frame number(%d) update gains fail\n", __func__, applied_frame_number);
		}
	} else {
		if (module_ctl->alg_reset_flag == false) {
			dbg_sensor(1, "[%s] frame number(%d)  alg_reset_flag (%d)\n", __func__,
					applied_frame_number, module_ctl->alg_reset_flag);
		} else if (module_ctl->sensor_frame_number == applied_frame_number) {
			err("[%s] frame number(%d) Shot process of AE is too slow, alg_reset_flag (%d)\n", __func__,
					applied_frame_number, module_ctl->alg_reset_flag);
		} else {
			info("module->sen_framenum(%d), applied_frame_num(%d), alg_reset_flag (%d)\n",
					module_ctl->sensor_frame_number, applied_frame_number, module_ctl->alg_reset_flag);
		}
	}

	if (sensor_peri->subdev_flash != NULL) {
		/* Pre-Flash on, Torch on/off */
		ret = fimc_is_sensor_peri_pre_flash_fire(device->subdev_module, &vsync_count);
	}

	/* Warning! Aperture mode should be set before setting ois mode */
	if (sensor_peri->mcu && sensor_peri->mcu->ois) {
		ret = CALL_OISOPS(sensor_peri->mcu->ois, ois_set_mode, sensor_peri->subdev_mcu, sensor_peri->mcu->ois->ois_mode);
		if (ret < 0) {
			err("[SEN:%d] v4l2_subdev_call(ois_mode_change, mode:%d) is fail(%d)",
				module->sensor_id, sensor_peri->mcu->ois->ois_mode, ret);
			goto p_err;
		}

		ret = CALL_OISOPS(sensor_peri->mcu->ois, ois_set_coef, sensor_peri->subdev_mcu, sensor_peri->mcu->ois->coef);
		if (ret < 0) {
			err("[SEN:%d] v4l2_subdev_call(ois_set_coef, coef:%d) is fail(%d)",
				module->sensor_id, sensor_peri->mcu->ois->coef, ret);
			goto p_err;
		}
	}

	/* TODO */
	/* FuncCompanionChangeConfig */

p_err:
	return;
}

void fimc_is_sensor_ois_update(struct fimc_is_device_sensor *device)
{
	int ret = 0;
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG_VOID(!device);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(device->subdev_module);
	if (unlikely(!module)) {
		err("%s, module in is NULL", __func__);
		return;
	}
	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;

	if (sensor_peri->subdev_ois) {
		ret = CALL_OISOPS(sensor_peri->ois, ois_set_mode, sensor_peri->subdev_ois, sensor_peri->ois->ois_mode);
		if (ret < 0) {
			err("[SEN:%d] v4l2_subdev_call(ois_mode_change, mode:%d) is fail(%d)",
						module->sensor_id, sensor_peri->ois->ois_mode, ret);
			goto p_err;
		}

		ret = CALL_OISOPS(sensor_peri->ois, ois_set_coef, sensor_peri->subdev_ois, sensor_peri->ois->coef);
		if (ret < 0) {
			err("[SEN:%d] v4l2_subdev_call(ois_set_coef, coef:%d) is fail(%d)",
						module->sensor_id, sensor_peri->ois->coef, ret);
			goto p_err;
		}
	}

p_err:
	return;
}
#ifdef USE_OIS_SLEEP_MODE
void fimc_is_sensor_ois_start(struct fimc_is_device_sensor *device)
{
	int ret = 0;
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG_VOID(!device);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(device->subdev_module);
	if (unlikely(!module)) {
		err("%s, module in is NULL", __func__);
		return;
	}
	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;

	if (sensor_peri->subdev_ois) {
		ret = CALL_OISOPS(sensor_peri->ois, ois_start, sensor_peri->subdev_ois);
		if (ret < 0) {
			err("[SEN:%d] v4l2_subdev_call(ois_mode_change, mode:%d) is fail(%d)",
						module->sensor_id, sensor_peri->ois->ois_mode, ret);
			goto p_err;
		}
	}
p_err:
	return;
}

void fimc_is_sensor_ois_stop(struct fimc_is_device_sensor *device)
{
	int ret = 0;
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG_VOID(!device);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(device->subdev_module);
	if (unlikely(!module)) {
		err("%s, module in is NULL", __func__);
		return;
	}
	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;

	if (sensor_peri->subdev_ois) {
		ret = CALL_OISOPS(sensor_peri->ois, ois_stop, sensor_peri->subdev_ois);
		if (ret < 0) {
			err("[SEN:%d] v4l2_subdev_call(ois_mode_change, mode:%d) is fail(%d)",
						module->sensor_id, sensor_peri->ois->ois_mode, ret);
			goto p_err;
		}
	}
p_err:
	return;
}
#endif

int fimc_is_sensor_ctl_adjust_sync(struct fimc_is_device_sensor *device, u32 sync)
{
	int ret = 0;
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	cis_shared_data *cis_data = NULL;

	FIMC_BUG(!device);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(device->subdev_module);
	if (unlikely(!module)) {
		err("%s, module in is NULL", __func__);
		module = NULL;
		goto p_err;
	}
	FIMC_BUG(!module);
	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;

	cis_data = sensor_peri->cis.cis_data;
	FIMC_BUG(!cis_data);
	ret = CALL_CISOPS(&sensor_peri->cis, cis_set_adjust_sync, sensor_peri->subdev_cis, sync);

p_err:
	return ret;
}
