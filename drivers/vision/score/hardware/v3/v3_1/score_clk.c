/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "score_log.h"
#include "score_firmware.h"
#include "score_device.h"
#include "score_clk.h"

int score_clk_set(struct score_clk *clk, unsigned long freq)
{
	return 0;
}

unsigned long score_clk_get(struct score_clk *clk, int core_id)
{
	unsigned long freq = 0;

	score_enter();
	if (core_id == SCORE_TS) {
		if (!IS_ERR(clk->clkm))
			freq = clk_get_rate(clk->clkm);
		else
			score_err("[dspm] clk is ERROR or NULL\n");
	} else if (core_id == SCORE_BARON1 || core_id == SCORE_BARON2) {
		if (!IS_ERR(clk->clks))
			freq = clk_get_rate(clk->clks);
		else
			score_err("[dsps] clk is ERROR or NULL\n");
	} else {
		score_err("clk_get is fail (core_id is invalid)\n");
	}

	score_leave();
	return freq;
}

static int __score_clk_on(struct score_clk *clk)
{
	int ret = 0;

	score_enter();
	if (!IS_ERR(clk->clkm)) {
		ret = clk_prepare_enable(clk->clkm);
		if (ret) {
			score_err("[dspm] clk_prepare_enable of is fail(%d)\n",
					ret);
			goto exit;
		}
	} else {
		ret = PTR_ERR(clk->clkm);
		score_err("[dspm] clk is ERROR or NULL(%d)\n", ret);
		goto exit;
	}

	if (!IS_ERR(clk->clks)) {
		ret = clk_prepare_enable(clk->clks);
		if (ret) {
			score_err("[dsps] clk_prepare_enable of is fail(%d)\n",
					ret);
			goto exit;
		}
	} else {
		ret = PTR_ERR(clk->clks);
		score_err("[dsps] clk is ERROR or NULL(%d)\n", ret);
		goto exit;
	}
	score_leave();
exit:
	return ret;
}

static void __score_clk_off(struct score_clk *clk)
{
	score_enter();
	if (!IS_ERR(clk->clks))
		clk_disable_unprepare(clk->clks);
	else
		score_err("[dsps] clk is ERROR or NULL\n");

	if (!IS_ERR(clk->clkm))
		clk_disable_unprepare(clk->clkm);
	else
		score_err("[dspm] clk is ERROR or NULL\n");

	score_leave();
}

int score_clk_open(struct score_clk *clk)
{
	int ret = 0;

	score_enter();
	ret = __score_clk_on(clk);
	score_leave();
	return ret;
}

void score_clk_close(struct score_clk *clk)
{
	score_enter();
	__score_clk_off(clk);
	score_leave();
}

int score_clk_probe(struct score_device *device)
{
	int ret = 0;
	struct score_clk *clk;

	score_enter();
	clk = &device->clk;

	clk->dev = device->dev;
	clk->clkm = devm_clk_get(clk->dev, "dspm");
	if (IS_ERR(clk->clkm)) {
		score_err("Failed(%ld) to get dspm clock\n",
				PTR_ERR(clk->clkm));
		ret = PTR_ERR(clk->clkm);
		goto exit;
	}
	clk->clks = devm_clk_get(clk->dev, "dsps");
	if (IS_ERR(clk->clks)) {
		score_err("Failed(%ld) to get dsps clock\n",
				PTR_ERR(clk->clks));
		ret = PTR_ERR(clk->clks);
	}
exit:
	score_leave();
	return ret;
}

void score_clk_remove(struct score_clk *clk)
{
	score_enter();
	score_leave();
}
