/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Header file for Exynos CAMERA-PP GDC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef CAMERAPP_STR_CORE_H
#define CAMERAPP_STR_CORE_H

#include "camerapp-str-video.h"
#include "camerapp-str-hw.h"

#define MODULE_NAME		"exynos5-camerapp-str"

#define video_to_core(x)	container_of(x, struct str_core, video_str)

/*
 * struct str_ion_ctx - The ION context structure for internal memory
 * @dev:		pointer to the device structure.
 * @align:		size alignment value.
 * @flags:		ION allocation flag.
 * @lock:		mutex for ion_ctx.
 */
struct str_ion_ctx {
	struct device	*dev;
	unsigned long	align;
	long		flags;
	struct mutex	lock;
};

/*
 * struct str_core - The abstraction for STR device
 * @dev:	pointer to the device structure.
 * @video_str:	video structure for STR video device.
 * @hw:		STR H/W abstracted structure.
 * @ion_ctx:	ION context structure.
 */
struct str_core {
	struct device		*dev;
	struct str_video	video_str;
	struct clk		*clk_mux;
	struct clk		*clk_gate;
	struct str_hw		hw;
	struct str_ion_ctx	ion_ctx;
};

struct str_priv_buf *str_ion_alloc(struct str_ion_ctx *ion_ctx, size_t size);
void str_ion_free(struct str_priv_buf *buf);
int str_power_clk_enable(struct str_core *core);
int str_power_clk_disable(struct str_core *core);
int str_shot(struct str_ctx *ctx);

#endif //CAMERAPP_STR_CORE_H
