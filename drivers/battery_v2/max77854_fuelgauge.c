/*
 *  max77854_fuelgauge.c
 *  Samsung MAX77854 Fuel Gauge Driver
 *
 *  Copyright (C) 2015 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG
/* #define BATTERY_LOG_MESSAGE */

#include <linux/mfd/max77854-private.h>
#include <linux/of_gpio.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include "include/fuelgauge/max77854_fuelgauge.h"

static enum power_supply_property max77854_fuelgauge_props[] = {
};

bool max77854_fg_fuelalert_init(struct max77854_fuelgauge_data *fuelgauge,
				int soc);

#if !defined(CONFIG_SEC_FACTORY)
static void max77854_fg_periodic_read(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 reg;
	int i;
	int data[0x10];
	char *str = NULL;

	str = kzalloc(sizeof(char)*1024, GFP_KERNEL);
	if (!str)
		return;

	for (i = 0; i < 16; i++) {
		if (i == 5)
			i = 11;
		else if (i == 12)
			i = 13;
		for (reg = 0; reg < 0x10; reg++) {
			data[reg] = max77854_read_word(fuelgauge->i2c, reg + i * 0x10);
			if (data[reg] < 0) {
				kfree(str);
				return;
			}
		}
		sprintf(str+strlen(str),
			"%04xh,%04xh,%04xh,%04xh,%04xh,%04xh,%04xh,%04xh,",
			data[0x00], data[0x01], data[0x02], data[0x03],
			data[0x04], data[0x05], data[0x06], data[0x07]);
		sprintf(str+strlen(str),
			"%04xh,%04xh,%04xh,%04xh,%04xh,%04xh,%04xh,%04xh,",
			data[0x08], data[0x09], data[0x0a], data[0x0b],
			data[0x0c], data[0x0d], data[0x0e], data[0x0f]);
		if (!fuelgauge->initial_update_of_soc) {
			msleep(1);
		}
	}

	pr_info("[FG] %s\n", str);

	kfree(str);
}
#endif

static int max77854_fg_read_vcell(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	u32 vcell;
	u16 w_data;
	u32 temp;
	u32 temp2;

	if (max77854_bulk_read(fuelgauge->i2c, VCELL_REG, 2, data) < 0) {
		pr_err("%s: Failed to read VCELL\n", __func__);
		return -1;
	}

	w_data = (data[1]<<8) | data[0];

	temp = (w_data & 0xFFF) * 78125;
	vcell = temp / 1000000;

	temp = ((w_data & 0xF000) >> 4) * 78125;
	temp2 = temp / 1000000;
	vcell += (temp2 << 4);

	if (!(fuelgauge->info.pr_cnt++ % PRINT_COUNT)) {
		fuelgauge->info.pr_cnt = 1;
		pr_info("%s: VCELL(%d), data(0x%04x)\n",
			__func__, vcell, (data[1]<<8) | data[0]);
	}

	if ((fuelgauge->vempty_mode == VEMPTY_MODE_SW_VALERT) && 
		(vcell >= fuelgauge->battery_data->sw_v_empty_recover_vol)) {
		fuelgauge->vempty_mode = VEMPTY_MODE_SW_RECOVERY;
		max77854_fg_fuelalert_init(fuelgauge,
					   fuelgauge->pdata->fuel_alert_soc);
		pr_info("%s : Recoverd from SW V EMPTY Activation\n", __func__);
	}

	return vcell;
}

static int max77854_fg_read_vfocv(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	u32 vfocv = 0;
	u16 w_data;
	u32 temp;
	u32 temp2;

	if (max77854_bulk_read(fuelgauge->i2c, VFOCV_REG, 2, data) < 0) {
		pr_err("%s: Failed to read VFOCV\n", __func__);
		return -1;
	}

	w_data = (data[1]<<8) | data[0];

	temp = (w_data & 0xFFF) * 78125;
	vfocv = temp / 1000000;

	temp = ((w_data & 0xF000) >> 4) * 78125;
	temp2 = temp / 1000000;
	vfocv += (temp2 << 4);

#if !defined(CONFIG_SEC_FACTORY)
	max77854_fg_periodic_read(fuelgauge);
#endif

	return vfocv;
}

static int max77854_fg_read_avg_vcell(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	u32 avg_vcell = 0;
	u16 w_data;
	u32 temp;
	u32 temp2;

	if (max77854_bulk_read(fuelgauge->i2c, AVR_VCELL_REG,
			       2, data) < 0) {
		pr_err("%s: Failed to read AVG_VCELL\n", __func__);
		return -1;
	}

	w_data = (data[1]<<8) | data[0];

	temp = (w_data & 0xFFF) * 78125;
	avg_vcell = temp / 1000000;

	temp = ((w_data & 0xF000) >> 4) * 78125;
	temp2 = temp / 1000000;
	avg_vcell += (temp2 << 4);

	return avg_vcell;
}

static int max77854_fg_check_battery_present(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 status_data[2];
	int ret = 1;

	/* 1. Check Bst bit */
	if (max77854_bulk_read(fuelgauge->i2c, STATUS_REG,
			       2, status_data) < 0) {
		pr_err("%s: Failed to read STATUS_REG\n", __func__);
		return 0;
	}

	if (status_data[0] & (0x1 << 3)) {
		pr_info("%s: addr(0x01), data(0x%04x)\n", __func__,
			(status_data[1]<<8) | status_data[0]);
		pr_info("%s: battery is absent!!\n", __func__);
		ret = 0;
	}

	return ret;
}

static void max77854_fg_set_vempty(struct max77854_fuelgauge_data *fuelgauge, int vempty_mode)
{
	u16 data = 0;
	u8 valrt_data[2] = {0,};

	if (!fuelgauge->using_temp_compensation) {
		pr_info("%s: does not use temp compensation, default hw vempty\n", __func__);
		vempty_mode = VEMPTY_MODE_HW;
	}

	fuelgauge->vempty_mode = vempty_mode;
	switch (vempty_mode) {
	case VEMPTY_MODE_SW:
		/* HW Vempty Disable */
		max77854_write_word(fuelgauge->i2c, VEMPTY_REG, fuelgauge->battery_data->V_empty_origin);
		/* Reset VALRT Threshold setting (enable) */
		valrt_data[1] = 0xFF;
		valrt_data[0] = fuelgauge->battery_data->sw_v_empty_vol / 20;
		if (max77854_bulk_write(fuelgauge->i2c, VALRT_THRESHOLD_REG,
				2, valrt_data) < 0) {
			pr_info("%s: Failed to write VALRT_THRESHOLD_REG\n", __func__);
			return;
		}
		data = max77854_read_word(fuelgauge->i2c, (u8)VALRT_THRESHOLD_REG);
		pr_info("%s: HW V EMPTY Disable, SW V EMPTY Enable with %d mV (%d) \n",
			__func__, fuelgauge->battery_data->sw_v_empty_vol, (data&0x00ff)*20);
		break;
	default:
		/* HW Vempty Enable */
		max77854_write_word(fuelgauge->i2c, VEMPTY_REG, fuelgauge->battery_data->V_empty);
		/* Reset VALRT Threshold setting (disable) */
		valrt_data[1] = 0xFF;
		valrt_data[0] = 0;
		if (max77854_bulk_write(fuelgauge->i2c, VALRT_THRESHOLD_REG,
				2, valrt_data) < 0) {
			pr_info("%s: Failed to write VALRT_THRESHOLD_REG\n", __func__);
			return;
		}
		data = max77854_read_word(fuelgauge->i2c, (u8)VALRT_THRESHOLD_REG);
		pr_info("%s: HW V EMPTY Enable, SW V EMPTY Disable %d mV (%d) \n",
			__func__, 0, (data&0x00ff)*20);
		break;
	}
}

static int max77854_fg_write_temp(struct max77854_fuelgauge_data *fuelgauge,
			 int temperature)
{
	u8 data[2];

	data[0] = (temperature%10) * 1000 / 39;
	data[1] = temperature / 10;
	max77854_bulk_write(fuelgauge->i2c, TEMPERATURE_REG, 2, data);

	pr_debug("%s: temperature to (%d, 0x%02x%02x)\n",
		__func__, temperature, data[1], data[0]);

	fuelgauge->temperature = temperature;
	return temperature;
}

static int max77854_fg_read_temp(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 data[2] = {0, 0};
	int temper = 0;

	if (max77854_fg_check_battery_present(fuelgauge)) {
		if (max77854_bulk_read(fuelgauge->i2c,
				       TEMPERATURE_REG, 2, data) < 0) {
			pr_err("%s: Failed to read TEMPERATURE_REG\n",
				__func__);
			return -1;
		}

		if (data[1]&(0x1 << 7)) {
			temper = ((~(data[1]))&0xFF)+1;
			temper *= (-1000);
			temper -= ((~((int)data[0]))+1) * 39 / 10;
		} else {
			temper = data[1] & 0x7f;
			temper *= 1000;
			temper += data[0] * 39 / 10;
		}
	} else
		temper = 20000;

	if (!(fuelgauge->info.pr_cnt % PRINT_COUNT))
		pr_info("%s: TEMPERATURE(%d), data(0x%04x)\n",
			__func__, temper, (data[1]<<8) | data[0]);

	return temper/100;
}

/* soc should be 0.1% unit */
static int max77854_fg_read_vfsoc(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	int soc;

	if (max77854_bulk_read(fuelgauge->i2c, VFSOC_REG,
			       2, data) < 0) {
		pr_err("%s: Failed to read VFSOC\n", __func__);
		return -1;
	}

	soc = ((data[1] * 100) + (data[0] * 100 / 256)) / 10;

	return min(soc, 1000);
}

/* soc should be 0.1% unit */
static int max77854_fg_read_avsoc(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	int soc;

	if (max77854_bulk_read(fuelgauge->i2c, SOCAV_REG,
			       2, data) < 0) {
		pr_err("%s: Failed to read AVSOC\n", __func__);
		return -1;
	}

	soc = ((data[1] * 100) + (data[0] * 100 / 256)) / 10;

	return min(soc, 1000);
}

/* soc should be 0.1% unit */
static int max77854_fg_read_soc(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	int soc, vf_soc;

	if (max77854_bulk_read(fuelgauge->i2c, SOCREP_REG, 2, data) < 0) {
		pr_err("%s: Failed to read SOCREP\n", __func__);
		return -1;
	}

	soc = ((data[1] * 100) + (data[0] * 100 / 256)) / 10;
	vf_soc = max77854_fg_read_vfsoc(fuelgauge);

#ifdef BATTERY_LOG_MESSAGE
	pr_debug("%s: raw capacity (%d)\n", __func__, soc);

	if (!(fuelgauge->info.pr_cnt % PRINT_COUNT)) {
		pr_debug("%s: raw capacity (%d), data(0x%04x)\n",
			 __func__, soc, (data[1]<<8) | data[0]);
		pr_debug("%s: REPSOC (%d), VFSOC (%d), data(0x%04x)\n",
				__func__, soc/10, vf_soc/10, (data[1]<<8) | data[0]);
	}
#endif

	return min(soc, 1000);
}

/* soc should be 0.01% unit */
static int max77854_fg_read_rawsoc(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	int soc;

	if (max77854_bulk_read(fuelgauge->i2c, SOCREP_REG,
			       2, data) < 0) {
		pr_err("%s: Failed to read SOCREP\n", __func__);
		return -1;
	}

	soc = (data[1] * 100) + (data[0] * 100 / 256);

	pr_debug("%s: raw capacity (0.01%%) (%d)\n",
		 __func__, soc);

	if (!(fuelgauge->info.pr_cnt % PRINT_COUNT))
		pr_debug("%s: raw capacity (%d), data(0x%04x)\n",
			 __func__, soc, (data[1]<<8) | data[0]);

	return min(soc, 10000);
}

static int max77854_fg_read_fullcap(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	int ret;

	if (max77854_bulk_read(fuelgauge->i2c, FULLCAP_REG,
			       2, data) < 0) {
		pr_err("%s: Failed to read FULLCAP\n", __func__);
		return -1;
	}

	ret = (data[1] << 8) + data[0];

	return ret;
}

static int max77854_fg_read_fullcaprep(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	int ret;

	if (max77854_bulk_read(fuelgauge->i2c, FG_FULLCAPREP,
			       2, data) < 0) {
		pr_err("%s: Failed to read FULLCAP\n", __func__);
		return -1;
	}

	ret = (data[1] << 8) + data[0];

	return ret;
}


static int max77854_fg_read_fullcapnom(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	int ret;

	if (max77854_bulk_read(fuelgauge->i2c, FULLCAP_NOM_REG,
			       2, data) < 0) {
		pr_err("%s: Failed to read FULLCAP\n", __func__);
		return -1;
	}

	ret = (data[1] << 8) + data[0];

	return ret;
}

static int max77854_fg_read_mixcap(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	int ret;

	if (max77854_bulk_read(fuelgauge->i2c, REMCAP_MIX_REG,
			       2, data) < 0) {
		pr_err("%s: Failed to read REMCAP_MIX_REG\n",
		       __func__);
		return -1;
	}

	ret = (data[1] << 8) + data[0];

	return ret;
}

static int max77854_fg_read_avcap(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	int ret;

	if (max77854_bulk_read(fuelgauge->i2c, REMCAP_AV_REG,
			       2, data) < 0) {
		pr_err("%s: Failed to read REMCAP_AV_REG\n",
		       __func__);
		return -1;
	}

	ret = (data[1] << 8) + data[0];

	return ret;
}

static int max77854_fg_read_repcap(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	int ret;

	if (max77854_bulk_read(fuelgauge->i2c, REMCAP_REP_REG,
			       2, data) < 0) {
		pr_err("%s: Failed to read REMCAP_REP_REG\n",
		       __func__);
		return -1;
	}

	ret = (data[1] << 8) + data[0];

	return ret;
}

static int max77854_fg_read_current(struct max77854_fuelgauge_data *fuelgauge, int unit)
{
	u8 data1[2];
	u32 temp, sign;
	s32 i_current;

	if (max77854_bulk_read(fuelgauge->i2c, CURRENT_REG,
			      2, data1) < 0) {
		pr_err("%s: Failed to read CURRENT\n", __func__);
		return -1;
	}

	temp = ((data1[1]<<8) | data1[0]) & 0xFFFF;
	/* Debug log for abnormal current case */
	if (temp & (0x1 << 15)) {
		sign = NEGATIVE;
		temp = (~temp & 0xFFFF) + 1;
	} else
		sign = POSITIVE;

	/* 1.5625uV/0.01Ohm(Rsense) = 156.25uA */
	/* 1.5625uV/0.005Ohm(Rsense) = 312.5uA */
	switch (unit) {
	case SEC_BATTERY_CURRENT_UA:
		i_current = temp * 15625 * fuelgauge->fg_resistor / 100;
		break;
	case SEC_BATTERY_CURRENT_MA:
	default:
		i_current = temp * 15625 * fuelgauge->fg_resistor / 100000;
	}

	if (sign)
		i_current *= -1;

	pr_debug("%s: current=%d\n", __func__, i_current);

	return i_current;
}

static int max77854_fg_read_avg_current(struct max77854_fuelgauge_data *fuelgauge, int unit)
{
	u8  data2[2];
	u32 temp, sign;
	s32 avg_current;
	int vcell;
	static int cnt;

	if (max77854_bulk_read(fuelgauge->i2c, AVG_CURRENT_REG,
			       2, data2) < 0) {
		pr_err("%s: Failed to read AVERAGE CURRENT\n",
		       __func__);
		return -1;
	}

	temp = ((data2[1]<<8) | data2[0]) & 0xFFFF;
	if (temp & (0x1 << 15)) {
		sign = NEGATIVE;
		temp = (~temp & 0xFFFF) + 1;
	} else
		sign = POSITIVE;

	/* 1.5625uV/0.01Ohm(Rsense) = 156.25uA */
	/* 1.5625uV/0.005Ohm(Rsense) = 312.5uA */
	switch (unit) {
	case SEC_BATTERY_CURRENT_UA:
		avg_current = temp * 15625 * fuelgauge->fg_resistor / 100;
		break;
	case SEC_BATTERY_CURRENT_MA:
	default:
		avg_current = temp * 15625 * fuelgauge->fg_resistor / 100000;
	}

	if (sign)
		avg_current *= -1;

	vcell = max77854_fg_read_vcell(fuelgauge);
	if ((vcell < 3500) && (cnt < 10) && (avg_current < 0) &&
	    fuelgauge->is_charging) {
		avg_current = 1;
		cnt++;
	}

	pr_debug("%s: avg_current=%d\n", __func__, avg_current);

	return avg_current;
}

static int max77854_fg_read_cycle(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	int ret;

	if (max77854_bulk_read(fuelgauge->i2c, CYCLES_REG,
			       2, data) < 0) {
		pr_err("%s: Failed to read FULLCAPCYCLE\n", __func__);
		return -1;
	}

	ret = (data[1] << 8) + data[0];

	return ret;
}

static bool max77854_check_jig_status(struct max77854_fuelgauge_data *fuelgauge)
{
	bool ret = false;
	if(fuelgauge->pdata->jig_gpio)
		ret = gpio_get_value(fuelgauge->pdata->jig_gpio);

	return ret;
}

int max77854_fg_reset_soc(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	int vfocv, fullcap;

	/* delay for current stablization */
	msleep(500);

	pr_info("%s: Before quick-start - VCELL(%d), VFOCV(%d), VfSOC(%d), RepSOC(%d)\n",
		__func__, max77854_fg_read_vcell(fuelgauge), max77854_fg_read_vfocv(fuelgauge),
		max77854_fg_read_vfsoc(fuelgauge), max77854_fg_read_soc(fuelgauge));
	pr_info("%s: Before quick-start - current(%d), avg current(%d)\n",
		__func__, max77854_fg_read_current(fuelgauge, SEC_BATTERY_CURRENT_MA),
		max77854_fg_read_avg_current(fuelgauge, SEC_BATTERY_CURRENT_MA));

	if (!max77854_check_jig_status(fuelgauge)) {
		pr_info("%s : Return by No JIG_ON signal\n", __func__);
		return 0;
	}

	max77854_write_word(fuelgauge->i2c, CYCLES_REG, 0);

	if (max77854_bulk_read(fuelgauge->i2c, MISCCFG_REG,
			       2, data) < 0) {
		pr_err("%s: Failed to read MiscCFG\n", __func__);
		return -1;
	}

	data[1] |= (0x1 << 2);
	if (max77854_bulk_write(fuelgauge->i2c, MISCCFG_REG,
				2, data) < 0) {
		pr_err("%s: Failed to write MiscCFG\n", __func__);
		return -1;
	}

	msleep(250);
	max77854_write_word(fuelgauge->i2c, FULLCAP_REG,
			    fuelgauge->battery_data->Capacity);
	msleep(500);

	pr_info("%s: After quick-start - VCELL(%d), VFOCV(%d), VfSOC(%d), RepSOC(%d)\n",
		__func__, max77854_fg_read_vcell(fuelgauge), max77854_fg_read_vfocv(fuelgauge),
		max77854_fg_read_vfsoc(fuelgauge), max77854_fg_read_soc(fuelgauge));
	pr_info("%s: After quick-start - current(%d), avg current(%d)\n",
		__func__, max77854_fg_read_current(fuelgauge, SEC_BATTERY_CURRENT_MA),
		max77854_fg_read_avg_current(fuelgauge, SEC_BATTERY_CURRENT_MA));

	max77854_write_word(fuelgauge->i2c, CYCLES_REG, 0x00a0);

/* P8 is not turned off by Quickstart @3.4V
 * (It's not a problem, depend on mode data)
 * Power off for factory test(File system, etc..) */
	vfocv = max77854_fg_read_vfocv(fuelgauge);
	if (vfocv < POWER_OFF_VOLTAGE_LOW_MARGIN) {
		pr_info("%s: Power off condition(%d)\n", __func__, vfocv);

		fullcap = max77854_read_word(fuelgauge->i2c, FULLCAP_REG);

		/* FullCAP * 0.009 */
		max77854_write_word(fuelgauge->i2c, REMCAP_REP_REG,
				    (u16)(fullcap * 9 / 1000));
		msleep(200);
		pr_info("%s: new soc=%d, vfocv=%d\n", __func__,
			max77854_fg_read_soc(fuelgauge), vfocv);
	}

	pr_info("%s: Additional step - VfOCV(%d), VfSOC(%d), RepSOC(%d)\n",
		__func__, max77854_fg_read_vfocv(fuelgauge),
		max77854_fg_read_vfsoc(fuelgauge), max77854_fg_read_soc(fuelgauge));

	return 0;
}

int max77854_fg_reset_capacity_by_jig_connection(struct max77854_fuelgauge_data *fuelgauge)
{
	union power_supply_propval val;
	val.intval = 1;
	psy_do_property("max77854-charger", set,
			POWER_SUPPLY_PROP_ENERGY_NOW, val);
	pr_info("%s: DesignCap = Capacity - 1 (Jig Connection)\n", __func__);

	return max77854_write_word(fuelgauge->i2c, DESIGNCAP_REG,
				   fuelgauge->battery_data->Capacity-1);
}

static int max77854_fg_check_status_reg(struct max77854_fuelgauge_data *fuelgauge)
{
	u8 status_data[2];
	int ret = 0;

	/* 1. Check Smn was generatedread */
	if (max77854_bulk_read(fuelgauge->i2c, STATUS_REG,
			       2, status_data) < 0) {
		pr_err("%s: Failed to read STATUS_REG\n", __func__);
		return -1;
	}

#ifdef BATTERY_LOG_MESSAGE
	pr_info("%s: addr(0x00), data(0x%04x)\n", __func__,
		(status_data[1]<<8) | status_data[0]);
#endif

	if (status_data[1] & (0x1 << 2))
		ret = 1;

	/* 2. clear Status reg */
	status_data[1] = 0;
	if (max77854_bulk_write(fuelgauge->i2c, STATUS_REG,
				2, status_data) < 0) {
		pr_info("%s: Failed to write STATUS_REG\n", __func__);
		return -1;
	}

	return ret;
}

int max77854_get_fuelgauge_value(struct max77854_fuelgauge_data *fuelgauge, int data)
{
	int ret;

	switch (data) {
	case FG_LEVEL:
		ret = max77854_fg_read_soc(fuelgauge);
		break;

	case FG_TEMPERATURE:
		ret = max77854_fg_read_temp(fuelgauge);
		break;

	case FG_VOLTAGE:
		ret = max77854_fg_read_vcell(fuelgauge);
		break;

	case FG_CURRENT:
		ret = max77854_fg_read_current(fuelgauge, SEC_BATTERY_CURRENT_MA);
		break;

	case FG_CURRENT_AVG:
		ret = max77854_fg_read_avg_current(fuelgauge, SEC_BATTERY_CURRENT_MA);
		break;

	case FG_CHECK_STATUS:
		ret = max77854_fg_check_status_reg(fuelgauge);
		break;

	case FG_RAW_SOC:
		ret = max77854_fg_read_rawsoc(fuelgauge);
		break;

	case FG_VF_SOC:
		ret = max77854_fg_read_vfsoc(fuelgauge);
		break;

	case FG_AV_SOC:
		ret = max77854_fg_read_avsoc(fuelgauge);
		break;

	case FG_FULLCAP:
		ret = max77854_fg_read_fullcap(fuelgauge);
		break;

	case FG_FULLCAPNOM:
		ret = max77854_fg_read_fullcapnom(fuelgauge);
		break;

	case FG_FULLCAPREP:
		ret = max77854_fg_read_fullcaprep(fuelgauge);
		break;

	case FG_MIXCAP:
		ret = max77854_fg_read_mixcap(fuelgauge);
		break;

	case FG_AVCAP:
		ret = max77854_fg_read_avcap(fuelgauge);
		break;

	case FG_REPCAP:
		ret = max77854_fg_read_repcap(fuelgauge);
		break;

	case FG_CYCLE:
		ret = max77854_fg_read_cycle(fuelgauge);
		break;

	default:
		ret = -1;
		break;
	}

	return ret;
}

int max77854_fg_alert_init(struct max77854_fuelgauge_data *fuelgauge, int soc)
{
	u8 misccgf_data[2];
	u8 salrt_data[2];
	u8 config_data[2];
	u8 talrt_data[2];
	u16 read_data = 0;

	fuelgauge->is_fuel_alerted = false;

	/* Using RepSOC */
	if (max77854_bulk_read(fuelgauge->i2c, MISCCFG_REG, 2,
			       misccgf_data) < 0) {
		pr_err("%s: Failed to read MISCCFG_REG\n", __func__);
		return -1;
	}
	misccgf_data[0] = misccgf_data[0] & ~(0x03);

	if (max77854_bulk_write(fuelgauge->i2c, MISCCFG_REG,
				2, misccgf_data) < 0) {
		pr_info("%s: Failed to write MISCCFG_REG\n", __func__);
		return -1;
	}

	/* SALRT Threshold setting */
	salrt_data[1] = 0xff;
	salrt_data[0] = soc;
	if (max77854_bulk_write(fuelgauge->i2c, SALRT_THRESHOLD_REG,
				2, salrt_data) < 0) {
		pr_info("%s: Failed to write SALRT_THRESHOLD_REG\n", __func__);
		return -1;
	}

	/* Reset TALRT Threshold setting (disable) */
	talrt_data[1] = 0x7F;
	talrt_data[0] = 0x80;
	if (max77854_bulk_write(fuelgauge->i2c, TALRT_THRESHOLD_REG,
				2, talrt_data) < 0) {
		pr_info("%s: Failed to write TALRT_THRESHOLD_REG\n", __func__);
		return -1;
	}

	read_data = max77854_read_word(fuelgauge->i2c, (u8)TALRT_THRESHOLD_REG);
	if (read_data != 0x7f80)
		pr_err("%s: TALRT_THRESHOLD_REG is not valid (0x%x)\n",
			__func__, read_data);

	/* Enable SOC alerts */
	if (max77854_bulk_read(fuelgauge->i2c, CONFIG_REG,
			       2, config_data) < 0) {
		pr_err("%s: Failed to read CONFIG_REG\n", __func__);
		return -1;
	}
	config_data[0] = config_data[0] | (0x1 << 2);

	if (max77854_bulk_write(fuelgauge->i2c, CONFIG_REG,
				2, config_data) < 0) {
		pr_info("%s: Failed to write CONFIG_REG\n", __func__);
		return -1;
	}

	max77854_update_reg(fuelgauge->pmic,
			    MAX77854_PMIC_REG_INTSRC_MASK,
			    ~MAX77854_IRQSRC_FG,
			    MAX77854_IRQSRC_FG);

	pr_info("[%s] SALRT(0x%02x%02x), CONFIG(0x%02x%02x)\n",
		__func__,
		salrt_data[1], salrt_data[0],
		config_data[1], config_data[0]);

	return 1;
}

static int max77854_get_fuelgauge_soc(struct max77854_fuelgauge_data *fuelgauge)
{
	int fg_soc = 0;
	int fg_vfsoc;
	int fg_vcell;
	int fg_current;
	int avg_current;

	fg_soc = max77854_get_fuelgauge_value(fuelgauge, FG_LEVEL);
	if (fg_soc < 0) {
		pr_info("Can't read soc!!!");
		fg_soc = fuelgauge->info.soc;
	}

	fg_vcell = max77854_get_fuelgauge_value(fuelgauge, FG_VOLTAGE);
	fg_current = max77854_get_fuelgauge_value(fuelgauge, FG_CURRENT);
	avg_current = max77854_get_fuelgauge_value(fuelgauge, FG_CURRENT_AVG);
	fg_vfsoc = max77854_get_fuelgauge_value(fuelgauge, FG_VF_SOC);

	if (fuelgauge->info.is_first_check)
		fuelgauge->info.is_first_check = false;

	if ((fg_vcell < 3400) && (avg_current < 0) && (fg_soc <= 10))
		fg_soc = 0;

	fuelgauge->info.soc = fg_soc;

	pr_debug("%s: soc(%d)\n",
		__func__, fuelgauge->info.soc);

	return fg_soc;
}

static irqreturn_t max77854_jig_irq_thread(int irq, void *irq_data)
{
	struct max77854_fuelgauge_data *fuelgauge = irq_data;

	if (max77854_check_jig_status(fuelgauge))
		max77854_fg_reset_capacity_by_jig_connection(fuelgauge);
	else
		pr_info("%s: jig removed\n", __func__);
	return IRQ_HANDLED;
}

bool max77854_fg_init(struct max77854_fuelgauge_data *fuelgauge)
{
	ktime_t	current_time;
	struct timespec ts;
	u8 data[2] = {0, 0};
	u32 volt_threshold = 0;
	u32 temp_threshold = 0;

#if defined(ANDROID_ALARM_ACTIVATED)
	current_time = alarm_get_elapsed_realtime();
	ts = ktime_to_timespec(current_time);
#else
	current_time = ktime_get_boottime();
	ts = ktime_to_timespec(current_time);
#endif

	fuelgauge->info.fullcap_check_interval = ts.tv_sec;

	fuelgauge->info.is_first_check = true;

	/* Init parameters to prevent wrong compensation. */
	fuelgauge->info.previous_fullcap =
		max77854_read_word(fuelgauge->i2c, FULLCAP_REG);
	fuelgauge->info.previous_vffullcap =
		max77854_read_word(fuelgauge->i2c, FULLCAP_NOM_REG);

	if (fuelgauge->pdata->jig_gpio) {
		int ret;
		ret = request_threaded_irq(fuelgauge->pdata->jig_irq,
				NULL, max77854_jig_irq_thread,
				IRQF_TRIGGER_RISING | IRQF_ONESHOT,
				"jig-irq", fuelgauge);
		if (ret) {
			pr_info("%s: Failed to Request IRQ\n",
					__func__);
		}
		pr_info("%s: jig_result : %d\n", __func__, max77854_check_jig_status(fuelgauge));

		/* initial check for the JIG */
		if (max77854_check_jig_status(fuelgauge))
			max77854_fg_reset_capacity_by_jig_connection(fuelgauge);
	}

	/* NOT using FG for temperature */
	if (fuelgauge->pdata->thermal_source != SEC_BATTERY_THERMAL_SOURCE_FG) {
		if (max77854_bulk_read(fuelgauge->i2c, CONFIG_REG,
				       2, data) < 0) {
			pr_err ("%s : Failed to read CONFIG_REG\n", __func__);
			return false;
		}
		data[1] |= 0x1;

		if (max77854_bulk_write(fuelgauge->i2c, CONFIG_REG,
					2, data) < 0) {
			pr_info("%s : Failed to write CONFIG_REG\n", __func__);
			return false;
		}
	} else {
		if (max77854_bulk_read(fuelgauge->i2c, CONFIG_REG,
				       2, data) < 0) {
			pr_err ("%s : Failed to read CONFIG_REG\n", __func__);
			return false;
		}
		data[1] &= 0xFE;
		data[0] |= 0x10;

		if (max77854_bulk_write(fuelgauge->i2c, CONFIG_REG,
					2, data) < 0) {
			pr_info("%s : Failed to write CONFIG_REG\n", __func__);
			return false;
		}
	}

	if (fuelgauge->auto_discharge_en) {
		/* Auto discharge EN & Alert Enable */
		max77854_bulk_read(fuelgauge->i2c, CONFIG2_REG, 2, data);
		data[1] |= 0x2;
		max77854_bulk_write(fuelgauge->i2c, CONFIG2_REG, 2, data);

		/* Set Auto Discharge temperature & Voltage threshold */
		volt_threshold =
			fuelgauge->discharge_volt_threshold < 3900 ? 0x0 :
			fuelgauge->discharge_volt_threshold > 4540 ? 0x20 :
			(fuelgauge->discharge_volt_threshold - 3900) / 20;

		temp_threshold =
			fuelgauge->discharge_temp_threshold < 470 ? 0x0 :
			fuelgauge->discharge_temp_threshold > 630 ? 0x20 :
			(fuelgauge->discharge_temp_threshold - 470) / 5;

		max77854_bulk_read(fuelgauge->i2c, DISCHARGE_THRESHOLD_REG, 2, data);
		data[1] &= ~0xF8;
		data[1] |= volt_threshold << 3;

		data[0] &= ~0xF8;
		data[0] |= temp_threshold << 3;

		max77854_bulk_write(fuelgauge->i2c, DISCHARGE_THRESHOLD_REG, 2, data);

		pr_info("%s: DISCHARGE_THRESHOLD Value : 0x%x\n",
			__func__, (data[1]<<8) | data[0]);
	}

	return true;
}

bool max77854_fg_fuelalert_init(struct max77854_fuelgauge_data *fuelgauge,
				int soc)
{
	/* 1. Set max77854 alert configuration. */
	if (max77854_fg_alert_init(fuelgauge, soc) > 0)
		return true;
	else
		return false;
}

void max77854_fg_fuelalert_set(struct max77854_fuelgauge_data *fuelgauge,
			       int enable)
{
	u8 config_data[2];
	u8 status_data[2];

	if (max77854_bulk_read(fuelgauge->i2c, CONFIG_REG,
			       2, config_data) < 0)
		pr_err("%s: Failed to read CONFIG_REG\n", __func__);

	if (enable)
		config_data[0] |= ALERT_EN;
	else
		config_data[0] &= ~ALERT_EN;

	pr_info("%s : CONFIG(0x%02x%02x)\n", __func__, config_data[1], config_data[0]);

	if (max77854_bulk_write(fuelgauge->i2c, CONFIG_REG,
				2, config_data) < 0)
		pr_info("%s: Failed to write CONFIG_REG\n", __func__);

	if (max77854_bulk_read(fuelgauge->i2c, STATUS_REG,
			       2, status_data) < 0)
		pr_err("%s : Failed to read STATUS_REG\n", __func__);

	if ((status_data[1] & 0x01) && !lpcharge && !fuelgauge->is_charging) {
		pr_info("%s : Battery Voltage is Very Low!! SW V EMPTY ENABLE\n", __func__);
		fuelgauge->vempty_mode = VEMPTY_MODE_SW_VALERT;
	}
}


bool max77854_fg_fuelalert_process(void *irq_data)
{
	struct max77854_fuelgauge_data *fuelgauge =
		(struct max77854_fuelgauge_data *)irq_data;

	max77854_fg_fuelalert_set(fuelgauge, 0);

	return true;
}

bool max77854_fg_reset(struct max77854_fuelgauge_data *fuelgauge)
{
	if (!max77854_fg_reset_soc(fuelgauge))
		return true;
	else
		return false;
}

static int max77854_fg_check_capacity_max(
	struct max77854_fuelgauge_data *fuelgauge, int capacity_max)
{
	int cap_max, cap_min;

	cap_max = (fuelgauge->pdata->capacity_max +
			fuelgauge->pdata->capacity_max_margin) * 100 / 101;
	cap_min = (fuelgauge->pdata->capacity_max -
		fuelgauge->pdata->capacity_max_margin) * 100 / 101;

	return (capacity_max < cap_min) ? cap_min :
		((capacity_max > cap_max) ? cap_max : capacity_max);
}

#define CAPACITY_MAX_CONTROL_THRESHOLD 300
static void max77854_fg_get_scaled_capacity(
	struct max77854_fuelgauge_data *fuelgauge,
	union power_supply_propval *val)
{
#if defined(CONFIG_DISABLE_SAVE_CAPACITY_MAX)
	u16 reg_data;
#endif
	union power_supply_propval cable_val, chg_val, chg_val2;
#if defined(CONFIG_BATTERY_SWELLING)
	union power_supply_propval swelling_val;
#endif
	int current_standard, raw_capacity = val->intval;
	struct power_supply *psy = get_power_supply_by_name("battery");

	if(!psy) {
		pr_info("%s : battery is not initailized\n", __func__);
		return;
	}

	psy_do_property("battery", get, POWER_SUPPLY_PROP_ONLINE, cable_val);
#if defined(CONFIG_BATTERY_SWELLING)
	/* Check whether DUT is in the swelling mode or not */
	psy_do_property("battery", get, POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT, swelling_val);
#endif

	chg_val.intval = cable_val.intval;
	psy_do_property("max77854-charger", get, POWER_SUPPLY_PROP_CURRENT_AVG,
			chg_val);
	psy_do_property("max77854-charger", get, POWER_SUPPLY_PROP_CHARGE_NOW,
			chg_val2);

	if (is_hv_wireless_type(fuelgauge->cable_type) || is_hv_wire_type(fuelgauge->cable_type))
		current_standard = CAPACITY_SCALE_HV_CURRENT;
	else
		current_standard = CAPACITY_SCALE_DEFAULT_CURRENT;

	pr_info("%s : current_standard(%d)\n", __func__, current_standard);

	if ((cable_val.intval != SEC_BATTERY_CABLE_NONE) &&
#if defined(CONFIG_BATTERY_SWELLING)
		(!swelling_val.intval) &&
#endif
		(!strcmp(chg_val2.strval, "CV Mode")) &&
		(chg_val.intval >= current_standard)) {
		int max_temp;
		int temp, sample;
		int curr;
		int topoff;
		int capacity_threshold;
		static int cnt;

		max_temp = fuelgauge->capacity_max;
		curr = max77854_get_fuelgauge_value(fuelgauge, FG_CURRENT_AVG);
		topoff = fuelgauge->pdata->full_check_current_1st;
		capacity_threshold = topoff + CAPACITY_MAX_CONTROL_THRESHOLD;

		pr_info("%s : curr(%d) topoff(%d) capacity_max(%d)\n", __func__, curr, topoff, max_temp);

		if ((curr < capacity_threshold) && (curr > topoff)) {
			if (!cnt) {
				cnt = 1;
				fuelgauge->standard_capacity = (val->intval < fuelgauge->pdata->capacity_min) ?
					0 : ((val->intval - fuelgauge->pdata->capacity_min) * 999 /
					(fuelgauge->capacity_max - fuelgauge->pdata->capacity_min));
			} else if (fuelgauge->standard_capacity < 999) {
				temp = (val->intval < fuelgauge->pdata->capacity_min) ?
					0 : ((val->intval - fuelgauge->pdata->capacity_min) * 999 /
					(fuelgauge->capacity_max - fuelgauge->pdata->capacity_min));

				sample = ((capacity_threshold - curr) * (999 - fuelgauge->standard_capacity)) /
					(capacity_threshold - topoff);

				pr_info("%s : %d = ((%d - %d) * (999 - %d)) / (%d - %d)\n",
					__func__,
					sample, capacity_threshold, curr, fuelgauge->standard_capacity,
					capacity_threshold, topoff);

				if ((temp - fuelgauge->standard_capacity) >= sample) {
					pr_info("%s : TEMP > SAMPLE\n", __func__);
				} else if ((sample - (temp - fuelgauge->standard_capacity)) < 5) {
					pr_info("%s : TEMP < SAMPLE && GAP UNDER 5\n", __func__);
					max_temp -= (sample - (temp - fuelgauge->standard_capacity));
				} else {
					pr_info("%s : TEMP > SAMPLE && GAP OVER 5\n", __func__);
					max_temp -= 5;
				}
				pr_info("%s : TEMP(%d) SAMPLE(%d) CAPACITY_MAX(%d)\n",
					__func__, temp, sample, fuelgauge->capacity_max);

				fuelgauge->capacity_max =
					max77854_fg_check_capacity_max(fuelgauge, max_temp);
			}
		} else {
			cnt = 0;
		}
	}

	val->intval = (val->intval < fuelgauge->pdata->capacity_min) ?
		0 : ((val->intval - fuelgauge->pdata->capacity_min) * 1000 /
		     (fuelgauge->capacity_max - fuelgauge->pdata->capacity_min));

#if defined(CONFIG_DISABLE_SAVE_CAPACITY_MAX)
	reg_data = max77854_read_word(fuelgauge->i2c, 0xD0);
	if (reg_data != fuelgauge->capacity_max) {
		pr_info("%s : 0xD0 Register Update (%d) -> (%d)\n",
			__func__, reg_data, fuelgauge->capacity_max);
		reg_data = fuelgauge->capacity_max;
		max77854_write_word(fuelgauge->i2c, 0xD0, reg_data);
	}
#endif

	pr_info("%s : CABLE TYPE(%d) INPUT CURRENT(%d) CHARGING MODE(%s)" \
		"capacity_max (%d) scaled capacity(%d.%d), raw_soc(%d.%d)\n",
		__func__, cable_val.intval, chg_val.intval, chg_val2.strval,
		fuelgauge->capacity_max, val->intval/10, val->intval%10, raw_capacity/10, raw_capacity%10);
}

/* capacity is integer */
static void max77854_fg_get_atomic_capacity(
	struct max77854_fuelgauge_data *fuelgauge,
	union power_supply_propval *val)
{

	pr_debug("%s : NOW(%d), OLD(%d)\n",
		__func__, val->intval, fuelgauge->capacity_old);

	if (fuelgauge->pdata->capacity_calculation_type &
		SEC_FUELGAUGE_CAPACITY_TYPE_ATOMIC) {
	if (fuelgauge->capacity_old < val->intval)
		val->intval = fuelgauge->capacity_old + 1;
	else if (fuelgauge->capacity_old > val->intval)
		val->intval = fuelgauge->capacity_old - 1;
	}

	/* keep SOC stable in abnormal status */
	if (fuelgauge->pdata->capacity_calculation_type &
		SEC_FUELGAUGE_CAPACITY_TYPE_SKIP_ABNORMAL) {
		if (!fuelgauge->is_charging &&
			fuelgauge->capacity_old < val->intval) {
			pr_err("%s: capacity (old %d : new %d)\n",
				__func__, fuelgauge->capacity_old, val->intval);
			val->intval = fuelgauge->capacity_old;
		}
	}

	/* updated old capacity */
	fuelgauge->capacity_old = val->intval;
}

static int max77854_fg_calculate_dynamic_scale(
	struct max77854_fuelgauge_data *fuelgauge, int capacity)
{
	union power_supply_propval raw_soc_val;
	raw_soc_val.intval = max77854_get_fuelgauge_value(fuelgauge,
						 FG_RAW_SOC) / 10;

	if (raw_soc_val.intval <
		fuelgauge->pdata->capacity_max -
		fuelgauge->pdata->capacity_max_margin) {
		pr_info("%s: raw soc(%d) is very low, skip routine\n",
			__func__, raw_soc_val.intval);
		return fuelgauge->capacity_max;
	} else {
		fuelgauge->capacity_max =
			(raw_soc_val.intval >
			fuelgauge->pdata->capacity_max +
			fuelgauge->pdata->capacity_max_margin) ?
			(fuelgauge->pdata->capacity_max +
			fuelgauge->pdata->capacity_max_margin) :
			raw_soc_val.intval;
		pr_debug("%s: raw soc (%d)", __func__,
			 fuelgauge->capacity_max);
	}

	fuelgauge->capacity_max =
		(fuelgauge->capacity_max * 100 / (capacity + 1));
	fuelgauge->capacity_old = capacity;

	pr_info("%s: %d is used for capacity_max, capacity(%d)\n",
		__func__, fuelgauge->capacity_max, capacity);

	return fuelgauge->capacity_max;
}

static void max77854_fg_check_qrtable(struct max77854_fuelgauge_data *fuelgauge)
{
	u16 data;

	data = max77854_read_word(fuelgauge->i2c, QRTABLE20_REG);
	if (data != fuelgauge->battery_data->QResidual20) {
		if (max77854_write_word(fuelgauge->i2c, QRTABLE20_REG,
			    fuelgauge->battery_data->QResidual20) < 0)
			pr_err("%s: Failed to write QRTABLE20\n", __func__);
	}

	data = max77854_read_word(fuelgauge->i2c, QRTABLE30_REG);
	if (data != fuelgauge->battery_data->QResidual30) {
		if (max77854_write_word(fuelgauge->i2c, QRTABLE30_REG,
			    fuelgauge->battery_data->QResidual30) <0)
			pr_err("%s: Failed to write QRTABLE30\n", __func__);
	}

	pr_debug("%s: QRTABLE20_REG(0x%04x), QRTABLE30_REG(0x%04x)\n", __func__,
		fuelgauge->battery_data->QResidual20,
		fuelgauge->battery_data->QResidual30);
}

#if defined(CONFIG_EN_OOPS)
static void max77854_set_full_value(struct max77854_fuelgauge_data *fuelgauge,
				    int cable_type)
{
	u16 ichgterm, misccfg, fullsocthr;

	if (is_hv_wireless_type(cable_type) || is_hv_wire_type(cable_type)) {
		ichgterm = fuelgauge->battery_data->ichgterm_2nd;
		misccfg = fuelgauge->battery_data->misccfg_2nd;
		fullsocthr = fuelgauge->battery_data->fullsocthr_2nd;
	} else {
		ichgterm = fuelgauge->battery_data->ichgterm;
		misccfg = fuelgauge->battery_data->misccfg;
		fullsocthr = fuelgauge->battery_data->fullsocthr;
	}

	max77854_write_word(fuelgauge->i2c, ICHGTERM_REG, ichgterm);
	max77854_write_word(fuelgauge->i2c, MISCCFG_REG, misccfg);
	max77854_write_word(fuelgauge->i2c, FULLSOCTHR_REG, fullsocthr);

	pr_info("%s : ICHGTERM(0x%04x) FULLSOCTHR(0x%04x), MISCCFG(0x%04x)\n",
		__func__, ichgterm, misccfg, fullsocthr);
}
#endif

static int calc_ttf(struct max77854_fuelgauge_data *fuelgauge, union power_supply_propval *val)
{
	int i;
	int cc_time = 0, cv_time = 0;

	int soc = fuelgauge->raw_capacity;
	int charge_current = val->intval;
	struct cv_slope *cv_data = fuelgauge->cv_data;
	int design_cap = fuelgauge->battery_data->Capacity * fuelgauge->fg_resistor / 2;

	if(!cv_data || (val->intval <= 0)) {
		pr_info("%s: no cv_data or val: %d\n", __func__, val->intval);
		return -1;
	}
	for (i = 0; i < fuelgauge->cv_data_lenth ;i++) {
		if (charge_current >= cv_data[i].fg_current)
			break;
	}
	i = i >= fuelgauge->cv_data_lenth ? fuelgauge->cv_data_lenth - 1 : i;
	if (cv_data[i].soc < soc) {
		for (i = 0; i < fuelgauge->cv_data_lenth; i++) {
			if (soc <= cv_data[i].soc)
				break;
		}
		cv_time = ((cv_data[i-1].time - cv_data[i].time) * (cv_data[i].soc - soc)\
				/ (cv_data[i].soc - cv_data[i-1].soc)) + cv_data[i].time;
	} else { //CC mode || NONE
		cv_time = cv_data[i].time;
		cc_time = design_cap * (cv_data[i].soc - soc)\
				/ val->intval * 3600 / 1000;
		pr_debug("%s: cc_time: %d\n", __func__, cc_time);
		if (cc_time < 0) {

			cc_time = 0;
		}
	}

        pr_debug("%s: cap: %d, soc: %4d, T: %6d, avg: %4d, cv soc: %4d, i: %4d, val: %d\n",
         __func__, design_cap, soc, cv_time + cc_time, fuelgauge->current_avg, cv_data[i].soc, i, val->intval);

        if (cv_time + cc_time >= 0)
                return cv_time + cc_time + 60;
        else
                return 60; //minimum 1minutes
}

static int max77854_fg_get_property(struct power_supply *psy,
			     enum power_supply_property psp,
			     union power_supply_propval *val)
{
	struct max77854_fuelgauge_data *fuelgauge = power_supply_get_drvdata(psy);
	static int abnormal_current_cnt = 0;
	union power_supply_propval value;
	u8 data[2] = {0, 0};

	switch (psp) {
		/* Cell voltage (VCELL, mV) */
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = max77854_get_fuelgauge_value(fuelgauge, FG_VOLTAGE);
		break;
		/* Additional Voltage Information (mV) */
	case POWER_SUPPLY_PROP_VOLTAGE_AVG:
		switch (val->intval) {
		case SEC_BATTERY_VOLTAGE_OCV:
			val->intval = max77854_fg_read_vfocv(fuelgauge);
			break;
		case SEC_BATTERY_VOLTAGE_AVERAGE:
		default:
			val->intval = max77854_fg_read_avg_vcell(fuelgauge);
			break;
		}
		break;
		/* Current */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		switch (val->intval) {
		case SEC_BATTERY_CURRENT_UA:
			val->intval =
				max77854_fg_read_current(fuelgauge,
						SEC_BATTERY_CURRENT_UA);
			break;
		case SEC_BATTERY_CURRENT_MA:
		default:
			fuelgauge->current_now = val->intval = max77854_get_fuelgauge_value(fuelgauge,
							  FG_CURRENT);
			psy_do_property("battery", get,
					POWER_SUPPLY_PROP_STATUS, value);
			/* To save log for abnormal case */
			if (value.intval == POWER_SUPPLY_STATUS_DISCHARGING && val->intval > 0) {
				abnormal_current_cnt++;
				if (abnormal_current_cnt >= 5) {
					pr_info("%s : Inow is increasing in not charging status\n",
						__func__);
					value.intval = fuelgauge->capacity_old + 15;
					psy_do_property("battery", set,
							POWER_SUPPLY_PROP_CAPACITY, value);
					abnormal_current_cnt = 0;
					value.intval = fuelgauge->capacity_old;
					psy_do_property("battery", set,
							POWER_SUPPLY_PROP_CAPACITY, value);
				}
			} else {
				abnormal_current_cnt = 0;
			}
			break;
		}
		break;
		/* Average Current */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		switch (val->intval) {
		case SEC_BATTERY_CURRENT_UA:
			val->intval =
				max77854_fg_read_avg_current(fuelgauge,
						    SEC_BATTERY_CURRENT_UA);
			break;
		case SEC_BATTERY_CURRENT_MA:
		default:
			fuelgauge->current_avg = val->intval =
				max77854_get_fuelgauge_value(fuelgauge,
						    FG_CURRENT_AVG);
			break;
		}
		break;
		/* Full Capacity */
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		switch (val->intval) {
		case SEC_BATTERY_CAPACITY_DESIGNED:
			val->intval = max77854_get_fuelgauge_value(fuelgauge,
							  FG_FULLCAP);
			break;
		case SEC_BATTERY_CAPACITY_ABSOLUTE:
			val->intval = max77854_get_fuelgauge_value(fuelgauge,
							  FG_MIXCAP);
			break;
		case SEC_BATTERY_CAPACITY_TEMPERARY:
			val->intval = max77854_get_fuelgauge_value(fuelgauge,
							  FG_AVCAP);
			break;
		case SEC_BATTERY_CAPACITY_CURRENT:
			val->intval = max77854_get_fuelgauge_value(fuelgauge,
							  FG_REPCAP);
			break;
		case SEC_BATTERY_CAPACITY_AGEDCELL:
			val->intval = max77854_get_fuelgauge_value(fuelgauge,
							  FG_FULLCAPNOM);
			break;
		case SEC_BATTERY_CAPACITY_CYCLE:
			val->intval = max77854_get_fuelgauge_value(fuelgauge,
							  FG_CYCLE);
			break;
		}
		break;
		/* SOC (%) */
	case POWER_SUPPLY_PROP_CAPACITY:
		if (val->intval == SEC_FUELGAUGE_CAPACITY_TYPE_RAW) {
			val->intval = max77854_get_fuelgauge_value(fuelgauge,
							  FG_RAW_SOC);
		} else {
			val->intval = max77854_get_fuelgauge_soc(fuelgauge);


			if (fuelgauge->pdata->capacity_calculation_type &
			    (SEC_FUELGAUGE_CAPACITY_TYPE_SCALE |
			     SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE))
				max77854_fg_get_scaled_capacity(fuelgauge, val);

			/* capacity should be between 0% and 100%
			 * (0.1% degree)
			 */
			if (val->intval > 1000)
				val->intval = 1000;
			if (val->intval < 0)
				val->intval = 0;

			fuelgauge->raw_capacity = val->intval;
			/* get only integer part */
			val->intval /= 10;

			/* SW/HW V Empty setting */
			if (fuelgauge->using_hw_vempty) {
				if (fuelgauge->temperature <= (int)fuelgauge->low_temp_limit) {
					if (fuelgauge->raw_capacity <= 50) {
						if (fuelgauge->vempty_mode != VEMPTY_MODE_HW) {
							max77854_fg_set_vempty(fuelgauge, VEMPTY_MODE_HW);
						}
					} else if (fuelgauge->vempty_mode == VEMPTY_MODE_HW) {
						max77854_fg_set_vempty(fuelgauge, VEMPTY_MODE_SW);
					}
				} else if (fuelgauge->vempty_mode != VEMPTY_MODE_HW) {
					max77854_fg_set_vempty(fuelgauge, VEMPTY_MODE_HW);
				}
			}

			if (!fuelgauge->is_charging &&
			    fuelgauge->vempty_mode == VEMPTY_MODE_SW_VALERT && !lpcharge) {
				pr_info("%s : SW V EMPTY. Decrease SOC\n", __func__);
				val->intval = 0;
			} else if ((fuelgauge->vempty_mode == VEMPTY_MODE_SW_RECOVERY) &&
				   (val->intval == fuelgauge->capacity_old)) {
				fuelgauge->vempty_mode = VEMPTY_MODE_SW;
			}

			/* check whether doing the wake_unlock */
			if ((val->intval > fuelgauge->pdata->fuel_alert_soc) &&
			     fuelgauge->is_fuel_alerted) {
				max77854_fg_fuelalert_init(fuelgauge,
					  fuelgauge->pdata->fuel_alert_soc);
			}

			/* (Only for atomic capacity)
			 * In initial time, capacity_old is 0.
			 * and in resume from sleep,
			 * capacity_old is too different from actual soc.
			 * should update capacity_old
			 * by val->intval in booting or resume.
			 */
			if ((fuelgauge->initial_update_of_soc) &&
			    (fuelgauge->vempty_mode == VEMPTY_MODE_HW)) {
				/* updated old capacity */
				fuelgauge->capacity_old = val->intval;
				fuelgauge->initial_update_of_soc = false;
				break;
			}

			if (fuelgauge->pdata->capacity_calculation_type &
			    (SEC_FUELGAUGE_CAPACITY_TYPE_ATOMIC |
			     SEC_FUELGAUGE_CAPACITY_TYPE_SKIP_ABNORMAL))
				max77854_fg_get_atomic_capacity(fuelgauge, val);
		}
		break;
		/* Battery Temperature */
	case POWER_SUPPLY_PROP_TEMP:
		/* Target Temperature */
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		val->intval = max77854_get_fuelgauge_value(fuelgauge,
							   FG_TEMPERATURE);
		break;
#if defined(CONFIG_EN_OOPS)
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		return -ENODATA;
#endif
	case POWER_SUPPLY_PROP_ENERGY_FULL:
		{
			int fullcap = max77854_get_fuelgauge_value(fuelgauge, FG_FULLCAPNOM);
			val->intval = fullcap * 100 / fuelgauge->battery_data->Capacity;
			pr_info("%s: asoc(%d), fullcap(0x%x)\n",
				__func__, val->intval, fullcap);
#if !defined(CONFIG_SEC_FACTORY)
			max77854_fg_periodic_read(fuelgauge);
#endif
		}
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		val->intval = fuelgauge->capacity_max;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		val->intval = calc_ttf(fuelgauge, val);
		break;
	case POWER_SUPPLY_PROP_CHARGE_ENABLED:
		max77854_bulk_read(fuelgauge->i2c, STATUS_REG, 2, data);
		val->intval = data[0] & 0x40;
		break;
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		return -ENODATA;
#endif
	case POWER_SUPPLY_PROP_FILTER_CFG:
		max77854_bulk_read(fuelgauge->i2c, FILTER_CFG_REG, 2, data);
		val->intval = data[1] << 8 | data[0];
		pr_debug("%s: FilterCFG=0x%04X\n", __func__, data[1] << 8 | data[0]);
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

#if defined(CONFIG_UPDATE_BATTERY_DATA)
static int max77854_fuelgauge_parse_dt(struct max77854_fuelgauge_data *fuelgauge);
#endif
static int max77854_fg_set_property(struct power_supply *psy,
			     enum power_supply_property psp,
			     const union power_supply_propval *val)
{
	struct max77854_fuelgauge_data *fuelgauge = power_supply_get_drvdata(psy);
	u8 data[2] = {0, 0};

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		if (fuelgauge->pdata->capacity_calculation_type &
			SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE)
			max77854_fg_calculate_dynamic_scale(fuelgauge, val->intval);
		break;
#if defined(CONFIG_EN_OOPS)
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		max77854_set_full_value(fuelgauge, val->intval);
		break;
#endif
	case POWER_SUPPLY_PROP_ONLINE:
		fuelgauge->cable_type = val->intval;
		if (val->intval == SEC_BATTERY_CABLE_NONE) {
			fuelgauge->is_charging = false;
		} else {
			fuelgauge->is_charging = true;

			/* enable alert */
			if (fuelgauge->vempty_mode >= VEMPTY_MODE_SW_VALERT) {
				max77854_fg_set_vempty(fuelgauge, VEMPTY_MODE_HW);
				fuelgauge->initial_update_of_soc = true;
				max77854_fg_fuelalert_init(fuelgauge,
							   fuelgauge->pdata->fuel_alert_soc);
			}
		}
		break;
		/* Battery Temperature */
	case POWER_SUPPLY_PROP_CAPACITY:
		if (val->intval == SEC_FUELGAUGE_CAPACITY_TYPE_RESET) {
			fuelgauge->initial_update_of_soc = true;
			if (!max77854_fg_reset(fuelgauge))
				return -EINVAL;
			else
				break;
		}
	case POWER_SUPPLY_PROP_TEMP:
		if(val->intval < 0) {
			u16 reg_data;
			reg_data = max77854_read_word(fuelgauge->i2c, DESIGNCAP_REG);
			if(reg_data == fuelgauge->battery_data->Capacity) {
				max77854_write_word(fuelgauge->i2c, DESIGNCAP_REG,
						fuelgauge->battery_data->Capacity+3);
				pr_info("%s: set the low temp reset! temp : %d, capacity : 0x%x, original capacity : 0x%x\n",
						__func__, val->intval, reg_data , fuelgauge->battery_data->Capacity);
			}
		}

		max77854_fg_write_temp(fuelgauge, val->intval);
		max77854_fg_check_qrtable(fuelgauge);
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		max77854_fg_reset_capacity_by_jig_connection(fuelgauge);
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		pr_info("%s: capacity_max changed, %d -> %d\n",
			__func__, fuelgauge->capacity_max, val->intval);
		fuelgauge->capacity_max = max77854_fg_check_capacity_max(fuelgauge, val->intval);
		fuelgauge->initial_update_of_soc = true;
		break;
	case POWER_SUPPLY_PROP_CHARGE_ENABLED:
		/* Forcedly discharge EN */
		max77854_bulk_read(fuelgauge->i2c, CONFIG2_REG, 2, data);
		data[1] &= ~0x6;
		data[1] |= val->intval ? 0x1 << 2 :
			fuelgauge->auto_discharge_en ? 0x1 << 1 : 0x0 << 1;
		max77854_bulk_write(fuelgauge->i2c, CONFIG2_REG, 2, data);
		break;
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		{
			u16 reg_fullsocthr;
			int val_soc = val->intval;
			if (val->intval > fuelgauge->pdata->full_condition_soc ||
				val->intval <= (fuelgauge->pdata->full_condition_soc - 10)) {
				pr_info("%s: abnormal value(%d). so thr is changed to default(%d)\n",
					__func__, val->intval, fuelgauge->pdata->full_condition_soc);
				val_soc = fuelgauge->pdata->full_condition_soc;
			}

			reg_fullsocthr = val_soc << 8;
			if (max77854_write_word(fuelgauge->i2c, FULLSOCTHR_REG, reg_fullsocthr) < 0) {
				pr_err("%s: Failed to write FULLSOCTHR_REG\n", __func__);
			} else {
				reg_fullsocthr = max77854_read_word(fuelgauge->i2c, FULLSOCTHR_REG);
				pr_info("%s: FullSOCThr %d%%(0x%04X)\n", __func__, val_soc, reg_fullsocthr);
			}
		}
		break;
#endif
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	case POWER_SUPPLY_PROP_POWER_DESIGN:
		max77854_fuelgauge_parse_dt(fuelgauge);
		break;
#endif
	case POWER_SUPPLY_PROP_FILTER_CFG:
		/* Set FilterCFG */
		max77854_bulk_read(fuelgauge->i2c, FILTER_CFG_REG, 2, data);
		pr_debug("%s: FilterCFG=0x%04X\n", __func__, data[1] << 8 | data[0]);
		data[0] &= ~0xF;
		data[0] |= (val->intval & 0xF);
		max77854_bulk_write(fuelgauge->i2c, FILTER_CFG_REG, 2, data);

		max77854_bulk_read(fuelgauge->i2c, FILTER_CFG_REG, 2, data);
		pr_debug("%s: FilterCFG=0x%04X\n", __func__, data[1] << 8 | data[0]);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void max77854_fg_isr_work(struct work_struct *work)
{
	struct max77854_fuelgauge_data *fuelgauge =
		container_of(work, struct max77854_fuelgauge_data, isr_work.work);

	/* process for fuel gauge chip */
	max77854_fg_fuelalert_process(fuelgauge);

	wake_unlock(&fuelgauge->fuel_alert_wake_lock);
}

static irqreturn_t max77854_fg_irq_thread(int irq, void *irq_data)
{
	struct max77854_fuelgauge_data *fuelgauge = irq_data;

	max77854_update_reg(fuelgauge->pmic,
			    MAX77854_PMIC_REG_INTSRC_MASK,
			    MAX77854_IRQSRC_FG,
			    MAX77854_IRQSRC_FG);

	pr_info("%s\n", __func__);

	if (fuelgauge->is_fuel_alerted) {
		return IRQ_HANDLED;
	} else {
		wake_lock(&fuelgauge->fuel_alert_wake_lock);
		fuelgauge->is_fuel_alerted = true;
		schedule_delayed_work(&fuelgauge->isr_work, 0);
	}

	return IRQ_HANDLED;
}

static int max77854_fuelgauge_debugfs_show(struct seq_file *s, void *data)
{
	struct max77854_fuelgauge_data *fuelgauge = s->private;
	int i;
	u8 reg;
	u16 reg_data;

	seq_printf(s, "MAX77854 FUELGAUGE IC :\n");
	seq_printf(s, "===================\n");
	for (i = 0; i < 16; i++) {
		if (i == 12)
			continue;
		for (reg = 0; reg < 0x10; reg++) {
			reg_data = max77854_read_word(fuelgauge->i2c, reg + i * 0x10);
			seq_printf(s, "0x%02x:\t0x%04x\n", reg + i * 0x10, reg_data);
		}
		if (i == 4)
			i = 10;
	}
	seq_printf(s, "\n");
	return 0;
}

static int max77854_fuelgauge_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, max77854_fuelgauge_debugfs_show, inode->i_private);
}

static const struct file_operations max77854_fuelgauge_debugfs_fops = {
	.open           = max77854_fuelgauge_debugfs_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
};

#ifdef CONFIG_OF
static int max77854_fuelgauge_parse_dt(struct max77854_fuelgauge_data *fuelgauge)
{
	struct device_node *np = of_find_node_by_name(NULL, "max77854-fuelgauge");
	sec_fuelgauge_platform_data_t *pdata = fuelgauge->pdata;
	int ret;
	int len;
	const u32 *p;

	/* reset, irq gpio info */
	if (np == NULL) {
		pr_err("%s np NULL\n", __func__);
	} else {
		ret = of_property_read_u32(np, "fuelgauge,capacity_max",
				&pdata->capacity_max);
		if (ret < 0)
			pr_err("%s error reading capacity_max %d\n", __func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,capacity_max_hv",
				&pdata->capacity_max_hv);
		if (ret < 0) {
			pr_err("%s error reading capacity_max_hv %d\n", __func__, ret);
			fuelgauge->pdata->capacity_max_hv = fuelgauge->pdata->capacity_max;
		}

		ret = of_property_read_u32(np, "fuelgauge,capacity_max_margin",
				&pdata->capacity_max_margin);
		if (ret < 0)
			pr_err("%s error reading capacity_max_margin %d\n", __func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,capacity_min",
				&pdata->capacity_min);
		if (ret < 0)
			pr_err("%s error reading capacity_min %d\n", __func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,capacity_calculation_type",
				&pdata->capacity_calculation_type);
		if (ret < 0)
			pr_err("%s error reading capacity_calculation_type %d\n",
					__func__, ret);
		ret = of_property_read_u32(np, "fuelgauge,fuel_alert_soc",
				&pdata->fuel_alert_soc);
		if (ret < 0)
			pr_err("%s error reading pdata->fuel_alert_soc %d\n",
					__func__, ret);

		pdata->repeated_fuelalert = of_property_read_bool(np,
				"fuelgauge,repeated_fuelalert");

		fuelgauge->using_temp_compensation = of_property_read_bool(np,
						   "fuelgauge,using_temp_compensation");
		if (fuelgauge->using_temp_compensation) {
			ret = of_property_read_u32(np, "fuelgauge,low_temp_limit",
						   &fuelgauge->low_temp_limit);
			if (ret < 0)
				pr_err("%s error reading low temp limit %d\n", __func__, ret);

			pr_info("%s : LOW TEMP LIMIT(%d)\n",
				__func__, fuelgauge->low_temp_limit);
		}

		fuelgauge->using_hw_vempty = of_property_read_bool(np,
								   "fuelgauge,using_hw_vempty");
		if (fuelgauge->using_hw_vempty) {
			ret = of_property_read_u32(np, "fuelgauge,v_empty",
						   &fuelgauge->battery_data->V_empty);
			if (ret < 0)
				pr_err("%s error reading v_empty %d\n",
				       __func__, ret);

			ret = of_property_read_u32(np, "fuelgauge,v_empty_origin",
						   &fuelgauge->battery_data->V_empty_origin);
			if(ret < 0)
				pr_err("%s error reading v_empty_origin %d\n",
				       __func__, ret);

			ret = of_property_read_u32(np, "fuelgauge,sw_v_empty_voltage",
						   &fuelgauge->battery_data->sw_v_empty_vol);
			if(ret < 0)
				pr_err("%s error reading sw_v_empty_default_vol %d\n",
					   __func__, ret);
			
			ret = of_property_read_u32(np, "fuelgauge,sw_v_empty_recover_voltage",
						   &fuelgauge->battery_data->sw_v_empty_recover_vol);
			if(ret < 0)
				pr_err("%s error reading sw_v_empty_recover_vol %d\n",
					   __func__, ret);
			
			pr_info("%s : SW V Empty (%d)mV,  SW V Empty recover (%d)mV\n",
				__func__, fuelgauge->battery_data->sw_v_empty_vol, fuelgauge->battery_data->sw_v_empty_recover_vol);
		}

		pdata->jig_gpio = of_get_named_gpio(np, "fuelgauge,jig_gpio", 0);
		if (pdata->jig_gpio < 0) {
			pr_err("%s error reading jig_gpio = %d\n",
					__func__,pdata->jig_gpio);
			pdata->jig_gpio = 0;
		} else {
			pdata->jig_irq = gpio_to_irq(pdata->jig_gpio);
		}

		ret = of_property_read_u32(np, "fuelgauge,qrtable20",
					   &fuelgauge->battery_data->QResidual20);
		if (ret < 0)
			pr_err("%s error reading qrtable20 %d\n",
			       __func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,qrtable30",
					   &fuelgauge->battery_data->QResidual30);
		if (ret < 0)
			pr_err("%s error reading qrtabel30 %d\n",
			       __func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,fg_resistor",
				&fuelgauge->fg_resistor);
		if (ret < 0) {
			pr_err("%s error reading fg_resistor %d\n",
					__func__, ret);
			fuelgauge->fg_resistor = 1;
		}
#if defined(CONFIG_EN_OOPS)
		ret = of_property_read_u32(np, "fuelgauge,ichgterm",
					   &fuelgauge->battery_data->ichgterm);
		if (ret < 0)
			pr_err("%s error reading ichgterm %d\n",
					__func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,ichgterm_2nd",
					   &fuelgauge->battery_data->ichgterm_2nd);
		if (ret < 0)
			pr_err("%s error reading ichgterm_2nd %d\n",
					__func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,misccfg",
					   &fuelgauge->battery_data->misccfg);
		if (ret < 0)
			pr_err("%s error reading misccfg %d\n",
					__func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,misccfg_2nd",
					   &fuelgauge->battery_data->misccfg_2nd);
		if (ret < 0)
			pr_err("%s error reading misccfg_2nd %d\n",
					__func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,fullsocthr",
					   &fuelgauge->battery_data->fullsocthr);
		if (ret < 0)
			pr_err("%s error reading fullsocthr %d\n",
					__func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,fullsocthr_2nd",
					   &fuelgauge->battery_data->fullsocthr_2nd);
		if (ret < 0)
			pr_err("%s error reading fullsocthr_2nd %d\n",
					__func__, ret);
#endif

		ret = of_property_read_u32(np, "fuelgauge,capacity",
					   &fuelgauge->battery_data->Capacity);
		if (ret < 0)
			pr_err("%s error reading capacity_calculation_type %d\n",
					__func__, ret);

		fuelgauge->auto_discharge_en = of_property_read_bool(np,
				"fuelgauge,auto_discharge_en");
		if (fuelgauge->auto_discharge_en) {
	                ret = of_property_read_u32(np, "fuelgauge,discharge_temp_threshold",
				&fuelgauge->discharge_temp_threshold);
			if (ret < 0)
				fuelgauge->discharge_temp_threshold = 600;

	                ret = of_property_read_u32(np, "fuelgauge,discharge_volt_threshold",
				&fuelgauge->discharge_volt_threshold);
			if (ret < 0)
				fuelgauge->discharge_volt_threshold = 4200;
		}

		p = of_get_property(np, "fuelgauge,cv_data", &len);
		if (p) {
			fuelgauge->cv_data = kzalloc(len,
						  GFP_KERNEL);
			fuelgauge->cv_data_lenth = len / sizeof(struct cv_slope);
			pr_err("%s len: %ld, lenth: %d, %d\n",
					__func__, sizeof(int) * len, len, fuelgauge->cv_data_lenth);
			ret = of_property_read_u32_array(np, "fuelgauge,cv_data",
					 (u32 *)fuelgauge->cv_data, len/sizeof(u32));
#if 0
			for(i = 0; i < fuelgauge->cv_data_lenth; i++) {
				pr_err("%s  %5d, %5d, %5d\n",
						__func__, fuelgauge->cv_data[i].fg_current,
						fuelgauge->cv_data[i].soc, fuelgauge->cv_data[i].time);
			}
#endif

			if (ret) {
				pr_err("%s failed to read fuelgauge->cv_data: %d\n",
						__func__, ret);
				kfree(fuelgauge->cv_data);
				fuelgauge->cv_data = NULL;
			}
		} else {
			pr_err("%s there is not cv_data\n", __func__);
		}

		np = of_find_node_by_name(NULL, "battery");
		ret = of_property_read_u32(np, "battery,thermal_source",
					   &pdata->thermal_source);
		if (ret < 0) {
			pr_err("%s error reading pdata->thermal_source %d\n",
			       __func__, ret);
		}

		np = of_find_node_by_name(NULL, "cable-info");
		ret = of_property_read_u32(np, "full_check_current_1st", &pdata->full_check_current_1st);
		ret = of_property_read_u32(np, "full_check_current_2nd", &pdata->full_check_current_2nd);

#if defined(CONFIG_BATTERY_AGE_FORECAST)
		ret = of_property_read_u32(np, "battery,full_condition_soc",
			&pdata->full_condition_soc);
		if (ret) {
			pdata->full_condition_soc = 93;
			pr_info("%s : Full condition soc is Empty\n", __func__);
		}
#endif

		pr_info("%s thermal: %d, fg_irq: %d, capacity_max: %d\n"
			"qrtable20: 0x%x, qrtable30 : 0x%x\n"
			"capacity_max_margin: %d, capacity_min: %d\n"
			"calculation_type: 0x%x, fuel_alert_soc: %d,\n"
			"repeated_fuelalert: %d\n",
			__func__, pdata->thermal_source, pdata->fg_irq, pdata->capacity_max,
			fuelgauge->battery_data->QResidual20,
			fuelgauge->battery_data->QResidual30,
			pdata->capacity_max_margin, pdata->capacity_min,
			pdata->capacity_calculation_type, pdata->fuel_alert_soc,
			pdata->repeated_fuelalert);
	}

	pr_info("[%s][%d]\n",
		__func__, fuelgauge->battery_data->Capacity);

	return 0;
}
#endif

static const struct power_supply_desc max77854_fuelgauge_power_supply_desc = {
	.name = "max77854-fuelgauge",
	.type = POWER_SUPPLY_TYPE_UNKNOWN,
	.properties = max77854_fuelgauge_props,
	.num_properties = ARRAY_SIZE(max77854_fuelgauge_props),
	.get_property = max77854_fg_get_property,
	.set_property = max77854_fg_set_property,
};

static int max77854_fuelgauge_probe(struct platform_device *pdev)
{
	struct max77854_dev *max77854 = dev_get_drvdata(pdev->dev.parent);
	struct max77854_platform_data *pdata = dev_get_platdata(max77854->dev);
	sec_fuelgauge_platform_data_t *fuelgauge_data;
	struct max77854_fuelgauge_data *fuelgauge;
	struct power_supply_config fuelgauge_cfg = {};
	int ret = 0;
	union power_supply_propval raw_soc_val;
#if defined(CONFIG_DISABLE_SAVE_CAPACITY_MAX)	
	u16 reg_data;
#endif

	pr_info("%s: MAX77854 Fuelgauge Driver Loading\n", __func__);

	fuelgauge = kzalloc(sizeof(*fuelgauge), GFP_KERNEL);
	if (!fuelgauge)
		return -ENOMEM;

	fuelgauge_data = kzalloc(sizeof(sec_fuelgauge_platform_data_t), GFP_KERNEL);
	if (!fuelgauge_data) {
		ret = -ENOMEM;
		goto err_free;
	}

	mutex_init(&fuelgauge->fg_lock);

	fuelgauge->dev = &pdev->dev;
	fuelgauge->pdata = fuelgauge_data;
	fuelgauge->i2c = max77854->fuelgauge;
	fuelgauge->pmic = max77854->i2c;
	fuelgauge->max77854_pdata = pdata;

#if defined(CONFIG_OF)
	fuelgauge->battery_data = kzalloc(sizeof(struct battery_data_t),
					  GFP_KERNEL);
	if(!fuelgauge->battery_data) {
		pr_err("Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_pdata_free;
	}
	ret = max77854_fuelgauge_parse_dt(fuelgauge);
	if (ret < 0) {
		pr_err("%s not found charger dt! ret[%d]\n",
		       __func__, ret);
	}
#endif

	platform_set_drvdata(pdev, fuelgauge);

	fuelgauge->capacity_max = fuelgauge->pdata->capacity_max;
	raw_soc_val.intval = max77854_get_fuelgauge_value(fuelgauge, FG_RAW_SOC) / 10;

#if defined(CONFIG_DISABLE_SAVE_CAPACITY_MAX)
	reg_data = max77854_read_word(fuelgauge->i2c, 0xD0);

	if (reg_data >= 900 && reg_data <= 1000 && reg_data != fuelgauge->capacity_max) {
		pr_info("%s : Capacity Max Update (%d) -> (%d)\n",
			__func__, fuelgauge->capacity_max, reg_data);
		fuelgauge->capacity_max = reg_data;
	} else {
		pr_info("%s : 0xD0 Register Update (%d) -> (%d)\n",
			__func__, reg_data, fuelgauge->capacity_max);
		reg_data = fuelgauge->capacity_max;
		max77854_write_word(fuelgauge->i2c, 0xD0, reg_data);
	}
#endif

	if (raw_soc_val.intval > fuelgauge->capacity_max)
		max77854_fg_calculate_dynamic_scale(fuelgauge, 100);

	(void) debugfs_create_file("max77854-fuelgauge-regs",
		S_IRUGO, NULL, (void *)fuelgauge, &max77854_fuelgauge_debugfs_fops);

	if (!max77854_fg_init(fuelgauge)) {
		pr_err("%s: Failed to Initialize Fuelgauge\n", __func__);
		goto err_data_free;
	}

	/* SW/HW init code. SW/HW V Empty mode must be opposite ! */
	fuelgauge->temperature = 300; /* default value */
	pr_info("%s: SW/HW V empty init \n", __func__);
	max77854_fg_set_vempty(fuelgauge, VEMPTY_MODE_HW);

	fuelgauge_cfg.drv_data = fuelgauge;

	fuelgauge->psy_fg = power_supply_register(&pdev->dev, &max77854_fuelgauge_power_supply_desc, &fuelgauge_cfg);
	if (!fuelgauge->psy_fg) {
		pr_err("%s: Failed to Register psy_fg\n", __func__);
		goto err_data_free;
	}

	fuelgauge->fg_irq = pdata->irq_base + MAX77854_FG_IRQ_ALERT;
	pr_info("[%s]IRQ_BASE(%d) FG_IRQ(%d)\n",
		__func__, pdata->irq_base, fuelgauge->fg_irq);

	fuelgauge->is_fuel_alerted = false;
	if (fuelgauge->pdata->fuel_alert_soc >= 0) {
		if (max77854_fg_fuelalert_init(fuelgauge,
				       fuelgauge->pdata->fuel_alert_soc)) {
			wake_lock_init(&fuelgauge->fuel_alert_wake_lock,
				       WAKE_LOCK_SUSPEND, "fuel_alerted");
			if (fuelgauge->fg_irq) {
				INIT_DELAYED_WORK(&fuelgauge->isr_work, max77854_fg_isr_work);

				ret = request_threaded_irq(fuelgauge->fg_irq,
					   NULL, max77854_fg_irq_thread,
					   IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					   "fuelgauge-irq", fuelgauge);
				if (ret) {
					pr_err("%s: Failed to Request IRQ\n", __func__);
					goto err_supply_unreg;
				}
			}
		} else {
			pr_err("%s: Failed to Initialize Fuel-alert\n",
			       __func__);
			goto err_supply_unreg;
		}
	}

	fuelgauge->initial_update_of_soc = true;
	pr_info("%s: MAX77854 Fuelgauge Driver Loaded\n", __func__);
	return 0;

err_supply_unreg:
	power_supply_unregister(fuelgauge->psy_fg);
err_data_free:
#if defined(CONFIG_OF)
	kfree(fuelgauge->battery_data);
#endif
err_pdata_free:
	kfree(fuelgauge_data);
	mutex_destroy(&fuelgauge->fg_lock);
err_free:
	kfree(fuelgauge);

	return ret;
}

static int max77854_fuelgauge_remove(struct platform_device *pdev)
{
	struct max77854_fuelgauge_data *fuelgauge =
		platform_get_drvdata(pdev);

	if (fuelgauge->pdata->fuel_alert_soc >= 0)
		wake_lock_destroy(&fuelgauge->fuel_alert_wake_lock);

	return 0;
}

static int max77854_fuelgauge_suspend(struct device *dev)
{
	return 0;
}

static int max77854_fuelgauge_resume(struct device *dev)
{
	struct max77854_fuelgauge_data *fuelgauge = dev_get_drvdata(dev);

	fuelgauge->initial_update_of_soc = true;

	return 0;
}

static void max77854_fuelgauge_shutdown(struct device *dev)
{
}

static SIMPLE_DEV_PM_OPS(max77854_fuelgauge_pm_ops, max77854_fuelgauge_suspend,
			 max77854_fuelgauge_resume);

static struct platform_driver max77854_fuelgauge_driver = {
	.driver = {
		   .name = "max77854-fuelgauge",
		   .owner = THIS_MODULE,
#ifdef CONFIG_PM
		   .pm = &max77854_fuelgauge_pm_ops,
#endif
		.shutdown = max77854_fuelgauge_shutdown,
	},
	.probe	= max77854_fuelgauge_probe,
	.remove	= max77854_fuelgauge_remove,
};

static int __init max77854_fuelgauge_init(void)
{
	pr_info("%s: \n", __func__);
	return platform_driver_register(&max77854_fuelgauge_driver);
}

static void __exit max77854_fuelgauge_exit(void)
{
	platform_driver_unregister(&max77854_fuelgauge_driver);
}
module_init(max77854_fuelgauge_init);
module_exit(max77854_fuelgauge_exit);

MODULE_DESCRIPTION("Samsung MAX77854 Fuel Gauge Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
