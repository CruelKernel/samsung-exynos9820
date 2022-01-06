/*
 * Copyright (C) 2012-2018, Samsung Electronics Co., Ltd.
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

#include <linux/anon_inodes.h>
#include <linux/module.h>
#include <linux/poll.h>

#include "tzdev.h"
#include "tz_iwsock.h"

#define IS_EINTR(x)				\
	(((x) == -EINTR)			\
	|| ((x) == -ERESTARTSYS)		\
	|| ((x) == -ERESTARTNOHAND)		\
	|| ((x) == -ERESTARTNOINTR)		\
	|| ((x) == -ERESTART_RESTARTBLOCK))

static const struct file_operations tz_uiwsock_fops;

static int tz_uiwsock_open(struct inode *inode, struct file *filp)
{
	struct sock_desc *sd;

	sd = tz_iwsock_socket(0);
	if (IS_ERR(sd)) {
		tzdev_uiwsock_error("Failed to create new socket, ret=%ld\n", PTR_ERR(sd));
		return PTR_ERR(sd);
	}

	filp->private_data = sd;

	return 0;
}

static long tz_uiwsock_connect(struct file *filp, unsigned long arg)
{
	long ret;
	struct tz_uiwsock_connection connection;
	struct sock_desc *sd = filp->private_data;
	struct tz_uiwsock_connection __user *argp = (struct tz_uiwsock_connection __user *)arg;

	tzdev_uiwsock_debug("Enter, filp=%pK\n", filp);

	if (copy_from_user(&connection, argp, sizeof(struct tz_uiwsock_connection))) {
		tzdev_uiwsock_error("Invalid user space pointer %pK, filp=%pK\n", argp, filp);
		return -EFAULT;
	}

	connection.name[TZ_UIWSOCK_MAX_NAME_LENGTH - 1] = 0;

	ret = tz_iwsock_connect(sd, connection.name, 0);
	if (ret < 0) {
		tzdev_uiwsock_error("Failed to connect to socket %s, filp=%pK, error %ld\n", connection.name, filp, ret);
		return ret;
	}

	tzdev_uiwsock_debug("Exit, filp=%pK\n", filp);

	return ret;
}

static long tz_uiwsock_wait_connection(struct file *filp, unsigned long arg)
{
	long ret;
	struct sock_desc *sd = (struct sock_desc *)filp->private_data;

	ret = tz_iwsock_wait_connection(sd);
	if (IS_EINTR(ret))
		tzdev_uiwsock_debug("Wait interrupted, filp=%pK\n", filp);
	else if (ret < 0)
		tzdev_uiwsock_error("Failed to wait connection to socket, filp=%pK socket=%d ret=%ld\n",
				filp, sd->id, ret);

	tzdev_uiwsock_debug("Exit, filp=%pK\n", filp);

	return ret;
}

static long tz_uiwsock_listen(struct file *filp, unsigned long arg)
{
	long ret;
	struct tz_uiwsock_connection connection;
	struct sock_desc *sd = (struct sock_desc *)filp->private_data;
	struct tz_uiwsock_connection __user *argp =
				(struct tz_uiwsock_connection __user *)arg;

	tzdev_uiwsock_debug("Enter, filp=%pK\n", filp);

	if (copy_from_user(&connection, argp, sizeof(struct tz_uiwsock_connection))) {
		tzdev_uiwsock_error("Invalid user space pointer %pK, filp=%pK\n",
									argp, filp);
		return -EFAULT;
	}
	

	connection.name[TZ_UIWSOCK_MAX_NAME_LENGTH - 1] = 0;

	ret = tz_iwsock_listen(sd, connection.name);
	if (ret) {
		tzdev_uiwsock_error("Failed to listen, error %ld\n", ret);
		return ret;
	}

	tzdev_uiwsock_debug("Exit, filp=%pK\n", filp);

	return ret;
}

static long tz_uiwsock_accept(struct file *filp, unsigned long arg)
{
	struct sock_desc *sd = filp->private_data;
	struct sock_desc *new_sd;
	long ret;

	tzdev_uiwsock_debug("Enter, filp=%pK\n", filp);

	new_sd = tz_iwsock_accept(sd);
	if (IS_ERR(new_sd)) {
		tzdev_uiwsock_error("Failed to accept, error %ld\n", PTR_ERR(new_sd));
		return PTR_ERR(new_sd);
	}

	ret = anon_inode_getfd("[tz-uiwsock]", &tz_uiwsock_fops, new_sd, O_CLOEXEC);
	if (ret < 0) {
		tzdev_uiwsock_error("Failed to get new anon inode fd, ret=%ld\n", ret);
		tz_iwsock_release(new_sd);
	}

	tzdev_uiwsock_debug("Exit, filp=%pK\n", filp);

	return ret;
}

static long tz_uiwsock_send(struct file *filp, unsigned long arg)
{
	struct tz_uiwsock_data data;
	struct sock_desc *sd = (struct sock_desc *)filp->private_data;
	struct tz_uiwsock_data __user *argp = (struct tz_uiwsock_data __user *)arg;
	long ret;

	tzdev_uiwsock_debug("Enter, filp=%pK\n", filp);

	if (copy_from_user(&data, argp, sizeof(struct tz_uiwsock_data))) {
		tzdev_uiwsock_error("Invalid user space pointer %pK, filp=%pK\n", argp, filp);
		return -EFAULT;
	}

	ret = tz_iwsock_write(sd, (void __user *)(uintptr_t)data.buffer,
			data.size, data.flags);

	tzdev_uiwsock_debug("Exit, filp=%pK\n", filp);

	return ret;
}

static long tz_uiwsock_recv(struct file *filp, unsigned long arg)
{
	struct tz_uiwsock_data data;
	struct sock_desc *sd = (struct sock_desc *)filp->private_data;
	struct tz_uiwsock_data __user *argp = (struct tz_uiwsock_data __user *)arg;
	long ret;

	tzdev_uiwsock_debug("Enter, filp=%pK\n", filp);

	if (copy_from_user(&data, argp, sizeof(struct tz_uiwsock_data))) {
		tzdev_uiwsock_error("Invalid user space pointer %pK, filp=%pK\n", argp, filp);
		return -EFAULT;
	}

	ret = tz_iwsock_read(sd, (void __user *)(uintptr_t)data.buffer,
			data.size, data.flags);
	if (ret < 0)
		tzdev_uiwsock_error("Failed to read data ret=%ld, filp=%pK\n", ret, filp);

	tzdev_uiwsock_debug("Exit, filp=%pK\n", filp);

	return ret;
}

static long tz_uiwsock_getsockopt(struct file *filp, unsigned long arg)
{
	struct sock_desc *sd = (struct sock_desc *)filp->private_data;
	struct tz_uiwsock_sockopt __user *argp = (struct tz_uiwsock_sockopt __user *)arg;
	struct tz_uiwsock_sockopt sockopt;
	int ret;

	tzdev_uiwsock_debug("Enter, filp=%pK\n", filp);

	if (copy_from_user(&sockopt, argp, sizeof(struct tz_uiwsock_sockopt))) {
		tzdev_uiwsock_error("Invalid user space pointer %pK, filp=%pK\n",
				argp, filp);
		return -EFAULT;
	}

	ret = tz_iwsock_getsockopt(sd, sockopt.level, sockopt.optname,
			(void *)(uintptr_t)sockopt.optval,
			(socklen_t *)(uintptr_t)sockopt.optlen);

	tzdev_uiwsock_debug("Exit, filp=%pK\n", filp);

	return ret;
}

static long tz_uiwsock_setsockopt(struct file *filp, unsigned long arg)
{
	struct sock_desc *sd = (struct sock_desc *)filp->private_data;
	struct tz_uiwsock_sockopt sockopt;
	struct tz_uiwsock_sockopt __user *argp = (struct tz_uiwsock_sockopt __user *)arg;
	int ret;

	tzdev_uiwsock_debug("Enter, filp=%pK\n", filp);

	if (copy_from_user(&sockopt, argp, sizeof(struct tz_uiwsock_sockopt))) {
		tzdev_uiwsock_error("Invalid user space pointer %pK, filp=%pK\n",
				argp, filp);
		return -EFAULT;
	}

	ret = tz_iwsock_setsockopt(sd, sockopt.level, sockopt.optname,
			(void *)(uintptr_t)sockopt.optval, sockopt.optlen);

	tzdev_uiwsock_debug("Exit, filp=%pK\n", filp);

	return ret;
}

static long tz_uiwsock_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret;

	switch (cmd) {
	case TZIO_UIWSOCK_CONNECT:
		ret = tz_uiwsock_connect(filp, arg);
		break;
	case TZIO_UIWSOCK_WAIT_CONNECTION:
		ret = tz_uiwsock_wait_connection(filp, arg);
		break;
	case TZIO_UIWSOCK_SEND:
		ret = tz_uiwsock_send(filp, arg);
		break;
	case TZIO_UIWSOCK_RECV:
		ret = tz_uiwsock_recv(filp, arg);
		break;
	case TZIO_UIWSOCK_LISTEN:
		ret = tz_uiwsock_listen(filp, arg);
		break;
	case TZIO_UIWSOCK_ACCEPT:
		ret = tz_uiwsock_accept(filp, arg);
		break;
	case TZIO_UIWSOCK_GETSOCKOPT:
		ret = tz_uiwsock_getsockopt(filp, arg);
		break;
	case TZIO_UIWSOCK_SETSOCKOPT:
		ret = tz_uiwsock_setsockopt(filp, arg);
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	if (IS_EINTR(ret))
		ret = -ERESTARTNOINTR;

	return ret;
}

static int tz_uiwsock_release(struct inode *inode, struct file *filp)
{
	struct sock_desc *sd = (struct sock_desc *)filp->private_data;
	(void)inode;

	tzdev_uiwsock_debug("Enter, filp=%pK\n", filp);

	tz_iwsock_release(sd);

	tzdev_uiwsock_debug("Exit, filp=%pK\n", filp);

	return 0;
}

static unsigned int tz_uiwsock_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;
	struct sock_desc *sd = filp->private_data;

	poll_wait(filp, &sd->wq, wait);

	if (sd->state != TZ_SK_CONNECTED && sd->state != TZ_SK_LISTENING)
		return 0;

	switch (sd->iwd_buf->nwd_state) {
	case BUF_SK_NEW:
		break;
	case BUF_SK_CONNECTED:
		if (!circ_buf_is_empty(&sd->read_buf))
			mask |= POLLIN | POLLRDNORM;

		switch(sd->iwd_buf->swd_state) {
		case BUF_SK_NEW:
			break;
		case BUF_SK_CLOSED:
			mask |= POLLHUP;
			break;
		case BUF_SK_CONNECTED:
			if (!circ_buf_is_full(&sd->write_buf))
				mask |= POLLOUT | POLLWRNORM;

			if (sd->state != TZ_SK_LISTENING) {
				if (!circ_buf_is_full(&sd->oob_buf))
					mask |= POLLOUT | POLLWRBAND;
			}

			break;
		}
		break;

	case BUF_SK_CLOSED:
		BUG();
	}

	return mask;
}

static const struct file_operations tz_uiwsock_fops = {
	.owner = THIS_MODULE,
	.open = tz_uiwsock_open,
	.poll = tz_uiwsock_poll,
	.unlocked_ioctl = tz_uiwsock_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = tz_uiwsock_ioctl,
#endif /* CONFIG_COMPAT */
	.release = tz_uiwsock_release,
};

static struct tz_cdev tz_uiwsock_cdev = {
	.name = "tziwsock",
	.fops = &tz_uiwsock_fops,
	.owner = THIS_MODULE,
};

int tz_uiwsock_init(void)
{
	int ret;

	ret = tz_cdev_register(&tz_uiwsock_cdev);
	if (ret) {
		tzdev_print(0, "failed to register iwsock device, error=%d\n", ret);
		return ret;
	}

	return 0;
}

void tz_uiwsock_fini(void)
{
	tz_cdev_unregister(&tz_uiwsock_cdev);
}
