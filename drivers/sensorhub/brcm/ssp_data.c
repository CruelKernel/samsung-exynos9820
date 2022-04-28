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
#include <linux/math64.h>
#include <linux/sched.h>

/* SSP -> AP Instruction */
#define MSG2AP_INST_BYPASS_DATA			0x37
#define MSG2AP_INST_VDIS_DATA			0x38
#define MSG2AP_INST_LIBRARY_DATA		0x01
#define MSG2AP_INST_DEBUG_DATA			0x03
#define MSG2AP_INST_BIG_DATA			0x04
#define MSG2AP_INST_META_DATA			0x05
#define MSG2AP_INST_TIME_SYNC			0x06
#define MSG2AP_INST_RESET			0x07
#define MSG2AP_INST_GYRO_CAL			0x08
#define MSG2AP_INST_MAG_CAL             0x09
#define MSG2AP_INST_SENSOR_INIT_DONE	0x0a
#define MSG2AP_INST_COLLECT_BIGDATA          0x0b
#define MSG2AP_INST_SCONTEXT_DATA		0x0c

#define CAL_DATA_FOR_BIG                             0x01

#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING // HIFI batch
#define U64_MS2NS 1000000ULL
#define U64_US2NS 1000ULL
#define U64_MS2US 1000ULL
#define MS_IDENTIFIER 1000000000U
#endif


#if defined(CONFIG_SENSORS_SSP_DAVINCI) || ANDROID_VERSION >= 100000
#define SUPER_VDIS_FORMAT	0xEEEE
#define VDIS_TIMESTAMP_FORMAT 	0xFFFF
#else
#define SUPER_VDIS_FORMAT	0xEEEEEEEE
#define VDIS_TIMESTAMP_FORMAT 	0xFFFFFFFF
#endif
#define NORMAL_TIMESTAMP_FORMAT 0x0
#define get_prev_index(a) (a - 1 + SIZE_TIMESTAMP_BUFFER) % SIZE_TIMESTAMP_BUFFER
#define get_next_index(a) (a + 1) % SIZE_TIMESTAMP_BUFFER

static u8 ts_index_cnt[SIZE_TIMESTAMP_BUFFER] = {0,};

void initialize_super_vdis_setting() {
	memset(ts_index_cnt, 0, sizeof(ts_index_cnt));
}

u64 get_delay(u64 kernel_delay)
{
	u64 ret = kernel_delay;
	u64 ms_delay = kernel_delay/1000000;

	if (ms_delay == 200)
		ret = 180 * 1000000;
	else if (ms_delay >= 60 && ms_delay < 70)
		ret = 60 * 1000000;

	return ret;
}

/*Compensate timestamp for jitter.*/
u64 get_leveling_timestamp(u64 timestamp, struct ssp_data *data, u64 time_delta, int sensor_type)
{
	u64 ret = timestamp;
	u64 base_time = get_delay(data->adDelayBuf[sensor_type]);
	u64 prev_time = data->lastTimestamp[sensor_type];
	u64 threshold = base_time + prev_time + ((base_time / 10000) * data->timestamp_factor);
	u64 level_threshold = base_time + prev_time + ((base_time / 10) * 19);

	if (data->first_sensor_data[sensor_type] == true) {
		if (time_delta == 1) {
			// check first data for base_time
			//pr_err("[SSP_DEBUG_TIME] sensor_type: %2d first_timestamp: %lld\n", sensor_type, timestamp);
			data->first_sensor_data[sensor_type] = false;
		}
		goto exit_current;
	}

	if (threshold < timestamp && level_threshold > timestamp)
		return timestamp - threshold < 30 * 1000000 ? threshold : timestamp;


exit_current:
	return ret;
}

static void get_timestamp(struct ssp_data *data, char *pchRcvDataFrame,
		int *iDataIdx, struct sensor_value *sensorsdata,
		u16 batch_mode, char msg_inst, int sensor_type)
{
	u64 time_delta_ns = 0;
	u64 update_timestamp = 0;
	u64 current_timestamp = get_current_timestamp();
	u32 ts_index = 0;
#if defined(CONFIG_SENSORS_SSP_DAVINCI) || ANDROID_VERSION >= 100000
	u16 ts_flag = 0;
#else
	u32 ts_flag = 0;
#endif
	u16 ts_cnt = 5;

	if (msg_inst == MSG2AP_INST_VDIS_DATA) {
		u64 prev_index = 0;

		memcpy(&ts_index, pchRcvDataFrame + *iDataIdx, 4);
#if defined(CONFIG_SENSORS_SSP_DAVINCI) || ANDROID_VERSION >= 100000
		memcpy(&ts_flag, pchRcvDataFrame + *iDataIdx + 4, 2);
		memcpy(&ts_cnt, pchRcvDataFrame + *iDataIdx + 6, 2);
#else
		memcpy(&ts_flag, pchRcvDataFrame + *iDataIdx + 4, 4);
#endif
		prev_index = get_prev_index(ts_index);

		if (ts_flag == SUPER_VDIS_FORMAT || ts_flag == VDIS_TIMESTAMP_FORMAT) {
			if (ts_index >= SIZE_TIMESTAMP_BUFFER) {
				pr_err("[SSP_DEBUG_TIME] index error, ts_index: %d\n", ts_index);
				goto normal_parse;
			} else if (ts_index_cnt[ts_index] == 0) {
				ts_index_cnt[prev_index] = 0;
			}
		}

		if (ts_flag == SUPER_VDIS_FORMAT) {
			u64 interval = (data->ts_index_buffer[ts_index] - data->ts_index_buffer[prev_index]) / ts_cnt; 
			u64 offset = interval * (ts_index_cnt[ts_index] + 1);

			ts_index_cnt[ts_index]++;

			time_delta_ns = data->ts_index_buffer[prev_index] + offset;

			if (time_delta_ns > current_timestamp) {
				time_delta_ns = current_timestamp;
				pr_err("[SSP_DEBUG_TIME] timestamp_error, ts_index: %d ts_cnt: %d ts_index_cnt: %d timestamp: %llu\n",
					       	ts_index, ts_cnt, ts_index_cnt[ts_index], time_delta_ns);
			}
		} else if (ts_flag == VDIS_TIMESTAMP_FORMAT) {
			if (data->ts_index_buffer[ts_index] < data->ts_index_buffer[prev_index]) {
				int i = 0;
				ssp_debug_time("[SSP_DEBUG_TIME] ts_index: %u prev_index: %llu stacked_cnt: %d", 
						ts_index, prev_index, data->ts_stacked_cnt);
				for (i = data->ts_stacked_cnt; i != ts_index; i = get_next_index(i)) {
					data->ts_index_buffer[get_next_index(i)] = data->ts_index_buffer[i] 
						+ data->adDelayBuf[GYROSCOPE_SENSOR];
				}
			}
			time_delta_ns = data->ts_index_buffer[ts_index];
		} else {
			pr_err("[SSP_TIME] ts_flag(%x), error\n", ts_flag);
			goto normal_parse;
		}
	} else {
normal_parse:
		ts_flag = 0;
		memset(&time_delta_ns, 0, 8);
		memcpy(&time_delta_ns, pchRcvDataFrame + *iDataIdx, 8);
	}
	update_timestamp = time_delta_ns;

	if (ts_flag == VDIS_TIMESTAMP_FORMAT || ts_flag == SUPER_VDIS_FORMAT) {
		ssp_debug_time("[SSP_DEBUG_TIME] ts_index: %u ts_index_cnt: %d stacked_cnt: %u ts_flag: 0x%x ts_cnt: %d current_ts: %lld update_ts: %llu latency: %lld",
			       	ts_index, ts_index_cnt[ts_index], data->ts_stacked_cnt, ts_flag, ts_cnt, current_timestamp, update_timestamp, current_timestamp - time_delta_ns);
	} else { 
		ssp_debug_time("[SSP_DEBUG_TIME] sensor_type: %2d update_ts: %lld current_ts: %lld diff: %lld latency: %lld\n",
			sensor_type, update_timestamp, current_timestamp,
			update_timestamp - data->lastTimestamp[sensor_type], current_timestamp - update_timestamp);
	}

	data->lastTimestamp[sensor_type] = time_delta_ns;

	sensorsdata->timestamp = data->lastTimestamp[sensor_type];
	*iDataIdx += 8;
}

static void get_3axis_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 6);
	*iDataIdx += 6;
}

static void get_gyro_sensordata(char *pchRcvDataFrame, int *iDataIdx,
struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 12);
	*iDataIdx += 12;
}

static void get_uncal_gyro_sensordata(char *pchRcvDataFrame, int *iDataIdx,
struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 24);
	*iDataIdx += 24;
}

static void get_geomagnetic_uncaldata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, UNCAL_MAGNETIC_SIZE);
	*iDataIdx += UNCAL_MAGNETIC_SIZE;
}

static void get_geomagnetic_caldata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, MAGNETIC_SIZE);
	*iDataIdx += MAGNETIC_SIZE;
}
static void get_geomagnetic_rawdata(char *pchRcvDataFrame, int *iDataIdx,
		struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 6);
	*iDataIdx += 6;
} // this func gather sensor data, but not report to HAL

static void get_rot_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 17);
	*iDataIdx += 17;
}

static void get_step_det_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
}

static void get_light_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
#ifdef CONFIG_SENSORS_SSP_LIGHT_REPORT_LUX
#ifdef CONFIG_SENSORS_SSP_LIGHT_MAX_GAIN_2BYTE
#ifdef CONFIG_SENSORS_SSP_LIGHT_MAX_ATIME_2BYTE
#ifdef CONFIG_SENSORS_SSP_LIGHT_ADDING_LUMINANCE
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 21);
	*iDataIdx += 21;
#else
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 20);
	*iDataIdx += 20;
#endif
#else
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 19);
	*iDataIdx += 19;
#endif
#else
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 18);
	*iDataIdx += 18;
#endif
#else
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 10);
	*iDataIdx += 10;
#endif
}

#ifdef CONFIG_SENSORS_SSP_IRDATA_FOR_CAMERA
static void get_light_ir_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
#ifdef CONFIG_SENSORS_SSP_LIGHT_MAX_GAIN_2BYTE
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 13);
	*iDataIdx += 13;
#else
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 12);
	*iDataIdx += 12;
#endif
}
#endif

static void get_light_flicker_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(&sensorsdata->light_flicker, pchRcvDataFrame + *iDataIdx, 2);
	*iDataIdx += 2;
}
static void get_light_cct_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
#ifdef CONFIG_SENSORS_SSP_LIGHT_REPORT_LUX
#ifdef CONFIG_SENSORS_SSP_LIGHT_MAX_GAIN_2BYTE
#ifdef CONFIG_SENSORS_SSP_LIGHT_LUX_RAW
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 25);
	*iDataIdx += 25;
#else
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 19);
	*iDataIdx += 19;
#endif

#else
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 18);
	*iDataIdx += 18;
#endif
#else
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 10);
	*iDataIdx += 10;
#endif
}
static void get_pressure_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 6);
	*iDataIdx += 6;
}

static void get_gesture_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 20);
	*iDataIdx += 20;
}

static void get_proximity_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
#ifdef CONFIG_SENSORS_SSP_PROX_LIGHT_DIFF
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 11);
	*iDataIdx += 11;
#else
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 7);
	*iDataIdx += 7;
#endif
}

static void get_proximity_alert_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 3);
	*iDataIdx += 3;
}

static void get_proximity_pocket_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
#if defined(CONFIG_SENSORS_SSP_TMG399x)
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 2);
	*iDataIdx += 2;
#else	//CONFIG_SENSORS_SSP_TMD4903, CONFIG_SENSORS_SSP_TMD3782, CONFIG_SENSORS_SSP_TMD4904
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 3);
	*iDataIdx += 3;
#endif
}

static void get_proximity_rawdata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
#if defined(CONFIG_SENSORS_SSP_TMG399x)
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
#else	//CONFIG_SENSORS_SSP_TMD4903, CONFIG_SENSORS_SSP_TMD3782, CONFIG_SENSORS_SSP_TMD4904
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 2);
	*iDataIdx += 2;
#endif
}

#ifdef CONFIG_SENSORS_SSP_PROX_ADC_CAL
static void get_proximity_adc_calib_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 6);
	*iDataIdx += 6;
}
#endif

#ifdef CONFIG_SENSORS_SSP_SX9306
static void get_grip_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 9);
		*iDataIdx += 9;
}
#endif

static void get_temp_humidity_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memset(&sensorsdata->data[2], 0, 2);
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 5);
	*iDataIdx += 5;
}

static void get_sig_motion_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
}

static void get_step_cnt_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(&sensorsdata->step_diff, pchRcvDataFrame + *iDataIdx, 4);
	*iDataIdx += 4;
}

static void get_shake_cam_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
}

static void get_tilt_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
}

static void get_pickup_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
}
static void get_wakeup_motion_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(&sensorsdata->wakeup_move_event[0], pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
}
static void get_call_gesture_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
}
static void get_move_detector_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(&sensorsdata->wakeup_move_event[1], pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
}

static void get_thermistor_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 3);
	*iDataIdx += 3;
}
static void get_ucal_accel_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 12);
	*iDataIdx += 12;
}
static void get_pocket_mode_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 26);
	*iDataIdx += 26;
}
static void get_led_cover_event_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
}
static void get_auto_rotation_data(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
}
static void get_sar_backoff_motion_data(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
}
#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING // HIFI batch

/*
 *static void get_sensor_data(char *pchRcvDataFrame, int *iDataIdx,
 *        struct sensor_value *sensorsdata, int data_size)
 *{
 *        memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, data_size);
 *        *iDataIdx += data_size;
 *}
 */


bool ssp_check_buffer(struct ssp_data *data)
{
	int idx_data = 0;
	u8 sensor_type = 0;
	bool res = true;
	u64 ts = get_current_timestamp();

	pr_err("[SSP_BUF] start check %lld\n", ts);
	do {
		sensor_type = data->batch_event.batch_data[idx_data++];

		if ((sensor_type != ACCELEROMETER_SENSOR) &&
			(sensor_type != GEOMAGNETIC_UNCALIB_SENSOR) &&
			(sensor_type != PRESSURE_SENSOR) &&
			(sensor_type != GAME_ROTATION_VECTOR) &&
			(sensor_type != PROXIMITY_SENSOR) &&
			(sensor_type != META_SENSOR) &&
			(sensor_type != ACCEL_UNCALIB_SENSOR)) {
			pr_err("[SSP]: %s - Mcu data frame1 error %d, idx_data %d\n", __func__,
					sensor_type, idx_data - 1);
			res = false;
			break;
		}

		switch (sensor_type) {
		case ACCELEROMETER_SENSOR:
			idx_data += 14;
			break;
		case GEOMAGNETIC_UNCALIB_SENSOR:
#ifdef CONFIG_SSP_SUPPORT_MAGNETIC_OVERFLOW
		idx_data += 21;
#else
		idx_data += 20;
#endif
                        break;
                case ACCEL_UNCALIB_SENSOR:
                        idx_data += 20;
			break;
		case PRESSURE_SENSOR:
			idx_data += 14;
			break;
		case GAME_ROTATION_VECTOR:
			idx_data += 25;
			break;
		case PROXIMITY_SENSOR:
			idx_data += 11;
			break;
		case META_SENSOR:
			idx_data += 1;
			break;
		}

		if (idx_data > data->batch_event.batch_length) {
			//stop index over max length
			pr_info("[SSP_CHK] invalid data1\n");
			res = false;
			break;
		}

		// run until max length
		if (idx_data == data->batch_event.batch_length) {
			//pr_info("[SSP_CHK] valid data\n");
			break;
		} else if (idx_data + 1 == data->batch_event.batch_length) {
			//stop if only sensor type exist
			pr_info("[SSP_CHK] invalid data2\n");
			res = false;
			break;
		}
	} while (true);
	ts = get_current_timestamp();
	pr_err("[SSP_BUF] finish check %lld\n", ts);

	return res;
}

void ssp_batch_resume_check(struct ssp_data *data)
{
	u64 acc_offset = 0, uncal_mag_offset = 0, press_offset = 0, grv_offset = 0, proxi_offset = 0;
	//if suspend -> wakeup case. calc. FIFO last timestamp
	if (data->bIsResumed) {
		u8 sensor_type = 0;
		struct sensor_value sensor_data;
		u64 delta_time_us = 0;
		int idx_data = 0;
		u64 timestamp = get_current_timestamp();
		//ssp_dbg("[SSP_BAT] LENGTH = %d, start index = %d ts %lld resume %lld\n",
		//data->batch_event.batch_length, idx_data, timestamp, data->resumeTimestamp);

		timestamp = data->resumeTimestamp = data->timestamp;

		while (idx_data < data->batch_event.batch_length) {
			sensor_type = data->batch_event.batch_data[idx_data++];
			if (sensor_type == META_SENSOR)	{
				sensor_data.meta_data.sensor = data->batch_event.batch_data[idx_data++];
				continue;
			}

			if ((sensor_type != ACCELEROMETER_SENSOR) &&
				(sensor_type != GEOMAGNETIC_UNCALIB_SENSOR) &&
				(sensor_type != PRESSURE_SENSOR) &&
				(sensor_type != GAME_ROTATION_VECTOR) &&
				(sensor_type != PROXIMITY_SENSOR) &&
				(sensor_type != ACCEL_UNCALIB_SENSOR)) {
				pr_err("[SSP]: %s - Mcu data frame1 error %d, idx_data %d\n", __func__,
						sensor_type, idx_data - 1);
				data->bIsResumed = false;
				data->resumeTimestamp = 0ULL;
				return;
			}

			data->get_sensor_data[sensor_type](data->batch_event.batch_data, &idx_data, &sensor_data);

			memset(&delta_time_us, 0, 8);
			memcpy(&delta_time_us, data->batch_event.batch_data + idx_data, 8);

			switch (sensor_type) {
			case ACCELEROMETER_SENSOR:
			case GEOMAGNETIC_UNCALIB_SENSOR:
			case GAME_ROTATION_VECTOR:
			case PRESSURE_SENSOR:
			case PROXIMITY_SENSOR:
                        case ACCEL_UNCALIB_SENSOR:
				data->lastTimestamp[sensor_type] = delta_time_us;
				break;
			}
			idx_data += 8;
		}

		ssp_dbg("[SSP_BAT] resume calc. acc %lld. uncalmag %lld. pressure %lld. GRV %lld proxi %lld\n",
			acc_offset, uncal_mag_offset, press_offset, grv_offset, proxi_offset);
	}
	data->bIsResumed = false;
	data->resumeTimestamp = 0ULL;
}

void ssp_batch_report(struct ssp_data *data)
{
	u8 sensor_type = 0;
	struct sensor_value sensor_data;
	int idx_data = 0;
	int count = 0;
	u64 timestamp = get_current_timestamp();

	ssp_dbg("[SSP_BAT] LENGTH = %d, start index = %d ts %lld\n",
			data->batch_event.batch_length, idx_data, timestamp);

	while (idx_data < data->batch_event.batch_length) {
		//ssp_dbg("[SSP_BAT] bcnt %d\n", count);
		sensor_type = data->batch_event.batch_data[idx_data++];

		if (sensor_type == META_SENSOR) {
			sensor_data.meta_data.sensor = data->batch_event.batch_data[idx_data++];
			report_meta_data(data, sensor_type, &sensor_data);
			count++;
			continue;
		}

		if ((sensor_type != ACCELEROMETER_SENSOR) &&
			(sensor_type != GEOMAGNETIC_UNCALIB_SENSOR) &&
			(sensor_type != PRESSURE_SENSOR) &&
			(sensor_type != GAME_ROTATION_VECTOR) &&
			(sensor_type != PROXIMITY_SENSOR) &&
			(sensor_type != ACCEL_UNCALIB_SENSOR)) {
			pr_err("[SSP]: %s - Mcu data frame1 error %d, idx_data %d\n", __func__,
					sensor_type, idx_data - 1);
			return;
		}

		usleep_range(150, 151);
		//ssp_dbg("[SSP_BAT] cnt %d\n", count);
		data->get_sensor_data[sensor_type](data->batch_event.batch_data, &idx_data, &sensor_data);

		get_timestamp(data, data->batch_event.batch_data, &idx_data, &sensor_data, BATCH_MODE_RUN, MSG2AP_INST_BYPASS_DATA, sensor_type);
		ssp_debug_time("[SSP_BAT]: sensor %d, AP %lld MCU %lld, diff %lld, count: %d\n",
			sensor_type, timestamp, sensor_data.timestamp, timestamp - sensor_data.timestamp, count);

		data->report_sensor_data[sensor_type](data, sensor_type, &sensor_data);
		data->reportedData[sensor_type] = true;
		count++;
	}
	ssp_dbg("[SSP_BAT] max cnt %d\n", count);
}


// Control batched data with long term
// Ref ssp_read_big_library_task
void ssp_batch_data_read_task(struct work_struct *work)
{
	struct ssp_big *big = container_of(work, struct ssp_big, work);
	struct ssp_data *data = big->data;
	struct ssp_msg *msg;
	int buf_len, residue, ret = 0, index = 0, pos = 0;
	u64 ts = 0;

	mutex_lock(&data->batch_events_lock);
	wake_lock(&data->ssp_batch_wake_lock);

	residue = big->length;
	data->batch_event.batch_length = big->length;
	data->batch_event.batch_data = vmalloc(big->length);
	if (data->batch_event.batch_data == NULL) {
		ssp_dbg("[SSP_BAT] batch data alloc fail\n");
		kfree(big);
		wake_unlock(&data->ssp_batch_wake_lock);
		mutex_unlock(&data->batch_events_lock);
		return;
	}

	//ssp_dbg("[SSP_BAT] IN : LENGTH = %d\n", big->length);

	while (residue > 0) {
		buf_len = residue > DATA_PACKET_SIZE
			? DATA_PACKET_SIZE : residue;

		msg = kzalloc(sizeof(*msg), GFP_ATOMIC);
		msg->cmd = MSG2SSP_AP_GET_BIG_DATA;
		msg->length = buf_len;
		msg->options = AP2HUB_READ | (index++ << SSP_INDEX);
		msg->data = big->addr;
		msg->buffer = data->batch_event.batch_data + pos;
		msg->free_buffer = 0;

		ret = ssp_spi_sync(big->data, msg, 1000);
		if (ret != SUCCESS) {
			pr_err("[SSP_BAT] read batch data err(%d) ignor\n", ret);
			vfree(data->batch_event.batch_data);
			data->batch_event.batch_data = NULL;
			data->batch_event.batch_length = 0;
			kfree(big);
			wake_unlock(&data->ssp_batch_wake_lock);
			mutex_unlock(&data->batch_events_lock);
			return;
		}

		pos += buf_len;
		residue -= buf_len;
		pr_info("[SSP_BAT] read batch data (%5d / %5d)\n", pos, big->length);
	}

	// TODO: Do not parse, jut put in to FIFO, and wake_up thread.

	// READ DATA FROM MCU COMPLETED
	//Wake up check
	if (ssp_check_buffer(data)) {
		ssp_batch_resume_check(data);

		// PARSE DATA FRAMES, Should run loop
		ts = get_current_timestamp();
		pr_info("[SSP] report start %lld\n", ts);
		ssp_batch_report(data);
		ts = get_current_timestamp();
		pr_info("[SSP] report finish %lld\n", ts);
	}

	vfree(data->batch_event.batch_data);
	data->batch_event.batch_data = NULL;
	data->batch_event.batch_length = 0;
	kfree(big);
	wake_unlock(&data->ssp_batch_wake_lock);
	mutex_unlock(&data->batch_events_lock);
}
#endif
int handle_big_data(struct ssp_data *data, char *pchRcvDataFrame, int *pDataIdx)
{
	u8 bigType = 0;
	struct ssp_big *big = kzalloc(sizeof(*big), GFP_KERNEL);

	big->data = data;
	bigType = pchRcvDataFrame[(*pDataIdx)++];
	memcpy(&big->length, pchRcvDataFrame + *pDataIdx, 4);
	*pDataIdx += 4;
	memcpy(&big->addr, pchRcvDataFrame + *pDataIdx, 4);
	*pDataIdx += 4;

	if (bigType >= BIG_TYPE_MAX) {
		kfree(big);
		return FAIL;
	}

	INIT_WORK(&big->work, data->ssp_big_task[bigType]);
#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING // HIFI batch
	if (bigType != BIG_TYPE_READ_HIFI_BATCH)
		queue_work(data->debug_wq, &big->work);
	else
		queue_work(data->batch_wq, &big->work);
#else
	queue_work(data->debug_wq, &big->work);
#endif
	return SUCCESS;
}

void handle_timestamp_sync(struct ssp_data *data, char *pchRcvDataFrame, int *index)
{
	u64 mcu_timestamp;
	u64 current_timestamp = get_current_timestamp();

	memcpy(&mcu_timestamp, pchRcvDataFrame + *index, sizeof(mcu_timestamp));
	data->timestamp_offset = current_timestamp - mcu_timestamp;
	schedule_delayed_work(&data->work_ssp_tiemstamp_sync, msecs_to_jiffies(100));

	*index += 8;
}


/*************************************************************************/
/* SSP parsing the dataframe                                             */
/*************************************************************************/
int parse_dataframe(struct ssp_data *data, char *pchRcvDataFrame, int iLength)
{
	int iDataIdx;
	int sensor_type;
	//u16 length = 0;

	struct sensor_value sensorsdata;
	s32 caldata[3] = { 0, };
	char msg_inst = 0;
	u64 timestampforReset;

	data->uIrqCnt++;

	for (iDataIdx = 0; iDataIdx < iLength;) {
		switch (msg_inst = pchRcvDataFrame[iDataIdx++]) {
		case MSG2AP_INST_TIMESTAMP_OFFSET:
			handle_timestamp_sync(data, pchRcvDataFrame, &iDataIdx);
			break;

		case MSG2AP_INST_SENSOR_INIT_DONE:
			pr_err("[SSP]: MCU sensor init done\n");
			complete(&data->hub_data->mcu_init_done);
			break;
// HIFI batch
		case MSG2AP_INST_VDIS_DATA:			
		case MSG2AP_INST_BYPASS_DATA:
			sensor_type = pchRcvDataFrame[iDataIdx++];
			if ((sensor_type < 0) || (sensor_type >= SENSOR_MAX)) {
				pr_err("[SSP]: %s - Mcu data frame1 error %d\n", __func__,
						sensor_type);
				return ERROR;
			}

			timestampforReset = get_current_timestamp();
			data->LastSensorTimeforReset[sensor_type] = timestampforReset;

			if (data->get_sensor_data[sensor_type] == NULL)
				goto error_return;

			data->get_sensor_data[sensor_type](pchRcvDataFrame, &iDataIdx, &sensorsdata);
			data->skipEventReport = false;

			get_timestamp(data, pchRcvDataFrame, &iDataIdx, &sensorsdata, 0, msg_inst, sensor_type);
			if (data->skipEventReport == false) {
				//Check sensor is enabled, before report sensordata
				u64 AddedSensorState = (u64)atomic64_read(&data->aSensorEnable);

				if (AddedSensorState & (1ULL << sensor_type)
				|| sensor_type == GEOMAGNETIC_RAW
				|| sensor_type == PROXIMITY_RAW
				|| sensor_type == PROXIMITY_ADC_CALIB)
					data->report_sensor_data[sensor_type](data, sensor_type, &sensorsdata);
				else {
					if (data->skipSensorData == 30) {
						pr_info("[SSP]: sensor not added,but sensor(%d) came", sensor_type);
						data->skipSensorData = 0;
					}
					data->skipSensorData++;
				}
			}

			data->reportedData[sensor_type] = true;
			//pr_err("[SSP]: (%d / %d)\n", iDataIdx, iLength);
			break;
		case MSG2AP_INST_DEBUG_DATA:
			sensor_type = print_mcu_debug(pchRcvDataFrame, &iDataIdx, iLength);
			if (sensor_type) {
				pr_err("[SSP]: %s - Mcu data frame3 error %d\n", __func__,
						sensor_type);
				return ERROR;
			}
			break;
		/*
		case MSG2AP_INST_LIBRARY_DATA:
			memcpy(&length, pchRcvDataFrame + iDataIdx, 2);
			iDataIdx += 2;
			ssp_sensorhub_handle_data(data, pchRcvDataFrame, iDataIdx,
					iDataIdx + length);
			iDataIdx += length;
			break;
		*/
		case MSG2AP_INST_BIG_DATA:
			handle_big_data(data, pchRcvDataFrame, &iDataIdx);
			break;
		case MSG2AP_INST_META_DATA:
			sensorsdata.meta_data.what = pchRcvDataFrame[iDataIdx++];
			sensorsdata.meta_data.sensor = pchRcvDataFrame[iDataIdx++];

			if (sensorsdata.meta_data.what != 1)
				goto error_return;

			if ((sensorsdata.meta_data.sensor < 0) || (sensorsdata.meta_data.sensor >= SENSOR_MAX)) {
				pr_err("[SSP]: %s - Mcu meta_data frame1 error %d\n", __func__,
						sensorsdata.meta_data.sensor);
				return ERROR;
			}
			report_meta_data(data, sensor_type, &sensorsdata);
			break;
		case MSG2AP_INST_TIME_SYNC:
			data->bTimeSyncing = true;
			break;
		case MSG2AP_INST_GYRO_CAL:
			pr_info("[SSP]: %s - Gyro caldata received from MCU\n",  __func__);
			memcpy(caldata, pchRcvDataFrame + iDataIdx, sizeof(caldata));
			if(data->IsAPsuspend) {
            			iDataIdx += sizeof(caldata);
			    break;
		        }
			wake_lock(&data->ssp_wake_lock);
			save_gyro_caldata(data, caldata);
			wake_unlock(&data->ssp_wake_lock);
			iDataIdx += sizeof(caldata);
			break;
		case MSG2AP_INST_MAG_CAL:
			wake_lock(&data->ssp_wake_lock);
			save_magnetic_cal_param_to_nvm(data, pchRcvDataFrame, &iDataIdx);
			wake_unlock(&data->ssp_wake_lock);
			break;
		case MSG2AP_INST_COLLECT_BIGDATA:
			switch (pchRcvDataFrame[iDataIdx++]) {
			case CAL_DATA_FOR_BIG:
				sensor_type = pchRcvDataFrame[iDataIdx++];
				if (sensor_type == ACCELEROMETER_SENSOR)
					set_AccelCalibrationInfoData(pchRcvDataFrame, &iDataIdx);
				else if (sensor_type == GYROSCOPE_SENSOR)
					set_GyroCalibrationInfoData(pchRcvDataFrame, &iDataIdx);
				break;
			default:
				pr_info("[SSP] wrong sub inst for big data\n");
				break;
			}
			break;
		case MSG2AP_INST_SCONTEXT_DATA:
			memcpy(sensorsdata.scontext_buf, pchRcvDataFrame + iDataIdx, SCONTEXT_DATA_SIZE);
			report_scontext_data(data, sensor_type, &sensorsdata);
			iDataIdx += SCONTEXT_DATA_SIZE;
				break;
		default:
			goto error_return;
		}
	}
	if (data->pktErrCnt >= 1) // if input error packet doesn't comes continually, do not reset
		data->pktErrCnt = 0;
	return SUCCESS;
error_return:
	pr_err("[SSP] %s err Inst 0x%02x\n", __func__, msg_inst);
	data->pktErrCnt++;
	if (data->pktErrCnt >= 2) {
		pr_err("[SSP] %s packet is polluted\n", __func__);
		data->mcuAbnormal = true;
		data->pktErrCnt = 0;
		data->errorCount++;
	}
	return ERROR;
}

void initialize_function_pointer(struct ssp_data *data)
{
	data->get_sensor_data[ACCELEROMETER_SENSOR] = get_3axis_sensordata;
	data->get_sensor_data[GYROSCOPE_SENSOR] = get_gyro_sensordata;
	data->get_sensor_data[GEOMAGNETIC_UNCALIB_SENSOR] =
		get_geomagnetic_uncaldata;
	data->get_sensor_data[GEOMAGNETIC_RAW] = get_geomagnetic_rawdata;
	data->get_sensor_data[GEOMAGNETIC_SENSOR] =
		get_geomagnetic_caldata;
	data->get_sensor_data[PRESSURE_SENSOR] = get_pressure_sensordata;
	data->get_sensor_data[GESTURE_SENSOR] = get_gesture_sensordata;
	data->get_sensor_data[PROXIMITY_SENSOR] = get_proximity_sensordata;
	data->get_sensor_data[PROXIMITY_ALERT_SENSOR] = get_proximity_alert_sensordata;
	data->get_sensor_data[LIGHT_FLICKER_SENSOR] = get_light_flicker_sensordata;

	data->get_sensor_data[PROXIMITY_RAW] = get_proximity_rawdata;
	data->get_sensor_data[PROXIMITY_POCKET] = get_proximity_pocket_sensordata;
#ifdef CONFIG_SENSORS_SSP_PROX_ADC_CAL
	data->get_sensor_data[PROXIMITY_ADC_CALIB] = get_proximity_adc_calib_sensordata;
#endif
#ifdef CONFIG_SENSORS_SSP_SX9306
	data->get_sensor_data[GRIP_SENSOR] = get_grip_sensordata;
#endif
	data->get_sensor_data[LIGHT_SENSOR] = get_light_sensordata;
	data->get_sensor_data[UNCAL_LIGHT_SENSOR] = get_light_sensordata;
#ifdef CONFIG_SENSORS_SSP_IRDATA_FOR_CAMERA
	data->get_sensor_data[LIGHT_IR_SENSOR] = get_light_ir_sensordata;
#endif
	data->get_sensor_data[LIGHT_CCT_SENSOR] = get_light_cct_sensordata;

	data->get_sensor_data[TEMPERATURE_HUMIDITY_SENSOR] =
		get_temp_humidity_sensordata;
	data->get_sensor_data[ROTATION_VECTOR] = get_rot_sensordata;
	data->get_sensor_data[GAME_ROTATION_VECTOR] = get_rot_sensordata;
	data->get_sensor_data[STEP_DETECTOR] = get_step_det_sensordata;
	data->get_sensor_data[SIG_MOTION_SENSOR] = get_sig_motion_sensordata;
	data->get_sensor_data[GYRO_UNCALIB_SENSOR] = get_uncal_gyro_sensordata;
	data->get_sensor_data[STEP_COUNTER] = get_step_cnt_sensordata;
	data->get_sensor_data[SHAKE_CAM] = get_shake_cam_sensordata;
#ifdef CONFIG_SENSORS_SSP_INTERRUPT_GYRO_SENSOR
	data->get_sensor_data[INTERRUPT_GYRO_SENSOR] = get_gyro_sensordata;
#endif
	data->get_sensor_data[TILT_DETECTOR] = get_tilt_sensordata;
	data->get_sensor_data[PICKUP_GESTURE] = get_pickup_sensordata;
	data->get_sensor_data[THERMISTOR_SENSOR] = get_thermistor_sensordata;
	data->get_sensor_data[ACCEL_UNCALIB_SENSOR] = get_ucal_accel_sensordata;
	data->get_sensor_data[WAKE_UP_MOTION] = get_wakeup_motion_sensordata;
	data->get_sensor_data[CALL_GESTURE] = get_call_gesture_sensordata;
    data->get_sensor_data[MOVE_DETECTOR] = get_move_detector_sensordata;
	data->get_sensor_data[POCKET_MODE_SENSOR] = get_pocket_mode_sensordata;
	data->get_sensor_data[LED_COVER_EVENT_SENSOR] = get_led_cover_event_sensordata;
	data->get_sensor_data[AUTO_ROTATION_SENSOR] = get_auto_rotation_data;
	data->get_sensor_data[SAR_BACKOFF_MOTION] = get_sar_backoff_motion_data;
	data->get_sensor_data[BULK_SENSOR] = NULL;
	data->get_sensor_data[GPS_SENSOR] = NULL;

	data->report_sensor_data[ACCELEROMETER_SENSOR] = report_acc_data;
	data->report_sensor_data[GYROSCOPE_SENSOR] = report_gyro_data;
	data->report_sensor_data[GEOMAGNETIC_UNCALIB_SENSOR] =
		report_mag_uncaldata;
	data->report_sensor_data[GEOMAGNETIC_RAW] = report_geomagnetic_raw_data;
	data->report_sensor_data[GEOMAGNETIC_SENSOR] =
		report_mag_data;
	data->report_sensor_data[PRESSURE_SENSOR] = report_pressure_data;
	data->report_sensor_data[GESTURE_SENSOR] = report_gesture_data;
	data->report_sensor_data[PROXIMITY_SENSOR] = report_prox_data;
	data->report_sensor_data[PROXIMITY_ALERT_SENSOR] = report_prox_alert_data;
	data->report_sensor_data[PROXIMITY_POCKET] = report_proximity_pocket_data;
	data->report_sensor_data[LIGHT_FLICKER_SENSOR] = report_light_flicker_data;

	data->report_sensor_data[PROXIMITY_RAW] = report_prox_raw_data;
#ifdef CONFIG_SENSORS_SSP_PROX_ADC_CAL 
	data->report_sensor_data[PROXIMITY_ADC_CALIB] = report_prox_adc_calib_data;
#endif
#ifdef CONFIG_SENSORS_SSP_SX9306
	data->report_sensor_data[GRIP_SENSOR] = report_grip_data;
#endif
	data->report_sensor_data[LIGHT_SENSOR] = report_light_data;
	data->report_sensor_data[UNCAL_LIGHT_SENSOR] = report_light_data;
#ifdef CONFIG_SENSORS_SSP_IRDATA_FOR_CAMERA
	data->report_sensor_data[LIGHT_IR_SENSOR] = report_light_ir_data;
#endif
	data->report_sensor_data[TEMPERATURE_HUMIDITY_SENSOR] =
		report_temp_humidity_data;
	data->report_sensor_data[ROTATION_VECTOR] = report_rot_data;
	data->report_sensor_data[GAME_ROTATION_VECTOR] = report_game_rot_data;
	data->report_sensor_data[STEP_DETECTOR] = report_step_det_data;
	data->report_sensor_data[SIG_MOTION_SENSOR] = report_sig_motion_data;
	data->report_sensor_data[GYRO_UNCALIB_SENSOR] = report_uncalib_gyro_data;
	data->report_sensor_data[STEP_COUNTER] = report_step_cnt_data;
	data->report_sensor_data[SHAKE_CAM] = report_shake_cam_data;
#ifdef CONFIG_SENSORS_SSP_INTERRUPT_GYRO_SENSOR
	data->report_sensor_data[INTERRUPT_GYRO_SENSOR] = report_interrupt_gyro_data;
#endif
	data->report_sensor_data[TILT_DETECTOR] = report_tilt_data;
	data->report_sensor_data[PICKUP_GESTURE] = report_pickup_data;

	data->report_sensor_data[BULK_SENSOR] = NULL;
	data->report_sensor_data[GPS_SENSOR] = NULL;
	data->report_sensor_data[LIGHT_CCT_SENSOR] = report_light_cct_data;
	data->report_sensor_data[THERMISTOR_SENSOR] = report_thermistor_data;
	data->report_sensor_data[ACCEL_UNCALIB_SENSOR] = report_uncalib_accel_data;
	data->report_sensor_data[WAKE_UP_MOTION] = report_wakeup_motion_data;
	data->report_sensor_data[CALL_GESTURE] = report_call_gesture_data;
    data->report_sensor_data[MOVE_DETECTOR] = report_move_detector_data;
	data->report_sensor_data[POCKET_MODE_SENSOR] = report_pocket_mode_data;
	data->report_sensor_data[LED_COVER_EVENT_SENSOR] = report_led_cover_event_data;
	data->report_sensor_data[AUTO_ROTATION_SENSOR] = report_auto_rotation_data;
	data->report_sensor_data[SAR_BACKOFF_MOTION] = report_sar_backoff_motion_data;
	data->ssp_big_task[BIG_TYPE_DUMP] = ssp_dump_task;
	data->ssp_big_task[BIG_TYPE_READ_LIB] = ssp_read_big_library_task;
#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING // HIFI batch
	data->ssp_big_task[BIG_TYPE_READ_HIFI_BATCH] = ssp_batch_data_read_task;
#endif
	data->ssp_big_task[BIG_TYPE_VOICE_NET] = ssp_send_big_library_task;
	data->ssp_big_task[BIG_TYPE_VOICE_GRAM] = ssp_send_big_library_task;
	data->ssp_big_task[BIG_TYPE_VOICE_PCM] = ssp_pcm_dump_task;
	data->ssp_big_task[BIG_TYPE_TEMP] = ssp_temp_task;
}
