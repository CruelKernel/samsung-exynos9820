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
#include <linux/platform_device.h>
#include <plat/adc.h>

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

#define VENDOR		"SENSIRION"
#define CHIP_ID		"SHTC1"

#define CP_THM_ADC_SAMPLING_CNT	7
#define DONE_CAL	3

#define SHTC1_IOCTL_MAGIC		0xFB
#define IOCTL_READ_COMPLETE	_IOR(SHTC1_IOCTL_MAGIC, 0x01, unsigned short *)
#define IOCTL_READ_ADC_BATT_DATA	_IOR(SHTC1_IOCTL_MAGIC, 0x02, unsigned short *)
#define IOCTL_READ_ADC_CHG_DATA	_IOR(SHTC1_IOCTL_MAGIC, 0x03, unsigned short *)
#define IOCTL_READ_THM_SHTC1_DATA	_IOR(SHTC1_IOCTL_MAGIC, 0x04, short *)
#define IOCTL_READ_HUM_SHTC1_DATA	_IOR(SHTC1_IOCTL_MAGIC, 0x05, unsigned short *)
#define IOCTL_READ_THM_BARO_DATA	_IOR(SHTC1_IOCTL_MAGIC, 0x06, unsigned short *)
#define IOCTL_READ_THM_GYRO_DATA	_IOR(SHTC1_IOCTL_MAGIC, 0x07, unsigned short *)

static long ssp_temphumidity_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	struct ssp_data *data
		= container_of(file->private_data,
			struct ssp_data, shtc1_device);

	void __user *argp = (void __user *)arg;
	int retries = 2;
	int length;
	int ret = 0;

	if (data->bulk_buffer == NULL) {
		pr_err("[SSP] %s, buffer is null\n", __func__);
		return -EINVAL;
	}

	length = data->bulk_buffer->len;
	mutex_lock(&data->bulk_temp_read_lock);
	switch (cmd) {
	case IOCTL_READ_COMPLETE: /* free */
		kfree(data->bulk_buffer);
		length = 1;
		break;

	case IOCTL_READ_ADC_BATT_DATA:
		while (retries--) {
			ret = copy_to_user(argp,
				data->bulk_buffer->batt,
				data->bulk_buffer->len*2);
			if (likely(!ret))
				break;
		}
		if (unlikely(ret)) {
			pr_err("[SSP] read bluk adc1 data err(%d)\n", ret);
			goto ioctl_error;
		}
		break;

	case IOCTL_READ_ADC_CHG_DATA:
		while (retries--) {
			ret = copy_to_user(argp,
				data->bulk_buffer->chg,
				data->bulk_buffer->len*2);
			if (likely(!ret))
				break;
		}
		if (unlikely(ret)) {
			pr_err("[SSP] read bluk adc1 data err(%d)\n", ret);
			goto ioctl_error;
		}
		break;

	case IOCTL_READ_THM_SHTC1_DATA:
		while (retries--) {
			ret = copy_to_user(argp,
				data->bulk_buffer->temp,
				data->bulk_buffer->len*2);
			if (likely(!ret))
				break;
		}
		if (unlikely(ret)) {
			pr_err("[SSP] read bluk adc1 data err(%d)\n", ret);
			goto ioctl_error;
		}
		break;

	case IOCTL_READ_HUM_SHTC1_DATA:
		while (retries--) {
			ret = copy_to_user(argp,
				data->bulk_buffer->humidity,
				data->bulk_buffer->len*2);
			if (likely(!ret))
				break;
		}
		if (unlikely(ret)) {
			pr_err("[SSP] read bluk adc1 data err(%d)\n", ret);
			goto ioctl_error;
		}
		break;

	case IOCTL_READ_THM_BARO_DATA:
		while (retries--) {
			ret = copy_to_user(argp,
				data->bulk_buffer->baro,
				data->bulk_buffer->len*2);
			if (likely(!ret))
				break;
		}
		if (unlikely(ret)) {
			pr_err("[SSP] read bluk adc1 data err(%d)\n", ret);
			goto ioctl_error;
		}
		break;

	case IOCTL_READ_THM_GYRO_DATA:
		while (retries--) {
			ret = copy_to_user(argp,
				data->bulk_buffer->gyro,
				data->bulk_buffer->len*2);
			if (likely(!ret))
				break;
		}
		if (unlikely(ret)) {
			pr_err("[SSP] read bluk adc1 data err(%d)\n", ret);
			goto ioctl_error;
		}
		break;

	default:
		pr_err("[SSP] temp ioctl cmd err(%d)\n", cmd);
		ret = EINVAL;
		goto ioctl_error;
	}
	mutex_unlock(&data->bulk_temp_read_lock);
	return length;

ioctl_error:
	mutex_unlock(&data->bulk_temp_read_lock);
	return -ret;
}

static struct file_operations const ssp_temphumidity_fops = {
	.owner = THIS_MODULE,
	.open = nonseekable_open,
	.unlocked_ioctl = ssp_temphumidity_ioctl,
};

static int cp_thm_get_adc_data(struct ssp_data *data)
{
	int adc_data;
	int adc_max = 0;
	int adc_min = 0;
	int adc_total = 0;
	int i;
	int err_value;

	for (i = 0; i < CP_THM_ADC_SAMPLING_CNT; i++) {
		mutex_lock(&data->cp_temp_adc_lock);
		if (data->adc_client)
			adc_data = s3c_adc_read(data->adc_client, data->cp_thm_adc_channel);
		else
			adc_data = 0;
		mutex_unlock(&data->cp_temp_adc_lock);

		if (adc_data < 0) {
			pr_err("[SSP] : %s err(%d) returned, skip read\n",
				__func__, adc_data);
			err_value = adc_data;
			goto err;
		}

		if (i != 0) {
			if (adc_data > adc_max)
				adc_max = adc_data;
			else if (adc_data < adc_min)
				adc_min = adc_data;
		} else {
			adc_max = adc_data;
			adc_min = adc_data;
		}
		adc_total += adc_data;
	}

	return (adc_total - adc_max - adc_min) / (CP_THM_ADC_SAMPLING_CNT - 2);
err:
	return err_value;
}

static int convert_adc_to_temp(struct ssp_data *data, unsigned int adc)
{
	int low = 0, mid = 0;
	int high;

	if (!data->cp_thm_adc_table || !data->cp_thm_adc_arr_size) {
		/* using fake temp */
		return 300;
	}

	high = data->cp_thm_adc_arr_size - 1;

	while (low <= high) {
		mid = (low + high) / 2;
		if (data->cp_thm_adc_table[mid].adc > adc)
			high = mid - 1;
		else if (data->cp_thm_adc_table[mid].adc < adc)
			low = mid + 1;
		else
			break;
	}
	return data->cp_thm_adc_table[mid].temperature;
}


static int hub_thm_get_adc(struct ssp_data *data, u32 channel)
{
	u32 adc = 0;
	int iRet = 0;
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	msg->cmd = MSG2SSP_AP_GET_THERM;
	msg->length = 2;
	msg->options = AP2HUB_READ;
	msg->data = channel;
	msg->buffer = (char *) &adc;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
		return iRet;
	}

	return adc;
}

static int convert_batt_to_temp(struct ssp_data *data, unsigned int adc)
{
	int low = 0, mid = 0;
	int high;

	if (!data->batt_thm_adc_table || !data->batt_thm_adc_arr_size) {
		/* using fake temp */
		return 300;
	}

	high = data->batt_thm_adc_arr_size - 1;

	while (low <= high) {
		mid = (low + high) / 2;
		if (data->batt_thm_adc_table[mid].adc > adc)
			high = mid - 1;
		else if (data->batt_thm_adc_table[mid].adc < adc)
			low = mid + 1;
		else
			break;
	}
	return data->batt_thm_adc_table[mid].temperature;
}

static int convert_chg_to_temp(struct ssp_data *data, unsigned int adc)
{
	int low = 0, mid = 0;
	int high;

	if (!data->chg_thm_adc_table || !data->chg_thm_adc_arr_size) {
		/* using fake temp */
		return 300;
	}

	high = data->chg_thm_adc_arr_size - 1;

	while (low <= high) {
		mid = (low + high) / 2;
		if (data->chg_thm_adc_table[mid].adc > adc)
			high = mid - 1;
		else if (data->chg_thm_adc_table[mid].adc < adc)
			low = mid + 1;
		else
			break;
	}
	return data->chg_thm_adc_table[mid].temperature;
}


static ssize_t temphumidity_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t temphumidity_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static ssize_t engine_version_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	pr_info("[SSP] %s - engine_ver = %s_%s\n",
		__func__, CONFIG_SENSORS_SSP_SHTC1_VER, data->comp_engine_ver);

	return sprintf(buf, "%s_%s\n",
		CONFIG_SENSORS_SSP_SHTC1_VER, data->comp_engine_ver);
}

static ssize_t engine_version_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	kfree(data->comp_engine_ver);
	data->comp_engine_ver =
		    kzalloc(((strlen(buf)+1) * sizeof(char)), GFP_KERNEL);
	strncpy(data->comp_engine_ver, buf, strlen(buf)+1);
	pr_info("[SSP] %s - engine_ver = %s, %s\n",
		__func__, data->comp_engine_ver, buf);

	return size;
}

static ssize_t pam_adc_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int cp_thm_adc = 0;

	if (data->bSspShutdown == false)
		cp_thm_adc = cp_thm_get_adc_data(data);
	else
		pr_info("[SSP] : %s, device is shutting down\n", __func__);

	pr_info("[SSP] %s, adc = %d\n", __func__, cp_thm_adc);
	return sprintf(buf, "%d\n", cp_thm_adc);
}

static ssize_t pam_temp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int adc, temp;

	adc = cp_thm_get_adc_data(data);
	if (adc < 0) {
		pr_err("[SSP] : %s, reading adc failed.(%d)\n", __func__, adc);
		temp = adc;
	} else
		temp = convert_adc_to_temp(data, adc);

	pr_info("[SSP] %s, adc = %d, temp = %d\n", __func__, adc, temp);
	return sprintf(buf, "%d\n", temp);
}

static ssize_t hub_batt_adc_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int adc = 0;

	if (data->bSspShutdown == false)
		adc = hub_thm_get_adc(data, ADC_BATT);
	else
		pr_info("[SSP] : %s, device is shutting down\n", __func__);

	pr_info("[SSP] %s, adc = %d\n", __func__, adc);
	return sprintf(buf, "%d\n", adc);
}

static ssize_t hub_chg_adc_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int adc = 0;

	if (data->bSspShutdown == false)
		adc = hub_thm_get_adc(data, ADC_CHG);
	else
		pr_info("[SSP] : %s, device is shutting down\n", __func__);

	pr_info("[SSP] %s, adc = %d\n", __func__, adc);
	return sprintf(buf, "%d\n", adc);
}

static ssize_t hub_batt_thm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int adc, temp;

	adc = hub_thm_get_adc(data, ADC_BATT);
	if (adc < 0) {
		pr_err("[SSP] %s, reading adc failed.(%d)\n", __func__, adc);
		temp = adc;
	} else {
		temp = convert_batt_to_temp(data, adc);
	}

	pr_info("[SSP] %s, adc = %d, temp = %d\n", __func__, adc, temp);
	return sprintf(buf, "%d\n", temp);
}

static ssize_t hub_chg_thm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int adc, temp;

	adc = hub_thm_get_adc(data, ADC_CHG);
	if (adc < 0) {
		pr_err("[SSP] %s, reading adc failed.(%d)\n", __func__, adc);
		temp = adc;
	} else {
		temp = convert_chg_to_temp(data, adc);
	}

	pr_info("[SSP] %s, adc = %d, temp = %d\n", __func__, adc, temp);
	return sprintf(buf, "%d\n", temp);
}

static ssize_t temphumidity_crc_check(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char chTempBuf = 0xff;
	int iRet = 0;
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	msg->cmd = TEMPHUMIDITY_CRC_FACTORY;
	msg->length = 1;
	msg->options = AP2HUB_READ;
	msg->buffer = &chTempBuf;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - Temphumidity check crc Timeout!! %d\n", __func__,
				iRet);
		goto exit;
	}

	pr_info("[SSP] : %s -Check_CRC : %d\n", __func__,
			chTempBuf);

exit:
	if (chTempBuf == 1)
		return sprintf(buf, "%s\n", "OK");
	else if (chTempBuf == 2)
		return sprintf(buf, "%s\n", "NG_NC");
	else
		return sprintf(buf, "%s\n", "NG");
}

ssize_t temphumidity_send_accuracy(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	u8 accuracy;

	if (kstrtou8(buf, 10, &accuracy) < 0) {
		pr_err("[SSP] %s - read buf is fail(%s)\n", __func__, buf);
		return size;
	}

	if (accuracy == DONE_CAL)
		ssp_send_cmd(data, MSG2SSP_AP_TEMPHUMIDITY_CAL_DONE, 0);
	pr_info("[SSP] %s - accuracy = %d\n", __func__, accuracy);

	return size;
}

static DEVICE_ATTR(name, 0444, temphumidity_name_show, NULL);
static DEVICE_ATTR(vendor, 0444, temphumidity_vendor_show, NULL);
static DEVICE_ATTR(engine_ver, 0664,
	engine_version_show, engine_version_store);
static DEVICE_ATTR(cp_thm, 0444, pam_adc_show, NULL);
static DEVICE_ATTR(cp_temperature, 0444, pam_temp_show, NULL);
static DEVICE_ATTR(mcu_batt_adc, 0444, hub_batt_adc_show, NULL);
static DEVICE_ATTR(mcu_chg_adc, 0444, hub_chg_adc_show, NULL);
static DEVICE_ATTR(batt_temperature, 0444, hub_batt_thm_show, NULL);
static DEVICE_ATTR(chg_temperature, 0444, hub_chg_thm_show, NULL);
static DEVICE_ATTR(crc_check, 0444, temphumidity_crc_check, NULL);
static DEVICE_ATTR(send_accuracy,  0220,
	NULL, temphumidity_send_accuracy);

static struct device_attribute *temphumidity_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_engine_ver,
	&dev_attr_cp_thm,
	&dev_attr_cp_temperature,
	&dev_attr_mcu_batt_adc,
	&dev_attr_mcu_chg_adc,
	&dev_attr_batt_temperature,
	&dev_attr_chg_temperature,
	&dev_attr_crc_check,
	&dev_attr_send_accuracy,
	NULL,
};

void initialize_temphumidity_factorytest(struct ssp_data *data)
{
	int ret;

	/* alloc platform device for adc client */
	data->pdev_pam_temp = platform_device_alloc("pam-temp-adc", -1);
	if (!data->pdev_pam_temp)
		pr_err("%s: could not allocation pam-temp-adc\n", __func__);
#if 0 // Temp block for Helsinki debugging
	data->adc_client = s3c_adc_register(data->pdev_pam_temp, NULL, NULL, 0);
	if (IS_ERR(data->adc_client))
		pr_err("%s, fail to register pam-temp-adc(%ld)\n",
			__func__, IS_ERR(data->adc_client));
#endif
	sensors_register(data->temphumidity_device,
		data, temphumidity_attrs, "temphumidity_sensor");

	data->shtc1_device.minor = MISC_DYNAMIC_MINOR;
	data->shtc1_device.name = "shtc1_sensor";
	data->shtc1_device.fops = &ssp_temphumidity_fops;

	ret = misc_register(&data->shtc1_device);
	if (ret < 0)
		pr_err("register temphumidity misc device err(%d)\n", ret);
}

void remove_temphumidity_factorytest(struct ssp_data *data)
{
#if 0 // Temp block for Helsinki debugging
	if (data->adc_client)
		s3c_adc_release(data->adc_client);
#endif
	if (data->pdev_pam_temp)
		platform_device_put(data->pdev_pam_temp);
	sensors_unregister(data->temphumidity_device, temphumidity_attrs);
	kfree(data->comp_engine_ver);
	ssp_temphumidity_fops.unlocked_ioctl = NULL;
	misc_deregister(&data->shtc1_device);
}
