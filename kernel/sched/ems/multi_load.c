/*
 * Multi-purpose Load tracker
 *
 * Copyright (C) 2018 Samsung Electronics Co., Ltd
 * Park Bumgyu <bumgyu.park@samsung.com>
 */

#include <linux/sched.h>
#include <linux/ems_service.h>
#include <linux/ems.h>
#include <trace/events/ems.h>

#include "../tune.h"
#include "../sched.h"
#include "ems.h"

extern long schedtune_margin(unsigned long capacity, unsigned long signal, long boost);

static inline int get_sse(struct sched_entity *se)
{
	if (se->my_q)
		return 0;

	return task_of(se)->sse;
}

/*
 * ml_task_runnable - task runnable
 *
 * The time while the task is in the runqueue. This includes not only task
 * running time but also waiting time in the runqueue. The calculation
 * is the same as the task util.
 */
unsigned long ml_task_runnable(struct task_struct *p)
{
	int boost = schedtune_task_boost(p);
	unsigned long runnable_avg = READ_ONCE(p->se.avg.ml.runnable_avg);
	unsigned long capacity;

	if (boost == 0)
		return runnable_avg;

	capacity = capacity_orig_of_sse(task_cpu(p), p->sse);

	return runnable_avg + schedtune_margin(capacity, runnable_avg, boost);
}

/*
 * ml_task_util - task util
 *
 * Task utilization. The calculation is the same as the task util of cfs,
 * but applied capacity is different according to sse and uss of the task,
 * therefore, it sse task has different values from the task util of cfs.
 */
unsigned long ml_task_util(struct task_struct *p)
{
	return READ_ONCE(p->se.avg.ml.util_avg);
}

/*
 * _ml_task_util_est/ml_task_util_est - task util with util-est
 *
 * Task utilization with util-est, The calculation is the same as
 * task_util_est of cfs.
 */
static unsigned long _ml_task_util_est(struct task_struct *p)
{
	struct util_est ue = READ_ONCE(p->se.avg.ml.util_est);

	return schedtune_util_est_en(p) ? max(ue.ewma, ue.enqueued)
					: ml_task_util(p);
}

unsigned long ml_task_util_est(struct task_struct *p)
{
	return schedtune_util_est_en(p) ? max(READ_ONCE(p->se.avg.ml.util_avg), _ml_task_util_est(p))
					: ml_task_util(p);
}

/*
 * ml_boosted_task_util - task util with schedtune boost
 *
 * Boosted task utilization, it same as boosted_task_util of cfs.
 */
unsigned long ml_boosted_task_util(struct task_struct *p)
{
	int boost = schedtune_task_boost(p);
	unsigned long util = ml_task_util(p);
	unsigned long capacity;

	if (boost == 0)
		return util;

	capacity = capacity_orig_of_sse(task_cpu(p), p->sse);

	return util + schedtune_margin(capacity, util, boost);
}

/*
 * __ml_cpu_util - sse/uss utilization in cpu
 *
 * Cpu utilization. This function returns sse or uss utilization in
 * the cpu according to "sse" parameter.
 */
unsigned long __ml_cpu_util(int cpu, int sse)
{
	struct cfs_rq *cfs_rq = &cpu_rq(cpu)->cfs;

	return sse ? READ_ONCE(cfs_rq->avg.ml.util_avg_s) :
				READ_ONCE(cfs_rq->avg.ml.util_avg);
}

unsigned long __ml_cpu_util_est(int cpu, int sse)
{
	struct cfs_rq *cfs_rq = &cpu_rq(cpu)->cfs;

	return sse ? READ_ONCE(cfs_rq->avg.ml.util_est_s.enqueued) :
		READ_ONCE(cfs_rq->avg.ml.util_est.enqueued);
}

/*
 * __normalize_util - combine sse and uss utilization
 *
 * Combine sse and uss utilization and normalize to sse or uss according to "sse"
 * parameter.
 */
static inline unsigned long
__normalize_util(int cpu, unsigned int uss_util, unsigned int sse_util, int sse)
{
	if (sse)
		return sse_util + ((capacity_ratio(cpu, sse) * uss_util) >> SCHED_CAPACITY_SHIFT);
	else
		return uss_util + ((capacity_ratio(cpu, sse) * sse_util) >> SCHED_CAPACITY_SHIFT);
}

/* uss is default because cfs refers uss */
#define USS	0
#define SSE	1

static unsigned long ml_cpu_util_est(int cpu)
{
	return __normalize_util(cpu, __ml_cpu_util_est(cpu, USS),
				__ml_cpu_util_est(cpu, SSE), USS);
}

/*
 * _ml_cpu_util - sse/uss combined cpu utilization
 *
 * Sse and uss combined cpu utilization. This function returns combined cpu
 * utilization normalized to sse or uss according to "sse" parameter.
 */
unsigned long _ml_cpu_util(int cpu, int sse)
{
	unsigned long util;

	util = __normalize_util(cpu, __ml_cpu_util(cpu, USS),
				__ml_cpu_util(cpu, SSE), sse);

	if (sched_feat(UTIL_EST))
		util = max_t(unsigned long, util, ml_cpu_util_est(cpu));

	return min_t(unsigned long, util, capacity_orig_of(cpu));
}

/*
 * ml_cpu_util - sse/uss combiend cpu utilization
 *
 * Sse and uss combined and uss normalized cpu utilization. Default policy
 * is to normalize to uss because cfs refers uss.
 */
unsigned long ml_cpu_util(int cpu)
{
	return _ml_cpu_util(cpu, USS);
}

/*
 * ml_cpu_util_ratio - cpu usuage ratio
 *
 * Cpu usage ratio of sse or uss. The ratio based on a maximum of 1024.
 */
unsigned long ml_cpu_util_ratio(int cpu, int sse)
{
	return (__ml_cpu_util(cpu, sse) << SCHED_CAPACITY_SHIFT)
					/ capacity_orig_of_sse(cpu, sse);
}

#define UTIL_AVG_UNCHANGED 0x1

/*
 * ml_cpu_util_wake - cpu utilization except waking task
 *
 * Cpu utilization with any contributions from the waking task p removed.
 */
unsigned long ml_cpu_util_wake(int cpu, struct task_struct *p)
{
	unsigned long uss_util, sse_util;

	if (cpu != task_cpu(p) || !READ_ONCE(p->se.avg.last_update_time))
		return ml_cpu_util(cpu);

	uss_util = __ml_cpu_util(cpu, USS);
	sse_util = __ml_cpu_util(cpu, SSE);

	if (p->sse)
		sse_util -= min_t(unsigned long, sse_util, ml_task_util(p));
	else
		uss_util -= min_t(unsigned long, uss_util, ml_task_util(p));

	if (sched_feat(UTIL_EST)) {
		unsigned int uss_util_est = __ml_cpu_util_est(cpu, USS);
		unsigned int sse_util_est = __ml_cpu_util_est(cpu, SSE);

		if (unlikely(task_on_rq_queued(p) || current == p)) {
			if (p->sse) {
				sse_util_est -= min_t(unsigned int, sse_util_est,
						(_ml_task_util_est(p) | UTIL_AVG_UNCHANGED));
			} else {
				uss_util_est -= min_t(unsigned int, uss_util_est,
						(_ml_task_util_est(p) | UTIL_AVG_UNCHANGED));
			}
		}

		uss_util = max_t(unsigned long, uss_util, uss_util_est);
		sse_util = max_t(unsigned long, sse_util, sse_util_est);
	}

	uss_util = min_t(unsigned long, uss_util, capacity_orig_of_sse(cpu, USS));
	sse_util = min_t(unsigned long, sse_util, capacity_orig_of_sse(cpu, SSE));

	return __normalize_util(cpu, uss_util, sse_util, USS);
}

/*
 * ml_task_attached_cpu_util - cpu utilization including waking task
 *
 * Cpu utilization when the waking task is attached to this cpu. There is no
 * change in utilizatino for the cpu on which the task is running.
 */
unsigned long ml_task_attached_cpu_util(int cpu, struct task_struct *p)
{
	unsigned long uss_util, sse_util;

	uss_util = __ml_cpu_util(cpu, USS);
	sse_util = __ml_cpu_util(cpu, SSE);

	if (READ_ONCE(p->se.avg.last_update_time))
		return ml_cpu_util(cpu);

	if (cpu != task_cpu(p)) {
		if (p->sse)
			sse_util += ml_task_util(p);
		else
			uss_util += ml_task_util(p);
	}

	if (sched_feat(UTIL_EST)) {
		unsigned int uss_util_est = __ml_cpu_util_est(cpu, USS);
		unsigned int sse_util_est = __ml_cpu_util_est(cpu, SSE);

		if (p->sse)
			sse_util_est += ml_task_util_est(p);
		else
			uss_util_est += ml_task_util_est(p);

		uss_util = max_t(unsigned long, uss_util, uss_util_est);
		sse_util = max_t(unsigned long, sse_util, sse_util_est);
	}

	return __normalize_util(cpu, uss_util, sse_util, USS);
}

/*
 * ml_boosted_cpu_util - sse/uss combiend cpu utilization with boost
 *
 * Sse and uss combined and uss normalized cpu utilization with schedtune.boost.
 */

extern DEFINE_PER_CPU(struct boost_groups, cpu_boost_groups);
unsigned long ml_boosted_cpu_util(int cpu)
{
	int fv_boost = 0, boost = schedtune_cpu_boost(cpu);
	struct boost_groups *bg = &per_cpu(cpu_boost_groups, cpu);
	unsigned long util = ml_cpu_util(cpu);
	unsigned long capacity;
	if (boost == 0)
		return util;

	capacity = capacity_orig_of(cpu);

	if (bg->group[STUNE_TOPAPP].tasks)
		fv_boost = freqvar_st_boost_vector(cpu);

	if (fv_boost > boost)
		boost = fv_boost;

	return util + schedtune_margin(capacity, util, boost);
}

static void update_next_balance(int cpu, struct multi_load *ml)
{
	struct sched_avg *sa = container_of(ml, struct sched_avg, ml);
	struct sched_entity *se = container_of(sa, struct sched_entity, avg);

	if (se->my_q)
		return;

	if (!need_ontime_migration_trigger(cpu, task_of(se)))
		return;

	/*
	 * Update the next_balance of this cpu because tick is most likely
	 * to occur first in currently running cpu.
	 */
	cpu_rq(smp_processor_id())->next_balance = jiffies;
}

/* declare extern function from cfs */
extern u64 decay_load(u64 val, u64 n);
static u32 __accumulate_pelt_segments(u64 periods, u32 d1, u32 d3)
{
	u32 c1, c2, c3 = d3;

	c1 = decay_load((u64)d1, periods);
	c2 = LOAD_AVG_MAX - decay_load(LOAD_AVG_MAX, periods) - 1024;

	return c1 + c2 + c3;
}

/*
 * Below 3 functions have same sequence to update load.
 *  - __update_task_runnable,
 *  - __update_task_util,
 *  - __update_cpu_util
 *
 * step 1: decay the load for the elapsed periods.
 * step 2: accumulate load if the condition is met.
 *	    - runnable : task weight is not 0
 *	    - util : task or cpu is running
 * step 3: update load avg with load sum
 */
static void
__update_task_runnable(struct multi_load *ml, u64 periods, u32 contrib,
		unsigned long scale_cpu, int weight, int cpu)
{
	if (periods)
		ml->runnable_sum = decay_load((u64)(ml->runnable_sum), periods);

	if (weight)
		ml->runnable_sum += contrib * scale_cpu;

	if (!periods)
		return;

	ml->runnable_avg = div_u64(ml->runnable_sum, LOAD_AVG_MAX - 1024 + ml->period_contrib);
	update_next_balance(cpu, ml);
}

static void
__update_task_util(struct multi_load *ml, u64 periods, u32 contrib,
		unsigned long scale_cpu, int running)
{
	if (periods)
		ml->util_sum = decay_load((u64)(ml->util_sum), periods);

	if (running)
		ml->util_sum += contrib * scale_cpu;

	if (!periods)
		return;

	ml->util_avg = ml->util_sum / (LOAD_AVG_MAX - 1024 + ml->period_contrib);
}

static void
__update_cpu_util(struct multi_load *ml, u64 periods, u32 contrib,
		unsigned long scale_cpu, int running, struct cfs_rq *cfs_rq)
{
	if (periods) {
		ml->util_sum = decay_load((u64)(ml->util_sum), periods);
		ml->util_sum_s = decay_load((u64)(ml->util_sum_s), periods);
	}

	if (running) {
		if (get_sse(cfs_rq->curr))
			ml->util_sum_s += contrib * scale_cpu;
		else
			ml->util_sum += contrib * scale_cpu;
	}

	if (!periods)
		return;

	ml->util_avg = ml->util_sum / (LOAD_AVG_MAX - 1024 + ml->period_contrib);
	ml->util_avg_s = ml->util_sum_s / (LOAD_AVG_MAX - 1024 + ml->period_contrib);
}

static void
trace_multi_load(struct multi_load *ml, struct cfs_rq *cfs_rq, struct sched_avg *avg)
{
	struct rq *rq;
	struct sched_entity *se;

	if (cfs_rq) {
		rq = cfs_rq->rq;

		/* trace only cpu root cfs_rq */
		if (&rq->cfs == cfs_rq)
			trace_ems_multi_load_cpu(cpu_of(rq), ml->util_avg, ml->util_avg_s);
	} else {
		se = container_of(avg, struct sched_entity, avg);
		if (!se->my_q)
			trace_ems_multi_load_task(task_of(se), ml->runnable_avg, ml->util_avg);
	}
}

void
update_multi_load(u64 delta, int cpu, struct sched_avg *sa,
		unsigned long weight, int running, struct cfs_rq *cfs_rq)
{
	struct multi_load *ml = &sa->ml;
	struct sched_entity *se;
	unsigned long scale_freq, scale_cpu;
	u32 contrib = (u32)delta;
	u64 periods;

	/* Obtain scale freq */
	scale_freq = arch_scale_freq_capacity(NULL, cpu);

	/* Obtain scale cpu */
	if (cfs_rq) {
		scale_cpu = running ? capacity_orig_of_sse(cpu, get_sse(cfs_rq->curr)) : 0;
	} else {
		se = container_of(sa, struct sched_entity, avg);
		scale_cpu = capacity_orig_of_sse(cpu, get_sse(se));
	}

	delta += ml->period_contrib;
	periods = delta / 1024; /* A period is 1024us (~1ms) */

	if (periods) {
		delta %= 1024;
		contrib = __accumulate_pelt_segments(periods,
			1024 - ml->period_contrib, delta);
	}

	ml->period_contrib = delta;
	contrib = (contrib * scale_freq) >> SCHED_CAPACITY_SHIFT;

	if (cfs_rq)
		__update_cpu_util(ml, periods, contrib, scale_cpu, running, cfs_rq);
	else {
		__update_task_util(ml, periods, contrib, scale_cpu, running);
		__update_task_runnable(ml, periods, contrib, scale_cpu, weight, cpu);
	}

	if (periods)
		trace_multi_load(ml, cfs_rq, sa);
}

void init_multi_load(struct sched_entity *se)
{
	struct multi_load *ml = &se->avg.ml;

	ml->period_contrib = 1023;

	ml->runnable_sum = current->se.avg.ml.runnable_sum >> 1;
	ml->runnable_avg = current->se.avg.ml.runnable_avg >> 1;

	ml->util_sum = 0;
	ml->util_avg = 0;
	ml->util_sum_s = 0;
	ml->util_avg_s = 0;
}

/**
 * detach_entity_multi_load - detach this entity from its cfs_rq load avg
 * @cfs_rq: cfs_rq to detach from
 * @se: sched_entity to detach
 */
void detach_entity_multi_load(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	if (get_sse(se)) {
		sub_positive(&cfs_rq->avg.ml.util_avg_s, se->avg.ml.util_avg);
		sub_positive(&cfs_rq->avg.ml.util_sum_s, se->avg.ml.util_sum);
	} else {
		sub_positive(&cfs_rq->avg.ml.util_avg, se->avg.ml.util_avg);
		sub_positive(&cfs_rq->avg.ml.util_sum, se->avg.ml.util_sum);
	}
}

/**
 * attach_entity_multi_load - attach this entity to its cfs_rq load avg
 * @cfs_rq: cfs_rq to attach to
 * @se: sched_entity to attach
 */
void attach_entity_multi_load(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	if (get_sse(se)) {
		cfs_rq->avg.ml.util_avg_s += se->avg.ml.util_avg;
		cfs_rq->avg.ml.util_sum_s += se->avg.ml.util_sum;
	} else {
		cfs_rq->avg.ml.util_avg += se->avg.ml.util_avg;
		cfs_rq->avg.ml.util_sum += se->avg.ml.util_sum;
	}
}

/**
 * remove_entity_multi_load - apply removed entity's util average to cfs_rq
 * @cfs_rq : cfs_rq to remove from
 * @se : sched_entity to remove
 */
void remove_entity_multi_load(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	if (get_sse(se))
		atomic_long_add(se->avg.ml.util_avg, &cfs_rq->ml_q.removed_util_avg_s);
	else
		atomic_long_add(se->avg.ml.util_avg, &cfs_rq->ml_q.removed_util_avg);
}

/**
 * apply_removed_multi_load - apply removed entity's util average to cfs_rq
 * @cfs_rq : cfs_rq to update
 */
void apply_removed_multi_load(struct cfs_rq *cfs_rq)
{
	struct multi_load *ml = &cfs_rq->avg.ml;
	long r;

	r = atomic_long_xchg(&cfs_rq->ml_q.removed_util_avg, 0);
	sub_positive(&ml->util_avg, r);
	sub_positive(&ml->util_sum, r * LOAD_AVG_MAX);

	r = atomic_long_xchg(&cfs_rq->ml_q.removed_util_avg_s, 0);
	sub_positive(&ml->util_avg_s, r);
	sub_positive(&ml->util_sum_s, r * LOAD_AVG_MAX);
}

/* Take into account change of utilization of a child task group */
void update_tg_multi_load(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	struct cfs_rq *gcfs_rq = se->my_q;
	unsigned long *cfs_rq_util_avg;
	u32 *cfs_rq_util_sum;
	long delta;

	if (get_sse(se)) {
		cfs_rq_util_avg = &gcfs_rq->avg.ml.util_avg_s;
		cfs_rq_util_sum = &gcfs_rq->avg.ml.util_sum_s;
	} else {
		cfs_rq_util_avg = &gcfs_rq->avg.ml.util_avg;
		cfs_rq_util_sum = &gcfs_rq->avg.ml.util_sum;
	}

	delta = *cfs_rq_util_avg - se->avg.ml.util_avg;

	/* Nothing to update */
	if (!delta)
		return;

	/* Set new sched_entity's utilization */
	se->avg.ml.util_avg = *cfs_rq_util_avg;
	se->avg.ml.util_sum = se->avg.ml.util_avg * LOAD_AVG_MAX;

	/* Update parent cfs_rq utilization */
	add_positive(cfs_rq_util_avg, delta);
	*cfs_rq_util_sum = *cfs_rq_util_avg * LOAD_AVG_MAX;
}

/*
 * When a task is dequeued, its estimated utilization should not be update if
 * its util_avg has not been updated at least once.
 * This flag is used to synchronize util_avg updates with util_est updates.
 * We map this information into the LSB bit of the utilization saved at
 * dequeue time (i.e. util_est.dequeued).
 */

void cfs_se_util_change_multi_load(struct task_struct *p, struct sched_avg *avg)
{
	unsigned int enqueued;

	if (!sched_feat(UTIL_EST))
		return;

	if (!schedtune_util_est_en(p))
		return;

	/* Avoid store if the flag has been already set */
	enqueued = avg->ml.util_est.enqueued;
	if (!(enqueued & UTIL_AVG_UNCHANGED))
		return;

	/* Reset flag to report util_avg has been updated */
	enqueued &= ~UTIL_AVG_UNCHANGED;
	WRITE_ONCE(avg->ml.util_est.enqueued, enqueued);
}

static inline struct util_est* cfs_rq_util_est(struct cfs_rq *cfs_rq, int sse)
{
	return sse ? &cfs_rq->avg.ml.util_est_s : &cfs_rq->avg.ml.util_est;
}

void enqueue_multi_load(struct cfs_rq *cfs_rq,
				    struct task_struct *p)
{
	unsigned int enqueued;
	struct util_est *cfs_rq_ue;

	if (!sched_feat(UTIL_EST))
		return;

	cfs_rq_ue = cfs_rq_util_est(cfs_rq, p->sse);

	/* Update root cfs_rq's estimated utilization */
	enqueued  = cfs_rq_ue->enqueued;
	enqueued += (_ml_task_util_est(p) | UTIL_AVG_UNCHANGED);
	WRITE_ONCE(cfs_rq_ue->enqueued, enqueued);

	/* Update plots for Task and CPU estimated utilization */
	trace_ems_util_est_task(p, &p->se.avg);
	trace_ems_util_est_cpu(cpu_of(cfs_rq->rq), cfs_rq);
}

/*
 * Check if a (signed) value is within a specified (unsigned) margin,
 * based on the observation that:
 *     abs(x) < y := (unsigned)(x + y - 1) < (2 * y - 1)
 *
 * NOTE: this only works when value + maring < INT_MAX.
 */
static inline bool within_margin(int value, int margin)
{
	return ((unsigned int)(value + margin - 1) < (2 * margin - 1));
}

void
dequeue_multi_load(struct cfs_rq *cfs_rq, struct task_struct *p, bool task_sleep)
{
	long last_ewma_diff;
	struct util_est ue, *cfs_rq_ue;

	if (!sched_feat(UTIL_EST))
		return;

	cfs_rq_ue = cfs_rq_util_est(cfs_rq, p->sse);

	/*
	 * Update root cfs_rq's estimated utilization
	 *
	 * If *p is the last task then the root cfs_rq's estimated utilization
	 * of a CPU is 0 by definition.
	 */
	ue.enqueued = 0;
	if (cfs_rq->nr_running) {
		ue.enqueued  = cfs_rq_ue->enqueued;
		ue.enqueued -= min_t(unsigned int, ue.enqueued,
				     (_ml_task_util_est(p) | UTIL_AVG_UNCHANGED));
	}
	WRITE_ONCE(cfs_rq_ue->enqueued, ue.enqueued);

	/* Update plots for CPU's estimated utilization */
	trace_ems_util_est_cpu(cpu_of(cfs_rq->rq), cfs_rq);

	/*
	 * Skip update of task's estimated utilization when the task has not
	 * yet completed an activation, e.g. being migrated.
	 */
	if (!task_sleep)
		return;

	if (!schedtune_util_est_en(p))
		return;

	/*
	 * If the PELT values haven't changed since enqueue time,
	 * skip the util_est update.
	 */
	ue = p->se.avg.ml.util_est;
	if (ue.enqueued & UTIL_AVG_UNCHANGED)
		return;

	/*
	 * Skip update of task's estimated utilization when its EWMA is
	 * already ~1% close to its last activation value.
	 */
	ue.enqueued = (ml_task_util(p) | UTIL_AVG_UNCHANGED);
	last_ewma_diff = ue.enqueued - ue.ewma;
	if (within_margin(last_ewma_diff, capacity_orig_of(task_cpu(p)) / 100))
		return;

	/*
	 * Update Task's estimated utilization
	 *
	 * When *p completes an activation we can consolidate another sample
	 * of the task size. This is done by storing the current PELT value
	 * as ue.enqueued and by using this value to update the Exponential
	 * Weighted Moving Average (EWMA):
	 *
	 *  ewma(t) = w *  task_util(p) + (1-w) * ewma(t-1)
	 *          = w *  task_util(p) +         ewma(t-1)  - w * ewma(t-1)
	 *          = w * (task_util(p) -         ewma(t-1)) +     ewma(t-1)
	 *          = w * (      last_ewma_diff            ) +     ewma(t-1)
	 *          = w * (last_ewma_diff  +  ewma(t-1) / w)
	 *
	 * Where 'w' is the weight of new samples, which is configured to be
	 * 0.25, thus making w=1/4 ( >>= UTIL_EST_WEIGHT_SHIFT)
	 */
	ue.ewma <<= UTIL_EST_WEIGHT_SHIFT;
	ue.ewma  += last_ewma_diff;
	ue.ewma >>= UTIL_EST_WEIGHT_SHIFT;
	WRITE_ONCE(p->se.avg.ml.util_est, ue);

	/* Update plots for Task's estimated utilization */
	trace_ems_util_est_task(p, &p->se.avg);
}

/****************************************************************/
/*		Periodic Active Ratio Tracking			*/
/****************************************************************/
enum {
	PART_POLICY_RECENT = 0,
	PART_POLICY_MAX,
	PART_POLICY_MAX_RECENT_MAX,
	PART_POLICY_LAST,
	PART_POLICY_MAX_RECENT_LAST,
	PART_POLICY_MAX_RECENT_AVG,
	PART_POLICY_INVALID,
};

char *part_policy_name[] = {
	"RECENT",
	"MAX",
	"MAX_RECENT_MAX",
	"LAST",
	"MAX_RECENT_LAST",
	"MAX_RECENT_AVG",
	"INVALID"
};

static __read_mostly unsigned int part_policy_idx = PART_POLICY_MAX_RECENT_LAST;
static __read_mostly u64 period_size = 8 * NSEC_PER_MSEC;
static __read_mostly u64 period_hist_size = 10;
static __read_mostly int high_patten_thres = 700;
static __read_mostly int high_patten_stdev = 200;
static __read_mostly int low_patten_count = 3;
static __read_mostly int low_patten_thres = 1024;
static __read_mostly int low_patten_stdev = 200;

static __read_mostly u64 boost_interval = 16 * NSEC_PER_MSEC;

/********************************************************/
/*		  Helper funcition			*/
/********************************************************/

static inline int inc_hist_idx(int idx)
{
	return (idx + 1) % period_hist_size;
}

static inline void calc_active_ratio_hist(struct part *pa)
{
	int idx;
	int sum = 0, max = 0;
	int p_avg = 0, p_stdev = 0, p_count = 0;
	int patten, diff;

	/* Calculate basic statistics of P.A.R.T */
	for (idx = 0; idx < period_hist_size; idx++) {
		sum += pa->hist[idx];
		max = max(max, pa->hist[idx]);
	}

	pa->active_ratio_avg = sum / period_hist_size;
	pa->active_ratio_max = max;
	pa->active_ratio_est = 0;
	pa->active_ratio_stdev = 0;

	/* Calculate stdev for patten recognition */
	for (idx = 0; idx < period_hist_size; idx += 2) {
		patten = pa->hist[idx] + pa->hist[idx + 1];
		if (patten == 0)
			continue;

		p_avg += patten;
		p_count++;
	}

	if (p_count <= 1) {
		p_avg = 0;
		p_stdev = 0;
		goto out;
	}

	p_avg /= p_count;

	for (idx = 0; idx < period_hist_size; idx += 2) {
		patten = pa->hist[idx] + pa->hist[idx + 1];
		if (patten == 0)
			continue;

		diff = patten - p_avg;
		p_stdev += diff * diff;
	}

	p_stdev /= p_count - 1;
	p_stdev = int_sqrt(p_stdev);

out:
	pa->active_ratio_stdev = p_stdev;
	if (p_count >= low_patten_count &&
			p_avg <= low_patten_thres &&
			p_stdev <= low_patten_stdev)
		pa->active_ratio_est = p_avg / 2;

	trace_ems_cpu_active_ratio_patten(cpu_of(container_of(pa, struct rq, pa)),
			p_count, p_avg, p_stdev);
}

static void update_cpu_active_ratio_hist(struct part *pa, bool full, unsigned int count)
{
	/*
	 * Reflect recent active ratio in the history.
	 */
	pa->hist_idx = inc_hist_idx(pa->hist_idx);
	pa->hist[pa->hist_idx] = pa->active_ratio_recent;

	/*
	 * If count is positive, there are empty/full periods.
	 * These will be reflected in the history.
	 */
	while (count--) {
		pa->hist_idx = inc_hist_idx(pa->hist_idx);
		pa->hist[pa->hist_idx] = full ? SCHED_CAPACITY_SCALE : 0;
	}

	/*
	 * Calculate avg/max active ratio through entire history.
	 */
	calc_active_ratio_hist(pa);
}

static void
__update_cpu_active_ratio(int cpu, struct part *pa, u64 now, int boost)
{
	u64 elapsed = now - pa->period_start;
	unsigned int period_count = 0;

	if (boost) {
		pa->last_boost_time = now;
		return;
	}

	if (pa->last_boost_time &&
	    now > pa->last_boost_time + boost_interval)
		pa->last_boost_time = 0;

	if (pa->running) {
		/*
		 * If 'pa->running' is true, it means that the rq is active
		 * from last_update until now.
		 */
		u64 contributer, remainder;

		/*
		 * If now is in recent period, contributer is from last_updated to now.
		 * Otherwise, it is from last_updated to period_end
		 * and remaining active time will be reflected in the next step.
		 */
		contributer = min(now, pa->period_start + period_size);
		pa->active_sum += contributer - pa->last_updated;
		pa->active_ratio_recent =
			div64_u64(pa->active_sum << SCHED_CAPACITY_SHIFT, period_size);

		/*
		 * If now has passed recent period, calculate full periods and reflect they.
		 */
		period_count = div64_u64_rem(elapsed, period_size, &remainder);
		if (period_count) {
			update_cpu_active_ratio_hist(pa, true, period_count - 1);
			pa->active_sum = remainder;
			pa->active_ratio_recent =
				div64_u64(pa->active_sum << SCHED_CAPACITY_SHIFT, period_size);
		}
	} else {
		/*
		 * If 'pa->running' is false, it means that the rq is idle
		 * from last_update until now.
		 */

		/*
		 * If now has passed recent period, calculate empty periods and reflect they.
		 */
		period_count = div64_u64(elapsed, period_size);
		if (period_count) {
			update_cpu_active_ratio_hist(pa, false, period_count - 1);
			pa->active_ratio_recent = 0;
			pa->active_sum = 0;
		}
	}

	pa->period_start += period_size * period_count;
	pa->last_updated = now;
}

/********************************************************/
/*			External APIs			*/
/********************************************************/
void update_cpu_active_ratio(struct rq *rq, struct task_struct *p, int type)
{
	struct part *pa = &rq->pa;
	int cpu = cpu_of(rq);
	u64 now = sched_clock_cpu(0);

	if (unlikely(pa->period_start == 0))
		return;

	switch (type) {
	/*
	 * 1) Enqueue
	 * This type is called when the rq is switched from idle to running.
	 * In this time, Update the active ratio for the idle interval
	 * and change the state to running.
	 */
	case EMS_PART_ENQUEUE:
		__update_cpu_active_ratio(cpu, pa, now, 0);

		if (rq->nr_running == 0) {
			pa->running = true;
			trace_ems_cpu_active_ratio(cpu, pa, "enqueue");
		}
		break;
	/*
	 * 2) Dequeue
	 * This type is called when the rq is switched from running to idle.
	 * In this time, Update the active ratio for the running interval
	 * and change the state to not-running.
	 */
	case EMS_PART_DEQUEUE:
		__update_cpu_active_ratio(cpu, pa, now, 0);

		if (rq->nr_running == 1) {
			pa->running = false;
			trace_ems_cpu_active_ratio(cpu, pa, "dequeue");
		}
		break;
	/*
	 * 3) Update
	 * This type is called to update the active ratio during rq is running.
	 */
	case EMS_PART_UPDATE:
		__update_cpu_active_ratio(cpu, pa, now, 0);
		trace_ems_cpu_active_ratio(cpu, pa, "update");
		break;

	case EMS_PART_WAKEUP_NEW:
		__update_cpu_active_ratio(cpu, pa, now, 1);
		trace_ems_cpu_active_ratio(cpu, pa, "new task");
		break;
	}
}

void part_cpu_active_ratio(unsigned long *util, unsigned long *max, int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	struct part *pa = &rq->pa;
	unsigned long pelt_max = *max;
	unsigned long pelt_util = *util;
	int util_ratio = *util * SCHED_CAPACITY_SCALE / *max;
	int demand = 0;

	if (unlikely(pa->period_start == 0))
		return;

	if (pa->last_boost_time && util_ratio < pa->active_ratio_boost) {
		*max = SCHED_CAPACITY_SCALE;
		*util = pa->active_ratio_boost;
		return;
	}

	if (util_ratio > pa->active_ratio_limit)
		return;

	if (!pa->running &&
			(pa->active_ratio_avg < high_patten_thres ||
			 pa->active_ratio_stdev > high_patten_stdev)) {
		*util = 0;
		*max = SCHED_CAPACITY_SCALE;
		return;
	}

	update_cpu_active_ratio(rq, NULL, EMS_PART_UPDATE);

	switch (part_policy_idx) {
	case PART_POLICY_RECENT:
		demand = pa->active_ratio_recent;
		break;
	case PART_POLICY_MAX:
		demand = pa->active_ratio_max;
		break;
	case PART_POLICY_MAX_RECENT_MAX:
		demand = max(pa->active_ratio_recent, pa->active_ratio_max);
		break;
	case PART_POLICY_LAST:
		demand = pa->hist[pa->hist_idx];
		break;
	case PART_POLICY_MAX_RECENT_LAST:
		demand = max(pa->active_ratio_recent, pa->hist[pa->hist_idx]);
		break;
	case PART_POLICY_MAX_RECENT_AVG:
		demand = max(pa->active_ratio_recent, pa->active_ratio_avg);
		break;
	}

	*util = max(demand, pa->active_ratio_est);
	*util = min_t(unsigned long, *util, (unsigned long)pa->active_ratio_limit);
	*max = SCHED_CAPACITY_SCALE;

	if (util_ratio > *util) {
		*util = pelt_util;
		*max = pelt_max;
	}

	trace_ems_cpu_active_ratio_util_stat(cpu, *util, (unsigned long)util_ratio);
}

void set_part_period_start(struct rq *rq)
{
	struct part *pa = &rq->pa;
	u64 now;

	if (likely(pa->period_start))
		return;

	now = sched_clock_cpu(0);
	pa->period_start = now;
	pa->last_updated = now;
}

/********************************************************/
/*			  SYSFS				*/
/********************************************************/
static ssize_t show_part_policy(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u. %s\n", part_policy_idx,
			part_policy_name[part_policy_idx]);
}

static ssize_t store_part_policy(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf,
		size_t count)
{
	long input;

	if (!sscanf(buf, "%ld", &input))
		return -EINVAL;

	if (input >= PART_POLICY_INVALID || input < 0)
		return -EINVAL;

	part_policy_idx = input;

	return count;
}

static ssize_t show_part_policy_list(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	ssize_t len = 0;
	int i;

	for (i = 0; i < PART_POLICY_INVALID ; i++)
		len += sprintf(buf + len, "%u. %s\n", i, part_policy_name[i]);

	return len;
}

static ssize_t show_active_ratio_limit(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct part *pa;
	int cpu, len = 0;

	for_each_possible_cpu(cpu) {
		pa = &cpu_rq(cpu)->pa;
		len += sprintf(buf + len, "cpu%d ratio:%3d\n",
				cpu, pa->active_ratio_limit);
	}

	return len;
}

static ssize_t store_active_ratio_limit(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf,
		size_t count)
{
	struct part *pa;
	int cpu, ratio, i;

	if (sscanf(buf, "%d %d", &cpu, &ratio) != 2)
		return -EINVAL;

	/* Check cpu is possible */
	if (!cpumask_test_cpu(cpu, cpu_possible_mask))
		return -EINVAL;

	/* Check ratio isn't outrage */
	if (ratio < 0 || ratio > SCHED_CAPACITY_SCALE)
		return -EINVAL;

	for_each_cpu(i, cpu_coregroup_mask(cpu)) {
		pa = &cpu_rq(i)->pa;
		pa->active_ratio_limit = ratio;
	}

	return count;
}

static ssize_t show_active_ratio_boost(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct part *pa;
	int cpu, len = 0;

	for_each_possible_cpu(cpu) {
		pa = &cpu_rq(cpu)->pa;
		len += sprintf(buf + len, "cpu%d ratio:%3d\n",
				cpu, pa->active_ratio_boost);
	}

	return len;
}

static ssize_t store_active_ratio_boost(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf,
		size_t count)
{
	struct part *pa;
	int cpu, ratio, i;

	if (sscanf(buf, "%d %d", &cpu, &ratio) != 2)
		return -EINVAL;

	/* Check cpu is possible */
	if (!cpumask_test_cpu(cpu, cpu_possible_mask))
		return -EINVAL;

	/* Check ratio isn't outrage */
	if (ratio < 0 || ratio > SCHED_CAPACITY_SCALE)
		return -EINVAL;

	for_each_cpu(i, cpu_coregroup_mask(cpu)) {
		pa = &cpu_rq(i)->pa;
		pa->active_ratio_boost = ratio;
	}

	return count;
}

#define show_node_function(_name)					\
static ssize_t show_##_name(struct kobject *kobj,			\
		struct kobj_attribute *attr, char *buf)			\
{									\
	return sprintf(buf, "%d\n", _name);				\
}

#define store_node_function(_name, _max)				\
static ssize_t store_##_name(struct kobject *kobj,			\
		struct kobj_attribute *attr, const char *buf,		\
		size_t count)						\
{									\
	unsigned int input;						\
									\
	if (!sscanf(buf, "%u", &input))					\
		return -EINVAL;						\
									\
	if (input > _max)						\
		return -EINVAL;						\
									\
	_name = input;							\
									\
	return count;							\
}

show_node_function(high_patten_thres);
store_node_function(high_patten_thres, SCHED_CAPACITY_SCALE);
show_node_function(high_patten_stdev);
store_node_function(high_patten_stdev, SCHED_CAPACITY_SCALE);
show_node_function(low_patten_count);
store_node_function(low_patten_count, (period_size / 2));
show_node_function(low_patten_thres);
store_node_function(low_patten_thres, (SCHED_CAPACITY_SCALE * 2));
show_node_function(low_patten_stdev);
store_node_function(low_patten_stdev, SCHED_CAPACITY_SCALE);

static struct kobj_attribute _policy =
__ATTR(policy, 0644, show_part_policy, store_part_policy);
static struct kobj_attribute _policy_list =
__ATTR(policy_list, 0444, show_part_policy_list, NULL);
static struct kobj_attribute _high_patten_thres =
__ATTR(high_patten_thres, 0644, show_high_patten_thres, store_high_patten_thres);
static struct kobj_attribute _high_patten_stdev =
__ATTR(high_patten_stdev, 0644, show_high_patten_stdev, store_high_patten_stdev);
static struct kobj_attribute _low_patten_count =
__ATTR(low_patten_count, 0644, show_low_patten_count, store_low_patten_count);
static struct kobj_attribute _low_patten_thres =
__ATTR(low_patten_thres, 0644, show_low_patten_thres, store_low_patten_thres);
static struct kobj_attribute _low_patten_stdev =
__ATTR(low_patten_stdev, 0644, show_low_patten_stdev, store_low_patten_stdev);
static struct kobj_attribute _active_ratio_limit =
__ATTR(active_ratio_limit, 0644, show_active_ratio_limit, store_active_ratio_limit);
static struct kobj_attribute _active_ratio_boost =
__ATTR(active_ratio_boost, 0644, show_active_ratio_boost, store_active_ratio_boost);

static struct attribute *attrs[] = {
	&_policy.attr,
	&_policy_list.attr,
	&_high_patten_thres.attr,
	&_high_patten_stdev.attr,
	&_low_patten_count.attr,
	&_low_patten_thres.attr,
	&_low_patten_stdev.attr,
	&_active_ratio_limit.attr,
	&_active_ratio_boost.attr,
	NULL,
};

static const struct attribute_group attr_group = {
	.attrs = attrs,
};

static int __init init_part_sysfs(void)
{
	struct kobject *kobj;

	kobj = kobject_create_and_add("part", ems_kobj);
	if (!kobj)
		return -EINVAL;

	if (sysfs_create_group(kobj, &attr_group))
		return -EINVAL;

	return 0;
}
late_initcall(init_part_sysfs);

static int __init parse_part(void)
{
	struct device_node *dn, *coregroup;
	char name[15];
	int cpu, cnt = 0, limit = -1, boost = -1;

	dn = of_find_node_by_path("/cpus/ems/part");
	if (!dn)
		return 0;

	for_each_possible_cpu(cpu) {
		struct part *pa = &cpu_rq(cpu)->pa;

		if (cpu != cpumask_first(cpu_coregroup_mask(cpu)))
			goto skip_parse;

		limit = -1;
		boost = -1;

		snprintf(name, sizeof(name), "coregroup%d", cnt++);
		coregroup = of_get_child_by_name(dn, name);
		if (!coregroup)
			continue;

		of_property_read_s32(coregroup, "active-ratio-limit", &limit);
		of_property_read_s32(coregroup, "active-ratio-boost", &boost);

skip_parse:
		if (limit >= 0)
			pa->active_ratio_limit = SCHED_CAPACITY_SCALE * limit / 100;

		if (boost >= 0)
			pa->active_ratio_boost = SCHED_CAPACITY_SCALE * boost / 100;
	}

	return 0;
}
core_initcall(parse_part);

void __init init_part(void)
{
	int cpu, idx;

	for_each_possible_cpu(cpu) {
		struct part *pa = &cpu_rq(cpu)->pa;

		/* Set by default value */
		pa->running = false;
		pa->active_sum = 0;
		pa->active_ratio_recent = 0;
		pa->hist_idx = 0;
		for (idx = 0; idx < PART_HIST_SIZE_MAX; idx++)
			pa->hist[idx] = 0;

		pa->period_start = 0;
		pa->last_updated = 0;
	}
}
