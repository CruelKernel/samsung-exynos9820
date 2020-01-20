/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *             http://www.samsung.com
 *
 * Header file for Exynos REPEATER driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/jiffies.h>

#include "repeater_dbg.h"
#include "repeater_buf.h"

void init_shared_buffer(struct shared_buffer *bufs, int buffer_count)
{
	int i = 0;

	bufs->buffer_count = buffer_count;
	for (i = 0; i < bufs->buffer_count; i++) {
		bufs->captured_timestamp_us[i] = 0;
		bufs->buf_status[i] = SHARED_BUF_INIT;
	}
}

int get_capturing_buf_idx(struct shared_buffer *bufs, int *buf_idx)
{
	int i = 0;
	int ret = -ENOBUFS;
	uint64_t oldest_time_us = 0xFFFFFFFFFFFFFFFF;
	int captured_buf_index = -1;

	print_repeater_debug(RPT_SHR_BUF_INFO, "%s++\n", __func__);

	for (i = 0; i < bufs->buffer_count; i++) {
		print_repeater_debug(RPT_SHR_BUF_INFO,
			"bufs->buf_status[%d] %d, time %lld\n",
			i, bufs->buf_status[i], bufs->captured_timestamp_us[i]);
	}

	// find init buf
	for (i = 0; i < bufs->buffer_count; i++) {
		if (bufs->buf_status[i] == SHARED_BUF_INIT) {
			bufs->buf_status[i] = SHARED_BUF_CAPTURING;
			*buf_idx = i;
			ret = 0;
			break;
		}
	}

	// find oldest captured buf
	if (ret) {
		for (i = 0; i < bufs->buffer_count; i++) {
			if (bufs->buf_status[i] == SHARED_BUF_CAPTURED) {
				oldest_time_us = bufs->captured_timestamp_us[i];
				captured_buf_index = i;
				break;
			}
		}

		for (i = 0; i < bufs->buffer_count; i++) {
			if (bufs->buf_status[i] == SHARED_BUF_CAPTURED) {
				if (bufs->captured_timestamp_us[i] == 0) {
					oldest_time_us = bufs->captured_timestamp_us[i];
					captured_buf_index = i;
					break;
				}

				if (oldest_time_us > bufs->captured_timestamp_us[i]) {
					oldest_time_us = bufs->captured_timestamp_us[i];
					captured_buf_index = i;
				}
			}
		}
		if (captured_buf_index >= 0) {
			bufs->buf_status[captured_buf_index] = SHARED_BUF_CAPTURING;
			*buf_idx = captured_buf_index;
			ret = 0;
		}
	}

	print_repeater_debug(RPT_SHR_BUF_INFO,
		"ret %d, buf_idx %d, oldest_time_us %lld\n",
		ret, *buf_idx, oldest_time_us);

	print_repeater_debug(RPT_SHR_BUF_INFO, "%s--\n", __func__);

	return ret;
}

int set_captured_buf_idx(struct shared_buffer *bufs, int buf_idx, int cap_idx)
{
	int i = 0;
	int ret = -ENOBUFS;
	ktime_t cur_ktime;
	uint64_t cur_timestamp;

	print_repeater_debug(RPT_SHR_BUF_INFO, "%s++\n", __func__);

	if ((buf_idx >= 0 && buf_idx < MAX_SHARED_BUF_NUM) &&
			bufs->buf_status[buf_idx] == SHARED_BUF_CAPTURING) {
		if (buf_idx == cap_idx) {
			bufs->buf_status[buf_idx] = SHARED_BUF_CAPTURED;
		} else {
			bufs->buf_status[buf_idx] = SHARED_BUF_INIT;
			print_repeater_debug(RPT_ERROR, "blending error, buf_idx %d, cap_idx %d\n",
				buf_idx, cap_idx);
		}
		cur_ktime = ktime_get();
		cur_timestamp = ktime_to_us(cur_ktime);
		bufs->captured_timestamp_us[buf_idx] = cur_timestamp;
		ret = 0;

		print_repeater_debug(RPT_SHR_BUF_INFO,
			"buf_idx %d, captured_timestamp_us %lld, cur_timestamp %lld\n",
			buf_idx, bufs->captured_timestamp_us[buf_idx], cur_timestamp);
	}

	for (i = 0; i < bufs->buffer_count; i++) {
		print_repeater_debug(RPT_SHR_BUF_INFO,
			"bufs->buf_status[%d] %d, captured_timestamp_us %lld\n",
			i, bufs->buf_status[i], bufs->captured_timestamp_us[i]);
	}

	print_repeater_debug(RPT_SHR_BUF_INFO, "%s--\n", __func__);

	return ret;
}


int get_latest_captured_buf_idx(struct shared_buffer *bufs, int *buf_idx)
{
	int i = 0;
	int ret = -ENOBUFS;
	uint64_t latest_time_us = 0;
	int captured_buf_index = -1;

	print_repeater_debug(RPT_SHR_BUF_INFO, "%s++\n", __func__);

	for (i = 0; i < bufs->buffer_count; i++) {
		if (bufs->buf_status[i] == SHARED_BUF_CAPTURED &&
			latest_time_us < bufs->captured_timestamp_us[i]) {
			latest_time_us = bufs->captured_timestamp_us[i];
			captured_buf_index = i;
		}

		if (bufs->buf_status[i] == SHARED_BUF_ENCODE) {
			/* MFC encoding is not done */
			captured_buf_index = -1;
			break;
		}
	}

	if (captured_buf_index >= 0) {
		*buf_idx = captured_buf_index;
		ret = 0;
	}

	for (i = 0; i < bufs->buffer_count; i++) {
		print_repeater_debug(RPT_SHR_BUF_INFO,
			"bufs->buf_status[%d] %d, time %lld\n",
			i, bufs->buf_status[i], bufs->captured_timestamp_us[i]);
	}

	print_repeater_debug(RPT_SHR_BUF_INFO, "%s--\n", __func__);

	return ret;
}

int set_encoding_start(struct shared_buffer *bufs, int buf_idx)
{
	int i = 0;
	int ret = -ENOBUFS;

	print_repeater_debug(RPT_SHR_BUF_INFO, "%s++\n", __func__);

	print_repeater_debug(RPT_SHR_BUF_INFO, "bufs->buf_status[%d] %d",
		buf_idx, bufs->buf_status[buf_idx]);

	if (bufs->buf_status[buf_idx] == SHARED_BUF_CAPTURED) {
		bufs->buf_status[buf_idx] = SHARED_BUF_ENCODE;
		ret = 0;
	}

	for (i = 0; i < bufs->buffer_count; i++) {
		print_repeater_debug(RPT_SHR_BUF_INFO,
			"bufs->buf_status[%d] %d, time %lld\n",
			i, bufs->buf_status[i], bufs->captured_timestamp_us[i]);
	}

	print_repeater_debug(RPT_SHR_BUF_INFO, "%s--\n", __func__);

	return ret;
}

int set_encoding_done(struct shared_buffer *bufs)
{
	int i = 0;
	int ret = -ENOBUFS;

	print_repeater_debug(RPT_SHR_BUF_INFO, "%s++\n", __func__);

	for (i = 0; i < bufs->buffer_count; i++) {
		if (bufs->buf_status[i] == SHARED_BUF_ENCODE) {
			bufs->buf_status[i] = SHARED_BUF_CAPTURED;
			ret = 0;
		}
	}

	for (i = 0; i < bufs->buffer_count; i++) {
		print_repeater_debug(RPT_SHR_BUF_INFO,
			"bufs->buf_status[%d] %d, time %lld\n",
			i, bufs->buf_status[i], bufs->captured_timestamp_us[i]);
	}

	print_repeater_debug(RPT_SHR_BUF_INFO, "%s--\n", __func__);

	return ret;
}
