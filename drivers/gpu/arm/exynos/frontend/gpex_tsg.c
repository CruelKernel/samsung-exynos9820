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

#include <gpex_tsg.h>

#include <gpex_utils.h>
#include <gpex_dvfs.h>
#include <gpex_pm.h>
#include <gpex_clock.h>
#include <gpexbe_devicetree.h>
#include <gpexbe_utilization.h>

#include "gpex_tsg_internal.h"
#include "gpex_dvfs_internal.h"
#include "gpu_dvfs_governor.h"

static struct _tsg_info tsg;

void gpex_tsg_input_nr_acc_cnt(void)
{
	tsg.input_job_nr_acc += tsg.input_job_nr;
}

void gpex_tsg_reset_acc_count(void)
{
	tsg.input_job_nr_acc = 0;
	gpex_tsg_set_queued_time_tick(0, 0);
	gpex_tsg_set_queued_time_tick(1, 0);
}

/* SETTER */
void gpex_tsg_set_migov_mode(int mode)
{
	tsg.migov_mode = mode;
	return;
}

void gpex_tsg_set_freq_margin(int margin)
{
	tsg.freq_margin = margin;
	return;
}

void gpex_tsg_set_util_history(int idx, int order, int input)
{
	tsg.prediction.util_history[idx][order] = input;
	return;
}

void gpex_tsg_set_weight_util(int idx, int input)
{
	tsg.prediction.weight_util[idx] = input;
	return;
}

void gpex_tsg_set_weight_freq(int input)
{
	tsg.prediction.weight_freq = input;
	return;
}

void gpex_tsg_set_en_signal(bool input)
{
	tsg.prediction.en_signal = input;
	return;
}

void gpex_tsg_set_pmqos(bool input)
{
	tsg.is_pm_qos_tsg = input;
	return;
}

void gpex_tsg_set_weight_table_idx(int idx, int input)
{
	if (idx == 0)
		tsg.weight_table_idx_0 = input;
	else
		tsg.weight_table_idx_1 = input;
	return;
}

void gpex_tsg_set_is_gov_set(int input)
{
	tsg.is_gov_set = input;
	return;
}

void gpex_tsg_set_saved_polling_speed(int input)
{
	tsg.migov_saved_polling_speed = input;
	return;
}

void gpex_tsg_set_governor_type_init(int input)
{
	tsg.governor_type_init = input;
	return;
}

void gpex_tsg_set_amigo_flags(int input)
{
	tsg.amigo_flags = input;
	return;
}

void gpex_tsg_set_queued_threshold(int idx, uint32_t input)
{
	tsg.queue.queued_threshold[idx] = input;
	return;
}

void gpex_tsg_set_queued_time_tick(int idx, ktime_t input)
{
	tsg.queue.queued_time_tick[idx] = input;
	return;
}

void gpex_tsg_set_queued_time(int idx, ktime_t input)
{
	tsg.queue.queued_time[idx] = input;
}

void gpex_tsg_set_queued_last_updated(ktime_t input)
{
	tsg.queue.last_updated = input;
}

void gpex_tsg_set_queue_nr(int type, int variation)
{
	/* GPEX_TSG_QUEUE_JOB/IN/OUT: 0, 1, 2*/
	/* GPEX_TSG_QUEUE_INC/DEC/RST: 3, 4, 5*/

	if (variation == GPEX_TSG_RST)
		atomic_set(&tsg.queue.nr[type], 0);
	else
		(variation == GPEX_TSG_INC) ? atomic_inc(&tsg.queue.nr[type]) :
						    atomic_dec(&tsg.queue.nr[type]);
}

/* GETTER */
int gpex_tsg_get_migov_mode(void)
{
	return tsg.migov_mode;
}

int gpex_tsg_get_freq_margin(void)
{
	return tsg.freq_margin;
}

int gpex_tsg_get_util_history(int idx, int order)
{
	return tsg.prediction.util_history[idx][order];
}

int gpex_tsg_get_weight_util(int idx)
{
	return tsg.prediction.weight_util[idx];
}

int gpex_tsg_get_weight_freq(void)
{
	return tsg.prediction.weight_freq;
}

bool gpex_tsg_get_en_signal(void)
{
	return tsg.prediction.en_signal;
}

bool gpex_tsg_get_pmqos(void)
{
	return tsg.is_pm_qos_tsg;
}

int gpex_tsg_get_weight_table_idx(int idx)
{
	return (idx == 0) ? tsg.weight_table_idx_0 : tsg.weight_table_idx_1;
}

int gpex_tsg_get_is_gov_set(void)
{
	return tsg.is_gov_set;
}

int gpex_tsg_get_saved_polling_speed(void)
{
	return tsg.migov_saved_polling_speed;
}

int gpex_tsg_get_governor_type_init(void)
{
	return tsg.governor_type_init;
}

int gpex_tsg_get_amigo_flags(void)
{
	return tsg.amigo_flags;
}

uint32_t gpex_tsg_get_queued_threshold(int idx)
{
	return tsg.queue.queued_threshold[idx];
}

ktime_t gpex_tsg_get_queued_time_tick(int idx)
{
	return tsg.queue.queued_time_tick[idx];
}

ktime_t gpex_tsg_get_queued_time(int idx)
{
	return tsg.queue.queued_time[idx];
}

ktime_t *gpex_tsg_get_queued_time_array(void)
{
	return tsg.queue.queued_time;
}

ktime_t gpex_tsg_get_queued_last_updated(void)
{
	return tsg.queue.last_updated;
}

int gpex_tsg_get_queue_nr(int type)
{
	return atomic_read(&tsg.queue.nr[type]);
}

struct atomic_notifier_head *gpex_tsg_get_frag_utils_change_notifier_list(void)
{
	return &(tsg.frag_utils_change_notifier_list);
}

int gpex_tsg_notify_frag_utils_change(u32 js0_utils)
{
	int ret = 0;
	ret = atomic_notifier_call_chain(gpex_tsg_get_frag_utils_change_notifier_list(), js0_utils,
					 NULL);
	return ret;
}

int gpex_tsg_spin_lock(void)
{
	return raw_spin_trylock(&tsg.spinlock);
}

void gpex_tsg_spin_unlock(void)
{
	raw_spin_unlock(&tsg.spinlock);
}

/* Function of Kbase */

static inline bool both_q_active(int in_nr, int out_nr)
{
	return in_nr > 0 && out_nr > 0;
}

static inline bool hw_q_active(int out_nr)
{
	return out_nr > 0;
}

static void accum_queued_time(ktime_t current_time, bool accum0, bool accum1)
{
	int time_diff = 0;
	time_diff = current_time - gpex_tsg_get_queued_last_updated();

	if (accum0 == true)
		gpex_tsg_set_queued_time_tick(0, gpex_tsg_get_queued_time_tick(0) + time_diff);
	if (accum1 == true)
		gpex_tsg_set_queued_time_tick(1, gpex_tsg_get_queued_time_tick(1) + time_diff);

	gpex_tsg_set_queued_last_updated(current_time);
}

int gpex_tsg_reset_count(int powered)
{
	if (powered)
		return 0;

	gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_JOB, GPEX_TSG_RST);
	gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_IN, GPEX_TSG_RST);
	gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_OUT, GPEX_TSG_RST);
	gpex_tsg_set_queued_last_updated(0);

	tsg.input_job_nr = 0;

	return !powered;
}

int gpex_tsg_set_count(u32 status, bool stop)
{
	int prev_out_nr, prev_in_nr;
	int cur_out_nr, cur_in_nr;
	bool need_update = false;
	ktime_t current_time = 0;

	if (gpex_tsg_get_queued_last_updated() == 0)
		gpex_tsg_set_queued_last_updated(ktime_get());

	prev_out_nr = gpex_tsg_get_queue_nr(GPEX_TSG_QUEUE_OUT);
	prev_in_nr = gpex_tsg_get_queue_nr(GPEX_TSG_QUEUE_IN);

	if (stop) {
		gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_IN, GPEX_TSG_INC);
		gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_OUT, GPEX_TSG_DEC);
		tsg.input_job_nr++;
	} else {
		switch (status) {
		case GPEX_TSG_ATOM_STATE_QUEUED:
			gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_IN, GPEX_TSG_INC);
			tsg.input_job_nr++;
			break;
		case GPEX_TSG_ATOM_STATE_IN_JS:
			gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_IN, GPEX_TSG_DEC);
			gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_OUT, GPEX_TSG_INC);
			tsg.input_job_nr--;
			if (tsg.input_job_nr < 0)
				tsg.input_job_nr = 0;
			break;
		case GPEX_TSG_ATOM_STATE_HW_COMPLETED:
			gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_OUT, GPEX_TSG_DEC);
			break;
		default:
			break;
		}
	}

	cur_in_nr = gpex_tsg_get_queue_nr(GPEX_TSG_QUEUE_IN);
	cur_out_nr = gpex_tsg_get_queue_nr(GPEX_TSG_QUEUE_OUT);

	if ((!both_q_active(prev_in_nr, prev_out_nr) && both_q_active(cur_in_nr, cur_out_nr)) ||
	    (!hw_q_active(prev_out_nr) && hw_q_active(cur_out_nr)))
		need_update = true;
	else if ((both_q_active(prev_in_nr, prev_out_nr) &&
		  !both_q_active(cur_in_nr, cur_out_nr)) ||
		 (hw_q_active(prev_out_nr) && !hw_q_active(cur_out_nr)))
		need_update = true;
	else if (prev_out_nr > cur_out_nr) {
		current_time = ktime_get();
		need_update =
			current_time - (gpex_tsg_get_queued_last_updated() > 2) * GPEX_TSG_PERIOD;
	}

	if (need_update) {
		current_time = (current_time == 0) ? ktime_get() : current_time;
		if (gpex_tsg_spin_lock()) {
			accum_queued_time(current_time, both_q_active(prev_in_nr, prev_out_nr),
					  hw_q_active(prev_out_nr));
			gpex_tsg_spin_unlock();
		}
	}

	if ((cur_in_nr + cur_out_nr) < 0)
		gpex_tsg_reset_count(0);

	return 0;
}

static void gpex_tsg_context_init(void)
{
	gpex_tsg_set_weight_table_idx(0, gpexbe_devicetree_get_int(gpu_weight_table_idx_0));
	gpex_tsg_set_weight_table_idx(1, gpexbe_devicetree_get_int(gpu_weight_table_idx_1));
	gpex_tsg_set_governor_type_init(gpex_dvfs_get_governor_type());
	gpex_tsg_set_queued_threshold(0, 0);
	gpex_tsg_set_queued_threshold(1, 0);
	gpex_tsg_set_queued_time_tick(0, 0);
	gpex_tsg_set_queued_time_tick(1, 0);
	gpex_tsg_set_queued_time(0, 0);
	gpex_tsg_set_queued_time(1, 0);
	gpex_tsg_set_queued_last_updated(0);
	gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_JOB, GPEX_TSG_RST);
	gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_IN, GPEX_TSG_RST);
	gpex_tsg_set_queue_nr(GPEX_TSG_QUEUE_OUT, GPEX_TSG_RST);

	tsg.input_job_nr = 0;
	tsg.input_job_nr_acc = 0;
}

int gpex_tsg_init(struct device **dev)
{
	raw_spin_lock_init(&tsg.spinlock);
	tsg.kbdev = container_of(dev, struct kbase_device, dev);

	gpex_tsg_context_init();
	gpex_tsg_external_init(&tsg);
	gpex_tsg_sysfs_init(&tsg);

	return 0;
}

int gpex_tsg_term(void)
{
	gpex_tsg_sysfs_term();
	gpex_tsg_external_term();
	tsg.kbdev = NULL;

	return 0;
}
