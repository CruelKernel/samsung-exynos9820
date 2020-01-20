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

#ifndef _REPEATER_DEV_H_
#define _REPEATER_DEV_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/types.h>
#include <linux/dma-buf.h>
#include <linux/timer.h>

#include <media/exynos_repeater.h>
#include <media/mfc_hwfc.h>

#include "repeater.h"
#include "repeater_buf.h"

#define REPEATER_MAX_CONTEXTS_NUM		1

enum repeater_context_status {
	REPEATER_CTX_INIT,
	REPEATER_CTX_MAP,
	REPEATER_CTX_START,
	REPEATER_CTX_PAUSE,
};

struct repeater_device {
	struct miscdevice misc_dev;
	struct device *dev;
	int ctx_num;
	struct repeater_context *ctx[REPEATER_MAX_CONTEXTS_NUM];
};

struct repeater_context {
	struct repeater_device *repeater_dev;
	struct repeater_info info;
	struct dma_buf *dmabufs[MAX_SHARED_BUF_NUM];

	struct shared_buffer shared_bufs;
	struct encoding_param enc_param;
	struct timer_list encoding_timer;
	uint64_t encoding_period_us;
	uint64_t last_encoding_time_us;
	uint64_t time_stamp_us;
	enum repeater_context_status ctx_status;

	struct delayed_work encoding_work;
	uint64_t encoding_start_timestamp;
	uint64_t video_frame_count;
	uint64_t paused_time;
	uint64_t pause_time;
	uint64_t resume_time;

	/* To dump data */
	wait_queue_head_t wait_queue_dump;
	int buf_idx_dump;
};

#define NODE_NAME		"repeater"
#define MODULE_NAME		"exynos-repeater"

#endif /* _REPEATER_DEV_H_ */
