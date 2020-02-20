/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef NPU_BINARY_H_
#define NPU_BINARY_H_

#include <linux/device.h>
#include <linux/firmware.h>

#define NPU_FW_PATH1		"/data/"
#define NPU_FW_PATH2		"/vendor/firmware/"
#define NPU_FW_PATH3		"npu/"
#define NPU_FW_NAME		"NPU.bin"
#define NPU_FW_NAME_LEN	100
#define NPU_VERSION_SIZE	42

struct npu_binary {
	struct device		*dev;
	char			fpath1[NPU_FW_NAME_LEN]; /* first try to load */
	char			fpath2[NPU_FW_NAME_LEN]; /* second try to load */
	size_t			image_size;
};

#define NCP_BIN_PATH		"/data/"
#define NCP_BIN_NAME		"ncp_object.bin"
#define NCP_BIN_NAME_LEN	100
#define NCP_VERSION_SIZE	42

struct ncp_binary {
	struct device		*dev;
	char			fpath1[NCP_BIN_NAME_LEN]; /* first try to load */
	char			fpath2[NCP_BIN_NAME_LEN]; /* second try to load */
	size_t			image_size;
};

int npu_binary_init(struct npu_binary *binary,
	struct device *dev,
	char *fpath1,
	char *fpath2,
	char *fname);
int npu_binary_read(struct npu_binary *binary,
	void *target,
	size_t target_size);
int npu_firmware_file_read(struct npu_binary *binary,
	void *target,
	size_t target_size);
int npu_binary_write(struct npu_binary *binary,
	void *target,
	size_t target_size);
int npu_binary_g_size(struct npu_binary *binary, size_t *size);

#endif
