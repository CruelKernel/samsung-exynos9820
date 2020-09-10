/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __TIMENVAL_H__
#define __TIMENVAL_H__

#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/wait.h>

struct timenval {
	struct timespec last_ts;
	int last_value;

	u64 sum;
	u64 avg;
	u64 elapsed_msec;

	u64 total_sum;
	u64 total_avg;
	u64 total_elapsed_msec;
};

int timenval_init(struct timenval *tnv);
int timenval_start(struct timenval *tnv, int value, struct timespec cur_ts);
int timenval_update_snapshot(struct timenval *tnv, int cur_value, struct timespec cur_ts);
int timenval_update_average(struct timenval *tnv, int cur_value, struct timespec cur_ts);
int timenval_clear_average(struct timenval *tnv);
#endif
