/*
 * sec_cisd.h
 * Samsung Mobile Charger Header
 *
 * Copyright (C) 2015 Samsung Electronics, Inc.
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
#ifndef __SEC_CISD_H
#define __SEC_CISD_H __FILE__

#define CISD_STATE_NONE			0x00
#define CISD_STATE_OVER_VOLTAGE		0x01

#define is_cisd_check_type(cable_type) ( \
	cable_type == SEC_BATTERY_CABLE_TA || \
	cable_type == SEC_BATTERY_CABLE_9V_TA || \
	cable_type == SEC_BATTERY_CABLE_9V_UNKNOWN || \
	cable_type == SEC_BATTERY_CABLE_9V_ERR || \
	cable_type == SEC_BATTERY_CABLE_PDIC)

enum cisd_data {
	CISD_DATA_RESET_ALG = 0,

	CISD_DATA_ALG_INDEX,
	CISD_DATA_FULL_COUNT,
	CISD_DATA_CAP_MAX,
	CISD_DATA_CAP_MIN,
	CISD_DATA_RECHARGING_COUNT,
	CISD_DATA_VALERT_COUNT,
	CISD_DATA_CYCLE,
	CISD_DATA_WIRE_COUNT,
	CISD_DATA_WIRELESS_COUNT,
	CISD_DATA_HIGH_TEMP_SWELLING,

	CISD_DATA_LOW_TEMP_SWELLING,
	CISD_DATA_WC_HIGH_TEMP_SWELLING,
	CISD_DATA_SWELLING_FULL_CNT,
	CISD_DATA_SWELLING_RECOVERY_CNT,
	CISD_DATA_AICL_COUNT,
	CISD_DATA_BATT_TEMP_MAX,
	CISD_DATA_BATT_TEMP_MIN,
	CISD_DATA_CHG_TEMP_MAX,
	CISD_DATA_CHG_TEMP_MIN,
	CISD_DATA_WPC_TEMP_MAX,

	CISD_DATA_WPC_TEMP_MIN,
	CISD_DATA_USB_TEMP_MAX,
	CISD_DATA_USB_TEMP_MIN,
	CISD_DATA_CHG_BATT_TEMP_MAX,
	CISD_DATA_CHG_BATT_TEMP_MIN,
	CISD_DATA_CHG_CHG_TEMP_MAX,
	CISD_DATA_CHG_CHG_TEMP_MIN,
	CISD_DATA_CHG_WPC_TEMP_MAX,
	CISD_DATA_CHG_WPC_TEMP_MIN,
	CISD_DATA_CHG_USB_TEMP_MAX,

	CISD_DATA_CHG_USB_TEMP_MIN,
	CISD_DATA_USB_OVERHEAT_CHARGING,
	CISD_DATA_UNSAFETY_VOLTAGE,
	CISD_DATA_UNSAFETY_TEMPERATURE,
	CISD_DATA_SAFETY_TIMER,
	CISD_DATA_VSYS_OVP,
	CISD_DATA_VBAT_OVP,
	CISD_DATA_USB_OVERHEAT_RAPID_CHANGE,
	CISD_DATA_ASOC,
	CISD_DATA_USB_OVERHEAT_ALONE,

	CISD_DATA_CAP_NOM,

	CISD_DATA_MAX,
};

enum cisd_data_per_day {
	CISD_DATA_FULL_COUNT_PER_DAY = CISD_DATA_MAX,

	CISD_DATA_CAP_MAX_PER_DAY,
	CISD_DATA_CAP_MIN_PER_DAY,
	CISD_DATA_RECHARGING_COUNT_PER_DAY,
	CISD_DATA_VALERT_COUNT_PER_DAY,
	CISD_DATA_WIRE_COUNT_PER_DAY,
	CISD_DATA_WIRELESS_COUNT_PER_DAY,
	CISD_DATA_HIGH_TEMP_SWELLING_PER_DAY,
	CISD_DATA_LOW_TEMP_SWELLING_PER_DAY,
	CISD_DATA_WC_HIGH_TEMP_SWELLING_PER_DAY,
	CISD_DATA_SWELLING_FULL_CNT_PER_DAY,

	CISD_DATA_SWELLING_RECOVERY_CNT_PER_DAY,
	CISD_DATA_AICL_COUNT_PER_DAY,
	CISD_DATA_BATT_TEMP_MAX_PER_DAY,
	CISD_DATA_BATT_TEMP_MIN_PER_DAY,
	CISD_DATA_CHG_TEMP_MAX_PER_DAY,
	CISD_DATA_CHG_TEMP_MIN_PER_DAY,
	CISD_DATA_WPC_TEMP_MAX_PER_DAY,
	CISD_DATA_WPC_TEMP_MIN_PER_DAY,
	CISD_DATA_USB_TEMP_MAX_PER_DAY,
	CISD_DATA_USB_TEMP_MIN_PER_DAY,

	CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY,
	CISD_DATA_CHG_BATT_TEMP_MIN_PER_DAY,
	CISD_DATA_CHG_CHG_TEMP_MAX_PER_DAY,
	CISD_DATA_CHG_CHG_TEMP_MIN_PER_DAY,
	CISD_DATA_CHG_WPC_TEMP_MAX_PER_DAY,
	CISD_DATA_CHG_WPC_TEMP_MIN_PER_DAY,
	CISD_DATA_CHG_USB_TEMP_MAX_PER_DAY,
	CISD_DATA_CHG_USB_TEMP_MIN_PER_DAY,
	CISD_DATA_USB_OVERHEAT_CHARGING_PER_DAY,
	CISD_DATA_UNSAFE_VOLTAGE_PER_DAY,

	CISD_DATA_UNSAFE_TEMPERATURE_PER_DAY,
	CISD_DATA_SAFETY_TIMER_PER_DAY,
	CISD_DATA_VSYS_OVP_PER_DAY,
	CISD_DATA_VBAT_OVP_PER_DAY,
	CISD_DATA_USB_OVERHEAT_RAPID_CHANGE_PER_DAY,
	CISD_DATA_BUCK_OFF_PER_DAY,
	CISD_DATA_USB_OVERHEAT_ALONE_PER_DAY,
	CISD_DATA_DROP_VALUE_PER_DAY,

	CISD_DATA_MAX_PER_DAY,
};

enum {
	WC_DATA_INDEX = 0,
	WC_UNKNOWN,
	WC_SNGL_NOBLE,
	WC_SNGL_VEHICLE,
	WC_SNGL_MINI,
	WC_SNGL_ZERO,
	WC_SNGL_DREAM,
	WC_STAND_HERO,
	WC_STAND_DREAM,
	WC_EXT_PACK,
	WC_EXT_PACK_TA,

	WC_DATA_MAX,
};

enum {
	CISD_CABLE_TA = 0,
	CISD_CABLE_AFC,
	CISD_CABLE_AFC_FAIL,
	CISD_CABLE_QC,
	CISD_CABLE_QC_FAIL,
	CISD_CABLE_PD,
	CISD_CABLE_PD_HIGH,
	CISD_CABLE_HV_WC_20,

	CISD_CABLE_TYPE_MAX,
};

enum {
	TX_ON = 0,
	TX_OTHER,
	TX_GEAR,
	TX_PHONE,
	TX_BUDS,
	TX_DATA_MAX,
};

enum {
	EVENT_DC_ERR = 0,
	EVENT_TA_OCP_DET,
	EVENT_TA_OCP_ON,
	EVENT_DATA_MAX,
};

extern const char *cisd_data_str[];
extern const char *cisd_data_str_d[];
extern const char *cisd_cable_data_str[];
extern const char *cisd_tx_data_str[];
extern const char *cisd_event_data_str[];

#define PAD_INDEX_STRING	"COUNT"
#define PAD_JSON_STRING		"PAD_0x"
#define MAX_PAD_ID			0xFF
#define MAX_CHARGER_POWER	100
#define POWER_JSON_STRING	"POWER_"
#define POWER_COUNT_JSON_STRING "COUNT"

struct pad_data {
	unsigned int id;
	unsigned int count;

	struct pad_data* prev;
	struct pad_data* next;
};

struct power_data {
	unsigned int power;
	unsigned int count;

	struct power_data* prev;
	struct power_data* next;
};

struct cisd {
	unsigned int cisd_alg_index;
	unsigned int state;

	unsigned int ab_vbat_max_count;
	unsigned int ab_vbat_check_count;
	unsigned int max_voltage_thr;

	/* Big Data Field */
	int data[CISD_DATA_MAX_PER_DAY];
	int cable_data[CISD_CABLE_TYPE_MAX];
	unsigned int tx_data[TX_DATA_MAX];
	unsigned int event_data[EVENT_DATA_MAX];

	struct mutex padlock;
	struct mutex powerlock;
	struct pad_data* pad_array;
	struct power_data* power_array;
	unsigned int pad_count;
	unsigned int power_count;
};

extern struct cisd *gcisd;
static inline void set_cisd_data(int type, int value)
{
	if (gcisd && (type >= CISD_DATA_RESET_ALG && type < CISD_DATA_MAX_PER_DAY))
		gcisd->data[type] = value;
}
static inline int get_cisd_data(int type)
{
	if (!gcisd || (type < CISD_DATA_RESET_ALG || type >= CISD_DATA_MAX_PER_DAY))
		return -1;

	return gcisd->data[type];
}
static inline void increase_cisd_count(int type)
{
	if (gcisd && (type >= CISD_DATA_RESET_ALG && type < CISD_DATA_MAX_PER_DAY))
		gcisd->data[type]++;
}

void init_cisd_pad_data(struct cisd *cisd);
void count_cisd_pad_data(struct cisd *cisd, unsigned int pad_id);

void init_cisd_power_data(struct cisd* cisd);
void count_cisd_power_data(struct cisd* cisd, int power);
#endif /* __SEC_CISD_H */
