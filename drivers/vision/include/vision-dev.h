/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VISION_DEV_H_
#define VISION_DEV_H_

#include <linux/fs.h>
#include <linux/device.h>

#define VISION_NUM_DEVICES	256
#define VISION_MAJOR		82
#define VISION_NAME		"vision4linux"

enum VISION_DEVICE_TYPE {
	VISION_DEVICE_TYPE_VERTEX
};

struct vision_file_ops {
	struct module *owner;	int (*open)(struct file *);
	int (*release)(struct file *);
	unsigned int (*poll)(struct file *, struct poll_table_struct *);
	long (*ioctl)(struct file *, unsigned int, unsigned long);
	long (*compat_ioctl)(struct file *file, unsigned int, unsigned long);
};

enum VISION_FLAGS {
	VISION_FL_REGISTERED
};

struct vision_device {
	int				minor;
	char				name[32];
	unsigned long			flags;
	int				index;
	u32				type;

	/* device ops */
	const struct vision_file_ops	*fops;

	/* sysfs */
	struct device			dev;
	struct cdev			*cdev;
	struct device			*parent;

	/* vb2_queue associated with this device node. May be NULL. */
	//struct vb2_queue *queue;


	int debug;			/* Activates debug level*/

	/* callbacks */
	void (*release)(struct vision_device *vdev);

	/* ioctl callbacks */
	const struct vertex_ioctl_ops *ioctl_ops;
	//DECLARE_BITMAP(valid_ioctls, BASE_VIDIOC_PRIVATE);

	/* serialization lock */
	//DECLARE_BITMAP(disable_locking, BASE_VIDIOC_PRIVATE);
	struct mutex *lock;
};

struct vision_device *vision_devdata(struct file *file);
int vision_register_device(struct vision_device *vdev, int minor, struct module *owner);

#endif