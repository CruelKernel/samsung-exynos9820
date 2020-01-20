/*
 * Copyright (c) 2018 Park Bumgyu, Samsung Electronics Co., Ltd <bumgyu.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_CPUPM_H
#define __EXYNOS_CPUPM_H __FILE__

extern int exynos_cpu_pm_enter(int cpu, int index);
extern void exynos_cpu_pm_exit(int cpu, int cancel);

enum {
	POWERMODE_TYPE_CLUSTER = 0,
	POWERMODE_TYPE_SYSTEM,
};

struct freqvariant_idlefactor {
	int init;
	unsigned int bias_index;	/* 0b1 : C1, 0b10 : C2, ... */
	spinlock_t freqvar_if_lock;
	unsigned int cur_freqvar_if;
	unsigned int nfreqvar_ifs;
	unsigned int *freqvar_ifs;

	unsigned int orig_target_res;
	unsigned int orig_exit_lat;

	unsigned int bias_target_res; /* same with btarget_residency in cpuidle_state struct */
	unsigned int bias_exit_lat; /* same with bexit_latency in cpuidle_state struct */
};

extern bool exynos_cpuhp_last_cpu(unsigned int cpu);

#ifdef CONFIG_CPU_IDLE
void exynos_update_ip_idle_status(int index, int idle);
int exynos_get_idle_ip_index(const char *name);
void disable_power_mode(int cpu, int type);
void enable_power_mode(int cpu, int type);
#else
static inline void exynos_update_ip_idle_status(int index, int idle)
{
	return;
}

static inline int exynos_get_idle_ip_index(const char *name)
{
	return 0;
}
static inline void disable_power_mode(int cpu, int type)
{
	return;
}
static inline void enable_power_mode(int cpu, int type)
{
	return;
}
#endif

#ifdef CONFIG_SMP
extern DEFINE_PER_CPU(bool, pending_ipi);
static inline bool is_IPI_pending(const struct cpumask *mask)
{
	unsigned int cpu;

	for_each_cpu_and(cpu, cpu_online_mask, mask) {
		if (per_cpu(pending_ipi, cpu))
			return true;
	}
	return false;
}
#else
static inline bool is_IPI_pending(const struct cpumask *mask)
{
	return false;
}
#endif
#endif /* __EXYNOS_CPUPM_H */
