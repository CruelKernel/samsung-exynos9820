/*
 * Samsung Exynos SoC series SCore driver
 *
 * Module for using shared memory in SCore
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_SM_H__
#define __SCORE_SM_H__

struct score_system;

/**
 * struct score_sm
 * TODO this will be added if shared memory is needed
 */
struct score_sm {
	int temp;
};

int score_sm_open(struct score_sm *sm);
int score_sm_close(struct score_sm *sm);

int score_sm_probe(struct score_system *system);
int score_sm_remove(struct score_sm *sm);

#endif

