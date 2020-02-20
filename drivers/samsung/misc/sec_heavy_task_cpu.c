/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
 
#include <linux/cpumask.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/sec_class.h>
#include <linux/sysfs.h>
#include <linux/threads.h>
#include <linux/types.h>

#include "../../../kernel/sched/sched.h"

#define HEAVY_TASK_CPU_LOAD_THRESHOLD 1000

static unsigned int heavy_task_cpu_count;

static ssize_t heavy_task_cpu_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int count = 0;
    unsigned long int task_util;
    unsigned long int cfs_load;
    unsigned long int no_task;
    unsigned long int remaining_load;
    unsigned long int avg_load;
    int cpu;

    for_each_possible_cpu(cpu) {
        struct rq *rq = cpu_rq(cpu);
        struct task_struct *p = rq->curr;

        task_util = (unsigned long int)p->se.avg.util_avg;
        cfs_load = (unsigned long int)rq->cfs.runnable_load_avg;
        no_task = (unsigned long int)rq->cfs.h_nr_running;

        if (task_util > HEAVY_TASK_CPU_LOAD_THRESHOLD) {
            count++;
        } else if (task_util <= HEAVY_TASK_CPU_LOAD_THRESHOLD && no_task > 1) {
            remaining_load = cfs_load - task_util;
            avg_load = remaining_load / (no_task - 1);
            if (avg_load > HEAVY_TASK_CPU_LOAD_THRESHOLD)
                count++;
        }
    }

    heavy_task_cpu_count = count;

    return snprintf(buf, 4, "%d\n", heavy_task_cpu_count);
}

static DEVICE_ATTR(heavy_task_cpu, 0444, heavy_task_cpu_show, NULL);

static struct attribute *heavy_task_cpu_attributes[] = {
    &dev_attr_heavy_task_cpu.attr,
    NULL
};

static const struct attribute_group heavy_task_cpu_attr_group = {
    .attrs = heavy_task_cpu_attributes,
};

int __init heavy_task_cpu_init(void)
{
    int ret = 0;
    struct device *dev;

    dev = sec_device_create(NULL, "sec_heavy_cpu");

    if (IS_ERR(dev)) {
        dev_err(dev, "%s: fail to create sec_dev\n", __func__);
        return PTR_ERR(dev);
    }
    ret = sysfs_create_group(&dev->kobj, &heavy_task_cpu_attr_group);
    if (ret)
        dev_err(dev, "failed to create sysfs group\n");

    return 0;
}
late_initcall(heavy_task_cpu_init);
