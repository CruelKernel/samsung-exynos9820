/*
 * Copyright (c) 2018, Park Choonghoon
 * Samsung Electronics Co., Ltd
 * <choong.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Exynos FF(Frequency Filter) driver implementation
 */

#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/syscore_ops.h>
#include <linux/tick.h>
#include <linux/delay.h>
#include <soc/samsung/cal-if.h>
#include <trace/events/power.h>
#include "exynos-ff.h"
#include "exynos-acme.h"
#include "../../../kernel/sched/sched.h"

bool hwi_dvfs_req;
atomic_t boost_throttling = ATOMIC_INIT(0);

static struct exynos_ff_driver *eff_driver;
static int (*eff_target)(struct cpufreq_policy *policy,
			        unsigned int target_freq,
			        unsigned int relation);

/*********************************************************************
 *                          HELPER FUNCTION                          *
 *********************************************************************/
static bool policy_need_filter(struct cpufreq_policy *policy)
{
	return cpumask_intersects(policy->cpus, &eff_driver->cpus);
}

#ifdef CONFIG_EXYNOS_PSTATE_HAFM_TB
static bool check_filtering(unsigned int target_freq, unsigned int flag)
{
	unsigned int cur_freq;

	cur_freq = (unsigned int)cal_dfs_get_rate(eff_driver->cal_id);

	/*
	 * Filtering conditions
	 * 1) SW request (normal request)
	 *	turbo boost is already activated (cur_freq >= boost_threshold)
	 *	and
	 *	this request could activate turbo boost (target_freq >= boost_threshold)
	 *
	 * 2) HWI request
	 *	turbo boost is released (cur_freq < boost_threshold)
	 */
	if ((flag & CPUFREQ_REQUEST_MASK) == CPUFREQ_NORMAL_REQ)
		return cur_freq >= eff_driver->boost_threshold &&
			target_freq >= eff_driver->boost_threshold;
	else
		return cur_freq < eff_driver->boost_threshold;
}

static bool check_boost_freq_throttled(struct cpufreq_policy *policy)
{
	return (policy->cur > eff_driver->boost_threshold) &&
		(policy->cur > policy->max);
}
#endif

/*********************************************************************
 *                       EXTERNAL REFERENCE APIs                     *
 *********************************************************************/
int __cpufreq_driver_target(struct cpufreq_policy *policy,
			    unsigned int target_freq,
			    unsigned int flag)
{
	int ret = 0;
	unsigned int old_target_freq = target_freq;

	if (!eff_driver)
		return -EINVAL;

	/* Make sure that target_freq is within supported range */
	target_freq = clamp_val(target_freq, policy->min, policy->max);

	pr_debug("target for CPU %u: %u kHz, relation %u, requested %u kHz\n",
		 policy->cpu, target_freq, flag & CPUFREQ_RELATION_MASK, old_target_freq);

	if (policy_need_filter(policy)) {
		mutex_lock(&eff_driver->lock);

#ifdef CONFIG_EXYNOS_PSTATE_HAFM_TB
		if (check_filtering(target_freq, flag))
			goto out;

		/*
		 * This flag is used in lower SW layer to determine
		 * whether this DVFS request is HW interventioned or not.
		 */
		hwi_dvfs_req = (flag & CPUFREQ_REQUEST_MASK) == CPUFREQ_HW_DVFS_REQ;

		/*
		 * In case of normal DVFS request (not HWI request),
		 * clamp target value to boost threshold,
		 * if target value > boost threshold.
		 * SW must not request DVFS with frequency above boost threshold.
		 */
		if (!hwi_dvfs_req && target_freq > eff_driver->boost_threshold)
			target_freq = eff_driver->boost_threshold;

		trace_cpu_frequency_filter(target_freq, hwi_dvfs_req);
#endif
	}

	/*
	 * This might look like a redundant call as we are checking it again
	 * after finding index. But it is left intentionally for cases where
	 * exactly same freq is called again and so we can save on few function
	 * calls.
	 */
	if (target_freq == policy->cur)
		goto out;

	/* Save last value to restore later on errors */
	policy->restore_freq = policy->cur;

	if (eff_target)
		ret = eff_target(policy, target_freq,
					flag & CPUFREQ_RELATION_MASK);
out:
	if (policy_need_filter(policy))
		mutex_unlock(&eff_driver->lock);

	return ret;
}

void cpufreq_policy_apply_limits(struct cpufreq_policy *policy)
{
#ifdef CONFIG_EXYNOS_PSTATE_HAFM_TB
	if (policy_need_filter(policy)) {
		if (check_boost_freq_throttled(policy)) {
			pr_debug("exynos-ff: wait for boost freq throttling completion\n");
			/* Wait for Completion of HWI Request */
			while (atomic_read(&boost_throttling))
				usleep_range(100, 200);

			/* After HW reqeusted request completes, policy->cur <= policy->max */
			pr_debug("exynos-ff: apply limits done, max:%d, cur:%d\n",
					policy->max, policy->cur);
			return;
		}
	}
#endif

	if (policy->max < policy->cur)
		__cpufreq_driver_target(policy, policy->max, CPUFREQ_RELATION_H);
	else if (policy->min > policy->cur)
		__cpufreq_driver_target(policy, policy->min, CPUFREQ_RELATION_L);
}

/*********************************************************************
 *                    INITIALIZE EXYNOS FF DRIVER                    *
 *********************************************************************/

static int exynos_ff_get_target(struct cpufreq_policy *policy, target_fn target)
{
	if (!cpumask_intersects(&eff_driver->cpus, policy->cpus))
		return 0;

	eff_target = target;

	return 0;
}

static struct exynos_cpufreq_ready_block exynos_ff_ready = {
	.get_target = exynos_ff_get_target,
};

/*********************************************************************
 *                    INITIALIZE EXYNOS FF DRIVER                    *
 *********************************************************************/
static int alloc_driver(void)
{
	int ret;
	const char *buf;
	struct device_node *dn;

	eff_driver = kzalloc(sizeof(struct exynos_ff_driver), GFP_KERNEL);
	if (!eff_driver) {
		pr_err("failed to allocate eff driver\n");
		return -ENODATA;
	}

	mutex_init(&eff_driver->lock);

	dn = of_find_node_by_type(NULL, "exynos-ff");
	if (!dn) {
		pr_err("Failed to initialize eff driver\n");
		return -ENODATA;
	}

	/* Get boost frequency threshold */
	ret = of_property_read_u32(dn, "boost-threshold", &eff_driver->boost_threshold);
	if (ret)
		return ret;

	/* Get cal id to get current frequency */
	ret = of_property_read_u32(dn, "cal-id", &eff_driver->cal_id);
	if (ret)
		return ret;

	/* Get cpumask which belongs to domain */
	ret = of_property_read_string(dn, "sibling-cpus", &buf);
	if (ret)
		return ret;

	cpulist_parse(buf, &eff_driver->cpus);
	cpumask_and(&eff_driver->cpus, &eff_driver->cpus, cpu_online_mask);
	if (cpumask_weight(&eff_driver->cpus) == 0)
		return -ENODEV;

	return 0;
}

static int __init exynos_ff_init(void)
{
	int ret;

	ret = alloc_driver();
	if (ret) {
		pr_err("exynos-ff: Fail to allocate Exynos FF driver\n");
		BUG_ON(1);
		return ret;
	}

	exynos_cpufreq_ready_list_add(&exynos_ff_ready);

	pr_info("exynos-ff: Initialized Exynos Frequency Filter driver\n");

	return ret;
}
device_initcall(exynos_ff_init);
