/*
 * drivers/media/platform/exynos/mfc/mfc_dec_vb2_ops.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include  "mfc_common.h"

#include "mfc_hwlock.h"
#include "mfc_nal_q.h"
#include "mfc_run.h"
#include "mfc_sync.h"

#include "mfc_queue.h"
#include "mfc_utils.h"
#include "mfc_buf.h"
#include "mfc_mem.h"

static int mfc_dec_queue_setup(struct vb2_queue *vq,
				unsigned int *buf_count, unsigned int *plane_count,
				unsigned int psize[], struct device *alloc_devs[])
{
	struct mfc_ctx *ctx = vq->drv_priv;
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_raw_info *raw;
	int i;

	mfc_debug_enter();

	raw = &ctx->raw_buf;

	/* Video output for decoding (source)
	 * this can be set after getting an instance */
	if (ctx->state == MFCINST_GOT_INST &&
	    vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		mfc_debug(4, "dec src\n");
		/* A single plane is required for input */
		*plane_count = 1;
		if (*buf_count < 1)
			*buf_count = 1;
		if (*buf_count > MFC_MAX_BUFFERS)
			*buf_count = MFC_MAX_BUFFERS;

		psize[0] = dec->src_buf_size;
		alloc_devs[0] = dev->device;
	/* Video capture for decoding (destination)
	 * this can be set after the header was parsed */
	} else if (ctx->state == MFCINST_HEAD_PARSED &&
		   vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		mfc_debug(4, "dec dst\n");
		/* Output plane count is different by the pixel format */
		*plane_count = ctx->dst_fmt->mem_planes;
		/* Setup buffer count */
		if (*buf_count < ctx->dpb_count)
			*buf_count = ctx->dpb_count;
		if (*buf_count > MFC_MAX_BUFFERS)
			*buf_count = MFC_MAX_BUFFERS;

		if (ctx->dst_fmt->mem_planes == 1) {
			psize[0] = raw->total_plane_size;
			alloc_devs[0] = dev->device;
		} else {
			for (i = 0; i < ctx->dst_fmt->num_planes; i++) {
				psize[i] = raw->plane_size[i];
				alloc_devs[i] = dev->device;
			}
		}
	} else {
		mfc_err_ctx("State seems invalid. State = %d, vq->type = %d\n",
							ctx->state, vq->type);
		return -EINVAL;
	}

	mfc_debug(2, "buf_count: %d, plane_count: %d, type: %#x\n",
			*buf_count, *plane_count, vq->type);
	for (i = 0; i < *plane_count; i++)
		mfc_debug(2, "plane[%d] size: %d\n", i, psize[i]);

	mfc_debug_leave();

	return 0;
}

static void mfc_dec_unlock(struct vb2_queue *q)
{
	struct mfc_ctx *ctx = q->drv_priv;
	struct mfc_dev *dev = ctx->dev;

	mutex_unlock(&dev->mfc_mutex);
}

static void mfc_dec_lock(struct vb2_queue *q)
{
	struct mfc_ctx *ctx = q->drv_priv;
	struct mfc_dev *dev = ctx->dev;

	mutex_lock(&dev->mfc_mutex);
}

static int mfc_dec_buf_init(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct mfc_ctx *ctx = vq->drv_priv;
	struct mfc_buf *buf = vb_to_mfc_buf(vb);
	dma_addr_t start_raw;
	int i, ret;

	mfc_debug_enter();

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = mfc_check_vb_with_fmt(ctx->dst_fmt, vb);
		if (ret < 0)
			return ret;

		start_raw = mfc_mem_get_daddr_vb(vb, 0);
		if (ctx->dst_fmt->fourcc == V4L2_PIX_FMT_NV12N) {
			buf->addr[0][0] = start_raw;
			buf->addr[0][1] = NV12N_CBCR_BASE(start_raw,
							ctx->img_width,
							ctx->img_height);
		} else if (ctx->dst_fmt->fourcc == V4L2_PIX_FMT_NV12N_10B) {
			buf->addr[0][0] = start_raw;
			buf->addr[0][1] = NV12N_10B_CBCR_BASE(start_raw,
							ctx->img_width,
							ctx->img_height);
		} else if (ctx->dst_fmt->fourcc == V4L2_PIX_FMT_YUV420N) {
			buf->addr[0][0] = start_raw;
			buf->addr[0][1] = YUV420N_CB_BASE(start_raw,
							ctx->img_width,
							ctx->img_height);
			buf->addr[0][2] = YUV420N_CR_BASE(start_raw,
							ctx->img_width,
							ctx->img_height);
		} else {
			for (i = 0; i < ctx->dst_fmt->mem_planes; i++)
				buf->addr[0][i] = mfc_mem_get_daddr_vb(vb, i);
		}

		if (call_cop(ctx, init_buf_ctrls, ctx, MFC_CTRL_TYPE_DST,
					vb->index) < 0)
			mfc_err_ctx("failed in init_buf_ctrls\n");
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		ret = mfc_check_vb_with_fmt(ctx->src_fmt, vb);
		if (ret < 0)
			return ret;

		buf->addr[0][0] = mfc_mem_get_daddr_vb(vb, 0);

		if (call_cop(ctx, init_buf_ctrls, ctx, MFC_CTRL_TYPE_SRC,
					vb->index) < 0)
			mfc_err_ctx("failed in init_buf_ctrls\n");
	} else {
		mfc_err_ctx("mfc_dec_buf_init: unknown queue type\n");
		return -EINVAL;
	}

	mfc_debug_leave();

	return 0;
}

static int mfc_dec_buf_prepare(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct mfc_ctx *ctx = vq->drv_priv;
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_raw_info *raw;
	unsigned int index = vb->index;
	size_t buf_size;
	int i;

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		raw = &ctx->raw_buf;
		/* check the size per plane */
		if (ctx->dst_fmt->mem_planes == 1) {
			buf_size = vb2_plane_size(vb, 0);
			mfc_debug(2, "[FRAME] single plane vb size: %lu, calc size: %d\n",
					buf_size, raw->total_plane_size);
			if (buf_size < raw->total_plane_size) {
				mfc_err_ctx("[FRAME] single plane size(%lu) is smaller than (%d)\n",
						buf_size, raw->total_plane_size);
				return -EINVAL;
			}
		} else {
			for (i = 0; i < ctx->dst_fmt->mem_planes; i++) {
				buf_size = vb2_plane_size(vb, i);
				mfc_debug(2, "[FRAME] plane[%d] vb size: %lu, calc size: %d\n",
						i, buf_size, raw->plane_size[i]);
				if (buf_size < raw->plane_size[i]) {
					mfc_err_ctx("[FRAME] plane[%d] size(%lu) is smaller than (%d)\n",
							i, buf_size, raw->plane_size[i]);
					return -EINVAL;
				}
			}
		}
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		buf_size = vb2_plane_size(vb, 0);
		mfc_debug(2, "[STREAM] vb size: %lu, calc size: %d\n",
				buf_size, dec->src_buf_size);

		if (buf_size < dec->src_buf_size) {
			mfc_err_ctx("[STREAM] size(%lu) is smaller than (%d)\n",
					buf_size, dec->src_buf_size);
			return -EINVAL;
		}

		if (call_cop(ctx, to_buf_ctrls, ctx, &ctx->src_ctrls[index]) < 0)
			mfc_err_ctx("failed in to_buf_ctrls\n");
	}

	return 0;
}

static void mfc_dec_buf_finish(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct mfc_ctx *ctx = vq->drv_priv;
	unsigned int index = vb->index;

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		if (call_cop(ctx, to_ctx_ctrls, ctx, &ctx->dst_ctrls[index]) < 0)
			mfc_err_ctx("failed in to_ctx_ctrls\n");
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if (call_cop(ctx, to_ctx_ctrls, ctx, &ctx->src_ctrls[index]) < 0)
			mfc_err_ctx("failed in to_ctx_ctrls\n");
	}
}

static void mfc_dec_buf_cleanup(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct mfc_ctx *ctx = vq->drv_priv;
	unsigned int index = vb->index;

	mfc_debug_enter();

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		if (call_cop(ctx, cleanup_buf_ctrls, ctx,
					MFC_CTRL_TYPE_DST, index) < 0)
			mfc_err_ctx("failed in cleanup_buf_ctrls\n");
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if (call_cop(ctx, cleanup_buf_ctrls, ctx,
					MFC_CTRL_TYPE_SRC, index) < 0)
			mfc_err_ctx("failed in cleanup_buf_ctrls\n");
	} else {
		mfc_err_ctx("mfc_dec_buf_cleanup: unknown queue type\n");
	}

	mfc_debug_leave();
}

static int mfc_dec_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct mfc_ctx *ctx = q->drv_priv;
	struct mfc_dev *dev = ctx->dev;

	mfc_update_real_time(ctx);

	/* If context is ready then dev = work->data;schedule it to run */
	if (mfc_ctx_ready(ctx))
		mfc_set_bit(ctx->num, &dev->work_bits);

	mfc_try_run(dev);

	return 0;
}

static void __mfc_dec_src_stop_streaming(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_buf *src_mb;
	int index = 0, csd, condition = 0, prev_state = 0;
	int ret = 0;

	while (1) {
		csd = mfc_check_buf_vb_flag(ctx, MFC_FLAG_CSD);

		if (csd == 1) {
			mfc_clean_ctx_int_flags(ctx);
			if (need_to_special_parsing(ctx)) {
				prev_state = ctx->state;
				mfc_change_state(ctx, MFCINST_SPECIAL_PARSING);
				condition = MFC_REG_R2H_CMD_SEQ_DONE_RET;
				mfc_info_ctx("try to special parsing! (before NAL_START)\n");
			} else if (need_to_special_parsing_nal(ctx)) {
				prev_state = ctx->state;
				mfc_change_state(ctx, MFCINST_SPECIAL_PARSING_NAL);
				condition = MFC_REG_R2H_CMD_FRAME_DONE_RET;
				mfc_info_ctx("try to special parsing! (after NAL_START)\n");
			} else {
				mfc_info_ctx("can't parsing CSD!, state = %d\n", ctx->state);
			}

			if (condition) {
				mfc_set_bit(ctx->num, &dev->work_bits);

				ret = mfc_just_run(dev, ctx->num);
				if (ret) {
					mfc_err_ctx("Failed to run MFC\n");
					mfc_change_state(ctx, prev_state);
				} else {
					if (mfc_wait_for_done_ctx(ctx, condition))
						mfc_err_ctx("special parsing time out\n");
				}
			}
		}

		src_mb = mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_NO_TOUCH_USED);
		if (!src_mb)
			break;

		index = src_mb->vb.vb2_buf.index;

		if (ctx->is_drm)
			mfc_stream_unprotect(ctx, src_mb, index);
		vb2_set_plane_payload(&src_mb->vb.vb2_buf, 0, 0);
		vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_ERROR);
	}

	dec->consumed = 0;
	dec->remained_size = 0;
	ctx->check_dump = 0;

	mfc_init_queue(&ctx->src_buf_queue);

	index = 0;
	while (index < MFC_MAX_BUFFERS) {
		index = find_next_bit(&ctx->src_ctrls_avail,
				MFC_MAX_BUFFERS, index);
		if (index < MFC_MAX_BUFFERS)
			call_cop(ctx, reset_buf_ctrls, &ctx->src_ctrls[index]);
		index++;
	}
}

static void __mfc_dec_dst_stop_streaming(struct mfc_ctx *ctx)
{
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_dev *dev = ctx->dev;
	int index = 0;

	mfc_cleanup_assigned_iovmm(ctx);

	mfc_cleanup_assigned_fd(ctx);
	mfc_cleanup_queue(&ctx->buf_queue_lock, &ctx->ref_buf_queue);

	dec->dynamic_used = 0;
	dec->err_reuse_flag = 0;
	dec->dec_only_release_flag = 0;

	mfc_cleanup_queue(&ctx->buf_queue_lock, &ctx->dst_buf_queue);

	ctx->is_dpb_realloc = 0;
	dec->available_dpb = 0;

	dec->y_addr_for_pb = 0;

	mfc_cleanup_assigned_dpb(ctx);

	while (index < MFC_MAX_BUFFERS) {
		index = find_next_bit(&ctx->dst_ctrls_avail,
				MFC_MAX_BUFFERS, index);
		if (index < MFC_MAX_BUFFERS)
			call_cop(ctx, reset_buf_ctrls, &ctx->dst_ctrls[index]);
		index++;
	}

	if (ctx->wait_state & WAIT_STOP) {
		ctx->wait_state &= ~(WAIT_STOP);
		mfc_debug(2, "clear WAIT_STOP %d\n", ctx->wait_state);
		MFC_TRACE_CTX("** DEC clear WAIT_STOP(wait_state %d)\n", ctx->wait_state);
	}
}

static void mfc_dec_stop_streaming(struct vb2_queue *q)
{
	struct mfc_ctx *ctx = q->drv_priv;
	struct mfc_dev *dev = ctx->dev;
	int ret = 0;
	int prev_state;

	mfc_info_ctx("dec stop_streaming is called, hwlock : %d, type : %d\n",
				test_bit(ctx->num, &dev->hwlock.bits), q->type);
	MFC_TRACE_CTX("** DEC streamoff(type:%d)\n", q->type);

	/* If a H/W operation is in progress, wait for it complete */
	ret = mfc_get_hwlock_ctx(ctx);
	if (ret < 0) {
		mfc_err_ctx("Failed to get hwlock\n");
		return;
	}

	if (q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		__mfc_dec_dst_stop_streaming(ctx);
	else if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		__mfc_dec_src_stop_streaming(ctx);

	if (ctx->state == MFCINST_FINISHING)
		mfc_change_state(ctx, MFCINST_RUNNING);

	if (q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE && need_to_dpb_flush(ctx)) {
		prev_state = ctx->state;
		mfc_change_state(ctx, MFCINST_DPB_FLUSHING);
		mfc_set_bit(ctx->num, &dev->work_bits);
		mfc_clean_ctx_int_flags(ctx);
		mfc_info_ctx("try to DPB flush\n");
		ret = mfc_just_run(dev, ctx->num);
		if (ret) {
			mfc_err_ctx("Failed to run MFC\n");
			mfc_release_hwlock_ctx(ctx);
			mfc_cleanup_work_bit_and_try_run(ctx);
			return;
		}

		if (mfc_wait_for_done_ctx(ctx, MFC_REG_R2H_CMD_DPB_FLUSH_RET)) {
			mfc_err_ctx("time out during DPB flush\n");
			dev->logging_data->cause |= (1 << MFC_CAUSE_FAIL_DPB_FLUSH);
			call_dop(dev, dump_and_stop_always, dev);
		}

		mfc_change_state(ctx, prev_state);
	}

	mfc_debug(2, "buffer cleanup & flush is done in stop_streaming, type : %d\n", q->type);

	mfc_clear_bit(ctx->num, &dev->work_bits);
	mfc_release_hwlock_ctx(ctx);

	if (mfc_ctx_ready(ctx))
		mfc_set_bit(ctx->num, &dev->work_bits);
	if (mfc_is_work_to_do(dev))
		queue_work(dev->butler_wq, &dev->butler_work);
}

static void mfc_dec_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct mfc_ctx *ctx = vq->drv_priv;
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_buf *buf = vb_to_mfc_buf(vb);
	int i;
	unsigned char *stream_vir = NULL;

	mfc_debug_enter();

	if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		mfc_debug(2, "[BUFINFO] ctx[%d] add src index:%d, addr: 0x%08llx\n",
				ctx->num, vb->index, buf->addr[0][0]);
		if (vb->memory == V4L2_MEMORY_DMABUF &&
				ctx->state < MFCINST_HEAD_PARSED && !ctx->is_drm)
			stream_vir = vb2_plane_vaddr(vb, 0);

		buf->vir_addr = stream_vir;

		mfc_add_tail_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, buf);

		if (debug_ts == 1)
			mfc_info_ctx("[TS] framerate: %ld, timestamp: %lld\n",
					ctx->framerate, buf->vb.vb2_buf.timestamp);

	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		for (i = 0; i < ctx->dst_fmt->mem_planes; i++)
			mfc_debug(2, "[BUFINFO] ctx[%d] add dst index: %d, addr[%d]: 0x%08llx\n",
					ctx->num, vb->index, i, buf->addr[0][i]);
		mfc_store_dpb(ctx, vb);

		if ((vb->memory == V4L2_MEMORY_USERPTR || vb->memory == V4L2_MEMORY_DMABUF) &&
				mfc_is_queue_count_same(&ctx->buf_queue_lock,
					&ctx->dst_buf_queue, dec->total_dpb_count))
			ctx->capture_state = QUEUE_BUFS_MMAPED;

		MFC_TRACE_CTX("Q dst[%d] fd: %d, %#llx / avail %#lx used %#x\n",
				vb->index, vb->planes[0].m.fd, buf->addr[0][0],
				dec->available_dpb, dec->dynamic_used);
	} else {
		mfc_err_ctx("Unsupported buffer type (%d)\n", vq->type);
	}

	if (mfc_ctx_ready(ctx)) {
		mfc_set_bit(ctx->num, &dev->work_bits);
		mfc_try_run(dev);
	}

	mfc_debug_leave();
}

struct vb2_ops mfc_dec_qops = {
	.queue_setup		= mfc_dec_queue_setup,
	.wait_prepare		= mfc_dec_unlock,
	.wait_finish		= mfc_dec_lock,
	.buf_init		= mfc_dec_buf_init,
	.buf_prepare		= mfc_dec_buf_prepare,
	.buf_finish		= mfc_dec_buf_finish,
	.buf_cleanup		= mfc_dec_buf_cleanup,
	.start_streaming	= mfc_dec_start_streaming,
	.stop_streaming		= mfc_dec_stop_streaming,
	.buf_queue		= mfc_dec_buf_queue,
};
