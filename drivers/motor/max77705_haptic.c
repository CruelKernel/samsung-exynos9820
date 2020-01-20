/*
 * haptic motor driver for max77705_haptic.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) "[VIB] " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/pwm.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/max77705.h>
#include <linux/mfd/max77705-private.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/sec_haptic.h>

#define MOTOR_LRA                       (1<<7)
#define MOTOR_EN                        (1<<6)
#define EXT_PWM                         (0<<5)
#define DIVIDER_128                     (1<<1)
#define MRDBTMER_MASK                   (0x7)
#define MREN                            (1 << 3)
#define BIASEN                          (1 << 7)

struct max77705_haptic_drvdata {
	struct max77705_dev *max77705;
	struct sec_haptic_drvdata *shdata;
	struct i2c_client *i2c;
	struct max77705_haptic_pdata *pdata;
	struct pwm_device *pwm;
	struct regulator *regulator;
};

static int max77705_haptic_set_pwm(void *data, u32 duty)
{
	struct max77705_haptic_drvdata *drvdata = (struct max77705_haptic_drvdata *)data;
	struct sec_haptic_drvdata *shdata = drvdata->shdata;
	int ret = 0;

	ret = pwm_config(drvdata->pwm, duty, shdata->period);
	if (ret < 0) {
		pr_err("failed to config pwm %d\n", ret);
		return ret;
	}

	if (duty != shdata->period >> 1) {
		ret = pwm_enable(drvdata->pwm);
		if (ret < 0)
			pr_err("failed to enable pwm %d\n", ret);
	} else {
		pwm_disable(drvdata->pwm);
	}

	return ret;
}

static int max77705_motor_boost_control(void *data, int control)
{
	return 0;
}

static int max77705_haptic_i2c(void *data, bool en)
{
	struct max77705_haptic_drvdata *drvdata = (struct max77705_haptic_drvdata *)data;
	int ret = 0;

	ret = max77705_update_reg(drvdata->i2c,
			MAX77705_PMIC_REG_MCONFIG,
			en ? 0xff : 0x0, MOTOR_LRA | MOTOR_EN);
	if (ret)
		pr_err("i2c LRA and EN update error %d\n", ret);

	return ret;
}

static void max77705_haptic_init_reg(struct max77705_haptic_drvdata *drvdata, bool init_en)
{
	int ret;

	ret = max77705_update_reg(drvdata->i2c,
			MAX77705_PMIC_REG_MAINCTRL1, 0xff, BIASEN);
	if (ret)
		pr_err("i2c REG_BIASEN update error %d\n", ret);

	if (init_en) {
		ret = max77705_update_reg(drvdata->i2c,
			MAX77705_PMIC_REG_MCONFIG, 0x0, MOTOR_EN);
		if (ret)
			pr_err("i2c MOTOR_EN update error %d\n", ret);
	}

	ret = max77705_update_reg(drvdata->i2c,
		MAX77705_PMIC_REG_MCONFIG, 0xff, MOTOR_LRA);
	if (ret)
		pr_err("i2c MOTOR_LPA update error %d\n", ret);

	ret = max77705_update_reg(drvdata->i2c,
		MAX77705_PMIC_REG_MCONFIG, 0xff, DIVIDER_128);
	if (ret)
		pr_err("i2c DIVIDER_128 update error %d\n", ret);
}

#if defined(CONFIG_OF)
static struct max77705_haptic_pdata *of_max77705_haptic_dt(struct device *dev)
{
	struct device_node *np_root = dev->parent->of_node;
	struct device_node *np;
	struct max77705_haptic_pdata *pdata;
	u32 temp;
	const char *type;
	int ret, i;

	pdata = kzalloc(sizeof(struct max77705_haptic_pdata),
		GFP_KERNEL);
	if (!pdata)
		return NULL;

	np = of_find_node_by_name(np_root,
			"haptic");
	if (np == NULL) {
		pr_err("%s : error to get dt node\n", __func__);
		goto err_parsing_dt;
	}

	ret = of_property_read_u32(np,
			"haptic,max_timeout", &temp);
	if (ret) {
		pr_err("%s : error to get dt node max_timeout\n", __func__);
		goto err_parsing_dt;
	} else
		pdata->max_timeout = (u16)temp;

	ret = of_property_read_u32(np, "haptic,multi_frequency", &temp);
	if (ret) {
		pr_err("%s : error to get dt node multi_frequency\n", __func__);
		pdata->multi_frequency = 0;
	} else
		pdata->multi_frequency = (int)temp;

	if (pdata->multi_frequency) {
		pdata->multi_freq_duty
			= devm_kzalloc(dev, sizeof(u32)*pdata->multi_frequency, GFP_KERNEL);
		if (!pdata->multi_freq_duty) {
			pr_err("%s: failed to allocate duty data\n", __func__);
			goto err_parsing_dt;
		}
		pdata->multi_freq_period
			= devm_kzalloc(dev, sizeof(u32)*pdata->multi_frequency, GFP_KERNEL);
		if (!pdata->multi_freq_period) {
			pr_err("%s: failed to allocate period data\n", __func__);
			goto err_parsing_dt;
		}

		ret = of_property_read_u32_array(np, "haptic,duty", pdata->multi_freq_duty,
				pdata->multi_frequency);
		if (ret) {
			pr_err("%s : error to get dt node duty\n", __func__);
			goto err_parsing_dt;
		}

		ret = of_property_read_u32_array(np, "haptic,period", pdata->multi_freq_period,
				pdata->multi_frequency);
		if (ret) {
			pr_err("%s : error to get dt node period\n", __func__);
			goto err_parsing_dt;
		}

		pdata->duty = pdata->multi_freq_duty[0];
		pdata->period = pdata->multi_freq_period[0];
		pdata->freq_num = 0;

		ret = of_property_read_u32(np,
				"haptic,normal_ratio", &temp);
		if (ret) {
			pr_err("%s : error to get dt node normal_ratio\n", __func__);
			goto err_parsing_dt;
		} else
			pdata->normal_ratio = (int)temp;

		ret = of_property_read_u32(np,
				"haptic,overdrive_ratio", &temp);
		if (ret) {
			pr_err("%s : error to get dt node overdrive_ratio\n", __func__);
			goto err_parsing_dt;
		} else
			pdata->overdrive_ratio = (int)temp;

	} else {
		ret = of_property_read_u32(np,
				"haptic,duty", &temp);
		if (ret) {
			pr_err("%s : error to get dt node duty\n", __func__);
			goto err_parsing_dt;
		} else
			pdata->duty = (u16)temp;

		ret = of_property_read_u32(np,
				"haptic,period", &temp);
		if (ret) {
			pr_err("%s : error to get dt node period\n", __func__);
			goto err_parsing_dt;
		} else
			pdata->period = (u16)temp;
	}

	ret = of_property_read_string(np,
			"haptic,type", &type);
	if (ret) {
		pr_err("%s : error to get dt node type\n", __func__);
		goto err_parsing_dt;
	} else
		pdata->vib_type = type;

	ret = of_property_read_u32(np,
			"haptic,pwm_id", &temp);
	if (ret) {
		pr_err("%s : error to get dt node pwm_id\n", __func__);
		goto err_parsing_dt;
	} else
		pdata->pwm_id = (u16)temp;

	/* debugging */
	pr_info("max_timeout = %d\n", pdata->max_timeout);
	if (pdata->multi_frequency) {
		pr_info("multi frequency = %d\n", pdata->multi_frequency);
		for (i = 0; i < pdata->multi_frequency; i++) {
			pr_info("duty[%d] = %d\n", i, pdata->multi_freq_duty[i]);
			pr_info("period[%d] = %d\n", i, pdata->multi_freq_period[i]);
		}
	} else {
		pr_info("duty = %d\n", pdata->duty);
		pr_info("period = %d\n", pdata->period);
	}
	pr_info("normal_ratio = %d\n", pdata->normal_ratio);
	pr_info("overdrive_ratio = %d\n", pdata->overdrive_ratio);
	pr_info("pwm_id = %d\n", pdata->pwm_id);

	return pdata;

err_parsing_dt:
	kfree(pdata);
	return NULL;
}
#endif

static int max77705_haptic_probe(struct platform_device *pdev)
{
	int error = 0;
	struct max77705_dev *max77705 = dev_get_drvdata(pdev->dev.parent);
	struct max77705_platform_data *max77705_pdata
		= dev_get_platdata(max77705->dev);
	struct max77705_haptic_pdata *pdata
		= max77705_pdata->haptic_data;
	struct max77705_haptic_drvdata *drvdata;
	struct sec_haptic_drvdata *shdata;

#if defined(CONFIG_OF)
	if (pdata == NULL) {
		pdata = of_max77705_haptic_dt(&pdev->dev);
		if (unlikely(!pdata)) {
			pr_err("max77705-haptic : %s not found haptic dt!\n",
					__func__);
			return -1;
		}
	}
#else
	if (unlikely(!pdata)) {
		pr_err("%s: no pdata\n", __func__);
		return -ENODEV;
	}
#endif /* CONFIG_OF */

	drvdata = kzalloc(sizeof(struct max77705_haptic_drvdata), GFP_KERNEL);
	if (unlikely(!drvdata))
		goto err_alloc1;

	shdata = kzalloc(sizeof(struct sec_haptic_drvdata), GFP_KERNEL);
	if (unlikely(!shdata))
		goto err_alloc2;

	platform_set_drvdata(pdev, drvdata);
	drvdata->max77705 = max77705;
	drvdata->i2c = max77705->i2c;
	drvdata->pdata = pdata;
	drvdata->shdata = shdata;

	shdata->intensity = MAX_INTENSITY;
	shdata->duty = shdata->max_duty = pdata->duty;
	shdata->period = pdata->period;
	shdata->max_timeout = pdata->max_timeout;
	shdata->multi_frequency = pdata->multi_frequency;
	shdata->freq_num = pdata->freq_num;
	shdata->vib_type = pdata->vib_type;
	shdata->multi_freq_duty = pdata->multi_freq_duty;
	shdata->multi_freq_period = pdata->multi_freq_period;
	shdata->data = drvdata;
	shdata->boost = max77705_motor_boost_control;
	shdata->enable = max77705_haptic_i2c;
	shdata->set_intensity = max77705_haptic_set_pwm;
	shdata->normal_ratio = pdata->normal_ratio;
	shdata->overdrive_ratio = pdata->overdrive_ratio;

	drvdata->pwm = pwm_request(pdata->pwm_id, "vibrator");
	if (IS_ERR(drvdata->pwm)) {
		error = -EFAULT;
		pr_err("Failed to request pwm, err num: %d\n", error);
		goto err_pwm_request;
	} else
		pwm_config(drvdata->pwm, pdata->period >> 1, pdata->period);
	max77705_haptic_init_reg(drvdata, true);
	max77705_motor_boost_control(drvdata, BOOST_ON);
	sec_haptic_register(shdata);

	return 0;

err_pwm_request:
	kfree(shdata);
err_alloc2:
	kfree(drvdata);
err_alloc1:
	kfree(pdata);
	return error;
}

static int max77705_haptic_remove(struct platform_device *pdev)
{
	struct max77705_haptic_drvdata *drvdata
		= platform_get_drvdata(pdev);

	max77705_motor_boost_control(drvdata, BOOST_OFF);
	sec_haptic_unregister(drvdata->shdata);
	pwm_free(drvdata->pwm);
	max77705_haptic_i2c(drvdata, false);
	kfree(drvdata);
	return 0;
}

static int max77705_haptic_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	struct max77705_haptic_drvdata *drvdata
		= platform_get_drvdata(pdev);

	if (drvdata->shdata) {
		kthread_flush_worker(&drvdata->shdata->kworker);
		hrtimer_cancel(&drvdata->shdata->timer);
	}
	max77705_motor_boost_control(drvdata, BOOST_OFF);
	return 0;
}

static int max77705_haptic_resume(struct platform_device *pdev)
{
	struct max77705_haptic_drvdata *drvdata
		= platform_get_drvdata(pdev);

	max77705_haptic_init_reg(drvdata, false);
	max77705_motor_boost_control(drvdata, BOOST_ON);
	return 0;
}

static void max77705_haptic_shutdown(struct platform_device *pdev)
{
	struct max77705_haptic_drvdata *drvdata
		= platform_get_drvdata(pdev);

	max77705_motor_boost_control(drvdata, BOOST_OFF);
}

static struct platform_driver max77705_haptic_driver = {
	.probe		= max77705_haptic_probe,
	.remove		= max77705_haptic_remove,
	.suspend	= max77705_haptic_suspend,
	.resume		= max77705_haptic_resume,
	.shutdown	= max77705_haptic_shutdown,
	.driver = {
		.name	= "max77705-haptic",
		.owner	= THIS_MODULE,
	},
};

static int __init max77705_haptic_init(void)
{
	return platform_driver_register(&max77705_haptic_driver);
}
module_init(max77705_haptic_init);

static void __exit max77705_haptic_exit(void)
{
	platform_driver_unregister(&max77705_haptic_driver);
}
module_exit(max77705_haptic_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("max77705 haptic driver");
