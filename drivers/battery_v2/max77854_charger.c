/*
 *  max77854_charger.c
 *  Samsung MAX77854 Charger Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define DEBUG

#include <linux/mfd/max77854-private.h>
#if defined(CONFIG_BATTERY_NOTIFIER)
#include <linux/battery/battery_notifier.h>
#endif
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/power_supply.h>
#include <linux/mfd/max77854.h>
#include <linux/of_gpio.h>
#include "include/charger/max77854_charger.h"
#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/usb_notify.h>
#endif

#define ENABLE 1
#define DISABLE 0

#if defined(CONFIG_SEC_FACTORY)
#define WC_CURRENT_WORK_STEP	250
#else
#define WC_CURRENT_WORK_STEP	1000
#endif

extern unsigned int lpcharge;

static enum power_supply_property max77854_charger_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL,
	POWER_SUPPLY_PROP_USB_HC,
#if defined(CONFIG_BATTERY_SWELLING)
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
#endif
#if defined(CONFIG_AFC_CHARGER_MODE)
	POWER_SUPPLY_PROP_AFC_CHARGER_MODE,
#endif
	POWER_SUPPLY_PROP_CHARGE_NOW,
};

static enum power_supply_property max77854_otg_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static struct device_attribute max77854_charger_attrs[] = {
	MAX77854_CHARGER_ATTR(chip_id),
	MAX77854_CHARGER_ATTR(data),
};

static void max77854_charger_initialize(struct max77854_charger_data *charger);
static int max77854_get_vbus_state(struct max77854_charger_data *charger);
static int max77854_get_charger_state(struct max77854_charger_data *charger);
static void max77854_set_charger_state(struct max77854_charger_data *charger,
				       int enable);
static void max77854_enable_aicl_irq(struct max77854_charger_data *charger);
static bool max77854_charger_unlock(struct max77854_charger_data *charger)
{
	u8 reg_data;
	u8 chgprot;
	int retry_cnt = 0;
	bool need_init = false;

	do {
		max77854_read_reg(charger->i2c, MAX77854_CHG_REG_CNFG_06, &reg_data);
		chgprot = ((reg_data & 0x0C) >> 2);
		if (chgprot != 0x03) {
			pr_err("%s: unlock err, chgprot(0x%x), retry(%d)\n",
					__func__, chgprot, retry_cnt);
			max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_06,
					(0x03 << 2), (0x03 << 2));
			need_init = true;
			msleep(20);
		} else {
			pr_debug("%s: unlock success, chgprot(0x%x)\n",
				__func__, chgprot);
			break;
		}
	} while ((chgprot != 0x03) && (++retry_cnt < 10));

	return need_init;
}

static void check_charger_unlock_state(struct max77854_charger_data *charger)
{
	bool need_reg_init;
	pr_debug("%s\n", __func__);

	need_reg_init = max77854_charger_unlock(charger);
	if (need_reg_init) {
		pr_err("%s: charger locked state, reg init\n", __func__);
		max77854_charger_initialize(charger);
	}
}

static void max77854_test_read(struct max77854_charger_data *charger)
{
	u8 data = 0;
	u32 addr = 0;
	char str[1024]={0,};

	for (addr = 0xB1; addr <= 0xC3; addr++) {
		max77854_read_reg(charger->i2c, addr, &data);
		sprintf(str + strlen(str), "[0x%02x]0x%02x, ", addr, data);
	}
	pr_info("MAX77854 : %s\n", str);
}

static int max77854_get_vbus_state(struct max77854_charger_data *charger)
{
	u8 reg_data;

	max77854_read_reg(charger->i2c,
			  MAX77854_CHG_REG_DETAILS_00, &reg_data);

	if (is_wireless_type(charger->cable_type))
		reg_data = ((reg_data & MAX77854_WCIN_DTLS) >>
			    MAX77854_WCIN_DTLS_SHIFT);
	else
		reg_data = ((reg_data & MAX77854_CHGIN_DTLS) >>
			    MAX77854_CHGIN_DTLS_SHIFT);

	switch (reg_data) {
	case 0x00:
		pr_info("%s: VBUS is invalid. CHGIN < CHGIN_UVLO\n",
			__func__);
		break;
	case 0x01:
		pr_info("%s: VBUS is invalid. CHGIN < MBAT+CHGIN2SYS" \
			"and CHGIN > CHGIN_UVLO\n", __func__);
		break;
	case 0x02:
		pr_info("%s: VBUS is invalid. CHGIN > CHGIN_OVLO",
			__func__);
		break;
	case 0x03:
		pr_info("%s: VBUS is valid. CHGIN < CHGIN_OVLO", __func__);
		break;
	default:
		break;
	}

	return reg_data;
}

static int max77854_get_charger_state(struct max77854_charger_data *charger)
{
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	u8 reg_data;

	max77854_read_reg(charger->i2c,
			  MAX77854_CHG_REG_DETAILS_01, &reg_data);

	pr_info("%s : charger status (0x%02x)\n", __func__, reg_data);

	reg_data &= 0x0f;

	switch (reg_data)
	{
	case 0x00:
	case 0x01:
	case 0x02:
		status = POWER_SUPPLY_STATUS_CHARGING;
		break;
	case 0x03:
	case 0x04:
		status = POWER_SUPPLY_STATUS_FULL;
		break;
	case 0x05:
	case 0x06:
	case 0x07:
		status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	case 0x08:
	case 0xA:
	case 0xB:
		status = POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	default:
		status = POWER_SUPPLY_STATUS_UNKNOWN;
		break;
	}

	return (int)status;
}

static bool max77854_is_constant_current(struct max77854_charger_data *charger)
{
	u8 reg_data;

	max77854_read_reg(charger->i2c,
			  MAX77854_CHG_REG_DETAILS_01, &reg_data);
	pr_info("%s : charger status (0x%02x)\n", __func__, reg_data);
	reg_data &= 0x0f;

	if (reg_data == 0x01)
		return true;
	return false;
}

static void max77854_set_float_voltage(struct max77854_charger_data *charger, int float_voltage)
{
	u8 reg_data = 0;

	reg_data = float_voltage <= 40500 ? 0x0 :
		float_voltage >= 45000 ? 0x24 :
		(float_voltage - 40500) / 125;

	max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_04,
	        (reg_data << CHG_CNFG_04_CHG_CV_PRM_SHIFT),
	        CHG_CNFG_04_CHG_CV_PRM_MASK);

	max77854_read_reg(charger->i2c, MAX77854_CHG_REG_CNFG_04, &reg_data);
	pr_info("%s: battery cv voltage 0x%x\n", __func__, reg_data);
}

static u8 max77854_get_float_voltage(struct max77854_charger_data *charger)
{
	u8 reg_data = 0;
	int float_voltage;

	max77854_read_reg(charger->i2c, MAX77854_CHG_REG_CNFG_04, &reg_data);
	reg_data &= 0x3F;
	float_voltage = reg_data * 125 + 40500;
	pr_debug("%s: battery cv reg : 0x%x, float voltage val : %d\n",
		__func__, reg_data, float_voltage);

	return reg_data;
}
static int max77854_get_charging_health(struct max77854_charger_data *charger)
{
	int state;
	int vbus_state;
	int retry_cnt;
	u8 chg_dtls, reg_data;
	u8 chg_cnfg_00;

	/* watchdog kick */
	max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_06,
			MAX77854_WDTCLR, MAX77854_WDTCLR);

	max77854_read_reg(charger->i2c,
			MAX77854_CHG_REG_DETAILS_01, &reg_data);
	reg_data = ((reg_data & MAX77854_BAT_DTLS) >> MAX77854_BAT_DTLS_SHIFT);

	pr_info("%s: reg_data(0x%x)\n", __func__, reg_data);
	switch (reg_data) {
	case 0x00:
		pr_info("%s: No battery and the charger is suspended\n",
			__func__);
		state = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
		break;
	case 0x01:
		pr_info("%s: battery is okay "
			"but its voltage is low(~VPQLB)\n", __func__);
		state = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case 0x02:
		pr_info("%s: battery dead\n", __func__);
		state = POWER_SUPPLY_HEALTH_DEAD;
		break;
	case 0x03:
		state = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case 0x04:
		pr_info("%s: battery is okay" \
			"but its voltage is low\n", __func__);
		state = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case 0x05:
		pr_info("%s: battery ovp\n", __func__);
		state = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		break;
	default:
		pr_info("%s: battery unknown : 0x%x\n", __func__, reg_data);
		state = POWER_SUPPLY_HEALTH_UNKNOWN;
		break;
	}

	if (state == POWER_SUPPLY_HEALTH_GOOD) {
		union power_supply_propval value;
		psy_do_property("battery", get,
				POWER_SUPPLY_PROP_HEALTH, value);
		/* VBUS OVP state return battery OVP state */
		vbus_state = max77854_get_vbus_state(charger);
		/* read CHG_DTLS and detecting battery terminal error */
		max77854_read_reg(charger->i2c,
				  MAX77854_CHG_REG_DETAILS_01, &chg_dtls);
		chg_dtls = ((chg_dtls & MAX77854_CHG_DTLS) >>
			    MAX77854_CHG_DTLS_SHIFT);
		max77854_read_reg(charger->i2c,
				  MAX77854_CHG_REG_CNFG_00, &chg_cnfg_00);

		/* print the log at the abnormal case */
		if((charger->is_charging == 1) && (chg_dtls & 0x08)) {
			max77854_test_read(charger);
			max77854_set_charger_state(charger, DISABLE);
			max77854_set_float_voltage(charger, charger->float_voltage);
			max77854_set_charger_state(charger, ENABLE);
		}

		pr_info("%s: vbus_state : 0x%x, chg_dtls : 0x%x\n", __func__, vbus_state, chg_dtls);
		/*  OVP is higher priority */
		if (vbus_state == 0x02) { /*  CHGIN_OVLO */
			pr_info("%s: vbus ovp\n", __func__);
			state = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
			if (is_wireless_type(charger->cable_type)) {
				retry_cnt = 0;
				do {
					msleep(50);
					vbus_state = max77854_get_vbus_state(charger);
				} while((retry_cnt++ < 2) && (vbus_state == 0x02));
				if (vbus_state == 0x02) {
					state = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
					pr_info("%s: wpc and over-voltage\n", __func__);
				} else
					state = POWER_SUPPLY_HEALTH_GOOD;
			}
		} else if (((vbus_state == 0x0) || (vbus_state == 0x01)) && (chg_dtls & 0x08) && \
			    (chg_cnfg_00 & MAX77854_MODE_BUCK) &&	\
			    (chg_cnfg_00 & MAX77854_MODE_CHGR) &&	\
			    is_not_wireless_type(charger->cable_type)) {
			pr_info("%s: vbus is under\n", __func__);
			state = POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
		} else if ((value.intval == POWER_SUPPLY_HEALTH_UNDERVOLTAGE) && \
				((vbus_state == 0x0) || (vbus_state == 0x01)) && \
				is_not_wireless_type(charger->cable_type)) {
			pr_info("%s: keep under-voltage\n", __func__);
			state = POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
		}
	}

	return (int)state;
}

static int max77854_get_charge_current(struct max77854_charger_data *charger)
{
	u8 reg_data;
	int get_current = 0;

	max77854_read_reg(charger->i2c,
		MAX77854_CHG_REG_CNFG_02, &reg_data);
	reg_data &= MAX77854_CHG_CC;

	get_current = reg_data <= 0x2 ? 100 : reg_data * 50;

	return get_current;
}

static int max77854_get_input_current_type(struct max77854_charger_data *charger,
	int cable_type)
{
	u8 reg_data;
	int get_current = 0;

	if (cable_type == SEC_BATTERY_CABLE_WIRELESS) {
		max77854_read_reg(charger->i2c,
			MAX77854_CHG_REG_CNFG_10, &reg_data);
		/* AND operation for removing the formal 2bit  */
		reg_data &= 0x3F;

		if (reg_data <= 0x3)
			get_current = 90;
		else
			get_current = reg_data * 30;
	} else {
		max77854_read_reg(charger->i2c,
			MAX77854_CHG_REG_CNFG_09, &reg_data);
		/* AND operation for removing the formal 1bit  */
		reg_data &= 0x7F;

		if (reg_data <= 0x3) {
			get_current = 100;
		} else if (reg_data >= 0x78) {
			get_current = 4000;
		} else {
			int quotient, remainder;
			quotient = reg_data / 3;
			remainder = reg_data % 3;
			if (remainder == 0)
				get_current = quotient * 100;
			else if (remainder == 1)
				get_current = quotient * 100 + 33;
			else
				get_current = quotient * 100 + 67;
		}
	}

	return get_current;
}

static int max77854_get_input_current(struct max77854_charger_data *charger)
{
	if (is_wireless_type(charger->cable_type))
		return max77854_get_input_current_type(charger, SEC_BATTERY_CABLE_WIRELESS);
	else
		return max77854_get_input_current_type(charger, SEC_BATTERY_CABLE_TA);
}

static void reduce_input_current(struct max77854_charger_data *charger, int cur)
{
	u8 set_reg, set_value;
	unsigned int input_curr_limit_step = 0;

	if (is_wireless_type(charger->cable_type)) {
		set_reg = MAX77854_CHG_REG_CNFG_10;
		input_curr_limit_step = 30;
	} else {
		set_reg = MAX77854_CHG_REG_CNFG_09;
		input_curr_limit_step = 33;
	}

	if (!max77854_read_reg(charger->i2c, set_reg, &set_value)) {
		if (set_value <= (MINIMUM_INPUT_CURRENT / input_curr_limit_step) ||
		    set_value <= (cur / input_curr_limit_step))
			return;
		set_value -= (cur / input_curr_limit_step);
		set_value = (set_value < (MINIMUM_INPUT_CURRENT / input_curr_limit_step)) ?
			(MINIMUM_INPUT_CURRENT / input_curr_limit_step) : set_value;
		max77854_write_reg(charger->i2c, set_reg, set_value);
		pr_info("%s: set current: reg:(0x%x), val:(0x%x)\n",
			__func__, set_reg, set_value);
		charger->input_current = max77854_get_input_current(charger);
		charger->aicl_on = true;
	}
}

static bool max77854_check_battery(struct max77854_charger_data *charger)
{
	u8 reg_data;
	u8 reg_data2;

	max77854_read_reg(charger->i2c,
			  MAX77854_CHG_REG_INT_OK, &reg_data);

	pr_info("%s : CHG_INT_OK(0x%x)\n", __func__, reg_data);

	max77854_read_reg(charger->i2c,
			  MAX77854_CHG_REG_DETAILS_00, &reg_data2);

	pr_info("%s : CHG_DETAILS00(0x%x)\n", __func__, reg_data2);

	if ((reg_data & MAX77854_BATP_OK) ||
	    !(reg_data2 & MAX77854_BATP_DTLS))
		return true;
	else
		return false;
}

static void max77854_set_buck(struct max77854_charger_data *charger,
		int enable)
{
	u8 reg_data;

	if (enable) {
		max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_00,
				CHG_CNFG_00_BUCK_MASK, CHG_CNFG_00_BUCK_MASK);
	} else {
		max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_00,
				0, CHG_CNFG_00_BUCK_MASK);
	}
	max77854_read_reg(charger->i2c, MAX77854_CHG_REG_CNFG_00, &reg_data);
	pr_info("%s : CHG_CNFG_00(0x%02x)\n", __func__, reg_data);
}

static void max77854_change_charge_path(struct max77854_charger_data *charger,
		int path)
{
	u8 cnfg12;

	if (is_wireless_type(path)) {
		cnfg12 = (0 << CHG_CNFG_12_CHGINSEL_SHIFT);
	} else {
		cnfg12 = (1 << CHG_CNFG_12_CHGINSEL_SHIFT);
	}

	max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_12,
		cnfg12,	CHG_CNFG_12_CHGINSEL_MASK);
	max77854_read_reg(charger->i2c, MAX77854_CHG_REG_CNFG_12, &cnfg12);
	pr_info("%s : CHG_CNFG_12(0x%02x)\n", __func__, cnfg12);
}

static void max77854_set_input_current(struct max77854_charger_data *charger,
				       int input_current)
{
	int quotient, remainder;
	u8 set_reg, reg_data;

	mutex_lock(&charger->charger_mutex);
	if (is_wireless_type(charger->cable_type)) {
		set_reg = MAX77854_CHG_REG_CNFG_10;
		max77854_read_reg(charger->i2c,
				  set_reg, &reg_data);
		reg_data &= ~MAX77854_CHG_WCIN_LIM;
	} else {
		set_reg = MAX77854_CHG_REG_CNFG_09;
		max77854_read_reg(charger->i2c,
				  set_reg, &reg_data);
		reg_data &= ~MAX77854_CHG_CHGIN_LIM;
	}

	if (!input_current) {
		max77854_write_reg(charger->i2c,
				   set_reg, reg_data);
	} else if (is_wireless_type(charger->cable_type)) {
		input_current = (input_current > 1890) ? 1890 : input_current;
		quotient = input_current / 30;
		reg_data |= quotient;
		max77854_write_reg(charger->i2c, set_reg, reg_data);
	} else {
		input_current = (input_current > 4000) ? 4000 : input_current;
		quotient = input_current / 100;
		remainder = input_current % 100;

		if (remainder >= 67)
			reg_data |= (quotient * 3) + 2;
		else if (remainder >= 33)
			reg_data |= (quotient * 3) + 1;
		else if (remainder < 33)
			reg_data |= quotient * 3;

		max77854_write_reg(charger->i2c,
				   set_reg, reg_data);
	}

	mutex_unlock(&charger->charger_mutex);
	pr_info("[%s] REG(0x%02x) DATA(0x%02x), CURRENT(%d)\n",
		__func__, set_reg, reg_data, input_current);
}

static void max77854_set_charge_current(struct max77854_charger_data *charger,
					int fast_charging_current)
{
	int curr_step = 50;
	u8 reg_data;

	max77854_read_reg(charger->i2c,
			  MAX77854_CHG_REG_CNFG_02, &reg_data);
	reg_data &= ~MAX77854_CHG_CC;

	if (!fast_charging_current) {
		max77854_write_reg(charger->i2c,
				   MAX77854_CHG_REG_CNFG_02, reg_data);
	} else {
		fast_charging_current = (fast_charging_current > 3150) ? 3150 : fast_charging_current;
		reg_data |= (fast_charging_current / curr_step);
		max77854_write_reg(charger->i2c,MAX77854_CHG_REG_CNFG_02, reg_data);
	}

	pr_info("[%s] REG(0x%02x) DATA(0x%02x), CURRENT(%d)\n",
		__func__, MAX77854_CHG_REG_CNFG_02,
		reg_data, fast_charging_current);
}

static void max77854_set_wireless_input_current(struct max77854_charger_data *charger,
				       int input_current)
{
	union power_supply_propval value;

	wake_lock(&charger->wc_current_wake_lock);
	if (is_wireless_type(charger->cable_type)) {
		/* Wcurr-A) In cases of wireless input current change,
			configure the Vrect adj room to 270mV for safe wireless charging. */
		wake_lock(&charger->wc_current_wake_lock);
		value.intval = WIRELESS_VRECT_ADJ_ROOM_1; /* 270mV */
		psy_do_property(charger->pdata->wireless_charger_name, set,
			POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
		msleep(500); /* delay 0.5sec */
		charger->wc_pre_current = max77854_get_input_current(charger);
		charger->wc_current = input_current;
		if (charger->wc_current > charger->wc_pre_current) {
			max77854_set_charge_current(charger, charger->charging_current);
		}
	}
	queue_delayed_work(charger->wqueue, &charger->wc_current_work, 0);
}

static void max77854_set_topoff_current(struct max77854_charger_data *charger,
					int termination_current)
{
	int curr_base = 150, curr_step = 50;
	u8 reg_data;

	if (termination_current < curr_base)
		termination_current = curr_base;
	else if (termination_current > 500)
		termination_current = 500;

	reg_data = (termination_current - curr_base) / curr_step;
	max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_03,
		reg_data, MAX77854_CHG_TO_ITH);

	pr_info("%s: reg_data(0x%02x), topoff(%dmA)\n",
		__func__, reg_data, termination_current);
}

static void max77854_set_charger_state(struct max77854_charger_data *charger,
	int enable)
{
	u8 cnfg_00, cnfg_12;

	if (enable) {
		max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_00,
				CHG_CNFG_00_CHG_MASK, CHG_CNFG_00_CHG_MASK);
		max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_12,
				0x1, CHG_CNFG_12_DISSKIP);
	} else {
		max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_00,
				0, CHG_CNFG_00_CHG_MASK);
		max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_12,
				0x0, CHG_CNFG_12_DISSKIP);
	}
	max77854_read_reg(charger->i2c, MAX77854_CHG_REG_CNFG_00, &cnfg_00);
	max77854_read_reg(charger->i2c, MAX77854_CHG_REG_CNFG_12, &cnfg_12);
	pr_info("%s : CHG_CNFG_00(0x%02x), CHG_CNFG_12(0x%02x)\n", __func__, cnfg_00, cnfg_12);
}

static void max77854_check_slow_charging(struct max77854_charger_data *charger,
	int input_current)
{
	/* under 400mA considered as slow charging concept for VZW */
	if (input_current <= SLOW_CHARGING_CURRENT_STANDARD &&
		charger->cable_type != SEC_BATTERY_CABLE_NONE) {
		union power_supply_propval value;

		charger->slow_charging = true;
		pr_info("%s: slow charging on : input current(%dmA), cable type(%d)\n",
			__func__, input_current, charger->cable_type);

		psy_do_property("battery", set,
			POWER_SUPPLY_PROP_CHARGE_TYPE, value);
	}
	else
		charger->slow_charging = false;
}

static void max77854_charger_initialize(struct max77854_charger_data *charger)
{
	u8 reg_data;
	int jig_gpio;
	pr_info("%s\n", __func__);

	/* unmasked: CHGIN_I, WCIN_I, BATP_I, BYP_I	*/
	/*max77854_write_reg(charger->i2c, MAX77854_CHG_REG_INT_MASK, 0x9a);*/

	/* unlock charger setting protect */
	reg_data = (0x03 << 2);
	max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_06, reg_data, reg_data);

	/*
	 * fast charge timer disable
	 * restart threshold disable
	 * pre-qual charge disable
	 */
	reg_data = (0x03 << 4);
	reg_data |= 0x08;
	max77854_write_reg(charger->i2c, MAX77854_CHG_REG_CNFG_01, reg_data);

	/*
	 * charge current 466mA(default)
	 * otg current limit 900mA
	 */
	max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_02,
			(0x01 << 6), 0xC0);

	/* BAT to SYS OCP 5.00A */
	max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_05, 0x05, 0x07);
	/*
	 * top off current 150mA
	 * top off timer 70min
	 */
	reg_data = 0x38;
	max77854_write_reg(charger->i2c, MAX77854_CHG_REG_CNFG_03, reg_data);

	/*
	 * cv voltage 4.2V or 4.35V
	 * MINVSYS 3.6V(default)
	 */
	max77854_set_float_voltage(charger, charger->pdata->chg_float_voltage);

	/* 
	* VCHGIN & WCHGIN regulation threshold set 4.3v, uvlo 4.5V
	* Auto skip mode
	*/
	max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_12,
		0x0, CHG_CNFG_12_VCHGIN_REG_MASK|CHG_CNFG_12_WCIN_REG_MASK);

	/* Boost mode possible in FACTORY MODE */
	max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_07,
			MAX77854_CHG_FMBST, CHG_CNFG_07_REG_FMBST_MASK);

	jig_gpio = gpio_get_value(charger->jig_gpio);
	max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_07,
			(jig_gpio << CHG_CNFG_07_REG_FGSRC_SHIFT),
			CHG_CNFG_07_REG_FGSRC_MASK);

	/* Watchdog Enable */
	max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_00,
			MAX77854_WDTEN, MAX77854_WDTEN);

	//max77854_test_read(charger);
}

static int max77854_chg_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(max77854_charger_attrs); i++) {
		rc = device_create_file(dev, &max77854_charger_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	return rc;

create_attrs_failed:
	dev_err(dev, "%s: failed (%d)\n", __func__, rc);
	while (i--)
		device_remove_file(dev, &max77854_charger_attrs[i]);
	return rc;
}

ssize_t max77854_chg_show_attrs(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct max77854_charger_data *charger = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - max77854_charger_attrs;
	int i = 0;
	u8 addr, data;

	switch(offset) {
	case CHIP_ID:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n",
			       (charger->pmic_ver >= 0x1 && charger->pmic_ver <= 0x03) ?
			       "MAX77854" : "Unknown");
		break;
	case DATA:
		for (addr = 0xB1; addr <= 0xC3; addr++) {
			max77854_read_reg(charger->i2c, addr, &data);
			i += scnprintf(buf + i, PAGE_SIZE - i, "0x%02x : 0x%02x\n", addr, data);
		}
		break;
	default:
		return -EINVAL;
	}
	return i;
}

ssize_t max77854_chg_store_attrs(struct device *dev,
			    struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct max77854_charger_data *charger = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - max77854_charger_attrs;
	int ret = 0;
	int x,y;

	switch(offset) {
	case CHIP_ID:
		ret = count;
		break;
	case DATA:
		if (sscanf(buf, "0x%x 0x%x", &x, &y) == 2) {
			if (x >= 0xB1 && x <= 0xC3) {
				u8 addr = x;
				u8 data = y;
				if (max77854_write_reg(charger->i2c, addr, data) < 0) {
					dev_info(charger->dev,
							"%s: addr: 0x%x write fail\n", __func__, addr);
				}
			} else {
				dev_info(charger->dev,
						"%s: addr: 0x%x is wrong\n", __func__, x);
			}
		}
		ret = count;
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static int max77854_chg_get_property(struct power_supply *psy,
			      enum power_supply_property psp,
			      union power_supply_propval *val)
{
	struct max77854_charger_data *charger = power_supply_get_drvdata(psy);
	u8 reg_data;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = SEC_BATTERY_CABLE_NONE;
		if (max77854_read_reg(charger->i2c,
			MAX77854_CHG_REG_INT_OK, &reg_data) == 0) {
			if (reg_data & MAX77854_WCIN_OK) {
				val->intval = SEC_BATTERY_CABLE_WIRELESS;
				charger->wc_w_state = 1;
			} else if (reg_data & MAX77854_CHGIN_OK) {
				val->intval = SEC_BATTERY_CABLE_TA;
			}
		}
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = max77854_check_battery(charger);
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = max77854_get_charger_state(charger);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		if (!charger->is_charging)
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
		else if (charger->slow_charging)
		{
			val->intval = POWER_SUPPLY_CHARGE_TYPE_SLOW;
			pr_info("%s: slow-charging mode\n", __func__);
		}
		else
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = max77854_get_charging_health(charger);
		max77854_test_read(charger);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = charger->input_current;
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		if (is_wireless_type(val->intval))
			val->intval = max77854_get_input_current_type(charger, SEC_BATTERY_CABLE_WIRELESS);
		else
			val->intval = max77854_get_input_current_type(charger, SEC_BATTERY_CABLE_TA);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = charger->charging_current;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = max77854_get_float_voltage(charger);
		break;
#if defined(CONFIG_AFC_CHARGER_MODE)
	case POWER_SUPPLY_PROP_AFC_CHARGER_MODE:
		return -ENODATA;
#endif
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		max77854_read_reg(charger->i2c,
				  MAX77854_CHG_REG_DETAILS_01, &reg_data);
		reg_data &= 0x0F;
		switch (reg_data) {
		case 0x01:
			val->strval = "CC Mode";
			break;
		case 0x02:
			val->strval = "CV Mode";
			break;
		case 0x03:
			val->strval = "EOC";
			break;
		case 0x04:
			val->strval = "DONE";
			break;
		default:
			val->strval = "NONE";
			break;
		}
		break;
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
		val->intval = charger->otg_on;
		break;
	case POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL:
		max77854_read_reg(charger->i2c, MAX77854_CHG_REG_CNFG_00,
			&reg_data);
		if ((reg_data & CHG_CNFG_00_UNO_CTRL) == CHG_CNFG_00_BOOST_MASK)
			val->intval = 1;
		else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_USB_HC:
		return -ENODATA;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		if (max77854_is_constant_current(charger)) {
			val->intval = 0;
		} else {
			val->intval = 1;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

#if defined(CONFIG_UPDATE_BATTERY_DATA)
static int max77854_charger_parse_dt(struct max77854_charger_data *charger);
#endif
static int max77854_chg_set_property(struct power_supply *psy,
			  enum power_supply_property psp,
			  const union power_supply_propval *val)
{
	struct max77854_charger_data *charger = power_supply_get_drvdata(psy);
	union power_supply_propval value;
	u8 chg_cnfg_00 = 0;
	static u8 chg_int_state;
	int buck_state = ENABLE;

	switch (psp) {
	/* val->intval : type */
	case POWER_SUPPLY_PROP_STATUS:
		charger->status = val->intval;
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		charger->charge_mode = val->intval;
		switch (charger->charge_mode) {
		case SEC_BAT_CHG_MODE_BUCK_OFF:
			buck_state = DISABLE;
		case SEC_BAT_CHG_MODE_CHARGING_OFF:
			charger->is_charging = false;
			break;
		case SEC_BAT_CHG_MODE_CHARGING:
			charger->is_charging = true;
			break;
		}
		max77854_set_buck(charger, buck_state);
		max77854_set_charger_state(charger, charger->is_charging);
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		charger->cable_type = val->intval;
		charger->aicl_on = false;
		charger->slow_charging = false;
		charger->input_current = max77854_get_input_current(charger);
		max77854_change_charge_path(charger, charger->cable_type);
		if (charger->cable_type == SEC_BATTERY_CABLE_NONE) {
			charger->wc_pre_current = WC_CURRENT_START;
			wake_unlock(&charger->wpc_wake_lock);
			cancel_delayed_work(&charger->wpc_work);
			max77854_update_reg(charger->i2c,
				MAX77854_CHG_REG_INT_MASK, 0, MAX77854_WCIN_IM);
		}
		break;
	/* val->intval : input charging current */
	case POWER_SUPPLY_PROP_CURRENT_MAX:
	{
		int input_current = val->intval;
		if (is_wireless_type(charger->cable_type)) {
			max77854_set_wireless_input_current(charger, input_current);
		} else {
			max77854_set_input_current(charger, input_current);
		}
		if (charger->cable_type == SEC_BATTERY_CABLE_NONE)
			max77854_set_wireless_input_current(charger, input_current);
		charger->input_current = input_current;
	}
		break;
	/*  val->intval : charging current */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		charger->charging_current = val->intval;
		max77854_set_charge_current(charger,
			val->intval);
		break;
	/* val->intval : charging current */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		charger->charging_current = val->intval;
		if (is_not_wireless_type(charger->cable_type)) {
			max77854_set_charge_current(charger, charger->charging_current);
		}
		break;
	case POWER_SUPPLY_PROP_CURRENT_FULL:
		max77854_set_topoff_current(charger, val->intval);
		break;
#if defined(CONFIG_AFC_CHARGER_MODE)
	case POWER_SUPPLY_PROP_AFC_CHARGER_MODE:
#if defined(CONFIG_MUIC_HV)
		max77854_hv_muic_charger_init();
#endif
		break;
#endif
#if defined(CONFIG_BATTERY_SWELLING)
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		charger->float_voltage = val->intval;
		pr_info("%s: float voltage(%d)\n", __func__, val->intval);
		max77854_set_float_voltage(charger, val->intval);
		break;
#endif
	case POWER_SUPPLY_PROP_USB_HC:
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		/* if jig attached, change the power source
		from the VBATFG to the internal VSYS*/
		max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_07,
			(val->intval << CHG_CNFG_07_REG_FGSRC_SHIFT),
			CHG_CNFG_07_REG_FGSRC_MASK);
		break;
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
		pr_info("%s: CHGIN-OTG %s\n", __func__, val->intval > 0 ? "on" : "off");
		if (charger->otg_on == val->intval || lpcharge)
			return 0;
		mutex_lock(&charger->charger_mutex);
		/* CHGIN-OTG */
		if (val->intval) {
			if (is_hv_wireless_type(charger->cable_type)) {
				pr_info("%s: OTG enabled on HV_WC, set 5V", __func__);
				value.intval = WIRELESS_VOUT_5V;
				psy_do_property(charger->pdata->wireless_charger_name, set,
					POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
				mdelay(200);
			}
			max77854_read_reg(charger->i2c, MAX77854_CHG_REG_INT_MASK,
				&chg_int_state);

			/* disable charger interrupt: CHG_I, CHGIN_I */
			/* enable charger interrupt: BYP_I */
			max77854_update_reg(charger->i2c, MAX77854_CHG_REG_INT_MASK,
				MAX77854_CHG_IM | MAX77854_CHGIN_IM,
				MAX77854_CHG_IM | MAX77854_CHGIN_IM | MAX77854_BYP_IM);

			/* OTG on, boost on */
			max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_00,
				CHG_CNFG_00_OTG_CTRL, CHG_CNFG_00_OTG_CTRL);

			/* Update CHG_CNFG_11 to 0x50(5V) */
			max77854_write_reg(charger->i2c,
				MAX77854_CHG_REG_CNFG_11, 0x50);
		} else {
			/* OTG off(UNO on), boost off */
			max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_00,
				0, CHG_CNFG_00_OTG_CTRL);

			/* Update CHG_CNFG_11 to 0x00(3V) */
			max77854_write_reg(charger->i2c,
				MAX77854_CHG_REG_CNFG_11, 0x00);
			mdelay(50);

			/* enable charger interrupt */
			max77854_write_reg(charger->i2c,
				MAX77854_CHG_REG_INT_MASK, chg_int_state);

			if (is_hv_wireless_type(charger->cable_type)) {
				pr_info("%s: OTG disabled on HV_WC, set 9V", __func__);
				value.intval = WIRELESS_VOUT_9V_OTG;
				psy_do_property(charger->pdata->wireless_charger_name, set,
					POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
			}
		}
		charger->otg_on = val->intval;
		max77854_read_reg(charger->i2c, MAX77854_CHG_REG_INT_MASK,
			&chg_int_state);
		max77854_read_reg(charger->i2c, MAX77854_CHG_REG_CNFG_00,
			&chg_cnfg_00);
		mutex_unlock(&charger->charger_mutex);
		pr_info("%s: INT_MASK(0x%x), CHG_CNFG_00(0x%x)\n",
			__func__, chg_int_state, chg_cnfg_00);
		power_supply_changed(charger->psy_otg);
		break;
	case POWER_SUPPLY_PROP_CHARGE_UNO_CONTROL:
		pr_info("%s: WCIN-UNO %s\n", __func__, val->intval > 0 ? "on" : "off");
		/* WCIN-UNO */
		if (val->intval) {
			max77854_read_reg(charger->i2c, MAX77854_CHG_REG_INT_MASK,
				&chg_int_state);

			/* disable charger interrupt: CHG_I, CHGIN_I */
			/* enable charger interrupt: BYP_I */
			max77854_update_reg(charger->i2c, MAX77854_CHG_REG_INT_MASK,
				MAX77854_CHG_IM | MAX77854_CHGIN_IM,
				MAX77854_CHG_IM | MAX77854_CHGIN_IM | MAX77854_BYP_IM);

			/* UNO on, boost on */
			max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_00,
				CHG_CNFG_00_BOOST_MASK, CHG_CNFG_00_UNO_CTRL);

			/* Update CHG_CNFG_11 to 0x50(5V) */
			max77854_write_reg(charger->i2c,
				MAX77854_CHG_REG_CNFG_11, 0x50);
		} else {
			/* boost off*/
			max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_00,
				0, CHG_CNFG_00_BOOST_MASK);

			/* Update CHG_CNFG_11 to 0x00(3V) */
			max77854_write_reg(charger->i2c,
				MAX77854_CHG_REG_CNFG_11, 0x00);
			mdelay(50);

			/* enable charger interrupt */
			max77854_write_reg(charger->i2c,
				MAX77854_CHG_REG_INT_MASK, chg_int_state);
		}
		max77854_read_reg(charger->i2c, MAX77854_CHG_REG_INT_MASK,
			&chg_int_state);
		max77854_read_reg(charger->i2c, MAX77854_CHG_REG_CNFG_00,
			&chg_cnfg_00);
		pr_info("%s: INT_MASK(0x%x), CHG_CNFG_00(0x%x)\n",
			__func__, chg_int_state, chg_cnfg_00);
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
		max77854_enable_aicl_irq(charger);
		queue_delayed_work(charger->wqueue, &charger->aicl_work, msecs_to_jiffies(50));
		break;
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	case POWER_SUPPLY_PROP_POWER_DESIGN:
		max77854_charger_parse_dt(charger);
		break;
#endif
	default:
		return -EINVAL;
	}
	return 0;
}

static int max77854_otg_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct max77854_charger_data *charger = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = charger->otg_on;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int max77854_otg_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct max77854_charger_data *charger = power_supply_get_drvdata(psy);
	union power_supply_propval value;
	static u8 chg_int_state;
	u8 chg_cnfg_00;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		pr_info("%s: CHGIN-OTG %s\n", __func__, val->intval > 0 ? "on" : "off");
		if (charger->otg_on == val->intval || lpcharge)
			return 0;
		mutex_lock(&charger->charger_mutex);
		/* CHGIN-OTG */
		if (val->intval) {
			if (is_hv_wireless_type(charger->cable_type)) {
				pr_info("%s: OTG enabled on HV_WC, set 5V", __func__);
				value.intval = WIRELESS_VOUT_5V;
				psy_do_property(charger->pdata->wireless_charger_name, set,
					POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
				mdelay(200);
			}
			max77854_read_reg(charger->i2c, MAX77854_CHG_REG_INT_MASK,
				&chg_int_state);

			/* disable charger interrupt: CHG_I, CHGIN_I */
			/* enable charger interrupt: BYP_I */
			max77854_update_reg(charger->i2c, MAX77854_CHG_REG_INT_MASK,
				MAX77854_CHG_IM | MAX77854_CHGIN_IM,
				MAX77854_CHG_IM | MAX77854_CHGIN_IM | MAX77854_BYP_IM);

			/* OTG on, boost on */
			max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_00,
				CHG_CNFG_00_OTG_CTRL, CHG_CNFG_00_OTG_CTRL);

			/* Update CHG_CNFG_11 to 0x50(5V) */
			max77854_write_reg(charger->i2c,
				MAX77854_CHG_REG_CNFG_11, 0x50);
		} else {
			/* OTG off(UNO on), boost off */
			max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_00,
				0, CHG_CNFG_00_OTG_CTRL);

			/* Update CHG_CNFG_11 to 0x00(3V) */
			max77854_write_reg(charger->i2c,
				MAX77854_CHG_REG_CNFG_11, 0x00);
			mdelay(50);

			/* enable charger interrupt */
			max77854_write_reg(charger->i2c,
				MAX77854_CHG_REG_INT_MASK, chg_int_state);

			if (is_hv_wireless_type(charger->cable_type)) {
				pr_info("%s: OTG disabled on HV_WC, set 9V", __func__);
				value.intval = WIRELESS_VOUT_9V_OTG;
				psy_do_property(charger->pdata->wireless_charger_name, set,
					POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
			}
		}
		charger->otg_on = val->intval;
		max77854_read_reg(charger->i2c, MAX77854_CHG_REG_INT_MASK,
			&chg_int_state);
		max77854_read_reg(charger->i2c, MAX77854_CHG_REG_CNFG_00,
			&chg_cnfg_00);
		mutex_unlock(&charger->charger_mutex);
		pr_info("%s: INT_MASK(0x%x), CHG_CNFG_00(0x%x)\n",
			__func__, chg_int_state, chg_cnfg_00);
		power_supply_changed(charger->psy_otg);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int max77854_debugfs_show(struct seq_file *s, void *data)
{
	struct max77854_charger_data *charger = s->private;
	u8 reg;
	u8 reg_data;

	seq_printf(s, "MAX77854 CHARGER IC :\n");
	seq_printf(s, "===================\n");
	for (reg = 0xB0; reg <= 0xC3; reg++) {
		max77854_read_reg(charger->i2c, reg, &reg_data);
		seq_printf(s, "0x%02x:\t0x%02x\n", reg, reg_data);
	}

	seq_printf(s, "\n");
	return 0;
}

static int max77854_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, max77854_debugfs_show, inode->i_private);
}

static const struct file_operations max77854_debugfs_fops = {
	.open           = max77854_debugfs_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
};

static void max77854_chg_isr_work(struct work_struct *work)
{
	struct max77854_charger_data *charger =
		container_of(work, struct max77854_charger_data, isr_work.work);

	union power_supply_propval val;

	if (charger->pdata->full_check_type ==
	    SEC_BATTERY_FULLCHARGED_CHGINT) {

		val.intval = max77854_get_charger_state(charger);

		switch (val.intval) {
		case POWER_SUPPLY_STATUS_DISCHARGING:
			pr_err("%s: Interrupted but Discharging\n", __func__);
			break;

		case POWER_SUPPLY_STATUS_NOT_CHARGING:
			pr_err("%s: Interrupted but NOT Charging\n", __func__);
			break;

		case POWER_SUPPLY_STATUS_FULL:
			pr_info("%s: Interrupted by Full\n", __func__);
			psy_do_property("battery", set,
				POWER_SUPPLY_PROP_STATUS, val);
			break;

		case POWER_SUPPLY_STATUS_CHARGING:
			pr_err("%s: Interrupted but Charging\n", __func__);
			break;

		case POWER_SUPPLY_STATUS_UNKNOWN:
		default:
			pr_err("%s: Invalid Charger Status\n", __func__);
			break;
		}
	}

	if (charger->pdata->ovp_uvlo_check_type ==
		SEC_BATTERY_OVP_UVLO_CHGINT) {
		val.intval = max77854_get_charging_health(charger);
		switch (val.intval) {
		case POWER_SUPPLY_HEALTH_OVERHEAT:
		case POWER_SUPPLY_HEALTH_COLD:
			pr_err("%s: Interrupted but Hot/Cold\n", __func__);
			break;

		case POWER_SUPPLY_HEALTH_DEAD:
			pr_err("%s: Interrupted but Dead\n", __func__);
			break;

		case POWER_SUPPLY_HEALTH_OVERVOLTAGE:
		case POWER_SUPPLY_HEALTH_UNDERVOLTAGE:
			pr_info("%s: Interrupted by OVP/UVLO\n", __func__);
			psy_do_property("battery", set,
				POWER_SUPPLY_PROP_HEALTH, val);
			break;

		case POWER_SUPPLY_HEALTH_UNSPEC_FAILURE:
			pr_err("%s: Interrupted but Unspec\n", __func__);
			break;

		case POWER_SUPPLY_HEALTH_GOOD:
			pr_err("%s: Interrupted but Good\n", __func__);
			break;

		case POWER_SUPPLY_HEALTH_UNKNOWN:
		default:
			pr_err("%s: Invalid Charger Health\n", __func__);
			break;
		}
	}
}

static irqreturn_t max77854_chg_irq_thread(int irq, void *irq_data)
{
	struct max77854_charger_data *charger = irq_data;

	pr_info("%s: Charger interrupt occured\n", __func__);

	if ((charger->pdata->full_check_type ==
	     SEC_BATTERY_FULLCHARGED_CHGINT) ||
	    (charger->pdata->ovp_uvlo_check_type ==
	     SEC_BATTERY_OVP_UVLO_CHGINT))
		schedule_delayed_work(&charger->isr_work, 0);

	return IRQ_HANDLED;
}

/* If we have external wireless TRx, enable WCIN interrupt to detect Mis-align only */
static void wpc_detect_work(struct work_struct *work)
{
	struct max77854_charger_data *charger = container_of(work,
						struct max77854_charger_data,
						wpc_work.work);

	max77854_update_reg(charger->i2c,
		MAX77854_CHG_REG_INT_MASK, 0, MAX77854_WCIN_IM);

	if (is_wireless_type(charger->cable_type)) {
		u8 reg_data, wcin_state, wcin_dtls, wcin_cnt = 0;
		do {
			wcin_cnt++;
			max77854_read_reg(charger->i2c, MAX77854_CHG_REG_INT_OK,
				&reg_data);
			wcin_state = (reg_data & MAX77854_WCIN_OK) >> MAX77854_WCIN_OK_SHIFT;
			max77854_read_reg(charger->i2c, MAX77854_CHG_REG_DETAILS_00,
				&reg_data);
			wcin_dtls = (reg_data & MAX77854_WCIN_DTLS) >> MAX77854_WCIN_DTLS_SHIFT;
			if (!wcin_state && !wcin_dtls && wcin_cnt >= 2) {
				union power_supply_propval value;
				pr_info("%s: invalid WCIN, Misalign occurs!\n", __func__);
				value.intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
				psy_do_property(charger->pdata->wireless_charger_name, set,
					POWER_SUPPLY_PROP_STATUS, value);
			}
			mdelay(50);
		} while (!wcin_state && !wcin_dtls && wcin_cnt < 2);
	} 

	/* Do unmask again. (for frequent wcin irq problem) */
	max77854_update_reg(charger->i2c,
		MAX77854_CHG_REG_INT_MASK, 0, MAX77854_WCIN_IM);

	wake_unlock(&charger->wpc_wake_lock);
}

static irqreturn_t wpc_charger_irq(int irq, void *data)
{
	struct max77854_charger_data *charger = data;
	
	pr_info("%s: irq(%d)\n", __func__, irq);

	max77854_update_reg(charger->i2c, MAX77854_CHG_REG_INT_MASK,
		MAX77854_WCIN_IM, MAX77854_WCIN_IM);
	wake_lock(&charger->wpc_wake_lock);
	queue_delayed_work(charger->wqueue, &charger->wpc_work, msecs_to_jiffies(10000));

	return IRQ_HANDLED;
}

static irqreturn_t max77854_batp_irq(int irq, void *data)
{
	struct max77854_charger_data *charger = data;
	union power_supply_propval value;
	u8 reg_data;

	pr_info("%s : irq(%d)\n", __func__, irq);

	max77854_read_reg(charger->i2c,
			  MAX77854_CHG_REG_INT_MASK, &reg_data);
	reg_data |= (1 << 2);
	max77854_write_reg(charger->i2c,
		MAX77854_CHG_REG_INT_MASK, reg_data);

	check_charger_unlock_state(charger);

	max77854_read_reg(charger->i2c,
			  MAX77854_CHG_REG_INT_OK,
			  &reg_data);

	if (!(reg_data & MAX77854_BATP_OK))
		psy_do_property("battery", set, POWER_SUPPLY_PROP_PRESENT, value);

	max77854_read_reg(charger->i2c,
			  MAX77854_CHG_REG_INT_MASK, &reg_data);
	reg_data &= ~(1 << 2);
	max77854_write_reg(charger->i2c,
			   MAX77854_CHG_REG_INT_MASK, reg_data);

	return IRQ_HANDLED;
}


static irqreturn_t max77854_bypass_irq(int irq, void *data)
{
	struct max77854_charger_data *charger = data;
	u8 dtls_02;
	u8 byp_dtls;
	u8 vbus_state;
#ifdef CONFIG_USB_HOST_NOTIFY
	struct otg_notify *o_notify;

	o_notify = get_otg_notify();
#endif

	pr_info("%s: irq(%d)\n", __func__, irq);

	/* check and unlock */
	check_charger_unlock_state(charger);

	max77854_read_reg(charger->i2c,
			  MAX77854_CHG_REG_DETAILS_02,
			  &dtls_02);

	byp_dtls = ((dtls_02 & MAX77854_BYP_DTLS) >>
				MAX77854_BYP_DTLS_SHIFT);
	pr_info("%s: BYP_DTLS(0x%02x)\n", __func__, byp_dtls);
	vbus_state = max77854_get_vbus_state(charger);

	if (byp_dtls & 0x1) {
	        union power_supply_propval val;
		pr_info("%s: bypass overcurrent limit\n", __func__);
#ifdef CONFIG_USB_HOST_NOTIFY
		send_otg_notify(o_notify, NOTIFY_EVENT_OVERCURRENT, 0);
#endif
		/* disable the register values just related to OTG and
		   keep the values about the charging */
                val.intval = 0;
                psy_do_property("otg", set,
                        POWER_SUPPLY_PROP_ONLINE, val);
	}
	return IRQ_HANDLED;
}

static void max77854_aicl_isr_work(struct work_struct *work)
{
	struct max77854_charger_data *charger = container_of(work,
				struct max77854_charger_data, aicl_work.work);
	u8 aicl_state, aicl_cnt = 0;

	pr_info("%s: \n", __func__);

	wake_lock(&charger->aicl_wake_lock);
	mutex_lock(&charger->charger_mutex);
	max77854_update_reg(charger->i2c,
		MAX77854_CHG_REG_INT_MASK, MAX77854_AICL_IM, MAX77854_AICL_IM);
	/* check and unlock */
	check_charger_unlock_state(charger);
	max77854_read_reg(charger->i2c, MAX77854_CHG_REG_INT_OK, &aicl_state);
	while (!(aicl_state & 0x80) && charger->cable_type != SEC_BATTERY_CABLE_NONE) {
		if (++aicl_cnt >= 2) {
			reduce_input_current(charger, REDUCE_CURRENT_STEP);
			aicl_cnt = 0;
		}
		mdelay(50);
		max77854_read_reg(charger->i2c, MAX77854_CHG_REG_INT_OK, &aicl_state);
		if (max77854_get_input_current(charger) <= MINIMUM_INPUT_CURRENT)
			break;
	}

	if (charger->aicl_on) {
		union power_supply_propval value;
		value.intval = max77854_get_input_current(charger);
		psy_do_property("battery", set,
			POWER_SUPPLY_EXT_PROP_AICL_CURRENT, value);
	}

	if (is_not_wireless_type(charger->cable_type))
		max77854_check_slow_charging(charger, charger->input_current);

	max77854_update_reg(charger->i2c,
		MAX77854_CHG_REG_INT_MASK, 0, MAX77854_AICL_IM);
	mutex_unlock(&charger->charger_mutex);
	wake_unlock(&charger->aicl_wake_lock);
}

static irqreturn_t max77854_aicl_irq(int irq, void *data)
{
	struct max77854_charger_data *charger = data;
	queue_delayed_work(charger->wqueue, &charger->aicl_work, msecs_to_jiffies(50));

	pr_info("%s: irq(%d)\n", __func__, irq);
	wake_unlock(&charger->wc_current_wake_lock);
	cancel_delayed_work(&charger->wc_current_work);

	return IRQ_HANDLED;
}

static void max77854_enable_aicl_irq(struct max77854_charger_data *charger)
{
	int ret;

	pr_info("%s \n", __func__);
	ret = request_threaded_irq(charger->irq_aicl, NULL,
			max77854_aicl_irq, 0, "aicl-irq", charger);
	if (ret < 0) {
		pr_err("%s: fail to request aicl IRQ: %d: %d\n",
				__func__, charger->irq_aicl, ret);
	} else {
		max77854_update_reg(charger->i2c,
			MAX77854_CHG_REG_INT_MASK, 0, MAX77854_AICL_IM);
	}
}

static void max77854_chgin_isr_work(struct work_struct *work)
{
	struct max77854_charger_data *charger = container_of(work,
				     struct max77854_charger_data, chgin_work);
	u8 chgin_dtls, chg_dtls, chg_cnfg_00, reg_data;
	u8 prev_chgin_dtls = 0xff;
	int battery_health;
	union power_supply_propval value;
	int stable_count = 0;

	wake_lock(&charger->chgin_wake_lock);

	max77854_read_reg(charger->i2c,
			  MAX77854_CHG_REG_INT_MASK, &reg_data);
	reg_data |= (1 << 6);
	max77854_write_reg(charger->i2c,
		MAX77854_CHG_REG_INT_MASK, reg_data);

	while (1) {
		psy_do_property("battery", get,
				POWER_SUPPLY_PROP_HEALTH, value);
		battery_health = value.intval;

		max77854_read_reg(charger->i2c,
				MAX77854_CHG_REG_DETAILS_00,
				&chgin_dtls);
		chgin_dtls = ((chgin_dtls & MAX77854_CHGIN_DTLS) >>
				MAX77854_CHGIN_DTLS_SHIFT);
		max77854_read_reg(charger->i2c,
				MAX77854_CHG_REG_DETAILS_01, &chg_dtls);
		chg_dtls = ((chg_dtls & MAX77854_CHG_DTLS) >>
				MAX77854_CHG_DTLS_SHIFT);
		max77854_read_reg(charger->i2c,
			MAX77854_CHG_REG_CNFG_00, &chg_cnfg_00);

		if (prev_chgin_dtls == chgin_dtls)
			stable_count++;
		else
			stable_count = 0;
		if (stable_count > 10) {
			pr_info("%s: irq(%d), chgin(0x%x), chg_dtls(0x%x) prev 0x%x\n",
					__func__, charger->irq_chgin,
					chgin_dtls, chg_dtls, prev_chgin_dtls);
			if (charger->is_charging) {
				if ((chgin_dtls == 0x02) && \
					(battery_health != POWER_SUPPLY_HEALTH_OVERVOLTAGE)) {
					pr_info("%s: charger is over voltage\n",
							__func__);
					value.intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
					psy_do_property("battery", set,
						POWER_SUPPLY_PROP_HEALTH, value);
				} else if (((chgin_dtls == 0x0) || (chgin_dtls == 0x01)) &&(chg_dtls & 0x08) && \
						(chg_cnfg_00 & MAX77854_MODE_BUCK) && \
						(chg_cnfg_00 & MAX77854_MODE_CHGR) && \
						(battery_health != POWER_SUPPLY_HEALTH_UNDERVOLTAGE) && \
						is_not_wireless_type(charger->cable_type)) {
					pr_info("%s, vbus_state : 0x%d, chg_state : 0x%d\n", __func__, chgin_dtls, chg_dtls);
					pr_info("%s: vBus is undervoltage\n", __func__);
					value.intval = POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
					psy_do_property("battery", set,
							POWER_SUPPLY_PROP_HEALTH, value);
				}
			} else {
				if ((battery_health == \
							POWER_SUPPLY_HEALTH_OVERVOLTAGE) &&
						(chgin_dtls != 0x02)) {
					pr_info("%s: vbus_state : 0x%d, chg_state : 0x%d\n", __func__, chgin_dtls, chg_dtls);
					pr_info("%s: overvoltage->normal\n", __func__);
					value.intval = POWER_SUPPLY_HEALTH_GOOD;
					psy_do_property("battery", set,
							POWER_SUPPLY_PROP_HEALTH, value);
				} else if ((battery_health == \
							POWER_SUPPLY_HEALTH_UNDERVOLTAGE) &&
						!((chgin_dtls == 0x0) || (chgin_dtls == 0x01))){
					pr_info("%s: vbus_state : 0x%d, chg_state : 0x%d\n", __func__, chgin_dtls, chg_dtls);
					pr_info("%s: undervoltage->normal\n", __func__);
					value.intval = POWER_SUPPLY_HEALTH_GOOD;
					psy_do_property("battery", set,
							POWER_SUPPLY_PROP_HEALTH, value);
					max77854_set_input_current(charger,
							charger->input_current);
				}
			}
			break;
		}

		prev_chgin_dtls = chgin_dtls;
		msleep(100);
	}
	max77854_read_reg(charger->i2c,
		MAX77854_CHG_REG_INT_MASK, &reg_data);
	reg_data &= ~(1 << 6);
	max77854_write_reg(charger->i2c,
		MAX77854_CHG_REG_INT_MASK, reg_data);
	wake_unlock(&charger->chgin_wake_lock);
}

static irqreturn_t max77854_chgin_irq(int irq, void *data)
{
	struct max77854_charger_data *charger = data;
	queue_work(charger->wqueue, &charger->chgin_work);

	return IRQ_HANDLED;
}

/* register chgin isr after sec_battery_probe */
static void max77854_chgin_init_work(struct work_struct *work)
{
	struct max77854_charger_data *charger = container_of(work,
						struct max77854_charger_data,
						chgin_init_work.work);
	int ret;

	pr_info("%s \n", __func__);
	ret = request_threaded_irq(charger->irq_chgin, NULL,
			max77854_chgin_irq, 0, "chgin-irq", charger);
	if (ret < 0) {
		pr_err("%s: fail to request chgin IRQ: %d: %d\n",
				__func__, charger->irq_chgin, ret);
	} else {
		max77854_update_reg(charger->i2c,
			MAX77854_CHG_REG_INT_MASK, 0, MAX77854_CHGIN_IM);
	}
}

static void max77854_wc_current_work(struct work_struct *work)
{
	struct max77854_charger_data *charger = container_of(work,
					struct max77854_charger_data,
					wc_current_work.work);
	int diff_current = 0;

	if (is_not_wireless_type(charger->cable_type)) {
		charger->wc_pre_current = WC_CURRENT_START;
		max77854_write_reg(charger->i2c,
			MAX77854_CHG_REG_CNFG_10, 0x10);
		wake_unlock(&charger->wc_current_wake_lock);
		return;
	}

	if (charger->wc_pre_current == charger->wc_current) {
		union power_supply_propval value;
		max77854_set_charge_current(charger, charger->charging_current);
		/* Wcurr-B) Restore Vrect adj room to previous value
			after finishing wireless input current setting. Refer to Wcurr-A) step */
		msleep(500);
		if (is_nv_wireless_type(charger->cable_type)) {
			psy_do_property("battery", get, POWER_SUPPLY_PROP_CAPACITY, value);
			if (value.intval < charger->pdata->wireless_cc_cv)
				value.intval = WIRELESS_VRECT_ADJ_ROOM_4; /* WPC 4.5W, Vrect Room 30mV */
			else 
				value.intval = WIRELESS_VRECT_ADJ_ROOM_5; /* WPC 4.5W, Vrect Room 80mV */
		} else if (is_hv_wireless_type(charger->cable_type)) {
			value.intval = WIRELESS_VRECT_ADJ_ROOM_5; /* WPC 9W, Vrect Room 80mV */
		} else
			value.intval = WIRELESS_VRECT_ADJ_OFF; /* PMA 4.5W, Vrect Room 0mV */
		psy_do_property(charger->pdata->wireless_charger_name, set,
			POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION, value);
		wake_unlock(&charger->wc_current_wake_lock);
	} else {
		if (charger->wc_pre_current > charger->wc_current) {
			diff_current = charger->wc_pre_current - charger->wc_current;
			if (diff_current < WC_CURRENT_STEP)
				charger->wc_pre_current -= diff_current;
			else
				charger->wc_pre_current -= WC_CURRENT_STEP;
		} else {
			diff_current = charger->wc_current - charger->wc_pre_current;
			if (diff_current < WC_CURRENT_STEP)
				charger->wc_pre_current += diff_current;
			else
				charger->wc_pre_current += WC_CURRENT_STEP;
		}
		max77854_set_input_current(charger, charger->wc_pre_current);
		queue_delayed_work(charger->wqueue, &charger->wc_current_work,
				msecs_to_jiffies(WC_CURRENT_WORK_STEP));
	}
	pr_info("%s: wc_current(%d), wc_pre_current(%d), diff(%d)\n",
		__func__, charger->wc_current, charger->wc_pre_current, diff_current);
}

#ifdef CONFIG_OF
static int max77854_charger_parse_dt(struct max77854_charger_data *charger)
{
	struct device_node *np;
	sec_charger_platform_data_t *pdata = charger->pdata;
	int ret = 0;

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s np NULL\n", __func__);
	} else {
		ret = of_property_read_u32(np, "battery,chg_float_voltage",
					   &pdata->chg_float_voltage);
		if (ret) {
			pr_info("%s: battery,chg_float_voltage is Empty\n", __func__);
			pdata->chg_float_voltage = 42000;
		}
		pr_info("%s: battery,chg_float_voltage is %d\n", __func__, pdata->chg_float_voltage);
		charger->float_voltage = pdata->chg_float_voltage;

		ret = of_property_read_string(np,
			"battery,wireless_charger_name", (char const **)&pdata->wireless_charger_name);
		if (ret)
			pr_info("%s: Wireless charger name is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,full_check_type_2nd",
					&pdata->full_check_type_2nd);
		if (ret)
			pr_info("%s : Full check type 2nd is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,wireless_cc_cv", &pdata->wireless_cc_cv);
		if (ret)
			pr_info("%s : wireless_cc_cv is Empty\n", __func__);
	}

	np = of_find_node_by_name(NULL, "max77854-fuelgauge");
	if (!np) {
		pr_err("%s: np NULL\n", __func__);
	} else {
		charger->jig_gpio = of_get_named_gpio(np, "fuelgauge,jig_gpio", 0);
		if (charger->jig_gpio < 0) {
			pr_err("%s error reading jig_gpio = %d\n",
					__func__, charger->jig_gpio);
			charger->jig_gpio = 0;
		}
	}

	return ret;
}
#endif

static const struct power_supply_desc max77854_charger_power_supply_desc = {
	.name = "max77854-charger",
	.type = POWER_SUPPLY_TYPE_UNKNOWN,
	.properties = max77854_charger_props,
	.num_properties = ARRAY_SIZE(max77854_charger_props),
	.get_property = max77854_chg_get_property,
	.set_property = max77854_chg_set_property,
};

static const struct power_supply_desc otg_power_supply_desc = {
	.name = "otg",
	.type = POWER_SUPPLY_TYPE_OTG,
	.properties = max77854_otg_props,
	.num_properties = ARRAY_SIZE(max77854_otg_props),
	.get_property = max77854_otg_get_property,
	.set_property = max77854_otg_set_property,
};

static int max77854_charger_probe(struct platform_device *pdev)
{
	struct max77854_dev *max77854 = dev_get_drvdata(pdev->dev.parent);
	struct max77854_platform_data *pdata = dev_get_platdata(max77854->dev);
	sec_charger_platform_data_t *charger_data;
	struct max77854_charger_data *charger;
	struct power_supply_config charger_cfg = {};
	int ret = 0;
	u8 reg_data;

	pr_info("%s: Max77854 Charger Driver Loading\n", __func__);

	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (!charger)
		return -ENOMEM;

	charger_data = kzalloc(sizeof(sec_charger_platform_data_t), GFP_KERNEL);
	if (!charger_data) {
		ret = -ENOMEM;
		goto err_free;
	}

	mutex_init(&charger->charger_mutex);

	charger->dev = &pdev->dev;
	charger->i2c = max77854->charger;
	charger->pmic_i2c = max77854->i2c;
	charger->pdata = charger_data;
	charger->aicl_on = false;
	charger->slow_charging = false;
	charger->is_mdock = false;
	charger->otg_on = false;
	charger->max77854_pdata = pdata;
	charger->wc_pre_current = WC_CURRENT_START;

#if defined(CONFIG_OF)
	ret = max77854_charger_parse_dt(charger);
	if (ret < 0) {
		pr_err("%s not found charger dt! ret[%d]\n",
		       __func__, ret);
	}
#endif
	platform_set_drvdata(pdev, charger);

	max77854_charger_initialize(charger);

	max77854_read_reg(charger->i2c, MAX77854_CHG_REG_INT_OK, &reg_data);
	if (reg_data & MAX77854_WCIN_OK)
		charger->cable_type = SEC_BATTERY_CABLE_WIRELESS;
	charger->input_current = max77854_get_input_current(charger);
	charger->charging_current = max77854_get_charge_current(charger);

	if (max77854_read_reg(max77854->i2c, MAX77854_PMIC_REG_PMICREV, &reg_data) < 0) {
		pr_err("device not found on this channel (this is not an error)\n");
		ret = -ENOMEM;
		goto err_pdata_free;
	} else {
		charger->pmic_ver = (reg_data & 0x7);
		pr_info("%s : device found : ver.0x%x\n", __func__, charger->pmic_ver);
	}

	(void) debugfs_create_file("max77854-regs",
		S_IRUGO, NULL, (void *)charger, &max77854_debugfs_fops);

	charger->wqueue =
	    create_singlethread_workqueue(dev_name(&pdev->dev));
	if (!charger->wqueue) {
		pr_err("%s: Fail to Create Workqueue\n", __func__);
		goto err_pdata_free;
	}
	wake_lock_init(&charger->chgin_wake_lock, WAKE_LOCK_SUSPEND,
		       "charger->chgin");
	INIT_WORK(&charger->chgin_work, max77854_chgin_isr_work);
	wake_lock_init(&charger->aicl_wake_lock, WAKE_LOCK_SUSPEND,
		"charger-aicl");
	INIT_DELAYED_WORK(&charger->aicl_work, max77854_aicl_isr_work);
	INIT_DELAYED_WORK(&charger->chgin_init_work, max77854_chgin_init_work);
	wake_lock_init(&charger->wpc_wake_lock, WAKE_LOCK_SUSPEND,
					       "charger-wpc");
	INIT_DELAYED_WORK(&charger->wpc_work, wpc_detect_work);
	wake_lock_init(&charger->wc_current_wake_lock, WAKE_LOCK_SUSPEND,
		       "charger->wc-current");
	INIT_DELAYED_WORK(&charger->wc_current_work, max77854_wc_current_work);

	charger_cfg.drv_data = charger;

	charger->psy_chg = power_supply_register(&pdev->dev, &max77854_charger_power_supply_desc, &charger_cfg);
	if (!charger->psy_chg) {
		pr_err("%s: Failed to Register psy_chg\n", __func__);
		goto err_power_supply_register;
	}

	charger->psy_otg = power_supply_register(&pdev->dev, &otg_power_supply_desc, &charger_cfg);
	if (!charger->psy_otg) {
		pr_err("%s: Failed to Register otg_chg\n", __func__);
		goto err_power_supply_register_otg;
	}

	if (charger->pdata->chg_irq) {
		INIT_DELAYED_WORK(&charger->isr_work, max77854_chg_isr_work);

		ret = request_threaded_irq(charger->pdata->chg_irq,
				NULL, max77854_chg_irq_thread,
				charger->pdata->chg_irq_attr,
				"charger-irq", charger);
		if (ret) {
			pr_err("%s: Failed to Request IRQ\n", __func__);
			goto err_irq;
		}

			ret = enable_irq_wake(charger->pdata->chg_irq);
			if (ret < 0)
				pr_err("%s: Failed to Enable Wakeup Source(%d)\n",
					__func__, ret);
		}

	charger->wc_w_irq = pdata->irq_base + MAX77854_CHG_IRQ_WCIN_I;
	ret = request_threaded_irq(charger->wc_w_irq,
				   NULL, wpc_charger_irq,
				   IRQF_TRIGGER_FALLING,
				   "wpc-int", charger);
	if (ret) {
		pr_err("%s: Failed to Request IRQ\n", __func__);
		goto err_wc_irq;
	}

	max77854_read_reg(charger->i2c,
			  MAX77854_CHG_REG_INT_OK, &reg_data);
	charger->wc_w_state = (reg_data & MAX77854_WCIN_OK)
		>> MAX77854_WCIN_OK_SHIFT;

	charger->irq_chgin = pdata->irq_base + MAX77854_CHG_IRQ_CHGIN_I;
	/* enable chgin irq after sec_battery_probe */
	queue_delayed_work(charger->wqueue, &charger->chgin_init_work,
			msecs_to_jiffies(3000));

	charger->irq_bypass = pdata->irq_base + MAX77854_CHG_IRQ_BYP_I;
	ret = request_threaded_irq(charger->irq_bypass, NULL,
			max77854_bypass_irq, 0, "bypass-irq", charger);
	if (ret < 0) {
		pr_err("%s: fail to request bypass IRQ: %d: %d\n",
				__func__, charger->irq_bypass, ret);
	} else {
		max77854_update_reg(charger->i2c,
			MAX77854_CHG_REG_INT_MASK, 0, MAX77854_BYP_IM);
	}

	charger->irq_aicl = pdata->irq_base + MAX77854_CHG_IRQ_AICL_I;
	charger->irq_batp = pdata->irq_base + MAX77854_CHG_IRQ_BATP_I;
	ret = request_threaded_irq(charger->irq_batp, NULL,
				   max77854_batp_irq, 0,
				   "batp-irq", charger);
	if (ret < 0)
		pr_err("%s: fail to request bypass IRQ: %d: %d\n",
		       __func__, charger->irq_batp, ret);

	ret = max77854_chg_create_attrs(&charger->psy_chg->dev);
	if (ret) {
		dev_err(charger->dev,
			"%s : Failed to create_attrs\n", __func__);
		goto err_wc_irq;
	}

	/* watchdog kick */
	max77854_update_reg(charger->i2c, MAX77854_CHG_REG_CNFG_06,
			MAX77854_WDTCLR, MAX77854_WDTCLR);

	pr_info("%s: MAX77854 Charger Driver Loaded\n", __func__);

	return 0;

err_wc_irq:
	free_irq(charger->pdata->chg_irq, NULL);
err_irq:
	power_supply_unregister(charger->psy_otg);
err_power_supply_register_otg:
	power_supply_unregister(charger->psy_chg);
err_power_supply_register:
	destroy_workqueue(charger->wqueue);
err_pdata_free:
	kfree(charger_data);
err_free:
	kfree(charger);

	return ret;
}

static int max77854_charger_remove(struct platform_device *pdev)
{
	struct max77854_charger_data *charger =
		platform_get_drvdata(pdev);

	destroy_workqueue(charger->wqueue);

	free_irq(charger->wc_w_irq, NULL);
	free_irq(charger->pdata->chg_irq, NULL);
	power_supply_unregister(charger->psy_chg);
	power_supply_unregister(charger->psy_otg);
	kfree(charger);

	return 0;
}

#if defined CONFIG_PM
static int max77854_charger_suspend(struct device *dev)
{
	return 0;
}

static int max77854_charger_resume(struct device *dev)
{
	return 0;
}
#else
#define max77854_charger_suspend NULL
#define max77854_charger_resume NULL
#endif

static void max77854_charger_shutdown(struct device *dev)
{
	struct max77854_charger_data *charger =
				dev_get_drvdata(dev);
	u8 reg_data;

	pr_info("%s: MAX77854 Charger driver shutdown\n", __func__);
	if (!charger->i2c) {
		pr_err("%s: no max77854 i2c client\n", __func__);
		return;
	}
	reg_data = 0x04;
	max77854_write_reg(charger->i2c,
		MAX77854_CHG_REG_CNFG_00, reg_data);
	reg_data = 0x0F;
	max77854_write_reg(charger->i2c,
		MAX77854_CHG_REG_CNFG_09, reg_data);
	reg_data = 0x10;
	max77854_write_reg(charger->i2c,
		MAX77854_CHG_REG_CNFG_10, reg_data);
	reg_data = 0x60;
	max77854_write_reg(charger->i2c,
		MAX77854_CHG_REG_CNFG_12, reg_data);
	pr_info("func:%s \n", __func__);
}

static SIMPLE_DEV_PM_OPS(max77854_charger_pm_ops, max77854_charger_suspend,
			 max77854_charger_resume);

static struct platform_driver max77854_charger_driver = {
	.driver = {
		.name = "max77854-charger",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &max77854_charger_pm_ops,
#endif
		.shutdown = max77854_charger_shutdown,
	},
	.probe = max77854_charger_probe,
	.remove = max77854_charger_remove,
};

static int __init max77854_charger_init(void)
{
	pr_info("%s : \n", __func__);
	return platform_driver_register(&max77854_charger_driver);
}

static void __exit max77854_charger_exit(void)
{
	platform_driver_unregister(&max77854_charger_driver);
}

module_init(max77854_charger_init);
module_exit(max77854_charger_exit);

MODULE_DESCRIPTION("Samsung MAX77854 Charger Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
