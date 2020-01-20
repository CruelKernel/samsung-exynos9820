/*
 * Samsung Exynos SoC series SCore driver
 *
 * Module for controlling clock of SCore
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_PROFILER_H__
#define __SCORE_PROFILER_H__

#include "score_device.h"
#include "score_mem_table.h"

void score_profiler_init(void);
int score_profiler_open(void);
void score_profiler_close(void);

int score_profiler_probe(struct score_device *device,
		const struct attribute_group *grp);
void score_profiler_remove(void);

#endif
