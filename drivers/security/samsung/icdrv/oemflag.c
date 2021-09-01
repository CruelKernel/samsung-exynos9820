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

#include <linux/printk.h>
#include <linux/types.h>
#include <linux/errno.h>
#include "oemflag.h"
#include "oemflag_arch.h"

static int oem_flags_check[OEMFLAG_NUM_OF_FLAG];

static int check_flags(uint32_t flag_index)
{
	return oem_flags_check[flag_index];
}

int oem_flags_set(enum oemflag_id index)
{
	int ret = 0;
	uint32_t name = index;

	pr_info("[oemflag]START\n");
	pr_info("[oemflag]set_fuse new\n");

	if (name > OEMFLAG_MIN_FLAG && name < OEMFLAG_NUM_OF_FLAG) {
		if (check_flags(name)) {
			pr_info("[oemflag]flag is already set. %u\n", name);
			return OEMFLAG_SUCCESS;
		}

		pr_info("[oemflag]set_fuse_name : %u\n", name);

		ret = set_tamper_fuse(name);
		if (ret) {
			pr_err("set_tamper_fuse error: ret=%d\n", ret);
			return OEMFLAG_FAIL;
		}
		ret = get_tamper_fuse(name);
		if (!ret) {
			pr_err("get_tamper_fuse error: ret=%d\n", ret);
			return OEMFLAG_FAIL;
		}
		oem_flags_check[name] = 1;
	} else {
		pr_info("[oemflag]param name is wrong\n");
		return -EINVAL;
	}
	return OEMFLAG_SUCCESS;
}

int oem_flags_get(enum oemflag_id index)
{
	uint32_t name = index;

	return get_tamper_fuse(name);
}
