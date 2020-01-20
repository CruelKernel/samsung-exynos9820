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

#ifndef _TZIRS_H_
#define _TZIRS_H_

#include <linux/ioctl.h>

#define IOC_MAGIC       'h'
#define TZIRS_NAME      "tzirs"
#define TZIRS_DEV       "/dev/"TZIRS_NAME

typedef struct irs_ctx {
	uint32_t id;            /* r1 - Flag ID */
	uint32_t func_cmd;      /* r2 - Function CMD */
	uint32_t value;         /* r3 - Value or irs_flag.param (IOCTL_ADD_FLAG) */
} irs_ctx_t;

#define IOCTL_IRS_CMD           _IOWR(IOC_MAGIC, 1, struct irs_ctx)

typedef enum {
	IRS_SET_FLAG_CMD        =           1,
	IRS_SET_FLAG_VALUE_CMD,
	IRS_INC_FLAG_CMD,
	IRS_GET_FLAG_VAL_CMD,
	IRS_ADD_FLAG_CMD,
	IRS_DEL_FLAG_CMD
} TZ_IRS_CMD;

typedef enum {
	IRS_FAIL			= -100,		/* Fail result */
	IRS_UNKNOWN_ID,					/* Unknown flag id */
	IRS_UNKNOWN_INT_CMD,				/* Unknown internal command */
	IRS_INCORRECT_FLAG_TYPE,			/* Incorrect flag type (can be boolean, value or counter) */
	IRS_RT_FLAGS_EMPTY,				/* List of run-time flags is empty */
	IRS_RT_FLAGS_FULL,
	IRS_INCORRECT_RT_ID,
	IRS_DENY_READ_FROM_SMC,
	IRS_DENY_WRITE_FROM_SMC,
	IRS_DENY_DELETE_FROM_SMC,
	IRS_SUCCESS			= 0		/*Success result*/
} TZ_IRS_ERR;

long tzirs_smc(unsigned long *p1, unsigned long *p2, unsigned long *p3);

#endif /* _TZIRS_H_ */
