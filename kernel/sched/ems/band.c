/*
 * thread group band
 *
 * Copyright (C) 2018 Samsung Electronics Co., Ltd
 * Park Bumgyu <bumgyu.park@samsung.com>
 */

#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/ems.h>
#include <linux/sched/signal.h>
#include <trace/events/ems.h>

#include "ems.h"
#include "../sched.h"

static struct task_band *lookup_band(struct task_struct *p)
{
	struct task_band *band;

	rcu_read_lock();
	band = rcu_dereference(p->band);
	rcu_read_unlock();

	if (!band)
		return NULL;

	return band;
}

int band_play_cpu(struct task_struct *p)
{
	struct task_band *band;
	int cpu, min_cpu = -1;
	unsigned long min_util = ULONG_MAX;

	band = lookup_band(p);
	if (!band || cpumask_empty(&band->playable_cpus))
		return -1;

	for_each_cpu_and(cpu, cpu_online_mask, &band->playable_cpus) {
		if (!cpu_rq(cpu)->nr_running)
			return cpu;

		if (ml_cpu_util(cpu) < min_util) {
			min_cpu = cpu;
			min_util = ml_cpu_util(cpu);
		}
	}

	return min_cpu;
}

static int can_play_one_stage(const struct cpumask *stages, int ratio, int type)
{
	unsigned long capacity, util;
	int cpu;

	for_each_cpu(cpu, stages) {
		capacity = capacity_orig_of(cpu);
		util = ml_cpu_util(cpu);
		trace_ems_band_schedule(*(unsigned int *)cpumask_bits(stages),
					cpu, ratio, capacity, util, type);

		/* can't play here */
		if (util * 100 > capacity * ratio)
			return 0;
	}

	return 1;
}

static int can_play_all_stage(const struct cpumask *stages, int ratio, int type)
{
	unsigned long capacity = 0, util = 0;
	int cpu;

	for_each_cpu(cpu, stages) {
		capacity += capacity_orig_of(cpu);
		util += ml_cpu_util(cpu);
	}

	trace_ems_band_schedule(*(unsigned int *)cpumask_bits(stages),
					-1, ratio, capacity, util, type);
	/* can't play here */
	if (util * 100 > capacity * ratio)
		return 0;

	return 1;
}

#define BAND_SCHEDULE_ONE_STAGE		1
#define BAND_SCHEDULE_ALL_STAGE		2

static int can_play(const struct cpumask *stages, int ratio, int type)
{
	if (type == BAND_SCHEDULE_ONE_STAGE)
		return can_play_one_stage(stages, ratio, type);

	if (type == BAND_SCHEDULE_ALL_STAGE)
		return can_play_all_stage(stages, ratio, type);

	return 0;
}

struct band_schedule {
	struct list_head list;
	int number;
	struct cpumask stage;
	unsigned int crowd;
	int type;
};

LIST_HEAD(schedule_list);

static void pick_playable_cpus(struct task_band *band)
{
	struct band_schedule *bs;

	cpumask_clear(&band->playable_cpus);

	list_for_each_entry(bs, &schedule_list, list) {
		if (can_play(&bs->stage, bs->crowd, bs->type)) {
			cpumask_copy(&band->playable_cpus, &bs->stage);
			return;
		}
	}
}

static unsigned long out_of_time = 100000000;	/* 100ms */

/* This function should be called protected with band->lock */
static void __update_band(struct task_band *band, unsigned long now)
{
	struct task_struct *task;
	unsigned long util_sum = 0;

	list_for_each_entry(task, &band->members, band_members) {
		if (now - task->se.avg.last_update_time > out_of_time)
			continue;
		util_sum += ml_task_util_est(task);
	}

	band->util = util_sum;
	band->last_update_time = now;

	pick_playable_cpus(band);

	trace_ems_update_band(band->id, band->util, band->member_count,
		*(unsigned int *)cpumask_bits(&band->playable_cpus));
}

static int update_interval = 20000000;	/* 20ms */

void update_band(struct task_struct *p, long old_util)
{
	struct task_band *band;
	unsigned long now = cpu_rq(0)->clock_task;

	band = lookup_band(p);
	if (!band)
		return;

	/*
	 * Updates the utilization of the band only when it has been enough time
	 * to update the utilization of the band, or when the utilization of the
	 * task changes abruptly.
	 */
	if (now - band->last_update_time >= update_interval ||
	    (old_util >= 0 && abs(old_util - ml_task_util(p)) > (SCHED_CAPACITY_SCALE >> 4))) {
		if (raw_spin_trylock(&band->lock)) {
			__update_band(band, now);
			raw_spin_unlock(&band->lock);
		}
	}
}

#define MAX_NUM_BAND_ID		20
static struct task_band *bands[MAX_NUM_BAND_ID];

DEFINE_RWLOCK(band_rwlock);

#define band_playing(band)	(band->tgid >= 0)
static void join_band(struct task_struct *p)
{
	struct task_band *band;
	int pos, empty = -1;
	char event[30] = "join band";

	write_lock(&band_rwlock);

	if (lookup_band(p)) {
		write_unlock(&band_rwlock);
		return;
	}

	/*
	 * Find the band assigned to the tasks's thread group in the
	 * band pool. If there is no band assigend to thread group, it
	 * indicates that the task is the first one in the thread group
	 * to join the band. In this case, assign the first empty band
	 * in the band pool to the thread group.
	 */
	for (pos = 0; pos < MAX_NUM_BAND_ID; pos++) {
		band = bands[pos];

		if (!band_playing(band)) {
			if (empty < 0)
				empty = pos;
			continue;
		}

		if (p->tgid == band->tgid)
			break;
	}

	/* failed to find band, organize the new band */
	if (pos == MAX_NUM_BAND_ID) {
		/* band pool is full */
		if (empty < 0) {
			write_unlock(&band_rwlock);
			return;
		}
		band = bands[empty];
	}

	raw_spin_lock(&band->lock);
	if (!band_playing(band)) {
		band->tgid = p->tgid;
		band->sse = p->sse;
	}
	list_add(&p->band_members, &band->members);
	rcu_assign_pointer(p->band, band);
	band->member_count++;
	trace_ems_manage_band(p, band->id, event);

	__update_band(band, cpu_rq(0)->clock_task);
	raw_spin_unlock(&band->lock);

	write_unlock(&band_rwlock);
}

static void leave_band(struct task_struct *p)
{
	struct task_band *band;
	char event[30] = "leave band";

	write_lock(&band_rwlock);

	band = lookup_band(p);
	if (!band) {
		write_unlock(&band_rwlock);
		return;
	}

	raw_spin_lock(&band->lock);
	list_del_init(&p->band_members);
	rcu_assign_pointer(p->band, NULL);
	band->member_count--;
	trace_ems_manage_band(p, band->id, event);

	/* last member of band, band split up */
	if (list_empty(&band->members)) {
		band->tgid = -1;
		cpumask_clear(&band->playable_cpus);
	}

	__update_band(band, cpu_rq(0)->clock_task);
	raw_spin_unlock(&band->lock);

	write_unlock(&band_rwlock);
}

void sync_band(struct task_struct *p, bool join)
{
	if (join)
		join_band(p);
	else
		leave_band(p);
}

void newbie_join_band(struct task_struct *newbie)
{
	unsigned long flags;
	struct task_band *band;
	struct task_struct *leader = newbie->group_leader;
	char event[30] = "newbie join band";

	if (thread_group_leader(newbie))
		return;

	write_lock_irqsave(&band_rwlock, flags);

	band = lookup_band(leader);
	if (!band || newbie->band) {
		write_unlock_irqrestore(&band_rwlock, flags);
		return;
	}

	raw_spin_lock(&band->lock);
	list_add(&newbie->band_members, &band->members);
	rcu_assign_pointer(newbie->band, band);
	band->member_count++;
	trace_ems_manage_band(newbie, band->id, event);
	raw_spin_unlock(&band->lock);

	write_unlock_irqrestore(&band_rwlock, flags);
}

static ssize_t show_band_schedule(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct band_schedule *bs;
	int len = 0;

	list_for_each_entry(bs, &schedule_list, list)
		len += sprintf(buf + len,
			"[band-schedule%d stage(%#x)] crowd:%d\n",
			bs->number, *(unsigned int *)cpumask_bits(&bs->stage),
			bs->crowd);

	return len;
}

static ssize_t store_band_schedule(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf,
		size_t count)
{
	struct band_schedule *bs;
	int number, crowd;

	if (sscanf(buf, "%d %d", &number, &crowd) != 2)
		return -EINVAL;

	if (crowd < 0 || crowd > 100)
		return -EINVAL;

	list_for_each_entry(bs, &schedule_list, list)
		if (number == bs->number)
			bs->crowd = crowd;

	return count;
}

static struct kobj_attribute band_schedule_attr =
__ATTR(band_schedule, 0644, show_band_schedule, store_band_schedule);

static int __init init_band_sysfs(void)
{
	int ret;

	ret = sysfs_create_file(ems_kobj, &band_schedule_attr.attr);
	if (ret)
		pr_err("%s: faile to create sysfs file\n", __func__);

	return 0;
}
late_initcall(init_band_sysfs);

static void __init init_band_schedule(void)
{
	struct device_node *dn, *child;
	struct band_schedule *bs;
	const char *buf;
	int number = 0;

	dn = of_find_node_by_name(NULL, "ems");
	dn = of_find_node_by_name(dn, "band");

	for_each_child_of_node(dn, child) {
		bs = kzalloc(sizeof(struct band_schedule), GFP_KERNEL);
		if (!bs)
			goto init_fail;

		bs->number = number;
		if (!of_property_read_string(child, "stage", &buf))
			cpulist_parse(buf, &bs->stage);
		of_property_read_u32(child, "crowd", &bs->crowd);
		of_property_read_u32(child, "type", &bs->type);

		list_add_tail(&bs->list, &schedule_list);
		number++;
	}

	list_for_each_entry(bs, &schedule_list, list)
		pr_info("[band schedule%d] stage=%#x, crowd=%d\n",
			bs->number, *(unsigned int *)cpumask_bits(&bs->stage),
			bs->crowd);

	return;

init_fail:
	list_for_each_entry(bs, &schedule_list, list)
		kfree(bs);
}

int alloc_bands(void)
{
	struct task_band *band;
	int pos, ret, i;

	for (pos = 0; pos < MAX_NUM_BAND_ID; pos++) {
		band = kzalloc(sizeof(*band), GFP_KERNEL);
		if (!band) {
			ret = -ENOMEM;
			goto fail;
		}

		band->id = pos;
		band->sse = 0;
		band->tgid = -1;
		raw_spin_lock_init(&band->lock);
		INIT_LIST_HEAD(&band->members);
		band->member_count = 0;
		cpumask_clear(&band->playable_cpus);

		bands[pos] = band;
	}

	init_band_schedule();

	return 0;

fail:
	for (i = pos - 1; i >= 0; i--) {
		kfree(bands[i]);
		bands[i] = NULL;
	}

	return ret;
}
