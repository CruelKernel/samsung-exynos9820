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
#include "score_firmware.h"
#include "score_device.h"
#include "score_clk.h"

#define MASTER_CLK	"dspm"
#define KNIGHT_CLK	"dsps"

int score_clk_set(struct score_clk *clk, unsigned long freq)
{
	int ret = 0;

#if 0
	/* clk_set is not supported. */
	if (!IS_ERR(clk->clkm)) {
		ret = clk_set_rate(clk->clkm, freq);
		if (ret) {
			score_err("[%s] clk_set_rate(freq:%ld) is fail(%d)\n",
					MASTER_CLK, freq, ret);
			goto exit;
		}
	} else {
		ret = PTR_ERR(clk->clkm);
		score_err("[%s] clk is ERROR or NULL(%d)\n",
				MASTER_CLK, ret);
		goto exit;
	}
	if (!IS_ERR(clk->clks)) {
		ret = clk_set_rate(clk->clks, freq);
		if (ret) {
			score_err("[%s] clk_set_rate(freq:%ld) is fail(%d)\n",
					KNIGHT_CLK, freq, ret);
			goto exit;
		}
	} else {
		ret = PTR_ERR(clk->clks);
		score_err("[%s] clk is ERROR or NULL(%d)\n",
				KNIGHT_CLK, ret);
		goto exit;
	}
exit:
#endif
	return ret;
}

unsigned long score_clk_get(struct score_clk *clk, int core_id)
{
	unsigned long freq = 0;

	score_enter();
	if (core_id == SCORE_MASTER) {
		if (!IS_ERR(clk->clkm))
			freq = clk_get_rate(clk->clkm);
		else
			score_err("[%s] clk is ERROR or NULL\n", MASTER_CLK);
	} else if (core_id == SCORE_KNIGHT1 || core_id == SCORE_KNIGHT2) {
		if (!IS_ERR(clk->clks))
			freq = clk_get_rate(clk->clks);
		else
			score_err("[%s] clk is ERROR or NULL\n", KNIGHT_CLK);
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
			score_err("[%s] clk_prepare_enable of is fail(%d)\n",
					MASTER_CLK, ret);
			goto exit;
		}
	} else {
		ret = PTR_ERR(clk->clkm);
		score_err("[%s] clk is ERROR or NULL(%d)\n",
				MASTER_CLK, ret);
		goto exit;
	}

	if (!IS_ERR(clk->clks)) {
		ret = clk_prepare_enable(clk->clks);
		if (ret) {
			score_err("[%s] clk_prepare_enable of is fail(%d)\n",
					KNIGHT_CLK, ret);
			goto exit;
		}
	} else {
		ret = PTR_ERR(clk->clks);
		score_err("[%s] clk is ERROR or NULL(%d)\n",
				KNIGHT_CLK, ret);
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
		score_err("[%s] clk is ERROR or NULL\n", KNIGHT_CLK);

	if (!IS_ERR(clk->clkm))
		clk_disable_unprepare(clk->clkm);
	else
		score_err("[%s] clk is ERROR or NULL\n", MASTER_CLK);

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
	clk->clkm = devm_clk_get(clk->dev, MASTER_CLK);
	if (IS_ERR(clk->clkm)) {
		score_err("Failed(%ld) to get %s clock\n",
				PTR_ERR(clk->clkm), MASTER_CLK);
		ret = PTR_ERR(clk->clkm);
		goto exit;
	}
	clk->clks = devm_clk_get(clk->dev, KNIGHT_CLK);
	if (IS_ERR(clk->clks)) {
		score_err("Failed(%ld) to get %s clock\n",
				PTR_ERR(clk->clks), KNIGHT_CLK);
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
