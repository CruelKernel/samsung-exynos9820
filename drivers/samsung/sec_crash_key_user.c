/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * Samsung TN debugging code
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/sec_debug.h>
#ifndef CONFIG_SEC_KEY_NOTIFIER
#include <linux/gpio_keys.h>
#else
#include "sec_key_notifier.h"
#endif

/* Input sequence 9530 */
#define CRASH_COUNT_FIRST 9
#define CRASH_COUNT_SECOND 5
#define CRASH_COUNT_THIRD 3

#define KEY_STATE_DOWN 1
#define KEY_STATE_UP 0

/* #define DEBUG */

#ifdef DEBUG
#define SEC_LOG(fmt, args...) pr_info("%s:%s: " fmt "\n", \
		"sec_upload", __func__, ##args)
#else
#define SEC_LOG(fmt, args...) do { } while (0)
#endif

struct crash_key {
	unsigned int key_code;
	unsigned int crash_count;
};

struct crash_key user_crash_key_combination[] = {
	{KEY_POWER, CRASH_COUNT_FIRST},
	{KEY_VOLUMEUP, CRASH_COUNT_SECOND},
	{KEY_POWER, CRASH_COUNT_THIRD},
};

struct upload_key_state {
	unsigned int key_code;
	unsigned int state;
};

struct upload_key_state upload_key_states[] = {
	{KEY_VOLUMEDOWN, KEY_STATE_UP},
	{KEY_VOLUMEUP, KEY_STATE_UP},
	{KEY_POWER, KEY_STATE_UP},
};

#ifdef CONFIG_SEC_KEY_NOTIFIER
static unsigned int user_crash_keys[] = {
	KEY_POWER,
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN
};
#endif

static unsigned int hold_key = KEY_VOLUMEDOWN;
static unsigned int hold_key_hold = KEY_STATE_UP;
static size_t check_count;
static size_t check_step;

static int is_hold_key(unsigned int code)
{
	return (code == hold_key);
}

static void set_hold_key_hold(int state)
{
	hold_key_hold = state;
}

static unsigned int is_hold_key_hold(void)
{
	return hold_key_hold;
}

static unsigned int get_current_step_key_code(void)
{
	return user_crash_key_combination[check_step].key_code;
}

static int is_key_matched_for_current_step(unsigned int code)
{
	SEC_LOG("%d == %d", code, get_current_step_key_code());
	return (code == get_current_step_key_code());
}

static int is_crash_keys(unsigned int code)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(upload_key_states); i++)
		if (upload_key_states[i].key_code == code)
			return 1;
	return 0;
}

static size_t get_count_for_next_step(void)
{
	size_t i;
	size_t count = 0;

	for (i = 0; i < check_step + 1; i++)
		count += user_crash_key_combination[i].crash_count;
	SEC_LOG("%zu", count);
	return count;
}

static int is_reaching_count_for_next_step(void)
{
	return (check_count == get_count_for_next_step());
}

static size_t get_count_for_panic(void)
{
	size_t i;
	size_t count = 0;

	for (i = 0; i < ARRAY_SIZE(user_crash_key_combination); i++)
		count += user_crash_key_combination[i].crash_count;
	return count - 1;
}

static unsigned int is_key_state_down(unsigned int code)
{
	size_t i;

	if (is_hold_key(code))
		return is_hold_key_hold();

	for (i = 0; i < ARRAY_SIZE(upload_key_states); i++)
		if (upload_key_states[i].key_code == code)
			return upload_key_states[i].state == KEY_STATE_DOWN;
	/* Do not reach here */
	panic("Invalid Keycode");
}

static void set_key_state_down(unsigned int code)
{
	size_t i;

	if (is_hold_key(code))
		set_hold_key_hold(KEY_STATE_DOWN);

	for (i = 0; i < ARRAY_SIZE(upload_key_states); i++)
		if (upload_key_states[i].key_code == code)
			upload_key_states[i].state = KEY_STATE_DOWN;
	SEC_LOG("code %d", code);
}

static void set_key_state_up(unsigned int code)
{
	size_t i;

	if (is_hold_key(code))
		set_hold_key_hold(KEY_STATE_UP);

	for (i = 0; i < ARRAY_SIZE(upload_key_states); i++)
		if (upload_key_states[i].key_code == code)
			upload_key_states[i].state = KEY_STATE_UP;
}

static void increase_step(void)
{
	if (check_step < ARRAY_SIZE(user_crash_key_combination))
		check_step++;
	else
		panic("User Crash key");
	SEC_LOG("%zu", check_step);
}

static void reset_step(void)
{
	check_step = 0;
	SEC_LOG("");
}

static void increase_count(void)
{
	if (check_count < get_count_for_panic())
		check_count++;
	else
		panic("User Crash Key");
	SEC_LOG("%zu < %zu", check_count, get_count_for_panic());
}

static void reset_count(void)
{
	check_count = 0;
	SEC_LOG("");
}

static int check_crash_keys_in_user(struct notifier_block *nb,
				unsigned long type, void *data)
{
#ifndef CONFIG_SEC_KEY_NOTIFIER
	unsigned int code = (unsigned int)type;
	int state = *(int *)data;
#else
	struct sec_key_notifier_param *param = data;
	unsigned int code = param->keycode;
	int state = param->down;
#endif

	if (!is_crash_keys(code))
		return NOTIFY_DONE;

	if (state == KEY_STATE_DOWN) {
		/* Duplicated input */
		if (is_key_state_down(code))
			return NOTIFY_DONE;
		set_key_state_down(code);

		if (is_hold_key(code)) {
			set_hold_key_hold(KEY_STATE_DOWN);
			return NOTIFY_DONE;
		}
		if (is_hold_key_hold()) {
			if (is_key_matched_for_current_step(code)) {
				increase_count();
			} else {
				pr_info("%s: crash key reset\n", "sec_upload");
				reset_count();
				reset_step();
			}
			if (is_reaching_count_for_next_step())
				increase_step();
		}

	} else {
		set_key_state_up(code);
		if (is_hold_key(code)) {
			pr_info("%s: crash key reset\n", "sec_upload");
			set_hold_key_hold(KEY_STATE_UP);
			reset_step();
			reset_count();
		}
	}
	return NOTIFY_OK;
}

#ifndef CONFIG_SEC_KEY_NOTIFIER
static struct notifier_block nb_gpio_keys = {
	.notifier_call = check_crash_keys_in_user
};
#else
static struct notifier_block seccmn_user_crash_key_notifier = {
	.notifier_call = check_crash_keys_in_user
};
#endif

int __init sec_upload_init(void)
{
	/* only work for debug level is low */
	if (!sec_debug_enter_upload())
#ifndef CONFIG_SEC_KEY_NOTIFIER
		register_gpio_keys_notifier(&nb_gpio_keys);
#else
		sec_kn_register_notifier(&seccmn_user_crash_key_notifier,
				user_crash_keys, ARRAY_SIZE(user_crash_keys));
#endif

	return 0;
}

early_initcall(sec_upload_init);
