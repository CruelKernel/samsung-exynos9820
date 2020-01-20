/*
 * max77833_fuelgauge.h
 * Samsung MAX77833 Fuel Gauge Header
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
 *
 * This software is 77833 under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MAX77833_FUELGAUGE_H
#define __MAX77833_FUELGAUGE_H __FILE__

#if defined(ANDROID_ALARM_ACTIVATED)
#include <linux/android_alarm.h>
#endif

#include <linux/battery/sec_charging_common.h>

#include <linux/mfd/core.h>
#include <linux/mfd/max77833.h>
#include <linux/mfd/max77833-private.h>
#include <linux/regulator/machine.h>
#include <linux/wakelock.h>

/* Slave address should be shifted to the right 1bit.
 * R/W bit should NOT be included.
 */

#define PRINT_COUNT	10

#define LOW_BATT_COMP_RANGE_NUM	5
#define LOW_BATT_COMP_LEVEL_NUM	2
#define MAX_LOW_BATT_CHECK_CNT	10

#define ALERT_EN 0x04


struct sec_fuelgauge_reg_data {
	u8 reg_addr;
	u8 reg_data1;
	u8 reg_data2;
};

struct max77833_fg_info {
	/* test print count */
	int pr_cnt;
	/* full charge comp */
	struct delayed_work	full_comp_work;
	u32 previous_fullcap;
	u32 previous_vffullcap;
	/* low battery comp */
	int low_batt_comp_cnt[LOW_BATT_COMP_RANGE_NUM][LOW_BATT_COMP_LEVEL_NUM];
	int low_batt_comp_flag;
	/* low battery boot */
	int low_batt_boot_flag;
	bool is_low_batt_alarm;

	/* battery info */
	u32 soc;

	/* miscellaneous */
	unsigned long fullcap_check_interval;
	int full_check_flag;
	bool is_first_check;
};


enum {
	MAX77833_FG_LEVEL = 0,
	MAX77833_FG_TEMPERATURE,
	MAX77833_FG_VOLTAGE,
	MAX77833_FG_CURRENT,
	MAX77833_FG_CURRENT_AVG,
	MAX77833_FG_CHECK_STATUS,
	MAX77833_FG_RAW_SOC,
	MAX77833_FG_VF_SOC,
	MAX77833_FG_AV_SOC,
	MAX77833_FG_FULLCAP,
	MAX77833_FG_FULLCAPNOM,
	MAX77833_FG_FULLCAPREP,
	MAX77833_FG_MIXCAP,
	MAX77833_FG_AVCAP,
	MAX77833_FG_REPCAP,
	MAX77833_FG_CYCLE,
};

enum {
	MAX77833_POSITIVE = 0,
	MAX77833_NEGATIVE,
};

enum {
	MAX77833_RANGE = 0,
	MAX77833_SLOPE,
	MAX77833_OFFSET,
	MAX77833_TABLE_MAX
};

#define CURRENT_RANGE_MAX_NUM	5

struct battery_data_t {
	u32 QResidual20;
	u32 QResidual30;
	u32 Capacity;
	u32 low_battery_comp_voltage;
	s32 low_battery_table[CURRENT_RANGE_MAX_NUM][MAX77833_TABLE_MAX];
	u8	*type_str;
	u32 ichgterm;
	u32 misccfg;
	u32 fullsocthr;
	u32 ichgterm_2nd;
	u32 misccfg_2nd;
	u32 fullsocthr_2nd;
};


/* FullCap learning setting */
#define VFFULLCAP_CHECK_INTERVAL	300 /* sec */
/* soc should be 0.1% unit */
#define VFSOC_FOR_FULLCAP_LEARNING	950
#define LOW_CURRENT_FOR_FULLCAP_LEARNING	20
#define HIGH_CURRENT_FOR_FULLCAP_LEARNING	120
#define LOW_AVGCURRENT_FOR_FULLCAP_LEARNING	20
#define HIGH_AVGCURRENT_FOR_FULLCAP_LEARNING	100

/* power off margin */
/* soc should be 0.1% unit */
#define POWER_OFF_SOC_HIGH_MARGIN	20
#define POWER_OFF_VOLTAGE_HIGH_MARGIN	3500
#define POWER_OFF_VOLTAGE_LOW_MARGIN	3400

/* FG recovery handler */
/* soc should be 0.1% unit */
#define STABLE_LOW_BATTERY_DIFF	30
#define STABLE_LOW_BATTERY_DIFF_LOWBATT	10
#define LOW_BATTERY_SOC_REDUCE_UNIT	10

struct cv_slope{
	int fg_current;
	int soc;
	int time;
};
struct max77833_fuelgauge_data {
	struct device           *dev;
	struct i2c_client       *i2c;
	struct i2c_client       *pmic;
	struct mutex            fuelgauge_mutex;
	struct max77833_platform_data *max77833_pdata;
	sec_fuelgauge_platform_data_t *pdata;
	struct power_supply		psy_fg;
	struct delayed_work isr_work;

	int cable_type;
	bool is_charging;

	/* HW-dedicated fuel guage info structure
	 * used in individual fuel gauge file only
	 * (ex. dummy_fuelgauge.c)
	 */
	struct max77833_fg_info	info;
	struct battery_data_t        *battery_data;

	bool is_fuel_alerted;
	struct wake_lock fuel_alert_wake_lock;

	unsigned int capacity_old;	/* only for atomic calculation */
	unsigned int capacity_max;	/* only for dynamic calculation */
	unsigned int standard_capacity;

	bool initial_update_of_soc;
	struct mutex fg_lock;

	/* register programming */
	int reg_addr;
	u8 reg_data[2];

	unsigned int pre_soc;
	int fg_irq;

	int raw_capacity;
	int current_now;
	int current_avg;
	struct cv_slope *cv_data;
	int cv_data_lenth;
};

#endif /* __MAX77833_FUELGAUGE_H */
