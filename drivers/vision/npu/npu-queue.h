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

#ifndef _NPU_QUEUE_H_
#define _NPU_QUEUE_H_

#include <linux/types.h>
#include <linux/mutex.h>

#include "vision-buffer.h"
#include "npu-memory.h"

struct npu_queue;

struct npu_queue_ops {
	int (*start)(struct npu_queue *queue);
	int (*stop)(struct npu_queue *queue);
	int (*format)(struct npu_queue *queue, struct vs4l_format_list *f);
	int (*queue)(struct npu_queue *queue, struct vb_container_list *incl, struct vb_container_list *otcl);
	int (*deque)(struct npu_queue *queue, struct vb_container_list *clist);
	int (*streamoff)(struct npu_queue *queue);
};

struct npu_queue {
	struct vb_queue			inqueue;
	struct vb_queue			otqueue;
	const struct npu_queue_ops	*qops;
};

int npu_queue_open(struct npu_queue *queue, struct npu_memory *memory, struct mutex *lock);
int npu_queue_s_format(struct npu_queue *queue, struct vs4l_format_list *f);
int npu_queue_start(struct npu_queue *queue);
int npu_queue_stop(struct npu_queue *queue, int is_forced);
int npu_queue_poll(struct npu_queue *queue, struct file *file, poll_table *poll);
int npu_queue_qbuf(struct npu_queue *queue, struct vs4l_container_list *c);
int npu_queue_dqbuf(struct npu_queue *queue, struct vs4l_container_list *c, bool nonblocking);
int npu_queue_prepare(struct npu_queue *queue, struct vs4l_container_list *c);
int npu_queue_unprepare(struct npu_queue *queue, struct vs4l_container_list *c);
int npu_queue_streamoff(struct npu_queue *queue);
void npu_queue_done(struct npu_queue *queue, struct vb_container_list *incl, struct vb_container_list *otcl, unsigned long flags);

#define CALL_QOPS(q, op, ...)	(((q)->qops->op) ? ((q)->qops->op(q, ##__VA_ARGS__)) : 0)

extern void __vb_queue_print(struct vb_queue *q);
extern void __npu_queue_print(struct vb_queue *q);

extern const struct vb_ops vb_ops;
extern const struct npu_queue_ops npu_queue_ops;	/* Defined in npu-session.c */

#endif
