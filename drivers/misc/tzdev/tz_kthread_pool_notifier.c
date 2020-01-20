/*
 * Copyright (C) 2012-2017, Samsung Electronics Co., Ltd.
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
#include <linux/version.h>

#include "sysdep.h"
#include "tzdev.h"
#include "tzlog.h"
#include "tz_iwservice.h"
#include "tz_mem.h"

enum {
	KTHREAD_SHOULD_SLEEP,
	KTHREAD_SHOULD_RUN,
	KTHREAD_SHOULD_STOP,
};

static atomic_t tz_kthread_pool_fini_done = ATOMIC_INIT(0);

static DEFINE_PER_CPU(struct task_struct *, worker);
static DECLARE_WAIT_QUEUE_HEAD(tz_cmd_waitqueue);
static atomic_t tz_nr_cmds = ATOMIC_INIT(0);

static int tz_kthread_pool_cpu_callback(struct notifier_block *nfb, unsigned long action, void *hcpu);

static struct notifier_block tz_cpu_notifier = {
	.notifier_call = tz_kthread_pool_cpu_callback,
};

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

static int tz_worker_handler(void *arg)
{
	unsigned long cpu = (unsigned long)arg;

	tzdev_kthread_info("Kthread on CPU %lu is alive.\n", cpu);

	while (!kthread_should_stop()) {
		int ret;

		ret = tz_kthread_pool_wait_for_event(cpu);

		switch (ret) {
		case KTHREAD_SHOULD_STOP:
			goto out;
		case KTHREAD_SHOULD_RUN:
			if (!kthread_should_stop())
				WARN_ON(cpu != raw_smp_processor_id());
			else
				tzdev_kthread_info("Kthread on CPU %lu should stop but already got command\n", cpu);

			tzdev_kthread_debug("Enter SWd from kthread on cpu = %lu\n", cpu);
			tzdev_smc_schedule();
			tzdev_kthread_debug("Exit SWd from kthread on cpu = %lu\n", cpu);
			break;
		default:
			BUG();
		}
	}
out:
	tzdev_kthread_info("Kthread on CPU %lu is stopped.\n", cpu);

	return 0;
}

static int tz_kthread_pool_cpu_callback(struct notifier_block *nfb, unsigned long action, void *hcpu)
{
	struct task_struct *t = NULL;
	unsigned long cpu = (unsigned long)(hcpu);

#ifndef CONFIG_TZDEV_SK_MULTICORE
	if (cpu >= NR_SW_CPU_IDS)
		return NOTIFY_DONE;
#endif /* CONFIG_TZDEV_SK_MULTICORE */

	switch (action) {
	case CPU_UP_PREPARE:
	case CPU_UP_PREPARE_FROZEN:
		t = kthread_create_on_node(tz_worker_handler,
				hcpu, cpu_to_node(cpu),
				"worker thread/%ld", cpu);
		if (IS_ERR(t)) {
			tzdev_kthread_error("Failed to create worker thread on cpu %ld, error %ld\n",
					cpu, PTR_ERR(t));
			return notifier_from_errno(PTR_ERR(t));
		}
		kthread_bind(t, cpu);
		per_cpu(worker, cpu) = t;
		tzdev_kthread_debug("Created kthread on cpu = %lu\n", cpu);
		break;
	case CPU_ONLINE:
	case CPU_ONLINE_FROZEN:
		wake_up_process(per_cpu(worker, cpu));
		break;
#ifdef CONFIG_HOTPLUG_CPU
	case CPU_UP_CANCELED:
	case CPU_UP_CANCELED_FROZEN:
		kthread_bind(per_cpu(worker, cpu), cpumask_any(cpu_online_mask));
		tzdev_kthread_debug("Unbinded kthread from cpu = %lu\n", cpu);
		/* fall through */
	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		if (!per_cpu(worker, cpu))
			break;
		kthread_stop(per_cpu(worker, cpu));
		per_cpu(worker, cpu) = NULL;
		tzdev_kthread_debug("Request to stop kthread on cpu = %lu\n", cpu);
		break;
#endif /* CONFIG_HOTPLUG_CPU */
	}

	return NOTIFY_OK;
}

static int tz_kthread_pool_cpu_up_callback(unsigned int cpu)
{
	int err;
	void *hcpu = (void *)(long)cpu;

	err = tz_kthread_pool_cpu_callback(&tz_cpu_notifier, CPU_UP_PREPARE, hcpu);
	BUG_ON(err != NOTIFY_OK && err != NOTIFY_DONE);

	tz_kthread_pool_cpu_callback(&tz_cpu_notifier, CPU_ONLINE, hcpu);

	return 0;
}

static int tz_kthread_pool_cpu_down_callback(unsigned int cpu)
{
	void *hcpu = (void *)(long)cpu;

	tz_kthread_pool_cpu_callback(&tz_cpu_notifier, CPU_UP_CANCELED, hcpu);

	return 0;
}

static __init int tz_kthread_pool_init(void)
{
	int err;
	void *cpu = (void *)(long)smp_processor_id();

	err = tz_kthread_pool_cpu_callback(&tz_cpu_notifier, CPU_UP_PREPARE, cpu);
	BUG_ON(err != NOTIFY_OK);

	tz_kthread_pool_cpu_callback(&tz_cpu_notifier, CPU_ONLINE, cpu);

	sysdep_register_cpu_notifier(&tz_cpu_notifier,
		tz_kthread_pool_cpu_up_callback,
		tz_kthread_pool_cpu_down_callback);

	return 0;
}

early_initcall(tz_kthread_pool_init);

void tz_kthread_pool_fini(void)
{
	unsigned int cpu;
	struct task_struct *t;

	if (atomic_cmpxchg(&tz_kthread_pool_fini_done, 0, 1))
		return;

	sysdep_unregister_cpu_notifier(&tz_cpu_notifier);

	for_each_possible_cpu(cpu) {
		t = per_cpu(worker, cpu);
		if (t) {
			per_cpu(worker, cpu) = NULL;
			kthread_stop(t);
		}
	}
}

void tz_kthread_pool_cmd_send(void)
{
	atomic_inc(&tz_nr_cmds);
	tz_kthread_pool_wake_up_all();
}

void tz_kthread_pool_enter_swd(void)
{
#ifdef CONFIG_TZDEV_SK_MULTICORE
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

#else /* CONFIG_TZDEV_SK_MULTICORE */
	tz_kthread_pool_cmd_send();
#endif /* CONFIG_TZDEV_SK_MULTICORE */
}
