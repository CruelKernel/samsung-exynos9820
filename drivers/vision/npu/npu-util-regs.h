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

#ifndef _NPU_UTIL_REGS_H_
#define _NPU_UTIL_REGS_H_

#include "npu-common.h"

/* Structure used to specify SFR entry */
struct reg_set_map {
	u32             offset;
	u32             val;
	u32             mask;
};

struct reg_set_map_2 {
	struct npu_iomem_area   *iomem_area;
	u32                     offset;
	u32                     val;
	u32                     mask;
};

void npu_write_hw_reg(const struct npu_iomem_area *base, u32 offset, u32 val, u32 mask);
int npu_set_hw_reg(
	const struct npu_iomem_area *base, const struct reg_set_map *set_map,
	size_t map_len, int regset_mdelay);
int npu_set_hw_reg_2(const struct reg_set_map_2 *set_map, size_t map_len, int regset_mdelay);
int npu_set_sfr(const u32 sfr_addr, const u32 value, const u32 mask);
int npu_get_sfr(const u32 sfr_addr);


#endif	/* _NPU_UTIL_REGS_H_ */
