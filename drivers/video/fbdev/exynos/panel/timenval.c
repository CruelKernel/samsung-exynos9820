// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "timenval.h"

int timenval_update_snapshot(struct timenval *tnv, int cur_value, struct timespec cur_ts)
{
	struct timespec delta_ts;
	static u64 update_cnt;
	s64 elapsed_msec;
	int last_value;

	if (tnv->last_ts.tv_sec == 0 && tnv->last_ts.tv_nsec == 0) {
		tnv->last_ts = cur_ts;
		tnv->last_value = cur_value;
		tnv->elapsed_msec = 0;
		tnv->sum = 0;
		tnv->avg = 0;
		return 0;
	}

	delta_ts = timespec_sub(cur_ts, tnv->last_ts);
	elapsed_msec = timespec_to_ns(&delta_ts) / NSEC_PER_MSEC;
	if (elapsed_msec == 0) {
		pr_debug("%s elapsed_msec 0 msec\n", __func__);
		elapsed_msec = 1;
	}

	last_value = tnv->last_value;
	tnv->sum += last_value * elapsed_msec;
	tnv->elapsed_msec += elapsed_msec;
	tnv->avg = tnv->sum / tnv->elapsed_msec;

	tnv->total_sum += last_value * elapsed_msec;
	tnv->total_elapsed_msec += elapsed_msec;
	tnv->total_avg = tnv->total_sum / tnv->total_elapsed_msec;
	tnv->last_ts = cur_ts;
	tnv->last_value = cur_value;

	pr_debug("%s new %d old %d avg %llu (sum %llu, %llu.%03llu sec)\n"
			"total avg %llu (sum %llu, %llu.%03llu sec) cnt %llu\n",
			__func__, cur_value, last_value, tnv->avg, tnv->sum,
			tnv->elapsed_msec / 1000, tnv->elapsed_msec % 1000,
			tnv->total_avg, tnv->total_sum,
			tnv->total_elapsed_msec / 1000, tnv->total_elapsed_msec % 1000,
			++update_cnt);
	return 0;
}

int timenval_update_average(struct timenval *tnv, int cur_value, struct timespec cur_ts)
{
	struct timespec delta_ts;
	static u64 update_cnt;
	s64 elapsed_msec;
	int last_value;

	if (tnv->last_ts.tv_sec == 0 && tnv->last_ts.tv_nsec == 0) {
		tnv->last_ts = cur_ts;
		tnv->last_value = cur_value;
		tnv->elapsed_msec = 0;
		tnv->sum = 0;
		tnv->avg = 0;
		return 0;
	}

	delta_ts = timespec_sub(cur_ts, tnv->last_ts);
	elapsed_msec = timespec_to_ns(&delta_ts) / NSEC_PER_MSEC;
	if (elapsed_msec == 0) {
		pr_debug("%s elapsed_msec 0 msec\n", __func__);
		elapsed_msec = 1;
	}

	last_value = cur_value;
	tnv->sum += last_value * elapsed_msec;
	tnv->elapsed_msec += elapsed_msec;
	tnv->avg = tnv->sum / tnv->elapsed_msec;

	tnv->total_sum += last_value * elapsed_msec;
	tnv->total_elapsed_msec += elapsed_msec;
	tnv->total_avg = tnv->total_sum / tnv->total_elapsed_msec;
	tnv->last_ts = cur_ts;
	tnv->last_value = cur_value;

	pr_debug("%s new %d old %d avg %llu (sum %llu, %llu.%03llu sec)\n"
			"total avg %llu (sum %llu, %llu.%03llu sec) cnt %llu\n",
			__func__, cur_value, last_value, tnv->avg, tnv->sum,
			tnv->elapsed_msec / 1000, tnv->elapsed_msec % 1000,
			tnv->total_avg, tnv->total_sum,
			tnv->total_elapsed_msec / 1000, tnv->total_elapsed_msec % 1000,
			++update_cnt);
	return 0;
}

int timenval_clear_average(struct timenval *tnv)
{
	tnv->elapsed_msec = 0;
	tnv->sum = 0;
	tnv->avg = 0;

	return 0;
}

int timenval_init(struct timenval *tnv)
{
	tnv->last_ts.tv_sec = 0;
	tnv->last_ts.tv_nsec = 0;
	tnv->last_value = 0;
	tnv->elapsed_msec = 0;
	tnv->sum = 0;
	tnv->avg = 0;

	return 0;
}

int timenval_start(struct timenval *tnv, int value, struct timespec cur_ts)
{
	tnv->last_ts = cur_ts;
	tnv->last_value = value;

	return 0;
}
