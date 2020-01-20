/*
 *  da9155_charger.c
 *  Samsung da9155 Charger Driver
 *
 *  Copyright (C) 2015 Samsung Electronics
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "include/charger/da9155_charger.h"
#include <linux/of.h>
#include <linux/of_gpio.h>

#define DEBUG

#define ENABLE 1
#define DISABLE 0
static void da9155_set_charge_current(struct da9155_charger_data *charger,
	int charge_current);

static enum power_supply_property da9155_charger_props[] = {
};

static int da9155_read_reg(struct i2c_client *client, u8 reg, u8 *dest)
{
	struct da9155_charger_data *charger = i2c_get_clientdata(client);
	int ret = 0;

	mutex_lock(&charger->io_lock);
	ret = i2c_smbus_read_byte_data(client, reg);
	mutex_unlock(&charger->io_lock);

	if (ret < 0) {
		pr_err("%s: can't read reg(0x%x), ret(%d)\n", __func__, reg, ret);
		return ret;
	}

	reg &= 0xFF;
	*dest = ret;

	return 0;
}

static int da9155_write_reg(struct i2c_client *client, u8 reg, u8 data)
{
	struct da9155_charger_data *charger = i2c_get_clientdata(client);
	int ret = 0;

	mutex_lock(&charger->io_lock);
	ret = i2c_smbus_write_byte_data(client, reg, data);
	mutex_unlock(&charger->io_lock);

	if (ret < 0)
		pr_err("%s: can't write reg(0x%x), ret(%d)\n", __func__, reg, ret);

	return ret;
}

static int da9155_update_reg(struct i2c_client *client, u8 reg, u8 val, u8 mask)
{
	struct da9155_charger_data *charger = i2c_get_clientdata(client);
	int ret = 0;

	mutex_lock(&charger->io_lock);
	ret = i2c_smbus_read_byte_data(client, reg);

	if (ret < 0)
		pr_err("%s: can't update reg(0x%x), ret(%d)\n", __func__, reg, ret);
	else {
		u8 old_val = ret & 0xFF;
		u8 new_val = (val & mask) | (old_val & (~mask));
		ret = i2c_smbus_write_byte_data(client, reg, new_val);
	}
	mutex_unlock(&charger->io_lock);

	return ret;
}

static void da9155_charger_test_read(struct da9155_charger_data *charger)
{
	u8 data = 0;
	u32 addr = 0;
	char str[1024]={0,};
	for (addr = 0x01; addr <= 0x13; addr++) {
		da9155_read_reg(charger->i2c, addr, &data);
		sprintf(str + strlen(str), "[0x%02x]0x%02x, ", addr, data);
	}
	pr_info("DA9155 : %s\n", str);
}

static int da9155_get_charger_state(struct da9155_charger_data *charger)
{
	u8 reg_data;

	da9155_read_reg(charger->i2c, DA9155_STATUS_B, &reg_data);
	if (reg_data & DA9155_MODE_MASK)
		return POWER_SUPPLY_STATUS_CHARGING;
	return POWER_SUPPLY_STATUS_NOT_CHARGING;
}

static int da9155_get_charger_health(struct da9155_charger_data *charger)
{
	u8 reg_data;

	// 80s same with maxim IC
	if (da9155_write_reg(charger->i2c, DA9155_TIMER_B, 0x50) < 0)
	{
		dev_info(charger->dev,
				"%s: addr: 0x%x write fail\n", __func__, DA9155_TIMER_B);
	}

	da9155_read_reg(charger->i2c, DA9155_STATUS_A, &reg_data);
	if (reg_data & DA9155_S_VIN_UV_MASK) {
		pr_info("%s: VIN undervoltage\n", __func__);
		return POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
	} else if (reg_data & DA9155_S_VIN_DROP_MASK) {
		pr_info("%s: VIN DROP\n", __func__);
		return POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
	} else if (reg_data & DA9155_S_VIN_OV_MASK) {
		pr_info("%s: VIN overvoltage\n", __func__);
		return POWER_SUPPLY_HEALTH_OVERVOLTAGE;
	} else
		return POWER_SUPPLY_HEALTH_GOOD;
}

static int da9155_get_charge_current(struct da9155_charger_data *charger)
{
	u8 reg_data;
	int charge_current;

	da9155_read_reg(charger->i2c, DA9155_BUCK_IOUT, &reg_data);
	charge_current = reg_data * 10 + 250;

	return charge_current;
}

static void da9155_charger_initialize(struct da9155_charger_data *charger)
{
	pr_info("%s\n", __func__);

	/* clear event reg */
	da9155_update_reg(charger->i2c, DA9155_EVENT_A, 0xFF, 0xFF);
	da9155_update_reg(charger->i2c, DA9155_EVENT_B, 0xFF, 0xFF);

	/* Safety timer enable */
	da9155_update_reg(charger->i2c, DA9155_CONTROL_E, 0, DA9155_TIMER_DIS_MASK);

	/* VBAT_OV: MAX(5V) */
	da9155_update_reg(charger->i2c, DA9155_CONTROL_C, 0x38, DA9155_VBAT_OV_MASK);
}

static void da9155_set_charger_state(struct da9155_charger_data *charger,
	int enable)
{
	u8 status = 0;
	int current_setting = 0;

	pr_info("%s: BUCK_EN(%s)\n", enable > 0 ? "ENABLE" : "DISABLE", __func__);

	if (enable){
		da9155_charger_initialize(charger);
		da9155_read_reg(charger->i2c, DA9155_STATUS_A, &status);
		if (status & 0x7E ){
			pr_info("%s: STATUS_A(0x%X)\n", __func__, status);
			return;
		}

		current_setting = da9155_get_charge_current(charger);
		da9155_set_charge_current(charger, 500);
		da9155_update_reg(charger->i2c, DA9155_BUCK_CONT, DA9155_BUCK_EN_MASK, DA9155_BUCK_EN_MASK);
		msleep(10);
		if (current_setting > 500)
			da9155_set_charge_current(charger, current_setting);
	}
	else {
		current_setting = da9155_get_charge_current(charger);
		if (current_setting > 700) {
			da9155_set_charge_current(charger, 700);
			msleep(100);
		}
		da9155_set_charge_current(charger, 500);
		msleep(100);
		da9155_update_reg(charger->i2c, DA9155_BUCK_CONT, 0, DA9155_BUCK_EN_MASK);
	}
}

#if 0
static void da9155_set_input_current(struct da9155_charger_data *charger,
	int input_current)
{
	u8 reg_data;

	reg_data = (input_current - 3000) / 100;

	da9155_update_reg(charger->i2c, DA9155_BUCK_ILIM, reg_data, DA9155_BUCK_ILIM_MASK);
	pr_info("%s: input_current(%d)\n", __func__, input_current);
}
#endif

static void da9155_set_charge_current(struct da9155_charger_data *charger,
	int charge_current)
{
	u8 reg_data;

	if (!charge_current) {
		reg_data = 0x00;
	} else {
		charge_current = (charge_current > 2500) ? 2500 : charge_current;
		reg_data = (charge_current - 250) / 10;
	}

	da9155_update_reg(charger->i2c, DA9155_BUCK_IOUT, reg_data, DA9155_BUCK_IOUT_MASK);
	pr_info("%s: charge_current(%d)\n", __func__, charge_current);
}

/* return : 0 (AFC type or High Voltage( > 6V), 1 : 5V ) */
static int da9155_check_cable_type(unsigned int type)
{
	if (type == SEC_BATTERY_CABLE_NONE)
		return -1;

	// AFC or High Voltage charger (6V > )
	if (type == SEC_BATTERY_CABLE_9V_TA ||	\
	    type == SEC_BATTERY_CABLE_12V_TA) {
		return 0;
	}
	else {
		return 1;
	}

	return -1;
}

static void da9155_set_vindrop_vbatov(struct da9155_charger_data *charger)
{
	/* VBAT OV Enable */
	da9155_write_reg(charger->i2c, DA9155_PAGE_CTRL_0, 0x06);
	da9155_write_reg(charger->i2c, 0x39, 0x05);
	da9155_write_reg(charger->i2c, 0x19, 0x0);
	da9155_write_reg(charger->i2c, 0x39, 0x0);
	da9155_write_reg(charger->i2c, DA9155_PAGE_CTRL_0, 0x0);

	/* set VIN_DROP to 4.5V */
	da9155_write_reg(charger->i2c, DA9155_CONTROL_A, 0x4);
}

static void da9155_mode_change(struct da9155_charger_data *charger, u8 enable)
{
	if(charger->i2c) {
		pr_info("%s: mode(%d), prev_cable_type(%d), cable_type(%d)\n",
			__func__, enable,charger->prev_cable_type, charger->cable_type);
		if (enable == ENABLE) {
			da9155_write_reg(charger->i2c, DA9155_PAGE_CTRL_0, 0x06);
			da9155_write_reg(charger->i2c, 0x39, 0x05);
			da9155_write_reg(charger->i2c, 0x11, 0x04);
			da9155_write_reg(charger->i2c, 0x39, 0x0);
			da9155_write_reg(charger->i2c, DA9155_PAGE_CTRL_0, 0x0);
		} else if (enable == DISABLE) {
			if((charger->prev_cable_type == SEC_BATTERY_CABLE_9V_TA ||
				charger->prev_cable_type == SEC_BATTERY_CABLE_12V_TA) &&
				charger->cable_type == SEC_BATTERY_CABLE_NONE)
					return;

			da9155_write_reg(charger->i2c, DA9155_PAGE_CTRL_0, 0x06);
			da9155_write_reg(charger->i2c, 0x39, 0x05);
			da9155_write_reg(charger->i2c, 0x11, 0x0);
			if(charger->prev_cable_type != SEC_BATTERY_CABLE_9V_TA &&
				charger->prev_cable_type != SEC_BATTERY_CABLE_12V_TA &&
				charger->prev_cable_type != SEC_BATTERY_CABLE_NONE &&
				charger->cable_type == SEC_BATTERY_CABLE_NONE)
					return;

			da9155_write_reg(charger->i2c, 0x39, 0x0);
			da9155_write_reg(charger->i2c, DA9155_PAGE_CTRL_0, 0x0);
		}
	}
}

static irqreturn_t da9155_irq_handler(int irq, void *data)
{
	struct da9155_charger_data *charger = data;
	u8 event_a, event_b;

	dev_info(charger->dev,
			"%s: \n", __func__);

	if (!da9155_read_reg(charger->i2c, DA9155_EVENT_A, &event_a) &&
			!da9155_read_reg(charger->i2c, DA9155_EVENT_B, &event_b)) {

		if ((event_b & 0x1) && (charger->cable_type != SEC_BATTERY_CABLE_NONE)) {
			dev_info(charger->dev, "%s: E_RDY Event occured", __func__);
			da9155_mode_change(charger, da9155_check_cable_type(charger->cable_type));
			da9155_charger_initialize(charger);
			da9155_set_vindrop_vbatov(charger);
		}

		/* clear event reg */
		da9155_write_reg(charger->i2c, DA9155_EVENT_A, event_a);
		da9155_write_reg(charger->i2c, DA9155_EVENT_B, event_b);

		dev_info(charger->dev,
				"%s: EVENT_A(0x%x), EVENT_B(0x%x)\n",
				__func__, event_a, event_b);
	}

	return IRQ_HANDLED;
}

static int da9155_chg_get_property(struct power_supply *psy,
	enum power_supply_property psp, union power_supply_propval *val)
{
	struct da9155_charger_data *charger =
		power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = psp;

	switch (psp) {
	case POWER_SUPPLY_PROP_HEALTH:
		if (charger->cable_type == SEC_BATTERY_CABLE_9V_TA ||
			charger->cable_type == SEC_BATTERY_CABLE_12V_TA) {
			da9155_charger_test_read(charger);
			val->intval = da9155_get_charger_health(charger);
		} else
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = da9155_get_charger_state(charger);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = da9155_get_charge_current(charger);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		return -ENODATA;
	case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_CHECK_SLAVE_I2C:
			{
				u8 reg_data;
				val->intval = (da9155_read_reg(charger->i2c, DA9155_EVENT_B, &reg_data) == 0);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_CHECK_MULTI_CHARGE:
			val->intval = (da9155_get_charger_health(charger) == POWER_SUPPLY_HEALTH_GOOD) ?
				da9155_get_charger_state(charger) : POWER_SUPPLY_STATUS_NOT_CHARGING;
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int da9155_chg_set_property(struct power_supply *psy,
	enum power_supply_property psp, const union power_supply_propval *val)
{
	struct da9155_charger_data *charger =
		power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		charger->is_charging =
			(val->intval == SEC_BAT_CHG_MODE_CHARGING) ? ENABLE : DISABLE;
		da9155_set_charger_state(charger, charger->is_charging);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		charger->charging_current = val->intval;
		da9155_set_charge_current(charger, charger->charging_current);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		charger->siop_level = val->intval;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		charger->cable_type = val->intval;
		if (val->intval != SEC_BATTERY_CABLE_NONE) {
			da9155_mode_change(charger, da9155_check_cable_type(charger->cable_type));
			charger->prev_cable_type = charger->cable_type;
			da9155_charger_initialize(charger);
			da9155_set_vindrop_vbatov(charger);
		} else {
			da9155_mode_change(charger, DISABLE);
			charger->prev_cable_type = SEC_BATTERY_CABLE_NONE;
		}
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_CURRENT_FULL:
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
	case POWER_SUPPLY_PROP_HEALTH:
		return -ENODATA;
	default:
		return -EINVAL;
	}

	return 0;
}

static int da9155_charger_parse_dt(struct da9155_charger_data *charger,
	struct da9155_charger_platform_data *pdata)
{
	struct device_node *np = of_find_node_by_name(NULL, "da9155-charger");
	int ret = 0;

	if (!np) {
		pr_err("%s: np is NULL\n", __func__);
		return -1;
	} else {
		ret = of_get_named_gpio_flags(np, "da9155-charger,irq-gpio",
			0, NULL);
		if (ret < 0) {
			pr_err("%s: da9155-charger,irq-gpio is empty\n", __func__);
			pdata->irq_gpio = 0;
		} else {
			pdata->irq_gpio = ret;
			pr_info("%s: irq-gpio = %d\n", __func__, pdata->irq_gpio);
		}
	}
	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s np NULL\n", __func__);
	} else {
		ret = of_property_read_u32(np, "battery,chg_float_voltage",
					   &charger->float_voltage);
		if (ret) {
			pr_info("%s: battery,chg_float_voltage is Empty\n", __func__);
			charger->float_voltage = 42000;
		}
	}

	return ret;
}

static const struct power_supply_desc da9155_charger_power_supply_desc = {
	.name = "da9155-charger",
	.type = POWER_SUPPLY_TYPE_UNKNOWN,
	.properties = da9155_charger_props,
	.num_properties = ARRAY_SIZE(da9155_charger_props),
	.get_property = da9155_chg_get_property,
	.set_property = da9155_chg_set_property,
};

static int da9155_charger_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct device_node *of_node = client->dev.of_node;
	struct da9155_charger_data *charger;
	struct da9155_charger_platform_data *pdata = client->dev.platform_data;
	struct power_supply_config sub_charger_cfg = {};
	int ret = 0;

	pr_info("%s: DA9155 Charger Driver Loading\n", __func__);

	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (!charger) {
		pr_err("%s: Failed to allocate memory\n", __func__);
		return -ENOMEM;
	}

	mutex_init(&charger->io_lock);
	charger->dev = &client->dev;
	charger->i2c = client;
	if (of_node) {
		pdata = devm_kzalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			pr_err("%s: Failed to allocate memory\n", __func__);
			ret = -ENOMEM;
			goto err_nomem;
		}
		ret = da9155_charger_parse_dt(charger, pdata);
		if (ret < 0)
			goto err_parse_dt;
	}
	charger->pdata = pdata;
	i2c_set_clientdata(client, charger);

	/* da9155_charger_initialize(charger); */
	charger->cable_type = SEC_BATTERY_CABLE_NONE;
	charger->prev_cable_type = SEC_BATTERY_CABLE_NONE;
	sub_charger_cfg.drv_data = charger;
	charger->psy_chg = power_supply_register(charger->dev, &da9155_charger_power_supply_desc, &sub_charger_cfg);
	if (ret) {
		pr_err("%s: Failed to Register psy_chg\n", __func__);
		ret = -1;
		goto err_power_supply_register;
	}

	if (pdata->irq_gpio) {
		charger->chg_irq = gpio_to_irq(pdata->irq_gpio);

		ret = request_threaded_irq(charger->chg_irq, NULL,
			da9155_irq_handler,
			IRQF_TRIGGER_LOW | IRQF_ONESHOT,
			"da9155-irq", charger);
		if (ret < 0) {
			pr_err("%s: Failed to Request IRQ(%d)\n", __func__, ret);
			goto err_req_irq;
		}
	}
	device_init_wakeup(charger->dev, 1);

	pr_info("%s: DA9155 Charger Driver Loaded\n", __func__);

	return 0;

err_req_irq:
	power_supply_unregister(charger->psy_chg);
err_power_supply_register:
err_parse_dt:
	kfree(pdata);
err_nomem:
	mutex_destroy(&charger->io_lock);
	kfree(charger);

	return ret;
}

static int da9155_charger_remove(struct i2c_client *client)
{
	struct da9155_charger_data *charger = i2c_get_clientdata(client);

	if (charger->chg_irq)
		free_irq(charger->chg_irq, charger);
	power_supply_unregister(charger->psy_chg);
	mutex_destroy(&charger->io_lock);
	kfree(charger->pdata);
	kfree(charger);

	return 0;
}

static void da9155_charger_shutdown(struct i2c_client *client)
{
	struct da9155_charger_data *charger = i2c_get_clientdata(client);

	if (charger->chg_irq)
		free_irq(charger->chg_irq, charger);
	da9155_update_reg(client, DA9155_BUCK_CONT, 0, DA9155_BUCK_EN_MASK);
	da9155_update_reg(client, DA9155_BUCK_IOUT, 0x7D, DA9155_BUCK_ILIM_MASK);
	if (charger->cable_type != SEC_BATTERY_CABLE_NONE)
		da9155_mode_change(charger, DISABLE);
}

static const struct i2c_device_id da9155_charger_id_table[] = {
	{"da9155-charger", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, da9155_id_table);

#ifdef CONFIG_OF
static struct of_device_id da9155_charger_match_table[] = {
	{.compatible = "dlg,da9155-charger"},
	{},
};
#else
#define da9155_charger_match_table NULL
#endif

#if defined(CONFIG_PM)
static int da9155_charger_suspend(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct da9155_charger_data *charger = i2c_get_clientdata(i2c);

	if (charger->chg_irq) {
		if (device_may_wakeup(dev))
			enable_irq_wake(charger->chg_irq);
		disable_irq(charger->chg_irq);
	}

	return 0;
}

static int da9155_charger_resume(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct da9155_charger_data *charger = i2c_get_clientdata(i2c);

	if (charger->chg_irq) {
		if (device_may_wakeup(dev))
			disable_irq_wake(charger->chg_irq);
		enable_irq(charger->chg_irq);
	}

	return 0;
}
#else
#define da9155_charger_suspend		NULL
#define da9155_charger_resume		NULL
#endif /* CONFIG_PM */

const struct dev_pm_ops da9155_pm = {
	.suspend = da9155_charger_suspend,
	.resume = da9155_charger_resume,
};

static struct i2c_driver da9155_charger_driver = {
	.driver = {
		.name	= "da9155-charger",
		.owner	= THIS_MODULE,
#if defined(CONFIG_PM)
		.pm	= &da9155_pm,
#endif /* CONFIG_PM */
		.of_match_table = da9155_charger_match_table,
	},
	.probe		= da9155_charger_probe,
	.remove		= da9155_charger_remove,
	.shutdown	= da9155_charger_shutdown,
	.id_table	= da9155_charger_id_table,
};

static int __init da9155_charger_init(void)
{
	pr_info("%s: \n", __func__);
	return i2c_add_driver(&da9155_charger_driver);
}

static void __exit da9155_charger_exit(void)
{
	pr_info("%s: \n", __func__);
	i2c_del_driver(&da9155_charger_driver);
}

module_init(da9155_charger_init);
module_exit(da9155_charger_exit);

MODULE_DESCRIPTION("Samsung DA9155 Charger Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
