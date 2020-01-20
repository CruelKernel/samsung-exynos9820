/*
 * Samsung Exynos SoC series SCore driver
 *
 * SCore context is unit of data allocated when user process(thread)
 * opens SCore dev
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_CONTEXT_H__
#define __SCORE_CONTEXT_H__

#include "score_core.h"
#include "score_frame.h"

struct score_frame;
struct score_context;

/**
 * struct score_context_ops - Operations controlling context
 * @queue: prepare command and send that to SCore device
 * @deque: wait result response of SCore device and receive that
 */
struct score_context_ops {
	int (*queue)(struct score_context *sctx, struct score_frame *frame);
	int (*deque)(struct score_context *sctx, struct score_frame *frame);
};

/**
 * struct score_context - Context allocated by open call about SCore dev
 * @id: unique id of each context
 * @state: state of context (TODO this is not fixed)
 * @mmu_ctx: object about score_mmu_context structure
 * @list: list to be included in score_core
 * @frame_list: list of frame running in this context
 * @frame_count: count of frame running in this context
 * @wait_time: max time that context can wait result response (unit : ms)
 * @ctx_ops: operations controlling context
 * @core: object about score_core structure
 * @system: object about score_system structure
 * @blocking: information if this context is blocking mode or not
 */
struct score_context {
	unsigned int			id;
	unsigned int			state;
	struct score_mmu_context	*mmu_ctx;
	struct list_head		list;
	struct list_head		frame_list;
	unsigned int			frame_count;
	unsigned int			frame_total_count;
	unsigned int			wait_time;

	const struct score_context_ops	*ctx_ops;
	struct score_core		*core;
	struct score_system		*system;

	int				blocking;
};

struct score_context *score_context_create(struct score_core *core);
void score_context_destroy(struct score_context *sctx);

#endif

