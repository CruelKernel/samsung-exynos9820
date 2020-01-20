/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *             http://www.samsung.com
 *
 * Header file for Exynos REPEATER driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _REPEATER_DBG_H_
#define _REPEATER_DBG_H_

extern int g_repeater_debug_level;

#define RPT_SHR_BUF_INFO		3
#define RPT_INT_INFO			2
#define RPT_EXT_INFO			1
#define RPT_ERROR				0

#define print_repeater_debug(level, fmt, args...)		\
	do {                                                    \
		if (g_repeater_debug_level >= level)		\
			pr_info("%s:%d: " fmt,        \
				__func__, __LINE__, ##args);    \
	} while (0)

#endif
