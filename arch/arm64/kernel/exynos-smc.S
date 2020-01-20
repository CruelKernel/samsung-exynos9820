/*
 * Copyright (C) 2014 Samsung Electronics.
 *
 * For Secure Monitor Call(SMC)
 *
 * This program is free software,you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/linkage.h>

#include <linux/smc.h>

#include <asm/assembler.h>

#define ESS_FLAG_IN	(1)
#define ESS_FLAG_OUT	(3)

/*
 * Function signature:
 * int exynos_smc(unsigned long cmd, unsigned long arg1, unsigned long arg2, unsigned long arg3)
 * int exynos_smc_readsfr(unsigned long addr, unsigned long* val)
 */

ENTRY(__exynos_smc)
#ifdef CONFIG_DEBUG_SNAPSHOT_LOGGING_SMC
	stp	x29, x30, [sp, #-48]!
	mov	x29, sp
	stp	x0, x1, [sp, #16]
	stp	x2, x3, [sp, #32]
	ldr	x3, =save_smc_id
	str	x0, [x3]
	mov	x3, #ESS_FLAG_IN
	bl	dbg_snapshot_irq
	ldp	x0, x1, [sp, #16]
	ldp	x2, x3, [sp, #32]
#endif
	dsb	sy
	smc	#0
#ifdef CONFIG_DEBUG_SNAPSHOT_LOGGING_SMC
	stp	x0, x1, [sp, #16]
	stp	x2, x3, [sp, #32]
	ldr	x3, =save_smc_id
	ldr	x0, [x3]
	mov	x3, #ESS_FLAG_OUT
	bl	dbg_snapshot_irq
	ldp	x0, x1, [sp, #16]
	ldp	x2, x3, [sp, #32]
	ldp	x29, x30, [sp], #48
#endif
	ret

	.section .bss
	.align	3
save_smc_id:
	.quad	0x0
	.previous

ENDPROC(__exynos_smc)

ENTRY(exynos_smc_readsfr)
	stp	x1, x3, [sp, #-16]!

	/* Currently, the addresses of SFR are 32bit */
	lsr	w1, w0, #2
	orr	w1, w1, #SMC_REG_CLASS_SFR_R
	mov	w0, #SMC_CMD_REG
	dsb	sy
	smc	#0
	ldp	x1, x3, [sp], #16
	cmp	x0, #0
	b.ne	fail_read

	str	x2, [x1]
fail_read:
	ret
ENDPROC(exynos_smc_readsfr)
