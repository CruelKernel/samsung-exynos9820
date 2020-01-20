/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "score_log.h"
#include "score_device.h"
#include "score_pm.h"

int score_pm_qos_active(struct score_pm *pm)
{
	int active = 1;

	score_enter();
	if (!atomic_read(&pm->device->open_count))
		active = 0;
	score_leave();
	return active;
}

int score_pm_open(struct score_pm *pm)
{
	score_enter();
	score_leave();
	return 0;
}

void score_pm_close(struct score_pm *pm)
{
	score_enter();
	score_leave();
}

int score_pm_probe(struct score_device *device)
{
	struct score_pm *pm;

	score_enter();
	pm = &device->pm;

	pm->device = device;
#if defined(CONFIG_PM)
	pm_runtime_enable(device->dev);
#endif
	mutex_init(&pm->lock);
	score_leave();
	return 0;
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
