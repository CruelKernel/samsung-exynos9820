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

#ifndef __TZ_IWLOG_H__
#define __TZ_IWLOG_H__

#if defined(CONFIG_TZLOG)
int tz_iwlog_initialize(void);
void tz_iwlog_read_buffers(void);
#else
static inline int tz_iwlog_initialize(void)
{
	return 0;
}

static inline void tz_iwlog_read_buffers(void)
{
}
#endif

#endif /* __TZ_IWLOG_H__ */
