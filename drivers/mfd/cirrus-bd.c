/*
 * Big-data logging support for Cirrus Logic CS35L41 codec
 *
 * Copyright 2017 Cirrus Logic
 *
 * Author:	David Rhodes	<david.rhodes@cirrus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/firmware.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/power_supply.h>
#include <linux/fs.h>
#include <linux/ktime.h>

#include <linux/mfd/cs35l41/core.h>
#include <linux/mfd/cs35l41/registers.h>
#include <linux/mfd/cs35l41/big_data.h>
#include <linux/mfd/cs35l41/wmfw.h>

#define CIRRUS_BD_VERSION "5.00.15"

#define CIRRUS_BD_CLASS_NAME		"cirrus"
#define CIRRUS_BD_DIR_NAME		"cirrus_bd"

struct cirrus_bd_t {
	struct class *bd_class;
	struct device *dev;
	struct regmap *regmap_left;
	struct regmap *regmap_right;
	unsigned int max_exc_left;
	unsigned int over_exc_count_left;
	unsigned int max_temp_left;
	unsigned int max_temp_keep_left;
	unsigned int over_temp_count_left;
	unsigned int abnm_mute_left;
	unsigned int max_exc_right;
	unsigned int over_exc_count_right;
	unsigned int max_temp_right;
	unsigned int max_temp_keep_right;
	unsigned int over_temp_count_right;
	unsigned int abnm_mute_right;
	unsigned long long int last_update;
};

struct cirrus_bd_t *cirrus_bd;

int cirrus_bd_amp_add(struct regmap *regmap_new, bool right_channel_amp)
{

	if (cirrus_bd){
		if (right_channel_amp)
			cirrus_bd->regmap_right = regmap_new;
		else
			cirrus_bd->regmap_left = regmap_new;
	} else {
		return -EINVAL;
	}

	return 0;
}

void cirrus_bd_store_values_right(void)
{
	unsigned int max_exc_right, over_exc_count_right, max_temp_right,
				over_temp_count_right, abnm_mute_right;

	regmap_read(cirrus_bd->regmap_right, CS35L41_BD_MAX_EXC,
			&max_exc_right);
	regmap_read(cirrus_bd->regmap_right, CS35L41_BD_OVER_EXC_COUNT,
			&over_exc_count_right);
	regmap_read(cirrus_bd->regmap_right, CS35L41_BD_MAX_TEMP,
			&max_temp_right);
	regmap_read(cirrus_bd->regmap_right, CS35L41_BD_OVER_TEMP_COUNT,
			&over_temp_count_right);
	regmap_read(cirrus_bd->regmap_right, CS35L41_BD_ABNORMAL_MUTE,
			&abnm_mute_right);

	if (max_temp_right > (99 * (1 << CS35L41_BD_TEMP_RADIX)) &&
		over_temp_count_right == 0)
		max_temp_right = (99 * (1 << CS35L41_BD_TEMP_RADIX));

	cirrus_bd->over_temp_count_right += over_temp_count_right;
	cirrus_bd->over_exc_count_right += over_exc_count_right;
	if (max_exc_right > cirrus_bd->max_exc_right)
		cirrus_bd->max_exc_right = max_exc_right;
	if (max_temp_right > cirrus_bd->max_temp_right)
		cirrus_bd->max_temp_right = max_temp_right;

	cirrus_bd->max_temp_keep_right = cirrus_bd->max_temp_right;

	cirrus_bd->abnm_mute_right += abnm_mute_right;

	cirrus_bd->last_update = ktime_to_ns(ktime_get());

	dev_info(cirrus_bd->dev, "Values stored:\n");
	dev_info(cirrus_bd->dev, "Max Excursion Right:\t\t%d.%d\n",
				cirrus_bd->max_exc_right >> CS35L41_BD_EXC_RADIX,
				(cirrus_bd->max_exc_right &
				(((1 << CS35L41_BD_EXC_RADIX) - 1))) *
				10000 / (1 << CS35L41_BD_EXC_RADIX));
	dev_info(cirrus_bd->dev, "Over Excursion Count Right:\t%d\n",
				cirrus_bd->over_exc_count_right);
	dev_info(cirrus_bd->dev, "Max Temp Right:\t\t\t%d.%d\n",
				cirrus_bd->max_temp_right >> CS35L41_BD_TEMP_RADIX,
				(cirrus_bd->max_temp_right &
				(((1 << CS35L41_BD_TEMP_RADIX) - 1))) *
				10000 / (1 << CS35L41_BD_TEMP_RADIX));
	dev_info(cirrus_bd->dev, "Over Temp Count Right:\t\t%d\n",
				cirrus_bd->over_temp_count_right);
	dev_info(cirrus_bd->dev, "Abnormal Mute Right:\t\t%d\n",
				cirrus_bd->abnm_mute_right);
	dev_info(cirrus_bd->dev, "Timestamp:\t\t%llu\n",
				cirrus_bd->last_update);

	regmap_write(cirrus_bd->regmap_right, CS35L41_BD_MAX_EXC, 0);
	regmap_write(cirrus_bd->regmap_right, CS35L41_BD_OVER_EXC_COUNT, 0);
	regmap_write(cirrus_bd->regmap_right, CS35L41_BD_MAX_TEMP, 0);
	regmap_write(cirrus_bd->regmap_right, CS35L41_BD_OVER_TEMP_COUNT, 0);
	regmap_write(cirrus_bd->regmap_right, CS35L41_BD_ABNORMAL_MUTE, 0);

}
EXPORT_SYMBOL_GPL(cirrus_bd_store_values_right);

void cirrus_bd_store_values_left(void)
{
	unsigned int max_exc_left, over_exc_count_left, max_temp_left,
				over_temp_count_left, abnm_mute_left;

	regmap_read(cirrus_bd->regmap_left, CS35L41_BD_MAX_EXC,
			&max_exc_left);
	regmap_read(cirrus_bd->regmap_left, CS35L41_BD_OVER_EXC_COUNT,
			&over_exc_count_left);
	regmap_read(cirrus_bd->regmap_left, CS35L41_BD_MAX_TEMP,
			&max_temp_left);
	regmap_read(cirrus_bd->regmap_left, CS35L41_BD_OVER_TEMP_COUNT,
			&over_temp_count_left);
	regmap_read(cirrus_bd->regmap_left, CS35L41_BD_ABNORMAL_MUTE,
			&abnm_mute_left);

	if (max_temp_left > (99 * (1 << CS35L41_BD_TEMP_RADIX)) &&
		over_temp_count_left == 0)
		max_temp_left = (99 * (1 << CS35L41_BD_TEMP_RADIX));

	cirrus_bd->over_temp_count_left += over_temp_count_left;
	cirrus_bd->over_exc_count_left += over_exc_count_left;
	if (max_exc_left > cirrus_bd->max_exc_left)
		cirrus_bd->max_exc_left = max_exc_left;
	if (max_temp_left > cirrus_bd->max_temp_left)
		cirrus_bd->max_temp_left = max_temp_left;

	cirrus_bd->max_temp_keep_left = cirrus_bd->max_temp_left;

	cirrus_bd->abnm_mute_left += abnm_mute_left;

	cirrus_bd->last_update = ktime_to_ns(ktime_get());

	dev_info(cirrus_bd->dev, "Values stored:\n");
	dev_info(cirrus_bd->dev, "Max Excursion Left:\t\t%d.%d\n",
				cirrus_bd->max_exc_left >> CS35L41_BD_EXC_RADIX,
				(cirrus_bd->max_exc_left &
				(((1 << CS35L41_BD_EXC_RADIX) - 1))) *
				10000 / (1 << CS35L41_BD_EXC_RADIX));
	dev_info(cirrus_bd->dev, "Over Excursion Count Left:\t%d\n",
				cirrus_bd->over_exc_count_left);
	dev_info(cirrus_bd->dev, "Max Temp Left:\t\t\t%d.%d\n",
				cirrus_bd->max_temp_left >> CS35L41_BD_TEMP_RADIX,
				(cirrus_bd->max_temp_left &
				(((1 << CS35L41_BD_TEMP_RADIX) - 1))) *
				10000 / (1 << CS35L41_BD_TEMP_RADIX));
	dev_info(cirrus_bd->dev, "Over Temp Count Left:\t\t%d\n",
				cirrus_bd->over_temp_count_left);
	dev_info(cirrus_bd->dev, "Abnormal Mute Left:\t\t%d\n",
				cirrus_bd->abnm_mute_left);
	dev_info(cirrus_bd->dev, "Timestamp:\t\t%llu\n",
				cirrus_bd->last_update);

	regmap_write(cirrus_bd->regmap_left, CS35L41_BD_MAX_EXC, 0);
	regmap_write(cirrus_bd->regmap_left, CS35L41_BD_OVER_EXC_COUNT, 0);
	regmap_write(cirrus_bd->regmap_left, CS35L41_BD_MAX_TEMP, 0);
	regmap_write(cirrus_bd->regmap_left, CS35L41_BD_OVER_TEMP_COUNT, 0);
	regmap_write(cirrus_bd->regmap_left, CS35L41_BD_ABNORMAL_MUTE, 0);

}
EXPORT_SYMBOL_GPL(cirrus_bd_store_values_left);

/***** SYSFS Interfaces *****/

static ssize_t cirrus_bd_version_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, CIRRUS_BD_VERSION "\n");
}

static ssize_t cirrus_bd_version_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_max_exc_left_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret = sprintf(buf, "%d.%d\n",
			cirrus_bd->max_exc_left >> CS35L41_BD_EXC_RADIX,
			(cirrus_bd->max_exc_left &
			(((1 << CS35L41_BD_EXC_RADIX) - 1))) *
			10000 / (1 << CS35L41_BD_EXC_RADIX));

	cirrus_bd->max_exc_left = 0;
	return ret;
}

static ssize_t cirrus_bd_max_exc_left_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_over_exc_count_left_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret = sprintf(buf, "%d\n", cirrus_bd->over_exc_count_left);

	cirrus_bd->over_exc_count_left = 0;
	return ret;
}

static ssize_t cirrus_bd_over_exc_count_left_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_max_temp_left_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret = sprintf(buf, "%d.%d\n",
			cirrus_bd->max_temp_left >> CS35L41_BD_TEMP_RADIX,
			(cirrus_bd->max_temp_left &
			(((1 << CS35L41_BD_TEMP_RADIX) - 1))) *
			10000 / (1 << CS35L41_BD_TEMP_RADIX));

	cirrus_bd->max_temp_left = 0;
	return ret;
}

static ssize_t cirrus_bd_max_temp_left_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_over_temp_count_left_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret = sprintf(buf, "%d\n", cirrus_bd->over_temp_count_left);

	cirrus_bd->over_temp_count_left = 0;
	return ret;
}

static ssize_t cirrus_bd_over_temp_count_left_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_abnm_mute_left_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret = sprintf(buf, "%d\n", cirrus_bd->abnm_mute_left);

	cirrus_bd->abnm_mute_left = 0;
	return ret;
}

static ssize_t cirrus_bd_abnm_mute_left_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}


static ssize_t cirrus_bd_max_exc_right_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret = sprintf(buf, "%d.%d\n",
			cirrus_bd->max_exc_right >> CS35L41_BD_EXC_RADIX,
			(cirrus_bd->max_exc_right &
			(((1 << CS35L41_BD_EXC_RADIX) - 1))) *
			10000 / (1 << CS35L41_BD_EXC_RADIX));

	cirrus_bd->max_exc_right = 0;
	return ret;
}

static ssize_t cirrus_bd_max_exc_right_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_over_exc_count_right_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret = sprintf(buf, "%d\n", cirrus_bd->over_exc_count_right);

	cirrus_bd->over_exc_count_right = 0;
	return ret;
}

static ssize_t cirrus_bd_over_exc_count_right_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_max_temp_right_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret = sprintf(buf, "%d.%d\n",
			cirrus_bd->max_temp_right >> CS35L41_BD_TEMP_RADIX,
			(cirrus_bd->max_temp_right &
			(((1 << CS35L41_BD_TEMP_RADIX) - 1))) *
			10000 / (1 << CS35L41_BD_TEMP_RADIX));

	cirrus_bd->max_temp_right = 0;
	return ret;
}

static ssize_t cirrus_bd_max_temp_right_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_over_temp_count_right_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret = sprintf(buf, "%d\n", cirrus_bd->over_temp_count_right);

	cirrus_bd->over_temp_count_right = 0;
	return ret;
}

static ssize_t cirrus_bd_over_temp_count_right_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_abnm_mute_right_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret = sprintf(buf, "%d\n", cirrus_bd->abnm_mute_right);

	cirrus_bd->abnm_mute_right = 0;
	return ret;
}

static ssize_t cirrus_bd_abnm_mute_right_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_max_temp_keep_left_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret = sprintf(buf, "%d.%d\n",
			cirrus_bd->max_temp_keep_left >>
					CS35L41_BD_TEMP_RADIX,
			(cirrus_bd->max_temp_keep_left &
			(((1 << CS35L41_BD_TEMP_RADIX) - 1))) *
			10000 / (1 << CS35L41_BD_TEMP_RADIX));

	return ret;
}

static ssize_t cirrus_bd_max_temp_keep_left_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_max_temp_keep_right_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret = sprintf(buf, "%d.%d\n",
			cirrus_bd->max_temp_keep_right >>
					CS35L41_BD_TEMP_RADIX,
			(cirrus_bd->max_temp_keep_right &
			(((1 << CS35L41_BD_TEMP_RADIX) - 1))) *
			10000 / (1 << CS35L41_BD_TEMP_RADIX));

	return ret;
}

static ssize_t cirrus_bd_max_temp_keep_right_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_bd_store_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return 0;
}

static ssize_t cirrus_bd_store_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	int store;
	int ret = kstrtos32(buf, 10, &store);

	if (ret == 0) {
		if (store == 1)
			cirrus_bd_store_values_left();
	}

	return size;
}

static ssize_t cirrus_bd_store_r_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return 0;
}

static ssize_t cirrus_bd_store_r_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	int store;
	int ret = kstrtos32(buf, 10, &store);

	if (ret == 0) {
		if (store == 1)
			cirrus_bd_store_values_right();
	}

	return size;
}

static DEVICE_ATTR(version, 0444, cirrus_bd_version_show,
				cirrus_bd_version_store);
static DEVICE_ATTR(max_exc_left, 0444, cirrus_bd_max_exc_left_show,
				cirrus_bd_max_exc_left_store);
static DEVICE_ATTR(over_exc_count_left, 0444, cirrus_bd_over_exc_count_left_show,
				cirrus_bd_over_exc_count_left_store);
static DEVICE_ATTR(max_temp_left, 0444, cirrus_bd_max_temp_left_show,
				cirrus_bd_max_temp_left_store);
static DEVICE_ATTR(over_temp_count_left, 0444, cirrus_bd_over_temp_count_left_show,
				cirrus_bd_over_temp_count_left_store);
static DEVICE_ATTR(abnm_mute_left, 0444, cirrus_bd_abnm_mute_left_show,
				cirrus_bd_abnm_mute_left_store);
static DEVICE_ATTR(max_exc_right, 0444, cirrus_bd_max_exc_right_show,
				cirrus_bd_max_exc_right_store);
static DEVICE_ATTR(over_exc_count_right, 0444, cirrus_bd_over_exc_count_right_show,
				cirrus_bd_over_exc_count_right_store);
static DEVICE_ATTR(max_temp_right, 0444, cirrus_bd_max_temp_right_show,
				cirrus_bd_max_temp_right_store);
static DEVICE_ATTR(over_temp_count_right, 0444, cirrus_bd_over_temp_count_right_show,
				cirrus_bd_over_temp_count_right_store);
static DEVICE_ATTR(abnm_mute_right, 0444, cirrus_bd_abnm_mute_right_show,
				cirrus_bd_abnm_mute_right_store);
static DEVICE_ATTR(max_temp_keep_left, 0444,
				cirrus_bd_max_temp_keep_left_show,
				cirrus_bd_max_temp_keep_left_store);
static DEVICE_ATTR(max_temp_keep_right, 0444,
				cirrus_bd_max_temp_keep_right_show,
				cirrus_bd_max_temp_keep_right_store);
static DEVICE_ATTR(store, 0644, cirrus_bd_store_show,
				cirrus_bd_store_store);
static DEVICE_ATTR(store_r, 0644, cirrus_bd_store_r_show,
				cirrus_bd_store_r_store);

static struct attribute *cirrus_bd_attr[] = {
	&dev_attr_version.attr,
	&dev_attr_max_exc_left.attr,
	&dev_attr_over_exc_count_left.attr,
	&dev_attr_max_temp_left.attr,
	&dev_attr_over_temp_count_left.attr,
	&dev_attr_abnm_mute_left.attr,
	&dev_attr_max_exc_right.attr,
	&dev_attr_over_exc_count_right.attr,
	&dev_attr_max_temp_right.attr,
	&dev_attr_over_temp_count_right.attr,
	&dev_attr_abnm_mute_right.attr,
	&dev_attr_max_temp_keep_left.attr,
	&dev_attr_max_temp_keep_right.attr,
	&dev_attr_store.attr,
	&dev_attr_store_r.attr,
	NULL,
};

static struct attribute_group cirrus_bd_attr_grp = {
	.attrs = cirrus_bd_attr,
};


int __init cirrus_bd_init(struct class *cirrus_amp_class)
{
	int ret;

	cirrus_bd = kzalloc(sizeof(struct cirrus_bd_t), GFP_KERNEL);
	if (cirrus_bd == NULL)
		return -ENOMEM;

	cirrus_bd->bd_class = cirrus_amp_class;

	cirrus_bd->dev = device_create(cirrus_bd->bd_class, NULL, 1, NULL,
						CIRRUS_BD_DIR_NAME);
	if (IS_ERR(cirrus_bd->dev)) {
		ret = PTR_ERR(cirrus_bd->dev);
		pr_err("Failed to create cirrus_bd device\n");
		class_destroy(cirrus_bd->bd_class);
		goto err;
	}

	ret = sysfs_create_group(&cirrus_bd->dev->kobj, &cirrus_bd_attr_grp);
	if (ret) {
		dev_err(cirrus_bd->dev, "Failed to create sysfs group\n");
		goto err;
	}

	return 0;
err:
	kfree(cirrus_bd);
	return ret;
}

void cirrus_bd_exit(void)
{
	kfree(cirrus_bd);
}

