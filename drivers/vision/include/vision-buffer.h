/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VISION_BUFFER_H_
#define VISION_BUFFER_H_

#include <linux/mm_types.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/dma-buf.h>
#include <linux/time.h>

#include <media/videobuf2-core.h>
#if defined(CONFIG_VIDEOBUF2_CMA_PHYS)
#include <media/videobuf2-cma-phys.h>
#elif defined(CONFIG_VIDEOBUF2_ION)
#include <media/videobuf2-ion.h>
#endif

#include "vision-config.h"
#include "vs4l.h"

#define VB_MAX_BUFFER			VISION_MAX_BUFFER
#define VB_MAX_PLANES			VISION_MAX_PLANE

struct vb_queue;

struct vb_fmt {
	char				*name;
	u32				colorspace;
	u32				planes;
	u32				bitsperpixel[VB_MAX_PLANES];
};

struct vb_format {
	u32				target;
	struct vb_fmt			*fmt;
	u32				colorspace;
	u32				plane;
	u32				width;
	u32				height;
	u32				size[VB_MAX_PLANES];
	u32				stride;
	u32				cstride;
	u32				channels;
	u32				pixel_format;

};

struct vb_format_list {
	u32				count;
	struct vb_format		*formats;
};

struct vb_buffer {
	struct vs4l_roi			roi;
	union {
		unsigned long		userptr;
		__s32			fd;
	} m;
	struct dma_buf			*dma_buf;
	struct dma_buf_attachment	*attachment;
	struct sg_table			*sgt;
	dma_addr_t			daddr;
	void				*vaddr;
	//size_t				size;
	ulong				reserved;
};

struct vb_container {
	u32				type;
	u32				target;
	u32				memory;
	u32				reserved[4];
	u32				count;
	struct vb_buffer		*buffers;
	struct vb_format		*format;
};

struct vb_container_list {
	u32				direction;
	u32				id;
	u32				index;
	unsigned long			flags;
	struct timeval			timestamp[6];
	u32				count;
	struct vb_container		*containers;
};

enum vb_bundle_state {
	VB_BUF_STATE_DEQUEUED,
	VB_BUF_STATE_QUEUED,
	VB_BUF_STATE_PROCESS,
	VB_BUF_STATE_DONE,
};

struct vb_bundle {
	/* this flag is for internal state */
	unsigned long			flags;
	enum vb_bundle_state		state;
	struct list_head		queued_entry;
	struct list_head		process_entry;
	struct list_head		done_entry;

	struct vb_container_list	clist;
};

struct vb_ops {
	int (*buf_prepare)(struct vb_queue *q, struct vb_container_list *clist);
	int (*buf_unprepare)(struct vb_queue *q, struct vb_container_list *clist);
};

enum vb_queue_state {
	VB_QUEUE_STATE_FORMAT,
	VB_QUEUE_STATE_START
};

struct vb_queue {
	u32				direction;
	const char			*name;
	unsigned long			state;
	unsigned int			streaming:1;
	struct mutex			*lock;

	struct list_head		queued_list;
	atomic_t			queued_count;
	struct list_head		process_list;
	atomic_t			process_count;
	struct list_head		done_list;
	atomic_t			done_count;

	spinlock_t			done_lock;
	wait_queue_head_t		done_wq;

	struct vb_format_list		format;
	struct vb_bundle		*bufs[VB_MAX_BUFFER];
	unsigned int			num_buffers;

	void				*alloc_ctx;
	const struct vb2_mem_ops	*mem_ops;
	const struct vb_ops		*ops;
	void				*private_data;
};

int vb_queue_init(struct vb_queue *q, void *alloc_ctx, const struct vb2_mem_ops *mem_ops, const struct vb_ops *ops, struct mutex *lock, u32 direction);
int vb_queue_s_format(struct vb_queue *q, struct vs4l_format_list *f);
int vb_queue_start(struct vb_queue *q);
int vb_queue_stop(struct vb_queue *q);
int vb_queue_stop_forced(struct vb_queue *q);
int vb_queue_qbuf(struct vb_queue *q, struct vs4l_container_list *c);
int vb_queue_dqbuf(struct vb_queue *q, struct vs4l_container_list *c, bool nonblocking);
int vb_queue_prepare(struct vb_queue *q, struct vs4l_container_list *c);
int vb_queue_unprepare(struct vb_queue *q, struct vs4l_container_list *c);
void vb_queue_process(struct vb_queue *q, struct vb_bundle *vb);
void vb_queue_done(struct vb_queue *q, struct vb_bundle *vb);

#define call_memop(q, op, args...) (((q)->mem_ops->op) ? ((q)->mem_ops->op(args)) : 0)
#define call_op(q, op, args...) (((q)->ops->op) ? ((q)->ops->op(args)) : 0)

#endif
