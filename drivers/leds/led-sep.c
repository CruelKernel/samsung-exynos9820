/*
 * LED driver for SEP
 *
 * Copyright (C) 2018 Samsung Electronics
 * Author: Hyuk Kang, HyungJae Im
 *
 */

#define pr_fmt(fmt) "sled: " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/sec_class.h>
#include "led-sep.h"

static struct led_sep *led_sep_one;

static int calc_color(int old, int new, int color)
{
	int r, g, b;
	int rgb = 0;

	if (old == new)
		return color;

	r = LED_R(color);
	g = LED_G(color);
	b = LED_B(color);

	pr_info("calc_color1: (0x%x, 0x%x) R(0x%x) G(0x%x) B(0x%x)\n",
			old, new, r, g, b);

	r = (r * 100 / old) * new / 100;
	g = (g * 100 / old) * new / 100;
	b = (b * 100 / old) * new / 100;

	pr_info("calc_color1: new R(0x%x) G(0x%x) B(0x%x)\n",
			r, g, b);

	rgb = COLOR(r, g, b);
	pr_info("calc_color1: 0x%x -> 0x%x\n", color, rgb);
	return rgb;
}

int led_sep_register_device(struct led_sep_ops *ops)
{
	int max, low;

	if (led_sep_one) {
		led_sep_one->ops = ops;

		max = ops->current_max;
		led_sep_one->maxpower = max ? max : COLOR_MAX;
		if (led_sep_one->maxpower > COLOR_MAX)
			led_sep_one->maxpower = COLOR_MAX;

		low = ops->current_lowpower;
		led_sep_one->lowpower_current = (low > (max / 3)) ? low : max;
		if (led_sep_one->lowpower_current > led_sep_one->maxpower)
			led_sep_one->lowpower_current = led_sep_one->maxpower;

		pr_info("New LED Added: %s, maxpower 0x%x, lowpower 0x%x\n",
				ops->name,
				led_sep_one->maxpower,
				led_sep_one->lowpower_current);

	} else
		return -ENODEV;
	return 0;
}
EXPORT_SYMBOL(led_sep_register_device);

static int led_sep_calc_color(struct led_sep *sled, int color)
{
	int ret = color;

	if (color) {
		if (sled->lowpower_mode)
			ret = calc_color(COLOR_MAX,
					sled->lowpower_current, color);

		else if (sled->maxpower != COLOR_MAX)
			ret = calc_color(COLOR_MAX, sled->maxpower, color);
	}
	pr_info("calc_color2: 0x%x -> 0x%x\n", color, ret);
	return ret;
}

#define check_capability(led, cap) \
	do { \
		if (!led_sep_check_capability(led, cap)) { \
			pr_info("%s: check failed, 0x%x\n", \
					__func__, cap); \
			return -EPERM; \
		} \
	} while (0)

static inline int led_sep_check_capability(struct led_sep *sled,
		unsigned long cap)
{
	if (sled && sled->ops && (sled->ops->capability & cap))
		return true;
	return false;
}

static int led_sep_light(struct led_sep *sled,
		int id, int on, int color)
{
	int ret = 0;
	int col = led_sep_calc_color(sled, color);

	if (sled && sled->ops && sled->ops->light)
		ret = sled->ops->light(sled->ops, id, on, col);
	else {
		pr_err("NO LED?\n");
		ret = -ENODEV;
	}

	return ret;
}

static int led_sep_blink(struct led_sep *sled,
		int id, int color, int on, int off, int blink_count)
{
	int ret = 0;
	int col = led_sep_calc_color(sled, color);

	if (sled && sled->ops && sled->ops->blink)
		ret = sled->ops->blink(sled->ops,
				id, col, on, off, blink_count);
	else {
		pr_info("sw blink is not ready.\n");
		//TODO
	}

	return ret;
}

static int led_sep_test(struct led_sep *sled,
		int reg, int value)
{
	int ret = 0;

	if (sled && sled->ops && sled->ops->test)
		ret = sled->ops->test(sled->ops, reg, value);
	else {
		pr_info("sw test is not supported.\n");
		ret = -ENODEV;
	}

	return ret;
}

static int led_sep_reset(struct led_sep *sled)
{
	int ret = 0;

	if (sled && sled->ops && sled->ops->reset)
		ret = sled->ops->reset(sled->ops);
	else {
		pr_info("sw reset is not supported.\n");
		ret = -ENODEV;
	}

	return ret;
}

static int led_sep_pattern(struct led_sep *sled, int mode)
{
	int ret = 0;

	if (sled && sled->ops && sled->ops->pattern) {
		ret = sled->ops->pattern(sled->ops, mode);

		if (ret > 0) {
			pr_info("%s ops->pattern Done\n", __func__);
			return ret;
		}
	}
	// TODO
	// Common patterns for all LED
	switch (mode) {
	case PATTERN_OFF:
		led_sep_light(sled, LED_ALL, 0, 0);
		break;
	case CHARGING:
		break;
	case CHARGING_ERR:
		break;
	case MISSED_NOTI:
		break;
	case LOW_BATTERY:
		break;
	case FULLY_CHARGED:
		break;
	case POWERING:
		break;
	case POWERING_OFF:
		break;
	case FOTA:
		break;
	default:
		break;
	}
	pr_info("%s pattern Done\n", __func__);
	return ret;
}

static ssize_t store_led_sep_lowpower(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	struct led_sep *sled = dev_get_drvdata(dev);
	u8 led_lowpower;
	int ret;

	check_capability(sled, SEP_NODE_LOWPOWER);

	ret = kstrtou8(buf, 0, &led_lowpower);
	if (ret != 0) {
		dev_err(dev, "fail to get led_lowpower.\n");
		return -EINVAL;
	}

	if (sled->maxpower < led_lowpower) {
		dev_err(dev, "lowpower is higher than maxpower.\n");
		return -EINVAL;
	}

	sled->lowpower_mode = led_lowpower;

	pr_info("led_lowpower mode set to %i\n", led_lowpower);

	return count;
}

static ssize_t store_led_sep_pattern(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	struct led_sep *sled = dev_get_drvdata(dev);
	unsigned int mode = 0;
	int ret;

	check_capability(sled, SEP_NODE_PATTERN);

	ret = sscanf(buf, "%1d", &mode);
	if (ret == 0) {
		dev_err(dev, "fail to get led_pattern mode.\n");
		return -EINVAL;
	}

	pr_info("%s: %s(%d) lowpower: %d current: %d\n", __func__,
			pattern_string(mode), mode,
			sled->lowpower_mode,
			sled->lowpower_current);

	led_sep_pattern(sled, mode);
	return count;
}

/*
 * turn on  : color 1 0
 * turn off : color 0 0
 * blink    : color 500 500
 *
 * Command format: color on off
 */
static ssize_t store_led_sep_blink(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	struct led_sep *sled = dev_get_drvdata(dev);
	int id, color, on_time, off_time;
	int ret = 0;

	color = on_time = off_time = 0;
	id = LED_ALL;

	pr_info("%s: len %d: %s", __func__, count, buf);
	check_capability(sled, SEP_NODE_BLINK);

	ret = sscanf(buf, "0x%8x %d %d", &color,
					&on_time, &off_time);
	if (ret == 0) {
		dev_err(dev, "fail to get led_blink value.\n");
		return -EINVAL;
	}

	pr_info("blink %d: 0x%x %d %d\n", ret, color,
			on_time, off_time);

	if (!on_time || on_time == 1)
		led_sep_light(sled, id, on_time, color);
	else
		led_sep_blink(sled, id, color, on_time, off_time, 0);

	return count;
}

/*
 * New API for SEP LED
 *
 * turn on  : id color 1 0
 * turn off : id color 0 0
 * blink    : id color 500 500 3(TBD: count)
 *
 * Command format: id color on off count
 */
static ssize_t store_led_sep_control(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	struct led_sep *sled = dev_get_drvdata(dev);
	int id, color, on_time, off_time, blink_count;
	int ret = 0;

	id = color = on_time = off_time = blink_count = 0;

	pr_info("%s: len %d: %s", __func__, count, buf);
	check_capability(sled, SEP_NODE_CONTROL);

	ret = sscanf(buf, "%d 0x%8x %d %d %d", &id, &color,
					&on_time, &off_time, &blink_count);
	if (ret < 3) {
		dev_err(dev, "fail to get led_control value. %d\n", ret);
		return -EINVAL;
	}

	pr_info("control %d: %d 0x%x %d %d %d\n", ret, id, color,
			on_time, off_time, blink_count);

	if (!on_time || on_time == 1)
		led_sep_light(sled, id, on_time, color);
	else
		led_sep_blink(sled, id, color,
				on_time, off_time, blink_count);

	return count;
}

static ssize_t store_led_sep_test(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	struct led_sep *sled = dev_get_drvdata(dev);
	int ret = 0;
	int reg, val;

	pr_info("%s: %s (%d)\n", __func__, buf, count);
	check_capability(sled, SEP_NODE_TEST);

	ret = sscanf(buf, "%2x %2x", &reg, &val);
	if (ret == 0) {
		dev_err(dev, "fail to get led_test value.\n");
		return -EINVAL;
	}

	pr_info("test reg: %2x val: %2x\n", reg, val);

	led_sep_test(sled, reg, val);

	return count;
}

static ssize_t store_led_sep_reset(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	struct led_sep *sled = dev_get_drvdata(dev);
	int ret = 0;
	int val;

	pr_info("%s: %s (%d)\n", __func__, buf, count);
	check_capability(sled, SEP_NODE_RESET);

	ret = kstrtoint(buf, 0, &val);
	if (ret < 0) {
		dev_err(dev, "fail to get led_reset value.\n");
		return -EINVAL;
	}

	led_sep_reset(sled);

	return count;
}

static DEVICE_ATTR(led_lowpower, 0660, NULL,  store_led_sep_lowpower);
static DEVICE_ATTR(led_pattern, 0660, NULL, store_led_sep_pattern);
static DEVICE_ATTR(led_blink, 0660, NULL, store_led_sep_blink);
static DEVICE_ATTR(led_control, 0660, NULL, store_led_sep_control);
static DEVICE_ATTR(led_test, 0660, NULL, store_led_sep_test);
static DEVICE_ATTR(led_reset, 0660, NULL, store_led_sep_reset);

static struct attribute *sec_led_attributes[] = {
	&dev_attr_led_lowpower.attr,
	&dev_attr_led_pattern.attr,
	&dev_attr_led_blink.attr,
	&dev_attr_led_control.attr,
	&dev_attr_led_test.attr,
	&dev_attr_led_reset.attr,
	NULL,
};

static struct attribute_group sec_led_attr_group = {
	.attrs = sec_led_attributes,
};

static int led_sep_probe(struct platform_device *pdev)
{
	struct led_sep *sled;
	int ret = 0;

	dev_info(&pdev->dev, "%s\n", __func__);

	sled = devm_kzalloc(&pdev->dev, sizeof(*sled), GFP_KERNEL);
	if (sled == NULL)
		return -ENOMEM;

	sled->sdev = sec_device_create(sled, "led");
	if (sled->sdev == NULL) {
		dev_err(&pdev->dev, "Failed to create device for SEP led\n");
		return -ENOMEM;
	}

	ret = (int)sysfs_create_group(&sled->sdev->kobj, &sec_led_attr_group);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to create sysfs group for SEP led\n");
		sec_device_destroy(sled->sdev->devt);
		return -ENOMEM;
	}

	led_sep_one = sled;
	platform_set_drvdata(pdev, sled);
	dev_info(&pdev->dev, "%s done\n", __func__);
	return ret;
}

static int led_sep_remove(struct platform_device *pdev)
{
	struct led_sep *sled = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "%s\n", __func__);
	sysfs_remove_group(&sled->sdev->kobj, &sec_led_attr_group);
	sec_device_destroy(sled->sdev->devt);
	dev_info(&pdev->dev, "%s done\n", __func__);
	return 0;
}

static const struct of_device_id led_sep_dt_ids[] = {
	{ .compatible = "samsung,led-sep",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, led_sep_dt_ids);

static struct platform_driver led_sep_driver = {
	.probe		= led_sep_probe,
	.remove		= led_sep_remove,
	.driver		= {
		.name	= "led_sep",
		.owner	= THIS_MODULE,
		.of_match_table	= of_match_ptr(led_sep_dt_ids),
	},
};

static int __init led_sep_init(void)
{
	return platform_driver_register(&led_sep_driver);
}

static void __init led_sep_exit(void)
{
	platform_driver_unregister(&led_sep_driver);
}

subsys_initcall(led_sep_init);
module_exit(led_sep_exit);

MODULE_AUTHOR("Samsung EIF Team");
