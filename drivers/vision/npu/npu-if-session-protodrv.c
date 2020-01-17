/*
 * Samsung Exynos SoC series NPU driver
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
#include <linux/kfifo.h>
#include <linux/spinlock.h>

#define NPU_LOG_TAG	"if-session-protodrv"

#include "npu-config.h"
#include "npu-common.h"
#include "npu-if-session-protodrv.h"
#include "npu-util-llq.h"
#include "npu-log.h"
#include "npu-queue.h"

extern int npu_session_save_result(struct npu_session *session, struct nw_result nw_result);
extern void npu_session_queue_done(struct npu_queue *queue, struct vb_container_list *inclist, struct vb_container_list *otclist, unsigned long flag);

/* Context information */
static struct npu_if_session_protodrv_ctx ctx = {
	.is_opened = ATOMIC_INIT(0),
};

/* TODO: Link appripirate function in Session manager */
struct npu_if_session_protodrv_ops npu_if_session_protodrv_ops = {
	.queue_done = npu_session_queue_done,
};

/* Initializer and deallocator */
struct npu_if_session_protodrv_ctx *npu_if_session_protodrv_ctx_open(void)
{
	npu_info("start in npu_if_session_protodrv_ctx_open\n");
	spin_lock_init(&ctx.buffer_q_lock);
	spin_lock_init(&ctx.ncp_mgmt_lock);

	INIT_KFIFO(ctx.buffer_q_list);
	INIT_KFIFO(ctx.ncp_mgmt_list);

	atomic_set(&ctx.is_opened, 1);
	npu_info("complete in npu_if_session_protodrv_ctx_open\n");
	return &ctx;
}

void npu_if_session_protodrv_ctx_close(void)
{
	npu_info("start in npu_if_session_protodrv_ctx_close\n");
	atomic_set(&ctx.is_opened, 0);
	kfifo_reset(&ctx.buffer_q_list);
	kfifo_reset(&ctx.ncp_mgmt_list);
	npu_info("complete in npu_if_session_protodrv_ctx_close\n");
}

/* Return 1 if the npu_if_session_protodrv is opened state
 * Otherwise, returns 0
 */
int npu_if_session_protodrv_is_opened(void)
{
	return atomic_read(&ctx.is_opened);
}

/* Operations for buffer_q */
int npu_buffer_q_get(struct npu_frame *frame)
{
	BUG_ON(!frame);
	return kfifo_out_spinlocked(&ctx.buffer_q_list, frame, 1, &ctx.buffer_q_lock);
}

int npu_buffer_q_put(const struct npu_frame *frame)
{
	int ret;

	BUG_ON(!frame);
	ret = kfifo_in_spinlocked(&ctx.buffer_q_list, frame, 1, &ctx.buffer_q_lock);
	if (ret > 0) {
		if (ctx.buffer_q_callback)	{
			ctx.buffer_q_callback();
		}
	}
	return ret;
}

int npu_buffer_q_is_available(void)
{
	return !kfifo_is_empty(&ctx.buffer_q_list);
}

void npu_buffer_q_register_cb(LLQ_task_t cb)
{
	npu_info("callback for buffer_q: [%pK]\n", cb);
	BUG_ON(!cb);
	ctx.buffer_q_callback = cb;
}

void npu_buffer_q_notify_done(const struct npu_frame *frame)
{
	unsigned long flags = 0;

	set_bit(VS4L_CL_FLAG_DONE, &flags);
	if (frame->result_code) {
		/* Error occurred - Assert invalid flag */
		npu_ufwarn("NDONE flag asserted.\n", frame);
		set_bit(VS4L_CL_FLAG_INVALID, &flags);
	}

	if (npu_if_session_protodrv_ops.queue_done) {
		if (frame->src_queue && frame->input && frame->output) {
			npu_uftrace("Calling npu_queue_done, frame_id = [%u]\n", frame, frame->frame_id);
			npu_if_session_protodrv_ops.queue_done(
				frame->src_queue, frame->input, frame->output, flags);
			npu_ufinfo("Succeeded frame_id = [%u]\n", frame, frame->frame_id);
		} else
			npu_ufinfo("Skip calling queue_done[queue=%pK, in=%pK, out=%pK]\n",
				frame, frame->src_queue, frame->input, frame->output);
	} else {
		npu_warn("not defined: queue_done\n");
	}
}

/* Operations for ncp_mgmt */
int npu_ncp_mgmt_get(struct npu_nw *frame)
{
	BUG_ON(!frame);
	return kfifo_out_spinlocked(&ctx.ncp_mgmt_list, frame, 1, &ctx.ncp_mgmt_lock);
}

int npu_ncp_mgmt_put(const struct npu_nw *frame)
{
	int ret;

	BUG_ON(!frame);
	ret = kfifo_in_spinlocked(&ctx.ncp_mgmt_list, frame, 1, &ctx.ncp_mgmt_lock);
	if (ret > 0) {
		if (ctx.ncp_mgmt_callback)	{
			ctx.ncp_mgmt_callback();
		}
	}
	return ret;
}

int npu_ncp_mgmt_is_available(void)
{
	return !kfifo_is_empty(&ctx.ncp_mgmt_list);
}

void npu_ncp_mgmt_register_cb(LLQ_task_t cb)
{
	npu_info("callback for ncp_mgmt(%pK)\n", cb);
	BUG_ON(!cb);
	ctx.ncp_mgmt_callback = cb;
}

int npu_ncp_mgmt_save_result(
	save_result_func notify_func,
	struct npu_session *sess,
	struct nw_result result)
{
	if (notify_func) {
		npu_dbg("notify_func invoked. sess=%pK, result=0x%08x\n",
			sess, result.result_code);
		return notify_func(sess, result);
	} else {
		npu_warn("not defined: save_result function\n");
		return 0;
	}
}
