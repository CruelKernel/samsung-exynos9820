/*
 * Samsung Exynos SoC series SCore driver
 *
 * Poser manager module for controlling power of SCore
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_EXYNOS_PM_H__
#define __SCORE_EXYNOS_PM_H__

#include <linux/device.h>
#include <linux/pm_runtime.h>
#include <linux/pm_qos.h>

enum score_pm_qos_table {
	SCORE_PM_QOS_L0,
	SCORE_PM_QOS_L1,
	SCORE_PM_QOS_L2,
	SCORE_PM_QOS_L3,
	SCORE_PM_QOS_L4
};

struct score_device;

/**
 * struct score_pm - Data about power information of SCore
 * @device: object about score_device structure
 * @qos_req: object about pm_qos_request structure
 * @lock: mutex to control using pm_qos add, update or remove
 * @qos_count: count of qos level
 * @qos_table: table of qos level
 * @default_qos: default value of pm_qos level
 * @resume_qos: backup value of pm_qos level for resume
 * @current_qos: current value of pm_qos level
 */
struct score_pm {
	struct score_device		*device;
	struct pm_qos_request		qos_req;
	struct mutex			lock;
#if defined(CONFIG_PM_DEVFREQ)
	int				qos_count;
	unsigned int			*qos_table;
	int				default_qos;
	int				resume_qos;
	int				current_qos;
#endif
};

static inline int score_pm_qos_set_default(struct score_pm *pm, int default_qos)
{
	return 0;
}

#if defined(CONFIG_PM_DEVFREQ)
int score_pm_qos_update(struct score_pm *pm, int request_qos);
void score_pm_qos_update_min(struct score_pm *pm);
void score_pm_qos_update_max(struct score_pm *pm);
void score_pm_qos_suspend(struct score_pm *pm);
void score_pm_qos_resume(struct score_pm *pm);
int score_pm_qos_get_info(struct score_pm *pm, int *count,
		int *min_qos, int *max_qos, int *default_qos, int *current_qos);
#else
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
#endif
int score_pm_qos_active(struct score_pm *pm);

int score_pm_open(struct score_pm *pm);
void score_pm_close(struct score_pm *pm);
int score_pm_probe(struct score_device *device);
void score_pm_remove(struct score_pm *pm);

#endif
