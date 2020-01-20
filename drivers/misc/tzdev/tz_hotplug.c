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

#include <linux/completion.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/kthread.h>
#include <linux/seqlock.h>

#include "sysdep.h"
#include "tz_hotplug.h"
#include "tz_iwservice.h"
#include "tzdev.h"

static DECLARE_COMPLETION(tz_hotplug_comp);

static cpumask_t nwd_cpu_mask;
static unsigned int tzdev_hotplug_run;
static DEFINE_SEQLOCK(tzdev_hotplug_lock);

static int tz_hotplug_callback(struct notifier_block *nfb,
			unsigned long action, void *hcpu);

static struct notifier_block tz_hotplug_notifier = {
	.notifier_call = tz_hotplug_callback,
};

void tz_hotplug_notify_swd_cpu_mask_update(void)
{
	complete(&tz_hotplug_comp);
}

static void tz_hotplug_cpus_up(cpumask_t mask)
{
	int cpu;

	for_each_cpu_mask(cpu, mask)
		if (!cpu_online(cpu)) {
			tzdev_print(0, "tzdev: enable cpu%d\n", cpu);
			cpu_up(cpu);
		}
}

void tz_hotplug_update_nwd_cpu_mask(unsigned long new_mask)
{
	cpumask_t new_cpus_mask;
	unsigned long new_mask_tmp = new_mask;

	write_seqlock(&tzdev_hotplug_lock);

	cpumask_copy(&nwd_cpu_mask, to_cpumask(&new_mask_tmp));

	write_sequnlock(&tzdev_hotplug_lock);

	cpumask_andnot(&new_cpus_mask, to_cpumask(&new_mask_tmp), cpu_online_mask);

	tz_hotplug_cpus_up(new_cpus_mask);
}

static int tz_hotplug_cpu(void *data)
{
	int ret, seq;
	cpumask_t new_cpu_mask;
	cpumask_t nwd_cpu_mask_local;
	unsigned long sk_cpu_mask;

	while (tzdev_hotplug_run) {
		ret = wait_for_completion_interruptible(&tz_hotplug_comp);

		do {
			seq = read_seqbegin(&tzdev_hotplug_lock);
			cpumask_copy(&nwd_cpu_mask_local, &nwd_cpu_mask);
		} while (read_seqretry(&tzdev_hotplug_lock, seq));

		sk_cpu_mask = tz_iwservice_get_cpu_mask();
		cpumask_or(&new_cpu_mask, to_cpumask(&sk_cpu_mask), &nwd_cpu_mask_local);

		tz_hotplug_cpus_up(new_cpu_mask);
	}

	return 0;
}

static int tz_hotplug_callback(struct notifier_block *nfb,
			unsigned long action, void *hcpu)
{
	int seq, set;
	unsigned long sk_cpu_mask;
	cpumask_t swd_cpu_mask;

	if (action == CPU_DOWN_PREPARE) {
		do {
			sk_cpu_mask = tz_iwservice_get_cpu_mask();
			cpumask_copy(&swd_cpu_mask, to_cpumask(&sk_cpu_mask));
			seq = read_seqbegin(&tzdev_hotplug_lock);
			set = cpu_isset((unsigned long)hcpu, swd_cpu_mask);
			set |= cpu_isset((unsigned long)hcpu, nwd_cpu_mask);
		} while (read_seqretry(&tzdev_hotplug_lock, seq));

		if (set) {
			tzdev_print(0, "deny cpu%ld shutdown\n", (unsigned long) hcpu);
			return NOTIFY_BAD;
		}
	}

	return NOTIFY_OK;
}

static int tz_hotplug_cpu_down_callback(unsigned int cpu)
{
	void *hcpu = (void *)(long)cpu;

	return tz_hotplug_callback(&tz_hotplug_notifier, CPU_DOWN_PREPARE, hcpu);
}

int tz_hotplug_init(void)
{
	void *th;

	sysdep_register_cpu_notifier(&tz_hotplug_notifier,
		NULL,
		tz_hotplug_cpu_down_callback);

	tzdev_hotplug_run = 1;
	th = kthread_run(tz_hotplug_cpu, NULL, "tzdev_hotplug");
	if (IS_ERR(th)) {
		printk("Can't start tzdev_cpu_hotplug thread\n");
		return PTR_ERR(th);
	}
	return 0;
}

void tz_hotplug_exit(void)
{
	tzdev_hotplug_run = 0;
	complete(&tz_hotplug_comp);

	sysdep_unregister_cpu_notifier(&tz_hotplug_notifier);
}
