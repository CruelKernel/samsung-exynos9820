/*
 * sec_charging_common.h
 * Samsung Mobile Charging Common Header
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

#ifndef __SEC_CHARGING_COMMON_H
#define __SEC_CHARGING_COMMON_H __FILE__

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/wakelock.h>

/* definitions */
#define SEC_BATTERY_CABLE_HV_WIRELESS_ETX	100

#define WC_AUTH_MSG		"@WC_AUTH "
#define WC_TX_MSG		"@Tx_Mode "

#define MFC_LDO_ON		1
#define MFC_LDO_OFF		0

enum power_supply_ext_property {
	POWER_SUPPLY_EXT_PROP_CHECK_SLAVE_I2C = POWER_SUPPLY_PROP_MAX,
	POWER_SUPPLY_EXT_PROP_MULTI_CHARGER_MODE,
	POWER_SUPPLY_EXT_PROP_WIRELESS_OP_FREQ,
	POWER_SUPPLY_EXT_PROP_WIRELESS_TRX_CMD,
	POWER_SUPPLY_EXT_PROP_WIRELESS_TRX_VAL,
	POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ID,
	POWER_SUPPLY_EXT_PROP_WIRELESS_ERR,
	POWER_SUPPLY_EXT_PROP_WIRELESS_SWITCH,
	POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ENABLE,
	POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT,
	POWER_SUPPLY_EXT_PROP_WIRELESS_TX_IOUT,
	POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_VIN,
	POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_IIN,
	POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONNECTED,
	POWER_SUPPLY_EXT_PROP_WIRELESS_RX_POWER,
	POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS,	
	POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_DATA,
	POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_SIZE,
	POWER_SUPPLY_EXT_PROP_WIRELESS_RX_TYPE,
	POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ERR,
	POWER_SUPPLY_EXT_PROP_WIRELESS_MIN_DUTY,
	POWER_SUPPLY_EXT_PROP_WIRELESS_SEND_FSK,
	POWER_SUPPLY_EXT_PROP_WIRELESS_RX_VOUT,
	POWER_SUPPLY_EXT_PROP_WIRELESS_INITIAL_WC_CHECK,
	POWER_SUPPLY_EXT_PROP_WIRELESS_PARAM_INFO,
	POWER_SUPPLY_EXT_PROP_WIRELESS_CHECK_FW_VER,
	POWER_SUPPLY_EXT_PROP_AICL_CURRENT,
	POWER_SUPPLY_EXT_PROP_CHECK_MULTI_CHARGE,
	POWER_SUPPLY_EXT_PROP_CHIP_ID,
	POWER_SUPPLY_EXT_PROP_ERROR_CAUSE,
	POWER_SUPPLY_EXT_PROP_SYSOVLO,
	POWER_SUPPLY_EXT_PROP_VBAT_OVP,
	POWER_SUPPLY_EXT_PROP_USB_CONFIGURE,
	POWER_SUPPLY_EXT_PROP_WDT_STATUS,
	POWER_SUPPLY_EXT_PROP_WATER_DETECT,
	POWER_SUPPLY_EXT_PROP_SURGE,
	POWER_SUPPLY_EXT_PROP_SUB_PBA_TEMP_REC,
	POWER_SUPPLY_EXT_PROP_OVERHEAT_NOTIFY,
	POWER_SUPPLY_EXT_PROP_CHARGE_POWER,
	POWER_SUPPLY_EXT_PROP_MEASURE_SYS,
	POWER_SUPPLY_EXT_PROP_MEASURE_INPUT,
	POWER_SUPPLY_EXT_PROP_HV_DISABLE,
	POWER_SUPPLY_EXT_PROP_WC_CONTROL,
	POWER_SUPPLY_EXT_PROP_CHGINSEL,
	POWER_SUPPLY_EXT_PROP_JIG_GPIO,
	POWER_SUPPLY_EXT_PROP_OVERHEAT_HICCUP,
	POWER_SUPPLY_EXT_PROP_MONITOR_WORK,
	POWER_SUPPLY_EXT_PROP_SHIPMODE_TEST,
	POWER_SUPPLY_EXT_PROP_AUTO_SHIPMODE_CONTROL,
	POWER_SUPPLY_EXT_PROP_WIRELESS_TIMER_ON,
	POWER_SUPPLY_EXT_PROP_CALL_EVENT,
	POWER_SUPPLY_EXT_PROP_DEFAULT_CURRENT,
	POWER_SUPPLY_PROP_WIRELESS_RX_POWER,
	POWER_SUPPLY_PROP_WIRELESS_MAX_VOUT,
	POWER_SUPPLY_EXT_PROP_CURRENT_EVENT,
	POWER_SUPPLY_EXT_PROP_PAD_VOLT_CTRL,
	POWER_SUPPLY_EXT_PROP_CURRENT_EVENT_CLEAR,
#if defined(CONFIG_BATTERY_SAMSUNG_MHS)
	POWER_SUPPLY_EXT_PROP_CHARGE_PORT,
#endif
#if defined(CONFIG_WIRELESS_TX_MODE)
	POWER_SUPPLY_EXT_PROP_WIRELESS_TX_AVG_CURR,
#endif
#if defined(CONFIG_DIRECT_CHARGING)
	POWER_SUPPLY_EXT_PROP_CURRENT_MEASURE,
	POWER_SUPPLY_EXT_PROP_DIRECT_CHARGER_MODE,
	POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED_DC,
	POWER_SUPPLY_EXT_PROP_DIRECT_DONE,
	POWER_SUPPLY_EXT_PROP_DIRECT_FIXED_PDO,
	POWER_SUPPLY_EXT_PROP_DIRECT_WDT_CONTROL,
	POWER_SUPPLY_EXT_PROP_DIRECT_VOLTAGE_MAX,
	POWER_SUPPLY_EXT_PROP_DIRECT_CURRENT_MAX,
	POWER_SUPPLY_EXT_PROP_DIRECT_FLOAT_MAX,
	POWER_SUPPLY_EXT_PROP_DIRECT_ADC_CTRL,
	POWER_SUPPLY_EXT_PROP_DIRECT_POWER_TYPE,
	POWER_SUPPLY_EXT_PROP_DIRECT_HV_PDO,
	POWER_SUPPLY_EXT_PROP_DIRECT_HAS_APDO,
	POWER_SUPPLY_EXT_PROP_DIRECT_TA_ALERT,
	POWER_SUPPLY_EXT_PROP_DIRECT_CLEAR_ERR,
	POWER_SUPPLY_EXT_PROP_CHANGE_CHARGING_SOURCE,
	POWER_SUPPLY_EXT_PROP_DIRECT_SEND_UVDM,
#endif
	POWER_SUPPLY_EXT_PROP_SRCCAP,
	POWER_SUPPLY_EXT_PROP_CHARGE_BOOST,
	POWER_SUPPLY_EXT_PROP_WPC_EN,
	POWER_SUPPLY_EXT_PROP_WPC_EN_MST,
};

enum rx_device_type {
	NO_DEV = 0,
	OTHER_DEV,
	SS_GEAR,
	SS_PHONE,
	SS_BUDS,
};

enum sec_battery_usb_conf {
	USB_CURRENT_UNCONFIGURED = 100,
	USB_CURRENT_HIGH_SPEED = 475,
	USB_CURRENT_SUPER_SPEED = 875,
};

enum power_supply_ext_health {
	POWER_SUPPLY_HEALTH_VSYS_OVP = POWER_SUPPLY_HEALTH_MAX,
	POWER_SUPPLY_HEALTH_VBAT_OVP,
	POWER_SUPPLY_HEALTH_DC_ERR,
};

enum sec_battery_cable {
	SEC_BATTERY_CABLE_UNKNOWN = 0,
	SEC_BATTERY_CABLE_NONE,			/* 1 */
	SEC_BATTERY_CABLE_PREPARE_TA,		/* 2 */
	SEC_BATTERY_CABLE_TA,			/* 3 */
	SEC_BATTERY_CABLE_USB,			/* 4 */
	SEC_BATTERY_CABLE_USB_CDP,		/* 5 */
	SEC_BATTERY_CABLE_9V_TA,		/* 6 */
	SEC_BATTERY_CABLE_9V_ERR,		/* 7 */
	SEC_BATTERY_CABLE_9V_UNKNOWN,		/* 8 */
	SEC_BATTERY_CABLE_12V_TA,		/* 9 */
	SEC_BATTERY_CABLE_WIRELESS,		/* 10 */
	SEC_BATTERY_CABLE_HV_WIRELESS,		/* 11 */
	SEC_BATTERY_CABLE_PMA_WIRELESS,		/* 12 */
	SEC_BATTERY_CABLE_WIRELESS_PACK,	/* 13 */
	SEC_BATTERY_CABLE_WIRELESS_HV_PACK,	/* 14 */
	SEC_BATTERY_CABLE_WIRELESS_STAND,	/* 15 */
	SEC_BATTERY_CABLE_WIRELESS_HV_STAND,	/* 16 */
	SEC_BATTERY_CABLE_QC20,			/* 17 */
	SEC_BATTERY_CABLE_QC30,			/* 18 */
	SEC_BATTERY_CABLE_PDIC,			/* 19 */
	SEC_BATTERY_CABLE_UARTOFF,		/* 20 */
	SEC_BATTERY_CABLE_OTG,			/* 21 */
	SEC_BATTERY_CABLE_LAN_HUB,		/* 22 */
	SEC_BATTERY_CABLE_POWER_SHARING,	/* 23 */
	SEC_BATTERY_CABLE_HMT_CONNECTED,	/* 24 */
	SEC_BATTERY_CABLE_HMT_CHARGE,		/* 25 */
	SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT,	/* 26 */
	SEC_BATTERY_CABLE_WIRELESS_VEHICLE,		/* 27 */
	SEC_BATTERY_CABLE_WIRELESS_HV_VEHICLE,	/* 28 */
	SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV,	/* 29 */
	SEC_BATTERY_CABLE_TIMEOUT,	        /* 30 */
	SEC_BATTERY_CABLE_SMART_OTG,            /* 31 */
	SEC_BATTERY_CABLE_SMART_NOTG,           /* 32 */
	SEC_BATTERY_CABLE_WIRELESS_TX,		/* 33 */
	SEC_BATTERY_CABLE_HV_WIRELESS_20,	/* 34 */
	SEC_BATTERY_CABLE_HV_WIRELESS_20_LIMIT,	/* 35 */
	SEC_BATTERY_CABLE_WIRELESS_FAKE, /* 36 */
	SEC_BATTERY_CABLE_PREPARE_WIRELESS_20,	/* 37 */
	SEC_BATTERY_CABLE_PDIC_APDO,		/* 38 */
	SEC_BATTERY_CABLE_MAX,			/* 39 */
};

enum sec_battery_voltage_mode {
	/* average voltage */
	SEC_BATTERY_VOLTAGE_AVERAGE = 0,
	/* open circuit voltage */
	SEC_BATTERY_VOLTAGE_OCV,
};

enum sec_battery_current_mode {
	/* uA */
	SEC_BATTERY_CURRENT_UA = 0,
	/* mA */
	SEC_BATTERY_CURRENT_MA,
};

enum sec_battery_capacity_mode {
	/* designed capacity */
	SEC_BATTERY_CAPACITY_DESIGNED = 0,
	/* absolute capacity by fuel gauge */
	SEC_BATTERY_CAPACITY_ABSOLUTE,
	/* temperary capacity in the time */
	SEC_BATTERY_CAPACITY_TEMPERARY,
	/* current capacity now */
	SEC_BATTERY_CAPACITY_CURRENT,
	/* cell aging information */
	SEC_BATTERY_CAPACITY_AGEDCELL,
	/* charge count */
	SEC_BATTERY_CAPACITY_CYCLE,
	/* full capacity rep */
	SEC_BATTERY_CAPACITY_FULL,
	/* QH capacity */
	SEC_BATTERY_CAPACITY_QH,
	/* vfsoc */
	SEC_BATTERY_CAPACITY_VFSOC,
};

enum sec_wireless_info_mode {
	SEC_WIRELESS_OTP_FIRM_RESULT = 0,
	SEC_WIRELESS_IC_REVISION,
	SEC_WIRELESS_IC_GRADE,
	SEC_WIRELESS_IC_CHIP_ID,
	SEC_WIRELESS_OTP_FIRM_VER_BIN,
	SEC_WIRELESS_OTP_FIRM_VER,
	SEC_WIRELESS_TX_FIRM_RESULT,
	SEC_WIRELESS_TX_FIRM_VER,
	SEC_TX_FIRMWARE,
	SEC_WIRELESS_OTP_FIRM_VERIFY,
	SEC_WIRELESS_MST_SWITCH_VERIFY,
};

enum sec_wireless_firm_update_mode {
	SEC_WIRELESS_RX_SDCARD_MODE = 0,
	SEC_WIRELESS_RX_BUILT_IN_MODE,
	SEC_WIRELESS_TX_ON_MODE,
	SEC_WIRELESS_TX_OFF_MODE,
	SEC_WIRELESS_RX_INIT,
	SEC_WIRELESS_RX_SPU_MODE,
};

enum sec_tx_sharing_mode {
	SEC_TX_OFF = 0,
	SEC_TX_STANDBY,
	SEC_TX_POWER_TRANSFER,
	SEC_TX_ERROR,
};

enum sec_wireless_auth_mode {
	WIRELESS_AUTH_WAIT = 0,
	WIRELESS_AUTH_START,
	WIRELESS_AUTH_SENT,
	WIRELESS_AUTH_RECEIVED,
	WIRELESS_AUTH_FAIL,
	WIRELESS_AUTH_PASS,
};

enum sec_wireless_vout_control_mode {
	WIRELESS_VOUT_OFF = 0,
	WIRELESS_VOUT_NORMAL_VOLTAGE,	/* 5V , reserved by factory */
	WIRELESS_VOUT_RESERVED,			/* 6V */
	WIRELESS_VOUT_HIGH_VOLTAGE,		/* 9V , reserved by factory */
	WIRELESS_VOUT_CC_CV_VOUT, /* 4 */
	WIRELESS_VOUT_CALL, /* 5 */
	WIRELESS_VOUT_5V, /* 6 */
	WIRELESS_VOUT_9V, /* 7 */
	WIRELESS_VOUT_10V, /* 8 */
	WIRELESS_VOUT_11V, /* 9 */
	WIRELESS_VOUT_12V, /* 10 */
	WIRELESS_VOUT_12_5V, /* 11 */
	WIRELESS_VOUT_5V_STEP, /* 12 */
	WIRELESS_VOUT_5_5V_STEP, /* 13 */
	WIRELESS_VOUT_9V_STEP, /* 14 */
	WIRELESS_VOUT_10V_STEP, /* 15 */
	WIRELESS_PAD_FAN_OFF,
	WIRELESS_PAD_FAN_ON,
	WIRELESS_PAD_LED_OFF,
	WIRELESS_PAD_LED_ON,
	WIRELESS_PAD_LED_DIMMING,
	WIRELESS_VRECT_ADJ_ON,
	WIRELESS_VRECT_ADJ_OFF,
	WIRELESS_VRECT_ADJ_ROOM_0,
	WIRELESS_VRECT_ADJ_ROOM_1,
	WIRELESS_VRECT_ADJ_ROOM_2,
	WIRELESS_VRECT_ADJ_ROOM_3,
	WIRELESS_VRECT_ADJ_ROOM_4,
	WIRELESS_VRECT_ADJ_ROOM_5,
	WIRELESS_CLAMP_ENABLE,
	WIRELESS_SLEEP_MODE_ENABLE,
	WIRELESS_SLEEP_MODE_DISABLE,
};

enum sec_wireless_tx_vout {
	WC_TX_VOUT_5_0V = 0,
	WC_TX_VOUT_5_5V,
	WC_TX_VOUT_6_0V,
	WC_TX_VOUT_6_5V,
	WC_TX_VOUT_7_0V,
	WC_TX_VOUT_7_5V,
	WC_TX_VOUT_8_0V,
	WC_TX_VOUT_8_5V,
	WC_TX_VOUT_9_0V,
	WC_TX_VOUT_OFF=100,
};

enum sec_wireless_pad_mode {
	SEC_WIRELESS_PAD_NONE = 0,
	SEC_WIRELESS_PAD_WPC,
	SEC_WIRELESS_PAD_WPC_HV,
	SEC_WIRELESS_PAD_WPC_PACK,
	SEC_WIRELESS_PAD_WPC_PACK_HV,
	SEC_WIRELESS_PAD_WPC_STAND,
	SEC_WIRELESS_PAD_WPC_STAND_HV,
	SEC_WIRELESS_PAD_PMA,
	SEC_WIRELESS_PAD_VEHICLE,
	SEC_WIRELESS_PAD_VEHICLE_HV,
	SEC_WIRELESS_PAD_PREPARE_HV,
	SEC_WIRELESS_PAD_TX,
	SEC_WIRELESS_PAD_WPC_PREPARE_HV_20,
	SEC_WIRELESS_PAD_WPC_HV_20,
	SEC_WIRELESS_PAD_WPC_DUO_HV_20_LIMIT,	
	SEC_WIRELESS_PAD_FAKE,
};

enum sec_wireless_pad_id {
	WC_PAD_ID_UNKNOWN	= 0x00,
	/* 0x01~1F : Single Port */
	WC_PAD_ID_SNGL_NOBLE = 0x10,
	WC_PAD_ID_SNGL_VEHICLE,
	WC_PAD_ID_SNGL_MINI,
	WC_PAD_ID_SNGL_ZERO,
	WC_PAD_ID_SNGL_DREAM,
	/* 0x20~2F : Multi Port */
	/* 0x30~3F : Stand Type */
	WC_PAD_ID_STAND_HERO = 0x30,
	WC_PAD_ID_STAND_DREAM,
	/* 0x40~4F : External Battery Pack */
	WC_PAD_ID_EXT_BATT_PACK = 0x40,
	WC_PAD_ID_EXT_BATT_PACK_TA,
	/* 0x50~6F : Reserved */
	WC_PAD_ID_MAX = 0x6F,
};

enum sec_wireless_rx_power_list {
	SEC_WIRELESS_RX_POWER_5W = 0,
	SEC_WIRELESS_RX_POWER_7_5W,
	SEC_WIRELESS_RX_POWER_12W,
	SEC_WIRELESS_RX_POWER_15W,
	SEC_WIRELESS_RX_POWER_17_5W,
	SEC_WIRELESS_RX_POWER_20W,
	SEC_WIRELESS_RX_POWER_MAX,
};

enum sec_wireless_rx_power_class_list {
	SEC_WIRELESS_RX_POWER_CLASS_1 = 1,	/* 4.5W ~ 7.5W */
	SEC_WIRELESS_RX_POWER_CLASS_2, 		/* 7.6W ~ 12W */
	SEC_WIRELESS_RX_POWER_CLASS_3,		/* 12.1W ~ 20W */
	SEC_WIRELESS_RX_POWER_CLASS_4,		/* reserved */
	SEC_WIRELESS_RX_POWER_CLASS_5,		/* reserved */
};

enum sec_battery_temp_control_source {
	TEMP_CONTROL_SOURCE_NONE = 0,
	TEMP_CONTROL_SOURCE_BAT_THM,
	TEMP_CONTROL_SOURCE_CHG_THM,
	TEMP_CONTROL_SOURCE_WPC_THM,
	TEMP_CONTROL_SOURCE_USB_THM,
};

/* ADC type */
enum sec_battery_adc_type {
	/* NOT using this ADC channel */
	SEC_BATTERY_ADC_TYPE_NONE = 0,
	/* ADC in AP */
	SEC_BATTERY_ADC_TYPE_AP,
	/* ADC by additional IC */
	SEC_BATTERY_ADC_TYPE_IC,
	SEC_BATTERY_ADC_TYPE_NUM
};

enum sec_battery_adc_channel {
	SEC_BAT_ADC_CHANNEL_CABLE_CHECK = 0,
	SEC_BAT_ADC_CHANNEL_BAT_CHECK,
	SEC_BAT_ADC_CHANNEL_TEMP,
	SEC_BAT_ADC_CHANNEL_TEMP_AMBIENT,
	SEC_BAT_ADC_CHANNEL_FULL_CHECK,
	SEC_BAT_ADC_CHANNEL_VOLTAGE_NOW,
	SEC_BAT_ADC_CHANNEL_CHG_TEMP,
	SEC_BAT_ADC_CHANNEL_INBAT_VOLTAGE,
	SEC_BAT_ADC_CHANNEL_DISCHARGING_CHECK,
	SEC_BAT_ADC_CHANNEL_DISCHARGING_NTC,
	SEC_BAT_ADC_CHANNEL_WPC_TEMP,
	SEC_BAT_ADC_CHANNEL_SLAVE_CHG_TEMP,
	SEC_BAT_ADC_CHANNEL_USB_TEMP,
	SEC_BAT_ADC_CHANNEL_NUM,
};

enum sec_battery_charge_mode {
	SEC_BAT_CHG_MODE_CHARGING = 0, /* buck, chg on */
	SEC_BAT_CHG_MODE_CHARGING_OFF,
	SEC_BAT_CHG_MODE_BUCK_OFF, /* buck, chg off */
//	SEC_BAT_CHG_MODE_BUCK_ON,
	SEC_BAT_CHG_MODE_OTG_ON,
	SEC_BAT_CHG_MODE_OTG_OFF,
	SEC_BAT_CHG_MODE_UNO_ON,
	SEC_BAT_CHG_MODE_UNO_OFF,
	SEC_BAT_CHG_MODE_UNO_ONLY,
	SEC_BAT_CHG_MODE_MAX,
};

/* charging mode */
enum sec_battery_charging_mode {
	/* no charging */
	SEC_BATTERY_CHARGING_NONE = 0,
	/* 1st charging */
	SEC_BATTERY_CHARGING_1ST,
	/* 2nd charging */
	SEC_BATTERY_CHARGING_2ND,
	/* recharging */
	SEC_BATTERY_CHARGING_RECHARGING,
};

/* POWER_SUPPLY_EXT_PROP_MEASURE_SYS */
enum sec_battery_measure_sys {
	SEC_BATTERY_ISYS_MA = 0,
	SEC_BATTERY_ISYS_UA,
	SEC_BATTERY_ISYS_AVG_MA,
	SEC_BATTERY_ISYS_AVG_UA,
	SEC_BATTERY_VSYS,
};

/* POWER_SUPPLY_EXT_PROP_MEASURE_INPUT */
enum sec_battery_measure_input {
	SEC_BATTERY_IIN_MA = 0,
	SEC_BATTERY_IIN_UA,
	SEC_BATTERY_VBYP,
	SEC_BATTERY_VIN_MA,
	SEC_BATTERY_VIN_UA,
};

enum sec_battery_wpc_en_ctrl {
	WPC_EN_SYSFS = 0x1,
	WPC_EN_CCIC = 0x2,
	WPC_EN_CHARGING = 0x4,
	WPC_EN_TX = 0x8,
	WPC_EN_MST = 0x10,
	WPC_EN_FW = 0x20,
};

/* tx_event */
#define BATT_TX_EVENT_WIRELESS_TX_STATUS		0x00000001
#define BATT_TX_EVENT_WIRELESS_RX_CONNECT		0x00000002
#define BATT_TX_EVENT_WIRELESS_TX_FOD			0x00000004
#define BATT_TX_EVENT_WIRELESS_TX_HIGH_TEMP	0x00000008
#define BATT_TX_EVENT_WIRELESS_RX_UNSAFE_TEMP	0x00000010
#define BATT_TX_EVENT_WIRELESS_RX_CHG_SWITCH	0x00000020
#define BATT_TX_EVENT_WIRELESS_RX_CS100			0x00000040
#define BATT_TX_EVENT_WIRELESS_TX_OTG_ON		0x00000080
#define BATT_TX_EVENT_WIRELESS_TX_LOW_TEMP		0x00000100
#define BATT_TX_EVENT_WIRELESS_TX_SOC_DRAIN		0x00000200
#define BATT_TX_EVENT_WIRELESS_TX_CRITICAL_EOC	0x00000400
#define BATT_TX_EVENT_WIRELESS_TX_CAMERA_ON		0x00000800
#define BATT_TX_EVENT_WIRELESS_TX_OCP			0x00001000
#define BATT_TX_EVENT_WIRELESS_TX_MISALIGN      0x00002000
#define BATT_TX_EVENT_WIRELESS_TX_ETC           0x00004000
#define BATT_TX_EVENT_WIRELESS_TX_RETRY			0x00008000
#define BATT_TX_EVENT_WIRELESS_ALL_MASK			0x0000ffff
#define BATT_TX_EVENT_WIRELESS_TX_ERR			(BATT_TX_EVENT_WIRELESS_TX_FOD | BATT_TX_EVENT_WIRELESS_TX_HIGH_TEMP | \
	BATT_TX_EVENT_WIRELESS_RX_UNSAFE_TEMP | BATT_TX_EVENT_WIRELESS_RX_CHG_SWITCH | BATT_TX_EVENT_WIRELESS_RX_CS100 | \
	BATT_TX_EVENT_WIRELESS_TX_OTG_ON | BATT_TX_EVENT_WIRELESS_TX_LOW_TEMP | BATT_TX_EVENT_WIRELESS_TX_SOC_DRAIN | \
	BATT_TX_EVENT_WIRELESS_TX_CRITICAL_EOC | BATT_TX_EVENT_WIRELESS_TX_CAMERA_ON | BATT_TX_EVENT_WIRELESS_TX_OCP | BATT_TX_EVENT_WIRELESS_TX_MISALIGN | BATT_TX_EVENT_WIRELESS_TX_ETC)

#define SEC_BAT_ERROR_CAUSE_NONE		0x0000
#define SEC_BAT_ERROR_CAUSE_FG_INIT_FAIL	0x0001
#define SEC_BAT_ERROR_CAUSE_I2C_FAIL		0xFFFFFFFF

#define SEC_BAT_TX_RETRY_NONE			0x0000
#define SEC_BAT_TX_RETRY_MISALIGN		0x0001
#define SEC_BAT_TX_RETRY_CAMERA			0x0002
#define SEC_BAT_TX_RETRY_CALL			0x0004
#define SEC_BAT_TX_RETRY_MIX_TEMP		0x0008
#define SEC_BAT_TX_RETRY_HIGH_TEMP		0x0010
#define SEC_BAT_TX_RETRY_LOW_TEMP		0x0020
#define SEC_BAT_TX_RETRY_OCP			0x0040

/* ext_event */
#define BATT_EXT_EVENT_NONE			0x00000000
#define BATT_EXT_EVENT_CAMERA		0x00000001
#define BATT_EXT_EVENT_DEX			0x00000002
#define BATT_EXT_EVENT_CALL			0x00000004

struct sec_bat_adc_api {
	bool (*init)(struct platform_device *);
	bool (*exit)(void);
	int (*read)(unsigned int);
};
#define sec_bat_adc_api_t struct sec_bat_adc_api

/* monitor activation */
enum sec_battery_polling_time_type {
	/* same order with power supply status */
	SEC_BATTERY_POLLING_TIME_BASIC = 0,
	SEC_BATTERY_POLLING_TIME_CHARGING,
	SEC_BATTERY_POLLING_TIME_DISCHARGING,
	SEC_BATTERY_POLLING_TIME_NOT_CHARGING,
	SEC_BATTERY_POLLING_TIME_SLEEP,
};

enum sec_battery_monitor_polling {
	/* polling work queue */
	SEC_BATTERY_MONITOR_WORKQUEUE,
	/* alarm polling */
	SEC_BATTERY_MONITOR_ALARM,
	/* timer polling (NOT USE) */
	SEC_BATTERY_MONITOR_TIMER,
};
#define sec_battery_monitor_polling_t \
	enum sec_battery_monitor_polling

/* full charged check : POWER_SUPPLY_PROP_STATUS */
enum sec_battery_full_charged {
	SEC_BATTERY_FULLCHARGED_NONE = 0,
	/* current check by ADC */
	SEC_BATTERY_FULLCHARGED_ADC,
	/* fuel gauge current check */
	SEC_BATTERY_FULLCHARGED_FG_CURRENT,
	/* time check */
	SEC_BATTERY_FULLCHARGED_TIME,
	/* SOC check */
	SEC_BATTERY_FULLCHARGED_SOC,
	/* charger GPIO, NO additional full condition */
	SEC_BATTERY_FULLCHARGED_CHGGPIO,
	/* charger interrupt, NO additional full condition */
	SEC_BATTERY_FULLCHARGED_CHGINT,
	/* charger power supply property, NO additional full condition */
	SEC_BATTERY_FULLCHARGED_CHGPSY,
};

#if defined(CONFIG_BATTERY_SAMSUNG_MHS)
enum charging_port {
	PORT_NONE = 0,
	MAIN_PORT,
	SUB_PORT,
};
#endif

enum ta_alert_mode {
	OCP_NONE = 0,
	OCP_DETECT,
	OCP_WA_ACTIVE,
};

#define sec_battery_full_charged_t \
	enum sec_battery_full_charged

/* full check condition type (can be used overlapped) */
#define sec_battery_full_condition_t unsigned int
/* SEC_BATTERY_FULL_CONDITION_NOTIMEFULL
  * full-charged by absolute-timer only in high voltage
  */
#define SEC_BATTERY_FULL_CONDITION_NOTIMEFULL	1
/* SEC_BATTERY_FULL_CONDITION_NOSLEEPINFULL
  * do not set polling time as sleep polling time in full-charged
  */
#define SEC_BATTERY_FULL_CONDITION_NOSLEEPINFULL	2
/* SEC_BATTERY_FULL_CONDITION_SOC
  * use capacity for full-charged check
  */
#define SEC_BATTERY_FULL_CONDITION_SOC		4
/* SEC_BATTERY_FULL_CONDITION_VCELL
  * use VCELL for full-charged check
  */
#define SEC_BATTERY_FULL_CONDITION_VCELL	8
/* SEC_BATTERY_FULL_CONDITION_AVGVCELL
  * use average VCELL for full-charged check
  */
#define SEC_BATTERY_FULL_CONDITION_AVGVCELL	16
/* SEC_BATTERY_FULL_CONDITION_OCV
  * use OCV for full-charged check
  */
#define SEC_BATTERY_FULL_CONDITION_OCV		32

/* recharge check condition type (can be used overlapped) */
#define sec_battery_recharge_condition_t unsigned int
/* SEC_BATTERY_RECHARGE_CONDITION_SOC
  * use capacity for recharging check
  */
#define SEC_BATTERY_RECHARGE_CONDITION_SOC		1
/* SEC_BATTERY_RECHARGE_CONDITION_AVGVCELL
  * use average VCELL for recharging check
  */
#define SEC_BATTERY_RECHARGE_CONDITION_AVGVCELL		2
/* SEC_BATTERY_RECHARGE_CONDITION_VCELL
  * use VCELL for recharging check
  */
#define SEC_BATTERY_RECHARGE_CONDITION_VCELL		4

/* battery check : POWER_SUPPLY_PROP_PRESENT */
enum sec_battery_check {
	/* No Check for internal battery */
	SEC_BATTERY_CHECK_NONE,
	/* by ADC */
	SEC_BATTERY_CHECK_ADC,
	/* by callback function (battery certification by 1 wired)*/
	SEC_BATTERY_CHECK_CALLBACK,
	/* by PMIC */
	SEC_BATTERY_CHECK_PMIC,
	/* by fuel gauge */
	SEC_BATTERY_CHECK_FUELGAUGE,
	/* by charger */
	SEC_BATTERY_CHECK_CHARGER,
	/* by interrupt (use check_battery_callback() to check battery) */
	SEC_BATTERY_CHECK_INT,
};
#define sec_battery_check_t \
	enum sec_battery_check

/* OVP, UVLO check : POWER_SUPPLY_PROP_HEALTH */
enum sec_battery_ovp_uvlo {
	/* by callback function */
	SEC_BATTERY_OVP_UVLO_CALLBACK,
	/* by PMIC polling */
	SEC_BATTERY_OVP_UVLO_PMICPOLLING,
	/* by PMIC interrupt */
	SEC_BATTERY_OVP_UVLO_PMICINT,
	/* by charger polling */
	SEC_BATTERY_OVP_UVLO_CHGPOLLING,
	/* by charger interrupt */
	SEC_BATTERY_OVP_UVLO_CHGINT,
};
#define sec_battery_ovp_uvlo_t \
	enum sec_battery_ovp_uvlo

/* thermal source */
enum sec_battery_thermal_source {
	/* by fuel gauge */
	SEC_BATTERY_THERMAL_SOURCE_FG,
	/* by external source */
	SEC_BATTERY_THERMAL_SOURCE_CALLBACK,
	/* by ADC */
	SEC_BATTERY_THERMAL_SOURCE_ADC,
	/* by charger */
	SEC_BATTERY_THERMAL_SOURCE_CHG_ADC,
};
#define sec_battery_thermal_source_t \
	enum sec_battery_thermal_source

/* temperature check type */
enum sec_battery_temp_check {
	SEC_BATTERY_TEMP_CHECK_NONE = 0,	/* no temperature check */
	SEC_BATTERY_TEMP_CHECK_ADC, 		/* by ADC value */
	SEC_BATTERY_TEMP_CHECK_TEMP,		/* by temperature */
	SEC_BATTERY_TEMP_CHECK_FAKE,		/* by a fake temperature */
};
#define sec_battery_temp_check_t \
	enum sec_battery_temp_check

/* cable check (can be used overlapped) */
#define sec_battery_cable_check_t unsigned int
/* SEC_BATTERY_CABLE_CHECK_NOUSBCHARGE
  * for USB cable in tablet model,
  * status is stuck into discharging,
  * but internal charging logic is working
  */
#define SEC_BATTERY_CABLE_CHECK_NOUSBCHARGE		1
/* SEC_BATTERY_CABLE_CHECK_NOINCOMPATIBLECHARGE
  * for incompatible charger
  * (Not compliant to USB specification,
  *  cable type is SEC_BATTERY_CABLE_UNKNOWN),
  * do NOT charge and show message to user
  * (only for VZW)
  */
#define SEC_BATTERY_CABLE_CHECK_NOINCOMPATIBLECHARGE	2
/* SEC_BATTERY_CABLE_CHECK_PSY
  * check cable by power supply set_property
  */
#define SEC_BATTERY_CABLE_CHECK_PSY			4
/* SEC_BATTERY_CABLE_CHECK_INT
  * check cable by interrupt
  */
#define SEC_BATTERY_CABLE_CHECK_INT			8
/* SEC_BATTERY_CABLE_CHECK_CHGINT
  * check cable by charger interrupt
  */
#define SEC_BATTERY_CABLE_CHECK_CHGINT			16
/* SEC_BATTERY_CABLE_CHECK_POLLING
  * check cable by GPIO polling
  */
#define SEC_BATTERY_CABLE_CHECK_POLLING			32

/* check cable source (can be used overlapped) */
#define sec_battery_cable_source_t unsigned int
/* SEC_BATTERY_CABLE_SOURCE_EXTERNAL
 * already given by external argument
 */
#define	SEC_BATTERY_CABLE_SOURCE_EXTERNAL	1
/* SEC_BATTERY_CABLE_SOURCE_CALLBACK
 * by callback (MUIC, USB switch)
 */
#define	SEC_BATTERY_CABLE_SOURCE_CALLBACK	2
/* SEC_BATTERY_CABLE_SOURCE_ADC
 * by ADC
 */
#define	SEC_BATTERY_CABLE_SOURCE_ADC		4

/* capacity calculation type (can be used overlapped) */
#define sec_fuelgauge_capacity_type_t int
/* SEC_FUELGAUGE_CAPACITY_TYPE_RESET
  * use capacity information to reset fuel gauge
  * (only for driver algorithm, can NOT be set by user)
  */
#define SEC_FUELGAUGE_CAPACITY_TYPE_RESET	(-1)
/* SEC_FUELGAUGE_CAPACITY_TYPE_RAW
  * use capacity information from fuel gauge directly
  */
#define SEC_FUELGAUGE_CAPACITY_TYPE_RAW		0x1
/* SEC_FUELGAUGE_CAPACITY_TYPE_SCALE
  * rescale capacity by scaling, need min and max value for scaling
  */
#define SEC_FUELGAUGE_CAPACITY_TYPE_SCALE	0x2
/* SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE
  * change only maximum capacity dynamically
  * to keep time for every SOC unit
  */
#define SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE	0x4
/* SEC_FUELGAUGE_CAPACITY_TYPE_ATOMIC
  * change capacity value by only -1 or +1
  * no sudden change of capacity
  */
#define SEC_FUELGAUGE_CAPACITY_TYPE_ATOMIC	0x8
/* SEC_FUELGAUGE_CAPACITY_TYPE_SKIP_ABNORMAL
  * skip current capacity value
  * if it is abnormal value
  */
#define SEC_FUELGAUGE_CAPACITY_TYPE_SKIP_ABNORMAL	0x10

#define SEC_FUELGAUGE_CAPACITY_TYPE_CAPACITY_POINT	0x20

#define SEC_FUELGAUGE_CAPACITY_TYPE_LOST_SOC	0x40

/* charger function settings (can be used overlapped) */
#define sec_charger_functions_t unsigned int
/* SEC_CHARGER_NO_GRADUAL_CHARGING_CURRENT
 * disable gradual charging current setting
 * SUMMIT:AICL, MAXIM:regulation loop
 */
#define SEC_CHARGER_NO_GRADUAL_CHARGING_CURRENT		1

/* SEC_CHARGER_MINIMUM_SIOP_CHARGING_CURRENT
 * charging current should be over than USB charging current
 */
#define SEC_CHARGER_MINIMUM_SIOP_CHARGING_CURRENT	2

/**
 * struct sec_bat_adc_table_data - adc to temperature table for sec battery
 * driver
 * @adc: adc value
 * @temperature: temperature(C) * 10
 */
struct sec_bat_adc_table_data {
	int adc;
	int data;
};
#define sec_bat_adc_table_data_t \
	struct sec_bat_adc_table_data

struct sec_bat_adc_region {
	int min;
	int max;
};
#define sec_bat_adc_region_t \
	struct sec_bat_adc_region

struct sec_charging_current {
	unsigned int input_current_limit;
	unsigned int fast_charging_current;
};

#define sec_charging_current_t \
	struct sec_charging_current

#if defined(CONFIG_BATTERY_AGE_FORECAST)
struct sec_age_data {
	unsigned int cycle;
	unsigned int float_voltage;
	unsigned int recharge_condition_vcell;
	unsigned int full_condition_vcell;
	unsigned int full_condition_soc;
#if defined(CONFIG_STEP_CHARGING)
	unsigned int step_charging_condition;
#endif
};

#define sec_age_data_t \
	struct sec_age_data
#endif

typedef struct {
	unsigned int cycle;
	unsigned int asoc;
} battery_health_condition;

struct sec_wireless_rx_power_info {
	unsigned int vout;
	unsigned int input_current_limit;
	unsigned int fast_charging_current;
	unsigned int ttf_charge_current;
	unsigned int wireless_power_class;
	unsigned int rx_power;
};

#define sec_wireless_rx_power_info_t \
	struct sec_wireless_rx_power_info

struct sec_battery_platform_data {
	/* NO NEED TO BE CHANGED */
	/* callback functions */
	void (*initial_check)(void);
	void (*monitor_additional_check)(void);
	bool (*bat_gpio_init)(void);
	bool (*fg_gpio_init)(void);
	bool (*is_lpm)(void);
	bool (*check_jig_status)(void);
	bool (*is_interrupt_cable_check_possible)(int);
	int (*check_cable_callback)(void);
	int (*get_cable_from_extended_cable_type)(int);
	bool (*cable_switch_check)(void);
	bool (*cable_switch_normal)(void);
	bool (*check_cable_result_callback)(int);
	bool (*check_battery_callback)(void);
	bool (*check_battery_result_callback)(void);
	int (*ovp_uvlo_callback)(void);
	bool (*ovp_uvlo_result_callback)(int);
	bool (*fuelalert_process)(bool);
	bool (*get_temperature_callback)(
			enum power_supply_property,
			union power_supply_propval*);

	/* ADC API for each ADC type */
	sec_bat_adc_api_t adc_api[SEC_BATTERY_ADC_TYPE_NUM];
	/* ADC region by power supply type
	 * ADC region should be exclusive
	 */
	sec_bat_adc_region_t *cable_adc_value;
	/* charging current for type (0: not use) */
	sec_charging_current_t *charging_current;
	sec_wireless_rx_power_info_t *wireless_power_info;
	unsigned int *polling_time;
	char *chip_vendor;
	unsigned int temp_adc_type;
	/* NO NEED TO BE CHANGED */
	unsigned int pre_afc_input_current;
	unsigned int pre_wc_afc_input_current;
	unsigned int store_mode_afc_input_current;
	unsigned int store_mode_hv_wireless_input_current;
	unsigned int prepare_ta_delay;

	char *pmic_name;

	/* battery */
	char *vendor;
	int technology;
	int battery_type;
	void *battery_data;

	int bat_gpio_ta_nconnected;
	/* 1 : active high, 0 : active low */
	int bat_polarity_ta_nconnected;
	int bat_irq;
	int bat_irq_gpio; /* BATT_INT(BAT_ID detecting) */
	unsigned long bat_irq_attr;
	int jig_irq;
	unsigned long jig_irq_attr;
	sec_battery_cable_check_t cable_check_type;
	sec_battery_cable_source_t cable_source_type;

	bool use_LED;				/* use charging LED */

	/* flag for skipping the swelling mode */
	bool swelling_mode_skip_in_high_temp;
	/* sustaining event after deactivated (second) */
	unsigned int event_waiting_time;

	/* battery swelling */
	int swelling_high_temp_block;
	int swelling_high_temp_recov;
	int swelling_wc_high_temp_block;
	int swelling_wc_high_temp_recov;
	int swelling_low_temp_block_1st;
	int swelling_low_temp_recov_1st;
	int swelling_low_temp_block_2nd;
	int swelling_low_temp_recov_2nd;
	int swelling_low_temp_block_3rd;
	int swelling_low_temp_recov_3rd;
	int swelling_low_temp_block_4th;
	int swelling_low_temp_recov_4th;
	int swelling_low_temp_block_5th;
	int swelling_low_temp_recov_5th;
	unsigned int swelling_low_temp_current;
	unsigned int swelling_low_temp_current_2nd;
	unsigned int swelling_low_temp_current_3rd;
	unsigned int swelling_low_temp_current_4th;
	unsigned int swelling_low_temp_current_5th;
	unsigned int swelling_low_temp_topoff;
	unsigned int swelling_high_temp_current;
	unsigned int swelling_high_temp_topoff;
	unsigned int swelling_wc_high_temp_current;
	unsigned int swelling_wc_low_temp_current;
	unsigned int swelling_wc_low_temp_current_2nd;
	unsigned int swelling_wc_low_temp_current_3rd;

	unsigned int swelling_normal_float_voltage;
	unsigned int swelling_drop_float_voltage;
	unsigned int swelling_high_rechg_voltage;
	unsigned int swelling_low_rechg_voltage;
	unsigned int swelling_low_rechg_thr;
	unsigned int swelling_drop_voltage_condition;

#if defined(CONFIG_CALC_TIME_TO_FULL)
	unsigned int ttf_hv_12v_charge_current;
	unsigned int ttf_hv_charge_current;
	unsigned int ttf_hv_12v_wireless_charge_current;
	unsigned int ttf_hv_wireless_charge_current;
	unsigned int ttf_wireless_charge_current;
	unsigned int ttf_dc25_charge_current;
	unsigned int ttf_dc45_charge_current;
#endif

#if defined(CONFIG_STEP_CHARGING)
	/* step charging */
	unsigned int *step_charging_condition;
	unsigned int *step_charging_condition_curr;
	unsigned int *step_charging_current;
	unsigned int *step_charging_float_voltage;
#if defined(CONFIG_DIRECT_CHARGING)
	unsigned int *dc_step_chg_cond_vol;
	unsigned int **dc_step_chg_cond_soc;
	unsigned int *dc_step_chg_cond_iin;
	int dc_step_chg_iin_check_cnt;

	unsigned int **dc_step_chg_val_iout;
	unsigned int **dc_step_chg_val_vfloat;
#endif
#endif

	/* Monitor setting */
	sec_battery_monitor_polling_t polling_type;
	/* for initial check */
	unsigned int monitor_initial_count;

	/* Battery check */
	sec_battery_check_t battery_check_type;
	/* how many times do we need to check battery */
	unsigned int check_count;
	/* ADC */
	/* battery check ADC maximum value */
	unsigned int check_adc_max;
	/* battery check ADC minimum value */
	unsigned int check_adc_min;

	/* OVP/UVLO check */
	sec_battery_ovp_uvlo_t ovp_uvlo_check_type;

	sec_battery_thermal_source_t thermal_source;
	sec_battery_thermal_source_t usb_thermal_source; /* To confirm the usb temperature */
	sec_battery_thermal_source_t chg_thermal_source; /* To confirm the charger temperature */
	sec_battery_thermal_source_t wpc_thermal_source; /* To confirm the wpc temperature */
	sec_battery_thermal_source_t slave_thermal_source; /* To confirm the slave charger temperature */
	sec_battery_thermal_source_t sub_bat_thermal_source; /* To confirm the sub battery temperature */
#if defined(CONFIG_DIRECT_CHARGING)
	sec_battery_thermal_source_t dchg_thermal_source; /* To confirm the charger temperature */
#endif

	/*
	 * inbat_adc_table
	 * in-battery voltage check for table models:
	 * To read real battery voltage with Jig cable attached,
	 * dedicated hw pin & conversion table of adc-voltage are required
	 */
	sec_bat_adc_table_data_t *temp_adc_table;
	sec_bat_adc_table_data_t *temp_amb_adc_table;
	sec_bat_adc_table_data_t *usb_temp_adc_table;
	sec_bat_adc_table_data_t *chg_temp_adc_table;
	sec_bat_adc_table_data_t *wpc_temp_adc_table;
	sec_bat_adc_table_data_t *slave_chg_temp_adc_table;
	sec_bat_adc_table_data_t *inbat_adc_table;
	sec_bat_adc_table_data_t *sub_bat_temp_adc_table;
#if defined(CONFIG_DIRECT_CHARGING)
	sec_bat_adc_table_data_t *dchg_temp_adc_table;
#endif

	unsigned int temp_adc_table_size;
	unsigned int temp_amb_adc_table_size;
	unsigned int usb_temp_adc_table_size;
	unsigned int chg_temp_adc_table_size;
	unsigned int wpc_temp_adc_table_size;
	unsigned int slave_chg_temp_adc_table_size;
	unsigned int inbat_adc_table_size;
	unsigned int sub_bat_temp_adc_table_size;
#if defined(CONFIG_DIRECT_CHARGING)
	unsigned int dchg_temp_adc_table_size;
#endif

	sec_battery_temp_check_t temp_check_type;
	unsigned int temp_check_count;
	sec_battery_temp_check_t usb_temp_check_type;
	sec_battery_temp_check_t usb_temp_check_type_backup;
	sec_battery_temp_check_t chg_temp_check_type;
	sec_battery_temp_check_t wpc_temp_check_type;
	sec_battery_temp_check_t slave_chg_temp_check_type;
	unsigned int sub_bat_temp_check_type;
#if defined(CONFIG_DIRECT_CHARGING)
	sec_battery_temp_check_t dchg_temp_check_type;
#endif

	unsigned int inbat_voltage;

	/*
	 * limit can be ADC value or Temperature
	 * depending on temp_check_type
	 * temperature should be temp x 10 (0.1 degree)
	 */
	int temp_highlimit_threshold_normal;
	int temp_highlimit_threshold_normal_backup;
	int temp_highlimit_recovery_normal;
	int temp_high_threshold_normal;
	int temp_high_recovery_normal;
	int temp_low_threshold_normal;
	int temp_low_recovery_normal;
	int temp_highlimit_threshold_lpm;
	int temp_highlimit_recovery_lpm;
	int temp_high_threshold_lpm;
	int temp_high_recovery_lpm;
	int temp_low_threshold_lpm;
	int temp_low_recovery_lpm;
	int wpc_high_threshold_normal;
	int wpc_high_recovery_normal;
	int wpc_low_threshold_normal;
	int wpc_low_recovery_normal;
	int tx_high_threshold;
	int tx_high_recovery;
	int tx_low_threshold;
	int tx_low_recovery;
	int chg_12v_high_temp;
	int chg_high_temp;
	int chg_high_temp_recovery;
	int dchg_high_temp;
	int dchg_high_temp_recovery;
	int dchg_high_batt_temp;
	int dchg_high_batt_temp_recovery;
	unsigned int chg_charging_limit_current;
	unsigned int chg_input_limit_current;
#if defined(CONFIG_DIRECT_CHARGING)
	unsigned int dchg_charging_limit_current;
	unsigned int dchg_input_limit_current;
#endif
	unsigned int wpc_temp_control_source;
	unsigned int wpc_temp_lcd_on_control_source;
	int wpc_high_temp;
	int wpc_high_temp_recovery;
	unsigned int wpc_input_limit_current;
	unsigned int wpc_charging_limit_current;
	int wpc_lcd_on_high_temp;
	int wpc_lcd_on_high_temp_rec;
	unsigned int wpc_lcd_on_input_limit_current;
	unsigned int sleep_mode_limit_current;
	unsigned int wc_full_input_limit_current;
	unsigned int max_charging_current;
	unsigned int max_charging_charge_power;
	int mix_high_temp;
	int mix_high_chg_temp;
	int mix_high_temp_recovery;
	unsigned int charging_limit_by_tx_check; /* check limited charging current during wireless power sharing with cable charging */
	unsigned int charging_limit_current_by_tx;
	unsigned int wpc_input_limit_by_tx_check; /* check limited wpc input current with tx device */
	unsigned int wpc_input_limit_current_by_tx;

	/* If these is NOT full check type or NONE full check type,
	 * it is skipped
	 */
	/* 1st full check */
	sec_battery_full_charged_t full_check_type;
	/* 2nd full check */
	sec_battery_full_charged_t full_check_type_2nd;
	unsigned int full_check_count;
	int chg_gpio_full_check;
	/* 1 : active high, 0 : active low */
	int chg_polarity_full_check;
	sec_battery_full_condition_t full_condition_type;
	unsigned int full_condition_soc;
	unsigned int full_condition_vcell;
	unsigned int full_condition_avgvcell;
	unsigned int full_condition_ocv;

	unsigned int recharge_check_count;
	sec_battery_recharge_condition_t recharge_condition_type;
	unsigned int recharge_condition_soc;
	unsigned int recharge_condition_avgvcell;
	unsigned int recharge_condition_vcell;

	/* for absolute timer (second) */
	unsigned long charging_total_time;
	/* for recharging timer (second) */
	unsigned long recharging_total_time;
	/* reset charging for abnormal malfunction (0: not use) */
	unsigned long charging_reset_time;
	unsigned int hv_charging_total_time;
	unsigned int normal_charging_total_time;
	unsigned int usb_charging_total_time;

	/* fuel gauge */
	char *fuelgauge_name;
	int fg_irq;
	unsigned long fg_irq_attr;
	/* fuel alert SOC (-1: not use) */
	int fuel_alert_soc;
	/* fuel alert can be repeated */
	bool repeated_fuelalert;
	sec_fuelgauge_capacity_type_t capacity_calculation_type;
	/* soc should be soc x 10 (0.1% degree)
	 * only for scaling
	 */
	int capacity_max;
	int capacity_max_hv;

	int capacity_max_margin;
	int capacity_min;

	unsigned int store_mode_charging_max;
	unsigned int store_mode_charging_min;
	/* charger */
	char *charger_name;
	char *fgsrc_switch_name;
	bool support_fgsrc_change;

	/* wireless charger */
	char *wireless_charger_name;
	int wireless_cc_cv;
	int wpc_det;
	int wpc_en;
	int mst_pwr_en;

	int chg_gpio_en;
	int chg_irq;
	unsigned long chg_irq_attr;
	/* float voltage (mV) */
	unsigned int chg_float_voltage;
	unsigned int chg_float_voltage_conv;

#if defined(CONFIG_BATTERY_AGE_FORECAST)
	int num_age_step;
	int age_step;
	int age_data_length;
	sec_age_data_t* age_data;
#endif
	battery_health_condition* health_condition;

	int siop_input_limit_current;
	int siop_charging_limit_current;
	int siop_hv_input_limit_current;
	int siop_hv_input_limit_current_2nd;
	int siop_hv_charging_limit_current;
	int siop_hv_12v_input_limit_current;
	int siop_hv_12v_charging_limit_current;
#if defined(CONFIG_DIRECT_CHARGING)
	int siop_apdo_input_limit_current;
	int siop_apdo_charging_limit_current;
#endif

	int siop_wireless_input_limit_current;
	int siop_wireless_charging_limit_current;
	int siop_hv_wireless_input_limit_current;
	int siop_hv_wireless_charging_limit_current;
	int wireless_otg_input_current;
	int wc_hero_stand_cc_cv;
	int wc_hero_stand_cv_current;
	int wc_hero_stand_hv_cv_current;

	int default_input_current;
	int default_charging_current;
	int default_usb_input_current;
	int default_usb_charging_current;
	unsigned int default_wc20_input_current;
	unsigned int default_wc20_charging_current;
	int max_input_voltage;
	int max_input_current;
	int pre_afc_work_delay;
	int pre_wc_afc_work_delay;

	unsigned int rp_current_rp1;
	unsigned int rp_current_rp2;
	unsigned int rp_current_rp3;
	unsigned int rp_current_rdu_rp3;
	unsigned int rp_current_abnormal_rp3;

	sec_charger_functions_t chg_functions_setting;

	bool fake_capacity;

	bool dis_auto_shipmode_temp_ctrl;

	/* tx power sharging */
	unsigned int tx_stop_capacity;

	unsigned int battery_full_capacity;
#if defined(CONFIG_BATTERY_CISD)	
	unsigned int cisd_cap_high_thr;
	unsigned int cisd_cap_low_thr;
	unsigned int cisd_cap_limit;
	unsigned int max_voltage_thr;
	unsigned int cisd_alg_index;
	unsigned int *ignore_cisd_index;
	unsigned int *ignore_cisd_index_d;
#endif

	/* ADC setting */
	unsigned int adc_check_count;

	unsigned int full_check_current_1st;
	unsigned int full_check_current_2nd;

	unsigned int pd_charging_charge_power;
	unsigned int nv_charge_power;

	unsigned int expired_time;
	unsigned int recharging_expired_time;
	int standard_curr;

	unsigned int tx_minduty_5V;
	unsigned int tx_minduty_default;

	unsigned int tx_uno_iout;
	unsigned int tx_mfc_iout_gear;
	unsigned int tx_mfc_iout_phone;
	unsigned int tx_mfc_iout_phone_5v;
	unsigned int tx_mfc_iout_lcd_on;

	/* ADC type for each channel */
	unsigned int adc_type[];
};

struct sec_charger_platform_data {
	bool (*chg_gpio_init)(void);

	/* charging current for type (0: not use) */
	sec_charging_current_t *charging_current;

	/* wirelss charger */
	char *wireless_charger_name;
	int wireless_cc_cv;

	int vbus_ctrl_gpio;
	int chg_gpio_en;
	/* float voltage (mV) */
	int chg_float_voltage;
	int irq_gpio;
	int chg_irq;
	unsigned long chg_irq_attr;
	unsigned int chg_ocp_current;
	unsigned int chg_ocp_dtc;
	unsigned int topoff_time;

	/* otg_en setting */
	int otg_en;

	/* OVP/UVLO check */
	sec_battery_ovp_uvlo_t ovp_uvlo_check_type;
	/* 1st full check */
	sec_battery_full_charged_t full_check_type;
	/* 2nd full check */
	sec_battery_full_charged_t full_check_type_2nd;

	sec_charger_functions_t chg_functions_setting;
};

struct sec_fuelgauge_platform_data {
	bool (*fg_gpio_init)(void);
	bool (*check_jig_status)(void);
	int (*check_cable_callback)(void);
	bool (*fuelalert_process)(bool);

	/* charging current for type (0: not use) */
	unsigned int full_check_current_1st;
	unsigned int full_check_current_2nd;

	int jig_irq;
	int jig_gpio;
	bool jig_low_active;
	unsigned long jig_irq_attr;

	sec_battery_thermal_source_t thermal_source;

	int fg_irq;
	unsigned long fg_irq_attr;
	/* fuel alert SOC (-1: not use) */
	int fuel_alert_soc;
	int fuel_alert_vol;
	/* fuel alert can be repeated */
	bool repeated_fuelalert;
	sec_fuelgauge_capacity_type_t capacity_calculation_type;
	/* soc should be soc x 10 (0.1% degree)
	 * only for scaling
	 */
	int capacity_max;
	int capacity_max_hv;
	int capacity_max_margin;
	int capacity_min;

#if defined(CONFIG_BATTERY_AGE_FORECAST)
	unsigned int full_condition_soc;
#endif
};

#define sec_battery_platform_data_t \
	struct sec_battery_platform_data

#define sec_charger_platform_data_t \
	struct sec_charger_platform_data

#define sec_fuelgauge_platform_data_t \
	struct sec_fuelgauge_platform_data

static inline struct power_supply *get_power_supply_by_name(char *name)
{
	if (!name)
		return (struct power_supply *)NULL;
	else
		return power_supply_get_by_name(name);
}

#define psy_do_property(name, function, property, value) \
({	\
	struct power_supply *psy;	\
	int ret = 0;	\
	psy = get_power_supply_by_name((name));	\
	if (!psy) {	\
		pr_err("%s: Fail to "#function" psy (%s)\n",	\
			__func__, (name));	\
		value.intval = 0;	\
		ret = -ENOENT;	\
	} else {	\
		if (psy->desc->function##_property != NULL) { \
			ret = psy->desc->function##_property(psy, \
				(enum power_supply_property) (property), &(value)); \
			if (ret < 0) {	\
				pr_err("%s: Fail to %s "#function" (%d=>%d)\n", \
						__func__, name, (property), ret);	\
				value.intval = 0;	\
			}	\
		} else {	\
			ret = -ENOSYS;	\
		}	\
		power_supply_put(psy);		\
	}					\
	ret;	\
})

#define get_battery_data(driver)	\
	(((struct battery_data_t *)(driver)->pdata->battery_data)	\
	[(driver)->pdata->battery_type])

#define is_hv_wireless_pad_type(cable_type) ( \
	cable_type == SEC_WIRELESS_PAD_WPC_HV || \
	cable_type == SEC_WIRELESS_PAD_WPC_PACK_HV || \
	cable_type == SEC_WIRELESS_PAD_WPC_STAND_HV || \
	cable_type == SEC_WIRELESS_PAD_VEHICLE_HV || \
	cable_type == SEC_WIRELESS_PAD_WPC_HV_20 || \
	cable_type == SEC_WIRELESS_PAD_WPC_DUO_HV_20_LIMIT)

#define is_nv_wireless_pad_type(cable_type) ( \
	cable_type == SEC_WIRELESS_PAD_WPC || \
	cable_type == SEC_WIRELESS_PAD_WPC_PACK || \
	cable_type == SEC_WIRELESS_PAD_WPC_STAND || \
	cable_type == SEC_WIRELESS_PAD_PMA || \
	cable_type == SEC_WIRELESS_PAD_VEHICLE || \
	cable_type == SEC_WIRELESS_PAD_WPC_PREPARE_HV_20 || \
	cable_type == SEC_WIRELESS_PAD_PREPARE_HV)

#define is_wireless_pad_type(cable_type) \
	(is_hv_wireless_pad_type(cable_type) || is_nv_wireless_pad_type(cable_type))

#define is_hv_wireless_type(cable_type) ( \
	cable_type == SEC_BATTERY_CABLE_HV_WIRELESS || \
	cable_type == SEC_BATTERY_CABLE_HV_WIRELESS_ETX || \
	cable_type == SEC_BATTERY_CABLE_WIRELESS_HV_STAND || \
	cable_type == SEC_BATTERY_CABLE_HV_WIRELESS_20 || \
	cable_type == SEC_BATTERY_CABLE_HV_WIRELESS_20_LIMIT || \
	cable_type == SEC_BATTERY_CABLE_WIRELESS_HV_VEHICLE || \
	cable_type == SEC_BATTERY_CABLE_WIRELESS_HV_PACK)

#define is_nv_wireless_type(cable_type)	( \
	cable_type == SEC_BATTERY_CABLE_WIRELESS || \
	cable_type == SEC_BATTERY_CABLE_PMA_WIRELESS || \
	cable_type == SEC_BATTERY_CABLE_WIRELESS_PACK || \
	cable_type == SEC_BATTERY_CABLE_WIRELESS_STAND || \
	cable_type == SEC_BATTERY_CABLE_WIRELESS_VEHICLE || \
	cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV || \
	cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_20 || \
	cable_type == SEC_BATTERY_CABLE_WIRELESS_TX)

#define is_wireless_type(cable_type) \
	(is_hv_wireless_type(cable_type) || is_nv_wireless_type(cable_type))

#define is_not_wireless_type(cable_type) ( \
	cable_type != SEC_BATTERY_CABLE_WIRELESS && \
	cable_type != SEC_BATTERY_CABLE_PMA_WIRELESS && \
	cable_type != SEC_BATTERY_CABLE_WIRELESS_PACK && \
	cable_type != SEC_BATTERY_CABLE_WIRELESS_STAND && \
	cable_type != SEC_BATTERY_CABLE_HV_WIRELESS && \
	cable_type != SEC_BATTERY_CABLE_HV_WIRELESS_ETX && \
	cable_type != SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV && \
	cable_type != SEC_BATTERY_CABLE_WIRELESS_HV_STAND && \
	cable_type != SEC_BATTERY_CABLE_WIRELESS_VEHICLE && \
	cable_type != SEC_BATTERY_CABLE_WIRELESS_HV_VEHICLE && \
	cable_type != SEC_BATTERY_CABLE_WIRELESS_TX && \
	cable_type != SEC_BATTERY_CABLE_PREPARE_WIRELESS_20 && \
	cable_type != SEC_BATTERY_CABLE_HV_WIRELESS_20 && \
	cable_type != SEC_BATTERY_CABLE_HV_WIRELESS_20_LIMIT && \
	cable_type != SEC_BATTERY_CABLE_WIRELESS_HV_PACK)

#define is_wired_type(cable_type) \
	(is_not_wireless_type(cable_type) && (cable_type != SEC_BATTERY_CABLE_NONE) && \
	(cable_type != SEC_BATTERY_CABLE_OTG))

#define is_hv_qc_wire_type(cable_type) ( \
	cable_type == SEC_BATTERY_CABLE_QC20 || \
	cable_type == SEC_BATTERY_CABLE_QC30)

#define is_hv_afc_wire_type(cable_type) ( \
	cable_type == SEC_BATTERY_CABLE_9V_ERR || \
	cable_type == SEC_BATTERY_CABLE_9V_TA || \
	cable_type == SEC_BATTERY_CABLE_9V_UNKNOWN || \
	cable_type == SEC_BATTERY_CABLE_12V_TA)

#define is_hv_wire_9v_type(cable_type) ( \
	cable_type == SEC_BATTERY_CABLE_9V_ERR || \
	cable_type == SEC_BATTERY_CABLE_9V_TA || \
	cable_type == SEC_BATTERY_CABLE_9V_UNKNOWN || \
	cable_type == SEC_BATTERY_CABLE_QC20)

#define is_hv_wire_12v_type(cable_type) ( \
	cable_type == SEC_BATTERY_CABLE_12V_TA || \
	cable_type == SEC_BATTERY_CABLE_QC30)

#define is_hv_wire_type(cable_type) ( \
	is_hv_afc_wire_type(cable_type) || is_hv_qc_wire_type(cable_type))

#define is_nocharge_type(cable_type) ( \
	cable_type == SEC_BATTERY_CABLE_NONE || \
	cable_type == SEC_BATTERY_CABLE_OTG || \
	cable_type == SEC_BATTERY_CABLE_POWER_SHARING)

#define is_slate_mode(battery) ((battery->current_event & SEC_BAT_CURRENT_EVENT_SLATE) \
		== SEC_BAT_CURRENT_EVENT_SLATE)

#define is_pd_wire_type(cable_type) ( \
	cable_type == SEC_BATTERY_CABLE_PDIC || \
	cable_type == SEC_BATTERY_CABLE_PDIC_APDO)

#define is_pd_apdo_wire_type(cable_type) ( \
	cable_type == SEC_BATTERY_CABLE_PDIC_APDO)
#define is_pd_fpdo_wire_type(cable_type) ( \
	cable_type == SEC_BATTERY_CABLE_PDIC)
#endif /* __SEC_CHARGING_COMMON_H */
