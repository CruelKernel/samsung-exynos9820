/*
 * Services for Exynos Mobile Scheduler
 *
 * Copyright (C) 2018 Samsung Electronics Co., Ltd
 * Park Bumgyu <bumgyu.park@samsung.com>
 */

#include <linux/kobject.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/ems_service.h>
#include <trace/events/ems.h>

#include "../sched.h"
#include "../tune.h"
#include "ems.h"

/**********************************************************************
 *                        Kernel Prefer Perf                          *
 **********************************************************************/
struct plist_head kpp_list[STUNE_GROUP_COUNT];

static bool kpp_en;

bool ib_ems_initialized;
EXPORT_SYMBOL(ib_ems_initialized);

int kpp_status(int grp_idx)
{
	if (unlikely(!kpp_en))
		return 0;

	if (grp_idx >= STUNE_GROUP_COUNT)
		return -EINVAL;

	if (plist_head_empty(&kpp_list[grp_idx]))
		return 0;

	return plist_last(&kpp_list[grp_idx])->prio;
}

static DEFINE_SPINLOCK(kpp_lock);

void kpp_request(int grp_idx, struct kpp *req, int value)
{
	unsigned long flags;

	if (unlikely(!kpp_en))
		return;

	if (grp_idx >= STUNE_GROUP_COUNT)
		return;

	if (req->node.prio == value)
		return;

	spin_lock_irqsave(&kpp_lock, flags);

	/*
	 * If the request already added to the list updates the value, remove
	 * the request from the list and add it again.
	 */
	if (req->active)
		plist_del(&req->node, &kpp_list[req->grp_idx]);
	else
		req->active = 1;

	plist_node_init(&req->node, value);
	plist_add(&req->node, &kpp_list[grp_idx]);
	req->grp_idx = grp_idx;

	spin_unlock_irqrestore(&kpp_lock, flags);
}

static void __init init_kpp(void)
{
	int i;

	for (i = 0; i < STUNE_GROUP_COUNT; i++)
		plist_head_init(&kpp_list[i]);

	kpp_en = 1;
}

struct prefer_perf {
	int			boost;

	unsigned int		light_threshold;
	unsigned int		light_threshold_s;
	unsigned int		heavy_threshold;
	unsigned int		heavy_threshold_s;

	unsigned int		coregroup_count;
	unsigned int		light_coregroup_count;
	unsigned int		heavy_coregroup_count;

	struct cpumask		*prefer_cpus;
	struct cpumask		*light_prefer_cpus;
	struct cpumask		*heavy_prefer_cpus;
};

static struct prefer_perf *prefer_perf_services;
static int prefer_perf_service_count;

static struct prefer_perf *find_prefer_perf(int boost)
{
	int i;

	for (i = 0; i < prefer_perf_service_count; i++)
		if (prefer_perf_services[i].boost == boost)
			return &prefer_perf_services[i];

	return NULL;
}

static int
select_prefer_cpu(struct task_struct *p, int coregroup_count, struct cpumask *prefer_cpus)
{
	struct cpumask mask;
	int coregroup, cpu;
	unsigned long max_spare_cap = 0;
	int best_perf_cstate = INT_MAX;
	int best_perf_cpu = -1;
	int backup_cpu = -1;

	rcu_read_lock();

	for (coregroup = 0; coregroup < coregroup_count; coregroup++) {
		cpumask_and(&mask, &prefer_cpus[coregroup], cpu_active_mask);
		if (cpumask_empty(&mask))
			continue;

		for_each_cpu_and(cpu, &p->cpus_allowed, &mask) {
			unsigned long capacity_orig;
			unsigned long wake_util_with;
			unsigned long wake_util_without;

			if (cpu >= nr_cpu_ids)
				break;

			if (idle_cpu(cpu)) {
				int idle_idx = idle_get_state_idx(cpu_rq(cpu));

				/* find shallowest idle state cpu */
				if (idle_idx >= best_perf_cstate)
					continue;

				/* Keep track of best idle CPU */
				best_perf_cstate = idle_idx;
				best_perf_cpu = cpu;
				continue;
			}

			capacity_orig = capacity_orig_of_sse(cpu, p->sse);

			/* In case of over-capacity */
			wake_util_with = ml_task_attached_cpu_util(cpu, p);
			if (capacity_orig < wake_util_with)
				continue;

			wake_util_without = ml_cpu_util_wake(cpu, p);
			if ((capacity_orig - wake_util_without) < max_spare_cap)
				continue;

			max_spare_cap = capacity_orig - wake_util_without;
			backup_cpu = cpu;
		}

		if (cpu_selected(best_perf_cpu))
			break;
	}

	rcu_read_unlock();

	trace_ems_select_service_cpu(p, best_perf_cpu, backup_cpu);

	if (best_perf_cpu == -1)
		return backup_cpu;

	return best_perf_cpu;
}

int select_service_cpu(struct task_struct *p)
{
	struct prefer_perf *pp;
	int boost, service_cpu;
	unsigned long util, light_threshold, heavy_threshold;
	char state[30];

	if (!prefer_perf_services)
		return -1;

	boost = schedtune_prefer_perf(p);
	if (boost <= 0)
		return -1;

	pp = find_prefer_perf(boost);
	if (!pp)
		return -1;

	light_threshold = p->sse ? pp->light_threshold_s : pp->light_threshold;
	heavy_threshold = p->sse ? pp->heavy_threshold_s : pp->heavy_threshold;

	util = ml_task_util_est(p);
	if (pp->light_coregroup_count > 0 && util <= light_threshold) {
		service_cpu = select_prefer_cpu(p, pp->light_coregroup_count,
							pp->light_prefer_cpus);
		if (cpu_selected(service_cpu)) {
			strcpy(state, "light task");
			goto out;
		}
	} else if (pp->heavy_coregroup_count > 0 && util >= heavy_threshold) {
		service_cpu = select_prefer_cpu(p, pp->heavy_coregroup_count,
							pp->heavy_prefer_cpus);
		if (cpu_selected(service_cpu)) {
			strcpy(state, "heavy task");
			goto out;
		}
	}

	service_cpu = select_prefer_cpu(p, pp->coregroup_count, pp->prefer_cpus);
	strcpy(state, "normal task");

out:
	trace_ems_prefer_perf_service(p, util, service_cpu, state);
	return service_cpu;
}

static void __init build_prefer_cpus(void)
{
	struct device_node *dn, *child;
	int index = 0;

	dn = of_find_node_by_name(NULL, "ems");
	dn = of_find_node_by_name(dn, "prefer-perf-service");
	prefer_perf_service_count = of_get_child_count(dn);

	prefer_perf_services = kcalloc(prefer_perf_service_count,
				sizeof(struct prefer_perf), GFP_KERNEL);
	if (!prefer_perf_services)
		return;

	for_each_child_of_node(dn, child) {
		const char *mask[NR_CPUS];
		int i, proplen;

		if (index >= prefer_perf_service_count)
			return;

		of_property_read_u32(child, "boost",
					&prefer_perf_services[index].boost);

		proplen = of_property_count_strings(child, "prefer-cpus");
		if (proplen < 0)
			goto next;

		prefer_perf_services[index].coregroup_count = proplen;

		of_property_read_string_array(child, "prefer-cpus", mask, proplen);
		prefer_perf_services[index].prefer_cpus = kcalloc(proplen,
						sizeof(struct cpumask), GFP_KERNEL);

		for (i = 0; i < proplen; i++)
			cpulist_parse(mask[i], &prefer_perf_services[index].prefer_cpus[i]);

		/* For light task processing */
		if (of_property_read_u32(child, "light-task-threshold",
				&prefer_perf_services[index].light_threshold))
			prefer_perf_services[index].light_threshold = 0;
		if (of_property_read_u32(child, "light-task-threshold-s",
				&prefer_perf_services[index].light_threshold_s))
			prefer_perf_services[index].light_threshold_s = 0;

		proplen = of_property_count_strings(child, "light-prefer-cpus");
		if (proplen < 0) {
			prefer_perf_services[index].light_coregroup_count = 0;
			goto heavy;
		}

		prefer_perf_services[index].light_coregroup_count = proplen;

		of_property_read_string_array(child, "light-prefer-cpus", mask, proplen);
		prefer_perf_services[index].light_prefer_cpus = kcalloc(proplen,
						sizeof(struct cpumask), GFP_KERNEL);

		for (i = 0; i < proplen; i++)
			cpulist_parse(mask[i], &prefer_perf_services[index].light_prefer_cpus[i]);
heavy:
		/* For heavy task processing */
		if (of_property_read_u32(child, "heavy-task-threshold",
				&prefer_perf_services[index].heavy_threshold))
			prefer_perf_services[index].heavy_threshold = UINT_MAX;
		if (of_property_read_u32(child, "heavy-task-threshold-s",
				&prefer_perf_services[index].heavy_threshold_s))
			prefer_perf_services[index].heavy_threshold_s = UINT_MAX;

		proplen = of_property_count_strings(child, "heavy-prefer-cpus");
		if (proplen < 0) {
			prefer_perf_services[index].heavy_coregroup_count = 0;
			goto next;
		}

		prefer_perf_services[index].heavy_coregroup_count = proplen;

		of_property_read_string_array(child, "heavy-prefer-cpus", mask, proplen);
		prefer_perf_services[index].heavy_prefer_cpus = kcalloc(proplen,
						sizeof(struct cpumask), GFP_KERNEL);

		for (i = 0; i < proplen; i++)
			cpulist_parse(mask[i], &prefer_perf_services[index].heavy_prefer_cpus[i]);
next:
		index++;
	}
}

static ssize_t show_kpp(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	int i, ret = 0;

	/* shows the prefer_perf value of all schedtune groups */
	for (i = 0; i < STUNE_GROUP_COUNT; i++)
		ret += snprintf(buf + ret, 10, "%d ", kpp_status(i));

	ret += snprintf(buf + ret, 10, "\n");

	return ret;
}

static struct kobj_attribute kpp_attr =
__ATTR(kernel_prefer_perf, 0444, show_kpp, NULL);

static ssize_t show_light_task_threshold(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	int i, ret = 0;

	for (i = 0; i < prefer_perf_service_count; i++)
		ret += snprintf(buf + ret, 40, "boost=%d light-task-threshold=%d\n",
				prefer_perf_services[i].boost,
				prefer_perf_services[i].light_threshold);

	return ret;
}

static ssize_t store_light_task_threshold(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf,
		size_t count)
{
	int i, boost, threshold, ret;

	ret = sscanf(buf, "%d %d", &boost, &threshold);
	if (ret != 2)
		return -EINVAL;

	if (boost < 0 || threshold < 0)
		return -EINVAL;

	for (i = 0; i < prefer_perf_service_count; i++)
		if (prefer_perf_services[i].boost == boost)
			prefer_perf_services[i].light_threshold = threshold;

	return count;
}

static struct kobj_attribute light_task_threshold_attr =
__ATTR(light_task_threshold, 0644, show_light_task_threshold, store_light_task_threshold);

static ssize_t show_light_task_threshold_s(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	int i, ret = 0;

	for (i = 0; i < prefer_perf_service_count; i++)
		ret += snprintf(buf + ret, 40, "boost=%d light-task-threshold-s=%d\n",
				prefer_perf_services[i].boost,
				prefer_perf_services[i].light_threshold_s);

	return ret;
}

static ssize_t store_light_task_threshold_s(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf,
		size_t count)
{
	int i, boost, threshold, ret;

	ret = sscanf(buf, "%d %d", &boost, &threshold);
	if (ret != 2)
		return -EINVAL;

	if (boost < 0 || threshold < 0)
		return -EINVAL;

	for (i = 0; i < prefer_perf_service_count; i++)
		if (prefer_perf_services[i].boost == boost)
			prefer_perf_services[i].light_threshold_s = threshold;

	return count;
}

static struct kobj_attribute light_task_threshold_s_attr =
__ATTR(light_task_threshold_s, 0644, show_light_task_threshold_s, store_light_task_threshold_s);

static ssize_t show_heavy_task_threshold(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	int i, ret = 0;

	for (i = 0; i < prefer_perf_service_count; i++)
		ret += snprintf(buf + ret, 40, "boost=%d heavy-task-threshold=%d\n",
				prefer_perf_services[i].boost,
				prefer_perf_services[i].heavy_threshold);

	return ret;
}

static ssize_t store_heavy_task_threshold(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf,
		size_t count)
{
	int i, boost, threshold, ret;

	ret = sscanf(buf, "%d %d", &boost, &threshold);
	if (ret != 2)
		return -EINVAL;

	if (boost < 0 || threshold < 0)
		return -EINVAL;

	for (i = 0; i < prefer_perf_service_count; i++)
		if (prefer_perf_services[i].boost == boost)
			prefer_perf_services[i].heavy_threshold = threshold;

	return count;
}

static struct kobj_attribute heavy_task_threshold_attr =
__ATTR(heavy_task_threshold, 0644, show_heavy_task_threshold, store_heavy_task_threshold);

static ssize_t show_heavy_task_threshold_s(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	int i, ret = 0;

	for (i = 0; i < prefer_perf_service_count; i++)
		ret += snprintf(buf + ret, 40, "boost=%d heavy-task-threshold-s=%d\n",
				prefer_perf_services[i].boost,
				prefer_perf_services[i].heavy_threshold_s);

	return ret;
}

static ssize_t store_heavy_task_threshold_s(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf,
		size_t count)
{
	int i, boost, threshold, ret;

	ret = sscanf(buf, "%d %d", &boost, &threshold);
	if (ret != 2)
		return -EINVAL;

	if (boost < 0 || threshold < 0)
		return -EINVAL;

	for (i = 0; i < prefer_perf_service_count; i++)
		if (prefer_perf_services[i].boost == boost)
			prefer_perf_services[i].heavy_threshold_s = threshold;

	return count;
}

static struct kobj_attribute heavy_task_threshold_s_attr =
__ATTR(heavy_task_threshold_s, 0644, show_heavy_task_threshold_s, store_heavy_task_threshold_s);

static struct attribute *service_attrs[] = {
	&kpp_attr.attr,
	&light_task_threshold_attr.attr,
	&light_task_threshold_s_attr.attr,
	&heavy_task_threshold_attr.attr,
	&heavy_task_threshold_s_attr.attr,
	NULL,
};

static const struct attribute_group service_group = {
	.attrs = service_attrs,
};

static struct kobject *service_kobj;

static int __init init_service(void)
{
	int ret;

	init_kpp();

	build_prefer_cpus();

	service_kobj = kobject_create_and_add("service", ems_kobj);
	if (!service_kobj) {
		pr_err("Fail to create ems service kboject\n");
		return -EINVAL;
	}

	ret = sysfs_create_group(service_kobj, &service_group);
	if (ret) {
		pr_err("Fail to create ems service group\n");
		return ret;
	}

	ib_ems_initialized = true;
	return 0;
}
late_initcall(init_service);
