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
#if defined(CONFIG_DIRECT_CHARGING)
#include "sec_direct_charger.h"
#endif

#include <linux/sec_batt.h>
#if defined(CONFIG_WIRELESS_AUTH)
#include "sec_battery_misc.h"
#endif

extern char *sec_cable_type[];

/* current event */
#define SEC_BAT_CURRENT_EVENT_NONE					0x000000
#define SEC_BAT_CURRENT_EVENT_AFC					0x000001
#define SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE		0x000002
#define SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL	0x000004
#define SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING		0x000080
#define SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING	0x000020
#if defined(CONFIG_ENABLE_100MA_CHARGING_BEFORE_USB_CONFIGURED)
#define SEC_BAT_CURRENT_EVENT_USB_100MA			0x000040
#else
#define SEC_BAT_CURRENT_EVENT_USB_100MA			0x000000
#endif
#define SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND			0x000010
#define SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_3RD			0x000008
#define SEC_BAT_CURRENT_EVENT_SWELLING_MODE		(SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND | SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_3RD | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_4TH | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_5TH)
#define SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE		(SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_3RD | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_4TH | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_5TH)
#define SEC_BAT_CURRENT_EVENT_USB_SUPER			0x000100
#define SEC_BAT_CURRENT_EVENT_CHG_LIMIT			0x000200
#define SEC_BAT_CURRENT_EVENT_CALL			0x000400
#define SEC_BAT_CURRENT_EVENT_SLATE			0x000800
#define SEC_BAT_CURRENT_EVENT_VBAT_OVP			0x001000
#define SEC_BAT_CURRENT_EVENT_VSYS_OVP			0x002000
#define SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK		0x004000
#define SEC_BAT_CURRENT_EVENT_AICL			0x008000
#define SEC_BAT_CURRENT_EVENT_HV_DISABLE		0x010000
#define SEC_BAT_CURRENT_EVENT_SELECT_PDO		0x020000
#define SEC_BAT_CURRENT_EVENT_FG_RESET			0x040000
#define SEC_BAT_CURRENT_EVENT_WDT_EXPIRED		0x080000
#define SEC_BAT_CURRENT_EVENT_SAFETY_TMR		0x100000
#define SEC_BAT_CURRENT_EVENT_ISDB			0x200000
#define SEC_BAT_CURRENT_EVENT_DC_ERR			0x400000
#define SEC_BAT_CURRENT_EVENT_SIOP_LIMIT		0x800000
#define SEC_BAT_CURRENT_EVENT_TEMP_CTRL_TEST		0x1000000
#define SEC_BAT_CURRENT_EVENT_25W_OCP			0x2000000
#define SEC_BAT_CURRENT_EVENT_SEND_UVDM			0x8000000

#define SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_4TH	0x10000000
#define SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_5TH	0x20000000
#define SEC_BAT_CURRENT_EVENT_WPC_EN			0x40000000

/* misc_event */
#define BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE	0x00000001
#define BATT_MISC_EVENT_WIRELESS_BACKPACK_TYPE	0x00000002
#define BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE		0x00000004
#define BATT_MISC_EVENT_BATT_RESET_SOC			0x00000008
#define BATT_MISC_EVENT_HICCUP_TYPE				0x00000020
#define BATT_MISC_EVENT_WIRELESS_DET_LEVEL		0x00000040
#define BATT_MISC_EVENT_WIRELESS_FOD			0x00000100
#define BATT_MISC_EVENT_WIRELESS_AUTH_START     0x00000200
#define BATT_MISC_EVENT_WIRELESS_AUTH_RECVED    0x00000400
#define BATT_MISC_EVENT_WIRELESS_AUTH_FAIL      0x00000800
#define BATT_MISC_EVENT_WIRELESS_AUTH_PASS      0x00001000
#define BATT_MISC_EVENT_TEMP_HICCUP_TYPE	0x00002000
#define BATT_MISC_EVENT_BATTERY_HEALTH			0x000F0000
#define BATT_MISC_EVENT_HEALTH_OVERHEATLIMIT		0x00100000
#define BATT_MISC_EVENT_FULL_CAPACITY		0x01000000

#define BATTERY_HEALTH_SHIFT                16
enum misc_battery_health {
	BATTERY_HEALTH_UNKNOWN = 0,
	BATTERY_HEALTH_GOOD,
	BATTERY_HEALTH_NORMAL,
	BATTERY_HEALTH_AGED,
	BATTERY_HEALTH_MAX = BATTERY_HEALTH_AGED,

	/* For event */
	BATTERY_HEALTH_BAD = 0xF,
};

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
#if defined(CONFIG_DIRECT_CHARGING)
#define SIOP_APDO_INPUT_LIMIT_CURRENT				1000
#define SIOP_APDO_CHARGING_LIMIT_CURRENT			2000
#endif

#define WIRELESS_OTG_INPUT_CURRENT 900

#define SEC_INPUT_VOLTAGE_0V	0
#define SEC_INPUT_VOLTAGE_5V	50
#define SEC_INPUT_VOLTAGE_5_5V	55
#define SEC_INPUT_VOLTAGE_9V	90
#define SEC_INPUT_VOLTAGE_10V	100
#define SEC_INPUT_VOLTAGE_12V	120
#define SEC_INPUT_VOLTAGE_12_5V	125
#define SEC_INPUT_VOLTAGE_NONE	1000

#define HV_CHARGER_STATUS_STANDARD1	12000 /* mW */
#define HV_CHARGER_STATUS_STANDARD2	20000 /* mW */
#define HV_CHARGER_STATUS_STANDARD3 24500 /* mW */
#define HV_CHARGER_STATUS_STANDARD4 40000 /* mW */

enum {
	NORMAL_TA,
	AFC_9V_OR_15W,
	AFC_12V_OR_20W,
	SFC_25W,
	SFC_45W,
};

#if defined(CONFIG_CCIC_NOTIFIER)
struct sec_bat_pdic_info {
	unsigned int pdo_index;
#if defined(CONFIG_PDIC_PD30) && defined(CONFIG_DIRECT_CHARGING)
	bool apdo;
	unsigned int max_voltage;
	unsigned int min_voltage;
	unsigned int max_current;
#else
	unsigned int input_voltage;
	unsigned int input_current;
#endif
};

struct sec_bat_pdic_list {
	struct sec_bat_pdic_info pd_info[MAX_PDO_NUM]; /* 5V ~ 12V */
	unsigned int now_pd_index;
	unsigned int max_pd_count;
#if defined(CONFIG_PDIC_PD30)
	bool now_isApdo;
	unsigned int num_fpdo;
	unsigned int num_apdo;
#endif
};
#endif

#if defined(CONFIG_BATTERY_SWELLING)
enum swelling_mode_state {
	SWELLING_MODE_NONE = 0,
	SWELLING_MODE_CHARGING,
	SWELLING_MODE_FULL,
};
#endif

enum tx_switch_mode_state {
	TX_SWITCH_MODE_OFF = 0,
	TX_SWITCH_CHG_ONLY,
	TX_SWITCH_UNO_ONLY,
};

struct adc_sample_info {
	unsigned int cnt;
	int total_adc;
	int average_adc;
	int adc_arr[ADC_SAMPLE_COUNT];
	int index;
};

#if defined(CONFIG_BATTERY_SAMSUNG_MHS)
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
	bool hv_pdo;

	struct pdic_notifier_struct pdic_info;
	struct sec_bat_pdic_list pd_list;
#endif
	int pd_usb_attached;

#if defined(CONFIG_AFC_CHARGER_MODE)
	char *hv_chg_name;
#endif
};
#endif

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
#if defined(CONFIG_BATTERY_SAMSUNG_MHS)
	struct notifier_block usb_typec_main_nb;
#endif
#else
#if defined(CONFIG_CCIC_NOTIFIER)
	struct notifier_block pdic_nb;
#endif
#if defined(CONFIG_MUIC_NOTIFIER)
	struct notifier_block batt_nb;
#endif
#endif

#if defined(CONFIG_CCIC_NOTIFIER)
	bool pdic_attach;
	bool pdic_ps_rdy;
	bool hv_pdo;
	bool init_src_cap;
	struct pdic_notifier_struct pdic_info;
	struct sec_bat_pdic_list pd_list;
#endif
	bool update_pd_list;

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
	unsigned int charger_mode;

	int voltage_now;		/* cell voltage (mV) */
	int voltage_avg;		/* average voltage (mV) */
	int voltage_ocv;		/* open circuit voltage (mV) */
	int current_now;		/* current (mA) */
	int inbat_adc;                  /* inbat adc */
	int current_avg;		/* average current (mA) */
	int current_max;		/* input current limit (mA) */
	int current_sys;		/* system current (mA) */
	int current_sys_avg;		/* average system current (mA) */
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

#if defined(CONFIG_WIRELESS_AUTH)
	sec_bat_misc_dev_t *misc_dev;
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

	/* lrp temperature check */
	unsigned int lrp_limit;
	unsigned int lrp_step;

	/* temperature check */
	int temperature;	/* battery temperature */
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	int temperature_test_battery;
	int temperature_test_usb;
	int temperature_test_wpc;
	int temperature_test_chg;
	int temperature_test_dchg;
	bool test_max_current;
	bool test_charge_current;
#if defined(CONFIG_STEP_CHARGING)
	int test_step_condition;
#endif
#endif
	int temper_amb;		/* target temperature */
	int usb_temp;
	int chg_temp;		/* charger temperature */
	int wpc_temp;
	int coil_temp;
	int slave_chg_temp;
	int usb_temp_flag;
#if defined(CONFIG_PREVENT_USB_CONN_OVERHEAT)
	int usb_protection_temp;
	int temp_gap_bat_usb;
#endif
#if defined(CONFIG_DIRECT_CHARGING)
	int dchg_temp;
#endif
	int lrp;
	int lrp_test;

	int temp_adc;
	int temp_ambient_adc;
	int usb_temp_adc;
	int chg_temp_adc;
	int wpc_temp_adc;
	int coil_temp_adc;
	int slave_chg_temp_adc;
#if defined(CONFIG_DIRECT_CHARGING)
	int dchg_temp_adc;
#endif

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

#if defined(CONFIG_BATTERY_SAMSUNG_MHS)
	struct cable_info *select;
	struct cable_info *main;
	struct cable_info *sub;
#endif

	bool is_jig_on;
	int cable_type;
	int muic_cable_type;
	int extended_cable_type;

	bool auto_mode;

#if defined(CONFIG_BATTERY_SAMSUNG_MHS)
	int charging_port;
#endif
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
	struct delayed_work siop_level_work;
	struct wake_lock siop_level_wake_lock;
	struct delayed_work wc_headroom_work;
	struct wake_lock wc_headroom_wake_lock;
	struct wake_lock wpc_tx_wake_lock;
	struct delayed_work wpc_tx_work;
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
	int wpc_max_vout_level;
	unsigned int current_event;
	bool refresh_current;

	/* wireless charging enable */
	struct mutex wclock;
	int wc_enable;
	int wc_enable_cnt;
	int wc_enable_cnt_value;
	int led_cover;
	int wc_status;
	bool wc_cv_mode;
	bool wc_pack_max_curr;
	bool wc_rx_phm_mode;
	bool wc_need_ldo_on;

	int wire_status;

	/* wireless tx */
	bool wc_tx_enable;
	bool wc_rx_connected;
	bool wc_tx_chg_limit;
	bool afc_disable;
	bool buck_cntl_by_tx;
	bool tx_switch_mode_change;
	int wc_tx_vout;
	bool uno_en;
	unsigned int wc_rx_type;
	unsigned int tx_minduty;
	unsigned int tx_switch_mode;
	unsigned int tx_switch_start_soc;

	unsigned int tx_mfc_iout;
	unsigned int tx_uno_iout;
	unsigned int wc20_power_class;
	unsigned int wc20_vout;

	/* wearable charging */
	int ps_status;
	int ps_enable;

	/* test mode */
	int test_mode;
	bool factory_mode;
	bool store_mode;

	/* MTBF test for CMCC */
	bool is_hc_usb;

	int siop_level;
	int siop_prev_event;
	int stability_test;
	int eng_not_full_status;

	bool skip_chg_temp_check;
	bool skip_wpc_temp_check;
	bool wpc_temp_mode;
	bool charging_block;
	bool wpc_vout_ctrl_lcd_on;
#if defined(CONFIG_BATTERY_SWELLING)
	unsigned int swelling_mode;
	int swelling_full_check_cnt;
	bool swelling_low_temp_3rd_ctrl;
	bool swelling_low_temp_4th_ctrl;
	bool swelling_low_temp_5th_ctrl;
#endif
#if defined(CONFIG_AFC_CHARGER_MODE)
	char *hv_chg_name;
#endif
#if defined(CONFIG_CALC_TIME_TO_FULL)
	int timetofull;
	unsigned int ttf_predict_wc20_charge_current;
	struct delayed_work timetofull_work;
#endif
#if defined(CONFIG_WIRELESS_TX_MODE)
	int tx_avg_curr;
	int tx_time_cnt;
	int tx_total_power;
	int tx_total_power_cisd;
	bool tx_clear;
	bool tx_clear_cisd;
	struct delayed_work wpc_txpower_calc_work;
#endif
#if defined(CONFIG_ENABLE_100MA_CHARGING_BEFORE_USB_CONFIGURED)
	struct delayed_work slowcharging_work;
#endif
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	int batt_cycle;
#endif
	int batt_asoc;
#if defined(CONFIG_STEP_CHARGING)
	unsigned int step_charging_type;
	unsigned int step_charging_charge_power;
	int step_charging_status;
	int step_charging_step;
#if defined(CONFIG_DIRECT_CHARGING)
	int dc_step_chg_step;
	unsigned int dc_step_chg_type;
	unsigned int dc_step_chg_charge_power;

	bool dc_float_voltage_set;
	unsigned int dc_step_chg_iin_cnt;
#endif
#endif
#if defined(CONFIG_ENG_BATTERY_CONCEPT) || defined(CONFIG_SEC_FACTORY)
	bool cooldown_mode;
#endif
	struct mutex misclock;
	struct mutex txeventlock;
	unsigned int misc_event;
	unsigned int tx_event;
	unsigned int ext_event;
	unsigned int prev_misc_event;
	unsigned int tx_retry_case;
	unsigned int tx_misalign_cnt;
	unsigned int tx_ocp_cnt;
	struct delayed_work ext_event_work;
	struct delayed_work misc_event_work;
	struct delayed_work wpc_tx_en_work;
	struct wake_lock ext_event_wake_lock;
	struct wake_lock misc_event_wake_lock;
	struct wake_lock tx_event_wake_lock;
	struct wake_lock wpc_tx_en_wake_lock;
	struct mutex batt_handlelock;
	struct mutex current_eventlock;
	struct mutex typec_notylock;
	struct mutex voutlock;
	unsigned long tx_misalign_start_time;
	unsigned long tx_misalign_passed_time;
	unsigned long tx_ocp_start_time;
	unsigned long tx_ocp_passed_time;

	unsigned int hiccup_status;
	bool hiccup_clear;

	bool stop_timer;
	unsigned long prev_safety_time;
	unsigned long expired_time;
	unsigned long cal_safety_time;
	int fg_reset;

	/* 25w ta alert */
	bool ta_alert_wa;
	int ta_alert_mode;

	bool boot_complete;

	bool support_unknown_wpcthm;
	int batt_full_capacity;
	bool usb_slow_chg;
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
extern bool mfc_fw_update;
extern bool boot_complete;

extern void select_pdo(int num);
#if defined(CONFIG_PDIC_PD30)
extern int sec_pd_select_pps(int num, int ppsVol, int ppsCur);
extern int sec_pd_get_apdo_max_power(unsigned int *pdo_pos, unsigned int *taMaxVol, unsigned int *taMaxCur, unsigned int *taMaxPwr);
#endif

extern int adc_read(struct sec_battery_info *battery, int channel);
extern void adc_init(struct platform_device *pdev, struct sec_battery_info *battery);
extern void adc_exit(struct sec_battery_info *battery);
extern void sec_cable_init(struct platform_device *pdev, struct sec_battery_info *battery);
extern int sec_bat_get_adc_data(struct sec_battery_info *battery, int adc_ch, int count);
extern int sec_bat_get_charger_type_adc(struct sec_battery_info *battery);
extern bool sec_bat_get_value_by_adc(struct sec_battery_info *battery, enum sec_battery_adc_channel channel, union power_supply_propval *value, enum sec_battery_temp_check check_type);
extern int sec_bat_get_adc_value(struct sec_battery_info *battery, int channel);
extern bool sec_bat_check_vf_adc(struct sec_battery_info *battery);
#if defined(CONFIG_DIRECT_CHARGING)
extern int sec_bat_get_direct_chg_temp_adc(struct sec_battery_info *battery, int adc_data, int count);
#endif
extern void sec_bat_set_misc_event(struct sec_battery_info *battery, unsigned int misc_event_val, unsigned int misc_event_mask);
extern void sec_bat_set_tx_event(struct sec_battery_info *battery, unsigned int tx_event_val, unsigned int tx_event_mask);
extern void sec_bat_set_current_event(struct sec_battery_info *battery, unsigned int current_event_val, unsigned int current_event_mask);
extern void sec_bat_set_temp_control_test(struct sec_battery_info *battery, bool temp_enable);
extern void sec_bat_get_battery_info(struct sec_battery_info *battery);
extern int sec_bat_set_charge(struct sec_battery_info *battery, int chg_mode);
extern int sec_bat_set_charging_current(struct sec_battery_info *battery);
extern void sec_bat_refresh_charging_current(struct sec_battery_info *battery);
extern void sec_bat_aging_check(struct sec_battery_info *battery);
extern void sec_wireless_set_tx_enable(struct sec_battery_info *battery, bool wc_tx_enable);

extern void sec_bat_check_battery_health(struct sec_battery_info *battery);

#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
extern void sec_bat_fw_update_work(struct sec_battery_info *battery, int mode);
extern bool sec_bat_check_boost_mfc_condition(struct sec_battery_info *battery, int mode);
#endif

#if defined(CONFIG_STEP_CHARGING)
extern void sec_bat_reset_step_charging(struct sec_battery_info *battery);
extern void sec_step_charging_init(struct sec_battery_info *battery, struct device *dev);
extern bool sec_bat_check_step_charging(struct sec_battery_info *battery);
#if defined(CONFIG_DIRECT_CHARGING)
extern bool sec_bat_check_dc_step_charging(struct sec_battery_info *battery);
#endif
#if defined(CONFIG_BATTERY_AGE_FORECAST)
void sec_bat_set_aging_info_step_charging(struct sec_battery_info *battery);
#endif
#endif

#if defined(CONFIG_DIRECT_CHARGING)
extern void sec_direct_chg_init(struct sec_battery_info *battery, struct device *dev);
#endif

#if defined(CONFIG_UPDATE_BATTERY_DATA)
extern int sec_battery_update_data(const char* file_path);
#endif
#if defined(CONFIG_BATTERY_CISD)
extern bool sec_bat_cisd_check(struct sec_battery_info *battery);
extern void sec_battery_cisd_init(struct sec_battery_info *battery);
extern void set_cisd_pad_data(struct sec_battery_info *battery, const char* buf);
extern void set_cisd_power_data(struct sec_battery_info *battery, const char* buf);
#endif

#if defined(CONFIG_WIRELESS_AUTH)
extern int sec_bat_misc_init(struct sec_battery_info *battery);
#endif

int sec_bat_parse_dt(struct device *dev, struct sec_battery_info *battery);
void sec_bat_parse_mode_dt(struct sec_battery_info *battery);
void sec_bat_parse_mode_dt_work(struct work_struct *work);
u8 sec_bat_get_wireless20_power_class(struct sec_battery_info *battery);
#endif /* __SEC_BATTERY_H */
