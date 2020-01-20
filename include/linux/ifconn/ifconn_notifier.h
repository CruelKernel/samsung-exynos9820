/*
 * include/linux/ifconn/ifconn_notifier.h
 *
 * header file supporting IFCONN notifier call chain information
 *
 * Copyright (C) 2010 Samsung Electronics
 * Sejong Park <sejong123.park@samsung.com>
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

#ifndef __IFCONN_MANAGER_NOTIFIER_H__
#define __IFCONN_MANAGER_NOTIFIER_H__

#include <linux/notifier.h>

#define IFCONN_MANAGER_NOTIFIER_BLOCK(name)	\
	struct notifier_block(name)

#define MAX 7

typedef enum {
	IFCONN_NOTIFY_MANAGER = 0,
	IFCONN_NOTIFY_USB,
	IFCONN_NOTIFY_BATTERY,
	IFCONN_NOTIFY_PDIC,
	IFCONN_NOTIFY_MUIC,
	IFCONN_NOTIFY_VBUS,
	IFCONN_NOTIFY_CCIC,
	IFCONN_NOTIFY_DP,
	IFCONN_NOTIFY_USB_DP,
	IFCONN_NOTIFY_ALL,
	IFCONN_NOTIFY_MAX,
} ifconn_notifier_t;

typedef enum {
	IFCONN_NOTIFY_PARAM_DATA,
	IFCONN_NOTIFY_PARAM_TEMPLATE
} ifconn_notifier_data_param_t;

enum {
	IFCONN_NOTIFY_LOW = 0,
	IFCONN_NOTIFY_HIGH,
	IFCONN_NOTIFY_IRQ,
};

typedef enum {
	IFCONN_NOTIFY_ID_INITIAL = 0,
	IFCONN_NOTIFY_ID_ATTACH,
	IFCONN_NOTIFY_ID_DETACH,
	IFCONN_NOTIFY_ID_RID,
	IFCONN_NOTIFY_ID_USB,
	IFCONN_NOTIFY_ID_POWER_STATUS,
	IFCONN_NOTIFY_ID_WATER,
	IFCONN_NOTIFY_ID_OTG,
	IFCONN_NOTIFY_ID_TA,
	IFCONN_NOTIFY_ID_VCONN,
	IFCONN_NOTIFY_ID_DP_CONNECT,
	IFCONN_NOTIFY_ID_DP_HPD,
	IFCONN_NOTIFY_ID_DP_LINK_CONF,
	IFCONN_NOTIFY_ID_USB_DP,
	IFCONN_NOTIFY_ID_ROLE_SWAP,
	IFCONN_NOTIFY_ID_SELECT_PDO,
	IFCONN_NOTIFY_ID_VBUS,
} ifconn_notifier_id_t;

typedef enum {
	IFCONN_NOTIFY_DETACH = 0,
	IFCONN_NOTIFY_ATTACH,
} ifconn_notifier_attach_t;

typedef enum {
	IFCONN_NOTIFY_VBUS_UNKNOWN = 0,
	IFCONN_NOTIFY_VBUS_LOW,
	IFCONN_NOTIFY_VBUS_HIGH,
} ifconn_notifier_vbus_t;

typedef enum {
	IFCONN_NOTIFY_STATUS_NOT_READY = 0,
	IFCONN_NOTIFY_STATUS_NOT_READY_DETECT,
	IFCONN_NOTIFY_STATUS_READY,
} ifconn_notifier_vbus_stat_t;

typedef enum {
	IFCONN_NOTIFY_EVENT_DETACH = 0,
	IFCONN_NOTIFY_EVENT_ATTACH,
	IFCONN_NOTIFY_EVENT_USB_ATTACH_DFP, // Host
	IFCONN_NOTIFY_EVENT_USB_ATTACH_UFP, // Device
	IFCONN_NOTIFY_EVENT_USB_ATTACH_DRP, // Dual role
	IFCONN_NOTIFY_EVENT_USB_ATTACH_HPD, // DP : Hot Plugged Detect
	IFCONN_NOTIFY_EVENT_PD_SINK,
	IFCONN_NOTIFY_EVENT_PD_SOURCE,
	IFCONN_NOTIFY_EVENT_PD_SINK_CAP,
	IFCONN_NOTIFY_EVENT_DP_LOW,
	IFCONN_NOTIFY_EVENT_DP_HIGH,
	IFCONN_NOTIFY_EVENT_DP_IRQ,
	IFCONN_NOTIFY_EVENT_MUIC_LOGICALLY_DETACH,
	IFCONN_NOTIFY_EVENT_MUIC_LOGICALLY_ATTACH,
	IFCONN_NOTIFY_EVENT_IN_HURRY,
} ifconn_notifier_event_t;

typedef enum {
	IFCONN_NOTIFY_DP_PIN_UNKNOWN = 0,
	IFCONN_NOTIFY_DP_PIN_A,
	IFCONN_NOTIFY_DP_PIN_B,
	IFCONN_NOTIFY_DP_PIN_C,
	IFCONN_NOTIFY_DP_PIN_D,
	IFCONN_NOTIFY_DP_PIN_E,
	IFCONN_NOTIFY_DP_PIN_F,
} ifconn_notifier_dp_pinconf_t;

typedef struct _ifconn_power_list {
	int max_voltage;
	int max_current;
} IFCONN_POWER_LIST;

struct ifconn_pd_sink_status {
	IFCONN_POWER_LIST power_list[MAX];
	int available_pdo_num; // the number of available PDO
	int selected_pdo_num; // selected number of PDO to change
	int current_pdo_num; // current number of PDO
	unsigned int rp_currentlvl; // rp current level by ccic
};

struct ifconn_notifier_template {
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t rid:8;
	uint64_t attach:16;
	uint64_t rprd:16;
	uint64_t cable_type:16;
	uint64_t event:16;
	uint64_t drp:16;
	uint64_t sub1:16;
	uint64_t sub2:16;
	uint64_t sub3:16;
	uint64_t up_src:16;
	uint64_t data_size:16;
	ifconn_notifier_vbus_t vbus_type;
	void *data;
	void *pdata;
	void *dest_dev;
};

struct ifconn_notifier {
	struct notifier_block *nb[IFCONN_NOTIFY_MAX];
	struct ifconn_notifier_template ifconn_template[IFCONN_NOTIFY_MAX];
	struct delayed_work notify_work;
	bool sent[IFCONN_NOTIFY_MAX];
	void *dev;
	struct blocking_notifier_head notifier_call_chain;
};

int ifconn_notifier_register(struct notifier_block *nb,
						notifier_fn_t notifier,
						ifconn_notifier_t listener,
						ifconn_notifier_t src);

int ifconn_notifier_unregister(ifconn_notifier_t src,
									ifconn_notifier_t listener);

int ifconn_notifier_notify(ifconn_notifier_t src,
								ifconn_notifier_t dest,
								int noti_id,
								int event,
								ifconn_notifier_data_param_t param_type,
								void *data);

#endif /* __IFCONN_MANAGER_NOTIFIER_H__ */
