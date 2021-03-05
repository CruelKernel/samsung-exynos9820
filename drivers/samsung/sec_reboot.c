/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/io.h>
#include <linux/gpio.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/input.h>
#endif
#include <linux/sec_ext.h>
#include "../battery_v2/include/sec_battery.h"
#include <linux/sec_batt.h>

#include <asm/cacheflush.h>
#include <asm/system_misc.h>

#include <soc/samsung/exynos-pmu.h>
#include <soc/samsung/acpm_ipc_ctrl.h>
#include <soc/samsung/exynos-sci.h>
#include <linux/sec_debug.h>

#if defined(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif

/* function ptr for original arm_pm_restart */
void (*mach_restart)(enum reboot_mode mode, const char *cmd);
EXPORT_SYMBOL(mach_restart);

/* MINFORM */
#define SEC_REBOOT_START_OFFSET		(24)
#define SEC_REBOOT_END_OFFSET		(16)

enum sec_power_flags {
	SEC_REBOOT_DEFAULT = 0x30,
	SEC_REBOOT_NORMAL = 0x4E,
	SEC_REBOOT_LPM = 0x70,
};

#define SEC_DUMPSINK_MASK 0x0000FFFF

/* PANIC INFORM */
#define SEC_RESET_REASON_PREFIX 0x12345670
#define SEC_RESET_SET_PREFIX    0xabc00000
enum sec_reset_reason {
	SEC_RESET_REASON_UNKNOWN   = (SEC_RESET_REASON_PREFIX | 0x0),
	SEC_RESET_REASON_DOWNLOAD  = (SEC_RESET_REASON_PREFIX | 0x1),
	SEC_RESET_REASON_UPLOAD    = (SEC_RESET_REASON_PREFIX | 0x2),
	SEC_RESET_REASON_CHARGING  = (SEC_RESET_REASON_PREFIX | 0x3),
	SEC_RESET_REASON_RECOVERY  = (SEC_RESET_REASON_PREFIX | 0x4),
	SEC_RESET_REASON_FOTA      = (SEC_RESET_REASON_PREFIX | 0x5),
	SEC_RESET_REASON_FOTA_BL   = (SEC_RESET_REASON_PREFIX | 0x6), /* update bootloader */
	SEC_RESET_REASON_SECURE    = (SEC_RESET_REASON_PREFIX | 0x7), /* image secure check fail */
	SEC_RESET_REASON_FWUP      = (SEC_RESET_REASON_PREFIX | 0x9), /* emergency firmware update */
	SEC_RESET_REASON_EM_FUSE   = (SEC_RESET_REASON_PREFIX | 0xa), /* EMC market fuse */
	SEC_RESET_REASON_BOOTLOADER   = (SEC_RESET_REASON_PREFIX | 0xd), /* go to download mode */
	SEC_RESET_REASON_EMERGENCY = 0x0,

	SEC_RESET_SET_FORCE_UPLOAD = (SEC_RESET_SET_PREFIX | 0x40000),
	SEC_RESET_SET_DEBUG        = (SEC_RESET_SET_PREFIX | 0xd0000),
	SEC_RESET_SET_SWSEL        = (SEC_RESET_SET_PREFIX | 0xe0000),
	SEC_RESET_SET_SUD          = (SEC_RESET_SET_PREFIX | 0xf0000),
	SEC_RESET_CP_DBGMEM        = (SEC_RESET_SET_PREFIX | 0x50000), /* cpmem_on: CP RAM logging */
#if defined(CONFIG_SEC_ABC)
	SEC_RESET_USER_DRAM_TEST   = (SEC_RESET_SET_PREFIX | 0x60000), /* USER DRAM TEST */
#endif
#if defined(CONFIG_SEC_SYSUP)
	SEC_RESET_SET_PARAM   = (SEC_RESET_SET_PREFIX | 0x70000),
#endif
	SEC_RESET_SET_DUMPSINK	   = (SEC_RESET_SET_PREFIX | 0x80000),
};

void sec_set_reboot_magic(int magic, int offset, int mask)
{
	u32 tmp = 0;

	exynos_pmu_read(SEC_DEBUG_MAGIC_INFORM, &tmp);
	pr_info("%s: prev: %x\n", __func__, tmp);
	mask <<= offset;
	tmp &= (~mask);
	tmp |= magic << offset;
	pr_info("%s: set as: %x\n", __func__, tmp);
	exynos_pmu_write(SEC_DEBUG_MAGIC_INFORM, tmp);
}

static void sec_power_off(void)
{
	int poweroff_try = 0;
	union power_supply_propval ac_val, usb_val, wpc_val, water_val;
	int powerkey_gpio = -1;
	struct device_node *np, *pp;

	np = of_find_node_by_path("/gpio_keys");
	if (!np)
		return;
	for_each_child_of_node(np, pp) {
		uint keycode = 0;
		if (!of_find_property(pp, "gpios", NULL))
			continue;
		of_property_read_u32(pp, "linux,code", &keycode);
		if (keycode == KEY_POWER) {
			pr_info("%s: <%u>\n", __func__,  keycode);
			powerkey_gpio = of_get_gpio(pp, 0);
			break;
		}
	}
	of_node_put(np);

	if (!gpio_is_valid(powerkey_gpio)) {
		pr_err("Couldn't find power key node\n");
		return;
	}

	local_irq_disable();

	sec_set_reboot_magic(SEC_REBOOT_LPM, SEC_REBOOT_END_OFFSET, 0xFF);
	psy_do_property("ac", get, POWER_SUPPLY_PROP_ONLINE, ac_val);
	psy_do_property("ac", get, POWER_SUPPLY_EXT_PROP_WATER_DETECT, water_val);
	psy_do_property("usb", get, POWER_SUPPLY_PROP_ONLINE, usb_val);
	psy_do_property("wireless", get, POWER_SUPPLY_PROP_ONLINE, wpc_val);
	pr_info("[%s] AC[%d], USB[%d], WPC[%d], WATER[%d]\n",
			__func__, ac_val.intval, usb_val.intval, wpc_val.intval, water_val.intval);

	sec_debug_clear_magic_rambase();

	flush_cache_all();
	llc_flush();

	while (1) {
		/* Check reboot charging */
#ifdef CONFIG_SAMSUNG_BATTERY
		if ((ac_val.intval || water_val.intval || usb_val.intval || wpc_val.intval || (poweroff_try >= 5)) && !lpcharge) {
#else
		if ((ac_val.intval || water_val.intval || usb_val.intval || wpc_val.intval || (poweroff_try >= 5))) {
#endif
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_UNKNOWN);
			pr_emerg("%s: charger connected or power off failed(%d), reboot!\n", __func__, poweroff_try);
			/* To enter LP charging */

			mach_restart(REBOOT_SOFT, "sw reset");

			pr_emerg("%s: waiting for reboot\n", __func__);
			while (1)
				;
		}

		/* wait for power button release */
		if (gpio_get_value(powerkey_gpio)) {
			exynos_acpm_reboot();

			pr_emerg("%s: set PS_HOLD low\n", __func__);
			exynos_pmu_update(EXYNOS_PMU_PS_HOLD_CONTROL, 0x1<<8, 0x0);

			++poweroff_try;
			pr_emerg
				("%s: Should not reach here! (poweroff_try:%d)\n",
				 __func__, poweroff_try);
		} else {
		/* if power button is not released, wait and check TA again */
			pr_info("%s: PowerButton is not released.\n", __func__);
		}
		mdelay(1000);
	}

}

static void sec_reboot(enum reboot_mode reboot_mode, const char *cmd)
{
	local_irq_disable();

	pr_emerg("%s (%d, %s)\n", __func__, reboot_mode, cmd ? cmd : "(null)");

	sec_debug_clear_magic_rambase();

	/* LPM mode prevention */
	sec_set_reboot_magic(SEC_REBOOT_NORMAL, SEC_REBOOT_END_OFFSET, 0xFF);

	if (cmd) {
		unsigned long value;
		if (!strcmp(cmd, "fota"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_FOTA);
		else if (!strcmp(cmd, "fota_bl"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_FOTA_BL);
		else if (!strncmp(cmd, "recovery", 8))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_RECOVERY);
		else if (!strcmp(cmd, "download"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_DOWNLOAD);
		else if (!strcmp(cmd, "bootloader"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_BOOTLOADER);
		else if (!strcmp(cmd, "upload"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_UPLOAD);
		else if (!strcmp(cmd, "secure"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_SECURE);
		else if (!strcmp(cmd, "fwup"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_FWUP);
		else if (!strcmp(cmd, "em_mode_force_user"))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_EM_FUSE);
#if defined(CONFIG_SEC_ABC)
		else if (!strcmp(cmd, "user_dram_test") && sec_abc_get_enabled())
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_USER_DRAM_TEST);
#endif
		else if (!strncmp(cmd, "emergency", 9))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_EMERGENCY);
		else if (!strncmp(cmd, "debug", 5) && !kstrtoul(cmd + 5, 0, &value))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_SET_DEBUG | value);
		else if (!strncmp(cmd, "dump_sink", 9) && !kstrtoul(cmd + 9, 0, &value))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_SET_DUMPSINK | (SEC_DUMPSINK_MASK & value));
		else if (!strncmp(cmd, "forceupload", 11) && !kstrtoul(cmd + 11, 0, &value))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_SET_FORCE_UPLOAD | value);
		else if (!strncmp(cmd, "swsel", 5) && !kstrtoul(cmd + 5, 0, &value))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_SET_SWSEL | value);
		else if (!strncmp(cmd, "sud", 3) && !kstrtoul(cmd + 3, 0, &value))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_SET_SUD | value);
#if defined(CONFIG_SEC_SYSUP)
		else if (!strncmp(cmd, "param", 5))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_SET_PARAM);
#endif
		else if (!strncmp(cmd, "cpmem_on", 8))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_CP_DBGMEM | 0x1);
		else if (!strncmp(cmd, "cpmem_off", 9))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_CP_DBGMEM | 0x2);
		else if (!strncmp(cmd, "mbsmem_on", 9))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_CP_DBGMEM | 0x1);
		else if (!strncmp(cmd, "mbsmem_off", 10))
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_CP_DBGMEM | 0x2);
		else if (!strncmp(cmd, "panic", 5)) {
			/*
			 * This line is intentionally blanked because the PANIC INFORM is used for upload cause
			 * in sec_debug_set_upload_cause() only in case of  panic() .
			 */
		} else
			exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_UNKNOWN);
	} else {
		exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, SEC_RESET_REASON_UNKNOWN);
	}

	flush_cache_all();
	llc_flush();

	mach_restart(REBOOT_SOFT, "sw reset");

	pr_emerg("%s: waiting for reboot\n", __func__);
	while (1)
		;
}

static int __init sec_reboot_init(void)
{
	mach_restart = arm_pm_restart;
	pm_power_off = sec_power_off;
	arm_pm_restart = sec_reboot;
	return 0;
}

subsys_initcall(sec_reboot_init);
