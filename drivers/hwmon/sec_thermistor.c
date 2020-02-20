/*
 * sec_thermistor.c - SEC Thermistor
 *
 *  Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *  Minsung Kim <ms925.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/iio/consumer.h>
#include <linux/platform_data/sec_thermistor.h>
#include <linux/sec_class.h>

#define ADC_SAMPLING_CNT	5

struct sec_therm_info {
	int id;
	struct device *dev;
	struct device *sec_dev;
	struct device *hwmon_dev;
	struct sec_therm_platform_data *pdata;
	struct iio_channel *chan;
	char name[PLATFORM_NAME_SIZE];
	char hwmon_name[PLATFORM_NAME_SIZE];
	struct device_node *np;
};

#ifdef CONFIG_OF
static const struct of_device_id sec_therm_match[] = {
	{ .compatible = "samsung,sec-thermistor", },
	{ },
};
MODULE_DEVICE_TABLE(of, sec_therm_match);

static int sec_therm_parse_dt(struct platform_device *pdev)
{
	struct sec_therm_info *info = platform_get_drvdata(pdev);
	struct sec_therm_platform_data *pdata;
	const char *name;
	int adc_arr_len, temp_arr_len;
	int i;
	u32 adc, tp;

	if (!info || !pdev->dev.of_node)
		return -ENODEV;

	info->np = pdev->dev.of_node;

	if (of_property_read_u32(info->np, "id", &info->id)) {
		dev_err(info->dev, "failed to get thermistor ID\n");
		return -EINVAL;
	}

	if (!of_property_read_string(info->np, "thermistor_name", &name)) {
		strlcpy(info->name, name, sizeof(info->name));
	} else {
		dev_err(info->dev, "failed to get thermistor name\n");
		return -EINVAL;
	}

	pdata = devm_kzalloc(info->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	if (!of_get_property(info->np, "adc_array", &adc_arr_len))
		return -ENOENT;
	if (!of_get_property(info->np, "temp_array", &temp_arr_len))
		return -ENOENT;

	if (adc_arr_len != temp_arr_len) {
		dev_err(info->dev, "%s: invalid array length(%u,%u)\n",
				__func__, adc_arr_len, temp_arr_len);
		return -EINVAL;
	}

	pdata->adc_arr_size = adc_arr_len / sizeof(u32);
	pdata->adc_table = devm_kzalloc(&pdev->dev,
			sizeof(*pdata->adc_table) * pdata->adc_arr_size,
			GFP_KERNEL);
	if (!pdata->adc_table)
		return -ENOMEM;

	for (i = 0; i < pdata->adc_arr_size; i++) {
		if (of_property_read_u32_index(info->np, "adc_array", i, &adc))
			return -EINVAL;
		if (of_property_read_u32_index(info->np, "temp_array", i, &tp))
			return -EINVAL;

		pdata->adc_table[i].adc = (int)adc;
		pdata->adc_table[i].temperature = (int)tp;
	}

	info->pdata = pdata;

	return 0;
}
#else
static int sec_therm_parse_dt(struct platform_device *pdev) { return -ENODEV; }
#endif

static int sec_therm_get_adc_data(struct sec_therm_info *info)
{
	int adc_data;
	int adc_max = 0, adc_min = 0, adc_total = 0;
	int i;

	for (i = 0; i < ADC_SAMPLING_CNT; i++) {
		int ret = iio_read_channel_raw(info->chan, &adc_data);

		if (ret < 0) {
			dev_err(info->dev, "%s : err(%d) returned, skip read\n",
				__func__, adc_data);
			return ret;
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

	return (adc_total - adc_max - adc_min) / (ADC_SAMPLING_CNT - 2);
}

static int convert_adc_to_temper(struct sec_therm_info *info, unsigned int adc)
{
	int low = 0;
	int high = 0;
	int temp = 0;
	int temp2 = 0;

	if (!info->pdata->adc_table || !info->pdata->adc_arr_size) {
		/* using fake temp */
		return 300;
	}

	high = info->pdata->adc_arr_size - 1;

	if (info->pdata->adc_table[low].adc >= adc)
		return info->pdata->adc_table[low].temperature;
	else if (info->pdata->adc_table[high].adc <= adc)
		return info->pdata->adc_table[high].temperature;

	while (low <= high) {
		int mid = 0;

		mid = (low + high) / 2;
		if (info->pdata->adc_table[mid].adc > adc)
			high = mid - 1;
		else if (info->pdata->adc_table[mid].adc < adc)
			low = mid + 1;
		else
			return info->pdata->adc_table[mid].temperature;
	}

	temp = info->pdata->adc_table[high].temperature;

	temp2 = (info->pdata->adc_table[low].temperature -
			info->pdata->adc_table[high].temperature) *
			(adc - info->pdata->adc_table[high].adc);

	temp += temp2 /
		(info->pdata->adc_table[low].adc -
			info->pdata->adc_table[high].adc);

	return temp;
}

static ssize_t sec_therm_show_temperature(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct sec_therm_info *info = dev_get_drvdata(dev);
	int adc, temp;

	adc = sec_therm_get_adc_data(info);

	if (adc >= 0)
		temp = convert_adc_to_temper(info, adc);
	else
		return adc;

	return sprintf(buf, "%d\n", temp);
}

static ssize_t sec_therm_show_temp_adc(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct sec_therm_info *info = dev_get_drvdata(dev);
	int adc;

	adc = sec_therm_get_adc_data(info);

	return sprintf(buf, "%d\n", adc);
}

static ssize_t sec_therm_show_name(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_therm_info *info = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", info->name);
}

static SENSOR_DEVICE_ATTR(temperature, 0444, sec_therm_show_temperature,
		NULL, 0);
static SENSOR_DEVICE_ATTR(temp_adc, 0444, sec_therm_show_temp_adc, NULL, 0);

static struct attribute *sec_therm_hwmon_attrs[] = {
	&sensor_dev_attr_temperature.dev_attr.attr,
	&sensor_dev_attr_temp_adc.dev_attr.attr,
	NULL
};
ATTRIBUTE_GROUPS(sec_therm_hwmon);

static DEVICE_ATTR(temperature, 0444, sec_therm_show_temperature, NULL);
static DEVICE_ATTR(temp_adc, 0444, sec_therm_show_temp_adc, NULL);
static DEVICE_ATTR(name, 0444, sec_therm_show_name, NULL);

static struct attribute *sec_therm_attrs[] = {
	&dev_attr_temperature.attr,
	&dev_attr_temp_adc.attr,
	&dev_attr_name.attr,
	NULL
};

static const struct attribute_group sec_therm_group = {
	.attrs = sec_therm_attrs,
};

static struct sec_therm_info *g_ap_therm_info;
int sec_therm_get_ap_temperature(void)
{
	int adc;
	int temp;

	if (unlikely(!g_ap_therm_info))
		return -ENODEV;

	adc = sec_therm_get_adc_data(g_ap_therm_info);

	if (adc >= 0)
		temp = convert_adc_to_temper(g_ap_therm_info, adc);
	else
		return adc;

	return temp;
}

static int sec_therm_probe(struct platform_device *pdev)
{
	struct sec_therm_info *info;
	int ret;
	char name[PLATFORM_NAME_SIZE];

	dev_dbg(&pdev->dev, "%s: SEC Thermistor Driver Loading\n", __func__);

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	platform_set_drvdata(pdev, info);
	info->dev = &pdev->dev;

	ret = sec_therm_parse_dt(pdev);
	if (ret) {
		dev_err(info->dev, "%s: fail to parse dt\n", __func__);
		return ret;
	}

	info->chan = iio_channel_get(info->dev, NULL);
	if (IS_ERR(info->chan)) {
		dev_err(info->dev, "%s: fail to get iio channel\n", __func__);
		return PTR_ERR(info->chan);
	}

	info->sec_dev = sec_device_create(info, info->name);
	if (IS_ERR(info->sec_dev)) {
		dev_err(info->dev, "%s: fail to create sec_dev\n", __func__);
		return PTR_ERR(info->sec_dev);
	}

	ret = sysfs_create_group(&info->sec_dev->kobj, &sec_therm_group);
	if (ret) {
		dev_err(info->dev, "failed to create sysfs group\n");
		goto err_create_sysfs;
	}

	if (sscanf(info->name, "sec-%s", name)) {
		char *token;
		char *str = name;
		token = strsep(&str, "-");
		strncpy(info->hwmon_name, token, PLATFORM_NAME_SIZE - 1);
	} else {
		dev_err(info->dev, "failed to sscanf hwmon_name\n");
		goto err_register_hwmon;
	}

	info->hwmon_dev = devm_hwmon_device_register_with_groups(info->dev,
			info->hwmon_name, info, sec_therm_hwmon_groups);

	if (IS_ERR(info->hwmon_dev)) {
		dev_err(info->dev, "unable to register as hwmon device.\n");
		ret = PTR_ERR(info->hwmon_dev);
		goto err_register_hwmon;
	}

	if (info->id == 0)
		g_ap_therm_info = info;

	dev_info(info->dev, "%s successfully probed.\n", info->name);

	return 0;

err_register_hwmon:
	sysfs_remove_group(&info->sec_dev->kobj, &sec_therm_group);
err_create_sysfs:
	sec_device_destroy(info->sec_dev->devt);
	return ret;
}

static int sec_therm_remove(struct platform_device *pdev)
{
	struct sec_therm_info *info = platform_get_drvdata(pdev);

	if (!info)
		return 0;

	if (info->id == 0)
		g_ap_therm_info = NULL;

	sysfs_remove_group(&info->sec_dev->kobj, &sec_therm_group);
	iio_channel_release(info->chan);
	sec_device_destroy(info->sec_dev->devt);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver sec_thermistor_driver = {
	.driver = {
		.name = "sec-thermistor",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(sec_therm_match),
	},
	.probe = sec_therm_probe,
	.remove = sec_therm_remove,
};

module_platform_driver(sec_thermistor_driver);

MODULE_DESCRIPTION("SEC Thermistor Driver");
MODULE_AUTHOR("Minsung Kim <ms925.kim@samsung.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sec-thermistor");
