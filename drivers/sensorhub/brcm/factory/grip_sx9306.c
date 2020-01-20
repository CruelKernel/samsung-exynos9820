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

#define VENDOR					"SEMTECH"
#define CALIBRATION_FILE_PATH			"/efs/FactoryApp/grip_cal_data"
#define SLOPE_FILE_PATH				"/efs/FactoryApp/grip_slope_data"
#define TEMP_CAL_FILE_PATH			"/efs/FactoryApp/grip_temp_cal"
#define TEMP_FILE_PATH				"/sys/class/power_supply/battery/temp"

#define CAP_MAIN_FLAT				(481000)
#define DIFF_THRESH_FLAT			(500)
#define INIT_THRESH_FLAT			(CAP_MAIN_FLAT+DIFF_THRESH_FLAT)

#define CAP_MAIN_EDGE				(481000)
#define DIFF_THRESH_EDGE			(500)
#define INIT_THRESH_EDGE			(CAP_MAIN_EDGE+DIFF_THRESH_EDGE)

#define CAL_RET_ERROR				-1
#define CAL_RET_NONE				0
#define CAL_RET_EXIST				1
#define CAL_RET_SUCCESS				2

#define MSG2SSP_AP_MCU_GRIP_FACTORY		(0x90)
#define MSG2SSP_AP_MCU_GRIP_SET_CAL_DATA	(0x91)
#define MSG2SSP_AP_MCU_GRIP_GET_CAL_DATA	(0x92)
#define MSG2SSP_AP_MCU_GRIP_GET_RAW_DATA	(0x93)
#define MSG2SSP_AP_MCU_GRIP_CAPMIN_DATA		(0x94)

#define SX9306_GRIP_SENSOR
#if defined(SX9306_GRIP_SENSOR)
#define CHIP_ID					"SX9306"
#define REG_PROX_CTRL1				(0x07)
#define REG_PROX_CTRL2				(0x08)
#define REG_PROX_CTRL6				(0x0C)
#else
#define CHIP_ID					"SX9310"
#define REG_PROX_CTRL3				(0x13)
#define REG_PROX_CTRL5				(0x15)
#define REG_PROX_CTRL9				(0x19)
#endif

#define SYNC_TIME				(1000)
#define TEMP_ERROR				(0xFFFF)

#define TEMP_LOW_BOUND				(200)
#define TEMP_HIGH_BOUND				(400)

enum GRIP_FACTORY_CMD {
	GRIP_FACTORY_CMD_SHOW_ALL_REG = 0,
	GRIP_FACTORY_CMD_SHOW_ONE_REG,
	GRIP_FACTORY_CMD_SET_RANGE,
	GRIP_FACTORY_CMD_SET_GAIN,
	GRIP_FACTORY_CMD_SET_THRESHOLD,
	GRIP_FACTORY_CMD_SET_CALIBRATION_DATA,
	GRIP_FACTORY_CMD_GET_CALIBRATION_DATA,
	GRIP_FACTORY_CMD_SET_TEST_MODE,
	GRIP_FACTORY_CMD_MAX,
};

static int get_temp(void)
{
	struct file *filp = NULL;
	mm_segment_t old_fs;
	char temp_raw[6] = { 0, };
	long val = 0;
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(TEMP_FILE_PATH, O_RDONLY | O_NOFOLLOW,
			0664);
	if (IS_ERR(filp)) {
		ret = PTR_ERR(filp);
		if (ret != -ENOENT)
			pr_err("[SSP]: %s - Can't open temp file.\n",
				__func__);
		else {
			pr_info("[SSP]: %s - There is no temp file\n",
				__func__);
		}
		set_fs(old_fs);
		return TEMP_ERROR;
	}

	ret = filp->f_op->read(filp, temp_raw, sizeof(temp_raw), &filp->f_pos);
	if (ret <= 0) {
		pr_err("[SSP]: %s - Can't read the temp from file\n",
			__func__);
		filp_close(filp, current->files);
		set_fs(old_fs);
		return TEMP_ERROR;
	}

	filp_close(filp, current->files);
	set_fs(old_fs);

	if (kstrtol(temp_raw, 10, &val)) {
		pr_err("[SSP]: %s - Invalid Argument\n", __func__);
		return TEMP_ERROR;
	}

	return (int)val;
}

static int send_grip_factory_cmd(struct ssp_data *data,
				char reg_addr, char value)
{
	struct ssp_msg *msg;
	int ret;

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_AP_MCU_GRIP_FACTORY;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(sizeof(reg_addr) + sizeof(value), GFP_KERNEL);
	msg->length = sizeof(reg_addr) + sizeof(value);
	msg->free_buffer = 1;

	memcpy(&msg->buffer[0], &reg_addr, sizeof(reg_addr));
	memcpy(&msg->buffer[1], &value, sizeof(reg_addr));

	ret = ssp_spi_async(data, msg);
	if (ret != SUCCESS) {
		pr_err("[SSP]: %s - MSG2SSP_AP_MCU_GRIP_FACTORY CMD fail %d\n",
			__func__, ret);
		return -EIO;
	}

	return SUCCESS;
}

static int set_grip_factory_data(struct ssp_data *data,
				int cmd, char *buffer, int len)
{
	struct ssp_msg *msg;
	int ret = 0;

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = cmd;
	msg->options = AP2HUB_WRITE;
	msg->buffer = buffer;
	msg->length = len;
	msg->free_buffer = 0;

	ret = ssp_spi_async(data, msg);
	if (ret != SUCCESS) {
		pr_err("[SSP]: %s - %d CMD fail %d\n", __func__, cmd, ret);
		return -EIO;
	}

	return SUCCESS;
}

static int get_grip_factory_data(struct ssp_data *data,
				int cmd, char *buffer, int len)
{
	struct ssp_msg *msg;
	int ret = 0;

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = cmd;
	msg->options = AP2HUB_READ;
	msg->buffer = buffer;
	msg->length = len;
	msg->free_buffer = 0;

	ret = ssp_spi_sync(data, msg, SYNC_TIME);
	if (ret != SUCCESS) {
		pr_err("[SSP]: %s - %d CMD fail %d\n", __func__, cmd, ret);
		return -EIO;
	}

	return SUCCESS;
}

void open_grip_caldata(struct ssp_data *data)
{
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;
	int cap_main_len = sizeof(data->gripcal.cap_main);
	int ref_cap_main_len = sizeof(data->gripcal.ref_cap_main);
	int useful_len = sizeof(data->gripcal.useful);
	int offset_len = sizeof(data->gripcal.offset);
	int len = cap_main_len + ref_cap_main_len + useful_len + offset_len;
	int temp;
	int ret;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH, O_RDONLY | O_NOFOLLOW,
			0664);
	if (IS_ERR(cal_filp)) {
		ret = PTR_ERR(cal_filp);
		if (ret != -ENOENT)
			pr_err("[SSP]: %s - Can't open calibration file.\n",
				__func__);
		else {
			pr_info("[SSP]: %s - There is no calibration file\n",
				__func__);
			data->gripcal.cap_main = 0;
			data->gripcal.ref_cap_main = 0;
			data->gripcal.useful = 0,
			data->gripcal.offset = 0;
		}
		set_fs(old_fs);
		data->gripcal.threshold = data->gripcal.init_threshold;
		data->gripcal.mode_set = true;
		goto slope;
	}

	ret = cal_filp->f_op->read(cal_filp, (char *)&data->gripcal,
		len, &cal_filp->f_pos);
	if (ret != len) {
		pr_err("[SSP]: %s - Can't read the cal data from file\n",
			__func__);
		data->gripcal.threshold = data->gripcal.init_threshold;
	} else {
		data->gripcal.threshold
			= data->gripcal.cap_main + data->gripcal.diff_threshold;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	data->gripcal.mode_set = true;

slope:
	/* slope */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(SLOPE_FILE_PATH, O_RDONLY | O_NOFOLLOW,
			0664);
	if (IS_ERR(cal_filp)) {
		ret = PTR_ERR(cal_filp);
		if (ret != -ENOENT)
			pr_err("[SSP]: %s - Can't open slope file.\n",
				__func__);
		else {
			pr_info("[SSP]: %s - There is no slope file\n",
				__func__);
			data->gripcal.slope = 0;
		}
		set_fs(old_fs);
		goto temp;
	}

	ret = cal_filp->f_op->read(cal_filp, (char *)&data->gripcal.slope,
		sizeof(data->gripcal.slope), &cal_filp->f_pos);
	if (ret != sizeof(data->gripcal.slope)) {
		pr_err("[SSP]: %s - Can't read the slope from file\n",
			__func__);
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

temp:
	/* temp */
	temp = get_temp();
	if (temp != TEMP_ERROR)
		data->gripcal.temp = temp;

	/* temp_cal */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(TEMP_CAL_FILE_PATH, O_RDONLY | O_NOFOLLOW,
			0664);
	if (IS_ERR(cal_filp)) {
		ret = PTR_ERR(cal_filp);
		if (ret != -ENOENT)
			pr_err("[SSP]: %s - Can't open temp_cal file.\n",
				__func__);
		else {
			pr_info("[SSP]: %s - There is no temp_cal file\n",
				__func__);
			data->gripcal.temp_cal = 0;
		}
		goto exit;
	}

	ret = cal_filp->f_op->read(cal_filp, (char *)&data->gripcal.temp_cal,
		sizeof(data->gripcal.temp_cal), &cal_filp->f_pos);
	if (ret != sizeof(data->gripcal.temp_cal)) {
		pr_err("[SSP]: %s - Can't read the temp_cal from file\n",
			__func__);
	}

	filp_close(cal_filp, current->files);

exit:
	set_fs(old_fs);

	pr_info("[SSP]: %s - cal = (%d,%d,%d,%d)\n", __func__,
		data->gripcal.cap_main, data->gripcal.ref_cap_main,
		data->gripcal.useful, data->gripcal.offset);
	pr_info("[SSP]: %s - slope = %d\n", __func__, data->gripcal.slope);
	pr_info("[SSP]: %s - temp = %d\n", __func__, data->gripcal.temp);
	pr_info("[SSP]: %s - temp_cal = %d\n", __func__, data->gripcal.temp_cal);
}

int set_grip_calibration(struct ssp_data *data, bool set)
{
	int cmd = MSG2SSP_AP_MCU_GRIP_SET_CAL_DATA;
	int set_len = sizeof(set);
	int cap_main_len = sizeof(data->gripcal.cap_main);
	int ref_cap_main_len = sizeof(data->gripcal.ref_cap_main);
	int useful_len = sizeof(data->gripcal.useful);
	int offset_len = sizeof(data->gripcal.offset);
	int slope_len = sizeof(data->gripcal.slope);
	int temp_len = sizeof(data->gripcal.temp);
	int temp_cal_len = sizeof(data->gripcal.temp_cal);
	int len = set_len+cap_main_len+ref_cap_main_len+useful_len+offset_len
			+slope_len+temp_len+temp_cal_len;
	char cal_data[len];
	int ret = 0;

	if (!(data->uSensorState & (1 << GRIP_SENSOR))) {
		pr_info("[SSP]: %s - Skip this function!!!\n"
			"[SSP]: grip sensor is not connected(0x%llx)\n",
			__func__, data->uSensorState);
		return -EIO;
	}

	memcpy(&cal_data[0], &set, set_len);
	memcpy(&cal_data[set_len], &data->gripcal.cap_main, cap_main_len);
	memcpy(&cal_data[set_len+cap_main_len], &data->gripcal.ref_cap_main,
		ref_cap_main_len);
	memcpy(&cal_data[set_len+cap_main_len+ref_cap_main_len], &data->gripcal.useful,
		useful_len);
	memcpy(&cal_data[set_len+cap_main_len+ref_cap_main_len+useful_len],
		&data->gripcal.offset, offset_len);
	memcpy(&cal_data[set_len+cap_main_len+ref_cap_main_len+useful_len+offset_len],
		&data->gripcal.slope, slope_len);
	memcpy(&cal_data[set_len+cap_main_len+ref_cap_main_len+useful_len+offset_len+slope_len],
		&data->gripcal.temp, temp_len);
	memcpy(&cal_data[set_len+cap_main_len+ref_cap_main_len+useful_len+offset_len+slope_len+temp_len],
		&data->gripcal.temp_cal, temp_cal_len);

	ret = set_grip_factory_data(data, cmd, cal_data, sizeof(cal_data));
	if (ret != SUCCESS) {
		pr_err("[SSP]: %s - set_grip_factory_data fail %d\n",
			__func__, ret);
		return -EIO;
	}

	return SUCCESS;
}

static int get_grip_calibration(struct ssp_data *data)
{
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;
	bool set = 0;
	int cmd = MSG2SSP_AP_MCU_GRIP_GET_CAL_DATA;
	int set_len = sizeof(set);
	int cap_main_len = sizeof(data->gripcal.cap_main);
	int ref_cap_main_len = sizeof(data->gripcal.ref_cap_main);
	int useful_len = sizeof(data->gripcal.useful);
	int offset_len = sizeof(data->gripcal.offset);
	int len = set_len+cap_main_len+ref_cap_main_len+useful_len+offset_len;
	char cal_data[len];
	int ret = 0;

	if (!(data->uSensorState & (1 << GRIP_SENSOR))) {
		pr_info("[SSP]: %s - Skip this function!!!\n"
			"[SSP]: grip sensor is not connected(0x%llx)\n",
			__func__, data->uSensorState);
		return -EIO;
	}

	if (data->gripcal.temp < TEMP_LOW_BOUND ||
		data->gripcal.temp > TEMP_HIGH_BOUND) {
		pr_info("[SSP]: %s - Skip calibration temp = %d\n",
			__func__, data->gripcal.temp);
		return -EINVAL;
	}

	ret = get_grip_factory_data(data, cmd, cal_data, sizeof(cal_data));
	if (ret != SUCCESS) {
		pr_err("[SSP]: %s - get_grip_factory_data fail %d\n",
			__func__, ret);
		return -EIO;
	}

	for (ret = 0; ret < (int)sizeof(cal_data); ret++) {
		pr_info("[SSP]: %s cal_data[%d] = %d\n",
			__func__, ret, cal_data[ret]);
	}

	memcpy(&set, &cal_data[0], set_len);
	if (!set) {
		pr_err("[SSP]: %s - Grip sensor calibration fail %d\n",
			__func__, set);
		return -EIO;
	}

	memcpy(&data->gripcal.cap_main, &cal_data[set_len], cap_main_len);
	memcpy(&data->gripcal.ref_cap_main,
		&cal_data[set_len+cap_main_len], ref_cap_main_len);
	memcpy(&data->gripcal.useful,
		&cal_data[set_len+cap_main_len+ref_cap_main_len],
		useful_len);
	memcpy(&data->gripcal.offset,
		&cal_data[set_len+cap_main_len+ref_cap_main_len+useful_len],
		offset_len);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(TEMP_CAL_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY | O_SYNC | O_NOFOLLOW,
			0664);
	if (IS_ERR(cal_filp)) {
		pr_err("[SSP]: %s - Can't open temp_cal file\n",
			__func__);
		goto exit;
	}

	ret = cal_filp->f_op->write(cal_filp, (char *)&data->gripcal.temp,
		sizeof(data->gripcal.temp), &cal_filp->f_pos);
	if (ret != sizeof(data->gripcal.temp)) {
		pr_err("[SSP]: %s - Can't write the temp_cal to file\n",
			__func__);
	} else {
		data->gripcal.temp_cal = data->gripcal.temp;
	}

	filp_close(cal_filp, current->files);
exit:
	set_fs(old_fs);

	pr_info("[SSP]: Grip calibration - %d,%d,%d,%d temp_cal = %d\n",
		data->gripcal.cap_main, data->gripcal.ref_cap_main,
		data->gripcal.useful, data->gripcal.offset,
		data->gripcal.temp_cal);

	return SUCCESS;
}

static int do_calibrate(struct ssp_data *data, int do_calib)
{
	int ret = 0;

	if (do_calib) {
		ret = get_grip_calibration(data);
		if (ret < 0)
			pr_err("[SSP]: %s - get_grip_calibration failed\n",
				__func__);
	} else {
		ret = set_grip_calibration(data, false);
		if (ret < 0)
			pr_err("[SSP]: %s - set_grip_calibration failed\n",
				__func__);

		data->gripcal.cap_main = 0;
		data->gripcal.ref_cap_main = 0;
		data->gripcal.useful = 0,
		data->gripcal.offset = 0;

		pr_info("[SSP]: %s - Erased!\n", __func__);
	}

	pr_info("[SSP]: %s - (%d, %d, %d, %d)\n", __func__,
		data->gripcal.cap_main, data->gripcal.ref_cap_main,
		data->gripcal.useful, data->gripcal.offset);

	return ret;
}

static ssize_t vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t name_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	static bool mode; /* INIT mode */

	if (data->gripcal.mode_set) {
		s32 cap_main = 0;
		s16 useful = 0;
		s16 offset = 0;
		u8 irq_stat = 0;
		int cap_main_len = sizeof(cap_main);
		int useful_len = sizeof(useful);
		int offset_len = sizeof(offset);
		int irq_stat_len = sizeof(irq_stat);
		int len = cap_main_len + useful_len + offset_len + irq_stat_len;
		int cmd = MSG2SSP_AP_MCU_GRIP_GET_RAW_DATA;
		int ret = 0;
		char raw_data[len];

		msleep(400);
		ret = get_grip_factory_data(data, cmd,
				raw_data, sizeof(raw_data));
		if (ret != SUCCESS) {
			pr_err("[SSP]: %s - get_grip_factory_data fail %d\n",
				__func__, ret);
			mode = true;
		} else {
			memcpy(&cap_main, &raw_data[0], cap_main_len);

			if (cap_main <= data->gripcal.threshold)
				mode = true; /* NORMAL mode */
			else
				mode = false; /* INIT mode */
		}

		data->gripcal.mode_set = false;

		pr_info("[SSP]: %s(%d) - mode = [%d]\n",
			__func__, __LINE__, mode);
	} else {
		if (data->buf[GRIP_SENSOR].cap_main <= data->gripcal.threshold)
			mode = true; /* NORMAL mode */
	}

	return snprintf(buf, PAGE_SIZE, "%u\n", mode);
}

static ssize_t raw_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int cmd = MSG2SSP_AP_MCU_GRIP_GET_RAW_DATA;
	s32 cap_main = 0;
	s16 useful = 0;
	s16 offset = 0;
	u8 irq_stat = 0;
	int cap_main_len = sizeof(cap_main);
	int useful_len = sizeof(useful);
	int offset_len = sizeof(offset);
	int irq_stat_len = sizeof(irq_stat);
	int len = cap_main_len + useful_len + offset_len + irq_stat_len;
	char raw_data[len];
	int ret = 0;

	ret = get_grip_factory_data(data, cmd, raw_data, sizeof(raw_data));
	if (ret != SUCCESS) {
		pr_err("[SSP]: %s - get_grip_factory_data fail %d\n",
			__func__, ret);
		return -EIO;
	}

	memcpy(&cap_main, &raw_data[0], cap_main_len);
	memcpy(&useful, &raw_data[cap_main_len], useful_len);
	memcpy(&offset, &raw_data[cap_main_len+useful_len], offset_len);
	memcpy(&irq_stat, &raw_data[cap_main_len+useful_len+offset_len],
		irq_stat_len);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%u\n",
			cap_main, useful, offset);
}

static ssize_t calibration_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int ret;

	if (!data->gripcal.calibrated && !data->gripcal.cap_main)
		ret = CAL_RET_NONE;
	else if (!data->gripcal.calibrated && data->gripcal.cap_main)
		ret = CAL_RET_EXIST;
	else if (data->gripcal.calibrated && data->gripcal.cap_main)
		ret = CAL_RET_SUCCESS;
	else
		ret = CAL_RET_ERROR;

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", ret,
			data->gripcal.useful, data->gripcal.offset);
}

static int save_caldata(struct ssp_data *data)
{
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;
	int cap_main_len = sizeof(data->gripcal.cap_main);
	int ref_cap_main_len = sizeof(data->gripcal.ref_cap_main);
	int useful_len = sizeof(data->gripcal.useful);
	int offset_len = sizeof(data->gripcal.offset);
	int len = cap_main_len + ref_cap_main_len + useful_len + offset_len;
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY | O_SYNC | O_NOFOLLOW,
			0664);
	if (IS_ERR(cal_filp)) {
		pr_err("[SSP]: %s - Can't open calibration file\n",
			__func__);
		set_fs(old_fs);
		ret = PTR_ERR(cal_filp);
		return ret;
	}

	ret = cal_filp->f_op->write(cal_filp, (char *)&data->gripcal,
		len, &cal_filp->f_pos);
	if (ret != len) {
		pr_err("[SSP]: %s - Can't write the cal data to file\n",
			__func__);
		ret = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return ret;
}

static ssize_t calibration_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	bool do_calib;
	int ret;
	struct ssp_data *data = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "1"))
		do_calib = true;
	else if (sysfs_streq(buf, "0"))
		do_calib = false;
	else {
		pr_info("[SSP]: %s - invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	ret = do_calibrate(data, do_calib);
	if (ret < 0) {
		pr_err("[SSP]: %s - do_calibrate fail(%d)\n",
			__func__, ret);
		goto exit;
	}

	ret = save_caldata(data);
	if (ret < 0) {
		pr_err("[SSP]: %s - save_caldata fail(%d)\n",
			__func__, ret);
		goto exit;
	}

	pr_info("[SSP]: %s - %u success!\n", __func__, do_calib);

exit:

	if (data->gripcal.cap_main && (ret >= 0))
		data->gripcal.calibrated = true;
	else
		data->gripcal.calibrated = false;

	return count;
}

static ssize_t onoff_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", !data->grip_off);
}

static ssize_t onoff_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	unsigned long enable = 0;

	if (kstrtoul(buf, 10, &enable))
		return -EINVAL;

	if (enable)
		data->grip_off = false;
	else
		data->grip_off = true;

	return count;
}

static ssize_t threshold_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	if (data->gripcal.cap_main == 0
		&& data->gripcal.useful == 0
		&& data->gripcal.offset == 0)
		data->gripcal.threshold = data->gripcal.init_threshold;
	else
		data->gripcal.threshold
			= data->gripcal.cap_main + data->gripcal.diff_threshold;

	return snprintf(buf, PAGE_SIZE, "%d\n", data->gripcal.threshold);
}

static ssize_t threshold_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	unsigned long val;
	char reg_addr, value;
	int ret;

	if (kstrtoul(buf, 10, &val)) {
		pr_err("[SSP]: %s - Invalid Argument\n", __func__);
		return -EINVAL;
	}

	value = (char)val;
	reg_addr = GRIP_FACTORY_CMD_SET_THRESHOLD;

	ret = send_grip_factory_cmd(data, reg_addr, value);
	if (ret < 0) {
		pr_err("[SSP]: %s - send_grip_factory_cmd fail %d\n",
			__func__, ret);
	}

	pr_info("[SSP]: %s - threshold %u\n", __func__, value);
	data->gripcal.threshold = value;

	return count;
}

static ssize_t readback_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	char reg_addr, value;
	int ret;

	reg_addr = GRIP_FACTORY_CMD_SHOW_ALL_REG;
	value = 0;

	ret = send_grip_factory_cmd(data, reg_addr, value);
	if (ret < 0) {
		pr_err("[SSP]: %s - send_grip_factory_cmd fail %d\n",
			__func__, ret);
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t register_read_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0\n");
}

static ssize_t register_read_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	unsigned long val;
	char reg_addr, value;
	int ret;

	if (kstrtoul(buf, 10, &val)) {
		pr_err("[SSP]: %s - Invalid Argument\n", __func__);
		return -EINVAL;
	}

	value = (char)val;
	reg_addr = GRIP_FACTORY_CMD_SHOW_ONE_REG;

	ret = send_grip_factory_cmd(data, reg_addr, value);
	if (ret < 0) {
		pr_err("[SSP]: %s - send_grip_factory_cmd fail %d\n",
			__func__, ret);
	}

	return count;
}

static ssize_t register_write_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0\n");
}

static ssize_t register_write_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int reg_addr, value;
	int ret;

	if (sscanf(buf, "%d,%d", &reg_addr, &value) != 2) {
		pr_err("[SSP]: %s - wrong number of grip data\n",
			__func__);
		return -EINVAL;
	}

	pr_err("[SSP]: %s - reg_addr = %d\n", __func__, reg_addr);
	pr_err("[SSP]: %s - value = %d\n", __func__, value);

#if defined(SX9306_GRIP_SENSOR)
	if (reg_addr == REG_PROX_CTRL1) {
		reg_addr = GRIP_FACTORY_CMD_SET_RANGE;
	} else if (reg_addr == REG_PROX_CTRL2) {
		reg_addr = GRIP_FACTORY_CMD_SET_GAIN;
	} else if (reg_addr == REG_PROX_CTRL6) {
		reg_addr = GRIP_FACTORY_CMD_SET_THRESHOLD;
	} else {
		pr_err("[SSP]: %s - Invalid reg addr %d\n",
			__func__, reg_addr);
		return -EINVAL;
	}
#else
	if (reg_addr == REG_PROX_CTRL5) {
		reg_addr = GRIP_FACTORY_CMD_SET_RANGE;
	} else if (reg_addr == REG_PROX_CTRL3) {
		reg_addr = GRIP_FACTORY_CMD_SET_GAIN;
	} else if (reg_addr == REG_PROX_CTRL9) {
		reg_addr = GRIP_FACTORY_CMD_SET_THRESHOLD;
	} else {
		pr_err("[SSP]: %s - Invalid reg addr %d\n",
			__func__, reg_addr);
		return -EINVAL;
	}
#endif
	ret = send_grip_factory_cmd(data, (char)reg_addr, (char)value);
	if (ret < 0) {
		pr_err("[SSP]: %s - send_grip_factory_cmd fail %d\n",
			__func__, ret);
		return -EINVAL;
	}

	return count;
}

static ssize_t start_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0\n");
}

static ssize_t start_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	unsigned long val;
	char reg_addr, value;
	int ret;

	if (kstrtoul(buf, 10, &val)) {
		pr_err("[SSP]: %s - Invalid Argument\n", __func__);
		return -EINVAL;
	}

	value = (char)val;
	reg_addr = GRIP_FACTORY_CMD_SET_TEST_MODE;

	if ((value != 0) && (value != 1)) {
		pr_err("[SSP]: %s - Invalid Argument(%d)\n", __func__, value);
		return -EINVAL;
	}

	ret = send_grip_factory_cmd(data, reg_addr, value);
	if (ret < 0) {
		pr_err("[SSP]: %s - send_grip_factory_cmd fail %d\n",
			__func__, ret);
	}

	return count;
}

static ssize_t slope_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->gripcal.slope);
}

static ssize_t slope_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	struct file *cal_filp = NULL;
	unsigned long val;
	mm_segment_t old_fs;
	char slope = 0;
	int ret = 0;

	if (kstrtoul(buf, 10, &val)) {
		pr_err("[SSP]: %s - Invalid Argument\n", __func__);
		return -EINVAL;
	}
	slope = (char)val;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(SLOPE_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY | O_SYNC | O_NOFOLLOW,
			0664);
	if (IS_ERR(cal_filp)) {
		pr_err("[SSP]: %s - Can't open slope file\n",
			__func__);
		set_fs(old_fs);
		return -EIO;
	}

	ret = cal_filp->f_op->write(cal_filp, (char *)&slope,
		sizeof(slope), &cal_filp->f_pos);
	if (ret != sizeof(slope)) {
		pr_err("[SSP]: %s - Can't write the slope data to file\n",
			__func__);
		ret = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	if (ret < 0) {
		return ret;

	data->gripcal.slope = slope;
	return count;
}

static ssize_t temp_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->gripcal.temp);
}

static ssize_t temp_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t temp_cal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	pr_info("[SSP]: %s - temp_cal = %d\n", __func__, data->gripcal.temp_cal);
	return snprintf(buf, PAGE_SIZE, "%d\n", data->gripcal.temp_cal);
}

static ssize_t temp_cal_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t capmain_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int cmd = MSG2SSP_AP_MCU_GRIP_CAPMIN_DATA;
	s32 cap_main_comp = 0;
	s32 cap_main_real = 0;
	s32 cap_main_cal = 0;
	int cap_main_comp_len = sizeof(cap_main_comp);
	int cap_main_real_len = sizeof(cap_main_real);
	int cap_main_cal_len = sizeof(cap_main_cal);
	int len = cap_main_comp_len + cap_main_real_len + cap_main_cal_len;
	char raw_data[len];
	int ret = 0;

	ret = get_grip_factory_data(data, cmd, raw_data, sizeof(raw_data));
	if (ret != SUCCESS) {
		pr_err("[SSP]: %s - get_grip_factory_data fail %d\n",
			__func__, ret);
		return -EIO;
	}

	memcpy(&cap_main_comp, &raw_data[0], cap_main_comp_len);
	memcpy(&cap_main_real, &raw_data[cap_main_comp_len], cap_main_real_len);
	memcpy(&cap_main_cal, &raw_data[cap_main_comp_len+cap_main_real_len], cap_main_cal_len);

	pr_err("[SSP]: %s - %d,%d,%d,%d,%d,%d\n", __func__,
		cap_main_comp, cap_main_real, cap_main_cal,
		data->gripcal.slope, data->gripcal.temp, data->gripcal.temp_cal);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
			cap_main_comp, cap_main_real, cap_main_cal);
}

static DEVICE_ATTR(name, 0444, name_show, NULL);
static DEVICE_ATTR(vendor, 0444, vendor_show, NULL);
static DEVICE_ATTR(mode, 0444, mode_show, NULL);
static DEVICE_ATTR(raw_data, 0444, raw_data_show, NULL);
static DEVICE_ATTR(calibration, 0664,
		calibration_show, calibration_store);
static DEVICE_ATTR(onoff, 0664,
		onoff_show, onoff_store);
static DEVICE_ATTR(threshold, 0664,
		threshold_show, threshold_store);
static DEVICE_ATTR(readback, 0444, readback_show, NULL);
static DEVICE_ATTR(register_read, 0664,
		register_read_show, register_read_store);
static DEVICE_ATTR(register_write, 0664,
		register_write_show, register_write_store);
static DEVICE_ATTR(start, 0664, start_show, start_store);
static DEVICE_ATTR(slope, 0664, slope_show, slope_store);
static DEVICE_ATTR(temp, 0664, temp_show, temp_store);
static DEVICE_ATTR(temp_cal, 0664, temp_cal_show, temp_cal_store);
static DEVICE_ATTR(capmain, 0444, capmain_show, NULL);


static struct device_attribute *grip_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_mode,
	&dev_attr_raw_data,
	&dev_attr_calibration,
	&dev_attr_onoff,
	&dev_attr_threshold,
	&dev_attr_readback,
	&dev_attr_register_read,
	&dev_attr_register_write,
	&dev_attr_start,
	&dev_attr_slope,
	&dev_attr_temp,
	&dev_attr_temp_cal,
	&dev_attr_capmain,
	NULL,
};

void initialize_grip_factorytest(struct ssp_data *data)
{
	sensors_register(data->grip_device, data, grip_attrs, "grip_sensor");

	if (data->glass_type) { /* edge */
		data->gripcal.init_threshold = INIT_THRESH_EDGE;
		data->gripcal.diff_threshold = DIFF_THRESH_EDGE;
	} else { /* flat */
		data->gripcal.init_threshold = INIT_THRESH_FLAT;
		data->gripcal.diff_threshold = DIFF_THRESH_FLAT;
	}
}

void remove_grip_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->grip_device, grip_attrs);
}
