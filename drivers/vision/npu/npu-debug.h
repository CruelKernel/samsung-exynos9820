/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _NPU_DEBUG_H_
#define _NPU_DEBUG_H_

#include <linux/debugfs.h>

struct npu_debug {
	struct dentry *dfile_root;
	unsigned long  state;
};

/* Forward declaration */
struct npu_device;

int npu_debug_probe(struct npu_device *npu_device);
int npu_debug_release(void);
int npu_debug_open(struct npu_device *npu_device);
int npu_debug_close(struct npu_device *npu_device);
int npu_debug_start(struct npu_device *npu_device);
int npu_debug_stop(struct npu_device *npu_device);

int npu_debug_register(const char *name, const struct file_operations *ops);
int npu_debug_register_arg(
	const char *name, void *private_arg,
	const struct file_operations *ops);

#endif /* _NPU_DEBUG_H_ */
