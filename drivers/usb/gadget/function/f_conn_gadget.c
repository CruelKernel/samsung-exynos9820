/*
 * Gadget Driver for Android Connectivity Gadget
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 * Copyright (C) 2013 DEVGURU CO.,LTD.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ChangeLog:
 *      20130819 - rename 'adb' to 'conn_gadget', shortname to 'android_ssusbconn'
 *      20130821 - fixed return actual read size
 *      20130822 - rework with 3.4.39 version's adb base source.
 *      20130822 - remove unused callbacks
 *      20130913 - add async read logic
 *      20130913 - use kfifo as read buffer queue
 *      20130913 - add polling handler
 *      20130914 - fix ep read condition check mistake
 *      20130923 - change CONN_GADGET_BULK_BUFFER_SIZE to 32KB
 *      20131004 - support USB 3.0 (SuperSpeed)
 *      20131009 - change  CONN_GADGET_BULK_BUFFER_SIZE to 4KB
 *      20131010 - fix typo. SuperSpeed support related.
 *      20140113 - expose 'usb_buffer_size' attribute
 *      20140113 - change kmalloc to vmalloc for kfifo buffer
 *      20140211 - support multiple read URB.request
 *      20140221 - remove fifo_lock
 * 		- disable change transfer_size when online
 *      20140228 - add android_usb_function callback registration code.
 *		- conditional conn_gadget_shortname for `android` and `tizen`
 *      20140311 - add ioctl to communicate to userland application
 *      20140318 - add flush to wakeup ioctl sleep before handle close
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/file.h>
#include <linux/configfs.h>
#include <linux/usb/composite.h>

/* platform specific definitions */
/* ex) #define __ANDROID__ */

/* platform specific pre-processing */
#define CONN_GADGET_SHORTNAME "android_ssusbcon"

/* logging macros */
#define CONN_GADGET_ERR(_fmt_, _arg_...)	printk(KERN_ERR "%s() " _fmt_, __func__, ## _arg_)
#define CONN_GADGET_DBG(_fmt_, _arg_...)	//printk(KERN_ERR "%s() " _fmt_, __func__, ## _arg_)

/* number of tx requests to allocate */
#define CONN_GADGET_TX_REQ_MAX 4
#define CONN_GADGET_RX_REQ_MAX 4

#define CONN_GADGET_DEFAULT_TRANSFER_SIZE	(4 * 1024)
#define CONN_GADGET_DEFAULT_Q_SIZE 		(16 * (CONN_GADGET_RX_REQ_MAX))
#define MAX_INST_NAME_LEN	40

/* ioctl */
#include "f_conn_gadget.ioctl.h"

static const char conn_gadget_shortname[] = CONN_GADGET_SHORTNAME;

struct conn_gadget_dev {
	struct usb_function function;
	struct usb_composite_dev *cdev;
	spinlock_t lock;

	struct usb_ep *ep_in;
	struct usb_ep *ep_out;

	int online;
	int error;


	atomic_t read_excl;
	atomic_t write_excl;
	atomic_t open_excl;

	struct list_head tx_idle;
	struct list_head rx_idle;
	struct list_head rx_busy;

	wait_queue_head_t read_wq;
	wait_queue_head_t write_wq;

	struct kfifo rd_queue;
	void  *rd_queue_buf;

	int transfer_size;
	int rd_queue_size; //byte

	wait_queue_head_t ioctl_wq;

	/* store privious `online` value
	 * for notificate to app about bind/unbind state
	 * through IOCTL */
	int memorized;

	/* flag variable that save flush call status
	* to check wakeup reason */
	atomic_t flush;
};

static struct usb_interface_descriptor conn_gadget_interface_desc = {
	.bLength                = USB_DT_INTERFACE_SIZE,
	.bDescriptorType        = USB_DT_INTERFACE,
	.bInterfaceNumber       = 0,
	.bNumEndpoints          = 2,
	.bInterfaceClass        = 0xFF,
	.bInterfaceSubClass     = 0x40,
	.bInterfaceProtocol     = 1,
};

static struct usb_endpoint_descriptor conn_gadget_superspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(1024),
};

static struct usb_endpoint_descriptor conn_gadget_superspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor conn_gadget_superspeed_bulk_comp_desc = {
    .bLength                = sizeof(conn_gadget_superspeed_bulk_comp_desc),
    .bDescriptorType        = USB_DT_SS_ENDPOINT_COMP,
    /* the following 2 values can be tweaked if necessary */
    /* .bMaxBurst           = 0, */
    /* .bmAttributes        = 0, */
};

static struct usb_endpoint_descriptor conn_gadget_highspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor conn_gadget_highspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor conn_gadget_fullspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor conn_gadget_fullspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *fs_conn_gadget_descs[] = {
	(struct usb_descriptor_header *) &conn_gadget_interface_desc,
	(struct usb_descriptor_header *) &conn_gadget_fullspeed_in_desc,
	(struct usb_descriptor_header *) &conn_gadget_fullspeed_out_desc,
	NULL,
};

static struct usb_descriptor_header *hs_conn_gadget_descs[] = {
	(struct usb_descriptor_header *) &conn_gadget_interface_desc,
	(struct usb_descriptor_header *) &conn_gadget_highspeed_in_desc,
	(struct usb_descriptor_header *) &conn_gadget_highspeed_out_desc,
	NULL,
};

static struct usb_descriptor_header *ss_conn_gadget_descs[] = {
    (struct usb_descriptor_header *) &conn_gadget_interface_desc,
	(struct usb_descriptor_header *) &conn_gadget_superspeed_in_desc,
	(struct usb_descriptor_header *) &conn_gadget_superspeed_bulk_comp_desc,
	(struct usb_descriptor_header *) &conn_gadget_superspeed_out_desc,
	(struct usb_descriptor_header *) &conn_gadget_superspeed_bulk_comp_desc,
    NULL,
};


/* temporary variable used between conn_gadget_open() and conn_gadget_gadget_bind() */
static struct conn_gadget_dev *_conn_gadget_dev;

struct conn_gadget_instance {
	struct usb_function_instance func_inst;
	const char *name;
};


static inline struct conn_gadget_dev *func_to_conn_gadget(struct usb_function *f)
{
	return container_of(f, struct conn_gadget_dev, function);
}


static struct usb_request *conn_gadget_request_new(struct usb_ep *ep, int buffer_size)
{
	struct usb_request *req = usb_ep_alloc_request(ep, GFP_KERNEL);
	if (!req)
		return NULL;

	/* now allocate buffers for the requests */
	req->buf = kmalloc(buffer_size, GFP_KERNEL);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	return req;
}

static void conn_gadget_request_free(struct usb_request *req, struct usb_ep *ep)
{
	if (req) {
		kfree(req->buf);
		usb_ep_free_request(ep, req);
	}
}

static inline int conn_gadget_lock(atomic_t *excl)
{
	if (atomic_inc_return(excl) == 1) {
		return 0;
	} else {
		atomic_dec(excl);
		return -1;
	}
}

static inline void conn_gadget_unlock(atomic_t *excl)
{
	atomic_dec(excl);
}

/* add a request to the tail of a list */
void conn_gadget_req_put(struct conn_gadget_dev *dev, struct list_head *head,
		struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&dev->lock, flags);
}

/* remove a request from the head of a list */
struct usb_request *conn_gadget_req_get(struct conn_gadget_dev *dev, struct list_head *head)
{
	unsigned long flags;
	struct usb_request *req;

	spin_lock_irqsave(&dev->lock, flags);
	if (list_empty(head)) {
		req = 0;
	} else {
		req = list_first_entry(head, struct usb_request, list);
		list_del(&req->list);
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return req;
}

static int conn_gadget_request_ep_out(struct conn_gadget_dev *dev)
{
	struct usb_request *req;
	int ret;

		while ((req = conn_gadget_req_get(dev, &dev->rx_idle))) {
		req->length = dev->transfer_size;
		ret = usb_ep_queue(dev->ep_out, req, GFP_ATOMIC);
		if (ret < 0) {
			CONN_GADGET_ERR("failed to queue req %p (%d)\n", req, ret);
			conn_gadget_req_put(dev, &dev->rx_idle, req);
			break;
		} else {
			conn_gadget_req_put(dev, &dev->rx_busy, req);
			CONN_GADGET_DBG("rx %p queue\n", req);
		}
	}

	return 0;
}

/* remove a request from it's list and add a request to other list */
void conn_gadget_req_move(struct conn_gadget_dev *dev, struct list_head *from_head,
		struct list_head *to_head, struct usb_request *req)
{
	unsigned long flags;
	(void)(from_head); //not used. but for readability

	spin_lock_irqsave(&dev->lock, flags);
	list_move_tail(&req->list, to_head);
	spin_unlock_irqrestore(&dev->lock, flags);
}

/* check state of list
 return value:
  - empty : 1
 */
int conn_gadget_empty(struct conn_gadget_dev *dev, struct list_head *head)
{
	int empty = 0;
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	if (list_empty(head)) {
		empty = 1;
	}
	spin_unlock_irqrestore(&dev->lock, flags);

	return empty;
}




static void conn_gadget_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct conn_gadget_dev *dev = _conn_gadget_dev;

	CONN_GADGET_DBG("status %d\n", req->status);

	if (req->status != 0) {
		dev->error = 1;
		pr_debug("%s: error %d\n", __func__, dev->error);
		CONN_GADGET_ERR("req->status f %d\n", req->status);
	}

	conn_gadget_req_put(dev, &dev->tx_idle, req);

	wake_up(&dev->write_wq);
}

static void conn_gadget_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	struct conn_gadget_dev *dev = _conn_gadget_dev;
	int high_water = (dev->rd_queue_size - (dev->transfer_size * (CONN_GADGET_RX_REQ_MAX+1)));
	int ret;

	CONN_GADGET_DBG("enter\n");

	if (req->status != 0) {
		if (req->status != -ECONNRESET) {
			dev->error = 1;
			pr_debug("%s: error %d\n", __func__, dev->error);
		}

		CONN_GADGET_ERR("req->status f %d\n", req->status);
		conn_gadget_req_move(dev, &dev->rx_busy, &dev->rx_idle, req); //move req from rx_busy list to rx_idle
		goto done;
	} else {
		kfifo_in(&dev->rd_queue, req->buf, req->actual);

	}

	//if queued buffer has overflow possibility, then exit function
	if (kfifo_len(&dev->rd_queue) >= high_water) {
		CONN_GADGET_DBG("kfifo_len is too high(%d)\n", high_water);
		conn_gadget_req_move(dev, &dev->rx_busy, &dev->rx_idle, req);
		goto done;
	}

	ret = usb_ep_queue(dev->ep_out, req, GFP_ATOMIC);
	if (ret < 0) {
		CONN_GADGET_ERR("failed to queue req %p (%d)\n", req, ret);
		conn_gadget_req_move(dev, &dev->rx_busy, &dev->rx_idle, req);
		goto done;
	} else {
		CONN_GADGET_DBG("rx %p queue\n", req);
	}

done:
	CONN_GADGET_DBG("rd_wq wkup\n");
	wake_up(&dev->read_wq);
}

static int conn_gadget_create_bulk_endpoints(struct conn_gadget_dev *dev,
				struct usb_endpoint_descriptor *in_desc,
				struct usb_endpoint_descriptor *out_desc)
{
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *req;
	struct usb_ep *ep;
	int i;

	pr_debug("create_bulk_endpoints dev: %p\n", dev);

	ep = usb_ep_autoconfig(cdev->gadget, in_desc);
	if (!ep) {
		printk(KERN_ERR "usb_ep_autoconfig for ep_in failed\n");
		return -ENODEV;
	}
	pr_debug("usb_ep_autoconfig for ep_in got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_in = ep;

	ep = usb_ep_autoconfig(cdev->gadget, out_desc);
	if (!ep) {
		printk(KERN_ERR "usb_ep_autoconfig for ep_out failed\n");
		return -ENODEV;
	}
	pr_debug("usb_ep_autoconfig for conn_gadget ep_out got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_out = ep;

	/* now allocate requests for our endpoints */
	for (i = 0; i < CONN_GADGET_RX_REQ_MAX; i++) {
		req = conn_gadget_request_new(dev->ep_out,
				(dev->transfer_size)?dev->transfer_size:CONN_GADGET_DEFAULT_TRANSFER_SIZE); //use default value
		if (!req)
			goto fail;
		req->complete = conn_gadget_complete_out;
		conn_gadget_req_put(dev, &dev->rx_idle, req);
	}

	for (i = 0; i < CONN_GADGET_TX_REQ_MAX; i++) {
		req = conn_gadget_request_new(dev->ep_in,
				(dev->transfer_size)?dev->transfer_size:CONN_GADGET_DEFAULT_TRANSFER_SIZE); //use default value
		if (!req)
			goto fail;
		req->complete = conn_gadget_complete_in;
		conn_gadget_req_put(dev, &dev->tx_idle, req);
	}

	return 0;

fail:
	CONN_GADGET_ERR("conn_gadget_bind() could not allocate requests\n");
	return -1;
}

static unsigned int conn_gadget_poll(struct file *fp, poll_table *wait)
{
	unsigned int mask;
	struct conn_gadget_dev *dev = fp->private_data;

	CONN_GADGET_DBG("enter\n");

	poll_wait(fp, &dev->read_wq, wait);
	poll_wait(fp, &dev->write_wq, wait);

	mask = 0;

	if (!_conn_gadget_dev) {
		CONN_GADGET_ERR("_conn_gadget_dev is NULL\n");
		mask |= (POLLNVAL | POLLERR);
		return mask;
	}

	if (!_conn_gadget_dev->online) {
		CONN_GADGET_ERR("_conn_gadget_dev is offlined\n");
		return mask;
	}

	if (!conn_gadget_lock(&dev->read_excl)) {

		unsigned int len = kfifo_len(&dev->rd_queue);
		if (len)
			mask |= (POLLIN | POLLRDNORM);

		conn_gadget_unlock(&dev->read_excl);
	}

	if (!conn_gadget_empty(dev, &dev->tx_idle))
		mask |= (POLLOUT | POLLWRNORM);

	CONN_GADGET_DBG("exit\n");
	return mask;
}

static ssize_t conn_gadget_read(struct file *fp, char __user *buf,
				size_t count, loff_t *pos)
{
	struct conn_gadget_dev *dev = fp->private_data;
	int r = count, xfer;
	int ret;

	CONN_GADGET_DBG("conn_gadget_read(%d)\n", count);

	if (!_conn_gadget_dev) {
		CONN_GADGET_ERR("_conn_gadget_dev is NULL\n");
		return -ENODEV;
	}

	if (count >= dev->transfer_size) {
		CONN_GADGET_ERR("count is too large (%d)\n", (int)count);
		return -EINVAL;
	}

	if (conn_gadget_lock(&dev->read_excl)) {
		CONN_GADGET_ERR("conn_gadget_lock(read_excl) f\n");
		return -EBUSY;
	}

	/* we will block until we're online */
	while (!(dev->online || dev->error)) {
		CONN_GADGET_ERR("waiting for online state\n");
		ret = wait_event_interruptible(dev->read_wq,
				(dev->online || dev->error));
		if (ret < 0) {
			CONN_GADGET_ERR("wait_event_interruptible f %d\n", ret);
			conn_gadget_unlock(&dev->read_excl);
			return ret;
		}
	}
	if (dev->error) {
		r = -EIO;
		CONN_GADGET_ERR("dev->error has value\n");
		goto done;
	}

	//if there is a ready buffer, then copy to user.
	do {
		xfer = kfifo_len(&dev->rd_queue);
		if (!xfer) {
			break;
		}
		xfer = (xfer < count) ? xfer : count;
		ret = kfifo_to_user(&dev->rd_queue, buf, xfer, &r); //assign copied byte to r
	} while (0);

	if (!xfer) {
		//r = -EAGAIN;
		r = 0;
		CONN_GADGET_ERR("zero queue\n");
		goto req;
	}

	if (ret < 0) {
		r = -EFAULT;
		CONN_GADGET_ERR("kfifo_to_user f %d\n", ret);
		goto done;
	}

	//if queued buffer is less than half, then usb_ep_queue
	if (kfifo_len(&dev->rd_queue) > dev->rd_queue_size / 2) {
		CONN_GADGET_DBG("rd_queue has much buffer already\n");
		goto done;
	}

req:
	//if there is a rx_idle, then usb_ep_queue
	CONN_GADGET_DBG("conn_gadget_request_ep_out\n");
	conn_gadget_request_ep_out(dev);

done:
	conn_gadget_unlock(&dev->read_excl);
	CONN_GADGET_DBG("conn_gadget_read returning %d\n", r);
	return r;
}

static ssize_t conn_gadget_write(struct file *fp, const char __user *buf,
				 size_t count, loff_t *pos)
{
	struct conn_gadget_dev *dev = fp->private_data;
	struct usb_request *req = 0;
	int r = count, xfer;
	int ret;

	if (!_conn_gadget_dev) {
		CONN_GADGET_ERR("_conn_gadget_dev is NULL\n");
		return -ENODEV;
	}

	CONN_GADGET_DBG("conn_gadget_write(%d)\n", count);

	if (conn_gadget_lock(&dev->write_excl)) {
		CONN_GADGET_ERR("conn_gadget_lock(write_excl) f\n");
		return -EBUSY;
	}


	while (count > 0) {
		CONN_GADGET_DBG("in the loop (user count %d)\n", (int)count);

		if (dev->error) {
			r = -EIO;
			CONN_GADGET_ERR("conn_gadget_write dev->error\n");
			break;
		}

		/* get an idle tx request to use */
		req = 0;
		ret = wait_event_interruptible(dev->write_wq,
			(req = conn_gadget_req_get(dev, &dev->tx_idle)) || dev->error);

		if (ret < 0) {
			r = ret;
			printk(KERN_ERR "%s: wait_event_interruptible(wrwq,reqget) failed %d\n", __func__, ret);
			break;
		}

		if (req != 0) {
			if (count > dev->transfer_size)
				xfer = dev->transfer_size;
			else
				xfer = count;

			if (copy_from_user(req->buf, buf, xfer)) {
				r = -EFAULT;
				printk(KERN_ERR "%s: copy_from_user failed\n", __func__);
				break;
			}

			req->length = xfer;

			ret = usb_ep_queue(dev->ep_in, req, GFP_ATOMIC);
			if (ret < 0) {
				dev->error = 1;
				pr_debug("%s: error %d\n", __func__, dev->error);
				r = -EIO;
				CONN_GADGET_ERR("xfer error %d\n", ret);
				break;
			}

			buf += xfer;
			count -= xfer;

			/* zero this so we don't try to free it on error exit */
			req = 0;
		}
	}

	if (req) {
		pr_debug("%s: req_put\n", __func__);
		conn_gadget_req_put(dev, &dev->tx_idle, req);
	}

	conn_gadget_unlock(&dev->write_excl);
	CONN_GADGET_DBG("conn_gadget_write returning %d\n", r);


	return r;
}

static int conn_gadget_open(struct inode *ip, struct file *fp)
{
	printk(KERN_INFO "conn_gadget_open\n");

	if (!_conn_gadget_dev) {
		CONN_GADGET_ERR("_conn_gadget_dev is NULL\n");
		return -ENODEV;
	}

	if (atomic_read(&_conn_gadget_dev->flush)) { //doing flush-ing
		CONN_GADGET_ERR("handle closing now. open again\n");
		return -EAGAIN;
	}

	if (conn_gadget_lock(&_conn_gadget_dev->open_excl)) {
		CONN_GADGET_ERR("conn_gadget_lock(open_excl) f\n");
		return -EBUSY;
	}

	fp->private_data = _conn_gadget_dev;

	/* clear the error latch */
	_conn_gadget_dev->error = 0;

	if (_conn_gadget_dev->online) {
		CONN_GADGET_ERR("_conn_gaddget_dev onlined\n");

		//if there is a rx_idle, then usb_ep_queue
		CONN_GADGET_DBG("conn_gadget_request_ep_out\n");
		conn_gadget_request_ep_out(_conn_gadget_dev);
	}


	//memorized status is not necessary before handle opened.
	_conn_gadget_dev->memorized = _conn_gadget_dev->online;

	CONN_GADGET_DBG("end\n");
	return 0;
}

static int conn_gadget_flush(struct file *fp, fl_owner_t id)
{
	struct conn_gadget_dev	*dev = _conn_gadget_dev;
	printk(KERN_INFO "conn_gadget_flush\n");

	if (!dev) {
		CONN_GADGET_ERR("_conn_gadget_dev is invalid\n");
		return -ENODEV;
	}

	atomic_set(&dev->flush, 1);

	CONN_GADGET_DBG("ioctl_wq wkup\n");
	wake_up(&dev->ioctl_wq);

	return 0;
}

static int conn_gadget_release(struct inode *ip, struct file *fp)
{
	struct usb_request *req, *n;
	unsigned long flags;

	printk(KERN_INFO "conn_gadget_release\n");

	spin_lock_irqsave(&_conn_gadget_dev->lock, flags);
	list_for_each_entry_safe(req, n, &_conn_gadget_dev->rx_busy, list) {
		spin_unlock_irqrestore(&_conn_gadget_dev->lock, flags);

		printk(KERN_INFO "list_for_each...\n");
		usb_ep_dequeue(_conn_gadget_dev->ep_out, req);

		spin_lock_irqsave(&_conn_gadget_dev->lock, flags);
	}
	spin_unlock_irqrestore(&_conn_gadget_dev->lock, flags);

	atomic_set(&_conn_gadget_dev->flush, 0);

	conn_gadget_unlock(&_conn_gadget_dev->open_excl);
	return 0;
}

static int conn_gadget_bind_status_copy_to_user(unsigned long value, int online)
{
	int err;
	unsigned long status = CONN_GADGET_IOCTL_BIND_STATUS_UNDEFINED;

	status = (online)?CONN_GADGET_IOCTL_BIND_STATUS_BIND : CONN_GADGET_IOCTL_BIND_STATUS_UNBIND;
	err = copy_to_user((void __user *)value, (const void *)&status, sizeof(status));
	if (err) {
		CONN_GADGET_ERR("copy_to_user f %d\n", err);
		err = -EFAULT;
	} else {
		CONN_GADGET_DBG("online value %d\n", online);
	}

	return err;
}

static long conn_gadget_ioctl(struct file *fp, unsigned int cmd,
		unsigned long value)
{
	struct conn_gadget_dev	*dev = NULL;
	int size;
	int flushed = 0; //is closing
	int err = 0;  //success
	const int IOCTL_ARRAY[CONN_GADGET_IOCTL_MAX_NR+1] = {
		CONN_GADGET_IOCTL_SUPPORT_LIST,
		CONN_GADGET_IOCTL_BIND_WAIT_NOTIFY,
		CONN_GADGET_IOCTL_BIND_GET_STATUS
		};

	if (_IOC_TYPE(cmd) != CONN_GADGET_IOCTL_MAGIC_SIG) {
		CONN_GADGET_ERR("cmd is not proper ioctl type %c\n",
				_IOC_TYPE(cmd));
		return -EINVAL;
	}

	if (_IOC_NR(cmd) >= CONN_GADGET_IOCTL_MAX_NR) {
		CONN_GADGET_ERR("cmd is not proper ioctl number %d\n",
				_IOC_NR(cmd));
		return -ENOTTY;
	}

	size = _IOC_SIZE(cmd);
	if (!size) {
		CONN_GADGET_ERR("cmd has no buffer\n");
		return -EINVAL;
	}

	if (!(_IOC_DIR(cmd) & _IOC_READ)) {
		CONN_GADGET_ERR("cmd has invalid direction\n");
		return -EINVAL;
	}

	if (!_conn_gadget_dev) {
		CONN_GADGET_ERR("_conn_gadget_dev is NULL\n");
		return -ENODEV;
	} else {
		dev = _conn_gadget_dev;
	}

	switch (cmd) {
	case CONN_GADGET_IOCTL_SUPPORT_LIST:
		err = copy_to_user((void __user *)value, (const void *)IOCTL_ARRAY, sizeof(IOCTL_ARRAY));
		if (err) {
			CONN_GADGET_ERR("SUPPORT_LIST copy_to_user f %d\n", err);
			err = -EFAULT;
		}
		break;
/** NOTE:
I think, memorized and online vairiable should be atomic variable. talk to choi */
	case CONN_GADGET_IOCTL_BIND_WAIT_NOTIFY:
		CONN_GADGET_DBG("in wait_event\n");
		wait_event_interruptible(dev->ioctl_wq,
				(dev->memorized != dev->online)
				|| (flushed = atomic_read(&dev->flush)));
		dev->memorized = dev->online;
		CONN_GADGET_DBG("out wait_event\n");

		if (flushed) {
			CONN_GADGET_ERR("close called\n");
			err = -EINTR;
			break;
		}

		err = conn_gadget_bind_status_copy_to_user(value, dev->online);
		if (err) {
			CONN_GADGET_ERR("WAIT_NOTIFY copy_to_user f %d\n", err);
		}
		break;

	case CONN_GADGET_IOCTL_BIND_GET_STATUS:
		err = conn_gadget_bind_status_copy_to_user(value, dev->online);
		if (err) {
			CONN_GADGET_ERR("GET_STATUS copy_to_user f %d\n", err);
		}
		break;
	}

	return err;
}

/* file operations for conn_gadget device /dev/android_ssusbcon */
static const struct file_operations conn_gadget_fops = {
	.owner = THIS_MODULE,
	.read = conn_gadget_read,
	.write = conn_gadget_write,
	.poll = conn_gadget_poll,
	.unlocked_ioctl = conn_gadget_ioctl,
	.open = conn_gadget_open,
	.release = conn_gadget_release,
	.flush = conn_gadget_flush,
};

static struct miscdevice conn_gadget_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = conn_gadget_shortname,
	.fops = &conn_gadget_fops,
};




static int
conn_gadget_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct conn_gadget_dev	*dev = func_to_conn_gadget(f);
	int			id;
	int			ret;

	dev->cdev = cdev;
	printk(KERN_ERR "conn_gadget_function_bind dev: %p\n", dev);

	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	conn_gadget_interface_desc.bInterfaceNumber = id;

	/* allocate endpoints */
	ret = conn_gadget_create_bulk_endpoints(dev, &conn_gadget_fullspeed_in_desc,
			&conn_gadget_fullspeed_out_desc);
	if (ret)
		return ret;

	/* support high speed hardware */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		conn_gadget_highspeed_in_desc.bEndpointAddress =
			conn_gadget_fullspeed_in_desc.bEndpointAddress;
		conn_gadget_highspeed_out_desc.bEndpointAddress =
			conn_gadget_fullspeed_out_desc.bEndpointAddress;
	}

	/* support super speed hardware */
	if (gadget_is_superspeed(c->cdev->gadget)) {
		conn_gadget_superspeed_in_desc.bEndpointAddress =
			conn_gadget_fullspeed_in_desc.bEndpointAddress;
		conn_gadget_superspeed_out_desc.bEndpointAddress =
			conn_gadget_fullspeed_out_desc.bEndpointAddress;
	}

	printk(KERN_ERR "%s speed %s: IN/%s, OUT/%s\n",
	gadget_is_superspeed(c->cdev->gadget) ? "super" :
			gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
			f->name, dev->ep_in->name, dev->ep_out->name);
	return 0;
}

static void
conn_gadget_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct conn_gadget_dev	*dev = func_to_conn_gadget(f);
	struct usb_request *req;

	printk(KERN_ERR "conn_gadget_function_unbind\n");

	dev->memorized = dev->online;
	dev->online = 0;
	dev->error = 1;
	pr_debug("%s: error %d\n", __func__, dev->error);

	CONN_GADGET_DBG("ioctl_wq wkup\n");
	wake_up(&dev->ioctl_wq);

	CONN_GADGET_DBG("rd_wq wkup\n");
	wake_up(&dev->read_wq);

	while ((req = conn_gadget_req_get(dev, &dev->rx_idle)))
		conn_gadget_request_free(req, dev->ep_out);

	while ((req = conn_gadget_req_get(dev, &dev->rx_busy)))
		conn_gadget_request_free(req, dev->ep_out);

	while ((req = conn_gadget_req_get(dev, &dev->tx_idle)))
		conn_gadget_request_free(req, dev->ep_in);
}

static int conn_gadget_function_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{
	struct conn_gadget_dev	*dev = func_to_conn_gadget(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	int ret;

	printk(KERN_ERR "%s: intf: %d alt: %d\n", __func__, intf, alt);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	ret = config_ep_by_speed(cdev->gadget, f, dev->ep_in);
	if (ret)
		return ret;
	ret = usb_ep_enable(dev->ep_in);
	if (ret)
		return ret;
#else
	ret = usb_ep_enable(dev->ep_in,
			ep_choose(cdev->gadget,
				&conn_gadget_highspeed_in_desc,
				&conn_gadget_fullspeed_in_desc));
	if (ret)
		return ret;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	ret = config_ep_by_speed(cdev->gadget, f, dev->ep_out);
	if (ret) {
		usb_ep_disable(dev->ep_in);
		return ret;
	}
	ret = usb_ep_enable(dev->ep_out);
	if (ret) {
		usb_ep_disable(dev->ep_in);
		return ret;
	}
#else
	ret = usb_ep_enable(dev->ep_out,
			ep_choose(cdev->gadget,
				&conn_gadget_highspeed_out_desc,
				&conn_gadget_fullspeed_out_desc));
	if (ret) {
		usb_ep_disable(dev->ep_in);
		return ret;
	}
#endif

	dev->memorized = dev->online;
	dev->online = 1;
	dev->error = 0;

	CONN_GADGET_ERR("kfifo_reset\n");
	kfifo_reset(&_conn_gadget_dev->rd_queue);

	//if there is a rx_idle, then usb_ep_queue
	CONN_GADGET_DBG("conn_gadget_request_ep_out\n");
	conn_gadget_request_ep_out(_conn_gadget_dev);

	CONN_GADGET_DBG("ioctl_wq wkup\n");
	wake_up(&dev->ioctl_wq);

	/* readers may be blocked waiting for us to go online */
	CONN_GADGET_DBG("rd_wq wkup\n");
	wake_up(&dev->read_wq);
	return 0;
}

static void conn_gadget_function_disable(struct usb_function *f)
{
	struct conn_gadget_dev	*dev = func_to_conn_gadget(f);
	struct usb_composite_dev	*cdev = dev->cdev;

	printk(KERN_ERR "conn_gadget_function_disable cdev %p\n", cdev);
	dev->memorized = dev->online;
	dev->online = 0;
	dev->error = 1;
	pr_debug("%s: error %d\n", __func__, dev->error);
	usb_ep_disable(dev->ep_in);
	usb_ep_disable(dev->ep_out);

	CONN_GADGET_DBG("ioctl_wq wkup\n");
	wake_up(&dev->ioctl_wq);

	/* readers may be blocked waiting for us to go online */
	CONN_GADGET_DBG("rd_wq wkup\n");
	wake_up(&dev->read_wq);

	pr_debug("%s disabled\n", dev->function.name);
}

#if 0
static int conn_gadget_bind_config(struct usb_configuration *c)
{
	struct conn_gadget_dev *dev = _conn_gadget_dev;

	printk(KERN_INFO "conn_gadget_bind_config\n");

	dev->cdev = c->cdev;
	dev->function.name = "conn_gadget";

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
	dev->function.fs_descriptors = fs_conn_gadget_descs;
#else
	dev->function.descriptors = fs_conn_gadget_descs;
#endif

	dev->function.hs_descriptors = hs_conn_gadget_descs;
	dev->function.ss_descriptors = ss_conn_gadget_descs;
	dev->function.bind = conn_gadget_function_bind;
	dev->function.unbind = conn_gadget_function_unbind;
	dev->function.set_alt = conn_gadget_function_set_alt;
	dev->function.disable = conn_gadget_function_disable;

	return usb_add_function(c, &dev->function);
}
#endif


static ssize_t conn_gadget_usb_buffer_size_show(struct device *dev,
		struct device_attribute *attr, char *buf) {
	if (!_conn_gadget_dev) {
		CONN_GADGET_ERR("_conn_gadget_dev is NULL\n");
	return -ENODEV;
    }

    return sprintf(buf, "%d\n", (_conn_gadget_dev->transfer_size / 1024));
}

static ssize_t conn_gadget_usb_buffer_size_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size) {
	void *rd_queue_buf = NULL;
	int value;
	int kfifo_size;

	if (!_conn_gadget_dev) {
		CONN_GADGET_ERR("_conn_gadget_dev is NULL\n");
		return size;
	}

	if (_conn_gadget_dev->online) {
		CONN_GADGET_ERR("_conn_gaddget_dev onlined\n");
		return size;
	}

	sscanf(buf, "%d", &value);
	kfifo_size = (value * 1024) * CONN_GADGET_DEFAULT_Q_SIZE;

	rd_queue_buf = vmalloc(kfifo_size);
	if (!rd_queue_buf) {
		CONN_GADGET_ERR("rd_queue_buf vmalloc f\n");
		return size;
	}

	if (_conn_gadget_dev->rd_queue_buf)
		vfree(_conn_gadget_dev->rd_queue_buf);

	_conn_gadget_dev->transfer_size 	= value * 1024;
	_conn_gadget_dev->rd_queue_size 	= kfifo_size;
	_conn_gadget_dev->rd_queue_buf 		= rd_queue_buf;

	kfifo_reset(&_conn_gadget_dev->rd_queue);
	kfifo_init(&_conn_gadget_dev->rd_queue,
			_conn_gadget_dev->rd_queue_buf,
			_conn_gadget_dev->rd_queue_size);

	/* T/O/D/O: renew allocate requests for our endpoints */
	/* No need to reallocate in this time,
	 * Because at alt_set time, requests are newly allocated. */

	return size;
}

static ssize_t conn_gadget_out_max_packet_size_show(
		struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (!_conn_gadget_dev || !_conn_gadget_dev->ep_out) {
		CONN_GADGET_ERR("_conn_gadget_dev is NULL\n");
		return -ENODEV;
	}

	return sprintf(buf, "%d\n", (_conn_gadget_dev->ep_out->maxpacket));
}

static ssize_t conn_gadget_out_max_packet_size_store(
		struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t size)
{
	CONN_GADGET_DBG("not supported\n");
	return size;
}

static ssize_t conn_gadget_in_max_packet_size_show(
		struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (!_conn_gadget_dev || !_conn_gadget_dev->ep_in) {
		CONN_GADGET_ERR("_conn_gadget_dev is NULL\n");
		return -ENODEV;
	}

	return sprintf(buf, "%d\n", (_conn_gadget_dev->ep_in->maxpacket));
}

static ssize_t conn_gadget_in_max_packet_size_store(
		struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t size)
{
	CONN_GADGET_DBG("not supported\n");
	return size;
}


static DEVICE_ATTR(usb_buffer_size, S_IRUGO | S_IWUSR,
	conn_gadget_usb_buffer_size_show,
	conn_gadget_usb_buffer_size_store);

static DEVICE_ATTR(out_max_packet_size, S_IRUGO | S_IWUSR,
	conn_gadget_out_max_packet_size_show,
	conn_gadget_out_max_packet_size_store);

static DEVICE_ATTR(in_max_packet_size, S_IRUGO | S_IWUSR,
	conn_gadget_in_max_packet_size_show,
	conn_gadget_in_max_packet_size_store);



static struct device_attribute *conn_gadget_function_attributes[] = {
	&dev_attr_usb_buffer_size,
	&dev_attr_out_max_packet_size,
	&dev_attr_in_max_packet_size,
	NULL
};

extern struct device *create_function_device(char *name);

static int conn_gadget_setup(struct conn_gadget_instance *fi_conn_gadget)
{
	struct conn_gadget_dev *dev;
	struct device *android_dev;
	struct device_attribute **attrs;
	struct device_attribute *attr;
	int ret;
	int err = 0;

	printk(KERN_INFO "conn_gadget_setup\n");

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		printk(KERN_ERR "alloc conn_gadget_dev F\n");
		return -ENOMEM;
	}

	spin_lock_init(&dev->lock);

	init_waitqueue_head(&dev->read_wq);
	init_waitqueue_head(&dev->write_wq);
	init_waitqueue_head(&dev->ioctl_wq);

	atomic_set(&dev->open_excl, 0);
	atomic_set(&dev->read_excl, 0);
	atomic_set(&dev->write_excl, 0);
	atomic_set(&dev->flush, 0);

	INIT_LIST_HEAD(&dev->tx_idle);
	INIT_LIST_HEAD(&dev->rx_idle);
	INIT_LIST_HEAD(&dev->rx_busy);

	dev->transfer_size 	= CONN_GADGET_DEFAULT_TRANSFER_SIZE;
	dev->rd_queue_size 	= dev->transfer_size * CONN_GADGET_DEFAULT_Q_SIZE;
	dev->rd_queue_buf 	= vmalloc(dev->rd_queue_size);
	if (!dev->rd_queue_buf) {
		printk(KERN_ERR "%s: error rd_queue vmalloc\n", __func__);
		ret = -ENOMEM;
		goto err_;
	}
	kfifo_init(&dev->rd_queue, dev->rd_queue_buf, dev->rd_queue_size);

	_conn_gadget_dev = dev;

	ret = misc_register(&conn_gadget_device);
	if (ret) {
		printk(KERN_ERR "%s: misc_register f %d\n", __func__, ret);
		goto err_;
	}

	android_dev = create_function_device("f_conn_gadget");
	if (IS_ERR(android_dev))
		return PTR_ERR(android_dev);

	attrs = conn_gadget_function_attributes;

	if (attrs) {
		while ((attr = *attrs++) && !err)
			err = device_create_file(android_dev, attr);
		if (err) {
			device_destroy(android_dev->class, android_dev->devt);
			goto err_;
		}
	}

	return 0;
err_:

    if (dev->rd_queue_buf)
	vfree(dev->rd_queue_buf);

	_conn_gadget_dev = NULL;
	kfree(dev);
	CONN_GADGET_ERR("conn_gadget gadget driver failed to initialize\n");
	return ret;
}

static void conn_gadget_cleanup(void)
{
	printk(KERN_INFO "conn_gadget_cleanup\n");

	if (!_conn_gadget_dev) {
		CONN_GADGET_ERR("_conn_gadget_dev is not allocated\n");
		return ;
	}

	misc_deregister(&conn_gadget_device);

    if (_conn_gadget_dev->rd_queue_buf)
	vfree(_conn_gadget_dev->rd_queue_buf);

	kfree(_conn_gadget_dev);
	_conn_gadget_dev = NULL;
}

static int conn_gadget_setup_configfs(struct conn_gadget_instance *fi_conn_gadget)
{
	return conn_gadget_setup(fi_conn_gadget);
}

static struct conn_gadget_instance *to_conn_gadget_instance(struct config_item *item)
{
	return container_of(to_config_group(item), struct conn_gadget_instance,
		func_inst.group);
}

static void conn_gadget_attr_release(struct config_item *item)
{
	struct conn_gadget_instance *fi_conn_gadget = to_conn_gadget_instance(item);
	usb_put_function_instance(&fi_conn_gadget->func_inst);
}


static struct configfs_item_operations conn_gadget_item_ops = {
	.release        = conn_gadget_attr_release,
};

static struct config_item_type conn_gadget_func_type = {
	.ct_item_ops    = &conn_gadget_item_ops,
	.ct_owner       = THIS_MODULE,
};

static struct conn_gadget_instance *to_fi_conn_gadget(struct usb_function_instance *fi)
{
	return container_of(fi, struct conn_gadget_instance, func_inst);
}

static int conn_gadget_set_inst_name(struct usb_function_instance *fi, const char *name)
{
	struct conn_gadget_instance *fi_conn_gadget;
	char *ptr;
	int name_len;

	name_len = strlen(name) + 1;
	if (name_len > MAX_INST_NAME_LEN)
		return -ENAMETOOLONG;

	ptr = kstrndup(name, name_len, GFP_KERNEL);
	if (!ptr)
		return -ENOMEM;

	fi_conn_gadget = to_fi_conn_gadget(fi);
	fi_conn_gadget->name = ptr;

	return 0;
}

static void conn_gadget_free_inst(struct usb_function_instance *fi)
{
	struct conn_gadget_instance *fi_conn_gadget;

	fi_conn_gadget = to_fi_conn_gadget(fi);
	kfree(fi_conn_gadget->name);
	conn_gadget_cleanup();
	kfree(fi_conn_gadget);
}

struct usb_function_instance *alloc_inst_conn_gadget(void)
{
	struct conn_gadget_instance *fi_conn;
	int err;

	fi_conn = kzalloc(sizeof(*fi_conn), GFP_KERNEL);

	if (!fi_conn)
		return ERR_PTR(-ENOMEM);

	fi_conn->func_inst.set_inst_name = conn_gadget_set_inst_name;
	fi_conn->func_inst.free_func_inst = conn_gadget_free_inst;

	err = conn_gadget_setup_configfs(fi_conn);

	if (err) {
		kfree(fi_conn);
		pr_err("Error setting conn gadget\n");
		return ERR_PTR(err);
	}

	config_group_init_type_name(&fi_conn->func_inst.group,
					"", &conn_gadget_func_type);

	return  &fi_conn->func_inst;

}
EXPORT_SYMBOL_GPL(alloc_inst_conn_gadget);

static struct usb_function_instance *conn_gadget_alloc_inst(void)
{
		return alloc_inst_conn_gadget();
}

static void conn_gadget_free(struct usb_function *f)
{
	/*NO-OP: no function specific resource allocation in conn_gadget_alloc*/
}

static struct usb_function *conn_gadget_alloc(struct usb_function_instance *fi)
{
	struct conn_gadget_dev *dev = _conn_gadget_dev;

	dev->function.name = "conn_gadget";
	dev->function.fs_descriptors = fs_conn_gadget_descs;
	dev->function.hs_descriptors = hs_conn_gadget_descs;
	dev->function.ss_descriptors = ss_conn_gadget_descs;
	dev->function.bind = conn_gadget_function_bind;
	dev->function.unbind = conn_gadget_function_unbind;
	dev->function.set_alt = conn_gadget_function_set_alt;
	dev->function.disable = conn_gadget_function_disable;
	dev->function.free_func = conn_gadget_free;

	return &dev->function;
}

DECLARE_USB_FUNCTION_INIT(conn_gadget, conn_gadget_alloc_inst, conn_gadget_alloc);

/*
static int conn_gadget_function_init(struct android_usb_function *f, struct usb_composite_dev *cdev)
{
	printk(KERN_ERR "%s(#) call conn_gadget_setup\n", __func__);
	return conn_gadget_setup();
}

static void conn_gadget_function_cleanup(struct android_usb_function *f)
{
	printk(KERN_ERR "%s(#) call conn_gadget_cleanup\n", __func__);
	conn_gadget_cleanup();
}

static int conn_gadget_function_bind_config(struct android_usb_function *f, struct usb_configuration *c)
{
	printk(KERN_ERR "%s(#) call conn_gadget_bind_config\n", __func__);
	return conn_gadget_bind_config(c);
}
*/



