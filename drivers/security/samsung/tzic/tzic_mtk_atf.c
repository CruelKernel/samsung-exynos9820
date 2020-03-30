/*
 * Set OEM flags
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
 * Nguyen Hoang Thai Kiet, <kiet.nht@samsung.com>
 * Phung Xuan Chien, <phung.chien@samsung.com>
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

#include <linux/types.h>
#include <linux/arm-smccc.h>
#include <mt-plat/mtk_secure_api.h>
#include "tzic.h"

/*
Note: res.a0 = func_cmd, res.a1 = name, res.a2 = value
*/
static uint32_t run_cmd_mtk_atf(uint32_t cmd, uint32_t index)
{
	struct arm_smccc_res res;
	arm_smccc_smc(cmd, index, 0, 0, 0, 0, 0, 0, &res);
	return res.a2;
}

int tzic_oem_flags_set(enum oemflag_id index)
{
	int ret;
	pr_info("[oemflag]MTK ATF cmd\n");
	ret = run_cmd_mtk_atf(MTK_SIP_OEM_FLAG_WRITE, index);
	return (ret == 1) ? 0 : ret;
}

int tzic_oem_flags_get(enum oemflag_id index)
{
	int ret;
	pr_info("[oemflag]MTK ATF cmd\n");
	ret = run_cmd_mtk_atf(MTK_SIP_OEM_FLAG_READ, index);
	return ret;
}
