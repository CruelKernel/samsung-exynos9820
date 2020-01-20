/*
 * drivers/media/m2m1shot.c
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Cho KyongHo <pullip.cho@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/dma-buf.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>

#include <media/m2m1shot.h>

#define M2M1SHOT_DEVNODE_PREFIX "m2m1shot_"
#define M2M1SHOT_DEVNODE_PREFIX_LEN 9

static void m2m1shot_task_schedule(struct m2m1shot_device *m21dev)
{
	struct m2m1shot_task *task;
	unsigned long flags;

next_task:
	spin_lock_irqsave(&m21dev->lock_task, flags);
	if (list_empty(&m21dev->tasks)) {
		/* No task to run */
		spin_unlock_irqrestore(&m21dev->lock_task, flags);
		return;
	}

	if (m21dev->current_task) {
		/* H/W is working */
		spin_unlock_irqrestore(&m21dev->lock_task, flags);
		return;
	}

	task = list_first_entry(&m21dev->tasks,
			struct m2m1shot_task, task_node);
	list_del(&task->task_node);

	m21dev->current_task = task;

	spin_unlock_irqrestore(&m21dev->lock_task, flags);

	task->state = M2M1SHOT_BUFSTATE_PROCESSING;

	if (m21dev->ops->device_run(task->ctx, task)) {
		task->state = M2M1SHOT_BUFSTATE_ERROR;

		spin_lock_irqsave(&m21dev->lock_task, flags);
		m21dev->current_task = NULL;
		spin_unlock_irqrestore(&m21dev->lock_task, flags);

		complete(&task->complete);

		goto next_task;
	}
}

void m2m1shot_task_finish(struct m2m1shot_device *m21dev,
				struct m2m1shot_task *task, bool success)
{
	unsigned long flags;

	spin_lock_irqsave(&m21dev->lock_task, flags);
	BUG_ON(!m21dev->current_task);
	BUG_ON(m21dev->current_task != task);
	m21dev->current_task = NULL;
	spin_unlock_irqrestore(&m21dev->lock_task, flags);

	task->state = success ?
			M2M1SHOT_BUFSTATE_DONE : M2M1SHOT_BUFSTATE_ERROR;

	complete(&task->complete);

	m2m1shot_task_schedule(m21dev);
}
EXPORT_SYMBOL(m2m1shot_task_finish);

void m2m1shot_task_cancel(struct m2m1shot_device *m21dev,
				struct m2m1shot_task *task,
				enum m2m1shot_state reason)
{
	unsigned long flags;

	spin_lock_irqsave(&m21dev->lock_task, flags);
	m21dev->current_task = NULL;
	spin_unlock_irqrestore(&m21dev->lock_task, flags);

	task->state = reason;

	complete(&task->complete);

	m2m1shot_task_schedule(m21dev);
}
EXPORT_SYMBOL(m2m1shot_task_cancel);

static void m2m1shot_buffer_put_dma_buf_plane(
				struct m2m1shot_buffer_plane_dma *plane)
{
	dma_buf_detach(plane->dmabuf, plane->attachment);
	dma_buf_put(plane->dmabuf);
	plane->dmabuf = NULL;
}

static void m2m1shot_buffer_put_dma_buf(struct m2m1shot_buffer *buffer,
					struct m2m1shot_buffer_dma *dma_buffer)
{
	int i;

	for (i = 0; i < buffer->num_planes; i++)
		m2m1shot_buffer_put_dma_buf_plane(&dma_buffer->plane[i]);
}

static int m2m1shot_buffer_get_dma_buf(struct m2m1shot_device *m21dev,
					struct m2m1shot_buffer *buffer,
					struct m2m1shot_buffer_dma *dma_buffer)
{
	struct m2m1shot_buffer_plane_dma *plane;
	int i, ret;

	for (i = 0; i < buffer->num_planes; i++) {
		plane = &dma_buffer->plane[i];

		plane->dmabuf = dma_buf_get(buffer->plane[i].fd);
		if (IS_ERR(plane->dmabuf)) {
			dev_err(m21dev->dev,
				"%s: failed to get dmabuf of fd %d\n",
				__func__, buffer->plane[i].fd);
			ret = PTR_ERR(plane->dmabuf);
			goto err;
		}

		if (plane->dmabuf->size < plane->bytes_used) {
			dev_err(m21dev->dev,
				"%s: needs %zx bytes but dmabuf is %zx\n",
				__func__, plane->bytes_used,
				plane->dmabuf->size);
			ret = -EINVAL;
			goto err;
		}

		plane->attachment = dma_buf_attach(plane->dmabuf, m21dev->dev);
		if (IS_ERR(plane->attachment)) {
			dev_err(m21dev->dev,
				"%s: Failed to attach dmabuf\n", __func__);
			ret = PTR_ERR(plane->attachment);
			goto err;
		}
	}

	return 0;
err:
	if (!IS_ERR(plane->dmabuf)) /* release dmabuf of the last iteration */
		dma_buf_put(plane->dmabuf);

	while (i-- > 0)
		m2m1shot_buffer_put_dma_buf_plane(&dma_buffer->plane[i]);

	return ret;
}

static int m2m1shot_prepare_get_buffer(struct m2m1shot_context *ctx,
					struct m2m1shot_buffer *buffer,
					struct m2m1shot_buffer_dma *dma_buffer,
					enum dma_data_direction dir)
{
	struct m2m1shot_device *m21dev = ctx->m21dev;
	int i, ret;

	for (i = 0; i < buffer->num_planes; i++) {
		struct m2m1shot_buffer_plane_dma *plane;

		plane = &dma_buffer->plane[i];

		if (plane->bytes_used == 0) {
			/*
			 * bytes_used = 0 means that the size of the plane is
			 * not able to be decided by the driver because it is
			 * dependent upon the content in the buffer.
			 * The best example of the buffer is the buffer of JPEG
			 * encoded stream for decompression.
			 */
			plane->bytes_used = buffer->plane[i].len;
		} else if (buffer->plane[i].len < plane->bytes_used) {
			dev_err(m21dev->dev,
				"%s: needs %zx bytes but %zx is given\n",
				__func__, plane->bytes_used,
				buffer->plane[i].len);
			return -EINVAL;
		}
	}

	if ((buffer->type != M2M1SHOT_BUFFER_USERPTR) &&
				(buffer->type != M2M1SHOT_BUFFER_DMABUF)) {
		dev_err(m21dev->dev, "%s: unknown buffer type %u\n",
			__func__, buffer->type);
		return -EINVAL;
	}

	if (buffer->type == M2M1SHOT_BUFFER_DMABUF) {
		ret = m2m1shot_buffer_get_dma_buf(m21dev, buffer, dma_buffer);
		if (ret)
			return ret;
	}

	dma_buffer->buffer = buffer;

	for (i = 0; i < buffer->num_planes; i++) {
		/* the callback function should fill 'dma_addr' field */
		ret = m21dev->ops->prepare_buffer(ctx, dma_buffer, i, dir);
		if (ret)
			goto err;
	}

	return 0;
err:
	dev_err(m21dev->dev, "%s: Failed to prepare plane %u", __func__, i);

	while (i-- > 0)
		m21dev->ops->finish_buffer(ctx, dma_buffer, i, dir);

	if (buffer->type == M2M1SHOT_BUFFER_DMABUF)
		m2m1shot_buffer_put_dma_buf(buffer, dma_buffer);

	return ret;
}

static void m2m1shot_finish_buffer(struct m2m1shot_device *m21dev,
				struct m2m1shot_context *ctx,
				struct m2m1shot_buffer *buffer,
				struct m2m1shot_buffer_dma *dma_buffer,
				enum dma_data_direction dir)
{
	int i;

	for (i = 0; i < buffer->num_planes; i++)
		m21dev->ops->finish_buffer(ctx, dma_buffer, i, dir);

	if (buffer->type == M2M1SHOT_BUFFER_DMABUF)
		m2m1shot_buffer_put_dma_buf(buffer, dma_buffer);
}

static int m2m1shot_prepare_format(struct m2m1shot_device *m21dev,
				struct m2m1shot_context *ctx,
				struct m2m1shot_task *task)
{
	int i, ret;
	size_t out_sizes[M2M1SHOT_MAX_PLANES] = { 0 };
	size_t cap_sizes[M2M1SHOT_MAX_PLANES] = { 0 };

	if (task->task.buf_out.num_planes > M2M1SHOT_MAX_PLANES) {
		dev_err(m21dev->dev, "Invalid number of output planes %u.\n",
			task->task.buf_out.num_planes);
		return -EINVAL;
	}

	if (task->task.buf_cap.num_planes > M2M1SHOT_MAX_PLANES) {
		dev_err(m21dev->dev, "Invalid number of capture planes %u.\n",
			task->task.buf_cap.num_planes);
		return -EINVAL;
	}

	ret = m21dev->ops->prepare_format(ctx, &task->task.fmt_out,
					DMA_TO_DEVICE, out_sizes);
	if (ret < 0)
		return ret;

	if (task->task.buf_out.num_planes != ret) {
		dev_err(m21dev->dev,
			"%s: needs %u output planes but %u is given\n",
				__func__, ret, task->task.buf_out.num_planes);
		return -EINVAL;
	}

	for (i = 0; i < task->task.buf_out.num_planes; i++)
		task->dma_buf_out.plane[i].bytes_used = out_sizes[i];

	ret = m21dev->ops->prepare_format(ctx, &task->task.fmt_cap,
					DMA_FROM_DEVICE, cap_sizes);
	if (ret < 0)
		return ret;

	if (task->task.buf_cap.num_planes < ret) {
		dev_err(m21dev->dev,
			"%s: needs %u capture planes but %u is given\n",
				__func__, ret, task->task.buf_cap.num_planes);
		return -EINVAL;
	}

	for (i = 0; i < task->task.buf_cap.num_planes; i++)
		task->dma_buf_cap.plane[i].bytes_used = cap_sizes[i];

	if (m21dev->ops->prepare_operation) {
		ret = m21dev->ops->prepare_operation(ctx, task);
		if (ret)
			return ret;
	}

	return 0;
}

static int m2m1shot_prepare_task(struct m2m1shot_device *m21dev,
				struct m2m1shot_context *ctx,
				struct m2m1shot_task *task)
{
	int ret;

	ret = m2m1shot_prepare_format(m21dev, ctx, task);
	if (ret)
		return ret;

	ret = m2m1shot_prepare_get_buffer(ctx, &task->task.buf_out,
				&task->dma_buf_out, DMA_TO_DEVICE);
	if (ret) {
		dev_err(m21dev->dev, "%s: Failed to get output buffer\n",
			__func__);
		return ret;
	}

	ret = m2m1shot_prepare_get_buffer(ctx, &task->task.buf_cap,
				&task->dma_buf_cap, DMA_FROM_DEVICE);
	if (ret) {
		m2m1shot_finish_buffer(m21dev, ctx,
			&task->task.buf_out, &task->dma_buf_out, DMA_TO_DEVICE);
		dev_err(m21dev->dev, "%s: Failed to get capture buffer\n",
			__func__);
		return ret;
	}

	return 0;
}

static void m2m1shot_finish_task(struct m2m1shot_device *m21dev,
				struct m2m1shot_context *ctx,
				struct m2m1shot_task *task)
{
	m2m1shot_finish_buffer(m21dev, ctx,
			&task->task.buf_cap, &task->dma_buf_cap,
			DMA_FROM_DEVICE);
	m2m1shot_finish_buffer(m21dev, ctx,
		&task->task.buf_out, &task->dma_buf_out, DMA_TO_DEVICE);
}

static void m2m1shot_destroy_context(struct kref *kref)
{
	struct m2m1shot_context *ctx = container_of(kref,
					struct m2m1shot_context, kref);

	ctx->m21dev->ops->free_context(ctx);

	spin_lock(&ctx->m21dev->lock_ctx);
	list_del(&ctx->node);
	spin_unlock(&ctx->m21dev->lock_ctx);

	kfree(ctx);
}

static int m2m1shot_process(struct m2m1shot_context *ctx,
			struct m2m1shot_task *task)
{
	struct m2m1shot_device *m21dev = ctx->m21dev;
	unsigned long flags;
	int ret;

	INIT_LIST_HEAD(&task->task_node);
	init_completion(&task->complete);

	kref_get(&ctx->kref);

	mutex_lock(&ctx->mutex);

	ret = m2m1shot_prepare_task(m21dev, ctx, task);
	if (ret)
		goto err;

	task->ctx = ctx;
	task->state = M2M1SHOT_BUFSTATE_READY;

	spin_lock_irqsave(&m21dev->lock_task, flags);
	list_add_tail(&task->task_node, &m21dev->tasks);
	spin_unlock_irqrestore(&m21dev->lock_task, flags);

	m2m1shot_task_schedule(m21dev);

	if (m21dev->timeout_jiffies != -1) {
		unsigned long elapsed;
		elapsed = wait_for_completion_timeout(&task->complete,
					m21dev->timeout_jiffies);
		if (!elapsed) { /* timed out */
			m2m1shot_task_cancel(m21dev, task,
					M2M1SHOT_BUFSTATE_TIMEDOUT);

			m21dev->ops->timeout_task(ctx, task);

			m2m1shot_finish_task(m21dev, ctx, task);

			dev_notice(m21dev->dev, "%s: %u msecs timed out\n",
				__func__,
				jiffies_to_msecs(m21dev->timeout_jiffies));
			ret = -ETIMEDOUT;
			goto err;
		}
	} else {
		wait_for_completion(&task->complete);
	}

	BUG_ON(task->state == M2M1SHOT_BUFSTATE_READY);

	m2m1shot_finish_task(m21dev, ctx, task);
err:

	mutex_unlock(&ctx->mutex);

	kref_put(&ctx->kref, m2m1shot_destroy_context);

	if (ret)
		return ret;
	return (task->state == M2M1SHOT_BUFSTATE_DONE) ? 0 : -EINVAL;
}

static int m2m1shot_open(struct inode *inode, struct file *filp)
{
	struct m2m1shot_device *m21dev = container_of(filp->private_data,
						struct m2m1shot_device, misc);
	struct m2m1shot_context *ctx;
	int ret;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	INIT_LIST_HEAD(&ctx->node);
	kref_init(&ctx->kref);
	mutex_init(&ctx->mutex);

	ctx->m21dev = m21dev;

	spin_lock(&m21dev->lock_ctx);
	list_add_tail(&ctx->node, &m21dev->contexts);
	spin_unlock(&m21dev->lock_ctx);

	filp->private_data = ctx;

	ret = m21dev->ops->init_context(ctx);
	if (ret) /* kref_put() is not called not to call .free_context() */
		kfree(ctx);

	return ret;
}

static int m2m1shot_release(struct inode *inode, struct file *filp)
{
	struct m2m1shot_context *ctx = filp->private_data;

	kref_put(&ctx->kref, m2m1shot_destroy_context);

	return 0;
}

static long m2m1shot_ioctl(struct file *filp,
			unsigned int cmd, unsigned long arg)
{
	struct m2m1shot_context *ctx = filp->private_data;
	struct m2m1shot_device *m21dev = ctx->m21dev;

	switch (cmd) {
	case M2M1SHOT_IOC_PROCESS:
	{
		struct m2m1shot_task data;
		int ret;

		memset(&data, 0, sizeof(data));

		if (copy_from_user(&data.task,
				(void __user *)arg, sizeof(data.task))) {
			dev_err(m21dev->dev,
				"%s: Failed to read userdata\n", __func__);
			return -EFAULT;
		}

		/*
		 * m2m1shot_process() does not wake up
		 * until the given task finishes
		 */
		ret = m2m1shot_process(ctx, &data);

		if (copy_to_user((void __user *)arg, &data.task,
						sizeof(data.task))) {
			dev_err(m21dev->dev,
				"%s: Failed to read userdata\n", __func__);
			return -EFAULT;
		}

		return ret;
	}
	case M2M1SHOT_IOC_CUSTOM:
	{
		struct m2m1shot_custom_data data;

		if (!m21dev->ops->custom_ioctl) {
			dev_err(m21dev->dev,
				"%s: custom_ioctl not defined\n", __func__);
			return -ENOSYS;
		}

		if (copy_from_user(&data, (void __user *)arg, sizeof(data))) {
			dev_err(m21dev->dev,
				"%s: Failed to read custom data\n", __func__);
			return -EFAULT;
		}

		return m21dev->ops->custom_ioctl(ctx, data.cmd, data.arg);
	}
	default:
		dev_err(m21dev->dev, "%s: Unknown ioctl cmd %x\n",
			__func__, cmd);
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_COMPAT
struct compat_m2m1shot_rect {
	compat_short_t left;
	compat_short_t top;
	compat_ushort_t width;
	compat_ushort_t height;
};

struct compat_m2m1shot_pix_format {
	compat_uint_t fmt;
	compat_uint_t width;
	compat_uint_t  height;
	struct v4l2_rect crop;
};

struct compat_m2m1shot_buffer_plane {
	union {
		compat_int_t fd;
		compat_ulong_t userptr;
	};
	compat_size_t len;
};

struct compat_m2m1shot_buffer {
	struct compat_m2m1shot_buffer_plane plane[M2M1SHOT_MAX_PLANES];
	__u8 type;
	__u8 num_planes;
};

struct compat_m2m1shot_operation {
	compat_short_t quality_level;
	compat_short_t rotate;
	compat_uint_t op; /* or-ing M2M1SHOT_FLIP_VIRT/HORI */
};

struct compat_m2m1shot {
	struct compat_m2m1shot_pix_format fmt_out;
	struct compat_m2m1shot_pix_format fmt_cap;
	struct compat_m2m1shot_buffer buf_out;
	struct compat_m2m1shot_buffer buf_cap;
	struct compat_m2m1shot_operation op;
	compat_ulong_t reserved[2];
};

struct compat_m2m1shot_custom_data {
	compat_uint_t cmd;
	compat_ulong_t arg;
};

#define COMPAT_M2M1SHOT_IOC_PROCESS	_IOWR('M',  0, struct compat_m2m1shot)
#define COMPAT_M2M1SHOT_IOC_CUSTOM	\
			_IOWR('M', 16, struct compat_m2m1shot_custom_data)

static long m2m1shot_compat_ioctl32(struct file *filp,
				unsigned int cmd, unsigned long arg)
{
	struct m2m1shot_context *ctx = filp->private_data;
	struct m2m1shot_device *m21dev = ctx->m21dev;

	switch (cmd) {
	case COMPAT_M2M1SHOT_IOC_PROCESS:
	{
		struct compat_m2m1shot data;
		struct m2m1shot_task task;
		int i, ret;

		memset(&task, 0, sizeof(task));

		if (copy_from_user(&data, compat_ptr(arg), sizeof(data))) {
			dev_err(m21dev->dev,
				"%s: Failed to read userdata\n", __func__);
			return -EFAULT;
		}

		if ((data.buf_out.num_planes > M2M1SHOT_MAX_PLANES) ||
			(data.buf_cap.num_planes > M2M1SHOT_MAX_PLANES)) {
			dev_err(m21dev->dev,
				"%s: Invalid plane number (out %u/cap %u)\n",
				__func__, data.buf_out.num_planes,
				data.buf_cap.num_planes);
			return -EINVAL;
		}

		task.task.fmt_out.fmt = data.fmt_out.fmt;
		task.task.fmt_out.width = data.fmt_out.width;
		task.task.fmt_out.height = data.fmt_out.height;
		task.task.fmt_out.crop.left = data.fmt_out.crop.left;
		task.task.fmt_out.crop.top = data.fmt_out.crop.top;
		task.task.fmt_out.crop.width = data.fmt_out.crop.width;
		task.task.fmt_out.crop.height = data.fmt_out.crop.height;
		task.task.fmt_cap.fmt = data.fmt_cap.fmt;
		task.task.fmt_cap.width = data.fmt_cap.width;
		task.task.fmt_cap.height = data.fmt_cap.height;
		task.task.fmt_cap.crop.left = data.fmt_cap.crop.left;
		task.task.fmt_cap.crop.top = data.fmt_cap.crop.top;
		task.task.fmt_cap.crop.width = data.fmt_cap.crop.width;
		task.task.fmt_cap.crop.height = data.fmt_cap.crop.height;
		for (i = 0; i < data.buf_out.num_planes; i++) {
			task.task.buf_out.plane[i].len =
						data.buf_out.plane[i].len;
			if (data.buf_out.type == M2M1SHOT_BUFFER_DMABUF)
				task.task.buf_out.plane[i].fd =
						data.buf_out.plane[i].fd;
			else /* data.buf_out.type == M2M1SHOT_BUFFER_USERPTR */
				task.task.buf_out.plane[i].userptr =
						data.buf_out.plane[i].userptr;
		}
		task.task.buf_out.type = data.buf_out.type;
		task.task.buf_out.num_planes = data.buf_out.num_planes;
		for (i = 0; i < data.buf_cap.num_planes; i++) {
			task.task.buf_cap.plane[i].len =
						data.buf_cap.plane[i].len;
			if (data.buf_cap.type == M2M1SHOT_BUFFER_DMABUF)
				task.task.buf_cap.plane[i].fd =
						data.buf_cap.plane[i].fd;
			else /* data.buf_cap.type == M2M1SHOT_BUFFER_USERPTR */
				task.task.buf_cap.plane[i].userptr =
						data.buf_cap.plane[i].userptr;
		}
		task.task.buf_cap.type = data.buf_cap.type;
		task.task.buf_cap.num_planes = data.buf_cap.num_planes;
		task.task.op.quality_level = data.op.quality_level;
		task.task.op.rotate = data.op.rotate;
		task.task.op.op = data.op.op;
		task.task.reserved[0] = data.reserved[0];
		task.task.reserved[1] = data.reserved[1];

		/*
		 * m2m1shot_process() does not wake up
		 * until the given task finishes
		 */
		ret = m2m1shot_process(ctx, &task);
		if (ret) {
			dev_err(m21dev->dev,
				"%s: Failed to process m2m1shot task\n",
				__func__);
			return ret;
		}

		data.fmt_out.fmt = task.task.fmt_out.fmt;
		data.fmt_out.width = task.task.fmt_out.width;
		data.fmt_out.height = task.task.fmt_out.height;
		data.fmt_out.crop.left = task.task.fmt_out.crop.left;
		data.fmt_out.crop.top = task.task.fmt_out.crop.top;
		data.fmt_out.crop.width = task.task.fmt_out.crop.width;
		data.fmt_out.crop.height = task.task.fmt_out.crop.height;
		data.fmt_cap.fmt = task.task.fmt_cap.fmt;
		data.fmt_cap.width = task.task.fmt_cap.width;
		data.fmt_cap.height = task.task.fmt_cap.height;
		data.fmt_cap.crop.left = task.task.fmt_cap.crop.left;
		data.fmt_cap.crop.top = task.task.fmt_cap.crop.top;
		data.fmt_cap.crop.width = task.task.fmt_cap.crop.width;
		data.fmt_cap.crop.height = task.task.fmt_cap.crop.height;
		for (i = 0; i < task.task.buf_out.num_planes; i++) {
			data.buf_out.plane[i].len =
				task.task.buf_out.plane[i].len;
			if (task.task.buf_out.type == M2M1SHOT_BUFFER_DMABUF)
				data.buf_out.plane[i].fd =
					task.task.buf_out.plane[i].fd;
			else /* buf_out.type == M2M1SHOT_BUFFER_USERPTR */
				data.buf_out.plane[i].userptr =
					task.task.buf_out.plane[i].userptr;
		}
		data.buf_out.type = task.task.buf_out.type;
		data.buf_out.num_planes = task.task.buf_out.num_planes;
		for (i = 0; i < task.task.buf_cap.num_planes; i++) {
			data.buf_cap.plane[i].len =
				task.task.buf_cap.plane[i].len;
			if (task.task.buf_cap.type == M2M1SHOT_BUFFER_DMABUF)
				data.buf_cap.plane[i].fd =
					task.task.buf_cap.plane[i].fd;
			else /* buf_cap.type == M2M1SHOT_BUFFER_USERPTR */
				data.buf_cap.plane[i].userptr =
					task.task.buf_cap.plane[i].userptr;
		}
		data.buf_cap.type = task.task.buf_cap.type;
		data.buf_cap.num_planes = task.task.buf_cap.num_planes;
		data.op.quality_level = task.task.op.quality_level;
		data.op.rotate = task.task.op.rotate;
		data.op.op = task.task.op.op;
		data.reserved[0] = (compat_ulong_t)task.task.reserved[0];
		data.reserved[1] = (compat_ulong_t)task.task.reserved[1];

		if (copy_to_user(compat_ptr(arg), &data, sizeof(data))) {
			dev_err(m21dev->dev,
				"%s: Failed to copy into userdata\n", __func__);
			return -EFAULT;
		}

		return 0;
	}
	case COMPAT_M2M1SHOT_IOC_CUSTOM:
	{
		struct compat_m2m1shot_custom_data data;

		if (!m21dev->ops->custom_ioctl) {
			dev_err(m21dev->dev,
				"%s: custom_ioctl not defined\n", __func__);
			return -ENOSYS;
		}

		if (copy_from_user(&data, compat_ptr(arg), sizeof(data))) {
			dev_err(m21dev->dev,
				"%s: Failed to read custom data\n", __func__);
			return -EFAULT;
		}

		return m21dev->ops->custom_ioctl(ctx, data.cmd, data.arg);
	}
	default:
		dev_err(m21dev->dev, "%s: Unknown ioctl cmd %x\n",
			__func__, cmd);
		return -EINVAL;
	}

	return 0;
}
#endif

static const struct file_operations m2m1shot_fops = {
	.owner          = THIS_MODULE,
	.open           = m2m1shot_open,
	.release        = m2m1shot_release,
	.unlocked_ioctl	= m2m1shot_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= m2m1shot_compat_ioctl32,
#endif
};

struct m2m1shot_device *m2m1shot_create_device(struct device *dev,
					const struct m2m1shot_devops *ops,
					const char *suffix, int id,
					unsigned long timeout_jiffies)
{
	struct m2m1shot_device *m21dev;
	char *name;
	size_t name_size;
	int ret = -ENOMEM;

	/* TODO: ops callback check */
	if (!ops || !ops->prepare_format || !ops->prepare_buffer) {
		dev_err(dev, "%s: m2m1shot_devops is not provided\n", __func__);
		return ERR_PTR(-EINVAL);
	}

	if (!suffix) {
		dev_err(dev, "%s: suffix of node name is not specified\n",
				__func__);
		return ERR_PTR(-EINVAL);
	}

	name_size = M2M1SHOT_DEVNODE_PREFIX_LEN + strlen(suffix) + 1;

	if (id >= 0)
		name_size += 3; /* instance number: maximul 3 digits */

	name = kmalloc(name_size, GFP_KERNEL);
	if (!name)
		return ERR_PTR(-ENOMEM);

	if (id < 0)
		scnprintf(name, name_size,
				M2M1SHOT_DEVNODE_PREFIX "%s", suffix);
	else
		scnprintf(name, name_size,
				M2M1SHOT_DEVNODE_PREFIX "%s%d", suffix, id);

	m21dev = kzalloc(sizeof(*m21dev), GFP_KERNEL);
	if (!m21dev)
		goto err_m21dev;

	m21dev->misc.minor = MISC_DYNAMIC_MINOR;
	m21dev->misc.name = name;
	m21dev->misc.fops = &m2m1shot_fops;
	ret = misc_register(&m21dev->misc);
	if (ret)
		goto err_misc;

	INIT_LIST_HEAD(&m21dev->tasks);
	INIT_LIST_HEAD(&m21dev->contexts);

	spin_lock_init(&m21dev->lock_task);
	spin_lock_init(&m21dev->lock_ctx);

	m21dev->dev = dev;
	m21dev->ops = ops;
	m21dev->timeout_jiffies = timeout_jiffies;

	return m21dev;

err_m21dev:
	kfree(name);
err_misc:
	kfree(m21dev);

	return ERR_PTR(ret);
}
EXPORT_SYMBOL(m2m1shot_create_device);

void m2m1shot_destroy_device(struct m2m1shot_device *m21dev)
{
	misc_deregister(&m21dev->misc);
	kfree(m21dev->misc.name);
	kfree(m21dev);
}
EXPORT_SYMBOL(m2m1shot_destroy_device);
