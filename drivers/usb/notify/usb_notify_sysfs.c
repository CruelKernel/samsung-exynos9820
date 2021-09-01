// SPDX-License-Identifier: GPL-2.0
/*
 *  drivers/usb/notify/usb_notify_sysfs.c
 *
 * Copyright (C) 2015-2020 Samsung, Inc.
 * Author: Dongrak Shin <dongrak.shin@samsung.com>
 *
 */

 /* usb notify layer v3.5 */

#define pr_fmt(fmt) "usb_notify: " fmt

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/usb.h>
#include <linux/usb_notify.h>
#include <linux/string.h>
#include "usb_notify_sysfs.h"

#define MAX_STRING_LEN 20

#if defined(CONFIG_USB_HW_PARAM)
const char
usb_hw_param_print[USB_CCIC_HW_PARAM_MAX][MAX_HWPARAM_STRING] = {
	{"CC_WATER"},
	{"CC_DRY"},
	{"CC_I2C"},
	{"CC_OVC"},
	{"CC_OTG"},
	{"CC_DP"},
	{"CC_VR"},
	{"H_SUPER"},
	{"H_HIGH"},
	{"H_FULL"},
	{"H_LOW"},
	{"C_SUPER"},
	{"C_HIGH"},
	{"H_AUDIO"},
	{"H_COMM"},
	{"H_HID"},
	{"H_PHYSIC"},
	{"H_IMAGE"},
	{"H_PRINTER"},
	{"H_STORAGE"},
	{"H_STO_S"},
	{"H_STO_H"},
	{"H_STO_F"},
	{"H_HUB"},
	{"H_CDC"},
	{"H_CSCID"},
	{"H_CONTENT"},
	{"H_VIDEO"},
	{"H_WIRE"},
	{"H_MISC"},
	{"H_APP"},
	{"H_VENDOR"},
	{"CC_DEX"},
	{"CC_WTIME"},
	{"CC_WVBUS"},
	{"CC_WVTIME"},
	{"CC_WLVBS"},
	{"CC_WLVTM"},
	{"CC_CSHORT"},
	{"CC_SVSHT"},
	{"CC_SGSHT"},
	{"M_AFCNAK"},
	{"M_AFCERR"},
	{"M_DCDTMO"},
	{"F_CNT"},
	{"CC_KILLER"},
	{"CC_FWERR"},
	{"C_ARP"},
	{"CC_VER"},
};
#endif

struct notify_data {
	struct class *usb_notify_class;
	atomic_t device_count;
};

static struct notify_data usb_notify_data;

static int is_valid_cmd(char *cur_cmd, char *prev_cmd)
{
	pr_info("%s : current state=%s, previous state=%s\n",
		__func__, cur_cmd, prev_cmd);

	if (!strcmp(cur_cmd, "ON") ||
			!strncmp(cur_cmd, "ON_ALL_", 7)) {
		if (!strcmp(prev_cmd, "ON") ||
				!strncmp(prev_cmd, "ON_ALL_", 7)) {
			goto ignore;
		} else if (!strncmp(prev_cmd, "ON_HOST_", 8)) {
			goto all;
		} else if (!strncmp(prev_cmd, "ON_CLIENT_", 10)) {
			goto all;
		} else if (!strcmp(prev_cmd, "OFF")) {
			goto all;
		} else {
			goto invalid;
		}
	} else if (!strcmp(cur_cmd, "OFF")) {
		if (!strcmp(prev_cmd, "ON") ||
				!strncmp(prev_cmd, "ON_ALL_", 7)) {
			goto off;
		} else if (!strncmp(prev_cmd, "ON_HOST_", 8)) {
			goto off;
		} else if (!strncmp(prev_cmd, "ON_CLIENT_", 10)) {
			goto off;
		} else if (!strcmp(prev_cmd, "OFF")) {
			goto ignore;
		} else {
			goto invalid;
		}
	} else if (!strncmp(cur_cmd, "ON_HOST_", 8)) {
		if (!strcmp(prev_cmd, "ON") ||
				!strncmp(prev_cmd, "ON_ALL_", 7)) {
			goto host;
		} else if (!strncmp(prev_cmd, "ON_HOST_", 8)) {
			goto ignore;
		} else if (!strncmp(prev_cmd, "ON_CLIENT_", 10)) {
			goto host;
		} else if (!strcmp(prev_cmd, "OFF")) {
			goto host;
		} else {
			goto invalid;
		}
	} else if (!strncmp(cur_cmd, "ON_CLIENT_", 10)) {
		if (!strcmp(prev_cmd, "ON") ||
				!strncmp(prev_cmd, "ON_ALL_", 7)) {
			goto client;
		} else if (!strncmp(prev_cmd, "ON_HOST_", 8)) {
			goto client;
		} else if (!strncmp(prev_cmd, "ON_CLIENT_", 10)) {
			goto ignore;
		} else if (!strcmp(prev_cmd, "OFF")) {
			goto client;
		} else {
			goto invalid;
		}
	} else {
		goto invalid;
	}
host:
	pr_info("%s cmd=%s is accepted.\n", __func__, cur_cmd);
	return NOTIFY_BLOCK_TYPE_HOST;
client:
	pr_info("%s cmd=%s is accepted.\n", __func__, cur_cmd);
	return NOTIFY_BLOCK_TYPE_CLIENT;
all:
	pr_info("%s cmd=%s is accepted.\n", __func__, cur_cmd);
	return NOTIFY_BLOCK_TYPE_ALL;
off:
	pr_info("%s cmd=%s is accepted.\n", __func__, cur_cmd);
	return NOTIFY_BLOCK_TYPE_NONE;
ignore:
	pr_err("%s cmd=%s is ignored but saved.\n", __func__, cur_cmd);
	return -EEXIST;
invalid:
	pr_err("%s cmd=%s is invalid.\n", __func__, cur_cmd);
	return -EINVAL;
}

static ssize_t disable_show(
	struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);

	pr_info("read disable_state %s\n", udev->disable_state_cmd);
	return sprintf(buf, "%s\n", udev->disable_state_cmd);
}

static ssize_t disable_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);

	char *disable;
	int sret, param = -EINVAL;
	size_t ret = -ENOMEM;

	if (size > MAX_DISABLE_STR_LEN) {
		pr_err("%s size(%zu) is too long.\n", __func__, size);
		goto error;
	}

	if (size < strlen(buf))
		goto error;
	disable = kzalloc(size+1, GFP_KERNEL);
	if (!disable)
		goto error;

	sret = sscanf(buf, "%s", disable);
	if (sret != 1)
		goto error1;

	if (udev->set_disable) {
		param = is_valid_cmd(disable, udev->disable_state_cmd);
		if (param == -EINVAL) {
			ret = param;
		} else {
			if (param != -EEXIST)
				udev->set_disable(udev, param);
			strncpy(udev->disable_state_cmd,
				disable, sizeof(udev->disable_state_cmd)-1);
			ret = size;
		}
	} else
		pr_err("set_disable func is NULL\n");
error1:
	kfree(disable);
error:
	return ret;
}

static ssize_t support_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	struct otg_notify *n = udev->o_notify;
	char *support;

	if (n->unsupport_host || !IS_ENABLED(CONFIG_USB_HOST_NOTIFY))
		support = "CLIENT";
	else
		support = "ALL";

	pr_info("read support %s\n", support);
	return snprintf(buf,  sizeof(support)+1, "%s\n", support);
}

static ssize_t otg_speed_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	struct otg_notify *n = udev->o_notify;
	char *speed;

	switch (n->speed) {
	case USB_SPEED_SUPER_PLUS:
		speed = "SUPER PLUS";
		break;
	case USB_SPEED_SUPER:
		speed = "SUPER";
		break;
	case USB_SPEED_HIGH:
		speed = "HIGH";
		break;
	case USB_SPEED_FULL:
		speed = "FULL";
		break;
	case USB_SPEED_LOW:
		speed = "LOW";
		break;
	default:
		speed = "UNKNOWN";
		break;
	}
	pr_info("%s : read otg speed %s\n", __func__, speed);
	return snprintf(buf,  sizeof(speed)+1, "%s\n", speed);
}

static ssize_t gadget_speed_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	struct otg_notify *n = udev->o_notify;
	const char *speed;

	if (n->get_gadget_speed)
		speed = usb_speed_string(n->get_gadget_speed());
	else
		speed = "UNKNOWN";

	pr_info("%s : read gadget speed %s\n", __func__, speed);
	return snprintf(buf,  MAX_STRING_LEN, "%s\n", speed);
}

#if defined(CONFIG_USB_HW_PARAM)
static unsigned long long strtoull(char *ptr, char **end, int base)
{
	unsigned long long ret = 0;

	if (base > 36)
		goto out;

	while (*ptr) {
		int digit;

		if (*ptr >= '0' && *ptr <= '9' && *ptr < '0' + base)
			digit = *ptr - '0';
		else if (*ptr >= 'A' && *ptr < 'A' + base - 10)
			digit = *ptr - 'A' + 10;
		else if (*ptr >= 'a' && *ptr < 'a' + base - 10)
			digit = *ptr - 'a' + 10;
		else
			break;

		ret *= base;
		ret += digit;
		ptr++;
	}

out:
	if (end)
		*end = (char *)ptr;

	return ret;
}

static ssize_t usb_hw_param_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	struct otg_notify *n = udev->o_notify;
	int index, ret = 0;
	unsigned long long *p_param = NULL;

	if (udev->fp_hw_param_manager) {
		p_param = get_hw_param(n, USB_CCIC_WATER_INT_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_INT_COUNT);
		p_param = get_hw_param(n, USB_CCIC_DRY_INT_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_DRY_INT_COUNT);
		p_param = get_hw_param(n, USB_CLIENT_SUPER_SPEED_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CLIENT_SUPER_SPEED_COUNT);
		p_param = get_hw_param(n, USB_CLIENT_HIGH_SPEED_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CLIENT_HIGH_SPEED_COUNT);
		p_param = get_hw_param(n, USB_CCIC_WATER_TIME_DURATION);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_TIME_DURATION);
		p_param = get_hw_param(n, USB_CCIC_WATER_VBUS_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_VBUS_COUNT);
		p_param = get_hw_param(n, USB_CCIC_WATER_LPM_VBUS_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_LPM_VBUS_COUNT);
		p_param = get_hw_param(n, USB_CCIC_WATER_VBUS_TIME_DURATION);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_VBUS_TIME_DURATION);
		p_param = get_hw_param(n,
				USB_CCIC_WATER_LPM_VBUS_TIME_DURATION);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_LPM_VBUS_TIME_DURATION);
	}
	p_param = get_hw_param(n, USB_CCIC_VERSION);
	if (p_param)
		*p_param = show_ccic_version();
	for (index = 0; index < USB_CCIC_HW_PARAM_MAX - 1; index++) {
		p_param = get_hw_param(n, index);
		if (p_param)
			ret += sprintf(buf + ret, "%llu ", *p_param);
		else
			ret += sprintf(buf + ret, "0 ");
	}
	p_param = get_hw_param(n, index);
	if (p_param)
		ret += sprintf(buf + ret, "%llu\n", *p_param);
	else
		ret += sprintf(buf + ret, "0\n");
	pr_info("%s - ret : %d\n", __func__, ret);

	return ret;
}

static ssize_t usb_hw_param_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	struct otg_notify *n = udev->o_notify;
	unsigned long long prev_hw_param[USB_CCIC_HW_PARAM_MAX] = {0, };
	unsigned long long *p_param = NULL;
	int index = 0;
	size_t ret = -ENOMEM;
	char *token, *str = (char *)buf;

	if (size > MAX_HWPARAM_STR_LEN) {
		pr_err("%s size(%zu) is too long.\n", __func__, size);
		goto error;
	}
	ret = size;
	if (size < USB_CCIC_HW_PARAM_MAX) {
		pr_err("%s efs file is not created correctly.\n", __func__);
		goto error;
	}

	for (index = 0; index < (USB_CCIC_HW_PARAM_MAX - 1); index++) {
		token = strsep(&str, " ");
		if (token)
			prev_hw_param[index] = strtoull(token, NULL, 10);

		if (!token || (prev_hw_param[index] > HWPARAM_DATA_LIMIT))
			goto error;
	}

	for (index = 0; index < (USB_CCIC_HW_PARAM_MAX - 1); index++) {
		p_param = get_hw_param(n, index);
		if (p_param)
			*p_param += prev_hw_param[index];
	}
	pr_info("%s - ret : %zu\n", __func__, ret);
error:
	return ret;
}

static ssize_t hw_param_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	struct otg_notify *n = udev->o_notify;
	int index, ret = 0;
	unsigned long long *p_param = NULL;

	if (udev->fp_hw_param_manager) {
		p_param = get_hw_param(n, USB_CCIC_WATER_INT_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_INT_COUNT);
		p_param = get_hw_param(n, USB_CCIC_DRY_INT_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_DRY_INT_COUNT);
		p_param = get_hw_param(n, USB_CLIENT_SUPER_SPEED_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CLIENT_SUPER_SPEED_COUNT);
		p_param = get_hw_param(n, USB_CLIENT_HIGH_SPEED_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CLIENT_HIGH_SPEED_COUNT);
		p_param = get_hw_param(n, USB_CCIC_WATER_TIME_DURATION);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_TIME_DURATION);
		p_param = get_hw_param(n, USB_CCIC_WATER_VBUS_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_VBUS_COUNT);
		p_param = get_hw_param(n, USB_CCIC_WATER_LPM_VBUS_COUNT);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_LPM_VBUS_COUNT);
		p_param = get_hw_param(n, USB_CCIC_WATER_VBUS_TIME_DURATION);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_VBUS_TIME_DURATION);
		p_param = get_hw_param(n,
				USB_CCIC_WATER_LPM_VBUS_TIME_DURATION);
		if (p_param)
			*p_param += udev->fp_hw_param_manager
					(USB_CCIC_WATER_LPM_VBUS_TIME_DURATION);
	}
	p_param = get_hw_param(n, USB_CCIC_VERSION);
	if (p_param)
		*p_param = show_ccic_version();
	for (index = 0; index < USB_CCIC_HW_PARAM_MAX - 1; index++) {
		p_param = get_hw_param(n, index);
		if (p_param)
			ret += sprintf(buf + ret, "\"%s\":\"%llu\",",
				usb_hw_param_print[index], *p_param);
		else
			ret += sprintf(buf + ret, "\"%s\":\"0\",",
				usb_hw_param_print[index]);
	}
	/* CCIC FW version */
	ret += sprintf(buf + ret, "\"%s\":\"",
		usb_hw_param_print[index]);
	p_param = get_hw_param(n, index);
	if (p_param) {
		/* HW Version */
		ret += sprintf(buf + ret, "%02X%02X%02X%02X",
			*((unsigned char *)p_param + 3),
			*((unsigned char *)p_param + 2),
			*((unsigned char *)p_param + 1),
			*((unsigned char *)p_param));
		/* SW Main Version */
		ret += sprintf(buf + ret, "%02X%02X%02X",
			*((unsigned char *)p_param + 6),
			*((unsigned char *)p_param + 5),
			*((unsigned char *)p_param + 4));
		/* SW Boot Version */
		ret += sprintf(buf + ret, "%02X",
			*((unsigned char *)p_param + 7));
		ret += sprintf(buf + ret, "\"\n");
	} else
		ret += sprintf(buf + ret, "0000000000000000\"\n");

	pr_info("%s - ret : %d\n", __func__, ret);
	return ret;
}

static ssize_t hw_param_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	struct otg_notify *n = udev->o_notify;
	int index = 0;
	size_t ret = -ENOMEM;
	char *str = (char *)buf;
	unsigned long long *p_param = NULL;

	if (size > 2) {
		pr_err("%s size(%zu) is too long.\n", __func__, size);
		goto error;
	}
	ret = size;
	pr_info("%s : %s\n", __func__, str);
	if (!strncmp(str, "c", 1))
		for (index = 0; index < USB_CCIC_HW_PARAM_MAX; index++) {
			p_param = get_hw_param(n, index);
			if (p_param)
				*p_param = 0;
		}
error:
	return ret;
}
#endif

char interface_class_name[USB_CLASS_VENDOR_SPEC][4] = {
	[U_CLASS_PER_INTERFACE]				= {"PER"},
	[U_CLASS_AUDIO]					= {"AUD"},
	[U_CLASS_COMM]					= {"COM"},
	[U_CLASS_HID]					= {"HID"},
	[U_CLASS_PHYSICAL]				= {"PHY"},
	[U_CLASS_STILL_IMAGE]				= {"STI"},
	[U_CLASS_PRINTER]				= {"PRI"},
	[U_CLASS_MASS_STORAGE]				= {"MAS"},
	[U_CLASS_HUB]					= {"HUB"},
	[U_CLASS_CDC_DATA]				= {"CDC"},
	[U_CLASS_CSCID]					= {"CSC"},
	[U_CLASS_CONTENT_SEC]				= {"CON"},
	[U_CLASS_VIDEO]					= {"VID"},
	[U_CLASS_WIRELESS_CONTROLLER]			= {"WIR"},
	[U_CLASS_MISC]					= {"MIS"},
	[U_CLASS_APP_SPEC]				= {"APP"},
	[U_CLASS_VENDOR_SPEC]				= {"VEN"}
};

void init_usb_whitelist_array(int *whitelist_array)
{
	int i;

	for (i = 1; i <= MAX_CLASS_TYPE_NUM; i++)
		whitelist_array[i] = 0;
}

int set_usb_whitelist_array(const char *buf, int *whitelist_array)
{
	int valid_class_count = 0;
	char *ptr = NULL;
	int i;
	char *source;

	source = (char *)buf;
	while ((ptr = strsep(&source, ":")) != NULL) {
		pr_info("%s token = %c%c%c!\n", __func__,
			ptr[0], ptr[1], ptr[2]);
		for (i = U_CLASS_PER_INTERFACE; i <= U_CLASS_VENDOR_SPEC; i++) {
			if (!strncmp(ptr, interface_class_name[i], 3))
				whitelist_array[i] = 1;
		}
	}

	for (i = U_CLASS_PER_INTERFACE; i <= U_CLASS_VENDOR_SPEC; i++) {
		if (whitelist_array[i])
			valid_class_count++;
	}
	pr_info("%s valid_class_count = %d!\n", __func__, valid_class_count);
	return valid_class_count;
}

static ssize_t whitelist_for_mdm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);

	if (udev == NULL) {
		pr_err("udev is NULL\n");
		return -EINVAL;
	}
	pr_info("%s read whitelist_classes %s\n",
		__func__, udev->whitelist_str);
	return sprintf(buf, "%s\n", udev->whitelist_str);
}

static ssize_t whitelist_for_mdm_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	char *disable;
	int sret;
	size_t ret = -ENOMEM;
	int mdm_disable;
	int valid_whilelist_count;

	if (udev == NULL) {
		pr_err("udev is NULL\n");
		ret = -EINVAL;
		goto error;
	}

	if (size > MAX_WHITELIST_STR_LEN) {
		pr_err("%s size(%zu) is too long.\n", __func__, size);
		goto error;
	}

	if (size < strlen(buf))
		goto error;
	disable = kzalloc(size+1, GFP_KERNEL);
	if (!disable)
		goto error;

	sret = sscanf(buf, "%s", disable);
	if (sret != 1)
		goto error1;
	pr_info("%s buf=%s\n", __func__, disable);

	init_usb_whitelist_array(udev->whitelist_array_for_mdm);
	/* To active displayport, hub class must be enabled */
	if (!strncmp(buf, "ABL", 3)) {
		udev->whitelist_array_for_mdm[U_CLASS_HUB] = 1;
		mdm_disable = NOTIFY_MDM_TYPE_ON;
	} else if (!strncmp(buf, "OFF", 3))
		mdm_disable = NOTIFY_MDM_TYPE_OFF;
	else {
		valid_whilelist_count =	set_usb_whitelist_array
			(buf, udev->whitelist_array_for_mdm);
		if (valid_whilelist_count > 0) {
			udev->whitelist_array_for_mdm[U_CLASS_HUB] = 1;
			mdm_disable = NOTIFY_MDM_TYPE_ON;
		} else
			mdm_disable = NOTIFY_MDM_TYPE_OFF;
	}

	strncpy(udev->whitelist_str,
		disable, sizeof(udev->whitelist_str)-1);

	if (udev->set_mdm) {
		udev->set_mdm(udev, mdm_disable);
		ret = size;
	} else {
		pr_err("set_mdm func is NULL\n");
		ret = -EINVAL;
	}
error1:
	kfree(disable);
error:
	return ret;
}

static ssize_t cards_show(
	struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);
	char card_strings[MAX_CARD_STR_LEN] = {0,};
	char buf_card[15] = {0,};
	int i;
	int cnt = 0;

	for (i = 0; i < MAX_USB_AUDIO_CARDS; i++) {
		if (udev->usb_audio_cards[i].cards) {
			cnt += snprintf(buf_card, sizeof(buf_card),
				"<%scard%d>",
				udev->usb_audio_cards[i].bundle ? "*" : "", i);
			if (cnt < 0) {
				pr_err("%s snprintf return %d\n",
						__func__, cnt);
				continue;
			}
			if (cnt >= MAX_CARD_STR_LEN) {
				pr_err("%s overflow\n", __func__);
				goto err;
			}
			strlcat(card_strings, buf_card, sizeof(card_strings));
		}
	}
err:
	pr_info("card_strings %s\n", card_strings);
	return sprintf(buf, "%s\n", card_strings);
}

int usb_notify_dev_uevent(struct usb_notify_dev *udev, char *envp_ext[])
{
	int ret = 0;

	if (!udev || !udev->dev) {
		pr_err("%s udev or udev->dev NULL\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	if (strncmp("TYPE", envp_ext[0], 4)) {
		pr_err("%s error.first array must be filled TYPE\n",
				__func__);
		ret = -EINVAL;
		goto err;
	}

	if (strncmp("STATE", envp_ext[1], 5)) {
		pr_err("%s error.second array must be filled STATE\n",
				__func__);
		ret = -EINVAL;
		goto err;
	}

	kobject_uevent_env(&udev->dev->kobj, KOBJ_CHANGE, envp_ext);
	pr_info("%s\n", __func__);

err:
	return ret;
}
EXPORT_SYMBOL_GPL(usb_notify_dev_uevent);

static DEVICE_ATTR_RW(disable);
static DEVICE_ATTR_RO(support);
static DEVICE_ATTR_RO(otg_speed);
static DEVICE_ATTR_RO(gadget_speed);
static DEVICE_ATTR_RW(whitelist_for_mdm);
static DEVICE_ATTR_RO(cards);
#if defined(CONFIG_USB_HW_PARAM)
static DEVICE_ATTR_RW(usb_hw_param);
static DEVICE_ATTR_RW(hw_param);
#endif

static struct attribute *usb_notify_attrs[] = {
	&dev_attr_disable.attr,
	&dev_attr_support.attr,
	&dev_attr_otg_speed.attr,
	&dev_attr_gadget_speed.attr,
	&dev_attr_whitelist_for_mdm.attr,
	&dev_attr_cards.attr,
#if defined(CONFIG_USB_HW_PARAM)
	&dev_attr_usb_hw_param.attr,
	&dev_attr_hw_param.attr,
#endif
	NULL,
};

static struct attribute_group usb_notify_attr_grp = {
	.attrs = usb_notify_attrs,
};

static int create_usb_notify_class(void)
{
	if (!usb_notify_data.usb_notify_class) {
		usb_notify_data.usb_notify_class
			= class_create(THIS_MODULE, "usb_notify");
		if (IS_ERR(usb_notify_data.usb_notify_class))
			return PTR_ERR(usb_notify_data.usb_notify_class);
		atomic_set(&usb_notify_data.device_count, 0);
	}

	return 0;
}

int usb_notify_dev_register(struct usb_notify_dev *udev)
{
	int ret;

	if (!usb_notify_data.usb_notify_class) {
		ret = create_usb_notify_class();
		if (ret < 0)
			return ret;
	}

	udev->index = atomic_inc_return(&usb_notify_data.device_count);
	udev->dev = device_create(usb_notify_data.usb_notify_class, NULL,
		MKDEV(0, udev->index), NULL, "%s", udev->name);
	if (IS_ERR(udev->dev))
		return PTR_ERR(udev->dev);

	udev->disable_state = 0;
	strncpy(udev->disable_state_cmd, "OFF",
			sizeof(udev->disable_state_cmd)-1);
	ret = sysfs_create_group(&udev->dev->kobj, &usb_notify_attr_grp);
	if (ret < 0) {
		device_destroy(usb_notify_data.usb_notify_class,
				MKDEV(0, udev->index));
		return ret;
	}

	dev_set_drvdata(udev->dev, udev);
	return 0;
}
EXPORT_SYMBOL_GPL(usb_notify_dev_register);

void usb_notify_dev_unregister(struct usb_notify_dev *udev)
{
	sysfs_remove_group(&udev->dev->kobj, &usb_notify_attr_grp);
	device_destroy(usb_notify_data.usb_notify_class, MKDEV(0, udev->index));
	dev_set_drvdata(udev->dev, NULL);
}
EXPORT_SYMBOL_GPL(usb_notify_dev_unregister);

int usb_notify_class_init(void)
{
	return create_usb_notify_class();
}
EXPORT_SYMBOL_GPL(usb_notify_class_init);

void usb_notify_class_exit(void)
{
	class_destroy(usb_notify_data.usb_notify_class);
}
EXPORT_SYMBOL_GPL(usb_notify_class_exit);

