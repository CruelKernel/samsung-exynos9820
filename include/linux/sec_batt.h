/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
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

#ifndef __SEC_BATT_H
#define __SEC_BATT_H

#include <linux/module.h>
#include <linux/sec_debug.h>

extern unsigned int lpcharge;
extern int charging_night_mode;
extern int fg_reset;
#if defined(CONFIG_SEC_FACTORY)
extern int factory_mode;
#endif

void exynos5_universal5430_pmic_init(void);
void exynos5_universal5430_power_init(void);
void exynos5_universal5430_clock_init(void);
void exynos5_universal5430_mmc_init(void);
void exynos5_universal5430_usb_init(void);
int exynos5_universal5430_battery_init(void);
void exynos5_universal5430_audio_init(void);
void exynos5_universal5430_input_init(void);
void exynos5_universal5430_display_init(void);
void exynos5_universal5430_media_init(void);
void exynos5_universal5430_mfd_init(void);
void exynos5_universal5430_sensor_init(void);
void exynos5_universal5430_nfc_init(void);
void exynos5_universal5430_vibrator_init(void);
void exynos5_universal5430_led_init(void);
#ifdef CONFIG_SAMSUNG_MHL_8240
void exynos5_universal5410_mhl_init(void);
#endif
void exynos5_universal5430_gpio_init(void);
void exynos5_universal5430_fpga_init(void);
void exynos5_universal5430_thermistor_init(void);
#ifdef CONFIG_W1
void exynos5_universal5430_cover_id_init(void);
#endif
#endif
