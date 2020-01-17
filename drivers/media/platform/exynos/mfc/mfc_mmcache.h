/*
 * drivers/media/platform/exynos/mfc/mfc_mmcache.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_MMCACHE_H
#define __MFC_MMCACHE_H __FILE__

#include "mfc_common.h"

#define MMCACHE_GLOBAL_CTRL		0x0000
#define MMCACHE_INVALIDATE_STATUS	0x0008
#define MMCACHE_WAY0_CTRL		0x0010
#define MMCACHE_MASTER_GRP_CTRL2	0x0028
#define MMCACHE_MASTER_GRP0_RPATH0	0x0040
#define MMCACHE_CG_CONTROL		0x0400
#define MMCACHE_INVALIDATE		0x0114

#define MMCACHE_GLOBAL_CTRL_VALUE	0x2
#define MMCACHE_CG_CONTROL_VALUE	0x7FF
#define MMCACHE_INVALIDATE_VALUE	0x41

#define MMCACHE_GROUP2			0x2

/* Need HW lock to call this function */

void mfc_mmcache_enable(struct mfc_dev *dev);
void mfc_mmcache_disable(struct mfc_dev *dev);

void mfc_mmcache_dump_info(struct mfc_dev *dev);

/* Need HW lock to call this function */
void mfc_invalidate_mmcache(struct mfc_dev *dev);

#endif /* __MFC_MMCACHE_H */
