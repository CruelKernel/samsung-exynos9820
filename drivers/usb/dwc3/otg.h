/**
 * otg.c - DesignWare USB3 DRD Controller OTG
 *
 * Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Authors: Ido Shayevitz <idos@codeaurora.org>
 *	    Anton Tikhomirov <av.tikhomirov@samsung.com>
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

#ifndef __LINUX_USB_DWC3_OTG_H
#define __LINUX_USB_DWC3_OTG_H
#include <linux/wakelock.h>
#include <linux/usb/otg-fsm.h>
#include <linux/pm_qos.h>
#ifdef CONFIG_SND_EXYNOS_USB_AUDIO
#include <linux/usb/exynos_usb_audio.h>
#endif
#include "dwc3-exynos.h"

struct dwc3_ext_otg_ops {
	int	(*setup)(struct device *dev, struct otg_fsm *fsm);
	void	(*exit)(struct device *dev);
	int	(*start) (struct device *dev);
	void	(*stop)(struct device *dev);
};

/**
 * struct dwc3_otg: OTG driver data. Shared by HCD and DCD.
 * @otg: USB OTG Transceiver structure.
 * @fsm: OTG Final State Machine.
 * @dwc: pointer to our controller context structure.
 * @irq: IRQ number assigned for HSUSB controller.
 * @regs: ioremapped register base address.
 * @wakelock: prevents the system from entering suspend while
 *		host or peripheral mode is active.
 * @vbus_reg: Vbus regulator.
 * @ready: is one when OTG is ready for operation.
 * @ext_otg_ops: external OTG engine ops.
 */
struct dwc3_otg {
	struct usb_otg          otg;
	struct otg_fsm		fsm;
	struct dwc3             *dwc;
	int                     irq;
	void __iomem            *regs;
	struct wake_lock	wakelock;

	unsigned		ready:1;
	int			otg_connection;

	struct regulator	*vbus_reg;
	int			*ldo_num;
	int			ldos;

	struct pm_qos_request	pm_qos_int_req;
	int			pm_qos_int_val;

	struct dwc3_ext_otg_ops *ext_otg_ops;
	
	int			dp_use_informed;
	
	struct notifier_block	pm_nb;
	struct completion	resume_cmpl;
	int			dwc3_suspended;
	struct			mutex lock;
	u32			combo_phy_control;
};

static inline int dwc3_ext_otg_setup(struct dwc3_otg *dotg)
{
	struct device *dev = dotg->dwc->dev->parent;

	if (!dotg->ext_otg_ops->setup)
		return -EOPNOTSUPP;
	return dotg->ext_otg_ops->setup(dev, &dotg->fsm);
}

static inline int dwc3_ext_otg_exit(struct dwc3_otg *dotg)
{
	struct device *dev = dotg->dwc->dev->parent;

	if (!dotg->ext_otg_ops->exit)
		return -EOPNOTSUPP;
	dotg->ext_otg_ops->exit(dev);
	return 0;
}

static inline int dwc3_ext_otg_start(struct dwc3_otg *dotg)
{
	struct device *dev = dotg->dwc->dev->parent;

	if (!dotg->ext_otg_ops->start)
		return -EOPNOTSUPP;
	return dotg->ext_otg_ops->start(dev);
}

static inline int dwc3_ext_otg_stop(struct dwc3_otg *dotg)
{
	struct device *dev = dotg->dwc->dev->parent;

	if (!dotg->ext_otg_ops->stop)
		return -EOPNOTSUPP;
	dotg->ext_otg_ops->stop(dev);
	return 0;
}

bool dwc3_exynos_rsw_available(struct device *dev);
int dwc3_exynos_rsw_setup(struct device *dev, struct otg_fsm *fsm);
void dwc3_exynos_rsw_exit(struct device *dev);
int dwc3_exynos_rsw_start(struct device *dev);
void dwc3_exynos_rsw_stop(struct device *dev);
extern int xhci_portsc_set(int on);
#if defined(CONFIG_USB_PORT_POWER_OPTIMIZATION)
extern int xhci_port_power_set(u32 on, u32 prt);
extern int is_otg_only;
#endif
#ifdef CONFIG_SND_EXYNOS_USB_AUDIO
extern struct exynos_usb_audio *usb_audio;
#endif

extern void __iomem *phycon_base_addr;

#endif /* __LINUX_USB_DWC3_OTG_H */
