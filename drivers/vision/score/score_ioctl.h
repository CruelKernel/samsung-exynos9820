/*
 * Samsung Exynos SoC series SCore driver
 *
 * Definition of ioctl for controlling SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_IOCTL_H__
#define __SCORE_IOCTL_H__

#include <linux/fs.h>
#include "score_context.h"

/* Flags for boot setting */
#define SCORE_BOOT_FORCE		(0x1)

/**
 * struct score_ioctl_boot - Command to initialize and boot device
 * @ret: [out] additional result value
 * @firmware_id: [in] id to select booting setting
 * @pm_level [in] pm_qos level to be requested
 * @option: [in] additional control command
 * @reserved: reserved parameter
 */
struct score_ioctl_boot {
	int			ret;
	unsigned int		firmware_id;
	unsigned int		pm_level;
	unsigned int		flag;
	int			reserved[4];
};

/**
 * struct score_ioctl_dvfs - Data to control DVFS level
 * @ret: [out] additional result value
 * @cmd: [in] specific command to control DVFS (Not used)
 * @request_qos: [in] request qos value
 * @qos_count [out] count of DVFS level
 * @default_qos: [out] default qos value
 * @current_qos: [out] current qos value
 * @max_qos: [out] maximum qos value
 * @min_qos: [out] minimum qos value
 * @reserved: reserved parameter
 */
struct score_ioctl_dvfs {
	int			ret;
	int			cmd;
	int			request_qos;
	int			qos_count;
	int			default_qos;
	int			current_qos;
	int			max_qos;
	int			min_qos;
	int			reserved[4];
};

/**
 * struct score_ioctl_request - Request data for communication with user space
 * @packet_fd: [in] fd of user packet memory
 * @ret: [out] return value of SCore device
 * @kctx_id: [out] id of context
 * @task_id: [out] id of frame
 * @kernel_id: [in] id of DSP kernel
 * @reserved: reserved parameter
 * @timestamp: time to measure performance
 */
struct score_ioctl_request {
	int			packet_fd;
	int			ret;
	unsigned int		kctx_id;
	unsigned int		task_id;
	unsigned int		kernel_id;
	int			reserved[3];
	struct timespec		timestamp[8];
};

/**
 * struct score_ioctl_secure - Data for executing at Secure O/S
 * @ret: result value about command
 * @cmd: command type about Secure O/S
 * @reserved: reserved parameter
 */
struct score_ioctl_secure {
	int			ret;
	int			cmd;
	int			reserved[4];
};

/**
 * struct score_ioctl_test - Data for verification
 * @ret: result value of test
 * @cmd: command type about test
 * @size: additional data size for test
 * @addr: start address of additional data for test
 */
struct score_ioctl_test {
	int			ret;
	int			cmd;
	int			size;
	unsigned long		addr;
};

union score_ioctl_arg {
	struct score_ioctl_boot		boot;
	struct score_ioctl_dvfs		dvfs;
	struct score_ioctl_request	req;
	struct score_ioctl_secure	sec;
	struct score_ioctl_test		test;
};

/**
 * struct score_ioctl_ops - Operations possible at score_ioctl
 * @score_ioctl_boot: initialize and boot device
 * @score_ioctl_get_dvfs: get information of qos value for DVFS
 * @score_ioctl_set_dvfs: send command to control DVFS
 * @score_ioctl_request: send command to SCore device, receive result
 *                       from SCore device and send result to user
 * @score_ioctl_request_nonblock: send command to device only
 * @score_ioctl_request_wait: wait for result of nonblock request
 * @score_ioctl_secure: send command for execution at Secure O/S
 * @score_ioctl_test: test operation for verification
 */
struct score_ioctl_ops {
	int (*score_ioctl_boot)(struct score_context *sctx,
			struct score_ioctl_boot *arg);
	int (*score_ioctl_get_dvfs)(struct score_context *sctx,
			struct score_ioctl_dvfs *arg);
	int (*score_ioctl_set_dvfs)(struct score_context *sctx,
			struct score_ioctl_dvfs *arg);
	int (*score_ioctl_request)(struct score_context *sctx,
			struct score_ioctl_request *arg);
	int (*score_ioctl_request_nonblock)(struct score_context *sctx,
			struct score_ioctl_request *arg, bool wait);
	int (*score_ioctl_request_wait)(struct score_context *sctx,
			struct score_ioctl_request *arg);
	int (*score_ioctl_secure)(struct score_context *sctx,
			struct score_ioctl_secure *arg);
	int (*score_ioctl_test)(struct score_context *sctx,
			struct score_ioctl_test *arg);
};

long score_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#if defined(CONFIG_COMPAT)
long score_compat_ioctl32(struct file *file, unsigned int cmd,
		unsigned long arg);
#else
static inline long score_compat_ioctl32(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	return 0;
}
#endif

#endif
