/*
 * Set OEM flags
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
 * Jonghun Song, <justin.song@samsung.com>
 * Egor Ulesykiy, <e.uelyskiy@samsung.com>
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

#include <linux/kernel.h>
#include <linux/types.h>
#include "oemflag_arch.h"

#define SMC_SET_FUSE 0x83000004
#define SMC_GET_FUSE 0x83000003

static uint32_t run_cmd_kinibi(uint32_t cmd, uint32_t arg1,
						uint32_t arg2, uint32_t arg3)
{
	register u64 reg0 __asm__("x0") = cmd;
	register u64 reg1 __asm__("x1") = arg1;
	register u64 reg2 __asm__("x2") = arg2;
	register u64 reg3 __asm__("x3") = arg3;

	__asm__ volatile (
		"dsb    sy\n"
		"smc    0\n"
		: "+r"(reg0), "+r"(reg1), "+r"(reg2), "+r"(reg3)

	);

	return reg1;
}

int set_tamper_fuse(enum oemflag_id name)
{
	int ret;

	ret = run_cmd_kinibi(SMC_SET_FUSE, 0, name, 0);

	return ret;
}

int get_tamper_fuse(enum oemflag_id name)
{
	int ret;

	pr_info("[oemflag]kinibi cmd\n");
	ret = run_cmd_kinibi(SMC_GET_FUSE, 1, name, 0);

	return ret;
}
