/*
 * Driver for the NXP PCA9468 battery charger.
 *
 * Copyright (C) 2018 NXP Semiconductor.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/pm_runtime.h>
#include <linux/power_supply.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/rtc.h>
#include <linux/debugfs.h>
#if defined(CONFIG_BATTERY_SAMSUNG)
#include <linux/of_gpio.h>
#include "include/charger/pca9468_charger.h"
#include "include/sec_charging_common.h"
#include "include/sec_direct_charger.h"
#else
#include <linux/power/pca9468_charger.h>
#endif

#if defined (CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif /* CONFIG_OF */

#include <linux/pm_wakeup.h>
#ifdef CONFIG_USBPD_PHY_QCOM
#include <linux/usb/usbpd.h>		// Use Qualcomm USBPD PHY
#endif

#ifdef CONFIG_USBPD_PHY_QCOM
static int pca9468_usbpd_setup(struct pca9468_charger *pca9468);
#endif

#if defined(CONFIG_BATTERY_SAMSUNG)
static int pca9468_send_pd_message(struct pca9468_charger *pca9468, unsigned int msg_type);
static int get_system_current(struct pca9468_charger *pca9468);
#endif

/*******************************/
/* Switching charger control function */
/*******************************/
#if defined(CONFIG_BATTERY_SAMSUNG)
char *charging_state_str[] = {
	"NO_CHARGING", "CHECK_VBAT", "PRESET_DC", "CHECK_ACTIVE", "ADJUST_CC",
	"START_CC", "CC_MODE", "START_CV", "CV_MODE", "CHARGING_DONE",
	"ADJUST_TAVOL", "ADJUST_TACUR"
};

static int pca9468_read_reg(struct pca9468_charger *pca9468, unsigned reg, void *val)
{
	int ret = 0;

	mutex_lock(&pca9468->i2c_lock);
	ret = regmap_read(pca9468->regmap, reg, val);
	mutex_unlock(&pca9468->i2c_lock);
	if (ret < 0)
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
	return ret;
}

static int pca9468_bulk_read_reg(struct pca9468_charger *pca9468, int reg, void *val, int count)
{
	int ret = 0;

	mutex_lock(&pca9468->i2c_lock);
	ret = regmap_bulk_read(pca9468->regmap, reg, val, count);
	mutex_unlock(&pca9468->i2c_lock);
	if (ret < 0)
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
	return ret;
}

static int pca9468_write_reg(struct pca9468_charger *pca9468, int reg, u8 val)
{
	int ret = 0;

	mutex_lock(&pca9468->i2c_lock);
	ret = regmap_write(pca9468->regmap, reg, val);
	mutex_unlock(&pca9468->i2c_lock);
	if (ret < 0)
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
	return ret;
}

static int pca9468_update_reg(struct pca9468_charger *pca9468, int reg, u8 mask, u8 val)
{
	int ret = 0;

	mutex_lock(&pca9468->i2c_lock);
	ret = regmap_update_bits(pca9468->regmap, reg, mask, val);
	if (reg == PCA9468_REG_START_CTRL && mask == PCA9468_BIT_STANDBY_EN) {
		if (val == PCA9468_STANDBY_DONOT) {
			pr_info("%s: PCA9468_STANDBY_DONOT 50ms\n", __func__);
			/* Wait 50ms, first to keep the start-up sequence */
			msleep(50);
		} else {
			pr_info("%s: PCA9468_STANDBY_FORCED 5ms\n", __func__);
			/* Wait 5ms to keep the shutdown sequence */
			usleep_range(5000, 6000);
		}
	}
	mutex_unlock(&pca9468->i2c_lock);
	if (ret < 0)
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
	return ret;
}

static int pca9468_read_adc(struct pca9468_charger *pca9468, u8 adc_ch);

static int pca9468_set_charging_state(struct pca9468_charger *pca9468, unsigned int charging_state) {
	union power_supply_propval value = {0,};
	static int prev_val = DC_STATE_NO_CHARGING;

	pca9468->charging_state = charging_state;

	switch (charging_state) {
	case DC_STATE_NO_CHARGING:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_OFF;
		break;
	case DC_STATE_CHECK_VBAT:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_CHECK_VBAT;
		break;
	case DC_STATE_PRESET_DC:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_PRESET;
		break;
	case DC_STATE_CHECK_ACTIVE:
	case DC_STATE_START_CC:
	case DC_STATE_START_CV:
	case DC_STATE_ADJUST_TAVOL:
	case DC_STATE_ADJUST_TACUR:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_ON_ADJUST;
		break;
	case DC_STATE_CC_MODE:
	case DC_STATE_CV_MODE:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_ON;
		break;
	case DC_STATE_CHARGING_DONE:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_DONE;
		break;
	default:
		return -1;
		break;
	}

	if (prev_val == value.intval)
		return -1;

	prev_val = value.intval;
	psy_do_property(pca9468->pdata->sec_dc_name, set,
				POWER_SUPPLY_EXT_PROP_DIRECT_CHARGER_MODE, value);

	return 0;
}

static void pca9468_init_adc_val(struct pca9468_charger *pca9468, int val)
{
	int i = 0;

	for (i = 0; i < ADCCH_MAX; ++i)
		pca9468->adc_val[i] = val;
}

static void pca9468_test_read(struct pca9468_charger *pca9468)
{
	int address = 0;
	unsigned int val;
	char str[1024] = { 0, };

	for (address = PCA9468_REG_INT1_STS; address <= PCA9468_REG_STS_ADC_9; address++) {
		pca9468_read_reg(pca9468, address, &val);
		sprintf(str + strlen(str), "[0x%02x]0x%02x, ", address, val);
	}

	for (address = PCA9468_REG_ICHG_CTRL; address <= PCA9468_REG_NTC_TH_2; address++) {
		pca9468_read_reg(pca9468, address, &val);
		sprintf(str + strlen(str), "[0x%02x]0x%02x, ", address, val);
	}

	pr_info("## pca9468 : [DC_CPEN:%d]%s\n", gpio_get_value(pca9468->pdata->chgen_gpio), str);
}

static void pca9468_monitor_work(struct pca9468_charger *pca9468)
{
	int ta_vol = pca9468->ta_vol / PCA9468_SEC_DENOM_U_M;
	int ta_cur = pca9468->ta_cur / PCA9468_SEC_DENOM_U_M;
	if (pca9468->charging_state == DC_STATE_NO_CHARGING)
		return;

	/* update adc value */
	pca9468_read_adc(pca9468, ADCCH_VIN);
	pca9468_read_adc(pca9468, ADCCH_IIN);
	pca9468_read_adc(pca9468, ADCCH_VBAT);
	pca9468_read_adc(pca9468, ADCCH_DIETEMP);

	pr_info("%s: state(%s), iin_cc(%dmA), v_float(%dmV), vbat(%dmV), vin(%dmV), iin(%dmA), die_temp(%d), isys(%dmA), pps_requested(%d/%dmV/%dmA)", __func__,
		charging_state_str[pca9468->charging_state],
		pca9468->iin_cc / PCA9468_SEC_DENOM_U_M, pca9468->pdata->v_float / PCA9468_SEC_DENOM_U_M,
		pca9468->adc_val[ADCCH_VBAT], pca9468->adc_val[ADCCH_VIN],
		pca9468->adc_val[ADCCH_IIN], pca9468->adc_val[ADCCH_DIETEMP],
		get_system_current(pca9468), pca9468->ta_objpos,
		ta_vol, ta_cur); 
}
#endif


/*******************************/
/* Switching charger control function */
/*******************************/
/* This function needs some modification by a customer */
#if defined(CONFIG_BATTERY_SAMSUNG)
static void pca9468_set_wdt_enable(struct pca9468_charger *pca9468, bool enable)
{
	int ret;
	unsigned int val;

	val = enable << MASK2SHIFT(PCA9468_BIT_WATCHDOG_EN);
	ret = pca9468_update_reg(pca9468, PCA9468_REG_SAFETY_CTRL,
			PCA9468_BIT_WATCHDOG_EN, val);
	pr_info("%s: set wdt enable = %d\n", __func__, enable);
}

static void pca9468_set_wdt_timer(struct pca9468_charger *pca9468, int time)
{
	int ret;
	unsigned int val;

	val = time << MASK2SHIFT(PCA9468_BIT_WATCHDOG_CFG);
	ret = pca9468_update_reg(pca9468, PCA9468_REG_SAFETY_CTRL,
			PCA9468_BIT_WATCHDOG_CFG, val);
	pr_info("%s: set wdt time = %d\n", __func__, time);
}

#if defined(CONFIG_ENG_BATTERY_CONCEPT)
static void pca9468_check_wdt_control(struct pca9468_charger *pca9468)
{
	struct device *dev = pca9468->dev;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);

	if (pca9468->wdt_kick) {
		pca9468_set_wdt_timer(pca9468, WDT_8SEC);
		schedule_delayed_work(&pca9468->wdt_control_work, msecs_to_jiffies(PCA9468_BATT_WDT_CONTROL_T));
	} else {
		pca9468_set_wdt_timer(pca9468, WDT_8SEC);
		if (client->addr == 0xff)
			client->addr = 0x57;
	}
}

static void pca9468_wdt_control_work(struct work_struct *work)
{
	struct pca9468_charger *pca9468 = container_of(work, struct pca9468_charger,
						 wdt_control_work.work);
	struct device *dev = pca9468->dev;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	int vin, iin;
	
	pca9468_set_wdt_timer(pca9468, WDT_4SEC);

	/* this is for kick watchdog */
	vin = pca9468_read_adc(pca9468, ADCCH_VIN);
	iin = pca9468_read_adc(pca9468, ADCCH_IIN);

	pca9468_send_pd_message(pca9468, PD_MSG_REQUEST_APDO);

	client->addr = 0xff;

	pr_info("## %s: disable slave addr (vin:%dmV, iin:%dmA)\n",
		__func__, vin/PCA9468_SEC_DENOM_U_M, iin/PCA9468_SEC_DENOM_U_M);
}
#endif

static void pca9468_set_done(struct pca9468_charger *pca9468, bool enable)
{
	int ret = 0;
	union power_supply_propval value = {0, };

	value.intval = enable;
	psy_do_property(pca9468->pdata->sec_dc_name, set,
		POWER_SUPPLY_EXT_PROP_DIRECT_DONE, value);

	if (ret < 0)
		pr_info("%s: error set_done, ret=%d\n", __func__, ret);
}

static void pca9468_set_switching_charger(struct pca9468_charger *pca9468, bool enable)
{
	int ret = 0;
	union power_supply_propval value = {0, };

	value.intval = enable;
	psy_do_property(pca9468->pdata->sec_dc_name, set,
		POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED_DC, value);

	if (ret < 0)
		pr_info("%s: error switching_charger, ret=%d\n", __func__, ret);
}
#else
static int pca9468_set_switching_charger( bool enable, 
					unsigned int input_current, 
					unsigned int charging_current, 
					unsigned int vfloat)
{
	int ret;
	struct power_supply *psy_swcharger;
	union power_supply_propval val;

	pr_info("%s: enable=%d, iin=%d, ichg=%d, vfloat=%d\n", 
		__func__, enable, input_current, charging_current, vfloat);
	
	/* Insert Code */

	/* Get power supply name */
#ifdef CONFIG_USBPD_PHY_QCOM
	psy_swcharger = power_supply_get_by_name("usb");
#else
	/* Change "sw-charger" to the customer's switching charger name */
	psy_swcharger = power_supply_get_by_name("sw-charger");
#endif
	if (psy_swcharger == NULL) {
		pr_err("%s: cannot get power_supply_name-usb\n", __func__);
		ret = -ENODEV;
		goto error;
	}

	if (enable == true)	{
		/* Set Switching charger */

		/* input current */
		val.intval = input_current;
		ret = power_supply_set_property(psy_swcharger, POWER_SUPPLY_PROP_CURRENT_MAX, &val);
		if (ret < 0)
			goto error;
		/* charigng current */
		val.intval = charging_current;
		ret = power_supply_set_property(psy_swcharger, POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT, &val);
		if (ret < 0)
			goto error;
		/* vfloat voltage */
		val.intval = vfloat;
		ret = power_supply_set_property(psy_swcharger, POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE, &val);
		if (ret < 0)
			goto error;

		/* it depends on customer's code to enable charger */
		val.intval = enable;
		ret = power_supply_set_property(psy_swcharger, POWER_SUPPLY_PROP_CHARGING_ENABLED, &val);
		if (ret < 0)
			goto error;
	} else {
		/* disable charger */
		/* it depends on customer's code to disable charger */
		val.intval = enable;
		ret = power_supply_set_property(psy_swcharger, POWER_SUPPLY_PROP_CHARGING_ENABLED, &val);
		if (ret < 0)
			goto error;

		/* input_current */
		val.intval = input_current;
		ret = power_supply_set_property(psy_swcharger, POWER_SUPPLY_PROP_CURRENT_MAX, &val);
		if (ret < 0)
			goto error;
	}

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}
#endif

#if !defined(CONFIG_BATTERY_SAMSUNG)
static int pca9468_get_switching_charger_property(enum power_supply_property prop,
						  union power_supply_propval *val)
{
	int ret;
	struct power_supply *psy_swcharger;

	/* Get power supply name */
#ifdef CONFIG_USBPD_PHY_QCOM
	psy_swcharger = power_supply_get_by_name("usb");
#else
	psy_swcharger = power_supply_get_by_name("sw-charger");
#endif
	if (psy_swcharger == NULL) {
		ret = -EINVAL;
		goto error;
	}
	ret = power_supply_get_property(psy_swcharger, prop, val);

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}
#endif

/******************/
/* Send PD message */
/******************/
/* Send Request message to the source */
/* This function needs some modification by a customer */
static int pca9468_send_pd_message(struct pca9468_charger *pca9468, unsigned int msg_type)
{
#ifdef CONFIG_USBPD_PHY_QCOM
#elif defined(CONFIG_BATTERY_SAMSUNG)
	unsigned int pdo_idx, pps_vol, pps_cur;
#else
	u8 msg_buf[4];	/* Data Buffer for raw PD message */
	unsigned int max_cur;
	unsigned int op_cur, out_vol;
#endif
	int ret = 0;

	/* Cancel pps request timer */
	cancel_delayed_work(&pca9468->pps_work);

	mutex_lock(&pca9468->lock);

	if (pca9468->charging_state == DC_STATE_NO_CHARGING) {
		/* Vbus reset happened in the previous PD communication */
		goto out;
	}
	
#ifdef CONFIG_USBPD_PHY_QCOM
	/* check the phandle */
	if (pca9468->pd == NULL) {
		pr_info("%s: get phandle\n", __func__);
		ret = pca9468_usbpd_setup(pca9468);
		if (ret != 0) {
			dev_err(pca9468->dev, "Error usbpd setup!\n");
			pca9468->pd = NULL;
			goto out;
		}
	}
#endif

	pr_info("%s: msg_type=%d, ta_cur=%d, ta_vol=%d, ta_objpos=%d\n", 
		__func__, msg_type, pca9468->ta_cur, pca9468->ta_vol, pca9468->ta_objpos);

#if defined(CONFIG_BATTERY_SAMSUNG)
	pdo_idx = pca9468->ta_objpos;
	pps_vol = pca9468->ta_vol / PCA9468_SEC_DENOM_U_M;
	pps_cur = pca9468->ta_cur / PCA9468_SEC_DENOM_U_M;
	pr_info("## %s: msg_type=%d, pdo_idx=%d, pps_vol=%dmV(max_vol=%dmV), pps_cur=%dmA(max_cur=%dmA)\n", 
		__func__, msg_type, pdo_idx,
		pps_vol, pca9468->pdo_max_voltage,
		pps_cur, pca9468->pdo_max_current);
#endif
		
	switch (msg_type) {
	case PD_MSG_REQUEST_APDO:
#ifdef CONFIG_USBPD_PHY_QCOM
		ret = usbpd_request_pdo(pca9468->pd, pca9468->ta_objpos, pca9468->ta_vol, pca9468->ta_cur);
		if (ret == -EBUSY) {
			/* wait 100ms */
			msleep(100);
			/* try again */
			ret = usbpd_request_pdo(pca9468->pd, pca9468->ta_objpos, pca9468->ta_vol, pca9468->ta_cur);
		}
#elif defined(CONFIG_BATTERY_SAMSUNG)
		ret = sec_pd_select_pps(pdo_idx, pps_vol, pps_cur);
		if (ret == -EBUSY) {
			pr_info("%s: request again ret=%d\n", __func__, ret);
			msleep(100);
			ret = sec_pd_select_pps(pdo_idx, pps_vol, pps_cur);
		}
#else
		op_cur = pca9468->ta_cur/50000; 	// Operating Current 50mA units
		out_vol = pca9468->ta_vol/20000;	// Output Voltage in 20mV units
		msg_buf[0] = op_cur & 0x7F; 		// Operating Current 50mA units - B6...0
		msg_buf[1] = (out_vol<<1) & 0xFE;	// Output Voltage in 20mV units - B19..(B15)..B9
		msg_buf[2] = (out_vol>>7) & 0x0F;	// Output Voltage in 20mV units - B19..(B16)..B9,
		msg_buf[3] = pca9468->ta_objpos<<4; // Object Position - B30...B28

		/* Send the PD messge to CC/PD chip */
		/* Todo - insert code */
#endif
		/* Start pps request timer */
		if (ret == 0) {
			schedule_delayed_work(&pca9468->pps_work, msecs_to_jiffies(PCA9468_PPS_PERIODIC_T));
		}
		break;
		
	case PD_MSG_REQUEST_FIXED_PDO:
#ifdef CONFIG_USBPD_PHY_QCOM
		ret = usbpd_request_pdo(pca9468->pd, pca9468->ta_objpos, pca9468->ta_vol, pca9468->ta_cur);
		if (ret == -EBUSY) {
			/* wait 100ms */
			msleep(100);
			/* try again */
			ret = usbpd_request_pdo(pca9468->pd, pca9468->ta_objpos, pca9468->ta_vol, pca9468->ta_cur);
		}
#elif defined(CONFIG_BATTERY_SAMSUNG)
		ret = sec_pd_select_pps(pdo_idx, pps_vol, pps_cur);
		if (ret == -EBUSY) {
			pr_info("%s: request again ret=%d\n", __func__, ret);
			msleep(100);
			ret = sec_pd_select_pps(pdo_idx, pps_vol, pps_cur);
		}
#else
		max_cur = pca9468->ta_cur/10000;	// Maximum Operation Current 10mA units
		op_cur = max_cur;					// Operating Current 10mA units
		msg_buf[0] = max_cur & 0xFF;		// Maximum Operation Current -B9..(7)..0
		msg_buf[1] = ((max_cur>>8) & 0x03) | ((op_cur<<2) & 0xFC);	// Operating Current - B19..(15)..10
		msg_buf[2] = ((op_cur>>6) & 0x0F);	// Operating Current - B19..(16)..10, Unchunked Extended Messages Supported  - not support
		msg_buf[3] = pca9468->ta_objpos<<4; // Object Position - B30...B28

		/* Send the PD messge to CC/PD chip */
		/* Todo - insert code */
#endif
		break;

	default:
		break;
	}

out:
	if (pca9468->charging_state == DC_STATE_NO_CHARGING) {
		/* Even though PD communication success, Vbus reset might happen
			So, check the charging state again */
		ret = -EINVAL;
	}
	
	pr_info("%s: ret=%d\n", __func__, ret);
	mutex_unlock(&pca9468->lock);
	return ret;
}

/************************/
/* Get APDO max power   */
/************************/
/* Get the max current/voltage/power of APDO from the CC/PD driver */
/* This function needs some modification by a customer */
static int pca9468_get_apdo_max_power(struct pca9468_charger *pca9468)
{
	int ret = 0;
#if defined(CONFIG_BATTERY_SAMSUNG)
	unsigned int ta_max_vol_mv = (pca9468->ta_max_vol/PCA9468_SEC_DENOM_U_M);
	unsigned int ta_max_cur_ma = 0;
	unsigned int ta_max_pwr_mw = 0;
#endif

#ifdef CONFIG_USBPD_PHY_QCOM
	/* check the phandle */
	if (pca9468->pd == NULL) {
		pr_info("%s: get phandle\n", __func__);
		ret = pca9468_usbpd_setup(pca9468);
		if (ret != 0) {
			dev_err(pca9468->dev, "Error usbpd setup!\n");
			pca9468->pd = NULL;
			goto out;
		}
	}

	ret = usbpd_get_apdo_max_power(pca9468->pd, &pca9468->ta_objpos,
			&pca9468->ta_max_vol, &pca9468->ta_max_cur, &pca9468->ta_max_pwr);
#elif defined(CONFIG_BATTERY_SAMSUNG)
	ret = sec_pd_get_apdo_max_power(&pca9468->ta_objpos,
				&ta_max_vol_mv, &ta_max_cur_ma, &ta_max_pwr_mw);
	/* mA,mV,mW --> uA,uV,uW */
	pca9468->ta_max_vol = ta_max_vol_mv * PCA9468_SEC_DENOM_U_M;
	pca9468->ta_max_cur = ta_max_cur_ma * PCA9468_SEC_DENOM_U_M;
	pca9468->ta_max_pwr = ta_max_pwr_mw;

	pr_info("%s: ta_max_vol=%d, ta_max_cur=%d, ta_max_pwr=%d\n", 
		__func__, pca9468->ta_max_vol, pca9468->ta_max_cur, pca9468->ta_max_pwr);

	pca9468->pdo_index = pca9468->ta_objpos;
	pca9468->pdo_max_voltage = ta_max_vol_mv;
	pca9468->pdo_max_current = ta_max_cur_ma;
#else
	/* Put ta_max_vol to the desired ta maximum value, ex) 9800mV */
	/* Get new ta_max_vol and ta_max_cur, ta_max_power and proper object position by CC/PD IC */
	/* insert code */
#endif

#ifdef CONFIG_USBPD_PHY_QCOM
out:
#endif
	return ret;
}


/* ADC Read function */
static int pca9468_read_adc(struct pca9468_charger *pca9468, u8 adc_ch)
{
	u8 reg_data[2];
	u16 raw_adc;	// raw ADC value
	int conv_adc;	// conversion ADC value
	int ret;
	u8 rsense; /* sense resistance */

	switch (adc_ch)
	{
	case ADCCH_VOUT:
		// Read ADC value
		ret = pca9468_bulk_read_reg(pca9468, PCA9468_REG_STS_ADC_4, reg_data, 2);
		if (ret < 0) {
			conv_adc = ret;
			goto error;
		}
		// Convert ADC
		raw_adc = ((reg_data[1] & PCA9468_BIT_ADC_VOUT9_2)<<2) | ((reg_data[0] & PCA9468_BIT_ADC_VOUT1_0)>>6);
		conv_adc = raw_adc * VOUT_STEP;	// unit - uV
		break;
	case ADCCH_VIN:
		// Read ADC value
		ret = pca9468_bulk_read_reg(pca9468, PCA9468_REG_STS_ADC_3, reg_data, 2);
		if (ret < 0) {
			conv_adc = ret;
			goto error;
		}
		// Convert ADC
		raw_adc = ((reg_data[1] & PCA9468_BIT_ADC_VIN9_4)<<4) | ((reg_data[0] & PCA9468_BIT_ADC_VIN3_0)>>4);
		conv_adc = raw_adc * VIN_STEP;	// uint - uV
		break;

	case ADCCH_VBAT:
		// Read ADC value
		ret = pca9468_bulk_read_reg(pca9468, PCA9468_REG_STS_ADC_6, reg_data, 2);
		if (ret < 0) {
			conv_adc = ret;
			goto error;
		}
		// Convert ADC
		raw_adc = ((reg_data[1] & PCA9468_BIT_ADC_VBAT9_8)<<8) | ((reg_data[0] & PCA9468_BIT_ADC_VBAT7_0)>>0);
		conv_adc = raw_adc * VBAT_STEP;		// unit - uV
		break;

	case ADCCH_ICHG:
		// Read ADC value
		ret = pca9468_bulk_read_reg(pca9468, PCA9468_REG_STS_ADC_2, reg_data, 2);
		if (ret < 0) {
			conv_adc = ret;
			goto error;
		}
		// Convert ADC
		rsense = (pca9468->pdata->snsres == 0) ? 5 : 10;	// snsres : 0 - 5mOhm, 1 - 10mOhm
		raw_adc = ((reg_data[1] & PCA9468_BIT_ADC_IOUT9_6)<<6) | ((reg_data[0] & PCA9468_BIT_ADC_IOUT5_0)>>2);
		conv_adc = raw_adc * ICHG_STEP;	// unit - uA
		break;

	case ADCCH_IIN:
		// Read ADC value
		ret = pca9468_bulk_read_reg(pca9468, PCA9468_REG_STS_ADC_1, reg_data, 2);
		if (ret < 0) {
			conv_adc = ret;
			goto error;
		}
		// Convert ADC - iin = rawadc*4.89 + (rawadc*4.89 - 900)*adc_comp_gain/100, unit - uA
		raw_adc = ((reg_data[1] & PCA9468_BIT_ADC_IIN9_8)<<8) | ((reg_data[0] & PCA9468_BIT_ADC_IIN7_0)>>0);
		conv_adc = raw_adc * IIN_STEP + (raw_adc * IIN_STEP - ADC_IIN_OFFSET)*pca9468->adc_comp_gain/100;		// unit - uA
		if (conv_adc < 0)
			conv_adc = 0;	// If ADC raw value is 0, convert value will be minus value becasue of compensation gain, so in this case conv_adc is 0
		break;

	
	case ADCCH_DIETEMP:
		// Read ADC value
		ret = pca9468_bulk_read_reg(pca9468, PCA9468_REG_STS_ADC_7, reg_data, 2);
		if (ret < 0) {
			conv_adc = ret;
			goto error;
		}
		// Convert ADC
		raw_adc = ((reg_data[1] & PCA9468_BIT_ADC_DIETEMP9_6)<<6) | ((reg_data[0] & PCA9468_BIT_ADC_DIETEMP5_0)>>2);
		conv_adc = (935 - raw_adc)*DIETEMP_STEP/DIETEMP_DENOM; 	// Temp = (935-rawadc)*0.435, unit - C
		if (conv_adc > DIETEMP_MAX) {
			conv_adc = DIETEMP_MAX;
		} else if (conv_adc < DIETEMP_MIN) {
			conv_adc = DIETEMP_MIN;
		}
		break;

	case ADCCH_NTC:
		// Read ADC value
		ret = pca9468_bulk_read_reg(pca9468, PCA9468_REG_STS_ADC_8, reg_data, 2);
		if (ret < 0) {
			conv_adc = ret;
			goto error;
		}
		// Convert ADC
		raw_adc = ((reg_data[1] & PCA9468_BIT_ADC_NTCV9_4)<<4) | ((reg_data[0] & PCA9468_BIT_ADC_NTCV3_0)>>4);
		conv_adc = raw_adc * NTCV_STEP;		// unit - uV
		break;

	default:
		conv_adc = -EINVAL;
		break;
	}

error:
	pr_info("%s: adc_ch=%d, convert_val=%d\n", __func__, adc_ch, conv_adc);

#if defined(CONFIG_BATTERY_SAMSUNG)
	if (adc_ch == ADCCH_DIETEMP)
		pca9468->adc_val[adc_ch] = conv_adc;
	else
		pca9468->adc_val[adc_ch] = conv_adc / PCA9468_SEC_DENOM_U_M;
#endif
	return conv_adc;
}


static int pca9468_set_vfloat(struct pca9468_charger *pca9468, unsigned int v_float)
{
	int ret, val;

	pr_info("%s: vfloat=%d\n", __func__, v_float);

	/* v float voltage */
	if (v_float < PCA9468_VFLOAT_MIN) {
		v_float = PCA9468_VFLOAT_MIN;
		pr_info("%s: -> vfloat=%d\n", __func__, v_float);
	}
	val = PCA9468_V_FLOAT(v_float);
	ret = pca9468_write_reg(pca9468, PCA9468_REG_V_FLOAT, val);
	return ret;
}

static int pca9468_set_charging_current(struct pca9468_charger *pca9468, unsigned int ichg)
{
	int ret, val;

	pr_info("%s: ichg=%d\n", __func__, ichg);

	/* charging current */
	if (ichg > PCA9468_ICHG_CFG_MAX) {
		ichg = PCA9468_ICHG_CFG_MAX;
		pr_info("%s: -> ichg=%d\n", __func__, ichg);
	}
	val = PCA9468_ICHG_CFG(ichg);
	ret = pca9468_update_reg(pca9468, PCA9468_REG_ICHG_CTRL, PCA9468_BIT_ICHG_CFG, val);
	return ret;
}

static int pca9468_set_input_current(struct pca9468_charger *pca9468, unsigned int iin)
{
	int ret, val;

	pr_info("%s: iin=%d\n", __func__, iin);

	/* input current */
	/* round off and increase one step */
	iin = iin + PD_MSG_TA_CUR_STEP;
	val = PCA9468_IIN_CFG(iin);
	/* Set IIN_CFG to one step higher */
	val = val + 1;

	if (val > 0x32)
		val = 0x32;	/* maximum value is 5A */

	ret = pca9468_update_reg(pca9468, PCA9468_REG_IIN_CTRL, PCA9468_BIT_IIN_CFG, val);
	pr_info("%s: real iin_cfg=%d\n", __func__, val*PCA9468_IIN_CFG_STEP);
	return ret;
}

static int pca9468_set_charging(struct pca9468_charger *pca9468, bool enable)
{
	int ret, val;

	pr_info("%s: enable=%d\n", __func__, enable);

	if (enable == true) {
		/* Improve adc */
		val = 0x5B;
		ret = pca9468_write_reg(pca9468, PCA9468_REG_ADC_ACCESS, val);
		if (ret < 0)
			goto error;
		ret = pca9468_update_reg(pca9468, PCA9468_REG_ADC_IMPROVE, PCA9468_BIT_ADC_IIN_IMP, 0);
		if (ret < 0)
			goto error;

		/* For fixing input current error */
		/* Overwirte 0x00 in 0x41 register */
		val = 0x00;
		ret = pca9468_write_reg(pca9468, 0x41, val);
		if (ret < 0)
			goto error;
		/* Overwirte 0x01 in 0x43 register */
		val = 0x01;
		ret = pca9468_write_reg(pca9468, 0x43, val);
		if (ret < 0)
			goto error;

		/* Overwirte 0x00 in 0x4B register */
		val = 0x00;
		ret = pca9468_write_reg(pca9468, 0x4B, val);
		if (ret < 0)
			goto error;
		/* End for fixing input current error */

	} else {
		/* Disable NTC_PROTECTION_EN */
		ret = pca9468_update_reg(pca9468, PCA9468_REG_TEMP_CTRL,
				PCA9468_BIT_NTC_PROTECTION_EN, 0);
	}

	/* Enable PCA9468 */
	val = (enable == true) ? PCA9468_STANDBY_DONOT: PCA9468_STANDBY_FORCED;
	ret = pca9468_update_reg(pca9468, PCA9468_REG_START_CTRL, PCA9468_BIT_STANDBY_EN, val);
	if (ret < 0)
		goto error;

	if (enable == true) {
		msleep(150);
		ret = pca9468_update_reg(pca9468, PCA9468_REG_ADC_IMPROVE, PCA9468_BIT_ADC_IIN_IMP, PCA9468_BIT_ADC_IIN_IMP);
		if (ret  < 0)
			goto error;
		
		val = 0x00;
		ret = pca9468_write_reg(pca9468, PCA9468_REG_ADC_ACCESS, val);

		/* Enable NTC_PROTECTION_EN */
		ret = pca9468_update_reg(pca9468, PCA9468_REG_TEMP_CTRL, 
				PCA9468_BIT_NTC_PROTECTION_EN, PCA9468_BIT_NTC_PROTECTION_EN);
		/* Enable TEMP_REG_EN */
		ret = pca9468_update_reg(pca9468, PCA9468_REG_TEMP_CTRL, 
				PCA9468_BIT_TEMP_REG_EN, PCA9468_BIT_TEMP_REG_EN);
	}

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* Stop Charging */
static int pca9468_stop_charging(struct pca9468_charger *pca9468)
{
	int ret = 0;

	/* Check the current state */
#if defined(CONFIG_BATTERY_SAMSUNG)
	if ((pca9468->charging_state != DC_STATE_NO_CHARGING) ||
		(pca9468->timer_id != TIMER_ID_NONE)) {
#else
	if (pca9468->charging_state != DC_STATE_NO_CHARGING) {
#endif
		// Stop Direct charging 
		cancel_delayed_work(&pca9468->timer_work);
		cancel_delayed_work(&pca9468->pps_work);
		mutex_lock(&pca9468->lock);
		pca9468->timer_id = TIMER_ID_NONE;
		pca9468->timer_period = 0;
		mutex_unlock(&pca9468->lock);
		__pm_relax(&pca9468->monitor_wake_lock);

		/* Clear parameter */
#if defined(CONFIG_BATTERY_SAMSUNG)
		pca9468_set_charging_state(pca9468, DC_STATE_NO_CHARGING);
		pca9468_init_adc_val(pca9468, -1);
#else
		pca9468->charging_state = DC_STATE_NO_CHARGING;
#endif
		pca9468->ret_state = DC_STATE_NO_CHARGING;
		pca9468->ta_target_vol = PCA9468_TA_MAX_VOL;
		pca9468->prev_iin = 0;
		pca9468->prev_inc = INC_NONE;
		mutex_lock(&pca9468->lock);
		pca9468->req_new_iin = false;
		pca9468->req_new_vfloat = false;
		mutex_unlock(&pca9468->lock);
		pca9468->ta_mode = TA_NO_DC_MODE;

		/* Set IIN_CFG and VFLOAT to the default value */
		pca9468->pdata->iin_cfg = PCA9468_IIN_CFG_DFT;
		pca9468->pdata->v_float = pca9468->pdata->v_float_max;

		/* Clear new Vfloat and new IIN */
		pca9468->new_vfloat = pca9468->pdata->v_float;
		pca9468->new_iin = pca9468->pdata->iin_cfg;

		/* Clear retry counter */
		pca9468->retry_cnt = 0;

		ret = pca9468_set_charging(pca9468, false);

#if defined(CONFIG_BATTERY_SAMSUNG)
		/* Set watchdog timer disable */
		pca9468_set_wdt_enable(pca9468, WDT_DISABLE);
#endif
	}
	
	pr_info("%s: END, ret=%d\n", __func__, ret);
	
	return ret;
}


/* Compensate TA current for the target input current */
static int pca9468_set_ta_current_comp(struct pca9468_charger *pca9468)
{
	int iin;
	
	/* Read IIN ADC */
	iin = pca9468_read_adc(pca9468, ADCCH_IIN);

	/* Compare IIN ADC with targer input current */
	if (iin > (pca9468->iin_cc + PCA9468_IIN_CC_COMP_OFFSET)) {
		/* TA current is higher than the target input current */
		/* Decrease TA current (50mA) */
		pca9468->ta_cur = pca9468->ta_cur - PD_MSG_TA_CUR_STEP;

		/* Send PD Message */
		mutex_lock(&pca9468->lock);
		pca9468->timer_id = TIMER_PDMSG_SEND;
		pca9468->timer_period = 0;
		mutex_unlock(&pca9468->lock);
	} else {
		if (iin < (pca9468->iin_cc - PCA9468_IIN_CC_COMP_OFFSET)) {
			/* compare IIN ADC with previous IIN ADC + 20mA */
			if (iin > (pca9468->prev_iin + PCA9468_IIN_ADC_OFFSET)) {
				/* Compare TA max voltage */
				if (pca9468->ta_vol == pca9468->ta_max_vol) {
					/* TA voltage is already the maximum voltage */
					/* Set timer */
					mutex_lock(&pca9468->lock);
					pca9468->timer_id = TIMER_CHECK_CCMODE;
					pca9468->timer_period = PCA9468_CCMODE_CHECK1_T;
					mutex_unlock(&pca9468->lock);
				} else {
					/* Increase TA voltage (20mV) */
					pca9468->ta_vol = pca9468->ta_vol + PD_MSG_TA_VOL_STEP;
					pr_info("%s: Comp. Cont: ta_vol=%d\n", __func__, pca9468->ta_vol);
					/* Send PD Message */
					mutex_lock(&pca9468->lock);
					pca9468->timer_id = TIMER_PDMSG_SEND;
					pca9468->timer_period = 0;
					mutex_unlock(&pca9468->lock);
				}
				/* Set TA increment flag */
				pca9468->prev_inc = INC_TA_VOL;
			} else {			
				/* TA current is lower than the target input current */
				/* Check the previous TA increment */
				if (pca9468->prev_inc == INC_TA_VOL) {
					/* The previous increment is TA voltage, but input current does not increase */
					/* Try to increase TA current */
					/* Compare TA max current */
					if (pca9468->ta_cur == pca9468->ta_max_cur) {
						/* TA current is already the maximum current */
						/* Set timer */
						mutex_lock(&pca9468->lock);
						pca9468->timer_id = TIMER_CHECK_CCMODE;
						pca9468->timer_period = PCA9468_CCMODE_CHECK1_T;
						mutex_unlock(&pca9468->lock);
					} else {
						/* Increase TA current (50mA) */
						pca9468->ta_cur = pca9468->ta_cur + PD_MSG_TA_CUR_STEP;
						pr_info("%s: Comp. Cont: ta_cur=%d\n", __func__, pca9468->ta_cur);
						/* Send PD Message */
						mutex_lock(&pca9468->lock);
						pca9468->timer_id = TIMER_PDMSG_SEND;
						pca9468->timer_period = 0;
						mutex_unlock(&pca9468->lock);
					}
					/* Set TA increment flag */
					pca9468->prev_inc = INC_TA_CUR;
				} else {				
					/* The previous increment is TA current, but input current does not increase */
					/* Try to increase TA voltage */
					/* Compare TA max voltage */
					if (pca9468->ta_vol == pca9468->ta_max_vol) {
						
						/* TA voltage is already the maximum voltage */
						/* Set timer */
						mutex_lock(&pca9468->lock);
						pca9468->timer_id = TIMER_CHECK_CCMODE;
						pca9468->timer_period = PCA9468_CCMODE_CHECK1_T;
						mutex_unlock(&pca9468->lock);
					} else {
						/* Increase TA voltage (20mV) */
						pca9468->ta_vol = pca9468->ta_vol + PD_MSG_TA_VOL_STEP;
						pr_info("%s: Comp. Cont: ta_vol=%d\n", __func__, pca9468->ta_vol);

						/* Send PD Message */
						mutex_lock(&pca9468->lock);
						pca9468->timer_id = TIMER_PDMSG_SEND;
						pca9468->timer_period = 0;
						mutex_unlock(&pca9468->lock);
					}
					/* Set TA increment flag */
					pca9468->prev_inc = INC_TA_VOL;
				}
			}
		} else {
			/* IIN ADC is in valid range */
			/* IIN_CC - 50mA < IIN ADC < IIN_CC + 50mA  */
			/* Set timer */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_CHECK_CCMODE;
			pca9468->timer_period = PCA9468_CCMODE_CHECK1_T;
			mutex_unlock(&pca9468->lock);
		}
	}
	
	/* Save previous iin adc */
	pca9468->prev_iin = iin;
	
	schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));

	return 0;
}

/* Compensate TA current for constant power mode */
static int pca9468_set_ta_current_comp2(struct pca9468_charger *pca9468)
{
	int iin;
	unsigned int val;
	unsigned int iin_apdo;
	
	/* Read IIN ADC */
	iin = pca9468_read_adc(pca9468, ADCCH_IIN);

	/* Compare IIN ADC with targer input current */
	if (iin > (pca9468->pdata->iin_cfg + PCA9468_IIN_CC_UPPER_OFFSET)) {
		/* TA current is higher than the target input current */
		/* Decrease TA current (50mA) */
		pca9468->ta_cur = pca9468->ta_cur - PD_MSG_TA_CUR_STEP;

		/* Send PD Message */
		mutex_lock(&pca9468->lock);
		pca9468->timer_id = TIMER_PDMSG_SEND;
		pca9468->timer_period = 0;
		mutex_unlock(&pca9468->lock);
	} else if (iin < (pca9468->iin_cc - PCA9468_IIN_CC_COMP_OFFSET_CP)) {
		/* IIN_ADC < IIN_CC -20mA */
		if (pca9468->ta_vol == pca9468->ta_max_vol) {
			/* Check IIN_ADC < IIN_CC - 50mA */
			if (iin < (pca9468->iin_cc - PCA9468_IIN_CC_COMP_OFFSET)) {
				/* Set new IIN_CC to IIN_CC - 50mA */
				pca9468->iin_cc = pca9468->iin_cc - PCA9468_IIN_CC_COMP_OFFSET;
				/* Set new TA_MAX_VOL to TA_MAX_PWR/IIN_CC */
				/* Adjust new IIN_CC with APDO resolution */
				iin_apdo = pca9468->iin_cc/PD_MSG_TA_CUR_STEP;
				iin_apdo = iin_apdo*PD_MSG_TA_CUR_STEP;
				val = pca9468->ta_max_pwr/(iin_apdo/pca9468->ta_mode/1000);	/* mV */
				val = val*1000/PD_MSG_TA_VOL_STEP;	/* Adjust values with APDO resolution(20mV) */
				val = val*PD_MSG_TA_VOL_STEP; /* uV */
				/* Set new TA_MAX_VOL */
				pca9468->ta_max_vol = MIN(val, PCA9468_TA_MAX_VOL*pca9468->ta_mode);
				/* Increase TA voltage(40mV) */
				pca9468->ta_vol = pca9468->ta_vol + PD_MSG_TA_VOL_STEP*2;
				
				pr_info("%s: Comp. Cont1: ta_vol=%d\n", __func__, pca9468->ta_vol);
				/* Send PD Message */
				mutex_lock(&pca9468->lock);
				pca9468->timer_id = TIMER_PDMSG_SEND;
				pca9468->timer_period = 0;
				mutex_unlock(&pca9468->lock);
			} else {
				/* Wait for next current step compensation */
				/* IIN_CC - 50mA < IIN ADC < IIN_CC - 20mA*/
				/* Set timer */
				mutex_lock(&pca9468->lock);
				pca9468->timer_id = TIMER_CHECK_CCMODE;
				pca9468->timer_period = PCA9468_CCMODE_CHECK2_T;
				mutex_unlock(&pca9468->lock);			
			}
		} else {
			/* Increase TA voltage(40mV) */
			pca9468->ta_vol = pca9468->ta_vol + PD_MSG_TA_VOL_STEP*2;
			if (pca9468->ta_vol > pca9468->ta_max_vol)
				pca9468->ta_vol = pca9468->ta_max_vol;
			
			pr_info("%s: Comp. Cont2: ta_vol=%d\n", __func__, pca9468->ta_vol);
			/* Send PD Message */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_PDMSG_SEND;
			pca9468->timer_period = 0;
			mutex_unlock(&pca9468->lock);
		}
	} else {
		/* IIN ADC is in valid range */
		/* IIN_CC - 50mA < IIN ADC < IIN_CFG + 50mA */
		/* Set timer */
		mutex_lock(&pca9468->lock);
		pca9468->timer_id = TIMER_CHECK_CCMODE;
		pca9468->timer_period = PCA9468_CCMODE_CHECK2_T;
		mutex_unlock(&pca9468->lock);
	}
	
	/* Save previous iin adc */
	pca9468->prev_iin = iin;
	
	schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));

	return 0;
}

/* Compensate TA voltage for the target input current */
static int pca9468_set_ta_voltage_comp(struct pca9468_charger *pca9468)
{
	int iin;

	pr_info("%s: ======START=======\n", __func__);

	/* Read IIN ADC */
	iin = pca9468_read_adc(pca9468, ADCCH_IIN);

	/* Compare IIN ADC with target input current */
	if (iin > (pca9468->iin_cc + PCA9468_IIN_CC_COMP_OFFSET)) {
		/* TA current is higher than the target input current */
		/* Decrease TA voltage (20mV) */
		pca9468->ta_vol = pca9468->ta_vol - PD_MSG_TA_VOL_STEP;
		pr_info("%s: Decrease: ta_vol=%d\n", __func__, pca9468->ta_vol);

		/* Send PD Message */
		mutex_lock(&pca9468->lock);
		pca9468->timer_id = TIMER_PDMSG_SEND;
		pca9468->timer_period = 0;
		mutex_unlock(&pca9468->lock);
	} else {
		if (iin < (pca9468->iin_cc - PCA9468_IIN_CC_COMP_OFFSET)) {
			/* TA current is lower than the target input current */
			/* Compare TA max voltage */
			if (pca9468->ta_vol == pca9468->ta_max_vol) {
				/* TA current is already the maximum voltage */
				/* Set timer */
				/* Check the current charging state */
				if (pca9468->charging_state == DC_STATE_CC_MODE) {
					/* CC mode */
					mutex_lock(&pca9468->lock);
					pca9468->timer_id = TIMER_CHECK_CCMODE;
					pca9468->timer_period = PCA9468_CCMODE_CHECK1_T;
					mutex_unlock(&pca9468->lock);
				} else {
					/* CV mode */
					mutex_lock(&pca9468->lock);
					pca9468->timer_id = TIMER_CHECK_CVMODE;
					pca9468->timer_period = PCA9468_CVMODE_CHECK_T;
					mutex_unlock(&pca9468->lock);
				}
			} else {
				/* Increase TA voltage (20mV) */
				pca9468->ta_vol = pca9468->ta_vol + PD_MSG_TA_VOL_STEP;
				pr_info("%s: Increase: ta_vol=%d\n", __func__, pca9468->ta_vol);

				/* Send PD Message */
				mutex_lock(&pca9468->lock);
				pca9468->timer_id = TIMER_PDMSG_SEND;
				pca9468->timer_period = 0;
				mutex_unlock(&pca9468->lock);
			}
		} else {
			/* IIN ADC is in valid range */
			/* IIN_CC - 50mA < IIN ADC < IIN_CC + 50mA  */
			/* Set timer */
			/* Check the current charging state */
			if (pca9468->charging_state == DC_STATE_CC_MODE) {
				/* CC mode */
				mutex_lock(&pca9468->lock);
				pca9468->timer_id = TIMER_CHECK_CCMODE;
				pca9468->timer_period = PCA9468_CCMODE_CHECK1_T;
				mutex_unlock(&pca9468->lock);
			} else {
				/* CV mode */
				mutex_lock(&pca9468->lock);
				pca9468->timer_id = TIMER_CHECK_CVMODE;
				pca9468->timer_period = PCA9468_CVMODE_CHECK_T;
				mutex_unlock(&pca9468->lock);
			}
		}
	}
	schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));

	return 0;
}


/* Set TA current for target current */
static int pca9468_adjust_ta_current (struct pca9468_charger *pca9468)
{
	int ret = 0;
	int vbat;
	unsigned int val;

	/* Set charging state to ADJUST_TACUR */
#if defined(CONFIG_BATTERY_SAMSUNG)
	pca9468_set_charging_state(pca9468, DC_STATE_ADJUST_TACUR);
#else
	pca9468->charging_state = DC_STATE_ADJUST_TACUR;
#endif

	if (pca9468->ta_cur == pca9468->iin_cc/pca9468->ta_mode) {
		/* finish sending PD message */
		/* Recover IIN_CC to the original value(new_iin) */
		pca9468->iin_cc = pca9468->new_iin;
		
		/* Clear req_new_iin */
		mutex_lock(&pca9468->lock);
		pca9468->req_new_iin = false;
		mutex_unlock(&pca9468->lock);
			
		/* Go to return state  */
#if defined(CONFIG_BATTERY_SAMSUNG)
		pca9468_set_charging_state(pca9468, pca9468->ret_state);
#else
		pca9468->charging_state = pca9468->ret_state;
#endif
		/* Set timer */
		mutex_lock(&pca9468->lock);
		if (pca9468->charging_state == DC_STATE_CC_MODE)
			pca9468->timer_id = TIMER_CHECK_CCMODE;
		else
			pca9468->timer_id = TIMER_CHECK_CVMODE;
		pca9468->timer_period = 1000;	/* Wait 1000ms */
		mutex_unlock(&pca9468->lock);
	} else {
		/* Compare new IIN with IIN_CFG */
		if (pca9468->iin_cc > pca9468->pdata->iin_cfg) {
			/* New iin is higher than the current iin_cfg */
			/* Update iin_cfg */
			pca9468->pdata->iin_cfg = pca9468->iin_cc;
			/* Set IIN_CFG to new IIN */
			ret = pca9468_set_input_current(pca9468, pca9468->iin_cc);
			if (ret < 0)
				goto error;

			/* Clear Request flag */
			mutex_lock(&pca9468->lock);
			pca9468->req_new_iin = false;
			mutex_unlock(&pca9468->lock);

			/* Set new TA voltage and current */
			/* Read VBAT ADC */
			vbat = pca9468_read_adc(pca9468, ADCCH_VBAT);

			/* Calculate new TA maximum current and voltage that used in the direct charging */
			/* Set IIN_CC to MIN[IIN, TA_MAX_CUR*TA_mode]*/
			pca9468->iin_cc = MIN(pca9468->pdata->iin_cfg, pca9468->ta_max_cur*pca9468->ta_mode);
			/* Set the current IIN_CC to iin_cfg for recovering it after resolution adjustment */
			pca9468->pdata->iin_cfg = pca9468->iin_cc;
			/* Calculate new TA max voltage */
			/* Adjust IIN_CC with APDO resoultion(50mA) - It will recover to the original value after max voltage calculation */
			val = pca9468->iin_cc/(PD_MSG_TA_CUR_STEP*pca9468->ta_mode);
			pca9468->iin_cc = val*(PD_MSG_TA_CUR_STEP*pca9468->ta_mode);
			/* Set TA_MAX_VOL to MIN[PCA9468_TA_MAX_VOL, (TA_MAX_PWR/IIN_CC)] */
			val = pca9468->ta_max_pwr/(pca9468->iin_cc/pca9468->ta_mode/1000);	/* mV */
			val = val*1000/PD_MSG_TA_VOL_STEP;	/* Adjust values with APDO resolution(20mV) */
			val = val*PD_MSG_TA_VOL_STEP; /* uV */
			pca9468->ta_max_vol = MIN(val, PCA9468_TA_MAX_VOL*pca9468->ta_mode);
			
			/* Set TA voltage to MAX[8000mV, (2*VBAT_ADC + 500 mV)] */
			pca9468->ta_vol = max(PCA9468_TA_MIN_VOL_PRESET*pca9468->ta_mode, (2*vbat*pca9468->ta_mode + PCA9468_TA_VOL_PRE_OFFSET));
			val = pca9468->ta_vol/PD_MSG_TA_VOL_STEP;	/* PPS voltage resolution is 20mV */
			pca9468->ta_vol = val*PD_MSG_TA_VOL_STEP;
			/* Set TA voltage to MIN[TA voltage, TA_MAX_VOL] */
			pca9468->ta_vol = MIN(pca9468->ta_vol, pca9468->ta_max_vol);
			/* Set TA current to IIN_CC */
			pca9468->ta_cur = pca9468->iin_cc/pca9468->ta_mode;
			/* Recover IIN_CC to the original value(iin_cfg) */
			pca9468->iin_cc = pca9468->pdata->iin_cfg;

			pr_info("%s: New IIN, ta_max_vol=%d, ta_max_cur=%d, ta_max_pwr=%d, iin_cc=%d, ta_mode=%d\n", 
				__func__, pca9468->ta_max_vol, pca9468->ta_max_cur, pca9468->ta_max_pwr, pca9468->iin_cc, pca9468->ta_mode);

			/* Clear previous IIN ADC */
			pca9468->prev_iin = 0;
			/* Clear TA increment flag */
			pca9468->prev_inc = INC_NONE;

			/* Send PD Message and go to Adjust CC mode */
			pca9468->charging_state = DC_STATE_ADJUST_CC;
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_PDMSG_SEND;
			pca9468->timer_period = 0;
			mutex_unlock(&pca9468->lock);
		} else {
			/* Set TA voltage to TA target voltage */
			pca9468->ta_vol = pca9468->ta_target_vol;
			/* Adjust IIN_CC with APDO resoultion(50mA) - It will recover to the original value after sending PD message */
			val = pca9468->iin_cc/PD_MSG_TA_CUR_STEP;
			pca9468->iin_cc = val*PD_MSG_TA_CUR_STEP;
			/* Set TA current to IIN_CC */
			pca9468->ta_cur = pca9468->iin_cc/pca9468->ta_mode;
			/* Send PD Message */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_PDMSG_SEND;
			pca9468->timer_period = 0;
			mutex_unlock(&pca9468->lock);
		}
	}
	schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));

error:
	return ret;
}


/* Set TA voltage for targer current */
static int pca9468_adjust_ta_voltage(struct pca9468_charger *pca9468)
{
	int iin;

#if defined(CONFIG_BATTERY_SAMSUNG)
	pca9468_set_charging_state(pca9468, DC_STATE_ADJUST_TAVOL);
#else
	pca9468->charging_state = DC_STATE_ADJUST_TAVOL;
#endif
	
	/* Read IIN ADC */
	iin = pca9468_read_adc(pca9468, ADCCH_IIN);

	/* Compare IIN ADC with targer input current */
	if (iin > (pca9468->iin_cc + PD_MSG_TA_CUR_STEP)) {
		/* TA current is higher than the target input current */
		/* Decrease TA voltage (20mV) */
		pca9468->ta_vol = pca9468->ta_vol - PD_MSG_TA_VOL_STEP;
		
		/* Send PD Message */
		mutex_lock(&pca9468->lock);
		pca9468->timer_id = TIMER_PDMSG_SEND;
		pca9468->timer_period = 0;
		mutex_unlock(&pca9468->lock);
	} else {
		if (iin < (pca9468->iin_cc - PD_MSG_TA_CUR_STEP)) {
			/* TA current is lower than the target input current */
			/* Compare TA max voltage */
			if (pca9468->ta_vol == pca9468->ta_max_vol) {
				/* TA current is already the maximum voltage */
				/* Clear req_new_iin */
				mutex_lock(&pca9468->lock);
				pca9468->req_new_iin = false;
				mutex_unlock(&pca9468->lock);
				/* Return charging state to the previous state */
#if defined(CONFIG_BATTERY_SAMSUNG)
				pca9468_set_charging_state(pca9468, pca9468->ret_state);
#else
				pca9468->charging_state = pca9468->ret_state;
#endif
				/* Set TA current and voltage to the same value */
				/* Send PD Message */
				mutex_lock(&pca9468->lock);
				pca9468->timer_id = TIMER_PDMSG_SEND;
				pca9468->timer_period = 0;
				mutex_unlock(&pca9468->lock);
			} else {
				/* Increase TA voltage (20mV) */
				pca9468->ta_vol = pca9468->ta_vol + PD_MSG_TA_VOL_STEP;
				
				/* Send PD Message */
				mutex_lock(&pca9468->lock);
				pca9468->timer_id = TIMER_PDMSG_SEND;
				pca9468->timer_period = 0;
				mutex_unlock(&pca9468->lock);
			}
		} else {
			/* IIN ADC is in valid range */
			/* Clear req_new_iin */
			mutex_lock(&pca9468->lock);
			pca9468->req_new_iin = false;
			mutex_unlock(&pca9468->lock);

			/* IIN_CC - 50mA < IIN ADC < IIN_CC + 50mA  */
			/* Return charging state to the previous state */
#if defined(CONFIG_BATTERY_SAMSUNG)
			pca9468_set_charging_state(pca9468, pca9468->ret_state);
#else
			pca9468->charging_state = pca9468->ret_state;
#endif
			/* Set TA current and voltage to the same value */
			/* Send PD Message */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_PDMSG_SEND;
			pca9468->timer_period = 0;
			mutex_unlock(&pca9468->lock);
		}
	}
	schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));

	return 0;
}


/* Set new input current  */
static int pca9468_set_new_iin(struct pca9468_charger *pca9468)
{
	int ret = 0;

	pr_info("%s: new_iin=%d\n", __func__, pca9468->new_iin);

	/* Check the charging state */
	if (pca9468->charging_state == DC_STATE_NO_CHARGING) {
		/* Apply new iin when the direct charging is started */
		pca9468->pdata->iin_cfg = pca9468->new_iin;
	} else {
		/* Check whether the previous request is done or not */
		if (pca9468->req_new_iin == true) {
			/* The previous request is not done yet */
			pr_err("%s: There is the previous request for New iin\n", __func__);
			ret = -EBUSY;
		} else {
			/* Set request flag */
			mutex_lock(&pca9468->lock);
			pca9468->req_new_iin = true;
			mutex_unlock(&pca9468->lock);

			/* Check the charging state */
			if ((pca9468->charging_state == DC_STATE_CC_MODE) ||
				(pca9468->charging_state == DC_STATE_CV_MODE)) {
				/* cancel delayed_work */
				cancel_delayed_work(&pca9468->timer_work);

				/* Set new IIN to IIN_CC */
				pca9468->iin_cc = pca9468->new_iin;

				/* Save return state */
				pca9468->ret_state = pca9468->charging_state;

				/* Check new IIN with the minimum TA current */
				if (pca9468->iin_cc < (PCA9468_TA_MIN_CUR * pca9468->ta_mode)) {
					/* Set the TA current to PCA9468_TA_MIN_CUR(1.0A) */
					pca9468->ta_cur = PCA9468_TA_MIN_CUR;
					/* Need to control TA voltage for request current */
					ret = pca9468_adjust_ta_voltage(pca9468);
				} else {
					/* Need to control TA current for request current */
					ret = pca9468_adjust_ta_current(pca9468);
				}
			} else if (pca9468->charging_state == DC_STATE_WC_CV_MODE) {
				/* Charging State is WC CV state */
				/* cancel delayed_work */
				cancel_delayed_work(&pca9468->timer_work);

				/* Set IIN_CFG to new iin */
				pca9468->pdata->iin_cfg = pca9468->new_iin;
				ret = pca9468_set_input_current(pca9468, pca9468->pdata->iin_cfg);
				if (ret < 0)
					goto error;

				/* Clear req_new_iin */					
				mutex_lock(&pca9468->lock);
				pca9468->req_new_iin = false;
				mutex_unlock(&pca9468->lock);

				/* Set IIN_CC to new iin */
				pca9468->iin_cc = pca9468->new_iin;

				pr_info("%s: WC CV state, New IIN=%d\n", __func__, pca9468->iin_cc);

				/* Go to WC CV mode */
				pca9468->charging_state = DC_STATE_WC_CV_MODE;
				mutex_lock(&pca9468->lock);
				pca9468->timer_id = TIMER_CHECK_WCCVMODE;
				pca9468->timer_period = PCA9468_CVMODE_CHECK2_T;
				mutex_unlock(&pca9468->lock);
				schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
			} else {
				/* Wait for next valid state */
				pr_info("%s: Not support new iin yet in charging state=%d\n", __func__, pca9468->charging_state);
			}
		}
	}

error:
	pr_info("%s: ret=%d\n", __func__, ret);
	return ret;
}


/* Set new float voltage */
static int pca9468_set_new_vfloat(struct pca9468_charger *pca9468)
{
	int ret = 0;
	int vbat;
	unsigned int val;
	
	/* Check the charging state */
	if (pca9468->charging_state == DC_STATE_NO_CHARGING) {
		/* Apply new vfloat when the direct charging is started */
		pca9468->pdata->v_float = pca9468->new_vfloat;
	} else {
		/* Check whether the previous request is done or not */
		if (pca9468->req_new_vfloat == true) {
			/* The previous request is not done yet */
			pr_err("%s: There is the previous request for New vfloat\n", __func__);
			ret = -EBUSY;
		} else {
			/* Set request flag */
			mutex_lock(&pca9468->lock);
			pca9468->req_new_vfloat = true;
			mutex_unlock(&pca9468->lock);

			/* Check the charging state */
			if ((pca9468->charging_state == DC_STATE_CC_MODE) ||
				(pca9468->charging_state == DC_STATE_CV_MODE)) {
				/* Read VBAT ADC */
				vbat = pca9468_read_adc(pca9468, ADCCH_VBAT);
				/* Compare the new VBAT with the current VBAT */
				if (pca9468->new_vfloat > vbat) {
					/* cancel delayed_work */
					cancel_delayed_work(&pca9468->timer_work);
					
					/* Set VFLOAT to new vfloat */
					pca9468->pdata->v_float = pca9468->new_vfloat;
					ret = pca9468_set_vfloat(pca9468, pca9468->pdata->v_float);
					if (ret < 0)
						goto error;
					pca9468->pdata->iin_cfg = pca9468->iin_cc;	/* save the current iin_cc in iin_cfg */
					/* Set IIN_CFG to the current IIN_CC */
					pca9468->pdata->iin_cfg = MIN(pca9468->pdata->iin_cfg, pca9468->ta_max_cur*pca9468->ta_mode);
					ret = pca9468_set_input_current(pca9468, pca9468->pdata->iin_cfg);
					if (ret < 0)
						goto error;
					pca9468->iin_cc = pca9468->pdata->iin_cfg;
					
					/* Clear req_new_vfloat */					
					mutex_lock(&pca9468->lock);
					pca9468->req_new_vfloat = false;
					mutex_unlock(&pca9468->lock);

					/* Calculate new TA maximum voltage that used in the direct charging */
					/* Calculate new TA max voltage */
					/* Adjust IIN_CC with APDO resoultion(50mA) - It will recover to the original value after max voltage calculation */
					val = pca9468->iin_cc/(PD_MSG_TA_CUR_STEP*pca9468->ta_mode);
					pca9468->iin_cc = val*(PD_MSG_TA_CUR_STEP*pca9468->ta_mode);
					/* Set TA_MAX_VOL to MIN[PCA9468_TA_MAX_VOL, (TA_MAX_PWR/IIN_CC)] */
					val = pca9468->ta_max_pwr/(pca9468->iin_cc/pca9468->ta_mode/1000);	/* mV */
					val = val*1000/PD_MSG_TA_VOL_STEP;	/* uV */
					val = val*PD_MSG_TA_VOL_STEP; /* Adjust values with APDO resolution(20mV) */
					pca9468->ta_max_vol = MIN(val, PCA9468_TA_MAX_VOL*pca9468->ta_mode);
								
					/* Set TA voltage to MAX[8000mV*TA_mode, (2*VBAT_ADC*TA_mode + 500 mV)] */
					pca9468->ta_vol = max(PCA9468_TA_MIN_VOL_PRESET*pca9468->ta_mode, (2*vbat*pca9468->ta_mode + PCA9468_TA_VOL_PRE_OFFSET));
					val = pca9468->ta_vol/PD_MSG_TA_VOL_STEP;	/* PPS voltage resolution is 20mV */
					pca9468->ta_vol = val*PD_MSG_TA_VOL_STEP;
					/* Set TA voltage to MIN[TA voltage, TA_MAX_VOL] */
					pca9468->ta_vol = MIN(pca9468->ta_vol, pca9468->ta_max_vol);
					/* Set TA current to IIN_CC */
					pca9468->ta_cur = pca9468->iin_cc/pca9468->ta_mode;
					/* Recover IIN_CC to the original value(iin_cfg) */
					pca9468->iin_cc = pca9468->pdata->iin_cfg;

					pr_info("%s: New VFLOAT, ta_max_vol=%d, ta_max_cur=%d, ta_max_pwr=%d, iin_cc=%d, ta_mode=%d\n", 
						__func__, pca9468->ta_max_vol, pca9468->ta_max_cur, pca9468->ta_max_pwr, pca9468->iin_cc, pca9468->ta_mode);

					/* Clear previous IIN ADC */
					pca9468->prev_iin = 0;
					/* Clear TA increment flag */
					pca9468->prev_inc = INC_NONE;

					/* Send PD Message and go to Adjust CC mode */
					pca9468->charging_state = DC_STATE_ADJUST_CC;
					mutex_lock(&pca9468->lock);
					pca9468->timer_id = TIMER_PDMSG_SEND;
					pca9468->timer_period = 0;
					mutex_unlock(&pca9468->lock);
					schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
				} else {
					/* The new VBAT is lower than the current VBAT */
					/* return invalid error */
#if defined(CONFIG_BATTERY_SAMSUNG)
					pr_err("## %s: New vfloat(%duA) is lower than VBAT ADC(%duA)\n",
							__func__, pca9468->new_vfloat, vbat);
#else
					pr_err("%s: New vfloat is lower than VBAT ADC\n", __func__);
#endif
					ret = -EINVAL;
				}
			} else if (pca9468->charging_state == DC_STATE_WC_CV_MODE) {
				/* Charging State is WC CV state */
				/* Read VBAT ADC */
				vbat = pca9468_read_adc(pca9468, ADCCH_VBAT);
				/* Compare the new VBAT with the current VBAT */
				if (pca9468->new_vfloat > vbat) {
					/* cancel delayed_work */
					cancel_delayed_work(&pca9468->timer_work);
					
					/* Set VFLOAT to new vfloat */
					pca9468->pdata->v_float = pca9468->new_vfloat;
					ret = pca9468_set_vfloat(pca9468, pca9468->pdata->v_float);
					if (ret < 0)
						goto error;

					/* Clear req_new_vfloat */					
					mutex_lock(&pca9468->lock);
					pca9468->req_new_vfloat = false;
					mutex_unlock(&pca9468->lock);

					pr_info("%s: WC CV state, New VFLOAT=%d\n", __func__, pca9468->pdata->v_float);

					/* Go to WC CV mode */
					pca9468->charging_state = DC_STATE_WC_CV_MODE;
					mutex_lock(&pca9468->lock);
					pca9468->timer_id = TIMER_CHECK_WCCVMODE;
					pca9468->timer_period = PCA9468_CVMODE_CHECK2_T;
					mutex_unlock(&pca9468->lock);
					schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
				}
			} else {
				/* Wait for next valid state */
				pr_info("%s: Not support new vfloat yet in charging state=%d\n", __func__, pca9468->charging_state);
			}
		}
	}
error:
	pr_info("%s: ret=%d\n", __func__, ret);
	return ret;
}


/* Check Acitve status */
static int pca9468_check_error(struct pca9468_charger *pca9468)
{
	int ret;
	unsigned int reg_val;
	int vbatt;

#if defined(CONFIG_BATTERY_SAMSUNG)
	pca9468->chg_status = POWER_SUPPLY_STATUS_CHARGING;
	pca9468->health_status = POWER_SUPPLY_HEALTH_GOOD;
#endif

	/* Read STS_B */
	ret = pca9468_read_reg(pca9468, PCA9468_REG_STS_B, &reg_val);
	if (ret < 0)
		goto error;

	/* Read VBAT_ADC */
	vbatt = pca9468_read_adc(pca9468, ADCCH_VBAT);
	
	/* Check Active status */
	if (reg_val & PCA9468_BIT_ACTIVE_STATE_STS) {
		/* PCA9468 is active state */
		/* PCA9468 is in charging */
		/* Check whether the battery voltage is over the minimum voltage level or not */
		if (vbatt > PCA9468_DC_VBAT_MIN) {
			/* Check temperature regulation loop */
			/* Read INT1_STS register */
			ret = pca9468_read_reg(pca9468, PCA9468_REG_INT1_STS, &reg_val);
			if (reg_val & PCA9468_BIT_TEMP_REG_STS) {				
				/* Over temperature protection */
				pr_err("%s: Device is in temperature regulation", __func__);
				ret = -EINVAL;
			} else {
				/* Normal temperature */
				ret = 0;
			}
		} else {
			/* Abnormal battery level */
			pr_err("%s: Error abnormal battery voltage=%d\n", __func__, vbatt);
			ret = -EINVAL;
		}
	} else {
		/* PCA9468 is not active state  - stanby or shutdown */
		/* Stop charging in timer_work */

		/* Read all status register for debugging */
#if defined(CONFIG_BATTERY_SAMSUNG)
		u8 val[8];
		u8 test_val[16];	/* Dump for test register */
#else
		unsigned int val[8];
#endif
		ret = pca9468_bulk_read_reg(pca9468, PCA9468_REG_INT1, &val[PCA9468_REG_INT1], 7);
		pr_err("%s: Error reg[1]=0x%x,[2]=0x%x,[3]=0x%x,[4]=0x%x,[5]=0x%x,[6]=0x%x,[7]=0x%x\n",
			__func__, val[1], val[2], val[3], val[4], val[5], val[6], val[7]);
		/* Read test register for debugging */
		ret = pca9468_bulk_read_reg(pca9468, 0x40, test_val, 16);
		pr_err("%s: Error reg[0x40]=0x%x,[0x41]=0x%x,[0x42]=0x%x,[0x43]=0x%x,[0x44]=0x%x,[0x45]=0x%x,[0x46]=0x%x,[0x47]=0x%x\n",
			__func__, test_val[0], test_val[1], test_val[2], test_val[3], test_val[4], test_val[5], test_val[6], test_val[7]);
		pr_err("%s: Error reg[0x48]=0x%x,[0x49]=0x%x,[0x4A]=0x%x,[0x4B]=0x%x,[0x4C]=0x%x,[0x4D]=0x%x,[0x4E]=0x%x,[0x4F]=0x%x\n",
			__func__, test_val[8], test_val[9], test_val[10], test_val[11], test_val[12], test_val[13], test_val[14], test_val[15]);

		/* Check INT1_STS first */
		if ((val[PCA9468_REG_INT1_STS] & PCA9468_BIT_V_OK_STS) != PCA9468_BIT_V_OK_STS) {
			/* VBUS is invalid */
			pr_err("%s: VOK is invalid", __func__);
			/* Check STS_A */
			if (val[PCA9468_REG_STS_A] & PCA9468_BIT_CFLY_SHORT_STS)
				pr_err("%s: Flying Cap is shorted to GND", __func__);	/* Flying cap is short to GND */
			else if (val[PCA9468_REG_STS_A] & PCA9468_BIT_VOUT_UV_STS)
				pr_err("%s: VOUT UV", __func__);	/* VOUT < VOUT_OK */
			else if (val[PCA9468_REG_STS_A] & PCA9468_BIT_VBAT_OV_STS)
				pr_err("%s: VBAT OV", __func__);	/* VBAT > VBAT_OV */
			else if (val[PCA9468_REG_STS_A] & PCA9468_BIT_VIN_OV_STS)
				pr_err("%s: VIN OV", __func__);		/* VIN > V_OV_FIXED or V_OV_TRACKING */
			else if (val[PCA9468_REG_STS_A] & PCA9468_BIT_VIN_UV_STS)
				pr_err("%s: VIN UV", __func__);		/* VIN < V_UVTH */
			else
				pr_err("%s: Invalid VIN or VOUT", __func__);
		
			ret = -EINVAL;
		} else if (val[PCA9468_REG_INT1_STS] & PCA9468_BIT_NTC_TEMP_STS) {
			/* NTC protection */
			int ntc_adc, ntc_th;
			/* NTC threshold */
			u8 reg_data[2];
			pca9468_bulk_read_reg(pca9468, PCA9468_REG_NTC_TH_1, reg_data, 2);
			ntc_th = ((reg_data[1] & PCA9468_BIT_NTC_THRESHOLD9_8)<<8) | reg_data[0];	/* uV unit */
			/* Read NTC ADC */
			ntc_adc = pca9468_read_adc(pca9468, ADCCH_NTC);	/* uV unit */
			pr_err("%s: NTC Protection, NTC_TH=%d(uV), NTC_ADC=%d(uV)", __func__, ntc_th, ntc_adc);

			ret = -EINVAL;
		} else if (val[PCA9468_REG_INT1_STS] & PCA9468_BIT_CTRL_LIMIT_STS) {
			/* OCP event happens */
			/* Check STS_B */
			if (val[PCA9468_REG_STS_B] & PCA9468_BIT_OCP_FAST_STS)
				pr_err("%s: IIN is over OCP_FAST", __func__); 	/* OCP_dlfeFAST happened */
			else if (val[PCA9468_REG_STS_B] & PCA9468_BIT_OCP_AVG_STS)
				pr_err("%s: IIN is over OCP_AVG", __func__);	/* OCP_AVG happened */
			else
				pr_err("%s: No Loop active", __func__);

			ret = -EINVAL;
		} else if (val[PCA9468_REG_INT1_STS] & PCA9468_BIT_TEMP_REG_STS) {
			/* Over temperature protection */
			pr_err("%s: Device is in temperature regulation", __func__);

			ret = -EINVAL;
		} else if (val[PCA9468_REG_INT1_STS] & PCA9468_BIT_TIMER_STS) {
			/* Check STS_B */
			if (val[PCA9468_REG_STS_B] & PCA9468_BIT_CHARGE_TIMER_STS)
				pr_err("%s: Charger timer is expired", __func__);
			else if (val[PCA9468_REG_STS_B] & PCA9468_BIT_WATCHDOG_TIMER_STS)
				pr_err("%s: Watchdog timer is expired", __func__);
			else
				pr_err("%s: Timer INT, but no timer STS", __func__);

			ret = -EINVAL;
		}
		else if (val[PCA9468_REG_STS_A] & PCA9468_BIT_CFLY_SHORT_STS) {
			/* Flying cap short */
			pr_err("%s: Flying Cap is shorted to GND", __func__);	/* Flying cap is short to GND */
			ret = -EINVAL;
		} else {
			/* There is no error, but not active state */
			if (reg_val & PCA9468_BIT_STANDBY_STATE_STS) {
				/* Standby state */
				/* Check the RCP condition, T_REVI_DET is 300ms */
				/* Wait 200ms */
				msleep(200);

				/* Check the charging state */
				/* Sometimes battery driver might call set_property function to stop charging during msleep
				    At this case, charging state would change DC_STATE_NO_CHARGING.
				    PCA9468 should stop checking RCP condition and exit timer_work
				*/
				if (pca9468->charging_state == DC_STATE_NO_CHARGING) {
					pr_err("%s: other driver forced to stop direct charging\n", __func__);
					ret = -EINVAL;
				} else {
					/* Keep the current charging state */
					/* Check PCA948 state again */
					/* Read STS_B */
					ret = pca9468_read_reg(pca9468, PCA9468_REG_STS_B, &reg_val);
					if (ret < 0)
						goto error;

					pr_info("%s:RCP check, STS_B=0x%x\n",	__func__, reg_val);

					/* Check Active status */
					if (reg_val & PCA9468_BIT_ACTIVE_STATE_STS) {
						/* RCP condition happened, but VIN is still valid */
						/* If VIN is increased, input current will increase over IIN_LOW level */
						/* Normal charging */
						pr_info("%s: RCP happened before, but VIN is valid\n", __func__);
						ret = 0;
					} else if (reg_val & PCA9468_BIT_STANDBY_STATE_STS) {
						/* It is not RCP condition */
						/* Need to retry if DC is in starting state */
						pr_err("%s: Any abnormal condition, retry\n", __func__);
						/* Dump register */
						ret = pca9468_bulk_read_reg(pca9468, PCA9468_REG_INT1, &val[PCA9468_REG_INT1], 7);
						pr_err("%s: Error reg[1]=0x%x,[2]=0x%x,[3]=0x%x,[4]=0x%x,[5]=0x%x,[6]=0x%x,[7]=0x%x\n",
								__func__, val[1], val[2], val[3], val[4], val[5], val[6], val[7]);
						ret = pca9468_bulk_read_reg(pca9468, 0x48, test_val, 3);
						pr_err("%s: Error reg[0x48]=0x%x,[0x49]=0x%x,[0x4a]=0x%x\n",
								__func__, test_val[0], test_val[1], test_val[2]);
						ret = -EAGAIN;
					} else {
						/* Shutdown State */
						pr_err("%s: Shutdown state\n", __func__);
						/* Dump register */
						ret = pca9468_bulk_read_reg(pca9468, PCA9468_REG_INT1, &val[PCA9468_REG_INT1], 7);
						pr_err("%s: Error reg[1]=0x%x,[2]=0x%x,[3]=0x%x,[4]=0x%x,[5]=0x%x,[6]=0x%x,[7]=0x%x\n",
								__func__, val[1], val[2], val[3], val[4], val[5], val[6], val[7]);
						ret = pca9468_bulk_read_reg(pca9468, 0x48, test_val, 3);
						pr_err("%s: Error reg[0x48]=0x%x,[0x49]=0x%x,[0x4a]=0x%x\n",
								__func__, test_val[0], test_val[1], test_val[2]);
						ret = -EINVAL;
					}
				}
			} else {
				/* PCA9468 is in shutdown state */
				pr_err("%s: PCA9468 is in shutdown state\n", __func__);
				ret = -EINVAL;
			}
		}
	}

#if defined(CONFIG_BATTERY_SAMSUNG)
	pca9468_test_read(pca9468);
#endif

error:
#if defined(CONFIG_BATTERY_SAMSUNG)
	if (ret == -EINVAL) {
		pca9468->chg_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		pca9468->health_status = POWER_SUPPLY_HEALTH_DC_ERR;
	}
#endif
	pr_info("%s: Active Status=%d\n", __func__, ret);
	return ret;
}


/* Check CC Mode status */
static int pca9468_check_ccmode_status(struct pca9468_charger *pca9468)
{
	unsigned int reg_val;
	int ret, vbat;
	
	/* Read STS_A */
	ret = pca9468_read_reg(pca9468, PCA9468_REG_STS_A, &reg_val);
	if (ret < 0)
		goto error;

	/* Read VBAT ADC */
	vbat = pca9468_read_adc(pca9468, ADCCH_VBAT);

	/* Check STS_A */
	if (reg_val & PCA9468_BIT_VIN_UV_STS) {
		ret = CCMODE_VIN_UVLO;
	} else if (reg_val & PCA9468_BIT_CHG_LOOP_STS) {
		ret = CCMODE_CHG_LOOP;
	} else if (reg_val & PCA9468_BIT_VFLT_LOOP_STS) {
		ret = CCMODE_VFLT_LOOP;
	} else if (reg_val & PCA9468_BIT_IIN_LOOP_STS) {
		ret = CCMODE_IIN_LOOP;
	} else {
		ret = CCMODE_LOOP_INACTIVE;
	}

error:
	pr_info("%s: CCMODE Status=%d\n", __func__, ret);
	return ret;
}


/* Check CVMode Status */
static int pca9468_check_cvmode_status(struct pca9468_charger *pca9468)
{
	unsigned int val;
	int ret, iin;

	if (pca9468->charging_state == DC_STATE_START_CV) {
		/* Read STS_A */
		ret = pca9468_read_reg(pca9468, PCA9468_REG_STS_A, &val);
		if (ret < 0)
			goto error;
		/* Check STS_A */
		if (val & PCA9468_BIT_CHG_LOOP_STS)	{
			ret = CVMODE_CHG_LOOP;
		} else if (val & PCA9468_BIT_VFLT_LOOP_STS) {
			ret = CVMODE_VFLT_LOOP;
		} else if (val & PCA9468_BIT_IIN_LOOP_STS) {
			ret = CVMODE_IIN_LOOP;
		} else if (val & PCA9468_BIT_VIN_UV_STS) {
			ret = CVMODE_VIN_UVLO;
		} else {
			/* Any LOOP is inactive */
			ret = CVMODE_LOOP_INACTIVE;
		}
	} else {
		/* Read IIN ADC */
		iin = pca9468_read_adc(pca9468, ADCCH_IIN);
		if (iin < 0) {
			ret = iin;
			goto error;
		}
		
		/* Check IIN < Input Topoff current */
		if (pca9468->pdata->v_float == pca9468->pdata->v_float_max &&
			iin < pca9468->pdata->iin_topoff) {
			/* Direct Charging Done */
			ret = CVMODE_CHG_DONE;
		} else {
			/* It doesn't reach top-off condition yet */
			
			/* Read STS_A */
			ret = pca9468_read_reg(pca9468, PCA9468_REG_STS_A, &val);
			if (ret < 0)
				goto error;
			/* Check STS_A */
			if (val & PCA9468_BIT_CHG_LOOP_STS) {
				ret = CVMODE_CHG_LOOP;
			} else if (val & PCA9468_BIT_VFLT_LOOP_STS) {
				ret = CVMODE_VFLT_LOOP;
			} else if (val & PCA9468_BIT_IIN_LOOP_STS) {
				ret = CVMODE_IIN_LOOP;
			} else if (val & PCA9468_BIT_VIN_UV_STS) {
				ret = CVMODE_VIN_UVLO;
			} else {
				/* Any LOOP is inactive */
				ret = CVMODE_LOOP_INACTIVE;
			}
		}
	}
	
error:
	pr_info("%s: CVMODE Status=%d\n", __func__, ret);
	return ret;
}


/* 2:1 Direct Charging Adjust CC MODE control */
static int pca9468_charge_adjust_ccmode(struct pca9468_charger *pca9468)
{
	int iin, ccmode;
	int vbatt;
	int vin_vol, val;
	int ret = 0;

	pr_info("%s: ======START=======\n", __func__);

#if defined(CONFIG_BATTERY_SAMSUNG)
	pca9468_set_charging_state(pca9468, DC_STATE_ADJUST_CC);
#else
	pca9468->charging_state = DC_STATE_ADJUST_CC;
#endif

	ret = pca9468_check_error(pca9468);
	if (ret != 0)
		goto error;	// This is not active mode.

	ccmode = pca9468_check_ccmode_status(pca9468);
	if (ccmode < 0) {
		ret = ccmode;
		goto error;
	}
	
	switch(ccmode) {
	case CCMODE_IIN_LOOP:
	case CCMODE_CHG_LOOP:
		/* Check TA current */
		if (pca9468->ta_cur > PCA9468_TA_MIN_CUR) {
			/* TA current is higher than 1.0A */
			/* Decrease TA current (50mA) */
			pca9468->ta_cur = pca9468->ta_cur - PD_MSG_TA_CUR_STEP;
		}
		pr_info("%s: CC adjust End(LOOP): ta_cur=%d, ta_vol=%d\n", __func__, pca9468->ta_cur, pca9468->ta_vol);
		/* Read VBAT ADC */
		vbatt = pca9468_read_adc(pca9468, ADCCH_VBAT);
		/* TA target voltage = TA voltage + (VFLOAT - VBAT_ADC)*2*TA_mode + 100mV */
		val = pca9468->ta_vol + (pca9468->pdata->v_float - vbatt)*2*pca9468->ta_mode + 100000;
		val = val/PD_MSG_TA_VOL_STEP;
		pca9468->ta_target_vol = val*PD_MSG_TA_VOL_STEP;
		if (pca9468->ta_target_vol > pca9468->ta_max_vol)
			pca9468->ta_target_vol = pca9468->ta_max_vol;
		/* End TA voltage and current adjustment */
		pr_info("%s: CC adjust End(LOOP): ta_target_vol=%d\n", __func__, pca9468->ta_target_vol);
		/* Clear TA increment flag */
		pca9468->prev_inc = INC_NONE;
		/* Send PD Message and go to Start CC mode */
#if defined(CONFIG_BATTERY_SAMSUNG)
		pca9468_set_charging_state(pca9468, DC_STATE_START_CC);
#else
		pca9468->charging_state = DC_STATE_START_CC;
#endif
		mutex_lock(&pca9468->lock);
		pca9468->timer_id = TIMER_PDMSG_SEND;
		pca9468->timer_period = 0;
		mutex_unlock(&pca9468->lock);
		schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
		break;
		
	case CCMODE_VFLT_LOOP:
		/* Save TA target and voltage*/
		pca9468->ta_target_vol = pca9468->ta_vol;
		pr_info("%s: CC adjust End(VFLOAT): ta_target_vol=%d\n", __func__, pca9468->ta_target_vol);
		/* Clear TA increment flag */
		pca9468->prev_inc = INC_NONE;
		/* Go to Pre-CV mode */
		mutex_lock(&pca9468->lock);
		pca9468->timer_id = TIMER_ENTER_CVMODE;
		pca9468->timer_period = 0;
		mutex_unlock(&pca9468->lock);
		schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
		break;

	case CCMODE_LOOP_INACTIVE:
		/* Check IIN ADC with IIN */
		iin = pca9468_read_adc(pca9468, ADCCH_IIN);
		/* IIN_ADC > IIN_CC -20mA ? */
		if (iin > (pca9468->iin_cc - PCA9468_IIN_ADC_OFFSET)) {
			/* Input current is already over IIN_CC */
			/* End TA voltage and current adjustment */
			/* change charging state to Start CC mode */
#if defined(CONFIG_BATTERY_SAMSUNG)
			pca9468_set_charging_state(pca9468, DC_STATE_START_CC);
#else
			pca9468->charging_state = DC_STATE_START_CC;
#endif
			/* Read VBAT ADC */
			vbatt = pca9468_read_adc(pca9468, ADCCH_VBAT);
			/* TA target voltage = TA voltage + (VFLOAT - VBAT_ADC)*2*TA_mode + 100mV */
			val = pca9468->ta_vol + (pca9468->pdata->v_float - vbatt)*2*pca9468->ta_mode + 100000;
			val = val/PD_MSG_TA_VOL_STEP;
			pca9468->ta_target_vol = val*PD_MSG_TA_VOL_STEP;
			if (pca9468->ta_target_vol > pca9468->ta_max_vol)
				pca9468->ta_target_vol = pca9468->ta_max_vol;
			pr_info("%s: CC adjust End: IIN_ADC=%d, ta_target_vol=%d\n", __func__, iin, pca9468->ta_target_vol);
			/* Clear TA increment flag */
			pca9468->prev_inc = INC_NONE;
			/* Go to Start CC mode */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_ENTER_CCMODE;
			pca9468->timer_period = 0;
			mutex_unlock(&pca9468->lock);
		} else {
			/* Check TA voltage */
			if (pca9468->ta_vol == pca9468->ta_max_vol) {
				/* TA voltage is already max value */
				pr_info("%s: CC adjust End: MAX value, ta_vol=%d, ta_cur=%d\n", 
					__func__, pca9468->ta_vol, pca9468->ta_cur);
				/* Clear TA increment flag */
				pca9468->prev_inc = INC_NONE;
				/* Save TA target voltage */
				pca9468->ta_target_vol = pca9468->ta_vol;
				/* Go to CC mode */
				mutex_lock(&pca9468->lock);
				pca9468->timer_id = TIMER_CHECK_CCMODE;
				pca9468->timer_period = 0;
				mutex_unlock(&pca9468->lock);
			} else {
				/* Check TA tolerance */
				/* The current input current compares the final input current(IIN_CC) with 100mA offset */
				/* PPS current tolerance has +/-150mA, so offset defined 100mA(tolerance +50mA) */
				if (iin < (pca9468->iin_cc - PCA9468_TA_IIN_OFFSET)) {
					/* TA voltage too low to enter TA CC mode, so we should increae TA voltage */
					pca9468->ta_vol = pca9468->ta_vol + PCA9468_TA_VOL_STEP_ADJ_CC*pca9468->ta_mode;
					if (pca9468->ta_vol > pca9468->ta_max_vol)
						pca9468->ta_vol = pca9468->ta_max_vol;
					pr_info("%s: CC adjust Cont: ta_vol=%d\n", __func__, pca9468->ta_vol);
					/* Set TA increment flag */
					pca9468->prev_inc = INC_TA_VOL;
					/* Send PD Message */
					mutex_lock(&pca9468->lock);
					pca9468->timer_id = TIMER_PDMSG_SEND;
					pca9468->timer_period = 0;
					mutex_unlock(&pca9468->lock);
				} else {
					/* compare IIN ADC with previous IIN ADC + 20mA */
					if (iin > (pca9468->prev_iin + PCA9468_IIN_ADC_OFFSET)) {
						/* TA can supply more current if TA voltage is high */
						/* TA voltage too low to enter TA CC mode, so we should increae TA voltage */
						pca9468->ta_vol = pca9468->ta_vol + PCA9468_TA_VOL_STEP_ADJ_CC*pca9468->ta_mode;
						if (pca9468->ta_vol > pca9468->ta_max_vol)
							pca9468->ta_vol = pca9468->ta_max_vol;
						pr_info("%s: CC adjust Cont: ta_vol=%d\n", __func__, pca9468->ta_vol);
						/* Set TA increment flag */
						pca9468->prev_inc = INC_TA_VOL;
						/* Send PD Message */
						mutex_lock(&pca9468->lock);
						pca9468->timer_id = TIMER_PDMSG_SEND;
						pca9468->timer_period = 0;
						mutex_unlock(&pca9468->lock);
					} else {
						/* Check the previous increment */
						if (pca9468->prev_inc == INC_TA_CUR) {
							/* The previous increment is TA current, but input current does not increase */
							/* Try to increase TA voltage(40mV) */
							pca9468->ta_vol = pca9468->ta_vol + PCA9468_TA_VOL_STEP_ADJ_CC*pca9468->ta_mode;;
							if (pca9468->ta_vol > pca9468->ta_max_vol)
								pca9468->ta_vol = pca9468->ta_max_vol;
							pr_info("%s: CC adjust(flag) Cont: ta_vol=%d\n", __func__, pca9468->ta_vol);
							/* Set TA increment flag */
							pca9468->prev_inc = INC_TA_VOL;
							/* Send PD Message */
							mutex_lock(&pca9468->lock);
							pca9468->timer_id = TIMER_PDMSG_SEND;
							pca9468->timer_period = 0;
							mutex_unlock(&pca9468->lock);
						} else {
							/* The previous increment is TA voltage, but input current does not increase */
							/* Try to increase TA current */				
							/* Check APDO max current */
							if (pca9468->ta_cur == pca9468->ta_max_cur) {
								/* TA current is maximum current */
								/* Read VBAT ADC */
								vbatt = pca9468_read_adc(pca9468, ADCCH_VBAT);
								/* TA target voltage = TA voltage + (VFLOAT - VBAT_ADC)*2*TA_mode + 100mV */
								val = pca9468->ta_vol + (pca9468->pdata->v_float - vbatt)*2*pca9468->ta_mode + 100000;
								val = val/PD_MSG_TA_VOL_STEP;
								pca9468->ta_target_vol = val*PD_MSG_TA_VOL_STEP;
								if (pca9468->ta_target_vol > pca9468->ta_max_vol)
									pca9468->ta_target_vol = pca9468->ta_max_vol;
								pr_info("%s: CC adjust End(MAX_CUR): IIN_ADC=%d, ta_target_vol=%d\n", __func__, iin, pca9468->ta_target_vol);
								/* Clear TA increment flag */
								pca9468->prev_inc = INC_NONE;
								/* Go to Start CC mode */
								mutex_lock(&pca9468->lock);
								pca9468->timer_id = TIMER_ENTER_CCMODE;
								pca9468->timer_period = 0;
								mutex_unlock(&pca9468->lock);
							} else {
								/* TA has tolerance and compensate it as real current */
								/* Increase TA current(50mA) */
								pca9468->ta_cur = pca9468->ta_cur + PD_MSG_TA_CUR_STEP;
								if (pca9468->ta_cur > pca9468->ta_max_cur)
									pca9468->ta_cur = pca9468->ta_max_cur;
								pr_info("%s: CC adjust Cont: ta_cur=%d\n", __func__, pca9468->ta_cur);
								/* Set TA increment flag */
								pca9468->prev_inc = INC_TA_CUR;
								/* Send PD Message */
								mutex_lock(&pca9468->lock);
								pca9468->timer_id = TIMER_PDMSG_SEND;
								pca9468->timer_period = 0;
								mutex_unlock(&pca9468->lock);
							}
						}
					}
				}
			}
		}
		/* Save previous iin adc */
		pca9468->prev_iin = iin;
		schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
		break;

	case CCMODE_VIN_UVLO:
		/* VIN UVLO - just notification , it works by hardware */
		vin_vol = pca9468_read_adc(pca9468, ADCCH_VIN);
		pr_info("%s: CC adjust VIN_UVLO: ta_vol=%d, vin_vol=%d\n", __func__, pca9468->ta_cur, vin_vol);
		/* Check VIN after 1sec */
		mutex_lock(&pca9468->lock);
		pca9468->timer_id = TIMER_ADJUST_CCMODE;
		pca9468->timer_period = 1000;
		mutex_unlock(&pca9468->lock);
		schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
		break;

	default:
		break;
	}
	
error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* 2:1 Direct Charging Start CC MODE control - Pre CC MODE */
/* Increase TA voltage to TA target voltage */
static int pca9468_charge_start_ccmode(struct pca9468_charger *pca9468)
{
	int ret = 0;
	
	pr_info("%s: ======START=======\n", __func__);

#if defined(CONFIG_BATTERY_SAMSUNG)
	pca9468_set_charging_state(pca9468, DC_STATE_START_CC);
#else
	pca9468->charging_state = DC_STATE_START_CC;
#endif

	/* Increase TA voltage */
	pca9468->ta_vol = pca9468->ta_vol + PCA9468_TA_VOL_STEP_PRE_CC * pca9468->ta_mode;
	/* Check TA target voltage */
	if (pca9468->ta_vol >= pca9468->ta_target_vol) {
		pca9468->ta_vol = pca9468->ta_target_vol;
		pr_info("%s: PreCC END: ta_vol=%d\n", __func__, pca9468->ta_vol);

		/* Change to DC state to CC mode */
		pca9468->charging_state = DC_STATE_CC_MODE;
	} else {
		pr_info("%s: PreCC: ta_vol=%d\n", __func__, pca9468->ta_vol);
	}

	/* Send PD Message */
	mutex_lock(&pca9468->lock);
	pca9468->timer_id = TIMER_PDMSG_SEND;
	pca9468->timer_period = 0;
	mutex_unlock(&pca9468->lock);
	schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));

	return ret;
}

/* 2:1 Direct Charging CC MODE control */
static int pca9468_charge_ccmode(struct pca9468_charger *pca9468)
{
	int ret = 0;
	int ccmode;
	int vin_vol, iin;
	
	pr_info("%s: ======START=======\n", __func__);

#if defined(CONFIG_BATTERY_SAMSUNG)
	pca9468_set_charging_state(pca9468, DC_STATE_CC_MODE);
#else
	pca9468->charging_state = DC_STATE_CC_MODE;
#endif

	ret = pca9468_check_error(pca9468);
	if (ret != 0)
		goto error;	// This is not active mode.

	/* Check new vfloat request and new iin request */
	if (pca9468->req_new_vfloat == true) {
		/* Clear request flag */
		mutex_lock(&pca9468->lock);
		pca9468->req_new_vfloat = false;
		mutex_unlock(&pca9468->lock);
		ret = pca9468_set_new_vfloat(pca9468);
	} else if (pca9468->req_new_iin == true) {
		/* Clear request flag */
		mutex_lock(&pca9468->lock);
		pca9468->req_new_iin = false;
		mutex_unlock(&pca9468->lock);
		ret = pca9468_set_new_iin(pca9468);
	} else {
		ccmode = pca9468_check_ccmode_status(pca9468);
		if (ccmode < 0) {
			ret = ccmode;
			goto error;
		}
		
		switch(ccmode) {
		case CCMODE_LOOP_INACTIVE:
			/* Set input current compensation */
			/* Check the current TA current with TA_MIN_CUR */
			if (pca9468->ta_cur <= PCA9468_TA_MIN_CUR) {
				/* Set TA current to PCA9468_TA_MIN_CUR(1.0A) */
				pca9468->ta_cur = PCA9468_TA_MIN_CUR;
				/* Need input voltage compensation */
				ret = pca9468_set_ta_voltage_comp(pca9468);
			} else {
				/* Check TA_MAX_VOL for detecting constant power mode */
				if (pca9468->ta_max_vol >= (PCA9468_TA_MAX_VOL_CP * pca9468->ta_mode)) {
					/* Need input current compensation */
					ret = pca9468_set_ta_current_comp(pca9468);
				} else {
					/* Need input current compensation in constant power mode */
					ret = pca9468_set_ta_current_comp2(pca9468);
				}
			}
			pr_info("%s: CC INACTIVE: ta_cur=%d, ta_vol=%d\n", __func__, pca9468->ta_cur, pca9468->ta_vol);
			break;

		case CCMODE_VFLT_LOOP:
			/* Read IIN_ADC */
			iin = pca9468_read_adc(pca9468, ADCCH_IIN);
			pr_info("%s: CC VFLOAT: iin=%d\n", __func__, iin);
			/* go to Pre-CV mode */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_ENTER_CVMODE;
			pca9468->timer_period = 0;
			mutex_unlock(&pca9468->lock);
			schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
			break;

		case CCMODE_IIN_LOOP:
		case CCMODE_CHG_LOOP:
			/* Read IIN_ADC */
			iin = pca9468_read_adc(pca9468, ADCCH_IIN);
			/* Check the current TA current with TA_MIN_CUR */
			if (pca9468->ta_cur <= PCA9468_TA_MIN_CUR) {
				/* Decrease TA voltage (20mV) */
				pca9468->ta_vol = pca9468->ta_vol - PD_MSG_TA_VOL_STEP;
				pr_info("%s: CC LOOP:iin=%d, next_ta_vol=%d\n", __func__, iin, pca9468->ta_vol);
			} else {
				/* Decrease TA current (50mA) */
				pca9468->ta_cur = pca9468->ta_cur - PD_MSG_TA_CUR_STEP;
				pr_info("%s: CC LOOP:iin=%d, next_ta_cur=%d\n", __func__, iin, pca9468->ta_cur);
			}
			/* Send PD Message */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_PDMSG_SEND;
			pca9468->timer_period = 0;
			mutex_unlock(&pca9468->lock);
			schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
			break;

		case CCMODE_VIN_UVLO:
			/* VIN UVLO - just notification , it works by hardware */
			vin_vol = pca9468_read_adc(pca9468, ADCCH_VIN);
			pr_info("%s: CC VIN_UVLO: ta_vol=%d, vin_vol=%d\n", __func__, pca9468->ta_cur, vin_vol);
			/* Check VIN after 1sec */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_CHECK_CCMODE;
			pca9468->timer_period = 1000;
			mutex_unlock(&pca9468->lock);
			schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
			break;

		default:
			break;
		}
	}
error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}


/* 2:1 Direct Charging Start CV MODE control - Pre CV MODE */
static int pca9468_charge_start_cvmode(struct pca9468_charger *pca9468)
{
	int ret = 0;
	int cvmode;
	int vin_vol;

	pr_info("%s: ======START=======\n", __func__);

#if defined(CONFIG_BATTERY_SAMSUNG)
	pca9468_set_charging_state(pca9468, DC_STATE_START_CV);
#else
	pca9468->charging_state = DC_STATE_START_CV;
#endif

	ret = pca9468_check_error(pca9468);
	if (ret != 0)
		goto error;	// This is not active mode.

	cvmode = pca9468_check_cvmode_status(pca9468);
	if (cvmode < 0) {
		ret = cvmode;
		goto error;
	}
	
	switch(cvmode) {
	case CVMODE_CHG_LOOP:
	case CVMODE_IIN_LOOP:
		/* Check TA current */
		if (pca9468->ta_cur > PCA9468_TA_MIN_CUR) {
			/* TA current is higher than 1.0A */
			/* Decrease TA current (50mA) */
			pca9468->ta_cur = pca9468->ta_cur - PD_MSG_TA_CUR_STEP;
			pr_info("%s: PreCV Cont: ta_cur=%d\n", __func__, pca9468->ta_cur);
		} else {
			/* TA current is less than 1.0A */
			/* Decrease TA voltage (20mV) */			
			pca9468->ta_vol = pca9468->ta_vol - PD_MSG_TA_VOL_STEP;
			pr_info("%s: PreCV Cont(IIN_LOOP): ta_vol=%d\n", __func__, pca9468->ta_vol);
		}
		/* Send PD Message */
		mutex_lock(&pca9468->lock);
		pca9468->timer_id = TIMER_PDMSG_SEND;
		pca9468->timer_period = 0;
		mutex_unlock(&pca9468->lock);
		schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
		break;

	case CVMODE_VFLT_LOOP:
		/* Decrease TA voltage (20mV) */
		pca9468->ta_vol = pca9468->ta_vol - PCA9468_TA_VOL_STEP_PRE_CV * pca9468->ta_mode;
		pr_info("%s: PreCV Cont: ta_vol=%d\n", __func__, pca9468->ta_vol);
		/* Send PD Message */
		mutex_lock(&pca9468->lock);
		pca9468->timer_id = TIMER_PDMSG_SEND;
		pca9468->timer_period = 0;
		mutex_unlock(&pca9468->lock);
		schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
		break;

	case CVMODE_LOOP_INACTIVE:
		/* Exit Pre CV mode */
		pr_info("%s: PreCV End: ta_vol=%d, ta_cur=%d\n", __func__, pca9468->ta_vol, pca9468->ta_cur);

		/* Set TA target voltage to TA voltage */
		pca9468->ta_target_vol = pca9468->ta_vol;
		
		/* Need to implement notification to other driver */
		/* To do here */
		
		/* Go to CV mode */
		mutex_lock(&pca9468->lock);
		pca9468->timer_id = TIMER_CHECK_CVMODE;
		pca9468->timer_period = 0;
		mutex_unlock(&pca9468->lock);
		schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
		break;

	case CVMODE_VIN_UVLO:
		/* VIN UVLO - just notification , it works by hardware */
		vin_vol = pca9468_read_adc(pca9468, ADCCH_VIN);
		pr_info("%s: PreCV VIN_UVLO: ta_vol=%d, vin_vol=%d\n", __func__, pca9468->ta_cur, vin_vol);
		/* Check VIN after 1sec */
		mutex_lock(&pca9468->lock);
		pca9468->timer_id = TIMER_ENTER_CVMODE;
		pca9468->timer_period = 1000;
		mutex_unlock(&pca9468->lock);
		schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
		break;
		
	default:
		break;
	}
	
error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* 2:1 Direct Charging CV MODE control */
static int pca9468_charge_cvmode(struct pca9468_charger *pca9468)
{
	int ret = 0;
	int cvmode;
	int vin_vol;

	pr_info("%s: ======START=======\n", __func__);

#if defined(CONFIG_BATTERY_SAMSUNG)
	pca9468_set_charging_state(pca9468, DC_STATE_CV_MODE);
#else
	pca9468->charging_state = DC_STATE_CV_MODE;
#endif

	ret = pca9468_check_error(pca9468);
	if (ret != 0)
		goto error;	// This is not active mode.

	/* Check new vfloat request and new iin request */
	if (pca9468->req_new_vfloat == true) {
		/* Clear request flag */
		mutex_lock(&pca9468->lock);
		pca9468->req_new_vfloat = false;
		mutex_unlock(&pca9468->lock);
		ret = pca9468_set_new_vfloat(pca9468);
	} else if (pca9468->req_new_iin == true) {
		/* Clear request flag */
		mutex_lock(&pca9468->lock);
		pca9468->req_new_iin = false;
		mutex_unlock(&pca9468->lock);
		ret = pca9468_set_new_iin(pca9468);
	} else {	
		cvmode = pca9468_check_cvmode_status(pca9468);
		if (cvmode < 0) {
			ret = cvmode;
			goto error;
		}
		
		switch(cvmode) {
		case CVMODE_CHG_DONE:
			/* Charging Done */
			/* Keep CV mode until battery driver send stop charging */
			
			/* Need to implement notification function */
			/* To do here */	

			pr_info("%s: CV Done\n", __func__);
#if defined(CONFIG_BATTERY_SAMSUNG)
			pca9468_set_done(pca9468, true);
#endif

			/* Check the charging status after notification function */
			if(pca9468->charging_state != DC_STATE_NO_CHARGING) {
				/* Notification function does not stop timer work yet */
				/* Keep the charging done state */
				/* Set timer */
				mutex_lock(&pca9468->lock);
				pca9468->timer_id = TIMER_CHECK_CVMODE;
				pca9468->timer_period = PCA9468_CVMODE_CHECK_T;
				mutex_unlock(&pca9468->lock);
				schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
			} else {
				/* Already called stop charging by notification function */
				pr_info("%s: Already stop DC\n", __func__);
			}
			break;

		case CVMODE_CHG_LOOP:
		case CVMODE_IIN_LOOP:
			/* Check TA current */
			if (pca9468->ta_cur > PCA9468_TA_MIN_CUR) {
				/* TA current is higher than (1.0A*TA_mode) */
				/* Decrease TA current (50mA) */
				pca9468->ta_cur = pca9468->ta_cur - PD_MSG_TA_CUR_STEP;
				pr_info("%s: CV LOOP, Cont: ta_cur=%d\n", __func__, pca9468->ta_cur);
			} else {
				/* TA current is less than (1.0A*TA_mode) */
				/* Decrease TA Voltage (20mV) */
				pca9468->ta_vol = pca9468->ta_vol - PD_MSG_TA_VOL_STEP;
				pr_info("%s: CV LOOP, Cont: ta_vol=%d\n", __func__, pca9468->ta_vol);
			}
			/* Send PD Message */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_PDMSG_SEND;
			pca9468->timer_period = 0;
			mutex_unlock(&pca9468->lock);
			schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
			break;

		case CVMODE_VFLT_LOOP:
			/* Decrease TA voltage */
			pca9468->ta_vol = pca9468->ta_vol - PD_MSG_TA_VOL_STEP;
			pr_info("%s: CV VFLOAT, Cont: ta_vol=%d\n", __func__, pca9468->ta_vol);

			/* Set TA target voltage to TA voltage */
			pca9468->ta_target_vol = pca9468->ta_vol;
			
			/* Send PD Message */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_PDMSG_SEND;
			pca9468->timer_period = 0;
			mutex_unlock(&pca9468->lock);
			schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
			break;

		case CVMODE_LOOP_INACTIVE:
			/* Set timer */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_CHECK_CVMODE;
			pca9468->timer_period = PCA9468_CVMODE_CHECK_T;
			mutex_unlock(&pca9468->lock);
			schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
			break;

		case CVMODE_VIN_UVLO:
			/* VIN UVLO - just notification , it works by hardware */
			vin_vol = pca9468_read_adc(pca9468, ADCCH_VIN);
			pr_info("%s: CC VIN_UVLO: ta_vol=%d, vin_vol=%d\n", __func__, pca9468->ta_cur, vin_vol);
			/* Check VIN after 1sec */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_CHECK_CVMODE;
			pca9468->timer_period = 1000;
			mutex_unlock(&pca9468->lock);
			schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
			break;

		default:
			break;
		}
	}

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* 2:1 Direct Charging WC CV MODE control */
static int pca9468_charge_wc_cvmode(struct pca9468_charger *pca9468)
{
	int ret = 0;
	int cvmode;
	int vin_vol;

	pr_info("%s: ======START=======\n", __func__);

	pca9468->charging_state = DC_STATE_WC_CV_MODE;

	ret = pca9468_check_error(pca9468);
	if (ret != 0)
		goto error;	// This is not active mode.

	/* Check new vfloat request and new iin request */
	if (pca9468->req_new_vfloat == true) {
		/* Clear request flag */
		mutex_lock(&pca9468->lock);
		pca9468->req_new_vfloat = false;
		mutex_unlock(&pca9468->lock);
		ret = pca9468_set_new_vfloat(pca9468);
	} else if (pca9468->req_new_iin == true) {
		/* Clear request flag */
		mutex_lock(&pca9468->lock);
		pca9468->req_new_iin = false;
		mutex_unlock(&pca9468->lock);
		ret = pca9468_set_new_iin(pca9468);
	} else {	
		cvmode = pca9468_check_cvmode_status(pca9468);
		if (cvmode < 0) {
			ret = cvmode;
			goto error;
		}
		
		switch(cvmode) {
		case CVMODE_CHG_DONE:
			/* Charging Done */
			/* Keep WC CV mode until battery driver send stop charging */
			
			/* Need to implement notification function */
			/* To do here */	

			pr_info("%s: WC CV Done\n", __func__);

			/* Check the charging status after notification function */
			if(pca9468->charging_state != DC_STATE_NO_CHARGING) {
				/* Notification function does not stop timer work yet */
				/* Keep the charging done state */
				/* Set timer */
				mutex_lock(&pca9468->lock);
				pca9468->timer_id = TIMER_CHECK_WCCVMODE;
				pca9468->timer_period = PCA9468_CVMODE_CHECK_T;
				mutex_unlock(&pca9468->lock);
				schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
			} else {
				/* Already called stop charging by notification function */
				pr_info("%s: Already stop DC\n", __func__);
			}
			break;

		case CVMODE_CHG_LOOP:
		case CVMODE_IIN_LOOP:
			/* IIN_LOOP happens */
			pr_info("%s: WC CV IIN_LOOP\n", __func__);

			/* Need to control WC RX voltage or current */

			/* Need to implement notification function */

			/* To do here */

			/* Set timer - 1s */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_CHECK_WCCVMODE;
			pca9468->timer_period = PCA9468_CVMODE_CHECK2_T;
			mutex_unlock(&pca9468->lock);
			schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
			break;

		case CVMODE_VFLT_LOOP:
			/* VFLOAT_LOOP happens */
			pr_info("%s: WC CV VFLOAT_LOOP\n", __func__);

			/* Need to control WC RX voltage */

			/* Need to implement notification function */

			/* To do here */

			/* Set timer - 1s */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_CHECK_WCCVMODE;
			pca9468->timer_period = PCA9468_CVMODE_CHECK2_T;
			mutex_unlock(&pca9468->lock);
			schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
			break;

		case CVMODE_LOOP_INACTIVE:
			pr_info("%s: WC CV INACTIVE\n", __func__);
			/* Set timer - 10s */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_CHECK_WCCVMODE;
			pca9468->timer_period = PCA9468_CVMODE_CHECK_T;
			mutex_unlock(&pca9468->lock);
			schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
			break;

		case CVMODE_VIN_UVLO:
			/* VIN UVLO - just notification , it works by hardware */
			vin_vol = pca9468_read_adc(pca9468, ADCCH_VIN);
			pr_info("%s: CV VIN_UVLO: vin_vol=%d\n", __func__, vin_vol);
			/* Check VIN after 1sec */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_CHECK_WCCVMODE;
			pca9468->timer_period = PCA9468_CVMODE_CHECK2_T;
			mutex_unlock(&pca9468->lock);
			schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
			break;

		default:
			break;
		}
	}

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* Preset TA voltage and current for Direct Charging Mode */
static int pca9468_preset_dcmode(struct pca9468_charger *pca9468)
{
	int vbat;
	unsigned int val;
	int ret = 0;
	int ta_mode;

	pr_info("%s: ======START=======\n", __func__);

#if defined(CONFIG_BATTERY_SAMSUNG)
	pca9468_set_charging_state(pca9468, DC_STATE_PRESET_DC);
#else
	pca9468->charging_state = DC_STATE_PRESET_DC;
#endif

	/* Read VBAT ADC */
	vbat = pca9468_read_adc(pca9468, ADCCH_VBAT);
	if (vbat < 0) {
	  ret = vbat;
	  goto error;
	}
	
	/* Compare VBAT with VBAT ADC */
	if (vbat > pca9468->pdata->v_float) {
		/* Invalid battery voltage to start direct charging */
#if defined(CONFIG_BATTERY_SAMSUNG)
		pr_err("## %s: vbat adc(%duA) is higher than VFLOAT(%duA)\n",
				__func__, vbat, pca9468->pdata->v_float);
#else
		pr_err("%s: vbat adc is higher than VFLOAT\n", __func__);
#endif
		ret = -EINVAL;
		goto error;
	}

	/* Check the TA mode for wireless charging */
	if (pca9468->ta_mode == WC_DC_MODE) {
		/* Wireless Charger DC mode */
		/* Set IIN_CC to IIN */
		pca9468->iin_cc = pca9468->pdata->iin_cfg;
		/* Go to preset config */
		mutex_lock(&pca9468->lock);
		pca9468->timer_id = TIMER_PRESET_CONFIG;
		pca9468->timer_period = 0;
		mutex_unlock(&pca9468->lock);
		schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
		goto error;
	}

	/* Set the TA max current to input request current(iin_cfg) initially 
		 to get TA maximum current from PD IC */
	pca9468->ta_max_cur = pca9468->pdata->iin_cfg;
	/* Set the TA max voltage to enough high value to find TA maximum voltage initially */
	pca9468->ta_max_vol = PCA9468_TA_MAX_VOL * pca9468->pdata->ta_mode;
	/* Search the proper object position of PDO */
	pca9468->ta_objpos = 0;
	/* Get the APDO max current/voltage(TA_MAX_CUR/VOL) */
	ret = pca9468_get_apdo_max_power(pca9468);
	if (ret < 0) {
		/* TA does not have the desired APDO */
		/* Check the desired mode */
		if (pca9468->pdata->ta_mode == TA_4TO1_DC_MODE) {
			/* TA doesn't have any APDO to support 4:1 mode */
			/* Get the APDO max current/voltage with 2:1 mode */
			pca9468->ta_max_vol = PCA9468_TA_MAX_VOL;
			pca9468->ta_objpos = 0;
			ret = pca9468_get_apdo_max_power(pca9468);
			if (ret < 0) {
				pr_err("%s: TA doesn't have any APDO to support 2:1 or 4:1\n", __func__);
				pca9468->ta_mode = TA_NO_DC_MODE;
				goto error;
			} else {
				/* TA has APDO to support 2:1 mode */
				pca9468->ta_mode = TA_2TO1_DC_MODE;
			}
		} else {
			/* The desired TA mode is 2:1 mode */
			/* TA doesn't have any APDO to support 2:1 mode*/
			pr_err("%s: TA doesn't have any APDO to support 2:1\n", __func__);
			pca9468->ta_mode = TA_NO_DC_MODE;
			goto error;
		}
	} else {
		/* TA has the desired APDO */
		pca9468->ta_mode = pca9468->pdata->ta_mode;
	}
	
	ta_mode = pca9468->ta_mode;
	
	/* Calculate new TA maximum current and voltage that used in the direct charging */
	/* Set IIN_CC to MIN[IIN, (TA_MAX_CUR by APDO)*TA_mode]*/
	pca9468->iin_cc = MIN(pca9468->pdata->iin_cfg, (pca9468->ta_max_cur*ta_mode));
	/* Set the current IIN_CC to iin_cfg for recovering it after resolution adjustment */
	pca9468->pdata->iin_cfg = pca9468->iin_cc;
	/* Calculate new TA max voltage */
	/* Adjust IIN_CC with APDO resoultion(50mA) - It will recover to the original value after max voltage calculation */
	val = pca9468->iin_cc/PD_MSG_TA_CUR_STEP;
	pca9468->iin_cc = val*PD_MSG_TA_CUR_STEP;
	/* Set TA_MAX_VOL to MIN[PCA9468_TA_MAX_VOL*TA_mode, TA_MAX_PWR/(IIN_CC/TA_mode)] */
	val = pca9468->ta_max_pwr/(pca9468->iin_cc/ta_mode/1000);	/* mV */
	val = val*1000/PD_MSG_TA_VOL_STEP;	/* Adjust values with APDO resolution(20mV) */
	val = val*PD_MSG_TA_VOL_STEP; /* uV */
	pca9468->ta_max_vol = MIN(val, PCA9468_TA_MAX_VOL*ta_mode);
	
	/* Set TA voltage to MAX[8000mV*TA_mode, (2*VBAT_ADC*TA_mode + 500 mV)] */
	pca9468->ta_vol = max(PCA9468_TA_MIN_VOL_PRESET*ta_mode, (2*vbat*ta_mode + PCA9468_TA_VOL_PRE_OFFSET));
	val = pca9468->ta_vol/PD_MSG_TA_VOL_STEP;	/* PPS voltage resolution is 20mV */
	pca9468->ta_vol = val*PD_MSG_TA_VOL_STEP;
	/* Set TA voltage to MIN[TA voltage, TA_MAX_VOL*TA_mode] */
	pca9468->ta_vol = MIN(pca9468->ta_vol, pca9468->ta_max_vol*ta_mode);
	/* Set the initial TA current to IIN_CC/TA_mode */
	pca9468->ta_cur = pca9468->iin_cc/ta_mode;
	/* Recover IIN_CC to the original value(iin_cfg) */
	pca9468->iin_cc = pca9468->pdata->iin_cfg;

	pr_info("%s: Preset DC, ta_max_vol=%d, ta_max_cur=%d, ta_max_pwr=%d, iin_cc=%d, ta_mode=%d\n", 
		__func__, pca9468->ta_max_vol, pca9468->ta_max_cur, pca9468->ta_max_pwr, pca9468->iin_cc, pca9468->ta_mode);

	pr_info("%s: Preset DC, ta_vol=%d, ta_cur=%d\n", 
		__func__, pca9468->ta_vol, pca9468->ta_cur);

	/* Send PD Message */
	mutex_lock(&pca9468->lock);
	pca9468->timer_id = TIMER_PDMSG_SEND;
	pca9468->timer_period = 0;
	mutex_unlock(&pca9468->lock);
	schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}


/* Preset direct charging configuration */
static int pca9468_preset_config(struct pca9468_charger *pca9468)
{
	int ret = 0;
	
	pr_info("%s: ======START=======\n", __func__);

#if defined(CONFIG_BATTERY_SAMSUNG)
	pca9468_set_charging_state(pca9468, DC_STATE_PRESET_DC);
#else
	pca9468->charging_state = DC_STATE_PRESET_DC;
#endif

	/* Set IIN_CFG to IIN_CC */
	ret = pca9468_set_input_current(pca9468, pca9468->iin_cc);
	if (ret < 0)
		goto error;

	/* Set ICHG_CFG to enough high value */
	ret = pca9468_set_charging_current(pca9468, pca9468->pdata->ichg_cfg);
	if (ret < 0)
		goto error;

	/* Set VFLOAT */
	ret = pca9468_set_vfloat(pca9468, pca9468->pdata->v_float);
	if (ret < 0)
		goto error;

	/* Enable PCA9468 */	
	ret = pca9468_set_charging(pca9468, true);
	if (ret < 0)
		goto error;

	/* Clear previous iin adc */
	pca9468->prev_iin = 0;

	/* Clear TA increment flag */
	pca9468->prev_inc = INC_NONE;

	/* Go to CHECK_ACTIVE state after 150ms*/
	mutex_lock(&pca9468->lock);
	pca9468->timer_id = TIMER_CHECK_ACTIVE;
	pca9468->timer_period = PCA4968_ENABLE_DELAY_T;
	mutex_unlock(&pca9468->lock);
	schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
	ret = 0;
	
error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret; 
}

/* Check the charging status before entering the adjust cc mode */
static int pca9468_check_active_state(struct pca9468_charger *pca9468)
{
	int ret = 0;

	pr_info("%s: ======START=======\n", __func__);

#if defined(CONFIG_BATTERY_SAMSUNG)
	pca9468_set_charging_state(pca9468, DC_STATE_CHECK_ACTIVE);
#else
	pca9468->charging_state = DC_STATE_CHECK_ACTIVE;
#endif

	ret = pca9468_check_error(pca9468);

	if (ret == 0) {
		/* PCA9468 is active state */
		/* Clear retry counter */
		pca9468->retry_cnt = 0;
		/* Check TA mode */
		if (pca9468->ta_mode == WC_DC_MODE) {
			/* Go to WC CV mode */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_CHECK_WCCVMODE;
			pca9468->timer_period = 0;
			mutex_unlock(&pca9468->lock);
		} else {
			/* Go to Adjust CC mode */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_ADJUST_CCMODE;
			pca9468->timer_period = 0;
			mutex_unlock(&pca9468->lock);
		}
		schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
		ret = 0;
	} else if (ret == -EAGAIN) {
		/* It is the retry condition */
		/* Check the retry counter */
		if (pca9468->retry_cnt < PCA9468_MAX_RETRY_CNT) {
			/* Disable charging */
			ret = pca9468_set_charging(pca9468, false);
			/* Increase retry counter */
			pca9468->retry_cnt++;
			pr_err("%s: retry charging start - retry_cnt=%d\n", __func__, pca9468->retry_cnt);
			/* Go to DC_STATE_PRESET_DC */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_PRESET_DC;
			pca9468->timer_period = 0;
			mutex_unlock(&pca9468->lock);
			schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
			ret = 0;
		} else {
			pr_err("%s: retry fail\n", __func__);
			/* Notify maximum retry error */
			ret = -EINVAL;
		}
	} else {
		/* Implement error handler function if it is needed */
		/* Stop charging in timer_work */
	}
	
	return ret;
}


/* Enter direct charging algorithm */
static int pca9468_start_direct_charging(struct pca9468_charger *pca9468)
{
	int ret;
	unsigned int val;
#if !defined(CONFIG_BATTERY_SAMSUNG) /* temp disable wc dc charging */
	struct power_supply *psy;
	union power_supply_propval pro_val;
#endif

	pr_info("%s: =========START=========\n", __func__);

	/* Set OV_DELTA to 30% */
#if defined(CONFIG_SEC_FACTORY)
	val = OV_DELTA_40P << MASK2SHIFT(PCA9468_BIT_OV_DELTA);
#else
	val = OV_DELTA_30P << MASK2SHIFT(PCA9468_BIT_OV_DELTA);
#endif
	ret = pca9468_update_reg(pca9468, PCA9468_REG_SAFETY_CTRL,
							PCA9468_BIT_OV_DELTA, val);
	if (ret < 0)
			return ret;
#if defined(CONFIG_BATTERY_SAMSUNG)
	/* Set watchdog timer enable */
	pca9468_set_wdt_enable(pca9468, WDT_ENABLE);
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	pca9468_check_wdt_control(pca9468);
#else
	pca9468_set_wdt_timer(pca9468, WDT_8SEC);
#endif
#endif

	/* Set Switching Frequency */
	val = pca9468->pdata->fsw_cfg;
	ret = pca9468_update_reg(pca9468, PCA9468_REG_START_CTRL,
							PCA9468_BIT_FSW_CFG, val);
	if (ret < 0)
		return ret;

	/* current sense resistance */
	val = pca9468->pdata->snsres << MASK2SHIFT(PCA9468_BIT_SNSRES);
	ret = pca9468_update_reg(pca9468, PCA9468_REG_START_CTRL,
							PCA9468_BIT_SNSRES, val);
	if (ret < 0)
		return ret;

	/* Set EN_CFG to active low */
	val =  PCA9468_EN_ACTIVE_L;
	ret = pca9468_update_reg(pca9468, PCA9468_REG_START_CTRL,
							PCA9468_BIT_EN_CFG, val);
	if (ret < 0)
		return ret;

	/* Set NTC voltage threshold */
	val = pca9468->pdata->ntc_th / PCA9468_NTC_TH_STEP;
	ret = pca9468_write_reg(pca9468, PCA9468_REG_NTC_TH_1, (val & 0xFF));	
	if (ret < 0)
		return ret;
	ret = pca9468_update_reg(pca9468, PCA9468_REG_NTC_TH_2, 
						PCA9468_BIT_NTC_THRESHOLD9_8, (val >> 8));
	if (ret < 0)
		return ret;
#if !defined(CONFIG_BATTERY_SAMSUNG) /* temp disable wc dc charging */
	/* Check the current power supply type whether it is the wireless charger or USBPD TA */
	/* The below code should be modified by the customer */
	/* Get power supply name */
	psy = power_supply_get_by_name("battery");
	if (!psy) {
		dev_err(pca9468->dev, "Cannot find battery power supply\n");
		ret = -ENODEV;
		return ret;
	}

	/* Get the current power supply type */
	ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_ONLINE, &pro_val);
	power_supply_put(psy);
	if (ret < 0) {
		dev_err(pca9468->dev, "Cannot get power supply type from battery driver\n");
		return ret;
	}
	/* Check the current power supply type */
	if (pro_val.intval == POWER_SUPPLY_TYPE_WIRELESS) {
		/* The current power supply type is wireless charger */
		pca9468->ta_mode = WC_DC_MODE;
		pr_info("%s: The current power supply type is WC, ta_mode=%d\n", __func__, pca9468->ta_mode);
	}
#endif
	/* wake lock */
	__pm_stay_awake(&pca9468->monitor_wake_lock);

	/* Preset charging configuration and TA condition */
	ret = pca9468_preset_dcmode(pca9468);

	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}


/* Check Vbat minimum level to start direct charging */
static int pca9468_check_vbatmin(struct pca9468_charger *pca9468)
{
	int vbat;
	int ret;
	union power_supply_propval val;

	pr_info("%s: =========START=========\n", __func__);

#if defined(CONFIG_BATTERY_SAMSUNG)
	pca9468_set_charging_state(pca9468, DC_STATE_CHECK_VBAT);
#else
	pca9468->charging_state = DC_STATE_CHECK_VBAT;
#endif

	/* Check Vbat */
	vbat = pca9468_read_adc(pca9468, ADCCH_VBAT);
	if (vbat < 0) {
		ret = vbat;
	}

	/* Read switching charger status */
#if defined(CONFIG_BATTERY_SAMSUNG)
	ret = psy_do_property(pca9468->pdata->sec_dc_name, get,
		POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED_DC, val);
#else
	ret = pca9468_get_switching_charger_property(POWER_SUPPLY_PROP_CHARGING_ENABLED, &val);
#endif
	if (ret < 0) {
		/* Start Direct Charging again after 1sec */
		mutex_lock(&pca9468->lock);
		pca9468->timer_id = TIMER_VBATMIN_CHECK;
		pca9468->timer_period = PCA9468_VBATMIN_CHECK_T;
		mutex_unlock(&pca9468->lock);
		schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
		goto error;
	}

	if (val.intval == 0) {
		/* already disabled switching charger */
		/* Clear retry counter */
		pca9468->retry_cnt = 0;
		/* Preset TA voltage and PCA9468 parameters */
		mutex_lock(&pca9468->lock);
		pca9468->timer_id = TIMER_PRESET_DC;
		pca9468->timer_period = 0;
		mutex_unlock(&pca9468->lock);
		schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
	} else {
		/* Switching charger is enabled */
		if (vbat > PCA9468_DC_VBAT_MIN) {
			/* Start Direct Charging */
			/* now switching charger is enabled */
			/* disable switching charger first */
#if defined(CONFIG_BATTERY_SAMSUNG)
			pca9468_set_switching_charger(pca9468, false);
#else
			ret = pca9468_set_switching_charger(false, 0, 0, 0);
#endif
		}

		/* Wait 1sec for stopping switching charger or Start 1sec timer for battery check */
		mutex_lock(&pca9468->lock);
		pca9468->timer_id = TIMER_VBATMIN_CHECK;
		pca9468->timer_period = PCA9468_VBATMIN_CHECK_T;
		mutex_unlock(&pca9468->lock);
		schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
	}

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

#ifdef CONFIG_RTC_HCTOSYS
static int get_current_time(unsigned long *now_tm_sec)
{
	struct rtc_time tm;
	struct rtc_device *rtc;
	int rc;

	rtc = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);
	if (rtc == NULL) {
		pr_err("%s: unable to open rtc device (%s)\n",
		       __FILE__, CONFIG_RTC_HCTOSYS_DEVICE);
		return -EINVAL;
	}

	rc = rtc_read_time(rtc, &tm);
	if (rc) {
		pr_err("Error reading rtc device (%s) : %d\n",
		       CONFIG_RTC_HCTOSYS_DEVICE, rc);
		goto close_time;
	}

	rc = rtc_valid_tm(&tm);
	if (rc) {
		pr_err("Invalid RTC time (%s): %d\n",
		       CONFIG_RTC_HCTOSYS_DEVICE, rc);
		goto close_time;
	}
	rtc_tm_to_time(&tm, now_tm_sec);

 close_time:
	rtc_class_close(rtc);
	return rc;
}
#endif

/* delayed work function for charging timer */
static void pca9468_timer_work(struct work_struct *work)
{
	struct pca9468_charger *pca9468 = container_of(work, struct pca9468_charger,
						 timer_work.work);
	int ret = 0;
	unsigned int val;
#if defined(CONFIG_BATTERY_SAMSUNG)
	union power_supply_propval value = {0,};

	psy_do_property("battery", get,
				POWER_SUPPLY_PROP_CHARGE_COUNTER_SHADOW, value);
	if (value.intval == SEC_BATTERY_CABLE_NONE) {
		goto error;
	}
#endif

#ifdef CONFIG_RTC_HCTOSYS
	get_current_time(&pca9468->last_update_time);

	pr_info("%s: timer id=%d, charging_state=%d, last_update_time=%lu\n", 
		__func__, pca9468->timer_id, pca9468->charging_state, pca9468->last_update_time);
#else
	pr_info("%s: timer id=%d, charging_state=%d\n", 
		__func__, pca9468->timer_id, pca9468->charging_state);
#endif

	switch (pca9468->timer_id) {
	case TIMER_VBATMIN_CHECK:
		ret = pca9468_check_vbatmin(pca9468);
		if (ret < 0)
			goto error;
		break;

	case TIMER_PRESET_DC:
		ret = pca9468_start_direct_charging(pca9468);
		if (ret < 0)
			goto error;
		break;
		
	case TIMER_PRESET_CONFIG:
		ret = pca9468_preset_config(pca9468);
		if (ret < 0)
			goto error;
		break;

	case TIMER_CHECK_ACTIVE:
		ret = pca9468_check_active_state(pca9468);
		if (ret < 0)
			goto error;
		break;
		
	case TIMER_ADJUST_CCMODE:
		ret = pca9468_charge_adjust_ccmode(pca9468);
		if (ret < 0)
			goto error;
		break;			

	case TIMER_ENTER_CCMODE:
		ret = pca9468_charge_start_ccmode(pca9468);
		if (ret < 0)
			goto error;
		break;

	case TIMER_CHECK_CCMODE:
		ret = pca9468_charge_ccmode(pca9468);
		if (ret < 0)
			goto error;
		break;

	case TIMER_ENTER_CVMODE:
		/* Enter Pre-CV mode */
		ret = pca9468_charge_start_cvmode(pca9468);
		if (ret < 0)
			goto error;
		break;

	case TIMER_CHECK_CVMODE:
		ret = pca9468_charge_cvmode(pca9468);
		if (ret < 0)
			goto error;
		break;

	case TIMER_PDMSG_SEND:
		/* Adjust TA current and voltage step */
		val = pca9468->ta_vol/PD_MSG_TA_VOL_STEP;	/* PPS voltage resolution is 20mV */
		pca9468->ta_vol = val*PD_MSG_TA_VOL_STEP;
		val = pca9468->ta_cur/PD_MSG_TA_CUR_STEP;	/* PPS current resolution is 50mA */
		pca9468->ta_cur = val*PD_MSG_TA_CUR_STEP;
		if (pca9468->ta_cur < PCA9468_TA_MIN_CUR)	/* PPS minimum current is 1000mA */
			pca9468->ta_cur = PCA9468_TA_MIN_CUR;
		/* Send PD Message */
		ret = pca9468_send_pd_message(pca9468, PD_MSG_REQUEST_APDO);
		if (ret < 0)
			goto error;

		/* Go to the next state */
		mutex_lock(&pca9468->lock);
		switch (pca9468->charging_state) {
		case DC_STATE_PRESET_DC:
			pca9468->timer_id = TIMER_PRESET_CONFIG;
			break;
		case DC_STATE_ADJUST_CC:
			pca9468->timer_id = TIMER_ADJUST_CCMODE;
			break;
		case DC_STATE_START_CC:
			pca9468->timer_id = TIMER_ENTER_CCMODE;
			break;
		case DC_STATE_CC_MODE:
			pca9468->timer_id = TIMER_CHECK_CCMODE;
			break;
		case DC_STATE_START_CV:
			pca9468->timer_id = TIMER_ENTER_CVMODE;
			break;
		case DC_STATE_CV_MODE:
			pca9468->timer_id = TIMER_CHECK_CVMODE;
			break;
		case DC_STATE_ADJUST_TAVOL:
			pca9468->timer_id = TIMER_ADJUST_TAVOL;
			break;
		case DC_STATE_ADJUST_TACUR:
			pca9468->timer_id = TIMER_ADJUST_TACUR;
			break;
		default:
			ret = -EINVAL;
			break;
		}
		pca9468->timer_period = PCA9468_PDMSG_WAIT_T;
		mutex_unlock(&pca9468->lock);
		schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
		break;

	case TIMER_ADJUST_TAVOL:
		ret = pca9468_adjust_ta_voltage(pca9468);
		if (ret < 0)
			goto error;
		break;

	case TIMER_ADJUST_TACUR:
		ret = pca9468_adjust_ta_current(pca9468);
		if (ret < 0)
			goto error;
		break;

	case TIMER_CHECK_WCCVMODE:
		ret = pca9468_charge_wc_cvmode(pca9468);
		if (ret < 0)
			goto error;
		break;

	default:
		break;
	}

	/* Check the charging state again */
	if (pca9468->charging_state == DC_STATE_NO_CHARGING) {
		/* Cancel work queue again */
		cancel_delayed_work(&pca9468->timer_work);
		cancel_delayed_work(&pca9468->pps_work);
	}
	return;
	
error:
#if defined(CONFIG_BATTERY_SAMSUNG)
	pca9468->chg_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
	pca9468->health_status = POWER_SUPPLY_HEALTH_DC_ERR;
#endif
	pca9468_stop_charging(pca9468);
	return;
}


/* delayed work function for pps periodic timer */
static void pca9468_pps_request_work(struct work_struct *work)
{
	struct pca9468_charger *pca9468 = container_of(work, struct pca9468_charger,
						 pps_work.work);

	int ret = 0;
#if defined(CONFIG_BATTERY_SAMSUNG)
	int vin, iin;

	/* this is for wdt */
	vin = pca9468_read_adc(pca9468, ADCCH_VIN);
	iin = pca9468_read_adc(pca9468, ADCCH_IIN);
	pr_info("%s: pps_work_start (vin:%dmV, iin:%dmA)\n",
			__func__, vin/PCA9468_SEC_DENOM_U_M, iin/PCA9468_SEC_DENOM_U_M);
#else
	pr_info("%s: pps_work_start\n", __func__);

	/* Send PD message */
	ret = pca9468_send_pd_message(pca9468, PD_MSG_REQUEST_APDO);
#endif
	pr_info("%s: End, ret=%d\n", __func__, ret);
}

static int pca9468_hw_init(struct pca9468_charger *pca9468)
{
	unsigned int val;
	int ret;

	pr_info("%s: =========START=========\n", __func__);

	/* Read Device info register to check the incomplete I2C operation */
	ret = pca9468_read_reg(pca9468, PCA9468_REG_DEVICE_INFO, &val);
	if ((ret < 0) || (val != PCA9468_DEVICE_ID)) {
		/* There is the incomplete I2C operation or I2C communication error */
		/* Read Device info register again */
		ret = pca9468_read_reg(pca9468, PCA9468_REG_DEVICE_INFO, &val);
		if ((ret < 0) || (val != PCA9468_DEVICE_ID)) {
			dev_err(pca9468->dev, "reading DEVICE_INFO failed, val=0x%x\n", val);
			ret = -EINVAL;
			return ret;
		}
	}

	/*
	 * Program the platform specific configuration values to the device
	 * first.
	 */
#if defined(CONFIG_BATTERY_SAMSUNG)
	pca9468->chg_status = POWER_SUPPLY_STATUS_DISCHARGING;
	pca9468->health_status = POWER_SUPPLY_HEALTH_GOOD;
#endif

#if defined(CONFIG_SEC_FACTORY)
	val = OV_DELTA_40P << MASK2SHIFT(PCA9468_BIT_OV_DELTA);
#else
	val = OV_DELTA_30P << MASK2SHIFT(PCA9468_BIT_OV_DELTA);
#endif
	ret = pca9468_update_reg(pca9468, PCA9468_REG_SAFETY_CTRL,
						 	PCA9468_BIT_OV_DELTA, val);
	if (ret < 0)
		return ret;

	/* Set Switching Frequencey */
	val = pca9468->pdata->fsw_cfg << MASK2SHIFT(PCA9468_BIT_FSW_CFG);
	ret = pca9468_update_reg(pca9468, PCA9468_REG_START_CTRL,
						 	PCA9468_BIT_FSW_CFG, val);
	if (ret < 0)
		return ret;

	/* Die Temperature regulation 120'C */
	ret = pca9468_update_reg(pca9468, PCA9468_REG_TEMP_CTRL,
			PCA9468_BIT_TEMP_REG, 0x3 << MASK2SHIFT(PCA9468_BIT_TEMP_REG));

	/* current sense resistance */
	val = pca9468->pdata->snsres << MASK2SHIFT(PCA9468_BIT_SNSRES);
	ret = pca9468_update_reg(pca9468, PCA9468_REG_START_CTRL,
						 	PCA9468_BIT_SNSRES, val);
	if (ret < 0)
		return ret;

	/* Set Reverse Current Detection and standby mode*/
	val = PCA9468_BIT_REV_IIN_DET | PCA9468_EN_ACTIVE_L | PCA9468_STANDBY_FORCED;
	ret = pca9468_update_reg(pca9468, PCA9468_REG_START_CTRL,
							(PCA9468_BIT_REV_IIN_DET | PCA9468_BIT_EN_CFG | PCA9468_BIT_STANDBY_EN), 
							val);
	if (ret < 0)
		return ret;

	/* clear LIMIT_INCREMENT_EN */
	val = 0;
	ret = pca9468_update_reg(pca9468, PCA9468_REG_IIN_CTRL,
						 	PCA9468_BIT_LIMIT_INCREMENT_EN, val);
	if (ret < 0)
		return ret;
	
	/* Set the ADC channel */
	val = (PCA9468_BIT_CH7_EN |	/* NTC voltage ADC */
		   PCA9468_BIT_CH6_EN | /* DIETEMP ADC */
		   PCA9468_BIT_CH5_EN | /* IIN ADC */
		   PCA9468_BIT_CH3_EN | /* VBAT ADC */
 		   PCA9468_BIT_CH2_EN | /* VIN ADC */
 		   PCA9468_BIT_CH1_EN); /* VOUT ADC */

	ret = pca9468_write_reg(pca9468, PCA9468_REG_ADC_CFG, val);
	if (ret < 0)
		return ret;

	/* ADC Mode change */
	val = 0x5B;
	ret = pca9468_write_reg(pca9468, PCA9468_REG_ADC_ACCESS, val);
	if (ret < 0)
		return ret;
	val = 0x10;
	ret = pca9468_write_reg(pca9468, PCA9468_REG_ADC_MODE, val);
	if (ret < 0)
		return ret;
	val = 0x00;
	ret = pca9468_write_reg(pca9468, PCA9468_REG_ADC_ACCESS, val);	
	if (ret < 0)
		return ret;

	/* Read ADC compesation gain */
	ret = pca9468_read_reg(pca9468, PCA9468_REG_ADC_ADJUST, &val);
	if (ret < 0)
		return ret;
	pca9468->adc_comp_gain = adc_gain[(val>>MASK2SHIFT(PCA9468_BIT_ADC_GAIN))];
	
	/* input current - uA*/
	ret = pca9468_set_input_current(pca9468, pca9468->pdata->iin_cfg);
	if (ret < 0)
		return ret;

	/* charging current */
	ret = pca9468_set_charging_current(pca9468, pca9468->pdata->ichg_cfg);
	if (ret < 0)
		return ret;

	/* v float voltage */
	ret = pca9468_set_vfloat(pca9468, pca9468->pdata->v_float);
	if (ret < 0)
		return ret;
	pca9468->pdata->v_float_max = pca9468->pdata->v_float;
	pr_info("%s: v_float_max(%duV)\n", __func__, pca9468->pdata->v_float_max);

	return ret;
}


static irqreturn_t pca9468_interrupt_handler(int irq, void *data)
{
	struct pca9468_charger *pca9468 = data;
	u8 int1[REG_INT1_MAX], sts[REG_STS_MAX];	/* INT1, INT1_MSK, INT1_STS, STS_A, B, C, D */
	u8 masked_int;	/* masked int */
	bool handled = false;
	int ret;

	/* Read INT1, INT1_MSK, INT1_STS */
	ret = pca9468_bulk_read_reg(pca9468, PCA9468_REG_INT1, int1, 3);
	if (ret < 0) {
		dev_warn(pca9468->dev, "reading INT1_X failed\n");
		return IRQ_NONE;
	}

	/* Read STS_A, B, C, D */
	ret = pca9468_bulk_read_reg(pca9468, PCA9468_REG_STS_A, sts, 4);
	if (ret < 0) {
		dev_warn(pca9468->dev, "reading STS_X failed\n");
		return IRQ_NONE;
	}

	pr_info("%s: int1=0x%2x, int1_sts=0x%2x, sts_a=0x%2x\n", __func__, 
			int1[REG_INT1], int1[REG_INT1_STS], sts[REG_STS_A]);

	/* Check Interrupt */
	masked_int = int1[REG_INT1] & !int1[REG_INT1_MSK];
	if (masked_int & PCA9468_BIT_V_OK_INT) {
		/* V_OK interrupt happened */
		mutex_lock(&pca9468->lock);
		pca9468->mains_online = (int1[REG_INT1_STS] & PCA9468_BIT_V_OK_STS) ? true : false;
		mutex_unlock(&pca9468->lock);
		power_supply_changed(pca9468->psy_chg);
		handled = true;
	}

	if (masked_int & PCA9468_BIT_NTC_TEMP_INT) {
		/* NTC_TEMP interrupt happened */
		if (int1[REG_INT1_STS] & PCA9468_BIT_NTC_TEMP_STS) {
			/* above NTC_THRESHOLD */
			dev_err(pca9468->dev, "charging stopped due to NTC threshold voltage\n");
		}
		handled = true;
	}

	if (masked_int & PCA9468_BIT_CHG_PHASE_INT) {
		/* CHG_PHASE interrupt happened */
		if (int1[REG_INT1_STS] & PCA9468_BIT_CHG_PHASE_STS) {
			/* Any of loops is active*/
			if (sts[REG_STS_A] & PCA9468_BIT_VFLT_LOOP_STS)	{
				/* V_FLOAT loop is in regulation */
				pr_info("%s: V_FLOAT loop interrupt\n", __func__);
				/* Disable CHG_PHASE_M */
				ret = pca9468_update_reg(pca9468, PCA9468_REG_INT1_MSK, 
										PCA9468_BIT_CHG_PHASE_M, PCA9468_BIT_CHG_PHASE_M);
				if (ret < 0) {
					handled = false;
					return handled;
				}
				/* Go to Pre CV Mode */
				pca9468->timer_id = TIMER_ENTER_CVMODE;
				pca9468->timer_period = 10;
				schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
			} else if (sts[REG_STS_A] & PCA9468_BIT_IIN_LOOP_STS) {
				/* IIN loop or ICHG loop is in regulation */
				pr_info("%s: IIN loop interrupt\n", __func__);
			} else if (sts[REG_STS_A] & PCA9468_BIT_CHG_LOOP_STS) {
				/* ICHG loop is in regulation */
				pr_info("%s: ICHG loop interrupt\n", __func__);
			}
		}
		handled = true;
	}

	if (masked_int & PCA9468_BIT_CTRL_LIMIT_INT) {
		/* CTRL_LIMIT interrupt happened */
		if (int1[REG_INT1_STS] & PCA9468_BIT_CTRL_LIMIT_STS) {
			/* No Loop is active or OCP */
			if (sts[REG_STS_B] & PCA9468_BIT_OCP_FAST_STS) {
				/* Input fast over current */
				dev_err(pca9468->dev, "IIN > 50A instantaneously\n");
			}
			if (sts[REG_STS_B] & PCA9468_BIT_OCP_AVG_STS) {
				/* Input average over current */
				dev_err(pca9468->dev, "IIN > IIN_CFG*150percent\n");
			}
		}
		handled = true;
	}

	if (masked_int & PCA9468_BIT_TEMP_REG_INT) {
		/* TEMP_REG interrupt happened */
		if (int1[REG_INT1_STS] & PCA9468_BIT_TEMP_REG_STS) {
			/* Device is in temperature regulation */
			dev_err(pca9468->dev, "Device is in temperature regulation\n");
		}
		handled = true;
	}

	if (masked_int & PCA9468_BIT_ADC_DONE_INT) {
		/* ADC complete interrupt happened */
		dev_dbg(pca9468->dev, "ADC has been completed\n");
		handled = true;
	}

	if (masked_int & PCA9468_BIT_TIMER_INT) {
		/* Timer falut interrupt happened */
		if (int1[REG_INT1_STS] & PCA9468_BIT_TIMER_STS) {
			if (sts[REG_STS_B] & PCA9468_BIT_CHARGE_TIMER_STS) {
				/* Charger timer is expired */
				dev_err(pca9468->dev, "Charger timer is expired\n");
			}
			if (sts[REG_STS_B] & PCA9468_BIT_WATCHDOG_TIMER_STS) {
				/* Watchdog timer is expired */
				dev_err(pca9468->dev, "Watchdog timer is expired\n");
			}
		}
		handled = true;
	}

	return handled ? IRQ_HANDLED : IRQ_NONE;
}

static int pca9468_irq_init(struct pca9468_charger *pca9468,
			   struct i2c_client *client)
{
	const struct pca9468_platform_data *pdata = pca9468->pdata;
	int ret, msk, irq;

	pr_info("%s: =========START=========\n", __func__);

	irq = gpio_to_irq(pdata->irq_gpio);

	ret = gpio_request_one(pdata->irq_gpio, GPIOF_IN, client->name);
	if (ret < 0)
		goto fail;

	ret = request_threaded_irq(irq, NULL, pca9468_interrupt_handler,
				   IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				   client->name, pca9468);
	if (ret < 0)
		goto fail_gpio;

	/*
	 * Configure the Mask Register for interrupts: disable all interrupts by default.
	 */
	msk = (PCA9468_BIT_V_OK_M | 
		   PCA9468_BIT_NTC_TEMP_M | 
		   PCA9468_BIT_CHG_PHASE_M |
		   PCA9468_BIT_RESERVED_M |
		   PCA9468_BIT_CTRL_LIMIT_M |
		   PCA9468_BIT_TEMP_REG_M |
		   PCA9468_BIT_ADC_DONE_M |
		   PCA9468_BIT_TIMER_M);
	ret = pca9468_write_reg(pca9468, PCA9468_REG_INT1_MSK, msk);
	if (ret < 0)
		goto fail_wirte;
	
	client->irq = irq;
	return 0;

fail_wirte:
	free_irq(irq, pca9468);
fail_gpio:
	gpio_free(pdata->irq_gpio);
fail:
	client->irq = 0;
	return ret;
}


/*
 * Returns the input current limit programmed
 * into the charger in uA.
 */
static int get_input_current_limit(struct pca9468_charger *pca9468)
{
	int ret, intval;
	unsigned int val;

	if (!pca9468->mains_online)
		return -ENODATA;

	ret = pca9468_read_reg(pca9468, PCA9468_REG_IIN_CTRL, &val);
	if (ret < 0)
		return ret;

	intval = (val & PCA9468_BIT_IIN_CFG) * 100000;

	if (intval < 500000)
		intval = 500000;
	
	return intval;
}

/*
 * Returns the constant charge current programmed
 * into the charger in uA.
 */
static int get_const_charge_current(struct pca9468_charger *pca9468)
{
	int ret, intval;
	unsigned int val;

	if (!pca9468->mains_online)
		return -ENODATA;

	ret = pca9468_read_reg(pca9468, PCA9468_REG_ICHG_CTRL, &val);
	if (ret < 0)
		return ret;

	intval = (val & PCA9468_BIT_ICHG_CFG) * 100000;

	return intval;
}

/*
 * Returns the constant charge voltage programmed
 * into the charger in uV.
 */
static int get_const_charge_voltage(struct pca9468_charger *pca9468)
{
	int ret, intval;
	unsigned int val;

	if (!pca9468->mains_online)
		return -ENODATA;

	ret = pca9468_read_reg(pca9468, PCA9468_REG_V_FLOAT, &val);
	if (ret < 0)
		return ret;

	intval = (val * 5 + 3725) * 1000;

	return intval;
}

/*
 * Returns the enable or disable value.
 * into 1 or 0.
 */
static int get_charging_enabled(struct pca9468_charger *pca9468)
{
	int ret, intval;
	unsigned int val;
	
	ret = pca9468_read_reg(pca9468, PCA9468_REG_START_CTRL, &val);
	if (ret < 0)
		return ret;

	intval = (val & PCA9468_BIT_STANDBY_EN) ? 0 : 1;

	return intval;
}

#if defined(CONFIG_BATTERY_SAMSUNG)
/*
 * Returns the system current in uV.
 */
static int get_system_current(struct pca9468_charger *pca9468)
{
	/* get the system current */
	/* get the battery power supply to get charging current */
	union power_supply_propval val;
	struct power_supply *psy;
	int ret;
	int iin, ibat, isys;

	psy = power_supply_get_by_name("battery");
	if (!psy) {
		dev_err(pca9468->dev, "Cannot find battery power supply\n");
		goto error;
	}

	/* get the charging current */
	ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_CURRENT_NOW, &val);
	power_supply_put(psy);
	if (ret < 0) {
		dev_err(pca9468->dev, "Cannot get battery current from FG\n");
		goto error;
	}
	ibat = val.intval;

	/* calculate the system current */
	/* get input current */
	iin = pca9468_read_adc(pca9468, ADCCH_IIN);
	if (iin < 0) {
		dev_err(pca9468->dev, "Invalid IIN ADC\n");
		goto error;
	}

	/* calculate the system current */
	/* Isys = (Iin - Ifsw_cfg)*2 - Ibat */
	iin = (iin - iin_fsw_cfg[pca9468->pdata->fsw_cfg])*2;
	iin /= PCA9468_SEC_DENOM_U_M;
	isys = iin - ibat;
	pr_info("%s: isys=%dmA\n", __func__, isys);

	return isys;

error:
	return -EINVAL;
}
#endif

static int pca9468_chg_set_property(struct power_supply *psy,
				     enum power_supply_property prop,
				     const union power_supply_propval *val)
{
	struct pca9468_charger *pca9468 = power_supply_get_drvdata(psy);
#if defined(CONFIG_BATTERY_SAMSUNG)
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) prop;
#endif
	int ret = 0;
	unsigned int temp = 0;

	pr_info("%s: =========START=========\n", __func__);
	pr_info("%s: prop=%d, val=%d\n", __func__, prop, val->intval);

	switch (prop) {
	/* Todo - Insert code */
	/* It needs modification by a customer */
	/* The customer make a decision to start charging and stop charging property */
	
	case POWER_SUPPLY_PROP_ONLINE:	/* need to change property */
		if (val->intval == 0) {
			pca9468->mains_online = false;
			// Stop Direct charging 
			ret = pca9468_stop_charging(pca9468);
			pca9468->chg_status = POWER_SUPPLY_STATUS_DISCHARGING;
			pca9468->health_status = POWER_SUPPLY_HEALTH_GOOD;
			if (ret < 0)
				goto error;
		} else {
			// Start Direct charging
			pca9468->mains_online = true;
#if !defined(CONFIG_BATTERY_SAMSUNG)
			/* Start 1sec timer for battery check */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_VBATMIN_CHECK;
#if defined(CONFIG_BATTERY_SAMSUNG)
			pca9468->timer_period = PCA9468_VBATMIN_CHECK_T;	/* The delay time for PD state goes to PE_SNK_STATE */
#else
			pca9468->timer_period = 5000;	/* The dealy time for PD state goes to PE_SNK_STATE */
#endif
			mutex_unlock(&pca9468->lock);
			schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
#endif
			ret = 0;
		}
		break;

	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		if (val->intval == 0) {
			// Stop Direct Charging
			ret = pca9468_stop_charging(pca9468);
			if (ret < 0)
				goto error;
		} else {
#if defined(CONFIG_BATTERY_SAMSUNG)
			if (pca9468->charging_state != DC_STATE_NO_CHARGING) {
				pr_info("## %s: duplicate charging enabled(%d)\n", __func__, val->intval);
				goto error;
			}
			if (!pca9468->mains_online) {
				pr_info("## %s: mains_online is not attached(%d)\n", __func__, val->intval);
				goto error;
			}
#endif
			// Start Direct Charging
			/* Start 1sec timer for battery check */
			mutex_lock(&pca9468->lock);
			pca9468->timer_id = TIMER_VBATMIN_CHECK;
#if defined(CONFIG_BATTERY_SAMSUNG)
			pca9468->timer_period = PCA9468_VBATMIN_CHECK_T;	/* The delay time for PD state goes to PE_SNK_STATE */
#else
			pca9468->timer_period = 5000;	/* The delay time for PD state goes to PE_SNK_STATE */
#endif
			mutex_unlock(&pca9468->lock);
			schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
			ret = 0;
		}
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		if (val->intval != pca9468->new_vfloat) {
			/* request new float voltage */
			pca9468->new_vfloat = val->intval;
			ret = pca9468_set_new_vfloat(pca9468);
		}
		break;

	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		if (val->intval != pca9468->new_iin) {
			/* request new input current */
			pca9468->new_iin = val->intval;
			ret = pca9468_set_new_iin(pca9468);
		}
		break;

#if defined(CONFIG_BATTERY_SAMSUNG)
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		pca9468->charging_current = val->intval;
#if 0
		pca9468->pdata->ichg_cfg = val->intval * PCA9468_SEC_DENOM_U_M;
		pr_info("## %s: charging current(%duA)\n", __func__, pca9468->pdata->ichg_cfg);
		pca9468_set_charging_current(pca9468, pca9468->pdata->ichg_cfg);
#endif
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
	/* this is same with POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT */
		pca9468->input_current = val->intval;
		temp = pca9468->input_current * PCA9468_SEC_DENOM_U_M;
		if (temp != pca9468->new_iin) {
			/* request new input current */
			pca9468->new_iin = temp;
			ret = pca9468_set_new_iin(pca9468);
			pr_info("## %s: input current(new_iin: %duA)\n", __func__, pca9468->new_iin);
		}

		break;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
#if defined(CONFIG_BATTERY_SAMSUNG) && defined(CONFIG_ENG_BATTERY_CONCEPT)
		case POWER_SUPPLY_EXT_PROP_DIRECT_WDT_CONTROL:
			if (val->intval) {
				pca9468->wdt_kick = true;
			} else {
				pca9468->wdt_kick = false;
				cancel_delayed_work(&pca9468->wdt_control_work);
			}
			pr_info("%s: wdt kick (%d)\n", __func__, pca9468->wdt_kick);
			break;
#endif
		case POWER_SUPPLY_EXT_PROP_DIRECT_VOLTAGE_MAX:
			pca9468->float_voltage = val->intval;
			temp = pca9468->float_voltage * PCA9468_SEC_DENOM_U_M;
			if (temp != pca9468->new_vfloat) {
				/* request new float voltage */
				pca9468->new_vfloat = temp;
				ret = pca9468_set_new_vfloat(pca9468);
				pr_info("## %s: float voltage(%duV)\n", __func__, pca9468->new_vfloat);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_CURRENT_MAX:
			pca9468->input_current = val->intval;
			temp = pca9468->input_current * PCA9468_SEC_DENOM_U_M;
			if (temp != pca9468->new_iin) {
				/* request new input current */
				pca9468->new_iin = temp;
				ret = pca9468_set_new_iin(pca9468);
				pr_info("## %s: input current(new_iin: %duA)\n", __func__, pca9468->new_iin);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_FLOAT_MAX:
			pca9468->pdata->v_float_max = val->intval * PCA9468_SEC_DENOM_U_M;
			pr_info("%s: v_float_max(%duV)\n", __func__, pca9468->pdata->v_float_max);
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_ADC_CTRL:
			if (val->intval) {
				temp = FORCE_NORMAL_MODE << MASK2SHIFT(PCA9468_BIT_FORCE_ADC_MODE);
				ret = pca9468_update_reg(pca9468, PCA9468_REG_ADC_CTRL, PCA9468_BIT_FORCE_ADC_MODE, temp);
				if (ret < 0)
					return ret;
			} else {
				temp = AUTO_MODE << MASK2SHIFT(PCA9468_BIT_FORCE_ADC_MODE);
				ret = pca9468_update_reg(pca9468, PCA9468_REG_ADC_CTRL, PCA9468_BIT_FORCE_ADC_MODE, temp);
				if (ret < 0)
					return ret;
			}

			ret = pca9468_read_reg(pca9468, PCA9468_REG_ADC_CTRL, &temp);
			pr_info("%s: ADC_CTRL : 0x%02x\n", __func__, temp);
			break;
		default:
			return -EINVAL;
		}
		break;
#endif
	default:
		ret = -EINVAL;
		break;
	}

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}


static int pca9468_chg_get_property(struct power_supply *psy,
				     enum power_supply_property prop,
				     union power_supply_propval *val)
{
	int ret = 0;
	struct pca9468_charger *pca9468 = power_supply_get_drvdata(psy);
#if defined(CONFIG_BATTERY_SAMSUNG)
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) prop;
#endif

	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = pca9468->mains_online;
		break;

#if defined(CONFIG_BATTERY_SAMSUNG)
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = pca9468->chg_status;
		pr_info("%s: CHG STATUS : %d\n", __func__, pca9468->chg_status);
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		if (pca9468->charging_state >= DC_STATE_CHECK_ACTIVE &&
			pca9468->charging_state <= DC_STATE_CV_MODE)
			pca9468_check_error(pca9468);
		val->intval = pca9468->health_status;
		pr_info("%s: HEALTH STATUS : %d\n", __func__, pca9468->health_status);
		break;
#endif

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		ret = get_const_charge_voltage(pca9468);
		if (ret < 0)
			return ret;
		else
#if defined(CONFIG_BATTERY_SAMSUNG)
			val->intval = pca9468->float_voltage;
#else
			val->intval = ret;
#endif
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = get_const_charge_current(pca9468);
		if (ret < 0)
			return ret;
		else
			val->intval = ret;
		break;

	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		ret = get_charging_enabled(pca9468);
		if (ret < 0)
			return ret;
		else
			val->intval = ret;
		break;

	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = get_input_current_limit(pca9468);
		if (ret < 0)
			return ret;
		else
			val->intval = ret;
		break;

	case POWER_SUPPLY_PROP_TEMP:
		/* return NTC voltage  - uV unit */
		ret = pca9468_read_adc(pca9468, ADCCH_NTC);
		if (ret < 0)
			return ret;
		else
			val->intval = ret;
		break;

#if defined(CONFIG_BATTERY_SAMSUNG)
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = pca9468->input_current;
		break;

	case POWER_SUPPLY_PROP_CURRENT_NOW: /* get charge current which was set */
		val->intval = pca9468->charging_current;
		break;

	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_MONITOR_WORK:
			pca9468_monitor_work(pca9468);
			pca9468_test_read(pca9468);
			break;
		case POWER_SUPPLY_EXT_PROP_MEASURE_INPUT:
			switch (val->intval) {
			case SEC_BATTERY_IIN_MA:
				pca9468_read_adc(pca9468, ADCCH_IIN);
				val->intval = pca9468->adc_val[ADCCH_IIN];
				break;
			case SEC_BATTERY_IIN_UA:
				pca9468_read_adc(pca9468, ADCCH_IIN);
				val->intval = pca9468->adc_val[ADCCH_IIN] * PCA9468_SEC_DENOM_U_M;
				break;
			case SEC_BATTERY_VIN_MA:
				val->intval = pca9468->adc_val[ADCCH_VIN];
				break;
			case SEC_BATTERY_VIN_UA:
				val->intval = pca9468->adc_val[ADCCH_VIN] * PCA9468_SEC_DENOM_U_M;
				break;
			default:
				val->intval = 0;
				break;
			}
			break;
		case POWER_SUPPLY_EXT_PROP_MEASURE_SYS:
			/* return system current - uA unit */
			/* check charging status */
			if (pca9468->charging_state == DC_STATE_NO_CHARGING) {
				/* return invalid */
				val->intval = 0;
				return -EINVAL;
			} else {
				/* calculate Isys */
				ret = get_system_current(pca9468);
				if (ret < 0)
					return 0;
				else
					val->intval = ret;
			}
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_VOLTAGE_MAX:
			val->intval = pca9468->float_voltage;
			break;
		default:
			return -EINVAL;
		}
		break;
#endif
	default:
		return -EINVAL;
	}

	return 0;
}

static enum power_supply_property pca9468_charger_props[] = {
};


static const struct regmap_config pca9468_regmap = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.max_register	= PCA9468_MAX_REGISTER,
};

static char *pca9468_supplied_to[] = {
	"pca9468-charger",
};

static const struct power_supply_desc pca9468_charger_power_supply_desc = {
	.name		= "pca9468-charger",
	.type		= POWER_SUPPLY_TYPE_UNKNOWN,
	.get_property	= pca9468_chg_get_property,
	.set_property 	= pca9468_chg_set_property,
	.properties	= pca9468_charger_props,
	.num_properties	= ARRAY_SIZE(pca9468_charger_props),
};

#if defined(CONFIG_OF)
static int pca9468_charger_parse_dt(struct device *dev, struct pca9468_platform_data *pdata)
{
	struct device_node *np_pca9468 = dev->of_node;
#if defined(CONFIG_BATTERY_SAMSUNG)
	struct device_node *np;
#endif
	int ret;
	if(!np_pca9468)
		return -EINVAL;

	/* irq gpio */
	pdata->irq_gpio = of_get_named_gpio(np_pca9468, "pca9468,irq-gpio", 0);
	pr_info("%s: irq-gpio: %u \n", __func__, pdata->irq_gpio);

	/* input current limit */
	ret = of_property_read_u32(np_pca9468, "pca9468,input-current-limit",
						   &pdata->iin_cfg);
	if (ret) {
		pr_info("%s: pca9468,input-current-limit is Empty\n", __func__);
		pdata->iin_cfg = PCA9468_IIN_CFG_DFT;
	}
	pr_info("%s: pca9468,iin_cfg is %d\n", __func__, pdata->iin_cfg);

	/* charging current */
	ret = of_property_read_u32(np_pca9468, "pca9468,charging-current",
							   &pdata->ichg_cfg);
	if (ret) {
		pr_info("%s: pca9468,charging-current is Empty\n", __func__);
		pdata->ichg_cfg = PCA9468_ICHG_CFG_DFT;
	}
	pr_info("%s: pca9468,ichg_cfg is %d\n", __func__, pdata->ichg_cfg);

#if !defined(CONFIG_BATTERY_SAMSUNG)
	/* charging float voltage */
	ret = of_property_read_u32(np_pca9468, "pca9468,float-voltage",
							   &pdata->v_float);
	if (ret) {
		pr_info("%s: pca9468,float-voltage is Empty\n", __func__);
		pdata->v_float = PCA9468_VFLOAT_DFT;
	}
	pr_info("%s: pca9468,v_float is %d\n", __func__, pdata->v_float);
#endif

	/* input topoff current */
	ret = of_property_read_u32(np_pca9468, "pca9468,input-itopoff",
							   &pdata->iin_topoff);
	if (ret) {
		pr_info("%s: pca9468,input-itopoff is Empty\n", __func__);
		pdata->iin_topoff = PCA9468_IIN_DONE_DFT;
	}
	pr_info("%s: pca9468,iin_topoff is %d\n", __func__, pdata->iin_topoff);

	/* sense resistance */
	ret = of_property_read_u32(np_pca9468, "pca9468,sense-resistance",
							   &pdata->snsres);
	if (ret) {
		pr_info("%s: pca9468,sense-resistance is Empty\n", __func__);
		pdata->snsres = PCA9468_SENSE_R_DFT;
	}
	pr_info("%s: pca9468,snsres is %d\n", __func__, pdata->snsres);

	/* switching frequency */
	ret = of_property_read_u32(np_pca9468, "pca9468,switching-frequency",
							   &pdata->fsw_cfg);
	if (ret) {
		pr_info("%s: pca9468,switching frequency is Empty\n", __func__);
		pdata->fsw_cfg = PCA9468_FSW_CFG_DFT;
	}
	pr_info("%s: pca9468,fsw_cfg is %d\n", __func__, pdata->fsw_cfg);

	/* NTC threshold voltage */
	ret = of_property_read_u32(np_pca9468, "pca9468,ntc-threshold",
							   &pdata->ntc_th);
	if (ret) {
		pr_info("%s: pca9468,ntc threshold voltage is Empty\n", __func__);
		pdata->ntc_th = PCA9468_NTC_TH_DFT;
	}
	pr_info("%s: pca9468,ntc_th is %d\n", __func__, pdata->ntc_th);

	/* TA voltage mode */
	ret = of_property_read_u32(np_pca9468, "pca9468,ta-mode",
							   &pdata->ta_mode);
	if (ret) {
		pr_info("%s: pca9468,ta mode is Empty\n", __func__);
		pdata->ta_mode = TA_2TO1_DC_MODE;
	}
	pr_info("%s: pca9468,ta_mode is %d\n", __func__, pdata->ta_mode);

#if defined(CONFIG_BATTERY_SAMSUNG)
	pdata->chgen_gpio = of_get_named_gpio(np_pca9468, "pca9468,chg_gpio_en", 0);
	if (pdata->chgen_gpio < 0) {
		pr_err("%s : cannot get chgen gpio : %d\n",
			__func__, pdata->chgen_gpio);
		return -ENODATA;	
	} else {
		pr_info("%s: chgen gpio : %d\n", __func__, pdata->chgen_gpio);
	}

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("## %s: np(battery) NULL\n", __func__);
	} else {
		ret = of_property_read_u32(np, "battery,chg_float_voltage",
					   &pdata->v_float);
		if (ret) {
			pr_info("## %s: battery,chg_float_voltage is Empty\n", __func__);
			pdata->v_float = PCA9468_VFLOAT_DFT;
		} else {		
			pdata->v_float = pdata->v_float * PCA9468_SEC_DENOM_U_M;
			pr_info("## %s: battery,chg_float_voltage is %d\n", __func__,
				pdata->v_float);
		}

		ret = of_property_read_string(np, "battery,charger_name",
				(char const **)&pdata->sec_dc_name);
		if (ret) {
			pr_err("## %s: direct_charger is Empty\n", __func__);
			pdata->sec_dc_name = "sec-direct-charger";
		}
		pr_info("%s: battery,charger_name is %s\n", __func__, pdata->sec_dc_name);

		/* charging float voltage */
		ret = of_property_read_u32(np, "battery,chg_float_voltage",
								   &pdata->v_float);
		pdata->v_float *= PCA9468_SEC_DENOM_U_M;
		if (ret) {
			pr_info("%s: battery,dc_float_voltage is Empty\n", __func__);
			pdata->v_float = PCA9468_VFLOAT_DFT;
		}
		pr_info("%s: battery,v_float is %d\n", __func__, pdata->v_float);
	}
#endif

	return 0;
}
#else
static int pca9468_charger_parse_dt(struct device *dev, struct pca9468_platform_data *pdata)
{
	return 0;
}
#endif /* CONFIG_OF */

#ifdef CONFIG_USBPD_PHY_QCOM
static int pca9468_usbpd_setup(struct pca9468_charger *pca9468)
{
	int ret = 0;
	const char *pd_phandle = "usbpd-phy";

	pca9468->pd = devm_usbpd_get_by_phandle(pca9468->dev, pd_phandle);

	if (IS_ERR(pca9468->pd)) {
		pr_err("get_usbpd phandle failed (%ld)\n",
				PTR_ERR(pca9468->pd));
		return PTR_ERR(pca9468->pd);
	}

	return ret;
}
#endif

static int read_reg(void *data, u64 *val)
{
	struct pca9468_charger *chip = data;
	int rc;
	unsigned int temp;

	rc = regmap_read(chip->regmap, chip->debug_address, &temp); 
	if (rc) {
		pr_err("Couldn't read reg %x rc = %d\n",
			chip->debug_address, rc);
		return -EAGAIN;
	}
	*val = temp;
	return 0;
}

static int write_reg(void *data, u64 val)
{
	struct pca9468_charger *chip = data;
	int rc;
	u8 temp;

	temp = (u8) val;
	rc = regmap_write(chip->regmap, chip->debug_address, temp);
	if (rc) {
		pr_err("Couldn't write 0x%02x to 0x%02x rc= %d\n",
			temp, chip->debug_address, rc);
		return -EAGAIN;
	}
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(register_debug_ops, read_reg, write_reg, "0x%02llx\n");

static int pca9468_create_debugfs_entries(struct pca9468_charger *chip)
{
	struct dentry *ent;
	int rc = 0;
	
	chip->debug_root = debugfs_create_dir("charger-pca9468", NULL);
	if (!chip->debug_root) {
		dev_err(chip->dev, "Couldn't create debug dir\n");
		rc = -ENOENT;
	} else {
		ent = debugfs_create_x32("address", S_IFREG | S_IWUSR | S_IRUGO,
					  chip->debug_root,
					  &(chip->debug_address));
		if (!ent) {
			dev_err(chip->dev,
				"Couldn't create address debug file\n");
			rc = -ENOENT;
		}

		ent = debugfs_create_file("data", S_IFREG | S_IWUSR | S_IRUGO,
					  chip->debug_root, chip,
					  &register_debug_ops);
		if (!ent) {
			dev_err(chip->dev,
				"Couldn't create data debug file\n");
			rc = -ENOENT;
		}
	}

	return rc;
}


static int pca9468_charger_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct power_supply_config chager_cfg = {};
	struct pca9468_platform_data *pdata;
	struct device *dev = &client->dev;
	struct pca9468_charger *pca9468_chg;
	int ret;

#if defined(CONFIG_BATTERY_SAMSUNG)
	pr_info("%s: PCA9468 Charger Driver Loading\n", __func__);
#endif
	pr_info("%s: =========START=========\n", __func__);

	pca9468_chg = devm_kzalloc(dev, sizeof(*pca9468_chg), GFP_KERNEL);
	if (!pca9468_chg)
		return -ENOMEM;

#if defined(CONFIG_OF)
	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev, sizeof(struct pca9468_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory \n");
			return -ENOMEM;
		}

		ret = pca9468_charger_parse_dt(&client->dev, pdata);
		if (ret < 0){
			dev_err(&client->dev, "Failed to get device of_node \n");
			return -ENOMEM;
		}

		client->dev.platform_data = pdata;
	} else {
		pdata = client->dev.platform_data;
	}
#else
	pdata = dev->platform_data;
#endif
	if (!pdata)
		return -EINVAL;

	i2c_set_clientdata(client, pca9468_chg);

	mutex_init(&pca9468_chg->lock);
	mutex_init(&pca9468_chg->i2c_lock);
	pca9468_chg->dev = &client->dev;
	pca9468_chg->pdata = pdata;
	pca9468_chg->charging_state = DC_STATE_NO_CHARGING;
#if defined(CONFIG_BATTERY_SAMSUNG)
	pca9468_chg->wdt_kick = false;
#endif

	wakeup_source_init(&pca9468_chg->monitor_wake_lock, "pca9468-charger-monitor");

	/* initialize work */
	INIT_DELAYED_WORK(&pca9468_chg->timer_work, pca9468_timer_work);
	mutex_lock(&pca9468_chg->lock);
	pca9468_chg->timer_id = TIMER_ID_NONE;
	pca9468_chg->timer_period = 0;
	mutex_unlock(&pca9468_chg->lock);

	INIT_DELAYED_WORK(&pca9468_chg->pps_work, pca9468_pps_request_work);
#if defined(CONFIG_BATTERY_SAMSUNG) && defined(CONFIG_ENG_BATTERY_CONCEPT)
	INIT_DELAYED_WORK(&pca9468_chg->wdt_control_work, pca9468_wdt_control_work);
#endif

	pca9468_chg->regmap = devm_regmap_init_i2c(client, &pca9468_regmap);
	if (IS_ERR(pca9468_chg->regmap)) {
#if defined(CONFIG_BATTERY_SAMSUNG)
		ret = PTR_ERR(pca9468_chg->regmap);
		goto err_regmap_init;
#else
		return PTR_ERR(pca9468_chg->regmap);
#endif
	}

#ifdef CONFIG_USBPD_PHY_QCOM
	if (pca9468_usbpd_setup(pca9468_chg)) {
		dev_err(dev, "Error usbpd setup!\n");
		pca9468_chg->pd = NULL;
	}
#endif

#if defined(CONFIG_BATTERY_SAMSUNG)
	pca9468_init_adc_val(pca9468_chg, -1);
#endif

	ret = pca9468_hw_init(pca9468_chg);
	if (ret < 0)
#if defined(CONFIG_BATTERY_SAMSUNG)
		goto err_hw_init;
#else
		return ret;
#endif

	chager_cfg.supplied_to = pca9468_supplied_to;
	chager_cfg.num_supplicants = ARRAY_SIZE(pca9468_supplied_to);
	chager_cfg.drv_data = pca9468_chg;
	pca9468_chg->psy_chg = power_supply_register(dev,
		&pca9468_charger_power_supply_desc, &chager_cfg);
	if (IS_ERR(pca9468_chg->psy_chg)) {
#if defined(CONFIG_BATTERY_SAMSUNG)
		ret = PTR_ERR(pca9468_chg->psy_chg);
		goto err_power_supply_regsister;
#else
		return PTR_ERR(pca9468_chg->mains);
#endif
	}

	/*
	 * Interrupt pin is optional. If it is connected, we setup the
	 * interrupt support here.
	 */
	if (pdata->irq_gpio >= 0) {
		ret = pca9468_irq_init(pca9468_chg, client);
		if (ret < 0) {
			dev_warn(dev, "failed to initialize IRQ: %d\n", ret);
			dev_warn(dev, "disabling IRQ support\n");
		}
		/* disable interrupt */
		disable_irq(client->irq);
	}

#if defined(CONFIG_BATTERY_SAMSUNG)
	ret = gpio_request(pdata->chgen_gpio, "DC_CPEN");
	if (ret) {
		pr_info("%s : Request GPIO %d failed\n",
				__func__, (int)pdata->chgen_gpio);
	}

	gpio_direction_output(pdata->chgen_gpio,
			false);
#endif

	ret = pca9468_create_debugfs_entries(pca9468_chg);
	if (ret < 0)
		return ret;

#if defined(CONFIG_BATTERY_SAMSUNG)
	pr_info("%s: PCA9468 Charger Driver Loaded\n", __func__);
#endif
	pr_info("%s: =========END=========\n", __func__);

	return 0;

#if defined(CONFIG_BATTERY_SAMSUNG)
err_power_supply_regsister:
err_hw_init:
err_regmap_init:
	wakeup_source_trash(&pca9468_chg->monitor_wake_lock);
	return ret;
#endif
}

static int pca9468_charger_remove(struct i2c_client *client)
{
	struct pca9468_charger *pca9468_chg = i2c_get_clientdata(client);

	pr_info("%s: ++\n", __func__);

	if (client->irq) {
		free_irq(client->irq, pca9468_chg);
		gpio_free(pca9468_chg->pdata->irq_gpio);
	}

	wakeup_source_trash(&pca9468_chg->monitor_wake_lock);
#if defined(CONFIG_BATTERY_SAMSUNG)
	if (pca9468_chg->psy_chg)
		power_supply_unregister(pca9468_chg->psy_chg);
#else
	if (pca9468_chg->mains)
		power_supply_unregister(pca9468_chg->mains);
#endif

	pr_info("%s: --\n", __func__);

	return 0;
}

static void pca9468_charger_shutdown(struct i2c_client *client)
{
	struct pca9468_charger *pca9468_chg = i2c_get_clientdata(client);

	pr_info("%s: ++\n", __func__);

	pca9468_set_charging(pca9468_chg, false);

	pr_info("%s: --\n", __func__);
}

static const struct i2c_device_id pca9468_charger_id_table[] = {
	{ "pca9468-charger", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, pca9468_charger_id_table);

#if defined(CONFIG_OF)
static struct of_device_id pca9468_charger_match_table[] = {
	{ .compatible = "nxp,pca9468" },
	{ },
};
MODULE_DEVICE_TABLE(of, pca9468_charger_match_table);
#endif /* CONFIG_OF */

#if defined(CONFIG_PM)
#ifdef CONFIG_RTC_HCTOSYS
static void pca9468_check_and_update_charging_timer(struct pca9468_charger *pca9468)
{
	unsigned long current_time = 0, next_update_time, time_left;

	get_current_time(&current_time);
	
	if (pca9468->timer_id != TIMER_ID_NONE)	{
		next_update_time = pca9468->last_update_time + (pca9468->timer_period / 1000);	// unit is second

		pr_info("%s: current_time=%ld, next_update_time=%ld\n", __func__, current_time, next_update_time);

		if (next_update_time > current_time)
			time_left = next_update_time - current_time;
		else
			time_left = 0;

		mutex_lock(&pca9468->lock);
		pca9468->timer_period = time_left * 1000;	// ms unit
		mutex_unlock(&pca9468->lock);
		schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
		pr_info("%s: timer_id=%d, time_period=%ld\n", __func__, pca9468->timer_id, pca9468->timer_period);
	}
	pca9468->last_update_time = current_time;
}
#endif

static int pca9468_charger_suspend(struct device *dev)
{
	struct pca9468_charger *pca9468 = dev_get_drvdata(dev);

	pr_info("%s: cancel delayed work\n", __func__);

	/* cancel delayed_work */
	cancel_delayed_work(&pca9468->timer_work);
	return 0;
}

static int pca9468_charger_resume(struct device *dev)
{
	struct pca9468_charger *pca9468 = dev_get_drvdata(dev);

	pr_info("%s: update_timer\n", __func__);

	/* Update the current timer */
#ifdef CONFIG_RTC_HCTOSYS
	pca9468_check_and_update_charging_timer(pca9468);
#else
	if (pca9468->timer_id != TIMER_ID_NONE) {
		mutex_lock(&pca9468->lock);
		pca9468->timer_period = 0;	// ms unit
		mutex_unlock(&pca9468->lock);
		schedule_delayed_work(&pca9468->timer_work, msecs_to_jiffies(pca9468->timer_period));
	}
#endif
	return 0;
}
#else
#define pca9468_charger_suspend		NULL
#define pca9468_charger_resume		NULL
#endif

const struct dev_pm_ops pca9468_pm_ops = {
	.suspend = pca9468_charger_suspend,
	.resume = pca9468_charger_resume,
};

static struct i2c_driver pca9468_charger_driver = {
	.driver = {
		.name = "pca9468-charger",
#if defined(CONFIG_OF)
		.of_match_table = pca9468_charger_match_table,
#endif /* CONFIG_OF */
#if defined(CONFIG_PM)
		.pm = &pca9468_pm_ops,
#endif
	},
	.probe        = pca9468_charger_probe,
	.remove       = pca9468_charger_remove,
	.shutdown     = pca9468_charger_shutdown,
	.id_table     = pca9468_charger_id_table,
};

module_i2c_driver(pca9468_charger_driver);

MODULE_AUTHOR("Clark Kim <clark.kim@nxp.com>");
MODULE_DESCRIPTION("PCA9468 charger driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("3.4.10S");
