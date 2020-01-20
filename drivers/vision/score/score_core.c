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

#include <linux/slab.h>
#include <linux/uaccess.h>

#include "score_log.h"
#include "score_util.h"
#include "score_ioctl.h"
#include "score_device.h"
#include "score_mmu.h"
#include "score_context.h"
#include "score_core.h"

static struct score_dev *score_dev;

static int score_ioctl_boot(struct score_context *sctx,
		struct score_ioctl_boot *boot)
{
	int ret = 0;
	struct score_core *core;

	score_enter();
	core = sctx->core;

	if (mutex_lock_interruptible(&core->device_lock)) {
		score_err("device_lock failed\n");
		ret = -ERESTARTSYS;
		goto p_err;
	}

	ret = score_device_open(core->device);
	if (ret)
		goto p_err_open;

	ret = score_device_start(core->device, boot->firmware_id,
			boot->pm_level, boot->flag);
	if (ret)
		goto p_err_start;

	mutex_unlock(&core->device_lock);
	boot->ret = 0;

	score_leave();
	return 0;
p_err_start:
p_err_open:
	mutex_unlock(&core->device_lock);
p_err:
	boot->ret = ret;
	return 0;
}

static int score_ioctl_get_dvfs(struct score_context *sctx,
		struct score_ioctl_dvfs *dvfs)
{
	int ret = 0;
	struct score_pm *pm;

	score_enter();
	pm = &sctx->core->device->pm;
	ret = score_pm_qos_get_info(pm,
			&dvfs->qos_count, &dvfs->min_qos, &dvfs->max_qos,
			&dvfs->default_qos, &dvfs->current_qos);
	if (ret)
		score_warn("runtime pm is not supported (%d)\n", ret);

	score_leave();
	dvfs->ret = ret;
	return 0;
}

static int score_ioctl_set_dvfs(struct score_context *sctx,
		struct score_ioctl_dvfs *dvfs)
{
	int ret = 0;
	struct score_pm *pm;
	struct score_ioctl_dvfs cur_dvfs;

	score_enter();
	pm = &sctx->core->device->pm;
	ret = score_pm_qos_update(pm, dvfs->request_qos);
	if (ret)
		goto p_err;

	ret = score_pm_qos_get_info(pm,
			&cur_dvfs.qos_count, &cur_dvfs.min_qos,
			&cur_dvfs.max_qos, &cur_dvfs.default_qos,
			&cur_dvfs.current_qos);
	if (ret) {
		score_warn("runtime pm is not supported (%d)\n", ret);
		goto p_err;
	}

	dvfs->current_qos = cur_dvfs.current_qos;
	score_leave();
p_err:
	dvfs->ret = ret;
	return 0;
}

static int score_ioctl_request(struct score_context *sctx,
		struct score_ioctl_request *req)
{
	int ret = 0;
	struct score_core *core;
	struct score_system *system;
	struct score_frame_manager *framemgr;
	struct score_frame *frame;
	struct score_mmu_packet packet;

	score_enter();
	core = sctx->core;
	if (mutex_lock_interruptible(&core->device_lock)) {
		score_err("device_lock failed\n");
		req->ret = -ERESTARTSYS;
		goto p_err;
	}
	ret = score_device_check_start(core->device);
	mutex_unlock(&core->device_lock);
	if (ret) {
		score_err("The device is not working (state:%#x)\n", ret);
		req->ret = -ENOSTR;
		goto p_err;
	}

	system = sctx->system;
	framemgr = &system->interface.framemgr;
	frame = score_frame_create(framemgr, sctx, TYPE_BLOCK);
	if (IS_ERR(frame)) {
		req->ret = PTR_ERR(frame);
		goto p_err;
	}
	req->kctx_id = sctx->id;
	req->task_id = frame->frame_id;
	frame->kernel_id = req->kernel_id;

	packet.fd = req->packet_fd;
	ret = score_mmu_packet_prepare(&system->mmu, &packet);
	if (ret) {
		req->ret = ret;
		goto p_err_prepare;
	}
	frame->packet = packet.kvaddr;
	frame->packet_size = packet.size;

	ret = sctx->ctx_ops->queue(sctx, frame);
	if (ret)
		goto p_err_queue;

	ret = sctx->ctx_ops->deque(sctx, frame);
	if (ret)
		goto p_err_queue;

	score_mmu_packet_unprepare(&system->mmu, &packet);
	score_frame_done(frame, &req->ret);
	req->timestamp[SCORE_TIME_SCQ_WRITE] =
		frame->timestamp[SCORE_TIME_SCQ_WRITE];
	req->timestamp[SCORE_TIME_ISR] =
		frame->timestamp[SCORE_TIME_ISR];

	score_frame_destroy(frame);

	score_leave();
	return 0;
p_err_queue:
	score_mmu_packet_unprepare(&system->mmu, &packet);
	score_frame_done(frame, &req->ret);
p_err_prepare:
	score_frame_destroy(frame);
p_err:
	/* return value is included in req->ret */
	return 0;
}

#if defined(CONFIG_EXYNOS_SCORE_FPGA)
static int score_ioctl_request_nonblock(struct score_context *sctx,
		struct score_ioctl_request *req, bool wait)
{
	score_enter();
	req->ret = -EINVAL;
	score_err("Not supported [%s]\n", __func__);
	score_leave();
	return 0;
}

static int score_ioctl_request_wait(struct score_context *sctx,
		struct score_ioctl_request *req)
{
	score_enter();
	req->ret = -EINVAL;
	score_err("Not supported [%s]\n", __func__);
	score_leave();
	return 0;
}
#else
static int score_ioctl_request_nonblock(struct score_context *sctx,
		struct score_ioctl_request *req, bool wait)
{
	int ret = 0;
	struct score_core *core;
	struct score_system *system;
	struct score_frame_manager *framemgr;
	struct score_frame *frame;
	struct score_mmu_packet packet;
	int frame_type;
	unsigned long flags;

	score_enter();
	core = sctx->core;
	if (mutex_lock_interruptible(&core->device_lock)) {
		score_err("device_lock failed\n");
		ret = -ERESTARTSYS;
		goto p_err;
	}
	ret = score_device_check_start(core->device);
	mutex_unlock(&core->device_lock);
	if (ret) {
		score_err("The device is not working (state:%#x)\n", ret);
		ret = -ENOSTR;
		goto p_err;
	}

	system = sctx->system;
	framemgr = &system->interface.framemgr;
	if (wait)
		frame_type = TYPE_NONBLOCK;
	else
		frame_type = TYPE_NONBLOCK_NOWAIT;

	frame = score_frame_create(framemgr, sctx, frame_type);
	if (IS_ERR(frame)) {
		ret = PTR_ERR(frame);
		goto p_err;
	}
	req->kctx_id = sctx->id;
	req->task_id = frame->frame_id;
	frame->kernel_id = req->kernel_id;

	packet.fd = req->packet_fd;
	ret = score_mmu_packet_prepare(&system->mmu, &packet);
	if (ret)
		goto p_err_prepare;

	frame->packet = packet.kvaddr;
	frame->packet_size = packet.size;

	ret = sctx->ctx_ops->queue(sctx, frame);
	if (ret)
		goto p_err_queue;

	score_mmu_packet_unprepare(&system->mmu, &packet);

	req->ret = 0;
	score_leave();
	return 0;
p_err_queue:
	score_mmu_packet_unprepare(&system->mmu, &packet);
p_err_prepare:
	spin_lock_irqsave(&framemgr->slock, flags);
	if (score_frame_check_type(frame, TYPE_NONBLOCK_NOWAIT) ||
			score_frame_check_type(frame, TYPE_NONBLOCK))
		score_frame_set_type_remove(frame);
	spin_unlock_irqrestore(&framemgr->slock, flags);

	if (score_frame_check_type(frame, TYPE_NONBLOCK_REMOVE))
		score_frame_destroy(frame);
p_err:
	req->ret = ret;
	return 0;
}

static int score_ioctl_request_wait(struct score_context *sctx,
		struct score_ioctl_request *req)
{
	int ret = 0;
	struct score_frame_manager *framemgr;
	struct score_frame *frame;
	unsigned long flags;

	score_enter();
	framemgr = &sctx->system->interface.framemgr;
	spin_lock_irqsave(&framemgr->slock, flags);
	frame = score_frame_get_by_id(framemgr, req->task_id);
	if (!frame) {
		spin_unlock_irqrestore(&framemgr->slock, flags);
		req->ret = -EINVAL;
		score_warn("frame is already completed (%u,%u)\n",
				req->task_id, sctx->id);
		goto p_err_frame;
	} else {
		if (score_frame_check_type(frame, TYPE_NONBLOCK)) {
			score_frame_set_type_block(frame);
		} else {
			spin_unlock_irqrestore(&framemgr->slock, flags);
			req->ret = -EINVAL;
			score_warn("frame isn't nonblock (%d,%u,%u,%u)\n",
					frame->type, frame->sctx->id,
					frame->frame_id, frame->kernel_id);
			goto p_err_frame;
		}
	}
	spin_unlock_irqrestore(&framemgr->slock, flags);

	ret = sctx->ctx_ops->deque(sctx, frame);
	if (ret)
		goto p_err_deque;

	score_frame_done(frame, &req->ret);
	req->timestamp[SCORE_TIME_SCQ_WRITE] =
		frame->timestamp[SCORE_TIME_SCQ_WRITE];
	req->timestamp[SCORE_TIME_ISR] =
		frame->timestamp[SCORE_TIME_ISR];

	score_frame_destroy(frame);

	score_leave();
	return 0;
p_err_deque:
	score_frame_done(frame, &req->ret);
	score_frame_destroy(frame);
p_err_frame:
	return 0;
}
#endif

static int score_ioctl_secure(struct score_context *sctx,
		struct score_ioctl_secure *sec)
{
	/* for secure mode change */
	return 0;
}

static int score_ioctl_test(struct score_context *sctx,
		struct score_ioctl_test *info)
{
	/* verification */
	return 0;
}

const struct score_ioctl_ops score_ioctl_ops = {
	.score_ioctl_boot		= score_ioctl_boot,
	.score_ioctl_get_dvfs		= score_ioctl_get_dvfs,
	.score_ioctl_set_dvfs		= score_ioctl_set_dvfs,
	.score_ioctl_request		= score_ioctl_request,
	.score_ioctl_request_nonblock	= score_ioctl_request_nonblock,
	.score_ioctl_request_wait	= score_ioctl_request_wait,
	.score_ioctl_secure		= score_ioctl_secure,
	.score_ioctl_test		= score_ioctl_test
};

static int score_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct miscdevice *miscdev;
	struct device *dev;
	struct score_device *device;
	struct score_core *core;
	struct score_context *sctx;

	score_enter();
	miscdev = file->private_data;
	dev = miscdev->parent;
	device = dev_get_drvdata(dev);
	core = &device->core;

	if (mutex_lock_interruptible(&core->device_lock)) {
		score_err("device_lock failed\n");
		ret = -ERESTARTSYS;
		goto p_err;
	}
	score_device_get(device);
	mutex_unlock(&core->device_lock);

	sctx = score_context_create(core);
	if (IS_ERR(sctx)) {
		ret = PTR_ERR(sctx);
		goto p_err_create_context;
	}
	file->private_data = sctx;

	score_leave();
	return ret;
p_err_create_context:
	mutex_lock(&core->device_lock);
	score_device_put(device);
	mutex_unlock(&core->device_lock);
p_err:
	return ret;
}

static int score_release(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct score_context *sctx;
	struct score_core *core;

	score_enter();
	sctx = file->private_data;
	core = sctx->core;

	score_context_destroy(sctx);

	mutex_lock(&core->device_lock);
	score_device_put(core->device);
	mutex_unlock(&core->device_lock);

	score_leave();
	return ret;
}

static const struct file_operations score_file_ops = {
	.owner		= THIS_MODULE,
	.open		= score_open,
	.release	= score_release,
	.unlocked_ioctl = score_ioctl,
	.compat_ioctl	= score_compat_ioctl32,
};

int score_core_probe(struct score_device *device)
{
	int ret = 0;
	struct score_core *core;

	score_enter();
	core = &device->core;
	score_dev = &core->sdev;
	core->device = device;
	core->system = &device->system;

	INIT_LIST_HEAD(&core->ctx_list);
	core->ctx_count = 0;
	spin_lock_init(&core->ctx_slock);
#if defined(CONFIG_EXYNOS_SCORE_FPGA)
	core->wait_time = 50000;
#else
	core->wait_time = 10000;
#endif
	score_util_bitmap_init(core->ctx_map, SCORE_MAX_CONTEXT);
	mutex_init(&core->device_lock);
	core->ioctl_ops = &score_ioctl_ops;
	core->dpmu_print = false;

	score_dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	score_dev->miscdev.name = "score";
	score_dev->miscdev.fops = &score_file_ops;
	score_dev->miscdev.parent = device->dev;

	ret = misc_register(&score_dev->miscdev);
	if (ret)
		score_err("miscdevice is not registered (%d)\n", ret);

	score_leave();
	return ret;
}

void score_core_remove(struct score_core *core)
{
	score_enter();
	mutex_destroy(&core->device_lock);
	misc_deregister(&core->sdev.miscdev);
	score_leave();
}
