/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <asm/uaccess.h>

#include "vision-config.h"
#include "vision-dev.h"

static struct vision_device *vision_device[VISION_NUM_DEVICES];
static DEFINE_MUTEX(visiondev_lock);

struct vision_device *vision_devdata(struct file *file)
{
	return vision_device[iminor(file_inode(file))];
}

static int vision_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct vision_device *vdev;

	mutex_lock(&visiondev_lock);

	vdev = vision_devdata(file);
	if (!vdev) {
		mutex_unlock(&visiondev_lock);
		vision_err("vision device is NULL\n");
		ret = -ENODEV;
		goto p_err;
	}

	if (!(vdev->flags & (1 << VISION_DEVICE_TYPE_VERTEX))) {
		mutex_unlock(&visiondev_lock);
		vision_err("[V] vision device is not registered\n");
		ret = -ENODEV;
		goto p_err;
	}

	get_device(&vdev->dev);
	mutex_unlock(&visiondev_lock);

	ret = (vdev->fops->open ? vdev->fops->open(file) : -EINVAL);
	if (ret) {
		put_device(&vdev->dev);
		goto p_err;
	}

p_err:
	return ret;
}

static int vision_release(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct vision_device *vdev = vision_devdata(file);

	ret = (vdev->fops->release ? vdev->fops->release(file) : -EINVAL);
	put_device(&vdev->dev);

	return ret;
}

static long vision_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct vision_device *vdev = vision_devdata(file);
	struct mutex *lock = vdev->lock;

	if (!vdev->fops->ioctl) {
		vision_err("ioctl is not registered\n");
		ret = -ENOTTY;
		goto p_err;
	}

	if (lock && mutex_lock_interruptible(lock))
		return -ERESTARTSYS;

	ret = vdev->fops->ioctl(file, cmd, arg);

	if (lock)
		mutex_unlock(lock);

p_err:
	return ret;
}

static long vision_compat_ioctl32(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct vision_device *vdev = vision_devdata(file);
	struct mutex *lock = vdev->lock;

	if (!vdev->fops->compat_ioctl) {
		vision_err("compat_ioctl is not registered\n");
		ret = -ENOTTY;
		goto p_err;
	}

	if (lock && mutex_lock_interruptible(lock))
		return -ERESTARTSYS;

	ret = vdev->fops->compat_ioctl(file, cmd, arg);

	if (lock)
		mutex_unlock(lock);

p_err:
	return ret;
}

static unsigned int vision_poll(struct file *file, struct poll_table_struct *poll)
{
	struct vision_device *vdev = vision_devdata(file);

	if (!vdev->fops->poll)
		return DEFAULT_POLLMASK;

	return vdev->fops->poll(file, poll);
}

static const struct file_operations vision_fops = {
	.owner = THIS_MODULE,
	.open = vision_open,
	.release = vision_release,
	.unlocked_ioctl = vision_ioctl,
	.compat_ioctl = vision_compat_ioctl32,
	.poll = vision_poll,
	.llseek = no_llseek,
};

static ssize_t index_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	struct vision_device *vdev = container_of(dev, struct vision_device, dev);

	return sprintf(buf, "%i\n", vdev->index);
}
static DEVICE_ATTR_RO(index);

static ssize_t debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct vision_device *vdev = container_of(dev, struct vision_device, dev);

	return sprintf(buf, "%i\n", vdev->debug);
}

static ssize_t debug_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len)
{
	struct vision_device *vdev = container_of(dev, struct vision_device, dev);
	int res = 0;
	u16 value;

	res = kstrtou16(buf, 0, &value);
	if (res) {
		vision_err("kstrtou16 is fail\n");
		return res;
	}

	vdev->debug = value;
	return len;
}
static DEVICE_ATTR_RW(debug);

static ssize_t name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct vision_device *vdev = container_of(dev, struct vision_device, dev);

	return sprintf(buf, "%.*s\n", (int)sizeof(vdev->name), vdev->name);
}
static DEVICE_ATTR_RO(name);

static struct attribute *vision_device_attrs[] = {
	&dev_attr_name.attr,
	&dev_attr_debug.attr,
	&dev_attr_index.attr,
	NULL,
};
ATTRIBUTE_GROUPS(vision_device);

static struct class vision_class = {
	.name = VISION_NAME,
	.dev_groups = vision_device_groups,
};

static void vision_device_release(struct device *dev)
{
	struct vision_device *vdev = container_of(dev, struct vision_device, dev);

	mutex_lock(&visiondev_lock);
	if (WARN_ON(vision_device[vdev->minor] != vdev)) {
		/* should not happen */
		mutex_unlock(&visiondev_lock);
		return;
	}

	/* Free up this device for reuse */
	vision_device[vdev->minor] = NULL;

	/* Delete the cdev on this minor as well */
	cdev_del(vdev->cdev);
	/* Just in case some driver tries to access this from
	   the release() callback. */
	vdev->cdev = NULL;

	mutex_unlock(&visiondev_lock);

	/* Release video_device and perform other
	   cleanups as needed. */
	vdev->release(vdev);
}

int vision_register_device(struct vision_device *vdev, int minor, struct module *owner)
{
	int ret = 0;
	const char *name_base;

	BUG_ON(!vdev->parent);
	WARN_ON(vision_device[minor] != NULL);

	vdev->cdev = NULL;

	switch (vdev->type) {
	case VISION_DEVICE_TYPE_VERTEX:
		name_base = "vertex";
		break;
	default:
		printk(KERN_ERR "%s called with unknown type: %d\n", __func__, vdev->type);
		return -EINVAL;
	}

	/* Part 3: Initialize the character device */
	vdev->cdev = cdev_alloc();
	if (vdev->cdev == NULL) {
		ret = -ENOMEM;
		goto cleanup;
	}
	vdev->cdev->ops = &vision_fops;
	vdev->cdev->owner = owner;
	ret = cdev_add(vdev->cdev, MKDEV(VISION_MAJOR, minor), 1);
	if (ret < 0) {
		printk(KERN_ERR "%s: cdev_add failed\n", __func__);
		kfree(vdev->cdev);
		vdev->cdev = NULL;
		goto cleanup;
	}

	/* Part 4: register the device with sysfs */
	vdev->dev.class = &vision_class;
	vdev->dev.parent = vdev->parent;
	vdev->dev.devt = MKDEV(VISION_MAJOR, minor);
	dev_set_name(&vdev->dev, "%s%d", name_base, minor);
	ret = device_register(&vdev->dev);
	if (ret < 0) {
		printk(KERN_ERR "%s: device_register failed\n", __func__);
		goto cleanup;
	}
	/* Register the release callback that will be called when the last
	   reference to the device goes away. */
	vdev->dev.release = vision_device_release;


	/* Part 6: Activate this minor. The char device can now be used. */
	set_bit(VISION_FL_REGISTERED, &vdev->flags);

	mutex_lock(&visiondev_lock);
	vdev->minor = minor;
	vision_device[minor] = vdev;
	mutex_unlock(&visiondev_lock);

	return 0;

cleanup:
	mutex_lock(&visiondev_lock);
	if (vdev->cdev)
		cdev_del(vdev->cdev);
	/* Mark this video device as never having been registered. */
	vdev->minor = -1;
	mutex_unlock(&visiondev_lock);
	return ret;
}
EXPORT_SYMBOL(vision_register_device);

static int __init visiondev_init(void)
{
	int ret;
	dev_t dev = MKDEV(VISION_MAJOR, 0);

	printk(KERN_INFO "Linux vision interface: v1.00\n");
	ret = register_chrdev_region(dev, VISION_NUM_DEVICES, VISION_NAME);
	if (ret < 0) {
		printk(KERN_WARNING "videodev: unable to get major %d\n", VISION_MAJOR);
		return ret;
	}

	ret = class_register(&vision_class);
	if (ret < 0) {
		unregister_chrdev_region(dev, VISION_NUM_DEVICES);
		printk(KERN_WARNING "video_dev: class_register failed\n");
		return -EIO;
	}

	return 0;
}

static void __exit visiondev_exit(void)
{
	dev_t dev = MKDEV(VISION_MAJOR, 0);

	class_unregister(&vision_class);
	unregister_chrdev_region(dev, VISION_NUM_DEVICES);
}

subsys_initcall(visiondev_init);
module_exit(visiondev_exit)

MODULE_AUTHOR("Gilyeon Lim <kilyeon.im@samsung.com>");
MODULE_DESCRIPTION("Device registrar for Vision4Linux drivers v1");
MODULE_LICENSE("GPL");
MODULE_ALIAS_CHARDEV_MAJOR(VISION_MAJOR);
