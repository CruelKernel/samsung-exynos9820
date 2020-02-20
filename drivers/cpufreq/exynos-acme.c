/*
 * Copyright (c) 2016 Park Bumgyu, Samsung Electronics Co., Ltd <bumgyu.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Exynos ACME(A Cpufreq that Meets Every chipset) driver implementation
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/debug-snapshot.h>
#include <linux/pm_opp.h>
#include <linux/cpu_cooling.h>
#include <linux/suspend.h>
#include <linux/ems.h>

#include <soc/samsung/cal-if.h>
#include <soc/samsung/exynos-dm.h>
#include <soc/samsung/ect_parser.h>
#include <soc/samsung/exynos-cpuhp.h>
#include <soc/samsung/exynos-cpupm.h>
#include <soc/samsung/exynos-emc.h>
#include <soc/samsung/exynos-alt.h>

#include "exynos-acme.h"

/*
 * list head of cpufreq domain
 */
LIST_HEAD(domains);

/*
 * list head of units which have cpufreq policy dependancy
 */
LIST_HEAD(ready_list);

/*********************************************************************
 *                          HELPER FUNCTION                          *
 *********************************************************************/
static struct exynos_cpufreq_domain *find_domain(unsigned int cpu)
{
	struct exynos_cpufreq_domain *domain;

	list_for_each_entry(domain, &domains, list)
		if (cpumask_test_cpu(cpu, &domain->cpus))
			return domain;

	pr_err("cannot find cpufreq domain by cpu\n");
	return NULL;
}

static
struct exynos_cpufreq_domain *find_domain_pm_qos_class(int pm_qos_class)
{
	struct exynos_cpufreq_domain *domain;

	list_for_each_entry(domain, &domains, list)
		if (domain->pm_qos_min_class == pm_qos_class ||
			domain->pm_qos_max_class == pm_qos_class)
			return domain;

	pr_err("cannot find cpufreq domain by PM QoS class\n");
	return NULL;
}

struct
exynos_cpufreq_domain *find_domain_cpumask(const struct cpumask *mask)
{
	struct exynos_cpufreq_domain *domain;

	list_for_each_entry(domain, &domains, list)
		if (cpumask_intersects(mask, &domain->cpus))
			return domain;

	pr_err("cannot find cpufreq domain by cpumask\n");
	return NULL;
}

struct list_head *get_domain_list(void)
{
	return &domains;
}

struct exynos_cpufreq_domain *first_domain(void)
{
	return list_first_entry(&domains,
			struct exynos_cpufreq_domain, list);
}

struct exynos_cpufreq_domain *last_domain(void)
{
	return list_last_entry(&domains,
			struct exynos_cpufreq_domain, list);
}

int exynos_cpufreq_domain_count(void)
{
	return last_domain()->id + 1;
}

/* __enable_domain/__disable_domain MUST be called with holding domain->lock */
static inline void __enable_domain(struct exynos_cpufreq_domain *domain)
{
	domain->enabled = true;
}

static inline void __disable_domain(struct exynos_cpufreq_domain *domain)
{
	domain->enabled = false;
}

static void enable_domain(struct exynos_cpufreq_domain *domain)
{
	mutex_lock(&domain->lock);
	__enable_domain(domain);
	mutex_unlock(&domain->lock);
}

static void disable_domain(struct exynos_cpufreq_domain *domain)
{
	mutex_lock(&domain->lock);
	__disable_domain(domain);
	mutex_unlock(&domain->lock);
}

static bool static_governor(struct cpufreq_policy *policy)
{
	/*
	 * During cpu hotplug in sequence, governor can be empty for
	 * a while. In this case, we regard governor as default
	 * governor. Exynos never use static governor as default.
	 */
	if (!policy->governor)
		return false;

	if ((strcmp(policy->governor->name, "userspace") == 0)
		|| (strcmp(policy->governor->name, "performance") == 0)
		|| (strcmp(policy->governor->name, "powersave") == 0))
		return true;

	return false;
}

static unsigned int index_to_freq(struct cpufreq_frequency_table *table,
					unsigned int index)
{
	return table[index].frequency;
}


/*********************************************************************
 *                         FREQUENCY SCALING                         *
 *********************************************************************/
/*
 * Depending on cluster structure, it cannot be possible to get/set
 * cpu frequency while cluster is off. For this, disable cluster-wide
 * power mode while getting/setting frequency.
 */
static unsigned int get_freq(struct exynos_cpufreq_domain *domain)
{
	int wakeup_flag = 0;
	unsigned int freq;
	struct cpumask temp;

	cpumask_and(&temp, &domain->cpus, cpu_active_mask);

	if (cpumask_empty(&temp))
		return domain->old;

	if (domain->need_awake) {
		if (likely(domain->old))
			return domain->old;

		wakeup_flag = 1;
		disable_power_mode(cpumask_any(&domain->cpus), POWERMODE_TYPE_CLUSTER);
	}

	freq = (unsigned int)cal_dfs_get_rate(domain->cal_id);
	if (!freq) {
		/* On changing state, CAL returns 0 */
		freq = domain->old;
	}

	if (unlikely(wakeup_flag))
		enable_power_mode(cpumask_any(&domain->cpus), POWERMODE_TYPE_CLUSTER);

	return freq;
}

static int set_freq(struct exynos_cpufreq_domain *domain,
					unsigned int target_freq)
{
	int err;

	dbg_snapshot_printk("ID %d: %d -> %d (%d)\n",
		domain->id, domain->old, target_freq, DSS_FLAG_IN);

	if (domain->need_awake)
		disable_power_mode(cpumask_any(&domain->cpus), POWERMODE_TYPE_CLUSTER);

	/* check target freq is available */
	emc_check_available_freq(&domain->cpus, target_freq);
	err = cal_dfs_set_rate(domain->cal_id, target_freq);
	if (err < 0)
		pr_err("failed to scale frequency of domain%d (%d -> %d)\n",
			domain->id, domain->old, target_freq);

	if (domain->need_awake)
		enable_power_mode(cpumask_any(&domain->cpus), POWERMODE_TYPE_CLUSTER);

	dbg_snapshot_printk("ID %d: %d -> %d (%d)\n",
		domain->id, domain->old, target_freq, DSS_FLAG_OUT);

	return err;
}

static unsigned int apply_pm_qos(struct exynos_cpufreq_domain *domain,
					struct cpufreq_policy *policy,
					unsigned int target_freq)
{
	unsigned int freq;
	int qos_min, qos_max;

	/*
	 * In case of static governor, it should garantee to scale to
	 * target, it does not apply PM QoS.
	 */
	if (static_governor(policy))
		return target_freq;

	qos_min = pm_qos_request(domain->pm_qos_min_class);
	qos_max = pm_qos_request(domain->pm_qos_max_class);

	if (qos_min < 0 || qos_max < 0)
		return target_freq;

	freq = max((unsigned int)qos_min, target_freq);
	freq = min((unsigned int)qos_max, freq);

	return freq;
}

static int pre_scale(void)
{
	return 0;
}

static int post_scale(void)
{
	return 0;
}

static int scale(struct exynos_cpufreq_domain *domain,
				struct cpufreq_policy *policy,
				unsigned int target_freq)
{
	int ret;
	struct cpufreq_freqs freqs = {
		.cpu		= policy->cpu,
		.old		= domain->old,
		.new		= target_freq,
		.flags		= 0,
	};

	cpufreq_freq_transition_begin(policy, &freqs);
	dbg_snapshot_freq(domain->id, domain->old, target_freq, DSS_FLAG_IN);

	ret = pre_scale();
	if (ret)
		goto fail_scale;

	/* Scale frequency by hooked function, set_freq() */
	ret = set_freq(domain, target_freq);
	if (ret)
		goto fail_scale;

	ret = post_scale();
	if (ret)
		goto fail_scale;

fail_scale:
	/* In scaling failure case, logs -1 to exynos snapshot */
	dbg_snapshot_freq(domain->id, domain->old, target_freq,
					ret < 0 ? ret : DSS_FLAG_OUT);
	cpufreq_freq_transition_end(policy, &freqs, ret);

	return ret;
}

static int update_freq(struct exynos_cpufreq_domain *domain,
					 unsigned int freq)
{
	struct cpufreq_policy *policy;
	int ret;
	struct cpumask mask;

	pr_debug("update frequency of domain%d to %d kHz\n",
						domain->id, freq);

	cpumask_and(&mask, &domain->cpus, cpu_online_mask);
	if (cpumask_empty(&mask))
		return -ENODEV;
	policy = cpufreq_cpu_get(cpumask_first(&mask));
	if (!policy)
		return -EINVAL;

	if (static_governor(policy)) {
		cpufreq_cpu_put(policy);
		return 0;
	}

	ret = cpufreq_driver_target(policy, freq, CPUFREQ_RELATION_H);
	cpufreq_cpu_put(policy);

	return ret;
}

/*********************************************************************
 *                   EXYNOS CPUFREQ DRIVER INTERFACE                 *
 *********************************************************************/
static int exynos_cpufreq_driver_init(struct cpufreq_policy *policy)
{
	struct exynos_cpufreq_domain *domain = find_domain(policy->cpu);
	int ret;

	if (!domain)
		return -EINVAL;

	ret = cpufreq_table_validate_and_show(policy, domain->freq_table);
	if (ret) {
		pr_err("%s: invalid frequency table: %d\n", __func__, ret);
		return ret;
	}

	policy->cur = get_freq(domain);
	policy->cpuinfo.transition_latency = TRANSITION_LATENCY;
	cpumask_copy(policy->cpus, &domain->cpus);

	pr_info("CPUFREQ domain%d registered\n", domain->id);

	return 0;
}

static int exynos_cpufreq_verify(struct cpufreq_policy *policy)
{
	struct exynos_cpufreq_domain *domain = find_domain(policy->cpu);

	if (!domain)
		return -EINVAL;

	return cpufreq_frequency_table_verify(policy, domain->freq_table);
}

static int __exynos_cpufreq_target(struct cpufreq_policy *policy,
				  unsigned int target_freq,
				  unsigned int relation)
{
	struct exynos_cpufreq_domain *domain = find_domain(policy->cpu);
	int index;
	int ret = 0;

	if (!domain)
		return -EINVAL;

	mutex_lock(&domain->lock);

	if (!domain->enabled)
		goto out;

	if (domain->old != get_freq(domain)) {
		pr_err("oops, inconsistency between domain->old:%d, real clk:%d\n",
			domain->old, get_freq(domain));
		BUG_ON(1);
	}

	/*
	 * Update target_freq.
	 * Updated target_freq is in between minimum and maximum PM QoS/policy,
	 * priority of policy is higher.
	 */
	index = cpufreq_frequency_table_target(policy, target_freq, relation);
	if (index < 0) {
		pr_err("target frequency(%d) out of range\n", target_freq);
		goto out;
	}

	target_freq = index_to_freq(domain->freq_table, index);

	/* Target is same as current, skip scaling */
	if (domain->old == target_freq)
		goto out;

	ret = scale(domain, policy, target_freq);
	if (ret)
		goto out;

	pr_debug("CPUFREQ domain%d frequency change %u kHz -> %u kHz\n",
			domain->id, domain->old, target_freq);

	domain->old = target_freq;
	arch_set_freq_scale(&domain->cpus, target_freq, policy->max);

out:
	mutex_unlock(&domain->lock);

	return ret;
}

static unsigned int exynos_cpufreq_get(unsigned int cpu)
{
	struct exynos_cpufreq_domain *domain = find_domain(cpu);

	if (!domain)
		return 0;

	return get_freq(domain);
}

static int exynos_cpufreq_target(struct cpufreq_policy *policy,
					unsigned int target_freq,
					unsigned int relation)
{
	struct exynos_cpufreq_domain *domain = find_domain(policy->cpu);
	unsigned long freq;
	int index;
	unsigned int policy_min, policy_max;
	unsigned int pm_qos_min, pm_qos_max;

	if (!domain)
		return -EINVAL;

	if (!domain->enabled)
		return -EINVAL;

	target_freq = apply_pm_qos(domain, policy, target_freq);

	if (list_empty(&domain->dm_list))
		return __exynos_cpufreq_target(policy, target_freq, relation);

	index = cpufreq_frequency_table_target(policy, target_freq, relation);
	if (index < 0) {
		pr_err("target frequency(%d) out of range\n", target_freq);
		return 0;
	}

	mutex_lock(&domain->lock);

	freq = (unsigned long)index_to_freq(domain->freq_table, index);
	if (domain->old == freq) {
		mutex_unlock(&domain->lock);
		return 0;
	}
	mutex_unlock(&domain->lock);

	policy_min = policy->min;
	policy_max = policy->max;

	pm_qos_min = pm_qos_request(domain->pm_qos_min_class);
	pm_qos_max = pm_qos_request(domain->pm_qos_max_class);

	freq = (unsigned long)target_freq;

	exynos_alt_call_chain();

	return policy_update_with_DM_CALL(domain->dm_type, max(policy_min, pm_qos_min),
				min(policy_max, pm_qos_max), &freq);
}

static int __exynos_cpufreq_suspend(struct exynos_cpufreq_domain *domain)
{
	unsigned int freq;

	if (!domain)
		return -EINVAL;

	/* To handle reboot faster, it does not thrrotle frequency of domain0 */
	if (system_state == SYSTEM_RESTART && domain->id != 0)
		freq = domain->min_freq;
	else
		freq = domain->resume_freq;

	pm_qos_update_request(&domain->min_qos_req, freq);
	pm_qos_update_request(&domain->max_qos_req, freq);

	/* To sync current freq with resume freq, check until they become same */
	mutex_lock(&domain->lock);
	while (domain->old > freq) {
		mutex_unlock(&domain->lock);
		update_freq(domain, freq);
		mutex_lock(&domain->lock);
	}

	/*
	 * Although cpufreq governor is stopped in cpufreq_suspend(),
	 * afterwards, frequency change can be requested by
	 * PM QoS. To prevent chainging frequency after
	 * cpufreq suspend, disable scaling for all domains.
	 */
	__disable_domain(domain);
	mutex_unlock(&domain->lock);

	return 0;
}

static int exynos_cpufreq_suspend(struct cpufreq_policy *policy)
{
	struct exynos_cpufreq_domain *domain = find_domain(policy->cpu);

	return __exynos_cpufreq_suspend(domain);
}

static int __exynos_cpufreq_resume(struct exynos_cpufreq_domain *domain)
{
	if (!domain)
		return -EINVAL;

	enable_domain(domain);

	pm_qos_update_request(&domain->min_qos_req, domain->min_freq);
	pm_qos_update_request(&domain->max_qos_req, domain->max_freq);

	return 0;
}

static int exynos_cpufreq_resume(struct cpufreq_policy *policy)
{
	struct exynos_cpufreq_domain *domain = find_domain(policy->cpu);

	return __exynos_cpufreq_resume(domain);
}

static void exynos_cpufreq_ready(struct cpufreq_policy *policy)
{
	struct exynos_cpufreq_ready_block *ready_block;

	list_for_each_entry(ready_block, &ready_list, list) {
		if (ready_block->update)
			ready_block->update(policy);
		if (ready_block->get_target)
			ready_block->get_target(policy, exynos_cpufreq_target);
	}
}

static int exynos_cpufreq_exit(struct cpufreq_policy *policy)
{
	return 0;
}

static int exynos_cpufreq_pm_notifier(struct notifier_block *notifier,
				       unsigned long pm_event, void *v)
{
	struct exynos_cpufreq_domain *domain;

	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
		list_for_each_entry_reverse(domain, &domains, list)
			if (__exynos_cpufreq_suspend(domain))
				return NOTIFY_BAD;
		break;
	case PM_POST_SUSPEND:
		list_for_each_entry_reverse(domain, &domains, list)
			if (__exynos_cpufreq_resume(domain))
				return NOTIFY_BAD;
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block exynos_cpufreq_pm = {
	.notifier_call = exynos_cpufreq_pm_notifier,
};

static struct cpufreq_driver exynos_driver = {
	.name		= "exynos_cpufreq",
	.flags		= CPUFREQ_STICKY | CPUFREQ_HAVE_GOVERNOR_PER_POLICY,
	.init		= exynos_cpufreq_driver_init,
	.verify		= exynos_cpufreq_verify,
	.target		= exynos_cpufreq_target,
	.get		= exynos_cpufreq_get,
	.suspend	= exynos_cpufreq_suspend,
	.resume		= exynos_cpufreq_resume,
	.ready		= exynos_cpufreq_ready,
	.exit		= exynos_cpufreq_exit,
	.attr		= cpufreq_generic_attr,
};

/*********************************************************************
 *                      SUPPORT for DVFS MANAGER                     *
 *********************************************************************/

static int dm_scaler(int dm_type, void *devdata, unsigned int target_freq,
						unsigned int relation)
{
	struct exynos_cpufreq_domain *domain = devdata;
	struct cpufreq_policy *policy;
	struct cpumask mask;
	int ret;

	/* Skip scaling if all cpus of domain are hotplugged out */
	cpumask_and(&mask, &domain->cpus, cpu_online_mask);
	if (cpumask_empty(&mask))
		return -ENODEV;

	if (relation == EXYNOS_DM_RELATION_L)
		relation = CPUFREQ_RELATION_L;
	else
		relation = CPUFREQ_RELATION_H;

	policy = cpufreq_cpu_get(cpumask_first(&mask));
	if (!policy) {
		pr_err("%s: failed get cpufreq policy\n", __func__);
		return -ENODEV;
	}

	ret = __exynos_cpufreq_target(policy, target_freq, relation);

	cpufreq_cpu_put(policy);

	return ret;
}

/*********************************************************************
 *                       CPUFREQ PM QOS HANDLER                      *
 *********************************************************************/
static int need_update_freq(struct exynos_cpufreq_domain *domain,
				int pm_qos_class, unsigned int freq)
{
	unsigned int cur = get_freq(domain);

	if (cur == freq)
		return 0;

	if (pm_qos_class == domain->pm_qos_min_class) {
		if (cur > freq)
			return 0;
	} else if (domain->pm_qos_max_class == pm_qos_class) {
		if (cur < freq)
			return 0;
	} else {
		/* invalid PM QoS class */
		return -EINVAL;
	}

	return 1;
}

static int exynos_cpufreq_pm_qos_callback(struct notifier_block *nb,
					unsigned long val, void *v)
{
	int pm_qos_class = *((int *)v);
	struct exynos_cpufreq_domain *domain;
	struct cpufreq_policy *policy;
	struct cpumask mask;
	int ret;

	pr_debug("update PM QoS class %d to %ld kHz\n", pm_qos_class, val);

	domain = find_domain_pm_qos_class(pm_qos_class);
	if (!domain)
		return NOTIFY_BAD;

	cpumask_and(&mask, &domain->cpus, cpu_online_mask);
	if (cpumask_empty(&mask))
		return NOTIFY_BAD;

	policy = cpufreq_cpu_get(cpumask_first(&mask));
	if (!policy)
		return NOTIFY_BAD;

	if (pm_qos_class == domain->pm_qos_max_class)
		update_qos_capacity(cpumask_first(&domain->cpus), val, policy->cpuinfo.max_freq);

	ret = need_update_freq(domain, pm_qos_class, val);
	if (ret < 0)
		return NOTIFY_BAD;
	if (!ret)
		return NOTIFY_OK;

	if (update_freq(domain, val))
		return NOTIFY_BAD;

	return NOTIFY_OK;
}

/*********************************************************************
 *                       EXTERNAL EVENT HANDLER                      *
 *********************************************************************/
static int exynos_cpufreq_policy_callback(struct notifier_block *nb,
				unsigned long event, void *data)
{
	struct cpufreq_policy *policy = data;
	struct exynos_cpufreq_domain *domain = find_domain(policy->cpu);

	if (!domain)
		return NOTIFY_OK;

	switch (event) {
	case CPUFREQ_NOTIFY:
		arch_set_freq_scale(&domain->cpus, domain->old, policy->max);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block exynos_cpufreq_policy_notifier = {
	.notifier_call = exynos_cpufreq_policy_callback,
};

extern bool cpuhp_tasks_frozen;
static int exynos_cpufreq_cpu_up_callback(unsigned int cpu)
{
	struct exynos_cpufreq_domain *domain;
	struct cpumask mask;

	/*
	 * CPU frequency is not changed before cpufreq_resume() is called.
	 * Therefore, if this callback is called by enable_nonboot_cpus(),
	 * it is ignored.
	 */
	if (cpuhp_tasks_frozen)
		return 0;

	domain = find_domain(cpu);
	if (!domain)
		return -ENODEV;

	/*
	 * The first incomming cpu in domain enables frequency scaling
	 * and clears limit of frequency.
	 */
	cpumask_and(&mask, &domain->cpus, cpu_online_mask);
	if (cpumask_weight(&mask) == 1) {
		enable_domain(domain);
		pm_qos_update_request(&domain->max_qos_req, domain->max_freq);
	}

	return 0;
}

static int exynos_cpufreq_cpu_down_callback(unsigned int cpu)
{
	struct exynos_cpufreq_domain *domain;
	struct cpumask mask;

	/*
	 * CPU frequency is not changed after cpufreq_suspend() is called.
	 * Therefore, if this callback is called by disable_nonboot_cpus(),
	 * it is ignored.
	 */
	if (cpuhp_tasks_frozen)
		return 0;

	domain = find_domain(cpu);
	if (!domain)
		return -ENODEV;

	/*
	 * The last outgoing cpu in domain limits frequency to minimum
	 * and disables frequency scaling.
	 */
	cpumask_and(&mask, &domain->cpus, cpu_online_mask);
	if (cpumask_weight(&mask) == 1) {
		pm_qos_update_request(&domain->max_qos_req, domain->min_freq);
		disable_domain(domain);
	}

	return 0;
}

/*********************************************************************
 *                       EXTERNAL REFERENCE APIs                     *
 *********************************************************************/
unsigned int exynos_cpufreq_get_max_freq(struct cpumask *mask)
{
	struct exynos_cpufreq_domain *domain = find_domain_cpumask(mask);

	return domain->max_freq;
}
EXPORT_SYMBOL(exynos_cpufreq_get_max_freq);

unsigned int exynos_cpufreq_get_locked(unsigned int cpu)
{
	struct exynos_cpufreq_domain *domain = find_domain(cpu);

	if (!domain)
		return 0;

	/*
	 * It is accompanied by a lock job to prevent
	 * reading frequency during frequency change
	 */
	mutex_lock(&domain->lock);
	mutex_unlock(&domain->lock);

	return get_freq(domain);
}
EXPORT_SYMBOL(exynos_cpufreq_get_locked);

void exynos_cpufreq_ready_list_add(struct exynos_cpufreq_ready_block *rb)
{
	if (!rb)
		return;

	list_add(&rb->list, &ready_list);
}
EXPORT_SYMBOL(exynos_cpufreq_ready_list_add);

unsigned int __weak exynos_pstate_get_boost_freq(int cpu)
{
	return 0;
}
EXPORT_SYMBOL(exynos_pstate_get_boost_freq);

#ifdef CONFIG_SEC_BOOTSTAT
void sec_bootstat_get_cpuinfo(int *freq, int *online)
{
	int cpu;
	int cluster;
	struct exynos_cpufreq_domain *domain;

	get_online_cpus();
	*online = cpumask_bits(cpu_online_mask)[0];
	for_each_online_cpu(cpu) {
		domain = find_domain(cpu);
		if (!domain)
			continue;
		pr_err("%s, dm type = %d\n", __func__, domain->dm_type);
		cluster = 0;
		if (domain->dm_type == DM_CPU_CL1)
			cluster = 1;
		else if (domain->dm_type == DM_CPU_CL2)
			cluster = 2;

		freq[cluster] = get_freq(domain);
	}
	put_online_cpus();
}
#endif

DECLARE_PER_CPU(struct freqvariant_idlefactor, fv_ifactor);
extern unsigned int default_freqvar_ifactor[];

/*
 * freqvar_ifs format :
 * {DomainID} {ratio} {freq}:{ratio} {freq}:{ratio} ...
 * converted into array like [DomainID][ratio][freq][ratio]...
 */
static unsigned int *get_tokenized_data(const char *buf, int *num_tokens, unsigned int* domainId)
{
	const char *cp;
	int i;
	int ntokens = 1;
	unsigned int *tokenized_data;
	int err = -EINVAL;

	cp = buf;
	while ((cp = strpbrk(cp + 1, " :")))
		ntokens++;

	if ((ntokens & 0x1))
		goto err;

	tokenized_data = kmalloc((ntokens - 1) * sizeof(unsigned int), GFP_KERNEL);
	if (!tokenized_data) {
		err = -ENOMEM;
		goto err;
	}

	cp = buf;
	i = 0;

	if (sscanf(cp, "%u", domainId) != 1)
		goto err_kfree;

	if (*domainId > 2)
		goto err_kfree;

	cp = strpbrk(cp, " ");
	if (!cp)
		goto err_kfree;
	cp++;

	while (i < (ntokens - 1)) {
		if (sscanf(cp, "%u", &tokenized_data[i++]) != 1)
			goto err_kfree;

		if ((i & 0x1) == 1 &&
			(tokenized_data[i-1] > 1000 || tokenized_data[i-1] < 10))
			goto err_kfree;
		cp = strpbrk(cp, " :");
		if (!cp)
			break;
		cp++;
	}

	if (i != (ntokens - 1))
		goto err_kfree;

	*num_tokens = ntokens - 1;
	return tokenized_data;

err_kfree:
	kfree(tokenized_data);
err:
	return ERR_PTR(err);
}

static ssize_t show_freqvar_idlelatency(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	struct exynos_cpufreq_domain *domain;
	ssize_t count = 0;
	int i, headCPU = -1;
	struct freqvariant_idlefactor *pfv_factor;
	unsigned long flags;

	list_for_each_entry(domain, &domains, list) {
		if (headCPU != cpumask_first(&domain->cpus))
			headCPU = cpumask_first(&domain->cpus);
		else
			continue;

		pfv_factor = &per_cpu(fv_ifactor, headCPU);

		count += sprintf(buf + count, "%u ", domain->id);

		spin_lock_irqsave(&pfv_factor->freqvar_if_lock, flags);
		for (i = 0; i < pfv_factor->nfreqvar_ifs; i++)
			count += sprintf(buf + count, "%u%s", pfv_factor->freqvar_ifs[i],
					i & 0x1 ? ":" : " ");
		spin_unlock_irqrestore(&pfv_factor->freqvar_if_lock, flags);

		count += sprintf(buf + count, "\n");
	}

	return count;

}

static ssize_t store_freqvar_idlelatency(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	struct exynos_cpufreq_domain *domain;
	int ntokens;
	unsigned int *new_freqvar_ifs = NULL;
	unsigned long flags;
	int ifs_updated = 0;
	unsigned int cpu, target_domain = NR_CPUS;
	struct freqvariant_idlefactor *pfv_factor;

	new_freqvar_ifs = get_tokenized_data(buf, &ntokens, &target_domain);
	if (IS_ERR(new_freqvar_ifs))
		return PTR_RET(new_freqvar_ifs);

	if (target_domain >= NR_CPUS) {
		pr_info("Wrong format: domain_id ratio freq:ratio freq:ratio ...\n");
		return 0;
	}

	list_for_each_entry(domain, &domains, list) {
		if (domain->id != target_domain)
			continue;

		for_each_cpu(cpu, &domain->cpus) {
			pfv_factor = &per_cpu(fv_ifactor, cpu);
			spin_lock_irqsave(&pfv_factor->freqvar_if_lock, flags);
			if (ifs_updated != 1 && pfv_factor->freqvar_ifs != default_freqvar_ifactor) {
				kfree(pfv_factor->freqvar_ifs);
				ifs_updated = 1;
			}
			pfv_factor->freqvar_ifs = new_freqvar_ifs;
			pfv_factor->nfreqvar_ifs = ntokens;
			spin_unlock_irqrestore(&pfv_factor->freqvar_if_lock, flags);
		}
	}
	return count;
}

static struct kobj_attribute freqvar_idlelatency =
__ATTR(freqvar_idlelatency, S_IRUGO | S_IWUSR,
		show_freqvar_idlelatency, store_freqvar_idlelatency);


/*********************************************************************
 *                  INITIALIZE EXYNOS CPUFREQ DRIVER                 *
 *********************************************************************/
static void print_domain_info(struct exynos_cpufreq_domain *domain)
{
	int i;
	char buf[10];

	pr_info("CPUFREQ of domain%d cal-id : %#x\n",
			domain->id, domain->cal_id);

	scnprintf(buf, sizeof(buf), "%*pbl", cpumask_pr_args(&domain->cpus));
	pr_info("CPUFREQ of domain%d sibling cpus : %s\n",
			domain->id, buf);

	pr_info("CPUFREQ of domain%d boot freq = %d kHz, resume freq = %d kHz\n",
			domain->id, domain->boot_freq, domain->resume_freq);

	pr_info("CPUFREQ of domain%d max freq : %d kHz, min freq : %d kHz\n",
			domain->id,
			domain->max_freq, domain->min_freq);

	pr_info("CPUFREQ of domain%d PM QoS max-class-id : %d, min-class-id : %d\n",
			domain->id,
			domain->pm_qos_max_class, domain->pm_qos_min_class);

	pr_info("CPUFREQ of domain%d table size = %d\n",
			domain->id, domain->table_size);

	for (i = 0; i < domain->table_size; i++) {
		if (domain->freq_table[i].frequency == CPUFREQ_ENTRY_INVALID)
			continue;

		pr_info("CPUFREQ of domain%d : L%2d  %7d kHz\n",
			domain->id,
			domain->freq_table[i].driver_data,
			domain->freq_table[i].frequency);
	}
}

static __init void init_sysfs(void)
{
	if (sysfs_create_file(power_kobj, &freqvar_idlelatency.attr))
		pr_err("failed to create freqvar_idlelatency node\n");

}

static __init int init_table(struct exynos_cpufreq_domain *domain)
{
	unsigned int index;
	unsigned long *table;
	unsigned int *volt_table;
	struct exynos_cpufreq_dm *dm;
	struct exynos_ufc *ufc;
	int cpu;
	int ret = 0;

	/*
	 * Initialize frequency and voltage table of domain.
	 * Allocate temporary table to get DVFS table from CAL.
	 * Deliver this table to CAL API, then CAL fills the information.
	 */
	table = kzalloc(sizeof(unsigned long) * domain->table_size, GFP_KERNEL);
	if (!table)
		return -ENOMEM;

	volt_table = kzalloc(sizeof(unsigned int) * domain->table_size, GFP_KERNEL);
	if (!volt_table) {
		ret = -ENOMEM;
		goto free_table;
	}

	cal_dfs_get_rate_table(domain->cal_id, table);
	cal_dfs_get_asv_table(domain->cal_id, volt_table);

	for (index = 0; index < domain->table_size; index++) {
		domain->freq_table[index].driver_data = index;

		if (table[index] > domain->max_freq)
			domain->freq_table[index].frequency = CPUFREQ_ENTRY_INVALID;
		else if (table[index] < domain->min_freq)
			domain->freq_table[index].frequency = CPUFREQ_ENTRY_INVALID;
		else {
			struct cpumask mask;
			domain->freq_table[index].frequency = table[index];
			/* Add OPP table to first cpu of domain */
			cpumask_and(&mask, &domain->cpus, cpu_online_mask);
			for_each_cpu(cpu, &mask) {
				if (!get_cpu_device(cpu))
					continue;

				dev_pm_opp_add(get_cpu_device(cpu),
						table[index] * 1000, volt_table[index]);
			}
		}

		/* Initialize table of DVFS manager constraint */
		list_for_each_entry(dm, &domain->dm_list, list)
			dm->c.freq_table[index].master_freq = table[index];

		/* Initialize table of UFC */
		list_for_each_entry(ufc, &domain->ufc_list, list)
			ufc->info.freq_table[index].master_freq =
						domain->freq_table[index].frequency;
	}
	domain->freq_table[index].driver_data = index;
	domain->freq_table[index].frequency = CPUFREQ_TABLE_END;

	init_sched_energy_table(&domain->cpus, domain->table_size, table, volt_table,
				domain->max_freq, domain->min_freq);

	kfree(volt_table);

free_table:
	kfree(table);

	return ret;
}

static __init void set_boot_qos(struct exynos_cpufreq_domain *domain)
{
	unsigned int boot_qos, val;
	struct device_node *dn = domain->dn;

	/*
	 * Basically booting pm_qos is set to max frequency of domain.
	 * But if pm_qos-booting exists in device tree,
	 * booting pm_qos is selected to smaller one
	 * between max frequency of domain and the value defined in device tree.
	 */
	boot_qos = domain->max_freq;
	if (!of_property_read_u32(dn, "pm_qos-booting", &val))
		boot_qos = min(boot_qos, val);

	pm_qos_update_request_timeout(&domain->min_qos_req,
			boot_qos, 40 * USEC_PER_SEC);
	pm_qos_update_request_timeout(&domain->max_qos_req,
			boot_qos, 40 * USEC_PER_SEC);

}

static __init int init_pm_qos(struct exynos_cpufreq_domain *domain,
					struct device_node *dn)
{
	int ret;

	ret = of_property_read_u32(dn, "pm_qos-min-class",
					&domain->pm_qos_min_class);
	if (ret)
		return ret;

	ret = of_property_read_u32(dn, "pm_qos-max-class",
					&domain->pm_qos_max_class);
	if (ret)
		return ret;

	domain->pm_qos_min_notifier.notifier_call = exynos_cpufreq_pm_qos_callback;
	domain->pm_qos_min_notifier.priority = INT_MAX;
	domain->pm_qos_max_notifier.notifier_call = exynos_cpufreq_pm_qos_callback;
	domain->pm_qos_max_notifier.priority = INT_MAX;

	pm_qos_add_notifier(domain->pm_qos_min_class,
				&domain->pm_qos_min_notifier);
	pm_qos_add_notifier(domain->pm_qos_max_class,
				&domain->pm_qos_max_notifier);

	pm_qos_add_request(&domain->min_qos_req,
			domain->pm_qos_min_class, domain->min_freq);
	pm_qos_add_request(&domain->max_qos_req,
			domain->pm_qos_max_class, domain->max_freq);

	return 0;
}

static int init_constraint_table_ect(struct exynos_cpufreq_domain *domain,
					struct exynos_cpufreq_dm *dm,
					struct device_node *dn)
{
	void *block;
	struct ect_minlock_domain *ect_domain;
	const char *ect_name;
	unsigned int index, c_index;
	bool valid_row = false;
	int ret;

	ret = of_property_read_string(dn, "ect-name", &ect_name);
	if (ret)
		return ret;

	block = ect_get_block(BLOCK_MINLOCK);
	if (!block)
		return -ENODEV;

	ect_domain = ect_minlock_get_domain(block, (char *)ect_name);
	if (!ect_domain)
		return -ENODEV;

	for (index = 0; index < domain->table_size; index++) {
		unsigned int freq = domain->freq_table[index].frequency;

		for (c_index = 0; c_index < ect_domain->num_of_level; c_index++) {
			/* find row same as frequency */
			if (freq == ect_domain->level[c_index].main_frequencies) {
				dm->c.freq_table[index].constraint_freq
					= ect_domain->level[c_index].sub_frequencies;
				valid_row = true;
				break;
			}
		}

		/*
		 * Due to higher levels of constraint_freq should not be NULL,
		 * they should be filled with highest value of sub_frequencies of ect
		 * until finding first(highest) domain frequency fit with main_frequeucy of ect.
		 */
		if (!valid_row)
			dm->c.freq_table[index].constraint_freq
				= ect_domain->level[0].sub_frequencies;
	}

	return 0;
}

static int init_constraint_table_dt(struct exynos_cpufreq_domain *domain,
					struct exynos_cpufreq_dm *dm,
					struct device_node *dn)
{
	struct exynos_dm_freq *table;
	int size, index, c_index;

	/*
	 * A DVFS Manager table row consists of CPU and MIF frequency
	 * value, the size of a row is 64bytes. Divide size in half when
	 * table is allocated.
	 */
	size = of_property_count_u32_elems(dn, "table");
	if (size < 0)
		return size;

	table = kzalloc(sizeof(struct exynos_dm_freq) * size / 2, GFP_KERNEL);
	if (!table)
		return -ENOMEM;

	of_property_read_u32_array(dn, "table", (unsigned int *)table, size);
	for (index = 0; index < domain->table_size; index++) {
		unsigned int freq = domain->freq_table[index].frequency;

		if (freq == CPUFREQ_ENTRY_INVALID)
			continue;

		for (c_index = 0; c_index < size / 2; c_index++) {
			/* find row same or nearby frequency */
			if (freq <= table[c_index].master_freq)
				dm->c.freq_table[index].constraint_freq
					= table[c_index].constraint_freq;

			if (freq >= table[c_index].master_freq)
				break;

		}
	}

	kfree(table);
	return 0;
}

static int init_dm(struct exynos_cpufreq_domain *domain,
				struct device_node *dn)
{
	struct device_node *root, *child;
	struct exynos_cpufreq_dm *dm;
	int ret;

	if (list_empty(&domain->dm_list))
		return 0;

	ret = of_property_read_u32(dn, "dm-type", &domain->dm_type);
	if (ret)
		return ret;

	ret = exynos_dm_data_init(domain->dm_type, domain, domain->min_freq,
				domain->max_freq, domain->old);
	if (ret)
		return ret;

	dm = list_entry(&domain->dm_list, struct exynos_cpufreq_dm, list);
	root = of_find_node_by_name(dn, "dm-constraints");
	for_each_child_of_node(root, child) {
		/*
		 * Initialize DVFS Manaver constraints
		 * - constraint_type : minimum or maximum constraint
		 * - constraint_dm_type : cpu/mif/int/.. etc
		 * - guidance : constraint from chipset characteristic
		 * - freq_table : constraint table
		 */
		dm = list_next_entry(dm, list);

		of_property_read_u32(child, "const-type", &dm->c.constraint_type);
		of_property_read_u32(child, "dm-type", &dm->c.constraint_dm_type);

		if (of_property_read_bool(child, "guidance")) {
			dm->c.guidance = true;
			if (init_constraint_table_ect(domain, dm, child))
				continue;
		} else {
			if (init_constraint_table_dt(domain, dm, child))
				continue;
		}

		dm->c.table_length = domain->table_size;

		ret = register_exynos_dm_constraint_table(domain->dm_type, &dm->c);
		if (ret)
			return ret;
	}

	return register_exynos_dm_freq_scaler(domain->dm_type, dm_scaler);
}

static __init int init_domain(struct exynos_cpufreq_domain *domain,
					struct device_node *dn)
{
	unsigned int val;
	int ret;

	mutex_init(&domain->lock);

	/* Initialize frequency scaling */
	domain->max_freq = cal_dfs_get_max_freq(domain->cal_id);
	domain->min_freq = cal_dfs_get_min_freq(domain->cal_id);

	/*
	 * If max-freq property exists in device tree, max frequency is
	 * selected to smaller one between the value defined in device
	 * tree and CAL. In case of min-freq, min frequency is selected
	 * to bigger one.
	 */
	if (!of_property_read_u32(dn, "max-freq", &val))
		domain->max_freq = min(domain->max_freq, val);
	if (!of_property_read_u32(dn, "min-freq", &val))
		domain->min_freq = max(domain->min_freq, val);

	/* If this domain has boost freq, change max */
	val = exynos_pstate_get_boost_freq(cpumask_first(&domain->cpus));
	if (val > domain->max_freq)
		domain->max_freq = val;

	if (of_property_read_bool(dn, "need-awake"))
		domain->need_awake = true;

	domain->boot_freq = cal_dfs_get_boot_freq(domain->cal_id);
	domain->resume_freq = cal_dfs_get_resume_freq(domain->cal_id);

	ufc_domain_init(domain);

	ret = init_table(domain);
	if (ret)
		return ret;

	domain->old = get_freq(domain);

	/* Initialize PM QoS */
	ret = init_pm_qos(domain, dn);
	if (ret)
		return ret;

	/*
	 * Initialize CPUFreq DVFS Manager
	 * DVFS Manager is the optional function, it does not check return value
	 */
	init_dm(domain, dn);

	pr_info("Complete to initialize cpufreq-domain%d\n", domain->id);

	return ret;
}

static __init int early_init_domain(struct exynos_cpufreq_domain *domain,
					struct device_node *dn)
{
	const char *buf;
	int ret;

	/* Initialize list head of DVFS Manager constraints */
	INIT_LIST_HEAD(&domain->dm_list);
	INIT_LIST_HEAD(&domain->ufc_list);

	ret = of_property_read_u32(dn, "cal-id", &domain->cal_id);
	if (ret)
		return ret;

	/* Get size of frequency table from CAL */
	domain->table_size = cal_dfs_get_lv_num(domain->cal_id);

	/* Get cpumask which belongs to domain */
	ret = of_property_read_string(dn, "sibling-cpus", &buf);
	if (ret)
		return ret;
	cpulist_parse(buf, &domain->cpus);
	cpumask_and(&domain->cpus, &domain->cpus, cpu_online_mask);
	if (cpumask_weight(&domain->cpus) == 0)
		return -ENODEV;

	return 0;
}

static __init void __free_domain(struct exynos_cpufreq_domain *domain)
{
	struct exynos_cpufreq_dm *dm;

	while (!list_empty(&domain->dm_list)) {
		dm = list_last_entry(&domain->dm_list,
				struct exynos_cpufreq_dm, list);
		list_del(&dm->list);
		kfree(dm->c.freq_table);
		kfree(dm);
	}

	kfree(domain->freq_table);
	kfree(domain);
}

static __init void free_domain(struct exynos_cpufreq_domain *domain)
{
	list_del(&domain->list);
	unregister_exynos_dm_freq_scaler(domain->dm_type);

	__free_domain(domain);
}

static __init struct exynos_cpufreq_domain *alloc_domain(struct device_node *dn)
{
	struct exynos_cpufreq_domain *domain;
	struct device_node *root, *child;

	domain = kzalloc(sizeof(struct exynos_cpufreq_domain), GFP_KERNEL);
	if (!domain)
		return NULL;

	/*
	 * early_init_domain() initailize the domain information requisite
	 * to allocate domain and table.
	 */
	if (early_init_domain(domain, dn))
		goto free;

	/*
	 * Allocate frequency table.
	 * Last row of frequency table must be set to CPUFREQ_TABLE_END.
	 * Table size should be one larger than real table size.
	 */
	domain->freq_table =
		kzalloc(sizeof(struct cpufreq_frequency_table)
				* (domain->table_size + 1), GFP_KERNEL);
	if (!domain->freq_table)
		goto free;

	/*
	 * Allocate DVFS Manager constraints.
	 * Constraints are needed only by DVFS Manager, these are not
	 * created when DVFS Manager is disabled. If constraints does
	 * not exist, driver does scaling without DVFS Manager.
	 */
#ifndef CONFIG_EXYNOS_DVFS_MANAGER
	return domain;
#endif

	root = of_find_node_by_name(dn, "dm-constraints");
	for_each_child_of_node(root, child) {
		struct exynos_cpufreq_dm *dm;

		dm = kzalloc(sizeof(struct exynos_cpufreq_dm), GFP_KERNEL);
		if (!dm)
			goto free;

		dm->c.freq_table = kzalloc(sizeof(struct exynos_dm_freq)
					* domain->table_size, GFP_KERNEL);
		if (!dm->c.freq_table)
			goto free;

		list_add_tail(&dm->list, &domain->dm_list);
	}

	return domain;

free:
	__free_domain(domain);

	return NULL;
}

static int __init exynos_cpufreq_init(void)
{
	struct device_node *dn = NULL;
	struct exynos_cpufreq_domain *domain;
	int ret = 0;
	unsigned int domain_id = 0;

	while ((dn = of_find_node_by_type(dn, "cpufreq-domain"))) {
		domain = alloc_domain(dn);
		if (!domain) {
			pr_err("failed to allocate domain%d\n", domain_id);
			continue;
		}

		list_add_tail(&domain->list, &domains);

		domain->dn = dn;
		domain->id = domain_id++;
		ret = init_domain(domain, dn);
		if (ret) {
			pr_err("failed to initialize cpufreq domain%d\n",
							domain_id);
			free_domain(domain);
			continue;
		}

		print_domain_info(domain);
	}

	if (!domain_id) {
		pr_err("Can't find device type cpufreq-domain\n");
		return -ENODATA;
	}

	ret = cpufreq_register_driver(&exynos_driver);
	if (ret) {
		pr_err("failed to register cpufreq driver\n");
		return ret;
	}

	init_sysfs();

	exynos_cpuhp_register("ACME", *cpu_possible_mask, 0);

	cpufreq_register_notifier(&exynos_cpufreq_policy_notifier,
					CPUFREQ_POLICY_NOTIFIER);

	cpuhp_setup_state_nocalls(CPUHP_AP_EXYNOS_ACME,
					"exynos:acme",
					exynos_cpufreq_cpu_up_callback,
					exynos_cpufreq_cpu_down_callback);

	register_pm_notifier(&exynos_cpufreq_pm);

	/*
	 * Enable scale of domain.
	 * Update frequency as soon as domain is enabled.
	 */
	list_for_each_entry(domain, &domains, list) {
		struct cpufreq_policy *policy;
		enable_domain(domain);
		policy = cpufreq_cpu_get_raw(cpumask_first(&domain->cpus));
		if (policy)
			exynos_cpufreq_cooling_register(domain->dn, policy);

		set_boot_qos(domain);
	}

	set_energy_table_status(true);

	pr_info("Initialized Exynos cpufreq driver\n");

	return ret;
}
device_initcall(exynos_cpufreq_init);
