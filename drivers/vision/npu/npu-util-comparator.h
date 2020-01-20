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

#ifndef _NPU_UTIL_COMPARATOR_H_
#define _NPU_UTIL_COMPARATOR_H_

#include <linux/types.h>
#include "npu-golden.h"

/* Represent feature map dimension */
struct npu_fmap_dim {
	size_t          axis1;
	size_t          axis2;
	size_t          axis3;
};

typedef int (*golden_comparator)(u32, const u8*, const u8*, const struct npu_fmap_dim, struct npu_golden_compare_result*);
typedef int (*golden_is_av_valid)(const u32, const u32, struct npu_golden_compare_result*);

int simple_compare(u32 flags, const u8 *golden, const u8 *target, const struct npu_fmap_dim dim, struct npu_golden_compare_result *result);
int reordered_compare(u32 flags, const u8 *golden, const u8 *target, const struct npu_fmap_dim dim, struct npu_golden_compare_result *result);

int is_av_valid_reordered(const u32 golden_size, const u32 target_size, struct npu_golden_compare_result *result);
int is_av_valid_simple(const u32 golden_size, const u32 target_size, struct npu_golden_compare_result *result);

#endif	/* _NPU_UTIL_COMPARATOR_H_ */

