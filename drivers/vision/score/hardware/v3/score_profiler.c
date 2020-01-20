/*
 * Samsung Exynos SoC series SCore driver
 *
 * SCore profiler module
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/kobject.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include "score_log.h"
#include "score_system.h"
#include "score_profiler.h"
#include "score_regs.h"
#include "score_system.h"

/*
 * This value must be equal to PROF_CALL_INFO_SIZE
 * defined in sc_profiler_common.h file from SCore
 * firmware.
 */
#define PROF_STRUCT_SIZE	(40)

#define PROF_STRUCT_TS_COUNT	(SCORE_TS_PROFILER_SIZE / PROF_STRUCT_SIZE)
#define PROF_STRUCT_BR_COUNT	(SCORE_BR_PROFILER_SIZE / PROF_STRUCT_SIZE)

struct score_profiler {
	struct kobject		kobj;
	unsigned char		*kvaddr[SCORE_CORE_NUM];
	unsigned int		prof_count[SCORE_CORE_NUM];
	struct score_system	*system;
	spinlock_t		slock;
};

static struct score_profiler profiler;

static inline int __get_head(int id)
{
	if (id == SCORE_TS)
		return readl(profiler.system->sfr + PROFILER_TS_HEAD);
	else if (id == SCORE_BARON1)
		return readl(profiler.system->sfr + PROFILER_BR1_HEAD);
	else if (id == SCORE_BARON2)
		return readl(profiler.system->sfr + PROFILER_BR2_HEAD);

	score_warn("Getting head of profiler queue failed (id:%d)\n", id);
	return -EINVAL;
}

static inline int __get_tail(int id)
{
	if (id == SCORE_TS)
		return readl(profiler.system->sfr + PROFILER_TS_TAIL);
	else if (id == SCORE_BARON1)
		return readl(profiler.system->sfr + PROFILER_BR1_TAIL);
	else if (id == SCORE_BARON2)
		return readl(profiler.system->sfr + PROFILER_BR2_TAIL);

	score_warn("Getting tail of profiler queue failed (id:%d)\n", id);
	return -EINVAL;
}

static inline void __set_head(int value, int id)
{
	if (id == SCORE_TS)
		writel(value, profiler.system->sfr + PROFILER_TS_HEAD);
	else if (id == SCORE_BARON1)
		writel(value, profiler.system->sfr + PROFILER_BR1_HEAD);
	else if (id == SCORE_BARON2)
		writel(value, profiler.system->sfr + PROFILER_BR2_HEAD);
	else
		score_warn("Setting head of profiler queue failed (id:%d)\n",
				id);
}

static inline void __set_tail(int value, int id)
{
	if (id == SCORE_TS)
		writel(value, profiler.system->sfr + PROFILER_TS_TAIL);
	else if (id == SCORE_BARON1)
		writel(value, profiler.system->sfr + PROFILER_BR1_TAIL);
	else if (id == SCORE_BARON2)
		writel(value, profiler.system->sfr + PROFILER_BR2_TAIL);
	else
		score_warn("Setting tail of profiler queue failed (id:%d)\n",
				id);
}

static inline unsigned int __get_mask(void)
{
	return readl(profiler.system->sfr + PROFILER_TAG_MASK);
}

static inline void __set_mask(unsigned int value)
{
	writel(value, profiler.system->sfr + PROFILER_TAG_MASK);
}

static ssize_t score_profiler_mask_show(struct kobject *dev,
	struct kobj_attribute *attr, char *buf)
{
	int ret = -EINVAL;
	struct score_device *device;
	struct score_pm *pm;

	device = dev_get_drvdata(profiler.system->dev);
	if (!device)
		goto p_end;

	pm = &device->pm;
	mutex_lock(&pm->lock);
	if (score_pm_qos_active(pm))
		ret = scnprintf(buf, PAGE_SIZE, "%08x\n", __get_mask());

	mutex_unlock(&pm->lock);
p_end:
	return ret;
}

static ssize_t score_profiler_mask_store(struct kobject *dev,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret = -EINVAL;
	struct score_device *device;
	struct score_pm *pm;
	unsigned int value;

	device = dev_get_drvdata(profiler.system->dev);
	if (!device)
		goto p_end;

	pm = &device->pm;
	mutex_lock(&pm->lock);
	if (!score_pm_qos_active(pm))
		goto p_power;

	if (kstrtouint(buf, 16, &value) == 0) {
		__set_mask(value);
		ret = count;
	}

p_power:
	mutex_unlock(&pm->lock);
p_end:
	return ret;
}

static void profiler_distribute_entries(unsigned int max, unsigned int *core)
{
	unsigned int sum;
	unsigned int core_max;
	unsigned int extra;
	unsigned int temp[SCORE_CORE_NUM];
	int id;

	sum = core[SCORE_TS] + core[SCORE_BARON1] + core[SCORE_BARON2];
	if (sum > max) {
		core_max = max / 3;
		extra = 0;

		for (id = SCORE_TS; id < SCORE_CORE_NUM; ++id) {
			if (core[id] > core_max) {
				temp[id] = core[id] - core_max;
				core[id] = core_max;
			} else {
				temp[id] = 0;
				extra += (core_max - core[id]);
			}
		}

		if (!extra)
			return;

		for (id = SCORE_TS; id < SCORE_CORE_NUM && extra; ++id) {
			if (temp[id]) {
				if (temp[id] > extra) {
					core[id] += extra;
					extra = 0;
				} else {
					core[id] += temp[id];
					extra -= temp[id];
				}
			}
		}
	}
}

static dma_addr_t profiler_get_target_addr(int id)
{
	if (id == SCORE_TS)
		return readl(profiler.system->sfr + PROFILER_TS_BASE_ADDR) -
			readl(profiler.system->sfr + TS_CACHE_DC_BASE_ADDR);
	else if (id == SCORE_BARON1)
		return readl(profiler.system->sfr + PROFILER_BR1_BASE_ADDR) -
			readl(profiler.system->sfr + BR1_CACHE_DC_BASE_ADDR);
	else if (id == SCORE_BARON2)
		return readl(profiler.system->sfr + PROFILER_BR2_BASE_ADDR) -
			readl(profiler.system->sfr + BR2_CACHE_DC_BASE_ADDR);

	score_warn("Getting addr of target for profiler failed (id:%d)\n", id);
	return 0;
}

static ssize_t read_structs(char *buffer, unsigned int max_entries)
{
	int id;
	int head[SCORE_CORE_NUM];
	int tail[SCORE_CORE_NUM];
	unsigned int core_entries[SCORE_CORE_NUM];
	unsigned int read_entries;
	size_t size;
	dma_addr_t addr;
	int ret;

	if (!max_entries)
		return 0;

	for (id = SCORE_TS; id < SCORE_CORE_NUM; ++id) {
		head[id] = __get_head(id);
		if (head[id] < 0)
			return 0;

		tail[id] = __get_tail(id);
		if (tail[id] < 0)
			return 0;

		if (head[id] < tail[id])
			core_entries[id] = tail[id] - head[id];
		else if (head[id] > tail[id])
			core_entries[id] = profiler.prof_count[id] -
				(head[id] - tail[id]);
		else
			core_entries[id] = 0;
	}

	if (!core_entries[SCORE_TS] &&
			!core_entries[SCORE_BARON1] &&
			!core_entries[SCORE_BARON2])
		return 0;

	profiler_distribute_entries(max_entries, core_entries);
	read_entries = 0;

	for (id = SCORE_TS; id < SCORE_CORE_NUM; ++id) {
		if (!core_entries[id])
			continue;

		addr = profiler_get_target_addr(id);
		if (!addr)
			return 0;

		if ((head[id] + core_entries[id]) > profiler.prof_count[id]) {
			/* Copy from head loacation to the end of queue */
			size = profiler.prof_count[id] - head[id];

			ret = score_system_dcache_control(profiler.system, id,
					DCACHE_CLEAN);
			if (ret)
				return 0;

			memcpy(buffer, profiler.kvaddr[id] +
					head[id] * PROF_STRUCT_SIZE,
					size * PROF_STRUCT_SIZE);
			buffer += (size * PROF_STRUCT_SIZE);

			/* Copy from the start of queue to desired loacation */
			head[id] = core_entries[id] - size;
			size = head[id];

			ret = score_system_dcache_control(profiler.system, id,
					DCACHE_CLEAN);
			if (ret)
				return 0;

			memcpy(buffer, profiler.kvaddr[id],
					size * PROF_STRUCT_SIZE);
			buffer += (size * PROF_STRUCT_SIZE);
		} else {
			/* Copy once from queue because it is forward */
			size = core_entries[id];

			ret = score_system_dcache_control(profiler.system, id,
					DCACHE_CLEAN);
			if (ret)
				return 0;

			memcpy(buffer, profiler.kvaddr[id] +
					head[id] * PROF_STRUCT_SIZE,
					size * PROF_STRUCT_SIZE);
			buffer += (size * PROF_STRUCT_SIZE);

			if ((head[id] + core_entries[id]) ==
					profiler.prof_count[id])
				head[id] = 0;
			else
				head[id] += core_entries[id];
		}

		read_entries += core_entries[id];
		__set_head(head[id], id);
	}

	return read_entries * PROF_STRUCT_SIZE;
}

static ssize_t score_profiler_read(struct file *filp, struct kobject *kobj,
				struct bin_attribute *bin_attr,
				char *buffer, loff_t pos, size_t size)
{
	ssize_t retval;

	score_enter();
	spin_lock(&profiler.slock);

	if (!profiler.kvaddr[SCORE_TS]
		|| !profiler.kvaddr[SCORE_BARON1]
		|| !profiler.kvaddr[SCORE_BARON2]) {
		retval = -EAGAIN;
		goto out;
	}

	retval = read_structs(buffer, size / PROF_STRUCT_SIZE);
out:
	spin_unlock(&profiler.slock);
	score_leave();

	return retval;
}

static struct bin_attribute profiler_attribute = {
	.attr = {
		.name = "profiler",
		.mode = 0440
	},
	.read = score_profiler_read,
};

static struct kobj_attribute profiler_mask_attribute =
	__ATTR(profiler_mask, 0660,
		score_profiler_mask_show,
		score_profiler_mask_store
	);

void score_profiler_init(void)
{
	int id;

	spin_lock(&profiler.slock);
	/* allow any events */
	__set_mask(0xFFFFFFFF);
	for (id = SCORE_TS; id < SCORE_CORE_NUM; ++id) {
		__set_tail(0, id);
		__set_head(0, id);
	}
	spin_unlock(&profiler.slock);
}

int score_profiler_open(void)
{
	struct score_system *system;

	score_enter();
	system = profiler.system;

	profiler.kvaddr[SCORE_TS] =
		score_mmu_get_profiler_kvaddr(&system->mmu, SCORE_TS);
	profiler.kvaddr[SCORE_BARON1] =
		score_mmu_get_profiler_kvaddr(&system->mmu, SCORE_BARON1);
	profiler.kvaddr[SCORE_BARON2] =
		score_mmu_get_profiler_kvaddr(&system->mmu, SCORE_BARON2);

	score_leave();
	return 0;
}

void score_profiler_close(void)
{
	int id;
	int head;
	int tail;

	score_enter();

	/* lock for rare case when profiler is still reading */
	spin_lock(&profiler.slock);
	for (id = SCORE_TS; id < SCORE_CORE_NUM; ++id) {
		profiler.kvaddr[id] = NULL;

		head = __get_head(id);
		tail = __get_tail(id);
		if (head != tail)
			score_warn("profiler data is remained ([%d] %d/%d)\n",
					id, head, tail);
	}
	spin_unlock(&profiler.slock);

	score_leave();
}

int score_profiler_probe(struct score_device *device,
			const struct attribute_group *grp)
{
	int ret;
	struct kobject *kobj;
	struct kernfs_node *parent;

	score_enter();
	profiler.system = &device->system;
	kobj = &device->dev->kobj;
	spin_lock_init(&profiler.slock);

	parent = kernfs_find_and_get(kobj->sd, grp->name);
	if (!parent) {
		score_err("Failed to find and get kernfs\n");
		ret = -ENOENT;
		goto p_err;
	}

	profiler.kobj.sd = parent;
	ret = sysfs_create_bin_file(&profiler.kobj, &profiler_attribute);
	if (ret) {
		score_err("Failed to create bin for profiler (%d)\n", ret);
		goto p_err_create_bin;
	}

	ret = sysfs_create_file(&profiler.kobj,
			&profiler_mask_attribute.attr);
	if (ret) {
		score_err("Failed to create mask attribute for profiler (%d)\n",
			ret);
		goto p_err_create_mask;
	}

	profiler.kvaddr[SCORE_TS] = NULL;
	profiler.kvaddr[SCORE_BARON1] = NULL;
	profiler.kvaddr[SCORE_BARON2] = NULL;

	profiler.prof_count[SCORE_TS] = PROF_STRUCT_TS_COUNT;
	profiler.prof_count[SCORE_BARON1] = PROF_STRUCT_BR_COUNT;
	profiler.prof_count[SCORE_BARON2] = PROF_STRUCT_BR_COUNT;

	score_leave();
	return ret;
p_err_create_mask:
	sysfs_remove_bin_file(&profiler.kobj, &profiler_attribute);
p_err_create_bin:
	kernfs_put(profiler.kobj.sd);
p_err:
	return ret;
}

void score_profiler_remove(void)
{
	score_enter();

	sysfs_remove_file(&profiler.kobj, &profiler_mask_attribute.attr);
	sysfs_remove_bin_file(&profiler.kobj, &profiler_attribute);
	kernfs_put(profiler.kobj.sd);

	score_leave();
}
