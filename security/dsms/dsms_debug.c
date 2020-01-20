/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/dsms.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/types.h>

#include "dsms_debug.h"

#ifdef DSMS_DEBUG_ENABLE

#define MAX_ALLOWED_DETAIL_LENGTH 1024

void dsms_debug_message(const char *feature_code, const char *detail,
	int64_t value)
{
	size_t len = strnlen(detail, MAX_ALLOWED_DETAIL_LENGTH);
	printk(DSMS_DEBUG_TAG "{'%s', '%s' (%zu bytes), %lld}", feature_code,
		detail, len, value);
}

#endif //DSMS_DEBUG_ENABLE
