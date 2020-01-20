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

#ifndef __SCORE_EXYNOS_CLK_H__
#define __SCORE_EXYNOS_CLK_H__

#include <linux/device.h>
#include <linux/clk.h>

struct score_device;

/**
 * struct score_clk - Data about clock of SCore
 * @dev: device structure registered in platform
 * @clk: object about clk structure
 * @clk_ops: clock control operations
 */
struct score_clk {
	struct device			*dev;
	struct clk			*clkm;
	struct clk			*clks;
};

unsigned long score_clk_get(struct score_clk *clk, int core_id);
int score_clk_set(struct score_clk *clk, unsigned long freq);

int score_clk_open(struct score_clk *clk);
void score_clk_close(struct score_clk *clk);
int score_clk_probe(struct score_device *device);
void score_clk_remove(struct score_clk *clk);

#endif

