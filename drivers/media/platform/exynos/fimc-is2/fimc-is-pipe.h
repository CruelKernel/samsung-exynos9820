/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef FIMC_IS_PIPE_H
#define FIMC_IS_PIPE_H

#include "fimc-is-groupmgr.h"

#define FIMC_IS_MAX_PIPE_BUFS (5)

enum pipe_slot_type {
	PIPE_SLOT_SRC,
	PIPE_SLOT_JUNCTION,
	PIPE_SLOT_DST,
	PIPE_SLOT_MAX,
};

struct fimc_is_pipe {
	u32 id;
	struct fimc_is_group *src;
	struct fimc_is_group *dst;
	struct fimc_is_video_ctx *vctx[PIPE_SLOT_MAX];
	struct camera2_dm pipe_dm;
	struct camera2_udm pipe_udm;
	struct v4l2_buffer buf[PIPE_SLOT_MAX][FIMC_IS_MAX_PIPE_BUFS];
	struct v4l2_plane planes[PIPE_SLOT_MAX][FIMC_IS_MAX_PIPE_BUFS][FIMC_IS_MAX_PLANES];
};

int fimc_is_pipe_probe(struct fimc_is_pipe *pipe);
int fimc_is_pipe_create(struct fimc_is_pipe *pipe,
	struct fimc_is_group *src,
	struct fimc_is_group *dst);
#endif
