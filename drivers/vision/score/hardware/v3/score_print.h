/*
 * Samsung Exynos SoC series SCore driver
 *
 * Print manager module for debugging of SCore
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_PRINT_H__
#define __SCORE_PRINT_H__

struct score_system;

/* Maximum size of buffer that can be printed once by score_printf */
#define MAX_PRINT_BUF_SIZE		(128)
/* Time Period of checking print buffer */
#define SCORE_PRINT_TIME_STEP		(HZ/100)

enum score_print_timer_status {
	SCORE_PRINT_TIMER_DISABLE,
	SCORE_PRINT_TIMER_ENABLE,
	SCORE_PRINT_TIMER_ADDED,
};

/**
 * struct score_print - struct for managing printf from score
 * @kva : Device virtual address of printf buffer
 * @size : Max rear of printf buffer
 * @timer : Timer information
 * @timer_en : Timer enable flag
 * @init : flag whether inited(1) or not(0)
 */
struct score_print {
	struct score_system		*system;
	void __iomem			*sfr;
	unsigned char			*kva;
	int				size;
	struct timer_list		timer;
	int				timer_en;
	int				init;
};

bool score_print_buf_full(struct score_print *print);
bool score_print_buf_empty(struct score_print *print);
void score_print_flush(struct score_print *print);

void score_print_init(struct score_print *print);
int score_print_open(struct score_print *print);
void score_print_close(struct score_print *print);

int score_print_probe(struct score_system *system);
void score_print_remove(struct score_print *print);

#endif
