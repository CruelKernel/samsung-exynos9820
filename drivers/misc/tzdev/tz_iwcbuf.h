/*
 * Copyright (C) 2013-2016 Samsung Electronics, Inc.
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

#ifndef __TZ_IWCBUF_H__
#define __TZ_IWCBUF_H__

struct tz_iwcbuf {
	uint32_t write_count;
	uint32_t read_count;
	char buffer[];
} __packed;

#endif /* __TZ_IWCBUF_H__ */
