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
/* factory Sysfs							 */
/*************************************************************************/

#define VENDOR		"STM"
#if defined (CONFIG_SENSORS_SSP_LSM6DSL)
#define CHIP_ID		"LSM6DSL"
#define SELFTEST_DPS_LIMIT  40
#elif  defined (CONFIG_SENSORS_SSP_LSM6DSO)
#define CHIP_ID		"LSM6DSO"
#define SELFTEST_DPS_LIMIT  15
#endif

#define CALIBRATION_FILE_PATH	"/efs/FactoryApp/gyro_cal_data"
#define VERBOSE_OUT 1
#define CALIBRATION_DATA_AMOUNT	20
#define DEF_GYRO_FULLSCALE	2000
//#define DEF_GYRO_SENS	(32768 / DEF_GYRO_FULLSCALE)
//#define DEF_BIAS_LSB_THRESH_SELF	(40 * DEF_GYRO_SENS)
//STMICRO
#define DEF_GYRO_SENS	(70) // 0.07 * 1000
#define DEF_BIAS_LSB_THRESH_SELF	(40000 / DEF_GYRO_SENS)

#define DEF_RMS_LSB_TH_SELF (5 * DEF_GYRO_SENS)
#define DEF_RMS_THRESH	((DEF_RMS_LSB_TH_SELF) * (DEF_RMS_LSB_TH_SELF))
#define DEF_SCALE_FOR_FLOAT (1000)
#define DEF_RMS_SCALE_FOR_RMS (10000)
#define DEF_SQRT_SCALE_FOR_RMS (100)
#define GYRO_LIB_DL_FAIL	9990

#ifndef ABS
#define ABS(a) ((a) > 0 ? (a) : -(a))
#endif

#define SELFTEST_REVISED 1

#define CALDATATOTALMAX		15
#define CALDATAFIELDLENGTH  17
static u8 gyroCalDataInfo[CALDATATOTALMAX][CALDATAFIELDLENGTH];
static int gyroCalDataIndx = -1;


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

static ssize_t selftest_revised_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", SELFTEST_REVISED);
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

		pr_err("[SSP]: %s - Can't open calibration file(%d)\n", __func__, -iRet);
		return iRet;
	}
	
	iRet = vfs_read(cal_filp, (char *)&data->gyrocal, 3 * sizeof(int), &cal_filp->f_pos);
	if (iRet < 0) {
		pr_err("[SSP]: %s - Can't read gyro cal to file\n", __func__);
		iRet = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	ssp_dbg("[SSP]: open gyro calibration %d, %d, %d\n",
		data->gyrocal.x, data->gyrocal.y, data->gyrocal.z);
	return iRet;
}

int save_gyro_caldata(struct ssp_data *data, s32 *iCalData)
{
	int iRet = 0;
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;
	u8 gyro_mag_cal[25] = {0, };

	data->gyrocal.x = iCalData[0];
	data->gyrocal.y = iCalData[1];
	data->gyrocal.z = iCalData[2];

	ssp_dbg("[SSP]: do gyro calibrate %d, %d, %d\n",
		data->gyrocal.x, data->gyrocal.y, data->gyrocal.z);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH, O_CREAT | O_RDWR | O_NOFOLLOW | O_NONBLOCK, 0660);
	if (IS_ERR(cal_filp)) {
		pr_err("[SSP]: %s - Can't open calibration file\n", __func__);
		set_fs(old_fs);
		iRet = PTR_ERR(cal_filp);
		return -EIO;
	}
	
	iRet = vfs_write(cal_filp, (char *)&data->gyrocal, 3 * sizeof(int), &cal_filp->f_pos);
	if (iRet < 0) {
		pr_err("[SSP]: %s - Can't write gyro cal to file\n", __func__);
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

	return iRet;
}

int set_gyro_cal(struct ssp_data *data)
{
	int iRet = 0;
	struct ssp_msg *msg;
	s32 gyro_cal[3];

	if (!(data->uSensorState & (1 << GYROSCOPE_SENSOR))) {
		pr_info("[SSP]: %s - Skip this function!!!, gyro sensor is not connected(0x%llx)\n",
			__func__, data->uSensorState);
		return iRet;
	}

	gyro_cal[0] = data->gyrocal.x;
	gyro_cal[1] = data->gyrocal.y;
	gyro_cal[2] = data->gyrocal.z;

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_AP_MCU_SET_GYRO_CAL;
	msg->length = sizeof(gyro_cal);
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(sizeof(gyro_cal), GFP_KERNEL);

	msg->free_buffer = 1;
	memcpy(msg->buffer, gyro_cal, sizeof(gyro_cal));

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

short lsm6dsl_gyro_get_temp(struct ssp_data *data)
{
	char chTempBuf[2] = { 0};
	unsigned char reg[2];
	short temperature = 0;
	int iRet = 0;
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	msg->cmd = GYROSCOPE_TEMP_FACTORY;
	msg->length = 2;
	msg->options = AP2HUB_READ;
	msg->buffer = chTempBuf;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 3000);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - Gyro Temp Timeout!!\n", __func__);
		goto exit;
	}

	reg[0] = chTempBuf[1];
	reg[1] = chTempBuf[0];
	temperature = (short) (((reg[0]) << 8) | reg[1]);
	ssp_dbg("[SSP]: %s - %d\n", __func__, temperature);

exit:
	return temperature;
}


static ssize_t gyro_get_temp(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	short temperature = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	temperature = lsm6dsl_gyro_get_temp(data);
	return sprintf(buf, "%d\n", temperature);
}
static ssize_t lsm6dsl_gyro_selftest(struct device *dev,
	struct device_attribute *attr, char *buf)
{
#if  defined (CONFIG_SENSORS_SSP_LSM6DSO)
	char chTempBuf[39] = { 0,};
        int fifo_test_result = 0, fifo_zro_result = 0, fifo_hwst_delta_result = 0;
#else
	char chTempBuf[36] = { 0,};
#endif
	u8 initialized = 0;
	s8 hw_result = 0;
	int j = 0, total_count = 0, ret_val = 0, gyro_lib_dl_fail = 0;
	long avg[3] = {0,}, rms[3] = {0,};
	int gyro_bias[3] = {0,};
	s16 shift_ratio[3] = {0,}; //self_diff value
	s32 iCalData[3] = {0,};
	char a_name[3][2] = { "X", "Y", "Z" };
	int iRet = 0;
	int bias_thresh = DEF_BIAS_LSB_THRESH_SELF;
	int fifo_ret = 0;
	int cal_ret = 0;
	int self_test_ret = 0;
	int self_test_zro_ret = 0;
	struct ssp_data *data = dev_get_drvdata(dev);
	s16 st_zro[3] = {0, };
	s16 st_bias[3] = {0, };
	int gyro_fifo_avg[3] = {0,}, gyro_self_zro[3] = {0,};
	int gyro_self_bias[3] = {0,}, gyro_self_diff[3] = {0,};
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	msg->cmd = GYROSCOPE_FACTORY;
	msg->length = ARRAY_SIZE(chTempBuf);
	msg->options = AP2HUB_READ;
	msg->buffer = chTempBuf;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 7000);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - Gyro Selftest Timeout!!\n", __func__);
		ret_val = 1;
		goto exit;
	}

	data->uTimeOutCnt = 0;

	pr_err("[SSP]%d %d %d %d %d %d %d %d %d %d %d %d\n", chTempBuf[0],
		chTempBuf[1], chTempBuf[2], chTempBuf[3], chTempBuf[4],
		chTempBuf[5], chTempBuf[6], chTempBuf[7], chTempBuf[8],
		chTempBuf[9], chTempBuf[10], chTempBuf[11]);

	initialized = chTempBuf[0];
	shift_ratio[0] = (s16)((chTempBuf[2] << 8) +
				chTempBuf[1]);
	shift_ratio[1] = (s16)((chTempBuf[4] << 8) +
				chTempBuf[3]);
	shift_ratio[2] = (s16)((chTempBuf[6] << 8) +
				chTempBuf[5]);
	hw_result = (s8)chTempBuf[7];
	total_count = (int)((chTempBuf[11] << 24) +
				(chTempBuf[10] << 16) +
				(chTempBuf[9] << 8) +
				chTempBuf[8]);
	avg[0] = (long)((chTempBuf[15] << 24) +
				(chTempBuf[14] << 16) +
				(chTempBuf[13] << 8) +
				chTempBuf[12]);
	avg[1] = (long)((chTempBuf[19] << 24) +
				(chTempBuf[18] << 16) +
				(chTempBuf[17] << 8) +
				chTempBuf[16]);
	avg[2] = (long)((chTempBuf[23] << 24) +
				(chTempBuf[22] << 16) +
				(chTempBuf[21] << 8) +
				chTempBuf[20]);
	rms[0] = (long)((chTempBuf[27] << 24) +
				(chTempBuf[26] << 16) +
				(chTempBuf[25] << 8) +
				chTempBuf[24]);
	rms[1] = (long)((chTempBuf[31] << 24) +
				(chTempBuf[30] << 16) +
				(chTempBuf[29] << 8) +
				chTempBuf[28]);
	rms[2] = (long)((chTempBuf[35] << 24) +
				(chTempBuf[34] << 16) +
				(chTempBuf[33] << 8) +
				chTempBuf[32]);

	st_zro[0] = (s16)((chTempBuf[25] << 8) +
				chTempBuf[24]);
	st_zro[1] = (s16)((chTempBuf[27] << 8) +
				chTempBuf[26]);
	st_zro[2] = (s16)((chTempBuf[29] << 8) +
				chTempBuf[28]);

	st_bias[0] = (s16)((chTempBuf[31] << 8) +
				chTempBuf[30]);
	st_bias[1] = (s16)((chTempBuf[33] << 8) +
				chTempBuf[32]);
	st_bias[2] = (s16)((chTempBuf[35] << 8) +
				chTempBuf[34]);
    
	pr_info("[SSP] init: %d, total cnt: %d\n", initialized, total_count);
	pr_info("[SSP] hw_result: %d, %d, %d, %d\n", hw_result,
		shift_ratio[0], shift_ratio[1],	shift_ratio[2]);
	pr_info("[SSP] avg %+8ld %+8ld %+8ld (LSB)\n", avg[0], avg[1], avg[2]);
	pr_info("[SSP] st_zro %+8d %+8d %+8d (LSB)\n", st_zro[0], st_zro[1], st_zro[2]);
	pr_info("[SSP] rms %+8ld %+8ld %+8ld (LSB)\n", rms[0], rms[1], rms[2]);
#if  defined (CONFIG_SENSORS_SSP_LSM6DSO)
        fifo_test_result = chTempBuf[36];
        fifo_zro_result = chTempBuf[37];
        fifo_hwst_delta_result = chTempBuf[38];
        pr_info("[SSP] test :%d zro :%d hwst_delta :%d", fifo_test_result, fifo_zro_result, fifo_hwst_delta_result);

        if(!fifo_test_result || !fifo_zro_result || !fifo_hwst_delta_result)        
                return sprintf(buf, "%d,%d,%d\n", fifo_test_result, fifo_zro_result, fifo_hwst_delta_result);
#endif

	//FIFO ZRO check pass / fail
	gyro_fifo_avg[0] = avg[0] * DEF_GYRO_SENS / DEF_SCALE_FOR_FLOAT;
	gyro_fifo_avg[1] = avg[1] * DEF_GYRO_SENS / DEF_SCALE_FOR_FLOAT;
	gyro_fifo_avg[2] = avg[2] * DEF_GYRO_SENS / DEF_SCALE_FOR_FLOAT;
	// ZRO self test
	gyro_self_zro[0] = st_zro[0] * DEF_GYRO_SENS / DEF_SCALE_FOR_FLOAT;
	gyro_self_zro[1] = st_zro[1] * DEF_GYRO_SENS / DEF_SCALE_FOR_FLOAT;
	gyro_self_zro[2] = st_zro[2] * DEF_GYRO_SENS / DEF_SCALE_FOR_FLOAT;
	//bias
	gyro_self_bias[0] = st_bias[0] * DEF_GYRO_SENS / DEF_SCALE_FOR_FLOAT;
	gyro_self_bias[1] = st_bias[1] * DEF_GYRO_SENS / DEF_SCALE_FOR_FLOAT;
	gyro_self_bias[2] = st_bias[2] * DEF_GYRO_SENS / DEF_SCALE_FOR_FLOAT;
	//diff = bias - ZRO
	gyro_self_diff[0] = shift_ratio[0] * DEF_GYRO_SENS / DEF_SCALE_FOR_FLOAT;
	gyro_self_diff[1] = shift_ratio[1] * DEF_GYRO_SENS / DEF_SCALE_FOR_FLOAT;
	gyro_self_diff[2] = shift_ratio[2] * DEF_GYRO_SENS / DEF_SCALE_FOR_FLOAT;

	if (total_count != 128) {
		pr_err("[SSP] %s, total_count is not 128. goto exit\n", __func__);
		ret_val = 2;
		goto exit;
	} else
		cal_ret = fifo_ret = 1;

	if ((gyro_self_diff[0] >= 150 && gyro_self_diff[0] <= 700)
			&& (gyro_self_diff[1] >= 150 && gyro_self_diff[1] <= 700)
			&& (gyro_self_diff[2] >= 150 && gyro_self_diff[2] <= 700))
		self_test_ret = 1;

	if (ABS(gyro_self_zro[0]) <= SELFTEST_DPS_LIMIT 
            && ABS(gyro_self_zro[1]) <= SELFTEST_DPS_LIMIT 
            && ABS(gyro_self_zro[2]) <= SELFTEST_DPS_LIMIT)
		self_test_zro_ret = 1;

	if (hw_result < 1) {
		pr_err("[SSP] %s - hw selftest fail(%d), sw selftest skip\n",
			__func__, hw_result);
		if (shift_ratio[0] == GYRO_LIB_DL_FAIL &&
			shift_ratio[1] == GYRO_LIB_DL_FAIL &&
			shift_ratio[2] == GYRO_LIB_DL_FAIL) {
			pr_err("[SSP] %s - gyro lib download fail\n", __func__);
			gyro_lib_dl_fail = 1;
		} else {
			/*
			 *ssp_dbg("[SSP]: %s - %d,%d,%d fail.\n",
			 *        __func__,
			 *        shift_ratio[0] / 10,
			 *        shift_ratio[1] / 10,
			 *        shift_ratio[2] / 10);
			 *return sprintf(buf, "%d,%d,%d\n",
			 *        shift_ratio[0] / 10,
			 *        shift_ratio[1] / 10,
			 *        shift_ratio[2] / 10);
			 */
			ssp_dbg("[SSP]: %s - %d,%d,%d fail.\n",	__func__, gyro_self_diff[0],
					gyro_self_diff[1], gyro_self_diff[2]);
			//return sprintf(buf, "%d,%d,%d\n", gyro_self_diff[0], gyro_self_diff[1], gyro_self_diff[2]);
		}
	}

	// AVG value range test +/- 40
	if ((ABS(gyro_fifo_avg[0]) > SELFTEST_DPS_LIMIT) 
            || (ABS(gyro_fifo_avg[1]) > SELFTEST_DPS_LIMIT) 
            || (ABS(gyro_fifo_avg[2]) > SELFTEST_DPS_LIMIT)) {
		ssp_dbg("[SSP]: %s - %d,%d,%d fail.\n",	__func__, gyro_fifo_avg[0], gyro_fifo_avg[1], gyro_fifo_avg[2]);
		return sprintf(buf, "%d,%d,%d\n", gyro_fifo_avg[0], gyro_fifo_avg[1], gyro_fifo_avg[2]);
	}

	// STMICRO
	gyro_bias[0] = avg[0] * DEF_GYRO_SENS;
	gyro_bias[1] = avg[1] * DEF_GYRO_SENS;
	gyro_bias[2] = avg[2] * DEF_GYRO_SENS;
	iCalData[0] = (s16)avg[0];
	iCalData[1] = (s16)avg[1];
	iCalData[2] = (s16)avg[2];

	if (VERBOSE_OUT) {
		pr_info("[SSP] abs bias : %+8d.%03d %+8d.%03d %+8d.%03d (dps)\n",
			(int)abs(gyro_bias[0]) / DEF_SCALE_FOR_FLOAT,
			(int)abs(gyro_bias[0]) % DEF_SCALE_FOR_FLOAT,
			(int)abs(gyro_bias[1]) / DEF_SCALE_FOR_FLOAT,
			(int)abs(gyro_bias[1]) % DEF_SCALE_FOR_FLOAT,
			(int)abs(gyro_bias[2]) / DEF_SCALE_FOR_FLOAT,
			(int)abs(gyro_bias[2]) % DEF_SCALE_FOR_FLOAT);
	}

	for (j = 0; j < 3; j++) {
		if (unlikely(abs(avg[j]) > bias_thresh)) {
			pr_err("[SSP] %s-Gyro bias (%ld) exceeded threshold (threshold = %d LSB)\n",
				a_name[j],
				avg[j], bias_thresh);
			ret_val |= 1 << (3 + j);
		}
	}
// STMICRO
/*
 *3rd, check RMS for dead gyros
 *If any of the RMS noise value returns zero,
 *then we might have dead gyro or FIFO/register failure,
 *the part is sleeping, or the part is not responsive
 */
	//if (rms[0] == 0 || rms[1] == 0 || rms[2] == 0)
	//ret_val |= 1 << 6;

	if (gyro_lib_dl_fail) {
		pr_err("[SSP] gyro_lib_dl_fail, Don't save cal data\n");
		ret_val = -1;
		goto exit;
	}

	if (likely(!ret_val)) {
		save_gyro_caldata(data, iCalData);
	} else {
		pr_err("[SSP] ret_val != 0, gyrocal is 0 at all axis\n");
		data->gyrocal.x = 0;
		data->gyrocal.y = 0;
		data->gyrocal.z = 0;
	}
exit:
	ssp_dbg("[SSP]: %s - %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		__func__,
		gyro_fifo_avg[0], gyro_fifo_avg[1], gyro_fifo_avg[2],
		gyro_self_zro[0], gyro_self_zro[1], gyro_self_zro[2],
		gyro_self_bias[0], gyro_self_bias[1], gyro_self_bias[2],
		gyro_self_diff[0], gyro_self_diff[1], gyro_self_diff[2],
		self_test_ret,
		self_test_zro_ret);

	// Gyro Calibration pass / fail, buffer 1~6 values.
	if ((fifo_ret == 0) || (cal_ret == 0))
		return sprintf(buf, "%d,%d,%d\n", gyro_self_diff[0], gyro_self_diff[1], gyro_self_diff[2]);

	return sprintf(buf,
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		gyro_fifo_avg[0], gyro_fifo_avg[1], gyro_fifo_avg[2],
		gyro_self_zro[0], gyro_self_zro[1], gyro_self_zro[2],
		gyro_self_bias[0], gyro_self_bias[1], gyro_self_bias[2],
		gyro_self_diff[0], gyro_self_diff[1], gyro_self_diff[2],
		self_test_ret,
		self_test_zro_ret);
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

	data->IsGyroselftest = true;
	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = GYROSCOPE_DPS_FACTORY;
	msg->length = 1;
	msg->options = AP2HUB_READ;
	msg->buffer = &chTempBuf;
	msg->free_buffer = 0;

	iRet = kstrtoll(buf, 10, &iNewDps);
	if (iRet < 0) {
		kfree(msg);
		return iRet;
	}

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
	data->IsGyroselftest = false;
	return count;
}

static ssize_t gyro_selftest_dps_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", data->uGyroDps);
}

struct gyro_cal_info {
	u16 version;
	u16 updated_index;
	s32 bias[3];	//x, y, z
	s8 temperature;
} __attribute__((__packed__));
static char printCalData[1024*3] = {0, };

static ssize_t gyro_calibration_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
   // struct ssp_data *data = dev_get_drvdata(dev);
	s32 i, j, float_bias;

	memset(printCalData, 0, sizeof(printCalData));

	if (gyroCalDataIndx == -1)
		return 0;
	for (i = 0; i <= gyroCalDataIndx; i++) {
		char temp[300] = {0, };
		struct gyro_cal_info infoFrame = {0, };

		memcpy(&infoFrame, gyroCalDataInfo[i], sizeof(struct gyro_cal_info));

		sprintf(temp, "VERSION:%hd,UPDATE_INDEX:%d,",
		infoFrame.version, infoFrame.updated_index);
		strcat(printCalData, temp);

		for (j = 0; j < 3; j++) {
			float_bias = ((infoFrame.bias[j] % 1000) >  0 ? (infoFrame.bias[j] % 1000)
					: -(infoFrame.bias[j] % 1000));

			if (infoFrame.bias[j] / 1000 == 0)
				infoFrame.bias[j] > 0 ? (sprintf(temp, "BIAS_%c:0.%03d,", 'X'+j, float_bias))
					: (sprintf(temp, "BIAS_%c:-0.%03d,", 'X'+j, float_bias));
			else
				sprintf(temp, "BIAS_%c:%d.%03d,", 'X'+j,  infoFrame.bias[j] / 1000, float_bias);

			strcat(printCalData, temp);
		}
		sprintf(temp, "TEMPERATURE:%d;", infoFrame.temperature);
		strcat(printCalData, temp);
	}

	gyroCalDataIndx = -1;
	return sprintf(buf, "%s", printCalData);
}

void set_GyroCalibrationInfoData(char *pchRcvDataFrame, int *iDataIdx)
{
	if (gyroCalDataIndx < (CALDATATOTALMAX - 1))
		memcpy(gyroCalDataInfo[++gyroCalDataIndx],  pchRcvDataFrame + *iDataIdx, CALDATAFIELDLENGTH);

	*iDataIdx += CALDATAFIELDLENGTH;
}

int send_vdis_flag(struct ssp_data *data, bool bFlag)
{
	int iRet = 0;
	struct ssp_msg *msg;
	char flag = bFlag;
	if (!(data->uSensorState & (1 << GYROSCOPE_SENSOR))) {
		pr_info("[SSP]: %s - Skip this function!!!, gyro sensor is not connected(0x%llx)\n",
			__func__, data->uSensorState);
		return iRet;
	}

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_INST_VDIS_FLAG;
	msg->length = 1;
	msg->options = AP2HUB_WRITE;
	msg->buffer = &flag;

	iRet = ssp_spi_async(data, msg);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
		iRet = ERROR;
	}

	pr_info("[SSP] Set VDIS FLAG %d\n", bFlag);
	return iRet;
}


static DEVICE_ATTR(name, 0440, gyro_name_show, NULL);
static DEVICE_ATTR(vendor, 0440, gyro_vendor_show, NULL);
static DEVICE_ATTR(power_off, 0440, gyro_power_off, NULL);
static DEVICE_ATTR(power_on, 0440, gyro_power_on, NULL);
static DEVICE_ATTR(temperature, 0440, gyro_get_temp, NULL);
static DEVICE_ATTR(selftest, 0440, lsm6dsl_gyro_selftest, NULL);
static DEVICE_ATTR(selftest_dps, 0660,
	gyro_selftest_dps_show, gyro_selftest_dps_store);
static DEVICE_ATTR(calibration_info, 0440, gyro_calibration_info_show, NULL);
static DEVICE_ATTR(selftest_revised, 0440, selftest_revised_show, NULL);

static struct device_attribute *gyro_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_selftest,
	&dev_attr_power_on,
	&dev_attr_power_off,
	&dev_attr_temperature,
	&dev_attr_selftest_dps,
	&dev_attr_calibration_info,
	&dev_attr_selftest_revised,
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
