/*
 * drivers/media/platform/exynos/mfc/mfc_debugfs.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_DEBUGFS_H
#define __MFC_DEBUGFS_H __FILE__

#include "mfc_common.h"

extern struct mfc_dev *g_mfc_dev;

void mfc_init_debugfs(struct mfc_dev *dev);

#endif /* __MFC_DEBUGFS_H */
