/*
 * driver/ccic/s2mm005.c - S2MM005 USBPD device driver
 *
 * Copyright (C) 2017 Samsung Electronics
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#include <linux/ccic/core.h>
#include <linux/ccic/s2mm005_usbpd.h>
#include <linux/ccic/s2mm005_usbpd_phy.h>
#include <linux/ccic/s2mm005_usbpd_fw.h>
#include <linux/ccic/usbpd_sysfs.h>
#include <linux/usb_notify.h>
#include <linux/sec_class.h>

static struct usbpd_dev *s2mm005_usbpd_desc_register(struct s2mm005_data *usbpd_data,
								   char *name)
{
	struct usbpd_desc *desc;
	struct usbpd_dev *udev;

	desc = kzalloc(sizeof(struct usbpd_desc), GFP_KERNEL);
	if (desc == NULL) {
		dev_err(usbpd_data->dev, "%s, alloc usbpd desc error\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	desc->name = name;
	s2mm005_usbpd_ops_register(desc);

	udev = usbpd_register(usbpd_data->dev, desc, usbpd_data);
	if (udev == NULL) {
		dev_err(usbpd_data->dev, "%s, register usbpd desc error\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	return udev;
}

static irqreturn_t s2mm005_usbpd_irq_thread(int irq, void *data)
{
	struct s2mm005_data *usbpd_data = data;
	struct i2c_client *i2c = usbpd_data->i2c;

	dev_info(&i2c->dev, "%d times irq num(%d)\n", ++usbpd_data->wq_times, irq);

	wake_lock_timeout(&usbpd_data->wlock, HZ);

	if (s2mm005_fw_ver_check(usbpd_data) == CCIC_FW_VERSION_INVALID) {
		goto ver_err;
	}

	s2mm005_usbpd_check_pd_state(usbpd_data);

ver_err:
	return IRQ_HANDLED;
}

static void s2mm005_usbpd_configure_init(struct s2mm005_data *_data)
{
	_data->p_prev_rid = -1;
	_data->prev_rid = -1;
	_data->cur_rid = RID_UNDEFINED;
	_data->is_dr_swap = 0;
	_data->is_pr_swap = 0;
	_data->pd_state = 0;
	_data->func_state = 0;
	_data->data_role = 0;
	_data->manual_lpm_mode = 0;
	_data->water_det = 0;
	_data->run_dry = 1;
	_data->booting_run_dry = 1;
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
	_data->try_state_change = 0;
#endif
#if defined(CONFIG_SEC_FACTORY)
	_data->fac_water_enable = 0;
#endif
	_data->cnt = 0;
}

static int s2mm005_usbpd_irq_init(struct s2mm005_data *_data)
{
	struct i2c_client *i2c = _data->i2c;
	struct device *dev = &i2c->dev;
	int ret = 0;

	if (!_data->irq_gpio) {
		dev_err(dev, "%s No interrupt specified\n", __func__);
		return -ENXIO;
	}

	i2c->irq = gpio_to_irq(_data->irq_gpio);

	if (i2c->irq) {
		ret = request_threaded_irq(i2c->irq, NULL,
				s2mm005_usbpd_irq_thread,
				(IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND | IRQF_ONESHOT),
				"s2mm005-usbpd", _data);
		if (ret < 0) {
			dev_err(dev, "%s failed to request irq(%d)\n",
					__func__, i2c->irq);
			return ret;
		}
	}

	return ret;
}

#if defined(CONFIG_OF)
static int of_s2mm005_usbpd_dt(struct device *dev,
			       struct s2mm005_data *usbpd_data)
{
	struct device_node *np = dev->of_node;
	int ret;

	usbpd_data->irq_gpio = of_get_named_gpio(np, "usbpd,usbpd_int", 0);
	usbpd_data->redriver_en = of_get_named_gpio(np, "usbpd,redriver_en", 0);

	usbpd_data->s2mm005_om = of_get_named_gpio(np, "usbpd,s2mm005_om", 0);
	usbpd_data->s2mm005_sda = of_get_named_gpio(np, "usbpd,s2mm005_sda", 0);
	usbpd_data->s2mm005_scl = of_get_named_gpio(np, "usbpd,s2mm005_scl", 0);

	np = of_find_all_nodes(NULL);
	ret = of_property_read_u32(np, "model_info-hw_rev", &usbpd_data->hw_rev);
	if (ret) {
		pr_info("%s: model_info-hw_rev is Empty\n", __func__);
		usbpd_data->hw_rev = 0;
	}

	dev_err(dev, "hw_rev:%02d usbpd_irq = %d redriver_en = %d s2mm005_om = %d\n"
		"s2mm005_sda = %d, s2mm005_scl = %d\n",
		usbpd_data->hw_rev,
		usbpd_data->irq_gpio, usbpd_data->redriver_en, usbpd_data->s2mm005_om,
		usbpd_data->s2mm005_sda, usbpd_data->s2mm005_scl);

	return 0;
}
#endif /* CONFIG_OF */

static int s2mm005_usbpd_probe(struct i2c_client *i2c,
			       const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(i2c->dev.parent);
	struct s2mm005_data *usbpd_data;
	int ret = 0;

	dev_info(&i2c->dev, "%s upload start\n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&i2c->dev, "i2c functionality check error\n");
		return -EIO;
	}
	usbpd_data = devm_kzalloc(&i2c->dev, sizeof(struct s2mm005_data), GFP_KERNEL);

	if (!usbpd_data) {
		dev_err(&i2c->dev, "Failed to allocate driver data\n");
		return -ENOMEM;
	}

#if defined(CONFIG_OF)
	if (i2c->dev.of_node)
		of_s2mm005_usbpd_dt(&i2c->dev, usbpd_data);
	else
		dev_err(&i2c->dev, "not found ccic dt! ret:%d\n", ret);
#endif
	usbpd_data->dev = &i2c->dev;
	usbpd_data->i2c = i2c;
	i2c_set_clientdata(i2c, usbpd_data);
	mutex_init(&usbpd_data->i2c_mutex);
	device_init_wakeup(usbpd_data->dev, true);

	s2mm005_usbpd_configure_init(usbpd_data);

	usbpd_data->udev =
		s2mm005_usbpd_desc_register(usbpd_data, S2MM005_USBPD_PORT_NAME);
	if (usbpd_data->udev == NULL) {
		dev_err(&i2c->dev, "%s, failed register usbpd_dev\n", __func__);
		goto err_register_udev;
	}

	wake_lock_init(&usbpd_data->wlock, WAKE_LOCK_SUSPEND,
		       "s2mm005-intr");

#if defined(CONFIG_DUAL_ROLE_USB_INTF)
	usbpd_data->try_state_change = 0;
#endif
#if defined(CONFIG_SEC_FACTORY)
	usbpd_data->fac_water_enable = 0;
#endif

	s2mm005_usbpd_firmware_check(usbpd_data);

	s2mm005_usbpd_phy_init(usbpd_data);

	ret = s2mm005_usbpd_irq_init(usbpd_data);
	if (ret < 0) {
		dev_err(&i2c->dev, "%s, failed init irq : %d\n", __func__, ret);
		goto err_init_irq;
	}

	s2mm005_usbpd_irq_thread(-1, usbpd_data);

	/* create sysfs group */
	usbpd_data->ccic_device = sec_device_create(NULL, "ccic");
	if (IS_ERR(usbpd_data->ccic_device))
		pr_err("%s Failed to create device(switch)!\n", __func__);

	/* create sysfs group */
	ret = sysfs_create_group(&usbpd_data->ccic_device->kobj, &ccic_sysfs_group);
	if (ret)
		pr_err("%s: ccic sysfs fail, ret %d", __func__, ret);

	dev_set_drvdata(usbpd_data->ccic_device, usbpd_data);

#if 0
defined(CONFIG_BATTERY_SAMSUNG)
	W_CHG_INFO[0] = 0x0f;
		W_CHG_INFO[1] = 0x0c;
		if (lpcharge)
			W_CHG_INFO[2] = 0x1;
		else
			W_CHG_INFO[2] = 0x0;

		s2mm005_write_byte(usbpd_data->i2c, 0x10, &W_CHG_INFO[0], 3);
#endif

	s2mm005_int_clear(usbpd_data);
	return ret;

err_init_irq:
	if (usbpd_data->irq) {
		free_irq(usbpd_data->irq, usbpd_data);
		usbpd_data->irq = 0;
	}
err_register_udev:
	gpio_free(usbpd_data->redriver_en);
	wake_lock_destroy(&usbpd_data->wlock);
	return ret;
}

static int s2mm005_usbpd_remove(struct i2c_client *i2c)
{
	struct s2mm005_data *usbpd_data = i2c_get_clientdata(i2c);

/*	TODO check neccessarity
	process_cc_detach(usbpd_data);
*/
	if (i2c->irq) {
		disable_irq_wake(usbpd_data->i2c->irq);
		free_irq(usbpd_data->i2c->irq, usbpd_data);
	}
	mutex_destroy(&usbpd_data->i2c_mutex);
	i2c_set_clientdata(i2c, NULL);

	wake_lock_destroy(&usbpd_data->wlock);

	return 0;
}

static void s2mm005_usbpd_shutdown(struct i2c_client *i2c)
{
	struct s2mm005_data *usbpd_data = i2c_get_clientdata(i2c);
	struct usbpd_info *pd_info = &usbpd_data->udev->desc->pd_info;
	struct device_node *np;
	int gpio_dp_sw_oe;

	disable_irq(i2c->irq);

	if (!usbpd_data->manual_lpm_mode) {
		pr_info("%s: pd_state=%d, water=%d, dry=%d\n", __func__,
			usbpd_data->pd_state, usbpd_data->water_det, usbpd_data->run_dry);

		if (usbpd_data->water_det) {
			s2mm005_hard_reset(usbpd_data);
		} else {
			if (usbpd_data->pd_state) {
				if (pd_info->dp_is_connect) {
					pr_info("aux_sw_oe pin set to high\n");
					np = of_find_node_by_name(NULL, "displayport");
					gpio_dp_sw_oe = of_get_named_gpio(np, "dp,aux_sw_oe", 0);
					gpio_direction_output(gpio_dp_sw_oe, 1);
				}
				s2mm005_manual_LPM(usbpd_data, 0xB);
				mdelay(110);
			}
			s2mm005_reset(usbpd_data);
		}
	}
}

#if defined(CONFIG_PM)
static int s2mm005_suspend(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	pr_info("%s:%s\n", USBPD005_DEV_NAME, __func__);
#endif /* CONFIG_SAMSUNG_PRODUCT_SHIP */

	if (device_may_wakeup(dev))
		enable_irq_wake(i2c->irq);

	disable_irq(i2c->irq);

	return 0;
}

static int s2mm005_resume(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	pr_info("%s:%s\n", USBPD005_DEV_NAME, __func__);
#endif /* CONFIG_SAMSUNG_PRODUCT_SHIP */

	if (device_may_wakeup(dev))
		disable_irq_wake(i2c->irq);

	enable_irq(i2c->irq);

	return 0;
}
#else
#define s2mm005_suspend	NULL
#define s2mm005_resume		NULL
#endif /* CONFIG_PM */

static const struct i2c_device_id s2mm005_usbpd_id[] = {
	{ USBPD005_DEV_NAME, 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, s2mm005_usbpd_id);

#if defined(CONFIG_OF)
static struct of_device_id s2mm005_i2c_dt_ids[] = {
	{ .compatible = "sec-s2mm005,i2c" },
	{ }
};
#endif /* CONFIG_OF */

#if defined(CONFIG_PM)
const struct dev_pm_ops s2mm005_pm = {
	.suspend = s2mm005_suspend,
	.resume = s2mm005_resume,
};
#endif /* CONFIG_PM */

static struct i2c_driver s2mm005_usbpd_driver = {
	.driver		= {
		.name	= USBPD005_DEV_NAME,
#if defined(CONFIG_PM)
		.pm	= &s2mm005_pm,
#endif /* CONFIG_PM */
#if defined(CONFIG_OF)
		.of_match_table	= s2mm005_i2c_dt_ids,
#endif /* CONFIG_OF */
	},
	.probe		= s2mm005_usbpd_probe,
	.remove		= s2mm005_usbpd_remove,
	.shutdown	= s2mm005_usbpd_shutdown,
	.id_table	= s2mm005_usbpd_id,
};

static int __init s2mm005_usbpd_init(void)
{
	return i2c_add_driver(&s2mm005_usbpd_driver);
}
module_init(s2mm005_usbpd_init);

static void __exit s2mm005_usbpd_exit(void)
{
	i2c_del_driver(&s2mm005_usbpd_driver);
}
module_exit(s2mm005_usbpd_exit);

MODULE_DESCRIPTION("s2mm005 USB PD driver");
MODULE_LICENSE("GPL");
