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

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/
int initialize_thermistor_table(struct ssp_data *data)
{
	int iRet = SUCCESS;
	struct ssp_msg *msg;

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_AP_SET_THERMISTOR_TABLE;
	msg->length = 4;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(4, GFP_KERNEL);

	msg->free_buffer = 1;
	memcpy(&msg->buffer[0], &data->tempTable_up[18], 2);
	memcpy(&msg->buffer[2], &data->tempTable_sub[18], 2);

	iRet = ssp_spi_async(data, msg);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
		iRet = ERROR;
	}

	data->threshold_up = data->tempTable_up[18];
	data->threshold_sub = data->tempTable_sub[18];
	pr_info("[SSP]: %s: UP:%hd SUB:%hd finished\n", __func__, data->tempTable_up[14], data->tempTable_sub[14]);

	return iRet;
}

static ssize_t thermistor_up_table_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf,
	"%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
	data->tempTable_up[0], data->tempTable_up[1], data->tempTable_up[2], data->tempTable_up[3],
	data->tempTable_up[4], data->tempTable_up[5], data->tempTable_up[6], data->tempTable_up[7],
	data->tempTable_up[8], data->tempTable_up[9], data->tempTable_up[10], data->tempTable_up[11],
	data->tempTable_up[12], data->tempTable_up[13], data->tempTable_up[14], data->tempTable_up[15],
	data->tempTable_up[16], data->tempTable_up[17], data->tempTable_up[18], data->tempTable_up[19],
	data->tempTable_up[20], data->tempTable_up[21], data->tempTable_up[22]);
}

static ssize_t thermistor_sub_table_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf,
	"%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
	data->tempTable_sub[0], data->tempTable_sub[1], data->tempTable_sub[2], data->tempTable_sub[3],
	data->tempTable_sub[4], data->tempTable_sub[5], data->tempTable_sub[6], data->tempTable_sub[7],
	data->tempTable_sub[8], data->tempTable_sub[9], data->tempTable_sub[10], data->tempTable_sub[11],
	data->tempTable_sub[12], data->tempTable_sub[13], data->tempTable_sub[14], data->tempTable_sub[15],
	data->tempTable_sub[16], data->tempTable_sub[17], data->tempTable_sub[18], data->tempTable_sub[19],
	data->tempTable_sub[20], data->tempTable_sub[21], data->tempTable_sub[22]);
}

static ssize_t show_thermistor_channel0_ADC(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	unsigned short buffer = 0;
	int iRet = 0;

	msg->cmd = MSG2SSP_AP_THERMISTOR_CHANNEL_0;
	msg->length = sizeof(buffer);
	msg->options = AP2HUB_READ;
	msg->buffer = (char *)&buffer;

	iRet = ssp_spi_sync(data, msg, 1000);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - fail %d\n", __func__, iRet);
		return 0;
	} else {
		pr_err("[SSP]: %s -- ADC : %d\n", __func__, buffer);

		return sprintf(buf, "%d\n", buffer);
	}
}

static ssize_t show_thermistor_channel1_ADC(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	unsigned short buffer = 0;
	int iRet = 0;

	msg->cmd = MSG2SSP_AP_THERMISTOR_CHANNEL_1;
	msg->length = sizeof(buffer);
	msg->options = AP2HUB_READ;
	msg->buffer = (char *)&buffer;

	iRet = ssp_spi_sync(data, msg, 1000);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - fail %d\n", __func__, iRet);
		return 0;
	} else {
		pr_err("[SSP]: %s -- ADC : %d\n", __func__, buffer);

		return sprintf(buf, "%d\n", buffer);
	}
}

static ssize_t set_threshold_up(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	s32 degree = 0, i = 0, iRet = SUCCESS;
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg;

	if (kstrtos32(buf, 10, &degree) < 0)
		return -1;

	if (degree < -20 || degree > 90 || (degree % 5 != 0)) {
		pr_info("[SSP] invalid degree: %d", degree);
		return -EINVAL;
	}

	i = (degree - (-20)) / 5; // find index on table

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_AP_SET_THERMISTOR_TABLE;
	msg->length = 4;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(4, GFP_KERNEL);

	msg->free_buffer = 1;
	memcpy(&msg->buffer[0], &data->tempTable_up[i], 2);
	memcpy(&msg->buffer[2], &data->threshold_sub, 2);

	iRet = ssp_spi_async(data, msg);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
		iRet = ERROR;
	}

	data->threshold_up = data->tempTable_up[i];
	pr_info("[SSP] set threshold on up: %d", degree);

	return size;
}
static ssize_t show_threshold_up(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	s32 i, current_threshold = -20;

	for (i = 0; i < ARRAY_SIZE(data->tempTable_up); i++) {
		if (data->threshold_up == data->tempTable_up[i]) {
			current_threshold += (5 * i);
			break;
		}
	}

	return sprintf(buf, "%d\n", current_threshold);
}
static ssize_t set_threshold_sub(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	s32 degree = 0, i = 0, iRet = SUCCESS;
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg;

	if (kstrtos32(buf, 10, &degree) < 0)
		return -1;

	if (degree < -20 || degree > 90 || (degree % 5 != 0)) {
		pr_info("[SSP] invalid degree: %d", degree);
		return -EINVAL;
	}

	i = (degree - (-20)) / 5; // find index on table

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_AP_SET_THERMISTOR_TABLE;
	msg->length = 4;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(4, GFP_KERNEL);

	msg->free_buffer = 1;
	memcpy(&msg->buffer[0], &data->threshold_up, 2);
	memcpy(&msg->buffer[2], &data->tempTable_sub[i], 2);

	iRet = ssp_spi_async(data, msg);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
		iRet = ERROR;
	}

	data->threshold_sub = data->tempTable_sub[i];
	pr_info("[SSP] set threshold on sub: %d", degree);

	return size;
}
static ssize_t show_threshold_sub(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	s32 i, current_threshold = -20;

	for (i = 0; i < ARRAY_SIZE(data->tempTable_sub); i++) {
		if (data->threshold_sub == data->tempTable_sub[i]) {
			current_threshold += (5 * i);
			break;
		}
	}

	return sprintf(buf, "%d\n", current_threshold);
}

static DEVICE_ATTR(up_table,  0440,
	thermistor_up_table_show, NULL);
static DEVICE_ATTR(sub_table,  0440,
	thermistor_sub_table_show, NULL);
static DEVICE_ATTR(thermistor_channel_0_ADC, 0440, show_thermistor_channel0_ADC, NULL);
static DEVICE_ATTR(thermistor_channel_1_ADC, 0440, show_thermistor_channel1_ADC, NULL);
static DEVICE_ATTR(threshold_up, 0660, show_threshold_up, set_threshold_up);
static DEVICE_ATTR(threshold_sub, 0660, show_threshold_sub, set_threshold_sub);


static struct device_attribute *thermistor_attrs[] = {
	&dev_attr_up_table,
	&dev_attr_sub_table,
	&dev_attr_thermistor_channel_0_ADC,
	&dev_attr_thermistor_channel_1_ADC,
	&dev_attr_threshold_up,
	&dev_attr_threshold_sub,
	NULL,
};

void initialize_thermistor_factorytest(struct ssp_data *data)
{
	sensors_register(data->thermistor_device, data, thermistor_attrs, "thermistor_sensor");
}

void remove_thremistor_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->thermistor_device, thermistor_attrs);
}
