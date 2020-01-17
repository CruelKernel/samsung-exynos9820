/*
 *  Copyright (C) 2015, Samsung Electronics Co. Ltd. All Rights Reserved.
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
#include "../ssp.h"

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

#define VENDOR_YAS		"YAMAHA"
#define CHIP_ID_YAS		"YAS539"

#define VENDOR_AKM			"AKM"
#if defined(CONFIG_SENSORS_SSP_AK09918)
#define CHIP_ID_AKM			"AK09918C"
#else
#define CHIP_ID_AKM			"AK09916C"
#endif

#define GM_AKM_DATA_SPEC_MIN	-6500
#define GM_AKM_DATA_SPEC_MAX	6500

#define GM_YAS_DATA_SPEC_MIN	-6500
#define GM_YAS_DATA_SPEC_MAX	6500

#define GM_SELFTEST_X_SPEC_MIN	-200
#define GM_SELFTEST_X_SPEC_MAX	200
#define GM_SELFTEST_Y_SPEC_MIN	-200
#define GM_SELFTEST_Y_SPEC_MAX	200
#define GM_SELFTEST_Z_SPEC_MIN	-1000
#define GM_SELFTEST_Z_SPEC_MAX	-200

#define YAS_STATIC_ELLIPSOID_MATRIX	{10000, 0, 0, 0, 10000, 0, 0, 0, 10000}
#define MAG_HW_OFFSET_FILE_PATH	"/efs/FactoryApp/hw_offset"
#define MAG_CAL_PARAM_FILE_PATH	"/efs/FactoryApp/gyro_cal_data"


static int check_data_spec(struct ssp_data *data, int sensortype)
{
	int data_spec_max = 0;
	int data_spec_min = 0;

	if (data->mag_type == MAG_TYPE_AKM) {
		data_spec_max = GM_AKM_DATA_SPEC_MAX;
		data_spec_min = GM_AKM_DATA_SPEC_MIN;
	} else {
		data_spec_max = GM_YAS_DATA_SPEC_MAX;
		data_spec_min = GM_YAS_DATA_SPEC_MIN;
	}

	if ((data->buf[sensortype].x == 0) &&
		(data->buf[sensortype].y == 0) &&
		(data->buf[sensortype].z == 0))
		return FAIL;
	else if ((data->buf[sensortype].x > data_spec_max)
		|| (data->buf[sensortype].x < data_spec_min)
		|| (data->buf[sensortype].y > data_spec_max)
		|| (data->buf[sensortype].y < data_spec_min)
		|| (data->buf[sensortype].z > data_spec_max)
		|| (data->buf[sensortype].z < data_spec_min))
		return FAIL;
	else
		return SUCCESS;
}

/* AKM Functions */
static ssize_t magnetic_get_asa(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d,%d,%d\n", (s16)data->uFuseRomData[0],
		(s16)data->uFuseRomData[1], (s16)data->uFuseRomData[2]);
}

static ssize_t magnetic_get_status(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	bool bSuccess;
	struct ssp_data *data = dev_get_drvdata(dev);

	if ((data->uFuseRomData[0] == 0) ||
		(data->uFuseRomData[0] == 0xff) ||
		(data->uFuseRomData[1] == 0) ||
		(data->uFuseRomData[1] == 0xff) ||
		(data->uFuseRomData[2] == 0) ||
		(data->uFuseRomData[2] == 0xff))
		bSuccess = false;
	else
		bSuccess = true;

	return sprintf(buf, "%s,%u\n", (bSuccess ? "OK" : "NG"), bSuccess);
}

static ssize_t magnetic_check_cntl(struct device *dev,
		struct device_attribute *attr, char *strbuf)
{
	bool bSuccess = false;
	int ret;
	char chTempBuf[22] = { 0,  };
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg;

	if (!data->uMagCntlRegData) {
		bSuccess = true;
	} else {
		pr_info("[SSP] %s - check cntl register before selftest",
			__func__);
		msg = kzalloc(sizeof(*msg), GFP_KERNEL);
		msg->cmd = GEOMAGNETIC_FACTORY;
		msg->length = 22;
		msg->options = AP2HUB_READ;
		msg->buffer = chTempBuf;
		msg->free_buffer = 0;

		ret = ssp_spi_sync(data, msg, 1000);

		if (ret != SUCCESS) {
			pr_err("[SSP] %s - spi sync failed due to Timeout!! %d\n",
					__func__, ret);
		}

		data->uMagCntlRegData = chTempBuf[21];
		bSuccess = !data->uMagCntlRegData;
	}

	pr_info("[SSP] %s - CTRL : 0x%x\n", __func__,
				data->uMagCntlRegData);

	data->uMagCntlRegData = 1;	/* reset the value */

	return sprintf(strbuf, "%s,%d,%d,%d\n",
		(bSuccess ? "OK" : "NG"), (bSuccess ? 1 : 0), 0, 0);
}


static ssize_t magnetic_get_selftest_akm(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	s8 iResult[4] = {-1, -1, -1, -1};
	char bufSelftest[28] = {0, };
	char bufAdc[4] = {0, };
	s16 iSF_X = 0, iSF_Y = 0, iSF_Z = 0;
	s16 iADC_X = 0, iADC_Y = 0, iADC_Z = 0;
	s32 dMsDelay = 20;
	int ret = 0, iSpecOutRetries = 0, i = 0;
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg;

	pr_info("[SSP] %s in\n", __func__);

	/* STATUS */
	if ((data->uFuseRomData[0] == 0) ||
		(data->uFuseRomData[0] == 0xff) ||
		(data->uFuseRomData[1] == 0) ||
		(data->uFuseRomData[1] == 0xff) ||
		(data->uFuseRomData[2] == 0) ||
		(data->uFuseRomData[2] == 0xff))
		iResult[0] = -1;
	else
		iResult[0] = 0;

Retry_selftest:
	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = GEOMAGNETIC_FACTORY;
	msg->length = 28;
	msg->options = AP2HUB_READ;
	msg->buffer = bufSelftest;
	msg->free_buffer = 0;

	ret = ssp_spi_sync(data, msg, 1000);
	if (ret != SUCCESS) {
		pr_err("[SSP] %s - Magnetic Selftest Timeout!! %d\n",
			__func__, ret);
		goto exit;
	}

	/* read 6bytes data registers */
	iSF_X = (s16)((bufSelftest[13] << 8) + bufSelftest[14]);
	iSF_Y = (s16)((bufSelftest[15] << 8) + bufSelftest[16]);
	iSF_Z = (s16)((bufSelftest[17] << 8) + bufSelftest[18]);

	/* DAC (store Cntl Register value to check power down) */
	iResult[2] = bufSelftest[21];

	iSF_X = (s16)(((iSF_X * data->uFuseRomData[0]) >> 7) + iSF_X);
	iSF_Y = (s16)(((iSF_Y * data->uFuseRomData[1]) >> 7) + iSF_Y);
	iSF_Z = (s16)(((iSF_Z * data->uFuseRomData[2]) >> 7) + iSF_Z);

	pr_info("[SSP] %s: self test x = %d, y = %d, z = %d\n",
		__func__, iSF_X, iSF_Y, iSF_Z);

	if ((iSF_X >= GM_SELFTEST_X_SPEC_MIN)
		&& (iSF_X <= GM_SELFTEST_X_SPEC_MAX))
		pr_info("[SSP] x passed self test, expect -200<=x<=200\n");
	else
		pr_info("[SSP] x failed self test, expect -200<=x<=200\n");
	if ((iSF_Y >= GM_SELFTEST_Y_SPEC_MIN)
		&& (iSF_Y <= GM_SELFTEST_Y_SPEC_MAX))
		pr_info("[SSP] y passed self test, expect -200<=y<=200\n");
	else
		pr_info("[SSP] y failed self test, expect -200<=y<=200\n");
	if ((iSF_Z >= GM_SELFTEST_Z_SPEC_MIN)
		&& (iSF_Z <= GM_SELFTEST_Z_SPEC_MAX))
		pr_info("[SSP] z passed self test, expect -1000<=z<=-200\n");
	else
		pr_info("[SSP] z failed self test, expect -1000<=z<=-200\n");

	/* SELFTEST */
	if ((iSF_X >= GM_SELFTEST_X_SPEC_MIN)
		&& (iSF_X <= GM_SELFTEST_X_SPEC_MAX)
		&& (iSF_Y >= GM_SELFTEST_Y_SPEC_MIN)
		&& (iSF_Y <= GM_SELFTEST_Y_SPEC_MAX)
		&& (iSF_Z >= GM_SELFTEST_Z_SPEC_MIN)
		&& (iSF_Z <= GM_SELFTEST_Z_SPEC_MAX))
		iResult[1] = 0;

	if ((iResult[1] == -1) && (iSpecOutRetries++ < 5)) {
		pr_err("[SSP] %s, selftest spec out. Retry = %d", __func__,
			iSpecOutRetries);
		goto Retry_selftest;
	}

	for(i = 0; i < 3; i++) {
		if(bufSelftest[22 + (i * 2)] == 1 && bufSelftest[23 + (i * 2)] == 0)
			continue;
		iResult[1] = -1;
		pr_info("[SSP] continuos selftest fail #%d:%d #%d:%d", 
				22 + (i * 2), bufSelftest[22 + (i * 2)],
				23 + (i * 2), bufSelftest[23 + (i * 2)]);
	}
	iSpecOutRetries = 10;

	/* ADC */
	memcpy(&bufAdc[0], &dMsDelay, 4);

	data->buf[GEOMAGNETIC_RAW].x = 0;
	data->buf[GEOMAGNETIC_RAW].y = 0;
	data->buf[GEOMAGNETIC_RAW].z = 0;

	if (!(atomic64_read(&data->aSensorEnable) & (1 << GEOMAGNETIC_RAW)))
		send_instruction(data, ADD_SENSOR, GEOMAGNETIC_RAW,
			bufAdc, 4);

	do {
		msleep(60);
		if (check_data_spec(data, GEOMAGNETIC_RAW) == SUCCESS)
			break;
	} while (--iSpecOutRetries);

	if (iSpecOutRetries > 0)
		iResult[3] = 0;

	iADC_X = data->buf[GEOMAGNETIC_RAW].x;
	iADC_Y = data->buf[GEOMAGNETIC_RAW].y;
	iADC_Z = data->buf[GEOMAGNETIC_RAW].z;

	if (!(atomic64_read(&data->aSensorEnable) & (1 << GEOMAGNETIC_RAW)))
		send_instruction(data, REMOVE_SENSOR, GEOMAGNETIC_RAW,
			bufAdc, 4);

	pr_info("[SSP] %s -adc, x = %d, y = %d, z = %d, retry = %d\n",
		__func__, iADC_X, iADC_Y, iADC_Z, iSpecOutRetries);

exit:
	pr_info("[SSP] %s out. Result = %d %d %d %d\n",
		__func__, iResult[0], iResult[1], iResult[2], iResult[3]);

	return sprintf(buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		iResult[0], iResult[1], iSF_X, iSF_Y, iSF_Z,
		iResult[2], iResult[3], iADC_X, iADC_Y, iADC_Z);
}

static int set_pdc_matrix(struct ssp_data *data)
{
	int ret = 0;
	struct ssp_msg *msg;

	if (!(data->uSensorState & 0x04)) {
		pr_info("[SSP] %s - Skip this function!!!, magnetic sensor is not connected(0x%llx)\n",
			__func__, data->uSensorState);
		return ret;
	}

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_AP_SET_MAGNETIC_STATIC_MATRIX;
	msg->length = sizeof(data->pdc_matrix);
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(sizeof(data->pdc_matrix), GFP_KERNEL);
	msg->free_buffer = 1;
	memcpy(msg->buffer, data->pdc_matrix, sizeof(data->pdc_matrix));

	ret = ssp_spi_async(data, msg);
	if (ret != SUCCESS) {
		pr_err("[SSP] %s - i2c fail %d\n", __func__, ret);
		ret = ERROR;
	}

	pr_info("[SSP] %s: finished\n", __func__);

	return ret;
}

int get_fuserom_data(struct ssp_data *data)
{
	int ret = 0;
	char buffer[3] = { 0, };
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	msg->cmd = MSG2SSP_AP_FUSEROM;
	msg->length = 3;
	msg->options = AP2HUB_READ;
	msg->buffer = buffer;
	msg->free_buffer = 0;

	ret = ssp_spi_sync(data, msg, 1000);

	if (ret) {
		data->uFuseRomData[0] = buffer[0];
		data->uFuseRomData[1] = buffer[1];
		data->uFuseRomData[2] = buffer[2];
	} else {
		data->uFuseRomData[0] = 0;
		data->uFuseRomData[1] = 0;
		data->uFuseRomData[2] = 0;
		return FAIL;
	}

	pr_info("[SSP] FUSE ROM Data %d , %d, %d\n", data->uFuseRomData[0],
			data->uFuseRomData[1], data->uFuseRomData[2]);

	return SUCCESS;
}

static ssize_t magnetic_logging_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char buffer[21] = {0, };
	int ret = 0;
	int logging_data[8] = {0, };
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg;

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_AP_GEOMAG_LOGGING;
	msg->length = 21;
	msg->options = AP2HUB_READ;
	msg->buffer = buffer;
	msg->free_buffer = 0;

	ret = ssp_spi_sync(data, msg, 1000);
	if (ret != SUCCESS) {
		pr_err("[SSP] %s - Magnetic logging data Timeout!! %d\n",
			__func__, ret);
		goto exit;
	}

	logging_data[0] = buffer[0];	/* ST1 Reg */
	logging_data[1] = (short)((buffer[3] << 8) + buffer[2]);
	logging_data[2] = (short)((buffer[5] << 8) + buffer[4]);
	logging_data[3] = (short)((buffer[7] << 8) + buffer[6]);
	logging_data[4] = buffer[1];	/* ST2 Reg */
	logging_data[5] = (short)((buffer[9] << 8) + buffer[8]);
	logging_data[6] = (short)((buffer[11] << 8) + buffer[10]);
	logging_data[7] = (short)((buffer[13] << 8) + buffer[12]);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			logging_data[0], logging_data[1],
			logging_data[2], logging_data[3],
			logging_data[4], logging_data[5],
			logging_data[6], logging_data[7],
			data->uFuseRomData[0], data->uFuseRomData[1],
			data->uFuseRomData[2]);
exit:
	return snprintf(buf, PAGE_SIZE, "-1,0,0,0,0,0,0,0,0,0,0\n");
}

/* YAS function*/
static ssize_t magnetic_get_selftest_yas(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char chTempBuf[24] = { 0,  };
	int iRet = 0;
	s8 id = 0, x = 0, y1 = 0, y2 = 0, dir = 0;
	s16 sx = 0, sy1 = 0, sy2 = 0, ohx = 0, ohy = 0, ohz = 0;
	s8 err[7] = {-1, };
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	msg->cmd = GEOMAGNETIC_FACTORY;
	msg->length = 24;
	msg->options = AP2HUB_READ;
	msg->buffer = chTempBuf;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - Magnetic Selftest Timeout!! %d\n", __func__, iRet);
		goto exit;
	}

	id = (s8)(chTempBuf[0]);
	err[0] = (s8)(chTempBuf[1]);
	err[1] = (s8)(chTempBuf[2]);
	err[2] = (s8)(chTempBuf[3]);
	x = (s8)(chTempBuf[4]);
	y1 = (s8)(chTempBuf[5]);
	y2 = (s8)(chTempBuf[6]);
	err[3] = (s8)(chTempBuf[7]);
	dir = (s8)(chTempBuf[8]);
	err[4] = (s8)(chTempBuf[9]);
	ohx = (s16)((chTempBuf[10] << 8) + chTempBuf[11]);
	ohy = (s16)((chTempBuf[12] << 8) + chTempBuf[13]);
	ohz = (s16)((chTempBuf[14] << 8) + chTempBuf[15]);
	err[6] = (s8)(chTempBuf[16]);
	sx = (s16)((chTempBuf[17] << 8) + chTempBuf[18]);
	sy1 = (s16)((chTempBuf[19] << 8) + chTempBuf[20]);
	sy2 = (s16)((chTempBuf[21] << 8) + chTempBuf[22]);
	err[5] = (s8)(chTempBuf[23]);

	if (unlikely(id != 0x8))
		err[0] = -1;
	if (unlikely(x < -30 || x > 30))
		err[3] = -1;
	if (unlikely(y1 < -30 || y1 > 30))
		err[3] = -1;
	if (unlikely(y2 < -30 || y2 > 30))
		err[3] = -1;
	if (unlikely(sx < 16544 || sx > 17024))
		err[5] = -1;
	if (unlikely(sy1 < 16517 || sy1 > 17184))
		err[5] = -1;
	if (unlikely(sy2 < 15584 || sy2 > 16251))
		err[5] = -1;
	if (unlikely(ohx < -1000 || ohx > 1000))
		err[6] = -1;
	if (unlikely(ohy < -1000 || ohy > 1000))
		err[6] = -1;
	if (unlikely(ohz < -1000 || ohz > 1000))
		err[6] = -1;

	pr_info("[SSP] %s\n"
		"[SSP] Test1 - err = %d, id = %d\n"
		"[SSP] Test3 - err = %d\n"
		"[SSP] Test4 - err = %d, offset = %d,%d,%d\n"
		"[SSP] Test5 - err = %d, direction = %d\n"
		"[SSP] Test6 - err = %d, sensitivity = %d,%d,%d\n"
		"[SSP] Test7 - err = %d, offset = %d,%d,%d\n"
		"[SSP] Test2 - err = %d\n",
		__func__, err[0], id, err[2], err[3], x, y1, y2, err[4], dir,
		err[5], sx, sy1, sy2, err[6], ohx, ohy, ohz, err[1]);

exit:
	return sprintf(buf,
			"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			err[0], id, err[2], err[3], x, y1, y2, err[4], dir,
			err[5], sx, sy1, sy2, err[6], ohx, ohy, ohz, err[1]);
}

int mag_open_hwoffset(struct ssp_data *data)
{
	int iRet = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(MAG_HW_OFFSET_FILE_PATH, O_RDONLY, 0);
	if (IS_ERR(cal_filp)) {
		pr_err("[SSP] %s: filp_open failed\n", __func__);
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);

		data->magoffset.x = 0;
		data->magoffset.y = 0;
		data->magoffset.z = 0;

		return iRet;
	}

	iRet = cal_filp->f_op->read(cal_filp, (char *)&data->magoffset,
		3 * sizeof(char), &cal_filp->f_pos);
	if (iRet != 3 * sizeof(char)) {
		pr_err("[SSP] %s: filp_open failed\n", __func__);
		iRet = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	ssp_dbg("[SSP]: %s: %d, %d, %d\n", __func__,
		(s8)data->magoffset.x,
		(s8)data->magoffset.y,
		(s8)data->magoffset.z);

	if ((data->magoffset.x == 0) && (data->magoffset.y == 0)
		&& (data->magoffset.z == 0))
		return ERROR;

	return iRet;
}

int mag_store_hwoffset(struct ssp_data *data)
{
	int iRet = 0;
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;

	if (get_hw_offset(data) < 0) {
		pr_err("[SSP]: %s - get_hw_offset failed\n", __func__);
		return ERROR;
	}
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(MAG_HW_OFFSET_FILE_PATH,
		O_CREAT | O_TRUNC | O_WRONLY, 0660);
	if (IS_ERR(cal_filp)) {
		pr_err("[SSP]: %s - Can't open hw_offset file\n",
			__func__);
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);
		return iRet;
	}
	iRet = cal_filp->f_op->write(cal_filp,
		(char *)&data->magoffset,
		3 * sizeof(char), &cal_filp->f_pos);
	if (iRet != 3 * sizeof(char)) {
		pr_err("[SSP]: %s - Can't write the hw_offset to file\n",
			__func__);
		iRet = -EIO;
	}
	filp_close(cal_filp, current->files);
	set_fs(old_fs);
	return iRet;
}

int set_hw_offset(struct ssp_data *data)
{
	int iRet = 0;
	struct ssp_msg *msg;

	if (!(data->uSensorState & 0x04)) {
		pr_info("[SSP]: %s - Skip this function!!!, magnetic sensor is not connected(0x%llx)\n",
			__func__, data->uSensorState);
		return iRet;
	}

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_AP_SET_MAGNETIC_HWOFFSET;
	msg->length = 3;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(3, GFP_KERNEL);
	msg->free_buffer = 1;

	msg->buffer[0] = data->magoffset.x;
	msg->buffer[1] = data->magoffset.y;
	msg->buffer[2] = data->magoffset.z;

	iRet = ssp_spi_async(data, msg);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
		iRet = ERROR;
	}

	pr_info("[SSP]: %s: x: %d, y: %d, z: %d\n", __func__,
		(s8)data->magoffset.x, (s8)data->magoffset.y, (s8)data->magoffset.z);
	return iRet;
}

int set_static_matrix(struct ssp_data *data)
{
	int iRet = SUCCESS;
	struct ssp_msg *msg;
	s16 static_matrix[9] = YAS_STATIC_ELLIPSOID_MATRIX;

	if (!(data->uSensorState & 0x04)) {
		pr_info("[SSP]: %s - Skip this function!!!, magnetic sensor is not connected(0x%llx)\n",
			__func__, data->uSensorState);
		return iRet;
	}

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_AP_SET_MAGNETIC_STATIC_MATRIX;
	msg->length = 18;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(18, GFP_KERNEL);

	msg->free_buffer = 1;
	if (data->static_matrix == NULL)
		memcpy(msg->buffer, static_matrix, 18);
	else
		memcpy(msg->buffer, data->static_matrix, 18);

	iRet = ssp_spi_async(data, msg);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
		iRet = ERROR;
	}
	pr_info("[SSP]: %s: finished\n", __func__);

	return iRet;
}

int get_hw_offset(struct ssp_data *data)
{
	int iRet = 0;
	char buffer[3] = { 0, };
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	msg->cmd = MSG2SSP_AP_GET_MAGNETIC_HWOFFSET;
	msg->length = 3;
	msg->options = AP2HUB_READ;
	msg->buffer = buffer;
	msg->free_buffer = 0;

	data->magoffset.x = 0;
	data->magoffset.y = 0;
	data->magoffset.z = 0;

	iRet = ssp_spi_sync(data, msg, 1000);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
		iRet = ERROR;
	}

	data->magoffset.x = buffer[0];
	data->magoffset.y = buffer[1];
	data->magoffset.z = buffer[2];

	pr_info("[SSP]: %s: x: %d, y: %d, z: %d\n", __func__,
		(s8)data->magoffset.x,
		(s8)data->magoffset.y,
		(s8)data->magoffset.z);
	return iRet;
}

static ssize_t hw_offset_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	mag_open_hwoffset(data);

	pr_info("[SSP] %s: %d %d %d\n", __func__,
		(s8)data->magoffset.x,
		(s8)data->magoffset.y,
		(s8)data->magoffset.z);

	return sprintf(buf, "%d %d %d\n",
		(s8)data->magoffset.x,
		(s8)data->magoffset.y,
		(s8)data->magoffset.z);
}

static ssize_t matrix_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	if (data->mag_type == MAG_TYPE_AKM) {
		return sprintf(buf,
			"%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
			data->pdc_matrix[0], data->pdc_matrix[1], data->pdc_matrix[2], data->pdc_matrix[3],
			data->pdc_matrix[4], data->pdc_matrix[5], data->pdc_matrix[6], data->pdc_matrix[7],
			data->pdc_matrix[8], data->pdc_matrix[9], data->pdc_matrix[10], data->pdc_matrix[11],
			data->pdc_matrix[12], data->pdc_matrix[13], data->pdc_matrix[14], data->pdc_matrix[15],
			data->pdc_matrix[16], data->pdc_matrix[17], data->pdc_matrix[18], data->pdc_matrix[19],
			data->pdc_matrix[20], data->pdc_matrix[21], data->pdc_matrix[22], data->pdc_matrix[23],
			data->pdc_matrix[24], data->pdc_matrix[25], data->pdc_matrix[26]);
	} else {
		return sprintf(buf, "%d %d %d %d %d %d %d %d %d\n", data->static_matrix[0], data->static_matrix[1],
			data->static_matrix[2], data->static_matrix[3], data->static_matrix[4], data->static_matrix[5],
			data->static_matrix[6], data->static_matrix[7], data->static_matrix[8]);
	}
}

static ssize_t matrix_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int iRet;
	int i;
	char *token;
	char *str;

	str = (char *)buf;

	if (data->mag_type == MAG_TYPE_AKM) {
		u8 val[PDC_SIZE] = {0, };

		for (i = 0; i < PDC_SIZE; i++) {
			token = strsep(&str, " \n");
			if (token == NULL) {
				pr_err("[SSP] %s : too few arguments (%d needed)", __func__, PDC_SIZE);
				return -EINVAL;
			}

			iRet = kstrtou8(token, 10, &val[i]);
			if (iRet < 0) {
				pr_err("[SSP] %s : kstros16 error %d", __func__, iRet);
				return iRet;
			}
		}

		for (i = 0; i < PDC_SIZE; i++)
			data->pdc_matrix[i] = val[i];

		pr_info("[SSP] %s : %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
			__func__, data->pdc_matrix[0], data->pdc_matrix[1], data->pdc_matrix[2], data->pdc_matrix[3],
			data->pdc_matrix[4], data->pdc_matrix[5], data->pdc_matrix[6], data->pdc_matrix[7],
			data->pdc_matrix[8], data->pdc_matrix[9], data->pdc_matrix[10], data->pdc_matrix[11],
			data->pdc_matrix[12], data->pdc_matrix[13], data->pdc_matrix[14], data->pdc_matrix[15],
			data->pdc_matrix[16], data->pdc_matrix[17], data->pdc_matrix[18], data->pdc_matrix[19],
			data->pdc_matrix[20], data->pdc_matrix[21], data->pdc_matrix[22], data->pdc_matrix[23],
			data->pdc_matrix[24], data->pdc_matrix[25], data->pdc_matrix[26]);
		set_pdc_matrix(data);
	} else {
		s16 val[9] = {0, };

		for (i = 0; i < 9; i++) {
			token = strsep(&str, "\n");
			if (token == NULL) {
				pr_err("[SSP] %s : too few arguments (9 needed)", __func__);
				return -EINVAL;
			}

			iRet = kstrtos16(token, 10, &val[i]);
			if (iRet < 0) {
				pr_err("[SSP] %s : kstros16 error %d", __func__, iRet);
				return iRet;
			}
		}

		for (i = 0; i < 9; i++)
			data->static_matrix[i] = val[i];

		pr_info("[SSP] %s : %d %d %d %d %d %d %d %d %d\n", __func__,
			data->static_matrix[0], data->static_matrix[1], data->static_matrix[2],
			data->static_matrix[3], data->static_matrix[4], data->static_matrix[5],
			data->static_matrix[6], data->static_matrix[7], data->static_matrix[8]);

		set_static_matrix(data);
	}

	return size;
}

/* common functions */

static ssize_t magnetic_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	if (data->mag_type == MAG_TYPE_AKM)
		return sprintf(buf, "%s\n", VENDOR_AKM);
	else
		return sprintf(buf, "%s\n", VENDOR_YAS);
}

static ssize_t magnetic_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	if (data->mag_type == MAG_TYPE_AKM)
		return sprintf(buf, "%s\n", CHIP_ID_AKM);
	else
		return sprintf(buf, "%s\n", CHIP_ID_YAS);
}

static ssize_t raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	pr_info("[SSP] %s - %d,%d,%d\n", __func__,
		data->buf[GEOMAGNETIC_RAW].x,
		data->buf[GEOMAGNETIC_RAW].y,
		data->buf[GEOMAGNETIC_RAW].z);

	if (data->bGeomagneticRawEnabled == false) {
		data->buf[GEOMAGNETIC_RAW].x = -1;
		data->buf[GEOMAGNETIC_RAW].y = -1;
		data->buf[GEOMAGNETIC_RAW].z = -1;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		data->buf[GEOMAGNETIC_RAW].x,
		data->buf[GEOMAGNETIC_RAW].y,
		data->buf[GEOMAGNETIC_RAW].z);
}


/* Get magnetic cal data from a file */
int load_magnetic_cal_param_from_nvm(u8 *data, u8 length)
{
	int iRet = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(MAG_CAL_PARAM_FILE_PATH, O_CREAT | O_RDONLY | O_NOFOLLOW | O_NONBLOCK, 0660);
	if (IS_ERR(cal_filp)) {
		pr_err("[SSP] %s: filp_open failed, errno = %d\n", __func__, PTR_ERR(cal_filp));
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);

		return iRet;
	}

	cal_filp->f_pos = 3 * sizeof(int); // gyro_cal : 12 bytes

	iRet = vfs_read(cal_filp, (char *)data, length * sizeof(char), &cal_filp->f_pos);

	if (iRet != length * sizeof(char)) {
		pr_err("[SSP] %s: filp_open read failed, read size = %d", __func__, iRet);
		iRet = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return iRet;
}


int set_magnetic_cal_param_to_ssp(struct ssp_data *data)
{
	int iRet = 0;
	u8 mag_caldata_akm[MAC_CAL_PARAM_SIZE_AKM] = {0, };
	u8 mag_caldata_yas[MAC_CAL_PARAM_SIZE_YAS] = {0, };
	u16 size = 0;

	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n", __func__);
		return iRet;
	}

	if (data->mag_type == MAG_TYPE_AKM) {
		size = MAC_CAL_PARAM_SIZE_AKM;
		load_magnetic_cal_param_from_nvm(mag_caldata_akm, MAC_CAL_PARAM_SIZE_AKM);
		msg->cmd = MSG2SSP_AP_MAG_CAL_PARAM;
		msg->length = MAC_CAL_PARAM_SIZE_AKM;
		msg->options = AP2HUB_WRITE;
		msg->buffer = kzalloc(MAC_CAL_PARAM_SIZE_AKM, GFP_KERNEL);
		msg->free_buffer = 1;
		memcpy(msg->buffer, mag_caldata_akm, MAC_CAL_PARAM_SIZE_AKM);
		pr_err("[SSP] %s, %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", __func__,
		mag_caldata_akm[0], mag_caldata_akm[1], mag_caldata_akm[2], mag_caldata_akm[3], mag_caldata_akm[4],
		mag_caldata_akm[5], mag_caldata_akm[6], mag_caldata_akm[7], mag_caldata_akm[8], mag_caldata_akm[9],
		mag_caldata_akm[10], mag_caldata_akm[11], mag_caldata_akm[12]);
	} else {
		size = MAC_CAL_PARAM_SIZE_YAS;
		load_magnetic_cal_param_from_nvm(mag_caldata_yas, MAC_CAL_PARAM_SIZE_YAS);
		msg->cmd = MSG2SSP_AP_MAG_CAL_PARAM;
		msg->length = MAC_CAL_PARAM_SIZE_YAS;
		msg->options = AP2HUB_WRITE;
		msg->buffer = kzalloc(MAC_CAL_PARAM_SIZE_YAS, GFP_KERNEL);
		msg->free_buffer = 1;
		memcpy(msg->buffer, mag_caldata_yas, MAC_CAL_PARAM_SIZE_YAS);
		pr_err("[SSP] %s,\n", __func__);
	}

	iRet = ssp_spi_async(data, msg);
	if (iRet != SUCCESS) {
		pr_err("[SSP] %s -fail to set. %d\n", __func__, iRet);
		iRet = ERROR;
	}

	return iRet;
}


int save_magnetic_cal_param_to_nvm(struct ssp_data *data, char *pchRcvDataFrame, int *iDataIdx)
{
	int iRet = 0;
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;
	int i = 0;
	int length = 0;
	u8 mag_caldata_akm[MAC_CAL_PARAM_SIZE_AKM] = {0, }; //AKM uses 13 byte.
	u8 mag_caldata_yas[MAC_CAL_PARAM_SIZE_YAS] = {0, }; // YAMAHA uses 7 byte.
	u8 gyro_mag_cal[25] = {0,};

	if (data->mag_type == MAG_TYPE_AKM) {
		//AKM uses 13 byte. YAMAHA uses 7 byte.
		memcpy(mag_caldata_akm, pchRcvDataFrame + *iDataIdx, sizeof(mag_caldata_akm));
		length = sizeof(mag_caldata_akm);
	} else {
		//AKM uses 13 byte. YAMAHA uses 7 byte.
		memcpy(mag_caldata_yas, pchRcvDataFrame + *iDataIdx, sizeof(mag_caldata_yas));
		length = sizeof(mag_caldata_yas);
	}

	ssp_dbg("[SSP]: %s\n", __func__);
	for (i = 0; i < length; i++) {
		if (data->mag_type == MAG_TYPE_AKM)
			ssp_dbg("[SSP] mag cal param[%d] %d\n", i, mag_caldata_akm[i]);
		else
			ssp_dbg("[SSP] mag cal param[%d] %d\n", i, mag_caldata_yas[i]);
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(MAG_CAL_PARAM_FILE_PATH, O_CREAT | O_RDWR | O_NOFOLLOW | O_NONBLOCK, 0660);
	if (IS_ERR(cal_filp)) {
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);
		pr_err("[SSP]: %s - Can't open mag cal file err(%d)\n", __func__, iRet);
		return -EIO;
	}

	cal_filp->f_pos = 12; // gyro_cal : 12 bytes

	if (data->mag_type == MAG_TYPE_AKM)
		iRet = vfs_write(cal_filp, (char *)mag_caldata_akm, length * sizeof(char), &cal_filp->f_pos);
	else
		iRet = vfs_write(cal_filp, (char *)mag_caldata_yas, length * sizeof(char), &cal_filp->f_pos);

	if (iRet != length * sizeof(char)) {
		pr_err("[SSP]: %s - Can't write mag cal to file\n", __func__);
		iRet = -EIO;
	}

	cal_filp->f_pos = 0;
	iRet = vfs_read(cal_filp, (char *)gyro_mag_cal, 25, &cal_filp->f_pos);

	pr_err("[SSP]: %s, gyro_cal= %d %d %d %d %d %d %d %d %d %d %d %d", __func__,
			gyro_mag_cal[0], gyro_mag_cal[1], gyro_mag_cal[2], gyro_mag_cal[3],
			gyro_mag_cal[4], gyro_mag_cal[5], gyro_mag_cal[6], gyro_mag_cal[7],
			gyro_mag_cal[8], gyro_mag_cal[9], gyro_mag_cal[10], gyro_mag_cal[11]);
	pr_err("[SSP]: %s, mag_cal= %d %d %d %d %d %d %d %d %d %d %d %d %d", __func__,
			gyro_mag_cal[12], gyro_mag_cal[13], gyro_mag_cal[14], gyro_mag_cal[15],
			gyro_mag_cal[16], gyro_mag_cal[17], gyro_mag_cal[18], gyro_mag_cal[19],
			gyro_mag_cal[20], gyro_mag_cal[21], gyro_mag_cal[22], gyro_mag_cal[23], gyro_mag_cal[24]);

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	*iDataIdx += length;

	return iRet;
}


static ssize_t raw_data_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	char chTempbuf[4] = { 0 };
	int ret;
	int64_t dEnable;
	int retries = 50;
	struct ssp_data *data = dev_get_drvdata(dev);
	s32 dMsDelay = 20;

	memcpy(&chTempbuf[0], &dMsDelay, 4);

	ret = kstrtoll(buf, 10, &dEnable);
	if (ret < 0)
		return ret;

	if (dEnable) {
		data->buf[GEOMAGNETIC_RAW].x = 0;
		data->buf[GEOMAGNETIC_RAW].y = 0;
		data->buf[GEOMAGNETIC_RAW].z = 0;

		send_instruction(data, ADD_SENSOR, GEOMAGNETIC_RAW,
			chTempbuf, 4);

		do {
			msleep(20);
			if (check_data_spec(data, GEOMAGNETIC_RAW) == SUCCESS)
				break;
		} while (--retries);

		if (retries > 0) {
			pr_info("[SSP] %s - success, %d\n", __func__, retries);
			data->bGeomagneticRawEnabled = true;
		} else {
			pr_err("[SSP] %s - wait timeout, %d\n", __func__,
				retries);
			data->bGeomagneticRawEnabled = false;
		}


	} else {
		send_instruction(data, REMOVE_SENSOR, GEOMAGNETIC_RAW,
			chTempbuf, 4);
		data->bGeomagneticRawEnabled = false;
	}

	return size;
}

static ssize_t adc_data_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	bool bSuccess = false;
	u8 chTempbuf[4] = { 0 };
	s16 iSensorBuf[3] = {0, };
	int retries = 10;
	struct ssp_data *data = dev_get_drvdata(dev);
	s32 dMsDelay = 20;

	memcpy(&chTempbuf[0], &dMsDelay, 4);

	data->buf[GEOMAGNETIC_SENSOR].x = 0;
	data->buf[GEOMAGNETIC_SENSOR].y = 0;
	data->buf[GEOMAGNETIC_SENSOR].z = 0;

	if (!(atomic64_read(&data->aSensorEnable) & (1 << GEOMAGNETIC_SENSOR)))
		send_instruction(data, ADD_SENSOR, GEOMAGNETIC_SENSOR,
			chTempbuf, 4);

	do {
		msleep(60);
		if (check_data_spec(data, GEOMAGNETIC_SENSOR) == SUCCESS)
			break;
	} while (--retries);

	if (retries > 0)
		bSuccess = true;

	iSensorBuf[0] = data->buf[GEOMAGNETIC_SENSOR].x;
	iSensorBuf[1] = data->buf[GEOMAGNETIC_SENSOR].y;
	iSensorBuf[2] = data->buf[GEOMAGNETIC_SENSOR].z;

	if (!(atomic64_read(&data->aSensorEnable) & (1 << GEOMAGNETIC_SENSOR)))
		send_instruction(data, REMOVE_SENSOR, GEOMAGNETIC_SENSOR,
			chTempbuf, 4);

	pr_info("[SSP] %s - x = %d, y = %d, z = %d\n", __func__,
		iSensorBuf[0], iSensorBuf[1], iSensorBuf[2]);

	return sprintf(buf, "%s,%d,%d,%d\n", (bSuccess ? "OK" : "NG"),
		iSensorBuf[0], iSensorBuf[1], iSensorBuf[2]);
}

static ssize_t magnetic_get_selftest(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	if (data->mag_type == MAG_TYPE_AKM)
		return magnetic_get_selftest_akm(dev, attr, buf);
	else
		return magnetic_get_selftest_yas(dev, attr, buf);
}
static ssize_t si_param_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	if (data->mag_type == MAG_TYPE_AKM) {
		return sprintf(buf, "\"SI_PARAMETER\":\"%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\"\n",
			data->pdc_matrix[0], data->pdc_matrix[1], data->pdc_matrix[2], data->pdc_matrix[3],
			data->pdc_matrix[4], data->pdc_matrix[5], data->pdc_matrix[6], data->pdc_matrix[7],
			data->pdc_matrix[8], data->pdc_matrix[9], data->pdc_matrix[10], data->pdc_matrix[11],
			data->pdc_matrix[12], data->pdc_matrix[13], data->pdc_matrix[14], data->pdc_matrix[15],
			data->pdc_matrix[16], data->pdc_matrix[17], data->pdc_matrix[18], data->pdc_matrix[19],
			data->pdc_matrix[20], data->pdc_matrix[21], data->pdc_matrix[22], data->pdc_matrix[23],
			data->pdc_matrix[24], data->pdc_matrix[25], data->pdc_matrix[26]);
	} else {
		return sprintf(buf, "\"SI_PARAMETER\":\"%d %d %d %d %d %d %d %d %d\"\n"
		, data->static_matrix[0], data->static_matrix[1], data->static_matrix[2]
		, data->static_matrix[3], data->static_matrix[4], data->static_matrix[5]
		, data->static_matrix[6], data->static_matrix[7], data->static_matrix[8]);
	}
}

static DEVICE_ATTR(name, 0440, magnetic_name_show, NULL);
static DEVICE_ATTR(vendor, 0440, magnetic_vendor_show, NULL);
static DEVICE_ATTR(raw_data, 0660,
		raw_data_show, raw_data_store);
static DEVICE_ATTR(adc, 0440, adc_data_read, NULL);
static DEVICE_ATTR(selftest, 0440, magnetic_get_selftest, NULL);

static DEVICE_ATTR(status, 0440,  magnetic_get_status, NULL);
static DEVICE_ATTR(dac, 0440, magnetic_check_cntl, NULL);
static DEVICE_ATTR(ak09911_asa, 0440, magnetic_get_asa, NULL);
static DEVICE_ATTR(logging_data, 0440, magnetic_logging_show, NULL);

static DEVICE_ATTR(hw_offset, 0440, hw_offset_show, NULL);
static DEVICE_ATTR(matrix, 0660, matrix_show, matrix_store);
static DEVICE_ATTR(dhr_sensor_info, 0440, si_param_show, NULL);

static struct device_attribute *mag_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_adc,
	&dev_attr_dac,
	&dev_attr_raw_data,
	&dev_attr_selftest,
	&dev_attr_status,
	&dev_attr_ak09911_asa,
	&dev_attr_logging_data,
	&dev_attr_hw_offset,
	&dev_attr_matrix,
	&dev_attr_dhr_sensor_info,
	NULL,
};

int initialize_magnetic_sensor(struct ssp_data *data)
{
	int ret;

	if (data->mag_type == MAG_TYPE_AKM) {
		ret = get_fuserom_data(data);
		if (ret < 0)
			pr_err("[SSP] %s - get_fuserom_data failed %d\n",
				__func__, ret);

		ret = set_pdc_matrix(data);
		if (ret < 0)
			pr_err("[SSP] %s - set_magnetic_pdc_matrix failed %d\n",
				__func__, ret);
	} else {
		ret = set_static_matrix(data);
		if (ret < 0)
			pr_err("[SSP]: %s - set_magnetic_static_matrix failed %d\n",
				__func__, ret);
	}
	ret = set_magnetic_cal_param_to_ssp(data);
	if (ret < 0)
		pr_err("[SSP]: %s - set_magnetic_static_matrix failed %d\n", __func__, ret);

	return ret < 0 ? ret : SUCCESS;
}

void initialize_magnetic_factorytest(struct ssp_data *data)
{
	sensors_register(data->mag_device, data, mag_attrs, "magnetic_sensor");
}

void remove_magnetic_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->mag_device, mag_attrs);
}
