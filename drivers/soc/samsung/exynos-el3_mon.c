/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * EXYNOS - EL3 monitor support
 * Author: Jang Hyunsung <hs79.jang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#include <linux/smc.h>
#include <asm/cacheflush.h>
#include <asm/memory.h>

#include <soc/samsung/exynos-el3_mon.h>

static char *smc_debug_mem;


#ifdef CONFIG_EXYNOS_KERNEL_PROTECTION
static int __init exynos_protect_kernel_text(void)
{
	int ret = 0;
	unsigned long ktext_start_va = 0;
	unsigned long ktext_start_pa = 0;
	unsigned long ktext_end_va = 0;
	unsigned long ktext_end_pa = 0;

	/* Get virtual addresses of kernel text */
	ktext_start_va = (unsigned long)_text;
	if (!ktext_start_va) {
		pr_err("%s: [ERROR] Kernel text start address is invalid\n",
								__func__);
		return -1;
	}

	ktext_end_va = (unsigned long)_etext;
	if (!ktext_end_va) {
		pr_err("%s: [ERROR] Kernel text end address is invalid\n",
								__func__);
		return -1;
	}

	/* Translate VA to PA */
	ktext_start_pa = (unsigned long)__pa_symbol(_text);
	ktext_end_pa = (unsigned long)__pa_symbol(_etext);

	pr_info("%s: Kernel text start VA(%pK), PA(%pK)\n",
			__func__, (void *)ktext_start_va, (void *)ktext_start_pa);
	pr_info("%s: Kernel text end VA(%pK), PA(%pK)\n",
			__func__, (void *)ktext_end_va, (void *)ktext_end_pa);

	/* I-cache flush to the PoC */
	flush_icache_range_poc(ktext_start_va, ktext_end_va);

	/* Request to protect kernel text area */
	ret = exynos_smc(SMC_CMD_PROTECT_KERNEL_TEXT,
			ktext_start_pa,
			ktext_end_pa,
			0);
	if (ret) {
		switch (ret) {
		case EXYNOS_ERROR_NOT_VALID_ADDRESS:
			pr_err("%s: [ERROR] Invalid address\n", __func__);
			break;
		case EXYNOS_ERROR_TZASC_WRONG_REGION:
			pr_err("%s: [ERROR] Wrong TZASC region\n", __func__);
			break;
		case EXYNOS_ERROR_ALREADY_INITIALIZED:
			pr_err("%s: [ERROR] Already initialized\n", __func__);
			break;
		default:
			pr_err("%s: [ERROR] Unknown error [ret = %#x]\n",
					__func__, ret);
			break;
		}

		return ret;
	}

	pr_info("%s: Success to set Kernel code as read-only\n", __func__);

	return 0;
}
core_initcall(exynos_protect_kernel_text);
#endif

static int  __init exynos_set_debug_mem(void)
{
	int ret;
	char *phys;
	unsigned int size = PAGE_SIZE * 2;

	smc_debug_mem = kmalloc(size, GFP_KERNEL);

	if (!smc_debug_mem) {
		pr_err("%s: kmalloc for smc_debug failed.\n", __func__);
		return 0;
	}

	/* to map & flush memory */
	memset(smc_debug_mem, 0x00, size);
	__flush_dcache_area(smc_debug_mem, size);

	phys = (char *)virt_to_phys(smc_debug_mem);
	pr_err("%s: alloc kmem for smc_dbg virt: 0x%pK phys: 0x%pK size: %d.\n",
			__func__, smc_debug_mem, phys, size);
	ret = exynos_smc(SMC_CMD_SET_DEBUG_MEM, (u64)phys, (u64)size, 0);

	/* correct return value is input size */
	if (ret != size) {
		pr_err("%s: Can not set the address to el3 monitor. "
				"ret = 0x%x. free the kmem\n", __func__, ret);
		kfree(smc_debug_mem);
	}

	return 0;
}
late_initcall(exynos_set_debug_mem);

static void exynos_smart_exception_handler(unsigned int id,
				unsigned long elr, unsigned long esr,
				unsigned long sctlr, unsigned long ttbr,
				unsigned long tcr, unsigned long x6,
				unsigned int offset)
{
	int i;
	unsigned long *ptr;
	unsigned long tmp;

	pr_err("========================================="
		"=========================================\n");

	if (id)
		pr_err("%s: There has been an unexpected exception from "
			"a LDFW which has smc id 0x%x\n\n", __func__, id);
	else
		pr_err("%s: There has been an unexpected exception from "
			"the EL3 monitor.\n\n", __func__);

	if (id) {
		pr_err("elr_el1   : 0x%016lx, \tesr_el1  : 0x%016lx\n",
								elr, esr);
		pr_err("sctlr_el1 : 0x%016lx, \tttbr_el1 : 0x%016lx\n",
								sctlr, ttbr);
		pr_err("tcr_el1   : 0x%016lx, \tlr (EL1) : 0x%016lx\n\n",
								tcr, x6);
		if ((offset > 0x0 && offset < (PAGE_SIZE * 2))
				&& !(offset % 0x8) && (smc_debug_mem)) {

			/* Invalidate smc_debug_mem for cache coherency */
			__inval_dcache_area(smc_debug_mem, PAGE_SIZE * 2);

			tmp = (unsigned long)smc_debug_mem;
			tmp += (unsigned long)offset;
			ptr = (unsigned long *)tmp;

			for (i = 0; i < 15; i++) {
				pr_err("x%02d : 0x%016lx, \tx%02d : 0x%016lx\n",
					i * 2, ptr[i * 2],
					i * 2 + 1, ptr[i * 2 + 1]);
			}
			pr_err("x%02d : 0x%016lx\n", i * 2,  ptr[i * 2]);
		} else {
			pr_err("GPR dump offset is not valid 0x%x\n", offset);
		}
	} else {
		pr_err("elr_el3   : 0x%016lx, \tesr_el3  : 0x%016lx\n",
								elr, esr);
		pr_err("sctlr_el3 : 0x%016lx, \tttbr_el3 : 0x%016lx\n",
								sctlr, ttbr);
		pr_err("tcr_el3   : 0x%016lx, \tscr_el3  : 0x%016lx\n\n",
								tcr, x6);

		if ((offset > 0x0 && offset < (PAGE_SIZE * 2))
				&& !(offset % 0x8) && (smc_debug_mem)) {

			/* Invalidate smc_debug_mem for cache coherency */
			__inval_dcache_area(smc_debug_mem, PAGE_SIZE * 2);

			tmp = (unsigned long)smc_debug_mem;
			tmp += (unsigned long)offset;
			ptr = (unsigned long *)tmp;

			for (i = 0; i < 15; i++) {
				pr_err("x%02d : 0x%016lx, \tx%02d : 0x%016lx\n",
					i * 2, ptr[i * 2],
					i * 2 + 1, ptr[i * 2 + 1]);
			}
			pr_err("x%02d : 0x%016lx\n", i * 2,  ptr[i * 2]);
		} else {
			pr_err("GPR dump offset is not valid 0x%x\n", offset);
		}
	}

	pr_err("\n[WARNING] IT'S GOING TO CAUSE KERNEL PANIC FOR DEBUGGING.\n\n");

	pr_err("========================================="
		"=========================================\n");
	/* make kernel panic */
	BUG();

	/* SHOULD NOT be here */
	while(1);
}

static int  __init exynos_set_seh_address(void)
{
	int ret;
	unsigned long addr = (unsigned long)exynos_smart_exception_handler;

	pr_info("%s: send smc call with SMC_CMD_SET_SEH_ADDRESS.\n", __func__);

	ret = exynos_smc(SMC_CMD_SET_SEH_ADDRESS, addr, 0, 0);

	/* return value not zero means failure */
	if (ret)
		pr_err("%s: did not set the seh address to el3 monitor. "
				"ret = 0x%x.\n", __func__, ret);
	else
		pr_err("%s: set the seh address to el3 monitor well.\n",
							__func__);

	return 0;
}
arch_initcall(exynos_set_seh_address);

int exynos_pd_tz_save(unsigned int addr)
{
	return exynos_smc(SMC_CMD_PREAPRE_PD_ONOFF,
				EXYNOS_GET_IN_PD_DOWN,
				addr,
				RUNTIME_PM_TZPC_GROUP);
}

int exynos_pd_tz_restore(unsigned int addr)
{
	return exynos_smc(SMC_CMD_PREAPRE_PD_ONOFF,
				EXYNOS_WAKEUP_PD_DOWN,
				addr,
				RUNTIME_PM_TZPC_GROUP);
}
