/*
 * s2dos05.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef __LINUX_MFD_S2DOS05_H
#define __LINUX_MFD_S2DOS05_H
#include <linux/platform_device.h>
#include <linux/regmap.h>

#define MFD_DEV_NAME "s2dos05"

/**
 * sec_regulator_data - regulator data
 * @id: regulator id
 * @initdata: regulator init data (contraints, supplies, ...)
 */

struct s2dos05_dev {
	struct device *dev;
	struct i2c_client *i2c; /* 0xB2; PMIC, Flash LED */
	struct mutex i2c_lock;

	int type;
	u8 rev_num; /* pmic Rev */
	bool wakeup;
	int dp_pmic_irq;
	int		adc_mode;
	int		adc_sync_mode;
	u8 adc_en_val;

	struct s2dos05_platform_data *pdata;
};

struct s2dos05_regulator_data {
	int id;
	struct regulator_init_data *initdata;
	struct device_node *reg_node;
};

struct s2dos05_platform_data {
	bool wakeup;
	int num_regulators;
	struct	s2dos05_regulator_data *regulators;
	int	device_type;
	int dp_pmic_irq;

	/* adc_mode
	 * 0 : not use
	 * 1 : current meter
	 * 2 : power meter
	*/
	int		adc_mode;
	/* 1 : sync mode, 2 : async mode  */
	int		adc_sync_mode;
};

struct s2dos05 {
	struct regmap *regmap;
};

/* S2DOS05 registers */
/* Slave Addr : 0xC0 */
enum S2DOS05_reg {
	S2DOS05_REG_DEV_ID,
	S2DOS05_REG_TOPSYS_STAT,
	S2DOS05_REG_STAT,
	S2DOS05_REG_EN,
	S2DOS05_REG_LDO1_CFG,
	S2DOS05_REG_LDO2_CFG,
	S2DOS05_REG_LDO3_CFG,
	S2DOS05_REG_LDO4_CFG,
	S2DOS05_REG_BUCK_CFG,
	S2DOS05_REG_BUCK_VOUT,
	S2DOS05_REG_IRQ_MASK = 0x0D,
	S2DOS05_REG_IRQ = 0x11,
};

/* S2DOS05 regulator ids */
enum S2DOS05_regulators {
	S2DOS05_LDO1,
	S2DOS05_LDO2,
	S2DOS05_LDO3,
	S2DOS05_LDO4,
	S2DOS05_BUCK1,
	S2DOS05_REG_MAX,
};

#define S2DOS05_IRQ_PWRMT_MASK	(1 << 5)
#define S2DOS05_IRQ_TSD_MASK	(1 << 4)
#define S2DOS05_IRQ_SSD_MASK	(1 << 3)
#define S2DOS05_IRQ_SCP_MASK	(1 << 2)
#define S2DOS05_IRQ_UVLO_MASK	(1 << 1)
#define S2DOS05_IRQ_OCD_MASK	(1 << 0)

#define S2DOS05_BUCK_MIN1	506250
#define S2DOS05_LDO_MIN1	1500000
#define S2DOS05_LDO_MIN2	2700000
#define S2DOS05_BUCK_STEP1	6250
#define S2DOS05_LDO_STEP1	25000
#define S2DOS05_LDO_VSEL_MASK	0x7F
#define S2DOS05_BUCK_VSEL_MASK	0xFF

#define S2DOS05_ENABLE_MASK_L1	1 << 0
#define S2DOS05_ENABLE_MASK_L2	1 << 1
#define S2DOS05_ENABLE_MASK_L3	1 << 2
#define S2DOS05_ENABLE_MASK_L4	1 << 3
#define S2DOS05_ENABLE_MASK_B1	1 << 4

#define S2DOS05_RAMP_DELAY	12000

#define S2DOS05_ENABLE_TIME_LDO		50
#define S2DOS05_ENABLE_TIME_BUCK	350

#define S2DOS05_ENABLE_SHIFT	0x06
#define S2DOS05_LDO_N_VOLTAGES	(S2DOS05_LDO_VSEL_MASK + 1)
#define S2DOS05_BUCK_N_VOLTAGES (S2DOS05_BUCK_VSEL_MASK + 1)

#define S2DOS05_PMIC_EN_SHIFT	6
#define S2DOS05_REGULATOR_MAX (S2DOS05_REG_MAX)

/* ----------power meter ----------*/
#define S2DOS05_REG_PWRMT_CTRL1		0x0A
#define S2DOS05_REG_PWRMT_CTRL2		0x0B
#define S2DOS05_REG_PWRMT_DATA	0x0C
#define S2DOS05_REG_IRQ_MASK	0x0D

#define CURRENT_ELVDD			2450
#define CURRENT_ELVSS			2450
#define CURRENT_AVDD		612
#define CURRENT_BUCK			1220
#define CURRENT_L1			2000
#define CURRENT_L2			2000
#define CURRENT_L3			2000
#define CURRENT_L4			2000

#define POWER_ELVDD			24500
#define	POWER_ELVSS			24500
#define POWER_AVDD			3060
#define POWER_BUCK			1525
#define POWER_L1			5000
#define POWER_L2			5000
#define POWER_L3			5000
#define POWER_L4			5000

#define ADC_EN_MASK			0x80
#define ADC_ASYNCRD_MASK	0x80
#define ADC_PTR_MASK			0x0F
#define ADC_PGEN_MASK			0x30
#define CURRENT_MODE			0x00
#define POWER_MODE			0x10
#define RAWCURRENT_MODE			0x20
#define SMPNUM_MASK			0x0F

#define S2DOS05_MAX_ADC_CHANNEL		8

extern void s2dos05_powermeter_init(struct s2dos05_dev *s2dos05);
extern void s2dos05_powermeter_deinit(struct s2dos05_dev *s2dos05);

/* S2DOS05 shared i2c API function */
extern int s2dos05_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest);
extern int s2dos05_write_reg(struct i2c_client *i2c, u8 reg, u8 value);
extern int s2dos05_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask);


#endif /*  __LINUX_MFD_S2DOS05_H */
