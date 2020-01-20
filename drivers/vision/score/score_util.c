/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "score_log.h"
#include "score_util.h"

/**
 * score_util_bitmap_init - Initialize bimap array by 0
 * @bitmap:	[in]	bitmap array
 * @nbits:	[in]	size of bimap array
 */
void score_util_bitmap_init(unsigned long *bitmap, unsigned int nbits)
{
	bitmap_zero(bitmap, nbits);
}

/**
 * score_util_bitmap_clear_bit - Clear one site in bitmap array
 * @bitmap:	[in]	bitmap array
 * @bit:	[in]	site to be cleared
 */
void score_util_bitmap_clear_bit(unsigned long *bitmap, int bit)
{
	clear_bit(bit, bitmap);
}

/**
 * score_util_bitmap_get_zero_bit - Get one site cleared
 * @bitmap:	[in]	bitmap array
 * @nbits:	[in]	size of bitmap array
 *
 * Returns site if bitmap is not full, otherwise nbits
 */
unsigned int score_util_bitmap_get_zero_bit(unsigned long *bitmap,
		unsigned int nbits)
{
	int bit;
	int old_bit;

	while ((bit = find_first_zero_bit(bitmap, nbits)) != nbits) {
		old_bit = test_and_set_bit(bit, bitmap);
		if (!old_bit)
			return bit;
	}

	return nbits;
}

/**
 * score_util_get_timespec - Measure time in the form of struct timespec
 *
 * Returns struct timespec
 */
struct timespec score_util_get_timespec(void)
{
	struct timespec ts;

	getrawmonotonic(&ts);
	return ts;
}

/**
 * score_util_diff_timespec - Calculate difference of time
 * @start:	[in]	start time
 * @end:	[in]	end time
 *
 * Returns difference of time
 */
struct timespec score_util_diff_timespec(struct timespec start,
		struct timespec end)
{
	return timespec_sub(end, start);
}

/**
 * score_util_print_diff_timespec - Print difference of time
 * @prefix_str:		[in]	prefix log
 * @start:		[in]	start time
 * @end:		[in]	end time
 */
void score_util_print_diff_timespec(const char *prefix_str,
		struct timespec start, struct timespec end)
{
	struct timespec diff;

	diff = score_util_diff_timespec(start, end);

	score_info("%s %ld.%09ld sec\n", prefix_str, diff.tv_sec, diff.tv_nsec);
}
