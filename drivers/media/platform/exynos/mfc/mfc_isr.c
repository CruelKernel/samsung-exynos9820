/*
 * drivers/media/platform/exynos/mfc/mfc_isr.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "mfc_isr.h"

#include "mfc_hwlock.h"
#include "mfc_nal_q.h"
#include "mfc_otf.h"
#include "mfc_sync.h"

#include "mfc_pm.h"
#include "mfc_perf_measure.h"
#include "mfc_reg_api.h"
#include "mfc_hw_reg_api.h"
#include "mfc_mmcache.h"

#include "mfc_qos.h"
#include "mfc_queue.h"
#include "mfc_buf.h"
#include "mfc_mem.h"

static void __mfc_handle_black_bar_info(struct mfc_dev *dev, struct mfc_ctx *ctx)
{
	struct v4l2_rect new_black_bar;
	int black_bar_info;
	struct mfc_dec *dec = ctx->dec_priv;

	black_bar_info = mfc_get_black_bar_detection();
	mfc_debug(3, "[BLACKBAR] type: %#x\n", black_bar_info);

	if (black_bar_info == MFC_REG_DISP_STATUS_BLACK_BAR) {
		new_black_bar.left = mfc_get_black_bar_pos_x();
		new_black_bar.top = mfc_get_black_bar_pos_y();
		new_black_bar.width = mfc_get_black_bar_image_w();
		new_black_bar.height = mfc_get_black_bar_image_h();
	} else if (black_bar_info == MFC_REG_DISP_STATUS_BLACK_SCREEN) {
		new_black_bar.left = -1;
		new_black_bar.top = -1;
		new_black_bar.width = ctx->img_width;
		new_black_bar.height = ctx->img_height;
	} else if (black_bar_info == MFC_REG_DISP_STATUS_NOT_DETECTED) {
		new_black_bar.left = 0;
		new_black_bar.top = 0;
		new_black_bar.width = ctx->img_width;
		new_black_bar.height = ctx->img_height;
	} else {
		mfc_err_ctx("[BLACKBAR] Not supported type: %#x\n", black_bar_info);
		dec->black_bar_updated = 0;
		return;
	}

	if ((new_black_bar.left == dec->black_bar.left) &&
			(new_black_bar.top == dec->black_bar.top) &&
			(new_black_bar.width == dec->black_bar.width) &&
			(new_black_bar.height == dec->black_bar.height)) {
		mfc_debug(4, "[BLACKBAR] information was not changed\n");
		dec->black_bar_updated = 0;
		return;
	}

	dec->black_bar = new_black_bar;
	dec->black_bar_updated = 1;
}

static unsigned int __mfc_handle_frame_field(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	unsigned int interlace_type = 0, is_interlace = 0, is_mbaff = 0;
	unsigned int field;

	if (CODEC_INTERLACED(ctx))
		is_interlace = mfc_is_interlace_picture();

	if (CODEC_MBAFF(ctx))
		is_mbaff = mfc_is_mbaff_picture();

	if (is_interlace) {
		interlace_type = mfc_get_interlace_type();
		if (interlace_type)
			field = V4L2_FIELD_INTERLACED_TB;
		else
			field = V4L2_FIELD_INTERLACED_BT;
	} else if (is_mbaff) {
		field = V4L2_FIELD_INTERLACED_TB;
	} else {
		field = V4L2_FIELD_NONE;
	}

	mfc_debug(2, "[INTERLACE] is_interlace: %d (type : %d), is_mbaff: %d, field: 0x%#x\n",
			is_interlace, interlace_type, is_mbaff, field);

	return field;
}

static void __mfc_handle_frame_all_extracted(struct mfc_ctx *ctx)
{
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_buf *dst_mb;
	int index, i, is_first = 1;

	mfc_debug(2, "Decided to finish\n");
	ctx->sequence++;

	while (1) {
		dst_mb = mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->dst_buf_queue, MFC_BUF_NO_TOUCH_USED);
		if (!dst_mb)
			break;

		mfc_debug(2, "Cleaning up buffer: %d\n",
					  dst_mb->vb.vb2_buf.index);

		index = dst_mb->vb.vb2_buf.index;

		for (i = 0; i < ctx->dst_fmt->mem_planes; i++)
			vb2_set_plane_payload(&dst_mb->vb.vb2_buf, i, 0);

		dst_mb->vb.sequence = (ctx->sequence++);
		dst_mb->vb.field = __mfc_handle_frame_field(ctx);
		mfc_clear_vb_flag(dst_mb);

		clear_bit(dst_mb->vb.vb2_buf.index, &dec->available_dpb);

		if (call_cop(ctx, get_buf_ctrls_val, ctx, &ctx->dst_ctrls[index]) < 0)
			mfc_err_ctx("failed in get_buf_ctrls_val\n");

		if (is_first) {
			call_cop(ctx, get_buf_update_val, ctx,
				&ctx->dst_ctrls[index],
				V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG,
				dec->stored_tag);
			is_first = 0;
		} else {
			call_cop(ctx, get_buf_update_val, ctx,
				&ctx->dst_ctrls[index],
				V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG,
				DEFAULT_TAG);
			call_cop(ctx, get_buf_update_val, ctx,
				&ctx->dst_ctrls[index],
				V4L2_CID_MPEG_VIDEO_H264_SEI_FP_AVAIL,
				0);
		}

		vb2_buffer_done(&dst_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);

		/* decoder dst buffer CFW UNPROT */
		if (ctx->is_drm)
			mfc_raw_unprotect(ctx, dst_mb, index);

		mfc_debug(2, "Cleaned up buffer: %d\n", index);
	}

	mfc_handle_force_change_status(ctx);
	mfc_debug(2, "After cleanup\n");
}

static void __mfc_handle_frame_copy_timestamp(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_buf *ref_mb, *src_mb;
	dma_addr_t dec_y_addr;

	dec_y_addr = (dma_addr_t)mfc_get_dec_y_addr();

	/* Get the source buffer */
	src_mb = mfc_get_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_NO_TOUCH_USED);
	if (!src_mb) {
		mfc_err_dev("[TS] no src buffers\n");
		return;
	}

	ref_mb = mfc_find_buf(&ctx->buf_queue_lock, &ctx->ref_buf_queue, dec_y_addr);
	if (ref_mb)
		ref_mb->vb.vb2_buf.timestamp = src_mb->vb.vb2_buf.timestamp;
}

static void __mfc_handle_frame_output_move(struct mfc_ctx *ctx,
		dma_addr_t dspl_y_addr, unsigned int released_flag)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_buf *ref_mb;
	int index;

	ref_mb = mfc_find_move_buf(&ctx->buf_queue_lock,
			&ctx->dst_buf_queue, &ctx->ref_buf_queue, dspl_y_addr, released_flag);
	if (ref_mb) {
		index = ref_mb->vb.vb2_buf.index;

		/* Check if this is the buffer we're looking for */
		mfc_debug(2, "[DPB] Found buf[%d] 0x%08llx, looking for disp addr 0x%08llx\n",
				index, ref_mb->addr[0][0], dspl_y_addr);

		if (released_flag & (1 << index)) {
			dec->available_dpb &= ~(1 << index);
			released_flag &= ~(1 << index);
			mfc_debug(2, "[DPB] Corrupted frame(%d), it will be re-used(release)\n",
					mfc_get_warn(mfc_get_int_err()));
		} else {
			dec->err_reuse_flag |= 1 << index;
			mfc_debug(2, "[DPB] Corrupted frame(%d), it will be re-used(not released)\n",
					mfc_get_warn(mfc_get_int_err()));
		}
		dec->dynamic_used |= released_flag;
	}
}

static void __mfc_handle_frame_output_del(struct mfc_ctx *ctx,
		unsigned int err, unsigned int released_flag)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_raw_info *raw = &ctx->raw_buf;
	struct mfc_buf *ref_mb;
	dma_addr_t dspl_y_addr;
	unsigned int frame_type;
	unsigned int dst_frame_status;
	unsigned int is_video_signal_type = 0, is_colour_description = 0;
	unsigned int is_content_light = 0, is_display_colour = 0;
	unsigned int is_hdr10_plus_sei = 0;
	unsigned int i, index;

	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->color_aspect_dec)) {
		is_video_signal_type = mfc_get_video_signal_type();
		is_colour_description = mfc_get_colour_description();
	}

	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->static_info_dec)) {
		is_content_light = mfc_get_sei_avail_content_light();
		is_display_colour = mfc_get_sei_avail_mastering_display();
	}

	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->hdr10_plus))
		is_hdr10_plus_sei = mfc_get_sei_avail_st_2094_40();

	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->black_bar) && dec->detect_black_bar)
		__mfc_handle_black_bar_info(dev, ctx);
	else
		dec->black_bar_updated = 0;

	if (dec->immediate_display == 1) {
		dspl_y_addr = (dma_addr_t)mfc_get_dec_y_addr();
		frame_type = mfc_get_dec_frame_type();
	} else {
		dspl_y_addr = (dma_addr_t)mfc_get_disp_y_addr();
		frame_type = mfc_get_disp_frame_type();
	}

	ref_mb = mfc_find_del_buf(&ctx->buf_queue_lock,
			&ctx->ref_buf_queue, dspl_y_addr);
	if (ref_mb) {
		index = ref_mb->vb.vb2_buf.index;
		/* Check if this is the buffer we're looking for */
		mfc_debug(2, "[BUFINFO][DPB] ctx[%d] get dst index: %d, addr[0]: 0x%08llx\n",
				ctx->num, index, ref_mb->addr[0][0]);

		ref_mb->vb.sequence = ctx->sequence;
		ref_mb->vb.field = __mfc_handle_frame_field(ctx);

		/* Set reserved2 bits in order to inform SEI information */
		mfc_clear_vb_flag(ref_mb);

		if (is_content_light) {
			mfc_set_vb_flag(ref_mb, MFC_FLAG_HDR_CONTENT_LIGHT);
			mfc_debug(2, "[HDR] content light level parsed\n");
		}

		if (is_display_colour) {
			mfc_set_vb_flag(ref_mb, MFC_FLAG_HDR_DISPLAY_COLOUR);
			mfc_debug(2, "[HDR] mastering display colour parsed\n");
		}

		if (is_video_signal_type) {
			mfc_set_vb_flag(ref_mb, MFC_FLAG_HDR_VIDEO_SIGNAL_TYPE);
			mfc_debug(2, "[HDR] video signal type parsed\n");
			if (is_colour_description) {
				mfc_set_vb_flag(ref_mb, MFC_FLAG_HDR_MAXTIX_COEFF);
				mfc_debug(2, "[HDR] matrix coefficients parsed\n");
				mfc_set_vb_flag(ref_mb, MFC_FLAG_HDR_COLOUR_DESC);
				mfc_debug(2, "[HDR] colour description parsed\n");
			}
		}

		if (IS_VP9_DEC(ctx) && MFC_FEATURE_SUPPORT(dev, dev->pdata->color_aspect_dec)) {
			if (dec->color_space != MFC_REG_D_COLOR_UNKNOWN) {
				mfc_set_vb_flag(ref_mb, MFC_FLAG_HDR_COLOUR_DESC);
				mfc_debug(2, "[HDR] color space parsed\n");
			}
			mfc_set_vb_flag(ref_mb, MFC_FLAG_HDR_VIDEO_SIGNAL_TYPE);
			mfc_debug(2, "[HDR] color range parsed\n");
		}

		if (dec->black_bar_updated) {
			mfc_set_vb_flag(ref_mb, MFC_FLAG_BLACKBAR_DETECT);
			mfc_debug(3, "[BLACKBAR] black bar detected\n");
		}

		if (is_hdr10_plus_sei) {
			mfc_get_hdr_plus_info(ctx, &dec->hdr10_plus_info[index]);
			mfc_set_vb_flag(ref_mb, MFC_FLAG_HDR_PLUS);
			mfc_debug(2, "[HDR+] HDR10 plus dyanmic SEI metadata parsed\n");
		} else {
			dec->hdr10_plus_info[index].valid = 0;
		}

		if (ctx->src_fmt->mem_planes == 1) {
			vb2_set_plane_payload(&ref_mb->vb.vb2_buf, 0,
					raw->total_plane_size);
			mfc_debug(5, "single plane payload: %d\n",
					raw->total_plane_size);
		} else {
			for (i = 0; i < ctx->src_fmt->mem_planes; i++) {
				vb2_set_plane_payload(&ref_mb->vb.vb2_buf, i,
						raw->plane_size[i]);
			}
		}

		clear_bit(index, &dec->available_dpb);

		ref_mb->vb.flags &= ~(V4L2_BUF_FLAG_KEYFRAME |
					V4L2_BUF_FLAG_PFRAME |
					V4L2_BUF_FLAG_BFRAME |
					V4L2_BUF_FLAG_ERROR);

		switch (frame_type) {
			case MFC_REG_DISPLAY_FRAME_I:
				ref_mb->vb.flags |= V4L2_BUF_FLAG_KEYFRAME;
				break;
			case MFC_REG_DISPLAY_FRAME_P:
				ref_mb->vb.flags |= V4L2_BUF_FLAG_PFRAME;
				break;
			case MFC_REG_DISPLAY_FRAME_B:
				ref_mb->vb.flags |= V4L2_BUF_FLAG_BFRAME;
				break;
			default:
				break;
		}

		if (mfc_get_warn(err)) {
			mfc_err_ctx("Warning for displayed frame: %d\n",
					mfc_get_warn(err));
			ref_mb->vb.flags |= V4L2_BUF_FLAG_ERROR;
		}

		if (call_cop(ctx, get_buf_ctrls_val, ctx, &ctx->dst_ctrls[index]) < 0)
			mfc_err_ctx("failed in get_buf_ctrls_val\n");

		mfc_handle_released_info(ctx, released_flag, index);

		if (dec->immediate_display == 1) {
			dst_frame_status = mfc_get_dec_status();

			call_cop(ctx, get_buf_update_val, ctx,
					&ctx->dst_ctrls[index],
					V4L2_CID_MPEG_MFC51_VIDEO_DISPLAY_STATUS,
					dst_frame_status);

			call_cop(ctx, get_buf_update_val, ctx,
					&ctx->dst_ctrls[index],
					V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG,
					dec->stored_tag);

			dec->immediate_display = 0;
		}

		/* Update frame tag for packed PB */
		if (CODEC_MULTIFRAME(ctx) && (dec->y_addr_for_pb == dspl_y_addr)) {
			call_cop(ctx, get_buf_update_val, ctx,
					&ctx->dst_ctrls[index],
					V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG,
					dec->stored_tag);
			dec->y_addr_for_pb = 0;
		}

		mfc_qos_update_last_framerate(ctx, ref_mb->vb.vb2_buf.timestamp);
		vb2_buffer_done(&ref_mb->vb.vb2_buf, mfc_get_warn(err) ?
				VB2_BUF_STATE_ERROR : VB2_BUF_STATE_DONE);
	}
}

static void __mfc_handle_frame_new(struct mfc_ctx *ctx, unsigned int err)
{
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_dev *dev = ctx->dev;
	dma_addr_t dspl_y_addr;
	unsigned int frame_type;
	int mvc_view_id;
	unsigned int prev_flag, released_flag = 0;

	frame_type = mfc_get_disp_frame_type();
	mvc_view_id = mfc_get_mvc_disp_view_id();

	if (IS_H264_MVC_DEC(ctx)) {
		if (mvc_view_id == 0)
			ctx->sequence++;
	} else {
		ctx->sequence++;
	}

	dspl_y_addr = mfc_get_disp_y_addr();

	if (dec->immediate_display == 1) {
		dspl_y_addr = (dma_addr_t)mfc_get_dec_y_addr();
		frame_type = mfc_get_dec_frame_type();
	}

	mfc_debug(2, "[FRAME] frame type: %d\n", frame_type);

	/* If frame is same as previous then skip and do not dequeue */
	if (frame_type == MFC_REG_DISPLAY_FRAME_NOT_CODED)
		if (!CODEC_NOT_CODED(ctx))
			return;

	prev_flag = dec->dynamic_used;
	dec->dynamic_used = mfc_get_dec_used_flag();
	released_flag = prev_flag & (~dec->dynamic_used);

	mfc_debug(2, "[DPB] Used flag: old = %08x, new = %08x, Released buffer = %08x\n",
			prev_flag, dec->dynamic_used, released_flag);

	/* decoder dst buffer CFW UNPROT */
	mfc_unprotect_released_dpb(ctx, released_flag);

	if ((IS_VC1_RCV_DEC(ctx) &&
		mfc_get_warn(err) == MFC_REG_ERR_SYNC_POINT_NOT_RECEIVED) ||
		(mfc_get_warn(err) == MFC_REG_ERR_BROKEN_LINK))
		__mfc_handle_frame_output_move(ctx, dspl_y_addr, released_flag);
	else
		__mfc_handle_frame_output_del(ctx, err, released_flag);
}

static void __mfc_handle_frame_error(struct mfc_ctx *ctx,
		unsigned int reason, unsigned int err)
{
	struct mfc_dec *dec;
	struct mfc_buf *src_mb;
	unsigned int index;

	if (ctx->type == MFCINST_ENCODER) {
		mfc_err_ctx("Encoder Interrupt Error: %d\n", err);
		return;
	}

	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no mfc decoder to run\n");
		return;
	}

	mfc_err_ctx("Interrupt Error: %d\n", err);

	/* Get the source buffer */
	src_mb = mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_NO_TOUCH_USED);

	if (!src_mb) {
		mfc_err_dev("no src buffers\n");
	} else {
		index = src_mb->vb.vb2_buf.index;
		if (call_cop(ctx, recover_buf_ctrls_val, ctx, &ctx->src_ctrls[index]) < 0)
			mfc_err_ctx("failed in recover_buf_ctrls_val\n");

		mfc_debug(2, "MFC needs next buffer\n");
		dec->consumed = 0;
		dec->remained_size = 0;

		if (call_cop(ctx, get_buf_ctrls_val, ctx, &ctx->src_ctrls[index]) < 0)
			mfc_err_ctx("failed in get_buf_ctrls_val\n");

		/* decoder src buffer CFW UNPROT */
		if (ctx->is_drm)
			mfc_stream_unprotect(ctx, src_mb, index);

		vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_ERROR);
	}

	mfc_debug(2, "Assesing whether this context should be run again\n");
}

static void __mfc_handle_ref_frame(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_buf *dst_mb;
	dma_addr_t dec_addr;

	dec_addr = (dma_addr_t)mfc_get_dec_y_addr();

	/* Try to search decoded address in whole dst queue */
	dst_mb = mfc_find_move_buf_used(&ctx->buf_queue_lock,
			&ctx->ref_buf_queue, &ctx->dst_buf_queue, dec_addr);
	if (dst_mb) {
		mfc_debug(2, "[DPB] Found in dst queue = 0x%08llx, buf = 0x%08llx\n",
				dec_addr, dst_mb->addr[0][0]);

		if (!(dec->dynamic_set & mfc_get_dec_used_flag()))
			dec->dynamic_used |= dec->dynamic_set;
	} else {
		mfc_debug(2, "[DPB] Can't find buffer for addr = 0x%08llx\n", dec_addr);
	}
}

static void __mfc_handle_reuse_buffer(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	unsigned int prev_flag, released_flag = 0;
	int i;

	prev_flag = dec->dynamic_used;
	dec->dynamic_used = mfc_get_dec_used_flag();
	released_flag = prev_flag & (~dec->dynamic_used);

	if (!released_flag)
		return;

	/* Reuse not referenced buf anymore */
	for (i = 0; i < MFC_MAX_DPBS; i++)
		if (released_flag & (1 << i))
			if (mfc_move_reuse_buffer(ctx, i))
				released_flag &= ~(1 << i);

	/* Not reused buffer should be released when there is a display frame */
	dec->dec_only_release_flag |= released_flag;
	for (i = 0; i < MFC_MAX_DPBS; i++)
		if (released_flag & (1 << i))
			clear_bit(i, &dec->available_dpb);
}

static void __mfc_handle_frame_input(struct mfc_ctx *ctx, unsigned int err)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_buf *src_mb;
	unsigned int index;
	int deleted = 0;
	unsigned long consumed;

	consumed = dec->consumed + mfc_get_consumed_stream();

	if (mfc_get_err(err) == MFC_REG_ERR_NON_PAIRED_FIELD) {
		/*
		 * For non-paired field, the same buffer need to be
		 * resubmitted and the consumed stream will be 0
		 */
		mfc_debug(2, "Not paired field. Running again the same buffer\n");
		return;
	}

	/* Get the source buffer */
	src_mb = mfc_get_del_if_consumed(ctx, &ctx->src_buf_queue,
			mfc_get_consumed_stream(), STUFF_BYTE, err, &deleted);
	if (!src_mb) {
		mfc_err_dev("no src buffers\n");
		return;
	}

	index = src_mb->vb.vb2_buf.index;
	mfc_debug(2, "[BUFINFO] ctx[%d] get src index: %d, addr: 0x%08llx\n",
			ctx->num, index, src_mb->addr[0][0]);

	if (!deleted) {
		/* Run MFC again on the same buffer */
		mfc_debug(2, "[MULTIFRAME] Running again the same buffer\n");

		if (CODEC_MULTIFRAME(ctx))
			dec->y_addr_for_pb = (dma_addr_t)mfc_get_dec_y_addr();

		dec->consumed = consumed;
		dec->remained_size = src_mb->vb.vb2_buf.planes[0].bytesused
					- dec->consumed;
		dec->has_multiframe = 1;

		MFC_TRACE_CTX("** consumed:%ld, remained:%ld, addr:0x%08llx\n",
			dec->consumed, dec->remained_size, dec->y_addr_for_pb);
		/* Do not move src buffer to done_list */
		return;
	}

	if (call_cop(ctx, recover_buf_ctrls_val, ctx, &ctx->src_ctrls[index]) < 0)
		mfc_err_ctx("failed in recover_buf_ctrls_val\n");

	dec->consumed = 0;
	dec->remained_size = 0;

	if (call_cop(ctx, get_buf_ctrls_val, ctx, &ctx->src_ctrls[index]) < 0)
		mfc_err_ctx("failed in get_buf_ctrls_val\n");

	/* decoder src buffer CFW UNPROT */
	if (ctx->is_drm)
		mfc_stream_unprotect(ctx, src_mb, index);

	vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);
}

/* Handle frame decoding interrupt */
static void __mfc_handle_frame(struct mfc_ctx *ctx,
			unsigned int reason, unsigned int err)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	unsigned int dst_frame_status, sei_avail_frame_pack;
	unsigned int res_change, need_dpb_change, need_scratch_change;

	dst_frame_status = mfc_get_disp_status();
	res_change = mfc_get_res_change();
	need_dpb_change = mfc_get_dpb_change();
	need_scratch_change = mfc_get_scratch_change();
	sei_avail_frame_pack = mfc_get_sei_avail_frame_pack();

	if (dec->immediate_display == 1)
		dst_frame_status = mfc_get_dec_status();

	mfc_debug(2, "[FRAME] frame status: %d\n", dst_frame_status);
	mfc_debug(2, "[FRAME] display status: %d, type: %d, yaddr: %#x\n",
			mfc_get_disp_status(), mfc_get_disp_frame_type(),
			mfc_get_disp_y_addr());
	mfc_debug(2, "[FRAME] decoded status: %d, type: %d, yaddr: %#x\n",
			mfc_get_dec_status(), mfc_get_dec_frame_type(),
			mfc_get_dec_y_addr());

	mfc_debug(4, "[HDR] SEI available status: 0x%08x\n", mfc_get_sei_avail());
	mfc_debug(4, "[HDR] SEI content light: 0x%08x\n", mfc_get_sei_content_light());
	mfc_debug(4, "[HDR] SEI luminance: 0x%08x, 0x%08x white point: 0x%08x\n",
			mfc_get_sei_mastering0(), mfc_get_sei_mastering1(),
			mfc_get_sei_mastering2());
	mfc_debug(4, "[HDR] SEI display primaries: 0x%08x, 0x%08x, 0x%08x\n",
			mfc_get_sei_mastering3(), mfc_get_sei_mastering4(),
			mfc_get_sei_mastering5());
	mfc_debug(2, "[DPB] Used flag: old = %08x, new = %08x\n",
				dec->dynamic_used, mfc_get_dec_used_flag());

	if (ctx->state == MFCINST_RES_CHANGE_INIT)
		mfc_change_state(ctx, MFCINST_RES_CHANGE_FLUSH);

	if (res_change) {
		mfc_debug(2, "[DRC] Resolution change set to %d\n", res_change);
		mfc_change_state(ctx, MFCINST_RES_CHANGE_INIT);
		ctx->wait_state = WAIT_G_FMT | WAIT_STOP;
		mfc_debug(2, "[DRC] Decoding waiting! : %d\n", ctx->wait_state);
		return;
	}

	if (need_dpb_change || need_scratch_change) {
		mfc_err_ctx("[DRC] Interframe resolution change is not supported\n");
		mfc_change_state(ctx, MFCINST_ERROR);
		return;
	}

	if (mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->src_buf_queue, 0) &&
		mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->dst_buf_queue, 0)) {
		mfc_err_dev("Queue count is zero for src and dst\n");
		goto leave_handle_frame;
	}

	if (IS_H264_DEC(ctx) && sei_avail_frame_pack &&
		dst_frame_status == MFC_REG_DEC_STATUS_DECODING_ONLY) {
		mfc_debug(2, "Frame packing SEI exists for a frame\n");
		mfc_debug(2, "Reallocate DPBs and issue init_buffer\n");
		ctx->is_dpb_realloc = 1;
		mfc_change_state(ctx, MFCINST_HEAD_PARSED);
		ctx->capture_state = QUEUE_FREE;
		ctx->wait_state = WAIT_STOP;
		__mfc_handle_frame_all_extracted(ctx);
		goto leave_handle_frame;
	}

	/* All frames remaining in the buffer have been extracted  */
	if (dst_frame_status == MFC_REG_DEC_STATUS_DECODING_EMPTY) {
		if (ctx->state == MFCINST_RES_CHANGE_FLUSH) {
			struct mfc_timestamp *temp_ts = NULL;

			mfc_debug(2, "[DRC] Last frame received after resolution change\n");
			__mfc_handle_frame_all_extracted(ctx);
			mfc_change_state(ctx, MFCINST_RES_CHANGE_END);
			/* If there is no display frame after resolution change,
			 * Some released frames can't be unprotected.
			 * So, check and request unprotection in the end of DRC.
			 */
			mfc_cleanup_assigned_dpb(ctx);

			/* empty the timestamp queue */
			while (!list_empty(&ctx->ts_list)) {
				temp_ts = list_entry((&ctx->ts_list)->next,
						struct mfc_timestamp, list);
				list_del(&temp_ts->list);
			}
			ctx->ts_count = 0;
			ctx->ts_is_full = 0;
			mfc_qos_reset_last_framerate(ctx);
			mfc_qos_set_framerate(ctx, DEC_DEFAULT_FPS);

			goto leave_handle_frame;
		} else {
			__mfc_handle_frame_all_extracted(ctx);
		}
	}

	/* Detection for QoS weight */
	if (!dec->num_of_tile_over_4 && mfc_get_num_of_tile() >= 4) {
		dec->num_of_tile_over_4 = 1;
		mfc_qos_on(ctx);
	}
	if (!dec->super64_bframe && IS_SUPER64_BFRAME(ctx, mfc_get_lcu_size(),
				mfc_get_dec_frame_type())) {
		dec->super64_bframe = 1;
		mfc_qos_on(ctx);
	}

	switch (dst_frame_status) {
	case MFC_REG_DEC_STATUS_DECODING_DISPLAY:
		__mfc_handle_ref_frame(ctx);
		break;
	case MFC_REG_DEC_STATUS_DECODING_ONLY:
		__mfc_handle_ref_frame(ctx);
		/*
		 * Some cases can have many decoding only frames like VP9
		 * alt-ref frame. So need handling release buffer
		 * because of DPB full.
		 */
		__mfc_handle_reuse_buffer(ctx);
		break;
	default:
		break;
	}

	if (mfc_dec_status_decoding(dst_frame_status))
		__mfc_handle_frame_copy_timestamp(ctx);

	/* A frame has been decoded and is in the buffer  */
	if (mfc_dec_status_display(dst_frame_status))
		__mfc_handle_frame_new(ctx, err);
	else
		mfc_debug(2, "No frame decode\n");

	/* Mark source buffer as complete */
	if (dst_frame_status != MFC_REG_DEC_STATUS_DISPLAY_ONLY)
		__mfc_handle_frame_input(ctx, err);

leave_handle_frame:
	mfc_debug(2, "Assesing whether this context should be run again\n");
}

static void __mfc_handle_stream_copy_timestamp(struct mfc_ctx *ctx, struct mfc_buf *src_mb)
{
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_enc_params *p = &enc->params;
	struct mfc_buf *dst_mb;
	u64 interval;
	u64 start_timestamp;
	u64 new_timestamp;

	start_timestamp = src_mb->vb.vb2_buf.timestamp;
	interval = NSEC_PER_SEC / p->rc_framerate;
	if (debug_ts == 1)
		mfc_info_ctx("[BUFCON][TS] %dfps, start timestamp: %lld, base interval: %lld\n",
				p->rc_framerate, start_timestamp, interval);

	new_timestamp = start_timestamp + (interval * src_mb->done_index);
	if (debug_ts == 1)
		mfc_info_ctx("[BUFCON][TS] new timestamp: %lld, interval: %lld\n",
				new_timestamp, interval * src_mb->done_index);

	/* Get the destination buffer */
	dst_mb = mfc_get_buf(&ctx->buf_queue_lock, &ctx->dst_buf_queue, MFC_BUF_NO_TOUCH_USED);
	if (dst_mb)
		dst_mb->vb.vb2_buf.timestamp = new_timestamp;
}

static void __mfc_handle_stream_input(struct mfc_ctx *ctx)
{
	struct mfc_raw_info *raw;
	struct mfc_buf *ref_mb, *src_mb;
	dma_addr_t enc_addr[3] = { 0, 0, 0 };
	int i, found_in_src_queue = 0;
	unsigned int index;

	raw = &ctx->raw_buf;

	mfc_get_enc_frame_buffer(ctx, &enc_addr[0], raw->num_planes);
	if (enc_addr[0] == 0) {
		mfc_debug(3, "no encoded src\n");
		goto move_buf;
	}
	for (i = 0; i < raw->num_planes; i++)
		mfc_debug(2, "[BUFINFO] ctx[%d] get src addr[%d]: 0x%08llx\n",
				ctx->num, i, enc_addr[i]);

	if (IS_BUFFER_BATCH_MODE(ctx)) {
		src_mb = mfc_find_first_buf(&ctx->buf_queue_lock,
				&ctx->src_buf_queue, enc_addr[0]);
		if (src_mb) {
			found_in_src_queue = 1;

			__mfc_handle_stream_copy_timestamp(ctx, src_mb);
			src_mb->done_index++;
			mfc_debug(4, "[BUFCON] batch buf done_index: %d\n", src_mb->done_index);

			index = src_mb->vb.vb2_buf.index;

			if (call_cop(ctx, recover_buf_ctrls_val, ctx,
						&ctx->src_ctrls[index]) < 0)
				mfc_err_ctx("failed in recover_buf_ctrls_val\n");

			/* single buffer || last image in a buffer container */
			if (!src_mb->num_valid_bufs || src_mb->done_index == src_mb->num_valid_bufs) {
				src_mb = mfc_find_del_buf(&ctx->buf_queue_lock,
						&ctx->src_buf_queue, enc_addr[0]);
				if (src_mb) {
					for (i = 0; i < raw->num_planes; i++)
						mfc_bufcon_put_daddr(ctx, src_mb, i);
					vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);

					/* encoder src buffer CFW UNPROT */
					if (ctx->is_drm)
						mfc_raw_unprotect(ctx, src_mb, index);
				}
			}
		}
	} else {
		/* normal single buffer */
		src_mb = mfc_find_del_buf(&ctx->buf_queue_lock,
				&ctx->src_buf_queue, enc_addr[0]);
		if (src_mb) {
			found_in_src_queue = 1;
			index = src_mb->vb.vb2_buf.index;
			if (call_cop(ctx, recover_buf_ctrls_val, ctx,
						&ctx->src_ctrls[index]) < 0)
				mfc_err_ctx("failed in recover_buf_ctrls_val\n");

			mfc_debug(3, "find src buf in src_queue\n");
			vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);

			/* encoder src buffer CFW UNPROT */
			if (ctx->is_drm)
				mfc_raw_unprotect(ctx, src_mb, index);
		} else {
			mfc_debug(3, "no src buf in src_queue\n");
			ref_mb = mfc_find_del_buf(&ctx->buf_queue_lock,
					&ctx->ref_buf_queue, enc_addr[0]);
			if (ref_mb) {
				mfc_debug(3, "find src buf in ref_queue\n");
				vb2_buffer_done(&ref_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);

				/* encoder src buffer CFW UNPROT */
				if (ctx->is_drm) {
					index = ref_mb->vb.vb2_buf.index;
					mfc_raw_unprotect(ctx, ref_mb, index);
				}
			} else {
				mfc_err_ctx("couldn't find src buffer\n");
			}
		}
	}

move_buf:
	/* move enqueued src buffer: src queue -> ref queue */
	if (!found_in_src_queue && ctx->state != MFCINST_FINISHING) {
		mfc_get_move_buf_used(&ctx->buf_queue_lock,
				&ctx->ref_buf_queue, &ctx->src_buf_queue);

		mfc_debug(2, "enc src_buf_queue(%d) -> ref_buf_queue(%d)\n",
				mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->src_buf_queue),
				mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->ref_buf_queue));
	}
}

static void __mfc_handle_stream_output(struct mfc_ctx *ctx, int slice_type,
					unsigned int strm_size)
{
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_buf *dst_mb;
	unsigned int index;

	if (strm_size == 0) {
		mfc_debug(3, "no encoded dst (reuse)\n");
		return;
	}

	/* at least one more dest. buffers exist always  */
	dst_mb = mfc_get_del_buf(&ctx->buf_queue_lock,
			&ctx->dst_buf_queue, MFC_BUF_NO_TOUCH_USED);
	if (!dst_mb) {
		mfc_err_ctx("no dst buffers\n");
		return;
	}

	mfc_debug(2, "[BUFINFO] ctx[%d] get dst addr: 0x%08llx\n",
			ctx->num, dst_mb->addr[0][0]);

	dst_mb->vb.flags &= ~(V4L2_BUF_FLAG_KEYFRAME |
				V4L2_BUF_FLAG_PFRAME |
				V4L2_BUF_FLAG_BFRAME);
	switch (slice_type) {
	case MFC_REG_E_SLICE_TYPE_I:
		dst_mb->vb.flags |= V4L2_BUF_FLAG_KEYFRAME;
		break;
	case MFC_REG_E_SLICE_TYPE_P:
		dst_mb->vb.flags |= V4L2_BUF_FLAG_PFRAME;
		break;
	case MFC_REG_E_SLICE_TYPE_B:
		dst_mb->vb.flags |= V4L2_BUF_FLAG_BFRAME;
		break;
	default:
		dst_mb->vb.flags |= V4L2_BUF_FLAG_KEYFRAME;
		break;
	}
	mfc_debug(2, "[STREAM] Slice type flag: %d\n", dst_mb->vb.flags);

	if (IS_BPG_ENC(ctx)) {
		strm_size += enc->header_size;
		mfc_debug(2, "bpg total stream size: %d\n", strm_size);
	}
	vb2_set_plane_payload(&dst_mb->vb.vb2_buf, 0, strm_size);

	index = dst_mb->vb.vb2_buf.index;
	if (call_cop(ctx, get_buf_ctrls_val, ctx, &ctx->dst_ctrls[index]) < 0)
		mfc_err_ctx("failed in get_buf_ctrls_val\n");

	vb2_buffer_done(&dst_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);

	/* encoder dst buffer CFW UNPROT */
	if (ctx->is_drm)
		mfc_stream_unprotect(ctx, dst_mb, index);
}

/* Handle frame encoding interrupt */
static int __mfc_handle_stream(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	int slice_type;
	unsigned int strm_size;
	unsigned int pic_count;

	slice_type = mfc_get_enc_slice_type();
	strm_size = mfc_get_enc_strm_size();
	pic_count = mfc_get_enc_pic_count();

	mfc_debug(2, "[STREAM] encoded slice type: %d, size: %d, display order: %d\n",
			slice_type, strm_size, pic_count);

	/* buffer full handling */
	if (enc->buf_full) {
		mfc_change_state(ctx, MFCINST_ABORT_INST);
		return 0;
	}
	if (ctx->state == MFCINST_RUNNING_BUF_FULL)
		mfc_change_state(ctx, MFCINST_RUNNING);

	/* set encoded frame type */
	enc->frame_type = slice_type;
	ctx->sequence++;

	if (enc->in_slice) {
		if (mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->dst_buf_queue, 0)) {
			mfc_clear_bit(ctx->num, &dev->work_bits);
		}
		return 0;
	}

	/* handle source buffer */
	__mfc_handle_stream_input(ctx);

	/* handle destination buffer */
	__mfc_handle_stream_output(ctx, slice_type, strm_size);

	return 0;
}

/* Error handling for interrupt */
static inline void __mfc_handle_error(struct mfc_ctx *ctx,
	unsigned int reason, unsigned int err)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_buf *src_mb;
	int index;

	mfc_err_ctx("Interrupt Error: display: %d, decoded: %d\n",
			mfc_get_warn(err), mfc_get_err(err));
	err = mfc_get_err(err);

	/* Error recovery is dependent on the state of context */
	switch (ctx->state) {
	case MFCINST_RES_CHANGE_END:
	case MFCINST_GOT_INST:
		/* This error had to happen while parsing the header */
		if (!ctx->is_drm) {
			unsigned char *stream_vir = NULL;
			unsigned int strm_size = 0;

			src_mb = mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_NO_TOUCH_USED);
			if (src_mb) {
				stream_vir = src_mb->vir_addr;
				strm_size = src_mb->vb.vb2_buf.planes[0].bytesused;
				if (strm_size > 640)
					strm_size = 640;

				if (stream_vir && strm_size)
					print_hex_dump(KERN_ERR, "No header: ",
							DUMP_PREFIX_ADDRESS, 32, 4,
							stream_vir, strm_size, false);

				vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);
			}
		} else {
			src_mb = mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_NO_TOUCH_USED);
			if (src_mb) {
				index = src_mb->vb.vb2_buf.index;
				/* decoder src buffer CFW UNPROT */
				mfc_stream_unprotect(ctx, src_mb, index);
				vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);
			}
		}
		break;
	case MFCINST_INIT:
		/* This error had to happen while acquireing instance */
	case MFCINST_RETURN_INST:
		/* This error had to happen while releasing instance */
	case MFCINST_DPB_FLUSHING:
		/* This error had to happen while flushing DPB */
	case MFCINST_SPECIAL_PARSING:
	case MFCINST_SPECIAL_PARSING_NAL:
		/* This error had to happen while special parsing */
		break;
	case MFCINST_HEAD_PARSED:
		/* This error had to happen while setting dst buffers */
	case MFCINST_RES_CHANGE_INIT:
	case MFCINST_RES_CHANGE_FLUSH:
		/* This error has to happen while resolution change */
	case MFCINST_ABORT_INST:
		/* This error has to happen while buffer full handling */
	case MFCINST_FINISHING:
		/* It is higly probable that an error occured
		 * while decoding a frame */
		mfc_change_state(ctx, MFCINST_ERROR);
		/* Mark all dst buffers as having an error */
		mfc_cleanup_queue(&ctx->buf_queue_lock, &ctx->dst_buf_queue);
		/* Mark all src buffers as having an error */
		mfc_cleanup_queue(&ctx->buf_queue_lock, &ctx->src_buf_queue);
		break;
	default:
		mfc_err_ctx("Encountered an error interrupt which had not been handled\n");
		mfc_err_ctx("ctx->state = %d, ctx->inst_no = %d\n",
						ctx->state, ctx->inst_no);
		break;
	}

	mfc_wake_up_dev(dev, reason, err);

	return;
}

/* Handle header decoder interrupt */
static int __mfc_handle_seq_dec(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	int i, is_interlace, is_mbaff;

	if (ctx->src_fmt->fourcc != V4L2_PIX_FMT_FIMV1) {
		ctx->img_width = mfc_get_img_width();
		ctx->img_height = mfc_get_img_height();
		ctx->crop_width = ctx->img_width;
		ctx->crop_height = ctx->img_height;
		mfc_info_ctx("[STREAM] resolution w: %d, h: %d\n", ctx->img_width, ctx->img_height);
	}

	ctx->dpb_count = mfc_get_dpb_count();
	ctx->scratch_buf_size = mfc_get_scratch_size();
	for (i = 0; i < ctx->dst_fmt->num_planes; i++)
		ctx->min_dpb_size[i] = mfc_get_min_dpb_size(i);

	mfc_dec_get_crop_info(ctx);
	dec->mv_count = mfc_get_mv_count();
	if (CODEC_10BIT(ctx) && dev->pdata->support_10bit) {
		if (mfc_get_luma_bit_depth_minus8() ||
			mfc_get_chroma_bit_depth_minus8() ||
			mfc_get_profile() == MFC_REG_D_PROFILE_HEVC_MAIN_10) {
			ctx->is_10bit = 1;
			mfc_info_ctx("[STREAM][10BIT] 10bit contents, profile: %d, depth: %d/%d\n",
					mfc_get_profile(),
					mfc_get_luma_bit_depth_minus8() + 8,
					mfc_get_chroma_bit_depth_minus8() + 8);
		}
	}
	if (CODEC_422FORMAT(ctx) && dev->pdata->support_422) {
		if (mfc_get_chroma_format() == MFC_REG_D_CHROMA_422) {
			ctx->is_422 = 1;
			mfc_info_ctx("[STREAM] 422 chroma format\n");
		}
	}

	if (ctx->img_width == 0 || ctx->img_height == 0)
		mfc_change_state(ctx, MFCINST_ERROR);
	else
		mfc_change_state(ctx, MFCINST_HEAD_PARSED);

	if (ctx->state == MFCINST_HEAD_PARSED) {
		is_interlace = mfc_is_interlace_picture();
		is_mbaff = mfc_is_mbaff_picture();
		if (is_interlace || is_mbaff)
			dec->is_interlaced = 1;
		mfc_debug(2, "[INTERLACE] interlace: %d, mbaff: %d\n", is_interlace, is_mbaff);
	}

	if (IS_H264_DEC(ctx) || IS_H264_MVC_DEC(ctx) || IS_HEVC_DEC(ctx)) {
		struct mfc_buf *src_mb = mfc_get_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_NO_TOUCH_USED);
		if (src_mb) {
			dec->consumed += mfc_get_consumed_stream();
			mfc_debug(2, "[STREAM] header total size : %d, consumed : %lu\n",
					src_mb->vb.vb2_buf.planes[0].bytesused, dec->consumed);
			if ((dec->consumed > 0) &&
					(src_mb->vb.vb2_buf.planes[0].bytesused > dec->consumed)) {
				dec->remained_size = src_mb->vb.vb2_buf.planes[0].bytesused -
					dec->consumed;
				mfc_debug(2, "[STREAM] there is remained bytes(%lu) after header parsing\n",
						dec->remained_size);
			} else {
				dec->consumed = 0;
				dec->remained_size = 0;
			}
		}
	}

	if (IS_VP9_DEC(ctx)) {
		dec->color_range = mfc_get_color_range();
		dec->color_space = mfc_get_color_space();
		mfc_debug(2, "color range: %d, color space: %d, It's valid for VP9\n",
				dec->color_range, dec->color_space);
	}

	return 0;
}

/* Handle header encoder interrupt */
static int __mfc_handle_seq_enc(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_enc_params *p = &enc->params;
	struct mfc_buf *dst_mb;
	int ret;

	enc->header_size = mfc_get_enc_strm_size();
	mfc_debug(2, "[STREAM] encoded slice type: %d, header size: %d, display order: %d\n",
			mfc_get_enc_slice_type(), enc->header_size,
			mfc_get_enc_pic_count());

	if (IS_BPG_ENC(ctx)) {
		dst_mb = mfc_get_buf(&ctx->buf_queue_lock, &ctx->dst_buf_queue, MFC_BUF_NO_TOUCH_USED);
		if (!dst_mb) {
			mfc_err_dev("no dst buffers\n");
			return -EAGAIN;
		}

		dst_mb->vb.vb2_buf.planes[0].data_offset += (enc->header_size +
				p->codec.bpg.thumb_size + p->codec.bpg.exif_size);
		mfc_debug(2, "offset for NAL_START: %d (header: %d + thumb: %d + exif: %d)\n",
				dst_mb->vb.vb2_buf.planes[0].data_offset,
				enc->header_size,
				p->codec.bpg.thumb_size,
				p->codec.bpg.exif_size);
	} else {
		if ((p->seq_hdr_mode == V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE) ||
			(p->seq_hdr_mode == V4L2_MPEG_VIDEO_HEADER_MODE_AT_THE_READY)) {
			dst_mb = mfc_get_del_buf(&ctx->buf_queue_lock,
					&ctx->dst_buf_queue, MFC_BUF_NO_TOUCH_USED);
			if (!dst_mb) {
				mfc_err_dev("no dst buffers\n");
				return -EAGAIN;
			}

			vb2_set_plane_payload(&dst_mb->vb.vb2_buf, 0, mfc_get_enc_strm_size());
			vb2_buffer_done(&dst_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);

			/* encoder dst buffer CFW UNPROT */
			if (ctx->is_drm) {
				int index = dst_mb->vb.vb2_buf.index;

				mfc_stream_unprotect(ctx, dst_mb, index);
			}
		}
	}

	ctx->dpb_count = mfc_get_enc_dpb_count();
	ctx->scratch_buf_size = mfc_get_enc_scratch_size();

	/* If the ROI is enabled at SEQ_START, clear ROI_ENABLE bit */
	mfc_clear_roi_enable(dev);

	if (ctx->codec_buffer_allocated) {
		mfc_debug(2, "[DRC] previous codec buffer is exist\n");

		if (dev->has_mmcache && dev->mmcache.is_on_status)
			mfc_invalidate_mmcache(dev);

		mfc_release_codec_buffers(ctx);
	}
	ret = mfc_alloc_codec_buffers(ctx);
	if (ret) {
		mfc_err_ctx("Failed to allocate encoding buffers\n");
		return ret;
	}

	mfc_change_state(ctx, MFCINST_HEAD_PARSED);

	return 0;
}

static inline int __mfc_is_err_condition(unsigned int err)
{
	if (err == MFC_REG_ERR_NO_AVAILABLE_DPB ||
		err == MFC_REG_ERR_INSUFFICIENT_DPB_SIZE ||
		err == MFC_REG_ERR_INSUFFICIENT_NUM_DPB ||
		err == MFC_REG_ERR_INSUFFICIENT_MV_BUF_SIZE ||
		err == MFC_REG_ERR_INSUFFICIENT_SCRATCH_BUF_SIZE)
		return 1;

	return 0;
}

irqreturn_t mfc_top_half_irq(int irq, void *priv)
{
	struct mfc_dev *dev = priv;
	struct mfc_ctx *ctx;
	unsigned int err;
	unsigned int reason;

	ctx = dev->ctx[dev->curr_ctx];
	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return IRQ_WAKE_THREAD;
	}

	reason = mfc_get_int_reason();
	err = mfc_get_int_err();
	mfc_debug(2, "[c:%d] Int reason: %d (err: %d)\n",
			dev->curr_ctx, reason, err);
	MFC_TRACE_CTX("<< INT(top): %d\n", reason);

	mfc_perf_measure_off(dev);

	return IRQ_WAKE_THREAD;
}

/*
 * Return value description
 *  0: NAL-Q is handled successfully
 *  1: NAL_START command
 * -1: Error
*/
static inline int __mfc_nal_q_irq(struct mfc_dev *dev,
		unsigned int reason, unsigned int err)
{
	int ret = -1;
	unsigned int errcode;

	nal_queue_handle *nal_q_handle = dev->nal_q_handle;
	EncoderOutputStr *pOutStr;

	switch (reason) {
	case MFC_REG_R2H_CMD_QUEUE_DONE_RET:
		pOutStr = mfc_nal_q_dequeue_out_buf(dev,
			nal_q_handle->nal_q_out_handle, &errcode);
		if (pOutStr) {
			if (mfc_nal_q_handle_out_buf(dev, pOutStr))
				mfc_err_dev("[NALQ] Failed to handle out buf\n");
		} else {
			mfc_err_dev("[NALQ] pOutStr is NULL\n");
		}

		if (nal_q_handle->nal_q_exception)
			mfc_set_bit(nal_q_handle->nal_q_out_handle->nal_q_ctx,
					&dev->work_bits);
		mfc_clear_int();

		if (!nal_q_handle->nal_q_exception)
			mfc_nal_q_clock_off(dev, nal_q_handle);

		ret = 0;
		break;
	case MFC_REG_R2H_CMD_COMPLETE_QUEUE_RET:
		mfc_watchdog_stop_tick(dev);
		nal_q_handle->nal_q_state = NAL_Q_STATE_CREATED;
		MFC_TRACE_DEV("** NAL Q state : %d\n", nal_q_handle->nal_q_state);
		mfc_debug(2, "[NALQ] return to created state\n");
		mfc_nal_q_cleanup_queue(dev);
		mfc_nal_q_cleanup_clock(dev);
		mfc_clear_int();
		mfc_pm_clock_off(dev);
		mfc_wake_up_dev(dev, reason, err);

		ret = 0;
		break;
	default:
		if (nal_q_handle->nal_q_state == NAL_Q_STATE_STARTED ||
			nal_q_handle->nal_q_state == NAL_Q_STATE_STOPPED) {
			mfc_err_dev("[NALQ] Should not be here! state: %d, int reason : %d\n",
				nal_q_handle->nal_q_state, reason);
			mfc_clear_int();

			ret = -1;
		} else {
			/* NAL START */
			ret = 1;
		}

		break;
	}

	if (ret == 0)
		queue_work(dev->butler_wq, &dev->butler_work);

	return ret;
}

static inline int __mfc_handle_done_frame(struct mfc_ctx *ctx,
				unsigned int reason, unsigned int err)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = NULL;

	if (ctx->type == MFCINST_DECODER) {
		if (ctx->state == MFCINST_SPECIAL_PARSING_NAL) {
			mfc_clear_int();
			mfc_pm_clock_off(dev);
			mfc_clear_bit(ctx->num, &dev->work_bits);
			mfc_change_state(ctx, MFCINST_RUNNING);
			mfc_wake_up_ctx(ctx, reason, err);
			return 0;
		}
		__mfc_handle_frame(ctx, reason, err);
	} else if (ctx->type == MFCINST_ENCODER) {
		if (ctx->otf_handle) {
			mfc_otf_handle_stream(ctx);
			return 1;
		}
		enc = ctx->enc_priv;
		if (reason == MFC_REG_R2H_CMD_SLICE_DONE_RET) {
			dev->preempt_ctx = ctx->num;
			enc->buf_full = 0;
			enc->in_slice = 1;
		} else if (reason == MFC_REG_R2H_CMD_ENC_BUFFER_FULL_RET) {
			mfc_err_ctx("stream buffer size(%d) isn't enough\n",
					mfc_get_enc_strm_size());
			dev->preempt_ctx = ctx->num;
			enc->buf_full = 1;
			enc->in_slice = 0;
		} else {
			enc->buf_full = 0;
			enc->in_slice = 0;
		}
		__mfc_handle_stream(ctx);
	}

	return 1;
}

static inline void __mfc_handle_nal_abort(struct mfc_ctx *ctx)
{
	struct mfc_enc *enc = ctx->enc_priv;

	if (ctx->type == MFCINST_ENCODER) {
		mfc_change_state(ctx, MFCINST_RUNNING_BUF_FULL);
		enc->buf_full = 0;
		if (IS_VP8_ENC(ctx))
			mfc_err_ctx("stream buffer size isn't enough\n");
		__mfc_handle_stream(ctx);
	} else {
		mfc_change_state(ctx, MFCINST_ABORT);
	}
}

static int __mfc_irq_dev(struct mfc_dev *dev, unsigned int reason, unsigned int err)
{
	/* Stop the timeout watchdog */
	if (reason != MFC_REG_R2H_CMD_FW_STATUS_RET)
		mfc_watchdog_stop_tick(dev);

	switch (reason) {
	case MFC_REG_R2H_CMD_CACHE_FLUSH_RET:
	case MFC_REG_R2H_CMD_SYS_INIT_RET:
	case MFC_REG_R2H_CMD_FW_STATUS_RET:
	case MFC_REG_R2H_CMD_SLEEP_RET:
	case MFC_REG_R2H_CMD_WAKEUP_RET:
		mfc_clear_int();
		mfc_wake_up_dev(dev, reason, err);
		return 0;
	}

	return 1;
}

static int __mfc_irq_ctx(struct mfc_ctx *ctx, unsigned int reason, unsigned int err)
{
	struct mfc_dev *dev = ctx->dev;

	switch (reason) {
	case MFC_REG_R2H_CMD_ERR_RET:
		if (ctx->otf_handle) {
			mfc_otf_handle_error(ctx, reason, err);
			break;
		}
		/* An error has occured */
		if (ctx->state == MFCINST_RUNNING || ctx->state == MFCINST_ABORT) {
			if ((mfc_get_err(err) >= MFC_REG_ERR_WARNINGS_START) &&
				(mfc_get_err(err) <= MFC_REG_ERR_WARNINGS_END))
				__mfc_handle_frame(ctx, reason, err);
			else
				__mfc_handle_frame_error(ctx, reason, err);
		} else {
			__mfc_handle_error(ctx, reason, err);
		}
		break;
	case MFC_REG_R2H_CMD_SLICE_DONE_RET:
	case MFC_REG_R2H_CMD_FIELD_DONE_RET:
	case MFC_REG_R2H_CMD_FRAME_DONE_RET:
	case MFC_REG_R2H_CMD_ENC_BUFFER_FULL_RET:
		return __mfc_handle_done_frame(ctx, reason, err);
	case MFC_REG_R2H_CMD_COMPLETE_SEQ_RET:
		if (ctx->type == MFCINST_ENCODER) {
			__mfc_handle_stream(ctx);
			mfc_change_state(ctx, MFCINST_FINISHED);
		} else if (ctx->type == MFCINST_DECODER) {
			return __mfc_handle_done_frame(ctx, reason, err);
		}
		break;
	case MFC_REG_R2H_CMD_SEQ_DONE_RET:
		if (ctx->type == MFCINST_ENCODER) {
			if (ctx->otf_handle) {
				mfc_otf_handle_seq(ctx);
				break;
			}
			__mfc_handle_seq_enc(ctx);
		} else if (ctx->type == MFCINST_DECODER) {
			__mfc_handle_seq_dec(ctx);
		}
		break;
	case MFC_REG_R2H_CMD_OPEN_INSTANCE_RET:
		ctx->inst_no = mfc_get_inst_no();
		mfc_change_state(ctx, MFCINST_GOT_INST);
		break;
	case MFC_REG_R2H_CMD_CLOSE_INSTANCE_RET:
		mfc_change_state(ctx, MFCINST_FREE);
		break;
	case MFC_REG_R2H_CMD_NAL_ABORT_RET:
		__mfc_handle_nal_abort(ctx);
		break;
	case MFC_REG_R2H_CMD_DPB_FLUSH_RET:
		mfc_change_state(ctx, MFCINST_ABORT);
		break;
	case MFC_REG_R2H_CMD_INIT_BUFFERS_RET:
		if (err != 0) {
			mfc_err_ctx("INIT_BUFFERS_RET error: %d\n", err);
			break;
		}

		mfc_change_state(ctx, MFCINST_RUNNING);
		if (ctx->type == MFCINST_DECODER) {
			if (ctx->is_dpb_realloc)
				ctx->is_dpb_realloc = 0;
		}
		break;
	default:
		mfc_err_ctx("Unknown int reason: %d\n", reason);
	}

	return 1;
}

/* Interrupt processing */
irqreturn_t mfc_irq(int irq, void *priv)
{
	struct mfc_dev *dev = priv;
	struct mfc_ctx *ctx;
	unsigned int reason;
	unsigned int err;
	int ret = -1;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		goto irq_end;
	}

	if (mfc_pm_get_pwr_ref_cnt(dev) == 0) {
		mfc_err_dev("no mfc power on\n");
		call_dop(dev, dump_and_stop_debug_mode, dev);
		goto irq_end;
	}

	/* Get the reason of interrupt and the error code */
	reason = mfc_get_int_reason();
	err = mfc_get_int_err();
	mfc_debug(1, "Int reason: %d (err: %d)\n", reason, err);
	MFC_TRACE_DEV("<< INT: %d (err: %d)\n", reason, err);

	dev->preempt_ctx = MFC_NO_INSTANCE_SET;

	if (dbg_enable && (reason != MFC_REG_R2H_CMD_QUEUE_DONE_RET))
		mfc_dbg_disable(dev);

	if ((sfr_dump & MFC_DUMP_ERR_INT) && (reason == MFC_REG_R2H_CMD_ERR_RET))
		call_dop(dev, dump_regs, dev);

	if ((sfr_dump & MFC_DUMP_WARN_INT) &&
			(err && (reason != MFC_REG_R2H_CMD_ERR_RET)))
		call_dop(dev, dump_regs, dev);

	if (__mfc_is_err_condition(err))
		call_dop(dev, dump_and_stop_debug_mode, dev);

	if (dev->nal_q_handle) {
		ret = __mfc_nal_q_irq(dev, reason, err);
		if (ret == 0) {
			mfc_debug(2, "[NALQ] command was handled\n");
			goto irq_end;
		} else if (ret == 1){
			/* Path through */
			mfc_debug(2, "NAL_START command will be handled\n");
		} else {
			mfc_debug(2, "[NALQ] command handling Error\n");
			goto irq_end;
		}
	}

	ret = __mfc_irq_dev(dev, reason, err);
	if (!ret)
		goto irq_end;

	ctx = dev->ctx[dev->curr_ctx];
	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		mfc_clear_int();
		mfc_pm_clock_off(dev);
		goto irq_end;
	}

	ret = __mfc_irq_ctx(ctx, reason, err);
	if (!ret)
		goto irq_end;

	/* clean-up interrupt */
	mfc_clear_int();

	if ((ctx->state != MFCINST_RES_CHANGE_INIT) && (mfc_ctx_ready(ctx) == 0))
		mfc_clear_bit(ctx->num, &dev->work_bits);

	if (ctx->otf_handle) {
		if (mfc_otf_ctx_ready(ctx))
			mfc_set_bit(ctx->num, &dev->work_bits);
		else
			mfc_clear_bit(ctx->num, &dev->work_bits);
	}

	mfc_hwlock_handler_irq(dev, ctx, reason, err);

irq_end:
	mfc_debug_leave();
	return IRQ_HANDLED;
}
