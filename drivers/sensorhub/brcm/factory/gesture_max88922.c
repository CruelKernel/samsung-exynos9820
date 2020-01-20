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

#define	VENDOR		"MAXIM"
#define	CHIP_ID		"MAX88922"


static ssize_t gestrue_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t gestrue_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t raw_data_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d\n",
		data->buf[GESTURE_SENSOR].data[1],
		data->buf[GESTURE_SENSOR].data[2],
		data->buf[GESTURE_SENSOR].data[3],
		data->buf[GESTURE_SENSOR].data[9]);
}

static ssize_t gesture_get_selftest_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	s16 raw_A = 0, raw_B = 0, raw_C = 0, raw_D = 0;
	int iRet = 0;
	char chTempBuf[8] = { 0, };
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	msg->cmd = GESTURE_FACTORY;
	msg->length = 8;
	msg->options = AP2HUB_READ;
	msg->buffer = chTempBuf;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 2000);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - Gesture Selftest Timeout!!\n", __func__);
		goto exit;
	}

	pr_info("%x %x %x %x %x %x %x %x\n", chTempBuf[0], chTempBuf[1], chTempBuf[2], chTempBuf[3],
		chTempBuf[4], chTempBuf[5], chTempBuf[6], chTempBuf[7]);

	raw_A = ((((s16)chTempBuf[0]) << 8) + ((s16)chTempBuf[1])) - 1023;
	raw_B = ((((s16)chTempBuf[2]) << 8) + ((s16)chTempBuf[3])) - 1023;
	raw_C = ((((s16)chTempBuf[4]) << 8) + ((s16)chTempBuf[5])) - 1023;
	raw_D = ((((s16)chTempBuf[6]) << 8) + ((s16)chTempBuf[7])) - 1023;

	pr_info("[SSP] %s: self test A = %d, B = %d, C = %d, D = %d\n",
		__func__, raw_A, raw_B, raw_C, raw_D);

exit:
	return sprintf(buf, "%d,%d,%d,%d\n",
	raw_A, raw_B, raw_C, raw_D);
}

static ssize_t ir_current_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_dbg("[SSP]: %s - Ir_Current Setting = %d\n",
		__func__, data->uIr_Current);

	return sprintf(buf, "%d\n", data->uIr_Current);
}

static ssize_t ir_current_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	u16 uNewIrCurrent = DEFAULT_IR_CURRENT;
	int iRet = 0;
	u16 current_index = 0;
	struct ssp_data *data = dev_get_drvdata(dev);
	static u16 set_current[2][16] = { {0, 6, 13, 20, 26, 33, 40, 46, 53, 60, 66, 73, 80, 86, 93, 100},
					  {0<<4, 1<<4, 2<<4, 3<<4, 4<<4, 5<<4, 6<<4, 7<<4,
						8<<4, 9<<4, 10<<4, 11<<4, 12<<4, 13<<4, 14<<4, 15<<4} };

	iRet = kstrtou16(buf, 10, &uNewIrCurrent);

	if (iRet < 0)
		pr_err("[SSP]: %s - kstrtoint failed.(%d)\n", __func__, iRet);
	else {
		for (current_index = 0; current_index < 16; current_index++) {
			if (set_current[0][current_index] == uNewIrCurrent) {
				data->uIr_Current = set_current[1][current_index];
				break;
			}
		}
		if (current_index == 16) {// current setting value wrong.
			return ERROR;
		}
		set_gesture_current(data, data->uIr_Current);
		data->uIr_Current = uNewIrCurrent;
	}

	ssp_dbg("[SSP]: %s - new Ir_Current Setting : %d\n",
	__func__, data->uIr_Current);

	return size;
}

static DEVICE_ATTR(vendor, 0444, gestrue_vendor_show, NULL);
static DEVICE_ATTR(name, 0444, gestrue_name_show, NULL);
static DEVICE_ATTR(raw_data, 0444, raw_data_read, NULL);
static DEVICE_ATTR(selftest, 0444, gesture_get_selftest_show, NULL);
static DEVICE_ATTR(ir_current, 0664,
	ir_current_show, ir_current_store);

static struct device_attribute *gesture_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_raw_data,
	&dev_attr_selftest,
	&dev_attr_ir_current,
	NULL,
};

void initialize_gesture_factorytest(struct ssp_data *data)
{
	sensors_register(data->ges_device, data,
		gesture_attrs, "gesture_sensor");
}

void remove_gesture_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->ges_device, gesture_attrs);
}
