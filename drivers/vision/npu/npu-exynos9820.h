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

#include <linux/io.h>
#include <linux/delay.h>

#include "npu-config.h"
#include "npu-exynos.h"
#include "npu-debug.h"

#include "npu-log.h"

#ifndef NPU_EXYNOS9820_H_
#define NPU_EXYNOS9820_H_

#define CMU_OFFSET	0	/* CMU is located at offset 0 of SFR area */
const u32 CMU_NPU0_BASE = NPU_IOMEM_SFR_NPU0_START + CMU_OFFSET;
const u32 CMU_NPU1_BASE = NPU_IOMEM_SFR_NPU1_START + CMU_OFFSET;

//------------------------------------
enum npu0_cmuclk_reg_name {
	MUX_CLKCMU_NPU0_BUS,
	MUX_CLKCMU_NPU0_CPU,
	DIV_CLKCMU_NPU0_BUS,
	DIV_CLKCMU_NPU0_CPU
};
struct npu_reg npu0_cmuclk_regs[] = {
	{0x0100, "MUX_CLKCMU_NPU0_BUS"},
	{0x0120, "MUX_CLKCMU_NPU0_CPU"},
	{0x1800, "DIV_CLKCMU_NPU0_BUS"},
	{0x1804, "DIV_CLKCMU_NPU0_CPU"}
};
//------------------------------------
enum npu1_cmuclk_reg_name {
	MUX_CLKCMU_NPU1_BUS,
	DIV_CLKCMU_NPU1_BUS
};
struct npu_reg npu1_cmuclk_regs[] = {
	{0x0100, "MUX_CLKCMU_NPU1_BUS"},
	{0x1800, "DIV_CLKCMU_NPU1_BUS"}
};
//------------------------------------


#endif
