/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/of.h>
#include "score_log.h"
#include "score_device.h"
#include "score_pm.h"

#if defined(CONFIG_PM_DEVFREQ)
static int __score_pm_qos_check_valid(struct score_pm *pm, int qos)
{
	score_check();
	if ((qos >= pm->qos_count) || (qos < 0)) {
		score_err("pm_qos level(%d) is invalid (L0 ~ L%d)\n",
				qos, pm->qos_count - 1);
		return -EINVAL;
	} else {
		return 0;
	}
}

static int __score_pm_qos_add(struct score_pm *pm)
{
	int ret;

	score_enter();
	ret = __score_pm_qos_check_valid(pm, pm->default_qos);
	if (ret)
		goto p_err;

	mutex_lock(&pm->lock);
	if (!pm_qos_request_active(&pm->qos_req)) {
		pm_qos_add_request(&pm->qos_req, PM_QOS_SCORE_THROUGHPUT,
				pm->qos_table[pm->default_qos]);
		pm->current_qos = pm->default_qos;
		score_info("The power of device is on(L%d)\n",
				pm->current_qos);
	}
	mutex_unlock(&pm->lock);
	score_leave();
	return 0;
p_err:
	return ret;
}

static void __score_pm_qos_remove(struct score_pm *pm)
{
	score_enter();
	mutex_lock(&pm->lock);
	if (pm_qos_request_active(&pm->qos_req)) {
		pm_qos_remove_request(&pm->qos_req);
		pm->current_qos = -1;
		score_info("The power of device is off\n");
	}
	mutex_unlock(&pm->lock);
	score_leave();
}

int score_pm_qos_set_default(struct score_pm *pm, int default_qos)
{
	int ret;

	score_enter();
	ret = __score_pm_qos_check_valid(pm, default_qos);
	if (ret)
		goto p_err;

	mutex_lock(&pm->lock);
	pm->default_qos = default_qos;
	score_info("default level is set as L%d\n", pm->default_qos);
	mutex_unlock(&pm->lock);
	score_leave();
	return 0;
p_err:
	return ret;
}

int score_pm_qos_update(struct score_pm *pm, int request_qos)
{
	int ret;

	score_enter();
	ret = __score_pm_qos_check_valid(pm, request_qos);
	if (ret)
		goto p_err;

	mutex_lock(&pm->lock);
	if (pm_qos_request_active(&pm->qos_req) &&
		pm->current_qos != request_qos) {
		pm_qos_update_request(&pm->qos_req,
				pm->qos_table[request_qos]);
		score_info("DVFS level is changed from L%d to L%d\n",
				pm->current_qos, request_qos);
		pm->current_qos = request_qos;
	}
	mutex_unlock(&pm->lock);
	score_leave();
	return 0;
p_err:
	return ret;
}

void score_pm_qos_update_min(struct score_pm *pm)
{
	score_enter();
	score_pm_qos_update(pm, pm->qos_count - 1);
	score_leave();
}

void score_pm_qos_update_max(struct score_pm *pm)
{
	score_enter();
	score_pm_qos_update(pm, SCORE_PM_QOS_L0);
	score_leave();
}

void score_pm_qos_suspend(struct score_pm *pm)
{
	score_enter();
	pm->resume_qos = pm->current_qos;
	score_pm_qos_update_min(pm);
	score_leave();
}

void score_pm_qos_resume(struct score_pm *pm)
{
	score_enter();
	score_pm_qos_update(pm, pm->resume_qos);
	pm->resume_qos = -1;
	score_leave();
}

int score_pm_qos_get_info(struct score_pm *pm, int *count,
		int *min_qos, int *max_qos, int *default_qos, int *current_qos)
{
	score_enter();
	*count = pm->qos_count;
	*min_qos = pm->qos_count - 1;
	*max_qos = 0;
	*default_qos = pm->default_qos;
	*current_qos = pm->current_qos;
	score_leave();
	return 0;
}
#endif

int score_pm_qos_active(struct score_pm *pm)
{
	int active = 1;

	score_enter();
#if defined(CONFIG_PM_DEVFREQ)
	if (!pm_qos_request_active(&pm->qos_req))
		active = 0;
#else
	if (!atomic_read(&pm->device->open_count))
		active = 0;
#endif /* CONFIG_PM_DEVFREQ */
	score_leave();
	return active;
}

int score_pm_open(struct score_pm *pm)
{
	int ret = 0;

	score_enter();
#if defined(CONFIG_PM_DEVFREQ)
	ret = __score_pm_qos_add(pm);
#endif
	score_leave();
	return ret;
}

void score_pm_close(struct score_pm *pm)
{
	score_enter();
#if defined(CONFIG_PM_DEVFREQ)
	__score_pm_qos_remove(pm);
#endif
	score_leave();
}

#if defined(CONFIG_PM_DEVFREQ)
static unsigned int qos_table[] = {
	666000,
	534000,
	467000,
	400000,
	200000,
	25000
};

static int __score_pm_qos_init(struct score_pm *pm)
{
	int ret = 0;

	score_enter();
	pm->qos_count = sizeof(qos_table) / sizeof(unsigned int);
	pm->qos_table = qos_table;
	pm->default_qos = -1;
	pm->resume_qos = -1;
	pm->current_qos = -1;

	score_leave();
	return ret;
}
#endif

int score_pm_probe(struct score_device *device)
{
	int ret = 0;
	struct score_pm *pm;

	score_enter();
	pm = &device->pm;

	pm->device = device;
#if defined(CONFIG_PM)
	pm_runtime_enable(device->dev);
#endif

#if defined(CONFIG_PM_DEVFREQ)
	ret = __score_pm_qos_init(pm);
#endif
	if (ret) {
		score_err("Failed to initialize qos table\n");
		goto p_err_parse;
	}
	mutex_init(&pm->lock);

	score_leave();
	return ret;
p_err_parse:
#if defined(CONFIG_PM)
	pm_runtime_disable(device->dev);
#endif
	return ret;
}

void score_pm_remove(struct score_pm *pm)
{
	score_enter();
	mutex_destroy(&pm->lock);

#if defined(CONFIG_PM)
	pm_runtime_disable(pm->device->dev);
#endif
	score_leave();
}
