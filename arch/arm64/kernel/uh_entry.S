/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/linkage.h>

#define ESS_FLAG_IN	(1)
#define ESS_FLAG_OUT	(3)

ENTRY(uh_call)
#ifdef CONFIG_EXYNOS_SNAPSHOT_LOGGING_HVC_CALL
	stp	x29, x30, [sp, #-64]!
	mov	x29, sp
	stp	x0, x1, [sp, #16]
	stp	x2, x3, [sp, #32]
	stp	x4, x5, [sp, #48]
	mov	x5, #ESS_FLAG_IN
	bl	exynos_ss_hvc
	ldp	x0, x1, [sp, #16]
	ldp	x2, x3, [sp, #32]
	ldp	x4, x5, [sp, #48]
#endif
	hvc	#0
#ifdef CONFIG_EXYNOS_SNAPSHOT_LOGGING_HVC_CALL
	stp	x0, x1, [sp, #16]
	stp	x2, x3, [sp, #32]
	stp	x4, x5, [sp, #48]
	mov	x5, #ESS_FLAG_OUT
	bl	exynos_ss_hvc
	ldp	x0, x1, [sp, #16]
	ldp	x2, x3, [sp, #32]
	ldp	x4, x5, [sp, #48]
	ldp	x29, x30, [sp], #64
#endif
	ret
ENDPROC(uh_call)