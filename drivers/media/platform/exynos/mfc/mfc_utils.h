/*
 * drivers/media/platform/exynos/mfc/mfc_utils.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_UTILS_H
#define __MFC_UTILS_H __FILE__

#include "mfc_common.h"

/* bit operation */
#define mfc_clear_bits(reg, mask, shift)	(reg &= ~(mask << shift))
#define mfc_set_bits(reg, mask, shift, value)	(reg |= (value & mask) << shift)
#define mfc_clear_set_bits(reg, mask, shift, value)	\
	do {						\
		reg &= ~(mask << shift);		\
		reg |= (value & mask) << shift;		\
	} while (0)

static inline void mfc_clean_dev_int_flags(struct mfc_dev *dev)
{
	dev->int_condition = 0;
	dev->int_reason = 0;
	dev->int_err = 0;
}

static inline void mfc_clean_ctx_int_flags(struct mfc_ctx *ctx)
{
	ctx->int_condition = 0;
	ctx->int_reason = 0;
	ctx->int_err = 0;
}

static inline void mfc_change_state(struct mfc_ctx *ctx, enum mfc_inst_state state)
{
	struct mfc_dev *dev = ctx->dev;

	MFC_TRACE_CTX("** state : %d\n", state);
	ctx->state = state;
}

static inline enum mfc_node_type mfc_get_node_type(struct file *file)
{
	struct video_device *vdev = video_devdata(file);
	enum mfc_node_type node_type;

	if (!vdev) {
		mfc_err_dev("failed to get video_device\n");
		return MFCNODE_INVALID;
	}

	mfc_debug(2, "video_device index: %d\n", vdev->index);

	switch (vdev->index) {
	case 0:
		node_type = MFCNODE_DECODER;
		break;
	case 1:
		node_type = MFCNODE_ENCODER;
		break;
	case 2:
		node_type = MFCNODE_DECODER_DRM;
		break;
	case 3:
		node_type = MFCNODE_ENCODER_DRM;
		break;
	case 4:
		node_type = MFCNODE_ENCODER_OTF;
		break;
	case 5:
		node_type = MFCNODE_ENCODER_OTF_DRM;
		break;
	default:
		node_type = MFCNODE_INVALID;
		break;
	}

	return node_type;
}

static inline int mfc_is_decoder_node(enum mfc_node_type node)
{
	if (node == MFCNODE_DECODER || node == MFCNODE_DECODER_DRM)
		return 1;

	return 0;
}

static inline int mfc_is_drm_node(enum mfc_node_type node)
{
	if (node == MFCNODE_DECODER_DRM || node == MFCNODE_ENCODER_DRM ||
			node == MFCNODE_ENCODER_OTF_DRM)
		return 1;

	return 0;
}

static inline int mfc_is_encoder_otf_node(enum mfc_node_type node)
{
	if (node == MFCNODE_ENCODER_OTF || node == MFCNODE_ENCODER_OTF_DRM)
		return 1;

	return 0;
}

static inline void mfc_clear_vb_flag(struct mfc_buf *mfc_buf)
{
	mfc_buf->vb.reserved2 = 0;
}

static inline void mfc_set_vb_flag(struct mfc_buf *mfc_buf, enum mfc_vb_flag f)
{
	mfc_buf->vb.reserved2 |= (1 << f);
}

static inline int mfc_check_vb_flag(struct mfc_buf *mfc_buf, enum mfc_vb_flag f)
{
	if (mfc_buf->vb.reserved2 & (1 << f))
		return 1;

	return 0;
}

int mfc_check_vb_with_fmt(struct mfc_fmt *fmt, struct vb2_buffer *vb);

void mfc_raw_protect(struct mfc_ctx *ctx, struct mfc_buf *mfc_buf,
					int index);
void mfc_raw_unprotect(struct mfc_ctx *ctx, struct mfc_buf *mfc_buf,
					int index);
void mfc_stream_protect(struct mfc_ctx *ctx, struct mfc_buf *mfc_buf,
					int index);
void mfc_stream_unprotect(struct mfc_ctx *ctx, struct mfc_buf *mfc_buf,
					int index);

void mfc_dec_calc_dpb_size(struct mfc_ctx *ctx);
void mfc_enc_calc_src_size(struct mfc_ctx *ctx);

static inline void mfc_cleanup_assigned_fd(struct mfc_ctx *ctx)
{
	struct mfc_dec *dec;
	int i;

	dec = ctx->dec_priv;

	for (i = 0; i < MFC_MAX_DPBS; i++)
		dec->assigned_fd[i] = MFC_INFO_INIT_FD;
}

static inline void mfc_clear_assigned_dpb(struct mfc_ctx *ctx)
{
	struct mfc_dec *dec;
	int i;

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return;
	}

	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no mfc decoder to run\n");
		return;
	}

	for (i = 0; i < MFC_MAX_DPBS; i++)
		dec->assigned_dpb[i] = NULL;
}

static inline int mfc_dec_status_decoding(unsigned int dst_frame_status)
{
	if (dst_frame_status == MFC_REG_DEC_STATUS_DECODING_DISPLAY ||
	    dst_frame_status == MFC_REG_DEC_STATUS_DECODING_ONLY)
		return 1;
	return 0;
}

static inline int mfc_dec_status_display(unsigned int dst_frame_status)
{
	if (dst_frame_status == MFC_REG_DEC_STATUS_DISPLAY_ONLY ||
	    dst_frame_status == MFC_REG_DEC_STATUS_DECODING_DISPLAY)
		return 1;

	return 0;
}

void mfc_cleanup_assigned_dpb(struct mfc_ctx *ctx);
void mfc_unprotect_released_dpb(struct mfc_ctx *ctx, unsigned int released_flag);
void mfc_protect_dpb(struct mfc_ctx *ctx, struct mfc_buf *dst_mb);

/* Watchdog interval */
#define WATCHDOG_TICK_INTERVAL   1000
/* After how many executions watchdog should assume lock up */
#define WATCHDOG_TICK_CNT_TO_START_WATCHDOG        5

void mfc_watchdog_tick(unsigned long arg);
void mfc_watchdog_start_tick(struct mfc_dev *dev);
void mfc_watchdog_stop_tick(struct mfc_dev *dev);
void mfc_watchdog_reset_tick(struct mfc_dev *dev);

/* MFC idle checker interval */
#define MFCIDLE_TICK_INTERVAL	1500

void mfc_idle_checker(unsigned long arg);
static inline void mfc_idle_checker_start_tick(struct mfc_dev *dev)
{
	dev->mfc_idle_timer.expires = jiffies +
		msecs_to_jiffies(MFCIDLE_TICK_INTERVAL);
	add_timer(&dev->mfc_idle_timer);
}

static inline void mfc_change_idle_mode(struct mfc_dev *dev,
			enum mfc_idle_mode idle_mode)
{
	MFC_TRACE_DEV("** idle mode : %d\n", idle_mode);
	dev->idle_mode = idle_mode;

	if (dev->idle_mode == MFC_IDLE_MODE_NONE)
		mfc_idle_checker_start_tick(dev);
}

#endif /* __MFC_UTILS_H */
