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
#include "score_device.h"
#include "score_clk.h"

int score_clk_set(struct score_clk *clk, unsigned long freq)
{
	score_enter();
	score_leave();
	return 0;
}

unsigned long score_clk_get(struct score_clk *clk, int core_id)
{
	score_enter();
	score_leave();
	return 0;
}

int score_clk_open(struct score_clk *clk)
{
	score_enter();
	score_leave();
	return 0;
}

void score_clk_close(struct score_clk *clk)
{
	score_enter();
	score_leave();
}

int score_clk_probe(struct score_device *device)
{
	score_enter();
	score_leave();
	return 0;
}

void score_clk_remove(struct score_clk *clk)
{
	score_enter();
	score_leave();
}
