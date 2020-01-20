/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * Author: Sung-Hyun Na <sunghyun.na@samsung.com>
 *
 * Chip Abstraction Layer for USB PHY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef PHY_EXYNOS_USBDP_H_
#define PHY_EXYNOS_USBDP_H_


extern void phy_exynos_usbdp_enable(struct exynos_usbphy_info *);
extern void phy_exynos_usbdp_ilbk(struct exynos_usbphy_info *info);
extern void phy_exynos_usbdp_pcs_reset(struct exynos_usbphy_info *info);
extern int phy_exynos_usbdp_check_pll_lock(struct exynos_usbphy_info *info);
extern void phy_exynos_usbdp_disable(struct exynos_usbphy_info *);
extern void phy_exynos_usbdp_tune_each(struct exynos_usbphy_info *, char *, int);
extern void phy_exynos_usbdp_tune(struct exynos_usbphy_info *info);
extern void exynos_usbdrd_request_phy_isol(void);
extern int exynos_usbdrd_inform_dp_use(int use, int lane_cnt);

#endif /* PHY_EXYNOS_USBDP_H_ */
