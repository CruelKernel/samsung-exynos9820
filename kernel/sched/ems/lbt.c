/*
 * Load Balance Trigger (LBT) for Exynos Mobile Scheduler (EMS)
 *
 * Copyright (C) 2018 Samsung Electronics Co., Ltd
 * LEE DAEYEONG <daeyeong.lee@samsung.com>
 */

#include <linux/sched.h>
#include <linux/cpuidle.h>
#include <linux/pm_qos.h>
#include <linux/ems.h>
#include <linux/sched/energy.h>

#include <trace/events/ems.h>

#include "../sched.h"
#include "../tune.h"
#include "./ems.h"

/****************************************************************/
/*			Load Balance Trigger			*/
/****************************************************************/
#define DISABLE_OU		-1
#define DEFAULT_OU_RATIO	80

struct lbt_overutil {
	bool			top;
	struct cpumask		cpus;
	unsigned long		capacity;
	int			ratio;
};
DEFINE_PER_CPU(struct lbt_overutil *, lbt_overutil);

static inline struct sched_domain *find_sd_by_level(int cpu, int level)
{
	struct sched_domain *sd;

	for_each_domain(cpu, sd) {
		if (sd->level == level)
			return sd;
	}

	return NULL;
}

static inline int get_topology_depth(void)
{
	struct sched_domain *sd;

	for_each_domain(0, sd) {
		if (sd->parent == NULL)
			return sd->level;
	}

	return -1;
}

static inline int get_last_level(struct lbt_overutil *ou)
{
	int level;

	for (level = 0; &ou[level] != NULL; level++) {
		if (ou[level].top == true)
			return level;
	}

	return -1;
}

/****************************************************************/
/*			External APIs				*/
/****************************************************************/
bool lbt_overutilized(int cpu, int level)
{
	struct lbt_overutil *ou = per_cpu(lbt_overutil, cpu);
	bool overutilized;

	if (!ou)
		return false;

	overutilized = (ml_cpu_util(cpu) > ou[level].capacity) ? true : false;

	if (overutilized)
		trace_ems_lbt_overutilized(cpu, level, ml_cpu_util(cpu),
				ou[level].capacity, overutilized);

	return overutilized;
}

void update_lbt_overutil(int cpu, unsigned long capacity)
{
	struct lbt_overutil *ou = per_cpu(lbt_overutil, cpu);
	int level, last = get_last_level(ou);

	for (level = 0; level <= last; level++) {
		if (ou[level].ratio == DISABLE_OU)
			continue;

		ou[level].capacity = (capacity * ou[level].ratio) / 100;
	}
}

/****************************************************************/
/*				SYSFS				*/
/****************************************************************/
#define lbt_attr_init(_attr, _name, _mode, _show, _store)		\
	sysfs_attr_init(&_attr.attr);					\
	_attr.attr.name = _name;					\
	_attr.attr.mode = VERIFY_OCTAL_PERMISSIONS(_mode);		\
	_attr.show	= _show;					\
	_attr.store	= _store;

static struct kobject *lbt_kobj;
static struct attribute **lbt_attrs;
static struct kobj_attribute *lbt_kattrs;
static struct attribute_group lbt_group;

static ssize_t show_overutil_ratio(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct lbt_overutil *ou = per_cpu(lbt_overutil, 0);
	int level = attr - lbt_kattrs;
	int cpu, ret = 0;

	for_each_possible_cpu(cpu) {
		ou = per_cpu(lbt_overutil, cpu);

		if (ou[level].ratio == DISABLE_OU)
			continue;

		ret += sprintf(buf + ret, "cpu%d ratio:%3d capacity:%4lu\n",
				cpu, ou[level].ratio, ou[level].capacity);
	}

	return ret;
}

static ssize_t store_overutil_ratio(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf,
		size_t count)
{
	struct lbt_overutil *ou;
	unsigned long capacity;
	int level = attr - lbt_kattrs;
	int cpu, ratio;

	if (sscanf(buf, "%d %d", &cpu, &ratio) != 2)
		return -EINVAL;

	/* Check cpu is possible */
	if (!cpumask_test_cpu(cpu, cpu_possible_mask))
		return -EINVAL;
	ou = per_cpu(lbt_overutil, cpu);

	/* If ratio is outrage, disable overutil */
	if (ratio < 0 || ratio > 100)
		ratio = DEFAULT_OU_RATIO;

	for_each_cpu(cpu, &ou[level].cpus) {
		ou = per_cpu(lbt_overutil, cpu);
		if (ou[level].ratio == DISABLE_OU)
			continue;

		ou[level].ratio = ratio;
		capacity = capacity_orig_of(cpu);
		update_lbt_overutil(cpu, capacity);
	}

	return count;
}

static int alloc_lbt_sysfs(int size)
{
	if (size < 0)
		return -EINVAL;

	lbt_attrs = kzalloc(sizeof(struct attribute *) * (size + 1),
			GFP_KERNEL);
	if (!lbt_attrs)
		goto fail_alloc;

	lbt_kattrs = kzalloc(sizeof(struct kobj_attribute) * (size),
			GFP_KERNEL);
	if (!lbt_kattrs)
		goto fail_alloc;

	return 0;

fail_alloc:
	kfree(lbt_attrs);
	kfree(lbt_kattrs);

	pr_err("LBT(%s): failed to alloc sysfs attrs\n", __func__);
	return -ENOMEM;
}

static int __init lbt_sysfs_init(void)
{
	int depth = get_topology_depth();
	int i;

	if (alloc_lbt_sysfs(depth + 1))
		goto out;

	for (i = 0; i <= depth; i++) {
		char buf[25];
		char *name;

		scnprintf(buf, sizeof(buf), "overutil_ratio_level%d", i);
		name = kstrdup(buf, GFP_KERNEL);
		if (!name)
			goto out;

		lbt_attr_init(lbt_kattrs[i], name, 0644,
				show_overutil_ratio, store_overutil_ratio);
		lbt_attrs[i] = &lbt_kattrs[i].attr;
	}

	lbt_group.attrs = lbt_attrs;

	lbt_kobj = kobject_create_and_add("lbt", ems_kobj);
	if (!lbt_kobj)
		goto out;

	if (sysfs_create_group(lbt_kobj, &lbt_group))
		goto out;

	return 0;

out:
	kfree(lbt_attrs);
	kfree(lbt_kattrs);

	pr_err("LBT(%s): failed to create sysfs node\n", __func__);
	return -EINVAL;
}
late_initcall(lbt_sysfs_init);

/****************************************************************/
/*			Initialization				*/
/****************************************************************/
static void free_lbt_overutil(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		if (per_cpu(lbt_overutil, cpu))
			kfree(per_cpu(lbt_overutil, cpu));
	}
}

static int alloc_lbt_overutil(void)
{
	int cpu, depth = get_topology_depth();

	for_each_possible_cpu(cpu) {
		struct lbt_overutil *ou = kzalloc(sizeof(struct lbt_overutil) *
				(depth + 1), GFP_KERNEL);
		if (!ou)
			goto fail_alloc;

		per_cpu(lbt_overutil, cpu) = ou;
	}
	return 0;

fail_alloc:
	free_lbt_overutil();
	return -ENOMEM;
}

static void default_lbt_overutil(int level)
{
	struct sched_domain *sd;
	struct lbt_overutil *ou;
	struct cpumask cpus;
	bool top;
	int cpu;

	/* If current level is same with topology depth, it is top level */
	top = !(get_topology_depth() - level);

	cpumask_clear(&cpus);

	for_each_possible_cpu(cpu) {
		int c;

		if (cpumask_test_cpu(cpu, &cpus))
			continue;

		sd = find_sd_by_level(cpu, level);
		if (!sd) {
			ou = per_cpu(lbt_overutil, cpu);
			ou[level].ratio = DISABLE_OU;
			ou[level].top = top;
			continue;
		}

		cpumask_copy(&cpus, sched_domain_span(sd));
		for_each_cpu(c, &cpus) {
			ou = per_cpu(lbt_overutil, c);
			cpumask_copy(&ou[level].cpus, &cpus);
			ou[level].ratio = DEFAULT_OU_RATIO;
			ou[level].top = top;
		}
	}
}

static void set_lbt_overutil(int level, const char *mask, int ratio)
{
	struct lbt_overutil *ou;
	struct cpumask cpus;
	bool top, overlap = false;
	int cpu;

	cpulist_parse(mask, &cpus);
	cpumask_and(&cpus, &cpus, cpu_possible_mask);
	if (!cpumask_weight(&cpus))
		return;

	/* If current level is same with topology depth, it is top level */
	top = !(get_topology_depth() - level);

	/* If this level is overlapped with prev level, disable this level */
	if (level > 0) {
		ou = per_cpu(lbt_overutil, cpumask_first(&cpus));
		overlap = cpumask_equal(&cpus, &ou[level-1].cpus);
	}

	for_each_cpu(cpu, &cpus) {
		ou = per_cpu(lbt_overutil, cpu);
		cpumask_copy(&ou[level].cpus, &cpus);
		ou[level].ratio = overlap ? DISABLE_OU : ratio;
		ou[level].top = top;
	}
}

static void parse_lbt_overutil(struct device_node *dn)
{
	struct device_node *lbt, *ou;
	int level, depth = get_topology_depth();

	/* If lbt node isn't, set by default value (80%) */
	lbt = of_get_child_by_name(dn, "lbt");
	if (!lbt) {
		for (level = 0; level <= depth; level++)
			default_lbt_overutil(level);
		return;
	}

	for (level = 0; level <= depth; level++) {
		char name[20];
		const char *mask[NR_CPUS];
		struct cpumask combi, each;
		int ratio[NR_CPUS];
		int i, proplen;

		snprintf(name, sizeof(name), "overutil-level%d", level);
		ou = of_get_child_by_name(lbt, name);
		if (!ou)
			goto default_setting;

		proplen = of_property_count_strings(ou, "cpus");
		if ((proplen < 0) || (proplen != of_property_count_u32_elems(ou, "ratio"))) {
			of_node_put(ou);
			goto default_setting;
		}

		of_property_read_string_array(ou, "cpus", mask, proplen);
		of_property_read_u32_array(ou, "ratio", ratio, proplen);
		of_node_put(ou);

		/*
		 * If combination of each cpus doesn't correspond with
		 * cpu_possible_mask, do not use this property
		 */
		cpumask_clear(&combi);
		for (i = 0; i < proplen; i++) {
			cpulist_parse(mask[i], &each);
			cpumask_or(&combi, &combi, &each);
		}
		if (!cpumask_equal(&combi, cpu_possible_mask))
			goto default_setting;

		for (i = 0; i < proplen; i++)
			set_lbt_overutil(level, mask[i], ratio[i]);
		continue;

default_setting:
		default_lbt_overutil(level);
	}

	of_node_put(lbt);
}

static int __init init_lbt(void)
{
	struct device_node *dn = of_find_node_by_path("/cpus/ems");

	if (alloc_lbt_overutil()) {
		pr_err("LBT(%s): failed to allocate lbt_overutil\n", __func__);
		of_node_put(dn);
		return -ENOMEM;
	}

	parse_lbt_overutil(dn);
	of_node_put(dn);
	return 0;
}
pure_initcall(init_lbt);
