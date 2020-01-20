/*
 * drivers/media/platform/exynos/mfc/mfc_enc_ops.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "mfc_common.h"

#include "mfc_reg_api.h"

static int __mfc_enc_ctrl_read_cst(struct mfc_ctx *ctx,
		struct mfc_buf_ctrl *buf_ctrl)
{
	int ret;
	struct mfc_enc *enc = ctx->enc_priv;

	switch (buf_ctrl->id) {
	case V4L2_CID_MPEG_MFC51_VIDEO_FRAME_STATUS:
		ret = !enc->in_slice;
		break;
	default:
		mfc_err_ctx("not support custom per-buffer control\n");
		ret = -EINVAL;
		break;
	}

	return ret;
}

static struct mfc_ctrl_cfg mfc_ctrl_list[] = {
	{	/* set frame tag */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_PICTURE_TAG,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{	/* get frame tag */
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RET_PICTURE_TAG,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{	/* encoded y physical addr */
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_MFC51_VIDEO_LUMA_ADDR,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_ENCODED_SOURCE_FIRST_ADDR,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{	/* encoded c physical addr */
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_MFC51_VIDEO_CHROMA_ADDR,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_ENCODED_SOURCE_SECOND_ADDR,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{	/* I, not coded frame insertion */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_MFC51_VIDEO_FORCE_FRAME_TYPE,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_FRAME_INSERTION,
		.mask = 0x3,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{	/* I period change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_MFC51_VIDEO_I_PERIOD_CH,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_GOP_CONFIG,
		.mask = 0xFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 0,
	},
	{	/* frame rate change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_RATE_CH,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_FRAME_RATE,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 1,
	},
	{	/* bit rate change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_MFC51_VIDEO_BIT_RATE_CH,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_BIT_RATE,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 2,
	},
	{	/* frame status (in slice or not) */
		.type = MFC_CTRL_TYPE_GET_DST,
		.id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_STATUS,
		.is_volatile = 0,
		.mode = MFC_CTRL_MODE_CST,
		.addr = 0,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
		.read_cst = __mfc_enc_ctrl_read_cst,
		.write_cst = NULL,
	},
	{	/* H.264 I frame QP Max change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_H264_MAX_QP,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND,
		.mask = 0xFF,
		.shft = 8,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* H.264 I frame QP Min change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_H264_MIN_QP,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND,
		.mask = 0xFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* H.263 I frame QP Max change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_H263_MAX_QP,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND,
		.mask = 0xFF,
		.shft = 8,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* H.263 I frame QP Min change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_H263_MIN_QP,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND,
		.mask = 0xFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* MPEG4 I frame QP Max change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND,
		.mask = 0xFF,
		.shft = 8,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* MPEG4 I frame QP Min change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND,
		.mask = 0xFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* VP8 I frame QP Max change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_VP8_MAX_QP,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND,
		.mask = 0xFF,
		.shft = 8,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* VP8 I frame QP Min change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_VP8_MIN_QP,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND,
		.mask = 0xFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* VP9 I frame QP Max change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_VP9_MAX_QP,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND,
		.mask = 0xFF,
		.shft = 8,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* VP9 I frame QP Min change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_VP9_MIN_QP,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND,
		.mask = 0xFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* HEVC I frame QP Max change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND,
		.mask = 0xFF,
		.shft = 8,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* HEVC I frame QP Min change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND,
		.mask = 0xFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* H.264 P frame QP Max change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_H264_MAX_QP_P,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND_PB,
		.mask = 0xFF,
		.shft = 8,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* H.264 P frame QP Min change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_H264_MIN_QP_P,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND_PB,
		.mask = 0xFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* H.263 P frame QP Max change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_H263_MAX_QP_P,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND_PB,
		.mask = 0xFF,
		.shft = 8,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* H.263 P frame QP Min change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_H263_MIN_QP_P,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND_PB,
		.mask = 0xFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* MPEG4 P frame QP Max change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP_P,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND_PB,
		.mask = 0xFF,
		.shft = 8,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* MPEG4 P frame QP Min change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP_P,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND_PB,
		.mask = 0xFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* VP8 P frame QP Max change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_VP8_MAX_QP_P,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND_PB,
		.mask = 0xFF,
		.shft = 8,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* VP8 P frame QP Min change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_VP8_MIN_QP_P,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND_PB,
		.mask = 0xFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* VP9 P frame QP Max change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_VP9_MAX_QP_P,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND_PB,
		.mask = 0xFF,
		.shft = 8,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* VP9 P frame QP Min change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_VP9_MIN_QP_P,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND_PB,
		.mask = 0xFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* HEVC P frame QP Max change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP_P,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND_PB,
		.mask = 0xFF,
		.shft = 8,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* HEVC P frame QP Min change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP_P,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND_PB,
		.mask = 0xFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* H.264 B frame QP Max change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_H264_MAX_QP_B,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND_PB,
		.mask = 0xFF,
		.shft = 24,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* H.264 B frame QP Min change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_H264_MIN_QP_B,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND_PB,
		.mask = 0xFF,
		.shft = 16,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* MPEG4 B frame QP Max change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP_B,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND_PB,
		.mask = 0xFF,
		.shft = 24,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* MPEG4 B frame QP Min change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP_B,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND_PB,
		.mask = 0xFF,
		.shft = 16,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* HEVC B frame QP Max change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP_B,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND_PB,
		.mask = 0xFF,
		.shft = 24,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* HEVC B frame QP Min change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP_B,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_QP_BOUND_PB,
		.mask = 0xFF,
		.shft = 16,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 4,
	},
	{	/* H.264 Dynamic Temporal Layer & bitrate change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_CH,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_HIERARCHICAL_BIT_RATE_LAYER0,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 10,
	},
	{	/* HEVC Dynamic Temporal Layer & bitrate change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_CH,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_HIERARCHICAL_BIT_RATE_LAYER0,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 10,
	},
	{	/* VP8 Dynamic Temporal Layer change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_VP8_HIERARCHICAL_CODING_LAYER_CH,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_HIERARCHICAL_BIT_RATE_LAYER0,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 10,
	},
	{	/* VP9 Dynamic Temporal Layer change */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_CH,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_HIERARCHICAL_BIT_RATE_LAYER0,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 10,
	},
	{	/* set level */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_H264_LEVEL,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_PICTURE_PROFILE,
		.mask = 0x000000FF,
		.shft = 8,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 5,
	},
	{	/* set profile */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_H264_PROFILE,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_PICTURE_PROFILE,
		.mask = 0x0000000F,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 5,
	},
	{	/* set store LTR */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_MFC_H264_MARK_LTR,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_H264_NAL_CONTROL,
		.mask = 0x00000003,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{	/* set use LTR */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_MFC_H264_USE_LTR,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_H264_NAL_CONTROL,
		.mask = 0x00000003,
		.shft = 2,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{	/* set base layer priority */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_MFC_H264_BASE_PRIORITY,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_H264_HD_SVC_EXTENSION_0,
		.mask = 0x0000003F,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 12,
	},
	{	/* set QP per each frame */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_MFC_CONFIG_QP,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_FIXED_PICTURE_QP,
		.mask = 0x000000FF,
		.shft = 24,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{	/* Region-Of-Interest control */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_ROI_CONTROL,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_ROI_CTRL,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_addr = 0,
		.flag_shft = 0,
	},
	{	/* set YSUM for weighted prediction */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_YSUM,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_WEIGHT_FOR_WEIGHTED_PREDICTION,
		.mask = 0xFFFFFFFF,
		.shft = 0,
		.flag_mode = MFC_CTRL_MODE_NONE,
		.flag_mode = 0,
		.flag_shft = 0,
	},
	{	/* set base layer priority */
		.type = MFC_CTRL_TYPE_SET,
		.id = V4L2_CID_MPEG_VIDEO_RATIO_OF_INTRA,
		.is_volatile = 1,
		.mode = MFC_CTRL_MODE_SFR,
		.addr = MFC_REG_E_RC_MODE,
		.mask = 0x000000FF,
		.shft = 8,
		.flag_mode = MFC_CTRL_MODE_SFR,
		.flag_addr = MFC_REG_E_PARAM_CHANGE,
		.flag_shft = 13,
	}
};

#define NUM_CTRL_CFGS ARRAY_SIZE(mfc_ctrl_list)

static int mfc_enc_cleanup_ctx_ctrls(struct mfc_ctx *ctx)
{
	struct mfc_ctx_ctrl *ctx_ctrl;

	while (!list_empty(&ctx->ctrls)) {
		ctx_ctrl = list_entry((&ctx->ctrls)->next,
				      struct mfc_ctx_ctrl, list);
		list_del(&ctx_ctrl->list);
		kfree(ctx_ctrl);
	}

	INIT_LIST_HEAD(&ctx->ctrls);

	return 0;
}

static int mfc_enc_get_buf_update_val(struct mfc_ctx *ctx,
			struct list_head *head, unsigned int id, int value)
{
	struct mfc_buf_ctrl *buf_ctrl;

	list_for_each_entry(buf_ctrl, head, list) {
		if (buf_ctrl->id == id) {
			buf_ctrl->val = value;
			mfc_debug(6, "[CTRLS] Update buffer control id: 0x%08x, val: %d\n",
					buf_ctrl->id, buf_ctrl->val);
			break;
		}
	}

	return 0;
}

static int mfc_enc_init_ctx_ctrls(struct mfc_ctx *ctx)
{
	unsigned long i;
	struct mfc_ctx_ctrl *ctx_ctrl;

	INIT_LIST_HEAD(&ctx->ctrls);

	for (i = 0; i < NUM_CTRL_CFGS; i++) {
		ctx_ctrl = kzalloc(sizeof(struct mfc_ctx_ctrl), GFP_KERNEL);
		if (ctx_ctrl == NULL) {
			mfc_err_dev("Failed to allocate context control "
					"id: 0x%08x, type: %d\n",
					mfc_ctrl_list[i].id,
					mfc_ctrl_list[i].type);

			mfc_enc_cleanup_ctx_ctrls(ctx);

			return -ENOMEM;
		}

		ctx_ctrl->type = mfc_ctrl_list[i].type;
		ctx_ctrl->id = mfc_ctrl_list[i].id;
		ctx_ctrl->has_new = 0;
		ctx_ctrl->val = 0;

		list_add_tail(&ctx_ctrl->list, &ctx->ctrls);
	}

	return 0;
}

static void mfc_enc_reset_buf_ctrls(struct list_head *head)
{
	struct mfc_buf_ctrl *buf_ctrl;

	list_for_each_entry(buf_ctrl, head, list) {
		buf_ctrl->has_new = 0;
		buf_ctrl->val = 0;
		buf_ctrl->old_val = 0;
		buf_ctrl->updated = 0;
	}
}

static void __mfc_enc_remove_buf_ctrls(struct list_head *head)
{
	struct mfc_buf_ctrl *buf_ctrl;

	while (!list_empty(head)) {
		buf_ctrl = list_entry(head->next,
				struct mfc_buf_ctrl, list);
		list_del(&buf_ctrl->list);
		kfree(buf_ctrl);
	}

	INIT_LIST_HEAD(head);
}

static int mfc_enc_init_buf_ctrls(struct mfc_ctx *ctx,
	enum mfc_ctrl_type type, unsigned int index)
{
	unsigned long i;
	struct mfc_buf_ctrl *buf_ctrl;
	struct list_head *head;

	if (index >= MFC_MAX_BUFFERS) {
		mfc_err_dev("Per-buffer control index is out of range\n");
		return -EINVAL;
	}

	if (type & MFC_CTRL_TYPE_SRC) {
		if (test_bit(index, &ctx->src_ctrls_avail)) {
			mfc_enc_reset_buf_ctrls(&ctx->src_ctrls[index]);

			return 0;
		}

		head = &ctx->src_ctrls[index];
	} else if (type & MFC_CTRL_TYPE_DST) {
		if (test_bit(index, &ctx->dst_ctrls_avail)) {
			mfc_enc_reset_buf_ctrls(&ctx->dst_ctrls[index]);

			return 0;
		}

		head = &ctx->dst_ctrls[index];
	} else {
		mfc_err_dev("Control type mismatch. type : %d\n", type);
		return -EINVAL;
	}

	INIT_LIST_HEAD(head);

	for (i = 0; i < NUM_CTRL_CFGS; i++) {
		if (!(type & mfc_ctrl_list[i].type))
			continue;

		buf_ctrl = kzalloc(sizeof(struct mfc_buf_ctrl), GFP_KERNEL);
		if (buf_ctrl == NULL) {
			mfc_err_dev("Failed to allocate buffer control "
					"id: 0x%08x, type: %d\n",
					mfc_ctrl_list[i].id,
					mfc_ctrl_list[i].type);

			__mfc_enc_remove_buf_ctrls(head);

			return -ENOMEM;
		}

		buf_ctrl->type = mfc_ctrl_list[i].type;
		buf_ctrl->id = mfc_ctrl_list[i].id;
		buf_ctrl->is_volatile = mfc_ctrl_list[i].is_volatile;
		buf_ctrl->mode = mfc_ctrl_list[i].mode;
		buf_ctrl->addr = mfc_ctrl_list[i].addr;
		buf_ctrl->mask = mfc_ctrl_list[i].mask;
		buf_ctrl->shft = mfc_ctrl_list[i].shft;
		buf_ctrl->flag_mode = mfc_ctrl_list[i].flag_mode;
		buf_ctrl->flag_addr = mfc_ctrl_list[i].flag_addr;
		buf_ctrl->flag_shft = mfc_ctrl_list[i].flag_shft;
		if (buf_ctrl->mode == MFC_CTRL_MODE_CST) {
			buf_ctrl->read_cst = mfc_ctrl_list[i].read_cst;
			buf_ctrl->write_cst = mfc_ctrl_list[i].write_cst;
		}

		list_add_tail(&buf_ctrl->list, head);
	}

	mfc_enc_reset_buf_ctrls(head);

	if (type & MFC_CTRL_TYPE_SRC)
		set_bit(index, &ctx->src_ctrls_avail);
	else
		set_bit(index, &ctx->dst_ctrls_avail);

	return 0;
}

static int mfc_enc_cleanup_buf_ctrls(struct mfc_ctx *ctx,
	enum mfc_ctrl_type type, unsigned int index)
{
	struct list_head *head;

	if (index >= MFC_MAX_BUFFERS) {
		mfc_err_dev("Per-buffer control index is out of range\n");
		return -EINVAL;
	}

	if (type & MFC_CTRL_TYPE_SRC) {
		if (!(test_and_clear_bit(index, &ctx->src_ctrls_avail))) {
			return 0;
		}

		head = &ctx->src_ctrls[index];
	} else if (type & MFC_CTRL_TYPE_DST) {
		if (!(test_and_clear_bit(index, &ctx->dst_ctrls_avail))) {
			return 0;
		}

		head = &ctx->dst_ctrls[index];
	} else {
		mfc_err_dev("Control type mismatch. type : %d\n", type);
		return -EINVAL;
	}

	__mfc_enc_remove_buf_ctrls(head);

	return 0;
}

static int mfc_enc_to_buf_ctrls(struct mfc_ctx *ctx, struct list_head *head)
{
	struct mfc_ctx_ctrl *ctx_ctrl;
	struct mfc_buf_ctrl *buf_ctrl;
	struct mfc_enc *enc = ctx->enc_priv;
	int index = 0;
	unsigned int reg = 0;

	list_for_each_entry(ctx_ctrl, &ctx->ctrls, list) {
		if (!(ctx_ctrl->type & MFC_CTRL_TYPE_SET) || !ctx_ctrl->has_new)
			continue;

		list_for_each_entry(buf_ctrl, head, list) {
			if (!(buf_ctrl->type & MFC_CTRL_TYPE_SET))
				continue;

			if (buf_ctrl->id == ctx_ctrl->id) {
				buf_ctrl->has_new = 1;
				buf_ctrl->val = ctx_ctrl->val;
				if (buf_ctrl->is_volatile)
					buf_ctrl->updated = 0;

				ctx_ctrl->has_new = 0;
				if (buf_ctrl->id == V4L2_CID_MPEG_VIDEO_ROI_CONTROL) {
					index = enc->roi_index;
					if (enc->roi_info[index].enable) {
						enc->roi_index =
							(index + 1) % MFC_MAX_EXTRA_BUF;
						reg |= enc->roi_info[index].enable;
						reg &= ~(0xFF << 8);
						reg |= (enc->roi_info[index].lower_qp << 8);
						reg &= ~(0xFFFF << 16);
						reg |= (enc->roi_info[index].upper_qp << 16);
						mfc_debug(3, "[ROI] buffer[%d] en %d, "
								"QP lower %d upper %d reg %#x\n",
								index, enc->roi_info[index].enable,
								enc->roi_info[index].lower_qp,
								enc->roi_info[index].upper_qp,
								reg);
					} else {
						mfc_debug(3, "[ROI] buffer[%d] is not enabled\n", index);
					}
					buf_ctrl->val = reg;
					buf_ctrl->old_val2 = index;
				}
				break;
			}
		}
	}

	return 0;
}

static int mfc_enc_to_ctx_ctrls(struct mfc_ctx *ctx, struct list_head *head)
{
	struct mfc_ctx_ctrl *ctx_ctrl;
	struct mfc_buf_ctrl *buf_ctrl;

	list_for_each_entry(buf_ctrl, head, list) {
		if (!(buf_ctrl->type & MFC_CTRL_TYPE_GET) || !buf_ctrl->has_new)
			continue;

		list_for_each_entry(ctx_ctrl, &ctx->ctrls, list) {
			if (!(ctx_ctrl->type & MFC_CTRL_TYPE_GET))
				continue;

			if (ctx_ctrl->id == buf_ctrl->id) {
				if (ctx_ctrl->has_new)
					mfc_debug(8,
					"Overwrite context control "
					"value id: 0x%08x, val: %d\n",
						ctx_ctrl->id, ctx_ctrl->val);

				ctx_ctrl->has_new = 1;
				ctx_ctrl->val = buf_ctrl->val;

				buf_ctrl->has_new = 0;
			}
		}
	}

	return 0;
}

static void __mfc_enc_set_buf_ctrls_temporal_svc(struct mfc_ctx *ctx,
			struct mfc_buf_ctrl *buf_ctrl)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	unsigned int value = 0, value2 = 0;
	struct temporal_layer_info temporal_LC;
	unsigned int i;
	struct mfc_enc_params *p = &enc->params;

	if (buf_ctrl->id
		== V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_CH ||
		buf_ctrl->id
		== V4L2_CID_MPEG_VIDEO_VP8_HIERARCHICAL_CODING_LAYER_CH ||
		buf_ctrl->id
		== V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_CH ||
		buf_ctrl->id
		== V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_CH) {
		memcpy(&temporal_LC,
			enc->sh_handle_svc.vaddr, sizeof(struct temporal_layer_info));

		if(((temporal_LC.temporal_layer_count & 0x7) < 1) ||
			((temporal_LC.temporal_layer_count > 3) && IS_VP8_ENC(ctx)) ||
			((temporal_LC.temporal_layer_count > 3) && IS_VP9_ENC(ctx))) {
			/* clear NUM_T_LAYER_CHANGE */
			value = MFC_READL(buf_ctrl->flag_addr);
			value &= ~(1 << 10);
			MFC_WRITEL(value, buf_ctrl->flag_addr);
			mfc_err_ctx("[HIERARCHICAL] layer count is invalid : %d\n",
					temporal_LC.temporal_layer_count);
			return;
		}

		if (IS_H264_ENC(ctx))
			p->codec.h264.num_hier_layer = temporal_LC.temporal_layer_count & 0x7;

		/* enable RC_BIT_RATE_CHANGE */
		value = MFC_READL(buf_ctrl->flag_addr);
		if (temporal_LC.temporal_layer_bitrate[0] > 0 || p->hier_bitrate_ctrl)
			/* set RC_BIT_RATE_CHANGE */
			value |= (1 << 2);
		else
			/* clear RC_BIT_RATE_CHANGE */
			value &= ~(1 << 2);
		MFC_WRITEL(value, buf_ctrl->flag_addr);

		mfc_debug(3, "[HIERARCHICAL] layer count %d, E_PARAM_CHANGE %#x\n",
				temporal_LC.temporal_layer_count & 0x7, value);

		value = MFC_READL(MFC_REG_E_NUM_T_LAYER);
		buf_ctrl->old_val2 = value;
		value &= ~(0x7);
		value |= (temporal_LC.temporal_layer_count & 0x7);
		value &= ~(0x1 << 8);
		value |= (p->hier_bitrate_ctrl & 0x1) << 8;
		MFC_WRITEL(value, MFC_REG_E_NUM_T_LAYER);
		mfc_debug(3, "[HIERARCHICAL] E_NUM_T_LAYER %#x\n", value);
		for (i = 0; i < (temporal_LC.temporal_layer_count & 0x7); i++) {
			mfc_debug(3, "[HIERARCHICAL] layer bitrate[%d] %d (FW ctrl: %d)\n",
					i, temporal_LC.temporal_layer_bitrate[i], p->hier_bitrate_ctrl);
			MFC_WRITEL(temporal_LC.temporal_layer_bitrate[i],
					buf_ctrl->addr + i * 4);
		}
		/* priority change */
		if (IS_H264_ENC(ctx)) {
			value = 0;
			value2 = 0;
			for (i = 0; i < (p->codec.h264.num_hier_layer & 0x07); i++) {
				if (i <= 4)
					value |= ((p->codec.h264.base_priority & 0x3F) + i)
						<< (6 * i);
				else
					value2 |= ((p->codec.h264.base_priority & 0x3F) + i)
						<< (6 * (i - 5));
			}
			MFC_WRITEL(value, MFC_REG_E_H264_HD_SVC_EXTENSION_0);
			MFC_WRITEL(value2, MFC_REG_E_H264_HD_SVC_EXTENSION_1);
			mfc_debug(3, "[HIERARCHICAL] EXTENSION0 %#x, EXTENSION1 %#x\n",
					value, value2);

			value = MFC_READL(buf_ctrl->flag_addr);
			value |= (1 << 12);
			MFC_WRITEL(value, buf_ctrl->flag_addr);
			mfc_debug(3, "[HIERARCHICAL] E_PARAM_CHANGE %#x\n", value);
		}

	}

	/* temproral layer priority */
	if (buf_ctrl->id == V4L2_CID_MPEG_MFC_H264_BASE_PRIORITY) {
		value = MFC_READL(MFC_REG_E_H264_HD_SVC_EXTENSION_0);
		buf_ctrl->old_val |= value & 0x3FFFFFC0;
		value &= ~(0x3FFFFFC0);
		value2 = MFC_READL(MFC_REG_E_H264_HD_SVC_EXTENSION_1);
		buf_ctrl->old_val2 = value2 & 0x0FFF;
		value2 &= ~(0x0FFF);
		for (i = 0; i < (p->codec.h264.num_hier_layer & 0x07); i++) {
			if (i <= 4)
				value |= ((buf_ctrl->val & 0x3F) + i) << (6 * i);
			else
				value2 |= ((buf_ctrl->val & 0x3F) + i) << (6 * (i - 5));
		}
		MFC_WRITEL(value, MFC_REG_E_H264_HD_SVC_EXTENSION_0);
		MFC_WRITEL(value2, MFC_REG_E_H264_HD_SVC_EXTENSION_1);
		p->codec.h264.base_priority = buf_ctrl->val;
		mfc_debug(3, "[HIERARCHICAL] EXTENSION0 %#x, EXTENSION1 %#x\n",
				value, value2);
	}
}

static void __mfc_enc_set_buf_ctrls_exception(struct mfc_ctx *ctx,
			struct mfc_buf_ctrl *buf_ctrl)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_enc_params *p = &enc->params;
	unsigned int value = 0;

	if (buf_ctrl->id == V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG)
		enc->stored_tag = buf_ctrl->val;

	/* temporal layer setting */
	__mfc_enc_set_buf_ctrls_temporal_svc(ctx, buf_ctrl);

	if (buf_ctrl->id == V4L2_CID_MPEG_MFC_H264_MARK_LTR) {
		value = MFC_READL(MFC_REG_E_H264_NAL_CONTROL);
		buf_ctrl->old_val2 = (value >> 8) & 0x7;
		value &= ~(0x7 << 8);
		value |= (buf_ctrl->val & 0x7) << 8;
		MFC_WRITEL(value, MFC_REG_E_H264_NAL_CONTROL);
	}
	if (buf_ctrl->id == V4L2_CID_MPEG_MFC_H264_USE_LTR) {
		value = MFC_READL(MFC_REG_E_H264_NAL_CONTROL);
		buf_ctrl->old_val2 = (value >> 11) & 0xF;
		value &= ~(0xF << 11);
		value |= (buf_ctrl->val & 0xF) << 11;
		MFC_WRITEL(value, MFC_REG_E_H264_NAL_CONTROL);
	}

	if (buf_ctrl->id == V4L2_CID_MPEG_MFC51_VIDEO_I_PERIOD_CH) {
		value = MFC_READL(MFC_REG_E_GOP_CONFIG2);
		buf_ctrl->old_val |= (value << 16) & 0x3FFF0000;
		value &= ~(0x3FFF);
		value |= (buf_ctrl->val >> 16) & 0x3FFF;
		MFC_WRITEL(value, MFC_REG_E_GOP_CONFIG2);
	}

	/* PROFILE & LEVEL have to be set up together */
	if (buf_ctrl->id == V4L2_CID_MPEG_VIDEO_H264_LEVEL) {
		value = MFC_READL(MFC_REG_E_PICTURE_PROFILE);
		buf_ctrl->old_val |= (value & 0x000F) << 8;
		value &= ~(0x000F);
		value |= p->codec.h264.profile & 0x000F;
		MFC_WRITEL(value, MFC_REG_E_PICTURE_PROFILE);
		p->codec.h264.level = buf_ctrl->val;
	}

	if (buf_ctrl->id == V4L2_CID_MPEG_VIDEO_H264_PROFILE) {
		value = MFC_READL(MFC_REG_E_PICTURE_PROFILE);
		buf_ctrl->old_val |= value & 0xFF00;
		value &= ~(0x00FF << 8);
		value |= (p->codec.h264.level << 8) & 0xFF00;
		MFC_WRITEL(value, MFC_REG_E_PICTURE_PROFILE);
		p->codec.h264.profile = buf_ctrl->val;
	}

	/* per buffer QP setting change */
	if (buf_ctrl->id == V4L2_CID_MPEG_MFC_CONFIG_QP)
		p->config_qp = buf_ctrl->val;

	/* set the ROI buffer DVA */
	if (buf_ctrl->id == V4L2_CID_MPEG_VIDEO_ROI_CONTROL) {
		MFC_WRITEL(enc->roi_buf[buf_ctrl->old_val2].daddr,
				MFC_REG_E_ROI_BUFFER_ADDR);
		mfc_debug(3, "[ROI] buffer[%d] addr %#llx, QP val: %#x\n",
				buf_ctrl->old_val2,
				enc->roi_buf[buf_ctrl->old_val2].daddr,
				buf_ctrl->val);
	}
}

static int mfc_enc_set_buf_ctrls_val(struct mfc_ctx *ctx, struct list_head *head)
{
	struct mfc_buf_ctrl *buf_ctrl;
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	unsigned int value = 0;
	struct mfc_enc_params *p = &enc->params;

	list_for_each_entry(buf_ctrl, head, list) {
		if (!(buf_ctrl->type & MFC_CTRL_TYPE_SET) || !buf_ctrl->has_new)
			continue;

		/* read old vlaue */
		value = MFC_READL(buf_ctrl->addr);

		/* save old value for recovery */
		if (buf_ctrl->is_volatile)
			buf_ctrl->old_val = (value >> buf_ctrl->shft) & buf_ctrl->mask;

		/* write new value */
		value &= ~(buf_ctrl->mask << buf_ctrl->shft);
		value |= ((buf_ctrl->val & buf_ctrl->mask) << buf_ctrl->shft);
		MFC_WRITEL(value, buf_ctrl->addr);

		/* set change flag bit */
		if (buf_ctrl->flag_mode == MFC_CTRL_MODE_SFR) {
			value = MFC_READL(buf_ctrl->flag_addr);
			value |= (1 << buf_ctrl->flag_shft);
			MFC_WRITEL(value, buf_ctrl->flag_addr);
		}

		buf_ctrl->has_new = 0;
		buf_ctrl->updated = 1;

		__mfc_enc_set_buf_ctrls_exception(ctx, buf_ctrl);

		mfc_debug(6, "[CTRLS] Set buffer control id: 0x%08x, val: %d\n",
				buf_ctrl->id, buf_ctrl->val);
	}

	if (!p->rc_frame && !p->rc_mb && p->dynamic_qp) {
		value = MFC_READL(MFC_REG_E_FIXED_PICTURE_QP);
		value &= ~(0xFF000000);
		value |= (p->config_qp & 0xFF) << 24;
		MFC_WRITEL(value, MFC_REG_E_FIXED_PICTURE_QP);
		mfc_debug(6, "[CTRLS] Dynamic QP changed %#x\n",
				MFC_READL(MFC_REG_E_FIXED_PICTURE_QP));
	}

	return 0;
}

static int mfc_enc_get_buf_ctrls_val(struct mfc_ctx *ctx, struct list_head *head)
{
	struct mfc_buf_ctrl *buf_ctrl;
	struct mfc_dev *dev = ctx->dev;
	unsigned int value = 0;

	list_for_each_entry(buf_ctrl, head, list) {
		if (!(buf_ctrl->type & MFC_CTRL_TYPE_GET))
			continue;

		if (buf_ctrl->mode == MFC_CTRL_MODE_SFR)
			value = MFC_READL(buf_ctrl->addr);
		else if (buf_ctrl->mode == MFC_CTRL_MODE_CST)
			value = call_bop(buf_ctrl, read_cst, ctx, buf_ctrl);

		value = (value >> buf_ctrl->shft) & buf_ctrl->mask;

		buf_ctrl->val = value;
		buf_ctrl->has_new = 1;

		mfc_debug(6, "[CTRLS] Get buffer control id: 0x%08x, val: %d\n",
				buf_ctrl->id, buf_ctrl->val);
	}

	return 0;
}

static int mfc_enc_set_buf_ctrls_val_nal_q(struct mfc_ctx *ctx,
			struct list_head *head, EncoderInputStr *pInStr)
{
	struct mfc_buf_ctrl *buf_ctrl;
	struct mfc_enc *enc = ctx->enc_priv;
	struct temporal_layer_info temporal_LC;
	unsigned int i, param_change;
	struct mfc_enc_params *p = &enc->params;

	mfc_debug_enter();

	list_for_each_entry(buf_ctrl, head, list) {
		if (!(buf_ctrl->type & MFC_CTRL_TYPE_SET) || !buf_ctrl->has_new)
			continue;
		param_change = 0;
		switch (buf_ctrl->id) {
		case V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG:
			pInStr->PictureTag &= ~(buf_ctrl->mask << buf_ctrl->shft);
			pInStr->PictureTag |=
				(buf_ctrl->val & buf_ctrl->mask) << buf_ctrl->shft;
			enc->stored_tag = buf_ctrl->val;
			break;
		case V4L2_CID_MPEG_MFC51_VIDEO_FORCE_FRAME_TYPE:
			pInStr->FrameInsertion &= ~(buf_ctrl->mask << buf_ctrl->shft);
			pInStr->FrameInsertion |=
				(buf_ctrl->val & buf_ctrl->mask) << buf_ctrl->shft;
			break;
		case V4L2_CID_MPEG_MFC51_VIDEO_I_PERIOD_CH:
			pInStr->GopConfig &= ~(buf_ctrl->mask << buf_ctrl->shft);
			pInStr->GopConfig |=
				(buf_ctrl->val & buf_ctrl->mask) << buf_ctrl->shft;
			pInStr->GopConfig2 &= ~(0x3FFF);
			pInStr->GopConfig2 |= (buf_ctrl->val >> 16) & 0x3FFF;
			param_change = 1;
			break;
		case V4L2_CID_MPEG_MFC51_VIDEO_FRAME_RATE_CH:
			pInStr->RcFrameRate &= ~(buf_ctrl->mask << buf_ctrl->shft);
			pInStr->RcFrameRate |=
				(buf_ctrl->val & buf_ctrl->mask) << buf_ctrl->shft;
			param_change = 1;
			break;
		case V4L2_CID_MPEG_MFC51_VIDEO_BIT_RATE_CH:
			pInStr->RcBitRate &= ~(buf_ctrl->mask << buf_ctrl->shft);
			pInStr->RcBitRate |=
				(buf_ctrl->val & buf_ctrl->mask) << buf_ctrl->shft;
			param_change = 1;
			break;
		case V4L2_CID_MPEG_VIDEO_H264_MAX_QP:
		case V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP:
		case V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP:
		case V4L2_CID_MPEG_VIDEO_H263_MAX_QP:
		case V4L2_CID_MPEG_VIDEO_VP8_MAX_QP:
		case V4L2_CID_MPEG_VIDEO_VP9_MAX_QP:
		case V4L2_CID_MPEG_VIDEO_H264_MIN_QP:
		case V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP:
		case V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP:
		case V4L2_CID_MPEG_VIDEO_H263_MIN_QP:
		case V4L2_CID_MPEG_VIDEO_VP8_MIN_QP:
		case V4L2_CID_MPEG_VIDEO_VP9_MIN_QP:
			pInStr->RcQpBound &= ~(buf_ctrl->mask << buf_ctrl->shft);
			pInStr->RcQpBound |=
				(buf_ctrl->val & buf_ctrl->mask) << buf_ctrl->shft;
			param_change = 1;
			break;
		case V4L2_CID_MPEG_VIDEO_H264_MAX_QP_P:
		case V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP_P:
		case V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP_P:
		case V4L2_CID_MPEG_VIDEO_H263_MAX_QP_P:
		case V4L2_CID_MPEG_VIDEO_VP8_MAX_QP_P:
		case V4L2_CID_MPEG_VIDEO_VP9_MAX_QP_P:
		case V4L2_CID_MPEG_VIDEO_H264_MIN_QP_P:
		case V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP_P:
		case V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP_P:
		case V4L2_CID_MPEG_VIDEO_H263_MIN_QP_P:
		case V4L2_CID_MPEG_VIDEO_VP8_MIN_QP_P:
		case V4L2_CID_MPEG_VIDEO_VP9_MIN_QP_P:
		case V4L2_CID_MPEG_VIDEO_H264_MAX_QP_B:
		case V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP_B:
		case V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP_B:
		case V4L2_CID_MPEG_VIDEO_H264_MIN_QP_B:
		case V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP_B:
		case V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP_B:
			pInStr->RcQpBoundPb &= ~(buf_ctrl->mask << buf_ctrl->shft);
			pInStr->RcQpBoundPb |=
				(buf_ctrl->val & buf_ctrl->mask) << buf_ctrl->shft;
			param_change = 1;
			break;
		case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_CH:
		case V4L2_CID_MPEG_VIDEO_VP8_HIERARCHICAL_CODING_LAYER_CH:
		case V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_CH:
		case V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_CH:
			memcpy(&temporal_LC,
				enc->sh_handle_svc.vaddr, sizeof(struct temporal_layer_info));

			if (((temporal_LC.temporal_layer_count & 0x7) < 1) ||
				((temporal_LC.temporal_layer_count > 3) && IS_VP8_ENC(ctx)) ||
				((temporal_LC.temporal_layer_count > 3) && IS_VP9_ENC(ctx))) {
				/* claer NUM_T_LAYER_CHANGE */
				mfc_err_ctx("[NALQ][HIERARCHICAL] layer count(%d) is invalid\n",
						temporal_LC.temporal_layer_count);
				return 0;
			}

			if (IS_H264_ENC(ctx))
				p->codec.h264.num_hier_layer =
					temporal_LC.temporal_layer_count & 0x7;

			/* enable RC_BIT_RATE_CHANGE */
			if (temporal_LC.temporal_layer_bitrate[0] > 0 || p->hier_bitrate_ctrl)
				pInStr->ParamChange |= (1 << 2);
			else
				pInStr->ParamChange &= ~(1 << 2);

			/* enalbe NUM_T_LAYER_CHANGE */
			if (temporal_LC.temporal_layer_count & 0x7)
				pInStr->ParamChange |= (1 << 10);
			else
				pInStr->ParamChange &= ~(1 << 10);
			mfc_debug(3, "[NALQ][HIERARCHICAL] layer count %d\n",
					temporal_LC.temporal_layer_count & 0x7);

			pInStr->NumTLayer &= ~(0x7);
			pInStr->NumTLayer |= (temporal_LC.temporal_layer_count & 0x7);
			pInStr->NumTLayer &= ~(0x1 << 8);
			pInStr->NumTLayer |= (p->hier_bitrate_ctrl & 0x1) << 8;
			for (i = 0; i < (temporal_LC.temporal_layer_count & 0x7); i++) {
				mfc_debug(3, "[NALQ][HIERARCHICAL] layer bitrate[%d] %d (FW ctrl: %d)\n",
					i, temporal_LC.temporal_layer_bitrate[i], p->hier_bitrate_ctrl);
				pInStr->HierarchicalBitRateLayer[i] =
					temporal_LC.temporal_layer_bitrate[i];
			}

			/* priority change */
			if (IS_H264_ENC(ctx)) {
				for (i = 0; i < (temporal_LC.temporal_layer_count & 0x7); i++) {
					if (i <= 4)
						pInStr->H264HDSvcExtension0 |=
							((p->codec.h264.base_priority & 0x3f) + i) << (6 * i);
					else
						pInStr->H264HDSvcExtension1 |=
							((p->codec.h264.base_priority & 0x3f) + i) << (6 * (i - 5));
				}
				mfc_debug(3, "[NALQ][HIERARCHICAL] EXTENSION0 %#x, EXTENSION1 %#x\n",
						pInStr->H264HDSvcExtension0, pInStr->H264HDSvcExtension1);

				pInStr->ParamChange |= (1 << 12);
			}
			break;
		case V4L2_CID_MPEG_VIDEO_H264_LEVEL:
			pInStr->PictureProfile &= ~(buf_ctrl->mask << buf_ctrl->shft);
			pInStr->PictureProfile |=
				(buf_ctrl->val & buf_ctrl->mask) << buf_ctrl->shft;
			pInStr->PictureProfile &= ~(0xf);
			pInStr->PictureProfile |= p->codec.h264.profile & 0xf;
			p->codec.h264.level = buf_ctrl->val;
			param_change = 1;
			break;
		case V4L2_CID_MPEG_VIDEO_H264_PROFILE:
			pInStr->PictureProfile &= ~(buf_ctrl->mask << buf_ctrl->shft);
			pInStr->PictureProfile |=
				(buf_ctrl->val & buf_ctrl->mask) << buf_ctrl->shft;
			pInStr->PictureProfile &= ~(0xff << 8);
			pInStr->PictureProfile |= (p->codec.h264.level << 8) & 0xff00;
			p->codec.h264.profile = buf_ctrl->val;
			param_change = 1;
			break;
		case V4L2_CID_MPEG_MFC_H264_MARK_LTR:
			pInStr->H264NalControl &= ~(buf_ctrl->mask << buf_ctrl->shft);
			pInStr->H264NalControl |=
				(buf_ctrl->val & buf_ctrl->mask) << buf_ctrl->shft;
			pInStr->H264NalControl &= ~(0x7 << 8);
			pInStr->H264NalControl |= (buf_ctrl->val & 0x7) << 8;
			break;
		case V4L2_CID_MPEG_MFC_H264_USE_LTR:
			pInStr->H264NalControl &= ~(buf_ctrl->mask << buf_ctrl->shft);
			pInStr->H264NalControl |=
				(buf_ctrl->val & buf_ctrl->mask) << buf_ctrl->shft;
			pInStr->H264NalControl &= ~(0xF << 11);
			pInStr->H264NalControl |= (buf_ctrl->val & 0xF) << 11;
			break;
		case V4L2_CID_MPEG_MFC_H264_BASE_PRIORITY:
			for (i = 0; i < (p->codec.h264.num_hier_layer & 0x7); i++)
				if (i <= 4)
					pInStr->H264HDSvcExtension0 |=
						((buf_ctrl->val & 0x3f) + i) << (6 * i);
				else
					pInStr->H264HDSvcExtension1 |=
						((buf_ctrl->val & 0x3f) + i) << (6 * (i - 5));
			p->codec.h264.base_priority = buf_ctrl->val;
			param_change = 1;
			break;
		case V4L2_CID_MPEG_MFC_CONFIG_QP:
			pInStr->FixedPictureQp &= ~(buf_ctrl->mask << buf_ctrl->shft);
			pInStr->FixedPictureQp |=
				(buf_ctrl->val & buf_ctrl->mask) << buf_ctrl->shft;
			p->config_qp = buf_ctrl->val;
			break;
		case V4L2_CID_MPEG_VIDEO_ROI_CONTROL:
			pInStr->RcRoiCtrl &= ~(buf_ctrl->mask << buf_ctrl->shft);
			pInStr->RcRoiCtrl |=
				(buf_ctrl->val & buf_ctrl->mask) << buf_ctrl->shft;
			pInStr->RoiBufferAddr = enc->roi_buf[buf_ctrl->old_val2].daddr;
			mfc_debug(3, "[NALQ][ROI] buffer[%d] addr %#llx, QP val: %#x\n",
					buf_ctrl->old_val2,
					enc->roi_buf[buf_ctrl->old_val2].daddr,
					buf_ctrl->val);
			break;
		case V4L2_CID_MPEG_VIDEO_YSUM:
			pInStr->Weight &= ~(buf_ctrl->mask << buf_ctrl->shft);
			pInStr->Weight |=
				(buf_ctrl->val & buf_ctrl->mask) << buf_ctrl->shft;
			break;
		case V4L2_CID_MPEG_VIDEO_RATIO_OF_INTRA:
			pInStr->RcMode &= ~(buf_ctrl->mask << buf_ctrl->shft);
			pInStr->RcMode |=
				(buf_ctrl->val & buf_ctrl->mask) << buf_ctrl->shft;
			param_change = 1;
			break;
		/* If new dynamic controls are added, insert here */
		default:
			mfc_info_ctx("[NALQ] can't find control, id: 0x%x\n",
					buf_ctrl->id);
		}

		if (param_change)
			pInStr->ParamChange |= (1 << buf_ctrl->flag_shft);

		buf_ctrl->has_new = 0;
		buf_ctrl->updated = 1;

		mfc_debug(6, "[NALQ][CTRLS] Set buffer control id: 0x%08x, val: %d\n",
				buf_ctrl->id, buf_ctrl->val);
	}

	if (!p->rc_frame && !p->rc_mb && p->dynamic_qp) {
		pInStr->FixedPictureQp &= ~(0xFF000000);
		pInStr->FixedPictureQp |= (p->config_qp & 0xFF) << 24;
		mfc_debug(6, "[NALQ][CTRLS] Dynamic QP changed %#x\n",
				pInStr->FixedPictureQp);
	}

	mfc_debug_leave();

	return 0;
}

static int mfc_enc_get_buf_ctrls_val_nal_q(struct mfc_ctx *ctx,
			struct list_head *head, EncoderOutputStr *pOutStr)
{
	struct mfc_buf_ctrl *buf_ctrl;
	struct mfc_enc *enc = ctx->enc_priv;
	unsigned int value = 0;

	mfc_debug_enter();

	list_for_each_entry(buf_ctrl, head, list) {
		if (!(buf_ctrl->type & MFC_CTRL_TYPE_GET))
			continue;
		switch (buf_ctrl->id) {
		case V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG:
			value = pOutStr->PictureTag;
			break;
		case V4L2_CID_MPEG_MFC51_VIDEO_LUMA_ADDR:
			value = pOutStr->EncodedFrameAddr[0];
			break;
		case V4L2_CID_MPEG_MFC51_VIDEO_CHROMA_ADDR:
			value = pOutStr->EncodedFrameAddr[1];
			break;
		case V4L2_CID_MPEG_MFC51_VIDEO_FRAME_STATUS:
			value = !enc->in_slice;
			break;
		/* If new dynamic controls are added, insert here */
		default:
			mfc_info_ctx("[NALQ] can't find control, id: 0x%x\n",
					buf_ctrl->id);
		}
		value = (value >> buf_ctrl->shft) & buf_ctrl->mask;

		buf_ctrl->val = value;
		buf_ctrl->has_new = 1;

		mfc_debug(6, "[NALQ][CTRLS] Get buffer control id: 0x%08x, val: %d\n",
				buf_ctrl->id, buf_ctrl->val);
	}

	mfc_debug_leave();

	return 0;
}

static int mfc_enc_recover_buf_ctrls_val(struct mfc_ctx *ctx,
						struct list_head *head)
{
	struct mfc_buf_ctrl *buf_ctrl;
	struct mfc_dev *dev = ctx->dev;
	unsigned int value = 0;

	list_for_each_entry(buf_ctrl, head, list) {
		if (!(buf_ctrl->type & MFC_CTRL_TYPE_SET)
			|| !buf_ctrl->is_volatile
			|| !buf_ctrl->updated)
			continue;

		if (buf_ctrl->mode == MFC_CTRL_MODE_SFR)
			value = MFC_READL(buf_ctrl->addr);

		value &= ~(buf_ctrl->mask << buf_ctrl->shft);
		value |= ((buf_ctrl->old_val & buf_ctrl->mask)
							<< buf_ctrl->shft);

		if (buf_ctrl->mode == MFC_CTRL_MODE_SFR)
			MFC_WRITEL(value, buf_ctrl->addr);

		/* clear change flag bit */
		if (buf_ctrl->flag_mode == MFC_CTRL_MODE_SFR) {
			value = MFC_READL(buf_ctrl->flag_addr);
			value &= ~(1 << buf_ctrl->flag_shft);
			MFC_WRITEL(value, buf_ctrl->flag_addr);
		}

		mfc_debug(6, "[CTRLS] Recover buffer control id: 0x%08x, old val: %d\n",
				buf_ctrl->id, buf_ctrl->old_val);

		if (buf_ctrl->id == V4L2_CID_MPEG_MFC51_VIDEO_I_PERIOD_CH) {
			value = MFC_READL(MFC_REG_E_GOP_CONFIG2);
			value &= ~(0x3FFF);
			value |= (buf_ctrl->old_val >> 16) & 0x3FFF;
			MFC_WRITEL(value, MFC_REG_E_GOP_CONFIG2);
		}
		if (buf_ctrl->id == V4L2_CID_MPEG_VIDEO_H264_LEVEL) {
			value = MFC_READL(MFC_REG_E_PICTURE_PROFILE);
			value &= ~(0x000F);
			value |= (buf_ctrl->old_val >> 8) & 0x000F;
			MFC_WRITEL(value, MFC_REG_E_PICTURE_PROFILE);
		}
		if (buf_ctrl->id == V4L2_CID_MPEG_VIDEO_H264_PROFILE) {
			value = MFC_READL(MFC_REG_E_PICTURE_PROFILE);
			value &= ~(0xFF00);
			value |= buf_ctrl->old_val & 0xFF00;
			MFC_WRITEL(value, MFC_REG_E_PICTURE_PROFILE);
		}
		if (buf_ctrl->id == V4L2_CID_MPEG_MFC_H264_BASE_PRIORITY) {
			MFC_WRITEL(buf_ctrl->old_val, MFC_REG_E_H264_HD_SVC_EXTENSION_0);
			MFC_WRITEL(buf_ctrl->old_val2, MFC_REG_E_H264_HD_SVC_EXTENSION_1);
		}
		if (buf_ctrl->id
			== V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_CH ||
			buf_ctrl->id
			== V4L2_CID_MPEG_VIDEO_VP8_HIERARCHICAL_CODING_LAYER_CH ||
			buf_ctrl->id
			== V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_CH) {
			MFC_WRITEL(buf_ctrl->old_val2, MFC_REG_E_NUM_T_LAYER);
			/* clear RC_BIT_RATE_CHANGE */
			value = MFC_READL(buf_ctrl->flag_addr);
			value &= ~(1 << 2);
			MFC_WRITEL(value, buf_ctrl->flag_addr);
		}
		if (buf_ctrl->id == V4L2_CID_MPEG_MFC_H264_MARK_LTR) {
			value = MFC_READL(MFC_REG_E_H264_NAL_CONTROL);
			value &= ~(0x7 << 8);
			value |= (buf_ctrl->old_val2 & 0x7) << 8;
			MFC_WRITEL(value, MFC_REG_E_H264_NAL_CONTROL);
		}
		if (buf_ctrl->id == V4L2_CID_MPEG_MFC_H264_USE_LTR) {
			value = MFC_READL(MFC_REG_E_H264_NAL_CONTROL);
			value &= ~(0xF << 11);
			value |= (buf_ctrl->old_val2 & 0xF) << 11;
			MFC_WRITEL(value, MFC_REG_E_H264_NAL_CONTROL);
		}
		buf_ctrl->updated = 0;
	}

	return 0;
}

static int mfc_enc_recover_buf_ctrls_nal_q(struct mfc_ctx *ctx,
		struct list_head *head)
{
	struct mfc_buf_ctrl *buf_ctrl;

	list_for_each_entry(buf_ctrl, head, list) {
		if (!(buf_ctrl->type & MFC_CTRL_TYPE_SET)
				|| !(buf_ctrl->updated))
			continue;

		buf_ctrl->has_new = 1;
		buf_ctrl->updated = 0;

		mfc_debug(6, "[NALQ][CTRLS] Recover buffer control id: 0x%08x, val: %d\n",
				buf_ctrl->id, buf_ctrl->val);
	}

	return 0;
}

struct mfc_ctrls_ops encoder_ctrls_ops = {
	.init_ctx_ctrls			= mfc_enc_init_ctx_ctrls,
	.cleanup_ctx_ctrls		= mfc_enc_cleanup_ctx_ctrls,
	.init_buf_ctrls			= mfc_enc_init_buf_ctrls,
	.reset_buf_ctrls		= mfc_enc_reset_buf_ctrls,
	.cleanup_buf_ctrls		= mfc_enc_cleanup_buf_ctrls,
	.to_buf_ctrls			= mfc_enc_to_buf_ctrls,
	.to_ctx_ctrls			= mfc_enc_to_ctx_ctrls,
	.set_buf_ctrls_val		= mfc_enc_set_buf_ctrls_val,
	.get_buf_ctrls_val		= mfc_enc_get_buf_ctrls_val,
	.recover_buf_ctrls_val		= mfc_enc_recover_buf_ctrls_val,
	.get_buf_update_val		= mfc_enc_get_buf_update_val,
	.set_buf_ctrls_val_nal_q_enc	= mfc_enc_set_buf_ctrls_val_nal_q,
	.get_buf_ctrls_val_nal_q_enc	= mfc_enc_get_buf_ctrls_val_nal_q,
	.recover_buf_ctrls_nal_q	= mfc_enc_recover_buf_ctrls_nal_q,
};
