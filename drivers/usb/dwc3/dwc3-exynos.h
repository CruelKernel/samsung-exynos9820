/**
 * dwc3-exynos.h - Samsung EXYNOS DWC3 Specific Glue layer header
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Anton Tikhomirov <av.tikhomirov@samsung.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LINUX_USB_DWC3_EXYNOS_H
#define __LINUX_USB_DWC3_EXYNOS_H

extern bool dwc3_exynos_rsw_available(struct device *dev);
extern int dwc3_exynos_rsw_setup(struct device *dev, struct otg_fsm *fsm);
extern void dwc3_exynos_rsw_exit(struct device *dev);
extern int dwc3_exynos_rsw_start(struct device *dev);
extern void dwc3_exynos_rsw_stop(struct device *dev);
extern int dwc3_exynos_id_event(struct device *dev, int state);
extern int dwc3_exynos_vbus_event(struct device *dev, bool vbus_active);
extern int dwc3_exynos_start_ldo(struct device *dev, bool on);
extern int dwc3_exynos_phy_enable(int owner, bool on);

#endif /* __LINUX_USB_DWC3_EXYNOS_H */

