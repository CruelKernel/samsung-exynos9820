/*
 *  sec_battery.c
 *  Samsung Mobile Battery Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "include/sec_battery.h"
#include "include/sec_battery_sysfs.h"
#include "include/sec_battery_dt.h"

#include <linux/sec_ext.h>
#include <linux/sec_debug.h>

#if defined(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif

bool sleep_mode = false;
bool batt_boot_complete = false;

static enum power_supply_property sec_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TEMP_AMBIENT,
#if defined(CONFIG_FUELGAUGE_MAX77705)
	POWER_SUPPLY_PROP_POWER_NOW,
	POWER_SUPPLY_PROP_POWER_AVG,
#endif
#if defined(CONFIG_CALC_TIME_TO_FULL)
	POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
#endif
	POWER_SUPPLY_PROP_CHARGE_COUNTER_SHADOW,
	POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL,
	POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
};

static enum power_supply_property sec_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property sec_wireless_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_PRESENT,
};

static enum power_supply_property sec_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TEMP,
};

static enum power_supply_property sec_ps_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
};

static char *supply_list[] = {
	"battery",
};

char *sec_cable_type[SEC_BATTERY_CABLE_MAX] = {
	"UNKNOWN",					/* 0 */
	"NONE",						/* 1 */
	"PREAPARE_TA",				/* 2 */
	"TA",						/* 3 */
	"USB",						/* 4 */
	"USB_CDP",					/* 5 */
	"9V_TA",					/* 6 */
	"9V_ERR",					/* 7 */
	"9V_UNKNOWN",				/* 8 */
	"12V_TA",					/* 9 */
	"WC",						/* 10 */
	"HV_WC",					/* 11 */
	"PMA_WC",					/* 12 */
	"WC_PACK",					/* 13 */
	"WC_HV_PACK",				/* 14 */
	"WC_STAND",					/* 15 */
	"WC_HV_STAND",				/* 16 */
	"OC20",						/* 17 */
	"QC30",						/* 18 */
	"PDIC",						/* 19 */
	"UARTOFF",					/* 20 */
	"OTG",						/* 21 */
	"LAN_HUB",					/* 22 */
	"POWER_SHARGING",			/* 23 */
	"HMT_CONNECTED",			/* 24 */
	"HMT_CHARGE",				/* 25 */
	"HV_TA_CHG_LIMIT",			/* 26 */
	"WC_VEHICLE",				/* 27 */
	"WC_HV_VEHICLE",			/* 28 */
	"WC_HV_PREPARE",			/* 29 */
	"TIMEOUT",					/* 30 */
	"SMART_OTG",				/* 31 */
	"SMART_NOTG",				/* 32 */
	"WC_TX",					/* 33 */
	"HV_WC_20",					/* 34 */
	"HV_WC_20_LIMIT",			/* 35 */
	"WC_FAKE",					/* 36 */
	"HV_WC_20_PREPARE",			/* 37 */
	"PDIC_APDO",				/* 38 */
};

char *sec_bat_charging_mode_str[] = {
	"None",
	"Normal",
	"Additional",
	"Re-Charging",
	"ABS"
};

char *sec_bat_status_str[] = {
	"Unknown",
	"Charging",
	"Discharging",
	"Not-charging",
	"Full"
};

char *sec_bat_health_str[] = {
	"Unknown",
	"Good",
	"Overheat",
	"Warm",
	"Dead",
	"OverVoltage",
	"UnspecFailure",
	"Cold",
	"Cool",
	"WatchdogTimerExpire",
	"SafetyTimerExpire",
	"UnderVoltage",
	"OverheatLimit",
	"VsysOVP",
	"VbatOVP",
#if defined(CONFIG_DIRECT_CHARGING)
	"DCErr",
#endif
};

char *sec_bat_charge_mode_str[] = {
	"Charging-On",
	"Charging-Off",
	"Buck-Off",
};

char *sec_bat_rx_type_str[] = {
	"No Dev",
	"Other DEV",	
	"SS Gear",
	"SS Phone",
	"SS Buds",
};

char *vout_control_mode_str[] = {
	"Set VOUT Off",
	"Set VOUT NV",
	"Set Vout Rsv",
	"Set Vout HV",
	"Set Vout CV",
	"Set Vout Call",
	"Set Vout 5V",
	"Set Vout 9V",
	"Set Vout 10V",
	"Set Vout 11V",
	"Set Vout 12V",
	"Set Vout 12.5V",
	"Set Vout 5V Step",
	"Set Vout 5.5V Step",
	"Set Vout 9V Step",
	"Set Vout 10V Step",
};

char *swelling_mode_str[] = {
	"Swelling None",
	"Swelling Charging",
	"Swelling Full",
};
extern int bootmode;

bool mfc_fw_update;
EXPORT_SYMBOL(mfc_fw_update);
bool boot_complete;
EXPORT_SYMBOL(boot_complete);

#if defined(CONFIG_PREVENT_USB_CONN_OVERHEAT)
extern int muic_set_hiccup_mode(int on_off);
extern void pdic_manual_ccopen_request(int is_on);
#endif
extern int muic_afc_set_voltage(int vol);
extern int muic_hv_charger_disable(bool en);

void sec_bat_set_misc_event(struct sec_battery_info *battery,
	unsigned int misc_event_val, unsigned int misc_event_mask) {

	unsigned int temp = battery->misc_event;

	mutex_lock(&battery->misclock);

	battery->misc_event &= ~misc_event_mask;
	battery->misc_event |= misc_event_val;

	pr_info("%s: misc event before(0x%x), after(0x%x)\n",
		__func__, temp, battery->misc_event);

	mutex_unlock(&battery->misclock);

	if (battery->prev_misc_event != battery->misc_event) {
		cancel_delayed_work(&battery->misc_event_work);
		wake_lock(&battery->misc_event_wake_lock);
		queue_delayed_work(battery->monitor_wqueue,
			&battery->misc_event_work, 0);
	}
}

void sec_bat_set_tx_event(struct sec_battery_info *battery,
	unsigned int tx_event_val, unsigned int tx_event_mask) {

	unsigned int temp = battery->tx_event;

	mutex_lock(&battery->txeventlock);

	battery->tx_event &= ~tx_event_mask;
	battery->tx_event |= tx_event_val;

	pr_info("@Tx_Mode %s: val(0x%x), mask(0x%x), tx event before(0x%x), after(0x%x)\n",
		__func__, tx_event_val, tx_event_mask, temp, battery->tx_event);

	pr_info("@Tx_Mode %s: tx event before(0x%x), after(0x%x)\n",
		__func__, temp, battery->tx_event);

	if (temp != battery->tx_event) {
		/* Assure receiving tx_event to App for sleep case */
		wake_lock_timeout(&battery->tx_event_wake_lock, HZ * 2);
		power_supply_changed(battery->psy_bat);
	}
	mutex_unlock(&battery->txeventlock);
}

void sec_bat_set_current_event(struct sec_battery_info *battery,
			      unsigned int current_event_val, unsigned int current_event_mask)
{
	unsigned int temp = battery->current_event;

	mutex_lock(&battery->current_eventlock);

	battery->current_event &= ~current_event_mask;
	battery->current_event |= current_event_val;

	pr_info("%s: current event before(0x%x), after(0x%x)\n",
		__func__, temp, battery->current_event);

	mutex_unlock(&battery->current_eventlock);
}

void sec_bat_set_temp_control_test(struct sec_battery_info *battery,
			      bool temp_enable)
{
	if (temp_enable) {
		pr_info("%s : BATT_TEMP_CONTROL_TEST ENABLE\n", __func__);
		sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_TEMP_CTRL_TEST,
			SEC_BAT_CURRENT_EVENT_TEMP_CTRL_TEST);
		battery ->pdata->usb_temp_check_type_backup = battery->pdata->usb_temp_check_type;
		battery->pdata->usb_temp_check_type = 0;

		battery->pdata->temp_highlimit_threshold_normal_backup =
			battery->pdata->temp_highlimit_threshold_normal;
		battery->pdata->temp_highlimit_threshold_normal = 990;
	} else {
		pr_info("%s : BATT_TEMP_CONTROL_TEST END\n", __func__);
		sec_bat_set_current_event(battery, 0,
			SEC_BAT_CURRENT_EVENT_TEMP_CTRL_TEST);
		if (!battery ->pdata->usb_temp_check_type)
			battery ->pdata->usb_temp_check_type = battery->pdata->usb_temp_check_type_backup;

		if (battery->pdata->temp_highlimit_threshold_normal == 990)
			battery->pdata->temp_highlimit_threshold_normal =
				battery->pdata->temp_highlimit_threshold_normal_backup;			
	}
}

static void sec_bat_change_default_current(struct sec_battery_info *battery,
					int cable_type, int input, int output)
{
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	if(!battery->test_max_current)
#endif
		battery->pdata->charging_current[cable_type].input_current_limit = input;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
		if(!battery->test_charge_current)
#endif
		battery->pdata->charging_current[cable_type].fast_charging_current = output;
	pr_info("%s: cable_type: %d input: %d output: %d\n",
		__func__,
		cable_type,
		battery->pdata->charging_current[cable_type].input_current_limit,
		battery->pdata->charging_current[cable_type].fast_charging_current);
}

static int sec_bat_get_wireless_current(struct sec_battery_info *battery, int incurr)
{
	union power_supply_propval value = {0, };
	int is_otg_on = 0;

	psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, value);
	is_otg_on = value.intval;

	if (is_otg_on) {
		pr_info("%s: both wireless chg and otg recognized.\n", __func__);
		incurr = battery->pdata->wireless_otg_input_current;
	}

	/* 2. WPC_SLEEP_MODE */
	if (is_hv_wireless_type(battery->cable_type) && sleep_mode) {
		if(incurr > battery->pdata->sleep_mode_limit_current)
			incurr = battery->pdata->sleep_mode_limit_current;
		pr_info("%s sleep_mode =%d, chg_limit =%d, in_curr = %d \n", __func__,
			sleep_mode, battery->chg_limit, incurr);

		if(!battery->auto_mode) {
			/* send cmd once */
			battery->auto_mode = true;
			value.intval = WIRELESS_SLEEP_MODE_ENABLE;
			psy_do_property(battery->pdata->wireless_charger_name, set,
							POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
		}
	}

	/* 3. WPC_TEMP_MODE */
	if (is_wireless_type(battery->cable_type) && battery->chg_limit) {
		if ((battery->siop_level >= 100 && !battery->lcd_status) &&
				(incurr > battery->pdata->wpc_input_limit_current)) {
			if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_TX &&
				battery->pdata->wpc_input_limit_by_tx_check)
				incurr = battery->pdata->wpc_input_limit_current_by_tx;
			else
				incurr = battery->pdata->wpc_input_limit_current;
		} else if ((battery->siop_level < 100 || battery->lcd_status) &&
				(incurr > battery->pdata->wpc_lcd_on_input_limit_current))
			incurr = battery->pdata->wpc_lcd_on_input_limit_current;
	}

	/* 5. Full-Additional state */
	if (battery->status == POWER_SUPPLY_STATUS_FULL && battery->charging_mode == SEC_BATTERY_CHARGING_2ND) {
		if (incurr > battery->pdata->siop_hv_wireless_input_limit_current)
			incurr = battery->pdata->siop_hv_wireless_input_limit_current;
	}

	/* 6. Hero Stand Pad CV */
	if (battery->capacity >= battery->pdata->wc_hero_stand_cc_cv) {
		if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_STAND) {
			if (incurr > battery->pdata->wc_hero_stand_cv_current)
				incurr = battery->pdata->wc_hero_stand_cv_current;
		} else if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_HV_STAND) {
			if (battery->chg_limit &&
					incurr > battery->pdata->wc_hero_stand_cv_current) {
				incurr = battery->pdata->wc_hero_stand_cv_current;
			} else if (!battery->chg_limit &&
					incurr > battery->pdata->wc_hero_stand_hv_cv_current) {
				incurr = battery->pdata->wc_hero_stand_hv_cv_current;
			}
		}
	}

	/* 7. Full-None state && SIOP_LEVEL 100 */
	if ((battery->siop_level >= 100 && !battery->lcd_status) &&
		battery->status == POWER_SUPPLY_STATUS_FULL && battery->charging_mode == SEC_BATTERY_CHARGING_NONE) {
		incurr = battery->pdata->wc_full_input_limit_current;
	}

	return incurr;
}

static void sec_bat_get_charging_current_by_siop(struct sec_battery_info *battery,
		int *input_current, int *charging_current) {

	if (battery->siop_level < 100  &&
		!((battery->siop_level == 80) && is_wired_type(battery->cable_type))) {
		int max_charging_current;

		if (is_wireless_type(battery->cable_type)) {
			max_charging_current = 1000; /* 1 step(70) */
			if (battery->siop_level == 0) { /* 3 step(0) */
				max_charging_current = 0;
			} else if (battery->siop_level <= 10) { /* 2 step(10) */
				max_charging_current = 500;
			}
		}
#if defined(CONFIG_DIRECT_CHARGING)
		else if (is_pd_apdo_wire_type(battery->cable_type)) {
			max_charging_current = battery->pdata->siop_apdo_charging_limit_current;
		}
#endif
		else {
			max_charging_current = 1800; /* 1 step(70) */
		}

		/* do forced set charging current */
		if (*charging_current > max_charging_current)
			*charging_current = max_charging_current;

		if (is_nv_wireless_type(battery->cable_type)) {
			if (*input_current > battery->pdata->siop_wireless_input_limit_current)
				*input_current = battery->pdata->siop_wireless_input_limit_current;
			if (*charging_current > battery->pdata->siop_wireless_charging_limit_current)
				*charging_current = battery->pdata->siop_wireless_charging_limit_current;
		} else if (is_hv_wireless_type(battery->cable_type)) {
			if (*input_current > battery->pdata->siop_hv_wireless_input_limit_current)
				*input_current = battery->pdata->siop_hv_wireless_input_limit_current;
			if (*charging_current > battery->pdata->siop_hv_wireless_charging_limit_current)
				*charging_current = battery->pdata->siop_hv_wireless_charging_limit_current;
		} else if (is_hv_wire_type(battery->cable_type) && is_hv_wire_type(battery->wire_status)) {
			if (is_hv_wire_12v_type(battery->cable_type)) {
				if (*input_current > battery->pdata->siop_hv_12v_input_limit_current)
					*input_current = battery->pdata->siop_hv_12v_input_limit_current;
			} else {
				if (*input_current > battery->pdata->siop_hv_input_limit_current)
					*input_current = battery->pdata->siop_hv_input_limit_current;
				/* 2 step(0) for hv_wire_type */
				if (battery->siop_level == 0 &&
					*input_current > battery->pdata->siop_hv_input_limit_current_2nd)
					*input_current = battery->pdata->siop_hv_input_limit_current_2nd;
			}
#if defined(CONFIG_CCIC_NOTIFIER)
		} else if (is_pd_wire_type(battery->cable_type)) {
			if (*input_current > (60000 / battery->input_voltage))
				*input_current = 60000 / battery->input_voltage;
			/* 2 step(0) for PD type */
			if (battery->siop_level == 0 &&
				*input_current > battery->pdata->siop_hv_input_limit_current_2nd)
				*input_current = battery->pdata->siop_hv_input_limit_current_2nd;
#endif
		} else {
			if (*input_current > battery->pdata->siop_input_limit_current)
				*input_current = battery->pdata->siop_input_limit_current;
		}
	}

	pr_info("%s: incurr(%d), chgcurr(%d)\n", __func__, *input_current, *charging_current);
}

static void sec_bat_change_pdo(struct sec_battery_info *battery, int vol)
{
	int target_pd_index = 0;

	if (is_pd_wire_type(battery->wire_status)) {

		if (vol == SEC_INPUT_VOLTAGE_9V) {
			/* select PDO greater than 5V */
#if defined(CONFIG_PDIC_PD30)
			target_pd_index = battery->pd_list.num_fpdo - 1;
#else
			target_pd_index = battery->pd_list.max_pd_count - 1;
#endif
		} else {
			/* select 5V PDO */
			target_pd_index = 0;
		}

		if (target_pd_index < 0 || target_pd_index >= MAX_PDO_NUM) {
			pr_info("%s: target_pd_index is wrong: %d\n", __func__, target_pd_index);
			return;
		}

		pr_info("%s: target_pd_index: %d, now_pd_index: %d\n", __func__,
			target_pd_index, battery->pd_list.now_pd_index);

		if (target_pd_index != battery->pd_list.now_pd_index) {
			/* change input current before request new pdo if new pdo's input current is less than now */
#if defined(CONFIG_PDIC_PD30)
			if (battery->pd_list.pd_info[target_pd_index].max_current < battery->input_current) {
#else			
			if (battery->pd_list.pd_info[target_pd_index].input_current < battery->input_current) {
#endif
				union power_supply_propval value = {0, };
#if defined(CONFIG_PDIC_PD30)
				value.intval = battery->pd_list.pd_info[target_pd_index].max_current;
#else
				value.intval = battery->pd_list.pd_info[target_pd_index].input_current;
#endif
				battery->input_current = value.intval;
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SELECT_PDO,
					SEC_BAT_CURRENT_EVENT_SELECT_PDO);
				psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_PROP_CURRENT_MAX, value);
			}
			battery->pdic_ps_rdy = false;
			if (target_pd_index >= 0 && target_pd_index < MAX_PDO_NUM)
				select_pdo(battery->pd_list.pd_info[target_pd_index].pdo_index);
		}
	}
}

#if !defined(CONFIG_SEC_FACTORY)
static int sec_bat_get_temp_by_temp_control_source(struct sec_battery_info *battery,
	enum sec_battery_temp_control_source tcs)
{
	switch (tcs) {
	case TEMP_CONTROL_SOURCE_CHG_THM:
		return battery->chg_temp;
	case TEMP_CONTROL_SOURCE_USB_THM:
		return battery->usb_temp;
	case TEMP_CONTROL_SOURCE_WPC_THM:
		return battery->wpc_temp;
	case TEMP_CONTROL_SOURCE_NONE:
	case TEMP_CONTROL_SOURCE_BAT_THM:
	default:
		return battery->temperature;
	}
}

static int sec_bat_check_mix_temp(struct sec_battery_info *battery, int input_current)
{
	int chg_temp;

	if (battery->pdata->temp_check_type == SEC_BATTERY_TEMP_CHECK_NONE ||
		battery->pdata->chg_temp_check_type == SEC_BATTERY_TEMP_CHECK_NONE)
		return input_current;

#if defined(CONFIG_DIRECT_CHARGING)
	if (is_pd_apdo_wire_type(battery->wire_status) && battery->pd_list.now_isApdo)
		chg_temp = battery->dchg_temp;
	else
		chg_temp = battery->chg_temp;
#else
	chg_temp = battery->chg_temp;
#endif

	if (battery->siop_level >= 100 && !battery->lcd_status &&
		is_not_wireless_type(battery->cable_type)) {
		if ((!battery->mix_limit &&
				(battery->temperature >= battery->pdata->mix_high_temp) &&
				(chg_temp >= battery->pdata->mix_high_chg_temp)) ||
			(battery->mix_limit &&
				(battery->temperature > battery->pdata->mix_high_temp_recovery))) {
			int max_input_current =
				battery->pdata->full_check_current_1st + 50;

			/* inpu current = float voltage * (topoff_current_1st + 50mA(margin)) / (vbus_level * 0.9) */
			input_current = ((battery->pdata->chg_float_voltage / battery->pdata->chg_float_voltage_conv) * max_input_current) /
				(battery->input_voltage * 9) / 10;
			if (input_current > max_input_current)
				input_current = max_input_current;

			battery->mix_limit = true;
			/* skip other heating control */
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL,
						  SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL);

#if defined(CONFIG_WIRELESS_TX_MODE)
			if (battery->wc_tx_enable) {
				pr_info("%s: @Tx_Mode enter mix_temp_limit, TX mode should turn off \n", __func__);
				sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_HIGH_TEMP, BATT_TX_EVENT_WIRELESS_TX_HIGH_TEMP);
				battery->tx_retry_case |= SEC_BAT_TX_RETRY_MIX_TEMP;
				sec_wireless_set_tx_enable(battery, false);
			}
#endif
		} else if (battery->mix_limit) {
			battery->mix_limit = false;
			if (battery->tx_retry_case & SEC_BAT_TX_RETRY_MIX_TEMP) {
				pr_info("%s: @Tx_Mode recovery mix_temp_limit, TX mode should be retried \n", __func__);
				if ((battery->tx_retry_case & ~SEC_BAT_TX_RETRY_MIX_TEMP) == 0)
					sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_RETRY, BATT_TX_EVENT_WIRELESS_TX_RETRY);
				battery->tx_retry_case &= ~SEC_BAT_TX_RETRY_MIX_TEMP;
			}
		}

		pr_info("%s: mix_limit(%d), temp(%d), chg_temp(%d), input_current(%d)\n",
			__func__, battery->mix_limit, battery->temperature, chg_temp, input_current);
	} else {
		battery->mix_limit = false;
	}
	return input_current;
}

static void sec_bat_check_wpc_temp(struct sec_battery_info *battery, int *input_current, int *charging_current)
{
	if (battery->pdata->wpc_temp_check_type == SEC_BATTERY_TEMP_CHECK_NONE)
		return;

	if (is_wireless_type(battery->cable_type)) {
		union power_supply_propval value = {0, };
		int wpc_vout_level = 0;
		bool flicker_wa = false;

		if (battery->cable_type == SEC_BATTERY_CABLE_HV_WIRELESS_20)
			wpc_vout_level = battery->wpc_max_vout_level;
		else
			wpc_vout_level = WIRELESS_VOUT_10V;
		mutex_lock(&battery->voutlock);

		/* get vout level */
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_RX_VOUT, value);

		if(is_hv_wireless_type(battery->cable_type) &&
			value.intval == WIRELESS_VOUT_5_5V_STEP &&
			battery->wpc_vout_level != WIRELESS_VOUT_5_5V_STEP) {
			pr_info("%s: real vout was not 10V \n", __func__);
			battery->wpc_vout_level = WIRELESS_VOUT_5_5V_STEP;
		}

		if (battery->siop_level >= 100 && !battery->lcd_status) {
			int temp_val = sec_bat_get_temp_by_temp_control_source(battery,
				battery->pdata->wpc_temp_control_source);

			if ((!battery->chg_limit && temp_val >= battery->pdata->wpc_high_temp) ||
				(battery->chg_limit && temp_val > battery->pdata->wpc_high_temp_recovery)) {
				battery->chg_limit = true;
				if(*input_current > battery->pdata->wpc_input_limit_current) {
					if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_TX &&
						battery->pdata->wpc_input_limit_by_tx_check)
						*input_current = battery->pdata->wpc_input_limit_current_by_tx;
					else
						*input_current = battery->pdata->wpc_input_limit_current;
				}
				if(*charging_current > battery->pdata->wpc_charging_limit_current)
					*charging_current = battery->pdata->wpc_charging_limit_current;
				wpc_vout_level = WIRELESS_VOUT_5_5V_STEP;
			} else if (battery->chg_limit) {
				battery->chg_limit = false;
			}
		} else {
			if ((is_hv_wireless_type(battery->cable_type) &&
				battery->cable_type != SEC_BATTERY_CABLE_WIRELESS_HV_VEHICLE) ||
				battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV ||
				battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_20) {
				int temp_val = sec_bat_get_temp_by_temp_control_source(battery,
					battery->pdata->wpc_temp_lcd_on_control_source);

				if ((!battery->chg_limit &&
						temp_val >= battery->pdata->wpc_lcd_on_high_temp) ||
					(battery->chg_limit &&
						temp_val > battery->pdata->wpc_lcd_on_high_temp_rec)) {
					if(*input_current > battery->pdata->wpc_lcd_on_input_limit_current)
						*input_current = battery->pdata->wpc_lcd_on_input_limit_current;
					if(*charging_current > battery->pdata->wpc_charging_limit_current)
						*charging_current = battery->pdata->wpc_charging_limit_current;
					battery->chg_limit = true;
					if (battery->cable_type == SEC_BATTERY_CABLE_HV_WIRELESS_20)
						wpc_vout_level = (battery->capacity < 95) ?
							WIRELESS_VOUT_5_5V_STEP : battery->wpc_max_vout_level;
					else
						wpc_vout_level = (battery->capacity < 95) ?
							WIRELESS_VOUT_5_5V_STEP : WIRELESS_VOUT_10V;
				} else if (battery->chg_limit) {
					battery->chg_limit = false;
				}
			} else if (battery->chg_limit) {
				battery->chg_limit = false;
			}
		}

		if (is_hv_wireless_type(battery->cable_type)) {
			bool skip_wa = false;
			if (battery->wpc_vout_ctrl_lcd_on) {
				psy_do_property(battery->pdata->wireless_charger_name, get,
					POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ID, value);
				if (value.intval == WC_PAD_ID_UNKNOWN ||
					value.intval == WC_PAD_ID_SNGL_DREAM ||
					value.intval == WC_PAD_ID_STAND_DREAM) {
					pr_info("%s: do not use flicker w/a\n", __func__);
					skip_wa = true;
				}
			}
#if defined(CONFIG_ISDB_CHARGING_CONTROL)
			if ((battery->current_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING) ||
				(battery->wpc_vout_ctrl_lcd_on && battery->lcd_status && !skip_wa) ||
				(battery->current_event & SEC_BAT_CURRENT_EVENT_ISDB) || sleep_mode) {
				pr_info("%s: vout 5.5V set. WPC_VOUT_CTRL_LCD_ON(%d), LCD(%d)\n",
					__func__, battery->wpc_vout_ctrl_lcd_on, battery->lcd_status);
#else
			if ((battery->current_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING) ||
				(battery->wpc_vout_ctrl_lcd_on && battery->lcd_status && !skip_wa) || sleep_mode) {
				pr_info("%s: vout 5.5V set. WPC_VOUT_CTRL_LCD_ON(%d), LCD(%d)\n",
					__func__, battery->wpc_vout_ctrl_lcd_on, battery->lcd_status);
#endif
				wpc_vout_level = WIRELESS_VOUT_5_5V_STEP;
			}

			if (wpc_vout_level != battery->wpc_vout_level) {
				battery->wpc_vout_level = wpc_vout_level;
				if (battery->current_event & SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK) {
					pr_info("%s: block to set wpc vout level(%s) because otg on\n",
						__func__, vout_control_mode_str[wpc_vout_level]);
				} else {
					value.intval = wpc_vout_level;
					psy_do_property(battery->pdata->wireless_charger_name, set,
						POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
					pr_info("%s: change vout level(%s)",
						__func__, vout_control_mode_str[battery->wpc_vout_level]);
					battery->aicl_current = 0; /* reset aicl current */
					flicker_wa = true;
				}
			} else if (battery->chg_limit && battery->wpc_vout_ctrl_lcd_on && !skip_wa) {
				value.intval = battery->lcd_status;
				psy_do_property(battery->pdata->wireless_charger_name, set,
						POWER_SUPPLY_EXT_PROP_PAD_VOLT_CTRL, value);
				flicker_wa = true;
			} else if ((battery->wpc_vout_level == WIRELESS_VOUT_10V || battery->wpc_vout_level == battery->wpc_max_vout_level) && !battery->chg_limit)
				/* reset aicl current to recover current for unexpected aicl during before vout boosting completion */
				battery->aicl_current = 0;

			value.intval = 0;
			psy_do_property(battery->pdata->wireless_charger_name, get,
					POWER_SUPPLY_EXT_PROP_PAD_VOLT_CTRL, value);

			if (battery->wpc_vout_ctrl_lcd_on && !flicker_wa && !skip_wa && (value.intval == battery->lcd_status)) {
				pr_info("%s : Different pad voltage and lcd on/off\n. Change Pad Voltage.\n", __func__);
				value.intval = battery->lcd_status;
				psy_do_property(battery->pdata->wireless_charger_name, set,
						POWER_SUPPLY_EXT_PROP_PAD_VOLT_CTRL, value);
			}
		}

		mutex_unlock(&battery->voutlock);
		pr_info("%s: change input_current(%d), change charge_current(%d), vout_level(%s), chg_limit(%d)\n",
			__func__, *input_current, *charging_current, vout_control_mode_str[battery->wpc_vout_level], battery->chg_limit);
	}
}

int get_chg_power_type(int ct, int ws, int pd_max_pw, int max_pw)
{
	if (!is_wireless_type(ct)) {
		if (is_pd_wire_type(ct) &&
			pd_max_pw >= HV_CHARGER_STATUS_STANDARD4)
			return SFC_45W;
		else if (is_pd_wire_type(ct) &&
			pd_max_pw >= HV_CHARGER_STATUS_STANDARD3)
			return SFC_25W;
		else if (is_hv_wire_12v_type(ct) ||
			max_pw >= HV_CHARGER_STATUS_STANDARD2) /* 20000mW */
			return AFC_12V_OR_20W;
		else if (is_hv_wire_type(ct) ||
			(is_pd_wire_type(ct) &&
			pd_max_pw >= HV_CHARGER_STATUS_STANDARD1) ||
			ws == SEC_BATTERY_CABLE_PREPARE_TA ||
			max_pw >= HV_CHARGER_STATUS_STANDARD1) /* 12000mW */
			return AFC_9V_OR_15W;
	}

	return NORMAL_TA;
}

int sec_bat_check_power_type(
	int max_chg_pwr, int pd_max_chg_pwr, int ct, int ws, int is_apdo)
{
	if (is_pd_wire_type(ct) && is_apdo) {
		if (get_chg_power_type(ct, ws, pd_max_chg_pwr, max_chg_pwr) == SFC_45W)
			return SFC_45W;
		else if (get_chg_power_type(ct, ws, pd_max_chg_pwr, max_chg_pwr) == SFC_25W)
			return SFC_25W;
		else
			return NORMAL_TA;
	} else
		return NORMAL_TA;
}

#if defined(CONFIG_DIRECT_CHARGING)
static int sec_bat_check_lpm_power(int lpm, int pt)
{
	int ret = 0;

	if (pt == SFC_25W)
		ret |= 0x02;

	if (lpm)
		ret |= 0x01;

	return ret;
}
#endif

int sec_bat_check_lrp_temp_cond(int prev_step,
	int temp, int trig, int recov)
{
	if (trig <= temp)
		prev_step++;
	else if (recov >= temp)
		prev_step--;

	if (prev_step < LRP_NONE)
		prev_step = LRP_NONE;
	else if (prev_step > LRP_STEP2)
		prev_step = LRP_STEP2;

	return prev_step;
}

int sec_bat_check_lrp_step(
	struct sec_battery_info *battery, int temp, int pt, bool lcd_sts)
{
	int step = LRP_NONE;
	int lcd_st = LCD_OFF;
	int lrp_pt = LRP_NORMAL;
	int lrp_high_temp_st1 = battery->pdata->lrp_temp[LRP_NORMAL].trig[ST1][LCD_OFF];
	int lrp_high_temp_st2 = battery->pdata->lrp_temp[LRP_NORMAL].trig[ST2][LCD_OFF];
	int lrp_high_temp_recov_st1 = battery->pdata->lrp_temp[LRP_NORMAL].recov[ST1][LCD_OFF];
	int lrp_high_temp_recov_st2 = battery->pdata->lrp_temp[LRP_NORMAL].recov[ST2][LCD_OFF];

	if (lcd_sts)
		lcd_st = LCD_ON;

	if (pt == SFC_45W)
		lrp_pt = LRP_45W;
	else if (pt == SFC_25W)
		lrp_pt = LRP_25W;

	lrp_high_temp_st1 = battery->pdata->lrp_temp[lrp_pt].trig[ST1][lcd_st];
	lrp_high_temp_st2 = battery->pdata->lrp_temp[lrp_pt].trig[ST2][lcd_st];
	lrp_high_temp_recov_st1 = battery->pdata->lrp_temp[lrp_pt].recov[ST1][lcd_st];
	lrp_high_temp_recov_st2 = battery->pdata->lrp_temp[lrp_pt].recov[ST2][lcd_st];

	pr_info("%s: st1(%d), st2(%d), recv_st1(%d), recv_st2(%d), lrp(%d)\n", __func__,
		lrp_high_temp_st1, lrp_high_temp_st2,
		lrp_high_temp_recov_st1, lrp_high_temp_recov_st2, temp);

	switch (battery->lrp_step) {
	case LRP_STEP2:
		step = sec_bat_check_lrp_temp_cond(battery->lrp_step,
				temp, 900, lrp_high_temp_recov_st2);
		break;
	case LRP_STEP1:
		step = sec_bat_check_lrp_temp_cond(battery->lrp_step,
				temp, lrp_high_temp_st2, lrp_high_temp_recov_st1);
		break;
	case LRP_NONE:
		step = sec_bat_check_lrp_temp_cond(battery->lrp_step,
				temp, lrp_high_temp_st1, -200);
		break;
	default:
		break;
	}

	if ((battery->lrp_step != LRP_STEP1) && (step == LRP_STEP1))
		step = sec_bat_check_lrp_temp_cond(step,
				temp, lrp_high_temp_st2, lrp_high_temp_recov_st1);

	return step;
}

int sec_bat_get_lrp_step(struct sec_battery_info *battery)
{
	bool is_apdo = false;
	int power_type = NORMAL_TA;
	int ct = battery->cable_type, ws = battery->wire_status;

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	is_apdo = (is_pd_apdo_wire_type(ct) && battery->pd_list.now_isApdo) ? 1 : 0;
#endif
	power_type = sec_bat_check_power_type(battery->max_charge_power,
				battery->pd_max_charge_power, ct, ws, is_apdo);

	return sec_bat_check_lrp_step(battery, battery->lrp, power_type, battery->lcd_status);
}

void sec_bat_check_lrp_temp(
	struct sec_battery_info *battery, int ct, int ws, int siop_level, bool lcd_sts, int *input_current, int *charging_current)
{
	int lrp_step = LRP_NONE;
	bool is_apdo = false;
	int power_type = NORMAL_TA;
	bool force_check = false;
	int ret = 0;
	int max_charging_current = 1800;
	int temp_ic = *input_current, temp_cc = *charging_current;

	if (battery->pdata->lrp_temp_check_type == SEC_BATTERY_TEMP_CHECK_NONE)
		return;

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	is_apdo = (is_pd_apdo_wire_type(ct) && battery->pd_list.now_isApdo) ? 1 : 0;
#endif
	power_type = sec_bat_check_power_type(battery->max_charge_power,
				battery->pd_max_charge_power, ct, ws, is_apdo);

	lrp_step = sec_bat_check_lrp_step(battery, battery->lrp, power_type, lcd_sts);

	/* 15w afc or pd ta */
	if (power_type == NORMAL_TA) {
		ret = battery->input_voltage;
		if (((ret == SEC_INPUT_VOLTAGE_5V) && !lcd_sts) ||
			((ret != SEC_INPUT_VOLTAGE_5V) && lcd_sts))
			force_check = true;
		pr_info("%s: force_check(%d), ret(%d), lcd(%d)\n", __func__,
			(force_check ? 1 : 0), ret, (lcd_sts ? 1 : 0));
	}

	if ((lrp_step == LRP_STEP2) || (lrp_step == LRP_STEP1)) {
		if (*charging_current > max_charging_current)
			*charging_current = max_charging_current;

		if (is_pd_wire_type(ct)) {
			if (power_type == SFC_45W) {
				*input_current = battery->pdata->lrp_curr[LRP_45W].st_icl[lrp_step - 1];
				*charging_current = battery->pdata->lrp_curr[LRP_45W].st_fcc[lrp_step - 1];
			} else if (power_type == SFC_25W) {
				*input_current = battery->pdata->lrp_curr[LRP_25W].st_icl[lrp_step - 1];
				*charging_current = battery->pdata->lrp_curr[LRP_25W].st_fcc[lrp_step - 1];
			} else {
				if (*input_current > (60000 / battery->input_voltage))
					*input_current = 60000 / battery->input_voltage;
			}
		} else if (is_hv_wire_type(ct)) {
			if (is_hv_wire_12v_type(battery->cable_type)) {
				if (*input_current > battery->pdata->siop_hv_12v_input_limit_current)
					*input_current = battery->pdata->siop_hv_12v_input_limit_current;
			} else {
				if (*input_current > battery->pdata->siop_hv_input_limit_current)
					*input_current = battery->pdata->siop_hv_input_limit_current;
			}
		} else {
			if (*input_current > battery->pdata->siop_input_limit_current)
				*input_current = battery->pdata->siop_input_limit_current;
		}
		if ((battery->lrp_step != lrp_step) || force_check)
			pr_info(
				"%s:LRP:%d%%,%dmV,lrp_step(%d),lcd(%d),tlrp(%d),icl(%d),fcc(%d),ct(%d),is_apdo(%d),mcp(%d,%d)",
					__func__, battery->capacity, battery->voltage_now, lrp_step, lcd_sts,
					battery->lrp, *input_current, *charging_current, battery->cable_type,
					is_apdo, battery->pd_max_charge_power, battery->max_charge_power);
		battery->lrp_limit = true;
	} else if ((battery->lrp_limit == true) && (lrp_step == LRP_NONE)) {
		battery->lrp_limit = false;
		pr_info(
			"%s:LRP:SOC(%d),Vnow(%d),lrp_lim(%d),tlrp(%d),ct(%d)",
				__func__, battery->capacity, battery->voltage_now, battery->lrp_limit,
				battery->lrp, battery->cable_type);
	}
	battery->lrp_step = lrp_step;

	pr_info("%s: cable_type(%d), lrp_step(%d), lrp(%d)\n", __func__,
		ct, battery->lrp_step, battery->lrp);

	if (temp_ic < *input_current || (power_type >= SFC_25W && temp_cc < *charging_current)) {
		pr_info("%s: do not set new icl(%d) cc(%d) because of old icl(%d) cc(%d)\n",
			__func__, *input_current, temp_ic, *charging_current, temp_cc);
		*input_current = temp_ic;
		*charging_current = temp_cc;
	}
}

#if defined(CONFIG_DIRECT_CHARGING)
void sec_bat_set_dchg_current(struct sec_battery_info *battery, int power_type, int is_apdo, int pt, int *input_current, int *charging_current)
{
	int temp_ic = *input_current, temp_cc = *charging_current;

	/* skip power_type check for non-use lrp_temp_check models */
	if (battery->pdata->lrp_temp_check_type == SEC_BATTERY_TEMP_CHECK_NONE)
		power_type = NORMAL_TA;

	if (power_type == SFC_45W) {
		if (pt & 0x01) {
			*input_current = battery->pdata->lrp_curr[LRP_45W].st_icl[ST1];
			*charging_current = battery->pdata->lrp_curr[LRP_45W].st_fcc[ST1];
		} else {
			*input_current = battery->pdata->lrp_curr[LRP_45W].st_icl[ST2];
			*charging_current = battery->pdata->lrp_curr[LRP_45W].st_fcc[ST2];
		}
	} else if (power_type == SFC_25W) {
		if (pt & 0x01) {
			*input_current = battery->pdata->lrp_curr[LRP_25W].st_icl[ST1];
			*charging_current = battery->pdata->lrp_curr[LRP_25W].st_fcc[ST1];
		} else {
			*input_current = battery->pdata->lrp_curr[LRP_25W].st_icl[ST2];
			*charging_current = battery->pdata->lrp_curr[LRP_25W].st_fcc[ST2];
		}
	} else {
		if (battery->input_voltage == SEC_INPUT_VOLTAGE_5V) {
			*input_current = battery->pdata->default_input_current;
			*charging_current = battery->pdata->default_charging_current;
		} else {
			if (is_apdo) {
				*input_current = battery->pdata->dchg_input_limit_current;
				*charging_current = battery->pdata->dchg_charging_limit_current;
			} else {
				*input_current = battery->pdata->chg_input_limit_current;
				*charging_current = battery->pdata->chg_charging_limit_current;
			}
		}
	}
	if (temp_ic < *input_current || (power_type >= SFC_25W && temp_cc < *charging_current)) {
		pr_info("%s: do not set new icl(%d) cc(%d) because of old icl(%d) cc(%d)\n",
			__func__, *input_current, *charging_current, temp_ic, temp_cc);
		*input_current = temp_ic;
		*charging_current = temp_cc;
	}
}
#endif

static bool sec_bat_change_vbus(struct sec_battery_info *battery, int *input_current)
{
	if (battery->pdata->chg_temp_check_type == SEC_BATTERY_TEMP_CHECK_NONE)
		return false;

#if defined(CONFIG_SUPPORT_HV_CTRL)
	union power_supply_propval value;
	unsigned int target_vbus = SEC_INPUT_VOLTAGE_0V;
	int lrp_step = LRP_NONE;

	if (battery->store_mode ||
		((battery->siop_level == 80) && is_wired_type(battery->cable_type))) {
		pr_info("%s : store_mode(%d) siop(%d) ct(%d)\n",
			__func__, battery->store_mode, battery->siop_level, battery->cable_type);
		return false;
	}

	if (is_hv_wire_type(battery->cable_type) &&
		(battery->cable_type != SEC_BATTERY_CABLE_QC30)) {

		if (battery->current_event & SEC_BAT_CURRENT_EVENT_AFC) {
			pr_info("%s: skip during current_event(0x%x)\n",
				__func__, battery->current_event);
			return false;
		}

		lrp_step = sec_bat_get_lrp_step(battery);
		pr_info("%s: lrp_step: %d\n", __func__, lrp_step);

		/* check target vbus */
		if (battery->vbus_limit)
			target_vbus = SEC_INPUT_VOLTAGE_0V;
		else if (battery->vbus_chg_by_full)
			target_vbus = SEC_INPUT_VOLTAGE_5V;
		else if (battery->siop_level >= 100 && lrp_step == LRP_NONE) {
			if (is_hv_wire_12v_type(battery->cable_type))
				target_vbus = SEC_INPUT_VOLTAGE_12V;
			else
				target_vbus = SEC_INPUT_VOLTAGE_9V;

			if (battery->vbus_chg_by_siop == SEC_INPUT_VOLTAGE_NONE)
				battery->vbus_chg_by_siop = target_vbus;

		} else if (battery->status == POWER_SUPPLY_STATUS_CHARGING)
			target_vbus = SEC_INPUT_VOLTAGE_5V;

		if (target_vbus == SEC_INPUT_VOLTAGE_0V) {
			pr_info("%s: skip set vbus %dV, level(%d), Cable(%s, %s, %d, %d)\n",
				__func__, target_vbus, battery->siop_level,
				sec_cable_type[battery->cable_type], sec_cable_type[battery->wire_status],
				battery->muic_cable_type, battery->pd_usb_attached);

			return false;
		}

		if (battery->vbus_chg_by_siop != target_vbus) {
			/* change input current to pre_afc_input_current */
			*input_current = battery->pdata->pre_afc_input_current;
			battery->charge_power = (battery->input_voltage * (*input_current)) / 10;
			value.intval = *input_current;
			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_PROP_CURRENT_MAX, value);
			battery->input_current = *input_current;

			/* set current event */
			cancel_delayed_work(&battery->afc_work);
			wake_unlock(&battery->afc_wake_lock);
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_AFC,
						  (SEC_BAT_CURRENT_EVENT_CHG_LIMIT | SEC_BAT_CURRENT_EVENT_AFC));

			battery->chg_limit = false;
			battery->vbus_chg_by_siop = target_vbus;
			muic_afc_set_voltage(target_vbus/10);

			pr_info("%s: vbus set %dV by level(%d), Cable(%s, %s, %d, %d)\n",
				__func__, target_vbus, battery->siop_level,
				sec_cable_type[battery->cable_type], sec_cable_type[battery->wire_status],
				battery->muic_cable_type, battery->pd_usb_attached);

			return true;
		}
	}
#endif
	return false;
}

#if defined(CONFIG_DIRECT_CHARGING)
static void sec_bat_check_direct_chg_temp(struct sec_battery_info *battery, int *input_current, int *charging_current)
{
	int pt = 0;
	int ct = battery->cable_type, ws = battery->wire_status;
	bool is_apdo = false;
	int power_type = NORMAL_TA;

	if (battery->pdata->dchg_temp_check_type == SEC_BATTERY_TEMP_CHECK_NONE)
		return;

	is_apdo = (is_pd_apdo_wire_type(ct) && battery->pd_list.now_isApdo) ? 1 : 0;
	power_type = sec_bat_check_power_type(battery->max_charge_power,
				battery->pd_max_charge_power, ct, ws, is_apdo);

	pt = sec_bat_check_lpm_power(lpcharge, power_type);

	if (battery->siop_level >= 100) {
		if (!battery->chg_limit && is_apdo &&
			((battery->dchg_temp >= battery->pdata->dchg_high_temp[pt]) ||
			(battery->temperature >= battery->pdata->dchg_high_batt_temp[pt]))) {
			sec_bat_set_dchg_current(battery, power_type, pt, is_apdo, input_current, charging_current);
			battery->chg_limit = true;
		} else if (!battery->chg_limit && (!is_apdo) &&
			(battery->chg_temp >= battery->pdata->chg_high_temp)) {
			sec_bat_set_dchg_current(battery, power_type, pt, is_apdo, input_current, charging_current);
			battery->chg_limit = true;
		} else if (battery->chg_limit) {
			if (((battery->dchg_temp <= battery->pdata->dchg_high_temp_recovery[pt]) &&
				(battery->temperature <= battery->pdata->dchg_high_batt_temp_recovery[pt]) &&
				is_apdo) || ((battery->chg_temp <= battery->pdata->chg_high_temp_recovery) &&
				(!is_apdo))) {
				*input_current = battery->pdata->charging_current[battery->cable_type].input_current_limit;
				*charging_current = battery->pdata->charging_current[battery->cable_type].fast_charging_current;
				battery->chg_limit = false;
			} else {
				sec_bat_set_dchg_current(battery, power_type, pt, is_apdo, input_current, charging_current);
				battery->chg_limit = true;
			}
		}
		pr_info("%s: cable_type(%d), chg_limit(%d) vbus_by_siop(%d) ic(%d) cc(%d)\n", __func__,
			battery->cable_type, battery->chg_limit, battery->vbus_chg_by_siop, *input_current, *charging_current);
	}
}
#endif

static void sec_bat_check_afc_temp(struct sec_battery_info *battery, int *input_current, int *charging_current)
{
	if (battery->pdata->chg_temp_check_type == SEC_BATTERY_TEMP_CHECK_NONE)
		return;

#if defined(CONFIG_SUPPORT_HV_CTRL)
	if (battery->siop_level >= 100) {
		if (!battery->chg_limit && is_hv_wire_type(battery->cable_type) && (battery->chg_temp >= battery->pdata->chg_high_temp)) {
			*input_current = battery->pdata->chg_input_limit_current;
			*charging_current = battery->pdata->chg_charging_limit_current;
			battery->chg_limit = true;
		} else if (!battery->chg_limit && battery->max_charge_power >= (battery->pdata->pd_charging_charge_power - 500) && (battery->chg_temp >= battery->pdata->chg_high_temp)) {
			*input_current = battery->pdata->default_input_current;
			*charging_current = battery->pdata->default_charging_current;
			battery->chg_limit = true;
		} else if (battery->chg_limit && is_hv_wire_type(battery->cable_type)) {
			if (battery->chg_temp <= battery->pdata->chg_high_temp_recovery) {
				*input_current = battery->pdata->charging_current[battery->cable_type].input_current_limit;
				*charging_current = battery->pdata->charging_current[battery->cable_type].fast_charging_current;
				battery->chg_limit = false;
			} else {
				*input_current = battery->pdata->chg_input_limit_current;
				*charging_current = battery->pdata->chg_charging_limit_current;
				battery->chg_limit = true;
			}
		} else if (battery->chg_limit && battery->max_charge_power >= (battery->pdata->pd_charging_charge_power - 500)) {
			if (battery->chg_temp <= battery->pdata->chg_high_temp_recovery) {
				*input_current = battery->pdata->charging_current[battery->cable_type].input_current_limit;
				*charging_current = battery->pdata->charging_current[battery->cable_type].fast_charging_current;
				battery->chg_limit = false;
			} else {
				*input_current = battery->pdata->chg_input_limit_current;
				*charging_current = battery->pdata->chg_charging_limit_current;
				battery->chg_limit = true;
			}
		}
		pr_info("%s: cable_type(%d), chg_limit(%d) vbus_by_siop(%d)\n", __func__,
			battery->cable_type, battery->chg_limit, battery->vbus_chg_by_siop);
	}
#else
	if ((!battery->chg_limit && is_hv_wire_type(battery->cable_type) && (battery->chg_temp >= battery->pdata->chg_high_temp)) ||
		(battery->chg_limit && is_hv_wire_type(battery->cable_type) && (battery->chg_temp >= battery->pdata->chg_high_temp_recovery))) {
		*input_current = battery->pdata->chg_input_limit_current;
		*charging_current = battery->pdata->chg_charging_limit_current;
		battery->chg_limit = true;
	} else if (battery->chg_limit && is_hv_wire_type(battery->cable_type) && (battery->chg_temp <= battery->pdata->chg_high_temp_recovery)) {
		*input_current = battery->pdata->charging_current[battery->cable_type].input_current_limit;
		*charging_current = battery->pdata->charging_current[battery->cable_type].fast_charging_current;
		battery->chg_limit = false;
	}
#endif
}

#if defined(CONFIG_CCIC_NOTIFIER)
extern void select_pdo(int num);
static bool sec_bat_change_vbus_pd(struct sec_battery_info *battery, int *input_current)
{
#if defined(CONFIG_SUPPORT_HV_CTRL)
	int target_pd_index = 0;
	int lrp_step = LRP_NONE;

	if (battery->pdata->chg_temp_check_type == SEC_BATTERY_TEMP_CHECK_NONE)
		return false;

	if (battery->store_mode ||
		((battery->siop_level == 80) && is_wired_type(battery->cable_type))) {
		pr_info("%s : store_mode(%d) siop(%d) ct(%d)\n",
			__func__, battery->store_mode, battery->siop_level, battery->cable_type);
		return false;
	}

	if (battery->cable_type == SEC_BATTERY_CABLE_PDIC) {
		if (battery->current_event & SEC_BAT_CURRENT_EVENT_SELECT_PDO) {
			pr_info("%s: skip during current_event(0x%x)\n",
				__func__, battery->current_event);
			return false;
		}

		lrp_step = sec_bat_get_lrp_step(battery);
		pr_info("%s: lrp_step: %d siop(%d)\n", __func__, lrp_step, battery->siop_level);

		if (battery->siop_level >= 100 && lrp_step == LRP_NONE) {
			/* select PDO greater than 5V */
			target_pd_index = battery->pd_list.max_pd_count - 1;
		} else {
			/* select 5V PDO */
			target_pd_index = 0;
		}

		if (target_pd_index < 0 || target_pd_index >= MAX_PDO_NUM) {
			pr_info("%s: target_pd_index is wrong: %d\n", __func__, target_pd_index);
			return false;
		}

		pr_info("%s: target_pd_index: %d, now_pd_index: %d\n", __func__,
			target_pd_index, battery->pd_list.now_pd_index);

		if (target_pd_index != battery->pd_list.now_pd_index) {
			/* change input current before request new pdo if new pdo's input current is less than now */
#if defined(CONFIG_PDIC_PD30)
			if (battery->pd_list.pd_info[target_pd_index].max_current < battery->input_current) {
#else			
			if (battery->pd_list.pd_info[target_pd_index].input_current < battery->input_current) {
#endif
				union power_supply_propval value = {0, };
#if defined(CONFIG_PDIC_PD30)
				*input_current = battery->pd_list.pd_info[target_pd_index].max_current;
#else
				*input_current = battery->pd_list.pd_info[target_pd_index].input_current;
#endif

				value.intval = *input_current;
				battery->input_current = *input_current;
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SELECT_PDO,
					SEC_BAT_CURRENT_EVENT_SELECT_PDO);
				psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_PROP_CURRENT_MAX, value);
			}
			battery->pdic_ps_rdy = false;
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SELECT_PDO,
				SEC_BAT_CURRENT_EVENT_SELECT_PDO);
			if (target_pd_index >= 0 && target_pd_index < MAX_PDO_NUM)
				select_pdo(battery->pd_list.pd_info[target_pd_index].pdo_index);

			return true;
		}
	}
#endif
	return false;
}

static void sec_bat_check_pdic_temp(struct sec_battery_info *battery, int *input_current, int *charging_current)
{
	if (battery->pdata->chg_temp_check_type == SEC_BATTERY_TEMP_CHECK_NONE)
		return;

	if (battery->pdic_ps_rdy && battery->siop_level >= 100 && !battery->lcd_status) {
		if ((!battery->chg_limit && (battery->chg_temp >= battery->pdata->chg_high_temp)) ||
			(battery->chg_limit && (battery->chg_temp >= battery->pdata->chg_high_temp_recovery))) {
			*input_current =
				(battery->pdata->chg_input_limit_current * SEC_INPUT_VOLTAGE_9V) / battery->input_voltage;
			*charging_current = battery->pdata->chg_charging_limit_current;
			battery->chg_limit = true;
		} else if (battery->chg_limit && battery->chg_temp <= battery->pdata->chg_high_temp_recovery) {
			*input_current = battery->pdata->charging_current[battery->cable_type].input_current_limit;
			*charging_current = battery->pdata->charging_current[battery->cable_type].fast_charging_current;
			battery->chg_limit = false;
		}
		pr_info("%s: cable_type(%d), chg_limit(%d)\n", __func__,
			battery->cable_type, battery->chg_limit);
	}
}

static int sec_bat_check_pd_input_current(struct sec_battery_info *battery, int input_current)
{
	if (battery->current_event & SEC_BAT_CURRENT_EVENT_SELECT_PDO) {
		input_current = battery->input_current;
		pr_info("%s: change input_current(%d), cable_type(%d)\n", __func__, input_current, battery->cable_type);
	}

	return input_current;
}
#endif
#endif

static int sec_bat_check_afc_input_current(struct sec_battery_info *battery, int input_current)
{
	if (battery->current_event & SEC_BAT_CURRENT_EVENT_AFC) {
		int work_delay = 0;

		if (!is_wireless_type(battery->cable_type)) {
			input_current = battery->pdata->pre_afc_input_current; // 1000mA
			work_delay = battery->pdata->pre_afc_work_delay;
		} else {
			input_current = battery->pdata->pre_wc_afc_input_current;
			/* do not reduce this time, this is for noble pad */
			work_delay = battery->pdata->pre_wc_afc_work_delay;
		}

		wake_lock(&battery->afc_wake_lock);
		if (!delayed_work_pending(&battery->afc_work))
			queue_delayed_work(battery->monitor_wqueue,
				&battery->afc_work , msecs_to_jiffies(work_delay));

		pr_info("%s: change input_current(%d), cable_type(%d)\n", __func__, input_current, battery->cable_type);
	}

	return input_current;
}

#if defined(CONFIG_CCIC_NOTIFIER)
static void sec_bat_get_input_current_in_power_list(struct sec_battery_info *battery)
{
	int pdo_num = battery->pdic_info.sink_status.current_pdo_num;
	int max_input_current = 0;

#if defined(CONFIG_PDIC_PD30)
	if (is_pd_apdo_wire_type(battery->wire_status) && battery->pd_list.now_isApdo)
		pdo_num = 1;
#endif

	max_input_current = battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC].input_current_limit =
		battery->pdic_info.sink_status.power_list[pdo_num].max_current;
#if defined(CONFIG_PDIC_PD30)
	battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC_APDO].input_current_limit =
		battery->pdic_info.sink_status.power_list[pdo_num].max_current;
#endif

	pr_info("%s:max_input_current : %dmA\n", __func__, max_input_current);
}

static void sec_bat_get_charging_current_in_power_list(struct sec_battery_info *battery)
{
	int max_charging_current = 0, pd_power = 0;
	int pdo_num = battery->pdic_info.sink_status.current_pdo_num;

#if defined(CONFIG_PDIC_PD30)
	if (is_pd_apdo_wire_type(battery->wire_status) && battery->pd_list.now_isApdo)
		pdo_num = 1;
#endif

	pd_power = (battery->pdic_info.sink_status.power_list[pdo_num].max_voltage *
		battery->pdic_info.sink_status.power_list[pdo_num].max_current);

	/* We assume that output voltage to float voltage */
	max_charging_current = pd_power / (battery->pdata->chg_float_voltage / battery->pdata->chg_float_voltage_conv);
	max_charging_current = max_charging_current > battery->pdata->max_charging_current ?
		battery->pdata->max_charging_current : max_charging_current;
	battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC].fast_charging_current = max_charging_current;
#if defined(CONFIG_PDIC_PD30)
#if defined(CONFIG_STEP_CHARGING)
	if (is_pd_apdo_wire_type(battery->wire_status) && !battery->pd_list.now_isApdo &&
		battery->step_charging_status < 0)
#else
	if (is_pd_apdo_wire_type(battery->wire_status) && !battery->pd_list.now_isApdo)
#endif
		battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC_APDO].fast_charging_current = max_charging_current;
#endif
	battery->charge_power = pd_power / 1000;

	pr_info("%s:pd_charge_power : %dmW, max_charging_current : %dmA\n", __func__,
		battery->charge_power, max_charging_current);
}
#endif

void sec_bat_set_decrease_iout(struct sec_battery_info *battery, bool last_delay)
{
	union power_supply_propval value = {0, };
	int i = 0, step = 3, input_current[3] = {500, 300, 100};
	int prev_input_current = battery->input_current;

	for (i = 0 ; i < step ; i++) {
		if (prev_input_current > input_current[i]) {
			pr_info("%s: Wireless iout goes to %dmA before switch charging path to cable\n",
				__func__, input_current[i]);
			prev_input_current = input_current[i];
			value.intval = input_current[i];
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_PAD_VOLT_CTRL, value);

			if ((i != step - 1) || last_delay)
				msleep(300);
		}
	}
}

void sec_bat_set_mfc_off(struct sec_battery_info *battery, bool need_ept)
{
	union power_supply_propval value = {0, };
	char wpc_en_status[2];

	if (need_ept) {
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_PROP_ONLINE, value);

		msleep(300);
	}

	wpc_en_status[0] = WPC_EN_CHARGING;
	wpc_en_status[1] = false;
	value.strval = wpc_en_status;
	psy_do_property(battery->pdata->wireless_charger_name, set,
		POWER_SUPPLY_EXT_PROP_WPC_EN, value);

	pr_info("%s: WC CONTROL: Disable\n", __func__);
}

void sec_bat_set_mfc_on(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };
	char wpc_en_status[2];

	wpc_en_status[0] = WPC_EN_CHARGING;
	wpc_en_status[1] = true;
	value.strval = wpc_en_status;
	psy_do_property(battery->pdata->wireless_charger_name, set,
		POWER_SUPPLY_EXT_PROP_WPC_EN, value);

	pr_info("%s: WC CONTROL: Enable\n", __func__);
}

int sec_bat_set_charging_current(struct sec_battery_info *battery)
{
	static int afc_init = false;
	union power_supply_propval value = {0, };
	unsigned int input_current = battery->pdata->charging_current[battery->cable_type].input_current_limit,
		charging_current = battery->pdata->charging_current[battery->cable_type].fast_charging_current,
		topoff_current = battery->pdata->full_check_current_1st;

	if (battery->aicl_current)
		input_current = battery->aicl_current;
	mutex_lock(&battery->iolock);
	if (is_nocharge_type(battery->cable_type)) {
	} else {
#if !defined(CONFIG_SEC_FACTORY)
		if (!(battery->current_event & SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL)) {
			input_current = sec_bat_check_mix_temp(battery, input_current);
		}
#endif

		/* check input current */
#if !defined(CONFIG_SEC_FACTORY)
		if (!(battery->current_event & SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL)) {
			if (is_wireless_type(battery->cable_type))
				sec_bat_check_wpc_temp(battery, &input_current, &charging_current);
#if defined(CONFIG_CCIC_NOTIFIER)
			else if (battery->cable_type == SEC_BATTERY_CABLE_PDIC) {
				if (!sec_bat_change_vbus_pd(battery, &input_current)) {
					sec_bat_check_pdic_temp(battery, &input_current, &charging_current);
					input_current = sec_bat_check_pd_input_current(battery, input_current);
				}
			}
#endif
#if defined(CONFIG_DIRECT_CHARGING)
			else if (is_pd_apdo_wire_type(battery->wire_status)) {
				sec_bat_check_direct_chg_temp(battery, &input_current, &charging_current);
			}
#endif
			else {
				if (!sec_bat_change_vbus(battery, &input_current))
					sec_bat_check_afc_temp(battery, &input_current, &charging_current);
			}
		}
#endif

		/* set limited charging current during wireless power sharing with cable charging */
		if (battery->pdata->charging_limit_by_tx_check &&
			battery->wc_tx_enable &&
			(is_hv_wire_type(battery->cable_type) || is_pd_wire_type(battery->cable_type)))
			if (charging_current > battery->pdata->charging_limit_current_by_tx)
				charging_current = battery->pdata->charging_limit_current_by_tx;

		input_current = sec_bat_check_afc_input_current(battery, input_current);

		/* Set limited max current with hv wire cable when store mode is set and LDU
			Limited max current should be set with over 5% capacity since target could be turned off during boot up */
		if (battery->store_mode && is_hv_wire_type(battery->wire_status) && (battery->capacity >= 5)) {
			input_current = battery->pdata->store_mode_afc_input_current;
		}

		sec_bat_get_charging_current_by_siop(battery, &input_current, &charging_current);
		sec_bat_check_lrp_temp(battery, battery->cable_type, battery->wire_status,
			battery->siop_level, battery->lcd_status, &input_current, &charging_current);

		/* Calculate wireless input current under the specific conditions (wpc_sleep_mode, chg_limit)*/
		if (battery->wc_status != SEC_WIRELESS_PAD_NONE) {
			input_current = sec_bat_get_wireless_current(battery, input_current);
		}

		/* check topoff current */
		if (battery->charging_mode == SEC_BATTERY_CHARGING_2ND &&
			battery->pdata->full_check_type_2nd == SEC_BATTERY_FULLCHARGED_CHGPSY) {
			topoff_current =
				battery->pdata->full_check_current_2nd;
		}

		/* check swelling state */
		if (is_wireless_type(battery->cable_type)) {
			if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND) {
				charging_current = (charging_current > battery->pdata->swelling_wc_low_temp_current_2nd) ?
					battery->pdata->swelling_wc_low_temp_current_2nd : charging_current;
				topoff_current = (topoff_current > battery->pdata->swelling_low_temp_topoff) ?
					battery->pdata->swelling_low_temp_topoff : topoff_current;
			} else if (battery->current_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING) {
				charging_current = (charging_current > battery->pdata->swelling_wc_high_temp_current) ?
					battery->pdata->swelling_wc_high_temp_current : charging_current;
				topoff_current = (topoff_current > battery->pdata->swelling_high_temp_topoff) ?
					battery->pdata->swelling_high_temp_topoff : topoff_current;
			} else if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING) {
				charging_current = (charging_current > battery->pdata->swelling_wc_low_temp_current) ?
					battery->pdata->swelling_wc_low_temp_current : charging_current;
			} else if (battery->swelling_low_temp_3rd_ctrl && (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_3RD)) {
				charging_current = (charging_current > battery->pdata->swelling_wc_low_temp_current_3rd) ?
					battery->pdata->swelling_wc_low_temp_current_3rd : charging_current;
			}

			if (is_hv_wireless_type(battery->cable_type) && (battery->wpc_vout_ctrl_lcd_on && battery->lcd_status)) {
				input_current = 500;
				if (input_current != battery->input_current) {
					value.intval = input_current;
					psy_do_property(battery->pdata->charger_name, set,
							POWER_SUPPLY_EXT_PROP_PAD_VOLT_CTRL, value);
				}
			}
		} else {
			if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND) {
				if (charging_current > battery->pdata->swelling_low_temp_current_2nd) {
					charging_current = battery->pdata->swelling_low_temp_current_2nd;
				}
				topoff_current = (topoff_current > battery->pdata->swelling_low_temp_topoff) ?
					battery->pdata->swelling_low_temp_topoff : topoff_current;
			} else if (battery->current_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING) {
				if (charging_current > battery->pdata->swelling_high_temp_current) {
					charging_current = battery->pdata->swelling_high_temp_current;
				}
				topoff_current = (topoff_current > battery->pdata->swelling_high_temp_topoff) ?
					battery->pdata->swelling_high_temp_topoff : topoff_current;
			} else if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING) {
				if (charging_current > battery->pdata->swelling_low_temp_current) {
					charging_current = battery->pdata->swelling_low_temp_current;
				}
			} else if (battery->swelling_low_temp_3rd_ctrl && (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_3RD)) {
				if (charging_current > battery->pdata->swelling_low_temp_current_3rd) {
					charging_current = battery->pdata->swelling_low_temp_current_3rd;
				}
			} else if (battery->swelling_low_temp_4th_ctrl && (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_4TH)) {
				if (charging_current > battery->pdata->swelling_low_temp_current_4th) {
					charging_current = battery->pdata->swelling_low_temp_current_4th;
				}
#if defined(CONFIG_DIRECT_CHARGING)
			} else if (battery->swelling_low_temp_5th_ctrl && (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_5TH)) {
				if (charging_current > battery->pdata->swelling_low_temp_current_5th && (battery->pd_list.num_apdo > 0)) {
					charging_current = battery->pdata->swelling_low_temp_current_5th;
				}
#endif
			}

#if defined(CONFIG_ENABLE_100MA_CHARGING_BEFORE_USB_CONFIGURED)
			/* usb unconfigured or suspend*/
			if ((battery->cable_type == SEC_BATTERY_CABLE_USB) && !lpcharge) {
				if (battery->current_event & SEC_BAT_CURRENT_EVENT_USB_100MA) {
					pr_info("%s: usb unconfigured\n", __func__);
					input_current = USB_CURRENT_UNCONFIGURED;
					charging_current = USB_CURRENT_UNCONFIGURED;
				}
			}
#endif
		}/* is_wireless_type */
	}/* is_nocharge_type(battery->cable_type) */



	/* set input current, charging current */
	if ((battery->refresh_current) ||
		(battery->input_current != input_current) ||
		(battery->charging_current != charging_current)) {
		/* update charge power */
		battery->charge_power = (battery->input_voltage * input_current) / 10;
		if (battery->charge_power > battery->max_charge_power)
			battery->max_charge_power = battery->charge_power;

		/* In wireless charging, must be set charging current before input current. */
		if (is_wireless_type(battery->cable_type)) {
			if(battery->charging_current < charging_current) {
				/* common charging current */
				value.intval = charging_current;
				psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_PROP_CURRENT_AVG, value);
				/* common input current */
				if (battery->input_current != input_current) {
					value.intval = input_current;
					psy_do_property(battery->pdata->charger_name, set,
						POWER_SUPPLY_PROP_CURRENT_MAX, value);
					battery->input_current = input_current;
				}
			} else {
				/* common input current */
				value.intval = input_current;
				psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_PROP_CURRENT_MAX, value);
				battery->input_current = input_current;
				/* wired charging current */
				value.intval = charging_current;
				psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_PROP_CURRENT_AVG, value);
			}
		} else {
			/* common input current */
			value.intval = input_current;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_CURRENT_MAX, value);
			battery->input_current = input_current;
			/* wired charging current */
			value.intval = charging_current;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_CURRENT_NOW, value);
		}

		if (charging_current <= 100)
			battery->charging_current = 100;
		else
			battery->charging_current = charging_current;
		pr_info("%s: power(%d), input(%d), charge(%d)\n", __func__,
			battery->charge_power, battery->input_current, battery->charging_current);
	}

#if defined(CONFIG_DIRECT_CHARGING)
	if (battery->dc_float_voltage_set) {
		pr_info("%s : step float voltage = %d \n", __func__,
			battery->pdata->dc_step_chg_val_vfloat[battery->pdata->age_step][battery->step_charging_status]);
		value.intval = battery->pdata->dc_step_chg_val_vfloat[battery->pdata->age_step][battery->step_charging_status];
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_EXT_PROP_DIRECT_VOLTAGE_MAX, value);
		battery->dc_float_voltage_set = false;
	}
#endif

	/* set topoff current */
	if ((battery->refresh_current) ||
		(battery->topoff_current != topoff_current)) {
		value.intval = topoff_current;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_FULL, value);
		battery->topoff_current = topoff_current;
	}
	if (!afc_init) {
		afc_init = true;
#if defined(CONFIG_AFC_CHARGER_MODE)
		value.intval = 1;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_AFC_CHARGER_MODE,
			value);
#endif
	}

	battery->refresh_current = false;
	mutex_unlock(&battery->iolock);
	return 0;
}

void sec_bat_refresh_charging_current(struct sec_battery_info *battery)
{
	mutex_lock(&battery->iolock);
	battery->refresh_current = true;
	mutex_unlock(&battery->iolock);

	pr_info("%s: refresh charging current (input=%d, fcc=%d, topoff=%d)\n",
		__func__, battery->input_current, battery->charging_current, battery->topoff_current);
	queue_delayed_work(battery->monitor_wqueue, &battery->siop_level_work, 0);
}

int sec_bat_set_charge(struct sec_battery_info *battery,
			int chg_mode)
{
	union power_supply_propval val = {0, };
	ktime_t current_time = {0, };
	struct timespec ts = {0, };

#if defined(CONFIG_PREVENT_USB_CONN_OVERHEAT)
	if ((battery->cable_type == SEC_BATTERY_CABLE_HMT_CONNECTED) ||
		((battery->usb_temp_flag || (battery->misc_event & BATT_MISC_EVENT_TEMP_HICCUP_TYPE)) && (chg_mode != SEC_BAT_CHG_MODE_BUCK_OFF)))
		return 0;
#else
	if (battery->cable_type == SEC_BATTERY_CABLE_HMT_CONNECTED)
		return 0;
#endif

	if ((battery->current_event & SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE ||
		battery->misc_event & BATT_MISC_EVENT_FULL_CAPACITY) &&
		(chg_mode == SEC_BAT_CHG_MODE_CHARGING)) {
		dev_info(battery->dev, "%s: charge disable by HMT or SOC\n", __func__);
		chg_mode = SEC_BAT_CHG_MODE_CHARGING_OFF;
	}

	battery->charger_mode = chg_mode;
	pr_info("%s set %s mode\n", __func__, sec_bat_charge_mode_str[chg_mode]);

	val.intval = battery->status;
	psy_do_property(battery->pdata->charger_name, set,
		POWER_SUPPLY_PROP_STATUS, val);
	current_time = ktime_get_boottime();
	ts = ktime_to_timespec(current_time);

	if (chg_mode == SEC_BAT_CHG_MODE_CHARGING) {
		/*Reset charging start time only in initial charging start */
		if (battery->charging_start_time == 0) {
			if (ts.tv_sec < 1)
				ts.tv_sec = 1;
			battery->charging_start_time = ts.tv_sec;
			battery->charging_next_time =
				battery->pdata->charging_reset_time;
		}
		battery->charging_block = false;
#if defined(CONFIG_DIRECT_CHARGING)
		if (is_pd_apdo_wire_type(battery->cable_type)) {
			sec_bat_reset_step_charging(battery);
			sec_bat_check_dc_step_charging(battery);
		}
#endif
	} else {
		battery->charging_start_time = 0;
		battery->charging_passed_time = 0;
		battery->charging_next_time = 0;
		battery->charging_fullcharged_time = 0;
		battery->full_check_cnt = 0;
		battery->charging_block = true;
#if defined(CONFIG_STEP_CHARGING)
		sec_bat_reset_step_charging(battery);
#endif
#if defined(CONFIG_BATTERY_CISD)
		battery->usb_overheat_check = false;
		battery->cisd.ab_vbat_check_count = 0;
#endif
	}

	battery->temp_highlimit_cnt = 0;
	battery->temp_high_cnt = 0;
	battery->temp_low_cnt = 0;
	battery->temp_recover_cnt = 0;

	if ((battery->wc_tx_enable && battery->buck_cntl_by_tx) &&
		(chg_mode != SEC_BAT_CHG_MODE_BUCK_OFF)) {
		dev_info(battery->dev, "@Tx_Mode %s: buck disable by Tx mode\n", __func__);
		chg_mode = SEC_BAT_CHG_MODE_BUCK_OFF;
	}
	val.intval = chg_mode;
	psy_do_property(battery->pdata->charger_name, set,
		POWER_SUPPLY_PROP_CHARGING_ENABLED, val);
	psy_do_property(battery->pdata->fuelgauge_name, set,
		POWER_SUPPLY_PROP_CHARGING_ENABLED, val);

	return 0;
}

static bool sec_bat_check_by_psy(struct sec_battery_info *battery)
{
	char *psy_name = NULL;
	union power_supply_propval value = {0, };
	bool ret = true;

	switch (battery->pdata->battery_check_type) {
	case SEC_BATTERY_CHECK_PMIC:
		psy_name = battery->pdata->pmic_name;
		break;
	case SEC_BATTERY_CHECK_FUELGAUGE:
		psy_name = battery->pdata->fuelgauge_name;
		break;
	case SEC_BATTERY_CHECK_CHARGER:
		psy_name = battery->pdata->charger_name;
		break;
	default:
		dev_err(battery->dev,
			"%s: Invalid Battery Check Type\n", __func__);
		ret = false;
		goto battery_check_error;
		break;
	}

	psy_do_property(psy_name, get,
		POWER_SUPPLY_PROP_PRESENT, value);
	ret = (bool)value.intval;

battery_check_error:
	return ret;
}

static bool sec_bat_check(struct sec_battery_info *battery)
{
	bool ret = true;

	if (battery->factory_mode || battery->is_jig_on) {
		dev_dbg(battery->dev, "%s: No need to check in factory mode\n",
			__func__);
		return ret;
	}

	if (battery->health != POWER_SUPPLY_HEALTH_GOOD &&
		battery->health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
		dev_dbg(battery->dev, "%s: No need to check\n", __func__);
		return ret;
	}

	switch (battery->pdata->battery_check_type) {
	case SEC_BATTERY_CHECK_ADC:
		if (is_nocharge_type(battery->cable_type))
			ret = battery->present;
		else
			ret = sec_bat_check_vf_adc(battery);
		break;
	case SEC_BATTERY_CHECK_INT:
	case SEC_BATTERY_CHECK_CALLBACK:
		if (is_nocharge_type(battery->cable_type)) {
			ret = battery->present;
		} else {
			if (battery->pdata->check_battery_callback)
				ret = battery->pdata->check_battery_callback();
		}
		break;
	case SEC_BATTERY_CHECK_PMIC:
	case SEC_BATTERY_CHECK_FUELGAUGE:
	case SEC_BATTERY_CHECK_CHARGER:
		ret = sec_bat_check_by_psy(battery);
		break;
	case SEC_BATTERY_CHECK_NONE:
		dev_dbg(battery->dev, "%s: No Check\n", __func__);
	default:
		break;
	}

	return ret;
}

static void sec_bat_send_cs100(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };
	bool send_cs100_cmd = true;

	if (is_wireless_type(battery->cable_type)) {
#ifdef CONFIG_CS100_JPNCONCEPT
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ID, value);

		/* In case of the JPN PAD, this pad blocks the charge after give the cs100 command. */
		send_cs100_cmd = (battery->charging_mode == SEC_BATTERY_CHARGING_2ND ||	value.intval);
#endif
		if (send_cs100_cmd) {
			value.intval = POWER_SUPPLY_STATUS_FULL;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_PROP_STATUS, value);
		}
	}
}
static bool sec_bat_get_cable_type(
			struct sec_battery_info *battery,
			int cable_source_type)
{
	bool ret = false;
	int cable_type = battery->cable_type;

	if (cable_source_type & SEC_BATTERY_CABLE_SOURCE_CALLBACK) {
		if (battery->pdata->check_cable_callback)
			cable_type =
				battery->pdata->check_cable_callback();
	}

	if (cable_source_type & SEC_BATTERY_CABLE_SOURCE_ADC) {
		if (gpio_get_value_cansleep(
			battery->pdata->bat_gpio_ta_nconnected) ^
			battery->pdata->bat_polarity_ta_nconnected)
			cable_type = SEC_BATTERY_CABLE_NONE;
		else
			cable_type =
				sec_bat_get_charger_type_adc(battery);
	}

	if (battery->cable_type == cable_type) {
		dev_dbg(battery->dev,
			"%s: No need to change cable status\n", __func__);
	} else {
		if (cable_type < SEC_BATTERY_CABLE_NONE ||
			cable_type >= SEC_BATTERY_CABLE_MAX) {
			dev_err(battery->dev,
				"%s: Invalid cable type\n", __func__);
		} else {
			battery->cable_type = cable_type;
			if (battery->pdata->check_cable_result_callback)
				battery->pdata->check_cable_result_callback(
						battery->cable_type);

			ret = true;

			dev_dbg(battery->dev, "%s: Cable Changed (%d)\n",
				__func__, battery->cable_type);
		}
	}

	return ret;
}

static void sec_bat_set_charging_status(struct sec_battery_info *battery,
		int status) {
	union power_supply_propval value = {0, };

	switch (status) {
	case POWER_SUPPLY_STATUS_CHARGING:
		if (battery->siop_level < 100 || battery->lcd_status || battery->wc_tx_enable)
			battery->stop_timer = true;
		break;
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
	case POWER_SUPPLY_STATUS_DISCHARGING:
		if ((battery->status == POWER_SUPPLY_STATUS_FULL ||
		     (battery->capacity == 100 && !is_slate_mode(battery))) &&
		    !battery->store_mode) {

			pr_info("%s : Update fg scale to 101%%\n", __func__);
			value.intval = 100;
			psy_do_property(battery->pdata->fuelgauge_name, set,
					POWER_SUPPLY_PROP_CHARGE_FULL, value);

			/* To get SOC value (NOT raw SOC), need to reset value */
			value.intval = 0;
			psy_do_property(battery->pdata->fuelgauge_name, get,
					POWER_SUPPLY_PROP_CAPACITY, value);
			battery->capacity = value.intval;
		}
		battery->expired_time = battery->pdata->expired_time;
		battery->prev_safety_time = 0;
		break;
	case POWER_SUPPLY_STATUS_FULL:
		sec_bat_send_cs100(battery);
		break;
	default:
		break;
	}
	battery->status = status;
}

static bool sec_bat_battery_cable_check(struct sec_battery_info *battery)
{
	if (!sec_bat_check(battery)) {
		if (battery->check_count < battery->pdata->check_count)
			battery->check_count++;
		else {
			dev_err(battery->dev,
				"%s: Battery Disconnected\n", __func__);
			battery->present = false;
			battery->health = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;

			if (battery->status !=
				POWER_SUPPLY_STATUS_DISCHARGING) {
				sec_bat_set_charging_status(battery,
						POWER_SUPPLY_STATUS_NOT_CHARGING);
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_BUCK_OFF);
			}

			if (battery->pdata->check_battery_result_callback)
				battery->pdata->
					check_battery_result_callback();
			return false;
		}
	} else
		battery->check_count = 0;

	battery->present = true;

	if (battery->health == POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
		battery->health = POWER_SUPPLY_HEALTH_GOOD;

		if (battery->status == POWER_SUPPLY_STATUS_NOT_CHARGING) {
			sec_bat_set_charging_status(battery,
					POWER_SUPPLY_STATUS_CHARGING);
#if defined(CONFIG_BATTERY_SWELLING)
			if (!battery->swelling_mode)
#endif
			sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING);
		}
	}

	dev_dbg(battery->dev, "%s: Battery Connected\n", __func__);

	if (battery->pdata->cable_check_type &
		SEC_BATTERY_CABLE_CHECK_POLLING) {
		if (sec_bat_get_cable_type(battery,
			battery->pdata->cable_source_type)) {
			wake_lock(&battery->cable_wake_lock);
			queue_delayed_work(battery->monitor_wqueue,
					   &battery->cable_work, 0);
		}
	}
	return true;
}

static int sec_bat_ovp_uvlo_by_psy(struct sec_battery_info *battery)
{
	char *psy_name = NULL;
	union power_supply_propval value = {0, };

	value.intval = POWER_SUPPLY_HEALTH_GOOD;

	switch (battery->pdata->ovp_uvlo_check_type) {
	case SEC_BATTERY_OVP_UVLO_PMICPOLLING:
		psy_name = battery->pdata->pmic_name;
		break;
	case SEC_BATTERY_OVP_UVLO_CHGPOLLING:
		psy_name = battery->pdata->charger_name;
		break;
	default:
		dev_err(battery->dev,
			"%s: Invalid OVP/UVLO Check Type\n", __func__);
		goto ovp_uvlo_check_error;
		break;
	}

	psy_do_property(psy_name, get,
		POWER_SUPPLY_PROP_HEALTH, value);

ovp_uvlo_check_error:
	return value.intval;
}

static bool sec_bat_ovp_uvlo_result(
		struct sec_battery_info *battery, int health)
{
#if defined(CONFIG_DIRECT_CHARGING)
	union power_supply_propval val = {0, };

	if (health == POWER_SUPPLY_HEALTH_DC_ERR) {
		dev_info(battery->dev,
			"%s: DC err (%d)\n",
			__func__, health);
		battery->is_recharging = false;
		battery->health_check_count = DEFAULT_HEALTH_CHECK_COUNT;
		wake_lock_timeout(&battery->vbus_wake_lock, HZ * 10);
		/* Enable charging anyway to check actual DC's health */
		val.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_CHARGING_ENABLED, val);
		
		sec_bat_set_charge(battery, battery->charger_mode);
	}
#endif

	if (battery->health != health) {
		battery->health = health;
		switch (health) {
		case POWER_SUPPLY_HEALTH_GOOD:
			dev_info(battery->dev, "%s: Safe voltage\n", __func__);
			dev_info(battery->dev, "%s: is_recharging : %d\n", __func__, battery->is_recharging);
			sec_bat_set_charging_status(battery,
					POWER_SUPPLY_STATUS_CHARGING);
			battery->charging_mode = SEC_BATTERY_CHARGING_1ST;
#if defined(CONFIG_BATTERY_SWELLING)
			if (!battery->swelling_mode)
#endif
			sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING);
			battery->health_check_count = 0;
			break;
		case POWER_SUPPLY_HEALTH_OVERVOLTAGE:
		case POWER_SUPPLY_HEALTH_UNDERVOLTAGE:
			dev_info(battery->dev,
				"%s: Unsafe voltage (%d)\n",
				__func__, health);
			sec_bat_set_charging_status(battery,
					POWER_SUPPLY_STATUS_NOT_CHARGING);
			sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
			battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
			battery->is_recharging = false;
			battery->health_check_count = DEFAULT_HEALTH_CHECK_COUNT;
#if defined(CONFIG_BATTERY_CISD)
			battery->cisd.data[CISD_DATA_UNSAFETY_VOLTAGE]++;
			battery->cisd.data[CISD_DATA_UNSAFE_VOLTAGE_PER_DAY]++;
#endif
			/* Take the wakelock during 10 seconds
			   when over-voltage status is detected	 */
			wake_lock_timeout(&battery->vbus_wake_lock, HZ * 10);
			break;
		}
		power_supply_changed(battery->psy_bat);
		return true;
	}

	return false;
}

static bool sec_bat_ovp_uvlo(struct sec_battery_info *battery)
{
	int health = POWER_SUPPLY_HEALTH_GOOD;

	if (battery->wdt_kick_disable) {
		dev_dbg(battery->dev,
			"%s: No need to check in wdt test\n",
			__func__);
		return false;
	} else if ((battery->status == POWER_SUPPLY_STATUS_FULL) &&
		   (battery->charging_mode == SEC_BATTERY_CHARGING_NONE)) {
		dev_dbg(battery->dev, "%s: No need to check in Full status", __func__);
		return false;
	}

#if defined(CONFIG_DIRECT_CHARGING)
	if (battery->health != POWER_SUPPLY_HEALTH_GOOD &&
		battery->health != POWER_SUPPLY_HEALTH_OVERVOLTAGE &&
		battery->health != POWER_SUPPLY_HEALTH_UNDERVOLTAGE &&
		battery->health != POWER_SUPPLY_HEALTH_DC_ERR) {
		dev_dbg(battery->dev, "%s: No need to check\n", __func__);
		return false;
	}
#else
	if (battery->health != POWER_SUPPLY_HEALTH_GOOD &&
		battery->health != POWER_SUPPLY_HEALTH_OVERVOLTAGE &&
		battery->health != POWER_SUPPLY_HEALTH_UNDERVOLTAGE) {
		dev_dbg(battery->dev, "%s: No need to check\n", __func__);
		return false;
	}
#endif

	health = battery->health;

	switch (battery->pdata->ovp_uvlo_check_type) {
	case SEC_BATTERY_OVP_UVLO_CALLBACK:
		if (battery->pdata->ovp_uvlo_callback)
			health = battery->pdata->ovp_uvlo_callback();
		break;
	case SEC_BATTERY_OVP_UVLO_PMICPOLLING:
	case SEC_BATTERY_OVP_UVLO_CHGPOLLING:
		health = sec_bat_ovp_uvlo_by_psy(battery);
		break;
	case SEC_BATTERY_OVP_UVLO_PMICINT:
	case SEC_BATTERY_OVP_UVLO_CHGINT:
		/* nothing for interrupt check */
	default:
		break;
	}

	/*
	 * Move the location for calling the get_health
	 * in case of attaching the jig
	 */
	if (battery->factory_mode || battery->is_jig_on) {
		dev_dbg(battery->dev,
			"%s: No need to check in factory mode\n",
			__func__);
		return false;
	}

	return sec_bat_ovp_uvlo_result(battery, health);
}

static bool sec_bat_check_recharge(struct sec_battery_info *battery)
{
#if defined(CONFIG_BATTERY_SWELLING)
	if (battery->swelling_mode == SWELLING_MODE_CHARGING ||
		battery->swelling_mode == SWELLING_MODE_FULL) {
		pr_info("%s: Skip normal recharge check routine for swelling mode\n",
			__func__);
		return false;
	}
#endif
	if ((battery->status == POWER_SUPPLY_STATUS_CHARGING) &&
			(battery->pdata->full_condition_type &
			 SEC_BATTERY_FULL_CONDITION_NOTIMEFULL) &&
			(battery->charging_mode == SEC_BATTERY_CHARGING_NONE)) {
		dev_info(battery->dev,
				"%s: Re-charging by NOTIMEFULL (%d)\n",
				__func__, battery->capacity);
		goto check_recharge_check_count;
	}

	if (battery->status == POWER_SUPPLY_STATUS_FULL &&
			battery->charging_mode == SEC_BATTERY_CHARGING_NONE) {
		int recharging_voltage = battery->pdata->recharge_condition_vcell;
		if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE) {
			/* float voltage - 150mV */
			recharging_voltage =\
				(battery->pdata->chg_float_voltage /\
				battery->pdata->chg_float_voltage_conv) - battery->pdata->swelling_low_rechg_thr;
			dev_info(battery->dev, "%s: recharging voltage changed by low temp(%d)\n",
					__func__, recharging_voltage);
		}
		dev_info(battery->dev, "%s: recharging voltage (%d)\n",
				__func__, recharging_voltage);

		if ((battery->pdata->recharge_condition_type &
					SEC_BATTERY_RECHARGE_CONDITION_SOC) &&
				(battery->capacity <=
				 battery->pdata->recharge_condition_soc)) {
			battery->expired_time = battery->pdata->recharging_expired_time;
			battery->prev_safety_time = 0;
			dev_info(battery->dev,
					"%s: Re-charging by SOC (%d)\n",
					__func__, battery->capacity);
			goto check_recharge_check_count;
		}

		if ((battery->pdata->recharge_condition_type &
		     SEC_BATTERY_RECHARGE_CONDITION_AVGVCELL) &&
		    (battery->voltage_avg <= recharging_voltage)) {
			battery->expired_time = battery->pdata->recharging_expired_time;
			battery->prev_safety_time = 0;
			dev_info(battery->dev,
					"%s: Re-charging by average VCELL (%d)\n",
					__func__, battery->voltage_avg);
			goto check_recharge_check_count;
		}

		if ((battery->pdata->recharge_condition_type &
		     SEC_BATTERY_RECHARGE_CONDITION_VCELL) &&
		    (battery->voltage_now <= recharging_voltage)) {
			battery->expired_time = battery->pdata->recharging_expired_time;
			battery->prev_safety_time = 0;
			dev_info(battery->dev,
					"%s: Re-charging by VCELL (%d)\n",
					__func__, battery->voltage_now);
			goto check_recharge_check_count;
		}
	}

	battery->recharge_check_cnt = 0;
	return false;

check_recharge_check_count:
	if (battery->recharge_check_cnt <
		battery->pdata->recharge_check_count)
		battery->recharge_check_cnt++;
	dev_dbg(battery->dev,
		"%s: recharge count = %d\n",
		__func__, battery->recharge_check_cnt);

	if (battery->recharge_check_cnt >=
		battery->pdata->recharge_check_count)
		return true;
	else
		return false;
}

static bool sec_bat_voltage_check(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };

	if (battery->status == POWER_SUPPLY_STATUS_DISCHARGING ||
		battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_FAKE) {
		dev_dbg(battery->dev,
			"%s: Charging Disabled\n", __func__);
		return true;
	}

	/* OVP/UVLO check */
	if (sec_bat_ovp_uvlo(battery)) {
		if (battery->pdata->ovp_uvlo_result_callback)
			battery->pdata->
				ovp_uvlo_result_callback(battery->health);
		return false;
	}

	if ((battery->status == POWER_SUPPLY_STATUS_FULL) &&
		(battery->charging_mode != SEC_BATTERY_CHARGING_NONE || battery->swelling_mode)) {
		pr_info("%s: chg mode (%d), swelling_mode(%d) \n",
			__func__, battery->charging_mode, battery->swelling_mode);

		value.intval = 0;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CAPACITY, value);
		if (value.intval < 98 &&
			battery->voltage_now <
				(battery->pdata->recharge_condition_vcell - 50)) {
			sec_bat_set_charging_status(battery,
					POWER_SUPPLY_STATUS_CHARGING);
			battery->is_recharging = false;
			battery->charging_mode = SEC_BATTERY_CHARGING_1ST;
			pr_info("%s: battery status full -> charging, RepSOC(%d)\n", __func__, value.intval);
			return false;
		}
	}

	/* Re-Charging check */
	if (sec_bat_check_recharge(battery)) {
		if (battery->pdata->full_check_type !=
			SEC_BATTERY_FULLCHARGED_NONE)
			battery->charging_mode = SEC_BATTERY_CHARGING_1ST;
		else
			battery->charging_mode = SEC_BATTERY_CHARGING_2ND;
		battery->is_recharging = true;
#if defined(CONFIG_BATTERY_CISD)
		battery->cisd.data[CISD_DATA_RECHARGING_COUNT]++;
		battery->cisd.data[CISD_DATA_RECHARGING_COUNT_PER_DAY]++;
#endif
#if defined(CONFIG_BATTERY_SWELLING)
		if (!battery->swelling_mode)
#endif
		sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING);
		return false;
	}

	return true;
}

#if defined(CONFIG_BATTERY_SWELLING)
static void sec_bat_swelling_check(struct sec_battery_info *battery)
{
	union power_supply_propval val = {0, };
	int swelling_rechg_voltage = battery->pdata->swelling_high_rechg_voltage;
	bool en_swelling = false, en_rechg = false;
	int swelling_high_recovery = battery->pdata->swelling_high_temp_recov;
	int swelling_high_block = battery->pdata->swelling_high_temp_block;

	if (battery->pdata->temp_check_type == SEC_BATTERY_TEMP_CHECK_NONE)
		return;

	if (is_wireless_type(battery->cable_type)) {
		swelling_high_recovery = battery->pdata->swelling_wc_high_temp_recov;
		swelling_high_block = battery->pdata->swelling_wc_high_temp_block;
	}
	pr_info("%s: swelling highblock(%d), highrecov(%d)\n", __func__, swelling_high_block, swelling_high_recovery);

	psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_PROP_VOLTAGE_MAX, val);

	pr_info("%s: status(%s), swell_mode(%s), chg_block(%d), low_temp_event(0x%x), vfloat(%d)mV, temp(%d)'C\n",
		__func__,
		sec_bat_status_str[battery->status],
		swelling_mode_str[battery->swelling_mode],
		battery->charging_block,
		(battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE),
		val.intval,
		battery->temperature);

	/* swelling_mode
		under voltage over voltage, battery missing */
	if ((battery->status == POWER_SUPPLY_STATUS_DISCHARGING) ||\
	    (battery->status == POWER_SUPPLY_STATUS_NOT_CHARGING) ||
	    battery->skip_swelling) {
		pr_debug("%s: DISCHARGING or NOT-CHARGING or 15 test mode. stop swelling mode\n", __func__);
		battery->swelling_mode = SWELLING_MODE_NONE;
		sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
		goto skip_swelling_check;
	}

	if (!battery->swelling_mode) {
		if (battery->temperature >= swelling_high_block) {
			pr_info("%s: swelling mode start. stop charging\n", __func__);
			battery->swelling_mode = SWELLING_MODE_CHARGING;
			battery->swelling_full_check_cnt = 0;

			if (battery->wc_tx_enable &&
				(is_hv_wire_type(battery->cable_type) || is_pd_wire_type(battery->cable_type)))
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
			else
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_BUCK_OFF);

			if (battery->temperature >= swelling_high_block) {
#if defined(CONFIG_BATTERY_CISD)
				if (is_wireless_type(battery->cable_type) || (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_FAKE)) {
					battery->cisd.data[CISD_DATA_WC_HIGH_TEMP_SWELLING]++;
					battery->cisd.data[CISD_DATA_WC_HIGH_TEMP_SWELLING_PER_DAY]++;
				} else {
					battery->cisd.data[CISD_DATA_HIGH_TEMP_SWELLING]++;
					battery->cisd.data[CISD_DATA_HIGH_TEMP_SWELLING_PER_DAY]++;
				}
#endif
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING,
					SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
			}
			en_swelling = true;
		} else if ((battery->temperature <= battery->pdata->swelling_low_temp_block_2nd) &&
			!(battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND)) {
			pr_info("%s: 2nd low temperature swelling step!! reduce current\n", __func__);
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND,
				SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
		} else if ((battery->temperature <= battery->pdata->swelling_low_temp_block_1st) &&
			!(battery->current_event & (SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND))) {
			pr_info("%s: low temperature reduce current\n", __func__);
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING,
						  SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
		} else if (battery->swelling_low_temp_3rd_ctrl && (battery->temperature <= battery->pdata->swelling_low_temp_block_3rd) &&
			!(battery->current_event & (SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_3RD))) {
			pr_info("%s: 3rd low temperature swelling step!!  reduce current\n", __func__);
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_3RD,
				SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
		} else if (battery->swelling_low_temp_4th_ctrl && (battery->temperature <= battery->pdata->swelling_low_temp_block_4th) &&
			!(battery->current_event & (SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_3RD | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_4TH))) {
			pr_info("%s: 4th low temperature swelling step!! reduce current\n", __func__);
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_4TH,
				SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
		} else if (battery->swelling_low_temp_5th_ctrl && (battery->temperature <= battery->pdata->swelling_low_temp_block_5th) &&
			!(battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE)) {
			pr_info("%s: 5th low temperature swelling step!! reduce current\n", __func__);
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_5TH,
						SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
		} else if (battery->swelling_low_temp_5th_ctrl && (battery->temperature >= battery->pdata->swelling_low_temp_recov_5th) &&
			(battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE)) {
			pr_info("%s: normal temperature recover from 5th current\n", __func__);
			sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE);
#if defined(CONFIG_STEP_CHARGING)
			sec_bat_reset_step_charging(battery);
#endif
		} else if (battery->swelling_low_temp_4th_ctrl && (battery->temperature >= battery->pdata->swelling_low_temp_recov_4th) &&
			(battery->current_event & (SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_3RD | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_4TH))) {
			pr_info("%s: upto 4th low temperature swelling recovery temp! 5th low temp swelling current set\n", __func__);
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_5TH,
						SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE);
		} else if (battery->swelling_low_temp_3rd_ctrl && (battery->temperature >= battery->pdata->swelling_low_temp_recov_3rd) &&
			(battery->current_event & (SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_3RD))) {
			if (battery->swelling_low_temp_4th_ctrl) {
				pr_info("%s: upto 3rd low temperature swelling recovery temp! 4th low temp swelling current set\n", __func__);
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_4TH,
							SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
			} else {
				pr_info("%s: normal temperature recover current\n", __func__);
				sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE);
			}
		} else if ((battery->temperature >= battery->pdata->swelling_low_temp_recov_1st) &&
			(battery->current_event & (SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING | SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND))) {
			if (battery->swelling_low_temp_3rd_ctrl) {
				pr_info("%s: upto 1st low temperature swelling recovery temp! 3rd low temp swelling current set\n", __func__);
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_3RD,
							 SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
			} else {
				pr_info("%s: normal temperature recover current\n", __func__);
				sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE);
			}
		} else if ((battery->temperature >= battery->pdata->swelling_low_temp_recov_2nd) &&
			(battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND)) {
			pr_info("%s: upto 2nd low temperature swelling recovery temp! 1st low temp swelling current set\n", __func__);
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING,
						  SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
		}
	}

	if (!battery->voltage_now)
		return;

	if (battery->swelling_mode) {

		if (battery->temperature <= swelling_high_recovery) {
			pr_info("%s: swelling mode end. restart charging\n", __func__);
			battery->swelling_mode = SWELLING_MODE_NONE;
			battery->charging_mode = SEC_BATTERY_CHARGING_1ST;
			sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
			sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING);
			/* restore 4.4V float voltage */
			val.intval = battery->pdata->swelling_normal_float_voltage;
			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_PROP_VOLTAGE_MAX, val);
#if defined(CONFIG_BATTERY_CISD)
			battery->cisd.data[CISD_DATA_SWELLING_RECOVERY_CNT]++;
			battery->cisd.data[CISD_DATA_SWELLING_RECOVERY_CNT_PER_DAY]++;
#endif
		} else if (battery->voltage_now < swelling_rechg_voltage &&
			   battery->charging_block) {
			pr_info("%s: swelling mode recharging start. Vbatt(%d)\n",
				__func__, battery->voltage_now);
			battery->charging_mode = SEC_BATTERY_CHARGING_1ST;
			en_rechg = true;
			/* change drop float voltage */
			val.intval = battery->pdata->swelling_drop_float_voltage;
			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_PROP_VOLTAGE_MAX, val);
			/* set charging enable */
			sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING);
			if (battery->temperature > swelling_high_recovery) {
				pr_info("%s: swelling mode reduce charging current(HIGH-temp:%d)\n",
					__func__, battery->temperature);
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING,
							  SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
			}
		}
	}

	if (en_swelling && !en_rechg) {
		pr_info("%s : SAFETY TIME RESET (SWELLING MODE CHARING STOP!)\n", __func__);
		battery->expired_time = battery->pdata->expired_time;
		battery->prev_safety_time = 0;
	}

skip_swelling_check:
	dev_dbg(battery->dev, "%s end\n", __func__);
}
#endif

#if defined(CONFIG_BATTERY_AGE_FORECAST)
static bool sec_bat_set_aging_step(struct sec_battery_info *battery, int step)
{
	union power_supply_propval value = {0, };

	if (battery->pdata->num_age_step <= 0 || step < 0 || step >= battery->pdata->num_age_step) {
		pr_info("%s: [AGE] abnormal age step : %d/%d\n",
			__func__, step, battery->pdata->num_age_step-1);
		return false;
	}

	battery->pdata->age_step = step;

	/* float voltage */
	battery->pdata->chg_float_voltage =
		battery->pdata->age_data[battery->pdata->age_step].float_voltage;
	battery->pdata->swelling_normal_float_voltage =
		battery->pdata->chg_float_voltage;

	if (!battery->swelling_mode) {
 		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_PROP_VOLTAGE_MAX, value);
		if (value.intval > battery->pdata->chg_float_voltage) {
			value.intval = battery->pdata->chg_float_voltage;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_VOLTAGE_MAX, value);
		}
	}

	/* full/recharge condition */
	battery->pdata->recharge_condition_vcell =
		battery->pdata->age_data[battery->pdata->age_step].recharge_condition_vcell;
	battery->pdata->full_condition_soc =
		battery->pdata->age_data[battery->pdata->age_step].full_condition_soc;
	battery->pdata->full_condition_vcell =
		battery->pdata->age_data[battery->pdata->age_step].full_condition_vcell;

	value.intval = battery->pdata->full_condition_soc;
	psy_do_property(battery->pdata->fuelgauge_name, set,
		POWER_SUPPLY_PROP_CAPACITY_LEVEL, value);

#if defined(CONFIG_STEP_CHARGING)
	sec_bat_set_aging_info_step_charging(battery);
#endif

	dev_info(battery->dev,
		 "%s: Step(%d/%d), Cycle(%d), float_v(%d), r_v(%d), f_s(%d), f_vl(%d)\n",
		 __func__,
		 battery->pdata->age_step, battery->pdata->num_age_step-1, battery->batt_cycle,
		 battery->pdata->chg_float_voltage,
		 battery->pdata->recharge_condition_vcell,
		 battery->pdata->full_condition_soc,
		 battery->pdata->full_condition_vcell);

	return true;
}

void sec_bat_aging_check(struct sec_battery_info *battery)
{
	int prev_step = battery->pdata->age_step;
	int calc_step = -1;
	bool ret = 0;

	if ((battery->pdata->num_age_step <= 0) || (battery->batt_cycle <= 0))
		return;

	if (battery->temperature < 50) {
		pr_info("%s: [AGE] skip (temperature:%d)\n", __func__, battery->temperature);
		return;
	}

	for (calc_step = battery->pdata->num_age_step - 1; calc_step >= 0; calc_step--) {
		if (battery->pdata->age_data[calc_step].cycle <= battery->batt_cycle)
			break;
	}

	if (calc_step == prev_step)
		return;

	ret = sec_bat_set_aging_step(battery, calc_step);
	dev_info(battery->dev,
		 "%s: %s change step (%d->%d), Cycle(%d)\n",
		 __func__, ret ? "Succeed in" : "Fail to",
		 prev_step, battery->pdata->age_step, battery->batt_cycle);
}
#endif

void sec_bat_check_battery_health(struct sec_battery_info *battery)
{
	static battery_health_condition default_table[3] =
		{{.cycle = 900, .asoc = 75}, {.cycle = 1200, .asoc = 65}, {.cycle = 1500, .asoc = 55}};

	battery_health_condition *ptable = default_table;
	battery_health_condition state;
	int i, battery_health, size = BATTERY_HEALTH_MAX;

	if (battery->pdata->health_condition == NULL) {
		/*
		 * If a new type is added to misc_battery_health, default table cannot verify the actual state except "bad".
		 * If you want to modify to return the correct values for all states,
		 * add a table that matches the state added to the dt file.
		*/
		pr_info("%s: does not set health_condition_table, use default table\n", __func__);
		size = 3;
	} else {
		ptable = battery->pdata->health_condition;
	}

	/* Checking Cycle and ASoC */
	state.cycle = state.asoc = BATTERY_HEALTH_BAD;
	for (i = size - 1; i >= 0; i--) {
		if (ptable[i].cycle >= (battery->batt_cycle % 10000))
			state.cycle = i + BATTERY_HEALTH_GOOD;
		if (ptable[i].asoc <= battery->batt_asoc)
			state.asoc = i + BATTERY_HEALTH_GOOD;
	}
	battery_health = max(state.cycle, state.asoc);
	pr_info("%s: update battery_health(%d), (%d - %d)\n",
		__func__, battery_health, state.cycle, state.asoc);
	/* Update battery health */
	sec_bat_set_misc_event(battery,
		(battery_health << BATTERY_HEALTH_SHIFT), BATT_MISC_EVENT_BATTERY_HEALTH);
}

static bool sec_bat_temperature(
				struct sec_battery_info *battery)
{
	bool ret;
	ret = true;

	if (is_wireless_type(battery->cable_type) || battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_FAKE) {
		battery->temp_highlimit_threshold =
			battery->pdata->temp_highlimit_threshold_normal;
		battery->temp_highlimit_recovery =
			battery->pdata->temp_highlimit_recovery_normal;
		battery->temp_high_threshold =
			battery->pdata->wpc_high_threshold_normal;
		battery->temp_high_recovery =
			battery->pdata->wpc_high_recovery_normal;
		battery->temp_low_recovery =
			battery->pdata->wpc_low_recovery_normal;
		battery->temp_low_threshold =
			battery->pdata->wpc_low_threshold_normal;
	} else {
		if (lpcharge) {
			battery->temp_highlimit_threshold =
				battery->pdata->temp_highlimit_threshold_lpm;
			battery->temp_highlimit_recovery =
				battery->pdata->temp_highlimit_recovery_lpm;
			battery->temp_high_threshold =
				battery->pdata->temp_high_threshold_lpm;
			battery->temp_high_recovery =
				battery->pdata->temp_high_recovery_lpm;
			battery->temp_low_recovery =
				battery->pdata->temp_low_recovery_lpm;
			battery->temp_low_threshold =
				battery->pdata->temp_low_threshold_lpm;
		} else {
			battery->temp_highlimit_threshold =
				battery->pdata->temp_highlimit_threshold_normal;
			battery->temp_highlimit_recovery =
				battery->pdata->temp_highlimit_recovery_normal;
			battery->temp_high_threshold =
				battery->pdata->temp_high_threshold_normal;
			battery->temp_high_recovery =
				battery->pdata->temp_high_recovery_normal;
			battery->temp_low_recovery =
				battery->pdata->temp_low_recovery_normal;
			battery->temp_low_threshold =
				battery->pdata->temp_low_threshold_normal;
		}
	}

	return ret;
}

static bool sec_bat_temperature_check(
				struct sec_battery_info *battery)
{
	int pre_health = POWER_SUPPLY_HEALTH_GOOD;

	if (battery->status == POWER_SUPPLY_STATUS_DISCHARGING) {
		battery->health_change = false;
		dev_dbg(battery->dev,
			"%s: Charging Disabled\n", __func__);
		return true;
	}

	if (battery->health != POWER_SUPPLY_HEALTH_GOOD &&
		battery->health != POWER_SUPPLY_HEALTH_OVERHEAT &&
		battery->health != POWER_SUPPLY_HEALTH_COLD &&
		battery->health != POWER_SUPPLY_HEALTH_OVERHEATLIMIT) {
		dev_dbg(battery->dev, "%s: No need to check\n", __func__);
		return false;
	}

#if defined(CONFIG_ENG_BATTERY_CONCEPT) || defined(CONFIG_SEC_FACTORY)
	if (!battery->cooldown_mode) {
		dev_err(battery->dev, "%s: Forced temp check block\n", __func__);
		return true;
	}
#endif

	sec_bat_temperature(battery);

	if (battery->pdata->temp_check_type == SEC_BATTERY_TEMP_CHECK_NONE) {
		dev_err(battery->dev,
			"%s: Invalid Temp Check Type\n", __func__);
		return true;
	}
	pre_health = battery->health;

	if (battery->pdata->usb_temp_check_type && (battery->usb_temp >= battery->temp_highlimit_threshold)) {
		if (battery->health != POWER_SUPPLY_HEALTH_OVERHEATLIMIT) {
			if (battery->temp_highlimit_cnt <
			    battery->pdata->temp_check_count) {
				battery->temp_highlimit_cnt++;
				battery->temp_high_cnt = 0;
				battery->temp_low_cnt = 0;
				battery->temp_recover_cnt = 0;
			}
			dev_err(battery->dev,
				"%s: usb therm highlimit count = %d\n",
				__func__, battery->temp_highlimit_cnt);
		}
	} else if (battery->pdata->usb_temp_check_type && (battery->usb_temp > battery->temp_highlimit_recovery)
		&& (battery->health == POWER_SUPPLY_HEALTH_OVERHEATLIMIT)) {
			dev_err(battery->dev,
				"%s: usb therm highlimit \n",__func__);
	} else if (battery->temperature >= battery->temp_highlimit_threshold && !battery->pdata->usb_temp_check_type) {
		if (battery->health != POWER_SUPPLY_HEALTH_OVERHEATLIMIT) {
			if (battery->temp_highlimit_cnt <
			    battery->pdata->temp_check_count) {
				battery->temp_highlimit_cnt++;
				battery->temp_high_cnt = 0;
				battery->temp_low_cnt = 0;
				battery->temp_recover_cnt = 0;
			}
			dev_err(battery->dev,
				"%s: highlimit count = %d\n",
				__func__, battery->temp_highlimit_cnt);
		}
	} else if (battery->temperature >= battery->temp_high_threshold) {
		if (battery->health == POWER_SUPPLY_HEALTH_OVERHEATLIMIT && !battery->pdata->usb_temp_check_type) {
			if (battery->temperature <= battery->temp_highlimit_recovery) {
				if (battery->temp_recover_cnt <
				    battery->pdata->temp_check_count) {
					battery->temp_recover_cnt++;
					battery->temp_highlimit_cnt = 0;
					battery->temp_high_cnt = 0;
					battery->temp_low_cnt = 0;
				}
				dev_err(battery->dev,
					"%s: recovery count = %d\n",
					__func__, battery->temp_recover_cnt);
			}
		} else if (battery->health != POWER_SUPPLY_HEALTH_OVERHEAT) {
			if (battery->temp_high_cnt <
			    battery->pdata->temp_check_count) {
				battery->temp_high_cnt++;
				battery->temp_highlimit_cnt = 0;
				battery->temp_low_cnt = 0;
				battery->temp_recover_cnt = 0;
			}
			dev_err(battery->dev,
				"%s: high count = %d\n",
				__func__, battery->temp_high_cnt);
		}
	} else if ((battery->temperature <= battery->temp_high_recovery) &&
				(battery->temperature >= battery->temp_low_recovery)) {
		if (battery->health == POWER_SUPPLY_HEALTH_OVERHEAT ||
			battery->health == POWER_SUPPLY_HEALTH_OVERHEATLIMIT ||
		    battery->health == POWER_SUPPLY_HEALTH_COLD) {
			if (battery->temp_recover_cnt <
			    battery->pdata->temp_check_count) {
				battery->temp_recover_cnt++;
				battery->temp_highlimit_cnt = 0;
				battery->temp_high_cnt = 0;
				battery->temp_low_cnt = 0;
			}
			dev_err(battery->dev,
				"%s: recovery count = %d\n",
				__func__, battery->temp_recover_cnt);
		}
	} else if (battery->temperature <= battery->temp_low_threshold) {
		if (battery->health != POWER_SUPPLY_HEALTH_COLD) {
			if (battery->temp_low_cnt <
			    battery->pdata->temp_check_count) {
				battery->temp_low_cnt++;
				battery->temp_highlimit_cnt = 0;
				battery->temp_high_cnt = 0;
				battery->temp_recover_cnt = 0;
			}
			dev_err(battery->dev,
				"%s: low count = %d\n",
				__func__, battery->temp_low_cnt);
		}
	} else {
		battery->temp_highlimit_cnt = 0;
		battery->temp_high_cnt = 0;
		battery->temp_low_cnt = 0;
		battery->temp_recover_cnt = 0;
	}

#if defined(CONFIG_PREVENT_USB_CONN_OVERHEAT)
	if (battery->pdata->usb_thermal_source && !battery->usb_temp_flag) {
		int gap = 0;

		if (battery->usb_temp > battery->temperature)
			gap = battery->usb_temp - battery->temperature;

		if (battery->usb_temp >= battery->temp_highlimit_threshold) {
			pr_info("%s: Usb Temp over %d(%d)\n", __func__, battery->temp_highlimit_threshold, battery->usb_temp);
			battery->cisd.data[CISD_DATA_USB_OVERHEAT_CHARGING]++;
			battery->cisd.data[CISD_DATA_USB_OVERHEAT_CHARGING_PER_DAY]++;
			battery->usb_temp_flag = true;
		} else if ((battery->usb_temp >= battery->usb_protection_temp) && (gap >= battery->temp_gap_bat_usb)) {
			pr_info("%s: Temp gap between Usb temp and Bat temp : %d\n", __func__, gap);
			battery->usb_temp_flag = true;
			battery->cisd.data[CISD_DATA_USB_OVERHEAT_RAPID_CHANGE]++;
			battery->cisd.data[CISD_DATA_USB_OVERHEAT_RAPID_CHANGE_PER_DAY]++;

			if (gap > battery->cisd.data[CISD_DATA_USB_OVERHEAT_ALONE_PER_DAY])
				battery->cisd.data[CISD_DATA_USB_OVERHEAT_ALONE_PER_DAY] = gap;
		}

		if (battery->usb_temp_flag) {
			pr_info("%s: Usb temp flag %d\n", __func__, battery->usb_temp_flag);
			if (lpcharge && (battery->health != POWER_SUPPLY_HEALTH_OVERHEATLIMIT)) {
				battery->temp_highlimit_cnt = battery->pdata->temp_check_count;
			} else if (is_pd_wire_type(battery->cable_type)) {
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_BUCK_OFF);
				select_pdo(1);
				muic_set_hiccup_mode(1);
				pdic_manual_ccopen_request(1);
				return false;
			} else {
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_BUCK_OFF);
				muic_set_hiccup_mode(1);
				return false;
			}
		}
	}
#endif

	if (battery->temp_highlimit_cnt >=
	    battery->pdata->temp_check_count) {
#if defined(CONFIG_PREVENT_USB_CONN_OVERHEAT)
		if (lpcharge) {
			battery->health = POWER_SUPPLY_HEALTH_OVERHEATLIMIT;
			battery->temp_highlimit_cnt = 0;
		}
#else
		battery->health = POWER_SUPPLY_HEALTH_OVERHEATLIMIT;
		battery->temp_highlimit_cnt = 0;
#endif
	} else {
#if defined(CONFIG_PREVENT_USB_CONN_OVERHEAT)
			if (lpcharge && battery->usb_temp_flag)
				return true;				
#endif
		if (battery->temp_high_cnt >=
			battery->pdata->temp_check_count) {
			battery->health = POWER_SUPPLY_HEALTH_OVERHEAT;
			battery->temp_high_cnt = 0;
		} else if (battery->temp_low_cnt >=
			battery->pdata->temp_check_count) {
			battery->health = POWER_SUPPLY_HEALTH_COLD;
			battery->temp_low_cnt = 0;
		} else if (battery->temp_recover_cnt >=
			 battery->pdata->temp_check_count) {
			if (battery->health == POWER_SUPPLY_HEALTH_OVERHEATLIMIT &&
				battery->temperature > battery->temp_high_recovery) {
				battery->health = POWER_SUPPLY_HEALTH_OVERHEAT;
			} else {
				battery->health = POWER_SUPPLY_HEALTH_GOOD;
			}
			battery->temp_recover_cnt = 0;
		}
	}
	if (pre_health != battery->health) {
		battery->health_change = true;
		dev_info(battery->dev, "%s, health_change true\n", __func__);
	} else {
		battery->health_change = false;
	}

	if ((battery->health == POWER_SUPPLY_HEALTH_OVERHEAT) ||
		(battery->health == POWER_SUPPLY_HEALTH_COLD) ||
		(battery->health == POWER_SUPPLY_HEALTH_OVERHEATLIMIT)) {
		if (battery->health_change) {
			union power_supply_propval val = {0, };
			battery->is_abnormal_temp = true;
			if (is_wireless_type(battery->cable_type) || battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_FAKE) {
				val.intval = battery->health;
				psy_do_property(battery->pdata->wireless_charger_name, set,
						POWER_SUPPLY_PROP_HEALTH, val);
			}
			dev_info(battery->dev,
				"%s: Unsafe Temperature\n", __func__);
			sec_bat_set_charging_status(battery,
					POWER_SUPPLY_STATUS_NOT_CHARGING);
#if defined(CONFIG_BATTERY_CISD)
			battery->cisd.data[CISD_DATA_UNSAFETY_TEMPERATURE]++;
			battery->cisd.data[CISD_DATA_UNSAFE_TEMPERATURE_PER_DAY]++;
#endif

			if (battery->health == POWER_SUPPLY_HEALTH_OVERHEATLIMIT) {
				/* change charging current to battery (default 0mA) */
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_BUCK_OFF);
				if (is_hv_afc_wire_type(battery->cable_type) && !battery->vbus_limit) {
#if defined(CONFIG_SUPPORT_HV_CTRL)
					battery->vbus_chg_by_siop = SEC_INPUT_VOLTAGE_0V;
					muic_afc_set_voltage(SEC_INPUT_VOLTAGE_0V);
#endif
					battery->vbus_limit = true;
					pr_info("%s: Set AFC TA to 0V\n", __func__);
				} else if (is_pd_wire_type(battery->cable_type)) {
					select_pdo(1);
					pr_info("%s: Set PD TA to PDO 0\n", __func__);
				}
			} else if (battery->health == POWER_SUPPLY_HEALTH_OVERHEAT) {
				/* to discharge battery */
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_BUCK_OFF);
			} else {
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
			}

			return false;
		}
		/* dose not need buck control at low temperature */
		if (battery->health == POWER_SUPPLY_HEALTH_OVERHEAT) {
			if((battery->charger_mode == SEC_BAT_CHG_MODE_BUCK_OFF) &&
				(battery->voltage_now < (battery->pdata->swelling_drop_float_voltage / battery->pdata->chg_float_voltage_conv))) {
				pr_info("%s: Vnow(%dmV) < %dmV has dropped enough to get buck on mode \n", __func__,
					battery->voltage_now,
					(battery->pdata->swelling_drop_float_voltage / battery->pdata->chg_float_voltage_conv));
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
			}
		}
	} else {
		/* if recovered from not charging */
		if ((battery->health == POWER_SUPPLY_HEALTH_GOOD) &&
			(battery->status == POWER_SUPPLY_STATUS_NOT_CHARGING) &&
			!(battery->misc_event & BATT_MISC_EVENT_FULL_CAPACITY)) {
			battery->is_abnormal_temp = false;
			dev_info(battery->dev,
					"%s: Safe Temperature\n", __func__);
			if (battery->capacity >= 100)
				sec_bat_set_charging_status(battery,
						POWER_SUPPLY_STATUS_FULL);
			else	/* Normal Charging */
				sec_bat_set_charging_status(battery,
						POWER_SUPPLY_STATUS_CHARGING);
#if defined(CONFIG_BATTERY_SWELLING)
			if (battery->temperature > battery->pdata->swelling_high_temp_recov) {
				pr_info("%s: swelling mode start. stop charging\n", __func__);
				battery->swelling_mode = SWELLING_MODE_CHARGING;
				battery->swelling_full_check_cnt = 0;
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_BUCK_OFF);
				if (battery->temperature > battery->pdata->swelling_high_temp_recov) {
					sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING,
							SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING);
				}
			} else {
				union power_supply_propval val = {0, };
				/* restore 4.4V float voltage */
				val.intval = battery->pdata->swelling_normal_float_voltage;
				psy_do_property(battery->pdata->charger_name, set,
						POWER_SUPPLY_PROP_VOLTAGE_MAX, val);
				/* turn on charger by cable type */
				if((battery->status == POWER_SUPPLY_STATUS_FULL) &&
					(battery->charging_mode == SEC_BATTERY_CHARGING_NONE)) {
					sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
				} else {
					sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING);
				}

				if (pre_health == POWER_SUPPLY_HEALTH_COLD) {
					if (battery->temperature <= battery->pdata->swelling_low_temp_block_2nd) {
						sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND,
									SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
					} else if (battery->temperature <= battery->pdata->swelling_low_temp_block_1st) {
						sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING,
									SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
					} else if (battery->swelling_low_temp_3rd_ctrl && battery->temperature <= battery->pdata->swelling_low_temp_block_3rd) {
						sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_3RD,
									SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
					} else if (battery->swelling_low_temp_4th_ctrl && battery->temperature <= battery->pdata->swelling_low_temp_block_4th) {
						sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_4TH,
									SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
					} else if (battery->swelling_low_temp_5th_ctrl && battery->temperature <= battery->pdata->swelling_low_temp_block_5th) {
						sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_5TH,
									SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
					}
				}
			}
#else
			/* turn on charger by cable type */
			if((battery->status == POWER_SUPPLY_STATUS_FULL) &&
				(battery->charging_mode == SEC_BATTERY_CHARGING_NONE)) {
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
			} else {
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING);
			}
#endif
			return false;
		}
	}
	return true;
}

static bool sec_bat_check_fullcharged_condition(struct sec_battery_info *battery) {
	int full_check_type = SEC_BATTERY_FULLCHARGED_NONE;

	if (battery->charging_mode == SEC_BATTERY_CHARGING_1ST)
		full_check_type = battery->pdata->full_check_type;
	else
		full_check_type = battery->pdata->full_check_type_2nd;

	switch (full_check_type) {
	case SEC_BATTERY_FULLCHARGED_ADC:
	case SEC_BATTERY_FULLCHARGED_FG_CURRENT:
	case SEC_BATTERY_FULLCHARGED_SOC:
	case SEC_BATTERY_FULLCHARGED_CHGGPIO:
	case SEC_BATTERY_FULLCHARGED_CHGPSY:
		break;

	/* If these is NOT full check type or NONE full check type,
	 * it is full-charged
	 */
	case SEC_BATTERY_FULLCHARGED_CHGINT:
	case SEC_BATTERY_FULLCHARGED_TIME:
	case SEC_BATTERY_FULLCHARGED_NONE:
	default:
		return true;
		break;
	}

#if defined(CONFIG_ENABLE_FULL_BY_SOC)
	if (battery->capacity >= 100 &&
		!battery->is_recharging) {
		dev_info(battery->dev,
			"%s: enough SOC (%d%%), skip other full_condition_type\n",
			__func__, battery->capacity);
		return true;
	}
#endif

	if (battery->pdata->full_condition_type &
		SEC_BATTERY_FULL_CONDITION_SOC) {
		if (battery->capacity <
			battery->pdata->full_condition_soc) {
			dev_dbg(battery->dev,
				"%s: Not enough SOC (%d%%)\n",
				__func__, battery->capacity);
			return false;
		}
	}

	if (battery->pdata->full_condition_type &
		SEC_BATTERY_FULL_CONDITION_VCELL) {
		if (battery->voltage_now <
			battery->pdata->full_condition_vcell) {
			dev_dbg(battery->dev,
				"%s: Not enough VCELL (%dmV)\n",
				__func__, battery->voltage_now);
			return false;
		}
	}

	if (battery->pdata->full_condition_type &
		SEC_BATTERY_FULL_CONDITION_AVGVCELL) {
		if (battery->voltage_avg <
			battery->pdata->full_condition_avgvcell) {
			dev_dbg(battery->dev,
				"%s: Not enough AVGVCELL (%dmV)\n",
				__func__, battery->voltage_avg);
			return false;
		}
	}

	if (battery->pdata->full_condition_type &
		SEC_BATTERY_FULL_CONDITION_OCV) {
		if (battery->voltage_ocv <
			battery->pdata->full_condition_ocv) {
			dev_dbg(battery->dev,
				"%s: Not enough OCV (%dmV)\n",
				__func__, battery->voltage_ocv);
			return false;
		}
	}

	return true;
}

static void sec_bat_do_test_function(struct sec_battery_info *battery) {
	union power_supply_propval value = {0, };

	switch (battery->test_mode) {
		case 1:
			if (battery->status == POWER_SUPPLY_STATUS_CHARGING) {
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
				sec_bat_set_charging_status(battery,
						POWER_SUPPLY_STATUS_DISCHARGING);
			}
			break;
		case 2:
			if(battery->status == POWER_SUPPLY_STATUS_DISCHARGING) {
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING);
				psy_do_property(battery->pdata->charger_name, get,
						POWER_SUPPLY_PROP_STATUS, value);
				sec_bat_set_charging_status(battery, value.intval);
			}
			battery->test_mode = 0;
			break;
		case 3: // clear temp block
			battery->health = POWER_SUPPLY_HEALTH_GOOD;
			sec_bat_set_charging_status(battery,
					POWER_SUPPLY_STATUS_DISCHARGING);
			break;
		case 4:
			if(battery->status == POWER_SUPPLY_STATUS_DISCHARGING) {
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING);
				psy_do_property(battery->pdata->charger_name, get,
						POWER_SUPPLY_PROP_STATUS, value);
				sec_bat_set_charging_status(battery, value.intval);
			}
			break;
		default:
			pr_info("%s: error test: unknown state\n", __func__);
			break;
	}
}

static bool sec_bat_time_management(struct sec_battery_info *battery) {
	struct timespec ts = {0, };
	unsigned long charging_time;

	if (battery->charging_start_time == 0 || !battery->safety_timer_set) {
		dev_dbg(battery->dev,
			"%s: Charging Disabled\n", __func__);
		return true;
	}

	get_monotonic_boottime(&ts);

	if (ts.tv_sec >= battery->charging_start_time) {
		charging_time = ts.tv_sec - battery->charging_start_time;
	} else {
		charging_time = 0xFFFFFFFF - battery->charging_start_time
			+ ts.tv_sec;
	}

	battery->charging_passed_time = charging_time;

	switch (battery->status) {
	case POWER_SUPPLY_STATUS_FULL:
		if (battery->expired_time == 0) {
			dev_info(battery->dev,
				"%s: Recharging Timer Expired\n", __func__);
			battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
			battery->health = POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE;
			sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);
			battery->is_recharging = false;
			if (sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF)) {
				dev_err(battery->dev,
					"%s: Fail to Set Charger\n", __func__);
				return true;
			}

			return false;
		}
		break;
	case POWER_SUPPLY_STATUS_CHARGING:
		if ((battery->pdata->full_condition_type &
		     SEC_BATTERY_FULL_CONDITION_NOTIMEFULL) &&
		    (battery->is_recharging && (battery->expired_time == 0))) {
			dev_info(battery->dev,
			"%s: Recharging Timer Expired\n", __func__);
			battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
			battery->health = POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE;
			sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);
			battery->is_recharging = false;
			if (sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF)) {
				dev_err(battery->dev,
					"%s: Fail to Set Charger\n", __func__);
				return true;
			}
			return false;
		} else if (!battery->is_recharging &&
			   (battery->expired_time == 0)) {
			dev_info(battery->dev,
				"%s: Charging Timer Expired\n", __func__);
			battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
			battery->health = POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE;
			sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);
#if defined(CONFIG_BATTERY_CISD)
			battery->cisd.data[CISD_DATA_SAFETY_TIMER]++;
			battery->cisd.data[CISD_DATA_SAFETY_TIMER_PER_DAY]++;
#endif
#if defined(CONFIG_SEC_ABC)
			sec_abc_send_event("MODULE=battery@ERROR=safety_timer");
#endif
			if (sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF)) {
				dev_err(battery->dev,
					"%s: Fail to Set Charger\n", __func__);
				return true;
			}
			return false;
		}
		break;
	default:
		dev_err(battery->dev,
			"%s: Undefine Battery Status\n", __func__);
		return true;
	}

	return true;
}

static bool sec_bat_check_fullcharged(struct sec_battery_info *battery) {
	union power_supply_propval value = {0, };
	int current_adc = 0;
	int full_check_type = SEC_BATTERY_FULLCHARGED_NONE;
	bool ret = false;
	int err = 0;

	if (!sec_bat_check_fullcharged_condition(battery))
		goto not_full_charged;

	if (battery->charging_mode == SEC_BATTERY_CHARGING_1ST)
		full_check_type = battery->pdata->full_check_type;
	else
		full_check_type = battery->pdata->full_check_type_2nd;

	switch (full_check_type) {
	case SEC_BATTERY_FULLCHARGED_ADC:
		current_adc =
			sec_bat_get_adc_data(battery,
			SEC_BAT_ADC_CHANNEL_FULL_CHECK,
			battery->pdata->adc_check_count);

		dev_dbg(battery->dev,
			"%s: Current ADC (%d)\n",
			__func__, current_adc);

		if (current_adc < 0)
			break;
		battery->current_adc = current_adc;

		if (battery->current_adc <
			(battery->charging_mode ==
			SEC_BATTERY_CHARGING_1ST ?
			battery->pdata->full_check_current_1st :
			battery->pdata->full_check_current_2nd)) {
			battery->full_check_cnt++;
			dev_dbg(battery->dev,
				"%s: Full Check ADC (%d)\n",
				__func__,
				battery->full_check_cnt);
		} else
			battery->full_check_cnt = 0;
		break;

	case SEC_BATTERY_FULLCHARGED_FG_CURRENT:
		if ((battery->current_now > 0 && battery->current_now <
			battery->pdata->full_check_current_1st) &&
			(battery->current_avg > 0 && battery->current_avg <
			(battery->charging_mode ==
			SEC_BATTERY_CHARGING_1ST ?
			battery->pdata->full_check_current_1st :
			battery->pdata->full_check_current_2nd))) {
				battery->full_check_cnt++;
				dev_dbg(battery->dev,
				"%s: Full Check Current (%d)\n",
				__func__,
				battery->full_check_cnt);
		} else
			battery->full_check_cnt = 0;
		break;

	case SEC_BATTERY_FULLCHARGED_TIME:
		if ((battery->charging_mode ==
			SEC_BATTERY_CHARGING_2ND ?
			(battery->charging_passed_time -
			battery->charging_fullcharged_time) :
			battery->charging_passed_time) >
			(battery->charging_mode ==
			SEC_BATTERY_CHARGING_1ST ?
			battery->pdata->full_check_current_1st :
			battery->pdata->full_check_current_2nd)) {
			battery->full_check_cnt++;
			dev_dbg(battery->dev,
				"%s: Full Check Time (%d)\n",
				__func__,
				battery->full_check_cnt);
		} else
			battery->full_check_cnt = 0;
		break;

	case SEC_BATTERY_FULLCHARGED_SOC:
		if (battery->capacity <=
			(battery->charging_mode ==
			SEC_BATTERY_CHARGING_1ST ?
			battery->pdata->full_check_current_1st :
			battery->pdata->full_check_current_2nd)) {
			battery->full_check_cnt++;
			dev_dbg(battery->dev,
				"%s: Full Check SOC (%d)\n",
				__func__,
				battery->full_check_cnt);
		} else
			battery->full_check_cnt = 0;
		break;

	case SEC_BATTERY_FULLCHARGED_CHGGPIO:
		err = gpio_request(
			battery->pdata->chg_gpio_full_check,
			"GPIO_CHG_FULL");
		if (err) {
			dev_err(battery->dev,
				"%s: Error in Request of GPIO\n", __func__);
			break;
		}
		if (!(gpio_get_value_cansleep(
			battery->pdata->chg_gpio_full_check) ^
			!battery->pdata->chg_polarity_full_check)) {
			battery->full_check_cnt++;
			dev_dbg(battery->dev,
				"%s: Full Check GPIO (%d)\n",
				__func__, battery->full_check_cnt);
		} else
			battery->full_check_cnt = 0;
		gpio_free(battery->pdata->chg_gpio_full_check);
		break;

	case SEC_BATTERY_FULLCHARGED_CHGINT:
	case SEC_BATTERY_FULLCHARGED_CHGPSY:
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_PROP_STATUS, value);

		if (value.intval == POWER_SUPPLY_STATUS_FULL) {
			battery->full_check_cnt++;
			dev_info(battery->dev,
				"%s: Full Check Charger (%d)\n",
				__func__, battery->full_check_cnt);
		} else
			battery->full_check_cnt = 0;
		break;

	/* If these is NOT full check type or NONE full check type,
	 * it is full-charged
	 */
	case SEC_BATTERY_FULLCHARGED_NONE:
		battery->full_check_cnt = 0;
		ret = true;
		break;
	default:
		dev_err(battery->dev,
			"%s: Invalid Full Check\n", __func__);
		break;
	}

#if defined(CONFIG_ENABLE_FULL_BY_SOC)
	if (battery->capacity >= 100 &&
		battery->charging_mode == SEC_BATTERY_CHARGING_1ST &&
		!battery->is_recharging) {
		battery->full_check_cnt++;
		dev_info(battery->dev,
			"%s: enough SOC to make FULL(%d%%)\n",
			__func__, battery->capacity);
	}
#endif

	if (battery->full_check_cnt >=
		battery->pdata->full_check_count) {
		battery->full_check_cnt = 0;
		ret = true;
	}

not_full_charged:
	return ret;
}

static void sec_bat_do_fullcharged(struct sec_battery_info *battery, bool force_fullcharged) {
	union power_supply_propval value = {0, };

	/* To let charger/fuel gauge know the full status,
	 * set status before calling sec_bat_set_charge()
	 */
#if defined(CONFIG_BATTERY_CISD)
	if (battery->status != POWER_SUPPLY_STATUS_FULL) {
		battery->cisd.data[CISD_DATA_FULL_COUNT]++;
		battery->cisd.data[CISD_DATA_FULL_COUNT_PER_DAY]++;
	}
#endif
	sec_bat_set_charging_status(battery,
			POWER_SUPPLY_STATUS_FULL);

	if (battery->charging_mode == SEC_BATTERY_CHARGING_1ST &&
		battery->pdata->full_check_type_2nd != SEC_BATTERY_FULLCHARGED_NONE && !force_fullcharged) {
		battery->charging_mode = SEC_BATTERY_CHARGING_2ND;
		battery->charging_fullcharged_time = battery->charging_passed_time;
		value.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;
		psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
		sec_bat_set_charging_current(battery);
		sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING);
		pr_info("%s: 1st charging is done\n", __func__);
	} else {
		battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
		battery->is_recharging = false;

		if (!battery->wdt_kick_disable) {
			pr_info("%s: wdt kick enable -> Charger Off, %d\n",
					__func__, battery->wdt_kick_disable);
			sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
			pr_info("%s: 2nd charging is done\n", __func__);
		} else {
			pr_info("%s: wdt kick disabled -> skip charger off, %d\n",
					__func__, battery->wdt_kick_disable);
		}

#if defined(CONFIG_BATTERY_AGE_FORECAST)
		sec_bat_aging_check(battery);
#endif

		/* this concept is only for power-off charging mode*/
		if (is_hv_wire_type(battery->cable_type) && is_hv_wire_type(battery->wire_status) &&
			!battery->store_mode && (battery->cable_type != SEC_BATTERY_CABLE_QC30) &&
			lpcharge && !battery->vbus_chg_by_full) {
			/* vbus level : 9V --> 5V */
			battery->vbus_chg_by_full = true;
			battery->vbus_chg_by_siop = SEC_INPUT_VOLTAGE_5V;
			muic_afc_set_voltage(SEC_INPUT_VOLTAGE_5V/10);
			pr_info("%s: vbus is set 5V by 2nd full\n", __func__);
		}

		value.intval = POWER_SUPPLY_STATUS_FULL;
		psy_do_property(battery->pdata->fuelgauge_name, set,
			POWER_SUPPLY_PROP_STATUS, value);
	}

	/* platform can NOT get information of battery
	 * because wakeup time is too short to check uevent
	 * To make sure that target is wakeup if full-charged,
	 * activated wake lock in a few seconds
	 */
	if (battery->pdata->polling_type == SEC_BATTERY_MONITOR_ALARM)
		wake_lock_timeout(&battery->vbus_wake_lock, HZ * 10);
}

static bool sec_bat_fullcharged_check(struct sec_battery_info *battery) {
	if ((battery->charging_mode == SEC_BATTERY_CHARGING_NONE) ||
		(battery->status == POWER_SUPPLY_STATUS_NOT_CHARGING)) {
		dev_dbg(battery->dev,
			"%s: No Need to Check Full-Charged\n", __func__);
		return true;
	}

	if (sec_bat_check_fullcharged(battery)) {
		union power_supply_propval value = {0, };
		if (battery->capacity < 100) {
			/* update capacity max */
			value.intval = battery->capacity;
			psy_do_property(battery->pdata->fuelgauge_name, set,
					POWER_SUPPLY_PROP_CHARGE_FULL, value);
			pr_info("%s : forced full-charged sequence for the capacity(%d)\n",
					__func__, battery->capacity);
			battery->full_check_cnt = battery->pdata->full_check_count;
		} else {
			sec_bat_do_fullcharged(battery, false);
		}
	}

	dev_info(battery->dev,
		"%s: Charging Mode : %s\n", __func__,
		battery->is_recharging ?
		sec_bat_charging_mode_str[SEC_BATTERY_CHARGING_RECHARGING] :
		sec_bat_charging_mode_str[battery->charging_mode]);

	return true;
}

#if !defined(CONFIG_SEC_FACTORY)
static void sec_bat_calc_unknown_wpc_temp(
	struct sec_battery_info *battery, int *batt_temp, int wpc_temp, int usb_temp)
{
	if (battery->support_unknown_wpcthm && battery->pdata->wpc_thermal_source &&
		!(is_wireless_type(battery->cable_type) ||
		(battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_FAKE))) {
		if (wpc_temp <= (-200)) {
			if (battery->swelling_low_temp_5th_ctrl) {
				if (usb_temp >= 330)
					*batt_temp = (usb_temp + 60) > 900 ? 900 : (usb_temp + 60);
				else if (usb_temp <= 270)
					*batt_temp = (usb_temp - 50) < (-200) ? (-200) : (usb_temp - 50);
				else
					*batt_temp = (170 * usb_temp - 32700) / 60;
			} else {
				if (usb_temp >= 270)
					*batt_temp = (usb_temp + 60) > 900 ? 900 : (usb_temp + 60);
				else if (usb_temp <= 210)
					*batt_temp = (usb_temp - 50) < (-200) ? (-200) : (usb_temp - 50);
				else
					*batt_temp = (170 * usb_temp - 26100) / 60;
			}
		}
	}
}
#endif

static void sec_bat_get_temperature_info(struct sec_battery_info *battery) {
	union power_supply_propval value = {0, };
	static bool shipmode_en = false;
	int batt_temp = battery->temperature;
	int usb_temp = battery->usb_temp;
	int chg_temp = battery->chg_temp;
#if defined(CONFIG_DIRECT_CHARGING)
	int dchg_temp = battery->dchg_temp;
#endif
	int wpc_temp = battery->wpc_temp;
	int slave_temp = battery->slave_chg_temp;

	/* get battery thm info */
	switch (battery->pdata->thermal_source) {
	case SEC_BATTERY_THERMAL_SOURCE_FG:
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_TEMP, value);
		batt_temp = value.intval;

		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_TEMP_AMBIENT, value);
		battery->temper_amb = value.intval;
		break;
	case SEC_BATTERY_THERMAL_SOURCE_CALLBACK:
		if (battery->pdata->get_temperature_callback) {
			battery->pdata->get_temperature_callback(
				POWER_SUPPLY_PROP_TEMP, &value);
			batt_temp = value.intval;
			psy_do_property(battery->pdata->fuelgauge_name, set,
				POWER_SUPPLY_PROP_TEMP, value);

			battery->pdata->get_temperature_callback(
				POWER_SUPPLY_PROP_TEMP_AMBIENT, &value);
			battery->temper_amb = value.intval;
			psy_do_property(battery->pdata->fuelgauge_name, set,
				POWER_SUPPLY_PROP_TEMP_AMBIENT, value);
		}
		break;
	case SEC_BATTERY_THERMAL_SOURCE_ADC:
		if(sec_bat_get_value_by_adc(battery,
				SEC_BAT_ADC_CHANNEL_TEMP, &value, battery->pdata->temp_check_type)) {
			batt_temp = value.intval;
			battery->temper_amb = value.intval;
		} else {
			batt_temp = 0;
			battery->temper_amb = 0;
		}
		break;
	default:
		break;
	}

	/* get usb thm info */
	switch (battery->pdata->usb_thermal_source) {
	case SEC_BATTERY_THERMAL_SOURCE_FG:
	case SEC_BATTERY_THERMAL_SOURCE_CALLBACK:
		break;
	case SEC_BATTERY_THERMAL_SOURCE_ADC:
		if(sec_bat_get_value_by_adc(battery,
			SEC_BAT_ADC_CHANNEL_USB_TEMP, &value, battery->pdata->usb_temp_check_type)) {
			usb_temp = value.intval;

			/* this shoud be moved */
			if (battery->vbus_limit && usb_temp <= battery->temp_highlimit_recovery)
				battery->vbus_limit = false;
		} else
			usb_temp = 0;
		break;
	default:
		break;
	}

	/* get chg thm info */
	switch (battery->pdata->chg_thermal_source) {
	case SEC_BATTERY_THERMAL_SOURCE_FG:
	case SEC_BATTERY_THERMAL_SOURCE_CALLBACK:
		break;
	case SEC_BATTERY_THERMAL_SOURCE_ADC:
		if(sec_bat_get_value_by_adc(battery,
			SEC_BAT_ADC_CHANNEL_CHG_TEMP, &value, battery->pdata->chg_temp_check_type)) {
			chg_temp = value.intval;
		} else
			chg_temp = 0;
		break;
	default:
		break;
	}

#if defined(CONFIG_DIRECT_CHARGING)
	if (is_pd_apdo_wire_type(battery->wire_status)) {
		switch (battery->pdata->dchg_thermal_source) {
		case SEC_BATTERY_THERMAL_SOURCE_CHG_ADC:
			psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_PROP_TEMP, value);

			dchg_temp = sec_bat_get_direct_chg_temp_adc(battery,
								value.intval, battery->pdata->adc_check_count);
			break;
		case SEC_BATTERY_THERMAL_SOURCE_CALLBACK:
		case SEC_BATTERY_THERMAL_SOURCE_ADC:
			break;
		default:
			break;
		}
	}
#endif

	/* get wpc thm info */
	switch (battery->pdata->wpc_thermal_source) {
	case SEC_BATTERY_THERMAL_SOURCE_FG:
	case SEC_BATTERY_THERMAL_SOURCE_CALLBACK:
		break;
	case SEC_BATTERY_THERMAL_SOURCE_ADC:
		if(sec_bat_get_value_by_adc(battery,
			SEC_BAT_ADC_CHANNEL_WPC_TEMP, &value, battery->pdata->wpc_temp_check_type)) {
			wpc_temp = value.intval;
		} else
			wpc_temp = 0;
		break;
	default:
		break;
	}

	/* get slave thm info */
	switch (battery->pdata->slave_thermal_source) {
	case SEC_BATTERY_THERMAL_SOURCE_FG:
	case SEC_BATTERY_THERMAL_SOURCE_CALLBACK:
		break;
	case SEC_BATTERY_THERMAL_SOURCE_ADC:
		if(sec_bat_get_value_by_adc(battery,
			SEC_BAT_ADC_CHANNEL_SLAVE_CHG_TEMP, &value, battery->pdata->slave_chg_temp_check_type)) {
			slave_temp = value.intval;

			/* set temperature */
			value.intval = (slave_temp << 16) | chg_temp;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_TEMP, value);
		} else
			slave_temp = 0;
		break;
	default:
		break;
	}

#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	if (battery->temperature_test_battery > -300 && battery->temperature_test_battery < 3000) {
		pr_info("%s : battery temperature test %d\n", __func__, battery->temperature_test_battery);
		batt_temp = battery->temperature_test_battery;
	}
	if (battery->temperature_test_usb > -300 && battery->temperature_test_usb < 3000) {
		pr_info("%s : usb temperature test %d\n", __func__, battery->temperature_test_usb);
		usb_temp = battery->temperature_test_usb;
	}
	if (battery->temperature_test_wpc > -300 && battery->temperature_test_wpc < 3000) {
		pr_info("%s : wpc temperature test %d\n", __func__, battery->temperature_test_wpc);
		wpc_temp = battery->temperature_test_wpc;
	}
	if (battery->temperature_test_chg > -300 && battery->temperature_test_chg < 3000) {
		pr_info("%s : chg temperature test %d\n", __func__, battery->temperature_test_chg);
		chg_temp = battery->temperature_test_chg;
	}
#if defined(CONFIG_DIRECT_CHARGING)
	if (battery->temperature_test_dchg > -300 && battery->temperature_test_dchg < 3000) {
		pr_info("%s : direct chg temperature test %d\n", __func__, battery->temperature_test_dchg);
		dchg_temp = battery->temperature_test_dchg;
	}
#endif
#endif

#if !defined(CONFIG_SEC_FACTORY)
	sec_bat_calc_unknown_wpc_temp(battery, &batt_temp, wpc_temp, usb_temp);
#endif

	battery->temperature = batt_temp;
	battery->usb_temp = usb_temp;
	battery->chg_temp = chg_temp;
#if defined(CONFIG_DIRECT_CHARGING)
	battery->dchg_temp = dchg_temp;
#endif
	battery->wpc_temp = wpc_temp;
	battery->slave_chg_temp = slave_temp;

#if defined(CONFIG_SEC_FACTORY)
	if (battery->pdata->usb_temp_check_type) {
		if (battery->temperature <= (-200))
			value.intval = (battery->usb_temp <= (-200) ? battery->chg_temp : battery->usb_temp);
		else
			value.intval = battery->temperature;
	}
#else
	value.intval = battery->temperature;
#endif
	psy_do_property(battery->pdata->fuelgauge_name, set,
		POWER_SUPPLY_PROP_TEMP, value);

	psy_do_property(battery->pdata->fuelgauge_name, set,
		POWER_SUPPLY_PROP_TEMP_AMBIENT, value);

	if (!battery->pdata->dis_auto_shipmode_temp_ctrl) {
		if (battery->temperature < 0 && !shipmode_en) {
			value.intval = 0;
			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_AUTO_SHIPMODE_CONTROL, value);
			shipmode_en = true;
		} else if (battery->temperature >= 50 && shipmode_en) {
			value.intval = 1;
			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_AUTO_SHIPMODE_CONTROL, value);
			shipmode_en = false;
		}
	}
}

void sec_bat_get_battery_info(struct sec_battery_info *battery) {
	union power_supply_propval value = {0, };

	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
	battery->voltage_now = value.intval;

	value.intval = SEC_BATTERY_VOLTAGE_AVERAGE;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_VOLTAGE_AVG, value);
	battery->voltage_avg = value.intval;

	/* Do not call it to reduce time after cable_work, this funtion call FG full log*/
	if (!(battery->current_event & SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL)) {
		value.intval = SEC_BATTERY_VOLTAGE_OCV;
		psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_VOLTAGE_AVG, value);
		battery->voltage_ocv = value.intval;
	}

	value.intval = SEC_BATTERY_CURRENT_MA;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_CURRENT_NOW, value);
	battery->current_now = value.intval;

	value.intval = SEC_BATTERY_CURRENT_MA;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_CURRENT_AVG, value);
	battery->current_avg = value.intval;

	value.intval = SEC_BATTERY_ISYS_AVG_MA;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_EXT_PROP_MEASURE_SYS, value);
	battery->current_sys_avg = value.intval;

	value.intval = SEC_BATTERY_ISYS_MA;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_EXT_PROP_MEASURE_SYS, value);
	battery->current_sys = value.intval;

	/* input current limit in charger */
	psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_PROP_CURRENT_MAX, value);
	battery->current_max = value.intval;

	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_CHARGE_COUNTER, value);
	battery->charge_counter = value.intval;

	/* check abnormal status for wireless charging */
	if (!(battery->current_event & SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL) &&
		(is_wireless_type(battery->cable_type) ||battery->wc_tx_enable)) {
		value.intval = (battery->status == POWER_SUPPLY_STATUS_FULL) ?
			100 : battery->capacity;
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);
	}

	value.intval = (battery->status == POWER_SUPPLY_STATUS_FULL) ?
		100 : battery->capacity;
	psy_do_property(battery->pdata->wireless_charger_name, set,
		POWER_SUPPLY_PROP_CAPACITY, value);

	sec_bat_get_temperature_info(battery);

	/* To get SOC value (NOT raw SOC), need to reset value */
	value.intval = 0;
	psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CAPACITY, value);
	/* if the battery status was full, and SOC wasn't 100% yet,
		then ignore FG SOC, and report (previous SOC +1)% */
	battery->capacity = value.intval;

#if defined(CONFIG_DIRECT_CHARGING)
	pr_info("%s:Vnow(%dmV),Vavg(%dmV),Inow(%dmA),Iavg(%dmA),Isysavg(%dmA),Imax(%dmA),Ichg(%dmA),SOC(%d%%),"
		"Tbat(%d),Tusb(%d),Tchg(%d),Twpc(%d),Tdchg(%d) lrp(%d)\n", __func__,
		battery->voltage_now, battery->voltage_avg, battery->current_now,
		battery->current_avg, battery->current_sys_avg,
		battery->current_max, battery->charging_current,
		battery->capacity, battery->temperature,
		battery->usb_temp, battery->chg_temp, battery->wpc_temp, battery->dchg_temp, battery->lrp
	);
#else
	pr_info("%s:Vnow(%dmV),Vavg(%dmV),Inow(%dmA),Iavg(%dmA),Isysavg(%dmA),Imax(%dmA),Ichg(%dmA),SOC(%d%%),"
		"Tbat(%d),Tusb(%d),Tchg(%d),Twpc(%d) lrp(%d)\n", __func__,
		battery->voltage_now, battery->voltage_avg, battery->current_now,
		battery->current_avg, battery->current_sys_avg,
		battery->current_max, battery->charging_current,
		battery->capacity, battery->temperature,
		battery->usb_temp, battery->chg_temp, battery->wpc_temp, battery->lrp
	);
#endif
	dev_dbg(battery->dev,
		"%s,Vavg(%dmV),Vocv(%dmV),Tamb(%d),"
		"Iavg(%dmA),Iadc(%d)\n",
		battery->present ? "Connected" : "Disconnected",
		battery->voltage_avg, battery->voltage_ocv,
		battery->temper_amb,
		battery->current_avg, battery->current_adc);

#if defined(CONFIG_SEC_DEBUG_EXTRA_INFO)
	sec_debug_set_extra_info_batt(battery->capacity, battery->voltage_avg, battery->temperature, battery->current_avg);
#endif
}

static void sec_bat_polling_work(struct work_struct *work) {
	struct sec_battery_info *battery = container_of(
		work, struct sec_battery_info, polling_work.work);

	wake_lock(&battery->monitor_wake_lock);
	queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
	dev_dbg(battery->dev, "%s: Activated\n", __func__);
}

static void sec_bat_program_alarm(struct sec_battery_info *battery, int seconds) {
	alarm_start(&battery->polling_alarm,
		    ktime_add(battery->last_poll_time, ktime_set(seconds, 0)));
}

static unsigned int sec_bat_get_polling_time(struct sec_battery_info *battery) {
	if (battery->status ==
		POWER_SUPPLY_STATUS_FULL)
		battery->polling_time =
			battery->pdata->polling_time[
			POWER_SUPPLY_STATUS_CHARGING];
	else
		battery->polling_time =
			battery->pdata->polling_time[
			battery->status];

	battery->polling_short = true;

	switch (battery->status) {
	case POWER_SUPPLY_STATUS_CHARGING:
		if (battery->polling_in_sleep)
			battery->polling_short = false;
		break;
	case POWER_SUPPLY_STATUS_DISCHARGING:
		if (battery->polling_in_sleep && (battery->ps_enable != true)) {
			battery->polling_time =
				battery->pdata->polling_time[
					SEC_BATTERY_POLLING_TIME_SLEEP];
		} else
			battery->polling_time =
				battery->pdata->polling_time[
				battery->status];
		if (!battery->wc_enable) {
			battery->polling_time = battery->pdata->polling_time[
					SEC_BATTERY_POLLING_TIME_CHARGING];
			pr_info("%s: wc_enable is false, polling time is 30sec\n", __func__);
		}

		battery->polling_short = false;
		break;
	case POWER_SUPPLY_STATUS_FULL:
		if (battery->polling_in_sleep) {
			if (!(battery->pdata->full_condition_type &
				SEC_BATTERY_FULL_CONDITION_NOSLEEPINFULL) &&
				battery->charging_mode ==
				SEC_BATTERY_CHARGING_NONE) {
				battery->polling_time =
					battery->pdata->polling_time[
						SEC_BATTERY_POLLING_TIME_SLEEP];
			}
			battery->polling_short = false;
		} else {
			if (battery->charging_mode ==
				SEC_BATTERY_CHARGING_NONE)
				battery->polling_short = false;
		}
		break;
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		if ((battery->health == POWER_SUPPLY_HEALTH_OVERVOLTAGE ||
			(battery->health == POWER_SUPPLY_HEALTH_UNDERVOLTAGE)) &&
			(battery->health_check_count > 0)) {
			battery->health_check_count--;
			battery->polling_time = 1;
			battery->polling_short = false;
		}
		break;
	}

#if defined(CONFIG_WIRELESS_TX_MODE)
	if (battery->wc_tx_enable) {
		battery->polling_time = 10;
		battery->polling_short = false;
		pr_info("%s: Tx mode enable polling time is 10sec\n", __func__);
	}
#endif

#if defined(CONFIG_PDIC_PD30)
	if (is_pd_apdo_wire_type(battery->cable_type) &&
		(battery->pd_list.now_isApdo || battery->ta_alert_mode != OCP_NONE)) {
		battery->polling_time = 10;
		battery->polling_short = false;
		pr_info("%s: DC mode enable polling time is 10sec\n", __func__);
	}
#endif

	if (battery->polling_short)
		return battery->pdata->polling_time[
			SEC_BATTERY_POLLING_TIME_BASIC];
	/* set polling time to 46s to reduce current noise on wc */
	else if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS &&
			battery->status == POWER_SUPPLY_STATUS_CHARGING)
		battery->polling_time = 46;

	return battery->polling_time;
}

static bool sec_bat_is_short_polling(struct sec_battery_info *battery) {
	/* Change the full and short monitoring sequence
	 * Originally, full monitoring was the last time of polling_count
	 * But change full monitoring to first time
	 * because temperature check is too late
	 */
	if (!battery->polling_short || battery->polling_count == 1)
		return false;
	else
		return true;
}

static void sec_bat_update_polling_count(
	struct sec_battery_info *battery)
{
	/* do NOT change polling count in sleep
	 * even though it is short polling
	 * to keep polling count along sleep/wakeup
	 */
	if (battery->polling_short && battery->polling_in_sleep)
		return;

	if (battery->polling_short &&
		((battery->polling_time /
		battery->pdata->polling_time[
		SEC_BATTERY_POLLING_TIME_BASIC])
		> battery->polling_count))
		battery->polling_count++;
	else
		battery->polling_count = 1;	/* initial value = 1 */
}

static void sec_bat_set_polling(
	struct sec_battery_info *battery)
{
	unsigned int polling_time_temp = 0;

	dev_dbg(battery->dev, "%s: Start\n", __func__);

	polling_time_temp = sec_bat_get_polling_time(battery);

	dev_dbg(battery->dev,
		"%s: Status:%s, Sleep:%s, Charging:%s, Short Poll:%s\n",
		__func__, sec_bat_status_str[battery->status],
		battery->polling_in_sleep ? "Yes" : "No",
		(battery->charging_mode ==
		SEC_BATTERY_CHARGING_NONE) ? "No" : "Yes",
		battery->polling_short ? "Yes" : "No");
	dev_info(battery->dev,
		"%s: Polling time %d/%d sec.\n", __func__,
		battery->polling_short ?
		(polling_time_temp * battery->polling_count) :
		polling_time_temp, battery->polling_time);

	/* To sync with log above,
	 * change polling count after log is displayed
	 * Do NOT update polling count in initial monitor
	 */
	if (!battery->pdata->monitor_initial_count)
		sec_bat_update_polling_count(battery);
	else
		dev_dbg(battery->dev,
			"%s: Initial monitor %d times left.\n", __func__,
			battery->pdata->monitor_initial_count);

	switch (battery->pdata->polling_type) {
	case SEC_BATTERY_MONITOR_WORKQUEUE:
		if (battery->pdata->monitor_initial_count) {
			battery->pdata->monitor_initial_count--;
			schedule_delayed_work(&battery->polling_work, HZ);
		} else
			schedule_delayed_work(&battery->polling_work,
				polling_time_temp * HZ);
		break;
	case SEC_BATTERY_MONITOR_ALARM:
		battery->last_poll_time = ktime_get_boottime();

		if (battery->pdata->monitor_initial_count) {
			battery->pdata->monitor_initial_count--;
			sec_bat_program_alarm(battery, 1);
		} else
			sec_bat_program_alarm(battery, polling_time_temp);
		break;
	case SEC_BATTERY_MONITOR_TIMER:
		break;
	default:
		break;
	}
	dev_dbg(battery->dev, "%s: End\n", __func__);
}

/* OTG during HV wireless charging or sleep mode have 4.5W normal wireless charging UI */
static bool sec_bat_hv_wc_normal_mode_check(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };

	psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, value);
	if (value.intval || sleep_mode) {
		pr_info("%s: otg(%d), sleep_mode(%d)\n", __func__, value.intval, sleep_mode);
		return true;
	}
	return false;
}

#if defined(CONFIG_BATTERY_SWELLING)
static void sec_bat_swelling_fullcharged_check(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };

	psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_PROP_STATUS, value);

	if (value.intval == POWER_SUPPLY_STATUS_FULL) {
		battery->swelling_full_check_cnt++;
		pr_info("%s: Swelling mode full-charged check (%d)\n",
			__func__, battery->swelling_full_check_cnt);
	} else
		battery->swelling_full_check_cnt = 0;

	if (battery->swelling_full_check_cnt >=
		battery->pdata->full_check_count) {
		battery->swelling_full_check_cnt = 0;
		battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
		battery->is_recharging = false;
		battery->swelling_mode = SWELLING_MODE_FULL;
		sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
		battery->expired_time = battery->pdata->expired_time;
		battery->prev_safety_time = 0;
#if defined(CONFIG_BATTERY_CISD)
		battery->cisd.data[CISD_DATA_SWELLING_FULL_CNT]++;
		battery->cisd.data[CISD_DATA_SWELLING_FULL_CNT_PER_DAY]++;
#endif
	}
}
#endif

static void sec_bat_handle_tx_misalign(struct sec_battery_info *battery, bool trigger_misalign)
{
	struct timespec ts = {0, };

	if (trigger_misalign) {
		if (battery->tx_misalign_start_time == 0) {
			ts = ktime_to_timespec(ktime_get_boottime());
			battery->tx_misalign_start_time = ts.tv_sec;
		}
		pr_info("@Tx_Mode %s: misalign is triggered!!(%d) \n", __func__, ++battery->tx_misalign_cnt);
		/* Attention!! in this case, 0x00(TX_OFF)  is sent first,
				and then 0x8000(RETRY) is sent */
		if (battery->tx_misalign_cnt < 3) {
			battery->tx_retry_case |= SEC_BAT_TX_RETRY_MISALIGN;
			sec_wireless_set_tx_enable(battery, false);
			/* clear tx all event */
			sec_bat_set_tx_event(battery, 0, BATT_TX_EVENT_WIRELESS_ALL_MASK);
			sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_RETRY, BATT_TX_EVENT_WIRELESS_TX_RETRY);
		} else {
			battery->tx_retry_case &= ~SEC_BAT_TX_RETRY_MISALIGN;
			battery->tx_misalign_start_time = 0;
			battery->tx_misalign_cnt = 0;
			pr_info("@Tx_Mode %s: Misalign over 3 times, TX OFF (cancel misalign)\n", __func__);
			sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_MISALIGN, BATT_TX_EVENT_WIRELESS_TX_MISALIGN);
			sec_wireless_set_tx_enable(battery, false);
		}
	} else if (battery->tx_retry_case & SEC_BAT_TX_RETRY_MISALIGN) {
		get_monotonic_boottime(&ts);
		if (ts.tv_sec >= battery->tx_misalign_start_time) {
			battery->tx_misalign_passed_time = ts.tv_sec - battery->tx_misalign_start_time;
		} else {
			battery->tx_misalign_passed_time = 0xFFFFFFFF - battery->tx_misalign_start_time
				+ ts.tv_sec;
		}
		pr_info("@Tx_Mode %s: already misaligned, passed time(%ld)\n", __func__, battery->tx_misalign_passed_time);

		if (battery->tx_misalign_passed_time >= 60) {
			pr_info("@Tx_Mode %s: after 1min\n", __func__);
			if (battery->wc_tx_enable) {
				if (battery->wc_rx_connected) {
					pr_info("@Tx_Mode %s: RX Dev, Keep TX ON status (cancel misalign)\n", __func__);
				} else {
					pr_info("@Tx_Mode %s: NO RX Dev, TX OFF (cancel misalign)\n", __func__);
					sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_MISALIGN, BATT_TX_EVENT_WIRELESS_TX_MISALIGN);
					sec_wireless_set_tx_enable(battery, false);
				}
			} else {
				pr_info("@Tx_Mode %s: Keep TX OFF status (cancel misalign)\n", __func__);
				//sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_ETC, BATT_TX_EVENT_WIRELESS_TX_ETC);
			}
			battery->tx_retry_case &= ~SEC_BAT_TX_RETRY_MISALIGN;
			battery->tx_misalign_start_time = 0;
			battery->tx_misalign_cnt = 0;
		}
	}
}

void sec_bat_handle_tx_ocp(struct sec_battery_info *battery, bool trigger_ocp)
{
	struct timespec ts = {0, };

	if (trigger_ocp) {
		if (battery->tx_ocp_start_time == 0) {
			ts = ktime_to_timespec(ktime_get_boottime());
			battery->tx_ocp_start_time = ts.tv_sec;
		}
		pr_info("@Tx_Mode %s: ocp is triggered!!(%d)\n", __func__, ++battery->tx_ocp_cnt);
		/* Attention!! in this case, 0x00(TX_OFF)  is sent first */
		/* and then 0x8000(RETRY) is sent */
		if (battery->tx_ocp_cnt < 3) {
			battery->tx_retry_case |= SEC_BAT_TX_RETRY_OCP;
			sec_wireless_set_tx_enable(battery, false);
			/* clear tx all event */
			sec_bat_set_tx_event(battery, 0, BATT_TX_EVENT_WIRELESS_ALL_MASK);
			sec_bat_set_tx_event(battery,
					BATT_TX_EVENT_WIRELESS_TX_RETRY, BATT_TX_EVENT_WIRELESS_TX_RETRY);
		} else {
			battery->tx_retry_case &= ~SEC_BAT_TX_RETRY_OCP;
			battery->tx_ocp_start_time = 0;
			battery->tx_ocp_cnt = 0;
			pr_info("@Tx_Mode %s: ocp over 3 times, TX OFF (cancel ocp)\n", __func__);
			sec_bat_set_tx_event(battery,
				BATT_TX_EVENT_WIRELESS_TX_OCP, BATT_TX_EVENT_WIRELESS_TX_OCP);
			sec_wireless_set_tx_enable(battery, false);
		}
	} else if (battery->tx_retry_case & SEC_BAT_TX_RETRY_OCP) {
		ts = ktime_to_timespec(ktime_get_boottime());
		if (ts.tv_sec >= battery->tx_ocp_start_time) {
			battery->tx_ocp_passed_time = ts.tv_sec - battery->tx_ocp_start_time;
		} else {
			battery->tx_ocp_passed_time = 0xFFFFFFFF - battery->tx_ocp_start_time
				+ ts.tv_sec;
		}
		pr_info("@Tx_Mode %s: already ocp, passed time(%ld)\n",
				__func__, battery->tx_ocp_passed_time);

		if (battery->tx_ocp_passed_time >= 60) {
			pr_info("@Tx_Mode %s: after 1min\n", __func__);
			if (battery->wc_tx_enable) {
				if (battery->wc_rx_connected) {
					pr_info("@Tx_Mode %s: RX Dev, Keep TX ON status (cancel ocp)\n", __func__);
				} else {
					pr_info("@Tx_Mode %s: NO RX Dev, TX OFF (cancel ocp)\n", __func__);
					sec_bat_set_tx_event(battery,
							BATT_TX_EVENT_WIRELESS_TX_OCP, BATT_TX_EVENT_WIRELESS_TX_OCP);
					sec_wireless_set_tx_enable(battery, false);
				}
			} else {
				pr_info("@Tx_Mode %s: Keep TX OFF status (cancel ocp)\n", __func__);
			}
			battery->tx_retry_case &= ~SEC_BAT_TX_RETRY_OCP;
			battery->tx_ocp_start_time = 0;
			battery->tx_ocp_cnt = 0;
		}
	}
}

static void sec_bat_wireless_minduty_cntl(struct sec_battery_info *battery, unsigned int duty_val)
{
	union power_supply_propval value = {0, };

	if (duty_val != battery->tx_minduty) {
		value.intval = duty_val;
		psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_MIN_DUTY, value);

		pr_info("@Tx_Mode %s : Min duty chagned (%d -> %d)\n", __func__, battery->tx_minduty, duty_val);
		battery->tx_minduty = duty_val;
	}
}

static void sec_bat_wireless_uno_cntl(struct sec_battery_info *battery, bool en)
{
	union power_supply_propval value = {0, };

	battery->uno_en = value.intval = en;
	pr_info("@Tx_Mode %s : Uno control %d\n", __func__, battery->uno_en);

	if (value.intval) {
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ENABLE, value);
	} else {
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONNECTED, value);
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL, value);
	}
}

static void sec_bat_wireless_iout_cntl(struct sec_battery_info *battery, int uno_iout, int mfc_iout) {
	union power_supply_propval value = {0, };

	if (battery->tx_uno_iout != uno_iout) {
		pr_info("@Tx_Mode %s : set uno iout(%d) -> (%d)\n", __func__, battery->tx_uno_iout, uno_iout);
		value.intval = battery->tx_uno_iout = uno_iout;
		psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_IOUT, value);
	} else {
		pr_info("@Tx_Mode %s : Already set Uno Iout(%d == %d)\n", __func__, battery->tx_uno_iout, uno_iout);
	}

#if !defined(CONFIG_SEC_FACTORY)
	if (battery->lcd_status && (mfc_iout == battery->pdata->tx_mfc_iout_phone)) {
		pr_info("@Tx_Mode %s Reduce Tx MFC Iout. LCD ON\n", __func__);
		mfc_iout = battery->pdata->tx_mfc_iout_lcd_on;
	}
#endif

	if (battery->tx_mfc_iout != mfc_iout) {
		pr_info("@Tx_Mode %s : set mfc iout(%d) -> (%d)\n", __func__, battery->tx_mfc_iout, mfc_iout);
		value.intval = battery->tx_mfc_iout = mfc_iout;
		psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_IOUT, value);
	} else {
		pr_info("@Tx_Mode %s : Already set MFC Iout(%d == %d)\n", __func__, battery->tx_mfc_iout, mfc_iout);
	}
}

static void sec_bat_wireless_vout_cntl(struct sec_battery_info *battery, int vout_now)
{
	union power_supply_propval value = {0, };
	int vout_mv, vout_now_mv;

	vout_mv = battery->wc_tx_vout == 0 ? 5000 : (5000 + (battery->wc_tx_vout * 500));
	vout_now_mv = vout_now == 0 ? 5000 : (5000 + (vout_now * 500));

	pr_info("@Tx_Mode %s : set uno & mfc vout (%dmV -> %dmV)\n", __func__, vout_mv, vout_now_mv);

	if (battery->wc_tx_vout >= vout_now) {
		battery->wc_tx_vout = value.intval = vout_now;
		psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT, value);
		psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT, value);
	} else if (vout_now > battery->wc_tx_vout) {
		battery->wc_tx_vout = value.intval = vout_now;
		psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT, value);
		psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT, value);
	}

}

#if defined(CONFIG_WIRELESS_TX_MODE)
#if !defined(CONFIG_SEC_FACTORY)
static void sec_bat_check_tx_battery_drain(struct sec_battery_info *battery)
{
	if(battery->capacity <= battery->pdata->tx_stop_capacity &&
		is_nocharge_type(battery->cable_type)) {
		pr_info("%s: @Tx_Mode battery level is drained, TX mode should turn off \n", __func__);
		/* set tx event */
		sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_SOC_DRAIN, BATT_TX_EVENT_WIRELESS_TX_SOC_DRAIN);
		sec_wireless_set_tx_enable(battery, false);
	}
}

static void sec_bat_check_tx_current(struct sec_battery_info *battery)
{
	if (battery->lcd_status && (battery->tx_mfc_iout > battery->pdata->tx_mfc_iout_lcd_on)) {
		sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_lcd_on);
		pr_info("@Tx_Mode %s Reduce Tx MFC Iout. LCD ON\n", __func__);
	} else if (!battery->lcd_status && (battery->tx_mfc_iout == battery->pdata->tx_mfc_iout_lcd_on)) {
		union power_supply_propval value = {0, };
		sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_phone);
		pr_info("@Tx_Mode %s  Recovery Tx MFC Iout. LCD OFF\n", __func__);

		value.intval = true;
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_SEND_FSK, value);
	}
}

static void sec_bat_check_tx_temperature(struct sec_battery_info *battery)
{
	if (battery->wc_tx_enable) {
		if(battery->temperature >= battery->pdata->tx_high_threshold) {
			pr_info("@Tx_Mode : %s: Battery temperature is too high. Tx mode should turn off  \n", __func__);
			/* set tx event */
			sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_HIGH_TEMP, BATT_TX_EVENT_WIRELESS_TX_HIGH_TEMP);
			battery->tx_retry_case |= SEC_BAT_TX_RETRY_HIGH_TEMP;
			sec_wireless_set_tx_enable(battery, false);	
		} else if (battery->temperature <= battery->pdata->tx_low_threshold) {
			pr_info("@Tx_Mode : %s: Battery temperature is too low. Tx mode should turn off  \n", __func__);
			/* set tx event */
			sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_LOW_TEMP, BATT_TX_EVENT_WIRELESS_TX_LOW_TEMP);
			battery->tx_retry_case |= SEC_BAT_TX_RETRY_LOW_TEMP;
			sec_wireless_set_tx_enable(battery, false);
		}
	} else if (battery->tx_retry_case & SEC_BAT_TX_RETRY_HIGH_TEMP) {
		if (battery->temperature <= battery->pdata->tx_high_recovery) {
			pr_info("@Tx_Mode : %s: Battery temperature goes to normal(High). Retry TX mode\n", __func__);
			battery->tx_retry_case &= ~SEC_BAT_TX_RETRY_HIGH_TEMP;
			if (!battery->tx_retry_case)
				sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_RETRY, BATT_TX_EVENT_WIRELESS_TX_RETRY);
		}
	} else if (battery->tx_retry_case & SEC_BAT_TX_RETRY_LOW_TEMP) {
		if (battery->temperature >= battery->pdata->tx_low_recovery) {
			pr_info("@Tx_Mode : %s: Battery temperature goes to normal(Low). Retry TX mode\n", __func__);
			battery->tx_retry_case &= ~SEC_BAT_TX_RETRY_LOW_TEMP;
			if (!battery->tx_retry_case)
				sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_RETRY, BATT_TX_EVENT_WIRELESS_TX_RETRY);
		}
	}
}
#endif

static void sec_bat_check_tx_switch_mode(struct sec_battery_info *battery) {
	union power_supply_propval value = {0, };

	if (battery->current_event & SEC_BAT_CURRENT_EVENT_AFC)	{
		pr_info("@Tx_mode %s Do not switch switch mode! AFC Event set\n", __func__); 
		return;
	}

	value.intval = SEC_FUELGAUGE_CAPACITY_TYPE_CAPACITY_POINT;
	psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CAPACITY, value);

	if ((battery->tx_switch_mode == TX_SWITCH_UNO_ONLY) && (!battery->buck_cntl_by_tx)) {
		battery->buck_cntl_by_tx = true;
		sec_bat_set_charge(battery, battery->charger_mode);

		sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_phone);
		sec_bat_wireless_vout_cntl(battery, WC_TX_VOUT_7_5V);
		sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_default);
	} else if ((battery->tx_switch_mode == TX_SWITCH_CHG_ONLY) && (battery->buck_cntl_by_tx)) {
		sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_phone_5v);
		sec_bat_wireless_vout_cntl(battery, WC_TX_VOUT_5_0V);
		sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_5V);

		battery->buck_cntl_by_tx = false;
		sec_bat_set_charge(battery, battery->charger_mode);
	}

	if (battery->status == POWER_SUPPLY_STATUS_FULL) {
		if (battery->charging_mode == SEC_BATTERY_CHARGING_NONE) {
			if (battery->tx_switch_mode == TX_SWITCH_CHG_ONLY)
				battery->tx_switch_mode_change = true;
		} else {
			if (battery->tx_switch_mode == TX_SWITCH_UNO_ONLY) {
				if (battery->tx_switch_start_soc >= 100) {
					if ((battery->capacity < 99) ||	((battery->capacity == 99) && (value.intval <= 1)))
						battery->tx_switch_mode_change = true;
				} else {
					if (((battery->capacity == battery->tx_switch_start_soc) && (value.intval <= 1)) ||
					(battery->capacity < battery->tx_switch_start_soc))
						battery->tx_switch_mode_change = true;
				}
			} else if (battery->tx_switch_mode == TX_SWITCH_CHG_ONLY) {
				if (battery->capacity >= 100)
					battery->tx_switch_mode_change = true;
			}
		}
	} else {
		if (battery->tx_switch_mode == TX_SWITCH_UNO_ONLY) {
			if (((battery->capacity == battery->tx_switch_start_soc) && (value.intval <= 1)) ||
				(battery->capacity < battery->tx_switch_start_soc))
				battery->tx_switch_mode_change = true;

		} else if (battery->tx_switch_mode == TX_SWITCH_CHG_ONLY) {
			if (((battery->capacity == (battery->tx_switch_start_soc + 1)) && (value.intval >= 8)) ||
				(battery->capacity > (battery->tx_switch_start_soc + 1)))
				battery->tx_switch_mode_change = true;
		}
	}
	pr_info("@Tx_mode Tx mode(%d) tx_switch_mode_chage(%d) start soc(%d) now soc(%d.%d)\n",
		battery->tx_switch_mode, battery->tx_switch_mode_change,
		battery->tx_switch_start_soc, battery->capacity, value.intval);
}
#endif

#if defined(CONFIG_CALC_TIME_TO_FULL)
static void sec_bat_calc_time_to_full(struct sec_battery_info * battery)
{
	if (delayed_work_pending(&battery->timetofull_work)) {
		pr_info("%s: keep time_to_full(%5d sec)\n", __func__, battery->timetofull);
	} else if ((battery->status == POWER_SUPPLY_STATUS_CHARGING ||
		(battery->status == POWER_SUPPLY_STATUS_FULL && battery->capacity != 100)) && !battery->wc_tx_enable) {
		union power_supply_propval value = {0, };
		int charge = 0;

		if (is_hv_wire_12v_type(battery->cable_type)) {
			charge = battery->pdata->ttf_hv_12v_charge_current;
		} else if (is_hv_wireless_type(battery->cable_type) ||
			battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV ||
			battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_20) {
			if (sec_bat_hv_wc_normal_mode_check(battery))
				charge = battery->pdata->ttf_wireless_charge_current;
			else if((battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_20 && !lpcharge) ||
				battery->cable_type == SEC_BATTERY_CABLE_HV_WIRELESS_20)
				charge = battery->ttf_predict_wc20_charge_current;
			else
				charge = battery->pdata->ttf_hv_wireless_charge_current;
		} else if (is_hv_wire_type(battery->cable_type)) {
			charge = battery->pdata->ttf_hv_charge_current;
		} else if (is_nv_wireless_type(battery->cable_type)) {
			charge = battery->pdata->ttf_wireless_charge_current;
		} else if (is_pd_apdo_wire_type(battery->cable_type) ||
			(is_pd_fpdo_wire_type(battery->cable_type) && battery->hv_pdo)) {
			if (battery->pd_max_charge_power > HV_CHARGER_STATUS_STANDARD4) {
				charge = battery->pdata->ttf_dc45_charge_current;
			} else if (battery->pd_max_charge_power > HV_CHARGER_STATUS_STANDARD3) {
				charge = battery->pdata->ttf_dc25_charge_current;
			} else if (battery->pd_max_charge_power <= battery->pdata->pd_charging_charge_power &&
				battery->pdata->charging_current[battery->cable_type].fast_charging_current >= \
				battery->pdata->max_charging_current) { /* same PD power with AFC */
				charge = battery->pdata->ttf_hv_charge_current;
			} else { /* other PD charging */
				charge = (battery->pd_max_charge_power / 5) > battery->pdata->charging_current[battery->cable_type].fast_charging_current ?
					battery->pdata->charging_current[battery->cable_type].fast_charging_current : (battery->pd_max_charge_power / 5);
			}
		} else {
			charge = (battery->max_charge_power / 5) > battery->pdata->charging_current[battery->cable_type].fast_charging_current ?
					battery->pdata->charging_current[battery->cable_type].fast_charging_current : (battery->max_charge_power / 5);
		}
		value.intval = charge;
		psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_TIME_TO_FULL_NOW, value);
		dev_info(battery->dev, "%s: T: %5d sec, passed time: %5ld, current: %d\n",
				__func__, value.intval, battery->charging_passed_time, charge);
		battery->timetofull = value.intval;

		if (battery->batt_full_capacity < 100 && battery->batt_full_capacity > 0) {
			value.intval = charge;
			psy_do_property(battery->pdata->fuelgauge_name, get,
					POWER_SUPPLY_EXT_PROP_TTF_FULL_CAPACITY, value);
			battery->timetofull = battery->timetofull - value.intval;
			dev_info(battery->dev, "%s: time to 85 - T: %5d sec, passed time: %5ld, current: %d\n",
					__func__, battery->timetofull, battery->charging_passed_time, charge);
		}
	} else {
		battery->timetofull = -1;
	}
}

static void sec_bat_time_to_full_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, timetofull_work.work);
	union power_supply_propval value = {0, };

	psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_PROP_CURRENT_MAX, value);
	battery->current_max = value.intval;

	value.intval = SEC_BATTERY_CURRENT_MA;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_CURRENT_NOW, value);
	battery->current_now = value.intval;

	value.intval = SEC_BATTERY_CURRENT_MA;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_CURRENT_AVG, value);
	battery->current_avg = value.intval;

	sec_bat_calc_time_to_full(battery);
	dev_info(battery->dev, "%s: \n",__func__);
	if (battery->voltage_now > 0)
		battery->voltage_now--;

	power_supply_changed(battery->psy_bat);
}
#endif

#if defined(CONFIG_WIRELESS_TX_MODE)
static void sec_bat_txpower_calc(struct sec_battery_info * battery)
{
	if (delayed_work_pending(&battery->wpc_txpower_calc_work)) {
		pr_info("%s: keep average tx power(%5d mA)\n", __func__, battery->tx_avg_curr);
	} else if (battery->wc_tx_enable) {
		int tx_vout=0, tx_iout=0, vbatt=0;
		union power_supply_propval value = {0, };

		if(battery->tx_clear) {
			battery->tx_time_cnt = 0;
			battery->tx_avg_curr = 0;
			battery->tx_total_power = 0;
			battery->tx_clear = false;
		}

		if(battery->tx_clear_cisd) {
			battery->tx_total_power_cisd = 0;
			battery->tx_clear_cisd = false;
		}
		
		psy_do_property(battery->pdata->wireless_charger_name, get,
		POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_VIN, value);
		tx_vout = value.intval;

		psy_do_property(battery->pdata->wireless_charger_name, get,
		POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_IIN, value);
		tx_iout = value.intval;

		psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
		vbatt = value.intval;

		battery->tx_time_cnt++;

		/* AVG curr will be calculated only when the battery is discharged */ 
		if (battery->current_avg <= 0 && vbatt > 0)
			tx_iout = (tx_vout / vbatt) * tx_iout;
		else
			tx_iout = 0;

		/* monitor work will be scheduled every 10s when wc_tx_enable is true */
		battery->tx_avg_curr = ((battery->tx_avg_curr * battery->tx_time_cnt) + tx_iout) / (battery->tx_time_cnt + 1);
		battery->tx_total_power = (battery->tx_avg_curr * battery->tx_time_cnt) / (60*60/10);

		/* tx_total_power_cisd : daily accumulated power consumption by Tx, will be cleared when cisd data is sent */
		battery->tx_total_power_cisd = battery->tx_total_power_cisd + battery->tx_total_power;

		dev_info(battery->dev,
		"%s:tx_time_cnt(%ds), UNO_Vin(%dV), UNU_Iin(%dmA), tx_avg_curr(%dmA), tx_total_power(%dmAh), tx_total_power_cisd(%dmAh))\n", __func__,
		battery->tx_time_cnt*10, tx_vout, tx_iout, battery->tx_avg_curr, battery->tx_total_power, battery->tx_total_power_cisd);
	}
}

static void sec_bat_txpower_calc_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, wpc_txpower_calc_work.work);

	sec_bat_txpower_calc(battery);
}
#endif

#if defined(CONFIG_ENABLE_100MA_CHARGING_BEFORE_USB_CONFIGURED)
extern bool get_usb_enumeration_state(void);
/* To disaply slow charging when usb charging 100MA*/
static void sec_bat_check_slowcharging_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, slowcharging_work.work);

	if (battery->pdic_info.sink_status.rp_currentlvl == RP_CURRENT_LEVEL_DEFAULT &&
		battery->cable_type == SEC_BATTERY_CABLE_USB) {
		if (!get_usb_enumeration_state() &&
			(battery->current_event & SEC_BAT_CURRENT_EVENT_USB_100MA)) {
			battery->usb_slow_chg = true;
			battery->max_charge_power = (battery->input_voltage * battery->current_max) / 10;
			wake_lock(&battery->monitor_wake_lock);
			queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		}
	}
	dev_info(battery->dev, "%s: \n",__func__);
}
#endif

static void sec_bat_wc_cv_mode_check(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };
	int is_otg_on = 0;

	psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, value);
	is_otg_on = value.intval;

	pr_info("%s: battery->wc_cv_mode = %d, otg(%d) \n", __func__, battery->wc_cv_mode, is_otg_on);

	if (battery->capacity >= battery->pdata->wireless_cc_cv && !is_otg_on) {
		pr_info("%s: 4.5W WC Changed Vout input current limit\n", __func__);
		battery->wc_cv_mode = true;
		sec_bat_set_charging_current(battery);
		value.intval = WIRELESS_VOUT_CC_CV_VOUT; // 5.5V
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
		value.intval = WIRELESS_VRECT_ADJ_ROOM_5; // 80mv
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
		if ((battery->cable_type == SEC_BATTERY_CABLE_WIRELESS ||
			battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_STAND ||
			battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_TX)) {
			value.intval = WIRELESS_CLAMP_ENABLE;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
		}
		/* Change FOD values for CV mode */
		value.intval = POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE;
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_PROP_STATUS, value);
	}
}

static void sec_bat_siop_level_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
			struct sec_battery_info, siop_level_work.work);

	pr_info("%s : set current by siop level(%d)\n",__func__, battery->siop_level);

	sec_bat_set_charging_current(battery);

	wake_unlock(&battery->siop_level_wake_lock);
}

static void sec_bat_wc_headroom_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
			struct sec_battery_info, wc_headroom_work.work);
	union power_supply_propval value = {0, };

	/* The default headroom is high, because initial wireless charging state is unstable.
		After 10sec wireless charging, however, recover headroom level to avoid chipset damage */
	if (battery->wc_status != SEC_WIRELESS_PAD_NONE) {
		/* When the capacity is higher than 99, and the device is in 5V wireless charging state,
			then Vrect headroom has to be headroom_2.
			Refer to the sec_bat_siop_work function. */
		if (battery->capacity < 99 && battery->status != POWER_SUPPLY_STATUS_FULL) {
			if (is_nv_wireless_type(battery->cable_type)) {
				if (battery->capacity < battery->pdata->wireless_cc_cv)
					value.intval = WIRELESS_VRECT_ADJ_ROOM_4; /* WPC 4.5W, Vrect Room 30mV */
				else
					value.intval = WIRELESS_VRECT_ADJ_ROOM_5; /* WPC 4.5W, Vrect Room 80mV */
			} else if (is_hv_wireless_type(battery->cable_type)) {
				value.intval = WIRELESS_VRECT_ADJ_ROOM_5;
			} else {
				value.intval = WIRELESS_VRECT_ADJ_OFF;
			}
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
			pr_info("%s: Changed Vrect adjustment from Rx activation(10seconds)", __func__);
		}
		if (is_nv_wireless_type(battery->cable_type))
			sec_bat_wc_cv_mode_check(battery);
	}
	wake_unlock(&battery->wc_headroom_wake_lock);
}

static void sec_bat_ext_event_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
			struct sec_battery_info, ext_event_work.work);

	union power_supply_propval value = {0, };

	if (battery->wc_tx_enable) { /* TX ON state */
		if (battery->ext_event & BATT_EXT_EVENT_CAMERA) {
			pr_info("@Tx_Mode %s: Camera ON, TX OFF\n", __func__);
			sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_CAMERA_ON, BATT_TX_EVENT_WIRELESS_TX_CAMERA_ON);
			sec_wireless_set_tx_enable(battery, false);
		} else if (battery->ext_event & BATT_EXT_EVENT_DEX) {
			pr_info("@Tx_Mode %s: Dex ON, TX OFF\n", __func__);
			sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_OTG_ON, BATT_TX_EVENT_WIRELESS_TX_OTG_ON);
			sec_wireless_set_tx_enable(battery, false);
		} else if (battery->ext_event & BATT_EXT_EVENT_CALL) {
			pr_info("@Tx_Mode %s: Call ON, TX OFF\n", __func__);
			battery->tx_retry_case |= SEC_BAT_TX_RETRY_CALL;
			sec_wireless_set_tx_enable(battery, false);
			/* clear tx all event */
			sec_bat_set_tx_event(battery, 0, BATT_TX_EVENT_WIRELESS_ALL_MASK);
		}
	} else { /* TX OFF state, it has only call scenario */
		if (battery->ext_event & BATT_EXT_EVENT_CALL) {
			pr_info("@Tx_Mode %s: Call ON\n", __func__);

			value.intval = BATT_EXT_EVENT_CALL;
			psy_do_property(battery->pdata->wireless_charger_name, set,
							POWER_SUPPLY_EXT_PROP_CALL_EVENT, value);

			if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_PACK ||
				battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_HV_PACK ||
				battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_TX) {
				pr_info("%s : Call is on during Wireless Pack or TX\n",__func__);
				battery->wc_rx_phm_mode = true;
			}
			if (battery->tx_retry_case != SEC_BAT_TX_RETRY_NONE) {
				pr_info("@Tx_Mode %s: TX OFF because of other reason(retry:0x%x), save call retry case\n",
					__func__, battery->tx_retry_case);
				battery->tx_retry_case |= SEC_BAT_TX_RETRY_CALL;
			}
		} else if (!(battery->ext_event & BATT_EXT_EVENT_CALL)) {
			pr_info("@Tx_Mode %s: Call OFF\n", __func__);

			value.intval = BATT_EXT_EVENT_NONE;
			psy_do_property(battery->pdata->wireless_charger_name, set,
							POWER_SUPPLY_EXT_PROP_CALL_EVENT, value);

			/* check the diff between current and previous ext_event state */
			if (battery->tx_retry_case & SEC_BAT_TX_RETRY_CALL) {
				battery->tx_retry_case &= ~SEC_BAT_TX_RETRY_CALL;
				if (!battery->tx_retry_case) {
					pr_info("@Tx_Mode %s: Call OFF, TX Retry\n", __func__);
					sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_RETRY, BATT_TX_EVENT_WIRELESS_TX_RETRY);
				}
			} else if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_PACK ||
				battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_HV_PACK ||
				battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_TX) {
				pr_info("%s : Call is off during Wireless Pack or TX\n",__func__);
			}

			/* process escape phm */
			if(battery->wc_rx_phm_mode) {
				pr_info("%s: ESCAPE PHM STEP 1\n", __func__);
				sec_bat_set_mfc_on(battery);
				msleep(100);

				pr_info("%s: ESCAPE PHM STEP 2\n", __func__);
				sec_bat_set_mfc_off(battery, false);
				msleep(510);

				pr_info("%s: ESCAPE PHM STEP 3\n", __func__);
				sec_bat_set_mfc_on(battery);
			}
			battery->wc_rx_phm_mode = false;
		}
	}

	wake_unlock(&battery->ext_event_wake_lock);
}

#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
bool sec_bat_check_boost_mfc_condition(struct sec_battery_info *battery, int mode)
{
	union power_supply_propval value = {0, };
	int boost_status = 0, wpc_det = 0, mst_pwr_en = 0;

	dev_info(battery->dev, "%s \n", __func__);

	if (mode == SEC_WIRELESS_RX_INIT) {
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_INITIAL_WC_CHECK, value);
		wpc_det = value.intval;
	}

	if (gpio_is_valid(battery->pdata->mst_pwr_en))
		mst_pwr_en = gpio_get_value(battery->pdata->mst_pwr_en);
	else
		pr_info("%s: invalid gpio(mst_pwr_en)\n", __func__);

	psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_EXT_PROP_CHARGE_BOOST, value);
	boost_status = value.intval;

	pr_info("%s wpc_det(%d), mst_pwr_en(%d), boost_status(%d) bootmode(%d)\n",
		__func__, wpc_det, mst_pwr_en, boost_status, bootmode);

	if (!boost_status && !wpc_det && !mst_pwr_en && (bootmode != 2))
		return true;
	return false;
}

void sec_bat_fw_update_work(struct sec_battery_info *battery, int mode)
{
	union power_supply_propval value = {0, };
	int ret = 0;	

	dev_info(battery->dev, "%s \n", __func__);

	wake_lock_timeout(&battery->vbus_wake_lock, HZ * 10);

	switch (mode) {
		case SEC_WIRELESS_RX_SDCARD_MODE:
		case SEC_WIRELESS_RX_BUILT_IN_MODE:
		case SEC_WIRELESS_RX_SPU_MODE:
			mfc_fw_update = true;
			value.intval = mode;
			ret = psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_PROP_CHARGE_POWERED_OTG_CONTROL, value);
			if (ret < 0)
				mfc_fw_update = false;
			break;
		case SEC_WIRELESS_TX_ON_MODE:
			value.intval = true;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL, value);

			value.intval = mode;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_PROP_CHARGE_POWERED_OTG_CONTROL, value);

			break;
		case SEC_WIRELESS_TX_OFF_MODE:
			value.intval = false;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL, value);
			break;
		default:
			break;
	}
}

static void sec_bat_fw_init_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, fw_init_work.work);

	union power_supply_propval value = {0, };
	int ret = 0;

#if defined(CONFIG_WIRELESS_IC_PARAM)
	psy_do_property(battery->pdata->wireless_charger_name, get,
		POWER_SUPPLY_EXT_PROP_WIRELESS_CHECK_FW_VER, value);
	if (value.intval) {		
		pr_info("%s: wireless firmware is already updated.\n", __func__);
		return;
	}
#endif
	if (sec_bat_check_boost_mfc_condition(battery, SEC_WIRELESS_RX_INIT) &&
		battery->capacity > 30 && !lpcharge) {
		mfc_fw_update = true;
		value.intval = SEC_WIRELESS_RX_INIT;
		ret = psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_PROP_CHARGE_POWERED_OTG_CONTROL, value);
		if (ret < 0)
			mfc_fw_update = false;
	}
}
#endif
#if defined(CONFIG_UPDATE_BATTERY_DATA)
static void sec_bat_update_data_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, batt_data_work.work);

	sec_battery_update_data(battery->data_path);
	wake_unlock(&battery->batt_data_wake_lock);
}
#endif

static void sec_bat_misc_event_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, misc_event_work.work);
	int xor_misc_event = battery->prev_misc_event ^ battery->misc_event;

	if ((xor_misc_event & (BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE |
		BATT_MISC_EVENT_HICCUP_TYPE | BATT_MISC_EVENT_TEMP_HICCUP_TYPE)) &&
		is_nocharge_type(battery->cable_type)) {
		if (battery->misc_event & (BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE |
			BATT_MISC_EVENT_HICCUP_TYPE | BATT_MISC_EVENT_TEMP_HICCUP_TYPE)) {
			sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_BUCK_OFF);
		} else if (battery->prev_misc_event & (BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE |
			BATT_MISC_EVENT_HICCUP_TYPE | BATT_MISC_EVENT_TEMP_HICCUP_TYPE)) {
			sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
		}
	}

	pr_info("%s: change misc event(0x%x --> 0x%x)\n",
		__func__, battery->prev_misc_event, battery->misc_event);
	battery->prev_misc_event = battery->misc_event;
	wake_unlock(&battery->misc_event_wake_lock);

	wake_lock(&battery->monitor_wake_lock);
	queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
}

static void sec_bat_calculate_safety_time(struct sec_battery_info *battery)
{
	unsigned long long expired_time = battery->expired_time;
	struct timespec ts = {0, };
	int curr = 0;
	int input_power = battery->current_max * battery->input_voltage * 100;
	int charging_power = battery->charging_current * (battery->pdata->chg_float_voltage / battery->pdata->chg_float_voltage_conv);
	static int discharging_cnt = 0;

	if (battery->current_avg < 0) {
		discharging_cnt++;
	} else {
		discharging_cnt = 0;
	}

	if (discharging_cnt >= 5) {
		battery->expired_time = battery->pdata->expired_time;
		battery->prev_safety_time = 0;
		pr_info("%s : SAFETY TIME RESET! DISCHARGING CNT(%d)\n",
			__func__, discharging_cnt);
		discharging_cnt = 0;
		return;
	} else if ((battery->lcd_status || battery->wc_tx_enable) && battery->stop_timer) {
		battery->prev_safety_time = 0;
		return;
	}

	get_monotonic_boottime(&ts);

	if (battery->prev_safety_time == 0) {
		battery->prev_safety_time = ts.tv_sec;
	}

	if (input_power > charging_power) {
		curr = battery->charging_current;
	} else {
		curr = input_power / (battery->pdata->chg_float_voltage / battery->pdata->chg_float_voltage_conv);
		curr = (curr * 9) / 10;
	}

	if ((battery->lcd_status || battery->wc_tx_enable) && !battery->stop_timer) {
		battery->stop_timer = true;
	} else if (!(battery->lcd_status || battery->wc_tx_enable) && battery->stop_timer) {
		battery->stop_timer = false;
	}

	pr_info("%s : EXPIRED_TIME(%llu), IP(%d), CP(%d), CURR(%d), STANDARD(%d)\n",
		__func__, expired_time, input_power, charging_power, curr, battery->pdata->standard_curr);

	if (curr == 0)
		return;
	else if (curr > battery->pdata->standard_curr)
		curr = battery->pdata->standard_curr;

	expired_time = (expired_time * battery->pdata->standard_curr) / curr;

	pr_info("%s : CAL_EXPIRED_TIME(%llu) TIME NOW(%ld) TIME PREV(%ld)\n", __func__, expired_time, ts.tv_sec, battery->prev_safety_time);

	if (expired_time <= ((ts.tv_sec - battery->prev_safety_time) * 1000))
		expired_time = 0;
	else
		expired_time -= ((ts.tv_sec - battery->prev_safety_time) * 1000);

	battery->cal_safety_time = expired_time;
	expired_time = (expired_time * curr) / battery->pdata->standard_curr;

	battery->expired_time = expired_time;
	battery->prev_safety_time = ts.tv_sec;
	pr_info("%s : REMAIN_TIME(%ld) CAL_REMAIN_TIME(%ld)\n", __func__, battery->expired_time, battery->cal_safety_time);
}

static void sec_bat_recov_full_capacity(struct sec_battery_info *battery)
{
	sec_bat_set_misc_event(battery, 0, BATT_MISC_EVENT_FULL_CAPACITY);

	if (battery->status == POWER_SUPPLY_STATUS_NOT_CHARGING
		&& battery->health == POWER_SUPPLY_HEALTH_GOOD) {
		sec_bat_set_charging_status(battery,
			POWER_SUPPLY_STATUS_CHARGING);
		sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING);
	}
}

static void sec_bat_check_full_capacity(struct sec_battery_info *battery)
{
	int rechg_capacity = battery->batt_full_capacity - 2;

	if (battery->batt_full_capacity >= 100 || battery->batt_full_capacity <= 0 ||
		battery->status == POWER_SUPPLY_STATUS_DISCHARGING) {
		if (battery->misc_event & BATT_MISC_EVENT_FULL_CAPACITY) {
			pr_info("%s: full_capacity(%d) status(%d)\n",
				__func__, battery->batt_full_capacity, battery->status);
			sec_bat_recov_full_capacity(battery);
		}
		return;
	}

	if (battery->misc_event & BATT_MISC_EVENT_FULL_CAPACITY) {
		if (battery->capacity <= rechg_capacity) {
			pr_info("%s : start re-charging(%d, %d)\n", __func__, battery->capacity, rechg_capacity);
			sec_bat_recov_full_capacity(battery);
		}
	} else if (battery->capacity >= battery->batt_full_capacity) {
		pr_info("%s : stop charging(%d, %d)\n", __func__, battery->capacity, battery->batt_full_capacity);

		sec_bat_set_misc_event(battery, BATT_MISC_EVENT_FULL_CAPACITY,
			BATT_MISC_EVENT_FULL_CAPACITY);
		sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);
		sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
		sec_bat_send_cs100(battery);
	}
}

static void sec_bat_monitor_work(
				struct work_struct *work)
{
	struct sec_battery_info *battery =
		container_of(work, struct sec_battery_info,
		monitor_work.work);
	static struct timespec old_ts = {0, };
	struct timespec c_ts = {0, };
	union power_supply_propval val = {0, };
	union power_supply_propval value = {0, };

	dev_dbg(battery->dev, "%s: Start\n", __func__);
	c_ts = ktime_to_timespec(ktime_get_boottime());

	mutex_lock(&battery->wclock);
	if (!battery->wc_enable) {
		pr_info("%s: wc_enable(%d), cnt(%d)\n",
			__func__, battery->wc_enable, battery->wc_enable_cnt);
		if (battery->wc_enable_cnt > battery->wc_enable_cnt_value) {
			char wpc_en_status[2];

			battery->wc_enable = true;
			battery->wc_enable_cnt = 0;
			wpc_en_status[0] = WPC_EN_SYSFS;
			wpc_en_status[1] = true;
			value.strval= wpc_en_status;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_WPC_EN, value);

			pr_info("%s: WC CONTROL: Enable\n", __func__);
			pr_info("%s: wpc_en(%d)\n",
				__func__, gpio_get_value(battery->pdata->wpc_en));
		}
		battery->wc_enable_cnt++;
	}
	mutex_unlock(&battery->wclock);

	/* monitor once after wakeup */
	if (battery->polling_in_sleep) {
		battery->polling_in_sleep = false;
		if ((battery->status == POWER_SUPPLY_STATUS_DISCHARGING) &&
			((battery->ps_enable != true) && !battery->wc_tx_enable)) {
			if ((unsigned long)(c_ts.tv_sec - old_ts.tv_sec) < 10 * 60) {
					psy_do_property(battery->pdata->fuelgauge_name, get,
						POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
					battery->voltage_now = value.intval;

					value.intval = 0;
					psy_do_property(battery->pdata->fuelgauge_name, get,
							POWER_SUPPLY_PROP_CAPACITY, value);
					battery->capacity = value.intval;

					sec_bat_get_temperature_info(battery);
#if defined(CONFIG_BATTERY_CISD)
					sec_bat_cisd_check(battery);
#endif
					power_supply_changed(battery->psy_bat);
					pr_info("Skip monitor work(%ld, Vnow:%d(mV), SoC:%d(%%), Tbat:%d(0.1'C))\n",
						c_ts.tv_sec - old_ts.tv_sec, battery->voltage_now, battery->capacity, battery->temperature);

				goto skip_monitor;
			}
		}
	}
	/* update last monitor time */
	old_ts = c_ts;

	sec_bat_get_battery_info(battery);

#if defined(CONFIG_BATTERY_CISD)
	sec_bat_cisd_check(battery);
#endif

#if defined(CONFIG_STEP_CHARGING)
	sec_bat_check_step_charging(battery);
#endif
#if defined(CONFIG_CALC_TIME_TO_FULL)
	/* time to full check */
	sec_bat_calc_time_to_full(battery);
#endif
	sec_bat_check_full_capacity(battery);
#if defined(CONFIG_WIRELESS_TX_MODE)
	/* tx mode check */
	if (battery->wc_tx_enable) {
		pr_info("@Tx_Mode %s: tx_retry(0x%x), tx_switch(0x%x)",
			__func__, battery->tx_retry_case, battery->tx_switch_mode);
#if !defined(CONFIG_SEC_FACTORY)
		sec_bat_check_tx_battery_drain(battery);
		sec_bat_check_tx_temperature(battery);

		if ((battery->wc_rx_type == SS_PHONE) || (battery->wc_rx_type == OTHER_DEV) || (battery->wc_rx_type == SS_BUDS))
			sec_bat_check_tx_current(battery);
#endif
		sec_bat_txpower_calc(battery);
		sec_bat_handle_tx_misalign(battery, false);
		sec_bat_handle_tx_ocp(battery, false);

		if (battery->tx_switch_mode != TX_SWITCH_MODE_OFF && battery->tx_switch_start_soc != 0)
			sec_bat_check_tx_switch_mode(battery);

	} else if (battery->tx_retry_case != SEC_BAT_TX_RETRY_NONE) {
		pr_info("@Tx_Mode %s: tx_retry(0x%x)",__func__, battery->tx_retry_case);
#if !defined(CONFIG_SEC_FACTORY)
		sec_bat_check_tx_temperature(battery);
#endif
		sec_bat_handle_tx_misalign(battery, false);
		sec_bat_handle_tx_ocp(battery, false);
	}
#endif

	/* 0. test mode */
	if (battery->test_mode) {
		dev_err(battery->dev, "%s: Test Mode\n", __func__);
		sec_bat_do_test_function(battery);
		if (battery->test_mode != 0)
			goto continue_monitor;
	}

	/* 1. battery check */
	if (!sec_bat_battery_cable_check(battery))
		goto continue_monitor;

	/* 2. voltage check */
	if (!sec_bat_voltage_check(battery))
		goto continue_monitor;

	/* monitor short routine in initial monitor */
	if (battery->pdata->monitor_initial_count || sec_bat_is_short_polling(battery))
		goto skip_current_monitor;

	/* 3. time management */
	if (!sec_bat_time_management(battery))
		goto continue_monitor;

	/* 4. temperature check */
	if (!sec_bat_temperature_check(battery))
		goto continue_monitor;

#if defined(CONFIG_BATTERY_SWELLING)
	/* 5. swelling check */
	sec_bat_swelling_check(battery);

	/* 6. full charging check */
	if ((battery->swelling_mode == SWELLING_MODE_CHARGING || battery->swelling_mode == SWELLING_MODE_FULL) &&
		(!battery->charging_block))
		sec_bat_swelling_fullcharged_check(battery);
	else
		sec_bat_fullcharged_check(battery);
#else
	/* 5. full charging check */
	sec_bat_fullcharged_check(battery);
#endif /* CONFIG_BATTERY_SWELLING */

	/* 7. additional check */
	if (battery->pdata->monitor_additional_check)
		battery->pdata->monitor_additional_check();

	if (is_nv_wireless_type(battery->cable_type) &&
		(!battery->wc_cv_mode) &&
		(battery->charging_passed_time > 10))
		sec_bat_wc_cv_mode_check(battery);

#if defined(CONFIG_STEP_CHARGING)
#if defined(CONFIG_DIRECT_CHARGING)
	if (is_pd_apdo_wire_type(battery->cable_type))
		sec_bat_check_dc_step_charging(battery);
#endif
#endif

continue_monitor:
	/* clear HEATING_CONTROL*/
	sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL);

	/* calculate safety time */
	if (!battery->charging_block)
		sec_bat_calculate_safety_time(battery);

	/* set charging current */
	sec_bat_set_charging_current(battery);

skip_current_monitor:
	psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_EXT_PROP_MONITOR_WORK, val);

	if (battery->pdata->wireless_charger_name)
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_MONITOR_WORK, val);

	dev_dbg(battery->dev,
		"%s: HLT(%d) HLR(%d) HT(%d), HR(%d), LT(%d), LR(%d), lpcharge(%d)\n",
		__func__, battery->temp_highlimit_threshold, battery->temp_highlimit_recovery,
		battery->temp_high_threshold, battery->temp_high_recovery,
		battery->temp_low_threshold, battery->temp_low_recovery, lpcharge);

	pr_info("%s: Status(%s), mode(%s), Health(%s), Cable(%s, %s, %d, %d), rp(%d), level(%d%%), lcd(%d), slate_mode(%d), store_mode(%d)"
#if defined(CONFIG_AFC_CHARGER_MODE)
		", HV(%s, %d), sleep_mode(%d)"
#endif
#if defined(CONFIG_BATTERY_AGE_FORECAST)
		", Cycle(%d)"
#endif
		 "\n", __func__,
		 sec_bat_status_str[battery->status],
		 sec_bat_charging_mode_str[battery->charging_mode],
		 sec_bat_health_str[battery->health],
		 sec_cable_type[battery->cable_type],
		 sec_cable_type[battery->wire_status],
		 battery->muic_cable_type,
		 battery->pd_usb_attached,
		 battery->pdic_info.sink_status.rp_currentlvl,
		 battery->siop_level,
		 battery->lcd_status,
		 is_slate_mode(battery),
		 battery->store_mode
#if defined(CONFIG_AFC_CHARGER_MODE)
		, battery->hv_chg_name, battery->vbus_chg_by_siop, sleep_mode
#endif
#if defined(CONFIG_BATTERY_AGE_FORECAST)
		, battery->batt_cycle
#endif
		 );

#if defined(CONFIG_WIRELESS_TX_MODE)
	if (battery->wc_tx_enable) {
		unsigned int vout;
		vout = battery->wc_tx_vout == 0 ? 5000 : (5000 + (battery->wc_tx_vout * 500));
		pr_info("@Tx_Mode %s: Rx(%s), WC_TX_VOUT(%dmV), UNO_IOUT(%d), MFC_IOUT(%d) AFC_DISABLE(%d)\n",
			__func__, sec_bat_rx_type_str[battery->wc_rx_type],
			vout, battery->tx_uno_iout, battery->tx_mfc_iout, battery->afc_disable);
	}
#endif

#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	pr_info("%s: battery->stability_test(%d), battery->eng_not_full_status(%d)\n",
			__func__, battery->stability_test, battery->eng_not_full_status);
#endif
#if defined(CONFIG_SEC_FACTORY)
	if (!is_nocharge_type(battery->cable_type)) {
#else
	if (!is_nocharge_type(battery->cable_type) && battery->store_mode) {
#endif
		pr_info("%s: @battery->capacity = (%d), battery->status= (%d), battery->store_mode=(%d)\n",
			 __func__, battery->capacity, battery->status, battery->store_mode);

		if (battery->capacity >= battery->pdata->store_mode_charging_max) {
			int chg_mode = battery->misc_event &
				(BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE | BATT_MISC_EVENT_HICCUP_TYPE | BATT_MISC_EVENT_TEMP_HICCUP_TYPE) ?
					SEC_BAT_CHG_MODE_BUCK_OFF : SEC_BAT_CHG_MODE_CHARGING_OFF;
			/* to discharge the battery, off buck */
			if (battery->capacity > battery->pdata->store_mode_charging_max)
				chg_mode = SEC_BAT_CHG_MODE_BUCK_OFF;

			sec_bat_set_charging_status(battery,
						    POWER_SUPPLY_STATUS_DISCHARGING);
			sec_bat_set_charge(battery, chg_mode);
		}

		if ((battery->capacity <= battery->pdata->store_mode_charging_min) && (battery->status == POWER_SUPPLY_STATUS_DISCHARGING)) {
			sec_bat_set_charging_status(battery,
						    POWER_SUPPLY_STATUS_CHARGING);
			sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING);
		}
	}
	power_supply_changed(battery->psy_bat);

skip_monitor:
	sec_bat_set_polling(battery);

#if defined(CONFIG_WIRELESS_TX_MODE)
	if (battery->tx_switch_mode_change) {
		cancel_delayed_work(&battery->wpc_tx_work);
		wake_lock(&battery->wpc_tx_wake_lock);
		queue_delayed_work(battery->monitor_wqueue,
				&battery->wpc_tx_work, 0);
	}
#endif

	if (battery->capacity <= 0 || battery->health_change)
		wake_lock_timeout(&battery->monitor_wake_lock, HZ * 5);
	else
		wake_unlock(&battery->monitor_wake_lock);

	dev_dbg(battery->dev, "%s: End\n", __func__);

	return;
}

static enum alarmtimer_restart sec_bat_alarm(
	struct alarm *alarm, ktime_t now)
{
	struct sec_battery_info *battery = container_of(alarm,
				struct sec_battery_info, polling_alarm);

	dev_dbg(battery->dev,
			"%s\n", __func__);

	/* In wake up, monitor work will be queued in complete function
	 * To avoid duplicated queuing of monitor work,
	 * do NOT queue monitor work in wake up by polling alarm
	 */
	if (!battery->polling_in_sleep) {
		wake_lock(&battery->monitor_wake_lock);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		dev_dbg(battery->dev, "%s: Activated\n", __func__);
	}

	return ALARMTIMER_NORESTART;
}

#if defined(CONFIG_CALC_TIME_TO_FULL)
static void sec_bat_predict_wireless20_time_to_full_current(struct sec_battery_info *battery, int rx_power)
{
	battery->ttf_predict_wc20_charge_current = battery->pdata->wireless_power_info[rx_power].ttf_charge_current;

	pr_info("%s: %dmA \n", __func__, battery->ttf_predict_wc20_charge_current);
}
#endif

static void sec_bat_set_wireless20_current(struct sec_battery_info *battery, int rx_power)
{
	battery->wc20_vout = battery->pdata->wireless_power_info[rx_power].vout / 100;

	pr_info("%s: vout= %d \n", __func__, battery->wc20_vout);
	if (battery->wc_status == SEC_WIRELESS_PAD_WPC_HV_20) {
		sec_bat_change_default_current(battery, SEC_BATTERY_CABLE_HV_WIRELESS_20,
				battery->pdata->wireless_power_info[rx_power].input_current_limit,
				battery->pdata->wireless_power_info[rx_power].fast_charging_current);

		if (battery->pdata->wireless_power_info[rx_power].rx_power <= 4500)
			battery->wc20_power_class = 0;
		else if (battery->pdata->wireless_power_info[rx_power].rx_power <= 7500)
			battery->wc20_power_class = SEC_WIRELESS_RX_POWER_CLASS_1;
		else if (battery->pdata->wireless_power_info[rx_power].rx_power <= 12000)
			battery->wc20_power_class = SEC_WIRELESS_RX_POWER_CLASS_2;
		else if (battery->pdata->wireless_power_info[rx_power].rx_power <= 20000)
			battery->wc20_power_class = SEC_WIRELESS_RX_POWER_CLASS_3;
		else
			battery->wc20_power_class = SEC_WIRELESS_RX_POWER_CLASS_4;

		if (is_wired_type(battery->cable_type)) {
			int wl_power = battery->pdata->wireless_power_info[rx_power].rx_power ;

			pr_info("%s: check power(%d <--> %d)\n",
				__func__, battery->max_charge_power, wl_power);
			if (battery->max_charge_power < wl_power) {
				wake_lock(&battery->cable_wake_lock);
				queue_delayed_work(battery->monitor_wqueue,
					&battery->cable_work, 0);
			}
		} else {
			sec_bat_set_charging_current(battery);
		}
	}
}

u8 sec_bat_get_wireless20_power_class(struct sec_battery_info *battery)
{
	u8 power_class = 0;

	if (battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_20) {
		power_class = battery->wc20_power_class;
	} else
		power_class = SEC_WIRELESS_RX_POWER_CLASS_1;

	pr_info("%s: power class = %d \n", __func__, power_class);

	return power_class;
}

static void sec_bat_check_input_voltage(struct sec_battery_info *battery)
{
	unsigned int voltage = 0;
	int input_current = battery->pdata->charging_current[battery->cable_type].input_current_limit;

	if (is_pd_wire_type(battery->cable_type)) {
		battery->max_charge_power = battery->pd_max_charge_power;
		return;
	}
	else if (is_hv_wire_12v_type(battery->cable_type))
		voltage = SEC_INPUT_VOLTAGE_12V;
	else if (is_hv_wire_9v_type(battery->cable_type))
		voltage = SEC_INPUT_VOLTAGE_9V;
	else if (battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_20 ||
			battery->cable_type == SEC_BATTERY_CABLE_HV_WIRELESS_20)
		voltage = battery->wc20_vout;	
	else if (is_hv_wireless_type(battery->cable_type) ||
			battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV)
		voltage = SEC_INPUT_VOLTAGE_10V;
	else if (is_nv_wireless_type(battery->cable_type))
		voltage = SEC_INPUT_VOLTAGE_5_5V;
	else
		voltage = SEC_INPUT_VOLTAGE_5V;

	battery->input_voltage = voltage;
	battery->charge_power = (voltage * input_current) / 10;
#if !defined(CONFIG_SEC_FACTORY)
	if (battery->charge_power > battery->max_charge_power)
#endif
	battery->max_charge_power = battery->charge_power;

	pr_info("%s: battery->input_voltage : %d.%dV, %dmW, %dmW)\n", __func__,
		battery->input_voltage/10, battery->input_voltage%10, battery->charge_power, battery->max_charge_power);
}

static void sec_bat_wpc_tx_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, wpc_tx_work.work);

	dev_info(battery->dev, "@Tx_Mode %s: Start\n", __func__);

	switch (battery->wc_rx_type) {
	case NO_DEV:
		if (is_hv_wire_type(battery->wire_status)) {
			pr_info("@Tx_Mode %s : charging voltage change(9V -> 5V).\n", __func__);
			muic_afc_set_voltage(SEC_INPUT_VOLTAGE_5V/10);
			break;
		}

#if defined(CONFIG_DIRECT_CHARGING)
		if (is_pd_apdo_wire_type(battery->wire_status) && battery->pd_list.now_isApdo) {
			pr_info("@Tx_Mode %s: PD30 source charnge (APDO -> Fixed). Because Tx Start.\n", __func__);
			sec_bat_set_charge(battery, battery->charger_mode);
			break;
		} else if (is_pd_wire_type(battery->wire_status) && battery->hv_pdo) {
#else
		if (is_pd_wire_type(battery->wire_status) && battery->hv_pdo) {
#endif
			pr_info("@Tx_Mode %s: PD charnge pdo (9V -> 5V). Because Tx Start.\n", __func__);
			sec_bat_change_pdo(battery, SEC_INPUT_VOLTAGE_5V);
			break;
		}

		if (battery->afc_disable) {
			battery->afc_disable = false;
			muic_hv_charger_disable(battery->afc_disable);
		}

		if (!battery->buck_cntl_by_tx) {
			battery->buck_cntl_by_tx = true;
			sec_bat_set_charge(battery, battery->charger_mode);
		}

		if (!battery->uno_en) {
			battery->buck_cntl_by_tx = true;
			sec_bat_wireless_uno_cntl(battery, true);
		}

		sec_bat_wireless_vout_cntl(battery, WC_TX_VOUT_5_0V);
		sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_gear);

		break;
	case SS_GEAR:
		if (!battery->afc_disable) {
			battery->afc_disable = true;
			muic_hv_charger_disable(battery->afc_disable);
		}

		if (is_hv_wire_type(battery->wire_status)) {
			pr_info("@Tx_Mode %s : charging voltage change(9V -> 5V).\n", __func__);
			muic_afc_set_voltage(SEC_INPUT_VOLTAGE_5V/10);
			break;
		} else if (is_pd_wire_type(battery->wire_status) && battery->hv_pdo) {
			pr_info("@Tx_Mode %s: PD charnge pdo (9V -> 5V). Because Tx Start.\n", __func__);
			sec_bat_change_pdo(battery, SEC_INPUT_VOLTAGE_5V);
			break;
		}

		if (is_wired_type(battery->wire_status) && battery->buck_cntl_by_tx) {
			battery->buck_cntl_by_tx = false;
			sec_bat_set_charge(battery, battery->charger_mode);
		} else if ((battery->wire_status == SEC_BATTERY_CABLE_NONE) && (!battery->buck_cntl_by_tx)) {
			battery->buck_cntl_by_tx = true;
			sec_bat_set_charge(battery, battery->charger_mode);
		}

		sec_bat_wireless_vout_cntl(battery, WC_TX_VOUT_5_0V);
		sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_gear);

		break;
	default:
		 if (battery->wire_status == SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT) {
			pr_info("@Tx_Mode %s : charging voltage change(5V -> 9V)\n", __func__);
			muic_afc_set_voltage(SEC_INPUT_VOLTAGE_9V/10);
			break;
		} else if (is_pd_wire_type(battery->wire_status) && !battery->hv_pdo) {
			pr_info("@Tx_Mode %s: PD charnge pdo (5V -> 9V). Because Tx Start.\n", __func__);
			sec_bat_change_pdo(battery, SEC_INPUT_VOLTAGE_9V);
			break;
		}

		 if (battery->wire_status == SEC_BATTERY_CABLE_NONE) {

			battery->tx_switch_mode = TX_SWITCH_MODE_OFF;
			battery->tx_switch_start_soc = 0;
			battery->tx_switch_mode_change = false;

			if (!battery->buck_cntl_by_tx) {
				battery->buck_cntl_by_tx = true;
				sec_bat_set_charge(battery, battery->charger_mode);
			}

			sec_bat_wireless_vout_cntl(battery, WC_TX_VOUT_7_5V);
			sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_phone);
			sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_default);

		} else if (is_hv_wire_type(battery->wire_status) || (is_pd_wire_type(battery->wire_status) && battery->hv_pdo)) {

			battery->tx_switch_mode = TX_SWITCH_MODE_OFF;
			battery->tx_switch_start_soc = 0;
			battery->tx_switch_mode_change = false;

			if (battery->buck_cntl_by_tx) {
				battery->buck_cntl_by_tx = false;
				sec_bat_set_charge(battery, battery->charger_mode);
			}

			sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_phone);
			sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_default);
		} else if (is_pd_wire_type(battery->wire_status) && battery->hv_pdo) {

			pr_info("@Tx_Mode %s: PD cable attached. HV PDO(%d)\n", __func__, battery->hv_pdo);

			battery->tx_switch_mode = TX_SWITCH_MODE_OFF;
			battery->tx_switch_start_soc = 0;
			battery->tx_switch_mode_change = false;

			if (battery->buck_cntl_by_tx) {
				battery->buck_cntl_by_tx = false;
				sec_bat_set_charge(battery, battery->charger_mode);
			}

			sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_phone);
			sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_default);
		} else if (is_wired_type(battery->wire_status) && !is_hv_wire_type(battery->wire_status) && (battery->wire_status != SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT)) {
			if (battery->current_event & SEC_BAT_CURRENT_EVENT_AFC)	{
				if (!battery->buck_cntl_by_tx) {
					battery->buck_cntl_by_tx = true;
					sec_bat_set_charge(battery, battery->charger_mode);
				}

				battery->tx_switch_mode = TX_SWITCH_MODE_OFF;
				battery->tx_switch_start_soc = 0;
				battery->tx_switch_mode_change = false;

				sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_phone);
				sec_bat_wireless_vout_cntl(battery, WC_TX_VOUT_7_5V);
				sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_default);

			} else if (battery->tx_switch_mode == TX_SWITCH_MODE_OFF) {
				battery->tx_switch_mode = TX_SWITCH_UNO_ONLY;
				battery->tx_switch_start_soc = battery->capacity;
				if (!battery->buck_cntl_by_tx) {
					battery->buck_cntl_by_tx = true;
					sec_bat_set_charge(battery, battery->charger_mode);
				}

				sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_phone);
				sec_bat_wireless_vout_cntl(battery, WC_TX_VOUT_7_5V);
				sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_default);

			} else if (battery->tx_switch_mode_change == true) {
				battery->tx_switch_start_soc = battery->capacity;

				pr_info("@Tx_mode: Switch Mode Change(%d -> %d)\n",
					battery->tx_switch_mode,
					battery->tx_switch_mode == TX_SWITCH_UNO_ONLY ?
					TX_SWITCH_CHG_ONLY : TX_SWITCH_UNO_ONLY);

				if (battery->tx_switch_mode == TX_SWITCH_UNO_ONLY) {
					battery->tx_switch_mode = TX_SWITCH_CHG_ONLY;

					sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_phone_5v);
					sec_bat_wireless_vout_cntl(battery, WC_TX_VOUT_5_0V);
					sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_5V);

					if (battery->buck_cntl_by_tx) {
						battery->buck_cntl_by_tx = false;
						sec_bat_set_charge(battery, battery->charger_mode);
					}

				} else if (battery->tx_switch_mode == TX_SWITCH_CHG_ONLY) {
					union power_supply_propval value = {0, };
					battery->tx_switch_mode = TX_SWITCH_UNO_ONLY;

					if (!battery->buck_cntl_by_tx) {
						battery->buck_cntl_by_tx = true;
						sec_bat_set_charge(battery, battery->charger_mode);
					}

					sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_phone);
					sec_bat_wireless_vout_cntl(battery, WC_TX_VOUT_7_5V);
					sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_default);

					value.intval = true;
					psy_do_property(battery->pdata->wireless_charger_name, set,
						POWER_SUPPLY_EXT_PROP_WIRELESS_SEND_FSK, value);

				}
				battery->tx_switch_mode_change = false;
			}

		}
		break;
	}
	wake_unlock(&battery->wpc_tx_wake_lock);
	dev_info(battery->dev, "@Tx_Mode %s End\n", __func__);
}

static void sec_bat_cable_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, cable_work.work);
	union power_supply_propval val = {0, };
	int current_cable_type = SEC_BATTERY_CABLE_NONE, current_wire_status = battery->wire_status;

	dev_info(battery->dev, "%s: Start\n", __func__);
	sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL,
				  SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL);
#if defined(CONFIG_CCIC_NOTIFIER)
	if (is_pd_wire_type(current_wire_status)) {
		if (battery->pdic_info.sink_status.selected_pdo_num ==
			battery->pdic_info.sink_status.current_pdo_num) {
			sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SELECT_PDO);
			sec_bat_get_input_current_in_power_list(battery);
			sec_bat_get_charging_current_in_power_list(battery);
		}
#if defined(CONFIG_STEP_CHARGING) && defined(CONFIG_DIRECT_CHARGING)
		if (is_pd_apdo_wire_type(battery->cable_type) && (battery->ta_alert_mode != OCP_NONE)) {
			battery->ta_alert_mode = OCP_WA_ACTIVE;
			sec_bat_reset_step_charging(battery);
		}
#endif
	}
#endif

	if (battery->wc_status && battery->wc_enable) {
		int wireless_current, wire_current;
		int temp_current_type;

		if (battery->wc_status == SEC_WIRELESS_PAD_WPC)
			current_cable_type = SEC_BATTERY_CABLE_WIRELESS;
		else if (battery->wc_status == SEC_WIRELESS_PAD_WPC_HV)
			current_cable_type = SEC_BATTERY_CABLE_HV_WIRELESS;
		else if (battery->wc_status == SEC_WIRELESS_PAD_WPC_PACK)
			current_cable_type = SEC_BATTERY_CABLE_WIRELESS_PACK;
		else if (battery->wc_status == SEC_WIRELESS_PAD_WPC_PACK_HV)
			current_cable_type = SEC_BATTERY_CABLE_WIRELESS_HV_PACK;
		else if (battery->wc_status == SEC_WIRELESS_PAD_WPC_STAND)
			current_cable_type = SEC_BATTERY_CABLE_WIRELESS_STAND;
		else if (battery->wc_status == SEC_WIRELESS_PAD_WPC_STAND_HV)
			current_cable_type = SEC_BATTERY_CABLE_WIRELESS_HV_STAND;
		else if (battery->wc_status == SEC_WIRELESS_PAD_VEHICLE)
			current_cable_type = SEC_BATTERY_CABLE_WIRELESS_VEHICLE;
		else if (battery->wc_status == SEC_WIRELESS_PAD_VEHICLE_HV)
			current_cable_type = SEC_BATTERY_CABLE_WIRELESS_HV_VEHICLE;
		else if (battery->wc_status == SEC_WIRELESS_PAD_PREPARE_HV)
			current_cable_type = SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV;
		else if (battery->wc_status == SEC_WIRELESS_PAD_TX)
			current_cable_type = SEC_BATTERY_CABLE_WIRELESS_TX;
		else if (battery->wc_status == SEC_WIRELESS_PAD_WPC_PREPARE_HV_20)
			current_cable_type = SEC_BATTERY_CABLE_PREPARE_WIRELESS_20;
		else if (battery->wc_status == SEC_WIRELESS_PAD_WPC_HV_20)
			current_cable_type = SEC_BATTERY_CABLE_HV_WIRELESS_20;
		else if (battery->wc_status == SEC_WIRELESS_PAD_FAKE)
			current_cable_type = SEC_BATTERY_CABLE_WIRELESS_FAKE;
		else
			current_cable_type = SEC_BATTERY_CABLE_PMA_WIRELESS;

		if (current_cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV)
			temp_current_type = SEC_BATTERY_CABLE_HV_WIRELESS;
		else if (current_cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_20)
			temp_current_type = SEC_BATTERY_CABLE_HV_WIRELESS_20;
		else
			temp_current_type = current_cable_type;

		if (!is_nocharge_type(current_wire_status)) {
			wireless_current = battery->pdata->charging_current[temp_current_type].input_current_limit;
			if (is_nv_wireless_type(temp_current_type))
				wireless_current = (wireless_current * SEC_INPUT_VOLTAGE_5_5V) / 10;
			else if (temp_current_type == SEC_BATTERY_CABLE_HV_WIRELESS_20)
				wireless_current = (wireless_current * battery->wc20_vout) / 10;
			else
				wireless_current = (wireless_current * SEC_INPUT_VOLTAGE_10V) / 10;

			wire_current = (battery->wire_status == SEC_BATTERY_CABLE_PREPARE_TA ?
				battery->pdata->charging_current[SEC_BATTERY_CABLE_TA].input_current_limit :
				battery->pdata->charging_current[battery->wire_status].input_current_limit);

			wire_current = wire_current * (is_hv_wire_type(battery->wire_status) ?
				(battery->wire_status == SEC_BATTERY_CABLE_12V_TA ? SEC_INPUT_VOLTAGE_12V : SEC_INPUT_VOLTAGE_9V) / 10
				: SEC_INPUT_VOLTAGE_5V / 10);

			if (is_pd_wire_type(current_wire_status)) {
				pr_info("%s: wl_cur(%d), pd_max_power(%d), wc_cable_type(%d), wire_cable_type(%d)\n",
					__func__, wireless_current, battery->pd_max_charge_power, current_cable_type, current_wire_status);
			} else {
				pr_info("%s: wl_cur(%d), wr_cur(%d), wc_cable_type(%d), wire_cable_type(%d)\n",
					__func__, wireless_current, wire_current, current_cable_type, current_wire_status);
			}

			if ((is_pd_wire_type(current_wire_status) && wireless_current < battery->pd_max_charge_power) ||
				(!is_pd_wire_type(current_wire_status) && wireless_current <= wire_current)) {
				current_cable_type = current_wire_status;
				pr_info("%s : switch charging path to cable\n", __func__);

				/* set limited charging current before switching cable charging from wireless charging,
				   this step for wireless 2.0 -> HV cable charging */
				if((battery->cable_type == SEC_BATTERY_CABLE_HV_WIRELESS_20) &&
					(temp_current_type == SEC_BATTERY_CABLE_HV_WIRELESS_20)) {
					mutex_lock(&battery->iolock);
					val.intval = battery->charging_current = battery->pdata->wpc_charging_limit_current;
					pr_info("%s : set TA charging current %dmA for a moment in case of TA OCP\n", __func__, val.intval);
					psy_do_property(battery->pdata->charger_name, set,
							POWER_SUPPLY_PROP_CURRENT_AVG, val);
					mutex_unlock(&battery->iolock);
					msleep(100);
				}

				battery->wc_need_ldo_on = true;
				val.intval = MFC_LDO_OFF;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_PROP_CHARGE_EMPTY, val);
				/* Turn off TX to charge by cable charging having more power */
				if (battery->wc_status == SEC_WIRELESS_PAD_TX) {
					pr_info("@Tx_Mode %s : It is RX device with TA, notify TX device of this info\n", __func__);
					val.intval = true;
					psy_do_property(battery->pdata->wireless_charger_name, set,
						POWER_SUPPLY_EXT_PROP_WIRELESS_SWITCH, val);
				}
			} else {
				pr_info("%s : switch charging path to wireless\n", __func__);
				battery->wc_need_ldo_on = false;
				val.intval = MFC_LDO_ON;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_PROP_CHARGE_EMPTY, val);
			}
		} else {
			/* turn on ldo when ldo was off because of TA, ldo is supposed to turn on automatically except force off by sw.
			   do not turn on ldo every wireless connection just in case ldo re-toggle by ic */
			if(battery->wc_need_ldo_on) {
				battery->wc_need_ldo_on = false;
				val.intval = MFC_LDO_ON;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_PROP_CHARGE_EMPTY, val);
			}
		}
	} else {
		sec_bat_change_default_current(battery, SEC_BATTERY_CABLE_HV_WIRELESS_20,
			battery->pdata->default_wc20_input_current,
			battery->pdata->default_wc20_charging_current);

		current_cable_type = current_wire_status;
	}

#if defined(CONFIG_BATTERY_SWELLING)
	if (is_nocharge_type(current_cable_type) ||
		(is_nocharge_type(battery->cable_type) && battery->swelling_mode == SWELLING_MODE_NONE)) {
		battery->swelling_mode = SWELLING_MODE_NONE;
		/* restore 4.4V float voltage */
		val.intval = battery->pdata->swelling_normal_float_voltage;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_VOLTAGE_MAX, val);
		pr_info("%s: float voltage = %d\n", __func__, val.intval);
	} else {
#if defined(CONFIG_STEP_CHARGING)
		sec_bat_reset_step_charging(battery);
#endif
		pr_info("%s: skip  float_voltage setting, swelling_mode(%d)\n",
			__func__, battery->swelling_mode);
	}
#endif

	if ((current_cable_type == battery->cable_type) && !is_slate_mode(battery)) {
		if (is_pd_wire_type(current_cable_type) && is_pd_wire_type(battery->cable_type)) {
			cancel_delayed_work(&battery->afc_work);
			wake_unlock(&battery->afc_wake_lock);
			sec_bat_set_current_event(battery, 0,
				SEC_BAT_CURRENT_EVENT_AFC | SEC_BAT_CURRENT_EVENT_AICL);
			battery->aicl_current = 0;
			sec_bat_set_charging_current(battery);
			power_supply_changed(battery->psy_bat);
		}
		dev_info(battery->dev, "%s: Cable is NOT Changed(%d)\n",
			__func__, battery->cable_type);
		/* Do NOT activate cable work for NOT changed */
		goto end_of_cable_work;
	}

	/* to clear this value when cable type switched without dettach */
	if ((is_wired_type(battery->cable_type) && is_wireless_type(current_cable_type))
		|| (is_wireless_type(battery->cable_type) && is_wired_type(current_cable_type))
		|| (battery->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_DISABLED_MUIC))
		battery->max_charge_power = 0;

	if (current_cable_type == SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT)
		current_cable_type = SEC_BATTERY_CABLE_9V_TA;

	battery->cable_type = current_cable_type;
	if (is_wireless_type(battery->cable_type)) {
		power_supply_changed(battery->psy_bat);
		/* After 10sec wireless charging, Vrect headroom has to be reduced */
		wake_lock(&battery->wc_headroom_wake_lock);
		queue_delayed_work(battery->monitor_wqueue, &battery->wc_headroom_work,
			msecs_to_jiffies(10000));
	} else if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_FAKE) {
		power_supply_changed(battery->psy_bat);
	}

	if (battery->pdata->check_cable_result_callback)
		battery->pdata->check_cable_result_callback(battery->cable_type);
	/* platform can NOT get information of cable connection
	 * because wakeup time is too short to check uevent
	 * To make sure that target is wakeup
	 * if cable is connected and disconnected,
	 * activated wake lock in a few seconds
	 */
	wake_lock_timeout(&battery->vbus_wake_lock, HZ * 10);

	if (is_nocharge_type(battery->cable_type) ||
		((battery->pdata->cable_check_type &
		SEC_BATTERY_CABLE_CHECK_NOINCOMPATIBLECHARGE) &&
		battery->cable_type == SEC_BATTERY_CABLE_UNKNOWN)) {
		/* initialize all status */
		battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
		battery->vbus_chg_by_siop = SEC_INPUT_VOLTAGE_NONE;
		battery->vbus_chg_by_full = false;
		battery->is_recharging = false;
#if defined(CONFIG_BATTERY_CISD)
		battery->cisd.ab_vbat_check_count = 0;
		battery->cisd.state &= ~CISD_STATE_OVER_VOLTAGE;
#endif
		battery->wc20_power_class = 0;
#if defined(CONFIG_CALC_TIME_TO_FULL)
		battery->ttf_predict_wc20_charge_current = 0;
#endif
		battery->wc20_vout = 0;
		battery->input_voltage = 0;
		battery->charge_power = 0;
		battery->max_charge_power = 0;
		battery->pd_max_charge_power = 0;
		sec_bat_set_charging_status(battery,
				POWER_SUPPLY_STATUS_DISCHARGING);
		battery->chg_limit = false;
		battery->lrp_limit = false;
		battery->lrp_step = LRP_NONE;
		battery->mix_limit = false;
		battery->chg_limit_recovery_cable = SEC_BATTERY_CABLE_NONE;
		battery->wc_heating_start_time = 0;
		battery->health = POWER_SUPPLY_HEALTH_GOOD;

		battery->ta_alert_mode = OCP_NONE;
		cancel_delayed_work(&battery->afc_work);
		wake_unlock(&battery->afc_wake_lock);
		sec_bat_change_default_current(battery, SEC_BATTERY_CABLE_USB,
			battery->pdata->default_usb_input_current,
			battery->pdata->default_usb_charging_current);
		sec_bat_change_default_current(battery, SEC_BATTERY_CABLE_TA,
			battery->pdata->default_input_current,
			battery->pdata->default_charging_current);
		sec_bat_change_default_current(battery, SEC_BATTERY_CABLE_HV_WIRELESS_20,
			battery->pdata->default_wc20_input_current,
			battery->pdata->default_wc20_charging_current);
		/* usb default current is 100mA before configured*/
		sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_USB_100MA,
					  (SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE |
					   SEC_BAT_CURRENT_EVENT_AFC |
					   SEC_BAT_CURRENT_EVENT_USB_SUPER |
					   SEC_BAT_CURRENT_EVENT_USB_100MA |
					   SEC_BAT_CURRENT_EVENT_VBAT_OVP |
					   SEC_BAT_CURRENT_EVENT_VSYS_OVP |
					   SEC_BAT_CURRENT_EVENT_CHG_LIMIT |
					   SEC_BAT_CURRENT_EVENT_AICL |
					   SEC_BAT_CURRENT_EVENT_SELECT_PDO |
					   SEC_BAT_CURRENT_EVENT_25W_OCP |
					   SEC_BAT_CURRENT_EVENT_DC_ERR |
					   SEC_BAT_CURRENT_EVENT_SEND_UVDM));

#if defined(CONFIG_ENABLE_100MA_CHARGING_BEFORE_USB_CONFIGURED)
		cancel_delayed_work(&battery->slowcharging_work);
#endif
		battery->wc_cv_mode = false;
		battery->is_sysovlo = false;
		battery->is_vbatovlo = false;
		battery->is_abnormal_temp = false;
		battery->auto_mode = false;
#if defined(CONFIG_PREVENT_USB_CONN_OVERHEAT)
		if (lpcharge)
			battery->usb_temp_flag = false;
#endif

		sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
		battery->usb_slow_chg = false;
	} else if (is_slate_mode(battery)) {
		dev_info(battery->dev,
			"%s:slate mode on\n",__func__);
		battery->is_recharging = false;
		battery->cable_type = SEC_BATTERY_CABLE_NONE;
		battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
		battery->health = POWER_SUPPLY_HEALTH_GOOD;
		battery->is_sysovlo = false;
		battery->is_vbatovlo = false;
		battery->is_abnormal_temp = false;
		sec_bat_set_charging_status(battery,
			POWER_SUPPLY_STATUS_DISCHARGING);
		sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_BUCK_OFF);
	} else {
#if defined(CONFIG_EN_OOPS)
		val.intval = battery->cable_type;
		psy_do_property(battery->pdata->fuelgauge_name, set,
				POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN, val);
#endif
		/* Do NOT display the charging icon when OTG or HMT_CONNECTED is enabled */
		if (battery->cable_type == SEC_BATTERY_CABLE_OTG ||
			battery->cable_type == SEC_BATTERY_CABLE_POWER_SHARING) {
			battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
			battery->status = POWER_SUPPLY_STATUS_DISCHARGING;
		} else if (battery->misc_event & BATT_MISC_EVENT_FULL_CAPACITY) {
			battery->status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		} else if (!battery->is_sysovlo && !battery->is_vbatovlo && !battery->is_abnormal_temp &&
				(!battery->charging_block || !battery->swelling_mode)) {
			if (battery->pdata->full_check_type !=
				SEC_BATTERY_FULLCHARGED_NONE)
				battery->charging_mode =
					SEC_BATTERY_CHARGING_1ST;
			else
				battery->charging_mode =
					SEC_BATTERY_CHARGING_2ND;

			if (battery->status == POWER_SUPPLY_STATUS_FULL)
				sec_bat_set_charging_status(battery,
						POWER_SUPPLY_STATUS_FULL);
			else
				sec_bat_set_charging_status(battery,
						POWER_SUPPLY_STATUS_CHARGING);
		}

		if (!battery->is_sysovlo && !battery->is_vbatovlo && !battery->is_abnormal_temp)
			battery->health = POWER_SUPPLY_HEALTH_GOOD;

		if (battery->cable_type == SEC_BATTERY_CABLE_TA ||
			battery->cable_type == SEC_BATTERY_CABLE_WIRELESS ||
			battery->cable_type == SEC_BATTERY_CABLE_PMA_WIRELESS ||
			(is_hv_wire_type(battery->cable_type) &&
			(battery->wc_status == SEC_WIRELESS_PAD_WPC_PREPARE_HV_20 ||
			battery->wc_status == SEC_WIRELESS_PAD_WPC_HV_20))) {
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_AFC, SEC_BAT_CURRENT_EVENT_AFC);
		} else {
			cancel_delayed_work(&battery->afc_work);
			wake_unlock(&battery->afc_wake_lock);
			sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_AFC);
		}

		if (battery->cable_type == SEC_BATTERY_CABLE_OTG ||
			battery->cable_type == SEC_BATTERY_CABLE_POWER_SHARING) {
			sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
			goto end_of_cable_work;
		} else if (!battery->is_sysovlo && !battery->is_vbatovlo && !battery->is_abnormal_temp &&
				(!battery->charging_block || !battery->swelling_mode)) {
#if defined(CONFIG_ENABLE_FULL_BY_SOC)
			if (battery->capacity >= 100) {
				sec_bat_do_fullcharged(battery, true);
				dev_info(battery->dev,
					"%s: charging start at full, do not turn on charging\n", __func__);
			} else
#endif

			sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING);
		}

#if defined(CONFIG_ENABLE_100MA_CHARGING_BEFORE_USB_CONFIGURED)
		if (battery->cable_type == SEC_BATTERY_CABLE_USB && !lpcharge)
			queue_delayed_work(battery->monitor_wqueue, &battery->slowcharging_work,
						msecs_to_jiffies(3000));
#endif

#if defined(CONFIG_CALC_TIME_TO_FULL)
		if (lpcharge) {
			cancel_delayed_work(&battery->timetofull_work);
			if (battery->current_event & SEC_BAT_CURRENT_EVENT_AFC) {
				int work_delay = 0;

				if (!is_wireless_type(battery->cable_type)) {
					work_delay = battery->pdata->pre_afc_work_delay;
				} else {
					work_delay = battery->pdata->pre_wc_afc_work_delay;
				}

				queue_delayed_work(battery->monitor_wqueue,
					&battery->timetofull_work, msecs_to_jiffies(work_delay));
			}
		}
#endif
	}

	if (battery->cable_type != SEC_BATTERY_CABLE_WIRELESS_FAKE) {
		/* set online(cable type) */
		val.intval = battery->cable_type;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_ONLINE, val);
		psy_do_property(battery->pdata->fuelgauge_name, set,
			POWER_SUPPLY_PROP_ONLINE, val);
		/* set charging current */
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_PROP_CURRENT_AVG, val);
		battery->aicl_current = 0;
		sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_AICL);
		battery->input_current = val.intval;
		/* to init battery type current when wireless charging -> battery case */
		if (is_nocharge_type(battery->cable_type))
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_CURRENT_MAX, val);
		if (battery->status != POWER_SUPPLY_STATUS_DISCHARGING)
			sec_bat_check_input_voltage(battery);
		sec_bat_set_charging_current(battery);
	}

	/* polling time should be reset when cable is changed
	 * polling_in_sleep should be reset also
	 * before polling time is re-calculated
	 * to prevent from counting 1 for events
	 * right after cable is connected
	 */
	battery->polling_in_sleep = false;
	sec_bat_get_polling_time(battery);

	dev_info(battery->dev,
		"%s: Status:%s, Sleep:%s, Charging:%s, Short Poll:%s\n",
		__func__, sec_bat_status_str[battery->status],
		battery->polling_in_sleep ? "Yes" : "No",
		(battery->charging_mode ==
		SEC_BATTERY_CHARGING_NONE) ? "No" : "Yes",
		battery->polling_short ? "Yes" : "No");
	dev_info(battery->dev,
		"%s: Polling time is reset to %d sec.\n", __func__,
		battery->polling_time);

	battery->polling_count = 1;	/* initial value = 1 */

	wake_lock(&battery->monitor_wake_lock);
	queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
end_of_cable_work:
	wake_unlock(&battery->cable_wake_lock);
	dev_info(battery->dev, "%s: End\n", __func__);
}

static void sec_bat_afc_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, afc_work.work);
	union power_supply_propval value = {0, };

	dev_info(battery->dev, "%s: start\n", __func__);
	psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_PROP_CURRENT_MAX, value);
	battery->current_max = value.intval;

	if (battery->current_event & SEC_BAT_CURRENT_EVENT_AFC) {
		sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_AFC);
		if ((battery->wc_status != SEC_WIRELESS_PAD_NONE &&
		     battery->current_max >= battery->pdata->pre_wc_afc_input_current) ||
		    ((is_hv_wire_type(battery->cable_type) || battery->cable_type == SEC_BATTERY_CABLE_TA) &&
		     battery->current_max >= battery->pdata->pre_afc_input_current)) {
			sec_bat_set_charging_current(battery);
			if (battery->cable_type == SEC_BATTERY_CABLE_TA)
				battery->cisd.cable_data[CISD_CABLE_TA]++;
		}
		if (battery->wc_tx_enable) {
			cancel_delayed_work(&battery->wpc_tx_work);
			wake_lock(&battery->wpc_tx_wake_lock);
			queue_delayed_work(battery->monitor_wqueue,
					&battery->wpc_tx_work, 0);
		}
	}
	dev_info(battery->dev, "%s: End\n", __func__);
	wake_unlock(&battery->afc_wake_lock);
}

static int sec_bat_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	int current_cable_type = SEC_BATTERY_CABLE_NONE;
	int full_check_type = SEC_BATTERY_FULLCHARGED_NONE;
	union power_supply_propval value = {0, };
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;

	dev_dbg(battery->dev,
		"%s: (%d,%d)\n", __func__, psp, val->intval);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (battery->charging_mode == SEC_BATTERY_CHARGING_1ST)
			full_check_type = battery->pdata->full_check_type;
		else
			full_check_type = battery->pdata->full_check_type_2nd;
		if ((full_check_type == SEC_BATTERY_FULLCHARGED_CHGINT) &&
			(val->intval == POWER_SUPPLY_STATUS_FULL))
			sec_bat_do_fullcharged(battery, false);
		sec_bat_set_charging_status(battery, val->intval);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		if (battery->cable_type != SEC_BATTERY_CABLE_WIRELESS_FAKE)
			sec_bat_ovp_uvlo_result(battery, val->intval);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		current_cable_type = val->intval;
#if !defined(CONFIG_CCIC_NOTIFIER)
		if ((battery->muic_cable_type != ATTACHED_DEV_SMARTDOCK_TA_MUIC)
		    && ((current_cable_type == SEC_BATTERY_CABLE_SMART_OTG) ||
			(current_cable_type == SEC_BATTERY_CABLE_SMART_NOTG)))
			break;
#endif

		if (current_cable_type < 0) {
			dev_info(battery->dev,
					"%s: ignore event(%d)\n",
					__func__, current_cable_type);
		} else if (current_cable_type == SEC_BATTERY_CABLE_OTG) {
			battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
			battery->is_recharging = false;
			sec_bat_set_charging_status(battery,
					POWER_SUPPLY_STATUS_DISCHARGING);
			battery->cable_type = current_cable_type;
			wake_lock(&battery->monitor_wake_lock);
			queue_delayed_work(battery->monitor_wqueue,
					   &battery->monitor_work, 0);
			break;
		} else {
			battery->wire_status = current_cable_type;
			if (is_nocharge_type(battery->wire_status) &&
				(battery->wc_status != SEC_WIRELESS_PAD_NONE) )
				current_cable_type = SEC_BATTERY_CABLE_WIRELESS;
		}
		dev_info(battery->dev,
				"%s: current_cable(%d), wc_status(%d), wire_status(%d)\n",
				__func__, current_cable_type, battery->wc_status,
				battery->wire_status);

		/* cable is attached or detached
		 * if current_cable_type is minus value,
		 * check cable by sec_bat_get_cable_type()
		 * although SEC_BATTERY_CABLE_SOURCE_EXTERNAL is set
		 * (0 is SEC_BATTERY_CABLE_UNKNOWN)
		 */
		if ((current_cable_type >= 0) &&
			(current_cable_type < SEC_BATTERY_CABLE_MAX) &&
			(battery->pdata->cable_source_type &
			SEC_BATTERY_CABLE_SOURCE_EXTERNAL)) {

			wake_lock(&battery->cable_wake_lock);
				queue_delayed_work(battery->monitor_wqueue,
					&battery->cable_work,0);
		} else {
			if (sec_bat_get_cable_type(battery,
						battery->pdata->cable_source_type)) {
				wake_lock(&battery->cable_wake_lock);
					queue_delayed_work(battery->monitor_wqueue,
						&battery->cable_work,0);
			}
		}
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		battery->capacity = val->intval;
		power_supply_changed(battery->psy_bat);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		/* If JIG is attached, the voltage is set as 1079 */
		pr_info("%s : set to the battery history : (%d)\n",__func__, val->intval);
		if(val->intval == 1079)	{
			battery->voltage_now = 1079;
			battery->voltage_avg = 1079;
			power_supply_changed(battery->psy_bat);
		}
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		wake_lock(&battery->monitor_wake_lock);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		battery->present = val->intval;

		wake_lock(&battery->monitor_wake_lock);
		queue_delayed_work(battery->monitor_wqueue,
				   &battery->monitor_work, 0);
		break;
#if defined(CONFIG_BATTERY_SWELLING)
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		break;
#endif
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION:
	case POWER_SUPPLY_PROP_CHARGE_COUNTER_SHADOW:
		break;
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
		value.intval = val->intval;
		pr_info("%s: CHGIN-OTG %s\n", __func__, value.intval > 0 ? "on" : "off");
		psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, value);
		break;
	case POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL:
		value.intval = val->intval;
		pr_info("%s: WCIN-UNO %s\n", __func__, value.intval > 0 ? "on" : "off");
		psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL, value);
		break;
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	case POWER_SUPPLY_PROP_POWER_DESIGN:
		sec_bat_parse_dt(battery->dev, battery);
		break;
#endif
#if defined(CONFIG_BATTERY_CISD)
	case POWER_SUPPLY_PROP_VOLTAGE_MIN:
		pr_info("%s: Valert was occured! run monitor work for updating cisd data!\n", __func__);
		battery->cisd.data[CISD_DATA_VALERT_COUNT]++;
		battery->cisd.data[CISD_DATA_VALERT_COUNT_PER_DAY]++;
		wake_lock(&battery->monitor_wake_lock);
		queue_delayed_work_on(0, battery->monitor_wqueue,
			&battery->monitor_work, 0);
		break;
#endif
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_AICL_CURRENT:
			battery->aicl_current = val->intval;
			battery->max_charge_power = battery->charge_power = (battery->input_voltage * val->intval) / 10;
			pr_info("%s: aicl : %dmA, %dmW)\n", __func__,
				battery->aicl_current, battery->charge_power);
				if (is_wired_type(battery->cable_type))
					sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_AICL,
						SEC_BAT_CURRENT_EVENT_AICL);

#if defined(CONFIG_BATTERY_CISD)
				battery->cisd.data[CISD_DATA_AICL_COUNT]++;
				battery->cisd.data[CISD_DATA_AICL_COUNT_PER_DAY]++;
#endif
			break;
		case POWER_SUPPLY_EXT_PROP_SYSOVLO:
			if (battery->status != POWER_SUPPLY_STATUS_DISCHARGING) {
				pr_info("%s: Vsys is ovlo !!\n", __func__);
				battery->is_sysovlo = true;
				battery->is_recharging = false;
				battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
				battery->health = POWER_SUPPLY_HEALTH_VSYS_OVP;
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_VSYS_OVP, SEC_BAT_CURRENT_EVENT_VSYS_OVP);
				sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);
#if defined(CONFIG_BATTERY_CISD)
				battery->cisd.data[CISD_DATA_VSYS_OVP]++;
				battery->cisd.data[CISD_DATA_VSYS_OVP_PER_DAY]++;
#endif
#if defined(CONFIG_SEC_ABC)
				sec_abc_send_event("MODULE=battery@ERROR=vsys_ovp");
#endif
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
				wake_lock(&battery->monitor_wake_lock);
				queue_delayed_work(battery->monitor_wqueue,
						   &battery->monitor_work, 0);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_VBAT_OVP:
			if (battery->status != POWER_SUPPLY_STATUS_DISCHARGING) {
				pr_info("%s: Vbat is ovlo !!\n", __func__);
				battery->is_vbatovlo = true;
				battery->is_recharging = false;
				battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
				battery->health = POWER_SUPPLY_HEALTH_VBAT_OVP;
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_VBAT_OVP, SEC_BAT_CURRENT_EVENT_VBAT_OVP);
				sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);

				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
				wake_lock(&battery->monitor_wake_lock);
				queue_delayed_work(battery->monitor_wqueue,
						   &battery->monitor_work, 0);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_USB_CONFIGURE:
			pr_info("%s: usb configured %d\n", __func__, val->intval);
			if (val->intval == USB_CURRENT_UNCONFIGURED) {
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_USB_100MA,
							  (SEC_BAT_CURRENT_EVENT_USB_100MA | SEC_BAT_CURRENT_EVENT_USB_SUPER));
			} else if (val->intval == USB_CURRENT_HIGH_SPEED) {
				battery->usb_slow_chg = false;
				sec_bat_set_current_event(battery, 0,
							  (SEC_BAT_CURRENT_EVENT_USB_100MA | SEC_BAT_CURRENT_EVENT_USB_SUPER));
				sec_bat_change_default_current(battery, SEC_BATTERY_CABLE_USB,
					battery->pdata->default_usb_input_current,
					battery->pdata->default_usb_charging_current);
			} else if (val->intval == USB_CURRENT_SUPER_SPEED) {
				battery->usb_slow_chg = false;
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_USB_SUPER,
							  (SEC_BAT_CURRENT_EVENT_USB_100MA | SEC_BAT_CURRENT_EVENT_USB_SUPER));
				sec_bat_change_default_current(battery, SEC_BATTERY_CABLE_USB,
						USB_CURRENT_SUPER_SPEED, USB_CURRENT_SUPER_SPEED);
			}
			sec_bat_set_charging_current(battery);
			break;
		case POWER_SUPPLY_EXT_PROP_OVERHEAT_NOTIFY:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_OVERHEAT_NOTIFY!\n", __func__);
			wake_lock(&battery->monitor_wake_lock);
			queue_delayed_work(battery->monitor_wqueue,
					   &battery->monitor_work, 0);
			break;
		case POWER_SUPPLY_EXT_PROP_HV_DISABLE:
			pr_info("HV wired charging mode is %s\n", (val->intval == CH_MODE_AFC_DISABLE_VAL ? "Disabled" : "Enabled"));
			if (val->intval == CH_MODE_AFC_DISABLE_VAL) {
				sec_bat_set_current_event(battery,
					SEC_BAT_CURRENT_EVENT_HV_DISABLE, SEC_BAT_CURRENT_EVENT_HV_DISABLE);

				if (is_pd_wire_type(battery->cable_type)) {
					battery->update_pd_list = true;
					pr_info("%s: update pd list\n", __func__);
					select_pdo(1);
				}
			} else if (battery->current_event & SEC_BAT_CURRENT_EVENT_HV_DISABLE) {
				int target_pd_index = 0;

				sec_bat_set_current_event(battery,
					0, SEC_BAT_CURRENT_EVENT_HV_DISABLE);

				if (is_pd_wire_type(battery->cable_type)) {
					battery->update_pd_list = true;
					pr_info("%s: update pd list\n", __func__);
#if defined(CONFIG_PDIC_PD30)
					target_pd_index = battery->pd_list.num_fpdo - 1;
#else
					target_pd_index = battery->pd_list.max_pd_count - 1;
#endif
					if (target_pd_index >= 0 && target_pd_index < MAX_PDO_NUM)
						select_pdo(battery->pd_list.pd_info[target_pd_index].pdo_index);
				}
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WC_CONTROL:
			pr_info("%s: Recover MFC IC (wc_enable: %d)\n",
				__func__, battery->wc_enable);

			mutex_lock(&battery->wclock);
			if (battery->wc_enable) {
				sec_bat_set_mfc_off(battery, false);
				msleep(500);

				sec_bat_set_mfc_on(battery);
			}
			mutex_unlock(&battery->wclock);
			break;
		case POWER_SUPPLY_EXT_PROP_CURRENT_EVENT:
			if (!(battery->current_event & val->intval)) {
				pr_info("%s: set new current_event %d\n", __func__, val->intval);
				if (val->intval == SEC_BAT_CURRENT_EVENT_DC_ERR)
					battery->cisd.event_data[EVENT_DC_ERR]++;
				sec_bat_set_current_event(battery, val->intval, val->intval);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_CURRENT_EVENT_CLEAR:
			pr_info("%s: new current_event clear %d\n", __func__, val->intval);
			sec_bat_set_current_event(battery, 0, val->intval);
			break;
#if defined(CONFIG_WIRELESS_TX_MODE)
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_AVG_CURR:
			break;
#endif
		case POWER_SUPPLY_EXT_PROP_SRCCAP:
			pr_info("%s: set init_src_cap(%d->%d)",
				__func__, battery->init_src_cap, val->intval);
			battery->init_src_cap = true;
			break;
#if defined(CONFIG_DIRECT_CHARGING)
		case POWER_SUPPLY_EXT_PROP_DIRECT_TA_ALERT:
			if (battery->ta_alert_wa) {
				pr_info("@TA_ALERT: %s: TA OCP DETECT\n", __func__);
				battery->cisd.event_data[EVENT_TA_OCP_DET]++;
				if (battery->ta_alert_mode == OCP_NONE)
					battery->cisd.event_data[EVENT_TA_OCP_ON]++;
				battery->ta_alert_mode = OCP_DETECT;
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_25W_OCP,
							SEC_BAT_CURRENT_EVENT_25W_OCP);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_SEND_UVDM:
			if (is_pd_apdo_wire_type(battery->cable_type)) {
				pr_info("@SEND_UVDM: Request Change Charging Source : %s \n",
					val->intval == 0 ? "Switch Charger" : "Direct Charger" );
				sec_bat_set_current_event(battery, val->intval == 0 ?
					SEC_BAT_CURRENT_EVENT_SEND_UVDM : 0, SEC_BAT_CURRENT_EVENT_SEND_UVDM);
				value.intval = val->intval;
	            psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_CHANGE_CHARGING_SOURCE, value);
			}
			break;
#endif
		case POWER_SUPPLY_EXT_PROP_WPC_EN:
			sec_bat_set_current_event(battery,
				val->intval ? 0 : SEC_BAT_CURRENT_EVENT_WPC_EN, SEC_BAT_CURRENT_EVENT_WPC_EN);
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sec_bat_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	union power_supply_propval value = {0, };
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if ((battery->health == POWER_SUPPLY_HEALTH_OVERVOLTAGE) ||
			(battery->health == POWER_SUPPLY_HEALTH_UNDERVOLTAGE)) {
				val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		} else {
			if ((battery->pdata->cable_check_type &
				SEC_BATTERY_CABLE_CHECK_NOUSBCHARGE) &&
				!lpcharge) {
				switch (battery->cable_type) {
				case SEC_BATTERY_CABLE_USB:
				case SEC_BATTERY_CABLE_USB_CDP:
					val->intval =
						POWER_SUPPLY_STATUS_DISCHARGING;
					return 0;
				}
			}
#if defined(CONFIG_STORE_MODE)
			if (battery->store_mode && !lpcharge &&
			    !is_nocharge_type(battery->cable_type) &&
			    battery->status == POWER_SUPPLY_STATUS_DISCHARGING) {
				val->intval = POWER_SUPPLY_STATUS_CHARGING;
			} else
#endif
				val->intval = battery->status;
		}
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		if (is_nocharge_type(battery->cable_type)) {
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
		} else if (is_hv_wire_type(battery->cable_type) || is_pd_wire_type(battery->cable_type)) {
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
		} else if (battery->usb_slow_chg) {
			val->intval = POWER_SUPPLY_CHARGE_TYPE_SLOW;
			pr_info("%s: slow-charging mode\n", __func__);
		} else {
			psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_PROP_CHARGE_TYPE, value);
			if (value.intval == SEC_BATTERY_CABLE_UNKNOWN)
				/* if error in CHARGE_TYPE of charger
				 * set CHARGE_TYPE as NONE
				 */
				val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
			else
				val->intval = value.intval;
		}
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		if (lpcharge &&
			(battery->health == POWER_SUPPLY_HEALTH_OVERVOLTAGE ||
			battery->health == POWER_SUPPLY_HEALTH_UNDERVOLTAGE ||
			battery->health == POWER_SUPPLY_HEALTH_DC_ERR))
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
		else if (battery->health >= POWER_SUPPLY_HEALTH_MAX)
			val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
		else
			val->intval = battery->health;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = battery->present;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		if (is_hv_wireless_type(battery->cable_type) ||
			(battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV) ||
			(battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_20)) {
			if (sec_bat_hv_wc_normal_mode_check(battery))
				val->intval = SEC_BATTERY_CABLE_WIRELESS;
			else
				val->intval = SEC_BATTERY_CABLE_HV_WIRELESS_ETX;
		}
		else if(battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_PACK ||
			battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_STAND ||
			battery->cable_type == SEC_BATTERY_CABLE_PMA_WIRELESS ||
			battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_VEHICLE ||
			battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_TX ||
			battery->cable_type == SEC_BATTERY_CABLE_HV_WIRELESS_20 ||
			battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_FAKE)
			val->intval = SEC_BATTERY_CABLE_WIRELESS;
		else
			val->intval = battery->cable_type;
		pr_info("%s cable type = %d sleep_mode = %d\n", __func__, val->intval, sleep_mode);
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = battery->pdata->technology;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
#ifdef CONFIG_SEC_FACTORY
		psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
		battery->voltage_now = value.intval;
		dev_err(battery->dev,
			"%s: voltage now(%d)\n", __func__, battery->voltage_now);
#endif
		/* voltage value should be in uV */
		val->intval = battery->voltage_now * 1000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_AVG:
#ifdef CONFIG_SEC_FACTORY
		value.intval = SEC_BATTERY_VOLTAGE_AVERAGE;
		psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_VOLTAGE_AVG, value);
		battery->voltage_avg = value.intval;
		dev_err(battery->dev,
			"%s: voltage avg(%d)\n", __func__, battery->voltage_avg);
#endif
		/* voltage value should be in uV */
		val->intval = battery->voltage_avg * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		value.intval = SEC_BATTERY_CURRENT_MA;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CURRENT_NOW, value);
		val->intval = value.intval;
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		value.intval = SEC_BATTERY_CURRENT_MA;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CURRENT_AVG, value);
		val->intval = value.intval;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		val->intval = battery->pdata->battery_full_capacity * 1000;
		break;
	/* charging mode (differ from power supply) */
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		val->intval = battery->charging_mode;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		if (battery->pdata->fake_capacity) {
			val->intval = 90;
			pr_info("%s : capacity(%d)\n", __func__, val->intval);
		} else {
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
			if (battery->status == POWER_SUPPLY_STATUS_FULL) {
				if(battery->eng_not_full_status)
					val->intval = battery->capacity;
				else
					val->intval = 100;
			} else {
				val->intval = battery->capacity;
			}
#else
			if (battery->status == POWER_SUPPLY_STATUS_FULL)
				val->intval = 100;
			else
				val->intval = battery->capacity;
#endif
		}
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = battery->temperature;
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		val->intval = battery->temper_amb;
		break;
#if defined(CONFIG_FUELGAUGE_MAX77705)
	case POWER_SUPPLY_PROP_POWER_NOW:
		value.intval = SEC_BATTERY_ISYS_MA;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_MEASURE_SYS, value);
		val->intval = value.intval;
		break;
	case POWER_SUPPLY_PROP_POWER_AVG:
		value.intval = SEC_BATTERY_ISYS_AVG_MA;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_MEASURE_SYS, value);
		val->intval = value.intval;
		break;
#endif
#if defined(CONFIG_CALC_TIME_TO_FULL)
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		if (battery->capacity == 100) {
			val->intval = 0;
			break;
		}

		if (((battery->status == POWER_SUPPLY_STATUS_CHARGING) ||
			(battery->status == POWER_SUPPLY_STATUS_FULL && battery->capacity != 100)) &&
			!battery->swelling_mode)
			val->intval = battery->timetofull;
		else
			val->intval = 0;
		break;
#endif
#if defined(CONFIG_BATTERY_SWELLING)
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		if (battery->swelling_mode)
			val->intval = 1;
		else
			val->intval = 0;
		break;
#endif
	case POWER_SUPPLY_PROP_CHARGE_COUNTER_SHADOW:
		val->intval = battery->wire_status;
		break;
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
	case POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL:
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		val->intval = battery->charge_counter;
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_SUB_PBA_TEMP_REC:
			val->intval = !battery->vbus_limit;
			break;
		case POWER_SUPPLY_EXT_PROP_CHARGE_POWER:
			val->intval = battery->charge_power;
			break;
		case POWER_SUPPLY_EXT_PROP_CURRENT_EVENT:
			val->intval = battery->current_event;
			break;
#if defined(CONFIG_WIRELESS_TX_MODE)
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_AVG_CURR:
			val->intval = battery->tx_avg_curr;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ENABLE:
			val->intval = battery->wc_tx_enable;
			break;
#endif
#if defined(CONFIG_DIRECT_CHARGING)
#if defined(CONFIG_PDIC_PD30)
		case POWER_SUPPLY_EXT_PROP_DIRECT_FIXED_PDO:
			val->intval = battery->pd_list.num_apdo > 0 ? battery->pd_list.num_fpdo : 1;
			break;
#endif
		case POWER_SUPPLY_EXT_PROP_DIRECT_CHARGER_MODE:
			val->intval = battery->pd_list.now_isApdo;
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_HV_PDO:
			val->intval = battery->hv_pdo;
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_HAS_APDO:
			val->intval = battery->pdic_info.sink_status.has_apdo;
			break;
#endif
		case POWER_SUPPLY_EXT_PROP_PAD_VOLT_CTRL:
                  if (battery->wpc_vout_ctrl_lcd_on)
                    val->intval = battery->lcd_status;
                  else
                    val->intval = false;
			break;
#if defined(CONFIG_DIRECT_CHARGING)
		case POWER_SUPPLY_EXT_PROP_DIRECT_TA_ALERT:
			if (battery->ta_alert_wa) {
				val->intval = battery->ta_alert_mode;
			} else
				val->intval = OCP_NONE;
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_SEND_UVDM:
			break;
#endif
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sec_usb_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

	if (psp != POWER_SUPPLY_PROP_ONLINE)
		return -EINVAL;

	if ((battery->health == POWER_SUPPLY_HEALTH_OVERVOLTAGE) ||
		(battery->health == POWER_SUPPLY_HEALTH_UNDERVOLTAGE)) {
		val->intval = 0;
		return 0;
	}
	/* Set enable=1 only if the USB charger is connected */
	switch (battery->wire_status) {
	case SEC_BATTERY_CABLE_USB:
	case SEC_BATTERY_CABLE_USB_CDP:
		val->intval = 1;
		break;
	case SEC_BATTERY_CABLE_PDIC:
	case SEC_BATTERY_CABLE_NONE:
	        val->intval = (battery->pd_usb_attached) ? 1:0;
	        break;
	default:
		val->intval = 0;
		break;
	}

	if (is_slate_mode(battery))
		val->intval = 0;
	return 0;
}

static int sec_ac_get_property(struct power_supply *psy,
			       enum power_supply_property psp,
			       union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if ((battery->health == POWER_SUPPLY_HEALTH_OVERVOLTAGE) ||
				(battery->health == POWER_SUPPLY_HEALTH_UNDERVOLTAGE)) {
			val->intval = 0;
			return 0;
		}

		/* Set enable=1 only if the AC charger is connected */
		switch (battery->cable_type) {
		case SEC_BATTERY_CABLE_TA:
		case SEC_BATTERY_CABLE_UARTOFF:
		case SEC_BATTERY_CABLE_LAN_HUB:
		case SEC_BATTERY_CABLE_UNKNOWN:
		case SEC_BATTERY_CABLE_PREPARE_TA:
		case SEC_BATTERY_CABLE_9V_ERR:
		case SEC_BATTERY_CABLE_9V_UNKNOWN:
		case SEC_BATTERY_CABLE_9V_TA:
		case SEC_BATTERY_CABLE_12V_TA:
		case SEC_BATTERY_CABLE_HMT_CONNECTED:
		case SEC_BATTERY_CABLE_HMT_CHARGE:
		case SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT:
		case SEC_BATTERY_CABLE_QC20:
		case SEC_BATTERY_CABLE_QC30:
		case SEC_BATTERY_CABLE_TIMEOUT:
		case SEC_BATTERY_CABLE_SMART_OTG:
		case SEC_BATTERY_CABLE_SMART_NOTG:
#if defined(CONFIG_PDIC_PD30)
		case SEC_BATTERY_CABLE_PDIC_APDO:
#endif
			val->intval = 1;
			break;
		case SEC_BATTERY_CABLE_PDIC:
			val->intval = (battery->pd_usb_attached) ? 0:1;
			break;
		default:
			val->intval = 0;
			break;
		}
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = battery->chg_temp;
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
			case POWER_SUPPLY_EXT_PROP_WATER_DETECT:
				if (battery->misc_event & (BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE |
					BATT_MISC_EVENT_HICCUP_TYPE)) {
					val->intval = 1;
					pr_info("%s: Water Detect\n", __func__);
				} else {
					val->intval = 0;
				}
				break;
			default:
				return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	if (lpcharge && (battery->misc_event & BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE)) {
		val->intval = 1;
	}

	return 0;
}

static int sec_wireless_get_property(struct power_supply *psy,
			       enum power_supply_property psp,
			       union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = (is_wireless_type(battery->cable_type) ||
		battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_FAKE) ?
			1 : 0;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = (battery->pdata->wireless_charger_name) ?
			1 : 0;
		break;
	case POWER_SUPPLY_PROP_POWER_DESIGN:
		if (battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_20)
			val->intval = battery->wc20_power_class;
		else
			val->intval = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void sec_bat_wpc_tx_en_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, wpc_tx_en_work.work);

	union power_supply_propval value = {0, };

	pr_info("@Tx_Mode %s: tx %s\n", __func__,
		battery->wc_tx_enable ? "on" : "off");

	battery->tx_minduty = battery->pdata->tx_minduty_default;
	battery->tx_switch_mode = TX_SWITCH_MODE_OFF;
	battery->tx_switch_start_soc = 0;
	battery->tx_switch_mode_change = false;

	if (battery->wc_tx_enable) {
		/* set tx event */
		sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_STATUS,
			(BATT_TX_EVENT_WIRELESS_TX_STATUS | BATT_TX_EVENT_WIRELESS_TX_RETRY));

#if defined(CONFIG_DIRECT_CHARGING)
		if (is_pd_apdo_wire_type(battery->wire_status) && battery->pd_list.now_isApdo) {
			pr_info("@Tx_Mode %s: PD30 source charnge (APDO -> Fixed). Because Tx Start.\n", __func__);
			sec_bat_set_charge(battery, battery->charger_mode);
		} else if (is_hv_wire_type(battery->wire_status)) {
#else
		if (is_hv_wire_type(battery->wire_status)) {
#endif
			muic_afc_set_voltage(SEC_INPUT_VOLTAGE_5V/10);
		} else if (is_pd_wire_type(battery->wire_status) && battery->hv_pdo) {
			pr_info("@Tx_Mode %s: PD charnge pdo (9V -> 5V). Because Tx Start.\n", __func__);
			sec_bat_change_pdo(battery, SEC_INPUT_VOLTAGE_5V);
		} else {
			battery->buck_cntl_by_tx = true;
			sec_bat_wireless_uno_cntl(battery, true);

			sec_bat_wireless_vout_cntl(battery, WC_TX_VOUT_5_0V);
			sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_gear);
		}

#if defined(CONFIG_WIRELESS_TX_MODE)
		pr_info("@Tx_Mode %s: TX Power Calculation start.\n", __func__);
		queue_delayed_work(battery->monitor_wqueue,
				&battery->wpc_txpower_calc_work, 0);
#endif
	} else {
		battery->uno_en = false;
		sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_default);
		value.intval = false;
		battery->wc_rx_type = NO_DEV;
		battery->wc_rx_connected = false;

		battery->tx_uno_iout = 0;
		battery->tx_mfc_iout = 0;

		if (battery->afc_disable) {
			battery->afc_disable = false;
			muic_hv_charger_disable(battery->afc_disable);
		}

#if defined(CONFIG_DIRECT_CHARGING)
		if (is_pd_apdo_wire_type(battery->cable_type) || battery->buck_cntl_by_tx) {
#else
		if (battery->buck_cntl_by_tx) {
#endif
			battery->buck_cntl_by_tx = false;
			sec_bat_set_charge(battery, battery->charger_mode);
		}

		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ENABLE, value);

		battery->wc_tx_vout = WC_TX_VOUT_5_0V;

		if (is_hv_wire_type(battery->cable_type)) {
			muic_afc_set_voltage(SEC_INPUT_VOLTAGE_9V/10);
		/* for 1) not supporting DC and charging bia PD20/DC on Tx
		       2) supporting DC and charging bia PD20 on Tx */
		} else if (is_pd_fpdo_wire_type(battery->cable_type) && !battery->hv_pdo) {
			sec_bat_change_pdo(battery, SEC_INPUT_VOLTAGE_9V);
		}

		cancel_delayed_work(&battery->wpc_tx_work);
#if defined(CONFIG_WIRELESS_TX_MODE)
		cancel_delayed_work(&battery->wpc_txpower_calc_work);
#endif
		wake_unlock(&battery->wpc_tx_wake_lock);
	}

	pr_info("@Tx_Mode %s Done\n", __func__);
	wake_unlock(&battery->wpc_tx_en_wake_lock);
}

void sec_wireless_set_tx_enable(struct sec_battery_info *battery, bool wc_tx_enable)
{
	pr_info("@Tx_Mode %s: TX Power enable ? (%d)\n", __func__, wc_tx_enable);

	battery->wc_tx_enable = wc_tx_enable;

	cancel_delayed_work(&battery->wpc_tx_en_work);
	wake_lock(&battery->wpc_tx_en_wake_lock);
	queue_delayed_work(battery->monitor_wqueue,
		&battery->wpc_tx_en_work, 0);
}

static void sec_wireless_otg_control(struct sec_battery_info *battery, int enable)
{
	union power_supply_propval value = {0, };

	if (enable) {
		sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK,
			SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK);
	} else {
		sec_bat_set_current_event(battery, 0,
			SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK);
	}

	value.intval = enable;
	psy_do_property(battery->pdata->wireless_charger_name, set,
		POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, value);

	if (is_hv_wireless_type(battery->cable_type)) {
		int cnt;

		mutex_lock(&battery->voutlock);
		value.intval = (enable) ? WIRELESS_VOUT_5V :
			battery->wpc_vout_level;
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
		battery->aicl_current = 0; /* reset aicl current */
		mutex_unlock(&battery->voutlock);

		for (cnt = 0; cnt < 5; cnt++) {
			msleep(100);
			psy_do_property(battery->pdata->wireless_charger_name, get,
				POWER_SUPPLY_PROP_ENERGY_NOW, value);
			if (value.intval <= 6000) {
				pr_info("%s: wireless vout goes to 5V Vout(%d).\n",
					__func__, value.intval);
				break;
			}
		}
	} else if (is_nv_wireless_type(battery->cable_type)) {
		union power_supply_propval value = {0, };
		if (enable) {
			pr_info("%s: wireless 5V with OTG\n", __func__);
			value.intval = WIRELESS_VOUT_5V;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
		} else {
			pr_info("%s: wireless 5.5V without OTG\n", __func__);
			value.intval = WIRELESS_VOUT_CC_CV_VOUT;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
		}
	} else if(battery->wc_tx_enable && enable) {
		/* TX power should turn off during otg on */
		pr_info("@Tx_Mode %s: OTG is going to work, TX power should off\n", __func__);
		/* set tx event */
		sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_OTG_ON, BATT_TX_EVENT_WIRELESS_TX_OTG_ON);
		sec_wireless_set_tx_enable(battery, false);
	}
}

static int sec_wireless_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
#if defined(CONFIG_BATTERY_CISD)
		if (val->intval != SEC_WIRELESS_PAD_NONE && battery->wc_status == SEC_WIRELESS_PAD_NONE) {
			battery->cisd.data[CISD_DATA_WIRELESS_COUNT]++;
			battery->cisd.data[CISD_DATA_WIRELESS_COUNT_PER_DAY]++;
		}
#endif
		pr_info("%s : wireless_type(0x%x)\n", __func__, val->intval);

		/* Clear the FOD , AUTH State */
		sec_bat_set_misc_event(battery, 0, BATT_MISC_EVENT_WIRELESS_FOD);

		battery->wc_status = val->intval;

		if ((battery->ext_event & BATT_EXT_EVENT_CALL) &&
			(battery->wc_status == SEC_WIRELESS_PAD_WPC_PACK ||
			battery->wc_status == SEC_WIRELESS_PAD_WPC_PACK_HV ||
			battery->wc_status == SEC_WIRELESS_PAD_TX)) {
				battery->wc_rx_phm_mode = true;
		}

		if (battery->wc_status == SEC_WIRELESS_PAD_NONE) {
			battery->wpc_vout_level = WIRELESS_VOUT_10V;
			battery->wpc_max_vout_level = WIRELESS_VOUT_12_5V;
			battery->auto_mode = false;

			if (delayed_work_pending(&battery->wc_headroom_work)) {
				wake_unlock(&battery->wc_headroom_wake_lock);
				cancel_delayed_work(&battery->wc_headroom_work);
			}

			sec_bat_set_misc_event(battery, 0,
			(BATT_MISC_EVENT_WIRELESS_DET_LEVEL | /* clear wpc_det level status */
			BATT_MISC_EVENT_WIRELESS_AUTH_START |
			BATT_MISC_EVENT_WIRELESS_AUTH_RECVED |
			BATT_MISC_EVENT_WIRELESS_AUTH_FAIL |
			BATT_MISC_EVENT_WIRELESS_AUTH_PASS));
		} else if (battery->wc_status != SEC_WIRELESS_PAD_FAKE) {
			sec_bat_set_misc_event(battery, BATT_MISC_EVENT_WIRELESS_DET_LEVEL, /* set wpc_det level status */
			BATT_MISC_EVENT_WIRELESS_DET_LEVEL);
		
			if (battery->wc_status == SEC_WIRELESS_PAD_WPC_HV_20) {
				sec_bat_set_misc_event(battery, BATT_MISC_EVENT_WIRELESS_AUTH_PASS,
					BATT_MISC_EVENT_WIRELESS_AUTH_PASS);
#if defined(CONFIG_BATTERY_CISD)
				if (battery->wc_status == SEC_WIRELESS_PAD_WPC_HV_20)
					battery->cisd.cable_data[CISD_CABLE_HV_WC_20]++;
#endif
			}
		}

		wake_lock(&battery->cable_wake_lock);
		queue_delayed_work(battery->monitor_wqueue,
			&battery->cable_work, 0);
		if (battery->wc_status == SEC_WIRELESS_PAD_NONE ||
			battery->wc_status == SEC_WIRELESS_PAD_WPC_PACK ||
			battery->wc_status == SEC_WIRELESS_PAD_WPC_PACK_HV ||
			battery->wc_status == SEC_WIRELESS_PAD_VEHICLE) {
			sec_bat_set_misc_event(battery,
				(battery->wc_status == SEC_WIRELESS_PAD_NONE ? 0 : BATT_MISC_EVENT_WIRELESS_BACKPACK_TYPE),
				BATT_MISC_EVENT_WIRELESS_BACKPACK_TYPE);
		}
		break;
	case POWER_SUPPLY_PROP_AUTHENTIC:
#if defined(CONFIG_BATTERY_CISD)
		pr_info("%s : tx_type(0x%x)\n", __func__, val->intval);
		count_cisd_pad_data(&battery->cisd, val->intval);
#endif
		break;
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
		sec_wireless_otg_control(battery, val->intval);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		battery->aicl_current = 0; /* reset aicl current */
		pr_info("%s: reset aicl\n", __func__);
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ENABLE:
			sec_wireless_set_tx_enable(battery, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_ERR:
			if (is_wireless_type(battery->cable_type))
				sec_bat_set_misc_event(battery, val->intval ? BATT_MISC_EVENT_WIRELESS_FOD : 0,
					BATT_MISC_EVENT_WIRELESS_FOD);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ERR:
			if (val->intval & BATT_TX_EVENT_WIRELESS_TX_MISALIGN) {
				sec_bat_handle_tx_misalign(battery, true);
			} else if (val->intval & BATT_TX_EVENT_WIRELESS_TX_OCP) {
				sec_bat_handle_tx_ocp(battery, true);
			} else {
				sec_bat_set_tx_event(battery, val->intval, val->intval);
				sec_wireless_set_tx_enable(battery, false);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONNECTED:
			sec_bat_set_tx_event(battery, val->intval ? BATT_TX_EVENT_WIRELESS_RX_CONNECT : 0,
				BATT_TX_EVENT_WIRELESS_RX_CONNECT);
			battery->wc_rx_connected = val->intval;
			if(!val->intval) {
				battery->wc_rx_type = NO_DEV;

				battery->tx_switch_mode = TX_SWITCH_MODE_OFF;
				battery->tx_switch_mode_change = false;	
				battery->tx_switch_start_soc = 0;

				if (battery->afc_disable) {
					battery->afc_disable = false;
					muic_hv_charger_disable(battery->afc_disable);
				}
				if (battery->wc_tx_enable) {
					pr_info("@Tx_Mode %s: Device detached.\n", __func__);

					if (is_hv_wire_type(battery->wire_status)) {
						pr_info("@Tx_Mode %s : charging voltage change(9V -> 5V).\n", __func__);
						muic_afc_set_voltage(SEC_INPUT_VOLTAGE_5V/10);
						break; /* do not set buck off/uno off untill vbus level get real 5V */
					} else if (is_pd_wire_type(battery->wire_status) && battery->hv_pdo) {
						pr_info("@Tx_Mode %s: PD charnge pdo (9V -> 5V). Because Tx Start.\n", __func__);
						sec_bat_change_pdo(battery, SEC_INPUT_VOLTAGE_5V);
						break; /* do not set buck off/uno off untill vbus level get real 5V */
					}

					if (!battery->buck_cntl_by_tx) {
						battery->buck_cntl_by_tx = true;
						sec_bat_set_charge(battery, battery->charger_mode);
					}

					sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_gear);
					sec_bat_wireless_vout_cntl(battery, WC_TX_VOUT_5_0V);
					sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_default);
				}
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_TYPE:
			battery->wc_rx_type = val->intval;
#if defined(CONFIG_BATTERY_CISD)
			if (battery->wc_rx_type) {
				if (battery->wc_rx_type == SS_BUDS) {
					battery->cisd.tx_data[SS_PHONE]--;
				}
				battery->cisd.tx_data[battery->wc_rx_type]++;
			}
#endif
			cancel_delayed_work(&battery->wpc_tx_work);
			wake_lock(&battery->wpc_tx_wake_lock);
			queue_delayed_work(battery->monitor_wqueue,
					&battery->wpc_tx_work, 0);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS:
			if(val->intval == WIRELESS_AUTH_START)
				sec_bat_set_misc_event(battery, BATT_MISC_EVENT_WIRELESS_AUTH_START, BATT_MISC_EVENT_WIRELESS_AUTH_START);
			else if(val->intval == WIRELESS_AUTH_RECEIVED)
				sec_bat_set_misc_event(battery, BATT_MISC_EVENT_WIRELESS_AUTH_RECVED, BATT_MISC_EVENT_WIRELESS_AUTH_RECVED);
			else if(val->intval == WIRELESS_AUTH_SENT) /* need to be clear this value when data is sent */
				sec_bat_set_misc_event(battery, 0, BATT_MISC_EVENT_WIRELESS_AUTH_START | BATT_MISC_EVENT_WIRELESS_AUTH_RECVED);
			else if(val->intval == WIRELESS_AUTH_FAIL)
				sec_bat_set_misc_event(battery, BATT_MISC_EVENT_WIRELESS_AUTH_FAIL, BATT_MISC_EVENT_WIRELESS_AUTH_FAIL);
			break;
		case POWER_SUPPLY_EXT_PROP_CALL_EVENT:
			if(val->intval == 1) {
				pr_info("%s : PHM enabled\n",__func__);
				battery->wc_rx_phm_mode = true;
			}
			break;
		case POWER_SUPPLY_PROP_WIRELESS_RX_POWER:
			pr_info("%s : rx power %d\n", __func__, val->intval);
			sec_bat_set_wireless20_current(battery, val->intval);
#if defined(CONFIG_CALC_TIME_TO_FULL)
			sec_bat_predict_wireless20_time_to_full_current(battery, val->intval);
#endif
			break;
		case POWER_SUPPLY_PROP_WIRELESS_MAX_VOUT:
			pr_info("%s : max vout %s\n", __func__, vout_control_mode_str[val->intval]);
			battery->wpc_max_vout_level = val->intval;
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sec_ps_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	union power_supply_propval value;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (val->intval == 0 && battery->ps_enable == true) {
			battery->ps_enable = false;
			value.intval = val->intval;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, value);
		} else if ((val->intval == 1) && (battery->ps_enable == false) &&
				(battery->ps_status == true)) {
			battery->ps_enable = true;
			value.intval = val->intval;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, value);
		} else {
			dev_err(battery->dev,
				"%s: invalid setting (%d)\n", __func__, val->intval);
		}
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		if (val->intval == SEC_BATTERY_CABLE_POWER_SHARING) {
			battery->ps_status = true;
			battery->ps_enable = true;
			value.intval = battery->ps_enable;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, value);
		} else {
			battery->ps_status = false;
			battery->ps_enable = false;
			value.intval = battery->ps_enable;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, value);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sec_ps_get_property(struct power_supply *psy,
			       enum power_supply_property psp,
			       union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = (battery->ps_enable) ? 1 : 0;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = (battery->ps_status) ? 1 : 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER) || defined(CONFIG_MUIC_NOTIFIER)
static int sec_bat_cable_check(struct sec_battery_info *battery,
				muic_attached_dev_t attached_dev)
{
	int current_cable_type = -1;
	union power_supply_propval val = {0, };

	pr_info("[%s]ATTACHED(%d)\n", __func__, attached_dev);

	switch (attached_dev)
	{
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
		battery->is_jig_on = true;
#if defined(CONFIG_BATTERY_CISD)
		battery->skip_cisd = true;
#endif
		current_cable_type = SEC_BATTERY_CABLE_NONE;
		break;
	case ATTACHED_DEV_SMARTDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_NONE;
		break;
	case ATTACHED_DEV_UNDEFINED_CHARGING_MUIC:
	case ATTACHED_DEV_UNDEFINED_RANGE_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_NONE;
		break;
	case ATTACHED_DEV_HICCUP_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_NONE;
		break;
	case ATTACHED_DEV_OTG_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_OTG_MUIC:
	case ATTACHED_DEV_HMT_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_OTG;
		break;
	case ATTACHED_DEV_TIMEOUT_OPEN_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_TIMEOUT;
		break;
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_SMARTDOCK_USB_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_USB_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_USB;
		break;
	case ATTACHED_DEV_JIG_UART_ON_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_FG_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_UARTOFF;
		break;
	case ATTACHED_DEV_RDU_TA_MUIC:
		battery->store_mode = true;
		wake_lock(&battery->parse_mode_dt_wake_lock);
		queue_delayed_work(battery->monitor_wqueue, &battery->parse_mode_dt_work, 0);
		current_cable_type = SEC_BATTERY_CABLE_TA;
		break;
	case ATTACHED_DEV_TA_MUIC:
	case ATTACHED_DEV_CARDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_VB_MUIC:
	case ATTACHED_DEV_SMARTDOCK_TA_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_TA_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_TA_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_ANY_MUIC:
	case ATTACHED_DEV_UNSUPPORTED_ID_VB_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_DISABLED_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_TA;
		break;
	case ATTACHED_DEV_AFC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_QC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_5V_DUPLI_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT;
		break;
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_CDP_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_USB_CDP;
		break;
	case ATTACHED_DEV_USB_LANHUB_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_LAN_HUB;
		break;
	case ATTACHED_DEV_CHARGING_CABLE_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_POWER_SHARING;
		break;
	case ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC:
	case ATTACHED_DEV_QC_CHARGER_PREPARE_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_PREPARE_TA;
		break;
	case ATTACHED_DEV_QC_CHARGER_9V_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_9V_TA;
		if ((battery->cable_type == SEC_BATTERY_CABLE_TA) ||
				(battery->cable_type == SEC_BATTERY_CABLE_NONE))
			battery->cisd.cable_data[CISD_CABLE_QC]++;
		break;
	case ATTACHED_DEV_AFC_CHARGER_9V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_9V_DUPLI_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_9V_TA;
		if ((battery->cable_type == SEC_BATTERY_CABLE_TA) ||
				(battery->cable_type == SEC_BATTERY_CABLE_NONE))
			battery->cisd.cable_data[CISD_CABLE_AFC]++;
		break;
#if defined(CONFIG_MUIC_HV_12V)
	case ATTACHED_DEV_AFC_CHARGER_12V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_12V_DUPLI_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_12V_TA;
		break;
#endif
	case ATTACHED_DEV_AFC_CHARGER_ERR_V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_ERR_V_DUPLI_MUIC:
		battery->cisd.cable_data[CISD_CABLE_AFC_FAIL]++;
		break;
	case ATTACHED_DEV_QC_CHARGER_ERR_V_MUIC:
		battery->cisd.cable_data[CISD_CABLE_QC_FAIL]++;
		break;
	case ATTACHED_DEV_HV_ID_ERR_UNDEFINED_MUIC:
	case ATTACHED_DEV_HV_ID_ERR_UNSUPPORTED_MUIC:
	case ATTACHED_DEV_HV_ID_ERR_SUPPORTED_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_9V_UNKNOWN;
		break;
	case ATTACHED_DEV_VZW_INCOMPATIBLE_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_UNKNOWN;
		break;
	default:
		pr_err("%s: invalid type for charger:%d\n",
			__func__, attached_dev);
		break;
	}

	if (battery->is_jig_on && !battery->pdata->support_fgsrc_change)
		psy_do_property(battery->pdata->fuelgauge_name, set,
			POWER_SUPPLY_PROP_ENERGY_NOW, val);

	return current_cable_type;
}
#endif

#if defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
#if defined(CONFIG_CCIC_NOTIFIER)
static int sec_bat_get_pd_list_index(PDIC_SINK_STATUS *sink_status, struct sec_bat_pdic_list *pd_list)
{
	int i = 0;

	for (i = 0; i < pd_list->max_pd_count; i++) {
		if (pd_list->pd_info[i].pdo_index == sink_status->current_pdo_num)
			return i;
	}

	return 0;
}

static void sec_bat_set_rp_current(struct sec_battery_info *battery, int cable_type)
{
	if (battery->pdic_info.sink_status.rp_currentlvl == RP_CURRENT_ABNORMAL) {
		sec_bat_change_default_current(battery, cable_type,
			battery->pdata->rp_current_abnormal_rp3, battery->pdata->rp_current_abnormal_rp3);
	} else if (battery->pdic_info.sink_status.rp_currentlvl == RP_CURRENT_LEVEL3) {
		if (battery->current_event & SEC_BAT_CURRENT_EVENT_HV_DISABLE)
			sec_bat_change_default_current(battery, cable_type,
				battery->pdata->default_input_current, battery->pdata->default_charging_current);
		else {
			if(battery->store_mode)
				sec_bat_change_default_current(battery, cable_type,
					battery->pdata->rp_current_rdu_rp3, battery->pdata->max_charging_current);
			else
				sec_bat_change_default_current(battery, cable_type,
					battery->pdata->rp_current_rp3, battery->pdata->max_charging_current);
		}
	} else if (battery->pdic_info.sink_status.rp_currentlvl == RP_CURRENT_LEVEL2) {
		sec_bat_change_default_current(battery, cable_type,
			battery->pdata->rp_current_rp2, battery->pdata->rp_current_rp2);
	} else if (battery->pdic_info.sink_status.rp_currentlvl == RP_CURRENT_LEVEL_DEFAULT) {
		if (cable_type == SEC_BATTERY_CABLE_USB) {
			if (battery->current_event & SEC_BAT_CURRENT_EVENT_USB_SUPER)
				sec_bat_change_default_current(battery, SEC_BATTERY_CABLE_USB,
					USB_CURRENT_SUPER_SPEED, USB_CURRENT_SUPER_SPEED);
			else
				sec_bat_change_default_current(battery, cable_type,
					battery->pdata->default_usb_input_current,
					battery->pdata->default_usb_charging_current);
		} else if (cable_type == SEC_BATTERY_CABLE_TA) {
			sec_bat_change_default_current(battery, cable_type,
				battery->pdata->default_input_current,
				battery->pdata->default_charging_current);
		}
	}
	battery->aicl_current = 0;

	pr_info("%s:(%d)\n", __func__, battery->pdic_info.sink_status.rp_currentlvl);
	battery->max_charge_power = 0;
	if (battery->status != POWER_SUPPLY_STATUS_DISCHARGING)
		sec_bat_check_input_voltage(battery);
	/* prevent TA ocp */
	if(!is_hv_wireless_type(battery->cable_type) &&
		battery->cable_type != SEC_BATTERY_CABLE_PREPARE_WIRELESS_20)
		sec_bat_set_charging_current(battery);
}
#endif

static int make_pd_list(struct sec_battery_info *battery)
{
	int i = 0;
	int base_charge_power = 0, selected_pdo_voltage = 0, selected_pdo_power = 0, selected_pdo_num = 0;
	int pd_list_index = 0, temp_power = 0, num_pd_list = 0, pd_list_select = 0;
	int pd_charging_charge_power = battery->current_event & SEC_BAT_CURRENT_EVENT_HV_DISABLE ?
		battery->pdata->nv_charge_power : battery->pdata->pd_charging_charge_power;
	union power_supply_propval value = {0, };
	POWER_LIST* pPower_list;

	/* If PD charger is attached first, current_pdo_num should be 1 supports 5V */
#if defined(CONFIG_PDIC_PD30)
	battery->pd_list.pd_info[0].max_voltage =
		battery->pdic_info.sink_status.power_list[1].max_voltage;
	battery->pd_list.pd_info[0].max_current =
		battery->pdic_info.sink_status.power_list[1].max_current;
#else
	battery->pd_list.pd_info[0].input_voltage =
		battery->pdic_info.sink_status.power_list[1].max_voltage;
	battery->pd_list.pd_info[0].input_current =
		battery->pdic_info.sink_status.power_list[1].max_current;
#endif
	battery->pd_list.pd_info[0].pdo_index = 1;
	pd_list_index++;

	base_charge_power =
		battery->pdic_info.sink_status.power_list[1].max_voltage * battery->pdic_info.sink_status.power_list[1].max_current;

	selected_pdo_voltage = SEC_INPUT_VOLTAGE_5V * 100;
	selected_pdo_power = 0;
	selected_pdo_num = 0;

	for (i = 1; i <= battery->pdic_info.sink_status.available_pdo_num; i++)
	{
		pPower_list = &battery->pdic_info.sink_status.power_list[i];
#if defined(CONFIG_PDIC_PD30)
		if (!pPower_list->accept || pPower_list->apdo) /* skip not accept of apdo list */
			continue;
#endif
		temp_power = pPower_list->max_voltage * pPower_list->max_current;

		if ((temp_power >= base_charge_power - 1000000) && (temp_power <= pd_charging_charge_power * 1000))
		{
			if (temp_power >= selected_pdo_power &&
				pPower_list->max_voltage > selected_pdo_voltage && pPower_list->max_voltage <= battery->pdata->max_input_voltage)
			{
				selected_pdo_voltage = pPower_list->max_voltage;
				selected_pdo_power = temp_power;
				selected_pdo_num = i;
			}
		}
	}
	if (selected_pdo_num)
	{
		POWER_LIST* pSelected_power_list =
			&battery->pdic_info.sink_status.power_list[selected_pdo_num];

		battery->pd_list.pd_info[pd_list_index].pdo_index = selected_pdo_num;
#if defined(CONFIG_PDIC_PD30)
		battery->pd_list.pd_info[pd_list_index].apdo = false;
		battery->pd_list.pd_info[pd_list_index].max_voltage = pSelected_power_list->max_voltage;
		battery->pd_list.pd_info[pd_list_index].max_current = pSelected_power_list->max_current;
		battery->pd_list.pd_info[pd_list_index].min_voltage = 0;
#else
		battery->pd_list.pd_info[pd_list_index].input_voltage = pSelected_power_list->max_voltage;
		battery->pd_list.pd_info[pd_list_index].input_current = pSelected_power_list->max_current;
#endif
		pd_list_index++;
	}

#if defined(CONFIG_PDIC_PD30)
	battery->pd_list.num_fpdo = pd_list_index;

	if (battery->pdic_info.sink_status.has_apdo) {
		/* unconditionally add APDO list */
		for (i = 1; i <= battery->pdic_info.sink_status.available_pdo_num; i++)
		{
			pPower_list = &battery->pdic_info.sink_status.power_list[i];

			if (pPower_list->apdo && pd_list_index >= 0 && pd_list_index < MAX_PDO_NUM) {
				battery->pd_list.pd_info[pd_list_index].pdo_index = i;
				battery->pd_list.pd_info[pd_list_index].apdo = true;
				battery->pd_list.pd_info[pd_list_index].max_voltage = pPower_list->max_voltage;
				battery->pd_list.pd_info[pd_list_index].min_voltage = pPower_list->min_voltage;
				battery->pd_list.pd_info[pd_list_index].max_current = pPower_list->max_current;

				pd_list_index++;
			}
		}
		battery->pd_list.num_apdo = pd_list_index - battery->pd_list.num_fpdo;
	} else {
		/* battery->pdic_info.sink_status has no apdo */
		battery->pd_list.num_apdo = 0;
	}
#endif

	num_pd_list = pd_list_index;

	if (num_pd_list <= 0 || num_pd_list > MAX_PDO_NUM) {
		pr_info("%s : PDO list is wrong: %d\n", __func__, num_pd_list);
		return 0;
	} else {
#if defined(CONFIG_PDIC_PD30)
		pr_info("%s: total num_pd_list: %d, num_fpdo: %d, num_apdo: %d\n",
			__func__, num_pd_list, battery->pd_list.num_fpdo, battery->pd_list.num_apdo);
#else
		pr_info("%s: total num_pd_list: %d\n", __func__, num_pd_list);
#endif
	}

#if defined(CONFIG_PDIC_PD30)
	if (battery->pdic_info.sink_status.has_apdo) {
		for (i = 0; i < battery->pd_list.num_fpdo - 1; i++) {
			/* select pdo 1, if pd have apdo */
			if (battery->pd_list.pd_info[i].pdo_index == 1) {
				pd_list_select = i;
				break;
			}
		}
	} else {
		pd_list_select = num_pd_list - battery->pd_list.num_apdo - 1;
	}
#else
	pd_list_select = num_pd_list - 1;
#endif

	if (pd_list_select < 0 || pd_list_select >= MAX_PDO_NUM) {
		pr_info("%s: pd_list_select is wrong: %d\n", __func__, pd_list_select);
		return 0;
	}

	for (i = 0; i < num_pd_list; i++) {
#if defined(CONFIG_PDIC_PD30)
		pr_info("%s: Made pd_list[%d] %s[%d,%s] maxVol:%d, minVol:%d, maxCur:%d\n",
			__func__, i, i == pd_list_select ? "**" : " ",
			battery->pd_list.pd_info[i].pdo_index,
			battery->pd_list.pd_info[i].apdo ? "APDO" : "FIXED",
			battery->pd_list.pd_info[i].max_voltage,
			battery->pd_list.pd_info[i].min_voltage,
			battery->pd_list.pd_info[i].max_current);
#else
		pr_info("%s: Made pd_list[%d] %s[%d] voltage : %d, current : %d\n",
			__func__, i, i == pd_list_select ? "**" : " ",
			battery->pd_list.pd_info[i].pdo_index,
			battery->pd_list.pd_info[i].input_voltage,
			battery->pd_list.pd_info[i].input_current);
#endif
	}

	battery->pd_list.max_pd_count = num_pd_list;

#if defined(CONFIG_PDIC_PD30)
	if (!battery->pdic_info.sink_status.has_apdo ||
		battery->current_event & SEC_BAT_CURRENT_EVENT_HV_DISABLE) {
		battery->max_charge_power = battery->pdic_info.sink_status.power_list[ \
			battery->pd_list.pd_info[pd_list_select].pdo_index].max_voltage * \
			battery->pdic_info.sink_status.power_list[battery->pd_list.pd_info[ \
			pd_list_select].pdo_index].max_current / 1000;
		battery->pd_max_charge_power = battery->max_charge_power;
	}
#else
	battery->max_charge_power = battery->pdic_info.sink_status.power_list[ \
		battery->pd_list.pd_info[pd_list_select].pdo_index].max_voltage * \
		battery->pdic_info.sink_status.power_list[battery->pd_list.pd_info[ \
		pd_list_select].pdo_index].max_current / 1000;
	battery->pd_max_charge_power = battery->max_charge_power;
#endif

	if (battery->cable_type == SEC_BATTERY_CABLE_NONE) {
		if (battery->pd_max_charge_power > 12000)
			battery->cisd.cable_data[CISD_CABLE_PD_HIGH]++;
		else
			battery->cisd.cable_data[CISD_CABLE_PD]++;
	}

	if (battery->pdic_info.sink_status.selected_pdo_num == battery->pd_list.pd_info[pd_list_select].pdo_index) {
		battery->pdic_ps_rdy = true;
		dev_info(battery->dev, "%s: battery->pdic_ps_rdy(%d)\n", __func__, battery->pdic_ps_rdy);
	} else if (battery->wc_rx_type != SS_GEAR) {
		/* change input current before request new pdo if new pdo's input current is less than now */
#if defined(CONFIG_PDIC_PD30)
		if (battery->pd_list.pd_info[pd_list_select].max_current < battery->input_current) {
			int input_current = battery->pd_list.pd_info[pd_list_select].max_current;
#else
		if (battery->pd_list.pd_info[pd_list_select].input_current < battery->input_current) {
			int input_current = battery->pd_list.pd_info[pd_list_select].input_current;
#endif

			value.intval = input_current;
			battery->input_current = input_current;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_CURRENT_MAX, value);
		}
		battery->pdic_ps_rdy = false;
		sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SELECT_PDO,
			SEC_BAT_CURRENT_EVENT_SELECT_PDO);
		if (pd_list_select >= 0 && pd_list_select < MAX_PDO_NUM)
			select_pdo(battery->pd_list.pd_info[pd_list_select].pdo_index);
	}

	battery->pd_list.now_pd_index = sec_bat_get_pd_list_index(&battery->pdic_info.sink_status,
		&battery->pd_list);
	pr_info("%s : now_pd_index : %d\n", __func__, battery->pd_list.now_pd_index);

#if defined(CONFIG_DIRECT_CHARGING) && defined(CONFIG_PDIC_PD30)
	value.intval = battery->pd_list.num_apdo > 0 ? battery->pd_list.num_fpdo : 1;
	psy_do_property(battery->pdata->charger_name, set,
		POWER_SUPPLY_EXT_PROP_DIRECT_FIXED_PDO, value);
#endif

	return battery->pd_list.max_pd_count;
}

static int usb_typec_handle_notification(struct notifier_block *nb,
		unsigned long action, void *data)
{
	const char *cmd = "NONE";
	struct sec_battery_info *battery =
			container_of(nb, struct sec_battery_info, usb_typec_nb);
	int cable_type = SEC_BATTERY_CABLE_NONE, i = 0, current_pdo = 0;
	int pd_charging_charge_power = battery->current_event & SEC_BAT_CURRENT_EVENT_HV_DISABLE ?
		battery->pdata->nv_charge_power : battery->pdata->pd_charging_charge_power;
	CC_NOTI_ATTACH_TYPEDEF usb_typec_info = *(CC_NOTI_ATTACH_TYPEDEF *)data;
	bool bPdIndexChanged = false;
	int max_power = 0;
#if defined(CONFIG_PDIC_PD30)
	bool bPrintPDlog = true;
	int fpdo_power = 0;
#if defined(CONFIG_DIRECT_CHARGING)
	union power_supply_propval val = {0, };
#endif
#endif

	dev_info(battery->dev, "%s: action (%ld) dump(0x%01x, 0x%01x, 0x%02x, 0x%04x, 0x%04x, 0x%04x)\n",
		__func__, action, usb_typec_info.src, usb_typec_info.dest, usb_typec_info.id,
		usb_typec_info.attach, usb_typec_info.rprd, usb_typec_info.cable_type);

	if (usb_typec_info.dest != CCIC_NOTIFY_DEV_BATTERY) {
		dev_info(battery->dev, "%s: skip handler dest(%d)\n",
			__func__, usb_typec_info.dest);
		return 0;
	}

	mutex_lock(&battery->typec_notylock);
	switch (usb_typec_info.id) {
	case CCIC_NOTIFY_ID_WATER:
	case CCIC_NOTIFY_ID_ATTACH:
		switch (usb_typec_info.attach) {
		case MUIC_NOTIFY_CMD_DETACH:
		case MUIC_NOTIFY_CMD_LOGICALLY_DETACH:
			cmd = "DETACH";
			battery->is_jig_on = false;
			battery->pd_usb_attached = false;
			cable_type = SEC_BATTERY_CABLE_NONE;
			battery->muic_cable_type = ATTACHED_DEV_NONE_MUIC;
			battery->pdic_info.sink_status.rp_currentlvl = RP_CURRENT_LEVEL_NONE;
			break;
		case MUIC_NOTIFY_CMD_ATTACH:
		case MUIC_NOTIFY_CMD_LOGICALLY_ATTACH:
			/* Skip notify from MUIC if PDIC is attached already */
			if (is_pd_wire_type(battery->wire_status) || battery->init_src_cap) {
				if (lpcharge) {
					mutex_unlock(&battery->typec_notylock);
					return 0;
				} else if (!battery->usb_temp_flag && !(battery->misc_event & BATT_MISC_EVENT_TEMP_HICCUP_TYPE)) {
					mutex_unlock(&battery->typec_notylock);
					return 0;
				}
			}
			cmd = "ATTACH";
			battery->muic_cable_type = usb_typec_info.cable_type;
			cable_type = sec_bat_cable_check(battery, battery->muic_cable_type);
			if (battery->cable_type != cable_type &&
				battery->pdic_info.sink_status.rp_currentlvl >= RP_CURRENT_LEVEL_DEFAULT &&
				(cable_type == SEC_BATTERY_CABLE_USB || cable_type == SEC_BATTERY_CABLE_TA)) {
				sec_bat_set_rp_current(battery, cable_type);
			} else if ((struct pdic_notifier_struct *)usb_typec_info.pd != NULL &&
				(*(struct pdic_notifier_struct *)usb_typec_info.pd).event == PDIC_NOTIFY_EVENT_CCIC_ATTACH &&
				(*(struct pdic_notifier_struct *)usb_typec_info.pd).sink_status.rp_currentlvl >= RP_CURRENT_LEVEL_DEFAULT &&
				(cable_type == SEC_BATTERY_CABLE_USB || cable_type == SEC_BATTERY_CABLE_TA)) {
				battery->pdic_info.sink_status.rp_currentlvl =
					(*(struct pdic_notifier_struct *)usb_typec_info.pd).sink_status.rp_currentlvl;
				sec_bat_set_rp_current(battery, cable_type);
			}
			break;
		default:
			cmd = "ERROR";
			cable_type = -1;
			battery->muic_cable_type = usb_typec_info.cable_type;
			break;
		}
		battery->pdic_attach = false;
		battery->pdic_ps_rdy = false;
		battery->init_src_cap = false;
#if defined(CONFIG_AFC_CHARGER_MODE)
		if (battery->muic_cable_type == ATTACHED_DEV_QC_CHARGER_9V_MUIC ||
			battery->muic_cable_type == ATTACHED_DEV_QC_CHARGER_ERR_V_MUIC)
			battery->hv_chg_name = "QC";
		else if (battery->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_9V_MUIC ||
			battery->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_9V_DUPLI_MUIC ||
			battery->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_ERR_V_MUIC ||
			battery->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_ERR_V_DUPLI_MUIC)
			battery->hv_chg_name = "AFC";
#if defined(CONFIG_MUIC_HV_12V)
		else if (battery->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_12V_MUIC ||
			battery->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_12V_DUPLI_MUIC)
			battery->hv_chg_name = "12V";
#endif
		else
			battery->hv_chg_name = "NONE";
#endif
		break;
	case CCIC_NOTIFY_ID_POWER_STATUS:
#ifdef CONFIG_SEC_FACTORY
		dev_info(battery->dev, "%s: pd_event(%d)\n", __func__,
			(*(struct pdic_notifier_struct *)usb_typec_info.pd).event);
#endif
		if ((*(struct pdic_notifier_struct *)usb_typec_info.pd).event == PDIC_NOTIFY_EVENT_DETACH) {
			dev_info(battery->dev, "%s: skip pd operation - attach(%d)\n", __func__, usb_typec_info.attach);
			battery->pdic_attach = false;
			battery->pdic_ps_rdy = false;
			battery->init_src_cap = false;
			battery->hv_pdo = false;
			battery->pd_list.now_pd_index = 0;
#if defined(CONFIG_PDIC_PD30)
			battery->pd_list.now_isApdo = false;
			battery->pd_list.num_apdo = 0;
			battery->pd_list.num_fpdo = 0;
#endif
 			mutex_unlock(&battery->typec_notylock);
			return 0;
		} else if ((*(struct pdic_notifier_struct *)usb_typec_info.pd).event == PDIC_NOTIFY_EVENT_PD_PRSWAP_SNKTOSRC) {
			cmd = "PD_PRWAP";
			dev_info(battery->dev, "%s: PRSWAP_SNKTOSRC(%d)\n", __func__, usb_typec_info.attach);
			cable_type = SEC_BATTERY_CABLE_NONE;

			battery->pdic_attach = false;
			battery->pdic_ps_rdy = false;
			battery->init_src_cap = false;
			battery->hv_pdo = false;
			battery->pd_list.now_pd_index = 0;
			goto skip_cable_check;
		} else if (!lpcharge && (battery->usb_temp_flag || (battery->misc_event & BATT_MISC_EVENT_TEMP_HICCUP_TYPE))) {
			goto skip_cable_check;
		}

		cmd = "PD_ATTACH";
		if ((*(struct pdic_notifier_struct *)usb_typec_info.pd).event == PDIC_NOTIFY_EVENT_CCIC_ATTACH) {
			battery->pdic_info.sink_status.rp_currentlvl =
				(*(struct pdic_notifier_struct *)usb_typec_info.pd).sink_status.rp_currentlvl;
			dev_info(battery->dev, "%s: battery->rp_currentlvl(%d)\n", __func__, battery->pdic_info.sink_status.rp_currentlvl);
			if (battery->wire_status == SEC_BATTERY_CABLE_USB || battery->wire_status == SEC_BATTERY_CABLE_TA) {
				cable_type = battery->wire_status;
				battery->chg_limit = false;
				battery->lrp_limit = false;
				battery->lrp_step = LRP_NONE;
				sec_bat_set_rp_current(battery, cable_type);
				goto skip_cable_check;
			}
			mutex_unlock(&battery->typec_notylock);
			return 0;
		}
		battery->init_src_cap = false;
		if ((*(struct pdic_notifier_struct *)usb_typec_info.pd).event == PDIC_NOTIFY_EVENT_PD_SINK_CAP ||
			battery->update_pd_list) {
			pr_info("%s : update_pd_list(%d)\n", __func__, battery->update_pd_list);
#if defined(CONFIG_DIRECT_CHARGING)
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_DIRECT_CLEAR_ERR, val);
#endif
			battery->pdic_attach = false;
			battery->update_pd_list = false;
		}
		if (!battery->pdic_attach) {
			battery->pdic_info = *(struct pdic_notifier_struct *)usb_typec_info.pd;
			battery->pd_list.now_pd_index = 0;
			bPdIndexChanged = true;
		} else {
			unsigned int prev_pd_index = battery->pd_list.now_pd_index;

			battery->pdic_info.sink_status.selected_pdo_num =
				(*(struct pdic_notifier_struct *)usb_typec_info.pd).sink_status.selected_pdo_num;
			battery->pdic_info.sink_status.current_pdo_num =
				(*(struct pdic_notifier_struct *)usb_typec_info.pd).sink_status.current_pdo_num;
			battery->pd_list.now_pd_index = sec_bat_get_pd_list_index(&battery->pdic_info.sink_status,
				&battery->pd_list);
			dev_info(battery->dev, "%s: battery->pd_list.now_pd_index(%d), prev_pd_index(%d)\n",
				__func__, battery->pd_list.now_pd_index, prev_pd_index);
			if (battery->pd_list.now_pd_index != prev_pd_index) {
				bPdIndexChanged = true;
			}

			battery->pdic_ps_rdy = true;
		}
		current_pdo = battery->pdic_info.sink_status.current_pdo_num;
		if ((battery->pdic_info.sink_status.power_list[current_pdo].max_voltage / 100) > SEC_INPUT_VOLTAGE_5V)
			battery->hv_pdo = true;
		else
			battery->hv_pdo = false;
		dev_info(battery->dev, "%s: battery->pdic_ps_rdy(%d), hv_pdo(%d)\n",
				__func__, battery->pdic_ps_rdy, battery->hv_pdo);

#if defined(CONFIG_PDIC_PD30)
		if (battery->pdic_info.sink_status.has_apdo) {
			cable_type = SEC_BATTERY_CABLE_PDIC_APDO;
			if (battery->pdic_info.sink_status.power_list[current_pdo].apdo) {
				battery->hv_chg_name = "PDIC_APDO";
				battery->pd_list.now_isApdo = true;
			} else {
				battery->hv_chg_name = "PDIC_FIXED";
				battery->pd_list.now_isApdo = false;
			}

			if (battery->pdic_attach)
				bPrintPDlog = false;
		} else {
			cable_type = SEC_BATTERY_CABLE_PDIC;
			battery->hv_chg_name = "PDIC";
			battery->pd_list.now_isApdo = false;
		}
#else
		cable_type = SEC_BATTERY_CABLE_PDIC;
#if defined(CONFIG_AFC_CHARGER_MODE)
		battery->hv_chg_name = "PDIC";
#endif
#endif //_CONFIG_PDIC_PD30
		battery->muic_cable_type = ATTACHED_DEV_NONE_MUIC;
		battery->input_voltage =
				battery->pdic_info.sink_status.power_list[current_pdo].max_voltage / 100;
		dev_info(battery->dev, "%s: available pdo : %d, current pdo : %d\n", __func__,
			battery->pdic_info.sink_status.available_pdo_num, current_pdo);

		for(i=1; i<= battery->pdic_info.sink_status.available_pdo_num; i++) {
			bool isUpdated = false;
#if defined(CONFIG_PDIC_PD30)
			bool isApdo = battery->pdic_info.sink_status.power_list[i].apdo;
			bool isAccpet = battery->pdic_info.sink_status.power_list[i].accept;
#endif
			if (!battery->pdic_attach &&
				(battery->pdic_info.sink_status.power_list[i].max_voltage *
				battery->pdic_info.sink_status.power_list[i].max_current) > max_power) {
				max_power = battery->pdic_info.sink_status.power_list[i].max_voltage *
					battery->pdic_info.sink_status.power_list[i].max_current / 1000;
				pr_info("%s: max_power = %dmW\n", __func__, max_power);
			}
#if defined(CONFIG_PDIC_PD30)
			if (bPrintPDlog)
				pr_info("%s:%spower_list[%d,%s,%s], maxVol:%d, minVol:%d, maxCur:%d, power:%d\n",
					__func__, i == current_pdo ? "**" : "  ",
					i, isApdo ? "APDO" : "FIXED", isAccpet ? "O" : "X",
					battery->pdic_info.sink_status.power_list[i].max_voltage,
					isApdo ? battery->pdic_info.sink_status.power_list[i].min_voltage : 0,
					battery->pdic_info.sink_status.power_list[i].max_current,
					battery->pdic_info.sink_status.power_list[i].max_voltage *
					battery->pdic_info.sink_status.power_list[i].max_current);

			if (!battery->pdic_attach && battery->pdic_info.sink_status.has_apdo && !isApdo &&
				(battery->pdic_info.sink_status.power_list[i].max_voltage *
				battery->pdic_info.sink_status.power_list[i].max_current) > fpdo_power) {
				fpdo_power = battery->pdic_info.sink_status.power_list[i].max_voltage *
					battery->pdic_info.sink_status.power_list[i].max_current / 1000;
				pr_info("%s: fpdo_power = %dmW\n", __func__, fpdo_power);
			}

			/* no change apdo */
			if (!isAccpet || isApdo)
				continue;
#else
			pr_info("%s:%spower_list[%d], voltage : %d, current : %d, power : %d\n",
				__func__, i == current_pdo ? "**" : "  ", i,
				battery->pdic_info.sink_status.power_list[i].max_voltage,
				battery->pdic_info.sink_status.power_list[i].max_current,
				battery->pdic_info.sink_status.power_list[i].max_voltage *
				battery->pdic_info.sink_status.power_list[i].max_current);
#endif
			if ((battery->pdic_info.sink_status.power_list[i].max_voltage *
			     battery->pdic_info.sink_status.power_list[i].max_current) >
			    (pd_charging_charge_power * 1000)) {
				battery->pdic_info.sink_status.power_list[i].max_current =
					(pd_charging_charge_power * 1000) /
					battery->pdic_info.sink_status.power_list[i].max_voltage;
				isUpdated = true;
			}

			if(battery->pdic_info.sink_status.power_list[i].max_current >
			    battery->pdata->max_input_current) {
				isUpdated = true;
				battery->pdic_info.sink_status.power_list[i].max_current =
					battery->pdata->max_input_current;
			}

			if (isUpdated) {
#if defined(CONFIG_PDIC_PD30)
				if (bPrintPDlog)
					pr_info("%s: ->updated [%d,%s,%s], maxVol:%d, minVol:%d, maxCur:%d, power:%d\n",
						__func__, i, isApdo ? "APDO" : "FIXED", isAccpet ? "O" : "X",
						battery->pdic_info.sink_status.power_list[i].max_voltage,
						isApdo ? battery->pdic_info.sink_status.power_list[i].min_voltage : 0,
						battery->pdic_info.sink_status.power_list[i].max_current,
						battery->pdic_info.sink_status.power_list[i].max_voltage *
						battery->pdic_info.sink_status.power_list[i].max_current);
#else
				pr_info("%s: ->updated [%d], voltage : %d, current : %d, power : %d\n", __func__, i,
					battery->pdic_info.sink_status.power_list[i].max_voltage,
					battery->pdic_info.sink_status.power_list[i].max_current,
					battery->pdic_info.sink_status.power_list[i].max_voltage *
					battery->pdic_info.sink_status.power_list[i].max_current);
#endif
			}
		}

		if (!battery->pdic_attach) {
#if defined(CONFIG_PDIC_PD30)
			if (battery->pdic_info.sink_status.has_apdo &&
				!(battery->current_event & SEC_BAT_CURRENT_EVENT_HV_DISABLE)) {
				fpdo_power = fpdo_power > battery->pdata->max_charging_charge_power ?
					battery->pdata->max_charging_charge_power : fpdo_power;
				battery->max_charge_power = fpdo_power;
				battery->pd_max_charge_power = battery->max_charge_power;
				pr_info("%s: pd_max_charge_power = %dmW\n", __func__, battery->pd_max_charge_power);
			}
#endif
			count_cisd_power_data(&battery->cisd, max_power);

			if (make_pd_list(battery) <= 0)
				goto skip_cable_work;
		}
		battery->pdic_attach = true;
#if defined(CONFIG_PDIC_PD30)
		if (is_pd_apdo_wire_type(battery->wire_status) && !bPdIndexChanged &&
			battery->pdic_info.sink_status.power_list[current_pdo].apdo) {
			battery->wire_status = cable_type;
			goto skip_cable_work;
		}
#endif
		break;
	case CCIC_NOTIFY_ID_USB:
		if(usb_typec_info.cable_type == PD_USB_TYPE)
			battery->pd_usb_attached = true;
		dev_info(battery->dev, "%s: CCIC_NOTIFY_ID_USB: %d\n",__func__, battery->pd_usb_attached);
		wake_lock(&battery->monitor_wake_lock);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		mutex_unlock(&battery->typec_notylock);
		return 0;
	default:
		cmd = "ERROR";
		cable_type = -1;
		battery->muic_cable_type = ATTACHED_DEV_NONE_MUIC;
#if defined(CONFIG_AFC_CHARGER_MODE)
		battery->hv_chg_name = "NONE";
#endif
		break;
	}

skip_cable_check:
	sec_bat_set_misc_event(battery,
		(battery->muic_cable_type == ATTACHED_DEV_UNDEFINED_CHARGING_MUIC ? BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE : 0) |
		(battery->muic_cable_type == ATTACHED_DEV_UNDEFINED_RANGE_MUIC ? BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE : 0),
		BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE);

	if (battery->muic_cable_type == ATTACHED_DEV_HICCUP_MUIC) {
		if (battery->usb_temp_flag || (battery->misc_event & BATT_MISC_EVENT_TEMP_HICCUP_TYPE)) {
			pr_info("%s: Hiccup Set because of USB Temp\n", __func__);
			sec_bat_set_misc_event(battery, BATT_MISC_EVENT_TEMP_HICCUP_TYPE, BATT_MISC_EVENT_TEMP_HICCUP_TYPE);
			battery->usb_temp_flag = false;
		} else {
			pr_info("%s: Hiccup Set because of Water detect\n", __func__);
			sec_bat_set_misc_event(battery, BATT_MISC_EVENT_HICCUP_TYPE, BATT_MISC_EVENT_HICCUP_TYPE);
		}
		battery->hiccup_status = 1;
	} else {
		battery->hiccup_status = 0;
		if (battery->hiccup_clear) {
			sec_bat_set_misc_event(battery,
									0, (BATT_MISC_EVENT_HICCUP_TYPE | BATT_MISC_EVENT_TEMP_HICCUP_TYPE));
			battery->hiccup_clear = false;
			pr_info("%s : Hiccup event clear! hiccup clear bit set (%d)\n", __func__, battery->hiccup_clear);
		} else if (battery->misc_event & (BATT_MISC_EVENT_HICCUP_TYPE | BATT_MISC_EVENT_TEMP_HICCUP_TYPE)) {
			wake_lock(&battery->monitor_wake_lock);
			queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		}
	}

	/* showing charging icon and noti(no sound, vi, haptic) only
	   if slow insertion is detected by MUIC */
	sec_bat_set_misc_event(battery,
		(battery->muic_cable_type == ATTACHED_DEV_TIMEOUT_OPEN_MUIC ? BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE : 0),
		 BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE);

	if (cable_type < 0 || cable_type > SEC_BATTERY_CABLE_MAX) {
		dev_info(battery->dev, "%s: ignore event(%d)\n",
			__func__, battery->muic_cable_type);
		goto skip_cable_work;
	} else if ((cable_type == SEC_BATTERY_CABLE_UNKNOWN) &&
		   (battery->status != POWER_SUPPLY_STATUS_DISCHARGING)) {
		battery->cable_type = cable_type;
		wake_lock(&battery->monitor_wake_lock);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		dev_info(battery->dev, "%s: UNKNOWN cable plugin\n", __func__);
		goto skip_cable_work;
	}
	battery->wire_status = cable_type;

	if (battery->wc_tx_enable) {
		if (battery->wire_status == SEC_BATTERY_CABLE_NONE) {
			battery->buck_cntl_by_tx = true;
			battery->tx_switch_mode = TX_SWITCH_MODE_OFF;
			battery->tx_switch_mode_change = false;	
			battery->tx_switch_start_soc = 0;
		}

		cancel_delayed_work(&battery->wpc_tx_work);
		wake_lock(&battery->wpc_tx_wake_lock);
		queue_delayed_work(battery->monitor_wqueue,
				&battery->wpc_tx_work, 0);
	}

	cancel_delayed_work(&battery->cable_work);
	wake_unlock(&battery->cable_wake_lock);

	if (cable_type == SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT) {
		/* set current event */
		cancel_delayed_work(&battery->afc_work);
		wake_unlock(&battery->afc_wake_lock);
		sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_CHG_LIMIT,
					  (SEC_BAT_CURRENT_EVENT_CHG_LIMIT | SEC_BAT_CURRENT_EVENT_AFC));
		wake_lock(&battery->monitor_wake_lock);
		battery->polling_count = 1;	/* initial value = 1 */
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
	} else if ((battery->wire_status == battery->cable_type) &&
		(((battery->wire_status == SEC_BATTERY_CABLE_USB || battery->wire_status == SEC_BATTERY_CABLE_TA) &&
		battery->pdic_info.sink_status.rp_currentlvl > RP_CURRENT_LEVEL_DEFAULT &&
		!(battery->current_event & SEC_BAT_CURRENT_EVENT_AFC)) ||
		is_hv_wire_type(battery->wire_status))) {
		cancel_delayed_work(&battery->afc_work);
		wake_unlock(&battery->afc_wake_lock);
		sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_AFC);

		wake_lock(&battery->monitor_wake_lock);
		battery->polling_count = 1;	/* initial value = 1 */
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
	} else if (cable_type == SEC_BATTERY_CABLE_PREPARE_TA) {
		sec_bat_set_current_event(battery,
			SEC_BAT_CURRENT_EVENT_AFC, SEC_BAT_CURRENT_EVENT_AFC);
		sec_bat_set_charging_current(battery);
		cancel_delayed_work(&battery->afc_work);
		wake_unlock(&battery->afc_wake_lock);
	} else {
		wake_lock(&battery->cable_wake_lock);
		if (battery->ta_alert_wa && battery->ta_alert_mode != OCP_NONE) {
			if (!strcmp(cmd, "DETACH")) {
				queue_delayed_work(battery->monitor_wqueue,
					&battery->cable_work, msecs_to_jiffies(3000));
			} else {
				queue_delayed_work(battery->monitor_wqueue,
					&battery->cable_work, 0);
			}
		} else {
			queue_delayed_work(battery->monitor_wqueue,
				&battery->cable_work, 0);
		}
	}

skip_cable_work:
	dev_info(battery->dev, "%s: CMD[%s], CABLE_TYPE[%d]\n", __func__, cmd, cable_type);
	mutex_unlock(&battery->typec_notylock);
	return 0;
}
#else
#if defined(CONFIG_CCIC_NOTIFIER)
static int batt_pdic_handle_notification(struct notifier_block *nb,
		unsigned long action, void *data)
{
	const char *cmd;
	struct sec_battery_info *battery =
		container_of(nb, struct sec_battery_info,
				pdic_nb);
	battery->pdic_info = *(struct pdic_notifier_struct *)data;

	mutex_lock(&battery->batt_handlelock);
	pr_info("%s: pdic_event: %d\n", __func__, battery->pdic_info.event);

	switch (battery->pdic_info.event) {
		int i, selected_pdo;

		case PDIC_NOTIFY_EVENT_DETACH:
			cmd = "DETACH";
			battery->pdic_attach = false;
			battery->init_src_cap = false;
			if (battery->wire_status == SEC_BATTERY_CABLE_PDIC) {
				battery->wire_status = SEC_BATTERY_CABLE_NONE;
				wake_lock(&battery->cable_wake_lock);
				queue_delayed_work(battery->monitor_wqueue,
						&battery->cable_work, 0);
			}
			break;
		case PDIC_NOTIFY_EVENT_CCIC_ATTACH:
			cmd = "ATTACH";
			break;
		case PDIC_NOTIFY_EVENT_PD_SINK:
			selected_pdo = battery->pdic_info.sink_status.selected_pdo_num;
			cmd = "ATTACH";
			battery->wire_status = SEC_BATTERY_CABLE_PDIC;
			battery->pdic_attach = true;
			battery->input_voltage =
				battery->pdic_info.sink_status.power_list[selected_pdo].max_voltage / 100;

			pr_info("%s: total pdo : %d, selected pdo : %d\n", __func__,
					battery->pdic_info.sink_status.available_pdo_num, selected_pdo);
			for(i=1; i<= battery->pdic_info.sink_status.available_pdo_num; i++)
			{
				pr_info("%s: power_list[%d], voltage : %d, current : %d, power : %d\n", __func__, i,
						battery->pdic_info.sink_status.power_list[i].max_voltage,
						battery->pdic_info.sink_status.power_list[i].max_current,
						battery->pdic_info.sink_status.power_list[i].max_voltage *
						battery->pdic_info.sink_status.power_list[i].max_current);
			}
			wake_lock(&battery->cable_wake_lock);
			queue_delayed_work(battery->monitor_wqueue,
					&battery->cable_work, 0);
			break;
		case PDIC_NOTIFY_EVENT_PD_SOURCE:
			cmd = "ATTACH";
			break;
		default:
			cmd = "ERROR";
			break;
	}
	pr_info("%s: CMD=%s, cable_type : %d\n", __func__, cmd, battery->cable_type);
	mutex_unlock(&battery->batt_handlelock);
	return 0;
}
#endif

#if defined(CONFIG_MUIC_NOTIFIER)
static int batt_handle_notification(struct notifier_block *nb,
		unsigned long action, void *data)
{
	const char *cmd;
	int cable_type = SEC_BATTERY_CABLE_NONE;
	struct sec_battery_info *battery =
		container_of(nb, struct sec_battery_info,
			     batt_nb);
	union power_supply_propval value = {0, };

#if defined(CONFIG_CCIC_NOTIFIER)
	CC_NOTI_ATTACH_TYPEDEF *p_noti = (CC_NOTI_ATTACH_TYPEDEF *)data;
	muic_attached_dev_t attached_dev = p_noti->cable_type;
#else
	muic_attached_dev_t attached_dev = *(muic_attached_dev_t *)data;
#endif

	mutex_lock(&battery->batt_handlelock);
	switch (action) {
	case MUIC_NOTIFY_CMD_DETACH:
	case MUIC_NOTIFY_CMD_LOGICALLY_DETACH:
		cmd = "DETACH";
		battery->is_jig_on = false;
		cable_type = SEC_BATTERY_CABLE_NONE;
		battery->muic_cable_type = ATTACHED_DEV_NONE_MUIC;
		break;
	case MUIC_NOTIFY_CMD_ATTACH:
	case MUIC_NOTIFY_CMD_LOGICALLY_ATTACH:
		cmd = "ATTACH";
		cable_type = sec_bat_cable_check(battery, attached_dev);
		battery->muic_cable_type = attached_dev;
		break;
	default:
		cmd = "ERROR";
		cable_type = -1;
		battery->muic_cable_type = ATTACHED_DEV_NONE_MUIC;
		break;
	}

	sec_bat_set_misc_event(battery,
#if !defined(CONFIG_ENG_BATTERY_CONCEPT) && !defined(CONFIG_SEC_FACTORY)
		(battery->muic_cable_type == ATTACHED_DEV_JIG_UART_ON_MUIC ? BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE : 0) |
		(battery->muic_cable_type == ATTACHED_DEV_JIG_USB_ON_MUIC ? BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE : 0) |
#endif
		(battery->muic_cable_type == ATTACHED_DEV_UNDEFINED_RANGE_MUIC ? BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE : 0),
		BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE);

	if (battery->muic_cable_type == ATTACHED_DEV_HICCUP_MUIC) {
		if (battery->usb_temp_flag || (battery->misc_event & BATT_MISC_EVENT_TEMP_HICCUP_TYPE)) {
			pr_info("%s: Hiccup Set because of USB Temp\n", __func__);
			sec_bat_set_misc_event(battery, BATT_MISC_EVENT_TEMP_HICCUP_TYPE, BATT_MISC_EVENT_TEMP_HICCUP_TYPE);
			battery->usb_temp_flag = false;
		} else {
			pr_info("%s: Hiccup Set because of Water detect\n", __func__);
			sec_bat_set_misc_event(battery, BATT_MISC_EVENT_HICCUP_TYPE, BATT_MISC_EVENT_HICCUP_TYPE);
		}
		battery->hiccup_status = 1;
	} else {
		battery->hiccup_status = 0;
		if (battery->misc_event & (BATT_MISC_EVENT_HICCUP_TYPE | BATT_MISC_EVENT_TEMP_HICCUP_TYPE)) {
			wake_lock(&battery->monitor_wake_lock);
			queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		}
	}

#if defined(CONFIG_CCIC_NOTIFIER)
	/* If PD cable is already attached, return this function */
	if(battery->pdic_attach) {
		dev_info(battery->dev, "%s: ignore event pdic attached(%d)\n",
			__func__, battery->pdic_attach);
		mutex_unlock(&battery->batt_handlelock);
		return 0;
	}
#endif

	if (attached_dev == ATTACHED_DEV_MHL_MUIC) {
		mutex_unlock(&battery->batt_handlelock);
		return 0;
	}

	if (cable_type < 0) {
		dev_info(battery->dev, "%s: ignore event(%d)\n",
			__func__, cable_type);
	} else if (cable_type == SEC_BATTERY_CABLE_POWER_SHARING) {
		battery->ps_status = true;
		battery->ps_enable = true;
		battery->wire_status = cable_type;
		dev_info(battery->dev, "%s: power sharing cable plugin\n", __func__);
	} else if (cable_type == SEC_BATTERY_CABLE_WIRELESS) {
		battery->wc_status = SEC_WIRELESS_PAD_WPC;
	} else if (cable_type == SEC_BATTERY_CABLE_WIRELESS_PACK) {
		battery->wc_status = SEC_WIRELESS_PAD_WPC_PACK;
	} else if (cable_type == SEC_BATTERY_CABLE_WIRELESS_HV_PACK) {
		battery->wc_status = SEC_WIRELESS_PAD_WPC_PACK_HV;
	} else if (cable_type == SEC_BATTERY_CABLE_HV_WIRELESS) {
		battery->wc_status = SEC_WIRELESS_PAD_WPC_HV;
	} else if (cable_type == SEC_BATTERY_CABLE_WIRELESS_STAND) {
		battery->wc_status = SEC_WIRELESS_PAD_WPC_STAND;
	} else if (cable_type == SEC_BATTERY_CABLE_WIRELESS_HV_STAND) {
		battery->wc_status = SEC_WIRELESS_PAD_WPC_STAND_HV;
	} else if (cable_type == SEC_BATTERY_CABLE_PMA_WIRELESS) {
		battery->wc_status = SEC_WIRELESS_PAD_PMA;
	} else if (cable_type == SEC_BATTERY_CABLE_WIRELESS_VEHICLE) {
		battery->wc_status = SEC_WIRELESS_PAD_VEHICLE;
	} else if (cable_type == SEC_BATTERY_CABLE_WIRELESS_HV_VEHICLE) {
		battery->wc_status = SEC_WIRELESS_PAD_VEHICLE_HV;
	} else if (cable_type == SEC_BATTERY_CABLE_WIRELESS_TX) {
		battery->wc_status = SEC_WIRELESS_PAD_TX;
	} else if (cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_20) {
		battery->wc_status = SEC_WIRELESS_PAD_WPC_PREPARE_HV_20;
	} else if (cable_type == SEC_BATTERY_CABLE_HV_WIRELESS_20) {
		battery->wc_status = SEC_WIRELESS_PAD_WPC_HV_20;
	} else if ((cable_type == SEC_BATTERY_CABLE_UNKNOWN) &&
		   (battery->status != POWER_SUPPLY_STATUS_DISCHARGING)) {
		battery->cable_type = cable_type;
		wake_lock(&battery->monitor_wake_lock);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		dev_info(battery->dev,
			"%s: UNKNOWN cable plugin\n", __func__);
		mutex_unlock(&battery->batt_handlelock);
		return 0;
	} else {
		battery->wire_status = cable_type;
		if (is_nocharge_type(battery->wire_status) &&
			(battery->wc_status) && (!battery->ps_status))
			cable_type = SEC_BATTERY_CABLE_WIRELESS;
	}
	dev_info(battery->dev,
			"%s: current_cable(%d), wc_status(%d), wire_status(%d)\n",
			__func__, cable_type, battery->wc_status,
			battery->wire_status);

	mutex_unlock(&battery->batt_handlelock);
	if (attached_dev == ATTACHED_DEV_USB_LANHUB_MUIC) {
		if (!strcmp(cmd, "ATTACH")) {
			value.intval = true;
			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_PROP_CHARGE_POWERED_OTG_CONTROL,
					value);
			dev_info(battery->dev,
				"%s: Powered OTG cable attached\n", __func__);
		} else {
			value.intval = false;
			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_PROP_CHARGE_POWERED_OTG_CONTROL,
					value);
			dev_info(battery->dev,
				"%s: Powered OTG cable detached\n", __func__);
		}
	}

#if defined(CONFIG_AFC_CHARGER_MODE)
	if (!strcmp(cmd, "ATTACH")) {
		if ((battery->muic_cable_type >= ATTACHED_DEV_QC_CHARGER_PREPARE_MUIC) &&
		    (battery->muic_cable_type <= ATTACHED_DEV_QC_CHARGER_9V_MUIC)) {
			battery->hv_chg_name = "QC";
		} else if ((battery->muic_cable_type >= ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC) &&
			 (battery->muic_cable_type <= ATTACHED_DEV_AFC_CHARGER_ERR_V_DUPLI_MUIC)) {
			battery->hv_chg_name = "AFC";
#if defined(CONFIG_MUIC_HV_12V)
		} else if (battery->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_12V_MUIC ||
			battery->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_12V_DUPLI_MUIC) {
			battery->hv_chg_name = "12V";
#endif
		} else
			battery->hv_chg_name = "NONE";
	} else {
			battery->hv_chg_name = "NONE";
	}

	pr_info("%s : HV_CHARGER_NAME(%s)\n",
		__func__, battery->hv_chg_name);
#endif

	if ((cable_type >= 0) &&
	    cable_type <= SEC_BATTERY_CABLE_MAX) {
		if (cable_type == SEC_BATTERY_CABLE_POWER_SHARING) {
			value.intval = battery->ps_enable;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, value);
			wake_lock(&battery->monitor_wake_lock);
			queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		} else if((cable_type == SEC_BATTERY_CABLE_NONE) && (battery->ps_status)) {
			if (battery->ps_enable) {
				battery->ps_enable = false;
				value.intval = battery->ps_enable;
				psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, value);
			}
			battery->ps_status = false;
			wake_lock(&battery->monitor_wake_lock);
			queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		} else if(cable_type != battery->cable_type) {
			wake_lock(&battery->cable_wake_lock);
			queue_delayed_work(battery->monitor_wqueue,
					   &battery->cable_work, 0);
		} else {
			dev_info(battery->dev,
				"%s: Cable is Not Changed(%d)\n",
				__func__, battery->cable_type);
		}
	}

	pr_info("%s: CMD=%s, attached_dev=%d\n", __func__, cmd, attached_dev);

	return 0;
}
#endif /* CONFIG_MUIC_NOTIFIER */
#endif

#if defined(CONFIG_VBUS_NOTIFIER)
static int vbus_handle_notification(struct notifier_block *nb,
		unsigned long action, void *data)
{
	vbus_status_t vbus_status = *(vbus_status_t *)data;
	struct sec_battery_info *battery =
		container_of(nb, struct sec_battery_info,
			     vbus_nb);
	union power_supply_propval value = {0, };

	mutex_lock(&battery->batt_handlelock);
	if (battery->muic_cable_type == ATTACHED_DEV_HMT_MUIC &&
		battery->muic_vbus_status != vbus_status &&
		battery->muic_vbus_status == STATUS_VBUS_HIGH &&
		vbus_status == STATUS_VBUS_LOW) {
		msleep(500);
		value.intval = true;
		psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL,
				value);
		dev_info(battery->dev,
			"%s: changed to OTG cable attached\n", __func__);

		battery->wire_status = SEC_BATTERY_CABLE_OTG;
		wake_lock(&battery->cable_wake_lock);
		queue_delayed_work(battery->monitor_wqueue, &battery->cable_work, 0);
	}
	pr_info("%s: action=%d, vbus_status=%d\n", __func__, (int)action, vbus_status);
	mutex_unlock(&battery->batt_handlelock);
	battery->muic_vbus_status = vbus_status;

	return 0;
}
#endif

#if !defined(CONFIG_MUIC_NOTIFIER)
static void cable_initial_check(struct sec_battery_info *battery)
{
	union power_supply_propval value;

	pr_info("%s : current_cable_type : (%d)\n", __func__, battery->cable_type);

	if (SEC_BATTERY_CABLE_NONE !=  battery->cable_type) {
		if (battery->cable_type == SEC_BATTERY_CABLE_POWER_SHARING) {
			value.intval =  battery->cable_type;
			psy_do_property("ps", set,
					POWER_SUPPLY_PROP_ONLINE, value);
		} else {
			value.intval =  battery->cable_type;
			psy_do_property("battery", set,
					POWER_SUPPLY_PROP_ONLINE, value);
		}
	} else {
		psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_PROP_ONLINE, value);
		if (value.intval == SEC_BATTERY_CABLE_WIRELESS) {
			value.intval = 1;
			psy_do_property("wireless", set,
				POWER_SUPPLY_PROP_ONLINE, value);
		}
	}
}
#endif

static void sec_bat_init_chg_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, init_chg_work.work);

	if (battery->cable_type == SEC_BATTERY_CABLE_NONE &&
		!(battery->misc_event & (BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE |
			BATT_MISC_EVENT_HICCUP_TYPE | BATT_MISC_EVENT_TEMP_HICCUP_TYPE))) {
		pr_info("%s: disable charging\n", __func__);
		sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
	}
}

static const struct power_supply_desc battery_power_supply_desc = {
	.name = "battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = sec_battery_props,
	.num_properties = ARRAY_SIZE(sec_battery_props),
	.get_property = sec_bat_get_property,
	.set_property = sec_bat_set_property,
};

static const struct power_supply_desc usb_power_supply_desc = {
	.name = "usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.properties = sec_power_props,
	.num_properties = ARRAY_SIZE(sec_power_props),
	.get_property = sec_usb_get_property,
};

static const struct power_supply_desc ac_power_supply_desc = {
	.name = "ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.properties = sec_ac_props,
	.num_properties = ARRAY_SIZE(sec_ac_props),
	.get_property = sec_ac_get_property,
};

static const struct power_supply_desc wireless_power_supply_desc = {
	.name = "wireless",
	.type = POWER_SUPPLY_TYPE_WIRELESS,
	.properties = sec_wireless_props,
	.num_properties = ARRAY_SIZE(sec_wireless_props),
	.get_property = sec_wireless_get_property,
	.set_property = sec_wireless_set_property,
};

static const struct power_supply_desc ps_power_supply_desc = {
	.name = "ps",
	.type = POWER_SUPPLY_TYPE_POWER_SHARING,
	.properties = sec_ps_props,
	.num_properties = ARRAY_SIZE(sec_ps_props),
	.get_property = sec_ps_get_property,
	.set_property = sec_ps_set_property,
};

static int sec_battery_probe(struct platform_device *pdev)
{
	sec_battery_platform_data_t *pdata = NULL;
	struct sec_battery_info *battery;
	struct power_supply_config battery_cfg = {};

	int ret = 0;
#ifndef CONFIG_OF
	int i = 0;
#endif

	union power_supply_propval value = {0, };

	dev_info(&pdev->dev,
		"%s: SEC Battery Driver Loading\n", __func__);

	battery = kzalloc(sizeof(*battery), GFP_KERNEL);
	if (!battery)
		return -ENOMEM;

	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev,
				sizeof(sec_battery_platform_data_t),
				GFP_KERNEL);
		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_bat_free;
		}

		battery->pdata = pdata;

		if (sec_bat_parse_dt(&pdev->dev, battery)) {
			dev_err(&pdev->dev,
				"%s: Failed to get battery dt\n", __func__);
			ret = -EINVAL;
			goto err_bat_free;
		}
	} else {
		pdata = dev_get_platdata(&pdev->dev);
		battery->pdata = pdata;
	}

	platform_set_drvdata(pdev, battery);

	battery->dev = &pdev->dev;

	mutex_init(&battery->adclock);
	mutex_init(&battery->iolock);
	mutex_init(&battery->misclock);
	mutex_init(&battery->txeventlock);
	mutex_init(&battery->batt_handlelock);
	mutex_init(&battery->current_eventlock);
	mutex_init(&battery->typec_notylock);
	mutex_init(&battery->wclock);
	mutex_init(&battery->voutlock);	

	dev_dbg(battery->dev, "%s: ADC init\n", __func__);

#ifdef CONFIG_OF
	adc_init(pdev, battery);
#else
	for (i = 0; i < SEC_BAT_ADC_CHANNEL_NUM; i++)
		adc_init(pdev, pdata, i);
#endif
	wake_lock_init(&battery->monitor_wake_lock, WAKE_LOCK_SUSPEND,
		       "sec-battery-monitor");
	wake_lock_init(&battery->cable_wake_lock, WAKE_LOCK_SUSPEND,
		       "sec-battery-cable");
	wake_lock_init(&battery->vbus_wake_lock, WAKE_LOCK_SUSPEND,
		       "sec-battery-vbus");
	wake_lock_init(&battery->afc_wake_lock, WAKE_LOCK_SUSPEND,
		       "sec-battery-afc");
	wake_lock_init(&battery->siop_level_wake_lock, WAKE_LOCK_SUSPEND,
			"sec-battery-siop_level");
	wake_lock_init(&battery->ext_event_wake_lock, WAKE_LOCK_SUSPEND,
			"sec-battery-ext_event");
	wake_lock_init(&battery->wc_headroom_wake_lock, WAKE_LOCK_SUSPEND,
			"sec-battery-wc_headroom");
	wake_lock_init(&battery->wpc_tx_wake_lock, WAKE_LOCK_SUSPEND,
			"sec-battery-wcp-tx");
	wake_lock_init(&battery->wpc_tx_en_wake_lock, WAKE_LOCK_SUSPEND,
			"sec-battery-wpc_tx_en");
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	wake_lock_init(&battery->batt_data_wake_lock, WAKE_LOCK_SUSPEND,
			"sec-battery-update-data");
#endif
	wake_lock_init(&battery->misc_event_wake_lock, WAKE_LOCK_SUSPEND,
			"sec-battery-misc-event");
	wake_lock_init(&battery->tx_event_wake_lock, WAKE_LOCK_SUSPEND,
			"sec-battery-tx-event");
#ifdef CONFIG_OF
	wake_lock_init(&battery->parse_mode_dt_wake_lock, WAKE_LOCK_SUSPEND,
			"sec-battery-parse_mode_dt");
#endif

	/* initialization of battery info */
	sec_bat_set_charging_status(battery,
			POWER_SUPPLY_STATUS_DISCHARGING);
	battery->health = POWER_SUPPLY_HEALTH_GOOD;
	battery->ta_alert_mode = OCP_NONE;
	battery->present = true;
	battery->is_jig_on = false;
	battery->wdt_kick_disable = 0;

	battery->polling_count = 1;	/* initial value = 1 */
	battery->polling_time = pdata->polling_time[
		SEC_BATTERY_POLLING_TIME_DISCHARGING];
	battery->polling_in_sleep = false;
	battery->polling_short = false;

	battery->check_count = 0;
	battery->check_adc_count = 0;
	battery->check_adc_value = 0;

	battery->input_current = 0;
	battery->charging_current = 0;
	battery->topoff_current = 0;
	battery->wpc_vout_level = WIRELESS_VOUT_10V;
	battery->wpc_max_vout_level = WIRELESS_VOUT_12_5V;	
	battery->charging_start_time = 0;
	battery->charging_passed_time = 0;
	battery->wc_heating_start_time = 0;
	battery->wc_heating_passed_time = 0;
	battery->charging_next_time = 0;
	battery->charging_fullcharged_time = 0;
	battery->siop_level = 100;
	battery->wc_enable = 1;
	battery->wc_enable_cnt = 0;
	battery->wc_enable_cnt_value = 3;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	battery->stability_test = 0;
	battery->eng_not_full_status = 0;
	battery->temperature_test_battery = 0x7FFF;
	battery->temperature_test_usb = 0x7FFF;
	battery->temperature_test_wpc = 0x7FFF;
	battery->temperature_test_chg = 0x7FFF;
	battery->temperature_test_dchg = 0x7FFF;
	battery->test_max_current = false;
	battery->test_charge_current = false;
#if defined(CONFIG_STEP_CHARGING)
	battery->test_step_condition = 0x7FFF;
	battery->step_charging_status = -1;
#endif
#endif
	battery->ps_enable = false;
    battery->wc_status = SEC_WIRELESS_PAD_NONE;
	battery->wc_cv_mode = false;
	battery->wire_status = SEC_BATTERY_CABLE_NONE;

	battery->wc_rx_phm_mode = false;
	battery->wc_tx_enable = false;
	battery->uno_en = false;
	battery->afc_disable = false;
	battery->buck_cntl_by_tx = false;
	battery->wc_tx_vout = WC_TX_VOUT_5_0V;
	battery->wc_rx_type = NO_DEV;
	battery->tx_mfc_iout = 0;
	battery->tx_uno_iout = 0;
	battery->wc_need_ldo_on = false;

	battery->tx_minduty = battery->pdata->tx_minduty_default;

#if defined(CONFIG_WIRELESS_TX_MODE)
	battery->tx_clear = true;
	battery->tx_clear_cisd = true;
#endif
#if defined(CONFIG_BATTERY_SWELLING)
	battery->swelling_mode = SWELLING_MODE_NONE;
#endif
	battery->charging_block = false;
	battery->chg_limit = false;
	battery->lrp_limit = false;
	battery->lrp_step = LRP_NONE;
	battery->mix_limit = false;
	battery->vbus_limit = false;
	battery->vbus_chg_by_siop = SEC_INPUT_VOLTAGE_0V;
	battery->vbus_chg_by_full = false;
	battery->usb_temp = 0;
#if defined(CONFIG_DIRECT_CHARGING)
	battery->dchg_temp = 0;
#endif
#if defined(CONFIG_ENG_BATTERY_CONCEPT) || defined(CONFIG_SEC_FACTORY)
	battery->cooldown_mode = true;
#endif
	battery->lrp = 0;
	battery->lrp_test = 0;
	battery->skip_swelling = false;
	battery->led_cover = 0;
	battery->hiccup_status = 0;
	battery->hiccup_clear = false;
	battery->ext_event = BATT_EXT_EVENT_NONE;
	battery->tx_retry_case = SEC_BAT_TX_RETRY_NONE;
	battery->tx_misalign_cnt = 0;
	battery->tx_ocp_cnt = 0;
	battery->auto_mode = false;
	battery->update_pd_list = false;

	psy_do_property(battery->pdata->wireless_charger_name, get,
		POWER_SUPPLY_EXT_PROP_WPC_EN, value);
	sec_bat_set_current_event(battery,
		value.intval ? SEC_BAT_CURRENT_EVENT_WPC_EN : 0, SEC_BAT_CURRENT_EVENT_WPC_EN);
	sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_USB_100MA, SEC_BAT_CURRENT_EVENT_USB_100MA);

	if (lpcharge) {
		battery->temp_highlimit_threshold =
			battery->pdata->temp_highlimit_threshold_lpm;
		battery->temp_highlimit_recovery =
			battery->pdata->temp_highlimit_recovery_lpm;
		battery->temp_high_threshold =
			battery->pdata->temp_high_threshold_lpm;
		battery->temp_high_recovery =
			battery->pdata->temp_high_recovery_lpm;
		battery->temp_low_recovery =
			battery->pdata->temp_low_recovery_lpm;
		battery->temp_low_threshold =
			battery->pdata->temp_low_threshold_lpm;
	} else {
		battery->temp_highlimit_threshold =
			battery->pdata->temp_highlimit_threshold_normal;
		battery->temp_highlimit_recovery =
			battery->pdata->temp_highlimit_recovery_normal;
		battery->temp_high_threshold =
			battery->pdata->temp_high_threshold_normal;
		battery->temp_high_recovery =
			battery->pdata->temp_high_recovery_normal;
		battery->temp_low_recovery =
			battery->pdata->temp_low_recovery_normal;
		battery->temp_low_threshold =
			battery->pdata->temp_low_threshold_normal;
	}

	battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
	battery->is_recharging = false;
	battery->cable_type = SEC_BATTERY_CABLE_NONE;
	battery->test_mode = 0;
	battery->factory_mode = false;
	battery->store_mode = false;
	battery->is_hc_usb = false;
	battery->is_sysovlo = false;
	battery->is_vbatovlo = false;
	battery->is_abnormal_temp = false;
	battery->hv_pdo = false;

	battery->safety_timer_set = true;
	battery->stop_timer = false;
	battery->prev_safety_time = 0;
	battery->lcd_status = false;

	battery->wc20_power_class = 0; 
#if defined(CONFIG_CALC_TIME_TO_FULL)
	battery->ttf_predict_wc20_charge_current = 0;
#endif
	battery->wc20_vout = 0;

#if defined(CONFIG_BATTERY_CISD)
	battery->usb_overheat_check = false;
	battery->skip_cisd = false;
#endif

#if defined(CONFIG_BATTERY_AGE_FORECAST)
	battery->batt_cycle = -1;
	battery->pdata->age_step = 0;
#endif

	battery->batt_asoc = 100;
	battery->health_change = false;
	battery->usb_temp_flag = false;

	battery->batt_full_capacity = 0;
	battery->usb_slow_chg = false;

	/* Check High Voltage charging option for wireless charging */
	/* '1' means disabling High Voltage charging */
	if (charging_night_mode == '1')
		sleep_mode = true;
	else
		sleep_mode = false;

	/* '1' means disable usb temp check & high temp/highlimit temp */
	if (temp_control_test == '1')
		sec_bat_set_temp_control_test(battery, true);
	else
		sec_bat_set_temp_control_test(battery, false);

	/* Check High Voltage charging option for wired charging */
	if (get_afc_mode() == CH_MODE_AFC_DISABLE_VAL) {
		pr_info("HV wired charging mode is disabled\n");
		sec_bat_set_current_event(battery,
			SEC_BAT_CURRENT_EVENT_HV_DISABLE, SEC_BAT_CURRENT_EVENT_HV_DISABLE);
	}

	if(fg_reset)
		sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_FG_RESET,
			SEC_BAT_CURRENT_EVENT_FG_RESET);

	battery->pdata->store_mode_charging_max = STORE_MODE_CHARGING_MAX;
	battery->pdata->store_mode_charging_min = STORE_MODE_CHARGING_MIN;
#if defined(CONFIG_CALC_TIME_TO_FULL)
	battery->timetofull = -1;
#endif

	if (battery->pdata->charger_name == NULL)
		battery->pdata->charger_name = "sec-charger";
	if (battery->pdata->fuelgauge_name == NULL)
		battery->pdata->fuelgauge_name = "sec-fuelgauge";

	/* create work queue */
	battery->monitor_wqueue =
		create_singlethread_workqueue(dev_name(&pdev->dev));
	if (!battery->monitor_wqueue) {
		dev_err(battery->dev,
			"%s: Fail to Create Workqueue\n", __func__);
		goto err_irq;
	}

	sec_bat_get_battery_info(battery);

	INIT_DELAYED_WORK(&battery->monitor_work, sec_bat_monitor_work);
	INIT_DELAYED_WORK(&battery->cable_work, sec_bat_cable_work);
	INIT_DELAYED_WORK(&battery->wpc_tx_work, sec_bat_wpc_tx_work);
	INIT_DELAYED_WORK(&battery->wpc_tx_en_work, sec_bat_wpc_tx_en_work);
#if defined(CONFIG_CALC_TIME_TO_FULL)
	INIT_DELAYED_WORK(&battery->timetofull_work, sec_bat_time_to_full_work);
#endif
#if defined(CONFIG_WIRELESS_TX_MODE)
	INIT_DELAYED_WORK(&battery->wpc_txpower_calc_work, sec_bat_txpower_calc_work);
#endif
#if defined(CONFIG_ENABLE_100MA_CHARGING_BEFORE_USB_CONFIGURED)
	INIT_DELAYED_WORK(&battery->slowcharging_work, sec_bat_check_slowcharging_work);
#endif
	INIT_DELAYED_WORK(&battery->afc_work, sec_bat_afc_work);
	INIT_DELAYED_WORK(&battery->ext_event_work, sec_bat_ext_event_work);
	INIT_DELAYED_WORK(&battery->siop_level_work, sec_bat_siop_level_work);
	INIT_DELAYED_WORK(&battery->wc_headroom_work, sec_bat_wc_headroom_work);
#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	INIT_DELAYED_WORK(&battery->fw_init_work, sec_bat_fw_init_work);
#endif
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	INIT_DELAYED_WORK(&battery->batt_data_work, sec_bat_update_data_work);
#endif
	INIT_DELAYED_WORK(&battery->misc_event_work, sec_bat_misc_event_work);
#ifdef CONFIG_OF
	INIT_DELAYED_WORK(&battery->parse_mode_dt_work, sec_bat_parse_mode_dt_work);
#endif
	INIT_DELAYED_WORK(&battery->init_chg_work, sec_bat_init_chg_work);

	switch (pdata->polling_type) {
	case SEC_BATTERY_MONITOR_WORKQUEUE:
		INIT_DELAYED_WORK(&battery->polling_work,
			sec_bat_polling_work);
		break;
	case SEC_BATTERY_MONITOR_ALARM:
		battery->last_poll_time = ktime_get_boottime();
		alarm_init(&battery->polling_alarm, ALARM_BOOTTIME,
			sec_bat_alarm);
		break;
	default:
		break;
	}

#if defined(CONFIG_BATTERY_CISD)
	sec_battery_cisd_init(battery);
#endif
	/* updates temperatures on boot */
	sec_bat_get_temperature_info(battery);

	battery_cfg.drv_data = battery;

	/* init power supplier framework */
	battery->psy_ps = power_supply_register(&pdev->dev, &ps_power_supply_desc, &battery_cfg);
	if (!battery->psy_ps) {
		dev_err(battery->dev,
			"%s: Failed to Register psy_ps\n", __func__);
		goto err_workqueue;
	}
	battery->psy_ps->supplied_to = supply_list;
	battery->psy_ps->num_supplicants = ARRAY_SIZE(supply_list);

	battery->psy_usb = power_supply_register(&pdev->dev, &usb_power_supply_desc, &battery_cfg);
	if (!battery->psy_usb) {
		dev_err(battery->dev,
			"%s: Failed to Register psy_usb\n", __func__);
		goto err_supply_unreg_ps;
	}
	battery->psy_usb->supplied_to = supply_list;
	battery->psy_usb->num_supplicants = ARRAY_SIZE(supply_list);

	battery->psy_ac = power_supply_register(&pdev->dev, &ac_power_supply_desc, &battery_cfg);
	if (!battery->psy_ac) {
		dev_err(battery->dev,
			"%s: Failed to Register psy_ac\n", __func__);
		goto err_supply_unreg_usb;
	}
	battery->psy_ac->supplied_to = supply_list;
	battery->psy_ac->num_supplicants = ARRAY_SIZE(supply_list);

	battery->psy_bat = power_supply_register(&pdev->dev, &battery_power_supply_desc, &battery_cfg);
	if (!battery->psy_bat) {
		dev_err(battery->dev,
			"%s: Failed to Register psy_bat\n", __func__);
		goto err_supply_unreg_ac;
	}

	battery->psy_wireless = power_supply_register(&pdev->dev, &wireless_power_supply_desc, &battery_cfg);
	if (!battery->psy_wireless) {
		dev_err(battery->dev,
			"%s: Failed to Register psy_wireless\n", __func__);
		goto err_supply_unreg_bat;
	}
	battery->psy_wireless->supplied_to = supply_list;
	battery->psy_wireless->num_supplicants = ARRAY_SIZE(supply_list);

	ret = sec_bat_create_attrs(&battery->psy_bat->dev);
	if (ret) {
		dev_err(battery->dev,
			"%s : Failed to create_attrs\n", __func__);
		goto err_req_irq;
	}

	/* initialize battery level*/
	value.intval = 0;
	psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CAPACITY, value);
	battery->capacity = value.intval;

#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	queue_delayed_work(battery->monitor_wqueue, &battery->fw_init_work, msecs_to_jiffies(2000));
#endif

	/* notify wireless charger driver when sec_battery probe is done,
		if wireless charging is possible, POWER_SUPPLY_PROP_ONLINE of wireless property will be called. */
	value.intval = 0;
	psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_PROP_CHARGE_TYPE, value);

#if defined(CONFIG_STORE_MODE) && !defined(CONFIG_SEC_FACTORY)
	battery->store_mode = true;
	sec_bat_parse_mode_dt(battery);
#endif

#if defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	battery->pdic_info.sink_status.rp_currentlvl = RP_CURRENT_LEVEL_NONE;
	manager_notifier_register(&battery->usb_typec_nb,
		usb_typec_handle_notification, MANAGER_NOTIFY_CCIC_BATTERY);
#else
#if defined(CONFIG_MUIC_NOTIFIER)
	muic_notifier_register(&battery->batt_nb,
       batt_handle_notification, MUIC_NOTIFY_DEV_CHARGER);
#else
	cable_initial_check(battery);
#endif
#if defined(CONFIG_CCIC_NOTIFIER)
	pr_info("%s: Registering PDIC_NOTIFY.\n", __func__);
	pdic_notifier_register(&battery->pdic_nb,
		batt_pdic_handle_notification, PDIC_NOTIFY_DEV_BATTERY);
#endif
#endif
#if defined(CONFIG_VBUS_NOTIFIER)
	vbus_notifier_register(&battery->vbus_nb,
		vbus_handle_notification, VBUS_NOTIFY_DEV_CHARGER);
#endif

#if defined(CONFIG_WIRELESS_AUTH)
	sec_bat_misc_init(battery);
#endif

	value.intval = true;
	psy_do_property(battery->pdata->charger_name, set,
		POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX, value);

	/* make fg_reset true again for actual normal booting after recovery kernel is done */
	if (fg_reset && (bootmode == 2)) {
		psy_do_property(battery->pdata->fuelgauge_name, set,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);
		pr_info("%s: make fg_reset true again for actual normal booting\n", __func__);
	}

	if ((battery->cable_type == SEC_BATTERY_CABLE_NONE) ||
		(battery->cable_type == SEC_BATTERY_CABLE_PREPARE_TA)) {
		queue_delayed_work(battery->monitor_wqueue, &battery->init_chg_work, 0);

		dev_info(&pdev->dev,
				"%s: SEC Battery Driver Monitorwork\n", __func__);
		wake_lock(&battery->monitor_wake_lock);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
	}

	if (battery->pdata->check_battery_callback)
		battery->present = battery->pdata->check_battery_callback();

	dev_info(battery->dev,
		"%s: SEC Battery Driver Loaded\n", __func__);
	return 0;

err_req_irq:
	if (battery->pdata->bat_irq)
		free_irq(battery->pdata->bat_irq, battery);
	power_supply_unregister(battery->psy_wireless);
err_supply_unreg_bat:
	power_supply_unregister(battery->psy_bat);
err_supply_unreg_ac:
	power_supply_unregister(battery->psy_ac);
err_supply_unreg_usb:
	power_supply_unregister(battery->psy_usb);
err_supply_unreg_ps:
	power_supply_unregister(battery->psy_ps);
err_workqueue:
	destroy_workqueue(battery->monitor_wqueue);
err_irq:
	wake_lock_destroy(&battery->monitor_wake_lock);
	wake_lock_destroy(&battery->cable_wake_lock);
	wake_lock_destroy(&battery->vbus_wake_lock);
	wake_lock_destroy(&battery->afc_wake_lock);
	wake_lock_destroy(&battery->siop_level_wake_lock);
	wake_lock_destroy(&battery->ext_event_wake_lock);
	wake_lock_destroy(&battery->wc_headroom_wake_lock);
	wake_lock_destroy(&battery->wpc_tx_wake_lock);
	wake_lock_destroy(&battery->wpc_tx_en_wake_lock);
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	wake_lock_destroy(&battery->batt_data_wake_lock);
#endif
	wake_lock_destroy(&battery->misc_event_wake_lock);
	wake_lock_destroy(&battery->tx_event_wake_lock);
#ifdef CONFIG_OF
	wake_lock_destroy(&battery->parse_mode_dt_wake_lock);
#endif
	mutex_destroy(&battery->adclock);
	mutex_destroy(&battery->iolock);
	mutex_destroy(&battery->misclock);
	mutex_destroy(&battery->txeventlock);
	mutex_destroy(&battery->batt_handlelock);
	mutex_destroy(&battery->current_eventlock);
	mutex_destroy(&battery->typec_notylock);
	mutex_destroy(&battery->wclock);
	mutex_destroy(&battery->voutlock);
	kfree(pdata);
err_bat_free:
	kfree(battery);

	return ret;
}

static int sec_battery_remove(struct platform_device *pdev)
{
	struct sec_battery_info *battery = platform_get_drvdata(pdev);
#ifndef CONFIG_OF
	int i;
#endif

	pr_info("%s: ++\n", __func__);

	switch (battery->pdata->polling_type) {
	case SEC_BATTERY_MONITOR_WORKQUEUE:
		cancel_delayed_work(&battery->polling_work);
		break;
	case SEC_BATTERY_MONITOR_ALARM:
		alarm_cancel(&battery->polling_alarm);
		break;
	default:
		break;
	}

	flush_workqueue(battery->monitor_wqueue);
	destroy_workqueue(battery->monitor_wqueue);
	wake_lock_destroy(&battery->monitor_wake_lock);
	wake_lock_destroy(&battery->cable_wake_lock);
	wake_lock_destroy(&battery->vbus_wake_lock);
	wake_lock_destroy(&battery->afc_wake_lock);
	wake_lock_destroy(&battery->siop_level_wake_lock);
	wake_lock_destroy(&battery->ext_event_wake_lock);
	wake_lock_destroy(&battery->misc_event_wake_lock);
	wake_lock_destroy(&battery->tx_event_wake_lock);
	wake_lock_destroy(&battery->wc_headroom_wake_lock);
	wake_lock_destroy(&battery->wpc_tx_wake_lock);
	wake_lock_destroy(&battery->wpc_tx_en_wake_lock);
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	wake_lock_destroy(&battery->batt_data_wake_lock);
#endif
#ifdef CONFIG_OF
	wake_lock_destroy(&battery->parse_mode_dt_wake_lock);
#endif
	mutex_destroy(&battery->adclock);
	mutex_destroy(&battery->iolock);
	mutex_destroy(&battery->misclock);
	mutex_destroy(&battery->txeventlock);
	mutex_destroy(&battery->batt_handlelock);
	mutex_destroy(&battery->current_eventlock);
	mutex_destroy(&battery->typec_notylock);
	mutex_destroy(&battery->wclock);
	mutex_destroy(&battery->voutlock);

#ifdef CONFIG_OF
	adc_exit(battery);
#else
	for (i = 0; i < SEC_BAT_ADC_CHANNEL_NUM; i++)
		adc_exit(battery->pdata, i);
#endif
	power_supply_unregister(battery->psy_ps);
	power_supply_unregister(battery->psy_wireless);
	power_supply_unregister(battery->psy_ac);
	power_supply_unregister(battery->psy_usb);
	power_supply_unregister(battery->psy_bat);

	kfree(battery);

	pr_info("%s: --\n", __func__);

	return 0;
}

static int sec_battery_prepare(struct device *dev)
{
	struct sec_battery_info *battery
		= dev_get_drvdata(dev);

	dev_info(battery->dev, "%s: Start\n", __func__);

	switch (battery->pdata->polling_type) {
	case SEC_BATTERY_MONITOR_WORKQUEUE:
		cancel_delayed_work(&battery->polling_work);
		break;
	case SEC_BATTERY_MONITOR_ALARM:
		alarm_cancel(&battery->polling_alarm);
		break;
	default:
		break;
	}

	/* monitor_wake_lock should be unlocked before cancle monitor_work */
	wake_unlock(&battery->monitor_wake_lock);
	cancel_delayed_work_sync(&battery->monitor_work);

	battery->polling_in_sleep = true;

	sec_bat_set_polling(battery);

	/* cancel work for polling
	 * that is set in sec_bat_set_polling()
	 * no need for polling in sleep
	 */
	if (battery->pdata->polling_type ==
		SEC_BATTERY_MONITOR_WORKQUEUE)
		cancel_delayed_work(&battery->polling_work);

	dev_info(battery->dev, "%s: End\n", __func__);

	return 0;
}

static int sec_battery_suspend(struct device *dev)
{
	return 0;
}

static int sec_battery_resume(struct device *dev)
{
	return 0;
}

static void sec_battery_complete(struct device *dev)
{
	struct sec_battery_info *battery
		= dev_get_drvdata(dev);

	dev_info(battery->dev, "%s: Start\n", __func__);

	/* cancel current alarm and reset after monitor work */
	if (battery->pdata->polling_type == SEC_BATTERY_MONITOR_ALARM)
		alarm_cancel(&battery->polling_alarm);

	wake_lock(&battery->monitor_wake_lock);
	queue_delayed_work(battery->monitor_wqueue,
		&battery->monitor_work, 0);

	dev_info(battery->dev, "%s: End\n", __func__);

	return;
}

static void sec_battery_shutdown(struct platform_device *pdev)
{
	struct sec_battery_info *battery
		= platform_get_drvdata(pdev);

	pr_info("%s: ++\n", __func__);

	switch (battery->pdata->polling_type) {
	case SEC_BATTERY_MONITOR_WORKQUEUE:
		cancel_delayed_work(&battery->polling_work);
		break;
	case SEC_BATTERY_MONITOR_ALARM:
		alarm_cancel(&battery->polling_alarm);
		break;
	default:
		break;
	}

	pr_info("%s: --\n", __func__);
}

#ifdef CONFIG_OF
static struct of_device_id sec_battery_dt_ids[] = {
	{ .compatible = "samsung,sec-battery" },
	{ }
};
MODULE_DEVICE_TABLE(of, sec_battery_dt_ids);
#endif /* CONFIG_OF */

static const struct dev_pm_ops sec_battery_pm_ops = {
	.prepare = sec_battery_prepare,
	.suspend = sec_battery_suspend,
	.resume = sec_battery_resume,
	.complete = sec_battery_complete,
};

static struct platform_driver sec_battery_driver = {
	.driver = {
		   .name = "sec-battery",
		   .owner = THIS_MODULE,
		   .pm = &sec_battery_pm_ops,
#ifdef CONFIG_OF
		.of_match_table = sec_battery_dt_ids,
#endif
	},
	.probe = sec_battery_probe,
	.remove = sec_battery_remove,
	.shutdown = sec_battery_shutdown,
};

static int __init sec_battery_init(void)
{
	return platform_driver_register(&sec_battery_driver);
}

static void __exit sec_battery_exit(void)
{
	platform_driver_unregister(&sec_battery_driver);
}

late_initcall(sec_battery_init);
module_exit(sec_battery_exit);

MODULE_DESCRIPTION("Samsung Battery Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
