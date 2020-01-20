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

#define VENDOR			"AKM"
#define CHIP_ID			"AK09911C"

#define GM_DATA_SPEC_MIN	-1600
#define GM_DATA_SPEC_MAX	1600

#define GM_SELFTEST_X_SPEC_MIN	-30
#define GM_SELFTEST_X_SPEC_MAX	30
#define GM_SELFTEST_Y_SPEC_MIN	-30
#define GM_SELFTEST_Y_SPEC_MAX	30
#define GM_SELFTEST_Z_SPEC_MIN	-400
#define GM_SELFTEST_Z_SPEC_MAX	-50

#define MAG_CAL_PARAM_FILE_PATH	"/efs/FactoryApp/mag_cal_param"

static ssize_t magnetic_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t magnetic_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static int check_data_spec(struct ssp_data *data, int sensortype)
{
	if ((data->buf[sensortype].x == 0) &&
		(data->buf[sensortype].y == 0) &&
		(data->buf[sensortype].z == 0))
		return FAIL;
	else if ((data->buf[sensortype].x > GM_DATA_SPEC_MAX)
		|| (data->buf[sensortype].x < GM_DATA_SPEC_MIN)
		|| (data->buf[sensortype].y > GM_DATA_SPEC_MAX)
		|| (data->buf[sensortype].y < GM_DATA_SPEC_MIN)
		|| (data->buf[sensortype].z > GM_DATA_SPEC_MAX)
		|| (data->buf[sensortype].z < GM_DATA_SPEC_MIN))
		return FAIL;
	else
		return SUCCESS;
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
	} else {
		if (data->buf[GEOMAGNETIC_RAW].x > 18000)
			data->buf[GEOMAGNETIC_RAW].x = 18000;
		else if (data->buf[GEOMAGNETIC_RAW].x < -18000)
			data->buf[GEOMAGNETIC_RAW].x = -18000;
		if (data->buf[GEOMAGNETIC_RAW].y > 18000)
			data->buf[GEOMAGNETIC_RAW].y = 18000;
		else if (data->buf[GEOMAGNETIC_RAW].y < -18000)
			data->buf[GEOMAGNETIC_RAW].y = -18000;
		if (data->buf[GEOMAGNETIC_RAW].z > 18000)
			data->buf[GEOMAGNETIC_RAW].z = 18000;
		else if (data->buf[GEOMAGNETIC_RAW].z < -18000)
			data->buf[GEOMAGNETIC_RAW].z = -18000;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		data->buf[GEOMAGNETIC_RAW].x,
		data->buf[GEOMAGNETIC_RAW].y,
		data->buf[GEOMAGNETIC_RAW].z);
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

		if (retries > 0)
			pr_info("[SSP] %s - success, %d\n", __func__, retries);
		else
			pr_err("[SSP] %s - wait timeout, %d\n", __func__,
				retries);

		data->bGeomagneticRawEnabled = true;
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
		if (msg == NULL) {
			pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n",
				__func__);
			return -ENOMEM;
		}
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

static ssize_t magnetic_get_selftest(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	s8 iResult[4] = {-1, -1, -1, -1};
	char bufSelftset[22] = {0, };
	char bufAdc[4] = {0, };
	s16 iSF_X = 0, iSF_Y = 0, iSF_Z = 0;
	s16 iADC_X = 0, iADC_Y = 0, iADC_Z = 0;
	s32 dMsDelay = 20;
	int ret = 0, iSpecOutRetries = 0;
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
	msg->length = 22;
	msg->options = AP2HUB_READ;
	msg->buffer = bufSelftset;
	msg->free_buffer = 0;

	ret = ssp_spi_sync(data, msg, 1000);
	if (ret != SUCCESS) {
		pr_err("[SSP] %s - Magnetic Selftest Timeout!! %d\n",
			__func__, ret);
		goto exit;
	}

	/* read 6bytes data registers */
	iSF_X = (s16)((bufSelftset[13] << 8) + bufSelftset[14]);
	iSF_Y = (s16)((bufSelftset[15] << 8) + bufSelftset[16]);
	iSF_Z = (s16)((bufSelftset[17] << 8) + bufSelftset[18]);

	/* DAC (store Cntl Register value to check power down) */
	iResult[2] = bufSelftset[21];

	iSF_X = (s16)(((iSF_X * data->uFuseRomData[0]) >> 7) + iSF_X);
	iSF_Y = (s16)(((iSF_Y * data->uFuseRomData[1]) >> 7) + iSF_Y);
	iSF_Z = (s16)(((iSF_Z * data->uFuseRomData[2]) >> 7) + iSF_Z);

	pr_info("[SSP] %s: self test x = %d, y = %d, z = %d\n",
		__func__, iSF_X, iSF_Y, iSF_Z);

	if ((iSF_X >= GM_SELFTEST_X_SPEC_MIN)
		&& (iSF_X <= GM_SELFTEST_X_SPEC_MAX))
		pr_info("[SSP] x passed self test, expect -30<=x<=30\n");
	else
		pr_info("[SSP] x failed self test, expect -30<=x<=30\n");
	if ((iSF_Y >= GM_SELFTEST_Y_SPEC_MIN)
		&& (iSF_Y <= GM_SELFTEST_Y_SPEC_MAX))
		pr_info("[SSP] y passed self test, expect -30<=y<=30\n");
	else
		pr_info("[SSP] y failed self test, expect -30<=y<=30\n");
	if ((iSF_Z >= GM_SELFTEST_Z_SPEC_MIN)
		&& (iSF_Z <= GM_SELFTEST_Z_SPEC_MAX))
		pr_info("[SSP] z passed self test, expect -400<=z<=-50\n");
	else
		pr_info("[SSP] z failed self test, expect -400<=z<=-50\n");

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

int initialize_magnetic_sensor(struct ssp_data *data)
{
	int ret;

	ret = get_fuserom_data(data);
	if (ret < 0)
		pr_err("[SSP] %s - get_fuserom_data failed %d\n",
			__func__, ret);

	ret = set_pdc_matrix(data);
	if (ret < 0)
		pr_err("[SSP] %s - set_magnetic_pdc_matrix failed %d\n",
			__func__, ret);

	ret = set_magnetic_cal_param_to_ssp(data);
	if (ret < 0)
		pr_err("[SSP]: %s - set_magnetic_static_matrix failed %d\n", __func__, ret);

	return ret < 0 ? ret : SUCCESS;
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

/* Get magnetic cal data from a file */
int load_magnetic_cal_param_from_nvm(u8 *data, u8 length)
{
	int iRet = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(MAG_CAL_PARAM_FILE_PATH, O_RDONLY, 0);
	if (IS_ERR(cal_filp)) {
		pr_err("[SSP] %s: filp_open failed\n", __func__);
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);

		return iRet;
	}

	iRet = vfs_read(cal_filp, (char *)data, length * sizeof(char), &cal_filp->f_pos);
	if (iRet != length * sizeof(char)) {
		pr_err("[SSP] %s: filp_open failed\n", __func__);
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

	if (data->mag_type == MAG_TYPE_AKM) {
		//AKM uses 13 byte. YAMAHA uses 7 byte.
		memcpy(mag_caldata_akm, pchRcvDataFrame + *iDataIdx, sizeof(mag_caldata_akm));
		length = sizeof(mag_caldata_akm);
	} else {
		pr_err("[SSP] %s invalid mag type %d", __func__, data->mag_type);
		return -EINVAL;
	}

	ssp_dbg("[SSP]: %s\n", __func__);
	for (i = 0; i < length; i++)
		ssp_dbg("[SSP] mag cal param[%d] %d\n", i, mag_caldata_akm[i]);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(MAG_CAL_PARAM_FILE_PATH, O_CREAT | O_TRUNC | O_WRONLY | O_NOFOLLOW | O_NONBLOCK, 0660);
	if (IS_ERR(cal_filp)) {
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);
		pr_err("[SSP]: %s - Can't open mag cal file err(%d)\n", __func__, iRet);
		return -EIO;
	}

	iRet = vfs_write(cal_filp, (char *)mag_caldata_akm, length * sizeof(char), &cal_filp->f_pos);
	if (iRet != length * sizeof(char)) {
		pr_err("[SSP]: %s - Can't write mag cal to file\n", __func__);
		iRet = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	*iDataIdx += length;

	return iRet;
}
static DEVICE_ATTR(name, 0444, magnetic_name_show, NULL);
static DEVICE_ATTR(vendor, 0444, magnetic_vendor_show, NULL);
static DEVICE_ATTR(raw_data, 0664,
		raw_data_show, raw_data_store);
static DEVICE_ATTR(adc, 0444, adc_data_read, NULL);
static DEVICE_ATTR(selftest, 0444, magnetic_get_selftest, NULL);
static DEVICE_ATTR(status, 0444,  magnetic_get_status, NULL);
static DEVICE_ATTR(dac, 0444, magnetic_check_cntl, NULL);
static DEVICE_ATTR(ak09911_asa, 0444, magnetic_get_asa, NULL);
static DEVICE_ATTR(logging_data, 0444, magnetic_logging_show, NULL);

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
	NULL,
};

void initialize_magnetic_factorytest(struct ssp_data *data)
{
	sensors_register(data->mag_device, data, mag_attrs, "magnetic_sensor");
}

void remove_magnetic_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->mag_device, mag_attrs);
}
