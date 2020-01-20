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

#include <linux/bug.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <../drivers/pinctrl/samsung/pinctrl-samsung.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/mfd/samsung/s2mps18.h>
#include <linux/mfd/samsung/s2mps18-private.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/debug-snapshot.h>
#include <linux/debugfs.h>
#include <linux/interrupt.h>

static struct s2mps18_info *static_info;
static struct regulator_desc regulators[S2MPS18_REGULATOR_MAX];

#ifdef CONFIG_DEBUG_FS
static u8 i2caddr = 0;
static u8 i2cdata = 0;
static struct i2c_client *dbgi2c = NULL;
static struct dentry *s2mps18_root = NULL;
static struct dentry *s2mps18_i2caddr = NULL;
static struct dentry *s2mps18_i2cdata = NULL;
#endif

struct s2mps18_info {
	struct regulator_dev *rdev[S2MPS18_REGULATOR_MAX];
	unsigned int opmode[S2MPS18_REGULATOR_MAX];
	int num_regulators;
	struct s2mps18_dev *iodev;
	struct mutex lock;
	bool g3d_en;
	struct i2c_client *i2c;
	struct i2c_client *debug_i2c;
	u8 adc_en_val;
	int b2_ocp_irq;
	int b3_ocp_irq;
	int th120C_irq;
};

static unsigned int s2mps18_of_map_mode(unsigned int val) {
	switch (val) {
	case SEC_OPMODE_SUSPEND:	/* ON in Standby Mode */
		return 0x1;
	case SEC_OPMODE_MIF:		/* ON in PWREN_MIF mode */
		return 0x2;
	case SEC_OPMODE_ON:		/* ON in Normal Mode */
		return 0x3;
	default:
		return 0x3;
	}
}

/* Some LDOs supports [LPM/Normal]ON mode during suspend state */
static int s2m_set_mode(struct regulator_dev *rdev,
				     unsigned int mode)
{
	struct s2mps18_info *s2mps18 = rdev_get_drvdata(rdev);
	unsigned int val;
	int id = rdev_get_id(rdev);

	val = mode << S2MPS18_ENABLE_SHIFT;

	s2mps18->opmode[id] = val;
	return 0;
}

/* set initial mode of LDO9,10,11,12,13,14 */
int s2m_ldo_set_mode(int ldo_num, unsigned int mode)
{
	unsigned int val, ldo_addr;
	int ret;

	if (ldo_num < 9 || ldo_num > 14) {
		pr_warn("Wrong LDO number! [%d]\n", ldo_num);
		return -EINVAL;
	}

	val = mode << S2MPS18_ENABLE_SHIFT;
	ldo_addr = S2MPS18_PMIC_REG_L9CTRL + (ldo_num - 9);

	ret = s2mps18_update_reg(static_info->i2c, ldo_addr, val, 0xC0);
	if (ret)
		return ret;

	pr_debug("%s : LDO%d mode [%d]\n", __func__, ldo_num, mode);

	static_info->opmode[ldo_num] = val;
	return 0;
}
EXPORT_SYMBOL_GPL(s2m_ldo_set_mode);

static int s2m_enable(struct regulator_dev *rdev)
{
	struct s2mps18_info *s2mps18 = rdev_get_drvdata(rdev);

	int reg_id = rdev_get_id(rdev);

	/* disregard BUCK6 enable */
	if (reg_id == S2MPS18_BUCK6 && s2mps18->g3d_en)
		return 0;

	return s2mps18_update_reg(s2mps18->i2c, rdev->desc->enable_reg,
				  s2mps18->opmode[rdev_get_id(rdev)],
				  rdev->desc->enable_mask);
}

static int s2m_disable_regmap(struct regulator_dev *rdev)
{
	struct s2mps18_info *s2mps18 = rdev_get_drvdata(rdev);
	int reg_id = rdev_get_id(rdev);
	unsigned int val;

	/* disregard BUCK6 disable */
	if (reg_id == S2MPS18_BUCK6 && s2mps18->g3d_en)
		return 0;

	if (rdev->desc->enable_is_inverted)
		val = rdev->desc->enable_mask;
	else
		val = 0;

	return s2mps18_update_reg(s2mps18->i2c, rdev->desc->enable_reg,
				  val, rdev->desc->enable_mask);
}

static int s2m_is_enabled_regmap(struct regulator_dev *rdev)
{
	struct s2mps18_info *s2mps18 = rdev_get_drvdata(rdev);
	int ret, reg_id = rdev_get_id(rdev);
	u8 val;

	/* BUCK6 is controlled by g3d gpio */
	if (reg_id == S2MPS18_BUCK6 && s2mps18->g3d_en) {
/*		exynos_pmu_read(EXYNOS_PMU_G3D_STATUS, &val);
		if ((val & LOCAL_PWR_CFG) == LOCAL_PWR_CFG)
			return 1;
		else
*/
			return 0;
	} else {
		ret = s2mps18_read_reg(s2mps18->i2c,
				rdev->desc->enable_reg, &val);
		if (ret)
			return ret;
	}

	if (rdev->desc->enable_is_inverted)
		return (val & rdev->desc->enable_mask) == 0;
	else
		return (val & rdev->desc->enable_mask) != 0;
}

static int get_ramp_delay(int ramp_delay)
{
	unsigned char cnt = 0;

	ramp_delay /= 6;

	while (true) {
		ramp_delay = ramp_delay >> 1;
		if (ramp_delay == 0)
			break;
		cnt++;
	}
	return cnt;
}

static int s2m_set_ramp_delay(struct regulator_dev *rdev, int ramp_delay)
{
	struct s2mps18_info *s2mps18 = rdev_get_drvdata(rdev);
	int ramp_shift, reg_id = rdev_get_id(rdev);
	int ramp_mask = 0x03;
	int ramp_addr;
	unsigned int ramp_value = 0;

	ramp_value = get_ramp_delay(ramp_delay/1000);
	if (ramp_value > 4) {
		pr_warn("%s: ramp_delay: %d not supported\n",
			rdev->desc->name, ramp_delay);
	}

	switch (reg_id) {
	case S2MPS18_BUCK1:
	case S2MPS18_BUCK3:
	case S2MPS18_BUCK7:
		ramp_shift = 6;
		break;
	case S2MPS18_BUCK2:
	case S2MPS18_BUCK8:
	case S2MPS18_BUCK9:
		ramp_shift = 4;
		break;
	case S2MPS18_BUCK4:
/*	case S2MPS18_BUCK14:*/
	case S2MPS18_BUCK10:
	case S2MPS18_BUCK11:
	case S2MPS18_BUCK12:
		ramp_shift = 2;
		break;
	case S2MPS18_BUCK5:
	case S2MPS18_BUCK6:
	case S2MPS18_BUCK13:
/*	case S2MPS18_BB1:*/
		ramp_shift = 0;
		break;
	default:
		return -EINVAL;
	}

	switch(reg_id) {
	case S2MPS18_BUCK1:
	case S2MPS18_BUCK2:
	case S2MPS18_BUCK3:
	case S2MPS18_BUCK4:
	case S2MPS18_BUCK5:
	case S2MPS18_BUCK6:
/*	case S2MPS18_BUCK14:*/
		ramp_addr = S2MPS18_PMIC_REG_BUCKRAMP;
		break;
	case S2MPS18_BUCK7:
	case S2MPS18_BUCK8:
	case S2MPS18_BUCK9:
	case S2MPS18_BUCK10:
	case S2MPS18_BUCK11:
	case S2MPS18_BUCK12:
	case S2MPS18_BUCK13:
/*	case S2MPS18_BB1:*/
		ramp_addr = S2MPS18_PMIC_REG_BUCKRAMP2;
		break;
	default:
		return -EINVAL;
	}

	return s2mps18_update_reg(s2mps18->i2c, ramp_addr,
		ramp_value << ramp_shift, ramp_mask << ramp_shift);
}

static int s2m_get_voltage_sel_regmap(struct regulator_dev *rdev)
{
	struct s2mps18_info *s2mps18 = rdev_get_drvdata(rdev);
	int ret;
	u8 val;

	ret = s2mps18_read_reg(s2mps18->i2c, rdev->desc->vsel_reg, &val);
	if (ret)
		return ret;

	val &= rdev->desc->vsel_mask;

	return val;
}

static int s2m_set_voltage_sel_regmap(struct regulator_dev *rdev, unsigned sel)
{
	struct s2mps18_info *s2mps18 = rdev_get_drvdata(rdev);
	int reg_id = rdev_get_id(rdev);
	int ret;
	char name[16];

	/* voltage information logging to snapshot feature */
	snprintf(name, sizeof(name), "LDO%d", (reg_id - S2MPS18_LDO1) + 1);
	ret = s2mps18_update_reg(s2mps18->i2c, rdev->desc->vsel_reg,
				  sel, rdev->desc->vsel_mask);
	if (ret < 0)
		goto out;

	if (rdev->desc->apply_bit)
		ret = s2mps18_update_reg(s2mps18->i2c, rdev->desc->apply_reg,
					 rdev->desc->apply_bit,
					 rdev->desc->apply_bit);
	return ret;
out:
	pr_warn("%s: failed to set voltage_sel_regmap\n", rdev->desc->name);
	return ret;
}

static int s2m_set_voltage_sel_regmap_buck(struct regulator_dev *rdev,
								unsigned sel)
{
	int ret = 0;
	struct s2mps18_info *s2mps18 = rdev_get_drvdata(rdev);

	ret = s2mps18_write_reg(s2mps18->i2c, rdev->desc->vsel_reg, sel);
	if (ret < 0)
		goto i2c_out;

	if (rdev->desc->apply_bit)
		ret = s2mps18_update_reg(s2mps18->i2c, rdev->desc->apply_reg,
					 rdev->desc->apply_bit,
					 rdev->desc->apply_bit);

	return ret;

i2c_out:
	pr_warn("%s: failed to set voltage_sel_regmap\n", rdev->desc->name);
	ret = -EINVAL;
	return ret;
}

static int s2m_set_voltage_time_sel(struct regulator_dev *rdev,
				   unsigned int old_selector,
				   unsigned int new_selector)
{
	unsigned int ramp_delay = 0;
	int old_volt, new_volt;

	if (rdev->constraints->ramp_delay)
		ramp_delay = rdev->constraints->ramp_delay;
	else if (rdev->desc->ramp_delay)
		ramp_delay = rdev->desc->ramp_delay;

	if (ramp_delay == 0) {
		pr_warn("%s: ramp_delay not set\n", rdev->desc->name);
		return -EINVAL;
	}

	/* sanity check */
	if (!rdev->desc->ops->list_voltage)
		return -EINVAL;

	old_volt = rdev->desc->ops->list_voltage(rdev, old_selector);
	new_volt = rdev->desc->ops->list_voltage(rdev, new_selector);

	if (old_selector < new_selector)
		return DIV_ROUND_UP(new_volt - old_volt, ramp_delay);

	return 0;
}


void s2m_init_dvs(void)
{
/*	if (cal_dfs_ext_ctrl(dvfs_g3d, cal_dfs_dvs, 2) != 0) {
		pr_info("%s: failed to init dvs\n", __func__);
	}*/
}

static struct regulator_ops s2mps18_ldo_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.is_enabled		= s2m_is_enabled_regmap,
	.enable			= s2m_enable,
	.disable		= s2m_disable_regmap,
	.get_voltage_sel	= s2m_get_voltage_sel_regmap,
	.set_voltage_sel	= s2m_set_voltage_sel_regmap,
	.set_voltage_time_sel	= s2m_set_voltage_time_sel,
	.set_mode		= s2m_set_mode,
};

static struct regulator_ops s2mps18_buck_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.is_enabled		= s2m_is_enabled_regmap,
	.enable			= s2m_enable,
	.disable		= s2m_disable_regmap,
	.get_voltage_sel	= s2m_get_voltage_sel_regmap,
	.set_voltage_sel	= s2m_set_voltage_sel_regmap_buck,
	.set_voltage_time_sel	= s2m_set_voltage_time_sel,
	.set_mode		= s2m_set_mode,
	.set_ramp_delay		= s2m_set_ramp_delay,
};

#define _BUCK(macro)	S2MPS18_BUCK##macro
#define _buck_ops(num)	s2mps18_buck_ops##num

#define _LDO(macro)	S2MPS18_LDO##macro
#define _REG(ctrl)	S2MPS18_PMIC_REG##ctrl
#define _ldo_ops(num)	s2mps18_ldo_ops##num
#define _TIME(macro)	S2MPS18_ENABLE_TIME##macro

#define BUCK_DESC(_name, _id, _ops, m, s, v, e, t)	{	\
	.name		= _name,				\
	.id		= _id,					\
	.ops		= _ops,					\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= m,					\
	.uV_step	= s,					\
	.n_voltages	= S2MPS18_BUCK_N_VOLTAGES,		\
	.vsel_reg	= v,					\
	.vsel_mask	= S2MPS18_BUCK_VSEL_MASK,		\
	.enable_reg	= e,					\
	.enable_mask	= S2MPS18_ENABLE_MASK,			\
	.enable_time	= t,					\
	.of_map_mode	= s2mps18_of_map_mode			\
}

#define LDO_DESC(_name, _id, _ops, m, s, v, e, t)	{	\
	.name		= _name,				\
	.id		= _id,					\
	.ops		= _ops,					\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= m,					\
	.uV_step	= s,					\
	.n_voltages	= S2MPS18_LDO_N_VOLTAGES,		\
	.vsel_reg	= v,					\
	.vsel_mask	= S2MPS18_LDO_VSEL_MASK,		\
	.enable_reg	= e,					\
	.enable_mask	= S2MPS18_ENABLE_MASK,			\
	.enable_time	= t,					\
	.of_map_mode	= s2mps18_of_map_mode			\
}

static struct regulator_desc regulators[S2MPS18_REGULATOR_MAX] = {
	/* name, id, ops, min_uv, uV_step, vsel_reg, enable_reg */
	LDO_DESC("LDO1", _LDO(1), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP1), _REG(_L1CTRL), _REG(_L1CTRL), _TIME(_LDO)),
	LDO_DESC("LDO2", _LDO(2), &_ldo_ops(), _LDO(_MIN3),
	_LDO(_STEP2), _REG(_L2CTRL1), _REG(_L2CTRL1), _TIME(_LDO)),
	LDO_DESC("LDO3", _LDO(3), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP2), _REG(_L3CTRL), _REG(_L3CTRL), _TIME(_LDO)),
	LDO_DESC("LDO4", _LDO(4), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP1), _REG(_L4CTRL), _REG(_L4CTRL), _TIME(_LDO)),
	LDO_DESC("LDO5", _LDO(5), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP2), _REG(_L5CTRL), _REG(_L5CTRL), _TIME(_LDO)),
	LDO_DESC("LDO6", _LDO(6), &_ldo_ops(), _LDO(_MIN4),
	_LDO(_STEP2), _REG(_L6CTRL), _REG(_L6CTRL), _TIME(_LDO)),
	LDO_DESC("LDO7", _LDO(7), &_ldo_ops(), _LDO(_MIN4),
	_LDO(_STEP2), _REG(_L7CTRL), _REG(_L7CTRL), _TIME(_LDO)),
	LDO_DESC("LDO8", _LDO(8), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP1), _REG(_L8CTRL), _REG(_L8CTRL), _TIME(_LDO)),
	LDO_DESC("LDO9", _LDO(9), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP2), _REG(_L9CTRL), _REG(_L9CTRL), _TIME(_LDO)),
	LDO_DESC("LDO10", _LDO(10), &_ldo_ops(), _LDO(_MIN4),
	_LDO(_STEP2), _REG(_L10CTRL), _REG(_L10CTRL), _TIME(_LDO)),
	LDO_DESC("LDO11", _LDO(11), &_ldo_ops(), _LDO(_MIN4),
	_LDO(_STEP2), _REG(_L11CTRL), _REG(_L11CTRL), _TIME(_LDO)),
	LDO_DESC("LDO12", _LDO(12), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP1), _REG(_L12CTRL), _REG(_L12CTRL), _TIME(_LDO)),
	LDO_DESC("LDO13", _LDO(13), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP2), _REG(_L13CTRL), _REG(_L13CTRL), _TIME(_LDO)),
	LDO_DESC("LDO14", _LDO(14), &_ldo_ops(), _LDO(_MIN3),
	_LDO(_STEP2), _REG(_L14CTRL), _REG(_L14CTRL), _TIME(_LDO)),
/*	LDO_DESC("LDO15", _LDO(15), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP1), _REG(_L15CTRL), _REG(_L15CTRL), _TIME(_LDO)),
	LDO_DESC("LDO16", _LDO(16), &_ldo_ops(), _LDO(_MIN2),
	_LDO(_STEP1), _REG(_L16CTRL), _REG(_L16CTRL), _TIME(_LDO)),
	LDO_DESC("LDO17", _LDO(17), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP1), _REG(_L17CTRL), _REG(_L17CTRL), _TIME(_LDO)),
	LDO_DESC("LDO18", _LDO(18), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP1), _REG(_L18CTRL), _REG(_L18CTRL), _TIME(_LDO)),
	LDO_DESC("LDO19", _LDO(19), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP2), _REG(_L19CTRL), _REG(_L19CTRL), _TIME(_LDO)),
	LDO_DESC("LDO20", _LDO(20), &_ldo_ops(), _LDO(_MIN3),
	_LDO(_STEP2), _REG(_L20CTRL), _REG(_L20CTRL), _TIME(_LDO)),
	LDO_DESC("LDO21", _LDO(21), &_ldo_ops(), _LDO(_MIN3),
	_LDO(_STEP2), _REG(_L21CTRL), _REG(_L21CTRL), _TIME(_LDO)),*/
	LDO_DESC("LDO22", _LDO(22), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP1), _REG(_L22CTRL), _REG(_L22CTRL), _TIME(_LDO)),
/*	LDO_DESC("LDO23", _LDO(23), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP2), _REG(_L23CTRL), _REG(_L23CTRL), _TIME(_LDO)),
	LDO_DESC("LDO24", _LDO(24), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP1), _REG(_L24CTRL), _REG(_L24CTRL), _TIME(_LDO)),
	LDO_DESC("LDO25", _LDO(25), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP1), _REG(_L25CTRL), _REG(_L25CTRL), _TIME(_LDO)),
	LDO_DESC("LDO26", _LDO(26), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP2), _REG(_L26CTRL), _REG(_L26CTRL), _TIME(_LDO)),
	LDO_DESC("LDO27", _LDO(27), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP2), _REG(_L27CTRL), _REG(_L27CTRL), _TIME(_LDO)),
	LDO_DESC("LDO28", _LDO(28), &_ldo_ops(), _LDO(_MIN3),
	_LDO(_STEP2), _REG(_L28CTRL), _REG(_L28CTRL), _TIME(_LDO)),
	LDO_DESC("LDO29", _LDO(29), &_ldo_ops(), _LDO(_MIN4),
	_LDO(_STEP2), _REG(_L29CTRL), _REG(_L29CTRL), _TIME(_LDO)),*/
	LDO_DESC("LDO30", _LDO(30), &_ldo_ops(), _LDO(_MIN2),
	_LDO(_STEP1), _REG(_L30CTRL), _REG(_L30CTRL), _TIME(_LDO)),
	LDO_DESC("LDO31", _LDO(31), &_ldo_ops(), _LDO(_MIN3),
	_LDO(_STEP2), _REG(_L31CTRL), _REG(_L31CTRL), _TIME(_LDO)),
	LDO_DESC("LDO32", _LDO(32), &_ldo_ops(), _LDO(_MIN3),
	_LDO(_STEP2), _REG(_L32CTRL), _REG(_L32CTRL), _TIME(_LDO)),
	LDO_DESC("LDO33", _LDO(33), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP2), _REG(_L33CTRL), _REG(_L33CTRL), _TIME(_LDO)),
	LDO_DESC("LDO34", _LDO(34), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP1), _REG(_L34CTRL), _REG(_L34CTRL), _TIME(_LDO)),
	LDO_DESC("LDO35", _LDO(35), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP2), _REG(_L35CTRL), _REG(_L35CTRL), _TIME(_LDO)),
	LDO_DESC("LDO36", _LDO(36), &_ldo_ops(), _LDO(_MIN3),
	_LDO(_STEP2), _REG(_L36CTRL), _REG(_L36CTRL), _TIME(_LDO)),
	LDO_DESC("LDO37", _LDO(37), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP2), _REG(_L37CTRL), _REG(_L37CTRL), _TIME(_LDO)),
	LDO_DESC("LDO38", _LDO(38), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP2), _REG(_L38CTRL), _REG(_L38CTRL), _TIME(_LDO)),
	LDO_DESC("LDO39", _LDO(39), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP1), _REG(_L39CTRL), _REG(_L39CTRL), _TIME(_LDO)),
	LDO_DESC("LDO40", _LDO(40), &_ldo_ops(), _LDO(_MIN3),
	_LDO(_STEP2), _REG(_L40CTRL), _REG(_L40CTRL), _TIME(_LDO)),
	LDO_DESC("LDO41", _LDO(41), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP2), _REG(_L41CTRL), _REG(_L41CTRL), _TIME(_LDO)),
	LDO_DESC("LDO42", _LDO(42), &_ldo_ops(), _LDO(_MIN3),
	_LDO(_STEP2), _REG(_L42CTRL), _REG(_L42CTRL), _TIME(_LDO)),
	LDO_DESC("LDO43", _LDO(43), &_ldo_ops(), _LDO(_MIN3),
	_LDO(_STEP2), _REG(_L43CTRL), _REG(_L43CTRL), _TIME(_LDO)),
	LDO_DESC("LDO44", _LDO(44), &_ldo_ops(), _LDO(_MIN1),
	_LDO(_STEP2), _REG(_L44CTRL), _REG(_L44CTRL), _TIME(_LDO)),
	LDO_DESC("LDO45", _LDO(45), &_ldo_ops(), _LDO(_MIN3),
	_LDO(_STEP2), _REG(_L45CTRL), _REG(_L45CTRL), _TIME(_LDO)),
	BUCK_DESC("BUCK1", _BUCK(1), &_buck_ops(), _BUCK(_MIN1),
	_BUCK(_STEP1), _REG(_B1OUT1), _REG(_B1CTRL), _TIME(_BUCK1)),
	BUCK_DESC("BUCK2", _BUCK(2), &_buck_ops(), _BUCK(_MIN1),
	_BUCK(_STEP1), _REG(_B2OUT1), _REG(_B2CTRL), _TIME(_BUCK2)),
	BUCK_DESC("BUCK3", _BUCK(3), &_buck_ops(), _BUCK(_MIN1),
	_BUCK(_STEP1), _REG(_B3OUT1), _REG(_B3CTRL), _TIME(_BUCK3)),
	BUCK_DESC("BUCK4", _BUCK(4), &_buck_ops(), _BUCK(_MIN1),
	_BUCK(_STEP1), _REG(_B4OUT1), _REG(_B4CTRL), _TIME(_BUCK4)),
	BUCK_DESC("BUCK5", _BUCK(5), &_buck_ops(), _BUCK(_MIN1),
	_BUCK(_STEP1), _REG(_B5OUT), _REG(_B5CTRL), _TIME(_BUCK5)),
	BUCK_DESC("BUCK6", _BUCK(6), &_buck_ops(), _BUCK(_MIN1),
	_BUCK(_STEP1), _REG(_B6OUT), _REG(_B6CTRL), _TIME(_BUCK6)),
	BUCK_DESC("BUCK7", _BUCK(7), &_buck_ops(), _BUCK(_MIN1),
	_BUCK(_STEP1), _REG(_B7OUT), _REG(_B7CTRL), _TIME(_BUCK7)),
	BUCK_DESC("BUCK8", _BUCK(8), &_buck_ops(), _BUCK(_MIN1),
	_BUCK(_STEP1), _REG(_B8OUT), _REG(_B8CTRL), _TIME(_BUCK8)),
	BUCK_DESC("BUCK9", _BUCK(9), &_buck_ops(), _BUCK(_MIN1),
	_BUCK(_STEP1), _REG(_B9OUT), _REG(_B9CTRL), _TIME(_BUCK9)),
	BUCK_DESC("BUCK10", _BUCK(10), &_buck_ops(), _BUCK(_MIN1),
	_BUCK(_STEP1), _REG(_B10OUT), _REG(_B10CTRL), _TIME(_BUCK10)),
	BUCK_DESC("BUCK11", _BUCK(11), &_buck_ops(), _BUCK(_MIN1),
	_BUCK(_STEP1), _REG(_B11OUT1), _REG(_B11CTRL), _TIME(_BUCK11)),
	BUCK_DESC("BUCK12", _BUCK(12), &_buck_ops(), _BUCK(_MIN1),
	_BUCK(_STEP1), _REG(_B12OUT), _REG(_B12CTRL), _TIME(_BUCK12)),
	BUCK_DESC("BUCK13", _BUCK(13), &_buck_ops(), _BUCK(_MIN2),
	_BUCK(_STEP2), _REG(_B13OUT), _REG(_B13CTRL), _TIME(_BUCK13)),
/*	BUCK_DESC("BUCK14", _BUCK(14), &_buck_ops(), _BUCK(_MIN1),
	_BUCK(_STEP1), _REG(_B14OUT), _REG(_B14CTRL), _TIME(_BUCK14)),
	BUCK_DESC("BB", S2MPS18_BB1, &_buck_ops(), _BUCK(_MIN1),
	_BUCK(_STEP2), _REG(_BB1OUT), _REG(_BB1CTRL),
	_TIME(_BB)),
*/
};
#ifdef CONFIG_OF
static int s2mps18_pmic_dt_parse_pdata(struct s2mps18_dev *iodev,
					struct s2mps18_platform_data *pdata)
{
	struct device_node *pmic_np, *regulators_np, *reg_np;
	struct s2mps18_regulator_data *rdata;
	unsigned int i;
	int ret;
	u32 val;
	pdata->smpl_warn_vth = 0;
	pdata->smpl_warn_hys = 0;
	pdata->cpu_ocp_warn_reset = 1;
	pdata->gpu_ocp_warn_reset = 1;
	pdata->cpu_ocp_warn_lv = 0;
	pdata->gpu_ocp_warn_lv = 0;

	pmic_np = iodev->dev->of_node;
	if (!pmic_np) {
		dev_err(iodev->dev, "could not find pmic sub-node\n");
		return -ENODEV;
	}
	/* get 1 gpio values */
	if (of_gpio_count(pmic_np) < 1) {
		dev_err(iodev->dev, "could not find pmic gpios\n");
		return -EINVAL;
	}
	pdata->smpl_warn = of_get_gpio(pmic_np, 0);

	ret = of_property_read_u32(pmic_np, "g3d_en", &val);
	if (ret)
		return -EINVAL;
	pdata->g3d_en = !!val;

	ret = of_property_read_u32(pmic_np, "smpl_warn_en", &val);
	if (ret)
		return -EINVAL;
	pdata->smpl_warn_en = !!val;

	ret = of_property_read_u32(pmic_np, "smpl_warn_vth", &val);
	if (ret)
		return -EINVAL;
	pdata->smpl_warn_vth = val;

	ret = of_property_read_u32(pmic_np, "smpl_warn_hys", &val);
	if (ret)
		return -EINVAL;
	pdata->smpl_warn_hys = val;

	ret = of_property_read_u32(pmic_np, "cpu_ocp_warn_en", &val);
	if (ret)
		return -EINVAL;
	pdata->cpu_ocp_warn_en = !!val;

	ret = of_property_read_u32(pmic_np, "cpu_ocp_warn_reset", &val);
	if (ret)
		return -EINVAL;
	pdata->cpu_ocp_warn_reset = val;

	ret = of_property_read_u32(pmic_np, "cpu_ocp_warn_lv", &val);
	if (ret)
		return -EINVAL;
	pdata->cpu_ocp_warn_lv = val;

	ret = of_property_read_u32(pmic_np, "gpu_ocp_warn_en", &val);
	if (ret)
		return -EINVAL;
	pdata->gpu_ocp_warn_en = !!val;

	ret = of_property_read_u32(pmic_np, "gpu_ocp_warn_reset", &val);
	if (ret)
		return -EINVAL;
	pdata->gpu_ocp_warn_reset = val;

	ret = of_property_read_u32(pmic_np, "gpu_ocp_warn_lv", &val);
	if (ret)
		return -EINVAL;
	pdata->gpu_ocp_warn_lv = val;

	pdata->adc_mode = 0;
	ret = of_property_read_u32(pmic_np, "adc_mode", &val);

	pdata->adc_mode = val;

	pdata->adc_sync_mode = 0;
	ret = of_property_read_u32(pmic_np, "adc_sync_mode", &val);

	pdata->adc_sync_mode = val;

	regulators_np = of_find_node_by_name(pmic_np, "regulators");
	if (!regulators_np) {
		dev_err(iodev->dev, "could not find regulators sub-node\n");
		return -EINVAL;
	}

	/* count the number of regulators to be supported in pmic */
	pdata->num_regulators = 0;
	for_each_child_of_node(regulators_np, reg_np) {
		pdata->num_regulators++;
	}

	rdata = devm_kzalloc(iodev->dev, sizeof(*rdata) *
				pdata->num_regulators, GFP_KERNEL);
	if (!rdata) {
		dev_err(iodev->dev,
			"could not allocate memory for regulator data\n");
		return -ENOMEM;
	}

	pdata->regulators = rdata;
	for_each_child_of_node(regulators_np, reg_np) {
		for (i = 0; i < ARRAY_SIZE(regulators); i++)
			if (!of_node_cmp(reg_np->name,
					regulators[i].name))
				break;

		if (i == ARRAY_SIZE(regulators)) {
			dev_warn(iodev->dev,
			"don't know how to configure regulator %s\n",
			reg_np->name);
			continue;
		}

		rdata->id = i;
		rdata->initdata = of_get_regulator_init_data(
						iodev->dev, reg_np,
						&regulators[i]);
		rdata->reg_node = reg_np;
		rdata++;
	}

	return 0;
}
#else
static int s2mps18_pmic_dt_parse_pdata(struct s2mps18_pmic_dev *iodev,
					struct s2mps18_platform_data *pdata)
{
	return 0;
}
#endif /* CONFIG_OF */

#ifdef CONFIG_DEBUG_FS
static ssize_t s2mps18_i2caddr_read(struct file *file, char __user *user_buf,
					size_t count, loff_t *ppos)
{
	char buf[10];
	ssize_t ret;

	ret = snprintf(buf, sizeof(buf), "0x%x\n", i2caddr);
	if (ret < 0)
		return ret;

	return simple_read_from_buffer(user_buf, count, ppos, buf, ret);
}

static ssize_t s2mps18_i2caddr_write(struct file *file, const char __user *user_buf,
					size_t count, loff_t *ppos)
{
	char buf[10];
	ssize_t len;
	u8 val;

	len = simple_write_to_buffer(buf, sizeof(buf) - 1, ppos, user_buf, count);
	if (len < 0)
		return len;

	buf[len] = '\0';

	if (!kstrtou8(buf, 0, &val))
		i2caddr = val;

	return len;
}

static ssize_t s2mps18_i2cdata_read(struct file *file, char __user *user_buf,
					size_t count, loff_t *ppos)
{
	char buf[10];
	ssize_t ret;

	ret = s2mps18_read_reg(dbgi2c, i2caddr, &i2cdata);
	if (ret)
		return ret;

	ret = snprintf(buf, sizeof(buf), "0x%x\n", i2cdata);
	if (ret < 0)
		return ret;

	return simple_read_from_buffer(user_buf, count, ppos, buf, ret);
}

static ssize_t s2mps18_i2cdata_write(struct file *file, const char __user *user_buf,
					size_t count, loff_t *ppos)
{
	char buf[10];
	ssize_t len, ret;
	u8 val;

	len = simple_write_to_buffer(buf, sizeof(buf) - 1, ppos, user_buf, count);
	if (len < 0)
		return len;

	buf[len] = '\0';

	if (!kstrtou8(buf, 0, &val)) {
		ret = s2mps18_write_reg(dbgi2c, i2caddr, val);
		if (ret < 0)
			return ret;
	}

	return len;
}

static const struct file_operations s2mps18_i2caddr_fops = {
	.open = simple_open,
	.read = s2mps18_i2caddr_read,
	.write = s2mps18_i2caddr_write,
	.llseek = default_llseek,
};
static const struct file_operations s2mps18_i2cdata_fops = {
	.open = simple_open,
	.read = s2mps18_i2cdata_read,
	.write = s2mps18_i2cdata_write,
	.llseek = default_llseek,
};
#endif

#ifdef CONFIG_EXYNOS_OCP
void get_s2mps18_i2c(struct i2c_client **i2c)
{
	*i2c = static_info->i2c;
}
#endif

static irqreturn_t s2mps18_b2_ocp_irq(int irq, void *data)
{
	struct s2mps18_info *s2mps18 = data;
	mutex_lock(&s2mps18->lock);
	pr_info("%s:irq(%d)\n", __func__, irq);
	mutex_unlock(&s2mps18->lock);
	return IRQ_HANDLED;
}

static irqreturn_t s2mps18_b3_ocp_irq(int irq, void *data)
{
	struct s2mps18_info *s2mps18 = data;
	mutex_lock(&s2mps18->lock);
	pr_info("%s:irq(%d)\n", __func__, irq);
	mutex_unlock(&s2mps18->lock);
	return IRQ_HANDLED;
}

static irqreturn_t s2mps18_th120C_irq(int irq, void *data)
{
	struct s2mps18_info *s2mps18 = data;

	mutex_lock(&s2mps18->lock);
	pr_info("%s: PMIC thermal 120C irq(%d)\n", __func__, irq);
	panic("PMIC thermal 120C Interrupt occur !!\n");
	mutex_unlock(&s2mps18->lock);
	return IRQ_HANDLED;
}

static int s2mps18_pmic_probe(struct platform_device *pdev)
{
	struct s2mps18_dev *iodev = dev_get_drvdata(pdev->dev.parent);
	struct s2mps18_platform_data *pdata = iodev->pdata;
	struct regulator_config config = { };
	struct s2mps18_info *s2mps18;
	int irq_base;
	int i, ret, val = 0;

	if (iodev->dev->of_node) {
		ret = s2mps18_pmic_dt_parse_pdata(iodev, pdata);
		if (ret)
			return ret;
	}

	if (!pdata) {
		dev_err(pdev->dev.parent, "Platform data not supplied\n");
		return -ENODEV;
	}

	s2mps18 = devm_kzalloc(&pdev->dev, sizeof(struct s2mps18_info),
				GFP_KERNEL);
	if (!s2mps18)
		return -ENOMEM;

	irq_base = pdata->irq_base;
	if (!irq_base) {
		dev_err(&pdev->dev, "Failed to get irq base %d\n", irq_base);
		return -ENODEV;
	}

	s2mps18->iodev = iodev;
	s2mps18->i2c = iodev->pmic;
	s2mps18->debug_i2c = iodev->debug_i2c;
	s2mps18->b2_ocp_irq = irq_base + S2MPS18_PMIC_IRQ_OCPB2_INT5;
	s2mps18->b3_ocp_irq = irq_base + S2MPS18_PMIC_IRQ_OCPB3_INT5;
	s2mps18->th120C_irq = irq_base + S2MPS18_PMIC_IRQ_120C_INT3;

	mutex_init(&s2mps18->lock);

	s2mps18->g3d_en = pdata->g3d_en;
	static_info = s2mps18;

	platform_set_drvdata(pdev, s2mps18);

	for (i = 0; i < pdata->num_regulators; i++) {
		int id = pdata->regulators[i].id;
		config.dev = &pdev->dev;
		config.init_data = pdata->regulators[i].initdata;
		config.driver_data = s2mps18;
		config.of_node = pdata->regulators[i].reg_node;
		s2mps18->opmode[id] =
			regulators[id].enable_mask;

		s2mps18->rdev[i] = regulator_register(
				&regulators[id], &config);
		if (IS_ERR(s2mps18->rdev[i])) {
			ret = PTR_ERR(s2mps18->rdev[i]);
			dev_err(&pdev->dev, "regulator init failed for %d\n",
				i);
			s2mps18->rdev[i] = NULL;
			goto err;
		}
	}

	s2mps18->num_regulators = pdata->num_regulators;

	if (pdata->g3d_en) {
		/* for buck6 gpio control, disable i2c control */
		ret = s2mps18_update_reg(s2mps18->i2c, S2MPS18_PMIC_REG_B6CTRL,
				0x80, 0xC0);
		if (ret) {
			dev_err(&pdev->dev, "buck6 gpio control error\n");
			goto err;
		}
		ret = s2mps18_update_reg(s2mps18->i2c, S2MPS18_PMIC_REG_L7CTRL,
				0x40, 0xC0);
		if (ret) {
			dev_err(&pdev->dev, "regulator sync enable error\n");
				goto err;
		}
	}

	if (pdata->smpl_warn_en) {
		ret = s2mps18_update_reg(s2mps18->i2c, S2MPS18_PMIC_REG_CTRL2,
						pdata->smpl_warn_vth, 0xe0);
		if (ret) {
			dev_err(&pdev->dev, "set smpl_warn configuration i2c write error\n");
			goto err;
		}
		pr_info("%s: smpl_warn vth is 0x%x\n", __func__,
							pdata->smpl_warn_vth);

		ret = s2mps18_update_reg(s2mps18->i2c, S2MPS18_PMIC_REG_CTRL2,
						pdata->smpl_warn_hys, 0x18);
		if (ret) {
			dev_err(&pdev->dev, "set smpl_warn configuration i2c write error\n");
			goto err;
		}
		pr_info("%s: smpl_warn hysteresis is 0x%x\n", __func__,
							pdata->smpl_warn_hys);
	}

	if (pdata->cpu_ocp_warn_en) {
		val = (pdata->cpu_ocp_warn_en << S2MPS18_OCP_WARN_EN_SHIFT) |
			(pdata->cpu_ocp_warn_reset << S2MPS18_OCP_WARN_RESET_SHIFT) |
			(pdata->cpu_ocp_warn_lv << S2MPS18_OCP_WARN_LEVEL_SHIFT);

		ret = s2mps18_write_reg(s2mps18->i2c, S2MPS18_PMIC_REG_OCP_WARN1, val);

		if (ret) {
			dev_err(&pdev->dev,"%s : i2c write for cpu ocp warn configuration caused error\n",
				__func__);
			goto err;
		}

		pr_info("%s : value for cpu ocp warn register is 0x%x\n", __func__, val);
	}

	val = 0;

	if (pdata->gpu_ocp_warn_en) {
		val = (pdata->gpu_ocp_warn_en << S2MPS18_OCP_WARN_EN_SHIFT) |
			(pdata->gpu_ocp_warn_reset << S2MPS18_OCP_WARN_RESET_SHIFT) |
			(pdata->gpu_ocp_warn_lv << S2MPS18_OCP_WARN_LEVEL_SHIFT);

		ret = s2mps18_write_reg(s2mps18->i2c, S2MPS18_PMIC_REG_OCP_WARN2, val);

		if (ret) {
			dev_err(&pdev->dev,"%s : i2c write for gpu ocp warn configuration caused error\n",
				__func__);
			goto err;
		}

		pr_info("%s : value for gpu ocp warn register is 0x%x\n", __func__, val);
	}

	s2mps18_update_reg(s2mps18->i2c, S2MPS18_PMIC_REG_RTCBUF, 0x4, 0x4);

	/* set BUCK1~4 output2 as 0.5V */
	s2mps18_write_reg(s2mps18->i2c, S2MPS18_PMIC_REG_B1OUT2, 0x20);
	s2mps18_write_reg(s2mps18->i2c, S2MPS18_PMIC_REG_B2OUT2, 0x20);
	s2mps18_write_reg(s2mps18->i2c, S2MPS18_PMIC_REG_B3OUT2, 0x20);
	s2mps18_write_reg(s2mps18->i2c, S2MPS18_PMIC_REG_B4OUT2, 0x20);

	/* set BUCK11 output2 as 1.1V --> 0.95V */
	s2mps18_write_reg(s2mps18->i2c, S2MPS18_PMIC_REG_B11OUT2, 0x68);

	/* previous w/a from s2mps17 */
	/* SELMIF : LDO4,5,8,11,12 */
	//s2mps18_write_reg(s2mps18->i2c, S2MPS18_PMIC_REG_SELMIF1, 0xD6);

	/* PWREN_SEL Register Setting - not set yet */
//	s2mps18_write_reg(s2mps18->i2c, S2MPS18_PMIC_REG_PWREN_SEL1, 0xff);
	s2mps18_write_reg(s2mps18->i2c, S2MPS18_PMIC_REG_PWREN_SEL2, 0xff);

	ret = devm_request_threaded_irq(&pdev->dev, s2mps18->b2_ocp_irq, NULL,
			s2mps18_b2_ocp_irq, 0, "B2-OCP-irq", s2mps18);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to request B2 OCP IRQ: %d: %d\n",
			s2mps18->b2_ocp_irq, ret);
	}
	ret = devm_request_threaded_irq(&pdev->dev, s2mps18->b3_ocp_irq, NULL,
			s2mps18_b3_ocp_irq, 0, "B3-OCP-irq", s2mps18);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to request B3 OCP IRQ: %d: %d\n",
			s2mps18->b3_ocp_irq, ret);
	}

	ret = devm_request_threaded_irq(&pdev->dev, s2mps18->th120C_irq, NULL,
			s2mps18_th120C_irq, 0, "th120C-irq", s2mps18);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to request th120C IRQ: %d: %d\n",
			s2mps18->th120C_irq, ret);
	}

#ifdef CONFIG_DEBUG_FS
	dbgi2c = s2mps18->i2c;
	s2mps18_root = debugfs_create_dir("s2mps18-regs", NULL);
	s2mps18_i2caddr = debugfs_create_file("i2caddr", 0644, s2mps18_root, NULL, &s2mps18_i2caddr_fops);
	s2mps18_i2cdata = debugfs_create_file("i2cdata", 0644, s2mps18_root, NULL, &s2mps18_i2cdata_fops);
#endif

	iodev->adc_mode = pdata->adc_mode;
	iodev->adc_sync_mode = pdata->adc_sync_mode;
	if (iodev->adc_mode > 0)
		s2mps18_powermeter_init(iodev);

	return 0;
err:
	for (i = 0; i < S2MPS18_REGULATOR_MAX; i++)
		regulator_unregister(s2mps18->rdev[i]);

	return ret;
}

static int s2mps18_pmic_remove(struct platform_device *pdev)
{
	struct s2mps18_info *s2mps18 = platform_get_drvdata(pdev);
	int i;

#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(s2mps18_i2cdata);
	debugfs_remove_recursive(s2mps18_i2caddr);
	debugfs_remove_recursive(s2mps18_root);
#endif

	for (i = 0; i < S2MPS18_REGULATOR_MAX; i++)
		regulator_unregister(s2mps18->rdev[i]);

	s2mps18_powermeter_deinit(s2mps18->iodev);
	return 0;
}

#if defined(CONFIG_PM)
static int s2mps18_pmic_suspend(struct device *dev)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct s2mps18_info *s2mps18 = platform_get_drvdata(pdev);
	pr_info("%s adc_mode : %d\n", __func__, s2mps18->iodev->adc_mode);
	if (s2mps18->iodev->adc_mode > 0) {
		s2mps18_read_reg(s2mps18->i2c, S2MPS18_REG_ADC_CTRL3, &s2mps18->adc_en_val);
		if (s2mps18->adc_en_val & 0x80)
			s2mps18_update_reg(s2mps18->i2c, S2MPS18_REG_ADC_CTRL3, 0, ADC_EN_MASK);
	}
	return 0;
}

static int s2mps18_pmic_resume(struct device *dev)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct s2mps18_info *s2mps18 = platform_get_drvdata(pdev);
	pr_info("%s adc_mode : %d\n", __func__, s2mps18->iodev->adc_mode);

	if (s2mps18->iodev->adc_mode > 0)
		s2mps18_update_reg(s2mps18->i2c, S2MPS18_REG_ADC_CTRL3,
				s2mps18->adc_en_val & 0x80, ADC_EN_MASK);
	return 0;
}
#else
#define s2mps18_pmic_suspend	NULL
#define s2mps18_pmic_resume	NULL
#endif /* CONFIG_PM */

const struct dev_pm_ops s2mps18_pmic_pm = {
	.suspend = s2mps18_pmic_suspend,
	.resume = s2mps18_pmic_resume,
};

static const struct platform_device_id s2mps18_pmic_id[] = {
	{ "s2mps18-regulator", 0},
	{ },
};
MODULE_DEVICE_TABLE(platform, s2mps18_pmic_id);

static struct platform_driver s2mps18_pmic_driver = {
	.driver = {
		.name = "s2mps18-regulator",
		.owner = THIS_MODULE,
#if defined(CONFIG_PM)
		.pm = &s2mps18_pmic_pm,
#endif
		.suppress_bind_attrs = true,
	},
	.probe = s2mps18_pmic_probe,
	.remove = s2mps18_pmic_remove,
	.id_table = s2mps18_pmic_id,
};

static int __init s2mps18_pmic_init(void)
{
	return platform_driver_register(&s2mps18_pmic_driver);
}
subsys_initcall(s2mps18_pmic_init);

static void __exit s2mps18_pmic_exit(void)
{
	platform_driver_unregister(&s2mps18_pmic_driver);
}
module_exit(s2mps18_pmic_exit);

/* Module information */
MODULE_AUTHOR("Sangbeom Kim <sbkim73@samsung.com>");
MODULE_DESCRIPTION("SAMSUNG S2MPS18 Regulator Driver");
MODULE_LICENSE("GPL");
