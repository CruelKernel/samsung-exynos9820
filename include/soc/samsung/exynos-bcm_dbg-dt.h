/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __EXYNOS_BCM_DBG_DT_H_
#define __EXYNOS_BCM_DBG_DT_H_

int exynos_bcm_dbg_parse_dt(struct device_node *np,
				struct exynos_bcm_dbg_data *data);
#endif	/* __EXYNOS_BCM_DBG_DT_H_ */
