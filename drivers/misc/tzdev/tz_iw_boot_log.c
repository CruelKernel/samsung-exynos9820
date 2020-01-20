/*
 * Copyright (C) 2013-2016 Samsung Electronics, Inc.
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

#include <linux/atomic.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/printk.h>
#include <linux/string.h>

#include "tz_iw_boot_log.h"
#include "tzdev.h"

#define TZ_BOOT_LOG_PREFIX		KERN_DEFAULT "SW> "

static atomic_t tz_iw_boot_log_already_read = ATOMIC_INIT(0);

static void tz_iw_boot_log_print(char *buf, unsigned long nbytes)
{
	unsigned long printed;
	char *p;

	while (nbytes) {
		p = memchr(buf, '\n', nbytes);

		if (p) {
			*p = '\0';
			printed = p - buf + 1;
		} else {
			printed = nbytes;
		}

		printk(TZ_BOOT_LOG_PREFIX "%.*s\n", (int)printed, buf);

		nbytes -= printed;
		buf += printed;
	}
}

void tz_iw_boot_log_read(void)
{
	struct page *pages;
	unsigned int order;
	int nbytes;

	if (atomic_cmpxchg(&tz_iw_boot_log_already_read, 0, 1))
		return;

	order = order_base_2(CONFIG_TZ_BOOT_LOG_PG_CNT);

	pages = alloc_pages(GFP_KERNEL, order);
	if (!pages) {
		tzdev_print(0, "pages allocation failed\n");
		return;
	}

	nbytes = tzdev_smc_boot_log_read(page_to_pfn(pages), CONFIG_TZ_BOOT_LOG_PG_CNT);
	if (nbytes > 0)
		tz_iw_boot_log_print(page_address(pages), nbytes);

	__free_pages(pages, order);
}
