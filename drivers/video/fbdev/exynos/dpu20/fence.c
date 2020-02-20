/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * DPU fence file for Samsung EXYNOS DPU driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#if !defined(CONFIG_SUPPORT_LEGACY_FENCE)
#include <linux/dma-fence.h>
#endif
#include <linux/sync_file.h>

#include "decon.h"

#if defined(CONFIG_SUPPORT_LEGACY_FENCE)
/* sync fence related functions */
void decon_create_timeline(struct decon_device *decon, char *name)
{
	decon->timeline = sync_timeline_create(name);
	decon->timeline_max = 0;
}

int decon_get_valid_fd(void)
{
	int fd = 0;
	int fd_idx = 0;
	int unused_fd[FD_TRY_CNT] = {0};

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0)
		return -EINVAL;

	if (fd < VALID_FD_VAL) {
		/*
		 * If fd from get_unused_fd() has value between 0 and 2,
		 * fd is tried to get value again except current fd vlaue.
		 */
		while (fd < VALID_FD_VAL) {
			decon_warn("%s, unvalid fd[%d] is assigned to DECON\n",
					__func__, fd);
			unused_fd[fd_idx++] = fd;
			fd = get_unused_fd_flags(O_CLOEXEC);
			if (fd < 0) {
				decon_err("%s, unvalid fd[%d]\n", __func__,
						fd);
				break;
			}
		}

		while (fd_idx-- > 0) {
			decon_warn("%s, unvalid fd[%d] is released by DECON\n",
					__func__, unused_fd[fd_idx]);
			put_unused_fd(unused_fd[fd_idx]);
		}

		if (fd < 0)
			return -EINVAL;
	}
	return fd;
}

void decon_create_release_fences(struct decon_device *decon,
		struct decon_win_config_data *win_data,
		struct sync_file *sync_file)
{
	int i = 0;

	for (i = 0; i < decon->dt.max_win; i++) {
		int state = win_data->config[i].state;
		int rel_fence = -1;

		if (state == DECON_WIN_STATE_BUFFER) {
			rel_fence = decon_get_valid_fd();
			if (rel_fence < 0) {
				decon_err("%s: failed to get unused fd\n",
						__func__);
				goto err;
			}

			fd_install(rel_fence,
					get_file(sync_file->file));
		}
		win_data->config[i].rel_fence = rel_fence;
	}
	return;
err:
	while (i-- > 0) {
		if (win_data->config[i].state == DECON_WIN_STATE_BUFFER) {
			put_unused_fd(win_data->config[i].rel_fence);
			win_data->config[i].rel_fence = -1;
		}
	}
	return;
}

int decon_create_fence(struct decon_device *decon, struct sync_file **sync_file)
{
	struct sync_pt *pt;
	int fd = -EMFILE;

	decon->timeline_max++;
	pt = sync_pt_create(decon->timeline, sizeof(*pt), decon->timeline_max);
	if (!pt) {
		decon_err("%s: failed to create sync pt\n", __func__);
		goto err;
	}

	*sync_file = sync_file_create(&pt->base);
	fence_put(&pt->base);
	if (!(*sync_file)) {
		decon_err("%s: failed to create sync file\n", __func__);
		goto err;
	}

	fd = decon_get_valid_fd();
	if (fd < 0) {
		decon_err("%s: failed to get unused fd\n", __func__);
		fput((*sync_file)->file);
		goto err;
	}

	return fd;

err:
	decon->timeline_max--;
	return fd;
}

int decon_wait_fence(struct decon_device *decon, struct sync_file *sync_file,
		int fd)
{
	int err = sync_file_wait(sync_file, msecs_to_jiffies(600));
	if (err >= 0)
		return 0;

	if (err < 0) {
		decon_warn("error waiting on acquire fence: %d\n", err);
		return err;
	}
}

void decon_signal_fence(struct decon_device *decon)
{
	sync_timeline_signal(decon->timeline, 1);
}
#else	/* dma fence in kernel version 4.14 */

static char *fence_evt[] = {
	"CREATE_RETIRE_FENCE",
	"CREATE_RELEASE_FENCE_FDS",
	"WAIT_ACQUIRE_FENCE",
	"SIGNAL_RETIRE_FENCE",
};

/* sync fence related functions */
void decon_create_timeline(struct decon_device *decon, char *name)
{
	decon->fence.context = dma_fence_context_alloc(1);
	spin_lock_init(&decon->fence.lock);
	strlcpy(decon->fence.name, name, sizeof(decon->fence.name));
}

static int decon_get_valid_fd(void)
{
	int fd = 0;
	int fd_idx = 0;
	int unused_fd[FD_TRY_CNT] = {0};

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0)
		return -EINVAL;

	if (fd < VALID_FD_VAL) {
		/*
		 * If fd from get_unused_fd() has value between 0 and 2,
		 * fd is tried to get value again except current fd vlaue.
		 */
		while (fd < VALID_FD_VAL) {
			decon_warn("%s, unvalid fd[%d] is assigned to DECON\n",
					__func__, fd);
			unused_fd[fd_idx++] = fd;
			fd = get_unused_fd_flags(O_CLOEXEC);
			if (fd < 0) {
				decon_err("%s, unvalid fd[%d]\n", __func__,
						fd);
				break;
			}
		}

		while (fd_idx-- > 0) {
			decon_warn("%s, unvalid fd[%d] is released by DECON\n",
					__func__, unused_fd[fd_idx]);
			put_unused_fd(unused_fd[fd_idx]);
		}

		if (fd < 0)
			return -EINVAL;
	}
	return fd;
}

void decon_create_release_fences(struct decon_device *decon,
		struct decon_win_config_data *win_data,
		struct sync_file *sync_file)
{
	int i = 0;
	struct dpu_fence_info release;
	struct dma_fence *fence = sync_file->fence;

	for (i = 0; i < decon->dt.max_win; i++) {
		int state = win_data->config[i].state;
		int rel_fence = -1;

		if (state == DECON_WIN_STATE_BUFFER) {
			rel_fence = decon_get_valid_fd();
			if (rel_fence < 0) {
				decon_err("%s: failed to get unused fd\n",
						__func__);
				goto err;
			}

			fd_install(rel_fence, get_file(sync_file->file));

			dpu_save_fence_info(rel_fence, fence, &release);
			DPU_F_EVT_LOG(DPU_F_EVT_CREATE_RELEASE_FENCE_FDS,
					&decon->sd, &release);
			DPU_DEBUG_FENCE("[%s] %s: seqno(%d), fd(%d), flags(0x%x)\n",
					fence_evt[DPU_F_EVT_CREATE_RELEASE_FENCE_FDS],
					release.name, release.seqno, release.fd,
					release.flags);
		}
		win_data->config[i].rel_fence = rel_fence;
	}
	return;
err:
	while (i-- > 0) {
		if (win_data->config[i].state == DECON_WIN_STATE_BUFFER) {
			put_unused_fd(win_data->config[i].rel_fence);
			win_data->config[i].rel_fence = -1;
		}
	}
	return;
}

static const char *decon_fence_get_driver_name(struct dma_fence *fence)
{
	struct decon_fence *decon_fence;

	decon_fence = container_of(fence->lock, struct decon_fence, lock);
	return decon_fence->name;
}

static bool decon_fence_enable_signaling(struct dma_fence *fence)
{
	/* nothing to do */
	return true;
}

static void decon_fence_value_str(struct dma_fence *fence, char *str, int size)
{
	snprintf(str, size, "%d", fence->seqno);
}

static struct dma_fence_ops decon_fence_ops = {
	.get_driver_name =	decon_fence_get_driver_name,
	.get_timeline_name =	decon_fence_get_driver_name,
	.enable_signaling =	decon_fence_enable_signaling,
	.wait =			dma_fence_default_wait,
	.fence_value_str =	decon_fence_value_str,
};

int decon_create_fence(struct decon_device *decon, struct sync_file **sync_file)
{
	struct dma_fence *fence;
	struct dpu_fence_info retire;
	int fd = -EMFILE;

	fence = kzalloc(sizeof(*fence), GFP_KERNEL);
	if (!fence)
		return -ENOMEM;

	dma_fence_init(fence, &decon_fence_ops, &decon->fence.lock,
		   decon->fence.context,
		   atomic_inc_return(&decon->fence.timeline));

	*sync_file = sync_file_create(fence);
	dma_fence_put(fence);
	if (!(*sync_file)) {
		decon_err("%s: failed to create sync file\n", __func__);
		return -ENOMEM;
	}

	fd = decon_get_valid_fd();
	if (fd < 0) {
		decon_err("%s: failed to get unused fd\n", __func__);
		fput((*sync_file)->file);
	}

	dpu_save_fence_info(fd, fence, &retire);
	DPU_F_EVT_LOG(DPU_F_EVT_CREATE_RETIRE_FENCE, &decon->sd, &retire);
	DPU_DEBUG_FENCE("[%s] %s: ctx(%llu) seqno(%d), fd(%d), flags(0x%lx)\n",
			fence_evt[DPU_F_EVT_CREATE_RETIRE_FENCE], retire.name,
			retire.context, retire.seqno, retire.fd, retire.flags);

	return fd;
}

#define FENCE_WARN_TIMEOUT_MS		(50)
int decon_wait_fence(struct decon_device *decon, struct dma_fence *fence, int fd)
{
	/* Positive values(ret, fence_err, err) mean normal or success */
	int err = 1;
	int fence_err = 1;
	int ret = 1;
	struct dpu_fence_info acquire;
	ktime_t s_time = ktime_get(), e_time;

	dpu_save_fence_info(fd, fence, &acquire);
	DPU_F_EVT_LOG(DPU_F_EVT_WAIT_ACQUIRE_FENCE, &decon->sd, &acquire);
	DPU_DEBUG_FENCE("[%s] %s: ctx(%llu), seqno(%d), fd(%d), flags(0x%lx)\n",
		fence_evt[DPU_F_EVT_WAIT_ACQUIRE_FENCE], acquire.name,
		acquire.context, acquire.seqno, acquire.fd, acquire.flags);
	err = dma_fence_wait_timeout(fence, false, msecs_to_jiffies(600));

	if (err <= 0) {
		decon_err("%s: waiting on acquire fence timeout\n", __func__);
		ret = err;
	}

	e_time = ktime_get();
	if (ktime_after(e_time, ktime_add_ms(s_time, FENCE_WARN_TIMEOUT_MS))) {
		decon_warn("%s: waiting on acquire fence (elapsed %lld msec)\n",
				__func__, ktime_to_ms(ktime_sub(e_time, s_time)));
	}

	/*
	 * If acquire fence has error value, it means image on buffer is corrupted.
	 * So, if this function returns error value, frame will be dropped and
	 * previous buffer is released.
	 *
	 * However, in case of video mode, if previous buffer is released and current
	 * buffer is not used, sysmmu fault can occurs
	 */
	if (decon->dt.psr_mode == DECON_MIPI_COMMAND_MODE) {
		fence_err = dma_fence_get_status(fence);
		if (fence_err <= 0) {
			decon_err("%s: get acquire fence error status\n",
					__func__);
			ret = fence_err;
		}
	}

	if ((err <= 0) || (fence_err <= 0) ||
			(ktime_after(e_time, ktime_add_ms(s_time, FENCE_WARN_TIMEOUT_MS)))) {
		decon_err("\t%s: ctx(%llu), seqno(%d), fd(%d), flags(0x%lx), err(%d:%d), remain_frame(%d)\n",
				acquire.name, acquire.context, acquire.seqno,
				acquire.fd, acquire.flags, err, fence_err,
				atomic_read(&decon->up.remaining_frame));
	}

	return ret;
}

void decon_signal_fence(struct decon_device *decon, struct dma_fence *fence)
{
	struct dpu_fence_info retire;
	int ret = 0;

	ret = dma_fence_signal(fence);
	if (ret < 0)
		decon_warn("failed to signal fence. err(%d)\n", ret);

	if (!fence) {
		decon_err("%s: fence is NULL\n", __func__);
		return;
	}

	dpu_save_fence_info(0, fence, &retire);
	DPU_F_EVT_LOG(DPU_F_EVT_SIGNAL_RETIRE_FENCE, &decon->sd, &retire);
	DPU_DEBUG_FENCE("[%s] %s: ctx(%llu), seqno(%d), flags(0x%lx)\n",
			fence_evt[DPU_F_EVT_SIGNAL_RETIRE_FENCE], retire.name,
			retire.context, retire.seqno, retire.flags);
}
#endif
