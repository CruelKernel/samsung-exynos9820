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

#ifndef _NPU_VERTEX_H_
#define _NPU_VERTEX_H_

#include "vision-dev.h"
#include "vision-ioctl.h"
#include "npu-queue.h"

struct npu_vertex;

enum npu_vertex_state {
	NPU_VERTEX_OPEN,
	NPU_VERTEX_GRAPH,
	NPU_VERTEX_FORMAT,
	NPU_VERTEX_STREAMON,
	NPU_VERTEX_STREAMOFF,
	NPU_VERTEX_CLOSE
};

struct npu_vertex_refcount {
	atomic_t			refcount;
	struct npu_vertex		*vertex;
	int				(*first)(struct npu_vertex *vertex);
	int				(*final)(struct npu_vertex *vertex);
};

struct npu_vertex {
	struct mutex lock;
	struct vision_device vd;
	struct npu_vertex_refcount open_cnt;
	struct npu_vertex_refcount start_cnt;
};

struct npu_vertex_ctx {
	u32 state;
	u32 id;
	struct mutex lock;
	struct npu_queue queue;
	struct npu_vertex *vertex;
};

int npu_vertex_probe(struct npu_vertex *vertex, struct device *parent);
#endif

