/*
 * Copyright (C) 2012-2017, Samsung Electronics Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __TZLOG_H__
#define __TZLOG_H__

extern unsigned int tzdev_verbosity;
extern unsigned int tzdev_teec_verbosity;
extern unsigned int tzdev_kthread_verbosity;
extern unsigned int tzdev_uiwsock_verbosity;

enum {
	TZDEV_LOG_LEVEL_ERROR,
	TZDEV_LOG_LEVEL_INFO,
	TZDEV_LOG_LEVEL_DEBUG,
};

static inline const char *tz_print_prefix(unsigned int lvl)
{
	switch (lvl) {
	case TZDEV_LOG_LEVEL_ERROR:
		return "ERR";
	case TZDEV_LOG_LEVEL_INFO:
		return "INF";
	case TZDEV_LOG_LEVEL_DEBUG:
		return "DBG";
	default:
		return "???";
	}
}

#define tz_print(lvl, param, fmt, ...)						\
	do {									\
		if (lvl <= param)						\
			printk("TZDEV> %s %s(%d): "fmt,				\
					tz_print_prefix(lvl),			\
					__func__, __LINE__, ##__VA_ARGS__);	\
	} while (0)

#define tzdev_print(lvl, fmt, ...)	tz_print(lvl, tzdev_verbosity, fmt,  ##__VA_ARGS__)

#define tzdev_teec_debug(fmt, ...)	tz_print(TZDEV_LOG_LEVEL_DEBUG, tzdev_teec_verbosity, fmt, ##__VA_ARGS__)
#define tzdev_teec_info(fmt, ...)	tz_print(TZDEV_LOG_LEVEL_INFO, tzdev_teec_verbosity, fmt, ##__VA_ARGS__)
#define tzdev_teec_error(fmt, ...)	tz_print(TZDEV_LOG_LEVEL_ERROR, tzdev_teec_verbosity, fmt, ##__VA_ARGS__)

#define tzdev_kthread_debug(fmt, ...)	tz_print(TZDEV_LOG_LEVEL_DEBUG, tzdev_kthread_verbosity, fmt, ##__VA_ARGS__)
#define tzdev_kthread_info(fmt, ...)	tz_print(TZDEV_LOG_LEVEL_INFO, tzdev_kthread_verbosity, fmt, ##__VA_ARGS__)
#define tzdev_kthread_error(fmt, ...)	tz_print(TZDEV_LOG_LEVEL_ERROR, tzdev_kthread_verbosity, fmt, ##__VA_ARGS__)

#define tzdev_uiwsock_debug(fmt, ...)	tz_print(TZDEV_LOG_LEVEL_DEBUG, tzdev_uiwsock_verbosity, fmt, ##__VA_ARGS__)
#define tzdev_uiwsock_info(fmt, ...)	tz_print(TZDEV_LOG_LEVEL_INFO, tzdev_uiwsock_verbosity, fmt, ##__VA_ARGS__)
#define tzdev_uiwsock_error(fmt, ...)	tz_print(TZDEV_LOG_LEVEL_ERROR, tzdev_uiwsock_verbosity, fmt, ##__VA_ARGS__)

#endif /* __TZLOG_H__ */
