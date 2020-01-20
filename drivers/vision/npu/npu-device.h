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

#ifndef _NPU_DEVICE_H_
#define _NPU_DEVICE_H_

#include "vision-dev.h"
#include "npu-log.h"
#include "npu-debug.h"
#include "npu-vertex.h"
#include "npu-system.h"
#include "npu-protodrv.h"
#include "npu-sessionmgr.h"

#ifdef CONFIG_NPU_HARDWARE
//#include "interface/hardware/npu-interface.h"
#endif

#ifdef CONFIG_NPU_GOLDEN_MATCH
#include "npu-golden.h"
#endif

//#define NPU_MINOR	10
#define NPU_DEVICE_NAME	"npu-turing"
//#define NPU_VERTEX_NAME "vertex"


enum npu_device_state {
	NPU_DEVICE_STATE_OPEN,
	NPU_DEVICE_STATE_START
};

enum npu_device_mode {
	NPU_DEVICE_MODE_NORMAL,
	NPU_DEVICE_MODE_TEST
};

enum npu_device_err_state {
	NPU_DEVICE_ERR_STATE_EMERGENCY
};

struct npu_device {
	struct device *dev;
	unsigned long state;
	unsigned long err_state;
	u32 mode;
	struct npu_system system;
	struct npu_vertex vertex;
	struct npu_debug debug;
	struct npu_proto_drv *proto_drv;
	struct npu_sessionmgr sessionmgr;
#ifdef CONFIG_NPU_GOLDEN_MATCH
	struct npu_golden_ctx *npu_golden_ctx;
#endif
	int magic;
};

int npu_device_open(struct npu_device *device);
int npu_device_close(struct npu_device *device);
int npu_device_start(struct npu_device *device);
int npu_device_stop(struct npu_device *device);

//int npu_device_emergency_recover(struct npu_device *device);
void npu_device_set_emergency_err(struct npu_device *device);
int npu_device_is_emergency_err(struct npu_device *device);
int npu_device_recovery_close(struct npu_device *device);
#endif // _NPU_DEVICE_H_
