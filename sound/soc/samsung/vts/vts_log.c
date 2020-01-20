/* sound/soc/samsung/vts/vts_log.c
 *
 * ALSA SoC - Samsung VTS Log driver
 *
 * Copyright (c) 2017 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
//#define DEBUG
#include <linux/io.h>
#include <linux/debugfs.h>
#include <linux/mutex.h>
#include <linux/vmalloc.h>
#include <sound/soc.h>

#include "vts.h"
#include "vts_log.h"

#define VTS_LOG_BUFFER_SIZE (SZ_1M)
#define S_IRWUG (S_IRUSR|S_IRGRP|S_IWUSR|S_IWGRP)

struct vts_kernel_log_buffer {
	char *buffer;
	unsigned int index;
	bool wrap;
	bool updated;
	wait_queue_head_t wq;
};

struct vts_log_buffer_info {
	struct device *dev;
	struct mutex lock;
	struct vts_log_buffer log_buffer;
	struct vts_kernel_log_buffer kernel_buffer;
	bool registered;
	u32 logbuf_index;
};

static struct dentry *vts_dbg_root_dir __read_mostly;
static struct vts_log_buffer_info glogbuf_info;

struct dentry *vts_dbg_get_root_dir(void)
{
	pr_debug("%s\n", __func__);

	if (vts_dbg_root_dir == NULL)
		vts_dbg_root_dir = debugfs_create_dir("vts", NULL);

	return vts_dbg_root_dir;
}

static ssize_t vts_log_file_index;

static int vts_log_file_open(struct inode *inode, struct  file *file)
{
	struct vts_log_buffer_info *info = inode->i_private;
	struct vts_data *data = dev_get_drvdata(info->dev);
	u32 values[3] = {0, 0, 0};
	int result = 0;

	dev_dbg(info->dev, "%s\n", __func__);

	file->private_data = inode->i_private;
	vts_log_file_index = -1;

	if (data->vts_ready) {
		values[0] = VTS_ENABLE_DEBUGLOG;
		values[1] = 0;
		values[2] = 0;
		result = vts_start_ipc_transaction(info->dev, data, VTS_IRQ_AP_TEST_COMMAND, &values, 0, 1);
		if (result < 0)
			dev_err(info->dev, "%s Enable_debuglog ipc transaction failed\n", __func__);
	}

	data->vtslog_enabled = 1;
	return 0;
}

static int vts_log_file_close(struct inode *inode, struct  file *file)
{
	struct vts_log_buffer_info *info = inode->i_private;
	struct vts_data *data = dev_get_drvdata(info->dev);
	u32 values[3] = {0, 0, 0};
	int result = 0;

	dev_dbg(info->dev, "%s\n", __func__);

	vts_log_file_index = -1;

	if (data->vts_ready) {
		values[0] = VTS_DISABLE_DEBUGLOG;
		values[1] = 0;
		values[2] = 0;
		result = vts_start_ipc_transaction(info->dev, data, VTS_IRQ_AP_TEST_COMMAND, &values, 0, 1);
		if (result < 0)
			dev_err(info->dev, "%s Disable_debuglog ipc transaction failed\n", __func__);
		/* reset VTS SRAM debug log buffer */
		vts_register_log_buffer(info->dev, 0, 0);
	}

	data->vtslog_enabled = 0;
	return 0;
}

static ssize_t vts_log_file_read(
	struct file *file,
	char __user *buf,
	size_t count,
	loff_t *ppos)
{
	struct vts_log_buffer_info *info = file->private_data;
	struct vts_kernel_log_buffer *kernel_buffer = &info->kernel_buffer;
	size_t end, size;
	bool first = (vts_log_file_index < 0);
	int result;

	dev_dbg(info->dev, "%s(%zu, %lld)\n", __func__, count, *ppos);

	mutex_lock(&info->lock);

	if (vts_log_file_index < 0)
		vts_log_file_index = likely(kernel_buffer->wrap) ?
						kernel_buffer->index : 0;

	do {
		end = ((vts_log_file_index < kernel_buffer->index) ||
			((vts_log_file_index == kernel_buffer->index)
			&& !first)) ? kernel_buffer->index
			: VTS_LOG_BUFFER_SIZE;
		size = min(end - vts_log_file_index, count);
		if (size == 0) {
			mutex_unlock(&info->lock);
			if (file->f_flags & O_NONBLOCK) {
				dev_dbg(info->dev, "non block\n");
				return -EAGAIN;
			}
			kernel_buffer->updated = false;

			result = wait_event_interruptible(kernel_buffer->wq,
					kernel_buffer->updated);

			if (result != 0) {
				dev_dbg(info->dev, "interrupted\n");
				return result;
			}
			mutex_lock(&info->lock);
		}
#ifdef VERBOSE_LOG
		dev_dbg(info->dev, "loop %zu, %zu, %zd, %zu\n", size, end, vts_log_file_index, count);
#endif
	} while (size == 0);

	dev_dbg(info->dev, "start=%zd, end=%zd size=%zd\n", vts_log_file_index, end, size);
	if (copy_to_user(buf, kernel_buffer->buffer + vts_log_file_index, size))
		return -EFAULT;

	vts_log_file_index += size;
	if (vts_log_file_index >= VTS_LOG_BUFFER_SIZE)
		vts_log_file_index = 0;

	mutex_unlock(&info->lock);

	dev_dbg(info->dev, "%s: size = %zd\n", __func__, size);

	return size;
}

static unsigned int vts_log_file_poll(struct file *file, poll_table *wait)
{
	struct vts_log_buffer_info *info = file->private_data;
	struct vts_kernel_log_buffer *kernel_buffer = &info->kernel_buffer;

	dev_dbg(info->dev, "%s\n", __func__);

	poll_wait(file, &kernel_buffer->wq, wait);
	return POLLIN | POLLRDNORM;
}

static const struct file_operations vts_log_fops = {
	.open = vts_log_file_open,
	.release = vts_log_file_close,
	.read = vts_log_file_read,
	.poll = vts_log_file_poll,
	.llseek = generic_file_llseek,
	.owner = THIS_MODULE,
};

static void vts_log_memcpy(struct device *dev,
	 char *src, size_t size)
{
	struct vts_kernel_log_buffer *kernel_buffer = NULL;
	size_t left_size = 0;

	kernel_buffer = &glogbuf_info.kernel_buffer;
	left_size = VTS_LOG_BUFFER_SIZE - kernel_buffer->index;

	dev_dbg(dev, "%s(%zu)\n", __func__, size);

	if (left_size < size) {
#ifdef VERBOSE_LOG
		dev_dbg(dev, "0: %s\n", src);
#endif
		memcpy_fromio(kernel_buffer->buffer +
			kernel_buffer->index, src, left_size);
		src += left_size;
		size -= left_size;
		kernel_buffer->index = 0;
		kernel_buffer->wrap = true;
	}
#ifdef VERBOSE_LOG
	dev_dbg(dev, "1: %s\n", src);
#endif
	memcpy_fromio(kernel_buffer->buffer + kernel_buffer->index, src, size);
	kernel_buffer->index += size;
}

static void vts_log_flush_work_func(struct work_struct *work)
{
	struct device *dev = glogbuf_info.dev;
	struct vts_log_buffer *log_buffer = &glogbuf_info.log_buffer;
	struct vts_kernel_log_buffer *kernel_buffer = NULL;
	int logbuf_index = glogbuf_info.logbuf_index;

	kernel_buffer = &glogbuf_info.kernel_buffer;

	dev_dbg(dev, "%s: LogBuffer Index: %d\n", __func__,
		logbuf_index);

	mutex_lock(&glogbuf_info.lock);

	vts_log_memcpy(dev, (log_buffer->addr +
			log_buffer->size * logbuf_index), log_buffer->size);
	/* memory barrier */
	wmb();
	mutex_unlock(&glogbuf_info.lock);

	kernel_buffer->updated = true;
	wake_up_interruptible(&kernel_buffer->wq);
}

static DECLARE_WORK(vts_log_work, vts_log_flush_work_func);
void vts_log_schedule_flush(struct device *dev, u32 index)
{
	if (glogbuf_info.registered &&
		glogbuf_info.log_buffer.size) {
		glogbuf_info.logbuf_index = index;
		schedule_work(&vts_log_work);
		dev_dbg(dev, "%s: VTS Log Buffer[%d] Scheduled\n",
			 __func__, index);
	} else
		dev_warn(dev, "%s: VTS Debugging buffer not registered\n",
				 __func__);
}
EXPORT_SYMBOL(vts_log_schedule_flush);

int vts_register_log_buffer(
	struct device *dev,
	u32 addroffset,
	u32 logsz)
{
	struct vts_data *data = dev_get_drvdata(dev);

	dev_dbg(dev, "%s(offset 0x%x)\n", __func__, addroffset);

	if ((addroffset + logsz) > data->sram_size) {
		dev_warn(dev, "%s: wrong offset[0x%x] or size[0x%x]\n",
				__func__, addroffset, logsz);
		return -EINVAL;
	}

	if (!glogbuf_info.registered) {
		glogbuf_info.dev = dev;
		mutex_init(&glogbuf_info.lock);
		glogbuf_info.kernel_buffer.buffer =
				vzalloc(VTS_LOG_BUFFER_SIZE);
		glogbuf_info.kernel_buffer.index = 0;
		glogbuf_info.kernel_buffer.wrap = false;
		init_waitqueue_head(&glogbuf_info.kernel_buffer.wq);

		debugfs_create_file("vts-log", S_IRWUG,
				vts_dbg_get_root_dir(),
				&glogbuf_info, &vts_log_fops);
		glogbuf_info.registered = true;
	}

	/* Update logging buffer address and size info */
	glogbuf_info.log_buffer.addr = data->sram_base + addroffset;
	glogbuf_info.log_buffer.size = logsz;

	return 0;
}
EXPORT_SYMBOL(vts_register_log_buffer);

static int __init samsung_vts_log_late_initcall(void)
{
	pr_info("%s\n", __func__);

	return 0;
}
late_initcall(samsung_vts_log_late_initcall);
