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

#ifndef __TZ_IW_BOOT_LOG_H__
#define __TZ_IW_BOOT_LOG_H__

#ifdef CONFIG_TZ_BOOT_LOG
void tz_iw_boot_log_read(void);
#else
static inline void tz_iw_boot_log_read(void)
{
}
#endif

#endif /* __TZ_IW_BOOT_LOG_H__ */
