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

#ifndef __TZ_REE_TIME_H__
#define __TZ_REE_TIME_H__

#define TZ_REE_TIME_SOCK_NAME	"ree_time_socket"

struct tz_ree_time {
	uint32_t sec;
	uint32_t nsec;
} __attribute__((__packed__));

#endif /* __TZ_REE_TIME_H__ */
