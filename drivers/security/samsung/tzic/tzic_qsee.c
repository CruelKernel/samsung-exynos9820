/*
 * Set OEM flags
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
 * Jonghun Song, <justin.song@samsung.com>
 * Egor Ulesykiy, <e.uelyskiy@samsung.com>
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

#include <linux/printk.h>
#include <linux/types.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
#include <soc/qcom/scm.h>
#else
#include <soc/qcom/qseecom_scm.h>
#endif
#include "tzic.h"

#ifndef SCM_SVC_FUSE
#define SCM_SVC_FUSE            0x08
#endif
#define SCM_BLOW_SW_FUSE_ID     0x01
#define SMC_FUSE 0x83000004
#define SCM_IS_SW_FUSE_BLOWN_ID 0x02

static int set_tamper_fuse_qsee(uint32_t index)
{
	struct scm_desc desc = {0};
	uint32_t fuse_id;

	desc.args[0] = fuse_id = index;
	desc.arginfo = SCM_ARGS(1);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
	return scm_call2(SCM_SIP_FNID(SCM_SVC_FUSE, SCM_BLOW_SW_FUSE_ID),
			&desc);
#else
	return qcom_scm_qseecom_call(SCM_SIP_FNID(SCM_SVC_FUSE, SCM_BLOW_SW_FUSE_ID),
			&desc);
#endif
}

static int get_tamper_fuse_qsee(uint32_t index)
{
	int ret;
	uint32_t fuse_id;
	uint8_t resp_buf;
	size_t resp_len;
	struct scm_desc desc = {0};

	resp_len = sizeof(resp_buf);

	desc.args[0] = fuse_id = index;
	desc.arginfo = SCM_ARGS(1);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
	ret = scm_call2(SCM_SIP_FNID(SCM_SVC_FUSE, SCM_IS_SW_FUSE_BLOWN_ID),
		&desc);
#else
	ret = qcom_scm_qseecom_call(SCM_SIP_FNID(SCM_SVC_FUSE, SCM_IS_SW_FUSE_BLOWN_ID),
		&desc);
#endif
	resp_buf = desc.ret[0];

	if (ret) {
		pr_info("scm_call/2 returned %d", ret);
		resp_buf = 0xff;
	}

	return resp_buf;
}

int tzic_oem_flags_set(enum oemflag_id index)
{
	int ret;
	pr_info("[oemflag]qsee cmd\n");
	ret = set_tamper_fuse_qsee(index);
	return ret;
}

int tzic_oem_flags_get(enum oemflag_id index)
{
	int ret;
	pr_info("[oemflag]qsee cmd\n");
	ret = get_tamper_fuse_qsee(index);
	return ret;
}
