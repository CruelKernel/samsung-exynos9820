/*
 * drivers/media/platform/exynos/mfc/mfc_enc_vb2_ops.c
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

#include "mfc_hwlock.h"
#include "mfc_nal_q.h"
#include "mfc_run.h"
#include "mfc_sync.h"

#include "mfc_qos.h"
#include "mfc_queue.h"
#include "mfc_utils.h"
#include "mfc_buf.h"
#include "mfc_mem.h"

static int mfc_enc_queue_setup(struct vb2_queue *vq,
				unsigned int *buf_count, unsigned int *plane_count,
				unsigned int psize[], struct device *alloc_devs[])
{
	struct mfc_ctx *ctx = vq->drv_priv;
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_raw_info *raw;
	int i;

	mfc_debug_enter();

	if (ctx->state != MFCINST_GOT_INST &&
	    vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		mfc_err_ctx("invalid state: %d\n", ctx->state);
		return -EINVAL;
	}
	if (ctx->state >= MFCINST_FINISHING &&
	    vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		mfc_err_ctx("invalid state: %d\n", ctx->state);
		return -EINVAL;
	}

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		mfc_debug(4, "enc dst\n");
		if (ctx->dst_fmt)
			*plane_count = ctx->dst_fmt->mem_planes;
		else
			*plane_count = MFC_ENC_CAP_PLANE_COUNT;

		if (*buf_count < 1)
			*buf_count = 1;
		if (*buf_count > MFC_MAX_BUFFERS)
			*buf_count = MFC_MAX_BUFFERS;

		psize[0] = enc->dst_buf_size;
		alloc_devs[0] = dev->device;
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		mfc_debug(4, "enc src\n");
		raw = &ctx->raw_buf;

		if (ctx->src_fmt)
			*plane_count = ctx->src_fmt->mem_planes;
		else
			*plane_count = MFC_ENC_OUT_PLANE_COUNT;

		if (*buf_count < 1)
			*buf_count = 1;
		if (*buf_count > MFC_MAX_BUFFERS)
			*buf_count = MFC_MAX_BUFFERS;

		if (*plane_count == 1) {
			psize[0] = raw->total_plane_size;
			alloc_devs[0] = dev->device;
		} else {
			for (i = 0; i < *plane_count; i++) {
				psize[i] = raw->plane_size[i];
				alloc_devs[i] = dev->device;
			}
		}
	} else {
		mfc_err_ctx("invalid queue type: %d\n", vq->type);
		return -EINVAL;
	}

	mfc_debug(2, "buf_count: %d, plane_count: %d, type: %#x\n",
			*buf_count, *plane_count, vq->type);
	for (i = 0; i < *plane_count; i++)
		mfc_debug(2, "plane[%d] size: %d\n", i, psize[i]);

	mfc_debug_leave();

	return 0;
}

static void mfc_enc_unlock(struct vb2_queue *q)
{
	struct mfc_ctx *ctx = q->drv_priv;
	struct mfc_dev *dev = ctx->dev;

	mutex_unlock(&dev->mfc_mutex);
}

static void mfc_enc_lock(struct vb2_queue *q)
{
	struct mfc_ctx *ctx = q->drv_priv;
	struct mfc_dev *dev = ctx->dev;

	mutex_lock(&dev->mfc_mutex);
}

static int mfc_enc_buf_init(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct mfc_ctx *ctx = vq->drv_priv;
	int ret;

	mfc_debug_enter();

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = mfc_check_vb_with_fmt(ctx->dst_fmt, vb);
		if (ret < 0)
			return ret;

		if (call_cop(ctx, init_buf_ctrls, ctx, MFC_CTRL_TYPE_DST,
					vb->index) < 0)
			mfc_err_ctx("failed in init_buf_ctrls\n");

	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		ret = mfc_check_vb_with_fmt(ctx->src_fmt, vb);
		if (ret < 0)
			return ret;

		if (call_cop(ctx, init_buf_ctrls, ctx, MFC_CTRL_TYPE_SRC,
					vb->index) < 0)
			mfc_err_ctx("failed in init_buf_ctrls\n");
	} else {
		mfc_err_ctx("inavlid queue type: %d\n", vq->type);
		return -EINVAL;
	}

	mfc_debug_leave();

	return 0;
}

static int mfc_enc_buf_prepare(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct mfc_ctx *ctx = vq->drv_priv;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_raw_info *raw;
	unsigned int index = vb->index;
	struct mfc_buf *buf = vb_to_mfc_buf(vb);
	struct dma_buf *bufcon_dmabuf[MFC_MAX_PLANES];
	int i, mem_get_count = 0;
	size_t buf_size;

	mfc_debug_enter();

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		buf_size = vb2_plane_size(vb, 0);
		mfc_debug(2, "[STREAM] vb size: %lu, calc size: %u\n",
			buf_size, enc->dst_buf_size);

		if (buf_size < enc->dst_buf_size) {
			mfc_err_ctx("[STREAM] size(%lu) is smaller than (%d)\n",
					buf_size, enc->dst_buf_size);
			return -EINVAL;
		}

		buf->addr[0][0] = mfc_mem_get_daddr_vb(vb, 0);
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		raw = &ctx->raw_buf;
		if (ctx->src_fmt->mem_planes == 1) {
			buf_size = vb2_plane_size(vb, 0);
			mfc_debug(2, "[FRAME] single plane vb size: %lu, calc size: %d\n",
					buf_size, raw->total_plane_size);
			if (buf_size < raw->total_plane_size) {
				mfc_err_ctx("[FRAME] single plane size(%lu) is smaller than (%d)\n",
						buf_size, raw->total_plane_size);
				return -EINVAL;
			}
		} else {
			for (i = 0; i < ctx->src_fmt->mem_planes; i++) {
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

		for (i = 0; i < ctx->src_fmt->mem_planes; i++) {
			bufcon_dmabuf[i] = dma_buf_get(vb->planes[i].m.fd);
			if (IS_ERR(bufcon_dmabuf[i])) {
				mfc_err_ctx("failed to get bufcon dmabuf\n");
				goto err_mem_put;
			}

			mem_get_count++;
			buf->num_bufs_in_batch = mfc_bufcon_get_buf_count(bufcon_dmabuf[i]);
			mfc_debug(3, "[BUFCON] num bufs in batch: %d\n", buf->num_bufs_in_batch);
			if (buf->num_bufs_in_batch == 0) {
				mfc_err_ctx("[BUFCON] bufs count couldn't be zero\n");
				goto err_mem_put;
			}

			if (buf->num_bufs_in_batch < 0)
				buf->num_bufs_in_batch = 0;

			if (!ctx->batch_mode && buf->num_bufs_in_batch > 0) {
				ctx->batch_mode = 1;
				mfc_debug(2, "[BUFCON] buffer batch mode enable\n");
			}

			if (buf->num_bufs_in_batch > 0) {
				if (mfc_bufcon_get_daddr(ctx, buf, bufcon_dmabuf[i], i)) {
					mfc_err_ctx("[BUFCON] failed to get daddr[%d] in buffer container\n", i);
					goto err_mem_put;
				}

				ctx->framerate = buf->num_valid_bufs * ENC_DEFAULT_CAM_CAPTURE_FPS;
				mfc_debug(3, "[BUFCON] framerate: %ld\n", ctx->framerate);

				dma_buf_put(bufcon_dmabuf[i]);
			} else {
				dma_addr_t start_raw;

				dma_buf_put(bufcon_dmabuf[i]);
				start_raw = mfc_mem_get_daddr_vb(vb, 0);
				if (start_raw == 0) {
					mfc_err_ctx("Plane mem not allocated\n");
					return -ENOMEM;
				}
				if (ctx->src_fmt->fourcc == V4L2_PIX_FMT_NV12N) {
					buf->addr[0][0] = start_raw;
					buf->addr[0][1] = NV12N_CBCR_BASE(start_raw,
							ctx->img_width,
							ctx->img_height);
				} else if (ctx->src_fmt->fourcc == V4L2_PIX_FMT_YUV420N) {
					buf->addr[0][0] = start_raw;
					buf->addr[0][1] = YUV420N_CB_BASE(start_raw,
							ctx->img_width,
							ctx->img_height);
					buf->addr[0][2] = YUV420N_CR_BASE(start_raw,
							ctx->img_width,
							ctx->img_height);
				} else {
					buf->addr[0][i] = mfc_mem_get_daddr_vb(vb, i);
				}
			}
		}

		if (call_cop(ctx, to_buf_ctrls, ctx, &ctx->src_ctrls[index]) < 0)
			mfc_err_ctx("failed in to_buf_ctrls\n");
	} else {
		mfc_err_ctx("inavlid queue type: %d\n", vq->type);
		return -EINVAL;
	}

	mfc_debug_leave();
	return 0;

err_mem_put:
	for (i = 0; i < mem_get_count; i++)
		dma_buf_put(bufcon_dmabuf[i]);

	return -ENOMEM;
}

static void mfc_enc_buf_finish(struct vb2_buffer *vb)
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

static void mfc_enc_buf_cleanup(struct vb2_buffer *vb)
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
		mfc_err_ctx("mfc_enc_buf_cleanup: unknown queue type\n");
	}

	mfc_debug_leave();
}

static int mfc_enc_start_streaming(struct vb2_queue *q, unsigned int count)
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

static void mfc_enc_stop_streaming(struct vb2_queue *q)
{
	struct mfc_ctx *ctx = q->drv_priv;
	struct mfc_dev *dev = ctx->dev;
	int index = 0;
	int ret = 0;

	mfc_info_ctx("enc stop_streaming is called, hwlock : %d, type : %d\n",
				test_bit(ctx->num, &dev->hwlock.bits), q->type);
	MFC_TRACE_CTX("** ENC streamoff(type:%d)\n", q->type);

	/* If a H/W operation is in progress, wait for it complete */
	if (need_to_wait_nal_abort(ctx)) {
		if (mfc_wait_for_done_ctx(ctx, MFC_REG_R2H_CMD_NAL_ABORT_RET)) {
			mfc_err_ctx("time out during nal abort\n");
			mfc_cleanup_work_bit_and_try_run(ctx);
		}
	}

	ret = mfc_get_hwlock_ctx(ctx);
	if (ret < 0) {
		mfc_err_ctx("Failed to get hwlock\n");
		return;
	}

	if (q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		mfc_cleanup_enc_dst_queue(ctx);

		while (index < MFC_MAX_BUFFERS) {
			index = find_next_bit(&ctx->dst_ctrls_avail,
					MFC_MAX_BUFFERS, index);
			if (index < MFC_MAX_BUFFERS)
				call_cop(ctx, reset_buf_ctrls, &ctx->dst_ctrls[index]);
			index++;
		}
	} else if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if (ctx->state == MFCINST_RUNNING || ctx->state == MFCINST_FINISHING) {
			mfc_change_state(ctx, MFCINST_FINISHING);
			mfc_set_bit(ctx->num, &dev->work_bits);

			while (ctx->state != MFCINST_FINISHED) {
				ret = mfc_just_run(dev, ctx->num);
				if (ret) {
					mfc_err_ctx("Failed to run MFC\n");
					break;
				}
				if (mfc_wait_for_done_ctx(ctx, MFC_REG_R2H_CMD_FRAME_DONE_RET)) {
					mfc_err_ctx("Waiting for LAST_SEQ timed out\n");
					break;
				}
			}
		}

		mfc_move_all_bufs(&ctx->buf_queue_lock, &ctx->src_buf_queue,
				&ctx->ref_buf_queue, MFC_QUEUE_ADD_BOTTOM);
		mfc_cleanup_enc_src_queue(ctx);

		while (index < MFC_MAX_BUFFERS) {
			index = find_next_bit(&ctx->src_ctrls_avail,
					MFC_MAX_BUFFERS, index);
			if (index < MFC_MAX_BUFFERS)
				call_cop(ctx, reset_buf_ctrls, &ctx->src_ctrls[index]);
			index++;
		}
	}

	if (ctx->state == MFCINST_FINISHING)
		mfc_change_state(ctx, MFCINST_FINISHED);

	mfc_debug(2, "buffer cleanup is done in stop_streaming, type : %d\n", q->type);

	mfc_clear_bit(ctx->num, &dev->work_bits);
	mfc_release_hwlock_ctx(ctx);

	if (mfc_is_work_to_do(dev))
		queue_work(dev->butler_wq, &dev->butler_work);
}

static void mfc_enc_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct mfc_ctx *ctx = vq->drv_priv;
	struct mfc_dev *dev = ctx->dev;
	struct mfc_buf *buf = vb_to_mfc_buf(vb);
	int i;

	mfc_debug_enter();

	buf->next_index = 0;
	buf->done_index = 0;

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		mfc_debug(2, "[BUFINFO] ctx[%d] add dst index: %d, addr: 0x%08llx\n",
				ctx->num, vb->index, buf->addr[0][0]);

		/* Mark destination as available for use by MFC */
		mfc_add_tail_buf(&ctx->buf_queue_lock, &ctx->dst_buf_queue, buf);
		mfc_qos_update_framerate(ctx, 0, 1);
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		for (i = 0; i < ctx->src_fmt->mem_planes; i++)
			mfc_debug(2, "[BUFINFO] ctx[%d] add src index: %d, addr[%d]: 0x%08llx\n",
					ctx->num, vb->index, i, buf->addr[0][i]);
		mfc_add_tail_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, buf);

		if (debug_ts == 1)
			mfc_info_ctx("[TS] framerate: %ld, timestamp: %lld\n",
					ctx->framerate, buf->vb.vb2_buf.timestamp);

		mfc_qos_update_last_framerate(ctx, buf->vb.vb2_buf.timestamp);
		mfc_qos_update_framerate(ctx, 0, 0);
	} else {
		mfc_err_ctx("unsupported buffer type (%d)\n", vq->type);
	}

	if (mfc_ctx_ready(ctx))
		mfc_set_bit(ctx->num, &dev->work_bits);

	mfc_try_run(dev);

	mfc_debug_leave();
}

struct vb2_ops mfc_enc_qops = {
	.queue_setup		= mfc_enc_queue_setup,
	.wait_prepare		= mfc_enc_unlock,
	.wait_finish		= mfc_enc_lock,
	.buf_init		= mfc_enc_buf_init,
	.buf_prepare		= mfc_enc_buf_prepare,
	.buf_finish		= mfc_enc_buf_finish,
	.buf_cleanup		= mfc_enc_buf_cleanup,
	.start_streaming	= mfc_enc_start_streaming,
	.stop_streaming		= mfc_enc_stop_streaming,
	.buf_queue		= mfc_enc_buf_queue,
};
