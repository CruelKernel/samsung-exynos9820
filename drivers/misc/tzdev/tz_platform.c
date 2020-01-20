/*
 * Copyright (C) 2012-2017, Samsung Electronics Co., Ltd.
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
#include <linux/errno.h>
#include <linux/fs.h>

#include "tzdev.h"

/* TZDEV platform specific functions */
int __weak tzdev_platform_register(void)
{
	return 0;
}

void __weak tzdev_platform_unregister(void)
{
	return;
}

int __weak tzdev_platform_init(void)
{
	return tzdev_run_init_sequence();
}

int __weak tzdev_platform_fini(void)
{
	return 0;
}

int __weak tzdev_platform_open(struct inode *inode, struct file *filp)
{
	(void)inode;
	(void)filp;

	return 0;
}

int __weak tzdev_platform_close(struct inode *inode, struct file *filp)
{
	(void)inode;
	(void)filp;

	return 0;
}

long __weak tzdev_fd_platform_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	(void)filp;
	(void)cmd;
	(void)arg;

	return -ENOTTY;
}

int __weak tzdev_fd_platform_close(struct inode *inode, struct file *filp)
{
	(void)inode;
	(void)filp;

	return 0;
}

int __weak tzdev_platform_smc_call(struct tzdev_smc_data *data)
{
	register unsigned long _r0 __asm__(REGISTERS_NAME "0") = data->args[0] | TZDEV_SMC_MAGIC;
	register unsigned long _r1 __asm__(REGISTERS_NAME "1") = data->args[1];
	register unsigned long _r2 __asm__(REGISTERS_NAME "2") = data->args[2];
	register unsigned long _r3 __asm__(REGISTERS_NAME "3") = data->args[3];
	register unsigned long _r4 __asm__(REGISTERS_NAME "4") = data->args[4];
	register unsigned long _r5 __asm__(REGISTERS_NAME "5") = data->args[5];
	register unsigned long _r6 __asm__(REGISTERS_NAME "6") = data->args[6];

	__asm__ __volatile("sub sp, sp, #16\n"
			"str %0, [sp]\n" : : "r" (data));

	__asm__ __volatile__(ARCH_EXTENSION SMC(0): "+r"(_r0) , "+r" (_r1) , "+r" (_r2),
			"+r" (_r3), "+r" (_r4), "+r" (_r5), "+r" (_r6) : : "memory");

	__asm__ __volatile("ldr %0, [sp]\n"
			"add sp, sp, #16\n" : : "r" (data));

	data->args[0] = (uint32_t)_r0;
	data->args[1] = (uint32_t)_r1;
	data->args[2] = (uint32_t)_r2;
	data->args[3] = (uint32_t)_r3;

	return 0;
}

uint32_t __weak tzdev_platform_get_sysconf_flags(void)
{
	return 0;
}
