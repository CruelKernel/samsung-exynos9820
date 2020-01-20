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

#include "sec_charging_common.h"
#include <linux/of_gpio.h>
#include <linux/alarmtimer.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>

#if defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
#include <linux/usb/manager/usb_typec_manager_notifier.h>
#else
#if defined(CONFIG_CCIC_NOTIFIER)
#include <linux/ccic/ccic_notifier.h>
#endif /* CONFIG_CCIC_NOTIFIER */
#if defined(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#endif
#endif

#if defined(CONFIG_BATTERY_NOTIFIER)
#include <linux/battery/battery_notifier.h>
#endif
#if defined(CONFIG_VBUS_NOTIFIER)
#include <linux/vbus_notifier.h>
#endif

#if defined(CONFIG_BATTERY_CISD)
#include "sec_cisd.h"
#endif

#include <linux/sec_batt.h>

#define SEC_BAT_CURRENT_EVENT_NONE					0x0000
#define SEC_BAT_CURRENT_EVENT_AFC					0x0001
#define SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE		0x0002
#define SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL	0x0004
#define SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING		0x0010
#define SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING	0x0020
#if defined(CONFIG_ENABLE_100MA_CHARGING_BEFORE_USB_CONFIGURED)
#define SEC_BAT_CURRENT_EVENT_USB_100MA			0x0040
#else
#define SEC_BAT_CURRENT_EVENT_USB_100MA			0x0000
#endif
#define SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND			0x0080
#define SEC_BAT_CURRENT_EVENT_SWELLING_MODE		(SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND | SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING)
#define SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE		(SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND)
#define SEC_BAT_CURRENT_EVENT_USB_SUPER			0x0100
#define SEC_BAT_CURRENT_EVENT_CHG_LIMIT			0x0200
#define SEC_BAT_CURRENT_EVENT_CALL			0x0400
#define SEC_BAT_CURRENT_EVENT_SLATE			0x0800
#define SEC_BAT_CURRENT_EVENT_VBAT_OVP			0x1000
#define SEC_BAT_CURRENT_EVENT_VSYS_OVP			0x2000
#define SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK		0x4000
#define SEC_BAT_CURRENT_EVENT_AICL			0x8000
#define SEC_BAT_CURRENT_EVENT_HV_DISABLE		0x10000
#define SEC_BAT_CURRENT_EVENT_SELECT_PDO		0x20000
#define SEC_BAT_CURRENT_EVENT_FG_RESET			0x40000

#define SIOP_EVENT_NONE		0x0000
#define SIOP_EVENT_WPC_CALL	0x0001

#if defined(CONFIG_SEC_FACTORY)             // SEC_FACTORY
#define STORE_MODE_CHARGING_MAX 80
#define STORE_MODE_CHARGING_MIN 70
#else                                       // !SEC_FACTORY, STORE MODE
#define STORE_MODE_CHARGING_MAX 70
#define STORE_MODE_CHARGING_MIN 60
#define STORE_MODE_CHARGING_MAX_VZW 35
#define STORE_MODE_CHARGING_MIN_VZW 30
#endif //(CONFIG_SEC_FACTORY)

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
#define SIOP_STORE_HV_WIRELESS_CHARGING_LIMIT_CURRENT	450
#define SIOP_HV_INPUT_LIMIT_CURRENT				1200
#define SIOP_HV_CHARGING_LIMIT_CURRENT			1000
#define SIOP_HV_12V_INPUT_LIMIT_CURRENT			535
#define SIOP_HV_12V_CHARGING_LIMIT_CURRENT		1000

#define WIRELESS_OTG_INPUT_CURRENT 900

#define BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE	0x00000001
#define BATT_MISC_EVENT_WIRELESS_BACKPACK_TYPE	0x00000002
#define BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE		0x00000004
#define BATT_MISC_EVENT_BATT_RESET_SOC			0x00000008
#define BATT_MISC_EVENT_HICCUP_TYPE				0x00000020
#define BATT_MISC_EVENT_WIRELESS_FOD			0x00000100

#define SEC_INPUT_VOLTAGE_0V	0
#define SEC_INPUT_VOLTAGE_5V	5
#define SEC_INPUT_VOLTAGE_9V	9
#define SEC_INPUT_VOLTAGE_10V	10
#define SEC_INPUT_VOLTAGE_12V	12

#define HV_CHARGER_STATUS_STANDARD1	12000 /* mW */
#define HV_CHARGER_STATUS_STANDARD2	20000 /* mW */

#if defined(CONFIG_CCIC_NOTIFIER)
struct sec_bat_pdic_info {
	unsigned int input_voltage;
	unsigned int input_current;
	unsigned int pdo_index;
};

struct sec_bat_pdic_list {
	struct sec_bat_pdic_info pd_info[8]; /* 5V ~ 12V */
	unsigned int now_pd_index;
	unsigned int max_pd_count;
};
#endif

#if defined(CONFIG_BATTERY_SWELLING)
enum swelling_mode_state {
	SWELLING_MODE_NONE = 0,
	SWELLING_MODE_CHARGING,
	SWELLING_MODE_FULL,
};
#endif

struct adc_sample_info {
	unsigned int cnt;
	int total_adc;
	int average_adc;
	int adc_arr[ADC_SAMPLE_COUNT];
	int index;
};

struct cable_info {
	int cable_type;
	int muic_cable_type;

	int input_current;
	int charging_current;

	unsigned int input_voltage;		/* CHGIN/WCIN input voltage (V) */
	unsigned int charge_power;
	unsigned int max_charge_power;		/* max charge power (mW) */
	unsigned int pd_max_charge_power;		/* max charge power for pd (mW) */

#if defined(CONFIG_CCIC_NOTIFIER)
	bool pdic_attach;
	bool pdic_ps_rdy;

	struct pdic_notifier_struct pdic_info;
	struct sec_bat_pdic_list pd_list;
#endif
	int pd_usb_attached;

#if defined(CONFIG_AFC_CHARGER_MODE)
	char *hv_chg_name;
#endif
};

struct sec_battery_info {
	struct device *dev;
	sec_battery_platform_data_t *pdata;

	/* power supply used in Android */
	struct power_supply *psy_bat;
	struct power_supply *psy_usb;
	struct power_supply *psy_ac;
	struct power_supply *psy_wireless;
	struct power_supply *psy_ps;
	unsigned int irq;

	int pd_usb_attached;
#if defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	struct notifier_block usb_typec_nb;
	struct notifier_block usb_typec_sub_nb;
#else
#if defined(CONFIG_CCIC_NOTIFIER)
	struct notifier_block pdic_nb;
#endif
#if defined(CONFIG_MUIC_NOTIFIER)
	struct notifier_block batt_nb;
#endif
#endif

#if defined(CONFIG_VBUS_NOTIFIER)
	struct notifier_block vbus_nb;
	int muic_vbus_status;
#endif

	bool is_sysovlo;
	bool is_vbatovlo;
	bool is_abnormal_temp;

	bool safety_timer_set;
	bool lcd_status;
	bool skip_swelling;

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
	int charge_counter;		/* remaining capacity (uAh) */
	int current_adc;

	unsigned int capacity;			/* SOC (%) */
	unsigned int input_voltage;		/* CHGIN/WCIN input voltage (V) */
	unsigned int charge_power;		/* charge power (mW) */
	unsigned int max_charge_power;		/* max charge power (mW) */
	unsigned int pd_max_charge_power;		/* max charge power for pd (mW) */
	unsigned int aicl_current;

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

#if defined(CONFIG_BATTERY_CISD)
	struct cisd cisd;
	bool skip_cisd;
	bool usb_overheat_check;
	int prev_volt;
	int prev_temp;
	int prev_jig_on;
	int enable_update_data;
	int prev_chg_on;
#endif

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
	unsigned int chg_limit;
	unsigned int chg_limit_recovery_cable;
	unsigned int vbus_chg_by_siop;
	unsigned int vbus_chg_by_full;
	unsigned int mix_limit;
	unsigned int vbus_limit;

	/* temperature check */
	int temperature;	/* battery temperature */
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	int temperature_test_battery;
	int temperature_test_usb;
	int temperature_test_wpc;
	int temperature_test_chg;
#endif
	int temper_amb;		/* target temperature */
	int usb_temp;
	int chg_temp;		/* charger temperature */
	int wpc_temp;
	int coil_temp;
	int slave_chg_temp;

	int temp_adc;
	int temp_ambient_adc;
	int usb_temp_adc;
	int chg_temp_adc;
	int wpc_temp_adc;
	int coil_temp_adc;
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
	int wdt_kick_disable;

	struct cable_info *select;
	struct cable_info *main;
	struct cable_info *sub;

	bool is_jig_on;
	int cable_type;
	int charging_port;

	struct wake_lock cable_wake_lock;
	struct delayed_work cable_work;
	struct wake_lock vbus_wake_lock;
	struct delayed_work siop_work;
	struct wake_lock siop_wake_lock;
	struct wake_lock afc_wake_lock;
	struct delayed_work afc_work;
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
#ifdef CONFIG_OF
	struct delayed_work parse_mode_dt_work;
	struct wake_lock parse_mode_dt_wake_lock;
#endif
	struct delayed_work init_chg_work;

	char batt_type[48];
	unsigned int full_check_cnt;
	unsigned int recharge_check_cnt;

	struct mutex iolock;
	int input_current;
	int charging_current;
	int topoff_current;
	int wpc_vout_level;
	unsigned int current_event;

	/* wireless charging enable */
	struct mutex wclock;
	int wc_enable;
	int wc_enable_cnt;
	int wc_enable_cnt_value;
	int led_cover;
	int wc_status;
	bool wc_cv_mode;
	bool wc_pack_max_curr;

	/* wearable charging */
	int ps_status;
	int ps_enable;

	/* test mode */
	int test_mode;
	bool factory_mode;
	bool store_mode;
	bool slate_mode;

	/* MTBF test for CMCC */
	bool is_hc_usb;

	int siop_level;
	int siop_event;
	int siop_prev_event;
	int stability_test;
	int eng_not_full_status;

	bool skip_chg_temp_check;
	bool skip_wpc_temp_check;
	bool wpc_temp_mode;
	bool charging_block;
#if defined(CONFIG_BATTERY_SWELLING)
	unsigned int swelling_mode;
	int swelling_full_check_cnt;
#endif
#if defined(CONFIG_CALC_TIME_TO_FULL)
	int timetofull;
	struct delayed_work timetofull_work;
#endif
#if defined(CONFIG_ENABLE_100MA_CHARGING_BEFORE_USB_CONFIGURED)
	struct delayed_work slowcharging_work;
#endif
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	int batt_cycle;
#endif
#if defined(CONFIG_STEP_CHARGING)
	unsigned int step_charging_type;
	unsigned int step_charging_charge_power;
	int step_charging_status;
	int step_charging_step;
#endif
#if defined(CONFIG_ENG_BATTERY_CONCEPT) || defined(CONFIG_SEC_FACTORY)
	bool cooldown_mode;
#endif
	struct mutex misclock;
	unsigned int misc_event;
	unsigned int prev_misc_event;
	struct delayed_work misc_event_work;
	struct wake_lock misc_event_wake_lock;
	struct mutex batt_handlelock;
	struct mutex current_eventlock;
	struct mutex typec_notylock;

	unsigned int hiccup_status;

	bool stop_timer;
	unsigned long prev_safety_time;
	unsigned long expired_time;
	unsigned long cal_safety_time;
	int fg_reset;
};

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

extern bool sleep_mode;

extern void select_pdo(int num);

extern int adc_read(struct sec_battery_info *battery, int channel);
extern void adc_init(struct platform_device *pdev, struct sec_battery_info *battery);
extern void adc_exit(struct sec_battery_info *battery);
extern void sec_cable_init(struct platform_device *pdev, struct sec_battery_info *battery);
extern int sec_bat_get_adc_data(struct sec_battery_info *battery, int adc_ch, int count);
extern int sec_bat_get_charger_type_adc(struct sec_battery_info *battery);
extern bool sec_bat_get_value_by_adc(struct sec_battery_info *battery, enum sec_battery_adc_channel channel, union power_supply_propval *value);
extern int sec_bat_get_adc_value(struct sec_battery_info *battery, int channel);
extern bool sec_bat_check_vf_adc(struct sec_battery_info *battery);
extern void sec_bat_set_misc_event(struct sec_battery_info *battery, const int misc_event_type, bool do_clear);
extern void sec_bat_set_current_event(struct sec_battery_info *battery, unsigned int current_event_val, unsigned int current_event_mask);
extern void sec_bat_get_battery_info(struct sec_battery_info *battery);
extern int sec_bat_set_charge(struct sec_battery_info *battery, int chg_mode);
extern int sec_bat_set_charging_current(struct sec_battery_info *battery);
extern void sec_bat_aging_check(struct sec_battery_info *battery);

#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
extern void sec_bat_fw_update_work(struct sec_battery_info *battery, int mode);
#endif

#if defined(CONFIG_STEP_CHARGING)
extern void sec_bat_reset_step_charging(struct sec_battery_info *battery);
extern void sec_step_charging_init(struct sec_battery_info *battery, struct device *dev);
extern bool sec_bat_check_step_charging(struct sec_battery_info *battery);
#endif

#if defined(CONFIG_UPDATE_BATTERY_DATA)
extern int sec_battery_update_data(const char* file_path);
#endif
#if defined(CONFIG_BATTERY_CISD)
extern bool sec_bat_cisd_check(struct sec_battery_info *battery);
extern void sec_battery_cisd_init(struct sec_battery_info *battery);
extern void set_cisd_pad_data(struct sec_battery_info *battery, const char* buf);
#endif
#endif /* __SEC_BATTERY_H */
