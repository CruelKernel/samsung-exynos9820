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

#include <linux/slab.h>
#include <linux/io.h>
#include <linux/delay.h>

#include "score_log.h"
#include "score_dpmu.h"
#include "score_util.h"
#include "score_context.h"

static void __score_context_frame_flush(struct score_context *sctx,
		struct score_frame *frame, bool unmap)
{
	struct score_mmu_context *mmu_ctx;
	struct score_mmu_buffer *buf, *tbuf;

	score_enter();
	mmu_ctx = sctx->mmu_ctx;
	if (frame->buffer_count) {
		list_for_each_entry_safe(buf, tbuf, &frame->buffer_list,
				frame_list) {
			score_frame_remove_buffer(frame, buf);
			if (buf->mirror || unmap) {
				score_mmu_unmap_buffer(mmu_ctx, buf);
				kfree(buf);
			}
		}
	}
	score_leave();
}

static int __score_context_wait_for_done(struct score_context *sctx,
		struct score_frame *frame)
{
	int ret = 0;
	struct score_frame_manager *framemgr = frame->owner;

	score_enter();
	if (!score_frame_check_done(frame)) {
		ret = wait_event_interruptible_timeout(framemgr->done_wq,
				score_frame_check_done(frame),
				msecs_to_jiffies(sctx->wait_time));
		if (!ret) {
			score_err("Wait time(%u ms) is over (%u-%u)\n",
					sctx->wait_time, frame->sctx->id,
					frame->frame_id);
			ret = -ETIMEDOUT;
		} else if (ret > 0) {
			ret = 0;
		} else {
			score_err("Waiting ended abnormaly (%d)\n", ret);
			ret = -ECONNABORTED;
		}
	}
	score_leave();
	return ret;
}

static int score_context_queue(struct score_context *sctx,
		struct score_frame *frame)
{
	int ret = 0;

	score_enter();
	if (!(sctx->frame_total_count % UNMAP_TRY)) {
		struct score_mmu_context *mmu_ctx = sctx->mmu_ctx;

		if (mmu_ctx->work_run)
			kthread_queue_work(&mmu_ctx->mmu->unmap_worker,
					&mmu_ctx->work);
	}

	ret = score_scq_send_packet(&sctx->system->scq, frame);
	if (ret) {
		score_err("Failed to send packet (%u-%u, %u)\n",
				frame->sctx->id, frame->frame_id,
				frame->kernel_id);
		__score_context_frame_flush(sctx, frame, true);
	}

	score_leave();
	return ret;
}

static int score_context_deque(struct score_context *sctx,
		struct score_frame *frame)
{
	int ret = 0;
	unsigned long flags;

	score_enter();
	ret = __score_context_wait_for_done(sctx, frame);
	if (ret) {
		spin_lock_irqsave(&frame->owner->slock, flags);
		score_err("frame(%d) is ended abnormaly (%u-%u, %u)\n",
				frame->state, frame->sctx->id,
				frame->frame_id, frame->kernel_id);
		switch (score_frame_get_state(frame)) {
		case SCORE_FRAME_STATE_READY:
			score_frame_trans_ready_to_complete(frame, ret);
			break;
		case SCORE_FRAME_STATE_PROCESS:
			score_dpmu_print_all();
			score_frame_trans_process_to_complete(frame, ret);
			score_frame_flush_process(frame->owner, -ENOSTR);
			wake_up_all(&frame->owner->done_wq);
			score_system_halt(sctx->system);
			score_system_boot(sctx->system);
			break;
		case SCORE_FRAME_STATE_PENDING:
			score_frame_trans_pending_to_complete(frame, ret);
			break;
		case SCORE_FRAME_STATE_COMPLETE:
			frame->ret = ret;
			break;
		default:
			score_frame_trans_any_to_complete(frame, ret);
			break;
		}
		spin_unlock_irqrestore(&frame->owner->slock, flags);
	} else {
		if (sctx->core->dpmu_print)
			score_dpmu_print_all();
	}

#if defined(CONFIG_EXYNOS_SCORE_FPGA)
	__score_context_frame_flush(sctx, frame, true);
#else
	__score_context_frame_flush(sctx, frame, false);
#endif

	score_leave();
	return ret;
}

const struct score_context_ops score_ctx_ops = {
	.queue		= score_context_queue,
	.deque		= score_context_deque
};

/**
 * score_context_create - Create new context and add that at score_core
 * @core:	[in]	object about score_core structure
 *
 * Returns context address created if secceeded, otherwise NULL
 */
struct score_context *score_context_create(struct score_core *core)
{
	int ret = 0;
	struct score_context *sctx;
	unsigned long flags;

	score_enter();
	sctx = kzalloc(sizeof(struct score_context), GFP_KERNEL);
	if (!sctx) {
		ret = -ENOMEM;
		score_err("Fail to alloc context\n");
		goto p_err;
	}

	sctx->mmu_ctx = score_mmu_create_context(&core->system->mmu);
	if (IS_ERR(sctx->mmu_ctx)) {
		ret = PTR_ERR(sctx->mmu_ctx);
		goto p_err_mem_ctx;
	}

	sctx->id = score_util_bitmap_get_zero_bit(core->ctx_map,
			SCORE_MAX_CONTEXT);
	if (sctx->id == SCORE_MAX_CONTEXT) {
		score_err("Fail to alloc id of context(%u)\n", sctx->id);
		ret = -ENOMEM;
		goto p_err_bitmap;
	}

	spin_lock_irqsave(&core->ctx_slock, flags);
	list_add_tail(&sctx->list, &core->ctx_list);
	core->ctx_count++;
	spin_unlock_irqrestore(&core->ctx_slock, flags);

	INIT_LIST_HEAD(&sctx->frame_list);
	sctx->frame_count = 0;
	sctx->frame_total_count = 0;
	sctx->wait_time = core->wait_time;

	sctx->ctx_ops = &score_ctx_ops;
	sctx->core = core;
	sctx->system = core->system;

	score_leave();
	return sctx;
p_err_bitmap:
	score_mmu_destroy_context(sctx->mmu_ctx);
p_err_mem_ctx:
	kfree(sctx);
p_err:
	return ERR_PTR(ret);
}

static void __score_context_wait_for_stop(struct score_context *sctx)
{
	int idx;

	score_enter();
	for (idx = 0; idx < 100; ++idx) {
		if (!sctx->frame_count)
			break;
		usleep_range(100, 200);
	}

	if (sctx->frame_count)
		score_warn("Context(%d) is unstable as frame remains(%d)\n",
				sctx->id, sctx->frame_count);

	score_leave();
}

static void __score_context_stop(struct score_context *sctx)
{
	struct score_system *system;
	struct score_frame_manager *framemgr;
	struct score_frame *frame, *tframe;
	unsigned long flags;
	bool nonblock = false;

	score_enter();
	if (!sctx->frame_count)
		return;

	system = sctx->system;
	framemgr = &system->interface.framemgr;

	spin_lock_irqsave(&framemgr->slock, flags);
	list_for_each_entry_safe(frame, tframe, &sctx->frame_list,
			list) {
		if (score_frame_get_state(frame) ==
				SCORE_FRAME_STATE_PROCESS) {
			score_frame_flush_process(framemgr, -ECONNABORTED);
			score_system_halt(system);
			score_system_boot(system);
		} else {
			score_frame_trans_any_to_complete(frame,
					-ECONNABORTED);
		}
		if (score_frame_check_type(frame, TYPE_NONBLOCK_NOWAIT) ||
				score_frame_check_type(frame, TYPE_NONBLOCK)) {
			nonblock = true;
			score_frame_set_type_destroy(frame);
		}
	}
	spin_unlock_irqrestore(&framemgr->slock, flags);
	wake_up_all(&framemgr->done_wq);

	if (nonblock) {
		list_for_each_entry_safe(frame, tframe, &sctx->frame_list,
				list) {
			if (score_frame_check_type(frame,
						TYPE_NONBLOCK_DESTROY)) {
				score_warn("Frame(%u-%u,%u) is destroyed\n",
						frame->sctx->id,
						frame->frame_id,
						frame->kernel_id);
				score_frame_destroy(frame);
			}
		}
	}

	__score_context_wait_for_stop(sctx);
	score_leave();
}

/**
 * score_context_destroy - Remove context and free memory of context
 * @sctx:	[in]	object about score_context structure
 */
void score_context_destroy(struct score_context *sctx)
{
	struct score_core *core;
	unsigned long flags;

	score_enter();
	__score_context_stop(sctx);

	core = sctx->core;
	spin_lock_irqsave(&core->ctx_slock, flags);
	core->ctx_count--;
	list_del(&sctx->list);
	spin_unlock_irqrestore(&core->ctx_slock, flags);

	score_util_bitmap_clear_bit(core->ctx_map, sctx->id);
	score_mmu_destroy_context(sctx->mmu_ctx);
	kfree(sctx);
	score_leave();
}
