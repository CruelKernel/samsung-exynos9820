/*
 * include/linux/muic/ccic_notifier.h
 *
 * header file supporting CCIC notifier call chain information
 *
 * Copyright (C) 2010 Samsung Electronics
 * Seung-Jin Hahn <sjin.hahn@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __USB_TYPEC_MANAGER_NOTIFIER_H__
#define __USB_TYPEC_MANAGER_NOTIFIER_H__

#include <linux/ccic/ccic_notifier.h>
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#ifdef CONFIG_VBUS_NOTIFIER
#include <linux/vbus_notifier.h>
#endif


/* USB TypeC Manager notifier call sequence,
 * largest priority number device will be called first. */


/* CCIC notifier call sequence,
 * largest priority number device will be called first. */
typedef enum {
/* MUIC */
	MANAGER_NOTIFY_MUIC_NONE = 0,
	MANAGER_NOTIFY_MUIC_DOCK,
	MANAGER_NOTIFY_MUIC_MHL,
	MANAGER_NOTIFY_MUIC_USB,
	MANAGER_NOTIFY_MUIC_TSP,
	MANAGER_NOTIFY_MUIC_CHARGER,
	MANAGER_NOTIFY_MUIC_CPUIDLE,
	MANAGER_NOTIFY_MUIC_CPUFREQ,
	MANAGER_NOTIFY_MUIC_TIMEOUT_OPEN_DEVICE,
	MANAGER_NOTIFY_MUIC_UART,

/* CCIC */
	MANAGER_NOTIFY_CCIC_INITIAL = 20,
	MANAGER_NOTIFY_CCIC_MUIC,
	MANAGER_NOTIFY_CCIC_USB,
	MANAGER_NOTIFY_CCIC_BATTERY,
	MANAGER_NOTIFY_CCIC_SUB_BATTERY,
	MANAGER_NOTIFY_CCIC_DP,
	MANAGER_NOTIFY_CCIC_USBDP,
	MANAGER_NOTIFY_CCIC_SENSORHUB,
	MANAGER_NOTIFY_CCIC_WACOM,

/* VBUS */
	MANAGER_NOTIFY_VBUS_USB = 30,
	MANAGER_NOTIFY_VBUS_CHARGER,
} manager_notifier_device_t;

typedef enum {
	CCIC_NOTY,
	MUIC_NOTY,
	VBUS_NOTY,
	USB_STATE_NOTY,
} manager_notify_t;

typedef enum {
	PD_USB_TYPE,
	PD_TA_TYPE,
} pd_usb_state_t;

#if defined(CONFIG_VBUS_NOTIFIER)
typedef enum {
	EVENT_LOAD = 0,
	EVENT_CANCEL,
} manager_muic_event;
#endif

typedef struct
{
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t sub1:16;
	uint64_t sub2:16;
	uint64_t sub3:16;
	void *pd;
} MANAGER_NOTI_TYPEDEF;

struct typec_manager_event_work
{
	struct work_struct typec_manager_work;
	int src;
	int dest;
	int id;
	int sub1;
	int sub2;
	int sub3;
};

typedef struct _manager_data_t
{
	struct blocking_notifier_head manager_muic_notifier;
	struct blocking_notifier_head manager_notifier;
	struct notifier_block ccic_nb;
	struct notifier_block muic_nb;
#if defined(CONFIG_VBUS_NOTIFIER)
	struct notifier_block vbus_nb;
#endif

	struct delayed_work manager_init_work;
	struct workqueue_struct *typec_manager_wq;
	struct workqueue_struct *typec_manager_muic_wq;
	struct delayed_work cable_check_work;
	struct delayed_work usb_event_work;
#if defined(CONFIG_USB_HW_PARAM)
	struct delayed_work rtctime_update_work;
#endif
#if defined(CONFIG_VBUS_NOTIFIER)
	struct delayed_work muic_event_work;
#endif
	struct mutex mo_lock;

	int muic_attach_state;
	int muic_cable_type;
	int muic_attach_state_without_ccic;
#if defined(CONFIG_VBUS_NOTIFIER)
	int muic_fake_event_wq_processing;
#endif
	int vbus_state;

#ifdef CONFIG_USE_SECOND_MUIC
	int second_muic_attach_state;
	int second_muic_cable_type;
#endif

	int ccic_attach_state;	// USB_STATUS_NOTIFY_DETACH, UFP, DFP, DRP, NO_USB
	int ccic_drp_state;
	int ccic_rid_state;
	int cable_type;
	int usb_enum_state;
	bool usb_enable_state;
	unsigned long otg_stamp;
	int vbus_by_otg_detection;
	int pd_con_state;
	int water_det;
	int wVbus_det;
	int is_MPSM;
	void *pd;
#if defined(CONFIG_USB_HW_PARAM)
	int water_count;
	int dry_count;
	int usb_highspeed_count;
	int usb_superspeed_count;
	int waterChg_count;
	unsigned long waterDet_duration;
	unsigned long waterDet_time;
	unsigned long dryDet_time;
	unsigned long wVbus_duration;
	unsigned long wVbusHigh_time;
	unsigned long wVbusLow_time;
#endif
	int water_cable_type;
	int alt_is_support;
	int dp_is_support;
	int dp_attach_state;
	int dp_cable_type;
	int dp_hpd_state;
	int dp_is_connect;
	int dp_hs_connect;
	int dp_check_done;
	struct notifier_block manager_external_notifier_nb; 
}manager_data_t;


#define CCIC_BATTERY	(1<<0)
#define CCIC_USB	(1<<1)
#define CCIC_DP		(1<<2)

#define MANAGER_NOTIFIER_BLOCK(name)	\
	struct notifier_block (name)

extern void manager_notifier_usbdp_support(void);
extern void manager_notifier_test(void *);


/* ccic notifier register/unregister API
 * for used any where want to receive ccic attached device attach/detach. */
extern int manager_notifier_register(struct notifier_block *nb,
		notifier_fn_t notifier, manager_notifier_device_t listener);
extern int manager_notifier_unregister(struct notifier_block *nb);

#endif /* __USB_TYPEC_MANAGER_NOTIFIER_H__ */
