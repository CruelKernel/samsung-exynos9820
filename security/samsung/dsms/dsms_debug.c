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

#define LOG_LINE_MAX 1008 /* to not exceed the frame size, 1024 */

void dsms_log_write(int loglevel, const char* format, ...)
{
	va_list ap;
	char log[LOG_LINE_MAX];

	va_start(ap, format);
	vsnprintf(log, sizeof(log), format, ap);

	switch (loglevel) {
	case LOG_ERROR:
		pr_err(DSMS_TAG "%s", log);
		break;
	case LOG_DEBUG:
		pr_debug(DSMS_TAG "%s", log);
		break;
	default: /* LOG_INFO or others */
		pr_info(DSMS_TAG "%s", log);
		break;
	}
	va_end(ap);
}
