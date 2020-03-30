/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * CPUIDLE driver for exynos 64bit
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/cpuidle.h>
#include <linux/cpu_pm.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/suspend.h>
#include <linux/cpu.h>
#include <linux/reboot.h>
#include <linux/of.h>
#include <linux/psci.h>
#include <linux/cpuidle_profiler.h>
#include <linux/exynos-ucc.h>

#include <asm/tlbflush.h>
#include <asm/cpuidle.h>
#include <asm/topology.h>

#include <soc/samsung/exynos-cpupm.h>
#include <linux/cpufreq.h>

#include "dt_idle_states.h"

#define DEFAULT_FREQVAR_IFACTOR 100	/* No more or less for the given constraints */
unsigned int default_freqvar_ifactor[] = {DEFAULT_FREQVAR_IFACTOR};
EXPORT_SYMBOL_GPL(default_freqvar_ifactor);
unsigned int *ank_freqvar_ifactor = default_freqvar_ifactor;
unsigned int *pmt_freqvar_ifactor = default_freqvar_ifactor;
unsigned int *cht_freqvar_ifactor = default_freqvar_ifactor;

DEFINE_PER_CPU(struct freqvariant_idlefactor, fv_ifactor);
DECLARE_PER_CPU(int, clhead_cpu);

/*
 * Exynos cpuidle driver supports the below idle states
 *
 * IDLE_C1 : WFI(Wait For Interrupt) low-power state
 * IDLE_C2 : Local CPU power gating
 */

/***************************************************************************
 *                           Cpuidle state handler                         *
 ***************************************************************************/
static unsigned int prepare_idle(unsigned int cpu, int index)
{
	unsigned int entry_state = 0;

	if (index > 0) {
		cpu_pm_enter();
		entry_state = exynos_cpu_pm_enter(cpu, index);
	}

	cpuidle_profile_cpu_idle_enter(cpu, index);

	return entry_state;
}

static void post_idle(unsigned int cpu, int index, int fail)
{
	cpuidle_profile_cpu_idle_exit(cpu, index, fail);

	if (!index)
		return;

	exynos_cpu_pm_exit(cpu, fail);
	cpu_pm_exit();
}

static int enter_idle(unsigned int index)
{
	/*
	 * idle state index 0 corresponds to wfi, should never be called
	 * from the cpu_suspend operations
	 */
	if (!index) {
		cpu_do_idle();
		return 0;
	}

	return arm_cpuidle_suspend(index);
}

DECLARE_PER_CPU(struct cpuidle_info, cpuidle_inf);

static int exynos_enter_idle(struct cpuidle_device *dev,
				struct cpuidle_driver *drv, int index)
{
	int i, idx, triggered = 0;
	int entry_state, ret = 0;
	unsigned int bias_target_residency;
	unsigned int bias_exit_latency;
	unsigned int bias_index;
	unsigned long flags;
	struct freqvariant_idlefactor *pfv_factor = &per_cpu(fv_ifactor, dev->cpu);
	struct cpuidle_info *idle_info = this_cpu_ptr(&cpuidle_inf);

	if (idle_info->bUse_GovDecision == 1 || pfv_factor->init != 0xdead)
		goto skip_moce;
	/* Fetch the criteria for idle state without spinlock
	 * with the way of Read-Copy-No update
	 * for lowering the idle transition cost
	 */
	spin_lock_irqsave(&pfv_factor->freqvar_if_lock, flags);
	bias_target_residency = pfv_factor->bias_target_res;
	bias_exit_latency = pfv_factor->bias_exit_lat;
	bias_index = pfv_factor->bias_index;
	spin_unlock_irqrestore(&pfv_factor->freqvar_if_lock, flags);

	/* Re-evaluate the depth of idle in accordance with the freq variation */
	idx = -1;
	for (i = idle_info->bfirst_idx; i < drv->state_count; i++) {
		struct cpuidle_state *s = &drv->states[i];
		struct cpuidle_state_usage *su = &dev->states_usage[i];

		if (s->disabled || su->disable)
			continue;

		if (idx == -1)
			idx = i; /* first enabled state */

		/* Future work:
		 * If the different biasing timing is applied onto plural idle states, pfv_factor also must have
		 * the different value for each idle state.
		 */
		if (bias_index & 0x1 << i) {
			if (bias_target_residency > idle_info->predicted_us|| bias_exit_latency > idle_info->latency_req)
				break;

			triggered = 1;

		} else if (triggered) {
			if (s->target_residency > idle_info->predicted_us || s->exit_latency > idle_info->latency_req)
				break;
		}

		idx = i;
	}

	/* Update the index /w MOCE */
	if (idx != -1 && index != idx) {
		index = idx;
	}

skip_moce:

	entry_state = prepare_idle(dev->cpu, index);

	ret = enter_idle(entry_state);

	post_idle(dev->cpu, index, ret);

	/*
	 * If cpu fail to enter idle, it should not update state usage
	 * count. Driver have to return an error value to
	 * cpuidle_enter_state().
	 */
	if (ret < 0)
		return ret;
	else
		return index;
}


/***************************************************************************
 *                            Define notifier call                         *
 ***************************************************************************/
static int exynos_cpuidle_reboot_notifier(struct notifier_block *this,
				unsigned long event, void *_cmd)
{
	switch (event) {
	case SYSTEM_POWER_OFF:
	case SYS_RESTART:
		cpuidle_pause();
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block exynos_cpuidle_reboot_nb = {
	.notifier_call = exynos_cpuidle_reboot_notifier,
};

/*
 * cpufreq trans call back function :
 * freq variant value is kept in only pfv_factor of representing core
 * For example, CPU#0 for CPU#0~3, CPU#4 for CPU#4~5
 * CPU#6 for CPU#6~7
 */
static int exynos_cpuidle_cpufreq_trans_notifier(struct notifier_block *nb,
				unsigned long val, void *data)
{
	struct cpufreq_freqs *freq = data;
	struct freqvariant_idlefactor *pfv_factor;
	unsigned int b_target_res, b_exit_lat, b_cur_freqvar_if;
	int leader_cpu, cpu, i;
	unsigned long flags;

	if (freq->flags & CPUFREQ_CONST_LOOPS)
		return NOTIFY_OK;

	if (val != CPUFREQ_POSTCHANGE)
		return NOTIFY_OK;

	leader_cpu = per_cpu(clhead_cpu, freq->cpu);
	pfv_factor = &per_cpu(fv_ifactor, leader_cpu);

	if (pfv_factor->init != 0xdead)
		return NOTIFY_OK;

	spin_lock_irqsave(&pfv_factor->freqvar_if_lock, flags);

	for (i = 0 ; i < pfv_factor->nfreqvar_ifs - 1 &&
		freq->new >= pfv_factor->freqvar_ifs[i+1]; i += 2)
		;

	pfv_factor->cur_freqvar_if = pfv_factor->freqvar_ifs[i];

	b_cur_freqvar_if = pfv_factor->cur_freqvar_if;
	b_target_res = (pfv_factor->orig_target_res * pfv_factor->cur_freqvar_if) / 100;
	b_exit_lat = (pfv_factor->orig_exit_lat * pfv_factor->cur_freqvar_if) / 100;

	if (b_target_res != pfv_factor->bias_target_res ||
		b_exit_lat != pfv_factor->bias_exit_lat) {
		pfv_factor->bias_target_res = b_target_res;
		pfv_factor->bias_exit_lat = b_exit_lat;

		spin_unlock_irqrestore(&pfv_factor->freqvar_if_lock, flags);

		for_each_possible_cpu(cpu) {
			if (per_cpu(clhead_cpu, cpu) == leader_cpu) {
				pfv_factor = &per_cpu(fv_ifactor, cpu);

				spin_lock_irqsave(&pfv_factor->freqvar_if_lock, flags);
				pfv_factor->cur_freqvar_if = b_cur_freqvar_if;
				pfv_factor->bias_target_res = b_target_res;
				pfv_factor->bias_exit_lat = b_exit_lat;
				spin_unlock_irqrestore(&pfv_factor->freqvar_if_lock, flags);
			}
		}
	} else {
		spin_unlock_irqrestore(&pfv_factor->freqvar_if_lock, flags);
	}

	return NOTIFY_OK;
}

static struct notifier_block exynos_cpufreq_trans_nb = {
	.notifier_call = exynos_cpuidle_cpufreq_trans_notifier,
};
/***************************************************************************
 *                         Initialize cpuidle driver                       *
 ***************************************************************************/
#define exynos_idle_wfi_state(state)					\
	do {								\
		state.enter = exynos_enter_idle;			\
		state.exit_latency = 1;					\
		state.target_residency = 1;				\
		state.power_usage = UINT_MAX;				\
		strncpy(state.name, "WFI", CPUIDLE_NAME_LEN - 1);	\
		strncpy(state.desc, "c1", CPUIDLE_DESC_LEN - 1);	\
	} while (0)

static struct cpuidle_driver exynos_idle_driver[NR_CPUS];

static const struct of_device_id exynos_idle_state_match[] __initconst = {
	{ .compatible = "exynos,idle-state",
	  .data = exynos_enter_idle },
	{ },
};

static int __init exynos_idle_driver_init(struct cpuidle_driver *drv,
					   struct cpumask *cpumask)
{
	int cpu = cpumask_first(cpumask);

	drv->name = kzalloc(sizeof("exynos_idleX"), GFP_KERNEL);
	if (!drv->name)
		return -ENOMEM;

	scnprintf((char *)drv->name, 12, "exynos_idle%d", cpu);
	drv->owner = THIS_MODULE;
	drv->cpumask = cpumask;
	exynos_idle_wfi_state(drv->states[0]);

	return 0;
}

static int __init exynos_idle_init(void)
{
	struct freqvariant_idlefactor *pfv_factor;
	int ret, cpu, i;

	for_each_possible_cpu(cpu) {
		ret = exynos_idle_driver_init(&exynos_idle_driver[cpu],
					      topology_sibling_cpumask(cpu));

		if (ret) {
			pr_err("failed to initialize cpuidle driver for cpu%d",
					cpu);
			goto out_unregister;
		}

		/*
		 * Initialize idle states data, starting at index 1.
		 * This driver is DT only, if no DT idle states are detected
		 * (ret == 0) let the driver initialization fail accordingly
		 * since there is no reason to initialize the idle driver
		 * if only wfi is supported.
		 */
		ret = dt_init_idle_driver(&exynos_idle_driver[cpu],
					exynos_idle_state_match, 1);
		if (ret < 0) {
			pr_err("failed to initialize idle state for cpu%d\n", cpu);
			goto out_unregister;
		}

		/*
		 * Call arch CPU operations in order to initialize
		 * idle states suspend back-end specific data
		 */
		ret = arm_cpuidle_init(cpu);
		if (ret) {
			pr_err("failed to initialize idle operation for cpu%d\n", cpu);
			goto out_unregister;
		}

		ret = cpuidle_register(&exynos_idle_driver[cpu], NULL);
		if (ret) {
			pr_err("failed to register cpuidle for cpu%d\n", cpu);
			goto out_unregister;
		}

		pfv_factor = &per_cpu(fv_ifactor, cpu);

		/* HACK */
		switch (cpu) {
		case 0 ... 3:
			pfv_factor->freqvar_ifs = ank_freqvar_ifactor;
			pfv_factor->nfreqvar_ifs = 1;
			pfv_factor->cur_freqvar_if = 100;
			break;
		case 4 ... 5:
			pfv_factor->freqvar_ifs = pmt_freqvar_ifactor;
			pfv_factor->nfreqvar_ifs = 1;
			pfv_factor->cur_freqvar_if = 100;
			break;
		case 6 ... 7:
			pfv_factor->freqvar_ifs = cht_freqvar_ifactor;
			pfv_factor->nfreqvar_ifs = 1;
			pfv_factor->cur_freqvar_if = 100;
			break;
		}
		/* MOCE will concern only C2. (CPD will take care later) */
		pfv_factor->orig_target_res = exynos_idle_driver[cpu].states[1].target_residency;
		pfv_factor->orig_exit_lat = exynos_idle_driver[cpu].states[1].exit_latency;
		pfv_factor->bias_target_res= exynos_idle_driver[cpu].states[1].target_residency;
		pfv_factor->bias_exit_lat= exynos_idle_driver[cpu].states[1].exit_latency;
		spin_lock_init(&pfv_factor->freqvar_if_lock);
		pfv_factor->bias_index = 0x2;	// We only focusing on state index 1 (C2)
		pfv_factor->init = 0xdead;
	}

	cpuidle_profile_cpu_idle_register(&exynos_idle_driver[0]);

	register_reboot_notifier(&exynos_cpuidle_reboot_nb);

	cpufreq_register_notifier(&exynos_cpufreq_trans_nb,
					CPUFREQ_TRANSITION_NOTIFIER);

	init_exynos_ucc();

	pr_info("Exynos cpuidle driver Initialized\n");

	return 0;

out_unregister:
	for (i = 0; i <= cpu; i++) {
		kfree(exynos_idle_driver[i].name);

		/*
		 * Cpuidle driver of variable "cpu" is always not registered.
		 * "cpu" should not call cpuidle_unregister().
		 */
		if (i < cpu)
			cpuidle_unregister(&exynos_idle_driver[i]);
	}

	return ret;
}
device_initcall(exynos_idle_init);
