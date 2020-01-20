/*
 * LED driver for Samsung Bolt ET6312B
 *
 * Copyright (C) 2018 Samsung Electronics
 * Author: Hyuk Kang, HyungJae Im
 *
 */

#define pr_fmt(fmt) "sled_et6312b: " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/hrtimer.h>
#include "led-sep.h"

#define LED_R_SHIFT			0
#define LED_G_SHIFT			2
#define LED_B_SHIFT			5

#define LEDXMD_R_MASK			0x03
#define LEDXMD_G_MASK			0x1C
#define LEDXMD_B_MASK			0xE0

#define ALWAYS_OFF			0
#define ALWAYS_ON			1
#define BLINK				2

#define ENABLE				0x90
#define DISABLE				0x00

#define LED_MAX				4
#define LED_NORMAL_CURRENT		255
#define RED				COLOR(0x1D, 0, 0)
#define GREEN				COLOR(0, 0xF, 0)
#define WHITE				COLOR(0x7, 0x7, 0x7)

#define ET6312B_CHIPCTR			0x00
#define ET6312B_PERIOD_RGB0		0x01
#define ET6312B_PERIOD_RGB3		0x10
#define ET6312B_TON1_RGB0		0x02
#define ET6312B_TON1_RGB3		0x11
#define ET6312B_DELAY_TIME_RGB0		0x05
#define ET6312B_DELAY_TIME_RGB1		0x0A
#define ET6312B_DELAY_TIME_RGB2		0x0F
#define ET6312B_DELAY_TIME_RGB3		0x14
#define ET6312B_LEDXMD_RGB0		0x16
#define ET6312B_LEDXMD_RGB1		0x17
#define ET6312B_LEDXMD_RGB2		0x18
#define ET6312B_LEDXMD_RGB3		0x19
#define ET6312B_LED1_CURT		0x1A
#define ET6312B_LED4_CURT		0x1D
#define ET6312B_LED7_CURT		0x20
#define ET6312B_LED10_CURT		0x23
#define ET6312B_LED12_CURT		0x25

#define TIMER_SET(total_time, count)	ktime_set(total_time * count / 1000, \
			(total_time * count % 1000) * 1000000)

static struct hrtimer et6312b_timer[4];
static struct work_struct et6312b_work[4];

extern unsigned int lpcharge;

enum led_regs {
	CURR,
	XMD,
	PERIOD,
	TON1,
	DELAY,
};

/* CURR, XMD, PERIOD, TON1, DELAY */
static int rgb0[] = { 0x1a, 0x16, 0x01, 0x02, 0x05 };
static int rgb1[] = { 0x1d, 0x17, 0x06, 0x07, 0x0a };
static int rgb2[] = { 0x20, 0x18, 0x0b, 0x0c, 0x0f };
static int rgb3[] = { 0x23, 0x19, 0x10, 0x11, 0x14 };

struct et6312b_led {
	struct i2c_client *i2c;
	int pattern_count;

	struct regulator *vdd;
	const char *regulator_name;

	int *rgb[4];
};

enum led_index {
	FIVEG = 1,
	LTE = 2,
	WIFI = 4,
	BATTERY = 8,
};

struct workqueue_struct *et6312b_workqueue;

static inline u8 get_address(struct et6312b_led *led, int led_num, int reg)
{
	if (led && (led_num < 4) && (reg < 5) && led->rgb[led_num])
		return led->rgb[led_num][reg];
	else {
		pr_err("%s failed. led%d, reg %d\n", __func__, led_num, reg);
		return 0;
	}
}

static int et6312b_i2c_write(struct i2c_client *i2c, u8 reg, u8 value)
{
	int ret;

	ret = i2c_smbus_write_byte_data(i2c, reg, value);
	if (ret < 0)
		pr_info("%s 0x%x, ret(%d)\n", __func__, reg, ret);
	return ret;
}

static int et6312b_i2c_read(struct i2c_client *i2c, u8 reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret < 0)
		pr_info("%s 0x%x: 0x%x\n", __func__, reg, ret);
	return ret;
}

static int et6312b_write(struct et6312b_led *led, u8 reg, u8 value)
{
	if (led && led->i2c)
		return et6312b_i2c_write(led->i2c, reg, value);
	return -ENODEV;
}

static int et6312b_read(struct et6312b_led *led, u8 reg)
{
	if (led && led->i2c)
		return et6312b_i2c_read(led->i2c, reg);
	return -ENODEV;
}

static int dump_reg(struct et6312b_led *led)
{
	int ret = 0;
	u8 i = 0;

	pr_info("DUMP START ============================================\n");
	for (i = 0; i < 0x27; i++) {
		ret = et6312b_read(led, i);
		pr_info("et6312b 0x%x: 0x%x\n", i, ret);
	}
	pr_info("DUMP END ============================================\n");
	return ret;
}

static inline void et6312b_clear(struct et6312b_led *led)
{
	u8 i = 0;

	for (i = 0; i < 0x27; i++)
		et6312b_write(led, i, 0);
}

static int et6312b_test(struct led_sep_ops *ops, int reg, int value)
{
	int ret = 0;
	struct et6312b_led *led = led_sep_get_opsdata(ops);

	pr_info("%s 0x%x, 0x%x\n", __func__, reg, value);

	ret = et6312b_write(led, reg, value);
	if (ret < 0)
		pr_info("%s 0x%x, ret(%d)\n", __func__, reg, ret);

	dump_reg(led);

	return ret;
}

static int et6312b_reset(struct led_sep_ops *ops)
{
	struct et6312b_led *led = led_sep_get_opsdata(ops);

	pr_info("%s\n", __func__);
	et6312b_clear(led);
	return 0;
}

static int et6312b_power(struct et6312b_led *led, bool on)
{
	int ret = 0;

	pr_info("%s: %s\n", __func__, on ? "on" : "off");

	if (!led->vdd) {
		pr_info("%s: no regulator, return without control\n", __func__);
		return -ENODEV;
	}

	if (on) {
		ret = regulator_enable(led->vdd);
		if (ret)
			pr_err("%s: Failed to enable LDO: %d\n", __func__, ret);
	} else
		regulator_disable(led->vdd);

	return ret;
}

static inline int update_xmd(u8 xmd, u8 mask, u8 shift, int value)
{
	u8 reg = xmd & ~mask;

	reg |= value << shift;
	return reg;
}

static int et6312b_workmode(struct et6312b_led *led,
		int led_num, int color, int mode)
{
	int ret = 0;
	u8 reg = get_address(led, led_num, XMD);
	u8 backup = et6312b_read(led, reg);
	u8 xmd = 0;

	pr_info("%s: led%d color 0x%x mode 0x%x\n", __func__,
			led_num, color, mode);

	if (mode == ALWAYS_OFF) {
		if (!LED_R(color))
			xmd = update_xmd(xmd, LEDXMD_R_MASK, LED_R_SHIFT, mode);
		if (!LED_G(color))
			xmd = update_xmd(xmd, LEDXMD_G_MASK, LED_G_SHIFT, mode);
		if (!LED_B(color))
			xmd = update_xmd(xmd, LEDXMD_B_MASK, LED_B_SHIFT, mode);
	} else {
		if (LED_R(color))
			xmd = update_xmd(xmd, LEDXMD_R_MASK, LED_R_SHIFT, mode);
		if (LED_G(color))
			xmd = update_xmd(xmd, LEDXMD_G_MASK, LED_G_SHIFT, mode);
		if (LED_B(color))
			xmd = update_xmd(xmd, LEDXMD_B_MASK, LED_B_SHIFT, mode);
	}

	if (backup != xmd) {
		pr_info("%s: led%d xmd 0x%x -> 0x%x\n", __func__,
				led_num, backup, xmd);
		et6312b_write(led, reg, xmd);
	}

	return ret;
}

static int et6312b_color(struct et6312b_led *led,
		int led_num, int color)
{
	int ret = 0;
	u8 reg = 0;

	reg = get_address(led, led_num, CURR);

	pr_info("%s: led%d (0x%x) color 0x%x\n", __func__,
			led_num, reg, color);

	ret = et6312b_write(led, reg, LED_R(color));
	if (ret < 0)
		return ret;
	ret = et6312b_write(led, reg + 1, LED_G(color));
	if (ret < 0)
		return ret;
	ret = et6312b_write(led, reg + 2, LED_B(color));
	if (ret < 0)
		return ret;
	return ret;
}

static inline int calc_period(int total)
{
	return (total < 128) ? 0 :
			(total < 16380) ? (total / 128) : 127;
}

static inline int calc_ton1(int on, int off)
{
	return (on * 100) / (on + off) * 256 / 100;
}

static int et6312b_set_blink(struct et6312b_led *led,
		int led_num, int color, int on, int off)
{
	u8 period, ton1, ton1_time;
	int total_time = on + off;

	period = get_address(led, led_num, PERIOD);
	ton1 = get_address(led, led_num, TON1);

	total_time = calc_period(total_time);
	ton1_time = calc_ton1(on, off);

	pr_info("blink period %d ms -> %d value, ton1 %d\n",
			on + off, total_time, ton1_time);

	et6312b_color(led, led_num, color);
	et6312b_workmode(led, led_num, color, BLINK);
	et6312b_write(led, period, total_time);
	et6312b_write(led, ton1, ton1_time);
	return 0;
}

static inline void et6312b_clear_delay(struct et6312b_led *led,
		int led_num)
{
	u8 delay = 0;

	delay = get_address(led, led_num, DELAY);
	et6312b_write(led, delay, 0);
}

static int et6312b_init(struct et6312b_led *led)
{
	int i = 0;

	for (i = 0; i < 4; i++) {
		/*delete delay set from BL */
		et6312b_clear_delay(led, i);
	}
	return 0;
}

static void et6312b_cancel_timer(void)
{
	int led_index = 0;

	for (led_index = 0 ; led_index < LED_MAX; led_index++) {
		hrtimer_cancel(&et6312b_timer[led_index]);
	}
}

static void et6312b_pattern_shutdown(struct led_sep_ops *ops)
{
	struct et6312b_led *led = led_sep_get_opsdata(ops);
	u8 reg_delay = 0;
	int i = 0;
	int j = 3;

	pr_info("%s\n", __func__);

	led_sep_clear_cap(ops, SEP_NODE_CONTROL);
	et6312b_cancel_timer();

	et6312b_write(led, ET6312B_CHIPCTR, DISABLE);

	for (i = 0; i < 4; i++, j--) {

		et6312b_set_blink(led, i, RED, 500, 1500);

		reg_delay = get_address(led, i, DELAY);
		et6312b_write(led, reg_delay, j * 4);

	}

	et6312b_write(led, ET6312B_CHIPCTR, ENABLE);
	dump_reg(led);
}

static void et6312b_pattern_fota(struct led_sep_ops *ops)
{
	struct et6312b_led *led = led_sep_get_opsdata(ops);
	u8 reg_delay = 0;
	int i = 0;
	int j = 0;

	pr_info("%s\n", __func__);

	et6312b_write(led, ET6312B_CHIPCTR, DISABLE);

	for (i = 0; i < 4; i++, j++) {

		et6312b_set_blink(led, i, WHITE, 250, 750);

		reg_delay = get_address(led, i, DELAY);
		et6312b_write(led, reg_delay, j * 2);

	}

	et6312b_write(led, ET6312B_CHIPCTR, ENABLE);
	dump_reg(led);
}

static int et6312b_light(struct led_sep_ops *ops,
		int id, int on, int color)
{
	struct et6312b_led *led = led_sep_get_opsdata(ops);
	int i, ret = 0;

	pr_info("%s: %d %d color 0x%x\n", __func__,
			id, on, color);

	et6312b_cancel_timer();
	et6312b_write(led, ET6312B_CHIPCTR, DISABLE);

	for (i = 0; i < 4; i++) {
		if (id & (1 << i)) {
			et6312b_color(led, i, color);
			et6312b_workmode(led, i, color,
					on ? ALWAYS_ON : ALWAYS_OFF);
		}
	}

	et6312b_write(led, ET6312B_CHIPCTR, ENABLE);
	return ret;
}

static int et6312b_blink(struct led_sep_ops *ops,
		int id, int color, int on_time, int off_time, int count)
{
	struct et6312b_led *led = led_sep_get_opsdata(ops);
	int total_time = 0;
	int i, ret = 0;

	pr_info("%s: %d %x %d %d %d\n", __func__,
			id, color, on_time, off_time, count);

	et6312b_cancel_timer();

	total_time = on_time + off_time;
	et6312b_write(led, ET6312B_CHIPCTR, DISABLE);

	for (i = 0; i < 4; i++) {
		if (id & (1 << i)) {

			et6312b_set_blink(led, i, color, on_time, off_time);
			et6312b_clear_delay(led, i);

			if (count)
				hrtimer_start(&et6312b_timer[i],
						TIMER_SET(total_time, count),
						HRTIMER_MODE_REL);
		}
	}

	et6312b_write(led, ET6312B_CHIPCTR, ENABLE);
	return ret;
}

static int et6312b_pattern(struct led_sep_ops *ops, int mode)
{
	struct et6312b_led *led = led_sep_get_opsdata(ops);
	int ret = 1;

	pr_info("%s: %s\n", __func__, pattern_string(mode));

	switch (mode) {
	case PATTERN_OFF:
		if (lpcharge)
			et6312b_clear(led);
		break;
	case CHARGING:
		et6312b_light(ops, BATTERY, ALWAYS_ON, RED);
		break;
	case CHARGING_ERR:
		et6312b_blink(ops, BATTERY, RED, 500, 500, 0);
		break;
	case FULLY_CHARGED:
		et6312b_light(ops, BATTERY, ALWAYS_ON, GREEN);
		break;
	case POWERING:
	case POWERING_OFF:
		/*
		 *	19s: boot ani -> powering (6)
		 *		-> we will discard this event.
		 */
		if (led->pattern_count)
			et6312b_pattern_shutdown(ops);
		break;
	case FOTA:
		et6312b_pattern_fota(ops);
		break;
	default:
		ret = 0;
		break;
	}

	pr_info("%s: %s Done, count %d\n", __func__,
			pattern_string(mode),
			led->pattern_count);
	led->pattern_count++;
	return ret;
}

static struct led_sep_ops et6312b_ops = {
	.name = "ET6312B",
	.current_max = 255,
	.current_lowpower = 255,

	.light = et6312b_light,
	.test = et6312b_test,
	.blink = et6312b_blink,
	.reset = et6312b_reset,
	.pattern = et6312b_pattern,
	.capability = (SEP_NODE_PATTERN |
			SEP_NODE_CONTROL |
			SEP_NODE_TEST |
			SEP_NODE_RESET),
};

static void et6312b_5G_finish_count(struct work_struct *work)
{
	et6312b_light(&et6312b_ops, FIVEG, 0, ALWAYS_OFF);
}
static void et6312b_LTE_finish_count(struct work_struct *work)
{
	et6312b_light(&et6312b_ops, LTE, 0, ALWAYS_OFF);
}
static void et6312b_WIFI_finish_count(struct work_struct *work)
{
	et6312b_light(&et6312b_ops, WIFI, 0, ALWAYS_OFF);
}
static void et6312b_BATTERY_finish_count(struct work_struct *work)
{
	et6312b_light(&et6312b_ops, BATTERY, 0, ALWAYS_OFF);
}

static inline enum hrtimer_restart et6312b_5G_timer_func(struct hrtimer *timer)
{
	queue_work(et6312b_workqueue, &et6312b_work[0]);
	return HRTIMER_NORESTART;
}
static enum hrtimer_restart et6312b_LTE_timer_func(struct hrtimer *timer)
{
	queue_work(et6312b_workqueue, &et6312b_work[1]);
	return HRTIMER_NORESTART;
}
static enum hrtimer_restart et6312b_WIFI_timer_func(struct hrtimer *timer)
{
	queue_work(et6312b_workqueue, &et6312b_work[2]);
	return HRTIMER_NORESTART;
}
static enum hrtimer_restart et6312b_BATTERY_timer_func(struct hrtimer *timer)
{
	queue_work(et6312b_workqueue, &et6312b_work[3]);
	return HRTIMER_NORESTART;
}

static int parse_rgb(struct et6312b_led *led,
		char *rgb_str, int led_num)
{
	struct device_node *np = led->i2c->dev.of_node;
	int index = 0;
	int ret = 0;

	ret = of_property_read_u32(np, rgb_str, &index);
	if (ret) {
		pr_info("error for reading %s, %d\n", rgb_str, ret);
		return -EINVAL;
	}

	if (index >= LED_MAX || index < 0) {
		pr_info("out of range for %s. led is changed %d -> %d\n",
				rgb_str, index, led_num);
		index = led_num;
		ret = -EINVAL;
	}

	switch (led_num) {
	case 0: led->rgb[index] = rgb0; break;
	case 1: led->rgb[index] = rgb1; break;
	case 2: led->rgb[index] = rgb2; break;
	case 3: led->rgb[index] = rgb3; break;
	default:
		pr_info("out of range for led%d\n", led_num);
		ret = -EINVAL;
	}
	return ret;
}

static int et6312b_parse_dt(struct et6312b_led *led)
{
	struct device_node *np = led->i2c->dev.of_node;
	const char *power;
	int ret = 0;

	pr_info("%s\n", __func__);

	ret = of_property_read_string(np, "et6312b,power", &power);
	if (ret) {
		pr_info("error for reading et6312b,power\n");
		ret = -EINVAL;
	} else {
		pr_info("power : %s\n", power);
		led->vdd = regulator_get(NULL, power);

		if (IS_ERR_OR_NULL(led->vdd)) {
			pr_err("%s: Failed to get %s regulator.\n",
					__func__, power);
			led->vdd = NULL;
			ret = PTR_ERR(led->vdd);
		}
	}

	ret = parse_rgb(led, "et6312b,rgb0", 0);
	ret = parse_rgb(led, "et6312b,rgb1", 1);
	ret = parse_rgb(led, "et6312b,rgb2", 2);
	ret = parse_rgb(led, "et6312b,rgb3", 3);

	return ret;
}

static int et6312b_led_probe(struct i2c_client *i2c,
		const struct i2c_device_id *i2c_id)
{
	struct et6312b_led *led;
	int led_num, ret;

	dev_info(&i2c->dev, "%s\n", __func__);

	led = devm_kzalloc(&i2c->dev,
			sizeof(struct et6312b_led),
			GFP_KERNEL);
	if (!led) {
		pr_err("%s Not enough memory.\n", __func__);
		return -ENOMEM;
	}
	led->i2c = i2c;
	i2c_set_clientdata(i2c, led);

	ret = et6312b_parse_dt(led);
	if (ret) {
		pr_info("%s: error while parsing values from dt\n", __func__);
		return ret;
	}

	et6312b_power(led, true);
	et6312b_init(led);

	for (led_num = 0; led_num < LED_MAX; led_num++) {
		hrtimer_init(&et6312b_timer[led_num],
				CLOCK_MONOTONIC, HRTIMER_MODE_REL);
#if 0
		et6312b_timer[led_num].function = et6312b_timer_func;
		INIT_WORK(&et6312b_work[i], et6312b_finish_count);
#endif
	}

	et6312b_timer[0].function = et6312b_5G_timer_func;
	INIT_WORK(&et6312b_work[0], et6312b_5G_finish_count);
	et6312b_timer[1].function = et6312b_LTE_timer_func;
	INIT_WORK(&et6312b_work[1], et6312b_LTE_finish_count);
	et6312b_timer[2].function = et6312b_WIFI_timer_func;
	INIT_WORK(&et6312b_work[2], et6312b_WIFI_finish_count);
	et6312b_timer[3].function = et6312b_BATTERY_timer_func;
	INIT_WORK(&et6312b_work[3], et6312b_BATTERY_finish_count);

	et6312b_workqueue = create_singlethread_workqueue("et6312b");

	led_sep_set_opsdata(&et6312b_ops, led);
	led_sep_register_device(&et6312b_ops);

	dev_info(&i2c->dev, "%s power on -> Done\n", __func__);

	return 0;
}

static int et6312b_led_remove(struct i2c_client *i2c)
{
	struct et6312b_led *led = i2c_get_clientdata(i2c);

	dev_info(&i2c->dev, "%s\n", __func__);

	destroy_workqueue(et6312b_workqueue);
	et6312b_clear(led);
	regulator_put(led->vdd);
	dev_info(&i2c->dev, "%s Done\n", __func__);
	return 0;
}

static void et6312b_led_shutdown(struct i2c_client *i2c)
{
	dev_info(&i2c->dev, "%s\n", __func__);
	et6312b_led_remove(i2c);
	dev_info(&i2c->dev, "%s Done\n", __func__);
}

static const struct i2c_device_id et6312b_i2c_id[] = {
	{ "ET6312B", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, et6312b_i2c_id);

static const struct of_device_id et6312b_led_of_match[] = {
	{ .compatible = "samsung,et6312b", },
	{ }
};

static struct i2c_driver et6312b_led_driver = {
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "sled_et6312b",
		.of_match_table = of_match_ptr(et6312b_led_of_match),
	},
	.probe	= et6312b_led_probe,
	.remove	= et6312b_led_remove,
	.shutdown = et6312b_led_shutdown,
	.id_table	= et6312b_i2c_id,
};

static int __init et6312b_i2c_init(void)
{
	pr_info("%s\n", __func__);
	return i2c_add_driver(&et6312b_led_driver);
}
module_init(et6312b_i2c_init);

static void __exit et6312b_i2c_exit(void)
{
	i2c_del_driver(&et6312b_led_driver);
}
module_exit(et6312b_i2c_exit);

MODULE_AUTHOR("Samsung EIF Team");
