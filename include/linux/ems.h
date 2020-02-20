/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include <linux/plist.h>
#include <linux/sched/idle.h>
#include <linux/sched/topology.h>

struct gb_qos_request {
	struct plist_node node;
	char *name;
	bool active;
};

#define LEAVE_BAND	0

struct task_band {
	int id;
	int sse;
	pid_t tgid;
	raw_spinlock_t lock;

	struct list_head members;
	int member_count;
	struct cpumask playable_cpus;

	unsigned long util;
	unsigned long last_update_time;
};

struct rq;

extern struct kobject *ems_kobj;
extern unsigned int get_cpu_max_capacity(unsigned int cpu, int sse);
#ifdef CONFIG_SCHED_EMS
/* core */
extern void init_ems(void);

/* task util initialization */
extern void exynos_init_entity_util_avg(struct sched_entity *se);

/* wakeup balance */
extern int
exynos_wakeup_balance(struct task_struct *p, int prev_cpu, int sd_flag, int sync);

/* ontime migration */
extern void ontime_migration(void);
extern int ontime_can_migration(struct task_struct *p, int cpu);
extern void ontime_update_load_avg(u64 delta, int cpu, unsigned long weight, struct sched_avg *sa);
extern void ontime_new_entity_load(struct task_struct *parent, struct sched_entity *se);
extern void ontime_trace_task_info(struct task_struct *p);

/* load balance trigger */
extern bool lbt_overutilized(int cpu, int level);
extern void update_lbt_overutil(int cpu, unsigned long capacity);

/* global boost */
extern void gb_qos_update_request(struct gb_qos_request *req, u32 new_value);

/* task band */
extern void sync_band(struct task_struct *p, bool join);
extern void newbie_join_band(struct task_struct *newbie);
extern void update_band(struct task_struct *p, long old_util);
extern int band_playing(struct task_struct *p, int cpu);

/* multi load  */
void update_multi_load(u64 delta, int cpu, struct sched_avg *sa,
		unsigned long weight, int running, struct cfs_rq *cfs_rq);
void init_multi_load(struct sched_entity *se);
void detach_entity_multi_load(struct cfs_rq *cfs_rq, struct sched_entity *se);
void attach_entity_multi_load(struct cfs_rq *cfs_rq, struct sched_entity *se);
void remove_entity_multi_load(struct cfs_rq *cfs_rq, struct sched_entity *se);
void apply_removed_multi_load(struct cfs_rq *cfs_rq);
void update_tg_multi_load(struct cfs_rq *cfs_rq, struct sched_entity *se);
void cfs_se_util_change_multi_load(struct task_struct *p, struct sched_avg *avg);
void enqueue_multi_load(struct cfs_rq *cfs_rq, struct task_struct *p);
void dequeue_multi_load(struct cfs_rq *cfs_rq, struct task_struct *p, bool task_sleep);

/* P.A.R.T */
void update_cpu_active_ratio(struct rq *rq, struct task_struct *p, int type);
void part_cpu_active_ratio(unsigned long *util, unsigned long *max, int cpu);
void set_part_period_start(struct rq *rq);

/* load balance */
extern void lb_add_cfs_task(struct rq *rq, struct sched_entity *se);
extern int lb_check_priority(int src_cpu, int dst_cpu);
extern struct list_head *lb_prefer_cfs_tasks(int src_cpu, int dst_cpu);
extern int lb_need_active_balance(enum cpu_idle_type idle,
				struct sched_domain *sd, int src_cpu, int dst_cpu);

/* check the status of energy table */
extern bool energy_initialized;
extern void set_energy_table_status(bool status);
extern bool get_energy_table_status(void);
#else
static inline void init_ems(void);
static inline void exynos_init_entity_util_avg(struct sched_entity *se) { }

static inline int
exynos_wakeup_balance(struct task_struct *p, int prev_cpu, int sd_flag, int sync)
{
	return -1;
}

static inline void ontime_migration(void) { }
static inline int ontime_can_migration(struct task_struct *p, int cpu)
{
	return 1;
}
static inline void ontime_update_load_avg(u64 delta, int cpu, unsigned long weight, struct sched_avg *sa) { }
static inline void ontime_new_entity_load(struct task_struct *p, struct sched_entity *se) { }
static inline void ontime_trace_task_info(struct task_struct *p) { }

static inline bool lbt_overutilized(int cpu, int level)
{
	return false;
}
static inline void update_lbt_overutil(int cpu, unsigned long capacity) { }

static inline void gb_qos_update_request(struct gb_qos_request *req, u32 new_value) { }

static inline void sync_band(struct task_struct *p, bool join) { }
static inline void newbie_join_band(struct task_struct *newbie) { }
static inline void update_band(struct task_struct *p, long old_util) { }
static inline int band_playing(struct task_struct *p, int cpu)
{
	return 0;
}

static inline void update_multi_load(u64 delta, int cpu, struct sched_avg *sa,
		unsigned long weight, int running, struct cfs_rq *cfs_rq) { }
static inline void init_multi_load(struct sched_entity *se) { }
static inline void detach_entity_multi_load(struct cfs_rq *cfs_rq, struct sched_entity *se) { }
static inline void attach_entity_multi_load(struct cfs_rq *cfs_rq, struct sched_entity *se) { }
static inline void remove_entity_multi_load(struct cfs_rq *cfs_rq, struct sched_entity *se) { }
static inline void apply_removed_multi_load(struct cfs_rq *cfs_rq) { }
static inline void update_tg_multi_load(struct cfs_rq *cfs_rq, struct sched_entity *se) { }
static inline void cfs_se_util_change_multi_load(struct task_struct *p, struct sched_avg *avg) { }
static inline void enqueue_multi_load(struct cfs_rq *cfs_rq, struct task_struct *p) { }
static inline void dequeue_multi_load(struct cfs_rq *cfs_rq, struct task_struct *p, bool task_sleep) { }

/* P.A.R.T */
static inline void update_cpu_active_ratio(struct rq *rq, struct task_struct *p, int type) { }
static inline void part_cpu_active_ratio(unsigned long *util, unsigned long *max, int cpu) { }
static inline void set_part_period_start(struct rq *rq) { }

static inline void lb_add_cfs_task(struct rq *rq, struct sched_entity *se) { }
static inline int lb_check_priority(int src_cpu, int dst_cpu)
{
	return 0;
}
static inline struct list_head *lb_prefer_cfs_tasks(int src_cpu, int dst_cpu)
{
	return NULL;
}
static inline int lb_need_active_balance(enum cpu_idle_type idle,
				struct sched_domain *sd, int src_cpu, int dst_cpu)
{
	return 0;
}
static inline void set_energy_table_status(bool status) { }
static inline bool get_energy_table_status(void)
{
	return false;
}
#endif /* CONFIG_SCHED_EMS */

#ifdef CONFIG_SIMPLIFIED_ENERGY_MODEL
extern void init_sched_energy_table(struct cpumask *cpus, int table_size,
				unsigned long *f_table, unsigned int *v_table,
				int max_f, int min_f);
extern void update_qos_capacity(int cpu, unsigned long freq, unsigned long max);
#else
static inline void init_sched_energy_table(struct cpumask *cpus, int table_size,
				unsigned long *f_table, unsigned int *v_table,
				int max_f, int min_f) { }
static inline void update_qos_capacity(int cpu, unsigned long freq, unsigned long max) { }
#endif

/* Fluid Real Time */
extern unsigned int frt_disable_cpufreq;

/*
 * Maximum number of boost groups to support
 * When per-task boosting is used we still allow only limited number of
 * boost groups for two main reasons:
 * 1. on a real system we usually have only few classes of workloads which
 *    make sense to boost with different values (e.g. background vs foreground
 *    tasks, interactive vs low-priority tasks)
 * 2. a limited number allows for a simpler and more memory/time efficient
 *    implementation especially for the computation of the per-CPU boost
 *    value
 */
#define BOOSTGROUPS_COUNT 5

struct boost_groups {
	/* Maximum boost value for all RUNNABLE tasks on a CPU */
	bool idle;
	int boost_max;
	u64 boost_ts;
	struct {
		/* The boost for tasks on that boost group */
		int boost;
		/* Count of RUNNABLE tasks on that boost group */
		unsigned tasks;
		/* Timestamp of boost activation */
		u64 ts;
	} group[BOOSTGROUPS_COUNT];
	/* CPU's boost group locking */
	raw_spinlock_t lock;
};
