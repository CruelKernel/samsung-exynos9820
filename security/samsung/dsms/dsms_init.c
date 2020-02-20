/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <asm/uaccess.h>

#include <linux/dsms.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/stddef.h>
#include <linux/string.h>

#include "dsms_debug.h"
#include "dsms_init.h"
#include "dsms_rate_limit.h"

static int is_dsms_initialized_flag = false;

int dsms_is_initialized(void)
{
	return is_dsms_initialized_flag;
}

static __init int dsms_init(void)
{
	dsms_log_write(LOG_INFO, "Started.");
	dsms_rate_limit_init();
	is_dsms_initialized_flag = true;
	return 0;
}

module_init(dsms_init);
