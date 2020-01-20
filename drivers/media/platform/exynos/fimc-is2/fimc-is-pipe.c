 /*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is pipe functions
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifdef ENABLE_BUFFER_HIDING

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/bug.h>

#include "fimc-is-pipe.h"
#include "fimc-is-core.h"
#include "fimc-is-video.h"
#include "fimc-is-framemgr.h"

int fimc_is_pipe_probe(struct fimc_is_pipe *pipe)
{
	int ret = 0;
	int i = 0;

	pipe->src = NULL;
	pipe->dst = NULL;

	for (i = 0; i < PIPE_SLOT_MAX; i++)
		pipe->vctx[i] = NULL;

	return ret;
}

/*
 * This function must be called in case of FIMC_IS_GROUP_PIPE_INPUT.
 */
static int fimc_is_cancel_all_cap_nodes(struct fimc_is_device_ischain *device,
	struct fimc_is_pipe *pipe,
	struct fimc_is_video_ctx  *src_vctx,
	struct fimc_is_video_ctx  *junction_vctx,
	struct fimc_is_framemgr *src_framemgr,
	struct fimc_is_frame *src_frame)
{
	int ret = 0;
	int capture_id;
	unsigned long flags;
	struct camera2_node *cap_node;
	struct fimc_is_subdev *junction;
	struct fimc_is_subdev *subdev;
	struct fimc_is_video_ctx  *sub_vctx;
	struct fimc_is_framemgr *sub_framemgr;
	struct fimc_is_frame *sub_frame;

	junction = (struct fimc_is_subdev *)junction_vctx->subdev;

	for (capture_id = 0; capture_id < CAPTURE_NODE_MAX; ++capture_id) {
		cap_node = &src_frame->shot_ext->node_group.capture[capture_id];

		if (!cap_node || !cap_node->request ||
				cap_node->vid == junction->vid)
			continue;

		subdev = video2subdev(FIMC_IS_ISCHAIN_SUBDEV, (void *)device, cap_node->vid);
		if (!subdev) {
			mperr("video2subdev is fail", device, pipe, src_vctx->video);
			continue;
		}

		sub_vctx = subdev->vctx;
		if (!sub_vctx) {
			mperr("sub_vctx is null", device, pipe, src_vctx->video);
			ret = -EINVAL;
			continue;
		}

		sub_framemgr = GET_SUBDEV_FRAMEMGR(subdev);

		framemgr_e_barrier_irqs(sub_framemgr, FMGR_IDX_28, flags);

		/* cancel "request" buffer not "process" buffer */
		sub_frame = peek_frame(sub_framemgr, FS_REQUEST);
		if (sub_frame) {
			sub_frame->stream->fcount = src_frame->fcount;
			sub_frame->stream->fvalid = 0;

			trans_frame(sub_framemgr, sub_frame, FS_COMPLETE);
			CALL_VOPS(sub_vctx, done, sub_frame->index, VB2_BUF_STATE_ERROR);
			mpwarn("[F%d] CANCEL(%d)", device, pipe, sub_vctx->video,
				sub_frame->fcount, sub_frame->index);
		} else {
			mperr("subframe(%d) is NULL..", device, pipe, src_vctx->video, cap_node->vid);
		}

		framemgr_x_barrier_irqr(sub_framemgr, FMGR_IDX_28, flags);
	}

	return ret;
}

/*
 * This function perform the same dqbuf.
 * But it gurantees that the v4l2_buffer's value shouldn't be touched.
 * Also this function acquire the video_device mutex lock instead of v4l2 framework.
 * And it return the buf index.
 */
static int fimc_is_pipe_dqbuf_proxy(struct fimc_is_video_ctx *vctx,
	struct v4l2_buffer *buf,
	bool blocking)
{
	struct v4l2_buffer v4l2_buf;
	struct v4l2_plane planes[VIDEO_MAX_PLANES];
	struct fimc_is_video *video;
	int ret;

	mvdbgs(3, "%s\n", vctx, &vctx->queue, __func__);

	memset(&v4l2_buf, 0, sizeof(struct v4l2_buffer));
	memset(planes, 0, sizeof(struct v4l2_plane) * VIDEO_MAX_PLANES);

	v4l2_buf.type           = buf->type;
	v4l2_buf.memory         = buf->memory;
	v4l2_buf.length         = buf->length;
	v4l2_buf.m.planes       = planes;

	video = vctx->video;
	ret = mutex_lock_interruptible(&video->lock);
	if (ret) {
		mverr("mutex_lock_interruptible is fail(%d)", vctx, video, ret);
		return ret;
	}

	ret = fimc_is_video_dqbuf(vctx, &v4l2_buf, blocking);
	if (ret) {
		err("fimc_is_video_dqbuf is fail(%d)", ret);
		mutex_unlock(&video->lock);
		return ret;
	}

	mutex_unlock(&video->lock);

	mvdbgs(3, "%s done(%d)\n", vctx, &vctx->queue, __func__, v4l2_buf.index);

	return v4l2_buf.index;
}

/*
 * This function perform the same dqbuf.
 * this function acquire the video_device mutex lock instead of v4l2 framework.
 */
static int fimc_is_pipe_qbuf_proxy(struct fimc_is_video_ctx *vctx,
	struct v4l2_buffer *buf)
{
	int ret;
	struct fimc_is_video *video;

	mvdbgs(3, "%s(%d)\n", vctx, &vctx->queue, __func__, buf->index);

	video = vctx->video;
	ret = mutex_lock_interruptible(&video->lock);
	if (ret) {
		mverr("mutex_lock_interruptible is fail(%d)", vctx, video, ret);
		return ret;
	}

	ret = fimc_is_video_qbuf(vctx, buf);
	if (ret)
		err("fimc_is_video_qbuf is fail(%d)", ret);

	mutex_unlock(&video->lock);

	return ret;
}

static int fimc_is_pipe_callback(struct fimc_is_device_ischain *device,
	struct fimc_is_group *group,
	struct fimc_is_frame *frame)
{
	int ret = 0;
	u32 timeout = 3000;
	unsigned long flags;
	u32 capture_id, index;
	struct fimc_is_pipe *pipe;
	struct fimc_is_framemgr *src_framemgr, *junction_framemgr, *dst_framemgr;
	struct fimc_is_frame *junction_frame = NULL;
	struct fimc_is_frame *dst_frame = NULL;
	struct fimc_is_video_ctx *src_vctx, *junction_vctx, *dst_vctx;
	struct fimc_is_subdev *junction;
	struct camera2_node *node;
	struct fimc_is_queue *queue;

	FIMC_BUG(!device);
	FIMC_BUG(!group);
	FIMC_BUG(!group->leader.vctx);

	if (!test_bit(FIMC_IS_GROUP_PIPE_OUTPUT, &group->state))
		goto p_err;

	mvdbgs(3, "%s(%d)\n", group->leader.vctx, &group->leader.vctx->queue, __func__, frame->index);

	pipe = &device->pipe;

	/*
	 * junction pipe qbuf before src pipe shot
	 * In case of PIPE_INPUT, HAL does not call q operation for junction.
	 * HAL must call prepare_buf for source video node!!
	 */
	junction_vctx = pipe->vctx[PIPE_SLOT_JUNCTION];
	junction_framemgr = GET_FRAMEMGR(junction_vctx);
	if (!junction_framemgr) {
		merr("junction_framemgr is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}
	junction = (struct fimc_is_subdev *)junction_vctx->subdev;

	src_vctx = pipe->vctx[PIPE_SLOT_SRC];
	src_framemgr = GET_FRAMEMGR(src_vctx);
	if (!src_framemgr) {
		merr("src_framemgr is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	dst_vctx = pipe->vctx[PIPE_SLOT_DST];
	dst_framemgr = GET_FRAMEMGR(dst_vctx);
	if (!dst_framemgr) {
		merr("dst_framemgr is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	for (capture_id = 0; capture_id < CAPTURE_NODE_MAX; ++capture_id) {
		node = &frame->shot_ext->node_group.capture[capture_id];

		if (junction->vid == node->vid) {
			/* eanble request for junction ip of pipe source group */
			node->request = 1;
			break;
		}
	}

	if (capture_id == CAPTURE_NODE_MAX) {
		mperr("can't find vid(%d)", device, pipe, junction_vctx->video, junction->vid);
		ret = -EINVAL;
		goto p_err;
	}

	/* get a free junction frame */
	framemgr_e_barrier_irqs(junction_framemgr, pipe->id, flags);
	junction_frame = peek_frame(junction_framemgr, FS_FREE);
	framemgr_x_barrier_irqr(junction_framemgr, pipe->id, flags);

	if (!junction_frame) {
		mpwarn("[F%d] no free junction frame", device, pipe, junction_vctx->video, frame->fcount);
		ret = -EINVAL;
		goto p_err;
	}

	index = junction_frame->index;
	dst_frame = &dst_framemgr->frames[index];

	/* wait for available same buffer using by dst */
	do {
		framemgr_e_barrier_irqs(dst_framemgr, pipe->id, flags);
		if (dst_frame->state == FS_REQUEST ||
				dst_frame->state == FS_PROCESS)
			junction_frame = NULL;
		else
			junction_frame = &junction_framemgr->frames[index];
		framemgr_x_barrier_irqr(dst_framemgr, pipe->id, flags);

		if (!junction_frame) {
			usleep_range(1000, 1000);
			timeout--;
		}
	} while(!junction_frame && timeout);

	if (!timeout) {
		mpwarn("[F%d] junction frame(%d) is being used in DST..",
				device, pipe, junction_vctx->video,
				dst_frame->fcount, index);

		/* cancel all capture node buffer */
		fimc_is_cancel_all_cap_nodes(device, pipe,
			src_vctx, junction_vctx, src_framemgr, frame);
		ret = -EINVAL;
		goto p_err;
	}

	/*
	 * Qbuf Destination
	 * We modified the videobuf's state to PREPARED from DEQUEUED
	 * to skip the unnecessary process(__buf_prepare) in this case.
	 * Also, without this setting, the problem that fimc_is_frame's device addr
	 * and vb2_queue's device addr are different can be happened.
	 */
	queue = GET_QUEUE(junction_vctx);
	if (queue && queue->vbq && queue->vbq->bufs[index])
		queue->vbq->bufs[index]->state = VB2_BUF_STATE_PREPARED;
	ret = CALL_VOPS(junction_vctx, qbuf, &pipe->buf[PIPE_SLOT_JUNCTION][index]);
	if (ret) {
		mperr("[F%d] fimc_is_video_qbuf is fail(%d)",
			device, pipe, junction_vctx->video, frame->fcount, ret);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
};

static int fimc_is_pipe_qbuf(struct fimc_is_video_ctx *vctx,
	struct v4l2_buffer *buf)
{
	int ret = 0;
	struct fimc_is_device_ischain *device;
	struct fimc_is_pipe *pipe;
	struct fimc_is_video_ctx  *src_vctx, *junction_vctx, *dst_vctx;
	bool qbuf_proxy = false;

	FIMC_BUG(!vctx);
	FIMC_BUG(!vctx->video);

	mvdbgs(3, "%s(%d)\n", vctx, &vctx->queue, __func__, buf->index);

	device = GET_DEVICE(vctx);
	pipe = &device->pipe;

	src_vctx = pipe->vctx[PIPE_SLOT_SRC];
	if (!src_vctx || !src_vctx->video) {
		mperr("src_vctx or video is null", device, pipe, vctx->video);
		ret = -EINVAL;
		goto p_err;
	}

	junction_vctx = pipe->vctx[PIPE_SLOT_JUNCTION];
	if (!junction_vctx || !junction_vctx->video) {
		mperr("junction_vctx or video is null", device, pipe, vctx->video);
		ret = -EINVAL;
		goto p_err;
	}

	dst_vctx = pipe->vctx[PIPE_SLOT_DST];
	if (!dst_vctx || !dst_vctx->video) {
		mperr("dst_vctx or video is null", device, pipe, vctx->video);
		ret = -EINVAL;
		goto p_err;
	}

	if (test_bit(FIMC_IS_GROUP_PIPE_INPUT, &pipe->dst->state)) {
		if (junction_vctx->video->id == vctx->video->id ||
			dst_vctx->video->id == vctx->video->id) {
			qbuf_proxy = true;
		}
	} else {
		if (dst_vctx->video->id == vctx->video->id)
			qbuf_proxy = true;
	}

	if (qbuf_proxy)
		ret = fimc_is_pipe_qbuf_proxy(vctx, buf);
	else
		ret = fimc_is_video_qbuf(vctx, buf);

	if (ret)
		mperr("fimc_is_video_qbuf(%d) is fail(%d)", device, pipe, vctx->video, buf->index, ret);

p_err:
	return ret;
}

static int fimc_is_pipe_dqbuf(struct fimc_is_video_ctx *vctx,
	struct v4l2_buffer *buf,
	bool blocking)
{
	int ret = 0;
	unsigned long flags;
	int index, src_findex, capture_id;

	struct fimc_is_device_ischain *device;
	struct fimc_is_pipe *pipe;
	struct fimc_is_framemgr *src_framemgr, *junction_framemgr, *dst_framemgr;
	struct fimc_is_frame *src_frame, *junction_frame, *dst_frame;
	struct fimc_is_video_ctx  *src_vctx, *junction_vctx, *dst_vctx;
	struct camera2_node *dst_node, *cap_node;
	struct fimc_is_queue *queue;

	FIMC_BUG(!vctx);
	FIMC_BUG(!vctx->video);

	mvdbgs(3, "%s\n", vctx, &vctx->queue, __func__);

	device = GET_DEVICE(vctx);
	pipe = &device->pipe;
	src_vctx = pipe->vctx[PIPE_SLOT_SRC];
	if (!src_vctx || !src_vctx->video) {
		mperr("src_vctx or video is null", device, pipe, vctx->video);
		ret = -EINVAL;
		goto pipe_pass;
	}

	junction_vctx = pipe->vctx[PIPE_SLOT_JUNCTION];
	if (!junction_vctx || !junction_vctx->video) {
		mperr("junction_vctx or video is null", device, pipe, vctx->video);
		ret = -EINVAL;
		goto pipe_pass;
	}

	dst_vctx = pipe->vctx[PIPE_SLOT_DST];
	if (!dst_vctx || !dst_vctx->video) {
		mperr("dst_vctx or video is null", device, pipe, vctx->video);
		ret = -EINVAL;
		goto pipe_pass;
	}

	if (src_vctx->video->id == vctx->video->id) {
		/* Source */
		ret = fimc_is_video_dqbuf(src_vctx, buf, blocking);
		src_framemgr = GET_FRAMEMGR(src_vctx);
		dst_framemgr = GET_FRAMEMGR(dst_vctx);
		src_frame = &src_framemgr->frames[buf->index];

		if (ret) {
			mperr("fimc_is_video_dqbuf is fail(%d)", device, pipe, vctx->video, ret);
		} else if (!(buf->flags & V4L2_BUF_FLAG_ERROR)) {
			/*
			 * In case of giving a feedback(dynamic meta) from dst to src,
			 * the integrity has to be guaranteed with a spinlock.
			 * For this process, dst's framemgr spinlock can be used.
			 */
			framemgr_e_barrier_irqs(dst_framemgr, pipe->id, flags);
			memcpy(&src_frame->shot->dm.stats, &pipe->pipe_dm.stats, sizeof(struct camera2_stats_dm));
			framemgr_x_barrier_irqr(dst_framemgr, pipe->id, flags);
		}

		/*
		 * junction dqbuf after buffer done of junction
		 * In case of PIPE_INPUT, HAL does not call dq operation for junction
		 */
		if (test_bit(FIMC_IS_GROUP_PIPE_INPUT, &pipe->dst->state)) {
			int tmp_ret = 0;

			/* error handling, set all capture node to error buffer done */
			if (buf->flags & V4L2_BUF_FLAG_ERROR) {
				mperr("cancel capture nodes(%d)",
						device, pipe, vctx->video, buf->index);
				/* cancel all capture node buffer */
				tmp_ret = fimc_is_cancel_all_cap_nodes(device, pipe,
						src_vctx, junction_vctx, src_framemgr, src_frame);
				if (tmp_ret) {
					mperr("can't cancel capture nodes(%d/%d)",
							device, pipe, vctx->video, buf->index, tmp_ret);
					goto pipe_pass;
				}
			}

			tmp_ret = CALL_VOPS(junction_vctx, dqbuf, &pipe->buf[PIPE_SLOT_JUNCTION][0], blocking);
			if (tmp_ret) {
				mperr("fimc_is_video_dqbuf is fail(%d)", device, pipe, vctx->video, tmp_ret);
				goto pipe_pass;
			}
		}
	} else if (junction_vctx->video->id == vctx->video->id) {
		/* Junction */
		if (test_bit(FIMC_IS_GROUP_PIPE_INPUT, &pipe->dst->state)) {
			index = fimc_is_pipe_dqbuf_proxy(junction_vctx, buf, blocking);
			if (index < 0) {
				ret = index;
				mperr("fimc_is_video_dqbuf is fail(%d)", device, pipe, vctx->video, ret);
				goto pipe_pass;
			}
		} else {
			ret = fimc_is_video_dqbuf(junction_vctx, buf, blocking);
			if (ret) {
				mperr("fimc_is_video_dqbuf is fail(%d)", device, pipe, vctx->video, ret);
				goto pipe_pass;
			}

			index = buf->index;
		}

		junction_framemgr = GET_FRAMEMGR(vctx);
		src_framemgr = GET_FRAMEMGR(src_vctx);
		dst_framemgr = GET_FRAMEMGR(dst_vctx);

		junction_frame = &junction_framemgr->frames[index];
		src_findex = junction_frame->stream->findex;
		src_frame = &src_framemgr->frames[src_findex];
		dst_frame = &dst_framemgr->frames[index];

		dst_node = &dst_frame->shot_ext->node_group.leader;

		/* frmae shot done check and pipe ip dqbuf */
		if (dst_frame->state == FS_COMPLETE) {
			ret = CALL_VOPS(dst_vctx, dqbuf, &pipe->buf[PIPE_SLOT_DST][0], blocking);
			if (ret < 0) {
				mperr("fimc_is_video_dqbuf is fail(%d)", device, pipe, vctx->video, ret);
				goto pipe_pass;
			}
		}

		/* pipe source group junction ip NDONE check */
		if (!junction_frame->stream->fvalid) {
			mperr("pipe source junction NDONE(%d, %d/%d)", device, pipe, vctx->video,
					src_findex, junction_frame->index, junction_frame->stream->fvalid);
			goto pipe_pass;
		}

		/* pipe ip shot meta copy */
		memcpy(dst_frame->shot_ext, src_frame->shot_ext, sizeof(struct camera2_shot_ext));

		/* pipe ip crop region update */
		for (capture_id = 0; capture_id < CAPTURE_NODE_MAX; ++capture_id) {
			cap_node = &src_frame->shot_ext->node_group.capture[capture_id];
			if (cap_node->vid == vctx->video->id) {
				dst_node->input.cropRegion[0] = cap_node->output.cropRegion[0];
				dst_node->input.cropRegion[1] = cap_node->output.cropRegion[1];
				dst_node->input.cropRegion[2] = cap_node->output.cropRegion[2];
				dst_node->input.cropRegion[3] = cap_node->output.cropRegion[3];
				break;
			}
		}

		if (capture_id == CAPTURE_NODE_MAX) {
			dst_node->input.cropRegion[0] = 0;
			dst_node->input.cropRegion[1] = 0;
			dst_node->input.cropRegion[2] = 0;
			dst_node->input.cropRegion[3] = 0;
			mperr("pipe junction capture id is invalid", device, pipe, vctx->video);
		}

		/*
		 * Qbuf Destination
		 * We modified the videobuf's state to PREPARED from DEQUEUED
		 * to skip the unnecessary process(__buf_prepare) in this case.
		 * Also, without this setting, the problem that fimc_is_frame's device addr
		 * and vb2_queue's device addr are different can be happened.
		 */
		queue = GET_QUEUE(dst_vctx);
		if (queue && queue->vbq && queue->vbq->bufs[index])
			queue->vbq->bufs[index]->state = VB2_BUF_STATE_PREPARED;

		ret = CALL_VOPS(dst_vctx, qbuf, &pipe->buf[PIPE_SLOT_DST][index]);
		if (ret)
			mperr("[F%d] fimc_is_video_qbuf is fail(%d)", device, pipe, vctx->video, index, ret);

	} else if (dst_vctx->video->id == vctx->video->id) {
		/* Destination */
		ret = fimc_is_pipe_dqbuf_proxy(dst_vctx, buf, blocking);
		if (ret < 0) {
			mperr("fimc_is_video_dqbuf is fail(%d)", device, pipe, vctx->video, ret);
			goto pipe_pass;
		}
	} else {
		mperr("There's no matched vctx for dqbuf", device, pipe, vctx->video);
		ret = -EINVAL;
		goto pipe_pass;
	}

pipe_pass:
	return ret;
}

static int fimc_is_pipe_done(struct fimc_is_video_ctx *vctx,
	u32 index, u32 state)
{
	int ret = 0;
	struct fimc_is_device_ischain *device;
	struct fimc_is_pipe *pipe;
	struct fimc_is_framemgr *dst_framemgr;
	struct fimc_is_frame *dst_frame;
	struct fimc_is_video_ctx *dst_vctx;

	FIMC_BUG(!vctx);
	FIMC_BUG(!vctx->video);

	mvdbgs(3, "%s(%d)\n", vctx, &vctx->queue, __func__, index);

	device = GET_DEVICE(vctx);
	pipe = &device->pipe;
	dst_vctx = pipe->vctx[PIPE_SLOT_DST];
	if (!dst_vctx || !dst_vctx->video) {
		mperr("dst_vctx or video is null", device, pipe, vctx->video);
		ret = -EINVAL;
		goto p_err;
	}

	if (dst_vctx->video->id == vctx->video->id) {
		dst_framemgr = GET_FRAMEMGR(dst_vctx);
		dst_frame = &dst_framemgr->frames[index];

		/* dm meta copy */
		memcpy(&pipe->pipe_dm, &dst_frame->shot->dm, sizeof(struct camera2_dm));
	}

	ret = fimc_is_video_buffer_done(vctx, index, state);
	if (ret)
		mperr("fimc_is_video_buffer_done is fail(%d)", device, pipe, vctx->video, ret);

p_err:
	return ret;
}

int fimc_is_pipe_create(struct fimc_is_pipe *pipe,
	struct fimc_is_group *src,
	struct fimc_is_group *dst)
{
	int ret = 0;
	int i = 0;

	FIMC_BUG(!pipe);
	FIMC_BUG(!src);
	FIMC_BUG(!src->leader.vctx);
	FIMC_BUG(!src->junction->vctx);
	FIMC_BUG(!dst);
	FIMC_BUG(!dst->leader.vctx);

	pipe->id = 0;
	pipe->src = src;
	pipe->dst = dst;
	src->pipe_shot_callback = fimc_is_pipe_callback;
	dst->pipe_shot_callback = fimc_is_pipe_callback;
	pipe->vctx[PIPE_SLOT_SRC] = src->leader.vctx;
	pipe->vctx[PIPE_SLOT_JUNCTION] = src->tail->junction->vctx;
	pipe->vctx[PIPE_SLOT_DST] = dst->leader.vctx;

	for (i = 0; i < PIPE_SLOT_MAX; i++) {
		pipe->vctx[i]->vops.qbuf = fimc_is_pipe_qbuf;
		pipe->vctx[i]->vops.dqbuf = fimc_is_pipe_dqbuf;
		pipe->vctx[i]->vops.done = fimc_is_pipe_done;

		memset(pipe->buf[i], 0x0, (sizeof(struct v4l2_buffer) * FIMC_IS_MAX_BUFS));
		memset(pipe->planes[i], 0x0, (sizeof(struct v4l2_plane) * FIMC_IS_MAX_BUFS * VIDEO_MAX_PLANES));
	}

	return ret;
}
#endif
