/*
 * drivers/soc/samsung/exynos-hdcp/exynos-hdcp2-log.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __EXYNOS_HDCP2_LOG_H__
#define __EXYNOS_HDCP2_LOG_H__

#include <linux/dp_logger.h>

#undef HDCP_DEBUG

#ifdef HDCP_DEBUG
#define hdcp_debug(fmt, args...)				\
	do {							\
		printk(KERN_ERR "[HDCP2]%s:%d: " fmt,	\
				__func__, __LINE__, ##args);	\
	} while (0)
#else
#define hdcp_debug(fmt, args...)
#endif

#define hdcp_err(fmt, args...)				\
	do {						\
		printk(KERN_ERR "[HDCP2]%s:%d: " fmt,	\
		       __func__, __LINE__, ##args);	\
	} while (0)

#define hdcp_info(fmt, args...)				\
	do {						\
		printk(KERN_INFO "[HDCP2]%s:%d: " fmt,	\
			__func__, __LINE__, ##args);	\
	} while (0)
#endif
