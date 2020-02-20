/*
 * RGB-led driver for Maxim MAX77705
 *
 * Copyright (C) 2013 Maxim Integrated Product
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/of.h>
#include <linux/mfd/max77705.h>
#include <linux/mfd/max77705-private.h>
#include <linux/leds-max77705-rgb.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/regmap.h>
#include <linux/sec_class.h>

#define SEC_LED_SPECIFIC

/* Registers */

/* MAX77705_REG_LED0BRT */
#define MAX77705_LED0BRT	0xFF

/* MAX77705_REG_LED1BRT */
#define MAX77705_LED1BRT	0xFF

/* MAX77705_REG_LED2BRT */
#define MAX77705_LED2BRT	0xFF

/* MAX77705_REG_LED3BRT */
#define MAX77705_LED3BRT	0xFF

/* MAX77705_REG_LEDBLNK */
#define MAX77705_LEDBLINKD	0xF0
#define MAX77705_LEDBLINKP	0x0F

/* MAX77705_REG_LEDRMP */
#define MAX77705_RAMPUP		0xF0
#define MAX77705_RAMPDN		0x0F

#define LED_R_MASK		0x00FF0000
#define LED_G_MASK		0x0000FF00
#define LED_B_MASK		0x000000FF
#define LED_MAX_CURRENT		0xFF

/* MAX77705_STATE*/
#define LED_DISABLE			0
#define LED_ALWAYS_ON			1
#define LED_BLINK			2

#define LEDBLNK_ON(time)	((time < 100) ? 0 :			\
				(time < 500) ? time/100-1 :		\
				(time < 3250) ? (time-500)/250+4 : 15)

#define LEDBLNK_OFF(time)	((time < 1) ? 0x00 :			\
				(time < 500) ? 0x01 :			\
				(time < 5000) ? time/500 :		\
				(time < 8000) ? (time-5000)/1000+10 :	 \
				(time < 12000) ? (time-8000)/2000+13 : 15)

#define OCTA_LEN		4
#define RGB_BUFSIZE		30

static u8 led_dynamic_current = 0x14;
static u8 normal_powermode_current = 0x14;
static u8 low_powermode_current = 0x05;

/* led_device_type */
static unsigned int led_device_type;

static unsigned int brightness_ratio_r = 100;
static unsigned int brightness_ratio_g = 100;
static unsigned int brightness_ratio_b = 100;
static unsigned int brightness_ratio_r_low = 20;
static unsigned int brightness_ratio_g_low = 20;
static unsigned int brightness_ratio_b_low = 20;
static u8 led_lowpower_mode;
static u8 led_pattern_num;

static unsigned int octa_color;

enum max77705_led_color {
	WHITE,
	RED,
	GREEN,
	BLUE,
};
enum max77705_led_pattern {
	PATTERN_OFF,
	CHARGING,
	CHARGING_ERR,
	MISSED_NOTI,
	LOW_BATTERY,
	FULLY_CHARGED,
	POWERING,
};

static struct device *led_dev;

struct max77705_rgb {
	struct led_classdev led[4];
	struct i2c_client *i2c;
	unsigned int delay_on_times_ms;
	unsigned int delay_off_times_ms;
};

static int max77705_rgb_number(struct led_classdev *led_cdev,
				struct max77705_rgb **p)
{
	const struct device *parent = led_cdev->dev->parent;
	struct max77705_rgb *max77705_rgb = dev_get_drvdata(parent);
	int i;

	*p = max77705_rgb;

	for (i = 0; i < 4; i++) {
		if (led_cdev == &max77705_rgb->led[i]) {
			pr_debug("leds-max77705-rgb: %s, %d\n", __func__, i);
			return i;
		}
	}

	pr_err("leds-max77705-rgb: %s, can't find rgb number\n", __func__);

	return -ENODEV;
}

static void max77705_rgb_set(struct led_classdev *led_cdev,
				unsigned int brightness)
{
	const struct device *parent = led_cdev->dev->parent;
	struct max77705_rgb *max77705_rgb = dev_get_drvdata(parent);
	struct device *dev;
	int n;
	int ret;

	ret = max77705_rgb_number(led_cdev, &max77705_rgb);
	if (ret < 0) {
		dev_err(led_cdev->dev,
			"max77705_rgb_number() returns %d.\n", ret);
		return;
	}

	dev = led_cdev->dev;
	n = ret;

	if (brightness == LED_OFF) {
		/* Flash OFF */
		ret = max77705_update_reg(max77705_rgb->i2c,
					MAX77705_RGBLED_REG_LEDEN, 0, 3 << (2*n));
		if (ret < 0) {
			dev_err(dev, "can't write LEDEN : %d\n", ret);
			return;
		}
	} else {
		/* Set current */
		ret = max77705_write_reg(max77705_rgb->i2c,
				MAX77705_RGBLED_REG_LED0BRT + n, brightness);
		if (ret < 0) {
			dev_err(dev, "can't write LEDxBRT : %d\n", ret);
			return;
		}
	}
}

static void max77705_rgb_set_state(struct led_classdev *led_cdev,
				unsigned int brightness, unsigned int led_state)
{
	const struct device *parent = led_cdev->dev->parent;
	struct max77705_rgb *max77705_rgb = dev_get_drvdata(parent);
	struct device *dev;
	int n;
	int ret;

	ret = max77705_rgb_number(led_cdev, &max77705_rgb);

	if (ret < 0) {
		dev_err(led_cdev->dev,
			"max77705_rgb_number() returns %d.\n", ret);
		return;
	}

	dev = led_cdev->dev;
	n = ret;

	if (brightness != 0) {
		/* apply brightness ratio for optimize each led brightness*/
		switch (n) {
		case RED:
			if (led_lowpower_mode == 1)
				brightness = brightness * brightness_ratio_r_low / 100;
			else
				brightness = brightness * brightness_ratio_r / 100;
			break;
		case GREEN:
			if (led_lowpower_mode == 1)
				brightness = brightness * brightness_ratio_g_low / 100;
			else
				brightness = brightness * brightness_ratio_g / 100;
			break;
		case BLUE:
			if (led_lowpower_mode == 1)
				brightness = brightness * brightness_ratio_b_low / 100;
			else
				brightness = brightness * brightness_ratio_b / 100;
			break;
		}

		/*
		 *	There is possibility that low_powermode_current is 0.
		 *	ex) low_powermode_current is 1 & brightness_ratio_r is 90
		 *	brightness = 1 * 90 / 100 = 0.9
		 *	brightness is inteager, so brightness is 0.
		 *	In this case, it is need to assign 1 of value.
		 */
		if (brightness == 0)
			brightness = 1;
	}
	max77705_rgb_set(led_cdev, brightness);

	pr_info("leds-max77705-rgb: %s, led_num = %d, brightness = %d\n", __func__, ret, brightness);

	ret = max77705_update_reg(max77705_rgb->i2c,
			MAX77705_RGBLED_REG_LEDEN, led_state << (2*n), 0x3 << 2*n);
	if (ret < 0) {
		dev_err(dev, "can't write FLASH_EN : %d\n", ret);
		return;
	}
}

static unsigned int max77705_rgb_get(struct led_classdev *led_cdev)
{
	const struct device *parent = led_cdev->dev->parent;
	struct max77705_rgb *max77705_rgb = dev_get_drvdata(parent);
	struct device *dev;
	int n;
	int ret;
	u8 value;

	pr_info("leds-max77705-rgb: %s\n", __func__);

	ret = max77705_rgb_number(led_cdev, &max77705_rgb);
	if (ret < 0) {
		dev_err(led_cdev->dev,
			"max77705_rgb_number() returns %d.\n", ret);
		return 0;
	}
	n = ret;

	dev = led_cdev->dev;

	/* Get status */
	ret = max77705_read_reg(max77705_rgb->i2c,
				MAX77705_RGBLED_REG_LEDEN, &value);
	if (ret < 0) {
		dev_err(dev, "can't read LEDEN : %d\n", ret);
		return 0;
	}
	if (!(value & (3 << (2*n))))
		return LED_OFF;

	/* Get current */
	ret = max77705_read_reg(max77705_rgb->i2c,
				MAX77705_RGBLED_REG_LED0BRT + n, &value);
	if (ret < 0) {
		dev_err(dev, "can't read LED0BRT : %d\n", ret);
		return 0;
	}

	return value;
}

static int max77705_rgb_ramp(struct device *dev, int ramp_up, int ramp_down)
{
	struct max77705_rgb *max77705_rgb = dev_get_drvdata(dev);
	int value;
	int ret;

	pr_info("leds-max77705-rgb: %s\n", __func__);

	if (ramp_up <= 800) {
		ramp_up /= 100;
	} else {
		ramp_up = (ramp_up - 800) * 2 + 800;
		ramp_up /= 100;
	}

	if (ramp_down <= 800) {
		ramp_down /= 100;
	} else {
		ramp_down = (ramp_down - 800) * 2 + 800;
		ramp_down /= 100;
	}

	value = (ramp_down) | (ramp_up << 4);
	ret = max77705_write_reg(max77705_rgb->i2c,
					MAX77705_RGBLED_REG_LEDRMP, value);
	if (ret < 0) {
		dev_err(dev, "can't write REG_LEDRMP : %d\n", ret);
		return -ENODEV;
	}

	return 0;
}

static int max77705_rgb_blink(struct device *dev,
				unsigned int delay_on, unsigned int delay_off)
{
	struct max77705_rgb *max77705_rgb = dev_get_drvdata(dev);
	int value;
	int ret = 0;

	pr_info("leds-max77705-rgb: %s\n", __func__);

	value = (LEDBLNK_ON(delay_on) << 4) | LEDBLNK_OFF(delay_off);
	ret = max77705_write_reg(max77705_rgb->i2c,
				MAX77705_RGBLED_REG_LEDBLNK, value);
	if (ret < 0) {
		dev_err(dev, "can't write REG_LEDBLNK : %d\n", ret);
		return -EINVAL;
	}

	return ret;
}

#ifdef CONFIG_OF
static struct max77705_rgb_platform_data
			*max77705_rgb_parse_dt(struct device *dev)
{
	struct max77705_rgb_platform_data *pdata;
	struct device_node *nproot = dev->parent->of_node;
	struct device_node *np;
	int ret;
	int i;
	int temp;
	char octa[4] = {0, };
	char br_ratio_r[23] = "br_ratio_r";
	char br_ratio_g[23] = "br_ratio_g";
	char br_ratio_b[23] = "br_ratio_b";
	char br_ratio_r_low[23] = "br_ratio_r_low";
	char br_ratio_g_low[23] = "br_ratio_g_low";
	char br_ratio_b_low[23] = "br_ratio_b_low";
	char normal_po_cur[29] = "normal_powermode_current";
	char low_po_cur[26] = "low_powermode_current";

	pr_info("leds-max77705-rgb: %s\n", __func__);

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (unlikely(pdata == NULL))
		return ERR_PTR(-ENOMEM);

	np = of_find_node_by_name(nproot, "rgb");
	if (unlikely(np == NULL)) {
		dev_err(dev, "rgb node not found\n");
		devm_kfree(dev, pdata);
		return ERR_PTR(-EINVAL);
	}

	for (i = 0; i < 4; i++)	{
		ret = of_property_read_string_index(np, "rgb-name", i,
						(const char **)&pdata->name[i]);

		pr_info("leds-max77705-rgb: %s, %s\n", __func__, pdata->name[i]);

		if (ret < 0) {
			devm_kfree(dev, pdata);
			return ERR_PTR(ret);
		}
	}

	/* get led_device_type value in dt */
	ret = of_property_read_u32(np, "led_device_type", &temp);
	if (ret < 0)
		pr_info("leds-max77705-rgb: %s, can't parsing led_device_type in dt\n", __func__);
	else
		led_device_type = (u8)temp;
	pr_info("leds-max77705-rgb: %s, led_device_type = %x\n", __func__, led_device_type);

	/* STAR and STAR2 */
	if (led_device_type == 0) {
		switch (octa_color) {
		case 0:
			strncpy(octa, "_uu", OCTA_LEN);
			break;
		case 1:
			strncpy(octa, "_bk", OCTA_LEN);
			break;
		case 2:
			strncpy(octa, "_wh", OCTA_LEN);
			break;
		case 3:
			strncpy(octa, "_gd", OCTA_LEN);
			break;
		case 4:
			strncpy(octa, "_sv", OCTA_LEN);
			break;
		case 5:
			strncpy(octa, "_gr", OCTA_LEN);
			break;
		case 6:
			strncpy(octa, "_bl", OCTA_LEN);
			break;
		case 7:
			strncpy(octa, "_pg", OCTA_LEN);
			break;
		default:
			break;
		}
	}

	strncat(normal_po_cur, octa, strlen(octa));
	strncat(low_po_cur, octa, strlen(octa));
	strncat(br_ratio_r, octa, strlen(octa));
	strncat(br_ratio_g, octa, strlen(octa));
	strncat(br_ratio_b, octa, strlen(octa));
	strncat(br_ratio_r_low, octa, strlen(octa));
	strncat(br_ratio_g_low, octa, strlen(octa));
	strncat(br_ratio_b_low, octa, strlen(octa));

	/* get normal_powermode_current value in dt */
	ret = of_property_read_u32(np, normal_po_cur, &temp);
	if (ret < 0)
		pr_info("leds-max77705-rgb: %s, can't parsing normal_powermode_current in dt\n", __func__);
	else
		normal_powermode_current = (u8)temp;
	pr_info("leds-max77705-rgb: %s, normal_powermode_current = %x\n", __func__, normal_powermode_current);

	/* get low_powermode_current value in dt */
	ret = of_property_read_u32(np, low_po_cur, &temp);
	if (ret < 0)
		pr_info("leds-max77705-rgb: %s, can't parsing low_powermode_current in dt\n", __func__);
	else
		low_powermode_current = (u8)temp;
	pr_info("leds-max77705-rgb: %s, low_powermode_current = %x\n", __func__, low_powermode_current);

	/* get led red brightness ratio */
	ret = of_property_read_u32(np, br_ratio_r, &temp);
	if (ret < 0)
		pr_info("leds-max77705-rgb: %s, can't parsing brightness_ratio_r in dt\n", __func__);
	else
		brightness_ratio_r = (int)temp;
	pr_info("leds-max77705-rgb: %s, brightness_ratio_r = %x\n", __func__, brightness_ratio_r);

	/* get led green brightness ratio */
	ret = of_property_read_u32(np, br_ratio_g, &temp);
	if (ret < 0)
		pr_info("leds-max77705-rgb: %s, can't parsing brightness_ratio_g in dt\n", __func__);
	else
		brightness_ratio_g = (int)temp;
	pr_info("leds-max77705-rgb: %s, brightness_ratio_g = %x\n", __func__, brightness_ratio_g);

	/* get led blue brightness ratio */
	ret = of_property_read_u32(np, br_ratio_b, &temp);
	if (ret < 0)
		pr_info("leds-max77705-rgb: %s, can't parsing brightness_ratio_b in dt\n", __func__);
	else
		brightness_ratio_b = (int)temp;
	pr_info("leds-max77705-rgb: %s, brightness_ratio_b = %x\n", __func__, brightness_ratio_b);


	/* get led red brightness ratio lowpower */
	ret = of_property_read_u32(np, br_ratio_r_low, &temp);
	if (ret < 0)
		pr_info("leds-max77705-rgb: %s, can't parsing brightness_ratio_r_low in dt\n", __func__);
	else
		brightness_ratio_r_low = (int)temp;
	pr_info("leds-max77705-rgb: %s, brightness_ratio_r_low = %x\n",	__func__, brightness_ratio_r_low);

	/* get led green brightness ratio lowpower*/
	ret = of_property_read_u32(np, br_ratio_g_low, &temp);
	if (ret < 0)
		pr_info("leds-max77705-rgb: %s, can't parsing brightness_ratio_g_low in dt\n", __func__);
	else
		brightness_ratio_g_low = (int)temp;
	pr_info("leds-max77705-rgb: %s, brightness_ratio_g_low = %x\n",	__func__, brightness_ratio_g_low);

	/* get led blue brightness ratio lowpower */
	ret = of_property_read_u32(np, br_ratio_b_low, &temp);
	if (ret < 0)
		pr_info("leds-max77705-rgb: %s, can't parsing brightness_ratio_b_low in dt\n", __func__);
	else
		brightness_ratio_b_low = (int)temp;
	pr_info("leds-max77705-rgb: %s, brightness_ratio_b_low = %x\n", __func__, brightness_ratio_b_low);

	return pdata;
}
#endif

static void max77705_rgb_reset(struct device *dev)
{
	struct max77705_rgb *max77705_rgb = dev_get_drvdata(dev);

	max77705_rgb_set_state(&max77705_rgb->led[RED], LED_OFF, LED_DISABLE);
	max77705_rgb_set_state(&max77705_rgb->led[GREEN], LED_OFF, LED_DISABLE);
	max77705_rgb_set_state(&max77705_rgb->led[BLUE], LED_OFF, LED_DISABLE);
	max77705_rgb_ramp(dev, 0, 0);
	max77705_rgb_blink(dev, 0, 0);
}

static ssize_t show_max77705_rgb_lowpower(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return snprintf(buf, RGB_BUFSIZE, "%d\n", led_lowpower_mode);
}

static ssize_t store_max77705_rgb_lowpower(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	int ret;
	u8 led_lowpower;

	ret = kstrtou8(buf, 0, &led_lowpower);
	if (ret != 0) {
		dev_err(dev, "fail to get led_lowpower.\n");
		return count;
	}

	led_lowpower_mode = led_lowpower;

	pr_info("leds-max77705-rgb: led_lowpower mode set to %i\n", led_lowpower);

	return count;
}

static ssize_t show_max77705_rgb_brightness(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return snprintf(buf, RGB_BUFSIZE, "%d\n", led_dynamic_current);
}

static ssize_t store_max77705_rgb_brightness(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	int ret;
	u8 brightness;

	pr_info("leds-max77705-rgb: %s\n", __func__);

	ret = kstrtou8(buf, 0, &brightness);
	if (ret != 0) {
		dev_err(dev, "fail to get led_brightness.\n");
		return count;
	}

	led_lowpower_mode = 0;

	led_dynamic_current = brightness;

	dev_dbg(dev, "led brightness set to %i\n", brightness);

	return count;
}

static ssize_t show_max77705_rgb_pattern(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return snprintf(buf, RGB_BUFSIZE, "%d\n", led_pattern_num);
}

static ssize_t store_max77705_rgb_pattern(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	struct max77705_rgb *max77705_rgb = dev_get_drvdata(dev);
	unsigned int mode = 0;
	int ret;

	ret = sscanf(buf, "%1d", &mode);
	if (ret == 0) {
		dev_err(dev, "fail to get led_pattern mode.\n");
		return count;
	}
	pr_info("leds-max77705-rgb: %s pattern=%d lowpower=%i\n", __func__, mode, led_lowpower_mode);
	led_pattern_num = mode;

	/* Set all LEDs Off */
	max77705_rgb_reset(dev);
	if (mode == PATTERN_OFF)
		return count;

	/* Set to low power consumption mode */
	if (led_lowpower_mode == 1)
		led_dynamic_current = low_powermode_current;
	else
		led_dynamic_current = normal_powermode_current;

	switch (mode) {

	case CHARGING:
		max77705_rgb_set_state(&max77705_rgb->led[RED], led_dynamic_current, LED_ALWAYS_ON);
		break;
	case CHARGING_ERR:
		max77705_rgb_blink(dev, 500, 500);
		max77705_rgb_set_state(&max77705_rgb->led[RED], led_dynamic_current, LED_BLINK);
		break;
	case MISSED_NOTI:
		max77705_rgb_blink(dev, 500, 5000);
		max77705_rgb_set_state(&max77705_rgb->led[BLUE], led_dynamic_current, LED_BLINK);
		break;
	case LOW_BATTERY:
		max77705_rgb_blink(dev, 500, 5000);
		max77705_rgb_set_state(&max77705_rgb->led[RED], led_dynamic_current, LED_BLINK);
		break;
	case FULLY_CHARGED:
		max77705_rgb_set_state(&max77705_rgb->led[GREEN], led_dynamic_current, LED_ALWAYS_ON);
		break;
	case POWERING:
		max77705_rgb_ramp(dev, 800, 800);
		max77705_rgb_blink(dev, 200, 200);
		max77705_rgb_set_state(&max77705_rgb->led[BLUE], led_dynamic_current, LED_ALWAYS_ON);
		max77705_rgb_set_state(&max77705_rgb->led[GREEN], led_dynamic_current, LED_BLINK);
		break;
	default:
		break;
	}

	return count;
}

static ssize_t show_max77705_rgb_blink(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct max77705_rgb *max77705_rgb = dev_get_drvdata(dev);
	unsigned int led_r_en = 0, led_g_en = 0, led_b_en = 0;
	int ret = 0;

	led_r_en = max77705_rgb_get(&max77705_rgb->led[RED]);
	led_g_en = max77705_rgb_get(&max77705_rgb->led[GREEN]);
	led_b_en = max77705_rgb_get(&max77705_rgb->led[BLUE]);

	ret = !!(led_r_en | led_g_en | led_b_en);

	return snprintf(buf, RGB_BUFSIZE, "%d\n", ret);
}

static ssize_t store_max77705_rgb_blink(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	struct max77705_rgb *max77705_rgb = dev_get_drvdata(dev);
	int led_brightness = 0;
	int delay_on_time = 0;
	int delay_off_time = 0;
	u8 led_r_brightness = 0;
	u8 led_g_brightness = 0;
	u8 led_b_brightness = 0;
	unsigned int led_total_br = 0;
	unsigned int led_max_br = 0;
	u8 led_en = 0;
	int led_num = 0;
	int ret;

	ret = sscanf(buf, "0x%8x %5d %5d", &led_brightness,
					&delay_on_time, &delay_off_time);
	if (ret == 0) {
		dev_err(dev, "fail to get led_blink value.\n");
		return count;
	}

	/* Set to low power consumption mode */
	if (led_lowpower_mode == 1)
		led_dynamic_current = low_powermode_current;
	else
		led_dynamic_current = normal_powermode_current;
	/*Reset led*/
	max77705_rgb_reset(dev);

	led_r_brightness = (led_brightness & LED_R_MASK) >> 16;
	led_g_brightness = (led_brightness & LED_G_MASK) >> 8;
	led_b_brightness = led_brightness & LED_B_MASK;

	/* In user case, LED current is restricted to less than tuning value */
	if (led_r_brightness != 0) {
		led_r_brightness = (led_r_brightness * led_dynamic_current) / LED_MAX_CURRENT;
		if (led_r_brightness == 0)
			led_r_brightness = 1;
	}
	if (led_g_brightness != 0) {
		led_g_brightness = (led_g_brightness * led_dynamic_current) / LED_MAX_CURRENT;
		if (led_g_brightness == 0)
			led_g_brightness = 1;
	}
	if (led_b_brightness != 0) {
		led_b_brightness = (led_b_brightness * led_dynamic_current) / LED_MAX_CURRENT;
		if (led_b_brightness == 0)
			led_b_brightness = 1;
	}

	led_total_br += led_r_brightness * brightness_ratio_r / 100;
	led_total_br += led_g_brightness * brightness_ratio_g / 100;
	led_total_br += led_b_brightness * brightness_ratio_b / 100;

	if (brightness_ratio_r >= brightness_ratio_g &&
		brightness_ratio_r >= brightness_ratio_b) {
		led_max_br = normal_powermode_current * brightness_ratio_r / 100;
	} else if (brightness_ratio_g >= brightness_ratio_r &&
		brightness_ratio_g >= brightness_ratio_b) {
		led_max_br = normal_powermode_current * brightness_ratio_g / 100;
	} else if (brightness_ratio_b >= brightness_ratio_r &&
		brightness_ratio_b >= brightness_ratio_g) {
		led_max_br = normal_powermode_current * brightness_ratio_b / 100;
	}

	/* Each color decreases according to the limit at the same rate. */
	if (led_total_br > led_max_br) {
		if (led_r_brightness != 0) {
			led_r_brightness = led_r_brightness * led_max_br / led_total_br;
			if (led_r_brightness == 0)
				led_r_brightness = 1;
		}
		if (led_g_brightness != 0) {
			led_g_brightness = led_g_brightness * led_max_br / led_total_br;
			if (led_g_brightness == 0)
				led_g_brightness = 1;
		}
		if (led_b_brightness != 0) {
			led_b_brightness = led_b_brightness * led_max_br / led_total_br;
			if (led_b_brightness == 0)
				led_b_brightness = 1;
		}
	}

	if (led_r_brightness) {
		max77705_rgb_set_state(&max77705_rgb->led[RED], led_r_brightness, LED_DISABLE);
		led_num = max77705_rgb_number(&max77705_rgb->led[RED], &max77705_rgb);
		led_en |= LED_BLINK << 2*led_num;
	}

	if (led_g_brightness) {
		max77705_rgb_set_state(&max77705_rgb->led[GREEN], led_g_brightness, LED_DISABLE);
		led_num = max77705_rgb_number(&max77705_rgb->led[GREEN], &max77705_rgb);
		led_en |= LED_BLINK << 2*led_num;
	}

	if (led_b_brightness) {
		max77705_rgb_set_state(&max77705_rgb->led[BLUE], led_b_brightness, LED_DISABLE);
		led_num = max77705_rgb_number(&max77705_rgb->led[BLUE], &max77705_rgb);
		led_en |= LED_BLINK << 2*led_num;
	}

	/*Set LED blink mode*/
	max77705_rgb_blink(dev, delay_on_time, delay_off_time);

	ret = max77705_update_reg(max77705_rgb->i2c, MAX77705_RGBLED_REG_LEDEN, led_en, 0xff);
	if (ret < 0)
		dev_err(dev, "can't write FLASH_EN : %d\n", ret);

	pr_info("leds-max77705-rgb: %s, delay_on_time: %d, delay_off_time: %d, color: 0x%x, lowpower: %i\n",
			__func__, delay_on_time, delay_off_time, led_brightness, led_lowpower_mode);

	return count;
}

static ssize_t store_led_r(struct device *dev,
			struct device_attribute *devattr,
				const char *buf, size_t count)
{
	struct max77705_rgb *max77705_rgb = dev_get_drvdata(dev);
	unsigned int brightness;
	int ret;

	ret = kstrtouint(buf, 0, &brightness);
	if (ret != 0) {
		dev_err(dev, "fail to get brightness.\n");
		goto out;
	}
	if (brightness != 0)
		max77705_rgb_set_state(&max77705_rgb->led[RED], brightness, LED_ALWAYS_ON);
	else
		max77705_rgb_set_state(&max77705_rgb->led[RED], LED_OFF, LED_DISABLE);

out:
	pr_info("leds-max77705-rgb: %s\n", __func__);
	return count;
}
static ssize_t store_led_g(struct device *dev,
			struct device_attribute *devattr,
			const char *buf, size_t count)
{
	struct max77705_rgb *max77705_rgb = dev_get_drvdata(dev);
	unsigned int brightness;
	int ret;

	ret = kstrtouint(buf, 0, &brightness);
	if (ret != 0) {
		dev_err(dev, "fail to get brightness.\n");
		goto out;
	}
	if (brightness != 0)
		max77705_rgb_set_state(&max77705_rgb->led[GREEN], brightness, LED_ALWAYS_ON);
	else
		max77705_rgb_set_state(&max77705_rgb->led[GREEN], LED_OFF, LED_DISABLE);

out:
	pr_info("leds-max77705-rgb: %s\n", __func__);
	return count;
}
static ssize_t store_led_b(struct device *dev,
		struct device_attribute *devattr,
		const char *buf, size_t count)
{
	struct max77705_rgb *max77705_rgb = dev_get_drvdata(dev);
	unsigned int brightness;
	int ret;

	ret = kstrtouint(buf, 0, &brightness);
	if (ret != 0) {
		dev_err(dev, "fail to get brightness.\n");
		goto out;
	}
	if (brightness != 0)
		max77705_rgb_set_state(&max77705_rgb->led[BLUE], brightness, LED_ALWAYS_ON);
	else
		max77705_rgb_set_state(&max77705_rgb->led[BLUE], LED_OFF, LED_DISABLE);

out:
	pr_info("leds-max77705-rgb: %s\n", __func__);
	return count;
}

/* Added for led common class */
static ssize_t led_delay_on_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct max77705_rgb *max77705_rgb = dev_get_drvdata(dev->parent);

	return snprintf(buf, RGB_BUFSIZE, "%d\n", max77705_rgb->delay_on_times_ms);
}

static ssize_t led_delay_on_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct max77705_rgb *max77705_rgb = dev_get_drvdata(dev->parent);
	unsigned int time;

	if (kstrtouint(buf, 0, &time)) {
		dev_err(dev, "can not write led_delay_on\n");
		return count;
	}

	max77705_rgb->delay_on_times_ms = time;

	return count;
}

static ssize_t led_delay_off_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct max77705_rgb *max77705_rgb = dev_get_drvdata(dev->parent);

	return snprintf(buf, RGB_BUFSIZE, "%d\n", max77705_rgb->delay_off_times_ms);
}

static ssize_t led_delay_off_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct max77705_rgb *max77705_rgb = dev_get_drvdata(dev->parent);
	unsigned int time;

	if (kstrtouint(buf, 0, &time)) {
		dev_err(dev, "can not write led_delay_off\n");
		return count;
	}

	max77705_rgb->delay_off_times_ms = time;

	return count;
}

static ssize_t led_blink_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct max77705_rgb *max77705_rgb = dev_get_drvdata(dev->parent);
	unsigned int blink_set;
	int ret;

	ret = sscanf(buf, "%1d", &blink_set);
	if (!ret) {
		dev_err(dev, "can not write led_blink\n");
		return count;
	}

	if (!blink_set) {
		max77705_rgb->delay_on_times_ms = LED_OFF;
		max77705_rgb->delay_off_times_ms = LED_OFF;
	}

	max77705_rgb_blink(dev->parent,
		max77705_rgb->delay_on_times_ms,
		max77705_rgb->delay_off_times_ms);
	max77705_rgb_set_state(led_cdev, led_dynamic_current, LED_BLINK);

	pr_info("leds-max77705-rgb: %s\n", __func__);
	return count;
}

/* permission for sysfs node */
static DEVICE_ATTR(delay_on, 0640, led_delay_on_show, led_delay_on_store);
static DEVICE_ATTR(delay_off, 0640, led_delay_off_show, led_delay_off_store);
static DEVICE_ATTR(blink, 0220, NULL, led_blink_store);

#ifdef SEC_LED_SPECIFIC
/* below nodes is SAMSUNG specific nodes */
static DEVICE_ATTR(led_r, 0220, NULL, store_led_r);
static DEVICE_ATTR(led_g, 0220, NULL, store_led_g);
static DEVICE_ATTR(led_b, 0220, NULL, store_led_b);
/* led_pattern node permission is 222 */
/* To access sysfs node from other groups */
static DEVICE_ATTR(led_pattern, 0660, show_max77705_rgb_pattern, store_max77705_rgb_pattern);
static DEVICE_ATTR(led_blink, 0660, show_max77705_rgb_blink,  store_max77705_rgb_blink);
static DEVICE_ATTR(led_brightness, 0660, show_max77705_rgb_brightness, store_max77705_rgb_brightness);
static DEVICE_ATTR(led_lowpower, 0660, show_max77705_rgb_lowpower,  store_max77705_rgb_lowpower);
#endif

static struct attribute *led_class_attrs[] = {
	&dev_attr_delay_on.attr,
	&dev_attr_delay_off.attr,
	&dev_attr_blink.attr,
	NULL,
};

static struct attribute_group common_led_attr_group = {
	.attrs = led_class_attrs,
};

#ifdef SEC_LED_SPECIFIC
static struct attribute *sec_led_attributes[] = {
	&dev_attr_led_r.attr,
	&dev_attr_led_g.attr,
	&dev_attr_led_b.attr,
	&dev_attr_led_pattern.attr,
	&dev_attr_led_blink.attr,
	&dev_attr_led_brightness.attr,
	&dev_attr_led_lowpower.attr,
	NULL,
};

static struct attribute_group sec_led_attr_group = {
	.attrs = sec_led_attributes,
};
#endif
static int max77705_rgb_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct max77705_rgb_platform_data *pdata;
	struct max77705_rgb *max77705_rgb;
	struct max77705_dev *max77705_dev = dev_get_drvdata(dev->parent);
	char name[40] = {0,}, *p;
	int i, ret;

	pr_info("leds-max77705-rgb: %s\n", __func__);

	octa_color = get_lcd_info("window_color");
#ifdef CONFIG_OF
	pdata = max77705_rgb_parse_dt(dev);
	if (unlikely(IS_ERR(pdata)))
		return PTR_ERR(pdata);

	led_dynamic_current = normal_powermode_current;
#else
	pdata = dev_get_platdata(dev);
#endif

	pr_info("leds-max77705-rgb: %s : octa_color=%x led_device_type=%x\n",
		__func__, octa_color, led_device_type);
	max77705_rgb = devm_kzalloc(dev, sizeof(struct max77705_rgb), GFP_KERNEL);
	if (unlikely(!max77705_rgb))
		return -ENOMEM;

	max77705_rgb->i2c = max77705_dev->i2c;
	platform_set_drvdata(pdev, max77705_rgb);

	for (i = 0; i < 4; i++) {
		ret = snprintf(name, RGB_BUFSIZE, "%s", pdata->name[i])+1;
		if (ret < 1)
			goto alloc_err_flash;

		p = devm_kzalloc(dev, ret, GFP_KERNEL);
		if (unlikely(!p))
			goto alloc_err_flash;

		strncpy(p, name, strlen(name));
		max77705_rgb->led[i].name = p;
		max77705_rgb->led[i].brightness_set = max77705_rgb_set;
		max77705_rgb->led[i].brightness_get = max77705_rgb_get;
		max77705_rgb->led[i].max_brightness = LED_MAX_CURRENT;

		ret = led_classdev_register(dev, &max77705_rgb->led[i]);
		if (ret < 0) {
			dev_err(dev, "unable to register RGB : %d\n", ret);
			goto alloc_err_flash_plus;
		}
		ret = sysfs_create_group(&max77705_rgb->led[i].dev->kobj,
						&common_led_attr_group);
		if (ret) {
			dev_err(dev, "can not register sysfs attribute\n");
			goto register_err_flash;
		}
	}

	led_dev = sec_device_create(max77705_rgb, "led");
	if (IS_ERR(led_dev)) {
		dev_err(dev, "Failed to create device for samsung specific led\n");
		goto create_err_flash;
	}

	ret = sysfs_create_group(&led_dev->kobj, &sec_led_attr_group);
	if (ret < 0) {
		dev_err(dev, "Failed to create sysfs group for samsung specific led\n");
		goto device_create_err;
	}

	pr_info("leds-max77705-rgb: %s done\n", __func__);

	return 0;

device_create_err:
	sec_device_destroy(led_dev->devt);
create_err_flash:
	sysfs_remove_group(&led_dev->kobj, &common_led_attr_group);
register_err_flash:
	led_classdev_unregister(&max77705_rgb->led[i]);
alloc_err_flash_plus:
alloc_err_flash:
	while (i--)
		led_classdev_unregister(&max77705_rgb->led[i]);

	devm_kfree(dev, max77705_rgb);
	return -ENOMEM;
}

static int max77705_rgb_remove(struct platform_device *pdev)
{
	struct max77705_rgb *max77705_rgb = platform_get_drvdata(pdev);
	int i;

	sysfs_remove_group(&led_dev->kobj, &sec_led_attr_group);
	sec_device_destroy(led_dev->devt);
	for (i = 0; i < 4; i++) {
		sysfs_remove_group(&max77705_rgb->led[i].dev->kobj,
						&common_led_attr_group);
		led_classdev_unregister(&max77705_rgb->led[i]);
	}

	return 0;
}

static void max77705_rgb_shutdown(struct platform_device *pdev)
{
	struct max77705_rgb *max77705_rgb = platform_get_drvdata(pdev);
	int i;

	if (!max77705_rgb->i2c)
		return;

	max77705_rgb_reset(&pdev->dev);

	sysfs_remove_group(&led_dev->kobj, &sec_led_attr_group);

	for (i = 0; i < 4; i++) {
		sysfs_remove_group(&max77705_rgb->led[i].dev->kobj,
						&common_led_attr_group);
		led_classdev_unregister(&max77705_rgb->led[i]);
	}
	devm_kfree(&pdev->dev, max77705_rgb);
}

static struct platform_driver max77705_fled_driver = {
	.driver		= {
		.name	= "leds-max77705-rgb",
		.owner	= THIS_MODULE,
	},
	.probe		= max77705_rgb_probe,
	.remove		= max77705_rgb_remove,
	.shutdown	= max77705_rgb_shutdown,
};

static int __init max77705_rgb_init(void)
{
	pr_info("leds-max77705-rgb: %s\n", __func__);
	return platform_driver_register(&max77705_fled_driver);
}
module_init(max77705_rgb_init);

static void __exit max77705_rgb_exit(void)
{
	platform_driver_unregister(&max77705_fled_driver);
}
module_exit(max77705_rgb_exit);

MODULE_ALIAS("platform:max77705-rgb");
MODULE_AUTHOR("Jeongwoong Lee<jell.lee@samsung.com>");
MODULE_DESCRIPTION("MAX77705 RGB driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");
