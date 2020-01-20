/*
 * max77865.c - mfd core driver for the Maxim 77865
 *
 * Copyright (C) 2016 Samsung Electronics
 * Insun Choi <insun77.choi@samsung.com>
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
 * This driver is based on max8997.c
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/mfd/core.h>
#include <linux/mfd/max77865.h>
#include <linux/mfd/max77865-private.h>
#include <linux/regulator/machine.h>

#include <linux/muic/muic.h>
#include <linux/muic/max77865-muic-hv-typedef.h>
#include <linux/muic/max77865-muic.h>

#if defined (CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif /* CONFIG_OF */

#define I2C_ADDR_PMIC	(0x92 >> 1)	/* Top sys, Haptic */
#define I2C_ADDR_MUIC	(0x4A >> 1)
#define I2C_ADDR_CHG    (0xD2 >> 1)
#define I2C_ADDR_FG     (0x6C >> 1)

static struct mfd_cell max77865_devs[] = {
#if defined(CONFIG_MUIC_UNIVERSAL)
	{ .name = "muic-universal", },
#endif /* CONFIG_MUIC_UNIVERSAL */
#if defined(CONFIG_MUIC_MAX77865)
	{ .name = MUIC_DEV_NAME, },
#endif /* CONFIG_MUIC_MAX77865 */
#if defined(CONFIG_REGULATOR_MAX77865)
	{ .name = "max77865-safeout", },
#endif /* CONFIG_REGULATOR_MAX77865 */
#if defined(CONFIG_FUELGAUGE_MAX77865)
	{ .name = "max77865-fuelgauge", },
#endif
#if defined(CONFIG_CHARGER_MAX77865)
	{ .name = "max77865-charger", },
#endif
#if defined(CONFIG_MOTOR_DRV_MAX77865)
	{ .name = "max77865-haptic", },
#endif /* CONFIG_MAX77865_HAPTIC */
#if defined(CONFIG_LEDS_MAX77865_RGB)
	{ .name = "leds-max77865-rgb", },
#endif /* CONFIG_LEDS_MAX77865_RGB */
#if defined(CONFIG_LEDS_MAX77865_FLASH)
	{ .name = "max77865-flash", },
#endif
};

int max77865_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest)
{
	struct max77865_dev *max77865 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77865->i2c_lock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	mutex_unlock(&max77865->i2c_lock);
	if (ret < 0) {
		pr_info("%s:%s reg(0x%x), ret(%d)\n", MFD_DEV_NAME, __func__, reg, ret);
		return ret;
	}

	ret &= 0xff;
	*dest = ret;
	return 0;
}
EXPORT_SYMBOL_GPL(max77865_read_reg);

int max77865_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max77865_dev *max77865 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77865->i2c_lock);
	ret = i2c_smbus_read_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&max77865->i2c_lock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(max77865_bulk_read);

int max77865_read_word(struct i2c_client *i2c, u8 reg)
{
	struct max77865_dev *max77865 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77865->i2c_lock);
	ret = i2c_smbus_read_word_data(i2c, reg);
	mutex_unlock(&max77865->i2c_lock);
	if (ret < 0)
		return ret;

	return ret;
}
EXPORT_SYMBOL_GPL(max77865_read_word);

int max77865_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct max77865_dev *max77865 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77865->i2c_lock);
	ret = i2c_smbus_write_byte_data(i2c, reg, value);
	mutex_unlock(&max77865->i2c_lock);
	if (ret < 0)
		pr_info("%s:%s reg(0x%x), ret(%d)\n",
				MFD_DEV_NAME, __func__, reg, ret);

	return ret;
}
EXPORT_SYMBOL_GPL(max77865_write_reg);

int max77865_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max77865_dev *max77865 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77865->i2c_lock);
	ret = i2c_smbus_write_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&max77865->i2c_lock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(max77865_bulk_write);

int max77865_write_word(struct i2c_client *i2c, u8 reg, u16 value)
{
	struct max77865_dev *max77865 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77865->i2c_lock);
	ret = i2c_smbus_write_word_data(i2c, reg, value);
	mutex_unlock(&max77865->i2c_lock);
	if (ret < 0)
		return ret;
	return 0;
}
EXPORT_SYMBOL_GPL(max77865_write_word);

int max77865_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	struct max77865_dev *max77865 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77865->i2c_lock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret >= 0) {
		u8 old_val = ret & 0xff;
		u8 new_val = (val & mask) | (old_val & (~mask));
		ret = i2c_smbus_write_byte_data(i2c, reg, new_val);
	}
	mutex_unlock(&max77865->i2c_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(max77865_update_reg);

#if defined(CONFIG_OF)
static int of_max77865_dt(struct device *dev, struct max77865_platform_data *pdata)
{
	struct device_node *np_max77865 = dev->of_node;

	if(!np_max77865)
		return -EINVAL;

	pdata->irq_gpio = of_get_named_gpio(np_max77865, "max77865,irq-gpio", 0);
	pdata->wakeup = of_property_read_bool(np_max77865, "max77865,wakeup");

	pr_info("%s: irq-gpio: %u \n", __func__, pdata->irq_gpio);

	return 0;
}
#else
static int of_max77834_dt(struct device *dev, struct max77834_platform_data *pdata)
{
	return 0;
}
#endif /* CONFIG_OF */

static int max77865_i2c_probe(struct i2c_client *i2c,
				const struct i2c_device_id *dev_id)
{
	struct max77865_dev *max77865;
	struct max77865_platform_data *pdata = i2c->dev.platform_data;

	u8 reg_data;
	int ret = 0;

	pr_info("%s:%s\n", MFD_DEV_NAME, __func__);

	max77865 = kzalloc(sizeof(struct max77865_dev), GFP_KERNEL);
	if (!max77865) {
		dev_err(&i2c->dev, "%s: Failed to alloc mem for max77865\n", __func__);
		return -ENOMEM;
	}

	if (i2c->dev.of_node) {
		pdata = devm_kzalloc(&i2c->dev, sizeof(struct max77865_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			dev_err(&i2c->dev, "Failed to allocate memory \n");
			ret = -ENOMEM;
			goto err;
		}

		ret = of_max77865_dt(&i2c->dev, pdata);
		if (ret < 0){
			dev_err(&i2c->dev, "Failed to get device of_node \n");
			goto err;
		}

		i2c->dev.platform_data = pdata;
	} else
		pdata = i2c->dev.platform_data;

	max77865->dev = &i2c->dev;
	max77865->i2c = i2c;
	max77865->irq = i2c->irq;
	if (pdata) {
		max77865->pdata = pdata;

		pdata->irq_base = irq_alloc_descs(-1, 0, MAX77865_IRQ_NR, -1);
		if (pdata->irq_base < 0) {
			pr_err("%s:%s irq_alloc_descs Fail! ret(%d)\n",
					MFD_DEV_NAME, __func__, pdata->irq_base);
			ret = -EINVAL;
			goto err;
		} else
			max77865->irq_base = pdata->irq_base;

		max77865->irq_gpio = pdata->irq_gpio;
		max77865->wakeup = pdata->wakeup;
	} else {
		ret = -EINVAL;
		goto err;
	}
	mutex_init(&max77865->i2c_lock);

	i2c_set_clientdata(i2c, max77865);

	if (max77865_read_reg(i2c, MAX77865_PMIC_REG_PMICREV, &reg_data) < 0) {
		dev_err(max77865->dev,
			"device not found on this channel (this is not an error)\n");
		ret = -ENODEV;
		goto err_w_lock;
	} else {
		/* print rev */
		max77865->pmic_rev = (reg_data & 0x7);
		max77865->pmic_ver = ((reg_data & 0xF8) >> 0x3);
		pr_info("%s:%s device found: rev.0x%x, ver.0x%x\n",
				MFD_DEV_NAME, __func__,
				max77865->pmic_rev, max77865->pmic_ver);
	}

	/* No active discharge on safeout ldo 1,2 */
//	max77865_update_reg(i2c, MAX77865_PMIC_REG_SAFEOUT_CTRL, 0x00, 0x30);

	max77865->muic = i2c_new_dummy(i2c->adapter, I2C_ADDR_MUIC);
	i2c_set_clientdata(max77865->muic, max77865);

	max77865->charger = i2c_new_dummy(i2c->adapter, I2C_ADDR_CHG);
	i2c_set_clientdata(max77865->charger, max77865);

	max77865->fuelgauge = i2c_new_dummy(i2c->adapter, I2C_ADDR_FG);
	i2c_set_clientdata(max77865->fuelgauge, max77865);

	ret = max77865_irq_init(max77865);

	if (ret < 0)
		goto err_irq_init;

	ret = mfd_add_devices(max77865->dev, -1, max77865_devs,
			ARRAY_SIZE(max77865_devs), NULL, 0, NULL);
	if (ret < 0)
		goto err_mfd;

	device_init_wakeup(max77865->dev, pdata->wakeup);

	/* Enable booster 5.2v for vibrator */
	max77865_update_reg(i2c, MAX77865_PMIC_REG_BOOSTCONTROL1, 0x00, MAX77865_PMIC_REG_BSTOUT_MASK);
	max77865_update_reg(i2c, MAX77865_PMIC_REG_BOOSTCONTROL2, 0x08, MAX77865_PMIC_REG_FORCE_EN_MASK);

	return ret;

err_mfd:
	mfd_remove_devices(max77865->dev);
err_irq_init:
	i2c_unregister_device(max77865->muic);
err_w_lock:
	mutex_destroy(&max77865->i2c_lock);
err:
	kfree(max77865);
	return ret;
}

static int max77865_i2c_remove(struct i2c_client *i2c)
{
	struct max77865_dev *max77865 = i2c_get_clientdata(i2c);

	mfd_remove_devices(max77865->dev);
	i2c_unregister_device(max77865->muic);
	kfree(max77865);

	return 0;
}

static const struct i2c_device_id max77865_i2c_id[] = {
	{ MFD_DEV_NAME, TYPE_MAX77865 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max77865_i2c_id);

#if defined(CONFIG_OF)
static struct of_device_id max77865_i2c_dt_ids[] = {
	{ .compatible = "maxim,max77865" },
	{ },
};
MODULE_DEVICE_TABLE(of, max77865_i2c_dt_ids);
#endif /* CONFIG_OF */

#if defined(CONFIG_PM)
static int max77865_suspend(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77865_dev *max77865 = i2c_get_clientdata(i2c);

	pr_info("%s:%s\n", MFD_DEV_NAME, __func__);

	if (device_may_wakeup(dev))
		enable_irq_wake(max77865->irq);

	disable_irq(max77865->irq);

	return 0;
}

static int max77865_resume(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77865_dev *max77865 = i2c_get_clientdata(i2c);

	pr_info("%s:%s\n", MFD_DEV_NAME, __func__);

	if (device_may_wakeup(dev))
		disable_irq_wake(max77865->irq);

	enable_irq(max77865->irq);

	return 0;
}
#else
#define max77865_suspend	NULL
#define max77865_resume		NULL
#endif /* CONFIG_PM */

#ifdef CONFIG_HIBERNATION

/*
u8 max77865_dumpaddr_pmic[] = {
#if 0
	MAX77865_LED_REG_IFLASH,
	MAX77865_LED_REG_IFLASH1,
	MAX77865_LED_REG_IFLASH2,
	MAX77865_LED_REG_ITORCH,
	MAX77865_LED_REG_ITORCHTORCHTIMER,
	MAX77865_LED_REG_FLASH_TIMER,
	MAX77865_LED_REG_FLASH_EN,
	MAX77865_LED_REG_MAX_FLASH1,
	MAX77865_LED_REG_MAX_FLASH2,
	MAX77865_LED_REG_VOUT_CNTL,
	MAX77865_LED_REG_VOUT_FLASH,
	MAX77865_LED_REG_VOUT_FLASH1,
	MAX77865_LED_REG_FLASH_INT_STATUS,
#endif
	MAX77865_PMIC_REG_PMICID1,
	MAX77865_PMIC_REG_PMICREV,
	MAX77865_PMIC_REG_MAINCTRL1,
	MAX77865_PMIC_REG_MCONFIG,
};
*/

u8 max77865_dumpaddr_muic[] = {
	MAX77865_MUIC_REG_INTMASK_MAIN,
	MAX77865_MUIC_REG_INTMASK_BC,
	MAX77865_MUIC_REG_INTMASK_FC,
	MAX77865_MUIC_REG_INTMASK_GP,
	MAX77865_MUIC_REG_STATUS1_BC,
	MAX77865_MUIC_REG_STATUS2_BC,
	MAX77865_MUIC_REG_STATUS_GP,
	MAX77865_MUIC_REG_CONTROL1_BC,
	MAX77865_MUIC_REG_CONTROL2_BC,
	MAX77865_MUIC_REG_CONTROL1,
	MAX77865_MUIC_REG_CONTROL2,
	MAX77865_MUIC_REG_CONTROL3,
	MAX77865_MUIC_REG_CONTROL4,
	MAX77865_MUIC_REG_HVCONTROL1,
	MAX77865_MUIC_REG_HVCONTROL2,
};

/*
u8 max77865_dumpaddr_haptic[] = {
	MAX77865_HAPTIC_REG_CONFIG1,
	MAX77865_HAPTIC_REG_CONFIG2,
	MAX77865_HAPTIC_REG_CONFIG_CHNL,
	MAX77865_HAPTIC_REG_CONFG_CYC1,
	MAX77865_HAPTIC_REG_CONFG_CYC2,
	MAX77865_HAPTIC_REG_CONFIG_PER1,
	MAX77865_HAPTIC_REG_CONFIG_PER2,
	MAX77865_HAPTIC_REG_CONFIG_PER3,
	MAX77865_HAPTIC_REG_CONFIG_PER4,
	MAX77865_HAPTIC_REG_CONFIG_DUTY1,
	MAX77865_HAPTIC_REG_CONFIG_DUTY2,
	MAX77865_HAPTIC_REG_CONFIG_PWM1,
	MAX77865_HAPTIC_REG_CONFIG_PWM2,
	MAX77865_HAPTIC_REG_CONFIG_PWM3,
	MAX77865_HAPTIC_REG_CONFIG_PWM4,
};
*/

u8 max77865_dumpaddr_led[] = {
	MAX77865_RGBLED_REG_LEDEN,
	MAX77865_RGBLED_REG_LED0BRT,
	MAX77865_RGBLED_REG_LED1BRT,
	MAX77865_RGBLED_REG_LED2BRT,
	MAX77865_RGBLED_REG_LED3BRT,
	MAX77865_RGBLED_REG_LEDBLNK,
	MAX77865_RGBLED_REG_LEDRMP,
};

static int max77865_freeze(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77865_dev *max77865 = i2c_get_clientdata(i2c);
	int i;

	for (i = 0; i < ARRAY_SIZE(max77865_dumpaddr_pmic); i++)
		max77865_read_reg(i2c, max77865_dumpaddr_pmic[i],
				&max77865->reg_pmic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77865_dumpaddr_muic); i++)
		max77865_read_reg(i2c, max77865_dumpaddr_muic[i],
				&max77865->reg_muic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77865_dumpaddr_led); i++)
		max77865_read_reg(i2c, max77865_dumpaddr_led[i],
				&max77865->reg_led_dump[i]);

	disable_irq(max77865->irq);

	return 0;
}

static int max77865_restore(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77865_dev *max77865 = i2c_get_clientdata(i2c);
	int i;

	enable_irq(max77865->irq);

	for (i = 0; i < ARRAY_SIZE(max77865_dumpaddr_pmic); i++)
		max77865_write_reg(i2c, max77865_dumpaddr_pmic[i],
				max77865->reg_pmic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77865_dumpaddr_muic); i++)
		max77865_write_reg(i2c, max77865_dumpaddr_muic[i],
				max77865->reg_muic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77865_dumpaddr_led); i++)
		max77865_write_reg(i2c, max77865_dumpaddr_led[i],
				max77865->reg_led_dump[i]);

	return 0;
}
#endif

const struct dev_pm_ops max77865_pm = {
	.suspend = max77865_suspend,
	.resume = max77865_resume,
#ifdef CONFIG_HIBERNATION
	.freeze =  max77865_freeze,
	.thaw = max77865_restore,
	.restore = max77865_restore,
#endif
};

static struct i2c_driver max77865_i2c_driver = {
	.driver		= {
		.name	= MFD_DEV_NAME,
		.owner	= THIS_MODULE,
#if defined(CONFIG_PM)
		.pm	= &max77865_pm,
#endif /* CONFIG_PM */
#if defined(CONFIG_OF)
		.of_match_table	= max77865_i2c_dt_ids,
#endif /* CONFIG_OF */
	},
	.probe		= max77865_i2c_probe,
	.remove		= max77865_i2c_remove,
	.id_table	= max77865_i2c_id,
};

static int __init max77865_i2c_init(void)
{
	pr_info("%s:%s\n", MFD_DEV_NAME, __func__);
	return i2c_add_driver(&max77865_i2c_driver);
}
/* init early so consumer devices can complete system boot */
subsys_initcall(max77865_i2c_init);

static void __exit max77865_i2c_exit(void)
{
	i2c_del_driver(&max77865_i2c_driver);
}
module_exit(max77865_i2c_exit);

MODULE_DESCRIPTION("MAXIM 77865 multi-function core driver");
MODULE_AUTHOR("Insun Choi <insun77.choi@samsung.com>");
MODULE_LICENSE("GPL");
