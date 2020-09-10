/*
 * driver/ccic/ccic_misc.c - SEC CCIC MISC driver
 *
 * Copyright (C) 2017 Samsung Electronics
 * Author: Wookwang Lee <wookwang.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/ccic/ccic_core.h>

static struct ccic_misc_dev *c_dev;

#define MAX_BUF 255
#define NODE_OF_MISC "ccic_misc"
#define CCIC_IOCTL_UVDM _IOWR('C', 0, struct uvdm_data)

static inline int _lock(atomic_t *excl)
{
	if (atomic_inc_return(excl) == 1)
		return 0;
	else {
		atomic_dec(excl);
		return -1;
	}
}

static inline void _unlock(atomic_t *excl)
{
	atomic_dec(excl);
}

static int ccic_misc_open(struct inode *inode, struct file *file)
{
	int ret = 0;

	pr_info("%s + open success\n", __func__);
	if (!c_dev) {
		pr_err("%s - error : c_dev is NULL\n", __func__);
		ret = -ENODEV;
		goto err;
	}

	if (_lock(&c_dev->open_excl)) {
		pr_err("%s - error : device busy\n", __func__);
		ret = -EBUSY;
		goto err;
	}

	/* stop direct charging(pps) for uvdm and wait latest psrdy done for 1 second */
	if (c_dev->pps_control) {
		if (!c_dev->pps_control(0)) {
			_unlock(&c_dev->open_excl);
			pr_err("%s - error : psrdy is not done\n", __func__);
			ret = -EBUSY;
			goto err;
		}
	}

	/* check if there is some connection */
	if (!c_dev->uvdm_ready()) {
		_unlock(&c_dev->open_excl);
		pr_err("%s - error : uvdm is not ready\n", __func__);
		ret = -EBUSY;
		goto err;
	}

	pr_info("%s - open success\n", __func__);

	return 0;
err:
	return ret;
}

static int ccic_misc_close(struct inode *inode, struct file *file)
{
	if (c_dev)
		_unlock(&c_dev->open_excl);
	c_dev->uvdm_close();
	if (c_dev->pps_control)
		c_dev->pps_control(1); /* start direct charging(pps) */

	pr_info("%s - close success\n", __func__);
	return 0;
}

static int send_uvdm_message(void *data, int size)
{
	int ret;

	ret = c_dev->uvdm_write(data, size);
	pr_info("%s - size : %d, ret : %d\n", __func__, size, ret);
	return ret;
}

static int receive_uvdm_message(void *data, int size)
{
	int ret;

	ret = c_dev->uvdm_read(data);
	pr_info("%s - size : %d, ret : %d\n", __func__, size, ret);
	return ret;
}

static long
ccic_misc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	void *buf = NULL;

	if (_lock(&c_dev->ioctl_excl)) {
		pr_err("%s - error : ioctl busy - cmd : %d\n", __func__, cmd);
		ret = -EBUSY;
		goto err2;
	}

	if (!c_dev->uvdm_ready()) {
		pr_err("%s - error : uvdm is not ready\n", __func__);
		ret = -EACCES;
		goto err1;
	}

	switch (cmd) {
	case CCIC_IOCTL_UVDM:
		pr_info("%s - CCIC_IOCTL_UVDM cmd\n", __func__);
		if (copy_from_user(&c_dev->u_data,
				(void __user *) arg, sizeof(struct uvdm_data))) {
			ret = -EIO;
			pr_err("%s - copy_from_user error\n", __func__);
			goto err1;
		}

		buf = kzalloc(MAX_BUF, GFP_KERNEL);
		if (!buf) {
			ret = -EINVAL;
			pr_err("%s - kzalloc error\n", __func__);
			goto err1;
		}

		if (c_dev->u_data.size > MAX_BUF) {
			ret = -ENOMEM;
			pr_err("%s - user data size is %d error\n",
					__func__, c_dev->u_data.size);
			goto err;
		}

		if (c_dev->u_data.dir == DIR_OUT) {
			if (copy_from_user(buf, c_dev->u_data.pData, c_dev->u_data.size)) {
				ret = -EIO;
				pr_err("%s - copy_from_user error\n", __func__);
				goto err;
			}
			ret = send_uvdm_message(buf, c_dev->u_data.size);
			if (ret < 0) {
				pr_err("%s - send_uvdm_message error\n", __func__);
				ret = -EINVAL;
				goto err;
			}
		} else {
			ret = receive_uvdm_message(buf, c_dev->u_data.size);
			if (ret < 0) {
				pr_err("%s - receive_uvdm_message error\n", __func__);
				ret = -EINVAL;
				goto err;
			}
			if (copy_to_user((void __user *)c_dev->u_data.pData,
					 buf, ret)) {
				ret = -EIO;
				pr_err("%s - copy_to_user error\n", __func__);
				goto err;
			}
		}
		break;

	default:
		pr_err("%s - unknown ioctl cmd : %d\n", __func__, cmd);
		ret = -ENOIOCTLCMD;
		goto err;
	}
err:
	kfree(buf);
err1:
	_unlock(&c_dev->ioctl_excl);
err2:
	return ret;
}

#ifdef CONFIG_COMPAT
static long
ccic_misc_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	pr_info("%s - cmd : %d\n", __func__, cmd);
	ret = ccic_misc_ioctl(file, cmd, (unsigned long)compat_ptr(arg));

	return ret;
}
#endif

static const struct file_operations ccic_misc_fops = {
	.owner		= THIS_MODULE,
	.open		= ccic_misc_open,
	.release	= ccic_misc_close,
	.llseek		= no_llseek,
	.unlocked_ioctl = ccic_misc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = ccic_misc_compat_ioctl,
#endif
};

static struct miscdevice ccic_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name	= NODE_OF_MISC,
	.fops	= &ccic_misc_fops,
};

int ccic_misc_init(pccic_data_t pccic_data)
{
	int ret = 0;

	ret = misc_register(&ccic_misc_device);
	if (ret) {
		pr_err("%s - return error : %d\n", __func__, ret);
		goto err;
	}

	c_dev = kzalloc(sizeof(struct ccic_misc_dev), GFP_KERNEL);
	if (!c_dev) {
		ret = -ENOMEM;
		pr_err("%s - kzalloc failed : %d\n", __func__, ret);
		goto err1;
	}
	atomic_set(&c_dev->open_excl, 0);
	atomic_set(&c_dev->ioctl_excl, 0);

	if (pccic_data)
		pccic_data->misc_dev = c_dev;

	pr_info("%s - register success\n", __func__);
	return 0;
err1:
	misc_deregister(&ccic_misc_device);
err:
	return ret;
}
EXPORT_SYMBOL(ccic_misc_init);

void ccic_misc_exit(void)
{
	pr_info("%s() called\n", __func__);
	if (!c_dev)
		return;
	kfree(c_dev);
	misc_deregister(&ccic_misc_device);
}
EXPORT_SYMBOL(ccic_misc_exit);
