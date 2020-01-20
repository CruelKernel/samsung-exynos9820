/*
 *  linux/drivers/devfreq/governor_simpleondemand.c
 *
 *  Copyright (C) 2011 Samsung Electronics
 *	MyungJoo Ham <myungjoo.ham@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/devfreq.h>
#include <linux/math64.h>
#include <linux/pm_qos.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "governor.h"

static int devfreq_simple_ondemand_notifier(struct notifier_block *nb, unsigned long val,
						void *v)
{
	struct devfreq_notifier_block *devfreq_nb;

	devfreq_nb = container_of(nb, struct devfreq_notifier_block, nb);

	mutex_lock(&devfreq_nb->df->lock);
	update_devfreq(devfreq_nb->df);
	mutex_unlock(&devfreq_nb->df->lock);

	return NOTIFY_OK;
}

/* Default constants for DevFreq-Simple-Ondemand (DFSO) */
#define DFSO_UPTHRESHOLD	(90)
#define DFSO_DOWNDIFFERENCTIAL	(5)
#define DFSO_WEIGHT		(100)
static int devfreq_simple_ondemand_func(struct devfreq *df,
					unsigned long *freq)
{
	int err;
	struct devfreq_dev_status *stat;
	unsigned long long a, b;
	unsigned int dfso_upthreshold = DFSO_UPTHRESHOLD;
	unsigned int dfso_downdifferential = DFSO_DOWNDIFFERENCTIAL;
	unsigned int dfso_multiplication_weight = DFSO_WEIGHT;
	struct devfreq_simple_ondemand_data *data = df->data;
	unsigned long max = (df->max_freq) ? df->max_freq : UINT_MAX;
	unsigned long pm_qos_min = 0;

	if (data && !df->disabled_pm_qos) {
		pm_qos_min = pm_qos_request(data->pm_qos_class);
		if (pm_qos_min >= data->cal_qos_max) {
			*freq = pm_qos_min;
			return 0;
		}
	}


	stat = &df->last_status;

	if (df->profile->get_dev_status) {
		err = devfreq_update_stats(df);
		if (err)
			return err;
	} else {
		*freq = pm_qos_min;
		return 0;
	}

	if (data) {
		if (data->upthreshold)
			dfso_upthreshold = data->upthreshold;
		if (data->downdifferential)
			dfso_downdifferential = data->downdifferential;
		if (data->multiplication_weight)
			dfso_multiplication_weight = data->multiplication_weight;
	}
	if (dfso_upthreshold > 100 ||
	    dfso_upthreshold < dfso_downdifferential)
		return -EINVAL;

	if (data && data->cal_qos_max)
		max = (df->max_freq) ? df->max_freq : 0;

	/* Assume MAX if it is going to be divided by zero */
	if (stat->total_time == 0) {
		if (data && data->cal_qos_max)
			max = max3(max, data->cal_qos_max, pm_qos_min);
		*freq = max;
		return 0;
	}

	/* Prevent overflow */
	if (stat->busy_time >= (1 << 24) || stat->total_time >= (1 << 24)) {
		stat->busy_time >>= 7;
		stat->total_time >>= 7;
	}

	stat->busy_time *= dfso_multiplication_weight;
	stat->busy_time = div64_u64(stat->busy_time, 100);

	/* Set MAX if it's busy enough */
	if (stat->busy_time * 100 >
	    stat->total_time * dfso_upthreshold) {
		if (data && data->cal_qos_max)
			max = max3(max, data->cal_qos_max, pm_qos_min);
		*freq = max;
		return 0;
	}

	/* Set MAX if we do not know the initial frequency */
	if (stat->current_frequency == 0) {
		if (data && data->cal_qos_max)
			max = max3(max, data->cal_qos_max, pm_qos_min);
		*freq = max;
		return 0;
	}

	/* Keep the current frequency */
	if (stat->busy_time * 100 >
	    stat->total_time * (dfso_upthreshold - dfso_downdifferential)) {
		*freq = max(stat->current_frequency, pm_qos_min);
		return 0;
	}

	/* Set the desired frequency based on the load */
	a = stat->busy_time;
	a *= stat->current_frequency;
	b = div64_u64(a, stat->total_time);
	b *= 100;
	b = div64_u64(b, (dfso_upthreshold - dfso_downdifferential / 2));

	if (data && data->cal_qos_max) {
		if (b > data->cal_qos_max)
			b = data->cal_qos_max;
	}

	*freq = (unsigned long) b;

	if (pm_qos_min)
		*freq = max(pm_qos_min, *freq);

	return 0;
}

static int devfreq_simple_ondemand_register_notifier(struct devfreq *df)
{
	int ret;
	struct devfreq_simple_ondemand_data *data = df->data;

	if (!data)
		return -EINVAL;

	data->nb.df = df;
	data->nb.nb.notifier_call = devfreq_simple_ondemand_notifier;

	ret = pm_qos_add_notifier(data->pm_qos_class, &data->nb.nb);
	if (ret < 0)
		goto err;

	return 0;
err:
	kfree((void *)&data->nb.nb);

	return ret;
}

static int devfreq_simple_ondemand_unregister_notifier(struct devfreq *df)
{
	struct devfreq_simple_ondemand_data *data = df->data;

	return pm_qos_remove_notifier(data->pm_qos_class, &data->nb.nb);
}

static int devfreq_simple_ondemand_handler(struct devfreq *devfreq,
				unsigned int event, void *data)
{
	int ret;

	switch (event) {
	case DEVFREQ_GOV_START:
		ret = devfreq_simple_ondemand_register_notifier(devfreq);
		if (ret)
			return ret;
		devfreq_monitor_start(devfreq);
		break;

	case DEVFREQ_GOV_STOP:
		devfreq_monitor_stop(devfreq);
		ret = devfreq_simple_ondemand_unregister_notifier(devfreq);
		if (ret)
			return ret;
		break;

	case DEVFREQ_GOV_INTERVAL:
		devfreq_interval_update(devfreq, (unsigned int *)data);
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

static struct devfreq_governor devfreq_simple_ondemand = {
	.name = "simple_ondemand",
	.get_target_freq = devfreq_simple_ondemand_func,
	.event_handler = devfreq_simple_ondemand_handler,
};

static int __init devfreq_simple_ondemand_init(void)
{
	return devfreq_add_governor(&devfreq_simple_ondemand);
}
subsys_initcall(devfreq_simple_ondemand_init);

static void __exit devfreq_simple_ondemand_exit(void)
{
	int ret;

	ret = devfreq_remove_governor(&devfreq_simple_ondemand);
	if (ret)
		pr_err("%s: failed remove governor %d\n", __func__, ret);

	return;
}
module_exit(devfreq_simple_ondemand_exit);
MODULE_LICENSE("GPL");
