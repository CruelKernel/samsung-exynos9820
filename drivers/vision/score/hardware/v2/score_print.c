/*
 * Samsung Exynos SoC series SCore driver
 *
 * Print manager module for debugging of SCore
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/io.h>
#include <linux/timer.h>
#include <linux/kernel.h>

#include "score_log.h"
#include "score_regs.h"
#include "score_print.h"
#include "score_system.h"
#include "score_mem_table.h"
#include "score_firmware.h"

static inline int __score_print_get_front(struct score_print *print)
{
	return readl(print->sfr + PRINT_QUEUE_FRONT_ADDR);
}

static inline int __score_print_get_rear(struct score_print *print)
{
	return readl(print->sfr + PRINT_QUEUE_REAR_ADDR);
}

static inline void __score_print_set_front(struct score_print *print, int value)
{
	writel(value, print->sfr + PRINT_QUEUE_FRONT_ADDR);
}

static inline void __score_print_set_rear(struct score_print *print, int value)
{
	writel(value, print->sfr + PRINT_QUEUE_REAR_ADDR);
}

static inline void __score_print_set_count(struct score_print *print, int value)
{
	writel(value, print->sfr + PRINT_COUNT);
}

/**
 * score_print_buf_full - check whether print buffer for score is full or
 * not.
 * @print: struct for managing printf from score
 */
bool score_print_buf_full(struct score_print *print)
{
	return ((__score_print_get_rear(print) + 1) % print->size) ==
		__score_print_get_front(print);
}

/**
 * score_print_buf_empty - check whether print buffer for score is empty
 * or not.
 * @print: struct for managing printf from score
 */
bool score_print_buf_empty(struct score_print *print)
{
	return (__score_print_get_rear(print) ==
		__score_print_get_front(print));
}

/**
 * __score_strcpy - copy string from print buffer to kernel memory.
 * @dest: destination address of memory
 * @src: source address of memory
 */
static char *__score_strcpy(char *dest, const unsigned char *src)
{
	char *saved = dest;

	while (*src)
		*dest++ = *src++;

	*dest = '\0';

	return saved;
}

/**
 * __score_print_dequeue - dequeue string from print buffer if it is.
 * @print: struct for managing printf from score
 * @log: space for storing and for printing out string from score
 * @core_id: enum number which indicate a certain core among cores in
 * score
 */
static int __score_print_dequeue(struct score_print *print, char *log)
{
	int front;
	unsigned char *print_buf;

	if (print->init == 0)
		return -EINVAL;
	if (score_print_buf_empty(print))
		return -ENODATA;

	front = (__score_print_get_front(print) + 1) % print->size;
	if (front < 0)
		return -EFAULT;

#if defined(SCORE_EVT1) && !defined(PROFILER_TEST)
	score_system_dcache_control_all(print->system, DCACHE_CLEAN);
#endif
	print_buf = print->kva + (MAX_PRINT_BUF_SIZE * front);

	__score_strcpy(log, (const unsigned char *)print_buf);
	__score_print_set_front(print, front);

	return 0;
}

/**
 * score_print_timer - timer handler function of master core for
 * checking print buffer periodically.
 * @data: struct for managing printf from score
 */
static void score_print_timer(unsigned long data)
{
	struct score_print *print = (struct score_print *)data;
	char log[MAX_PRINT_BUF_SIZE];

	memset(log, '\0', MAX_PRINT_BUF_SIZE);
	while (!__score_print_dequeue(print, log))
		score_info("[timer] %s", log);

	mod_timer(&print->timer, jiffies + SCORE_PRINT_TIME_STEP);
}

/**
 * score_print_flush - print out all contents in print buffer
 * not.
 * @print: struct for managing printf from score
 */
void score_print_flush(struct score_print *print)
{
	char log[MAX_PRINT_BUF_SIZE];

	score_enter();
	if (print->init == 0)
		return;

	while (!__score_print_dequeue(print, log))
		score_info("[flush] %s", log);
	score_leave();
}

void score_print_init(struct score_print *print)
{
	__score_print_set_front(print, 0);
	__score_print_set_rear(print, 0);
	__score_print_set_count(print, 0);
}

/**
 * score_print_buf_init - initiaize score_print struct for each core.
 * @start: starting address of print buffer
 * @size: size of print buffer for one core
 */
static void __score_print_buf_init(struct score_print *print,
		void *start, int size)
{
	/// setting data regard with print buffer for Master core
	print->kva = (unsigned char *)start;
	print->size = (size / MAX_PRINT_BUF_SIZE);

	/// indicate that initializing is done
	print->init = 1;

	/// set timer for checking buffer periodically
	if (print->timer_en == SCORE_PRINT_TIMER_ENABLE) {
		/* can't disable timer, because timer is already added */
		print->timer_en = SCORE_PRINT_TIMER_ADDED;
		add_timer(&print->timer);
	}
}

int score_print_open(struct score_print *print)
{
	struct score_system *system;
	void *score_print_kva;

	score_enter();
	system = print->system;
	score_print_kva = score_mmu_get_print_kvaddr(&system->mmu);
	score_print_init(print);
	__score_print_buf_init(print, score_print_kva, SCORE_PRINT_SIZE);

	score_leave();
	return 0;
}

void score_print_close(struct score_print *print)
{
	score_enter();
	if (print->timer_en == SCORE_PRINT_TIMER_ADDED) {
		del_timer_sync(&print->timer);
		print->timer_en = SCORE_PRINT_TIMER_ENABLE;
	}
	score_print_flush(print);

	print->init = 0;

	score_leave();
}

int score_print_probe(struct score_system *system)
{
	struct score_print *print;

	score_enter();
	print = &system->print;
	print->system = system;

	print->sfr = system->sfr;
	init_timer(&print->timer);
	print->timer.expires = jiffies + SCORE_PRINT_TIME_STEP;
	print->timer.data = (unsigned long)print;
	print->timer.function = score_print_timer;

	/* timer disable because of memory bus error */
	print->timer_en = SCORE_PRINT_TIMER_ENABLE;
	score_leave();
	return 0;
}

void score_print_remove(struct score_print *print)
{
	score_enter();
	score_leave();
}
