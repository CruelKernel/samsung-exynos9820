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
#include "../ssp.h"
#include "magnetic_yas539.h"

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

#define VENDOR		"YAMAHA"
#define CHIP_ID		"YAS539"
#define MAG_HW_OFFSET_FILE_PATH	"/efs/FactoryApp/hw_offset"
#define MAG_CAL_PARAM_FILE_PATH	"/efs/FactoryApp/mag_cal_param"

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

static int check_rawdata_spec(struct ssp_data *data)
{
	if ((data->buf[GEOMAGNETIC_RAW].x == 0) &&
		(data->buf[GEOMAGNETIC_RAW].y == 0) &&
		(data->buf[GEOMAGNETIC_RAW].z == 0))
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
	int iRet;
	int64_t dEnable;
	int iRetries = 50;
	struct ssp_data *data = dev_get_drvdata(dev);
	s32 dMsDelay = 20;

	memcpy(&chTempbuf[0], &dMsDelay, 4);

	iRet = kstrtoll(buf, 10, &dEnable);
	if (iRet < 0)
		return iRet;

	if (dEnable) {
		data->buf[GEOMAGNETIC_RAW].x = 0;
		data->buf[GEOMAGNETIC_RAW].y = 0;
		data->buf[GEOMAGNETIC_RAW].z = 0;

		send_instruction(data, ADD_SENSOR, GEOMAGNETIC_RAW,
			chTempbuf, 4);

		do {
			msleep(20);
			if (check_rawdata_spec(data) == SUCCESS)
				break;
		} while (--iRetries);

		if (iRetries > 0) {
			pr_info("[SSP] %s - success, %d\n", __func__, iRetries);
			data->bGeomagneticRawEnabled = true;
		} else {
			pr_err("[SSP] %s - wait timeout, %d\n", __func__,
				iRetries);
			data->bGeomagneticRawEnabled = false;
		}
	} else {
		send_instruction(data, REMOVE_SENSOR, GEOMAGNETIC_RAW,
			chTempbuf, 4);
		data->bGeomagneticRawEnabled = false;
	}

	return size;
}

static int check_data_spec(struct ssp_data *data)
{
	if ((data->buf[GEOMAGNETIC_SENSOR].x == 0) &&
		(data->buf[GEOMAGNETIC_SENSOR].y == 0) &&
		(data->buf[GEOMAGNETIC_SENSOR].z == 0))
		return FAIL;
	else if ((data->buf[GEOMAGNETIC_SENSOR].x > 6500) ||
		(data->buf[GEOMAGNETIC_SENSOR].x < -6500) ||
		(data->buf[GEOMAGNETIC_SENSOR].y > 6500) ||
		(data->buf[GEOMAGNETIC_SENSOR].y < -6500) ||
		(data->buf[GEOMAGNETIC_SENSOR].z > 6500) ||
		(data->buf[GEOMAGNETIC_SENSOR].z < -6500))
		return FAIL;
	else
		return SUCCESS;
}

static ssize_t adc_data_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	bool bSuccess = false;
	u8 chTempbuf[4] = { 0 };
	s16 iSensorBuf[3] = {0, };
	int iRetries = 10;
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
		if (check_data_spec(data) == SUCCESS)
			break;
	} while (--iRetries);

	if (iRetries > 0)
		bSuccess = true;

	iSensorBuf[0] = data->buf[GEOMAGNETIC_SENSOR].x;
	iSensorBuf[1] = data->buf[GEOMAGNETIC_SENSOR].y;
	iSensorBuf[2] = data->buf[GEOMAGNETIC_SENSOR].z;

	if (!(atomic64_read(&data->aSensorEnable) & (1 << GEOMAGNETIC_SENSOR)))
		send_instruction(data, REMOVE_SENSOR, GEOMAGNETIC_SENSOR,
			chTempbuf, 4);

	pr_info("[SSP]: %s - x = %d, y = %d, z = %d\n", __func__,
		iSensorBuf[0], iSensorBuf[1], iSensorBuf[2]);

	return sprintf(buf, "%s,%d,%d,%d\n", (bSuccess ? "OK" : "NG"),
		iSensorBuf[0], iSensorBuf[1], iSensorBuf[2]);
}

static ssize_t magnetic_get_selftest(struct device *dev,
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

	return sprintf(buf,
			"%d %d %d %d %d %d %d %d %d\n",
			data->static_matrix[0], data->static_matrix[1], data->static_matrix[2],
			data->static_matrix[3], data->static_matrix[4], data->static_matrix[5],
			data->static_matrix[6], data->static_matrix[7], data->static_matrix[8]);
}

static ssize_t matrix_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int iRet;
	int i;
	s16 val[9] = {0, };
	char *token;
	char *str;

	str = (char *)buf;

	for (i = 0; i < 9; i++) {
		token = strsep(&str, " \n");
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

	return size;
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

	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n", __func__);
		return iRet;
	}

	size = MAC_CAL_PARAM_SIZE_YAS;
	load_magnetic_cal_param_from_nvm(mag_caldata_yas, MAC_CAL_PARAM_SIZE_YAS);
	msg->cmd = MSG2SSP_AP_MAG_CAL_PARAM;
	msg->length = MAC_CAL_PARAM_SIZE_YAS;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(MAC_CAL_PARAM_SIZE_YAS, GFP_KERNEL);
	msg->free_buffer = 1;
	memcpy(msg->buffer, mag_caldata_yas, MAC_CAL_PARAM_SIZE_YAS);
	pr_err("[SSP] %s,\n", __func__);

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
	u8 mag_caldata_yas[MAC_CAL_PARAM_SIZE_YAS] = {0, }; // YAMAHA uses 7 byte.

	if (data->mag_type == MAG_TYPE_YAS) {
		//AKM uses 13 byte. YAMAHA uses 7 byte.
		memcpy(mag_caldata_yas, pchRcvDataFrame + *iDataIdx, sizeof(mag_caldata_yas));
		length = sizeof(mag_caldata_yas);
	} else {
		pr_err("[SSP] %s invalid mag type %d", __func__, data->mag_type);
		return -EINVAL;
	}

	ssp_dbg("[SSP]: %s\n", __func__);
	for (i = 0; i < length; i++)
		ssp_dbg("[SSP] mag cal param[%d] %d\n", i, mag_caldata_yas[i]);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(MAG_CAL_PARAM_FILE_PATH, O_CREAT | O_TRUNC | O_WRONLY | O_NOFOLLOW | O_NONBLOCK, 0660);
	if (IS_ERR(cal_filp)) {
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);
		pr_err("[SSP]: %s - Can't open mag cal file err(%d)\n", __func__, iRet);
		return -EIO;
	}

	iRet = vfs_write(cal_filp, (char *)mag_caldata_yas, length * sizeof(char), &cal_filp->f_pos);
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
static DEVICE_ATTR(hw_offset, 0444, hw_offset_show, NULL);
static DEVICE_ATTR(matrix, 0664, matrix_show, matrix_store);

static struct device_attribute *mag_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_adc,
	&dev_attr_raw_data,
	&dev_attr_selftest,
	&dev_attr_hw_offset,
	&dev_attr_matrix,
	NULL,
};

int initialize_magnetic_sensor(struct ssp_data *data)
{
	int ret;

	ret = set_static_matrix(data);
	if (ret < 0)
		pr_err("[SSP]: %s - set_magnetic_static_matrix failed %d\n",
			__func__, ret);

	ret = set_magnetic_cal_param_to_ssp(data);
	if (ret < 0)
		pr_err("[SSP]: %s - set_magnetic_static_matrix failed %d\n", __func__, ret);

	return ret;
}

void initialize_magnetic_factorytest(struct ssp_data *data)
{
	sensors_register(data->mag_device, data, mag_attrs, "magnetic_sensor");
}

void remove_magnetic_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->mag_device, mag_attrs);
}
