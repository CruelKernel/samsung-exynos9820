/* sound/soc/samsung/abox/abox_log.c
 *
 * ALSA SoC Audio Layer - Samsung Abox Log driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/* #define DEBUG */
#include <linux/debugfs.h>
#include <linux/mutex.h>
#include <linux/vmalloc.h>
#include <sound/samsung/abox.h>

#include "abox_util.h"
#include "abox.h"
#include "abox_dbg.h"
#include "abox_log.h"

#undef VERBOSE_LOG

#undef TEST
#ifdef TEST
#define SIZE_OF_BUFFER (SZ_128)
#else
#define SIZE_OF_BUFFER (SZ_2M)
#endif

#define S_IRWUG (0660)

struct abox_log_kernel_buffer {
	char *buffer;
	unsigned int index;
	bool wrap;
	bool updated;
	wait_queue_head_t wq;
};

struct abox_log_buffer_info {
	struct list_head list;
	struct device *dev;
	int id;
	bool file_created;
	atomic_t opened;
	ssize_t file_index;
	struct mutex lock;
	struct ABOX_LOG_BUFFER *log_buffer;
	struct abox_log_kernel_buffer kernel_buffer;
};

static LIST_HEAD(abox_log_list_head);
static u32 abox_log_auto_save;

static void abox_log_memcpy(struct device *dev,
	struct abox_log_kernel_buffer *kernel_buffer,
	const char *src, size_t size)
{
	size_t left_size = SIZE_OF_BUFFER - kernel_buffer->index;

	dev_dbg(dev, "%s(%zu)\n", __func__, size);

	if (left_size < size) {
#ifdef VERBOSE_LOG
		dev_dbg(dev, "0: %s\n", src);
#endif
		memcpy(kernel_buffer->buffer + kernel_buffer->index, src,
				left_size);
		src += left_size;
		size -= left_size;
		kernel_buffer->index = 0;
		kernel_buffer->wrap = true;
	}
#ifdef VERBOSE_LOG
	dev_dbg(dev, "1: %s\n", src);
#endif
	memcpy(kernel_buffer->buffer + kernel_buffer->index, src, size);
	kernel_buffer->index += (unsigned int)size;
}

static void abox_log_file_name(struct device *dev,
		struct abox_log_buffer_info *info, char *name, size_t size)
{
	snprintf(name, size, "/data/calliope-%02d.log", info->id);
}

static void abox_log_file_save(struct device *dev,
		struct abox_log_buffer_info *info)
{
	struct ABOX_LOG_BUFFER *log_buffer = info->log_buffer;
	unsigned int index_writer = log_buffer->index_writer;
	char name[32];
	struct file *filp;
	mm_segment_t old_fs;

	dev_dbg(dev, "%s(%d)\n", __func__, info->id);

	abox_log_file_name(dev, info, name, sizeof(name));

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	if (likely(info->file_created)) {
		filp = filp_open(name, O_RDWR | O_APPEND | O_CREAT, S_IRWUG);
		dev_dbg(dev, "appended\n");
	} else {
		filp = filp_open(name, O_RDWR | O_TRUNC | O_CREAT, S_IRWUG);
		info->file_created = true;
		dev_dbg(dev, "created\n");
	}
	if (IS_ERR(filp)) {
		dev_warn(dev, "%s: saving log fail\n", __func__);
		goto out;
	}


	if (log_buffer->index_reader > index_writer) {
		vfs_write(filp, log_buffer->buffer + log_buffer->index_reader,
			log_buffer->size - log_buffer->index_reader,
			&filp->f_pos);
		vfs_write(filp, log_buffer->buffer, index_writer, &filp->f_pos);
	} else {
		vfs_write(filp, log_buffer->buffer + log_buffer->index_reader,
			index_writer - log_buffer->index_reader, &filp->f_pos);
	}

	vfs_fsync(filp, 0);
	filp_close(filp, NULL);
out:
	set_fs(old_fs);

}

static void abox_log_flush(struct device *dev,
		struct abox_log_buffer_info *info)
{
	struct ABOX_LOG_BUFFER *log_buffer = info->log_buffer;
	unsigned int index_writer = log_buffer->index_writer;
	struct abox_log_kernel_buffer *kernel_buffer = &info->kernel_buffer;

	if (log_buffer->index_reader == index_writer)
		return;

	dev_dbg(dev, "%s(%d): index_writer=%u, index_reader=%u, size=%u\n",
			__func__, info->id, index_writer,
			log_buffer->index_reader, log_buffer->size);

	mutex_lock(&info->lock);

	if (abox_log_auto_save)
		abox_log_file_save(dev, info);

	if (log_buffer->index_reader > index_writer) {
		abox_log_memcpy(info->dev, kernel_buffer,
				log_buffer->buffer + log_buffer->index_reader,
				log_buffer->size - log_buffer->index_reader);
		log_buffer->index_reader = 0;
	}
	abox_log_memcpy(info->dev, kernel_buffer,
			log_buffer->buffer + log_buffer->index_reader,
			index_writer - log_buffer->index_reader);
	log_buffer->index_reader = index_writer;
	mutex_unlock(&info->lock);

	kernel_buffer->updated = true;
	wake_up_interruptible(&kernel_buffer->wq);

#ifdef TEST
	dev_dbg(dev, "shared_buffer: %s\n", log_buffer->buffer);
	dev_dbg(dev, "kernel_buffer: %s\n", info->kernel_buffer.buffer);
#endif
}

void abox_log_flush_all(struct device *dev)
{
	struct abox_log_buffer_info *info;

	dev_dbg(dev, "%s\n", __func__);

	list_for_each_entry(info, &abox_log_list_head, list) {
		abox_log_flush(info->dev, info);
	}
}
EXPORT_SYMBOL(abox_log_flush_all);

static unsigned long abox_log_flush_all_work_rearm_self;
static void abox_log_flush_all_work_func(struct work_struct *work);
static DECLARE_DEFERRABLE_WORK(abox_log_flush_all_work,
		abox_log_flush_all_work_func);

static void abox_log_flush_all_work_func(struct work_struct *work)
{
	abox_log_flush_all(NULL);
	schedule_delayed_work(&abox_log_flush_all_work, msecs_to_jiffies(3000));
	set_bit(0, &abox_log_flush_all_work_rearm_self);
}

void abox_log_schedule_flush_all(struct device *dev)
{
	if (test_and_clear_bit(0, &abox_log_flush_all_work_rearm_self))
		cancel_delayed_work(&abox_log_flush_all_work);
	schedule_delayed_work(&abox_log_flush_all_work, msecs_to_jiffies(100));
}
EXPORT_SYMBOL(abox_log_schedule_flush_all);

void abox_log_drain_all(struct device *dev)
{
	cancel_delayed_work(&abox_log_flush_all_work);
	abox_log_flush_all(dev);
}
EXPORT_SYMBOL(abox_log_drain_all);

static int abox_log_file_open(struct inode *inode, struct  file *file)
{
	struct abox_log_buffer_info *info = inode->i_private;

	dev_dbg(info->dev, "%s\n", __func__);

	if (atomic_cmpxchg(&info->opened, 0, 1))
		return -EBUSY;

	info->file_index = -1;
	file->private_data = info;

	return 0;
}

static int abox_log_file_release(struct inode *inode, struct file *file)
{
	struct abox_log_buffer_info *info = inode->i_private;

	dev_dbg(info->dev, "%s\n", __func__);

	atomic_cmpxchg(&info->opened, 1, 0);

	return 0;
}

static ssize_t abox_log_file_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	struct abox_log_buffer_info *info = file->private_data;
	struct abox_log_kernel_buffer *kernel_buffer = &info->kernel_buffer;
	unsigned int index;
	size_t end, size;
	bool first = (info->file_index < 0);
	int ret;

	dev_dbg(info->dev, "%s(%zu, %lld)\n", __func__, count, *ppos);

	mutex_lock(&info->lock);

	if (first) {
		info->file_index = likely(kernel_buffer->wrap) ?
				kernel_buffer->index : 0;
	}

	do {
		index = kernel_buffer->index;
		end = ((info->file_index < index) ||
				((info->file_index == index) && !first)) ?
				index : SIZE_OF_BUFFER;
		size = min(end - info->file_index, count);
		if (size == 0) {
			mutex_unlock(&info->lock);
			if (file->f_flags & O_NONBLOCK) {
				dev_dbg(info->dev, "non block\n");
				return -EAGAIN;
			}
			kernel_buffer->updated = false;

			ret = wait_event_interruptible(kernel_buffer->wq,
					kernel_buffer->updated);
			if (ret != 0) {
				dev_dbg(info->dev, "interrupted\n");
				return ret;
			}
			mutex_lock(&info->lock);
		}
#ifdef VERBOSE_LOG
		dev_dbg(info->dev, "loop %zu, %zu, %zd, %zu\n", size, end,
				info->file_index, count);
#endif
	} while (size == 0);

	dev_dbg(info->dev, "start=%zd, end=%zd size=%zd\n", info->file_index,
			end, size);
	if (copy_to_user(buf, kernel_buffer->buffer + info->file_index,
			size)) {
		mutex_unlock(&info->lock);
		return -EFAULT;
	}

	info->file_index += size;
	if (info->file_index >= SIZE_OF_BUFFER)
		info->file_index = 0;

	mutex_unlock(&info->lock);

	dev_dbg(info->dev, "%s: size = %zd\n", __func__, size);

	return size;
}

static unsigned int abox_log_file_poll(struct file *file, poll_table *wait)
{
	struct abox_log_buffer_info *info = file->private_data;
	struct abox_log_kernel_buffer *kernel_buffer = &info->kernel_buffer;

	dev_dbg(info->dev, "%s\n", __func__);

	poll_wait(file, &kernel_buffer->wq, wait);
	return POLLIN | POLLRDNORM;
}

static const struct file_operations abox_log_fops = {
	.open = abox_log_file_open,
	.release = abox_log_file_release,
	.read = abox_log_file_read,
	.poll = abox_log_file_poll,
	.llseek = generic_file_llseek,
	.owner = THIS_MODULE,
};

static struct abox_log_buffer_info abox_log_buffer_info_new;

void abox_log_register_buffer_work_func(struct work_struct *work)
{
	struct device *dev;
	int id;
	struct ABOX_LOG_BUFFER *buffer;
	struct abox_log_buffer_info *info;
	char name[16];

	dev = abox_log_buffer_info_new.dev;
	id = abox_log_buffer_info_new.id;
	buffer = abox_log_buffer_info_new.log_buffer;
	abox_log_buffer_info_new.dev = NULL;
	abox_log_buffer_info_new.id = 0;
	abox_log_buffer_info_new.log_buffer = NULL;

	dev_info(dev, "%s(%d)\n", __func__, id);

	info = vmalloc(sizeof(*info));
	mutex_init(&info->lock);
	info->id = id;
	info->file_created = false;
	atomic_set(&info->opened, 0);
	info->kernel_buffer.buffer = vzalloc(SIZE_OF_BUFFER);
	info->kernel_buffer.index = 0;
	info->kernel_buffer.wrap = false;
	init_waitqueue_head(&info->kernel_buffer.wq);
	info->dev = dev;
	info->log_buffer = buffer;
	list_add_tail(&info->list, &abox_log_list_head);

	snprintf(name, sizeof(name), "log-%02d", id);
	debugfs_create_file(name, 0664, abox_dbg_get_root_dir(), info,
			&abox_log_fops);
}

static DECLARE_WORK(abox_log_register_buffer_work,
		abox_log_register_buffer_work_func);

int abox_log_register_buffer(struct device *dev, int id,
		struct ABOX_LOG_BUFFER *buffer)
{
	struct abox_log_buffer_info *info;

	dev_dbg(dev, "%s(%d)\n", __func__, id);

	if (abox_log_buffer_info_new.dev != NULL ||
			abox_log_buffer_info_new.id > 0 ||
			abox_log_buffer_info_new.log_buffer != NULL) {
		return -EBUSY;
	}

	list_for_each_entry(info, &abox_log_list_head, list) {
		if (info->id == id) {
			dev_dbg(dev, "already registered log: %d\n", id);
			return 0;
		}
	}

	abox_log_buffer_info_new.dev = dev;
	abox_log_buffer_info_new.id = id;
	abox_log_buffer_info_new.log_buffer = buffer;
	schedule_work(&abox_log_register_buffer_work);

	return 0;
}
EXPORT_SYMBOL(abox_log_register_buffer);

#ifdef TEST
static struct ABOX_LOG_BUFFER *abox_log_test_buffer;
static void abox_log_test_work_func(struct work_struct *work);
DECLARE_DELAYED_WORK(abox_log_test_work, abox_log_test_work_func);
static void abox_log_test_work_func(struct work_struct *work)
{
	struct ABOX_LOG_BUFFER *log = abox_log_test_buffer;
	static unsigned int i;
	char buffer[32];
	char *buffer_index = buffer;
	int size, left;

	pr_debug("%s: %d\n", __func__, i);

	size = snprintf(buffer, sizeof(buffer), "%d ", i++);

	if (log->index_writer + size > log->size) {
		left = log->size - log->index_writer;
		memcpy(&log->buffer[log->index_writer], buffer_index, left);
		log->index_writer = 0;
		buffer_index += left;
	}

	left = size - (buffer_index - buffer);
	memcpy(&log->buffer[log->index_writer], buffer_index, left);
	log->index_writer += left;

	abox_log_flush_all(NULL);

	schedule_delayed_work(&abox_log_test_work, msecs_to_jiffies(1000));
}
#endif

static int __init samsung_abox_log_late_initcall(void)
{
	pr_info("%s\n", __func__);

	debugfs_create_u32("log_auto_save", S_IRWUG, abox_dbg_get_root_dir(),
			&abox_log_auto_save);

#ifdef TEST
	abox_log_test_buffer = vzalloc(SZ_128);
	abox_log_test_buffer->size = SZ_64;
	abox_log_register_buffer(NULL, 0, abox_log_test_buffer);
	schedule_delayed_work(&abox_log_test_work, msecs_to_jiffies(1000));
#endif

	return 0;
}
late_initcall(samsung_abox_log_late_initcall);
