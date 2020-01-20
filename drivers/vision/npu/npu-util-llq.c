/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>
#include "npu-util-llq.h"

#define NS_TO_MS	1000000
static inline s64 timespec_sub_to_ms(const struct timespec *lhs, const struct timespec *rhs)
{
	struct timespec ts;

	ts = timespec_sub(*lhs, *rhs);
	return timespec_to_ns(&ts) / NS_TO_MS;
}

size_t llq_print_timestat(const struct llq_time_stat *stat, char *out_buf, const size_t len)
{
	size_t remain = len;

#ifdef LLQ_TIME_TRACKING
	struct timespec ts_now;
	s64 d_init, d_succ_put, d_atmp_put, d_succ_get, d_atmp_get, d_empty;
	int ret;

	ts_now = ktime_to_timespec(ktime_get_boottime());

	d_init = timespec_sub_to_ms(&ts_now, &stat->ts_init);
	d_succ_put = timespec_sub_to_ms(&ts_now, &stat->ts_success_put);
	d_atmp_put = timespec_sub_to_ms(&ts_now, &stat->ts_attempt_put);
	d_succ_get = timespec_sub_to_ms(&ts_now, &stat->ts_success_get);
	d_atmp_get = timespec_sub_to_ms(&ts_now, &stat->ts_attempt_get);
	d_empty = timespec_sub_to_ms(&ts_now, &stat->ts_is_empty);

	ret = scnprintf(out_buf, remain,
			"Initialized at [%lld]ms ago\n"
			"GET - Last attempt [%lld]ms, Last success [%lld]ms ago\n"
			"PUT - Last attempt [%lld]ms, Last success [%lld]ms ago\n"
			"IS_AVAILABLE - Last invocation [%lld]ms ago\n",
			d_init, d_atmp_get, d_succ_get,
			d_atmp_put, d_succ_put, d_empty);
	if (ret)
		remain -= ret;

#endif	/* LLQ_TIME_TRACKING */
	return len - remain;
}

