/*
 * drivers/media/platform/exynos/mfc/mfc_pm.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_PM_H
#define __MFC_PM_H __FILE__

#include "mfc_common.h"

static inline int mfc_pm_get_pwr_ref_cnt(struct mfc_dev *dev)
{
	return atomic_read(&dev->pm.pwr_ref);
}

static inline int mfc_pm_get_clk_ref_cnt(struct mfc_dev *dev)
{
	return atomic_read(&dev->clk_ref);
}

void mfc_pm_init(struct mfc_dev *dev);
void mfc_pm_final(struct mfc_dev *dev);

int mfc_pm_clock_on(struct mfc_dev *dev);
int mfc_pm_clock_on_with_base(struct mfc_dev *dev,
			enum mfc_buf_usage_type buf_type);
void mfc_pm_clock_off(struct mfc_dev *dev);
int mfc_pm_power_on(struct mfc_dev *dev);
int mfc_pm_power_off(struct mfc_dev *dev);

#endif /* __MFC_PM_H */
