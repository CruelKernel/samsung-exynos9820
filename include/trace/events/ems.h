/*
 *  Copyright (C) 2017 Park Bumgyu <bumgyu.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM ems

#if !defined(_TRACE_EMS_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_EMS_H

#include <linux/sched.h>
#include <linux/tracepoint.h>

/*
 * Tracepoint for selecting eco cpu
 */
TRACE_EVENT(ems_select_eco_cpu,

	TP_PROTO(struct task_struct *p, int eco_cpu, int prev_cpu, int best_cpu,
		unsigned int prev_energy, unsigned int best_energy),

	TP_ARGS(p, eco_cpu, prev_cpu, best_cpu, prev_energy, best_energy),

	TP_STRUCT__entry(
		__array(	char,		comm,	TASK_COMM_LEN	)
		__field(	pid_t,		pid			)
		__field(	int,		eco_cpu			)
		__field(	int,		prev_cpu		)
		__field(	int,		best_cpu		)
		__field(	unsigned int,	prev_energy		)
		__field(	unsigned int,	best_energy		)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->eco_cpu	= eco_cpu;
		__entry->prev_cpu	= prev_cpu;
		__entry->best_cpu	= best_cpu;
		__entry->prev_energy	= prev_energy;
		__entry->best_energy	= best_energy;
	),

	TP_printk("comm=%s pid=%d eco_cpu=%d prev_cpu=%d best_cpu=%d "
		  "prev_energy=%u best_energy=%u",
		__entry->comm, __entry->pid, __entry->eco_cpu, __entry->prev_cpu,
		__entry->best_cpu, __entry->prev_energy, __entry->best_energy)
);

/*
 * Tracepoint for proper cpu selection
 */
TRACE_EVENT(ems_select_proper_cpu,

	TP_PROTO(struct task_struct *p, int best_cpu, unsigned long min_util),

	TP_ARGS(p, best_cpu, min_util),

	TP_STRUCT__entry(
		__array(	char,		comm,	TASK_COMM_LEN	)
		__field(	pid_t,		pid			)
		__field(	int,		best_cpu		)
		__field(	unsigned long,	min_util		)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->best_cpu	= best_cpu;
		__entry->min_util	= min_util;
	),

	TP_printk("comm=%s pid=%d best_cpu=%d min_util=%lu",
		  __entry->comm, __entry->pid, __entry->best_cpu, __entry->min_util)
);

/*
 * Tracepoint for wakeup balance
 */
TRACE_EVENT(ems_wakeup_balance,

	TP_PROTO(struct task_struct *p, int target_cpu, char *state),

	TP_ARGS(p, target_cpu, state),

	TP_STRUCT__entry(
		__array(	char,		comm,	TASK_COMM_LEN	)
		__field(	pid_t,		pid			)
		__field(	int,		target_cpu		)
		__array(	char,		state,		30	)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->target_cpu	= target_cpu;
		memcpy(__entry->state, state, 30);
	),

	TP_printk("comm=%s pid=%d target_cpu=%d state=%s",
		  __entry->comm, __entry->pid, __entry->target_cpu, __entry->state)
);

/*
 * Tracepoint for performance cpu finder
 */
TRACE_EVENT(ems_select_perf_cpu,

	TP_PROTO(struct task_struct *p, int best_cpu, int backup_cpu),

	TP_ARGS(p, best_cpu, backup_cpu),

	TP_STRUCT__entry(
		__array(	char,		comm,	TASK_COMM_LEN	)
		__field(	pid_t,		pid			)
		__field(	int,		best_cpu		)
		__field(	int,		backup_cpu		)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->best_cpu	= best_cpu;
		__entry->backup_cpu	= backup_cpu;
	),

	TP_printk("comm=%s pid=%d best_cpu=%d backup_cpu=%d",
		  __entry->comm, __entry->pid, __entry->best_cpu, __entry->backup_cpu)
);

/*
 * Tracepoint for global boost
 */
TRACE_EVENT(ems_global_boost,

	TP_PROTO(char *name, int boost),

	TP_ARGS(name, boost),

	TP_STRUCT__entry(
		__array(	char,	name,	64	)
		__field(	int,	boost		)
	),

	TP_fast_assign(
		memcpy(__entry->name, name, 64);
		__entry->boost		= boost;
	),

	TP_printk("name=%s global_boost=%d", __entry->name, __entry->boost)
);

/*
 * Tracepoint for prefer idle
 */
TRACE_EVENT(ems_prefer_idle,

	TP_PROTO(struct task_struct *p, int orig_cpu, int target_cpu,
		unsigned long capacity_orig, unsigned long task_util,
		unsigned long new_util, int idle),

	TP_ARGS(p, orig_cpu, target_cpu, capacity_orig, task_util, new_util, idle),

	TP_STRUCT__entry(
		__array(	char,		comm,	TASK_COMM_LEN	)
		__field(	pid_t,		pid			)
		__field(	int,		orig_cpu		)
		__field(	int,		target_cpu		)
		__field(	unsigned long,	capacity_orig		)
		__field(	unsigned long,	task_util		)
		__field(	unsigned long,	new_util		)
		__field(	int,		idle			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->orig_cpu	= orig_cpu;
		__entry->target_cpu	= target_cpu;
		__entry->capacity_orig	= capacity_orig;
		__entry->task_util	= task_util;
		__entry->new_util	= new_util;
		__entry->idle		= idle;
	),

	TP_printk("comm=%s pid=%d orig_cpu=%d target_cpu=%d cap_org=%lu task_util=%lu new_util=%lu idle=%d",
		__entry->comm, __entry->pid, __entry->orig_cpu, __entry->target_cpu,
		__entry->capacity_orig, __entry->task_util, __entry->new_util, __entry->idle)
);

TRACE_EVENT(ems_select_idle_cpu,

	TP_PROTO(struct task_struct *p, int cpu, char *state),

	TP_ARGS(p, cpu, state),

	TP_STRUCT__entry(
		__array(	char,		comm,	TASK_COMM_LEN	)
		__field(	pid_t,		pid			)
		__field(	int,		cpu			)
		__array(	char,		state,		30	)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->cpu		= cpu;
		memcpy(__entry->state, state, 30);
	),

	TP_printk("comm=%s pid=%d target_cpu=%d state=%s",
		  __entry->comm, __entry->pid, __entry->cpu, __entry->state)
);

/*
 * Tracepoint for ontime migration
 */
TRACE_EVENT(ems_ontime_migration,

	TP_PROTO(struct task_struct *p, unsigned long load,
		int src_cpu, int dst_cpu, int boost_migration),

	TP_ARGS(p, load, src_cpu, dst_cpu, boost_migration),

	TP_STRUCT__entry(
		__array(	char,		comm,	TASK_COMM_LEN	)
		__field(	pid_t,		pid			)
		__field(	int,		sse			)
		__field(	unsigned long,	load			)
		__field(	int,		src_cpu			)
		__field(	int,		dst_cpu			)
		__field(	int,		bm			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->sse		= p->sse;
		__entry->load		= load;
		__entry->src_cpu	= src_cpu;
		__entry->dst_cpu	= dst_cpu;
		__entry->bm		= boost_migration;
	),

	TP_printk("comm=%s pid=%d sse=%d ontime_load_avg=%lu src_cpu=%d dst_cpu=%d boost_migration=%d",
		__entry->comm, __entry->pid, __entry->sse, __entry->load,
		__entry->src_cpu, __entry->dst_cpu, __entry->bm)
);

TRACE_EVENT(ems_ontime_check_migrate,

	TP_PROTO(struct task_struct *tsk, int cpu, int migrate, char *label),

	TP_ARGS(tsk, cpu, migrate, label),

	TP_STRUCT__entry(
		__array( char,		comm,	TASK_COMM_LEN	)
		__field( pid_t,		pid			)
		__field( int,		cpu			)
		__field( int,		migrate			)
		__array( char,		label,	64		)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid			= tsk->pid;
		__entry->cpu			= cpu;
		__entry->migrate		= migrate;
		strncpy(__entry->label, label, 63);
	),

	TP_printk("comm=%s pid=%d target_cpu=%d migrate=%d reason=%s",
		__entry->comm, __entry->pid, __entry->cpu,
		__entry->migrate, __entry->label)
);

TRACE_EVENT(ems_ontime_task_wakeup,

	TP_PROTO(struct task_struct *tsk, int src_cpu, int dst_cpu, char *label),

	TP_ARGS(tsk, src_cpu, dst_cpu, label),

	TP_STRUCT__entry(
		__array( char,		comm,	TASK_COMM_LEN	)
		__field( pid_t,		pid			)
		__field( int,		sse			)
		__field( int,		src_cpu			)
		__field( int,		dst_cpu			)
		__array( char,		label,	64		)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid			= tsk->pid;
		__entry->sse			= tsk->sse;
		__entry->src_cpu		= src_cpu;
		__entry->dst_cpu		= dst_cpu;
		strncpy(__entry->label, label, 63);
	),

	TP_printk("comm=%s pid=%d sse=%d src_cpu=%d dst_cpu=%d reason=%s",
		__entry->comm, __entry->pid, __entry->sse,
		__entry->src_cpu, __entry->dst_cpu, __entry->label)
);

TRACE_EVENT(ems_lbt_overutilized,

	TP_PROTO(int cpu, int level, unsigned long util, unsigned long capacity, bool overutilized),

	TP_ARGS(cpu, level, util, capacity, overutilized),

	TP_STRUCT__entry(
		__field( int,		cpu			)
		__field( int,		level			)
		__field( unsigned long,	util			)
		__field( unsigned long,	capacity		)
		__field( bool,		overutilized		)
	),

	TP_fast_assign(
		__entry->cpu			= cpu;
		__entry->level			= level;
		__entry->util			= util;
		__entry->capacity		= capacity;
		__entry->overutilized		= overutilized;
	),

	TP_printk("cpu=%d level=%d util=%lu capacity=%lu overutilized=%d",
		__entry->cpu, __entry->level, __entry->util,
		__entry->capacity, __entry->overutilized)
);

TRACE_EVENT(ems_update_band,

	TP_PROTO(int band_id, unsigned long band_util, int member_count, unsigned int playable_cpus),

	TP_ARGS(band_id, band_util, member_count, playable_cpus),

	TP_STRUCT__entry(
		__field( int,		band_id			)
		__field( unsigned long,	band_util		)
		__field( int,		member_count		)
		__field( unsigned int,	playable_cpus		)
	),

	TP_fast_assign(
		__entry->band_id		= band_id;
		__entry->band_util		= band_util;
		__entry->member_count		= member_count;
		__entry->playable_cpus		= playable_cpus;
	),

	TP_printk("band_id=%d band_util=%ld member_count=%d playable_cpus=%#x",
			__entry->band_id, __entry->band_util, __entry->member_count,
			__entry->playable_cpus)
);

TRACE_EVENT(ems_band_schedule,

	TP_PROTO(unsigned int play_cpus, int cpu, int ratio,
		unsigned long capacity, unsigned long util, int type),

	TP_ARGS(play_cpus, cpu, ratio, capacity, util, type),

	TP_STRUCT__entry(
		__field( unsigned int,	play_cpus	)
		__field( int,		cpu		)
		__field( int,		ratio		)
		__field( unsigned long,	capacity	)
		__field( unsigned long,	util		)
		__field( int,		type		)
	),

	TP_fast_assign(
		__entry->play_cpus	= play_cpus;
		__entry->cpu		= cpu;
		__entry->ratio		= ratio;
		__entry->capacity	= capacity;
		__entry->util		= util;
		__entry->type		= type;
	),

	TP_printk("play_cpus=%#x cpu=%d ratio=%d total_capcaity=%ld total_util=%ld type=%d",
		__entry->play_cpus, __entry->cpu, __entry->ratio,
		__entry->capacity, __entry->util, __entry->type)
);

TRACE_EVENT(ems_manage_band,

	TP_PROTO(struct task_struct *p, int band_id, char *event),

	TP_ARGS(p, band_id, event),

	TP_STRUCT__entry(
		__array( char,		comm,		TASK_COMM_LEN	)
		__field( pid_t,		pid				)
		__field( int,		band_id				)
		__array( char,		event,		64		)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid			= p->pid;
		__entry->band_id		= band_id;
		strncpy(__entry->event, event, 63);
	),

	TP_printk("comm=%s pid=%d band_id=%d event=%s",
			__entry->comm, __entry->pid, __entry->band_id, __entry->event)
);

/*
 * Tracepoint for initializing multi load for new tasks.
 */
TRACE_EVENT(ems_multi_load_new_task,

	TP_PROTO(struct task_struct *tsk, u32 sum, unsigned long avg),

	TP_ARGS(tsk, sum, avg),

	TP_STRUCT__entry(
		__array( char,		comm,	TASK_COMM_LEN		)
		__field( pid_t,		pid				)
		__field( int,		cpu				)
		__field( unsigned long,	avg				)
		__field( u32,		sum				)
		__field( int,		sse				)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid			= tsk->pid;
		__entry->cpu			= task_cpu(tsk);
		__entry->avg			= avg;
		__entry->sum			= sum;
		__entry->sse			= tsk->sse;
	),
	TP_printk("comm=%s pid=%d cpu=%d runnable_avg=%lu runnable_sum=%d sse=%d",
		  __entry->comm, __entry->pid, __entry->cpu, __entry->avg,
		  __entry->sum, __entry->sse)
);

/*
 * Tracepoint for accounting multi load for tasks.
 */
TRACE_EVENT(ems_multi_load_task,

	TP_PROTO(struct task_struct *tsk, unsigned long runnable, unsigned long util),

	TP_ARGS(tsk, runnable, util),

	TP_STRUCT__entry(
		__array( char,		comm,	TASK_COMM_LEN		)
		__field( pid_t,		pid				)
		__field( int,		cpu				)
		__field( int,		sse				)
		__field( unsigned long,	runnable			)
		__field( unsigned long,	util				)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid			= tsk->pid;
		__entry->cpu			= task_cpu(tsk);
		__entry->sse			= tsk->sse;
		__entry->runnable		= runnable;
		__entry->util			= util;
	),
	TP_printk("comm=%s pid=%d cpu=%d sse=%d runnable=%lu util=%lu",
		  __entry->comm, __entry->pid, __entry->cpu, __entry->sse,
					__entry->runnable, __entry->util)
);

/*
 * Tracepoint for accounting multi load for cpu.
 */
TRACE_EVENT(ems_multi_load_cpu,

	TP_PROTO(int cpu, unsigned long util, unsigned long util_s),

	TP_ARGS(cpu, util, util_s),

	TP_STRUCT__entry(
		__field( int,		cpu				)
		__field( unsigned long,	util				)
		__field( unsigned long,	util_s				)
	),

	TP_fast_assign(
		__entry->cpu			= cpu;
		__entry->util			= util;
		__entry->util_s			= util_s;
	),
	TP_printk("cpu=%d util=%lu util_s=%lu",
		  __entry->cpu, __entry->util, __entry->util_s)
);

/*
 * Tracepoint for tasks' estimated utilization.
 */
TRACE_EVENT(ems_util_est_task,

	TP_PROTO(struct task_struct *tsk, struct sched_avg *avg),

	TP_ARGS(tsk, avg),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN		)
		__field( pid_t,		pid			)
		__field( int,		cpu			)
		__field( unsigned int,	util_avg		)
		__field( unsigned int,	est_enqueued		)
		__field( unsigned int,	est_ewma		)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid			= tsk->pid;
		__entry->cpu			= task_cpu(tsk);
		__entry->util_avg		= avg->ml.util_avg;
		__entry->est_enqueued		= avg->ml.util_est.enqueued;
		__entry->est_ewma		= avg->ml.util_est.ewma;
	),

	TP_printk("comm=%s pid=%d cpu=%d util_avg=%u util_est_ewma=%u util_est_enqueued=%u",
		  __entry->comm,
		  __entry->pid,
		  __entry->cpu,
		  __entry->util_avg,
		  __entry->est_ewma,
		  __entry->est_enqueued)
);

/*
 * Tracepoint for root cfs_rq's estimated utilization.
 */
TRACE_EVENT(ems_util_est_cpu,

	TP_PROTO(int cpu, struct cfs_rq *cfs_rq),

	TP_ARGS(cpu, cfs_rq),

	TP_STRUCT__entry(
		__field( int,		cpu			)
		__field( unsigned int,	util_avg		)
		__field( unsigned int,	util_est_enqueued	)
		__field( unsigned int,	util_est_enqueued_s	)
	),

	TP_fast_assign(
		__entry->cpu			= cpu;
		__entry->util_avg		= cfs_rq->avg.ml.util_avg;
		__entry->util_est_enqueued	= cfs_rq->avg.ml.util_est.enqueued;
		__entry->util_est_enqueued_s	= cfs_rq->avg.ml.util_est_s.enqueued;
	),

	TP_printk("cpu=%d util_avg=%u util_est_enqueued=%u util_est_enqueued_s=%u",
		  __entry->cpu,
		  __entry->util_avg,
		  __entry->util_est_enqueued,
		  __entry->util_est_enqueued_s)
);

TRACE_EVENT(ems_cpu_active_ratio_patten,

	TP_PROTO(int cpu, int p_count, int p_avg, int p_stdev),

	TP_ARGS(cpu, p_count, p_avg, p_stdev),

	TP_STRUCT__entry(
		__field( int,	cpu				)
		__field( int,	p_count				)
		__field( int,	p_avg				)
		__field( int,	p_stdev				)
	),

	TP_fast_assign(
		__entry->cpu			= cpu;
		__entry->p_count		= p_count;
		__entry->p_avg			= p_avg;
		__entry->p_stdev		= p_stdev;
	),

	TP_printk("cpu=%d p_count=%2d p_avg=%4d p_stdev=%4d",
		__entry->cpu, __entry->p_count, __entry->p_avg, __entry->p_stdev)
);

TRACE_EVENT(ems_cpu_active_ratio,

	TP_PROTO(int cpu, struct part *pa, char *event),

	TP_ARGS(cpu, pa, event),

	TP_STRUCT__entry(
		__field( int,	cpu				)
		__field( u64,	start				)
		__field( int,	recent				)
		__field( int,	last				)
		__field( int,	avg				)
		__field( int,	max				)
		__field( int,	est				)
		__array( char,	event,		64		)
	),

	TP_fast_assign(
		__entry->cpu			= cpu;
		__entry->start			= pa->period_start;
		__entry->recent			= pa->active_ratio_recent;
		__entry->last			= pa->hist[pa->hist_idx];
		__entry->avg			= pa->active_ratio_avg;
		__entry->max			= pa->active_ratio_max;
		__entry->est			= pa->active_ratio_est;
		strncpy(__entry->event, event, 63);
	),

	TP_printk("cpu=%d start=%llu recent=%4d last=%4d avg=%4d max=%4d est=%4d event=%s",
		__entry->cpu, __entry->start, __entry->recent, __entry->last,
		__entry->avg, __entry->max, __entry->est, __entry->event)
);

TRACE_EVENT(ems_cpu_active_ratio_util_stat,

	TP_PROTO(int cpu, unsigned long part_util, unsigned long pelt_util),

	TP_ARGS(cpu, part_util, pelt_util),

	TP_STRUCT__entry(
		__field( int,		cpu					)
		__field( unsigned long,	part_util				)
		__field( unsigned long,	pelt_util				)
	),

	TP_fast_assign(
		__entry->cpu			= cpu;
		__entry->part_util		= part_util;
		__entry->pelt_util		= pelt_util;
	),

	TP_printk("cpu=%d part_util=%lu pelt_util=%lu", __entry->cpu, __entry->part_util, __entry->pelt_util)
);

TRACE_EVENT(ems_prefer_perf_service,

	TP_PROTO(struct task_struct *p, unsigned long util, int service_cpu, char *event),

	TP_ARGS(p, util, service_cpu, event),

	TP_STRUCT__entry(
		__array( char,		comm,		TASK_COMM_LEN	)
		__field( pid_t,		pid				)
		__field( unsigned long,	util				)
		__field( int,		service_cpu			)
		__array( char,		event,		64		)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid			= p->pid;
		__entry->util			= util;
		__entry->service_cpu		= service_cpu;
		strncpy(__entry->event, event, 63);
	),

	TP_printk("comm=%s pid=%d util=%lu service_cpu=%d event=%s",
			__entry->comm, __entry->pid, __entry->util, __entry->service_cpu, __entry->event)
);

TRACE_EVENT(ems_select_service_cpu,

	TP_PROTO(struct task_struct *p, int best_cpu, int backup_cpu),

	TP_ARGS(p, best_cpu, backup_cpu),

	TP_STRUCT__entry(
		__array( char,		comm,		TASK_COMM_LEN	)
		__field( pid_t,		pid				)
		__field( int,		best_cpu			)
		__field( int,		backup_cpu			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid			= p->pid;
		__entry->best_cpu		= best_cpu;
		__entry->backup_cpu		= backup_cpu;
	),

	TP_printk("comm=%s pid=%d best_cpu=%d backup_cpu",
			__entry->comm, __entry->pid, __entry->best_cpu, __entry->backup_cpu)
);

/*
 * Tracepoint for frequency variant boost
 */

TRACE_EVENT(ems_freqvar_st_boost,

	TP_PROTO(int cpu, int ratio),

	TP_ARGS(cpu, ratio),

	TP_STRUCT__entry(
		__field( int,		cpu			)
		__field( int,		ratio			)
	),

	TP_fast_assign(
		__entry->cpu			= cpu;
		__entry->ratio			= ratio;
	),

	TP_printk("cpu=%d ratio=%d", __entry->cpu, __entry->ratio)
);
TRACE_EVENT(ems_freqvar_boost,

	TP_PROTO(int cpu, int ratio, unsigned long step_max_util,
			unsigned long util, unsigned long boosted_util),

	TP_ARGS(cpu, ratio, step_max_util, util, boosted_util),

	TP_STRUCT__entry(
		__field( int,		cpu			)
		__field( int,		ratio			)
		__field( unsigned long,	step_max_util		)
		__field( unsigned long,	util			)
		__field( unsigned long,	boosted_util		)
	),

	TP_fast_assign(
		__entry->cpu			= cpu;
		__entry->ratio			= ratio;
		__entry->step_max_util		= step_max_util;
		__entry->util			= util;
		__entry->boosted_util		= boosted_util;
	),

	TP_printk("cpu=%d ratio=%d step_max_util=%lu util=%lu boosted_util=%lu",
		  __entry->cpu, __entry->ratio, __entry->step_max_util,
		  __entry->util, __entry->boosted_util)
);
#endif /* _TRACE_EMS_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
