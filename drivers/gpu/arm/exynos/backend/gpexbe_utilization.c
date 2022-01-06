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

#include <mali_kbase.h>
#include <gpexbe_utilization.h>
#include <gpex_gts.h>

/* Mali pm metrics uses 256ns as a unit */
#define KBASE_PM_TIME_SHIFT 8

struct _util_info {
	struct kbase_device *kbdev;
	int pure_compute_time_rate;
	atomic_t time_compute_jobs;
	atomic_t time_vertex_jobs;
	atomic_t time_fragment_jobs;
	atomic_t cnt_compute_jobs;
	atomic_t cnt_fragment_jobs;
	atomic_t cnt_vertex_jobs;
	int cur_utilization;
};

static struct _util_info util_info;

static inline void atomic_add_shifted(u64 val, atomic_t *res)
{
	atomic_add(val >> KBASE_PM_TIME_SHIFT, res);
}

static inline void update_compute_job_load(u64 ns_elapsed)
{
	atomic_add_shifted(ns_elapsed, &util_info.time_compute_jobs);
}

static inline void update_fragment_job_load(u64 ns_elapsed)
{
	atomic_add_shifted(ns_elapsed, &util_info.time_fragment_jobs);
}

static inline void update_vertex_job_load(u64 ns_elapsed)
{
	atomic_add_shifted(ns_elapsed, &util_info.time_vertex_jobs);
}

static inline void increment_compute_job_cnt(void)
{
	atomic_inc(&util_info.cnt_compute_jobs);
}

static inline void increment_fragment_job_cnt(void)
{
	atomic_inc(&util_info.cnt_fragment_jobs);
}

static inline void increment_vertex_job_cnt(void)
{
	atomic_inc(&util_info.cnt_vertex_jobs);
}

static inline bool is_pure_compute_job(struct kbase_jd_atom *katom)
{
	return katom->core_req & BASE_JD_REQ_ONLY_COMPUTE;
}

static inline bool is_fragment_job(struct kbase_jd_atom *katom)
{
	return katom->core_req & BASE_JD_REQ_FS;
}

static inline bool is_compute_job(struct kbase_jd_atom *katom)
{
	/* Includes vertex shader, geometry shader and actual compute shader job */
	return katom->core_req & BASE_JD_REQ_CS;
}

/* Precondition: katom and end_timestamp are not NULL */
void gpexbe_utilization_update_job_load(struct kbase_jd_atom *katom, ktime_t *end_timestamp)
{
	u64 ns_spent = ktime_to_ns(ktime_sub(*end_timestamp, katom->start_timestamp));

	if (is_pure_compute_job(katom)) {
		update_compute_job_load(ns_spent);
		increment_compute_job_cnt();
	} else if (is_fragment_job(katom)) {
		update_fragment_job_load(ns_spent);
		increment_fragment_job_cnt();
	} else if (is_compute_job(katom)) {
		update_vertex_job_load(ns_spent);
		increment_vertex_job_cnt();
	}
}

int gpexbe_utilization_get_compute_job_time(void)
{
	return atomic_read(&util_info.time_compute_jobs);
}

int gpexbe_utilization_get_vertex_job_time(void)
{
	return atomic_read(&util_info.time_vertex_jobs);
}

int gpexbe_utilization_get_fragment_job_time(void)
{
	return atomic_read(&util_info.time_fragment_jobs);
}

int gpexbe_utilization_get_compute_job_cnt(void)
{
	return atomic_read(&util_info.cnt_compute_jobs);
}

int gpexbe_utilization_get_vertex_job_cnt(void)
{
	return atomic_read(&util_info.cnt_vertex_jobs);
}

int gpexbe_utilization_get_fragment_job_cnt(void)
{
	return atomic_read(&util_info.cnt_fragment_jobs);
}

int gpexbe_utilization_get_utilization(void)
{
	return util_info.cur_utilization;
}

int gpexbe_utilization_get_pure_compute_time_rate(void)
{
	return util_info.pure_compute_time_rate;
}

void gpexbe_utilization_calculate_compute_ratio(void)
{
	int compute_time = atomic_read(&util_info.time_compute_jobs);
	int vertex_time = atomic_read(&util_info.time_vertex_jobs);
	int fragment_time = atomic_read(&util_info.time_fragment_jobs);
	int total_time = compute_time + vertex_time + fragment_time;

	if (compute_time > 0 && total_time > 0)
		util_info.pure_compute_time_rate = (100 * compute_time) / total_time;
	else
		util_info.pure_compute_time_rate = 0;

	atomic_set(&util_info.time_compute_jobs, 0);
	atomic_set(&util_info.time_vertex_jobs, 0);
	atomic_set(&util_info.time_fragment_jobs, 0);
}

/* TODO: Refactor this function */
int gpexbe_utilization_calc_utilization(void)
{
	unsigned long flags;
	int utilisation = 0;
	struct kbase_device *kbdev = util_info.kbdev;

	ktime_t now = ktime_get();
	ktime_t diff;
	u32 ns_time;

	spin_lock_irqsave(&kbdev->pm.backend.metrics.lock, flags);
	diff = ktime_sub(now, kbdev->pm.backend.metrics.time_period_start);
	ns_time = (u32)(ktime_to_ns(diff) >> KBASE_PM_TIME_SHIFT);

	if (kbdev->pm.backend.metrics.gpu_active) {
		kbdev->pm.backend.metrics.values.time_busy += ns_time;
		/* TODO: busy_cl can be a static global here */
		kbdev->pm.backend.metrics.values.busy_cl[0] +=
			ns_time * kbdev->pm.backend.metrics.active_cl_ctx[0];
		kbdev->pm.backend.metrics.values.busy_cl[1] +=
			ns_time * kbdev->pm.backend.metrics.active_cl_ctx[1];

		kbdev->pm.backend.metrics.time_period_start = now;
	} else {
		kbdev->pm.backend.metrics.values.time_idle += ns_time;
		kbdev->pm.backend.metrics.time_period_start = now;
	}

	gpex_gts_update_jobslot_util(kbdev->pm.backend.metrics.gpu_active, ns_time);

	spin_unlock_irqrestore(&kbdev->pm.backend.metrics.lock, flags);

	if (kbdev->pm.backend.metrics.values.time_idle +
		    kbdev->pm.backend.metrics.values.time_busy ==
	    0) {
		/* No data - so we return NOP */
		utilisation = -1;
		goto out;
	}

	utilisation = (100 * kbdev->pm.backend.metrics.values.time_busy) /
		      (kbdev->pm.backend.metrics.values.time_idle +
		       kbdev->pm.backend.metrics.values.time_busy);

	gpex_gts_update_gpu_data();

out:
	spin_lock_irqsave(&kbdev->pm.backend.metrics.lock, flags);
	kbdev->pm.backend.metrics.values.time_idle = 0;
	kbdev->pm.backend.metrics.values.time_busy = 0;
	kbdev->pm.backend.metrics.values.busy_cl[0] = 0;
	kbdev->pm.backend.metrics.values.busy_cl[1] = 0;
	kbdev->pm.backend.metrics.values.busy_gl = 0;

	gpex_gts_clear();
	util_info.cur_utilization = utilisation;
	spin_unlock_irqrestore(&kbdev->pm.backend.metrics.lock, flags);

	return utilisation;
}

int gpexbe_utilization_init(struct device **dev)
{
	util_info.kbdev = container_of(dev, struct kbase_device, dev);

	atomic_set(&util_info.time_compute_jobs, 0);
	atomic_set(&util_info.time_vertex_jobs, 0);
	atomic_set(&util_info.time_fragment_jobs, 0);
	atomic_set(&util_info.cnt_compute_jobs, 0);
	atomic_set(&util_info.cnt_fragment_jobs, 0);
	atomic_set(&util_info.cnt_vertex_jobs, 0);

	util_info.pure_compute_time_rate = 0;

	return 0;
}

void gpexbe_utilization_term(void)
{
	util_info.kbdev = NULL;

	atomic_set(&util_info.time_compute_jobs, 0);
	atomic_set(&util_info.time_vertex_jobs, 0);
	atomic_set(&util_info.time_fragment_jobs, 0);
	atomic_set(&util_info.cnt_compute_jobs, 0);
	atomic_set(&util_info.cnt_fragment_jobs, 0);
	atomic_set(&util_info.cnt_vertex_jobs, 0);

	util_info.pure_compute_time_rate = 0;
}
