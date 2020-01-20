/*
 * copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Core file for Samsung EXYNOS TSMUX driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef TSMUX_DBG_H
#define TSMUX_DBG_H

extern int g_tsmux_debug_level;

#define TSMUX_DBG_SFR		(1 << 5)
#define TSMUX_SFR		(1 << 4)
#define TSMUX_OTF		(1 << 3)
#define TSMUX_M2M		(1 << 2)
#define TSMUX_COMMON		(1 << 1)
#define TSMUX_ERR		(1)

#define print_tsmux(level, fmt, args...)			\
	do {							\
		if ((g_tsmux_debug_level & level))		\
			pr_info("%s:%d: " fmt,			\
				__func__, __LINE__, ##args);	\
	} while (0)
#endif
