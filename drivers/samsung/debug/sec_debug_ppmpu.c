/*
 *sec_debug_ppmpu.c
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/sec_debug.h>
#include <linux/smc.h>

static void __print_ppmpu_prot(u64 virt, int num)
{
	int ret;
	u64 phys;

	/*
	 * don't attempt to dump non-kernel addresses or
	 * values that are probably just small negative numbers
	 */
	if (virt < PAGE_OFFSET || virt >= -256UL) {
		/*
		 * If kaslr is enabled, Kernel code is able to
		 * locate in VMALLOC address.
		 */
		if (virt < (unsigned long)KERNEL_START ||
		    virt > (unsigned long)KERNEL_END)
			return;
	}

	phys = __virt_to_phys(virt);

	/* 0(not matched with secure memory), 1(matched with secure memory) */
	ret = exynos_smc(SMC_PPMPU_GET_PROT, phys, 0, 0);
	pr_err("x%-2d: %016llx (0x%llx): %s(%d)", num, virt, phys,
	       (ret == 1) ? "Protected" : (ret == 0) ? "unprotected" : "", ret);
}

void print_ppmpu_protection(struct pt_regs *regs)
{
	int i;

	pr_err("PPMPU:");
	exynos_smc(SMC_PPMPU_PR_VIOLATION, 0, 0, 0);

	if (user_mode(regs))
		return;

	for (i = 0; i < 30; i++)
		__print_ppmpu_prot(regs->regs[i], i);
}
