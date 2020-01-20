/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS - Mode Changer for boosting
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_PSTATE_MODE_CHANGER_H
#define __EXYNOS_PSTATE_MODE_CHANGER_H __FILE__

#ifdef CONFIG_EXYNOS_PSTATE_MODE_CHANGER
void exynos_emc_update(int cpu);
int exynos_emc_update_cpu_pwr(unsigned int cpu, bool on);
int emc_get_boost_freq(int cpu);
void emc_check_available_freq(struct cpumask *cpus, unsigned int target_freq);
int emc_cpu_pre_off_callback(unsigned int cpu);
#else
static inline void exynos_emc_update(int cpu) {};
static inline int exynos_emc_update_cpu_pwr(unsigned int cpu, bool on) { return 0; };
static inline int emc_get_boost_freq(int cpu) { return 0; };
static inline void emc_check_available_freq(struct cpumask *cpus, unsigned int target_freq) { return; };
static inline int emc_cpu_pre_off_callback(unsigned int cpu) { return 0; };
#endif

unsigned int exynos_cpufreq_get_locked(unsigned int cpu);

#endif /* __EXYNOS_PSTATE_MODE_CHANGER_H */

