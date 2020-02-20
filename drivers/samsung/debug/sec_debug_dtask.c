/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/sec_debug.h>

/* from kernel/locking/mutex.c */
#define MUTEX_FLAGS		0x07
static inline struct task_struct *__owner_task(unsigned long owner)
{
	return (struct task_struct *)(owner & ~MUTEX_FLAGS);
}

static void sec_debug_print_mutex_info(struct task_struct *task, struct sec_debug_wait *winfo, bool raw)
{
	struct mutex *wmutex = (struct mutex *)winfo->data;
	struct task_struct *owner_task = __owner_task(atomic_long_read(&wmutex->owner));
	
	pr_info("Mutex: %pS", wmutex);
	if (owner_task) {
		if (raw)
			pr_cont(": owner[0x%lx %s :%d]", owner_task, owner_task->comm, owner_task->pid);
		else
			pr_cont(": owner[%s :%d]", owner_task->comm, owner_task->pid);
	}
	pr_cont("\n");
}

void sec_debug_wtsk_print_info(struct task_struct *task, bool raw)
{
	struct sec_debug_wait winfo;

	winfo.type = task->ssdbg_wait.type;
	winfo.data = task->ssdbg_wait.data;

	if (!(winfo.type && winfo.data))
		return;

	switch (winfo.type) {
	case DTYPE_MUTEX:
		sec_debug_print_mutex_info(task, &winfo, raw);
		break;
	default:
		/* unknown type */
		break;
	}
}

void sec_debug_wtsk_set_data(int type, void *data)
{
	struct sec_debug_wait *cur_wait = &current->ssdbg_wait;
	
	cur_wait->type = type;
	cur_wait->data = data;
}

