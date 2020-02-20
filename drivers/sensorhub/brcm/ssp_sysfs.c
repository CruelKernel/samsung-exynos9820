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
#include <linux/iio/iio.h>
#include <linux/math64.h>
#include <linux/string.h>
#include "ssp_iio.h"
#define BATCH_IOCTL_MAGIC		0xFC

struct batch_config {
	int64_t timeout;
	int64_t delay;
	int flag;
};

/*************************************************************************/
/* SSP data delay function                                              */
/*************************************************************************/

int get_msdelay(int64_t dDelayRate)
{
	return div_s64(dDelayRate, 1000000);
}

s32 SettingVDIS_Support(struct ssp_data *data, int64_t *dNewDelay)
{
	s32 dMsDelay = 0;
	int64_t NewDelay = *dNewDelay;

	if ((NewDelay == CAMERA_GYROSCOPE_SYNC || NewDelay == CAMERA_GYROSCOPE_VDIS_SYNC
		|| NewDelay == CAMERA_GYROSCOPE_SUPER_VDIS_SYNC
		|| NewDelay == CAMERA_GYROSCOPE_ULTRA_VDIS_SYNC) && !data->IsVDIS_Enabled) {
		data->IsVDIS_Enabled = true;
		data->ts_stacked_cnt = 0;
		send_vdis_flag(data, data->IsVDIS_Enabled);
	} else if (!(NewDelay == CAMERA_GYROSCOPE_SYNC || NewDelay == CAMERA_GYROSCOPE_VDIS_SYNC
		|| NewDelay == CAMERA_GYROSCOPE_SUPER_VDIS_SYNC
		|| NewDelay == CAMERA_GYROSCOPE_ULTRA_VDIS_SYNC) && data->IsVDIS_Enabled) {
		data->IsVDIS_Enabled = false;
		data->ts_stacked_cnt = 0;
		send_vdis_flag(data, data->IsVDIS_Enabled);
	}
	if (NewDelay == CAMERA_GYROSCOPE_SYNC) {
		*dNewDelay = CAMERA_GYROSCOPE_SYNC_DELAY;
		data->cameraGyroSyncMode = true;
	} else if (NewDelay == CAMERA_GYROSCOPE_VDIS_SYNC) {
		*dNewDelay = CAMERA_GYROSCOPE_VDIS_SYNC_DELAY;
		data->cameraGyroSyncMode = true;
	} else if (NewDelay == CAMERA_GYROSCOPE_SUPER_VDIS_SYNC) {
		*dNewDelay = CAMERA_GYROSCOPE_SUPER_VDIS_SYNC_DELAY;
		data->cameraGyroSyncMode = true;
		initialize_super_vdis_setting();
	} else if (NewDelay == CAMERA_GYROSCOPE_ULTRA_VDIS_SYNC) {
		*dNewDelay = CAMERA_GYROSCOPE_ULTRA_VDIS_SYNC_DELAY;
		data->cameraGyroSyncMode = true;
		initialize_super_vdis_setting();
	} else {
		data->cameraGyroSyncMode = false;
		if ((data->adDelayBuf[GYROSCOPE_SENSOR] == NewDelay)
			&& (data->aiCheckStatus[GYROSCOPE_SENSOR] == RUNNING_SENSOR_STATE)) {
			pr_err("[SSP] same delay ignored!\n");
			return 0;
		}
	}

	if ((data->cameraGyroSyncMode == true) && (data->aiCheckStatus[GYROSCOPE_SENSOR] == RUNNING_SENSOR_STATE)) {
		if (data->adDelayBuf[GYROSCOPE_SENSOR] == NewDelay)
			return 0;
	}

	dMsDelay = get_msdelay(*dNewDelay);
	return dMsDelay;
}

static void enable_sensor(struct ssp_data *data,
	int iSensorType, int64_t dNewDelay)
{
	u8 uBuf[9];
	u64 uNewEnable = 0;
	s32 maxBatchReportLatency = 0;
	s8 batchOptions = 0;
	int64_t dTempDelay = data->adDelayBuf[iSensorType];
	s32 dMsDelay = get_msdelay(dNewDelay);
	int ret = 0;

	// SUPPORT CAMERA SYNC ++++++
	if (iSensorType == GYROSCOPE_SENSOR) {
		dMsDelay = SettingVDIS_Support(data, &dNewDelay);
		if (dMsDelay == 0)
			return;
	}
	// SUPPORT CAMERA SYNC -----

	data->adDelayBuf[iSensorType] = dNewDelay;
	maxBatchReportLatency = data->batchLatencyBuf[iSensorType];
	batchOptions = data->batchOptBuf[iSensorType];

	switch (data->aiCheckStatus[iSensorType]) {
	case ADD_SENSOR_STATE:
		ssp_dbg("[SSP]: %s - add %llu, New = %lldns\n",
			 __func__, 1ULL << iSensorType, dNewDelay);

		if (iSensorType == PROXIMITY_SENSOR) {
#ifdef CONFIG_SENSORS_SSP_PROX_FACTORYCAL
			get_proximity_threshold(data);
			proximity_open_calibration(data);
#endif
			set_proximity_threshold(data);
		}

		if (iSensorType == PROXIMITY_ALERT_SENSOR)
			set_proximity_alert_threshold(data);

#ifdef CONFIG_SENSORS_SSP_IRDATA_FOR_CAMERA
		if (iSensorType == LIGHT_SENSOR 
			|| iSensorType == LIGHT_IR_SENSOR
			|| iSensorType == LIGHT_CCT_SENSOR
			|| iSensorType == UNCAL_LIGHT_SENSOR ) {
			data->light_log_cnt = 0;
			data->light_ir_log_cnt = 0;
			data->light_cct_log_cnt = 0;
		}
#else
		if (iSensorType == LIGHT_SENSOR
			|| iSensorType == LIGHT_CCT_SENSOR
			|| iSensorType == UNCAL_LIGHT_SENSOR)
			data->light_log_cnt = 0;
			data->light_cct_log_cnt = 0;
#endif

#ifdef CONFIG_SENSORS_SSP_SX9306
		if (iSensorType == GRIP_SENSOR) {
			open_grip_caldata(data);
			set_grip_calibration(data, true);
		}
#endif

		memcpy(&uBuf[0], &dMsDelay, 4);
		memcpy(&uBuf[4], &maxBatchReportLatency, 4);
		uBuf[8] = batchOptions;

		if (iSensorType == TEMPERATURE_HUMIDITY_SENSOR &&
			dMsDelay == 10000)
			pr_info("[SSP] Skip Add Temphumidity Sensor\n");
		else
			ret = send_instruction(data, ADD_SENSOR,
				iSensorType, uBuf, 9);
		pr_info("[SSP], delay %d, timeout %d, flag=%d, ret%d\n",
			dMsDelay, maxBatchReportLatency, uBuf[8], ret);
		if (ret <= 0) {
			uNewEnable =
				(u64)atomic64_read(&data->aSensorEnable)
				& (~(u64)(1ULL << iSensorType));
			atomic64_set(&data->aSensorEnable, uNewEnable);

			data->aiCheckStatus[iSensorType] = NO_SENSOR_STATE;
			break;
		}

		data->aiCheckStatus[iSensorType] = RUNNING_SENSOR_STATE;

		break;
	case RUNNING_SENSOR_STATE:
		if (get_msdelay(dTempDelay)
			== get_msdelay(data->adDelayBuf[iSensorType]))
			break;

		ssp_dbg("[SSP]: %s - Change %llu, New = %lldns\n",
			__func__, 1ULL << iSensorType, dNewDelay);

		memcpy(&uBuf[0], &dMsDelay, 4);
		memcpy(&uBuf[4], &maxBatchReportLatency, 4);
		uBuf[8] = batchOptions;
		send_instruction(data, CHANGE_DELAY, iSensorType, uBuf, 9);

		break;
	default:
		data->aiCheckStatus[iSensorType] = ADD_SENSOR_STATE;
	}
}
static void change_sensor_delay(struct ssp_data *data,
	int iSensorType, int64_t dNewDelay)
{
	u8 uBuf[9];
	s32 maxBatchReportLatency = 0;
	s8 batchOptions = 0;
	int64_t dTempDelay = data->adDelayBuf[iSensorType];
	s32 dMsDelay = get_msdelay(dNewDelay);

	// SUPPORT CAMERA SYNC ++++++
	if (iSensorType == GYROSCOPE_SENSOR) {
		dMsDelay = SettingVDIS_Support(data, &dNewDelay);
		if (dMsDelay == 0)
			return;
	}
	// SUPPORT CAMERA SYNC -----

	data->adDelayBuf[iSensorType] = dNewDelay;
	data->batchLatencyBuf[iSensorType] = maxBatchReportLatency;
	data->batchOptBuf[iSensorType] = batchOptions;

	switch (data->aiCheckStatus[iSensorType]) {
	case RUNNING_SENSOR_STATE:
		if (get_msdelay(dTempDelay)
			== get_msdelay(data->adDelayBuf[iSensorType]))
			break;

		ssp_dbg("[SSP]: %s - Change %llu, New = %lldns\n",
			__func__, 1ULL << iSensorType, dNewDelay);

		memcpy(&uBuf[0], &dMsDelay, 4);
		memcpy(&uBuf[4], &maxBatchReportLatency, 4);
		uBuf[8] = batchOptions;
		send_instruction(data, CHANGE_DELAY, iSensorType, uBuf, 9);

		break;
	default:
		break;
	}
}

/*************************************************************************/
/* SSP data enable function                                              */
/*************************************************************************/

static int ssp_remove_sensor(struct ssp_data *data,
	unsigned int uChangedSensor, u64 uNewEnable)
{
	u8 uBuf[4];
	int64_t dSensorDelay = data->adDelayBuf[uChangedSensor];

	ssp_dbg("[SSP]: %s - remove sensor = %lld, current state = %lld\n",
		__func__, (u64)(1ULL << uChangedSensor), uNewEnable);

	data->adDelayBuf[uChangedSensor] = DEFAULT_POLLING_DELAY;
	data->batchLatencyBuf[uChangedSensor] = 0;
	data->batchOptBuf[uChangedSensor] = 0;

	if (uChangedSensor == ORIENTATION_SENSOR) {
		if (!(atomic64_read(&data->aSensorEnable)
			& (1 << ACCELEROMETER_SENSOR))) {
			uChangedSensor = ACCELEROMETER_SENSOR;
		} else {
			change_sensor_delay(data, ACCELEROMETER_SENSOR,
				data->adDelayBuf[ACCELEROMETER_SENSOR]);
			return 0;
		}
	} else if (uChangedSensor == ACCELEROMETER_SENSOR) {
		if (atomic64_read(&data->aSensorEnable)
			& (1 << ORIENTATION_SENSOR)) {
			change_sensor_delay(data, ORIENTATION_SENSOR,
				data->adDelayBuf[ORIENTATION_SENSOR]);
			return 0;
		}
	} else if (uChangedSensor == GYROSCOPE_SENSOR) {
		data->cameraGyroSyncMode = false;
		if (data->IsVDIS_Enabled) {
			data->IsVDIS_Enabled = false;
			send_vdis_flag(data, data->IsVDIS_Enabled);
		}
	}

	if (!data->bSspShutdown)
		if (atomic64_read(&data->aSensorEnable) & (1ULL << uChangedSensor)) {
			s32 dMsDelay = get_msdelay(dSensorDelay);

			memcpy(&uBuf[0], &dMsDelay, 4);

			send_instruction(data, REMOVE_SENSOR,
				uChangedSensor, uBuf, 4);
		}
	data->aiCheckStatus[uChangedSensor] = NO_SENSOR_STATE;

	return 0;
}

/*************************************************************************/
/* ssp Sysfs                                                             */
/*************************************************************************/

static ssize_t show_enable_irq(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_dbg("[SSP]: %s - %d\n", __func__, !data->bSspShutdown);

	return sprintf(buf, "%d\n", !data->bSspShutdown);
}

static ssize_t set_enable_irq(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	u8 dTemp;
	struct ssp_data *data = dev_get_drvdata(dev);

	if (kstrtou8(buf, 10, &dTemp) < 0)
		return -1;

	pr_info("[SSP] %s - %d start\n", __func__, dTemp);
	mutex_lock(&data->ssp_enable_mutex);
	if (dTemp) {
		reset_mcu(data);
		enable_debug_timer(data);
	} else if (!dTemp) {
		disable_debug_timer(data);
		ssp_enable(data, 0);
	} else
		pr_err("[SSP] %s - invalid value\n", __func__);
	mutex_unlock(&data->ssp_enable_mutex);
	pr_info("[SSP] %s - %d end\n", __func__, dTemp);
	return size;
}

static ssize_t show_sensors_enable(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_dbg("[SSP]: %s - cur_enable = %llu\n", __func__,
		 (u64)(atomic64_read(&data->aSensorEnable)));

	return sprintf(buf, "%llu\n", (u64)(atomic64_read(&data->aSensorEnable)));
}

static ssize_t set_sensors_enable(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	u64 dTemp;
	u64 uNewEnable = 0;
	unsigned int uChangedSensor = 0;
	struct ssp_data *data = dev_get_drvdata(dev);
	int ret = kstrtoull(buf, 10, &dTemp); 

	if (ret < 0) {
		pr_info("[SSP] %s - kstrtoull failed (%d)\n", __func__, ret);
		return ret;
	}

	uNewEnable = (u64)dTemp;
	ssp_dbg("[SSP]: %s - new_enable = %llu, old_enable = %llu\n", __func__,
		 uNewEnable, (u64)atomic64_read(&data->aSensorEnable));

	if ((uNewEnable != atomic64_read(&data->aSensorEnable)) &&
		!(data->uSensorState & (uNewEnable - atomic64_read(&data->aSensorEnable)))) {
		pr_info("[SSP] %s - %llu is not connected(sensortate: 0x%llx)\n",
			__func__, uNewEnable - atomic64_read(&data->aSensorEnable), data->uSensorState);
		return -EINVAL;
	}

	if (uNewEnable == atomic64_read(&data->aSensorEnable))
		return size;

	mutex_lock(&data->enable_mutex);
	for (uChangedSensor = 0; uChangedSensor < SENSOR_MAX; uChangedSensor++) {
		if ((atomic64_read(&data->aSensorEnable) & (1ULL << uChangedSensor))
			!= (uNewEnable & (1ULL << uChangedSensor))) {

			if (!(uNewEnable & (1ULL << uChangedSensor))) {
				data->reportedData[uChangedSensor] = false;
				ssp_remove_sensor(data, uChangedSensor,
					uNewEnable); /* disable */
			} else { /* Change to ADD_SENSOR_STATE from KitKat */
				if (data->aiCheckStatus[uChangedSensor] == INITIALIZATION_STATE) {
					if (uChangedSensor == ACCELEROMETER_SENSOR) {
						accel_open_calibration(data);
						if (set_accel_cal(data) < 0)
							pr_err("[SSP]: %s - set_accel_cal failed\n", __func__);
					} else if (uChangedSensor == PRESSURE_SENSOR)
						pressure_open_calibration(data);
					else if (uChangedSensor == PROXIMITY_SENSOR) {
#ifdef CONFIG_SENSORS_SSP_PROX_FACTORYCAL
						get_proximity_threshold(data);
						proximity_open_calibration(data);
#endif
						set_proximity_threshold(data);
					} else if (uChangedSensor == PROXIMITY_ALERT_SENSOR) {
						set_proximity_alert_threshold(data);
					}
#ifdef CONFIG_SENSORS_SSP_SX9306
					else if (uChangedSensor == GRIP_SENSOR) {
						open_grip_caldata(data);
						set_grip_calibration(data, true);
					}
#endif
				}
				data->aiCheckStatus[uChangedSensor] = ADD_SENSOR_STATE;
				enable_sensor(data, uChangedSensor, data->adDelayBuf[uChangedSensor]);
			}
			break;
		}
	}
	atomic64_set(&data->aSensorEnable, uNewEnable);
	mutex_unlock(&data->enable_mutex);

	return size;
}

static ssize_t set_flush(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int64_t dTemp;
	u8 sensor_type = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	if (kstrtoll(buf, 10, &dTemp) < 0)
		return -EINVAL;

	sensor_type = (u8)dTemp;
	if (!(atomic64_read(&data->aSensorEnable) & (1ULL << sensor_type)))
		return -EINVAL;

	if (flush(data, sensor_type) < 0) {
		pr_err("[SSP] ssp returns error for flush(%x)\n", sensor_type);
		return -EINVAL;
	}

	return size;
}

int get_index_by_name(struct iio_dev *indio_dev, struct ssp_data *data)
{
	int i = 0, index = -1;

	for (i = 0; i < SENSOR_MAX; i++) {
		if (data->indio_dev[i] != NULL && strcmp(data->indio_dev[i]->name, indio_dev->name) == 0) {
			index = i;
			break;
		}
	}

	return index;
}

static ssize_t show_sensor_delay(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct ssp_data *data = dev_get_drvdata(dev);
	int index = get_index_by_name(indio_dev, data);

	if (index >= 0) {
		snprintf(buf, PAGE_SIZE, "%lld\n", data->adDelayBuf[index]);
	}

	pr_err("[SSP]: %s, dev_name = %s index = %d\n", __func__, indio_dev->name, index);
	return 0;
}

static ssize_t set_sensor_delay(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct ssp_data *data = dev_get_drvdata(dev);
	int index = get_index_by_name(indio_dev, data);
	int64_t delay = 0;

	if (kstrtoll(buf, 10, &delay) < 0)
		return -EINVAL;

	if (index >= 0) {
		change_sensor_delay(data, index, delay);
	}

	pr_err("[SSP]: %s, dev_name = %s index = %d\n", __func__, indio_dev->name, index);
	return 0;
}

// for data injection
static ssize_t show_data_injection_enable(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data  = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->data_injection_enable);
}

static ssize_t set_data_injection_enable(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg;
	unsigned int iRet;
	int64_t buffer;

	if (kstrtoll(buf, 10, &buffer) < 0)
		return -EINVAL;

	if (buffer != 1 && buffer != 0)
		return -EINVAL;

	data->data_injection_enable = buffer;

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_AP_DATA_INJECTION_MODE_ON_OFF;
	msg->length = 1;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(1, GFP_KERNEL);

	msg->free_buffer = 1;
	msg->buffer[0] = data->data_injection_enable;
	iRet = ssp_spi_async(data, msg);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - Data Injection Mode fail %d\n", __func__, iRet);
		return iRet;
	}

	pr_info("[SSP] %s Data Injection Mode Success %d data_injection_enable %d\n",
		__func__, iRet, data->data_injection_enable);
	return size;
}

static ssize_t show_sensor_state(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data  = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", data->sensor_state);
}

//dev_attr_mcu_power
static ssize_t show_mcu_power(struct device *dev,
	struct device_attribute *attr, char *buf){
	struct ssp_data *data = dev_get_drvdata(dev);

	//pr_err("[SSP] %s++\n",__func__);

	if (data->regulator_vdd_mcu_1p8 != NULL) {
		pr_err("[SSP] %s: vdd_mcu_1p8, is enabled %d\n", __func__,
				regulator_is_enabled(data->regulator_vdd_mcu_1p8));
		return sprintf(buf, "%d\n", regulator_is_enabled(data->regulator_vdd_mcu_1p8));
	} else if (data->shub_en >= 0) {
		pr_err("[SSP] %s: shub_en(%d), is enabled %d\n", __func__,
				data->shub_en, gpio_get_value(data->shub_en));
		return sprintf(buf, "%d\n", gpio_get_value(data->shub_en));
	}
	return 0;
}

static ssize_t set_mcu_power(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	long long enable = 0;
	int ret = 0;

	//pr_err("[SSP] %s++\n",__func__);

	if (kstrtoll(buf, 8, &enable) < 0)
		return -1;

	if (data->regulator_vdd_mcu_1p8 != NULL) {
		switch (enable) {
		case 0:
			if (regulator_is_enabled(data->regulator_vdd_mcu_1p8))
				ret = regulator_disable(data->regulator_vdd_mcu_1p8);
			break;
		case 1:
			if (!regulator_is_enabled(data->regulator_vdd_mcu_1p8))
				ret = regulator_enable(data->regulator_vdd_mcu_1p8);
			if (ret)
				pr_err("[SSP] %s:failed to enable vdd_mcu_1p8(%d)\n", __func__, ret);
			break;
		}
		msleep(20);
		pr_err("[SSP] %s: enable vdd_mcu_1p8(%lld), is enabled %d\n", __func__,
				enable, regulator_is_enabled(data->regulator_vdd_mcu_1p8));
	} else if (data->shub_en >= 0) {
		switch (enable) {
		case 0:
			gpio_set_value(data->shub_en, 0);
			break;
		case 1:
			gpio_set_value(data->shub_en, 1);
			break;
		}
		msleep(20);
		pr_err("[SSP] %s: enable shub_en(%d), is enabled %d\n", __func__,
				data->shub_en, gpio_get_value(data->shub_en));
	} else {
		pr_err("[SSP] %s:no support to individual power source for sensorhub\n", __func__);
	}
	return size;
}

static ssize_t set_ssp_control(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int iRet = 0;

	pr_info("[SSP] SSP_CONTROL : %s\n", buf);

	if (strstr(buf, SSP_DEBUG_TIME_FLAG_ON))
		ssp_debug_time_flag = true;
	else if (strstr(buf, SSP_DEBUG_TIME_FLAG_OFF))
		ssp_debug_time_flag = false;
	else if (strstr(buf, SSP_HALL_IC_ON)
 		|| strstr(buf, SSP_HALL_IC_OFF)) {

		data->hall_ic_status = strstr(buf, SSP_HALL_IC_ON) ? 1 : 0;
	
		iRet = send_hall_ic_status(data->hall_ic_status);

		if (iRet != SUCCESS) {
			pr_err("[SSP]: %s - hall ic command, failed %d\n", __func__, iRet);
			return iRet;
		}

		pr_info("[SSP] %s HALL IC ON/OFF, %d enabled %d\n",
			__func__, iRet, data->hall_ic_status);

	}
	
	return size;
}

static ssize_t sensor_dump_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data  = dev_get_drvdata(dev);
	int types[] = SENSOR_DUMP_SENSOR_LIST;
	char str_no_sensor_dump[] = "there is no sensor dump";
	int i = 0, ret;
	char *sensor_dump;
	char temp[sensor_dump_length(DUMPREGISTER_MAX_SIZE)+LENGTH_SENSOR_TYPE_MAX+3] = {0,};

	sensor_dump = kzalloc((sensor_dump_length(DUMPREGISTER_MAX_SIZE)+LENGTH_SENSOR_TYPE_MAX+3)*(ARRAY_SIZE(types)),
			GFP_KERNEL);

	for (i = 0; i < ARRAY_SIZE(types); i++) {

		if (data->sensor_dump[types[i]] != NULL) {
			snprintf(temp, (int)strlen(data->sensor_dump[types[i]])+LENGTH_SENSOR_TYPE_MAX+3,
				"%3d\n%s\n\n",/* %3d -> 3 : LENGTH_SENSOR_TYPE_MAX */
				types[i], data->sensor_dump[types[i]]);
			strcpy(&sensor_dump[(int)strlen(sensor_dump)], temp);
		}
	}

	if ((int)strlen(sensor_dump) == 0)
		ret = snprintf(buf, (int)strlen(str_no_sensor_dump)+1, "%s\n", str_no_sensor_dump);
	else
		ret = snprintf(buf, (int)strlen(sensor_dump)+1, "%s\n", sensor_dump);

	kfree(sensor_dump);

	pr_info("[SSP]: %s, f/w version: BR01%u,BR01%u\n", __func__, get_module_rev(data), data->uCurFirmRev);

	return ret;
}

static ssize_t sensor_dump_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data  = dev_get_drvdata(dev);
	int sensor_type, ret;
	char name[LENGTH_SENSOR_NAME_MAX+1] = {0,};
	int iRet;

	iRet = sscanf(buf, "%30s", name);		/* 30 : LENGTH_SENSOR_NAME_MAX */
	if (iRet < 0)
		return iRet;

	if ((strcmp(name, "all")) == 0) {
		ret = send_all_sensor_dump_command(data);
	} else {
		if (strcmp(name, "accelerometer") == 0)
			sensor_type = ACCELEROMETER_SENSOR;
		else if (strcmp(name, "gyroscope") == 0)
			sensor_type = GYROSCOPE_SENSOR;
		else if (strcmp(name, "magnetic") == 0)
			sensor_type = GEOMAGNETIC_UNCALIB_SENSOR;
		else if (strcmp(name, "pressure") == 0)
			sensor_type = PRESSURE_SENSOR;
		else if (strcmp(name, "proximity") == 0)
			sensor_type = PROXIMITY_SENSOR;
		else if (strcmp(name, "light") == 0)
			sensor_type = LIGHT_SENSOR;
		else {
			pr_err("[SSP] %s - is not supported : %s", __func__, buf);
			sensor_type = -1;
			return -EINVAL;
		}
		ret = send_sensor_dump_command(data, sensor_type);
	}

	return (ret == SUCCESS) ? size : ret;
}

static ssize_t show_thermistor_channel0(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	short buffer = 0;
	int iRet = 0;

	msg->cmd = MSG2SSP_AP_THERMISTOR_CHANNEL_0;
	msg->length = sizeof(buffer);
	msg->options = AP2HUB_READ;
	msg->buffer = (char *)&buffer;

	iRet = ssp_spi_sync(data, msg, 1000);
	buffer = thermistor_rawToTemperature(data, 0, buffer);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - fail %d\n", __func__, iRet);
		return 0;
	} else {
		pr_err("[SSP]: %s -- %d\n", __func__, (int)buffer / 10);

		return sprintf(buf, "%d\n", (int)buffer / 10);
	}
}

static ssize_t show_thermistor_channel1(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	short buffer = 0;
	int iRet = 0;

	msg->cmd = MSG2SSP_AP_THERMISTOR_CHANNEL_1;
	msg->length = sizeof(buffer);
	msg->options = AP2HUB_READ;
	msg->buffer = (char *)&buffer;

	iRet = ssp_spi_sync(data, msg, 1000);
	buffer = thermistor_rawToTemperature(data, 1, buffer);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - fail %d\n", __func__, iRet);
		return 0;
	} else {
		pr_err("[SSP]: %s -- %d\n", __func__, (int)buffer / 10);

		return sprintf(buf, "%d\n", (int)buffer / 10);
	}
}

#if defined(CONFIG_SSP_REGISTER_RW)
int htoi(char input)
{
	int ret = 0;

	if ('0' <= input && input <= '9')
		return ret = input - '0';
	else if ('a' <= input && input <= 'f')
		return ret = input - 'a' + 10;
	else if ('A' <= input && input <= 'F')
		return ret = input - 'A' + 10;
	else
		return 0;
}

int checkInputtedRegisterString(const char *string, int *result)
{
	int ret = 0;
	int index = 0;
	char *Dupstring = NULL;
	char *pDupstring = NULL;
	char *CheckString[5] = {0, };

	pDupstring = Dupstring = kstrdup(string, GFP_KERNEL);

	while ((CheckString[index] = strsep(&Dupstring, " ")) != NULL) {
		u32 tmp = 0;

		switch (index) {
		case 0:
			if (kstrtou32(&CheckString[index][0], 10, &tmp) < 0 
					|| (tmp >= SENSOR_MAX && tmp != REGISTER_RW_DDI)) {
				pr_info("[SSP] %s invalid(%d)\n", __func__, tmp);
				goto exit;
			}
			break;
		case 1:
			if (CheckString[index][0] == 'r' || CheckString[index][0] == 'w') {
				tmp = (CheckString[index][0] == 'w' ? 0 : 1);
			} else if (kstrtou32(&CheckString[index][0], 10, &tmp) < 0 || ((tmp != 0) && (tmp != 1))) {
				pr_info("[SSP] %s invalid r/w\n", __func__);
				goto exit;
			}
			break;
		case 2:
		case 3:
			if (CheckString[index][0] != '0' && CheckString[index][1] != 'x') {
				pr_info("[SSP] %s invalid value(0xOO) %s\n", __func__, CheckString[index]);
				goto exit;
			}
			tmp = (uint8_t)((htoi(CheckString[index][2]) << 4) | htoi(CheckString[index][3]));
			ret = index;
			break;
		default:
			pr_info("[SSP] %s invalid cmd size", __func__);
			ret = false;
			goto exit;
		}
		result[index++] = tmp;
	}
	kfree(pDupstring);
return ret;
exit:
	ret = 0;
	kfree(pDupstring);
	pr_info("[SSP] %s - ret %d\n", __func__, ret);
return ret;
}

static ssize_t register_rw_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
		struct ssp_data *data = dev_get_drvdata(dev);
		
		if (data->registerValue[0] == REGISTER_RW_DDI) {
			int len = 0, i = 0;
			int read_size = data->registerValue[2];
	
			if (read_size > REGISTER_RW_BUFFER_MAX)
				return 0;

			for (i = 0; i < read_size; i++) {
				len += sprintf(buf + len, "%d ", data->registerValue[3 + i]);
			}
			//buf[len] = 0;
			pr_err("[SSP] : sensor(%d) register(%x) size(%d), %s\n", 
					data->registerValue[0], data->registerValue[1],
					data->registerValue[2], buf);

			return len;
		}

		if (data->registerValue[1] == 1) { // 1 is read
			return sprintf(buf, "sensor(%d) %c regi(0x%x) val(0x%x) ret(%d)\n",
						data->registerValue[0], data->registerValue[1] == 1 ? 'r' : 'w',
						data->registerValue[2], data->registerValue[3], data->registerValue[4]);
		} else {
			if (data->registerValue[4] == true)
				return sprintf(buf, "sensor(%d) %c regi(0x%x) val(0x%x) SUCCESS\n",
						data->registerValue[0], data->registerValue[1] == 1 ? 'r' : 'w',
						data->registerValue[2], data->registerValue[3]);
			else
				return sprintf(buf, "sensor(%d) %c regi(0x%x) val(0x%x) FAIL\n",
						data->registerValue[0], data->registerValue[1] == 1 ? 'r' : 'w',
						data->registerValue[2], data->registerValue[3]);
		}
}
static ssize_t register_rw_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
		struct ssp_data *data = dev_get_drvdata(dev);
		struct ssp_msg *msg;
		int index = 0, iRet = 0;
		int CheckString[4] = {0, };
		char sendBuff[REGISTER_RW_BUFFER_MAX + 3] = {0, };
		u16 dataLength = 0;

		msg = kzalloc(sizeof(*msg), GFP_KERNEL);
		if (ZERO_OR_NULL_PTR(msg)) {
			pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n", __func__);
			return -ENOMEM;
		}

		iRet = checkInputtedRegisterString(buf, CheckString);

		// common input format error
		if (iRet == 0) {
			goto register_rw_failed;
		}
	
		// ddi register format error
		if (CheckString[0] == REGISTER_RW_DDI 
				&& CheckString[3] > REGISTER_RW_BUFFER_MAX) {
			goto register_rw_failed;
		}

		msg->cmd = MSG2SSP_AP_REGISTER_SETTING;
		msg->length = dataLength = CheckString[0] == REGISTER_RW_DDI ? CheckString[3] + 3 : 5;
		msg->options = AP2HUB_READ;
		msg->data = 0;
		msg->buffer = sendBuff;
		msg->free_buffer = 0;

		for (index = 0; index <= iRet; index++)
		{
			msg->data |= (u32)(CheckString[index] << (24 - 8*index));
			pr_info("SSP %s %d: %d", __func__, index, CheckString[index]);
		}

		iRet = ssp_spi_sync(data, msg, 2000);

		if (iRet != SUCCESS)
			pr_err("[SSP] %s - fail %d\n", __func__, iRet);
		
		memcpy(data->registerValue, sendBuff, dataLength);

		return size;
register_rw_failed:
		kfree(msg);
		return -1;
}
static DEVICE_ATTR(register_rw, 0664,
	register_rw_show, register_rw_store);

#endif //CONFIG_SSP_REGISTER_RW
char resetString[1024] = {0, };
static ssize_t bcm_minidump_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data  = dev_get_drvdata(dev);
	int ret = 0;
	int totalLen = 0, index = 0;
        const char token = ';';

	totalLen = (int)strlen(data->resetInfo)
		+ (data->IsGpsWorking ? (int)strlen("GPS ON ") : (int)strlen("GPS OFF "));

	if ((int)strlen(data->resetInfo)) {
                memset(resetString, 0, ARRAY_SIZE(resetString));
                for(index = 0; index < (int)strlen(data->resetInfo); index++) {
                        if(data->resetInfo[index] == token)
                            break;
                        resetString[index] = data->resetInfo[index];
                }
                ret = snprintf(buf, totalLen + 1, "%s%s",
                        (data->IsGpsWorking ? "GPS ON " : "GPS OFF "), data->resetInfo);

        } else
		ret =  snprintf(buf, totalLen + 1, "%s", (data->IsGpsWorking ? "GPS ON " : "GPS OFF "));

	memset(data->resetInfo, 0, ARRAY_SIZE(data->resetInfo));

	return ret;
}

static ssize_t reset_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data  = dev_get_drvdata(dev);
	int ret = 0;
	int totalLen = 0, index = 0;
        const char token = ';';
        
	totalLen = (int)strlen(data->resetInfo)
		+ (data->IsGpsWorking ? (int)strlen("GPS ON ") : (int)strlen("GPS OFF "));

	if ((int)strlen(data->resetInfo)) {
                for(index = 0; index < (int)strlen(data->resetInfo); index++) {
                        if(data->resetInfo[index] == token)
                            break;
                        resetString[index] = data->resetInfo[index];
                }
        } else
		ret =  snprintf(buf, totalLen + 1, "%s", (data->IsGpsWorking ? "GPS ON " : "GPS OFF "));

        if((int)strlen(resetString)) {
                totalLen = (int)strlen(resetString)
		    + (data->IsGpsWorking ? (int)strlen("GPS ON ") : (int)strlen("GPS OFF "));
        	ret = snprintf(buf, totalLen + 1, "%s%s",
			(data->IsGpsWorking ? "GPS ON " : "GPS OFF "), resetString);
        }
        
        memset(resetString, 0, ARRAY_SIZE(resetString));
	return ret;
}

/*
static ssize_t show_timestamp_factor(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ssp_data *data  = dev_get_drvdata(dev);
	int ret = sprintf(buf, "timestamp_factor: %d\n", data->timestamp_factor);
	return ret;
}

static ssize_t set_timestamp_factor(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data  = dev_get_drvdata(dev);
	u64 dTemp = 5;

	int ret = kstrtoull(buf, 10, &dTemp); 

	if (ret < 0) {
		pr_info("[SSP] %s - kstrtoull failed (%d), set to default value(5)\n", __func__, ret);
		data->timestamp_factor = dTemp;
		return ret;
	}

	data->timestamp_factor = dTemp;

	return size;
}
*/

static DEVICE_ATTR(mcu_rev, 0444, mcu_revision_show, NULL);
static DEVICE_ATTR(mcu_name, 0444, mcu_model_name_show, NULL);
static DEVICE_ATTR(mcu_reset, 0444, mcu_reset_show, NULL);
static DEVICE_ATTR(mcu_test, 0664,
	mcu_factorytest_show, mcu_factorytest_store);
static DEVICE_ATTR(mcu_sleep_test, 0664,
	mcu_sleep_factorytest_show, mcu_sleep_factorytest_store);
static DEVICE_ATTR(poll_delay, 0664, show_sensor_delay, set_sensor_delay);
static DEVICE_ATTR(enable, 0664,
	show_sensors_enable, set_sensors_enable);
static DEVICE_ATTR(enable_irq, 0664,
	show_enable_irq, set_enable_irq);
static DEVICE_ATTR(ssp_flush, 0220,
	NULL, set_flush);

// for data injection
static DEVICE_ATTR(data_injection_enable, 0664,
	show_data_injection_enable, set_data_injection_enable);

static DEVICE_ATTR(ssp_control, 0220, NULL, set_ssp_control);
static DEVICE_ATTR(sensor_dump, 0664,
	sensor_dump_show, sensor_dump_store);
static DEVICE_ATTR(bcm_minidump, 0440, bcm_minidump_show, NULL);
static DEVICE_ATTR(reset_info, 0440, reset_info_show, NULL);
static DEVICE_ATTR(sensor_state, 0444, show_sensor_state, NULL);
static DEVICE_ATTR(mcu_power, 0664, show_mcu_power, set_mcu_power);

static DEVICE_ATTR(thermistor_channel_0, 0444, show_thermistor_channel0, NULL);
static DEVICE_ATTR(thermistor_channel_1, 0444, show_thermistor_channel1, NULL);

//static DEVICE_ATTR(timestamp_factor, 0664, show_timestamp_factor, set_timestamp_factor);

static struct device_attribute *mcu_attrs[] = {
	&dev_attr_enable,
	&dev_attr_mcu_rev,
	&dev_attr_mcu_name,
	&dev_attr_mcu_test,
	&dev_attr_mcu_reset,
	&dev_attr_mcu_sleep_test,
	&dev_attr_enable_irq,
	&dev_attr_ssp_flush,
	&dev_attr_data_injection_enable,
	&dev_attr_sensor_state,
	&dev_attr_ssp_control,
	&dev_attr_sensor_dump,
#if defined(CONFIG_SSP_REGISTER_RW)
	&dev_attr_register_rw,
#endif
	&dev_attr_thermistor_channel_0,
	&dev_attr_thermistor_channel_1,
	&dev_attr_bcm_minidump,
	&dev_attr_reset_info,
//	&dev_attr_timestamp_factor,
	NULL,
};

static long ssp_batch_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	struct ssp_data *data
		= container_of(file->private_data,
			struct ssp_data, batch_io_device);

	struct batch_config batch;

	void __user *argp = (void __user *)arg;
	int retries = 2;
	int ret = 0;
	int sensor_type, ms_delay;
	int64_t dNewDelay;
	int timeout_ms = 0;
	u8 uBuf[9];

	sensor_type = (cmd & 0xFF);

	if (sensor_type >= SENSOR_MAX) {
		pr_err("[SSP] Invalid sensor_type %d\n", sensor_type);
		return -EINVAL;
	}

	if ((cmd >> 8 & 0xFF) != BATCH_IOCTL_MAGIC) {
		pr_err("[SSP] Invalid BATCH CMD %x\n", cmd);
		return -EINVAL;
	}

	while (retries--) {
		ret = copy_from_user(&batch, argp, sizeof(batch));
		if (likely(!ret))
			break;
	}
	if (unlikely(ret)) {
		pr_err("[SSP] batch ioctl err(%d)\n", ret);
		return -EINVAL;
	}
	dNewDelay = batch.delay;
	ms_delay = get_msdelay(batch.delay);
	timeout_ms = div_s64(batch.timeout, 1000000);
	memcpy(&uBuf[0], &ms_delay, 4);
	memcpy(&uBuf[4], &timeout_ms, 4);
	uBuf[8] = batch.flag;

	// SUPPORT CAMERA SYNC ++++++
	if (sensor_type == GYROSCOPE_SENSOR) {
		ms_delay = SettingVDIS_Support(data, &dNewDelay);
		if (ms_delay == 0)
			return 0;
		memcpy(&uBuf[0], &ms_delay, 4);
	}
	// SUPPORT CAMERA SYNC -----

	if (batch.timeout) { /* add or dry */

		if (!(batch.flag & SENSORS_BATCH_DRY_RUN)) { /* real batch, NOT DRY, change delay */
			ret = 1;
			/* if sensor is not running state, enable will be called.
			 *  MCU return fail when receive chage delay inst during NO_SENSOR STATE
			 */
			if (data->aiCheckStatus[sensor_type] == RUNNING_SENSOR_STATE)
				ret = send_instruction(data, CHANGE_DELAY, sensor_type, uBuf, 9);

			if (ret > 0) { // ret 1 is success
				data->batchOptBuf[sensor_type] = (u8)batch.flag;
				data->batchLatencyBuf[sensor_type] = timeout_ms;
				data->adDelayBuf[sensor_type] = dNewDelay;
			}
		} else { /* real batch, DRY RUN */
			ret = send_instruction(data, CHANGE_DELAY, sensor_type, uBuf, 9);
			if (ret > 0) { // ret 1 is success
				data->batchOptBuf[sensor_type] = (u8)batch.flag;
				data->batchLatencyBuf[sensor_type] = timeout_ms;
				data->adDelayBuf[sensor_type] = dNewDelay;
			}
		}
	} else { /* remove batch or normal change delay, remove or add will be called. */
		if (!(batch.flag & SENSORS_BATCH_DRY_RUN)) { /* no batch, NOT DRY, change delay */
			data->batchOptBuf[sensor_type] = 0;
			data->batchLatencyBuf[sensor_type] = 0;
			data->adDelayBuf[sensor_type] = dNewDelay;
			if (data->aiCheckStatus[sensor_type] == RUNNING_SENSOR_STATE)
				send_instruction(data, CHANGE_DELAY, sensor_type, uBuf, 9);
		}
	}

	pr_info("[SSP] batch %d: delay %lld, timeout %lld, flag %d, ret %d\n",
		sensor_type, dNewDelay, batch.timeout, batch.flag, ret);
	if (!batch.timeout)
		return 0;
	if (ret <= 0)
		return -EINVAL;
	else
		return 0;
}


static struct file_operations const ssp_batch_fops = {
	.owner = THIS_MODULE,
	.open = nonseekable_open,
	.unlocked_ioctl = ssp_batch_ioctl,
};

static ssize_t ssp_data_injection_write(struct file *file, const char __user *buf,
				size_t count, loff_t *pos)
{
	struct ssp_data *data
		= container_of(file->private_data,
			struct ssp_data, ssp_data_injection_device);

	struct ssp_msg *msg;
	int ret = 0;
	char *send_buffer;
	unsigned int sensor_type = 0;
	size_t x = 0;

	if (unlikely(count < 4)) {
		pr_err("[SSP] %s data length err(%d)\n", __func__, (u32)count);
		return -EINVAL;
	}

	send_buffer = kcalloc(count, sizeof(char), GFP_KERNEL);
	if (unlikely(!send_buffer)) {
		pr_err("[SSP] %s allocate memory for kernel buffer err\n", __func__);
		return -ENOMEM;
	}

	ret = copy_from_user(send_buffer, buf, count);
	if (unlikely(ret)) {
		pr_err("[SSP] %s memcpy for kernel buffer err\n", __func__);
		ret = -EFAULT;
		goto exit;
	}

	pr_info("[SSP] %s count %d endable %d\n", __func__, (u32)count, data->data_injection_enable);

// sensorhub sensor type is enum, HAL layer sensor type is 1 << sensor_type. So it needs to change to enum format.
	for (sensor_type = 0; sensor_type < SENSOR_MAX; sensor_type++) {
		if (send_buffer[0] == (1 << sensor_type)) {
			send_buffer[0] = sensor_type; // sensor type change to enum format.
			pr_info("[SSP] %s sensor_type = %d %d\n", __func__, sensor_type, (1 << sensor_type));
			break;
		}
		if (sensor_type == SENSOR_MAX - 1)
			pr_info("[SSP] %s there in no sensor_type\n", __func__);
	}

	for (x = 0; x < count; x++)
		pr_info("[SSP] %s Data Injection : %d 0x%x\n", __func__, send_buffer[x], send_buffer[x]);


	// injection data send to sensorhub.
	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_AP_DATA_INJECTION_SEND;
	msg->length = count;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(count, GFP_KERNEL);
	msg->free_buffer = 1;

	memcpy(msg->buffer, send_buffer, count);

	ret = ssp_spi_async(data, msg);

	if (ret != SUCCESS) {
		pr_err("[SSP]: %s - Data Injection fail %d\n", __func__, ret);
		goto exit;
		//return ret;
	}

	pr_info("[SSP] %s Data Injection Success %d\n", __func__, ret);

exit:
	kfree(send_buffer);
	return ret >= 0 ? 0 : -1;
}

#if 0
static ssize_t ssp_data_injection_read(struct file *file, char __user *buf,
				size_t count, loff_t *pos)
{

	return 0;
}
#endif
static struct file_operations const ssp_data_injection_fops = {
	.owner = THIS_MODULE,
	.open = nonseekable_open,
	.write = ssp_data_injection_write,

};

/*
 *#if defined (CONFIG_SENSORS_SSP_VLTE)
 *int folder_state;
 *int ssp_ckeck_lcd(int state)
 *{
 *        folder_state = state;
 *        pr_info("[SSP] %s folder_state %d\n", __func__, folder_state);
 *
 *        return folder_state;
 *}
 *#endif
 */

static void initialize_mcu_factorytest(struct ssp_data *data)
{
	sensors_register(data->mcu_device, data, mcu_attrs, "ssp_sensor");
}

static void remove_mcu_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->mcu_device, mcu_attrs);
}

int initialize_sysfs(struct ssp_data *data)
{
	int i = 0;
	struct device *sec_sensorhub_dev = sec_device_create(data, "sensorhub");

	device_create_file(sec_sensorhub_dev, &dev_attr_mcu_power);

	for (i = 0; i < SENSOR_MAX; i++) {
		if (data->indio_dev[i] != NULL) {
			device_create_file(&data->indio_dev[i]->dev, &dev_attr_poll_delay);
		}
	}

	data->batch_io_device.minor = MISC_DYNAMIC_MINOR;
	data->batch_io_device.name = "batch_io";
	data->batch_io_device.fops = &ssp_batch_fops;
	if (misc_register(&data->batch_io_device))
		goto err_batch_io_dev;

	data->ssp_data_injection_device.minor = MISC_DYNAMIC_MINOR;
	data->ssp_data_injection_device.name = "ssp_data_injection";
	data->ssp_data_injection_device.fops = &ssp_data_injection_fops;
	if ((misc_register(&data->ssp_data_injection_device)) < 0)
		goto err_ssp_data_injection_device;

	initialize_accel_factorytest(data);
	initialize_gyro_factorytest(data);
#ifndef CONFIG_SENSORS_SSP_CM3323
	initialize_prox_factorytest(data);
#endif
	initialize_light_factorytest(data);
	initialize_pressure_factorytest(data);
	initialize_magnetic_factorytest(data);
	initialize_mcu_factorytest(data);
#ifdef CONFIG_SENSORS_SSP_TMG399x
	initialize_gesture_factorytest(data);
#endif

#ifdef CONFIG_SENSORS_SSP_IRLED
	initialize_irled_factorytest(data);
#endif
#ifdef CONFIG_SENSORS_SSP_SHTC1
	initialize_temphumidity_factorytest(data);
#endif
#ifdef CONFIG_SENSORS_SSP_MOBEAM
	initialize_mobeam(data);
#endif
#ifdef CONFIG_SENSORS_SSP_SX9306
	initialize_grip_factorytest(data);
#endif
#ifdef CONFIG_SENSORS_SSP_LIGHT_COLORID
	initialize_hiddenhole_factorytest(data);
#endif
	initialize_thermistor_factorytest(data);

	return SUCCESS;

err_ssp_data_injection_device:
	misc_deregister(&data->batch_io_device);
err_batch_io_dev:
	pr_err("[SSP] error init sysfs\n");
	return ERROR;
}

void remove_sysfs(struct ssp_data *data)
{
	misc_deregister(&data->batch_io_device);

	misc_deregister(&data->ssp_data_injection_device);

	remove_accel_factorytest(data);
	remove_gyro_factorytest(data);
#ifndef CONFIG_SENSORS_SSP_CM3323
	remove_prox_factorytest(data);
#endif
	remove_light_factorytest(data);
	remove_pressure_factorytest(data);
	remove_magnetic_factorytest(data);
	remove_mcu_factorytest(data);
#ifdef CONFIG_SENSORS_SSP_TMG399x
	remove_gesture_factorytest(data);
#endif
#ifdef CONFIG_SENSORS_SSP_IRLED
	remove_irled_factorytest(data);
#endif
#ifdef CONFIG_SENSORS_SSP_SHTC1
	remove_temphumidity_factorytest(data);
#endif
#ifdef CONFIG_SENSORS_SSP_MOBEAM
	remove_mobeam(data);
#endif
#ifdef CONFIG_SENSORS_SSP_SX9306
	remove_grip_factorytest(data);
#endif
#ifdef CONFIG_SENSORS_SSP_LIGHT_COLORID
	remove_hiddenhole_factorytest(data);
#endif
	remove_thremistor_factorytest(data);

	destroy_sensor_class();
}
