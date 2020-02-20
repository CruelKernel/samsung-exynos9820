/*
 * Power-management support for Cirrus Logic CS35L41 amplifier
 *
 * Copyright 2018 Cirrus Logic
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
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <linux/ktime.h>

#include <linux/mfd/cs35l41/core.h>
#include <linux/mfd/cs35l41/registers.h>
#include <linux/mfd/cs35l41/power.h>

#define CIRRUS_PWR_VERSION "5.01.3"

#define CIRRUS_PWR_CLASS_NAME		"cirrus"
#define CIRRUS_PWR_DIR_NAME		"cirrus_pwr"
#define CIRRUS_PWR_WORKQ_NAME		"cirrus_pwr_wq"

#define CIRRUS_PWR_STATUS_DISABLED	0
#define	CIRRUS_PWR_STATUS_ENABLED	1
#define CIRRUS_PWR_STATUS_ERROR		3

#define CIRRUS_PWR_AMB_TEMP_OFFSET	500
#define CIRRUS_PWR_SCALING_Q15		846397

struct cirrus_pwr_t {
	struct class *pwr_class;
	struct device *dev;
	struct regmap *regmap_left;
	struct regmap *regmap_right;
	struct mutex pwr_lock;
	struct delayed_work pwr_work;
	struct workqueue_struct *pwr_workqueue;
	unsigned int uptime_ms;
	unsigned int interval;
	unsigned int status;
	unsigned int target_min_time_ms;
	unsigned int target_temp_left;
	unsigned int target_temp_right;
	unsigned int exit_temp_left;
	unsigned int exit_temp_right;
	unsigned int amb_temp_left;
	unsigned int amb_temp_right;
	unsigned int spk_temp_left;
	unsigned int spk_temp_right;
	unsigned int passport_enable_left;
	unsigned int passport_enable_right;
	unsigned int global_enable;
	bool amp_right_active;
	bool amp_left_active;
};

static struct cirrus_pwr_t *cirrus_pwr;

static unsigned int sqrt_q24(unsigned long int x)
{
	u32 root, remHi, remLo, testDiv, count;

	root = 0;
	remHi = 0;
	remLo = x;
	count = 24;

	do {
		remHi = (remHi << 2) | (remLo >> 30);
		remLo <<= 2;
		root <<= 1;
		testDiv = (root << 1) + 1;
		if (remHi >= testDiv) {
			remHi -= testDiv;
			root++;
		}
	} while (count-- != 0);

	return root; /* Q21 result */
}

static unsigned int convert_power(unsigned int power_squared)
{
	unsigned long long int power;

	power = sqrt_q24(power_squared*2);
	power *= CIRRUS_PWR_SCALING_Q15;

	dev_dbg(cirrus_pwr->dev,
			"converted power (%d W^2): %llu.%04llu W\n",
			power_squared,
			power >> 36,
			(power & (((1ull << 36) - 1ull))) *
			    10000 / (1ull << 36));

	power *= 1000;
	power >>= 28;

	dev_dbg(cirrus_pwr->dev,
		"converted power q8 mW: %d mW = 0x%x\n",
		(unsigned int)(power / 256), (unsigned int)(power));

	return (unsigned int)power;
}

int cirrus_pwr_amp_add(struct regmap *regmap_new, bool right_channel_amp)
{
	if (cirrus_pwr) {
		dev_err(cirrus_pwr->dev, "%s\n", __func__);
		if (right_channel_amp)
			cirrus_pwr->regmap_right = regmap_new;
		else
			cirrus_pwr->regmap_left = regmap_new;
	} else {
		return -EINVAL;
	}

	return 0;
}


int cirrus_pwr_set_params(bool global_enable, bool right_channel_amp,
			unsigned int target_temp, unsigned int exit_temp)
{
	cirrus_pwr->global_enable = global_enable;
	if (right_channel_amp) {
		cirrus_pwr->target_temp_right = target_temp;
		cirrus_pwr->exit_temp_right = exit_temp;
	} else {
		cirrus_pwr->target_temp_left = target_temp;
		cirrus_pwr->exit_temp_left = exit_temp;
	}

	dev_info(cirrus_pwr->dev,
		"%s: global enable = %d,%s, target temp = %d, exit temp = %d\n",
		__func__, global_enable,
		(right_channel_amp ? " right amp" : " left amp"),
		target_temp, exit_temp);

	return 0;
}

static void cirrus_pwr_passport_enable(struct regmap *regmap_enable,
							bool enable)
{
	if (regmap_enable)
		regmap_write(regmap_enable,
			CIRRUS_PWR_CSPL_PASSPORT_ENABLE,
			(uint)enable);
}

void cirrus_pwr_start(bool right_amp)
{
	if (right_amp)
		cirrus_pwr->amp_right_active = 1;
	else
		cirrus_pwr->amp_left_active = 1;

	if (!cirrus_pwr->global_enable)
		return;

	mutex_lock(&cirrus_pwr->pwr_lock);

	if (cirrus_pwr->status == CIRRUS_PWR_STATUS_ENABLED) {
		/* State machine already active on one amp */
		dev_dbg(cirrus_pwr->dev,
			"cirrus_pwr_start(), additional amp activated");
	} else {
		/* Init state machine */
		dev_dbg(cirrus_pwr->dev,
			"cirrus_pwr_start() Entering wait period.\n");
		cirrus_pwr->status = CIRRUS_PWR_STATUS_ENABLED;

		/* Queue state machine operation */
		queue_delayed_work(cirrus_pwr->pwr_workqueue,
				   &cirrus_pwr->pwr_work,
				   msecs_to_jiffies(cirrus_pwr->interval));
	}

	mutex_unlock(&cirrus_pwr->pwr_lock);
}
EXPORT_SYMBOL_GPL(cirrus_pwr_start);

void cirrus_pwr_stop(bool right_amp)
{
	if (right_amp)
		cirrus_pwr->amp_right_active = 0;
	else
		cirrus_pwr->amp_left_active = 0;

	if (!cirrus_pwr->global_enable)
		return;

	mutex_lock(&cirrus_pwr->pwr_lock);

	if (cirrus_pwr->amp_right_active ||
		cirrus_pwr->amp_left_active) {
		/* One amp still active */
		dev_dbg(cirrus_pwr->dev,
			"Amp %s deactivated\n",
			(right_amp) ? "Right" : "Left");
	} else {
		/* Exit state machine */
		dev_dbg(cirrus_pwr->dev,
			"cirrus_pwr_stop(). Disabling PASSPORT\n");
		cirrus_pwr_passport_enable(cirrus_pwr->regmap_right, false);
		cirrus_pwr_passport_enable(cirrus_pwr->regmap_left, false);
		cirrus_pwr->passport_enable_right = 0;
		cirrus_pwr->passport_enable_left = 0;

		/* Reset state machine variables */
		cirrus_pwr->uptime_ms = 0;
		cirrus_pwr->status = CIRRUS_PWR_STATUS_DISABLED;

		/* cancel workqueue */
		if (delayed_work_pending(&cirrus_pwr->pwr_work))
			cancel_delayed_work(&cirrus_pwr->pwr_work);
	}

	mutex_unlock(&cirrus_pwr->pwr_lock);
}
EXPORT_SYMBOL_GPL(cirrus_pwr_stop);

static void cirrus_pwr_work(struct work_struct *work)
{
	mutex_lock(&cirrus_pwr->pwr_lock);

	/* Run state machine and enable/disable Passport accordingly */

	switch (cirrus_pwr->status) {
	case CIRRUS_PWR_STATUS_ENABLED:
		cirrus_pwr->uptime_ms += cirrus_pwr->interval;

		if (cirrus_pwr->uptime_ms <= cirrus_pwr->target_min_time_ms) {
			dev_dbg(cirrus_pwr->dev,
				"Waiting for min time... (%d / %d ms)\n",
				cirrus_pwr->uptime_ms,
				cirrus_pwr->target_min_time_ms);
			break;
		}

		/* Enabled and > min time */
		/* Evaluate temp for each amp and enable/disable Passport */

		dev_dbg(cirrus_pwr->dev, "Right Amp\n");
		dev_dbg(cirrus_pwr->dev, "Spk Temp:\t%d.%d C\t(Target: %d.%d C)\n",
				cirrus_pwr->spk_temp_right / 100,
				cirrus_pwr->spk_temp_right % 100,
				cirrus_pwr->target_temp_right / 100,
				cirrus_pwr->target_temp_right % 100);
		dev_dbg(cirrus_pwr->dev, "Amb Temp:\t%d.%d\n",
				cirrus_pwr->amb_temp_right / 100,
				cirrus_pwr->amb_temp_right % 100);

		dev_dbg(cirrus_pwr->dev, "Left Amp\n");
		dev_dbg(cirrus_pwr->dev, "Spk Temp:\t%d.%d C\t(Target: %d.%d C)\n",
				cirrus_pwr->spk_temp_left / 100,
				cirrus_pwr->spk_temp_left % 100,
				cirrus_pwr->target_temp_left / 100,
				cirrus_pwr->target_temp_left % 100);
		dev_dbg(cirrus_pwr->dev, "Amb Temp:\t%d.%d\n",
				cirrus_pwr->amb_temp_left / 100,
				cirrus_pwr->amb_temp_left % 100);

		if (cirrus_pwr->amp_right_active) {
			if (cirrus_pwr->passport_enable_right) {
				/* Evaluate exit criteria */
				if (cirrus_pwr->spk_temp_right <
					cirrus_pwr->exit_temp_right) {
					cirrus_pwr_passport_enable(
						cirrus_pwr->regmap_right,
						false);
					dev_info(cirrus_pwr->dev,
					"Right Amp below exit temp. Disabling PASSPORT\n");
					cirrus_pwr->passport_enable_right = 0;
				}
			} else {
				/* Evaluate entry criteria */
				if ((cirrus_pwr->amb_temp_right +
						CIRRUS_PWR_AMB_TEMP_OFFSET <
						cirrus_pwr->spk_temp_right) &&
					(cirrus_pwr->spk_temp_right >
					cirrus_pwr->target_temp_right)) {
					cirrus_pwr_passport_enable(
						cirrus_pwr->regmap_right, true);
					dev_info(cirrus_pwr->dev,
					"Right Amp above target temp and ambient + 5.\n");
					dev_info(cirrus_pwr->dev, "Enabling PASSPORT\n");
					cirrus_pwr->passport_enable_right = 1;
				}

			}
		}

		if (cirrus_pwr->amp_left_active) {
			if (cirrus_pwr->passport_enable_left) {
				/* Evaluate exit criteria */
				if (cirrus_pwr->spk_temp_left <
					cirrus_pwr->exit_temp_left) {
					cirrus_pwr_passport_enable(
						cirrus_pwr->regmap_left, false);
					dev_info(cirrus_pwr->dev,
					"Left Amp below exit temp. Disabling PASSPORT\n");
					cirrus_pwr->passport_enable_left = 0;
				}
			} else {
				/* Evaluate entry criteria */
				if ((cirrus_pwr->amb_temp_left +
						CIRRUS_PWR_AMB_TEMP_OFFSET <
						cirrus_pwr->spk_temp_left) &&
					(cirrus_pwr->spk_temp_left >
						cirrus_pwr->target_temp_left)) {
					cirrus_pwr_passport_enable(
						cirrus_pwr->regmap_left, true);
					dev_info(cirrus_pwr->dev,
					"Left Amp above target temp and ambient + 5.\n");
					dev_info(cirrus_pwr->dev, "Enabling PASSPORT\n");
					cirrus_pwr->passport_enable_left = 1;
				}
			}
		}

		dev_dbg(cirrus_pwr->dev, "Right Amp: Passport %s\n",
				cirrus_pwr->passport_enable_right ?
				"Enabled" : "Disabled");
		dev_dbg(cirrus_pwr->dev, "Left Amp: Passport %s\n",
				cirrus_pwr->passport_enable_left ?
				"Enabled" : "Disabled");

		break;
	case CIRRUS_PWR_STATUS_ERROR:
	case CIRRUS_PWR_STATUS_DISABLED:
	default:
		break;
	}

	mutex_unlock(&cirrus_pwr->pwr_lock);

	/* Queue next operation */
	if (cirrus_pwr->global_enable) {
		queue_delayed_work(cirrus_pwr->pwr_workqueue,
			   &cirrus_pwr->pwr_work,
			   msecs_to_jiffies(cirrus_pwr->interval));
	}
}

/***** SYSFS Interfaces *****/

static ssize_t cirrus_pwr_version_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, CIRRUS_PWR_VERSION "\n");
}

static ssize_t cirrus_pwr_version_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return size;
}

static ssize_t cirrus_pwr_uptime_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", cirrus_pwr->uptime_ms);
}

static ssize_t cirrus_pwr_uptime_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return size;
}

static ssize_t cirrus_pwr_power_left_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	unsigned int power_squared;
	unsigned int power = 0;

	if (cirrus_pwr->amp_left_active) {
		regmap_read(cirrus_pwr->regmap_left,
			CIRRUS_PWR_CSPL_OUTPUT_POWER_SQ,
			&power_squared);
		power = convert_power(power_squared);
	}

	return sprintf(buf, "%x\n", power);
}

static ssize_t cirrus_pwr_power_left_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return size;
}

static ssize_t cirrus_pwr_power_right_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	unsigned int power_squared;
	unsigned int power = 0;

	if (cirrus_pwr->amp_right_active) {
		regmap_read(cirrus_pwr->regmap_right,
			CIRRUS_PWR_CSPL_OUTPUT_POWER_SQ,
			&power_squared);
		power = convert_power(power_squared);
	}

	return sprintf(buf, "%x\n", power);
}

static ssize_t cirrus_pwr_power_right_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return size;
}

static ssize_t cirrus_pwr_interval_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", cirrus_pwr->interval);
}

static ssize_t cirrus_pwr_interval_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &cirrus_pwr->interval))
		dev_err(cirrus_pwr->dev,
			"%s: Failed to convert from str to u32.\n",
			__func__);
	return size;
}

static ssize_t cirrus_pwr_status_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	switch (cirrus_pwr->status) {
	case CIRRUS_PWR_STATUS_DISABLED:
		return sprintf(buf, "Disabled\n");
	case CIRRUS_PWR_STATUS_ENABLED:
		return sprintf(buf, "Enabled\n");
	case CIRRUS_PWR_STATUS_ERROR:
		return sprintf(buf, "Error\n");
	default:
		return sprintf(buf, "\n");
	}
}

static ssize_t cirrus_pwr_status_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return size;
}

static ssize_t cirrus_pwr_target_min_time_ms_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", cirrus_pwr->target_min_time_ms);
}

static ssize_t cirrus_pwr_target_min_time_ms_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &cirrus_pwr->target_min_time_ms))
		dev_err(cirrus_pwr->dev,
			"%s: Failed to convert from str to u32.\n",
			__func__);
	return size;
}

static ssize_t cirrus_pwr_target_temp_left_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", cirrus_pwr->target_temp_left);
}

static ssize_t cirrus_pwr_target_temp_left_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &cirrus_pwr->target_temp_left))
		dev_err(cirrus_pwr->dev,
			"%s: Failed to convert from str to u32.\n",
			__func__);
	return size;
}

static ssize_t cirrus_pwr_target_temp_right_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", cirrus_pwr->target_temp_right);
}

static ssize_t cirrus_pwr_target_temp_right_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &cirrus_pwr->target_temp_right))
		dev_err(cirrus_pwr->dev,
			"%s: Failed to convert from str to u32.\n",
			__func__);
	return size;
}

static ssize_t cirrus_pwr_exit_temp_left_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", cirrus_pwr->exit_temp_left);
}

static ssize_t cirrus_pwr_exit_temp_left_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &cirrus_pwr->exit_temp_left))
		dev_err(cirrus_pwr->dev,
			"%s: Failed to convert from str to u32.\n",
			__func__);
	return size;
}

static ssize_t cirrus_pwr_exit_temp_right_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", cirrus_pwr->exit_temp_right);
}

static ssize_t cirrus_pwr_exit_temp_right_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &cirrus_pwr->exit_temp_right))
		dev_err(cirrus_pwr->dev,
			"%s: Failed to convert from str to u32.\n",
			__func__);
	return size;
}

static ssize_t cirrus_pwr_amb_temp_left_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", cirrus_pwr->amb_temp_left);
}

static ssize_t cirrus_pwr_amb_temp_left_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &cirrus_pwr->amb_temp_left))
		dev_err(cirrus_pwr->dev,
			"%s: Failed to convert from str to u32.\n",
			__func__);
	return size;
}

static ssize_t cirrus_pwr_amb_temp_right_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", cirrus_pwr->amb_temp_right);
}

static ssize_t cirrus_pwr_amb_temp_right_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &cirrus_pwr->amb_temp_right))
		dev_err(cirrus_pwr->dev,
			"%s: Failed to convert from str to u32.\n",
			__func__);
	return size;
}

static ssize_t cirrus_pwr_spk_temp_left_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", cirrus_pwr->spk_temp_left);
}

static ssize_t cirrus_pwr_spk_temp_left_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &cirrus_pwr->spk_temp_left))
		dev_err(cirrus_pwr->dev,
			"%s: Failed to convert from str to u32.\n",
			__func__);
	return size;
}

static ssize_t cirrus_pwr_spk_temp_right_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", cirrus_pwr->spk_temp_right);
}

static ssize_t cirrus_pwr_spk_temp_right_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &cirrus_pwr->spk_temp_right))
		dev_err(cirrus_pwr->dev,
			"%s: Failed to convert from str to u32.\n",
			__func__);
	return size;
}

static ssize_t cirrus_pwr_global_enable_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", cirrus_pwr->global_enable);
}

static ssize_t cirrus_pwr_global_enable_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	unsigned int enable;

	if (kstrtou32(buf, 0, &enable)) {
		dev_err(cirrus_pwr->dev,
			"%s: Failed to convert from str to u32.\n",
			__func__);
		return size;
	}

	cirrus_pwr->global_enable = enable;

	if (enable == 0 &&
		cirrus_pwr->status == CIRRUS_PWR_STATUS_ENABLED) {
		/* Stop both amps */
		cirrus_pwr_stop(true);
		cirrus_pwr_stop(false);
	}

	return size;
}


static DEVICE_ATTR(version, 0444, cirrus_pwr_version_show,
				cirrus_pwr_version_store);
static DEVICE_ATTR(uptime, 0444, cirrus_pwr_uptime_show,
				cirrus_pwr_uptime_store);
static DEVICE_ATTR(value, 0444, cirrus_pwr_power_left_show,
				cirrus_pwr_power_left_store);
static DEVICE_ATTR(value_r, 0444, cirrus_pwr_power_right_show,
				cirrus_pwr_power_right_store);
static DEVICE_ATTR(interval, 0664, cirrus_pwr_interval_show,
				cirrus_pwr_interval_store);
static DEVICE_ATTR(status, 0664, cirrus_pwr_status_show,
				cirrus_pwr_status_store);
static DEVICE_ATTR(target_min_time_ms, 0664, cirrus_pwr_target_min_time_ms_show,
				cirrus_pwr_target_min_time_ms_store);
static DEVICE_ATTR(target_temp, 0664, cirrus_pwr_target_temp_left_show,
				cirrus_pwr_target_temp_left_store);
static DEVICE_ATTR(target_temp_r, 0664, cirrus_pwr_target_temp_right_show,
				cirrus_pwr_target_temp_right_store);
static DEVICE_ATTR(exit_temp, 0664, cirrus_pwr_exit_temp_left_show,
				cirrus_pwr_exit_temp_left_store);
static DEVICE_ATTR(exit_temp_r, 0664, cirrus_pwr_exit_temp_right_show,
				cirrus_pwr_exit_temp_right_store);
static DEVICE_ATTR(env_temp, 0664, cirrus_pwr_amb_temp_left_show,
				cirrus_pwr_amb_temp_left_store);
static DEVICE_ATTR(env_temp_r, 0664, cirrus_pwr_amb_temp_right_show,
				cirrus_pwr_amb_temp_right_store);
static DEVICE_ATTR(spk_t, 0664, cirrus_pwr_spk_temp_left_show,
				cirrus_pwr_spk_temp_left_store);
static DEVICE_ATTR(spk_t_r, 0664, cirrus_pwr_spk_temp_right_show,
				cirrus_pwr_spk_temp_right_store);
static DEVICE_ATTR(global_enable, 0664, cirrus_pwr_global_enable_show,
				cirrus_pwr_global_enable_store);

static struct attribute *cirrus_pwr_attr[] = {
	&dev_attr_version.attr,
	&dev_attr_uptime.attr,
	&dev_attr_value.attr,
	&dev_attr_value_r.attr,
	&dev_attr_interval.attr,
	&dev_attr_status.attr,
	&dev_attr_target_min_time_ms.attr,
	&dev_attr_target_temp.attr,
	&dev_attr_target_temp_r.attr,
	&dev_attr_exit_temp.attr,
	&dev_attr_exit_temp_r.attr,
	&dev_attr_env_temp.attr,
	&dev_attr_env_temp_r.attr,
	&dev_attr_spk_t.attr,
	&dev_attr_spk_t_r.attr,
	&dev_attr_global_enable.attr,
	NULL,
};

static struct attribute_group cirrus_pwr_attr_grp = {
	.attrs = cirrus_pwr_attr,
};

int __init cirrus_pwr_init(struct class *cirrus_amp_class)
{
	int ret;

	cirrus_pwr = kzalloc(sizeof(struct cirrus_pwr_t), GFP_KERNEL);
	if (cirrus_pwr == NULL)
		return -ENOMEM;

	cirrus_pwr->pwr_class = cirrus_amp_class;

	cirrus_pwr->dev = device_create(cirrus_pwr->pwr_class, NULL, 1, NULL,
						CIRRUS_PWR_DIR_NAME);
	if (IS_ERR(cirrus_pwr->dev)) {
		ret = PTR_ERR(cirrus_pwr->dev);
		pr_err("Failed to create cirrus_pwr device\n");
		class_destroy(cirrus_pwr->pwr_class);
		goto err;
	}

	cirrus_pwr->pwr_workqueue = create_singlethread_workqueue(
						CIRRUS_PWR_WORKQ_NAME);
	if (cirrus_pwr->pwr_workqueue == NULL) {
		dev_err(cirrus_pwr->dev, "Failed to create workqueue\n");
		ret = -ENOENT;
		goto err;
	}

	cirrus_pwr->interval = 10000;
	cirrus_pwr->amb_temp_right = 2500;
	cirrus_pwr->amb_temp_left = 2500;
	cirrus_pwr->spk_temp_right = 2500;
	cirrus_pwr->spk_temp_left = 2500;
	cirrus_pwr->target_temp_right = 3400;
	cirrus_pwr->target_temp_left = 3400;
	cirrus_pwr->exit_temp_right = 3250;
	cirrus_pwr->exit_temp_left = 3250;
	cirrus_pwr->uptime_ms = 0;
	cirrus_pwr->target_min_time_ms = 300000;
	cirrus_pwr->global_enable = 1;
	cirrus_pwr->passport_enable_right = 0;
	cirrus_pwr->passport_enable_left = 0;

	ret = sysfs_create_group(&cirrus_pwr->dev->kobj, &cirrus_pwr_attr_grp);
	if (ret) {
		dev_err(cirrus_pwr->dev, "Failed to create sysfs group\n");
		goto err;
	}

	mutex_init(&cirrus_pwr->pwr_lock);
	INIT_DELAYED_WORK(&cirrus_pwr->pwr_work,
						cirrus_pwr_work);

	return 0;
err:
	kfree(cirrus_pwr);
	return ret;
}

void cirrus_pwr_exit(void)
{
	kfree(cirrus_pwr);
}

