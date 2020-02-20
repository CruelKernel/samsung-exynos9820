/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/dsms.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>

#include "dsms_rate_limit.h"
#include "dsms_debug.h"

#define ROUND_DURATION_MS ((u64)(1000L))
#define MAX_MESSAGES_PER_ROUND (50)

static int dsms_message_count;
static u64 dsms_round_start_ms;

static __always_inline u64 round_end_ms(u64 round_start_ms) {
	return round_start_ms + ROUND_DURATION_MS;
}

static __always_inline int is_new_round(u64 now_ms, u64 last_round_start_ms)
{
	return now_ms >= round_end_ms(last_round_start_ms);
}

static __always_inline u64 dsms_get_time_ms(void) {
	return ktime_get_ns() / NSEC_PER_MSEC;
}

void dsms_rate_limit_init(void)
{
	dsms_message_count = 0;
	dsms_round_start_ms = dsms_get_time_ms();
	dsms_log_write(LOG_DEBUG, "[rate limit] INIT dsms_round_start_ms=%lu dsms_message_count=%d",
		dsms_round_start_ms, dsms_message_count);
}

int dsms_check_message_rate_limit(void)
{
	int dropped_messages;
	u64 current_time_ms;

	current_time_ms = dsms_get_time_ms();
	if (current_time_ms < dsms_round_start_ms) {
		dsms_log_write(LOG_DEBUG, "[rate limit] RESET current_time_ms=%lu dsms_round_start_ms=%lu dsms_message_count=%d",
			current_time_ms, dsms_round_start_ms, dsms_message_count);
		dsms_round_start_ms = current_time_ms;
		dsms_message_count = 0;
	}

	if (is_new_round(current_time_ms, dsms_round_start_ms)) {
		dropped_messages = dsms_message_count - MAX_MESSAGES_PER_ROUND;
		if (dropped_messages > 0)
			dsms_log_write(LOG_ERROR, "[rate limit] %d of %d messages dropped", dropped_messages, dsms_message_count);
		dsms_round_start_ms = current_time_ms;
		dsms_message_count = 0;
		return DSMS_SUCCESS;
	}

	if (++dsms_message_count > MAX_MESSAGES_PER_ROUND)
		return DSMS_DENY;
	return DSMS_SUCCESS;
}
