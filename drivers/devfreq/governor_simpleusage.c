/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/errno.h>
#include <linux/devfreq.h>
#include <linux/math64.h>
#include <linux/pm_qos.h>
#include <linux/slab.h>
#include <linux/module.h>

#include "governor.h"

/* Default constants for DevFreq-Simple-Ondemand (DFSO) */
#define DFSO_UPTHRESHOLD	(50)
#define DFSO_TARGET_PERCENTAGE	(20)
#define DFSO_PROPORTIONAL	(120)
#define DFSO_WEIGHT		(100)

static int devfreq_simple_usage_notifier(struct notifier_block *nb, unsigned long val, void *data)
{
	struct devfreq_notifier_block *devfreq_nb;
	struct devfreq_simple_usage_data *simple_usage_data;

	devfreq_nb = container_of(nb, struct devfreq_notifier_block, nb);
	simple_usage_data = container_of(devfreq_nb, struct devfreq_simple_usage_data, nb);
	devfreq_nb = &simple_usage_data->nb;

	mutex_lock(&devfreq_nb->df->lock);
	update_devfreq(devfreq_nb->df);
	mutex_unlock(&devfreq_nb->df->lock);

	return NOTIFY_OK;
}

static int devfreq_simple_usage_func(struct devfreq *df, unsigned long *freq)
{
	struct devfreq_dev_status stat;
	int err = df->profile->get_dev_status(df->dev.parent, &stat);
	unsigned long long a, b;
	unsigned int dfso_upthreshold = DFSO_UPTHRESHOLD;
	unsigned int dfso_target_percentage = DFSO_TARGET_PERCENTAGE;
	unsigned int dfso_proportional = DFSO_PROPORTIONAL;
	unsigned int dfso_multiplication_weight = DFSO_WEIGHT;
	struct devfreq_simple_usage_data *data = df->data;
	unsigned long max = (df->max_freq) ? df->max_freq : 0;
	unsigned long pm_qos_min;

	if (!data)
		return -EINVAL;

	if (!df->disabled_pm_qos)
		pm_qos_min = pm_qos_request(data->pm_qos_class);

	if (err)
		return err;

	if (data->upthreshold)
		dfso_upthreshold = data->upthreshold;
	if (data->target_percentage)
		dfso_target_percentage = data->target_percentage;
	if (data->proportional)
		dfso_proportional = data->proportional;
	if (data->multiplication_weight)
		dfso_multiplication_weight = data->multiplication_weight;

	a = stat.busy_time * dfso_multiplication_weight;
	a = div64_u64(a, 100);
	a = a * dfso_proportional;
	b = div64_u64(a, stat.total_time);

	/* If percentage is larger than upthreshold, set with max freq */
	if (b >= data->upthreshold) {
		max = max(data->cal_qos_max, pm_qos_min);
		*freq = max;

		if (*freq > df->max_freq)
			*freq = df->max_freq;

		return 0;
	}

	b *= stat.current_frequency;

	a = div64_u64(b, dfso_target_percentage);

	if (a > data->cal_qos_max)
		a = data->cal_qos_max;

	*freq = (unsigned long) a;

	if (pm_qos_min && *freq < pm_qos_min)
		*freq = pm_qos_min;

	return 0;
}

static int devfreq_simple_usage_register_notifier(struct devfreq *df)
{
	int ret;
	struct devfreq_simple_usage_data *data = df->data;

	if (!data)
		return -EINVAL;

	data->nb.df = df;
	data->nb.nb.notifier_call = devfreq_simple_usage_notifier;

	ret = pm_qos_add_notifier(data->pm_qos_class, &data->nb.nb);
	if (ret < 0)
		goto err;

	return 0;
err:
	kfree((void *)&data->nb.nb);

	return ret;
}

static int devfreq_simple_usage_unregister_notifier(struct devfreq *df)
{
	struct devfreq_simple_usage_data *data = df->data;

	return pm_qos_remove_notifier(data->pm_qos_class, &data->nb.nb);
}

static int devfreq_simple_usage_handler(struct devfreq *devfreq,
				unsigned int event, void *data)
{
	int ret;

	switch (event) {
	case DEVFREQ_GOV_START:
		ret = devfreq_simple_usage_register_notifier(devfreq);
		if (ret)
			return ret;
		devfreq_monitor_start(devfreq);
		break;

	case DEVFREQ_GOV_STOP:
		devfreq_monitor_stop(devfreq);
		ret = devfreq_simple_usage_unregister_notifier(devfreq);
		if (ret)
			return ret;
		break;

	case DEVFREQ_GOV_INTERVAL:
		devfreq_interval_update(devfreq, (unsigned int*)data);
		break;

	case DEVFREQ_GOV_SUSPEND:
		devfreq_monitor_suspend(devfreq);
		break;

	case DEVFREQ_GOV_RESUME:
		devfreq_monitor_resume(devfreq);
		break;

	default:
		break;
	}

	return 0;
}

static struct devfreq_governor devfreq_simple_usage = {
	.name = "simple_usage",
	.get_target_freq = devfreq_simple_usage_func,
	.event_handler = devfreq_simple_usage_handler,
};

static int __init devfreq_simple_usage_init(void)
{
	return devfreq_add_governor(&devfreq_simple_usage);
}
subsys_initcall(devfreq_simple_usage_init);

static void __exit devfreq_simple_usage_exit(void)
{
	int ret;

	ret = devfreq_remove_governor(&devfreq_simple_usage);
	if (ret)
		pr_err("%s: failed remove governor %d\n", __func__, ret);

	return;
}
module_exit(devfreq_simple_usage_exit);
MODULE_LICENSE("GPL");
