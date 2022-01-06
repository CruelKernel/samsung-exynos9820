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
#ifndef _MALI_EXYNOS_TSG_H_
#define _MALI_EXYNOS_TSG_H_

#define GPEX_TSG_PERIOD NSEC_PER_MSEC
/* Return whether katom will run on the GPU or not. Currently only soft jobs and
 * dependency-only atoms do not run on the GPU */

#define PRE_FRAME 3
#define WINDOW (4 * PRE_FRAME)

#include <linux/slab.h>
#include <linux/device.h>

enum queue_type {
	GPEX_TSG_QUEUE_JOB,
	GPEX_TSG_QUEUE_IN,
	GPEX_TSG_QUEUE_OUT,
	GPEX_TSG_INC,
	GPEX_TSG_DEC,
	GPEX_TSG_RST
};

enum gpex_tsg_atom_state {
	GPEX_TSG_ATOM_STATE_UNUSED,
	GPEX_TSG_ATOM_STATE_QUEUED,
	GPEX_TSG_ATOM_STATE_IN_JS,
	GPEX_TSG_ATOM_STATE_HW_COMPLETED,
	GPEX_TSG_ATOM_STATE_COMPLETED
};

void gpex_tsg_set_migov_mode(int mode);
void gpex_tsg_set_freq_margin(int margin);
void gpex_tsg_set_util_history(int idx, int order, int input);
void gpex_tsg_set_weight_util(int idx, int input);
void gpex_tsg_set_weight_freq(int input);
void gpex_tsg_set_en_signal(bool input);
void gpex_tsg_set_pmqos(bool pm_qos_tsg);
void gpex_tsg_set_weight_table_idx(int idx, int input);
void gpex_tsg_set_is_gov_set(int input);
void gpex_tsg_set_saved_polling_speed(int input);
void gpex_tsg_set_governor_type_init(int input);
void gpex_tsg_set_amigo_flags(int input);

void gpex_tsg_set_queued_threshold(int idx, uint32_t input);
void gpex_tsg_set_queued_time_tick(int idx, ktime_t input);
void gpex_tsg_set_queued_time(int idx, ktime_t input);
void gpex_tsg_set_queued_last_updated(ktime_t input);
void gpex_tsg_set_queue_nr(int type, int variation);

int gpex_tsg_get_migov_mode(void);
int gpex_tsg_get_freq_margin(void);
bool gpex_tsg_get_pmqos(void);
int gpex_tsg_get_util_history(int idx, int order);
int gpex_tsg_get_weight_util(int idx);
int gpex_tsg_get_weight_freq(void);
bool gpex_tsg_get_en_signal(void);
int gpex_tsg_get_weight_table_idx(int idx);
int gpex_tsg_get_is_gov_set(void);
int gpex_tsg_get_saved_polling_speed(void);
int gpex_tsg_get_governor_type_init(void);
int gpex_tsg_get_amigo_flags(void);

uint32_t gpex_tsg_get_queued_threshold(int idx);
ktime_t gpex_tsg_get_queued_time_tick(int idx);
ktime_t gpex_tsg_get_queued_time(int idx);
ktime_t *gpex_tsg_get_queued_time_array(void);
ktime_t gpex_tsg_get_queued_last_updated(void);
int gpex_tsg_get_queue_nr(int type);
struct atomic_notifier_head *gpex_tsg_get_frag_utils_change_notifier_list(void);
int gpex_tsg_notify_frag_utils_change(u32 js0_utils);

int gpex_tsg_spin_lock(void);
void gpex_tsg_spin_unlock(void);

int gpex_tsg_reset_count(int powered);
int gpex_tsg_set_count(u32 status, bool stop);

int gpex_tsg_init(struct device **dev);
int gpex_tsg_term(void);

/* functions needed by Exynos9830 Amigo */
void gpex_tsg_input_nr_acc_cnt(void);
void gpex_tsg_reset_acc_count(void);

#endif /* _MALI_EXYNOS_TSG_H_ */
