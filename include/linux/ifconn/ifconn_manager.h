/*
 * include/linux/ifconn/ifconn_manager.h
 *
 * header file supporting CCIC notifier call chain information
 *
 * Copyright (C) 2010 Samsung Electronics
 * Sejong Park <sejong.park@samsung.com>
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

#ifndef __IFCONN_MANAGER_H__
#define __IFCONN_MANAGER_H__

typedef enum {
/* MUIC */
	IFCONN_CABLE_TYPE_MUIC_NONE = 0,
	IFCONN_CABLE_TYPE_MUIC_DOCK,
	IFCONN_CABLE_TYPE_MUIC_MHL,
	IFCONN_CABLE_TYPE_MUIC_USB,
	IFCONN_CABLE_TYPE_MUIC_TSP,
	IFCONN_CABLE_TYPE_MUIC_CHARGER,
	IFCONN_CABLE_TYPE_MUIC_CPUIDLE,
	IFCONN_CABLE_TYPE_MUIC_CPUFREQ,
	IFCONN_CABLE_TYPE_MUIC_TIMEOUT_OPEN_DEVICE,

/* CCIC */
	IFCONN_CABLE_TYPE_CCIC_INITIAL = 20,
	IFCONN_CABLE_TYPE_CCIC_MUIC,
	IFCONN_CABLE_TYPE_CCIC_USB,
	IFCONN_CABLE_TYPE_CCIC_BATTERY,
	IFCONN_CABLE_TYPE_CCIC_DP,
	IFCONN_CABLE_TYPE_CCIC_USBDP,
	IFCONN_CABLE_TYPE_CCIC_SENSORHUB,

/* VBUS */
	IFCONN_CABLE_TYPE_VBUS_USB = 30,
	IFCONN_CABLE_TYPE_VBUS_CHARGER,
} ifconn_cable_type_t;

typedef enum {
	IFCONN_PD_USB_TYPE,
	IFCONN_PD_TA_TYPE,
} ifconn_pd_usb_state_t;

typedef enum {
	IFCONN_MANAGER_USB_STATUS_DETACH = 0,
	IFCONN_MANAGER_USB_STATUS_ATTACH_DFP = 1, /* Host */
	IFCONN_MANAGER_USB_STATUS_ATTACH_UFP = 2, /* Device */
	IFCONN_MANAGER_USB_STATUS_ATTACH_DRP = 3, /* Dual role */
	IFCONN_MANAGER_USB_STATUS_ATTACH_HPD = 4, /* DP : Hot Plugged Detect */
} ifconn_manager_usb_status_t;

typedef enum {
	IFCONN_MANAGER_VBUS_STATUS_UNKNOWN = 0,
	IFCONN_MANAGER_VBUS_STATUS_LOW,
	IFCONN_MANAGER_VBUS_STATUS_HIGH,
} ifconn_manager_vbus_status_t;

typedef enum {
	IFCONN_MANAGER_RID_UNDEFINED = 0,
	IFCONN_MANAGER_RID_000K,
	IFCONN_MANAGER_RID_001K,
	IFCONN_MANAGER_RID_255K,
	IFCONN_MANAGER_RID_301K,
	IFCONN_MANAGER_RID_523K,
	IFCONN_MANAGER_RID_619K,
	IFCONN_MANAGER_RID_OPEN,
} ifconn_manager_rid_t;

struct ifconn_manager_platform_data {
	void (*initial_check)(void);
	void (*select_pdo)(int);
	const char *usbpd_name;
	const char *muic_name;
};

#define IFCONN_SEND_NOTI(dest, id, event, data) \
{	\
	int ret;	\
	ret = ifconn_notifier_notify( \
					IFCONN_NOTIFY_MANAGER,	\
					IFCONN_NOTIFY_##dest,	\
					IFCONN_NOTIFY_ID_##id,	\
					IFCONN_NOTIFY_EVENT_##event,	\
					IFCONN_NOTIFY_PARAM_DATA,	\
					data);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti : "#dest" "#id"\n", \
				__func__);	\
	}	\
}

#define IFCONN_SEND_TEMPLATE_NOTI(data) \
{	\
	int ret;	\
	struct ifconn_notifier_template *template	\
		= (struct ifconn_notifier_template *)data;	\
	ret = ifconn_notifier_notify( \
					template->src,	\
					template->dest,	\
					template->id,	\
					template->event,	\
					IFCONN_NOTIFY_PARAM_TEMPLATE,	\
					&template);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti\n", \
				__func__);	\
	}	\
}

#define IFCONN_SEND_TEMPLATE_UP_NOTI(nd) \
{	\
	int ret;	\
	struct ifconn_notifier_template *template	\
		= (struct ifconn_notifier_template *)nd;	\
		pr_info("%s: dbg, line : %d\n", __func__, __LINE__);\
	ret = ifconn_notifier_notify( \
					template->src,	\
					template->dest,	\
					template->id,	\
					template->event,	\
					IFCONN_NOTIFY_PARAM_TEMPLATE,	\
					nd);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti\n", \
				__func__);	\
	}	\
}

struct ifconn_manager_template {
	struct ifconn_notifier_template node;
	void *rp;
	void *np;
};

struct ifconn_manager_data {
	struct ifconn_manager_platform_data *pdata;
	struct notifier_block		nb;
	struct ifconn_notifier_template *template;
	struct ifconn_manager_template *hp;
	struct ifconn_manager_template *tp;
	int template_cnt;
	struct device	*dev;
	struct mutex noti_mutex;
	struct mutex workqueue_mutex;
	struct mutex enqueue_mutex;
	struct work_struct noti_work;

	int muic_action;
	int muic_cable_type;
	int muic_data_refresh;
	int muic_attach_state_without_ccic;
#if defined(CONFIG_VBUS_NOTIFIER)
	int muic_fake_event_wq_processing;
#endif
	int vbus_state;
	/* USB_STATUS_NOTIFY_DETACH, UFP, DFP, DRP, NO_USB */
	int ccic_attach_state;
	int ccic_drp_state;
	int ccic_rid_state;
	int cable_type;
	int usb_enum_state;
	bool usb_enable_state;
	int pd_con_state;
	int water_det;
	int is_UFPS;
	void *pd;
	int water_count;
	int dry_count;
	int usb_highspeed_count;
	int usb_superspeed_count;
	int waterChg_count;
	unsigned long waterDet_duration;
	unsigned long waterDet_time;
	unsigned long dryDet_time;
	int dp_attach_state;
	int dp_cable_type;
	int dp_hpd_state;
	int dp_is_connect;
	int dp_hs_connect;
	int dp_check_done;
};

extern void _ifconn_show_attr(struct ifconn_notifier_template *t);
#endif /* __IFCONN_MANAGER_H__ */
