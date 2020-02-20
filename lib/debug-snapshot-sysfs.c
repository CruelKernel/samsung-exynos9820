/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Exynos-SnapShot debugging framework for Exynos SoC
 *
 * Author: Hosung Kim <Hosung0.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/ktime.h>
#include <linux/kallsyms.h>
#include <linux/platform_device.h>
#include <linux/clk-provider.h>
#include <linux/pstore_ram.h>
#include <linux/sched/clock.h>
#include <linux/ftrace.h>

#include "debug-snapshot-local.h"
#include <asm/irq.h>
#include <asm/traps.h>
#include <asm/hardirq.h>
#include <asm/stacktrace.h>
#include <linux/debug-snapshot.h>
#include <linux/kernel_stat.h>
#include <linux/irqnr.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>

/*
 *  sysfs implementation for debug-snapshot
 *  you can access the sysfs of debug-snapshot to /sys/devices/system/debug_snapshot
 *  path.
 */
static struct bus_type dss_subsys = {
	.name = "debug-snapshot",
	.dev_name = "debug-snapshot",
};

extern int dss_irqlog_exlist[DSS_EX_MAX_NUM];
extern int dss_irqexit_exlist[DSS_EX_MAX_NUM];
extern unsigned int dss_irqexit_threshold;

static ssize_t dss_enable_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	struct dbg_snapshot_item *item;
	unsigned long i;
	ssize_t n = 0;

	/*  item  */
	for (i = 0; i < dss_desc.log_cnt; i++) {
		item = &dss_items[i];
		n += scnprintf(buf + n, 24, "%-12s : %sable\n",
			item->name, item->entry.enabled ? "en" : "dis");
	}

	/*  base  */
	n += scnprintf(buf + n, 24, "%-12s : %sable\n",
			"base", dss_base.enabled ? "en" : "dis");

	return n;
}

static ssize_t dss_enable_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	int en;
	char *name;

	name = (char *)kstrndup(buf, count, GFP_KERNEL);
	if (!name)
		return count;

	en = dbg_snapshot_get_enable(name);
	if (en == -1)
		pr_info("echo name > enabled\n");
	else {
		if (en)
			dbg_snapshot_set_enable(name, false);
		else
			dbg_snapshot_set_enable(name, true);
	}

	kfree(name);

	return count;
}

static ssize_t dss_callstack_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	ssize_t n = 0;

	n = scnprintf(buf, 24, "callstack depth : %d\n", dss_desc.callstack);

	return n;
}

static ssize_t dss_callstack_store(struct kobject *kobj, struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long callstack;

	callstack = simple_strtoul(buf, NULL, 0);
	pr_info("callstack depth(min 1, max 4) : %lu\n", callstack);

	if (callstack < 5 && callstack > 0) {
		dss_desc.callstack = (unsigned int)callstack;
		pr_info("success inserting %lu to callstack value\n", callstack);
	}
	return count;
}

static ssize_t dss_irqlog_exlist_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	unsigned long i;
	ssize_t n = 0;

	n = scnprintf(buf, 24, "excluded irq number\n");

	for (i = 0; i < ARRAY_SIZE(dss_irqlog_exlist); i++) {
		if (dss_irqlog_exlist[i] == 0)
			break;
		n += scnprintf(buf + n, 24, "irq num: %-4d\n", dss_irqlog_exlist[i]);
	}
	return n;
}

static ssize_t dss_irqlog_exlist_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	unsigned long i;
	unsigned long irq;

	irq = simple_strtoul(buf, NULL, 0);
	pr_info("irq number : %lu\n", irq);

	for (i = 0; i < ARRAY_SIZE(dss_irqlog_exlist); i++) {
		if (dss_irqlog_exlist[i] == 0)
			break;
	}

	if (i == ARRAY_SIZE(dss_irqlog_exlist)) {
		pr_err("list is full\n");
		return count;
	}

	if (irq != 0) {
		dss_irqlog_exlist[i] = irq;
		pr_info("success inserting %lu to list\n", irq);
	}
	return count;
}

#ifdef CONFIG_DEBUG_SNAPSHOT_IRQ_EXIT
static ssize_t dss_irqexit_exlist_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	unsigned long i;
	ssize_t n = 0;

	n = scnprintf(buf, 36, "Excluded irq number\n");
	for (i = 0; i < ARRAY_SIZE(dss_irqexit_exlist); i++) {
		if (dss_irqexit_exlist[i] == 0)
			break;
		n += scnprintf(buf + n, 24, "IRQ num: %-4d\n", dss_irqexit_exlist[i]);
	}
	return n;
}

static ssize_t dss_irqexit_exlist_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long i;
	unsigned long irq;

	irq = simple_strtoul(buf, NULL, 0);
	pr_info("irq number : %lu\n", irq);

	for (i = 0; i < ARRAY_SIZE(dss_irqexit_exlist); i++) {
		if (dss_irqexit_exlist[i] == 0)
			break;
	}

	if (i == ARRAY_SIZE(dss_irqexit_exlist)) {
		pr_err("list is full\n");
		return count;
	}

	if (irq != 0) {
		dss_irqexit_exlist[i] = irq;
		pr_info("success inserting %lu to list\n", irq);
	}
	return count;
}

static ssize_t dss_irqexit_threshold_show(struct kobject *kobj,
					struct kobj_attribute *attr, char *buf)
{
	ssize_t n;

	n = scnprintf(buf, 46, "threshold : %12u us\n", dss_irqexit_threshold);
	return n;
}

static ssize_t dss_irqexit_threshold_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long val;

	val = simple_strtoul(buf, NULL, 0);
	pr_info("threshold value : %lu\n", val);

	if (val != 0) {
		dss_irqexit_threshold = (unsigned int)val;
		pr_info("success %lu to threshold\n", val);
	}
	return count;
}
#endif

#ifdef CONFIG_DEBUG_SNAPSHOT_REG
static ssize_t dss_reg_exlist_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	unsigned long i;
	ssize_t n = 0;

	n = scnprintf(buf, 36, "excluded register address\n");
	for (i = 0; i < ARRAY_SIZE(dss_reg_exlist); i++) {
		if (dss_reg_exlist[i].addr == 0)
			break;
		n += scnprintf(buf + n, 40, "register addr: %08zx size: %08zx\n",
				dss_reg_exlist[i].addr, dss_reg_exlist[i].size);
	}
	return n;
}

static ssize_t dss_reg_exlist_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long i;
	size_t addr;

	addr = simple_strtoul(buf, NULL, 0);
	pr_info("register addr: %zx\n", addr);

	for (i = 0; i < ARRAY_SIZE(dss_reg_exlist); i++) {
		if (dss_reg_exlist[i].addr == 0)
			break;
	}
	if (addr != 0) {
		dss_reg_exlist[i].size = SZ_4K;
		dss_reg_exlist[i].addr = addr;
		pr_info("success %zx to threshold\n", (addr));
	}
	return count;
}
#endif

#ifdef CONFIG_SEC_PM_DEBUG
static ssize_t dss_log_work_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	return dss_log_work_print(buf);
}
#endif /* CONFIG_SEC_PM_DEBUG */

static struct kobj_attribute dss_enable_attr =
__ATTR(enabled, 0644, dss_enable_show, dss_enable_store);

static struct kobj_attribute dss_callstack_attr =
__ATTR(callstack, 0644, dss_callstack_show, dss_callstack_store);

static struct kobj_attribute dss_irqlog_attr =
__ATTR(exlist_irqdisabled, 0644, dss_irqlog_exlist_show,
					dss_irqlog_exlist_store);
#ifdef CONFIG_DEBUG_SNAPSHOT_IRQ_EXIT
static struct kobj_attribute dss_irqexit_attr =
__ATTR(exlist_irqexit, 0644, dss_irqexit_exlist_show,
					dss_irqexit_exlist_store);

static struct kobj_attribute dss_irqexit_threshold_attr =
__ATTR(threshold_irqexit, 0644, dss_irqexit_threshold_show,
					dss_irqexit_threshold_store);
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_REG

static struct kobj_attribute dss_reg_attr =
__ATTR(exlist_reg, 0644, dss_reg_exlist_show, dss_reg_exlist_store);
#endif

#ifdef CONFIG_SEC_PM_DEBUG
static struct kobj_attribute dss_log_work_attr =
__ATTR(kworker, 0444, dss_log_work_show, NULL);
#endif /* CONFIG_SEC_PM_DEBUG */

static struct attribute *dss_sysfs_attrs[] = {
	&dss_enable_attr.attr,
	&dss_callstack_attr.attr,
	&dss_irqlog_attr.attr,
#ifdef CONFIG_DEBUG_SNAPSHOT_IRQ_EXIT
	&dss_irqexit_attr.attr,
	&dss_irqexit_threshold_attr.attr,
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_REG
	&dss_reg_attr.attr,
#endif
#ifdef CONFIG_SEC_PM_DEBUG
	&dss_log_work_attr.attr,
#endif /* CONFIG_SEC_PM_DEBUG */
	NULL,
};

static struct attribute_group dss_sysfs_group = {
	.attrs = dss_sysfs_attrs,
};

static const struct attribute_group *dss_sysfs_groups[] = {
	&dss_sysfs_group,
	NULL,
};

static int __init dbg_snapshot_sysfs_init(void)
{
	int ret = 0;

	ret = subsys_system_register(&dss_subsys, dss_sysfs_groups);
	if (ret)
		pr_err("fail to register debug-snapshop subsys\n");

	return ret;
}
late_initcall(dbg_snapshot_sysfs_init);
