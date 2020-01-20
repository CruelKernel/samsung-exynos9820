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

#include "npu-log.h"
#include "npu-system.h"

#ifdef CONFIG_FIRMWARE_SRAM_DUMP_DEBUGFS
int npu_util_memdump_probe(struct npu_system *system);
int npu_util_memdump_open(struct npu_system *system);
int npu_util_memdump_close(struct npu_system *system);
int npu_util_memdump_start(struct npu_system *system);
int npu_util_memdump_stop(struct npu_system *system);
#else
#define npu_util_memdump_probe(...)	(0)
#define npu_util_memdump_open(...)	(0)
#define npu_util_memdump_close(...)	(0)
#define npu_util_memdump_start(...)	(0)
#define npu_util_memdump_stop(...)	(0)
#endif

int npu_util_dump_handle_error_k(struct npu_device *device);

