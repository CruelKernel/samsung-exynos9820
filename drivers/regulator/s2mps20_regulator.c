/*
 * s2mps20.c
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
#include <linux/mfd/samsung/s2mps20.h>
#include <linux/mfd/samsung/s2mps20-regulator.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/debug-snapshot.h>
#include <linux/debugfs.h>
#include <linux/interrupt.h>
#ifdef CONFIG_SEC_PM
#include <linux/sec_class.h>

struct device *ap_sub_pmic_dev;
#endif /* CONFIG_SEC_PM */

static struct s2mps20_info *s2mps20_static_info;
static struct regulator_desc regulators[S2MPS20_REGULATOR_MAX];
int s2mps20_buck_ocp_cnt[3]; /* BUCK 1~3 OCP count */
int s2mps20_temp_cnt[2]; /* 0 : 120 degree , 1 : 140 degree */


#ifdef CONFIG_DEBUG_FS
static u8 i2caddr = 0;
static u8 i2cdata = 0;
static struct i2c_client *dbgi2c = NULL;
static struct dentry *s2mps20_root = NULL;
static struct dentry *s2mps20_i2caddr = NULL;
static struct dentry *s2mps20_i2cdata = NULL;
#endif

struct s2mps20_info {
	struct regulator_dev *rdev[S2MPS20_REGULATOR_MAX];
	unsigned int opmode[S2MPS20_REGULATOR_MAX];
	int num_regulators;
	struct s2mps20_dev *iodev;
	struct mutex lock;
	bool g3d_en;
	struct i2c_client *i2c;
	struct i2c_client *debug_i2c;
	u8 adc_en_val;
	int buck_ocp_irq[3];	/* BUCK OCP IRQ */
	int temp_irq[2];	/* 0 : 120 degree, 1 : 140 degree */
};

static unsigned int s2mps20_of_map_mode(unsigned int val) {
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
	struct s2mps20_info *s2mps20 = rdev_get_drvdata(rdev);
	unsigned int val;
	int id = rdev_get_id(rdev);

	val = mode << S2MPS20_ENABLE_SHIFT;

	s2mps20->opmode[id] = val;
	return 0;
}

static int s2m_enable(struct regulator_dev *rdev)
{
	struct s2mps20_info *s2mps20 = rdev_get_drvdata(rdev);

	return s2mps20_update_reg(s2mps20->i2c, rdev->desc->enable_reg,
				  s2mps20->opmode[rdev_get_id(rdev)],
				  rdev->desc->enable_mask);
}

static int s2m_disable_regmap(struct regulator_dev *rdev)
{
	struct s2mps20_info *s2mps20 = rdev_get_drvdata(rdev);
	unsigned int val;

	if (rdev->desc->enable_is_inverted)
		val = rdev->desc->enable_mask;
	else
		val = 0;

	return s2mps20_update_reg(s2mps20->i2c, rdev->desc->enable_reg,
				  val, rdev->desc->enable_mask);
}

static int s2m_is_enabled_regmap(struct regulator_dev *rdev)
{
	struct s2mps20_info *s2mps20 = rdev_get_drvdata(rdev);
	int ret;
	u8 val;

	ret = s2mps20_read_reg(s2mps20->i2c,
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
	struct s2mps20_info *s2mps20 = rdev_get_drvdata(rdev);
	int ramp_shift, reg_id = rdev_get_id(rdev);
	int ramp_mask = 0x03;
	unsigned int ramp_value = 0;

	ramp_value = get_ramp_delay(ramp_delay/1000);
	if (ramp_value > 4) {
		pr_warn("%s: ramp_delay: %d not supported\n",
			rdev->desc->name, ramp_delay);
	}

	switch (reg_id) {
	case S2MPS20_BUCK1:
		ramp_shift = 6;
		break;
	case S2MPS20_BUCK2:
		ramp_shift = 4;
		break;
	default:
		return -EINVAL;
	}

	return s2mps20_update_reg(s2mps20->i2c, S2MPS20_PMIC_REG_BUCKRAMP,
		ramp_value << ramp_shift, ramp_mask << ramp_shift);
}

static int s2m_get_voltage_sel_regmap(struct regulator_dev *rdev)
{
	struct s2mps20_info *s2mps20 = rdev_get_drvdata(rdev);
	int ret;
	u8 val;

	ret = s2mps20_read_reg(s2mps20->i2c, rdev->desc->vsel_reg, &val);
	if (ret)
		return ret;

	val &= rdev->desc->vsel_mask;

	return val;
}

static int s2m_set_voltage_sel_regmap(struct regulator_dev *rdev, unsigned sel)
{
	struct s2mps20_info *s2mps20 = rdev_get_drvdata(rdev);
	int reg_id = rdev_get_id(rdev);
	int ret;
	char name[16];

	/* voltage information logging to snapshot feature */
	snprintf(name, sizeof(name), "LDO%d", (reg_id - S2MPS20_LDO1) + 1);
	ret = s2mps20_update_reg(s2mps20->i2c, rdev->desc->vsel_reg,
				  sel, rdev->desc->vsel_mask);
	if (ret < 0)
		goto out;

	if (rdev->desc->apply_bit)
		ret = s2mps20_update_reg(s2mps20->i2c, rdev->desc->apply_reg,
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
	struct s2mps20_info *s2mps20 = rdev_get_drvdata(rdev);

	ret = s2mps20_write_reg(s2mps20->i2c, rdev->desc->vsel_reg, sel);
	if (ret < 0)
		goto i2c_out;

	if (rdev->desc->apply_bit)
		ret = s2mps20_update_reg(s2mps20->i2c, rdev->desc->apply_reg,
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

static struct regulator_ops s2mps20_ldo_ops = {
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

static struct regulator_ops s2mps20_buck_ops = {
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

#define _BUCK(macro)	S2MPS20_BUCK##macro
#define _buck_ops(num)	s2mps20_buck_ops##num
#define _LDO(macro)		S2MPS20_LDO##macro
#define _ldo_ops(num)	s2mps20_ldo_ops##num

#define _REG(ctrl)		S2MPS20_PMIC_REG##ctrl
#define _TIME(macro)	S2MPS20_ENABLE_TIME##macro

#define _LDO_MIN(group)		S2MPS20_LDO_MIN##group
#define _LDO_STEP(group)	S2MPS20_LDO_STEP##group
#define _BUCK_MIN(group)	S2MPS20_BUCK_MIN##group
#define _BUCK_STEP(group)	S2MPS20_BUCK_STEP##group

#define BUCK_DESC(_name, _id, _ops, g, v, e, t)	{	\
	.name		= _name,				\
	.id			= _id,					\
	.ops		= _ops,					\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= _BUCK_MIN(g),					\
	.uV_step	= _BUCK_STEP(g),					\
	.n_voltages	= S2MPS20_BUCK_N_VOLTAGES,		\
	.vsel_reg	= v,					\
	.vsel_mask	= S2MPS20_BUCK_VSEL_MASK,		\
	.enable_reg	= e,					\
	.enable_mask	= S2MPS20_ENABLE_MASK,			\
	.enable_time	= t,					\
	.of_map_mode	= s2mps20_of_map_mode			\
}

#define LDO_DESC(_name, _id, _ops, g, v, e, t)	{	\
	.name		= _name,				\
	.id			= _id,					\
	.ops		= _ops,					\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= _LDO_MIN(g),					\
	.uV_step	= _LDO_STEP(g),					\
	.n_voltages	= S2MPS20_LDO_N_VOLTAGES,		\
	.vsel_reg	= v,					\
	.vsel_mask	= S2MPS20_LDO_VSEL_MASK,		\
	.enable_reg	= e,					\
	.enable_mask	= S2MPS20_ENABLE_MASK,			\
	.enable_time	= t,					\
	.of_map_mode	= s2mps20_of_map_mode			\
}

static struct regulator_desc regulators[S2MPS20_REGULATOR_MAX] = {
	/* name, id, ops, group, vsel_reg, enable_reg, ramp_delay */
	LDO_DESC("LDO1S", _LDO(1), &_ldo_ops(), 1,
	_REG(_L1CTRL), _REG(_L1CTRL), _TIME(_LDO)),
	LDO_DESC("LDO2S", _LDO(2), &_ldo_ops(), 2,
	_REG(_L2CTRL), _REG(_L2CTRL), _TIME(_LDO)),
/*	LDO_DESC("LDO3S", _LDO(3), &_ldo_ops(), 4,
	_REG(_L3CTRL), _REG(_L3CTRL), _TIME(_LDO)),
	LDO_DESC("LDO4S", _LDO(4), &_ldo_ops(), 4,
	_REG(_L4CTRL), _REG(_L4CTRL), _TIME(_LDO)),
	LDO_DESC("LDO5S", _LDO(5), &_ldo_ops(), 4,
	_REG(_L5CTRL), _REG(_L5CTRL), _TIME(_LDO)),
	LDO_DESC("LDO6S", _LDO(6), &_ldo_ops(), 2,
	_REG(_L6CTRL), _REG(_L6CTRL), _TIME(_LDO)),
*/	LDO_DESC("LDO7S", _LDO(7), &_ldo_ops(), 2,
	_REG(_L7CTRL), _REG(_L7CTRL), _TIME(_LDO)),
	LDO_DESC("LDO8S", _LDO(8), &_ldo_ops(), 4,
	_REG(_L8CTRL), _REG(_L8CTRL), _TIME(_LDO)),
	LDO_DESC("LDO9S", _LDO(9), &_ldo_ops(), 4,
	_REG(_L9CTRL), _REG(_L9CTRL), _TIME(_LDO)),
	LDO_DESC("LDO10S", _LDO(10), &_ldo_ops(), 2,
	_REG(_L10CTRL), _REG(_L10CTRL), _TIME(_LDO)),
	LDO_DESC("LDO11S", _LDO(11), &_ldo_ops(), 2,
	_REG(_L11CTRL), _REG(_L11CTRL), _TIME(_LDO)),
	LDO_DESC("LDO12S", _LDO(12), &_ldo_ops(), 3,
	_REG(_L12CTRL), _REG(_L12CTRL), _TIME(_LDO)),
	BUCK_DESC("BUCK1S", _BUCK(1), &_buck_ops(), 1,
	_REG(_B1S_OUT), _REG(_B1S_CTRL), _TIME(_BUCK)),
	BUCK_DESC("BUCK2S", _BUCK(2), &_buck_ops(), 1,
	_REG(_B2S_OUT), _REG(_B2S_CTRL), _TIME(_BUCK)),
/*	BUCK_DESC("BUCK3S", _BUCK(3), &_buck_ops(), 1,
	_REG(_B3S_OUT), _REG(_B3S_CTRL), _TIME(_BUCK))*/
};
#ifdef CONFIG_OF
static int s2mps20_pmic_dt_parse_pdata(struct s2mps20_dev *iodev,
					struct s2mps20_platform_data *pdata)
{
	struct device_node *pmic_np, *regulators_np, *reg_np;
	struct s2mps20_regulator_data *rdata;
	unsigned int i;
	int ret;
	u32 val;

	pmic_np = iodev->dev->of_node;
	if (!pmic_np) {
		dev_err(iodev->dev, "could not find pmic sub-node\n");
		return -ENODEV;
	}

	pdata->adc_mode = 0;
	ret = of_property_read_u32(pmic_np, "adc_mode", &val);

	pdata->adc_mode = val;

	pdata->adc_sync_mode = 0;
	ret = of_property_read_u32(pmic_np, "adc_sync_mode", &val);

	pdata->adc_sync_mode = val;

	ret = of_property_read_u32(pmic_np, "ocp_warn3_en", &val);
	if (ret)
		return -EINVAL;
	pdata->ocp_warn3_en = !!val;

	ret = of_property_read_u32(pmic_np, "ocp_warn3_cnt", &val);
	if (ret)
		return -EINVAL;
	pdata->ocp_warn3_cnt = val;

	ret = of_property_read_u32(pmic_np, "ocp_warn3_dvs_mask", &val);
	if (ret)
		return -EINVAL;
	pdata->ocp_warn3_dvs_mask = val;

	ret = of_property_read_u32(pmic_np, "ocp_warn3_lv", &val);
	if (ret)
		return -EINVAL;
	pdata->ocp_warn3_lv = val;

	pdata->buck1s_pwm = 0;
	ret = of_property_read_u32(pmic_np, "buck1s_pwm", &val);
	if (ret)
		pdata->buck1s_pwm = 0;
	else
		pdata->buck1s_pwm = !!val;

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
static int s2mps20_pmic_dt_parse_pdata(struct s2mps20_pmic_dev *iodev,
					struct s2mps20_platform_data *pdata)
{
	return 0;
}
#endif /* CONFIG_OF */

#ifdef CONFIG_DEBUG_FS
static ssize_t s2mps20_i2caddr_read(struct file *file, char __user *user_buf,
					size_t count, loff_t *ppos)
{
	char buf[10];
	ssize_t ret;

	ret = snprintf(buf, sizeof(buf), "0x%x\n", i2caddr);
	if (ret < 0)
		return ret;

	return simple_read_from_buffer(user_buf, count, ppos, buf, ret);
}

static ssize_t s2mps20_i2caddr_write(struct file *file, const char __user *user_buf,
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

static ssize_t s2mps20_i2cdata_read(struct file *file, char __user *user_buf,
					size_t count, loff_t *ppos)
{
	char buf[10];
	ssize_t ret;

	ret = s2mps20_read_reg(dbgi2c, i2caddr, &i2cdata);
	if (ret)
		return ret;

	ret = snprintf(buf, sizeof(buf), "0x%x\n", i2cdata);
	if (ret < 0)
		return ret;

	return simple_read_from_buffer(user_buf, count, ppos, buf, ret);
}

static ssize_t s2mps20_i2cdata_write(struct file *file, const char __user *user_buf,
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
		ret = s2mps20_write_reg(dbgi2c, i2caddr, val);
		if (ret < 0)
			return ret;
	}

	return len;
}

static const struct file_operations s2mps20_i2caddr_fops = {
	.open = simple_open,
	.read = s2mps20_i2caddr_read,
	.write = s2mps20_i2caddr_write,
	.llseek = default_llseek,
};
static const struct file_operations s2mps20_i2cdata_fops = {
	.open = simple_open,
	.read = s2mps20_i2cdata_read,
	.write = s2mps20_i2cdata_write,
	.llseek = default_llseek,
};
#endif

#ifdef CONFIG_EXYNOS_OCP
void get_s2mps20_i2c(struct i2c_client **i2c)
{
	*i2c = s2mps20_static_info->i2c;
}
#endif

static irqreturn_t s2mps20_buck_ocp_irq(int irq, void *data)
{
	struct s2mps20_info *s2mps20 = data;
	int i;

	mutex_lock(&s2mps20->lock);

	for (i = 0; i < 3; i++) {
		if (s2mps20_static_info->buck_ocp_irq[i] == irq) {
			s2mps20_buck_ocp_cnt[i]++;
			pr_info("%s : BUCK[%d] OCP IRQ : %d, %d\n",
				__func__, i+1, s2mps20_buck_ocp_cnt[i], irq);
			break;
		}
	}

	mutex_unlock(&s2mps20->lock);
	return IRQ_HANDLED;
}

static irqreturn_t s2mps20_temp_irq(int irq, void *data)
{
	struct s2mps20_info *s2mps20 = data;

	mutex_lock(&s2mps20->lock);

	if (s2mps20_static_info->temp_irq[0] == irq) {
		s2mps20_temp_cnt[0]++;
		pr_info("%s: PMIC thermal 120C IRQ : %d, %d\n",
			__func__, s2mps20_temp_cnt[0], irq);
	} else if (s2mps20_static_info->temp_irq[1] == irq) {
		s2mps20_temp_cnt[1]++;
		pr_info("%s: PMIC thermal 140C IRQ : %d, %d\n",
			__func__, s2mps20_temp_cnt[1], irq);
	}

	mutex_unlock(&s2mps20->lock);
	return IRQ_HANDLED;
}

void s2mps20_oi_function(struct s2mps20_dev *iodev)
{
	struct i2c_client *i2c = iodev->pmic;
	int i;
	u8 val;

	/* BUCK1~3 OI function enable & power down disable */
	s2mps20_update_reg(i2c, S2MPS20_PMIC_REG_BUCK_OI_EN1, 0x7, 0x77);

	/* OI detection time window : 500us, OI comp. output count : 50 times */
	s2mps20_write_reg(i2c, S2MPS20_PMIC_REG_BUCK_OI_CTRL1, 0xcc);
	s2mps20_update_reg(i2c, S2MPS20_PMIC_REG_BUCK_OI_CTRL2, 0x0c, 0x0c);

	pr_info("%s\n", __func__);
	for (i = S2MPS20_PMIC_REG_BUCK_OI_EN1; i <= S2MPS20_PMIC_REG_BUCK_OI_CTRL2; i++) {
		s2mps20_read_reg(i2c, i, &val);
		pr_info("0x%x[0x%x], ", i, val);
	}
	pr_info("\n");
}

static int s2mps20_pmic_probe(struct platform_device *pdev)
{
	struct s2mps20_dev *iodev = dev_get_drvdata(pdev->dev.parent);
	struct s2mps20_platform_data *pdata = iodev->pdata;
	struct regulator_config config = { };
	struct s2mps20_info *s2mps20;
	int irq_base;
	int i, ret;
	u8 val;

	if (iodev->dev->of_node) {
		ret = s2mps20_pmic_dt_parse_pdata(iodev, pdata);
		if (ret)
			return ret;
	}

	if (!pdata) {
		dev_err(pdev->dev.parent, "Platform data not supplied\n");
		return -ENODEV;
	}

	s2mps20 = devm_kzalloc(&pdev->dev, sizeof(struct s2mps20_info),
				GFP_KERNEL);
	if (!s2mps20)
		return -ENOMEM;

	irq_base = pdata->irq_base;
	if (!irq_base) {
		dev_err(&pdev->dev, "Failed to get irq base %d\n", irq_base);
		return -ENODEV;
	}

	s2mps20->iodev = iodev;
	s2mps20->i2c = iodev->pmic;
	s2mps20->debug_i2c = iodev->debug_i2c;

	mutex_init(&s2mps20->lock);

	s2mps20->g3d_en = pdata->g3d_en;
	s2mps20_static_info = s2mps20;

	platform_set_drvdata(pdev, s2mps20);

#ifdef CONFIG_SEC_PM
	/* Clear OFFSRC register */
	ret = s2mps20_write_reg(s2mps20->i2c, S2MPS20_PMIC_REG_OFFSRC, 0);
	if (ret)
		dev_err(&pdev->dev, "failed to write OFFSRC\n");
#endif /* CONFIG_SEC_PM */

	for (i = 0; i < pdata->num_regulators; i++) {
		int id = pdata->regulators[i].id;
		config.dev = &pdev->dev;
		config.init_data = pdata->regulators[i].initdata;
		config.driver_data = s2mps20;
		config.of_node = pdata->regulators[i].reg_node;
		s2mps20->opmode[id] =
			regulators[id].enable_mask;

		s2mps20->rdev[i] = regulator_register(
				&regulators[id], &config);
		if (IS_ERR(s2mps20->rdev[i])) {
			ret = PTR_ERR(s2mps20->rdev[i]);
			dev_err(&pdev->dev, "regulator init failed for %d\n",
				i);
			s2mps20->rdev[i] = NULL;
			goto err;
		}
	}

	s2mps20->num_regulators = pdata->num_regulators;

	for (i = 0; i < 3; i++) {
		s2mps20->buck_ocp_irq[i] = irq_base + S2MPS20_PMIC_IRQ_OCP_B1_INT2 + i;

		ret = devm_request_threaded_irq(&pdev->dev, s2mps20->buck_ocp_irq[i], NULL,
			s2mps20_buck_ocp_irq, 0, "BUCK_OCP_IRQ", s2mps20);
		if (ret < 0) {
			dev_err(&pdev->dev, "Failed to request BUCK[%d] OCP IRQ: %d: %d\n",
				i+1, s2mps20->buck_ocp_irq[i], ret);
		}
	}

	for (i = 0; i < 2; i++) {
		s2mps20->temp_irq[i] = irq_base + S2MPS20_PMIC_IRQ_INT120C_INT2 + i;

		ret = devm_request_threaded_irq(&pdev->dev, s2mps20->temp_irq[i], NULL,
			s2mps20_temp_irq, 0, "TEMP_IRQ", s2mps20);
		if (ret < 0) {
			dev_err(&pdev->dev, "Failed to request over temperature[%d] IRQ: %d: %d\n",
				i, s2mps20->temp_irq[i], ret);
		}
	}

	if (pdata->ocp_warn3_en) {
		val = (pdata->ocp_warn3_en << S2MPS20_OCP_WARN_EN_SHIFT) |
			(pdata->ocp_warn3_cnt << S2MPS20_OCP_WARN_CNT_SHIFT) |
			(pdata->ocp_warn3_dvs_mask << S2MPS20_OCP_WARN_DVS_MASK_SHIFT) |
			((pdata->ocp_warn3_lv & 0xF) << S2MPS20_OCP_WARN_LV_SHIFT);

		ret = s2mps20_write_reg(s2mps20->i2c, S2MPS20_PMIC_REG_OCP_WARN3, val);

		if (ret) {
			dev_err(&pdev->dev,"%s : i2c write for ocp warn3 configuration caused error\n",
				__func__);
			goto err;
		}

		pr_info("%s : value for ocp warn3 register is 0x%x\n", __func__, val);
	}

	s2mps20_oi_function(iodev);

#ifdef CONFIG_SEC_PM
	ap_sub_pmic_dev = sec_device_create(NULL, "ap_sub_pmic");
#endif /* CONFIG_SEC_PM */

	/* BUCK1S PWM */
	if (pdata->buck1s_pwm) {
		ret = s2mps20_update_reg(s2mps20->i2c, S2MPS20_PMIC_REG_B1S_CTRL, 0x0C, 0x0C);
		if (ret) {
			dev_err(&pdev->dev,"%s : i2c write for buck1s pwm configuration caused error\n",
				__func__);
			goto err;
		}
		pr_info("%s : Set BUCK1S PWM\n", __func__);
	}

#ifdef CONFIG_DEBUG_FS
	dbgi2c = s2mps20->i2c;
	s2mps20_root = debugfs_create_dir("s2mps20-regs", NULL);
	s2mps20_i2caddr = debugfs_create_file("i2caddr", 0644, s2mps20_root, NULL, &s2mps20_i2caddr_fops);
	s2mps20_i2cdata = debugfs_create_file("i2cdata", 0644, s2mps20_root, NULL, &s2mps20_i2cdata_fops);
#endif

	iodev->adc_mode = pdata->adc_mode;
	iodev->adc_sync_mode = pdata->adc_sync_mode;
	if (iodev->adc_mode > 0)
		s2mps20_powermeter_init(iodev);

	return 0;
err:
	for (i = 0; i < S2MPS20_REGULATOR_MAX; i++)
		regulator_unregister(s2mps20->rdev[i]);

	return ret;
}

static int s2mps20_pmic_remove(struct platform_device *pdev)
{
	struct s2mps20_info *s2mps20 = platform_get_drvdata(pdev);
	int i;

#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(s2mps20_i2cdata);
	debugfs_remove_recursive(s2mps20_i2caddr);
	debugfs_remove_recursive(s2mps20_root);
#endif

	for (i = 0; i < S2MPS20_REGULATOR_MAX; i++)
		regulator_unregister(s2mps20->rdev[i]);

	s2mps20_powermeter_deinit(s2mps20->iodev);
	return 0;
}

static void s2mps20_pmic_shutdown(struct platform_device *pdev)
{
	struct s2mps20_info *s2mps20 = platform_get_drvdata(pdev);

	s2mps20_update_reg(s2mps20->i2c, S2MPS20_REG_ADC_CTRL3, 0, ADC_EN_MASK);
	pr_info("%s : ADC OFF\n", __func__);
}

#if defined(CONFIG_PM)
static int s2mps20_pmic_suspend(struct device *dev)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct s2mps20_info *s2mps20 = platform_get_drvdata(pdev);
	pr_info("%s adc_mode : %d\n", __func__, s2mps20->iodev->adc_mode);
	if (s2mps20->iodev->adc_mode > 0) {
		s2mps20_read_reg(s2mps20->i2c, S2MPS20_REG_ADC_CTRL3, &s2mps20->adc_en_val);
		if (s2mps20->adc_en_val & 0x80)
			s2mps20_update_reg(s2mps20->i2c, S2MPS20_REG_ADC_CTRL3, 0, ADC_EN_MASK);
	}
	return 0;

}

static int s2mps20_pmic_resume(struct device *dev)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct s2mps20_info *s2mps20 = platform_get_drvdata(pdev);
	pr_info("%s adc_mode : %d\n", __func__, s2mps20->iodev->adc_mode);

	if (s2mps20->iodev->adc_mode > 0)
		s2mps20_update_reg(s2mps20->i2c, S2MPS20_REG_ADC_CTRL3,
				s2mps20->adc_en_val & 0x80, ADC_EN_MASK);
	return 0;

}
#else
#define s2mps20_pmic_suspend	NULL
#define s2mps20_pmic_resume	NULL
#endif /* CONFIG_PM */

const struct dev_pm_ops s2mps20_pmic_pm = {
	.suspend = s2mps20_pmic_suspend,
	.resume = s2mps20_pmic_resume,
};

static const struct platform_device_id s2mps20_pmic_id[] = {
	{ "s2mps20-regulator", 0},
	{ },
};
MODULE_DEVICE_TABLE(platform, s2mps20_pmic_id);

static struct platform_driver s2mps20_pmic_driver = {
	.driver = {
		.name = "s2mps20-regulator",
		.owner = THIS_MODULE,
#if defined(CONFIG_PM)
		.pm = &s2mps20_pmic_pm,
#endif
		.suppress_bind_attrs = true,
	},
	.probe = s2mps20_pmic_probe,
	.remove = s2mps20_pmic_remove,
	.shutdown = s2mps20_pmic_shutdown,
	.id_table = s2mps20_pmic_id,
};

static int __init s2mps20_pmic_init(void)
{
	return platform_driver_register(&s2mps20_pmic_driver);
}
subsys_initcall(s2mps20_pmic_init);

static void __exit s2mps20_pmic_exit(void)
{
	platform_driver_unregister(&s2mps20_pmic_driver);
}
module_exit(s2mps20_pmic_exit);

/* Module information */
MODULE_AUTHOR("Sangbeom Kim <sbkim73@samsung.com>");
MODULE_DESCRIPTION("SAMSUNG S2MPS20 Regulator Driver");
MODULE_LICENSE("GPL");
