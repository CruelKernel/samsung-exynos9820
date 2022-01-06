/*
 *  sec_battery_dt.c
 *  Samsung Mobile Battery Driver
 *
 *  Copyright (C) 2018 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "include/sec_battery.h"
#include "include/sec_battery_dt.h"


#ifdef CONFIG_OF
int sec_bat_parse_dt(struct device *dev,
		struct sec_battery_info *battery)
{
	struct device_node *np;
	sec_battery_platform_data_t *pdata = battery->pdata;
	int ret = 0, len = 0;
	unsigned int i = 0;
	const u32 *p;
	u32 temp = 0;

	np = of_find_node_by_name(NULL, "cable-info");
	if (!np) {
		pr_err ("%s : np NULL\n", __func__);
	} else {
		struct device_node *child;
		u32 input_current = 0, charging_current = 0;

		ret = of_property_read_u32(np, "default_input_current", &input_current);
		ret = of_property_read_u32(np, "default_charging_current", &charging_current);
		ret = of_property_read_u32(np, "full_check_current_1st", &pdata->full_check_current_1st);
		ret = of_property_read_u32(np, "full_check_current_2nd", &pdata->full_check_current_2nd);

		pdata->default_input_current = input_current;
		pdata->default_charging_current = charging_current;
		pdata->charging_current =
			kzalloc(sizeof(sec_charging_current_t) * SEC_BATTERY_CABLE_MAX,
				GFP_KERNEL);

		for (i = 0; i < SEC_BATTERY_CABLE_MAX; i++) {
			pdata->charging_current[i].input_current_limit = (unsigned int)input_current;
			pdata->charging_current[i].fast_charging_current = (unsigned int)charging_current;
		}

		for_each_child_of_node(np, child) {
			ret = of_property_read_u32(child, "input_current", &input_current);
			ret = of_property_read_u32(child, "charging_current", &charging_current);
			p = of_get_property(child, "cable_number", &len);
			if (!p)
				return 1;

			len = len / sizeof(u32);

			for (i = 0; i <= len; i++) {
				ret = of_property_read_u32_index(child, "cable_number", i, &temp);
				pdata->charging_current[temp].input_current_limit = (unsigned int)input_current;
				pdata->charging_current[temp].fast_charging_current = (unsigned int)charging_current;
			}
		}
	}

	for (i = 0; i < SEC_BATTERY_CABLE_MAX; i++) {
		pr_info("%s : CABLE_NUM(%d) INPUT(%d) CHARGING(%d)\n",
			__func__, i,
			pdata->charging_current[i].input_current_limit,
			pdata->charging_current[i].fast_charging_current);
	}

	np = of_find_node_by_name(NULL, "wireless-power-info");
	if (!np) {
		pr_err ("%s : np NULL\n", __func__);
	} else {
		struct device_node *child;
		u32 wireless_power_class = 0, vout = 0, input_current_limit = 0, fast_charging_current = 0, ttf_charge_current = 0, rx_power = 0;

		pdata->wireless_power_info =
			kzalloc(sizeof(sec_wireless_rx_power_info_t) * SEC_WIRELESS_RX_POWER_MAX,
				GFP_KERNEL);
		i = 0;
		for_each_child_of_node(np, child) {
			ret = of_property_read_u32(child, "vout", &vout);
			ret = of_property_read_u32(child, "input_current_limit", &input_current_limit);
			ret = of_property_read_u32(child, "fast_charging_current", &fast_charging_current);
			ret = of_property_read_u32(child, "ttf_charge_current", &ttf_charge_current);
			ret = of_property_read_u32(child, "rx_power", &rx_power);
			ret = of_property_read_u32(child, "wireless_power_class", &wireless_power_class);

			pdata->wireless_power_info[i].wireless_power_class = (unsigned int)wireless_power_class;
			pdata->wireless_power_info[i].vout = (unsigned int)vout;
			pdata->wireless_power_info[i].input_current_limit = (unsigned int)input_current_limit;
			pdata->wireless_power_info[i].fast_charging_current = (unsigned int)fast_charging_current;
			pdata->wireless_power_info[i].ttf_charge_current = (unsigned int)ttf_charge_current;
			pdata->wireless_power_info[i].rx_power = (unsigned int)rx_power;
			i++;
		}
		if (i > 0) {
			int len_wireless_power_info = i;
			while (i < SEC_WIRELESS_RX_POWER_MAX) {
				pdata->wireless_power_info[i].wireless_power_class = pdata->wireless_power_info[len_wireless_power_info-1].wireless_power_class;
				pdata->wireless_power_info[i].vout = pdata->wireless_power_info[len_wireless_power_info-1].vout;
				pdata->wireless_power_info[i].input_current_limit = pdata->wireless_power_info[len_wireless_power_info-1].input_current_limit;
				pdata->wireless_power_info[i].fast_charging_current = pdata->wireless_power_info[len_wireless_power_info-1].fast_charging_current;
				pdata->wireless_power_info[i].ttf_charge_current = pdata->wireless_power_info[len_wireless_power_info-1].ttf_charge_current;
				pdata->wireless_power_info[i].rx_power = pdata->wireless_power_info[len_wireless_power_info-1].rx_power;
				i++;
			}
		}
		for (i = 0; i < SEC_WIRELESS_RX_POWER_MAX; i++) {
			pr_info("%s : POWER_LIST(%d) POWER_CLASS(%d) VOUT(%d) INPUT(%d) CHARGING(%d) TTF(%d) POWER(%d)\n",
				__func__, i,
				pdata->wireless_power_info[i].wireless_power_class,
				pdata->wireless_power_info[i].vout,
				pdata->wireless_power_info[i].input_current_limit,
				pdata->wireless_power_info[i].fast_charging_current,
				pdata->wireless_power_info[i].ttf_charge_current,
				pdata->wireless_power_info[i].rx_power);
		}
	}

	pr_info("%s : TOPOFF_1ST(%d), TOPOFF_2ND(%d)\n",
		__func__, pdata->full_check_current_1st, pdata->full_check_current_2nd);

	pdata->default_usb_input_current = pdata->charging_current[SEC_BATTERY_CABLE_USB].input_current_limit;
	pdata->default_usb_charging_current = pdata->charging_current[SEC_BATTERY_CABLE_USB].fast_charging_current;
	pdata->default_wc20_input_current = pdata->charging_current[SEC_BATTERY_CABLE_HV_WIRELESS_20].input_current_limit;
	pdata->default_wc20_charging_current = pdata->charging_current[SEC_BATTERY_CABLE_HV_WIRELESS_20].fast_charging_current;	
#ifdef CONFIG_SEC_FACTORY
	pdata->default_charging_current = 1500;
	pdata->charging_current[SEC_BATTERY_CABLE_TA].fast_charging_current = 1500;
#endif
	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_info("%s: np NULL\n", __func__);
		return 1;
	}

	ret = of_property_read_u32(np, "battery,battery_full_capacity",
			&pdata->battery_full_capacity);

	if (ret)
		pr_info("%s : battery_full_capacity is Empty\n", __func__);
#if defined(CONFIG_BATTERY_CISD)	
	else {
		pr_info("%s : battery_full_capacity : %d\n", __func__, pdata->battery_full_capacity);
		pdata->cisd_cap_high_thr = pdata->battery_full_capacity + 1000; /* battery_full_capacity + 1000 */
		pdata->cisd_cap_low_thr = pdata->battery_full_capacity + 500; /* battery_full_capacity + 500 */
		pdata->cisd_cap_limit = (pdata->battery_full_capacity * 11) / 10; /* battery_full_capacity + 10% */
	}

	ret = of_property_read_u32(np, "battery,cisd_max_voltage_thr",
		&pdata->max_voltage_thr);
	if (ret) {
		pr_info("%s : cisd_max_voltage_thr is Empty\n", __func__);
		pdata->max_voltage_thr = 4400;
	}

	ret = of_property_read_u32(np, "battery,cisd_alg_index",
			&pdata->cisd_alg_index);
	if (ret) {
		pr_info("%s : cisd_alg_index is Empty. Defalut set to six\n", __func__);
		pdata->cisd_alg_index = 6;
	} else {
		pr_info("%s : set cisd_alg_index : %d\n", __func__, pdata->cisd_alg_index);
	}
#endif

	ret = of_property_read_u32(np,
				   "battery,expired_time", &temp);
	if (ret) {
		pr_info("expired time is empty\n");
		pdata->expired_time = 3 * 60 * 60;
	} else {
		pdata->expired_time = (unsigned int) temp;
	}
	pdata->expired_time *= 1000;
	battery->expired_time = pdata->expired_time;

	ret = of_property_read_u32(np,
				   "battery,recharging_expired_time", &temp);
	if (ret) {
		pr_info("expired time is empty\n");
		pdata->recharging_expired_time = 90 * 60;
	} else {
		pdata->recharging_expired_time = (unsigned int) temp;
	}
	pdata->recharging_expired_time *= 1000;

	ret = of_property_read_u32(np,
				   "battery,standard_curr", &pdata->standard_curr);
	if (ret) {
		pr_info("standard_curr is empty\n");
		pdata->standard_curr = 2150;
	}

	ret = of_property_read_string(np,
		"battery,vendor", (char const **)&pdata->vendor);
	if (ret)
		pr_info("%s: Vendor is Empty\n", __func__);

	ret = of_property_read_string(np,
		"battery,charger_name", (char const **)&pdata->charger_name);
	if (ret)
		pr_info("%s: Charger name is Empty\n", __func__);

	ret = of_property_read_string(np,
		"battery,fuelgauge_name", (char const **)&pdata->fuelgauge_name);
	if (ret)
		pr_info("%s: Fuelgauge name is Empty\n", __func__);

	ret = of_property_read_string(np,
		"battery,wireless_charger_name", (char const **)&pdata->wireless_charger_name);
	if (ret)
		pr_info("%s: Wireless charger name is Empty\n", __func__);

	ret = of_property_read_string(np,
		"battery,fgsrc_switch_name", (char const **)&pdata->fgsrc_switch_name);
	if (ret) {
		pdata->support_fgsrc_change = false;
		pr_info("%s: fgsrc_switch_name is Empty\n", __func__);
	}
	else
		pdata->support_fgsrc_change = true;

	ret = of_property_read_string(np,
		"battery,wireless_charger_name", (char const **)&pdata->wireless_charger_name);
	if (ret)
		pr_info("%s: Wireless charger name is Empty\n", __func__);

	ret = of_property_read_string(np,
		"battery,chip_vendor", (char const **)&pdata->chip_vendor);
	if (ret)
		pr_info("%s: Chip vendor is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,technology",
		&pdata->technology);
	if (ret)
		pr_info("%s : technology is Empty\n", __func__);

	ret = of_property_read_u32(np,
		"battery,wireless_cc_cv", &pdata->wireless_cc_cv);

	pdata->fake_capacity = of_property_read_bool(np,
						     "battery,fake_capacity");

	pdata->dis_auto_shipmode_temp_ctrl = of_property_read_bool(np,
						     "battery,dis_auto_shipmode_temp_ctrl");

	battery->ta_alert_wa = of_property_read_bool(np, "battery,ta_alert_wa");

	p = of_get_property(np, "battery,polling_time", &len);
	if (!p)
		return 1;

	len = len / sizeof(u32);
	pdata->polling_time = kzalloc(sizeof(*pdata->polling_time) * len, GFP_KERNEL);
	ret = of_property_read_u32_array(np, "battery,polling_time",
					 pdata->polling_time, len);
	if (ret)
		pr_info("%s : battery,polling_time is Empty\n", __func__);

	/* battery thermistor */
	ret = of_property_read_u32(np, "battery,thermal_source",
		&pdata->thermal_source);
	if (ret)
		pr_info("%s : Thermal source is Empty\n", __func__);

	if (pdata->thermal_source == SEC_BATTERY_THERMAL_SOURCE_ADC) {
		p = of_get_property(np, "battery,temp_table_adc", &len);
		if (!p)
			return 1;

		len = len / sizeof(u32);

		pdata->temp_adc_table_size = len;
		pdata->temp_amb_adc_table_size = len;

		pdata->temp_adc_table =
			kzalloc(sizeof(sec_bat_adc_table_data_t) *
				pdata->temp_adc_table_size, GFP_KERNEL);
		pdata->temp_amb_adc_table =
			kzalloc(sizeof(sec_bat_adc_table_data_t) *
				pdata->temp_adc_table_size, GFP_KERNEL);

		for(i = 0; i < pdata->temp_adc_table_size; i++) {
			ret = of_property_read_u32_index(np,
					 "battery,temp_table_adc", i, &temp);
			pdata->temp_adc_table[i].adc = (int)temp;
			if (ret)
				pr_info("%s : Temp_adc_table(adc) is Empty\n",
					__func__);

			ret = of_property_read_u32_index(np,
							 "battery,temp_table_data", i, &temp);
			pdata->temp_adc_table[i].data = (int)temp;
			if (ret)
				pr_info("%s : Temp_adc_table(data) is Empty\n",
					__func__);

			ret = of_property_read_u32_index(np,
							 "battery,temp_table_adc", i, &temp);
			pdata->temp_amb_adc_table[i].adc = (int)temp;
			if (ret)
				pr_info("%s : Temp_amb_adc_table(adc) is Empty\n",
					__func__);

			ret = of_property_read_u32_index(np,
							 "battery,temp_table_data", i, &temp);
			pdata->temp_amb_adc_table[i].data = (int)temp;
			if (ret)
				pr_info("%s : Temp_amb_adc_table(data) is Empty\n",
					__func__);
		}
	}
	ret = of_property_read_u32(np, "battery,temp_check_type",
		&pdata->temp_check_type);
	if (ret)
		pr_info("%s : Temp check type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,temp_check_count",
		&pdata->temp_check_count);
	if (ret)
		pr_info("%s : Temp check count is Empty\n", __func__);

	/* usb thermistor */
	ret = of_property_read_u32(np, "battery,usb_thermal_source",
		&pdata->usb_thermal_source);
	if (ret)
		pr_info("%s : usb_thermal_source is Empty\n", __func__);

	if(pdata->usb_thermal_source == SEC_BATTERY_THERMAL_SOURCE_ADC) {
		p = of_get_property(np, "battery,usb_temp_table_adc", &len);
		if (!p)
			return 1;

		len = len / sizeof(u32);

		pdata->usb_temp_adc_table_size = len;

		pdata->usb_temp_adc_table =
			kzalloc(sizeof(sec_bat_adc_table_data_t) *
				pdata->usb_temp_adc_table_size, GFP_KERNEL);

		for(i = 0; i < pdata->usb_temp_adc_table_size; i++) {
			ret = of_property_read_u32_index(np,
							 "battery,usb_temp_table_adc", i, &temp);
			pdata->usb_temp_adc_table[i].adc = (int)temp;
			if (ret)
				pr_info("%s : Usb_Temp_adc_table(adc) is Empty\n",
					__func__);

			ret = of_property_read_u32_index(np,
							 "battery,usb_temp_table_data", i, &temp);
			pdata->usb_temp_adc_table[i].data = (int)temp;
			if (ret)
				pr_info("%s : Usb_Temp_adc_table(data) is Empty\n",
					__func__);
		}
	}
	ret = of_property_read_u32(np, "battery,usb_temp_check_type",
		&pdata->usb_temp_check_type);
	if (ret)
		pr_info("%s : usb_temp_check_type is Empty\n", __func__);

	/* chg thermistor */
	ret = of_property_read_u32(np, "battery,chg_thermal_source",
		&pdata->chg_thermal_source);
	if (ret)
		pr_info("%s : chg_thermal_source is Empty\n", __func__);

	if(pdata->chg_thermal_source == SEC_BATTERY_THERMAL_SOURCE_ADC) {
		p = of_get_property(np, "battery,chg_temp_table_adc", &len);
		if (!p)
			return 1;

		len = len / sizeof(u32);

		pdata->chg_temp_adc_table_size = len;

		pdata->chg_temp_adc_table =
			kzalloc(sizeof(sec_bat_adc_table_data_t) *
				pdata->chg_temp_adc_table_size, GFP_KERNEL);

		for(i = 0; i < pdata->chg_temp_adc_table_size; i++) {
			ret = of_property_read_u32_index(np,
							 "battery,chg_temp_table_adc", i, &temp);
			pdata->chg_temp_adc_table[i].adc = (int)temp;
			if (ret)
				pr_info("%s : CHG_Temp_adc_table(adc) is Empty\n",
					__func__);

			ret = of_property_read_u32_index(np,
							 "battery,chg_temp_table_data", i, &temp);
			pdata->chg_temp_adc_table[i].data = (int)temp;
			if (ret)
				pr_info("%s : CHG_Temp_adc_table(data) is Empty\n",
					__func__);
		}
	}
	ret = of_property_read_u32(np, "battery,chg_temp_check_type",
		&pdata->chg_temp_check_type);
	if (ret)
		pr_info("%s : chg_temp_check_type is Empty\n", __func__);

#if defined(CONFIG_DIRECT_CHARGING)
	/* direct chg thermistor */
	ret = of_property_read_u32(np, "battery,dchg_thermal_source",
		&pdata->dchg_thermal_source);
	if (ret)
		pr_info("%s : dchg_thermal_source is Empty\n", __func__);

	if(pdata->dchg_thermal_source == SEC_BATTERY_THERMAL_SOURCE_CHG_ADC) {
		p = of_get_property(np, "battery,dchg_temp_table_adc", &len);
		if (!p)
			return 1;

		len = len / sizeof(u32);

		pdata->dchg_temp_adc_table_size = len;

		pdata->dchg_temp_adc_table =
			kzalloc(sizeof(sec_bat_adc_table_data_t) *
				pdata->dchg_temp_adc_table_size, GFP_KERNEL);

		for(i = 0; i < pdata->dchg_temp_adc_table_size; i++) {
			ret = of_property_read_u32_index(np,
							 "battery,dchg_temp_table_adc", i, &temp);
			pdata->dchg_temp_adc_table[i].adc = (int)temp;
			if (ret)
				pr_info("%s : DIRECT CHG_Temp_adc_table(adc) is Empty\n",
					__func__);

			ret = of_property_read_u32_index(np,
							 "battery,dchg_temp_table_data", i, &temp);
			pdata->dchg_temp_adc_table[i].data = (int)temp;
			if (ret)
				pr_info("%s : DIRECT CHG_Temp_adc_table(data) is Empty\n",
					__func__);
		}
	}
	ret = of_property_read_u32(np, "battery,dchg_temp_check_type",
		&pdata->dchg_temp_check_type);
	if (ret)
		pr_info("%s : dchg_temp_check_type is Empty\n", __func__);
#endif
	/* wpc thermistor */
	ret = of_property_read_u32(np, "battery,wpc_thermal_source",
		&pdata->wpc_thermal_source);
	if (ret)
		pr_info("%s : wpc_thermal_source is Empty\n", __func__);

	if(pdata->wpc_thermal_source == SEC_BATTERY_THERMAL_SOURCE_ADC) {
		p = of_get_property(np, "battery,wpc_temp_table_adc", &len);
		if (!p) {
			pr_info("%s : wpc_temp_table_adc(adc) is Empty\n",__func__);
		} else {
			len = len / sizeof(u32);

			pdata->wpc_temp_adc_table_size = len;

			pdata->wpc_temp_adc_table =
				kzalloc(sizeof(sec_bat_adc_table_data_t) *
					pdata->wpc_temp_adc_table_size, GFP_KERNEL);

			for(i = 0; i < pdata->wpc_temp_adc_table_size; i++) {
				ret = of_property_read_u32_index(np,
								 "battery,wpc_temp_table_adc", i, &temp);
				pdata->wpc_temp_adc_table[i].adc = (int)temp;
				if (ret)
					pr_info("%s : WPC_Temp_adc_table(adc) is Empty\n",
						__func__);

				ret = of_property_read_u32_index(np,
								 "battery,wpc_temp_table_data", i, &temp);
				pdata->wpc_temp_adc_table[i].data = (int)temp;
				if (ret)
					pr_info("%s : WPC_Temp_adc_table(data) is Empty\n",
						__func__);
			}
		}
	}
	ret = of_property_read_u32(np, "battery,wpc_temp_check_type",
		&pdata->wpc_temp_check_type);
	if (ret)
		pr_info("%s : wpc_temp_check_type is Empty\n", __func__);
	
	/* sub bat thermistor */
	ret = of_property_read_u32(np, "battery,sub_bat_thermal_source",
		&pdata->sub_bat_thermal_source);
	if (ret)
		pr_info("%s : sub_bat_thermal_source is Empty\n", __func__);

	if(pdata->sub_bat_thermal_source) {
		p = of_get_property(np, "battery,sub_bat_temp_table_adc", &len);
		if (!p) {
			pr_info("%s : sub_bat_temp_table_adc(adc) is Empty\n",__func__);
		} else {
			len = len / sizeof(u32);

			pdata->sub_bat_temp_adc_table_size = len;

			pdata->sub_bat_temp_adc_table =
				kzalloc(sizeof(sec_bat_adc_table_data_t) *
					pdata->sub_bat_temp_adc_table_size, GFP_KERNEL);

			for(i = 0; i < pdata->sub_bat_temp_adc_table_size; i++) {
				ret = of_property_read_u32_index(np,
								 "battery,sub_bat_temp_table_adc", i, &temp);
				pdata->sub_bat_temp_adc_table[i].adc = (int)temp;
				if (ret)
					pr_info("%s : sub_bat_adc_table(adc) is Empty\n",
						__func__);

				ret = of_property_read_u32_index(np,
								 "battery,sub_bat_temp_table_data", i, &temp);
				pdata->sub_bat_temp_adc_table[i].data = (int)temp;
				if (ret)
					pr_info("%s : sub_bat_temp_adc_table(data) is Empty\n",
						__func__);
			}
		}
	}
	ret = of_property_read_u32(np, "battery,sub_bat_temp_check_type",
		&pdata->sub_bat_temp_check_type);
	if (ret)
		pr_info("%s : sub_bat_temp_check_type is Empty\n", __func__);

	/* slave thermistor */
	ret = of_property_read_u32(np, "battery,slave_thermal_source",
		&pdata->slave_thermal_source);
	if (ret)
		pr_info("%s : slave_thermal_source is Empty\n", __func__);

	if(pdata->slave_thermal_source == SEC_BATTERY_THERMAL_SOURCE_ADC) {
		p = of_get_property(np, "battery,slave_chg_temp_table_adc", &len);
		if (!p)
			return 1;

		len = len / sizeof(u32);

		pdata->slave_chg_temp_adc_table_size = len;

		pdata->slave_chg_temp_adc_table =
			kzalloc(sizeof(sec_bat_adc_table_data_t) *
				pdata->slave_chg_temp_adc_table_size, GFP_KERNEL);

		for(i = 0; i < pdata->slave_chg_temp_adc_table_size; i++) {
			ret = of_property_read_u32_index(np,
							 "battery,slave_chg_temp_table_adc", i, &temp);
			pdata->slave_chg_temp_adc_table[i].adc = (int)temp;
			if (ret)
				pr_info("%s : slave_chg_temp_adc_table(adc) is Empty\n",
					__func__);

			ret = of_property_read_u32_index(np,
							 "battery,slave_chg_temp_table_data", i, &temp);
			pdata->slave_chg_temp_adc_table[i].data = (int)temp;
			if (ret)
				pr_info("%s : slave_chg_temp_adc_table(data) is Empty\n",
					__func__);
		}
	}
	ret = of_property_read_u32(np, "battery,slave_chg_temp_check_type",
		&pdata->slave_chg_temp_check_type);
	if (ret)
		pr_info("%s : slave_chg_temp_check_type is Empty\n", __func__);

	if (pdata->chg_temp_check_type) {
		ret = of_property_read_u32(np, "battery,chg_12v_high_temp",
					   &temp);
		pdata->chg_12v_high_temp = (int)temp;
		if (ret)
			pr_info("%s : chg_12v_high_temp is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,chg_high_temp",
					   &temp);
		pdata->chg_high_temp = (int)temp;
		if (ret)
			pr_info("%s : chg_high_temp is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,chg_high_temp_recovery",
					   &temp);
		pdata->chg_high_temp_recovery = (int)temp;
		if (ret)
			pr_info("%s : chg_temp_recovery is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,chg_charging_limit_current",
					   &pdata->chg_charging_limit_current);
		if (ret)
			pr_info("%s : chg_charging_limit_current is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,chg_input_limit_current",
					   &pdata->chg_input_limit_current);
		if (ret)
			pr_info("%s : chg_input_limit_current is Empty\n", __func__);

#if defined(CONFIG_DIRECT_CHARGING)
		if (pdata->dchg_temp_check_type) {
			ret = of_property_read_u32(np, "battery,dchg_high_temp",
						   &temp);
			pdata->dchg_high_temp = (int)temp;
			if (ret) {
				pr_info("%s : dchg_high_temp is Empty\n", __func__);
				pdata->dchg_high_temp = pdata->chg_high_temp;
			}
			ret = of_property_read_u32(np, "battery,dchg_high_temp_recovery",
						   &temp);
			pdata->dchg_high_temp_recovery = (int)temp;
			if (ret) {
				pr_info("%s : dchg_temp_recovery is Empty\n", __func__);
				pdata->dchg_high_temp_recovery = pdata->chg_high_temp_recovery;
			}

			ret = of_property_read_u32(np, "battery,dchg_charging_limit_current",
						   &pdata->dchg_charging_limit_current);
			if (ret) {
				pr_info("%s : dchg_charging_limit_current is Empty\n", __func__);
				pdata->dchg_charging_limit_current = pdata->chg_charging_limit_current;
			}

			ret = of_property_read_u32(np, "battery,dchg_input_limit_current",
						   &pdata->dchg_input_limit_current);
			if (ret) {
				pr_info("%s : dchg_input_limit_current is Empty\n", __func__);
				pdata->dchg_input_limit_current = pdata->chg_input_limit_current;
			}

			ret = of_property_read_u32(np, "battery,dchg_high_batt_temp",
						   &temp);
			pdata->dchg_high_batt_temp = (int)temp;
			if (ret) {
				pr_info("%s : dchg_high_batt_temp is Empty\n", __func__);
				pdata->dchg_high_batt_temp = pdata->chg_high_temp;
			}
			ret = of_property_read_u32(np, "battery,dchg_high_batt_temp_recovery",
						   &temp);
			pdata->dchg_high_batt_temp_recovery = (int)temp;
			if (ret) {
				pr_info("%s : dchg_high_batt_temp_recovery is Empty\n", __func__);
				pdata->dchg_high_batt_temp_recovery = pdata->chg_high_temp_recovery;
			}
		}
#endif
		ret = of_property_read_u32(np, "battery,mix_high_temp",
					   &temp);
		pdata->mix_high_temp = (int)temp;
		if (ret)
			pr_info("%s : mix_high_temp is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,mix_high_chg_temp",
					   &temp);
		pdata->mix_high_chg_temp = (int)temp;
		if (ret)
			pr_info("%s : mix_high_chg_temp is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,mix_high_temp_recovery",
					   &temp);
		pdata->mix_high_temp_recovery = (int)temp;
		if (ret)
			pr_info("%s : mix_high_temp_recovery is Empty\n", __func__);
	}

	if (pdata->wpc_temp_check_type) {
		ret = of_property_read_u32(np, "battery,wpc_temp_control_source",
				&pdata->wpc_temp_control_source);
		if (ret) {
			pr_info("%s : wpc_temp_control_source is Empty\n", __func__);
			pdata->wpc_temp_control_source = TEMP_CONTROL_SOURCE_CHG_THM;
		}

		ret = of_property_read_u32(np, "battery,wpc_temp_lcd_on_control_source",
				&pdata->wpc_temp_lcd_on_control_source);
		if (ret) {
			pr_info("%s : wpc_temp_lcd_on_control_source is Empty\n", __func__);
			pdata->wpc_temp_lcd_on_control_source = TEMP_CONTROL_SOURCE_CHG_THM;
		}

		ret = of_property_read_u32(np, "battery,wpc_high_temp",
				&pdata->wpc_high_temp);
		if (ret)
			pr_info("%s : wpc_high_temp is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,wpc_high_temp_recovery",
				&pdata->wpc_high_temp_recovery);
		if (ret)
			pr_info("%s : wpc_high_temp_recovery is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,wpc_input_limit_current",
				&pdata->wpc_input_limit_current);
		if (ret)
			pr_info("%s : wpc_input_limit_current is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,wpc_charging_limit_current",
				&pdata->wpc_charging_limit_current);
		if (ret)
			pr_info("%s : wpc_charging_limit_current is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,wpc_lcd_on_high_temp",
				&pdata->wpc_lcd_on_high_temp);
		if (ret)
			pr_info("%s : wpc_lcd_on_high_temp is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,wpc_lcd_on_high_temp_rec",
				&pdata->wpc_lcd_on_high_temp_rec);
		if (ret)
			pr_info("%s : wpc_lcd_on_high_temp_rec is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,wpc_lcd_on_input_limit_current",
				&pdata->wpc_lcd_on_input_limit_current);
		if (ret) {
			pr_info("%s : wpc_lcd_on_input_limit_current is Empty\n", __func__);
			pdata->wpc_lcd_on_input_limit_current =
				pdata->wpc_input_limit_current;
		}

                battery->wpc_vout_ctrl_lcd_on = of_property_read_bool(np,
						     "battery,wpc_vout_ctrl_lcd_on");
		battery->support_unknown_wpcthm = of_property_read_bool(np,
						     "battery,support_unknown_wpcthm");
	}

	ret = of_property_read_u32(np, "battery,wc_full_input_limit_current",
		&pdata->wc_full_input_limit_current);
	if (ret)
		pr_info("%s : wc_full_input_limit_current is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,wc_hero_stand_cc_cv",
		&pdata->wc_hero_stand_cc_cv);
	if (ret) {
		pr_info("%s : wc_hero_stand_cc_cv is Empty\n", __func__);
		pdata->wc_hero_stand_cc_cv = 70;
	}
	ret = of_property_read_u32(np, "battery,wc_hero_stand_cv_current",
		&pdata->wc_hero_stand_cv_current);
	if (ret) {
		pr_info("%s : wc_hero_stand_cv_current is Empty\n", __func__);
		pdata->wc_hero_stand_cv_current = 600;
	}
	ret = of_property_read_u32(np, "battery,wc_hero_stand_hv_cv_current",
		&pdata->wc_hero_stand_hv_cv_current);
	if (ret) {
		pr_info("%s : wc_hero_stand_hv_cv_current is Empty\n", __func__);
		pdata->wc_hero_stand_hv_cv_current = 450;
	}

	ret = of_property_read_u32(np, "battery,sleep_mode_limit_current",
			&pdata->sleep_mode_limit_current);
	if (ret)
		pr_info("%s : sleep_mode_limit_current is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,inbat_voltage",
			&pdata->inbat_voltage);
	if (ret)
		pr_info("%s : inbat_voltage is Empty\n", __func__);

	if (pdata->inbat_voltage) {
		p = of_get_property(np, "battery,inbat_voltage_table_adc", &len);
		if (!p)
			return 1;

		len = len / sizeof(u32);

		pdata->inbat_adc_table_size = len;

		pdata->inbat_adc_table =
			kzalloc(sizeof(sec_bat_adc_table_data_t) *
					pdata->inbat_adc_table_size, GFP_KERNEL);

		for(i = 0; i < pdata->inbat_adc_table_size; i++) {
			ret = of_property_read_u32_index(np,
							 "battery,inbat_voltage_table_adc", i, &temp);
			pdata->inbat_adc_table[i].adc = (int)temp;
			if (ret)
				pr_info("%s : inbat_adc_table(adc) is Empty\n",
						__func__);

			ret = of_property_read_u32_index(np,
							 "battery,inbat_voltage_table_data", i, &temp);
			pdata->inbat_adc_table[i].data = (int)temp;
			if (ret)
				pr_info("%s : inbat_adc_table(data) is Empty\n",
						__func__);
		}
	}

	ret = of_property_read_u32(np, "battery,pre_afc_input_current",
		&pdata->pre_afc_input_current);
	if (ret) {
		pr_info("%s : pre_afc_input_current is Empty\n", __func__);
		pdata->pre_afc_input_current = 1000;
	}

	ret = of_property_read_u32(np, "battery,pre_afc_work_delay",
			&pdata->pre_afc_work_delay);
	if (ret) {
		pr_info("%s : pre_afc_work_delay is Empty\n", __func__);
		pdata->pre_afc_work_delay = 2000;
	}

	ret = of_property_read_u32(np, "battery,pre_wc_afc_input_current",
		&pdata->pre_wc_afc_input_current);
	if (ret) {
		pr_info("%s : pre_wc_afc_input_current is Empty\n", __func__);
		pdata->pre_wc_afc_input_current = 500; /* wc input default */
	}

	ret = of_property_read_u32(np, "battery,pre_wc_afc_work_delay",
			&pdata->pre_wc_afc_work_delay);
	if (ret) {
		pr_info("%s : pre_wc_afc_work_delay is Empty\n", __func__);
		pdata->pre_wc_afc_work_delay = 4000;
	}

	ret = of_property_read_u32(np, "battery,prepare_ta_delay",
			&pdata->prepare_ta_delay);
	if (ret) {
		pr_info("%s : prepare_ta_delay is Empty\n", __func__);
		pdata->prepare_ta_delay = 500;
	}

	ret = of_property_read_u32(np, "battery,tx_stop_capacity",
		&pdata->tx_stop_capacity);
	if (ret) {
		pr_info("%s : tx_stop_capacity is Empty\n", __func__);
		pdata->tx_stop_capacity = 30;
	}

	ret = of_property_read_u32(np, "battery,adc_check_count",
		&pdata->adc_check_count);
	if (ret)
		pr_info("%s : Adc check count is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,temp_adc_type",
		&pdata->temp_adc_type);
	if (ret)
		pr_info("%s : Temp adc type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,cable_check_type",
		&pdata->cable_check_type);
	if (ret)
		pr_info("%s : Cable check type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,cable_source_type",
		&pdata->cable_source_type);
	if (ret)
		pr_info("%s: Cable_source_type is Empty\n", __func__);
#if defined(CONFIG_CHARGING_VZWCONCEPT)
	pdata->cable_check_type &= ~SEC_BATTERY_CABLE_CHECK_NOUSBCHARGE;
	pdata->cable_check_type |= SEC_BATTERY_CABLE_CHECK_NOINCOMPATIBLECHARGE;
#endif
	ret = of_property_read_u32(np, "battery,polling_type",
		&pdata->polling_type);
	if (ret)
		pr_info("%s : Polling type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,monitor_initial_count",
		&pdata->monitor_initial_count);
	if (ret)
		pr_info("%s : Monitor initial count is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,battery_check_type",
		&pdata->battery_check_type);
	if (ret)
		pr_info("%s : Battery check type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,check_count",
		&pdata->check_count);
	if (ret)
		pr_info("%s : Check count is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,check_adc_max",
		&pdata->check_adc_max);
	if (ret)
		pr_info("%s : Check adc max is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,check_adc_min",
		&pdata->check_adc_min);
	if (ret)
		pr_info("%s : Check adc min is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,ovp_uvlo_check_type",
		&pdata->ovp_uvlo_check_type);
	if (ret)
		pr_info("%s : Ovp Uvlo check type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,temp_highlimit_threshold_normal",
				   &temp);
	pdata->temp_highlimit_threshold_normal =  (int)temp;
	if (ret)
		pr_info("%s : Temp highlimit threshold normal is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,temp_highlimit_recovery_normal",
				   &temp);
	pdata->temp_highlimit_recovery_normal =  (int)temp;
	if (ret)
		pr_info("%s : Temp highlimit recovery normal is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,temp_high_threshold_normal",
				   &temp);
	pdata->temp_high_threshold_normal =  (int)temp;
	if (ret)
		pr_info("%s : Temp high threshold normal is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,temp_high_recovery_normal",
				   &temp);
	pdata->temp_high_recovery_normal =  (int)temp;
	if (ret)
		pr_info("%s : Temp high recovery normal is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,temp_low_threshold_normal",
				   &temp);
	pdata->temp_low_threshold_normal =  (int)temp;
	if (ret)
		pr_info("%s : Temp low threshold normal is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,temp_low_recovery_normal",
				   &temp);
	pdata->temp_low_recovery_normal =  (int)temp;
	if (ret)
		pr_info("%s : Temp low recovery normal is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,temp_highlimit_threshold_lpm",
				   &temp);
	pdata->temp_highlimit_threshold_lpm = (int)temp;
	if (ret)
		pr_info("%s : Temp highlimit threshold lpm is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,temp_highlimit_recovery_lpm",
				   &temp);
	pdata->temp_highlimit_recovery_lpm = (int)temp;
	if (ret)
		pr_info("%s : Temp highlimit recovery lpm is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,temp_high_threshold_lpm",
				   &temp);
	pdata->temp_high_threshold_lpm = (int)temp;
	if (ret)
		pr_info("%s : Temp high threshold lpm is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,temp_high_recovery_lpm",
				   &temp);
	pdata->temp_high_recovery_lpm = (int)temp;
	if (ret)
		pr_info("%s : Temp high recovery lpm is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,temp_low_threshold_lpm",
				   &temp);
	pdata->temp_low_threshold_lpm = (int)temp;
	if (ret)
		pr_info("%s : Temp low threshold lpm is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,temp_low_recovery_lpm",
				   &temp);
	pdata->temp_low_recovery_lpm = (int)temp;
	if (ret)
		pr_info("%s : Temp low recovery lpm is Empty\n", __func__);

	pr_info("%s : HIGHLIMIT_THRESHOLD_NOLMAL(%d), HIGHLIMIT_RECOVERY_NORMAL(%d)\n"
		"HIGH_THRESHOLD_NORMAL(%d), HIGH_RECOVERY_NORMAL(%d) LOW_THRESHOLD_NORMAL(%d), LOW_RECOVERY_NORMAL(%d)\n"
		"HIGHLIMIT_THRESHOLD_LPM(%d), HIGHLIMIT_RECOVERY_LPM(%d)\n"
		"HIGH_THRESHOLD_LPM(%d), HIGH_RECOVERY_LPM(%d) LOW_THRESHOLD_LPM(%d), LOW_RECOVERY_LPM(%d)\n",
		__func__,
		pdata->temp_highlimit_threshold_normal, pdata->temp_highlimit_recovery_normal,
		pdata->temp_high_threshold_normal, pdata->temp_high_recovery_normal,
		pdata->temp_low_threshold_normal, pdata->temp_low_recovery_normal,
		pdata->temp_highlimit_threshold_lpm, pdata->temp_highlimit_recovery_lpm,
		pdata->temp_high_threshold_lpm, pdata->temp_high_recovery_lpm,
		pdata->temp_low_threshold_lpm, pdata->temp_low_recovery_lpm);

	ret = of_property_read_u32(np, "battery,wpc_high_threshold_normal",
				   &temp);
	pdata->wpc_high_threshold_normal =  (int)temp;
	if (ret)
		pr_info("%s : wpc_high_threshold_normal is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,wpc_high_recovery_normal",
				   &temp);
	pdata->wpc_high_recovery_normal =  (int)temp;
	if (ret)
		pr_info("%s : wpc_high_recovery_normal is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,wpc_low_threshold_normal",
				   &temp);
	pdata->wpc_low_threshold_normal =  (int)temp;
	if (ret)
		pr_info("%s : wpc_low_threshold_normal is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,wpc_low_recovery_normal",
				   &temp);
	pdata->wpc_low_recovery_normal =  (int)temp;
	if (ret)
		pr_info("%s : wpc_low_recovery_normal is Empty\n", __func__);

#if defined(CONFIG_PREVENT_USB_CONN_OVERHEAT)
	ret = of_property_read_u32(np, "battery,usb_protection_temp",
				   &temp);
	battery->usb_protection_temp = (int)temp;
	if (ret) {
		pr_info("%s : usb protection temp value is Empty\n", __func__);
		battery->usb_protection_temp = 610;
	}

	ret = of_property_read_u32(np, "battery,temp_gap_bat_usb",
				   &temp);
	battery->temp_gap_bat_usb = (int)temp;
	if (ret) {
		pr_info("%s : temp gap value is Empty\n", __func__);
		battery->temp_gap_bat_usb = 200;
	}
#endif
	ret = of_property_read_u32(np, "battery,tx_high_threshold",
				   &temp);
	pdata->tx_high_threshold = (int)temp;
	if (ret) {
		pr_info("%s : tx_high_threshold is Empty\n", __func__);
		pdata->tx_high_threshold = 450;
	}

	ret = of_property_read_u32(np, "battery,tx_high_recovery",
				   &temp);
	pdata->tx_high_recovery = (int)temp;
	if (ret) {
		pr_info("%s : tx_high_recovery is Empty\n", __func__);
		pdata->tx_high_recovery = 400;
	}

	ret = of_property_read_u32(np, "battery,tx_low_threshold",
				   &temp);
	pdata->tx_low_threshold = (int)temp;
	if (ret) {
		pr_info("%s : tx_low_threshold is Empty\n", __func__);
		pdata->tx_low_recovery = 0;
	}
	ret = of_property_read_u32(np, "battery,tx_low_recovery",
				   &temp);
	pdata->tx_low_recovery = (int)temp;
	if (ret) {
		pr_info("%s : tx_low_recovery is Empty\n", __func__);
		pdata->tx_low_recovery = 50;
	}

	ret = of_property_read_u32(np, "battery,charging_limit_by_tx_check",
				   &pdata->charging_limit_by_tx_check);
	if (ret)
		pr_info("%s : charging_limit_by_tx_check is Empty\n", __func__);

	if(pdata->charging_limit_by_tx_check) {
		ret = of_property_read_u32(np, "battery,charging_limit_current_by_tx",
					   &pdata->charging_limit_current_by_tx);
		if (ret) {
			pr_info("%s : charging_limit_current_by_tx is Empty\n", __func__);
			pdata->charging_limit_current_by_tx = 1000;
		}
	}

	ret = of_property_read_u32(np, "battery,wpc_input_limit_by_tx_check",
				   &pdata->wpc_input_limit_by_tx_check);
	if (ret)
		pr_info("%s : wpc_input_limit_by_tx_check is Empty\n", __func__);
		
	if(pdata->wpc_input_limit_by_tx_check) {
		ret = of_property_read_u32(np, "battery,wpc_input_limit_current_by_tx",
				&pdata->wpc_input_limit_current_by_tx);
		if (ret) {
			pr_info("%s : wpc_input_limit_current_by_tx is Empty\n", __func__);
			pdata->wpc_input_limit_current_by_tx =
				pdata->wpc_input_limit_current;
		}
	}	

	ret = of_property_read_u32(np, "battery,full_check_type",
		&pdata->full_check_type);
	if (ret)
		pr_info("%s : Full check type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,full_check_type_2nd",
		&pdata->full_check_type_2nd);
	if (ret)
		pr_info("%s : Full check type 2nd is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,full_check_count",
		&pdata->full_check_count);
	if (ret)
		pr_info("%s : Full check count is Empty\n", __func__);

        ret = of_property_read_u32(np, "battery,chg_gpio_full_check",
                &pdata->chg_gpio_full_check);
	if (ret)
		pr_info("%s : Chg gpio full check is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,chg_polarity_full_check",
		&pdata->chg_polarity_full_check);
	if (ret)
		pr_info("%s : Chg polarity full check is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,full_condition_type",
		&pdata->full_condition_type);
	if (ret)
		pr_info("%s : Full condition type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,full_condition_soc",
		&pdata->full_condition_soc);
	if (ret)
		pr_info("%s : Full condition soc is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,full_condition_vcell",
		&pdata->full_condition_vcell);
	if (ret)
		pr_info("%s : Full condition vcell is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,recharge_check_count",
		&pdata->recharge_check_count);
	if (ret)
		pr_info("%s : Recharge check count is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,recharge_condition_type",
		&pdata->recharge_condition_type);
	if (ret)
		pr_info("%s : Recharge condition type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,recharge_condition_soc",
		&pdata->recharge_condition_soc);
	if (ret)
		pr_info("%s : Recharge condition soc is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,recharge_condition_vcell",
		&pdata->recharge_condition_vcell);
	if (ret)
		pr_info("%s : Recharge condition vcell is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,charging_total_time",
		(unsigned int *)&pdata->charging_total_time);
	if (ret)
		pr_info("%s : Charging total time is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,hv_charging_total_time",
				   &pdata->hv_charging_total_time);
	if (ret) {
		pdata->hv_charging_total_time = 3 * 60 * 60;
		pr_info("%s : HV Charging total time is %d\n",
			__func__, pdata->hv_charging_total_time);
	}

	ret = of_property_read_u32(np, "battery,normal_charging_total_time",
				   &pdata->normal_charging_total_time);
	if (ret) {
		pdata->normal_charging_total_time = 5 * 60 * 60;
		pr_info("%s : Normal(WC) Charging total time is %d\n",
			__func__, pdata->normal_charging_total_time);
	}

	ret = of_property_read_u32(np, "battery,usb_charging_total_time",
				   &pdata->usb_charging_total_time);
	if (ret) {
		pdata->usb_charging_total_time = 10 * 60 * 60;
		pr_info("%s : USB Charging total time is %d\n",
			__func__, pdata->usb_charging_total_time);
	}

	ret = of_property_read_u32(np, "battery,recharging_total_time",
		(unsigned int *)&pdata->recharging_total_time);
	if (ret)
		pr_info("%s : Recharging total time is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,charging_reset_time",
		(unsigned int *)&pdata->charging_reset_time);
	if (ret)
		pr_info("%s : Charging reset time is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,chg_float_voltage",
		(unsigned int *)&pdata->chg_float_voltage);
	if (ret) {
		pr_info("%s: chg_float_voltage is Empty\n", __func__);
		pdata->chg_float_voltage = 43500;
	}

	ret = of_property_read_u32(np, "battery,chg_float_voltage_conv",
				   &pdata->chg_float_voltage_conv);
	if (ret) {
		pr_info("%s: chg_float_voltage_conv is Empty\n", __func__);
		pdata->chg_float_voltage_conv = 1;
	}
#if defined(CONFIG_BATTERY_SWELLING)
	ret = of_property_read_u32(np, "battery,chg_float_voltage",
		(unsigned int *)&pdata->swelling_normal_float_voltage);
	if (ret)
		pr_info("%s: chg_float_voltage is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,swelling_high_temp_block",
				   &temp);
	pdata->swelling_high_temp_block = (int)temp;
	if (ret)
		pr_info("%s: swelling high temp block is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,swelling_high_temp_recov",
				   &temp);
	pdata->swelling_high_temp_recov = (int)temp;
	if (ret)
		pr_info("%s: swelling high temp recovery is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,swelling_wc_high_temp_block",
				   &temp);
	pdata->swelling_wc_high_temp_block = (int)temp;
	if (ret) {
		pdata->swelling_wc_high_temp_block = pdata->swelling_high_temp_block;
		pr_info("%s: swelling wireless high temp block is %d\n",
			__func__, pdata->swelling_wc_high_temp_block);
	}

	ret = of_property_read_u32(np, "battery,swelling_wc_high_temp_recov",
				   &temp);
	pdata->swelling_wc_high_temp_recov = (int)temp;
	if (ret) {
		pdata->swelling_wc_high_temp_recov = pdata->swelling_high_temp_recov;
		pr_info("%s: swelling wireless high temp recovery is %d\n",
			__func__, pdata->swelling_wc_high_temp_recov);
	}

	ret = of_property_read_u32(np, "battery,swelling_low_temp_block_1st",
				   &temp);
	pdata->swelling_low_temp_block_1st = (int)temp;
	if (ret)
		pr_info("%s: swelling low temp block is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,swelling_low_temp_recov_1st",
				   &temp);
	pdata->swelling_low_temp_recov_1st = (int)temp;
	if (ret)
		pr_info("%s: swelling low temp recovery is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,swelling_low_temp_block_2nd",
				   &temp);
	pdata->swelling_low_temp_block_2nd = (int)temp;
	if (ret)
		pr_info("%s: swelling low temp block is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,swelling_low_temp_recov_2nd",
				   &temp);
	pdata->swelling_low_temp_recov_2nd = (int)temp;
	if (ret)
		pr_info("%s: swelling low temp recovery 2nd is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,swelling_low_temp_current",
					&pdata->swelling_low_temp_current);
	if (ret) {
		pr_info("%s: swelling_low_temp_current is Empty, Defualt value 600mA \n", __func__);
		pdata->swelling_low_temp_current = 600;
	}

	ret = of_property_read_u32(np, "battery,swelling_low_temp_current_2nd",
					&pdata->swelling_low_temp_current_2nd);
	if (ret) {
		pr_info("%s: swelling_low_temp_current_2nd is Empty, set swelling_low_temp_current value \n", __func__);
		pdata->swelling_low_temp_current_2nd = pdata->swelling_low_temp_current;
	}

	ret = of_property_read_u32(np, "battery,swelling_low_temp_topoff",
					&pdata->swelling_low_temp_topoff);
	if (ret) {
		pr_info("%s: swelling_low_temp_topoff is Empty, Defualt value 200mA \n", __func__);
		pdata->swelling_low_temp_topoff = 200;
	}

	ret = of_property_read_u32(np, "battery,swelling_high_temp_current",
					&pdata->swelling_high_temp_current);
	if (ret) {
		pr_info("%s: swelling_high_temp_current is Empty, Defualt value 1300mA \n", __func__);
		pdata->swelling_high_temp_current = 1300;
	}

	ret = of_property_read_u32(np, "battery,swelling_high_temp_topoff",
					&pdata->swelling_high_temp_topoff);
	if (ret) {
		pr_info("%s: swelling_high_temp_topoff is Empty, Defualt value 200mA \n", __func__);
		pdata->swelling_high_temp_topoff = 200;
	}

	ret = of_property_read_u32(np, "battery,swelling_wc_high_temp_current",
					&pdata->swelling_wc_high_temp_current);
	if (ret) {
		pr_info("%s: swelling_wc_high_temp_current is Empty, Defualt value 600mA \n", __func__);
		pdata->swelling_wc_high_temp_current = 600;
	}

	ret = of_property_read_u32(np, "battery,swelling_wc_low_temp_current",
					&pdata->swelling_wc_low_temp_current);
	if (ret) {
		pr_info("%s: swelling_wc_low_temp_current is Empty, Defualt value 600mA \n", __func__);
		pdata->swelling_wc_low_temp_current = 600;
	}

	ret = of_property_read_u32(np, "battery,swelling_wc_low_temp_current_2nd",
					&pdata->swelling_wc_low_temp_current_2nd);
	if (ret) {
		pr_info("%s: swelling_wc_low_temp_current_2nd is Empty, set swelling_wc_low_temp_current \n", __func__);
		pdata->swelling_wc_low_temp_current_2nd = pdata->swelling_low_temp_current;
	}


	battery->swelling_low_temp_3rd_ctrl = of_property_read_bool(np,
						     "battery,swelling_low_temp_3rd_ctrl");
	if (battery->swelling_low_temp_3rd_ctrl) {
		ret = of_property_read_u32(np, "battery,swelling_low_temp_block_3rd",
					   &temp);
		pdata->swelling_low_temp_block_3rd = (int)temp;
		if (ret) {
			pr_info("%s: swelling low temp block is Empty\n", __func__);
			pdata->swelling_low_temp_block_3rd = pdata->swelling_low_temp_block_2nd;
		}

		ret = of_property_read_u32(np, "battery,swelling_low_temp_recov_3rd",
					   &temp);
		pdata->swelling_low_temp_recov_3rd = (int)temp;
		if (ret) {
			pr_info("%s: swelling low temp recovery 3rd is Empty\n", __func__);
			pdata->swelling_low_temp_recov_3rd = pdata->swelling_low_temp_recov_2nd;
		}

		ret = of_property_read_u32(np, "battery,swelling_low_temp_current_3rd",
						&pdata->swelling_low_temp_current_3rd);
		if (ret) {
			pr_info("%s: swelling_low_temp_current_3rd is Empty, set swelling_low_temp_current value \n", __func__);
			pdata->swelling_low_temp_current_3rd = pdata->swelling_low_temp_current_2nd;
		}

		ret = of_property_read_u32(np, "battery,swelling_wc_low_temp_current_3rd",
						&pdata->swelling_wc_low_temp_current_3rd);
		if (ret) {
			pr_info("%s: swelling_wc_low_temp_current_3rd is Empty, set swelling_wc_low_temp_current \n", __func__);
			pdata->swelling_wc_low_temp_current_3rd = pdata->swelling_low_temp_current_2nd;
		}
	}

	battery->swelling_low_temp_4th_ctrl = of_property_read_bool(np,
						     "battery,swelling_low_temp_4th_ctrl");
	if (battery->swelling_low_temp_4th_ctrl) {
		ret = of_property_read_u32(np, "battery,swelling_low_temp_block_4th",
					   &temp);
		pdata->swelling_low_temp_block_4th = (int)temp;
		if (ret) {
			pr_info("%s: swelling low temp block is Empty\n", __func__);
			pdata->swelling_low_temp_block_4th = pdata->swelling_low_temp_block_3rd;
		}

		ret = of_property_read_u32(np, "battery,swelling_low_temp_recov_4th",
					   &temp);
		pdata->swelling_low_temp_recov_4th = (int)temp;
		if (ret) {
			pr_info("%s: swelling low temp recovery 4th is Empty\n", __func__);
			pdata->swelling_low_temp_recov_4th = pdata->swelling_low_temp_recov_3rd;
		}

		ret = of_property_read_u32(np, "battery,swelling_low_temp_current_4th",
						&pdata->swelling_low_temp_current_4th);
		if (ret) {
			pr_info("%s: swelling_low_temp_current_4th is Empty, set swelling_low_temp_current value \n", __func__);
			pdata->swelling_low_temp_current_4th = pdata->swelling_low_temp_current_3rd;
		}
	}

	battery->swelling_low_temp_5th_ctrl = of_property_read_bool(np,
						     "battery,swelling_low_temp_5th_ctrl");
	if (battery->swelling_low_temp_5th_ctrl) {
		ret = of_property_read_u32(np, "battery,swelling_low_temp_block_5th",
					   &temp);
		pdata->swelling_low_temp_block_5th = (int)temp;
		if (ret) {
			pr_info("%s: swelling low temp block is Empty\n", __func__);
			pdata->swelling_low_temp_block_5th = pdata->swelling_low_temp_block_3rd;
		}

		ret = of_property_read_u32(np, "battery,swelling_low_temp_recov_5th",
					   &temp);
		pdata->swelling_low_temp_recov_5th = (int)temp;
		if (ret) {
			pr_info("%s: swelling low temp recovery 5th is Empty\n", __func__);
			pdata->swelling_low_temp_recov_5th = pdata->swelling_low_temp_recov_3rd;
		}

		ret = of_property_read_u32(np, "battery,swelling_low_temp_current_5th",
						&pdata->swelling_low_temp_current_5th);
		if (ret) {
			pr_info("%s: swelling_low_temp_current_5th is Empty, set swelling_low_temp_current value \n", __func__);
			pdata->swelling_low_temp_current_5th = pdata->swelling_low_temp_current_3rd;
		}
	}

	ret = of_property_read_u32(np, "battery,swelling_drop_float_voltage",
		(unsigned int *)&pdata->swelling_drop_float_voltage);
	if (ret) {
		pr_info("%s: swelling drop float voltage is Empty, Default value 4250mV \n", __func__);
		pdata->swelling_drop_float_voltage = 4250;
		pdata->swelling_drop_voltage_condition = 4250;
	} else {
		pdata->swelling_drop_voltage_condition = (pdata->swelling_drop_float_voltage > 10000) ?
			(pdata->swelling_drop_float_voltage / 10) : (pdata->swelling_drop_float_voltage);
		pr_info("%s : swelling drop voltage(set : %d, condition : %d)\n", __func__,
			pdata->swelling_drop_float_voltage, pdata->swelling_drop_voltage_condition);
	}

	ret = of_property_read_u32(np, "battery,swelling_high_rechg_voltage",
		(unsigned int *)&pdata->swelling_high_rechg_voltage);
	if (ret) {
		pr_info("%s: swelling_high_rechg_voltage is Empty\n", __func__);
		pdata->swelling_high_rechg_voltage = 4150;
	}

	ret = of_property_read_u32(np, "battery,swelling_low_rechg_voltage",
		(unsigned int *)&pdata->swelling_low_rechg_voltage);
	if (ret) {
		pr_info("%s: swelling_low_rechg_voltage is Empty\n", __func__);
				pdata->swelling_low_rechg_voltage = 4000;
	}

	ret = of_property_read_u32(np, "battery,swelling_low_rechg_thr",
		(unsigned int *)&pdata->swelling_low_rechg_thr);
	if (ret) {
		pr_info("%s: swelling_low_rechg_thr is Empty\n", __func__);
		pdata->swelling_low_rechg_thr = 150;
	}

	pr_info("%s : SWELLING_HIGH_TEMP(%d) SWELLING_HIGH_TEMP_RECOVERY(%d)\n"
		"SWELLING_LOW_TEMP_1st(%d) SWELLING_LOW_TEMP_RECOVERY_1st(%d) "
		"SWELLING_LOW_TEMP_2nd(%d) SWELLING_LOW_TEMP_RECOVERY_2nd(%d) "
		"SWELLING_LOW_CURRENT(%d, %d), SWELLING_HIGH_CURRENT(%d, %d)\n"
		"SWELLING_HIGH_TEMP_RCHG_VOL(%d), SWELLING_LOW_TEMP_RCHG_THRESHOLD(%d)\n",
		__func__, pdata->swelling_high_temp_block, pdata->swelling_high_temp_recov,
		pdata->swelling_low_temp_block_1st, pdata->swelling_low_temp_recov_1st,
		pdata->swelling_low_temp_block_2nd, pdata->swelling_low_temp_recov_2nd,
		pdata->swelling_low_temp_current, pdata->swelling_low_temp_topoff,
		pdata->swelling_high_temp_current, pdata->swelling_high_temp_topoff,
		pdata->swelling_high_rechg_voltage, pdata->swelling_low_rechg_thr);

	if(battery->swelling_low_temp_3rd_ctrl)
		pr_info("%s : SWELLING_LOW_TEMP_3rd(%d) SWELLING_LOW_TEMP_RECOVERY_3rd(%d) SWELLING_LOW_CURRENT(%d) SWELLING_LOW_CURRENT_WC(%d)\n",
			__func__, pdata->swelling_low_temp_block_3rd, pdata->swelling_low_temp_recov_3rd,
			pdata->swelling_low_temp_current_3rd, pdata->swelling_wc_low_temp_current_3rd);

	if(battery->swelling_low_temp_4th_ctrl)
		pr_info("%s : SWELLING_LOW_TEMP_4th(%d) SWELLING_LOW_TEMP_RECOVERY_4th(%d) SWELLING_LOW_CURRENT(%d)\n",
			__func__, pdata->swelling_low_temp_block_4th, pdata->swelling_low_temp_recov_4th,
			pdata->swelling_low_temp_current_4th);

	if(battery->swelling_low_temp_5th_ctrl)
		pr_info("%s : SWELLING_LOW_TEMP_5th(%d) SWELLING_LOW_TEMP_RECOVERY_5th(%d) SWELLING_LOW_CURRENT(%d)\n",
			__func__, pdata->swelling_low_temp_block_5th, pdata->swelling_low_temp_recov_5th,
			pdata->swelling_low_temp_current_5th);
#endif

#if defined(CONFIG_CALC_TIME_TO_FULL)
	ret = of_property_read_u32(np, "battery,ttf_hv_12v_charge_current",
					&pdata->ttf_hv_12v_charge_current);
	if (ret) {
		pdata->ttf_hv_12v_charge_current =
			pdata->charging_current[SEC_BATTERY_CABLE_12V_TA].fast_charging_current;
		pr_info("%s: ttf_hv_12v_charge_current is Empty, Defualt value %d \n",
			__func__, pdata->ttf_hv_12v_charge_current);
	}
	ret = of_property_read_u32(np, "battery,ttf_hv_charge_current",
					&pdata->ttf_hv_charge_current);
	if (ret) {
		pdata->ttf_hv_charge_current =
			pdata->charging_current[SEC_BATTERY_CABLE_9V_TA].fast_charging_current;
		pr_info("%s: ttf_hv_charge_current is Empty, Defualt value %d \n",
			__func__, pdata->ttf_hv_charge_current);
	}

	ret = of_property_read_u32(np, "battery,ttf_hv_wireless_charge_current",
					&pdata->ttf_hv_wireless_charge_current);
	if (ret) {
		pr_info("%s: ttf_hv_wireless_charge_current is Empty, Defualt value 0 \n", __func__);
		pdata->ttf_hv_wireless_charge_current =
			pdata->charging_current[SEC_BATTERY_CABLE_HV_WIRELESS].fast_charging_current - 300;
	}

	ret = of_property_read_u32(np, "battery,ttf_hv_12v_wireless_charge_current",
					&pdata->ttf_hv_12v_wireless_charge_current);
	if (ret) {
		pr_info("%s: ttf_hv_12v_wireless_charge_current is Empty, Defualt value 0 \n", __func__);
		pdata->ttf_hv_12v_wireless_charge_current =
			pdata->charging_current[SEC_BATTERY_CABLE_HV_WIRELESS_20].fast_charging_current - 300;
	}

	ret = of_property_read_u32(np, "battery,ttf_wireless_charge_current",
					&pdata->ttf_wireless_charge_current);
	if (ret) {
		pr_info("%s: ttf_wireless_charge_current is Empty, Defualt value 0 \n", __func__);
		pdata->ttf_wireless_charge_current =
			pdata->charging_current[SEC_BATTERY_CABLE_WIRELESS].input_current_limit;
	}

	ret = of_property_read_u32(np, "battery,ttf_dc25_charge_current",
					&pdata->ttf_dc25_charge_current);
	if (ret) {
		pr_info("%s: ttf_dc25_charge_current is Empty, Defualt value 0 \n", __func__);
		pdata->ttf_dc25_charge_current =
			pdata->charging_current[SEC_BATTERY_CABLE_9V_TA].fast_charging_current;
	}

	ret = of_property_read_u32(np, "battery,ttf_dc45_charge_current",
					&pdata->ttf_dc45_charge_current);
	if (ret) {
		pr_info("%s: ttf_dc45_charge_current is Empty, Defualt value 0 \n", __func__);
		pdata->ttf_dc45_charge_current = pdata->ttf_dc25_charge_current;
	}
#endif

#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	/* wpc_det */
	ret = pdata->wpc_det = of_get_named_gpio(np, "battery,wpc_det", 0);
	if (ret < 0) {
		pr_info("%s : can't get wpc_det\n", __func__);
	}

	/* mst_pwr_en */
	ret = pdata->mst_pwr_en = of_get_named_gpio(np, "battery,mst_pwr_en", 0);
	if (ret < 0) {
		pr_info("%s : can't get mst_pwr_en\n", __func__);
	}
#endif

	/* wpc_en */
	ret = pdata->wpc_en = of_get_named_gpio(np, "battery,wpc_en", 0);
	if (ret < 0) {
		pr_info("%s : can't get wpc_en\n", __func__);
		pdata->wpc_en = 0;
	}
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	p = of_get_property(np, "battery,age_data", &len);
	if (p) {
		battery->pdata->num_age_step = len / sizeof(sec_age_data_t);
		battery->pdata->age_data = kzalloc(len, GFP_KERNEL);
		ret = of_property_read_u32_array(np, "battery,age_data",
				 (u32 *)battery->pdata->age_data, len/sizeof(u32));
		if (ret) {
			pr_err("%s failed to read battery->pdata->age_data: %d\n",
					__func__, ret);
			kfree(battery->pdata->age_data);
			battery->pdata->age_data = NULL;
			battery->pdata->num_age_step = 0;
		}
		pr_err("%s num_age_step : %d\n", __func__, battery->pdata->num_age_step);
#if defined(CONFIG_STEP_CHARGING)		
		for (len = 0; len < battery->pdata->num_age_step; ++len) {
			pr_err("[%d/%d]cycle:%d, float:%d, full_v:%d, recharge_v:%d, soc:%d, step charging condition:%d\n",
				len, battery->pdata->num_age_step-1,
				battery->pdata->age_data[len].cycle,
				battery->pdata->age_data[len].float_voltage,
				battery->pdata->age_data[len].full_condition_vcell,
				battery->pdata->age_data[len].recharge_condition_vcell,
				battery->pdata->age_data[len].full_condition_soc,
				battery->pdata->age_data[len].step_charging_condition);
		}
#else
		for (len = 0; len < battery->pdata->num_age_step; ++len) {
			pr_err("[%d/%d]cycle:%d, float:%d, full_v:%d, recharge_v:%d, soc:%d\n",
				len, battery->pdata->num_age_step-1,
				battery->pdata->age_data[len].cycle,
				battery->pdata->age_data[len].float_voltage,
				battery->pdata->age_data[len].full_condition_vcell,
				battery->pdata->age_data[len].recharge_condition_vcell,
				battery->pdata->age_data[len].full_condition_soc);
		}
#endif
	} else {
		battery->pdata->num_age_step = 0;
		pr_err("%s there is not age_data\n", __func__);
	}
#endif

	p = of_get_property(np, "battery,health_condition", &len);
	if (p || (len / sizeof(battery_health_condition)) == BATTERY_HEALTH_MAX) {
		battery->pdata->health_condition = kzalloc(len, GFP_KERNEL);
		ret = of_property_read_u32_array(np, "battery,health_condition",
				 (u32 *)battery->pdata->health_condition, len/sizeof(u32));
		if (ret) {
			pr_err("%s failed to read battery->pdata->health_condition: %d\n",
					__func__, ret);
			kfree(battery->pdata->health_condition);
			battery->pdata->health_condition = NULL;
		} else {
			for (i = 0; i < BATTERY_HEALTH_MAX; i++) {
				pr_err("%s: [BATTERY_HEALTH] %d: Cycle(~ %d), ASoC(~ %d)\n",
					__func__, i,
					battery->pdata->health_condition[i].cycle,
					battery->pdata->health_condition[i].asoc);
			}
		}
	} else {
		battery->pdata->health_condition = NULL;
		pr_err("%s there is not health_condition, len(%d)\n", __func__, len);
	}

	ret = of_property_read_u32(np, "battery,siop_input_limit_current",
			&pdata->siop_input_limit_current);
	if (ret)
		pdata->siop_input_limit_current = SIOP_INPUT_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,siop_charging_limit_current",
			&pdata->siop_charging_limit_current);
	if (ret)
		pdata->siop_charging_limit_current = SIOP_CHARGING_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,siop_hv_12v_input_limit_current",
			&pdata->siop_hv_12v_input_limit_current);
	if (ret)
		pdata->siop_hv_12v_input_limit_current = SIOP_HV_12V_INPUT_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,siop_hv_12v_charging_limit_current",
			&pdata->siop_hv_12v_charging_limit_current);
	if (ret)
		pdata->siop_hv_12v_charging_limit_current = SIOP_HV_12V_CHARGING_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,siop_hv_input_limit_current",
			&pdata->siop_hv_input_limit_current);
	if (ret)
		pdata->siop_hv_input_limit_current = SIOP_HV_INPUT_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,siop_hv_input_limit_current_2nd",
			&pdata->siop_hv_input_limit_current_2nd);
	if (ret)
		pdata->siop_hv_input_limit_current_2nd = pdata->siop_hv_input_limit_current;

	ret = of_property_read_u32(np, "battery,siop_hv_charging_limit_current",
			&pdata->siop_hv_charging_limit_current);
	if (ret)
		pdata->siop_hv_charging_limit_current = SIOP_HV_CHARGING_LIMIT_CURRENT;

#if defined(CONFIG_DIRECT_CHARGING)
	ret = of_property_read_u32(np, "battery,siop_apdo_input_limit_current",
			&pdata->siop_apdo_input_limit_current);
	if (ret)
		pdata->siop_apdo_input_limit_current = SIOP_APDO_INPUT_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,siop_apdo_charging_limit_current",
			&pdata->siop_apdo_charging_limit_current);
	if (ret)
		pdata->siop_apdo_charging_limit_current = SIOP_APDO_CHARGING_LIMIT_CURRENT;
#endif

	ret = of_property_read_u32(np, "battery,siop_wireless_input_limit_current",
			&pdata->siop_wireless_input_limit_current);
	if (ret)
		pdata->siop_wireless_input_limit_current = SIOP_WIRELESS_INPUT_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,siop_wireless_charging_limit_current",
			&pdata->siop_wireless_charging_limit_current);
	if (ret)
		pdata->siop_wireless_charging_limit_current = SIOP_WIRELESS_CHARGING_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,siop_hv_wireless_input_limit_current",
			&pdata->siop_hv_wireless_input_limit_current);
	if (ret)
		pdata->siop_hv_wireless_input_limit_current = SIOP_HV_WIRELESS_INPUT_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,siop_hv_wireless_charging_limit_current",
			&pdata->siop_hv_wireless_charging_limit_current);
	if (ret)
		pdata->siop_hv_wireless_charging_limit_current = SIOP_HV_WIRELESS_CHARGING_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,wireless_otg_input_current",
			&pdata->wireless_otg_input_current);
	if (ret)
		pdata->wireless_otg_input_current = WIRELESS_OTG_INPUT_CURRENT;

	ret = of_property_read_u32(np, "battery,max_input_voltage",
			&pdata->max_input_voltage);
	if (ret)
		pdata->max_input_voltage = 12000;

	ret = of_property_read_u32(np, "battery,max_input_current",
			&pdata->max_input_current);
	if (ret)
		pdata->max_input_current = 3000;

	ret = of_property_read_u32(np, "battery,pd_charging_charge_power",
			&pdata->pd_charging_charge_power);
	if (ret) {
		pr_err("%s: pd_charging_charge_power is Empty\n", __func__);
		pdata->pd_charging_charge_power = 15000;
	}

	ret = of_property_read_u32(np, "battery,rp_current_rp1",
			&pdata->rp_current_rp1);
	if (ret) {
		pr_err("%s: rp_current_rp1 is Empty\n", __func__);
		pdata->rp_current_rp1 = 500;
	}

	ret = of_property_read_u32(np, "battery,rp_current_rp2",
			&pdata->rp_current_rp2);
	if (ret) {
		pr_err("%s: rp_current_rp2 is Empty\n", __func__);
		pdata->rp_current_rp2 = 1500;
	}

	ret = of_property_read_u32(np, "battery,rp_current_rp3",
			&pdata->rp_current_rp3);
	if (ret) {
		pr_err("%s: rp_current_rp3 is Empty\n", __func__);
		pdata->rp_current_rp3 = 3000;
	}

	ret = of_property_read_u32(np, "battery,rp_current_rdu_rp3",
			&pdata->rp_current_rdu_rp3);
	if (ret) {
		pr_err("%s: rp_current_rdu_rp3 is Empty\n", __func__);
		pdata->rp_current_rdu_rp3 = 2100;
	}

	ret = of_property_read_u32(np, "battery,rp_current_abnormal_rp3",
			&pdata->rp_current_abnormal_rp3);
	if (ret) {
		pr_err("%s: rp_current_abnormal_rp3 is Empty\n", __func__);
		pdata->rp_current_rdu_rp3 = 1800;
	}

	ret = of_property_read_u32(np, "battery,nv_charge_power",
			&pdata->nv_charge_power);
	if (ret) {
		pr_err("%s: nv_charge_power is Empty\n", __func__);
		pdata->nv_charge_power =
			(SEC_INPUT_VOLTAGE_5V/10) * pdata->default_input_current;
	}

	ret = of_property_read_u32(np, "battery,tx_minduty_default",
			&pdata->tx_minduty_default);
	if (ret) {
		pdata->tx_minduty_default = 20;
		pr_err("%s: tx minduty is Empty. set %d\n", __func__, pdata->tx_minduty_default);
	}

	ret = of_property_read_u32(np, "battery,tx_minduty_5V",
			&pdata->tx_minduty_5V);
	if (ret) {
		pdata->tx_minduty_5V = 50;
		pr_err("%s: tx minduty 5V is Empty. set %d\n", __func__, pdata->tx_minduty_5V);
	}

	ret = of_property_read_u32(np, "battery,tx_uno_iout",
			&pdata->tx_uno_iout);
	if (ret) {
		pdata->tx_uno_iout = 1500;
		pr_err("%s: tx uno iout is Empty. set %d\n", __func__, pdata->tx_uno_iout);
	}

	ret = of_property_read_u32(np, "battery,tx_mfc_iout_gear",
			&pdata->tx_mfc_iout_gear);
	if (ret) {
		pdata->tx_mfc_iout_gear = 1500;
		pr_err("%s: tx mfc iout gear is Empty. set %d\n", __func__, pdata->tx_mfc_iout_gear);
	}

	ret = of_property_read_u32(np, "battery,tx_mfc_iout_phone",
			&pdata->tx_mfc_iout_phone);
	if (ret) {
		pdata->tx_mfc_iout_phone = 1100;
		pr_err("%s: tx mfc iout phone is Empty. set %d\n", __func__, pdata->tx_mfc_iout_phone);
	}

	ret = of_property_read_u32(np, "battery,tx_mfc_iout_phone_5v",
			&pdata->tx_mfc_iout_phone_5v);
	if (ret) {
		pdata->tx_mfc_iout_phone_5v = 300;
		pr_err("%s: tx mfc iout phone 5v is Empty. set %d\n", __func__, pdata->tx_mfc_iout_phone_5v);
	}

	ret = of_property_read_u32(np, "battery,tx_mfc_iout_lcd_on",
			&pdata->tx_mfc_iout_lcd_on);
	if (ret) {
		pdata->tx_mfc_iout_lcd_on = 900;
		pr_err("%s: tx mfc iout lcd on is Empty. set %d\n", __func__, pdata->tx_mfc_iout_lcd_on);
	}

	pr_info("%s: vendor : %s, technology : %d, cable_check_type : %d\n"
		"cable_source_type : %d, event_waiting_time : %d\n"
		"polling_type : %d, initial_count : %d, check_count : %d\n"
		"check_adc_max : %d, check_adc_min : %d\n"
		"ovp_uvlo_check_type : %d, thermal_source : %d\n"
		"temp_check_type : %d, temp_check_count : %d, nv_charge_power : %d\n",
		__func__,
		pdata->vendor, pdata->technology,pdata->cable_check_type,
		pdata->cable_source_type, pdata->event_waiting_time,
		pdata->polling_type, pdata->monitor_initial_count,
		pdata->check_count, pdata->check_adc_max, pdata->check_adc_min,
		pdata->ovp_uvlo_check_type, pdata->thermal_source,
		pdata->temp_check_type, pdata->temp_check_count, pdata->nv_charge_power);

#if defined(CONFIG_STEP_CHARGING)
	sec_step_charging_init(battery, dev);
#endif
	ret = of_property_read_u32(np, "battery,max_charging_current",
			&pdata->max_charging_current);
	if (ret) {
		pr_err("%s: max_charging_current is Empty\n", __func__);
		pdata->max_charging_current = 3000;
	}

	ret = of_property_read_u32(np, "battery,max_charging_charge_power",
			&pdata->max_charging_charge_power);
	if (ret) {
		pr_err("%s: max_charging_charge_power is Empty\n", __func__);
		pdata->max_charging_charge_power = 25000;
	}

#if defined(CONFIG_BATTERY_CISD)
	p = of_get_property(np, "battery,ignore_cisd_index", &len);
	pdata->ignore_cisd_index = kzalloc(sizeof(*pdata->ignore_cisd_index) * 2, GFP_KERNEL);
	if (p) {
		len = len / sizeof(u32);
		ret = of_property_read_u32_array(np, "battery,ignore_cisd_index",
	                     pdata->ignore_cisd_index, len);
	} else {
		pr_info("%s : battery,ignore_cisd_index is Empty\n", __func__);
	}

	p = of_get_property(np, "battery,ignore_cisd_index_d", &len);
	pdata->ignore_cisd_index_d = kzalloc(sizeof(*pdata->ignore_cisd_index_d) * 2, GFP_KERNEL);
	if (p) {
		len = len / sizeof(u32);
		ret = of_property_read_u32_array(np, "battery,ignore_cisd_index_d",
	                     pdata->ignore_cisd_index_d, len);
	} else {
		pr_info("%s : battery,ignore_cisd_index_d is Empty\n", __func__);
	}
#endif

#if defined(CONFIG_DIRECT_CHARGING)
	sec_direct_chg_init(battery, dev);
#endif
	return 0;
}

void sec_bat_parse_mode_dt(struct sec_battery_info *battery)
{
	struct device_node *np;
	sec_battery_platform_data_t *pdata = battery->pdata;
	int ret = 0;
	u32 temp = 0;

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s np NULL\n", __func__);
		return;
	}

	if (battery->store_mode) {
		ret = of_property_read_u32(np, "battery,store_mode_afc_input_current",
			&pdata->store_mode_afc_input_current);
		if (ret) {
			pr_info("%s : store_mode_afc_input_current is Empty\n", __func__);
			pdata->store_mode_afc_input_current = 440;
		}

		ret = of_property_read_u32(np, "battery,store_mode_hv_wireless_input_current",
			&pdata->store_mode_hv_wireless_input_current);
		if (ret) {
			pr_info("%s : store_mode_hv_wireless_input_current is Empty\n", __func__);
			pdata->store_mode_hv_wireless_input_current = 400;
		}

		if (pdata->wpc_temp_check_type) {
			ret = of_property_read_u32(np, "battery,wpc_store_high_temp",
			   &temp);
			if (!ret)
				pdata->wpc_high_temp = temp;

			ret = of_property_read_u32(np, "battery,wpc_store_high_temp_recovery",
			   &temp);
			if (!ret)
				pdata->wpc_high_temp_recovery = temp;

			ret = of_property_read_u32(np, "battery,wpc_store_charging_limit_current",
			   &temp);
			if (!ret)
				pdata->wpc_input_limit_current = temp;

			ret = of_property_read_u32(np, "battery,wpc_store_lcd_on_high_temp",
			   &temp);
			if (!ret)
				pdata->wpc_lcd_on_high_temp = (int)temp;

			ret = of_property_read_u32(np, "battery,wpc_store_lcd_on_high_temp_rec",
			   &temp);
			if (!ret)
				pdata->wpc_lcd_on_high_temp_rec = (int)temp;

			ret = of_property_read_u32(np, "battery,wpc_store_lcd_on_charging_limit_current",
				&temp);
			if (!ret)
				pdata->wpc_lcd_on_input_limit_current = (int)temp;

			pr_info("%s: update store_mode - wpc high_temp(t:%d, r:%d), lcd_on_high_temp(t:%d, r:%d), curr(%d, %d)\n",
				__func__,
				pdata->wpc_high_temp, pdata->wpc_high_temp_recovery,
				pdata->wpc_lcd_on_high_temp, pdata->wpc_lcd_on_high_temp_rec,
				pdata->wpc_input_limit_current,
				pdata->wpc_lcd_on_input_limit_current);
		}

		ret = of_property_read_u32(np, "battery,siop_store_hv_wireless_input_limit_current",
			&temp);
		if (!ret)
			pdata->siop_hv_wireless_input_limit_current = temp;
		else
			pdata->siop_hv_wireless_input_limit_current = SIOP_STORE_HV_WIRELESS_CHARGING_LIMIT_CURRENT;
		pr_info("%s: update siop_hv_wireless_input_limit_current(%d)\n",
			__func__, pdata->siop_hv_wireless_input_limit_current);
	}
}

void sec_bat_parse_mode_dt_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
		struct sec_battery_info, parse_mode_dt_work.work);

	sec_bat_parse_mode_dt(battery);

	if (is_hv_wire_type(battery->cable_type) ||
		is_hv_wireless_type(battery->cable_type)) {
		sec_bat_set_charging_current(battery);
	}

	wake_unlock(&battery->parse_mode_dt_wake_lock);
}
#endif
