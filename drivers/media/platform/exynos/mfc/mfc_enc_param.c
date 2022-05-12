/*
 * drivers/media/platform/exynos/mfc/mfc_enc_param.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "mfc_enc_param.h"

#include "mfc_reg_api.h"

/* Definition */
#define CBR_FIX_MAX			10
#define CBR_I_LIMIT_MAX			5
#define BPG_EXTENSION_TAG_SIZE		5

void mfc_set_slice_mode(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;

	/* multi-slice control */
	if (enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SICE_MODE_MAX_BYTES)
		MFC_RAW_WRITEL((enc->slice_mode + 0x4), MFC_REG_E_MSLICE_MODE);
	else if (enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_MB_ROW)
		MFC_RAW_WRITEL((enc->slice_mode - 0x2), MFC_REG_E_MSLICE_MODE);
	else if (enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_FIXED_BYTES)
		MFC_RAW_WRITEL((enc->slice_mode + 0x3), MFC_REG_E_MSLICE_MODE);
	else
		MFC_RAW_WRITEL(enc->slice_mode, MFC_REG_E_MSLICE_MODE);

	/* multi-slice MB number or bit size */
	if ((enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SICE_MODE_MAX_MB) ||
			(enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_MB_ROW)) {
		MFC_RAW_WRITEL(enc->slice_size_mb, MFC_REG_E_MSLICE_SIZE_MB);
	} else if ((enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SICE_MODE_MAX_BYTES) ||
			(enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_FIXED_BYTES)){
		MFC_RAW_WRITEL(enc->slice_size_bits, MFC_REG_E_MSLICE_SIZE_BITS);
	} else {
		MFC_RAW_WRITEL(0x0, MFC_REG_E_MSLICE_SIZE_MB);
		MFC_RAW_WRITEL(0x0, MFC_REG_E_MSLICE_SIZE_BITS);
	}
}

void mfc_set_aso_slice_order_h264(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_enc_params *p = &enc->params;
	struct mfc_h264_enc_params *p_264 = &p->codec.h264;
	int i;

	if (p_264->aso_enable) {
		for (i = 0; i < 8; i++)
			MFC_RAW_WRITEL(p_264->aso_slice_order[i],
				MFC_REG_E_H264_ASO_SLICE_ORDER_0 + i * 4);
	}
}

void mfc_set_enc_ts_delta(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_enc_params *p = &enc->params;
	unsigned int reg = 0;
	int ts_delta;

	ts_delta = mfc_enc_get_ts_delta(ctx);

	reg = MFC_RAW_READL(MFC_REG_E_TIME_STAMP_DELTA);
	reg &= ~(0xFFFF);
	reg |= (ts_delta & 0xFFFF);
	MFC_RAW_WRITEL(reg, MFC_REG_E_TIME_STAMP_DELTA);
	if (ctx->ts_last_interval)
		mfc_debug(3, "[DFR] fps %d -> %ld, delta: %d, reg: %#x\n",
				p->rc_framerate, USEC_PER_SEC / ctx->ts_last_interval,
				ts_delta, reg);
	else
		mfc_debug(3, "[DFR] fps %d -> 0, delta: %d, reg: %#x\n",
				p->rc_framerate, ts_delta, reg);
}

static void __mfc_set_gop_size(struct mfc_ctx *ctx, int ctrl_mode)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_enc_params *p = &enc->params;
	unsigned int reg = 0;

	if (ctrl_mode) {
		p->i_frm_ctrl_mode = 1;
		p->i_frm_ctrl = p->gop_size * (p->num_b_frame + 1);
		if (p->i_frm_ctrl >= 0x3FFFFFFF) {
			mfc_info_ctx("I frame interval is bigger than max: %d\n",
					p->i_frm_ctrl);
			p->i_frm_ctrl = 0x3FFFFFFF;
		}
	} else {
		p->i_frm_ctrl_mode = 0;
		p->i_frm_ctrl = p->gop_size;
	}

	mfc_debug(2, "I frame interval: %d, (P: %d, B: %d), ctrl mode: %d\n",
			p->i_frm_ctrl, p->gop_size,
			p->num_b_frame, p->i_frm_ctrl_mode);

	/* pictype : IDR period, number of B */
	reg = MFC_RAW_READL(MFC_REG_E_GOP_CONFIG);
	mfc_clear_set_bits(reg, 0xFFFF, 0, p->i_frm_ctrl);
	mfc_clear_set_bits(reg, 0x1, 19, p->i_frm_ctrl_mode);
	/* if B frame is used, the performance falls by half */
	mfc_clear_set_bits(reg, 0x3, 16, p->num_b_frame);
	MFC_RAW_WRITEL(reg, MFC_REG_E_GOP_CONFIG);

	reg = MFC_RAW_READL(MFC_REG_E_GOP_CONFIG2);
	mfc_clear_set_bits(reg, 0x3FFF, 0, (p->i_frm_ctrl >> 16));
	MFC_RAW_WRITEL(reg, MFC_REG_E_GOP_CONFIG2);
}

static void __mfc_set_default_params(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	int i;

	mfc_debug(2, "Set default param -  enc_param_num: %d\n", dev->pdata->enc_param_num);
	for (i = 0; i < dev->pdata->enc_param_num; i++) {
		if (i >= MFC_MAX_DEFAULT_PARAM) {
			mfc_err_dev("enc_param_num(%d) is over max number(%d)\n",
					dev->pdata->enc_param_num, MFC_MAX_DEFAULT_PARAM);
			break;
		}
		MFC_RAW_WRITEL(dev->pdata->enc_param_val[i], dev->pdata->enc_param_addr[i]);
		mfc_debug(2, "Set default param[%d] - addr:0x%x, val:0x%x\n",
				i, dev->pdata->enc_param_addr[i], dev->pdata->enc_param_val[i]);
	}
}

static void __mfc_init_regs(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;

	/* Register initialization */
	MFC_RAW_WRITEL(0x0, MFC_REG_E_FRAME_INSERTION);
	MFC_RAW_WRITEL(0x0, MFC_REG_E_ROI_BUFFER_ADDR);
	MFC_RAW_WRITEL(0x0, MFC_REG_E_PARAM_CHANGE);
	MFC_RAW_WRITEL(0x0, MFC_REG_E_PICTURE_TAG);
	MFC_RAW_WRITEL(0x0, MFC_REG_E_METADATA_BUFFER_ADDR);
	MFC_RAW_WRITEL(0x0, MFC_REG_E_METADATA_BUFFER_SIZE);
}

static void __mfc_set_enc_params(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_enc_params *p = &enc->params;
	unsigned int reg = 0;

	mfc_debug_enter();

	__mfc_init_regs(ctx);
	__mfc_set_default_params(ctx);

	/* width */
	MFC_RAW_WRITEL(ctx->crop_width, MFC_REG_E_CROPPED_FRAME_WIDTH);
	/* height */
	MFC_RAW_WRITEL(ctx->crop_height, MFC_REG_E_CROPPED_FRAME_HEIGHT);
	/* cropped offset */
	mfc_set_bits(reg, MFC_REG_E_FRAME_CROP_OFFSET_MASK,
			MFC_REG_E_FRAME_CROP_OFFSET_LEFT, ctx->crop_left);
	mfc_set_bits(reg, MFC_REG_E_FRAME_CROP_OFFSET_MASK,
			MFC_REG_E_FRAME_CROP_OFFSET_TOP, ctx->crop_top);
	MFC_RAW_WRITEL(reg, MFC_REG_E_FRAME_CROP_OFFSET);

	/* multi-slice control */
	/* multi-slice MB number or bit size */
	enc->slice_mode = p->slice_mode;

	if (p->slice_mode == V4L2_MPEG_VIDEO_MULTI_SICE_MODE_MAX_MB) {
		enc->slice_size_mb = p->slice_mb;
	} else if ((p->slice_mode == V4L2_MPEG_VIDEO_MULTI_SICE_MODE_MAX_BYTES) ||
			(p->slice_mode == V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_FIXED_BYTES)){
		enc->slice_size_bits = p->slice_bit;
	} else if (p->slice_mode == V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_MB_ROW) {
		enc->slice_size_mb = p->slice_mb_row * ((ctx->crop_width + 15) / 16);
	} else {
		enc->slice_size_mb = 0;
		enc->slice_size_bits = 0;
	}

	mfc_set_slice_mode(ctx);

	/* cyclic intra refresh */
	MFC_RAW_WRITEL(p->intra_refresh_mb, MFC_REG_E_IR_SIZE);

	reg = MFC_RAW_READL(MFC_REG_E_ENC_OPTIONS);
	/* frame skip mode */
	mfc_clear_set_bits(reg, 0x3, 0, p->frame_skip_mode);
	/* seq header ctrl */
	mfc_clear_set_bits(reg, 0x1, 2, p->seq_hdr_mode);
	/* cyclic intra refresh */
	mfc_clear_bits(reg, 0x1, 4);
	if (p->intra_refresh_mb)
		mfc_set_bits(reg, 0x1, 4, 0x1);
	/* disable seq header generation if OTF mode */
	mfc_clear_bits(reg, 0x1, 6);
	if (ctx->otf_handle) {
		mfc_set_bits(reg, 0x1, 6, 0x1);
		mfc_debug(2, "[OTF] SEQ_HEADER_GENERATION is disabled\n");
	}
	/* 'NON_REFERENCE_STORE_ENABLE' for debugging */
	mfc_clear_bits(reg, 0x1, 9);
	/* Disable parallel processing if nal_q_parallel_disable was set */
	mfc_clear_bits(reg, 0x1, 18);
	if (nal_q_parallel_disable)
		mfc_set_bits(reg, 0x1, 18, 0x1);
	MFC_RAW_WRITEL(reg, MFC_REG_E_ENC_OPTIONS);

	mfc_set_pixel_format(ctx, ctx->src_fmt->fourcc);

	/* padding control & value */
	MFC_RAW_WRITEL(0x0, MFC_REG_E_PADDING_CTRL);
	if (p->pad) {
		reg = 0;
		/** enable */
		mfc_set_bits(reg, 0x1, 31, 0x1);
		/** cr value */
		mfc_set_bits(reg, 0xFF, 16, p->pad_cr);
		/** cb value */
		mfc_set_bits(reg, 0xFF, 8, p->pad_cb);
		/** y value */
		mfc_set_bits(reg, 0xFF, 0, p->pad_luma);
		MFC_RAW_WRITEL(reg, MFC_REG_E_PADDING_CTRL);
	}

	/* rate control config. */
	reg = MFC_RAW_READL(MFC_REG_E_RC_CONFIG);
	/* macroblock level rate control */
	mfc_clear_set_bits(reg, 0x1, 8, p->rc_mb);
	/* frame-level rate control */
	mfc_clear_set_bits(reg, 0x1, 9, p->rc_frame);
	/* drop control */
	mfc_clear_set_bits(reg, 0x1, 10, p->drop_control);
	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->enc_ts_delta))
		mfc_clear_set_bits(reg, 0x1, 20, 1);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_CONFIG);

	/*
	 * frame rate
	 * delta is timestamp diff
	 * ex) 30fps: 33, 60fps: 16
	 */
	p->rc_frame_delta = p->rc_framerate_res / p->rc_framerate;
	reg = MFC_RAW_READL(MFC_REG_E_RC_FRAME_RATE);
	mfc_clear_set_bits(reg, 0xFFFF, 16, p->rc_framerate_res);
	mfc_clear_set_bits(reg, 0xFFFF, 0, p->rc_frame_delta);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_FRAME_RATE);

	/* bit rate */
	ctx->Kbps = p->rc_bitrate / 1024;
	MFC_RAW_WRITEL(p->rc_bitrate, MFC_REG_E_RC_BIT_RATE);

	reg = MFC_RAW_READL(MFC_REG_E_RC_MODE);
	mfc_clear_bits(reg, 0x3, 0);
	mfc_clear_bits(reg, 0x3, 4);
	mfc_clear_bits(reg, 0xFF, 8);
	if (p->rc_frame) {
		if (p->rc_reaction_coeff <= CBR_I_LIMIT_MAX) {
			mfc_set_bits(reg, 0x3, 0, MFC_REG_E_RC_CBR_I_LIMIT);
			/*
			 * Ratio of intra for max frame size
			 * is controled when only CBR_I_LIMIT mode.
			 * And CBR_I_LIMIT mode is valid for H.264, HEVC codec
			 */
			if (p->ratio_intra)
				mfc_set_bits(reg, 0xFF, 8, p->ratio_intra);
		} else if (p->rc_reaction_coeff <= CBR_FIX_MAX) {
			mfc_set_bits(reg, 0x3, 0, MFC_REG_E_RC_CBR_FIX);
		} else {
			mfc_set_bits(reg, 0x3, 0, MFC_REG_E_RC_VBR);
		}

		if (p->rc_mb)
			mfc_set_bits(reg, 0x3, 4, p->rc_pvc);
	}
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_MODE);

	/* extended encoder ctrl */
	/** vbv buffer size */
	reg = MFC_RAW_READL(MFC_REG_E_VBV_BUFFER_SIZE);
	mfc_clear_bits(reg, 0xFF, 0);
	if (p->frame_skip_mode == V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT)
		mfc_set_bits(reg, 0xFF, 0, p->vbv_buf_size);
	MFC_RAW_WRITEL(reg, MFC_REG_E_VBV_BUFFER_SIZE);

	mfc_debug_leave();
}

static void __mfc_set_temporal_svc_h264(struct mfc_ctx *ctx, struct mfc_h264_enc_params *p_264)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_enc_params *p = &enc->params;
	unsigned int reg = 0, reg2 = 0;
	int i;

	reg = MFC_RAW_READL(MFC_REG_E_H264_OPTIONS_2);
	/* pic_order_cnt_type = 0 for backward compatibilities */
	mfc_clear_bits(reg, 0x3, 0);
	/* Enable LTR */
	mfc_clear_bits(reg, 0x1, 2);
	if ((p_264->enable_ltr & 0x1) || (p_264->num_of_ltr > 0))
		mfc_set_bits(reg, 0x1, 2, 0x1);
	/* Number of LTR */
	mfc_clear_bits(reg, 0x3, 7);
	if (p_264->num_of_ltr > 2)
		mfc_set_bits(reg, 0x3, 7, (p_264->num_of_ltr - 2));
	MFC_RAW_WRITEL(reg, MFC_REG_E_H264_OPTIONS_2);

	/* Temporal SVC - qp type, layer number */
	reg = MFC_RAW_READL(MFC_REG_E_NUM_T_LAYER);
	mfc_clear_set_bits(reg, 0x1, 3, p_264->hier_qp_type);
	mfc_clear_set_bits(reg, 0x7, 0, p_264->num_hier_layer);
	mfc_clear_bits(reg, 0x7, 4);
	if (p_264->hier_ref_type) {
		mfc_set_bits(reg, 0x1, 7, 0x1);
		mfc_set_bits(reg, 0x7, 4, p->num_hier_max_layer);
	} else {
		mfc_clear_bits(reg, 0x1, 7);
		mfc_set_bits(reg, 0x7, 4, 0x7);
	}
	mfc_clear_set_bits(reg, 0x1, 8, p->hier_bitrate_ctrl);
	MFC_RAW_WRITEL(reg, MFC_REG_E_NUM_T_LAYER);
	mfc_debug(3, "[HIERARCHICAL] hier_qp_enable %d, enable_ltr %d, "
		"num_hier_layer %d, max_layer %d, hier_ref_type %d, NUM_T_LAYER 0x%x\n",
		p_264->hier_qp_enable, p_264->enable_ltr, p_264->num_hier_layer,
		p->num_hier_max_layer, p_264->hier_ref_type, reg);
	/* QP & Bitrate for each layer */
	for (i = 0; i < 7; i++) {
		MFC_RAW_WRITEL(p_264->hier_qp_layer[i],
				MFC_REG_E_HIERARCHICAL_QP_LAYER0 + i * 4);
		/* If hier_bitrate_ctrl is set to 1, this is meaningless */
		MFC_RAW_WRITEL(p_264->hier_bit_layer[i],
				MFC_REG_E_HIERARCHICAL_BIT_RATE_LAYER0 + i * 4);
		mfc_debug(3, "[HIERARCHICAL] layer[%d] QP: %#x, bitrate: %d(FW ctrl: %d)\n",
					i, p_264->hier_qp_layer[i],
					p_264->hier_bit_layer[i], p->hier_bitrate_ctrl);
	}
	if (p_264->set_priority) {
		reg = 0;
		reg2 = 0;
		for (i = 0; i < (p_264->num_hier_layer & 0x7); i++) {
			if (i <= 4)
				mfc_set_bits(reg, 0x3F, (6 * i), (p_264->base_priority + i));
			else
				mfc_set_bits(reg2, 0x3F, (6 * (i - 5)), (p_264->base_priority + i));
		}
		MFC_RAW_WRITEL(reg, MFC_REG_E_H264_HD_SVC_EXTENSION_0);
		MFC_RAW_WRITEL(reg2, MFC_REG_E_H264_HD_SVC_EXTENSION_1);
		mfc_debug(3, "[HIERARCHICAL] priority EXTENSION0: %#x, EXTENSION1: %#x\n",
							reg, reg2);
	}
}

static void __mfc_set_fmo_slice_map_h264(struct mfc_ctx *ctx, struct mfc_h264_enc_params *p_264)
{
	struct mfc_dev *dev = ctx->dev;
	int i;

	if (p_264->fmo_enable) {
		switch (p_264->fmo_slice_map_type) {
		case V4L2_MPEG_VIDEO_H264_FMO_MAP_TYPE_INTERLEAVED_SLICES:
			if (p_264->fmo_slice_num_grp > 4)
				p_264->fmo_slice_num_grp = 4;
			for (i = 0; i < (p_264->fmo_slice_num_grp & 0xF); i++)
				MFC_RAW_WRITEL(p_264->fmo_run_length[i] - 1,
				MFC_REG_E_H264_FMO_RUN_LENGTH_MINUS1_0 + i*4);
			break;
		case V4L2_MPEG_VIDEO_H264_FMO_MAP_TYPE_SCATTERED_SLICES:
			if (p_264->fmo_slice_num_grp > 4)
				p_264->fmo_slice_num_grp = 4;
			break;
		case V4L2_MPEG_VIDEO_H264_FMO_MAP_TYPE_RASTER_SCAN:
		case V4L2_MPEG_VIDEO_H264_FMO_MAP_TYPE_WIPE_SCAN:
			if (p_264->fmo_slice_num_grp > 2)
				p_264->fmo_slice_num_grp = 2;
			MFC_RAW_WRITEL(p_264->fmo_sg_dir & 0x1,
				MFC_REG_E_H264_FMO_SLICE_GRP_CHANGE_DIR);
			/* the valid range is 0 ~ number of macroblocks -1 */
			MFC_RAW_WRITEL(p_264->fmo_sg_rate, MFC_REG_E_H264_FMO_SLICE_GRP_CHANGE_RATE_MINUS1);
			break;
		default:
			mfc_err_ctx("Unsupported map type for FMO: %d\n",
					p_264->fmo_slice_map_type);
			p_264->fmo_slice_map_type = 0;
			p_264->fmo_slice_num_grp = 1;
			break;
		}

		MFC_RAW_WRITEL(p_264->fmo_slice_map_type, MFC_REG_E_H264_FMO_SLICE_GRP_MAP_TYPE);
		MFC_RAW_WRITEL(p_264->fmo_slice_num_grp - 1, MFC_REG_E_H264_FMO_NUM_SLICE_GRP_MINUS1);
	} else {
		MFC_RAW_WRITEL(0, MFC_REG_E_H264_FMO_NUM_SLICE_GRP_MINUS1);
	}
}

static void __mfc_set_enc_params_h264(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_enc_params *p = &enc->params;
	struct mfc_h264_enc_params *p_264 = &p->codec.h264;
	unsigned int reg = 0;

	mfc_debug_enter();

	p->rc_framerate_res = FRAME_RATE_RESOLUTION;
	__mfc_set_enc_params(ctx);

	if (p_264->num_hier_layer & 0x7) {
		/* set gop_size without i_frm_ctrl mode */
		__mfc_set_gop_size(ctx, 0);
	} else {
		/* set gop_size with i_frm_ctrl mode */
		__mfc_set_gop_size(ctx, 1);
	}

	/* UHD encoding case */
	if(IS_UHD_RES(ctx)) {
		if (p_264->level < 51) {
			mfc_info_ctx("Set Level 5.1 for UHD\n");
			p_264->level = 51;
		}
		if (p_264->profile != 0x2) {
			mfc_info_ctx("Set High profile for UHD\n");
			p_264->profile = 0x2;
		}
	}

	/* profile & level */
	reg = 0;
	/** level */
	mfc_clear_set_bits(reg, 0xFF, 8, p_264->level);
	/** profile - 0 ~ 3 */
	mfc_clear_set_bits(reg, 0x3F, 0, p_264->profile);
	MFC_RAW_WRITEL(reg, MFC_REG_E_PICTURE_PROFILE);

	reg = MFC_RAW_READL(MFC_REG_E_H264_OPTIONS);
	/* entropy coding mode */
	mfc_clear_set_bits(reg, 0x1, 0, p_264->entropy_mode);
	/* loop filter ctrl */
	mfc_clear_set_bits(reg, 0x3, 1, p_264->loop_filter_mode);
	/* interlace */
	mfc_clear_set_bits(reg, 0x1, 3, p_264->interlace);
	/* intra picture period for H.264 open GOP */
	mfc_clear_set_bits(reg, 0x1, 4, p_264->open_gop);
	/* extended encoder ctrl */
	mfc_clear_set_bits(reg, 0x1, 5, p_264->ar_vui);
	/* ASO enable */
	mfc_clear_set_bits(reg, 0x1, 6, p_264->aso_enable);
	/* if num_refs_for_p is 2, the performance falls by half */
	mfc_clear_set_bits(reg, 0x1, 7, (p->num_refs_for_p - 1));
	/* Temporal SVC - hier qp enable */
	mfc_clear_set_bits(reg, 0x1, 8, p_264->hier_qp_enable);
	/* Weighted Prediction enable */
	mfc_clear_set_bits(reg, 0x3, 9, p->weighted_enable);
	/* 8x8 transform enable [12]: INTER_8x8_TRANS_ENABLE */
	mfc_clear_set_bits(reg, 0x1, 12, p_264->_8x8_transform);
	/* 8x8 transform enable [13]: INTRA_8x8_TRANS_ENABLE */
	mfc_clear_set_bits(reg, 0x1, 13, p_264->_8x8_transform);
	/* 'CONSTRAINED_INTRA_PRED_ENABLE' is disable */
	mfc_clear_bits(reg, 0x1, 14);
	/*
	 * CONSTRAINT_SET0_FLAG: all constraints specified in
	 * Baseline Profile
	 */
	mfc_set_bits(reg, 0x1, 26, 0x1);
	/* sps pps control */
	mfc_clear_set_bits(reg, 0x1, 29, p_264->prepend_sps_pps_to_idr);
	/* enable sps pps control in OTF scenario */
	if (ctx->otf_handle) {
		mfc_set_bits(reg, 0x1, 29, 0x1);
		mfc_debug(2, "[OTF] SPS_PPS_CONTROL enabled\n");
	}
	/* VUI parameter disable */
	mfc_clear_set_bits(reg, 0x1, 30, p_264->vui_enable);
	MFC_RAW_WRITEL(reg, MFC_REG_E_H264_OPTIONS);

	/* cropped height */
	if (p_264->interlace)
		MFC_RAW_WRITEL(ctx->crop_height >> 1, MFC_REG_E_CROPPED_FRAME_HEIGHT);

	/* loopfilter alpha offset */
	reg = MFC_RAW_READL(MFC_REG_E_H264_LF_ALPHA_OFFSET);
	mfc_clear_set_bits(reg, 0x1F, 0, p_264->loop_filter_alpha);
	MFC_RAW_WRITEL(reg, MFC_REG_E_H264_LF_ALPHA_OFFSET);

	/* loopfilter beta offset */
	reg = MFC_RAW_READL(MFC_REG_E_H264_LF_BETA_OFFSET);
	mfc_clear_set_bits(reg, 0x1F, 0, p_264->loop_filter_beta);
	MFC_RAW_WRITEL(reg, MFC_REG_E_H264_LF_BETA_OFFSET);

	/* rate control config. */
	reg = MFC_RAW_READL(MFC_REG_E_RC_CONFIG);
	/** frame QP */
	mfc_clear_set_bits(reg, 0xFF, 0, p_264->rc_frame_qp);
	mfc_clear_bits(reg, 0x1, 11);
	if (!p->rc_frame && !p->rc_mb && p->dynamic_qp)
		mfc_set_bits(reg, 0x1, 11, 0x1);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_CONFIG);

	/* max & min value of QP for I frame */
	reg = MFC_RAW_READL(MFC_REG_E_RC_QP_BOUND);
	/** max I frame QP */
	mfc_clear_set_bits(reg, 0xFF, 8, p_264->rc_max_qp);
	/** min I frame QP */
	mfc_clear_set_bits(reg, 0xFF, 0, p_264->rc_min_qp);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_QP_BOUND);

	/* max & min value of QP for P/B frame */
	reg = MFC_RAW_READL(MFC_REG_E_RC_QP_BOUND_PB);
	/** max B frame QP */
	mfc_clear_set_bits(reg, 0xFF, 24, p_264->rc_max_qp_b);
	/** min B frame QP */
	mfc_clear_set_bits(reg, 0xFF, 16, p_264->rc_min_qp_b);
	/** max P frame QP */
	mfc_clear_set_bits(reg, 0xFF, 8, p_264->rc_max_qp_p);
	/** min P frame QP */
	mfc_clear_set_bits(reg, 0xFF, 0, p_264->rc_min_qp_p);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_QP_BOUND_PB);

	reg = MFC_RAW_READL(MFC_REG_E_FIXED_PICTURE_QP);
	mfc_clear_set_bits(reg, 0xFF, 24, p->config_qp);
	mfc_clear_set_bits(reg, 0xFF, 16, p_264->rc_b_frame_qp);
	mfc_clear_set_bits(reg, 0xFF, 8, p_264->rc_p_frame_qp);
	mfc_clear_set_bits(reg, 0xFF, 0, p_264->rc_frame_qp);
	MFC_RAW_WRITEL(reg, MFC_REG_E_FIXED_PICTURE_QP);

	MFC_RAW_WRITEL(0x0, MFC_REG_E_ASPECT_RATIO);
	MFC_RAW_WRITEL(0x0, MFC_REG_E_EXTENDED_SAR);
	if (p_264->ar_vui) {
		/* aspect ration IDC */
		reg = 0;
		mfc_set_bits(reg, 0xFF, 0, p_264->ar_vui_idc);
		MFC_RAW_WRITEL(reg, MFC_REG_E_ASPECT_RATIO);
		if (p_264->ar_vui_idc == 0xFF) {
			/* sample  AR info. */
			reg = 0;
			mfc_set_bits(reg, 0xFFFF, 16, p_264->ext_sar_width);
			mfc_set_bits(reg, 0xFFFF, 0, p_264->ext_sar_height);
			MFC_RAW_WRITEL(reg, MFC_REG_E_EXTENDED_SAR);
		}
	}
	/* intra picture period for H.264 open GOP, value */
	reg = MFC_RAW_READL(MFC_REG_E_H264_REFRESH_PERIOD);
	mfc_clear_bits(reg, 0xFFFF, 0);
	if (p_264->open_gop)
		mfc_set_bits(reg, 0xFFFF, 0, p_264->open_gop_size);
	MFC_RAW_WRITEL(reg, MFC_REG_E_H264_REFRESH_PERIOD);

	/* Temporal SVC */
	__mfc_set_temporal_svc_h264(ctx, p_264);

	/* set frame pack sei generation */
	if (p_264->sei_gen_enable) {
		/* frame packing enable */
		reg = MFC_RAW_READL(MFC_REG_E_H264_OPTIONS);
		mfc_set_bits(reg, 0x1, 25, 0x1);
		MFC_RAW_WRITEL(reg, MFC_REG_E_H264_OPTIONS);

		/* set current frame0 flag & arrangement type */
		reg = 0;
		/** current frame0 flag */
		mfc_set_bits(reg, 0x1, 2, p_264->sei_fp_curr_frame_0);
		/** arrangement type */
		mfc_set_bits(reg, 0x3, 0, (p_264->sei_fp_arrangement_type - 3));
		MFC_RAW_WRITEL(reg, MFC_REG_E_H264_FRAME_PACKING_SEI_INFO);
	}

	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->color_aspect_enc) && p->check_color_range) {
		reg = MFC_RAW_READL(MFC_REG_E_VIDEO_SIGNAL_TYPE);
		/* VIDEO_SIGNAL_TYPE_FLAG */
		mfc_set_bits(reg, 0x1, 31, 0x1);
		/* COLOR_RANGE */
		mfc_clear_set_bits(reg, 0x1, 25, p->color_range);
		if ((p->colour_primaries != 0) && (p->transfer_characteristics != 0) &&
				(p->matrix_coefficients != 3)) {
			/* COLOUR_DESCRIPTION_PRESENT_FLAG */
			mfc_set_bits(reg, 0x1, 24, 0x1);
			/* COLOUR_PRIMARIES */
			mfc_clear_set_bits(reg, 0xFF, 16, p->colour_primaries);
			/* TRANSFER_CHARACTERISTICS */
			mfc_clear_set_bits(reg, 0xFF, 8, p->transfer_characteristics);
			/* MATRIX_COEFFICIENTS */
			mfc_clear_set_bits(reg, 0xFF, 0, p->matrix_coefficients);
		} else {
			mfc_clear_bits(reg, 0x1, 24);
		}
		MFC_RAW_WRITEL(reg, MFC_REG_E_VIDEO_SIGNAL_TYPE);
		mfc_debug(2, "[HDR] H264 ENC Color aspect: range(%s), pri(%d), trans(%d), mat(%d)\n",
				p->color_range ? "Full" : "Limited", p->colour_primaries,
				p->transfer_characteristics, p->matrix_coefficients);
	} else {
		MFC_RAW_WRITEL(0, MFC_REG_E_VIDEO_SIGNAL_TYPE);
	}

	__mfc_set_fmo_slice_map_h264(ctx, p_264);

	mfc_debug_leave();
}

static void __mfc_set_enc_params_mpeg4(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_enc_params *p = &enc->params;
	struct mfc_mpeg4_enc_params *p_mpeg4 = &p->codec.mpeg4;
	unsigned int reg = 0;

	mfc_debug_enter();

	p->rc_framerate_res = FRAME_RATE_RESOLUTION;
	__mfc_set_enc_params(ctx);

	/* set gop_size with I_FRM_CTRL mode */
	__mfc_set_gop_size(ctx, 1);

	/* profile & level */
	reg = 0;
	/** level */
	mfc_set_bits(reg, 0xFF, 8, p_mpeg4->level);
	/** profile - 0 ~ 1 */
	mfc_set_bits(reg, 0x3F, 0, p_mpeg4->profile);
	MFC_RAW_WRITEL(reg, MFC_REG_E_PICTURE_PROFILE);

	/* quarter_pixel */
	/* MFC_RAW_WRITEL(p_mpeg4->quarter_pixel, MFC_REG_ENC_MPEG4_QUART_PXL); */

	/* qp */
	reg = MFC_RAW_READL(MFC_REG_E_FIXED_PICTURE_QP);
	mfc_clear_set_bits(reg, 0xFF, 24, p->config_qp);
	mfc_clear_set_bits(reg, 0xFF, 16, p_mpeg4->rc_b_frame_qp);
	mfc_clear_set_bits(reg, 0xFF, 8, p_mpeg4->rc_p_frame_qp);
	mfc_clear_set_bits(reg, 0xFF, 0, p_mpeg4->rc_frame_qp);
	MFC_RAW_WRITEL(reg, MFC_REG_E_FIXED_PICTURE_QP);

	/* rate control config. */
	reg = MFC_RAW_READL(MFC_REG_E_RC_CONFIG);
	/** frame QP */
	mfc_clear_set_bits(reg, 0xFF, 0, p_mpeg4->rc_frame_qp);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_CONFIG);

	/* max & min value of QP for I frame */
	reg = MFC_RAW_READL(MFC_REG_E_RC_QP_BOUND);
	/** max I frame QP */
	mfc_clear_set_bits(reg, 0xFF, 8, p_mpeg4->rc_max_qp);
	/** min I frame QP */
	mfc_clear_set_bits(reg, 0xFF, 0, p_mpeg4->rc_min_qp);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_QP_BOUND);

	/* max & min value of QP for P/B frame */
	reg = MFC_RAW_READL(MFC_REG_E_RC_QP_BOUND_PB);
	/** max B frame QP */
	mfc_clear_set_bits(reg, 0xFF, 24, p_mpeg4->rc_max_qp_b);
	/** min B frame QP */
	mfc_clear_set_bits(reg, 0xFF, 16, p_mpeg4->rc_min_qp_b);
	/** max P frame QP */
	mfc_clear_set_bits(reg, 0xFF, 8, p_mpeg4->rc_max_qp_p);
	/** min P frame QP */
	mfc_clear_set_bits(reg, 0xFF, 0, p_mpeg4->rc_min_qp_p);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_QP_BOUND_PB);

	/* initialize for '0' only setting*/
	MFC_RAW_WRITEL(0x0, MFC_REG_E_MPEG4_OPTIONS); /* SEQ_start only */
	MFC_RAW_WRITEL(0x0, MFC_REG_E_MPEG4_HEC_PERIOD); /* SEQ_start only */

	mfc_debug_leave();
}

static void __mfc_set_enc_params_h263(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_enc_params *p = &enc->params;
	struct mfc_mpeg4_enc_params *p_mpeg4 = &p->codec.mpeg4;
	unsigned int reg = 0;

	mfc_debug_enter();

	/* For H.263 only 8 bit is used and maximum value can be 0xFF */
	p->rc_framerate_res = 100;
	__mfc_set_enc_params(ctx);

	/* set gop_size with I_FRM_CTRL mode */
	__mfc_set_gop_size(ctx, 1);

	/* profile & level: supports only baseline profile Level 70 */

	/* qp */
	reg = MFC_RAW_READL(MFC_REG_E_FIXED_PICTURE_QP);
	mfc_clear_set_bits(reg, 0xFF, 24, p->config_qp);
	mfc_clear_set_bits(reg, 0xFF, 8, p_mpeg4->rc_p_frame_qp);
	mfc_clear_set_bits(reg, 0xFF, 0, p_mpeg4->rc_frame_qp);
	MFC_RAW_WRITEL(reg, MFC_REG_E_FIXED_PICTURE_QP);

	/* rate control config. */
	reg = MFC_RAW_READL(MFC_REG_E_RC_CONFIG);
	/** frame QP */
	mfc_clear_set_bits(reg, 0xFF, 0, p_mpeg4->rc_frame_qp);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_CONFIG);

	/* max & min value of QP for I frame */
	reg = MFC_RAW_READL(MFC_REG_E_RC_QP_BOUND);
	/** max I frame QP */
	mfc_clear_set_bits(reg, 0xFF, 8, p_mpeg4->rc_max_qp);
	/** min I frame QP */
	mfc_clear_set_bits(reg, 0xFF, 0, p_mpeg4->rc_min_qp);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_QP_BOUND);

	/* max & min value of QP for P/B frame */
	reg = MFC_RAW_READL(MFC_REG_E_RC_QP_BOUND_PB);
	/** max P frame QP */
	mfc_clear_set_bits(reg, 0xFF, 8, p_mpeg4->rc_max_qp_p);
	/** min P frame QP */
	mfc_clear_set_bits(reg, 0xFF, 0, p_mpeg4->rc_min_qp_p);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_QP_BOUND_PB);

	mfc_debug_leave();
}

static void __mfc_set_enc_params_vp8(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_enc_params *p = &enc->params;
	struct mfc_vp8_enc_params *p_vp8 = &p->codec.vp8;
	unsigned int reg = 0;
	int i;

	mfc_debug_enter();

	p->rc_framerate_res = FRAME_RATE_RESOLUTION;
	__mfc_set_enc_params(ctx);

	if (p_vp8->num_hier_layer & 0x3) {
		/* set gop_size without i_frm_ctrl mode */
		__mfc_set_gop_size(ctx, 0);
	} else {
		/* set gop_size with i_frm_ctrl mode */
		__mfc_set_gop_size(ctx, 1);
	}

	/* profile*/
	reg = 0;
	mfc_set_bits(reg, 0xF, 0, p_vp8->vp8_version);
	MFC_RAW_WRITEL(reg, MFC_REG_E_PICTURE_PROFILE);

	reg = MFC_RAW_READL(MFC_REG_E_VP8_OPTION);
	/* if num_refs_for_p is 2, the performance falls by half */
	mfc_clear_set_bits(reg, 0x1, 0, (p->num_refs_for_p - 1));
	/* vp8 partition is possible as below value: 1/2/4/8 */
	if (p_vp8->vp8_numberofpartitions & 0x1) {
		if (p_vp8->vp8_numberofpartitions > 1)
			mfc_err_ctx("partition should be even num (%d)\n",
					p_vp8->vp8_numberofpartitions);
		p_vp8->vp8_numberofpartitions = (p_vp8->vp8_numberofpartitions & ~0x1);
	}
	mfc_clear_set_bits(reg, 0xF, 3, p_vp8->vp8_numberofpartitions);
	mfc_clear_set_bits(reg, 0x1, 10, p_vp8->intra_4x4mode_disable);
	/* Temporal SVC - hier qp enable */
	mfc_clear_set_bits(reg, 0x1, 11, p_vp8->hier_qp_enable);
	/* Disable IVF header */
	mfc_clear_set_bits(reg, 0x1, 12, p->ivf_header_disable);
	MFC_RAW_WRITEL(reg, MFC_REG_E_VP8_OPTION);

	reg = MFC_RAW_READL(MFC_REG_E_VP8_GOLDEN_FRAME_OPTION);
	mfc_clear_set_bits(reg, 0x1, 0, p_vp8->vp8_goldenframesel);
	mfc_clear_set_bits(reg, 0xFFFF, 1, p_vp8->vp8_gfrefreshperiod);
	MFC_RAW_WRITEL(reg, MFC_REG_E_VP8_GOLDEN_FRAME_OPTION);

	/* Temporal SVC - layer number */
	reg = MFC_RAW_READL(MFC_REG_E_NUM_T_LAYER);
	mfc_clear_set_bits(reg, 0x7, 0, p_vp8->num_hier_layer);
	mfc_clear_set_bits(reg, 0x7, 4, 0x3);
	mfc_clear_set_bits(reg, 0x1, 8, p->hier_bitrate_ctrl);
	MFC_RAW_WRITEL(reg, MFC_REG_E_NUM_T_LAYER);
	mfc_debug(3, "[HIERARCHICAL] hier_qp_enable %d, num_hier_layer %d, NUM_T_LAYER 0x%x\n",
			p_vp8->hier_qp_enable, p_vp8->num_hier_layer, reg);

	/* QP & Bitrate for each layer */
	for (i = 0; i < 3; i++) {
		MFC_RAW_WRITEL(p_vp8->hier_qp_layer[i],
				MFC_REG_E_HIERARCHICAL_QP_LAYER0 + i * 4);
		/* If hier_bitrate_ctrl is set to 1, this is meaningless */
		MFC_RAW_WRITEL(p_vp8->hier_bit_layer[i],
				MFC_REG_E_HIERARCHICAL_BIT_RATE_LAYER0 + i * 4);
		mfc_debug(3, "[HIERARCHICAL] layer[%d] QP: %#x, bitrate: %#x(FW ctrl: %d)\n",
					i, p_vp8->hier_qp_layer[i],
					p_vp8->hier_bit_layer[i], p->hier_bitrate_ctrl);
	}

	reg = 0;
	mfc_set_bits(reg, 0x7, 0, p_vp8->vp8_filtersharpness);
	mfc_set_bits(reg, 0x3F, 8, p_vp8->vp8_filterlevel);
	MFC_RAW_WRITEL(reg, MFC_REG_E_VP8_FILTER_OPTION);

	/* qp */
	reg = MFC_RAW_READL(MFC_REG_E_FIXED_PICTURE_QP);
	mfc_clear_set_bits(reg, 0xFF, 24, p->config_qp);
	mfc_clear_set_bits(reg, 0xFF, 8, p_vp8->rc_p_frame_qp);
	mfc_clear_set_bits(reg, 0xFF, 0, p_vp8->rc_frame_qp);
	MFC_RAW_WRITEL(reg, MFC_REG_E_FIXED_PICTURE_QP);

	/* rate control config. */
	reg = MFC_RAW_READL(MFC_REG_E_RC_CONFIG);
	/** frame QP */
	mfc_clear_set_bits(reg, 0xFF, 0, p_vp8->rc_frame_qp);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_CONFIG);

	/* max & min value of QP for I frame */
	reg = MFC_RAW_READL(MFC_REG_E_RC_QP_BOUND);
	/** max I frame QP */
	mfc_clear_set_bits(reg, 0xFF, 8, p_vp8->rc_max_qp);
	/** min I frame QP */
	mfc_clear_set_bits(reg, 0xFF, 0, p_vp8->rc_min_qp);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_QP_BOUND);

	/* max & min value of QP for P/B frame */
	reg = MFC_RAW_READL(MFC_REG_E_RC_QP_BOUND_PB);
	/** max P frame QP */
	mfc_clear_set_bits(reg, 0xFF, 8, p_vp8->rc_max_qp_p);
	/** min P frame QP */
	mfc_clear_set_bits(reg, 0xFF, 0, p_vp8->rc_min_qp_p);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_QP_BOUND_PB);

	mfc_debug_leave();
}

static void __mfc_set_enc_params_vp9(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_enc_params *p = &enc->params;
	struct mfc_vp9_enc_params *p_vp9 = &p->codec.vp9;
	unsigned int reg = 0;
	int i;

	mfc_debug_enter();

	p->rc_framerate_res = FRAME_RATE_RESOLUTION;
	__mfc_set_enc_params(ctx);

	if (p_vp9->num_hier_layer & 0x3) {
		/* set gop_size without i_frm_ctrl mode */
		__mfc_set_gop_size(ctx, 0);
	} else {
		/* set gop_size with i_frm_ctrl mode */
		__mfc_set_gop_size(ctx, 1);
	}

	/* profile*/
	reg = 0;
	mfc_set_bits(reg, 0xF, 0, p_vp9->profile);
	/* level */
	mfc_set_bits(reg, 0xFF, 8, p_vp9->level);
	/* bit depth minus8 */
	if (ctx->is_10bit) {
		mfc_set_bits(reg, 0x7, 17, 0x2);
		mfc_set_bits(reg, 0x7, 20, 0x2);
	}
	MFC_RAW_WRITEL(reg, MFC_REG_E_PICTURE_PROFILE);

	/* for only information about wrong setting */
	if (ctx->is_422) {
		if ((p_vp9->profile != MFC_REG_E_PROFILE_VP9_PROFILE1) &&
			(p_vp9->profile != MFC_REG_E_PROFILE_VP9_PROFILE3)) {
			mfc_err_ctx("4:2:2 format is not matched with profile(%d)\n",
					p_vp9->profile);
		}
	}
	if (ctx->is_10bit) {
		if ((p_vp9->profile != MFC_REG_E_PROFILE_VP9_PROFILE2) &&
			(p_vp9->profile != MFC_REG_E_PROFILE_VP9_PROFILE3)) {
			mfc_err_ctx("[10BIT] format is not matched with profile(%d)\n",
					p_vp9->profile);
		}
	}

	reg = MFC_RAW_READL(MFC_REG_E_VP9_OPTION);
	/* if num_refs_for_p is 2, the performance falls by half */
	mfc_clear_set_bits(reg, 0x1, 0, (p->num_refs_for_p - 1));
	mfc_clear_set_bits(reg, 0x1, 1, p_vp9->intra_pu_split_disable);
	mfc_clear_set_bits(reg, 0x1, 3, p_vp9->max_partition_depth);
	/* Temporal SVC - hier qp enable */
	mfc_clear_set_bits(reg, 0x1, 11, p_vp9->hier_qp_enable);
	/* Disable IVF header */
	mfc_clear_set_bits(reg, 0x1, 12, p->ivf_header_disable);
	MFC_RAW_WRITEL(reg, MFC_REG_E_VP9_OPTION);

	reg = MFC_RAW_READL(MFC_REG_E_VP9_GOLDEN_FRAME_OPTION);
	mfc_clear_set_bits(reg, 0x1, 0, p_vp9->vp9_goldenframesel);
	mfc_clear_set_bits(reg, 0xFFFF, 1, p_vp9->vp9_gfrefreshperiod);
	MFC_RAW_WRITEL(reg, MFC_REG_E_VP9_GOLDEN_FRAME_OPTION);

	/* Temporal SVC - layer number */
	reg = MFC_RAW_READL(MFC_REG_E_NUM_T_LAYER);
	mfc_clear_set_bits(reg, 0x7, 0, p_vp9->num_hier_layer);
	mfc_clear_set_bits(reg, 0x7, 4, 0x3);
	mfc_clear_set_bits(reg, 0x1, 8, p->hier_bitrate_ctrl);
	MFC_RAW_WRITEL(reg, MFC_REG_E_NUM_T_LAYER);
	mfc_debug(3, "[HIERARCHICAL] hier_qp_enable %d, num_hier_layer %d, NUM_T_LAYER 0x%x\n",
			p_vp9->hier_qp_enable, p_vp9->num_hier_layer, reg);

	/* QP & Bitrate for each layer */
	for (i = 0; i < 3; i++) {
		MFC_RAW_WRITEL(p_vp9->hier_qp_layer[i],
				MFC_REG_E_HIERARCHICAL_QP_LAYER0 + i * 4);
		/* If hier_bitrate_ctrl is set to 1, this is meaningless */
		MFC_RAW_WRITEL(p_vp9->hier_bit_layer[i],
				MFC_REG_E_HIERARCHICAL_BIT_RATE_LAYER0 + i * 4);
		mfc_debug(3, "[HIERARCHICAL] layer[%d] QP: %#x, bitrate: %#x (FW ctrl: %d)\n",
					i, p_vp9->hier_qp_layer[i],
					p_vp9->hier_bit_layer[i], p->hier_bitrate_ctrl);
	}

	/* qp */
	reg = MFC_RAW_READL(MFC_REG_E_FIXED_PICTURE_QP);
	mfc_clear_set_bits(reg, 0xFF, 24, p->config_qp);
	mfc_clear_set_bits(reg, 0xFF, 8, p_vp9->rc_p_frame_qp);
	mfc_clear_set_bits(reg, 0xFF, 0, p_vp9->rc_frame_qp);
	MFC_RAW_WRITEL(reg, MFC_REG_E_FIXED_PICTURE_QP);

	/* rate control config. */
	reg = MFC_RAW_READL(MFC_REG_E_RC_CONFIG);
	/** frame QP */
	mfc_clear_set_bits(reg, 0xFF, 0, p_vp9->rc_frame_qp);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_CONFIG);

	/* max & min value of QP for I frame */
	reg = MFC_RAW_READL(MFC_REG_E_RC_QP_BOUND);
	/** max I frame QP */
	mfc_clear_set_bits(reg, 0xFF, 8, p_vp9->rc_max_qp);
	/** min I frame QP */
	mfc_clear_set_bits(reg, 0xFF, 0, p_vp9->rc_min_qp);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_QP_BOUND);

	/* max & min value of QP for P/B frame */
	reg = MFC_RAW_READL(MFC_REG_E_RC_QP_BOUND_PB);
	/** max P frame QP */
	mfc_clear_set_bits(reg, 0xFF, 8, p_vp9->rc_max_qp_p);
	/** min P frame QP */
	mfc_clear_set_bits(reg, 0xFF, 0, p_vp9->rc_min_qp_p);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_QP_BOUND_PB);

	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->color_aspect_enc) && p->check_color_range) {
		reg = MFC_RAW_READL(MFC_REG_E_VIDEO_SIGNAL_TYPE);
		/* VIDEO_SIGNAL_TYPE_FLAG */
		mfc_clear_set_bits(reg, 0x1, 31, 0x1);
		/* COLOR_SPACE: VP9 uses colour_primaries interface for color space */
		mfc_clear_set_bits(reg, 0x1F, 26, p->colour_primaries);
		/* COLOR_RANGE */
		mfc_clear_set_bits(reg, 0x1, 25, p->color_range);
		MFC_RAW_WRITEL(reg, MFC_REG_E_VIDEO_SIGNAL_TYPE);
		mfc_debug(2, "[HDR] VP9 ENC Color aspect: range(%s), space(%d)\n",
				p->color_range ? "Full" : "Limited", p->colour_primaries);
	} else {
		MFC_RAW_WRITEL(0, MFC_REG_E_VIDEO_SIGNAL_TYPE);
	}

	mfc_debug_leave();
}

static void __mfc_set_enc_params_hevc(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_enc_params *p = &enc->params;
	struct mfc_hevc_enc_params *p_hevc = &p->codec.hevc;
	unsigned int reg = 0;
	int i;

	mfc_debug_enter();

	p->rc_framerate_res = FRAME_RATE_RESOLUTION;
	__mfc_set_enc_params(ctx);

	if (p_hevc->num_hier_layer & 0x7) {
		/* set gop_size without i_frm_ctrl mode */
		__mfc_set_gop_size(ctx, 0);
	} else {
		/* set gop_size with i_frm_ctrl mode */
		__mfc_set_gop_size(ctx, 1);
	}

	/* UHD encoding case */
	if (IS_UHD_RES(ctx)) {
		p_hevc->level = 51;
		p_hevc->tier_flag = 0;
	/* this tier_flag can be changed */
	}

	/* tier_flag & level & profile */
	reg = 0;
	/* profile */
	mfc_set_bits(reg, 0xF, 0, p_hevc->profile);
	/* level */
	mfc_set_bits(reg, 0xFF, 8, p_hevc->level);
	/* tier_flag - 0 ~ 1 */
	mfc_set_bits(reg, 0x1, 16, p_hevc->tier_flag);
	/* bit depth minus8 */
	if (ctx->is_10bit) {
		mfc_set_bits(reg, 0x7, 17, 0x2);
		mfc_set_bits(reg, 0x7, 20, 0x2);
	}
	MFC_RAW_WRITEL(reg, MFC_REG_E_PICTURE_PROFILE);

	/* for only information about wrong setting */
	if (ctx->is_422) {
		if ((p_hevc->profile != MFC_REG_E_PROFILE_HEVC_MAIN_422_10_INTRA) &&
			(p_hevc->profile != MFC_REG_E_PROFILE_HEVC_MAIN_422_10)) {
			mfc_err_ctx("4:2:2 format is not matched with profile(%d)\n",
					p_hevc->profile);
		}
	}
	if (ctx->is_10bit) {
		if ((p_hevc->profile != MFC_REG_E_PROFILE_HEVC_MAIN_422_10_INTRA) &&
			(p_hevc->profile != MFC_REG_E_PROFILE_HEVC_MAIN_10) &&
			(p_hevc->profile != MFC_REG_E_PROFILE_HEVC_MAIN_422_10)) {
			mfc_err_ctx("[10BIT] format is not matched with profile(%d)\n",
					p_hevc->profile);
		}
	}

	/* max partition depth */
	reg = MFC_RAW_READL(MFC_REG_E_HEVC_OPTIONS);
	mfc_clear_set_bits(reg, 0x3, 0, p_hevc->max_partition_depth);
	/* if num_refs_for_p is 2, the performance falls by half */
	mfc_clear_set_bits(reg, 0x1, 2, (p->num_refs_for_p - 1));
	mfc_clear_set_bits(reg, 0x3, 3, p_hevc->refreshtype);
	mfc_clear_set_bits(reg, 0x1, 5, p_hevc->const_intra_period_enable);
	mfc_clear_set_bits(reg, 0x1, 6, p_hevc->lossless_cu_enable);
	mfc_clear_set_bits(reg, 0x1, 7, p_hevc->wavefront_enable);
	mfc_clear_set_bits(reg, 0x1, 8, p_hevc->loopfilter_disable);
	mfc_clear_set_bits(reg, 0x1, 9, p_hevc->loopfilter_across);
	mfc_clear_set_bits(reg, 0x1, 10, p_hevc->enable_ltr);
	mfc_clear_set_bits(reg, 0x1, 11, p_hevc->hier_qp_enable);
	mfc_clear_set_bits(reg, 0x1, 13, p_hevc->general_pb_enable);
	mfc_clear_set_bits(reg, 0x1, 14, p_hevc->temporal_id_enable);
	mfc_clear_set_bits(reg, 0x1, 15, p_hevc->strong_intra_smooth);
	mfc_clear_set_bits(reg, 0x1, 16, p_hevc->intra_pu_split_disable);
	mfc_clear_set_bits(reg, 0x1, 17, p_hevc->tmv_prediction_disable);
	mfc_clear_set_bits(reg, 0x7, 18, p_hevc->max_num_merge_mv);
	mfc_clear_set_bits(reg, 0x1, 23, p_hevc->encoding_nostartcode_enable);
	mfc_clear_set_bits(reg, 0x1, 26, p_hevc->prepend_sps_pps_to_idr);
	/* enable sps pps control in OTF scenario */
	if (ctx->otf_handle) {
		mfc_set_bits(reg, 0x1, 26, 0x1);
		mfc_debug(2, "[OTF] SPS_PPS_CONTROL enabled\n");
	}
	/* Weighted Prediction enable */
	mfc_clear_set_bits(reg, 0x1, 28, p->weighted_enable);
	/* 30bit is 32x32 transform. If it is enabled, the performance falls by half */
	mfc_clear_bits(reg, 0x1, 30);
	MFC_RAW_WRITEL(reg, MFC_REG_E_HEVC_OPTIONS);
	/* refresh period */
	reg = MFC_RAW_READL(MFC_REG_E_HEVC_REFRESH_PERIOD);
	mfc_clear_set_bits(reg, 0xFFFF, 0, p_hevc->refreshperiod);
	MFC_RAW_WRITEL(reg, MFC_REG_E_HEVC_REFRESH_PERIOD);
	/* loop filter setting */
	MFC_RAW_WRITEL(0, MFC_REG_E_HEVC_LF_BETA_OFFSET_DIV2);
	MFC_RAW_WRITEL(0, MFC_REG_E_HEVC_LF_TC_OFFSET_DIV2);
	if (!p_hevc->loopfilter_disable) {
		MFC_RAW_WRITEL(p_hevc->lf_beta_offset_div2, MFC_REG_E_HEVC_LF_BETA_OFFSET_DIV2);
		MFC_RAW_WRITEL(p_hevc->lf_tc_offset_div2, MFC_REG_E_HEVC_LF_TC_OFFSET_DIV2);
	}
	/* long term reference */
	if (p_hevc->enable_ltr) {
		reg = 0;
		mfc_set_bits(reg, 0x3, 0, p_hevc->store_ref);
		mfc_set_bits(reg, 0x3, 2, p_hevc->user_ref);
		MFC_RAW_WRITEL(reg, MFC_REG_E_HEVC_NAL_CONTROL);
	}

	/* Temporal SVC - qp type, layer number */
	reg = MFC_RAW_READL(MFC_REG_E_NUM_T_LAYER);
	mfc_clear_set_bits(reg, 0x1, 3, p_hevc->hier_qp_type);
	mfc_clear_set_bits(reg, 0x7, 0, p_hevc->num_hier_layer);
	mfc_clear_bits(reg, 0x7, 4);
	if (p_hevc->hier_ref_type) {
		mfc_set_bits(reg, 0x1, 7, 0x1);
		mfc_set_bits(reg, 0x7, 4, p->num_hier_max_layer);
	} else {
		mfc_clear_bits(reg, 0x1, 7);
		mfc_set_bits(reg, 0x7, 4, 0x7);
	}
	mfc_clear_set_bits(reg, 0x1, 8, p->hier_bitrate_ctrl);
	MFC_RAW_WRITEL(reg, MFC_REG_E_NUM_T_LAYER);
	mfc_debug(3, "[HIERARCHICAL] hier_qp_enable %d, enable_ltr %d, "
		"num_hier_layer %d, max_layer %d, hier_ref_type %d, NUM_T_LAYER 0x%x\n",
		p_hevc->hier_qp_enable, p_hevc->enable_ltr, p_hevc->num_hier_layer,
		p->num_hier_max_layer, p_hevc->hier_ref_type, reg);

	/* QP & Bitrate for each layer */
	for (i = 0; i < 7; i++) {
		MFC_RAW_WRITEL(p_hevc->hier_qp_layer[i],
			MFC_REG_E_HIERARCHICAL_QP_LAYER0 + i * 4);
		MFC_RAW_WRITEL(p_hevc->hier_bit_layer[i],
			MFC_REG_E_HIERARCHICAL_BIT_RATE_LAYER0 + i * 4);
		mfc_debug(3, "[HIERARCHICAL] layer[%d] QP: %#x, bitrate: %d(FW ctrl: %d)\n",
					i, p_hevc->hier_qp_layer[i],
					p_hevc->hier_bit_layer[i], p->hier_bitrate_ctrl);
	}

	/* rate control config. */
	reg = MFC_RAW_READL(MFC_REG_E_RC_CONFIG);
	/** frame QP */
	mfc_clear_set_bits(reg, 0xFF, 0, p_hevc->rc_frame_qp);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_CONFIG);

	/* max & min value of QP for I frame */
	reg = MFC_RAW_READL(MFC_REG_E_RC_QP_BOUND);
	/** max I frame QP */
	mfc_clear_set_bits(reg, 0xFF, 8, p_hevc->rc_max_qp);
	/** min I frame QP */
	mfc_clear_set_bits(reg, 0xFF, 0, p_hevc->rc_min_qp);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_QP_BOUND);

	/* max & min value of QP for P/B frame */
	reg = MFC_RAW_READL(MFC_REG_E_RC_QP_BOUND_PB);
	/** max B frame QP */
	mfc_clear_set_bits(reg, 0xFF, 24, p_hevc->rc_max_qp_b);
	/** min B frame QP */
	mfc_clear_set_bits(reg, 0xFF, 16, p_hevc->rc_min_qp_b);
	/** max P frame QP */
	mfc_clear_set_bits(reg, 0xFF, 8, p_hevc->rc_max_qp_p);
	/** min P frame QP */
	mfc_clear_set_bits(reg, 0xFF, 0, p_hevc->rc_min_qp_p);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_QP_BOUND_PB);

	reg = MFC_RAW_READL(MFC_REG_E_FIXED_PICTURE_QP);
	mfc_clear_set_bits(reg, 0xFF, 24, p->config_qp);
	mfc_clear_set_bits(reg, 0xFF, 16, p_hevc->rc_b_frame_qp);
	mfc_clear_set_bits(reg, 0xFF, 8, p_hevc->rc_p_frame_qp);
	mfc_clear_set_bits(reg, 0xFF, 0, p_hevc->rc_frame_qp);
	MFC_RAW_WRITEL(reg, MFC_REG_E_FIXED_PICTURE_QP);

	/* ROI enable: it must set on SEQ_START only for HEVC encoder */
	reg = MFC_RAW_READL(MFC_REG_E_RC_ROI_CTRL);
	mfc_clear_set_bits(reg, 0x1, 0, p->roi_enable);
	MFC_RAW_WRITEL(reg, MFC_REG_E_RC_ROI_CTRL);
	mfc_debug(3, "[ROI] HEVC ROI enable\n");

	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->color_aspect_enc) && p->check_color_range) {
		reg = MFC_RAW_READL(MFC_REG_E_VIDEO_SIGNAL_TYPE);
		/* VIDEO_SIGNAL_TYPE_FLAG */
		mfc_clear_set_bits(reg, 0x1, 31, 0x1);
		/* COLOR_RANGE */
		mfc_clear_set_bits(reg, 0x1, 25, p->color_range);
		if ((p->colour_primaries != 0) && (p->transfer_characteristics != 0) &&
				(p->matrix_coefficients != 3)) {
			/* COLOUR_DESCRIPTION_PRESENT_FLAG */
			mfc_clear_set_bits(reg, 0x1, 24, 0x1);
			/* COLOUR_PRIMARIES */
			mfc_clear_set_bits(reg, 0xFF, 16, p->colour_primaries);
			/* TRANSFER_CHARACTERISTICS */
			mfc_clear_set_bits(reg, 0xFF, 8, p->transfer_characteristics);
			/* MATRIX_COEFFICIENTS */
			mfc_clear_set_bits(reg, 0xFF, 0, p->matrix_coefficients);
		} else {
			mfc_clear_bits(reg, 0x1, 24);
		}
		MFC_RAW_WRITEL(reg, MFC_REG_E_VIDEO_SIGNAL_TYPE);
		mfc_debug(2, "[HDR] HEVC ENC Color aspect: range(%s), pri(%d), trans(%d), mat(%d)\n",
				p->color_range ? "Full" : "Limited", p->colour_primaries,
				p->transfer_characteristics, p->matrix_coefficients);
	} else {
		MFC_RAW_WRITEL(0, MFC_REG_E_VIDEO_SIGNAL_TYPE);
	}

	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->static_info_enc) &&
			p->static_info_enable && ctx->is_10bit) {
		reg = MFC_RAW_READL(MFC_REG_E_HEVC_OPTIONS_2);
		/* HDR_STATIC_INFO_ENABLE */
		mfc_clear_set_bits(reg, 0x1, 0, p->static_info_enable);
		MFC_RAW_WRITEL(reg, MFC_REG_E_HEVC_OPTIONS_2);
		/* MAX_PIC_AVERAGE_LIGHT & MAX_CONTENT_LIGHT */
		mfc_clear_set_bits(reg, 0xFFFF, 0, p->max_pic_average_light);
		mfc_clear_set_bits(reg, 0xFFFF, 16, p->max_content_light);
		MFC_RAW_WRITEL(reg, MFC_REG_E_CONTENT_LIGHT_LEVEL_INFO_SEI);
		/* MAX_DISPLAY_LUMINANCE */
		MFC_RAW_WRITEL(p->max_display_luminance, MFC_REG_E_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_0);
		/* MIN DISPLAY LUMINANCE */
		MFC_RAW_WRITEL(p->min_display_luminance, MFC_REG_E_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_1);
		/* WHITE_POINT */
		MFC_RAW_WRITEL(p->white_point, MFC_REG_E_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_2);
		/* DISPLAY PRIMARIES_0 */
		MFC_RAW_WRITEL(p->display_primaries_0, MFC_REG_E_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_3);
		/* DISPLAY PRIMARIES_1 */
		MFC_RAW_WRITEL(p->display_primaries_1, MFC_REG_E_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_4);
		/* DISPLAY PRIMARIES_2 */
		MFC_RAW_WRITEL(p->display_primaries_2, MFC_REG_E_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_5);

		mfc_debug(2, "[HDR] HEVC ENC static info: enable(%d), max_pic(0x%x), max_content(0x%x)\n",
				p->static_info_enable, p->max_pic_average_light, p->max_content_light);
		mfc_debug(2, "[HDR] max_disp(0x%x), min_disp(0x%x), white_point(0x%x)\n",
				p->max_display_luminance, p->min_display_luminance, p->white_point);
		mfc_debug(2, "[HDR] disp_pri_0(0x%x), disp_pri_1(0x%x), disp_pri_2(0x%x)\n",
				p->display_primaries_0, p->display_primaries_1, p->display_primaries_2);
	} else {
		reg = MFC_RAW_READL(MFC_REG_E_HEVC_OPTIONS_2);
		/* HDR_STATIC_INFO_ENABLE */
		mfc_clear_bits(reg, 0x1, 0);
		MFC_RAW_WRITEL(reg, MFC_REG_E_HEVC_OPTIONS_2);
	}

	mfc_debug_leave();
}

static void __mfc_set_enc_params_bpg(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_enc_params *p = &enc->params;
	struct mfc_bpg_enc_params *p_bpg = &p->codec.bpg;
	unsigned int reg = 0;

	p->rc_framerate_res = FRAME_RATE_RESOLUTION;
	__mfc_set_enc_params(ctx);

	/* extension tag */
	reg = p_bpg->thumb_size + p_bpg->exif_size;
	MFC_RAW_WRITEL(reg, MFC_REG_E_BPG_EXTENSION_DATA_SIZE);
	mfc_debug(3, "main image extension size %d (thumbnail: %d, exif: %d)\n",
			reg, p_bpg->thumb_size, p_bpg->exif_size);

	/* profile & level */
	reg = 0;
	/* bit depth minus8 */
	if (ctx->is_10bit) {
		mfc_set_bits(reg, 0x7, 17, 0x2);
		mfc_set_bits(reg, 0x7, 20, 0x2);
		/* fixed profile */
		if (ctx->is_422)
			mfc_set_bits(reg, 0x1, 0, 0x1);
	}
	MFC_RAW_WRITEL(reg, MFC_REG_E_PICTURE_PROFILE);
}

int mfc_set_enc_params(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;

	if (IS_H264_ENC(ctx))
		__mfc_set_enc_params_h264(ctx);
	else if (IS_MPEG4_ENC(ctx))
		__mfc_set_enc_params_mpeg4(ctx);
	else if (IS_H263_ENC(ctx))
		__mfc_set_enc_params_h263(ctx);
	else if (IS_VP8_ENC(ctx))
		__mfc_set_enc_params_vp8(ctx);
	else if (IS_VP9_ENC(ctx))
		__mfc_set_enc_params_vp9(ctx);
	else if (IS_HEVC_ENC(ctx))
		__mfc_set_enc_params_hevc(ctx);
	else if (IS_BPG_ENC(ctx))
		__mfc_set_enc_params_bpg(ctx);
	else {
		mfc_err_ctx("Unknown codec for encoding (%x)\n",
			ctx->codec_mode);
		return -EINVAL;
	}

	mfc_debug(5, "RC) Bitrate: %d / framerate: %#x / config %#x / mode %#x\n",
			MFC_RAW_READL(MFC_REG_E_RC_BIT_RATE),
			MFC_RAW_READL(MFC_REG_E_RC_FRAME_RATE),
			MFC_RAW_READL(MFC_REG_E_RC_CONFIG),
			MFC_RAW_READL(MFC_REG_E_RC_MODE));

	return 0;
}

void mfc_set_test_params(struct mfc_dev *dev)
{

	unsigned int base_addr = 0xF000;
	unsigned int i;

	if (!dev->reg_val) {
		mfc_err_dev("[REGTEST] There is no reg_val set the register value\n");
		return;
	}

	mfc_info_dev("[REGTEST] Overwrite register value for encoder register test\n");

	for (i = 0; i < dev->reg_cnt; i++)
		if (((base_addr + (i * 4)) != MFC_REG_E_STREAM_BUFFER_ADDR) &&
				((base_addr + (i * 4)) != MFC_REG_E_SOURCE_FIRST_STRIDE) &&
				((base_addr + (i * 4)) != MFC_REG_E_SOURCE_SECOND_STRIDE) &&
				((base_addr + (i * 4)) != MFC_REG_E_SOURCE_THIRD_STRIDE) &&
				((base_addr + (i * 4)) != MFC_REG_PIXEL_FORMAT))
			MFC_RAW_WRITEL(dev->reg_val[i], base_addr + (i * 4));

}
