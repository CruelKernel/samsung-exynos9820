/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS - PPMU support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/sched/clock.h>
#include <linux/math64.h>

#include "exynos-ppc.h"

#define PMNC			0x0004
#define CNTENS			0x0008
#define CNTENC			0x000C
#define CNTRESET		0x002C
#define CNTAUTO			0x0030
#define PMCNT			0x0034
#define CCNT			0x0048

#define BIT_CHVALUE		((0x1 << 31) | (0x1 << 0))
#define BIT_REGVALUE		(0x1 << 24)
#define BIT_RESETALL		(0x6)
#define BIT_GLBCNTEN		(0x1)

static void exynos_init_ppc(void __iomem *base)
{
	/* Initialize Q-CH, OP Mode */
	__raw_writel(BIT_REGVALUE | BIT_RESETALL, base + PMNC);

	/* Enable CCNT, PMCNTTx Counting*/
	__raw_writel(BIT_CHVALUE, base + CNTENS);
}

static void exynos_start_ppc(void __iomem *base)
{
	__raw_writel(BIT_REGVALUE | BIT_GLBCNTEN, base + PMNC);
}

static void exynos_stop_ppc(void __iomem *base)
{
	__raw_writel(BIT_REGVALUE, base + PMNC);
}

static void exynos_clear_ppc(void __iomem *base)
{
	__raw_writel(BIT_RESETALL, base + PMNC);
}

static void exynos_reset_ppc(void __iomem *base)
{
	__raw_writel(BIT_REGVALUE | BIT_RESETALL, base + PMNC);
}

static void exynos_read_ppc(struct exynos_devfreq_data *data)
{
	int i;
	struct ppc_data ppc = { 0, };
	int index = 0;

	data->um_data.val_ccnt = 0;
	data->um_data.val_pmcnt = 0;

	for (i = 0; i < data->um_data.um_count; i++) {
		ppc.ccnt = __raw_readl(data->um_data.va_base[i] + CCNT);
		ppc.pmcnt = __raw_readl(data->um_data.va_base[i] + PMCNT);

		if (data->um_data.val_pmcnt < ppc.pmcnt) {
			data->um_data.val_ccnt = ppc.ccnt;
			data->um_data.val_pmcnt = ppc.pmcnt;
			index = i;
		}
	}
}

int exynos_devfreq_um_init(struct exynos_devfreq_data *data)
{
	int i = 0;

	for (i = 0; i < data->um_data.um_count; i++)
		exynos_init_ppc(data->um_data.va_base[i]);

	for (i = 0; i < data->um_data.um_count; i++)
		exynos_start_ppc(data->um_data.va_base[i]);

	return 0;
}

void exynos_devfreq_um_exit(struct exynos_devfreq_data *data)
{
	int i = 0;

	for (i = 0; i < data->um_data.um_count; i++)
		exynos_clear_ppc(data->um_data.va_base[i]);
}

static int exynos_devfreq_get_dev_status(struct device *dev, struct devfreq_dev_status *stat)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	u64 cur_time;
	int i;

	if (data->devfreq_disabled)
		return -EAGAIN;

	cur_time = sched_clock();
	data->last_monitor_period = (cur_time - data->last_monitor_time);
	data->last_monitor_time = cur_time;

	for (i = 0; i < data->um_data.um_count; i++)
		exynos_stop_ppc(data->um_data.va_base[i]);

	/* Read current counter */
	exynos_read_ppc(data);

	stat->current_frequency = data->devfreq->previous_freq;
	stat->busy_time = data->um_data.val_pmcnt;
	stat->total_time = data->um_data.val_ccnt;
	stat->delta_time = data->last_monitor_period;

	data->last_um_usage_rate = div64_u64(stat->busy_time * 100, stat->total_time);

	for (i = 0; i < data->um_data.um_count; i++) {
		exynos_reset_ppc(data->um_data.va_base[i]);
		exynos_start_ppc(data->um_data.va_base[i]);
	}

	return 0;
}

void register_get_dev_status(struct exynos_devfreq_data *data)
{
	data->devfreq_profile.get_dev_status = exynos_devfreq_get_dev_status;
}
