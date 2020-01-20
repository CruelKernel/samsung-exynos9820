/*
 * Samsung Exynos SoC series SCore driver
 *
 * Frame manager module (frame is unit of task that user app requests)
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_FRAME_H__
#define __SCORE_FRAME_H__

#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/wait.h>

#include "score_util.h"
#include "score_mmu.h"

struct score_context;

/* The number of max frame in one context (TODO this is temporal value) */
#define SCORE_MAX_FRAME		128

/**
 * enum score_frame_state - State of frame depending on task progression
 * @SCORE_FRAME_STATE_READY:
 *	state that frame is created and packet is not yey writed at scq
 * @SCORE_FRAME_STATE_PROCESS:
 *	state that packet is writed at scq
 * @SCORE_FRAME_STATE_PENDING:
 *	state that writing at scq failed because scq is full
 * @SCORE_FRAME_STATE_COMPLETE:*
 state that task is done if task succeeded or failefd
 */
enum score_frame_state {
	SCORE_FRAME_STATE_READY = 1,
	SCORE_FRAME_STATE_PROCESS,
	SCORE_FRAME_STATE_PENDING,
	SCORE_FRAME_STATE_COMPLETE
};

/**
 * enum score_frame_type - Task type of frame
 * @TYPE_BLOCK: task is blocking mode
 * @TYPE_NONBLOCK: task is non-blocking mode, must be waited
 * @TYPE_NONBLOCK_NOWAIT:
 *	task is non-blocking mode, needs not to be waited
 * @TYPE_NONBLOCK_REMOVE:
 *	task is non-blocking and no-wait mode, it is removed at isr
 * @TYPE_NONBLOCK_DESTROY:
 *	task is non-blocking mode, it is removed when manager is destroyed
 */
enum score_frame_type {
	TYPE_START,
	TYPE_BLOCK,
	TYPE_NONBLOCK,
	TYPE_NONBLOCK_NOWAIT,
	TYPE_NONBLOCK_REMOVE,
	TYPE_NONBLOCK_DESTROY,
	TYPE_END
};

/**
 * struct score_frame - Object allocated to one task
 * @state: state of frame
 * @frame_id: unique id allocated at one frame
 * @type: task type of this frame (block, non-block or no-wait)
 * @sctx: context which this frame is included in
 * @work: kthread work to write packet at scq
 * @owner: frame manager
 * @entire_list: list to be included in total list of frame manager
 * @state_list: list to be included in each state list of frame manager
 * @packet: packet address delivered from user
 * @pended: true if pended
 * @pending_packet: packet address when state of frame is changed to pending
 * @packet_size: packet ION buffer size
 * @ret: result value delivered form SCore device
 * @buffer_list: list of memory buffer matched with sc_buffer
 *	included in one task
 * @buffer_count: count of memory buffer matched with sc_buffer
 *	included in one task
 * @kernel_id: id of DSP kernel
 * @priority: true if triority is high
 * @task_type: optional execution type of DSP task
 * @timestamp: [optional] time to measure performance
 */
struct score_frame {
	unsigned int			state;
	unsigned int			frame_id;
	unsigned int			type;
	struct score_context		*sctx;
	struct list_head		list;
	struct kthread_work		work;

	struct score_frame_manager	*owner;
	struct list_head		entire_list;
	struct list_head		state_list;

	void				*packet;
	size_t				packet_size;
	bool				pended;
	void				*pending_packet;
	int				ret;
	struct list_head		buffer_list;
	unsigned int			buffer_count;

	unsigned int			kernel_id;
	bool				priority;
	unsigned int			task_type;
	struct timespec			timestamp[SCORE_TIME_POINT_NUM];
};

/**
 * struct score_frame_manager - Manager controlling state of frame
 * @entire_list: list of total frames
 * @entire_count: count of total frames
 * @ready_list: list of frames that state is ready
 * @ready_count: count of frames that state is ready
 * @process_list: list of frames that state is process
 * @process_count: count of all frames that state is process
 * @process_high_count: count of frames that state is process,
 *                      priority is high
 * @process_normal_count: count of frames that state is process,
 *                        priority is normal
 * @pending_list: list of frames that state is pending
 * @pending_count: count of all frames that state is pending
 * @pending_high_count: count of frames that state is pending, priority is high
 * @pending_normal_count: count of frames that state is pending,
 *                        priority is normal
 * @complete_list: list of frames that state is complete
 * @complete_count: count of frames that state is complete
 * @slock: spin lock to prevent state of frame overlapped
 * @frame_map: map to allocate frame id
 * @done_wq: wait queue to wake up task waitting result in blocking mode
 * @block: if block is true, frame manager can not be used
 */
struct score_frame_manager {
	struct list_head		entire_list;
	unsigned int			entire_count;
	struct list_head		ready_list;
	unsigned int			ready_count;
	struct list_head		process_list;
	unsigned int			process_count;
	unsigned int			process_high_count;
	unsigned int			process_normal_count;
	struct list_head		pending_list;
	unsigned int			pending_count;
	unsigned int			pending_high_count;
	unsigned int			pending_normal_count;
	struct list_head		complete_list;
	unsigned int			complete_count;

	unsigned int			all_count;
	unsigned int			normal_count;
	unsigned int			abnormal_count;

	spinlock_t			slock;
	DECLARE_BITMAP(frame_map, SCORE_MAX_FRAME);
	wait_queue_head_t		done_wq;
	bool				block;
};

void score_frame_add_buffer(struct score_frame *frame,
		struct score_mmu_buffer *buf);
void score_frame_remove_buffer(struct score_frame *frame,
		struct score_mmu_buffer *buf);
int score_frame_check_done(struct score_frame *frame);
void score_frame_done(struct score_frame *frame, int *ret);

int score_frame_trans_ready_to_process(struct score_frame *frame);
int score_frame_trans_ready_to_complete(struct score_frame *frame, int result);
int score_frame_trans_process_to_pending(struct score_frame *frame);
int score_frame_trans_process_to_complete(struct score_frame *frame,
		int result);
int score_frame_trans_pending_to_ready(struct score_frame *frame);
int score_frame_trans_pending_to_complete(struct score_frame *frame,
		int result);
int score_frame_trans_any_to_complete(struct score_frame *frame, int result);
unsigned int score_frame_get_state(struct score_frame *frame);
bool score_frame_check_type(struct score_frame *frame, int type);
void score_frame_set_type_block(struct score_frame *frame);
void score_frame_set_type_remove(struct score_frame *frame);
void score_frame_set_type_destroy(struct score_frame *frame);

struct score_frame *score_frame_get_process_by_id(
		struct score_frame_manager *framemgr, unsigned int id);
struct score_frame *score_frame_get_by_id(
		struct score_frame_manager *framemgr, unsigned int id);
struct score_frame *score_frame_get_first_pending(
		struct score_frame_manager *framemgr, bool priority);
unsigned int score_frame_get_pending_count(
		struct score_frame_manager *framemgr, bool priority);
void score_frame_flush_process(struct score_frame_manager *framemgr,
		int result);
void score_frame_flush_all(struct score_frame_manager *framemgr, int result);
void score_frame_remove_nonblock_all(struct score_frame_manager *framemgr);

void score_frame_manager_block(struct score_frame_manager *framemgr);
void score_frame_manager_unblock(struct score_frame_manager *framemgr);

struct score_frame *score_frame_create(struct score_frame_manager *framemgr,
		struct score_context *sctx, int type);
void score_frame_destroy(struct score_frame *frame);

int score_frame_manager_probe(struct score_frame_manager *framemgr);
void score_frame_manager_remove(struct score_frame_manager *framemgr);

#endif
