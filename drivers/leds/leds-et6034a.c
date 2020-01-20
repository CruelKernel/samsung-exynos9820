/*
 * LED driver for Samsung ET6034A
 *
 * Copyright (C) 2018 Samsung Electronics
 * Author: Hyuk Kang
 *
 */

#define pr_fmt(fmt) "sled_et6034a: " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include "led-sep.h"

#define SW_RSTCTR_ON	0X00
#define SW_RSTCTR_OFF	0x01
#define RGB_OE_MASK     0x33

enum et6034a_reg {
	ET6034A_RSTCTR = 0,
	ET6034A_RGB_OE,
};

struct et6034a_led {
	struct i2c_client *i2c;
	u8 oe_state; /* backup of RGB_OE register */
};

/* FIXBRIT_LEDx base address */
#if 0
static int fixbrit[4] = { 0x02, 0x05, 0x0e, 0x11 };
static int oe_shift[4] = { 0, 1, 4, 5 };
#else
static int fixbrit[4] = { 0x11, 0x0e, 0x05, 0x2 };
static int oe_shift[4] = { 5, 4, 1, 0 };
#endif

#define ET6034A_REGS	18
static u8 backup_regs[ET6034A_REGS];
static void dump_reg(void)
{
	u8 i = 0;

	pr_info("DUMP START ============================================\n");

	for (i = 0; i < ET6034A_REGS; i++)
		pr_info("et6034a 0x%x: 0x%x\n", i, backup_regs[i]);

	pr_info("DUMP END ============================================\n");
}

static int et6034a_i2c_write(struct i2c_client *i2c, u8 reg, u8 value)
{
	int ret;

	pr_info("  %s 0x%x, 0x%x\n", __func__, reg, value);

	if (reg >= ET6034A_REGS)
		return -EINVAL;

	backup_regs[reg] = value;

	ret = i2c_smbus_write_byte_data(i2c, reg, value);
	if (ret < 0)
		pr_info("%s 0x%x, ret(%d)\n", __func__, reg, ret);
	return ret;
}

static int et6034a_write(struct et6034a_led *led, u8 reg, u8 value)
{
	if (led && led->i2c)
		return et6034a_i2c_write(led->i2c, reg, value);
	return -ENODEV;
}

static int led_test(struct et6034a_led *led)
{
	int ret = 0;
	u8 i = 0;

	for (i = 2; i < 8; i++)
		ret = et6034a_write(led, i, 0x7f);

	for (i = 12; i < ET6034A_REGS; i++)
		ret = et6034a_write(led, i, 0x7f);
	return ret;
}

static int et6034a_power(bool on)
{
	struct regulator *ldo = NULL;
	int ret = 0;

	pr_info("%s: %s\n", __func__, on ? "on" : "off");

	ldo = regulator_get(NULL, "m_vdd_ldo29");
	if (IS_ERR_OR_NULL(ldo)) {
		pr_err("%s: Failed to get %s regulator.\n",
				__func__, "m_vdd_ldo29");
		ret = PTR_ERR(ldo);
		goto error;
	}

	if (on) {
		ret = regulator_enable(ldo);
		if (ret)
			pr_err("%s: Failed to enable LDO: %d\n", __func__, ret);
	} else
		regulator_disable(ldo);

error:
	regulator_put(ldo);

	return ret;
}

static int et6034a_color(struct et6034a_led *led,
		int led_num, int color)
{
	int ret = 0;

	pr_info("%s: led %d color 0x%x\n", __func__,
			led_num, color);

	ret = et6034a_write(led, fixbrit[led_num], LED_R(color));
	if (ret < 0)
		return ret;
	ret = et6034a_write(led, fixbrit[led_num] + 1, LED_G(color));
	if (ret < 0)
		return ret;
	ret = et6034a_write(led, fixbrit[led_num] + 2, LED_B(color));
	if (ret < 0)
		return ret;
	return ret;
}

static inline void et6034a_update_oe(struct et6034a_led *led, int id, int onoff)
{
	int on = !!onoff;
	u8 oe_value = led->oe_state;

	oe_value &= ~(1 << oe_shift[id]);
	oe_value |= on << oe_shift[id];

	pr_info("%s: 0x%x -> 0x%x\n", __func__, led->oe_state, oe_value);

	led->oe_state = oe_value;
}

static inline int et6034a_set_oe(struct et6034a_led *led)
{
	return et6034a_write(led, ET6034A_RGB_OE, led->oe_state);
}

static int et6034a_light(struct led_sep_ops *ops,
		int id, int on, int color)
{
	struct et6034a_led *led = led_sep_get_opsdata(ops);
	int i, ret = 0;

	pr_info("%s: id %x, on %d color 0x%x\n", __func__,
			id, on, color);

	for (i = 0; i < 4; i++) {
		if (id & (1 << i)) {
			et6034a_color(led, i, color);
			et6034a_update_oe(led, i, on);
		}
	}
	et6034a_set_oe(led);

	return ret;
}

static int et6034a_test(struct led_sep_ops *ops, int reg, int value)
{
	int ret = 0;
	struct et6034a_led *led = led_sep_get_opsdata(ops);

	pr_info("%s 0x%x, 0x%x\n", __func__, reg, value);

	ret = et6034a_write(led, reg, value);
	if (ret < 0)
		pr_info("%s 0x%x, ret(%d)\n", __func__, reg, ret);

	return ret;
}

static int et6034a_reset(struct led_sep_ops *ops)
{
	int reg, ret = 0;
	struct et6034a_led *led = led_sep_get_opsdata(ops);

	pr_info("%s\n", __func__);

	for (reg = 0; reg < 0x13 ; reg++) {
		ret = et6034a_write(led, reg, 0x00);
		if (ret < 0)
			pr_info("%s 0x%x, ret(%d)\n", __func__, reg, ret);
	}

	return ret;
}

static int et6034a_pattern(struct led_sep_ops *ops, int mode)
{
	int ret = 0;

	pr_info("%s: %s\n", __func__, pattern_string(mode));

	switch (mode) {
	case PATTERN_OFF:
		break;
	case POWERING:
		break;
	default:
		break;
	}

	dump_reg();
	pr_info("%s: %s Done\n", __func__, pattern_string(mode));
	return ret;
}

static struct led_sep_ops et6034a_ops = {
	.name = "ET6034A",
	.current_max = 127,
	.current_lowpower = 127,

	.light = et6034a_light,
	.test = et6034a_test,
	.reset = et6034a_reset,
	.pattern = et6034a_pattern,
	.capability = (SEP_NODE_PATTERN | SEP_NODE_CONTROL),
};

static int et6034a_led_probe(struct i2c_client *client,
		const struct i2c_device_id *i2c_id)
{
	struct et6034a_led *led;

	dev_info(&client->dev, "%s\n", __func__);

	led = devm_kzalloc(&client->dev,
			sizeof(struct et6034a_led),
			GFP_KERNEL);
	if (!led) {
		dev_info(&client->dev, "%s power on -> No memory.\n",
				__func__);
		return -ENOMEM;

	}
	led->i2c = client;
	i2c_set_clientdata(client, led);

	et6034a_power(true);
	msleep(100);
	et6034a_write(led, ET6034A_RSTCTR, SW_RSTCTR_ON);
	et6034a_write(led, ET6034A_RGB_OE, RGB_OE_MASK);
	led_test(led);

	led_sep_set_opsdata(&et6034a_ops, led);
	led_sep_register_device(&et6034a_ops);

	dev_info(&client->dev, "%s power on -> Done\n", __func__);

	return 0;
}

static int et6034a_led_remove(struct i2c_client *client)
{
	dev_info(&client->dev, "%s\n", __func__);
	et6034a_i2c_write(client, 0x00, 0x00);
	et6034a_power(false);
	return 0;
}

static void et6034a_led_shutdown(struct i2c_client *client)
{
	et6034a_led_remove(client);
	dev_info(&client->dev, "%s\n", __func__);
}

static const struct i2c_device_id et6034a_i2c_id[] = {
	{ "ET6034A", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, et6034a_i2c_id);

static const struct of_device_id et6034a_led_of_match[] = {
	{ .compatible = "samsung,et6034a", },
	{ }
};

static struct i2c_driver et6034a_led_driver = {
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "ET6034A",
		.of_match_table = of_match_ptr(et6034a_led_of_match),
	},
	.probe	= et6034a_led_probe,
	.remove	= et6034a_led_remove,
	.shutdown = et6034a_led_shutdown,
	.id_table	= et6034a_i2c_id,
};

static int __init et6034a_i2c_init(void)
{
	pr_info("%s\n", __func__);
	return i2c_add_driver(&et6034a_led_driver);
}
module_init(et6034a_i2c_init);

static void __exit et6034a_i2c_exit(void)
{
	i2c_del_driver(&et6034a_led_driver);
}
module_exit(et6034a_i2c_exit);

MODULE_AUTHOR("Samsung EIF Team");
