/*
 * s2mps18.c
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/err.h>
#include <linux/slab.h>
#include <linux/mfd/samsung/s2mps18.h>
#include <linux/mfd/samsung/s2mps18-private.h>
#include <linux/platform_device.h>

#define CURRENT_METER		1
#define POWER_METER 		2
#define RAWCURRENT_METER 	3
#define HIGH_ACC_CURRENT_METER	4
#define SYNC_MODE	1
#define ASYNC_MODE	2

struct adc_info *adc_meter;
struct device *s2mps18_adc_dev;
struct class *s2mps18_adc_class;

struct adc_info {
	struct i2c_client *i2c;
	u8 adc_mode;
	u8 adc_sync_mode;
	u8 *adc_reg;
	u16 *adc_val;
	u8 adc_ctrl1;
};

static const unsigned int current_buck_coeffs[14] = {CURRENT_BS, CURRENT_BT, CURRENT_BS,
	CURRENT_BS, CURRENT_BD, CURRENT_BD, CURRENT_BS, CURRENT_BS, CURRENT_BS, CURRENT_BS,
	CURRENT_BS, CURRENT_BS, CURRENT_BV, CURRENT_BS};

static const unsigned int current_ldo_coeffs[45] = {CURRENT_L450, CURRENT_L450, CURRENT_L450,
	CURRENT_L600, CURRENT_L300, CURRENT_L450, CURRENT_L300, CURRENT_L150, CURRENT_L150,
	CURRENT_L300, CURRENT_L150, CURRENT_L300, CURRENT_L150, CURRENT_L150, CURRENT_L150,
	CURRENT_L150, CURRENT_L150, CURRENT_L150, CURRENT_L150, CURRENT_L150, CURRENT_L150,
	CURRENT_L150, CURRENT_L300, CURRENT_L450, CURRENT_L600, CURRENT_L300, CURRENT_L150,
	CURRENT_L150, CURRENT_L300, CURRENT_L600, CURRENT_L800, CURRENT_L300, CURRENT_L450,
	CURRENT_L600, CURRENT_L150, CURRENT_L300, CURRENT_L300, CURRENT_L150, CURRENT_L300,
	CURRENT_L150, CURRENT_L450, CURRENT_L300, CURRENT_L450, CURRENT_L150, CURRENT_L300};

static const unsigned int power_buck_coeffs[14] = {POWER_CMS, POWER_CMT, POWER_CMS,
	POWER_CMS, POWER_CMD, POWER_CMD, POWER_CMS, POWER_CMS, POWER_CMS, POWER_CMS, POWER_CMS,
	POWER_CMS, POWER_VM, POWER_CMS};

static const unsigned int power_ldo_coeffs[45] = {POWER_N450H, POWER_P450HL, POWER_P450HL,
	POWER_N600HL, POWER_P300HL, POWER_N450DVS, POWER_N300DVS, POWER_N150HL, POWER_P150HL,
	POWER_N300DVS, POWER_N150DVS, POWER_N300H, POWER_P150HL, POWER_P150HL, POWER_N150HL,
	POWER_N150HL, POWER_N150HL, POWER_N150HL, POWER_P150HL, POWER_P150HL, POWER_P150HL,
	POWER_N150HL, POWER_P300HL, POWER_N450H, POWER_N600HL, POWER_P300HL, POWER_P150HL,
	POWER_P150HL, POWER_N300DVS, POWER_N600HL, POWER_P800L, POWER_P300HL, POWER_P450HL,
	POWER_N600HL, POWER_P150HL, POWER_P300HL, POWER_P300HL, POWER_P150HL, POWER_N300H,
	POWER_P150HL, POWER_P450HL, POWER_P300HL, POWER_P450HL, POWER_P150HL, POWER_P300HL};

static const unsigned int current_12bit_buck_coeffs[14] = {HIGH_ACC_CURRENT_BS, HIGH_ACC_CURRENT_BT,
	HIGH_ACC_CURRENT_BS, HIGH_ACC_CURRENT_BS, HIGH_ACC_CURRENT_BD, HIGH_ACC_CURRENT_BD,
	HIGH_ACC_CURRENT_BS, HIGH_ACC_CURRENT_BS, HIGH_ACC_CURRENT_BS, HIGH_ACC_CURRENT_BS,
	HIGH_ACC_CURRENT_BS, HIGH_ACC_CURRENT_BS, HIGH_ACC_CURRENT_BV, HIGH_ACC_CURRENT_BS};

static const unsigned int current_12bit_ldo_coeffs[45] = {HIGH_ACC_CURRENT_L450, HIGH_ACC_CURRENT_L450,
	HIGH_ACC_CURRENT_L450, HIGH_ACC_CURRENT_L600, HIGH_ACC_CURRENT_L300, HIGH_ACC_CURRENT_L450,
	HIGH_ACC_CURRENT_L300, HIGH_ACC_CURRENT_L150, HIGH_ACC_CURRENT_L150, HIGH_ACC_CURRENT_L300,
	HIGH_ACC_CURRENT_L150, HIGH_ACC_CURRENT_L300, HIGH_ACC_CURRENT_L150, HIGH_ACC_CURRENT_L150,
	HIGH_ACC_CURRENT_L150, HIGH_ACC_CURRENT_L150, HIGH_ACC_CURRENT_L150, HIGH_ACC_CURRENT_L150,
	HIGH_ACC_CURRENT_L150, HIGH_ACC_CURRENT_L150, HIGH_ACC_CURRENT_L150, HIGH_ACC_CURRENT_L150,
	HIGH_ACC_CURRENT_L300, HIGH_ACC_CURRENT_L450, HIGH_ACC_CURRENT_L600, HIGH_ACC_CURRENT_L300,
	HIGH_ACC_CURRENT_L150, HIGH_ACC_CURRENT_L150, HIGH_ACC_CURRENT_L300, HIGH_ACC_CURRENT_L600,
	HIGH_ACC_CURRENT_L800, HIGH_ACC_CURRENT_L300, HIGH_ACC_CURRENT_L450, HIGH_ACC_CURRENT_L600,
	HIGH_ACC_CURRENT_L150, HIGH_ACC_CURRENT_L300, HIGH_ACC_CURRENT_L300, HIGH_ACC_CURRENT_L150,
	HIGH_ACC_CURRENT_L300, HIGH_ACC_CURRENT_L150, HIGH_ACC_CURRENT_L450, HIGH_ACC_CURRENT_L300,
	HIGH_ACC_CURRENT_L450, HIGH_ACC_CURRENT_L150, HIGH_ACC_CURRENT_L300};

static void s2m_adc_read_data(struct device *dev, int channel)
{
	int i;
	u8 data_l, data_h;

	/* ASYNCRD bit '1' --> 2ms delay --> read  in case of ADC Async mode */
	if (adc_meter->adc_sync_mode == ASYNC_MODE) {
		s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL2, ADC_ASYNCRD_MASK, ADC_ASYNCRD_MASK);
		usleep_range(2000, 2100);
	}

	if (adc_meter->adc_mode == CURRENT_METER) {
		if(channel < 0) {
			for (i = 0; i < 8; i++) {
				s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3,
							i, ADC_PTR_MASK);
				s2mps18_read_reg(adc_meter->i2c, S2MPS18_REG_ADC_DATA,
							&data_l);
				adc_meter->adc_val[i] = data_l;
			}
		} else {
			s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3,
						channel, ADC_PTR_MASK);
			s2mps18_read_reg(adc_meter->i2c, S2MPS18_REG_ADC_DATA,
						&data_l);
			adc_meter->adc_val[channel] = data_l;
		}
	} else if (adc_meter->adc_mode == POWER_METER) {
		if(channel < 0) {
			for (i = 0; i < 8; i++) {
				s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3,
							2*i, ADC_PTR_MASK);
				s2mps18_read_reg(adc_meter->i2c, S2MPS18_REG_ADC_DATA,
							&data_l);

				s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3,
							2*i+1, ADC_PTR_MASK);
				s2mps18_read_reg(adc_meter->i2c, S2MPS18_REG_ADC_DATA,
							&data_h);

				adc_meter->adc_val[i] = ((data_h & 0xff) << 8) | (data_l & 0xff);
			}
		} else {
			s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3,
						2*channel, ADC_PTR_MASK);
			s2mps18_read_reg(adc_meter->i2c, S2MPS18_REG_ADC_DATA,
						&data_l);

			s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3,
						2*channel+1, ADC_PTR_MASK);
			s2mps18_read_reg(adc_meter->i2c, S2MPS18_REG_ADC_DATA,
						&data_h);

			adc_meter->adc_val[channel] = ((data_h & 0xff) << 8) | (data_l & 0xff);
		}
	} else if (adc_meter->adc_mode == HIGH_ACC_CURRENT_METER) {
		if(channel < 0) {
			for(i = 0; i < 8; i++) {
				s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3,
							2*i, ADC_PTR_MASK);
				s2mps18_read_reg(adc_meter->i2c, S2MPS18_REG_ADC_DATA,
							&data_l);

				s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3,
							2*i+1, ADC_PTR_MASK);
				s2mps18_read_reg(adc_meter->i2c, S2MPS18_REG_ADC_DATA,
							&data_h);

				adc_meter->adc_val[i] = ((data_h & 0xf) << 8) | (data_l & 0xff);
			}
		} else {
			s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3,
						2*channel, ADC_PTR_MASK);
			s2mps18_read_reg(adc_meter->i2c, S2MPS18_REG_ADC_DATA,
						&data_l);

			s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3,
						2*channel+1, ADC_PTR_MASK);
			s2mps18_read_reg(adc_meter->i2c, S2MPS18_REG_ADC_DATA,
						&data_h);

			adc_meter->adc_val[channel] = ((data_h & 0xf) << 8) | (data_l & 0xff);
		}
	} else {
		dev_err(dev, "%s: invalid adc mode(%d)\n", __func__, adc_meter->adc_mode);
	}
}

static unsigned int get_coeff(struct device *dev, u8 adc_reg_num)
{
	unsigned int coeff;

	if (adc_meter->adc_mode == CURRENT_METER) {
		/* if the regulator is LDO */
		if (adc_reg_num >= S2MPS18_LDO_START && adc_reg_num <= S2MPS18_LDO_END)
			coeff = current_ldo_coeffs[adc_reg_num - S2MPS18_LDO_START];
		/* if the regulator is BUCK */
		else if (adc_reg_num >= S2MPS18_BUCK_START && adc_reg_num <= S2MPS18_BUCK_END)
			coeff = current_buck_coeffs[adc_reg_num - S2MPS18_BUCK_START];
		else {
			dev_err(dev, "%s: invalid adc regulator number(%d)\n", __func__, adc_reg_num);
			coeff = 0;
		}
	} else if (adc_meter->adc_mode == POWER_METER) {
		/* if the regulator is LDO */
		if (adc_reg_num >= S2MPS18_LDO_START && adc_reg_num <= S2MPS18_LDO_END)
			coeff = power_ldo_coeffs[adc_reg_num - S2MPS18_LDO_START];
		/* if the regulator is BUCK */
		else if (adc_reg_num >= S2MPS18_BUCK_START && adc_reg_num <= S2MPS18_BUCK_END)
			coeff = power_buck_coeffs[adc_reg_num - S2MPS18_BUCK_START];
		else {
			dev_err(dev, "%s: invalid adc regulator number(%d)\n", __func__, adc_reg_num);
			coeff = 0;
		}
	} else if (adc_meter->adc_mode == HIGH_ACC_CURRENT_METER)  {
		/* if the regulator is LDO */
		if(adc_reg_num >= S2MPS18_LDO_START && adc_reg_num <= S2MPS18_LDO_END) 
			coeff = current_12bit_ldo_coeffs[adc_reg_num - S2MPS18_LDO_START];
		/* if the regulator is BUCK */
		else if (adc_reg_num >= S2MPS18_BUCK_START && adc_reg_num <= S2MPS18_BUCK_END)
			coeff = current_12bit_buck_coeffs[adc_reg_num - S2MPS18_BUCK_START];
		else {
			dev_err(dev, "%s: invalid adc regulator number(%d)\n", __func__, adc_reg_num);
			coeff = 0;
		}
	} else {
		dev_err(dev, "%s: invalid adc mode(%d)\n", __func__, adc_meter->adc_mode);
		coeff = 0;
	}
	return coeff;
}

static ssize_t adc_val_all_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	s2m_adc_read_data(dev, -1);
	if (adc_meter->adc_mode == POWER_METER) {
		return snprintf(buf, PAGE_SIZE, "CH0[%x]:%d uW (%d), CH1[%x]:%d uW (%d), CH2[%x]:%d uW (%d), CH3[%x]:%d uW (%d)\nCH4[%x]:%d uW (%d), CH5[%x]:%d uW (%d), CH6[%x]:%d uW (%d), CH7[%x]:%d uW (%d)\n",
			adc_meter->adc_reg[0], (adc_meter->adc_val[0] * get_coeff(dev, adc_meter->adc_reg[0]))/10, adc_meter->adc_val[0],
			adc_meter->adc_reg[1], (adc_meter->adc_val[1] * get_coeff(dev, adc_meter->adc_reg[1]))/10, adc_meter->adc_val[1],
			adc_meter->adc_reg[2], (adc_meter->adc_val[2] * get_coeff(dev, adc_meter->adc_reg[2]))/10, adc_meter->adc_val[2],
			adc_meter->adc_reg[3], (adc_meter->adc_val[3] * get_coeff(dev, adc_meter->adc_reg[3]))/10, adc_meter->adc_val[3],
			adc_meter->adc_reg[4], (adc_meter->adc_val[4] * get_coeff(dev, adc_meter->adc_reg[4]))/10, adc_meter->adc_val[4],
			adc_meter->adc_reg[5], (adc_meter->adc_val[5] * get_coeff(dev, adc_meter->adc_reg[5]))/10, adc_meter->adc_val[5],
			adc_meter->adc_reg[6], (adc_meter->adc_val[6] * get_coeff(dev, adc_meter->adc_reg[6]))/10, adc_meter->adc_val[6],
			adc_meter->adc_reg[7], (adc_meter->adc_val[7] * get_coeff(dev, adc_meter->adc_reg[7]))/10, adc_meter->adc_val[7]);
	} else {
		return snprintf(buf, PAGE_SIZE, "CH0[%x]:%d uA (%d), CH1[%x]:%d uA (%d), CH2[%x]:%d uA (%d), CH3[%x]:%d uA (%d)\nCH4[%x]:%d uA (%d), CH5[%x]:%d uA (%d), CH6[%x]:%d uA (%d), CH7[%x]:%d uA (%d)\n",
			adc_meter->adc_reg[0], adc_meter->adc_val[0] * get_coeff(dev, adc_meter->adc_reg[0]), adc_meter->adc_val[0],
			adc_meter->adc_reg[1], adc_meter->adc_val[1] * get_coeff(dev, adc_meter->adc_reg[1]), adc_meter->adc_val[1],
			adc_meter->adc_reg[2], adc_meter->adc_val[2] * get_coeff(dev, adc_meter->adc_reg[2]), adc_meter->adc_val[2],
			adc_meter->adc_reg[3], adc_meter->adc_val[3] * get_coeff(dev, adc_meter->adc_reg[3]), adc_meter->adc_val[3],
			adc_meter->adc_reg[4], adc_meter->adc_val[4] * get_coeff(dev, adc_meter->adc_reg[4]), adc_meter->adc_val[4],
			adc_meter->adc_reg[5], adc_meter->adc_val[5] * get_coeff(dev, adc_meter->adc_reg[5]), adc_meter->adc_val[5],
			adc_meter->adc_reg[6], adc_meter->adc_val[6] * get_coeff(dev, adc_meter->adc_reg[6]), adc_meter->adc_val[6],
			adc_meter->adc_reg[7], adc_meter->adc_val[7] * get_coeff(dev, adc_meter->adc_reg[7]), adc_meter->adc_val[7]);
	}
}

static ssize_t adc_en_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 adc_ctrl3;
	s2mps18_read_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3, &adc_ctrl3);
	if ((adc_ctrl3 & 0x80) == 0x80)
		return snprintf(buf, PAGE_SIZE, "ADC enable (%x)\n", adc_ctrl3);
	else
		return snprintf(buf, PAGE_SIZE, "ADC disable (%x)\n", adc_ctrl3);
}

static ssize_t adc_en_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	u8 temp, val;

	ret = kstrtou8(buf, 16, &temp);
	if (ret)
		return -EINVAL;
	else {
		switch (temp) {
		case 0 :
			val = 0x00;
			break;
		case 1 :
			val = 0x80;
			break;
		default :
			val = 0x00;
			break;
		}
	}

	s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3,
				val, ADC_EN_MASK);
	return count;
}

static ssize_t adc_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 adc_ctrl3;
	s2mps18_read_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3, &adc_ctrl3);

	adc_ctrl3 = adc_ctrl3 & 0x30;
	switch (adc_ctrl3) {
	case CURRENT_MODE :
		return snprintf(buf, PAGE_SIZE, "CURRENT MODE (%d)\n", CURRENT_METER);
	case POWER_MODE :
		return snprintf(buf, PAGE_SIZE, "POWER MODE (%d)\n", POWER_METER);
	case RAWCURRENT_MODE :
		return snprintf(buf, PAGE_SIZE, "RAW CURRENT MODE (%x)\n", RAWCURRENT_METER);
	default :
		return snprintf(buf, PAGE_SIZE, "error (%x)\n", adc_ctrl3);
	}
}

static ssize_t adc_mode_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	u8 temp, val;

	ret = kstrtou8(buf, 16, &temp);
	if (ret)
		return -EINVAL;
	else {
		switch (temp) {
		case CURRENT_METER :
			adc_meter->adc_mode = 1;
		/*	s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL1, 0x00, SMPNUM_MASK); */
			val = CURRENT_MODE;
			break;
		case POWER_METER :
			adc_meter->adc_mode = 2;
		/*	s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL1, 0x08, SMPNUM_MASK); */
			val = POWER_MODE;
			break;
		case RAWCURRENT_METER :
			adc_meter->adc_mode = 3;
		/*	s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL1, 0x00, SMPNUM_MASK); */
			val = RAWCURRENT_MODE;
			break;
		default :
			val = CURRENT_MODE;
			break;
		}
		s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3,
				(ADC_EN_MASK | val), (ADC_EN_MASK | ADC_PGEN_MASK));
		return count;
	}
}

static ssize_t adc_sync_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	switch (adc_meter->adc_sync_mode) {
	case SYNC_MODE:
		return snprintf(buf, PAGE_SIZE, "SYNC_MODE (%d)\n", adc_meter->adc_sync_mode);
	case ASYNC_MODE:
		return snprintf(buf, PAGE_SIZE, "ASYNC_MODE (%d)\n", adc_meter->adc_sync_mode);
	default:
		return snprintf(buf, PAGE_SIZE, "error (%x)\n", adc_meter->adc_sync_mode);
	}
}

static ssize_t adc_sync_mode_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	u8 temp;

	ret = kstrtou8(buf, 16, &temp);
	if (ret)
		return -EINVAL;

	switch (temp) {
	case SYNC_MODE:
		adc_meter->adc_sync_mode = 1;
		break;
	case ASYNC_MODE:
		adc_meter->adc_sync_mode = 2;
		break;
	default:
		adc_meter->adc_sync_mode = 1;
		break;
	}
	return count;
}

static ssize_t adc_val_0_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	s2m_adc_read_data(dev, 0);
	if (adc_meter->adc_mode == POWER_METER)
		return snprintf(buf, PAGE_SIZE, "%d uW\n", (adc_meter->adc_val[0] * get_coeff(dev, adc_meter->adc_reg[0]))/10);
	else
		return snprintf(buf, PAGE_SIZE, "%d uA\n", adc_meter->adc_val[0] * get_coeff(dev, adc_meter->adc_reg[0]));
}

static ssize_t adc_val_1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	s2m_adc_read_data(dev, 1);
	if (adc_meter->adc_mode == POWER_METER)
		return snprintf(buf, PAGE_SIZE, "%d uW\n", (adc_meter->adc_val[1] * get_coeff(dev, adc_meter->adc_reg[1]))/10);
	else
		return snprintf(buf, PAGE_SIZE, "%d uA\n", adc_meter->adc_val[1] * get_coeff(dev, adc_meter->adc_reg[1]));
}

static ssize_t adc_val_2_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	s2m_adc_read_data(dev, 2);
	if (adc_meter->adc_mode == POWER_METER)
		return snprintf(buf, PAGE_SIZE, "%d uW\n", (adc_meter->adc_val[2] * get_coeff(dev, adc_meter->adc_reg[2]))/10);
	else
		return snprintf(buf, PAGE_SIZE, "%d uA\n", adc_meter->adc_val[2] * get_coeff(dev, adc_meter->adc_reg[2]));
}

static ssize_t adc_val_3_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	s2m_adc_read_data(dev, 3);
	if (adc_meter->adc_mode == POWER_METER)
		return snprintf(buf, PAGE_SIZE, "%d uW\n", (adc_meter->adc_val[3] * get_coeff(dev, adc_meter->adc_reg[3]))/10);
	else
		return snprintf(buf, PAGE_SIZE, "%d uA\n", adc_meter->adc_val[3] * get_coeff(dev, adc_meter->adc_reg[3]));
}

static ssize_t adc_val_4_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	s2m_adc_read_data(dev, 4);
	if (adc_meter->adc_mode == POWER_METER)
		return snprintf(buf, PAGE_SIZE, "%d uW\n", (adc_meter->adc_val[4] * get_coeff(dev, adc_meter->adc_reg[4]))/10);
	else
		return snprintf(buf, PAGE_SIZE, "%d uA\n", adc_meter->adc_val[4] * get_coeff(dev, adc_meter->adc_reg[4]));
}

static ssize_t adc_val_5_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	s2m_adc_read_data(dev, 5);
	if (adc_meter->adc_mode == POWER_METER)
		return snprintf(buf, PAGE_SIZE, "%d uW\n", (adc_meter->adc_val[5] * get_coeff(dev, adc_meter->adc_reg[5]))/10);
	else
		return snprintf(buf, PAGE_SIZE, "%d uA\n", adc_meter->adc_val[5] * get_coeff(dev, adc_meter->adc_reg[5]));
}

static ssize_t adc_val_6_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	s2m_adc_read_data(dev, 6);
	if (adc_meter->adc_mode == POWER_METER)
		return snprintf(buf, PAGE_SIZE, "%d uW\n", (adc_meter->adc_val[6] * get_coeff(dev, adc_meter->adc_reg[6]))/10);
	else
		return snprintf(buf, PAGE_SIZE, "%d uA\n", adc_meter->adc_val[6] * get_coeff(dev, adc_meter->adc_reg[6]));
}

static ssize_t adc_val_7_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	s2m_adc_read_data(dev, 7);
	if (adc_meter->adc_mode == POWER_METER)
		return snprintf(buf, PAGE_SIZE, "%d uW\n", (adc_meter->adc_val[7] * get_coeff(dev, adc_meter->adc_reg[7]))/10);
	else
		return snprintf(buf, PAGE_SIZE, "%d uA\n", adc_meter->adc_val[7] * get_coeff(dev, adc_meter->adc_reg[7]));
}

static ssize_t adc_reg_0_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%2x\n", adc_meter->adc_reg[0]);
}

static ssize_t adc_reg_1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%2x\n", adc_meter->adc_reg[1]);
}

static ssize_t adc_reg_2_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%2x\n", adc_meter->adc_reg[2]);
}

static ssize_t adc_reg_3_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%2x\n", adc_meter->adc_reg[3]);
}

static ssize_t adc_reg_4_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%2x\n", adc_meter->adc_reg[4]);
}

static ssize_t adc_reg_5_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%2x\n", adc_meter->adc_reg[5]);
}

static ssize_t adc_reg_6_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%2x\n", adc_meter->adc_reg[6]);
}

static ssize_t adc_reg_7_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%2x\n", adc_meter->adc_reg[7]);
}

static void adc_reg_update(struct device *dev)
{
	int i = 0;

	/* ADC OFF */
	s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3,
				0x00, ADC_EN_MASK);
	/* CHANNEL setting */
	s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3,
					 CHANNELSET_MODE, ADC_PGEN_MASK);

	for (i = 0; i < 8; i++) {
		s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3,
					i, ADC_PTR_MASK);
		s2mps18_write_reg(adc_meter->i2c, S2MPS18_REG_ADC_DATA,
					adc_meter->adc_reg[i]);
	}

	/* ADC EN */
	switch (adc_meter->adc_mode) {
	case CURRENT_METER :
		s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3,
				(ADC_EN_MASK | CURRENT_MODE), (ADC_EN_MASK | ADC_PGEN_MASK));
		pr_info("%s: current mode enable (0x%2x)\n",
				 __func__,(ADC_EN_MASK | CURRENT_MODE));
		break;
	case POWER_METER:
		s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3,
				(ADC_EN_MASK | POWER_MODE), (ADC_EN_MASK | ADC_PGEN_MASK));
		pr_info("%s: power mode enable (0x%2x)\n",
				 __func__,(ADC_EN_MASK | POWER_MODE));
		break;
	default :
		s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3,
				(0x00 | CURRENT_MODE), (ADC_EN_MASK | ADC_PGEN_MASK));
		pr_info("%s: current mode enable (0x%2x)\n",
				 __func__,(ADC_EN_MASK | CURRENT_MODE));

	}
}

static u8 buf_to_adc_reg(const char *buf)
{
	u8 adc_reg_num;

	if (kstrtou8(buf, 16, &adc_reg_num))
		return 0;

	if ((adc_reg_num >= S2MPS18_BUCK_START && adc_reg_num <= S2MPS18_BUCK_END) ||
		(adc_reg_num >= S2MPS18_LDO_START && adc_reg_num <= S2MPS18_LDO_END))
		return adc_reg_num;
	else
		return 0;
}

static ssize_t adc_reg_0_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	u8 adc_reg_num = buf_to_adc_reg(buf);
	if (!adc_reg_num)
		return -EINVAL;
	else {
		adc_meter->adc_reg[0] = adc_reg_num;
		adc_reg_update(dev);
		return count;
	}
}

static ssize_t adc_reg_1_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	u8 adc_reg_num = buf_to_adc_reg(buf);
	if (!adc_reg_num)
		return -EINVAL;
	else {
		adc_meter->adc_reg[1] = adc_reg_num;
		adc_reg_update(dev);
		return count;
	}
}

static ssize_t adc_reg_2_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	u8 adc_reg_num = buf_to_adc_reg(buf);
	if (!adc_reg_num)
		return -EINVAL;
	else {
		adc_meter->adc_reg[2] = adc_reg_num;
		adc_reg_update(dev);
		return count;
	}
}

static ssize_t adc_reg_3_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	u8 adc_reg_num = buf_to_adc_reg(buf);
	if (!adc_reg_num)
		return -EINVAL;
	else {
		adc_meter->adc_reg[3] = adc_reg_num;
		adc_reg_update(dev);
		return count;
	}
}

static ssize_t adc_reg_4_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	u8 adc_reg_num = buf_to_adc_reg(buf);
	if (!adc_reg_num)
		return -EINVAL;
	else {
		adc_meter->adc_reg[4] = adc_reg_num;
		adc_reg_update(dev);
		return count;
	}
}
static ssize_t adc_reg_5_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	u8 adc_reg_num = buf_to_adc_reg(buf);
	if (!adc_reg_num)
		return -EINVAL;
	else {
		adc_meter->adc_reg[5] = adc_reg_num;
		adc_reg_update(dev);
		return count;
	}
}
static ssize_t adc_reg_6_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	u8 adc_reg_num = buf_to_adc_reg(buf);
	if (!adc_reg_num)
		return -EINVAL;
	else {
		adc_meter->adc_reg[6] = adc_reg_num;
		adc_reg_update(dev);
		return count;
	}
}
static ssize_t adc_reg_7_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	u8 adc_reg_num = buf_to_adc_reg(buf);
	if (!adc_reg_num)
		return -EINVAL;
	else {
		adc_meter->adc_reg[7] = adc_reg_num;
		adc_reg_update(dev);
		return count;
	}
}

static void adc_ctrl1_update(struct device *dev)
{
	/* ADC temporarily off */
	s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3, 0x00, ADC_EN_MASK);

	/* update ADC_CTRL1 register */
	s2mps18_write_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL1, adc_meter->adc_ctrl1);

	/* ADC Continuous ON */
	s2mps18_update_reg(adc_meter->i2c, S2MPS18_REG_ADC_CTRL3, ADC_EN_MASK, ADC_EN_MASK);
}

static ssize_t adc_ctrl1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%2x\n", adc_meter->adc_ctrl1);
}

static ssize_t adc_ctrl1_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	u8 temp;

	ret = kstrtou8(buf, 16, &temp);
	if (ret)
		return -EINVAL;
	else {
		temp &= 0x0f;
		adc_meter->adc_ctrl1 &= 0xf0;
		adc_meter->adc_ctrl1 |= temp;
		adc_ctrl1_update(dev);
		return count;
	}
}

static DEVICE_ATTR(adc_val_all, 0444, adc_val_all_show, NULL);
static DEVICE_ATTR(adc_en, 0644, adc_en_show, adc_en_store);
static DEVICE_ATTR(adc_mode, 0644, adc_mode_show, adc_mode_store);
static DEVICE_ATTR(adc_sync_mode, 0644, adc_sync_mode_show, adc_sync_mode_store);
static DEVICE_ATTR(adc_val_0, 0444, adc_val_0_show, NULL);
static DEVICE_ATTR(adc_val_1, 0444, adc_val_1_show, NULL);
static DEVICE_ATTR(adc_val_2, 0444, adc_val_2_show, NULL);
static DEVICE_ATTR(adc_val_3, 0444, adc_val_3_show, NULL);
static DEVICE_ATTR(adc_val_4, 0444, adc_val_4_show, NULL);
static DEVICE_ATTR(adc_val_5, 0444, adc_val_5_show, NULL);
static DEVICE_ATTR(adc_val_6, 0444, adc_val_6_show, NULL);
static DEVICE_ATTR(adc_val_7, 0444, adc_val_7_show, NULL);
static DEVICE_ATTR(adc_reg_0, 0644, adc_reg_0_show, adc_reg_0_store);
static DEVICE_ATTR(adc_reg_1, 0644, adc_reg_1_show, adc_reg_1_store);
static DEVICE_ATTR(adc_reg_2, 0644, adc_reg_2_show, adc_reg_2_store);
static DEVICE_ATTR(adc_reg_3, 0644, adc_reg_3_show, adc_reg_3_store);
static DEVICE_ATTR(adc_reg_4, 0644, adc_reg_4_show, adc_reg_4_store);
static DEVICE_ATTR(adc_reg_5, 0644, adc_reg_5_show, adc_reg_5_store);
static DEVICE_ATTR(adc_reg_6, 0644, adc_reg_6_show, adc_reg_6_store);
static DEVICE_ATTR(adc_reg_7, 0644, adc_reg_7_show, adc_reg_7_store);
static DEVICE_ATTR(adc_ctrl1, 0644, adc_ctrl1_show, adc_ctrl1_store);

void s2mps18_powermeter_init(struct s2mps18_dev *s2mps18)
{
	int i, ret;

	adc_meter = kzalloc(sizeof(struct adc_info), GFP_KERNEL);
	if (!adc_meter) {
		pr_err("%s: adc_meter alloc fail.\n", __func__);
		return;
	}

	adc_meter->adc_val = kzalloc(sizeof(u16)*S2MPS18_MAX_ADC_CHANNEL, GFP_KERNEL);
	adc_meter->adc_reg = kzalloc(sizeof(u8)*S2MPS18_MAX_ADC_CHANNEL, GFP_KERNEL);

	pr_info("%s: s2mps18 power meter init start\n", __func__);

	/* initial regulators : BUCK 1,2,3,4,5,6,7,8 */
	adc_meter->adc_reg[0] = 0x1;
	adc_meter->adc_reg[1] = 0x2;
	adc_meter->adc_reg[2] = 0x3;
	adc_meter->adc_reg[3] = 0x4;
	adc_meter->adc_reg[4] = 0x5;
	adc_meter->adc_reg[5] = 0x6;
	adc_meter->adc_reg[6] = 0x7;
	adc_meter->adc_reg[7] = 0x8;

	adc_meter->adc_mode = s2mps18->adc_mode;
	adc_meter->adc_sync_mode = s2mps18->adc_sync_mode;

	/* POWER_METER mode needs bigger SMP_NUM to get stable value */
	switch (adc_meter->adc_mode) {
	case CURRENT_METER :
		adc_meter->adc_ctrl1 = 0xA0;
		break;
	case POWER_METER :
		adc_meter->adc_ctrl1 = 0xA8;
		break;
	case HIGH_ACC_CURRENT_METER :
		adc_meter->adc_ctrl1 = 0xE0;
		break;
	default :
		adc_meter->adc_ctrl1 = 0xA0;
		break;
	}

	/*  SMP_NUM = 1011(16384) ~16s in case of aync mode */
	if (adc_meter->adc_sync_mode == ASYNC_MODE)
		adc_meter->adc_ctrl1 = 0xAB;

	if (s2mps18->pmic_rev == 0x00) {
		pr_info("%s: EVT0 sample ADC_CTRL1[4] = 1\n", __func__);
		adc_meter->adc_ctrl1 |= 0x10;
	}

	/* clear ADC_CTRL1[7] in order to improve calculation accuracy */
	adc_meter->adc_ctrl1 &= ~(1 << 7);

	s2mps18_write_reg(s2mps18->pmic, S2MPS18_REG_ADC_CTRL1, adc_meter->adc_ctrl1);

	/* CHANNEL setting */
	s2mps18_update_reg(s2mps18->pmic, S2MPS18_REG_ADC_CTRL3,
					 CHANNELSET_MODE, ADC_PGEN_MASK);

	for (i = 0; i < 8; i++) {
		s2mps18_update_reg(s2mps18->pmic, S2MPS18_REG_ADC_CTRL3,
					i, ADC_PTR_MASK);
		s2mps18_write_reg(s2mps18->pmic, S2MPS18_REG_ADC_DATA,
					adc_meter->adc_reg[i]);
	}

	/* ADC EN */
	switch (adc_meter->adc_mode) {
	case CURRENT_METER :
		s2mps18_update_reg(s2mps18->pmic, S2MPS18_REG_ADC_CTRL3,
				(ADC_EN_MASK | CURRENT_MODE), (ADC_EN_MASK | ADC_PGEN_MASK));
		pr_info("%s: current mode enable (0x%2x)\n",
				 __func__,(ADC_EN_MASK | CURRENT_MODE));
		break;
	case POWER_METER:
		s2mps18_update_reg(s2mps18->pmic, S2MPS18_REG_ADC_CTRL3,
				(ADC_EN_MASK | POWER_MODE), (ADC_EN_MASK | ADC_PGEN_MASK));
		pr_info("%s: power mode enable (0x%2x)\n",
				 __func__,(ADC_EN_MASK | POWER_MODE));
		break;
	case HIGH_ACC_CURRENT_METER :
		s2mps18_update_reg(s2mps18->pmic, S2MPS18_REG_ADC_CTRL3,
				(ADC_EN_MASK | CURRENT_MODE), (ADC_EN_MASK | ADC_PGEN_MASK));
		pr_info("%s : high accuracy current mode enable (0x%2x)\n",
				__func__,(ADC_EN_MASK | CURRENT_MODE));
		break;
	default :
		s2mps18_update_reg(s2mps18->pmic, S2MPS18_REG_ADC_CTRL3,
				(0x00 | CURRENT_MODE), (ADC_EN_MASK | ADC_PGEN_MASK));
		pr_info("%s: current/power meter disable (0x%2x)\n",
				 __func__,(ADC_EN_MASK | CURRENT_MODE));

	}

	adc_meter->i2c = s2mps18->pmic;

	s2mps18_adc_class = class_create(THIS_MODULE, "adc_meter");
	s2mps18_adc_dev = device_create(s2mps18_adc_class, NULL, 0, NULL, "s2mps18_adc");

	/* create sysfs entries */
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_en);
	if (ret)
		goto remove_adc_en;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_mode);
	if (ret)
		goto remove_adc_mode;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_sync_mode);
	if (ret)
		goto remove_adc_sync_mode;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_val_all);
	if (ret)
		goto remove_adc_val_all;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_val_0);
	if (ret)
		goto remove_adc_val_0;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_val_1);
	if (ret)
		goto remove_adc_val_1;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_val_2);
	if (ret)
		goto remove_adc_val_2;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_val_3);
	if (ret)
		goto remove_adc_val_3;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_val_4);
	if (ret)
		goto remove_adc_val_4;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_val_5);
	if (ret)
		goto remove_adc_val_5;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_val_6);
	if (ret)
		goto remove_adc_val_6;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_val_7);
	if (ret)
		goto remove_adc_val_7;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_reg_0);
	if (ret)
		goto remove_adc_reg_0;;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_reg_1);
	if (ret)
		goto remove_adc_reg_1;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_reg_2);
	if (ret)
		goto remove_adc_reg_2;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_reg_3);
	if (ret)
		goto remove_adc_reg_3;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_reg_4);
	if (ret)
		goto remove_adc_reg_4;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_reg_5);
	if (ret)
		goto remove_adc_reg_5;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_reg_6);
	if (ret)
		goto remove_adc_reg_6;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_reg_7);
	if (ret)
		goto remove_adc_reg_7;
	ret = device_create_file(s2mps18_adc_dev, &dev_attr_adc_ctrl1);
	if (ret)
		goto remove_adc_ctrl1;

	pr_info("%s: s2mps18 power meter init end\n", __func__);
	return ;
remove_adc_ctrl1:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_ctrl1);
remove_adc_reg_7:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_reg_7);
remove_adc_reg_6:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_reg_6);
remove_adc_reg_5:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_reg_5);
remove_adc_reg_4:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_reg_4);
remove_adc_reg_3:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_reg_3);
remove_adc_reg_2:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_reg_2);
remove_adc_reg_1:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_reg_1);
remove_adc_reg_0:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_reg_0);
remove_adc_val_7:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_val_7);
remove_adc_val_6:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_val_6);
remove_adc_val_5:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_val_5);
remove_adc_val_4:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_val_4);
remove_adc_val_3:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_val_3);
remove_adc_val_2:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_val_2);
remove_adc_val_1:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_val_1);
remove_adc_val_0:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_val_0);
remove_adc_val_all:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_val_all);
remove_adc_sync_mode:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_sync_mode);
remove_adc_mode:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_mode);
remove_adc_en:
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_en);

	kfree(adc_meter->adc_val);
	kfree(adc_meter->adc_reg);
	dev_info(s2mps18->dev, "%s : fail to create sysfs\n",__func__);
	return ;
}

void s2mps18_powermeter_deinit(struct s2mps18_dev *s2mps18)
{
	/* remove sysfs entries */
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_en);
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_val_all);
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_mode);
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_sync_mode);
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_val_0);
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_val_1);
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_val_2);
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_val_3);
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_val_4);
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_val_5);
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_val_6);
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_val_7);
	device_remove_file(s2mps18_adc_dev, &dev_attr_adc_ctrl1);

	/* ADC turned off */
	s2mps18_write_reg(s2mps18->pmic, S2MPS18_REG_ADC_CTRL3, 0);
	kfree(adc_meter->adc_val);
	kfree(adc_meter->adc_reg);
}
