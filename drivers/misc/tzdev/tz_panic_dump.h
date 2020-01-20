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

#ifndef __TZ_PANIC_DUMP_H__
#define __TZ_PANIC_DUMP_H__

#include <linux/types.h>

#define TZ_PANIC_DUMP_IOC_MAGIC		'c'
#define TZ_PANIC_DUMP_GET_SIZE		_IOW(TZ_PANIC_DUMP_IOC_MAGIC, 0, __u32)

#if defined(CONFIG_TZ_PANIC_DUMP)
int tz_panic_dump_alloc_buffer(void);
#else
static inline int tz_panic_dump_alloc_buffer(void)
{
	return 0;
}
#endif

#endif /* __TZ_PANIC_DUMP_H__ */
