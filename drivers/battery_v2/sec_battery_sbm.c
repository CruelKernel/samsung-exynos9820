/*
 *  sec_battery_log.c
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

#define DISABLE_FUNC 0
#define MAX_DATA_STR_LEN 128//1024

static char strSbmData[MAX_DATA_STR_LEN];

void sec_bat_get_sbm_data_string(union power_supply_propval *value)
{
	pr_err("%s: %s\n", __func__,strSbmData);

	sprintf(strSbmData+strlen(strSbmData), "\n");
	value->strval = strSbmData;
}

static bool sec_bat_make_sbm_data_battery(
			struct sec_battery_info *battery, int data_type)
{
	union power_supply_propval value;
	bool bRet = true;
	char *str = NULL;

	pr_err("%s \n", __func__);

	str = kzalloc(sizeof(char)*MAX_DATA_STR_LEN, GFP_KERNEL);
	if (!str)
		return false;

	mutex_lock(&battery->sbmlock);
	sprintf(str, " [%d]", data_type);

	switch(data_type) {
		case SBM_DATA_COMMON_INFO:
			sprintf(str+strlen(str), "%d,%d,%d,%d,%d,%d,%d,%d,%d",
				battery->cable_type,
				battery->capacity,
				battery->voltage_now,
				battery->health,
				battery->current_now,
				battery->current_max,
				battery->charging_current,
				battery->temperature,
				battery->chg_temp);
			break;
		case SBM_DATA_SET_CHARGE:
			sprintf(str+strlen(str), "%d,%d,%d",
				battery->cable_type, battery->capacity, battery->voltage_now);
			break;
		case SBM_DATA_FULL_1ST:
			sprintf(str+strlen(str), "%d,%d,%d",
				battery->charging_mode, battery->capacity, battery->voltage_now);
			/* get data from fuelgague */
			value.intval = SBM_DATA_FULL_1ST;
			psy_do_property(battery->pdata->fuelgauge_name, set,
				POWER_SUPPLY_EXT_PROP_SBM_DATA, value);
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_EXT_PROP_SBM_DATA, value);
			if (value.intval) {
				sprintf(str+strlen(str), ",%s", value.strval);
			}
			break;
		case SBM_DATA_FULL_2ND:
			sprintf(str+strlen(str), "%d,%d,%d",
				battery->charging_mode, battery->capacity, battery->voltage_now);
			/* get data from fuelgague */
			value.intval = SBM_DATA_FULL_2ND;
			psy_do_property(battery->pdata->fuelgauge_name, set,
				POWER_SUPPLY_EXT_PROP_SBM_DATA, value);
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_EXT_PROP_SBM_DATA, value);
			if (value.intval) {
				sprintf(str+strlen(str), ",%s", value.strval);
			}
			break;
		case SBM_DATA_TEMP:
			sprintf(str+strlen(str), "%d,%d,%d,%d",
				battery->temperature, battery->chg_temp, battery->wpc_temp, battery->usb_temp);
			break;
		default:
			bRet = false;
			goto err_make_data;
			break;
	}

	if (strlen(strSbmData) + strlen(str) > MAX_DATA_STR_LEN) {
		/* do someting */
		if(strlen(str) < MAX_DATA_STR_LEN)
			sprintf(strSbmData, "%s", str);
		else
			pr_err("%s new str length is too long\n", __func__);

		goto err_make_data;
	}

	if (battery->sbm_data)
		sprintf(strSbmData+strlen(strSbmData), "%s", str);
	else
		sprintf(strSbmData, "%s", str);

err_make_data:
	kfree(str);
	mutex_unlock(&battery->sbmlock);

	return bRet;
}

bool sec_bat_add_sbm_data(struct sec_battery_info *battery, int data_type)
{
	bool bData = false;

	pr_err("%s \n", __func__);

	if (is_sbm_data_type(data_type)) {
		bData = sec_bat_make_sbm_data_battery(battery, data_type);
	} else {
		return false;
	}

	battery->sbm_data = bData;
	return true;
}

void sec_bat_init_sbm(struct sec_battery_info *battery)
{
	pr_err("%s \n", __func__);
}

void sec_bat_exit_sbm(struct sec_battery_info *battery)
{
	pr_err("%s \n", __func__);

	if (battery->strSbmData)
		kfree(battery->strSbmData);
	if (battery->strSbmDataB)
		kfree(battery->strSbmDataB);
}
