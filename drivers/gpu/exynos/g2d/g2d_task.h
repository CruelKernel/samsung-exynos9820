/*
 * linux/drivers/gpu/exynos/g2d/g2d_task.h
 *
 * Copyright (C) 2017 Samsung Electronics Co., Ltd.
 *
 * Contact: Hyesoo Yu <hyesoo.yu@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef __EXYNOS_G2D_TASK_H__
#define __EXYNOS_G2D_TASK_H__

#include <linux/ktime.h>
#include <linux/dma-buf.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/sync_file.h>

#include "g2d_format.h"

#define G2D_MAX_IMAGES		16
#define G2D_MAX_IMAGES_HALF	8
#define G2D_MAX_JOBS		16
#define G2D_CMD_LIST_SIZE	8192

struct g2d_buffer_prot_info {
	unsigned int chunk_count;
	unsigned int dma_addr;
	unsigned int flags;
	unsigned int chunk_size;
	unsigned long bus_address;
};

struct g2d_buffer {
	union {
		struct {
			unsigned int			offset;
			struct dma_buf			*dmabuf;
			struct dma_buf_attachment	*attachment;
			struct sg_table			*sgt;
		} dmabuf;
		struct {
			unsigned long			addr;
			struct vm_area_struct		*vma;
		} userptr;
	};
	unsigned int	length;
	unsigned int	payload;
	dma_addr_t	dma_addr;
};

struct g2d_layer {
	struct g2d_task		*task;
	int			flags;
	int			buffer_type;
	int			num_buffers;
	struct g2d_buffer	buffer[G2D_MAX_PLANES];
	struct dma_fence	*fence;
	struct dma_fence_cb	fence_cb;
	struct g2d_reg		*commands;
};

#define G2D_TASKSTATE_WAITING		(1 << 1)
#define G2D_TASKSTATE_UNPREPARED	(1 << 2)
#define G2D_TASKSTATE_PREPARED		(1 << 3)
#define G2D_TASKSTATE_ACTIVE		(1 << 4)
#define G2D_TASKSTATE_PROCESSED		(1 << 5)
#define G2D_TASKSTATE_ERROR		(1 << 6)
#define G2D_TASKSTATE_KILLED		(1 << 7)
#define G2D_TASKSTATE_TIMEOUT		(1 << 8)

struct g2d_context;
struct g2d_device;

#define IS_HWFC(flags)	(!!((flags) & G2D_FLAG_HWFC))

struct g2d_task {
	struct list_head	node;
	struct g2d_task		*next;
	struct g2d_device	*g2d_dev;

	unsigned int		flags;
	unsigned int		bufidx;
	unsigned long		state;
	struct sync_file	*release_fence;
	struct kref		starter;
	struct timer_list	fence_timer;
	struct timer_list	hw_timer;

	struct g2d_layer	*source;
	struct g2d_layer	target;
	unsigned int		num_source;

	/* Command list */
	struct page		*cmd_page;
	dma_addr_t		cmd_addr;

	struct {
		unsigned long	cmd_paddr;
		unsigned int	cmd_count;
		unsigned int	priority;
		unsigned int	job_id;
		/* temporarily used by g2d_hw_push_task_by_smc */
		int		secure_layer_mask;
	} sec;

	ktime_t			ktime_begin;
	ktime_t			ktime_end;

	struct work_struct	work;
	struct completion	completion;

	unsigned int		total_cached_len;
	unsigned int		total_hwrender_len;
	spinlock_t		fence_timeout_lock;
#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	struct g2d_buffer_prot_info prot_info;
#endif
};

/* The below macros should be called with g2d_device.lock_tasks held */
#define change_task_state_active(task) do {		\
	(task)->state &= ~G2D_TASKSTATE_PREPARED;	\
	(task)->state |= G2D_TASKSTATE_ACTIVE;		\
} while (0)

#define change_task_state_prepared(task) do {		\
	(task)->state |= G2D_TASKSTATE_PREPARED;	\
	(task)->state &= ~G2D_TASKSTATE_UNPREPARED;	\
} while (0)

#define change_task_state_finished(task) do {		\
	(task)->state &= ~(G2D_TASKSTATE_ACTIVE |	\
			   G2D_TASKSTATE_KILLED |	\
			   G2D_TASKSTATE_TIMEOUT);	\
	(task)->state |= G2D_TASKSTATE_PROCESSED;	\
} while (0)

static inline void mark_task_state_error(struct g2d_task *task)
{
	task->state |= G2D_TASKSTATE_ERROR;
}

static inline void mark_task_state_killed(struct g2d_task *task)
{
	task->state |= G2D_TASKSTATE_KILLED;
}

static inline void init_task_state(struct g2d_task *task)
{
	task->state = G2D_TASKSTATE_UNPREPARED;
}

static inline void clear_task_state(struct g2d_task *task)
{
	task->state = 0;
}

#define is_task_state_idle(task)   ((task)->state == 0)
#define is_task_state_active(task) (((task)->state & G2D_TASKSTATE_ACTIVE) != 0)
#define is_task_state_killed(task) (((task)->state & G2D_TASKSTATE_KILLED) != 0)
#define is_task_state_error(task)  (((task)->state & G2D_TASKSTATE_ERROR) != 0)

static inline bool g2d_task_wait_completion(struct g2d_task *task)
{
	wait_for_completion(&task->completion);
	return !is_task_state_error(task);
}

#define G2D_HW_TIMEOUT_MSEC	500
#define G2D_FENCE_TIMEOUT_MSEC	800

struct g2d_task *g2d_get_active_task_from_id(struct g2d_device *g2d_dev,
					     unsigned int id);
void g2d_destroy_tasks(struct g2d_device *g2d_dev);
int g2d_create_tasks(struct g2d_device *g2d_dev);

struct g2d_task *g2d_get_free_task(struct g2d_device *g2d_dev,
				    struct g2d_context *g2d_ctx, bool hwfc);
void g2d_put_free_task(struct g2d_device *g2d_dev, struct g2d_task *task);

void g2d_start_task(struct g2d_task *task);
void g2d_cancel_task(struct g2d_task *task);
void g2d_finish_task_with_id(struct g2d_device *g2d_dev,
			     unsigned int job_id, bool success);
void g2d_flush_all_tasks(struct g2d_device *g2d_dev);

void g2d_prepare_suspend(struct g2d_device *g2d_dev);
void g2d_suspend_finish(struct g2d_device *g2d_dev);

void g2d_fence_callback(struct dma_fence *fence, struct dma_fence_cb *cb);

void g2d_queuework_task(struct kref *kref);

#endif /*__EXYNOS_G2D_TASK_H__*/
