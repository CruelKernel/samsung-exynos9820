/*
 * ima-cert.S
 *
 * Copyright (C) 2016 Samsung Electronics, Inc.
 * Egor Uleyskiy, <e.uleyskiy@samsung.com>
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

#include <linux/export.h>
#include <linux/init.h>

	__INITRODATA

.align 8
#define GLOBAL(name)	\
	.globl VMLINUX_SYMBOL(name);	\
	VMLINUX_SYMBOL(name):

	.section ".init.data","aw"

#ifdef CONFIG_FIVE_CERT_ENG
GLOBAL(five_local_ca_start_eng)
	.incbin CONFIG_FIVE_CERT_ENG
GLOBAL(five_local_ca_end_eng)
#endif

#ifdef CONFIG_FIVE_CERT_USER
GLOBAL(five_local_ca_start_user)
	.incbin CONFIG_FIVE_CERT_USER
GLOBAL(five_local_ca_end_user)
#endif
