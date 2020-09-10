/*
 * s2mps19.c
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
#include <linux/mfd/samsung/s2mps19.h>
#include <linux/mfd/samsung/s2mps19-regulator.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/debug-snapshot.h>
#include <linux/debugfs.h>
#include <linux/interrupt.h>

#include "../soc/samsung/cal-if/cmucal.h"

#ifdef CONFIG_SEC_PM_DEBUG
#include <linux/cpufreq.h>
#endif
#ifdef CONFIG_SEC_PM_BIGDATA
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/sec_hqm_device.h>
#endif /* CONFIG_SEC_PM_BIGDATA */
#ifdef CONFIG_SEC_PM
#include <linux/sec_class.h>
#include <linux/soc/samsung/exynos-soc.h>

#define STATUS1_ACOKB	BIT(2)

struct device *ap_pmic_dev;
#endif /* CONFIG_SEC_PM */

static struct s2mps19_info *s2mps19_static_info;
static struct regulator_desc regulators[S2MPS19_REGULATOR_MAX];
int s2mps19_buck_ocp_cnt[12]; /* BUCK 1~12 OCP count */
int s2mps19_bb_ocp_cnt;		/* BUCK-BOOST OCP count */
int s2mps19_temp_cnt[2]; /* 0 : 120 degree , 1 : 140 degree */

#ifdef CONFIG_SEC_PM_BIGDATA
static int hqm_bocp_cnt[12];
#endif

#ifdef CONFIG_DEBUG_FS
static u8 i2caddr = 0;
static u8 i2cdata = 0;
static struct i2c_client *dbgi2c = NULL;
static struct dentry *s2mps19_root = NULL;
static struct dentry *s2mps19_i2caddr = NULL;
static struct dentry *s2mps19_i2cdata = NULL;
#endif

struct s2mps19_info {
	struct regulator_dev *rdev[S2MPS19_REGULATOR_MAX];
	unsigned int opmode[S2MPS19_REGULATOR_MAX];
	int num_regulators;
	struct s2mps19_dev *iodev;
	struct mutex lock;
	struct i2c_client *i2c;
	struct i2c_client *debug_i2c;
	u8 adc_en_val;
	int buck_ocp_irq[12];	/* BUCK OCP IRQ */
	int bb_ocp_irq;		/* BUCK-BOOST OCP IRQ */
	int temp_irq[2];	/* 0 : 120 degree, 1 : 140 degree */
#ifdef CONFIG_SEC_PM_BIGDATA
	struct delayed_work hqm_pmtp_work;
	struct delayed_work hqm_bocp_work;
#endif
};

static unsigned int s2mps19_of_map_mode(unsigned int val) {
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
	struct s2mps19_info *s2mps19 = rdev_get_drvdata(rdev);
	unsigned int val;
	int id = rdev_get_id(rdev);

	val = mode << S2MPS19_ENABLE_SHIFT;

	s2mps19->opmode[id] = val;
	return 0;
}

static int s2m_enable(struct regulator_dev *rdev)
{
	struct s2mps19_info *s2mps19 = rdev_get_drvdata(rdev);

	return s2mps19_update_reg(s2mps19->i2c, rdev->desc->enable_reg,
				  s2mps19->opmode[rdev_get_id(rdev)],
				  rdev->desc->enable_mask);
}

static int s2m_disable_regmap(struct regulator_dev *rdev)
{
	struct s2mps19_info *s2mps19 = rdev_get_drvdata(rdev);
	unsigned int val;

	if (rdev->desc->enable_is_inverted)
		val = rdev->desc->enable_mask;
	else
		val = 0;

	return s2mps19_update_reg(s2mps19->i2c, rdev->desc->enable_reg,
				  val, rdev->desc->enable_mask);
}

static int s2m_is_enabled_regmap(struct regulator_dev *rdev)
{
	struct s2mps19_info *s2mps19 = rdev_get_drvdata(rdev);
	int ret;
	u8 val;

	ret = s2mps19_read_reg(s2mps19->i2c,
				rdev->desc->enable_reg, &val);
	if (ret)
		return ret;

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
	struct s2mps19_info *s2mps19 = rdev_get_drvdata(rdev);
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
	case S2MPS19_BUCK1:
	case S2MPS19_BUCK4:
	case S2MPS19_BUCK8:
		ramp_shift = 6;
		break;
	case S2MPS19_BUCK2:
	case S2MPS19_BUCK6:
	case S2MPS19_BUCK9:
	case S2MPS19_BUCK10:
		ramp_shift = 4;
		break;
	case S2MPS19_BUCK3:
	case S2MPS19_BUCK5:
	case S2MPS19_BUCK11:
		ramp_shift = 2;
		break;
	case S2MPS19_BUCK7:
	case S2MPS19_BUCK12:
	case S2MPS19_BB:
		ramp_shift = 0;
		break;
	default:
		return -EINVAL;
	}

	switch(reg_id) {
	case S2MPS19_BUCK1:
	case S2MPS19_BUCK2:
	case S2MPS19_BUCK3:
	case S2MPS19_BUCK4:
	case S2MPS19_BUCK5:
	case S2MPS19_BUCK6:
	case S2MPS19_BUCK7:
		ramp_addr = S2MPS19_PMIC_REG_BUCKRAMP;
		break;
	case S2MPS19_BUCK8:
	case S2MPS19_BUCK9:
	case S2MPS19_BUCK10:
	case S2MPS19_BUCK11:
	case S2MPS19_BUCK12:
	case S2MPS19_BB:
		ramp_addr = S2MPS19_PMIC_REG_BUCKRAMP2;
		break;
	default:
		return -EINVAL;
	}

	return s2mps19_update_reg(s2mps19->i2c, ramp_addr,
		ramp_value << ramp_shift, ramp_mask << ramp_shift);
}

static int s2m_get_voltage_sel_regmap(struct regulator_dev *rdev)
{
	struct s2mps19_info *s2mps19 = rdev_get_drvdata(rdev);
	int ret;
	u8 val;

	ret = s2mps19_read_reg(s2mps19->i2c, rdev->desc->vsel_reg, &val);
	if (ret)
		return ret;

	val &= rdev->desc->vsel_mask;

	return val;
}

static int s2m_set_voltage_sel_regmap(struct regulator_dev *rdev, unsigned sel)
{
	struct s2mps19_info *s2mps19 = rdev_get_drvdata(rdev);
	int reg_id = rdev_get_id(rdev);
	int ret;
	char name[16];

	/* voltage information logging to snapshot feature */
	snprintf(name, sizeof(name), "LDO%d", (reg_id - S2MPS19_LDO1) + 1);
	ret = s2mps19_update_reg(s2mps19->i2c, rdev->desc->vsel_reg,
				  sel, rdev->desc->vsel_mask);
	if (ret < 0)
		goto out;

	if (rdev->desc->apply_bit)
		ret = s2mps19_update_reg(s2mps19->i2c, rdev->desc->apply_reg,
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
	struct s2mps19_info *s2mps19 = rdev_get_drvdata(rdev);

	ret = s2mps19_write_reg(s2mps19->i2c, rdev->desc->vsel_reg, sel);
	if (ret < 0)
		goto i2c_out;

	if (rdev->desc->apply_bit)
		ret = s2mps19_update_reg(s2mps19->i2c, rdev->desc->apply_reg,
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
	else
		return DIV_ROUND_UP(old_volt - new_volt, ramp_delay);

	return 0;
}

static struct regulator_ops s2mps19_ldo_ops = {
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

static struct regulator_ops s2mps19_buck_ops = {
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

static struct regulator_ops s2mps19_bb_ops = {
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

#define _BUCK(macro)	S2MPS19_BUCK##macro
#define _buck_ops(num)	s2mps19_buck_ops##num
#define _LDO(macro)		S2MPS19_LDO##macro
#define _ldo_ops(num)	s2mps19_ldo_ops##num
#define _BB(macro)		S2MPS19_BB##macro
#define _bb_ops(num)	s2mps19_bb_ops##num

#define _REG(ctrl)		S2MPS19_PMIC_REG##ctrl
#define _TIME(macro)	S2MPS19_ENABLE_TIME##macro

#define _LDO_MIN(group)		S2MPS19_LDO_MIN##group
#define _LDO_STEP(group)	S2MPS19_LDO_STEP##group
#define _BUCK_MIN(group)	S2MPS19_BUCK_MIN##group
#define _BUCK_STEP(group)	S2MPS19_BUCK_STEP##group
#define _BB_MIN(group)		S2MPS19_BB_MIN##group
#define _BB_STEP(group)		S2MPS19_BB_STEP##group

#define BUCK_DESC(_name, _id, _ops, g, v, e, t)	{	\
	.name		= _name,				\
	.id			= _id,					\
	.ops		= _ops,					\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= _BUCK_MIN(g),					\
	.uV_step	= _BUCK_STEP(g),					\
	.n_voltages	= S2MPS19_BUCK_N_VOLTAGES,		\
	.vsel_reg	= v,					\
	.vsel_mask	= S2MPS19_BUCK_VSEL_MASK,		\
	.enable_reg	= e,					\
	.enable_mask	= S2MPS19_ENABLE_MASK,			\
	.enable_time	= t,					\
	.of_map_mode	= s2mps19_of_map_mode			\
}

#define LDO_DESC(_name, _id, _ops, g, v, e, t)	{	\
	.name		= _name,				\
	.id			= _id,					\
	.ops		= _ops,					\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= _LDO_MIN(g),					\
	.uV_step	= _LDO_STEP(g),					\
	.n_voltages	= S2MPS19_LDO_N_VOLTAGES,		\
	.vsel_reg	= v,					\
	.vsel_mask	= S2MPS19_LDO_VSEL_MASK,		\
	.enable_reg	= e,					\
	.enable_mask	= S2MPS19_ENABLE_MASK,			\
	.enable_time	= t,					\
	.of_map_mode	= s2mps19_of_map_mode			\
}

#define BB_DESC(_name, _id, _ops, g, v, e, t)	{	\
	.name		= _name,				\
	.id			= _id,					\
	.ops		= _ops,					\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= _BB_MIN(),				\
	.uV_step	= _BB_STEP(),				\
	.n_voltages	= S2MPS19_BB_N_VOLTAGES,		\
	.vsel_reg	= v,					\
	.vsel_mask	= S2MPS19_BB_VSEL_MASK,		\
	.enable_reg	= e,					\
	.enable_mask	= S2MPS19_ENABLE_MASK,			\
	.enable_time	= t,					\
	.of_map_mode 	= s2mps19_of_map_mode			\
}

static struct regulator_desc regulators[S2MPS19_REGULATOR_MAX] = {
	/* name, id, ops, group, vsel_reg, enable_reg, ramp_delay */
	LDO_DESC("LDO1M", _LDO(1), &_ldo_ops(), 3,
	_REG(_L1CTRL), _REG(_L1CTRL), _TIME(_LDO)),
	LDO_DESC("LDO2M", _LDO(2), &_ldo_ops(), 2,
	_REG(_L2CTRL), _REG(_L2CTRL), _TIME(_LDO)),
	LDO_DESC("LDO3M", _LDO(3), &_ldo_ops(), 3,
	_REG(_L3CTRL), _REG(_L3CTRL), _TIME(_LDO)),
	LDO_DESC("LDO4M", _LDO(4), &_ldo_ops(), 1,
	_REG(_L4CTRL), _REG(_L4CTRL), _TIME(_LDO)),
	LDO_DESC("LDO5M", _LDO(5), &_ldo_ops(), 5,
	_REG(_L5CTRL), _REG(_L5CTRL), _TIME(_LDO)),
	LDO_DESC("LDO6M", _LDO(6), &_ldo_ops(), 5,
	_REG(_L6CTRL), _REG(_L6CTRL), _TIME(_LDO)),
	LDO_DESC("LDO7M", _LDO(7), &_ldo_ops(), 5,
	_REG(_L7CTRL), _REG(_L7CTRL), _TIME(_LDO)),
	LDO_DESC("LDO8M", _LDO(8), &_ldo_ops(), 5,
	_REG(_L8CTRL), _REG(_L8CTRL), _TIME(_LDO)),
	LDO_DESC("LDO9M", _LDO(9), &_ldo_ops(), 3,
	_REG(_L9CTRL), _REG(_L9CTRL), _TIME(_LDO)),
	LDO_DESC("LDO10M", _LDO(10), &_ldo_ops(), 3,
	_REG(_L10CTRL), _REG(_L10CTRL), _TIME(_LDO)),
	LDO_DESC("LDO11M", _LDO(11), &_ldo_ops(), 1,
	_REG(_L11CTRL), _REG(_L11CTRL), _TIME(_LDO)),
	LDO_DESC("LDO12M", _LDO(12), &_ldo_ops(), 2,
	_REG(_L12CTRL), _REG(_L12CTRL), _TIME(_LDO)),
/*	LDO_DESC("LDO13M", _LDO(13), &_ldo_ops(), 2,
	_REG(_L13CTRL), _REG(_L13CTRL), _TIME(_LDO)),
	LDO_DESC("LDO14M", _LDO(14), &_ldo_ops(), 2,
	_REG(_L14CTRL), _REG(_L14CTRL), _TIME(_LDO)),
*/	LDO_DESC("LDO15M", _LDO(15), &_ldo_ops(), 2,
	_REG(_L15CTRL), _REG(_L15CTRL), _TIME(_LDO)),
	LDO_DESC("LDO16M", _LDO(16), &_ldo_ops(), 2,
	_REG(_L16CTRL), _REG(_L16CTRL), _TIME(_LDO)),
	LDO_DESC("LDO17M", _LDO(17), &_ldo_ops(), 1,
	_REG(_L17CTRL), _REG(_L17CTRL), _TIME(_LDO)),
	LDO_DESC("LDO18M", _LDO(18), &_ldo_ops(), 3,
	_REG(_L18CTRL), _REG(_L18CTRL), _TIME(_LDO)),
	LDO_DESC("LDO19M", _LDO(19), &_ldo_ops(), 1,
	_REG(_L19CTRL), _REG(_L19CTRL), _TIME(_LDO)),
	LDO_DESC("LDO20M", _LDO(20), &_ldo_ops(), 2,
	_REG(_L20CTRL), _REG(_L20CTRL), _TIME(_LDO)),
	LDO_DESC("LDO21M", _LDO(21), &_ldo_ops(), 1,
	_REG(_L21CTRL), _REG(_L21CTRL), _TIME(_LDO)),
	LDO_DESC("LDO22M", _LDO(22), &_ldo_ops(), 1,
	_REG(_L22CTRL), _REG(_L22CTRL), _TIME(_LDO)),
	LDO_DESC("LDO23M", _LDO(23), &_ldo_ops(), 3,
	_REG(_L23CTRL), _REG(_L23CTRL), _TIME(_LDO)),
	LDO_DESC("LDO24M", _LDO(24), &_ldo_ops(), 2,
	_REG(_L24CTRL), _REG(_L24CTRL), _TIME(_LDO)),
	LDO_DESC("LDO25M", _LDO(25), &_ldo_ops(), 1,
	_REG(_L25CTRL), _REG(_L25CTRL), _TIME(_LDO)),
	LDO_DESC("LDO26M", _LDO(26), &_ldo_ops(), 2,
	_REG(_L26CTRL), _REG(_L26CTRL), _TIME(_LDO)),
	LDO_DESC("LDO27M", _LDO(27), &_ldo_ops(), 2,
	_REG(_L27CTRL), _REG(_L27CTRL), _TIME(_LDO)),
	LDO_DESC("LDO28M", _LDO(28), &_ldo_ops(), 1,
	_REG(_L28CTRL), _REG(_L28CTRL), _TIME(_LDO)),
	LDO_DESC("LDO29M", _LDO(29), &_ldo_ops(), 2,
	_REG(_L29CTRL), _REG(_L29CTRL), _TIME(_LDO)),
	BUCK_DESC("BUCK1M", _BUCK(1), &_buck_ops(), 1,
	_REG(_B1M_OUT1), _REG(_B1M_CTRL), _TIME(_BUCK)),
	BUCK_DESC("BUCK2M", _BUCK(2), &_buck_ops(), 1,
	_REG(_B2M_OUT1), _REG(_B2M_CTRL), _TIME(_BUCK)),
	BUCK_DESC("BUCK3M", _BUCK(3), &_buck_ops(), 1,
	_REG(_B3M_OUT1), _REG(_B3M_CTRL), _TIME(_BUCK)),
	BUCK_DESC("BUCK4M", _BUCK(4), &_buck_ops(), 1,
	_REG(_B4M_OUT1), _REG(_B4M_CTRL), _TIME(_BUCK)),
	BUCK_DESC("BUCK5M", _BUCK(5), &_buck_ops(), 1,
	_REG(_B5M_OUT1), _REG(_B5M_CTRL), _TIME(_BUCK)),
	BUCK_DESC("BUCK6M", _BUCK(6), &_buck_ops(), 1,
	_REG(_B6M_OUT), _REG(_B6M_CTRL), _TIME(_BUCK)),
	BUCK_DESC("BUCK7M", _BUCK(7), &_buck_ops(), 1,
	_REG(_B7M_OUT), _REG(_B7M_CTRL), _TIME(_BUCK)),
	BUCK_DESC("BUCK8M", _BUCK(8), &_buck_ops(), 1,
	_REG(_B8M_OUT), _REG(_B8M_CTRL), _TIME(_BUCK)),
	BUCK_DESC("BUCK9M", _BUCK(9), &_buck_ops(), 1,
	_REG(_B9M_OUT), _REG(_B9M_CTRL), _TIME(_BUCK)),
	BUCK_DESC("BUCK10M", _BUCK(10), &_buck_ops(), 1,
	_REG(_B10M_OUT1), _REG(_B10M_CTRL), _TIME(_BUCK)),
	BUCK_DESC("BUCK11M", _BUCK(11), &_buck_ops(), 1,
	_REG(_B11M_OUT), _REG(_B11M_CTRL), _TIME(_BUCK)),
	BUCK_DESC("BUCK12M", _BUCK(12), &_buck_ops(), 2,
	_REG(_B12M_OUT), _REG(_B12M_CTRL), _TIME(_BUCK)),
	BB_DESC("BBM", _BB(), &_bb_ops(), 1,
	_REG(_BBM_OUT), _REG(_BBM_CTRL), _TIME(_BB)),
};
#ifdef CONFIG_OF
static int s2mps19_pmic_dt_parse_pdata(struct s2mps19_dev *iodev,
					struct s2mps19_platform_data *pdata)
{
	struct device_node *pmic_np, *regulators_np, *reg_np;
	struct s2mps19_regulator_data *rdata;
	unsigned int i;
	int ret;
	u32 val;
	pdata->smpl_warn_vth = 0;
	pdata->smpl_warn_hys = 0;

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

	ret = of_property_read_u32(pmic_np, "smpl_warn_en", &val);
	if (ret)
		return -EINVAL;
	pdata->smpl_warn_en = !!val;

	ret = of_property_read_u32(pmic_np, "smpl_warn_dev2", &val);
	if (ret)
		return -EINVAL;
	pdata->smpl_warn_dev2 = !!val;

#ifdef CONFIG_SEC_PM
	pdata->smpl_warn_en_by_evt =
		of_property_read_bool(pmic_np, "smpl_warn_en_by_evt");
#endif /* CONFIG_SEC_PM */

	ret = of_property_read_u32(pmic_np, "smpl_warn_vth", &val);
	if (ret)
		return -EINVAL;
	pdata->smpl_warn_vth = val;

	ret = of_property_read_u32(pmic_np, "smpl_warn_hys", &val);
	if (ret)
		return -EINVAL;
	pdata->smpl_warn_hys = val;

	ret = of_property_read_u32(pmic_np, "ocp_warn1_en", &val);
	if (ret)
		return -EINVAL;
	pdata->ocp_warn1_en = !!val;

	ret = of_property_read_u32(pmic_np, "ocp_warn1_cnt", &val);
	if (ret)
		return -EINVAL;
	pdata->ocp_warn1_cnt = val;

	ret = of_property_read_u32(pmic_np, "ocp_warn1_dvs_mask", &val);
	if (ret)
		return -EINVAL;
	pdata->ocp_warn1_dvs_mask = val;

	ret = of_property_read_u32(pmic_np, "ocp_warn1_lv", &val);
	if (ret)
		return -EINVAL;
	pdata->ocp_warn1_lv = val;

	ret = of_property_read_u32(pmic_np, "ocp_warn2_en", &val);
	if (ret)
		return -EINVAL;
	pdata->ocp_warn2_en = !!val;

	ret = of_property_read_u32(pmic_np, "ocp_warn2_cnt", &val);
	if (ret)
		return -EINVAL;
	pdata->ocp_warn2_cnt = val;

	ret = of_property_read_u32(pmic_np, "ocp_warn2_dvs_mask", &val);
	if (ret)
		return -EINVAL;
	pdata->ocp_warn2_dvs_mask = val;

	ret = of_property_read_u32(pmic_np, "ocp_warn2_lv", &val);
	if (ret)
		return -EINVAL;
	pdata->ocp_warn2_lv = val;

	pdata->adc_mode = 0;
	ret = of_property_read_u32(pmic_np, "adc_mode", &val);

	pdata->adc_mode = val;

	pdata->adc_sync_mode = 0;
	ret = of_property_read_u32(pmic_np, "adc_sync_mode", &val);

	pdata->adc_sync_mode = val;

	pdata->support_vdd_pll_1p7 = of_property_read_bool(pmic_np, "support_vdd_pll_1p7");

	dev_info(iodev->dev, "support_vdd_pll_1p7 : %d\n", pdata->support_vdd_pll_1p7);

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
static int s2mps19_pmic_dt_parse_pdata(struct s2mps19_pmic_dev *iodev,
					struct s2mps19_platform_data *pdata)
{
	return 0;
}
#endif /* CONFIG_OF */

#ifdef CONFIG_DEBUG_FS
static ssize_t s2mps19_i2caddr_read(struct file *file, char __user *user_buf,
					size_t count, loff_t *ppos)
{
	char buf[10];
	ssize_t ret;

	ret = snprintf(buf, sizeof(buf), "0x%x\n", i2caddr);
	if (ret < 0)
		return ret;

	return simple_read_from_buffer(user_buf, count, ppos, buf, ret);
}

static ssize_t s2mps19_i2caddr_write(struct file *file, const char __user *user_buf,
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

static ssize_t s2mps19_i2cdata_read(struct file *file, char __user *user_buf,
					size_t count, loff_t *ppos)
{
	char buf[10];
	ssize_t ret;

	ret = s2mps19_read_reg(dbgi2c, i2caddr, &i2cdata);
	if (ret)
		return ret;

	ret = snprintf(buf, sizeof(buf), "0x%x\n", i2cdata);
	if (ret < 0)
		return ret;

	return simple_read_from_buffer(user_buf, count, ppos, buf, ret);
}

static ssize_t s2mps19_i2cdata_write(struct file *file, const char __user *user_buf,
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
		ret = s2mps19_write_reg(dbgi2c, i2caddr, val);
		if (ret < 0)
			return ret;
	}

	return len;
}

static const struct file_operations s2mps19_i2caddr_fops = {
	.open = simple_open,
	.read = s2mps19_i2caddr_read,
	.write = s2mps19_i2caddr_write,
	.llseek = default_llseek,
};
static const struct file_operations s2mps19_i2cdata_fops = {
	.open = simple_open,
	.read = s2mps19_i2cdata_read,
	.write = s2mps19_i2cdata_write,
	.llseek = default_llseek,
};
#endif

#ifdef CONFIG_EXYNOS_OCP
void get_s2mps19_i2c(struct i2c_client **i2c)
{
	*i2c = s2mps19_static_info->i2c;
}
#endif

#ifdef CONFIG_SEC_PM_BIGDATA
void send_hqm_pmtp_work(struct work_struct *work)
{
	hqm_device_info hqm_info;
	char feature[HQM_FEATURE_LEN] ="PMTP";
	 
	memcpy(hqm_info.feature, feature, HQM_FEATURE_LEN);
	send_uevent_via_hqm_device(hqm_info);
}

void send_hqm_bocp_work(struct work_struct *work)
{
	hqm_device_info hqm_info;
	char feature[HQM_FEATURE_LEN] ="BOCP";

	memcpy(hqm_info.feature, feature, HQM_FEATURE_LEN);
	send_uevent_via_hqm_device(hqm_info);
}
#endif /* CONFIG_SEC_PM_BIGDATA */

static irqreturn_t s2mps19_buck_ocp_irq(int irq, void *data)
{
	struct s2mps19_info *s2mps19 = data;
	int i;

	mutex_lock(&s2mps19->lock);

	for (i = 0; i < 12; i++) {
		if (s2mps19_static_info->buck_ocp_irq[i] == irq) {
			s2mps19_buck_ocp_cnt[i]++;
#ifdef CONFIG_SEC_PM_BIGDATA
			hqm_bocp_cnt[i]++;
#endif
			pr_info("%s : BUCK[%d] OCP IRQ : %d, %d\n",
				__func__, i+1, s2mps19_buck_ocp_cnt[i], irq);
			break;
		}
	}

	mutex_unlock(&s2mps19->lock);

#ifdef CONFIG_SEC_PM_DEBUG
	pr_info("BUCK OCP%d: BIG: %u kHz, MID: %u kHz, LITTLE: %u kHz\n",
			irq - s2mps19_static_info->buck_ocp_irq[0] + 1,
			cpufreq_get(6), cpufreq_get(4), cpufreq_get(0));
#endif /* CONFIG_SEC_PM_DEBUG */

#ifdef CONFIG_SEC_PM_BIGDATA
	cancel_delayed_work(&s2mps19->hqm_bocp_work);
	schedule_delayed_work(&s2mps19->hqm_bocp_work, 5 * HZ);
#endif /* CONFIG_SEC_PM_BIGDATA */

	return IRQ_HANDLED;
}

static irqreturn_t s2mps19_bb_ocp_irq(int irq, void *data)
{
	struct s2mps19_info *s2mps19 = data;

	mutex_lock(&s2mps19->lock);

	s2mps19_bb_ocp_cnt++;
	pr_info("%s : BUCKBOST OCP IRQ : %d, %d\n", __func__, s2mps19_bb_ocp_cnt, irq);

	mutex_unlock(&s2mps19->lock);
	return IRQ_HANDLED;
}

static irqreturn_t s2mps19_temp_irq(int irq, void *data)
{
	struct s2mps19_info *s2mps19 = data;

	mutex_lock(&s2mps19->lock);

	if (s2mps19_static_info->temp_irq[0] == irq) {
		s2mps19_temp_cnt[0]++;
		pr_info("%s: PMIC thermal 120C IRQ : %d, %d\n",
			__func__, s2mps19_temp_cnt[0], irq);
	} else if (s2mps19_static_info->temp_irq[1] == irq) {
		s2mps19_temp_cnt[1]++;
		pr_info("%s: PMIC thermal 140C IRQ : %d, %d\n",
			__func__, s2mps19_temp_cnt[1], irq);
	}

	mutex_unlock(&s2mps19->lock);

#ifdef CONFIG_SEC_PM_BIGDATA
	cancel_delayed_work(&s2mps19->hqm_pmtp_work);
	schedule_delayed_work(&s2mps19->hqm_pmtp_work, 5 * HZ);
#endif /* CONFIG_SEC_PM_BIGDATA */

	return IRQ_HANDLED;
}

#ifdef CONFIG_SEC_PM_DEBUG
static u8 s2mps19_pwronsrc;
static u8 s2mps19_offsrc;

void s2mps19_get_pwr_onoffsrc(u8 *onsrc, u8 *offsrc)
{
	*onsrc = s2mps19_pwronsrc;
	*offsrc = s2mps19_offsrc;
}
#endif /* CONFIG_SEC_PM_DEBUG */

#ifdef CONFIG_SEC_PM
static ssize_t show_ap_pmic_th120C_count(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int cnt = s2mps19_temp_cnt[0];

	pr_info("%s: PMIC thermal 120C count: %d\n", __func__, s2mps19_temp_cnt[0]);
	s2mps19_temp_cnt[0] = 0;

	return sprintf(buf, "%d\n", cnt);
}

static ssize_t store_ap_pmic_th120C_count(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret, val;

	ret = kstrtoint(buf, 0, &val);
	if (ret < 0)
		return ret;

	s2mps19_temp_cnt[0] = val;

	return count;
}

static ssize_t show_ap_pmic_th140C_count(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int cnt = s2mps19_temp_cnt[1];

	pr_info("%s: PMIC thermal 140C count: %d\n", __func__, s2mps19_temp_cnt[1]);
	s2mps19_temp_cnt[1] = 0;

	return sprintf(buf, "%d\n", cnt);
}

static ssize_t store_ap_pmic_th140C_count(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret, val;

	ret = kstrtoint(buf, 0, &val);
	if (ret < 0)
		return ret;

	s2mps19_temp_cnt[1] = val;

	return count;
}

static ssize_t show_ap_pmic_buck_ocp_count(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int i, buf_offset = 0;

	for (i = 0; i < 12; i++)
		if (s2mps19_static_info->buck_ocp_irq[i])
			buf_offset += sprintf(buf + buf_offset, "B%d : %d\n",
					i+1, s2mps19_buck_ocp_cnt[i]);

	return buf_offset;
}

#ifdef CONFIG_SEC_PM_BIGDATA
static ssize_t hqm_bocp_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int i, buf_offset = 0;

	for (i = 0; i < 12; i++) {
		if (s2mps19_static_info->buck_ocp_irq[i]) {
			buf_offset += sprintf(buf + buf_offset, "\"B%d\":\"%d\",",
					i+1, hqm_bocp_cnt[i]);
			hqm_bocp_cnt[i] = 0;
		}
	}
	if(buf_offset > 0)
		buf[--buf_offset] = '\0';

	return buf_offset;
}
#endif /* CONFIG_SEC_PM_BIGDATA */

static ssize_t pmic_id_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	int pmic_id = s2mps19_static_info->iodev->pmic_rev;

	return sprintf(buf, "0x%02X\n", pmic_id);
}

static ssize_t chg_det_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	int ret, chg_det;
	u8 val;

	ret = s2mps19_read_reg(s2mps19_static_info->i2c, S2MPS19_PMIC_REG_STATUS1, &val);
	if(ret)
		chg_det = -1;
	else
		chg_det = !(val & STATUS1_ACOKB);

	pr_info("%s: ap pmic chg det: %d\n", __func__, chg_det);

	return sprintf(buf, "%d\n", chg_det);
}

static ssize_t show_manual_reset(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	bool enabled;
	u8 val;

	ret = s2mps19_read_reg(s2mps19_static_info->i2c, S2MPS19_PMIC_REG_CTRL1, &val);
	if (ret)
		return ret;

	enabled = !!(val & (1 << 4));

	pr_info("%s: %s[0x%02X]\n", __func__, enabled ? "enabled" :  "disabled",
			val);

	return sprintf(buf, "%s\n", enabled ? "enabled" :  "disabled");
}

static ssize_t store_manual_reset(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	bool enable;
	u8 val;

	ret = strtobool(buf, &enable);
	if (ret)
		return ret;

	ret = s2mps19_read_reg(s2mps19_static_info->i2c, S2MPS19_PMIC_REG_CTRL1, &val);
	if (ret)
		return ret;

	val &= ~(1 << 4);
	val |= enable << 4;

	ret = s2mps19_write_reg(s2mps19_static_info->i2c, S2MPS19_PMIC_REG_CTRL1, val);
	if (ret)
		return ret;

	pr_info("%s: %d [0x%02X]\n", __func__, enable, val);

	return count;
}

static DEVICE_ATTR(th120C_count, 0664, show_ap_pmic_th120C_count,
		store_ap_pmic_th120C_count);
static DEVICE_ATTR(th140C_count, 0664, show_ap_pmic_th140C_count,
		store_ap_pmic_th140C_count);
static DEVICE_ATTR(buck_ocp_count, 0444, show_ap_pmic_buck_ocp_count, NULL);
#ifdef CONFIG_SEC_PM_BIGDATA
static DEVICE_ATTR_RO(hqm_bocp_count);
#endif
static DEVICE_ATTR_RO(pmic_id);
static DEVICE_ATTR_RO(chg_det);
static DEVICE_ATTR(manual_reset, 0664, show_manual_reset, store_manual_reset);

static struct attribute *ap_pmic_attributes[] = {
	&dev_attr_th120C_count.attr,
	&dev_attr_th140C_count.attr,
	&dev_attr_buck_ocp_count.attr,
#ifdef CONFIG_SEC_PM_BIGDATA
	&dev_attr_hqm_bocp_count.attr,
#endif
	&dev_attr_pmic_id.attr,
	&dev_attr_chg_det.attr,
	&dev_attr_manual_reset.attr,
	NULL
};

static const struct attribute_group ap_pmic_attr_group = {
	.attrs = ap_pmic_attributes,
};
#endif /* CONFIG_SEC_PM */


void s2mps19_oi_function(struct s2mps19_dev *iodev)
{
	struct i2c_client *i2c = iodev->pmic;
	int i;
	u8 val;

	/* BUCK1~12 & buck-boost OI function enable */
	s2mps19_write_reg(i2c, S2MPS19_PMIC_REG_BUCK_OI_EN1, 0xff);
	s2mps19_update_reg(i2c, S2MPS19_PMIC_REG_BUCK_OI_EN2, 0x1f, 0x1f);

	/* BUCK1~12 & buck-boost OI power down disable */
	s2mps19_write_reg(i2c, S2MPS19_PMIC_REG_BUCK_OI_PD_EN1, 0x0);
	s2mps19_update_reg(i2c, S2MPS19_PMIC_REG_BUCK_OI_PD_EN2, 0x0, 0x1f);

	/* OI detection time window : 500us, OI comp. output count : 50 times */
	s2mps19_write_reg(i2c, S2MPS19_PMIC_REG_BUCK_OI_CTRL1, 0xcc);
	s2mps19_write_reg(i2c, S2MPS19_PMIC_REG_BUCK_OI_CTRL2, 0xcc);
	s2mps19_write_reg(i2c, S2MPS19_PMIC_REG_BUCK_OI_CTRL3, 0xcc);
	s2mps19_write_reg(i2c, S2MPS19_PMIC_REG_BUCK_OI_CTRL4, 0xcc);
	s2mps19_write_reg(i2c, S2MPS19_PMIC_REG_BUCK_OI_CTRL5, 0xcc);
	s2mps19_write_reg(i2c, S2MPS19_PMIC_REG_BUCK_OI_CTRL6, 0xcc);
	s2mps19_update_reg(i2c, S2MPS19_PMIC_REG_BUCK_OI_CTRL7, 0x0c, 0x0c);

	pr_info("%s\n", __func__);
	for (i = S2MPS19_PMIC_REG_BUCK_OI_EN1; i <= S2MPS19_PMIC_REG_BUCK_OI_CTRL7; i++) {
		s2mps19_read_reg(i2c, i, &val);
		pr_info("0x%x[0x%x], ", i, val);
	}
	pr_info("\n");
}


static int s2mps19_pmic_probe(struct platform_device *pdev)
{
	struct s2mps19_dev *iodev = dev_get_drvdata(pdev->dev.parent);
	struct s2mps19_platform_data *pdata = iodev->pdata;
	struct regulator_config config = { };
	struct s2mps19_info *s2mps19;
	int irq_base;
	int i, ret, ret1, ret2, ret3;
	u8 val, val1, val2, val3;

	if (iodev->dev->of_node) {
		ret = s2mps19_pmic_dt_parse_pdata(iodev, pdata);
		if (ret)
			return ret;
	}

	if (!pdata) {
		dev_err(pdev->dev.parent, "Platform data not supplied\n");
		return -ENODEV;
	}

	s2mps19 = devm_kzalloc(&pdev->dev, sizeof(struct s2mps19_info),
				GFP_KERNEL);
	if (!s2mps19)
		return -ENOMEM;

	irq_base = pdata->irq_base;
	if (!irq_base) {
		dev_err(&pdev->dev, "Failed to get irq base %d\n", irq_base);
		return -ENODEV;
	}

	s2mps19->iodev = iodev;
	s2mps19->i2c = iodev->pmic;
	s2mps19->debug_i2c = iodev->debug_i2c;

	mutex_init(&s2mps19->lock);

	s2mps19_static_info = s2mps19;

	platform_set_drvdata(pdev, s2mps19);

#ifdef CONFIG_SEC_PM_DEBUG
	ret = s2mps19_read_reg(s2mps19->i2c, S2MPS19_PMIC_REG_PWRONSRC,
			&s2mps19_pwronsrc);
	if (ret)
		dev_err(&pdev->dev, "failed to read PWRONSRC\n");

	ret = s2mps19_read_reg(s2mps19->i2c, S2MPS19_PMIC_REG_OFFSRC,
			&s2mps19_offsrc);
	if (ret)
		dev_err(&pdev->dev, "failed to read OFFSRC\n");

	/* Clear OFFSRC register */
	ret = s2mps19_write_reg(s2mps19->i2c, S2MPS19_PMIC_REG_OFFSRC, 0);
	if (ret)
		dev_err(&pdev->dev, "failed to write OFFSRC\n");
#endif /* CONFIG_SEC_PM_DEBUG */

	for (i = 0; i < pdata->num_regulators; i++) {
		int id = pdata->regulators[i].id;
		config.dev = &pdev->dev;
		config.init_data = pdata->regulators[i].initdata;
		config.driver_data = s2mps19;
		config.of_node = pdata->regulators[i].reg_node;
		s2mps19->opmode[id] =
			regulators[id].enable_mask;

		s2mps19->rdev[i] = regulator_register(
				&regulators[id], &config);
		if (IS_ERR(s2mps19->rdev[i])) {
			ret = PTR_ERR(s2mps19->rdev[i]);
			dev_err(&pdev->dev, "regulator init failed for %d\n",
				i);
			s2mps19->rdev[i] = NULL;
			goto err;
		}
	}

	s2mps19->num_regulators = pdata->num_regulators;

#ifdef CONFIG_SEC_PM_BIGDATA
	INIT_DELAYED_WORK(&s2mps19->hqm_pmtp_work, send_hqm_pmtp_work);
	INIT_DELAYED_WORK(&s2mps19->hqm_bocp_work, send_hqm_bocp_work);
#endif /* CONFIG_SEC_PM_BIGDATA */

	for (i = 0; i < 12; i++) {
		s2mps19->buck_ocp_irq[i] = irq_base + S2MPS19_PMIC_IRQ_OCP_B1M_INT4 + i;

		ret = devm_request_threaded_irq(&pdev->dev, s2mps19->buck_ocp_irq[i], NULL,
			s2mps19_buck_ocp_irq, 0, "BUCK_OCP_IRQ", s2mps19);
		if (ret < 0) {
			dev_err(&pdev->dev, "Failed to request BUCK[%d] OCP IRQ: %d: %d\n",
				i+1, s2mps19->buck_ocp_irq[i], ret);
		}
	}

	s2mps19->bb_ocp_irq = irq_base + S2MPS19_PMIC_IRQ_OCP_BB_INT5;
	ret = devm_request_threaded_irq(&pdev->dev, s2mps19->bb_ocp_irq, NULL,
			s2mps19_bb_ocp_irq, 0, "BB_OCP_IRQ", s2mps19);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to request BUCKBOOST OCP IRQ: %d: %d\n",
			s2mps19->bb_ocp_irq, ret);
	}

	for (i = 0; i < 2; i++) {
		s2mps19->temp_irq[i] = irq_base + S2MPS19_PMIC_IRQ_120C_INT3 + i;

		ret = devm_request_threaded_irq(&pdev->dev, s2mps19->temp_irq[i], NULL,
			s2mps19_temp_irq, 0, "TEMP_IRQ", s2mps19);
		if (ret < 0) {
			dev_err(&pdev->dev, "Failed to request over temperature[%d] IRQ: %d: %d\n",
				i, s2mps19->temp_irq[i], ret);
		}
	}

	/* PWREN_SEL write */
	s2mps19_write_reg(s2mps19->i2c, S2MPS19_PMIC_REG_PWREN_SEL1, 0xBE);
	s2mps19_write_reg(s2mps19->i2c, S2MPS19_PMIC_REG_PWREN_SEL2, 0x07);

#ifdef CONFIG_SEC_PM
	if (pdata->smpl_warn_en_by_evt) {
		if (exynos_soc_info.main_rev == 1
				&& exynos_soc_info.sub_rev == 0) {
			dev_info(&pdev->dev, "Disable SMPL_WARN for EVT 1.0\n");
			pdata->smpl_warn_en = false;
			pdata->smpl_warn_dev2 = false;
		} else {
			dev_info(&pdev->dev, "Enable SMPL_WARN for EVT %u.%u\n",
					exynos_soc_info.main_rev,
					exynos_soc_info.sub_rev);
			pdata->smpl_warn_en = true;
			pdata->smpl_warn_dev2 = true;
		}
	}
#endif /* CONFIG_SEC_PM */

	if (pdata->smpl_warn_en) {
		ret = s2mps19_update_reg(s2mps19->i2c, S2MPS19_PMIC_REG_CTRL2,
						pdata->smpl_warn_vth, 0xe0);
		if (ret) {
			dev_err(&pdev->dev, "set smpl_warn configuration i2c write error\n");
			goto err;
		}
		pr_info("%s: smpl_warn vth is 0x%x\n", __func__,
							pdata->smpl_warn_vth);

		ret = s2mps19_update_reg(s2mps19->i2c, S2MPS19_PMIC_REG_CTRL2,
						pdata->smpl_warn_hys, 0x18);
		if (ret) {
			dev_err(&pdev->dev, "set smpl_warn configuration i2c write error\n");
			goto err;
		}
		pr_info("%s: smpl_warn hysteresis is 0x%x\n", __func__,
							pdata->smpl_warn_hys);
	}

	/* Check OCP Warn fusing address */
	ret = s2mps19_read_reg(s2mps19->i2c, S2MPS19_PMIC_REG_OCP_WARN1, &val);
	ret1 = s2mps19_read_reg(s2mps19->i2c, S2MPS19_PMIC_REG_OCP_WARN1_X, &val1);
	ret2 = s2mps19_read_reg(s2mps19->i2c, S2MPS19_PMIC_REG_OCP_WARN1_Y, &val2);
	ret3 = s2mps19_read_reg(s2mps19->i2c, S2MPS19_PMIC_REG_OCP_WARN1_Z, &val3);
	if (ret || ret1 || ret2 || ret3) {
		dev_err(&pdev->dev, "%s : i2c read error for ocp warn1 fusing value\n",
			__func__);
		goto err;
	}
	pr_info("%s : OCP Warn 0xA9= 0x%x, [0xAB= 0x%x, 0xAC= 0x%x, 0xAD= 0x%x]\n", __func__, val, val1, val2, val3);

	if (pdata->smpl_warn_dev2 && cal_set_cmu_smpl_warn)
		cal_set_cmu_smpl_warn();

	if (pdata->ocp_warn1_en) {
		val = (pdata->ocp_warn1_en << S2MPS19_OCP_WARN_EN_SHIFT) |
			(pdata->ocp_warn1_cnt << S2MPS19_OCP_WARN_CNT_SHIFT) |
			(pdata->ocp_warn1_dvs_mask << S2MPS19_OCP_WARN_DVS_MASK_SHIFT) |
			((pdata->ocp_warn1_lv & 0xF) << S2MPS19_OCP_WARN_LV_SHIFT);

		ret = s2mps19_write_reg(s2mps19->i2c, S2MPS19_PMIC_REG_OCP_WARN1, val);

		if (ret) {
			dev_err(&pdev->dev,"%s : i2c write for ocp warn1 configuration caused error\n",
				__func__);
			goto err;
		}

		pr_info("%s : value for ocp warn1 register is 0x%x\n", __func__, val);
	}

	if (pdata->ocp_warn2_en) {
		val = (pdata->ocp_warn2_en << S2MPS19_OCP_WARN_EN_SHIFT) |
			(pdata->ocp_warn2_cnt << S2MPS19_OCP_WARN_CNT_SHIFT) |
			(pdata->ocp_warn2_dvs_mask << S2MPS19_OCP_WARN_DVS_MASK_SHIFT) |
			((pdata->ocp_warn2_lv & 0xF) << S2MPS19_OCP_WARN_LV_SHIFT);

		ret = s2mps19_write_reg(s2mps19->i2c, S2MPS19_PMIC_REG_OCP_WARN2, val);

		if (ret) {
			dev_err(&pdev->dev,"%s : i2c write for ocp warn2 configuration caused error\n",
				__func__);
			goto err;
		}

		pr_info("%s : value for ocp warn2 register is 0x%x\n", __func__, val);
	}

	s2mps19_oi_function(iodev);

#ifdef CONFIG_SEC_PM
	ap_pmic_dev = sec_device_create(NULL, "ap_pmic");

	ret = sysfs_create_group(&ap_pmic_dev->kobj, &ap_pmic_attr_group);
	if (ret)
		dev_err(&pdev->dev, "failed to create ap_pmic sysfs group\n");
#endif /* CONFIG_SEC_PM */

#ifdef CONFIG_DEBUG_FS
	dbgi2c = s2mps19->i2c;
	s2mps19_root = debugfs_create_dir("s2mps19-regs", NULL);
	s2mps19_i2caddr = debugfs_create_file("i2caddr", 0644, s2mps19_root, NULL, &s2mps19_i2caddr_fops);
	s2mps19_i2cdata = debugfs_create_file("i2cdata", 0644, s2mps19_root, NULL, &s2mps19_i2cdata_fops);
#endif

	iodev->adc_mode = pdata->adc_mode;
	iodev->adc_sync_mode = pdata->adc_sync_mode;

#ifdef CONFIG_SOC_EXYNOS9820
	/* SICD_DVS voltage settings - BUCK1/2/3/4/5M_OUT2 */
	s2mps19_write_reg(s2mps19->i2c, S2MPS19_PMIC_REG_B1M_OUT2, 0x20);
	s2mps19_write_reg(s2mps19->i2c, S2MPS19_PMIC_REG_B2M_OUT2, 0x20);
	s2mps19_write_reg(s2mps19->i2c, S2MPS19_PMIC_REG_B3M_OUT2, 0x20);
	s2mps19_write_reg(s2mps19->i2c, S2MPS19_PMIC_REG_B4M_OUT2, 0x20);
	s2mps19_write_reg(s2mps19->i2c, S2MPS19_PMIC_REG_B5M_OUT2, 0x20);
	if (pdata->support_vdd_pll_1p7)
		s2mps19_write_reg(s2mps19->i2c, S2MPS19_PMIC_REG_L4CTRL, 0x68);
#endif
	/* set BUCK10M output2 as 1.1V --> 0.95V */
	s2mps19_write_reg(s2mps19->i2c, S2MPS19_PMIC_REG_B10M_OUT2, 0x68);


	if (iodev->adc_mode > 0)
		s2mps19_powermeter_init(iodev);

	return 0;
err:
	for (i = 0; i < S2MPS19_REGULATOR_MAX; i++)
		regulator_unregister(s2mps19->rdev[i]);

	return ret;
}

static int s2mps19_pmic_remove(struct platform_device *pdev)
{
	struct s2mps19_info *s2mps19 = platform_get_drvdata(pdev);
	int i;

#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(s2mps19_i2cdata);
	debugfs_remove_recursive(s2mps19_i2caddr);
	debugfs_remove_recursive(s2mps19_root);
#endif

	for (i = 0; i < S2MPS19_REGULATOR_MAX; i++)
		regulator_unregister(s2mps19->rdev[i]);

	s2mps19_powermeter_deinit(s2mps19->iodev);

#ifdef CONFIG_SEC_PM_BIGDATA
	cancel_delayed_work(&s2mps19->hqm_pmtp_work);
	cancel_delayed_work(&s2mps19->hqm_bocp_work);
#endif /* CONFIG_SEC_PM_BIGDATA */
	return 0;
}

static void s2mps19_pmic_shutdown(struct platform_device *pdev)
{
	struct s2mps19_info *s2mps19 = platform_get_drvdata(pdev);

	s2mps19_update_reg(s2mps19->i2c, S2MPS19_REG_ADC_CTRL3, 0, ADC_EN_MASK);
	pr_info("%s : ADC OFF\n", __func__);
}

#if defined(CONFIG_PM)
static int s2mps19_pmic_suspend(struct device *dev)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct s2mps19_info *s2mps19 = platform_get_drvdata(pdev);
	pr_info("%s adc_mode : %d\n", __func__, s2mps19->iodev->adc_mode);
	if (s2mps19->iodev->adc_mode > 0) {
		s2mps19_read_reg(s2mps19->i2c, S2MPS19_REG_ADC_CTRL3, &s2mps19->adc_en_val);
		if (s2mps19->adc_en_val & 0x80)
			s2mps19_update_reg(s2mps19->i2c, S2MPS19_REG_ADC_CTRL3, 0, ADC_EN_MASK);
	}
	return 0;

}

static int s2mps19_pmic_resume(struct device *dev)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct s2mps19_info *s2mps19 = platform_get_drvdata(pdev);
	pr_info("%s adc_mode : %d\n", __func__, s2mps19->iodev->adc_mode);

	if (s2mps19->iodev->adc_mode > 0)
		s2mps19_update_reg(s2mps19->i2c, S2MPS19_REG_ADC_CTRL3,
				s2mps19->adc_en_val & 0x80, ADC_EN_MASK);
	return 0;

}
#else
#define s2mps19_pmic_suspend	NULL
#define s2mps19_pmic_resume	NULL
#endif /* CONFIG_PM */

const struct dev_pm_ops s2mps19_pmic_pm = {
	.suspend = s2mps19_pmic_suspend,
	.resume = s2mps19_pmic_resume,
};

static const struct platform_device_id s2mps19_pmic_id[] = {
	{ "s2mps19-regulator", 0},
	{ },
};
MODULE_DEVICE_TABLE(platform, s2mps19_pmic_id);

static struct platform_driver s2mps19_pmic_driver = {
	.driver = {
		.name = "s2mps19-regulator",
		.owner = THIS_MODULE,
#if defined(CONFIG_PM)
		.pm = &s2mps19_pmic_pm,
#endif
		.suppress_bind_attrs = true,
	},
	.probe = s2mps19_pmic_probe,
	.remove = s2mps19_pmic_remove,
	.shutdown = s2mps19_pmic_shutdown,
	.id_table = s2mps19_pmic_id,
};

static int __init s2mps19_pmic_init(void)
{
	return platform_driver_register(&s2mps19_pmic_driver);
}
subsys_initcall(s2mps19_pmic_init);

static void __exit s2mps19_pmic_exit(void)
{
	platform_driver_unregister(&s2mps19_pmic_driver);
}
module_exit(s2mps19_pmic_exit);

/* Module information */
MODULE_AUTHOR("Sangbeom Kim <sbkim73@samsung.com>");
MODULE_DESCRIPTION("SAMSUNG S2MPS19 Regulator Driver");
MODULE_LICENSE("GPL");
