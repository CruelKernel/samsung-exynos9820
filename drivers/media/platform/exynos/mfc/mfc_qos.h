/*
 * drivers/media/platform/exynos/mfc/mfc_qos.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_QOS_H
#define __MFC_QOS_H __FILE__

#include "mfc_common.h"

#define MFC_MAX_FPS			(480000)
#define DEC_DEFAULT_FPS			(240000)
#define ENC_DEFAULT_FPS			(240000)
#define ENC_DEFAULT_CAM_CAPTURE_FPS	(60000)

#define MB_COUNT_PER_UHD_FRAME		32400
#define MAX_FPS_PER_UHD_FRAME		120
#define MIN_BW_PER_SEC			1

#define MFC_DRV_TIME			500

#ifdef CONFIG_MFC_USE_BUS_DEVFREQ
void mfc_perf_boost_enable(struct mfc_dev *dev);
void mfc_perf_boost_disable(struct mfc_dev *dev);
void mfc_qos_on(struct mfc_ctx *ctx);
void mfc_qos_off(struct mfc_ctx *ctx);
#else
#define mfc_perf_boost_enable(dev)	do {} while (0)
#define mfc_perf_boost_disable(dev)	do {} while (0)
#define mfc_qos_on(ctx)				do {} while (0)
#define mfc_qos_off(ctx)		do {} while (0)
#endif

void mfc_qos_idle_worker(struct work_struct *work);
void mfc_qos_update_framerate(struct mfc_ctx *ctx, u32 bytesused,
		int idle_trigger_only);
void mfc_qos_update_last_framerate(struct mfc_ctx *ctx, u64 timestamp);

static inline void mfc_qos_reset_framerate(struct mfc_ctx *ctx)
{
	if (ctx->type == MFCINST_DECODER)
		ctx->framerate = DEC_DEFAULT_FPS;
	else if (ctx->type == MFCINST_ENCODER)
		ctx->framerate = ENC_DEFAULT_FPS;
}

static inline void mfc_qos_reset_last_framerate(struct mfc_ctx *ctx)
{
	ctx->last_framerate = 0;
}

static inline void mfc_qos_set_framerate(struct mfc_ctx *ctx, int rate)
{
	ctx->framerate = rate;
}

static inline int mfc_qos_get_framerate(struct mfc_ctx *ctx)
{
	return ctx->framerate;
}

#endif /* __MFC_QOS_H */
