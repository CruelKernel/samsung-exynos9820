/* tui/stui_core.h
 *
 * Samsung TUI HW Handler driver.
 *
 * Copyright (c) 2015 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __STUI_CORE_H_
#define __STUI_CORE_H_

#include <linux/fs.h>
#include <linux/types.h>
#include <linux/wakelock.h>

#define STUI_ALIGN_4kB_SZ	0x1000		/*   4kB */
#define STUI_ALIGN_64kB_SZ	0x10000		/*  64kB */
#define STUI_ALIGN_128kB_SZ	0x20000		/* 128kB */
#define STUI_ALIGN_1MB_SZ	0x100000	/*   1MB */
#define STUI_ALIGN_16MB_SZ	0x1000000	/*  16MB */
#define STUI_ALIGN_32MB_SZ	0x2000000	/*  32MB */

#define STUI_ALIGN_UP(size, block) ((((size) + (block) - 1) / (block)) * (block))

#define STUI_DEV_NAME "tuihw"

extern struct wake_lock tui_wakelock;

long stui_process_cmd(struct file *f, unsigned int cmd, unsigned long arg);

#endif /* __STUI_CORE_H_ */
