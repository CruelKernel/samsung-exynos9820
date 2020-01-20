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

#include <linux/kernel.h>
#include <linux/types.h>
#include "npu-util-comparator.h"
#include "npu-log.h"

int simple_compare(u32 flags, const u8 *golden, const u8 *target, struct npu_fmap_dim dim, struct npu_golden_compare_result *result)
{
	size_t c, h, w;

	const u8 *pgolden = golden;
	const u8 *ptarget = target;

	int diff_cnt = 0;
	int match_cnt = 0;

	BUG_ON(!pgolden);
	BUG_ON(!ptarget);

	for (c = 0; c < dim.axis1; c++) {
		for (h = 0; h < dim.axis2; h++) {
			for (w = 0; w < dim.axis3; w++) {
				if (*pgolden != *ptarget) {
					add_compare_msg(result, " (%lu %lu %lu %u %u)",
						c, h, w, *pgolden, *ptarget);
					diff_cnt++;
				} else {
					match_cnt++;
				}
				pgolden++;
				ptarget++;
			}
		}
	}
	return diff_cnt;
}

#define ROUNDUP_FACTOR(N, B) ((((N) + (B)-1)) / (B))
#define ROUNDUP(N, B) (ROUNDUP_FACTOR((N), (B)) * (B))
#define CELL_W 4
#define CELL_H 4

static u8 atBinFile08(const u8 *addr, struct npu_fmap_dim dim, size_t channel, size_t row, size_t col)
{
	const size_t channelSize = dim.axis2 * dim.axis3;
	const size_t rowSize = dim.axis3;

	const size_t idx = channel * channelSize + row * rowSize + col;

	return *(addr + idx);
}

__attribute__((unused)) static u16 atBinFile16(const u8 *addr, struct npu_fmap_dim dim, size_t channel, size_t row, size_t col)
{
	return 0;
}

static u8 atNPUDump08(const u8 *addr, struct npu_fmap_dim dim, size_t channel, size_t row, size_t col)
{
	const size_t channelIdx = channel;
	const size_t channelStride =
		ROUNDUP(dim.axis3, CELL_W) * ROUNDUP(dim.axis2, CELL_H);

	const size_t rowIdx = row / CELL_H;
	const size_t rowStride = CELL_H * ROUNDUP(dim.axis3, CELL_W);

	const size_t cellIdx = col / CELL_W;
	const size_t cellStride = CELL_W * CELL_H;

	const size_t cellrow = row % CELL_H;
	const size_t cellcol = col % CELL_W;

	size_t offset = 0;
	u8 value = 0;

	offset += channelIdx * channelStride;
	offset += rowIdx * rowStride;
	offset += cellIdx * cellStride;
	offset += (cellrow * CELL_W + cellcol);

	return value = *(addr + offset);
}

__attribute__((unused)) static u16 atNPUDump16(const u8 *addr, struct npu_fmap_dim dim, size_t channel, size_t row, size_t col)
{
	return 0;
}

int reordered_compare(u32 flags, const u8 *golden, const u8 *target, struct npu_fmap_dim dim, struct npu_golden_compare_result *result)
{
	size_t ch, row, col;

	int diff_cnt = 0;
	int pass_cnt = 0;

	for (ch = 0; ch < dim.axis1; ch++) {
		for (row = 0; row < dim.axis2; row++) {
			for (col = 0; col < dim.axis3; col++) {
				const u8 golden_value = atBinFile08(golden, dim, ch, row, col);
				const u8 target_value = atNPUDump08(target, dim, ch, row, col);

				/*
				if(golden_value != target_value) {
					diff_cnt++;
				}
				*/

				if (golden_value != target_value) {
					add_compare_msg(result, " (%lu %lu %lu %u %u)",
						ch, row, col, golden_value, target_value);
					diff_cnt++;
				} else
					pass_cnt++;

			}
		}
	}
	return diff_cnt;
}

/* Return 1 if check was success, 0 if failed */
int is_av_valid_simple(const u32 golden_size, const u32 target_size, struct npu_golden_compare_result *result)
{
	BUG_ON(!result);

	if (golden_size > target_size) {
		add_compare_msg(result, "Size mismatch - Golden(%uB) / Target(%uB):Golden size should be smaller or equal to target. ",
			golden_size, target_size);
		return 0;
	}
	return 1;
}

int is_av_valid_reordered(const u32 golden_size, const u32 target_size, struct npu_golden_compare_result *result)
{
	BUG_ON(!result);

	if (golden_size > target_size) {
		add_compare_msg(result, "Size mismatch - Golden(%uB) / Target(%uB):Golden size should be smaller or equal to target. ",
			golden_size, target_size);
		return 0;
	}
	return 1;
}
