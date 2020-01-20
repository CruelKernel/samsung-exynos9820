/*
 * sec_battery.h
 * Samsung Mobile Battery Header
 *
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

#ifndef __SEC_BATTERY_H
#define __SEC_BATTERY_H __FILE__

#include <linux/battery/sec_charging_common.h>
#include <linux/of_gpio.h>
#include <linux/alarmtimer.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>

#if defined(CONFIG_BATTERY_NOTIFIER)
#include <linux/battery/battery_notifier.h>
#endif /* CONFIG_BATTERY_NOTIFIER */

#if defined(CONFIG_CCIC_NOTIFIER)
#include <linux/ccic/ccic_notifier.h>
#endif /* CONFIG_CCIC_NOTIFIER */

#if defined(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#endif /* CONFIG_MUIC_NOTIFIER */

#if defined(CONFIG_VBUS_NOTIFIER)
#include <linux/vbus_notifier.h>
#endif

#include <linux/sec_batt.h>

#define SEC_BAT_CURRENT_EVENT_NONE					0x0000
#define SEC_BAT_CURRENT_EVENT_AFC					0x0001
#define SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING		0x0010
#define SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING	0x0020

#define SIOP_EVENT_NONE		0x0000
#define SIOP_EVENT_WPC_CALL	0x0001

#if defined(CONFIG_CHARGING_VZWCONCEPT)
#define STORE_MODE_CHARGING_MAX 35
#define STORE_MODE_CHARGING_MIN 30
#else
#define STORE_MODE_CHARGING_MAX 70
#define STORE_MODE_CHARGING_MIN 60
#endif

#define ADC_CH_COUNT		10
#define ADC_SAMPLE_COUNT	10

#define DEFAULT_HEALTH_CHECK_COUNT	5
#define TEMP_HIGHLIMIT_DEFAULT	2000

#define SIOP_INPUT_LIMIT_CURRENT                1200
#define SIOP_CHARGING_LIMIT_CURRENT             1000
#define SIOP_WIRELESS_INPUT_LIMIT_CURRENT       530
#define SIOP_WIRELESS_CHARGING_LIMIT_CURRENT    780
#define SIOP_HV_WIRELESS_INPUT_LIMIT_CURRENT	700
#define SIOP_HV_WIRELESS_CHARGING_LIMIT_CURRENT	600
#define SIOP_HV_INPUT_LIMIT_CURRENT                1200
#define SIOP_HV_CHARGING_LIMIT_CURRENT             1000

#define BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE	0x00000001

struct adc_sample_info {
	unsigned int cnt;
	int total_adc;
	int average_adc;
	int adc_arr[ADC_SAMPLE_COUNT];
	int index;
};

struct sec_battery_info {
	struct device *dev;
	sec_battery_platform_data_t *pdata;
#if defined(CONFIG_CCIC_NOTIFIER)
	bool pdic_attach;
	struct pdic_notifier_struct pdic_info;
#endif
	/* power supply used in Android */
	struct power_supply psy_bat;
	struct power_supply psy_usb;
	struct power_supply psy_ac;
	struct power_supply psy_wireless;
	struct power_supply psy_ps;
	unsigned int irq;

	struct notifier_block batt_nb;
#if defined(CONFIG_BATTERY_NOTIFIER)
	struct notifier_block pdic_nb;
#endif
#if defined(CONFIG_VBUS_NOTIFIER)
	struct notifier_block vbus_nb;
#endif

	int status;
	int health;
	bool present;

	int voltage_now;		/* cell voltage (mV) */
	int voltage_avg;		/* average voltage (mV) */
	int voltage_ocv;		/* open circuit voltage (mV) */
	int current_now;		/* current (mA) */
	int inbat_adc;                  /* inbat adc */
	int current_avg;		/* average current (mA) */
	int current_max;		/* input current limit (mA) */
	int current_adc;

	unsigned int capacity;			/* SOC (%) */

	struct mutex adclock;
	struct adc_sample_info	adc_sample[ADC_CH_COUNT];

	/* keep awake until monitor is done */
	struct wake_lock monitor_wake_lock;
	struct workqueue_struct *monitor_wqueue;
	struct delayed_work monitor_work;
#ifdef CONFIG_SAMSUNG_BATTERY_FACTORY
	struct wake_lock lpm_wake_lock;
#endif
	unsigned int polling_count;
	unsigned int polling_time;
	bool polling_in_sleep;
	bool polling_short;

	struct delayed_work polling_work;
	struct alarm polling_alarm;
	ktime_t last_poll_time;

	/* battery check */
	unsigned int check_count;
	/* ADC check */
	unsigned int check_adc_count;
	unsigned int check_adc_value;

	/* health change check*/
	bool health_change;
	/* ovp-uvlo health check */
	int health_check_count;

	/* time check */
	unsigned long charging_start_time;
	unsigned long charging_passed_time;
	unsigned long charging_next_time;
	unsigned long charging_fullcharged_time;

	unsigned long wc_heating_start_time;
	unsigned long wc_heating_passed_time;
	unsigned int wc_heat_limit;

	/* chg temperature check */
	bool chg_limit;

	/* wpc temperature and pad status check */
	bool pad_limit;

	/* bat temperature check */
	bool mix_limit;

	/* temperature check */
	int temperature;	/* battery temperature */
	int temper_amb;		/* target temperature */
	int chg_temp;		/* charger temperature */
	int pre_chg_temp;
	int wpc_temp;
	int slave_chg_temp;
	int pre_slave_chg_temp;

	int temp_adc;
	int temp_ambient_adc;
	int chg_temp_adc;
	int wpc_temp_adc;
	int slave_chg_temp_adc;

	int temp_highlimit_threshold;
	int temp_highlimit_recovery;
	int temp_high_threshold;
	int temp_high_recovery;
	int temp_low_threshold;
	int temp_low_recovery;

	unsigned int temp_highlimit_cnt;
	unsigned int temp_high_cnt;
	unsigned int temp_low_cnt;
	unsigned int temp_recover_cnt;

	/* charging */
	unsigned int charging_mode;
	bool is_recharging;
	bool is_jig_on;
	int cable_type;
	int muic_cable_type;
#if defined(CONFIG_VBUS_NOTIFIER)
	int muic_vbus_status;
#endif
	int extended_cable_type;
	struct wake_lock cable_wake_lock;
	struct delayed_work cable_work;
	struct wake_lock vbus_wake_lock;
	struct delayed_work siop_work;
	struct wake_lock siop_wake_lock;
	struct wake_lock afc_wake_lock;
	struct delayed_work afc_work;
	struct delayed_work wc_afc_work;
#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	struct delayed_work update_work;
	struct delayed_work fw_init_work;
#endif
	struct delayed_work siop_event_work;
	struct wake_lock siop_event_wake_lock;
	struct delayed_work siop_level_work;
	struct wake_lock siop_level_wake_lock;
	struct delayed_work wc_headroom_work;
	struct wake_lock wc_headroom_wake_lock;
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	struct delayed_work batt_data_work;
	struct wake_lock batt_data_wake_lock;
	char *data_path;
#endif

	unsigned int full_check_cnt;
	unsigned int recharge_check_cnt;

	struct mutex iolock;
	int wired_input_current;
	int wireless_input_current;
	int charging_current;
	int topoff_current;
	unsigned int current_event;

	/* wireless charging enable */
	int wc_enable;
	int wc_enable_cnt;
	int wc_enable_cnt_value;
	int wc_status;
	bool wc_cv_mode;
	bool wc_pack_max_curr;

	int wire_status;

	/* wearable charging */
	int ps_status;
	int ps_enable;

	/* test mode */
	int test_mode;
	bool factory_mode;
	bool store_mode;
	bool ignore_store_mode;
	bool slate_mode;

	/* MTBF test for CMCC */
	bool is_hc_usb;

	bool ignore_siop;
	int r_siop_level;
	int siop_level;
	int siop_event;
	int siop_prev_event;
	int stability_test;
	int eng_not_full_status;

	bool skip_chg_temp_check;
	bool skip_wpc_temp_check;
	bool wpc_temp_mode;
#if defined(CONFIG_BATTERY_SWELLING_SELF_DISCHARGING)
	bool factory_self_discharging_mode_on;
	bool force_discharging;
	bool self_discharging;
	bool discharging_ntc;
	int discharging_ntc_adc;
	int self_discharging_adc;
#endif
#if defined(CONFIG_SW_SELF_DISCHARGING)
	bool sw_self_discharging;
	struct wake_lock self_discharging_wake_lock;
#endif
	bool charging_block;
#if defined(CONFIG_BATTERY_SWELLING)
	bool swelling_mode;
	unsigned long swelling_block_start;
	unsigned long swelling_block_passed;
	int swelling_full_check_cnt;
#endif
#if defined(CONFIG_AFC_CHARGER_MODE)
	char *hv_chg_name;
#endif
#if defined(CONFIG_CALC_TIME_TO_FULL)
	int timetofull;
	bool complete_timetofull;
	struct delayed_work timetofull_work;
#endif
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	int batt_cycle;
#endif
#if defined(CONFIG_STEP_CHARGING)
	unsigned int step_charging_type;
	int step_charging_status;
	int step_charging_step;
#endif

	struct mutex misclock;
	unsigned int misc_event;
	unsigned int prev_misc_event;
	struct delayed_work misc_event_work;
	struct wake_lock misc_event_wake_lock;
};

ssize_t sec_bat_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t sec_bat_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define SEC_BATTERY_ATTR(_name)						\
{									\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = sec_bat_show_attrs,					\
	.store = sec_bat_store_attrs,					\
}

/* event check */
#define EVENT_NONE				(0)
#define EVENT_2G_CALL			(0x1 << 0)
#define EVENT_3G_CALL			(0x1 << 1)
#define EVENT_MUSIC				(0x1 << 2)
#define EVENT_VIDEO				(0x1 << 3)
#define EVENT_BROWSER			(0x1 << 4)
#define EVENT_HOTSPOT			(0x1 << 5)
#define EVENT_CAMERA			(0x1 << 6)
#define EVENT_CAMCORDER			(0x1 << 7)
#define EVENT_DATA_CALL			(0x1 << 8)
#define EVENT_WIFI				(0x1 << 9)
#define EVENT_WIBRO				(0x1 << 10)
#define EVENT_LTE				(0x1 << 11)
#define EVENT_LCD			(0x1 << 12)
#define EVENT_GPS			(0x1 << 13)

enum {
	BATT_RESET_SOC = 0,
	BATT_READ_RAW_SOC,
	BATT_READ_ADJ_SOC,
	BATT_TYPE,
	BATT_VFOCV,
	BATT_VOL_ADC,
	BATT_VOL_ADC_CAL,
	BATT_VOL_AVER,
	BATT_VOL_ADC_AVER,
	BATT_CURRENT_UA_NOW,
	BATT_CURRENT_UA_AVG,
	BATT_FILTER_CFG,
	BATT_TEMP,
	BATT_TEMP_ADC,
	BATT_TEMP_AVER,
	BATT_TEMP_ADC_AVER,
	BATT_CHG_TEMP,
	BATT_CHG_TEMP_ADC,
	BATT_SLAVE_CHG_TEMP,
	BATT_SLAVE_CHG_TEMP_ADC,
	BATT_VF_ADC,
	BATT_SLATE_MODE,

	BATT_LP_CHARGING,
	SIOP_ACTIVATED,
	SIOP_LEVEL,
	SIOP_EVENT,
	BATT_CHARGING_SOURCE,
	FG_REG_DUMP,
	FG_RESET_CAP,
	FG_CAPACITY,
	FG_ASOC,
	AUTH,
	CHG_CURRENT_ADC,
	WC_ADC,
	WC_STATUS,
	WC_ENABLE,
	WC_CONTROL,
	WC_CONTROL_CNT,
	HV_CHARGER_STATUS,
	HV_WC_CHARGER_STATUS,
	HV_CHARGER_SET,
	FACTORY_MODE,
	STORE_MODE,
	UPDATE,
	TEST_MODE,

	BATT_EVENT_CALL,
	BATT_EVENT_2G_CALL,
	BATT_EVENT_TALK_GSM,
	BATT_EVENT_3G_CALL,
	BATT_EVENT_TALK_WCDMA,
	BATT_EVENT_MUSIC,
	BATT_EVENT_VIDEO,
	BATT_EVENT_BROWSER,
	BATT_EVENT_HOTSPOT,
	BATT_EVENT_CAMERA,
	BATT_EVENT_CAMCORDER,
	BATT_EVENT_DATA_CALL,
	BATT_EVENT_WIFI,
	BATT_EVENT_WIBRO,
	BATT_EVENT_LTE,
	BATT_EVENT_LCD,
	BATT_EVENT_GPS,
	BATT_EVENT,
	BATT_TEMP_TABLE,
	BATT_HIGH_CURRENT_USB,
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	BATT_TEST_CHARGE_CURRENT,
#endif
	BATT_STABILITY_TEST,
	BATT_CAPACITY_MAX,
	BATT_INBAT_VOLTAGE,
#if defined(CONFIG_BATTERY_SWELLING_SELF_DISCHARGING)
	BATT_DISCHARGING_CHECK,
	BATT_DISCHARGING_CHECK_ADC,
	BATT_DISCHARGING_NTC,
	BATT_DISCHARGING_NTC_ADC,
	BATT_SELF_DISCHARGING_CONTROL,
#endif
#if defined(CONFIG_SW_SELF_DISCHARGING)
	BATT_SW_SELF_DISCHARGING,
#endif
	BATT_INBAT_WIRELESS_CS100,
	HMT_TA_CONNECTED,
	HMT_TA_CHARGE,
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	FG_CYCLE,
	FG_FULL_VOLTAGE,
	FG_FULLCAPNOM,
	BATTERY_CYCLE,
#endif
	BATT_WPC_TEMP,
	BATT_WPC_TEMP_ADC,
#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	BATT_WIRELESS_FIRMWARE_UPDATE,
	BATT_WIRELESS_OTP_FIRMWARE_RESULT,
	BATT_WIRELESS_IC_GRADE,
	BATT_WIRELESS_FIRMWARE_VER_BIN,
	BATT_WIRELESS_FIRMWARE_VER,
	BATT_WIRELESS_TX_FIRMWARE_RESULT,
	BATT_WIRELESS_TX_FIRMWARE_VER,
	BATT_TX_STATUS,
#endif
	BATT_WIRELESS_VOUT,
	BATT_WIRELESS_VRCT,
	BATT_HV_WIRELESS_STATUS,
	BATT_HV_WIRELESS_PAD_CTRL,
	BATT_TUNE_FLOAT_VOLTAGE,
	BATT_TUNE_INPUT_CHARGE_CURRENT,
	BATT_TUNE_FAST_CHARGE_CURRENT,
	BATT_TUNE_UI_TERM_CURRENT_1ST,
	BATT_TUNE_UI_TERM_CURRENT_2ND,
	BATT_TUNE_TEMP_HIGH_NORMAL,
	BATT_TUNE_TEMP_HIGH_REC_NORMAL,
	BATT_TUNE_TEMP_LOW_NORMAL,
	BATT_TUNE_TEMP_LOW_REC_NORMAL,
	BATT_TUNE_CHG_TEMP_HIGH,
	BATT_TUNE_CHG_TEMP_REC,
	BATT_TUNE_CHG_LIMMIT_CURRENT,
	BATT_TUNE_COIL_TEMP_HIGH,
	BATT_TUNE_COIL_TEMP_REC,
	BATT_TUNE_COIL_LIMMIT_CURRENT,

#if defined(CONFIG_UPDATE_BATTERY_DATA)
	BATT_UPDATE_DATA,
#endif

	BATT_MISC_EVENT,
	BATT_EXT_DEV_CHG,
};

enum {
	EXT_DEV_NONE = 0,
	EXT_DEV_GAMEPAD_CHG,
	EXT_DEV_GAMEPAD_OTG,
};

#ifdef CONFIG_OF
extern int adc_read(struct sec_battery_info *battery, int channel);
extern void adc_init(struct platform_device *pdev, struct sec_battery_info *battery);
extern void adc_exit(struct sec_battery_info *battery);
#endif

#if defined(CONFIG_STEP_CHARGING)
extern void sec_bat_reset_step_charging(struct sec_battery_info *battery);
extern void sec_step_charging_init(struct sec_battery_info *battery, struct device *dev);
extern bool sec_bat_check_step_charging(struct sec_battery_info *battery);
#endif

#if defined(CONFIG_UPDATE_BATTERY_DATA)
extern int sec_battery_update_data(const char* file_path);
#endif

#endif /* __SEC_BATTERY_H */
