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

#ifndef __TZ_IWLOG_POLLING_H__
#define __TZ_IWLOG_POLLING_H__

#if defined(CONFIG_TZLOG_POLLING)
void tz_iwlog_schedule_delayed_work(void);
void tz_iwlog_cancel_delayed_work(void);
#else
static inline void tz_iwlog_schedule_delayed_work(void)
{
}

static inline void tz_iwlog_cancel_delayed_work(void)
{
}
#endif

#endif /* __TZ_IWLOG_POLLING_H__ */
