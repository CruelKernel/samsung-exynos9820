/*
 * s2mu004_charger.h - Header of S2MU004 Charger Driver
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef S2MU004_CHARGER_H
#define S2MU004_CHARGER_H
#include <linux/mfd/samsung/s2mu004.h>
#include <linux/mfd/samsung/s2mu004-private.h>

#if defined(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#endif /* CONFIG_MUIC_NOTIFIER */

#include <linux/muic/s2mu004-muic.h>
#include <linux/power/s2mu00x_battery.h>


#define MASK(width, shift)	(((0x1 << (width)) - 1) << shift)

#define S2MU004_CHG_STATUS0		0x0A
#define S2MU004_CHG_STATUS1		0x0B
#define S2MU004_CHG_STATUS2		0x0C
#define S2MU004_CHG_STATUS3		0x0D
#define S2MU004_CHG_STATUS4		0x0E
#define S2MU004_CHG_STATUS5		0x0F
#define S2MU004_CHG_CTRL0		0x10
#define S2MU004_CHG_CTRL1		0x11
#define S2MU004_CHG_CTRL2		0x12
#define S2MU004_CHG_CTRL3		0x13
#define S2MU004_CHG_CTRL4		0x14
#define S2MU004_CHG_CTRL5		0x15
#define S2MU004_CHG_CTRL6		0x16
#define S2MU004_CHG_CTRL7		0x17
#define S2MU004_CHG_CTRL8		0x18
#define S2MU004_CHG_CTRL9		0x19
#define S2MU004_CHG_CTRL10		0x1A
#define S2MU004_CHG_CTRL11		0x1B
#define S2MU004_CHG_CTRL12		0x1C
#define S2MU004_CHG_CTRL13		0x1D
#define S2MU004_CHG_CTRL14		0x1E
#define S2MU004_CHG_CTRL15		0x1F
#define S2MU004_CHG_CTRL16		0x20
#define S2MU004_CHG_CTRL17		0x21
#define S2MU004_CHG_CTRL18		0x22
#define S2MU004_CHG_CTRL19		0x23
#define S2MU004_CHG_CTRL20		0x24

#define S2MU004_PWRSEL_CTRL0	0x72
#define PWRSEL_CTRL0_SHIFT		7
#define PWRSEL_CTRL0_WIDTH		1
#define PWRSEL_CTRL0_MASK	MASK(PWRSEL_CTRL0_WIDTH, PWRSEL_CTRL0_SHIFT)

#define EN_JIG_REG_AP_SHIFT		7
#define EN_JIG_REG_AP_WIDTH		1
#define EN_JIG_REG_AP_MASK	MASK(EN_JIG_REG_AP_WIDTH, EN_JIG_REG_AP_SHIFT)

/* S2MU004_SC_INT_MASK */
#define Poor_CHG_INT_SHIFT	1
#define Poor_CHG_INT_MASK	BIT(Poor_CHG_INT_SHIFT)

/* S2MU004_CHG_STATUS0 */
#define FG_SOC_STATUS_SHIFT	0
#define FG_SOC_STATUS_WIDTH	2
#define FG_SOC_STATUS_MASK	MASK(FG_SOC_STATUS_WIDTH, FG_SOC_STATUS_SHIFT)

#define WCIN_STATUS_SHIFT	2
#define WCIN_STATUS_WIDTH	3
#define WCIN_STATUS_MASK	MASK(WCIN_STATUS_WIDTH, WCIN_STATUS_SHIFT)

#define WCIN_M_SHIFT	6
#define WCIN_M_MASK		BIT(WCIN_M_SHIFT)

#define CHGIN_STATUS_SHIFT	5
#define CHGIN_STATUS_WIDTH	3
#define CHGIN_STATUS_MASK	MASK(CHGIN_STATUS_WIDTH, CHGIN_STATUS_SHIFT)

#define VBUS_OVP_MASK		0xE0
#define VBUS_OVP_SHIFT		5

/* S2MU004_CHG_STATUS1 */
#define SELF_DISCHG_STATUS_SHIFT	7
#define SELF_DISCHG_STATUS_MASK		BIT(SELF_DISCHG_STATUS_SHIFT)

#define CHG_FAULT_STATUS_SHIFT		3
#define CHG_FAULT_STATUS_WIDTH		4
#define CHG_FAULT_STATUS_MASK		MASK(CHG_FAULT_STATUS_WIDTH,\
					 CHG_FAULT_STATUS_SHIFT)

#define CHG_STATUS_PRE_CHARGE	1
#define CHG_STATUS_FAST_CHARGE	2
#define CHG_STATUS_WD_SUSPEND	3
#define CHG_STATUS_WD_RST	4
#define CHG_STATUS_TSD		5
#define CHG_STATUS_TFB		6

#define CHG_Restart_STATUS_SHIFT	2
#define CHG_Restart_STATUS_MASK		BIT(CHG_Restart_STATUS_SHIFT)

#define TOP_OFF_STATUS_SHIFT		1
#define TOP_OFF_STATUS_MASK		BIT(TOP_OFF_STATUS_SHIFT)

#define DONE_STATUS_SHIFT	0
#define DONE_STATUS_MASK	BIT(DONE_STATUS_SHIFT)

/* S2MU004_CHG_STATUS2 */
#define OTG_STATUS_SHIFT	5
#define OTG_STATUS_WIDTH	3
#define OTG_STATUS_MASK		MASK(OTG_STATUS_WIDTH, OTG_STATUS_SHIFT)

#define TX_STATUS_SHIFT		2
#define TX_STATUS_WIDTH		3
#define TX_STATUS_MASK		MASK(TX_STATUS_WIDTH, TX_STATUS_SHIFT)

#define SYS_STATUS_SHIFT	2
#define SYS_STATUS_WIDTH	3
#define SYS_STATUS_MASK		MASK(SYS_STATUS_WIDTH, SYS_STATUS_SHIFT)

/* S2MU004_CHG_STATUS3 */
#define DET_BAT_STATUS_SHIFT	0
#define DET_BAT_STATUS_MASK	BIT(DET_BAT_STATUS_SHIFT)

#define BAT_STATUS_SHIFT	1
#define BAT_STATUS_WIDTH	2
#define BAT_STATUS_MASK		MASK(BAT_STATUS_WIDTH, BAT_STATUS_SHIFT)

#define AICL_STATUS_SHIFT	4
#define AICL_STATUS_WIDTH	2
#define AICL_STATUS_MASK	MASK(AICL_STATUS_WIDTH, AICL_STATUS_SHIFT)

#define ICR_STATUS_SHIFT	6
#define ICR_STATUS_MASK		BIT(ICR_STATUS_SHIFT)

#define IVR_STATUS_SHIFT	7
#define IVR_STATUS_MASK		BIT(IVR_STATUS_SHIFT)

/* S2MU004_CHG_CTRL0 */
#define EN_CHG_SHIFT		7
#define EN_CHG_MASK		BIT(EN_CHG_SHIFT)

#define REG_MODE_SHIFT		0
#define REG_MODE_WIDTH		4
#define REG_MODE_MASK		MASK(REG_MODE_WIDTH, REG_MODE_SHIFT)

#define CHARGER_OFF_MODE	0
#define CHG_MODE		3
#define BUCK_MODE		1
#define OTG_BST_MODE		6

/* S2MU004_CHG_CTRL1 */

/* S2MU004_CHG_CTRL2 */
#define INPUT_CURRENT_LIMIT_SHIFT	0
#define INPUT_CURRENT_LIMIT_WIDTH	7
#define INPUT_CURRENT_LIMIT_MASK	MASK(INPUT_CURRENT_LIMIT_WIDTH,\
					INPUT_CURRENT_LIMIT_SHIFT)

/* S2MU004_CHG_CTRL4 */
#define OTG_OCP_SW_ON_SHIFT		5
#define OTG_OCP_SW_ON_MASK		BIT(OTG_OCP_SW_ON_SHIFT)

#define OTG_OCP_SW_OFF_SHIFT	4
#define OTG_OCP_SW_OFF_MASK		BIT(OTG_OCP_SW_OFF_SHIFT)

#define SET_OTG_OCP_SHIFT	2
#define SET_OTG_OCP_WIDTH	2
#define SET_OTG_OCP_MASK	MASK(SET_OTG_OCP_WIDTH, SET_OTG_OCP_SHIFT)

/* S2MU004_CHG_CTRL5 */
#define SET_CHG_2L_DROP_SHIFT	4
#define SET_CHG_2L_DROP_WIDTH	2
#define SET_CHG_2L_DROP_MASK	MASK(SET_CHG_2L_DROP_WIDTH,\
				SET_CHG_2L_DROP_SHIFT)

#define SET_CHG_3L_DROP_SHIFT	6
#define SET_CHG_3L_DROP_WIDTH	2
#define SET_CHG_3L_DROP_MASK	MASK(SET_CHG_3L_DROP_WIDTH,\
					SET_CHG_3L_DROP_SHIFT)

/* S2MU004_CHG_CTRL6 */
#define SET_VF_VBAT_SHIFT	0
#define SET_VF_VBAT_WIDTH	6
#define SET_VF_VBAT_MASK	MASK(SET_VF_VBAT_WIDTH, SET_VF_VBAT_SHIFT)

/* S2MU004_CHG_CTRL7 */
#define SET_VF_VBYP_SHIFT	5
#define SET_VF_VBYP_WIDTH	2
#define SET_VF_VBYP_MASK	MASK(SET_VF_VBYP_WIDTH, SET_VF_VBYP_SHIFT)
#define SET_VSYS_SHIFT	0
#define SET_VSYS_WIDTH	3
#define SET_VSYS_MASK	MASK(SET_VSYS_WIDTH, SET_VSYS_SHIFT)

/* S2MU004_CHG_CTRL8 */
#define COOL_CHARGING_CURRENT_SHIFT	0
#define COOL_CHARGING_CURRENT_WIDTH	7
#define COOL_CHARGING_CURRENT_MASK	MASK(COOL_CHARGING_CURRENT_WIDTH,\
					COOL_CHARGING_CURRENT_SHIFT)

/* S2MU004_CHG_CTRL9 */
#define FAST_CHARGING_CURRENT_SHIFT	0
#define FAST_CHARGING_CURRENT_WIDTH	7
#define FAST_CHARGING_CURRENT_MASK	MASK(FAST_CHARGING_CURRENT_WIDTH,\
					FAST_CHARGING_CURRENT_SHIFT)

/* S2MU004_CHG_CTRL11 */
#define FIRST_TOPOFF_CURRENT_SHIFT	0
#define FIRST_TOPOFF_CURRENT_WIDTH	4
#define FIRST_TOPOFF_CURRENT_MASK	MASK(FIRST_TOPOFF_CURRENT_WIDTH,\
					FIRST_TOPOFF_CURRENT_SHIFT)

#define SECOND_TOPOFF_CURRENT_SHIFT	4
#define SECOND_TOPOFF_CURRENT_WIDTH	4
#define SECOND_TOPOFF_CURRENT_MASK	MASK(SECOND_TOPOFF_CURRENT_WIDTH,\
					SECOND_TOPOFF_CURRENT_SHIFT)

/* S2MU004_CHG_CTRL12 */
#define SET_OSC_BUCK_SHIFT		0
#define SET_OSC_BUCK_WIDTH		3
#define SET_OSC_BUCK_MASK		MASK(SET_OSC_BUCK_WIDTH,\
					SET_OSC_BUCK_SHIFT)

#define SET_OSC_BUCK_3L_SHIFT		3
#define SET_OSC_BUCK_3L_WIDTH		3
#define SET_OSC_BUCK_3L_MASK		MASK(SET_OSC_BUCK_3L_WIDTH,\
					SET_OSC_BUCK_3L_SHIFT)

enum {
	S2MU004_OSC_BUCK_FRQ_500kHz	= 0x0,
	S2MU004_OSC_BUCK_FRQ_750kHz	= 0x1,
	S2MU004_OSC_BUCK_FRQ_1MHz	= 0x2,
	S2MU004_OSC_BUCK_FRQ_1P25MHz	= 0x3,
	S2MU004_OSC_BUCK_FRQ_1P5MHz	= 0x4,
	S2MU004_OSC_BUCK_FRQ_1P75MHz	= 0x5,
	S2MU004_OSC_BUCK_FRQ_2MHz	= 0x6,
	S2MU004_OSC_BUCK_FRQ_2P25MHz	= 0x7,
};

/* S2MU004_CHG_CTRL13 */
#define SET_IVR_Recovery_SHIFT	5
#define SET_IVR_Recovery_MASK	BIT(SET_IVR_Recovery_SHIFT)

#define SET_EN_WDT_SHIFT 1
#define SET_EN_WDT_MASK BIT(SET_EN_WDT_SHIFT)

#define SET_EN_WDT_AP_RESET_SHIFT 0
#define SET_EN_WDT_AP_RESET_MASK BIT(SET_EN_WDT_AP_RESET_SHIFT)

/* S2MU004_CHG_CTRL14 */
#define WDT_CLR_SHIFT 0
#define WDT_CLR_MASK BIT(WDT_CLR_SHIFT)

/* S2MU004_CHG_CTRL15 */
#define SET_OSC_BST_SHIFT	5
#define SET_OSC_BST_WIDTH	3
#define SET_OSC_BST_MASK	MASK(SET_OSC_BST_WIDTH, SET_OSC_BST_SHIFT)

/* S2MU004_CHG_CTRL16 */
#define SET_TIME_CHG_SHIFT	3
#define SET_TIME_CHG_WIDTH	3
#define SET_TIME_CHG_MASK	MASK(SET_TIME_CHG_WIDTH, SET_TIME_CHG_SHIFT)

/* S2MU004_CHG_CTRL17 */
#define TOP_OFF_TIME_SHIFT    3
#define TOP_OFF_TIME_WIDTH    3
#define TOP_OFF_TIME_MASK    MASK(TOP_OFF_TIME_WIDTH, TOP_OFF_TIME_SHIFT)

#define WDT_TIME_SHIFT        0
#define WDT_TIME_WIDTH        3
#define WDT_TIME_MASK        MASK(WDT_TIME_WIDTH, WDT_TIME_SHIFT)

/* S2MU004_CHG_CTRL18 */
#define CHGIN_ON_SHIFT		2
#define CHGIN_ON_WIDTH		2
#define CHGIN_ON_MASK		MASK(CHGIN_ON_WIDTH, CHGIN_ON_SHIFT)

/* S2MU005_REG_SELFDIS_CFG1 */
#define FC_SELF_DISCHG_SHIFT	3
#define FC_SELF_DISCHG_MASK		BIT(FC_SELF_DISCHG_SHIFT)

/* S2MU004_REG_SELFDIS_CFG3 */
#define SELF_DISCHG_MODE_SHIFT	7
#define SELF_DISCHG_MODE_MASK	BIT(SELF_DISCHG_MODE_SHIFT)

#define FAKE_BAT_LEVEL          50

enum {
	CHIP_ID = 0,
};

ssize_t s2mu004_chg_show_attrs(struct device *dev,
		struct device_attribute *attr, char *buf);

ssize_t s2mu004_chg_store_attrs(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

#define S2MU004_CHARGER_ATTR(_name)				\
{							                    \
	.attr = {.name = #_name, .mode = 0664},	    \
	.show = s2mu004_chg_show_attrs,			    \
	.store = s2mu004_chg_store_attrs,			\
}

enum {
	CHG_REG = 0,
	CHG_DATA,
	CHG_REGS,
};

enum {
	S2MU004_TOPOFF_TIMER_500us	= 0x0,
	S2MU004_TOPOFF_TIMER_5m		= 0x1,
	S2MU004_TOPOFF_TIMER_10m	= 0x2,
	S2MU004_TOPOFF_TIMER_30m	= 0x3,
	S2MU004_TOPOFF_TIMER_50m	= 0x4,
	S2MU004_TOPOFF_TIMER_70m	= 0x5,
	S2MU004_TOPOFF_TIMER_90m	= 0x6,
	S2MU004_TOPOFF_TIMER_DIS	= 0x7,
};

enum {
	S2MU004_WDT_TIMER_40s	= 0x1,
	S2MU004_WDT_TIMER_50s	= 0x2,
	S2MU004_WDT_TIMER_60s	= 0x3,
	S2MU004_WDT_TIMER_70s	= 0x4,
	S2MU004_WDT_TIMER_80s	= 0x5,
	S2MU004_WDT_TIMER_90s	= 0x6,
	S2MU004_WDT_TIMER_100s	= 0x7,
};

enum {
	S2MU004_FC_CHG_TIMER_4hr	= 0x1,
	S2MU004_FC_CHG_TIMER_6hr	= 0x2,
	S2MU004_FC_CHG_TIMER_8hr	= 0x3,
	S2MU004_FC_CHG_TIMER_10hr	= 0x4,
	S2MU004_FC_CHG_TIMER_12hr	= 0x5,
	S2MU004_FC_CHG_TIMER_14hr	= 0x6,
	S2MU004_FC_CHG_TIMER_16hr	= 0x7,
};

enum {
	S2MU004_SET_OTG_OCP_500mA   = 0x0,
	S2MU004_SET_OTG_OCP_900mA   = 0x1,
	S2MU004_SET_OTG_OCP_1200mA  = 0x2,
	S2MU004_SET_OTG_OCP_1500mA  = 0x3,
};

typedef struct s2mu004_charger_platform_data {
	s2mu00x_charging_current_t *charging_current_table;
	s2mu00x_charging_current_t *charging_current;
	int chg_float_voltage;
	char *charger_name;
	char *fuelgauge_name;
	bool chg_eoc_dualpath;
	int recharge_vcell;
	uint32_t is_1MHz_switching:1;
	int chg_switching_freq;
} s2mu004_charger_platform_data_t;


struct s2mu004_charger_data {
	struct i2c_client       *i2c;
	struct device *dev;
	struct s2mu004_platform_data *s2mu004_pdata;
	struct delayed_work charger_work;
	struct delayed_work otg_vbus_work;

	struct workqueue_struct *charger_wqueue;
	struct power_supply *psy_chg;
	struct power_supply_desc psy_chg_desc;
	struct power_supply *psy_otg;
	struct power_supply_desc psy_otg_desc;

	s2mu004_charger_platform_data_t *pdata;
	int dev_id;
	int input_current;
	int charging_current;
	int topoff_current;
	int siop_level;
	int cable_type;
	int battery_cable_type;
	bool is_charging;
	struct mutex charger_mutex;
	bool noti_check;

	/* register programming */
	int reg_addr;
	int reg_data;

	bool full_charged;
	bool ovp;
	bool otg_on;

	int unhealth_cnt;
	bool battery_valid;
	int status;
	int health;

	struct delayed_work get_capacity;
	struct delayed_work afc_current_down;
	struct delayed_work polling_work;

	/* s2mu004 */
	int irq_det_bat;
	int irq_chg;
	int irq_chgin;
	int irq_chg_fault;
	int irq_vbus;
	int irq_rst;
	int irq_done;
	int irq_sys;
	int irq_event;

	int charge_mode;

#if defined(CONFIG_FUELGAUGE_S2MU004)
	int voltage_now;
	int voltage_avg;
	int voltage_ocv;
	unsigned int capacity;
#endif

#if defined(CONFIG_MUIC_NOTIFIER)
	struct notifier_block cable_check;
#endif
};

#endif /*S2MU004_CHARGER_H*/
