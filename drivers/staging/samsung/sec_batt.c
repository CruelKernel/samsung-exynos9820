/*
 * Copyright (C) 2012 Samsung Electronics, Inc.
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
#include <linux/sec_batt.h>

#if defined(CONFIG_BATTERY_SAMSUNG)

unsigned int lpcharge;
EXPORT_SYMBOL(lpcharge);

int charging_night_mode;
EXPORT_SYMBOL(charging_night_mode);

static int sec_bat_is_lpm_check(char *str)
{
	if (strncmp(str, "charger", 7) == 0)
		lpcharge = 1;

	pr_info("%s: Low power charging mode: %d\n", __func__, lpcharge);

	return lpcharge;
}
__setup("androidboot.mode=", sec_bat_is_lpm_check);

static int __init charging_mode(char *str)
{
	int mode;

	/*
	 * Only update loglevel value when a correct setting was passed,
	 * to prevent blind crashes (when loglevel being set to 0) that
	 * are quite hard to debug
	 */
	if (get_option(&str, &mode)) {
		charging_night_mode = mode & 0x000000FF;

		printk(KERN_ERR "charging_mode() : 0x%x(%d)\n", charging_night_mode, charging_night_mode);

		return 0;
	}

	printk(KERN_ERR "charging_mode() : %d\n", -EINVAL);

	return -EINVAL;
}
early_param("charging_mode", charging_mode);

int fg_reset;
EXPORT_SYMBOL(fg_reset);

static int sec_bat_get_fg_reset(char *val)
{
	fg_reset = strncmp(val, "1", 1) ? 0 : 1;
	pr_info("%s, fg_reset:%d\n", __func__, fg_reset);
	return 1;
}
__setup("fg_reset=", sec_bat_get_fg_reset);

#if defined(CONFIG_SEC_FACTORY)
int factory_mode;
EXPORT_SYMBOL(factory_mode);

static int sec_bat_get_factory_mode(char *val)
{
	factory_mode = strncmp(val, "1", 1) ? 0 : 1;
	pr_info("%s, factory_mode : %d\n", __func__, factory_mode);
	return 1;
}
__setup("factory_mode=", sec_bat_get_factory_mode);
#endif
#endif
