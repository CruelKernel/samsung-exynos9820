/*
 * Samsung Exynos SoC series SCore driver
 *
 * Module for controlling semaphore register of SCore
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_SR_H__
#define __SCORE_SR_H__

struct score_system;

/**
 * struct score_sr
 * TODO this will be added if semaphore of score is needed
 */
struct score_sr {
	int temp;
};

int score_sr_open(struct score_sr *sr);
int score_sr_close(struct score_sr *sr);

int score_sr_probe(struct score_system *system);
int score_sr_remove(struct score_sr *sr);

#endif
