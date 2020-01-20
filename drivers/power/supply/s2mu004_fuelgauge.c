/*
 *  s2mu004_fuelgauge.c
 *  Samsung S2MU004 Fuel Gauge Driver
 *
 *  Copyright (C) 2015 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG

#define SINGLE_BYTE	1
#define TABLE_SIZE	22
#define FAKE_TEMP 1

#include <linux/power/s2mu004_fuelgauge.h>
#include <linux/of_gpio.h>

static enum power_supply_property s2mu004_fuelgauge_props[] = {
};

static int s2mu004_get_vbat(struct s2mu004_fuelgauge_data *fuelgauge);
static int s2mu004_get_ocv(struct s2mu004_fuelgauge_data *fuelgauge);
static int s2mu004_get_current(struct s2mu004_fuelgauge_data *fuelgauge);
static int s2mu004_get_avgcurrent(struct s2mu004_fuelgauge_data *fuelgauge);
static int s2mu004_get_avgvbat(struct s2mu004_fuelgauge_data *fuelgauge);
static int s2mu004_get_monout_avgvbat(struct s2mu004_fuelgauge_data *fuelgauge);

static int s2mu004_write_reg_byte(struct i2c_client *client, int reg, u8 data)
{
	int ret, i = 0;

	ret = i2c_smbus_write_byte_data(client, reg, data);
	if (ret < 0) {
		for (i = 0; i < 3; i++) {
			ret = i2c_smbus_write_byte_data(client, reg,  data);
			if (ret >= 0)
				break;
		}

		if (i >= 3)
			dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
	}

	return ret;
}

static int s2mu004_write_reg(struct i2c_client *client, int reg, u8 *buf)
{
#if SINGLE_BYTE
	int ret = 0;

	s2mu004_write_reg_byte(client, reg, buf[0]);
	s2mu004_write_reg_byte(client, reg+1, buf[1]);
#else
	int ret, i = 0;

	ret = i2c_smbus_write_i2c_block_data(client, reg, 2, buf);
	if (ret < 0) {
		for (i = 0; i < 3; i++) {
			ret = i2c_smbus_write_i2c_block_data(client, reg, 2, buf);
			if (ret >= 0)
				break;
		}

		if (i >= 3)
			dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
	}
#endif
	return ret;
}

static int s2mu004_read_reg_byte(struct i2c_client *client, int reg, void *data)
{
	int ret = 0;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		return ret;
	*(u8 *)data = (u8)ret;

	return ret;
}

static int s2mu004_read_reg(struct i2c_client *client, int reg, u8 *buf)
{
#if SINGLE_BYTE
	int ret = 0;
	u8 data1 = 0, data2 = 0;

	s2mu004_read_reg_byte(client, reg, &data1);
	s2mu004_read_reg_byte(client, reg+1, &data2);
	buf[0] = data1;
	buf[1] = data2;
#else
	int ret = 0, i = 0;

	ret = i2c_smbus_read_i2c_block_data(client, reg, 2, buf);
	if (ret < 0) {
		for (i = 0; i < 3; i++) {
			ret = i2c_smbus_read_i2c_block_data(client, reg, 2, buf);
			if (ret >= 0)
				break;
		}

		if (i >= 3)
			dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
	}
#endif
	return ret;
}

static void s2mu004_fg_test_read(struct i2c_client *client)
{
		u8 data;
		char str[1016] = {0,};
		int i;

	/* address 0x00 ~ 0x1f */
	for (i = 0x0; i <= 0x1F; i++) {
		s2mu004_read_reg_byte(client, i, &data);
		sprintf(str+strlen(str), "0x%02x:0x%02x, ", i, data);
	}

	/* address 0x25 */
	s2mu004_read_reg_byte(client, 0x25, &data);
	sprintf(str+strlen(str), "0x25:0x%02x, ", data);

	/* address 0x27 */
	s2mu004_read_reg_byte(client, 0x27, &data);
	sprintf(str+strlen(str), "0x27:0x%02x, ", data);
	/* print buffer */
	pr_info("[FG]%s: %s\n", __func__, str);
}

static void WA_0_issue_at_init(struct s2mu004_fuelgauge_data *fuelgauge)
{
	int a = 0;
	u8 v_4e = 0, v_4f = 0, temp1, temp2;
	int FG_volt, UI_volt, offset;
	u8 v_40 = 0;
	u8 temp_REG26 = 0, temp_REG27 = 0, temp = 0;

	/* Step 1: [Surge test]  get UI voltage (0.1mV)*/
	UI_volt = s2mu004_get_ocv(fuelgauge);

	/* current fix for soc */
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x27, &temp_REG27);
	temp = temp_REG27;
	temp |= 0x0F;
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x27, temp);

	s2mu004_read_reg_byte(fuelgauge->i2c, 0x26, &temp_REG26);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x26, 0xFF);

	/* avgvbat factor value set to 0xFF  */
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x40, &v_40);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x40, 0xFF);

	s2mu004_write_reg_byte(fuelgauge->i2c, 0x1E, 0x0F);
	msleep(50);

	/* Step 2: [Surge test] get FG voltage (0.1mV) */
	FG_volt = s2mu004_get_vbat(fuelgauge) * 10;

	/* Step 3: [Surge test] get offset */
	offset = UI_volt - FG_volt;
	pr_err("%s: UI_volt(%d), FG_volt(%d), offset(%d)\n",
			__func__, UI_volt, FG_volt, offset);

	/* Step 4: [Surge test] */
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x4f, &v_4f);
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x4e, &v_4e);
	pr_err("%s: v_4f(0x%x), v_4e(0x%x)\n", __func__, v_4f, v_4e);

	a = (v_4f & 0x0F) << 8;
	a += v_4e;
	pr_err("%s: a before add offset (0x%x)\n", __func__, a);

	/* 2`s complement */
	if (a & (0x01 << 11))
		a = (-10000 * ((a^0xFFF) + 1)) >> 13;
	else
		a = (10000 * a) >> 13;

	a = a + offset;
	pr_err("%s: a after add offset (0x%x)\n", __func__, a);

	/* limit upper/lower offset */
	if (a > 2490)
		a = 2490;

	if (a < (-2490))
		a = -2490;

	a = (a << 13) / 10000;
	if (a < 0)
		a = -1*((a^0xFFF)+1);

	pr_err("%s: a after add offset (0x%x)\n", __func__, a);

	a &= 0xfff;
	pr_err("%s: (a)&0xFFF (0x%x)\n", __func__, a);

	/* modify 0x4f[3:0] */
	temp1 = v_4f & 0xF0;
	temp2 = (u8)((a&0xF00) >> 8);
	temp1 |= temp2;
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x4f, temp1);

	/* modify 0x4e[7:0] */
	temp2 = (u8)(a & 0xFF);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x4e, temp2);

	/* restart and dumpdone */
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x1E, 0x0F);
	msleep(300);

	/* restore current register */
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x27, temp_REG27);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x26, temp_REG26);

	/* recovery 0x4e and 0x4f */
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x4f, &temp1);
	temp1 &= 0xF0;
	temp1 |= (v_4f & 0x0F);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x4f, temp1);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x4e, v_4e);

	/* restore monout avgvbat factor value */
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x40, v_40);
}

static int s2mu004_get_soc_from_ocv(struct s2mu004_fuelgauge_data *fuelgauge, int target_ocv)
{
	/* 22 values of mapping table for EVT1*/

	int *soc_arr;
	int *ocv_arr;
	int soc = 0;
	int ocv = target_ocv * 10;

	int high_index = TABLE_SIZE - 1;
	int low_index = 0;
	int mid_index = 0;

	soc_arr = fuelgauge->info.soc_arr_val;
	ocv_arr = fuelgauge->info.ocv_arr_val;

	pr_err("%s: soc_arr(%d) ocv_arr(%d)\n", __func__, *soc_arr, *ocv_arr);

	if (ocv <= ocv_arr[TABLE_SIZE - 1]) {
		soc = soc_arr[TABLE_SIZE - 1];
		goto soc_ocv_mapping;
	} else if (ocv >= ocv_arr[0]) {
		soc = soc_arr[0];
		goto soc_ocv_mapping;
	}
	while (low_index <= high_index) {
		mid_index = (low_index + high_index) >> 1;
		if (ocv_arr[mid_index] > ocv)
			low_index = mid_index + 1;
		else if (ocv_arr[mid_index] < ocv)
			high_index = mid_index - 1;
		else {
			soc = soc_arr[mid_index];
			goto soc_ocv_mapping;
		}
	}
	soc = soc_arr[high_index];
	soc += ((soc_arr[low_index] - soc_arr[high_index]) *
					(ocv - ocv_arr[high_index])) /
					(ocv_arr[low_index] - ocv_arr[high_index]);

soc_ocv_mapping:
	dev_info(&fuelgauge->i2c->dev, "%s: ocv (%d), soc (%d)\n", __func__, ocv, soc);
	return soc;
}

static void WA_0_issue_at_init1(struct s2mu004_fuelgauge_data *fuelgauge, int target_ocv)
{
	int a = 0;
	u8 v_4e = 0, v_4f = 0, temp1, temp2;
	int FG_volt, UI_volt, offset;
	u8 v_40 = 0;
	u8 temp_REG26 = 0, temp_REG27 = 0, temp = 0;

	/* Step 1: [Surge test]  get UI voltage (0.1mV)*/
	UI_volt = target_ocv * 10;

	/* avgvbat factor value set to 0xFF  */
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x40, &v_40);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x40, 0xFF);

	/* current fix for soc */
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x27, &temp_REG27);
	temp = temp_REG27;
	temp |= 0x0F;
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x27, temp);

	s2mu004_read_reg_byte(fuelgauge->i2c, 0x26, &temp_REG26);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x26, 0xFF);

	s2mu004_write_reg_byte(fuelgauge->i2c, 0x1E, 0x0F);
	msleep(50);

	/* Step 2: [Surge test] get FG voltage (0.1mV) */
	FG_volt = s2mu004_get_vbat(fuelgauge) * 10;

	/* Step 3: [Surge test] get offset */
	offset = UI_volt - FG_volt;
	pr_err("%s: UI_volt(%d), FG_volt(%d), offset(%d)\n",
			__func__, UI_volt, FG_volt, offset);

	/* Step 4: [Surge test] */
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x4f, &v_4f);
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x4e, &v_4e);
	pr_err("%s: v_4f(0x%x), v_4e(0x%x)\n", __func__, v_4f, v_4e);

	a = (v_4f & 0x0F) << 8;
	a += v_4e;
	pr_err("%s: a before add offset (0x%x)\n", __func__, a);

	/* 2`s complement */
	if (a & (0x01 << 11))
		a = (-10000 * ((a^0xFFF) + 1)) >> 13;
	else
		a = (10000 * a) >> 13;

	a = a + offset;
	pr_err("%s: a after add offset (0x%x)\n", __func__, a);

	/* limit upper/lower offset */
	if (a > 2490)
		a = 2490;

	if (a < (-2490))
		a = -2490;

	a = (a << 13) / 10000;
	if (a < 0)
		a = -1*((a^0xFFF)+1);

	pr_err("%s: a after add offset (0x%x)\n", __func__, a);

	a &= 0xfff;
	pr_err("%s: (a)&0xFFF (0x%x)\n", __func__, a);

	/* modify 0x4f[3:0] */
	temp1 = v_4f & 0xF0;
	temp2 = (u8)((a&0xF00) >> 8);
	temp1 |= temp2;
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x4f, temp1);

	/* modify 0x4e[7:0] */
	temp2 = (u8)(a & 0xFF);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x4e, temp2);

	/* restart and dumpdone */
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x1E, 0x0F);
	msleep(300);

	/* restore current register */
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x27, temp_REG27);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x26, temp_REG26);

	pr_info("%s: S2MU004 VBAT : %d\n", __func__, s2mu004_get_vbat(fuelgauge) * 10);

	/* recovery 0x4e and 0x4f */
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x4f, &temp1);
	temp1 &= 0xF0;
	temp1 |= (v_4f & 0x0F);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x4f, temp1);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x4e, v_4e);

	/* restore monout avgvbat factor value */
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x40, v_40);
}

static void s2mu004_reset_fg(struct s2mu004_fuelgauge_data *fuelgauge)
{
	int i;
	u8 temp = 0;

	/* step 0: [Surge test] initialize register of FG */
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x0E, fuelgauge->info.batcap[0]);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x0F, fuelgauge->info.batcap[1]);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x10, fuelgauge->info.batcap[2]);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x11, fuelgauge->info.batcap[3]);

	for (i = 0x92; i <= 0xe9; i++)
		s2mu004_write_reg_byte(fuelgauge->i2c, i, fuelgauge->info.battery_table3[i - 0x92]);

	for (i = 0xea; i <= 0xff; i++)
		s2mu004_write_reg_byte(fuelgauge->i2c, i, fuelgauge->info.battery_table4[i - 0xea]);

	s2mu004_write_reg_byte(fuelgauge->i2c, 0x21, 0x13);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x14, 0x40);

	s2mu004_read_reg_byte(fuelgauge->i2c, 0x45, &temp);
	temp &= 0xF0;
	temp |= fuelgauge->info.accum[0];
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x45, temp);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x44, fuelgauge->info.accum[1]);

	s2mu004_read_reg_byte(fuelgauge->i2c, 0x27, &temp);
	temp |= 0x10;
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x27, temp);

	/* Interrupt source reference at mixed mode */
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x43, &temp);
	temp &= 0xF3;
	temp |= 0x08;
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x43, temp);

	/* Charger top off current sensing method change for int. 0x49[7]=0 */
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x49, &temp);
	temp &= 0x7F;
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x49, temp);

	s2mu004_write_reg_byte(fuelgauge->i2c, 0x4B, 0x0B);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x4A, 0x10);

	s2mu004_read_reg_byte(fuelgauge->i2c, 0x03, &temp);
	temp |= 0x10;
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x03, temp);

	s2mu004_write_reg_byte(fuelgauge->i2c, 0x40, 0x08);

	WA_0_issue_at_init(fuelgauge);
	pr_err("%s: Reset FG completed\n", __func__);
}

static void s2mu004_restart_gauging(struct s2mu004_fuelgauge_data *fuelgauge)
{
	u8 temp = 0, temp_REG26 = 0, temp_REG27 = 0;
	u8 data[2], r_data[2];
	u8 v_40;

	pr_err("%s: Re-calculate SOC and voltage\n", __func__);

	s2mu004_read_reg(fuelgauge->i2c, S2MU004_REG_IRQ, data);
	pr_info("%s: irq_reg data (%02x%02x)\n", __func__, data[1], data[0]);

	/* store data for interrupt mask */
	r_data[0] = data[0];
	r_data[1] = data[1];

	/* disable irq for unwanted interrupt */
	data[1] |= 0x0f;
	s2mu004_write_reg(fuelgauge->i2c, S2MU004_REG_IRQ, data);

	s2mu004_read_reg_byte(fuelgauge->i2c, 0x27, &temp_REG27);
	temp = temp_REG27;
	temp |= 0x0F;
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x27, temp);

	s2mu004_read_reg_byte(fuelgauge->i2c, 0x26, &temp_REG26);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x26, 0xFF);

	/* avgvbat factor value set to 0xFF  */
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x40, &v_40);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x40, 0xFF);

	/* restart gauge */
	/* s2mu004_write_reg_byte(fuelgauge->i2c, 0x1f, 0x01); */
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x21, 0x13);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x1E, 0x0F);

	msleep(300);

	s2mu004_write_reg_byte(fuelgauge->i2c, 0x27, temp_REG27);
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x26, temp_REG26);

	s2mu004_read_reg_byte(fuelgauge->i2c, 0x27, &temp);
	pr_info("%s: 0x27 : %02x\n", __func__, temp);
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x26, &temp);
	pr_info("%s: 0x26 : %02x\n", __func__, temp);

	/* restore monout avgvbat factor value */
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x40, v_40);

	/* enable irq after reset */
	s2mu004_write_reg(fuelgauge->i2c, S2MU004_REG_IRQ, r_data);
	pr_info("%s: re-store irq_reg data (%02x%02x)\n", __func__, r_data[1], r_data[0]);
}

static void s2mu004_init_regs(struct s2mu004_fuelgauge_data *fuelgauge)
{
	u8 temp = 0;

	pr_err("%s: s2mu004 fuelgauge initialize\n", __func__);

	/* Reduce top-off current difference between
	 * Power on charging and Power off charging
	 */
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x27, &temp);
	temp |= 0x10;
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x27, temp);

	/* Interrupt source reference at mixed mode */
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x43, &temp);
	temp &= 0xF3;
	temp |= 0x08;
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x43, temp);

	/* Charger top off current sensing method change for int. 0x49[7]=0 */
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x49, &temp);
	temp &= 0x7F;
	s2mu004_write_reg_byte(fuelgauge->i2c, 0x49, temp);

	s2mu004_read_reg_byte(fuelgauge->i2c, 0x4F, &temp);
	fuelgauge->reg_OTP_4F = temp;
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x4E, &temp);
	fuelgauge->reg_OTP_4E = temp;
}

static void s2mu004_alert_init(struct s2mu004_fuelgauge_data *fuelgauge)
{
	u8 data[2];

	/* VBAT Threshold setting */
	data[0] = ((fuelgauge->pdata->fuel_alert_vol - 2800) / 50) & 0x0f;

	/* SOC Threshold setting */
	data[0] = data[0] | (fuelgauge->pdata->fuel_alert_soc << 4);

	data[1] = 0x00;
	s2mu004_write_reg(fuelgauge->i2c, S2MU004_REG_IRQ_LVL, data);

	pr_info("%s: irq_lvl(vbat:0x%x, soc:0x%x)\n", __func__, data[0] & 0x0F, data[0] & 0xF0);
}

static int s2mu004_set_temperature(struct s2mu004_fuelgauge_data *fuelgauge,
		int temperature)
{
	/*
	 * s5mu004 include temperature sensor so,
	 * do not need to set temperature value.
	 */
	return temperature;
}

static int s2mu004_get_temperature(struct s2mu004_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	u16 compliment;
	s32 temperature = 0;
	/*
	 *  use monitor regiser.
	 *  monitor register default setting is temperature
	 */
	mutex_lock(&fuelgauge->fg_lock);

	s2mu004_write_reg_byte(fuelgauge->i2c, S2MU004_REG_MONOUT_SEL, 0x10);
	if (s2mu004_read_reg(fuelgauge->i2c, S2MU004_REG_MONOUT, data) < 0)
		goto err;

	mutex_unlock(&fuelgauge->fg_lock);
	compliment = (data[1] << 8) | (data[0]);

	/* data[] store 2's compliment format number */
	if (compliment & (0x1 << 15)) {
		/* Negative */
		temperature = -1 * ((~compliment & 0xFFFF) + 1);
	} else {
		temperature = compliment & 0x7FFF;
	}
	temperature = ((temperature * 100) >> 8)/10;

	dev_dbg(&fuelgauge->i2c->dev, "%s: temperature (%d)\n",
		__func__, temperature);

	/* For test, return room temperature */
	/* To use IC's value, check there is NTC & register setting is right*/
#if FAKE_TEMP
	temperature = 250;
#endif

	return temperature;

err:
	mutex_unlock(&fuelgauge->fg_lock);
	return -ERANGE;
}

static int s2mu004_get_rawsoc(struct s2mu004_fuelgauge_data *fuelgauge)
{
	u8 data[2], check_data[2];
	u16 compliment;
	int rsoc, i;
	u8 por_state = 0;
	u8 reg = S2MU004_REG_RSOC;
	u8 reg_OTP_4E = 0, reg_OTP_4F = 0;
	int fg_reset = 0;
	bool charging_enabled = false;
	union power_supply_propval value;
	int force_power_off_voltage = 0;
	int rbat = 0;

	int avg_current = 0, avg_vbat = 0, vbat = 0, curr = 0, avg_monout_vbat = 0;
	int ocv_pwroff = 0, ocv_pwr_voltagemode = 0;
	int target_soc = 0;

	/* SOC VM Monitoring For debugging SOC error */
	u8 r_monoutsel;
	u8 mount_data[2];
	u32 mount_compliment;
	int rvmsoc;

	struct power_supply *psy;
	int ret;

	s2mu004_read_reg_byte(fuelgauge->i2c, 0x1F, &por_state);
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x4F, &reg_OTP_4F);
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x4E, &reg_OTP_4E);

	dev_err(&fuelgauge->i2c->dev, "%s: OTP 4E(%02x) 4F(%02x) current 4E(%02x) 4F(%02x)\n",
			__func__, fuelgauge->reg_OTP_4E, fuelgauge->reg_OTP_4F, reg_OTP_4E, reg_OTP_4F);

	if ((por_state & 0x10) ||
			(fuelgauge->probe_done == true &&
			 (fuelgauge->reg_OTP_4E != reg_OTP_4E || fuelgauge->reg_OTP_4F != reg_OTP_4F))) {

		/* check charging enable */

		psy = power_supply_get_by_name("s2mu004-charger");
		if (!psy)
			return -EINVAL;
		ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_CHARGING_ENABLED, &value);
		if (ret < 0)
			pr_err("%s: Fail to execute property\n", __func__);

		charging_enabled = value.intval;

		value.intval = S2MU00X_BAT_CHG_MODE_CHARGING_OFF;

		psy = power_supply_get_by_name("s2mu004-charger");
		if (!psy)
			return -EINVAL;
		ret = power_supply_set_property(psy, POWER_SUPPLY_PROP_CHARGING_ENABLED, &value);
		if (ret < 0)
			pr_err("%s: Fail to execute property\n", __func__);

		if (fuelgauge->reg_OTP_4E != reg_OTP_4E || fuelgauge->reg_OTP_4F != reg_OTP_4F) {

			psy = power_supply_get_by_name("s2mu004-charger");
			if (!psy)
				return -EINVAL;
			ret = power_supply_set_property(psy, POWER_SUPPLY_PROP_FUELGAUGE_RESET, &value);
			if (ret < 0)
				pr_err("%s: Fail to execute property\n", __func__);

			s2mu004_write_reg_byte(fuelgauge->i2c, 0x1F, 0x40);
			msleep(50);
			s2mu004_write_reg_byte(fuelgauge->i2c, 0x1F, 0x01);

			s2mu004_read_reg_byte(fuelgauge->i2c, 0x4F, &reg_OTP_4F);
			s2mu004_read_reg_byte(fuelgauge->i2c, 0x4E, &reg_OTP_4E);

			dev_err(&fuelgauge->i2c->dev, "1st reset after %s: OTP 4E(%02x) 4F(%02x) current 4E(%02x) 4F(%02x)\n",
					__func__, fuelgauge->reg_OTP_4E, fuelgauge->reg_OTP_4F, reg_OTP_4E, reg_OTP_4F);

			if (fuelgauge->reg_OTP_4E != reg_OTP_4E || fuelgauge->reg_OTP_4F != reg_OTP_4F) {

				psy = power_supply_get_by_name("s2mu004-charger");
				if (!psy)
					return -EINVAL;
				ret = power_supply_set_property(psy, POWER_SUPPLY_PROP_FUELGAUGE_RESET, &value);
				if (ret < 0)
					pr_err("%s: Fail to execute property\n", __func__);

				s2mu004_write_reg_byte(fuelgauge->i2c, 0x1F, 0x40);
				msleep(50);
				s2mu004_write_reg_byte(fuelgauge->i2c, 0x1F, 0x01);
				dev_err(&fuelgauge->i2c->dev, "%s : 2nd reset\n", __func__);
			}
		}

		dev_info(&fuelgauge->i2c->dev, "%s: FG reset\n", __func__);
		s2mu004_reset_fg(fuelgauge);
		por_state &= ~0x10;
		s2mu004_write_reg_byte(fuelgauge->i2c, 0x1F, por_state);

		fg_reset = 1;
	}

	mutex_lock(&fuelgauge->fg_lock);

	reg = S2MU004_REG_RSOC;

	for (i = 0; i < 50; i++) {
		if (s2mu004_read_reg(fuelgauge->i2c, reg, data) < 0)
			goto err;
		if (s2mu004_read_reg(fuelgauge->i2c, reg, check_data) < 0)
			goto err;

		dev_dbg(&fuelgauge->i2c->dev, "[DEBUG]%s: data0 (%d) data1 (%d)\n", __func__, data[0], data[1]);
		if ((data[0] == check_data[0]) && (data[1] == check_data[1]))
			break;
	}

	/* SOC VM Monitoring For debugging SOC error */
	s2mu004_read_reg_byte(fuelgauge->i2c, S2MU004_REG_MONOUT_SEL, &r_monoutsel);
	s2mu004_write_reg_byte(fuelgauge->i2c, S2MU004_REG_MONOUT_SEL, 0x02);
	mdelay(10);
	s2mu004_read_reg(fuelgauge->i2c, S2MU004_REG_MONOUT, mount_data);

	s2mu004_write_reg_byte(fuelgauge->i2c, S2MU004_REG_MONOUT_SEL, r_monoutsel);

	mutex_unlock(&fuelgauge->fg_lock);

	/* SOC VM Monitoring For debugging SOC error */
	mount_compliment  = ((mount_data[0] + (mount_data[1] << 8)) * 10000) >> 12;
	rvmsoc = mount_compliment;

	dev_dbg(&fuelgauge->i2c->dev, "%s: vm soc raw data0 (%d) data1 (%d)\n", __func__, mount_data[0], mount_data[1]);
	dev_info(&fuelgauge->i2c->dev, "%s: vm soc (%d)\n", __func__, rvmsoc);

	if (fg_reset && charging_enabled) {
		value.intval = S2MU00X_BAT_CHG_MODE_CHARGING;

		psy = power_supply_get_by_name("s2mu004-charger");
		if (!psy)
			return -EINVAL;
		ret = power_supply_set_property(psy, POWER_SUPPLY_PROP_CHARGING_ENABLED, &value);
		if (ret < 0)
			pr_err("%s: Fail to execute property\n", __func__);
	}

	compliment = (data[1] << 8) | (data[0]);

	/* data[] store 2's compliment format number */
	if (compliment & (0x1 << 15)) {
		/* Negative */
		rsoc = ((~compliment) & 0xFFFF) + 1;
		rsoc = (rsoc * (-10000)) / (0x1 << 14);
	} else {
		rsoc = compliment & 0x7FFF;
		rsoc = ((rsoc * 10000) / (0x1 << 14));
	}

	if (fg_reset)
		fuelgauge->diff_soc = fuelgauge->info.soc - rsoc;


	dev_info(&fuelgauge->i2c->dev, "%s: current_soc (%d), previous soc (%d), diff (%d), FG_mode(%d)\n",
			__func__, rsoc, fuelgauge->info.soc, fuelgauge->diff_soc, fuelgauge->mode);

	fuelgauge->info.soc = rsoc + fuelgauge->diff_soc;

	avg_current = s2mu004_get_avgcurrent(fuelgauge);
	avg_monout_vbat =  s2mu004_get_monout_avgvbat(fuelgauge);
	ocv_pwr_voltagemode = avg_monout_vbat - avg_current*30 / 100;

	if (avg_current < (-500))
		rbat = 10;
	else
		rbat = 30;

	ocv_pwr_voltagemode = avg_monout_vbat - avg_current*rbat / 100;

	/* switch to voltage mocd for accuracy */
	if ((fuelgauge->info.soc <= 300) || ((ocv_pwr_voltagemode <= 3600) && (avg_current < 10))) {
		if (fuelgauge->mode == CURRENT_MODE) { /* switch to VOLTAGE_MODE */
			fuelgauge->mode = LOW_SOC_VOLTAGE_MODE;

			s2mu004_write_reg_byte(fuelgauge->i2c, 0x4A, 0xFF);

			dev_info(&fuelgauge->i2c->dev, "%s: FG is in low soc voltage mode\n", __func__);
		}
	} else if ((fuelgauge->info.soc > 325) && ((ocv_pwr_voltagemode > 3650) || (avg_current >= 10))) {
		if (fuelgauge->mode == LOW_SOC_VOLTAGE_MODE) {
			fuelgauge->mode = CURRENT_MODE;

			s2mu004_write_reg_byte(fuelgauge->i2c, 0x4A, 0x10);

			dev_info(&fuelgauge->i2c->dev, "%s: FG is in current mode\n", __func__);
		}
	}

	psy = power_supply_get_by_name("battery");
	if (!psy)
		return -EINVAL;
	ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_CAPACITY, &value);
	if (ret < 0)
		pr_err("%s: Fail to execute property\n", __func__);

	if (value.intval >= 98) {
		if (fuelgauge->mode == CURRENT_MODE) { /* switch to VOLTAGE_MODE */
			fuelgauge->mode = HIGH_SOC_VOLTAGE_MODE;

			s2mu004_write_reg_byte(fuelgauge->i2c, 0x4A, 0xFF);

			dev_info(&fuelgauge->i2c->dev, "%s: FG is in high soc voltage mode\n", __func__);
		}
	} else if (value.intval < 97) {
		if (fuelgauge->mode == HIGH_SOC_VOLTAGE_MODE) {
			fuelgauge->mode = CURRENT_MODE;

			s2mu004_write_reg_byte(fuelgauge->i2c, 0x4A, 0x10);

			dev_info(&fuelgauge->i2c->dev, "%s: FG is in current mode\n", __func__);
		}
	}

	avg_vbat =  s2mu004_get_avgvbat(fuelgauge);
	vbat = s2mu004_get_vbat(fuelgauge);
	curr = s2mu004_get_current(fuelgauge);

	psy = power_supply_get_by_name("battery");
	if (!psy)
		return -EINVAL;
	ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_TEMP, &value);
	if (ret < 0)
		pr_err("%s: Fail to execute property\n", __func__);

	if (value.intval <= (-150))
		force_power_off_voltage = 3550;
	else
		force_power_off_voltage = 3300;

	dev_info(&fuelgauge->i2c->dev,
			"%s: Fuelgauge Mode: %d, Force power-off voltage: %d\n",
			__func__, fuelgauge->mode, force_power_off_voltage);

	if (((avg_current < (-17)) && (curr < (-17))) &&
			((avg_monout_vbat - avg_current*rbat / 100) <= 3500) && (fuelgauge->info.soc > 100)) {
		ocv_pwroff = 3300;
		target_soc = s2mu004_get_soc_from_ocv(fuelgauge, ocv_pwroff);
		pr_info("%s : F/G reset Start - current flunctuation\n", __func__);
		WA_0_issue_at_init1(fuelgauge, ocv_pwroff);
	} else if (avg_current < (-60) && avg_vbat <= force_power_off_voltage) {
		if (fuelgauge->mode == CURRENT_MODE) {
			if (abs(avg_vbat - vbat) <= 20 && abs(avg_current - curr) <= 30) {
				ocv_pwroff = avg_vbat - avg_current * 15 / 100;
				target_soc = s2mu004_get_soc_from_ocv(fuelgauge, ocv_pwroff);
				if (abs(target_soc - fuelgauge->info.soc) > 300) {
					pr_info("%s : F/G reset Start - current mode: %d\n", __func__, target_soc);
					WA_0_issue_at_init1(fuelgauge, ocv_pwroff);

				}
			}
		} else {
			if (abs(avg_vbat - vbat) <= 20) {
				ocv_pwroff = avg_vbat;
				target_soc = s2mu004_get_soc_from_ocv(fuelgauge, ocv_pwroff);
				if (abs(target_soc - fuelgauge->info.soc) > 300) {
					pr_info("%s : F/G reset Start\n", __func__);
					WA_0_issue_at_init1(fuelgauge, ocv_pwroff);
				}
			}
		}
	}


	s2mu004_fg_test_read(fuelgauge->i2c);
	return min(fuelgauge->info.soc, 10000);

err:
	mutex_unlock(&fuelgauge->fg_lock);
	return -EINVAL;
}

static int s2mu004_get_current(struct s2mu004_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	u16 compliment;
	int curr = 0;

	if (s2mu004_read_reg(fuelgauge->i2c, S2MU004_REG_RCUR_CC, data) < 0)
		return -EINVAL;
	compliment = (data[1] << 8) | (data[0]);
	dev_dbg(&fuelgauge->i2c->dev, "%s: rCUR_CC(0x%4x)\n", __func__, compliment);

	if (compliment & (0x1 << 15)) { /* Charging */
		curr = ((~compliment) & 0xFFFF) + 1;
		curr = (curr * 1000) >> 12;
	} else { /* dischaging */
		curr = compliment & 0x7FFF;
		curr = (curr * (-1000)) >> 12;
	}

	dev_info(&fuelgauge->i2c->dev, "%s: current (%d)mA\n", __func__, curr);

	return curr;
}

#define TABLE_SIZE  22
static int s2mu004_get_ocv(struct s2mu004_fuelgauge_data *fuelgauge)
{
	/* 22 values of mapping table for EVT1*/

	int *soc_arr;
	int *ocv_arr;

	int soc = fuelgauge->info.soc;
	int ocv = 0;

	int high_index = TABLE_SIZE - 1;
	int low_index = 0;
	int mid_index = 0;

	soc_arr = fuelgauge->info.soc_arr_val;
	ocv_arr = fuelgauge->info.ocv_arr_val;

	dev_err(&fuelgauge->i2c->dev,
			"%s: soc (%d) soc_arr[TABLE_SIZE-1] (%d) ocv_arr[TABLE_SIZE-1) (%d)\n",
			__func__, soc, soc_arr[TABLE_SIZE-1], ocv_arr[TABLE_SIZE-1]);
	if (soc <= soc_arr[TABLE_SIZE - 1]) {
		ocv = ocv_arr[TABLE_SIZE - 1];
		goto ocv_soc_mapping;
	} else if (soc >= soc_arr[0]) {
		ocv = ocv_arr[0];
		goto ocv_soc_mapping;
	}
	while (low_index <= high_index) {
		mid_index = (low_index + high_index) >> 1;
		if (soc_arr[mid_index] > soc)
			low_index = mid_index + 1;
		else if (soc_arr[mid_index] < soc)
			high_index = mid_index - 1;
		else {
			ocv = ocv_arr[mid_index];
			goto ocv_soc_mapping;
		}
	}
	ocv = ocv_arr[high_index];
	ocv += ((ocv_arr[low_index] - ocv_arr[high_index]) *
					(soc - soc_arr[high_index])) /
					(soc_arr[low_index] - soc_arr[high_index]);

ocv_soc_mapping:
	dev_info(&fuelgauge->i2c->dev, "%s: soc (%d), ocv (%d)\n", __func__, soc, ocv);
	return ocv;
}

static int s2mu004_get_avgcurrent(struct s2mu004_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	u16 compliment;
	int curr = 0;

	mutex_lock(&fuelgauge->fg_lock);

	s2mu004_write_reg_byte(fuelgauge->i2c, S2MU004_REG_MONOUT_SEL, 0x26);

	if (s2mu004_read_reg(fuelgauge->i2c, S2MU004_REG_MONOUT, data) < 0)
		goto err;
	compliment = (data[1] << 8) | (data[0]);

	if (compliment & (0x1 << 15)) { /* Charging */
		curr = ((~compliment) & 0xFFFF) + 1;
		curr = (curr * 1000) >> 12;
	} else { /* dischaging */
		curr = compliment & 0x7FFF;
		curr = (curr * (-1000)) >> 12;
	}
	s2mu004_write_reg_byte(fuelgauge->i2c, S2MU004_REG_MONOUT_SEL, 0x10);

	mutex_unlock(&fuelgauge->fg_lock);

	dev_info(&fuelgauge->i2c->dev, "%s: MONOUT(0x%4x), avg current (%d)mA\n",
			__func__, compliment, curr);

	return curr;

err:
	mutex_unlock(&fuelgauge->fg_lock);
	return -EINVAL;
}

static int s2mu004_maintain_avgcurrent(
		struct s2mu004_fuelgauge_data *fuelgauge)
{
	static int cnt;
	int vcell = 0;
	int curr = 0;

	curr = s2mu004_get_avgcurrent(fuelgauge);

	vcell = s2mu004_get_vbat(fuelgauge);
	if ((cnt < 10) && (curr < 0) && (fuelgauge->is_charging) &&
			(vcell < 3500)) {
		curr = 1;
		cnt++;
		dev_info(&fuelgauge->i2c->dev, "%s: vcell (%d)mV,  modified avg current (%d)mA\n",
				__func__, vcell, curr);
	}

	return curr;
}

static int s2mu004_get_vbat(struct s2mu004_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	u8 vbat_src;
	u32 vbat = 0;

	if (s2mu004_read_reg(fuelgauge->i2c, S2MU004_REG_RVBAT, data) < 0)
		return -EINVAL;

	dev_dbg(&fuelgauge->i2c->dev, "%s: data0 (%d) data1 (%d)\n", __func__, data[0], data[1]);
	vbat = ((data[0] + (data[1] << 8)) * 1000) >> 13;

	s2mu004_read_reg_byte(fuelgauge->i2c, S2MU004_REG_CTRL0, &vbat_src);
	dev_info(&fuelgauge->i2c->dev, "%s: vbat (%d), src (0x%02X)\n",
			__func__, vbat, (vbat_src & 0x30) >> 4);

	return vbat;
}

static int s2mu004_get_monout_avgvbat(struct s2mu004_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	u16 compliment, avg_vbat;

	mutex_lock(&fuelgauge->fg_lock);

	s2mu004_write_reg_byte(fuelgauge->i2c, S2MU004_REG_MONOUT_SEL, 0x27);

	mdelay(50);

	if (s2mu004_read_reg(fuelgauge->i2c, S2MU004_REG_MONOUT, data) < 0)
		goto err;
	compliment = (data[1] << 8) | (data[0]);

	avg_vbat = (compliment * 1000) >> 12;

	s2mu004_write_reg_byte(fuelgauge->i2c, S2MU004_REG_MONOUT_SEL, 0x10);

	mutex_unlock(&fuelgauge->fg_lock);

	dev_info(&fuelgauge->i2c->dev, "%s: avgvbat (%d)\n", __func__, avg_vbat);

	return avg_vbat;

err:
	mutex_unlock(&fuelgauge->fg_lock);
	return -EINVAL;
}

static int s2mu004_get_avgvbat(struct s2mu004_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	u32 new_vbat, old_vbat = 0;
	int cnt;

	for (cnt = 0; cnt < 5; cnt++) {
		if (s2mu004_read_reg(fuelgauge->i2c, S2MU004_REG_RVBAT, data) < 0)
			return -EINVAL;

		new_vbat = ((data[0] + (data[1] << 8)) * 1000) >> 13;

		if (cnt == 0)
			old_vbat = new_vbat;
		else
			old_vbat = new_vbat / 2 + old_vbat / 2;
	}

	dev_dbg(&fuelgauge->i2c->dev, "%s: avgvbat (%d)\n", __func__, old_vbat);

	return old_vbat;
}

bool s2mu004_fuelgauge_fuelalert_init(struct i2c_client *client, int soc)
{
	struct s2mu004_fuelgauge_data *fuelgauge = i2c_get_clientdata(client);
	u8 data[2];

	fuelgauge->is_fuel_alerted = false;

	/* 1. Set s2mu004 alert configuration. */
	s2mu004_alert_init(fuelgauge);

	if (s2mu004_read_reg(client, S2MU004_REG_IRQ, data) < 0)
		return -1;

	/*Enable VBAT, SOC */
	data[1] &= 0xfc;

	/*Disable IDLE_ST, INIT)ST */
	data[1] |= 0x0c;

	s2mu004_write_reg(client, S2MU004_REG_IRQ, data);

	dev_dbg(&client->dev, "%s: irq_reg(%02x%02x) irq(%d)\n",
			__func__, data[1], data[0], fuelgauge->pdata->fg_irq);

	return true;
}

static void s2mu004_fg_reset_capacity_by_jig_connection(struct s2mu004_fuelgauge_data *fuelgauge)
{
	u8 data = 0;

	s2mu004_read_reg_byte(fuelgauge->i2c, S2MU004_REG_FG_ID, &data);
	data &= 0xF0;
	data |= 0x0F; /* set model data version 0xF for next boot up initializing fuelgague */
	s2mu004_write_reg_byte(fuelgauge->i2c, S2MU004_REG_FG_ID, data);

	pr_info("%s: set Model data version (0x%x)\n", __func__, data & 0x0F);
}

static int s2mu004_fg_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct s2mu004_fuelgauge_data *fuelgauge =
					power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		return -ENODATA;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		val->intval = fuelgauge->pdata->capacity_full;
		break;
		/* Cell voltage (VCELL, mV) */
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = s2mu004_get_vbat(fuelgauge);
		break;
		/* Additional Voltage Information (mV) */
	case POWER_SUPPLY_PROP_VOLTAGE_AVG:
		switch (val->intval) {
		case S2MU00X_BATTERY_VOLTAGE_AVERAGE:
			val->intval = s2mu004_get_monout_avgvbat(fuelgauge);
			break;
		case S2MU00X_BATTERY_VOLTAGE_OCV:
			val->intval = s2mu004_get_ocv(fuelgauge);
			break;
		}
		break;
		/* Current (mA) */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (val->intval == S2MU00X_BATTERY_CURRENT_UA)
			val->intval = s2mu004_get_current(fuelgauge) * 1000;
		else
			val->intval = s2mu004_get_current(fuelgauge);
		break;
		/* Average Current (mA) */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		if (val->intval == S2MU00X_BATTERY_CURRENT_UA)
			val->intval = s2mu004_maintain_avgcurrent(fuelgauge) * 1000;
		else
			val->intval = s2mu004_maintain_avgcurrent(fuelgauge);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = s2mu004_get_rawsoc(fuelgauge) / 10;

		/* capacity should be between 0% and 100%
		 * (0.1% degree)
		 */
		if (val->intval > 1000)
			val->intval = 1000;
		if (val->intval < 0)
			val->intval = 0;

		/* get only integer part */
		val->intval /= 10;

		/* check whether doing the wake_unlock */
		if ((val->intval > fuelgauge->pdata->fuel_alert_soc) &&
				fuelgauge->is_fuel_alerted) {
			wake_unlock(&fuelgauge->fuel_alert_wake_lock);
			s2mu004_fuelgauge_fuelalert_init(fuelgauge->i2c,
					fuelgauge->pdata->fuel_alert_soc);
		}
		break;
	/* IFPMIC Temperature */
	case POWER_SUPPLY_PROP_TEMP:
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		val->intval = s2mu004_get_temperature(fuelgauge);
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		val->intval = fuelgauge->capacity_max;
		break;
	case POWER_SUPPLY_PROP_SCOPE:
		val->intval = fuelgauge->mode;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int s2mu004_fg_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct s2mu004_fuelgauge_data *fuelgauge =
				power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		/*full*/
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		fuelgauge->cable_type = val->intval;
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		if (val->intval)
			fuelgauge->is_charging = true;
		else
			fuelgauge->is_charging = false;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		if (val->intval == S2MU00X_FUELGAUGE_CAPACITY_TYPE_RESET) {
			fuelgauge->initial_update_of_soc = true;
			s2mu004_restart_gauging(fuelgauge);
		}
		break;
	case POWER_SUPPLY_PROP_TEMP:
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		s2mu004_set_temperature(fuelgauge, val->intval);
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		s2mu004_fg_reset_capacity_by_jig_connection(fuelgauge);
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		dev_dbg(&fuelgauge->i2c->dev,
			"%s: capacity_max changed, %d -> %d\n",
			__func__, fuelgauge->capacity_max, val->intval);
		fuelgauge->initial_update_of_soc = true;
		break;
	case POWER_SUPPLY_PROP_CHARGE_EMPTY:
		pr_info("%s: WA for battery 0 percent\n", __func__);
		s2mu004_write_reg_byte(fuelgauge->i2c, 0x1F, 0x01);
		break;
	case POWER_SUPPLY_PROP_ENERGY_AVG:
		pr_info("%s: WA for power off issue: val(%d)\n", __func__, val->intval);
		if (val->intval)
			s2mu004_write_reg_byte(fuelgauge->i2c, 0x41, 0x10); /* charger start */
		else
			s2mu004_write_reg_byte(fuelgauge->i2c, 0x41, 0x04); /* charger end */
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void s2mu004_fg_isr_work(struct work_struct *work)
{
	struct s2mu004_fuelgauge_data *fuelgauge =
		container_of(work, struct s2mu004_fuelgauge_data, isr_work.work);
	u8 fg_alert_status = 0;

	s2mu004_read_reg_byte(fuelgauge->i2c, S2MU004_REG_STATUS, &fg_alert_status);
	dev_info(&fuelgauge->i2c->dev, "%s : fg_alert_status(0x%x)\n",
			__func__, fg_alert_status);

	fg_alert_status &= 0x03;
	if (fg_alert_status & 0x01)
		pr_info("%s : Battery Level is very Low!\n", __func__);

	if (fg_alert_status & 0x02) {
		int voltage = s2mu004_get_vbat(fuelgauge);

		pr_info("%s : Battery Votage is very Low! (%dmV)\n",
				__func__, voltage);
	}

	if (!fg_alert_status) {
		fuelgauge->is_fuel_alerted = false;
		pr_info("%s : Battery Health is good!\n", __func__);
		wake_unlock(&fuelgauge->fuel_alert_wake_lock);
	}
}

static irqreturn_t s2mu004_fg_irq_thread(int irq, void *irq_data)
{
	struct s2mu004_fuelgauge_data *fuelgauge = irq_data;
	u8 fg_irq = 0;

	s2mu004_read_reg_byte(fuelgauge->i2c, S2MU004_REG_IRQ, &fg_irq);
	pr_info("%s: fg_irq(0x%x)\n", __func__, fg_irq);

	if (!fuelgauge->is_fuel_alerted) {
		wake_lock(&fuelgauge->fuel_alert_wake_lock);
		fuelgauge->is_fuel_alerted = true;
		schedule_delayed_work(&fuelgauge->isr_work, 0);
	}

	return IRQ_HANDLED;
}

#ifdef CONFIG_OF
static int s2mu004_fuelgauge_parse_dt(struct s2mu004_fuelgauge_data *fuelgauge)
{
	struct device_node *np = of_find_node_by_name(NULL, "s2mu004-fuelgauge");
	int ret;

	/* reset, irq gpio info */
	if (np == NULL) {
		pr_err("%s np NULL\n", __func__);
	} else {
		fuelgauge->pdata->fg_irq = of_get_named_gpio(np, "fuelgauge,fuel_int", 0);
		if (fuelgauge->pdata->fg_irq < 0)
			pr_err("%s error reading fg_irq = %d\n",
					__func__, fuelgauge->pdata->fg_irq);

		ret = of_property_read_u32(np, "fuelgauge,capacity_max",
				&fuelgauge->pdata->capacity_max);
		if (ret < 0)
			pr_err("%s error reading capacity_max %d\n", __func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,capacity_max_margin",
				&fuelgauge->pdata->capacity_max_margin);
		if (ret < 0)
			pr_err("%s error reading capacity_max_margin %d\n", __func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,capacity_min",
				&fuelgauge->pdata->capacity_min);
		if (ret < 0)
			pr_err("%s error reading capacity_min %d\n", __func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,fuel_alert_soc",
				&fuelgauge->pdata->fuel_alert_soc);
		if (ret < 0)
			pr_err("%s error reading pdata->fuel_alert_soc %d\n",
					__func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,capacity_full",
				&fuelgauge->pdata->capacity_full);
		if (ret < 0)
			pr_err("%s error reading pdata->capacity_full %d\n",
					__func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,fuel_alert_vol",
				&fuelgauge->pdata->fuel_alert_vol);
		if (ret < 0)
			pr_err("%s error reading pdata->fuel_alert_vol %d\n",
					__func__, ret);

		fuelgauge->pdata->repeated_fuelalert = of_property_read_bool(np,
				"fuelgauge,repeated_fuelalert");
	}

	/* get battery node */
	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s battery node NULL\n", __func__);
	} else {
		/* get battery_table */
		ret = of_property_read_u32_array(np, "battery,battery_table3", fuelgauge->info.battery_table3, 88);
		if (ret < 0)
			pr_err("%s error reading battery,battery_table3\n", __func__);

		ret = of_property_read_u32_array(np, "battery,battery_table4", fuelgauge->info.battery_table4, 22);
		if (ret < 0)
			pr_err("%s error reading battery,battery_table4\n", __func__);

		ret = of_property_read_u32_array(np, "battery,batcap", fuelgauge->info.batcap, 4);
		if (ret < 0)
			pr_err("%s error reading battery,batcap\n", __func__);

		ret = of_property_read_u32_array(np, "battery,accum", fuelgauge->info.accum, 2);
		if (ret < 0)
			pr_err("%s error reading battery,accum\n", __func__);

		ret = of_property_read_u32_array(np, "battery,soc_arr_val", fuelgauge->info.soc_arr_val, 22);
		if (ret < 0)
			pr_err("%s error reading battery,soc_arr_val\n", __func__);

		ret = of_property_read_u32_array(np, "battery,ocv_arr_val", fuelgauge->info.ocv_arr_val, 22);
		if (ret < 0)
			pr_err("%s error reading battery,ocv_arr_val\n", __func__);
	}

	return 0;
}

static const struct of_device_id s2mu004_fuelgauge_match_table[] = {
		{ .compatible = "samsung,s2mu004-fuelgauge",},
		{},
};
#else
static int s2mu004_fuelgauge_parse_dt(struct s2mu004_fuelgauge_data *fuelgauge)
{
	return -ENOSYS;
}

#define s2mu004_fuelgauge_match_table NULL
#endif /* CONFIG_OF */

static int s2mu004_fuelgauge_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct s2mu004_fuelgauge_data *fuelgauge;
	union power_supply_propval raw_soc_val;
	struct power_supply_config psy_cfg = {};
	int ret = 0;
	u8 temp = 0;

	pr_info("%s: S2MU004 Fuelgauge Driver Loading\n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	fuelgauge = kzalloc(sizeof(*fuelgauge), GFP_KERNEL);
	if (!fuelgauge)
		return -ENOMEM;

	mutex_init(&fuelgauge->fg_lock);

	fuelgauge->i2c = client;

	if (client->dev.of_node) {
		fuelgauge->pdata = devm_kzalloc(&client->dev, sizeof(*(fuelgauge->pdata)),
				GFP_KERNEL);
		if (!fuelgauge->pdata) {
			ret = -ENOMEM;
			goto err_parse_dt_nomem;
		}
		ret = s2mu004_fuelgauge_parse_dt(fuelgauge);
		if (ret < 0)
			goto err_parse_dt;
	} else {
		fuelgauge->pdata = client->dev.platform_data;
	}

	i2c_set_clientdata(client, fuelgauge);

	if (fuelgauge->pdata->fuelgauge_name == NULL)
		fuelgauge->pdata->fuelgauge_name = "s2mu004-fuelgauge";

	fuelgauge->psy_fg_desc.name          = fuelgauge->pdata->fuelgauge_name;
	fuelgauge->psy_fg_desc.type          = POWER_SUPPLY_TYPE_UNKNOWN;
	fuelgauge->psy_fg_desc.get_property  = s2mu004_fg_get_property;
	fuelgauge->psy_fg_desc.set_property  = s2mu004_fg_set_property;
	fuelgauge->psy_fg_desc.properties    = s2mu004_fuelgauge_props;
	fuelgauge->psy_fg_desc.num_properties =
			ARRAY_SIZE(s2mu004_fuelgauge_props);

	/* 0x48[7:4]=0010 : EVT2 */
	fuelgauge->revision = 0;
	s2mu004_read_reg_byte(fuelgauge->i2c, 0x48, &temp);
	fuelgauge->revision = (temp & 0xF0) >> 4;

	pr_info("%s: S2MU004 Fuelgauge revision: %d, reg 0x48 = 0x%x\n", __func__, fuelgauge->revision, temp);

	fuelgauge->capacity_max = fuelgauge->pdata->capacity_max;
	fuelgauge->info.soc = 0;
	fuelgauge->mode = CURRENT_MODE;

	raw_soc_val.intval = s2mu004_get_rawsoc(fuelgauge);
	raw_soc_val.intval = raw_soc_val.intval / 10;

	s2mu004_init_regs(fuelgauge);

	psy_cfg.drv_data = fuelgauge;
	fuelgauge->psy_fg = power_supply_register(&client->dev, &fuelgauge->psy_fg_desc, &psy_cfg);
	if (IS_ERR(fuelgauge->psy_fg)) {
		pr_err("%s: Failed to Register psy_fg\n", __func__);
		ret = PTR_ERR(fuelgauge->psy_fg);
		goto err_data_free;
	}

	fuelgauge->is_fuel_alerted = false;
	if (fuelgauge->pdata->fuel_alert_soc >= 0) {
		s2mu004_fuelgauge_fuelalert_init(fuelgauge->i2c,
					fuelgauge->pdata->fuel_alert_soc);
		wake_lock_init(&fuelgauge->fuel_alert_wake_lock,
					WAKE_LOCK_SUSPEND, "fuel_alerted");

		if (fuelgauge->pdata->fg_irq > 0) {
			INIT_DELAYED_WORK(
					&fuelgauge->isr_work, s2mu004_fg_isr_work);

			fuelgauge->fg_irq = gpio_to_irq(fuelgauge->pdata->fg_irq);
			dev_info(&client->dev,
					"%s : fg_irq = %d\n", __func__, fuelgauge->fg_irq);
			if (fuelgauge->fg_irq > 0) {
				ret = request_threaded_irq(fuelgauge->fg_irq,
						NULL, s2mu004_fg_irq_thread,
						IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
						"fuelgauge-irq", fuelgauge);
				if (ret) {
					dev_err(&client->dev,
							"%s: Failed to Request IRQ\n", __func__);
					goto err_supply_unreg;
				}

				ret = enable_irq_wake(fuelgauge->fg_irq);
				if (ret < 0)
					dev_err(&client->dev,
							"%s: Failed to Enable Wakeup Source(%d)\n",
							__func__, ret);
			} else {
				dev_err(&client->dev, "%s: Failed gpio_to_irq(%d)\n",
						__func__, fuelgauge->fg_irq);
				goto err_supply_unreg;
			}
		}
	}

	fuelgauge->sleep_initial_update_of_soc = false;
	fuelgauge->initial_update_of_soc = true;

	fuelgauge->cc_on = true;
	fuelgauge->probe_done = true;

	pr_info("%s: S2MU004 Fuelgauge Driver Loaded\n", __func__);
	return 0;

err_supply_unreg:
	power_supply_unregister(fuelgauge->psy_fg);
err_data_free:
	if (client->dev.of_node)
		kfree(fuelgauge->pdata);

err_parse_dt:
err_parse_dt_nomem:
	mutex_destroy(&fuelgauge->fg_lock);
	kfree(fuelgauge);

	return ret;
}

static const struct i2c_device_id s2mu004_fuelgauge_id[] = {
	{"s2mu004-fuelgauge", 0},
	{}
};

static void s2mu004_fuelgauge_shutdown(struct i2c_client *client)
{

}

static int s2mu004_fuelgauge_remove(struct i2c_client *client)
{
	struct s2mu004_fuelgauge_data *fuelgauge = i2c_get_clientdata(client);

	if (fuelgauge->pdata->fuel_alert_soc >= 0)
		wake_lock_destroy(&fuelgauge->fuel_alert_wake_lock);

	return 0;
}

#if defined CONFIG_PM
static int s2mu004_fuelgauge_suspend(struct device *dev)
{
	return 0;
}

static int s2mu004_fuelgauge_resume(struct device *dev)
{
	struct s2mu004_fuelgauge_data *fuelgauge = dev_get_drvdata(dev);

	fuelgauge->sleep_initial_update_of_soc = true;

	return 0;
}
#else
#define s2mu004_fuelgauge_suspend NULL
#define s2mu004_fuelgauge_resume NULL
#endif

static SIMPLE_DEV_PM_OPS(s2mu004_fuelgauge_pm_ops, s2mu004_fuelgauge_suspend,
		s2mu004_fuelgauge_resume);

static struct i2c_driver s2mu004_fuelgauge_driver = {
	.driver = {
		.name = "s2mu004-fuelgauge",
		.owner = THIS_MODULE,
		.pm = &s2mu004_fuelgauge_pm_ops,
		.of_match_table = s2mu004_fuelgauge_match_table,
	},
	.probe  = s2mu004_fuelgauge_probe,
	.remove = s2mu004_fuelgauge_remove,
	.shutdown   = s2mu004_fuelgauge_shutdown,
	.id_table   = s2mu004_fuelgauge_id,
};

static int __init s2mu004_fuelgauge_init(void)
{
	pr_info("%s: S2MU004 Fuelgauge Init\n", __func__);
	return i2c_add_driver(&s2mu004_fuelgauge_driver);
}

static void __exit s2mu004_fuelgauge_exit(void)
{
	i2c_del_driver(&s2mu004_fuelgauge_driver);
}
module_init(s2mu004_fuelgauge_init);
module_exit(s2mu004_fuelgauge_exit);

MODULE_DESCRIPTION("Samsung S2MU004 Fuel Gauge Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
