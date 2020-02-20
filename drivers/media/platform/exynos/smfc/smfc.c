/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * The main source file of Samsung Exynos SMFC Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/exynos_iovmm.h>

#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-sg.h>

#include "smfc.h"
#include "smfc-sync.h"

static atomic_t smfc_hwfc_state;
static wait_queue_head_t smfc_hwfc_sync_wq;
static wait_queue_head_t smfc_suspend_wq;

enum {
	SMFC_HWFC_STANDBY = 0,
	SMFC_HWFC_RUN,
	SMFC_HWFC_WAIT,
};

int exynos_smfc_wait_done(bool enable_hwfc)
{
	int prev, new;

	wait_event(smfc_hwfc_sync_wq,
		   atomic_read(&smfc_hwfc_state) < SMFC_HWFC_WAIT);

	prev = atomic_read(&smfc_hwfc_state);
	while (enable_hwfc && prev == SMFC_HWFC_RUN
			&& (new = atomic_cmpxchg((&smfc_hwfc_state),
					prev, SMFC_HWFC_WAIT)) != prev)
		prev = new;

	return 0;
}

static void __exynos_smfc_wakeup_done_waiters(struct smfc_ctx *ctx)
{
	if ((!!(ctx->flags & SMFC_CTX_COMPRESS)) && ctx->enable_hwfc) {
		atomic_set(&smfc_hwfc_state, SMFC_HWFC_STANDBY);
		wake_up(&smfc_hwfc_sync_wq);
	}
}

static irqreturn_t exynos_smfc_irq_handler(int irq, void *priv)
{
	struct smfc_dev *smfc = priv;
	struct smfc_ctx *ctx = v4l2_m2m_get_curr_priv(smfc->m2mdev);
	ktime_t ktime = ktime_get();
	enum vb2_buffer_state state = VB2_BUF_STATE_DONE;
	u32 streamsize = smfc_get_streamsize(smfc);
	u32 thumb_streamsize = smfc_get_2nd_streamsize(smfc);
	bool suspending = false;

	spin_lock(&smfc->flag_lock);

	if (!!(smfc->flags & SMFC_DEV_TIMEDOUT)) {
		/* The tieout handler does the rest */
		dev_err(smfc->dev, "Interrupt occurred after timed-out.\n");
		spin_unlock(&smfc->flag_lock);
		return IRQ_HANDLED;
	}

	if (!(smfc->flags & SMFC_DEV_RUNNING)) {
		smfc_dump_registers(smfc);
		BUG();
	}

	suspending = !!(smfc->flags & SMFC_DEV_SUSPENDING);
	if (!!(smfc->flags & SMFC_DEV_OTF_EMUMODE))
		del_timer(&smfc->timer);
	smfc->flags &= ~(SMFC_DEV_RUNNING | SMFC_DEV_OTF_EMUMODE);

	spin_unlock(&smfc->flag_lock);

	if (!smfc_hwstatus_okay(smfc, ctx)) {
		smfc_dump_registers(smfc);
		state = VB2_BUF_STATE_ERROR;
		smfc_hwconfigure_reset(smfc);
	}

	if (!IS_ERR(smfc->clk_gate)) {
		clk_disable(smfc->clk_gate);
		if (!IS_ERR(smfc->clk_gate2))
			clk_disable(smfc->clk_gate2);
	}

	pm_runtime_put(smfc->dev);

	/* ctx is NULL if streamoff is called before (de)compression finishes */
	if (ctx) {
		struct vb2_v4l2_buffer *vb;

		vb = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);
		if (vb) {
			if (!!(ctx->flags & SMFC_CTX_COMPRESS)) {
				vb2_set_plane_payload(&vb->vb2_buf,
						      0, streamsize);
				if (!!(ctx->flags & SMFC_CTX_B2B_COMPRESS))
					vb2_set_plane_payload(&vb->vb2_buf, 1,
							thumb_streamsize);
			}

			vb->reserved2 =
				(__u32)ktime_us_delta(ktime, ctx->ktime_beg);
			v4l2_m2m_buf_done(vb, state);
		}

		vb = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
		if (vb)
			v4l2_m2m_buf_done(vb, state);

		__exynos_smfc_wakeup_done_waiters(ctx);

		if (!suspending) {
			v4l2_m2m_job_finish(smfc->m2mdev, ctx->fh.m2m_ctx);
		} else {
			/*
			 * smfc_resume() is in charge of calling
			 * v4l2_m2m_job_finish() on resuming
			 * from Suspend To RAM
			 */
			spin_lock(&smfc->flag_lock);
			smfc->flags &= ~SMFC_DEV_SUSPENDING;
			spin_unlock(&smfc->flag_lock);

			wake_up(&smfc_suspend_wq);
		}
	} else {
		dev_err(smfc->dev, "Spurious interrupt on H/W JPEG occurred\n");
	}

	return IRQ_HANDLED;
}

static void smfc_timedout_handler(unsigned long arg)
{
	struct smfc_dev *smfc = (struct smfc_dev *)arg;
	struct smfc_ctx *ctx;
	unsigned long flags;
	bool suspending;

	spin_lock_irqsave(&smfc->flag_lock, flags);
	if (!(smfc->flags & SMFC_DEV_RUNNING)) {
		/* Interrupt is occurred before timer handler is called */
		spin_unlock_irqrestore(&smfc->flag_lock, flags);
		return;
	}

	/* timer is enabled only when HWFC is enabled */
	BUG_ON(!(smfc->flags & SMFC_DEV_OTF_EMUMODE));
	suspending = !!(smfc->flags & SMFC_DEV_SUSPENDING);
	smfc->flags |= SMFC_DEV_TIMEDOUT; /* indicate the timedout is handled */
	smfc->flags &= ~(SMFC_DEV_RUNNING | SMFC_DEV_OTF_EMUMODE);
	spin_unlock_irqrestore(&smfc->flag_lock, flags);

	dev_err(smfc->dev, "=== TIMED-OUT! (1 sec.) =========================");
	smfc_dump_registers(smfc);
	smfc_hwconfigure_reset(smfc);

	if (!IS_ERR(smfc->clk_gate)) {
		clk_disable(smfc->clk_gate);
		if (!IS_ERR(smfc->clk_gate2))
			clk_disable(smfc->clk_gate2);
	}

	pm_runtime_put(smfc->dev);

	ctx = v4l2_m2m_get_curr_priv(smfc->m2mdev);
	if (ctx) {
		v4l2_m2m_buf_done(v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx),
							VB2_BUF_STATE_ERROR);
		v4l2_m2m_buf_done(v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx),
							VB2_BUF_STATE_ERROR);

		__exynos_smfc_wakeup_done_waiters(ctx);

		if (!suspending) {
			v4l2_m2m_job_finish(smfc->m2mdev, ctx->fh.m2m_ctx);
		} else {
			spin_lock(&smfc->flag_lock);
			smfc->flags &= ~SMFC_DEV_SUSPENDING;
			spin_unlock(&smfc->flag_lock);

			wake_up(&smfc_suspend_wq);
		}
	}

	spin_lock_irqsave(&smfc->flag_lock, flags);
	/* finished timedout handling */
	smfc->flags &= ~SMFC_DEV_TIMEDOUT;
	spin_unlock_irqrestore(&smfc->flag_lock, flags);
}


static int smfc_vb2_queue_setup(struct vb2_queue *vq, unsigned int *num_buffers,
				unsigned int *num_planes, unsigned int sizes[],
				struct device *alloc_devs[])
{
	struct smfc_ctx *ctx = vb2_get_drv_priv(vq);

	if (!(ctx->flags & SMFC_CTX_COMPRESS) && (*num_buffers > 1)) {
		dev_info(ctx->smfc->dev,
			"Decompression does not allow >1 buffers\n");
		dev_info(ctx->smfc->dev, "forced buffer count to 1\n");
		*num_buffers = 1;
	}

	if (smfc_is_compressed_type(ctx, vq->type)) {
		/*
		 * SMFC is able to stop compression if the target buffer is not
		 * enough. Therefore, it is not required to configure larger
		 * buffer for compression.
		 */
		sizes[0] = PAGE_SIZE;
		*num_planes = 1;
		alloc_devs[0] = ctx->smfc->dev;
		if (!!(ctx->flags & SMFC_CTX_B2B_COMPRESS)) {
			sizes[1] = PAGE_SIZE;
			alloc_devs[1] = ctx->smfc->dev;
			(*num_planes)++;
		}
	} else {
		unsigned int i;
		*num_planes = ctx->img_fmt->num_buffers;
		for (i = 0; i < *num_planes; i++) {
			sizes[i] = ctx->width * ctx->height;
			sizes[i] = (sizes[i] * ctx->img_fmt->bpp_buf[i]) / 8;
			alloc_devs[i] = ctx->smfc->dev;
		}

		if (!!(ctx->flags & SMFC_CTX_B2B_COMPRESS)) {
			unsigned int idx;
			for (i = 0; i < *num_planes; i++) {
				idx = *num_planes + i;
				sizes[idx] = ctx->thumb_width;
				sizes[idx] *= ctx->thumb_height;
				sizes[idx] *= ctx->img_fmt->bpp_buf[i];
				sizes[idx] /= 8;
				alloc_devs[idx] = ctx->smfc->dev;
			}

			*num_planes *= 2;
		}
	}

	return 0;
}

static int smfc_vb2_buf_prepare(struct vb2_buffer *vb)
{
	struct smfc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	unsigned int i;
	bool full_clean = false;

	/* output buffers should have valid bytes_used */
	if (V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type)) {
		if (!!(ctx->flags & SMFC_CTX_COMPRESS)) {
			unsigned long payload = ctx->width * ctx->height;

			for (i = 0; i < ctx->img_fmt->num_buffers; i++) {
				unsigned long planebytes = payload;

				planebytes *= ctx->img_fmt->bpp_buf[i];
				planebytes /= 8;
				if (vb2_get_plane_payload(vb, i) < planebytes) {
					dev_err(ctx->smfc->dev,
					"Too small payload[%u]=%lu (req:%lu)\n",
					i, vb2_get_plane_payload(vb, i),
					planebytes);
					return -EINVAL;
				}
			}
		} else {
			/* buffer contains JPEG stream to decompress */
			int ret = smfc_parse_jpeg_header(ctx, vb);

			if (ret != 0)
				return ret;
		}
	} else {
		/*
		 * capture payload of compression is configured
		 * in exynos_smfc_irq_handler().
		 */
		if (!(ctx->flags & SMFC_CTX_COMPRESS)) {
			unsigned long payload = ctx->width * ctx->height;

			for (i = 0; i < ctx->img_fmt->num_buffers; i++) {
				unsigned long planebits = payload;

				planebits *= ctx->img_fmt->bpp_buf[i];
				vb2_set_plane_payload(vb, i, planebits / 8);
			}
		} else {
			/*
			 * capture buffer of compression should be fully
			 * invalidated
			 */
			full_clean = true;
		}
	}

	/*
	 * FIXME: develop how to maintain a part of buffer
	if (!(to_vb2_v4l2_buffer(vb)->flags & V4L2_BUF_FLAG_NO_CACHE_CLEAN))
		return (full_clean) ?
			vb2_ion_buf_prepare(vb) : vb2_ion_buf_prepare_exact(vb);
	 */
	return 0;
}

static void smfc_vb2_buf_finish(struct vb2_buffer *vb)
{
	/*
	 * FIXME: develop how to maintain a part of buffer
	if (!(vb->v4l2_buf.flags & V4L2_BUF_FLAG_NO_CACHE_INVALIDATE))
		vb2_ion_buf_finish_exact(vb);
	 */
}

static void smfc_vb2_buf_cleanup(struct vb2_buffer *vb)
{
	struct smfc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	if (!V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type) &&
			!!(ctx->flags & SMFC_CTX_COMPRESS)) {
		/*
		 * TODO: clean the APP segments written by CPU in front
		 * of the start of the JPEG stream to be written by H/W
		 * for the later use of this buffer.
		 */
	}
}

static void smfc_vb2_buf_queue(struct vb2_buffer *vb)
{
	struct smfc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	v4l2_m2m_buf_queue(ctx->fh.m2m_ctx, to_vb2_v4l2_buffer(vb));
}

static void smfc_vb2_stop_streaming(struct vb2_queue *vq)
{
	struct vb2_buffer *vb;
	struct smfc_ctx *ctx = vb2_get_drv_priv(vq);

	if (V4L2_TYPE_IS_OUTPUT(vq->type)) {
		while ((vb = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx)))
			vb2_buffer_done(vb, VB2_BUF_STATE_ERROR);
	} else {
		while ((vb = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx)))
			vb2_buffer_done(vb, VB2_BUF_STATE_ERROR);
	}

	vb2_wait_for_all_buffers(vq);
}

static int smfc_vb2_dma_sg_flags(struct vb2_buffer *vb)
{
	int flags = 0;
	struct vb2_queue *vq = vb->vb2_queue;

	/*
	 * There is no chance to clean CPU caches if HWFC is
	 * enabled because the compression starts before the
	 * image producer completes writing.
	 * Therefore, the image producer (MCSC) and the read DMA
	 * of JPEG/SMFC should access the memory with the same
	 * shareability attributes.
	 * MCSC does use shareable memory access for now. Let's make
	 * unshareable to the shared buffer with MSCS here.
	 */
	if (V4L2_TYPE_IS_OUTPUT(vq->type)) {
		struct smfc_ctx *ctx = vq->drv_priv;

		if (!!(ctx->flags & SMFC_CTX_COMPRESS) && ctx->enable_hwfc)
			flags = VB2_DMA_SG_MEMFLAG_IOMMU_UNCACHED;
	}

	return flags;
}

static struct vb2_ops smfc_vb2_ops = {
	.queue_setup	= smfc_vb2_queue_setup,
	.buf_prepare	= smfc_vb2_buf_prepare,
	.buf_finish	= smfc_vb2_buf_finish,
	.buf_cleanup	= smfc_vb2_buf_cleanup,
	.buf_queue	= smfc_vb2_buf_queue,
	.wait_finish	= vb2_ops_wait_finish,
	.wait_prepare	= vb2_ops_wait_prepare,
	.stop_streaming	= smfc_vb2_stop_streaming,
	.mem_flags	= smfc_vb2_dma_sg_flags,
};

static int smfc_queue_init(void *priv, struct vb2_queue *src_vq,
		      struct vb2_queue *dst_vq)
{
	struct smfc_ctx *ctx = priv;
	int ret;

	memset(src_vq, 0, sizeof(*src_vq));
	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	src_vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	src_vq->ops = &smfc_vb2_ops;
	src_vq->mem_ops = &vb2_dma_sg_memops;
	src_vq->drv_priv = ctx;
	src_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &ctx->smfc->video_device_mutex;

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	memset(dst_vq, 0, sizeof(*dst_vq));
	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dst_vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	dst_vq->ops = &smfc_vb2_ops;
	dst_vq->mem_ops = &vb2_dma_sg_memops;
	dst_vq->drv_priv = ctx;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &ctx->smfc->video_device_mutex;

	return vb2_queue_init(dst_vq);
}

static int exynos_smfc_open(struct file *filp)
{
	struct smfc_dev *smfc = video_drvdata(filp);
	struct smfc_ctx *ctx;
	int ret;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		dev_err(smfc->dev, "Failed to allocate smfc_ctx");
		return -ENOMEM;
	}

	ctx->smfc = smfc;

	v4l2_fh_init(&ctx->fh, smfc->videodev);

	ret = smfc_init_controls(smfc, &ctx->v4l2_ctrlhdlr);
	if (ret)
		goto err_control;

	ctx->fh.ctrl_handler = &ctx->v4l2_ctrlhdlr;

	ctx->fh.m2m_ctx = v4l2_m2m_ctx_init(smfc->m2mdev, ctx, smfc_queue_init);
	if (IS_ERR(ctx->fh.m2m_ctx)) {
		ret = PTR_ERR(ctx->fh.m2m_ctx);
		dev_err(smfc->dev, "Failed(%d) to init m2m_ctx\n", ret);
		goto err_m2m_ctx_init;
	}

	v4l2_fh_add(&ctx->fh);

	filp->private_data = &ctx->fh;

	if (!IS_ERR(smfc->clk_gate)) {
		ret = clk_prepare(smfc->clk_gate);
		if (!ret && !IS_ERR(smfc->clk_gate2))
			ret = clk_prepare(smfc->clk_gate2);
		if (ret) {
			clk_unprepare(smfc->clk_gate);
			dev_err(smfc->dev,
				"Failed(%d) to prepare gate clocks\n", ret);
			goto err_clk;
		}
	}

	/*
	 * default mode: compression
	 * default image format: YUYV
	 * default size: 16x8
	 * default thumbnail size: 0x0 (back2back comp. not enabled)
	 * default chroma subsampling for JPEG: YUV422
	 * default quality factor for compression: 96
	 * default quality factor of thumbnail for compression: 50
	 */
	ctx->img_fmt = SMFC_DEFAULT_OUTPUT_FORMAT;
	ctx->width = SMFC_MIN_WIDTH << ctx->img_fmt->chroma_hfactor;
	ctx->height = SMFC_MIN_HEIGHT << ctx->img_fmt->chroma_vfactor;
	ctx->chroma_hfactor = ctx->img_fmt->chroma_hfactor;
	ctx->chroma_vfactor = ctx->img_fmt->chroma_vfactor;
	ctx->flags |= SMFC_CTX_COMPRESS;
	ctx->quality_factor = 96;
	ctx->restart_interval = 0;
	ctx->thumb_quality_factor = 50;
	ctx->enable_hwfc = 0;

	return 0;
err_clk:
	v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);
err_m2m_ctx_init:
	v4l2_ctrl_handler_free(&ctx->v4l2_ctrlhdlr);
err_control:
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	kfree(ctx);
	return ret;
}

static int exynos_smfc_release(struct file *filp)
{
	struct smfc_ctx *ctx = v4l2_fh_to_smfc_ctx(filp->private_data);

	v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);
	v4l2_ctrl_handler_free(&ctx->v4l2_ctrlhdlr);
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);

	if (!IS_ERR(ctx->smfc->clk_gate)) {
		clk_unprepare(ctx->smfc->clk_gate);
		if (!IS_ERR(ctx->smfc->clk_gate2))
			clk_unprepare(ctx->smfc->clk_gate2);
	}

	kfree(ctx->quantizer_tables);
	kfree(ctx->huffman_tables);

	kfree(ctx);

	return 0;
}

static const struct v4l2_file_operations smfc_v4l2_fops = {
	.owner		= THIS_MODULE,
	.open		= exynos_smfc_open,
	.release	= exynos_smfc_release,
	.poll		= v4l2_m2m_fop_poll,
	.unlocked_ioctl	= video_ioctl2,
	.mmap		= v4l2_m2m_fop_mmap,
};

static bool smfc_check_hwfc_configuration(struct smfc_ctx *ctx, bool hwfc_en)
{
	const struct smfc_image_format *fmt = ctx->img_fmt;
	if (!(ctx->flags & SMFC_CTX_COMPRESS) || !hwfc_en)
		return true;

	if (fmt->chroma_hfactor == 2) {
		/* YUV422 1-plane */
		if ((fmt->chroma_vfactor == 1) && (fmt->num_planes == 1))
			return true;
		/* YUV420 2-plane */
		if ((fmt->chroma_vfactor == 2) && (fmt->num_planes == 2))
			return true;
	}

	dev_err(ctx->smfc->dev,
		"HWFC is only available with YUV420-2p and YUV422-1p\n");

	return false;
}

static void smfc_m2m_device_run(void *priv)
{
	struct smfc_ctx *ctx = priv;
	unsigned long flags;
	int ret;
	unsigned char chroma_hfactor = ctx->chroma_hfactor;
	unsigned char chroma_vfactor = ctx->chroma_vfactor;
	unsigned char restart_interval = ctx->restart_interval;
	unsigned char quality_factor = ctx->quality_factor;
	unsigned char thumb_quality_factor = ctx->thumb_quality_factor;
	unsigned char enable_hwfc = ctx->enable_hwfc;

	ret = in_irq() ? pm_runtime_get(ctx->smfc->dev) :
			 pm_runtime_get_sync(ctx->smfc->dev);
	if (ret < 0) {
		pr_err("Failed to enable power\n");
		goto err_pm;
	}

	if (!IS_ERR(ctx->smfc->clk_gate)) {
		ret = clk_enable(ctx->smfc->clk_gate);
		if (!ret && !IS_ERR(ctx->smfc->clk_gate2)) {
			ret = clk_enable(ctx->smfc->clk_gate2);
			if (ret)
				clk_disable(ctx->smfc->clk_gate);
		}
	}

	if (ret < 0) {
		dev_err(ctx->smfc->dev, "Failed to enable clocks\n");
		goto err_clk;
	}

	if (!smfc_check_hwfc_configuration(ctx, !!enable_hwfc))
		goto err_hwfc;

	smfc_hwconfigure_reset(ctx->smfc);

	if (!!(ctx->flags & SMFC_CTX_COMPRESS)) {
		smfc_hwconfigure_tables(ctx, quality_factor,
					ctx->ctrl_qtbl2->p_cur.p_u8);
		smfc_hwconfigure_image(ctx, chroma_hfactor, chroma_vfactor);
		if (!!(ctx->flags & SMFC_CTX_B2B_COMPRESS)) {
			smfc_hwconfigure_2nd_tables(ctx, thumb_quality_factor);
			smfc_hwconfigure_2nd_image(ctx, !!enable_hwfc);
		}
	} else {
		if ((ctx->stream_width != ctx->width) ||
				(ctx->stream_height != ctx->height)) {
			dev_err(ctx->smfc->dev,
				"Downscaling on decompression not allowed\n");
			/* It is okay to abort after reset */
			goto err_invalid_size;
		}

		smfc_hwconfigure_image(ctx,
				ctx->stream_hfactor, ctx->stream_vfactor);
		smfc_hwconfigure_tables_for_decompression(ctx);
	}

	spin_lock_irqsave(&ctx->smfc->flag_lock, flags);
	ctx->smfc->flags |= SMFC_DEV_RUNNING;
	if (!!enable_hwfc)
		ctx->smfc->flags |= SMFC_DEV_OTF_EMUMODE;
	spin_unlock_irqrestore(&ctx->smfc->flag_lock, flags);

	/*
	 * SMFC internal timer is unavailable if HWFC is enabled
	 * Therefore, S/W timer object is used to detect unexpected delay.
	 */
	if (!!enable_hwfc) {
		mod_timer(&ctx->smfc->timer, jiffies + HZ); /* 1 sec. */
		atomic_set(&smfc_hwfc_state, SMFC_HWFC_RUN);
	}

	ctx->ktime_beg = ktime_get();

	smfc_hwconfigure_start(ctx, restart_interval, !!enable_hwfc);

	return;
err_invalid_size:
err_hwfc:
	if (!IS_ERR(ctx->smfc->clk_gate)) {
		clk_disable(ctx->smfc->clk_gate);
		if (!IS_ERR(ctx->smfc->clk_gate2))
			clk_disable(ctx->smfc->clk_gate2);
	}
err_clk:
	pm_runtime_put(ctx->smfc->dev);
err_pm:
	v4l2_m2m_buf_done(
		v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx), VB2_BUF_STATE_ERROR);
	v4l2_m2m_buf_done(
		v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx), VB2_BUF_STATE_ERROR);
	/*
	 * It is safe to call v4l2_m2m_job_finish() here because .device_run()
	 * is called without any lock held
	 */
	v4l2_m2m_job_finish(ctx->smfc->m2mdev, ctx->fh.m2m_ctx);
}

static void smfc_m2m_job_abort(void *priv)
{
	/* nothing to do */
}

static struct v4l2_m2m_ops smfc_m2m_ops = {
	.device_run	= smfc_m2m_device_run,
	.job_abort	= smfc_m2m_job_abort,
};

static int smfc_init_v4l2(struct device *dev, struct smfc_dev *smfc)
{
	int ret;
	size_t str_len;

	strncpy(smfc->v4l2_dev.name, "exynos-hwjpeg",
		sizeof(smfc->v4l2_dev.name) - 1);
	smfc->v4l2_dev.name[sizeof(smfc->v4l2_dev.name) - 1] = '\0';

	ret = v4l2_device_register(dev, &smfc->v4l2_dev);
	if (ret) {
		dev_err(dev, "Failed to register v4l2 device\n");
		return ret;
	}

	smfc->videodev = video_device_alloc();
	if (!smfc->videodev) {
		dev_err(dev, "Failed to allocate video_device");
		ret = -ENOMEM;
		goto err_videodev_alloc;
	}

	str_len = sizeof(smfc->videodev->name);
	if (smfc->device_id < 0) {
		strncpy(smfc->videodev->name, MODULE_NAME, str_len);
		smfc->videodev->name[str_len - 1] = '\0';
	} else {
		scnprintf(smfc->videodev->name, str_len,
			  "%s.%d", MODULE_NAME, smfc->device_id);
	}

	mutex_init(&smfc->video_device_mutex);

	smfc->videodev->fops		= &smfc_v4l2_fops;
	smfc->videodev->ioctl_ops	= &smfc_v4l2_ioctl_ops;
	smfc->videodev->release		= video_device_release;
	smfc->videodev->lock		= &smfc->video_device_mutex;
	smfc->videodev->vfl_dir		= VFL_DIR_M2M;
	smfc->videodev->v4l2_dev	= &smfc->v4l2_dev;

	video_set_drvdata(smfc->videodev, smfc);

	smfc->m2mdev = v4l2_m2m_init(&smfc_m2m_ops);
	if (IS_ERR(smfc->m2mdev)) {
		ret = PTR_ERR(smfc->m2mdev);
		dev_err(dev, "Failed(%d) to create v4l2_m2m_device\n", ret);
		goto err_m2m_init;
	}

	/* TODO: promote the magic number 12 in public */
	ret = video_register_device(smfc->videodev, VFL_TYPE_GRABBER, 12);
	if (ret < 0) {
		dev_err(dev, "Failed(%d) to register video_device[%d]\n",
			ret, 12);
		goto err_register;
	}

	return 0;

err_register:
	v4l2_m2m_release(smfc->m2mdev);
err_m2m_init:
	video_device_release(smfc->videodev);
err_videodev_alloc:
	v4l2_device_unregister(&smfc->v4l2_dev);
	return ret;
}

static void smfc_deinit_v4l2(struct device *dev, struct smfc_dev *smfc)
{
	v4l2_m2m_release(smfc->m2mdev);
	video_device_release(smfc->videodev);
	v4l2_device_unregister(&smfc->v4l2_dev);
}

static int smfc_init_clock(struct device *dev, struct smfc_dev *smfc)
{
	smfc->clk_gate = devm_clk_get(dev, "gate");
	if (IS_ERR(smfc->clk_gate)) {
		if (PTR_ERR(smfc->clk_gate) != -ENOENT) {
			dev_err(dev, "Failed(%ld) to get 'gate' clock",
				PTR_ERR(smfc->clk_gate));
			return PTR_ERR(smfc->clk_gate);
		}

		dev_info(dev, "'gate' clock is note present\n");
		smfc->clk_gate2 = ERR_PTR(-ENOENT);
		return 0;
	}

	smfc->clk_gate2 = devm_clk_get(dev, "gate2");
	if (IS_ERR(smfc->clk_gate2) && (PTR_ERR(smfc->clk_gate2) != -ENOENT)) {
		dev_err(dev, "Failed(%ld) to get 'gate2' clock\n",
				PTR_ERR(smfc->clk_gate2));
		clk_put(smfc->clk_gate);
		return PTR_ERR(smfc->clk_gate2);
	}

	return 0;
}

static int smfc_find_hw_version(struct device *dev, struct smfc_dev *smfc)
{
	int ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		dev_err(dev, "Failed(%d) to get the local power\n", ret);
		return ret;
	}

	if (!IS_ERR(smfc->clk_gate)) {
		ret = clk_prepare_enable(smfc->clk_gate);
		if (!ret && !IS_ERR(smfc->clk_gate2))
			ret = clk_prepare_enable(smfc->clk_gate2);
		if (ret) {
			clk_disable_unprepare(smfc->clk_gate);
			dev_err(dev, "Failed(%d) to get gate clocks\n", ret);
			goto err_clk;
		}
	}

	if (ret >= 0) {
		smfc->hwver = readl(smfc->reg + REG_IP_VERSION_NUMBER);
		if (!IS_ERR(smfc->clk_gate)) {
			clk_disable_unprepare(smfc->clk_gate);
			if (!IS_ERR(smfc->clk_gate2))
				clk_disable_unprepare(smfc->clk_gate2);
		}
	}

err_clk:
	pm_runtime_put(dev);

	return ret;
}

static int __attribute__((unused)) smfc_iommu_fault_handler(
		struct iommu_domain *domain, struct device *dev,
		unsigned long fault_addr, int fault_flags, void *token)
{
	struct smfc_dev *smfc = token;

	if (smfc->flags & SMFC_DEV_RUNNING)
		smfc_dump_registers(smfc);

	return 0;
}

static const struct smfc_device_data smfc_8890_data = {
	.device_caps = V4L2_CAP_EXYNOS_JPEG_B2B_COMPRESSION
			| V4L2_CAP_EXYNOS_JPEG_HWFC
			| V4L2_CAP_EXYNOS_JPEG_NO_STREAMBASE_ALIGN
			| V4L2_CAP_EXYNOS_JPEG_NO_IMAGEBASE_ALIGN
			| V4L2_CAP_EXYNOS_JPEG_DECOMPRESSION,
	.burstlenth_bits = 4, /* 16 bytes: 1 burst */
};

static const struct smfc_device_data smfc_7870_data = {
	.device_caps = V4L2_CAP_EXYNOS_JPEG_B2B_COMPRESSION
			| V4L2_CAP_EXYNOS_JPEG_NO_STREAMBASE_ALIGN
			| V4L2_CAP_EXYNOS_JPEG_NO_IMAGEBASE_ALIGN
			| V4L2_CAP_EXYNOS_JPEG_DECOMPRESSION,
	.burstlenth_bits = 4, /* 16 bytes: 1 burst */
};

static const struct smfc_device_data smfc_7420_data = {
	.device_caps = V4L2_CAP_EXYNOS_JPEG_NO_STREAMBASE_ALIGN
			| V4L2_CAP_EXYNOS_JPEG_NO_IMAGEBASE_ALIGN
			| V4L2_CAP_EXYNOS_JPEG_DECOMPRESSION
			| V4L2_CAP_EXYNOS_JPEG_DOWNSCALING
			| V4L2_CAP_EXYNOS_JPEG_DECOMPRESSION_CROP,
	.burstlenth_bits = 5, /* 32 bytes: 2 bursts */
};

static const struct smfc_device_data smfc_3475_data = {
	.device_caps = V4L2_CAP_EXYNOS_JPEG_NO_STREAMBASE_ALIGN
			| V4L2_CAP_EXYNOS_JPEG_NO_IMAGEBASE_ALIGN,
	.burstlenth_bits = 5, /* 32 bytes: 2 bursts */
};

static const struct of_device_id exynos_smfc_match[] = {
	{
		.compatible = "samsung,exynos-jpeg",
		.data = &smfc_3475_data,
	}, {
		.compatible = "samsung,exynos8890-jpeg",
		.data = &smfc_8890_data,
	}, {
		.compatible = "samsung,exynos7870-jpeg",
		.data = &smfc_7870_data,
	}, {
		.compatible = "samsung,exynos7420-jpeg",
		.data = &smfc_7420_data,
	}, {
		.compatible = "samsung,exynos3475-jpeg",
		.data = &smfc_3475_data,
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_smfc_match);

static int exynos_smfc_probe(struct platform_device *pdev)
{
	struct smfc_dev *smfc;
	struct resource *res;
	const struct of_device_id *of_id;
	int ret;

	atomic_set(&smfc_hwfc_state, SMFC_HWFC_STANDBY);
	init_waitqueue_head(&smfc_hwfc_sync_wq);
	init_waitqueue_head(&smfc_suspend_wq);

	smfc = devm_kzalloc(&pdev->dev, sizeof(*smfc), GFP_KERNEL);
	if (!smfc) {
		dev_err(&pdev->dev, "Failed to get allocate drvdata");
		return -ENOMEM;
	}

	smfc->dev = &pdev->dev;

	dma_set_mask(&pdev->dev, DMA_BIT_MASK(36));

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	smfc->reg = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(smfc->reg))
		return PTR_ERR(smfc->reg);

	of_id = of_match_node(exynos_smfc_match, pdev->dev.of_node);
	smfc->devdata = of_id->data;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "Failed to get IRQ resource");
		return -ENOENT;
	}

	ret = devm_request_irq(&pdev->dev, res->start, exynos_smfc_irq_handler,
				0, pdev->name, smfc);
	if (ret) {
		dev_err(&pdev->dev, "Failed to install IRQ handler");
		return ret;
	}

	ret = smfc_init_clock(&pdev->dev, smfc);
	if (ret)
		return ret;

	smfc->device_id = of_alias_get_id(pdev->dev.of_node, "jpeg");
	if (smfc->device_id < 0) {
		dev_info(&pdev->dev,
			"device ID is not declared: unique device\n");
		smfc->device_id = -1;
	}

	pm_runtime_enable(&pdev->dev);

	if (!of_property_read_u32(pdev->dev.of_node, "smfc,int_qos_minlock",
				(u32 *)&smfc->qosreq_int_level)) {
		if (smfc->qosreq_int_level > 0) {
			pm_qos_add_request(&smfc->qosreq_int,
					PM_QOS_DEVICE_THROUGHPUT, 0);
			dev_info(&pdev->dev, "INT Min.Lock Freq. = %d\n",
					smfc->qosreq_int_level);
		} else {
			smfc->qosreq_int_level = 0;
		}
	}

	platform_set_drvdata(pdev, smfc);

	ret = smfc_init_v4l2(&pdev->dev, smfc);
	if (ret < 0)
		goto err_v4l2;

	iovmm_set_fault_handler(&pdev->dev, smfc_iommu_fault_handler, smfc);

	ret = iovmm_activate(&pdev->dev);
	if (ret < 0)
		goto err_iommu;

	setup_timer(&smfc->timer, smfc_timedout_handler, (unsigned long)smfc);

	ret = smfc_find_hw_version(&pdev->dev, smfc);
	if (ret < 0)
		goto err_hwver;

	spin_lock_init(&smfc->flag_lock);

	dev_info(&pdev->dev, "Probed H/W Version: %02x.%02x.%04x\n",
			(smfc->hwver >> 24) & 0xFF, (smfc->hwver >> 16) & 0xFF,
			smfc->hwver & 0xFFFF);
	return 0;

err_hwver:
	iovmm_deactivate(&pdev->dev);
err_iommu:
	smfc_deinit_v4l2(&pdev->dev, smfc);
err_v4l2:
	if (smfc->qosreq_int_level > 0)
		pm_qos_remove_request(&smfc->qosreq_int);

	return ret;
}

static void smfc_deinit_clock(struct smfc_dev *smfc)
{
	if (!IS_ERR(smfc->clk_gate2))
		clk_put(smfc->clk_gate2);
	if (!IS_ERR(smfc->clk_gate))
		clk_put(smfc->clk_gate);
}

static int exynos_smfc_remove(struct platform_device *pdev)
{
	struct smfc_dev *smfc = platform_get_drvdata(pdev);

	pm_qos_remove_request(&smfc->qosreq_int);
	smfc_deinit_clock(smfc);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int smfc_suspend(struct device *dev)
{
	struct smfc_dev *smfc = dev_get_drvdata(dev);
	unsigned long flags;

	spin_lock_irqsave(&smfc->flag_lock, flags);
	if (!!(smfc->flags & SMFC_DEV_RUNNING))
		smfc->flags |= SMFC_DEV_SUSPENDING;
	spin_unlock_irqrestore(&smfc->flag_lock, flags);

	/*
	 * SMFC_DEV_SUSPENDING is cleared by exynos_smfc_irq_handler()
	 * It is okay to read flags without a lock because the flag is not
	 * updated during reading the flag.
	 */
	wait_event(smfc_suspend_wq, !(smfc->flags & SMFC_DEV_SUSPENDING));

	/*
	 * It is guaranteed that the Runtime PM is suspended
	 * and all relavent clocks are disabled.
	 */

	return 0;
}

static int smfc_resume(struct device *dev)
{
	struct smfc_dev *smfc = dev_get_drvdata(dev);
	struct smfc_ctx *ctx = v4l2_m2m_get_curr_priv(smfc->m2mdev);

	/* completing the unfinished job and resuming the next pending jobs */
	if (ctx)
		v4l2_m2m_job_finish(smfc->m2mdev, ctx->fh.m2m_ctx);
	return 0;
}
#endif

#ifdef CONFIG_PM
static int smfc_runtime_resume(struct device *dev)
{
	struct smfc_dev *smfc = dev_get_drvdata(dev);

	if (smfc->qosreq_int_level > 0)
		pm_qos_update_request(&smfc->qosreq_int, smfc->qosreq_int_level);

	return 0;
}

static int smfc_runtime_suspend(struct device *dev)
{
	struct smfc_dev *smfc = dev_get_drvdata(dev);

	if (smfc->qosreq_int_level > 0)
		pm_qos_update_request(&smfc->qosreq_int, 0);

	return 0;
}
#endif

static void exynos_smfc_shutdown(struct platform_device *pdev)
{
	struct smfc_dev *smfc = platform_get_drvdata(pdev);
	unsigned long flags;

	spin_lock_irqsave(&smfc->flag_lock, flags);
	if (!!(smfc->flags & SMFC_DEV_RUNNING))
		smfc->flags |= SMFC_DEV_SUSPENDING;
	spin_unlock_irqrestore(&smfc->flag_lock, flags);
	wait_event(smfc_suspend_wq, !(smfc->flags & SMFC_DEV_SUSPENDING));

	iovmm_deactivate(&pdev->dev);
}

static const struct dev_pm_ops exynos_smfc_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(smfc_suspend, smfc_resume)
	SET_RUNTIME_PM_OPS(NULL, smfc_runtime_resume, smfc_runtime_suspend)
};

static struct platform_driver exynos_smfc_driver = {
	.probe		= exynos_smfc_probe,
	.remove		= exynos_smfc_remove,
	.shutdown	= exynos_smfc_shutdown,
	.driver = {
		.name	= MODULE_NAME,
		.owner	= THIS_MODULE,
		.pm	= &exynos_smfc_pm_ops,
		.of_match_table = of_match_ptr(exynos_smfc_match),
	}
};

module_platform_driver(exynos_smfc_driver);

MODULE_AUTHOR("Cho KyongHo <pullip.cho@samsung.com>");
MODULE_DESCRIPTION("Exynos Still MFC(JPEG) V4L2 Driver");
MODULE_LICENSE("GPL");
