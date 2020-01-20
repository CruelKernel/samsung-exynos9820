/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/uaccess.h>

#include "score_log.h"
#include "score_context.h"
#include "score_ioctl.h"

#define SCORE_IOC_BOOT				\
	_IOWR('S', 0, struct score_ioctl_boot)
#define SCORE_IOC_GET_DEVICE_INFO		\
	_IOWR('S', 1, unsigned long)

#define SCORE_IOC_GET_DVFS			\
	_IOWR('S', 2, struct score_ioctl_dvfs)
#define SCORE_IOC_SET_DVFS			\
	_IOWR('S', 3, struct score_ioctl_dvfs)

#define SCORE_IOC_REQUEST			\
	_IOWR('S', 5, struct score_ioctl_request)
#define SCORE_IOC_REQUEST_NONBLOCK		\
	_IOWR('S', 6, struct score_ioctl_request)
#define SCORE_IOC_REQUEST_WAIT			\
	_IOWR('S', 7, struct score_ioctl_request)
#define SCORE_IOC_REQUEST_NONBLOCK_NOWAIT	\
	_IOWR('S', 8, struct score_ioctl_request)

#define SCORE_IOC_SECURE			\
	_IOWR('S', 10, struct score_ioctl_secure)
#define SCORE_IOC_TEST				\
	_IOWR('S', 11, struct score_ioctl_test)

long score_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	union score_ioctl_arg karg;
	void __user *uarg;
	struct score_context *sctx;
	const struct score_ioctl_ops *ops;

	score_enter();
	if (_IOC_SIZE(cmd) > sizeof(karg)) {
		score_err("cmd size(%u) is invalid (Max:%zu)\n",
				_IOC_SIZE(cmd), sizeof(karg));
		ret = -EINVAL;
		goto p_err;
	}

	uarg = (void __user *)arg;
	ret = copy_from_user(&karg, uarg, _IOC_SIZE(cmd));
	if (ret) {
		score_err("Failed to copy from user (%d)\n", ret);
		goto p_err;
	}

	sctx = file->private_data;
	ops = sctx->core->ioctl_ops;
	switch (cmd) {
	case SCORE_IOC_BOOT:
		ret = ops->score_ioctl_boot(sctx, &karg.boot);
		if (ret)
			goto p_err;
		break;
	case SCORE_IOC_GET_DEVICE_INFO:
		ret = 0;
		break;
	case SCORE_IOC_GET_DVFS:
		ret = ops->score_ioctl_get_dvfs(sctx, &karg.dvfs);
		if (ret)
			goto p_err;
		break;
	case SCORE_IOC_SET_DVFS:
		ret = ops->score_ioctl_set_dvfs(sctx, &karg.dvfs);
		if (ret)
			goto p_err;
		break;
	case SCORE_IOC_REQUEST:
		ret = ops->score_ioctl_request(sctx, &karg.req);
		if (ret)
			goto p_err;
		break;
	case SCORE_IOC_REQUEST_NONBLOCK:
		ret = ops->score_ioctl_request_nonblock(sctx, &karg.req, true);
		if (ret)
			goto p_err;
		break;
	case SCORE_IOC_REQUEST_WAIT:
		ret = ops->score_ioctl_request_wait(sctx, &karg.req);
		if (ret)
			goto p_err;
		break;
	case SCORE_IOC_REQUEST_NONBLOCK_NOWAIT:
		ret = ops->score_ioctl_request_nonblock(sctx, &karg.req, false);
		if (ret)
			goto p_err;
		break;
	case SCORE_IOC_SECURE:
		ret = ops->score_ioctl_secure(sctx, &karg.sec);
		if (ret)
			goto p_err;
		break;
	case SCORE_IOC_TEST:
		ret = ops->score_ioctl_test(sctx, &karg.test);
		if (ret)
			goto p_err;
		break;
	default:
		score_err("invalid ioctl command\n");
		ret = -EINVAL;
		goto p_err;
	}

	ret = copy_to_user(uarg, &karg, _IOC_SIZE(cmd));
	if (ret) {
		score_err("Failed to copy to user (%d)\n", ret);
		goto p_err;
	}
	score_leave();
p_err:
	return ret;
}
