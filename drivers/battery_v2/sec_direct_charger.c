/*
 *  sec_multi_charger.c
 *  Samsung Mobile Charger Driver
 *
 *  Copyright (C) 2015 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define DEBUG

#include "include/sec_direct_charger.h"

char *sec_direct_chg_mode_str[] = {
	"OFF", //SEC_DIRECT_CHG_MODE_DIRECT_OFF
	"CHECK_VBAT", //SEC_DIRECT_CHG_MODE_DIRECT_CHECK_VBAT
	"PRESET", //SEC_DIRECT_CHG_MODE_DIRECT_PRESET
	"ON_ADJUST", // SEC_DIRECT_CHG_MODE_DIRECT_ON_ADJUST
	"ON", //SEC_DIRECT_CHG_MODE_DIRECT_ON
	"DONE", //SEC_DIRECT_CHG_MODE_DIRECT_DONE
};

char *sec_direct_charger_mode_str[] = {
	"Charging-On",
	"Charging-Off",
	"Buck-Off",
	"OTG-On",
	"OTG-Off",
	"UNO-On",
	"UNO-Off",
	"UNO-Only",
	"Max",
};

void sec_direct_chg_init(struct sec_battery_info *battery, struct device *dev)
{
	pr_info("%s: called\n", __func__);
}

void sec_direct_chg_monitor(struct sec_direct_charger_info *charger)
{
	if(charger->charging_source == SEC_DIRECT_CHG_CHARGING_SOURCE_DIRECT) {
		pr_info("%s: Src(%s), direct(%s), switching(%s), Imax(%dmA), Ichg(%dmA), dc_input(%dmA)\n",
			__func__, charger->charging_source ? "DIRECT" : "SWITCHING",
			sec_direct_charger_mode_str[charger->charger_mode_direct],
			sec_direct_charger_mode_str[charger->charger_mode_main],
			charger->input_current, charger->charging_current, charger->dc_input_current);
	}
}

static bool sec_direct_chg_set_direct_charge(
		struct sec_direct_charger_info *charger, unsigned int charger_mode)
{
	union power_supply_propval value = {0,};

	if (charger->ta_alert_wa) {
		psy_do_property("battery", get,
				POWER_SUPPLY_EXT_PROP_DIRECT_TA_ALERT, value);
		charger->ta_alert_mode =  value.intval;
	}

	if (charger->charger_mode_direct == charger_mode && !(charger->dc_retry_cnt) &&
		(charger->ta_alert_mode == OCP_NONE)) {
		pr_info("%s: charger_mode is same(%s)\n", __func__,
			sec_direct_charger_mode_str[charger->charger_mode_direct]);
		return false;
	}

	pr_info("%s: charger_mode(%s->%s)\n", __func__, 
		sec_direct_charger_mode_str[charger->charger_mode_direct],
		sec_direct_charger_mode_str[charger_mode]);
	charger->charger_mode_direct = charger_mode;

	if (charger_mode == SEC_BAT_CHG_MODE_CHARGING)
		value.intval = true;
	else
		value.intval = false;

	psy_do_property(charger->pdata->direct_charger_name, set,
		POWER_SUPPLY_PROP_CHARGING_ENABLED, value);

	return true;
}

static bool sec_direct_chg_set_switching_charge(
		struct sec_direct_charger_info *charger, unsigned int charger_mode)
{
	union power_supply_propval value = {0,};

	pr_info("%s: charger_mode(%s->%s)\n", __func__,
		sec_direct_charger_mode_str[charger->charger_mode_main],
		sec_direct_charger_mode_str[charger_mode]);
	charger->charger_mode_main = charger_mode;

	value.intval = charger_mode;
	psy_do_property(charger->pdata->main_charger_name, set,
		POWER_SUPPLY_PROP_CHARGING_ENABLED, value);

	return true;
}

static int sec_direct_chg_check_charging_source(struct sec_direct_charger_info *charger)
{
	union power_supply_propval value = {0,};

	pr_info("%s: dc_retry_cnt(%d)\n", __func__, charger->dc_retry_cnt);

	if (charger->dc_err) {
		if (charger->ta_alert_wa) {
			psy_do_property("battery", get,
					POWER_SUPPLY_EXT_PROP_DIRECT_TA_ALERT, value);
			charger->ta_alert_mode =  value.intval;
		}

		pr_info("%s: dc_err(%d), ta_alert(%d)\n", __func__, charger->dc_err,
			charger->ta_alert_mode);
		value.intval = SEC_BAT_CURRENT_EVENT_DC_ERR;
		psy_do_property("battery", set,
			POWER_SUPPLY_EXT_PROP_CURRENT_EVENT, value);
		if (!charger->ta_alert_wa || (charger->ta_alert_mode == OCP_NONE))
			return SEC_DIRECT_CHG_CHARGING_SOURCE_SWITCHING;
	}

	psy_do_property("battery", get,
				POWER_SUPPLY_PROP_STATUS, value);
	charger->batt_status = value.intval;

	psy_do_property("battery", get,
				POWER_SUPPLY_PROP_CAPACITY, value);
	charger->capacity = value.intval;

	psy_do_property("battery", get,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ENABLE, value);
	charger->wc_tx_enable = value.intval;
	if (charger->wc_tx_enable) {
		pr_info("@TX_Mode %s: Source Switching charger during Tx mode\n", __func__);
		return SEC_DIRECT_CHG_CHARGING_SOURCE_SWITCHING;
	}

	psy_do_property("battery", get,
				POWER_SUPPLY_PROP_TEMP, value);
	charger->bat_temp = value.intval;

	psy_do_property("battery", get,
				POWER_SUPPLY_EXT_PROP_CURRENT_EVENT, value);
	if (((charger->bat_temp <= charger->pdata->dchg_temp_low_threshold) || (charger->bat_temp >= charger->pdata->dchg_temp_high_threshold)) ||
		(value.intval & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING ||
		value.intval & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_2ND ||
		value.intval & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING ||
		value.intval & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_3RD ||
		value.intval & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_4TH ||
		value.intval & SEC_BAT_CURRENT_EVENT_HV_DISABLE ||
		((value.intval & SEC_BAT_CURRENT_EVENT_DC_ERR) && charger->ta_alert_mode == OCP_NONE) ||
		value.intval & SEC_BAT_CURRENT_EVENT_SIOP_LIMIT || value.intval & SEC_BAT_CURRENT_EVENT_SEND_UVDM) || charger->test_mode_source == SEC_DIRECT_CHG_CHARGING_SOURCE_SWITCHING)
		return SEC_DIRECT_CHG_CHARGING_SOURCE_SWITCHING;

	psy_do_property("battery", get,
		POWER_SUPPLY_PROP_ONLINE, value);
	if (!is_pd_apdo_wire_type(charger->cable_type) || !is_pd_apdo_wire_type(value.intval))
		return SEC_DIRECT_CHG_CHARGING_SOURCE_SWITCHING;

	if ((charger->batt_status == POWER_SUPPLY_STATUS_FULL) ||
		(charger->batt_status == POWER_SUPPLY_STATUS_NOT_CHARGING) ||
		(charger->batt_status == POWER_SUPPLY_STATUS_DISCHARGING))
		return SEC_DIRECT_CHG_CHARGING_SOURCE_SWITCHING;

	psy_do_property("battery", get,
			POWER_SUPPLY_EXT_PROP_DIRECT_HAS_APDO, value);

	if (charger->direct_chg_done || (charger->capacity >= 95) || !value.intval)
		return SEC_DIRECT_CHG_CHARGING_SOURCE_SWITCHING;

	return SEC_DIRECT_CHG_CHARGING_SOURCE_DIRECT;
}

static int sec_direct_chg_set_charging_source(struct sec_direct_charger_info *charger,
		unsigned int charger_mode, int charging_source)
{
	mutex_lock(&charger->charger_mutex);
	if (charging_source == SEC_DIRECT_CHG_CHARGING_SOURCE_DIRECT &&
		charger_mode == SEC_BAT_CHG_MODE_CHARGING) {
		sec_direct_chg_set_switching_charge(charger, SEC_BAT_CHG_MODE_BUCK_OFF);
		sec_direct_chg_set_direct_charge(charger, SEC_BAT_CHG_MODE_CHARGING);
		if (charger->fpdo_pos == 0) {
			union power_supply_propval value = {0,};

			psy_do_property("battery", get,
					POWER_SUPPLY_EXT_PROP_DIRECT_FIXED_PDO, value);
			pr_info("%s : fixed pdo number change (%d -> %d)\n", __func__, charger->fpdo_pos, value.intval);
			charger->fpdo_pos = value.intval;
		}
	} else {
		union power_supply_propval value = {0,};

		psy_do_property("battery", get,
					POWER_SUPPLY_EXT_PROP_DIRECT_CHARGER_MODE, value);
		charger->now_isApdo = value.intval;

		psy_do_property("battery", get,
					POWER_SUPPLY_EXT_PROP_DIRECT_HV_PDO, value);
		charger->hv_pdo = value.intval;
		if (charger->ta_alert_wa) {
			psy_do_property("battery", get,
					POWER_SUPPLY_EXT_PROP_DIRECT_TA_ALERT, value);
			charger->ta_alert_mode =  value.intval;
		}

		if ((is_pd_apdo_wire_type(charger->cable_type) &&
			(charger->now_isApdo ||
			(charger->dc_err && (charger->ta_alert_mode == OCP_NONE)) ||
			(!charger->hv_pdo && (charger->fpdo_pos > 1)))) &&
			charger->batt_status != POWER_SUPPLY_STATUS_DISCHARGING) {
			if ((charger->wc_tx_enable && charger->now_isApdo) || !charger->wc_tx_enable)
				select_pdo(charger->fpdo_pos);
		}
		sec_direct_chg_set_direct_charge(charger, SEC_BAT_CHG_MODE_CHARGING_OFF);
		sec_direct_chg_set_switching_charge(charger, charger_mode);
	}

	charger->charging_source = charging_source;
	mutex_unlock(&charger->charger_mutex);

	return 0;
}

static void sec_direct_chg_set_charge(struct sec_direct_charger_info *charger, unsigned int charger_mode)
{
	int charging_source;

	charger->charger_mode = charger_mode;
	
	switch (charger->charger_mode) {
		case SEC_BAT_CHG_MODE_BUCK_OFF:
		case SEC_BAT_CHG_MODE_CHARGING_OFF:
			charger->is_charging = false;
			break;
		case SEC_BAT_CHG_MODE_CHARGING:
			charger->is_charging = true;
			break;
	}

	charging_source = sec_direct_chg_check_charging_source(charger);
	sec_direct_chg_set_charging_source(charger, charger_mode, charging_source);
}

static void sec_direct_chg_do_dc_fullcharged(struct sec_direct_charger_info *charger) {
	int charging_source;

	pr_info("%s: called\n", __func__);
	charger->direct_chg_done = true;

	charging_source = sec_direct_chg_check_charging_source(charger);
	sec_direct_chg_set_charging_source(charger, charger->charger_mode, charging_source);
}

static int sec_direct_chg_set_input_current(struct sec_direct_charger_info *charger,
			enum power_supply_property psp, int input_current) {
	union power_supply_propval value = {0,};

	pr_info("%s: called(%dmA)\n", __func__, input_current);

	value.intval = input_current;
	psy_do_property(charger->pdata->main_charger_name, set,
		POWER_SUPPLY_PROP_CURRENT_MAX, value);

	/* direct charger input current is based on charging current */
	return 0;
}

static int sec_direct_chg_set_charging_current(struct sec_direct_charger_info *charger,
			enum power_supply_property psp, int charging_current) {
	union power_supply_propval value = {0,};
	int charging_source;

	pr_info("%s: called(%dmA)\n", __func__, charging_current);

	/* main charger */
	value.intval = charging_current;
	if (psp == POWER_SUPPLY_PROP_CURRENT_AVG) {
		psy_do_property(charger->pdata->main_charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_AVG, value);
	} else {
		psy_do_property(charger->pdata->main_charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_NOW, value);
	}

	/* direct charger */
	if (is_pd_apdo_wire_type(charger->cable_type)) {
		charger->dc_charging_current = charging_current;
		charger->dc_input_current = charger->dc_charging_current / 2;

		charging_source = sec_direct_chg_check_charging_source(charger);
		if (charging_source == SEC_DIRECT_CHG_CHARGING_SOURCE_DIRECT) {
			value.intval = charger->dc_input_current;
			psy_do_property(charger->pdata->direct_charger_name, set,
				POWER_SUPPLY_PROP_CURRENT_MAX, value);
		}
		sec_direct_chg_set_charging_source(charger, charger->charger_mode, charging_source);
	}

	return 0;
}

static int sec_direct_chg_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct sec_direct_charger_info *charger = power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;
	union power_supply_propval value = {0,};
	int ret = 0;

	value.intval = val->intval;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (charger->charging_source == SEC_DIRECT_CHG_CHARGING_SOURCE_DIRECT) {
			psy_do_property(charger->pdata->direct_charger_name, get, psp, value);
		} else {
			psy_do_property(charger->pdata->main_charger_name, get, psp, value);
		}
		val->intval = value.intval;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		if (charger->charging_source == SEC_DIRECT_CHG_CHARGING_SOURCE_DIRECT) {
			psy_do_property(charger->pdata->direct_charger_name, get, psp, value);
			if (value.intval == POWER_SUPPLY_HEALTH_DC_ERR) {
				charger->dc_retry_cnt++;
				if (charger->dc_retry_cnt > 2) {
					charger->dc_err = true;
				} else
					charger->dc_err = false;
			} else {
				charger->dc_err = false;
				charger->dc_retry_cnt = 0;
			}
		} else {
			psy_do_property(charger->pdata->main_charger_name, get, psp, value);
			charger->dc_retry_cnt = 0;
		}
		val->intval = value.intval;
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX: /* get input current which was set */
		psy_do_property(charger->pdata->main_charger_name, get, psp, value);
		if (is_direct_chg_mode_on(charger->direct_chg_mode)) {
			// NEED to CHECK
			val->intval = charger->input_current;
		} else {
			val->intval = value.intval;
		}
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW: /* get charge current which was set */
		psy_do_property(charger->pdata->main_charger_name, get, psp, value);
		if (is_direct_chg_mode_on(charger->direct_chg_mode)) {
			// NEED to CHECK
			val->intval = charger->charging_current;
		} else {
			val->intval = value.intval;
		}
		break;
	case POWER_SUPPLY_PROP_TEMP:
		psy_do_property(charger->pdata->direct_charger_name, get, psp, value);
		val->intval = value.intval;
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_MONITOR_WORK:
			psy_do_property(charger->pdata->main_charger_name, get, ext_psp, value);			
			if (is_pd_apdo_wire_type(charger->cable_type))
				psy_do_property(charger->pdata->direct_charger_name, get, ext_psp, value);
			sec_direct_chg_monitor(charger);
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_CHARGER_MODE:
			val->intval = charger->direct_chg_mode;
			break;
		case POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED_DC:
			psy_do_property(charger->pdata->main_charger_name, get,
				POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
			if (value.intval == SEC_BAT_CHG_MODE_CHARGING)
				val->intval = true;
			else
				val->intval = false;
			break;
		case POWER_SUPPLY_EXT_PROP_MEASURE_INPUT:
			psy_do_property(charger->pdata->direct_charger_name, get, ext_psp, value);
			val->intval = value.intval;
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_VOLTAGE_MAX:
			psy_do_property(charger->pdata->direct_charger_name, get, ext_psp, value);
			val->intval = value.intval;
			break;
		case POWER_SUPPLY_EXT_PROP_CHANGE_CHARGING_SOURCE:
			val->intval = charger->test_mode_source;
			break;
		default:
			ret = psy_do_property(charger->pdata->main_charger_name, get, ext_psp, value);
			val->intval = value.intval;
			return ret;
		}
		break;
	default:
		ret = psy_do_property(charger->pdata->main_charger_name, get, psp, value);
		val->intval = value.intval;
		return ret;
	}

	return ret;
}

static int sec_direct_chg_set_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    const union power_supply_propval *val)
{
	struct sec_direct_charger_info *charger = power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;
	union power_supply_propval value = {0,};
	int prev_val;
	int ret = 0;

	value.intval = val->intval;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		psy_do_property(charger->pdata->main_charger_name, set,
			psp, value);
		charger->batt_status = val->intval;
		pr_info("%s: batt status(%d)\n", __func__, charger->batt_status);
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		sec_direct_chg_set_charge(charger, val->intval);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		prev_val = charger->cable_type;
		charger->cable_type = val->intval;
		if (charger->cable_type == SEC_BATTERY_CABLE_NONE) {
			if (charger->dc_err) {
				value.intval = SEC_BAT_CURRENT_EVENT_DC_ERR;
				psy_do_property("battery", set,
					POWER_SUPPLY_EXT_PROP_CURRENT_EVENT_CLEAR, value);
			}	
			charger->direct_chg_done = false;

			charger->fpdo_pos = 0;
			charger->dc_charging_current = charger->pdata->dchg_min_current;
			charger->dc_input_current = charger->dc_charging_current / 2;
			charger->dc_err = false;
			charger->dc_retry_cnt = 0;
			charger->test_mode_source = SEC_DIRECT_CHG_CHARGING_SOURCE_DIRECT;
		}

		/* main charger */
		value.intval = val->intval;
		psy_do_property(charger->pdata->main_charger_name, set,
			psp, value);

		/* direct charger */
		if (is_pd_apdo_wire_type(charger->cable_type)) {
			charger->direct_chg_mode = SEC_DIRECT_CHG_MODE_DIRECT_CHECK_VBAT;
			value.intval = 1;
			psy_do_property(charger->pdata->direct_charger_name, set,
				psp, value);
		} else {				
			value.intval = 0;
			psy_do_property(charger->pdata->direct_charger_name, set,
				psp, value);
		}
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		charger->input_current = val->intval;
		sec_direct_chg_set_input_current(charger, psp, charger->input_current);
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		charger->charging_current = val->intval;
		sec_direct_chg_set_charging_current(charger, psp, charger->charging_current);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		charger->float_voltage = val->intval;
		psy_do_property(charger->pdata->main_charger_name, set,
			psp, value);
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_DIRECT_CHARGER_MODE:
			if (val->intval >= SEC_DIRECT_CHG_MODE_MAX) {
				pr_info("%s: abnormal direct_chg_mode(%d)\n", __func__, val->intval);
			} else {
				if (!charger->direct_chg_done) {
					pr_info("%s: direct_chg_mode:%s(%d)->%s(%d)\n", __func__,
						sec_direct_chg_mode_str[charger->direct_chg_mode], charger->direct_chg_mode,
						sec_direct_chg_mode_str[val->intval], val->intval); 
					charger->direct_chg_mode = val->intval;
				}
			}
			break;
		case POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED_DC:
#if 0
			if (val->intval)
				sec_direct_chg_check_set_charge(charger, charger->charger_mode,
					SEC_BAT_CHG_MODE_BUCK_OFF, SEC_BAT_CHG_MODE_CHARGING);
			else
				sec_direct_chg_check_set_charge(charger, charger->charger_mode,
					SEC_BAT_CHG_MODE_CHARGING, SEC_BAT_CHG_MODE_CHARGING_OFF);				
#endif
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_DONE:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_DIRECT_DONE(%d)\n", __func__, val->intval);
			if (val->intval)
				sec_direct_chg_do_dc_fullcharged(charger);
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_FIXED_PDO:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_DIRECT_FIXED_PDO(%d)\n", __func__, val->intval);
			charger->fpdo_pos = val->intval;
			break;
		case POWER_SUPPLY_EXT_PROP_CURRENT_MEASURE:
			psy_do_property(charger->pdata->main_charger_name, set,
				ext_psp, value);
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_WDT_CONTROL:
			psy_do_property(charger->pdata->direct_charger_name, set,
				ext_psp, value);
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_VOLTAGE_MAX:
			psy_do_property(charger->pdata->direct_charger_name, set,
				ext_psp, value);
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_CURRENT_MAX:
			psy_do_property(charger->pdata->direct_charger_name, set,
				ext_psp, value);
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_FLOAT_MAX:
			psy_do_property(charger->pdata->direct_charger_name, set,
				ext_psp, value);
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_ADC_CTRL:
			psy_do_property(charger->pdata->direct_charger_name, set,
				ext_psp, value);
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_CLEAR_ERR:
			/* If SRCCAP is changed by Src, clear DC err variables */
			charger->dc_err = false;
			charger->dc_retry_cnt = 0;
			break;
        case POWER_SUPPLY_EXT_PROP_CHANGE_CHARGING_SOURCE:
            {
				charger->test_mode_source = val->intval;
                pr_info("%s: POWER_SUPPLY_EXT_PROP_CHANGE_CHARGING_SOURCE(%d)", __func__, charger->test_mode_source);

				if (charger->test_mode_source == SEC_DIRECT_CHG_CHARGING_SOURCE_DIRECT)
	                charger->test_mode_source = sec_direct_chg_check_charging_source(charger);

                sec_direct_chg_set_charging_source(charger, charger->charger_mode, charger->test_mode_source);
            }
            break;
 		default:
			ret = psy_do_property(charger->pdata->main_charger_name, set, ext_psp, value);
			return ret;
		}
		break;
	default:
		ret = psy_do_property(charger->pdata->main_charger_name, set, psp, value);
		return ret;
	}

	return ret;
}

#ifdef CONFIG_OF
static int sec_direct_charger_parse_dt(struct device *dev,
		struct sec_direct_charger_info *charger)
{
	struct device_node *np = dev->of_node;
	int ret = 0;
	
	if (!np) {
		pr_err("%s: np NULL\n", __func__);
		return 1;
	} else {
		ret = of_property_read_string(np, "charger,battery_name",
				(char const **)&charger->pdata->battery_name);
		if (ret)
			pr_err("%s: battery_name is Empty\n", __func__);

		ret = of_property_read_string(np, "charger,main_charger",
				(char const **)&charger->pdata->main_charger_name);
		if (ret)
			pr_err("%s: main_charger is Empty\n", __func__);

		ret = of_property_read_string(np, "charger,direct_charger",
				(char const **)&charger->pdata->direct_charger_name);
		if (ret)
			pr_err("%s: direct_charger is Empty\n", __func__);

		ret = of_property_read_u32(np, "charger,dchg_min_current",
			&charger->pdata->dchg_min_current);
		if (ret) {
			pr_err("%s : charger,dchg_min_current is Empty\n", __func__);
			charger->pdata->dchg_min_current = SEC_DIRECT_CHG_MIN_IOUT;
		}
		pr_info("%s: charger,dchg_min_current is %d\n", __func__, charger->pdata->dchg_min_current);

		ret = of_property_read_u32(np, "charger,dchg_temp_low_threshold",
			&charger->pdata->dchg_temp_low_threshold);
		if (ret) {
			pr_err("%s : charger,dchg_temp_low_threshold is Empty\n", __func__);
			charger->pdata->dchg_temp_low_threshold = 180;
		}
		pr_info("%s: charger,dchg_temp_low_threshold is %d\n", __func__, charger->pdata->dchg_temp_low_threshold);

		ret = of_property_read_u32(np, "charger,dchg_temp_high_threshold",
			&charger->pdata->dchg_temp_high_threshold);
		if (ret) {
			pr_err("%s : charger,dchg_temp_high_threshold is Empty\n", __func__);
			charger->pdata->dchg_temp_high_threshold = 410;
		}
		pr_info("%s: charger,dchg_temp_high_threshold is %d\n", __func__, charger->pdata->dchg_temp_high_threshold);

		charger->ta_alert_wa = of_property_read_bool(np, "charger,ta_alert_wa");
	}
	return 0;
}
#else
static int sec_direct_charger_parse_dt(struct device *dev,
		struct sec_direct_charger_info *charger)
{
	return 0;
}
#endif /* CONFIG_OF */

static enum power_supply_property sec_direct_charger_props[] = {
};

static const struct power_supply_desc sec_direct_charger_power_supply_desc = {
	.name = "sec-direct-charger",
	.type = POWER_SUPPLY_TYPE_UNKNOWN,
	.properties = sec_direct_charger_props,
	.num_properties = ARRAY_SIZE(sec_direct_charger_props),
	.get_property = sec_direct_chg_get_property,
	.set_property = sec_direct_chg_set_property,
};

static int sec_direct_charger_probe(struct platform_device *pdev)
{
	struct sec_direct_charger_info *charger;
	struct sec_direct_charger_platform_data *pdata = NULL;
	struct power_supply_config direct_charger_cfg = {};
	int ret = 0;

	pr_info("%s: SEC Direct-Charger Driver Loading\n", __func__);

	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (!charger)
		return -ENOMEM;

	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev,
				sizeof(struct sec_direct_charger_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_charger_free;
		}

		charger->pdata = pdata;
		if (sec_direct_charger_parse_dt(&pdev->dev, charger)) {
			dev_err(&pdev->dev,
				"%s: Failed to get sec-direct-charger dt\n", __func__);
			ret = -EINVAL;
			goto err_charger_free;
		}
	} else {
		pdata = dev_get_platdata(&pdev->dev);
		charger->pdata = pdata;
	}

	/* init direct charger variables */
	charger->direct_chg_done = false;
	charger->direct_chg_mode = SEC_DIRECT_CHG_MODE_DIRECT_OFF;
	charger->cable_type = SEC_BATTERY_CABLE_NONE;

	charger->charger_mode = SEC_BAT_CHG_MODE_CHARGING_OFF;
	charger->charger_mode_direct = SEC_BAT_CHG_MODE_MAX;
	charger->charger_mode_main = SEC_BAT_CHG_MODE_MAX;
	charger->test_mode_source = SEC_DIRECT_CHG_CHARGING_SOURCE_DIRECT;

	charger->wc_tx_enable = false;
	charger->now_isApdo = false;

	platform_set_drvdata(pdev, charger);
	charger->dev = &pdev->dev;
	direct_charger_cfg.drv_data = charger;
	charger->ta_alert_mode = OCP_NONE;

	mutex_init(&charger->charger_mutex);

	charger->psy_chg = power_supply_register(&pdev->dev,
			&sec_direct_charger_power_supply_desc, &direct_charger_cfg);
	if (!charger->psy_chg) {
		dev_err(charger->dev,
			"%s: Failed to Register psy_chg\n", __func__);
		goto err_pdata_free;
	}

	pr_info("%s: SEC Direct-Charger Driver Loaded(%s, %s)\n",
		__func__, charger->pdata->main_charger_name, charger->pdata->direct_charger_name);
	return 0;

err_pdata_free:
	mutex_destroy(&charger->charger_mutex);
	kfree(pdata);
err_charger_free:
	kfree(charger);

	return ret;
}

static int sec_direct_charger_remove(struct platform_device *pdev)
{
	struct sec_direct_charger_info *charger = platform_get_drvdata(pdev);

	pr_info("%s: ++\n", __func__);

	power_supply_unregister(charger->psy_chg);
	mutex_destroy(&charger->charger_mutex);

	dev_dbg(charger->dev, "%s: End\n", __func__);

	kfree(charger->pdata);
	kfree(charger);

	pr_info("%s: --\n", __func__);

	return 0;
}

static int sec_direct_charger_suspend(struct device *dev)
{
	return 0;
}

static int sec_direct_charger_resume(struct device *dev)
{
	return 0;
}

static void sec_direct_charger_shutdown(struct platform_device *pdev)
{
	struct sec_direct_charger_info *charger = platform_get_drvdata(pdev);
	union power_supply_propval value = {0,};

	pr_info("%s: ++\n", __func__);

	value.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;
	psy_do_property(charger->pdata->direct_charger_name, set,
		POWER_SUPPLY_PROP_CHARGING_ENABLED, value);

	if (charger->fpdo_pos > 0)
		select_pdo(charger->fpdo_pos);

	pr_info("%s: --\n", __func__);
}

#ifdef CONFIG_OF
static struct of_device_id sec_direct_charger_dt_ids[] = {
	{ .compatible = "samsung,sec-direct-charger" },
	{ }
};
MODULE_DEVICE_TABLE(of, sec_direct_charger_dt_ids);
#endif /* CONFIG_OF */

static const struct dev_pm_ops sec_direct_charger_pm_ops = {
	.suspend = sec_direct_charger_suspend,
	.resume = sec_direct_charger_resume,
};

static struct platform_driver sec_direct_charger_driver = {
	.driver = {
		.name = "sec-direct-charger",
		.owner = THIS_MODULE,
		.pm = &sec_direct_charger_pm_ops,
#ifdef CONFIG_OF
		.of_match_table = sec_direct_charger_dt_ids,
#endif
	},
	.probe = sec_direct_charger_probe,
	.remove = sec_direct_charger_remove,
	.shutdown = sec_direct_charger_shutdown,
};

static int __init sec_direct_charger_init(void)
{
	pr_info("%s: \n", __func__);
	return platform_driver_register(&sec_direct_charger_driver);
}

static void __exit sec_direct_charger_exit(void)
{
	platform_driver_unregister(&sec_direct_charger_driver);
}

device_initcall_sync(sec_direct_charger_init);
module_exit(sec_direct_charger_exit);

MODULE_DESCRIPTION("Samsung Direct Charger Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
