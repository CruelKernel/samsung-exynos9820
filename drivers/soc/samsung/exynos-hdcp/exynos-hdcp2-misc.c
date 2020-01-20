/*
 * drivers/soc/samsung/exynos-hdcp/exynos-hdcp2-misc.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>

void hdcp_hexdump(uint8_t *buf, size_t len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (i > 0 && !(i % 16))
			printk(KERN_ERR "\n");
		printk(KERN_ERR "%02x ", buf[i]);
	}
	printk(KERN_ERR "\n");
}
