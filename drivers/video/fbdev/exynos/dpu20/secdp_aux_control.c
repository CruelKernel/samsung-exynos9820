/*
 * Copyright © 2015 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#define pr_fmt(fmt)	"[drm-dp] %s: " fmt, __func__

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/sched/signal.h>

#include <linux/dp_logger.h>
#include "secdp_aux_control.h"

#define DP_ENUM_STR(x)	#x

#define IOCTL_MAGIC		't'
#define IOCTL_MAXNR		30
#define IOCTL_DP_AUXCMD_TYPE	_IO(IOCTL_MAGIC, 0)
#define IOCTL_DP_HPD_STATUS	_IO(IOCTL_MAGIC, 1)
#define DP_AUX_MAX_BUF 16

enum dp_auxcmd_type {
	DP_AUXCMD_NATIVE,
	DP_AUXCMD_I2C,
};

struct ioctl_auxdev_info {
	int size;	/* unused */
	enum dp_auxcmd_type cmd_type;
};

struct secdp_aux_dev {
	unsigned int index;
	struct device *dev;
	struct kref refcount;
	atomic_t usecount;
	enum dp_auxcmd_type cmd_type;

	ssize_t (*secdp_i2c_write)(void *buffer, size_t size);
	ssize_t (*secdp_i2c_read)(void *buffer, size_t size);
	ssize_t (*secdp_dpcd_write)(unsigned int offset,
			void *buffer, size_t size);
	ssize_t (*secdp_dpcd_read)(unsigned int offset,
			void *buffer, size_t size);
	int (*secdp_get_hpd_status)(void);
};

static inline char *auxdev_ioctl_cmd_to_string(u32 cmd)
{
	switch (cmd) {
	case IOCTL_DP_AUXCMD_TYPE:
		return DP_ENUM_STR(IOCTL_DP_AUXCMD_TYPE);
	case IOCTL_DP_HPD_STATUS:
		return DP_ENUM_STR(IOCTL_DP_HPD_STATUS);
	default:
		return "unknown";
	}
}

static inline char *auxcmd_type_to_string(u32 cmd_type)
{
	switch (cmd_type) {
	case DP_AUXCMD_NATIVE:
		return DP_ENUM_STR(DP_AUXCMD_NATIVE);
	case DP_AUXCMD_I2C:
		return DP_ENUM_STR(DP_AUXCMD_I2C);
	default:
		return "unknown";
	}
}

#define DRM_AUX_MINORS	256
#define AUX_MAX_OFFSET	(1 << 20)

static DEFINE_IDR(aux_idr);
static DEFINE_MUTEX(aux_idr_mutex);
static struct class *secdp_aux_dev_class;
static int drm_dev_major = -1;

static struct secdp_aux_dev *secdp_aux_dev_get_by_minor(unsigned int index)
{
	struct secdp_aux_dev *aux_dev = NULL;

	mutex_lock(&aux_idr_mutex);
	aux_dev = idr_find(&aux_idr, index);
	if (!kref_get_unless_zero(&aux_dev->refcount))
		aux_dev = NULL;
	mutex_unlock(&aux_idr_mutex);

	return aux_dev;
}

static void release_secdp_aux_dev(struct kref *ref)
{
	struct secdp_aux_dev *aux_dev =
		container_of(ref, struct secdp_aux_dev, refcount);

	kfree(aux_dev);
}

static int auxdev_open(struct inode *inode, struct file *file)
{
	unsigned int minor = iminor(inode);
	struct secdp_aux_dev *aux_dev;

	aux_dev = secdp_aux_dev_get_by_minor(minor);
	if (!aux_dev)
		return -ENODEV;

	file->private_data = aux_dev;
	return 0;
}

static loff_t auxdev_llseek(struct file *file, loff_t offset, int whence)
{
	return fixed_size_llseek(file, offset, whence, AUX_MAX_OFFSET);
}

static ssize_t auxdev_read(struct file *file, char __user *buf, size_t count,
		loff_t *offset)
{
	size_t bytes_pending, num_bytes_processed = 0;
	struct secdp_aux_dev *aux_dev = file->private_data;
	ssize_t res = 0;

	if (!aux_dev->secdp_get_hpd_status())
		return -ENODEV;

	if (!atomic_inc_not_zero(&aux_dev->usecount))
		return -ENODEV;

	bytes_pending = min((loff_t)count, AUX_MAX_OFFSET - (*offset));

	if (!access_ok(VERIFY_WRITE, buf, bytes_pending)) {
		res = -EFAULT;
		goto out;
	}

	while (bytes_pending > 0) {
		uint8_t localbuf[DP_AUX_MAX_BUF];
		ssize_t todo = min_t(size_t, bytes_pending, sizeof(localbuf));

		if (signal_pending(current)) {
			res = num_bytes_processed ?
				num_bytes_processed : -ERESTARTSYS;
			goto out;
		}

		if (aux_dev->cmd_type == DP_AUXCMD_NATIVE)
			res = aux_dev->secdp_dpcd_read(*offset, localbuf, todo);
		else
			res = aux_dev->secdp_i2c_read(localbuf, todo);

		if (res <= 0) {
			res = num_bytes_processed ? num_bytes_processed : res;
			goto out;
		}
		if (__copy_to_user(buf + num_bytes_processed, localbuf, res)) {
			res = num_bytes_processed ?
				num_bytes_processed : -EFAULT;
			goto out;
		}
		bytes_pending -= res;

		if (aux_dev->cmd_type == DP_AUXCMD_NATIVE)
			*offset += res;

		num_bytes_processed += res;
		res = num_bytes_processed;
	}

out:
	atomic_dec(&aux_dev->usecount);
	wake_up_atomic_t(&aux_dev->usecount);
	return res;
}

static ssize_t auxdev_write(struct file *file, const char __user *buf,
		size_t count, loff_t *offset)
{
	size_t bytes_pending, num_bytes_processed = 0;
	struct secdp_aux_dev *aux_dev = file->private_data;
	ssize_t res = 0;

	if (!aux_dev->secdp_get_hpd_status())
		return -ENODEV;

	if (!atomic_inc_not_zero(&aux_dev->usecount))
		return -ENODEV;

	bytes_pending = min((loff_t)count, AUX_MAX_OFFSET - *offset);

	if (!access_ok(VERIFY_READ, buf, bytes_pending)) {
		res = -EFAULT;
		goto out;
	}

	while (bytes_pending > 0) {
		uint8_t localbuf[DP_AUX_MAX_BUF];
		ssize_t todo = min_t(size_t, bytes_pending, sizeof(localbuf));

		if (signal_pending(current)) {
			res = num_bytes_processed ?
				num_bytes_processed : -ERESTARTSYS;
			goto out;
		}

		if (__copy_from_user(localbuf,
					buf + num_bytes_processed, todo)) {
			res = num_bytes_processed ?
				num_bytes_processed : -EFAULT;
			goto out;
		}

		if (aux_dev->cmd_type == DP_AUXCMD_NATIVE)
			res = aux_dev->secdp_dpcd_write(*offset, localbuf, todo);
		else
			res = aux_dev->secdp_i2c_write(localbuf, todo);

		if (res <= 0) {
			res = num_bytes_processed ? num_bytes_processed : res;
			goto out;
		}
		bytes_pending -= res;
		if (aux_dev->cmd_type == DP_AUXCMD_NATIVE)
			*offset += res;
		num_bytes_processed += res;
		res = num_bytes_processed;
	}

out:
	atomic_dec(&aux_dev->usecount);
	wake_up_atomic_t(&aux_dev->usecount);
	return res;
}

static int auxdev_release(struct inode *inode, struct file *file)
{
	struct secdp_aux_dev *aux_dev = file->private_data;

	kref_put(&aux_dev->refcount, release_secdp_aux_dev);
	return 0;
}

static long auxdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	s32 res = 0;
	u32 size = 0;
	char hpd;
	struct ioctl_auxdev_info info;
	struct secdp_aux_dev *aux_dev = file->private_data;

	pr_debug("+++\n");

	if (_IOC_TYPE(cmd) != IOCTL_MAGIC)
		return -EINVAL;
	if (_IOC_NR(cmd) >= IOCTL_MAXNR)
		return -EINVAL;

	size = sizeof(struct ioctl_auxdev_info);
	pr_info("cmd: %s\n", auxdev_ioctl_cmd_to_string(cmd));

	switch (cmd) {
	case IOCTL_DP_AUXCMD_TYPE:
		res = copy_from_user((void *)&info, (void *)arg, size);
		if (res) {
			pr_debug("error at copy_from_user, rc(%d)\n", res);
			break;
		}
		aux_dev->cmd_type = info.cmd_type;
		pr_info("auxcmd_type: %s\n", auxcmd_type_to_string(aux_dev->cmd_type));
		break;
	case IOCTL_DP_HPD_STATUS:
		hpd = aux_dev->secdp_get_hpd_status();
		res = copy_to_user((void *)arg, (void *)&hpd, 1);
		break;
	default:
		break;
	}

	return res;
}

static const struct file_operations auxdev_fops = {
	.owner		= THIS_MODULE,
	.llseek		= auxdev_llseek,
	.read		= auxdev_read,
	.write		= auxdev_write,
	.open		= auxdev_open,
	.release	= auxdev_release,
	.unlocked_ioctl	= auxdev_ioctl,
	.compat_ioctl	= auxdev_ioctl,
};

static int auxdev_wait_atomic_t(atomic_t *p)
{
	schedule();
	return 0;
}

void secdp_aux_unregister_devnode(struct secdp_aux_dev *aux_dev)
{
	unsigned int minor;

	mutex_lock(&aux_idr_mutex);
	idr_remove(&aux_idr, aux_dev->index);
	mutex_unlock(&aux_idr_mutex);

	atomic_dec(&aux_dev->usecount);
	wait_on_atomic_t(&aux_dev->usecount, auxdev_wait_atomic_t,
			TASK_UNINTERRUPTIBLE);

	minor = aux_dev->index;
	if (aux_dev->dev)
		device_destroy(secdp_aux_dev_class,
				MKDEV(drm_dev_major, minor));

	pr_info("secdp_aux_dev: aux unregistering\n");
	kref_put(&aux_dev->refcount, release_secdp_aux_dev);
}

int secdp_aux_create_node(struct secdp_aux_dev *aux_dev)
{
	int res;

	secdp_aux_dev_class = class_create(THIS_MODULE, "dp_sec_aux");
	if (IS_ERR(secdp_aux_dev_class))
		return -EINVAL;

	res = register_chrdev(0, "secdp_aux", &auxdev_fops);
	if (res < 0)
		goto out;

	drm_dev_major = res;
	aux_dev->dev = device_create(secdp_aux_dev_class, NULL,
			MKDEV(drm_dev_major, aux_dev->index), NULL,
			"secdp_aux");

	if (IS_ERR(aux_dev->dev)) {
		res = -EINVAL;
		aux_dev->dev = NULL;
		goto error;
	}

	return 0;
error:
	secdp_aux_unregister_devnode(aux_dev);
out:
	class_destroy(secdp_aux_dev_class);
	return res;
}

int secdp_aux_dev_init(ssize_t (*secdp_i2c_write)(void *buffer, size_t size),
		ssize_t (*secdp_i2c_read)(void *buffer, size_t size),
		ssize_t (*secdp_dpcd_write)(unsigned int offset, void *buffer, size_t size),
		ssize_t (*secdp_dpcd_read)(unsigned int offset,	void *buffer, size_t size),
		int (*secdp_get_hpd_status)(void))
{
	static bool check_init;
	struct secdp_aux_dev *aux_dev;
	int index;
	int ret;

	if (check_init)
		return 0;

	aux_dev = kzalloc(sizeof(*aux_dev), GFP_KERNEL);
	if (!aux_dev)
		return -ENOMEM;

	atomic_set(&aux_dev->usecount, 1);
	kref_init(&aux_dev->refcount);

	mutex_lock(&aux_idr_mutex);
	index = idr_alloc_cyclic(&aux_idr, aux_dev, 0, DRM_AUX_MINORS,
			GFP_KERNEL);
	mutex_unlock(&aux_idr_mutex);
	if (index < 0) {
		ret = index;
		goto error;
	}

	aux_dev->index = index;
	aux_dev->cmd_type = DP_AUXCMD_NATIVE;

	ret = secdp_aux_create_node(aux_dev);
	if (ret < 0)
		goto error;

	aux_dev->secdp_i2c_write = secdp_i2c_write;
	aux_dev->secdp_i2c_read = secdp_i2c_read;
	aux_dev->secdp_dpcd_write = secdp_dpcd_write;
	aux_dev->secdp_dpcd_read = secdp_dpcd_read;
	aux_dev->secdp_get_hpd_status = secdp_get_hpd_status;

	check_init = true;

	return 0;
error:
	kfree(aux_dev);
	return ret;
}

void secdp_aux_dev_exit(void)
{
	unregister_chrdev(drm_dev_major, "secdp_aux");
	class_destroy(secdp_aux_dev_class);
}

