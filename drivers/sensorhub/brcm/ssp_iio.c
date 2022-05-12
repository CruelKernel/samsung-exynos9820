/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */
#include "ssp.h"
#include "ssp_iio.h"
#include "ssp_sensorhub.h"
#include <linux/string.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/events.h>
#include <linux/iio/buffer.h>
#include <linux/iio/buffer_impl.h>
#include <linux/iio/types.h>
#include <linux/iio/kfifo_buf.h>
#if defined(CONFIG_BATTERY_SAMSUNG_V2)
#include "../../battery_v2/include/sec_charging_common.h"
#else
#include <linux/battery/sec_charging_common.h>
#endif

static struct sensor_info info_table[] = {
	SENSOR_INFO_ACCELEROMETER,
	SENSOR_INFO_GEOMAGNETIC,
	SENSOR_INFO_GEOMAGNETIC_POWER,
	SENSOR_INFO_GEOMAGNETIC_UNCAL,
	SENSOR_INFO_GYRO,
	SENSOR_INFO_GYRO_UNCALIBRATED,
	SENSOR_INFO_INTERRUPT_GYRO,
	SENSOR_INFO_PRESSURE,
	SENSOR_INFO_LIGHT,
	SENSOR_INFO_LIGHT_IR,
	SENSOR_INFO_LIGHT_FLICKER,
	SENSOR_INFO_PROXIMITY,
	SENSOR_INFO_PROXIMITY_ALERT,
	SENSOR_INFO_PROXIMITY_RAW,
	SENSOR_INFO_ROTATION_VECTOR,
	SENSOR_INFO_GAME_ROTATION_VECTOR,
	SENSOR_INFO_SIGNIFICANT_MOTION,
	SENSOR_INFO_STEP_DETECTOR,
	SENSOR_INFO_STEP_COUNTER,
	SENSOR_INFO_TILT_DETECTOR,
	SENSOR_INFO_PICK_UP_GESTURE,
	SENSOR_INFO_SCONTEXT,
	SENSOR_INFO_LIGHT_CCT,
	SENSOR_INFO_THERMISTOR,
	SENSOR_INFO_PROXIMITY_POCKET,
	SENSOR_INFO_ACCEL_UNCALIBRATED,
	SENSOR_INFO_META,
	SENSOR_INFO_WAKE_UP_MOTION,
	SENSOR_INFO_MOVE_DETECTOR,
	SENSOR_INFO_CALL_GESTURE,
	SENSOR_INFO_PROXIMITY_ADC_CALIB,
	SENSOR_INFO_UNCAL_LIGHT,
	SENSOR_INFO_POCKET_MODE,
	SENSOR_INFO_LED_COVER_EVENT,
	SENSOR_INFO_AUTO_ROTATION,
	SENSOR_INFO_SAR_BACKOFF_MOTION,
};

#define IIO_ST(si, rb, sb, sh)	\
	{ .sign = si, .realbits = rb, .storagebits = sb, .shift = sh }

int info_table_size =  sizeof(info_table)/sizeof(struct sensor_info);

static struct sensor_info sensors_info[SENSOR_MAX];
static struct iio_chan_spec indio_channels[SENSOR_MAX];

static const struct iio_info indio_info = {
	.driver_module = THIS_MODULE,
};

int initialize_iio_buffer_and_device(struct iio_dev *indio_dev, struct ssp_data *data)
{
	int iRet = 0;

	iRet = ssp_iio_configure_ring(indio_dev);
	if (iRet)
		goto err_config_ring_buffer;

	iRet = iio_device_register(indio_dev);
	if (iRet)
		goto err_register_device;

	iio_device_set_drvdata(indio_dev, data);

	return iRet;

err_register_device:
	pr_err("[SSP]: failed to register %s device\n", indio_dev->name);
	iio_device_unregister(indio_dev);
err_config_ring_buffer:
	pr_err("[SSP]: failed to configure %s buffer\n", indio_dev->name);
	ssp_iio_unconfigure_ring(indio_dev);

	return iRet;
}

static int ssp_push_iio_buffer(struct iio_dev *indio_dev, u64 timestamp, u8 *data, int data_len)
{
	u8 buf[data_len + sizeof(timestamp)];
	int iRet = 0, retry = 3;

	memcpy(buf, data, data_len);
	memcpy(&buf[data_len], &timestamp, sizeof(timestamp));

	mutex_lock(&indio_dev->mlock);

	do {
		if(retry != 3)
			usleep_range(150, 151);

		iRet = iio_push_to_buffers(indio_dev, buf);
	} while(iRet < 0 && retry-- > 0);

	if(retry == 0 && iRet < 0)
		pr_err("[SSP] %s - %s push fail erro %d", __func__, indio_dev->name, iRet);

	mutex_unlock(&indio_dev->mlock);

	return 0;
}

void report_meta_data(struct ssp_data *data, int sensor_type, struct sensor_value *s)
{
	pr_info("[SSP]: %s - what: %d, sensor: %d\n", __func__,
		s->meta_data.what, s->meta_data.sensor);

	if (s->meta_data.sensor == ACCELEROMETER_SENSOR) {
		s16 accel_buf[3];

		memset(accel_buf, 0, sizeof(s16) * 3);
		ssp_push_iio_buffer(data->indio_dev[ACCELEROMETER_SENSOR], 0, (u8 *)accel_buf, sizeof(accel_buf));
	} else if (s->meta_data.sensor == GYROSCOPE_SENSOR) {
		int gyro_buf[3];

		memset(gyro_buf, 0, sizeof(int) * 3);
		ssp_push_iio_buffer(data->indio_dev[GYROSCOPE_SENSOR], 0, (u8 *)gyro_buf, sizeof(gyro_buf));
	} else if (s->meta_data.sensor == GYRO_UNCALIB_SENSOR) {
		s32 uncal_gyro_buf[6];

		memset(uncal_gyro_buf, 0, sizeof(uncal_gyro_buf));
		ssp_push_iio_buffer(data->indio_dev[GYRO_UNCALIB_SENSOR], 0, (u8 *)uncal_gyro_buf, sizeof(uncal_gyro_buf));
	} else if (s->meta_data.sensor == GEOMAGNETIC_SENSOR) {
		u8 mag_buf[8];

		memset(mag_buf, 0, sizeof(mag_buf));
		ssp_push_iio_buffer(data->indio_dev[GEOMAGNETIC_SENSOR], 0, (u8 *)mag_buf, sizeof(mag_buf));
	} else if (s->meta_data.sensor == GEOMAGNETIC_UNCALIB_SENSOR) {
		u8 uncal_mag_buf[13];

		memset(uncal_mag_buf, 0, sizeof(uncal_mag_buf));
		ssp_push_iio_buffer(data->indio_dev[GEOMAGNETIC_UNCALIB_SENSOR], 0, (u8 *)uncal_mag_buf, sizeof(uncal_mag_buf));
	} else if (s->meta_data.sensor == PRESSURE_SENSOR) {
		int pressure_buf[3];

		memset(pressure_buf, 0, sizeof(int) * 3);
		ssp_push_iio_buffer(data->indio_dev[PRESSURE_SENSOR], 0, (u8 *)pressure_buf, sizeof(pressure_buf));
	} else if (s->meta_data.sensor == ROTATION_VECTOR) {
		int rot_buf[5];

		memset(rot_buf, 0, sizeof(int) * 5);
		ssp_push_iio_buffer(data->indio_dev[ROTATION_VECTOR], 0, (u8 *)rot_buf, sizeof(rot_buf));
	} else if (s->meta_data.sensor == GAME_ROTATION_VECTOR) {
		int grot_buf[5];

		memset(grot_buf, 0, sizeof(int) * 5);
		ssp_push_iio_buffer(data->indio_dev[GAME_ROTATION_VECTOR], 0, (u8 *)grot_buf, sizeof(grot_buf));
	} else if (s->meta_data.sensor == STEP_DETECTOR) {
		u8 step_buf[1] = {0};

		ssp_push_iio_buffer(data->indio_dev[STEP_DETECTOR], 0, (u8 *)step_buf, sizeof(step_buf));
	} else {
		ssp_push_iio_buffer(data->meta_indio_dev, 0, (u8 *)(&s->meta_data), 8);
	}
}

void report_iio_data(struct ssp_data *data, int type, struct sensor_value *sensor_data)
{
	memcpy(&data->buf[type], sensor_data, sensors_info[type].get_data_len);
	ssp_push_iio_buffer(data->indio_dev[type], sensor_data->timestamp, (u8 *)(&data->buf[type]), sensors_info[type].report_data_len);
}

void report_acc_data(struct ssp_data *data, int sensor_type, struct sensor_value *accdata)
{
	//this exception is for CTS suspend test
	if ((accdata->x != 0 || accdata->y != 0 || accdata->z != 0)
			&& accdata->timestamp == 0)
		return;
	report_iio_data(data, ACCELEROMETER_SENSOR, accdata);
}

void report_gyro_data(struct ssp_data *data, int sensor_type, struct sensor_value *gyrodata)
{
	report_iio_data(data, GYROSCOPE_SENSOR, gyrodata);
}

void report_interrupt_gyro_data(struct ssp_data *data, int sensor_type, struct sensor_value *gyrodata)
{
	report_iio_data(data, INTERRUPT_GYRO_SENSOR, gyrodata);
}

void report_geomagnetic_raw_data(struct ssp_data *data, int sensor_type, struct sensor_value *magrawdata)
{
	data->buf[GEOMAGNETIC_RAW].x = magrawdata->x;
	data->buf[GEOMAGNETIC_RAW].y = magrawdata->y;
	data->buf[GEOMAGNETIC_RAW].z = magrawdata->z;
}

void report_mag_data(struct ssp_data *data, int sensor_type, struct sensor_value *magdata)
{
	report_iio_data(data, GEOMAGNETIC_SENSOR, magdata);
}

void report_mag_uncaldata(struct ssp_data *data, int sensor_type, struct sensor_value *magdata)
{
	report_iio_data(data, GEOMAGNETIC_UNCALIB_SENSOR, magdata);
}

void report_uncalib_gyro_data(struct ssp_data *data, int sensor_type, struct sensor_value *gyrodata)
{
	report_iio_data(data, GYRO_UNCALIB_SENSOR, gyrodata);
}

void report_sig_motion_data(struct ssp_data *data, int sensor_type,
	struct sensor_value *sig_motion_data)
{
	report_iio_data(data, SIG_MOTION_SENSOR, sig_motion_data);
	wake_lock_timeout(&data->ssp_wake_lock, 0.3*HZ);
}

void report_rot_data(struct ssp_data *data, int sensor_type, struct sensor_value *rotdata)
{
	report_iio_data(data, ROTATION_VECTOR, rotdata);
}

void report_game_rot_data(struct ssp_data *data, int sensor_type, struct sensor_value *grvec_data)
{
	report_iio_data(data, GAME_ROTATION_VECTOR, grvec_data);
}

void report_pressure_data(struct ssp_data *data, int sensor_type, struct sensor_value *predata)
{
	int temp[3] = {0, };

	data->buf[PRESSURE_SENSOR].pressure =
		predata->pressure - data->iPressureCal;
	data->buf[PRESSURE_SENSOR].temperature = predata->temperature;

	temp[0] = data->buf[PRESSURE_SENSOR].pressure;
	temp[1] = data->buf[PRESSURE_SENSOR].temperature;
	temp[2] = data->sealevelpressure;

	ssp_push_iio_buffer(data->indio_dev[PRESSURE_SENSOR], predata->timestamp, (u8 *)temp, sizeof(temp));
}

void report_light_data(struct ssp_data *data, int sensor_type, struct sensor_value *lightdata)
{
	report_iio_data(data, sensor_type, lightdata);
	if (data->light_log_cnt < 8) {
		char *header = sensor_type == UNCAL_LIGHT_SENSOR ? "#>UL" : "#>L";
#ifdef CONFIG_SENSORS_SSP_LIGHT_REPORT_LUX
#ifdef CONFIG_SENSORS_SSP_LIGHT_ADDING_LUMINANCE
		ssp_dbg("[SSP] %s lux=%u cct=%d r=%d g=%d b=%d c=%d atime=%d again=%d brightness=%d",
			header, data->buf[sensor_type].lux, data->buf[sensor_type].cct,
			data->buf[sensor_type].r, data->buf[sensor_type].g, data->buf[sensor_type].b,
			data->buf[sensor_type].w, data->buf[sensor_type].a_time, data->buf[sensor_type].a_gain,
			data->buf[sensor_type].brightness);
#else
		ssp_dbg("[SSP] %s lux=%u cct=%d r=%d g=%d b=%d c=%d atime=%d again=%d",
			header, data->buf[sensor_type].lux, data->buf[sensor_type].cct,
			data->buf[sensor_type].r, data->buf[sensor_type].g, data->buf[sensor_type].b,
			data->buf[sensor_type].w, data->buf[sensor_type].a_time, data->buf[sensor_type].a_gain);
#endif
#else
		ssp_dbg("[SSP] %s r=%d g=%d b=%d c=%d atime=%d again=%d",
			header, data->buf[sensor_type].r, data->buf[sensor_type].g, data->buf[sensor_type].b,
			data->buf[sensor_type].w, data->buf[sensor_type].a_time, data->buf[sensor_type].a_gain);
#endif
		data->light_log_cnt++;
	}
}

#ifdef CONFIG_SENSORS_SSP_IRDATA_FOR_CAMERA
void report_light_ir_data(struct ssp_data *data, int sensor_type, struct sensor_value *lightirdata)
{
	report_iio_data(data, LIGHT_IR_SENSOR, lightirdata);

	if (data->light_ir_log_cnt < 3) {
		ssp_dbg("[SSP]#>IR irdata=%d r=%d g=%d b=%d c=%d atime/brightness_level=%d again=%d \n",
			data->buf[LIGHT_IR_SENSOR].irdata, data->buf[LIGHT_IR_SENSOR].ir_r, data->buf[LIGHT_IR_SENSOR].ir_g,
			data->buf[LIGHT_IR_SENSOR].ir_b, data->buf[LIGHT_IR_SENSOR].ir_w, data->buf[LIGHT_IR_SENSOR].ir_a_time,
			data->buf[LIGHT_IR_SENSOR].ir_a_gain);
		data->light_ir_log_cnt++;
	}

}
#endif
void report_light_cct_data(struct ssp_data *data, int sensor_type, struct sensor_value *lightdata)
{
	report_iio_data(data, LIGHT_CCT_SENSOR, lightdata);

	if (data->light_cct_log_cnt< 3) {
#ifdef CONFIG_SENSORS_SSP_LIGHT_REPORT_LUX
#ifdef CONFIG_SENSORS_SSP_LIGHT_LUX_RAW
		ssp_dbg("[SSP] #>CCT lux=%u cct=%d r=%d g=%d b=%d c=%d atime=%d again=%d lux_raw=%d",
			data->buf[LIGHT_CCT_SENSOR].lux, data->buf[LIGHT_CCT_SENSOR].cct,
			data->buf[LIGHT_CCT_SENSOR].r, data->buf[LIGHT_CCT_SENSOR].g, data->buf[LIGHT_CCT_SENSOR].b,
			data->buf[LIGHT_CCT_SENSOR].w, data->buf[LIGHT_CCT_SENSOR].a_time, data->buf[LIGHT_CCT_SENSOR].a_gain,
			data->buf[LIGHT_CCT_SENSOR].lux_raw);
#else
	ssp_dbg("[SSP] #>CCT lux=%u cct=%d r=%d g=%d b=%d c=%d atime=%d again=%d",
			data->buf[LIGHT_CCT_SENSOR].lux, data->buf[LIGHT_CCT_SENSOR].cct,
			data->buf[LIGHT_CCT_SENSOR].r, data->buf[LIGHT_CCT_SENSOR].g, data->buf[LIGHT_CCT_SENSOR].b,
			data->buf[LIGHT_CCT_SENSOR].w, data->buf[LIGHT_CCT_SENSOR].a_time, data->buf[LIGHT_CCT_SENSOR].a_gain);

#endif
#else
		ssp_dbg("[SSP] #>CCT r=%d g=%d b=%d c=%d atime=%d again=%d",
			data->buf[LIGHT_CCT_SENSOR].r, data->buf[LIGHT_CCT_SENSOR].g, data->buf[LIGHT_CCT_SENSOR].b,
			data->buf[LIGHT_CCT_SENSOR].w, data->buf[LIGHT_CCT_SENSOR].a_time, data->buf[LIGHT_CCT_SENSOR].a_gain);
#endif
		data->light_cct_log_cnt++;
	}
}

void report_light_flicker_data(struct ssp_data *data, int sensor_type, struct sensor_value *lightFlickerData)
{
	report_iio_data(data, LIGHT_FLICKER_SENSOR, lightFlickerData);
}

void report_prox_data(struct ssp_data *data, int sensor_type, struct sensor_value *proxdata)
{
	u32 ts_high, ts_low;

        usleep_range(500, 1000);

	report_iio_data(data, PROXIMITY_SENSOR, proxdata);
	wake_lock_timeout(&data->ssp_wake_lock, 0.3*HZ);

        ts_high = (u32)((proxdata->timestamp)>>32);
        ts_low = (u32)((proxdata->timestamp)&0x00000000ffffffff);
#if defined(CONFIG_SENSORS_SSP_PROX_LIGHT_DIFF)
	   ssp_dbg("[SSP] Proximity Sensor Detect : %u, raw : %u light_diff: %d ts : %llu %d %d\n",
		proxdata->prox_detect, proxdata->prox_adc, proxdata->light_diff, proxdata->timestamp, ts_high, ts_low);
#else
        ssp_dbg("[SSP] Proximity Sensor Detect : %u, raw : %u ts : %llu %d %d\n",
		proxdata->prox_detect, proxdata->prox_adc, proxdata->timestamp, ts_high, ts_low);
#endif
}

void report_prox_raw_data(struct ssp_data *data, int sensor_type,
	struct sensor_value *proxrawdata)
{
	if (data->uFactoryProxAvg[0]++ >= PROX_AVG_READ_NUM) {
		data->uFactoryProxAvg[2] /= PROX_AVG_READ_NUM;
		data->buf[PROXIMITY_RAW].prox_raw[1] = (u16)data->uFactoryProxAvg[1];
		data->buf[PROXIMITY_RAW].prox_raw[2] = (u16)data->uFactoryProxAvg[2];
		data->buf[PROXIMITY_RAW].prox_raw[3] = (u16)data->uFactoryProxAvg[3];

		data->uFactoryProxAvg[0] = 0;
		data->uFactoryProxAvg[1] = 0;
		data->uFactoryProxAvg[2] = 0;
		data->uFactoryProxAvg[3] = 0;
	} else {
		data->uFactoryProxAvg[2] += proxrawdata->prox_raw[0];

		if (data->uFactoryProxAvg[0] == 1)
			data->uFactoryProxAvg[1] = proxrawdata->prox_raw[0];
		else if (proxrawdata->prox_raw[0] < data->uFactoryProxAvg[1])
			data->uFactoryProxAvg[1] = proxrawdata->prox_raw[0];

		if (proxrawdata->prox_raw[0] > data->uFactoryProxAvg[3])
			data->uFactoryProxAvg[3] = proxrawdata->prox_raw[0];
	}
	
	data->buf[PROXIMITY_RAW].prox_raw[0] = proxrawdata->prox_raw[0];
	
	report_iio_data(data, PROXIMITY_RAW, proxrawdata);	
}

void report_proximity_pocket_data(struct ssp_data *data, int sensor_type, struct sensor_value *proximity_pocket)
{
	usleep_range(500, 1000);

	report_iio_data(data, PROXIMITY_POCKET, proximity_pocket);
	wake_lock_timeout(&data->ssp_wake_lock, 0.3*HZ);
}

void report_prox_alert_data(struct ssp_data *data, int sensor_type, struct sensor_value *prox_alert_data)
{
	report_iio_data(data, PROXIMITY_ALERT_SENSOR, prox_alert_data);
	wake_lock_timeout(&data->ssp_wake_lock, 0.3*HZ);
    
        ssp_dbg("[SSP] Proximity alert Sensor Detect : %d, raw : %u ts : %llu\n",
            prox_alert_data->prox_alert_detect, prox_alert_data->prox_alert_adc, prox_alert_data->timestamp);
}

#ifdef CONFIG_SENSORS_SSP_PROX_ADC_CAL
void report_prox_adc_calib_data(struct ssp_data *data, int sensor_type, struct sensor_value *proxcalibdata)
{
	data->ProxOffset = proxcalibdata->proxcal_offset;
	data->uProxHiThresh = proxcalibdata->proxcal_thresh_hi;
	data->uProxLoThresh = proxcalibdata->proxcal_thresh_lo;
	data->uProxLoThresh_detect = proxcalibdata->proxcal_thresh_lo;
	proximity_save_calibration(data);
}
#endif

void report_step_det_data(struct ssp_data *data, int sensor_type,
		struct sensor_value *stepdet_data)
{
	data->buf[STEP_DETECTOR].step_det = stepdet_data->step_det;
	ssp_push_iio_buffer(data->indio_dev[STEP_DETECTOR], stepdet_data->timestamp, (u8 *)&stepdet_data->step_det, 1);
}

void report_step_cnt_data(struct ssp_data *data, int sensor_type,
	struct sensor_value *sig_motion_data)
{
	data->buf[STEP_COUNTER].step_diff = sig_motion_data->step_diff;
	data->step_count_total += data->buf[STEP_COUNTER].step_diff;
	ssp_push_iio_buffer(data->indio_dev[STEP_COUNTER], sig_motion_data->timestamp,
			(u8 *)(&data->step_count_total), sensors_info[STEP_COUNTER].report_data_len);
}

void report_tilt_data(struct ssp_data *data, int sensor_type,
		struct sensor_value *tilt_data)
{
	report_iio_data(data, TILT_DETECTOR, tilt_data);
	wake_lock_timeout(&data->ssp_wake_lock, 0.3*HZ);
	pr_err("[SSP]: %s: %d", __func__,  tilt_data->tilt_detector);
}

void report_pickup_data(struct ssp_data *data, int sensor_type,
		struct sensor_value *pickup_data)
{
	report_iio_data(data, PICKUP_GESTURE, pickup_data);
	wake_lock_timeout(&data->ssp_wake_lock, 0.3*HZ);
	pr_err("[SSP]: %s: %d", __func__,  pickup_data->pickup_gesture);
}

void report_wakeup_motion_data(struct ssp_data *data, int sensor_type,
		struct sensor_value *wakeup_motion_data)
{
	memcpy(&data->buf[WAKE_UP_MOTION], &wakeup_motion_data->wakeup_move_event[0], sensors_info[WAKE_UP_MOTION].get_data_len);
	ssp_push_iio_buffer(data->indio_dev[WAKE_UP_MOTION], wakeup_motion_data->timestamp, 
		(u8 *)(&data->buf[WAKE_UP_MOTION]), sensors_info[WAKE_UP_MOTION].report_data_len);
	wake_lock_timeout(&data->ssp_wake_lock, 0.3*HZ);
	pr_err("[SSP]: %s: %d", __func__,  wakeup_motion_data->wakeup_move_event[0]);
}

void report_move_detector_data(struct ssp_data *data, int sensor_type,
		struct sensor_value *move_detector_data)
{
	memcpy(&data->buf[MOVE_DETECTOR], &move_detector_data->wakeup_move_event[1], sensors_info[MOVE_DETECTOR].get_data_len);
	ssp_push_iio_buffer(data->indio_dev[MOVE_DETECTOR], move_detector_data->timestamp, 
		(u8 *)(&data->buf[MOVE_DETECTOR]), sensors_info[MOVE_DETECTOR].report_data_len);
	wake_lock_timeout(&data->ssp_wake_lock, 0.3*HZ);
	pr_err("[SSP]: %s: %d", __func__,  move_detector_data->wakeup_move_event[1]);
}

void report_call_gesture_data(struct ssp_data *data, int sensor_type,
		struct sensor_value *call_gesture_data)
{
	report_iio_data(data, CALL_GESTURE, call_gesture_data);
	wake_lock_timeout(&data->ssp_wake_lock, 0.3*HZ);
	pr_err("[SSP]: %s: %d", __func__,  call_gesture_data->call_gesture);
}

void report_scontext_data(struct ssp_data *data, int sensor_type,
		struct sensor_value *scontextbuf)
{
	short start, end;
	char printbuf[72] = {0, };

	memcpy(printbuf, scontextbuf, 72);
	memcpy(&start, printbuf+4, sizeof(short));
	memcpy(&end, printbuf+6, sizeof(short));

	if (end - start + 1 <= 64) {
		ssp_sensorhub_log("ssp_AlgoPacket", printbuf + 8, end - start + 1);
	} else {
		pr_err("[SSP] : ssp_AlgoPacket packet error start:%d end:%d", start, end);
	}

	mutex_lock(&data->indio_scontext_dev->mlock);
	iio_push_to_buffers(data->indio_scontext_dev, scontextbuf);
	mutex_unlock(&data->indio_scontext_dev->mlock);

	wake_lock_timeout(&data->ssp_wake_lock, 0.3*HZ);
}
void report_uncalib_accel_data(struct ssp_data *data, int sensor_type, struct sensor_value *acceldata)
{
	report_iio_data(data, ACCEL_UNCALIB_SENSOR, acceldata);
}

void report_pocket_mode_data(struct ssp_data *data, int sensor_type,
		struct sensor_value *pocket_data)
{
	report_iio_data(data, POCKET_MODE_SENSOR, pocket_data);
	wake_lock_timeout(&data->ssp_wake_lock, 0.3*HZ);
	pr_err("[SSP]: %s: state = %d reason = %d base = %d current = %d temp = %d ts: %llu", __func__,
		pocket_data->pocket_mode, pocket_data->pocket_reason, pocket_data->pocket_base_proxy,
		pocket_data->pocket_current_proxy, pocket_data->pocket_temp, pocket_data->timestamp);
}

void report_led_cover_event_data(struct ssp_data *data, int sensor_type,
		struct sensor_value *led_cover_event_data)
{
	report_iio_data(data, LED_COVER_EVENT_SENSOR, led_cover_event_data);
	wake_lock_timeout(&data->ssp_wake_lock, 0.3*HZ);
	pr_err("[SSP]: %s: %d ts: %llu", __func__,  led_cover_event_data->led_cover_event, led_cover_event_data->timestamp);
}

void report_auto_rotation_data(struct ssp_data *data, int sensor_type,
		struct sensor_value *auto_rotation_data)
{
	report_iio_data(data, AUTO_ROTATION_SENSOR, auto_rotation_data);
	wake_lock_timeout(&data->ssp_wake_lock, 0.3*HZ);
	pr_err("[SSP]: %s: %d ts: %llu", __func__, auto_rotation_data->auto_rotation_event, auto_rotation_data->timestamp);
}

void report_sar_backoff_motion_data(struct ssp_data *data, int sensor_type,
		struct sensor_value *auto_rotation_data)
{
	report_iio_data(data, SAR_BACKOFF_MOTION, auto_rotation_data);
	wake_lock_timeout(&data->ssp_wake_lock, 0.3*HZ);
	pr_err("[SSP]: %s: %d ts: %llu", __func__, auto_rotation_data->auto_rotation_event, auto_rotation_data->timestamp);
}

#define THM_UP		0
#define THM_SUB		1
short thermistor_rawToTemperature(struct ssp_data *data, int type, s16 raw)
{
	int startTemp = -20, i = 0, diff;
	int temperature = 0, temperature_norm = 0;

	for (i = 0; i < ARRAY_SIZE(data->tempTable_up) - 1; i++) {
		if (type == THM_UP) {
			if (raw > data->tempTable_up[0]) {
				temperature = -20; // MIN value
				break;
			} else if (raw <= data->tempTable_up[22]) {
				temperature = 90; // MAX value
				break;
			} else if (data->tempTable_up[i] >= raw && raw > data->tempTable_up[i + 1]) {
				diff = data->tempTable_up[i] - data->tempTable_up[i + 1];
				//value : diff = temperature : 5
				temperature = startTemp + (5 * i) + ((data->tempTable_up[i] - raw) * 5 / diff);
				temperature_norm = ((data->tempTable_up[i] - raw) * 5 % diff);
				temperature_norm = (temperature_norm * 10) / diff;
				break;
			}
		} else {
			if (raw > data->tempTable_sub[0]) {
				temperature = -20; // MIN value
				break;
			} else if (raw <= data->tempTable_sub[22]) {
				temperature = 90; // MAX value
				break;
			} else if (data->tempTable_sub[i] >= raw && raw > data->tempTable_sub[i + 1]) {
				diff = data->tempTable_sub[i] - data->tempTable_sub[i + 1];
				temperature = startTemp + (5 * i) + ((data->tempTable_sub[i] - raw) * 5 / diff);
				temperature_norm = ((data->tempTable_sub[i] - raw) * 5 % diff);
				temperature_norm = (temperature_norm * 10) / diff;
				break;
			}
		}
	}

	temperature = temperature * 10 + temperature_norm;
	pr_info("[SSP] %s type: %d raw: %d temper: %d", __func__, type, raw, temperature);
	return temperature;
}

void report_thermistor_data(struct ssp_data *data, int sensor_type,
		struct sensor_value *thermistor_data)
{
	u8 reportData[3];
	s16 temperature = thermistor_rawToTemperature(data, thermistor_data->thermistor_type, thermistor_data->thermistor_raw);
	union power_supply_propval val = {0,};

	reportData[0] = thermistor_data->thermistor_type;
	memcpy(&reportData[1], &temperature, sizeof(temperature));
	ssp_push_iio_buffer(data->indio_dev[THERMISTOR_SENSOR], thermistor_data->timestamp, (u8 *)reportData, sizeof(reportData));
	wake_lock_timeout(&data->ssp_wake_lock, 0.3*HZ);

	data->buf[THERMISTOR_SENSOR].thermistor_type = thermistor_data->thermistor_type;
	data->buf[THERMISTOR_SENSOR].thermistor_raw = thermistor_data->thermistor_raw;

	psy_do_property("battery", set, POWER_SUPPLY_EXT_PROP_OVERHEAT_NOTIFY, val);
}
void report_temp_humidity_data(struct ssp_data *data, int sensor_type,
	struct sensor_value *temp_humi_data)
{
	return;
}

#ifdef CONFIG_SENSORS_SSP_SHTC1
void report_bulk_comp_data(struct ssp_data *data)
{
	return;
}
#endif

void report_gesture_data(struct ssp_data *data, int sensor_type, struct sensor_value *gesdata)
{
	return;
}


int initialize_event_symlink(struct ssp_data *data)
{
	return SUCCESS;
}

void remove_event_symlink(struct ssp_data *data)
{
	return;
}

static const struct iio_chan_spec indio_meta_channels[] = {
	{
		.type = IIO_TIMESTAMP,
		.channel = IIO_CHANNEL,
		.scan_index = IIO_SCAN_INDEX,
		.scan_type = IIO_ST('s', 16 * 8, 16 * 8, 0)
	}
};

static const struct iio_chan_spec indio_scontext_channels[] = {
	{
		.type = IIO_TIMESTAMP,
		.channel = IIO_CHANNEL,
		.scan_index = IIO_SCAN_INDEX,
		.scan_type.sign = 's',
		.scan_type.realbits = 192,
		.scan_type.storagebits = 192,
		.scan_type.shift = 0,
		.scan_type.repeat = 3
	}
};

int initialize_input_dev(struct ssp_data *data)
{
	int iRet = 0, i = 0;

	memset(&sensors_info, -1, sizeof(struct sensor_info) * SENSOR_MAX);

	for (i = 0; i < info_table_size; i++) {
		int index = info_table[i].type;

		if (index < SENSOR_MAX) {
			int bit_size = (info_table[i].report_data_len + 8) * BITS_PER_BYTE;
			int repeat_size = bit_size / 256 + 1;

			sensors_info[index] = info_table[i];

			indio_channels[index].type = IIO_TIMESTAMP;
			indio_channels[index].channel = IIO_CHANNEL;
			indio_channels[index].scan_index = IIO_SCAN_INDEX;
			indio_channels[index].scan_type.sign = 's';
			indio_channels[index].scan_type.realbits = bit_size / repeat_size;
			indio_channels[index].scan_type.storagebits = bit_size / repeat_size;
			indio_channels[index].scan_type.shift = 0;
			indio_channels[index].scan_type.repeat = repeat_size;
		}

		// we do not check index < SENSOR_MAX because of META_SENSOR
		// scontext indio is not a sensor type
		if (index != -1 && info_table[i].report_data_len > 0) {
			struct iio_dev *indio_dev = NULL;

			indio_dev = iio_device_alloc(0);
			indio_dev->name = info_table[i].name;
			indio_dev->dev.parent = &data->spi->dev;
			indio_dev->info = &indio_info;
			if (index == META_SENSOR)
				indio_dev->channels = indio_meta_channels;
			else if (strcmp(info_table[i].name, "scontext_iio") == 0)
				indio_dev->channels = indio_scontext_channels;
			else
				indio_dev->channels = &indio_channels[index];
			indio_dev->num_channels = 1;
			indio_dev->modes = INDIO_DIRECT_MODE;
			indio_dev->currentmode = INDIO_DIRECT_MODE;

			iRet = initialize_iio_buffer_and_device(indio_dev, data);

			if (index == META_SENSOR)
				data->meta_indio_dev = indio_dev;
			else if (strcmp(info_table[i].name, "scontext_iio") == 0)
				data->indio_scontext_dev = indio_dev;
			else
				data->indio_dev[index] = indio_dev;

			pr_err("[SSP] %s(%d) - iio_device, iRet = %d\n", info_table[i].name, i, iRet);
		}
	}

	return iRet;
}

void remove_input_dev(struct ssp_data *data)
{
	return;
}
