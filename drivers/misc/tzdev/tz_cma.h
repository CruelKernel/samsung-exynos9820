/*
 * Copyright (C) 2016 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __TZ_CMA_H__
#define __TZ_CMA_H__

#include <linux/device.h>

#if defined(CONFIG_TZDEV_CMA)

int tzdev_cma_mem_register(void);
void tzdev_cma_mem_init(struct device *dev);
void tzdev_cma_mem_release(struct device *dev);

#else /* CONFIG_TZDEV_CMA */

static inline int tzdev_cma_mem_register(void)
{
	return 0;
}

static inline void tzdev_cma_mem_init(struct device *dev)
{
	(void)dev;

	return;
}

static inline void tzdev_cma_mem_release(struct device *dev)
{
	(void)dev;

	return;
}
#endif /* CONFIG_TZDEV_CMA */

#endif /* __TZ_CMA_H__ */
