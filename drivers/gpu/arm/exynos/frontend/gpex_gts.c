// SPDX-License-Identifier: GPL-2.0

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

#include <gpex_clock.h>
#include <gpex_tsg.h>
#include <gpex_gts.h>
#include <gpexbe_utilization.h>
#include <gpexbe_devicetree.h>

#include <mali_exynos_ioctl.h>

struct _gts_info {
	int heavy_compute_mode;
	atomic_t heavy_compute_util_count;

	int highspeed_clock;

	u32 busy_js;
	u32 idle_js;
	bool jobslot_active;
};

static struct _gts_info gts_info;

#define LCNT 4

struct gpu_shared_d {
	u64 out_data[LCNT];
	u64 input;
	u64 input2;
	u64 freq;
	u64 flimit;
};

struct gpu_data {
	void (*get_out_data)(u64 *cnt);
	u64 last_out_data;
	u64 freq;
	u64 input;
	u64 input2;
	int update_idx;
	ktime_t last_update_t;
	struct gpu_shared_d sd;
	int odidx;
};

struct gpu_data gpud;

void gpu_register_out_data(void (*fn)(u64 *cnt))
{
	gpud.get_out_data = fn;
}
EXPORT_SYMBOL_GPL(gpu_register_out_data);

static inline int get_outd_idx(int idx)
{
	if (++idx >= 4)
		return 0;
	return idx;
}

int gpex_gts_get_ioctl_gts_info(struct mali_exynos_ioctl_gts_info *info)
{
	int i = 0;
#if IS_ENABLED(CONFIG_SMP)
	info->util_avg = READ_ONCE(current->se.avg.util_avg);
#else
	info->util_avg = 0;
#endif
	for (i = 0; i < LCNT; i++)
		info->out_data[i] = gpud.sd.out_data[i];

	info->input = gpud.sd.input / 10;
	info->input2 = gpud.sd.input2;
	info->freq = gpud.sd.freq;
	info->flimit = gts_info.highspeed_clock;
	info->hcm_mode = gts_info.heavy_compute_mode;

	return 0;
}

static int calculate_jobslot_util(void)
{
	return 100 * gts_info.busy_js / (gts_info.idle_js + gts_info.busy_js);
}

static void calculate_heavy_compute_count(void)
{
	/* HCM STUFF */
	int heavy_compute_count = 0;
	int compute_time;
	int vertex_time;
	int fragment_time;
	int total_time;

	compute_time = gpexbe_utilization_get_compute_job_time();
	vertex_time = gpexbe_utilization_get_vertex_job_time();
	fragment_time = gpexbe_utilization_get_fragment_job_time();
	total_time = compute_time + vertex_time + fragment_time;

	heavy_compute_count = atomic_read(&gts_info.heavy_compute_util_count);

	if (total_time > 0 && fragment_time > 0) {
		int compute_cnt = gpexbe_utilization_get_compute_job_cnt();
		int vertex_cnt = gpexbe_utilization_get_vertex_job_cnt();
		int fragment_cnt = gpexbe_utilization_get_fragment_job_cnt();
		int vf_ratio = 0;

		if (fragment_cnt > 0) {
			if ((compute_cnt + vertex_cnt) > 0)
				vf_ratio = (compute_cnt + vertex_cnt) * 100 / fragment_cnt;
			else
				vf_ratio = 0;
		} else {
			if ((compute_cnt + vertex_cnt) > 0)
				vf_ratio = 10;
			else
				vf_ratio = 0;
		}

		if (vf_ratio > 94 && ((vertex_time + compute_time) * 100 / fragment_time) > 60) {
			if (heavy_compute_count < 10) {
				atomic_inc(&gts_info.heavy_compute_util_count);
			}
		} else {
			if (heavy_compute_count > 0) {
				atomic_dec(&gts_info.heavy_compute_util_count);
			}
		}
	}

	/* HCM Stuff end */
}

bool gpex_gts_update_gpu_data(void)
{
	ktime_t now = ktime_get();
	u64 out_data;

	int jsutil;
	int input2;

	if (unlikely(!gpud.get_out_data)) {
		gpud.sd.input = 0;
		gpud.sd.input2 = 0;
		return false;
	}

	jsutil = calculate_jobslot_util();
	calculate_heavy_compute_count();
	input2 = atomic_read(&gts_info.heavy_compute_util_count);

#if IS_ENABLED(CONFIG_MALI_NOTIFY_UTILISATION)
	if (gts_info.heavy_compute_mode & HCM_MODE_B &&
	    gpex_clock_get_cur_clock() > gpex_clock_get_clock(1))
		gpex_tsg_notify_frag_utils_change(jsutil >> 1);
	else
		gpex_tsg_notify_frag_utils_change(jsutil); // Futher update utilizations
#endif

	gpud.freq += gpex_clock_get_cur_clock();
	gpud.input += jsutil;
	gpud.update_idx++;

#define UPDATE_PERIOD 250
	if (UPDATE_PERIOD > ktime_to_ms(ktime_sub(now, gpud.last_update_t)) || !gpud.update_idx)
		return false;

	gpud.sd.freq = gpud.freq / gpud.update_idx;
	gpud.sd.input = gpud.input / gpud.update_idx;
	gpud.sd.input2 = input2;

	gpud.freq = 0;
	gpud.input = 0;
	gpud.update_idx = 0;

	gpud.get_out_data(&out_data);
	gpud.odidx = get_outd_idx(gpud.odidx);
	gpud.sd.out_data[gpud.odidx] = out_data - gpud.last_out_data;
	gpud.last_out_data = out_data;
	gpud.last_update_t = now;

	return true;
}

void gpex_gts_update_jobslot_util(bool gpu_active, u32 ns_time)
{
	if (gpu_active && gts_info.jobslot_active)
		gts_info.busy_js += ns_time;
	else
		gts_info.idle_js += ns_time;
}

void gpex_gts_set_jobslot_status(bool is_active)
{
	gts_info.jobslot_active = is_active;
}

void gpex_gts_clear(void)
{
	gts_info.busy_js = 0;
	gts_info.idle_js = 0;
}

void gpex_gts_set_hcm_mode(int hcm_mode_val)
{
	gts_info.heavy_compute_mode = hcm_mode_val;
}

int gpex_gts_get_hcm_mode(void)
{
	return gts_info.heavy_compute_mode;
}

int gpex_gts_init(struct device **dev)
{
	atomic_set(&gts_info.heavy_compute_util_count, 0);

	gts_info.highspeed_clock = gpexbe_devicetree_get_int(interactive_info.highspeed_clock);
	gts_info.heavy_compute_mode = 0;
	gts_info.busy_js = 0;
	gts_info.idle_js = 0;

	return 0;
}

void gpex_gts_term(void)
{
	gts_info.highspeed_clock = 0;
	gts_info.heavy_compute_mode = 0;
	gts_info.busy_js = 0;
	gts_info.idle_js = 0;
}
