/*
 * Copyright (C) 2016 Samsung Electronics, Inc.
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

#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/printk.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include "tzirs.h"
#include "tz_cdev.h"
#include "tz_platform.h"
#include "tzdev.h"

#if defined(CONFIG_ARCH_MSM) && defined(CONFIG_MSM_SCM)
#if defined(CONFIG_ARCH_MSM8939) || defined(CONFIG_ARCH_MSM8996)
#include <soc/qcom/scm.h>
#else
#error "Unsupported target! The only MSM8996 and MSM8939 are supported for Qualcomm chipset"
#endif
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oleksii Mosolab");

#ifdef TZIRS_DEBUG
#define DBG(...)		pr_info( "[tzirs] DBG : " __VA_ARGS__)
#define ERR(...)		pr_alert("[tzirs] ERR : " __VA_ARGS__)
#else
#define DBG(...)
#define ERR(...)		pr_alert("[tzirs] ERR : " __VA_ARGS__)
#endif

/* Set appropriate command value */
#define SMC_IRS_CMD_RAW	(0x0000000B)

#ifdef CONFIG_TZDEV_USE_ARM_CALLING_CONVENTION
#define SMC_IRS_CMD		CREATE_SMC_CMD(SMC_TYPE_FAST, SMC_AARCH_32, SMC_TOS0_SERVICE_MASK, SMC_IRS_CMD_RAW)
#else
#define SMC_IRS_CMD		SMC_IRS_CMD_RAW
#endif

#if defined(CONFIG_ARCH_MSM) && defined(CONFIG_MSM_SCM)
#define SCM_BLOW_SW_FUSE_ID     0x01
#define SCM_IS_SW_FUSE_BLOWN_ID 0x02

static long __tzirs_smc_cmd(uint32_t p0, uint32_t *p1, uint32_t *p2, uint32_t *p3)
{
	unsigned int ret;

	struct scm_desc desc = {0};
	desc.args[0] = *p1;
	desc.arginfo = SCM_ARGS(1);

	switch(*p3) {
	case IRS_SET_FLAG_CMD:
		ret = scm_call2(SCM_SIP_FNID(p0, SCM_BLOW_SW_FUSE_ID), &desc);
		DBG("[SET_FLAG_CMD] : scm_call2 returned 0x%08x\n", ret);
		break;

	case IRS_GET_FLAG_VAL_CMD:
		ret = scm_call2(SCM_SIP_FNID(p0, SCM_IS_SW_FUSE_BLOWN_ID), &desc);
		if (ret) {
			ERR("[GET_FLAG_CMD] : scm_call2 failed ret = 0x%08x\n", ret);
			break;
		}
		DBG("[GET_FLAG_CMD] : desc.ret[0] = 0x%08x\n", (unsigned int) desc.ret[0]);
		*p2 = (uint32_t) desc.ret[0];
		break;

	default:
		ERR("Wrong SCM command ID\n");
		ret = IRS_INCORRECT_FLAG_TYPE;
		break;
	}

	return ret;
}
#else /* defined(CONFIG_ARCH_MSM) && defined(CONFIG_MSM_SCM) */
static long __tzirs_smc_cmd(unsigned long p0, unsigned long *p1, unsigned long *p2, unsigned long *p3)
{
	struct tzdev_smc_data data;

	data.args[0] = (uint32_t)p0;
	data.args[1] = (uint32_t)*p1;
	data.args[2] = (uint32_t)*p2;
	data.args[3] = (uint32_t)*p3;
	data.args[4] = 0;
	data.args[5] = 0;
	data.args[6] = 0;

	tzdev_platform_smc_call(&data);

	*p1 = data.args[1];
	*p2 = data.args[2];
	*p3 = data.args[3];

	return data.args[0];
}
#endif /* defined(CONFIG_ARCH_MSM) && defined(CONFIG_MSM_SCM) */

long tzirs_smc(unsigned long *p1, unsigned long *p2, unsigned long *p3)
{
#if defined(CONFIG_ARCH_MSM) && defined(CONFIG_MSM_SCM)
	return __tzirs_smc_cmd(SCM_SVC_FUSE, (uint32_t *)p1, (uint32_t *)p2 , (uint32_t *)p3);
#else
	return __tzirs_smc_cmd(SMC_IRS_CMD, p1, p2, p3);
#endif
}
