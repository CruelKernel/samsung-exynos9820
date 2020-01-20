/*
 * s2mpb03.c - Regulator driver for the Samsung s2mpb03
 *
 * Copyright (C) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/module.h>
#include <linux/bug.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/s2mpb03.h>
#include <linux/regulator/of_regulator.h>

struct s2mpb03_data {
	struct s2mpb03_dev *iodev;
	int num_regulators;
	struct regulator_dev *rdev[S2MPB03_REGULATOR_MAX];
	int opmode[S2MPB03_REGULATOR_MAX];
};

int s2mpb03_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest)
{
	struct s2mpb03_dev *s2mpb03 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&s2mpb03->i2c_lock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	mutex_unlock(&s2mpb03->i2c_lock);
	if (ret < 0) {
		pr_info("%s:%s reg(0x%x), ret(%d)\n",
			 MFD_DEV_NAME, __func__, reg, ret);
		return ret;
	}

	ret &= 0xff;
	*dest = ret;
	return 0;
}

int s2mpb03_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct s2mpb03_dev *s2mpb03 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&s2mpb03->i2c_lock);
	ret = i2c_smbus_read_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&s2mpb03->i2c_lock);
	if (ret < 0)
		return ret;

	return 0;
}

int s2mpb03_read_word(struct i2c_client *i2c, u8 reg)
{
	struct s2mpb03_dev *s2mpb03 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&s2mpb03->i2c_lock);
	ret = i2c_smbus_read_word_data(i2c, reg);
	mutex_unlock(&s2mpb03->i2c_lock);
	if (ret < 0)
		return ret;

	return ret;
}

int s2mpb03_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct s2mpb03_dev *s2mpb03 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&s2mpb03->i2c_lock);
	ret = i2c_smbus_write_byte_data(i2c, reg, value);
	mutex_unlock(&s2mpb03->i2c_lock);
	if (ret < 0)
		pr_info("%s:%s reg(0x%x), ret(%d)\n",
				MFD_DEV_NAME, __func__, reg, ret);

	return ret;
}

int s2mpb03_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct s2mpb03_dev *s2mpb03 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&s2mpb03->i2c_lock);
	ret = i2c_smbus_write_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&s2mpb03->i2c_lock);
	if (ret < 0)
		return ret;

	return 0;
}

int s2mpb03_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	struct s2mpb03_dev *s2mpb03 = i2c_get_clientdata(i2c);
	int ret;
	u8 old_val, new_val;

	mutex_lock(&s2mpb03->i2c_lock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret >= 0) {
		old_val = ret & 0xff;
		new_val = (val & mask) | (old_val & (~mask));
		ret = i2c_smbus_write_byte_data(i2c, reg, new_val);
	}
	mutex_unlock(&s2mpb03->i2c_lock);
	return ret;
}

static int s2m_enable(struct regulator_dev *rdev)
{
	struct s2mpb03_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;

	return s2mpb03_update_reg(i2c, rdev->desc->enable_reg,
				  info->opmode[rdev_get_id(rdev)],
					rdev->desc->enable_mask);
}

static int s2m_disable_regmap(struct regulator_dev *rdev)
{
	struct s2mpb03_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	u8 val;

	if (rdev->desc->enable_is_inverted)
		val = rdev->desc->enable_mask;
	else
		val = 0;

	return s2mpb03_update_reg(i2c, rdev->desc->enable_reg,
				   val, rdev->desc->enable_mask);
}

static int s2m_is_enabled_regmap(struct regulator_dev *rdev)
{
	struct s2mpb03_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret;
	u8 val;

	ret = s2mpb03_read_reg(i2c, rdev->desc->enable_reg, &val);
	if (ret < 0)
		return ret;

	if (rdev->desc->enable_is_inverted)
		return (val & rdev->desc->enable_mask) == 0;
	else
		return (val & rdev->desc->enable_mask) != 0;
}

static int s2m_get_voltage_sel_regmap(struct regulator_dev *rdev)
{
	struct s2mpb03_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret;
	u8 val;

	ret = s2mpb03_read_reg(i2c, rdev->desc->vsel_reg, &val);
	if (ret < 0)
		return ret;

	val &= rdev->desc->vsel_mask;

	return val;
}

static int s2m_set_voltage_sel_regmap(struct regulator_dev *rdev, unsigned sel)
{
	struct s2mpb03_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret;

	ret = s2mpb03_update_reg(i2c, rdev->desc->vsel_reg,
				sel, rdev->desc->vsel_mask);
	if (ret < 0)
		goto out;

	if (rdev->desc->apply_bit)
		ret = s2mpb03_update_reg(i2c, rdev->desc->apply_reg,
					 rdev->desc->apply_bit,
					 rdev->desc->apply_bit);
	return ret;
out:
	pr_warn("%s: failed to set voltage_sel_regmap\n", rdev->desc->name);
	return ret;
}

static int s2m_set_voltage_time_sel(struct regulator_dev *rdev,
				   unsigned int old_selector,
				   unsigned int new_selector)
{
	int old_volt, new_volt;

	/* sanity check */
	if (!rdev->desc->ops->list_voltage)
		return -EINVAL;

	old_volt = rdev->desc->ops->list_voltage(rdev, old_selector);
	new_volt = rdev->desc->ops->list_voltage(rdev, new_selector);

	if (old_selector < new_selector)
		return DIV_ROUND_UP(new_volt - old_volt, S2MPB03_RAMP_DELAY);

	return 0;
}

static struct regulator_ops s2mpb03_ldo_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.is_enabled		= s2m_is_enabled_regmap,
	.enable			= s2m_enable,
	.disable		= s2m_disable_regmap,
	.get_voltage_sel	= s2m_get_voltage_sel_regmap,
	.set_voltage_sel	= s2m_set_voltage_sel_regmap,
	.set_voltage_time_sel	= s2m_set_voltage_time_sel,
};

#define _LDO(macro)	S2MPB03_LDO##macro
#define _REG(ctrl)	S2MPB03_REG##ctrl
#define _ldo_ops(num)	s2mpb03_ldo_ops##num
#define _TIME(macro)	S2MPB03_ENABLE_TIME##macro

#define LDO_DESC(_name, _id, _ops, m, s, v, e, t)	{	\
	.name		= _name,				\
	.id		= _id,					\
	.ops		= _ops,					\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= m,					\
	.uV_step	= s,					\
	.n_voltages	= S2MPB03_LDO_N_VOLTAGES,		\
	.vsel_reg	= v,					\
	.vsel_mask	= S2MPB03_LDO_VSEL_MASK,		\
	.enable_reg	= e,					\
	.enable_mask	= S2MPB03_LDO_ENABLE_MASK,		\
	.enable_time	= t					\
}

static struct regulator_desc regulators[S2MPB03_REGULATOR_MAX] = {
		/* name, id, ops, min_uv, uV_step, vsel_reg, enable_reg */
		LDO_DESC("s2mpb03-ldo1", _LDO(1), &_ldo_ops(), _LDO(_MIN1),
			_LDO(_STEP2), _REG(_LDO1_CTRL),
			_REG(_LDO1_CTRL), _TIME(_LDO)),
		LDO_DESC("s2mpb03-ldo2", _LDO(2), &_ldo_ops(), _LDO(_MIN1),
			_LDO(_STEP2), _REG(_LDO2_CTRL),
			_REG(_LDO2_CTRL),  _TIME(_LDO)),
		LDO_DESC("s2mpb03-ldo3", _LDO(3), &_ldo_ops(), _LDO(_MIN1),
			_LDO(_STEP1), _REG(_LDO3_CTRL),
			_REG(_LDO3_CTRL), _TIME(_LDO)),
		LDO_DESC("s2mpb03-ldo4", _LDO(4), &_ldo_ops(), _LDO(_MIN1),
			_LDO(_STEP2), _REG(_LDO4_CTRL),
			_REG(_LDO4_CTRL), _TIME(_LDO)),
		LDO_DESC("s2mpb03-ldo5", _LDO(5), &_ldo_ops(), _LDO(_MIN2),
			_LDO(_STEP1), _REG(_LDO5_CTRL),
			_REG(_LDO5_CTRL), _TIME(_LDO)),
		LDO_DESC("s2mpb03-ldo6", _LDO(6), &_ldo_ops(), _LDO(_MIN2),
			_LDO(_STEP1), _REG(_LDO6_CTRL),
			_REG(_LDO6_CTRL), _TIME(_LDO)),
		LDO_DESC("s2mpb03-ldo7", _LDO(7), &_ldo_ops(), _LDO(_MIN2),
			_LDO(_STEP1), _REG(_LDO7_CTRL),
			_REG(_LDO7_CTRL), _TIME(_LDO))
};

#ifdef CONFIG_OF
static int s2mpb03_pmic_dt_parse_pdata(struct device *dev,
					struct s2mpb03_platform_data *pdata)
{
	struct device_node *pmic_np, *regulators_np, *reg_np;
	struct s2mpb03_regulator_data *rdata;
	unsigned int i;

	pmic_np = dev->of_node;
	if (!pmic_np) {
		dev_err(dev, "could not find pmic sub-node\n");
		return -ENODEV;
	}
	pdata->wakeup = of_property_read_bool(pmic_np, "s2mpb03,wakeup");

	regulators_np = of_find_node_by_name(pmic_np, "regulators");
	if (!regulators_np) {
		dev_err(dev, "could not find regulators sub-node\n");
		return -EINVAL;
	}

	/* count the number of regulators to be supported in pmic */
	pdata->num_regulators = 0;
	for_each_child_of_node(regulators_np, reg_np) {
		pdata->num_regulators++;
	}

	rdata = devm_kzalloc(dev, sizeof(*rdata) *
				pdata->num_regulators, GFP_KERNEL);
	if (!rdata) {
		dev_err(dev,
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
			dev_warn(dev,
			"don't know how to configure regulator %s\n",
			reg_np->name);
			continue;
		}

		rdata->id = i;
		rdata->initdata = of_get_regulator_init_data(
						dev, reg_np,
						&regulators[i]);
		rdata->reg_node = reg_np;
		rdata++;
	}
	of_node_put(regulators_np);

	return 0;
}
#else
static int s2mpb03_pmic_dt_parse_pdata(struct s2mpb03_dev *iodev,
					struct s2mpb03_platform_data *pdata)
{
	return 0;
}
#endif /* CONFIG_OF */

static int s2mpb03_pmic_probe(struct i2c_client *i2c,
				const struct i2c_device_id *dev_id)
{
	struct s2mpb03_dev *iodev;
	struct s2mpb03_platform_data *pdata = i2c->dev.platform_data;
	struct regulator_config config = { };
	struct s2mpb03_data *s2mpb03;
	int i;
	int ret = 0;

	pr_info("%s:%s\n", MFD_DEV_NAME, __func__);

	iodev = devm_kzalloc(&i2c->dev, sizeof(struct s2mpb03_dev), GFP_KERNEL);
	if (!iodev) {
		dev_err(&i2c->dev, "%s: Failed to alloc mem for s2mpb03\n",
							__func__);
		return -ENOMEM;
	}

	if (i2c->dev.of_node) {
		pdata = devm_kzalloc(&i2c->dev,
			sizeof(struct s2mpb03_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&i2c->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_pdata;
		}
		ret = s2mpb03_pmic_dt_parse_pdata(&i2c->dev, pdata);
		if (ret < 0) {
			dev_err(&i2c->dev, "Failed to get device of_node\n");
			goto err_pdata;
		}

		i2c->dev.platform_data = pdata;
	} else
		pdata = i2c->dev.platform_data;

	iodev->dev = &i2c->dev;
	iodev->i2c = i2c;

	if (pdata) {
		iodev->pdata = pdata;
		iodev->wakeup = pdata->wakeup;
	} else {
		ret = -EINVAL;
		goto err_pdata;
	}
	mutex_init(&iodev->i2c_lock);
	i2c_set_clientdata(i2c, iodev);

	s2mpb03 = devm_kzalloc(&i2c->dev, sizeof(struct s2mpb03_data),
				GFP_KERNEL);
	if (!s2mpb03) {
		pr_info("[%s:%d] if (!s2mpb03)\n", __FILE__, __LINE__);
		ret = -ENOMEM;
		goto err_s2mpb03_data;
	}

	s2mpb03->iodev = iodev;
	s2mpb03->num_regulators = pdata->num_regulators;

	for (i = 0; i < pdata->num_regulators; i++) {
		int id = pdata->regulators[i].id;
		config.dev = &i2c->dev;
		config.init_data = pdata->regulators[i].initdata;
		config.driver_data = s2mpb03;
		config.of_node = pdata->regulators[i].reg_node;
		s2mpb03->opmode[id] = regulators[id].enable_mask;
		s2mpb03->rdev[i] = devm_regulator_register(&i2c->dev, &regulators[id], &config);
		if (IS_ERR(s2mpb03->rdev[i])) {
			ret = PTR_ERR(s2mpb03->rdev[i]);
			dev_err(&i2c->dev, "regulator init failed for %d\n",
				id);
			s2mpb03->rdev[i] = NULL;
			goto err_s2mpb03_data;
		}
	}

	return ret;

err_s2mpb03_data:
	mutex_destroy(&iodev->i2c_lock);
err_pdata:
	pr_info("[%s:%d] err\n", __func__, __LINE__);

	return ret;
}

#if defined(CONFIG_OF)
static struct of_device_id s2mpb03_i2c_dt_ids[] = {
	{ .compatible = "samsung,s2mpb03pmic" },
	{ },
};
#endif /* CONFIG_OF */

static int s2mpb03_pmic_remove(struct i2c_client *i2c)
{
	dev_info(&i2c->dev, "%s\n", __func__);
	return 0;
}

#if defined(CONFIG_OF)
static const struct i2c_device_id s2mpb03_pmic_id[] = {
	{"s2mpb03-regulator", 0},
	{},
};
#endif

static struct i2c_driver s2mpb03_i2c_driver = {
	.driver = {
		.name = "s2mpb03-regulator",
		.owner = THIS_MODULE,
#if defined(CONFIG_OF)
		.of_match_table	= s2mpb03_i2c_dt_ids,
#endif /* CONFIG_OF */
		.suppress_bind_attrs = true,
	},
	.probe = s2mpb03_pmic_probe,
	.remove = s2mpb03_pmic_remove,
	.id_table = s2mpb03_pmic_id,
};

static int __init s2mpb03_i2c_init(void)
{
	pr_info("%s:%s\n", MFD_DEV_NAME, __func__);
	return i2c_add_driver(&s2mpb03_i2c_driver);
}
subsys_initcall(s2mpb03_i2c_init);

static void __exit s2mpb03_i2c_exit(void)
{
	i2c_del_driver(&s2mpb03_i2c_driver);
}
module_exit(s2mpb03_i2c_exit);

MODULE_DESCRIPTION("SAMSUNG s2mpb03 Regulator Driver");
MODULE_LICENSE("GPL");
