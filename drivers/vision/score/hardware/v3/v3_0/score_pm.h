/*
 * Samsung Exynos SoC series SCore driver
 *
 * Poser manager module for controlling power of SCore
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_ZYNQ_PM_H__
#define __SCORE_ZYNQ_PM_H__

#include <linux/device.h>
#include <linux/pm_runtime.h>
#include <linux/pm_qos.h>

struct score_device;

/**
 * struct score_pm - Data about power information of SCore
 * @device: object about score_device structure
 * @lock: mutex to control using pm
 */
struct score_pm {
	struct score_device		*device;
	struct mutex                    lock;
};

static inline int score_pm_qos_set_default(struct score_pm *pm, int default_qos)
{
	return 0;
}
static inline int score_pm_qos_update(struct score_pm *pm, int request_qos)
{
	return 0;
}
static inline void score_pm_qos_update_min(struct score_pm *pm)
{
}
static inline void score_pm_qos_update_max(struct score_pm *pm)
{
}
static inline void score_pm_qos_suspend(struct score_pm *pm)
{
}
static inline void score_pm_qos_resume(struct score_pm *pm)
{
}
static inline int score_pm_qos_get_info(struct score_pm *pm, int *count,
		int *min_qos, int *max_qos, int *default_qos, int *current_qos)
{
	return -EACCES;
}
int score_pm_qos_active(struct score_pm *pm);

int score_pm_open(struct score_pm *pm);
void score_pm_close(struct score_pm *pm);
int score_pm_probe(struct score_device *device);
void score_pm_remove(struct score_pm *pm);

#endif
