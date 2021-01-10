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
bool dt_need_overwrite = false;

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
	"UNKNOWN",                 /* 0 */
	"NONE",                    /* 1 */
	"PREAPARE_TA",             /* 2 */
	"TA",                      /* 3 */
	"USB",                     /* 4 */
	"USB_CDP",                 /* 5 */
	"9V_TA",                   /* 6 */
	"9V_ERR",                  /* 7 */
	"9V_UNKNOWN",              /* 8 */
	"12V_TA",                  /* 9 */
	"WIRELESS",                /* 10 */
	"HV_WIRELESS",             /* 11 */
	"PMA_WIRELESS",            /* 12 */
	"WIRELESS_PACK",           /* 13 */
	"WIRELESS_HV_PACK",        /* 14 */
	"WIRELESS_STAND",          /* 15 */
	"WIRELESS_HV_STAND",       /* 16 */
	"OC20",                    /* 17 */
	"QC30",                    /* 18 */
	"PDIC",                    /* 19 */
	"UARTOFF",                 /* 20 */
	"OTG",                     /* 21 */
	"LAN_HUB",                 /* 22 */
	"POWER_SHARGING",          /* 23 */
	"HMT_CONNECTED",           /* 24 */
	"HMT_CHARGE",              /* 25 */
	"HV_TA_CHG_LIMIT",          /* 26 */
	"WIRELESS_VEHICLE",          /* 27 */
	"WIRELESS_HV_VEHICLE",	   /* 28 */
	"WIRELESS_HV_PREPARE",	   /* 29 */
	"TIMEOUT",                 /* 30 */
	"SMART_OTG",               /* 31 */
	"SMART_NOTG"               /* 32 */
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
};

char *sec_bat_charge_mode_str[] = {
	"Charging-On",
	"Charging-Off",
	"Buck-Off",
};

extern int bootmode;
bool mfc_fw_update;
EXPORT_SYMBOL(mfc_fw_update);
bool boot_complete;
EXPORT_SYMBOL(boot_complete);

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

	if (temp != battery->tx_event)
		power_supply_changed(battery->psy_bat);

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

static void sec_bat_change_default_current(struct sec_battery_info *battery,
					int cable_type, int input, int output)
{
	battery->pdata->charging_current[cable_type].input_current_limit = input;
	battery->pdata->charging_current[cable_type].fast_charging_current = output;
	pr_info("%s: cable_type: %d input: %d output: %d\n",__func__, cable_type, input, output);
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
	}

	/* 3. WPC_TEMP_MODE */
	if (is_wireless_type(battery->cable_type) && battery->chg_limit) {
		if ((battery->siop_level >= 100) &&
				(incurr > battery->pdata->wpc_charging_limit_current))
			incurr = battery->pdata->wpc_charging_limit_current;
		else if ((battery->siop_level < 100) &&
				(incurr > battery->pdata->wpc_charging_limit_current))
			incurr = battery->pdata->wpc_charging_limit_current;
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
	if (battery->siop_level == 100 &&
		battery->status == POWER_SUPPLY_STATUS_FULL && battery->charging_mode == SEC_BATTERY_CHARGING_NONE) {
		incurr = battery->pdata->wc_full_input_limit_current;
	}

	return incurr;
}

static void sec_bat_get_charging_current_by_siop(struct sec_battery_info *battery,
		int *input_current, int *charging_current) {
	if (battery->siop_level == 3) {
		/* side sync scenario : siop_level 3 */
		if (is_nv_wireless_type(battery->cable_type)) {
			if (*input_current > battery->pdata->siop_wireless_input_limit_current)
				*input_current = battery->pdata->siop_wireless_input_limit_current;
			*charging_current = battery->pdata->siop_wireless_charging_limit_current;
		} else if (is_hv_wireless_type(battery->cable_type)) {
			if (*input_current > battery->pdata->siop_hv_wireless_input_limit_current)
				*input_current = battery->pdata->siop_hv_wireless_input_limit_current;
			*charging_current = battery->pdata->siop_hv_wireless_charging_limit_current;
		} else if (is_hv_wire_type(battery->cable_type)) {
			if (*input_current > 450)
				*input_current = 450;
			*charging_current = battery->pdata->siop_hv_charging_limit_current;
#if defined(CONFIG_CCIC_NOTIFIER)
		} else if (battery->cable_type == SEC_BATTERY_CABLE_PDIC) {
			if (*input_current > (4000 / battery->input_voltage))
				*input_current = 4000 / battery->input_voltage;
			*charging_current = battery->pdata->siop_hv_charging_limit_current;
#endif
		} else {
			if (*input_current > 800)
				*input_current = 800;
			*charging_current = battery->pdata->charging_current[
				battery->cable_type].fast_charging_current;
			if (*charging_current > battery->pdata->siop_charging_limit_current)
				*charging_current = battery->pdata->siop_charging_limit_current;
		}
	} else if (battery->siop_level == 5) {
		/* special senario : calling or browsing during wired charging */
		if (is_hv_wire_type(battery->cable_type) &&
		    !(battery->current_event & SEC_BAT_CURRENT_EVENT_CHG_LIMIT)) {
			if (battery->cable_type == SEC_BATTERY_CABLE_12V_TA)
				*input_current = 440;
			else
				*input_current = 600;
			*charging_current = 900;
		} else if (battery->cable_type == SEC_BATTERY_CABLE_TA ||
			   (battery->current_event & SEC_BAT_CURRENT_EVENT_CHG_LIMIT)) {
			*input_current = 1000;
			*charging_current = 900;
		}
	} else if (battery->siop_level < 100) {
		int max_charging_current;

		if (is_wireless_type(battery->cable_type)) {
			max_charging_current = 1000; /* 1 step(70) */
			if (battery->siop_level == 0) { /* 3 step(0) */
				max_charging_current = 0;
			} else if (battery->siop_level <= 10) { /* 2 step(10) */
				max_charging_current = 500;
			}
		} else {
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
			}
#if defined(CONFIG_CCIC_NOTIFIER)
		} else if (battery->cable_type == SEC_BATTERY_CABLE_PDIC) {
			if (*input_current > (6000 / battery->input_voltage))
				*input_current = 6000 / battery->input_voltage;
#endif
		} else {
			if (*input_current > battery->pdata->siop_input_limit_current)
				*input_current = battery->pdata->siop_input_limit_current;
		}
	}

	pr_info("%s: incurr(%d), chgcurr(%d)\n", __func__, *input_current, *charging_current);
}

extern int muic_afc_set_voltage(int vol);

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
	if (battery->pdata->chg_temp_check_type && battery->siop_level >= 100 && is_not_wireless_type(battery->cable_type)) {
		if ((!battery->mix_limit &&
				(battery->temperature >= battery->pdata->mix_high_temp) &&
				(battery->chg_temp >= battery->pdata->mix_high_chg_temp)) ||
			(battery->mix_limit &&
				(battery->temperature > battery->pdata->mix_high_temp_recovery))) {
			int max_input_current =
				battery->pdata->full_check_current_1st + 50;

			/* inpu current = float voltage * (topoff_current_1st + 50mA(margin)) / (vbus_level * 0.9) */
			input_current = ((battery->pdata->chg_float_voltage / battery->pdata->chg_float_voltage_conv) * max_input_current) /
				(battery->input_voltage * 90) / 10;
			if (input_current > max_input_current)
				input_current = max_input_current;

			battery->mix_limit = true;
			/* skip other heating control */
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL,
						  SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL);
		} else if (battery->mix_limit) {
			battery->mix_limit = false;
		}

		pr_info("%s: mix_limit(%d), temp(%d), chg_temp(%d), input_current(%d)\n",
			__func__, battery->mix_limit, battery->temperature, battery->chg_temp, input_current);
	} else {
		battery->mix_limit = false;
	}
	return input_current;
}

static int sec_bat_check_wpc_temp(struct sec_battery_info *battery, int input_current)
{
	if (is_wireless_type(battery->cable_type)) {
		int wpc_vout_level = WIRELESS_VOUT_10V;

		mutex_lock(&battery->voutlock);
		if (battery->siop_level >= 100) {
			int temp_val = sec_bat_get_temp_by_temp_control_source(battery,
				battery->pdata->wpc_temp_control_source);

			if ((!battery->chg_limit && temp_val >= battery->pdata->wpc_high_temp) ||
				(battery->chg_limit && temp_val > battery->pdata->wpc_high_temp_recovery)) {
				battery->chg_limit = true;
				input_current = battery->pdata->wpc_charging_limit_current;
				wpc_vout_level = WIRELESS_VOUT_5_5V_STEP;
			} else if (battery->chg_limit) {
				battery->chg_limit = false;
			}
		} else {
			if ((is_hv_wireless_type(battery->cable_type) &&
				battery->cable_type != SEC_BATTERY_CABLE_WIRELESS_HV_VEHICLE) ||
				battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV) {
				int temp_val = sec_bat_get_temp_by_temp_control_source(battery,
					battery->pdata->wpc_temp_lcd_on_control_source);

				if ((!battery->chg_limit &&
						temp_val >= battery->pdata->wpc_lcd_on_high_temp) ||
					(battery->chg_limit &&
						temp_val > battery->pdata->wpc_lcd_on_high_temp_rec)) {
					input_current = battery->pdata->wpc_charging_limit_current;
					battery->chg_limit = true;
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
			if (battery->current_event & SEC_BAT_CURRENT_EVENT_SWELLING_MODE)
				wpc_vout_level = WIRELESS_VOUT_5_5V_STEP;

			if (wpc_vout_level != battery->wpc_vout_level) {
				battery->wpc_vout_level = wpc_vout_level;
				if (battery->current_event & SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK) {
					pr_info("%s: block to set wpc vout level(%d) because otg on\n",
						__func__, wpc_vout_level);
				} else {
					union power_supply_propval value = {0, };

					value.intval = wpc_vout_level;
					psy_do_property(battery->pdata->wireless_charger_name, set,
						POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
					pr_info("%s: change vout level(%d)",
						__func__, battery->wpc_vout_level);
					battery->aicl_current = 0; /* reset aicl current */
				}
			} else if (battery->wpc_vout_level == WIRELESS_VOUT_10V && !battery->chg_limit)
				/* reset aicl current to recover current for unexpected aicl during before vout boosting completion */
				battery->aicl_current = 0;
		}
		mutex_unlock(&battery->voutlock);
		pr_info("%s: change input_current(%d), vout_level(%d), chg_limit(%d)\n",
			__func__, input_current, battery->wpc_vout_level, battery->chg_limit);
	}

	return input_current;
}

static bool sec_bat_change_vbus(struct sec_battery_info *battery, int *input_current)
{
#if defined(CONFIG_MUIC_HV) || defined(CONFIG_SUPPORT_QC30) || defined(CONFIG_SUPPORT_HV_CTRL)
	union power_supply_propval value;
	unsigned int target_vbus = SEC_INPUT_VOLTAGE_0V;

	if (battery->store_mode)
		return false;

	if (is_hv_wire_type(battery->cable_type) &&
		(battery->cable_type != SEC_BATTERY_CABLE_QC30)) {

		if (battery->current_event & SEC_BAT_CURRENT_EVENT_AFC) {
			pr_info("%s: skip during current_event(0x%x)\n",
				__func__, battery->current_event);
			return false;
		}

		/* check target vbus */
		if (battery->vbus_limit)
			target_vbus = SEC_INPUT_VOLTAGE_0V;
		else if (battery->vbus_chg_by_full)
			target_vbus = SEC_INPUT_VOLTAGE_5V;
		else if (battery->siop_level >= 100) {
			if (is_hv_wire_12v_type(battery->cable_type))
				target_vbus = SEC_INPUT_VOLTAGE_12V;
			else
				target_vbus = SEC_INPUT_VOLTAGE_9V;
		} else if (battery->status == POWER_SUPPLY_STATUS_CHARGING)
			target_vbus = SEC_INPUT_VOLTAGE_5V;

		if (target_vbus == SEC_INPUT_VOLTAGE_0V) {
			pr_info("%s: skip set vbus %dV, level(%d), Cable(%s, %s, %d, %d)\n",
				__func__, target_vbus, battery->siop_level,
				sec_cable_type[battery->cable_type], sec_cable_type[battery->select->cable_type],
				battery->select->muic_cable_type, battery->pd_usb_attached);

			return false;
		}

		if (battery->vbus_chg_by_siop != target_vbus) {
			/* change input current to pre_afc_input_current */
			*input_current = battery->pdata->pre_afc_input_current;
			battery->charge_power = battery->input_voltage * (*input_current);
			value.intval = *input_current;
			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_PROP_CURRENT_MAX, value);
			battery->input_current = *input_current;

			/* set current event */
			cancel_delayed_work(&battery->afc_work);
			wake_unlock(&battery->afc_wake_lock);
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_AFC,
						  (SEC_BAT_CURRENT_EVENT_CHG_LIMIT | SEC_BAT_CURRENT_EVENT_AFC));

			mutex_lock(&battery->voutlock);
			if ((battery->charging_port == MAIN_PORT) &&
				(battery->current_event & SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK)) {
				target_vbus = SEC_INPUT_VOLTAGE_5V;
			}
			battery->chg_limit = false;
			battery->vbus_chg_by_siop = target_vbus;
			muic_afc_set_voltage(target_vbus);
			mutex_unlock(&battery->voutlock);

			pr_info("%s: vbus set %dV by level(%d), Cable(%s, %s, %d, %d)\n",
				__func__, target_vbus, battery->siop_level,
				sec_cable_type[battery->cable_type], sec_cable_type[battery->select->cable_type],
				battery->select->muic_cable_type, battery->pd_usb_attached);
			return true;
		}
	}
#endif
	return false;
}

static void sec_bat_check_afc_temp(struct sec_battery_info *battery, int *input_current, int *charging_current)
{
#if defined(CONFIG_MUIC_HV) || defined(CONFIG_SUPPORT_QC30) || defined(CONFIG_SUPPORT_HV_CTRL)
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
static int sec_bat_check_pdic_temp(struct sec_battery_info *battery, int input_current)
{
	if (battery->select->pdic_ps_rdy && battery->siop_level >= 100) {
		struct sec_bat_pdic_list *pd_list = &battery->select->pd_list;
		int pd_index = pd_list->now_pd_index;

		if (!battery->chg_limit) {
			if (battery->chg_temp >= battery->pdata->chg_high_temp) {
				battery->chg_limit = true;
				pd_index--;
			} else
				pd_index++;
		} else {
			if (battery->chg_temp <= battery->pdata->chg_high_temp_recovery)
				battery->chg_limit = false;
			else if (battery->chg_temp >= battery->pdata->chg_high_temp)
				pd_index--;
		}

		if (pd_index < 0) {
			if (battery->chg_limit) {
				input_current = (input_current > (battery->pdata->nv_charge_power / (pd_list->pd_info[0].input_voltage / 1000))) ?
					(battery->pdata->nv_charge_power / (pd_list->pd_info[0].input_voltage / 1000)) : input_current;
				pd_index = -1;
			} else {
				pd_index = 0;
			}
			pd_list->now_pd_index = pd_index;
		} else {
			pd_index =
				(pd_index >= pd_list->max_pd_count) ? (pd_list->max_pd_count - 1) : pd_index;

			if (pd_list->now_pd_index != pd_index) {
				/* change input current before request new pdo
				 * if new pdo's input current is less than now
				 */
				if (pd_list->pd_info[pd_index].input_current < input_current) {
					union power_supply_propval value = {0, };

					input_current = pd_list->pd_info[pd_index].input_current;
					value.intval = input_current;
					battery->input_current = input_current;
					sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SELECT_PDO,
						SEC_BAT_CURRENT_EVENT_SELECT_PDO);
					psy_do_property(battery->pdata->charger_name, set,
						POWER_SUPPLY_PROP_CURRENT_MAX, value);
				}
				/* select next pdo */
				battery->select->pdic_ps_rdy = false;
				select_pdo(pd_list->pd_info[pd_index].pdo_index);
				pr_info("%s: change pd_list - index: %d, pdo_index: %d\n",
					__func__, pd_index, pd_list->pd_info[pd_index].pdo_index);
			}
		}
		pr_info("%s: pd_index(%d), input_current(%d), chg_limit(%d)\n",
			__func__, pd_index, input_current, battery->chg_limit);
	}

	return input_current;
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
	int pdo_num = battery->select->pdic_info.sink_status.current_pdo_num;
	int max_input_current = 0;

	max_input_current = battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC].input_current_limit =
		battery->select->pdic_info.sink_status.power_list[pdo_num].max_current;

	pr_info("%s:max_input_current : %dmA\n", __func__, max_input_current);
}

static void sec_bat_get_charging_current_in_power_list(struct sec_battery_info *battery)
{
	int max_charging_current = 0;
	int pdo_num = battery->select->pdic_info.sink_status.current_pdo_num;
	int pd_power = (battery->select->pdic_info.sink_status.power_list[pdo_num].max_voltage *
		battery->select->pdic_info.sink_status.power_list[pdo_num].max_current);

	/* We assume that output voltage to float voltage */
	max_charging_current = pd_power / (battery->pdata->chg_float_voltage / battery->pdata->chg_float_voltage_conv);
	max_charging_current = max_charging_current > battery->pdata->max_charging_current ?
		battery->pdata->max_charging_current : max_charging_current;
	battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC].fast_charging_current = max_charging_current;
	battery->charge_power = pd_power;

	pr_info("%s:pd_charge_power : %dmW, max_charging_current : %dmA\n", __func__,
		battery->charge_power, max_charging_current);
}
#endif

int sec_bat_set_charging_current(struct sec_battery_info *battery)
{
	static int afc_init = false;
	union power_supply_propval value = {0, };
	int input_current = battery->pdata->charging_current[battery->cable_type].input_current_limit,
		charging_current = battery->pdata->charging_current[battery->cable_type].fast_charging_current,
		topoff_current = battery->pdata->full_check_current_1st;
#if !defined(CONFIG_SEC_FACTORY)
	int temp = 0;
#endif

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
			if (is_wireless_type(battery->cable_type) && battery->pdata->wpc_temp_check_type) {
				temp = sec_bat_check_wpc_temp(battery, input_current);
				if (input_current > temp)
					input_current = temp;
			}
#if defined(CONFIG_CCIC_NOTIFIER)
			else if (battery->cable_type == SEC_BATTERY_CABLE_PDIC && battery->pdata->chg_temp_check_type) {
				input_current = sec_bat_check_pdic_temp(battery, input_current);
				input_current = sec_bat_check_pd_input_current(battery, input_current);
			}
#endif
			else if (battery->pdata->chg_temp_check_type) {
				if (!sec_bat_change_vbus(battery, &input_current))
					sec_bat_check_afc_temp(battery, &input_current, &charging_current);
			}
		}
#endif

		input_current = sec_bat_check_afc_input_current(battery, input_current);
		/* Set limited max current with hv wire cable when store mode is set and LDU
			Limited max current should be set with over 5% capacity since target could be turned off during boot up */
		if (battery->store_mode && is_hv_wire_type(battery->select->cable_type) && (battery->capacity >= 5)) {
			input_current = battery->pdata->store_mode_afc_input_current;
		}

		sec_bat_get_charging_current_by_siop(battery, &input_current, &charging_current);

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
			}
		} else {
			if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND) {
				charging_current = (charging_current > battery->pdata->swelling_low_temp_current_2nd) ?
					battery->pdata->swelling_low_temp_current_2nd : charging_current;
				topoff_current = (topoff_current > battery->pdata->swelling_low_temp_topoff) ?
					battery->pdata->swelling_low_temp_topoff : topoff_current;
			} else if (battery->current_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING) {
				charging_current = (charging_current > battery->pdata->swelling_high_temp_current) ?
					battery->pdata->swelling_high_temp_current : charging_current;
				topoff_current = (topoff_current > battery->pdata->swelling_high_temp_topoff) ?
					battery->pdata->swelling_high_temp_topoff : topoff_current;
			} else if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING) {
				charging_current = (charging_current > battery->pdata->swelling_low_temp_current) ?
					battery->pdata->swelling_low_temp_current : charging_current;
			}

#if defined(CONFIG_ENABLE_100MA_CHARGING_BEFORE_USB_CONFIGURED)
		/* usb unconfigured or suspend*/
		if ((battery->cable_type == SEC_BATTERY_CABLE_USB) && !lpcharge &&
			(battery->pdic_info.sink_status.rp_currentlvl == RP_CURRENT_LEVEL_DEFAULT)) {
			if (battery->current_event & SEC_BAT_CURRENT_EVENT_USB_100MA) {
				pr_info("%s: usb unconfigured\n", __func__);
				input_current = USB_CURRENT_UNCONFIGURED;
				charging_current = USB_CURRENT_UNCONFIGURED;
			}
		}
#endif
		}
	}

	/* In wireless charging, must be set charging current before input current. */
	if (is_wireless_type(battery->cable_type) &&
		battery->charging_current != charging_current) {
		value.intval = charging_current;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_AVG, value);
		battery->charging_current = charging_current;
	}
	/* set input current, charging current */
	if ((battery->input_current != input_current) ||
		(battery->charging_current != charging_current)) {
		/* update charge power */
		battery->charge_power = battery->input_voltage * input_current;
		if (battery->charge_power > battery->max_charge_power)
			battery->max_charge_power = battery->charge_power;

		value.intval = input_current;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_MAX, value);
		battery->input_current = input_current;

		value.intval = charging_current;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_NOW, value);

		if (charging_current <= 100)
			battery->charging_current = 100;
		else
			battery->charging_current = charging_current;
		pr_info("%s: power(%d), input(%d), charge(%d)\n", __func__,
			battery->charge_power, battery->input_current, battery->charging_current);
	}

	/* set topoff current */
	if (battery->topoff_current != topoff_current) {
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
	mutex_unlock(&battery->iolock);
	return 0;
}

int sec_bat_set_charge(struct sec_battery_info *battery,
			int chg_mode)
{
	union power_supply_propval val = {0, };
	ktime_t current_time = {0, };
	struct timespec ts = {0, };

	if (battery->cable_type == SEC_BATTERY_CABLE_HMT_CONNECTED)
		return 0;

	if ((battery->current_event & SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE) &&
		(chg_mode == SEC_BAT_CHG_MODE_CHARGING)) {
		dev_info(battery->dev, "%s: charge disable by HMT\n", __func__);
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
		if(is_nocharge_type(battery->cable_type))
			ret = battery->present;
		else
			ret = sec_bat_check_vf_adc(battery);
		break;
	case SEC_BATTERY_CHECK_INT:
	case SEC_BATTERY_CHECK_CALLBACK:
		if(is_nocharge_type(battery->cable_type)) {
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
		if (battery->siop_level != 100)
			battery->stop_timer = true;
		break;
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
	case POWER_SUPPLY_STATUS_DISCHARGING:
		if ((battery->status == POWER_SUPPLY_STATUS_FULL ||
		     (battery->capacity == 100 && !is_slate_mode(battery))) &&
		    !battery->store_mode) {
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
		if (is_wireless_type(battery->cable_type)) {
			bool send_cs100_cmd = true;

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

	if (battery->health != POWER_SUPPLY_HEALTH_GOOD &&
		battery->health != POWER_SUPPLY_HEALTH_OVERVOLTAGE &&
		battery->health != POWER_SUPPLY_HEALTH_UNDERVOLTAGE) {
		dev_dbg(battery->dev, "%s: No need to check\n", __func__);
		return false;
	}

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
				battery->pdata->chg_float_voltage_conv) - 150;
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

	if (battery->status == POWER_SUPPLY_STATUS_DISCHARGING) {
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
#if defined(CONFIG_BATTERY_SWELLING)
	    (battery->charging_mode == SEC_BATTERY_CHARGING_2ND ||
	     battery->is_recharging || battery->swelling_mode)) {
#else
		(battery->charging_mode == SEC_BATTERY_CHARGING_2ND ||
		 battery->is_recharging)) {
#endif
		value.intval = 0;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CAPACITY, value);
		if (value.intval <
			battery->pdata->full_condition_soc &&
				battery->voltage_now <
				(battery->pdata->recharge_condition_vcell - 50)) {
			sec_bat_set_charging_status(battery,
					POWER_SUPPLY_STATUS_CHARGING);
			dev_info(battery->dev,
				"%s: battery status full -> charging, RepSOC(%d)\n", __func__, value.intval);
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

	if (is_wireless_type(battery->cable_type)) {
		swelling_high_recovery = battery->pdata->swelling_wc_high_temp_recov;
	}
	pr_info("%s: swelling highblock(%d), highrecov(%d)\n", __func__, battery->pdata->swelling_high_temp_block, swelling_high_recovery);

	psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_PROP_VOLTAGE_MAX, val);

	pr_info("%s: status(%d), swell_mode(%d:%d:%d), cv(%d)mV, temp(%d)\n",
		__func__, battery->status, battery->swelling_mode,
		battery->charging_block, (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE),
		val.intval, battery->temperature);

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
		if (((battery->temperature >= battery->pdata->swelling_high_temp_block) ||
			(battery->temperature <= battery->pdata->swelling_low_temp_block_2nd)) &&
			battery->pdata->temp_check_type) {
			pr_info("%s: swelling mode start. stop charging\n", __func__);
			battery->swelling_mode = SWELLING_MODE_CHARGING;
			battery->swelling_full_check_cnt = 0;
			sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_BUCK_OFF);

			if (battery->temperature >= battery->pdata->swelling_high_temp_block) {
#if defined(CONFIG_BATTERY_CISD)
				battery->cisd.data[CISD_DATA_HIGH_TEMP_SWELLING]++;
				battery->cisd.data[CISD_DATA_HIGH_TEMP_SWELLING_PER_DAY]++;
#endif
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING,
					SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
			} else if (battery->temperature <= battery->pdata->swelling_low_temp_block_2nd) {
#if defined(CONFIG_BATTERY_CISD)
				battery->cisd.data[CISD_DATA_LOW_TEMP_SWELLING]++;
				battery->cisd.data[CISD_DATA_LOW_TEMP_SWELLING_PER_DAY]++;
#endif
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND,
					SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
			}
			en_swelling = true;
		} else if ((battery->temperature <= battery->pdata->swelling_low_temp_block_1st) &&
			!(battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING)) {
			pr_info("%s: low temperature reduce current\n", __func__);
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING,
						  SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
		} else if ((battery->temperature >= battery->pdata->swelling_low_temp_recov_1st) &&
			(battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING)) {
			pr_info("%s: normal temperature temperature recover current\n", __func__);
			sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING);
		}
	}

	if (!battery->voltage_now)
		return;

	if (battery->swelling_mode) {
		if (battery->temperature <= battery->pdata->swelling_low_temp_recov_2nd) {
			swelling_rechg_voltage = battery->pdata->swelling_low_rechg_voltage;
		}

		if ((battery->temperature <= swelling_high_recovery) &&
		    (battery->temperature >= battery->pdata->swelling_low_temp_recov_2nd)) {
			pr_info("%s: swelling mode end. restart charging\n", __func__);
			battery->swelling_mode = SWELLING_MODE_NONE;
			battery->charging_mode = SEC_BATTERY_CHARGING_1ST;
			if ((battery->temperature <= battery->pdata->swelling_low_temp_block_1st) ||
				((battery->temperature < battery->pdata->swelling_low_temp_recov_1st) &&
				(battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING))) {
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING,
							SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
			} else {
				sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
			}
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
			if (battery->temperature < battery->pdata->swelling_low_temp_recov_2nd) {
				pr_info("%s: swelling mode reduce charging current(LOW-temp:%d)\n",
					__func__, battery->temperature);
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND,
							  SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
			} else if (battery->temperature > swelling_high_recovery) {
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
		value.intval = battery->pdata->chg_float_voltage;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_VOLTAGE_MAX, value);
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
	union power_supply_propval value;
	battery_health_condition state;
	int i, battery_health;

	/* check to support ASoC and Cycle */
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_ENERGY_FULL, value);
#if !defined(CONFIG_BATTERY_AGE_FORECAST)
	value.intval = -1;
#endif
	if (value.intval <= 0 || battery->pdata->health_condition == NULL) {
		pr_err("%s: does not support cycle or asoc or health_condition\n", __func__);
		return;
	}
	/* Checking Cycle and ASoC */
	state.cycle = state.asoc = BATTERY_HEALTH_BAD;
	for (i = BATTERY_HEALTH_MAX - 1; i >= 0; i--) {
		if (battery->pdata->health_condition[i].cycle >= (battery->batt_cycle % 10000))
			state.cycle = i + BATTERY_HEALTH_GOOD;
		if (battery->pdata->health_condition[i].asoc <= battery->batt_asoc)
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

	if (is_wireless_type(battery->cable_type)) {
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

	if (battery->temp_highlimit_cnt >=
	    battery->pdata->temp_check_count) {
		battery->health = POWER_SUPPLY_HEALTH_OVERHEATLIMIT;
		battery->temp_highlimit_cnt = 0;
	} else if (battery->temp_high_cnt >=
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
	if (pre_health != battery->health) {
		battery->health_change = true;
		dev_info(battery->dev, "%s, health_change true\n", __func__);
	} else {
		battery->health_change = false;
	}

	if ((battery->health == POWER_SUPPLY_HEALTH_OVERHEAT) ||
		(battery->health == POWER_SUPPLY_HEALTH_COLD) ||
		(battery->health == POWER_SUPPLY_HEALTH_OVERHEATLIMIT)) {
		union power_supply_propval val = {0, };
		if (battery->health_change) {
			battery->is_abnormal_temp = true;
			if (is_wireless_type(battery->cable_type)) {
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
#if defined(CONFIG_MUIC_HV) || defined(CONFIG_SUPPORT_QC30) || defined(CONFIG_SUPPORT_HV_CTRL)
					battery->vbus_chg_by_siop = SEC_INPUT_VOLTAGE_0V;
					muic_afc_set_voltage(SEC_INPUT_VOLTAGE_0V);
#endif
					battery->vbus_limit = true;
					pr_info("%s: Set AFC TA to 0V\n", __func__);
				}
			} else if (battery->health == POWER_SUPPLY_HEALTH_OVERHEAT) {
				/* to discharge battery */
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_BUCK_OFF);
			} else {
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
			}

			psy_do_property(battery->pdata->charger_name, get,
					POWER_SUPPLY_PROP_VOLTAGE_MAX, val);
			if (val.intval != battery->pdata->swelling_drop_float_voltage) {
				val.intval = battery->pdata->swelling_drop_float_voltage;
				psy_do_property(battery->pdata->charger_name, set,
						POWER_SUPPLY_PROP_VOLTAGE_MAX, val);

			}
			return false;
		}
		/* dose not need buck control at low temperature */
		if (battery->health == POWER_SUPPLY_HEALTH_OVERHEATLIMIT ||
			battery->health == POWER_SUPPLY_HEALTH_OVERHEAT) {
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
			(battery->status ==
			 POWER_SUPPLY_STATUS_NOT_CHARGING)) {
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
			if ((battery->temperature > battery->pdata->swelling_high_temp_recov) ||
				(battery->temperature < battery->pdata->swelling_low_temp_recov_2nd)) {
				pr_info("%s: swelling mode start. stop charging\n", __func__);
				battery->swelling_mode = SWELLING_MODE_CHARGING;
				battery->swelling_full_check_cnt = 0;
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_BUCK_OFF);
				if (battery->temperature > battery->pdata->swelling_high_temp_recov) {
					sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING,
							SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
				} else if (battery->temperature < battery->pdata->swelling_low_temp_recov_2nd) {
					sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND,
							SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
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

				if (battery->temperature <= battery->pdata->swelling_low_temp_block_1st ||
					((battery->temperature < battery->pdata->swelling_low_temp_recov_1st) &&
					(pre_health == POWER_SUPPLY_HEALTH_COLD))) {
					sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING,
								SEC_BAT_CURRENT_EVENT_SWELLING_MODE);
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

	if (battery->full_check_cnt >=
		battery->pdata->full_check_count) {
		battery->full_check_cnt = 0;
		ret = true;
	}

not_full_charged:
	return ret;
}

static void sec_bat_do_fullcharged(struct sec_battery_info *battery) {
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
		battery->pdata->full_check_type_2nd != SEC_BATTERY_FULLCHARGED_NONE) {
		battery->charging_mode = SEC_BATTERY_CHARGING_2ND;
		battery->charging_fullcharged_time = battery->charging_passed_time;
		value.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;
		psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
		sec_bat_set_charging_current(battery);
		sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING);
	} else {
		battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
		battery->is_recharging = false;

		if (!battery->wdt_kick_disable) {
			pr_info("%s: wdt kick enable -> Charger Off, %d\n",
					__func__, battery->wdt_kick_disable);
			sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
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
			muic_afc_set_voltage(SEC_INPUT_VOLTAGE_5V);
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
			battery->full_check_cnt = battery->pdata->full_check_count;
		} else {
			sec_bat_do_fullcharged(battery);
		}

		/* update capacity max */
		value.intval = battery->capacity;
		psy_do_property(battery->pdata->fuelgauge_name, set,
				POWER_SUPPLY_PROP_CHARGE_FULL, value);
		pr_info("%s : forced full-charged sequence for the capacity(%d)\n",
				__func__, battery->capacity);
	}

	dev_info(battery->dev,
		"%s: Charging Mode : %s\n", __func__,
		battery->is_recharging ?
		sec_bat_charging_mode_str[SEC_BATTERY_CHARGING_RECHARGING] :
		sec_bat_charging_mode_str[battery->charging_mode]);

	return true;
}

static void sec_bat_get_temperature_info(struct sec_battery_info *battery) {
	union power_supply_propval value = {0, };

	/* get battery thm info */
	switch (battery->pdata->thermal_source) {
	case SEC_BATTERY_THERMAL_SOURCE_FG:
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_TEMP, value);
		battery->temperature = value.intval;

		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_TEMP_AMBIENT, value);
		battery->temper_amb = value.intval;
		break;
	case SEC_BATTERY_THERMAL_SOURCE_CALLBACK:
		if (battery->pdata->get_temperature_callback) {
			battery->pdata->get_temperature_callback(
				POWER_SUPPLY_PROP_TEMP, &value);
			battery->temperature = value.intval;
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
			battery->temperature = value.intval;
			battery->temper_amb = value.intval;
		} else {
			battery->temperature = 0;
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
			battery->usb_temp = value.intval;

			/* this shoud be moved */
			if (battery->vbus_limit && battery->usb_temp <= battery->temp_highlimit_recovery)
				battery->vbus_limit = false;
		} else
			battery->usb_temp = 0;
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
			battery->chg_temp = value.intval;
		} else
			battery->chg_temp = 0;
		break;
	default:
		break;
	}

	/* get wpc thm info */
	switch (battery->pdata->wpc_thermal_source) {
	case SEC_BATTERY_THERMAL_SOURCE_FG:
	case SEC_BATTERY_THERMAL_SOURCE_CALLBACK:
		break;
	case SEC_BATTERY_THERMAL_SOURCE_ADC:
		if(sec_bat_get_value_by_adc(battery,
			SEC_BAT_ADC_CHANNEL_WPC_TEMP, &value, battery->pdata->wpc_temp_check_type)) {
			battery->wpc_temp = value.intval;
		} else
			battery->wpc_temp = 0;
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
			battery->slave_chg_temp = value.intval;

			/* set temperature */
			value.intval = ((battery->slave_chg_temp) << 16) | (battery->chg_temp);
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_TEMP, value);
		} else
			battery->slave_chg_temp = 0;
		break;
	default:
		break;
	}

#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	if (battery->temperature_test_battery > -300 && battery->temperature_test_battery < 3000) {
		pr_info("%s : battery temperature test %d\n", __func__, battery->temperature_test_battery);
		battery->temperature = battery->temperature_test_battery;
	}
	if (battery->temperature_test_usb > -300 && battery->temperature_test_usb < 3000) {
		pr_info("%s : usb temperature test %d\n", __func__, battery->temperature_test_usb);
		battery->usb_temp = battery->temperature_test_usb;
	}
	if (battery->temperature_test_wpc > -300 && battery->temperature_test_wpc < 3000) {
		pr_info("%s : wpc temperature test %d\n", __func__, battery->temperature_test_wpc);
		battery->wpc_temp = battery->temperature_test_wpc;
	}
	if (battery->temperature_test_chg > -300 && battery->temperature_test_chg < 3000) {
		pr_info("%s : chg temperature test %d\n", __func__, battery->temperature_test_chg);
		battery->chg_temp = battery->temperature_test_chg;
	}
#endif

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

	/* input current limit in charger */
	psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_PROP_CURRENT_MAX, value);
	battery->current_max = value.intval;

	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_CHARGE_COUNTER, value);
	battery->charge_counter = value.intval;

	/* check abnormal status for wireless charging */
	if (!(battery->current_event & SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL) &&
		is_wireless_type(battery->cable_type)) {
		value.intval = (battery->status == POWER_SUPPLY_STATUS_FULL) ?
			100 : battery->capacity;
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);
	}

	sec_bat_get_temperature_info(battery);

	/* To get SOC value (NOT raw SOC), need to reset value */
	value.intval = 0;
	psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CAPACITY, value);
	/* if the battery status was full, and SOC wasn't 100% yet,
		then ignore FG SOC, and report (previous SOC +1)% */
	battery->capacity = value.intval;

	dev_info(battery->dev,
		"%s:Vnow(%dmV),Vavg(%dmV),Inow(%dmA),Imax(%dmA),Ichg(%dmA),SOC(%d%%),"
		"Tbat(%d),Tusb(%d),Tchg(%d),Twpc(%d)\n", __func__,
		battery->voltage_now, battery->voltage_avg, battery->current_now,
		battery->current_max, battery->charging_current,
		battery->capacity, battery->temperature,
		battery->usb_temp, battery->chg_temp, battery->wpc_temp
	);
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

#if defined(CONFIG_CALC_TIME_TO_FULL)
static void sec_bat_calc_time_to_full(struct sec_battery_info * battery)
{
	if (delayed_work_pending(&battery->timetofull_work)) {
		pr_info("%s: keep time_to_full(%5d sec)\n", __func__, battery->timetofull);
	} else if (battery->status == POWER_SUPPLY_STATUS_CHARGING ||
		(battery->status == POWER_SUPPLY_STATUS_FULL && battery->capacity != 100)) {
		union power_supply_propval value = {0, };
		int charge = 0;

		if (is_hv_wire_12v_type(battery->cable_type) ||
				battery->max_charge_power >= (battery->pdata->pd_charging_charge_power + 5000)) { /* 20000mW */
			charge = battery->pdata->ttf_hv_12v_charge_current;
		} else if (is_hv_wire_type(battery->cable_type) ||
			   /* if max_charge_power could support over than max_charging_current, calculate based on ttf_hv_charge_current */
			   battery->max_charge_power >= (battery->pdata->max_charging_current * 5)) {
			charge = battery->pdata->ttf_hv_charge_current;
		} else if (is_hv_wireless_type(battery->cable_type) ||
				battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV) {
			if (sec_bat_hv_wc_normal_mode_check(battery))
				charge = battery->pdata->ttf_wireless_charge_current;
			else
				charge = battery->pdata->ttf_hv_wireless_charge_current;
		} else if (is_nv_wireless_type(battery->cable_type)) {
			charge = battery->pdata->ttf_wireless_charge_current;
		} else {
			charge = battery->max_charge_power / 5;
		}
		value.intval = charge;
		psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_TIME_TO_FULL_NOW, value);
		dev_info(battery->dev, "%s: T: %5d sec, passed time: %5ld, current: %d\n",
				__func__, value.intval, battery->charging_passed_time, charge);
		battery->timetofull = value.intval;
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
			sec_bat_set_misc_event(battery, BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE, BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE);
			battery->max_charge_power = battery->input_voltage * battery->current_max;
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
			battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_STAND)) {
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

static void sec_bat_siop_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, siop_work.work);

	pr_info("%s : set current by siop level(%d)\n",__func__, battery->siop_level);

	sec_bat_set_charging_current(battery);
	wake_unlock(&battery->siop_wake_lock);
}

static void sec_bat_siop_level_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
			struct sec_battery_info, siop_level_work.work);

	queue_delayed_work(battery->monitor_wqueue, &battery->siop_work, 0);

	wake_lock(&battery->siop_wake_lock);
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

#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
void sec_bat_fw_update_work(struct sec_battery_info *battery, int mode)
{
	union power_supply_propval value = {0, };

	dev_info(battery->dev, "%s \n", __func__);

	wake_lock_timeout(&battery->vbus_wake_lock, HZ * 10);

	switch (mode) {
		case SEC_WIRELESS_RX_SDCARD_MODE:
		case SEC_WIRELESS_RX_BUILT_IN_MODE:
			value.intval = mode;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_PROP_CHARGE_POWERED_OTG_CONTROL, value);
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
	int uno_status = 0, wpc_det = 0;

	dev_info(battery->dev, "%s \n", __func__);

	wpc_det = gpio_get_value(battery->pdata->wpc_det);

	pr_info("%s wpc_det = %d \n", __func__, wpc_det);

	psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL, value);
	uno_status = value.intval;
	pr_info("%s uno = %d \n", __func__, uno_status);

	if (!uno_status && !wpc_det) {
		pr_info("%s uno on \n", __func__);
		value.intval = true;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL, value);
	}

	value.intval = SEC_WIRELESS_RX_INIT;
	psy_do_property(battery->pdata->wireless_charger_name, set,
		POWER_SUPPLY_PROP_CHARGE_POWERED_OTG_CONTROL, value);

	if (!uno_status && !wpc_det) {
		pr_info("%s uno off \n", __func__);
		value.intval = false;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL, value);
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
		BATT_MISC_EVENT_HICCUP_TYPE)) &&
		is_nocharge_type(battery->cable_type)) {
		if (battery->misc_event & (BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE |
			BATT_MISC_EVENT_HICCUP_TYPE)) {
			sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_BUCK_OFF);
		} else if (battery->prev_misc_event & (BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE |
			BATT_MISC_EVENT_HICCUP_TYPE)) {
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
	int input_power = battery->current_max * battery->input_voltage * 1000;
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
	} else if ((battery->siop_level < 100) && battery->stop_timer) {
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

	if ((battery->siop_level < 100) && !battery->stop_timer) {
		battery->stop_timer = true;
	} else if ((battery->siop_level >= 100) && battery->stop_timer) {
		battery->stop_timer = false;
	}

	pr_info("%s : EXPIRED_TIME(%llu), IP(%d), CP(%d), CURR(%d), STANDARD(%d)\n",
		__func__, expired_time, input_power, charging_power, curr, battery->pdata->standard_curr);

	if (curr == 0)
		return;

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

static void sec_bat_monitor_work(
				struct work_struct *work)
{
	struct sec_battery_info *battery =
		container_of(work, struct sec_battery_info,
		monitor_work.work);
	static struct timespec old_ts = {0, };
	struct timespec c_ts = {0, };
	union power_supply_propval val = {0, };

	dev_dbg(battery->dev, "%s: Start\n", __func__);
	c_ts = ktime_to_timespec(ktime_get_boottime());


	mutex_lock(&battery->wclock);
	if (!battery->wc_enable) {
		pr_info("%s: wc_enable(%d), cnt(%d)\n",
			__func__, battery->wc_enable, battery->wc_enable_cnt);
		if (battery->wc_enable_cnt > battery->wc_enable_cnt_value) {
			battery->wc_enable = true;
			battery->wc_enable_cnt = 0;
			if (battery->pdata->wpc_en) {
				gpio_direction_output(battery->pdata->wpc_en, 0);
				pr_info("%s: WC CONTROL: Enable", __func__);
			}
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
			(battery->ps_enable != true)) {
			if ((unsigned long)(c_ts.tv_sec - old_ts.tv_sec) < 10 * 60) {
				union power_supply_propval value = {0, };

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
	sec_bat_swelling_check(battery);

	if ((battery->swelling_mode == SWELLING_MODE_CHARGING || battery->swelling_mode == SWELLING_MODE_FULL) &&
		(!battery->charging_block))
		sec_bat_swelling_fullcharged_check(battery);
	else
		sec_bat_fullcharged_check(battery);
#else
	/* 5. full charging check */
	sec_bat_fullcharged_check(battery);
#endif /* CONFIG_BATTERY_SWELLING */

	/* 6. additional check */
	if (battery->pdata->monitor_additional_check)
		battery->pdata->monitor_additional_check();

	if ((battery->cable_type == SEC_BATTERY_CABLE_WIRELESS ||
		battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_STAND) &&
		!battery->wc_cv_mode && battery->charging_passed_time > 10)
		sec_bat_wc_cv_mode_check(battery);

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
	
	dev_dbg(battery->dev,
		"%s: HLT(%d) HLR(%d) HT(%d), HR(%d), LT(%d), LR(%d), lpcharge(%d)\n",
		__func__, battery->temp_highlimit_threshold, battery->temp_highlimit_recovery,
		battery->temp_high_threshold, battery->temp_high_recovery,
		battery->temp_low_threshold, battery->temp_low_recovery, lpcharge);

	dev_info(battery->dev,
		 "%s: Status(%s), mode(%s), Health(%s), Cable(%s, %s, %s, %d, %d, %d), Port(%d), level(%d%%), slate_mode(%d), store_mode(%d)"
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
		 sec_cable_type[battery->main->cable_type],
		 sec_cable_type[battery->sub->cable_type],
		 battery->sub->muic_cable_type,
		 battery->main->muic_cable_type,
		 battery->pd_usb_attached,
		 battery->charging_port,
		 battery->siop_level,
		 is_slate_mode(battery),
		 battery->store_mode
#if defined(CONFIG_AFC_CHARGER_MODE)
		, battery->select->hv_chg_name, battery->vbus_chg_by_siop, sleep_mode
#endif
#if defined(CONFIG_BATTERY_AGE_FORECAST)
		, battery->batt_cycle
#endif
		 );
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	dev_info(battery->dev,
			"%s: battery->stability_test(%d), battery->eng_not_full_status(%d)\n",
			__func__, battery->stability_test, battery->eng_not_full_status);
#endif
#if defined(CONFIG_SEC_FACTORY)
	if (!is_nocharge_type(battery->cable_type) && (battery->cable_type != SEC_BATTERY_CABLE_OTG)) {
#else
	if (!is_nocharge_type(battery->cable_type) && (battery->cable_type != SEC_BATTERY_CABLE_OTG) && battery->store_mode) {
#endif
		dev_info(battery->dev,
			 "%s: @battery->capacity = (%d), battery->status= (%d), battery->store_mode=(%d)\n",
			 __func__, battery->capacity, battery->status, battery->store_mode);

		if (battery->capacity >= STORE_MODE_CHARGING_MAX) {
			int chg_mode = battery->misc_event &
				(BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE | BATT_MISC_EVENT_HICCUP_TYPE) ?
					SEC_BAT_CHG_MODE_BUCK_OFF : SEC_BAT_CHG_MODE_CHARGING_OFF;
			/* to discharge the battery, off buck */
			if (battery->capacity > STORE_MODE_CHARGING_MAX)
				chg_mode = SEC_BAT_CHG_MODE_BUCK_OFF;

			sec_bat_set_charging_status(battery,
						    POWER_SUPPLY_STATUS_DISCHARGING);
			sec_bat_set_charge(battery, chg_mode);
		}

		if ((battery->capacity <= STORE_MODE_CHARGING_MIN) && (battery->status == POWER_SUPPLY_STATUS_DISCHARGING)) {
			sec_bat_set_charging_status(battery,
						    POWER_SUPPLY_STATUS_CHARGING);
			sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING);
		}
	}
	power_supply_changed(battery->psy_bat);

skip_monitor:
	sec_bat_set_polling(battery);

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

static void sec_bat_check_input_voltage(struct sec_battery_info *battery, struct cable_info *cable_info)
{
	unsigned int voltage = 0;
	int input_current = battery->pdata->charging_current[cable_info->cable_type].input_current_limit;

	if (cable_info->cable_type == SEC_BATTERY_CABLE_PDIC) {
		cable_info->charge_power = cable_info->pd_max_charge_power;
		return;
	}
	else if (is_hv_wire_12v_type(cable_info->cable_type))
		voltage = SEC_INPUT_VOLTAGE_12V;
	else if (is_hv_wire_9v_type(cable_info->cable_type))
		voltage = SEC_INPUT_VOLTAGE_9V;
	else
		voltage = SEC_INPUT_VOLTAGE_5V;

	cable_info->input_voltage = voltage;
	cable_info->charge_power = voltage * input_current;
#if !defined(CONFIG_SEC_FACTORY)
	if (cable_info->charge_power > cable_info->max_charge_power)
#endif
	cable_info->max_charge_power = cable_info->charge_power;

	pr_info("%s: battery->input_voltage : %dV, %dmW, %dmW)\n", __func__,
		cable_info->input_voltage, cable_info->charge_power, cable_info->max_charge_power);
}

static void sec_bat_cable_info_clear(struct cable_info *cable_info)
{
	cable_info->input_current = 0;
	cable_info->charging_current = 0;
	cable_info->input_voltage = 0;
	cable_info->charge_power = 0;
	cable_info->max_charge_power = 0;
	cable_info->pd_max_charge_power = 0;
	cable_info->pdic_attach = 0;
	cable_info->pdic_ps_rdy = 0;
	cable_info->pd_usb_attached = 0;
	cable_info->hv_chg_name = "NULL";
}

static void sec_bat_cable_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, cable_work.work);
	union power_supply_propval val = {0, };
	struct cable_info *current_cable_info;
	int current_charging_port = PORT_NONE;

	dev_info(battery->dev, "%s: Start\n", __func__);
	sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL,
				  SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL);

	if ((battery->main->cable_type == SEC_BATTERY_CABLE_NONE) && (battery->sub->cable_type != SEC_BATTERY_CABLE_NONE)) {
		current_cable_info = battery->sub;
		current_charging_port = SUB_PORT;
	} else if ((battery->main->cable_type != SEC_BATTERY_CABLE_NONE) && (battery->sub->cable_type == SEC_BATTERY_CABLE_NONE)) {
		current_cable_info = battery->main;
		current_charging_port = MAIN_PORT;
	} else if ((battery->main->cable_type != SEC_BATTERY_CABLE_NONE) && (battery->sub->cable_type != SEC_BATTERY_CABLE_NONE)) {
		if (battery->main->charge_power >= battery->sub->charge_power) {
			current_cable_info = battery->main;
			current_charging_port = MAIN_PORT;
		} else {
			current_cable_info = battery->sub;
			current_charging_port = SUB_PORT;
		}
	} else {
		current_cable_info = battery->main;
		current_charging_port = PORT_NONE;
	}
	/* set charging path */
	val.intval = current_charging_port;
	psy_do_property(battery->pdata->charger_name, set,
		POWER_SUPPLY_EXT_PROP_CHGINSEL, val);

#if defined(CONFIG_CCIC_NOTIFIER)
	if (battery->cable_type == SEC_BATTERY_CABLE_PDIC) {
		if (battery->select->pdic_info.sink_status.selected_pdo_num ==
			battery->select->pdic_info.sink_status.current_pdo_num)
			sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SELECT_PDO);
		sec_bat_get_input_current_in_power_list(battery);
		sec_bat_get_charging_current_in_power_list(battery);
	}
#endif

	if (current_cable_info->cable_type == SEC_BATTERY_CABLE_PDIC &&
		battery->cable_type == SEC_BATTERY_CABLE_PDIC &&
		current_charging_port == battery->charging_port &&
		!is_slate_mode(battery)) {
		cancel_delayed_work(&battery->afc_work);
		wake_unlock(&battery->afc_wake_lock);
		sec_bat_set_current_event(battery, 0,
			SEC_BAT_CURRENT_EVENT_AFC | SEC_BAT_CURRENT_EVENT_AICL);
		battery->aicl_current = 0;
		sec_bat_set_charging_current(battery);
		power_supply_changed(battery->psy_bat);
		goto end_of_cable_work;
	}

	if ((current_cable_info->cable_type == battery->cable_type) && (current_charging_port == battery->charging_port) && !is_slate_mode(battery)) {
		dev_info(battery->dev,
				"%s: Cable is NOT Changed(%d)\n",
				__func__, battery->cable_type);
		/* Do NOT activate cable work for NOT changed */
		goto end_of_cable_work;
	}

#if defined(CONFIG_BATTERY_SWELLING)
	if ((current_cable_info->cable_type == SEC_BATTERY_CABLE_NONE) ||
		(battery->select->cable_type == SEC_BATTERY_CABLE_NONE && battery->swelling_mode == SWELLING_MODE_NONE) ||
		(battery->select->cable_type == SEC_BATTERY_CABLE_OTG && battery->swelling_mode == SWELLING_MODE_NONE)) {
		battery->swelling_mode = SWELLING_MODE_NONE;
		/* restore 4.4V float voltage */
		val.intval = battery->pdata->swelling_normal_float_voltage;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_VOLTAGE_MAX, val);
		pr_info("%s: float voltage = %d\n", __func__, val.intval);
	} else {
		pr_info("%s: skip  float_voltage setting, swelling_mode(%d)\n",
			__func__, battery->swelling_mode);
	}
#endif

	if (current_cable_info->cable_type == SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT)
		current_cable_info->cable_type = SEC_BATTERY_CABLE_9V_TA;	   

	battery->select = current_cable_info;
	battery->cable_type = battery->wire_status = battery->select->cable_type;
	battery->charging_port = current_charging_port;
	battery->pdic_ps_rdy = battery->select->pdic_ps_rdy;

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
		battery->vbus_chg_by_siop = SEC_INPUT_VOLTAGE_0V;
		battery->vbus_chg_by_full = false;
		battery->is_recharging = false;
#if defined(CONFIG_BATTERY_CISD)
		battery->cisd.ab_vbat_check_count = 0;
		battery->cisd.state &= ~CISD_STATE_OVER_VOLTAGE;
#endif
		battery->input_voltage = 0;
		battery->charge_power = 0;
		battery->max_charge_power = 0;
		battery->pd_max_charge_power = 0;
		sec_bat_set_charging_status(battery,
				POWER_SUPPLY_STATUS_DISCHARGING);
		battery->chg_limit = false;
		battery->mix_limit = false;
		battery->chg_limit_recovery_cable = SEC_BATTERY_CABLE_NONE;
		battery->wc_heating_start_time = 0;
		battery->health = POWER_SUPPLY_HEALTH_GOOD;
		cancel_delayed_work(&battery->afc_work);
		wake_unlock(&battery->afc_wake_lock);
		sec_bat_change_default_current(battery, SEC_BATTERY_CABLE_USB,
			battery->pdata->default_usb_input_current,
			battery->pdata->default_usb_charging_current);
		sec_bat_change_default_current(battery, SEC_BATTERY_CABLE_TA,
			battery->pdata->default_input_current,
			battery->pdata->default_charging_current);
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
					   SEC_BAT_CURRENT_EVENT_SELECT_PDO));

#if defined(CONFIG_ENABLE_100MA_CHARGING_BEFORE_USB_CONFIGURED)
		cancel_delayed_work(&battery->slowcharging_work);
#endif
		battery->wc_cv_mode = false;
		battery->is_sysovlo = false;
		battery->is_vbatovlo = false;
		battery->is_abnormal_temp = false;

		sec_bat_cable_info_clear(battery->select);

		sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
	} else if(is_slate_mode(battery)) {
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

		if (battery->cable_type == SEC_BATTERY_CABLE_TA) {
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
	if (is_nocharge_type(battery->cable_type))
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_MAX, val);
	if (battery->status != POWER_SUPPLY_STATUS_DISCHARGING) {
		sec_bat_check_input_voltage(battery, battery->select);

		battery->input_voltage = battery->select->input_voltage;
		battery->charge_power = battery->select->charge_power;
		battery->max_charge_power = battery->select->charge_power;
	}
	sec_bat_set_charging_current(battery);

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
		if ((is_hv_wire_type(battery->cable_type) || battery->cable_type == SEC_BATTERY_CABLE_TA) &&
		     battery->current_max >= battery->pdata->pre_afc_input_current) {
			sec_bat_set_charging_current(battery);
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
			sec_bat_do_fullcharged(battery);
		sec_bat_set_charging_status(battery, val->intval);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		sec_bat_ovp_uvlo_result(battery, val->intval);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		current_cable_type = val->intval;
#if !defined(CONFIG_CCIC_NOTIFIER)
		if ((battery->select->muic_cable_type != ATTACHED_DEV_SMARTDOCK_TA_MUIC)
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
		}

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
			battery->max_charge_power = battery->charge_power = battery->input_voltage * val->intval;
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
			if (battery->select->pdic_info.sink_status.rp_currentlvl > RP_CURRENT_LEVEL_DEFAULT)
				return 0;
			pr_info("%s: usb configured %d\n", __func__, val->intval);
			if (val->intval == USB_CURRENT_UNCONFIGURED) {
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_USB_100MA,
							  (SEC_BAT_CURRENT_EVENT_USB_100MA | SEC_BAT_CURRENT_EVENT_USB_SUPER));
			} else if (val->intval == USB_CURRENT_HIGH_SPEED) {
				sec_bat_set_misc_event(battery, 0, BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE);
				sec_bat_set_current_event(battery, 0,
							  (SEC_BAT_CURRENT_EVENT_USB_100MA | SEC_BAT_CURRENT_EVENT_USB_SUPER));
				sec_bat_change_default_current(battery, SEC_BATTERY_CABLE_USB,
						battery->pdata->default_usb_input_current,
						battery->pdata->default_usb_charging_current);
			} else if (val->intval == USB_CURRENT_SUPER_SPEED) {
				sec_bat_set_misc_event(battery, 0, BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE);
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
			sec_bat_set_current_event(battery, (val->intval == CH_MODE_AFC_DISABLE_VAL ?
				SEC_BAT_CURRENT_EVENT_HV_DISABLE : 0), SEC_BAT_CURRENT_EVENT_HV_DISABLE);
			break;
		case POWER_SUPPLY_EXT_PROP_WC_CONTROL:
			pr_info("%s: Recover MFC IC (wc_enable: %d)\n",
				__func__, battery->wc_enable);

			if (battery->pdata->wpc_en) {
				mutex_lock(&battery->wclock);
				if (battery->wc_enable) {
					gpio_direction_output(battery->pdata->wpc_en, 1);
					msleep(500);
					gpio_direction_output(battery->pdata->wpc_en, 0);
				}
				mutex_unlock(&battery->wclock);
			}
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
		if (battery->health >= POWER_SUPPLY_HEALTH_MAX)
			val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
		else
			val->intval = battery->health;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = battery->present;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		if (is_hv_wireless_type(battery->cable_type) ||
			(battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV)) {
			if (sec_bat_hv_wc_normal_mode_check(battery))
				val->intval = SEC_BATTERY_CABLE_WIRELESS;
			else
				val->intval = SEC_BATTERY_CABLE_HV_WIRELESS_ETX;
		}
		else if(battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_PACK)
			val->intval = SEC_BATTERY_CABLE_WIRELESS;
		else if(battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_HV_PACK)
			val->intval = SEC_BATTERY_CABLE_WIRELESS;
		else if(battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_STAND)
			val->intval = SEC_BATTERY_CABLE_WIRELESS;
		else if(battery->cable_type == SEC_BATTERY_CABLE_PMA_WIRELESS)
			val->intval = SEC_BATTERY_CABLE_WIRELESS;
		else if(battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_VEHICLE)
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
		val->intval = battery->current_now;
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = battery->current_avg;
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
			val->intval = -1;
			break;
		}

		if (((battery->status == POWER_SUPPLY_STATUS_CHARGING) ||
			(battery->status == POWER_SUPPLY_STATUS_FULL && battery->capacity != 100)) &&
			!battery->swelling_mode)
			val->intval = battery->timetofull;
		else
			val->intval = -1;
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
		val->intval = battery->select->cable_type;
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
		case POWER_SUPPLY_EXT_PROP_CHARGE_PORT:
			val->intval = battery->charging_port;
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
	switch (battery->select->cable_type) {
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
				if (battery->misc_event & BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE) {
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
		val->intval = is_wireless_type(battery->cable_type) ?
			1 : 0;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = (battery->pdata->wireless_charger_name) ?
			1 : 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
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
		/* Clear the FOD State */
		sec_bat_set_misc_event(battery, 0, BATT_MISC_EVENT_WIRELESS_FOD);

		battery->wc_status = val->intval;

		wake_lock(&battery->cable_wake_lock);
		queue_delayed_work(battery->monitor_wqueue,
			&battery->cable_work, 0);
		if (battery->wc_status == SEC_WIRELESS_PAD_NONE ||
			battery->wc_status == SEC_WIRELESS_PAD_WPC_PACK ||
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
		mutex_lock(&battery->voutlock);
		if (val->intval) {
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK,
				SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK);
		} else {
			sec_bat_set_current_event(battery, 0,
				SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK);
		}

		if ((battery->charging_port == MAIN_PORT) &&
			is_hv_wire_type(battery->cable_type)) {
			int target_vbus = SEC_INPUT_VOLTAGE_0V;

			target_vbus = (val->intval) ?
				SEC_INPUT_VOLTAGE_5V :
				((is_hv_wire_12v_type(battery->cable_type) ?
					SEC_INPUT_VOLTAGE_12V : SEC_INPUT_VOLTAGE_9V));

			if (battery->vbus_chg_by_siop != target_vbus) {
				if (target_vbus == SEC_INPUT_VOLTAGE_5V) {
					battery->vbus_chg_by_siop = target_vbus;
					muic_afc_set_voltage(target_vbus);
				}
				cancel_delayed_work(&battery->siop_work);
				wake_lock(&battery->siop_level_wake_lock);
				queue_delayed_work(battery->monitor_wqueue,
					&battery->siop_level_work, 0);
			}
		}
		mutex_unlock(&battery->voutlock);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		battery->aicl_current = 0; /* reset aicl current */
		pr_info("%s: reset aicl\n", __func__);
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
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
	case ATTACHED_DEV_AFC_CHARGER_9V_MUIC:
	case ATTACHED_DEV_QC_CHARGER_9V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_9V_DUPLI_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_9V_TA;
		break;
#if defined(CONFIG_MUIC_HV_12V)
	case ATTACHED_DEV_AFC_CHARGER_12V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_12V_DUPLI_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_12V_TA;
		break;
#endif
	case ATTACHED_DEV_AFC_CHARGER_ERR_V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_ERR_V_DUPLI_MUIC:
	case ATTACHED_DEV_QC_CHARGER_ERR_V_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_9V_ERR;
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

static void sec_bat_set_rp_current(struct sec_battery_info *battery, struct cable_info *cable_info, int cable_type)
{
	if (cable_info->pdic_info.sink_status.rp_currentlvl == RP_CURRENT_ABNORMAL) {
		sec_bat_change_default_current(battery, cable_type,
			battery->pdata->rp_current_abnormal_rp3, battery->pdata->rp_current_abnormal_rp3);
	} else if (cable_info->pdic_info.sink_status.rp_currentlvl == RP_CURRENT_LEVEL3) {
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
	} else if (cable_info->pdic_info.sink_status.rp_currentlvl == RP_CURRENT_LEVEL2) {
		sec_bat_change_default_current(battery, cable_type,
			battery->pdata->rp_current_rp2, battery->pdata->rp_current_rp2);
	} else if (cable_info->pdic_info.sink_status.rp_currentlvl == RP_CURRENT_LEVEL_DEFAULT) {
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

	pr_info("%s:(%d)\n", __func__, cable_info->pdic_info.sink_status.rp_currentlvl);
	battery->max_charge_power = 0;
	if (battery->status != POWER_SUPPLY_STATUS_DISCHARGING)
		sec_bat_check_input_voltage(battery, cable_info);
	sec_bat_set_charging_current(battery);
}
#endif

static int make_pd_list(struct sec_battery_info *battery, struct cable_info *cable_info)
{
	int i = 0, j = 0, min = 0, temp_voltage = 0, temp_current = 0, temp_index = 0;
	int base_charge_power = 0, selected_pdo_voltage = 0, selected_pdo_num = 0;
	int pd_list_index = 0;
	int pd_charging_charge_power = battery->current_event & SEC_BAT_CURRENT_EVENT_HV_DISABLE ?
		battery->pdata->nv_charge_power : battery->pdata->pd_charging_charge_power;

	for (base_charge_power = pd_charging_charge_power * 1000;
	    base_charge_power >= 1000000; base_charge_power -= 1000000)
	{
		selected_pdo_voltage = battery->pdata->max_input_voltage + 1;
		selected_pdo_num = 0;
		for(i=1; i<= cable_info->pdic_info.sink_status.available_pdo_num; i++)
		{
			if (cable_info->pdic_info.sink_status.power_list[i].max_voltage *
			    cable_info->pdic_info.sink_status.power_list[i].max_current > base_charge_power - 1000000 &&
			    cable_info->pdic_info.sink_status.power_list[i].max_voltage *
			    cable_info->pdic_info.sink_status.power_list[i].max_current <= base_charge_power)
			{
				if (cable_info->pdic_info.sink_status.power_list[i].max_voltage < selected_pdo_voltage)
				{
					selected_pdo_voltage = cable_info->pdic_info.sink_status.power_list[i].max_voltage;
					selected_pdo_num = i;
				}
			}
		}
		if (selected_pdo_num)
		{
			cable_info->pd_list.pd_info[pd_list_index].input_voltage =
				cable_info->pdic_info.sink_status.power_list[selected_pdo_num].max_voltage;
			cable_info->pd_list.pd_info[pd_list_index].input_current =
				cable_info->pdic_info.sink_status.power_list[selected_pdo_num].max_current;
			cable_info->pd_list.pd_info[pd_list_index].pdo_index = selected_pdo_num;
			pd_list_index++;
		}
	}
	pr_info("%s: total pd_list_index: %d\n", __func__, pd_list_index);
	if (pd_list_index <= 0) {
		pr_info("%s : PDO list is empty!!\n", __func__);
		return 0;
	}

	for (i = 0; i < pd_list_index - 1; i++) {
		min = i;
		for (j = i + 1; j < pd_list_index; j++) {
			if (cable_info->pd_list.pd_info[j].input_voltage <
				cable_info->pd_list.pd_info[min].input_voltage)
				min = j;
		}

		if(min != i) {
			temp_voltage = cable_info->pd_list.pd_info[i].input_voltage;
			cable_info->pd_list.pd_info[i].input_voltage =
				cable_info->pd_list.pd_info[min].input_voltage;
			cable_info->pd_list.pd_info[min].input_voltage = temp_voltage;
			temp_current = cable_info->pd_list.pd_info[i].input_current;
			cable_info->pd_list.pd_info[i].input_current =
				cable_info->pd_list.pd_info[min].input_current;
			cable_info->pd_list.pd_info[min].input_current = temp_current;
			temp_index = cable_info->pd_list.pd_info[i].pdo_index;
			cable_info->pd_list.pd_info[i].pdo_index =
				cable_info->pd_list.pd_info[min].pdo_index;
			cable_info->pd_list.pd_info[min].pdo_index = temp_index;
		}
	}
	for(i = 0; i < pd_list_index; i++) {
		pr_info("%s: Made pd_list[%d], voltage : %d, current : %d, index : %d\n", __func__, i,
		cable_info->pd_list.pd_info[i].input_voltage,
		cable_info->pd_list.pd_info[i].input_current,
		cable_info->pd_list.pd_info[i].pdo_index);
	}
	cable_info->pd_list.max_pd_count = pd_list_index;
	cable_info->max_charge_power = cable_info->pdic_info.sink_status.power_list[ \
		cable_info->pd_list.pd_info[pd_list_index-1].pdo_index].max_voltage * \
		cable_info->pdic_info.sink_status.power_list[cable_info->pd_list.pd_info[ \
		pd_list_index-1].pdo_index].max_current / 1000;
	cable_info->pd_max_charge_power = cable_info->max_charge_power;

	if (cable_info->pdic_info.sink_status.selected_pdo_num == cable_info->pd_list.pd_info[pd_list_index-1].pdo_index) {
		cable_info->pd_list.now_pd_index = pd_list_index - 1;
		cable_info->pdic_ps_rdy = true;
		dev_info(battery->dev, "%s: battery->pdic_ps_rdy(%d)\n", __func__, cable_info->pdic_ps_rdy);
	} else {
		/* change input current before request new pdo if new pdo's input current is less than now */
		if (cable_info->pd_list.pd_info[pd_list_index-1].input_current < battery->input_current) {
			union power_supply_propval value = {0, };
			int input_current = cable_info->pd_list.pd_info[pd_list_index-1].input_current;

			value.intval = input_current;
			battery->input_current = input_current;
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SELECT_PDO,
				SEC_BAT_CURRENT_EVENT_SELECT_PDO);
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_CURRENT_MAX, value);
		}
		cable_info->pdic_ps_rdy = false;
		select_pdo(cable_info->pd_list.pd_info[pd_list_index-1].pdo_index);
	}

	return cable_info->pd_list.max_pd_count;
}

static int usb_typec_handle_notification(struct notifier_block *nb,
		unsigned long action, void *data)
{
	const char *cmd;
	struct sec_battery_info *battery =
			container_of(nb, struct sec_battery_info, usb_typec_nb);
	int cable_type = SEC_BATTERY_CABLE_NONE, i = 0, current_pdo = 0;
	int pd_charging_charge_power = battery->current_event & SEC_BAT_CURRENT_EVENT_HV_DISABLE ?
		battery->pdata->nv_charge_power : battery->pdata->pd_charging_charge_power;
	CC_NOTI_ATTACH_TYPEDEF usb_typec_info = *(CC_NOTI_ATTACH_TYPEDEF *)data;

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
			battery->sub->pd_usb_attached = false;
			cable_type = SEC_BATTERY_CABLE_NONE;
			battery->sub->muic_cable_type = ATTACHED_DEV_NONE_MUIC;
			battery->sub->pdic_info.sink_status.rp_currentlvl = RP_CURRENT_LEVEL_NONE;
			break;
		case MUIC_NOTIFY_CMD_ATTACH:
		case MUIC_NOTIFY_CMD_LOGICALLY_ATTACH:
			/* Skip notify from MUIC if PDIC is attached already */
			if (battery->sub->cable_type == SEC_BATTERY_CABLE_PDIC) {
				mutex_unlock(&battery->typec_notylock);
				return 0;
			}
			cmd = "ATTACH";
			battery->sub->muic_cable_type = usb_typec_info.cable_type;
			cable_type = sec_bat_cable_check(battery, battery->sub->muic_cable_type);
			if (battery->sub->cable_type != cable_type &&
				battery->sub->pdic_info.sink_status.rp_currentlvl >= RP_CURRENT_LEVEL_DEFAULT &&
				(cable_type == SEC_BATTERY_CABLE_USB || cable_type == SEC_BATTERY_CABLE_TA)) {
				sec_bat_set_rp_current(battery, battery->sub, cable_type);
			} else if ((struct pdic_notifier_struct *)usb_typec_info.pd != NULL &&
				(*(struct pdic_notifier_struct *)usb_typec_info.pd).event == PDIC_NOTIFY_EVENT_CCIC_ATTACH &&
				(*(struct pdic_notifier_struct *)usb_typec_info.pd).sink_status.rp_currentlvl >= RP_CURRENT_LEVEL_DEFAULT &&
				(cable_type == SEC_BATTERY_CABLE_USB || cable_type == SEC_BATTERY_CABLE_TA)) {
				battery->sub->pdic_info.sink_status.rp_currentlvl =
					(*(struct pdic_notifier_struct *)usb_typec_info.pd).sink_status.rp_currentlvl;
				sec_bat_set_rp_current(battery, battery->sub, cable_type);
			}
			break;
		default:
			cmd = "ERROR";
			cable_type = -1;
			battery->sub->muic_cable_type = usb_typec_info.cable_type;
			break;
		}
		battery->sub->pdic_attach = false;
		battery->sub->pdic_ps_rdy = false;
#if defined(CONFIG_AFC_CHARGER_MODE)
		if (battery->sub->muic_cable_type == ATTACHED_DEV_QC_CHARGER_9V_MUIC ||
			battery->sub->muic_cable_type == ATTACHED_DEV_QC_CHARGER_ERR_V_MUIC)
			battery->sub->hv_chg_name = "QC";
		else if (battery->sub->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_9V_MUIC ||
			battery->sub->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_9V_DUPLI_MUIC ||
			battery->sub->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_ERR_V_MUIC ||
			battery->sub->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_ERR_V_DUPLI_MUIC)
			battery->sub->hv_chg_name = "AFC";
#if defined(CONFIG_MUIC_HV_12V)
		else if (battery->sub->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_12V_MUIC ||
			battery->sub->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_12V_DUPLI_MUIC)
			battery->sub->hv_chg_name = "12V";
#endif
		else
			battery->sub->hv_chg_name = "NONE";
#endif
		break;
	case CCIC_NOTIFY_ID_POWER_STATUS:
#ifdef CONFIG_SEC_FACTORY
		dev_info(battery->dev, "%s: pd_event(%d)\n", __func__,
			(*(struct pdic_notifier_struct *)usb_typec_info.pd).event);
#endif
		if ((*(struct pdic_notifier_struct *)usb_typec_info.pd).event == PDIC_NOTIFY_EVENT_DETACH) {
			dev_info(battery->dev, "%s: skip pd operation - attach(%d)\n", __func__, usb_typec_info.attach);
			battery->sub->pdic_attach = false;
			battery->sub->pdic_ps_rdy = false;
			battery->sub->pd_list.now_pd_index = 0;
			mutex_unlock(&battery->typec_notylock);
			return 0;
		} else if ((*(struct pdic_notifier_struct *)usb_typec_info.pd).event == PDIC_NOTIFY_EVENT_PD_PRSWAP_SNKTOSRC) {
			cmd = "PD_PRWAP";
			dev_info(battery->dev, "%s: PRSWAP_SNKTOSRC(%d)\n", __func__, usb_typec_info.attach);
			cable_type = SEC_BATTERY_CABLE_NONE;

			battery->sub->pdic_attach = false;
			battery->sub->pdic_ps_rdy = false;
			battery->sub->pd_list.now_pd_index = 0;
			goto skip_cable_check;
		}

		cmd = "PD_ATTACH";
		if ((*(struct pdic_notifier_struct *)usb_typec_info.pd).event == PDIC_NOTIFY_EVENT_CCIC_ATTACH) {
			battery->sub->pdic_info.sink_status.rp_currentlvl =
				(*(struct pdic_notifier_struct *)usb_typec_info.pd).sink_status.rp_currentlvl;
			dev_info(battery->dev, "%s: battery->rp_currentlvl(%d)\n", __func__, battery->sub->pdic_info.sink_status.rp_currentlvl);
			if (battery->sub->cable_type == SEC_BATTERY_CABLE_USB || battery->sub->cable_type == SEC_BATTERY_CABLE_TA) {
				cable_type = battery->sub->cable_type;
				battery->chg_limit = false;
				sec_bat_set_rp_current(battery, battery->sub, cable_type);
				goto skip_cable_check;
			}
			mutex_unlock(&battery->typec_notylock);
			return 0;
		}
		if ((*(struct pdic_notifier_struct *)usb_typec_info.pd).event == PDIC_NOTIFY_EVENT_PD_SINK_CAP)
			battery->sub->pdic_attach = false;
		if (!battery->sub->pdic_attach) {
			battery->sub->pdic_info = *(struct pdic_notifier_struct *)usb_typec_info.pd;
			battery->sub->pd_list.now_pd_index = 0;
		} else {
			battery->sub->pdic_info.sink_status.selected_pdo_num =
				(*(struct pdic_notifier_struct *)usb_typec_info.pd).sink_status.selected_pdo_num;
			battery->sub->pdic_info.sink_status.current_pdo_num =
				(*(struct pdic_notifier_struct *)usb_typec_info.pd).sink_status.current_pdo_num;
			battery->sub->pd_list.now_pd_index = sec_bat_get_pd_list_index(&battery->sub->pdic_info.sink_status,
				&battery->sub->pd_list);
			battery->sub->pdic_ps_rdy = true;
			dev_info(battery->dev, "%s: battery->pdic_ps_rdy(%d)\n", __func__, battery->sub->pdic_ps_rdy);
		}
		current_pdo = battery->sub->pdic_info.sink_status.current_pdo_num;
		cable_type = SEC_BATTERY_CABLE_PDIC;
		battery->sub->muic_cable_type = ATTACHED_DEV_NONE_MUIC;
#if defined(CONFIG_AFC_CHARGER_MODE)
		battery->sub->hv_chg_name = "PDIC";
#endif
		battery->input_voltage =
				battery->sub->pdic_info.sink_status.power_list[current_pdo].max_voltage / 1000;
		dev_info(battery->dev, "%s: available pdo : %d, current pdo : %d\n", __func__,
			battery->sub->pdic_info.sink_status.available_pdo_num, current_pdo);

		for(i=1; i<= battery->sub->pdic_info.sink_status.available_pdo_num; i++) {
			pr_info("%s: power_list[%d], voltage : %d, current : %d, power : %d\n", __func__, i,
				battery->sub->pdic_info.sink_status.power_list[i].max_voltage,
				battery->sub->pdic_info.sink_status.power_list[i].max_current,
				battery->sub->pdic_info.sink_status.power_list[i].max_voltage *
				battery->sub->pdic_info.sink_status.power_list[i].max_current);

			if ((battery->sub->pdic_info.sink_status.power_list[i].max_voltage *
			     battery->sub->pdic_info.sink_status.power_list[i].max_current) >
			    (pd_charging_charge_power * 1000)) {
				battery->sub->pdic_info.sink_status.power_list[i].max_current =
					(pd_charging_charge_power * 1000) /
					battery->sub->pdic_info.sink_status.power_list[i].max_voltage;

				pr_info("%s: ->updated [%d], voltage : %d, current : %d, power : %d\n", __func__, i,
					battery->sub->pdic_info.sink_status.power_list[i].max_voltage,
					battery->sub->pdic_info.sink_status.power_list[i].max_current,
					battery->sub->pdic_info.sink_status.power_list[i].max_voltage *
					battery->sub->pdic_info.sink_status.power_list[i].max_current);
			}
			if(battery->sub->pdic_info.sink_status.power_list[i].max_current >
			    battery->pdata->max_input_current) {
				battery->sub->pdic_info.sink_status.power_list[i].max_current =
					battery->pdata->max_input_current;

				pr_info("%s: ->updated [%d], voltage : %d, current : %d, power : %d\n", __func__, i,
					battery->sub->pdic_info.sink_status.power_list[i].max_voltage,
					battery->sub->pdic_info.sink_status.power_list[i].max_current,
					battery->sub->pdic_info.sink_status.power_list[i].max_voltage *
					battery->sub->pdic_info.sink_status.power_list[i].max_current);
			}
		}
		if (!battery->sub->pdic_attach) {
			if (make_pd_list(battery, battery->sub) <= 0)
				goto skip_cable_work;
		}
		battery->sub->pdic_attach = true;
		break;
	case CCIC_NOTIFY_ID_USB:
		if(usb_typec_info.cable_type == PD_USB_TYPE)
			battery->sub->pd_usb_attached = true;
		dev_info(battery->dev, "%s: CCIC_NOTIFY_ID_USB: %d\n",__func__, battery->sub->pd_usb_attached);
		wake_lock(&battery->monitor_wake_lock);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		mutex_unlock(&battery->typec_notylock);
		return 0;
	default:
		cmd = "ERROR";
		cable_type = -1;
		battery->sub->muic_cable_type = ATTACHED_DEV_NONE_MUIC;
#if defined(CONFIG_AFC_CHARGER_MODE)
		battery->sub->hv_chg_name = "NONE";
#endif
		break;
	}

skip_cable_check:
	sec_bat_set_misc_event(battery,
		(battery->sub->muic_cable_type == ATTACHED_DEV_UNDEFINED_CHARGING_MUIC ? BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE : 0) ||
		(battery->sub->muic_cable_type == ATTACHED_DEV_UNDEFINED_RANGE_MUIC ? BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE : 0),
		BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE);

	if (battery->sub->muic_cable_type == ATTACHED_DEV_HICCUP_MUIC) {
		sec_bat_set_misc_event(battery, BATT_MISC_EVENT_HICCUP_TYPE, BATT_MISC_EVENT_HICCUP_TYPE);
		battery->hiccup_status = 1;
	} else {
		battery->hiccup_status = 0;
		if (battery->misc_event & BATT_MISC_EVENT_HICCUP_TYPE) {
			wake_lock(&battery->monitor_wake_lock);
			queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		}
	}

	/* showing charging icon and noti(no sound, vi, haptic) only
	   if slow insertion is detected by MUIC */
	sec_bat_set_misc_event(battery,
		(battery->sub->muic_cable_type == ATTACHED_DEV_TIMEOUT_OPEN_MUIC ? BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE : 0),
		 BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE);

	if (cable_type < 0 || cable_type > SEC_BATTERY_CABLE_MAX) {
		dev_info(battery->dev, "%s: ignore event(%d)\n",
			__func__, battery->sub->muic_cable_type);
		goto skip_cable_work;
	} else if ((cable_type == SEC_BATTERY_CABLE_UNKNOWN) &&
		   (battery->status != POWER_SUPPLY_STATUS_DISCHARGING)) {
		battery->sub->cable_type = cable_type;
		wake_lock(&battery->monitor_wake_lock);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		dev_info(battery->dev, "%s: UNKNOWN cable plugin\n", __func__);
		goto skip_cable_work;
	}
	battery->sub->cable_type = cable_type;

	if (battery->sub->cable_type == SEC_BATTERY_CABLE_NONE)
		sec_bat_cable_info_clear(battery->sub);

	sec_bat_check_input_voltage(battery, battery->sub);

	cancel_delayed_work(&battery->cable_work);
	wake_unlock(&battery->cable_wake_lock);

	if (cable_type == SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT) {
		/* set current event */
		cancel_delayed_work(&battery->afc_work);
		wake_unlock(&battery->afc_wake_lock);
		sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_CHG_LIMIT,
					  (SEC_BAT_CURRENT_EVENT_CHG_LIMIT | SEC_BAT_CURRENT_EVENT_AFC));
		battery->wire_status = battery->sub->cable_type;
		wake_lock(&battery->monitor_wake_lock);
		battery->polling_count = 1;	/* initial value = 1 */
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
	} else if ((battery->sub->cable_type == battery->cable_type) &&
		(((battery->sub->cable_type == SEC_BATTERY_CABLE_USB || battery->sub->cable_type == SEC_BATTERY_CABLE_TA) &&
		battery->sub->pdic_info.sink_status.rp_currentlvl > RP_CURRENT_LEVEL_DEFAULT) ||
		is_hv_wire_type(battery->sub->cable_type))) {
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
		queue_delayed_work(battery->monitor_wqueue,
				&battery->cable_work, 0);
	}

skip_cable_work:
	dev_info(battery->dev, "%s: CMD[%s], CABLE_TYPE[%d]\n", __func__, cmd, cable_type);
	mutex_unlock(&battery->typec_notylock);
	return 0;
}

 static int usb_typec_handle_notification_main(struct notifier_block *nb,
		unsigned long action, void *data)
{
  const char *cmd;
	struct sec_battery_info *battery =
			container_of(nb, struct sec_battery_info, usb_typec_main_nb);
	int cable_type = SEC_BATTERY_CABLE_NONE;
	CC_NOTI_ATTACH_TYPEDEF usb_typec_info = *(CC_NOTI_ATTACH_TYPEDEF *)data;

	dev_info(battery->dev, "%s: action (%ld) dump(0x%01x, 0x%01x, 0x%02x, 0x%04x, 0x%04x, 0x%04x)\n",
		__func__, action, usb_typec_info.src, usb_typec_info.dest, usb_typec_info.id,
		usb_typec_info.attach, usb_typec_info.rprd, usb_typec_info.cable_type);

	if (usb_typec_info.dest != CCIC_NOTIFY_DEV_SUB_BATTERY) {
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
			cable_type = SEC_BATTERY_CABLE_NONE;
			battery->main->muic_cable_type = ATTACHED_DEV_NONE_MUIC;
			break;
		case MUIC_NOTIFY_CMD_ATTACH:
		case MUIC_NOTIFY_CMD_LOGICALLY_ATTACH:
			/* Skip notify from MUIC if PDIC is attached already */

			cmd = "ATTACH";
			battery->main->muic_cable_type = usb_typec_info.cable_type;
			cable_type = sec_bat_cable_check(battery, battery->main->muic_cable_type);
			break;
		default:
			cmd = "ERROR";
			cable_type = -1;
			battery->main->muic_cable_type = usb_typec_info.cable_type;
			break;
		}
#if defined(CONFIG_AFC_CHARGER_MODE)
		if (battery->main->muic_cable_type == ATTACHED_DEV_QC_CHARGER_9V_MUIC ||
			battery->main->muic_cable_type == ATTACHED_DEV_QC_CHARGER_ERR_V_MUIC)
			battery->main->hv_chg_name = "QC";
		else if (battery->main->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_9V_MUIC ||
			battery->main->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_9V_DUPLI_MUIC ||
			battery->main->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_ERR_V_MUIC ||
			battery->main->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_ERR_V_DUPLI_MUIC)
			battery->main->hv_chg_name = "AFC";
#if defined(CONFIG_MUIC_HV_12V)
		else if (battery->main->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_12V_MUIC ||
			battery->main->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_12V_DUPLI_MUIC)
			battery->main->hv_chg_name = "12V";
#endif
		else
			battery->main->hv_chg_name = "NONE";
#endif
		break;
	default:
		cmd = "ERROR";
		cable_type = -1;
		battery->main->muic_cable_type = ATTACHED_DEV_NONE_MUIC;
#if defined(CONFIG_AFC_CHARGER_MODE)
		battery->main->hv_chg_name = "NONE";
#endif
		break;
	}

	sec_bat_set_misc_event(battery,
		(battery->sub->muic_cable_type == ATTACHED_DEV_UNDEFINED_CHARGING_MUIC ? BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE : 0) ||
		(battery->sub->muic_cable_type == ATTACHED_DEV_UNDEFINED_RANGE_MUIC ? BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE : 0),
		BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE);

	if (battery->main->muic_cable_type == ATTACHED_DEV_HICCUP_MUIC) {
		sec_bat_set_misc_event(battery, BATT_MISC_EVENT_HICCUP_TYPE, BATT_MISC_EVENT_HICCUP_TYPE);
		battery->hiccup_status = 1;
	} else {
		battery->hiccup_status = 0;
		if (battery->misc_event & BATT_MISC_EVENT_HICCUP_TYPE) {
			wake_lock(&battery->monitor_wake_lock);
			queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		}
	}

	/* showing charging icon and noti(no sound, vi, haptic) only
	   if slow insertion is detected by MUIC */
	sec_bat_set_misc_event(battery,
		(battery->sub->muic_cable_type == ATTACHED_DEV_TIMEOUT_OPEN_MUIC ? BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE : 0),
		 BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE);

	if (cable_type < 0 || cable_type > SEC_BATTERY_CABLE_MAX) {
		dev_info(battery->dev, "%s: ignore event(%d)\n",
			__func__, battery->main->muic_cable_type);
		goto skip_cable_work;
	} else if ((cable_type == SEC_BATTERY_CABLE_UNKNOWN) &&
		   (battery->status != POWER_SUPPLY_STATUS_DISCHARGING)) {
		battery->main->cable_type = cable_type;
		wake_lock(&battery->monitor_wake_lock);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		dev_info(battery->dev, "%s: UNKNOWN cable plugin\n", __func__);
		goto skip_cable_work;
	}
	battery->main->cable_type = cable_type;

	if (battery->main->cable_type == SEC_BATTERY_CABLE_NONE)
		sec_bat_cable_info_clear(battery->main);

	sec_bat_check_input_voltage(battery, battery->main);

	cancel_delayed_work(&battery->cable_work);
	wake_unlock(&battery->cable_wake_lock);

	if (cable_type == SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT) {
		/* set current event */
		cancel_delayed_work(&battery->afc_work);
		wake_unlock(&battery->afc_wake_lock);
		sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_CHG_LIMIT,
					  (SEC_BAT_CURRENT_EVENT_CHG_LIMIT | SEC_BAT_CURRENT_EVENT_AFC));
		battery->wire_status = battery->main->cable_type;
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
		queue_delayed_work(battery->monitor_wqueue,
				&battery->cable_work, 0);
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
			if (battery->select->cable_type == SEC_BATTERY_CABLE_PDIC) {
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
				battery->pdic_info.sink_status.power_list[selected_pdo].max_voltage / 1000;

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

	sec_bat_set_misc_event(battery, BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE,
#if !defined(CONFIG_ENG_BATTERY_CONCEPT) && !defined(CONFIG_SEC_FACTORY)
		(battery->muic_cable_type != ATTACHED_DEV_JIG_UART_ON_MUIC) &&
		(battery->muic_cable_type != ATTACHED_DEV_JIG_USB_ON_MUIC) &&
#endif
		(battery->muic_cable_type != ATTACHED_DEV_UNDEFINED_RANGE_MUIC));
	if (battery->muic_cable_type == ATTACHED_DEV_HICCUP_MUIC) {
		sec_bat_set_misc_event(battery, BATT_MISC_EVENT_HICCUP_TYPE, 0);
		battery->hiccup_status = 1;
	} else {
		battery->hiccup_status = 0;
		if (battery->misc_event & BATT_MISC_EVENT_HICCUP_TYPE) {
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
	if (battery->select->muic_cable_type == ATTACHED_DEV_HMT_MUIC &&
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

		battery->cable_type = SEC_BATTERY_CABLE_OTG;
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

	if (!is_nocharge_type(battery->cable_type)) {
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
			BATT_MISC_EVENT_HICCUP_TYPE))) {
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
	struct cable_info *cable_select = NULL, *cable_main = NULL, *cable_sub = NULL;

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
			goto err_pdata_free;
		}

		cable_select = devm_kzalloc(&pdev->dev,
				sizeof(struct cable_info), GFP_KERNEL);
		if (!cable_select) {
			dev_err(&pdev->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_pdata_free;
		}
		battery->select = cable_select;

		cable_main = devm_kzalloc(&pdev->dev,
				sizeof(struct cable_info), GFP_KERNEL);
		if (!cable_main) {
			dev_err(&pdev->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_cable_select_free;
		}
		battery->main = cable_main;

		cable_sub = devm_kzalloc(&pdev->dev,
				sizeof(struct cable_info), GFP_KERNEL);
		if (!cable_sub) {
			dev_err(&pdev->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_cable_main_free;
		}
		battery->sub = cable_sub;

	} else {
		pdata = dev_get_platdata(&pdev->dev);
		battery->pdata = pdata;
	}

	platform_set_drvdata(pdev, battery);

	battery->dev = &pdev->dev;

	mutex_init(&battery->adclock);
	mutex_init(&battery->iolock);
	mutex_init(&battery->misclock);
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
	wake_lock_init(&battery->siop_wake_lock, WAKE_LOCK_SUSPEND,
		       "sec-battery-siop");
	wake_lock_init(&battery->siop_level_wake_lock, WAKE_LOCK_SUSPEND,
			"sec-battery-siop_level");
	wake_lock_init(&battery->wc_headroom_wake_lock, WAKE_LOCK_SUSPEND,
			"sec-battery-wc_headroom");
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	wake_lock_init(&battery->batt_data_wake_lock, WAKE_LOCK_SUSPEND,
			"sec-battery-update-data");
#endif
	wake_lock_init(&battery->misc_event_wake_lock, WAKE_LOCK_SUSPEND,
			"sec-battery-misc-event");
#ifdef CONFIG_OF
	wake_lock_init(&battery->parse_mode_dt_wake_lock, WAKE_LOCK_SUSPEND,
			"sec-battery-parse_mode_dt");
#endif

	/* initialization of battery info */
	sec_bat_set_charging_status(battery,
			POWER_SUPPLY_STATUS_DISCHARGING);
	battery->health = POWER_SUPPLY_HEALTH_GOOD;
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
#endif
	battery->ps_enable = false;
    battery->wc_status = SEC_WIRELESS_PAD_NONE;
	battery->wc_cv_mode = false;
	battery->wire_status = SEC_BATTERY_CABLE_NONE;

#if defined(CONFIG_BATTERY_SWELLING)
	battery->swelling_mode = SWELLING_MODE_NONE;
#endif
	battery->charging_block = false;
	battery->chg_limit = false;
	battery->mix_limit = false;
	battery->vbus_limit = false;
	battery->vbus_chg_by_siop = SEC_INPUT_VOLTAGE_0V;
	battery->vbus_chg_by_full = false;
	battery->usb_temp = 0;
#if defined(CONFIG_ENG_BATTERY_CONCEPT) || defined(CONFIG_SEC_FACTORY)
	battery->cooldown_mode = true;
#endif
	battery->skip_swelling = false;
	battery->led_cover = 0;
	battery->hiccup_status = 0;

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
	battery->sub->cable_type = SEC_BATTERY_CABLE_NONE;
	battery->main->cable_type = SEC_BATTERY_CABLE_NONE;
	battery->test_mode = 0;
	battery->factory_mode = false;
	battery->store_mode = false;
	battery->is_hc_usb = false;
	battery->is_sysovlo = false;
	battery->is_vbatovlo = false;
	battery->is_abnormal_temp = false;

	battery->safety_timer_set = true;
	battery->stop_timer = false;
	battery->prev_safety_time = 0;
	battery->lcd_status = false;

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

	/* Check High Voltage charging option for wireless charging */
	/* '1' means disabling High Voltage charging */
	if (charging_night_mode == '1')
		sleep_mode = true;
	else
		sleep_mode = false;

	/* Check High Voltage charging option for wired charging */
	if (get_afc_mode() == CH_MODE_AFC_DISABLE_VAL) {
		pr_info("HV wired charging mode is disabled\n");
		sec_bat_set_current_event(battery,
			SEC_BAT_CURRENT_EVENT_HV_DISABLE, SEC_BAT_CURRENT_EVENT_HV_DISABLE);
	}

	if(fg_reset)
		sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_FG_RESET,
			SEC_BAT_CURRENT_EVENT_FG_RESET);

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

	INIT_DELAYED_WORK(&battery->monitor_work, sec_bat_monitor_work);
	INIT_DELAYED_WORK(&battery->cable_work, sec_bat_cable_work);
#if defined(CONFIG_CALC_TIME_TO_FULL)
	INIT_DELAYED_WORK(&battery->timetofull_work, sec_bat_time_to_full_work);
#endif
#if defined(CONFIG_ENABLE_100MA_CHARGING_BEFORE_USB_CONFIGURED)
	INIT_DELAYED_WORK(&battery->slowcharging_work, sec_bat_check_slowcharging_work);
#endif
	INIT_DELAYED_WORK(&battery->afc_work, sec_bat_afc_work);
	INIT_DELAYED_WORK(&battery->siop_work, sec_bat_siop_work);
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
	/* queue_delayed_work(battery->monitor_wqueue, &battery->fw_init_work, 0); */
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
	manager_notifier_register(&battery->usb_typec_main_nb,
		usb_typec_handle_notification_main, MANAGER_NOTIFY_CCIC_SUB_BATTERY);
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
	wake_lock_destroy(&battery->siop_wake_lock);
	wake_lock_destroy(&battery->siop_level_wake_lock);
	wake_lock_destroy(&battery->wc_headroom_wake_lock);
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	wake_lock_destroy(&battery->batt_data_wake_lock);
#endif
	wake_lock_destroy(&battery->misc_event_wake_lock);
#ifdef CONFIG_OF
	wake_lock_destroy(&battery->parse_mode_dt_wake_lock);
#endif
	mutex_destroy(&battery->adclock);
	mutex_destroy(&battery->iolock);
	mutex_destroy(&battery->misclock);
	mutex_destroy(&battery->batt_handlelock);
	mutex_destroy(&battery->current_eventlock);
	mutex_destroy(&battery->typec_notylock);
	mutex_destroy(&battery->wclock);
	mutex_destroy(&battery->voutlock);
	kfree(cable_sub);
err_cable_main_free:
	kfree(cable_main);
err_cable_select_free:
	kfree(cable_select);
err_pdata_free:
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
	wake_lock_destroy(&battery->siop_wake_lock);
	wake_lock_destroy(&battery->siop_level_wake_lock);
	wake_lock_destroy(&battery->misc_event_wake_lock);
	wake_lock_destroy(&battery->wc_headroom_wake_lock);
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	wake_lock_destroy(&battery->batt_data_wake_lock);
#endif
#ifdef CONFIG_OF
	wake_lock_destroy(&battery->parse_mode_dt_wake_lock);
#endif
	mutex_destroy(&battery->adclock);
	mutex_destroy(&battery->iolock);
	mutex_destroy(&battery->misclock);
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
