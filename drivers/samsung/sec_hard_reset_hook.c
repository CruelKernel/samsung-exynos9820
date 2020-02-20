/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/atomic.h>
#include <linux/sec_hard_reset_hook.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif
#include <linux/moduleparam.h>
#ifndef CONFIG_SEC_KEY_NOTIFIER
#include <linux/gpio_keys.h>
#else
#include "sec_key_notifier.h"
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

struct gpio_key_info {
	unsigned int keycode;
	int gpio;
	bool active_low;
};

static unsigned int hard_reset_keys[] = { KEY_POWER, KEY_VOLUMEDOWN };
static struct gpio_key_info keys_info[ARRAY_SIZE(hard_reset_keys)];

static atomic_t hold_keys = ATOMIC_INIT(0);
static ktime_t hold_time;
static struct hrtimer hard_reset_hook_timer;
static bool hard_reset_occurred;
static int all_pressed;

/* Proc node to enable hard reset */
static bool hard_reset_hook_enable = 1;
module_param_named(hard_reset_hook_enable, hard_reset_hook_enable, bool, 0664);
MODULE_PARM_DESC(hard_reset_hook_enable, "1: Enabled, 0: Disabled");

static bool is_hard_reset_key(unsigned int code)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(hard_reset_keys); i++)
		if (code == hard_reset_keys[i])
			return true;
	return false;
}

static int hard_reset_key_set(unsigned int code)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(hard_reset_keys); i++)
		if (code == hard_reset_keys[i])
			atomic_or(0x1 << i, &hold_keys);
	return atomic_read(&hold_keys);
}

static int hard_reset_key_unset(unsigned int code)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(hard_reset_keys); i++)
		if (code == hard_reset_keys[i])
			atomic_and(~(0x1) << i, &hold_keys);

	return atomic_read(&hold_keys);
}

static int hard_reset_key_all_pressed(void)
{
	return (atomic_read(&hold_keys) == all_pressed);
}

static int get_gpio_info(unsigned int code, int *gpio, bool *active_low)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(keys_info); i++)
		if (code == keys_info[i].keycode) {
			*gpio = keys_info[i].gpio;
			*active_low = keys_info[i].active_low;
			return 0;
		}
	return -1;
}

static bool is_pressed_gpio_key(int gpio, bool active_low)
{
	if (!gpio_is_valid(gpio))
		return false;

	if ((gpio_get_value(gpio) ? 1 : 0) ^ active_low)
		return true;
	else
		return false;
}

static bool is_gpio_keys_all_pressed(void)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(hard_reset_keys); i++) {
		int gpio;
		bool active_low;

		if (get_gpio_info(hard_reset_keys[i], &gpio, &active_low))
			return false;
		pr_info("%s:key:%d, gpio:%d, active:%d\n", __func__,
			hard_reset_keys[i], gpio, active_low);

		if (!is_pressed_gpio_key(gpio, active_low)) {
			pr_warn("[%d] is not pressed\n", hard_reset_keys[i]);
			return false;
		}
	}
	return true;
}

static enum hrtimer_restart hard_reset_hook_callback(struct hrtimer *hrtimer)
{
	if (!is_gpio_keys_all_pressed()) {
		pr_warn("All gpio keys are not pressed\n");
		return HRTIMER_NORESTART;
	}

	pr_err("Hard Reset\n");
	hard_reset_occurred = true;
	BUG();
	return HRTIMER_RESTART;
}

static int load_gpio_key_info(void)
{
#ifdef CONFIG_OF
	size_t i;
	struct device_node *np, *pp;
	static int nk;

	np = of_find_node_by_path("/gpio_keys");
	if (!np)
		return -1;
	for_each_child_of_node(np, pp) {
		uint keycode = 0;

		if (!of_find_property(pp, "gpios", NULL))
			continue;
		if (of_property_read_u32(pp, "linux,code", &keycode))
			continue;
		for (i = 0; i < ARRAY_SIZE(hard_reset_keys); ++i) {
			if (keycode == hard_reset_keys[i]) {
				enum of_gpio_flags flags;

				keys_info[nk].keycode = keycode;
				keys_info[nk].gpio = of_get_gpio_flags(pp, 0, &flags);
				if (gpio_is_valid(keys_info[nk].gpio))
					keys_info[nk].active_low = flags & OF_GPIO_ACTIVE_LOW;
				nk++;
				break;
			}
		}
	}
	of_node_put(np);
#endif
	return 0;
}

static int hard_reset_hook(struct notifier_block *nb,
			   unsigned long type, void *data)
{
#ifndef CONFIG_SEC_KEY_NOTIFIER
	unsigned int code = (unsigned int)type;
	int pressed = *(int *)data;
#else
	struct sec_key_notifier_param *param = data;
	unsigned int code = param->keycode;
	int pressed = param->down;
#endif
	if (unlikely(!hard_reset_hook_enable))
		return NOTIFY_DONE;

	if (!is_hard_reset_key(code))
		return NOTIFY_DONE;

	if (pressed)
		hard_reset_key_set(code);
	else
		hard_reset_key_unset(code);

	if (hard_reset_key_all_pressed()) {
		hrtimer_start(&hard_reset_hook_timer,
			      hold_time, HRTIMER_MODE_REL);		
		pr_info("%s : hrtimer_start\n", __func__);
	}
	else {
		hrtimer_try_to_cancel(&hard_reset_hook_timer);
	}

	return NOTIFY_OK;
}

#ifndef CONFIG_SEC_KEY_NOTIFIER
static struct notifier_block nb_gpio_keys = {
	.notifier_call = hard_reset_hook
};
#else
static struct notifier_block seccmn_hard_reset_notifier = {
	.notifier_call = hard_reset_hook
};
#endif

bool is_hard_reset_occurred(void)
{
	return hard_reset_occurred;
}

void hard_reset_delay(void)
{
	/* HQE team request hard reset key should guarantee 7 seconds.
	 * To get proper stack, hard reset hook starts after 6 seconds.
	 * And it will reboot before 7 seconds.
	 * Add delay to keep the 7 seconds
	 */
	if (hard_reset_occurred) {
		pr_err("wait until PMIC reset occurred");
		mdelay(2000);
	}
}

int __init hard_reset_hook_init(void)
{
	size_t i;

	hrtimer_init(&hard_reset_hook_timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
	hard_reset_hook_timer.function = hard_reset_hook_callback;
	hold_time = ktime_set(6, 0); /* 6 seconds */

	for (i = 0; i < ARRAY_SIZE(hard_reset_keys); i++)
		all_pressed |= 0x1 << i;
	load_gpio_key_info();
#ifndef CONFIG_SEC_KEY_NOTIFIER
	register_gpio_keys_notifier(&nb_gpio_keys);
#else
	sec_kn_register_notifier(&seccmn_hard_reset_notifier,
			hard_reset_keys, ARRAY_SIZE(hard_reset_keys));
#endif

	return 0;
}

late_initcall(hard_reset_hook_init);
