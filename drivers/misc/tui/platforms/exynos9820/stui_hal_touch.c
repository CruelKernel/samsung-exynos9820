/*
 * Copyright (c) 2015-2018 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "stui_core.h"
#include "stui_hal.h"

static int touch_requested;
extern int stui_tsp_enter(void);
extern int stui_tsp_exit(void);

static int request_touch(void)
{
	int ret = 0;

	if (touch_requested == 1)
		return -EALREADY;

	ret = stui_tsp_enter();
	if (ret) {
		pr_err("[STUI] stui_tsp_enter failed:%d\n", ret);
		return ret;
	}

	touch_requested = 1;
	printk(KERN_DEBUG "[STUI] Touch requested\n");

	return ret;
}

static int release_touch(void)
{
	int ret = 0;

	if (touch_requested != 1)
		return -EALREADY;

	ret = stui_tsp_exit();
	if (ret) {
		pr_err("[STUI] stui_tsp_exit failed : %d\n", ret);
		return ret;
	}

	touch_requested = 0;
	printk(KERN_DEBUG "[STUI] Touch release\n");

	return ret;
}

int stui_i2c_protect(bool is_protect)
{
	int ret;

	printk(KERN_DEBUG "[STUI] %s(%s) called\n",
			__func__, is_protect ? "true" : "false");

	if (is_protect)
		ret = request_touch();
	else
		ret = release_touch();

	return ret;
}
