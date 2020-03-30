/*
 * Set OEM flags
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
 * Phung Xuan Chien, <phung.chien@samsung.com>
 * Nguyen Hoang Thai Kiet, <kiet.nht@samsung.com>
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
#include "oemflag_arch.h"

/*
Note: res.a0 = func_cmd, res.a1 = name, res.a2 = value
*/
static uint32_t run_cmd_mtk_atf(uint32_t cmd, uint32_t index)
{
	struct arm_smccc_res res;
	arm_smccc_smc(cmd, index, 0, 0, 0, 0, 0, 0, &res);
	return res.a2;
}

int set_tamper_fuse(enum oemflag_id name)
{
	int ret;

	pr_info("[oemflag][icdrv] MTK ATF cmd\n");
	ret = run_cmd_mtk_atf(MTK_SIP_OEM_FLAG_WRITE, name);
	return (ret == 1) ? 0 : ret;
}

int get_tamper_fuse(enum oemflag_id flag)
{
	return 0;
}
