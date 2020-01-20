/*
 * s2mu004_charger.c - S2MU004 Charger Driver
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <linux/mfd/samsung/s2mu004.h>
#include <linux/power/s2mu004_charger.h>
#include <linux/version.h>
#include <linux/muic/muic.h>

#define ENABLE_MIVR 0

#define EN_OVP_IRQ 1
#define EN_IEOC_IRQ 1
#define EN_TOPOFF_IRQ 1
#define EN_RECHG_REQ_IRQ 0
#define EN_TR_IRQ 0
#define EN_MIVR_SW_REGULATION 0
#define EN_BST_IRQ 0
#define EN_BAT_DET_IRQ 0
#define MINVAL(a, b) ((a <= b) ? a : b)

#define EOC_DEBOUNCE_CNT 2
#define HEALTH_DEBOUNCE_CNT 1
#define DEFAULT_CHARGING_CURRENT 500

#define EOC_SLEEP 200
#define EOC_TIMEOUT (EOC_SLEEP * 6)
#ifndef EN_TEST_READ
#define EN_TEST_READ 1
#endif

#define ENABLE 1
#define DISABLE 0

static char *s2mu004_supplied_to[] = {
	"battery",
};

static enum power_supply_property s2mu004_charger_props[] = {
};

static enum power_supply_property s2mu004_otg_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static int s2mu004_get_charging_health(struct s2mu004_charger_data *charger);

static void s2mu004_test_read(struct i2c_client *i2c)
{
	u8 data;
	char str[1016] = {0,};
	int i;

	for (i = 0x0A; i <= 0x24; i++) {
		s2mu004_read_reg(i2c, i, &data);

		sprintf(str+strlen(str), "0x%02x:0x%02x, ", i, data);
	}
	s2mu004_read_reg(i2c, 0x33, &data);
	pr_err("%s: %s0x33:0x%02x\n", __func__, str, data);
}

static int s2mu004_charger_otg_control(
		struct s2mu004_charger_data *charger, bool enable)
{
	u8 chg_sts2, chg_ctrl0, temp;

	pr_info("%s: called charger otg control : %s\n", __func__,
			enable ? "ON" : "OFF");

	if (charger->is_charging) {
		pr_info("%s: Charger is enabled and OTG noti received!!!\n", __func__);
		pr_info("%s: is_charging: %d, otg_on: %d",
				__func__, charger->is_charging, charger->otg_on);
		s2mu004_test_read(charger->i2c);
		return 0;
	}

	if (charger->otg_on == enable)
		return 0;

	mutex_lock(&charger->charger_mutex);
	if (!enable) {
		s2mu004_update_reg(charger->i2c,
				S2MU004_CHG_CTRL0, CHG_MODE, REG_MODE_MASK);
		s2mu004_update_reg(charger->i2c, 0xAE, 0x80, 0xF0);
	} else {
		s2mu004_update_reg(charger->i2c,
				S2MU004_CHG_CTRL4,
				S2MU004_SET_OTG_OCP_1500mA << SET_OTG_OCP_SHIFT,
				SET_OTG_OCP_MASK);
		msleep(30);
		s2mu004_update_reg(charger->i2c, 0xAE, 0x00, 0xF0);
		s2mu004_update_reg(charger->i2c,
				S2MU004_CHG_CTRL0, OTG_BST_MODE, REG_MODE_MASK);
		charger->cable_type = POWER_SUPPLY_TYPE_OTG;
	}
	charger->otg_on = enable;
	mutex_unlock(&charger->charger_mutex);

	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS2, &chg_sts2);
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL0, &chg_ctrl0);
	s2mu004_read_reg(charger->i2c, 0xAE, &temp);
	pr_info("%s S2MU004_CHG_STATUS2: 0x%x\n", __func__, chg_sts2);
	pr_info("%s S2MU004_CHG_CTRL0: 0x%x\n", __func__, chg_ctrl0);
	pr_info("%s 0xAE: 0x%x\n", __func__, temp);

	power_supply_changed(charger->psy_otg);
	return enable;
}

static void s2mu004_enable_charger_switch(
	struct s2mu004_charger_data *charger, int onoff)
{
	if (charger->otg_on) {
		pr_info("[DEBUG] %s: skipped set(%d) : OTG is on\n", __func__, onoff);
		return;
	}

	if (onoff > 0) {
		pr_info("[DEBUG]%s: turn on charger\n", __func__);

		/* forced ASYNC */
		s2mu004_update_reg(charger->i2c, 0x30, 0x03, 0x03);

		mdelay(30);

		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL0, CHG_MODE, REG_MODE_MASK);

		/* timer fault set 16hr(max) */
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL16,
				S2MU004_FC_CHG_TIMER_16hr << SET_TIME_CHG_SHIFT,
				SET_TIME_CHG_MASK);

		mdelay(100);

		/* Auto SYNC to ASYNC - default */
		s2mu004_update_reg(charger->i2c, 0x30, 0x01, 0x03);

		/* async off */
		s2mu004_update_reg(charger->i2c, 0x96, 0x00, 0x01 << 3);
	} else {
		pr_info("[DEBUG] %s: turn off charger\n", __func__);

		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL0, BUCK_MODE, REG_MODE_MASK);

		/* async on */
		s2mu004_update_reg(charger->i2c, 0x96, 0x01 << 3, 0x01 << 3);
		mdelay(100);
	}
}

static void s2mu004_set_buck(
	struct s2mu004_charger_data *charger, int enable) {

	if (enable) {
		pr_info("[DEBUG]%s: set buck on\n", __func__);
		s2mu004_enable_charger_switch(charger, charger->is_charging);
	} else {
		pr_info("[DEBUG]%s: set buck off (charger off mode)\n", __func__);

		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL0, CHARGER_OFF_MODE, REG_MODE_MASK);

		/* async on */
		s2mu004_update_reg(charger->i2c, 0x96, 0x01 << 3, 0x01 << 3);
		mdelay(100);
	}
}

static void s2mu004_set_regulation_voltage(
		struct s2mu004_charger_data *charger, int float_voltage)
{
	u8 data;

	pr_info("[DEBUG]%s: float_voltage %d\n", __func__, float_voltage);
	if (float_voltage <= 3900)
		data = 0;
	else if (float_voltage > 3900 && float_voltage <= 4530)
		data = (float_voltage - 3900) / 10;
	else
		data = 0x3f;

	s2mu004_update_reg(charger->i2c,
			S2MU004_CHG_CTRL6, data << SET_VF_VBAT_SHIFT, SET_VF_VBAT_MASK);
}

static int s2mu004_get_regulation_voltage(struct s2mu004_charger_data *charger)
{
	u8 reg_data = 0;
	int float_voltage;

	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL6, &reg_data);
	reg_data &= 0x3F;
	float_voltage = reg_data * 10 + 3900;
	pr_debug("%s: battery cv reg : 0x%x, float voltage val : %d\n",
			__func__, reg_data, float_voltage);

	return float_voltage;
}

static void s2mu004_set_input_current_limit(
		struct s2mu004_charger_data *charger, int charging_current)
{
	u8 data;

	if (charging_current <= 100)
		data = 0x02;
	else if (charging_current > 100 && charging_current <= 2500)
		data = (charging_current - 50) / 25;
	else
		data = 0x62;

	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL2,
			data << INPUT_CURRENT_LIMIT_SHIFT, INPUT_CURRENT_LIMIT_MASK);

	pr_info("[DEBUG]%s: current  %d, 0x%x\n", __func__, charging_current, data);

#if EN_TEST_READ
	s2mu004_test_read(charger->i2c);
#endif
}

static int s2mu004_get_input_current_limit(struct s2mu004_charger_data *charger)
{
	u8 data;

	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL2, &data);
	if (data < 0)
		return data;

	data = data & INPUT_CURRENT_LIMIT_MASK;
	if (data > 0x62) {
		pr_err("%s: Invalid current limit in register\n", __func__);
		data = 0x62;
	}
	return  data * 25 + 50;
}

static void s2mu004_set_fast_charging_current(
		struct s2mu004_charger_data *charger, int charging_current)
{
	u8 data;

	if (charging_current <= 100)
		data = 0x03;
	else if (charging_current > 100 && charging_current <= 3150)
		data = (charging_current / 25) - 1;
	else
		data = 0x7D;

	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL9,
			data << FAST_CHARGING_CURRENT_SHIFT, FAST_CHARGING_CURRENT_MASK);

	pr_info("[DEBUG]%s: current  %d, 0x%02x\n", __func__, charging_current, data);

	if (data > 0x11)
		data = 0x11; /* 0x11 : 450mA */
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL8,
			data << COOL_CHARGING_CURRENT_SHIFT, COOL_CHARGING_CURRENT_MASK);

#if EN_TEST_READ
	s2mu004_test_read(charger->i2c);
#endif
}

static int s2mu004_get_fast_charging_current(
		struct s2mu004_charger_data *charger)
{
	u8 data;

	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL9, &data);
	if (data < 0)
		return data;

	data = data & FAST_CHARGING_CURRENT_MASK;

	if (data > 0x7D) {
		pr_err("%s: Invalid fast charging current in register\n", __func__);
		data = 0x7D;
	}
	return (data + 1) * 25;
}

static void s2mu004_set_topoff_current(
		struct s2mu004_charger_data *charger,
		int eoc_1st_2nd, int current_limit)
{
	int data;

	pr_info("[DEBUG]%s: current  %d\n", __func__, current_limit);
	if (current_limit <= 100)
		data = 0;
	else if (current_limit > 100 && current_limit <= 475)
		data = (current_limit - 100) / 25;
	else
		data = 0x0F;

	switch (eoc_1st_2nd) {
	case 1:
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL11,
				data << FIRST_TOPOFF_CURRENT_SHIFT, FIRST_TOPOFF_CURRENT_MASK);
		break;
	default:
		break;
	}
}

static int s2mu004_get_topoff_setting(
		struct s2mu004_charger_data *charger)
{
	u8 data;

	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL11, &data);
	if (data < 0)
		return data;

	data = data & FIRST_TOPOFF_CURRENT_MASK;

	if (data > 0x0F)
		data = 0x0F;
	return data * 25 + 100;
}

enum {
	S2MU004_CHG_2L_IVR_4300MV = 0,
	S2MU004_CHG_2L_IVR_4500MV,
	S2MU004_CHG_2L_IVR_4700MV,
	S2MU004_CHG_2L_IVR_4900MV,
};

#if ENABLE_MIVR
/* charger input regulation voltage setting */
static void s2mu004_set_ivr_level(struct s2mu004_charger_data *charger)
{
	int chg_2l_ivr = S2MU004_CHG_2L_IVR_4500MV;

	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL5,
			chg_2l_ivr << SET_CHG_2L_DROP_SHIFT, SET_CHG_2L_DROP_MASK);
}
#endif /*ENABLE_MIVR*/

static bool s2mu004_chg_init(struct s2mu004_charger_data *charger)
{
	u8 temp;

	/* Read Charger IC Dev ID */
	s2mu004_read_reg(charger->i2c, S2MU004_REG_REV_ID, &temp);
	charger->dev_id = (temp & 0xF0) >> 4;

	pr_info("%s : DEV ID : 0x%x\n", __func__, charger->dev_id);

	/* Poor-Chg-INT Masking */
	s2mu004_update_reg(charger->i2c, 0x32, 0x03, 0x03);

	/*
	 * When Self Discharge Function is activated, Charger doesn't stop charging.
	 * If you write 0xb0[4]=1, charger will stop the charging, when self discharge
	 * condition is satisfied.
	 */
	s2mu004_update_reg(charger->i2c, 0xb0, 0x0, 0x1 << 4);

	s2mu004_update_reg(charger->i2c, S2MU004_REG_SC_INT1_MASK,
			Poor_CHG_INT_MASK, Poor_CHG_INT_MASK);

	s2mu004_write_reg(charger->i2c, 0x02, 0x0);
	s2mu004_write_reg(charger->i2c, 0x03, 0x0);

	/* ready for self-discharge, 0x76 */
	s2mu004_update_reg(charger->i2c, S2MU004_REG_SELFDIS_CFG3,
			SELF_DISCHG_MODE_MASK, SELF_DISCHG_MODE_MASK);

	/* Set Top-Off timer to 30 minutes */
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL17,
			S2MU004_TOPOFF_TIMER_30m << TOP_OFF_TIME_SHIFT,
			TOP_OFF_TIME_MASK);

	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL17, &temp);
	pr_info("%s : S2MU004_CHG_CTRL17 : 0x%x\n", __func__, temp);

	/* enable Watchdog timer and only Charging off */
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL13,
			ENABLE << SET_EN_WDT_SHIFT | DISABLE << SET_EN_WDT_AP_RESET_SHIFT,
			SET_EN_WDT_MASK | SET_EN_WDT_AP_RESET_MASK);
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL13, &temp);
	pr_info("%s : S2MU004_CHG_CTRL13 : 0x%x\n", __func__, temp);

	/* set watchdog timer to 80 seconds */
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL17,
			S2MU004_WDT_TIMER_80s << WDT_TIME_SHIFT,
			WDT_TIME_MASK);

	/* IVR Recovery enable */
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL13,
			0x1 << SET_IVR_Recovery_SHIFT, SET_IVR_Recovery_MASK);

	/* Boost OSC 1Mhz */
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL15,
			0x02 << SET_OSC_BST_SHIFT, SET_OSC_BST_MASK);

	/* QBAT switch speed config */
	s2mu004_update_reg(charger->i2c, 0xB2, 0x0, 0xf << 4);

	/* Top off debounce time set 1 sec */
	s2mu004_update_reg(charger->i2c, 0xC0, 0x3 << 6, 0x3 << 6);

	/* SC_CTRL21 register Minumum Charging OCP Level set to 6A */
	s2mu004_write_reg(charger->i2c, 0x29, 0x04);

	switch (charger->pdata->chg_switching_freq) {
	case S2MU004_OSC_BUCK_FRQ_750kHz:
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL12,
				S2MU004_OSC_BUCK_FRQ_750kHz << SET_OSC_BUCK_SHIFT, SET_OSC_BUCK_MASK);
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL12,
				S2MU004_OSC_BUCK_FRQ_750kHz << SET_OSC_BUCK_3L_SHIFT, SET_OSC_BUCK_3L_MASK);
		break;
	default:
		/* Set OSC BUCK/BUCK 3L frequencies to default 1MHz */
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL12,
				S2MU004_OSC_BUCK_FRQ_1MHz << SET_OSC_BUCK_SHIFT, SET_OSC_BUCK_MASK);
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL12,
				S2MU004_OSC_BUCK_FRQ_1MHz << SET_OSC_BUCK_3L_SHIFT, SET_OSC_BUCK_3L_MASK);
		break;
	}
	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL12, &temp);
	pr_info("%s : S2MU004_CHG_CTRL12 : 0x%x\n", __func__, temp);

	return true;
}

static int s2mu004_get_charging_status(
		struct s2mu004_charger_data *charger)
{
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	int ret;
	u8 chg_sts0, chg_sts1;
	union power_supply_propval value;
	struct power_supply *psy;

	ret = s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS0, &chg_sts0);
	ret = s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS1, &chg_sts1);

	psy = power_supply_get_by_name(charger->pdata->fuelgauge_name);
	if (!psy)
		return -EINVAL;
	ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_CURRENT_AVG, &value);
	if (ret < 0)
		pr_err("%s: Fail to execute property\n", __func__);

	if (ret < 0)
		return status;

	if (chg_sts1 & 0x80)
		status = POWER_SUPPLY_STATUS_DISCHARGING;
	else if (chg_sts1 & 0x02 || chg_sts1 & 0x01) {
		pr_info("%s: full check curr_avg(%d), topoff_curr(%d)\n",
				__func__, value.intval, charger->topoff_current);
		if (value.intval < charger->topoff_current)
			status = POWER_SUPPLY_STATUS_FULL;
		else
			status = POWER_SUPPLY_STATUS_CHARGING;
	} else if ((chg_sts0 & 0xE0) == 0xA0 || (chg_sts0 & 0xE0) == 0x60)
		status = POWER_SUPPLY_STATUS_CHARGING;
	else
		status = POWER_SUPPLY_STATUS_NOT_CHARGING;

#if EN_TEST_READ
	s2mu004_test_read(charger->i2c);
#endif
	return status;
}

static int s2mu004_get_charge_type(struct s2mu004_charger_data *charger)
{
	int status = POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;
	u8 ret;

	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS3, &ret);
	if (ret < 0)
		pr_err("%s fail\n", __func__);

	switch ((ret & BAT_STATUS_MASK) >> BAT_STATUS_SHIFT) {
	case 0x4:
	case 0x5:
		status = POWER_SUPPLY_CHARGE_TYPE_FAST;
		break;
	case 0x2:
		/* pre-charge mode */
		status = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
		break;
	}
	return status;
}

static bool s2mu004_get_batt_present(struct s2mu004_charger_data *charger)
{
	u8 ret;

	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS3, &ret);
	if (ret < 0)
		return false;

	return (ret & DET_BAT_STATUS_MASK) ? true : false;
}

static void s2mu004_wdt_clear(struct s2mu004_charger_data *charger)
{
	u8 reg_data, chg_fault_status, en_chg;

	/* watchdog kick */
	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL14,
			0x1 << WDT_CLR_SHIFT, WDT_CLR_MASK);

	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS1, &reg_data);
	chg_fault_status = (reg_data & CHG_FAULT_STATUS_MASK) >> CHG_FAULT_STATUS_SHIFT;

	if ((chg_fault_status == CHG_STATUS_WD_SUSPEND) ||
			(chg_fault_status == CHG_STATUS_WD_RST)) {
		pr_info("%s: watchdog error status(0x%02x,%d)\n",
				__func__, reg_data, chg_fault_status);
		if (charger->is_charging) {
			pr_info("%s: toggle charger\n", __func__);
			s2mu004_enable_charger_switch(charger, false);
			s2mu004_enable_charger_switch(charger, true);
		}
	}

	s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL0, &en_chg);
	if (!(en_chg & 0x80))
		s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL0,
				0x1 << EN_CHG_SHIFT, EN_CHG_MASK);
}

static int s2mu004_get_charging_health(struct s2mu004_charger_data *charger)
{

	u8 ret;
	union power_supply_propval value;
	struct power_supply *psy;

	if (charger->is_charging)
		s2mu004_wdt_clear(charger);

	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS0, &ret);
	pr_info("[DEBUG] %s: S2MU004_CHG_STATUS0 0x%x\n", __func__, ret);
	if (ret < 0)
		return POWER_SUPPLY_HEALTH_UNKNOWN;

	ret = (ret & (CHGIN_STATUS_MASK)) >> CHGIN_STATUS_SHIFT;

	switch (ret) {
	case 0x03:
	case 0x05:
		charger->ovp = false;
		charger->unhealth_cnt = 0;
		return POWER_SUPPLY_HEALTH_GOOD;
	default:
		break;
	}

	charger->unhealth_cnt++;
	if (charger->unhealth_cnt < HEALTH_DEBOUNCE_CNT)
		return POWER_SUPPLY_HEALTH_GOOD;

	/* 005 need to check ovp & health count */
	charger->unhealth_cnt = HEALTH_DEBOUNCE_CNT;
	if (charger->ovp)
		return POWER_SUPPLY_HEALTH_OVERVOLTAGE;

	psy = power_supply_get_by_name("battery");
	if (!psy)
		return -EINVAL;
	ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
	if (ret < 0)
		pr_err("%s: Fail to execute property\n", __func__);

	if (value.intval == POWER_SUPPLY_TYPE_USB_PD)
		return POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
	else
		return POWER_SUPPLY_HEALTH_GOOD;

#if EN_TEST_READ
	s2mu004_test_read(charger->i2c);
#endif
}

static int s2mu004_chg_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	int chg_curr, aicr;
	struct s2mu004_charger_data *charger =
		power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = charger->is_charging ? 1 : 0;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = s2mu004_get_charging_status(charger);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = s2mu004_get_charging_health(charger);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = s2mu004_get_input_current_limit(charger);
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (charger->charging_current) {
			aicr = s2mu004_get_input_current_limit(charger);
			chg_curr = s2mu004_get_fast_charging_current(charger);
			val->intval = MINVAL(aicr, chg_curr);
		} else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		val->intval = s2mu004_get_fast_charging_current(charger);
		break;
	case POWER_SUPPLY_PROP_CURRENT_FULL:
		val->intval = s2mu004_get_topoff_setting(charger);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = s2mu004_get_charge_type(charger);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = s2mu004_get_regulation_voltage(charger);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = s2mu004_get_batt_present(charger);
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		val->intval = charger->is_charging;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int s2mu004_chg_set_property(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	struct s2mu004_charger_data *charger = power_supply_get_drvdata(psy);
	int buck_state = ENABLE;
	union power_supply_propval value;
	int ret;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		charger->status = val->intval;
		break;
		/* val->intval : type */
	case POWER_SUPPLY_PROP_ONLINE:
		charger->cable_type = val->intval;
		if (charger->cable_type != POWER_SUPPLY_TYPE_OTG) {
			if (charger->cable_type == POWER_SUPPLY_TYPE_BATTERY ||
			charger->cable_type == POWER_SUPPLY_TYPE_UNKNOWN) {
				pr_err("[DEBUG]%s:[BATT] Type Battery\n", __func__);
				value.intval = 0;
			} else {
#if ENABLE_MIVR
				s2mu004_set_ivr_level(charger);
#endif
				value.intval = 1;
			}

			psy = power_supply_get_by_name(charger->pdata->fuelgauge_name);
			if (!psy)
				return -EINVAL;
			ret = power_supply_set_property(psy, POWER_SUPPLY_PROP_ENERGY_AVG, &value);
			if (ret < 0)
				pr_err("%s: Fail to execute property\n", __func__);
		}
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		{
			int input_current = val->intval;

			s2mu004_set_input_current_limit(charger, input_current);
			charger->input_current = input_current;
		}
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		pr_info("[DEBUG] %s: is_charging %d\n", __func__, charger->is_charging);
		charger->charging_current = val->intval;
		/* set charging current */
		if (charger->is_charging)
			s2mu004_set_fast_charging_current(charger, charger->charging_current);
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		break;
	case POWER_SUPPLY_PROP_CURRENT_FULL:
		charger->topoff_current = val->intval;
		s2mu004_set_topoff_current(charger, 1, val->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		pr_info("[DEBUG]%s: float voltage(%d)\n", __func__, val->intval);
		charger->pdata->chg_float_voltage = val->intval;
		s2mu004_set_regulation_voltage(charger,
				charger->pdata->chg_float_voltage);
	case POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL:
		s2mu004_charger_otg_control(charger, val->intval);
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		charger->charge_mode = val->intval;

		psy = power_supply_get_by_name("battery");
		if (!psy)
			return -EINVAL;
		ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
		if (ret < 0)
			pr_err("%s: Fail to execute property\n", __func__);

		if (value.intval != POWER_SUPPLY_TYPE_OTG) {
			switch (charger->charge_mode) {
			case S2MU00X_BAT_CHG_MODE_BUCK_OFF:
				buck_state = DISABLE;
			case S2MU00X_BAT_CHG_MODE_CHARGING_OFF:
				charger->is_charging = false;
				break;
			case S2MU00X_BAT_CHG_MODE_CHARGING:
				charger->is_charging = true;
				break;
			}
			value.intval = charger->is_charging;

			psy = power_supply_get_by_name(charger->pdata->fuelgauge_name);
			if (!psy)
				return -EINVAL;
			ret = power_supply_set_property(psy, POWER_SUPPLY_PROP_CHARGING_ENABLED, &value);
			if (ret < 0)
				pr_err("%s: Fail to execute property\n", __func__);

			if (buck_state)
				s2mu004_enable_charger_switch(charger, charger->is_charging);
			else
				s2mu004_set_buck(charger, buck_state);
		} else {
			pr_info("[DEBUG]%s: SKIP CHARGING CONTROL while OTG(%d)\n",
					__func__, value.intval);
		}
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		break;
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION:
		if (val->intval) {
			pr_debug("%s: Relieve VBUS2BAT\n", __func__);
			s2mu004_write_reg(charger->i2c, 0x2F, 0x5D);
		}
		break;
	case POWER_SUPPLY_PROP_FUELGAUGE_RESET:
		s2mu004_write_reg(charger->i2c, 0x6F, 0xC4);
		msleep(1000);
		s2mu004_write_reg(charger->i2c, 0x6F, 0x04);
		msleep(50);
		pr_info("%s: reset fuelgauge when surge occur!\n", __func__);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int s2mu004_otg_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct s2mu004_charger_data *charger = power_supply_get_drvdata(psy);
	u8 reg;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = charger->otg_on;
		break;
	case POWER_SUPPLY_PROP_CHARGE_POWERED_OTG_CONTROL:
		s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS2, &reg);
		pr_info("%s: S2MU004_CHG_STATUS2 : 0x%X\n", __func__, reg);
		if ((reg & 0xE0) == 0x60)
			val->intval = 1;
		else
			val->intval = 0;
		s2mu004_read_reg(charger->i2c, S2MU004_CHG_CTRL0, &reg);
		pr_info("%s: S2MU004_CHG_CTRL0 : 0x%X\n", __func__, reg);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int s2mu004_otg_set_property(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	struct s2mu004_charger_data *charger =  power_supply_get_drvdata(psy);
	union power_supply_propval value;
	int ret;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		value.intval = val->intval;
		pr_info("%s: OTG %s\n", __func__, value.intval > 0 ? "ON" : "OFF");

		psy = power_supply_get_by_name(charger->pdata->charger_name);
		if (!psy)
			return -EINVAL;
		ret = power_supply_set_property(psy, POWER_SUPPLY_PROP_CHARGE_OTG_CONTROL, &value);
		if (ret < 0)
			pr_err("%s: Fail to execute property\n", __func__);

		power_supply_changed(charger->psy_otg);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void s2mu004_charger_otg_vbus_work(struct work_struct *work)
{
	struct s2mu004_charger_data *charger = container_of(work,
			struct s2mu004_charger_data,
			otg_vbus_work.work);

	s2mu004_update_reg(charger->i2c, S2MU004_CHG_CTRL7, 0x2 << SET_VF_VBYP_SHIFT, SET_VF_VBYP_MASK);
}

#if EN_BAT_DET_IRQ
/* s2mu004 interrupt service routine */
static irqreturn_t s2mu004_det_bat_isr(int irq, void *data)
{
	struct s2mu004_charger_data *charger = data;
	u8 val;

	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS3, &val);
	if ((val & DET_BAT_STATUS_MASK) == 0) {
		s2mu004_enable_charger_switch(charger, 0);
		pr_err("charger-off if battery removed\n");
	}
	return IRQ_HANDLED;
}
#endif

static irqreturn_t s2mu004_done_isr(int irq, void *data)
{
	struct s2mu004_charger_data *charger = data;
	u8 val;

	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS1, &val);
	pr_info("%s , %02x\n", __func__, val);
	if (val & (DONE_STATUS_MASK)) {
		pr_err("add self chg done\n");
		/* add chg done code here */
	}
	return IRQ_HANDLED;
}

static irqreturn_t s2mu004_chg_isr(int irq, void *data)
{
	struct s2mu004_charger_data *charger = data;
	union power_supply_propval value;
	u8 val;
	struct power_supply *psy;
	int ret;

	value.intval = POWER_SUPPLY_HEALTH_GOOD;

	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS0, &val);
	pr_info("%s , %02x\n", __func__, val);
#if EN_OVP_IRQ
	if ((val & CHGIN_STATUS_MASK) == (2 << CHGIN_STATUS_SHIFT)) {
		charger->ovp = true;
		pr_info("%s: OVP triggered\n", __func__);
		value.intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		s2mu004_update_reg(charger->i2c, 0xBE, 0x10, 0x10);
	} else if ((val & CHGIN_STATUS_MASK) == (3 << CHGIN_STATUS_SHIFT) ||
			(val & CHGIN_STATUS_MASK) == (5 << CHGIN_STATUS_SHIFT)) {
		dev_dbg(&charger->i2c->dev, "%s: Vbus status 0x%x\n", __func__, val);
		charger->unhealth_cnt = HEALTH_DEBOUNCE_CNT;
		charger->ovp = false;
		pr_info("%s: recover from OVP\n", __func__);
		value.intval = POWER_SUPPLY_HEALTH_GOOD;
		s2mu004_update_reg(charger->i2c, 0xBE, 0x00, 0x10);
	}

	psy = power_supply_get_by_name("battery");
	if (!psy)
		return -EINVAL;
	ret = power_supply_set_property(psy, POWER_SUPPLY_PROP_HEALTH, &value);
	if (ret < 0)
		pr_err("%s: Fail to execute property\n", __func__);

#endif
	return IRQ_HANDLED;
}

static irqreturn_t s2mu004_event_isr(int irq, void *data)
{
	struct s2mu004_charger_data *charger = data;
	u8 val;

	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS0, &val);
	pr_info("%s , %02x\n", __func__, val);
	return IRQ_HANDLED;
}

static irqreturn_t s2mu004_ovp_isr(int irq, void *data)
{
	struct s2mu004_charger_data *charger = data;
	u8 val;

	s2mu004_read_reg(charger->i2c, S2MU004_CHG_STATUS0, &val);
	pr_info("%s ovp %02x\n", __func__, val);

	return IRQ_HANDLED;
}

static int s2mu004_charger_parse_dt(struct device *dev,
		struct s2mu004_charger_platform_data *pdata)
{
	struct device_node *np = of_find_node_by_name(NULL, "s2mu004-charger");
	int ret = 0;

	if (!np) {
		pr_err("%s np NULL(s2mu004-charger)\n", __func__);
	} else {
		ret = of_property_read_u32(np, "battery,chg_switching_freq",
				&pdata->chg_switching_freq);
		if (ret < 0)
			pr_info("%s: Charger switching FRQ is Empty\n", __func__);
	}

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s np NULL\n", __func__);
	} else {
		int len;
		unsigned int i;
		const u32 *p;

		ret = of_property_read_string(np,
				"battery,fuelgauge_name",
				(char const **)&pdata->fuelgauge_name);
		if (ret < 0)
			pr_info("%s: Fuel-gauge name is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,chg_float_voltage",
				&pdata->chg_float_voltage);
		if (ret) {
			pr_info("%s: battery,chg_float_voltage is Empty\n", __func__);
			pdata->chg_float_voltage = 4200;
		}
		pr_info("%s: battery,chg_float_voltage is %d\n",
				__func__, pdata->chg_float_voltage);

		pdata->chg_eoc_dualpath = of_property_read_bool(np,
				"battery,chg_eoc_dualpath");

		p = of_get_property(np, "battery,input_current_limit", &len);
		if (!p)
			return 1;

		len = len / sizeof(u32);

		pdata->charging_current =
			kzalloc(sizeof(s2mu00x_charging_current_t) * len,
					GFP_KERNEL);

		for (i = 0; i < len; i++) {
			ret = of_property_read_u32_index(np,
					"battery,input_current_limit", i,
					&pdata->charging_current[i].input_current_limit);
			if (ret)
				pr_info("%s : Input_current_limit is Empty\n",
						__func__);

			ret = of_property_read_u32_index(np,
					"battery,fast_charging_current", i,
					&pdata->charging_current[i].fast_charging_current);
			if (ret)
				pr_info("%s : Fast charging current is Empty\n",
						__func__);

			ret = of_property_read_u32_index(np,
					"battery,full_check_current", i,
					&pdata->charging_current[i].full_check_current);
			if (ret)
				pr_info("%s : Full check current is Empty\n",
						__func__);
		}
	}

	pr_info("%s DT file parsed succesfully, %d\n", __func__, ret);
	return ret;
}

/* if need to set s2mu004 pdata */
static const struct of_device_id s2mu004_charger_match_table[] = {
	{ .compatible = "samsung,s2mu004-charger",},
	{},
};

static int s2mu004_charger_probe(struct platform_device *pdev)
{
	struct s2mu004_dev *s2mu004 = dev_get_drvdata(pdev->dev.parent);
	struct s2mu004_platform_data *pdata = dev_get_platdata(s2mu004->dev);
	struct s2mu004_charger_data *charger;
	struct power_supply_config psy_cfg = {};
	int ret = 0;

	pr_info("%s:[BATT] S2MU004 Charger driver probe\n", __func__);
	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (!charger)
		return -ENOMEM;

	mutex_init(&charger->charger_mutex);
	charger->otg_on = false;

	charger->dev = &pdev->dev;
	charger->i2c = s2mu004->i2c;

	charger->pdata = devm_kzalloc(&pdev->dev, sizeof(*(charger->pdata)),
			GFP_KERNEL);
	if (!charger->pdata) {
		ret = -ENOMEM;
		goto err_parse_dt_nomem;
	}
	ret = s2mu004_charger_parse_dt(&pdev->dev, charger->pdata);
	if (ret < 0)
		goto err_parse_dt;

	platform_set_drvdata(pdev, charger);

	if (charger->pdata->charger_name == NULL)
		charger->pdata->charger_name = "s2mu004-charger";
	if (charger->pdata->fuelgauge_name == NULL)
		charger->pdata->fuelgauge_name = "s2mu004-fuelgauge";

	charger->psy_chg_desc.name           = charger->pdata->charger_name;
	charger->psy_chg_desc.type           = POWER_SUPPLY_TYPE_UNKNOWN;
	charger->psy_chg_desc.get_property   = s2mu004_chg_get_property;
	charger->psy_chg_desc.set_property   = s2mu004_chg_set_property;
	charger->psy_chg_desc.properties     = s2mu004_charger_props;
	charger->psy_chg_desc.num_properties = ARRAY_SIZE(s2mu004_charger_props);

	charger->psy_otg_desc.name           = "otg";
	charger->psy_otg_desc.type           = POWER_SUPPLY_TYPE_UNKNOWN;
	charger->psy_otg_desc.get_property   = s2mu004_otg_get_property;
	charger->psy_otg_desc.set_property   = s2mu004_otg_set_property;
	charger->psy_otg_desc.properties     = s2mu004_otg_props;
	charger->psy_otg_desc.num_properties = ARRAY_SIZE(s2mu004_otg_props);

	s2mu004_chg_init(charger);
	charger->input_current = s2mu004_get_input_current_limit(charger);
	charger->charging_current = s2mu004_get_fast_charging_current(charger);

	psy_cfg.drv_data = charger;
	psy_cfg.supplied_to = s2mu004_supplied_to;
	psy_cfg.num_supplicants = ARRAY_SIZE(s2mu004_supplied_to);

	charger->psy_chg = power_supply_register(&pdev->dev, &charger->psy_chg_desc, &psy_cfg);
	if (IS_ERR(charger->psy_chg)) {
		pr_err("%s: Failed to Register psy_chg\n", __func__);
		ret = PTR_ERR(charger->psy_chg);
		goto err_power_supply_register;
	}

	charger->psy_otg = power_supply_register(&pdev->dev, &charger->psy_otg_desc, &psy_cfg);
	if (IS_ERR(charger->psy_otg)) {
		pr_err("%s: Failed to Register psy_otg\n", __func__);
		ret = PTR_ERR(charger->psy_otg);
		goto err_power_supply_register_otg;
	}

	charger->charger_wqueue = create_singlethread_workqueue("charger-wq");
	if (!charger->charger_wqueue) {
		pr_info("%s: failed to create wq.\n", __func__);
		ret = -ESRCH;
		goto err_create_wq;
	}

	/*
	 * irq request
	 * if you need to add irq , please refer below code.
	 */
	charger->irq_sys = pdata->irq_base + S2MU004_CHG1_IRQ_SYS;
	ret = request_threaded_irq(charger->irq_sys, NULL,
			s2mu004_ovp_isr, 0, "sys-irq", charger);
	if (ret < 0) {
		dev_err(s2mu004->dev, "%s: Fail to request SYS in IRQ: %d: %d\n",
				__func__, charger->irq_sys, ret);
		goto err_reg_irq;
	}

#if EN_BAT_DET_IRQ
	charger->irq_det_bat = pdata->irq_base + S2MU004_CHG2_IRQ_DET_BAT;
	ret = request_threaded_irq(charger->irq_det_bat, NULL,
			s2mu004_det_bat_isr, 0, "det_bat-irq", charger);
	if (ret < 0) {
		dev_err(s2mu004->dev, "%s: Fail to request DET_BAT in IRQ: %d: %d\n",
				__func__, charger->irq_det_bat, ret);
		goto err_reg_irq;
	}
#endif

	charger->irq_chgin = pdata->irq_base + S2MU004_CHG1_IRQ_CHGIN;
	ret = request_threaded_irq(charger->irq_chgin, NULL,
			s2mu004_chg_isr, 0, "chgin-irq", charger);
	if (ret < 0) {
		dev_err(s2mu004->dev, "%s: Fail to request CHGIN in IRQ: %d: %d\n",
				__func__, charger->irq_chgin, ret);
		goto err_reg_irq;
	}

	charger->irq_rst = pdata->irq_base + S2MU004_CHG1_IRQ_CHG_RSTART;
	ret = request_threaded_irq(charger->irq_rst, NULL,
			s2mu004_chg_isr, 0, "restart-irq", charger);
	if (ret < 0) {
		dev_err(s2mu004->dev, "%s: Fail to request CHG_Restart in IRQ: %d: %d\n",
				__func__, charger->irq_rst, ret);
		goto err_reg_irq;
	}

	charger->irq_done = pdata->irq_base + S2MU004_CHG1_IRQ_DONE;
	ret = request_threaded_irq(charger->irq_done, NULL,
			s2mu004_done_isr, 0, "done-irq", charger);
	if (ret < 0) {
		dev_err(s2mu004->dev, "%s: Fail to request DONE in IRQ: %d: %d\n",
				__func__, charger->irq_done, ret);
		goto err_reg_irq;
	}

	charger->irq_chg_fault = pdata->irq_base + S2MU004_CHG1_IRQ_CHG_Fault;
	ret = request_threaded_irq(charger->irq_chg_fault, NULL,
			s2mu004_event_isr, 0, "chg_fault-irq", charger);
	if (ret < 0) {
		dev_err(s2mu004->dev, "%s: Fail to request CHG_Fault in IRQ: %d: %d\n",
				__func__, charger->irq_chg_fault, ret);
		goto err_reg_irq;
	}

	INIT_DELAYED_WORK(&charger->otg_vbus_work, s2mu004_charger_otg_vbus_work);

#if EN_TEST_READ
	s2mu004_test_read(charger->i2c);
#endif
	pr_info("%s:[BATT] S2MU004 charger driver loaded OK\n", __func__);

	return 0;

err_create_wq:
	destroy_workqueue(charger->charger_wqueue);
err_reg_irq:
	power_supply_unregister(charger->psy_otg);
err_power_supply_register_otg:
	power_supply_unregister(charger->psy_chg);
err_power_supply_register:
err_parse_dt:
err_parse_dt_nomem:
	mutex_destroy(&charger->charger_mutex);
	kfree(charger);
	return ret;
}

static int s2mu004_charger_remove(struct platform_device *pdev)
{
	struct s2mu004_charger_data *charger =
		platform_get_drvdata(pdev);

	power_supply_unregister(charger->psy_chg);
	mutex_destroy(&charger->charger_mutex);
	kfree(charger);
	return 0;
}

#if defined CONFIG_PM
static int s2mu004_charger_suspend(struct device *dev)
{
	return 0;
}

static int s2mu004_charger_resume(struct device *dev)
{
	return 0;
}
#else
#define s2mu004_charger_suspend NULL
#define s2mu004_charger_resume NULL
#endif

static void s2mu004_charger_shutdown(struct device *dev)
{
	pr_info("%s: S2MU004 Charger driver shutdown\n", __func__);
}

static SIMPLE_DEV_PM_OPS(s2mu004_charger_pm_ops, s2mu004_charger_suspend,
		s2mu004_charger_resume);

static struct platform_driver s2mu004_charger_driver = {
	.driver         = {
		.name   = "s2mu004-charger",
		.owner  = THIS_MODULE,
		.of_match_table = s2mu004_charger_match_table,
		.pm     = &s2mu004_charger_pm_ops,
		.shutdown   =   s2mu004_charger_shutdown,
	},
	.probe          = s2mu004_charger_probe,
	.remove     = s2mu004_charger_remove,
};

static int __init s2mu004_charger_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&s2mu004_charger_driver);

	return ret;
}
module_init(s2mu004_charger_init);

static void __exit s2mu004_charger_exit(void)
{
	platform_driver_unregister(&s2mu004_charger_driver);
}
module_exit(s2mu004_charger_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Charger driver for S2MU004");
