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

#ifndef _GPEX_TSG_INTERNAL_H_
#define _GPEX_TSG_INTERNAL_H_
#include <linux/ktime.h>

struct _tsg_info {
	struct kbase_device *kbdev;
	struct {
		int util_history[2][WINDOW];
		int freq_history[WINDOW];
		int average_util;
		int average_freq;
		int diff_util;
		int diff_freq;
		int weight_util[2];
		int weight_freq;
		int next_util;
		int next_freq;
		int pre_util;
		int pre_freq;
		bool en_signal;
	} prediction;

	struct {
		/* job queued time, index represents queued_time each threshold */
		uint32_t queued_threshold[2];
		ktime_t queued_time_tick[2];
		ktime_t queued_time[2];
		ktime_t last_updated;
		atomic_t nr[3]; /* Legacy: job_nr, in_nr, out_nr */
	} queue;

	uint64_t input_job_nr_acc;
	int input_job_nr;

	int freq_margin;
	int migov_mode;
	int weight_table_idx_0;
	int weight_table_idx_1;
	int migov_saved_polling_speed;
	bool is_pm_qos_tsg;
	int amigo_flags;

	int governor_type_init;
	int is_gov_set;

	raw_spinlock_t spinlock;
	struct atomic_notifier_head frag_utils_change_notifier_list;
};

int gpex_tsg_external_init(struct _tsg_info *_tsg_info);
int gpex_tsg_sysfs_init(struct _tsg_info *_tsg_info);

int gpex_tsg_external_term(void);
int gpex_tsg_sysfs_term(void);

#endif
