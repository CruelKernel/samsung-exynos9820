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

#include <linux/kernel.h>
#include <linux/atomic.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/timekeeping.h>
#include "npu-log.h"
#include "npu-protodrv.h"
#include "npu-util-liststatemgr.h"
#include "npu-debug.h"
#include "npu-errno.h"
#include "npu-device.h"
#include "npu-queue.h"
#include "npu-if-protodrv-mbox2.h"
#include "npu-if-session-protodrv.h"
#include "npu-profile.h"

#define NPU_PROTO_DRV_SIZE	1024
#define NPU_PROTO_DRV_NAME	"npu-proto_driver"
#define NPU_PROTO_DRV_FRAME_LSM_NAME	"npu-proto_frame_lsm"
#define NPU_PROTO_DRV_NW_LSM_NAME	"npu-proto_nw_lsm"
#define NPU_PROTO_DRV_AST_NAME		"npu-proto_AST"
#ifdef NPU_LOG_TAG
#undef NPU_LOG_TAG
#endif
#define NPU_LOG_TAG		"proto-drv"

#define UNLINK_CHECK_OWNERSHIP

#define TIME_STAT_BUF_LEN	256

const char *TYPE_NAME_FRAME = "frame";
const char *TYPE_NAME_NW = "Netwrok mgmt.";

/* Print log if driver is idle more than 10 seconds */
const s64 NPU_PROTO_DRV_IDLE_LOG_DELAY_NS = 10L * 1000 * 1000 * 1000;

#ifdef MBOX_MOCK_ENABLE
int deinit_for_mbox_mock(void);
int setup_for_mbox_mock(void);
#endif

/* State management */
typedef enum {
	PROTO_DRV_STATE_UNLOADED = 0,
	PROTO_DRV_STATE_PROBED,
	PROTO_DRV_STATE_OPENED,
	PROTO_DRV_STATE_INVALID,
} proto_drv_state_e;
const char *PROTO_DRV_STATE_NAME[PROTO_DRV_STATE_INVALID + 1]
	= {"UNLOADED", "PROBED", "OPENED", "INVALID"};

static const u8 proto_drv_thread_state_transition[][PROTO_DRV_STATE_INVALID+1] = {
	/* From    -   To	UNLOADED	PROBED		OPENED		INVALID*/
	/* UNLOADED	*/ {	0,		1,		0,		0},
	/* PROBED	*/ {	1,		0,		1,		0},
	/* OPENED	*/ {	1,		1,		0,		0},
	/* INVALID	*/ {	0,		0,		0,		0},
};

/* Declare List State Manager object for frame and nw*/
LSM_DECLARE(proto_frame_lsm, struct proto_req_frame, NPU_PROTO_DRV_SIZE, NPU_PROTO_DRV_FRAME_LSM_NAME);
LSM_DECLARE(proto_nw_lsm, struct proto_req_nw, NPU_PROTO_DRV_SIZE, NPU_PROTO_DRV_NW_LSM_NAME);

/* Definition of proto-drv singleton object */
struct npu_proto_drv npu_proto_drv = {
	.magic_head = PROTO_DRV_MAGIC_HEAD,
	.frame_lsm = &proto_frame_lsm,
	.nw_lsm = &proto_nw_lsm,
	.ast_param = {0},
	.state = ATOMIC_INIT(PROTO_DRV_STATE_UNLOADED),
	.req_id_gen = ATOMIC_INIT(NPU_REQ_ID_INITIAL - 1),
	.if_session_ctx = NULL,
	.npu_device = NULL,
	.session_ref = {
		.hash_table = {NULL},
		.entry_list = LIST_HEAD_INIT(npu_proto_drv.session_ref.entry_list),
	},
	.magic_tail = PROTO_DRV_MAGIC_TAIL,
};

struct proto_drv_ops {
	struct session_nw_ops		session_nw_ops;
	struct session_frame_ops	session_frame_ops;
	struct mbox_nw_ops		mbox_nw_ops;
	struct mbox_frame_ops		mbox_frame_ops;
};

/*******************************************************************************
 * State management functions
 *
 */
static proto_drv_state_e state_transition(proto_drv_state_e new_state)
{
	proto_drv_state_e old_state;

	BUG_ON(new_state < 0);
	BUG_ON(new_state >= PROTO_DRV_STATE_INVALID);

	old_state = atomic_xchg(&npu_proto_drv.state, new_state);

	/* Check after transition is made - To ensure atomicity */
	if (!proto_drv_thread_state_transition[old_state][new_state]) {
		npu_err("Invalid transition [%s] -> [%s]\n",
			PROTO_DRV_STATE_NAME[old_state],
			PROTO_DRV_STATE_NAME[new_state]);
		BUG_ON(1);
	}
	return old_state;
}

static inline proto_drv_state_e get_state(void)
{
	return atomic_read(&npu_proto_drv.state);
}

#define IS_TRANSITABLE(TARGET) \
({										\
	proto_drv_state_e __curr_state = get_state();				\
	proto_drv_state_e __target_state = (TARGET);				\
	u8 __ret;									\
	BUG_ON(__target_state < 0);						\
	BUG_ON(__target_state >= PROTO_DRV_STATE_INVALID);			\
	__ret = proto_drv_thread_state_transition[__curr_state][__target_state];	\
	if (!__ret)								\
		npu_err("Invalid transition (%s) -> (%s)\n",			\
			PROTO_DRV_STATE_NAME[__curr_state],			\
			PROTO_DRV_STATE_NAME[__target_state]);			\
	__ret;									\
})

#define EXPECT_STATE(STATE) \
({										\
	proto_drv_state_e __curr_state = get_state();				\
	proto_drv_state_e __expect_state = (STATE);				\
	BUG_ON(__expect_state < 0);						\
	BUG_ON(__expect_state >= PROTO_DRV_STATE_INVALID);			\
	if (__curr_state != __expect_state)					\
		npu_err("Requires state (%s), but current state is (%s)\n",	\
			PROTO_DRV_STATE_NAME[__expect_state],			\
			PROTO_DRV_STATE_NAME[__curr_state]);			\
	(__curr_state == __expect_state);					\
})

/*******************************************************************************
 * Utility functions
 *
 */
static inline s64 get_time_ns(void)
{
	return ktime_to_ns(ktime_get_boottime());
}

static inline const char *getTypeName(const proto_drv_req_type_e type)
{
	switch (type) {
	case PROTO_DRV_REQ_TYPE_FRAME:
		return TYPE_NAME_FRAME;
	case PROTO_DRV_REQ_TYPE_NW:
		return TYPE_NAME_NW;
	case PROTO_DRV_REQ_TYPE_INVALID:
	default:
		return "INVALID";
	}
}

/* Look-up table for NW names */
static const char* NW_CMD_NAMES[NPU_NW_CMD_END - NPU_NW_CMD_BASE] = {
	NULL,			/* For NPU_NW_CMD_BASE */
	"LOAD",
	"UNLOAD",
	"STREAM_ON",
	"STREAM_OFF",
	"POWER_DOWN",
	"PROFILE_START",
	"PROFILE_STOP",
	"FW_TC_EXECUTE",
	"CLEAR_CB",
};
/* Convinient function to get stringfy name of command */
static const char* __cmd_name(const u32 cmd)
{
	u32 idx = cmd - NPU_NW_CMD_BASE;
	BUG_ON(idx < 0);
	BUG_ON(idx >= ARRAY_SIZE(NW_CMD_NAMES));
	BUG_ON(!NW_CMD_NAMES[idx]);

	return NW_CMD_NAMES[idx];
}


/*******************************************************************************
 * Session reference management
 *
 */
static void reset_session_ref(void)
{
	struct session_ref *sess_ref = &(npu_proto_drv.session_ref);

	/* Clear to Zero and initialize list head */
	memset(sess_ref, 0, sizeof(*sess_ref));
	INIT_LIST_HEAD(&sess_ref->entry_list);
}

static u32 __find_hash_id(const npu_uid_t uid)
{
	u32 hash_id;
	u32 add_val = 0;
	struct session_ref *sess_ref = &(npu_proto_drv.session_ref);
	struct session_ref_entry *sess_entry;

	/* Retrive hash_id from session uid */
	hash_id = uid % SESS_REF_HASH_TABLE_SIZE;
	for (add_val = 0; add_val < SESS_REF_HASH_TABLE_SIZE; add_val += SESS_REF_HASH_MAGIC) {
		hash_id = (hash_id + add_val) % SESS_REF_HASH_TABLE_SIZE;
		sess_entry = sess_ref->hash_table[hash_id];
		if (sess_entry != NULL) {
			if (sess_entry->uid == uid) {
				/* Match */
				return hash_id;
			}
		}
	}

	/* Failed to find */
	return SESS_REF_INVALID_HASH_ID;
}

/* Return 0 if the nw object is not belong to any session.
 * Currently, POWER_DOWN and Profiling commands are not belong to session.
 */
static int is_belong_session(const struct npu_nw *nw)
{
	BUG_ON(!nw);

	switch (nw->cmd) {
	case NPU_NW_CMD_POWER_DOWN:
	case NPU_NW_CMD_PROFILE_START:
	case NPU_NW_CMD_PROFILE_STOP:
	case NPU_NW_CMD_FW_TC_EXECUTE:
		return 0;
	default:
		return 1;
	}
}

/*
 * Return true for errcode whose proto_req_* object need to be
 * kept on STUCKED state (To prepare out-of-order msgid arrival)
 */
static int is_stucked_result_code(const npu_errno_t result_code)
{
	/* Determine by its result code */
	switch (NPU_ERR_CODE(result_code)) {
	case NPU_ERR_NPU_TIMEOUT:
	case NPU_ERR_QUEUE_TIMEOUT:
		return 1;
	default:
		return 0;
	}
}

static int is_stucked_req_frame(const struct proto_req_frame *req_frame)
{
	return is_stucked_result_code(req_frame->frame.result_code);
}

static int is_stucked_req_nw(const struct proto_req_nw *req_nw)
{
	return is_stucked_result_code(req_nw->nw.result_code);
}

/* Dump the session reference to npu log */
static void log_session_ref(void)
{
	struct session_ref		*sess_ref = &(npu_proto_drv.session_ref);
	struct session_ref_entry	*sess_entry;
	const struct npu_session	*sess;
	int				session_cnt = 0;
	u64				u_pid;

	BUG_ON(!sess_ref);

	npu_info("Session state list =============================\n");
	list_for_each_entry(sess_entry, &sess_ref->entry_list, list) {
		npu_info("Entry[%d] : UID[%u] state[%d] frame[%s] nw[%s]\n",
			session_cnt, sess_entry->uid, sess_entry->s_state,
			(list_empty(&sess_entry->frame_list)) ? "EMPTY" : "NOT_EMPTY",
			(list_empty(&sess_entry->nw_list)) ? "EMPTY" : "NOT_EMPTY");

		sess = sess_entry->session;
		if (sess) {
			u_pid = (u64)sess->pid;
			npu_info("Entry[%d] : Session UID[%u] PID[%llu] frame_id[%u]\n",
				session_cnt, sess->uid, u_pid, sess->frame_id);
		} else {
			npu_info("Entry[%d] : NULL Session\n", session_cnt);
		}

		session_cnt++;
	}
	npu_info("End of session state list [%d] entries =========\n", session_cnt);
}

static struct session_ref_entry *__find_session_ref(const npu_uid_t uid)
{
	u32 hash_id;
	struct session_ref *sess_ref = &(npu_proto_drv.session_ref);
	struct session_ref_entry *sess_entry;

	/* Try search on hash table */
	hash_id = __find_hash_id(uid);
	if (hash_id != SESS_REF_INVALID_HASH_ID) {
		/* Find on hash table */
		sess_entry = sess_ref->hash_table[hash_id];
		goto find_entry;
	}

	/* No entry found on hash table -> Look up the linked list */
	npu_warn("UID=%u cannot be found on hashtable. Lookup on linked list.\n", uid);
	list_for_each_entry(sess_entry, &sess_ref->entry_list, list) {
		if (sess_entry->uid == uid) {
			/* Match */
			goto find_entry;
		}
	}
	npu_warn("UID=%u cannot be found on hash and linked list.\n", uid);
	return NULL;

find_entry:
	/* Now the sess_entry points valid session_ref_entry to session */
	return sess_entry;
}

static struct session_ref_entry *find_session_ref_frame(const struct proto_req_frame *req_frame)
{
	const struct npu_frame *frame;

	BUG_ON(!req_frame);

	frame = &req_frame->frame;
	BUG_ON(!frame);

	return __find_session_ref(frame->uid);
}

static struct session_ref_entry *find_session_ref_nw(const struct proto_req_nw *req_nw)
{
	const struct npu_nw *nw;

	BUG_ON(!req_nw);

	nw = &req_nw->nw;
	BUG_ON(!nw);
	npu_trace("cmd:%u / nw->uid : %u\n", nw->cmd, nw->uid);

	if (is_belong_session(nw)) {
		return __find_session_ref(nw->uid);
	} else {
		/* REQ_NW not belong to session */
		return NULL;
	}
}

static enum protodrv_session_state_e get_session_ref_state_frame(const struct proto_req_frame *req_frame)
{
	struct session_ref_entry *ref;

	ref = find_session_ref_frame(req_frame);
	if (ref == NULL) {
		return SESSION_REF_STATE_INVALID;
	} else {
		return ref->s_state;
	}
}

static enum protodrv_session_state_e get_session_ref_state_nw(const struct proto_req_nw *req_nw)
{
	struct session_ref_entry *ref;

	ref = find_session_ref_nw(req_nw);
	if (ref == NULL) {
		return SESSION_REF_STATE_INVALID;
	} else {
		return ref->s_state;
	}
}

static struct session_ref_entry *alloc_session_ref_entry(const struct npu_session *sess)
{
	struct session_ref_entry *entry;

	BUG_ON(!sess);

	/* TODO: Use slab allocator */
	entry = kmalloc(sizeof(*entry), GFP_ATOMIC);
	if (entry == NULL) {
		return NULL;
	}
	memset(entry, 0, sizeof(*entry));
	entry->hash_id = SESS_REF_INVALID_HASH_ID;
	entry->s_state = SESSION_REF_STATE_INVALID;
	entry->uid = sess->uid;
	entry->session = sess;
	INIT_LIST_HEAD(&entry->list);
	INIT_LIST_HEAD(&entry->frame_list);
	INIT_LIST_HEAD(&entry->nw_list);

	return entry;
}

static void free_session_ref_entry(struct session_ref_entry *sess_entry)
{
	if (sess_entry == NULL) {
		npu_warn("free request for null pointer.\n");
		return;
	}
	kfree(sess_entry);
}

int register_session_ref(struct npu_session *sess)
{
	u32 hash_id;
	u32 add_val = 0;
	struct session_ref *sess_ref = &(npu_proto_drv.session_ref);
	struct session_ref_entry *sess_entry = NULL;

	BUG_ON(!sess);

	/* Create entry */
	sess_entry = alloc_session_ref_entry(sess);
	if (sess_entry == NULL) {
		npu_uerr("fail in alloc_session_ref_entry\n", sess);
		return -ENOMEM;
	}

	/* Find free entry in hash table */
	hash_id = (sess->uid) % SESS_REF_HASH_TABLE_SIZE;
	for (add_val = 0; add_val < SESS_REF_HASH_TABLE_SIZE; add_val += SESS_REF_HASH_MAGIC) {
		hash_id = (hash_id + add_val) % SESS_REF_HASH_TABLE_SIZE;

		if (sess_ref->hash_table[hash_id] == NULL) {
			/* Find room */
			npu_udbg("Register UID at hashtable[%u]\n", sess, hash_id);
			sess_ref->hash_table[hash_id] = sess_entry;
			sess_entry->hash_id = hash_id;
			break;
		}
	}
	if (add_val >= SESS_REF_HASH_TABLE_SIZE) {
		/* Hash table is not available even thought it is registered */
		npu_uwarn("UID cannot be stored on hash table.\n", sess);
	}

	list_add_tail(&sess_entry->list, &sess_ref->entry_list);
	npu_udbg("session ref @%pK registered.\n", sess, sess_entry);

	return 0;
}

int drop_session_ref(const npu_uid_t uid)
{
	u32 hash_id;
	struct session_ref *sess_ref = &(npu_proto_drv.session_ref);
	struct session_ref_entry *sess_entry = NULL;

	sess_entry = __find_session_ref(uid);
	if (sess_entry == NULL) {
		npu_err("cannot found session reference for UID [%u].\n", uid);
		return -EINVAL;
	}

	/* Remove from hash table */
	hash_id = __find_hash_id(uid);
	if (hash_id != SESS_REF_INVALID_HASH_ID) {
		/* Found on hash table */
		BUG_ON(sess_entry != sess_ref->hash_table[hash_id]);
		sess_ref->hash_table[hash_id] = NULL;
	}
	/* Check entries */
	if (!list_empty(&sess_entry->frame_list)) {
		npu_warn("frame list for UID [%u] is not empty. First is at %pK\n",
			uid, list_first_entry_or_null(&sess_entry->frame_list, struct proto_req_frame, sess_ref_list));
	}
	if (!list_empty(&sess_entry->nw_list)) {
		npu_warn("network mgmt. list for UID [%u] is not empty. First is at %pK\n",
			uid, list_first_entry_or_null(&sess_entry->nw_list, struct proto_req_nw, sess_ref_list));
	}
	npu_info("[U%u]session ref @%pK dropped.\n", uid, sess_entry);

	/* Remove from Linked list */
	list_del_init(&sess_entry->list);

	/* Free entry */
	free_session_ref_entry(sess_entry);

	return 0;
}

static int is_list_used(struct list_head *list)
{
	if (!list_empty(list)) {
		/* next is not self and next is not null -> Belong to other list */
		if (list->next != NULL) {
			return 1;
		}
	}
	return 0;
}

int link_session_frame(struct proto_req_frame *req_frame)
{
	const struct npu_frame *frame;
	struct session_ref_entry *sess_entry = NULL;

	BUG_ON(!req_frame);

	frame = &req_frame->frame;

	sess_entry = find_session_ref_frame(req_frame);
	if (sess_entry == NULL) {
		npu_uferr("cannot found session ref.\n", frame);
		return -EINVAL;
	}
	if (is_list_used(&req_frame->sess_ref_list)) {
		npu_uerr("frame seemed to belong other session. next pointer is %pK\n"
			, frame, req_frame->sess_ref_list.next);
		return -EFAULT;
	}
	INIT_LIST_HEAD(&req_frame->sess_ref_list);
	list_add_tail(&req_frame->sess_ref_list, &sess_entry->frame_list);
	npu_ufdbg("frame linked to ref@%pK.\n", frame, sess_entry);
	return 0;
}

int link_session_nw(struct proto_req_nw *req_nw)
{
	const struct npu_nw *nw;
	struct session_ref_entry *sess_entry = NULL;

	BUG_ON(!req_nw);

	nw = &req_nw->nw;

	if (!is_belong_session(nw)) {
		/* No link necessary */
		npu_uinfo("NW no need to linked to session reference.\n", nw);
		return 0;
	}

	sess_entry = find_session_ref_nw(req_nw);
	if (sess_entry == NULL) {
		npu_uerr("cannot found session ref.\n", nw);
		return -EINVAL;
	}
	if (is_list_used(&req_nw->sess_ref_list)) {
		npu_uerr("network mgmt. seemed to belong other session. next pointer is %pK\n"
			, nw, req_nw->sess_ref_list.next);
		return -EFAULT;
	}
	INIT_LIST_HEAD(&req_nw->sess_ref_list);
	list_add_tail(&req_nw->sess_ref_list, &sess_entry->nw_list);
	npu_udbg("NW linked to ref@%pK.\n", nw, sess_entry);
	return 0;
}

int unlink_session_frame(struct proto_req_frame *req_frame)
{
#ifdef UNLINK_CHECK_OWNERSHIP
	/* This check is somewhat time consuming operation */
	struct session_ref_entry *sess_entry = NULL;
	struct proto_req_frame *iter_frame;
	const struct npu_frame *frame;

	BUG_ON(!req_frame);

	frame = &req_frame->frame;

	sess_entry = find_session_ref_frame(req_frame);
	if (sess_entry == NULL) {
		npu_uferr("cannot found session ref.\n", frame);
		return -EINVAL;
	}
	list_for_each_entry(iter_frame, &sess_entry->frame_list, sess_ref_list) {
		if (iter_frame == req_frame) {
			goto found_match;
		}
	}
	npu_uferr("frame does not belong to session ref@%pK.\n", frame, sess_entry);
	return -EINVAL;

found_match:
#endif /* UNLINK_CHECK_OWNERSHIP */
	list_del_init(&req_frame->sess_ref_list);
	npu_ufdbg("frame unlinked from ref@%pK.\n", frame, sess_entry);
	return 0;
}

int unlink_session_nw(struct proto_req_nw *req_nw)
{
#ifdef UNLINK_CHECK_OWNERSHIP
	/* This check is somewhat time consuming operation */
	struct session_ref_entry *sess_entry = NULL;
	struct proto_req_nw *iter_nw;
	const struct npu_nw *nw;

	BUG_ON(!req_nw);

	nw = &req_nw->nw;

	if (!is_belong_session(nw)) {
		/* No unlink necessary */
		npu_uinfo("NW no need to be unlinked from session reference.\n", nw);
		return 0;
	}

	sess_entry = find_session_ref_nw(req_nw);
	if (sess_entry == NULL) {
		npu_uerr("cannot found session ref.\n", nw);
		return -EINVAL;
	}
	list_for_each_entry(iter_nw, &sess_entry->nw_list, sess_ref_list) {
		if (iter_nw == req_nw) {
			goto found_match;
		}
	}
	npu_uerr("network mgmt. does not belong to session ref@%pK.\n", nw, sess_entry);
	return -EINVAL;

found_match:
#endif /* UNLINK_CHECK_OWNERSHIP */
	list_del_init(&req_nw->sess_ref_list);
	npu_uinfo("NW unlinked from ref@%pK.\n", nw, sess_entry);
	return 0;
}

/*
 * Nullify src_queue(for frame) and notify_func(for nw)
 * for all requests linked with session reference for specified nw
 * (But do not clear notify_func for req_nw itself)
 * to make no more notificatin sent for this session.
 */
static int force_clear_cb(struct proto_req_nw *req_nw)
{
	struct session_ref_entry	*sess_entry = NULL;
	struct proto_req_nw		*iter_nw;
	struct proto_req_frame		*iter_frame;
	struct npu_nw			*nw;
	int				cnt;

	BUG_ON(!req_nw);

	nw = &req_nw->nw;

	sess_entry = find_session_ref_nw(req_nw);
	if (sess_entry == NULL) {
		npu_uerr("cannot found session ref.\n", nw);
		return -ENOENT;
	}

	/* Clear callback from associated frame list */
	cnt = 0;
	list_for_each_entry(iter_frame, &sess_entry->frame_list, sess_ref_list) {
		npu_ufinfo("Reset src_queue for frame in state [%d]\n",
			&iter_frame->frame, iter_frame->state);
		iter_frame->frame.src_queue = NULL;
		cnt++;
	}
	npu_uinfo("Clear src_queue for frame: %d entries\n", nw, cnt);

	cnt = 0;
	list_for_each_entry(iter_nw, &sess_entry->nw_list, sess_ref_list) {
		/* Do not clear CB for req_nw itself */
		if (iter_nw != req_nw)	{
			npu_uinfo("Reset notify_func for nw in state [%d]\n",
				&iter_nw->nw, iter_nw->state);
			iter_nw->nw.notify_func = NULL;
			cnt++;
		}
	}
	npu_uinfo("Clear notify_func for nw: %d entries\n", nw, cnt);

	return 0;
}

/*
 * Returns 1 if the specified npu_nw object is the only linked to its associated session.
 * Otherwise, returns 0.
 */
int is_last_session_ref(struct proto_req_nw *req_nw)
{
	struct session_ref_entry *sess_entry = NULL;
	struct proto_req_nw *iter_nw;
	const struct npu_nw *nw;
	int ret;

	BUG_ON(!req_nw);

	nw = &req_nw->nw;

	sess_entry = find_session_ref_nw(req_nw);
	if (sess_entry == NULL) {
		npu_uwarn("cannot found session ref.\n", nw);
		return 0;
	}
	/* No frame shall be associated */
	if (!list_empty(&sess_entry->frame_list)) {
		return 0;	/* FALSE */
	}
	/* Only one frame shall be available, and it should be nw */
	ret = 0;
	list_for_each_entry(iter_nw, &sess_entry->nw_list, sess_ref_list) {
		if (iter_nw == req_nw)	{
			ret = 1;	/* TRUE, but need to check other entry */
		} else {
			return 0;	/* Other entry except nw -> FALSE */
		}
	}
	return ret;
}

/* Returns 0 if no session ref is exist.
 * Otherwise, return 1
 */
static int is_session_ref_exist(void)
{
	struct session_ref *sess_ref = &(npu_proto_drv.session_ref);

	return !list_empty(&sess_ref->entry_list);
}

/* Iniitialize all entry's sess_ref_list on start up.
 * This macro is called on npu_protodrv_open()
 */
#define NPU_PROTODRV_LSM_ENTRY_INIT(LSM_NAME, TYPE)		\
do {								\
	TYPE *__entry;						\
	LSM_FOR_EACH_ENTRY_IN(LSM_NAME, FREE, __entry,		\
		memset(__entry, 0, sizeof(*__entry));		\
		INIT_LIST_HEAD(&__entry->sess_ref_list);	\
		atomic_set(&__entry->ts.curr_entry, 0);		\
	)							\
} while (0)

/*******************************************************************************
 * NPU data handling functions
 */


static inline int check_time(const struct npu_timestamp *tstamp, const u64 timeout_ns, const u64 now_ns)
{
	u64 diff;
	const struct npu_timestamp_entry *ts_entry;

	if (timeout_ns == 0) {
		return 0;	/* No timeout defined */
	}
	ts_entry = &(tstamp->hist[atomic_read(&tstamp->curr_entry)]);
	diff = now_ns - ts_entry->enter;
	return diff > timeout_ns;
}

/*
 * Returns true if request's ts.curr exceedes specified timeout.
 * (PROTO_REQ_PTR can be both proto_req_frame* or proto_req_nw*)
 * Otherwise, return false.
 */
#define is_timedout(PROTO_REQ_PTR, timeout_ns, now_ns)	\
({							\
	typeof(PROTO_REQ_PTR) __p = (PROTO_REQ_PTR);	\
	BUG_ON(!__p);					\
	check_time(&__p->ts, (timeout_ns), (now_ns));	\
})							\

/*
 * Generate a unique npu_req_id and return it.
 * Current implementation implies the npu_req_id_gen is
 * signed integer type.
 */
static npu_req_id_t get_next_npu_req_id(void)
{
	/* handling roll over
	   The initial value is NPU_REQ_ID_INITIAL - 1 because
	   the return value is made by current counter + 1 */
	atomic_cmpxchg(&npu_proto_drv.req_id_gen, NPU_REQ_ID_ROLLOVER, (NPU_REQ_ID_INITIAL - 1));

	/* return next count */
	return (npu_req_id_t)atomic_inc_return(&npu_proto_drv.req_id_gen);
}

/*******************************************************************************
 * Timestamp handling functions
 */
static void __update_npu_timestamp(const lsm_list_type_e old_state, const lsm_list_type_e new_state, struct npu_timestamp *ts)
{
	int ts_entry_idx_curr, ts_entry_idx_next;
	s64 now = get_time_ns();

	BUG_ON(!ts);

	do {
		ts_entry_idx_curr = atomic_read(&ts->curr_entry);
		ts_entry_idx_next = (ts_entry_idx_curr + 1) % LSM_ELEM_STATUS_TRACK_LEN;
	} while (atomic_cmpxchg(&ts->curr_entry, ts_entry_idx_curr, ts_entry_idx_next) != ts_entry_idx_curr);

	if (ts->hist[ts_entry_idx_curr].state != old_state) {
		npu_warn("history index(%d)'s state(%d) does not match with last state (%d).",
			ts_entry_idx_curr, ts->hist[ts_entry_idx_curr].state, old_state);
	}
	ts->hist[ts_entry_idx_curr].leave = ts->hist[ts_entry_idx_next].enter = now;
	ts->hist[ts_entry_idx_next].state = new_state;
}

/* Print timestamp history, until it gets FREE state */
static char *__print_npu_timestamp(struct npu_timestamp *ts, char *buf, const size_t len)
{
	s64 now = get_time_ns();
	int i, ret;
	int idx, tmp_idx;
	int idx_start;
	int buf_idx = 0;
	struct timespec ts_time;

	BUG_ON(!ts);
	idx_start = atomic_read(&ts->curr_entry);

	if (idx_start < 0 || idx_start >= LSM_ELEM_STATUS_TRACK_LEN) {
		npu_err("invalid curr_entry (%d)\n", idx_start);
		ret = scnprintf(buf + buf_idx, len - buf_idx, "<Invalid curr_entry (%d)>", idx_start);
		buf_idx += ret;
		goto ret_exit;
	}

	/* Back track to start(FREE) state */
	idx = idx_start;
	while (ts->hist[idx].state != FREE) {
		tmp_idx = idx - 1;
		if (tmp_idx < 0)
			tmp_idx = LSM_ELEM_STATUS_TRACK_LEN - 1;
		if (tmp_idx == idx_start) {
			/* Not enough history to FREE */
			ret = scnprintf(buf + buf_idx, len - buf_idx, "<Not enough history>");
			buf_idx += ret;
			break;
		}

		idx = tmp_idx;
	}

	/* printout history */
	i = idx;
	while (1) {
		ts_time = ns_to_timespec(ts->hist[i].enter);
		ret = scnprintf(buf + buf_idx, len - buf_idx, "[%s](%ld.%09ld) ",
			LSM_STATE_NAMES[ts->hist[i].state], ts_time.tv_sec, ts_time.tv_nsec);
		if (!ret)
			break;

		buf_idx += ret;

		if (i == idx_start)
			break;	/* This is normal termination condition */
		i = (i + 1) % LSM_ELEM_STATUS_TRACK_LEN;
	}

	ts_time = ns_to_timespec(now);
	scnprintf(buf + buf_idx, len - buf_idx, "[DONE](%ld.%09ld) ",
		ts_time.tv_sec, ts_time.tv_nsec);
ret_exit:
	buf[len - 1] = '\0';

	return buf;
}

int session_fault_listener(void)
{
	#define mn_state_f(x)	#x
	struct session_ref_entry *sess_entry = NULL;
	struct session_ref *sess_ref = &(npu_proto_drv.session_ref);
	struct proto_req_frame *iter_frame;
	struct proto_req_nw *iter_nw;
	struct npu_frame *frame;
	struct npu_nw	*nw;

	struct npu_session *session;

	struct av_info *IFM_info;
	struct av_info *OFM_info;
	struct addr_info *IFM_addr;
	struct addr_info *OFM_addr;
	struct addr_info *IMB_addr;
	struct addr_info *WGT_addr;

	int qbuf_IOFM_idx;
	int dqbuf_IOFM_idx;

	size_t IFM_cnt;
	size_t OFM_cnt;
	size_t IMB_cnt;
	size_t WGT_cnt;

	int session_processing_cnt = 0;

	size_t idx0 = 0;
	size_t idx1 = 0;
	size_t idx2 = 0;

	int session_chk[NPU_MAX_SESSION] = {0};
	u32 session_uid;
	u32 req_cnt = 0;

	idx2 = 0;
	list_for_each_entry(sess_entry, &sess_ref->entry_list, list) {
		list_for_each_entry(iter_nw, &sess_entry->nw_list, sess_ref_list) {
			if (iter_nw->state == PROCESSING)
				req_cnt++;
		}
		list_for_each_entry(iter_frame, &sess_entry->frame_list, sess_ref_list) {
			if (iter_frame->state == PROCESSING)
				req_cnt++;
		}
	}

	npu_info("<Outstanding requeset [%u]>\n", req_cnt);
	npu_info("[Index] [Type] [KVA] [DVA] [Size]\n");

	idx2 = 0;
	list_for_each_entry(sess_entry, &sess_ref->entry_list, list) {
		list_for_each_entry(iter_nw, &sess_entry->nw_list, sess_ref_list) {
			if (iter_nw->state == PROCESSING) {
				nw = &iter_nw->nw;
				npu_info("[%zu] Type:NW UID:%u ReqID:%u FrameID:N/A CMD:%d MsgID:%d\n",
					idx2, nw->uid, nw->npu_req_id, nw->cmd, nw->msgid);
				idx2++;
			}
		}
		list_for_each_entry(iter_frame, &sess_entry->frame_list, sess_ref_list) {
			if (iter_frame->state == PROCESSING) {
				frame = &iter_frame->frame;
				npu_info("[%zu] Type:FRAME UID:%u ReqID:%u FrameID:%u CMD:%d MsgID:%d\n",
					idx2, frame->uid, frame->npu_req_id, frame->frame_id, frame->cmd, frame->msgid);
				IFM_info = &frame->IFM_info[0];
				OFM_info = &frame->OFM_info[0];
				IFM_addr = IFM_info->addr_info;
				OFM_addr = OFM_info->addr_info;
				IFM_cnt = IFM_info->address_vector_cnt;
				OFM_cnt = OFM_info->address_vector_cnt;

				for (idx1 = 0; idx1 < IFM_cnt; idx1++) {
					npu_info("- %u %d %pK %pad %zu\n",
						(IFM_addr + idx1)->av_index, MEMORY_TYPE_IN_FMAP,
						(IFM_addr + idx1)->vaddr, &((IFM_addr + idx1)->daddr),
						(IFM_addr + idx1)->size);
				}
				for (idx1 = 0; idx1 < OFM_cnt; idx1++) {
					npu_info("- %u %d %pK %pad %zu\n",
						(OFM_addr + idx1)->av_index, MEMORY_TYPE_OT_FMAP,
						(OFM_addr + idx1)->vaddr, &((OFM_addr + idx1)->daddr),
						(OFM_addr + idx1)->size);
				}
				idx2++;
			}
		}
	}

	idx2 = 0;
	list_for_each_entry(sess_entry, &sess_ref->entry_list, list) {
		list_for_each_entry(iter_frame, &sess_entry->frame_list, sess_ref_list) {
			if (iter_frame->state == PROCESSING) {
				frame = &iter_frame->frame;
				session = frame->session;
				if (!session)
					continue;

				session_uid = session->uid;
				session_uid++;
				for (idx2 = 0; idx2 < NPU_MAX_SESSION; idx2++) {
					if (session_chk[idx2] != session_uid)
						session_chk[idx2] = session_uid;
				}
			}
		}
	}
	for (idx2 = 0; idx2 < NPU_MAX_SESSION; idx2++) {
		if (session_chk[idx2] == 0) {
			session_processing_cnt = idx2;
			break;
		}
	}

	npu_info("<Session information [%d]>\n", session_processing_cnt);
	npu_info("[Index] [Type] [KVA] [DVA] [Size]\n");

	idx2 = 0;
	list_for_each_entry(sess_entry, &sess_ref->entry_list, list) {
		list_for_each_entry(iter_frame, &sess_entry->frame_list, sess_ref_list) {
			if (iter_frame->state == PROCESSING) {
				frame = &iter_frame->frame;
				session = frame->session;
				if (!session)
					continue;

				session_uid = session->uid;
				session_uid++;
				for (idx2 = 0; idx2 < NPU_MAX_SESSION; idx2++) {
					if (session_chk[idx2] == session_uid)
						continue;
				}
				session_uid--;

				qbuf_IOFM_idx = session->qbuf_IOFM_idx;
				dqbuf_IOFM_idx = session->dqbuf_IOFM_idx;
				npu_info("[%zu] UID:%d q/dq_buf_idx(%d/%d)\n",
					idx2, session_uid, qbuf_IOFM_idx, dqbuf_IOFM_idx);

				IFM_cnt = session->IFM_cnt;
				OFM_cnt = session->OFM_cnt;
				IMB_cnt = session->IMB_cnt;
				WGT_cnt = session->WGT_cnt;

				for (idx0 = 0; idx0 < VISION_MAX_BUFFER; idx0++) {
					IFM_info = &session->IFM_info[idx0];
					OFM_info = &session->OFM_info[idx0];
					IFM_addr = IFM_info->addr_info;
					OFM_addr = OFM_info->addr_info;

					for (idx1 = 0; idx1 < IFM_cnt; idx1++) {
						npu_info("- %zu %d %pK %pad %zu\n",
							idx1, MEMORY_TYPE_IN_FMAP, (IFM_addr + idx1)->vaddr,
							&((IFM_addr + idx1)->daddr), (IFM_addr + idx1)->size);
					}
					for (idx1 = 0; idx1 < OFM_cnt; idx1++) {
						npu_info("- %zu %d %pK %pad %zu\n",
							idx1, MEMORY_TYPE_OT_FMAP, (OFM_addr + idx1)->vaddr,
							&((OFM_addr + idx1)->daddr), (OFM_addr + idx1)->size);
					}
				}
				IMB_addr = session->IMB_info;
				WGT_addr = session->WGT_info;
				for (idx1 = 0; idx1 < IMB_cnt; idx1++) {
					npu_info("- %zu %d %pK %pad %zu\n",
						idx1, MEMORY_TYPE_IM_FMAP, (IMB_addr + idx1)->vaddr,
						&((IMB_addr + idx1)->daddr), (IMB_addr + idx1)->size);
				}
				for (idx1 = 0; idx1 < WGT_cnt; idx1++) {
					npu_info("- %zu %d %pK %pad %zu\n",
						idx1, MEMORY_TYPE_WEIGHT, (WGT_addr + idx1)->vaddr,
						&((WGT_addr + idx1)->daddr), (WGT_addr + idx1)->size);
				}
				idx2++;
			}
		}
	}
	return 0;
}

int proto_req_fault_listener(void)
{
	#define mn_state_f(x)	#x
	struct session_ref_entry *sess_entry = NULL;
	struct session_ref *sess_ref = &(npu_proto_drv.session_ref);
	struct proto_req_frame *iter_frame;
	struct proto_req_nw *iter_nw;
	struct npu_frame *frame;
	struct npu_nw	*nw;
	int ss_Cnt = 0;
	int fr_Cnt = 0;
	int nw_Cnt = 0;
	char *mn_state;
	char stat_buf[TIME_STAT_BUF_LEN];
	size_t i = 0;

	list_for_each_entry(sess_entry, &sess_ref->entry_list, list) {
		list_for_each_entry(iter_frame, &sess_entry->frame_list, sess_ref_list) {
			frame = &iter_frame->frame;
			mn_state = mn_state_f(iter_frame->state);
			npu_err("(TIMEOUT):%s (%s): ss_cnt[%d] nFrame[%d] uid[%u] fid[%u] a_vec_cnt[%u] a_vec_st_daddr[%pad] NPU-TIMESTAT:%s\n",
					TYPE_NAME_FRAME,
					mn_state,
					ss_Cnt,
					fr_Cnt,
					frame->uid,
					frame->frame_id,
					frame->mbox_process_dat.address_vector_cnt,
					&(frame->mbox_process_dat.address_vector_start_daddr),
					__print_npu_timestamp(&iter_frame->ts, stat_buf, TIME_STAT_BUF_LEN));

			if (iter_frame->state == PROCESSING) {
				npu_err("FRAME info:\n");
				do {
					printk("%08x ", *(volatile u32 *)(frame->session->IOFM_mem_buf->vaddr +
						(sizeof(64) * i)));
					i++;
					if ((i % 4) == 0)
					       printk("\n");
				} while (sizeof(u32) * i < frame->session->IOFM_mem_buf->size);
			}
			fr_Cnt++;
		}
		list_for_each_entry(iter_nw, &sess_entry->nw_list, sess_ref_list) {
			nw = &iter_nw->nw;
			mn_state = mn_state_f(iter_nw->state);
			npu_err("(TIMEOUT):%s (%s): ss_cnt[%d] nNw[%d] cmd[%d] uid[%u] cmd.length[%zu] payload[%pad] NPU-TIMESTAT:%s\n",
					TYPE_NAME_NW,
					mn_state,
					ss_Cnt,
					nw_Cnt,
					nw->cmd,
					nw->uid,
					nw->ncp_addr.size,
					&nw->ncp_addr.daddr,
					__print_npu_timestamp(&iter_nw->ts, stat_buf, TIME_STAT_BUF_LEN));
			nw_Cnt++;
		}
		ss_Cnt++;
	}
	return 0;
}

/*******************************************************************************
 * Interfacing functions
 */
/* nw_mgmp_ops -> Use functions in npu-if-session-protodrv */
static int nw_mgmt_op_is_available(void)
{
	return npu_ncp_mgmt_is_available();
}

#ifdef T32_GROUP_TRACE_SUPPORT
void t32_trace_ncp_load(void)
{
	npu_info("T32_TRACE: continue execution.\n");
}
#endif

static int nw_mgmt_op_get_request(struct proto_req_nw *target)
{
	if (npu_ncp_mgmt_get(&target->nw) == 0) {
		return 0;	/* Empty */
	} else {
		target->nw.npu_req_id = get_next_npu_req_id();
#ifdef T32_GROUP_TRACE_SUPPORT
		if (target->nw.cmd == NPU_NW_CMD_LOAD) {
			/* Trap NCP load command for trace */
			struct npu_session *sess;

			sess = target->nw.session;
			if (sess && sess->ncp_info.ncp_addr.vaddr) {
				npu_info("T32_TRACE: NCP at d:0x%pK\n", sess->ncp_info.ncp_addr.vaddr);
			} else {
				npu_info("T32_TRACE: Cannot locate NCP.\n");
			}
			t32_trace_ncp_load();
		}
#endif

		return 1;
	}
}

static int nw_mgmt_op_put_result(const struct proto_req_nw *src)
{
	int ret;
	struct nw_result result;

	BUG_ON(!src);

	/* Save result */
	result.result_code = src->nw.result_code;
	result.nw = src->nw;

	/* Invoke callback registered on npu_nw object with result code */
	ret = npu_ncp_mgmt_save_result(src->nw.notify_func, src->nw.session, result);
	if (ret) {
		npu_uerr("error(%d) in npu_ncp_mgmt_save_result\n", &src->nw, ret);
		return ret;
	}

	return 0;
}

/* frame_proc_ops -> Use functions in npu-if-session-protodrv */
static int npu_queue_op_is_available(void)
{
	return npu_buffer_q_is_available();
}

static int npu_queue_op_get_request_pair(struct proto_req_frame *target)
{
	if (npu_buffer_q_get(&target->frame) == 0) {
		return 0;	/* Empty */
	} else {
		target->frame.npu_req_id = get_next_npu_req_id();
		return 1;
	}
}

static int npu_queue_op_put_result(const struct proto_req_frame *src)
{
	const struct npu_frame *result;

	BUG_ON(!src);
	result = &(src->frame);


	/* Copu input's id to output' id */
#ifdef MATCH_ID_INCL_OTCL
	src->output.id = src->input.id;
#endif
#ifdef CONFIG_NPU_GOLDEN_MATCH
	if (result->result_code == NPU_ERR_NO_ERROR) {
		npu_ufdbg("invoke golden match.\n", result);
		npu_golden_compare(result);
	}
#endif
	npu_buffer_q_notify_done(result);

	return 1;
}

static int get_msgid_type(const int msgid)
{
	return msgid_get_pt_type(&npu_proto_drv.msgid_pool, msgid);
}

/* nw_mbox_ops -> Use npu-if-protodrv-mbox object stored in npu_proto_drv */
static int nw_mbox_op_is_available(void)
{
	return npu_nw_mbox_op_is_available();
}
static int nw_mbox_ops_get(struct proto_req_nw **target)
{
	return npu_nw_mbox_ops_get(&npu_proto_drv.msgid_pool, target);
}
static int nw_mbox_ops_put(struct proto_req_nw *src)
{
	return npu_nw_mbox_ops_put(&npu_proto_drv.msgid_pool, src);
}

/* frame_mbox_ops -> Use npu-if-protodrv-mbox object stored in npu_proto_drv */
static int frame_mbox_op_is_available(void)
{
	return npu_frame_mbox_op_is_available();
}
static int frame_mbox_ops_get(struct proto_req_frame **target)
{
	return npu_frame_mbox_ops_get(&npu_proto_drv.msgid_pool, target);
}
static int frame_mbox_ops_put(struct proto_req_frame *src)
{
	return npu_frame_mbox_ops_put(&npu_proto_drv.msgid_pool, src);
}

/**********************************************
*
* Emergency Error Set Function in Proto Drv
*
***********************************************/
static void set_emergency_err_from_req_frame(struct proto_req_frame *req_frame)
{
	struct npu_device *device = npu_proto_drv.npu_device;

	BUG_ON(!device);

	npu_warn("call npu_device_set_emergency_err(device)\n");
	npu_device_set_emergency_err(device);
}

static void set_emergency_err_from_req_nw(struct proto_req_nw *req_nw)
{
	struct npu_device *device = npu_proto_drv.npu_device;

	BUG_ON(!device);

	npu_warn("call npu_device_set_emergency_err(device)\n");
	npu_device_set_emergency_err(device);
}


/*******************************************************************************
 * LSM state transition functions
 */
/*
 * Collect network management request and start processing.
 *
 * FREE -> REQUESTED
 */
static int npu_protodrv_handler_nw_free(void)
{
	int ret = 0;
	int handle_cnt = 0;
	int err_cnt = 0;
	struct proto_req_nw *entry;
	struct session_ref_entry *session_ref_entry;
	s64 now = get_time_ns();

	/* Take a entry from FREE list, before access the queue */
	while ((entry = proto_nw_lsm.lsm_get_entry(FREE)) != NULL) {
		/* Is request available ? */
		if (nw_mgmt_op_get_request(entry) != 0) {
			/* Save the request and put it to REQUESTED state */
			entry->ts.init = now;

			npu_uinfo("(REQ)NW: cmd:%u(%s) / req_id:%u\n",
				&entry->nw, entry->nw.cmd, __cmd_name(entry->nw.cmd),
				entry->nw.npu_req_id);
			if (entry->nw.cmd == NPU_NW_CMD_LOAD) {
				/* Load command -> Register the session */
				ret = register_session_ref(entry->nw.session);
				if (ret) {
					npu_uerr("cannot register session: ret = %d\n",
						&entry->nw, ret);
					entry->nw.result_code = NPU_ERR_DRIVER(NPU_ERR_SESSION_REGISTER_FAILED);
					goto error_req;
				}
			}

			/* Link to the session reference */
			ret = link_session_nw(entry);
			if (ret) {
				npu_uerr("cannot link session: ret = %d\n",
					&entry->nw, ret);
				entry->nw.result_code = NPU_ERR_DRIVER(NPU_ERR_INVALID_UID);
				goto error_req;
			}

			/* Command specific error handling */
			switch (entry->nw.cmd) {
			case NPU_NW_CMD_STREAMOFF:
				/* Update status */
				session_ref_entry = find_session_ref_nw(entry);
				if (session_ref_entry == NULL) {
					npu_uerr("requested STREAM_OFF, but Session entry find error.\n", &entry->nw);
					entry->nw.result_code = NPU_ERR_DRIVER(NPU_ERR_INVALID_UID);
					goto error_req;
				}
				npu_udbg("requested STREAM_OFF : Change s_state [%d] -> [%d]\n",
					&entry->nw, session_ref_entry->s_state, SESSION_REF_STATE_STOPPING);
				switch (session_ref_entry->s_state) {
				case SESSION_REF_STATE_ACTIVE:
					/* No problem */
					break;
				case SESSION_REF_STATE_STOPPING:
					/* Retry of failed stream off */
					npu_uwarn("stream off on SESSION_REF_STATE_STOPPING state. It seems retry of STREAM_OFF.\n",
						&entry->nw);
					break;
				default:
					/* Invalid state */
					npu_uerr("requested STREAM_OFF, but Session state is not Active (%d).\n",
						&entry->nw, session_ref_entry->s_state);
					entry->nw.result_code = NPU_ERR_DRIVER(NPU_ERR_INVALID_STATE);
					goto error_req;
				}
				session_ref_entry->s_state = SESSION_REF_STATE_STOPPING;
				break;

			case NPU_NW_CMD_UNLOAD:
				session_ref_entry = find_session_ref_nw(entry);
				if (session_ref_entry == NULL) {
					npu_uerr("requested UNLOAD, but Session entry find error.\n", &entry->nw);
					entry->nw.result_code = NPU_ERR_DRIVER(NPU_ERR_INVALID_UID);
					goto error_req;
				}
				/* Accepts UNLOAD command only in INACTIVE status */
				if (session_ref_entry->s_state != SESSION_REF_STATE_INACTIVE) {
					npu_uerr("unload command on invalid state (%d).\n"
						, &entry->nw, session_ref_entry->s_state);
					entry->nw.result_code = NPU_ERR_DRIVER(NPU_ERR_INVALID_STATE);
					goto error_req;
				}
				break;

			case NPU_NW_CMD_CLEAR_CB:
				/* Clear all callbacks associated with referred session */
				ret = force_clear_cb(entry);
				if (ret)
					npu_uwarn("force_clear_cb error (%d).\n", &entry->nw, ret);

			default:
				/* No additional checking for other commands */
				break;
			}

			/* Move to REQUESTED state */
			proto_nw_lsm.lsm_put_entry(REQUESTED, entry);
			handle_cnt++;
			goto finally;
error_req:
			/* Error occured -> Move to COMPLETED state */
			proto_nw_lsm.lsm_put_entry(COMPLETED, entry);
			err_cnt++;
			goto finally;
finally:
			;
		} else {
			/* No more request available. rollbacks the entry to FREE */
			proto_nw_lsm.lsm_put_entry(FREE, entry);
			goto break_entry_iter;
		}
	}
break_entry_iter:
	if (entry == NULL) {
		/* No FREE entry ? */
		npu_warn("no free entry for handling %s.\n", TYPE_NAME_NW);
	}
	if (handle_cnt)
		npu_dbg("(%s) free ---> [%d] ---> requested.\n", TYPE_NAME_NW, handle_cnt);
	if (err_cnt)
		npu_dbg("(%s) free ---> [%d] ---> completed.\n", TYPE_NAME_NW, err_cnt);
	return handle_cnt + err_cnt;
}						\

/*
 * Collect frame processing request and start processing.
 *
 * FREE -> REQUESTED
 */
static int npu_protodrv_handler_frame_free(void)
{
	int ret = 0;
	int handle_cnt = 0;
	struct proto_req_frame *entry_lsm;
	s64 now = get_time_ns();

	/* Take a entry from FREE list, before access the queue */
	while ((entry_lsm = proto_frame_lsm.lsm_get_entry(FREE)) != NULL) {
		/* Is request available ? */
		if (npu_queue_op_get_request_pair(entry_lsm) != 0) {
			/* Save the request and put it to REQUESTED state */
			entry_lsm->ts.init = now;

			npu_ufdbg("(REQ)FRAME: cmd:%u / req_id:%u / frame_id:%u\n",
				&entry_lsm->frame, entry_lsm->frame.cmd, entry_lsm->frame.npu_req_id, entry_lsm->frame.frame_id);
			/* Link to the session reference */
			ret = link_session_frame(entry_lsm);
			if (ret) {
				npu_uerr("cannot link session: ret = %d\n",
					&entry_lsm->frame, ret);
				entry_lsm->frame.result_code = NPU_ERR_DRIVER(NPU_ERR_INVALID_UID);
				goto error_req;
			}
			/* Move to REQUESTED state */
			proto_frame_lsm.lsm_put_entry(REQUESTED, entry_lsm);
			goto finally;
error_req:
			/* Error occured -> Move to COMPLETED state */
			proto_frame_lsm.lsm_put_entry(COMPLETED, entry_lsm);
finally:
			handle_cnt++;
		} else {
			/* No more request available. rollbacks the entry to FREE */
			proto_frame_lsm.lsm_put_entry(FREE, entry_lsm);
			goto break_entry_iter;
		}
	}
break_entry_iter:
	if (entry_lsm == NULL) {
		/* No FREE entry ? */
		npu_warn("no free entry for handling %s request.\n", TYPE_NAME_FRAME);
	}

	if (handle_cnt)
		npu_dbg("(%s) free ---> (%d) ---> requested.\n", TYPE_NAME_FRAME, handle_cnt);
	return handle_cnt;
}

/* Error handler helper function */
static int __mbox_frame_ops_put(struct proto_req_frame *entry)
{
	/*
	 * No further request is pumping into hardware
	 * on emergency recovery mode
	 */
	if (npu_device_is_emergency_err(npu_proto_drv.npu_device)) {
		/* Print log message only if the list is not empty */
		npu_ufwarn("EMERGENCY - do not send request to hardware.\n", &entry->frame);
		return 0;
	}

	return frame_mbox_ops_put(entry);
}

static int npu_protodrv_handler_frame_requested(void)
{
	int proc_handle_cnt = 0;
	int compl_handle_cnt = 0;
	int entryCnt = 0;	/* For trace */
	int q_full = 0;		/* Prevent reordering */
	struct proto_req_frame *entry;
	enum protodrv_session_state_e s_state;

	/* Process each element in REQUESTED list one by one */
	LSM_FOR_EACH_ENTRY_IN(proto_frame_lsm, REQUESTED, entry,
		entryCnt++;
		/* Retrive Session state of entry */
		switch (s_state = get_session_ref_state_frame(entry)) {
		case SESSION_REF_STATE_INACTIVE:
			/* Inactive state - Don't move */
			npu_uftrace("INACTIVE\n", &entry->frame);
			break;
		case SESSION_REF_STATE_ACTIVE:
			npu_uftrace("ACTIVE\n", &entry->frame);
			/* Move to Processing state */
			if (q_full)	{
				break;
			}
			if (__mbox_frame_ops_put(entry) > 0) {
				/* Success */
				proto_frame_lsm.lsm_move_entry(PROCESSING, entry);
				proc_handle_cnt++;
			} else {
				npu_ufwarn("REQUESTED %s cannot be queued to mbox [frame_id=%u, npu_req_id=%u]\n",
					   &entry->frame, TYPE_NAME_FRAME, entry->frame.frame_id, entry->frame.npu_req_id);
				q_full = 1;
			}
			break;
		case SESSION_REF_STATE_STOPPING:
			npu_uftrace("STOPPING\n", &entry->frame);
			/* Set error code */
			entry->frame.result_code = NPU_ERR_DRIVER(NPU_ERR_FRAME_CANCELED);
			/* Move to completed state */
			proto_frame_lsm.lsm_move_entry(COMPLETED, entry);
			compl_handle_cnt++;
			break;

		case SESSION_REF_STATE_INVALID:
		default:
			npu_uftrace("INVALID\n", &entry->frame);
			npu_uerr("Invalid Session state [%d]\n", &entry->frame, s_state);
			break;
		}
	) /* End of LSM_FOR_EACH_ENTRY_IN */
	if (proc_handle_cnt)
		npu_dbg("(%s) [%d]REQUESTED ---> [%d] ---> PROCESSING.\n", TYPE_NAME_FRAME, entryCnt, proc_handle_cnt);
	if (compl_handle_cnt)
		npu_dbg("(%s) [%d]REQUESTED ---> [%d] ---> COMPLETED.\n", TYPE_NAME_FRAME, entryCnt, compl_handle_cnt);

	return proc_handle_cnt + compl_handle_cnt;
}

/* Error handler helper function */
static int __mbox_nw_ops_put(struct proto_req_nw *entry)
{
	int ret;

	/*
	 * No further request is pumping into hardware
	 * on emergency recovery mode
	 */
	if (npu_device_is_emergency_err(npu_proto_drv.npu_device)) {
		/* Print log message only if the list is not empty */
		npu_uwarn("EMERGENCY - do not send request [%d:%s] to hardware.\n",
			&entry->nw, entry->nw.cmd, __cmd_name(entry->nw.cmd));
		return 0;
	}

	ret = nw_mbox_ops_put(entry);
	if (ret <= 0)
		npu_uwarn("REQUESTED %s cannot be queued to mbox [npu_req_id=%u]\n",
				&entry->nw, TYPE_NAME_NW, entry->nw.npu_req_id);
	return ret;
}

static int  npu_protodrv_handler_nw_requested(void)
{
	int proc_handle_cnt = 0;
	int compl_handle_cnt = 0;
	int entryCnt = 0;	/* For trace */
	struct proto_req_nw *entry;
	enum protodrv_session_state_e s_state;

	/* Process each element in REQUESTED list one by one */
	LSM_FOR_EACH_ENTRY_IN(proto_nw_lsm, REQUESTED, entry,
		entryCnt++;
		/* Retrive Session state of entry */
		s_state = get_session_ref_state_nw(entry);
		switch (entry->nw.cmd) {
		case NPU_NW_CMD_STREAMOFF:
#ifndef BYPASS_HW_STREAMOFF
			/* Publish message to mailbox */
			if (__mbox_nw_ops_put(entry) > 0) {
				/* Success */
				proto_nw_lsm.lsm_move_entry(PROCESSING, entry);
				proc_handle_cnt++;
			}
#else
			npu_uinfo("BYPASS_HW_STREAMOFF enabled. Bypassing Streamoff.\n", &entry->nw);
			entry->nw.result_code = NPU_ERR_NO_ERROR;
			proto_nw_lsm.lsm_move_entry(COMPLETED, entry);
			compl_handle_cnt++;
#endif	/* BYPASS_HW_STREAMOFF */
			break;
		case NPU_NW_CMD_STREAMON:
			/* Move to completed state */
			proto_nw_lsm.lsm_move_entry(COMPLETED, entry);
			compl_handle_cnt++;
			break;
		case NPU_NW_CMD_UNLOAD:
			/* Publish if INACTIVE */
			if (s_state == SESSION_REF_STATE_INACTIVE) {
				if (__mbox_nw_ops_put(entry) > 0) {
					/* Success */
					proto_nw_lsm.lsm_move_entry(PROCESSING, entry);
					proc_handle_cnt++;
				}
			}
			break;
		case NPU_NW_CMD_POWER_DOWN:
			/* Publish if no session is active */
			if (!is_session_ref_exist()) {
				if (__mbox_nw_ops_put(entry) > 0) {
					/* Success */
					proto_nw_lsm.lsm_move_entry(PROCESSING, entry);
					proc_handle_cnt++;
				}
			}
			break;
		case NPU_NW_CMD_CLEAR_CB:
			/* Move to completed state */
			proto_nw_lsm.lsm_move_entry(COMPLETED, entry);
			compl_handle_cnt++;
			break;
		default:
			npu_utrace("Conventional command cmd:(%u)(%s)\n",
				&entry->nw, entry->nw.cmd, __cmd_name(entry->nw.cmd));
			/* Conventional command -> Publish message to mailbox */
			if (__mbox_nw_ops_put(entry) > 0) {
				/* Success */
				proto_nw_lsm.lsm_move_entry(PROCESSING, entry);
				proc_handle_cnt++;
			}
			break;
		}
	) /* End of LSM_FOR_EACH_ENTRY_IN */
	if (proc_handle_cnt)
		npu_dbg("(%s) [%d]REQUESTED ---> [%d] ---> PROCESSING.\n", TYPE_NAME_NW, entryCnt, proc_handle_cnt);
	if (compl_handle_cnt)
		npu_dbg("(%s) [%d]REQUESTED ---> [%d] ---> COMPLETED.\n", TYPE_NAME_NW, entryCnt, compl_handle_cnt);

	return proc_handle_cnt + compl_handle_cnt;
}

/*
 * Get result of frame processing from NPU hardware
 * and mark it as completed
 *
 * PROCESSING -> COMPLETED
 */
static int  npu_protodrv_handler_frame_processing(void)
{
	int handle_cnt = 0;
	struct proto_req_frame *entry;

	while (frame_mbox_ops_get(&entry) > 0) {
		/* Check entry's state */
		if (entry->state != PROCESSING) {
			npu_warn("out of order response from mbox");
			continue;
		}
		/* Result code already set on frame_mbox_ops_get() -> Just change its state */
		proto_frame_lsm.lsm_move_entry(COMPLETED, entry);
		handle_cnt++;
	}
	if (handle_cnt)
		npu_dbg("(%s) PROCESSING ---> [%d] ---> COMPLETED.\n", TYPE_NAME_FRAME, handle_cnt);
	return handle_cnt;
}

/*
 * Get result of network management from NPU hardware
 * and mark it as completed
 *
 * PROCESSING -> COMPLETED
 */
static int  npu_protodrv_handler_nw_processing(void)
{
	int handle_cnt = 0;
	struct proto_req_nw *entry;

	while (nw_mbox_ops_get(&entry) > 0) {
		/* Check entry's state */
		if (entry->state != PROCESSING) {
			npu_warn("out of order response from mbox");
			continue;
		}

		switch (entry->nw.cmd) {
		case NPU_NW_CMD_STREAMON:
			npu_uerr("invalid command(%u)\n", &entry->nw, entry->nw.cmd);
			BUG_ON(1);

		default:
			/* Result code already set on nw_mbox_ops_get() -> Just change its state */
			proto_nw_lsm.lsm_move_entry(COMPLETED, entry);
			handle_cnt++;
			break;
		}
	}
	if (handle_cnt)
		npu_dbg("(%s) processing ---> (%d) ---> completed.\n", TYPE_NAME_NW, handle_cnt);
	return handle_cnt;
}

static int npu_protodrv_handler_frame_completed(void)
{
	int ret = 0;
	int handle_cnt = 0;
	int entryCnt = 0;	/* For trace */
	struct proto_req_frame *entry;
	char stat_buf[TIME_STAT_BUF_LEN];

	/* Process each element in REQUESTED list one by one */
	LSM_FOR_EACH_ENTRY_IN(proto_frame_lsm, COMPLETED, entry,
		entryCnt++;

		/* Unlink from session ref */
		ret = unlink_session_frame(entry);
		if (ret) {
			npu_uferr("unlink_session_frame failed : %d\n", &entry->frame, ret);
		}

		/* Time keeping */
		npu_ufdbg("(COMPLETED)frame: cmd(%u) / req_id(%d) / result(%u/0x%08x) NPU-TIMESTAT:%s\n",
			&entry->frame, entry->frame.cmd, entry->frame.npu_req_id,
			entry->frame.result_code, entry->frame.result_code,
			__print_npu_timestamp(&entry->ts, stat_buf, TIME_STAT_BUF_LEN));
		if (entry->frame.result_code != NPU_ERR_NO_ERROR)
			fw_will_note(FW_LOGSIZE/4);
		/* Write back result */
		if (npu_queue_op_put_result(entry) > 0) {
			/* Check whether the request need to be stucked or not */
			if (unlikely(is_stucked_req_frame(entry))) {
				proto_frame_lsm.lsm_move_entry(STUCKED, entry);

				// when request frame is stucked,
				// set npu device emergency error
				npu_dbg("call set_emergency_err_from_req_frame()\n");
				set_emergency_err_from_req_frame(entry);

			} else {
				/* If result was written, move back to free state */
				proto_frame_lsm.lsm_move_entry(FREE, entry);
			}
			handle_cnt++;
		} else {
			npu_ufwarn("COMPLETED %s cannot be queued to result [frame_id=%u, npu_req_id=%u]\n",
					&entry->frame, TYPE_NAME_FRAME, entry->frame.frame_id, entry->frame.npu_req_id);
		}
		npu_ufinfo("(COMPLETED)FRAME: Notification sent.\n", &entry->frame);
	) /* End of LSM_FOR_EACH_ENTRY_IN */
	if (handle_cnt)
		npu_dbg("(%s) [%d]COMPLETED ---> [%d] ---> FREE.\n", TYPE_NAME_FRAME, entryCnt, handle_cnt);
	return handle_cnt;
}

static int npu_protodrv_handler_nw_completed(void)
{
	int ret = 0;
	int handle_cnt = 0;
	int entryCnt = 0;	/* For trace */
	struct proto_req_nw *entry;
	struct session_ref_entry *session_ref_entry;
	int transition;
	char stat_buf[TIME_STAT_BUF_LEN];

	/* Process each element in REQUESTED list one by one */
	LSM_FOR_EACH_ENTRY_IN(proto_nw_lsm, COMPLETED, entry,
		entryCnt++;
		transition = 0;

		if (!is_belong_session(&entry->nw)) {
			/* Special treatment because no session is associated with it */
			npu_udbg("complete (%s), with result code (0x%08x)\n",
				&entry->nw, __cmd_name(entry->nw.cmd), entry->nw.result_code);
			if (entry->nw.result_code != NPU_ERR_NO_ERROR)
				fw_will_note(FW_LOGSIZE/4);

			transition = 1;
		} else {
			/* Command other than POWER DOWN -> Associated with session */
			/* Retrive Session state of entry */
			session_ref_entry = find_session_ref_nw(entry);
			if (session_ref_entry == NULL) {
				npu_uerr("Session entry find error.\n", &entry->nw);
				transition = 1;
			} else {
				switch (entry->nw.cmd) {
				case NPU_NW_CMD_LOAD:
					/* Update status */
					if (entry->nw.result_code == NPU_ERR_NO_ERROR) {
						npu_udbg("complete load : Change s_state (%d) -> (%d)\n",
							&entry->nw, session_ref_entry->s_state, SESSION_REF_STATE_INACTIVE);
						session_ref_entry->s_state = SESSION_REF_STATE_INACTIVE;
					} else {
						npu_uerr("complete load, with error(%u/0x%08x) :"
							" Keep current s_state [%d]\n",
							&entry->nw, entry->nw.result_code, entry->nw.result_code,
							session_ref_entry->s_state);
						fw_will_note(FW_LOGSIZE/4);
					}
					transition = 1;
					break;
				case NPU_NW_CMD_STREAMON:
					/* Update status */
					if (entry->nw.result_code == NPU_ERR_NO_ERROR) {
						npu_udbg("complete STREAM_ON : change s_state (%d) -> (%d)\n",
							&entry->nw, session_ref_entry->s_state, SESSION_REF_STATE_ACTIVE);
						session_ref_entry->s_state = SESSION_REF_STATE_ACTIVE;
					} else {
						npu_uerr("complete STREAM_ON, with error(%u/0x%08x) :"
							" keep current s_state (%d)\n",
							&entry->nw, entry->nw.result_code, entry->nw.result_code,
							session_ref_entry->s_state);
						fw_will_note(FW_LOGSIZE/4);
					}
					transition = 1;
					break;
				case NPU_NW_CMD_STREAMOFF:
					/* Claim the packet if the current is last one and result is DONE / or result is NDONE */
					if ((entry->nw.result_code == NPU_ERR_NO_ERROR) && is_last_session_ref(entry)) {
						npu_udbg("complete STREAM_OFF : change s_state (%d) -> (%d)\n",
							&entry->nw, session_ref_entry->s_state, SESSION_REF_STATE_INACTIVE);
						session_ref_entry->s_state = SESSION_REF_STATE_INACTIVE;
						transition = 1;
					} else if (entry->nw.result_code != NPU_ERR_NO_ERROR) {
						npu_uerr("complete STREAM_OFF, with error(%u/0x%08x) :"
							" keep current s_state(%d)\n",
							&entry->nw, entry->nw.result_code, entry->nw.result_code,
							session_ref_entry->s_state);
						transition = 1;
						fw_will_note(FW_LOGSIZE/4);
					} else {
						/* No error but there are outstanding requests */
						npu_udbg("there are pending request(s). STREAM_OFF blocked at s_state [%d]\n",
							&entry->nw, session_ref_entry->s_state);
						transition = 0;
					}
					break;
				case NPU_NW_CMD_UNLOAD:
					/* Claim the packet if the current is last one and result is DONE / or result is NDONE */
					if ((entry->nw.result_code == NPU_ERR_NO_ERROR) && is_last_session_ref(entry)) {
						npu_udbg("complete UNLOAD : change s_state (%d) -> (%d)\n",
							&entry->nw, session_ref_entry->s_state, SESSION_REF_STATE_INVALID);
						session_ref_entry->s_state = SESSION_REF_STATE_INVALID;
						transition = 1;
					} else if (entry->nw.result_code != NPU_ERR_NO_ERROR) {
						npu_uerr("complete UNLOAD, with error(%u/0x%08x) :"
							" Keep current s_state [%d]\n",
							&entry->nw, entry->nw.result_code, entry->nw.result_code,
							session_ref_entry->s_state);
						transition = 1;
						fw_will_note(FW_LOGSIZE/4);
					} else {
						/* No error but there are outstanding requests */
						npu_udbg("there are pending request(s). UNLOAD blocked at s_state (%d)\n",
							&entry->nw, session_ref_entry->s_state);
						transition = 0;
					}
					break;

				case NPU_NW_CMD_CLEAR_CB:
					/* Always success */
					entry->nw.result_code = NPU_ERR_NO_ERROR;
					npu_udbg("complete CLEAR_CB\n", &entry->nw);

					/* Leave assertion if this message is request other than emergency mode */
					if (!npu_device_is_emergency_err(npu_proto_drv.npu_device)) {
						npu_uwarn("NPU_NW_CMD_CLEAR_CB posted on non-emergency situation.\n",
							&entry->nw);
					}
					transition = 1;
					break;

				case NPU_NW_CMD_POWER_DOWN:
				case NPU_NW_CMD_PROFILE_START:
				case NPU_NW_CMD_PROFILE_STOP:
					/* Should be processed on above if clause */
				default:
					npu_uerr("invalid command(%u)\n", &entry->nw, entry->nw.cmd);
					BUG_ON(1);
				}
			}
		}
		/* Post result if processing can be completed */
		if (transition) {
			npu_udbg("(COMPLETED)NW: cmd(%u)(%s) / req_id(%u) / result(%u/0x%08x) NPU-TIMESTAT:%s\n",
				&entry->nw, entry->nw.cmd, __cmd_name(entry->nw.cmd),
				entry->nw.npu_req_id, entry->nw.result_code, entry->nw.result_code,
				__print_npu_timestamp(&entry->ts, stat_buf, TIME_STAT_BUF_LEN));

			if (!nw_mgmt_op_put_result(entry)) {
				npu_uinfo("(COMPLETED)NW: notification sent result(0x%08x)\n",
					&entry->nw, entry->nw.result_code);
			} else {
				/* Shall be retried on next iteration */
				npu_uwarn("COMPLETED %s cannot be queued to result npu_req_id(%u)\n",
					   &entry->nw, TYPE_NAME_NW, entry->nw.npu_req_id);
				goto retry_on_next;
			}

			ret = unlink_session_nw(entry);
			if (ret) {
				npu_uerr("unlink_session_nw for CMD[%u] failed : %d\n",
					&entry->nw, entry->nw.cmd, ret);
			}

			if (entry->nw.cmd == NPU_NW_CMD_UNLOAD) {
				/* Failed UNLOAD - left warning but session ref shall be destroyed anyway */
				if (unlikely(entry->nw.result_code != NPU_ERR_NO_ERROR))
					npu_uwarn("Unload failed but Session ref will be destroyed.\n", &entry->nw);

				/* Destroy session ref */
				ret = drop_session_ref(entry->nw.uid);
				if (ret) {
					npu_uerr("drop_session_ref error : %d", &entry->nw, ret);
				}
			}

			/* Check whether the request need to be stucked or not */
			if (unlikely(is_stucked_req_nw(entry))) {
				proto_nw_lsm.lsm_move_entry(STUCKED, entry);

				// when nw request is stucked,
				// set npu_device emergency_error
				npu_dbg("call set_emergency_err_from_req_nw(entry)\n");
				set_emergency_err_from_req_nw(entry);
			} else {
				proto_nw_lsm.lsm_move_entry(FREE, entry);
			}
			handle_cnt++;
retry_on_next:
			;
		}
	) /* End of LSM_FOR_EACH_ENTRY_IN */
	if (handle_cnt)
		npu_dbg("(%s) [%d]COMPLETED ---> [%d] ---> FREE.\n", TYPE_NAME_NW, entryCnt, handle_cnt);
	return handle_cnt;
}

/*
 * Map for timeout handling.
 * Zero means no timeout
 */
#define S2N	1000000000	/* Seconds to nano seconds */
static struct {
	u64		timeout_ns;
	npu_errno_t	err_code;	/* Error code on timeout. Refer npu-error.h */
#ifdef CONFIG_NPU_ZEBU_EMULATION
} NPU_PROTODRV_TIMEOUT_MAP[LSM_LIST_TYPE_INVALID][PROTO_DRV_REQ_TYPE_INVALID] = {
			/*Dummy*/		/* Frame request */							/* NCP mgmt. request */
/* FREE - (NA)      */	{{0, 0}, {.timeout_ns = 0,          .err_code = 0                    }, {.timeout_ns = 0,           .err_code = 0} },
/* REQUESTED        */	{{0, 0}, {.timeout_ns = 0,          .err_code = NPU_CRITICAL_DRIVER(NPU_ERR_SCHED_TIMEOUT)}, {.timeout_ns = 0,           .err_code = 0} },
/* PROCESSING       */	{{0, 0}, {.timeout_ns = 0,          .err_code = NPU_CRITICAL_DRIVER(NPU_ERR_NPU_TIMEOUT)  }, {.timeout_ns = 0,           .err_code = 0} },
/* COMPLETED - (NA) */	{{0, 0}, {.timeout_ns = 0,          .err_code = 0                    }, {.timeout_ns = 0,           .err_code = 0} },
};
#else
} NPU_PROTODRV_TIMEOUT_MAP[LSM_LIST_TYPE_INVALID][PROTO_DRV_REQ_TYPE_INVALID] = {
			/*Dummy*/		/* Frame request */							/* NCP mgmt. request */
/* FREE - (NA)      */	{{0, 0}, {.timeout_ns = 0,          .err_code = 0                    }, {.timeout_ns = 0,           .err_code = 0} },
/* REQUESTED        */	{{0, 0}, {.timeout_ns = (5L * S2N), .err_code = NPU_CRITICAL_DRIVER(NPU_ERR_SCHED_TIMEOUT)}, {.timeout_ns = (5L * S2N),  .err_code = NPU_CRITICAL_DRIVER(NPU_ERR_SCHED_TIMEOUT)} },
/* PROCESSING       */	{{0, 0}, {.timeout_ns = (10L * S2N), .err_code = NPU_CRITICAL_DRIVER(NPU_ERR_NPU_TIMEOUT)  }, {.timeout_ns = (S2N / 2), .err_code = NPU_CRITICAL_DRIVER(NPU_ERR_QUEUE_TIMEOUT)} },
/* COMPLETED - (NA) */	{{0, 0}, {.timeout_ns = 0,          .err_code = 0                    }, {.timeout_ns = 0,           .err_code = 0} },
};
#endif

static int proto_drv_timedout_handling(const lsm_list_type_e state)
{
	s64 now = get_time_ns();
	int timeout_entry_cnt = 0;
	{
		struct proto_req_frame *entry;

		LSM_FOR_EACH_ENTRY_IN(proto_frame_lsm, state, entry,
			if (is_timedout(entry, NPU_PROTODRV_TIMEOUT_MAP[state][PROTO_DRV_REQ_TYPE_FRAME].timeout_ns, now)) {
				timeout_entry_cnt++;
				/* Timeout */
				entry->frame.result_code = NPU_PROTODRV_TIMEOUT_MAP[state][PROTO_DRV_REQ_TYPE_FRAME].err_code;
				proto_frame_lsm.lsm_move_entry(COMPLETED, entry);
				npu_uwarn("timeout entry (%s) on state %s - req_id(%u), result_code(%u/0x%08x)\n",
					&entry->frame,
					getTypeName(PROTO_DRV_REQ_TYPE_FRAME), LSM_STATE_NAMES[state],
					entry->frame.npu_req_id, entry->frame.result_code, entry->frame.result_code);
			}
		)
	}
	{
		struct proto_req_nw *entry;

		LSM_FOR_EACH_ENTRY_IN(proto_nw_lsm, state, entry,
			if (is_timedout(entry, NPU_PROTODRV_TIMEOUT_MAP[state][PROTO_DRV_REQ_TYPE_NW].timeout_ns, now)) {
				timeout_entry_cnt++;
				/* Timeout */
				entry->nw.result_code = NPU_PROTODRV_TIMEOUT_MAP[state][PROTO_DRV_REQ_TYPE_NW].err_code;
				proto_nw_lsm.lsm_move_entry(COMPLETED, entry);
				npu_uwarn("timeout entry (%s) on state %s - req_id(%u), result_code(%u/0x%08x)\n",
					&entry->nw,
					getTypeName(PROTO_DRV_REQ_TYPE_NW), LSM_STATE_NAMES[state],
					entry->nw.npu_req_id, entry->nw.result_code, entry->nw.result_code);
			}
		)
	}
	return timeout_entry_cnt;
}

/* Hook function which is called on every LSM transition */
static void proto_drv_put_hook_frame(lsm_list_type_e state, struct proto_req_frame *entry)
{
	lsm_list_type_e old_state;
	struct npu_frame *frame;
	BUG_ON(!entry);
	old_state = entry->state;
	entry->state = state;

	/* Update timestamp on state transition */
	if (old_state != state) {
		__update_npu_timestamp(old_state, state, &entry->ts);

		/* Save profiling point */
		frame = &entry->frame;
		switch (state) {
		case REQUESTED:
			profile_point3(PROBE_ID_DD_FRAME_SCHEDULED, frame->uid, frame->frame_id, 0, frame->npu_req_id, 0);
			break;
		case PROCESSING:
			profile_point3(PROBE_ID_DD_FRAME_PROCESS, frame->uid, frame->frame_id, 0, frame->npu_req_id, 0);
			break;
		case COMPLETED:
			profile_point3(PROBE_ID_DD_FRAME_COMPLETED, frame->uid, frame->frame_id, 0, frame->npu_req_id, 0);
			break;
		case FREE:
			profile_point3(PROBE_ID_DD_FRAME_DONE, frame->uid, frame->frame_id, 0, frame->npu_req_id, 0);
			break;
		default:
			break;
		}
	}

	if (old_state != FREE && state == FREE) {
		/* Transition from other state to free - clean-up its contents */
		if (is_list_used(&entry->sess_ref_list)) {
			npu_warn("session ref list is not empty but claimed to FREE.\n");
		}
		INIT_LIST_HEAD(&entry->sess_ref_list);
		/* ts and frame are intentionally left for debugging */
	}

}

static void proto_drv_put_hook_nw(lsm_list_type_e state, struct proto_req_nw *entry)
{
	lsm_list_type_e old_state;
	struct npu_nw *nw;
	BUG_ON(!entry);
	old_state = entry->state;
	entry->state = state;

	/* Update timestamp on state transition */
	if (old_state != state) {
		__update_npu_timestamp(old_state, state, &entry->ts);

		/* Save profiling point */
		nw = &entry->nw;
		switch (state) {
		case REQUESTED:
			profile_point3(PROBE_ID_DD_NW_SCHEDULED, nw->uid, 0, nw->cmd, nw->npu_req_id, 0);
			break;
		case PROCESSING:
			profile_point3(PROBE_ID_DD_NW_PROCESS, nw->uid, 0, nw->cmd, nw->npu_req_id, 0);
			break;
		case COMPLETED:
			profile_point3(PROBE_ID_DD_NW_COMPLETED, nw->uid, 0, nw->cmd, nw->npu_req_id, 0);
			break;
		case FREE:
			profile_point3(PROBE_ID_DD_NW_DONE, nw->uid, 0, nw->cmd, nw->npu_req_id, 0);
			break;
		default:
			break;
		}
	}

	if (old_state != FREE && state == FREE) {
		/* Transition from other state to free - clean-up its contents */
		if (is_list_used(&entry->sess_ref_list)) {
			npu_warn("session ref list is not empty but claimed to FREE");
		}
		INIT_LIST_HEAD(&entry->sess_ref_list);
		/* ts and frame are intentionally left for debugging */
	}
}

/* Main thread function performed by AST */
static int proto_drv_do_task(struct auto_sleep_thread_param *data)
{
	int ret = 0;

	if (!EXPECT_STATE(PROTO_DRV_STATE_OPENED)) {
		return 0;
	}

	ret += npu_protodrv_handler_frame_processing();
	ret += npu_protodrv_handler_nw_processing();

	ret += npu_protodrv_handler_frame_completed();
	ret += npu_protodrv_handler_nw_completed();

	ret += npu_protodrv_handler_frame_free();
	ret += npu_protodrv_handler_nw_free();

	ret += npu_protodrv_handler_frame_requested();
	ret += npu_protodrv_handler_nw_requested();

	/* Timeout handling */
	ret += proto_drv_timedout_handling(REQUESTED);
	ret += proto_drv_timedout_handling(PROCESSING);

	npu_trace("return value = %d\n", ret);
	return ret;
}

static int proto_drv_check_work(struct auto_sleep_thread_param *data)
{
	if (!EXPECT_STATE(PROTO_DRV_STATE_OPENED))
		return 0;

	return (nw_mgmt_op_is_available() > 0)
	       || (npu_queue_op_is_available() > 0)
	       || (nw_mbox_op_is_available() > 0)
	       || (frame_mbox_op_is_available() > 0);
}

static void proto_drv_on_idle(struct auto_sleep_thread_param *data, s64 idle_duration_ns)
{
	if (idle_duration_ns < NPU_PROTO_DRV_IDLE_LOG_DELAY_NS)
		return;

	/* Idle is longer than NPU_PROTO_DRV_IDLE_LOG_DELAY_NS */
	npu_warn("NPU driver is idle for [%lld ns].\n", idle_duration_ns);

	/* Print out session info */
	log_session_ref();
}

static void proto_drv_ast_signal_from_session(void)
{
	auto_sleep_thread_signal(&npu_proto_drv.ast);
}

static void proto_drv_ast_signal_from_mailbox(void *arg)
{
	auto_sleep_thread_signal(&npu_proto_drv.ast);
}

struct lsm_state_stat {
	u32 entry_cnt[LSM_LIST_TYPE_INVALID];
	s64 max_curr[LSM_LIST_TYPE_INVALID];
	s64 max_init[LSM_LIST_TYPE_INVALID];
	s64 avg_curr[LSM_LIST_TYPE_INVALID];
	s64 avg_init[LSM_LIST_TYPE_INVALID];
	void *max_curr_entry[LSM_LIST_TYPE_INVALID];
	void *max_init_entry[LSM_LIST_TYPE_INVALID];
};

/* Structure to keep the call back function to retrive
 * information from two LSM defined in protodrv
 */
struct lsm_getinfo_ops {
	size_t (*get_entry_num)(void);
	lsm_list_type_e (*get_qtype)(int idx);
	struct npu_timestamp* (*get_timestamp)(int idx);
	void* (*get_entry_ptr)(int idx);
};

#define DEF_LSM_GET_INFO_OPS(LSM_NAME, OPS_NAME)		\
size_t LSM_NAME##_get_entry_num(void)				\
{								\
	return LSM_NAME.lsm_data.entry_num;			\
}								\
lsm_list_type_e LSM_NAME##_get_qtype(int idx)			\
{								\
	if (idx >= 0 && idx < LSM_NAME.lsm_data.entry_num)	\
		return LSM_NAME.entry_array[idx].data.state;	\
	else							\
		return LSM_LIST_TYPE_INVALID;			\
}								\
struct npu_timestamp *LSM_NAME##_get_timestamp(int idx)		\
{								\
	if (idx >= 0 && idx < LSM_NAME.lsm_data.entry_num)	\
		return &LSM_NAME.entry_array[idx].data.ts;	\
	else							\
		return NULL;					\
}								\
void *LSM_NAME##_get_entry_ptr(int idx)				\
{								\
	if (idx >= 0 && idx < LSM_NAME.lsm_data.entry_num)	\
		return (void *)&LSM_NAME.entry_array[idx].data;	\
	else							\
		return NULL;					\
}								\
static struct lsm_getinfo_ops OPS_NAME = {			\
	.get_entry_num = LSM_NAME##_get_entry_num,		\
	.get_qtype = LSM_NAME##_get_qtype,			\
	.get_timestamp = LSM_NAME##_get_timestamp,		\
	.get_entry_ptr = LSM_NAME##_get_entry_ptr,		\
};								\

/* OPS definition for Frame LSM */
DEF_LSM_GET_INFO_OPS(proto_frame_lsm, proto_frame_lsm_getinfo_ops)
/* OPS definition for Network management LSM */
DEF_LSM_GET_INFO_OPS(proto_nw_lsm, proto_nw_lsm_getinfo_ops)

/* Collect time statistics with supplied lsm_getinfo_ops */
static int retrive_lsm_stat(struct lsm_state_stat *stat, const struct lsm_getinfo_ops *getinfo_ops)
{
	struct npu_timestamp *ts;
	struct npu_timestamp_entry *ts_entry;
	int ts_entry_idx;
	size_t all_entry_num = getinfo_ops->get_entry_num();
	size_t i;
	s64 curr_diff, init_diff;
	lsm_list_type_e qtype;
	s64 now_ns;

	/* Get current time */
	now_ns = get_time_ns();

	BUG_ON(!stat);
	memset(stat, 0, sizeof(*stat));

	/* Traverse each list */
	for (i = 0; i < all_entry_num; ++i) {
		qtype = getinfo_ops->get_qtype(i);
		if (qtype < 0 || qtype >= LSM_LIST_TYPE_INVALID) {
			npu_err("invalid state [%d] on entry_array[%lu]\n",
				qtype, i);
			continue;
		}

		ts = getinfo_ops->get_timestamp(i);
		if (!ts) {
			npu_err("no timestamp info on entry_array[%lu]\n", i);
			continue;
		}

		ts_entry_idx = atomic_read(&ts->curr_entry);
		if (ts_entry_idx < 0 || ts_entry_idx >= LSM_ELEM_STATUS_TRACK_LEN) {
			npu_err("invalid entry_idx[%d] on timestamp on entry_array[%lu]\n"
				, ts_entry_idx, i);
			continue;
		}
		ts_entry = ts->hist + ts_entry_idx;

		// Get elapsed time
		curr_diff = now_ns - ts_entry->enter;
		init_diff = now_ns - ts->init;

		// Update statistics
		stat->entry_cnt[qtype]++;
		if (stat->max_curr[qtype] < curr_diff) {
			stat->max_curr[qtype] = curr_diff;
			stat->max_curr_entry[qtype] = getinfo_ops->get_entry_ptr(i);
		}
		if (stat->max_init[qtype] < init_diff) {
			stat->max_init[qtype] = init_diff;
			stat->max_init_entry[qtype] = getinfo_ops->get_entry_ptr(i);
		}

		stat->avg_curr[qtype] += ((curr_diff - stat->avg_curr[qtype])
					/ stat->entry_cnt[qtype]);
		stat->avg_init[qtype] += ((init_diff - stat->avg_init[qtype])
					/ stat->entry_cnt[qtype]);
	}
	return 0;
}

/* Stringfy information stored in the struct lsm_state_stat */
static size_t stat_to_string(const struct lsm_state_stat *stat, char *outbuf, const size_t len)
{
	size_t remain_len = len;
	int i, ret;
	struct timespec ts_avg_curr, ts_avg_init, ts_max_curr, ts_max_init;

	BUG_ON(!stat);
	BUG_ON(!outbuf);

	/* Printout for each state */
	for (i = 0; i < LSM_LIST_TYPE_INVALID; ++i) {
		ret = scnprintf(outbuf, remain_len,
				"State [%s] - Entry count [%u]\n",
				LSM_STATE_NAMES[i], stat->entry_cnt[i]);
		if (ret > 0) {
			remain_len -= ret;
			outbuf += ret;
		} else {
			return 0;
		}
		/* Skip if the state is FREE of no entry exist on this state */
		if (i == FREE || stat->entry_cnt[i] == 0) {
			continue;
		}

		/* Print more detailed information */
		ts_avg_curr = ns_to_timespec(stat->avg_curr[i]);
		ts_avg_init = ns_to_timespec(stat->avg_init[i]);
		ts_max_curr = ns_to_timespec(stat->max_curr[i]);
		ts_max_init = ns_to_timespec(stat->max_init[i]);
		ret = scnprintf(outbuf, remain_len,
				"Average curr age %ld.%09ldns, init age %ld.%09ldns\n"
				" Max curr age %ld.%09ldns - entry at [%pK]\n"
				" Max init age %ld.%09ldns - entry at [%pK]\n",
				ts_avg_curr.tv_sec, ts_avg_curr.tv_nsec,
				ts_avg_init.tv_sec, ts_avg_init.tv_nsec,
				ts_max_curr.tv_sec, ts_max_curr.tv_nsec, stat->max_curr_entry[i],
				ts_max_init.tv_sec, ts_max_init.tv_nsec, stat->max_init_entry[i]);
		if (ret > 0) {
			remain_len -= ret;
			outbuf += ret;
		} else {
			return 0;
		}
	}
	return len - remain_len;
}

/*******************************************************************************
 * Debugging functions (via debugfs)
 */
static ssize_t proto_drv_dump_status(char *outbuf, const size_t len)
{
	int ret;
	struct lsm_state_stat frame_stat;
	struct lsm_state_stat nw_stat;
	size_t remain_len = len;	/* Remaining size of output */
	proto_drv_state_e current_state = get_state();

	/* Print global state */
	ret = scnprintf(outbuf, remain_len, "Protodrv state [%s]\n",
			PROTO_DRV_STATE_NAME[current_state]);
	if (ret > 0) {
		remain_len -= ret;
		outbuf += ret;
	} else {
		npu_err("dump error in output writing\n");
		return -EFAULT;
	}
	if (current_state != PROTO_DRV_STATE_OPENED) {
		/* Protodrv is not operational. Just print current state */
		goto ok_exit;
	}

	/* Collect statistics */
	if (retrive_lsm_stat(&frame_stat, &proto_frame_lsm_getinfo_ops) != 0) {
		npu_err("fail in retrive_lsm_stat(FRAME)");
		return -EFAULT;
	}
	if (retrive_lsm_stat(&nw_stat, &proto_nw_lsm_getinfo_ops) != 0) {
		npu_err("fail retrive_lsm_stat(NW).");
		return -EFAULT;
	}

	/* Print stat for Frame LSM */
	ret = scnprintf(outbuf, remain_len,
			"--- Printing out LSM [%s]\n",
			proto_frame_lsm.lsm_data.name);
	if (ret > 0) {
		remain_len -= ret;
		outbuf += ret;
	} else {
		npu_err("dump error in output writing");
		return -EFAULT;
	}
	ret = stat_to_string(&frame_stat, outbuf, remain_len);
	if (ret > 0) {
		remain_len -= ret;
		outbuf += ret;
	} else {
		npu_err("dump error in output writing");
		return -EFAULT;
	}
	ret = scnprintf(outbuf, remain_len,
			"--- End of Printing out LSM [%s]\n",
			proto_frame_lsm.lsm_data.name);
	if (ret > 0) {
		remain_len -= ret;
		outbuf += ret;
	} else {
		npu_err("dump error in output writing\n");
		return -EFAULT;
	}

	/* Print stat for Network Mgmt. LSM */
	ret = scnprintf(outbuf, remain_len,
			"--- Printing out LSM [%s]\n",
			proto_nw_lsm.lsm_data.name);
	if (ret > 0) {
		remain_len -= ret;
		outbuf += ret;
	} else {
		npu_err("dump error in output writing");
		return -EFAULT;
	}
	ret = stat_to_string(&nw_stat, outbuf, remain_len);
	if (ret > 0) {
		remain_len -= ret;
		outbuf += ret;
	} else {
		npu_err("dump error in output writing");
		return -EFAULT;
	}
	ret = scnprintf(outbuf, remain_len,
			"--- End of Printing out LSM [%s]\n",
			proto_nw_lsm.lsm_data.name);
	if (ret > 0) {
		remain_len -= ret;
		outbuf += ret;
	} else {
		npu_err("dump error in output writing\n");
		return -EFAULT;
	}

ok_exit:
	return len - remain_len;
}

/* Functions for debugfs */
struct proto_drv_dump_debug_data {
	char *buf;
	size_t len;
	size_t cur;
};

#define DUMP_BUF_SIZE	4096
/*
 * Open :
 *  Allocate a buffer to store dumped result,
 *  and populate the buffer with dump result.
 */
int proto_drv_dump_debug_open(struct inode *inode, struct file *file)
{
	struct proto_drv_dump_debug_data *dump_data = NULL;
	int ret;
	ssize_t dump_len;

	npu_info("starti in proto_drv_dump_debug_open\n");
	BUG_ON(!file);

	/* Memory allocation */
	dump_data = kzalloc(sizeof(*dump_data), GFP_KERNEL);
	if (!dump_data) {
		npu_err("fail in dump_debug_data allocation\n");
		ret = -ENOMEM;
		goto err_exit;
	}
	dump_data->buf = NULL;
	dump_data->buf = kzalloc(DUMP_BUF_SIZE, GFP_KERNEL);
	if (!dump_data->buf) {
		npu_err("fail in  output buffer allocation\n");
		ret = -ENOMEM;
		goto err_exit;
	}

	/* Store result on buffer */
	dump_data->cur = 0;
	dump_len = proto_drv_dump_status(dump_data->buf, DUMP_BUF_SIZE);
	if (dump_len < 0) {
		npu_err("error(%zd) in dump status\n", dump_len);
		ret = -EFAULT;
		goto err_exit;
	}
	dump_data->len = (size_t)dump_len;

	/* Store the buffer as private data */
	file->private_data = (void *)dump_data;

	npu_info("complete in proto_drv_dump_debug_open, [%lu] bytes output.\n"
		 , dump_data->len);
	return 0;

err_exit:
	if (dump_data != NULL) {
		if (dump_data->buf != NULL)	{
			kfree(dump_data->buf);
		}
		kfree(dump_data);
	}
	return ret;
}

/*
 * Close : Deallocate the buffer made on open
 */
int proto_drv_dump_debug_close(struct inode *inode, struct file *file)
{
	struct proto_drv_dump_debug_data *dump_data;

	npu_info("start in proto_drv_dump_debug_close\n");
	BUG_ON(!file);
	dump_data = (struct proto_drv_dump_debug_data *)file->private_data;
	BUG_ON(!dump_data);

	if (dump_data->buf != NULL)	{
		kfree(dump_data->buf);
	}
	kfree(dump_data);
	file->private_data = NULL;

	npu_info("complete in proto_drv_dump_debug_close\n");
	return 0;
}

/*
 * Read : Returns contents in dump_data,
 *        which was generated and kept in open() function
 */
ssize_t proto_drv_dump_debug_read(struct file *file, char __user *outbuf, size_t outlen, loff_t *loff)
{
	struct proto_drv_dump_debug_data *dump_data;
	ssize_t copy_len;
	unsigned long ul_ret;

	npu_dbg("start in proto_drv_dump_debug_read\n");
	BUG_ON(!file);
	dump_data = (struct proto_drv_dump_debug_data *)file->private_data;
	BUG_ON(!dump_data);

	/* No more data */
	if (dump_data->len <= dump_data->cur) {
		npu_dbg("complete in eead for dump debug\n");
		return 0;
	}

	copy_len = min(outlen, (dump_data->len - dump_data->cur));
	ul_ret = copy_to_user(outbuf, &(dump_data->buf[dump_data->cur]), copy_len);
	if (ul_ret != 0) {
		npu_err("error on copy_to_user: [%lu]\n", ul_ret);
		return -EFAULT;
	}
	dump_data->cur += copy_len;
	npu_dbg("complete in proto_drv_dump_debug_read, [%lu] bytes are read.\n"
		, copy_len);
	return copy_len;
}

/* file operation structure which will be passed to npu-debug */
static const struct file_operations npu_proto_drv_dump_debug_fops = {
	.owner = THIS_MODULE,
	.open = proto_drv_dump_debug_open,
	.release = proto_drv_dump_debug_close,
	.read = proto_drv_dump_debug_read,
};

/*******************************************************************************
 * Object lifecycle functions
 *
 * Those functions are exported and invoked by npu-device
 */
int proto_drv_probe(struct npu_device *npu_device)
{
	int ret = 0;

	probe_info("start in proto_drv_probe\n");
	if (!IS_TRANSITABLE(PROTO_DRV_STATE_PROBED))	{
		return -EBADR;
	}

	/* Registre dump function on debugfs */
	ret = npu_debug_register("proto-drv-dump", &npu_proto_drv_dump_debug_fops);
	if (ret)
		probe_err("fail(%d) in npu_debug_register\n", ret);

	/* Pass reference of npu_proto_drv via npu_device */
	npu_device->proto_drv = &npu_proto_drv;
	npu_proto_drv.npu_device = npu_device;

#ifdef T32_GROUP_TRACE_SUPPORT
	probe_info("T32_TRACE: to do trace, use following T32 command.\n Break.Set 0x%pK\n",
		   t32_trace_ncp_load);
#endif

	state_transition(PROTO_DRV_STATE_PROBED);
	probe_info("complete in proto_drv_probe\n");
	return ret;
}

int proto_drv_release(void)
{
	probe_info("start in proto_drv_release\n");
	if (!IS_TRANSITABLE(PROTO_DRV_STATE_UNLOADED))	{
		return -EBADR;
	}

	npu_proto_drv.npu_device = NULL;

#ifdef MBOX_MOCK_ENABLE
	npu_dbg("deinit mbox_mock()");
	deinit_for_mbox_mock();
#endif

	state_transition(PROTO_DRV_STATE_UNLOADED);
	probe_info("complete in proto_drv_release\n");
	return 0;
}

int proto_drv_open(struct npu_device *npu_device)
{
	int ret = 0;

	npu_info("start in Open Protodrv : Starting.\n");
	if (!IS_TRANSITABLE(PROTO_DRV_STATE_OPENED))	{
		return -EBADR;
	}
	npu_proto_drv.open_steps = 0;

#ifdef MBOX_MOCK_ENABLE
	npu_dbg("start mbox_mock()");
	setup_for_mbox_mock();
	set_bit(PROTO_DRV_OPEN_SETUP_MBOX_MOCK, &npu_proto_drv.open_steps);
#endif

	/* Initialize session ref */
	reset_session_ref();
	set_bit(PROTO_DRV_OPEN_SESSION_REF, &npu_proto_drv.open_steps);

	/* Initialize interface with session */
	npu_proto_drv.if_session_ctx = npu_if_session_protodrv_ctx_open();
	if (!npu_proto_drv.if_session_ctx) {
		npu_err("npu_if_session_protodrv init failed.\n");
		ret = -EFAULT;
		goto err_exit;
	}
	set_bit(PROTO_DRV_OPEN_IF_SESSION_CTX, &npu_proto_drv.open_steps);

	/* Register signaling callback - Session side */
	npu_buffer_q_register_cb(proto_drv_ast_signal_from_session);
	npu_ncp_mgmt_register_cb(proto_drv_ast_signal_from_session);
	set_bit(PROTO_DRV_OPEN_REGISTER_CB, &npu_proto_drv.open_steps);

	/* Register signaling callback and msgid type lookup function
	 * will be called from Mailbox handler */
	npu_mbox_op_register_notifier(proto_drv_ast_signal_from_mailbox);
	npu_mbox_op_register_msgid_type_getter(get_msgid_type);
	set_bit(PROTO_DRV_OPEN_REGISTER_NOTIFIER, &npu_proto_drv.open_steps);
#if 0
	/* Initialize interface with mailbox manager */
	npu_proto_drv.if_mbox_ctx = npu_if_protodrv_mbox_ctx_open();
	if (!npu_proto_drv.if_mbox_ctx) {
		npu_err("npu_if_protodrv_mbox init failed.\n");
		ret = -EFAULT;
		goto err_exit;
	}
	LLQ_REGISTER_TASK(npu_proto_drv.if_mbox_ctx->mbox_nw_resp,
			  proto_drv_ast_signal);
	LLQ_REGISTER_TASK(npu_proto_drv.if_mbox_ctx->mbox_frame_resp,
			  proto_drv_ast_signal);
#endif
	/* MSGID pool initialization */
	msgid_pool_init(&npu_proto_drv.msgid_pool);
	set_bit(PROTO_DRV_OPEN_MSGID_POOL, &npu_proto_drv.open_steps);

	/* LSM initialization (without AST) */
	ret = proto_frame_lsm.lsm_init(NULL, NULL, NULL);
	if (ret) {
		npu_err("init fail(%d) in LSM(frame)\n", ret);
		goto err_exit;
	}
	proto_frame_lsm.lsm_set_hook(proto_drv_put_hook_frame);
	NPU_PROTODRV_LSM_ENTRY_INIT(proto_frame_lsm, struct proto_req_frame);
	set_bit(PROTO_DRV_OPEN_FRAME_LSM, &npu_proto_drv.open_steps);

	ret = proto_nw_lsm.lsm_init(NULL, NULL, NULL);
	if (ret) {
		npu_err("init fail(%d) in LSM(nw)\n", ret);
		goto err_exit;
	}
	proto_nw_lsm.lsm_set_hook(proto_drv_put_hook_nw);
	NPU_PROTODRV_LSM_ENTRY_INIT(proto_nw_lsm, struct proto_req_nw);
	set_bit(PROTO_DRV_OPEN_NW_LSM, &npu_proto_drv.open_steps);

	/* AST initialization */
	ret = auto_sleep_thread_create(&npu_proto_drv.ast,
		NPU_PROTO_DRV_AST_NAME,
		proto_drv_do_task, proto_drv_check_work, proto_drv_on_idle);
	if (ret) {
		npu_err("fail(%d) in AST create\n", ret);
		goto err_exit;
	}
	set_bit(PROTO_DRV_OPEN_AST_CREATE, &npu_proto_drv.open_steps);

	state_transition(PROTO_DRV_STATE_OPENED);
	ret = auto_sleep_thread_start(&npu_proto_drv.ast, npu_proto_drv.ast_param);
	if (ret) {
		npu_err("fail(%d) in AST start\n", ret);
		goto err_exit;
	}
	set_bit(PROTO_DRV_OPEN_AST_START, &npu_proto_drv.open_steps);

	npu_info("complete in proto_drv_open\n");
	set_bit(PROTO_DRV_OPEN_COMPLETE, &npu_proto_drv.open_steps);
	return 0;

err_exit:
	return ret;
}

int proto_drv_close(struct npu_device *npu_device)
{
	npu_info("start in proto_drv_close\n");
	if (!IS_TRANSITABLE(PROTO_DRV_STATE_PROBED))	{
		return -EBADR;
	}
	BIT_CHECK_AND_EXECUTE(PROTO_DRV_OPEN_COMPLETE, &npu_proto_drv.open_steps, NULL, ;);

	BIT_CHECK_AND_EXECUTE(PROTO_DRV_OPEN_AST_START, &npu_proto_drv.open_steps, "Stopping AST", {
		auto_sleep_thread_terminate(&npu_proto_drv.ast);
	});

	BIT_CHECK_AND_EXECUTE(PROTO_DRV_OPEN_AST_CREATE, &npu_proto_drv.open_steps, NULL, ;);

	BIT_CHECK_AND_EXECUTE(PROTO_DRV_OPEN_NW_LSM, &npu_proto_drv.open_steps, "Close NW LSM", {
		proto_nw_lsm.lsm_destroy();
	});
	BIT_CHECK_AND_EXECUTE(PROTO_DRV_OPEN_FRAME_LSM, &npu_proto_drv.open_steps, "Close FRAME LSM", {
		proto_frame_lsm.lsm_destroy();
	});

#if 0
	npu_info("Close Protodrv : Closing npu_if_protodrv_mbox.\n");
	npu_if_protodrv_mbox_ctx_close();
	npu_proto_drv.if_mbox_ctx = NULL;
#endif
	BIT_CHECK_AND_EXECUTE(PROTO_DRV_OPEN_MSGID_POOL, &npu_proto_drv.open_steps, NULL, ;);
	BIT_CHECK_AND_EXECUTE(PROTO_DRV_OPEN_REGISTER_NOTIFIER, &npu_proto_drv.open_steps, NULL, ;);
	BIT_CHECK_AND_EXECUTE(PROTO_DRV_OPEN_REGISTER_CB, &npu_proto_drv.open_steps, NULL, ;);

	BIT_CHECK_AND_EXECUTE(PROTO_DRV_OPEN_IF_SESSION_CTX,
		&npu_proto_drv.open_steps,
		"Close npu_if_session_protodrv", {
		npu_if_session_protodrv_ctx_close();
		npu_proto_drv.if_session_ctx = NULL;
	});

	BIT_CHECK_AND_EXECUTE(PROTO_DRV_OPEN_SESSION_REF, &npu_proto_drv.open_steps, NULL, ;);

#ifdef MBOX_MOCK_ENABLE
	BIT_CHECK_AND_EXECUTE(PROTO_DRV_OPEN_SETUP_MBOX_MOCK, &npu_proto_drv.open_steps, NULL, ;);
#endif

	if (npu_proto_drv.open_steps != 0)
		npu_warn("Missing clean-up steps [%lu] found.\n", npu_proto_drv.open_steps);

	state_transition(PROTO_DRV_STATE_PROBED);
	npu_info("complete in proto_drv_close\n");
	return 0;
}

int proto_drv_start(struct npu_device *npu_device)
{
	npu_info("start in proto_drv_start\n");
	npu_info("complete in proto_drv_start\n");
	return 0;
}

int proto_drv_stop(struct npu_device *npu_device)
{
	npu_info("start in proto_drv_stop\n");
	npu_info("complete in proto_drv_stop\n");
	return 0;
}

/* Unit test */
#ifdef CONFIG_VISION_UNITTEST
#define IDIOT_TESTCASE_IMPL "npu-protodrv.idiot"
#include "idiot-def.h"
#endif
