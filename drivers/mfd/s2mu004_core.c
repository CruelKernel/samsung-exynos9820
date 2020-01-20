/*
 * s2mu004.c - mfd core driver for the s2mu004
 *
 * Copyright (C) 2015 Samsung Electronics
 *
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
 * This driver is based on max77843.c
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/mfd/core.h>
#include <linux/mfd/samsung/s2mu004.h>
#include <linux/mfd/samsung/s2mu004-private.h>
#include <linux/regulator/machine.h>

#if defined(CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif /* CONFIG_OF */

static struct mfd_cell s2mu004_devs[] = {
#if defined(CONFIG_MUIC_S2MU004)
	{ .name = "s2mu004-muic", },
#endif /* CONFIG_MUIC_S2MU004 */
#if defined(CONFIG_AFC_S2MU004)
	{ .name = "s2mu004-afc", },
#endif /* CONFIG_AFC_S2MU004 */
#if defined(CONFIG_REGULATOR_S2MU004)
	{ .name = "s2mu004-safeout", },
#endif /* CONFIG_REGULATOR_S2MU004 */
#if defined(CONFIG_CHARGER_S2MU004)
	{ .name = "s2mu004-charger", },
#endif
#if defined(CONFIG_BATTERY_S2MU00X)
	{ .name = "s2mu00x-battery", },
#endif
#if 0 // defined(CONFIG_MOTOR_DRV_S2MU004)
	{ .name = "s2mu004-haptic", },
#endif /* CONFIG_S2MU004_HAPTIC */
#if defined(CONFIG_LEDS_S2MU004_RGB)
	{ .name = "leds-s2mu004-rgb", },
#endif /* CONFIG_LEDS_S2MU004_RGB */
};

int s2mu004_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest)
{
	struct s2mu004_dev *s2mu004 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&s2mu004->i2c_lock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	mutex_unlock(&s2mu004->i2c_lock);
	if (ret < 0) {
		pr_info("%s:%s reg(0x%x), ret(%d)\n", MFD_DEV_NAME,
						 __func__, reg, ret);
		return ret;
	}

	ret &= 0xff;
	*dest = ret;
	return 0;
}
EXPORT_SYMBOL_GPL(s2mu004_read_reg);

int s2mu004_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct s2mu004_dev *s2mu004 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&s2mu004->i2c_lock);
	ret = i2c_smbus_read_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&s2mu004->i2c_lock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(s2mu004_bulk_read);

int s2mu004_read_word(struct i2c_client *i2c, u8 reg)
{
	struct s2mu004_dev *s2mu004 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&s2mu004->i2c_lock);
	ret = i2c_smbus_read_word_data(i2c, reg);
	mutex_unlock(&s2mu004->i2c_lock);
	if (ret < 0)
		return ret;

	return ret;
}
EXPORT_SYMBOL_GPL(s2mu004_read_word);

int s2mu004_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct s2mu004_dev *s2mu004 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&s2mu004->i2c_lock);
	ret = i2c_smbus_write_byte_data(i2c, reg, value);
	mutex_unlock(&s2mu004->i2c_lock);
	if (ret < 0)
		pr_info("%s:%s reg(0x%x), ret(%d)\n",
				MFD_DEV_NAME, __func__, reg, ret);

	return ret;
}
EXPORT_SYMBOL_GPL(s2mu004_write_reg);

int s2mu004_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct s2mu004_dev *s2mu004 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&s2mu004->i2c_lock);
	ret = i2c_smbus_write_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&s2mu004->i2c_lock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(s2mu004_bulk_write);

int s2mu004_write_word(struct i2c_client *i2c, u8 reg, u16 value)
{
	struct s2mu004_dev *s2mu004 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&s2mu004->i2c_lock);
	ret = i2c_smbus_write_word_data(i2c, reg, value);
	mutex_unlock(&s2mu004->i2c_lock);
	if (ret < 0)
		return ret;
	return 0;
}
EXPORT_SYMBOL_GPL(s2mu004_write_word);

int s2mu004_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	struct s2mu004_dev *s2mu004 = i2c_get_clientdata(i2c);
	int ret;
	u8 old_val, new_val;

	mutex_lock(&s2mu004->i2c_lock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret >= 0) {
		old_val = ret & 0xff;
		new_val = (val & mask) | (old_val & (~mask));
		ret = i2c_smbus_write_byte_data(i2c, reg, new_val);
	}
	mutex_unlock(&s2mu004->i2c_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(s2mu004_update_reg);

#if defined(CONFIG_OF)
static int of_s2mu004_dt(struct device *dev,
				struct s2mu004_platform_data *pdata)
{
	struct device_node *np_s2mu004 = dev->of_node;

	if (!np_s2mu004)
		return -EINVAL;

	pdata->irq_gpio = of_get_named_gpio(np_s2mu004, "s2mu004,irq-gpio", 0);
	pdata->wakeup = of_property_read_bool(np_s2mu004, "s2mu004,wakeup");

	pr_info("%s: irq-gpio: %u\n", __func__, pdata->irq_gpio);

	return 0;
}
#else
static int of_s2mu004_dt(struct device *dev,
				struct max77834_platform_data *pdata)
{
	return 0;
}
#endif /* CONFIG_OF */

static int s2mu004_i2c_probe(struct i2c_client *i2c,
				const struct i2c_device_id *dev_id)
{
	struct s2mu004_dev *s2mu004;
	struct s2mu004_platform_data *pdata = i2c->dev.platform_data;

	int ret = 0;
	u8 temp = 0;

	dev_info(&i2c->dev, "%s start\n", __func__);

	s2mu004 = kzalloc(sizeof(struct s2mu004_dev), GFP_KERNEL);
	if (!s2mu004) {
		dev_err(&i2c->dev, "%s: Failed to alloc mem for s2mu004\n",
				__func__);
		return -ENOMEM;
	}

	if (i2c->dev.of_node) {
		pdata = devm_kzalloc(&i2c->dev,
			 sizeof(struct s2mu004_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&i2c->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err;
		}

		ret = of_s2mu004_dt(&i2c->dev, pdata);
		if (ret < 0) {
			dev_err(&i2c->dev, "Failed to get device of_node\n");
			goto err;
		}

		i2c->dev.platform_data = pdata;
	} else
		pdata = i2c->dev.platform_data;

	s2mu004->dev = &i2c->dev;
	s2mu004->i2c = i2c;
	s2mu004->irq = i2c->irq;
	if (pdata) {
		s2mu004->pdata = pdata;

		pdata->irq_base = irq_alloc_descs(-1, 0, S2MU004_IRQ_NR, -1);
		if (pdata->irq_base < 0) {
			pr_err("%s:%s irq_alloc_descs Fail! ret(%d)\n",
				MFD_DEV_NAME, __func__, pdata->irq_base);
			ret = -EINVAL;
			goto err;
		} else
			s2mu004->irq_base = pdata->irq_base;

		s2mu004->irq_gpio = pdata->irq_gpio;
		s2mu004->wakeup = pdata->wakeup;
	} else {
		ret = -EINVAL;
		goto err;
	}
	mutex_init(&s2mu004->i2c_lock);

	i2c_set_clientdata(i2c, s2mu004);

	s2mu004_read_reg(s2mu004->i2c, S2MU004_REG_REV_ID, &temp);
	if (temp < 0)
		pr_err("[s2mu004 mfd] %s : i2c read error\n", __func__);

	s2mu004->pmic_ver = temp & 0x0F;
	pr_err("[s2mu004 mfd] %s : ver=0x%x\n", __func__, s2mu004->pmic_ver);

	ret = s2mu004_irq_init(s2mu004);

	if (ret < 0)
		goto err_irq_init;

	ret = mfd_add_devices(s2mu004->dev, -1, s2mu004_devs,
			ARRAY_SIZE(s2mu004_devs), NULL, 0, NULL);
	if (ret < 0)
		goto err_mfd;

	device_init_wakeup(s2mu004->dev, pdata->wakeup);

	return ret;

err_mfd:
	mutex_destroy(&s2mu004->i2c_lock);
	mfd_remove_devices(s2mu004->dev);
err_irq_init:
	i2c_unregister_device(s2mu004->i2c);
err:
	kfree(s2mu004);
	return ret;
}

static int s2mu004_i2c_remove(struct i2c_client *i2c)
{
	struct s2mu004_dev *s2mu004 = i2c_get_clientdata(i2c);

	mfd_remove_devices(s2mu004->dev);
	i2c_unregister_device(s2mu004->i2c);
	kfree(s2mu004);

	return 0;
}

static const struct i2c_device_id s2mu004_i2c_id[] = {
	{ MFD_DEV_NAME, TYPE_S2MU004 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, s2mu004_i2c_id);

#if defined(CONFIG_OF)
static struct of_device_id s2mu004_i2c_dt_ids[] = {
	{.compatible = "samsung,s2mu004mfd"},
	{ },
};
#endif /* CONFIG_OF */

#if defined(CONFIG_PM)
static int s2mu004_suspend(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct s2mu004_dev *s2mu004 = i2c_get_clientdata(i2c);

	if (device_may_wakeup(dev))
		enable_irq_wake(s2mu004->irq);

	disable_irq(s2mu004->irq);

	return 0;
}

static int s2mu004_resume(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct s2mu004_dev *s2mu004 = i2c_get_clientdata(i2c);

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	pr_info("%s:%s\n", MFD_DEV_NAME, __func__);
#endif /* CONFIG_SAMSUNG_PRODUCT_SHIP */

	if (device_may_wakeup(dev))
		disable_irq_wake(s2mu004->irq);

	enable_irq(s2mu004->irq);

	return 0;
}
#else
#define s2mu004_suspend	NULL
#define s2mu004_resume	NULL
#endif /* CONFIG_PM */

#ifdef CONFIG_HIBERNATION
#if false
u8 s2mu004_dumpaddr_pmic[] = {
#if 0
	s2mu004_LED_REG_IFLASH,
	s2mu004_LED_REG_IFLASH1,
	s2mu004_LED_REG_IFLASH2,
	s2mu004_LED_REG_ITORCH,
	s2mu004_LED_REG_ITORCHTORCHTIMER,
	s2mu004_LED_REG_FLASH_TIMER,
	s2mu004_LED_REG_FLASH_EN,
	s2mu004_LED_REG_MAX_FLASH1,
	s2mu004_LED_REG_MAX_FLASH2,
	s2mu004_LED_REG_VOUT_CNTL,
	s2mu004_LED_REG_VOUT_FLASH,
	s2mu004_LED_REG_VOUT_FLASH1,
	s2mu004_LED_REG_FLASH_INT_STATUS,
#endif
	s2mu004_PMIC_REG_PMICID1,
	s2mu004_PMIC_REG_PMICREV,
	s2mu004_PMIC_REG_MAINCTRL1,
	s2mu004_PMIC_REG_MCONFIG,
};
#endif

u8 s2mu004_dumpaddr_muic[] = {
	s2mu004_MUIC_REG_INTMASK1,
	s2mu004_MUIC_REG_INTMASK2,
	s2mu004_MUIC_REG_INTMASK3,
	s2mu004_MUIC_REG_CDETCTRL1,
	s2mu004_MUIC_REG_CDETCTRL2,
	s2mu004_MUIC_REG_CTRL1,
	s2mu004_MUIC_REG_CTRL2,
	s2mu004_MUIC_REG_CTRL3,
};

#if false
u8 s2mu004_dumpaddr_haptic[] = {
	s2mu004_HAPTIC_REG_CONFIG1,
	s2mu004_HAPTIC_REG_CONFIG2,
	s2mu004_HAPTIC_REG_CONFIG_CHNL,
	s2mu004_HAPTIC_REG_CONFG_CYC1,
	s2mu004_HAPTIC_REG_CONFG_CYC2,
	s2mu004_HAPTIC_REG_CONFIG_PER1,
	s2mu004_HAPTIC_REG_CONFIG_PER2,
	s2mu004_HAPTIC_REG_CONFIG_PER3,
	s2mu004_HAPTIC_REG_CONFIG_PER4,
	s2mu004_HAPTIC_REG_CONFIG_DUTY1,
	s2mu004_HAPTIC_REG_CONFIG_DUTY2,
	s2mu004_HAPTIC_REG_CONFIG_PWM1,
	s2mu004_HAPTIC_REG_CONFIG_PWM2,
	s2mu004_HAPTIC_REG_CONFIG_PWM3,
	s2mu004_HAPTIC_REG_CONFIG_PWM4,
};
#endif

u8 s2mu004_dumpaddr_led[] = {
	s2mu004_RGBLED_REG_LEDEN,
	s2mu004_RGBLED_REG_LED0BRT,
	s2mu004_RGBLED_REG_LED1BRT,
	s2mu004_RGBLED_REG_LED2BRT,
	s2mu004_RGBLED_REG_LED3BRT,
	s2mu004_RGBLED_REG_LEDBLNK,
	s2mu004_RGBLED_REG_LEDRMP,
};

static int s2mu004_freeze(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct s2mu004_dev *s2mu004 = i2c_get_clientdata(i2c);
	int i;

	for (i = 0; i < ARRAY_SIZE(s2mu004_dumpaddr_pmic); i++)
		s2mu004_read_reg(i2c, s2mu004_dumpaddr_pmic[i],
				&s2mu004->reg_pmic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(s2mu004_dumpaddr_muic); i++)
		s2mu004_read_reg(i2c, s2mu004_dumpaddr_muic[i],
				&s2mu004->reg_muic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(s2mu004_dumpaddr_led); i++)
		s2mu004_read_reg(i2c, s2mu004_dumpaddr_led[i],
				&s2mu004->reg_led_dump[i]);

	disable_irq(s2mu004->irq);

	return 0;
}

static int s2mu004_restore(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct s2mu004_dev *s2mu004 = i2c_get_clientdata(i2c);
	int i;

	enable_irq(s2mu004->irq);

	for (i = 0; i < ARRAY_SIZE(s2mu004_dumpaddr_pmic); i++)
		s2mu004_write_reg(i2c, s2mu004_dumpaddr_pmic[i],
				s2mu004->reg_pmic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(s2mu004_dumpaddr_muic); i++)
		s2mu004_write_reg(i2c, s2mu004_dumpaddr_muic[i],
				s2mu004->reg_muic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(s2mu004_dumpaddr_led); i++)
		s2mu004_write_reg(i2c, s2mu004_dumpaddr_led[i],
				s2mu004->reg_led_dump[i]);

	return 0;
}
#endif

const struct dev_pm_ops s2mu004_pm = {
	.suspend = s2mu004_suspend,
	.resume = s2mu004_resume,
#ifdef CONFIG_HIBERNATION
	.freeze =  s2mu004_freeze,
	.thaw = s2mu004_restore,
	.restore = s2mu004_restore,
#endif
};

static struct i2c_driver s2mu004_i2c_driver = {
	.driver		= {
		.name	= MFD_DEV_NAME,
		.owner	= THIS_MODULE,
#if defined(CONFIG_PM)
		.pm	= &s2mu004_pm,
#endif /* CONFIG_PM */
#if defined(CONFIG_OF)
		.of_match_table	= s2mu004_i2c_dt_ids,
#endif /* CONFIG_OF */
	},
	.probe		= s2mu004_i2c_probe,
	.remove		= s2mu004_i2c_remove,
	.id_table	= s2mu004_i2c_id,
};

static int __init s2mu004_i2c_init(void)
{
	pr_info("%s:%s\n", MFD_DEV_NAME, __func__);
	return i2c_add_driver(&s2mu004_i2c_driver);
}
/* init early so consumer devices can complete system boot */
subsys_initcall(s2mu004_i2c_init);

static void __exit s2mu004_i2c_exit(void)
{
	i2c_del_driver(&s2mu004_i2c_driver);
}
module_exit(s2mu004_i2c_exit);

MODULE_DESCRIPTION("s2mu004 multi-function core driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
