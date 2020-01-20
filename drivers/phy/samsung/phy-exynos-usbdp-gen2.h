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

#ifndef PHY_EXYNOS_USBDP_GEN2_H_
#define PHY_EXYNOS_USBDP_GEN2_H_

extern void phy_exynos_usbdp_g2_enable(struct exynos_usbphy_info *);
extern int phy_exynos_usbdp_g2_check_pll_lock(struct exynos_usbphy_info *info);
extern void phy_exynos_usbdp_g2_disable(struct exynos_usbphy_info *);
extern void phy_exynos_usbdp_g2_tune_each(struct exynos_usbphy_info *, char *, int);
extern void phy_exynos_usbdp_g2_run_lane(struct exynos_usbphy_info *info);
extern int phy_exynos_usbdp_g2_rx_cdr_pll_lock(struct exynos_usbphy_info *info);
extern void phy_exynos_g2_usbdp_tune(struct exynos_usbphy_info *info);
extern void phy_exynos_usbdp_add_tune(struct exynos_usbphy_info *info,
	struct exynos_usb_tune_param *add_tune);
#endif /* PHY_EXYNOS_USBDP__G2_H_ */
