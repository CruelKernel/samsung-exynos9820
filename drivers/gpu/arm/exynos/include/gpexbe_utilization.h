/* SPDX-License-Identifier: GPL-2.0 */

/*
 * (C) COPYRIGHT 2021 Samsung Electronics Inc. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

#ifndef _GPEXBE_UTILIZATION_H_
#define _GPEXBE_UTILIZATION_H_

#include <linux/device.h>
#include <mali_kbase.h>

int gpexbe_utilization_init(struct device **dev);
void gpexbe_utilization_term(void);
int gpexbe_utilization_calc_utilization(void);
void gpexbe_utilization_calculate_compute_ratio(void);
void gpexbe_utilization_update_job_load(struct kbase_jd_atom *katom, ktime_t *end_timestamp);

int gpexbe_utilization_get_compute_job_time(void);
int gpexbe_utilization_get_vertex_job_time(void);
int gpexbe_utilization_get_fragment_job_time(void);
int gpexbe_utilization_get_compute_job_cnt(void);
int gpexbe_utilization_get_vertex_job_cnt(void);
int gpexbe_utilization_get_fragment_job_cnt(void);
int gpexbe_utilization_get_utilization(void);
int gpexbe_utilization_get_pure_compute_time_rate(void);

#endif /* _GPEXBE_UTILIZATION_H_ */
