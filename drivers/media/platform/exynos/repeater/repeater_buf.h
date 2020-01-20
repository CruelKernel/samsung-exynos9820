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

#ifndef _REPEATER_BUF_H_
#define _REPEATER_BUF_H_

#include <linux/dma-buf.h>

#include <media/exynos_repeater.h>

enum shared_buffer_status {
	SHARED_BUF_INIT,
	SHARED_BUF_CAPTURING,
	SHARED_BUF_CAPTURED,
	SHARED_BUF_ENCODE
};

/**
 * struct shared_buffer
 *
 * @buf_status		: status of bufs
 * @dmabuf		: dmabuf of bufs
 * @buf_spinlock	: spinlock for bufs
 */
struct shared_buffer {
	enum shared_buffer_status buf_status[MAX_SHARED_BUF_NUM];
	uint64_t captured_timestamp_us[MAX_SHARED_BUF_NUM];
	int buffer_count;
};

void init_shared_buffer(struct shared_buffer *bufs, int buffer_count);

int get_capturing_buf_idx(struct shared_buffer *bufs, int *buf_idx);

int set_captured_buf_idx(struct shared_buffer *bufs, int buf_idx, int cap_idx);

int get_latest_captured_buf_idx(struct shared_buffer *bufs, int *buf_idx);

int set_encoding_start(struct shared_buffer *bufs, int buf_idx);

int set_encoding_done(struct shared_buffer *bufs);

#endif /* _REPEATER_BUF_H_ */
