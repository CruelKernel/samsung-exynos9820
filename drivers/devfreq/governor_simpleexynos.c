/*
 *  linux/drivers/devfreq/governor_simpleexynos.c
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

static int gov_simple_exynos = 0;

static int devfreq_simple_exynos_notifier(struct notifier_block *nb, unsigned long val,
						void *v)
{
	struct devfreq_notifier_block *devfreq_nb;

	devfreq_nb = container_of(nb, struct devfreq_notifier_block, nb);

	mutex_lock(&devfreq_nb->df->lock);
	update_devfreq(devfreq_nb->df);
	mutex_unlock(&devfreq_nb->df->lock);

	return NOTIFY_OK;
}

/* Default constants for DevFreq-Simple-Exynos(DFE) */
#define DFE_URGENTTHRESHOLD	(65)
#define DFE_UPTHRESHOLD		(60)
#define DFE_DOWNTHRESHOLD	(45)
#define DFE_IDLETHRESHOLD	(30)

static int devfreq_simple_exynos_func(struct devfreq *df,
					unsigned long *freq)
{
	struct devfreq_dev_status stat;
	unsigned int dfe_urgentthreshold = DFE_URGENTTHRESHOLD;
	unsigned int dfe_upthreshold = DFE_UPTHRESHOLD;
	unsigned int dfe_downthreshold = DFE_DOWNTHRESHOLD;
	unsigned int dfe_idlethreshold = DFE_IDLETHRESHOLD;
	struct devfreq_simple_exynos_data *data = df->data;
	unsigned long max = (df->max_freq) ? df->max_freq : UINT_MAX;
	unsigned long pm_qos_min = 0, cal_qos_max = 0;
	unsigned long pm_qos_max = 0;
	unsigned long usage_rate;
	int err;

	if (data) {
		if (!df->disabled_pm_qos) {
			pm_qos_min = pm_qos_request(data->pm_qos_class);
			if (data->pm_qos_class_max)
				pm_qos_max = pm_qos_request(data->pm_qos_class_max);
			if (unlikely(gov_simple_exynos))
				printk("pm_qos: %lu\n", pm_qos_min);
		}

		if (data->urgentthreshold)
			dfe_urgentthreshold = data->urgentthreshold;
		if (data->upthreshold)
			dfe_upthreshold = data->upthreshold;
		if (data->downthreshold)
			dfe_downthreshold = data->downthreshold;
		if (data->idlethreshold)
			dfe_idlethreshold = data->idlethreshold;

		if (data->cal_qos_max) {
			cal_qos_max = data->cal_qos_max;
			max = (df->max_freq) ? df->max_freq : 0;
		} else {
			cal_qos_max = df->max_freq;
		}
	}

	if (df->profile->get_dev_status) {
		err = df->profile->get_dev_status(df->dev.parent, &stat);
		if (err)
			return err;
	} else {
		*freq = pm_qos_min;
		if (pm_qos_max)
			*freq = min(pm_qos_max, *freq);
		return 0;
	}

	/* Assume MAX if it is going to be divided by zero */
	if (stat.total_time == 0) {
		*freq = max3(max, cal_qos_max, pm_qos_min);
		if (pm_qos_max)
			*freq = min(pm_qos_max, *freq);
		return 0;
	}

	/* Set MAX if we do not know the initial frequency */
	if (stat.current_frequency == 0) {
		*freq = max3(max, cal_qos_max, pm_qos_min);
		if (pm_qos_max)
			*freq = min(pm_qos_max, *freq);
		return 0;
	}

	usage_rate = div64_u64(stat.busy_time * 100, stat.total_time);

	/* Set MAX if it's busy enough */
	if (usage_rate > dfe_urgentthreshold)
		*freq = max3(max, cal_qos_max, pm_qos_min);
	else if (usage_rate >= dfe_upthreshold) {
		if (data)
			*freq = data->above_freq;
	} else if (usage_rate > dfe_downthreshold)
		*freq = stat.current_frequency;
	else if (usage_rate > dfe_idlethreshold) {
		if (data)
			*freq = data->below_freq;
	} else
		*freq = 0;

	if (*freq > cal_qos_max)
		*freq = cal_qos_max;

	if (pm_qos_min)
		*freq = max(pm_qos_min, *freq);

	if (pm_qos_max)
		*freq = min(pm_qos_max, *freq);

	if (unlikely(gov_simple_exynos))
		printk("Usage: %lu, freq: %lu, old: %lu\n", usage_rate, *freq, stat.current_frequency);

	return 0;
}

static int devfreq_simple_exynos_register_notifier(struct devfreq *df)
{
	int ret;
	struct devfreq_simple_exynos_data *data = df->data;

	if (!data)
		return -EINVAL;

	data->nb.df = df;
	data->nb.nb.notifier_call = devfreq_simple_exynos_notifier;

	ret = pm_qos_add_notifier(data->pm_qos_class, &data->nb.nb);
	if (ret < 0)
		goto err1;

	if (data->pm_qos_class_max) {
		data->nb_max.df = df;
		data->nb_max.nb.notifier_call = devfreq_simple_exynos_notifier;

		ret = pm_qos_add_notifier(data->pm_qos_class_max, &data->nb_max.nb);
		if (ret < 0) {
			pm_qos_remove_notifier(data->pm_qos_class, &data->nb.nb);
			goto err2;
		}
	}

	return 0;
err2:
	kfree((void *)&data->nb_max.nb);

err1:
	kfree((void *)&data->nb.nb);

	return ret;
}

static int devfreq_simple_exynos_unregister_notifier(struct devfreq *df)
{
	int ret;
	struct devfreq_simple_exynos_data *data = df->data;

	if (!data)
		return -EINVAL;

	if (data->pm_qos_class_max) {
		ret = pm_qos_remove_notifier(data->pm_qos_class_max, &data->nb_max.nb);
		if (ret < 0)
			goto err;
	}

	ret = pm_qos_remove_notifier(data->pm_qos_class, &data->nb.nb);

err:
	return ret;
}

static int devfreq_simple_exynos_handler(struct devfreq *devfreq,
				unsigned int event, void *data)
{
	int ret;

	switch (event) {
	case DEVFREQ_GOV_START:
		ret = devfreq_simple_exynos_register_notifier(devfreq);
		if (ret)
			return ret;
		devfreq_monitor_start(devfreq);
		break;

	case DEVFREQ_GOV_STOP:
		devfreq_monitor_stop(devfreq);
		ret = devfreq_simple_exynos_unregister_notifier(devfreq);
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

static struct devfreq_governor devfreq_simple_exynos = {
	.name = "simple_exynos",
	.get_target_freq = devfreq_simple_exynos_func,
	.event_handler = devfreq_simple_exynos_handler,
};

static int __init devfreq_simple_exynos_init(void)
{
	return devfreq_add_governor(&devfreq_simple_exynos);
}
subsys_initcall(devfreq_simple_exynos_init);

static void __exit devfreq_simple_exynos_exit(void)
{
	int ret;

	ret = devfreq_remove_governor(&devfreq_simple_exynos);
	if (ret)
		pr_err("%s: failed remove governor %d\n", __func__, ret);

	return;
}
module_exit(devfreq_simple_exynos_exit);
MODULE_LICENSE("GPL");
