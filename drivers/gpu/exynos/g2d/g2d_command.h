/*
 * linux/drivers/gpu/exynos/g2d/g2d_command.h
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

#ifndef _G2D_COMMAND_H_
#define _G2D_COMMAND_H_

#include "g2d_format.h"

struct g2d_device;
struct g2d_task;
struct g2d_layer;
struct g2d_task_data;

struct g2d_fmt {
	const char	*name;
	u32		fmtvalue;
	u8		bpp[G2D_MAX_PLANES];
	u8		num_planes;
};

void g2d_init_commands(struct g2d_task *task);
void g2d_complete_commands(struct g2d_task *task);
const struct g2d_fmt *g2d_find_format(u32 fmtval, unsigned long devcaps);
int g2d_import_commands(struct g2d_device *g2d_dev, struct g2d_task *task,
			struct g2d_task_data *data, unsigned int num_sources);
bool g2d_validate_source_commands(struct g2d_device *g2d_dev,
				  struct g2d_task *task,
				  unsigned int i, struct g2d_layer *source,
				  struct g2d_layer *target);
bool g2d_validate_target_commands(struct g2d_device *g2d_dev,
				  struct g2d_task *task);
size_t g2d_get_payload(struct g2d_reg cmd[], const struct g2d_fmt *fmt,
		       u32 flags, unsigned long caps);
size_t g2d_get_payload_index(struct g2d_reg cmd[], const struct g2d_fmt *fmt,
			     unsigned int idx, unsigned int buffer_count,
			     unsigned long caps);
bool g2d_prepare_source(struct g2d_task *task,
			struct g2d_layer *layer, int index);
bool g2d_prepare_target(struct g2d_task *task);

#endif /* _G2D_COMMAND_H_ */
