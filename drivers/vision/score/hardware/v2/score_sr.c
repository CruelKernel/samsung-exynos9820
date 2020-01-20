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
#include "score_system.h"
#include "score_sr.h"

int score_sr_open(struct score_sr *sr)
{
	score_enter();
	score_leave();
	return 0;
}

int score_sr_close(struct score_sr *sr)
{
	score_enter();
	score_leave();
	return 0;
}

int score_sr_probe(struct score_system *system)
{
	score_enter();
	score_leave();
	return 0;
}

int score_sr_remove(struct score_sr *sr)
{
	score_enter();
	score_leave();
	return 0;
}
