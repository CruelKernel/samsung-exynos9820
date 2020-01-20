/*
 * Samsung Exynos SoC series SCore driver
 *
 * Definition of log type for SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_LOG_H__
#define __SCORE_LOG_H__

#include <linux/printk.h>

/* Enable entire call path log */
//#define DBG_CALL_PATH_LOG
/* Enable debug log */
//#define ENABLE_DBG_LOG

/**
 * Definition of log to add prefix for SCore
 */
#define score_info(fmt, args...) \
	pr_info("SCore:" fmt, ##args)
#define score_note(fmt, args...) \
	pr_info("SCore:[LOG](%d):" fmt, __LINE__, ##args)
#define score_warn(fmt, args...) \
	pr_warn("SCore:[WRN](%d):" fmt, __LINE__, ##args)
#define score_err(fmt, args...) \
	pr_err("SCore:[ERR](%d):" fmt, __LINE__, ##args)

#if defined(ENABLE_DBG_LOG)
#define score_dbg(fmt, args...) \
	pr_info("SCore: [DBG](%d): " fmt, __LINE__, ##args)
#else
#define score_dbg(fmt, args...)
#endif

#if defined(DBG_CALL_PATH_LOG)
#define score_enter()			score_note("enter (%s)\n", __func__)
#define score_leave()			score_note("leave (%s)\n", __func__)
#define score_check()			score_note("check (%s)\n", __func__)
#else
#define score_enter()
#define score_leave()
#define score_check()
#endif

#endif
