/*
 * linux/drivers/exynos/soc/samsung/exynos-emc.c
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/sched/types.h>
#include <linux/irq_work.h>
#include <linux/kobject.h>
#include <linux/cpufreq.h>
#include <linux/suspend.h>
#include <linux/cpuidle.h>

#include <soc/samsung/exynos-cpuhp.h>
#include <soc/samsung/cal-if.h>
#include <soc/samsung/exynos-pmu.h>
#include <soc/samsung/exynos-emc.h>
#include <dt-bindings/soc/samsung/exynos-emc.h>

#include <trace/events/power.h>
#include "../../cpufreq/exynos-acme.h"
#include "../../../kernel/sched/sched.h"

#define DEFAULT_BOOT_ENABLE_MS (40000)		/* 40 s */

#define EMC_EVENT_NUM	2
enum emc_event {
	/* mode change finished and then waiting new mode */
	EMC_WAITING_NEW_MODE = 0,

	/* dynimic mode change by sched event */
	EMC_DYNIMIC_MODE_CHANGE_STARTED = (0x1 << 0),

	/* static mode change by user or pm scenarios */
	EMC_STATIC_MODE_CHANGE_STARTED = (0x1 << 1),
};

enum emc_throttle_idx {
	EMC_THROTTLE_POL = 0,
	EMC_THROTTLE_QOS = 1,
	EMC_THROTTLE_END = 2,
};

struct emc_mode {
	struct list_head	list;
	const char		*name;
	struct cpumask		cpus;
	struct cpumask		boost_cpus;
	unsigned int		ldsum_thr;
	unsigned int		cal_id;
	unsigned int		max_freq;
	unsigned int		change_latency;
	unsigned int		enabled;

	/* kobject for sysfs group */
	struct kobject		kobj;
};

struct emc_domain {
	struct list_head	list;
	const char		*name;
	struct cpumask		cpus;
	unsigned int		role;
	unsigned int		cpu_heavy_thr;
	unsigned int		cpu_idle_thr;
	unsigned int		busy_ratio;

	unsigned long		load;
	unsigned long		max;

	/* kobject for sysfs group */
	struct kobject		kobj;
};

struct emc {
	unsigned int		enabled;
	int			blocked;
	unsigned int		event;
	unsigned int		ctrl_type;

	unsigned int		qos_class;
	bool			throttled;
	bool			throttle_ip[EMC_THROTTLE_END];

	struct list_head	domains;
	struct list_head	modes;

	struct emc_mode		*cur_mode;	/* current mode */
	struct emc_mode		*req_mode;	/* requested mode */
	struct emc_mode		*user_mode;	/* user requesting mode */
	unsigned int		in_progress;
	struct cpumask		heavy_cpus;	/* cpus need to boost */
	struct cpumask		busy_cpus;	/* cpus need to online */
	/* loadsum of boostable and trigger domain */
	unsigned int		ldsum;

	/* member for mode change */
	struct task_struct	*task;
	struct irq_work		irq_work;
	struct hrtimer		timer;
	wait_queue_head_t	wait_q;

	/* member for max freq control */
	struct cpumask		pre_cpu_mask;
	struct cpumask		pwr_cpu_mask;
	unsigned long		max_freq;

	/* member for sysfs */
	struct kobject		kobj;
	struct mutex		attrib_lock;

} emc;

static DEFINE_SPINLOCK(emc_lock);
static DEFINE_MUTEX(emc_const_lock);
DEFINE_RAW_SPINLOCK(emc_load_lock);
/**********************************************************************************/
/*				   Helper					  */
/**********************************************************************************/
/* return base mode. base mode is default and lowest boost mode */
static struct emc_mode* emc_get_base_mode(void)
{
	return list_first_entry(&emc.modes, struct emc_mode, list);
}

/* return matches arg mask with mode->cpus */
static struct emc_mode* emc_find_mode(struct cpumask *mask)
{
	struct emc_mode *mode;

	list_for_each_entry(mode, &emc.modes, list)
		if (cpumask_equal(&mode->cpus, mask))
			return mode;

	/* if don,t find any matehd mode, return base mode */
	return emc_get_base_mode();
}

/* return domain including arg cpu */
static struct emc_domain* emc_find_domain(unsigned int cpu)
{
	struct emc_domain *domain;

	list_for_each_entry(domain, &emc.domains, list)
		if (cpumask_test_cpu(cpu, &domain->cpus))
			return domain;

	return NULL;	/* error */
}

/* return boost domain */
static struct emc_domain* emc_get_boost_domain(void)
{
	/* HACK : Supports only one boostable domain */
	return list_last_entry(&emc.domains, struct emc_domain, list);
}

/* update cpu capacity for scheduler */
static int emc_update_capacity(struct cpumask *mask)
{
	struct sched_domain *sd;
	struct cpumask temp;
	int cpu;

	rcu_read_lock();

	cpumask_and(&temp, mask, cpu_active_mask);
	if (cpumask_empty(&temp))
		goto exit;
	cpu = cpumask_first(&temp);

	sd = rcu_dereference(per_cpu(sd_ea, cpu));
	if (!sd)
		goto exit;

	while (sd->child)
		sd = sd->child;

	update_group_capacity(sd, cpu);

exit:
	rcu_read_unlock();

	return 0;
}

void emc_check_available_freq(struct cpumask *cpus, unsigned int target_freq)
{
	unsigned int max_freq;
	struct emc_domain *domain = emc_get_boost_domain();
	int cpu = cpumask_first(cpus);
	struct cpumask online_mask;
	struct emc_mode *mode;

	cpumask_copy(&online_mask, cpu_online_mask);
	mode = emc_find_mode(&online_mask);

	if (!cpumask_equal(cpus, &domain->cpus))
		return;

	if (mode)
		max_freq = mode->max_freq;
	else
		max_freq = emc_get_base_mode()->max_freq;

	if (target_freq > max_freq)
		panic("cpu%d target_freq(%d) is higher than max_freq(%d, mode %s)\n",
						cpu, target_freq, max_freq, mode->name);
}

/* check policy->max constaints and real clock violation */
int emc_verify_constraints(void)
{
	struct cpufreq_policy *policy;
	struct emc_domain *domain;
	unsigned int cpu, cur_freq;

	domain = emc_get_boost_domain();
	cpu = cpumask_first(&domain->cpus);

	policy = cpufreq_cpu_get(cpu);
	if (!policy) {
		pr_warn("EMC: can't get the policy of cpu %d\n", cpu);
		goto check_real_freq;
	}

	/* check policy max */
	if (policy->max > emc.max_freq) {
		cpufreq_cpu_put(policy);
		pr_warn("EMC: constraints isn't yet applyied(emc_max(%lu) < cur_max(%d)\n",
				emc.max_freq, policy->max);
		return 0;
	}
	cpufreq_cpu_put(policy);

check_real_freq:
	/* check whether real cpu freq within max constraints or not */
	cur_freq = exynos_cpufreq_get_locked(cpu);
	if (cur_freq > emc.max_freq) {
		pr_warn("EMC(%s: cur freq(%d) is higher than max(%lu), retring...\n",
			__func__, cur_freq, emc.max_freq);
		return 0;
	}

	return 1;
}

/*
 * return highest boost frequency
 */
unsigned int exynos_pstate_get_boost_freq(int cpu)
{
	struct emc_domain *domain = emc_get_boost_domain();

	if (!cpumask_test_cpu(cpu, &domain->cpus))
		return 0;

	return list_last_entry(&emc.modes, struct emc_mode, list)->max_freq;
}

/**********************************************************************************/
/*				   Update Load					  */
/**********************************************************************************/
/* update cpu and domain load */
static int emc_update_load(void)
{
	struct emc_domain *domain;
	int cpu;

	list_for_each_entry(domain, &emc.domains, list) {
		domain->load = 0;
		domain->max = 0;
		for_each_cpu_and(cpu, &domain->cpus, cpu_online_mask) {
			struct rq *rq = cpu_rq(cpu);
			struct sched_avg *sa = &rq->cfs.avg;
			unsigned long load;

			domain->max += (rq->cpu_capacity_orig);
			if (sa->util_avg > rq->cpu_capacity_orig)
				load = rq->cpu_capacity_orig;
			else
				load = sa->util_avg;

			domain->load += load;

			trace_emc_cpu_load(cpu, load, rq->cpu_capacity_orig);
		}
		trace_emc_domain_load(domain->name, domain->load, domain->max);
	}

	return 0;
}

/* update domains's cpus status whether busy or idle */
static int emc_update_domain_status(struct emc_domain *domain)
{
	struct cpumask heavy_cpus, busy_cpus, idle_cpus;
	int cpu;

	cpumask_clear(&heavy_cpus);
	cpumask_clear(&busy_cpus);
	cpumask_clear(&idle_cpus);
	emc.ldsum = 0;

	/*
	 * Takes offline core like idle core
	 * IDLE_CPU	   : util_avg < domain->cpu_idle_thr
	 * BUSY_CPU        : domain->cpu_idle_thr <= util_avg < domain->cpu_heavy_thr
	 * HEAVY_CPU	   : util_avg >= domain->cpu_heavy_thr
	 */
	for_each_cpu_and(cpu, &domain->cpus, cpu_online_mask) {
		struct rq *rq = cpu_rq(cpu);
		struct sched_avg *sa = &rq->cfs.avg;

		if (sa->util_avg >= domain->cpu_heavy_thr) {
			cpumask_set_cpu(cpu, &heavy_cpus);
			cpumask_set_cpu(cpu, &busy_cpus);
			emc.ldsum += sa->util_avg;
		} else if (sa->util_avg >= domain->cpu_idle_thr) {
			cpumask_set_cpu(cpu, &busy_cpus);
			emc.ldsum += sa->util_avg;
		} else
			cpumask_set_cpu(cpu, &idle_cpus);
	}

	/* domain cpus status updated system cpus mask */
	cpumask_or(&emc.heavy_cpus, &emc.heavy_cpus, &heavy_cpus);
	cpumask_or(&emc.busy_cpus, &emc.busy_cpus, &busy_cpus);

	trace_emc_domain_status(domain->name,
			*(unsigned int *)cpumask_bits(&emc.heavy_cpus),
			*(unsigned int *)cpumask_bits(&emc.busy_cpus),
			*(unsigned int *)cpumask_bits(&heavy_cpus),
			*(unsigned int *)cpumask_bits(&busy_cpus));

	return 0;
}

/*
 * update imbalance_heavy_cpus & busy_cpus_mask
 * and return true if there is status change
 */
static bool emc_update_system_status(void)
{
	struct emc_domain *domain;
	struct cpumask prev_heavy_cpus, prev_busy_cpus;

	/* back up prev mode and clear */
	cpumask_copy(&prev_heavy_cpus, &emc.heavy_cpus);
	cpumask_copy(&prev_busy_cpus, &emc.busy_cpus);
	cpumask_clear(&emc.heavy_cpus);
	cpumask_clear(&emc.busy_cpus);

	/* update system status */
	list_for_each_entry(domain, &emc.domains, list)
		emc_update_domain_status(domain);

	trace_emc_update_system_status(*(unsigned int *)cpumask_bits(&prev_heavy_cpus),
				*(unsigned int *)cpumask_bits(&prev_busy_cpus),
				*(unsigned int *)cpumask_bits(&emc.heavy_cpus),
				*(unsigned int *)cpumask_bits(&emc.busy_cpus));
	/*
	 * Check whether prev_cpus status and latest_cpus status is different or not,
	 * if it is different, return true. true means we should check whether there is
	 * more adaptive mode or not
	 */
	if (!cpumask_equal(&prev_busy_cpus, &emc.busy_cpus) ||
		!cpumask_equal(&prev_heavy_cpus, &emc.heavy_cpus))
		return true;

	return false;
}

/**********************************************************************************/
/*				   MODE SELECTION				  */
/**********************************************************************************/
static int emc_domain_busy(struct emc_domain *domain)
{
	if (domain->load > ((domain->max * domain->busy_ratio) / 100))
		return true;;

	return false;
}

static bool emc_system_busy(void)
{
	struct emc_domain *domain;
	struct cpumask mask;

	list_for_each_entry(domain, &emc.domains, list) {
		/* if all cpus of domain was hotplug out, skip */
		cpumask_and(&mask, &domain->cpus, cpu_online_mask);
		if (cpumask_empty(&mask))
			continue;

		if (domain->busy_ratio && !emc_domain_busy(domain)) {
			trace_emc_domain_busy(domain->name, domain->load, false);
			return false;
		}
	}

	return true;
}

/*
 * return true when system has boostable cpu
 * To has boostable cpu, boostable domains must has heavy cpu
 */
static bool emc_has_boostable_cpu(void)
{
	struct emc_domain *domain;

	domain = emc_get_boost_domain();

	return cpumask_intersects(&emc.heavy_cpus, &domain->cpus);
}

static struct emc_mode* emc_select_mode(void)
{
	struct emc_mode *mode, *target_mode = NULL;
	int need_online_cnt;

	/* if there is no boostable cpu, we don't need to booting */
	if (!emc_has_boostable_cpu())
		return emc_get_base_mode();

	/*
	 * need_online_cnt: number of cpus that need online
	 */
	need_online_cnt = cpumask_weight(&emc.busy_cpus);

	/* In reverse order to find the most boostable mode */
	list_for_each_entry_reverse(mode, &emc.modes, list) {
		if (!mode->enabled)
			continue;
		/* if ldsum_thr is 0, it means ldsum is disabled */
		if (!mode->ldsum_thr && emc.ldsum <= mode->ldsum_thr) {
			target_mode = mode;
			break;
		}
		if (need_online_cnt > cpumask_weight(&mode->boost_cpus))
			continue;
		target_mode = mode;
		break;
	}

	if (!target_mode)
		target_mode = emc_get_base_mode();

	trace_emc_select_mode(target_mode->name, need_online_cnt);

	return target_mode;
}

/* return latest adaptive mode */
static struct emc_mode* emc_get_mode(bool updated)
{
	/*
	 * if system is busy overall, return base mode.
	 * becuase system is busy, maybe base mode will be
	 * the best performance
	 */
	if (emc_system_busy())
		return emc_get_base_mode();

	/*
	 * if system isn't busy overall and no status updated,
	 * keep previous mode
	 */
	if (!updated)
		return emc.req_mode;

	/* if not all above, try to find more adaptive mode */
	return emc_select_mode();
}

/**********************************************************************************/
/*				   Mode Change					  */
/**********************************************************************************/
/* static mode change function */
static void emc_set_mode(struct emc_mode *target_mode)
{
	unsigned long flags;

	if (hrtimer_active(&emc.timer))
		hrtimer_cancel(&emc.timer);

	spin_lock_irqsave(&emc_lock, flags);
	emc.event = emc.event | EMC_STATIC_MODE_CHANGE_STARTED;
	emc.req_mode = target_mode;
	spin_unlock_irqrestore(&emc_lock, flags);

	wake_up(&emc.wait_q);

	return;
}

static void emc_request_mode_change(struct emc_mode *target_mode)
{
	unsigned long flags;

	spin_lock_irqsave(&emc_lock, flags);
	emc.req_mode = target_mode;
	spin_unlock_irqrestore(&emc_lock, flags);

	irq_work_queue(&emc.irq_work);
}

static void emc_irq_work(struct irq_work *irq_work)
{
	/*
	 * If req_mode is changed before mode change latency,
	 * cancel requesting mode change
	 */
	if (hrtimer_active(&emc.timer))
		hrtimer_cancel(&emc.timer);

	/* if req_mode and cur_mode is same, skip the mode change */
	if (emc.req_mode == emc.cur_mode)
		return;

	trace_emc_start_timer(emc.req_mode->name, emc.req_mode->change_latency);

	/* emc change applying req_mode after keeps same mode as change_latency */
	hrtimer_start(&emc.timer,
		ms_to_ktime(emc.req_mode->change_latency),
		HRTIMER_MODE_REL);
}

static enum hrtimer_restart emc_mode_change_func(struct hrtimer *timer)
{
	unsigned long flags;

	spin_lock_irqsave(&emc_lock, flags);
	emc.event = emc.event | EMC_DYNIMIC_MODE_CHANGE_STARTED;
	spin_unlock_irqrestore(&emc_lock, flags);

	/* wake up mode change task */
	wake_up(&emc.wait_q);
	return HRTIMER_NORESTART;
}

static unsigned int emc_clear_event(void)
{
	int i;

	/* find pending wake-up event */
	for (i = 0; i < EMC_EVENT_NUM; i++)
		if (emc.event & (0x1 << i))
			break;

	/* clear event */
	emc.event = emc.event & ~(0x1 << i);

	return (0x1 << i);
}

/* mode change function */
static int emc_do_mode_change(void *data)
{
	unsigned long flags;

	while (1) {
		unsigned int event;

		wait_event(emc.wait_q, emc.event || kthread_should_park());
		if (kthread_should_park())
			break;

		spin_lock_irqsave(&emc_lock, flags);
		emc.in_progress = 1;
		event = emc_clear_event();

		trace_emc_do_mode_change(emc.cur_mode->name,
				emc.req_mode->name, emc.event);

		emc.cur_mode = emc.req_mode;
		spin_unlock_irqrestore(&emc_lock, flags);

		/* request mode change */
		exynos_cpuhp_request("EMC", emc.cur_mode->cpus, emc.ctrl_type);

		dbg_snapshot_printk("EMC: mode change finished %s (cpus%d)\n",
			emc.cur_mode->name, cpumask_weight(&emc.cur_mode->cpus));
		emc.in_progress = 0;
	}

	return 0;
}

void exynos_emc_update(int cpu)
{
	unsigned long flags;
	struct emc_mode *target_mode;
	bool updated;

	if (!emc.enabled || emc.blocked > 0)
		return;

	/* little cpus sikp updating bt mode */
	if (cpumask_test_cpu(cpu, cpu_coregroup_mask(0)))
		return;

	if (!raw_spin_trylock_irqsave(&emc_load_lock, flags))
		return;

	/* if user set user_mode, always return user mode */
	if (unlikely(emc.user_mode)) {
		target_mode = emc.user_mode;
		goto skip_load_check;
	}

	/* if current system is throttled, always uses base_mode */
	if (emc.throttled) {
		target_mode = emc_get_base_mode();
		goto skip_load_check;
	}

	/* update sched_load */
	emc_update_load();

	/* update cpus status whether cpu is busy or heavy */
	updated = emc_update_system_status();

	/* get mode */
	target_mode = emc_get_mode(updated);

skip_load_check:
	/* request mode */
	if (emc.req_mode != target_mode)
		emc_request_mode_change(target_mode);

	raw_spin_unlock_irqrestore(&emc_load_lock, flags);
}

/**********************************************************************************/
/*			      Max Frequency Control				  */
/**********************************************************************************/
static int emc_update_domain_const(struct emc_domain *domain)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(1000);
	struct cpumask mask;
	int cpu;

	/* If there is no online cpu on the domain, skip policy update */
	cpumask_and(&mask, &domain->cpus, cpu_online_mask);
	if (!cpumask_weight(&mask))
		return 0;
	cpu = cpumask_first(&mask);

	/* if max constraints is not changed util 50ms, cancel cpu_up */
	cpufreq_update_policy(cpu);
	while (!emc_verify_constraints()) {
		cpufreq_update_policy(cpu);
		if (time_after(jiffies, timeout)) {
			panic("EMC: failed to update domain(cpu%d) constraints\n", cpu);
			return -EBUSY;
		}
		udelay(100);
	}

	/* update capacity for scheduler */
	emc_update_capacity(&mask);

	return 0;
}

/* update constraints base on pre_cpu_mask */
int emc_update_constraints(void)
{
	struct emc_domain *domain;
	int ret = 0;

	/* update max constraint of all domains related this cpu power buget */
	list_for_each_entry(domain, &emc.domains, list) {
		if (domain->role & BOOSTER)
			ret = emc_update_domain_const(domain);
		if (ret)
			break;
	}

	return ret;
}

static void emc_set_const(struct cpumask *mask)
{
	struct emc_mode *mode;

	mutex_lock(&emc_const_lock);

	mode = emc_find_mode(mask);

	if (mode->max_freq == emc.max_freq)
		goto skip_update_const;

	emc.max_freq = mode->max_freq;
	emc_update_constraints();

skip_update_const:
	mutex_unlock(&emc_const_lock);
}

static unsigned long emc_get_const(void)
{
	if (!emc.enabled || emc.blocked > 0)
		return emc_get_base_mode()->max_freq;

	return emc.max_freq;
}

static void emc_update_throttle(bool throttled, unsigned int idx)
{
	unsigned long flags;

	spin_lock_irqsave(&emc_lock, flags);

	emc.throttle_ip[idx] = throttled;
	emc.throttled = emc.throttle_ip[EMC_THROTTLE_POL]
				| emc.throttle_ip[EMC_THROTTLE_QOS];

	spin_unlock_irqrestore(&emc_lock, flags);
}

static int emc_qos_callback(struct notifier_block *nb,
				unsigned long val, void *v)
{
	bool throttled = true;

	if (val > emc_get_base_mode()->max_freq)
		throttled = false;

	emc_update_throttle(throttled, EMC_THROTTLE_QOS);

	return NOTIFY_OK;
}

static struct notifier_block emc_qos_max_notifier = {
	.notifier_call = emc_qos_callback,
};

static int cpufreq_policy_notifier(struct notifier_block *nb,
				    unsigned long event, void *data)
{
	struct cpufreq_policy *policy = data;
	unsigned long max_freq;
	struct emc_domain *domain = emc_find_domain(policy->cpu);
	bool throttled = false;

	if (!(domain->role & BOOSTER))
		return 0;

	switch (event) {
	case CPUFREQ_ADJUST:
	/* It updates max frequency of policy */
		max_freq = emc_get_const();

		if (policy->max != max_freq)
			cpufreq_verify_within_limits(policy, 0, max_freq);

		break;
	case CPUFREQ_NOTIFY:
	/* It updates boostable flag */
		if (policy->max < emc_get_base_mode()->max_freq)
			throttled = true;

		emc_update_throttle(throttled, EMC_THROTTLE_POL);

		break;
	}

	return 0;
}
/* Notifier for cpufreq policy change */
static struct notifier_block emc_policy_nb = {
	.notifier_call = cpufreq_policy_notifier,
};

/*
 * emc_update_cpu_pwr controls constraints acording
 * to mode matched cpu power status to be changed.
 * so, following function call order shouled be followed.
 *
 * CPU POWER ON Scenario  : Call this function -> CPU_POWER_ON
 * CPU POWER OFF Scenario : CPU_POWER_OFF -> Call this function
 */
static int emc_update_cpu_pwr(unsigned int cpu, bool on)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(100);

	/*
	 * check acutal cpu power. this function shuouled be call
	 * before power on or after power off
	 */
	while (exynos_cpu.power_state(cpu)) {
		if (time_after(jiffies, timeout))
			panic("CPU%d %s power %s!\n",
				cpu, on? "already" : "not yet", on? "on" : "off");

		udelay(100);
	}

	if (on)
		cpumask_set_cpu(cpu, &emc.pwr_cpu_mask);
	else
		cpumask_clear_cpu(cpu, &emc.pwr_cpu_mask);

	trace_emc_update_cpu_pwr(
		*(unsigned int *)cpumask_bits(&emc.pwr_cpu_mask), cpu, on);

	emc_set_const(&emc.pwr_cpu_mask);

	return 0;
}

static int emc_pre_update_constraints(void)
{
	emc_set_const(&emc.pre_cpu_mask);

	return 0;
}

static int emc_update_pre_mask(unsigned int cpu, bool on)
{
	struct cpumask temp;
	struct emc_domain *domain = emc_get_boost_domain();

	if (on)
		cpumask_set_cpu(cpu, &emc.pre_cpu_mask);
	else
		cpumask_clear_cpu(cpu, &emc.pre_cpu_mask);

	/*
	 * If all cpus of boost cluster will be power down,
	 * change constraints before cpufreq is not working
	 */
	cpumask_and(&temp, &emc.pre_cpu_mask, &domain->cpus);
	if (cpumask_empty(&temp))
		emc_pre_update_constraints();

	return 0;
}

/*
 * emc_hp_callback MUST CALL emc_update_cpu_pwr
 * to control max constraints and MUST KEEP following orders.
 *
 * hotplug OUT: cpu power DOWN -> Call emc_update_cpu_pwr
 * hotplug IN : Call emc_update_cpu_pwr -> cpu power UP
 */
static int emc_cpu_on_callback(unsigned int cpu)
{
	if (emc_update_cpu_pwr(cpu, true))
		return -EINVAL;

	return 0;
}
int emc_cpu_pre_on_callback(unsigned int cpu)
{
	emc_update_pre_mask(cpu, true);
	return 0;
}

static int emc_cpu_off_callback(unsigned int cpu)
{
	if (emc_update_cpu_pwr(cpu, false))
		return -EINVAL;

	return 0;
}
int emc_cpu_pre_off_callback(unsigned int cpu)
{
	emc_update_pre_mask(cpu, false);
	return 0;
}

/**********************************************************************************/
/*					SYSFS					  */
/**********************************************************************************/
#define to_emc(k) container_of(k, struct emc, kobj)
#define to_domain(k) container_of(k, struct emc_domain, kobj)
#define to_mode(k) container_of(k, struct emc_mode, kobj)

#define emc_show(file_name, object)				\
static ssize_t show_##file_name						\
(struct kobject *kobj, char *buf)					\
{									\
	struct emc *emc = to_emc(kobj);			\
									\
	return sprintf(buf, "%u\n", emc->object);			\
}

#define emc_store(file_name, object)				\
static ssize_t store_##file_name					\
(struct kobject *kobj, const char *buf, size_t count)			\
{									\
	int ret;							\
	unsigned int val;						\
	struct emc *emc = to_emc(kobj);			\
									\
	ret = kstrtoint(buf, 10, &val);					\
	if (ret)							\
		return -EINVAL;						\
									\
	emc->object = val;						\
									\
	return ret ? ret : count;					\
}

#define emc_domain_show(file_name, object)				\
static ssize_t show_##file_name						\
(struct kobject *kobj, char *buf)					\
{									\
	struct emc_domain *domain = to_domain(kobj);			\
									\
	return sprintf(buf, "%u\n", domain->object);			\
}

#define emc_domain_store(file_name, object)				\
static ssize_t store_##file_name					\
(struct kobject *kobj, const char *buf, size_t count)			\
{									\
	int ret;							\
	unsigned int val;						\
	struct emc_domain *domain = to_domain(kobj);			\
									\
	ret = kstrtoint(buf, 10, &val);					\
	if (ret)							\
		return -EINVAL;						\
									\
	domain->object = val;						\
									\
	return ret ? ret : count;					\
}

#define emc_mode_show(file_name, object)				\
static ssize_t show_##file_name						\
(struct kobject *kobj, char *buf)					\
{									\
	struct emc_mode *mode = to_mode(kobj);			\
									\
	return sprintf(buf, "%u\n", mode->object);			\
}

#define emc_mode_store(file_name, object)				\
static ssize_t store_##file_name					\
(struct kobject *kobj, const char *buf, size_t count)			\
{									\
	int ret;							\
	unsigned int val;						\
	struct emc_mode *mode = to_mode(kobj);			\
									\
	ret = kstrtoint(buf, 10, &val);					\
	if (ret)							\
		return -EINVAL;						\
									\
	mode->object = val;						\
									\
	return ret ? ret : count;					\
}

emc_show(enabled, enabled);
emc_show(ctrl_type, ctrl_type);
emc_show(throttled, throttled);

emc_domain_store(cpu_heavy_thr, cpu_heavy_thr);
emc_domain_store(cpu_idle_thr, cpu_idle_thr);
emc_domain_store(busy_ratio, busy_ratio);
emc_domain_show(cpu_heavy_thr, cpu_heavy_thr);
emc_domain_show(cpu_idle_thr, cpu_idle_thr);
emc_domain_show(busy_ratio, busy_ratio);

emc_mode_store(max_freq, max_freq);
emc_mode_store(change_latency, change_latency);
emc_mode_store(ldsum_thr, ldsum_thr);
emc_mode_store(mode_enabled, enabled);
emc_mode_show(max_freq, max_freq);
emc_mode_show(change_latency, change_latency);
emc_mode_show(ldsum_thr, ldsum_thr);
emc_mode_show(mode_enabled, enabled);

static int emc_set_enable(bool enable);
static ssize_t store_enabled(struct kobject *kobj,
			const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = kstrtoint(buf, 10, &val);
	if (ret)
		return -EINVAL;

	if (val > 0)
		ret = emc_set_enable(true);
	else
		ret = emc_set_enable(false);

	return ret ? ret : count;
}

static void __emc_set_disable(void)
{
	struct emc_mode *base_mode = emc_get_base_mode();

	spin_lock(&emc_lock);
	emc.enabled = false;
	emc.user_mode = 0;
	emc.cur_mode = emc.req_mode = base_mode;
	emc.event = 0;
	smp_wmb();
	spin_unlock(&emc_lock);

	exynos_cpuhp_request("EMC",
		base_mode->cpus, emc.ctrl_type);

	pr_info("EMC: Stop hotplug governor\n");
}

static void __emc_set_enable(void)
{
	struct emc_mode *base_mode = emc_get_base_mode();

	spin_lock(&emc_lock);
	emc.user_mode = 0;
	emc.cur_mode = emc.req_mode = base_mode;
	emc.event = 0;
	emc.enabled = true;
	smp_wmb();
	spin_unlock(&emc_lock);

	pr_info("EMC: Start hotplug governor\n");
}

static int emc_set_enable(bool enable)
{
	int start = true;

	spin_lock(&emc_lock);

	if (enable)
		if (emc.enabled) {
			pr_info("EMC: Already enabled\n");
			spin_unlock(&emc_lock);
			goto skip;
		} else {
			start = true;
		}
	else
		if (emc.enabled) {
			start = false;
		} else {
			pr_info("EMC: Already disabled\n");
			spin_unlock(&emc_lock);
			goto skip;
		}

	spin_unlock(&emc_lock);

	if (start)
		__emc_set_enable();
	else
		__emc_set_disable();

skip:
	return 0;
}
static ssize_t store_ctrl_type(struct kobject *kobj,
			const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = kstrtoint(buf, 10, &val);
	if (ret)
		return -EINVAL;

	emc.ctrl_type = val ? FAST_HP : 0;

	return ret ? ret : count;
}

static ssize_t store_user_mode(struct kobject *kobj,
			const char *buf, size_t count)
{
	int ret, i = 0;
	unsigned int val;
	struct emc_mode *mode;

	ret = kstrtoint(buf, 10, &val);
	if (ret)
		return -EINVAL;

	/* Cancel or Disable user mode */
	if (!val) {
		emc.user_mode = NULL;
		mode = emc_get_base_mode();
		emc_set_mode(mode);
		goto exit;
	}

	list_for_each_entry_reverse(mode, &emc.modes, list)
		if (++i == val)
			goto check_mode;

	/* input is invalid */
	pr_info("Input value is invalid\n");
	goto exit;

check_mode:
	if (!mode->enabled) {
		pr_info("%s mode is not enabled\n", mode->name);
		goto exit;
	}

	emc.user_mode = mode;
	emc_set_mode(mode);
exit:
	return ret ? ret : count;
}

static ssize_t show_user_mode(struct kobject *kobj, char *buf)
{
	int ret = 0, i = 0;
	struct emc_mode *mode;

	if (emc.user_mode)
		return sprintf(buf, "%s\n", emc.user_mode->name);

	/* If user_mode is not set, show avaiable user mode */
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"Available mode> 0:DISABLE ");

	list_for_each_entry_reverse(mode, &emc.modes, list)
		ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"%d:%s ", ++i, mode->name);

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");

	return ret;
}

static ssize_t show_domain_name(struct kobject *kobj, char *buf)
{
	struct emc_domain *domain = to_domain(kobj);

	return sprintf(buf, "%s\n", domain->name);
}

static ssize_t show_mode_name(struct kobject *kobj, char *buf)
{
	struct emc_mode *mode = to_mode(kobj);

	return sprintf(buf, "%s\n", mode->name);
}

struct emc_attr {
	struct attribute attr;
	ssize_t (*show)(struct kobject *, char *);
	ssize_t (*store)(struct kobject *, const char *, size_t count);
};

#define emc_attr_ro(_name)		\
static struct emc_attr _name =			\
__ATTR(_name, 0444, show_##_name, NULL)

#define emc_attr_rw(_name)		\
static struct emc_attr _name =			\
__ATTR(_name, 0644, show_##_name, store_##_name)

#define to_attr(a) container_of(a, struct emc_attr, attr)

static ssize_t show(struct kobject *kobj, struct attribute *attr, char *buf)
{
	struct emc_attr *hattr = to_attr(attr);
	ssize_t ret;

	mutex_lock(&emc.attrib_lock);
	ret = hattr->show(kobj, buf);
	mutex_unlock(&emc.attrib_lock);

	return ret;
}

static ssize_t store(struct kobject *kobj, struct attribute *attr,
		     const char *buf, size_t count)
{
	struct emc_attr *hattr = to_attr(attr);
	ssize_t ret = -EINVAL;

	mutex_lock(&emc.attrib_lock);
	ret = hattr->store(kobj, buf, count);
	mutex_unlock(&emc.attrib_lock);

	return ret;
}

emc_attr_rw(enabled);
emc_attr_rw(ctrl_type);
emc_attr_ro(throttled);
emc_attr_rw(user_mode);

emc_attr_ro(domain_name);
emc_attr_rw(cpu_heavy_thr);
emc_attr_rw(cpu_idle_thr);
emc_attr_rw(busy_ratio);

emc_attr_ro(mode_name);
emc_attr_rw(max_freq);
emc_attr_rw(change_latency);
emc_attr_rw(ldsum_thr);
emc_attr_rw(mode_enabled);

static struct attribute *emc_attrs[] = {
	&enabled.attr,
	&ctrl_type.attr,
	&throttled.attr,
	&user_mode.attr,
	NULL
};

static struct attribute *emc_domain_attrs[] = {
	&domain_name.attr,
	&cpu_heavy_thr.attr,
	&cpu_idle_thr.attr,
	&busy_ratio.attr,
	NULL
};

static struct attribute *emc_mode_attrs[] = {
	&mode_name.attr,
	&max_freq.attr,
	&change_latency.attr,
	&ldsum_thr.attr,
	&mode_enabled.attr,
	NULL
};

static const struct sysfs_ops emc_sysfs_ops = {
	.show	= show,
	.store	= store,
};

static struct kobj_type ktype_domain = {
	.sysfs_ops	= &emc_sysfs_ops,
	.default_attrs	= emc_attrs,
};

static struct kobj_type ktype_emc_domain = {
	.sysfs_ops	= &emc_sysfs_ops,
	.default_attrs	= emc_domain_attrs,
};

static struct kobj_type ktype_emc_mode = {
	.sysfs_ops	= &emc_sysfs_ops,
	.default_attrs	= emc_mode_attrs,
};

/**********************************************************************************/
/*				  Initialization				  */
/**********************************************************************************/
static void emc_boot_enable(struct work_struct *work);
static DECLARE_DELAYED_WORK(emc_boot_work, emc_boot_enable);
static void emc_boot_enable(struct work_struct *work)
{
	/* infom exynos-cpu-hotplug driver that hp governor is ready */
	emc.blocked--;
}

static int emc_pm_suspend_notifier(struct notifier_block *notifier,
				       unsigned long pm_event, void *v)
{
	struct emc_mode *mode = emc_get_base_mode();

	if (pm_event != PM_SUSPEND_PREPARE)
		return NOTIFY_OK;

	/* disable frequency boosting */
	emc.blocked++;
	emc_set_const(&mode->cpus);

	if (!emc_verify_constraints())
		BUG_ON(1);

	return NOTIFY_OK;
}

static int emc_pm_resume_notifier(struct notifier_block *notifier,
				       unsigned long pm_event, void *v)
{
	if (pm_event != PM_POST_SUSPEND)
		return NOTIFY_OK;

	/* restore frequency boosting */
	emc.blocked--;

	return NOTIFY_OK;
}

static struct notifier_block emc_suspend_nb = {
	.notifier_call = emc_pm_suspend_notifier,
	.priority = INT_MAX,
};

static struct notifier_block emc_resume_nb = {
	.notifier_call = emc_pm_resume_notifier,
	.priority = INT_MIN,
};

static void emc_print_inform(void)
{
	struct emc_mode *mode;
	struct emc_domain *domain;
	char buf[10];
	int i = 0;

	pr_info("EMC: Mode Information\n");
	pr_info("ctrl_type: %d\n", emc.ctrl_type);

	list_for_each_entry(mode, &emc.modes, list) {
		pr_info("mode%d name: %s\n", i, mode->name);
		scnprintf(buf, sizeof(buf), "%*pbl",
				cpumask_pr_args(&mode->cpus));
		pr_info("mode%d cpus: %s\n", i, buf);
		scnprintf(buf, sizeof(buf), "%*pbl",
				cpumask_pr_args(&mode->boost_cpus));
		pr_info("mode%d boost_cpus: %s\n", i, buf);
		pr_info("mode%d ldsum_thr: %u\n", i, mode->ldsum_thr);
		pr_info("mode%d cal-id: %u\n", i, mode->cal_id);
		pr_info("mode%d max_freq: %u\n", i, mode->max_freq);
		pr_info("mode%d change_latency: %u\n", i, mode->change_latency);
		pr_info("mode%d enabled: %u\n", i, mode->enabled);
		i++;
	}

	i = 0;
	pr_info("EMC: Domain Information\n");
	list_for_each_entry(domain, &emc.domains, list) {
		pr_info("domain%d name: %s\n", i, domain->name);
		scnprintf(buf, sizeof(buf), "%*pbl",
				cpumask_pr_args(&domain->cpus));
		pr_info("domain%d cpus: %s\n", i, buf);
		pr_info("domain%d role: %u\n", i, domain->role);
		pr_info("domain%d cpu_heavy_thr: %u\n", i, domain->cpu_heavy_thr);
		pr_info("domain%d cpu_idle_thr: %u\n", i, domain->cpu_idle_thr);
		pr_info("domain%d busy_ratio: %u\n", i, domain->busy_ratio);
		i++;
	}

	return;
}

static int __init emc_parse_mode(struct device_node *dn)
{
	struct emc_mode *mode;
	const char *buf;
	unsigned int val = UINT_MAX;

	mode = kzalloc(sizeof(struct emc_mode), GFP_KERNEL);
	if (!mode)
		return -ENOBUFS;

	if (of_property_read_string(dn, "mode_name", &mode->name))
		goto free;

	if (of_property_read_string(dn, "cpus", &buf))
		goto free;
	if (cpulist_parse(buf, &mode->cpus))
		goto free;

	if (!of_property_read_string(dn, "boost_cpus", &buf))
		if (cpulist_parse(buf, &mode->boost_cpus))
			goto free;

	if (!of_property_read_u32(dn, "cal-id", &mode->cal_id))
		val = cal_dfs_get_max_freq(mode->cal_id);
	if (of_property_read_u32(dn, "max_freq", &mode->max_freq))
		mode->max_freq = UINT_MAX;
	mode->max_freq = min(val, mode->max_freq);
	if (mode->max_freq == UINT_MAX)
		goto free;

	if(of_property_read_u32(dn, "change_latency", &mode->change_latency))
		goto free;

	if(of_property_read_u32(dn, "ldsum_thr", &mode->ldsum_thr))
		mode->ldsum_thr = 0;

	if (of_property_read_u32(dn, "enabled", &mode->enabled))
		goto free;

	list_add_tail(&mode->list, &emc.modes);

	return 0;
free:
	pr_warn("EMC: failed to parse emc mode\n");
	kfree(mode);
	return -EINVAL;
}

static int __init emc_parse_domain(struct device_node *dn)
{
	struct emc_domain *domain;
	const char *buf;

	domain = kzalloc(sizeof(struct emc_domain), GFP_KERNEL);
	if (!domain)
		return -ENOBUFS;

	if (of_property_read_string(dn, "domain_name", &domain->name))
		goto free;

	if (of_property_read_string(dn, "cpus", &buf))
		goto free;
	if (cpulist_parse(buf, &domain->cpus))
		goto free;

	if (of_property_read_u32(dn, "role", &domain->role))
		goto free;

	if (of_property_read_u32(dn, "cpu_heavy_thr", &domain->cpu_heavy_thr))
		goto free;

	if (of_property_read_u32(dn, "cpu_idle_thr", &domain->cpu_idle_thr))
		goto free;

	if (of_property_read_u32(dn, "busy_ratio", &domain->busy_ratio))
		goto free;

	list_add_tail(&domain->list, &emc.domains);

	return 0;
free:
	pr_warn("EMC: failed to parse emc domain\n");
	kfree(domain);
	return -EINVAL;
}

static int __init emc_parse_dt(void)
{
	struct device_node *root, *dn, *child;
	unsigned int temp;

	root = of_find_node_by_name(NULL, "exynos_mode_changer");
	if (!root)
		return -EINVAL;

	if (of_property_read_u32(root, "enabled", &temp))
		goto failure;
	if (!temp)  {
		pr_info("EMC: EMC disabled\n");
		return -1;
	}

	if (of_property_read_u32(root, "ctrl_type", &temp))
		goto failure;
	if (temp)
		emc.ctrl_type = FAST_HP;

	if (of_property_read_u32(root, "qos_class", &emc.qos_class))
		goto failure;

	/* parse emce modes */
	INIT_LIST_HEAD(&emc.modes);

	dn = of_find_node_by_name(root, "emc_modes");
	if (!dn)
		goto failure;
	for_each_child_of_node(dn, child)
		if(emc_parse_mode(child))
			goto failure;

	/* parse emce domains */
	INIT_LIST_HEAD(&emc.domains);

	dn = of_find_node_by_name(root, "emc_domains");
	if (!dn)
		goto failure;
	for_each_child_of_node(dn, child)
		if(emc_parse_domain(child))
			goto failure;

	return 0;
failure:
	pr_warn("EMC: failed to parse dt\n");
	return -EINVAL;
}

static int __init emc_mode_change_func_init(void)
{
	struct sched_param param;
	struct emc_mode *base_mode = emc_get_base_mode();

	/* register cpu mode control */
	if (exynos_cpuhp_register("EMC", base_mode->cpus, emc.ctrl_type)) {
		pr_err("EMC: Failed to register cpu control \n");
		return -EINVAL;
	}

	/* init timer */
	hrtimer_init(&emc.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	emc.timer.function = emc_mode_change_func;

	/* init workqueue */
	init_waitqueue_head(&emc.wait_q);
	init_irq_work(&emc.irq_work, emc_irq_work);

	/* create hp task */
	emc.task = kthread_create(emc_do_mode_change, NULL, "exynos_emc");
	if (IS_ERR(emc.task)) {
		pr_err("EMC: Failed to create emc_thread \n");
		return -EINVAL;
	}

	param.sched_priority = 20;
	sched_setscheduler_nocheck(emc.task, SCHED_FIFO, &param);
	set_cpus_allowed_ptr(emc.task, cpu_coregroup_mask(0));

	wake_up_process(emc.task);

	return 0;
}

static int __init __emc_sysfs_init(void)
{
	int ret;

	ret = kobject_init_and_add(&emc.kobj, &ktype_domain,
				   power_kobj, "emc");
	if (ret) {
		pr_err("EMC: failed to init emc.kobj: %d\n", ret);
		return -EINVAL;
	}

	return ret;
}

static int __init __emc_domain_sysfs_init(struct emc_domain *domain, int num)
{
	int ret;

	ret = kobject_init_and_add(&domain->kobj, &ktype_emc_domain,
				   &emc.kobj, "domain%u", num);
	if (ret) {
		pr_err("EMC: failed to init domain->kobj: %d\n", ret);
		return -EINVAL;
	}

	return 0;
}

static int __init __emc_mode_sysfs_init(struct emc_mode *mode, int num)
{
	int ret;

	ret = kobject_init_and_add(&mode->kobj, &ktype_emc_mode,
				   &emc.kobj, "mode%u", num);
	if (ret) {
		pr_err("EMC: failed to init mode->kobj: %d\n", ret);
		return -EINVAL;
	}

	return 0;
}

static int __init emc_sysfs_init(void)
{
	struct emc_domain *domain;
	struct emc_mode *mode;
	int i = 0;

	/* init attrb_lock */
	mutex_init(&emc.attrib_lock);

	/* init emc_sysfs */
	if (__emc_sysfs_init())
		goto failure;

	/* init emc domain sysfs */
	list_for_each_entry(domain, &emc.domains, list)
		if (__emc_domain_sysfs_init(domain, i++))
			goto failure;

	/* init emc mode sysfs */
	i = 0;
	list_for_each_entry(mode, &emc.modes, list)
		if (__emc_mode_sysfs_init(mode, i++))
			goto failure;

	return 0;

failure:
	return -1;
}

static int __init emc_max_freq_control_init(void)
{
	cpufreq_register_notifier(&emc_policy_nb,
			CPUFREQ_POLICY_NOTIFIER);

	pm_qos_add_notifier(emc.qos_class, &emc_qos_max_notifier);

	/* Initial pre_cpu_mask should be sync-up cpu_online_mask */
	cpumask_copy(&emc.pre_cpu_mask, cpu_online_mask);
	cpumask_copy(&emc.pwr_cpu_mask, cpu_online_mask);

	cpuhp_setup_state_nocalls(CPUHP_EXYNOS_BOOST_CTRL_POST,
					"exynos_boost_ctrl_post",
					emc_cpu_on_callback,
					emc_cpu_off_callback);
	cpuhp_setup_state_nocalls(CPUHP_EXYNOS_BOOST_CTRL_PRE,
					"exynos_boost_ctrl_pre",
					emc_cpu_pre_on_callback,
					emc_cpu_pre_off_callback);

	return 0;
}

static bool __init emc_boostable(void)
{
	struct emc_mode *mode;
	unsigned int freq = 0;

	if (cpumask_weight(cpu_online_mask) != NR_CPUS)
		return false;

	list_for_each_entry(mode, &emc.modes, list)
		freq = max(freq, mode->max_freq);
	if (freq > emc_get_base_mode()->max_freq)
		return true;

	return false;
}

static void __init emc_pm_init(void)
{
	/* register pm notifier */
	register_pm_notifier(&emc_suspend_nb);
	register_pm_notifier(&emc_resume_nb);
}

static int __init emc_init(void)
{
	/* parse dt */
	if (emc_parse_dt())
		goto failure;

	/* check whether system is boostable or not */
	if (!emc_boostable()) {
		pr_info("EMC: Doesn't support boosting\n");
		return 0;
	}
	emc.max_freq = emc_get_base_mode()->max_freq;

	/* init sysfs */
	if (emc_sysfs_init())
		goto failure;

	/* init max frequency control */
	if (emc_max_freq_control_init())
		goto failure;

	/* init mode change func */
	if (emc_mode_change_func_init())
		goto failure;

	/* init pm */
	emc_pm_init();

	/* show emc infromation */
	emc_print_inform();

	/* enable */
	emc.blocked = 1;	/* it is released after boot lock time */
	emc_set_enable(true);

	/* keep the base mode during boot time */
	schedule_delayed_work_on(0, &emc_boot_work,
			msecs_to_jiffies(DEFAULT_BOOT_ENABLE_MS));

	return 0;

failure:
	pr_warn("EMC: Initialization failed \n");
	return 0;
} subsys_initcall(emc_init);
