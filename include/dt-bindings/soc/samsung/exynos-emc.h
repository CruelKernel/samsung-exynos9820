/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS - CPU HOTPLUG Governor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_HOTPLUG_GOVERNOR
#define __EXYNOS_HOTPLUG_GOVERONR __FILE__

/* Role of domain with cpu boosting system */
#define	NOTHING			(0x0)
#define	TRIGGER			(0x1 << 0)
#define	BOOSTER			(0x1 << 1)
#define	TRIGGER_AND_BOOSTER	(TRIGGER | BOOSTER)

#endif /* __EXYNOS_HP_GOV__ */
