/*
 * PROCA task descriptor interface
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

#ifndef __LINUX_PROCA_TASK_DESCR_H

#include <linux/proca.h>

struct proca_task_descr *create_proca_task_descr(struct task_struct *task,
						 struct proca_identity *ident);

struct proca_task_descr *create_unsigned_proca_task_descr(
						struct task_struct *task);

void destroy_proca_task_descr(struct proca_task_descr *proca_task_descr);

#endif /* __LINUX_PROCA_H */
