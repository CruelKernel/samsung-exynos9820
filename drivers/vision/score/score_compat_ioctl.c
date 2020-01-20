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

struct score_ioctl_boot32 {
	int			ret;
	unsigned int		firmware_id;
	unsigned int		pm_level;
	unsigned int		flag;
	int			reserved[4];
};

struct score_ioctl_dvfs32 {
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

struct score_ioctl_request32 {
	int			packet_fd;
	int			ret;
	unsigned int		kctx_id;
	unsigned int		task_id;
	unsigned int		kernel_id;
	int			reserved[3];
	struct compat_timespec	timestamp[8];
};

struct score_ioctl_secure32 {
	int			ret;
	int			cmd;
	int			reserved[4];
};

struct score_ioctl_test32 {
	int			ret;
	int			cmd;
	int			size;
	compat_caddr_t		addr;
};

#define SCORE_IOC_BOOT32			\
	_IOWR('S', 0, struct score_ioctl_boot32)
#define SCORE_IOC_GET_DEVICE_INFO32		\
	_IOWR('S', 1, compat_caddr_t)

#define SCORE_IOC_GET_DVFS32			\
	_IOWR('S', 2, struct score_ioctl_dvfs32)
#define SCORE_IOC_SET_DVFS32			\
	_IOWR('S', 3, struct score_ioctl_dvfs32)

#define SCORE_IOC_REQUEST32			\
	_IOWR('S', 5, struct score_ioctl_request32)
#define SCORE_IOC_REQUEST_NONBLOCK32		\
	_IOWR('S', 6, struct score_ioctl_request32)
#define SCORE_IOC_REQUEST_WAIT32		\
	_IOWR('S', 7, struct score_ioctl_request32)
#define SCORE_IOC_REQUEST_NONBLOCK_NOWAIT32	\
	_IOWR('S', 8, struct score_ioctl_request32)

#define SCORE_IOC_SECURE32			\
	_IOWR('S', 10, struct score_ioctl_secure32)
#define SCORE_IOC_TEST32			\
	_IOWR('S', 11, struct score_ioctl_test32)

static int __score_ioctl_get_boot32(struct score_ioctl_boot *karg,
		struct score_ioctl_boot32 __user *uarg)
{
	int ret = 0;

	score_enter();
	ret = access_ok(VERIFY_READ, uarg, sizeof(*uarg));
	if (!ret) {
		ret = -EFAULT;
		goto p_err;
	}
	memset(karg, 0, sizeof(*karg));

	ret = get_user(karg->firmware_id, &uarg->firmware_id);
	if (ret)
		goto p_err;
	ret = get_user(karg->pm_level, &uarg->pm_level);
	if (ret)
		goto p_err;
	ret = get_user(karg->flag, &uarg->flag);
	if (ret)
		goto p_err;

	score_leave();
	return ret;
p_err:
	score_err("Failed to copy from user (%d)\n", ret);
	return ret;
}

static int __score_ioctl_put_boot32(struct score_ioctl_boot *karg,
		struct score_ioctl_boot32 __user *uarg)
{
	int ret = 0;

	score_enter();
	ret = access_ok(VERIFY_WRITE, uarg, sizeof(*uarg));
	if (!ret) {
		ret = -EFAULT;
		goto p_err;
	}

	ret = put_user(karg->ret, &uarg->ret);
	if (ret)
		goto p_err;

	score_leave();
	return ret;
p_err:
	score_err("Failed to copy to user (%d)\n", ret);
	return ret;
}

static int __score_ioctl_get_dvfs_info32(struct score_ioctl_dvfs *karg,
		struct score_ioctl_dvfs32 __user *uarg)
{
	int ret = 0;

	score_enter();
	ret = access_ok(VERIFY_READ, uarg, sizeof(*uarg));
	if (!ret) {
		ret = -EFAULT;
		goto p_err;
	}
	memset(karg, 0, sizeof(*karg));

	ret = get_user(karg->cmd, &uarg->cmd);
	if (ret)
		goto p_err;

	score_leave();
	return ret;
p_err:
	score_err("Failed to copy from user (%d)\n", ret);
	return ret;
}

static int __score_ioctl_put_dvfs_info32(struct score_ioctl_dvfs *karg,
		struct score_ioctl_dvfs32 __user *uarg)
{
	int ret = 0;

	score_enter();
	ret = access_ok(VERIFY_WRITE, uarg, sizeof(*uarg));
	if (!ret) {
		ret = -EFAULT;
		goto p_err;
	}

	ret = put_user(karg->ret, &uarg->ret);
	if (ret)
		goto p_err;
	ret = put_user(karg->qos_count, &uarg->qos_count);
	if (ret)
		goto p_err;
	ret = put_user(karg->default_qos, &uarg->default_qos);
	if (ret)
		goto p_err;
	ret = put_user(karg->current_qos, &uarg->current_qos);
	if (ret)
		goto p_err;
	ret = put_user(karg->max_qos, &uarg->max_qos);
	if (ret)
		goto p_err;
	ret = put_user(karg->min_qos, &uarg->min_qos);
	if (ret)
		goto p_err;

	score_leave();
	return ret;
p_err:
	score_err("Failed to copy to user (%d)\n", ret);
	return ret;
}

static int __score_ioctl_get_dvfs_req32(struct score_ioctl_dvfs *karg,
		struct score_ioctl_dvfs32 __user *uarg)
{
	int ret = 0;

	score_enter();
	ret = access_ok(VERIFY_READ, uarg, sizeof(*uarg));
	if (!ret) {
		ret = -EFAULT;
		goto p_err;
	}
	memset(karg, 0, sizeof(*karg));

	ret = get_user(karg->cmd, &uarg->cmd);
	if (ret)
		goto p_err;
	ret = get_user(karg->request_qos, &uarg->request_qos);
	if (ret)
		goto p_err;

	score_leave();
	return ret;
p_err:
	score_err("Failed to copy from user (%d)\n", ret);
	return ret;
}

static int __score_ioctl_put_dvfs_req32(struct score_ioctl_dvfs *karg,
		struct score_ioctl_dvfs32 __user *uarg)
{
	int ret = 0;

	score_enter();
	ret = access_ok(VERIFY_WRITE, uarg, sizeof(*uarg));
	if (!ret) {
		ret = -EFAULT;
		goto p_err;
	}

	ret = put_user(karg->ret, &uarg->ret);
	if (ret)
		goto p_err;
	ret = put_user(karg->current_qos, &uarg->current_qos);
	if (ret)
		goto p_err;

	score_leave();
	return ret;
p_err:
	score_err("Failed to copy to user (%d)\n", ret);
	return ret;
}

static int __score_ioctl_get_request32(struct score_ioctl_request *karg,
		struct score_ioctl_request32 __user *uarg)
{
	int ret = 0;

	score_enter();
	ret = access_ok(VERIFY_READ, uarg, sizeof(*uarg));
	if (!ret) {
		ret = -EFAULT;
		goto p_err;
	}
	memset(karg, 0, sizeof(*karg));

	ret = get_user(karg->packet_fd, &uarg->packet_fd);
	if (ret)
		goto p_err;
	ret = get_user(karg->kernel_id, &uarg->kernel_id);
	if (ret)
		goto p_err;

	score_leave();
	return ret;
p_err:
	score_err("Failed to copy from user (%d)\n", ret);
	return ret;
}

static int __score_ioctl_put_request32(struct score_ioctl_request *karg,
		struct score_ioctl_request32 __user *uarg)
{
	int ret = 0;
	int idx;

	score_enter();
	ret = access_ok(VERIFY_WRITE, uarg, sizeof(*uarg));
	if (!ret) {
		ret = -EFAULT;
		goto p_err;
	}

	ret = put_user(karg->ret, &uarg->ret);
	if (ret)
		goto p_err;
	ret = put_user(karg->kctx_id, &uarg->kctx_id);
	if (ret)
		goto p_err;
	ret = put_user(karg->task_id, &uarg->task_id);
	if (ret)
		goto p_err;

	for (idx = 0; idx < 8; ++idx) {
		ret = put_user(karg->timestamp[idx].tv_sec,
				&uarg->timestamp[idx].tv_sec);
		if (ret)
			goto p_err;
		ret = put_user(karg->timestamp[idx].tv_nsec,
				&uarg->timestamp[idx].tv_nsec);
		if (ret)
			goto p_err;
	}

	score_leave();
	return ret;
p_err:
	score_err("Failed to copy to user (%d)\n", ret);
	return ret;
}

static int __score_ioctl_get_request_nonblock32(
		struct score_ioctl_request *karg,
		struct score_ioctl_request32 __user *uarg)
{
	int ret = 0;

	score_enter();
	ret = access_ok(VERIFY_READ, uarg, sizeof(*uarg));
	if (!ret) {
		ret = -EFAULT;
		goto p_err;
	}
	memset(karg, 0, sizeof(*karg));

	ret = get_user(karg->packet_fd, &uarg->packet_fd);
	if (ret)
		goto p_err;
	ret = get_user(karg->kernel_id, &uarg->kernel_id);
	if (ret)
		goto p_err;

	score_leave();
	return ret;
p_err:
	score_err("Failed to copy from user (%d)\n", ret);
	return ret;
}

static int __score_ioctl_put_request_nonblock32(
		struct score_ioctl_request *karg,
		struct score_ioctl_request32 __user *uarg)
{
	int ret = 0;

	score_enter();
	ret = access_ok(VERIFY_WRITE, uarg, sizeof(*uarg));
	if (!ret) {
		ret = -EFAULT;
		goto p_err;
	}

	ret = put_user(karg->ret, &uarg->ret);
	if (ret)
		goto p_err;
	ret = put_user(karg->kctx_id, &uarg->kctx_id);
	if (ret)
		goto p_err;
	ret = put_user(karg->task_id, &uarg->task_id);
	if (ret)
		goto p_err;

	score_leave();
	return ret;
p_err:
	score_err("Failed to copy to user (%d)\n", ret);
	return ret;
}

static int __score_ioctl_get_request_wait32(
		struct score_ioctl_request *karg,
		struct score_ioctl_request32 __user *uarg)
{
	int ret = 0;

	score_enter();
	ret = access_ok(VERIFY_READ, uarg, sizeof(*uarg));
	if (!ret) {
		ret = -EFAULT;
		goto p_err;
	}
	memset(karg, 0, sizeof(*karg));

	ret = get_user(karg->kctx_id, &uarg->kctx_id);
	if (ret)
		goto p_err;
	ret = get_user(karg->task_id, &uarg->task_id);
	if (ret)
		goto p_err;
	ret = get_user(karg->kernel_id, &uarg->kernel_id);
	if (ret)
		goto p_err;

	score_leave();
	return ret;
p_err:
	score_err("Failed to copy from user (%d)\n", ret);
	return ret;
}

static int __score_ioctl_put_request_wait32(
		struct score_ioctl_request *karg,
		struct score_ioctl_request32 __user *uarg)
{
	int ret = 0;
	int idx;

	score_enter();
	ret = access_ok(VERIFY_READ, uarg, sizeof(*uarg));
	if (!ret) {
		ret = -EFAULT;
		goto p_err;
	}

	ret = put_user(karg->ret, &uarg->ret);
	if (ret)
		goto p_err;

	for (idx = 0; idx < 8; ++idx) {
		ret = put_user(karg->timestamp[idx].tv_sec,
				&uarg->timestamp[idx].tv_sec);
		if (ret)
			goto p_err;
		ret = put_user(karg->timestamp[idx].tv_nsec,
				&uarg->timestamp[idx].tv_nsec);
		if (ret)
			goto p_err;
	}

	score_leave();
	return ret;
p_err:
	score_err("Failed to copy to user (%d)\n", ret);
	return ret;
}

static int __score_ioctl_get_secure32(struct score_ioctl_secure *karg,
		struct score_ioctl_secure32 __user *uarg)
{
	int ret = 0;

	score_enter();
	ret = access_ok(VERIFY_READ, uarg, sizeof(*uarg));
	if (!ret) {
		ret = -EFAULT;
		goto p_err;
	}
	memset(karg, 0, sizeof(*karg));

	ret = get_user(karg->cmd, &uarg->cmd);
	if (ret)
		goto p_err;

	score_leave();
	return ret;
p_err:
	score_err("Failed to copy from user (%d)\n", ret);
	return ret;
}

static int __score_ioctl_put_secure32(struct score_ioctl_secure *karg,
		struct score_ioctl_secure32 __user *uarg)
{
	int ret = 0;

	score_enter();
	ret = access_ok(VERIFY_WRITE, uarg, sizeof(*uarg));
	if (!ret) {
		ret = -EFAULT;
		goto p_err;
	}

	ret = put_user(karg->ret, &uarg->ret);
	if (ret)
		goto p_err;

	score_leave();
	return ret;
p_err:
	score_err("Failed to copy to user (%d)\n", ret);
	return ret;
}

static int __score_ioctl_get_test32(struct score_ioctl_test *karg,
		struct score_ioctl_test32 __user *uarg)
{
	int ret = 0;

	score_enter();
	ret = access_ok(VERIFY_READ, uarg, sizeof(*uarg));
	if (!ret) {
		ret = -EFAULT;
		goto p_err;
	}
	memset(karg, 0, sizeof(*karg));

	ret = get_user(karg->cmd, &uarg->cmd);
	if (ret)
		goto p_err;
	ret = get_user(karg->size, &uarg->size);
	if (ret)
		goto p_err;
	ret = get_user(karg->addr, &uarg->addr);
	if (ret)
		goto p_err;

	score_leave();
	return ret;
p_err:
	score_err("Failed to copy from user (%d)\n", ret);
	return ret;
}

static int __score_ioctl_put_test32(struct score_ioctl_test *karg,
		struct score_ioctl_test32 __user *uarg)
{
	int ret = 0;

	score_enter();
	ret = access_ok(VERIFY_WRITE, uarg, sizeof(*uarg));
	if (!ret) {
		ret = -EFAULT;
		goto p_err;
	}

	ret = put_user(karg->ret, &uarg->ret);
	if (ret)
		goto p_err;

	score_leave();
	return ret;
p_err:
	score_err("Failed to copy to user (%d)\n", ret);
	return ret;
}

long score_compat_ioctl32(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	int ret = 0;
	struct score_context *sctx;
	const struct score_ioctl_ops *ops;
	union score_ioctl_arg karg;
	void __user *compat_arg;

	score_enter();
	sctx = file->private_data;
	ops = sctx->core->ioctl_ops;
	compat_arg = compat_ptr(arg);

	switch (cmd) {
	case SCORE_IOC_BOOT32:
		ret = __score_ioctl_get_boot32(&karg.boot, compat_arg);
		if (ret)
			goto p_err;

		ret = ops->score_ioctl_boot(sctx, &karg.boot);
		if (ret)
			goto p_err;

		ret = __score_ioctl_put_boot32(&karg.boot, compat_arg);
		if (ret)
			goto p_err;
		ret = 0;
		break;
	case SCORE_IOC_GET_DEVICE_INFO32:
		ret = 0;
		break;
	case SCORE_IOC_GET_DVFS32:
		ret = __score_ioctl_get_dvfs_info32(&karg.dvfs, compat_arg);
		if (ret)
			goto p_err;

		ret = ops->score_ioctl_get_dvfs(sctx, &karg.dvfs);
		if (ret)
			goto p_err;

		ret = __score_ioctl_put_dvfs_info32(&karg.dvfs, compat_arg);
		if (ret)
			goto p_err;
		break;
	case SCORE_IOC_SET_DVFS32:
		ret = __score_ioctl_get_dvfs_req32(&karg.dvfs, compat_arg);
		if (ret)
			goto p_err;

		ret = ops->score_ioctl_set_dvfs(sctx, &karg.dvfs);
		if (ret)
			goto p_err;

		ret = __score_ioctl_put_dvfs_req32(&karg.dvfs, compat_arg);
		if (ret)
			goto p_err;
		break;
	case SCORE_IOC_REQUEST32:
		ret = __score_ioctl_get_request32(&karg.req, compat_arg);
		if (ret)
			goto p_err;

		ret = ops->score_ioctl_request(sctx, &karg.req);
		if (ret)
			goto p_err;

		ret = __score_ioctl_put_request32(&karg.req, compat_arg);
		if (ret)
			goto p_err;
		break;
	case SCORE_IOC_REQUEST_NONBLOCK32:
		ret = __score_ioctl_get_request_nonblock32(
				&karg.req, compat_arg);
		if (ret)
			goto p_err;

		ret = ops->score_ioctl_request_nonblock(sctx, &karg.req, true);
		if (ret)
			goto p_err;

		ret = __score_ioctl_put_request_nonblock32(
				&karg.req, compat_arg);
		if (ret)
			goto p_err;
		break;
	case SCORE_IOC_REQUEST_WAIT32:
		ret = __score_ioctl_get_request_wait32(&karg.req, compat_arg);
		if (ret)
			goto p_err;

		ret = ops->score_ioctl_request_wait(sctx, &karg.req);
		if (ret)
			goto p_err;

		ret = __score_ioctl_put_request_wait32(&karg.req, compat_arg);
		if (ret)
			goto p_err;
		break;
	case SCORE_IOC_REQUEST_NONBLOCK_NOWAIT32:
		ret = __score_ioctl_get_request_nonblock32(
				&karg.req, compat_arg);
		if (ret)
			goto p_err;

		ret = ops->score_ioctl_request_nonblock(sctx, &karg.req, false);
		if (ret)
			goto p_err;

		ret = __score_ioctl_put_request_nonblock32(
				&karg.req, compat_arg);
		if (ret)
			goto p_err;
		break;
	case SCORE_IOC_SECURE32:
		ret = __score_ioctl_get_secure32(&karg.sec, compat_arg);
		if (ret)
			goto p_err;

		ret = ops->score_ioctl_secure(sctx, &karg.sec);
		if (ret)
			goto p_err;

		ret = __score_ioctl_put_secure32(&karg.sec, compat_arg);
		if (ret)
			goto p_err;
		break;
	case SCORE_IOC_TEST32:
		ret = __score_ioctl_get_test32(&karg.test, compat_arg);
		if (ret)
			goto p_err;

		ret = ops->score_ioctl_test(sctx, &karg.test);
		if (ret)
			goto p_err;

		ret = __score_ioctl_put_test32(&karg.test, compat_arg);
		if (ret)
			goto p_err;
		break;
	default:
		score_err("invalid ioctl command\n");
		ret = -EINVAL;
		goto p_err;
	}

	score_leave();
p_err:
	return ret;
}
