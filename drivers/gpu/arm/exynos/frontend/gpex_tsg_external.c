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

#include <linux/notifier.h>
#include <linux/ktime.h>

#include <gpex_tsg.h>
#include <gpex_dvfs.h>
#include <gpex_utils.h>
#include <gpex_pm.h>
#include <gpex_ifpo.h>
#include <gpex_clock.h>
#include <gpexbe_pm.h>

#include "gpex_tsg_internal.h"

#define DVFS_TABLE_ROW_MAX 20

static struct _tsg_info *tsg_info;

unsigned long exynos_stats_get_job_state_cnt(void)
{
	return tsg_info->input_job_nr_acc;
}
EXPORT_SYMBOL(exynos_stats_get_job_state_cnt);

int exynos_stats_get_gpu_cur_idx(void)
{
	int i;
	int level = -1;
	int clock = 0;
	int idx_max_clk, idx_min_clk;

	if (gpexbe_pm_get_exynos_pm_domain()) {
		if (gpexbe_pm_get_status() == 1) {
			clock = gpex_clock_get_cur_clock();
		} else {
			GPU_LOG(MALI_EXYNOS_ERROR, "%s: can't get dvfs cur clock\n", __func__);
			clock = 0;
		}
	} else {
		if (gpex_pm_get_status(true) == 1) {
			if (gpex_ifpo_get_mode() && !gpex_ifpo_get_status()) {
				GPU_LOG(MALI_EXYNOS_ERROR, "%s: can't get dvfs cur clock\n",
					__func__);
				clock = 0;
			} else {
				clock = gpex_clock_get_cur_clock();
			}
		}
	}

	idx_max_clk = gpex_clock_get_table_idx(gpex_clock_get_max_clock());
	idx_min_clk = gpex_clock_get_table_idx(gpex_clock_get_min_clock());

	if ((idx_max_clk < 0) || (idx_min_clk < 0)) {
		GPU_LOG(MALI_EXYNOS_ERROR,
			"%s: mismatch clock table index. max_clock_level : %d, min_clock_level : %d\n",
			__func__, idx_max_clk, idx_min_clk);
		return -1;
	}

	if (clock == 0)
		return idx_min_clk - idx_max_clk;

	for (i = idx_max_clk; i <= idx_min_clk; i++) {
		if (gpex_clock_get_clock(i) == clock) {
			level = i;
			break;
		}
	}

	return (level - idx_max_clk);
}
EXPORT_SYMBOL(exynos_stats_get_gpu_cur_idx);

int exynos_stats_get_gpu_coeff(void)
{
	int coef = 6144;

	return coef;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_coeff);

uint32_t exynos_stats_get_gpu_table_size(void)
{
	return (gpex_clock_get_table_idx(gpex_clock_get_min_clock()) -
		gpex_clock_get_table_idx(gpex_clock_get_max_clock()) + 1);
}
EXPORT_SYMBOL(exynos_stats_get_gpu_table_size);

static uint32_t freqs[DVFS_TABLE_ROW_MAX];
uint32_t *exynos_stats_get_gpu_freq_table(void)
{
	int i;
	int idx_max_clk, idx_min_clk;

	idx_max_clk = gpex_clock_get_table_idx(gpex_clock_get_max_clock());
	idx_min_clk = gpex_clock_get_table_idx(gpex_clock_get_min_clock());

	if ((idx_max_clk < 0) || (idx_min_clk < 0)) {
		GPU_LOG(MALI_EXYNOS_ERROR,
			"%s: mismatch clock table index. idx_max_clk : %d, idx_min_clk : %d\n",
			__func__, idx_max_clk, idx_min_clk);
		return freqs;
	}

	for (i = idx_max_clk; i <= idx_min_clk; i++)
		freqs[i - idx_max_clk] = (uint32_t)gpex_clock_get_clock(i);

	return freqs;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_freq_table);

static uint32_t volts[DVFS_TABLE_ROW_MAX];
uint32_t *exynos_stats_get_gpu_volt_table(void)
{
	int i;
	int idx_max_clk, idx_min_clk;

	idx_max_clk = gpex_clock_get_table_idx(gpex_clock_get_max_clock());
	idx_min_clk = gpex_clock_get_table_idx(gpex_clock_get_min_clock());

	if ((idx_max_clk < 0) || (idx_min_clk < 0)) {
		GPU_LOG(MALI_EXYNOS_ERROR,
			"%s: mismatch clock table index. idx_max_clk : %d, idx_min_clk : %d\n",
			__func__, idx_max_clk, idx_min_clk);
		return volts;
	}

	for (i = idx_max_clk; i <= idx_min_clk; i++)
		volts[i - idx_max_clk] = (uint32_t)gpex_clock_get_voltage(gpex_clock_get_clock(i));

	return volts;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_volt_table);

static ktime_t time_in_state[DVFS_TABLE_ROW_MAX];
ktime_t *exynos_stats_get_gpu_time_in_state(void)
{
	int i;
	int idx_max_clk, idx_min_clk;

	idx_max_clk = gpex_clock_get_table_idx(gpex_clock_get_max_clock());
	idx_min_clk = gpex_clock_get_table_idx(gpex_clock_get_min_clock());

	if ((idx_max_clk < 0) || (idx_min_clk < 0)) {
		GPU_LOG(MALI_EXYNOS_ERROR,
			"%s: mismatch clock table index. idx_max_clk : %d, idx_min_clk : %d\n",
			__func__, idx_max_clk, idx_min_clk);
		return time_in_state;
	}

	for (i = idx_max_clk; i <= idx_min_clk; i++) {
		time_in_state[i - idx_max_clk] =
			ms_to_ktime((u64)(gpex_clock_get_time_busy(i) * 4) / 100);
	}

	return time_in_state;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_time_in_state);

int exynos_stats_get_gpu_max_lock(void)
{
	unsigned long flags;
	int locked_clock = -1;

	gpex_dvfs_spin_lock(&flags);
	locked_clock = gpex_clock_get_max_lock();
	if (locked_clock <= 0)
		locked_clock = gpex_clock_get_max_clock();
	gpex_dvfs_spin_unlock(&flags);

	return locked_clock;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_max_lock);

int exynos_stats_get_gpu_min_lock(void)
{
	unsigned long flags;
	int locked_clock = -1;

	gpex_dvfs_spin_lock(&flags);
	locked_clock = gpex_clock_get_min_lock();
	if (locked_clock <= 0)
		locked_clock = gpex_clock_get_min_clock();
	gpex_dvfs_spin_unlock(&flags);

	return locked_clock;
}
EXPORT_SYMBOL(exynos_stats_get_gpu_min_lock);

int exynos_stats_set_queued_threshold_0(uint32_t threshold)
{
	gpex_tsg_set_queued_threshold(0, threshold);
	return gpex_tsg_get_queued_threshold(0);
}
EXPORT_SYMBOL(exynos_stats_set_queued_threshold_0);

int exynos_stats_set_queued_threshold_1(uint32_t threshold)
{
	gpex_tsg_set_queued_threshold(1, threshold);
	return gpex_tsg_get_queued_threshold(1);
}
EXPORT_SYMBOL(exynos_stats_set_queued_threshold_1);

ktime_t *exynos_stats_get_gpu_queued_job_time(void)
{
	int i;
	for (i = 0; i < 2; i++) {
		gpex_tsg_set_queued_time(i, gpex_tsg_get_queued_time_tick(i));
	}
	return gpex_tsg_get_queued_time_array();
}
EXPORT_SYMBOL(exynos_stats_get_gpu_queued_job_time);

ktime_t exynos_stats_get_gpu_queued_last_updated(void)
{
	return gpex_tsg_get_queued_last_updated();
}
EXPORT_SYMBOL(exynos_stats_get_gpu_queued_last_updated);

void exynos_stats_set_gpu_polling_speed(int polling_speed)
{
	gpex_dvfs_set_polling_speed(polling_speed);
}
EXPORT_SYMBOL(exynos_stats_set_gpu_polling_speed);

int exynos_stats_get_gpu_polling_speed(void)
{
	return gpex_dvfs_get_polling_speed();
}
EXPORT_SYMBOL(exynos_stats_get_gpu_polling_speed);

void exynos_migov_set_mode(int mode)
{
	gpex_tsg_set_migov_mode(mode);
}
EXPORT_SYMBOL(exynos_migov_set_mode);

void exynos_migov_set_gpu_margin(int margin)
{
	gpex_tsg_set_freq_margin(margin);
}
EXPORT_SYMBOL(exynos_migov_set_gpu_margin);

int register_frag_utils_change_notifier(struct notifier_block *nb)
{
	int ret = 0;
	ret = atomic_notifier_chain_register(gpex_tsg_get_frag_utils_change_notifier_list(), nb);
	return ret;
}
EXPORT_SYMBOL(register_frag_utils_change_notifier);

int unregister_frag_utils_change_notifier(struct notifier_block *nb)
{
	int ret = 0;
	ret = atomic_notifier_chain_unregister(gpex_tsg_get_frag_utils_change_notifier_list(), nb);
	return ret;
}
EXPORT_SYMBOL(unregister_frag_utils_change_notifier);

/* TODO: this sysfs function use external fucntion. */
/* Actually, Using external function in internal module is not ideal with the Refactoring rules */
/* So, if we can modify outer modules such as 'migov, cooling, ...' in the future, */
/* fix it following the rules*/
static ssize_t show_feedback_governor_impl(char *buf)
{
	ssize_t ret = 0;
	int i;
	uint32_t *freqs;
	uint32_t *volts;
	ktime_t *time_in_state;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "feedback governer implementation\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- unsigned int exynos_stats_get_gpu_table_size(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "     +- int gpu_dvfs_get_step(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %u\n",
			exynos_stats_get_gpu_table_size());
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- unsigned int exynos_stats_get_gpu_cur_idx(void)\n");

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "     +- int gpu_dvfs_get_cur_level(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- clock=%u\n",
			gpex_clock_get_cur_clock());
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- level=%d\n",
			exynos_stats_get_gpu_cur_idx());
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- unsigned int exynos_stats_get_gpu_coeff(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"     +- int gpu_dvfs_get_coefficient_value(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %u\n",
			exynos_stats_get_gpu_coeff());
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- unsigned int *exynos_stats_get_gpu_freq_table(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"     +- unsigned int *gpu_dvfs_get_freqs(void)\n");
	freqs = exynos_stats_get_gpu_freq_table();
	for (i = 0; i < exynos_stats_get_gpu_table_size(); i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %u\n", freqs[i]);
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- unsigned int *exynos_stats_get_gpu_volt_table(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"     +- unsigned int *gpu_dvfs_get_volts(void)\n");
	volts = exynos_stats_get_gpu_volt_table();
	for (i = 0; i < exynos_stats_get_gpu_table_size(); i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %u\n", volts[i]);
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- ktime_t *exynos_stats_get_gpu_time_in_state(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"     +- ktime_t *gpu_dvfs_get_time_in_state(void)\n");
	time_in_state = exynos_stats_get_gpu_time_in_state();
	for (i = 0; i < exynos_stats_get_gpu_table_size(); i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %lld\n", time_in_state[i]);
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- ktime_t *exynos_stats_get_gpu_queued_job_time(void)\n");
	time_in_state = exynos_stats_get_gpu_queued_job_time();
	for (i = 0; i < 2; i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %lld\n", time_in_state[i]);
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret, " +- queued_threshold_check\n");
	for (i = 0; i < 2; i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %lld\n",
				gpex_tsg_get_queued_threshold(i));
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			" +- int exynos_stats_get_gpu_polling_speed(void)\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "         +- %d\n",
			exynos_stats_get_gpu_polling_speed());

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_feedback_governor_impl);

int gpex_tsg_external_init(struct _tsg_info *_tsg_info)
{
	tsg_info = _tsg_info;
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD_RO(feedback_governor_impl, show_feedback_governor_impl);
	return 0;
}

int gpex_tsg_external_term(void)
{
	tsg_info = (void *)0;

	return 0;
}
