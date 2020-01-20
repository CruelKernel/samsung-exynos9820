/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "vs4l.h"

#ifndef VISION_IOCTL_H_
#define VISION_IOCTL_H_

struct vertex_ioctl_ops {
	int (*vertexioc_s_graph)(struct file *file, struct vs4l_graph *graph);
	int (*vertexioc_s_format)(struct file *file, struct vs4l_format_list *flist);
	int (*vertexioc_s_param)(struct file *file, struct vs4l_param_list *plist);
	int (*vertexioc_s_ctrl)(struct file *file, struct vs4l_ctrl *ctrl);
	int (*vertexioc_qbuf)(struct file *file, struct vs4l_container_list *clist);
	int (*vertexioc_dqbuf)(struct file *file, struct vs4l_container_list *clist);
	int (*vertexioc_prepare)(struct file *file, struct vs4l_container_list *clist);
	int (*vertexioc_unprepare)(struct file *file, struct vs4l_container_list *clist);
	int (*vertexioc_streamon)(struct file *file);
	int (*vertexioc_streamoff)(struct file *file);
};

long vertex_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
long vertex_compat_ioctl32(struct file *file, unsigned int cmd, unsigned long arg);

#endif

