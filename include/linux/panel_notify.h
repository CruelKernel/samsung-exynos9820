/*
 * linux/drivers/video/fbdev/exynos/panel/panel_notify.h
 *
 * Header file for Samsung Common LCD Driver.
 *
 * Copyright (c) 2018 Samsung Electronics
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PANEL_NOTIFY_H__
#define __PANEL_NOTIFY_H__

enum panel_notifier_event_t {
	PANEL_EVENT_LCD_CHANGED,
	PANEL_EVENT_BL_CHANGED,
};

struct panel_bl_event_data {
	int brightness;
	int aor_ratio;
};

#ifdef CONFIG_PANEL_NOTIFY
extern int panel_notifier_register(struct notifier_block *nb);
extern int panel_notifier_unregister(struct notifier_block *nb);
extern int panel_notifier_call_chain(unsigned long val, void *v);
#else
static inline int panel_notifier_register(struct notifier_block *nb)
{
	return 0;
};

static inline int panel_notifier_unregister(struct notifier_block *nb)
{
	return 0;
};

static inline int panel_notifier_call_chain(unsigned long val, void *v)
{
	return 0;
};
#endif /* CONFIG_PANEL_NOTIFY */
#endif /* __PANEL_NOTIFY_H__ */
