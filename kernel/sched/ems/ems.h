/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "../sched-pelt.h"

#define cpu_selected(cpu)	(cpu >= 0)
#define tsk_cpus_allowed(tsk)	(&(tsk)->cpus_allowed)

extern struct kobject *ems_kobj;

extern int select_service_cpu(struct task_struct *p);
extern int ontime_task_wakeup(struct task_struct *p, int sync);
extern int select_perf_cpu(struct task_struct *p);
extern int global_boosting(struct task_struct *p);
extern int global_boosted(void);
extern int select_energy_cpu(struct task_struct *p, int prev_cpu, int sd_flag, int sync);
extern int select_best_cpu(struct task_struct *p, int prev_cpu, int sd_flag, int sync);
extern unsigned int calculate_energy(struct task_struct *p, int target_cpu);
extern int alloc_bands(void);
extern int band_play_cpu(struct task_struct *p);

extern int need_ontime_migration_trigger(int cpu, struct task_struct *p);

extern unsigned long ml_task_util(struct task_struct *p);
extern unsigned long ml_task_runnable(struct task_struct *p);
extern unsigned long ml_task_util_est(struct task_struct *p);
extern unsigned long ml_boosted_task_util(struct task_struct *p);
extern unsigned long ml_cpu_util_wake(int cpu, struct task_struct *p);
extern unsigned long ml_task_attached_cpu_util(int cpu, struct task_struct *p);
extern unsigned long ml_boosted_cpu_util(int cpu);
extern unsigned long ml_cpu_util(int cpu);
extern unsigned long ml_cpu_util_ratio(int cpu, int sse);
extern unsigned long ml_task_util_est(struct task_struct *p);
extern unsigned long __ml_cpu_util_est(int cpu, int sse);

extern void init_part(void);

#ifdef CONFIG_SCHED_TUNE
extern int prefer_perf_cpu(struct task_struct *p);
extern int prefer_idle_cpu(struct task_struct *p);
extern int group_balancing(struct task_struct *p);
#else
static inline int prefer_perf_cpu(struct task_struct *p) { return -1; }
static inline int prefer_idle_cpu(struct task_struct *p) { return -1; }
#endif

extern unsigned long task_util(struct task_struct *p);
extern unsigned int get_cpu_mips(unsigned int cpu, int sse);
extern unsigned int get_cpu_max_capacity(unsigned int cpu, int sse);
extern unsigned long get_freq_cap(unsigned int cpu, unsigned long freq, int sse);

extern unsigned long boosted_task_util(struct task_struct *p);
extern unsigned long capacity_orig_of_sse(int cpu, int sse);
extern unsigned long capacity_ratio(int cpu, int sse);

static inline struct task_struct *task_of(struct sched_entity *se)
{
	return container_of(se, struct task_struct, se);
}

struct list_head *lb_cfs_tasks(struct rq *rq, int sse);

extern unsigned long freqvar_st_boost_vector(int cpu);
