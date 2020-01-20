/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_DEVICE_PREPROCESSOR_H
#define FIMC_IS_DEVICE_PREPROCESSOR_H

#include <linux/interrupt.h>
#include <exynos-fimc-is-sensor.h>
#include <exynos-fimc-is-preprocessor.h>
#include "fimc-is-video.h"
#include "fimc-is-config.h"

#define FIMC_IS_PREPROC_DEV_NAME "exynos-fimc-is-preprocessor"

#define PREPROC_SCENARIO_MASK		0xF0000000
#define PREPROC_SCENARIO_SHIFT		28
#define PREPROC_MODULE_MASK		0x0FFFFFFF
#define PREPROC_MODULE_SHIFT		0
#define MAX_PREPROC_S_INPUT_WAITING	2000 /* 2 second */
#define PREPROC_PDAF_BUF_NUM 2

enum fimc_is_preproc_state {
	FIMC_IS_PREPROC_OPEN,
	FIMC_IS_PREPROC_MCLK_ON,
	FIMC_IS_PREPROC_ICLK_ON,
	FIMC_IS_PREPROC_GPIO_ON,
	FIMC_IS_PREPROC_S_INPUT
};

struct fimc_is_device_preproc {
	struct v4l2_device				v4l2_dev;
	struct platform_device				*pdev;
	void __iomem					*regs;
	struct fimc_is_mem				mem;
	u32						instance;

	struct fimc_is_video_ctx			*vctx;
	struct fimc_is_video				video;

	unsigned long					state;

	struct fimc_is_resourcemgr			*resourcemgr;
	struct exynos_platform_fimc_is_preproc		*pdata;
	struct fimc_is_module_enum			*module;
	void						*private_data;
};

int fimc_is_preproc_open(struct fimc_is_device_preproc *device,
	struct fimc_is_video_ctx *vctx);
int fimc_is_preproc_close(struct fimc_is_device_preproc *device);
int fimc_is_preproc_s_input(struct fimc_is_device_preproc *device,
	u32 input,
	u32 scenario);
int fimc_is_preproc_g_module(struct fimc_is_device_preproc *device,
	struct fimc_is_module_enum **module);
int fimc_is_preproc_runtime_suspend(struct device *dev);
int fimc_is_preproc_runtime_resume(struct device *dev);
#endif
