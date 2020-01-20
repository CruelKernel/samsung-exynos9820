/*
 * drivers/media/platform/exynos/mfc/mfc_hwlock.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_HWLOCK_H
#define __MFC_HWLOCK_H __FILE__

#include "mfc_common.h"

static inline void mfc_init_listable_wq_dev(struct mfc_dev *dev)
{
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return;
	}

	INIT_LIST_HEAD(&dev->hwlock_wq.list);
	init_waitqueue_head(&dev->hwlock_wq.wait_queue);
	mutex_init(&dev->hwlock_wq.wait_mutex);
	dev->hwlock_wq.ctx = NULL;
	dev->hwlock_wq.dev = dev;
}

static inline void mfc_init_listable_wq_ctx(struct mfc_ctx *curr_ctx)
{
	if (!curr_ctx) {
		mfc_err_dev("no mfc context to run\n");
		return;
	}

	INIT_LIST_HEAD(&curr_ctx->hwlock_wq.list);
	init_waitqueue_head(&curr_ctx->hwlock_wq.wait_queue);
	mutex_init(&curr_ctx->hwlock_wq.wait_mutex);
	curr_ctx->hwlock_wq.ctx = curr_ctx;
	curr_ctx->hwlock_wq.dev = NULL;
}

static inline void mfc_destroy_listable_wq_dev(struct mfc_dev *dev)
{
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return;
	}

	mutex_destroy(&dev->hwlock_wq.wait_mutex);
}

static inline void mfc_destroy_listable_wq_ctx(struct mfc_ctx *curr_ctx)
{
	if (!curr_ctx) {
		mfc_err_dev("no mfc context to run\n");
		return;
	}

	mutex_destroy(&curr_ctx->hwlock_wq.wait_mutex);
}

void mfc_init_hwlock(struct mfc_dev *dev);

int mfc_get_hwlock_dev(struct mfc_dev *dev);
int mfc_get_hwlock_ctx(struct mfc_ctx *curr_ctx);

void mfc_release_hwlock_dev(struct mfc_dev *dev);
void mfc_release_hwlock_ctx(struct mfc_ctx *curr_ctx);

void mfc_cache_flush(struct mfc_dev *dev, int is_drm);

void mfc_try_run(struct mfc_dev *dev);
void mfc_cleanup_work_bit_and_try_run(struct mfc_ctx *ctx);
int mfc_just_run(struct mfc_dev *dev, int new_ctx_index);

void mfc_hwlock_handler_irq(struct mfc_dev *dev, struct mfc_ctx *ctx,
		unsigned int reason, unsigned int err);

#endif /* __MFC_HWLOCK_H */
