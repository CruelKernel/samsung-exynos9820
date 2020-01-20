/*
 * Copyright (C) 2016 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/fs.h>
#include "tz_cdev.h"

static atomic_t tz_cdev_state = ATOMIC_INIT(0);

int tz_cdev_register(struct tz_cdev *cdev)
{
	int err;

	err = alloc_chrdev_region(&cdev->dev, 0, 1, cdev->name);
	if (unlikely(err))
		return err;

	cdev->class = class_create(cdev->owner, cdev->name);
	if (IS_ERR(cdev->class)) {
		err = PTR_ERR(cdev->class);
		goto class_create_failed;
	}

	cdev_init(&cdev->cdev, cdev->fops);

	err = cdev_add(&cdev->cdev, cdev->dev, 1);
	if (unlikely(err))
		goto cdev_add_failed;

	cdev->device = device_create(cdev->class,
			NULL, cdev->dev, NULL, cdev->name);
	if (IS_ERR(cdev->device)) {
		err = PTR_ERR(cdev->device);
		goto device_create_failed;
	}

	atomic_set(&tz_cdev_state, 1);

	return 0;

device_create_failed:
	cdev_del(&cdev->cdev);
cdev_add_failed:
	class_destroy(cdev->class);
class_create_failed:
	unregister_chrdev_region(cdev->dev, 1);

	return err;
}

void tz_cdev_unregister(struct tz_cdev *cdev)
{
	if (!atomic_cmpxchg(&tz_cdev_state, 1, 0))
		return;

	device_destroy(cdev->class, cdev->dev);
	cdev_del(&cdev->cdev);
	class_destroy(cdev->class);
	unregister_chrdev_region(cdev->dev, 1);
}
