/*
 * Copyright (C) 2015-2017 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

 /* usb notify layer v3.2 */

#define pr_fmt(fmt) "usb_notify: " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/usb.h>
#include <linux/notifier.h>
#include <linux/version.h>
#include <linux/usb_notify.h>
#include "../core/hub.h"

#define SMARTDOCK_INDEX	1
#define MMDOCK_INDEX	2

struct dev_table {
	struct usb_device_id dev;
	int index;
};

static struct dev_table enable_notify_hub_table[] = {
	{ .dev = { USB_DEVICE(0x0424, 0x2514), },
	   .index = SMARTDOCK_INDEX,
	}, /* SMART DOCK HUB 1 */
	{ .dev = { USB_DEVICE(0x1a40, 0x0101), },
	   .index = SMARTDOCK_INDEX,
	}, /* SMART DOCK HUB 2 */
	{ .dev = { USB_DEVICE(0x0424, 0x9512), },
	   .index = MMDOCK_INDEX,
	}, /* SMSC USB LAN HUB 9512 */
	{}
};

static struct dev_table essential_device_table[] = {
	{ .dev = { USB_DEVICE(0x08bb, 0x2704), },
	   .index = SMARTDOCK_INDEX,
	}, /* TI USB Audio DAC 1 */
	{ .dev = { USB_DEVICE(0x08bb, 0x27c4), },
	   .index = SMARTDOCK_INDEX,
	}, /* TI USB Audio DAC 2 */
	{ .dev = { USB_DEVICE(0x0424, 0xec00), },
	   .index = MMDOCK_INDEX,
	}, /* SMSC LAN Driver */
	{}
};

static struct dev_table update_autotimer_device_table[] = {
	{ .dev = { USB_DEVICE(0x04e8, 0xa500), },
	   .index = 5, /* 5 sec timer */
	}, /* GearVR1 */
	{ .dev = { USB_DEVICE(0x04e8, 0xa501), },
	   .index = 5,
	}, /* GearVR2 */
	{ .dev = { USB_DEVICE(0x04e8, 0xa502), },
	   .index = 5,
	}, /* GearVR3 */
	{}
};

static int check_essential_device(struct usb_device *dev, int index)
{
	struct dev_table *id;
	int ret = 0;

	/* check VID, PID */
	for (id = essential_device_table; id->dev.match_flags; id++) {
		if ((id->dev.match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
		(id->dev.match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
		id->dev.idVendor == le16_to_cpu(dev->descriptor.idVendor) &&
		id->dev.idProduct == le16_to_cpu(dev->descriptor.idProduct) &&
		id->index == index) {
			ret = 1;
			break;
		}
	}
	return ret;
}

static int check_gamepad_device(struct usb_device *dev)
{
	int ret = 0;

	pr_info("%s : product=%s\n", __func__, dev->product);

	if (!dev->product)
		return ret;

	if (!strncmp(dev->product, "Gamepad for SAMSUNG", 19))
		ret = 1;

	return ret;
}

static int check_lanhub_device(struct usb_device *dev)
{
	int ret = 0;

	pr_info("%s : product=%s\n", __func__, dev->product);

	if (!dev->product)
		return ret;

	if (!strncmp(dev->product, "LAN9512", 8))
		ret = 1;

	return ret;
}

static int is_notify_hub(struct usb_device *dev)
{
	struct dev_table *id;
	struct usb_device *hdev;
	int ret = 0;

	hdev = dev->parent;
	if (!hdev)
		goto skip;
	/* check VID, PID */
	for (id = enable_notify_hub_table; id->dev.match_flags; id++) {
		if ((id->dev.match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
		(id->dev.match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
		id->dev.idVendor == le16_to_cpu(hdev->descriptor.idVendor) &&
		id->dev.idProduct == le16_to_cpu(hdev->descriptor.idProduct)) {
			ret = (hdev->parent &&
			(hdev->parent == dev->bus->root_hub)) ? id->index : 0;
			break;
		}
	}
skip:
	return ret;
}

static int get_autosuspend_time(struct usb_device *dev)
{
	struct dev_table *id;
	int ret = 0;

	/* check VID, PID */
	for (id = update_autotimer_device_table; id->dev.match_flags; id++) {
		if ((id->dev.match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
		(id->dev.match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
		id->dev.idVendor == le16_to_cpu(dev->descriptor.idVendor) &&
		id->dev.idProduct == le16_to_cpu(dev->descriptor.idProduct)) {
			ret = id->index;
			break;
		}
	}
	return ret;
}

static int call_battery_notify(struct usb_device *dev, bool on)
{
	struct usb_device *hdev;
	struct usb_device *udev;
	struct usb_hub *hub;
	struct otg_notify *o_notify = get_otg_notify();
	int index = 0;
	int count = 0;
	int port;

	index = is_notify_hub(dev);
	if (!index)
		goto skip;
	if (check_essential_device(dev, index))
		goto skip;

	hdev = dev->parent;
	hub = usb_hub_to_struct_hub(hdev);
	if (!hub)
		goto skip;

	for (port = 1; port <= hdev->maxchild; port++) {
		udev = hub->ports[port-1]->child;
		if (udev) {
			if (!check_essential_device(udev, index)) {
				if (!on && (udev == dev))
					continue;
				else
					count++;
			}
		}
	}

	pr_info("%s : VID : 0x%x, PID : 0x%x, on=%d, count=%d\n", __func__,
		dev->descriptor.idVendor, dev->descriptor.idProduct,
			on, count);
	if (on) {
		if (count == 1) {
			if (index == SMARTDOCK_INDEX)
				send_otg_notify(o_notify,
					NOTIFY_EVENT_SMTD_EXT_CURRENT, 1);
			else if (index == MMDOCK_INDEX)
				send_otg_notify(o_notify,
					NOTIFY_EVENT_MMD_EXT_CURRENT, 1);
		}
	} else {
		if (!count) {
			if (index == SMARTDOCK_INDEX)
				send_otg_notify(o_notify,
					NOTIFY_EVENT_SMTD_EXT_CURRENT, 0);
			else if (index == MMDOCK_INDEX)
				send_otg_notify(o_notify,
					NOTIFY_EVENT_MMD_EXT_CURRENT, 0);
		}
	}
skip:
	return 0;
}

static int call_device_notify(struct usb_device *dev)
{
	struct otg_notify *o_notify = get_otg_notify();

	if (dev->bus->root_hub != dev) {
		pr_info("%s device\n", __func__);
		send_otg_notify(o_notify, NOTIFY_EVENT_DEVICE_CONNECT, 1);

		if (check_gamepad_device(dev))
			send_otg_notify(o_notify,
				NOTIFY_EVENT_GAMEPAD_CONNECT, 1);
		else if (check_lanhub_device(dev))
			send_otg_notify(o_notify,
				NOTIFY_EVENT_LANHUB_CONNECT, 1);
		else
			;
	} else
		pr_info("%s root hub\n", __func__);

	return 0;
}

static int update_hub_autosuspend_timer(struct usb_device *dev)
{
	struct usb_device *hdev;
	int time = 0;

	if (!dev)
		goto skip;

	hdev = dev->parent;

	if (hdev == NULL || dev->bus->root_hub != hdev)
		goto skip;

	/* hdev is root hub */
	time = get_autosuspend_time(dev);
	if (time == hdev->dev.power.autosuspend_delay)
		goto skip;

	pm_runtime_set_autosuspend_delay(&hdev->dev, time*1000);
	pr_info("set autosuspend delay time=%d sec\n", time);
skip:
	return 0;
}

static void check_device_speed(struct usb_device *dev, bool on)
{
	struct otg_notify *o_notify = get_otg_notify();
	struct usb_device *hdev;
	struct usb_device *udev;
	struct usb_hub *hub;
	int port = 0;
	int speed = USB_SPEED_UNKNOWN;
	static int hs_hub = 0;
	static int ss_hub = 0;

	if (!o_notify) {
		pr_err("%s otg_notify is null\n", __func__);
		return;
	}

	hdev = dev->parent;
	if (!hdev)
		return;
	
	hdev = dev->bus->root_hub;

	hub = usb_hub_to_struct_hub(hdev);

	/* check all ports */
	for (port = 1; port <= hdev->maxchild; port++) {
		udev = hub->ports[port-1]->child;
		if (udev) {
			if (!on && (udev == dev))
				continue;
			if (udev->speed > speed)
				speed = udev->speed;
		}
	}

	if (hdev->speed >= USB_SPEED_SUPER) {
		if (speed > USB_SPEED_UNKNOWN) 
			ss_hub = 1;
		else
			ss_hub = 0;
	} else if (hdev->speed > USB_SPEED_UNKNOWN
			&& hdev->speed != USB_SPEED_WIRELESS) {
		if (speed > USB_SPEED_UNKNOWN) 
			hs_hub = 1;
		else
			hs_hub = 0;
	} else ;

	if (ss_hub || hs_hub) {
		if (speed > o_notify->speed)
			o_notify->speed = speed;
	} else
		o_notify->speed = USB_SPEED_UNKNOWN;
	
	pr_info("%s : dev->speed %s %s\n", __func__,
				usb_speed_string(dev->speed), on ? "on" : "off");

	pr_info("%s : o_notify->speed %s\n", __func__,
				usb_speed_string(o_notify->speed));
}

#if defined(CONFIG_USB_HW_PARAM)
static int set_hw_param(struct usb_device *dev)
{
	struct otg_notify *o_notify = get_otg_notify();
	int ret = 0;
	int bInterfaceClass = 0, speed = 0;

	if (o_notify == NULL || dev->config->interface[0] == NULL) {
		ret =  -EFAULT;
		goto err;
	}

	if (dev->bus->root_hub != dev) {
		bInterfaceClass
			= dev->config->interface[0]
				->cur_altsetting->desc.bInterfaceClass;
		speed = dev->speed;

		pr_info("%s USB device connected - Class : 0x%x, speed : 0x%x\n",
			__func__, bInterfaceClass, speed);

		if (bInterfaceClass == USB_CLASS_AUDIO)
			inc_hw_param(o_notify, USB_HOST_CLASS_AUDIO_COUNT);
		else if (bInterfaceClass == USB_CLASS_COMM)
			inc_hw_param(o_notify, USB_HOST_CLASS_COMM_COUNT);
		else if (bInterfaceClass == USB_CLASS_HID)
			inc_hw_param(o_notify, USB_HOST_CLASS_HID_COUNT);
		else if (bInterfaceClass == USB_CLASS_PHYSICAL)
			inc_hw_param(o_notify, USB_HOST_CLASS_PHYSICAL_COUNT);
		else if (bInterfaceClass == USB_CLASS_STILL_IMAGE)
			inc_hw_param(o_notify, USB_HOST_CLASS_IMAGE_COUNT);
		else if (bInterfaceClass == USB_CLASS_PRINTER)
			inc_hw_param(o_notify, USB_HOST_CLASS_PRINTER_COUNT);
		else if (bInterfaceClass == USB_CLASS_MASS_STORAGE) {
			inc_hw_param(o_notify, USB_HOST_CLASS_STORAGE_COUNT);
			if (speed == USB_SPEED_SUPER)
				inc_hw_param(o_notify,
					USB_HOST_STORAGE_SUPER_COUNT);
			else if (speed == USB_SPEED_HIGH)
				inc_hw_param(o_notify,
					USB_HOST_STORAGE_HIGH_COUNT);
			else if (speed == USB_SPEED_FULL)
				inc_hw_param(o_notify,
					USB_HOST_STORAGE_FULL_COUNT);
		} else if (bInterfaceClass == USB_CLASS_HUB)
			inc_hw_param(o_notify, USB_HOST_CLASS_HUB_COUNT);
		else if (bInterfaceClass == USB_CLASS_CDC_DATA)
			inc_hw_param(o_notify, USB_HOST_CLASS_CDC_COUNT);
		else if (bInterfaceClass == USB_CLASS_CSCID)
			inc_hw_param(o_notify, USB_HOST_CLASS_CSCID_COUNT);
		else if (bInterfaceClass == USB_CLASS_CONTENT_SEC)
			inc_hw_param(o_notify, USB_HOST_CLASS_CONTENT_COUNT);
		else if (bInterfaceClass == USB_CLASS_VIDEO)
			inc_hw_param(o_notify, USB_HOST_CLASS_VIDEO_COUNT);
		else if (bInterfaceClass == USB_CLASS_WIRELESS_CONTROLLER)
			inc_hw_param(o_notify, USB_HOST_CLASS_WIRELESS_COUNT);
		else if (bInterfaceClass == USB_CLASS_MISC)
			inc_hw_param(o_notify, USB_HOST_CLASS_MISC_COUNT);
		else if (bInterfaceClass == USB_CLASS_APP_SPEC)
			inc_hw_param(o_notify, USB_HOST_CLASS_APP_COUNT);
		else if (bInterfaceClass == USB_CLASS_VENDOR_SPEC)
			inc_hw_param(o_notify, USB_HOST_CLASS_VENDOR_COUNT);

		if (speed == USB_SPEED_SUPER)
			inc_hw_param(o_notify, USB_HOST_SUPER_SPEED_COUNT);
		else if (speed == USB_SPEED_HIGH)
			inc_hw_param(o_notify, USB_HOST_HIGH_SPEED_COUNT);
		else if (speed == USB_SPEED_FULL)
			inc_hw_param(o_notify, USB_HOST_FULL_SPEED_COUNT);
		else if (speed == USB_SPEED_LOW)
			inc_hw_param(o_notify, USB_HOST_LOW_SPEED_COUNT);
	}
err:
	return ret;
}
#endif
static int dev_notify(struct notifier_block *self,
			       unsigned long action, void *dev)
{
	switch (action) {
	case USB_DEVICE_ADD:
		call_device_notify(dev);
		call_battery_notify(dev, 1);
		check_device_speed(dev, 1);
		update_hub_autosuspend_timer(dev);
#if defined(CONFIG_USB_HW_PARAM)
		set_hw_param(dev);
#endif
		break;
	case USB_DEVICE_REMOVE:
		call_battery_notify(dev, 0);
		check_device_speed(dev, 0);
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block dev_nb = {
	.notifier_call = dev_notify,
};

void register_usbdev_notify(void)
{
	usb_register_notify(&dev_nb);
}
EXPORT_SYMBOL(register_usbdev_notify);

void unregister_usbdev_notify(void)
{
	usb_unregister_notify(&dev_nb);
}
EXPORT_SYMBOL(unregister_usbdev_notify);

