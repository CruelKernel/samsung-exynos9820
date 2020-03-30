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
#include <linux/pm_opp.h>
#include <linux/ems_service.h>
#include <linux/exynos-ucc.h>

#include <soc/samsung/exynos-cpuhp.h>

#include "exynos-acme.h"

/*********************************************************************
 *                          SYSFS INTERFACES                         *
 *********************************************************************/
/*
 * Log2 of the number of scale size. The frequencies are scaled up or
 * down as the multiple of this number.
 */
#define SCALE_SIZE	3

static int last_max_limit = -1;
static int sse_mode;
static int sse_mode_game;

static ssize_t show_cpufreq_table(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	struct list_head *domains = get_domain_list();
	struct exynos_cpufreq_domain *domain;
	ssize_t count = 0;
	int i, scale = 0;

	list_for_each_entry_reverse(domain, domains, list) {
		for (i = 0; i < domain->table_size; i++) {
			unsigned int freq = domain->freq_table[i].frequency;

			if (freq == CPUFREQ_ENTRY_INVALID)
				continue;

			count += snprintf(&buf[count], 10, "%d ",
					freq >> (scale * SCALE_SIZE));
		}

		scale++;
	}

	count += snprintf(&buf[count - 1], 2, "\n");

	return count - 1;
}

static ssize_t show_cpufreq_min_limit(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	struct list_head *domains = get_domain_list();
	struct exynos_cpufreq_domain *domain;
	unsigned int pm_qos_min;
	int scale = -1;

	list_for_each_entry_reverse(domain, domains, list) {
		scale++;

		/* get value of minimum PM QoS */
		pm_qos_min = pm_qos_request(domain->pm_qos_min_class);
		if (pm_qos_min > 0) {
			pm_qos_min = min(pm_qos_min, domain->max_freq);
			pm_qos_min = max(pm_qos_min, domain->min_freq);

			/*
			 * To manage frequencies of all domains at once,
			 * scale down frequency as multiple of 4.
			 * ex) domain2 = freq
			 *     domain1 = freq /4
			 *     domain0 = freq /16
			 */
			pm_qos_min = pm_qos_min >> (scale * SCALE_SIZE);
			return snprintf(buf, 10, "%u\n", pm_qos_min);
		}
	}

	/*
	 * If there is no QoS at all domains, it returns minimum
	 * frequency of last domain
	 */
	return snprintf(buf, 10, "%u\n",
		first_domain()->min_freq >> (scale * SCALE_SIZE));
}

static struct kpp kpp_ta;
static struct kpp kpp_fg;

static struct ucc_req ucc_req =
{
	.name = "ufc",
};
static int ucc_requested;
static int ucc_requested_val = 0;

static ssize_t store_cpufreq_min_limit(struct kobject *kobj,
				struct kobj_attribute *attr, const char *buf,
				size_t count)
{
	struct list_head *domains = get_domain_list();
	struct exynos_cpufreq_domain *domain;
	int input, scale = -1;
	unsigned int freq;
	unsigned int req_limit_freq, req_last_freq;
	bool set_max = false;
	bool set_limit = false;
	int index = 0;
	struct cpumask mask;
	int domain_cnt = 0;
	bool is_big_on = false;
	bool is_mid_on = false;
	bool is_lit_on = false;

	if (!sscanf(buf, "%8d", &input))
		return -EINVAL;

	if (!domains) {
		pr_err("failed to get domains!\n");
		return -ENXIO;
	}

	list_for_each_entry_reverse(domain, domains, list) {
		struct cpufreq_policy *policy = NULL;

		domain_cnt++;
		cpumask_and(&mask, &domain->cpus, cpu_online_mask);
		if (!cpumask_weight(&mask))
			continue;

		policy = cpufreq_cpu_get_raw(cpumask_any(&mask));
		if (!policy)
			continue;

		if (domain_cnt == 1)
			is_big_on = true;
		else if (domain_cnt == 2)
			is_mid_on = true;
		else
			is_lit_on = true;
	}
	list_for_each_entry_reverse(domain, domains, list) {
		struct exynos_ufc *ufc, *r_ufc = NULL, *r_ufc_32 = NULL;
		struct cpufreq_policy *policy = NULL;

		scale++;
		cpumask_and(&mask, &domain->cpus, cpu_online_mask);
		if (!cpumask_weight(&mask))
			continue;

		policy = cpufreq_cpu_get_raw(cpumask_any(&mask));
		if (!policy)
			continue;

		ufc = list_entry(&domain->ufc_list, struct exynos_ufc, list);

		list_for_each_entry(ufc, &domain->ufc_list, list) {
			if (ufc->info.ctrl_type == PM_QOS_MIN_LIMIT) {
				if (ufc->info.exe_mode == AARCH64_MODE) {
					r_ufc = ufc;
				} else {
					r_ufc_32 = ufc;
				}
			}
		}

		if (set_limit) {
			if (req_limit_freq > 0)
				req_limit_freq = min(req_limit_freq, domain->max_freq);
			else
				req_limit_freq = domain->max_freq;

			pm_qos_update_request(&domain->user_min_qos_req, req_limit_freq);
			set_limit = false;
			continue;
		}

		if (set_max) {
			unsigned int qos = domain->max_freq;

			if (req_last_freq > 0)
				qos = min(req_last_freq, domain->max_freq);

			if (domain->user_default_qos)
				qos = max(req_last_freq, domain->user_default_qos);

			pm_qos_update_request(&domain->user_min_qos_req, qos);
			continue;
		}

		/* Clear all constraint by cpufreq_min_limit */
		if (input < 0) {
			pm_qos_update_request(&domain->user_min_qos_req, 0);
			kpp_request(STUNE_TOPAPP, &kpp_ta, 0);
			kpp_request(STUNE_FOREGROUND, &kpp_fg, 0);
			ucc_requested_val = 0;
			ucc_update_request(&ucc_req, ucc_requested_val);
			continue;
		}

		/*
		 * User inputs scaled down frequency. To recover real
		 * frequency, scale up frequency as multiple of 4.
		 * ex) domain2 = freq
		 *     domain1 = freq * 4
		 *     domain0 = freq * 16
		 */
		freq = input << (scale * SCALE_SIZE);

		if (freq < domain->min_freq) {
			pm_qos_update_request(&domain->user_min_qos_req, 0);
			continue;
		}

		if (r_ufc) {
			if (sse_mode && r_ufc_32)
				r_ufc = r_ufc_32;

			if (policy->freq_table == NULL)
				continue;

			index = cpufreq_frequency_table_target(policy, freq, CPUFREQ_RELATION_L);
			req_limit_freq = r_ufc->info.freq_table[index].limit_freq;
			req_last_freq = r_ufc->info.freq_table[index].last_freq;
			if (req_limit_freq && is_mid_on)
				set_limit = true;
			if (is_lit_on)
				set_max = true;
		}
		freq = min(freq, domain->max_freq);
		pm_qos_update_request(&domain->user_min_qos_req, freq);

		if ((domain->user_boost == 3) && sse_mode_game) {
			kpp_request(STUNE_TOPAPP, &kpp_ta, domain->user_boost_game);
			kpp_request(STUNE_FOREGROUND, &kpp_fg, domain->user_boost_game);
			ucc_requested_val = domain->ucc_index;
			ucc_update_request(&ucc_req, ucc_requested_val);
		} else {
			kpp_request(STUNE_TOPAPP, &kpp_ta, domain->user_boost);
			kpp_request(STUNE_FOREGROUND, &kpp_fg, domain->user_boost);
			ucc_requested_val = domain->ucc_index;
			ucc_update_request(&ucc_req, ucc_requested_val);
		}

		if (is_lit_on)
			set_max = true;
	}

	return count;
}

static ssize_t store_cpufreq_min_limit_wo_boost(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf,
		size_t count)
{
	struct list_head *domains = get_domain_list();
	struct exynos_cpufreq_domain *domain;
	int input, scale = -1;
	unsigned int freq;
	unsigned int req_limit_freq;
	bool set_max = false;
	bool set_limit = false;
	int index = 0;
	struct cpumask mask;

	if (!sscanf(buf, "%8d", &input))
		return -EINVAL;

	if (!domains) {
		pr_err("failed to get domains!\n");
		return -ENXIO;
	}

	list_for_each_entry_reverse(domain, domains, list) {
		struct exynos_ufc *ufc, *r_ufc = NULL, *r_ufc_32 = NULL;
		struct cpufreq_policy *policy = NULL;

		cpumask_and(&mask, &domain->cpus, cpu_online_mask);
		if (!cpumask_weight(&mask))
			continue;

		policy = cpufreq_cpu_get_raw(cpumask_any(&mask));
		if (!policy)
			continue;

		ufc = list_entry(&domain->ufc_list, struct exynos_ufc, list);

		list_for_each_entry(ufc, &domain->ufc_list, list) {
			if (ufc->info.ctrl_type == PM_QOS_MIN_WO_BOOST_LIMIT) {
				if (ufc->info.exe_mode == AARCH64_MODE) {
					r_ufc = ufc;
				} else {
					r_ufc_32 = ufc;
				}
			}
		}

		scale++;

		if (set_limit) {
			req_limit_freq = min(req_limit_freq, domain->max_freq);
			pm_qos_update_request(&domain->user_min_qos_req, req_limit_freq);
			set_limit = false;
			continue;
		}

		if (set_max) {
			unsigned int qos = domain->max_freq;

			if (domain->user_default_qos)
				qos = domain->user_default_qos;

			pm_qos_update_request(&domain->user_min_qos_wo_boost_req, qos);
			continue;
		}

		/* Clear all constraint by cpufreq_min_limit */
		if (input < 0) {
			pm_qos_update_request(&domain->user_min_qos_wo_boost_req, 0);
			continue;
		}

		/*
		 ** User inputs scaled down frequency. To recover real
		 ** frequency, scale up frequency as multiple of 4.
		 ** ex) domain2 = freq
		 **     domain1 = freq * 8
		 **     domain0 = freq * 64
		 **/
		freq = input << (scale * SCALE_SIZE);

		if (freq < domain->min_freq) {
			pm_qos_update_request(&domain->user_min_qos_wo_boost_req, 0);
			continue;
		}

		if (r_ufc) {
			if (sse_mode && r_ufc_32)
				r_ufc = r_ufc_32;

			if (policy->freq_table == NULL)
				continue;

			index = cpufreq_frequency_table_target(policy, freq, CPUFREQ_RELATION_L);
			req_limit_freq = r_ufc->info.freq_table[index].limit_freq;
			if (req_limit_freq)
				set_limit = true;
		}

		freq = min(freq, domain->max_freq);
		pm_qos_update_request(&domain->user_min_qos_wo_boost_req, freq);

		set_max = true;
	}

	return count;

}
static ssize_t show_cpufreq_max_limit(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct list_head *domains = get_domain_list();
	struct exynos_cpufreq_domain *domain;
	unsigned int pm_qos_max;
	int scale = -1;

	if (!domains) {
		pr_err("failed to get domains!\n");
		return -ENXIO;
	}

	list_for_each_entry_reverse(domain, domains, list) {
		scale++;

		/* get value of minimum PM QoS */
		pm_qos_max = pm_qos_request(domain->pm_qos_max_class);
		if (pm_qos_max > 0) {
			pm_qos_max = min(pm_qos_max, domain->max_freq);
			pm_qos_max = max(pm_qos_max, domain->min_freq);

			/*
			 * To manage frequencies of all domains at once,
			 * scale down frequency as multiple of 4.
			 * ex) domain2 = freq
			 *     domain1 = freq /4
			 *     domain0 = freq /16
			 */
			pm_qos_max = pm_qos_max >> (scale * SCALE_SIZE);
			return snprintf(buf, 10, "%u\n", pm_qos_max);
		}
	}

	/*
	 * If there is no QoS at all domains, it returns minimum
	 * frequency of last domain
	 */
	return snprintf(buf, 10, "%u\n",
		first_domain()->min_freq >> (scale * SCALE_SIZE));
}

unsigned int get_cpufreq_max_limit(void)
{
	struct list_head *domains = get_domain_list();
	struct exynos_cpufreq_domain *domain;
	unsigned int pm_qos_max;
	int scale = -1;

	if (!domains) {
		pr_err("failed to get domains!\n");
		return -ENXIO;
	}

	list_for_each_entry_reverse(domain, domains, list) {
		scale++;

		/* get value of minimum PM QoS */
		pm_qos_max = pm_qos_request(domain->pm_qos_max_class);
		if (pm_qos_max > 0) {
			pm_qos_max = min(pm_qos_max, domain->max_freq);
			pm_qos_max = max(pm_qos_max, domain->min_freq);

			/*
			 * To manage frequencies of all domains at once,
			 * scale down frequency as multiple of 4.
			 * ex) domain2 = freq
			 *     domain1 = freq /4
			 *     domain0 = freq /16
			 */
			pm_qos_max = pm_qos_max >> (scale * SCALE_SIZE);
			return pm_qos_max;
		}
	}

	/*
	 * If there is no QoS at all domains, it returns minimum
	 * frequency of last domain
	 */
	return first_domain()->min_freq >> (scale * SCALE_SIZE);
}

static void enable_domain_cpus(struct exynos_cpufreq_domain *domain)
{
	struct cpumask mask;

	if (domain == first_domain())
		return;

	cpumask_or(&mask, cpu_online_mask, &domain->cpus);
	exynos_cpuhp_request("ACME", mask, 0);
}

static void disable_domain_cpus(struct exynos_cpufreq_domain *domain)
{
	struct cpumask mask;

	if (domain == first_domain())
		return;

	cpumask_andnot(&mask, cpu_online_mask, &domain->cpus);
	exynos_cpuhp_request("ACME", mask, 0);
}

static void cpufreq_max_limit_update(int input_freq)
{
	struct list_head *domains = get_domain_list();
	struct exynos_cpufreq_domain *domain;
	int scale = -1;
	unsigned int freq;
	bool set_max = false;
	unsigned int req_limit_freq;
	bool set_limit = false;
	int index = 0;
	struct cpumask mask;

	list_for_each_entry_reverse(domain, domains, list) {
		struct exynos_ufc *ufc, *r_ufc= NULL, *r_ufc_32 = NULL;
		struct cpufreq_policy *policy = NULL;

		cpumask_and(&mask, &domain->cpus, cpu_online_mask);
		if (cpumask_weight(&mask))
			policy = cpufreq_cpu_get_raw(cpumask_any(&mask));

		ufc = list_entry(&domain->ufc_list, struct exynos_ufc, list);

		list_for_each_entry(ufc, &domain->ufc_list, list) {
			if (ufc->info.ctrl_type == PM_QOS_MAX_LIMIT) {
				if (ufc->info.exe_mode == AARCH64_MODE) {
					r_ufc = ufc;
				} else {
					r_ufc_32 = ufc;
				}
			}
		}

		scale++;

		if (set_limit) {
			req_limit_freq = max(req_limit_freq, domain->min_freq);
			pm_qos_update_request(&domain->user_max_qos_req,
					req_limit_freq);
			set_limit = false;
			continue;
		}

		if (set_max) {
			pm_qos_update_request(&domain->user_max_qos_req,
					domain->max_freq);
			continue;
		}

		/* Clear all constraint by cpufreq_max_limit */
		if (input_freq < 0) {
			enable_domain_cpus(domain);
			pm_qos_update_request(&domain->user_max_qos_req,
					domain->max_freq);
			continue;
		}

		/*
		 ** User inputs scaled down frequency. To recover real
		 ** frequency, scale up frequency as multiple of 4.
		 ** ex) domain2 = freq
		 **     domain1 = freq * 4
		 **     domain0 = freq * 16
		 **/
		freq = input_freq << (scale * SCALE_SIZE);

		if (policy && r_ufc) {
			if (sse_mode && r_ufc_32)
				r_ufc = r_ufc_32;

			if (policy->freq_table == NULL)
				continue;

			index = cpufreq_frequency_table_target(policy, freq, CPUFREQ_RELATION_L);
			req_limit_freq = r_ufc->info.freq_table[index].limit_freq;
			if (req_limit_freq)
				set_limit = true;
		}

		if (freq < domain->min_freq) {
			set_limit = false;
			pm_qos_update_request(&domain->user_max_qos_req, 0);
			disable_domain_cpus(domain);
			continue;
		}

		enable_domain_cpus(domain);
		freq = max(freq, domain->min_freq);
		pm_qos_update_request(&domain->user_max_qos_req, freq);

		set_max = true;
	}
}
static ssize_t store_cpufreq_max_limit(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	int input;

	if (!sscanf(buf, "%8d", &input))
		return -EINVAL;

	last_max_limit = input;
	cpufreq_max_limit_update(input);

	return count;
}

static ssize_t show_execution_mode_change(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 10, "%d\n",sse_mode);
}

static ssize_t store_execution_mode_change(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	int input;
	int prev_mode;

	if (!sscanf(buf, "%8d", &input))
		return -EINVAL;

	prev_mode = sse_mode;
	sse_mode = !!input;

	if (prev_mode != sse_mode) {
		if (last_max_limit != -1)
			cpufreq_max_limit_update(last_max_limit);
	}

	return count;
}

static ssize_t show_cstate_control(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
    return snprintf(buf, 10, "%d\n", ucc_requested);
}

static ssize_t store_cstate_control(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	int input;
	if (!sscanf(buf, "%8d", &input))
		return -EINVAL;

	if (input < 0)
		return -EINVAL;

	input = !!input;

	if (input == ucc_requested)
		goto out;

	ucc_requested = input;

	if (ucc_requested)
		ucc_add_request(&ucc_req, ucc_requested_val);
	else
		ucc_remove_request(&ucc_req);

out:
	return count;
}

static ssize_t show_boost_mode_change(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 10, "%d\n",sse_mode_game);
}

static ssize_t store_boost_mode_change(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	int input;
	int prev_mode;

	if (!sscanf(buf, "%8d", &input))
		return -EINVAL;

	prev_mode = sse_mode_game;
	sse_mode_game = !!input;

	if (prev_mode != sse_mode_game) {
		if (last_max_limit != -1)
			cpufreq_max_limit_update(last_max_limit);
	}

	return count;
}

static struct kobj_attribute cpufreq_table =
__ATTR(cpufreq_table, 0444 , show_cpufreq_table, NULL);
static struct kobj_attribute cpufreq_min_limit =
__ATTR(cpufreq_min_limit, 0644,
		show_cpufreq_min_limit, store_cpufreq_min_limit);
static struct kobj_attribute cpufreq_min_limit_wo_boost =
__ATTR(cpufreq_min_limit_wo_boost, 0644,
		show_cpufreq_min_limit, store_cpufreq_min_limit_wo_boost);
static struct kobj_attribute cpufreq_max_limit =
__ATTR(cpufreq_max_limit, 0644,
		show_cpufreq_max_limit, store_cpufreq_max_limit);
static struct kobj_attribute execution_mode_change =
__ATTR(execution_mode_change, 0644,
		show_execution_mode_change, store_execution_mode_change);
static struct kobj_attribute cstate_control =
__ATTR(cstate_control, 0644, show_cstate_control, store_cstate_control);
static struct kobj_attribute boost_mode_change =
__ATTR(boost_mode_change, 0644,
		show_boost_mode_change, store_boost_mode_change);

static __init void init_sysfs(void)
{
	if (sysfs_create_file(power_kobj, &cpufreq_table.attr))
		pr_err("failed to create cpufreq_table node\n");

	if (sysfs_create_file(power_kobj, &cpufreq_min_limit.attr))
		pr_err("failed to create cpufreq_min_limit node\n");

	if (sysfs_create_file(power_kobj, &cpufreq_min_limit_wo_boost.attr))
		pr_err("failed to create cpufreq_min_limit_wo_boost node\n");

	if (sysfs_create_file(power_kobj, &cpufreq_max_limit.attr))
		pr_err("failed to create cpufreq_max_limit node\n");

	if (sysfs_create_file(power_kobj, &execution_mode_change.attr))
		pr_err("failed to create execution_mode_change node\n");

	if (sysfs_create_file(power_kobj, &cstate_control.attr))
		pr_err("failed to create cstate_control node\n");

	if (sysfs_create_file(power_kobj, &boost_mode_change.attr))
		pr_err("failed to create boost_mode_change node\n");

}

static int parse_ufc_ctrl_info(struct exynos_cpufreq_domain *domain,
					struct device_node *dn)
{
	unsigned int val;

	if (!of_property_read_u32(dn, "user-default-qos", &val))
		domain->user_default_qos = val;

	if (!of_property_read_u32(dn, "user-boost", &val))
		domain->user_boost = val;

	if (!of_property_read_u32(dn, "ucc-index", &val))
		domain->ucc_index = val;

	if (!of_property_read_u32(dn, "user-boost-game", &val))
		domain->user_boost_game = val;

	return 0;
}

static __init void init_pm_qos(struct exynos_cpufreq_domain *domain)
{
	pm_qos_add_request(&domain->user_min_qos_req,
			domain->pm_qos_min_class, domain->min_freq);
	pm_qos_add_request(&domain->user_max_qos_req,
			domain->pm_qos_max_class, domain->max_freq);
	pm_qos_add_request(&domain->user_min_qos_wo_boost_req,
			domain->pm_qos_min_class, domain->min_freq);
}

int ufc_domain_init(struct exynos_cpufreq_domain *domain)
{
	struct device_node *dn, *child;
	struct cpumask mask;
	const char *buf;

	dn = of_find_node_by_name(NULL, "cpufreq-ufc");

	while((dn = of_find_node_by_type(dn, "cpufreq-userctrl"))) {
		of_property_read_string(dn, "shared-cpus", &buf);
		cpulist_parse(buf, &mask);
		if (cpumask_intersects(&mask, &domain->cpus)) {
			printk("found!\n");
			break;
		}
	}

	for_each_child_of_node(dn, child) {
		struct exynos_ufc *ufc;

		ufc = kzalloc(sizeof(struct exynos_ufc), GFP_KERNEL);
		if (!ufc)
			return -ENOMEM;

		ufc->info.freq_table = kzalloc(sizeof(struct exynos_ufc_freq)
				* domain->table_size, GFP_KERNEL);

		if (!ufc->info.freq_table) {
			kfree(ufc);
			return -ENOMEM;
		}

		list_add_tail(&ufc->list, &domain->ufc_list);
	}

	return 0;
}

static int __init init_ufc_table_dt(struct exynos_cpufreq_domain *domain,
					struct device_node *dn)
{
	struct device_node *child;
	struct exynos_ufc_freq *table;
	struct exynos_ufc *ufc;
	int size, index, c_index;
	int ret;

	ufc = list_entry(&domain->ufc_list, struct exynos_ufc, list);

	pr_info("Initialize ufc table for Domain %d\n",domain->id);

	for_each_child_of_node(dn, child) {

		ufc = list_next_entry(ufc, list);

		if (of_property_read_u32(child, "ctrl-type", &ufc->info.ctrl_type))
			continue;

		if (of_property_read_u32(child, "execution-mode", &ufc->info.exe_mode))
			continue;

		size = of_property_count_u32_elems(child, "table");
		if (size < 0)
			return size;

		table = kzalloc(sizeof(struct exynos_ufc_freq) * size / 3, GFP_KERNEL);
		if (!table)
			return -ENOMEM;

		ret = of_property_read_u32_array(child, "table", (unsigned int *)table, size);
		if (ret)
			return -EINVAL;

		pr_info("Register UFC Type-%d Execution Mode-%d for Domain %d \n",
				ufc->info.ctrl_type, ufc->info.exe_mode,domain->id);

		for (index = 0; index < domain->table_size; index++) {
			unsigned int freq = domain->freq_table[index].frequency;

			if (freq == CPUFREQ_ENTRY_INVALID)
				continue;

			for (c_index = 0; c_index < size / 3; c_index++) {
				if (freq <= table[c_index].master_freq) {
					if (table[c_index].limit_freq > 0)
						ufc->info.freq_table[index].limit_freq = table[c_index].limit_freq;
					else
						ufc->info.freq_table[index].limit_freq = 0;

					if (table[c_index].last_freq > 0)
						ufc->info.freq_table[index].last_freq = table[c_index].last_freq;
					else
						ufc->info.freq_table[index].last_freq = 0;

				}
				if (freq >= table[c_index].master_freq)
					break;
			}
			pr_info("Master_freq : %u kHz - limit_freq : %u kHz\n",
					ufc->info.freq_table[index].master_freq,
					ufc->info.freq_table[index].limit_freq);
		}
		kfree(table);
	}

	return 0;
}

static int __init exynos_ufc_init(void)
{
	struct device_node *dn = NULL;
	const char *buf;
	struct exynos_cpufreq_domain *domain;
	int ret = 0;

	while((dn = of_find_node_by_type(dn, "cpufreq-userctrl"))) {
		struct cpumask shared_mask;

		ret = of_property_read_string(dn, "shared-cpus", &buf);
		if (ret) {
			pr_err("failed to get shared-cpus for ufc\n");
			goto exit;
		}

		cpulist_parse(buf, &shared_mask);
		domain = find_domain_cpumask(&shared_mask);
		if (!domain) {
			pr_err("Can't found domain for ufc!\n");
			goto exit;
		}

		/* Initialize user control information from dt */
		ret = parse_ufc_ctrl_info(domain, dn);
		if (ret) {
			pr_err("failed to get ufc ctrl info\n");
			goto exit;
		}

		/* Parse user frequency ctrl table info from dt */
		ret = init_ufc_table_dt(domain, dn);
		if (ret) {
			pr_err("failed to parse frequency table for ufc ctrl\n");
			goto exit;
		}
		/* Initialize PM QoS */
		init_pm_qos(domain);
		pr_info("Complete to initialize domain%d\n",domain->id);
	}

	init_sysfs();

	pr_info("Initialized Exynos UFC(User-Frequency-Ctrl) driver\n");
exit:
	return 0;
}
late_initcall(exynos_ufc_init);
