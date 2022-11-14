// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2014-2020 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

 /* usb notify layer v3.5 */
#define NOTIFY_VERSION "3.5"

#define pr_fmt(fmt) "usb_notify: " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/usb_notify.h>
#include <sound/core.h>
#include <linux/usb/audio.h>
#include "host_notify_class.h"
#include "dock_notify.h"
#include "usb_notify_sysfs.h"

#define DEFAULT_OVC_POLL_SEC 3

struct  ovc {
	struct otg_notify *o_notify;
	wait_queue_head_t	 delay_wait;
	struct completion	scanning_done;
	struct task_struct *th;
	struct mutex ovc_lock;
	int thread_remove;
	int can_ovc;
	int poll_period;
	int prev_state;
	void *data;
	int (*check_state)(void *data);
};

struct vbus_gpio {
	spinlock_t lock;
	int gpio_status;
};

struct otg_state_work {
	struct otg_notify *o_notify;
	struct work_struct otg_work;
	unsigned long event;
	int enable;
};

struct otg_booting_delay {
	struct delayed_work booting_work;
	unsigned long reserve_state;
};

struct typec_info {
	int data_role;
	int power_role;
	int pd;
};

struct usb_gadget_info {
	int bus_state;
	int usb_cable_connect;
};

struct usb_notify {
	struct otg_notify *o_notify;
	struct atomic_notifier_head	otg_notifier;
	struct blocking_notifier_head extra_notifier;
	struct notifier_block otg_nb;
	struct notifier_block extra_nb;
	struct vbus_gpio v_gpio;
	struct host_notify_dev ndev;
	struct usb_notify_dev udev;
	struct workqueue_struct *notifier_wq;
	struct wakeup_source ws;
	struct otg_booster *booster;
	struct ovc ovc_info;
	struct otg_booting_delay b_delay;
	struct delayed_work check_work;
	struct typec_info typec_status;
	struct usb_gadget_info gadget_status;
	struct mutex state_lock;
	int is_device;
	int check_work_complete;
	int oc_noti;
	int disable_v_drive;
	unsigned long c_type;
	int c_status;
	int sec_whitelist_enable;
	int reserve_vbus_booster;
	int disable_state;
#if defined(CONFIG_USB_HW_PARAM)
	unsigned long long hw_param[USB_CCIC_HW_PARAM_MAX];
#endif
};

struct usb_notify_core {
	struct otg_notify *o_notify;
};

static struct usb_notify_core *u_notify_core;

/*
 *  Define event types.
 *  NOTIFY_EVENT_STATE can be called in both interrupt context
 *		and process context. But it executes queue_work.
 *  NOTIFY_EVENT_EXTRA can be called directly without queue_work.
 *           But it must be called in process context.
 *  NOTIFY_EVENT_DELAY events can not run inner booting delay.
 *  NOTIFY_EVENT_NEED_VBUSDRIVE events need to drive 5v out
 *           from phone charger ic
 *  NOTIFY_EVENT_NOBLOCKING events are not blocked by disable sysfs.
 *  NOTIFY_EVENT_NOSAVE events are not saved in cable type.
 */
static int check_event_type(enum otg_notify_events event)
{
	int ret = 0;

	switch (event) {
	case NOTIFY_EVENT_OVERCURRENT:
	case NOTIFY_EVENT_VBUSPOWER:
	case NOTIFY_EVENT_SMSC_OVC:
	case NOTIFY_EVENT_SMTD_EXT_CURRENT:
	case NOTIFY_EVENT_MMD_EXT_CURRENT:
	case NOTIFY_EVENT_HMD_EXT_CURRENT:
	case NOTIFY_EVENT_DEVICE_CONNECT:
	case NOTIFY_EVENT_GAMEPAD_CONNECT:
	case NOTIFY_EVENT_LANHUB_CONNECT:
	case NOTIFY_EVENT_POWER_SOURCE:
	case NOTIFY_EVENT_PD_CONTRACT:
	case NOTIFY_EVENT_VBUS_RESET:
	case NOTIFY_EVENT_RESERVE_BOOSTER:
	case NOTIFY_EVENT_USB_CABLE:
	case NOTIFY_EVENT_USBD_SUSPENDED:
	case NOTIFY_EVENT_USBD_UNCONFIGURED:
	case NOTIFY_EVENT_USBD_CONFIGURED:
		ret |= NOTIFY_EVENT_EXTRA;
		break;
	case NOTIFY_EVENT_VBUS:
	case NOTIFY_EVENT_SMARTDOCK_USB:
		ret |= (NOTIFY_EVENT_STATE | NOTIFY_EVENT_DELAY
				| NOTIFY_EVENT_NEED_CLIENT);
		break;
	case NOTIFY_EVENT_HOST:
	case NOTIFY_EVENT_HMT:
	case NOTIFY_EVENT_GAMEPAD:
		ret |= (NOTIFY_EVENT_STATE | NOTIFY_EVENT_NEED_VBUSDRIVE
				| NOTIFY_EVENT_DELAY | NOTIFY_EVENT_NEED_HOST);
		break;
	case NOTIFY_EVENT_POGO:
		ret |= (NOTIFY_EVENT_STATE | NOTIFY_EVENT_DELAY
				| NOTIFY_EVENT_NEED_HOST);
		break;
	case NOTIFY_EVENT_HOST_RELOAD:
		ret |= (NOTIFY_EVENT_STATE | NOTIFY_EVENT_NEED_HOST
				| NOTIFY_EVENT_NOSAVE);
			break;
	case NOTIFY_EVENT_ALL_DISABLE:
	case NOTIFY_EVENT_HOST_DISABLE:
	case NOTIFY_EVENT_CLIENT_DISABLE:
	case NOTIFY_EVENT_MDM_ON_OFF:
		ret |= (NOTIFY_EVENT_STATE | NOTIFY_EVENT_NOBLOCKING
				| NOTIFY_EVENT_NOSAVE);
		break;
	case NOTIFY_EVENT_DRIVE_VBUS:
	case NOTIFY_EVENT_LANHUB_TA:
		ret |= (NOTIFY_EVENT_STATE | NOTIFY_EVENT_NOSAVE
				| NOTIFY_EVENT_NEED_HOST);
		break;
	case NOTIFY_EVENT_SMARTDOCK_TA:
	case NOTIFY_EVENT_AUDIODOCK:
	case NOTIFY_EVENT_LANHUB:
	case NOTIFY_EVENT_MMDOCK:
		ret |= (NOTIFY_EVENT_DELAY | NOTIFY_EVENT_NEED_HOST);
	case NOTIFY_EVENT_CHARGER:
	case NOTIFY_EVENT_NONE:
	default:
		ret |= NOTIFY_EVENT_STATE;
		break;
	}
	return ret;
}

static int check_same_event_type(enum otg_notify_events event1,
		enum otg_notify_events event2)
{
	return (check_event_type(event1)
			== check_event_type(event2));
}

const char *event_string(enum otg_notify_events event)
{
	int virt;

	virt = IS_VIRTUAL(event);
	event = PHY_EVENT(event);

	switch (event) {
	case NOTIFY_EVENT_NONE:
		return "none";
	case NOTIFY_EVENT_VBUS:
		return virt ? "vbus(virtual)" : "vbus";
	case NOTIFY_EVENT_HOST:
		return virt ? "host_id(virtual)" : "host_id";
	case NOTIFY_EVENT_CHARGER:
		return virt ? "charger(virtual)" : "charger";
	case NOTIFY_EVENT_SMARTDOCK_TA:
		return virt ? "smartdock_ta(virtual)" : "smartdock_ta";
	case NOTIFY_EVENT_SMARTDOCK_USB:
		return virt ? "smartdock_usb(virtual)" : "smartdock_usb";
	case NOTIFY_EVENT_AUDIODOCK:
		return virt ? "audiodock(virtual)" : "audiodock";
	case NOTIFY_EVENT_LANHUB:
		return virt ? "lanhub(virtual)" : "lanhub";
	case NOTIFY_EVENT_LANHUB_TA:
		return virt ? "lanhub_ta(virtual)" : "lanhub_ta";
	case NOTIFY_EVENT_MMDOCK:
		return virt ? "mmdock(virtual)" : "mmdock";
	case NOTIFY_EVENT_HMT:
		return virt ? "hmt(virtual)" : "hmt";
	case NOTIFY_EVENT_GAMEPAD:
		return virt ? "gamepad(virtual)" : "gamepad";
	case NOTIFY_EVENT_POGO:
		return virt ? "pogo(virtual)" : "pogo";
	case NOTIFY_EVENT_HOST_RELOAD:
		return virt ? "host_reload(virtual)" : "host_reload";
	case NOTIFY_EVENT_DRIVE_VBUS:
		return "drive_vbus";
	case NOTIFY_EVENT_ALL_DISABLE:
		return "disable_all_notify";
	case NOTIFY_EVENT_HOST_DISABLE:
		return "disable_host_notify";
	case NOTIFY_EVENT_CLIENT_DISABLE:
		return "disable_client_notify";
	case NOTIFY_EVENT_MDM_ON_OFF:
		return "mdm control_notify";
	case NOTIFY_EVENT_OVERCURRENT:
		return "overcurrent";
	case NOTIFY_EVENT_VBUSPOWER:
		return "vbus_power";
	case NOTIFY_EVENT_SMSC_OVC:
		return "smsc_ovc";
	case NOTIFY_EVENT_SMTD_EXT_CURRENT:
		return "smtd_ext_current";
	case NOTIFY_EVENT_MMD_EXT_CURRENT:
		return "mmd_ext_current";
	case NOTIFY_EVENT_HMD_EXT_CURRENT:
		return "hmd_ext_current";
	case NOTIFY_EVENT_DEVICE_CONNECT:
		return "device_connect";
	case NOTIFY_EVENT_GAMEPAD_CONNECT:
		return "gamepad_connect";
	case NOTIFY_EVENT_LANHUB_CONNECT:
		return "lanhub_connect";
	case NOTIFY_EVENT_POWER_SOURCE:
		return "power_role_source";
	case NOTIFY_EVENT_PD_CONTRACT:
		return "pd_contract";
	case NOTIFY_EVENT_VBUS_RESET:
		return "host_accessory_restart";
	case NOTIFY_EVENT_RESERVE_BOOSTER:
		return "reserve_booster";
	case NOTIFY_EVENT_USB_CABLE:
		return "usb_cable";
	case NOTIFY_EVENT_USBD_SUSPENDED:
		return "usb_d_suspended";
	case NOTIFY_EVENT_USBD_UNCONFIGURED:
		return "usb_d_unconfigured";
	case NOTIFY_EVENT_USBD_CONFIGURED:
		return "usb_d_configured";
	default:
		return "undefined";
	}
}
EXPORT_SYMBOL(event_string);

const char *status_string(enum otg_notify_event_status status)
{
	switch (status) {
	case NOTIFY_EVENT_DISABLED:
		return "disabled";
	case NOTIFY_EVENT_DISABLING:
		return "disabling";
	case NOTIFY_EVENT_ENABLED:
		return "enabled";
	case NOTIFY_EVENT_ENABLING:
		return "enabling";
	case NOTIFY_EVENT_BLOCKED:
		return "blocked";
	case NOTIFY_EVENT_BLOCKING:
		return "blocking";
	default:
		return "undefined";
	}
}
EXPORT_SYMBOL(status_string);

static const char *block_string(enum otg_notify_block_type type)
{
	switch (type) {
	case NOTIFY_BLOCK_TYPE_NONE:
		return "block_off";
	case NOTIFY_BLOCK_TYPE_HOST:
		return "block_host";
	case NOTIFY_BLOCK_TYPE_CLIENT:
		return "block_client";
	case NOTIFY_BLOCK_TYPE_ALL:
		return "block_all";
	default:
		return "undefined";
	}
}

static int create_usb_notify(void)
{
	int ret = 0;

	if (u_notify_core)
		goto err;

	u_notify_core = kzalloc(sizeof(struct usb_notify_core), GFP_KERNEL);
	if (!u_notify_core) {
		ret = -ENOMEM;
		goto err;
	}

	ret = notify_class_init();
	if (ret) {
		pr_err("unable to do host_notify_class_init\n");
		goto err1;
	}

	ret = usb_notify_class_init();
	if (ret) {
		pr_err("unable to do usb_notify_class_init\n");
		goto err2;
	}
	external_notifier_init();

	return 0;
err2:
	notify_class_exit();
err1:
	kfree(u_notify_core);
err:
	return ret;
}

static bool is_host_cable_block(struct otg_notify *n)
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);

	if ((check_event_type(u_notify->c_type)
		& NOTIFY_EVENT_NEED_HOST) &&
			(u_notify->c_status == NOTIFY_EVENT_BLOCKED
				|| u_notify->c_status == NOTIFY_EVENT_BLOCKING))
		return true;
	else
		return false;
}

static bool is_host_cable_enable(struct otg_notify *n)
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);

	if ((check_event_type(u_notify->c_type)
		& NOTIFY_EVENT_NEED_HOST) &&
			(u_notify->c_status == NOTIFY_EVENT_ENABLED
				|| u_notify->c_status == NOTIFY_EVENT_ENABLING))
		return true;
	else
		return false;
}

static bool is_client_cable_block(struct otg_notify *n)
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);

	if ((check_event_type(u_notify->c_type)
		& NOTIFY_EVENT_NEED_CLIENT) &&
			(u_notify->c_status == NOTIFY_EVENT_BLOCKED
				|| u_notify->c_status == NOTIFY_EVENT_BLOCKING))
		return true;
	else
		return false;
}

static bool is_client_cable_enable(struct otg_notify *n)
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);

	if ((check_event_type(u_notify->c_type)
		& NOTIFY_EVENT_NEED_CLIENT) &&
			(u_notify->c_status == NOTIFY_EVENT_ENABLED
				|| u_notify->c_status == NOTIFY_EVENT_ENABLING))
		return true;
	else
		return false;
}

static bool check_block_event(struct otg_notify *n, unsigned long event)
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);

	if ((test_bit(NOTIFY_BLOCK_TYPE_HOST, &u_notify->udev.disable_state)
		&& (check_event_type(event) & NOTIFY_EVENT_NEED_HOST))
		|| (test_bit(NOTIFY_BLOCK_TYPE_CLIENT,
				&u_notify->udev.disable_state)
		&& (check_event_type(event) & NOTIFY_EVENT_NEED_CLIENT)))
		return true;
	else
		return false;
}

static void enable_ovc(struct usb_notify *u_noti, int enable)
{
	u_noti->ovc_info.can_ovc = enable;
}

static int ovc_scan_thread(void *data)
{
	struct ovc *ovcinfo = (struct ovc *)data;
	struct otg_notify *o_notify = ovcinfo->o_notify;
	struct usb_notify *u_notify = (struct usb_notify *)(o_notify->u_notify);
	int state = 0, event = 0;

	while (!kthread_should_stop()) {
		wait_event_interruptible_timeout(ovcinfo->delay_wait,
			ovcinfo->thread_remove, (ovcinfo->poll_period)*HZ);
		if (ovcinfo->thread_remove)
			break;
		mutex_lock(&ovcinfo->ovc_lock);
		if (ovcinfo->check_state
			&& ovcinfo->data
				&& ovcinfo->can_ovc) {

			state = ovcinfo->check_state(ovcinfo->data);

			if (ovcinfo->prev_state != state) {
				if (state == HNOTIFY_LOW) {
					pr_err("%s overcurrent detected\n",
							__func__);
					host_state_notify(&u_notify->ndev,
						NOTIFY_HOST_OVERCURRENT);
					event
					= NOTIFY_EXTRA_USBHOST_OVERCURRENT;
					store_usblog_notify(NOTIFY_EXTRA,
						(void *)&event, NULL);
				} else if (state == HNOTIFY_HIGH) {
					pr_info("%s vbus draw detected\n",
							__func__);
					host_state_notify(&u_notify->ndev,
						NOTIFY_HOST_NONE);
				}
			}
			ovcinfo->prev_state = state;
		}
		mutex_unlock(&ovcinfo->ovc_lock);
		if (!ovcinfo->can_ovc)
			ovcinfo->thread_remove = 1;
	}

	pr_info("%s exit\n", __func__);
	complete_and_exit(&ovcinfo->scanning_done, 0);
	return 0;
}

void ovc_start(struct usb_notify *u_noti)
{
	struct otg_notify *o_notify = u_noti->o_notify;

	if (!u_noti->ovc_info.can_ovc)
		goto skip;

	u_noti->ovc_info.prev_state = HNOTIFY_INITIAL;
	u_noti->ovc_info.poll_period = (o_notify->smsc_ovc_poll_sec) ?
			o_notify->smsc_ovc_poll_sec : DEFAULT_OVC_POLL_SEC;
	reinit_completion(&u_noti->ovc_info.scanning_done);
	u_noti->ovc_info.thread_remove = 0;
	u_noti->ovc_info.th = kthread_run(ovc_scan_thread,
			&u_noti->ovc_info, "ovc-scan-thread");
	if (IS_ERR(u_noti->ovc_info.th)) {
		pr_err("Unable to start the ovc-scanning thread\n");
		complete(&u_noti->ovc_info.scanning_done);
	}
	pr_info("%s on\n", __func__);
	return;
skip:
	complete(&u_noti->ovc_info.scanning_done);
	pr_info("%s skip\n", __func__);
}

void ovc_stop(struct usb_notify *u_noti)
{
	u_noti->ovc_info.thread_remove = 1;
	wake_up_interruptible(&u_noti->ovc_info.delay_wait);
	wait_for_completion(&u_noti->ovc_info.scanning_done);
	mutex_lock(&u_noti->ovc_info.ovc_lock);
	u_noti->ovc_info.check_state = NULL;
	u_noti->ovc_info.data = 0;
	mutex_unlock(&u_noti->ovc_info.ovc_lock);
	pr_info("%s\n", __func__);
}

static void ovc_init(struct usb_notify *u_noti)
{
	init_waitqueue_head(&u_noti->ovc_info.delay_wait);
	init_completion(&u_noti->ovc_info.scanning_done);
	mutex_init(&u_noti->ovc_info.ovc_lock);
	u_noti->ovc_info.prev_state = HNOTIFY_INITIAL;
	u_noti->ovc_info.o_notify = u_noti->o_notify;
	pr_info("%s\n", __func__);
}

static irqreturn_t vbus_irq_isr(int irq, void *data)
{
	struct otg_notify *notify = (struct otg_notify *)(data);
	struct usb_notify *u_notify = (struct usb_notify *)(notify->u_notify);
	unsigned long flags = 0;
	int gpio_value = 0;
	irqreturn_t ret = IRQ_NONE;

	spin_lock_irqsave(&u_notify->v_gpio.lock, flags);
	gpio_value = gpio_get_value(notify->vbus_detect_gpio);
	if (u_notify->v_gpio.gpio_status != gpio_value) {
		u_notify->v_gpio.gpio_status = gpio_value;
		ret = IRQ_WAKE_THREAD;
	} else
		ret = IRQ_HANDLED;
	spin_unlock_irqrestore(&u_notify->v_gpio.lock, flags);

	return ret;
}

static irqreturn_t vbus_irq_thread(int irq, void *data)
{
	struct otg_notify *notify = (struct otg_notify *)(data);
	struct usb_notify *u_notify = (struct usb_notify *)(notify->u_notify);
	unsigned long flags = 0;
	int gpio_value = 0, event = 0;

	spin_lock_irqsave(&u_notify->v_gpio.lock, flags);
	gpio_value = u_notify->v_gpio.gpio_status;
	spin_unlock_irqrestore(&u_notify->v_gpio.lock, flags);

	if (gpio_value) {
		u_notify->ndev.booster = NOTIFY_POWER_ON;
		pr_info("vbus on detect\n");
		if (notify->post_vbus_detect)
			notify->post_vbus_detect(NOTIFY_POWER_ON);
	} else {
		if ((u_notify->ndev.mode == NOTIFY_HOST_MODE)
			&& (u_notify->ndev.booster == NOTIFY_POWER_ON
				&& u_notify->oc_noti)) {
			host_state_notify(&u_notify->ndev,
					NOTIFY_HOST_OVERCURRENT);
			event = NOTIFY_EXTRA_USBHOST_OVERCURRENT;
			store_usblog_notify(NOTIFY_EXTRA,
				(void *)&event, NULL);
			pr_err("OTG overcurrent!!!!!!\n");
		} else {
			pr_info("vbus off detect\n");
			if (notify->post_vbus_detect)
				notify->post_vbus_detect(NOTIFY_POWER_OFF);
		}
		u_notify->ndev.booster = NOTIFY_POWER_OFF;
	}
	return IRQ_HANDLED;
}

int register_gpios(struct otg_notify *n)
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);
	int ret = 0;
	int vbus_irq = 0;
	int vbus_gpio = -1;
	int redriver_gpio = -1;

	if (!gpio_is_valid(n->vbus_detect_gpio))
		goto redriver_en_gpio_phase;

	vbus_gpio = n->vbus_detect_gpio;

	spin_lock_init(&u_notify->v_gpio.lock);

	if (n->pre_gpio)
		n->pre_gpio(vbus_gpio, NOTIFY_VBUS);

	ret = gpio_request(vbus_gpio, "vbus_detect_notify");
	if (ret) {
		pr_err("failed to request %d\n", vbus_gpio);
		goto err;
	}
	gpio_direction_input(vbus_gpio);

	u_notify->v_gpio.gpio_status
		= gpio_get_value(vbus_gpio);
	vbus_irq = gpio_to_irq(vbus_gpio);
	ret = request_threaded_irq(vbus_irq,
			vbus_irq_isr,
			vbus_irq_thread,
			(IRQF_TRIGGER_FALLING |
				IRQF_TRIGGER_RISING |
					IRQF_ONESHOT),
			"vbus_irq_notify",
			n);
	if (ret) {
		pr_err("Failed to register IRQ\n");
		goto err;
	}
	if (n->post_gpio)
		n->post_gpio(vbus_gpio, NOTIFY_VBUS);

	pr_info("vbus detect gpio %d is registered.\n", vbus_gpio);
redriver_en_gpio_phase:
	if (!gpio_is_valid(n->redriver_en_gpio))
		goto err;

	redriver_gpio = n->redriver_en_gpio;

	if (n->pre_gpio)
		n->pre_gpio(redriver_gpio, NOTIFY_REDRIVER);

	ret = gpio_request(redriver_gpio, "usb30_redriver_en");
	if (ret) {
		pr_err("failed to request %d\n", redriver_gpio);
		goto err;
	}
	gpio_direction_output(redriver_gpio, 0);
	if (n->post_gpio)
		n->post_gpio(redriver_gpio, NOTIFY_REDRIVER);

	pr_info("redriver en gpio %d is registered.\n", redriver_gpio);
err:
	return ret;
}

int do_notify_blockstate(struct otg_notify *n, unsigned long event,
					int type, int enable)
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);
	int ret = 0;

	switch (event) {
	case NOTIFY_EVENT_NONE:
	case NOTIFY_EVENT_CHARGER:
		break;
	case NOTIFY_EVENT_SMARTDOCK_USB:
	case NOTIFY_EVENT_VBUS:
		if (enable)
			if (n->set_chg_current)
				n->set_chg_current(NOTIFY_USB_CONFIGURED);
		break;
	case NOTIFY_EVENT_LANHUB:
	case NOTIFY_EVENT_HMT:
	case NOTIFY_EVENT_HOST:
	case NOTIFY_EVENT_MMDOCK:
	case NOTIFY_EVENT_SMARTDOCK_TA:
	case NOTIFY_EVENT_AUDIODOCK:
	case NOTIFY_EVENT_GAMEPAD:
	case NOTIFY_EVENT_POGO:
		if (n->unsupport_host) {
			pr_err("This model doesn't support usb host\n");
			goto skip;
		}
		if (enable)
			host_state_notify(&u_notify->ndev, NOTIFY_HOST_BLOCK);
		else
			host_state_notify(&u_notify->ndev, NOTIFY_HOST_NONE);
		break;
	case NOTIFY_EVENT_DRIVE_VBUS:
		ret = -ESRCH;
		break;
	default:
		break;
	}

skip:
	return ret;
}

static void update_cable_status(struct otg_notify *n, unsigned long event,
		int virtual, int enable, int start)
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);

	if (enable) {
		u_notify->c_type = event;
		if (check_block_event(n, event) ||
			(check_event_type(u_notify->c_type)
				& NOTIFY_EVENT_NEED_HOST &&
					n->unsupport_host))
			u_notify->c_status = (start) ?
				NOTIFY_EVENT_BLOCKING : NOTIFY_EVENT_BLOCKED;
		else
			u_notify->c_status = (start) ?
				NOTIFY_EVENT_ENABLING : NOTIFY_EVENT_ENABLED;
	} else {
		if (virtual)
			u_notify->c_status = (start) ?
				NOTIFY_EVENT_BLOCKING : NOTIFY_EVENT_BLOCKED;
		else {
			u_notify->c_type = NOTIFY_EVENT_NONE;
			u_notify->c_status = (start) ?
				NOTIFY_EVENT_DISABLING : NOTIFY_EVENT_DISABLED;
		}
	}
}

static void reserve_state_check(struct work_struct *work)
{
	struct otg_booting_delay *o_b_d = container_of(work,
			struct otg_booting_delay, booting_work.work);
	struct usb_notify *u_noti = container_of(o_b_d,
			struct usb_notify, b_delay);
	int enable = 1;
	unsigned long state = 0;

	state = u_noti->b_delay.reserve_state;
	u_noti->o_notify->booting_delay_sec = 0;
	pr_info("%s booting delay finished\n", __func__);

	if (u_noti->b_delay.reserve_state != NOTIFY_EVENT_NONE) {
		pr_info("%s event=%s(%lu) enable=%d\n", __func__,
				event_string(state), state, enable);
		if (check_event_type(state) & NOTIFY_EVENT_EXTRA)
			blocking_notifier_call_chain
				(&u_noti->extra_notifier,
						state, &enable);
		else
			atomic_notifier_call_chain
				(&u_noti->otg_notifier,
						state, &enable);
	}
	send_external_notify(EXTERNAL_NOTIFY_POSSIBLE_USB, 1);
}

static void device_connect_check(struct work_struct *work)
{
	struct usb_notify *u_notify = container_of(work,
			struct usb_notify, check_work.work);

	pr_info("%s start. is_device=%d\n", __func__, u_notify->is_device);
	if (!u_notify->is_device) {
		send_external_notify(EXTERNAL_NOTIFY_3S_NODEVICE, 1);

		if (u_notify->o_notify->vbus_drive)
			u_notify->o_notify->vbus_drive(0);
		u_notify->typec_status.power_role = HNOTIFY_SINK;
	}
	u_notify->check_work_complete = 1;
	pr_info("%s finished\n", __func__);
}

int set_notify_disable(struct usb_notify_dev *udev, int disable)
{
	struct otg_notify *n = udev->o_notify;
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);
	unsigned long usb_notify;
	int usb_notify_state;

	if (!n->disable_control) {
		pr_err("%s disable_control is not supported\n", __func__);
		goto skip;
	}

	pr_info("%s prev=%s(%d) => disable=%s(%d)\n", __func__,
			block_string(u_notify->disable_state), u_notify->disable_state,
			block_string(disable), disable);

	if (u_notify->disable_state == disable) {
		pr_err("%s duplicated state\n", __func__);
		goto skip;
	}

	u_notify->disable_state = disable;

	switch (disable) {
	case NOTIFY_BLOCK_TYPE_ALL:
		send_external_notify(EXTERNAL_NOTIFY_HOSTBLOCK_EARLY, 1);
		if (is_host_cable_enable(n) ||
			is_client_cable_enable(n)) {
			pr_info("%s event=%s(%lu) disable\n", __func__,
				event_string(VIRT_EVENT(u_notify->c_type)),
					VIRT_EVENT(u_notify->c_type));
			if (is_host_cable_enable(n)) {
				if (!n->auto_drive_vbus &&
					(u_notify->typec_status.power_role
						 == HNOTIFY_SOURCE) &&
					check_event_type(u_notify->c_type)
						& NOTIFY_EVENT_NEED_VBUSDRIVE)
					send_otg_notify(n,
						NOTIFY_EVENT_DRIVE_VBUS, 0);
			} else {
				if (u_notify->typec_status.power_role
						== HNOTIFY_SOURCE)
					send_otg_notify(n,
						NOTIFY_EVENT_DRIVE_VBUS, 0);
			}
			send_otg_notify(n, VIRT_EVENT(u_notify->c_type), 0);
		} else {
			if (u_notify->typec_status.power_role
					== HNOTIFY_SOURCE)
				send_otg_notify(n,
						NOTIFY_EVENT_DRIVE_VBUS, 0);
		}
		send_otg_notify(n, NOTIFY_EVENT_ALL_DISABLE, 1);

		usb_notify = NOTIFY_EVENT_ALL_DISABLE;
		usb_notify_state = NOTIFY_EVENT_BLOCKED;
		store_usblog_notify(NOTIFY_EVENT,
			(void *)&usb_notify, (void *)&usb_notify_state);
		break;
	case NOTIFY_BLOCK_TYPE_HOST:
		send_external_notify(EXTERNAL_NOTIFY_HOSTBLOCK_EARLY, 1);
		if (is_host_cable_enable(n)) {

			pr_info("%s event=%s(%lu) disable\n", __func__,
				event_string(VIRT_EVENT(u_notify->c_type)),
					VIRT_EVENT(u_notify->c_type));

			if (!n->auto_drive_vbus &&
				(u_notify->typec_status.power_role
					== HNOTIFY_SOURCE) &&
				check_event_type(u_notify->c_type)
						& NOTIFY_EVENT_NEED_VBUSDRIVE)
				send_otg_notify(n, NOTIFY_EVENT_DRIVE_VBUS, 0);

			send_otg_notify(n, VIRT_EVENT(u_notify->c_type), 0);
		} else {
			if (u_notify->typec_status.power_role
					== HNOTIFY_SOURCE)
				send_otg_notify(n, NOTIFY_EVENT_DRIVE_VBUS, 0);
		}
		send_otg_notify(n, NOTIFY_EVENT_HOST_DISABLE, 1);

		usb_notify = NOTIFY_EVENT_HOST_DISABLE;
		usb_notify_state = NOTIFY_EVENT_BLOCKED;
		store_usblog_notify(NOTIFY_EVENT,
			(void *)&usb_notify, (void *)&usb_notify_state);

		if (!is_client_cable_block(n))
			goto skip;

		pr_info("%s event=%s(%lu) enable\n", __func__,
			event_string(VIRT_EVENT(u_notify->c_type)),
				VIRT_EVENT(u_notify->c_type));

		send_otg_notify(n, VIRT_EVENT(u_notify->c_type), 1);
		break;
	case NOTIFY_BLOCK_TYPE_CLIENT:
		if (is_client_cable_enable(n)) {

			pr_info("%s event=%s(%lu) disable\n", __func__,
				event_string(VIRT_EVENT(u_notify->c_type)),
					VIRT_EVENT(u_notify->c_type));

			send_otg_notify(n, VIRT_EVENT(u_notify->c_type), 0);
		}

		send_otg_notify(n, NOTIFY_EVENT_CLIENT_DISABLE, 1);

		usb_notify = NOTIFY_EVENT_CLIENT_DISABLE;
		usb_notify_state = NOTIFY_EVENT_BLOCKED;
		store_usblog_notify(NOTIFY_EVENT,
			(void *)&usb_notify, (void *)&usb_notify_state);

		if (!is_host_cable_block(n))
			goto skip;

		if (n->unsupport_host)
			goto skip;

		pr_info("%s event=%s(%lu) enable\n", __func__,
			event_string(VIRT_EVENT(u_notify->c_type)),
				VIRT_EVENT(u_notify->c_type));
		if (!n->auto_drive_vbus &&
			(u_notify->typec_status.power_role
				 == HNOTIFY_SOURCE) &&
			check_event_type(u_notify->c_type)
				& NOTIFY_EVENT_NEED_VBUSDRIVE)
			send_otg_notify(n, NOTIFY_EVENT_DRIVE_VBUS, 1);

		send_otg_notify(n, VIRT_EVENT(u_notify->c_type), 1);
		break;
	case NOTIFY_BLOCK_TYPE_NONE:
		send_external_notify(EXTERNAL_NOTIFY_HOSTBLOCK_EARLY, 0);
		send_otg_notify(n, NOTIFY_EVENT_ALL_DISABLE, 0);

		usb_notify = NOTIFY_EVENT_ALL_DISABLE;
		usb_notify_state = NOTIFY_EVENT_DISABLED;
		store_usblog_notify(NOTIFY_EVENT,
			(void *)&usb_notify, (void *)&usb_notify_state);

		if (!is_host_cable_block(n) && !is_client_cable_block(n)) {
			if (u_notify->typec_status.power_role
					== HNOTIFY_SOURCE)
				send_otg_notify(n, NOTIFY_EVENT_DRIVE_VBUS, 1);
			goto skip;
		}

		if (check_event_type(u_notify->c_type)
				& NOTIFY_EVENT_NEED_HOST && n->unsupport_host)
			goto skip;
		pr_info("%s event=%s(%lu) enable\n", __func__,
			event_string(VIRT_EVENT(u_notify->c_type)),
				VIRT_EVENT(u_notify->c_type));
		if (is_host_cable_block(n)) {
			if (!n->auto_drive_vbus &&
				(u_notify->typec_status.power_role
						 == HNOTIFY_SOURCE) &&
				check_event_type(u_notify->c_type)
					& NOTIFY_EVENT_NEED_VBUSDRIVE)
				send_otg_notify(n, NOTIFY_EVENT_DRIVE_VBUS, 1);
		} else {
			if (u_notify->typec_status.power_role
					== HNOTIFY_SOURCE)
				send_otg_notify(n, NOTIFY_EVENT_DRIVE_VBUS, 1);
		}
		send_otg_notify(n, VIRT_EVENT(u_notify->c_type), 1);
	break;
	}
skip:
	return 0;
}

void send_usb_mdm_uevent(void)
{
	struct otg_notify *o_notify = get_otg_notify();
	char *envp[4];
	char *type = {"TYPE=usbmdm"};
	char *state = {"STATE=ADD"};
	char *words = {"WORDS=no_whitelist"};
	int index = 0;

	if (!o_notify) {
		pr_err("%s o_notify is null\n", __func__);
		goto err;
	}

	envp[index++] = type;
	envp[index++] = state;

	envp[index++] = words;

	envp[index++] = NULL;

	if (send_usb_notify_uevent(o_notify, envp)) {
		pr_err("%s error\n", __func__);
		goto err;
	}
	pr_info("%s\n", __func__);
err:
	return;
}
EXPORT_SYMBOL(send_usb_mdm_uevent);

void send_usb_certi_uevent(int usb_certi)
{
	struct otg_notify *o_notify = get_otg_notify();
	char *envp[4];
	char *type = {"TYPE=usbcerti"};
	char *state = {"STATE=ADD"};
	char *words;
	int index = 0;

	if (!o_notify) {
		pr_err("%s o_notify is null\n", __func__);
		goto err;
	}

	envp[index++] = type;
	envp[index++] = state;

	switch (usb_certi) {
	case USB_CERTI_UNSUPPORT_ACCESSORY:
		words = "WORDS=unsupport_accessory";
		break;
	case USB_CERTI_NO_RESPONSE:
		words = "WORDS=no_response";
		break;
	case USB_CERTI_HUB_DEPTH_EXCEED:
		words = "WORDS=hub_depth_exceed";
		break;
	case USB_CERTI_HUB_POWER_EXCEED:
		words = "WORDS=hub_power_exceed";
		break;
	case USB_CERTI_HOST_RESOURCE_EXCEED:
		words = "WORDS=host_resource_exceed";
		break;
	default:
		pr_err("%s invalid input\n", __func__);
		goto err;
	}

	envp[index++] = words;

	envp[index++] = NULL;

	if (send_usb_notify_uevent(o_notify, envp)) {
		pr_err("%s error\n", __func__);
		goto err;
	}
	pr_info("%s: %s\n", __func__, words);
err:
	return;
}
EXPORT_SYMBOL(send_usb_certi_uevent);

void send_usb_err_uevent(int err_type, int mode)
{
	struct otg_notify *o_notify = get_otg_notify();
	char *envp[4];
	char *type = {"TYPE=usberr"};
	char *state;
	char *words;
	int index = 0;

	if (!o_notify) {
		pr_err("%s o_notify is null\n", __func__);
		goto err;
	}

	if (mode)
		state = "STATE=ADD";
	else
		state = "STATE=REMOVE";

	envp[index++] = type;
	envp[index++] = state;

	switch (err_type) {
	case USB_ERR_ABNORMAL_RESET:
		words = "WORDS=abnormal_reset";
		if (mode)
			inc_hw_param(o_notify,
				USB_CLIENT_ANDROID_AUTO_RESET_POPUP_COUNT);
		break;
	default:
		pr_err("%s invalid input\n", __func__);
		goto err;
	}

	envp[index++] = words;
	envp[index++] = NULL;

	if (send_usb_notify_uevent(o_notify, envp)) {
		pr_err("%s error\n", __func__);
		goto err;
	}
	pr_info("%s: %s\n", __func__, words);
err:
	return;
}
EXPORT_SYMBOL(send_usb_err_uevent);

void send_usb_itracker_uevent(int err_type)
{
	struct otg_notify *o_notify = get_otg_notify();
	char *envp[4];
	char *type = {"TYPE=usbtracker"};
	char *state = {"STATE=ADD"};
	char *words;
	int index = 0;

	if (!o_notify) {
		pr_err("%s o_notify is null\n", __func__);
		goto err;
	}

	envp[index++] = type;
	envp[index++] = state;

	switch (err_type) {
	case NOTIFY_USB_CC_REPEAT:
		words = "WORDS=repeat_ccirq";
		break;
	default:
		pr_err("%s invalid input\n", __func__);
		goto err;
	}

	envp[index++] = words;
	envp[index++] = NULL;

	if (send_usb_notify_uevent(o_notify, envp)) {
		pr_err("%s error\n", __func__);
		goto err;
	}
	pr_info("%s: %s\n", __func__, words);
err:
	return;
}
EXPORT_SYMBOL(send_usb_itracker_uevent);

int get_class_index(int ch9_class_num)
{
	int internal_class_index;

	switch (ch9_class_num) {
	case USB_CLASS_PER_INTERFACE:
		internal_class_index = U_CLASS_PER_INTERFACE;
		break;
	case USB_CLASS_AUDIO:
		internal_class_index = U_CLASS_AUDIO;
		break;
	case USB_CLASS_COMM:
		internal_class_index = U_CLASS_COMM;
		break;
	case USB_CLASS_HID:
		internal_class_index = U_CLASS_HID;
		break;
	case USB_CLASS_PHYSICAL:
		internal_class_index = U_CLASS_PHYSICAL;
		break;
	case USB_CLASS_STILL_IMAGE:
		internal_class_index = U_CLASS_STILL_IMAGE;
		break;
	case USB_CLASS_PRINTER:
		internal_class_index = U_CLASS_PRINTER;
		break;
	case USB_CLASS_MASS_STORAGE:
		internal_class_index = U_CLASS_MASS_STORAGE;
		break;
	case USB_CLASS_HUB:
		internal_class_index = U_CLASS_HUB;
		break;
	case USB_CLASS_CDC_DATA:
		internal_class_index = U_CLASS_CDC_DATA;
		break;
	case USB_CLASS_CSCID:
		internal_class_index = U_CLASS_CSCID;
		break;
	case USB_CLASS_CONTENT_SEC:
		internal_class_index = U_CLASS_CONTENT_SEC;
		break;
	case USB_CLASS_VIDEO:
		internal_class_index = U_CLASS_VIDEO;
		break;
	case USB_CLASS_WIRELESS_CONTROLLER:
		internal_class_index = U_CLASS_WIRELESS_CONTROLLER;
		break;
	case USB_CLASS_MISC:
		internal_class_index = U_CLASS_MISC;
		break;
	case USB_CLASS_APP_SPEC:
		internal_class_index = U_CLASS_APP_SPEC;
		break;
	case USB_CLASS_VENDOR_SPEC:
		internal_class_index = U_CLASS_VENDOR_SPEC;
		break;
	default:
		internal_class_index = 0;
		break;
	}
	return internal_class_index;
}

static bool usb_match_any_interface_for_mdm(struct usb_device *udev,
				    int *whitelist_array)
{
	unsigned int i;

	for (i = 0; i < udev->descriptor.bNumConfigurations; ++i) {
		struct usb_host_config *cfg = &udev->config[i];
		unsigned int j;

		for (j = 0; j < cfg->desc.bNumInterfaces; ++j) {
			struct usb_interface_cache *cache;
			struct usb_host_interface *intf;
			int intf_class;

			cache = cfg->intf_cache[j];
			if (cache->num_altsetting == 0)
				continue;

			intf = &cache->altsetting[0];
			intf_class = intf->desc.bInterfaceClass;
			if (!whitelist_array[get_class_index(intf_class)]) {
				pr_info("%s : FAIL,%x interface, it's not in whitelist\n",
					__func__, intf_class);
				return false;
			}
			pr_info("%s : SUCCESS,%x interface, it's in whitelist\n",
				__func__, intf_class);
		}
	}
	return true;
}

int usb_check_whitelist_for_mdm(struct usb_device *dev)
{
	int *whitelist_array;
	struct otg_notify *o_notify;
	struct usb_notify *u_notify;
	/* return 1 if the enumeration will be going . */
	/* return 0 if the enumeration will be skept . */
	o_notify = get_otg_notify();
	if (o_notify == NULL) {
		pr_err("o_notify is NULL\n");
		return 1;
	}

	u_notify = (struct usb_notify *)(o_notify->u_notify);
	if (u_notify == NULL) {
		pr_err("u_notify is NULL\n");
		return 1;
	}

	if (u_notify->sec_whitelist_enable) {
		whitelist_array = u_notify->udev.whitelist_array_for_mdm;
		if (usb_match_any_interface_for_mdm(dev, whitelist_array)) {
			dev_info(&dev->dev, "the device is matched with whitelist!\n");
			return 1;
		}
		return 0;
	}
	return 1;
}
EXPORT_SYMBOL(usb_check_whitelist_for_mdm);

int usb_otg_restart_accessory(struct usb_device *dev)
{
	struct otg_notify *o_notify;
	int res = 0;

	pr_info("%s\n", __func__);
	o_notify = get_otg_notify();
	if (o_notify == NULL) {
		pr_err("o_notify is NULL\n");
		return -1;
	}

	send_otg_notify(o_notify, NOTIFY_EVENT_VBUS_RESET, 0);
	return res;
}
EXPORT_SYMBOL(usb_otg_restart_accessory);

void set_notify_mdm(struct usb_notify_dev *udev, int disable)
{
	struct otg_notify *n = udev->o_notify;

	switch (disable) {
	case NOTIFY_MDM_TYPE_ON:
		send_otg_notify(n, NOTIFY_EVENT_MDM_ON_OFF, 1);
		if (is_host_cable_enable(n)) {
			pr_info("%s event=%s(%d)\n", __func__,
				event_string(
					VIRT_EVENT(NOTIFY_EVENT_HOST_RELOAD)),
					VIRT_EVENT(NOTIFY_EVENT_HOST_RELOAD));
			send_otg_notify(n,
				VIRT_EVENT(NOTIFY_EVENT_HOST_RELOAD), 1);
		}
		break;
	case NOTIFY_MDM_TYPE_OFF:
		send_otg_notify(n, NOTIFY_EVENT_MDM_ON_OFF, 0);
		break;
	}
}

static void otg_notify_state(struct otg_notify *n,
			unsigned long event, int enable)
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);
	int type = 0;
	int virtual = 0;
	int status = 0;
	unsigned long prev_c_type = 0;

	pr_info("%s+ event=%s(%lu), enable=%s\n", __func__,
		event_string(event), event, enable == 0 ? "off" : "on");

	prev_c_type = u_notify->c_type;
	virtual = IS_VIRTUAL(event);
	event = PHY_EVENT(event);

	type = check_event_type(event);

	if (!(type & NOTIFY_EVENT_NOSAVE)) {
		update_cable_status(n, event, virtual, enable, 1);

		store_usblog_notify(NOTIFY_EVENT,
			(void *)&event, (void *)&u_notify->c_status);

	} else {
		if (enable)
			status = NOTIFY_EVENT_ENABLING;
		else
			status = NOTIFY_EVENT_DISABLING;
		store_usblog_notify(NOTIFY_EVENT,
			(void *)&event, (void *)&status);
	}

	if (check_block_event(n, event) &&
			!(type & NOTIFY_EVENT_NOBLOCKING)) {
		pr_err("%s usb notify is blocked. cause %s\n", __func__,
					u_notify->udev.disable_state_cmd);
		if (do_notify_blockstate(n, event, type, enable))
			goto no_save_event;
		else
			goto err;
	}

	switch (event) {
	case NOTIFY_EVENT_NONE:
		break;
	case NOTIFY_EVENT_SMARTDOCK_USB:
	case NOTIFY_EVENT_VBUS:
		if (enable) {
			u_notify->ndev.mode = NOTIFY_PERIPHERAL_MODE;
			if (n->is_wakelock)
				__pm_stay_awake(&u_notify->ws);
			if (gpio_is_valid(n->redriver_en_gpio))
				gpio_direction_output
					(n->redriver_en_gpio, 1);
			if (n->pre_peri_delay_us)
				usleep_range(n->pre_peri_delay_us * 1000,
					n->pre_peri_delay_us * 1000);
			if (n->set_peripheral)
				n->set_peripheral(true);
		} else {
			mutex_lock(&u_notify->state_lock);
			u_notify->ndev.mode = NOTIFY_NONE_MODE;
			u_notify->gadget_status.bus_state
					= NOTIFY_USB_UNCONFIGURED;
			mutex_unlock(&u_notify->state_lock);
			if (n->set_peripheral)
				n->set_peripheral(false);
			if (gpio_is_valid(n->redriver_en_gpio))
				gpio_direction_output
					(n->redriver_en_gpio, 0);
			if (n->is_wakelock)
				__pm_relax(&u_notify->ws);
		}
		break;
	case NOTIFY_EVENT_LANHUB_TA:
		u_notify->disable_v_drive = enable;
		if (enable)
			u_notify->oc_noti = 0;
		if (n->set_lanhubta)
			n->set_lanhubta(enable);
		break;
	case NOTIFY_EVENT_LANHUB:
		if (n->unsupport_host) {
			pr_err("This model doesn't support usb host\n");
			goto err;
		}
		u_notify->disable_v_drive = enable;
		if (enable) {
			u_notify->oc_noti = 0;
			u_notify->ndev.mode = NOTIFY_HOST_MODE;
			if (n->is_host_wakelock)
				__pm_stay_awake(&u_notify->ws);
			host_state_notify(&u_notify->ndev, NOTIFY_HOST_ADD);
			if (gpio_is_valid(n->redriver_en_gpio))
				gpio_direction_output
					(n->redriver_en_gpio, 1);
			if (n->set_host)
				n->set_host(true);
		} else {
			u_notify->ndev.mode = NOTIFY_NONE_MODE;
			if (n->set_host)
				n->set_host(false);
			if (gpio_is_valid(n->redriver_en_gpio))
				gpio_direction_output
					(n->redriver_en_gpio, 0);
			host_state_notify(&u_notify->ndev, NOTIFY_HOST_REMOVE);
			if (n->is_host_wakelock)
				__pm_relax(&u_notify->ws);
		}
		break;
	case NOTIFY_EVENT_HMT:
	case NOTIFY_EVENT_HOST:
	case NOTIFY_EVENT_GAMEPAD:
		if (n->unsupport_host) {
			pr_err("This model doesn't support usb host\n");
			goto err;
		}
		u_notify->disable_v_drive = 0;
		if (enable) {
			if (check_same_event_type(prev_c_type, event)
				&& !virtual) {
				pr_err("now host mode, skip this command\n");
				goto err;
			}
			u_notify->ndev.mode = NOTIFY_HOST_MODE;
			if (n->is_host_wakelock)
				__pm_stay_awake(&u_notify->ws);
			host_state_notify(&u_notify->ndev, NOTIFY_HOST_ADD);
			if (gpio_is_valid(n->redriver_en_gpio))
				gpio_direction_output
					(n->redriver_en_gpio, 1);
			if (n->auto_drive_vbus == NOTIFY_OP_PRE) {
				u_notify->oc_noti = 1;
				if (n->vbus_drive)
					n->vbus_drive(1);
				u_notify->typec_status.power_role
							= HNOTIFY_SOURCE;
			}
			if (n->set_host)
				n->set_host(true);
			if (n->auto_drive_vbus == NOTIFY_OP_POST) {
				u_notify->oc_noti = 1;
				if (n->vbus_drive)
					n->vbus_drive(1);
				u_notify->typec_status.power_role
							= HNOTIFY_SOURCE;
			}
			if (n->auto_drive_vbus == NOTIFY_OP_OFF) {
				mutex_lock(&u_notify->state_lock);
				if ((u_notify->typec_status.power_role
						== HNOTIFY_SOURCE)
					&& u_notify->reserve_vbus_booster
					&& !is_blocked(n,
						NOTIFY_BLOCK_TYPE_HOST)) {
					pr_info("reserved vbus turn on\n");
					if (n->vbus_drive)
						n->vbus_drive(1);
					u_notify->reserve_vbus_booster = 0;
				}
				mutex_unlock(&u_notify->state_lock);
			}
		} else { /* disable */
			u_notify->ndev.mode = NOTIFY_NONE_MODE;
			if (n->auto_drive_vbus == NOTIFY_OP_POST) {
				u_notify->oc_noti = 0;
				if (n->vbus_drive)
					n->vbus_drive(0);
				u_notify->typec_status.power_role
							= HNOTIFY_SINK;
			}
			if (n->set_host)
				n->set_host(false);
			if (n->auto_drive_vbus == NOTIFY_OP_PRE) {
				u_notify->oc_noti = 0;
				if (n->vbus_drive)
					n->vbus_drive(0);
				u_notify->typec_status.power_role
							= HNOTIFY_SINK;
			}
			if (gpio_is_valid(n->redriver_en_gpio))
				gpio_direction_output
					(n->redriver_en_gpio, 0);
			host_state_notify(&u_notify->ndev, NOTIFY_HOST_REMOVE);
			if (n->is_host_wakelock)
				__pm_relax(&u_notify->ws);
		}
		break;
	case NOTIFY_EVENT_CHARGER:
		if (n->set_charger)
			n->set_charger(enable);
		break;
	case NOTIFY_EVENT_MMDOCK:
		enable_ovc(u_notify, enable);
		/* To detect overcurrent, ndev state is initialized */
		if (enable)
			host_state_notify(&u_notify->ndev,
							NOTIFY_HOST_NONE);
	case NOTIFY_EVENT_POGO:
	case NOTIFY_EVENT_SMARTDOCK_TA:
	case NOTIFY_EVENT_AUDIODOCK:
		if (n->unsupport_host) {
			pr_err("This model doesn't support usb host\n");
			goto err;
		}
		u_notify->disable_v_drive = enable;
		if (enable) {
			u_notify->ndev.mode = NOTIFY_HOST_MODE;
			if (n->is_host_wakelock)
				__pm_stay_awake(&u_notify->ws);
			if (n->set_host)
				n->set_host(true);
		} else {
			u_notify->ndev.mode = NOTIFY_NONE_MODE;
			if (n->set_host)
				n->set_host(false);
			if (n->is_host_wakelock)
				__pm_relax(&u_notify->ws);
		}
		break;
	case NOTIFY_EVENT_HOST_RELOAD:
		if (u_notify->ndev.mode != NOTIFY_HOST_MODE) {
			pr_err("mode is not host. skip host reload.\n");
			goto no_save_event;
		}
		if (n->unsupport_host) {
			pr_err("This model doesn't support usb host\n");
			goto no_save_event;
		}
		if (n->set_host) {
			n->set_host(false);
			msleep(100);
			n->set_host(true);
		}
		goto no_save_event;
	case NOTIFY_EVENT_DRIVE_VBUS:
		if (n->unsupport_host) {
			pr_err("This model doesn't support usb host\n");
			goto no_save_event;
		}
		if (u_notify->disable_v_drive) {
			pr_info("cable type=%s disable vbus draw\n",
					event_string(u_notify->c_type));
			goto no_save_event;
		}
		u_notify->oc_noti = enable;
		if (n->vbus_drive)
			n->vbus_drive((bool)enable);
		goto no_save_event;
	case NOTIFY_EVENT_ALL_DISABLE:
		if (!n->disable_control) {
			pr_err("This model doesn't support disable_control\n");
			goto no_save_event;
		}
		if (enable) {
			send_external_notify(EXTERNAL_NOTIFY_HOSTBLOCK_PRE, 1);

			set_bit(NOTIFY_BLOCK_TYPE_HOST,
				&u_notify->udev.disable_state);
			set_bit(NOTIFY_BLOCK_TYPE_CLIENT,
				&u_notify->udev.disable_state);

			send_external_notify(EXTERNAL_NOTIFY_HOSTBLOCK_POST, 1);
		} else {
			send_external_notify(EXTERNAL_NOTIFY_HOSTBLOCK_PRE, 0);

			clear_bit(NOTIFY_BLOCK_TYPE_HOST,
				&u_notify->udev.disable_state);
			clear_bit(NOTIFY_BLOCK_TYPE_CLIENT,
				&u_notify->udev.disable_state);

			send_external_notify(EXTERNAL_NOTIFY_HOSTBLOCK_POST, 0);
		}
		goto no_save_event;
	case NOTIFY_EVENT_HOST_DISABLE:
		if (!n->disable_control) {
			pr_err("This model doesn't support disable_control\n");
			goto no_save_event;
		}
		if (enable) {
			send_external_notify(EXTERNAL_NOTIFY_HOSTBLOCK_PRE, 1);

			clear_bit(NOTIFY_BLOCK_TYPE_CLIENT,
				&u_notify->udev.disable_state);
			set_bit(NOTIFY_BLOCK_TYPE_HOST,
				&u_notify->udev.disable_state);

			send_external_notify(EXTERNAL_NOTIFY_HOSTBLOCK_POST, 1);
		}
		goto no_save_event;
	case NOTIFY_EVENT_CLIENT_DISABLE:
		if (!n->disable_control) {
			pr_err("This model doesn't support disable_control\n");
			goto no_save_event;
		}
		if (enable) {
			clear_bit(NOTIFY_BLOCK_TYPE_HOST,
				&u_notify->udev.disable_state);
			set_bit(NOTIFY_BLOCK_TYPE_CLIENT,
				&u_notify->udev.disable_state);
		}
		goto no_save_event;
	case NOTIFY_EVENT_MDM_ON_OFF:
		pr_info("%s : mdm block enable for usb whiltelist = %d\n",
			__func__, enable);
		if (enable) {
			send_external_notify(EXTERNAL_NOTIFY_MDMBLOCK_PRE, 1);
			/*whilte list start*/
			u_notify->sec_whitelist_enable = 1;
			send_external_notify(EXTERNAL_NOTIFY_MDMBLOCK_POST, 1);
		} else {
			/*whilte list end*/
			u_notify->sec_whitelist_enable = 0;
		}
		goto no_save_event;
	default:
		break;
	}

	if (((type & NOTIFY_EVENT_NEED_VBUSDRIVE)
				&& event != NOTIFY_EVENT_HOST)
				|| event == NOTIFY_EVENT_POGO) {
		if (enable) {
			if (n->device_check_sec) {
				if (prev_c_type != NOTIFY_EVENT_HOST)
					u_notify->is_device = 0;
				u_notify->check_work_complete = 0;
				schedule_delayed_work(&u_notify->check_work,
					n->device_check_sec*HZ);
				pr_info("%s check work start\n", __func__);
			}
		} else {
			if (n->device_check_sec &&
				!u_notify->check_work_complete) {
				pr_info("%s check work cancel\n", __func__);
				cancel_delayed_work_sync(&u_notify->check_work);
			}
			u_notify->is_device = 0;
		}
	}
	if (type & NOTIFY_EVENT_NEED_HOST) {
		if (!enable) {
			u_notify->is_device = 0;
			pr_info("%s end host\n", __func__);
			send_external_notify(EXTERNAL_NOTIFY_DEVICEADD, 0);
		}
	}
err:
	update_cable_status(n, event, virtual, enable, 0);

no_save_event:
	pr_info("%s- event=%s, cable=%s\n", __func__,
		event_string(event),
			event_string(u_notify->c_type));
}

static void extra_notify_state(struct otg_notify *n,
			unsigned long event, int enable)
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);
	int status = 0;

	pr_info("%s+ event=%s(%lu), enable=%s\n", __func__,
		event_string(event), event, enable == 0 ? "off" : "on");

	switch (event) {
	case NOTIFY_EVENT_NONE:
		break;
	case NOTIFY_EVENT_OVERCURRENT:
		if (!u_notify->ndev.dev) {
			pr_err("ndev is NULL. Maybe usb host is not supported.\n");
			break;
		}
		host_state_notify(&u_notify->ndev,
						NOTIFY_HOST_OVERCURRENT);
		pr_err("OTG overcurrent!!!!!!\n");
		status = NOTIFY_EXTRA_USBHOST_OVERCURRENT;
		store_usblog_notify(NOTIFY_EXTRA,
			(void *)&status, NULL);
		break;
	case NOTIFY_EVENT_VBUSPOWER:
		if (enable) {
			u_notify->ndev.booster = NOTIFY_POWER_ON;
			status = NOTIFY_EVENT_ENABLED;
		} else {
			u_notify->ndev.booster = NOTIFY_POWER_OFF;
			status = NOTIFY_EVENT_DISABLED;
		}

		store_usblog_notify(NOTIFY_EVENT,
			(void *)&event, (void *)&status);
		break;
	case NOTIFY_EVENT_SMSC_OVC:
		if (enable)
			ovc_start(u_notify);
		else
			ovc_stop(u_notify);
		break;
	case NOTIFY_EVENT_SMTD_EXT_CURRENT:
		if (u_notify->c_type != NOTIFY_EVENT_SMARTDOCK_TA) {
			pr_err("No smart dock!!!!!!\n");
			break;
		}
		if (n->set_battcall)
			n->set_battcall
				(NOTIFY_EVENT_SMTD_EXT_CURRENT, enable);
		break;
	case NOTIFY_EVENT_MMD_EXT_CURRENT:
		if (u_notify->c_type != NOTIFY_EVENT_MMDOCK) {
			pr_err("No mmdock!!!!!!\n");
			break;
		}
		if (n->set_battcall)
			n->set_battcall
				(NOTIFY_EVENT_MMD_EXT_CURRENT, enable);
		break;
	case NOTIFY_EVENT_HMD_EXT_CURRENT:
		if (n->set_battcall)
			n->set_battcall
				(NOTIFY_EVENT_HMD_EXT_CURRENT, enable);
		break;
	case NOTIFY_EVENT_DEVICE_CONNECT:
		if (!u_notify->is_device) {
			u_notify->is_device = 1;
			send_external_notify(EXTERNAL_NOTIFY_DEVICEADD, 1);
		}
		break;
	case NOTIFY_EVENT_GAMEPAD_CONNECT:
		if (u_notify->c_type == NOTIFY_EVENT_HOST ||
			u_notify->c_type == NOTIFY_EVENT_GAMEPAD)
			send_external_notify(EXTERNAL_NOTIFY_DEVICE_CONNECT,
					EXTERNAL_NOTIFY_GPAD);
		break;
	case NOTIFY_EVENT_LANHUB_CONNECT:
		if (u_notify->c_type == NOTIFY_EVENT_HOST ||
			u_notify->c_type == NOTIFY_EVENT_LANHUB)
			send_external_notify(EXTERNAL_NOTIFY_DEVICE_CONNECT,
					EXTERNAL_NOTIFY_LANHUB);
		break;
	case NOTIFY_EVENT_POWER_SOURCE:
		if (enable) {
			u_notify->typec_status.power_role = HNOTIFY_SOURCE;
			host_state_notify(&u_notify->ndev,
						NOTIFY_HOST_SOURCE);
		} else {
			u_notify->typec_status.power_role = HNOTIFY_SINK;
			host_state_notify(&u_notify->ndev,
						NOTIFY_HOST_SINK);
		}
		send_external_notify(EXTERNAL_NOTIFY_POWERROLE,
				u_notify->typec_status.power_role);
		break;
	case NOTIFY_EVENT_PD_CONTRACT:
		if (enable)
			u_notify->typec_status.pd = enable;
		else
			u_notify->typec_status.pd = 0;
		break;
	case NOTIFY_EVENT_VBUS_RESET:
		send_external_notify(EXTERNAL_NOTIFY_VBUS_RESET, 0);
		break;
	case NOTIFY_EVENT_RESERVE_BOOSTER:
		mutex_lock(&u_notify->state_lock);
		if (enable)
			u_notify->reserve_vbus_booster = 1;
		else
			u_notify->reserve_vbus_booster = 0;
		mutex_unlock(&u_notify->state_lock);
		break;
	case NOTIFY_EVENT_USB_CABLE:
		mutex_lock(&u_notify->state_lock);
		if (enable)
			u_notify->gadget_status.usb_cable_connect = 1;
		else
			u_notify->gadget_status.usb_cable_connect = 0;

		if (u_notify->ndev.mode == NOTIFY_PERIPHERAL_MODE) {
			if ((u_notify->gadget_status.bus_state
						== NOTIFY_USB_SUSPENDED)
				&& u_notify->gadget_status.usb_cable_connect) {
				if (n->set_chg_current)
					n->set_chg_current
						(NOTIFY_USB_SUSPENDED);
			}
		}
		mutex_unlock(&u_notify->state_lock);
		break;
	case NOTIFY_EVENT_USBD_SUSPENDED:
		mutex_lock(&u_notify->state_lock);
		if (u_notify->ndev.mode == NOTIFY_PERIPHERAL_MODE) {
			u_notify->gadget_status.bus_state
					= NOTIFY_USB_SUSPENDED;
			if (u_notify->gadget_status.usb_cable_connect) {
				if (n->set_chg_current)
					n->set_chg_current
						(NOTIFY_USB_SUSPENDED);
			}
		}
		mutex_unlock(&u_notify->state_lock);
		break;
	case NOTIFY_EVENT_USBD_UNCONFIGURED:
		mutex_lock(&u_notify->state_lock);
		if (u_notify->ndev.mode == NOTIFY_PERIPHERAL_MODE)
			u_notify->gadget_status.bus_state
					= NOTIFY_USB_UNCONFIGURED;
		mutex_unlock(&u_notify->state_lock);
		break;
	case NOTIFY_EVENT_USBD_CONFIGURED:
		mutex_lock(&u_notify->state_lock);
		if (u_notify->ndev.mode == NOTIFY_PERIPHERAL_MODE)
			u_notify->gadget_status.bus_state
					= NOTIFY_USB_CONFIGURED;
		mutex_unlock(&u_notify->state_lock);
		break;
	default:
		break;
	}
	pr_info("%s- event=%s(%lu), cable=%s\n", __func__,
		event_string(event), event,
		event_string(u_notify->c_type));
}

static void otg_notify_work(struct work_struct *data)
{
	struct otg_state_work *state_work =
		container_of(data, struct otg_state_work, otg_work);

	otg_notify_state(state_work->o_notify,
			state_work->event, state_work->enable);

	kfree(state_work);
}

static int otg_notifier_callback(struct notifier_block *nb,
		unsigned long event, void *param)
{
	struct usb_notify *u_noti = container_of(nb,
			struct usb_notify, otg_nb);
	struct otg_notify *n = NULL;
	struct otg_state_work *state_work = NULL;

	n = u_noti->o_notify;

	pr_info("%s event=%s(%lu)\n", __func__,
			event_string(event), event);

	if (event > VIRT_EVENT(NOTIFY_EVENT_VBUSPOWER)) {
		pr_err("%s event is invalid\n", __func__);
		return NOTIFY_DONE;
	}

	state_work = kmalloc(sizeof(struct otg_state_work), GFP_ATOMIC);
	if (!state_work)
		return notifier_from_errno(-ENOMEM);

	INIT_WORK(&state_work->otg_work, otg_notify_work);
	state_work->o_notify = n;
	state_work->event = event;
	state_work->enable = *(int *)param;
	queue_work(u_noti->notifier_wq, &state_work->otg_work);
	return NOTIFY_OK;
}

static int extra_notifier_callback(struct notifier_block *nb,
		unsigned long event, void *param)
{
	struct usb_notify *u_noti = container_of(nb,
			struct usb_notify, extra_nb);
	struct otg_notify *n = NULL;

	n = u_noti->o_notify;

	pr_info("%s event=%s(%lu)\n", __func__,
			event_string(event), event);

	if (event > VIRT_EVENT(NOTIFY_EVENT_VBUSPOWER)) {
		pr_err("%s event is invalid\n", __func__);
		return NOTIFY_DONE;
	}

	extra_notify_state(n, event, *(int *)param);

	return NOTIFY_OK;
}

void send_otg_notify(struct otg_notify *n,
				unsigned long event, int enable)
{
	struct usb_notify *u_notify = NULL;
	int type = 0;

	if (!n) {
		pr_err("%s otg_notify is null\n", __func__);
		goto end;
	}

	u_notify = (struct usb_notify *)(n->u_notify);
	if (!u_notify) {
		pr_err("%s u_notify structure is null\n", __func__);
		goto end;
	}
	pr_info("%s event=%s(%lu) enable=%d\n", __func__,
			event_string(event), event, enable);

	type = check_event_type(event);

	if (type & NOTIFY_EVENT_DELAY) {
		if (n->booting_delay_sec) {
			if (u_notify) {
				u_notify->b_delay.reserve_state =
					(enable) ? event : NOTIFY_EVENT_NONE;
				pr_info("%s reserve event\n", __func__);
			} else
				pr_err("%s u_notify is null\n", __func__);
			goto end;
		}
	}

	if (type & NOTIFY_EVENT_EXTRA)
		blocking_notifier_call_chain
			(&u_notify->extra_notifier, event, &enable);
	else if (type & NOTIFY_EVENT_STATE)
		atomic_notifier_call_chain
			(&u_notify->otg_notifier, event, &enable);
	else
		goto end;
end:
	return;
}
EXPORT_SYMBOL(send_otg_notify);

int get_typec_status(struct otg_notify *n, int event)
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);
	int ret = -ENODEV;

	if (u_notify == NULL) {
		pr_err("u_notify is NULL\n");
		goto end;
	}

	if (event == NOTIFY_EVENT_POWER_SOURCE) {
		/* SINK == 0, SOURCE == 1 */
		ret = u_notify->typec_status.power_role;
	} else
		ret = u_notify->typec_status.pd;
end:
	return ret;
}
EXPORT_SYMBOL(get_typec_status);

struct otg_booster *find_get_booster(struct otg_notify *n)
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);
	int ret = 0;

	if (!u_notify) {
		pr_err("%s u_notify structure is null\n", __func__);
		goto err;
	}

	if (!u_notify_core) {
		ret = create_usb_notify();
		if (ret) {
			pr_err("unable create_usb_notify\n");
			goto err;
		}
	}

	if (!u_notify->booster) {
		pr_err("error. No matching booster\n");
		goto err;
	}

	return u_notify->booster;
err:
	return NULL;
}
EXPORT_SYMBOL(find_get_booster);

int register_booster(struct otg_notify *n, struct otg_booster *b)
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);
	int ret = 0;

	if (!u_notify) {
		pr_err("%s u_notify structure is null\n", __func__);
		goto err;
	}

	u_notify->booster = b;
err:
	return ret;
}
EXPORT_SYMBOL(register_booster);

int register_ovc_func(struct otg_notify *n,
			int (*check_state)(void *), void *data)
{
	struct usb_notify *u_notify;
	int ret = 0;

	if (!n) {
		pr_err("%s otg_notify is null\n", __func__);
		return -ENODEV;
	}

	u_notify = (struct usb_notify *)(n->u_notify);
	if (!u_notify) {
		pr_err("%s u_notify structure is null\n", __func__);
		return -EFAULT;
	}

	mutex_lock(&u_notify->ovc_info.ovc_lock);
	u_notify->ovc_info.check_state = check_state;
	u_notify->ovc_info.data = data;
	mutex_unlock(&u_notify->ovc_info.ovc_lock);
	pr_info("%s\n", __func__);
	return ret;
}
EXPORT_SYMBOL(register_ovc_func);

int get_booster(struct otg_notify *n)
{
	struct usb_notify *u_notify;
	int ret = 0;

	if (!n) {
		pr_err("%s otg_notify is null\n", __func__);
		return -ENODEV;
	}

	u_notify = (struct usb_notify *)(n->u_notify);
	if (!u_notify) {
		pr_err("%s u_notify structure is null\n", __func__);
		return NOTIFY_NONE_MODE;
	}

	if (!u_notify_core) {
		ret = create_usb_notify();
		if (ret) {
			pr_err("unable create_usb_notify\n");
			return -EFAULT;
		}
	}
	pr_info("%s usb booster=%d\n", __func__, u_notify->ndev.booster);
	ret = u_notify->ndev.booster;
	return ret;
}
EXPORT_SYMBOL(get_booster);

int get_usb_mode(struct otg_notify *n)
{
	struct usb_notify *u_notify;
	int ret = 0;

	if (!n) {
		pr_err("%s otg_notify is null\n", __func__);
		return -ENODEV;
	}

	u_notify = (struct usb_notify *)(n->u_notify);
	if (!u_notify) {
		pr_err("%s u_notify structure is null\n", __func__);
		return NOTIFY_NONE_MODE;
	}

	if (!u_notify_core) {
		ret = create_usb_notify();
		if (ret) {
			pr_err("unable create_usb_notify\n");
			return -EFAULT;
		}
	}
	pr_info("%s usb mode=%d\n", __func__, u_notify->ndev.mode);
	ret = u_notify->ndev.mode;
	return ret;
}
EXPORT_SYMBOL(get_usb_mode);

unsigned long get_cable_type(struct otg_notify *n)
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);
	unsigned long ret = 0;
	int noti_ret = 0;

	if (!u_notify) {
		pr_err("%s u_notify structure is null\n", __func__);
		return NOTIFY_EVENT_NONE;
	}

	if (!u_notify_core) {
		noti_ret = create_usb_notify();
		if (noti_ret) {
			pr_err("unable create_usb_notify\n");
			return NOTIFY_EVENT_NONE;
		}
	}
	pr_info("%s cable type =%s\n", __func__,
			event_string(u_notify->c_type));
	ret = u_notify->c_type;
	return ret;
}
EXPORT_SYMBOL(get_cable_type);

int is_usb_host(struct otg_notify *n)
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);
	int ret = 0;
	int noti_ret = 0;

	if (!u_notify) {
		pr_err("%s u_notify structure is null\n", __func__);
		return NOTIFY_EVENT_NONE;
	}

	if (!u_notify_core) {
		noti_ret = create_usb_notify();
		if (noti_ret) {
			pr_err("unable create_usb_notify\n");
			return NOTIFY_EVENT_NONE;
		}
	}

	if (n->unsupport_host || !IS_ENABLED(CONFIG_USB_HOST_NOTIFY))
		ret = 0;
	else
		ret = 1;

	pr_info("%s %d\n", __func__, ret);
	return ret;
}
EXPORT_SYMBOL(is_usb_host);

bool is_blocked(struct otg_notify *n, int type)
{
	struct usb_notify *u_notify = NULL;
	int ret = 0;

	if (!n) {
		pr_err("%s otg_notify is null\n", __func__);
		goto end;
	}
	u_notify = (struct usb_notify *)(n->u_notify);

	if (!u_notify) {
		pr_err("%s u_notify structure is null\n", __func__);
		goto end;
	}

	if (!u_notify_core) {
		ret = create_usb_notify();
		if (ret) {
			pr_err("unable create_usb_notify\n");
			goto end;
		}
	}
	pr_info("%s type=%d, disable_state=%lu\n",
		__func__, type, u_notify->udev.disable_state);

	if (type == NOTIFY_BLOCK_TYPE_HOST) {
		if (test_bit(NOTIFY_BLOCK_TYPE_HOST,
				&u_notify->udev.disable_state))
			goto end2;
	} else if (type == NOTIFY_BLOCK_TYPE_CLIENT) {
		if (test_bit(NOTIFY_BLOCK_TYPE_CLIENT,
				&u_notify->udev.disable_state))
			goto end2;
	} else if (type == NOTIFY_BLOCK_TYPE_ALL) {
		if (test_bit(NOTIFY_BLOCK_TYPE_HOST,
			&u_notify->udev.disable_state) &&
				test_bit(NOTIFY_BLOCK_TYPE_CLIENT,
					&u_notify->udev.disable_state))
			goto end2;
	}

end:
	return false;
end2:
	return true;
}
EXPORT_SYMBOL(is_blocked);

bool is_snkdfp_usb_device_connected(struct otg_notify *n)
{
	struct usb_notify *u_notify = NULL;

	if (!n) {
		pr_err("%s otg_notify is null\n", __func__);
		return false;
	}
	u_notify = (struct usb_notify *)(n->u_notify);

	if (!u_notify) {
		pr_err("%s u_notify structure is null\n", __func__);
		return false;
	}

	pr_info("%s is_device = %d, power_role = %d\n",
		__func__, u_notify->is_device,
			u_notify->typec_status.power_role);
	if (u_notify->is_device &&
		u_notify->typec_status.power_role == HNOTIFY_SINK) {
		return true;
	}

	return false;
}
EXPORT_SYMBOL(is_snkdfp_usb_device_connected);

struct dev_table {
	struct usb_device_id dev;
	int index;
};

static struct dev_table known_usbaudio_device_table[] = {
	{ .dev = { USB_DEVICE(0x04e8, 0xa051), },
	},
	{ .dev = { USB_DEVICE(0x04e8, 0xa054), },
	},
	{ .dev = { USB_DEVICE(0x04e8, 0xa05b), },
	},
	{ .dev = { USB_DEVICE(0x04e8, 0xa058), },
	},
	{ .dev = { USB_DEVICE(0x04e8, 0xa057), },
	},
	{ .dev = { USB_DEVICE(0x04e8, 0xa059), },
	},
	{ .dev = { USB_DEVICE(0x04e8, 0xa05e), },
	},
	{}
};

static int check_audio_id(struct usb_device *dev)
{
	struct dev_table *id;
	int ret = 0;

	/* check VID, PID */
	for (id = known_usbaudio_device_table; id->dev.match_flags; id++) {
		if ((id->dev.match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
		(id->dev.match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
		id->dev.idVendor == le16_to_cpu(dev->descriptor.idVendor) &&
		id->dev.idProduct == le16_to_cpu(dev->descriptor.idProduct)) {
			ret = 1;
			break;
		}
	}
	if (ret)
		pr_info("%s find\n", __func__);

	return ret;
}

static int check_audio_descriptor(struct usb_device *dev)
{
	struct usb_interface *intf;
	struct usb_host_interface *alts;
	struct usb_endpoint_descriptor *endpt;
	unsigned int i, j;
	int ret = 0;
	__u8 play_intf = 0, cap_intf = 0;
	__u8 aud_con_cnt = 0, out_ep = 0, in_ep = 0;

/* 1. check samsung vid */
	if (le16_to_cpu(dev->descriptor.idVendor) != 0x04e8)
		goto done;

/* 2. If set config is not execute, return false */
	if (!dev->actconfig) {
		pr_info("%s no set config\n", __func__);
		goto done;
	}

	for (i = 0; i < dev->actconfig->desc.bNumInterfaces; i++) {
		intf = dev->actconfig->interface[i];
		alts = intf->cur_altsetting;

		if (alts->desc.bInterfaceClass == USB_CLASS_AUDIO) {
			if (alts->desc.bInterfaceSubClass
					== USB_SUBCLASS_AUDIOCONTROL)
				aud_con_cnt++;
			if (alts->desc.bInterfaceSubClass
					!= USB_SUBCLASS_AUDIOSTREAMING &&
					alts->desc.bInterfaceSubClass
						!= USB_CLASS_VENDOR_SPEC)
				continue;

			out_ep = 0;
			in_ep = 0;
			for (j = 0; j < intf->num_altsetting; j++) {
				alts = &intf->altsetting[j];

				if (alts->desc.bNumEndpoints < 1)
					continue;

				endpt = &alts->endpoint[0].desc;
				 /*
				  * If there is endpoint[1],
				  *	it will be sync endpoint(feedback).
				  */

				if (!endpt)
					continue;

				if (endpt->bEndpointAddress & USB_DIR_IN) {
					if (!in_ep)
						in_ep = endpt->bEndpointAddress;
					else if (in_ep !=
						endpt->bEndpointAddress) {
						pr_info("%s in_ep 2 or more\n",
								__func__);
						goto done;
					} else
						continue;
				} else {
					if (!out_ep)
						out_ep =
							endpt->bEndpointAddress;
					else if (out_ep !=
						endpt->bEndpointAddress) {
						pr_info("%s out_ep 2 or more\n",
								__func__);
						goto done;
					} else
						continue;
				}
			}
			if (out_ep)
				play_intf++;
			else if (in_ep)
				cap_intf++;
			else {
				pr_err("%s no ep\n", __func__);
				goto done;
			}
		}
	}
/* 3. final check. AUDIOCONTROL 1. playback 1. capture 1 */
	if (aud_con_cnt == 1 && play_intf == 1 && cap_intf == 1)
		ret = 1;
done:
	if (aud_con_cnt)
		pr_info("%s ret=%d,aud_con_cnt=%d,play_intf=%d,cap_intf=%d\n",
			__func__, ret, aud_con_cnt, play_intf, cap_intf);
	return ret;
}

int is_known_usbaudio(struct usb_device *dev)
{
	int ret = 0;

	ret = check_audio_id(dev);
	if (ret)
		goto done;

	ret = check_audio_descriptor(dev);
	if (ret)
		goto done;

done:
	return ret;
}
EXPORT_SYMBOL(is_known_usbaudio);

void set_usb_audio_cardnum(int card_num, int bundle, int attach)
{
	struct otg_notify *o_notify = get_otg_notify();
	struct usb_notify *u_notify = NULL;

	if (!o_notify) {
		pr_err("%s o_notify is null\n", __func__);
		goto err;
	}
	u_notify = (struct usb_notify *)(o_notify->u_notify);

	if (!u_notify) {
		pr_err("%s u_notify structure is null\n",
			__func__);
		goto err;
	}

	pr_info("%s card=%d attach=%d\n", __func__, card_num, attach);

	if (attach) {
		u_notify->udev.usb_audio_cards[card_num].cards = 1;
		if (bundle)
			u_notify->udev.usb_audio_cards[card_num].bundle = 1;
	} else {
		u_notify->udev.usb_audio_cards[card_num].cards = 0;
		u_notify->udev.usb_audio_cards[card_num].bundle = 0;
	}
err:
	return;
}
EXPORT_SYMBOL(set_usb_audio_cardnum);

#ifdef CONFIG_USB_AUDIO_ENHANCED_DETECT_TIME
int __weak get_next_snd_card_number(struct module *module)
{
	int idx = -1;

	pr_info("%s call weak function\n", __func__);
	return idx;
}
#endif

void send_usb_audio_uevent(struct usb_device *dev,
		int card_num, int attach)
{
	struct otg_notify *o_notify = get_otg_notify();
	char *envp[6];
	char *type = {"TYPE=usbaudio"};
	char *state_add = {"STATE=ADD"};
	char *state_remove = {"STATE=REMOVE"};
	char vidpid_vuf[15];
	char path_buf[50];
	int index = 0;
#ifdef CONFIG_USB_AUDIO_ENHANCED_DETECT_TIME
	char cardnum_buf[10];
	int cardnum = 0;
#endif

	if (!o_notify) {
		pr_err("%s o_notify is null\n", __func__);
		goto err;
	}

	if (!is_known_usbaudio(dev))
		goto err;

	envp[index++] = type;

	if (attach)
		envp[index++] = state_add;
	else
		envp[index++] = state_remove;

	snprintf(vidpid_vuf, sizeof(vidpid_vuf),
		"ID=%04X/%04X", le16_to_cpu(dev->descriptor.idVendor),
				le16_to_cpu(dev->descriptor.idProduct));

	envp[index++] = vidpid_vuf;

	snprintf(path_buf, sizeof(path_buf),
		"PATH=/dev/bus/usb/%03d/%03d", dev->bus->busnum, dev->devnum);

	envp[index++] = path_buf;

#ifdef CONFIG_USB_AUDIO_ENHANCED_DETECT_TIME
	if (attach && !card_num) {
		cardnum = get_next_snd_card_number(THIS_MODULE);
		if (cardnum < 0) {
			pr_err("%s cardnum error\n", __func__);
			goto err;
		}
	} else
		cardnum = card_num;

	set_usb_audio_cardnum(cardnum, 1, attach);

	snprintf(cardnum_buf, sizeof(cardnum_buf),
		"CARDNUM=%d", cardnum);

	envp[index++] = cardnum_buf;
#endif

	envp[index++] = NULL;

	if (send_usb_notify_uevent(o_notify, envp)) {
		pr_err("%s error\n", __func__);
		goto err;
	}
	pr_info("%s\n", __func__);
err:
	return;
}
EXPORT_SYMBOL(send_usb_audio_uevent);

int send_usb_notify_uevent(struct otg_notify *n, char *envp_ext[])
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);
	int ret = 0;

	if (!u_notify) {
		pr_err("%s u_notify is null\n", __func__);
		ret = -EFAULT;
		goto err;
	}

	ret = usb_notify_dev_uevent(&u_notify->udev, envp_ext);
err:
	return ret;
}
EXPORT_SYMBOL(send_usb_notify_uevent);

#if defined(CONFIG_USB_HW_PARAM)
unsigned long long *get_hw_param(struct otg_notify *n,
	enum usb_hw_param index)
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);
	int ret = 0;

	if (index < 0 || index >= USB_CCIC_HW_PARAM_MAX) {
		pr_err("%s usb_hw_param is out of bound\n", __func__);
		return NULL;
	}

	if (!u_notify) {
		pr_err("%s u_notify structure is null\n", __func__);
		return NULL;
	}

	if (!u_notify_core) {
		ret = create_usb_notify();
		if (ret) {
			pr_err("unable create_usb_notify\n");
			return NULL;
		}
	}
	return &(u_notify->hw_param[index]);
}
EXPORT_SYMBOL(get_hw_param);

int inc_hw_param(struct otg_notify *n,
	enum usb_hw_param index)
{
	struct usb_notify *u_notify;
	int ret = 0;

	if (!n) {
		pr_err("%s otg_notify is null\n", __func__);
		return -ENODEV;
	}

	u_notify = (struct usb_notify *)(n->u_notify);

	if (index < 0 || index >= USB_CCIC_HW_PARAM_MAX) {
		pr_err("%s usb_hw_param is out of bound\n", __func__);
		ret = -ENOMEM;
		return ret;
	}

	if (!u_notify) {
		pr_err("%s u_notify structure is null\n", __func__);
		ret = -ENOENT;
		return ret;
	}

	if (!u_notify_core) {
		ret = create_usb_notify();
		if (ret) {
			pr_err("unable create_usb_notify\n");
			return ret;
		}
	}
	u_notify->hw_param[index]++;
	return ret;
}
EXPORT_SYMBOL(inc_hw_param);

int inc_hw_param_host(struct host_notify_dev *dev,
	enum usb_hw_param index)
{
	struct usb_notify *u_notify = container_of(dev,
			struct usb_notify, ndev);
	int ret = 0;

	if (index < 0 || index >= USB_CCIC_HW_PARAM_MAX) {
		pr_err("%s usb_hw_param is out of bound\n", __func__);
		ret = -ENOMEM;
		return ret;
	}

	if (!u_notify) {
		pr_err("%s u_notify structure is null\n", __func__);
		ret = -ENOENT;
		return ret;
	}

	if (!u_notify_core) {
		ret = create_usb_notify();
		if (ret) {
			pr_err("unable create_usb_notify\n");
			return ret;
		}
	}
	u_notify->hw_param[index]++;
	return ret;
}
EXPORT_SYMBOL(inc_hw_param_host);

int register_hw_param_manager(struct otg_notify *n, unsigned long (*fptr)(int))
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);
	int ret = 0;

	if (!u_notify) {
		pr_err("%s u_notify structure is null\n", __func__);
		ret = -ENOENT;
		goto err;
	}

	if (!u_notify_core) {
		ret = create_usb_notify();
		if (ret) {
			pr_err("unable create_usb_notify\n");
			goto err;
		}
	}
	u_notify->udev.fp_hw_param_manager = fptr;
	pr_info("%s\n", __func__);
err:
	return ret;
}
EXPORT_SYMBOL(register_hw_param_manager);
#endif

void *get_notify_data(struct otg_notify *n)
{
	if (n)
		return n->o_data;
	else
		return NULL;
}
EXPORT_SYMBOL(get_notify_data);

void set_notify_data(struct otg_notify *n, void *data)
{
	n->o_data = data;
}
EXPORT_SYMBOL(set_notify_data);

struct otg_notify *get_otg_notify(void)
{
	if (!u_notify_core)
		return NULL;
	if (!u_notify_core->o_notify)
		return NULL;
	return u_notify_core->o_notify;
}
EXPORT_SYMBOL(get_otg_notify);

int set_otg_notify(struct otg_notify *n)
{
	struct usb_notify *u_notify;
	int ret = 0;

	if (!u_notify_core) {
		ret = create_usb_notify();
		if (ret) {
			pr_err("unable create_usb_notify\n");
			goto err;
		}
	}

	if (u_notify_core->o_notify && n) {
		pr_err("error : already set o_notify\n");
		goto err;
	}

	pr_info("registered otg_notify +\n");
	if (!n) {
		pr_err("otg notify structure is null\n");
		ret = -EFAULT;
		goto err1;
	}
	u_notify_core->o_notify = n;

	u_notify = kzalloc(sizeof(struct usb_notify), GFP_KERNEL);
	if (!u_notify) {
		ret = -ENOMEM;
		goto err1;
	}

	u_notify->o_notify = n;

	n->u_notify = (void *)u_notify;

	u_notify->notifier_wq
		= create_singlethread_workqueue("usb_notify");
	if (!u_notify->notifier_wq) {
		pr_err("%s failed to create work queue\n", __func__);
		ret = -ENOMEM;
		goto err2;
	}

	ovc_init(u_notify);
	mutex_init(&u_notify->state_lock);

	ATOMIC_INIT_NOTIFIER_HEAD(&u_notify->otg_notifier);
	u_notify->otg_nb.notifier_call = otg_notifier_callback;
	ret = atomic_notifier_chain_register(&u_notify->otg_notifier,
				&u_notify->otg_nb);
	if (ret < 0) {
		pr_err("atomic_notifier_chain_register failed\n");
		goto err3;
	}

	BLOCKING_INIT_NOTIFIER_HEAD(&u_notify->extra_notifier);
	u_notify->extra_nb.notifier_call = extra_notifier_callback;
	ret = blocking_notifier_chain_register
		(&u_notify->extra_notifier, &u_notify->extra_nb);
	if (ret < 0) {
		pr_err("blocking_notifier_chain_register failed\n");
		goto err4;
	}

	if (!n->unsupport_host) {
		u_notify->ndev.name = "usb_otg";
		u_notify->ndev.set_booster = n->vbus_drive;
		u_notify->ndev.set_mode = n->set_host;
		ret = host_notify_dev_register(&u_notify->ndev);
		if (ret < 0) {
			pr_err("host_notify_dev_register is failed\n");
			goto err5;
		}

		if (!n->vbus_drive) {
			pr_err("vbus_drive is null\n");
			goto err6;
		}
	}

	u_notify->udev.name = "usb_control";
	u_notify->udev.set_disable = set_notify_disable;
	u_notify->udev.set_mdm = set_notify_mdm;
	u_notify->udev.fp_hw_param_manager = NULL;
	u_notify->udev.o_notify = n;

	ret = usb_notify_dev_register(&u_notify->udev);
	if (ret < 0) {
		pr_err("usb_notify_dev_register is failed\n");
		goto err6;
	}

	if (gpio_is_valid(n->vbus_detect_gpio) ||
			gpio_is_valid(n->redriver_en_gpio)) {
		ret = register_gpios(n);
		if (ret < 0) {
			pr_err("register_gpios is failed\n");
			goto err7;
		}
	}

	if (n->is_wakelock || n->is_host_wakelock) {
		u_notify->ws.name = "usb_notify";
		wakeup_source_add(&u_notify->ws);
	}

	if (n->booting_delay_sec) {
		INIT_DELAYED_WORK(&u_notify->b_delay.booting_work,
				  reserve_state_check);
		schedule_delayed_work(&u_notify->b_delay.booting_work,
				n->booting_delay_sec*HZ);
	}

	if (n->device_check_sec)
		INIT_DELAYED_WORK(&u_notify->check_work,
				  device_connect_check);

	register_usbdev_notify();

	register_usblog_proc();

	pr_info("registered otg_notify -\n");
	return 0;
err7:
	usb_notify_dev_unregister(&u_notify->udev);
err6:
	if (!n->unsupport_host)
		host_notify_dev_unregister(&u_notify->ndev);
err5:
	blocking_notifier_chain_unregister(&u_notify->extra_notifier,
				&u_notify->extra_nb);
err4:
	atomic_notifier_chain_unregister(&u_notify->otg_notifier,
				&u_notify->otg_nb);
err3:
	flush_workqueue(u_notify->notifier_wq);
	destroy_workqueue(u_notify->notifier_wq);
err2:
	kfree(u_notify);
err1:
	u_notify_core->o_notify = NULL;
err:
	return ret;
}
EXPORT_SYMBOL(set_otg_notify);

void put_otg_notify(struct otg_notify *n)
{
	struct usb_notify *u_notify = (struct usb_notify *)(n->u_notify);

	if (!u_notify) {
		pr_err("%s u_notify structure is null\n", __func__);
		return;
	}
	unregister_usblog_proc();
	unregister_usbdev_notify();
	if (n->booting_delay_sec)
		cancel_delayed_work_sync(&u_notify->b_delay.booting_work);
	if (n->is_wakelock || n->is_host_wakelock)
		wakeup_source_remove(&u_notify->ws);

	if (gpio_is_valid(n->vbus_detect_gpio))
		free_irq(gpio_to_irq(n->vbus_detect_gpio), NULL);
	usb_notify_dev_unregister(&u_notify->udev);
	if (!n->unsupport_host)
		host_notify_dev_unregister(&u_notify->ndev);
	blocking_notifier_chain_unregister(&u_notify->extra_notifier,
				&u_notify->extra_nb);
	atomic_notifier_chain_unregister(&u_notify->otg_notifier,
				&u_notify->otg_nb);
	flush_workqueue(u_notify->notifier_wq);
	destroy_workqueue(u_notify->notifier_wq);
	u_notify->o_notify = NULL;
	kfree(u_notify);
}
EXPORT_SYMBOL(put_otg_notify);

static int __init usb_notify_init(void)
{
	return create_usb_notify();
}

static void __exit usb_notify_exit(void)
{
	if (!u_notify_core)
		return;
	usb_notify_class_exit();
	notify_class_exit();
	kfree(u_notify_core);
}

module_init(usb_notify_init);
module_exit(usb_notify_exit);

MODULE_AUTHOR("Samsung USB Team");
MODULE_DESCRIPTION("USB Notify Layer");
MODULE_LICENSE("GPL");
MODULE_VERSION(NOTIFY_VERSION);
