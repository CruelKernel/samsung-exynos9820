/*
 * s2mu00x_battery.h
 *
 * Copyright (C) 2017 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __S2MU00X_BATTERY_H
#define __S2MU00X_BATTERY_H

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/power_supply.h>

enum s2mu00x_battery_voltage_mode {
	S2MU00X_BATTERY_VOLTAGE_AVERAGE = 0,
	S2MU00X_BATTERY_VOLTAGE_OCV,
};

enum s2mu00x_battery_current_mode {
	S2MU00X_BATTERY_CURRENT_UA = 0,
	S2MU00X_BATTERY_CURRENT_MA,
};

enum s2mu00x_battery_charger_mode {
	S2MU00X_BAT_CHG_MODE_CHARGING = 0,
	S2MU00X_BAT_CHG_MODE_CHARGING_OFF,
	S2MU00X_BAT_CHG_MODE_BUCK_OFF,
};

struct s2mu00x_charging_current {
#ifdef CONFIG_OF
	unsigned int input_current_limit;
	unsigned int fast_charging_current;
	unsigned int full_check_current;
#else
	int input_current_limit;
	int fast_charging_current;
	int full_check_current;
#endif
};

#define s2mu00x_charging_current_t struct s2mu00x_charging_current

#define S2MU00X_FUELGAUGE_CAPACITY_TYPE_RESET   (-1)

#endif /* __S2MU00X_BATTERY_H */
