/*
 * drivers/media/platform/exynos/mfc/mfc_hwlock.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "mfc_hwlock.h"

#include "mfc_nal_q.h"
#include "mfc_otf.h"
#include "mfc_run.h"
#include "mfc_sync.h"

#include "mfc_pm.h"
#include "mfc_cmd.h"
#include "mfc_reg_api.h"
#include "mfc_hw_reg_api.h"

#include "mfc_queue.h"
#include "mfc_utils.h"

static inline void __mfc_print_hwlock(struct mfc_dev *dev)
{
	mfc_debug(2, "dev.hwlock.dev = 0x%lx, bits = 0x%lx, owned_by_irq = %d, wl_count = %d, transfer_owner = %d\n",
		dev->hwlock.dev, dev->hwlock.bits, dev->hwlock.owned_by_irq,
		dev->hwlock.wl_count, dev->hwlock.transfer_owner);
}

void mfc_init_hwlock(struct mfc_dev *dev)
{
	unsigned long flags;

	spin_lock_init(&dev->hwlock.lock);
	spin_lock_irqsave(&dev->hwlock.lock, flags);

	INIT_LIST_HEAD(&dev->hwlock.waiting_list);
	dev->hwlock.wl_count = 0;
	dev->hwlock.bits = 0;
	dev->hwlock.dev = 0;
	dev->hwlock.owned_by_irq = 0;
	dev->hwlock.transfer_owner = 0;

	spin_unlock_irqrestore(&dev->hwlock.lock, flags);
}

static void __mfc_remove_listable_wq_dev(struct mfc_dev *dev)
{
	struct mfc_listable_wq *listable_wq;
	unsigned long flags;

	spin_lock_irqsave(&dev->hwlock.lock, flags);
	__mfc_print_hwlock(dev);

	list_for_each_entry(listable_wq, &dev->hwlock.waiting_list, list) {
		if (!listable_wq->dev)
			continue;

		mfc_debug(2, "Found dev and will delete it!\n");

		list_del(&listable_wq->list);
		dev->hwlock.wl_count--;

		break;
	}

	__mfc_print_hwlock(dev);
	spin_unlock_irqrestore(&dev->hwlock.lock, flags);
}

static void __mfc_remove_listable_wq_ctx(struct mfc_ctx *curr_ctx)
{
	struct mfc_dev *dev = curr_ctx->dev;
	struct mfc_listable_wq *listable_wq;
	unsigned long flags;

	spin_lock_irqsave(&dev->hwlock.lock, flags);
	__mfc_print_hwlock(dev);

	list_for_each_entry(listable_wq, &dev->hwlock.waiting_list, list) {
		if (!listable_wq->ctx)
			continue;

		if (listable_wq->ctx->num == curr_ctx->num) {
			mfc_debug(2, "Found ctx and will delete it (%d)!\n", curr_ctx->num);

			list_del(&listable_wq->list);
			dev->hwlock.wl_count--;
			break;
		}
	}

	__mfc_print_hwlock(dev);
	spin_unlock_irqrestore(&dev->hwlock.lock, flags);
}

/*
 * Return value description
 *    0: succeeded to get hwlock
 * -EIO: failed to get hwlock (time out)
 */
int mfc_get_hwlock_dev(struct mfc_dev *dev)
{
	int ret = 0;
	unsigned long flags;

	mutex_lock(&dev->hwlock_wq.wait_mutex);

	spin_lock_irqsave(&dev->hwlock.lock, flags);
	__mfc_print_hwlock(dev);

	if (dev->shutdown) {
		mfc_info_dev("Couldn't lock HW. Shutdown was called\n");
		spin_unlock_irqrestore(&dev->hwlock.lock, flags);
		mutex_unlock(&dev->hwlock_wq.wait_mutex);
		return -EINVAL;
	}

	if ((dev->hwlock.bits != 0) || (dev->hwlock.dev != 0)) {
		list_add_tail(&dev->hwlock_wq.list, &dev->hwlock.waiting_list);
		dev->hwlock.wl_count++;

		spin_unlock_irqrestore(&dev->hwlock.lock, flags);

		mfc_debug(2, "Waiting for hwlock to be released\n");

		ret = wait_event_timeout(dev->hwlock_wq.wait_queue,
			((dev->hwlock.transfer_owner == 1) && (dev->hwlock.dev == 1)),
			msecs_to_jiffies(MFC_HWLOCK_TIMEOUT));

		dev->hwlock.transfer_owner = 0;
		__mfc_remove_listable_wq_dev(dev);
		if (ret == 0) {
			mfc_err_dev("Woken up but timed out\n");
			__mfc_print_hwlock(dev);
			mutex_unlock(&dev->hwlock_wq.wait_mutex);
			return -EIO;
		} else {
			mfc_debug(2, "Woken up and got hwlock\n");
			__mfc_print_hwlock(dev);
			mutex_unlock(&dev->hwlock_wq.wait_mutex);
		}
	} else {
		dev->hwlock.bits = 0;
		dev->hwlock.dev = 1;
		dev->hwlock.owned_by_irq = 0;

		__mfc_print_hwlock(dev);
		spin_unlock_irqrestore(&dev->hwlock.lock, flags);
		mutex_unlock(&dev->hwlock_wq.wait_mutex);
	}

	/* Stop NAL-Q after getting hwlock */
	if (dev->nal_q_handle)
		mfc_nal_q_stop_if_started(dev);

	return 0;
}

/*
 * Return value description
 *    0: succeeded to get hwlock
 * -EIO: failed to get hwlock (time out)
 */
int mfc_get_hwlock_ctx(struct mfc_ctx *curr_ctx)
{
	struct mfc_dev *dev = curr_ctx->dev;
	int ret = 0;
	unsigned long flags;

	mutex_lock(&curr_ctx->hwlock_wq.wait_mutex);

	spin_lock_irqsave(&dev->hwlock.lock, flags);
	__mfc_print_hwlock(dev);

	if (dev->shutdown) {
		mfc_info_dev("Couldn't lock HW. Shutdown was called\n");
		spin_unlock_irqrestore(&dev->hwlock.lock, flags);
		mutex_unlock(&curr_ctx->hwlock_wq.wait_mutex);
		return -EINVAL;
	}

	if ((dev->hwlock.bits != 0) || (dev->hwlock.dev != 0)) {
		list_add_tail(&curr_ctx->hwlock_wq.list, &dev->hwlock.waiting_list);
		dev->hwlock.wl_count++;

		spin_unlock_irqrestore(&dev->hwlock.lock, flags);

		mfc_debug(2, "Waiting for hwlock to be released\n");

		ret = wait_event_timeout(curr_ctx->hwlock_wq.wait_queue,
			((dev->hwlock.transfer_owner == 1) && (test_bit(curr_ctx->num, &dev->hwlock.bits))),
			msecs_to_jiffies(MFC_HWLOCK_TIMEOUT));

		dev->hwlock.transfer_owner = 0;
		__mfc_remove_listable_wq_ctx(curr_ctx);
		if (ret == 0) {
			mfc_err_dev("Woken up but timed out\n");
			__mfc_print_hwlock(dev);
			mutex_unlock(&curr_ctx->hwlock_wq.wait_mutex);
			return -EIO;
		} else {
			mfc_debug(2, "Woken up and got hwlock\n");
			__mfc_print_hwlock(dev);
			mutex_unlock(&curr_ctx->hwlock_wq.wait_mutex);
		}
	} else {
		dev->hwlock.bits = 0;
		dev->hwlock.dev = 0;
		set_bit(curr_ctx->num, &dev->hwlock.bits);
		dev->hwlock.owned_by_irq = 0;

		__mfc_print_hwlock(dev);
		spin_unlock_irqrestore(&dev->hwlock.lock, flags);
		mutex_unlock(&curr_ctx->hwlock_wq.wait_mutex);
	}

	/* Stop NAL-Q after getting hwlock */
	if (dev->nal_q_handle)
		mfc_nal_q_stop_if_started(dev);

	return 0;
}

/*
 * Return value description
 *  0: succeeded to release hwlock
 *  1: succeeded to release hwlock, hwlock is captured by another module
 * -1: error since device is waiting again.
 */
void mfc_release_hwlock_dev(struct mfc_dev *dev)
{
	struct mfc_listable_wq *listable_wq;
	unsigned long flags;

	spin_lock_irqsave(&dev->hwlock.lock, flags);
	__mfc_print_hwlock(dev);

	dev->hwlock.dev = 0;
	dev->hwlock.owned_by_irq = 0;

	if (dev->shutdown) {
		mfc_debug(2, "Couldn't wakeup module. Shutdown was called\n");
	} else if (list_empty(&dev->hwlock.waiting_list)) {
		mfc_debug(2, "No waiting module\n");
	} else {
		mfc_debug(2, "There is a waiting module\n");
		listable_wq = list_entry(dev->hwlock.waiting_list.next, struct mfc_listable_wq, list);
		list_del(&listable_wq->list);
		dev->hwlock.wl_count--;

		if (listable_wq->dev) {
			mfc_debug(2, "Waking up dev\n");
			dev->hwlock.dev = 1;
		} else {
			mfc_debug(2, "Waking up another ctx\n");
			set_bit(listable_wq->ctx->num, &dev->hwlock.bits);
		}

		dev->hwlock.transfer_owner = 1;

		wake_up(&listable_wq->wait_queue);
	}

	__mfc_print_hwlock(dev);
	spin_unlock_irqrestore(&dev->hwlock.lock, flags);
}

/*
 * Should be called with hwlock.lock
 *
 * Return value description
 * 0: succeeded to release hwlock
 * 1: succeeded to release hwlock, hwlock is captured by another module
 */
static void __mfc_release_hwlock_ctx_protected(struct mfc_ctx *curr_ctx)
{
	struct mfc_dev *dev = curr_ctx->dev;
	struct mfc_listable_wq *listable_wq;

	__mfc_print_hwlock(dev);
	clear_bit(curr_ctx->num, &dev->hwlock.bits);
	dev->hwlock.owned_by_irq = 0;

	if (dev->shutdown) {
		mfc_debug(2, "Couldn't wakeup module. Shutdown was called\n");
	} else if (list_empty(&dev->hwlock.waiting_list)) {
		mfc_debug(2, "No waiting module\n");
	} else {
		mfc_debug(2, "There is a waiting module\n");
		listable_wq = list_entry(dev->hwlock.waiting_list.next, struct mfc_listable_wq, list);
		list_del(&listable_wq->list);
		dev->hwlock.wl_count--;

		if (listable_wq->dev) {
			mfc_debug(2, "Waking up dev\n");
			dev->hwlock.dev = 1;
		} else {
			mfc_debug(2, "Waking up another ctx\n");
			set_bit(listable_wq->ctx->num, &dev->hwlock.bits);
		}

		dev->hwlock.transfer_owner = 1;

		wake_up(&listable_wq->wait_queue);
	}

	__mfc_print_hwlock(dev);
}

/*
 * Return value description
 * 0: succeeded to release hwlock
 * 1: succeeded to release hwlock, hwlock is captured by another module
 */
void mfc_release_hwlock_ctx(struct mfc_ctx *curr_ctx)
{
	struct mfc_dev *dev = curr_ctx->dev;
	unsigned long flags;

	spin_lock_irqsave(&dev->hwlock.lock, flags);
	__mfc_release_hwlock_ctx_protected(curr_ctx);
	spin_unlock_irqrestore(&dev->hwlock.lock, flags);
}

static inline void __mfc_yield_hwlock(struct mfc_dev *dev, struct mfc_ctx *curr_ctx)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->hwlock.lock, flags);

	__mfc_release_hwlock_ctx_protected(curr_ctx);

	spin_unlock_irqrestore(&dev->hwlock.lock, flags);

	/* Trigger again if other instance's work is waiting */
	if (mfc_is_work_to_do(dev))
		queue_work(dev->butler_wq, &dev->butler_work);
}

/*
 * Should be called with hwlock.lock
 */
static inline void __mfc_transfer_hwlock_ctx_protected(struct mfc_dev *dev, int curr_ctx_index)
{
	dev->hwlock.dev = 0;
	dev->hwlock.bits = 0;
	set_bit(curr_ctx_index, &dev->hwlock.bits);
}

/*
 * Should be called with hwlock.lock
 *
 * Return value description
 *   >=0: succeeded to get hwlock_bit for the context, index of new context
 *   -1, -EINVAL: failed to get hwlock_bit for a context
 */
static int __mfc_try_to_get_new_ctx_protected(struct mfc_dev *dev)
{
	int ret = 0;
	int index;
	struct mfc_ctx *new_ctx;

	if (dev->shutdown) {
		mfc_info_dev("Couldn't lock HW. Shutdown was called\n");
		return -EINVAL;
	}

	if (dev->sleep) {
		mfc_info_dev("Couldn't lock HW. Sleep was called\n");
		return -EINVAL;
	}

	/* Check whether hardware is not running */
	if ((dev->hwlock.bits != 0) || (dev->hwlock.dev != 0)) {
		/* This is perfectly ok, the scheduled ctx should wait */
		mfc_debug(2, "Couldn't lock HW\n");
		return -1;
	}

	/* Choose the context to run */
	index = mfc_get_new_ctx(dev);
	if (index < 0) {
		/* This is perfectly ok, the scheduled ctx should wait
		 * No contexts to run
		 */
		mfc_debug(2, "No ctx is scheduled to be run\n");
		ret = -1;
		return ret;
	}

	new_ctx = dev->ctx[index];
	if (!new_ctx) {
		mfc_err_dev("no mfc context to run\n");
		ret = -1;
		return ret;
	}

	set_bit(new_ctx->num, &dev->hwlock.bits);
	ret = index;

	return ret;
}

/*
 * Should be called without hwlock holding
 *
 * Try to run an operation on hardware
 */
void mfc_try_run(struct mfc_dev *dev)
{
	int new_ctx_index;
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&dev->hwlock.lock, flags);
	__mfc_print_hwlock(dev);

	new_ctx_index = __mfc_try_to_get_new_ctx_protected(dev);
	if (new_ctx_index < 0) {
		mfc_debug(2, "Failed to get new context to run\n");
		__mfc_print_hwlock(dev);
		spin_unlock_irqrestore(&dev->hwlock.lock, flags);
		return;
	}

	dev->hwlock.owned_by_irq = 1;

	__mfc_print_hwlock(dev);
	spin_unlock_irqrestore(&dev->hwlock.lock, flags);

	ret = mfc_just_run(dev, new_ctx_index);
	if (ret)
		__mfc_yield_hwlock(dev, dev->ctx[new_ctx_index]);
}

/*
 * Should be called without hwlock holding
 *
 */
void mfc_cleanup_work_bit_and_try_run(struct mfc_ctx *curr_ctx)
{
	struct mfc_dev *dev = curr_ctx->dev;

	mfc_clear_bit(curr_ctx->num, &dev->work_bits);

	mfc_try_run(dev);
}

void mfc_cache_flush(struct mfc_dev *dev, int is_drm)
{
	mfc_cmd_cache_flush(dev);
	if (mfc_wait_for_done_dev(dev, MFC_REG_R2H_CMD_CACHE_FLUSH_RET)) {
		mfc_err_dev("Failed to CACHE_FLUSH\n");
		dev->logging_data->cause |= (1 << MFC_CAUSE_FAIL_CHACHE_FLUSH);
		call_dop(dev, dump_and_stop_always, dev);
	}

	mfc_pm_clock_off(dev);
	dev->curr_ctx_is_drm = is_drm;
	mfc_pm_clock_on_with_base(dev, (is_drm ? MFCBUF_DRM : MFCBUF_NORMAL));
}

/*
 * Return value description
 *  0: NAL-Q is handled successfully
 *  1: NAL_START command should be handled
 * -1: Error
*/
static int __mfc_nal_q_just_run(struct mfc_ctx *ctx, int need_cache_flush)
{
	struct mfc_dev *dev = ctx->dev;
	nal_queue_handle *nal_q_handle = dev->nal_q_handle;
	unsigned int ret = -1;

	switch (nal_q_handle->nal_q_state) {
	case NAL_Q_STATE_CREATED:
		if (mfc_nal_q_check_enable(dev) == 0) {
			/* NAL START */
			ret = 1;
		} else {
			mfc_nal_q_clock_on(dev, nal_q_handle);

			mfc_nal_q_init(dev, nal_q_handle);

			/* enable NAL QUEUE */
			if (need_cache_flush)
				mfc_cache_flush(dev, ctx->is_drm);

			mfc_info_ctx("[NALQ] start NAL QUEUE\n");
			mfc_nal_q_start(dev, nal_q_handle);

			if (mfc_nal_q_enqueue_in_buf(dev, ctx, nal_q_handle->nal_q_in_handle)) {
				mfc_debug(2, "[NALQ] Failed to enqueue input data\n");
				mfc_nal_q_clock_off(dev, nal_q_handle);
			}

			mfc_clear_bit(ctx->num, &dev->work_bits);
			if ((mfc_ctx_ready(ctx) && !ctx->clear_work_bit) ||
					nal_q_handle->nal_q_exception)
				mfc_set_bit(ctx->num, &dev->work_bits);
			ctx->clear_work_bit = 0;

			mfc_release_hwlock_ctx(ctx);

			if (mfc_is_work_to_do(dev))
				queue_work(dev->butler_wq, &dev->butler_work);

			ret = 0;
		}
		break;
	case NAL_Q_STATE_STARTED:
		mfc_nal_q_clock_on(dev, nal_q_handle);

		if (mfc_nal_q_check_enable(dev) == 0 ||
				nal_q_handle->nal_q_exception) {
			/* disable NAL QUEUE */
			mfc_nal_q_stop(dev, nal_q_handle);
			mfc_info_ctx("[NALQ] stop NAL QUEUE\n");
			if (mfc_wait_for_done_dev(dev,
					MFC_REG_R2H_CMD_COMPLETE_QUEUE_RET)) {
				mfc_err_dev("[NALQ] Failed to stop queue\n");
				dev->logging_data->cause |= (1 << MFC_CAUSE_FAIL_STOP_NAL_Q);
				call_dop(dev, dump_and_stop_always, dev);
	                }
			ret = 1;
			break;
		} else {
			/* NAL QUEUE */
			if (mfc_nal_q_enqueue_in_buf(dev, ctx, nal_q_handle->nal_q_in_handle)) {
				mfc_debug(2, "[NALQ] Failed to enqueue input data\n");
				mfc_nal_q_clock_off(dev, nal_q_handle);
			}

			mfc_clear_bit(ctx->num, &dev->work_bits);

			if ((mfc_ctx_ready(ctx) && !ctx->clear_work_bit) ||
					nal_q_handle->nal_q_exception)
				mfc_set_bit(ctx->num, &dev->work_bits);
			ctx->clear_work_bit = 0;

			mfc_release_hwlock_ctx(ctx);

			if (mfc_is_work_to_do(dev))
				queue_work(dev->butler_wq, &dev->butler_work);
			ret = 0;
		}
		break;
	default:
		mfc_info_ctx("[NALQ] can't try command, nal_q_state : %d\n",
				nal_q_handle->nal_q_state);
		ret = -1;
		break;
	}

	return ret;
}

static int __mfc_just_run_dec(struct mfc_ctx *ctx)
{
	int ret = 0;

	switch (ctx->state) {
	case MFCINST_FINISHING:
		ret = mfc_run_dec_last_frames(ctx);
		break;
	case MFCINST_RUNNING:
	case MFCINST_SPECIAL_PARSING_NAL:
		ret = mfc_run_dec_frame(ctx);
		break;
	case MFCINST_INIT:
		mfc_cmd_open_inst(ctx);
		break;
	case MFCINST_RETURN_INST:
		ret = mfc_cmd_close_inst(ctx);
		break;
	case MFCINST_GOT_INST:
	case MFCINST_SPECIAL_PARSING:
		ret = mfc_run_dec_init(ctx);
		break;
	case MFCINST_HEAD_PARSED:
		if (ctx->codec_buffer_allocated == 0) {
			ctx->clear_work_bit = 1;
			mfc_err_ctx("codec buffer is not allocated\n");
			ret = -EAGAIN;
			break;
		}
		if (ctx->wait_state != WAIT_NONE) {
			mfc_err_ctx("wait_state(%d) is not ready\n", ctx->wait_state);
			ret = -EAGAIN;
			break;
		}
		ret = mfc_cmd_dec_init_buffers(ctx);
		break;
	case MFCINST_RES_CHANGE_INIT:
		ret = mfc_run_dec_last_frames(ctx);
		break;
	case MFCINST_RES_CHANGE_FLUSH:
		ret = mfc_run_dec_last_frames(ctx);
		break;
	case MFCINST_RES_CHANGE_END:
		mfc_debug(2, "[DRC] Finished remaining frames after resolution change\n");
		ctx->capture_state = QUEUE_FREE;
		mfc_debug(2, "[DRC] Will re-init the codec\n");
		ret = mfc_run_dec_init(ctx);
		break;
	case MFCINST_DPB_FLUSHING:
		mfc_cmd_dpb_flush(ctx);
		break;
	default:
		mfc_info_ctx("can't try command(decoder just_run), state : %d\n", ctx->state);
		ret = -EAGAIN;
	}

	return ret;
}

static int __mfc_just_run_enc(struct mfc_ctx *ctx)
{
	int ret = 0;

	switch (ctx->state) {
		case MFCINST_FINISHING:
			ret = mfc_run_enc_last_frames(ctx);
			break;
		case MFCINST_RUNNING:
			if (ctx->otf_handle) {
				ret = mfc_otf_run_enc_frame(ctx);
				break;
			}
			ret = mfc_run_enc_frame(ctx);
			break;
		case MFCINST_INIT:
			mfc_cmd_open_inst(ctx);
			break;
		case MFCINST_RETURN_INST:
			ret = mfc_cmd_close_inst(ctx);
			break;
		case MFCINST_GOT_INST:
			if (ctx->otf_handle) {
				ret = mfc_otf_run_enc_init(ctx);
				break;
			}
			ret = mfc_run_enc_init(ctx);
			break;
		case MFCINST_HEAD_PARSED:
			ret = mfc_cmd_enc_init_buffers(ctx);
			break;
		case MFCINST_ABORT_INST:
			mfc_cmd_abort_inst(ctx);
			break;
		default:
			mfc_info_ctx("can't try command(encoder just_run), state : %d\n", ctx->state);
			ret = -EAGAIN;
	}

	return ret;
}

/* Run an operation on hardware */
int mfc_just_run(struct mfc_dev *dev, int new_ctx_index)
{
	struct mfc_ctx *ctx = dev->ctx[new_ctx_index];
	unsigned int ret = 0;
	int need_cache_flush = 0;

	atomic_inc(&dev->hw_run_cnt);

	if (ctx->state == MFCINST_RUNNING)
		mfc_clean_ctx_int_flags(ctx);

	mfc_debug(2, "New context: %d\n", new_ctx_index);
	dev->curr_ctx = ctx->num;

	/* Got context to run in ctx */
	mfc_debug(2, "src: %d, dst: %d, state: %d, dpb_count = %d\n",
		mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->src_buf_queue),
		mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->dst_buf_queue),
		ctx->state, ctx->dpb_count);
	mfc_debug(2, "ctx->state = %d\n", ctx->state);
	/* Last frame has already been sent to MFC
	 * Now obtaining frames from MFC buffer */

	/* Check if cache flush command is needed */
	if (dev->curr_ctx_is_drm != ctx->is_drm)
		need_cache_flush = 1;
	else
		dev->curr_ctx_is_drm = ctx->is_drm;

	mfc_debug(2, "need_cache_flush = %d, is_drm = %d\n", need_cache_flush, ctx->is_drm);

	if (dev->nal_q_handle) {
		ret = __mfc_nal_q_just_run(ctx, need_cache_flush);
		if (ret == 0) {
			mfc_debug(2, "NAL_Q was handled\n");
			return ret;
		} else if (ret == 1){
			/* Path through */
			mfc_debug(2, "NAL_START will be handled\n");
		} else {
			return ret;
		}
	}

	mfc_debug(2, "continue_clock_on = %d\n", dev->continue_clock_on);
	if (!dev->continue_clock_on) {
		mfc_pm_clock_on(dev);
	} else {
		dev->continue_clock_on = false;
	}

	if (need_cache_flush)
		mfc_cache_flush(dev, ctx->is_drm);

	if (ctx->type == MFCINST_DECODER) {
		ret = __mfc_just_run_dec(ctx);
	} else if (ctx->type == MFCINST_ENCODER) {
		ret = __mfc_just_run_enc(ctx);
	} else {
		mfc_err_ctx("invalid context type: %d\n", ctx->type);
		ret = -EAGAIN;
	}

	if (ret) {
		/* Check again the ctx condition and clear work bits
		 * if ctx is not available. */
		if (mfc_ctx_ready(ctx) == 0 || ctx->clear_work_bit) {
			mfc_clear_bit(ctx->num, &dev->work_bits);
			ctx->clear_work_bit = 0;
		}

		mfc_pm_clock_off(dev);
	}

	return ret;
}

void mfc_hwlock_handler_irq(struct mfc_dev *dev, struct mfc_ctx *curr_ctx,
		unsigned int reason, unsigned int err)
{
	int new_ctx_index;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&dev->hwlock.lock, flags);
	__mfc_print_hwlock(dev);

	if (dev->hwlock.owned_by_irq) {
		if (dev->preempt_ctx > MFC_NO_INSTANCE_SET) {
			mfc_debug(2, "There is a preempt_ctx\n");
			dev->continue_clock_on = true;
			mfc_wake_up_ctx(curr_ctx, reason, err);
			new_ctx_index = dev->preempt_ctx;
			mfc_debug(2, "preempt_ctx is : %d\n", new_ctx_index);

			spin_unlock_irqrestore(&dev->hwlock.lock, flags);

			ret = mfc_just_run(dev, new_ctx_index);
			if (ret) {
				dev->continue_clock_on = false;
				__mfc_yield_hwlock(dev, dev->ctx[new_ctx_index]);
			}
		} else if (!list_empty(&dev->hwlock.waiting_list)) {
			mfc_debug(2, "There is a waiting module for hwlock\n");
			dev->continue_clock_on = false;
			mfc_pm_clock_off(dev);

			spin_unlock_irqrestore(&dev->hwlock.lock, flags);

			mfc_wake_up_ctx(curr_ctx, reason, err);
			mfc_release_hwlock_ctx(curr_ctx);
			queue_work(dev->butler_wq, &dev->butler_work);
		} else {
			mfc_debug(2, "No preempt_ctx and no waiting module\n");
			new_ctx_index = mfc_get_new_ctx(dev);
			if (new_ctx_index < 0) {
				mfc_debug(2, "No ctx to run\n");
				/* No contexts to run */
				dev->continue_clock_on = false;
				mfc_pm_clock_off(dev);

				spin_unlock_irqrestore(&dev->hwlock.lock, flags);

				mfc_wake_up_ctx(curr_ctx, reason, err);
				mfc_release_hwlock_ctx(curr_ctx);
				queue_work(dev->butler_wq, &dev->butler_work);
			} else {
				mfc_debug(2, "There is a ctx to run\n");
				dev->continue_clock_on = true;
				mfc_wake_up_ctx(curr_ctx, reason, err);

				/* If cache flush command is needed or there is OTF handle, handler should stop */
				if ((dev->curr_ctx_is_drm != dev->ctx[new_ctx_index]->is_drm) ||
						dev->ctx[new_ctx_index]->otf_handle) {
					mfc_debug(2, "Secure and nomal switching or OTF mode\n");
					mfc_debug(2, "DRM attribute %d->%d\n",
							dev->curr_ctx_is_drm, dev->ctx[new_ctx_index]->is_drm);

					spin_unlock_irqrestore(&dev->hwlock.lock, flags);

					mfc_release_hwlock_ctx(curr_ctx);
					queue_work(dev->butler_wq, &dev->butler_work);
				} else {
					mfc_debug(2, "Work to do successively (next ctx: %d)\n", new_ctx_index);
					__mfc_transfer_hwlock_ctx_protected(dev, new_ctx_index);

					spin_unlock_irqrestore(&dev->hwlock.lock, flags);

					ret = mfc_just_run(dev, new_ctx_index);
					if (ret) {
						dev->continue_clock_on = false;
						__mfc_yield_hwlock(dev, dev->ctx[new_ctx_index]);
					}
				}
			}
		}
	} else {
		mfc_debug(2, "hwlock is NOT owned by irq\n");
		dev->continue_clock_on = false;
		mfc_pm_clock_off(dev);
		mfc_wake_up_ctx(curr_ctx, reason, err);
		queue_work(dev->butler_wq, &dev->butler_work);

		spin_unlock_irqrestore(&dev->hwlock.lock, flags);
	}
	__mfc_print_hwlock(dev);
}
