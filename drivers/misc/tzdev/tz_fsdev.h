/*
 * Copyright (C) 2017 Samsung Electronics, Inc.
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

#ifndef _TZ_FSDEV_H_
#define _TZ_FSDEV_H_

#include "tzdev.h"

#define TZ_FSDEV_NAME			"nwfsdev"

#define TZ_FSDEV_IOC_MAGIC		'f'
#define TZ_FSDEV_REG_DAEMON		_IO(TZ_FSDEV_IOC_MAGIC, 0)
#define TZ_FSDEV_UNEXPECTED_EXIT	_IOR(TZ_FSDEV_IOC_MAGIC, 1, struct tz_fsdev_child_exit)
#define TZ_FSDEV_GET_CMD		_IOW(TZ_FSDEV_IOC_MAGIC, 2, struct tz_fsdev_data)
#define TZ_FSDEV_REPLY			_IOR(TZ_FSDEV_IOC_MAGIC, 3, struct tz_fsdev_reply)
#define TZ_FSDEV_CREATE_SESSION_BUF	_IOR(TZ_FSDEV_IOC_MAGIC, 4, uint32_t)
#define TZ_FSDEV_FREE_SESSION_BUF	_IO(TZ_FSDEV_IOC_MAGIC, 5)
#define TZ_FSDEV_GET_WP_CMD		_IOW(TZ_FSDEV_IOC_MAGIC, 6, struct tz_fsdev_wp_command)
#define TZ_FSDEV_REPLY_WP_CMD		_IOR(TZ_FSDEV_IOC_MAGIC, 7, struct tz_fsdev_wp_reply)
#ifdef CONFIG_COMPAT
#define COMPAT_TZ_FSDEV_GET_WP_CMD	_IOW(TZ_FSDEV_IOC_MAGIC, 6, struct compat_tz_fsdev_wp_command)
#define COMPAT_TZ_FSDEV_REPLY_WP_CMD	_IOR(TZ_FSDEV_IOC_MAGIC, 7, struct compat_tz_fsdev_wp_reply)
#endif /* CONFIG_COMPAT */

enum {
	NWFS_CMD_CREATE_SESSION,
	NWFS_CMD_CLOSE_SESSION,
	NWFS_CMD_OPEN_FILE,
	NWFS_CMD_CLOSE_FILE,
	NWFS_CMD_READ_FILE,
	NWFS_CMD_READ_FILE_CONTINUE,
	NWFS_CMD_CNT,
};

/* nwfs_daemon <--> fsdev */
struct tz_fsdev_data {
	uint32_t session_id;
	uint32_t cmd;
} __packed;

struct tz_fsdev_reply {
	uint32_t session_id;
	pid_t pid;
} __packed;

struct tz_fsdev_child_exit {
	pid_t pid;
	int32_t status;
} __packed;

/* WP <--> fsdev*/
struct tz_fsdev_wp_command {
	uint32_t cmd;
	uint32_t size;
	void *buf;
} __packed;

struct tz_fsdev_wp_reply {
	uint32_t ret;
	uint32_t size;
	uint32_t is_new_buf;
	void *buf;
} __packed;

#ifdef CONFIG_COMPAT
struct compat_tz_fsdev_wp_command {
	uint32_t cmd;
	uint32_t size;
	uint32_t buf;
} __packed;

struct compat_tz_fsdev_wp_reply {
	uint32_t ret;
	uint32_t size;
	uint32_t is_new_buf;
	uint32_t buf;
} __packed;
#endif /* CONFIG_COMPAT */

#ifdef CONFIG_TZ_NWFS
int tz_fsdev_initialize(void);
#else /* CONFIG_TZ_NWFS */
int tz_fsdev_initialize(void)
{
	return 0;
}
#endif /* CONFIG_TZ_NWFS */

#endif /* _TZ_FSDEV_H_ */
