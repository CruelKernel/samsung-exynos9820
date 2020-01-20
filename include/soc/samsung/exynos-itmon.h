/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * EXYNOS IPs Traffic Monitor Driver for Samsung EXYNOS SoC
 * By Hosung Kim (hosung0.kim@samsung.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef EXYNOS_ITMON__H
#define EXYNOS_ITMON__H

struct itmon_notifier {
	char *port;			/* The block to which the master IP belongs */
	char *master;			/* The master's name which problem occurred */
	char *dest;			/* The destination which the master tried to access */
	bool read;			/* Transaction Type */
	unsigned long target_addr;	/* The physical address which the master tried to access */
	unsigned int errcode;		/* The error code which the problem occurred */
	bool onoff;			/* Target Block on/off */
	char *pd_name;			/* Target Block power domain name */
};
#ifdef CONFIG_EXYNOS_ITMON
extern void itmon_notifier_chain_register(struct notifier_block *n);
extern void itmon_enable(bool enabled);
extern void itmon_set_errcnt(int cnt);
extern int cal_pd_status(unsigned int id);
#else
static inline void itmon_enable(bool enabled) {}
#define itmon_notifier_chain_register(x)		do { } while (0)
#define itmon_set_errcnt(x)				do { } while (0)
#define itmon_enable(x)					do { } while (0)
#endif

#endif
