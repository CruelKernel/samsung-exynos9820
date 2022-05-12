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
#include <linux/fs.h>
#include <linux/sec_debug.h>
#include <linux/iio/iio.h>
#include <linux/iio/buffer.h>
#include <linux/iio/buffer_impl.h>

#define SSP_DEBUG_TIMER_SEC		(5 * HZ)

#define LIMIT_RESET_CNT			40
#define LIMIT_TIMEOUT_CNT		3

#define DUMP_FILE_PATH "/data/log/MCU_DUMP"

void ssp_dump_task(struct work_struct *work)
{
#ifdef CONFIG_SENSORS_SSP_BBD
	pr_err("[SSPBBD]:TODO:%s()\n", __func__);
#else

	struct ssp_big *big;
	struct file *dump_file;
	struct ssp_msg *msg;
	char *buffer;
	char strFilePath[60];
	struct timeval cur_time;
	int iTimeTemp;
	mm_segment_t fs;
	int buf_len, packet_len, residue;
	int index = 0, iRetTrans = 0, iRetWrite = 0;

	big = container_of(work, struct ssp_big, work);
	pr_err("[SSP]: %s - start ssp dumping (%d)(%d)\n",
		__func__, big->data->bMcuDumpMode, big->data->uDumpCnt);
	big->data->uDumpCnt++;
	wake_lock(&big->data->ssp_wake_lock);

	fs = get_fs();
	set_fs(get_ds());

	if (big->data->bMcuDumpMode == true) {
		do_gettimeofday(&cur_time);
		iTimeTemp = (int) cur_time.tv_sec;

		sprintf(strFilePath, "%s%d.txt", DUMP_FILE_PATH, iTimeTemp);

		dump_file = filp_open(strFilePath,
				O_RDWR|O_CREAT|O_APPEND, 0660);
		if (IS_ERR(dump_file)) {
			pr_err("[SSP]: %s - Can't open dump file\n", __func__);
			set_fs(fs);
			iRet = PTR_ERR(dump_file);
			wake_unlock(&big->data->ssp_wake_lock);
			kfree(big);
			return;
		}
	} else
		dump_file = NULL;

	buf_len = (big->length > DATA_PACKET_SIZE) ?
			DATA_PACKET_SIZE : big->length;
	buffer = kzalloc(buf_len, GFP_KERNEL);
	residue = big->length;

	while (residue > 0) {
		packet_len = residue > DATA_PACKET_SIZE ?
				DATA_PACKET_SIZE : residue;

		msg = kzalloc(sizeof(*msg), GFP_KERNEL);
		msg->cmd = MSG2SSP_AP_GET_BIG_DATA;
		msg->length = packet_len;
		msg->options = AP2HUB_READ | (index++ << SSP_INDEX);
		msg->data = big->addr;
		msg->buffer = buffer;
		msg->free_buffer = 0;

		iRetTrans = ssp_spi_sync(big->data, msg, 1000);
		if (iRetTrans != SUCCESS) {
			pr_err("[SSP]: %s - Fail to receive data %d (%d)\n",
				__func__, iRetTrans, residue);
			break;
		}
		if (big->data->bMcuDumpMode == true) {
			iRetWrite = vfs_write(dump_file,
					(char __user *) buffer,
					packet_len,
					&dump_file->f_pos);
			if (iRetWrite < 0) {
				pr_err("[SSP] %s Can't write dump to file\n",
					__func__);
				break;
			}
		}
		residue -= packet_len;
	}

	if (big->data->bMcuDumpMode == true &&
		(iRetTrans != SUCCESS || iRetWrite < 0)) {

		char FAILSTRING[100];

		sprintf(FAILSTRING, "FAIL OCCURRED(%d)(%d)(%d)",
			iRetTrans,
			iRetWrite,
			big->length);
		vfs_write(dump_file, (char __user *) FAILSTRING,
			strlen(FAILSTRING),
			&dump_file->f_pos);
	}

	big->data->bDumping = false;
	if (big->data->bMcuDumpMode == true)
		filp_close(dump_file, current->files);

	set_fs(fs);

	wake_unlock(&big->data->ssp_wake_lock);
	kfree(buffer);
	kfree(big);

	pr_err("[SSP]: %s done\n", __func__);
#endif
}

void ssp_temp_task(struct work_struct *work)
{
#ifdef CONFIG_SENSORS_SSP_BBD
	pr_err("[SSPBBD]:TODO:%s()\n", __func__);
#else
	struct ssp_big *big;
	struct ssp_msg *msg;
	char *buffer;
	int buf_len, packet_len, residue;
	int iRet = 0, index = 0, i = 0, buffindex = 0;

	big = container_of(work, struct ssp_big, work);
	buf_len = big->length > DATA_PACKET_SIZE ?
		DATA_PACKET_SIZE : big->length;
	buffer = kzalloc(buf_len, GFP_KERNEL);
	residue = big->length;
#ifdef CONFIG_SENSORS_SSP_SHTC1
	mutex_lock(&big->data->bulk_temp_read_lock);
	if (big->data->bulk_buffer == NULL)
		big->data->bulk_buffer = kzalloc(sizeof(struct shtc1_buffer),
				GFP_KERNEL);
	big->data->bulk_buffer->len = big->length / 12;
#endif
	while (residue > 0) {
		packet_len = (residue > DATA_PACKET_SIZE) ?
				DATA_PACKET_SIZE : residue;

		msg = kzalloc(sizeof(*msg), GFP_KERNEL);
		msg->cmd = MSG2SSP_AP_GET_BIG_DATA;
		msg->length = packet_len;
		msg->options = AP2HUB_READ | (index++ << SSP_INDEX);
		msg->data = big->addr;
		msg->buffer = buffer;
		msg->free_buffer = 0;

		iRet = ssp_spi_sync(big->data, msg, 1000);
		if (iRet != SUCCESS) {
			pr_err("[SSP]: %s - Fail to receive data %d\n",
				__func__, iRet);
			break;
		}
		/* 12 = 1 chunk size for ks79.shin
		 * order is thermistor Bat, thermistor PA, Temp,
		 * Humidity, Baro, Gyro
		 * each data consist of 2bytes
		 */
		i = 0;
		while (packet_len - i >= 12) {
			ssp_dbg("[SSP]: %s %d %d %d %d %d %d", __func__,
					*((s16 *) (buffer + i + 0)), *((s16 *) (buffer + i + 2)),
					*((s16 *) (buffer + i + 4)), *((s16 *) (buffer + i + 6)),
					*((s16 *) (buffer + i + 8)), *((s16 *) (buffer + i + 10)));
#ifdef CONFIG_SENSORS_SSP_SHTC1
			big->data->bulk_buffer->batt[buffindex] = *((u16 *) (buffer + i + 0));
			big->data->bulk_buffer->chg[buffindex] = *((u16 *) (buffer + i + 2));
			big->data->bulk_buffer->temp[buffindex] = *((s16 *) (buffer + i + 4));
			big->data->bulk_buffer->humidity[buffindex] = *((u16 *) (buffer + i + 6));
			big->data->bulk_buffer->baro[buffindex] = *((s16 *) (buffer + i + 8));
			big->data->bulk_buffer->gyro[buffindex] = *((s16 *) (buffer + i + 10));
			buffindex++;
			i += 12;
#else
			buffindex++;
			i += 12;/* 6 ?? */
#endif
		}

		residue -= packet_len;
	}
#ifdef CONFIG_SENSORS_SSP_SHTC1
	if (iRet == SUCCESS)
		report_bulk_comp_data(big->data);
	mutex_unlock(&big->data->bulk_temp_read_lock);
#endif
	kfree(buffer);
	kfree(big);
	ssp_dbg("[SSP]: %s done\n", __func__);
#endif
}

/*************************************************************************/
/* SSP Debug timer function                                              */
/*************************************************************************/
int print_mcu_debug(char *pchRcvDataFrame, int *pDataIdx,
		int iRcvDataFrameLength)
{
	int iLength = 0;
	int cur = *pDataIdx;

	memcpy(&iLength, pchRcvDataFrame + *pDataIdx, sizeof(u16));
	*pDataIdx += sizeof(u16);

	if (iLength > iRcvDataFrameLength - *pDataIdx || iLength <= 0) {
		ssp_dbg("[SSP]: MSG From MCU - invalid debug length(%d/%d/%d)\n",
			iLength, iRcvDataFrameLength, cur);
		return iLength ? iLength : ERROR;
	}

	ssp_dbg("[SSP]: MSG From MCU - %s\n", &pchRcvDataFrame[*pDataIdx]);
	*pDataIdx += iLength;
	return 0;
}

void reset_mcu(struct ssp_data *data)
{
	func_dbg();

	ssp_enable(data, false);
	clean_pending_list(data);
	bbd_mcu_reset(false);

	data->uTimeOutCnt = 0;
	data->uComFailCnt = 0;
	data->mcuAbnormal = false;
}

void sync_sensor_state(struct ssp_data *data)
{
	unsigned char uBuf[9] = {0,};
	unsigned int uSensorCnt;
	int iRet = 0;

	gyro_open_calibration(data);
	iRet = set_gyro_cal(data);
	if (iRet < 0)
		pr_err("[SSP]: %s - set_gyro_cal failed\n", __func__);

	iRet = set_accel_cal(data);
	if (iRet < 0)
		pr_err("[SSP]: %s - set_accel_cal failed\n", __func__);

#ifdef CONFIG_SENSORS_SSP_PROX_ADC_CAL
	proximity_open_calibration(data);
	set_prox_cal(data);
#endif

#ifdef CONFIG_SENSORS_SSP_SX9306
	if (atomic64_read(&data->aSensorEnable) & (1 << GRIP_SENSOR)) {
		open_grip_caldata(data);
		set_grip_calibration(data, true);
	}
#endif

	udelay(10);

	for (uSensorCnt = 0; uSensorCnt < SENSOR_MAX; uSensorCnt++) {
		mutex_lock(&data->enable_mutex);
		if (atomic64_read(&data->aSensorEnable) & (1ULL << uSensorCnt)) {
			s32 dMsDelay =
				get_msdelay(data->adDelayBuf[uSensorCnt]);
			memcpy(&uBuf[0], &dMsDelay, 4);
			memcpy(&uBuf[4], &data->batchLatencyBuf[uSensorCnt], 4);
			uBuf[8] = data->batchOptBuf[uSensorCnt];
			send_instruction(data, ADD_SENSOR, uSensorCnt, uBuf, 9);
			udelay(10);
		}
		mutex_unlock(&data->enable_mutex);
	}
        if (atomic64_read(&data->aSensorEnable) & (1ULL << GYROSCOPE_SENSOR))
                send_vdis_flag(data, data->IsVDIS_Enabled);

	if (data->bProximityRawEnabled == true) {
		s32 dMsDelay = 20;

		memcpy(&uBuf[0], &dMsDelay, 4);
		send_instruction(data, ADD_SENSOR, PROXIMITY_RAW, uBuf, 4);
	}

	set_proximity_threshold(data);
	set_light_coef(data);

	data->bMcuDumpMode = ssp_check_sec_dump_mode();
	iRet = ssp_send_cmd(data, MSG2SSP_AP_MCU_SET_DUMPMODE,
		data->bMcuDumpMode);
	if (iRet < 0)
		pr_err("[SSP]: %s - MSG2SSP_AP_MCU_SET_DUMPMODE failed\n",
			__func__);
}

static void print_sensordata(struct ssp_data *data, unsigned int uSensor)
{
	switch (uSensor) {
	case ACCELEROMETER_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d (%ums, %dms)\n", uSensor,
			data->buf[uSensor].x, data->buf[uSensor].y,
			data->buf[uSensor].z,
			get_msdelay(data->adDelayBuf[uSensor]),
			data->batchLatencyBuf[uSensor]);
		break;
	case GYROSCOPE_SENSOR:
#ifdef CONFIG_SENSORS_SSP_INTERRUPT_GYRO_SENSOR
	case INTERRUPT_GYRO_SENSOR:
#endif
		ssp_dbg("[SSP] %u : %d, %d, %d (%ums, %dms)\n", uSensor,
			data->buf[uSensor].gyro.x, data->buf[uSensor].gyro.y,
			data->buf[uSensor].gyro.z,
			get_msdelay(data->adDelayBuf[uSensor]),
			data->batchLatencyBuf[uSensor]);
		break;
	case GEOMAGNETIC_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].cal_x, data->buf[uSensor].cal_y,
			data->buf[uSensor].cal_z, data->buf[uSensor].accuracy,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case GEOMAGNETIC_UNCALIB_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d, %d, %d, %d (%ums)\n",
			uSensor,
			data->buf[uSensor].uncal_x,
			data->buf[uSensor].uncal_y,
			data->buf[uSensor].uncal_z,
			data->buf[uSensor].offset_x,
			data->buf[uSensor].offset_y,
			data->buf[uSensor].offset_z,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case PRESSURE_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d (%ums, %dms)\n", uSensor,
			data->buf[uSensor].pressure,
			data->buf[uSensor].temperature,
			get_msdelay(data->adDelayBuf[uSensor]),
			data->batchLatencyBuf[uSensor]);
		break;
	case GESTURE_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].data[3], data->buf[uSensor].data[4],
			data->buf[uSensor].data[5], data->buf[uSensor].data[6],
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case TEMPERATURE_HUMIDITY_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].x, data->buf[uSensor].y,
			data->buf[uSensor].z,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case LIGHT_SENSOR:
	case UNCAL_LIGHT_SENSOR:
		ssp_dbg("[SSP] %u : %u, %u, %u, %u, %u, %u (%ums)\n", uSensor,
			data->buf[uSensor].r, data->buf[uSensor].g,
			data->buf[uSensor].b, data->buf[uSensor].w,
			data->buf[uSensor].a_time, data->buf[uSensor].a_gain,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case LIGHT_FLICKER_SENSOR:
		ssp_dbg("[SSP] %u : %u, (%ums)\n", uSensor,
			data->buf[uSensor].light_flicker, get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case LIGHT_CCT_SENSOR:
		ssp_dbg("[SSP] %u : %u, %u, %u, %u, %u, %u (%ums)\n", uSensor,
			data->buf[uSensor].r, data->buf[uSensor].g,
			data->buf[uSensor].b, data->buf[uSensor].w,
			data->buf[uSensor].a_time, data->buf[uSensor].a_gain,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case PROXIMITY_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].prox_detect, data->buf[uSensor].prox_adc,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case PROXIMITY_POCKET:
		ssp_dbg("[SSP] %u : %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].proximity_pocket_detect, data->buf[uSensor].proximity_pocket_adc,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case PROXIMITY_ALERT_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].prox_alert_detect, data->buf[uSensor].prox_alert_adc,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case STEP_DETECTOR:
		ssp_dbg("[SSP] %u : %u (%ums, %dms)\n", uSensor,
			data->buf[uSensor].step_det,
			get_msdelay(data->adDelayBuf[uSensor]),
			data->batchLatencyBuf[uSensor]);
		break;
	case GAME_ROTATION_VECTOR:
	case ROTATION_VECTOR:
		ssp_dbg("[SSP] %u : %d, %d, %d, %d, %d (%ums, %dms)\n", uSensor,
			data->buf[uSensor].quat_a, data->buf[uSensor].quat_b,
			data->buf[uSensor].quat_c, data->buf[uSensor].quat_d,
			data->buf[uSensor].acc_rot,
			get_msdelay(data->adDelayBuf[uSensor]),
			data->batchLatencyBuf[uSensor]);
		break;
	case SIG_MOTION_SENSOR:
		ssp_dbg("[SSP] %u : %u(%ums)\n", uSensor,
			data->buf[uSensor].sig_motion,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case GYRO_UNCALIB_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d, %d, %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].uncal_gyro.x, data->buf[uSensor].uncal_gyro.y,
			data->buf[uSensor].uncal_gyro.z, data->buf[uSensor].uncal_gyro.offset_x,
			data->buf[uSensor].uncal_gyro.offset_y,
			data->buf[uSensor].uncal_gyro.offset_z,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case STEP_COUNTER:
		ssp_dbg("[SSP] %u : %u(%ums)\n", uSensor,
			data->buf[uSensor].step_diff,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case LIGHT_IR_SENSOR:
		ssp_dbg("[SSP] %u : %u, %u, %u, %u, %u, %u, %u (%ums)\n", uSensor,
			data->buf[uSensor].irdata,
			data->buf[uSensor].ir_r, data->buf[uSensor].ir_g,
			data->buf[uSensor].ir_b, data->buf[uSensor].ir_w,
			data->buf[uSensor].ir_a_time, data->buf[uSensor].ir_a_gain,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case TILT_DETECTOR:
		ssp_dbg("[SSP] %u : %u(%ums)\n", uSensor,
			data->buf[uSensor].tilt_detector,
		get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case PICKUP_GESTURE:
		ssp_dbg("[SSP] %u : %u(%ums)\n", uSensor,
			data->buf[uSensor].pickup_gesture,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case THERMISTOR_SENSOR:
		ssp_dbg("[SSP] %u : %u %u (%ums)\n", uSensor,
			data->buf[uSensor].thermistor_type,
			data->buf[uSensor].thermistor_raw,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case ACCEL_UNCALIB_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d, %d, %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].uncal_x, data->buf[uSensor].uncal_y,
			data->buf[uSensor].uncal_z, data->buf[uSensor].offset_x,
			data->buf[uSensor].offset_y,
			data->buf[uSensor].offset_z,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case WAKE_UP_MOTION:
		ssp_dbg("[SSP] %u : %d (%ums)\n", uSensor,
			data->buf[uSensor].wakeup_move_event[0],
			get_msdelay(data->adDelayBuf[uSensor]));
        break;
    case CALL_GESTURE:
		ssp_dbg("[SSP] %u : %d (%ums)\n", uSensor,
			data->buf[uSensor].call_gesture,
			get_msdelay(data->adDelayBuf[uSensor]));
        break;
                break;
        case MOVE_DETECTOR:
		ssp_dbg("[SSP] %u : %d (%ums)\n", uSensor,
			data->buf[uSensor].wakeup_move_event[1],
			get_msdelay(data->adDelayBuf[uSensor]));
                break;
	case POCKET_MODE_SENSOR:
		ssp_dbg("[SSP] %u : %d (%ums)\n", uSensor,
			data->buf[uSensor].pocket_mode,
			get_msdelay(data->adDelayBuf[uSensor]));
                break;
	case LED_COVER_EVENT_SENSOR:
		ssp_dbg("[SSP] %u : %d (%ums)\n", uSensor,
			data->buf[uSensor].led_cover_event,
			get_msdelay(data->adDelayBuf[uSensor]));
                break;
	case AUTO_ROTATION_SENSOR:
		ssp_dbg("[SSP] %u : %d (%ums)\n", uSensor,
			data->buf[uSensor].auto_rotation_event,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case SAR_BACKOFF_MOTION:
		ssp_dbg("[SSP] %u : %d (%ums)\n", uSensor,
			data->buf[uSensor].sar_backoff_motion_event,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case BULK_SENSOR:
	case GPS_SENSOR:
		break;
	default:
		ssp_dbg("[SSP] Wrong sensorCnt: %u\n", uSensor);
		break;
	}
}
/*
 *check_sensor_event
 *- return
 *        true : there is no accel or light sensor event over 5sec when sensor is registered
 */
bool check_wait_event(struct ssp_data *data)
{
	u64 timestamp = get_current_timestamp();
	int check_sensors[2] = {ACCELEROMETER_SENSOR, LIGHT_SENSOR};
	int i, sensor;
	bool res = false;
	int arrSize = (ANDROID_VERSION < 90000 ? 2 : 1);

	for (i = 0 ; i < arrSize ; i++) { // because light sensor does not check anymore
		sensor = check_sensors[i];
		//the sensor is registered
		if ((atomic64_read(&data->aSensorEnable) & (1 << sensor) && !(data->IsGyroselftest))
			//non batching mode
			&& data->IsBypassMode[sensor] == 1
			//there is no sensor event over 3sec
			&& data->LastSensorTimeforReset[sensor] + 7000000000ULL < timestamp) {
			pr_info("[SSP] %s - sensor(%d) last = %lld, cur = %lld\n",
				__func__, sensor, data->LastSensorTimeforReset[sensor], timestamp);
			res = true;
			data->uNoRespSensorCnt++;
		}
		//pr_info("[SSP]test %s - sensor(%d mode %d) last = %lld, cur = %lld\n",
		//__func__,sensor,data->IsBypassMode[sensor],data->LastSensorTimeforReset[sensor],timestamp);
	}

	return res;
}

static void debug_work_func(struct work_struct *work)
{
	unsigned int uSensorCnt;
	struct ssp_data *data = container_of(work, struct ssp_data, work_debug);

	ssp_dbg("[SSP]: %s(%u) - Sensor state: 0x%llx, RC: %u(%u, %u, %u), CC: %u, TC: %u NSC: %u EC: %u GPS: %s\n",
		__func__, data->uIrqCnt, data->uSensorState, data->uResetCnt, data->mcuCrashedCnt, data->IsNoRespCnt,
		data->resetCntGPSisOn, data->uComFailCnt, data->uTimeOutCnt, data->uNoRespSensorCnt, data->errorCount,
		data->IsGpsWorking ? "true" : "false");
    
        if(!strstr("", data->resetInfoDebug))
            pr_info("[SSP]: %s(%lld, %lld)\n", data->resetInfoDebug, data->resetInfoDebugTime, get_current_timestamp());

	for (uSensorCnt = 0; uSensorCnt < SENSOR_MAX; uSensorCnt++) {
		if ((atomic64_read(&data->aSensorEnable) & (1ULL << uSensorCnt))
			|| data->batchLatencyBuf[uSensorCnt]) {
			print_sensordata(data, uSensorCnt);
			if (data->indio_dev[uSensorCnt] != NULL
					&& list_empty(&data->indio_dev[uSensorCnt]->buffer->buffer_list)) {
				pr_err("[SSP] %u : buffer_list of iio:device%d is empty!\n",
						 uSensorCnt, data->indio_dev[uSensorCnt]->id);
			}
		}
	}

	if (data->resetting)
		goto exit;

	if (((atomic64_read(&data->aSensorEnable) & (1 << ACCELEROMETER_SENSOR))
	&& (data->batchLatencyBuf[ACCELEROMETER_SENSOR] == 0)
		&& (data->uIrqCnt == 0) && (data->uTimeOutCnt > 0))
		|| (data->uTimeOutCnt > LIMIT_TIMEOUT_CNT)
		|| (check_wait_event(data))
		|| (data->mcuAbnormal == true)) {

		mutex_lock(&data->ssp_enable_mutex);
			pr_info("[SSP] : %s - uTimeOutCnt(%u), pending(%u)\n",
				__func__, data->uTimeOutCnt,
				!list_empty(&data->pending_list));
			reset_mcu(data);
		mutex_unlock(&data->ssp_enable_mutex);
	}

exit:
	data->uIrqCnt = 0;
}

static void debug_timer_func(unsigned long ptr)
{
	struct ssp_data *data = (struct ssp_data *)ptr;

	queue_work(data->debug_wq, &data->work_debug);
	mod_timer(&data->debug_timer,
		round_jiffies_up(jiffies + SSP_DEBUG_TIMER_SEC));
}

void enable_debug_timer(struct ssp_data *data)
{
	mod_timer(&data->debug_timer,
		round_jiffies_up(jiffies + SSP_DEBUG_TIMER_SEC));
}

void disable_debug_timer(struct ssp_data *data)
{
	del_timer_sync(&data->debug_timer);
	cancel_work_sync(&data->work_debug);
}

int initialize_debug_timer(struct ssp_data *data)
{
	setup_timer(&data->debug_timer, debug_timer_func, (unsigned long)data);

	data->debug_wq = create_singlethread_workqueue("ssp_debug_wq");
	if (!data->debug_wq)
		return ERROR;

	INIT_WORK(&data->work_debug, debug_work_func);
	return SUCCESS;
}

/* if returns true dump mode on */
unsigned int ssp_check_sec_dump_mode(void)
{
#if 0 /* def CONFIG_SEC_DEBUG */
	if (sec_debug_level.en.kernel_fault == 1)
		return 1;
	else
		return 0;
#endif
	return 0;
}
