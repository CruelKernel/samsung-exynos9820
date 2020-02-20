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

#define	VENDOR		"AMS"
#if defined(CONFIG_SENSORS_SSP_TMG399x)
#define	CHIP_ID		"TMG399X"
#elif defined(CONFIG_SENSORS_SSP_TMD3782)
#define CHIP_ID		"TMD3782"
#elif defined(CONFIG_SENSORS_SSP_TMD4903)
#define CHIP_ID		"TMD4903"
#elif defined(CONFIG_SENSORS_SSP_TMD4904)
#define CHIP_ID		"TMD4904"
#elif defined(CONFIG_SENSORS_SSP_TMD4905)
#define CHIP_ID		"TMD4905"
#elif defined(CONFIG_SENSORS_SSP_TMD4906)
#define CHIP_ID		"TMD4906"
#elif defined(CONFIG_SENSORS_SSP_TMD4907)
#define CHIP_ID		"TMD4907"
#elif defined(CONFIG_SENSORS_SSP_TMD4910)
#define CHIP_ID		"TMD4910"
#else
#define CHIP_ID		"UNKNOWN"
#endif

#define CONFIG_PANEL_NOTIFY	1
/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/
static ssize_t light_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t light_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_SENSORS_SSP_BEYOND)
	struct ssp_data *data = dev_get_drvdata(dev);
	char *name[5] = {"TCS3407", "TCS3407", "TCS3407", "TCS3701", "UNKNOWN"};
	if (data->ap_type >= 0 && data->ap_type < 4) {
		return sprintf(buf, "%s\n", name[data->ap_type]);
	} else {
		return sprintf(buf, "%s\n", name[4]);
	}
#endif
	return sprintf(buf, "%s\n", CHIP_ID);
}

static ssize_t light_lux_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%u,%u,%u,%u,%u,%u\n",
		data->buf[UNCAL_LIGHT_SENSOR].r, data->buf[UNCAL_LIGHT_SENSOR].g,
		data->buf[UNCAL_LIGHT_SENSOR].b, data->buf[UNCAL_LIGHT_SENSOR].w,
		data->buf[UNCAL_LIGHT_SENSOR].a_time, data->buf[UNCAL_LIGHT_SENSOR].a_gain);
}

static ssize_t light_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%u,%u,%u,%u,%u,%u\n",
		data->buf[UNCAL_LIGHT_SENSOR].r, data->buf[UNCAL_LIGHT_SENSOR].g,
		data->buf[UNCAL_LIGHT_SENSOR].b, data->buf[UNCAL_LIGHT_SENSOR].w,
		data->buf[UNCAL_LIGHT_SENSOR].a_time, data->buf[UNCAL_LIGHT_SENSOR].a_gain);
}

static ssize_t light_coef_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int iRet, iReties = 0;
	struct ssp_msg *msg;
	int coef_buf[7];

	memset(coef_buf, 0, sizeof(int)*7);
retries:
	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_AP_GET_LIGHT_COEF;
	msg->length = 28;
	msg->options = AP2HUB_READ;
	msg->buffer = (u8 *)coef_buf;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);
	if (iRet != SUCCESS) {
		pr_err("[SSP] %s fail %d\n", __func__, iRet);

		if (iReties++ < 2) {
			pr_err("[SSP] %s fail, retry\n", __func__);
			mdelay(5);
			goto retries;
		}
		return FAIL;
	}

	pr_info("[SSP] %s - %d %d %d %d %d %d %d\n", __func__,
		coef_buf[0], coef_buf[1], coef_buf[2], coef_buf[3], coef_buf[4], coef_buf[5], coef_buf[6]);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d\n",
		coef_buf[0], coef_buf[1], coef_buf[2], coef_buf[3], coef_buf[4], coef_buf[5], coef_buf[6]);
}

#ifdef CONFIG_PANEL_NOTIFY
static ssize_t light_sensorhub_ddi_spi_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{

 	struct ssp_data *data = dev_get_drvdata(dev);
	int iRet = 0;
	int iReties = 0;
	struct ssp_msg *msg;
	short copr_buf = 0;

retries:
	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_GET_DDI_COPR;
	msg->length = sizeof(copr_buf);
	msg->options = AP2HUB_READ;
	msg->buffer = (u8 *)&copr_buf;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);
	if (iRet != SUCCESS) {
		pr_err("[SSP] %s fail %d\n", __func__, iRet);

		if (iReties++ < 2) {
			pr_err("[SSP] %s fail, retry\n", __func__);
			mdelay(5);
			goto retries;
		}
		return FAIL;
	}

	pr_info("[SSP] %s - %d\n", __func__, copr_buf);

	return snprintf(buf, PAGE_SIZE, "%d\n", copr_buf);
}

static ssize_t light_test_copr_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{

 	struct ssp_data *data = dev_get_drvdata(dev);
	int iRet = 0;
	int iReties = 0;
	struct ssp_msg *msg;
	short copr_buf[4];

	memset(copr_buf, 0, sizeof(copr_buf));
retries:
	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_GET_TEST_COPR;
	msg->length = sizeof(copr_buf);
	msg->options = AP2HUB_READ;
	msg->buffer = (u8 *)&copr_buf;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);
	if (iRet != SUCCESS) {
		pr_err("[SSP] %s fail %d\n", __func__, iRet);

		if (iReties++ < 2) {
			pr_err("[SSP] %s fail, retry\n", __func__);
			mdelay(5);
			goto retries;
		}
		return FAIL;
	}

	pr_info("[SSP] %s - %d, %d, %d, %d\n", __func__,
		       	copr_buf[0], copr_buf[1], copr_buf[2], copr_buf[3]);

	return snprintf(buf, PAGE_SIZE, "%d, %d, %d, %d\n", 
			copr_buf[0], copr_buf[1], copr_buf[2], copr_buf[3]);
}

static ssize_t light_copr_roix_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{

 	struct ssp_data *data = dev_get_drvdata(dev);
	int iRet = 0;
	int iReties = 0;
	struct ssp_msg *msg;
	short copr_buf[12];

	memset(copr_buf, 0, sizeof(copr_buf));
retries:
	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_GET_COPR_ROIX;
	msg->length = sizeof(copr_buf);
	msg->options = AP2HUB_READ;
	msg->buffer = (u8 *)&copr_buf;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);
	if (iRet != SUCCESS) {
		pr_err("[SSP] %s fail %d\n", __func__, iRet);

		if (iReties++ < 2) {
			pr_err("[SSP] %s fail, retry\n", __func__);
			mdelay(5);
			goto retries;
		}
		return FAIL;
	}

	pr_info("[SSP] %s - %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", __func__,
		       	copr_buf[0], copr_buf[1], copr_buf[2], copr_buf[3],
			copr_buf[4], copr_buf[5], copr_buf[6], copr_buf[7],
			copr_buf[8], copr_buf[9], copr_buf[10], copr_buf[11]);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", 
			copr_buf[0], copr_buf[1], copr_buf[2], copr_buf[3],
			copr_buf[4], copr_buf[5], copr_buf[6], copr_buf[7],
			copr_buf[8], copr_buf[9], copr_buf[10], copr_buf[11]);
}


static ssize_t light_read_copr_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{

 	struct ssp_data *data = dev_get_drvdata(dev);
	int iRet = 0;
	int iReties = 0;
	struct ssp_msg *msg;
	short copr_buf;

retries:
	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_GET_READ_COPR;
	msg->length = sizeof(copr_buf);
	msg->options = AP2HUB_READ;
	msg->buffer = (u8 *)&copr_buf;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);
	if (iRet != SUCCESS) {
		pr_err("[SSP] %s fail %d\n", __func__, iRet);

		if (iReties++ < 2) {
			pr_err("[SSP] %s fail, retry\n", __func__);
			mdelay(5);
			goto retries;
		}
		return FAIL;
	}

	pr_info("[SSP] %s - %d\n", __func__, copr_buf);

	return snprintf(buf, PAGE_SIZE, "%d\n", copr_buf);
}

static ssize_t light_read_copr_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int iRet = 0;
	bool on_off = sysfs_streq(buf, "1");
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	
	pr_info("[SSP] %s - %s\n", __func__, on_off == true ? "on" : "off");

	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n", __func__);
		return iRet;
	}
	msg->cmd = MSG2SSP_READ_COPR_ON_OFF;
	msg->length = 1;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(1, GFP_KERNEL);
	msg->free_buffer = 1;
	msg->buffer[0] = (char)on_off;

	iRet = ssp_spi_async(data, msg);
	
	if (iRet != SUCCESS) {
		pr_err("[SSP] %s -fail to %s %d\n", __func__, __func__, iRet);
		iRet = ERROR;
	}
	return size;
}

struct decimal_point {
	int integer, point;
};

void set_decimal_point(struct decimal_point* dp, int i, int p) {
	dp->integer = i;
	dp->point  = p;
}

static ssize_t light_circle_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	struct decimal_point x = { 0, }, y = { 0, }, diameter = { 0, };

#if defined(CONFIG_SENSORS_SSP_BEYOND)
	if(data->ap_type == 3) {
		set_decimal_point(&x, 26, 9);
		set_decimal_point(&y, 7,  5);
		set_decimal_point(&diameter, 2, 2);
	} else if(data->ap_rev < 20) {
	// DV 1th
		switch(data->ap_type) {
			case 0:
				set_decimal_point(&x, 47, 3);
				set_decimal_point(&y, 1,  1);
				set_decimal_point(&diameter, 2, 3);
				break;
			case 1:
				set_decimal_point(&x, 49, 5);
				set_decimal_point(&y, 1,  2);
				set_decimal_point(&diameter, 2, 2);
				break;
			case 2:
				set_decimal_point(&x, 48, 1);
				set_decimal_point(&y, 1,  0);
				set_decimal_point(&diameter, 2, 2);
				break;
		}
	} else if(data->ap_rev < 23) {
	// DV 2th ~ PV 1th
		switch(data->ap_type) {
			case 0:
				set_decimal_point(&x, 47, 3);
				set_decimal_point(&y, 1,  1);
				set_decimal_point(&diameter, 2, 3);
				break;
			case 1:
				set_decimal_point(&x, 49, 5);
				set_decimal_point(&y, 1,  2);
				set_decimal_point(&diameter, 2, 2);
				break;
			case 2:
				set_decimal_point(&x, 48, 1);
				set_decimal_point(&y, 1,  0);
				set_decimal_point(&diameter, 2, 2);
				break;
		}
	} else {
	// PV 2th ~
		switch(data->ap_type) {
			case 0:
				set_decimal_point(&x, 47, 3);
				set_decimal_point(&y, 8,  7);
				set_decimal_point(&diameter, 2, 2);
				break;
			case 1:
				set_decimal_point(&x, 49, 8);
				set_decimal_point(&y, 8,  5);
				set_decimal_point(&diameter, 2, 2);
				break;
			case 2:
				set_decimal_point(&x, 48, 0);
				set_decimal_point(&y, 8,  7);
				set_decimal_point(&diameter, 2, 2);
				break;
		}	
	}
#elif defined(CONFIG_SENSORS_SSP_DAVINCI)
	switch(data->ap_type) {
		case 0:
		case 1:
			set_decimal_point(&x, 41, 3);
			set_decimal_point(&y, 7,  1);
			set_decimal_point(&diameter, 2, 4);

			break;
		case 2:
		case 3:
			set_decimal_point(&x, 43, 8);
			set_decimal_point(&y, 6,  7);
			set_decimal_point(&diameter, 2, 4);
			break;
	}
#endif	

	return snprintf(buf, PAGE_SIZE, "%d.%d %d.%d %d.%d\n", x.integer, x.point,
			y.integer, y.point, diameter.integer, diameter.point);
}
#endif

#ifdef CONFIG_SENSORS_SSP_PROX_SETTING
static ssize_t light_coef_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int iRet, i;
	int coef[7];
	char *token;
	char *str;
	struct ssp_data *data = dev_get_drvdata(dev);

	pr_info("[SSP] %s - %s\n", __func__, buf);

	//parsing
	str = (char *)buf;
	for (i = 0; i < 7; i++) {
		token = strsep(&str, "\n");
		if (token == NULL) {
			pr_err("[SSP] %s : too few arguments (7 needed)", __func__);
				return -EINVAL;
		}

		iRet = kstrtos32(token, 10, &coef[i]);
		if (iRet < 0) {
			pr_err("[SSP] %s : kstrtou8 error %d", __func__, iRet);
			return iRet;
		}
	}

	memcpy(data->light_coef, coef, sizeof(data->light_coef));

	set_light_coef(data);

	return size;
}
#endif

static DEVICE_ATTR(vendor, 0440, light_vendor_show, NULL);
static DEVICE_ATTR(name, 0440, light_name_show, NULL);
static DEVICE_ATTR(lux, 0440, light_lux_show, NULL);
static DEVICE_ATTR(raw_data, 0440, light_data_show, NULL);

#ifdef CONFIG_SENSORS_SSP_PROX_SETTING
static DEVICE_ATTR(coef, 0660, light_coef_show, light_coef_store);
#else
static DEVICE_ATTR(coef, 0440, light_coef_show, NULL);
#endif

#ifdef CONFIG_PANEL_NOTIFY
static DEVICE_ATTR(sensorhub_ddi_spi_check, 0440, light_sensorhub_ddi_spi_check_show, NULL);
static DEVICE_ATTR(test_copr, 0440, light_test_copr_show, NULL);
static DEVICE_ATTR(light_circle, 0440, light_circle_show, NULL);
static DEVICE_ATTR(read_copr, 0660, light_read_copr_show, light_read_copr_store);
static DEVICE_ATTR(copr_roix, 0440, light_copr_roix_show, NULL);
#endif

static struct device_attribute *light_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_lux,
	&dev_attr_raw_data,
	&dev_attr_coef,
#ifdef CONFIG_PANEL_NOTIFY
	&dev_attr_sensorhub_ddi_spi_check,
	&dev_attr_test_copr,
	&dev_attr_read_copr,
	&dev_attr_light_circle,
	&dev_attr_copr_roix,
#endif
	NULL,
};

void initialize_light_factorytest(struct ssp_data *data)
{
	sensors_register(data->light_device, data, light_attrs, "light_sensor");
}

void remove_light_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->light_device, light_attrs);
}
