/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * Samsung TN debugging code
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/gpio_keys.h>
#include <linux/sec_debug.h>
#include <linux/debug-snapshot.h>

static int sec_crash_key_check_keys(struct notifier_block *nb,
				    unsigned long c, void *v)
{
	unsigned int code = (unsigned int)c;
	int state = *(int *)v;

	dbg_snapshot_check_crash_key(code, state);

	return NOTIFY_DONE;
}

static struct notifier_block nb_gpio_keys = {
	.notifier_call = sec_crash_key_check_keys
};

int __init sec_crash_key_init(void)
{
	/* only work when upload enabled*/
	if (sec_debug_enter_upload()) {
		register_gpio_keys_notifier(&nb_gpio_keys);
		pr_info("%s: force upload key registered\n", __func__);
	}
	return 0;
}

early_initcall(sec_crash_key_init);
