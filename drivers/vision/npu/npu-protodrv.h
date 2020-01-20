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

#ifndef _NPU_PROTODRV_H_
#define _NPU_PROTODRV_H_

#include <linux/list.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include "vs4l.h"
#include "vision-buffer.h"
#include "npu-util-liststatemgr.h"
#include "npu-common.h"
#include "npu-if-session-protodrv.h"
#include "npu-util-msgidgen.h"

#define PROTO_DRV_MAGIC_HEAD	0x50524F4F40484452L	/* "PROT@HDR" */
#define PROTO_DRV_MAGIC_TAIL	0x5441494050524F4FL	/* "TAI@PROT" */

/* Constants for Session ref look-up hash table */
#define SESS_REF_HASH_TABLE_SIZE	64
#define SESS_REF_HASH_MAGIC		7
#define SESS_REF_INVALID_HASH_ID	0xFFFFFFFF

enum proto_drv_open_steps {
	PROTO_DRV_OPEN_SETUP_MBOX_MOCK = 0,
	PROTO_DRV_OPEN_SESSION_REF,
	PROTO_DRV_OPEN_IF_SESSION_CTX,
	PROTO_DRV_OPEN_REGISTER_CB,
	PROTO_DRV_OPEN_REGISTER_NOTIFIER,
	PROTO_DRV_OPEN_MSGID_POOL,
	PROTO_DRV_OPEN_FRAME_LSM,
	PROTO_DRV_OPEN_NW_LSM,
	PROTO_DRV_OPEN_AST_CREATE,
	PROTO_DRV_OPEN_AST_START,
	PROTO_DRV_OPEN_COMPLETE
};

typedef enum {
	PROTO_DRV_REQ_TYPE_FRAME = 1,
	PROTO_DRV_REQ_TYPE_NW,
	PROTO_DRV_REQ_TYPE_INVALID,
} proto_drv_req_type_e;

#define LSM_ELEM_STATUS_TRACK_LEN	8
/* All times are ns scale */
struct npu_timestamp_entry {
	lsm_list_type_e	state;
	s64	enter;
	s64	leave;
};

struct npu_timestamp {
	atomic_t			curr_entry;
	s64				init;
	struct npu_timestamp_entry	hist[LSM_ELEM_STATUS_TRACK_LEN];
};

struct proto_req_frame {
	lsm_list_type_e		state;
	struct npu_timestamp	ts;
	/* Session reference list */
	struct list_head	sess_ref_list;

	struct npu_frame	frame;
};

struct proto_req_nw {
	lsm_list_type_e		state;
	struct npu_timestamp	ts;
	/* Session reference list */
	struct list_head	sess_ref_list;

	struct npu_nw		nw;
};

/*
 * Return value
 * isAvailable() : 0 if the queue is empty. > 0 if one or more entries are available.
 * get(..) : 0 if the queue is empty. 1 if a element is fetched.
 * put(..) : 0 if the queue is full. 1 if the enqueue was successful.
 */
struct session_nw_ops {
	int (*is_available)(void);
	int (*get_request)(struct proto_req_nw *target);
	int (*put_result)(const struct proto_req_nw *src);
};

struct session_frame_ops {
	int (*is_available)(void);
	int (*get_request_pair)(struct proto_req_frame *target);
	int (*put_result)(const struct proto_req_frame *src);
};

struct mbox_nw_ops {
	int (*is_available)(void);
	int (*put)(struct proto_req_nw *src);
	int (*get)(struct proto_req_nw **target);
};

struct mbox_frame_ops {
	int (*is_available)(void);
	int (*put)(struct proto_req_frame *src);
	int (*get)(struct proto_req_frame **target);
};


enum protodrv_session_state_e {
	SESSION_REF_STATE_INVALID = 0,
	SESSION_REF_STATE_INACTIVE,
	SESSION_REF_STATE_ACTIVE,
	SESSION_REF_STATE_STOPPING,
};

struct session_ref_entry {
	u32 hash_id;
	enum protodrv_session_state_e s_state;
	npu_uid_t uid;
	const struct npu_session *session;
	struct list_head list;
	struct list_head frame_list;
	struct list_head nw_list;
};

struct session_ref {
	struct session_ref_entry *hash_table[SESS_REF_HASH_TABLE_SIZE];
	struct list_head entry_list;
};

/* Top-level singleton object of proto-drv */
LSM_DECLARE_REF(proto_frame_lsm);
LSM_DECLARE_REF(proto_nw_lsm);
struct npu_proto_drv {
	/* Magic number identifier */
	const u64			magic_head;
	/* Reference to LSM */
	LSM_TYPE(proto_frame_lsm) * frame_lsm;
	LSM_TYPE(proto_nw_lsm) * nw_lsm;
	/* AST to manage LSM */
	struct auto_sleep_thread	ast;
	struct auto_sleep_thread_param	ast_param;
	/* State of proto-drv */
	atomic_t			state;
	/* Request ID generator */
	atomic_t			req_id_gen;
	/* MSG ID pool */
	struct msgid_pool		msgid_pool;
	/* Q structures between session (for debugging purpose) */
	const struct npu_if_session_protodrv_ctx *if_session_ctx;
	/* Reference of npu_device */
	struct npu_device		*npu_device;
	/* Reference of mbox interface */
	struct npu_if_protodrv_mbox_ctx *if_mbox_ctx;
	/* Session reference */
	struct session_ref		session_ref;
	/* Open status (Bitfield of proto_drv_open_steps) */
	unsigned long			open_steps;
	/* Magic number identifier */
	const u64			magic_tail;
};

/*
 * Defining parameters for request ID generation.
 * INITIAL : Initial value of request ID
 * ROLLOVER : Maximum value of request ID. After issuing this ID,
 *            next ID will be INITIAL.
 */
#define NPU_REQ_ID_ROLLOVER	((npu_req_id_t)((1<<20) - 1))
#define NPU_REQ_ID_INITIAL	1

/* Exported functions */
int proto_drv_probe(struct npu_device *npu_device);
int proto_drv_release(void);
int proto_drv_open(struct npu_device *npu_device);
int proto_drv_close(struct npu_device *npu_device);
int proto_drv_start(struct npu_device *npu_device);
int proto_drv_stop(struct npu_device *npu_device);
int proto_req_fault_listener(void);
int session_fault_listener(void);
#endif /* _NPU_PROTODRV_H_ */
