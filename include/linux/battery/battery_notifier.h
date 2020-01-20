/*
 * include/linux/battery/battery_notifier.h
 *
 * header file supporting battery notifier call chain information
 *
 * Copyright (C) 2016 Samsung Electronics
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

#ifndef __BATTERY_NOTIFIER_H__
#define __BATTERY_NOTIFIER_H__
#define MAX_PDO_NUM 8
#define AVAILABLE_VOLTAGE 9000
#define UNIT_FOR_VOLTAGE 50
#define UNIT_FOR_CURRENT 10
#define UNIT_FOR_APDO_VOLTAGE 100
#define UNIT_FOR_APDO_CURRENT 50

typedef enum {
	CHARGER_NOTIFY = 0,
	PDIC_NOTIFY,
} battery_notifier_id_t;

/* CHARGER notifier call sequence,
 * largest priority number device will be called first. */
typedef enum {
	CHARGER_NOTIFY_DEV_BATTERY = 0,
	CHARGER_NOTIFY_DEV_MULTICHARGER,
	CHARGER_NOTIFY_DEV_CHARGER,
	CHARGER_NOTIFY_DEV_FUELGAUGE,
} charger_notifier_device_t;

typedef enum {
	CHARGER_NOTIFY_EVENT_INITIAL = 0,
	CHARGER_NOTIFY_EVENT_AICL,
} charger_notifier_event_t;

typedef struct _charger_aicl_status {
	int input_current;
	int charging_current;
} CHARGER_AICL_STATUS;

struct charger_notifier_struct {
	charger_notifier_event_t event;
	CHARGER_AICL_STATUS aicl_status;
	struct blocking_notifier_head notifier_call_chain;
};

/* charger notifier register/unregister API
 * for used any where want to receive charger attached device attach/detach. */
extern void charger_notifier_call(struct charger_notifier_struct *value);
extern int charger_notifier_register(struct notifier_block *nb,
		notifier_fn_t notifier, charger_notifier_device_t listener);
extern int charger_notifier_unregister(struct notifier_block *nb);

/* PDIC notifier register/unregister API
 * for used any where want to receive charger attached device attach/detach. */

/* PDIC notifier call sequence,
 * largest priority number device will be called first. */

typedef enum {
	PDIC_NOTIFY_DEV_BATTERY = 0,
	PDIC_NOTIFY_DEV_CHARGER,
	PDIC_NOTIFY_DEV_FUELGAUGE,
	PDIC_NOTIFY_DEV_MULTICHARGER,
} pdic_notifier_device_t;

typedef enum {
	PDIC_NOTIFY_EVENT_DETACH = 0,
	PDIC_NOTIFY_EVENT_CCIC_ATTACH,
	PDIC_NOTIFY_EVENT_PD_SINK,
	PDIC_NOTIFY_EVENT_PD_SOURCE,
	PDIC_NOTIFY_EVENT_PD_SINK_CAP,
	PDIC_NOTIFY_EVENT_PD_PRSWAP_SNKTOSRC,
} pdic_notifier_event_t;

typedef struct _power_list {
#if defined(CONFIG_PDIC_PD30)
	int accept;
	int max_voltage;
	int min_voltage;
	int max_current;
	int apdo;
#else
	int max_voltage;
	int max_current;
#endif
} POWER_LIST;

typedef enum
{
	RP_CURRENT_LEVEL_NONE = 0,
	RP_CURRENT_LEVEL_DEFAULT,
	RP_CURRENT_LEVEL2,
	RP_CURRENT_LEVEL3,
	RP_CURRENT_ABNORMAL,
} RP_CURRENT_LEVEL;

typedef struct _pdic_sink_status {
	POWER_LIST power_list[MAX_PDO_NUM+1];
	int available_pdo_num; // the number of available PDO
	int selected_pdo_num; // selected number of PDO to change
	int current_pdo_num; // current number of PDO
#if defined(CONFIG_PDIC_PD30)
	int pps_voltage;
	int pps_current;
	int request_apdo; // apdo for pps communication
	int has_apdo; // pd source has apdo or not
#endif
	unsigned int rp_currentlvl; // rp current level by ccic
} PDIC_SINK_STATUS;

struct pdic_notifier_struct {
	pdic_notifier_event_t event;
	PDIC_SINK_STATUS sink_status;
	struct blocking_notifier_head notifier_call_chain;
	void *pusbpd;
};

extern void pdic_notifier_call(struct pdic_notifier_struct *value);
extern int pdic_notifier_register(struct notifier_block *nb,
		notifier_fn_t notifier, pdic_notifier_device_t listener);
extern int pdic_notifier_unregister(struct notifier_block *nb);
#endif /* __BATTERY_NOTIFIER_H__ */
