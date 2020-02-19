/* linux/arm/arm/mach-exynos/include/mach/regs-clock.h
 *
 * Copyright (C) 2013 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * EXYNOS5 - Header file for exynos pm support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_PM_H
#define __EXYNOS_PM_H

#include <linux/kernel.h>
#include <linux/notifier.h>

/*
 * Event codes for PM states
 */
enum exynos_pm_event {
	/* CPU is entering the LPA state */
	LPA_ENTER,

	/* CPU failed to enter the LPA state */
	LPA_ENTER_FAIL,

	/* CPU is exiting the LPA state */
	LPA_EXIT,

	/* CPU is entering the SICD/SICD_AUD state */
	SICD_ENTER,
	SICD_AUD_ENTER,

	/* CPU is exiting the SICD/SICD_AUD state */
	SICD_EXIT,
	SICD_AUD_EXIT,
};

#define EXYNOS_PM_PREFIX	"EXYNOS-PM:"

int register_usb_is_connect(u32 (*func)(void));

#ifdef CONFIG_CPU_IDLE
int exynos_pm_register_notifier(struct notifier_block *nb);
int exynos_pm_unregister_notifier(struct notifier_block *nb);
int exynos_pm_notify(enum exynos_pm_event event);
#else
static inline int exynos_pm_register_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline int exynos_pm_unregister_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline int exynos_pm_notify(enum exynos_pm_event event)
{
	return 0;
}
#endif

#ifdef CONFIG_EXYNOS_FLEXPMU_DBG
extern u32 acpm_get_mifdn_count(void);
extern u32 acpm_get_apsocdn_count(void);
extern u32 acpm_get_early_wakeup_count(void);
extern int acpm_get_mif_request(void);
#else
static inline int acpm_get_mif_request(void)
{
	return 0;
}
static inline u32 acpm_get_mifdn_count(void)
{
	return 0;
}
static inline u32 acpm_get_apsocdn_count(void)
{
	return 0;
}
static inline u32 acpm_get_early_wakeup_count(void)
{
	return 0;
}
#endif

#ifdef CONFIG_USB_DWC3_EXYNOS
extern u32 otg_is_connect(void);
#else
static inline u32 otg_is_connect(void)
{
	return 0;
}
#endif

#if defined(CONFIG_SEC_DEBUG)
enum ids_info {
	tg,
	lg,
	mg,
	bg,
	g3dg,
	mifg,
	lids,
	mids,
	bids,
	gids,
};

extern int asv_ids_information(enum ids_info id);
#endif

#endif /* __EXYNOS_PM_H */
