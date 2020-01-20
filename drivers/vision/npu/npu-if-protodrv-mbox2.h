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

#ifndef _NPU_IF_PROTODRV_MBOX2_H_
#define _NPU_IF_PROTODRV_MBOX2_H_

#include <linux/types.h>
#include <linux/mutex.h>

/* Type definition of signal function of protodrv */
typedef void (*protodrv_notifier)(void *);

#include "npu-config.h"
#include "npu-common.h"
#include "npu-util-msgidgen.h"
#include "npu-protodrv.h"
#include "interface/hardware/mailbox_ipc.h"
#include "interface/hardware/mailbox_msg.h"

struct npu_if_protodrv_mbox_ops {
	int (*frame_result_available)(void);
	int (*frame_post_request)(int msgid, struct npu_frame *frame);
	int (*frame_get_result)(int *ret_msgid, struct npu_frame *frame);
	int (*nw_result_available)(void);
	int (*nw_post_request)(int msgid, struct npu_nw *nw);
	int (*nw_get_result)(int *ret_msgid, struct npu_nw *nw);
	int (*register_notifier)(protodrv_notifier);
	int (*register_msgid_type_getter)(int (*)(int));
};

#ifdef CONFIG_VISION_UNITTEST
/* Allow replacement of interface function link for unit testing */
extern struct npu_if_protodrv_mbox_ops npu_if_protodrv_mbox_ops;
#endif

/* Exported functions */
int npu_mbox_op_register_notifier(protodrv_notifier sig_func);
int npu_mbox_op_register_msgid_type_getter(int (*msgid_get_type_func)(int));
int npu_nw_mbox_op_is_available(void);
int npu_nw_mbox_ops_get(struct msgid_pool *pool, struct proto_req_nw **target);
int npu_nw_mbox_ops_put(struct msgid_pool *pool, struct proto_req_nw *src);
int npu_frame_mbox_op_is_available(void);
int npu_frame_mbox_ops_get(struct msgid_pool *pool, struct proto_req_frame **target);
int npu_frame_mbox_ops_put(struct msgid_pool *pool, struct proto_req_frame *src);

#endif	/* _NPU_IF_PROTODRV_MBOX_H_ */

