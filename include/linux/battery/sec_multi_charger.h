/*
 * sec_multi_charger.h
 * Samsung Mobile Charger Header
 *
 * Copyright (C) 2015 Samsung Electronics, Inc.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __SEC_MULTI_CHARGER_H
#define __SEC_MULTI_CHARGER_H __FILE__

#include <linux/battery/sec_charging_common.h>

#define SEC_SUB_CHARGER_CONDITION_NONE			0x00
#define SEC_SUB_CHARGER_CONDITION_ONLINE		0x01
#define SEC_SUB_CHARGER_CONDITION_CV			0x02
#define SEC_SUB_CHARGER_CONDITION_CURRENT_MAX	0x10
#define SEC_SUB_CHARGER_CONDITION_CURRENT_NOW	0x20

struct charger_temp_control {
	unsigned int temp;
	unsigned int level;
	unsigned int threshold;
	unsigned int step;
	unsigned int drop_level;
};

struct sec_multi_charger_platform_data {
	char *battery_name;
	char *main_charger_name;
	char *sub_charger_name;

	bool is_serial;
	bool aicl_disable;

	unsigned int sub_charger_condition;
	int sub_charger_condition_current_max;
	int sub_charger_condition_current_now;
	int sub_charger_enable_current_now;
	unsigned int *sub_charger_condition_online;
	unsigned int sub_charger_condition_online_size;

	struct charger_temp_control main_charger_temp;
	struct charger_temp_control sub_charger_temp;

	unsigned int main_charger_current_level;
	unsigned int sub_charger_current_level;

	unsigned int sub_charger_current_max;
};

struct sec_multi_charger_info {
	struct device *dev;
	struct sec_multi_charger_platform_data *pdata;
	struct power_supply		psy_chg;

	int cable_type;
	int siop_level;
	int status;

	/* sub_charger should be disabled before 1st EOC */
	bool sub_is_charging;

	sec_charging_current_t total_current;
	sec_charging_current_t main_current;
	sec_charging_current_t sub_current;

	int chg_mode;
#if 0
	int cable_type;
	int status;
	bool is_charging;

	/* charging current : + charging, - OTG */
	int charging_current;
	unsigned charging_current_max;

	/* register programming */
	int reg_addr;
	int reg_data;
	int irq_base;
#endif
};

#endif /* __SEC_MULTI_CHARGER_H */
