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

#include <linux/atomic.h>
#include "npu-util-msgidgen.h"
#include "npu-log.h"

void msgid_pool_init(struct msgid_pool *handle)
{
	int i;
	BUG_ON(!handle);

	for (i = 0; i < NPU_MAX_MSG_ID_CNT; i++)
		atomic_set(&handle->pool[i].occupied, 0);

	handle->magic = MSGID_POOL_MAGIC;
}

/* Returns an unoccupied message ID from pool.
 * returns -1 if there is not message ID available
 */
int msgid_issue(struct msgid_pool *handle)
{
	int i;
	BUG_ON(!handle);
	BUG_ON(handle->magic != MSGID_POOL_MAGIC);

	for (i = 0; i < NPU_MAX_MSG_ID_CNT; i++) {
		if (atomic_cmpxchg(&handle->pool[i].occupied, 0, 1) == 0) {
			/* Find unused */
#ifdef NPU_MAILBOX_MSG_ID_TIME_KEEPING
			do_gettimeofdat(&handle->pool[i].tv_issued);
#endif
			npu_dbg("issue(%d)\n", i);
			return i;
		}
	}
	/* No available MSG ID */
	npu_warn("no message ID available\n");
	return -1;
}

int msgid_issue_save_ref(struct msgid_pool *handle, const int pt_type, void *ref)
{
	int id = msgid_issue(handle);
	npu_dbg("issue_ref(%d)\n", id);
	if (id >= 0) {
		handle->pool[id].ref = ref;
		handle->pool[id].pt_type = pt_type;
	}
	return id;
}

static inline int __msgid_claim(struct msgid_pool *handle, const int msg_id)
{
	int occupied;
	occupied = atomic_xchg(&handle->pool[msg_id].occupied, 0);
	if (!occupied) {
		npu_warn("claim for msg_id(%d), not occupied", msg_id);
		return 1;
	}
	return 0;
}

static inline void __validate_handle_msgid(struct msgid_pool *handle, const int msg_id)
{
	BUG_ON(!handle);
	BUG_ON(handle->magic != MSGID_POOL_MAGIC);

	BUG_ON(msg_id < 0);
	BUG_ON(msg_id >= NPU_MAX_MSG_ID_CNT);
}

/* Claim the issued request id and set to unoccupied status */
void msgid_claim(struct msgid_pool *handle, const int msg_id)
{
	__validate_handle_msgid(handle, msg_id);
	npu_dbg("claim(%d)\n", msg_id);
	__msgid_claim(handle, msg_id);
}

/* Claim the issued request id and set to unoccupied status, and return its associated reference */
void *msgid_claim_get_ref(struct msgid_pool *handle, const int msg_id, const int expected_type)
{
	void *ref;
	__validate_handle_msgid(handle, msg_id);
	npu_dbg("claim_ref(%d)\n", msg_id);
	BUG_ON(expected_type != handle->pool[msg_id].pt_type);

	ref = handle->pool[msg_id].ref;
	if (__msgid_claim(handle, msg_id)) {
		return NULL;
	}

	return ref;
}

int msgid_get_pt_type(struct msgid_pool *handle, const int msg_id)
{
	BUG_ON(!handle);
	__validate_handle_msgid(handle, msg_id);

	if (!atomic_read(&handle->pool[msg_id].occupied)) {
		npu_warn("request pt_type for unoccupied msg_id(%d)", msg_id);
		return -1;
	}
	return handle->pool[msg_id].pt_type;
}
