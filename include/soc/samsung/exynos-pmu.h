/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * EXYNOS - PMU(Power Management Unit) support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __EXYNOS_PMU_H
#define __EXYNOS_PMU_H __FILE__

#include <asm/smp_plat.h>

/**
 * struct exynos_cpu_power_ops
 *
 * CPU power control operations
 *
 * @power_up : Set cpu configuration register
 * @power_down : Clear cpu configuration register
 * @power_state : Show cpu status. Return true if cpu power on, otherwise return false.
 */
struct exynos_cpu_power_ops {
        void (*power_up)(unsigned int cpu);
        void (*power_down)(unsigned int cpu);
        int (*power_state)(unsigned int cpu);
        void (*cluster_up)(unsigned int cpu);
        void (*cluster_down)(unsigned int cpu);
        int (*cluster_state)(unsigned int cpu);
};
extern struct exynos_cpu_power_ops exynos_cpu;

#define phy_cluster(cpu)	MPIDR_AFFINITY_LEVEL(cpu_logical_map(cpu), 1)
#define phy_cpu(cpu)		MPIDR_AFFINITY_LEVEL(cpu_logical_map(cpu), 0)

/**
 * The APIs to control the PMU
 */
#ifdef CONFIG_EXYNOS_PMU
extern int exynos_pmu_read(unsigned int offset, unsigned int *val);
extern int exynos_pmu_write(unsigned int offset, unsigned int val);
extern int exynos_pmu_update(unsigned int offset, unsigned int mask, unsigned int val);

extern void exynos_cpu_reset_enable(unsigned int cpu);
extern void exynos_cpu_reset_disable(unsigned int cpu);
extern int exynos_check_cp_status(void);
#else
static inline int exynos_pmu_read(unsigned int offset, unsigned int *val) {return 0;}
static inline int exynos_pmu_write(unsigned int offset, unsigned int val) {return 0;}
static inline int exynos_pmu_update(unsigned int offset, unsigned int mask, unsigned int val) {return 0;}

static inline void exynos_cpu_reset_enable(unsigned int cpu) {return ;}
static inline void exynos_cpu_reset_disable(unsigned int cpu) {return ;}
static inline int exynos_check_cp_status(void) {return 0;}
#endif
#endif /* __EXYNOS_PMU_H */
