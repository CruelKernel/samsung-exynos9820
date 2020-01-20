/*
 * linux/drivers/gpu/exynos/g2d/g2d_fence.h
 *
 * Copyright (C) 2017 Samsung Electronics Co., Ltd.
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

#ifndef __EXYNOS_G2D_FENCE_H__
#define __EXYNOS_G2D_FENCE_H__

struct g2d_device;
struct g2d_layer;
struct g2d_task;
struct g2d_task_data;
struct dma_fence;

void g2d_fence_timeout_handler(unsigned long arg);
struct dma_fence *g2d_get_acquire_fence(struct g2d_device *g2d_dev,
					struct g2d_layer *layer, s32 fence_fd);
struct sync_file *g2d_create_release_fence(struct g2d_device *g2d_dev,
					   struct g2d_task *task,
					   struct g2d_task_data *data);
bool g2d_task_has_error_fence(struct g2d_task *task);

#endif /*__EXYNOS_G2D_FENCE_H__*/
