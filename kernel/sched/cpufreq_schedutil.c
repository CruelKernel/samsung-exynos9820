/*
 * CPUFreq governor based on scheduler-provided CPU utilization data.
 *
 * Copyright (C) 2016, Intel Corporation
 * Author: Rafael J. Wysocki <rafael.j.wysocki@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/cpufreq.h>
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
#include <linux/slab.h>
#include <linux/cpu_pm.h>
#include <linux/ems.h>

#include <trace/events/power.h>

#include "sched.h"
#include "tune.h"
#include "ems/ems.h"

#ifdef CONFIG_SCHED_KAIR_GLUE
#include <linux/kair.h>
/**
 * 2nd argument of kair_obj_creator() experimentally decided by KAIR client
 * itself, which represents how much variant the random variable registered to
 * the KAIR instance can behave at most, in terms of referencing d2u_decl_cmtpdf
 * table(maximum index of d2u_decl_cmtpdf table).
 **/
#define UTILAVG_KAIR_VARIANCE	16
DECLARE_KAIRISTICS(cpufreq, 32, 25, 24, 25);
#endif

unsigned long boosted_cpu_util(int cpu, unsigned long other_util);

#define SUGOV_KTHREAD_PRIORITY	50

struct sugov_tunables {
	struct gov_attr_set attr_set;
	unsigned int up_rate_limit_us;
	unsigned int down_rate_limit_us;
#ifdef CONFIG_SCHED_KAIR_GLUE
	bool fb_legacy;
#endif
};

struct sugov_policy {
	struct cpufreq_policy *policy;

	struct sugov_tunables *tunables;
	struct list_head tunables_hook;

	raw_spinlock_t update_lock;  /* For shared policies */
	u64 last_freq_update_time;
	s64 min_rate_limit_ns;
	s64 up_rate_delay_ns;
	s64 down_rate_delay_ns;
	unsigned int next_freq;
	unsigned int cached_raw_freq;

	/* The next fields are only needed if fast switch cannot be used. */
	struct irq_work irq_work;
	struct kthread_work work;
	struct mutex work_lock;
	struct kthread_worker worker;
	struct task_struct *thread;
	bool work_in_progress;

	bool need_freq_update;
#ifdef CONFIG_SCHED_KAIR_GLUE
	bool be_stochastic;
#endif
};

struct sugov_cpu {
	struct update_util_data update_util;
	struct sugov_policy *sg_policy;
	unsigned int cpu;

	bool iowait_boost_pending;
	unsigned int iowait_boost;
	unsigned int iowait_boost_max;
	u64 last_update;

#ifdef CONFIG_SCHED_KAIR_GLUE
	/**
	 * KAIR instance which should be referenced in percpu manner,
	 * and data accordingly to handle the target job intensity.
	 **/
	struct kair_class *util_vessel;
	unsigned long cached_util;
#endif
	/* The fields below are only needed when sharing a policy. */
	unsigned long util;
	unsigned long max;
	unsigned int flags;

	/* The field below is for single-CPU policies only. */
#ifdef CONFIG_NO_HZ_COMMON
	unsigned long saved_idle_calls;
#endif
};

static DEFINE_PER_CPU(struct sugov_cpu, sugov_cpu);

/******************* exynos specific function *******************/
#define DEFAULT_EXPIRED_TIME	70
struct sugov_exynos {
	/* for slack timer */
	unsigned long min;
	int enabled;
	bool started;
	int expired_time;
	struct timer_list timer;

	/* pm_qos_class */
	int qos_min_class;
};
static DEFINE_PER_CPU(struct sugov_exynos, sugov_exynos);
static void sugov_stop_slack(int cpu);
static void sugov_start_slack(int cpu);
static void sugov_update_min(struct cpufreq_policy *policy);

/************************ Governor internals ***********************/
struct sugov_policy_list {
	struct list_head list;
	struct sugov_policy *sg_policy;
	struct cpumask cpus;
};
static LIST_HEAD(sugov_policy_list);

static inline struct sugov_policy_list
	*find_sg_pol_list(struct cpufreq_policy *policy)
{
	struct sugov_policy_list *sg_pol_list;

	list_for_each_entry(sg_pol_list, &sugov_policy_list, list)
		if (cpumask_test_cpu(policy->cpu, &sg_pol_list->cpus))
			return sg_pol_list;

	return NULL;
}

static struct sugov_policy
	*sugov_restore_policy(struct cpufreq_policy *policy)
{
	struct sugov_policy_list *sg_pol_list =
			sg_pol_list = find_sg_pol_list(policy);

	if (!sg_pol_list)
		return NULL;

	pr_info("Restore sg_policy(%d) from policy_list\(%x)n",
		policy->cpu,
		*(unsigned int *)cpumask_bits(&sg_pol_list->cpus));

	return sg_pol_list->sg_policy;
}

static int sugov_save_policy(struct sugov_policy *sg_policy)
{
	struct sugov_policy_list *sg_pol_list;
	struct cpufreq_policy *policy = sg_policy->policy;

	if (unlikely(!sg_policy))
		return 0;

	sg_pol_list = find_sg_pol_list(policy);
	if (sg_pol_list) {
		pr_info("Already saved sg_policy(%d) to policy_list\(%x)n",
			policy->cpu,
			*(unsigned int *)cpumask_bits(&sg_pol_list->cpus));
		return 1;
	}

	/* Back up sugov_policy to list */
	sg_pol_list = kzalloc(sizeof(struct sugov_policy_list), GFP_KERNEL);
	if (!sg_pol_list)
		return 0;

	cpumask_copy(&sg_pol_list->cpus, policy->related_cpus);
	sg_pol_list->sg_policy = sg_policy;
	list_add(&sg_pol_list->list, &sugov_policy_list);

	pr_info("Save sg_policy(%d) to policy_list(%x)\n",
		policy->cpu,
		*(unsigned int *)cpumask_bits(&sg_pol_list->cpus));

	return 1;
}

static bool sugov_should_update_freq(struct sugov_policy *sg_policy, u64 time)
{
	s64 delta_ns;

	/*
	 * Since cpufreq_update_util() is called with rq->lock held for
	 * the @target_cpu, our per-cpu data is fully serialized.
	 *
	 * However, drivers cannot in general deal with cross-cpu
	 * requests, so while get_next_freq() will work, our
	 * sugov_update_commit() call may not for the fast switching platforms.
	 *
	 * Hence stop here for remote requests if they aren't supported
	 * by the hardware, as calculating the frequency is pointless if
	 * we cannot in fact act on it.
	 *
	 * For the slow switching platforms, the kthread is always scheduled on
	 * the right set of CPUs and any CPU can find the next frequency and
	 * schedule the kthread.
	 */
	if (sg_policy->policy->fast_switch_enabled &&
	    !cpufreq_can_do_remote_dvfs(sg_policy->policy))
		return false;

	if (sg_policy->work_in_progress)
		return false;

	if (unlikely(sg_policy->need_freq_update)) {
		sg_policy->need_freq_update = false;
		/*
		 * This happens when limits change, so forget the previous
		 * next_freq value and force an update.
		 */
		sg_policy->next_freq = UINT_MAX;
		return true;
	}

	/* No need to recalculate next freq for min_rate_limit_us
	 * at least. However we might still decide to further rate
	 * limit once frequency change direction is decided, according
	 * to the separate rate limits.
	 */

	delta_ns = time - sg_policy->last_freq_update_time;
	return delta_ns >= sg_policy->min_rate_limit_ns;
}

static bool sugov_up_down_rate_limit(struct sugov_policy *sg_policy, u64 time,
				     unsigned int next_freq)
{
	s64 delta_ns;

	delta_ns = time - sg_policy->last_freq_update_time;

	if (next_freq > sg_policy->next_freq &&
	    delta_ns < sg_policy->up_rate_delay_ns)
			return true;

	if (next_freq < sg_policy->next_freq &&
	    delta_ns < sg_policy->down_rate_delay_ns)
			return true;

	return false;
}

static int sugov_select_scaling_cpu(void)
{
	int cpu, candidate = -1;
	unsigned long rt, util, min = INT_MAX;
	cpumask_t mask;

	cpumask_clear(&mask);
	cpumask_and(&mask, cpu_coregroup_mask(0), cpu_active_mask);

	/* Idle core of the boot cluster is selected to scaling cpu */
	for_each_cpu(cpu, &mask) {
		rt = sched_get_rt_rq_util(cpu);
#ifdef CONFIG_SCHED_EMS
		util = ml_boosted_cpu_util(cpu) + rt;
#else
		util = boosted_cpu_util(cpu, rt);
#endif
		if (util < min) {
			min = util;
			candidate = cpu;
		}
	}
	return candidate;
}

static void sugov_update_commit(struct sugov_policy *sg_policy, u64 time,
				unsigned int next_freq)
{
	struct cpufreq_policy *policy = sg_policy->policy;
	int cpu;

	if (sg_policy->next_freq == next_freq)
		return;

	if (sugov_up_down_rate_limit(sg_policy, time, next_freq))
		return;

	sg_policy->next_freq = next_freq;
	sg_policy->last_freq_update_time = time;

	if (policy->fast_switch_enabled) {
		next_freq = cpufreq_driver_fast_switch(policy, next_freq);
		if (!next_freq)
			return;

		policy->cur = next_freq;
		trace_cpu_frequency(next_freq, smp_processor_id());
	} else {
		cpu = sugov_select_scaling_cpu();
		if (cpu < 0)
			return;

		sg_policy->work_in_progress = true;
		irq_work_queue_on(&sg_policy->irq_work, cpu);
	}
}

#ifdef CONFIG_FREQVAR_TUNE
unsigned long freqvar_boost_vector(int cpu, unsigned long util);
#else
static inline unsigned long freqvar_boost_vector(int cpu, unsigned long util)
{
	return util;
}
#endif

/**
 * get_next_freq - Compute a new frequency for a given cpufreq policy.
 * @sg_policy: schedutil policy object to compute the new frequency for.
 * @util: Current CPU utilization.
 * @max: CPU capacity.
 *
 * If the utilization is frequency-invariant, choose the new frequency to be
 * proportional to it, that is
 *
 * next_freq = C * max_freq * util / max
 *
 * Otherwise, approximate the would-be frequency-invariant utilization by
 * util_raw * (curr_freq / max_freq) which leads to
 *
 * next_freq = C * curr_freq * util_raw / max
 *
 * Take C = 1.25 for the frequency tipping point at (util / max) = 0.8.
 *
 * The lowest driver-supported frequency which is equal or greater than the raw
 * next_freq (as calculated above) is returned, subject to policy min/max and
 * cpufreq driver limitations.
 */
static unsigned int get_next_freq(struct sugov_policy *sg_policy,
				  unsigned long util, unsigned long max)
{
	struct cpufreq_policy *policy = sg_policy->policy;
	unsigned int freq = arch_scale_freq_invariant() ?
				policy->max : policy->cur;
#ifdef CONFIG_SCHED_KAIR_GLUE
	struct sugov_cpu *sg_cpu;
	struct kair_class *vessel;
	unsigned int delta_max, delta_min;
	int util_delta;
	unsigned int legacy_freq;

#ifdef KAIR_CLUSTER_TRAVERSING
	unsigned int each;
	unsigned int sigma_cpu = policy->cpu;
	randomness most_rand = 0;
#endif
	int cur_rand = KAIR_DIVERGING;
	RV_DECLARE(rv);
#endif

	freq = (freq + (freq >> 2)) * util / max;

#ifdef CONFIG_SCHED_KAIR_GLUE
	legacy_freq = freq;

	if (sg_policy->tunables->fb_legacy)
		goto skip_betting;

#ifndef KAIR_CLUSTER_TRAVERSING
	sg_cpu = &per_cpu(sugov_cpu, policy->cpu);
	vessel = sg_cpu->util_vessel;

	if (!vessel)
		goto skip_betting;

	cur_rand = vessel->job_inferer(vessel);
	if (cur_rand == KAIR_DIVERGING)
		goto skip_betting;
#else
	for_each_cpu(each, policy->cpus) {
		sg_cpu = &per_cpu(sugov_cpu, each);

		vessel = sg_cpu->util_vessel;
		if (vessel) {
			cur_rand = vessel->job_inferer(vessel);
			if (cur_rand == KAIR_DIVERGING)
				goto skip_betting;
			else {
				if (cur_rand > (int)most_rand) {
					most_rand = (randomness)cur_rand;
					sigma_cpu = each;
				}
			}
		} else
			goto skip_betting;
	}

	sg_cpu = &per_cpu(sugov_cpu, sigma_cpu);
	vessel = sg_cpu->util_vessel;
#endif
	util_delta = sg_cpu->util - sg_cpu->cached_util;
	delta_max  = sg_cpu->max - sg_cpu->cached_util;
	delta_min  = sg_cpu->cached_util;

	RV_SET(rv, util_delta, delta_max, delta_min);
	freq = vessel->cap_bettor(vessel, &rv, freq);

skip_betting:
	trace_sugov_kair_freq(policy->cpu, util, max, cur_rand, legacy_freq, freq);
#endif

	if (freq == sg_policy->cached_raw_freq && sg_policy->next_freq != UINT_MAX)
		return sg_policy->next_freq;
	sg_policy->cached_raw_freq = freq;
	freq = cpufreq_driver_resolve_freq(policy, freq);
	trace_cpu_frequency_sugov(freq, util, policy->cpu);

	return freq;
}

static void sugov_get_util(unsigned long *util, unsigned long *max, int cpu)
{
	unsigned long max_cap, rt;

	max_cap = arch_scale_cpu_capacity(NULL, cpu);

	rt = sched_get_rt_rq_util(cpu);

#ifdef CONFIG_SCHED_EMS
	*util = ml_boosted_cpu_util(cpu) + rt;
#else
	*util = boosted_cpu_util(cpu, rt);
#endif
	*util = freqvar_boost_vector(cpu, *util);
	*util = min(*util, max_cap);
	*max = max_cap;

#ifdef CONFIG_SCHED_EMS
	part_cpu_active_ratio(util, max, cpu);
#endif

}

#ifdef CONFIG_SCHED_KAIR_GLUE
static inline void sugov_util_collapse(struct sugov_cpu *sg_cpu)
{
	struct kair_class *vessel = sg_cpu->util_vessel;
	int util_delta = min(sg_cpu->max, sg_cpu->util) - sg_cpu->cached_util;
	unsigned int delta_max = sg_cpu->max - sg_cpu->cached_util;
	unsigned int delta_min = sg_cpu->cached_util;

	RV_DECLARE(job);

	if (vessel) {
		RV_SET(job, util_delta, delta_max, delta_min);
		vessel->job_learner(vessel, &job);
	}
}
#endif

static void sugov_set_iowait_boost(struct sugov_cpu *sg_cpu, u64 time,
				   unsigned int flags)
{
	if (flags & SCHED_CPUFREQ_IOWAIT) {
		if (sg_cpu->iowait_boost_pending)
			return;

		sg_cpu->iowait_boost_pending = true;

		if (sg_cpu->iowait_boost) {
			sg_cpu->iowait_boost <<= 1;
			if (sg_cpu->iowait_boost > sg_cpu->iowait_boost_max)
				sg_cpu->iowait_boost = sg_cpu->iowait_boost_max;
		} else {
			sg_cpu->iowait_boost = sg_cpu->sg_policy->policy->min;
		}
	} else if (sg_cpu->iowait_boost) {
		s64 delta_ns = time - sg_cpu->last_update;

		/* Clear iowait_boost if the CPU apprears to have been idle. */
		if (delta_ns > TICK_NSEC) {
			sg_cpu->iowait_boost = 0;
			sg_cpu->iowait_boost_pending = false;
		}
	}
}

static void sugov_iowait_boost(struct sugov_cpu *sg_cpu, unsigned long *util,
			       unsigned long *max)
{
	unsigned int boost_util, boost_max;

	if (!sg_cpu->iowait_boost)
		return;

	if (sg_cpu->iowait_boost_pending) {
		sg_cpu->iowait_boost_pending = false;
	} else {
		sg_cpu->iowait_boost >>= 1;
		if (sg_cpu->iowait_boost < sg_cpu->sg_policy->policy->min) {
			sg_cpu->iowait_boost = 0;
			return;
		}
	}

	boost_util = sg_cpu->iowait_boost;
	boost_max = sg_cpu->iowait_boost_max;

	if (*util * boost_max < *max * boost_util) {
		*util = boost_util;
		*max = boost_max;
	}
}

static unsigned int sugov_next_freq_shared(struct sugov_cpu *sg_cpu, u64 time)
{
	struct sugov_policy *sg_policy = sg_cpu->sg_policy;
	struct cpufreq_policy *policy = sg_policy->policy;
	unsigned long util = 0, max = 1;
	unsigned int j;

	for_each_cpu_and(j, policy->related_cpus, cpu_online_mask) {
		struct sugov_cpu *j_sg_cpu = &per_cpu(sugov_cpu, j);
		unsigned long j_util, j_max;
		s64 delta_ns;

		/*
		 * If the CPU utilization was last updated before the previous
		 * frequency update and the time elapsed between the last update
		 * of the CPU utilization and the last frequency update is long
		 * enough, don't take the CPU into account as it probably is
		 * idle now (and clear iowait_boost for it).
		 */
		delta_ns = time - j_sg_cpu->last_update;
		if (delta_ns > TICK_NSEC && idle_cpu(j)) {
			j_sg_cpu->iowait_boost = 0;
			j_sg_cpu->iowait_boost_pending = false;
			continue;
		}
		if (j_sg_cpu->flags & SCHED_CPUFREQ_DL)
			return policy->cpuinfo.max_freq;

		j_util = j_sg_cpu->util;
		j_max = j_sg_cpu->max;
		if (j_util * max > j_max * util) {
			util = j_util;
			max = j_max;
		}

		sugov_iowait_boost(j_sg_cpu, &util, &max);
	}

	return get_next_freq(sg_policy, util, max);
}

static void sugov_update_shared(struct update_util_data *hook, u64 time,
				unsigned int flags)
{
	struct sugov_cpu *sg_cpu = container_of(hook, struct sugov_cpu, update_util);
	struct sugov_policy *sg_policy = sg_cpu->sg_policy;
	unsigned long util, max;
	unsigned int next_f;

	sugov_get_util(&util, &max, sg_cpu->cpu);

	raw_spin_lock(&sg_policy->update_lock);

#ifdef CONFIG_SCHED_KAIR_GLUE
	sg_cpu->cached_util = min(max, sg_cpu->max ?
				mult_frac(sg_cpu->util, max, sg_cpu->max) : sg_cpu->util);
#endif
	sg_cpu->util = util;
	sg_cpu->max = max;
	sg_cpu->flags = flags;
#ifdef CONFIG_SCHED_KAIR_GLUE
	sugov_util_collapse(sg_cpu);
#endif
	sugov_set_iowait_boost(sg_cpu, time, flags);
	sg_cpu->last_update = time;

	if (sugov_should_update_freq(sg_policy, time)) {
		if (flags & SCHED_CPUFREQ_DL)
			next_f = sg_policy->policy->cpuinfo.max_freq;
		else
			next_f = sugov_next_freq_shared(sg_cpu, time);

		sugov_update_commit(sg_policy, time, next_f);
	}

	raw_spin_unlock(&sg_policy->update_lock);
}

static void sugov_work(struct kthread_work *work)
{
	struct sugov_policy *sg_policy = container_of(work, struct sugov_policy, work);

	down_write(&sg_policy->policy->rwsem);
	mutex_lock(&sg_policy->work_lock);
	__cpufreq_driver_target(sg_policy->policy, sg_policy->next_freq,
				CPUFREQ_RELATION_L);
	mutex_unlock(&sg_policy->work_lock);
	up_write(&sg_policy->policy->rwsem);

	sg_policy->work_in_progress = false;
}

static void sugov_irq_work(struct irq_work *irq_work)
{
	struct sugov_policy *sg_policy;

	sg_policy = container_of(irq_work, struct sugov_policy, irq_work);

	/*
	 * For RT and deadline tasks, the schedutil governor shoots the
	 * frequency to maximum. Special care must be taken to ensure that this
	 * kthread doesn't result in the same behavior.
	 *
	 * This is (mostly) guaranteed by the work_in_progress flag. The flag is
	 * updated only at the end of the sugov_work() function and before that
	 * the schedutil governor rejects all other frequency scaling requests.
	 *
	 * There is a very rare case though, where the RT thread yields right
	 * after the work_in_progress flag is cleared. The effects of that are
	 * neglected for now.
	 */
	kthread_queue_work(&sg_policy->worker, &sg_policy->work);
}

/************************ Governor externals ***********************/
static void update_min_rate_limit_ns(struct sugov_policy *sg_policy);
void sugov_update_rate_limit_us(struct cpufreq_policy *policy,
			int up_rate_limit_ms, int down_rate_limit_ms)
{
	struct sugov_policy *sg_policy;
	struct sugov_tunables *tunables;

	sg_policy = policy->governor_data;
	if (!sg_policy)
		return;

	tunables = sg_policy->tunables;
	if (!tunables)
		return;

	tunables->up_rate_limit_us = (unsigned int)(up_rate_limit_ms * USEC_PER_MSEC);
	tunables->down_rate_limit_us = (unsigned int)(down_rate_limit_ms * USEC_PER_MSEC);

	sg_policy->up_rate_delay_ns = up_rate_limit_ms * NSEC_PER_MSEC;
	sg_policy->down_rate_delay_ns = down_rate_limit_ms * NSEC_PER_MSEC;

	update_min_rate_limit_ns(sg_policy);
}

int sugov_sysfs_add_attr(struct cpufreq_policy *policy, const struct attribute *attr)
{
	struct sugov_policy *sg_policy;
	struct sugov_tunables *tunables;

	sg_policy = policy->governor_data;
	if (!sg_policy)
		return -ENODEV;

	tunables = sg_policy->tunables;
	if (!tunables)
		return -ENODEV;

	return sysfs_create_file(&tunables->attr_set.kobj, attr);
}

struct cpufreq_policy *sugov_get_attr_policy(struct gov_attr_set *attr_set)
{
	struct sugov_policy *sg_policy = list_first_entry(&attr_set->policy_list,
						typeof(*sg_policy), tunables_hook);
	return sg_policy->policy;
}

/************************** sysfs interface ************************/

static struct sugov_tunables *global_tunables;
static DEFINE_MUTEX(global_tunables_lock);

static inline struct sugov_tunables *to_sugov_tunables(struct gov_attr_set *attr_set)
{
	return container_of(attr_set, struct sugov_tunables, attr_set);
}

static DEFINE_MUTEX(min_rate_lock);

static void update_min_rate_limit_ns(struct sugov_policy *sg_policy)
{
	mutex_lock(&min_rate_lock);
	sg_policy->min_rate_limit_ns = min(sg_policy->up_rate_delay_ns,
					   sg_policy->down_rate_delay_ns);
	mutex_unlock(&min_rate_lock);
}

static ssize_t up_rate_limit_us_show(struct gov_attr_set *attr_set, char *buf)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);

	return sprintf(buf, "%u\n", tunables->up_rate_limit_us);
}

static ssize_t down_rate_limit_us_show(struct gov_attr_set *attr_set, char *buf)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);

	return sprintf(buf, "%u\n", tunables->down_rate_limit_us);
}

static ssize_t up_rate_limit_us_store(struct gov_attr_set *attr_set,
				      const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	struct sugov_policy *sg_policy;
	unsigned int rate_limit_us;

	if (kstrtouint(buf, 10, &rate_limit_us))
		return -EINVAL;

	tunables->up_rate_limit_us = rate_limit_us;

	list_for_each_entry(sg_policy, &attr_set->policy_list, tunables_hook) {
		sg_policy->up_rate_delay_ns = rate_limit_us * NSEC_PER_USEC;
		update_min_rate_limit_ns(sg_policy);
	}

	return count;
}

static ssize_t down_rate_limit_us_store(struct gov_attr_set *attr_set,
					const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	struct sugov_policy *sg_policy;
	unsigned int rate_limit_us;

	if (kstrtouint(buf, 10, &rate_limit_us))
		return -EINVAL;

	tunables->down_rate_limit_us = rate_limit_us;

	list_for_each_entry(sg_policy, &attr_set->policy_list, tunables_hook) {
		sg_policy->down_rate_delay_ns = rate_limit_us * NSEC_PER_USEC;
		update_min_rate_limit_ns(sg_policy);
	}

	return count;
}

#ifdef CONFIG_SCHED_KAIR_GLUE
static ssize_t fb_legacy_show(struct gov_attr_set *attr_set, char *buf)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);

	return scnprintf(buf, PAGE_SIZE, "%u\n", tunables->fb_legacy);
}

static ssize_t fb_legacy_store(struct gov_attr_set *attr_set, const char *buf,
			       size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);

	if (kstrtobool(buf, &tunables->fb_legacy))
		return -EINVAL;

	return count;
}
#endif

static struct governor_attr up_rate_limit_us = __ATTR_RW(up_rate_limit_us);
static struct governor_attr down_rate_limit_us = __ATTR_RW(down_rate_limit_us);
#ifdef CONFIG_SCHED_KAIR_GLUE
static struct governor_attr fb_legacy = __ATTR_RW(fb_legacy);
#endif

static struct attribute *sugov_attributes[] = {
	&up_rate_limit_us.attr,
	&down_rate_limit_us.attr,
#ifdef CONFIG_SCHED_KAIR_GLUE
	&fb_legacy.attr,
#endif
	NULL
};

static struct kobj_type sugov_tunables_ktype = {
	.default_attrs = sugov_attributes,
	.sysfs_ops = &governor_sysfs_ops,
};

/********************** cpufreq governor interface *********************/

static struct cpufreq_governor schedutil_gov;

static struct sugov_policy *sugov_policy_alloc(struct cpufreq_policy *policy)
{
	struct sugov_policy *sg_policy;

	sg_policy = kzalloc(sizeof(*sg_policy), GFP_KERNEL);
	if (!sg_policy)
		return NULL;

	sg_policy->policy = policy;
	raw_spin_lock_init(&sg_policy->update_lock);
	return sg_policy;
}

static void sugov_policy_free(struct sugov_policy *sg_policy)
{
	kfree(sg_policy);
}

static int sugov_kthread_create(struct sugov_policy *sg_policy)
{
	struct task_struct *thread;
	struct sched_param param = { .sched_priority = MAX_USER_RT_PRIO / 2 };
	struct cpufreq_policy *policy = sg_policy->policy;
	int ret;

	/* kthread only required for slow path */
	if (policy->fast_switch_enabled)
		return 0;

	kthread_init_work(&sg_policy->work, sugov_work);
	kthread_init_worker(&sg_policy->worker);
	thread = kthread_create(kthread_worker_fn, &sg_policy->worker,
				"sugov:%d",
				cpumask_first(policy->related_cpus));
	if (IS_ERR(thread)) {
		pr_err("failed to create sugov thread: %ld\n", PTR_ERR(thread));
		return PTR_ERR(thread);
	}

	ret = sched_setscheduler_nocheck(thread, SCHED_FIFO, &param);
	if (ret) {
		kthread_stop(thread);
		pr_warn("%s: failed to set SCHED_FIFO\n", __func__);
		return ret;
	}

	sg_policy->thread = thread;

	/* Kthread is bound to all CPUs by default */
	if (!policy->dvfs_possible_from_any_cpu)
		kthread_bind_mask(thread, cpu_coregroup_mask(0));

	init_irq_work(&sg_policy->irq_work, sugov_irq_work);
	mutex_init(&sg_policy->work_lock);

	wake_up_process(thread);

	return 0;
}

static void sugov_kthread_stop(struct sugov_policy *sg_policy)
{
	/* kthread only required for slow path */
	if (sg_policy->policy->fast_switch_enabled)
		return;

	kthread_flush_worker(&sg_policy->worker);
	kthread_stop(sg_policy->thread);
	mutex_destroy(&sg_policy->work_lock);
}

static struct sugov_tunables *sugov_tunables_alloc(struct sugov_policy *sg_policy)
{
	struct sugov_tunables *tunables;

	tunables = kzalloc(sizeof(*tunables), GFP_KERNEL);
	if (tunables) {
		gov_attr_set_init(&tunables->attr_set, &sg_policy->tunables_hook);
		if (!have_governor_per_policy())
			global_tunables = tunables;
	}
	return tunables;
}

static void sugov_tunables_free(struct sugov_tunables *tunables)
{
	if (!have_governor_per_policy())
		global_tunables = NULL;

	kfree(tunables);
}

static int sugov_init(struct cpufreq_policy *policy)
{
	struct sugov_policy *sg_policy;
	struct sugov_tunables *tunables;
	int ret = 0;

	/* State should be equivalent to EXIT */
	if (policy->governor_data)
		return -EBUSY;

	cpufreq_enable_fast_switch(policy);

	/* restore saved sg_policy */
	sg_policy = sugov_restore_policy(policy);
	if (sg_policy)
		goto tunables_init;

	sg_policy = sugov_policy_alloc(policy);
	if (!sg_policy) {
		ret = -ENOMEM;
		goto disable_fast_switch;
	}

	ret = sugov_kthread_create(sg_policy);
	if (ret)
		goto free_sg_policy;

tunables_init:
	mutex_lock(&global_tunables_lock);

	if (global_tunables) {
		if (WARN_ON(have_governor_per_policy())) {
			ret = -EINVAL;
			goto stop_kthread;
		}
		policy->governor_data = sg_policy;
		sg_policy->tunables = global_tunables;

		gov_attr_set_get(&global_tunables->attr_set, &sg_policy->tunables_hook);
		goto out;
	}

	tunables = sugov_tunables_alloc(sg_policy);
	if (!tunables) {
		ret = -ENOMEM;
		goto stop_kthread;
	}

	tunables->up_rate_limit_us = cpufreq_policy_transition_delay_us(policy);
	tunables->down_rate_limit_us = cpufreq_policy_transition_delay_us(policy);
#ifdef CONFIG_SCHED_KAIR_GLUE
	tunables->fb_legacy = true;
	sg_policy->be_stochastic = false;
#endif

	policy->governor_data = sg_policy;
	sg_policy->tunables = tunables;

	ret = kobject_init_and_add(&tunables->attr_set.kobj, &sugov_tunables_ktype,
				   get_governor_parent_kobj(policy), "%s",
				   schedutil_gov.name);
	if (ret)
		goto fail;

out:
	mutex_unlock(&global_tunables_lock);
	return 0;

fail:
	policy->governor_data = NULL;
	sugov_tunables_free(tunables);

stop_kthread:
	sugov_kthread_stop(sg_policy);
	mutex_unlock(&global_tunables_lock);

free_sg_policy:
	sugov_policy_free(sg_policy);

disable_fast_switch:
	cpufreq_disable_fast_switch(policy);

	pr_err("initialization failed (error %d)\n", ret);
	return ret;
}

static void sugov_exit(struct cpufreq_policy *policy)
{
	struct sugov_policy *sg_policy = policy->governor_data;
	struct sugov_tunables *tunables = sg_policy->tunables;
	unsigned int count;
#ifdef CONFIG_SCHED_KAIR_GLUE
	struct sugov_cpu *sg_cpu = &per_cpu(sugov_cpu, policy->cpu);
#endif

	mutex_lock(&global_tunables_lock);

	count = gov_attr_set_put(&tunables->attr_set, &sg_policy->tunables_hook);
	policy->governor_data = NULL;
	if (!count)
		sugov_tunables_free(tunables);

#ifdef CONFIG_SCHED_KAIR_GLUE
	if (sg_cpu->util_vessel) {
		sg_cpu->util_vessel->finalizer(sg_cpu->util_vessel);
		kair_obj_destructor(sg_cpu->util_vessel);
		sg_cpu->util_vessel = NULL;
	}
	sg_policy->be_stochastic = false;
#endif

	if (sugov_save_policy(sg_policy))
		goto out;

	sugov_kthread_stop(sg_policy);
	sugov_policy_free(sg_policy);

out:
	mutex_unlock(&global_tunables_lock);

	cpufreq_disable_fast_switch(policy);
}

static int sugov_start(struct cpufreq_policy *policy)
{
	struct sugov_policy *sg_policy = policy->governor_data;
	unsigned int cpu;
#ifdef CONFIG_SCHED_KAIR_GLUE
	char alias[KAIR_ALIAS_LEN];
#endif

	sg_policy->up_rate_delay_ns =
		sg_policy->tunables->up_rate_limit_us * NSEC_PER_USEC;
	sg_policy->down_rate_delay_ns =
		sg_policy->tunables->down_rate_limit_us * NSEC_PER_USEC;
	update_min_rate_limit_ns(sg_policy);
	sg_policy->last_freq_update_time = 0;
	sg_policy->next_freq = UINT_MAX;
	sg_policy->work_in_progress = false;
	sg_policy->need_freq_update = false;
	sg_policy->cached_raw_freq = 0;

	for_each_cpu(cpu, policy->cpus) {
		struct sugov_cpu *sg_cpu = &per_cpu(sugov_cpu, cpu);

#ifdef CONFIG_SCHED_KAIR_GLUE
		if (cpu != policy->cpu) {
			memset(sg_cpu, 0, sizeof(*sg_cpu));
			goto skip_subcpus;
		}

		if (!sg_policy->be_stochastic) {
			memset(alias, 0, KAIR_ALIAS_LEN);
			sprintf(alias, "govern%d", cpu);
			memset(sg_cpu, 0, sizeof(*sg_cpu));
			sg_cpu->util_vessel =
				kair_obj_creator(alias,
						 UTILAVG_KAIR_VARIANCE,
						 policy->cpuinfo.max_freq,
						 policy->cpuinfo.min_freq,
						 &kairistic_cpufreq);
			if (sg_cpu->util_vessel->initializer(sg_cpu->util_vessel) < 0) {
				sg_cpu->util_vessel->finalizer(sg_cpu->util_vessel);
				kair_obj_destructor(sg_cpu->util_vessel);
				sg_cpu->util_vessel = NULL;
			}
		} else {
			struct kair_class *vptr = sg_cpu->util_vessel;
			memset(sg_cpu, 0, sizeof(*sg_cpu));
			sg_cpu->util_vessel = vptr;
		}
skip_subcpus:
#else
		memset(sg_cpu, 0, sizeof(*sg_cpu));
#endif
		sg_cpu->cpu = cpu;
		sg_cpu->sg_policy = sg_policy;
		sg_cpu->flags = 0;
		sugov_start_slack(cpu);
		sg_cpu->iowait_boost_max = policy->cpuinfo.max_freq;
	}

#ifdef CONFIG_SCHED_KAIR_GLUE
	sg_policy->be_stochastic = true;
#endif

	for_each_cpu(cpu, policy->cpus) {
		struct sugov_cpu *sg_cpu = &per_cpu(sugov_cpu, cpu);

		cpufreq_add_update_util_hook(cpu, &sg_cpu->update_util,
							sugov_update_shared);
	}
	return 0;
}

static void sugov_stop(struct cpufreq_policy *policy)
{
	struct sugov_policy *sg_policy = policy->governor_data;
	unsigned int cpu;

	for_each_cpu(cpu, policy->cpus) {
		sugov_stop_slack(cpu);
		cpufreq_remove_update_util_hook(cpu);
	}

	synchronize_sched();

#ifdef CONFIG_SCHED_KAIR_GLUE
	for_each_cpu(cpu, policy->cpus) {
		struct sugov_cpu *sg_cpu = &per_cpu(sugov_cpu, cpu);
		if (sg_cpu->util_vessel) {
			sg_cpu->util_vessel->stopper(sg_cpu->util_vessel);
		}
	}
#endif

	if (!policy->fast_switch_enabled) {
		irq_work_sync(&sg_policy->irq_work);
	}
}

static void sugov_limits(struct cpufreq_policy *policy)
{
	struct sugov_policy *sg_policy = policy->governor_data;

	mutex_lock(&global_tunables_lock);

	if (!sg_policy) {
		mutex_unlock(&global_tunables_lock);
		return;
	}

	if (!policy->fast_switch_enabled) {
		mutex_lock(&sg_policy->work_lock);
		cpufreq_policy_apply_limits(policy);
		mutex_unlock(&sg_policy->work_lock);
	}

	sugov_update_min(policy);

	sg_policy->need_freq_update = true;

	mutex_unlock(&global_tunables_lock);
}

static struct cpufreq_governor schedutil_gov = {
	.name = "schedutil",
	.owner = THIS_MODULE,
	.dynamic_switching = true,
	.init = sugov_init,
	.exit = sugov_exit,
	.start = sugov_start,
	.stop = sugov_stop,
	.limits = sugov_limits,
};

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_SCHEDUTIL
struct cpufreq_governor *cpufreq_default_governor(void)
{
	return &schedutil_gov;
}
#endif
static void sugov_update_min(struct cpufreq_policy *policy)
{
	int cpu, max_cap;
	struct sugov_exynos *sg_exynos;
	int min_cap;

	max_cap = arch_scale_cpu_capacity(NULL, policy->cpu);

	/* min_cap is minimum value making higher frequency than policy->min */
	min_cap = max_cap * policy->min / policy->max;
	min_cap = (min_cap * 4 / 5) + 1;

	for_each_cpu(cpu, policy->cpus) {
		sg_exynos = &per_cpu(sugov_exynos, cpu);
		sg_exynos->min = min_cap;
	}
}

static void sugov_nop_timer(unsigned long data)
{
	/*
	 * The purpose of slack-timer is to wake up the CPU from IDLE, in order
	 * to decrease its frequency if it is not set to minimum already.
	 *
	 * This is important for platforms where CPU with higher frequencies
	 * consume higher power even at IDLE.
	 */
	trace_sugov_slack_func(smp_processor_id());
}

static void sugov_start_slack(int cpu)
{
	struct sugov_exynos *sg_exynos = &per_cpu(sugov_exynos, cpu);

	if (!sg_exynos->enabled)
		return;

	sg_exynos->min = ULONG_MAX;
	sg_exynos->started = true;
}

static void sugov_stop_slack(int cpu)
{
	struct sugov_exynos *sg_exynos = &per_cpu(sugov_exynos, cpu);

	sg_exynos->started = false;
	if (timer_pending(&sg_exynos->timer))
		del_timer_sync(&sg_exynos->timer);
}

static s64 get_next_event_time_ms(int cpu)
{
	return ktime_to_us(ktime_sub(*(get_next_event_cpu(cpu)), ktime_get()));
}

static int sugov_need_slack_timer(unsigned int cpu)
{
	struct sugov_cpu *sg_cpu = &per_cpu(sugov_cpu, cpu);
	struct sugov_exynos *sg_exynos = &per_cpu(sugov_exynos, cpu);

	if (schedtune_cpu_boost(cpu))
		return 0;

	if (sg_cpu->util > sg_exynos->min &&
		get_next_event_time_ms(cpu) > sg_exynos->expired_time)
		return 1;

	return 0;
}

static int sugov_pm_notifier(struct notifier_block *self,
						unsigned long action, void *v)
{
	unsigned int cpu = raw_smp_processor_id();
	struct sugov_exynos *sg_exynos = &per_cpu(sugov_exynos, cpu);
	struct sugov_cpu *sg_cpu = &per_cpu(sugov_cpu, cpu);
	struct timer_list *timer = &sg_exynos->timer;

	if (!sg_exynos->started)
		return NOTIFY_OK;

	switch (action) {
	case CPU_PM_ENTER_PREPARE:
		if (timer_pending(timer))
			del_timer_sync(timer);

		if (sugov_need_slack_timer(cpu)) {
			timer->expires = jiffies + msecs_to_jiffies(sg_exynos->expired_time);
			add_timer_on(timer, cpu);
			trace_sugov_slack(cpu, sg_cpu->util, sg_exynos->min, action, 1);
		}
		break;

	case CPU_PM_ENTER:
		if (timer_pending(timer) && !sugov_need_slack_timer(cpu)) {
			del_timer_sync(timer);
			trace_sugov_slack(cpu, sg_cpu->util, sg_exynos->min, action, -1);
		}
		break;

	case CPU_PM_EXIT_POST:
		if (timer_pending(timer) && (time_after(timer->expires, jiffies))) {
			del_timer_sync(timer);
			trace_sugov_slack(cpu, sg_cpu->util, sg_exynos->min, action, -1);
		}
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block sugov_pm_nb = {
	.notifier_call = sugov_pm_notifier,
};

static int find_cpu_pm_qos_class(int pm_qos_class)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		struct sugov_exynos *sg_exynos = &per_cpu(sugov_exynos, cpu);

		if ((sg_exynos->qos_min_class == pm_qos_class) &&
				cpumask_test_cpu(cpu, cpu_active_mask))
			return cpu;
	}

	pr_err("cannot find cpu of PM QoS class\n");
	return -EINVAL;
}

static int sugov_pm_qos_callback(struct notifier_block *nb,
					unsigned long val, void *v)
{
	struct sugov_cpu *sg_cpu;
	struct cpufreq_policy *policy;
	int pm_qos_class = *((int *)v);
	unsigned int next_freq;
	int cpu;

	cpu = find_cpu_pm_qos_class(pm_qos_class);
	if (cpu < 0)
		return NOTIFY_BAD;

	sg_cpu = &per_cpu(sugov_cpu, cpu);
	if (!sg_cpu || !sg_cpu->sg_policy || !sg_cpu->sg_policy->policy)
		return NOTIFY_BAD;

	next_freq = sg_cpu->sg_policy->next_freq;

	policy = cpufreq_cpu_get(cpu);
	if (!policy)
		return NOTIFY_BAD;

	if (val >= policy->cur) {
		cpufreq_cpu_put(policy);
		return NOTIFY_BAD;
	}

	cpufreq_driver_target(policy, next_freq, CPUFREQ_RELATION_L);

	cpufreq_cpu_put(policy);

	return NOTIFY_OK;
}

static struct notifier_block sugov_min_qos_notifier = {
	.notifier_call = sugov_pm_qos_callback,
	.priority = INT_MIN,
};

static int __init sugov_parse_dt(struct device_node *dn, int cpu)
{
	struct sugov_exynos *sg_exynos = &per_cpu(sugov_exynos, cpu);

	/* parsing slack info */
	if (of_property_read_u32(dn, "enabled", &sg_exynos->enabled))
		return -EINVAL;
	if (sg_exynos->enabled)
		if (of_property_read_u32(dn, "expired_time", &sg_exynos->expired_time))
			sg_exynos->expired_time = DEFAULT_EXPIRED_TIME;

	/* parsing pm_qos_class info */
	if (of_property_read_u32(dn, "qos_min_class", &sg_exynos->qos_min_class))
		return -EINVAL;

	return 0;
}

static void __init sugov_exynos_init(void)
{
	int cpu, ret;
	struct device_node *dn = NULL;
	const char *buf;

	while ((dn = of_find_node_by_type(dn, "schedutil-domain"))) {
		struct cpumask shared_mask;
		/* Get shared cpus */
		ret = of_property_read_string(dn, "shared-cpus", &buf);
		if (ret)
			goto exit;

		cpulist_parse(buf, &shared_mask);
		for_each_cpu(cpu, &shared_mask)
			if (sugov_parse_dt(dn, cpu))
				goto exit;
	}

	for_each_possible_cpu(cpu) {
		struct sugov_exynos *sg_exynos = &per_cpu(sugov_exynos, cpu);

		if (!sg_exynos->enabled)
			continue;

		/* Initialize slack-timer */
		init_timer_pinned(&sg_exynos->timer);
		sg_exynos->timer.function = sugov_nop_timer;
	}

	pm_qos_add_notifier(PM_QOS_CLUSTER0_FREQ_MIN, &sugov_min_qos_notifier);
	pm_qos_add_notifier(PM_QOS_CLUSTER1_FREQ_MIN, &sugov_min_qos_notifier);
	pm_qos_add_notifier(PM_QOS_CLUSTER2_FREQ_MIN, &sugov_min_qos_notifier);
	cpu_pm_register_notifier(&sugov_pm_nb);

	return;
exit:
	pr_info("%s: failed to initialized slack_timer, pm_qos handler check\n", __func__);
}

static int __init sugov_register(void)
{
	sugov_exynos_init();

	return cpufreq_register_governor(&schedutil_gov);
}
fs_initcall(sugov_register);
