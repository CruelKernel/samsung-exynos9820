/*
 * Samsung Exynos SoC series SCore driver
 *
 * Utility for additional function
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_UTIL_H__
#define __SCORE_UTIL_H__

#include <linux/bitmap.h>
#include <linux/ktime.h>

/**
 * The number of time measured for performance measure
 * value can be changed as many as user
 */
#define SCORE_TIME_POINT_NUM		(4)

/**
 * enum score_time_measure_point - Point to measure time
 * @SCORE_TIME_SCQ_WRITE: time writing parameter at scq (target start time)
 * @SCORE_TIME_ISR: time that isr is called by scq (target end time)
 */
enum score_time_mesure_point {
	SCORE_TIME_SCQ_WRITE,
	SCORE_TIME_ISR
};

void score_util_bitmap_init(unsigned long *bitmap, unsigned int nbits);
void score_util_bitmap_clear_bit(unsigned long *bitmap, int bit);
int score_util_bitmap_full(unsigned long *bitmap, unsigned int nbits);
int score_util_bitmap_empty(unsigned long *bitmap, unsigned int nbits);
unsigned int score_util_bitmap_get_zero_bit(unsigned long *bitmap,
		unsigned int nbits);

struct timespec score_util_get_timespec(void);
struct timespec score_util_diff_timespec(struct timespec start,
		struct timespec end);
void score_util_print_diff_timespec(const char *prefix_str,
		struct timespec start, struct timespec end);

#endif
