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

#ifndef _NPU_IF_SESSION_PROTODRV_H_
#define _NPU_IF_SESSION_PROTODRV_H_

#include <linux/types.h>
#include <linux/atomic.h>

#include "npu-config.h"
#include "npu-common.h"
#include "npu-errno.h"
#include "npu-util-llq.h"	/* For LLQ_task_t definition */

/*
 *  +---------+                       <For network management>                  +-------+
 *  |         |  --npu_ncp_mgmt_put()--> [ncp_mgmt_list] ---npu_buffer_q_get--> |       |
 *  |         |                                                                 | Proto |
 *  | Session |                                                                 |  drv  |
 *  |         |                        <For frame processing>                   |       |
 *  |         | --npu_buffer_q_put()--> [buffer_q_list] --npu_buffer_q_get()--> |       |
 *  |         |                                                                 |       |
 *  |         |                                                                 |       |
 *  +---------+                                                                 +-------+
 *
 */

/* Interface definition to session */
struct npu_if_session_protodrv_ops {
	void (*queue_done)(struct npu_queue *, struct vb_container_list *, struct vb_container_list *, unsigned long);
};

#ifdef CONFIG_VISION_UNITTEST
/* Allow replacement of interface function link for unit testing */
extern struct npu_if_session_protodrv_ops npu_if_session_protodrv_ops;
#endif

/* Context information */
/* Later the DECLARE_KFIFO can be replaced with dynamic allocation
   to preserve the memory when not used */
struct npu_if_session_protodrv_ctx {
	spinlock_t buffer_q_lock;
	DECLARE_KFIFO(buffer_q_list, struct npu_frame, BUFFER_Q_LIST_SIZE);
	LLQ_task_t buffer_q_callback;

	spinlock_t ncp_mgmt_lock;
	DECLARE_KFIFO(ncp_mgmt_list, struct npu_nw, NCP_MGMT_LIST_SIZE);
	LLQ_task_t ncp_mgmt_callback;

	atomic_t	is_opened;
};

/* Initializer and deallocator */
struct npu_if_session_protodrv_ctx *npu_if_session_protodrv_ctx_open(void);
void npu_if_session_protodrv_ctx_close(void);
int npu_if_session_protodrv_is_opened(void);

/* Operations for buffer_q */
int npu_buffer_q_get(struct npu_frame *frame);
int npu_buffer_q_put(const struct npu_frame *frame);
int npu_buffer_q_is_available(void);
void npu_buffer_q_register_cb(LLQ_task_t cb);
void npu_buffer_q_notify_done(const struct npu_frame *frame);

/* Operations for ncp_mgmt */
int npu_ncp_mgmt_get(struct npu_nw *nw);
int npu_ncp_mgmt_put(const struct npu_nw *nw);
int npu_ncp_mgmt_is_available(void);
void npu_ncp_mgmt_register_cb(LLQ_task_t cb);

/* Save result operation */
int npu_ncp_mgmt_save_result(
	save_result_func notify_func,
	struct npu_session *sess,
	struct nw_result result);
#endif	/* _NPU_IF_SESSION_PROTODRV_H_ */
