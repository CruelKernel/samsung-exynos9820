/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/types.h>
#include <linux/slab.h>

#include "score_log.h"
#include "score_util.h"
#include "score_context.h"
#include "score_frame.h"

/**
 * score_frame_add_buffer - Add list of memory buffer to list of frame
 * @frame:	[in]	object about score_frame structure
 * @buf:	[in]	object about score_memory_buffer structure
 */
void score_frame_add_buffer(struct score_frame *frame,
		struct score_mmu_buffer *buf)
{
	score_enter();
	list_add_tail(&buf->frame_list, &frame->buffer_list);
	frame->buffer_count++;
	score_leave();
}

/**
 * score_frame_remove_buffer - Remove list of memory buffer at list of frame
 * @frame:	[in]	object about score_frame structure
 * @buf:	[in]	object about score_memory_buffer structure
 */
void score_frame_remove_buffer(struct score_frame *frame,
		struct score_mmu_buffer *buf)
{
	score_enter();
	list_del(&buf->frame_list);
	frame->buffer_count--;
	score_leave();
}

/**
 * score_frame_check_done - Check that task is done of frame
 * @frame:	[in]	object about score_frame structure
 *
 * Returns 1 if task is done of frame, otherwise 0
 */
int score_frame_check_done(struct score_frame *frame)
{
	return frame->state == SCORE_FRAME_STATE_COMPLETE;
}

void score_frame_done(struct score_frame *frame, int *ret)
{
	*ret = frame->ret;
}

static inline void __score_frame_set_ready(struct score_frame_manager *fmgr,
		struct score_frame *frame)
{
	score_enter();
	frame->state = SCORE_FRAME_STATE_READY;
	list_add_tail(&frame->state_list, &fmgr->ready_list);
	fmgr->ready_count++;
	score_leave();
}

static inline int __score_frame_del_ready(struct score_frame_manager *fmgr,
		struct score_frame *frame)
{
	score_enter();
	if (frame->state != SCORE_FRAME_STATE_READY) {
		score_warn("frame state is not ready(%u-%u)\n",
				frame->sctx->id, frame->frame_id);
		return -EINVAL;
	}

	if (!fmgr->ready_count) {
		score_warn("frame manager doesn't have ready frame(%u-%u)\n",
				frame->sctx->id, frame->frame_id);
		return -EINVAL;
	}

	list_del(&frame->state_list);
	fmgr->ready_count--;
	score_leave();

	return 0;
}

static inline void __score_frame_set_process(
		struct score_frame_manager *fmgr,
		struct score_frame *frame)
{
	score_enter();
	frame->state = SCORE_FRAME_STATE_PROCESS;
	list_add_tail(&frame->state_list, &fmgr->process_list);
	fmgr->process_count++;
	if (frame->priority)
		fmgr->process_high_count++;
	else
		fmgr->process_normal_count++;
	score_leave();
}

static inline int __score_frame_del_process(
		struct score_frame_manager *fmgr,
		struct score_frame *frame)
{
	score_enter();
	if (frame->state != SCORE_FRAME_STATE_PROCESS) {
		score_warn("frame state is not process(%u-%u)\n",
				frame->sctx->id, frame->frame_id);
		return -EINVAL;
	}

	if (!fmgr->process_count) {
		score_warn("frame manager doesn't have process frame(%u-%u)\n",
				frame->sctx->id, frame->frame_id);
		return -EINVAL;
	}

	list_del(&frame->state_list);
	fmgr->process_count--;
	if (frame->priority)
		fmgr->process_high_count--;
	else
		fmgr->process_normal_count--;

	score_leave();
	return 0;
}

static inline void __score_frame_set_pending(
		struct score_frame_manager *fmgr,
		struct score_frame *frame)
{
	score_enter();
	frame->state = SCORE_FRAME_STATE_PENDING;
	list_add_tail(&frame->state_list, &fmgr->pending_list);
	fmgr->pending_count++;
	if (frame->priority)
		fmgr->pending_high_count++;
	else
		fmgr->pending_normal_count++;
	score_leave();
}

static inline int __score_frame_del_pending(struct score_frame_manager *fmgr,
		struct score_frame *frame)
{
	score_enter();
	if (frame->state != SCORE_FRAME_STATE_PENDING) {
		score_warn("frame state is not pending(%u-%u)\n",
				frame->sctx->id, frame->frame_id);
		return -EINVAL;
	}

	if (!fmgr->pending_count) {
		score_warn("frame manager doesn't have pending frame(%u-%u)\n",
				frame->sctx->id, frame->frame_id);
		return -EINVAL;
	}

	list_del(&frame->state_list);
	fmgr->pending_count--;
	if (frame->priority)
		fmgr->pending_high_count--;
	else
		fmgr->pending_normal_count--;

	score_leave();
	return 0;
}

static inline void __score_frame_set_complete(
		struct score_frame_manager *fmgr,
		struct score_frame *frame, int result)
{
	score_enter();
	frame->state = SCORE_FRAME_STATE_COMPLETE;
	list_add_tail(&frame->state_list, &fmgr->complete_list);
	fmgr->complete_count++;
	frame->ret = result;
	score_leave();
}

static inline int __score_frame_del_complete(struct score_frame_manager *fmgr,
		struct score_frame *frame)
{
	score_enter();
	if (frame->state != SCORE_FRAME_STATE_COMPLETE) {
		score_warn("frame state is not complete(%u-%u)\n",
				frame->sctx->id, frame->frame_id);
		return -EINVAL;
	}

	if (!fmgr->complete_count) {
		score_warn("frame manager doesn't have complete frame(%u-%u)\n",
				frame->sctx->id, frame->frame_id);
		return -EINVAL;
	}

	list_del(&frame->state_list);
	fmgr->complete_count--;
	score_leave();

	return 0;
}

/**
 * score_frame_trans_ready_to_process -
 *	Translate state of frame from ready to process
 * @frame:      [in]	object about score_frame structure
 *
 * Return 0 if succeeded, otherwise errno
 */
int score_frame_trans_ready_to_process(struct score_frame *frame)
{
	int ret = 0;
	struct score_frame_manager *fmgr = frame->owner;

	score_enter();
	ret = __score_frame_del_ready(fmgr, frame);
	if (ret)
		goto p_err;

	__score_frame_set_process(fmgr, frame);

	score_leave();
	return 0;
p_err:
	return ret;
}

/**
 * score_frame_trans_ready_to_complete -
 *	Translate state of frame from ready to complete
 * @frame:	[in]	object about score_frame structure
 * @result:	[in]	result of task included in frame
 *
 * Return 0 if succeeded, otherwise errno
 */
int score_frame_trans_ready_to_complete(struct score_frame *frame, int result)
{
	int ret = 0;
	struct score_frame_manager *fmgr = frame->owner;

	score_enter();
	ret = __score_frame_del_ready(fmgr, frame);
	if (ret)
		goto p_err;

	__score_frame_set_complete(fmgr, frame, result);

	score_leave();
	return 0;
p_err:
	return ret;
}

/**
 * score_frame_trans_process_to_pending -
 *	Translate state of frame from process to pending
 * @frame:      [in]	object about score_frame structure
 *
 * Return 0 if succeeded, otherwise errno
 */
int score_frame_trans_process_to_pending(struct score_frame *frame)
{
	int ret = 0;
	struct score_frame_manager *fmgr = frame->owner;

	score_enter();
	ret = __score_frame_del_process(fmgr, frame);
	if (ret)
		goto p_err;

	__score_frame_set_pending(fmgr, frame);

	score_leave();
	return 0;
p_err:
	return ret;
}

/**
 * score_frame_trans_process_to_complete -
 *	Translate state of frame from process to complete
 * @frame:	[in]	object about score_frame structure
 * @result:	[in]	result of task included in frame
 *
 * Return 0 if succeeded, otherwise errno
 */
int score_frame_trans_process_to_complete(struct score_frame *frame, int result)
{
	int ret = 0;
	struct score_frame_manager *fmgr = frame->owner;

	score_enter();
	ret = __score_frame_del_process(fmgr, frame);
	if (ret)
		goto p_err;

	__score_frame_set_complete(fmgr, frame, result);

	score_leave();
	return 0;
p_err:
	return ret;
}

/**
 * score_frame_trans_pending_to_ready -
 *	Translate state of frame from pending to ready
 * @frame:      [in]	object about score_frame structure
 *
 * Return 0 if succeeded, otherwise errno
 */
int score_frame_trans_pending_to_ready(struct score_frame *frame)
{
	int ret = 0;
	struct score_frame_manager *fmgr = frame->owner;

	score_enter();
	ret = __score_frame_del_pending(fmgr, frame);
	if (ret)
		goto p_err;

	__score_frame_set_ready(fmgr, frame);

	score_leave();
	return 0;
p_err:
	return ret;
}

/**
 * score_frame_trans_pending_to_complete -
 *	Translate state of frame from pending to complete
 * @frame:	[in]	object about score_frame structure
 * @result:	[in]	result of task included in frame
 *
 * Return 0 if succeeded, otherwise errno
 */
int score_frame_trans_pending_to_complete(struct score_frame *frame, int result)
{
	int ret = 0;
	struct score_frame_manager *fmgr = frame->owner;

	score_enter();
	ret = __score_frame_del_pending(fmgr, frame);
	if (ret)
		goto p_err;

	__score_frame_set_complete(fmgr, frame, result);

	score_leave();
	return 0;
p_err:
	return ret;
}

/**
 * score_frame_get_process_by_id -
 *	Get frame that state is process and frame id is same with input id
 * @fmgr:	[in]	object about score_frame_manager structure
 * @id:		[in]	id value searched
 *
 * Returns frame address if succeeded, otherwise NULL
 */
struct score_frame *score_frame_get_process_by_id(
		struct score_frame_manager *fmgr, unsigned int id)
{
	struct score_frame *list_frame, *tframe;

	score_enter();
	list_for_each_entry_safe(list_frame, tframe, &fmgr->process_list,
			state_list) {
		if (list_frame->frame_id == id)
			return list_frame;
	}
	score_warn("fmgr doesn't have process frame that id is %u\n", id);
	score_leave();
	return NULL;
}

/**
 * score_frame_get_by_id -
 *	Get frame that frame id is same with input id
 * @fmgr:	[in]	object about score_frame_manager structure
 * @id:		[in]	id value searched
 *
 * Returns frame address if succeeded, otherwise NULL
 */
struct score_frame *score_frame_get_by_id(
		struct score_frame_manager *fmgr, unsigned int id)
{
	struct score_frame *list_frame, *tframe;

	score_enter();
	list_for_each_entry_safe(list_frame, tframe, &fmgr->entire_list,
			entire_list) {
		if (list_frame->frame_id == id)
			return list_frame;
	}
	score_leave();
	return NULL;
}

/**
 * score_frame_get_first_pending - Get first frame that state is pending
 * @fmgr:	[in]	object about score_frame_manager structure
 * @priority:	[in]	select priority high or normal
 *
 * Returns frame address if succeeded, otherwise NULL
 */

struct score_frame *score_frame_get_first_pending(
		struct score_frame_manager *fmgr, bool priority)
{
	struct score_frame *frame, *tframe;
	struct score_frame *pending_frame = NULL;

	score_enter();
	if (!fmgr->pending_count ||
			(priority && !fmgr->pending_high_count) ||
			(!priority && !fmgr->pending_normal_count)) {
		score_warn("fmar doesn't have pending frame (%u/%u/%u)\n",
				fmgr->pending_high_count,
				fmgr->pending_normal_count,
				fmgr->pending_count);
		return NULL;
	}

	if (fmgr->pending_count !=
			(fmgr->pending_high_count +
			 fmgr->pending_normal_count))
		score_warn("pending count is invalid (%u/%u/%u)\n",
				fmgr->pending_high_count,
				fmgr->pending_normal_count,
				fmgr->pending_count);

	list_for_each_entry_safe(frame, tframe, &fmgr->pending_list,
			state_list) {
		if (frame->priority == priority) {
			pending_frame = frame;
			break;
		}
	}

	score_leave();
	return pending_frame;
}

static int __score_frame_destroy_state(struct score_frame_manager *fmgr,
		struct score_frame *frame)
{
	int ret = 0;

	score_enter();
	switch (frame->state) {
	case SCORE_FRAME_STATE_READY:
		ret = __score_frame_del_ready(fmgr, frame);
		break;
	case SCORE_FRAME_STATE_PROCESS:
		ret = __score_frame_del_process(fmgr, frame);
		break;
	case SCORE_FRAME_STATE_PENDING:
		ret = __score_frame_del_pending(fmgr, frame);
		break;
	case SCORE_FRAME_STATE_COMPLETE:
		ret = __score_frame_del_complete(fmgr, frame);
		break;
	default:
		ret = -EINVAL;
		score_warn("frame state is invalid (%u-%u)\n",
				frame->sctx->id, frame->frame_id);
		goto p_err;
	}
	score_leave();
p_err:
	return ret;
}

/**
 * score_frame_trans_any_to_complete -
 *	Translate state of frame to complete regardless of state
 * @frame:	[in]	object about score_frame structure
 * @result:	[in]	result of task included in frame
 *
 * Return 0 if succeeded, otherwise errno
 */
int score_frame_trans_any_to_complete(struct score_frame *frame, int result)
{
	int ret = 0;
	struct score_frame_manager *fmgr = frame->owner;

	score_enter();
	if (frame->state != SCORE_FRAME_STATE_COMPLETE) {
		score_warn("Frame(%d, %u-%u, %u) is completed by force(%d)\n",
				frame->state, frame->sctx->id, frame->frame_id,
				frame->kernel_id, result);
		ret = __score_frame_destroy_state(fmgr, frame);
		__score_frame_set_complete(fmgr, frame, result);
	}

	score_leave();
	return ret;
}

/**
 * score_frame_get_state - Get state of frame
 * @frame:      [in]	object about score_frame structure
 *
 * Return state of frame
 */
unsigned int score_frame_get_state(struct score_frame *frame)
{
	return frame->state;
}

/**
 * score_frame_check_type - Check what type is
 * @frame:      [in]	object about score_frame structure
 *
 * Return true if frame type is the same
 */
bool score_frame_check_type(struct score_frame *frame, int type)
{
	return frame->type == type;
}

/**
 * score_frame_set_type_block - Set frame to blocking
 * @frame:      [in]	object about score_frame structure
 */
void score_frame_set_type_block(struct score_frame *frame)
{
	frame->type = TYPE_BLOCK;
}

/**
 * score_frame_set_type_remove - Set frame to nonblock-remove
 * @frame:      [in]	object about score_frame structure
 */
void score_frame_set_type_remove(struct score_frame *frame)
{
	frame->type = TYPE_NONBLOCK_REMOVE;
}

/**
 * score_frame_set_type_destroy - Set frame to nonblock-destroy
 * @frame:      [in]	object about score_frame structure
 */
void score_frame_set_type_destroy(struct score_frame *frame)
{
	frame->type = TYPE_NONBLOCK_DESTROY;
}

/**
 * score_frame_get_pending_count - Get count of pending frame
 * @fmgr:	[in]	object about score_frame_manager structure
 * @priority:	[in]	select priority high or normal
 *
 * Return count of pending frame
 */
unsigned int score_frame_get_pending_count(struct score_frame_manager *fmgr,
		bool priority)
{
	if (priority)
		return fmgr->pending_high_count;
	else
		return fmgr->pending_normal_count;
}

/**
 * score_frame_flush_process - Transfer all process frame to complete
 * @fmgr:	[in]	object about score_frame_manager structure
 * @result:	[in]	result of task included in frame
 */
void score_frame_flush_process(struct score_frame_manager *fmgr, int result)
{
	struct score_frame *list_frame, *tframe;

	score_enter();
	list_for_each_entry_safe(list_frame, tframe, &fmgr->process_list,
			state_list) {
		score_warn("Process frame(%u-%u, %u) is flushed (%d)\n",
				list_frame->sctx->id, list_frame->frame_id,
				list_frame->kernel_id, result);
		score_frame_trans_process_to_complete(list_frame, result);
	}
	score_leave();
}

/**
 * score_frame_flush_all - Transfer all frame to complete
 * @fmgr:	[in]	object about score_frame_manager structure
 * @result:	[in]	result of task included in frame
 */
void score_frame_flush_all(struct score_frame_manager *fmgr, int result)
{
	struct score_frame *list_frame, *tframe;

	score_enter();
	list_for_each_entry_safe(list_frame, tframe, &fmgr->entire_list,
			entire_list) {
		if (list_frame->state != SCORE_FRAME_STATE_COMPLETE) {
			score_warn("Frame(%d, %u-%u, %u) is flushed (%d)\n",
					list_frame->state, list_frame->sctx->id,
					list_frame->frame_id,
					list_frame->kernel_id, result);
			score_frame_trans_any_to_complete(list_frame, result);
		}
	}
	score_leave();
}

/**
 * score_frame_remove_nonblock_all - Remove all frame that type is nonblock
 * @fmgr:	[in]	object about score_frame_manager structure
 */
void score_frame_remove_nonblock_all(struct score_frame_manager *fmgr)
{
	struct score_frame *list_frame, *tframe;
	unsigned long flags;

	score_enter();
	spin_lock_irqsave(&fmgr->slock, flags);
	list_for_each_entry_safe(list_frame, tframe, &fmgr->entire_list,
			entire_list) {
		if (score_frame_check_type(list_frame, TYPE_NONBLOCK_NOWAIT) ||
				score_frame_check_type(list_frame,
					TYPE_NONBLOCK)) {
			score_frame_set_type_destroy(list_frame);
			spin_unlock_irqrestore(&fmgr->slock, flags);
			score_warn("Frame(%d, %u-%u, %u) is destroyed\n",
					list_frame->state,
					list_frame->sctx->id,
					list_frame->frame_id,
					list_frame->kernel_id);
			score_frame_destroy(list_frame);
			spin_lock_irqsave(&fmgr->slock, flags);
		}
	}
	spin_unlock_irqrestore(&fmgr->slock, flags);
	score_leave();
}

/**
 * score_frame_manager_block - Block to prevent creating frame
 * @fmgr:	[in]	object about score_frame_manager structure
 */
void score_frame_manager_block(struct score_frame_manager *fmgr)
{
	score_enter();
	fmgr->block = true;
	score_leave();
}

/**
 * score_frame_manager_unblock - Clear the blocking of frame manager
 * @fmgr:	[in]	object about score_frame_manager structure
 */
void score_frame_manager_unblock(struct score_frame_manager *fmgr)
{
	score_enter();
	fmgr->block = false;
	score_leave();
}

/**
 * score_frame_create - Create new frame and add that at frame manager
 * @fmgr:	[in]	object about score_frame_manager structure
 * @sctx:	[in]	pointer for context
 * @type:	[in]	task type of this frame (block, non-block or no-wait)
 *
 * Returns frame address created if succeeded, otherwise NULL
 */
struct score_frame *score_frame_create(struct score_frame_manager *fmgr,
		struct score_context *sctx, int type)
{
	int ret = 0;
	unsigned long flags;
	struct score_frame *frame;

	score_enter();
	if (!(type > TYPE_START && type < TYPE_END)) {
		ret = -EINVAL;
		score_err("Type(%d) of frame is invalid [sctx:%u]\n",
				type, sctx->id);
		return ERR_PTR(ret);
	}

	frame = kzalloc(sizeof(struct score_frame), GFP_KERNEL);
	if (!frame) {
		ret = -ENOMEM;
		score_err("Memory for frame is not allocated [sctx:%u]\n",
				sctx->id);
		return ERR_PTR(ret);
	}

	frame->sctx = sctx;
	frame->frame_id = score_util_bitmap_get_zero_bit(fmgr->frame_map,
			SCORE_MAX_FRAME);
	if (frame->frame_id == SCORE_MAX_FRAME) {
		ret = -ENOMEM;
		score_err("frame bitmap is full [sctx:%u]\n", sctx->id);
		goto p_err;
	}
	frame->type = type;
	frame->owner = fmgr;

	spin_lock_irqsave(&fmgr->slock, flags);
	if (fmgr->block) {
		spin_unlock_irqrestore(&fmgr->slock, flags);
		score_warn("frame manager is blocked [sctx:%u]\n", sctx->id);
		ret = -EINVAL;
		goto p_block;
	}

	list_add_tail(&frame->list, &sctx->frame_list);
	sctx->frame_count++;
	sctx->frame_total_count++;
	list_add_tail(&frame->entire_list, &fmgr->entire_list);
	fmgr->entire_count++;
	__score_frame_set_ready(fmgr, frame);

	/* debug count */
	fmgr->all_count++;
	spin_unlock_irqrestore(&fmgr->slock, flags);

	frame->packet = NULL;
	frame->packet_size = 0;
	frame->pended = false;
	frame->pending_packet = NULL;
	frame->ret = 0;
	INIT_LIST_HEAD(&frame->buffer_list);
	frame->buffer_count = 0;
	frame->work.func = NULL;

	frame->kernel_id = -1;
	frame->priority = false;
	frame->task_type = 0;

	score_leave();
	return frame;
p_block:
	score_util_bitmap_clear_bit(fmgr->frame_map, frame->frame_id);
p_err:
	kfree(frame);
	return ERR_PTR(ret);
}

static void __score_frame_destroy(struct score_frame_manager *fmgr,
		struct score_frame *frame)
{
	unsigned long flags;
	bool pended;

	score_enter();
	spin_lock_irqsave(&fmgr->slock, flags);
	score_frame_trans_any_to_complete(frame, -ENOSTR);
	spin_unlock_irqrestore(&fmgr->slock, flags);

	if (frame->work.func)
		kthread_flush_work(&frame->work);

	if (frame->buffer_count) {
		struct score_mmu_buffer *buf, *tbuf;

		list_for_each_entry_safe(buf, tbuf, &frame->buffer_list,
				frame_list) {
			score_frame_remove_buffer(frame, buf);
			if (buf->mirror) {
				score_mmu_unmap_buffer(frame->sctx->mmu_ctx,
						buf);
				kfree(buf);
			}
		}
	}

	spin_lock_irqsave(&fmgr->slock, flags);
	__score_frame_destroy_state(fmgr, frame);

	fmgr->entire_count--;
	list_del(&frame->entire_list);
	frame->sctx->frame_count--;
	list_del(&frame->list);

	pended = frame->pended;

	spin_unlock_irqrestore(&fmgr->slock, flags);

	if (frame->ret)
		fmgr->abnormal_count++;
	else
		fmgr->normal_count++;

	if (pended)
		kfree(frame->pending_packet);

	score_util_bitmap_clear_bit(fmgr->frame_map, frame->frame_id);
	kfree(frame);
	score_leave();
}

/**
 * score_frame_destroy - Remove frame at frame manager and free memory of frame
 * @frame:      [in]	object about score_frame structure
 */
void score_frame_destroy(struct score_frame *frame)
{
	__score_frame_destroy(frame->owner, frame);
}

int score_frame_manager_probe(struct score_frame_manager *fmgr)
{
	score_enter();
	INIT_LIST_HEAD(&fmgr->entire_list);
	fmgr->entire_count = 0;

	INIT_LIST_HEAD(&fmgr->ready_list);
	fmgr->ready_count = 0;

	INIT_LIST_HEAD(&fmgr->process_list);
	fmgr->process_count = 0;
	fmgr->process_high_count = 0;
	fmgr->process_normal_count = 0;

	INIT_LIST_HEAD(&fmgr->pending_list);
	fmgr->pending_count = 0;
	fmgr->pending_high_count = 0;
	fmgr->pending_normal_count = 0;

	INIT_LIST_HEAD(&fmgr->complete_list);
	fmgr->complete_count = 0;

	fmgr->all_count = 0;
	fmgr->normal_count = 0;
	fmgr->abnormal_count = 0;

	spin_lock_init(&fmgr->slock);
	score_util_bitmap_init(fmgr->frame_map, SCORE_MAX_FRAME);
	init_waitqueue_head(&fmgr->done_wq);
	fmgr->block = false;

	score_leave();
	return 0;
}

void score_frame_manager_remove(struct score_frame_manager *fmgr)
{
	struct score_frame *frame, *tframe;

	score_enter();
	if (fmgr->entire_count) {
		list_for_each_entry_safe(frame, tframe, &fmgr->entire_list,
				entire_list) {
			score_warn("[%u]frame is destroyed(count:%d)\n",
					frame->frame_id,
					fmgr->entire_count);
			__score_frame_destroy(fmgr, frame);
		}
	}
	score_leave();
}
