/*
 * PROCA task descriptors table
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
 * Hryhorii Tur, <hryhorii.tur@partner.samsung.com>
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

#ifndef _LINUX_PROCA_TABLE_H
#define _LINUX_PROCA_TABLE_H

#include <linux/proca.h>

void proca_table_init(struct proca_table *table);

void proca_table_add_task_descr(struct proca_table *table,
				struct proca_task_descr *descr);

struct proca_task_descr *proca_table_get_by_pid(struct proca_table *table,
						pid_t pid);

struct proca_task_descr *proca_table_remove_by_pid(struct proca_table *table,
						   pid_t pid);

void proca_table_remove_task_descr(struct proca_table *table,
				   struct proca_task_descr *descr);
#endif //_LINUX_PROCA_TABLE_H
