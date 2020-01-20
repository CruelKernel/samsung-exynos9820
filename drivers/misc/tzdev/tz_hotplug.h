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

#ifndef __TZ_HOTPLUG_H__
#define __TZ_HOTPLUG_H__

#include <linux/compiler.h>

#ifdef CONFIG_TZDEV_HOTPLUG
int tz_hotplug_init(void);
void tz_hotplug_exit(void);
void tz_hotplug_update_nwd_cpu_mask(unsigned long new_mask);
void tz_hotplug_notify_swd_cpu_mask_update(void);

#else
static inline int tz_hotplug_init(void)
{
	return 0;
}
static inline void tz_hotplug_exit(void)
{
}
static inline void tz_hotplug_update_nwd_cpu_mask(unsigned long new_mask)
{
	(void) new_mask;
}
static inline void tz_hotplug_notify_swd_cpu_mask_update(void)
{
}
#endif /* CONFIG_TZDEV_HOTPLUG */

#endif /* __TZ_HOTPLUG_H__ */
