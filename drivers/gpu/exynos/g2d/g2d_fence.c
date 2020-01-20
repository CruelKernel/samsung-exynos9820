/*
 * linux/drivers/gpu/exynos/g2d/g2d_fence.c
 *
 * Copyright (C) 2017 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/dma-fence.h>
#include <linux/sync_file.h>

#include "g2d.h"
#include "g2d_uapi.h"
#include "g2d_task.h"
#include "g2d_fence.h"
#include "g2d_debug.h"

void g2d_fence_timeout_handler(unsigned long arg)
{
	struct g2d_task *task = (struct g2d_task *)arg;
	struct g2d_device *g2d_dev = task->g2d_dev;
	struct dma_fence *fence;
	unsigned long flags;
	char name[32];
	int i;
	s32 afbc = 0;

	for (i = 0; i < task->num_source; i++) {
		fence = task->source[i].fence;
		if (fence) {
			strlcpy(name, fence->ops->get_driver_name(fence),
			       sizeof(name));
			perrfndev(g2d_dev, " SOURCE[%d]:  %s #%d (%s)",
				  i, name, fence->seqno,
				  dma_fence_is_signaled(fence)
					? "signaled" : "active");
		}
	}

	fence = task->target.fence;
	if (fence) {
		strlcpy(name, fence->ops->get_driver_name(fence), sizeof(name));
		perrfn(" TARGET:     %s #%d (%s)", name, fence->seqno,
		       dma_fence_is_signaled(fence) ? "signaled" : "active");
	}

	if (task->release_fence)
		perrfn("   Pending g2d release fence: #%d",
		       task->release_fence->fence->seqno);

	/*
	 * Give up waiting the acquire fences that are not currently signaled
	 * and force pushing this pending task to the H/W to avoid indefinite
	 * wait for the fences to be signaled.
	 * The reference count is required to prevent racing about the
	 * acqure fences between this time handler and the fence callback.
	 */
	spin_lock_irqsave(&task->fence_timeout_lock, flags);

	/*
	 * Make sure if there is really a unsignaled fences. task->starter is
	 * decremented under fence_timeout_lock held if it is done by fence
	 * signal.
	 */
	if (atomic_read(&task->starter.refcount.refs) == 0) {
		spin_unlock_irqrestore(&task->fence_timeout_lock, flags);
		perr("All fences are signaled. (work_busy? %d, state %#lx)",
		     work_busy(&task->work), task->state);
		/*
		 * If this happens, there is racing between
		 * g2d_fence_timeout_handler() and g2d_queuework_task(). Once
		 * g2d_queuework_task() is invoked, it is guaranteed that the
		 * task is to be scheduled to H/W.
		 */
		return;
	}

	perrfn("%d Fence(s) timed out after %d msec.",
	       atomic_read(&task->starter.refcount.refs),
	       G2D_FENCE_TIMEOUT_MSEC);

	/* Increase reference to prevent running the workqueue in callback */
	kref_get(&task->starter);

	spin_unlock_irqrestore(&task->fence_timeout_lock, flags);

	for (i = 0; i < task->num_source; i++) {
		fence = task->source[i].fence;
		if (fence)
			dma_fence_remove_callback(fence,
						  &task->source[i].fence_cb);
	}

	fence = task->target.fence;
	if (fence)
		dma_fence_remove_callback(fence, &task->target.fence_cb);

	/*
	 * Now it is OK to init kref for g2d_start_task() below.
	 * All fences waiters are removed
	 */
	kref_init(&task->starter);

	/* check compressed buffer because crashed buffer makes recovery */
	for (i = 0; i < task->num_source; i++) {
		if (IS_AFBC(
			task->source[i].commands[G2DSFR_IMG_COLORMODE].value))
			afbc |= 1 << i;
	}

	if (IS_AFBC(task->target.commands[G2DSFR_IMG_COLORMODE].value))
		afbc |= 1 << G2D_MAX_IMAGES;

	g2d_stamp_task(task, G2D_STAMP_STATE_TIMEOUT_FENCE, afbc);

	g2d_cancel_task(task);
};

static const char *g2d_fence_get_driver_name(struct dma_fence *fence)
{
	return "g2d";
}

static bool g2d_fence_enable_signaling(struct dma_fence *fence)
{
	/* nothing to do */
	return true;
}

static void g2d_fence_release(struct dma_fence *fence)
{
	kfree(fence);
}

static void g2d_fence_value_str(struct dma_fence *fence, char *str, int size)
{
	snprintf(str, size, "%d", fence->seqno);
}

static struct dma_fence_ops g2d_fence_ops = {
	.get_driver_name =	g2d_fence_get_driver_name,
	.get_timeline_name =	g2d_fence_get_driver_name,
	.enable_signaling =	g2d_fence_enable_signaling,
	.wait =			dma_fence_default_wait,
	.release =		g2d_fence_release,
	.fence_value_str =	g2d_fence_value_str,
};

struct sync_file *g2d_create_release_fence(struct g2d_device *g2d_dev,
					   struct g2d_task *task,
					   struct g2d_task_data *data)
{
	struct dma_fence *fence;
	struct sync_file *file;
	s32 release_fences[g2d_dev->max_layers + 1];
	int i;
	int ret = 0;

	if (!(task->flags & G2D_FLAG_NONBLOCK) || !data->num_release_fences)
		return NULL;

	if (data->num_release_fences > (task->num_source + 1)) {
		perrfndev(g2d_dev,
			  "Too many release fences %d required (src: %d)",
			  data->num_release_fences, task->num_source);
		return ERR_PTR(-EINVAL);
	}

	fence = kzalloc(sizeof(*fence), GFP_KERNEL);
	if (!fence)
		return ERR_PTR(-ENOMEM);

	dma_fence_init(fence, &g2d_fence_ops, &g2d_dev->fence_lock,
		   g2d_dev->fence_context,
		   atomic_inc_return(&g2d_dev->fence_timeline));

	file = sync_file_create(fence);
	dma_fence_put(fence);
	if (!file)
		return ERR_PTR(-ENOMEM);

	for (i = 0; i < data->num_release_fences; i++) {
		release_fences[i] = get_unused_fd_flags(O_CLOEXEC);
		if (release_fences[i] < 0) {
			ret = release_fences[i];
			while (i-- > 0)
				put_unused_fd(release_fences[i]);
			goto err_fd;
		}
	}

	if (copy_to_user(data->release_fences, release_fences,
				sizeof(u32) * data->num_release_fences)) {
		ret = -EFAULT;
		perrfndev(g2d_dev, "Failed to copy release fences to user");
		goto err_fd;
	}

	for (i = 0; i < data->num_release_fences; i++)
		fd_install(release_fences[i], get_file(file->file));

	return file;
err_fd:
	while (i-- > 0)
		put_unused_fd(release_fences[i]);
	fput(file->file);

	return ERR_PTR(ret);
}

struct dma_fence *g2d_get_acquire_fence(struct g2d_device *g2d_dev,
					struct g2d_layer *layer, s32 fence_fd)
{
	struct dma_fence *fence;
	int ret;

	if (!(layer->flags & G2D_LAYERFLAG_ACQUIRE_FENCE))
		return NULL;

	fence = sync_file_get_fence(fence_fd);
	if (!fence)
		return ERR_PTR(-EINVAL);

	kref_get(&layer->task->starter);

	ret = dma_fence_add_callback(fence, &layer->fence_cb, g2d_fence_callback);
	if (ret < 0) {
		kref_put(&layer->task->starter, g2d_queuework_task);
		dma_fence_put(fence);
		return (ret == -ENOENT) ? NULL : ERR_PTR(ret);
	}

	return fence;
}

static bool g2d_fence_has_error(struct g2d_layer *layer, int layer_idx)
{
	struct dma_fence *fence = layer->fence;
	unsigned long flags;
	bool err = false;

	if (fence) {
		spin_lock_irqsave(fence->lock, flags);
		err = dma_fence_get_status_locked(fence) < 0;
		spin_unlock_irqrestore(fence->lock, flags);
	}

	if (err) {
		char name[32];

		strlcpy(name, fence->ops->get_driver_name(fence), sizeof(name));

		dev_err(layer->task->g2d_dev->dev,
			"%s: Error fence of %s%d found: %s#%d\n", __func__,
			(layer_idx < 0) ? "target" : "source",
			layer_idx, name, fence->seqno);

		return true;
	}

	return false;
}

bool g2d_task_has_error_fence(struct g2d_task *task)
{
	unsigned int i;

	for (i = 0; i < task->num_source; i++)
		if (g2d_fence_has_error(&task->source[i], i))
			return false;

	if (g2d_fence_has_error(&task->target, -1))
		return true;

	return false;
}
