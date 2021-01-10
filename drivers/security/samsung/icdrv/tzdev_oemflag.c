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

#include <linux/debugfs.h>
#include <linux/types.h>
#include <linux/errno.h>
#if defined(CONFIG_TEEGRIS_VERSION) && (CONFIG_TEEGRIS_VERSION >= 4)
#include "extensions/irs.h"
#else
#include "tzirs.h"
#endif
#include "oemflag_arch.h"

static uint32_t run_cmd_teegris(uint32_t cmd, uint32_t arg1,
						uint32_t arg2, uint32_t arg3)
{
	int ret = 0;
	unsigned long p1, p2, p3;

	pr_info("[oemflag]tzirs cmd\n");

	p1 = arg2;	// param.name
	p2 = arg3;	// param.value
	p3 = cmd;	// param.func_cmd

	pr_info("[oemflag]before: id = 0x%lx, value = 0x%lx, cmd = 0x%lx\n",
		(unsigned long)p1, (unsigned long)p2, (unsigned long)p3);

	ret = tzirs_smc(&p1, &p2, &p3);

	pr_info("[oemflag]after: id = 0x%lx, value = 0x%lx, cmd = 0x%lx\n",
		(unsigned long)p1, (unsigned long)p2, (unsigned long)p3);

	if (ret) {
		pr_info("[oemflag]Unable to send IRS_CMD : id = 0x%lx, ret = %d\n",
				(unsigned long)p1, ret);
		return -EFAULT;
	}

	if (arg1)
		return p2;
	else
		return ret;
}

int set_tamper_fuse(enum oemflag_id name)
{
	int ret;

	ret = run_cmd_teegris(IRS_SET_FLAG_VALUE_CMD, 0, name, 1);

	return ret;
}

int get_tamper_fuse(enum oemflag_id name)
{
	int ret;

	ret = run_cmd_teegris(IRS_GET_FLAG_VAL_CMD, 1, name, 0);

	return ret;
}
