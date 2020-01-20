/*
 * Copyright (C) 2012-2018, Samsung Electronics Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/atomic.h>
#include <linux/completion.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/percpu.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <linux/smpboot.h>
#include <linux/version.h>

#include "sysdep.h"
#include "tzdev.h"
#include "tzlog.h"
#include "tz_iwservice.h"
#include "tz_kthread_pool.h"
#include "tz_mem.h"

enum {
	KTHREAD_SHOULD_SLEEP,
	KTHREAD_SHOULD_RUN,
	KTHREAD_SHOULD_STOP,
	KTHREAD_SHOULD_PARK,
};

static atomic_t tz_kthread_pool_fini_done = ATOMIC_INIT(0);

static DEFINE_PER_CPU(struct task_struct *, worker);
static DECLARE_WAIT_QUEUE_HEAD(tz_cmd_waitqueue);
static atomic_t tz_nr_cmds = ATOMIC_INIT(0);

static int tz_kthread_pool_cmd_get(void)
{
	return atomic_dec_if_positive(&tz_nr_cmds) >= 0;
}

static int tz_kthread_pool_should_wake(unsigned long cpu)
{
	unsigned long sk_cpu_mask;
	cpumask_t cpu_mask;
	cpumask_t outstanding_cpu_mask;
	cpumask_t requested_cpu_mask;

	if (kthread_should_stop()) {
		tzdev_kthread_info("Requested kthread stop on cpu = %lx\n", cpu);
		return KTHREAD_SHOULD_STOP;
	}

	sk_cpu_mask = tz_iwservice_get_cpu_mask();

	cpumask_copy(&requested_cpu_mask, to_cpumask(&sk_cpu_mask));
	cpumask_and(&cpu_mask, &requested_cpu_mask, cpu_online_mask);
	cpumask_andnot(&outstanding_cpu_mask, &requested_cpu_mask, &cpu_mask);

	tzdev_kthread_info("cpu mask iwservice = %lx\n",
			sk_cpu_mask);
	tzdev_kthread_info("cpu mask requested = %*pb\n",
			cpumask_pr_args(&requested_cpu_mask));
	tzdev_kthread_info("cpu mask effective = %*pb\n",
			cpumask_pr_args(&cpu_mask));
	tzdev_kthread_info("cpu mask outstanding = %*pb\n",
			cpumask_pr_args(&outstanding_cpu_mask));

	if (kthread_should_park()) {
		if (cpu_isset(cpu, cpu_mask))
			tz_kthread_pool_cmd_send();

		tzdev_kthread_info("Requested kthread park on cpu = %lx\n", cpu);
		return KTHREAD_SHOULD_PARK;
	}

	if (cpu_isset(cpu, cpu_mask)) {
		tzdev_kthread_debug("Direct cpu hit = %lu\n", cpu);
		tz_kthread_pool_cmd_get();
		return KTHREAD_SHOULD_RUN;
	} else if (!cpumask_empty(&outstanding_cpu_mask)) {
		tzdev_kthread_debug("No proper cpus to satisfy requested affinity\n");
		tz_kthread_pool_cmd_get();
		return KTHREAD_SHOULD_RUN;
	}

	if (tz_kthread_pool_cmd_get()) {
		tzdev_kthread_debug("Handle initial cmd on cpu = %lu\n", cpu);
		return KTHREAD_SHOULD_RUN;
	}

	return KTHREAD_SHOULD_SLEEP;
}

static int tz_kthread_pool_wake_function(wait_queue_t *wait, unsigned int mode, int sync,
				void *key)
{
	unsigned long cpu = (unsigned long)key;

	/*
	 * Avoid a wakeup if enter to SWd should be done from
	 * user thread context
	 */
	if (cpu == raw_smp_processor_id())
		return 0;
	return autoremove_wake_function(wait, mode, sync, key);
}

static int tz_kthread_pool_wait_for_event(unsigned long cpu)
{
	int ret;
	DEFINE_WAIT_FUNC(wait, tz_kthread_pool_wake_function);

	for (;;) {
		prepare_to_wait(&tz_cmd_waitqueue, &wait, TASK_UNINTERRUPTIBLE);
		ret = tz_kthread_pool_should_wake(cpu);
		if (ret != KTHREAD_SHOULD_SLEEP)
			break;
		schedule();
	}
	finish_wait(&tz_cmd_waitqueue, &wait);

	return ret;
}

static void tz_kthread_pool_wake_up_all_but(unsigned long cpu)
{
	__wake_up(&tz_cmd_waitqueue, TASK_NORMAL, 0, (void *)cpu);
}

void tz_kthread_pool_wake_up_all(void)
{
	tz_kthread_pool_wake_up_all_but(NR_CPUS);
}

static void tz_worker_handler(unsigned int cpu)
{
	int ret;

	for (;;) {
		ret = tz_kthread_pool_wait_for_event(cpu);

		switch (ret) {
		case KTHREAD_SHOULD_STOP:
		case KTHREAD_SHOULD_PARK:
			return;
		case KTHREAD_SHOULD_RUN:
			tzdev_kthread_debug("Enter SWd from kthread on cpu = %u\n", cpu);
			tzdev_smc_schedule();
			tzdev_kthread_debug("Exit SWd from kthread on cpu = %u\n", cpu);
			continue;
		default:
			BUG();
		}
	}
}

static int tz_kthread_pool_should_run(unsigned int cpu)
{
	(void) cpu;

	/*
	 * Implementing wait_for_event here is prohibited as
	 * this function called with preemption disabled.
	 *
	 * So, always return 1 here to unconditionally call
	 * for tz_worker_handler where kthread will sleep
	 * on waitqueue waiting for request.
	 */
	return 1;
}

static struct smp_hotplug_thread tz_kthread_pool_smp_hotplug = {
	.store = &worker,
	.thread_should_run = tz_kthread_pool_should_run,
	.thread_fn = tz_worker_handler,
	.thread_comm = "tz_worker_thread/%u",
};

static __init int tz_kthread_pool_init(void)
{
	return smpboot_register_percpu_thread(&tz_kthread_pool_smp_hotplug);
}

early_initcall(tz_kthread_pool_init);

void tz_kthread_pool_fini(void)
{
	if (atomic_cmpxchg(&tz_kthread_pool_fini_done, 0, 1))
		return;

	smpboot_unregister_percpu_thread(&tz_kthread_pool_smp_hotplug);
}

void tz_kthread_pool_cmd_send(void)
{
	atomic_inc(&tz_nr_cmds);
	tz_kthread_pool_wake_up_all();
}

void tz_kthread_pool_enter_swd(void)
{
	unsigned long cpu;
	unsigned long sk_cpu_mask;
	unsigned long sk_user_cpu_mask;
	cpumask_t requested_cpu_mask;
	cpumask_t requested_user_cpu_mask;

	sk_cpu_mask = tz_iwservice_get_cpu_mask();
	smp_rmb();
	sk_user_cpu_mask = tz_iwservice_get_user_cpu_mask();

	cpumask_copy(&requested_cpu_mask, to_cpumask(&sk_cpu_mask));
	cpumask_copy(&requested_user_cpu_mask, to_cpumask(&sk_user_cpu_mask));

	cpu = get_cpu();
	if (!sk_user_cpu_mask || cpu_isset(cpu, requested_user_cpu_mask) ||
			cpu_isset(cpu, requested_cpu_mask)) {
		tz_kthread_pool_wake_up_all_but(cpu);

		tzdev_kthread_debug("Enter SWd directly on cpu = %lu\n", cpu);
		tzdev_smc_schedule();
		tzdev_kthread_debug("Exit SWd directly on cpu = %lu\n", cpu);
	} else {
		tz_kthread_pool_cmd_send();
	}
	put_cpu();
}
