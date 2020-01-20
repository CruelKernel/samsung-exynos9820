/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Header file for Exynos Repeater driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _EXYNOS_REPEATER_H_
#define _EXYNOS_REPEATER_H_

#include <linux/dma-buf.h>

#define MAX_SHARED_BUF_NUM	3

/**
 * struct shared_buffer_info
 *
 * @pixel_format	: pixel_format of bufs
 * @width		: width of bufs
 * @height		: height of bufs
 * @buffer_count	: valid buffer count of bufs
 * @bufs		: pointer of struct dma_buf for buffer sharing.
 */
struct shared_buffer_info {
	int pixel_format;
	int width;
	int height;
	int buffer_count;
	struct dma_buf *bufs[MAX_SHARED_BUF_NUM];
};

/**
 * hwfc_request_buffer - Get infomation of shared buffer
 * @info: information of shared buffer
 * owner: G2D is 0, MFC is 1
 *
 * G2D/MFC calls it to get shared buffer information.
 * Repeater increments reference count on the dma-buf.
 * G2D/MFC should decrement reference count on the dma-buf.
 * If repeater has valid buffer, it returns a zero.
 * If repeater don't have valid buffer, it returns a errno.
 *
 */
#ifdef CONFIG_VIDEO_EXYNOS_REPEATER
int hwfc_request_buffer(struct shared_buffer_info *info, int owner);
#else
static inline int hwfc_request_buffer(struct shared_buffer_info *info, int owner)
{
	return -1;
}
#endif

/**
 * hwfc_get_valid_buffer - Get shared buf idx for capture output
 * @buf_idx: shared buf idx for capture output
 *
 * G2D call it to get shared buffer index for capture output.
 * If repeater has valid buffer for capture output, it returns a zero.
 * If repeater don't have valid buffer for capture output, it returns a errno.
 *
 */
#ifdef CONFIG_VIDEO_EXYNOS_REPEATER
int hwfc_get_valid_buffer(int *buf_idx);
#else
static inline int hwfc_get_valid_buffer(int *buf_idx)
{
	return -1;
}
#endif

/**
 * hwfc_set_valid_buffer - Set shared buf idx & job id for capture output
 * @buf_idx	: shared buf idx for capture output
 * @capture_idx	: hwfc capture idx
 *
 * G2D calls it after G2D H/W starts capture.
 *
 */
#ifdef CONFIG_VIDEO_EXYNOS_REPEATER
int hwfc_set_valid_buffer(int buf_idx, int capture_idx);
#else
static inline int hwfc_set_valid_buffer(int buf_idx, int capture_idx)
{
	return -1;
}
#endif

/**
 * hwfc_encoding_done - Set ret about encoding
 * @encoding_ret : ret about encoding
 *
 * MFC calls it when encoding is done.
 *
 */
#ifdef CONFIG_VIDEO_EXYNOS_REPEATER
int hwfc_encoding_done(int encoding_ret);
#else
static inline int hwfc_encoding_done(int encoding_ret)
{
	return -1;
}
#endif

#endif /* _EXYNOS_REPEATER_H_ */
