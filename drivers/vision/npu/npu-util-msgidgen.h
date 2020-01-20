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

#ifndef _NPU_UTIL_MSGIDGEN_H_
#define _NPU_UTIL_MSGIDGEN_H_

#include <linux/atomic.h>
#include <linux/time.h>
#include "npu-config.h"

#define MSGID_POOL_MAGIC	0x18D718D7

/* Request ID pool */
struct msgid_pool {
	struct {
		atomic_t occupied;
		int     pt_type;
		void *ref;
#ifdef NPU_MAILBOX_MSG_ID_TIME_KEEPING
		struct  timeval tv_issued;
#endif
	} pool[NPU_MAX_MSG_ID_CNT];
	u32	magic;
};

void msgid_pool_init(struct msgid_pool *handle);
int msgid_issue(struct msgid_pool *handle);
int msgid_issue_save_ref(struct msgid_pool *handle, int pt_type, void *ref);
void msgid_claim(struct msgid_pool *handle, const int msg_id);
void *msgid_claim_get_ref(struct msgid_pool *handle, const int msg_id, const int expected_type);
int msgid_get_pt_type(struct msgid_pool *handle, const int msg_id);
#endif
