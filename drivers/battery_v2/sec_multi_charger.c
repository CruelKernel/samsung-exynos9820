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

#include "include/sec_multi_charger.h"

static enum power_supply_property sec_multi_charger_props[] = {
};

static bool sec_multi_chg_check_sub_charging(struct sec_multi_charger_info *charger)
{
	union power_supply_propval value;

	if (!charger->pdata->sub_charger_condition) {
		pr_info("%s: sub charger off(default)\n", __func__);
		return false;
	}

	if (charger->pdata->sub_charger_condition &
		SEC_SUB_CHARGER_CONDITION_CHARGE_POWER) {
		psy_do_property(charger->pdata->battery_name, get, POWER_SUPPLY_PROP_POWER_NOW, value);
		if (value.intval < charger->pdata->sub_charger_condition_charge_power) {
			if (charger->sub_is_charging)
				pr_info("%s: sub charger off CHARGE_POWER(%d)\n", __func__, value.intval);
			return false;
		}
	}

	if (charger->pdata->sub_charger_condition &
		SEC_SUB_CHARGER_CONDITION_CURRENT_MAX) {
		if (charger->total_current.input_current_limit < charger->pdata->sub_charger_condition_current_max) {
			if (charger->sub_is_charging)
				pr_info("%s: sub charger off CURRENT_MAX(%d)\n", __func__,
					charger->total_current.input_current_limit);
			return false;
		}
	}

	if (charger->pdata->sub_charger_condition &
		SEC_SUB_CHARGER_CONDITION_ONLINE) {
		int i = 0;

		for (i = 0; i < charger->pdata->sub_charger_condition_online_size; i++) {
			if (charger->cable_type == charger->pdata->sub_charger_condition_online[i])
				break;
		}

		if (i >= charger->pdata->sub_charger_condition_online_size) {
			if (charger->sub_is_charging)
				pr_info("%s: sub charger off ONLINE(%d)\n", __func__, i);
			return false;
		}
	}

	if (charger->pdata->sub_charger_condition &
		SEC_SUB_CHARGER_CONDITION_CHARGE_DONE) {
		if (charger->sub_is_charging) {
			/* psy_do_property(charger->pdata->main_charger_name, get,
				POWER_SUPPLY_PROP_STATUS, value);
			if (value.intval == POWER_SUPPLY_STATUS_FULL) {
				pr_info("%s: sub charger off CHARGE DONE by main charger\n", __func__);
				return false;
			} */
			psy_do_property(charger->pdata->sub_charger_name, get,
				POWER_SUPPLY_PROP_STATUS, value);
			if (value.intval == POWER_SUPPLY_STATUS_FULL) {
				pr_info("%s: sub charger off CHARGE DONE by sub charger\n", __func__);
				return false;
			}
		}
	}

	if (charger->pdata->sub_charger_condition &
		SEC_SUB_CHARGER_CONDITION_CV) {
		psy_do_property(charger->pdata->main_charger_name, get,
			POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE, value);

		if (value.intval) {
			if (charger->sub_is_charging)
				pr_info("%s: sub charger off CV(%d)\n", __func__, value.intval);
			return false;
		}
	}

	if (charger->pdata->sub_charger_condition &
		SEC_SUB_CHARGER_CONDITION_CURRENT_NOW) {
		int max_current_now = (charger->total_current.fast_charging_current / 2) +
			charger->full_check_current_1st + SEC_SUB_CHARGER_CURRENT_MARGIN;
		pr_info("%s: update max_current_now(%d)\n", __func__, max_current_now);

		psy_do_property(charger->pdata->battery_name, get,
			POWER_SUPPLY_PROP_CURRENT_NOW, value);

		if (value.intval < max_current_now) {
			if (charger->sub_is_charging)
				pr_info("%s: sub charger off CURRENT_NOW(%d)\n", __func__, value.intval);
			return false;
		} else if (value.intval < max_current_now + SEC_SUB_CHARGER_CURRENT_MARGIN) {
			if (!charger->sub_is_charging) {
				return false;
			}
		}
	}

	return true;
}

static int sec_multi_chg_set_input_current(struct sec_multi_charger_info *charger)
{
	union power_supply_propval value;
	int main_input_current = charger->main_current.input_current_limit,
		sub_input_current = charger->sub_current.input_current_limit;

	if (!charger->pdata->is_serial && charger->sub_is_charging) {
		main_input_current = charger->total_current.input_current_limit / 2;
		sub_input_current = charger->total_current.input_current_limit / 2;

		/* check current max */
		value.intval = sub_input_current;
		psy_do_property(charger->pdata->sub_charger_name, get,
			POWER_SUPPLY_PROP_CURRENT_MAX, value);
		if (value.intval != sub_input_current) {
			main_input_current = charger->total_current.input_current_limit - value.intval;
			sub_input_current = value.intval;
		}
	} else {
		main_input_current = charger->total_current.input_current_limit;
		sub_input_current = 0;
	}

	/* set input current */
	if (main_input_current != charger->main_current.input_current_limit) {
		charger->main_current.input_current_limit = main_input_current;
		value.intval = main_input_current;
		psy_do_property(charger->pdata->main_charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_MAX, value);

		pr_info("%s: set input current - main(%dmA)\n", __func__, value.intval);
	}
	if (sub_input_current != charger->sub_current.input_current_limit) {
		charger->sub_current.input_current_limit = sub_input_current;
		value.intval = sub_input_current;
		psy_do_property(charger->pdata->sub_charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_MAX, value);

		pr_info("%s: set input current - sub(%dmA)\n", __func__, value.intval);
	}

	return 0;
}

static int sec_multi_chg_set_charging_current(struct sec_multi_charger_info *charger)
{
	union power_supply_propval value;
	unsigned int main_charging_current = charger->main_current.fast_charging_current,
		sub_charging_current = charger->sub_current.fast_charging_current;

	if (charger->sub_is_charging) {
		main_charging_current = charger->total_current.fast_charging_current / 2;
		sub_charging_current = charger->total_current.fast_charging_current / 2;
	} else {
		main_charging_current = charger->total_current.fast_charging_current;
		sub_charging_current = 0;
	}

	/* set charging current */
	if (main_charging_current != charger->main_current.fast_charging_current) {
		charger->main_current.fast_charging_current = main_charging_current;
		value.intval = main_charging_current;
		psy_do_property(charger->pdata->main_charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_NOW, value);

		pr_info("%s: set charging current - main(%dmA)\n", __func__, value.intval);
	}
	if (sub_charging_current != charger->sub_current.fast_charging_current) {
		charger->sub_current.fast_charging_current = sub_charging_current;
		value.intval = sub_charging_current;
		psy_do_property(charger->pdata->sub_charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_NOW, value);

		pr_info("%s: set charging current - sub(%dmA)\n", __func__, value.intval);
	}

	return 0;
}

static bool sec_multi_chg_check_abnormal_case(struct sec_multi_charger_info *charger)
{
	union power_supply_propval value;
	bool check_val = false;

	/* check abnormal case */
	psy_do_property(charger->pdata->sub_charger_name, get,
		POWER_SUPPLY_EXT_PROP_CHECK_MULTI_CHARGE, value);

	check_val = (value.intval != POWER_SUPPLY_STATUS_CHARGING && charger->sub_is_charging);
	pr_info("%s: check abnormal case(check_val:%d, status:%d, sub_is_charging:%d)\n",
		__func__, check_val, value.intval, charger->sub_is_charging);

	return check_val;
}

static void sec_multi_chg_check_input_current(struct sec_multi_charger_info *charger)
{
	union power_supply_propval value;
	bool sub_is_charging = charger->sub_is_charging;

	if (!sub_is_charging || is_nocharge_type(charger->cable_type)) {
		pr_info("%s: does not need that check input current when sub charger is off.", __func__);
		return;
	}

	if (charger->pdata->sub_charger_condition &
		SEC_SUB_CHARGER_CONDITION_CHARGE_POWER) {
		psy_do_property(charger->pdata->battery_name, get, POWER_SUPPLY_PROP_POWER_NOW, value);
		if (value.intval < charger->pdata->sub_charger_condition_charge_power) {
			if (sub_is_charging)
				pr_info("%s: sub charger off CHARGE_POWER(%d)\n", __func__, value.intval);
			sub_is_charging = false;
		}
	}

	if (charger->pdata->sub_charger_condition &
		SEC_SUB_CHARGER_CONDITION_CURRENT_MAX) {
		if (charger->total_current.input_current_limit < charger->pdata->sub_charger_condition_current_max) {
			if (sub_is_charging)
				pr_info("%s: sub charger off CURRENT_MAX(%d)\n", __func__,
					charger->total_current.input_current_limit);
			sub_is_charging = false;
		}
	}

	if (!sub_is_charging || sec_multi_chg_check_abnormal_case(charger)) {
		charger->sub_is_charging = sub_is_charging;

		if (sub_is_charging)
			value.intval = SEC_BAT_CHG_MODE_CHARGING;
		else
			value.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;

		psy_do_property(charger->pdata->sub_charger_name, set,
			POWER_SUPPLY_PROP_CHARGING_ENABLED, value);

		sec_multi_chg_set_charging_current(charger);
	}
}

static int sec_multi_chg_check_enable(struct sec_multi_charger_info *charger)
{
	union power_supply_propval value;
	bool sub_is_charging = charger->sub_is_charging;

	if (is_nocharge_type(charger->cable_type) ||
		(charger->status == POWER_SUPPLY_STATUS_DISCHARGING) ||
		charger->chg_mode != SEC_BAT_CHG_MODE_CHARGING) {
		pr_info("%s: skip multi charging routine\n", __func__);
		return 0;
	}

	if (charger->multi_mode != SEC_MULTI_CHARGER_NORMAL) {
		pr_info("%s: skip multi charging routine, because the multi_mode = %d\n", __func__, charger->multi_mode);
		return 0;
	}

	/* check sub charging */
	charger->sub_is_charging = sec_multi_chg_check_sub_charging(charger);

	/* set sub charging */
	if (charger->sub_is_charging != sub_is_charging) {
		if (charger->sub_is_charging)
			value.intval = SEC_BAT_CHG_MODE_CHARGING;
		else
			value.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;

		/* set charging current */
		sec_multi_chg_set_input_current(charger);
		sec_multi_chg_set_charging_current(charger);

		psy_do_property(charger->pdata->sub_charger_name, set,
			POWER_SUPPLY_PROP_CHARGING_ENABLED, value);

		pr_info("%s: change sub_is_charging(%d)\n", __func__, charger->sub_is_charging);
	} else if (charger->sub_is_charging && sec_multi_chg_check_abnormal_case(charger)) {
		pr_info("%s: abnormal case, sub charger off\n ", __func__);

		charger->sub_is_charging = false;

		sec_multi_chg_set_input_current(charger);
		sec_multi_chg_set_charging_current(charger);

		value.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;
		psy_do_property(charger->pdata->sub_charger_name, set,
			POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
	}
	return 0;
}

static int sec_multi_chg_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct sec_multi_charger_info *charger =
		power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = psp;
	union power_supply_propval value;

	value.intval = val->intval;

	switch (psp) {
	case POWER_SUPPLY_PROP_HEALTH:
		psy_do_property(charger->pdata->battery_name, get,
			POWER_SUPPLY_PROP_HEALTH, value);
		if (!is_nocharge_type(charger->cable_type) && 
			value.intval != POWER_SUPPLY_HEALTH_UNDERVOLTAGE)
			psy_do_property(charger->pdata->sub_charger_name, get, psp, value);
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_ONLINE:
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
	case POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL:
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
		psy_do_property(charger->pdata->main_charger_name, get, psp, value);
		val->intval = value.intval;
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		psy_do_property(charger->pdata->main_charger_name, get, psp, value);
		val->intval = value.intval;
		sec_multi_chg_check_enable(charger);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = charger->total_current.fast_charging_current;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_CHECK_SLAVE_I2C:
			psy_do_property(charger->pdata->sub_charger_name, get, psp, value);
			val->intval = value.intval;
			break;
		case POWER_SUPPLY_EXT_PROP_MULTI_CHARGER_MODE:
			switch (charger->multi_mode) {
				case SEC_MULTI_CHARGER_MAIN_ONLY:
					val->strval = "master";
					break;
				case SEC_MULTI_CHARGER_SUB_ONLY:
					val->strval = "slave";
					break;
				case SEC_MULTI_CHARGER_ALL_ENABLE:
					val->strval = "dual";
					break;
				case SEC_MULTI_CHARGER_NORMAL:
					if (!charger->sub_is_charging)
						val->strval = "master"; //Main Charger Default ON;	Sub charger depend on sub_charger_condition .
					else
						val->strval = "dual";
					break;
				default:
					val->strval = "master";
					break;
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

static int sec_multi_chg_set_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    const union power_supply_propval *val)
{
	struct sec_multi_charger_info *charger =
		power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = psp;
	union power_supply_propval value, get_value;

	value.intval = val->intval;
	switch (psp) {
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		charger->chg_mode = val->intval;
		psy_do_property(charger->pdata->main_charger_name, set,
			psp, value);

		psy_do_property(charger->pdata->main_charger_name, get, POWER_SUPPLY_PROP_ONLINE, get_value);
		if (!is_nocharge_type(get_value.intval)) {
			if (val->intval != SEC_BAT_CHG_MODE_CHARGING) {
				psy_do_property(charger->pdata->sub_charger_name, set,
					psp, value);
			} else if (charger->sub_is_charging) {
				psy_do_property(charger->pdata->sub_charger_name, set,
					psp, value);
			}
		}
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		psy_do_property(charger->pdata->main_charger_name, set,
			psp, value);
		psy_do_property(charger->pdata->sub_charger_name, set,
			psp, value);

		/* INIT */
		if (is_nocharge_type(val->intval)) {
			charger->sub_is_charging = false;
			charger->main_current.input_current_limit = 0;
			charger->main_current.fast_charging_current = 0;
			charger->sub_current.input_current_limit = 0;
			charger->sub_current.fast_charging_current = 0;
			charger->multi_mode = SEC_MULTI_CHARGER_NORMAL;
		} else if (charger->sub_is_charging &&
			charger->cable_type != val->intval &&
			charger->chg_mode == SEC_BAT_CHG_MODE_CHARGING &&
			charger->multi_mode == SEC_MULTI_CHARGER_NORMAL) {
			charger->sub_is_charging = false;
			/* set charging current */
			sec_multi_chg_set_input_current(charger);
			sec_multi_chg_set_charging_current(charger);
			/* disable sub charger */
			value.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;
			psy_do_property(charger->pdata->sub_charger_name, set,
				POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
		} else {
			pr_info("%s: invalid condition (sub_is_charging(%d), chg_mode(%d), multi_mode(%d))\n",
				__func__, charger->sub_is_charging, charger->chg_mode, charger->multi_mode);
		}
		charger->cable_type = val->intval;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		charger->status = val->intval;
	case POWER_SUPPLY_PROP_HEALTH:
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		psy_do_property(charger->pdata->main_charger_name, set, psp, value);
		psy_do_property(charger->pdata->sub_charger_name, set, psp, value);
		break;
	case POWER_SUPPLY_PROP_CURRENT_FULL:
		charger->full_check_current_1st = val->intval;
		psy_do_property(charger->pdata->main_charger_name, set, psp, value);
		psy_do_property(charger->pdata->sub_charger_name, set, psp, value);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		charger->total_current.input_current_limit = val->intval;
		sec_multi_chg_check_input_current(charger);
		sec_multi_chg_set_input_current(charger);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		charger->total_current.fast_charging_current = val->intval;
		sec_multi_chg_set_charging_current(charger);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		charger->siop_level = val->intval;
		psy_do_property(charger->pdata->main_charger_name, set, psp, value);
		psy_do_property(charger->pdata->sub_charger_name, set, psp, value);
		break;
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
	case POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL:
		psy_do_property(charger->pdata->main_charger_name, set, psp, value);
		break;
#if defined(CONFIG_AFC_CHARGER_MODE)
	case POWER_SUPPLY_PROP_AFC_CHARGER_MODE:
		psy_do_property(charger->pdata->main_charger_name, set, psp, value);
		break;
#endif
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
		/* AICL Enable */
		if(!charger->pdata->aicl_disable)
			psy_do_property(charger->pdata->main_charger_name, set, psp, value);
		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_MULTI_CHARGER_MODE:
			if (charger->chg_mode == SEC_BAT_CHG_MODE_CHARGING && charger->multi_mode != val->intval) {
				charger->multi_mode = val->intval;
				switch (val->intval) {
				case SEC_MULTI_CHARGER_MAIN_ONLY:
					pr_info("%s: Only Use Main Charger \n", __func__);
					charger->total_current.input_current_limit = is_hv_wire_type(charger->cable_type) ?
						SEC_MULTI_CHARGER_TEST_MASTER_MODE_CURRENT :charger->total_current.input_current_limit;
					charger->sub_is_charging = false;
					value.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;
					psy_do_property(charger->pdata->sub_charger_name, set,
						POWER_SUPPLY_PROP_CHARGING_ENABLED, value);

					value.intval = SEC_BAT_CHG_MODE_CHARGING;
					psy_do_property(charger->pdata->main_charger_name, set,
						POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
					break;
				case SEC_MULTI_CHARGER_SUB_ONLY:
					pr_info("%s: Only Use Sub Charger \n", __func__);
					charger->total_current.input_current_limit = is_hv_wire_type(charger->cable_type) ?
						SEC_MULTI_CHARGER_TEST_SLAVE_MODE_CURRENT :charger->total_current.input_current_limit;
					charger->sub_is_charging = true;
					value.intval = SEC_BAT_CHG_MODE_CHARGING;
					psy_do_property(charger->pdata->sub_charger_name, set,
						POWER_SUPPLY_PROP_CHARGING_ENABLED, value);

					value.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;
					psy_do_property(charger->pdata->main_charger_name, set,
						POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
					break;
				case SEC_MULTI_CHARGER_ALL_ENABLE:
					pr_info("%s: Enable Main & Sub Charger together \n", __func__);
					charger->sub_is_charging = true;
					value.intval = SEC_BAT_CHG_MODE_CHARGING;
					psy_do_property(charger->pdata->sub_charger_name, set,
						POWER_SUPPLY_PROP_CHARGING_ENABLED, value);

					value.intval = SEC_BAT_CHG_MODE_CHARGING;
					psy_do_property(charger->pdata->main_charger_name, set,
						POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
					break;
				default:
					charger->multi_mode = SEC_MULTI_CHARGER_NORMAL;
					charger->sub_is_charging = false;
					value.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;
					psy_do_property(charger->pdata->sub_charger_name, set,
						POWER_SUPPLY_PROP_CHARGING_ENABLED, value);

					value.intval = SEC_BAT_CHG_MODE_CHARGING;
					psy_do_property(charger->pdata->main_charger_name, set,
						POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
					break;
				}
				/* set charging current */
				sec_multi_chg_set_input_current(charger);
				sec_multi_chg_set_charging_current(charger);
			}
			pr_info("%s: set Multi Charger Mode (%d)\n", __func__, charger->multi_mode);
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

#ifdef CONFIG_OF
static int sec_multi_charger_parse_dt(struct device *dev,
		struct sec_multi_charger_info *charger)
{
	struct device_node *np = dev->of_node;
	struct sec_multi_charger_platform_data *pdata = charger->pdata;
	int ret = 0, temp_value = 0;
	int len;
	const u32 *p;
	
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

		ret = of_property_read_string(np, "charger,sub_charger",
				(char const **)&charger->pdata->sub_charger_name);
		if (ret)
			pr_err("%s: sub_charger is Empty\n", __func__);

		ret = of_property_read_u32(np, "charger,is_serial",
			&temp_value);
		if (ret) {
			pr_err("%s: is_serial is Empty\n", __func__);
			temp_value = 1;
		}
		pdata->is_serial = (temp_value != 0);

		pdata->aicl_disable = of_property_read_bool(np,
			"charger,aicl_disable");

		ret = of_property_read_u32(np, "charger,sub_charger_condition",
				&pdata->sub_charger_condition);
		if (ret) {
			pr_err("%s: sub_charger_condition is Empty\n", __func__);
			pdata->sub_charger_condition = 0;
		}

		if (pdata->sub_charger_condition) {
			ret = of_property_read_u32(np, "charger,sub_charger_condition_current_max",
					&pdata->sub_charger_condition_current_max);
			if (ret) {
				pr_err("%s: sub_charger_condition_current_max is Empty\n", __func__);
				pdata->sub_charger_condition &= ~SEC_SUB_CHARGER_CONDITION_CURRENT_MAX;
				pdata->sub_charger_condition_current_max = 0;
			}

			ret = of_property_read_u32(np, "charger,sub_charger_condition_charge_power",
					&pdata->sub_charger_condition_charge_power);
			if (ret) {
				pr_err("%s: sub_charger_condition_charge_power is Empty\n", __func__);
				pdata->sub_charger_condition &= ~SEC_SUB_CHARGER_CONDITION_CHARGE_POWER;
				pdata->sub_charger_condition_charge_power = 15000;
			}

			p = of_get_property(np, "charger,sub_charger_condition_online", &len);
			if (p) {
				len = len / sizeof(u32);

				pdata->sub_charger_condition_online = kzalloc(sizeof(unsigned int) * len,
								  GFP_KERNEL);
				ret = of_property_read_u32_array(np, "charger,sub_charger_condition_online",
						 pdata->sub_charger_condition_online, len);

				pdata->sub_charger_condition_online_size = len;
			} else {
				pdata->sub_charger_condition &= ~SEC_SUB_CHARGER_CONDITION_ONLINE;
				pdata->sub_charger_condition_online_size = 0;
			}

			pr_info("%s: sub_charger_condition(0x%x)\n", __func__, pdata->sub_charger_condition);
		}
	}
	return 0;
}
#endif

static const struct power_supply_desc sec_multi_charger_power_supply_desc = {
	.name = "sec-multi-charger",
	.type = POWER_SUPPLY_TYPE_UNKNOWN,
	.properties = sec_multi_charger_props,
	.num_properties = ARRAY_SIZE(sec_multi_charger_props),
	.get_property = sec_multi_chg_get_property,
	.set_property = sec_multi_chg_set_property,
};

static int sec_multi_charger_probe(struct platform_device *pdev)
{
	struct sec_multi_charger_info *charger;
	struct sec_multi_charger_platform_data *pdata = NULL;
	struct power_supply_config multi_charger_cfg = {};
	int ret = 0;

	dev_info(&pdev->dev,
		"%s: SEC Multi-Charger Driver Loading\n", __func__);

	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (!charger)
		return -ENOMEM;

	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev,
				sizeof(struct sec_multi_charger_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_charger_free;
		}

		charger->pdata = pdata;
		if (sec_multi_charger_parse_dt(&pdev->dev, charger)) {
			dev_err(&pdev->dev,
				"%s: Failed to get sec-multi-charger dt\n", __func__);
			ret = -EINVAL;
			goto err_charger_free;
		}
	} else {
		pdata = dev_get_platdata(&pdev->dev);
		charger->pdata = pdata;
	}

	charger->sub_is_charging = false;
	charger->multi_mode = SEC_MULTI_CHARGER_NORMAL;

	platform_set_drvdata(pdev, charger);
	charger->dev = &pdev->dev;
	multi_charger_cfg.drv_data = charger;

	charger->psy_chg = power_supply_register(&pdev->dev, &sec_multi_charger_power_supply_desc, &multi_charger_cfg);
	if (!charger->psy_chg) {
		dev_err(charger->dev,
			"%s: Failed to Register psy_chg\n", __func__);
		goto err_pdata_free;
	}

	dev_info(charger->dev,
		"%s: SEC Multi-Charger Driver Loaded\n", __func__);
	return 0;

err_pdata_free:
	kfree(pdata);
err_charger_free:
	kfree(charger);

	return ret;
}

static int sec_multi_charger_remove(struct platform_device *pdev)
{
	struct sec_multi_charger_info *charger = platform_get_drvdata(pdev);

	power_supply_unregister(charger->psy_chg);

	dev_dbg(charger->dev, "%s: End\n", __func__);

	kfree(charger->pdata);
	kfree(charger);

	return 0;
}

static int sec_multi_charger_suspend(struct device *dev)
{
	return 0;
}

static int sec_multi_charger_resume(struct device *dev)
{
	return 0;
}

static void sec_multi_charger_shutdown(struct device *dev)
{
}

#ifdef CONFIG_OF
static struct of_device_id sec_multi_charger_dt_ids[] = {
	{ .compatible = "samsung,sec-multi-charger" },
	{ }
};
MODULE_DEVICE_TABLE(of, sec_multi_charger_dt_ids);
#endif /* CONFIG_OF */

static const struct dev_pm_ops sec_multi_charger_pm_ops = {
	.suspend = sec_multi_charger_suspend,
	.resume = sec_multi_charger_resume,
};

static struct platform_driver sec_multi_charger_driver = {
	.driver = {
		.name = "sec-multi-charger",
		.owner = THIS_MODULE,
		.pm = &sec_multi_charger_pm_ops,
		.shutdown = sec_multi_charger_shutdown,
#ifdef CONFIG_OF
		.of_match_table = sec_multi_charger_dt_ids,
#endif
	},
	.probe = sec_multi_charger_probe,
	.remove = sec_multi_charger_remove,
};

static int __init sec_multi_charger_init(void)
{
	pr_info("%s: \n", __func__);
	return platform_driver_register(&sec_multi_charger_driver);
}

static void __exit sec_multi_charger_exit(void)
{
	platform_driver_unregister(&sec_multi_charger_driver);
}

device_initcall_sync(sec_multi_charger_init);
module_exit(sec_multi_charger_exit);

MODULE_DESCRIPTION("Samsung Multi Charger Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
