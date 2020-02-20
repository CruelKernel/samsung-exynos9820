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
#include <linux/gpio_keys.h>
#include <linux/sec_debug.h>
#ifdef CONFIG_SEC_KEY_NOTIFIER
#include "../samsung/sec_key_notifier.h"
#endif

/* Input sequence 9530 */
#define CRASH_COUNT_FIRST 3
#define CRASH_COUNT_SECOND 3
#define CRASH_COUNT_THIRD 3

#define KEY_STATE_DOWN 1
#define KEY_STATE_UP 0

struct tsp_dump_callbacks dump_callbacks;

struct crash_key {
	unsigned int key_code;
	unsigned int crash_count;
};

struct crash_key tsp_dump_key_combination[] = {
	{KEY_VOLUMEDOWN, CRASH_COUNT_FIRST},
	{KEY_POWER, CRASH_COUNT_SECOND},
	{KEY_VOLUMEDOWN, CRASH_COUNT_THIRD},
};

struct tsp_dump_key_state {
	unsigned int key_code;
	unsigned int state;
};

struct tsp_dump_key_state tsp_dump_key_states[] = {
	{KEY_VOLUMEDOWN, KEY_STATE_UP},
	{KEY_VOLUMEUP, KEY_STATE_UP},
	{KEY_POWER, KEY_STATE_UP},
	{KEY_HOMEPAGE, KEY_STATE_UP},
};

#ifdef CONFIG_SEC_KEY_NOTIFIER
static unsigned int __tsp_dump_keys[] = {
	KEY_POWER,
	KEY_VOLUMEDOWN,
	KEY_VOLUMEUP,
	KEY_HOMEPAGE
};
#endif

static unsigned int hold_key = KEY_VOLUMEUP;
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
	return tsp_dump_key_combination[check_step].key_code;
}

static int is_key_matched_for_current_step(unsigned int code)
{
	return (code == get_current_step_key_code());
}

static int is_crash_keys(unsigned int code)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(tsp_dump_key_states); i++)
		if (tsp_dump_key_states[i].key_code == code)
			return 1;
	return 0;
}

static size_t get_count_for_next_step(void)
{
	size_t i;
	size_t count = 0;

	for (i = 0; i < check_step + 1; i++)
		count += tsp_dump_key_combination[i].crash_count;
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

	for (i = 0; i < ARRAY_SIZE(tsp_dump_key_combination); i++)
		count += tsp_dump_key_combination[i].crash_count;
	return count - 1;
}

static unsigned int is_key_state_down(unsigned int code)
{
	size_t i;

	if (is_hold_key(code))
		return is_hold_key_hold();

	for (i = 0; i < ARRAY_SIZE(tsp_dump_key_states); i++)
		if (tsp_dump_key_states[i].key_code == code)
			return tsp_dump_key_states[i].state == KEY_STATE_DOWN;
	return 0;
}

static void set_key_state_down(unsigned int code)
{
	size_t i;

	if (is_hold_key(code))
		set_hold_key_hold(KEY_STATE_DOWN);

	for (i = 0; i < ARRAY_SIZE(tsp_dump_key_states); i++)
		if (tsp_dump_key_states[i].key_code == code)
			tsp_dump_key_states[i].state = KEY_STATE_DOWN;
}

static void set_key_state_up(unsigned int code)
{
	size_t i;

	if (is_hold_key(code))
		set_hold_key_hold(KEY_STATE_UP);

	for (i = 0; i < ARRAY_SIZE(tsp_dump_key_states); i++)
		if (tsp_dump_key_states[i].key_code == code)
			tsp_dump_key_states[i].state = KEY_STATE_UP;
}

static void increase_step(void)
{
	if (check_step < ARRAY_SIZE(tsp_dump_key_combination))
		check_step++;
	else if (dump_callbacks.inform_dump)
		dump_callbacks.inform_dump();
}

static void reset_step(void)
{
	check_step = 0;
}

static void increase_count(void)
{
	if (check_count < get_count_for_panic())
		check_count++;
	else if (dump_callbacks.inform_dump)
		dump_callbacks.inform_dump();
}

static void reset_count(void)
{
	check_count = 0;
}

static int check_tsp_crash_keys(struct notifier_block *nb,
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
				reset_count();
				reset_step();
			}
			if (is_reaching_count_for_next_step())
				increase_step();
		}

	} else {
		set_key_state_up(code);
		if (is_hold_key(code)) {
			set_hold_key_hold(KEY_STATE_UP);
			reset_step();
			reset_count();
		}
	}
	return NOTIFY_OK;
}

#ifndef CONFIG_SEC_KEY_NOTIFIER
static struct notifier_block nb_gpio_keys = {
	.notifier_call = check_tsp_crash_keys
};
#else
static struct notifier_block seccmn_tsp_crash_key_notifier = {
	.notifier_call = check_tsp_crash_keys
};
#endif

static int __init sec_tsp_dumpkey_init(void)
{
	/* only work for debug level is low */
//	if (!sec_debug_enter_upload())
#ifndef CONFIG_SEC_KEY_NOTIFIER
	register_gpio_keys_notifier(&nb_gpio_keys);
#else
	sec_kn_register_notifier(&seccmn_tsp_crash_key_notifier,
			__tsp_dump_keys, ARRAY_SIZE(__tsp_dump_keys));
#endif
	return 0;
}

late_initcall(sec_tsp_dumpkey_init);
