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
#include <linux/kernel.h>
#include "../ssp.h"

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

#define VENDOR		"BOSCH"
#define CHIP_ID		"BMI058"

#define CALIBRATION_FILE_PATH	"/efs/FactoryApp/gyro_cal_data"
#define CALIBRATION_DATA_AMOUNT            20
#define SELFTEST_DATA_AMOUNT               64
#define SELFTEST_LIMITATION_OF_ERROR       5250
///////////////////////////
#define VERBOSE_OUT 1
#define DEF_GYRO_FULLSCALE	2000
#define DEF_GYRO_SENS	(32768 / DEF_GYRO_FULLSCALE)
#define DEF_BIAS_LSB_THRESH_SELF	(20 * DEF_GYRO_SENS)
#define DEF_BIAS_LSB_THRESH_SELF_6500	(30 * DEF_GYRO_SENS)
#define DEF_RMS_LSB_TH_SELF (5 * DEF_GYRO_SENS)
#define DEF_RMS_THRESH	((DEF_RMS_LSB_TH_SELF) * (DEF_RMS_LSB_TH_SELF))
#define DEF_SCALE_FOR_FLOAT (1000)
#define DEF_RMS_SCALE_FOR_RMS (10000)
#define DEF_SQRT_SCALE_FOR_RMS (100)

static ssize_t gyro_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t gyro_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

int gyro_open_calibration(struct ssp_data *data)
{
	int iRet = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH, O_RDONLY | O_NOFOLLOW | O_NONBLOCK, 0660);
	if (IS_ERR(cal_filp)) {
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);

		data->gyrocal.x = 0;
		data->gyrocal.y = 0;
		data->gyrocal.z = 0;

		return iRet;
	}

	iRet = cal_filp->f_op->read(cal_filp, (char *)&data->gyrocal,
		3 * sizeof(int), &cal_filp->f_pos);
	if (iRet != 3 * sizeof(int))
		iRet = -EIO;

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	ssp_dbg("[SSP]: open gyro calibration %d, %d, %d\n",
		data->gyrocal.x, data->gyrocal.y, data->gyrocal.z);
	return iRet;
}

static int save_gyro_caldata(struct ssp_data *data, s16 *iCalData)
{
	int iRet = 0;
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;

	data->gyrocal.x = iCalData[0]; // << 2;
	data->gyrocal.y = iCalData[1]; // << 2;
	data->gyrocal.z = iCalData[2]; //<< 2;

	ssp_dbg("[SSP]: do gyro calibrate %d, %d, %d\n",
		data->gyrocal.x, data->gyrocal.y, data->gyrocal.z);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY | O_NOFOLLOW | O_NONBLOCK, 0660);
	if (IS_ERR(cal_filp)) {
		pr_err("[SSP]: %s - Can't open calibration file\n", __func__);
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);
		return -EIO;
	}

	iRet = cal_filp->f_op->write(cal_filp, (char *)&data->gyrocal,
		3 * sizeof(int), &cal_filp->f_pos);
	if (iRet != 3 * sizeof(int)) {
		pr_err("[SSP]: %s - Can't write gyro cal to file\n", __func__);
		iRet = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return iRet;
}

int set_gyro_cal(struct ssp_data *data)
{
	int iRet = 0;
	struct ssp_msg *msg;
	s16 gyro_cal[3];

	gyro_cal[0] = data->gyrocal.x;
	gyro_cal[1] = data->gyrocal.y;
	gyro_cal[2] = data->gyrocal.z;

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_AP_MCU_SET_GYRO_CAL;
	msg->length = 6;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(6, GFP_KERNEL);
	if (!(msg->buffer)) {
		kfree(msg);
		return -ENOMEM;
	}
	msg->free_buffer = 1;
	memcpy(msg->buffer, gyro_cal, 6);

	iRet = ssp_spi_async(data, msg);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
		iRet = ERROR;
	}

	pr_info("[SSP] Set gyro cal data %d, %d, %d\n", gyro_cal[0], gyro_cal[1], gyro_cal[2]);
	return iRet;
}

static ssize_t gyro_power_off(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssp_dbg("[SSP]: %s\n", __func__);

	return sprintf(buf, "%d\n", 1);
}

static ssize_t gyro_power_on(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssp_dbg("[SSP]: %s\n", __func__);

	return sprintf(buf, "%d\n", 1);
}

short bmi058_gyro_get_temp(struct ssp_data *data)
{
	char chTempBuf = 0;
	short temperature = 0;
	int iRet = 0;
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	msg->cmd = GYROSCOPE_TEMP_FACTORY;
	msg->length = 1;
	msg->options = AP2HUB_READ;
	msg->buffer = &chTempBuf;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 3000);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - Gyro Temp Timeout!!\n", __func__);
		goto exit;
	}

	temperature = (short)chTempBuf;
	ssp_dbg("[SSP]: %s - %d\n", __func__, temperature);

exit:
	return temperature;
}

static ssize_t gyro_get_temp(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	short temperature = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	temperature = bmi058_gyro_get_temp(data);
	return sprintf(buf, "%d\n", temperature);
}

static ssize_t bmi058_gyro_selftest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char chTempBuf[19] = { 0,};
	u8 bist = 0, selftest = 0;
	int datax_check = 0;
	int datay_check = 0;
	int dataz_check = 0;
	s16 iCalData[3] = {0,};
	int iRet = 0;
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	msg->cmd = GYROSCOPE_FACTORY;
	msg->length = 19;
	msg->options = AP2HUB_READ;
	msg->buffer = chTempBuf;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 3000);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - Gyro Selftest Timeout!!\n", __func__);
		goto exit;
	}

	data->uTimeOutCnt = 0;

	pr_err("[SSP]%d %d %d %d %d %d %d %d %d %d %d %d %d\n", chTempBuf[0],
		chTempBuf[1], chTempBuf[2], chTempBuf[3], chTempBuf[4],
		chTempBuf[5], chTempBuf[6], chTempBuf[7], chTempBuf[8],
		chTempBuf[9], chTempBuf[10], chTempBuf[11], chTempBuf[12]);
	/* Should I get bist? */
	selftest = chTempBuf[0];
	if (selftest == 0)
		bist = 1;
	else
		bist = 0;
	datax_check = (int)((chTempBuf[4] << 24) + (chTempBuf[3] << 16)
						  +(chTempBuf[2] << 8) + chTempBuf[1]);
	datay_check = (int)((chTempBuf[8] << 24) + (chTempBuf[7] << 16)
						  +(chTempBuf[6] << 8) + chTempBuf[5]);
	dataz_check = (int)((chTempBuf[12] << 24) + (chTempBuf[11] << 16)
						  +(chTempBuf[10] << 8) + chTempBuf[9]);
	iCalData[0] = (s16)((chTempBuf[14] << 8) +
				chTempBuf[13]);
	iCalData[1] = (s16)((chTempBuf[16] << 8) +
				chTempBuf[15]);
	iCalData[2] = (s16)((chTempBuf[18] << 8) +
				chTempBuf[17]);
	//shift_ratio[1] = (s16)((chTempBuf[4] << 8) +chTempBuf[3]);
	//total_count = (int)((chTempBuf[11] << 24) +(chTempBuf[10] << 16) + (chTempBuf[9] << 8) +chTempBuf[8]);
	pr_info("[SSP] gyro bist: %d, selftest: %d\n", bist, selftest);
	pr_info("[SSP] gyro X: %d, Y: %d, Z: %d\n", datax_check, datay_check, dataz_check);
	pr_info("[SSP] iCalData X: %d, Y: %d, Z: %d\n", iCalData[0], iCalData[1], iCalData[2]);
	//pr_info("[SSP] avg %+8ld %+8ld %+8ld (LSB)\n", avg[0], avg[1], avg[2]);

	if ((datax_check <= SELFTEST_LIMITATION_OF_ERROR)
		&& (datay_check <= SELFTEST_LIMITATION_OF_ERROR)
		&& (dataz_check <= SELFTEST_LIMITATION_OF_ERROR)) {
		pr_info("[SENSOR]: %s - Gyro zero rate OK!- Gyro selftest Pass\n", __func__);
		//bmg160_get_caldata(data);
		save_gyro_caldata(data, iCalData);
	} else {
		pr_info("[SENSOR]: %s - Gyro zero rate NG!- Gyro selftest fail!\n", __func__);
		selftest |= 1;
	}

	#if 0 /*Do Not saveing gyro cal after selftest NOW at BMI058*/
	if (likely(!ret_val)) {
		save_gyro_caldata(data, iCalData);
	} else {
		pr_err("[SSP] ret_val != 0, gyrocal is 0 at all axis\n");
		data->gyrocal.x = 0;
		data->gyrocal.y = 0;
		data->gyrocal.z = 0;
	}
	#endif

exit:
	pr_info("%d,%d,%d.%03d,%d.%03d,%d.%03d\n",
			selftest ? 0 : 1, bist,
			(datax_check / 1000), (datax_check % 1000),
			(datay_check / 1000), (datay_check % 1000),
			(dataz_check / 1000), (dataz_check % 1000));

	return sprintf(buf, "%d,%d,%d.%03d,%d.%03d,%d.%03d, %d,%d,%d,%d,%d,%d,%d,%d\n",
			selftest ? 0 : 1, bist,
			(datax_check / 1000), (datax_check % 1000),
			(datay_check / 1000), (datay_check % 1000),
			(dataz_check / 1000), (dataz_check % 1000),
			iRet, iRet, iRet, iRet, iRet, iRet, iRet, iRet);
}

static ssize_t gyro_selftest_dps_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int64_t iNewDps = 0;
	int iRet = 0;
	char chTempBuf = 0;

	struct ssp_data *data = dev_get_drvdata(dev);

	struct ssp_msg *msg;

	if (!(data->uSensorState & (1 << GYROSCOPE_SENSOR)))
		goto exit;

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = GYROSCOPE_DPS_FACTORY;
	msg->length = 1;
	msg->options = AP2HUB_READ;
	msg->buffer = &chTempBuf;
	msg->free_buffer = 0;

	iRet = kstrtoll(buf, 10, &iNewDps);
		if (iRet < 0)
			return iRet;

	if (iNewDps == GYROSCOPE_DPS250)
		msg->options |= 0 << SSP_GYRO_DPS;
	else if (iNewDps == GYROSCOPE_DPS500)
		msg->options |= 1 << SSP_GYRO_DPS;
	else if (iNewDps == GYROSCOPE_DPS2000)
		msg->options |= 2 << SSP_GYRO_DPS;
	else {
		msg->options |= 1 << SSP_GYRO_DPS;
		iNewDps = GYROSCOPE_DPS500;
	}

	iRet = ssp_spi_sync(data, msg, 3000);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - Gyro Selftest DPS Timeout!!\n", __func__);
		goto exit;
	}

	if (chTempBuf != SUCCESS) {
		pr_err("[SSP]: %s - Gyro Selftest DPS Error!!\n", __func__);
		goto exit;
	}

	data->uGyroDps = (unsigned int)iNewDps;
	pr_err("[SSP]: %s - %u dps stored\n", __func__, data->uGyroDps);
exit:
	return count;
}

static ssize_t gyro_selftest_dps_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", data->uGyroDps);
}

static DEVICE_ATTR(name, 0444, gyro_name_show, NULL);
static DEVICE_ATTR(vendor, 0444, gyro_vendor_show, NULL);
static DEVICE_ATTR(power_off, 0444, gyro_power_off, NULL);
static DEVICE_ATTR(power_on, 0444, gyro_power_on, NULL);
static DEVICE_ATTR(temperature, 0444, gyro_get_temp, NULL);
static DEVICE_ATTR(selftest, 0444, bmi058_gyro_selftest_show, NULL);
static DEVICE_ATTR(selftest_dps, 0664,
	gyro_selftest_dps_show, gyro_selftest_dps_store);

static struct device_attribute *gyro_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_selftest,
	&dev_attr_power_on,
	&dev_attr_power_off,
	&dev_attr_temperature,
	&dev_attr_selftest_dps,
	NULL,
};

void initialize_gyro_factorytest(struct ssp_data *data)
{
	sensors_register(data->gyro_device, data, gyro_attrs, "gyro_sensor");
}

void remove_gyro_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->gyro_device, gyro_attrs);
}
