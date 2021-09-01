/*
 * max77705_fuelgauge.h
 * Samsung max77705 Fuel Gauge Header
 *
 * Copyright (C) 2015 Samsung Electronics, Inc.
 *
 * This software is 77854 under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MAX77705_FUELGAUGE_H
#define __MAX77705_FUELGAUGE_H __FILE__

#include <linux/mfd/core.h>
#include <linux/mfd/max77705.h>
#include <linux/mfd/max77705-private.h>
#include <linux/regulator/machine.h>
#include "../sec_charging_common.h"

#include <linux/sec_batt.h>

/* Slave address should be shifted to the right 1bit.
 * R/W bit should NOT be included.
 */

#define PRINT_COUNT	10

#define ALERT_EN 0x04
#define CAPACITY_SCALE_DEFAULT_CURRENT 1000
#define CAPACITY_SCALE_HV_CURRENT 600

enum max77705_vempty_mode {
	VEMPTY_MODE_HW = 0,
	VEMPTY_MODE_SW,
	VEMPTY_MODE_SW_VALERT,
	VEMPTY_MODE_SW_RECOVERY,
};

enum {
	FG_DATA,
};

ssize_t max77705_fg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t max77705_fg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define MAX77705_FG_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0660},	\
	.show = max77705_fg_show_attrs,			\
	.store = max77705_fg_store_attrs,			\
}

struct sec_fg_info {
	/* test print count */
	int pr_cnt;
	/* full charge comp */
	struct delayed_work	full_comp_work;

	/* battery info */
	u32 soc;

	/* miscellaneous */
	unsigned long fullcap_check_interval;
	int full_check_flag;
	bool is_first_check;
};

enum {
	FG_LEVEL = 0,
	FG_TEMPERATURE,
	FG_VOLTAGE,
	FG_CURRENT,
	FG_CURRENT_AVG,
	FG_CHECK_STATUS,
	FG_RAW_SOC,
	FG_VF_SOC,
	FG_AV_SOC,
	FG_FULLCAP,
	FG_FULLCAPNOM,
	FG_FULLCAPREP,
	FG_MIXCAP,
	FG_AVCAP,
	FG_REPCAP,
	FG_CYCLE,
	FG_QH,
	FG_QH_VF_SOC,
	FG_ISYS,
	FG_ISYS_AVG,
	FG_VSYS,
	FG_IIN,
	FG_VBYP,
};

enum {
	POSITIVE = 0,
	NEGATIVE,
};

enum {
	RANGE = 0,
	SLOPE,
	OFFSET,
	TABLE_MAX
};

#define CURRENT_RANGE_MAX_NUM	5

struct battery_data_t {
	u32 V_empty;
	u32 V_empty_origin;
	u32 sw_v_empty_vol;
	u32 sw_v_empty_vol_cisd;
	u32 sw_v_empty_recover_vol;
	u32 QResidual20;
	u32 QResidual30;
	u32 Capacity;
	u8  *type_str;
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

#define LEARNING_QRTABLE 0x0001

struct cv_slope {
	int fg_current;
	int soc;
	int time;
};

#define FG_RESET_DATA_COUNT		5

struct fg_reset_wa {
	u32 fullcapnom;
	u32 dPacc;
	u32 dQacc;
	u32 rcomp0;
	u32 tempco;
};

struct lost_soc_data {
	/* dt data */
	int trig_soc; /* default 10% */
	int trig_d_soc; /* delta soc, default 2% */
	int trig_scale; /* default 2x */
	int guarantee_soc; /* default 2% */
	int min_vol; /* default 3200mV */

	/* data */
	bool ing;
	int prev_raw_soc;
	int prev_remcap;
	int prev_qh;
	int lost_cap;
	int weight;
};

struct max77705_fuelgauge_data {
	struct device           *dev;
	struct i2c_client       *i2c;
	struct i2c_client       *pmic;
	struct mutex            fuelgauge_mutex;
	struct max77705_platform_data *max77705_pdata;
	sec_fuelgauge_platform_data_t *pdata;
	struct power_supply	      *psy_fg;
	struct delayed_work isr_work;

	int cable_type;
	bool is_charging;

	/* HW-dedicated fuel gauge info structure
	 * used in individual fuel gauge file only
	 * (ex. dummy_fuelgauge.c)
	 */
	struct sec_fg_info	info;
	struct battery_data_t        *battery_data;

	bool is_fuel_alerted;
	struct wake_lock fuel_alert_wake_lock;

	unsigned int capacity_old;	/* only for atomic calculation */
	unsigned int capacity_max;	/* only for dynamic calculation */
	unsigned int g_capacity_max;	/* only for dynamic calculation */
	unsigned int standard_capacity;

	bool capacity_max_conv;
	bool initial_update_of_soc;
	bool sleep_initial_update_of_soc;	
	struct mutex fg_lock;

	/* register programming */
	int reg_addr;
	u8 reg_data[2];

	unsigned int pre_soc;
	int fg_irq;

	int raw_capacity;
	int current_now;
	int current_avg;
	unsigned int ttf_capacity;
	struct cv_slope *cv_data;
	int cv_data_length;

	bool using_temp_compensation;
	bool using_hw_vempty;
	unsigned int vempty_mode;
	int temperature;
	bool vempty_init_flag;

	int low_temp_limit;

	bool auto_discharge_en;
	u32 discharge_temp_threshold;
	u32 discharge_volt_threshold;

	u32 fg_resistor;

	struct fg_reset_wa *fg_reset_data;

#if defined(CONFIG_BATTERY_CISD)
	bool valert_count_flag;
#endif
	struct lost_soc_data lost_soc;
};

#endif /* __MAX77705_FUELGAUGE_H */
