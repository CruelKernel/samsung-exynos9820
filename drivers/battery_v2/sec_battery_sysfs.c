/*
 *  sec_battery_sysfs.c
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
#include "include/sec_battery_sysfs.h"
#include "include/sec_charging_common.h"

#include <linux/sec_ext.h>
#include <linux/sec_debug.h>

#if defined(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif

static struct device_attribute sec_battery_attrs[] = {
	SEC_BATTERY_ATTR(batt_reset_soc),
	SEC_BATTERY_ATTR(batt_read_raw_soc),
	SEC_BATTERY_ATTR(batt_read_adj_soc),
	SEC_BATTERY_ATTR(batt_type),
	SEC_BATTERY_ATTR(batt_vfocv),
	SEC_BATTERY_ATTR(batt_vol_adc),
	SEC_BATTERY_ATTR(batt_vol_adc_cal),
	SEC_BATTERY_ATTR(batt_vol_aver),
	SEC_BATTERY_ATTR(batt_vol_adc_aver),
	SEC_BATTERY_ATTR(batt_voltage_now),
	SEC_BATTERY_ATTR(batt_current_ua_now),
	SEC_BATTERY_ATTR(batt_current_ua_avg),
	SEC_BATTERY_ATTR(batt_filter_cfg),
	SEC_BATTERY_ATTR(batt_temp),
	SEC_BATTERY_ATTR(batt_temp_adc),
	SEC_BATTERY_ATTR(batt_temp_aver),
	SEC_BATTERY_ATTR(batt_temp_adc_aver),
	SEC_BATTERY_ATTR(usb_temp),
	SEC_BATTERY_ATTR(usb_temp_adc),
	SEC_BATTERY_ATTR(chg_temp),
	SEC_BATTERY_ATTR(chg_temp_adc),
	SEC_BATTERY_ATTR(slave_chg_temp),
	SEC_BATTERY_ATTR(slave_chg_temp_adc),
#if defined(CONFIG_DIRECT_CHARGING)
	SEC_BATTERY_ATTR(dchg_adc_mode_ctrl),
	SEC_BATTERY_ATTR(dchg_temp),
	SEC_BATTERY_ATTR(dchg_temp_adc),
#endif
	SEC_BATTERY_ATTR(batt_vf_adc),
	SEC_BATTERY_ATTR(batt_slate_mode),

	SEC_BATTERY_ATTR(batt_lp_charging),
	SEC_BATTERY_ATTR(siop_activated),
	SEC_BATTERY_ATTR(siop_level),
	SEC_BATTERY_ATTR(siop_event),
	SEC_BATTERY_ATTR(batt_charging_source),
	SEC_BATTERY_ATTR(fg_reg_dump),
	SEC_BATTERY_ATTR(fg_reset_cap),
	SEC_BATTERY_ATTR(fg_capacity),
	SEC_BATTERY_ATTR(fg_asoc),
	SEC_BATTERY_ATTR(auth),
	SEC_BATTERY_ATTR(chg_current_adc),
	SEC_BATTERY_ATTR(wc_adc),
	SEC_BATTERY_ATTR(wc_status),
	SEC_BATTERY_ATTR(wc_enable),
	SEC_BATTERY_ATTR(wc_control),
	SEC_BATTERY_ATTR(wc_control_cnt),
	SEC_BATTERY_ATTR(led_cover),
	SEC_BATTERY_ATTR(hv_charger_status),
	SEC_BATTERY_ATTR(hv_wc_charger_status),
	SEC_BATTERY_ATTR(hv_charger_set),
	SEC_BATTERY_ATTR(factory_mode),
	SEC_BATTERY_ATTR(store_mode),
	SEC_BATTERY_ATTR(update),
	SEC_BATTERY_ATTR(test_mode),

	SEC_BATTERY_ATTR(call),
	SEC_BATTERY_ATTR(2g_call),
	SEC_BATTERY_ATTR(talk_gsm),
	SEC_BATTERY_ATTR(3g_call),
	SEC_BATTERY_ATTR(talk_wcdma),
	SEC_BATTERY_ATTR(music),
	SEC_BATTERY_ATTR(video),
	SEC_BATTERY_ATTR(browser),
	SEC_BATTERY_ATTR(hotspot),
	SEC_BATTERY_ATTR(camera),
	SEC_BATTERY_ATTR(camcorder),
	SEC_BATTERY_ATTR(data_call),
	SEC_BATTERY_ATTR(wifi),
	SEC_BATTERY_ATTR(wibro),
	SEC_BATTERY_ATTR(lte),
	SEC_BATTERY_ATTR(lcd),
	SEC_BATTERY_ATTR(gps),
	SEC_BATTERY_ATTR(event),
	SEC_BATTERY_ATTR(batt_temp_table),
	SEC_BATTERY_ATTR(batt_high_current_usb),
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	SEC_BATTERY_ATTR(test_charge_current),
#if defined(CONFIG_STEP_CHARGING)
	SEC_BATTERY_ATTR(test_step_condition),
#endif
#endif
	SEC_BATTERY_ATTR(set_stability_test),
	SEC_BATTERY_ATTR(batt_capacity_max),
	SEC_BATTERY_ATTR(batt_inbat_voltage),
	SEC_BATTERY_ATTR(batt_inbat_voltage_ocv),
	SEC_BATTERY_ATTR(check_slave_chg),
	SEC_BATTERY_ATTR(batt_inbat_wireless_cs100),
	SEC_BATTERY_ATTR(hmt_ta_connected),
	SEC_BATTERY_ATTR(hmt_ta_charge),
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	SEC_BATTERY_ATTR(fg_cycle),
	SEC_BATTERY_ATTR(fg_full_voltage),
	SEC_BATTERY_ATTR(fg_fullcapnom),
	SEC_BATTERY_ATTR(battery_cycle),
	SEC_BATTERY_ATTR(battery_cycle_test),
#endif
	SEC_BATTERY_ATTR(batt_wpc_temp),
	SEC_BATTERY_ATTR(batt_wpc_temp_adc),
	SEC_BATTERY_ATTR(mst_switch_test), /* MFC MST switch test */
#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	SEC_BATTERY_ATTR(batt_wireless_firmware_update),
	SEC_BATTERY_ATTR(otp_firmware_result),
	SEC_BATTERY_ATTR(wc_ic_grade),
	SEC_BATTERY_ATTR(wc_ic_chip_id),
	SEC_BATTERY_ATTR(otp_firmware_ver_bin),
	SEC_BATTERY_ATTR(otp_firmware_ver),
	SEC_BATTERY_ATTR(tx_firmware_result),
	SEC_BATTERY_ATTR(tx_firmware_ver),
	SEC_BATTERY_ATTR(batt_tx_status),
#endif
	SEC_BATTERY_ATTR(wc_vout),
	SEC_BATTERY_ATTR(wc_vrect),
	SEC_BATTERY_ATTR(wc_tx_en),
	SEC_BATTERY_ATTR(wc_tx_vout),
	SEC_BATTERY_ATTR(batt_hv_wireless_status),
	SEC_BATTERY_ATTR(batt_hv_wireless_pad_ctrl),
	SEC_BATTERY_ATTR(wc_tx_id),
	SEC_BATTERY_ATTR(wc_op_freq),
	SEC_BATTERY_ATTR(wc_cmd_info),
	SEC_BATTERY_ATTR(wc_rx_connected),
	SEC_BATTERY_ATTR(wc_rx_connected_dev),
	SEC_BATTERY_ATTR(wc_tx_mfc_vin_from_uno),
	SEC_BATTERY_ATTR(wc_tx_mfc_iin_from_uno),
#if defined(CONFIG_WIRELESS_TX_MODE)
	SEC_BATTERY_ATTR(wc_tx_avg_curr),
	SEC_BATTERY_ATTR(wc_tx_total_pwr),
#endif
	SEC_BATTERY_ATTR(wc_tx_stop_capacity),
	SEC_BATTERY_ATTR(wc_tx_timer_en),
#if defined(CONFIG_ENG_BATTERY_CONCEPT)	
	SEC_BATTERY_ATTR(batt_tune_float_voltage),
	SEC_BATTERY_ATTR(batt_tune_input_charge_current),
	SEC_BATTERY_ATTR(batt_tune_fast_charge_current),
	SEC_BATTERY_ATTR(batt_tune_ui_term_cur_1st),
	SEC_BATTERY_ATTR(batt_tune_ui_term_cur_2nd),
	SEC_BATTERY_ATTR(batt_tune_temp_high_normal),
	SEC_BATTERY_ATTR(batt_tune_temp_high_rec_normal),
	SEC_BATTERY_ATTR(batt_tune_temp_low_normal),
	SEC_BATTERY_ATTR(batt_tune_temp_low_rec_normal),
	SEC_BATTERY_ATTR(batt_tune_chg_temp_high),
	SEC_BATTERY_ATTR(batt_tune_chg_temp_rec),
	SEC_BATTERY_ATTR(batt_tune_chg_limit_cur),
	SEC_BATTERY_ATTR(batt_tune_coil_temp_high),
	SEC_BATTERY_ATTR(batt_tune_coil_temp_rec),
	SEC_BATTERY_ATTR(batt_tune_coil_limit_cur),
	SEC_BATTERY_ATTR(batt_tune_wpc_temp_high),
	SEC_BATTERY_ATTR(batt_tune_wpc_temp_high_rec),
	SEC_BATTERY_ATTR(batt_tune_dchg_temp_high),
	SEC_BATTERY_ATTR(batt_tune_dchg_temp_high_rec),
	SEC_BATTERY_ATTR(batt_tune_dchg_batt_temp_high),
	SEC_BATTERY_ATTR(batt_tune_dchg_batt_temp_high_rec),
	SEC_BATTERY_ATTR(batt_tune_dchg_limit_input_cur),
	SEC_BATTERY_ATTR(batt_tune_dchg_limit_chg_cur),
#endif	
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	SEC_BATTERY_ATTR(batt_update_data),
#endif
	SEC_BATTERY_ATTR(batt_misc_event),
	SEC_BATTERY_ATTR(batt_tx_event),
	SEC_BATTERY_ATTR(batt_ext_dev_chg),
	SEC_BATTERY_ATTR(batt_wdt_control),
	SEC_BATTERY_ATTR(mode),
	SEC_BATTERY_ATTR(check_ps_ready),
	SEC_BATTERY_ATTR(batt_chip_id),
	SEC_BATTERY_ATTR(error_cause),
	SEC_BATTERY_ATTR(cisd_fullcaprep_max),
#if defined(CONFIG_BATTERY_CISD)
	SEC_BATTERY_ATTR(cisd_data),
	SEC_BATTERY_ATTR(cisd_data_json),
	SEC_BATTERY_ATTR(cisd_data_d_json),
	SEC_BATTERY_ATTR(cisd_wire_count),
	SEC_BATTERY_ATTR(cisd_wc_data),
	SEC_BATTERY_ATTR(cisd_wc_data_json),
	SEC_BATTERY_ATTR(cisd_power_data),
	SEC_BATTERY_ATTR(cisd_power_data_json),
	SEC_BATTERY_ATTR(cisd_cable_data),
	SEC_BATTERY_ATTR(cisd_cable_data_json),
	SEC_BATTERY_ATTR(cisd_tx_data),
	SEC_BATTERY_ATTR(cisd_tx_data_json),
	SEC_BATTERY_ATTR(cisd_event_data),
	SEC_BATTERY_ATTR(cisd_event_data_json),
	SEC_BATTERY_ATTR(prev_battery_data),
	SEC_BATTERY_ATTR(prev_battery_info),
#endif
	SEC_BATTERY_ATTR(safety_timer_set),
	SEC_BATTERY_ATTR(batt_swelling_control),
	SEC_BATTERY_ATTR(batt_temp_control_test),
	SEC_BATTERY_ATTR(safety_timer_info),
	SEC_BATTERY_ATTR(batt_shipmode_test),
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	SEC_BATTERY_ATTR(batt_temp_test),
#endif
	SEC_BATTERY_ATTR(batt_current_event),
	SEC_BATTERY_ATTR(batt_jig_gpio),
	SEC_BATTERY_ATTR(cc_info),
#if defined(CONFIG_WIRELESS_AUTH)
	SEC_BATTERY_ATTR(wc_auth_adt_sent),
#endif
	SEC_BATTERY_ATTR(wc_duo_rx_power),
#if defined(CONFIG_BATTERY_SAMSUNG_MHS)
	SEC_BATTERY_ATTR(batt_charging_port),
#endif
	SEC_BATTERY_ATTR(ext_event),
	SEC_BATTERY_ATTR(direct_charging_status),
#if defined(CONFIG_DIRECT_CHARGING)
	SEC_BATTERY_ATTR(direct_charging_step),
	SEC_BATTERY_ATTR(direct_charging_iin),
	SEC_BATTERY_ATTR(switch_charging_source),
#endif
	SEC_BATTERY_ATTR(charging_type),
#if defined(CONFIG_SEC_FACTORY)
	SEC_BATTERY_ATTR(batt_factory_mode),
#endif
	SEC_BATTERY_ATTR(boot_completed),
	SEC_BATTERY_ATTR(batt_full_capacity),
};

void update_external_temp_table(struct sec_battery_info *battery, int temp[])
{
	battery->pdata->temp_high_threshold_normal = temp[0];
	battery->pdata->temp_high_recovery_normal = temp[1];
	battery->pdata->temp_low_threshold_normal = temp[2];
	battery->pdata->temp_low_recovery_normal = temp[3];
	battery->pdata->temp_high_threshold_lpm = temp[4];
	battery->pdata->temp_high_recovery_lpm = temp[5];
	battery->pdata->temp_low_threshold_lpm = temp[6];
	battery->pdata->temp_low_recovery_lpm = temp[7];

}

ssize_t sec_bat_show_attrs(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - sec_battery_attrs;
	union power_supply_propval value = {0, };
	int i = 0;
	int ret = 0;

	switch (offset) {
	case BATT_RESET_SOC:
		break;
	case BATT_READ_RAW_SOC:
		{
			value.intval =
				SEC_FUELGAUGE_CAPACITY_TYPE_RAW;
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_CAPACITY, value);

			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_READ_ADJ_SOC:
		break;
	case BATT_TYPE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n",
			battery->batt_type);
		break;
	case BATT_VFOCV:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->voltage_ocv);
		break;
	case BATT_VOL_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->inbat_adc);
		break;
	case BATT_VOL_ADC_CAL:
		break;
	case BATT_VOL_AVER:
		break;
	case BATT_VOL_ADC_AVER:
		break;
	case BATT_VOLTAGE_NOW:
		{
			value.intval = 0;
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			        value.intval * 1000);
		}
		break;
	case BATT_CURRENT_UA_NOW:
		{
			value.intval = SEC_BATTERY_CURRENT_UA;
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_CURRENT_NOW, value);
#if defined(CONFIG_SEC_FACTORY)
			pr_err("%s: batt_current_ua_now (%d)\n",
					__func__, value.intval);
#endif
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_CURRENT_UA_AVG:
		{
			value.intval = SEC_BATTERY_CURRENT_UA;
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_CURRENT_AVG, value);

			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_FILTER_CFG:
		{
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_FILTER_CFG, value);

			i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n",
				value.intval);
		}
		break;
	case BATT_TEMP:
		switch (battery->pdata->thermal_source) {
		case SEC_BATTERY_THERMAL_SOURCE_FG:
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_TEMP, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
			break;
		case SEC_BATTERY_THERMAL_SOURCE_CALLBACK:
			if (battery->pdata->get_temperature_callback) {
			battery->pdata->get_temperature_callback(
				POWER_SUPPLY_PROP_TEMP, &value);
			}
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
			break;
		case SEC_BATTERY_THERMAL_SOURCE_ADC:
			if (sec_bat_get_value_by_adc(battery,
					SEC_BAT_ADC_CHANNEL_TEMP, &value, battery->pdata->temp_check_type)) {
				i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
					value.intval);
			} else {
				i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				       0);
			}
			break;
		default:
			break;
		}
		break;
	case BATT_TEMP_ADC:
		/*
			If F/G is used for reading the temperature and
			compensation table is used,
			the raw value that isn't compensated can be read by
			POWER_SUPPLY_PROP_TEMP_AMBIENT
		 */
		switch (battery->pdata->thermal_source) {
		case SEC_BATTERY_THERMAL_SOURCE_FG:
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_TEMP_AMBIENT, value);
			battery->temp_adc = value.intval;
			break;
		default:
			break;
		}
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->temp_adc);
		break;
	case BATT_TEMP_AVER:
		break;
	case BATT_TEMP_ADC_AVER:
		break;
	case USB_TEMP:
		if (sec_bat_get_value_by_adc(battery,
				SEC_BAT_ADC_CHANNEL_USB_TEMP, &value, battery->pdata->usb_temp_check_type)) {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				       value.intval);
		} else {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				       0);
		}
		break;
	case USB_TEMP_ADC:
		if (battery->pdata->usb_thermal_source) {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				       battery->usb_temp_adc);
		} else {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				       0);
		}
		break;
	case CHG_TEMP:
		if (sec_bat_get_value_by_adc(battery,
			SEC_BAT_ADC_CHANNEL_CHG_TEMP, &value, battery->pdata->chg_temp_check_type)) {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				       value.intval);
		} else {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				       0);
		}
		break;
	case CHG_TEMP_ADC:
		if (battery->pdata->chg_thermal_source) {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				       battery->chg_temp_adc);
		} else {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				       0);
		}
		break;
	case SLAVE_CHG_TEMP:
		if (sec_bat_get_value_by_adc(battery,
			SEC_BAT_ADC_CHANNEL_SLAVE_CHG_TEMP, &value , battery->pdata->slave_chg_temp_check_type)) {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
					   value.intval);
		} else {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
					   0);
		}
		break;
	case SLAVE_CHG_TEMP_ADC:
		if (battery->pdata->slave_thermal_source) {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
					   battery->slave_chg_temp_adc);
		} else {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
					   0);
		}
		break;
#if defined(CONFIG_DIRECT_CHARGING)
	case DCHG_ADC_MODE_CTRL:
		break;
	case DCHG_TEMP:
		{
			psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_PROP_TEMP, value);
			battery->dchg_temp = sec_bat_get_direct_chg_temp_adc(battery,
						value.intval, battery->pdata->adc_check_count);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery->dchg_temp);
		}
		break;
	case DCHG_TEMP_ADC:
		{
			psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_PROP_TEMP, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
#endif
	case BATT_VF_ADC:
		break;
	case BATT_SLATE_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			is_slate_mode(battery));
		break;

	case BATT_LP_CHARGING:
		if (lpcharge) {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				       lpcharge ? 1 : 0);
		}
		break;
	case SIOP_ACTIVATED:
		break;
	case SIOP_LEVEL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->siop_level);
		break;
	case SIOP_EVENT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			0);
		break;
	case BATT_CHARGING_SOURCE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->cable_type);
		break;
	case FG_REG_DUMP:
		break;
	case FG_RESET_CAP:
		break;
	case FG_CAPACITY:
	{
		value.intval =
			SEC_BATTERY_CAPACITY_DESIGNED;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);

		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%04x ",
			value.intval);

		value.intval =
			SEC_BATTERY_CAPACITY_ABSOLUTE;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);

		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%04x ",
			value.intval);

		value.intval =
			SEC_BATTERY_CAPACITY_TEMPERARY;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);

		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%04x ",
			value.intval);

		value.intval =
			SEC_BATTERY_CAPACITY_CURRENT;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);

		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%04x\n",
			value.intval);
	}
		break;
	case FG_ASOC:
		value.intval = -1;
		{
			struct power_supply *psy_fg = NULL;
			psy_fg = get_power_supply_by_name(battery->pdata->fuelgauge_name);
			if (!psy_fg) {
				pr_err("%s: Fail to get psy (%s)\n",
						__func__, battery->pdata->fuelgauge_name);
			} else {
				if (psy_fg->desc->get_property != NULL) {
					ret = psy_fg->desc->get_property(psy_fg,
							POWER_SUPPLY_PROP_ENERGY_FULL, &value);
					if (ret < 0) {
						pr_err("%s: Fail to %s get (%d=>%d)\n",
								__func__, battery->pdata->fuelgauge_name,
								POWER_SUPPLY_PROP_ENERGY_FULL, ret);
					}
				}
			}
		}
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       value.intval);
		break;
	case AUTH:
		break;
	case CHG_CURRENT_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->current_adc);
		break;
	case WC_ADC:
		break;
	case WC_STATUS:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			is_wireless_type(battery->cable_type) ? 1: 0);
		break;
	case WC_ENABLE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->wc_enable);
		break;
	case WC_CONTROL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->wc_enable);
		break;
	case WC_CONTROL_CNT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->wc_enable_cnt_value);
		break;
	case LED_COVER:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->led_cover);
		break;
	case HV_CHARGER_STATUS:
		{
			int check_val = 0;
			if(is_wireless_type(battery->cable_type)) {
				check_val = 0;
			} else {
				if (is_pd_wire_type(battery->cable_type) &&
				battery->pd_max_charge_power >= HV_CHARGER_STATUS_STANDARD4)
					check_val = SFC_45W;
				else if (is_pd_wire_type(battery->cable_type) &&
					battery->pd_max_charge_power >= HV_CHARGER_STATUS_STANDARD3)
					check_val = SFC_25W;
				else if (is_hv_wire_12v_type(battery->cable_type) ||
					battery->max_charge_power >= HV_CHARGER_STATUS_STANDARD2) /* 20000mW */
					check_val = AFC_12V_OR_20W;
				else if (is_hv_wire_type(battery->cable_type) ||
					(is_pd_wire_type(battery->cable_type) &&
					battery->pd_max_charge_power >= HV_CHARGER_STATUS_STANDARD1 &&
					battery->hv_pdo) ||
					battery->wire_status == SEC_BATTERY_CABLE_PREPARE_TA ||
					battery->max_charge_power >= HV_CHARGER_STATUS_STANDARD1) /* 12000mW */
					check_val = AFC_9V_OR_15W;
			}
                        pr_info("%s : HV_CHARGER_STATUS(%d) pd max charge power(%d)\n", __func__, check_val, battery->pd_max_charge_power);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", check_val);
		}
		break;
	case HV_WC_CHARGER_STATUS:
		{
			int check_val = 0;
#if !defined(CONFIG_BATTERY_SAMSUNG_MHS)
			if(is_nv_wireless_type(battery->cable_type))
				check_val = 0;
			else {
				if (battery->cable_type == SEC_BATTERY_CABLE_HV_WIRELESS_20)
					check_val = sec_bat_get_wireless20_power_class(battery);
				else
					check_val = 1;
			}
#endif
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", check_val);
		}
		break;
	case HV_CHARGER_SET:
		break;
	case FACTORY_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->factory_mode);
		break;
	case STORE_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->store_mode);
		break;
	case UPDATE:
		break;
	case TEST_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->test_mode);
		break;

	case BATT_EVENT_CALL:
	case BATT_EVENT_2G_CALL:
	case BATT_EVENT_TALK_GSM:
		break;
	case BATT_EVENT_3G_CALL:
	case BATT_EVENT_TALK_WCDMA:
		break;
	case BATT_EVENT_MUSIC:
		break;
	case BATT_EVENT_VIDEO:
		break;
	case BATT_EVENT_BROWSER:
		break;
	case BATT_EVENT_HOTSPOT:
		break;
	case BATT_EVENT_CAMERA:
		break;
	case BATT_EVENT_CAMCORDER:
		break;
	case BATT_EVENT_DATA_CALL:
		break;
	case BATT_EVENT_WIFI:
		break;
	case BATT_EVENT_WIBRO:
		break;
	case BATT_EVENT_LTE:
		break;
	case BATT_EVENT_LCD:
		break;
	case BATT_EVENT_GPS:
		break;
	case BATT_EVENT:
		break;
	case BATT_TEMP_TABLE:
		i += scnprintf(buf + i, PAGE_SIZE - i,
			"%d %d %d %d %d %d %d %d\n",
			battery->pdata->temp_high_threshold_normal,
			battery->pdata->temp_high_recovery_normal,
			battery->pdata->temp_low_threshold_normal,
			battery->pdata->temp_low_recovery_normal,
			battery->pdata->temp_high_threshold_lpm,
			battery->pdata->temp_high_recovery_lpm,
			battery->pdata->temp_low_threshold_lpm,
			battery->pdata->temp_low_recovery_lpm);
		break;
	case BATT_HIGH_CURRENT_USB:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->is_hc_usb);
		break;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	case TEST_CHARGE_CURRENT:
		{
			psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_PROP_CURRENT_NOW, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
					value.intval);
		}
		break;
#if defined(CONFIG_STEP_CHARGING)		
	case TEST_STEP_CONDITION:
		{
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
					battery->test_step_condition);
		}
		break;
#endif
#endif
	case SET_STABILITY_TEST:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->stability_test);
		break;
	case BATT_CAPACITY_MAX:
		psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case BATT_INBAT_VOLTAGE:
	case BATT_INBAT_VOLTAGE_OCV:
		if(battery->pdata->support_fgsrc_change == true) {
			int j, k, ocv, ocv_data[10];
			value.intval = 0;
			psy_do_property(battery->pdata->fgsrc_switch_name, set,
					POWER_SUPPLY_PROP_ENERGY_NOW, value);
			for (j = 0; j < 10; j++) {
				msleep(175);
				psy_do_property(battery->pdata->fuelgauge_name, get,
						POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
				ocv_data[j] = value.intval;
			}
			value.intval = 1;
			psy_do_property(battery->pdata->fgsrc_switch_name, set,
					POWER_SUPPLY_PROP_ENERGY_NOW, value);
			for (j = 1; j < 10; j++) {
				ocv = ocv_data[j];
				k = j;
				while (k > 0 && ocv_data[k-1] > ocv) {
					ocv_data[k] = ocv_data[k-1];
					k--;
				}
				ocv_data[k] = ocv;
			}
			ocv = 0;
			for (j = 2; j < 8; j++) {
				ocv += ocv_data[j];
			}
			ret = ocv / 6;
		} else {
			ret = sec_bat_get_adc_data(battery,
			SEC_BAT_ADC_CHANNEL_INBAT_VOLTAGE,
			battery->pdata->adc_check_count);
		}
		dev_info(battery->dev, "in-battery voltage ocv(%d)\n", ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case CHECK_SLAVE_CHG:
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_CHECK_SLAVE_I2C, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		pr_info("%s : CHECK_SLAVE_CHG=%d\n",__func__,value.intval);
		break;
	case BATT_INBAT_WIRELESS_CS100:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_STATUS, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case HMT_TA_CONNECTED:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(battery->cable_type == SEC_BATTERY_CABLE_HMT_CONNECTED) ? 1 : 0);
		break;
	case HMT_TA_CHARGE:
#if defined(CONFIG_CCIC_NOTIFIER)
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(battery->current_event & SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE) ? 0 : 1);
#else
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(battery->cable_type == SEC_BATTERY_CABLE_HMT_CHARGE) ? 1 : 0);
#endif
		break;
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	case FG_CYCLE:
		value.intval = SEC_BATTERY_CAPACITY_CYCLE;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);
		value.intval = value.intval / 100;
		dev_info(battery->dev, "fg cycle(%d)\n", value.intval);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case FG_FULL_VOLTAGE:
          {
            int recharging_voltage = battery->pdata->recharge_condition_vcell;

            if (battery->current_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING) {
              recharging_voltage = battery->pdata->swelling_high_rechg_voltage;
            } else if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE) {
              /* float voltage - 150mV */
              recharging_voltage = (battery->pdata->chg_float_voltage / battery->pdata->chg_float_voltage_conv) \
                                   - 150;
            }

		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_PROP_VOLTAGE_MAX, value);		
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d %d\n",
			value.intval, recharging_voltage);
		break;
          }
	case FG_FULLCAPNOM:
		value.intval =
			SEC_BATTERY_CAPACITY_AGEDCELL;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case BATTERY_CYCLE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->batt_cycle);
		break;
	case BATTERY_CYCLE_TEST:
		break;
#endif
	case BATT_WPC_TEMP:
		if (sec_bat_get_value_by_adc(battery,
			SEC_BAT_ADC_CHANNEL_WPC_TEMP, &value, battery->pdata->wpc_temp_check_type)) {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		} else {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				0);
		}
		break;
	case BATT_WPC_TEMP_ADC:
		if (battery->pdata->wpc_thermal_source) {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery->wpc_temp_adc);
		} else {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				0);
		}
		break;
	case BATT_WIRELESS_MST_SWITCH_TEST:
		value.intval = SEC_WIRELESS_MST_SWITCH_VERIFY;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		pr_info("%s MST switch verify. result: %x\n", __func__, value.intval);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n", value.intval);
		break;
#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	case BATT_WIRELESS_FIRMWARE_UPDATE:
		value.intval = SEC_WIRELESS_OTP_FIRM_VERIFY;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		pr_info("%s RX firmware verify. result: %d\n", __func__, value.intval);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case OTP_FIRMWARE_RESULT:
		value.intval = SEC_WIRELESS_OTP_FIRM_RESULT;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case WC_IC_GRADE:
		value.intval = SEC_WIRELESS_IC_GRADE;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%x ", value.intval);

		value.intval = SEC_WIRELESS_IC_REVISION;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%x\n", value.intval);
		break;
	case WC_IC_CHIP_ID:
		value.intval = SEC_WIRELESS_IC_CHIP_ID;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n", value.intval);
		break;
	case OTP_FIRMWARE_VER_BIN:
		value.intval = SEC_WIRELESS_OTP_FIRM_VER_BIN;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n", value.intval);
		break;
	case OTP_FIRMWARE_VER:
		value.intval = SEC_WIRELESS_OTP_FIRM_VER;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);

		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n", value.intval);
		break;
	case TX_FIRMWARE_RESULT:
		value.intval = SEC_WIRELESS_TX_FIRM_RESULT;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case TX_FIRMWARE_VER:
		value.intval = SEC_WIRELESS_TX_FIRM_VER;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);

		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n", value.intval);
		break;
	case BATT_TX_STATUS:
		value.intval = SEC_TX_FIRMWARE;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);

		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n", value.intval);
		break;
#endif
	case WC_VOUT:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case WC_VRECT:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_ENERGY_AVG, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;

	case WC_TX_EN:
		pr_info("%s wc tx eanble(%d)",__func__, battery->wc_tx_enable);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery->wc_tx_enable);
		break;
	case WC_TX_VOUT:
		pr_info("%s wc tx vout(%d)",__func__, battery->wc_tx_vout);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery->wc_tx_vout);
		break;

	case BATT_HV_WIRELESS_STATUS:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case BATT_HV_WIRELESS_PAD_CTRL:
		break;
	case WC_TX_ID:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ID, value);

		pr_info("%s TX ID (%d)",__func__, value.intval);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case WC_OP_FREQ:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_OP_FREQ, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case WC_CMD_INFO:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TRX_CMD, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%02x ",
			value.intval);

		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TRX_VAL, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%02x ",
			value.intval);
		break;
	case WC_RX_CONNECTED:
		pr_info("%s RX Connected (%d)",__func__, battery->wc_rx_connected);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->wc_rx_connected);
		break;
	case WC_RX_CONNECTED_DEV:
		pr_info("%s RX Type (%d)",__func__, battery->wc_rx_type);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->wc_rx_type);
		break;
	case WC_TX_MFC_VIN_FROM_UNO:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_VIN, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			value.intval);
		break;
	case WC_TX_MFC_IIN_FROM_UNO:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_IIN, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			value.intval);
		break;
#if defined(CONFIG_WIRELESS_TX_MODE)
	case WC_TX_AVG_CURR:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
		battery->tx_avg_curr);
		/* If PMS read this value, average Tx current will be reset */
		//battery->tx_clear = true;
	break;
	case WC_TX_TOTAL_PWR:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
		battery->tx_total_power);
		/* If PMS read this value, average Tx current will be reset */
		battery->tx_clear = true;
	break;
#endif
	case WC_TX_STOP_CAPACITY:
		ret = battery->pdata->tx_stop_capacity;
		pr_info("%s tx stop capacity = %d%%", __func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			ret);
		break;
	case WC_TX_TIMER_EN:
		break;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	case BATT_TUNE_FLOAT_VOLTAGE:
		ret = battery->pdata->chg_float_voltage;
		pr_info("%s float voltage = %d mA",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_INPUT_CHARGE_CURRENT:
		ret = battery->pdata->charging_current[i].input_current_limit;
		pr_info("%s input charge current = %d mA",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_FAST_CHARGE_CURRENT:
		ret = battery->pdata->charging_current[i].fast_charging_current;
		pr_info("%s fast charge current = %d mA",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_UI_TERM_CURRENT_1ST:
		ret = battery->pdata->full_check_current_1st;
		pr_info("%s ui term current = %d mA",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_UI_TERM_CURRENT_2ND:
		ret = battery->pdata->full_check_current_2nd;
		pr_info("%s ui term current = %d mA",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_TEMP_HIGH_NORMAL:
		ret = battery->pdata->temp_high_threshold_normal;
		pr_info("%s temp high normal block	= %d ",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_TEMP_HIGH_REC_NORMAL:
		ret = battery->pdata->temp_high_recovery_normal;
		pr_info("%s temp high normal recover  = %d ",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_TEMP_LOW_NORMAL:
		ret = battery->pdata->temp_low_threshold_normal;
		pr_info("%s temp low normal block  = %d ",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_TEMP_LOW_REC_NORMAL:
		ret = battery->pdata->temp_low_recovery_normal;
		pr_info("%s temp low normal recover  = %d ",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_CHG_TEMP_HIGH:
		ret = battery->pdata->chg_high_temp;
		pr_info("%s chg_high_temp = %d ",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_CHG_TEMP_REC:
		ret = battery->pdata->chg_high_temp_recovery;
		pr_info("%s chg_high_temp_recovery	= %d ",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_CHG_LIMMIT_CUR:
		ret = battery->pdata->chg_charging_limit_current;
		pr_info("%s chg_charging_limit_current = %d ",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_COIL_TEMP_HIGH:
		break;
	case BATT_TUNE_COIL_TEMP_REC:
		break;
	case BATT_TUNE_COIL_LIMMIT_CUR:
		break;
	case BATT_TUNE_WPC_TEMP_HIGH:
		ret = battery->pdata->wpc_high_temp;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);		
		break;
	case BATT_TUNE_WPC_TEMP_HIGH_REC:
		ret = battery->pdata->wpc_high_temp_recovery;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);		
		break;
	case BATT_TUNE_DCHG_TEMP_HIGH:
		ret = battery->pdata->dchg_high_temp;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_DCHG_TEMP_HIGH_REC:
		ret = battery->pdata->dchg_high_temp_recovery;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_DCHG_BATT_TEMP_HIGH:
		ret = battery->pdata->dchg_high_batt_temp;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_DCHG_BATT_TEMP_HIGH_REC:
		ret = battery->pdata->dchg_high_batt_temp_recovery;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
#if defined(CONFIG_DIRECT_CHARGING)
	case BATT_TUNE_DCHG_LIMMIT_INPUT_CUR:
		ret = battery->pdata->dchg_input_limit_current;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_DCHG_LIMMIT_CHG_CUR:
		ret = battery->pdata->dchg_charging_limit_current;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
#endif
#endif
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	case BATT_UPDATE_DATA:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n",
				battery->data_path ? "OK" : "NOK");
		break;
#endif
	case BATT_MISC_EVENT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery->misc_event);
		break;
	case BATT_TX_EVENT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery->tx_event);
		if (battery->tx_event & BATT_TX_EVENT_WIRELESS_TX_ERR) {
			/* clear tx all event */
			sec_bat_set_tx_event(battery, 0, BATT_TX_EVENT_WIRELESS_ALL_MASK);
		}
		break;
	case BATT_EXT_DEV_CHG:
		break;
	case BATT_WDT_CONTROL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery->wdt_kick_disable);
		break;
	case MODE:
		value.strval = NULL;
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_MULTI_CHARGER_MODE, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n",
			(value.strval) ? value.strval : "master");
		break;
	case CHECK_PS_READY:
#if defined(CONFIG_CCIC_NOTIFIER)
#if defined(CONFIG_BATTERY_SAMSUNG_MHS)
		value.intval = battery->sub->pdic_ps_rdy;
#else
		value.intval = battery->pdic_ps_rdy;
#endif
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		pr_info("%s : CHECK_PS_READY=%d\n",__func__,value.intval);
#endif
		break;
	case BATT_CHIP_ID:
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_CHIP_ID, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		break;
	case ERROR_CAUSE:
		{
			int error_cause = SEC_BAT_ERROR_CAUSE_NONE;

			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_EXT_PROP_ERROR_CAUSE, value);
			error_cause |= value.intval;
			pr_info("%s: ERROR_CAUSE = 0x%X ",__func__, error_cause);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				error_cause);
		}
		break;
	case CISD_FULLCAPREP_MAX:
		{
			union power_supply_propval fullcaprep_val;

			fullcaprep_val.intval = SEC_BATTERY_CAPACITY_FULL;
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_ENERGY_NOW, fullcaprep_val);

			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
					fullcaprep_val.intval);
		}
		break;
#if defined(CONFIG_BATTERY_CISD)
	case CISD_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "%d", pcisd->data[CISD_DATA_RESET_ALG]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = CISD_DATA_RESET_ALG + 1; j < CISD_DATA_MAX_PER_DAY; j++) {
				snprintf(temp_buf+strlen(temp_buf), size, " %d", pcisd->data[j]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_DATA_JSON:
		{
			struct cisd *pcisd = &battery->cisd;
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "\"%s\":\"%d\"",
					cisd_data_str[CISD_DATA_RESET_ALG], pcisd->data[CISD_DATA_RESET_ALG]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = CISD_DATA_RESET_ALG + 1; j < CISD_DATA_MAX; j++) {
				if (battery->pdata->ignore_cisd_index[j / 32] & (0x1 << (j % 32)))
					continue;
				snprintf(temp_buf+strlen(temp_buf), size, ",\"%s\":\"%d\"", cisd_data_str[j], pcisd->data[j]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_DATA_D_JSON:
		{
			struct cisd *pcisd = &battery->cisd;
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "\"%s\":\"%d\"",
				cisd_data_str_d[CISD_DATA_FULL_COUNT_PER_DAY-CISD_DATA_MAX],
				pcisd->data[CISD_DATA_FULL_COUNT_PER_DAY]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = CISD_DATA_FULL_COUNT_PER_DAY + 1; j < CISD_DATA_MAX_PER_DAY; j++) {
				if (battery->pdata->ignore_cisd_index_d[(j - CISD_DATA_FULL_COUNT_PER_DAY) / 32] & (0x1 << ((j - CISD_DATA_FULL_COUNT_PER_DAY) % 32)))
					continue;
				snprintf(temp_buf+strlen(temp_buf), size, ",\"%s\":\"%d\"",
				cisd_data_str_d[j-CISD_DATA_MAX], pcisd->data[j]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}

			/* Clear Daily Data */
			for (j = CISD_DATA_FULL_COUNT_PER_DAY; j < CISD_DATA_MAX_PER_DAY; j++)
				pcisd->data[j] = 0;

			pcisd->data[CISD_DATA_FULL_COUNT_PER_DAY] = 1;
			pcisd->data[CISD_DATA_BATT_TEMP_MAX_PER_DAY] = -300;
			pcisd->data[CISD_DATA_CHG_TEMP_MAX_PER_DAY] = -300;
			pcisd->data[CISD_DATA_WPC_TEMP_MAX_PER_DAY] = -300;
			pcisd->data[CISD_DATA_USB_TEMP_MAX_PER_DAY] = -300;
			pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY] = 1000;
			pcisd->data[CISD_DATA_CHG_TEMP_MIN_PER_DAY] = 1000;
			pcisd->data[CISD_DATA_WPC_TEMP_MIN_PER_DAY] = 1000;
			pcisd->data[CISD_DATA_USB_TEMP_MIN_PER_DAY] = 1000;

			pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY] = -300;
			pcisd->data[CISD_DATA_CHG_CHG_TEMP_MAX_PER_DAY] = -300;
			pcisd->data[CISD_DATA_CHG_WPC_TEMP_MAX_PER_DAY] = -300;
			pcisd->data[CISD_DATA_CHG_USB_TEMP_MAX_PER_DAY] = -300;
			pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN_PER_DAY] = 1000;
			pcisd->data[CISD_DATA_CHG_CHG_TEMP_MIN_PER_DAY] = 1000;
			pcisd->data[CISD_DATA_CHG_WPC_TEMP_MIN_PER_DAY] = 1000;
			pcisd->data[CISD_DATA_CHG_USB_TEMP_MIN_PER_DAY] = 1000;

			pcisd->data[CISD_DATA_CAP_MIN_PER_DAY] = 0xFFFF;
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);

#if defined(CONFIG_WIRELESS_TX_MODE)
			/* clear accumulated power consumption by Tx */
			battery->tx_clear_cisd = true;
#endif
		}
		break;
	case CISD_WIRE_COUNT:
		{
			struct cisd *pcisd = &battery->cisd;
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				pcisd->data[CISD_DATA_WIRE_COUNT]);
		}
		break;
	case CISD_WC_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			struct pad_data *pad_data = pcisd->pad_array;
			char temp_buf[1024] = {0,};
			int j = 0;

			sprintf(temp_buf+strlen(temp_buf), "%d", pcisd->pad_count);
			while ((pad_data != NULL) && ((pad_data = pad_data->next) != NULL) &&
					(pad_data->id < MAX_PAD_ID) && (j++ < pcisd->pad_count))
				sprintf(temp_buf+strlen(temp_buf), " 0x%02x:%d", pad_data->id, pad_data->count);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_WC_DATA_JSON:
		{
			struct cisd *pcisd = &battery->cisd;
			struct pad_data *pad_data = pcisd->pad_array;
			char temp_buf[1024] = {0,};
			int j = 0;

			sprintf(temp_buf+strlen(temp_buf), "\"%s\":\"%d\"",
				PAD_INDEX_STRING, pcisd->pad_count);

			while ((pad_data != NULL) && ((pad_data = pad_data->next) != NULL) &&
					(pad_data->id < MAX_PAD_ID) && (j++ < pcisd->pad_count))
				sprintf(temp_buf+strlen(temp_buf), ",\"%s%02x\":\"%d\"",
					PAD_JSON_STRING, pad_data->id, pad_data->count);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_POWER_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			struct power_data *power_data = pcisd->power_array;
			char temp_buf[1024] = {0,};
			int j = 0;

			sprintf(temp_buf+strlen(temp_buf), "%d", pcisd->power_count);
			while ((power_data != NULL) && ((power_data = power_data->next) != NULL) &&
					(power_data->power < MAX_CHARGER_POWER) && (j++ < pcisd->power_count))
				sprintf(temp_buf+strlen(temp_buf), " %d:%d", power_data->power, power_data->count);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_POWER_DATA_JSON:
		{
			struct cisd *pcisd = &battery->cisd;
			struct power_data *power_data = pcisd->power_array;
			char temp_buf[1024] = {0,};
			int j = 0;

			sprintf(temp_buf+strlen(temp_buf), "\"%s\":\"%d\"",
				POWER_COUNT_JSON_STRING, pcisd->power_count);

			while ((power_data != NULL) && ((power_data = power_data->next) != NULL) &&
					(power_data->power < MAX_CHARGER_POWER) && (j++ < pcisd->power_count))
				sprintf(temp_buf+strlen(temp_buf), ",\"%s%d\":\"%d\"",
					POWER_JSON_STRING, power_data->power, power_data->count);

			/* clear daily power data */
			init_cisd_power_data(&battery->cisd);

			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_CABLE_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "%d", pcisd->cable_data[CISD_CABLE_TA]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = CISD_CABLE_TA + 1; j < CISD_CABLE_TYPE_MAX; j++) {
				snprintf(temp_buf+strlen(temp_buf), size, " %d", pcisd->cable_data[j]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);

		}
		break;

	case CISD_CABLE_DATA_JSON:
		{
			struct cisd *pcisd = &battery->cisd;
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "\"%s\":\"%d\"",
				cisd_cable_data_str[CISD_CABLE_TA], pcisd->cable_data[CISD_CABLE_TA]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = CISD_CABLE_TA + 1; j < CISD_CABLE_TYPE_MAX; j++) {
				snprintf(temp_buf+strlen(temp_buf), size, ",\"%s\":\"%d\"",
					cisd_cable_data_str[j], pcisd->cable_data[j]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}

			/* Clear Daily Cable Data */
			for (j = CISD_CABLE_TA; j < CISD_CABLE_TYPE_MAX; j++)
				pcisd->cable_data[j] = 0;

			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_TX_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "%d", pcisd->tx_data[TX_ON]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = TX_ON + 1; j < TX_DATA_MAX; j++) {
				snprintf(temp_buf+strlen(temp_buf), size, " %d", pcisd->tx_data[j]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_TX_DATA_JSON:
		{
			struct cisd *pcisd = &battery->cisd;
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "\"%s\":\"%d\"",
				cisd_tx_data_str[TX_ON], pcisd->tx_data[TX_ON]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = TX_ON + 1; j < TX_DATA_MAX; j++) {
				snprintf(temp_buf+strlen(temp_buf), size, ",\"%s\":\"%d\"",
					cisd_tx_data_str[j], pcisd->tx_data[j]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}

			/* Clear Daily Tx Data */
			for (j = TX_ON; j < TX_DATA_MAX; j++)
				pcisd->tx_data[j] = 0;

			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_EVENT_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "%d", pcisd->event_data[EVENT_DC_ERR]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = EVENT_DC_ERR + 1; j < EVENT_DATA_MAX; j++) {
				snprintf(temp_buf+strlen(temp_buf), size, " %d", pcisd->event_data[j]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_EVENT_DATA_JSON:
		{
			struct cisd *pcisd = &battery->cisd;
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "\"%s\":\"%d\"",
				cisd_event_data_str[EVENT_DC_ERR], pcisd->event_data[EVENT_DC_ERR]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = EVENT_DC_ERR + 1; j < EVENT_DATA_MAX; j++) {
				snprintf(temp_buf+strlen(temp_buf), size, ",\"%s\":\"%d\"",
					cisd_event_data_str[j], pcisd->event_data[j]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}

			/* Clear Daily Event Data */
			for (j = EVENT_DC_ERR; j < EVENT_DATA_MAX; j++)
				pcisd->event_data[j] = 0;

			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case PREV_BATTERY_DATA:
		{
			if (battery->enable_update_data)
				i += scnprintf(buf + i, PAGE_SIZE - i, "%d, %d, %d, %d\n",
					battery->voltage_now, battery->temperature,
					battery->is_jig_on, !battery->charging_block);
		}
		break;
	case PREV_BATTERY_INFO:
		{
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d,%d,%d,%d\n",
				battery->prev_volt, battery->prev_temp,
				battery->prev_jig_on, battery->prev_chg_on);
			pr_info("%s: Read Prev Battery Info : %d, %d, %d, %d\n", __func__,
				battery->prev_volt, battery->prev_temp,
				battery->prev_jig_on, battery->prev_chg_on);
		}
		break;
#endif
	case SAFETY_TIMER_SET:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       battery->safety_timer_set);
		break;
	case BATT_SWELLING_CONTROL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       battery->skip_swelling);
		break;
	case BATT_TEMP_CONTROL_TEST:
		{
			int temp_ctrl_t = 0;

			if (battery->current_event & SEC_BAT_CURRENT_EVENT_TEMP_CTRL_TEST)
				temp_ctrl_t = 1;
			else
				temp_ctrl_t = 0;

			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				temp_ctrl_t);
		}
		break;
	case SAFETY_TIMER_INFO:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%ld\n",
			       battery->cal_safety_time);
		break;
	case BATT_SHIPMODE_TEST:
		break;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	case BATT_TEMP_TEST:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d %d %d %d %d\n",
			battery->temperature_test_battery,
			battery->temperature_test_usb,
			battery->temperature_test_wpc,
			battery->temperature_test_chg,
			battery->temperature_test_dchg);
		break;
#endif
	case BATT_CURRENT_EVENT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->current_event);
		break;
	case BATT_JIG_GPIO:
 		value.intval = 0;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_JIG_GPIO, value);
		if(value.intval < 0) {
			value.intval = -1;
			pr_info("%s: dose not surpport JIG GPIO PIN READN \n", __func__);
		}
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			value.intval);
		break;
	case CC_INFO:
		{
			union power_supply_propval cc_val;

			cc_val.intval = SEC_BATTERY_CAPACITY_QH;
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_ENERGY_NOW, cc_val);

			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
					cc_val.intval);
		}
		break;
#if defined(CONFIG_WIRELESS_AUTH)
	case WC_AUTH_ADT_SENT:
		{
			//union power_supply_propval val = {0, };
			u8 auth_mode;

			psy_do_property(battery->pdata->wireless_charger_name, get,
				POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS, value);
			auth_mode = value.intval;
			if(auth_mode == WIRELESS_AUTH_WAIT)
				value.strval = "None";
			else if(auth_mode == WIRELESS_AUTH_START)
				value.strval = "Start";
			else if(auth_mode == WIRELESS_AUTH_SENT)
				value.strval = "Sent";
			else if(auth_mode == WIRELESS_AUTH_RECEIVED)
				value.strval = "Received";
			else if(auth_mode == WIRELESS_AUTH_FAIL)
				value.strval = "Fail";
			else if(auth_mode == WIRELESS_AUTH_PASS)
				value.strval = "Pass";
			else
				value.strval = "None";				
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", value.strval);
		}
		break;
#endif
	case WC_DUO_RX_POWER:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_RX_POWER, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			value.intval);
		break;
#if defined(CONFIG_BATTERY_SAMSUNG_MHS)
	case BATT_CHARGING_PORT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->charging_port);
		break;
#endif
	case EXT_EVENT:
		break;
#if defined(CONFIG_DIRECT_CHARGING)
	case DIRECT_CHARGING_STATUS:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->pd_list.now_isApdo);
		break;
	case DIRECT_CHARGING_STEP:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->step_charging_status);
		break;
	case DIRECT_CHARGING_IIN:
		if (is_pd_apdo_wire_type(battery->wire_status)) {
			value.intval = SEC_BATTERY_IIN_UA;
			psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_EXT_PROP_MEASURE_INPUT, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		} else {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
					0);
		}
		break;
	case SWITCH_CHARGING_SOURCE:
		psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_EXT_PROP_CHANGE_CHARGING_SOURCE, value);
		pr_info("%s Test Charging Source(%d) ",__func__, value.intval);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
#else
	case DIRECT_CHARGING_STATUS:
		ret = -1; /* DC not supported model returns -1 */
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", ret);
		break;
#endif
	case CHARGING_TYPE:
		{
			if (battery->cable_type > 0 && battery->cable_type < SEC_BATTERY_CABLE_MAX) {
				value.strval = sec_cable_type[battery->cable_type];
#if defined(CONFIG_DIRECT_CHARGING)
				if (is_pd_apdo_wire_type(battery->cable_type) &&
					battery->current_event & SEC_BAT_CURRENT_EVENT_DC_ERR)
					value.strval = "PDIC";
#endif
			} else
				value.strval = "UNKNOWN";
			pr_info("%s: CHARGING_TYPE = %s\n",__func__, value.strval);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", value.strval);
		}
		break;
#if defined(CONFIG_SEC_FACTORY)
	case BATT_FACTORY_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			factory_mode);
		break;
#endif
	case BATT_FULL_CAPACITY:
		pr_info("%s: BATT_FULL_CAPACITY = %d\n", __func__, battery->batt_full_capacity);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->batt_full_capacity);
		break;
	default:
		i = -EINVAL;
		break;
	}

	return i;
}

ssize_t sec_bat_store_attrs(
					struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - sec_battery_attrs;
	int ret = -EINVAL;
	int x = 0;
	int t[12];
	int i = 0;

	union power_supply_propval value = {0, };

	switch (offset) {
	case BATT_RESET_SOC:
		/* Do NOT reset fuel gauge in charging mode */
		if (is_nocharge_type(battery->cable_type) ||
			battery->is_jig_on) {
			sec_bat_set_misc_event(battery, BATT_MISC_EVENT_BATT_RESET_SOC, BATT_MISC_EVENT_BATT_RESET_SOC);

			value.intval =
				SEC_FUELGAUGE_CAPACITY_TYPE_RESET;
			psy_do_property(battery->pdata->fuelgauge_name, set,
					POWER_SUPPLY_PROP_CAPACITY, value);
			dev_info(battery->dev,"do reset SOC\n");
			/* update battery info */
			sec_bat_get_battery_info(battery);
		}
		ret = count;
		break;
	case BATT_READ_RAW_SOC:
		break;
	case BATT_READ_ADJ_SOC:
		break;
	case BATT_TYPE:
		strncpy(battery->batt_type, buf, sizeof(battery->batt_type) - 1);
		battery->batt_type[sizeof(battery->batt_type)-1] = '\0';
		ret = count;
		break;
	case BATT_VFOCV:
		break;
	case BATT_VOL_ADC:
		break;
	case BATT_VOL_ADC_CAL:
		break;
	case BATT_VOL_AVER:
		break;
	case BATT_VOL_ADC_AVER:
		break;
	case BATT_VOLTAGE_NOW:
		break;
	case BATT_CURRENT_UA_NOW:
		break;
	case BATT_CURRENT_UA_AVG:
		break;
	case BATT_FILTER_CFG:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			value.intval = x;
			psy_do_property(battery->pdata->fuelgauge_name, set,
					POWER_SUPPLY_PROP_FILTER_CFG, value);
			ret = count;
		}
		break;
	case BATT_TEMP:
#if defined(CONFIG_ENG_BATTERY_CONCEPT) || defined(CONFIG_SEC_FACTORY)
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_info(battery->dev,
				"%s: cooldown mode %s \n", __func__, (x ? "enable" : "disable"));
			if (x == 0)
				battery->cooldown_mode = false;
			else
				battery->cooldown_mode = true;
			ret = count;
		}
#endif
		break;
	case BATT_TEMP_ADC:
	case BATT_TEMP_AVER:
	case BATT_TEMP_ADC_AVER:
	case USB_TEMP:
	case USB_TEMP_ADC:
	case CHG_TEMP:
	case CHG_TEMP_ADC:
	case SLAVE_CHG_TEMP:
	case SLAVE_CHG_TEMP_ADC:
#if defined(CONFIG_DIRECT_CHARGING)
	case DCHG_ADC_MODE_CTRL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_info(battery->dev,
				 "%s : direct charger adc mode cntl : %d\n", __func__, x);
			value.intval = x;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_DIRECT_ADC_CTRL, value);
			ret = count;
		}
		break;
	case DCHG_TEMP:
	case DCHG_TEMP_ADC:
		break;
#endif
	case BATT_VF_ADC:
		break;
	case BATT_SLATE_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x == is_slate_mode(battery)) {
				dev_info(battery->dev,
					 "%s : skip same slate mode : %d\n", __func__, x);
				return count;
			} else if (x == 1) {
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SLATE, SEC_BAT_CURRENT_EVENT_SLATE);
				dev_info(battery->dev,
					"%s: enable slate mode : %d\n", __func__, x);
			} else if (x == 0) {
				sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SLATE);
				dev_info(battery->dev,
					"%s: disable slate mode : %d\n", __func__, x);
			} else {
				dev_info(battery->dev,
					"%s: SLATE MODE unknown command\n", __func__);
				return -EINVAL;
			}
			wake_lock(&battery->cable_wake_lock);
			queue_delayed_work(battery->monitor_wqueue,
					   &battery->cable_work, 0);
			ret = count;
		}
		break;
	case BATT_LP_CHARGING:
		break;
	case SIOP_ACTIVATED:
		break;
	case SIOP_LEVEL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_info(battery->dev,
					"%s: siop level: %d\n", __func__, x);

			battery->wc_heating_start_time = 0;
			if (x == battery->siop_level) {
				dev_info(battery->dev,
					"%s: skip same siop level: %d\n", __func__, x);
				return count;
			} else if (x >= 0 && x <= 100 && battery->pdata->temp_check_type) {
				battery->siop_level = x;
				if (battery->siop_level == 0)
					sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SIOP_LIMIT, SEC_BAT_CURRENT_EVENT_SIOP_LIMIT);
				else
					sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SIOP_LIMIT);
			} else {
				battery->siop_level = 100;
			}

			wake_lock(&battery->siop_level_wake_lock);
			queue_delayed_work(battery->monitor_wqueue, &battery->siop_level_work, 0);

			ret = count;
		}
		break;
	case SIOP_EVENT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_CHARGING_SOURCE:
		break;
	case FG_REG_DUMP:
		break;
	case FG_RESET_CAP:
		break;
	case FG_CAPACITY:
		break;
	case FG_ASOC:
		if (sscanf(buf, "%d\n", &x) == 1) {
			if (x >= 0 && x <= 100) {
				battery->batt_asoc = x;
#if defined(CONFIG_BATTERY_CISD)
				battery->cisd.data[CISD_DATA_ASOC] = x;
#endif
				sec_bat_check_battery_health(battery);
			}
			ret = count;
		}
		break;
	case AUTH:
		break;
	case CHG_CURRENT_ADC:
		break;
	case WC_ADC:
		break;
	case WC_STATUS:
		break;
	case WC_ENABLE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x == 0) {
				mutex_lock(&battery->wclock);
				battery->wc_enable = false;
				battery->wc_enable_cnt = 0;
				mutex_unlock(&battery->wclock);
			} else if (x == 1) {
				mutex_lock(&battery->wclock);
				battery->wc_enable = true;
				battery->wc_enable_cnt = 0;
				mutex_unlock(&battery->wclock);
			} else {
				dev_info(battery->dev,
					"%s: WPC ENABLE unknown command\n",
					__func__);
				return -EINVAL;
			}
			wake_lock(&battery->cable_wake_lock);
			queue_delayed_work(battery->monitor_wqueue,
					&battery->cable_work, 0);
			ret = count;
		}
		break;
	case WC_CONTROL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			char wpc_en_status[2];
	
			wpc_en_status[0] = WPC_EN_SYSFS;
			if (x == 0) {
				mutex_lock(&battery->wclock);
				battery->wc_enable = false;
				battery->wc_enable_cnt = 0;
				value.intval = 0;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_WC_CONTROL, value);

				wpc_en_status[1] = false;
				value.strval= wpc_en_status;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_WPC_EN, value);
				pr_info("%s: WC CONTROL: Disable\n", __func__);
				mutex_unlock(&battery->wclock);
			} else if (x == 1) {
				mutex_lock(&battery->wclock);
				battery->wc_enable = true;
				battery->wc_enable_cnt = 0;
				value.intval = 1;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_WC_CONTROL, value);

				wpc_en_status[1] = true;
				value.strval= wpc_en_status;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_WPC_EN, value);
				pr_info("%s: WC CONTROL: Enable\n", __func__);
				mutex_unlock(&battery->wclock);
			} else {
				dev_info(battery->dev,
					"%s: WC CONTROL unknown command\n",
					__func__);
				return -EINVAL;
			}
			ret = count;
		}
		break;
	case WC_CONTROL_CNT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			battery->wc_enable_cnt_value = x;
			ret = count;
		}
		break;
	case LED_COVER:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s: MFC, LED_COVER(%d)\n", __func__, x);
			battery->led_cover = x;
			value.intval = battery->led_cover;
			psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_PROP_FILTER_CFG, value);
			ret = count;
		}
		break;
	case HV_CHARGER_STATUS:
		break;
	case HV_WC_CHARGER_STATUS:
		break;
	case HV_CHARGER_SET:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_info(battery->dev,
				"%s: HV_CHARGER_SET(%d)\n", __func__, x);
			if (x == 1) {
				battery->wire_status = SEC_BATTERY_CABLE_9V_TA;
				wake_lock(&battery->cable_wake_lock);
				queue_delayed_work(battery->monitor_wqueue, &battery->cable_work, 0);
			} else {
				battery->wire_status = SEC_BATTERY_CABLE_NONE;
				wake_lock(&battery->cable_wake_lock);
				queue_delayed_work(battery->monitor_wqueue, &battery->cable_work, 0);
			}
			ret = count;
		}
		break;
	case FACTORY_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			battery->factory_mode = x ? true : false;
			ret = count;
		}
		break;
	case STORE_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
#if !defined(CONFIG_SEC_FACTORY)
			if (x) {
				battery->store_mode = true;
				wake_lock(&battery->parse_mode_dt_wake_lock);
				queue_delayed_work(battery->monitor_wqueue,
					&battery->parse_mode_dt_work, 0);
			}
#endif
			ret = count;
		}
		break;
	case UPDATE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			/* update battery info */
			sec_bat_get_battery_info(battery);
			ret = count;
		}
		break;
	case TEST_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			battery->test_mode = x;
			wake_lock(&battery->monitor_wake_lock);
			queue_delayed_work(battery->monitor_wqueue,
				&battery->monitor_work, 0);
			ret = count;
		}
		break;

	case BATT_EVENT_CALL:
	case BATT_EVENT_2G_CALL:
	case BATT_EVENT_TALK_GSM:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_3G_CALL:
	case BATT_EVENT_TALK_WCDMA:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_MUSIC:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_VIDEO:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_BROWSER:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_HOTSPOT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_CAMERA:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_CAMCORDER:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_DATA_CALL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_WIFI:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_WIBRO:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_LTE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_LCD:
		if (sscanf(buf, "%10d\n", &x) == 1) {
#if !defined(CONFIG_SEC_FACTORY)
			struct timespec ts;
			get_monotonic_boottime(&ts);
			if (x) {
				battery->lcd_status = true;
			} else {
				battery->lcd_status = false;
			}
			pr_info("%s : lcd_status (%d)\n", __func__, battery->lcd_status);

			if (battery->wc_tx_enable || battery->wpc_vout_ctrl_lcd_on) {
				battery->polling_short = false;
				wake_lock(&battery->monitor_wake_lock);
				queue_delayed_work(battery->monitor_wqueue,
					&battery->monitor_work, 0);
			}
#endif
			ret = count;
		}
		break;
	case BATT_EVENT_GPS:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_TEMP_TABLE:
		if (sscanf(buf, "%10d %10d %10d %10d %10d %10d %10d %10d\n",
			&t[0], &t[1], &t[2], &t[3], &t[4], &t[5], &t[6], &t[7]) == 8) {
			pr_info("%s: (new) %d %d %d %d %d %d %d %d\n",
				__func__, t[0], t[1], t[2], t[3], t[4], t[5], t[6], t[7]);
			pr_info("%s: (default) %d %d %d %d %d %d %d %d\n",
				__func__,
				battery->pdata->temp_high_threshold_normal,
				battery->pdata->temp_high_recovery_normal,
				battery->pdata->temp_low_threshold_normal,
				battery->pdata->temp_low_recovery_normal,
				battery->pdata->temp_high_threshold_lpm,
				battery->pdata->temp_high_recovery_lpm,
				battery->pdata->temp_low_threshold_lpm,
				battery->pdata->temp_low_recovery_lpm);
			update_external_temp_table(battery, t);
			ret = count;
		}
		break;
	case BATT_HIGH_CURRENT_USB:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			battery->is_hc_usb = x ? true : false;
			value.intval = battery->is_hc_usb;

			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_PROP_USB_HC, value);

			pr_info("%s: is_hc_usb (%d)\n", __func__, battery->is_hc_usb);
			ret = count;
		}
		break;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	case TEST_CHARGE_CURRENT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x >= 0 && x <= 2000) {
				dev_err(battery->dev,
					"%s: BATT_TEST_CHARGE_CURRENT(%d)\n", __func__, x);
				battery->pdata->charging_current[
					SEC_BATTERY_CABLE_USB].input_current_limit = x;
				battery->pdata->charging_current[
					SEC_BATTERY_CABLE_USB].fast_charging_current = x;
				if (x > 500) {
					battery->eng_not_full_status = true;
					battery->pdata->temp_check_type =
						SEC_BATTERY_TEMP_CHECK_NONE;
				}
				if (battery->cable_type == SEC_BATTERY_CABLE_USB) {
					value.intval = x;
					psy_do_property(battery->pdata->charger_name, set,
						POWER_SUPPLY_PROP_CURRENT_NOW,
						value);
				}
			}
			ret = count;
		}
		break;
#if defined(CONFIG_STEP_CHARGING)
	case TEST_STEP_CONDITION:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x >= 0 && x <= 100) {
				dev_err(battery->dev,
					"%s: TEST_STEP_CONDITION(%d)\n", __func__, x);
				battery->test_step_condition = x;
			}
			ret = count;
		}
		break;
#endif
#endif
	case SET_STABILITY_TEST:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_err(battery->dev,
				"%s: BATT_STABILITY_TEST(%d)\n", __func__, x);
			if (x) {
				battery->stability_test = true;
				battery->eng_not_full_status = true;
			}
			else {
				battery->stability_test = false;
				battery->eng_not_full_status = false;
			}
			ret = count;
		}
		break;
	case BATT_CAPACITY_MAX:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_err(battery->dev,
					"%s: BATT_CAPACITY_MAX(%d), fg_reset(%d)\n", __func__, x, fg_reset);
			if (!fg_reset && !battery->store_mode) {
				value.intval = x;
				psy_do_property(battery->pdata->fuelgauge_name, set,
						POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN, value);

				/* update soc */
				value.intval = 0;
				psy_do_property(battery->pdata->fuelgauge_name, get,
						POWER_SUPPLY_PROP_CAPACITY, value);
				battery->capacity = value.intval;
			} else {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				battery->fg_reset = 1;
#endif
			}
			ret = count;
		}
		break;
	case BATT_INBAT_VOLTAGE:
		break;
	case BATT_INBAT_VOLTAGE_OCV:
		break;
	case CHECK_SLAVE_CHG:
		break;
	case BATT_INBAT_WIRELESS_CS100:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s send cs100 command \n",__func__);
			value.intval = POWER_SUPPLY_STATUS_FULL;
			psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_PROP_STATUS, value);
			ret = count;
		}
		break;
	case HMT_TA_CONNECTED:
		if (sscanf(buf, "%10d\n", &x) == 1) {
#if !defined(CONFIG_CCIC_NOTIFIER)
			dev_info(battery->dev,
					"%s: HMT_TA_CONNECTED(%d)\n", __func__, x);
			if (x) {
				value.intval = false;
				psy_do_property(battery->pdata->charger_name, set,
						POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL,
						value);
				dev_info(battery->dev,
						"%s: changed to OTG cable detached\n", __func__);

				battery->wire_status = SEC_BATTERY_CABLE_HMT_CONNECTED;
				wake_lock(&battery->cable_wake_lock);
				queue_delayed_work(battery->monitor_wqueue, &battery->cable_work, 0);
			} else {
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
#endif
			ret = count;
		}
		break;
	case HMT_TA_CHARGE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
#if defined(CONFIG_CCIC_NOTIFIER)
			dev_info(battery->dev,
					"%s: HMT_TA_CHARGE(%d)\n", __func__, x);

			/* do not charge off without cable type, since wdt could be expired */
			if (x) {
				sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE);
				/* No charging when FULL & NONE */
				if(!is_nocharge_type(battery->cable_type) &&
					(battery->status != POWER_SUPPLY_STATUS_FULL) &&
					(battery->status != POWER_SUPPLY_STATUS_NOT_CHARGING)) {
					sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING);
				}
			} else if (!x && !is_nocharge_type(battery->cable_type)) {
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE,
						SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE);
				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_CHARGING_OFF);
			} else
				dev_info(battery->dev, "%s: Wrong HMT control\n", __func__);

			ret = count;
#else
			dev_info(battery->dev,
					"%s: HMT_TA_CHARGE(%d)\n", __func__, x);
			psy_do_property(battery->pdata->charger_name, get,
					POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, value);
			if (value.intval) {
				dev_info(battery->dev,
					"%s: ignore HMT_TA_CHARGE(%d)\n", __func__, x);
			} else {
				if (x) {
					value.intval = false;
					psy_do_property(battery->pdata->charger_name, set,
							POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL,
							value);
					dev_info(battery->dev,
						"%s: changed to OTG cable detached\n", __func__);
					battery->wire_status = SEC_BATTERY_CABLE_HMT_CHARGE;
					wake_lock(&battery->cable_wake_lock);
					queue_delayed_work(battery->monitor_wqueue, &battery->cable_work, 0);
				} else {
					value.intval = false;
					psy_do_property(battery->pdata->charger_name, set,
							POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL,
							value);
					dev_info(battery->dev,
							"%s: changed to OTG cable detached\n", __func__);
					battery->wire_status = SEC_BATTERY_CABLE_HMT_CONNECTED;
					wake_lock(&battery->cable_wake_lock);
					queue_delayed_work(battery->monitor_wqueue, &battery->cable_work, 0);
				}
			}
			ret = count;
#endif
		}
		break;
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	case FG_CYCLE:
		break;
	case FG_FULL_VOLTAGE:
		break;
	case FG_FULLCAPNOM:
		break;
	case BATTERY_CYCLE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_info(battery->dev, "%s: BATTERY_CYCLE(%d)\n", __func__, x);
			if (x >= 0) {
				int prev_battery_cycle = battery->batt_cycle;
				battery->batt_cycle = x;
#if defined(CONFIG_BATTERY_CISD)
				battery->cisd.data[CISD_DATA_CYCLE] = x;
#endif
				if (prev_battery_cycle < 0) {
					sec_bat_aging_check(battery);
				}
				sec_bat_check_battery_health(battery);
			}
			ret = count;
		}
		break;
	case BATTERY_CYCLE_TEST:
		sec_bat_aging_check(battery);
	break;
#endif
	case BATT_WPC_TEMP:
	case BATT_WPC_TEMP_ADC:
		break;
#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	case BATT_WIRELESS_FIRMWARE_UPDATE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (sec_bat_check_boost_mfc_condition(battery, x)) {
				if (x == SEC_WIRELESS_RX_SDCARD_MODE) {
					pr_info("%s fw mode is SDCARD \n", __func__);
					sec_bat_fw_update_work(battery, SEC_WIRELESS_RX_SDCARD_MODE);
				} else if (x == SEC_WIRELESS_RX_BUILT_IN_MODE) {
					pr_info("%s fw mode is BUILD IN \n", __func__);
					sec_bat_fw_update_work(battery, SEC_WIRELESS_RX_BUILT_IN_MODE);
				} else if (x == SEC_WIRELESS_TX_ON_MODE) {
					pr_info("%s tx mode is on \n", __func__);
					sec_bat_fw_update_work(battery, SEC_WIRELESS_TX_ON_MODE);
				} else if (x == SEC_WIRELESS_TX_OFF_MODE) {
					pr_info("%s tx mode is off \n", __func__);
					sec_bat_fw_update_work(battery, SEC_WIRELESS_TX_OFF_MODE);
				} else if (x == SEC_WIRELESS_RX_SPU_MODE) {
					pr_info("%s fw mode is SPU \n", __func__);
					sec_bat_fw_update_work(battery, SEC_WIRELESS_RX_SPU_MODE);
				} else {
					dev_info(battery->dev, "%s: wireless firmware unknown command\n", __func__);
					return -EINVAL;
				}
			} else
			pr_info("%s skip fw update at this time \n", __func__);
			ret = count;
		}
		break;
	case OTP_FIRMWARE_RESULT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x == 2) {
				value.intval = x;
				pr_info("%s RX firmware update ready!\n", __func__);
				psy_do_property(battery->pdata->wireless_charger_name, set,
								POWER_SUPPLY_PROP_MANUFACTURER, value);
			} else {
				dev_info(battery->dev, "%s: firmware unknown command\n", __func__);
				return -EINVAL;
			}
			ret = count;
		}
		break;
	case WC_IC_GRADE:
	case WC_IC_CHIP_ID:	
	case OTP_FIRMWARE_VER_BIN:
	case OTP_FIRMWARE_VER:
	case TX_FIRMWARE_RESULT:
	case TX_FIRMWARE_VER:
		break;
	case BATT_TX_STATUS:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x == SEC_TX_OFF) {
				pr_info("%s TX mode is off \n", __func__);
				sec_bat_fw_update_work(battery, SEC_WIRELESS_TX_OFF_MODE);
			} else if (x == SEC_TX_STANDBY) {
				pr_info("%s TX mode is on \n", __func__);
				sec_bat_fw_update_work(battery, SEC_WIRELESS_TX_ON_MODE);
			} else {
				dev_info(battery->dev, "%s: TX firmware unknown command\n", __func__);
				return -EINVAL;
			}
			ret = count;
		}
		break;
#endif
	case WC_VOUT:
	case WC_VRECT:
	case WC_RX_CONNECTED:
	case WC_RX_CONNECTED_DEV:
	case WC_TX_MFC_VIN_FROM_UNO:
	case WC_TX_MFC_IIN_FROM_UNO:
#if defined(CONFIG_WIRELESS_TX_MODE)
	case WC_TX_AVG_CURR:
	case WC_TX_TOTAL_PWR:
#endif

		break;
	case WC_TX_STOP_CAPACITY:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s : tx stop capacity (%d)%%\n", __func__, x);
			if (x >= 0 && x <= 100)
				battery->pdata->tx_stop_capacity = x;
			ret = count;
		}
		break;
	case WC_TX_TIMER_EN:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s : tx receiver detecting timer (%d)%%\n", __func__, x);
			value.intval = x;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TIMER_ON, value);
			ret = count;
		}
		break;
	case WC_TX_EN:
		if (sscanf(buf, "%10d\n", &x) == 1) {
#if !defined(CONFIG_BATTERY_SAMSUNG_MHS)
			if (mfc_fw_update) {
				pr_info("@Tx_Mode %s : skip Tx by mfc_fw_update\n", __func__);
				return count;
			}
				
			if (battery->wc_tx_enable == x) {
				pr_info("@Tx_Mode %s : Ignore same tx status\n", __func__);
				return count;
			}

			if (x &&
				(is_wireless_type(battery->cable_type) || (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_FAKE))) {
				pr_info("@Tx_Mode %s : Can't enable Tx mode during wireless charging\n", __func__);
				return count;
			} else {
				pr_info("@Tx_Mode %s: Set TX Enable (%d)\n", __func__, x);
				sec_wireless_set_tx_enable(battery, x);
				if (!x) {
					/* clear tx all event */
					sec_bat_set_tx_event(battery, 0, BATT_TX_EVENT_WIRELESS_ALL_MASK);
				}
#if defined(CONFIG_BATTERY_CISD)
				if (x)
					battery->cisd.tx_data[TX_ON]++;
#endif
			}
#endif
			ret = count;
		}
		break;
	case WC_TX_VOUT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("@Tx_Mode %s: Set TX Vout (%d)\n", __func__, x);
			battery->wc_tx_vout = value.intval = x;
			if (battery->wc_tx_enable) {
				pr_info("@Tx_Mode %s: set TX Vout (%d)\n", __func__, value.intval);
				psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT, value);
			} else {
				pr_info("@Tx_Mode %s: TX mode turned off now\n", __func__);
			}
			ret = count;
		}
		break;
	case BATT_HV_WIRELESS_STATUS:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x == 1 && is_hv_wireless_type(battery->cable_type)) {
				wake_lock(&battery->cable_wake_lock);
#ifdef CONFIG_SEC_FACTORY
				pr_info("%s change cable type HV WIRELESS -> WIRELESS \n", __func__);
				battery->wc_status = SEC_WIRELESS_PAD_WPC;
				battery->cable_type = SEC_BATTERY_CABLE_WIRELESS;
				sec_bat_set_charging_current(battery);
#endif
				pr_info("%s HV_WIRELESS_STATUS set to 1. Vout set to 5V. \n", __func__);
				value.intval = WIRELESS_VOUT_5V;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
				wake_unlock(&battery->cable_wake_lock);
			} else if (x == 3 && is_hv_wireless_type(battery->cable_type)) {
				pr_info("%s HV_WIRELESS_STATUS set to 3. Vout set to 10V. \n", __func__);
				value.intval = WIRELESS_VOUT_10V;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
			} else {
				dev_info(battery->dev, "%s: HV_WIRELESS_STATUS unknown command\n", __func__);
				return -EINVAL;
			}
			ret = count;
		}
		break;
	case BATT_HV_WIRELESS_PAD_CTRL:
		if (sscanf(buf, "%10d\n", &x) == 1) {

			pr_err("%s: x : %d\n", __func__, x);

			if (x == 1) {
#if defined(CM_OFFSET)
				ret = sec_set_param(CM_OFFSET, '1');
#endif
				if (ret < 0) {
					pr_err("%s:sec_set_param failed\n", __func__);
					return ret;
				} else {
					pr_info("%s: hv wireless charging is disabled\n", __func__);
					sleep_mode = true;
				}
			} else if (x == 2) {
#if defined(CM_OFFSET)
				ret = sec_set_param(CM_OFFSET, '0');
#endif
				if (ret < 0) {
					pr_err("%s: sec_set_param failed\n", __func__);
					return ret;
				} else {
					pr_info("%s: hv wireless charging is enabled\n", __func__);
					sleep_mode = false;
					battery->auto_mode = false;
					value.intval = WIRELESS_SLEEP_MODE_DISABLE;
					psy_do_property(battery->pdata->wireless_charger_name, set,
								POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
				}
			} else if (x == 3) {
				pr_info("%s led off \n", __func__);
				value.intval = WIRELESS_PAD_LED_OFF;
				psy_do_property(battery->pdata->wireless_charger_name, set,
								POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
			} else if (x == 4) {
				pr_info("%s led on \n", __func__);
				value.intval = WIRELESS_PAD_LED_ON;
				psy_do_property(battery->pdata->wireless_charger_name, set,
								POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
			} else {
				dev_info(battery->dev, "%s: BATT_HV_WIRELESS_PAD_CTRL unknown command\n", __func__);
				return -EINVAL;
			}

			wake_lock(&battery->monitor_wake_lock);
			queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
			ret = count;
		}
		break;
	case WC_TX_ID:
		break;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	case BATT_TUNE_FLOAT_VOLTAGE:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s float voltage = %d mV",__func__, x);

		if(x > 4000 && x <= 4400 ){
			value.intval = x;
			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_PROP_VOLTAGE_MAX, value);
		}
		break;
	case BATT_TUNE_INPUT_CHARGE_CURRENT:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s input charge current = %d mA",__func__, x);
		if(x >= 0 && x <= 4000 ){
			battery->test_max_current = true;
			for(i=0; i < SEC_BATTERY_CABLE_MAX; i++) {
				if(i != SEC_BATTERY_CABLE_USB)
					battery->pdata->charging_current[i].input_current_limit = x;
				pr_info("%s [%d] = %d mA",__func__, i, battery->pdata->charging_current[i].input_current_limit);
			}

			if(battery->cable_type != SEC_BATTERY_CABLE_USB) {
				value.intval = x;
				psy_do_property(battery->pdata->charger_name, set,
							POWER_SUPPLY_PROP_CURRENT_MAX, value);
			}
		}
		break;
	case BATT_TUNE_FAST_CHARGE_CURRENT:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s fast charge current = %d mA",__func__, x);
		if(x >= 0 && x <= 4000 ){
			battery->test_charge_current = true;
			for(i=0; i < SEC_BATTERY_CABLE_MAX; i++) {
				if(i != SEC_BATTERY_CABLE_USB)
					battery->pdata->charging_current[i].fast_charging_current = x;
				pr_info("%s [%d] = %d mA",__func__, i, battery->pdata->charging_current[i].fast_charging_current);
			}

			if(battery->cable_type != SEC_BATTERY_CABLE_USB) {
				value.intval = x;
					psy_do_property(battery->pdata->charger_name, set,
						POWER_SUPPLY_PROP_CURRENT_AVG, value);
			}
		}
		break;
	case BATT_TUNE_UI_TERM_CURRENT_1ST:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s ui term current = %d mA",__func__, x);

		if(x > 0 && x < 1000 ){
			battery->pdata->full_check_current_1st = x;
		}
		break;
	case BATT_TUNE_UI_TERM_CURRENT_2ND:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s ui term current = %d mA",__func__, x);

		if(x > 0 && x < 1000 ){
			battery->pdata->full_check_current_2nd = x;
		}
		break;
	case BATT_TUNE_TEMP_HIGH_NORMAL:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s temp high normal block	= %d ",__func__, x);
		if(x < 1000 && x >= -200)
			battery->pdata->temp_high_threshold_normal = x;
		break;
	case BATT_TUNE_TEMP_HIGH_REC_NORMAL:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s temp high normal recover  = %d ",__func__, x);
		if(x < 1000 && x >= -200)
			battery->pdata->temp_high_recovery_normal = x;
		break;
	case BATT_TUNE_TEMP_LOW_NORMAL:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s temp low normal block  = %d ",__func__, x);
		if(x < 1000 && x >= -200)
			battery->pdata->temp_low_threshold_normal = x;
		break;
	case BATT_TUNE_TEMP_LOW_REC_NORMAL:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s temp low normal recover  = %d ",__func__, x);
		if(x < 1000 && x >= -200)
			battery->pdata->temp_low_recovery_normal = x;
		break;
	case BATT_TUNE_CHG_TEMP_HIGH:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s chg_high_temp  = %d ",__func__, x);
		if(x < 1000 && x >= -200)
			battery->pdata->chg_high_temp = x;
		break;
	case BATT_TUNE_CHG_TEMP_REC:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s chg_high_temp_recovery	= %d ",__func__, x);
		if(x < 1000 && x >= -200)
			battery->pdata->chg_high_temp_recovery = x;
		break;
	case BATT_TUNE_CHG_LIMMIT_CUR:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s chg_charging_limit_current	= %d ", __func__, x);
		if(x < 3000 && x > 0)
		{
			battery->pdata->chg_charging_limit_current = x;
			battery->pdata->charging_current[SEC_BATTERY_CABLE_9V_ERR].input_current_limit= x;
			battery->pdata->charging_current[SEC_BATTERY_CABLE_9V_UNKNOWN].input_current_limit= x;
			battery->pdata->charging_current[SEC_BATTERY_CABLE_9V_TA].input_current_limit= x;
		}
		break;
	case BATT_TUNE_COIL_TEMP_HIGH:
		break;
	case BATT_TUNE_COIL_TEMP_REC:
		break;
	case BATT_TUNE_COIL_LIMMIT_CUR:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s wpc_input_limit_current	= %d ",__func__, x);
		if(x < 3000 && x > 0)
		{
			battery->pdata->charging_current[SEC_BATTERY_CABLE_9V_ERR].input_current_limit= x;
			battery->pdata->charging_current[SEC_BATTERY_CABLE_9V_UNKNOWN].input_current_limit= x;
			battery->pdata->charging_current[SEC_BATTERY_CABLE_9V_TA].input_current_limit= x;
		}
		break;
	case BATT_TUNE_WPC_TEMP_HIGH:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s wpc_high_temp = %d ",__func__, x);
		battery->pdata->wpc_high_temp = x;
		break;
	case BATT_TUNE_WPC_TEMP_HIGH_REC:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s wpc_high_temp_recovery = %d ",__func__, x);
		battery->pdata->wpc_high_temp_recovery = x;
		break;
	case BATT_TUNE_DCHG_TEMP_HIGH:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s dchg_high_temp = %d ",__func__, x);
		battery->pdata->dchg_high_temp = x;
		break;
	case BATT_TUNE_DCHG_TEMP_HIGH_REC:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s dchg_high_temp_recovery = %d ",__func__, x);
		battery->pdata->dchg_high_temp_recovery = x;
		break;
	case BATT_TUNE_DCHG_BATT_TEMP_HIGH:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s dchg_high_batt_temp = %d ",__func__, x);
		battery->pdata->dchg_high_batt_temp = x;
		break;
	case BATT_TUNE_DCHG_BATT_TEMP_HIGH_REC:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s dchg_high_batt_temp_recovery = %d ",__func__, x);
		battery->pdata->dchg_high_batt_temp_recovery = x;
		break;		
#if defined(CONFIG_DIRECT_CHARGING)
	case BATT_TUNE_DCHG_LIMMIT_INPUT_CUR:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s dchg_input_limit_current = %d ",__func__, x);
		battery->pdata->dchg_input_limit_current = x;
		break;
	case BATT_TUNE_DCHG_LIMMIT_CHG_CUR:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s dchg_charging_limit_current = %d ",__func__, x);
		battery->pdata->dchg_charging_limit_current = x;
		break;
#endif
#endif		
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	case BATT_UPDATE_DATA:
		if (!battery->data_path && (count * sizeof(char)) < 256) {
			battery->data_path = kzalloc((count * sizeof(char) + 1), GFP_KERNEL);
			if (battery->data_path) {
				sscanf(buf, "%s\n", battery->data_path);
				cancel_delayed_work(&battery->batt_data_work);
				wake_lock(&battery->batt_data_wake_lock);
				queue_delayed_work(battery->monitor_wqueue,
					&battery->batt_data_work, msecs_to_jiffies(100));
			} else {
				pr_info("%s: failed to alloc data_path buffer\n", __func__);
			}
		}
		ret = count;
		break;
#endif
	case BATT_MISC_EVENT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s: PMS sevice hiccup read done : %d ", __func__, x);
			if (battery->misc_event & (BATT_MISC_EVENT_HICCUP_TYPE | BATT_MISC_EVENT_TEMP_HICCUP_TYPE)) {
				if (!battery->hiccup_status) {
					sec_bat_set_misc_event(battery,
						0, (BATT_MISC_EVENT_HICCUP_TYPE | BATT_MISC_EVENT_TEMP_HICCUP_TYPE));
				} else {
					battery->hiccup_clear = true;
					pr_info("%s : Hiccup event doesn't clear. Hiccup clear bit set (%d)\n", __func__, battery->hiccup_clear);
				}
			}
		}
		ret = count;
		break;
	case BATT_EXT_DEV_CHG:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s: Connect Ext Device : %d ",__func__, x);

			switch (x) {
				case EXT_DEV_NONE:
					battery->wire_status = SEC_BATTERY_CABLE_NONE;
					value.intval = 0;
					break;
				case EXT_DEV_GAMEPAD_CHG:
					battery->wire_status = SEC_BATTERY_CABLE_TA;
					value.intval = 0;
					break;
				case EXT_DEV_GAMEPAD_OTG:
					battery->wire_status = SEC_BATTERY_CABLE_OTG;
					value.intval = 1;
					break;
				default:
					break;
			}

			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL,
					value);

			queue_delayed_work(battery->monitor_wqueue,
					&battery->cable_work, 0);
			ret = count;
		}
		break;
	case BATT_WDT_CONTROL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s: Charger WDT Set : %d\n", __func__, x);
			battery->wdt_kick_disable = x;
#if defined(CONFIG_ENG_BATTERY_CONCEPT) && defined(CONFIG_DIRECT_CHARGING)
			value.intval = x;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_DIRECT_WDT_CONTROL, value);
#endif
		}
		ret = count;
		break;
	case MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			value.intval = x;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_MULTI_CHARGER_MODE, value);
			ret = count;
		}
		break;
	case CHECK_PS_READY:
	case BATT_CHIP_ID:
		break;
	case ERROR_CAUSE:
		break;
	case CISD_FULLCAPREP_MAX:
		break;
#if defined(CONFIG_BATTERY_CISD)
	case CISD_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			int temp_data[CISD_DATA_MAX_PER_DAY] = {0,};

			sscanf(buf, "%10d\n", &temp_data[0]);

			if (temp_data[CISD_DATA_RESET_ALG] > 0) {
				if (sscanf(buf, "%10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d\n",
					&temp_data[0], &temp_data[1], &temp_data[2],
					&temp_data[3], &temp_data[4], &temp_data[5],
					&temp_data[6], &temp_data[7], &temp_data[8],
					&temp_data[9], &temp_data[10], &temp_data[11],
					&temp_data[12], &temp_data[13], &temp_data[14],
					&temp_data[15], &temp_data[16], &temp_data[17],
					&temp_data[18], &temp_data[19], &temp_data[20],
					&temp_data[21], &temp_data[22], &temp_data[23],
					&temp_data[24], &temp_data[25], &temp_data[26],
					&temp_data[27], &temp_data[28], &temp_data[29],
					&temp_data[30], &temp_data[31], &temp_data[32],
					&temp_data[33], &temp_data[34], &temp_data[35],
					&temp_data[36], &temp_data[37], &temp_data[38],
					&temp_data[39], &temp_data[40], &temp_data[41],
					&temp_data[42], &temp_data[43], &temp_data[44],
					&temp_data[45], &temp_data[46], &temp_data[47],
					&temp_data[48], &temp_data[49], &temp_data[50],
					&temp_data[51], &temp_data[52], &temp_data[53],
					&temp_data[54], &temp_data[55], &temp_data[56],
					&temp_data[57], &temp_data[58], &temp_data[59],
					&temp_data[60], &temp_data[61], &temp_data[62],
					&temp_data[63], &temp_data[64], &temp_data[65],
					&temp_data[66], &temp_data[67], &temp_data[68],
					&temp_data[69], &temp_data[70], &temp_data[71],
					&temp_data[72], &temp_data[73], &temp_data[74],
					&temp_data[75], &temp_data[76]) <= CISD_DATA_MAX_PER_DAY) {
					for (i = 0; i < CISD_DATA_MAX_PER_DAY; i++)
						pcisd->data[i] = 0;

					pcisd->data[CISD_DATA_ALG_INDEX] = battery->pdata->cisd_alg_index;
					pcisd->data[CISD_DATA_FULL_COUNT] = temp_data[0];
					pcisd->data[CISD_DATA_CAP_MAX] = temp_data[1];
					pcisd->data[CISD_DATA_CAP_MIN] = temp_data[2];
					pcisd->data[CISD_DATA_VALERT_COUNT] = temp_data[16];
					pcisd->data[CISD_DATA_CYCLE] = temp_data[17];
					pcisd->data[CISD_DATA_WIRE_COUNT] = temp_data[18];
					pcisd->data[CISD_DATA_WIRELESS_COUNT] = temp_data[19];
					pcisd->data[CISD_DATA_HIGH_TEMP_SWELLING] = temp_data[20];
					pcisd->data[CISD_DATA_LOW_TEMP_SWELLING] = temp_data[21];
					pcisd->data[CISD_DATA_WC_HIGH_TEMP_SWELLING] = temp_data[22];
					pcisd->data[CISD_DATA_AICL_COUNT] = temp_data[26];
					pcisd->data[CISD_DATA_BATT_TEMP_MAX] = temp_data[27];
					pcisd->data[CISD_DATA_BATT_TEMP_MIN] = temp_data[28];
					pcisd->data[CISD_DATA_CHG_TEMP_MAX] = temp_data[29];
					pcisd->data[CISD_DATA_CHG_TEMP_MIN] = temp_data[30];
					pcisd->data[CISD_DATA_WPC_TEMP_MAX] = temp_data[31];
					pcisd->data[CISD_DATA_WPC_TEMP_MIN] = temp_data[32];
					pcisd->data[CISD_DATA_UNSAFETY_VOLTAGE] = temp_data[33];
					pcisd->data[CISD_DATA_UNSAFETY_TEMPERATURE] = temp_data[34];
					pcisd->data[CISD_DATA_SAFETY_TIMER] = temp_data[35];
					pcisd->data[CISD_DATA_VSYS_OVP] = temp_data[36];
					pcisd->data[CISD_DATA_VBAT_OVP] = temp_data[37];
				}
			} else {
				const char *p = buf;

				pr_info("%s: %s\n", __func__, buf);
				for (i = CISD_DATA_RESET_ALG; i < CISD_DATA_MAX_PER_DAY; i++) {
					if (sscanf(p, "%10d%n", &pcisd->data[i], &x) > 0) {
						p += (size_t)x;
						if (pcisd->data[CISD_DATA_ALG_INDEX] != battery->pdata->cisd_alg_index) {
							pr_info("%s: ALG_INDEX is changed %d -> %d\n", __func__,
								pcisd->data[CISD_DATA_ALG_INDEX], battery->pdata->cisd_alg_index);
							temp_data[CISD_DATA_RESET_ALG] = -1;
							break;
						}
					} else {
						pr_info("%s: NO DATA (cisd_data)\n", __func__);
						temp_data[CISD_DATA_RESET_ALG] = -1;
						break;
					}
				}

				pr_info("%s: %s cisd data\n", __func__,
					((temp_data[CISD_DATA_RESET_ALG] < 0 || battery->fg_reset) ? "init" : "update"));

				if (temp_data[CISD_DATA_RESET_ALG] < 0 || battery->fg_reset) {
					/* initialize data */
					for (i = CISD_DATA_RESET_ALG; i < CISD_DATA_MAX_PER_DAY; i++)
						pcisd->data[i] = 0;

					battery->fg_reset = 0;

					pcisd->data[CISD_DATA_ALG_INDEX] = battery->pdata->cisd_alg_index;

					pcisd->data[CISD_DATA_FULL_COUNT] = 1;
					pcisd->data[CISD_DATA_BATT_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_WPC_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_USB_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_BATT_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CHG_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_WPC_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_USB_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CAP_MIN] = 0xFFFF;

					pcisd->data[CISD_DATA_FULL_COUNT_PER_DAY] = 1;
					pcisd->data[CISD_DATA_BATT_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_WPC_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_USB_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_CHG_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_WPC_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_USB_TEMP_MIN_PER_DAY] = 1000;

					pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_CHG_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_WPC_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_USB_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CHG_CHG_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CHG_WPC_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CHG_USB_TEMP_MIN] = 1000;

					pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_CHG_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_WPC_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_USB_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_CHG_CHG_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_CHG_WPC_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_CHG_USB_TEMP_MIN_PER_DAY] = 1000;

					pcisd->data[CISD_DATA_CAP_MIN_PER_DAY] = 0xFFFF;
					pcisd->data[CISD_DATA_USB_OVERHEAT_RAPID_CHANGE] = 0;
					pcisd->data[CISD_DATA_USB_OVERHEAT_RAPID_CHANGE_PER_DAY] = 0;
					pcisd->data[CISD_DATA_USB_OVERHEAT_ALONE] = 0;

					/* initialize pad data */
					init_cisd_pad_data(&battery->cisd);

					/* initialize power data */
					init_cisd_power_data(&battery->cisd);
				}
			}
			ret = count;
			wake_lock(&battery->monitor_wake_lock);
			queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		}
		break;
	case CISD_DATA_JSON:
		{
			char tc;
			struct cisd *pcisd = &battery->cisd;

			if (sscanf(buf, "%1c\n", &tc) == 1) {
				if (tc == 'c') {
					for (i = 0; i < CISD_DATA_MAX; i++)
						pcisd->data[i] = 0;

					pcisd->data[CISD_DATA_FULL_COUNT] = 1;
					pcisd->data[CISD_DATA_BATT_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_WPC_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_USB_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_BATT_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CHG_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_WPC_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_USB_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CAP_MIN] = 0xFFFF;

					pcisd->data[CISD_DATA_FULL_COUNT_PER_DAY] = 1;
					pcisd->data[CISD_DATA_BATT_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_WPC_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_USB_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_CHG_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_WPC_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_USB_TEMP_MIN_PER_DAY] = 1000;

					pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_CHG_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_WPC_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_USB_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CHG_CHG_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CHG_WPC_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CHG_USB_TEMP_MIN] = 1000;

					pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_CHG_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_WPC_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_USB_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_CHG_CHG_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_CHG_WPC_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_CHG_USB_TEMP_MIN_PER_DAY] = 1000;

					pcisd->data[CISD_DATA_CAP_MIN_PER_DAY] = 0xFFFF;
					pcisd->data[CISD_DATA_USB_OVERHEAT_RAPID_CHANGE] = 0;
					pcisd->data[CISD_DATA_USB_OVERHEAT_RAPID_CHANGE_PER_DAY] = 0;
					pcisd->data[CISD_DATA_USB_OVERHEAT_ALONE] = 0;
				}
			}
			ret = count;
		}
		break;
	case CISD_DATA_D_JSON:
		break;
	case CISD_WIRE_COUNT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			struct cisd *pcisd = &battery->cisd;
			pr_info("%s: Wire Count : %d\n", __func__, x);
			pcisd->data[CISD_DATA_WIRE_COUNT] = x;
			pcisd->data[CISD_DATA_WIRE_COUNT_PER_DAY]++;
		}
		ret = count;
		break;
	case CISD_WC_DATA:
		set_cisd_pad_data(battery, buf);
		ret = count;
		break;
	case CISD_WC_DATA_JSON:
		break;
	case CISD_POWER_DATA:
		set_cisd_power_data(battery, buf);
		ret = count;
		break;
	case CISD_POWER_DATA_JSON:
		break;
	case CISD_CABLE_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			const char *p = buf;

			pr_info("%s: %s\n", __func__, buf);
			for (i = CISD_CABLE_TA; i < CISD_CABLE_TYPE_MAX; i++) {
				if (sscanf(p, "%10d%n", &pcisd->cable_data[i], &x) > 0) {
					p += (size_t)x;
				} else {
					pr_info("%s: NO DATA (CISD_CABLE_TYPE)\n", __func__);
					pcisd->cable_data[i] = 0;
					break;
				}
			}
		}
		ret = count;
		break;

	case CISD_CABLE_DATA_JSON:
		break;
	case CISD_TX_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			const char *p = buf;

			pr_info("%s: %s\n", __func__, buf);
			for (i = TX_ON; i < TX_DATA_MAX; i++) {
				if (sscanf(p, "%10d%n", &pcisd->tx_data[i], &x) > 0) {
					p += (size_t)x;
				} else {
					pr_info("%s: NO DATA (CISD_TX_DATA)\n", __func__);
					pcisd->tx_data[i] = 0;
					break;
				}
			}
		}
		ret = count;
		break;
	case CISD_TX_DATA_JSON:
		break;
	case CISD_EVENT_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			const char *p = buf;

			pr_info("%s: %s\n", __func__, buf);
			for (i = EVENT_DC_ERR; i < EVENT_DATA_MAX; i++) {
				if (sscanf(p, "%10d%n", &pcisd->event_data[i], &x) > 0) {
					p += (size_t)x;
				} else {
					pr_info("%s: NO DATA (CISD_EVENT_DATA)\n", __func__);
					pcisd->event_data[i] = 0;
					break;
				}
			}
		}
		ret = count;
		break;
	case CISD_EVENT_DATA_JSON:
		break;
	case PREV_BATTERY_DATA:
		if (sscanf(buf, "%10d, %10d, %10d, %10d\n",
				&battery->prev_volt, &battery->prev_temp,
				&battery->prev_jig_on, &battery->prev_chg_on) >= 4) {
			pr_info("%s: prev voltage : %d, prev_temp : %d, prev_jig_on : %d, prev_chg_on : %d\n",
				__func__, battery->prev_volt, battery->prev_temp,
				battery->prev_jig_on, battery->prev_chg_on);

			if (battery->prev_volt >= 3700 && battery->prev_temp >= 150 &&
					!battery->prev_jig_on && battery->fg_reset)
				pr_info("%s: Battery have been Removed\n", __func__);

			ret = count;
		}
		battery->enable_update_data = 1;
		break;
	case PREV_BATTERY_INFO:
		break;
#endif
	case SAFETY_TIMER_SET:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x) {
				battery->safety_timer_set = true;
			} else {
				battery->safety_timer_set = false;
			}
			ret = count;
		}
		break;
	case BATT_SWELLING_CONTROL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x) {
				pr_info("%s : 15TEST START!! SWELLING MODE DISABLE\n", __func__);
				battery->skip_swelling = true;
			} else {
				pr_info("%s : 15TEST END!! SWELLING MODE END\n", __func__);
				battery->skip_swelling = false;
			}
			ret = count;
		}
		break;
	case BATT_TEMP_CONTROL_TEST:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x) {
#if !defined(CONFIG_BATTERY_SAMSUNG_MHS)
				sec_bat_set_temp_control_test(battery, true);
#endif
#if defined(CM_OFFSET)
				ret = sec_set_param(CM_OFFSET + 2, '1');
				if (ret < 0) {
					pr_err("%s:sec_set_param failed\n", __func__);
					return ret;
				} else {
					pr_info("%s:batt_temp_control_test param is disabled\n", __func__);
				}
#endif
			} else {
#if !defined(CONFIG_BATTERY_SAMSUNG_MHS)
				sec_bat_set_temp_control_test(battery, false);
#endif
#if defined(CM_OFFSET)
				ret = sec_set_param(CM_OFFSET + 2, '0');
				if (ret < 0) {
					pr_err("%s:sec_set_param failed\n", __func__);
					return ret;
				} else {
					pr_info("%s: batt_temp_control_test param is enabled\n", __func__);
				}
#endif
			}
			ret = count;
		}
		break;
	case SAFETY_TIMER_INFO:
		break;
	case BATT_SHIPMODE_TEST:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s ship mode test %d\n", __func__, x);
			value.intval = x;
			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_SHIPMODE_TEST, value);
			ret = count;
		}
		break;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	case BATT_TEMP_TEST:
	{
		char tc;
		if (sscanf(buf, "%c %10d\n", &tc, &x) == 2) {
			pr_info("%s : temperature t: %c, temp: %d\n", __func__, tc, x);
			if (tc == 'u') {
				if (x > 900)
					battery->pdata->usb_temp_check_type = 0;
				else
					battery->temperature_test_usb = x;
			} else if (tc == 'w') {
				if (x > 900)
					battery->pdata->wpc_temp_check_type = 0;
				else
					battery->temperature_test_wpc = x;
			} else if (tc == 'b') {
				if (x > 900)
					battery->pdata->temp_check_type = 0;
				else
					battery->temperature_test_battery = x;
			} else if (tc == 'c') {
				if (x > 900)
					battery->pdata->chg_temp_check_type = 0;
				else
					battery->temperature_test_chg = x;
#if defined(CONFIG_DIRECT_CHARGING)
			} else if (tc == 'd') {
				if (x > 900)
					battery->pdata->dchg_temp_check_type = 0;
				else
					battery->temperature_test_dchg = x;
#endif
			}
			ret = count;
		}
		break;
	}
#endif
	case BATT_CURRENT_EVENT:
		break;
	case BATT_JIG_GPIO:
		break;
	case CC_INFO:
		break;
#if defined(CONFIG_WIRELESS_AUTH)
	case WC_AUTH_ADT_SENT:
		break;
#endif
#if defined(CONFIG_BATTERY_SAMSUNG_MHS)
	case BATT_CHARGING_PORT:
		break;
#endif
	case EXT_EVENT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_info(battery->dev,
				"%s: ext event 0x%x \n", __func__, x);
			battery->ext_event = x;
			wake_lock(&battery->ext_event_wake_lock);
			queue_delayed_work(battery->monitor_wqueue, &battery->ext_event_work, 0);
			ret = count;
		}
		break;
#if defined(CONFIG_DIRECT_CHARGING)
	case DIRECT_CHARGING_STATUS:
		break;
	case DIRECT_CHARGING_STEP:
		break;
	case DIRECT_CHARGING_IIN:
		break;
	case SWITCH_CHARGING_SOURCE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (is_pd_apdo_wire_type(battery->cable_type)) {
				dev_info(battery->dev, "%s: Request Change Charging Source : %s \n",
					__func__, x == 0 ? "Switch Charger" : "Direct Charger" );

				value.intval = (x == 0) ? SEC_DIRECT_CHG_CHARGING_SOURCE_SWITCHING : SEC_DIRECT_CHG_CHARGING_SOURCE_DIRECT;
				psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_CHANGE_CHARGING_SOURCE, value);
			}
		}
		break;
#endif
	case CHARGING_TYPE:
		break;
#if defined(CONFIG_SEC_FACTORY)
	case BATT_FACTORY_MODE:
		break;
#endif
	case BOOT_COMPLETED:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			boot_complete = true;
			dev_info(battery->dev,
				"%s: boot completed(%d)\n", __func__, boot_complete);
#if defined(CONFIG_WIRELESS_IC_PARAM)
			psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_WIRELESS_PARAM_INFO, value);
#endif
			ret = count;
		}
		break;
	case BATT_FULL_CAPACITY:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x >= 0 && x <= 100) {
				pr_info("%s: update BATT_FULL_CAPACITY(%d)\n", __func__, x);
				battery->batt_full_capacity = x;

				wake_lock(&battery->monitor_wake_lock);
				queue_delayed_work(battery->monitor_wqueue,
					&battery->monitor_work, 0);
			} else {
				pr_info("%s: out of range(%d)\n", __func__, x);
			}
			ret = count;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

int sec_bat_create_attrs(struct device *dev)
{
	unsigned long i = 0;
	int rc = 0;

	for (i = 0; i < ARRAY_SIZE(sec_battery_attrs); i++) {
		rc = device_create_file(dev, &sec_battery_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	goto create_attrs_succeed;

create_attrs_failed:
	while (i--)
		device_remove_file(dev, &sec_battery_attrs[i]);
create_attrs_succeed:
	return rc;
}

