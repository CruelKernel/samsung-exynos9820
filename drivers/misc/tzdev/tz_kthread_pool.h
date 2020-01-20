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

#ifndef __TZ_KTHREAD_POOL_H__
#define __TZ_KTHREAD_POOL_H__

void tz_kthread_pool_fini(void);
void tz_kthread_pool_enter_swd(void);
void tz_kthread_pool_cmd_send(void);
void tz_kthread_pool_wake_up_all(void);

#endif /* __TZ_KTHREAD_POOL_H__ */
