/*
 * max77804_charger.h
 * Samsung max77804 Charger Header
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
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
#ifndef __MAX77804_CHARGER_H
#define __MAX77804_CHARGER_H __FILE__
#include <linux/mfd/core.h>
#include <linux/mfd/max77804.h>
#include <linux/mfd/max77804-private.h>
#include <linux/regulator/machine.h>

#if defined(CONFIG_V1A) || defined(CONFIG_N1A)
#define max77888_charger
#else
#undef max77888_charger
#endif

#define ENABLE 1
#define DISABLE 0

/* macro */
#define MAX(x, y)	((x) > (y) ? (x) : (y))
#define MIN(x, y)	((x) < (y) ? (x) : (y))
#define ABS(x)		((x) < 0 ? (-1 * (x)) : (x))
/*
 * Use for battery
 */
#define OFF_CURR	0	/* charger off current */
#define KEEP_CURR	-1	/* keep previous current */

/* MAX77803 CHG_CNFG_04 register */
#define CHG_CNFG_04_CHG_CV_PRM_SHIFT            0
#define CHG_CNFG_04_CHG_CV_PRM_MASK             (0x1f << CHG_CNFG_04_CHG_CV_PRM_SHIFT)

/* MAX77803 CHG_CNFG_12 register */
#define CHG_CNFG_12_CHGINSEL_SHIFT              5
#define CHG_CNFG_12_CHGINSEL_MASK               (0x1 << CHG_CNFG_12_CHGINSEL_SHIFT)

/* MAX77804_CHG_REG_CHG_INT */
#define MAX77804_BYP_I                  (1 << 0)
#define MAX77804_BATP_I			(1 << 2)
#define MAX77804_BAT_I                  (1 << 3)
#define MAX77804_CHG_I                  (1 << 4)
#define MAX77804_WCIN_I			(1 << 5)
#define MAX77804_CHGIN_I                (1 << 6)

/* MAX77804_CHG_REG_CHG_INT_MASK */
#define MAX77804_BYP_IM                 (1 << 0)
#define MAX77804_THM_IM                 (1 << 2)
#define MAX77804_BAT_IM                 (1 << 3)
#define MAX77804_CHG_IM                 (1 << 4)
#define MAX77804_WCIN_IM		(1 << 5)
#define MAX77804_CHGIN_IM               (1 << 6)

/* MAX77804_CHG_REG_CHG_INT_OK */
#define MAX77804_BYP_OK                 0x01
#define MAX77804_BYP_OK_SHIFT           0
#define MAX77804_BATP_OK		0x04
#define MAX77804_BATP_OK_SHIFT		2
#define MAX77804_BAT_OK                 0x08
#define MAX77804_BAT_OK_SHIFT           3
#define MAX77804_CHG_OK                 0x10
#define MAX77804_CHG_OK_SHIFT           4
#define MAX77804_WCIN_OK		0x20
#define MAX77804_WCIN_OK_SHIFT		5
#define MAX77804_CHGIN_OK               0x40
#define MAX77804_CHGIN_OK_SHIFT         6
#define MAX77804_DETBAT                 0x04
#define MAX77804_DETBAT_SHIFT           2

/* MAX77804_CHG_REG_CHG_DTLS_00 */
#define MAX77804_BATP_DTLS		0x01
#define MAX77804_BATP_DTLS_SHIFT	0
#define MAX77804_WCIN_DTLS		0x18
#define MAX77804_WCIN_DTLS_SHIFT	3
#define MAX77804_CHGIN_DTLS             0x60
#define MAX77804_CHGIN_DTLS_SHIFT       5

/* MAX77804_CHG_REG_CHG_DTLS_01 */
#define MAX77804_CHG_DTLS               0x0F
#define MAX77804_CHG_DTLS_SHIFT         0
#define MAX77804_BAT_DTLS               0x70
#define MAX77804_BAT_DTLS_SHIFT         4

/* MAX77804_CHG_REG_CHG_DTLS_02 */
#define MAX77804_BYP_DTLS               0x0F
#define MAX77804_BYP_DTLS_SHIFT         0
#define MAX77804_BYP_DTLS0      0x1
#define MAX77804_BYP_DTLS1      0x2
#define MAX77804_BYP_DTLS2      0x4
#define MAX77804_BYP_DTLS3      0x8

/* MAX77804_CHG_REG_CHG_CNFG_00 */
#define MAX77804_MODE_DEFAULT   0x04
#define MAX77804_MODE_CHGR      0x01
#define MAX77804_MODE_OTG       0x02
#define MAX77804_MODE_BUCK      0x04
#define MAX77804_MODE_BOOST		0x08

/* MAX77804_CHG_REG_CHG_CNFG_02 */
#define MAX77804_CHG_CC         0x3F

/* MAX77804_CHG_REG_CHG_CNFG_03 */
#define MAX77804_CHG_TO_ITH		0x07

/* MAX77804_CHG_REG_CHG_CNFG_04 */
#define MAX77804_CHG_MINVSYS_MASK       0xE0
#define MAX77804_CHG_MINVSYS_SHIFT      5
#define MAX77804_CHG_PRM_MASK           0x1F
#define MAX77804_CHG_PRM_SHIFT          0

/* MAX77804_CHG_REG_CHG_CNFG_09 */
#define MAX77804_CHG_CHGIN_LIM  0x7F

/* MAX77804_CHG_REG_CHG_CNFG_12 */
#define MAX77804_CHG_WCINSEL		0x40

/* MAX77804_MUIC_REG_CDETCTRL1 */
#define MAX77804_CHGTYPMAN              0x02
#define MAX77804_CHGTYPMAN_SHIFT        1

/* MAX77804_MUIC_REG_STATUS2 */
#define MAX77804_VBVOLT                 0x40
#define MAX77804_VBVOLT_SHIFT           6
#define MAX77804_CHGDETRUN              0x08
#define MAX77804_CHGDETRUN_SHIFT        3
#define MAX77804_CHGTYPE                0x07
#define MAX77804_CHGTYPE_SHIFT          0

/* irq */
#define IRQ_DEBOUNCE_TIME       20      /* msec */

/* charger type detection */
#define DET_ERR_RETRY   5
#define DET_ERR_DELAY   200

/* soft charging */
#define SOFT_CHG_START_CURR     100     /* mA */
#define SOFT_CHG_START_DUR      100     /* ms */
#define SOFT_CHG_CURR_STEP      100     /* mA */
#define SOFT_CHG_STEP_DUR       20      /* ms */

#define DEFAULT_AC_CURRENT	1600	/* mA */
#define DEFAULT_USB_CURRENT	500	/* mA */

enum {
	POWER_SUPPLY_VBUS_UNKNOWN = 0,
	POWER_SUPPLY_VBUS_UVLO,
	POWER_SUPPLY_VBUS_WEAK,
	POWER_SUPPLY_VBUS_OVLO,
	POWER_SUPPLY_VBUS_GOOD,
};

extern sec_battery_platform_data_t sec_battery_pdata;

#if defined(CONFIG_TARGET_LOCALE_KOR)
#define SEC_CHG_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = sec_chg_show_attrs,			\
	.store = sec_chg_store_attrs,			\
}

enum {
	CHG_SIOP_INPUT_LIMIT = 0,
};
#endif

#endif
