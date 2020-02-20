/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Debug-SnapShot: Debug Framework for Ramdump based debugging method
 * The original code is Exynos-Snapshot for Exynos SoC
 *
 * Author: Hosung Kim <hosung0.kim@samsung.com>
 * Author: Changki Kim <changki.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef EXYNOS_DEBUG_H
#define EXYNOS_DEBUG_H

#ifdef CONFIG_S3C2410_WATCHDOG
extern int s3c2410wdt_set_emergency_stop(int index);
#ifdef CONFIG_SEC_DEBUG
extern int __s3c2410wdt_set_emergency_reset(unsigned int timeout_cnt, int index, unsigned long addr);
#endif
extern int s3c2410wdt_set_emergency_reset(unsigned int timeout, int index);
extern int s3c2410wdt_keepalive_emergency(bool reset, int index);
extern void s3c2410wdt_reset_confirm(unsigned long mtime, int index);
#else
#define s3c2410wdt_set_emergency_stop(a) 	(-1)
#define s3c2410wdt_set_emergency_reset(a, b)	do { } while(0)
#define s3c2410wdt_keepalive_emergency(a, b)	do { } while(0)
#define s3c2410wdt_reset_confirm(a, b)		do { } while(0)
#endif

#endif


