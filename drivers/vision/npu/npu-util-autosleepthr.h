/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _NPU_UTIL_AUTOSLEEPTHR_H_
#define _NPU_UTIL_AUTOSLEEPTHR_H_

#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/atomic.h>

#define PRINT_NAME_LEN			64

/* TODO: Need to be extended */
struct auto_sleep_thread_param {
	void *data;
};

typedef enum {
	THREAD_STATE_NOT_INITIALIZED = 0,
	THREAD_STATE_INITIALIZED,
	THREAD_STATE_RUNNING,
	THREAD_STATE_SLEEPING,
	THREAD_STATE_TERMINATED,
	THREAD_STATE_INVALID
} auto_sleep_thread_state_e;

struct auto_sleep_thread {
	struct task_struct *thread_ref;
	wait_queue_head_t	wq;
	char	name[PRINT_NAME_LEN + 4];

	int (*do_task)(struct auto_sleep_thread_param *data);
	int (*check_work)(struct auto_sleep_thread_param *data);
	void (*on_idle)(struct auto_sleep_thread_param *data, const s64 idle_duration_ns);

	struct auto_sleep_thread_param	task_param;
	int	no_activity_threshold;
	s64	idle_start_ns;

	atomic_t	thr_state;	/* enum auto_sleep_thread_state_e */
};


/* Prototypes */
int auto_sleep_thread_create(struct auto_sleep_thread *newthr, const char *print_name,
	int (*do_task)(struct auto_sleep_thread_param *data),
	int (*check_work)(struct auto_sleep_thread_param *data),
	void (*on_idle)(struct auto_sleep_thread_param *data, s64 idle_duration_ns));
int auto_sleep_thread_start(struct auto_sleep_thread *thr, struct auto_sleep_thread_param param);
int auto_sleep_thread_terminate(struct auto_sleep_thread *thr);
void auto_sleep_thread_signal(struct auto_sleep_thread *thr);


#endif /* _NPU_UTIL_AUTOSLEEPTHR_H_ */

