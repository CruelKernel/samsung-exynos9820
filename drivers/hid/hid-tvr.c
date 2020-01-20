/*
 * TVR driver for Linux. Based on hidraw
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/hid.h>
#include <linux/mutex.h>
#include <linux/sched/signal.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/usb.h>

#include <linux/hidraw.h>
#include "hid-ids.h"

#define TVR_PROTOCOL_SENSOR		0
#define TVR_PROTOCOL_CONTROL	5

/* number of reports to buffer */
#define TVR_HIDRAW_BUFFER_SIZE 64
#define TVR_HIDRAW_MAX_DEVICES 64

#define TVR_CONNECTED		(1)
#define TVR_DISCONNECTED	(0)
static int isTvrConnected	= TVR_DISCONNECTED;

#define SUPPORT_TVR_NONE				(0x0000)
#define SUPPORT_TVR_KEY_DEVICE			(0x0001)
#define SUPPORT_TVR_KEY_INJECTION		(0x0002)
#define SUPPORT_TVR_GEARVR_DEVICE		(0x0010)
#define SUPPORT_TVR_GEARVR_DATA_RELAY	(0x0020)
#define SUPPORT_TVR_GEARVR_DATA_TVR		(0x0100)
static int tvr_data_on 		= (SUPPORT_TVR_KEY_DEVICE |		\
								SUPPORT_TVR_KEY_INJECTION |	\
                                SUPPORT_TVR_GEARVR_DEVICE |	\
								SUPPORT_TVR_GEARVR_DATA_RELAY);

#define TVR_KEY_REPORTID	0x3d

#define TVR_KEY_VOLUMEUP	0xe9
#define TVR_KEY_VOLUMEDOWN	0xea
#define TVR_KEY_MEDIA		0xeb
#define TVR_KEY_MAX			3

#define TVR_KEY_PRESSED		0x1
#define TVR_KEY_RELEASED	0x0

static int tvr_keys[TVR_KEY_MAX][3] = { 
	{KEY_VOLUMEUP, TVR_KEY_RELEASED, TVR_KEY_VOLUMEUP}, 
	{KEY_VOLUMEDOWN, TVR_KEY_RELEASED, TVR_KEY_VOLUMEDOWN}, 
	{KEY_MEDIA, TVR_KEY_RELEASED, TVR_KEY_MEDIA}
};

static struct workqueue_struct *tvr_wq;
static void tvr_input_work(struct work_struct *work);
static DECLARE_DELAYED_WORK(tvr_work, tvr_input_work);

static struct class *tvr_class;
static struct hidraw *tvr_hidraw_table[TVR_HIDRAW_MAX_DEVICES];
static DEFINE_MUTEX(minors_lock);
static DEFINE_SPINLOCK(list_lock);

static int tvr_major;
static struct cdev tvr_cdev;
static struct kobject *virtual_dir = NULL;
extern struct kobject *virtual_device_parent(struct device *dev);

struct input_dev *tvr_input_dev = NULL;
static int tvr_report_event(struct hid_device *hid, u8 *data, int len);
static int tvr_connect(struct hid_device *hid);
static void tvr_disconnect(struct hid_device *hid);

struct hidraw *tvrraw = NULL;

#ifdef CONFIG_HID_OVR
extern int ovr_connect(struct hid_device *hid, int mode);
extern void ovr_disconnect(struct hid_device *hid);
extern int ovr_raw_event(struct hid_device *hdev, struct hid_report *report, u8 *data, int size);
#endif

static ssize_t tvr_hidraw_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos)
{
	struct hidraw_list *list = file->private_data;
	int ret = 0, len;
	DECLARE_WAITQUEUE(wait, current);

	mutex_lock(&list->read_mutex);

	while (ret == 0) {
		if (list->head == list->tail) {
			add_wait_queue(&list->hidraw->wait, &wait);
			set_current_state(TASK_INTERRUPTIBLE);

			while (list->head == list->tail) {
				if (signal_pending(current)) {
					ret = -ERESTARTSYS;
					break;
				}
				if (!list->hidraw->exist) {
					ret = -EIO;
					break;
				}
				if (file->f_flags & O_NONBLOCK) {
					ret = -EAGAIN;
					break;
				}

				/* allow O_NONBLOCK to work well from other threads */
				mutex_unlock(&list->read_mutex);
				schedule();
				mutex_lock(&list->read_mutex);
				set_current_state(TASK_INTERRUPTIBLE);
			}

			set_current_state(TASK_RUNNING);
			remove_wait_queue(&list->hidraw->wait, &wait);
		}

		if (ret)
			goto out;

		len = list->buffer[list->tail].len > count ?
			count : list->buffer[list->tail].len;

		if (list->buffer[list->tail].value) {
			if (copy_to_user(buffer, list->buffer[list->tail].value, len)) {
				ret = -EFAULT;
				goto out;
			}
			ret = len;
		}

		kfree(list->buffer[list->tail].value);
		list->buffer[list->tail].value = NULL;
		list->tail = (list->tail + 1) & (TVR_HIDRAW_BUFFER_SIZE - 1);
	}
out:
	mutex_unlock(&list->read_mutex);

	return ret;
}

/* The first byte is expected to be a report number.
 * This function is to be called with the minors_lock mutex held */
static ssize_t tvr_hidraw_send_report(struct file *file, const char __user *buffer, size_t count, unsigned char report_type)
{
	unsigned int minor = iminor(file_inode(file));
	struct hid_device *dev;
	__u8 *buf;
	int ret = 0;

	if (!tvr_hidraw_table[minor] || !tvr_hidraw_table[minor]->exist) {
		ret = -ENODEV;
		goto out;
	}

	dev = tvr_hidraw_table[minor]->hid;
	if (!dev) {
		ret = -ENODEV;
		goto out;
	}

	if (count > HID_MAX_BUFFER_SIZE) {
		hid_warn(dev, "TVR: pid %d passed too large report\n",
			 task_pid_nr(current));
		ret = -EINVAL;
		goto out;
	}

	if (count < 2) {
		hid_warn(dev, "TVR: pid %d passed too short report\n",
			 task_pid_nr(current));
		ret = -EINVAL;
		goto out;
	}

	buf = kmalloc(count * sizeof(__u8), GFP_KERNEL);
	if (!buf) {
		ret = -ENOMEM;
		goto out;
	}

	if (copy_from_user(buf, buffer, count)) {
		ret = -EFAULT;
		goto out_free;
	}

	if ((report_type == HID_OUTPUT_REPORT) &&
	    !(dev->quirks & HID_QUIRK_NO_OUTPUT_REPORTS_ON_INTR_EP)) {
		ret = hid_hw_output_report(dev, buf, count);
		/*
		 * compatibility with old implementation of USB-HID and I2C-HID:
		 * if the device does not support receiving output reports,
		 * on an interrupt endpoint, fallback to SET_REPORT HID command.
		 */
		if (ret != -ENOSYS)
			goto out_free;
	}

	ret = hid_hw_raw_request(dev, buf[0], buf, count, report_type,
				HID_REQ_SET_REPORT);

out_free:
	kfree(buf);
out:
	return ret;
}

/* the first byte is expected to be a report number */
static ssize_t tvr_hidraw_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	ssize_t ret;
	mutex_lock(&minors_lock);
	ret = tvr_hidraw_send_report(file, buffer, count, HID_OUTPUT_REPORT);
	mutex_unlock(&minors_lock);	
	return ret;
}

/* This function performs a Get_Report transfer over the control endpoint
 * per section 7.2.1 of the HID specification, version 1.1.  The first byte
 * of buffer is the report number to request, or 0x0 if the defice does not
 * use numbered reports. The report_type parameter can be HID_FEATURE_REPORT
 * or HID_INPUT_REPORT.  This function is to be called with the minors_lock
 *  mutex held. */
static ssize_t tvr_hidraw_get_report(struct file *file, char __user *buffer, size_t count, unsigned char report_type)
{
	unsigned int minor = iminor(file_inode(file));
	struct hid_device *dev;
	__u8 *buf;
	int ret = 0, len;
	unsigned char report_number;

	dev = tvr_hidraw_table[minor]->hid;
	if (!dev) {
		ret = -ENODEV;
		goto out;		
	}

	if (!dev->ll_driver->raw_request) {
		ret = -ENODEV;
		goto out;
	}

	if (count > HID_MAX_BUFFER_SIZE) {
		printk(KERN_WARNING "TVR: hidraw: pid %d passed too large report\n",
				task_pid_nr(current));
		ret = -EINVAL;
		goto out;
	}

	if (count < 2) {
		printk(KERN_WARNING "TVR: hidraw: pid %d passed too short report\n",
				task_pid_nr(current));
		ret = -EINVAL;
		goto out;
	}

	buf = kmalloc(count * sizeof(__u8), GFP_KERNEL);
	if (!buf) {
		ret = -ENOMEM;
		goto out;
	}

	/*
	 * Read the first byte from the user. This is the report number,
	 * which is passed to tvr_hid_hw_raw_request().
	 */
	if (copy_from_user(&report_number, buffer, 1)) {
		ret = -EFAULT;
		goto out_free;
	}

	ret = hid_hw_raw_request(dev, report_number, buf, count, report_type,
				 HID_REQ_GET_REPORT);

	if (ret < 0)
		goto out_free;

	len = (ret < count) ? ret : count;

	if (copy_to_user(buffer, buf, len)) {
		ret = -EFAULT;
		goto out_free;
	}

	ret = len;

out_free:
	kfree(buf);
out:
	return ret;
}

static unsigned int tvr_hidraw_poll(struct file *file, poll_table *wait)
{
	struct hidraw_list *list = file->private_data;

	poll_wait(file, &list->hidraw->wait, wait);
	if (list->head != list->tail)
		return POLLIN | POLLRDNORM;
	if (!list->hidraw->exist)
		return POLLERR | POLLHUP;
	return 0;
}

static int tvr_hidraw_open(struct inode *inode, struct file *file)
{
	unsigned int minor = iminor(inode);
	struct hidraw *dev;
	struct hidraw_list *list;
	int err = 0;

	if (!(list = kzalloc(sizeof(struct hidraw_list), GFP_KERNEL))) {
		err = -ENOMEM;
		goto out;
	}

	mutex_lock(&minors_lock);
	if (!tvr_hidraw_table[minor]) {
		err = -ENODEV;
		goto out_unlock;
	}

	printk("TVR: open %d (%d:%s) >>>\n", minor, current->pid, current->comm);

	list->hidraw = tvr_hidraw_table[minor];
	mutex_init(&list->read_mutex);

	spin_lock_irq(&list_lock);
	list_add_tail(&list->node, &tvr_hidraw_table[minor]->list);
	spin_unlock_irq(&list_lock);

	file->private_data = list;

	dev = tvr_hidraw_table[minor];
	dev->open++;

	printk("TVR: open(%d) err %d <<<\n", dev->open, err);

out_unlock:
	mutex_unlock(&minors_lock);
out:
	if (err < 0)
		kfree(list);
	return err;
}

static int tvr_hidraw_fasync(int fd, struct file *file, int on)
{
	struct hidraw_list *list = file->private_data;

	return fasync_helper(fd, file, on, &list->fasync);
}

static int tvr_hidraw_release(struct inode * inode, struct file * file)
{
	unsigned int minor = iminor(inode);
	struct hidraw *dev;
	struct hidraw_list *list = file->private_data;
	int ret;
	int i;
	unsigned long flags;

	mutex_lock(&minors_lock);
	if (!tvr_hidraw_table[minor]) {
		ret = -ENODEV;
		goto unlock;
	}

	printk("TVR: release %d (%d:%s) >>>\n", minor, current->pid, current->comm);

	spin_lock_irqsave(&list_lock, flags);
	list_del(&list->node);
	spin_unlock_irqrestore(&list_lock, flags);

	dev = tvr_hidraw_table[minor];
	--dev->open;

	if (!dev->open) {
		if (!list->hidraw->exist) {
			printk("TVR: freed tvr_hidraw_table %d\n", minor);
			kfree(list->hidraw);
			tvr_hidraw_table[minor] = NULL;
		}
	}

	for (i = 0; i < TVR_HIDRAW_BUFFER_SIZE; ++i)
		kfree(list->buffer[i].value);
	kfree(list);
	ret = 0;

	printk("TVR: release <<<\n");

unlock:
	mutex_unlock(&minors_lock);

	return ret;
}

static int tvr_report_event(struct hid_device *hid, u8 *data, int len)
{
	struct hidraw *dev = hid->hidtvr;
	struct hidraw_list *list;
	int ret = 0;
	unsigned long flags;
	
	if (!dev)
		return -EPERM;
	
	spin_lock_irqsave(&list_lock, flags);
	list_for_each_entry(list, &dev->list, node) {
		int new_head = (list->head + 1) & (TVR_HIDRAW_BUFFER_SIZE - 1);

		if (new_head == list->tail)
			continue;

		if (!(list->buffer[list->head].value = kmemdup(data, len, GFP_ATOMIC))) {
			ret = -ENOMEM;
			spin_unlock_irqrestore(&list_lock, flags);
			break;
		}

		list->buffer[list->head].len = len;
		list->head = new_head;
		kill_fasync(&list->fasync, SIGIO, POLL_IN);
	}
	spin_unlock_irqrestore(&list_lock, flags);

	wake_up_interruptible(&dev->wait);

	return ret;
}

static int tvr_connect(struct hid_device *hid)
{
	int minor, result;
	struct hidraw *dev;

	/* we accept any HID device, no matter the applications */
	dev = kzalloc(sizeof(struct hidraw), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	result = -EINVAL;

	mutex_lock(&minors_lock);

	for (minor = 0; minor < TVR_HIDRAW_MAX_DEVICES; minor++)
	{
		if (tvr_hidraw_table[minor]) {
			printk("TVR: old tvr_hidraw_table %d\n", minor);
			continue;
		}

		tvr_hidraw_table[minor] = dev;
		result = 0;
		break;
	}

	printk("TVR: connect %d %d (%d:%s) >>>\n", minor, result, current->pid, current->comm);

	if (result) {
		mutex_unlock(&minors_lock);
		kfree(dev);
		goto out;
	}

	dev->dev = device_create(tvr_class, &hid->dev, MKDEV(tvr_major, minor),
					 NULL, "%s%d", "tvr", minor);

	if (IS_ERR(dev->dev)) {
		tvr_hidraw_table[minor] = NULL;
		mutex_unlock(&minors_lock);
		result = PTR_ERR(dev->dev);
		kfree(dev);
		goto out;
	}

	printk("TVR: connect <<<\n");

	init_waitqueue_head(&dev->wait);
	INIT_LIST_HEAD(&dev->list);

	dev->hid = hid;
	dev->minor = minor;

	dev->exist = 1;
	hid->hidtvr = dev;
	tvrraw = dev;

	mutex_unlock(&minors_lock);
out:
	return result;
}

static void tvr_disconnect(struct hid_device *hid)
{
	struct hidraw *hidraw = hid->hidtvr;

	mutex_lock(&minors_lock);

	printk("TVR: disconnect %d %d (%d:%s) >>>\n", hidraw->minor, hidraw->open, current->pid, current->comm);

	hidraw->exist = 0;

	device_destroy(tvr_class, MKDEV(tvr_major, hidraw->minor));

	if (hidraw->open) {
		wake_up_interruptible(&hidraw->wait);
	} else {
		printk("TVR: freed tvr_hidraw_table %d\n", hidraw->minor);
		tvr_hidraw_table[hidraw->minor] = NULL;
		kfree(hidraw);
	}

	printk("TVR: disconnect <<<\n");

	mutex_unlock(&minors_lock);
}

static long tvr_hidraw_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct inode *inode = file->f_path.dentry->d_inode;
	unsigned int minor = iminor(inode);
	long ret = 0;
	struct hidraw *dev;
	void __user *user_arg = (void __user*) arg;

	mutex_lock(&minors_lock);
	dev = tvr_hidraw_table[minor];
	if (!dev || (dev && !dev->exist)) {
		ret = -ENODEV;
		goto out;
	}

	switch (cmd) {
		case HIDIOCGRDESCSIZE:
			if (dev->hid) {
				if (put_user(dev->hid->rsize, (int __user *)arg))
					ret = -EFAULT;
			} else {
				ret = -EFAULT;
			}
			break;

		case HIDIOCGRDESC:
			{
				if (dev->hid) {
					__u32 len;

					if (get_user(len, (int __user *)arg))
						ret = -EFAULT;
					else if (len > HID_MAX_DESCRIPTOR_SIZE - 1)
						ret = -EINVAL;
					else if (copy_to_user(user_arg + offsetof(
						struct hidraw_report_descriptor,
						value[0]),
						dev->hid->rdesc,
						min(dev->hid->rsize, len)))
						ret = -EFAULT;
				} else {
					ret = -EFAULT;
				}
				break;
			}
		case HIDIOCGRAWINFO:
			{
				struct hidraw_devinfo dinfo;

				dinfo.bustype = dev->hid->bus;
				dinfo.vendor = dev->hid->vendor;
				dinfo.product = dev->hid->product;

				if (copy_to_user(user_arg, &dinfo, sizeof(dinfo)))
					ret = -EFAULT;
				break;
			}
		default:
			{
				struct hid_device *hid = dev->hid;

				if (_IOC_TYPE(cmd) != 'H') {
					ret = -EINVAL;
					break;
				}

				if (_IOC_NR(cmd) == _IOC_NR(HIDIOCSFEATURE(0))) {
					int len = _IOC_SIZE(cmd);

					ret = tvr_hidraw_send_report(file, user_arg, len, HID_FEATURE_REPORT);

					break;
				}
				if (_IOC_NR(cmd) == _IOC_NR(HIDIOCGFEATURE(0))) {
					int len = _IOC_SIZE(cmd);

					ret = tvr_hidraw_get_report(file, user_arg, len, HID_FEATURE_REPORT);

					break;
				}

				/* Begin Read-only ioctls. */
				if (_IOC_DIR(cmd) != _IOC_READ) {
					ret = -EINVAL;
					break;
				}

				if (_IOC_NR(cmd) == _IOC_NR(HIDIOCGRAWNAME(0))) {

					int len = strlen(hid->name) + 1;
					if (len > _IOC_SIZE(cmd))
						len = _IOC_SIZE(cmd);
					ret = copy_to_user(user_arg, hid->name, len) ?
						-EFAULT : len;

					break;
				}

				if (_IOC_NR(cmd) == _IOC_NR(HIDIOCGRAWPHYS(0))) {

					int len = strlen(hid->phys) + 1;
					if (len > _IOC_SIZE(cmd))
						len = _IOC_SIZE(cmd);
					ret = copy_to_user(user_arg, hid->phys, len) ?
						-EFAULT : len;
					break;
				}
			}

		ret = -ENOTTY;
	}
out:
	mutex_unlock(&minors_lock);
	return ret;
}

static int gearvr_raw_event(struct hid_device *hdev, struct hid_report *report, u8 *data, int size) {
	if (tvr_data_on&(SUPPORT_TVR_GEARVR_DEVICE|SUPPORT_TVR_GEARVR_DATA_RELAY)) {
#ifdef CONFIG_HID_OVR
		return ovr_raw_event(hdev, report, data, size);
#endif		
	}
	return 0;
}
static int gearvr_connect(struct hid_device *hid, int mode) {
	if (tvr_data_on&(SUPPORT_TVR_GEARVR_DEVICE)) {
#ifdef CONFIG_HID_OVR
		return ovr_connect(hid, mode);
#endif
	}
	return 0;
}

static void gearvr_disconnect(struct hid_device *hid) {
	if (tvr_data_on&(SUPPORT_TVR_GEARVR_DEVICE)) {
#ifdef CONFIG_HID_OVR
		ovr_disconnect(hid);
#endif
	}
}

static void tvr_input_work(struct work_struct *work)
{
	if (tvr_input_dev){
		input_sync(tvr_input_dev);
	}
}

static void check_input_event(u8 *data, int size)
{
	int i;

	if (tvr_data_on&(SUPPORT_TVR_KEY_DEVICE|SUPPORT_TVR_KEY_INJECTION)) {
		if (tvr_input_dev && size >= 3 && data[0] == TVR_KEY_REPORTID) {
			for (i=0; i<TVR_KEY_MAX; i++) {
				if (tvr_keys[i][2] == data[2]) {
					tvr_keys[i][1] = data[1];
					printk("TVR: keyevent %d(%s)", tvr_keys[i][0], tvr_keys[i][1]?"pressed":"released");
					input_report_key(tvr_input_dev, tvr_keys[i][0], tvr_keys[i][1]);
					if (tvr_wq) {
						queue_delayed_work(tvr_wq, &tvr_work, 0);
					}
					break;
				}
			}
		}
	}
}

static int tvr_init_keys(struct hid_device *hdev)
{
	int error, i;

	if (tvr_data_on&(SUPPORT_TVR_KEY_DEVICE)) {
		tvr_input_dev = input_allocate_device();
		if (tvr_input_dev == NULL) {
			printk("TVR: failed to allocate input device\n");
			return -ENOMEM;
		}

		tvr_input_dev->name = "TVR input device";
		tvr_input_dev->evbit[0] = BIT(EV_KEY);

		for (i=0; i<TVR_KEY_MAX; i++) {
			input_set_capability(tvr_input_dev, EV_KEY, tvr_keys[i][0]);
			tvr_keys[i][1] = TVR_KEY_RELEASED;
		}

		error = input_register_device(tvr_input_dev);
		if (error) {
			printk("TVR: error registering the input device\n");
			input_free_device(tvr_input_dev);
			return error;
		}

		tvr_wq = create_workqueue("tvr_work");
	}

	return 0;
}

static void tvr_exit_keys(void)
{
	int i;

	if (tvr_input_dev) {

		if (tvr_wq) {
			flush_workqueue(tvr_wq);
			destroy_workqueue(tvr_wq);
			tvr_wq = NULL;
		}

		for (i=0; i<TVR_KEY_MAX; i++) {
			if (tvr_keys[i][1] != TVR_KEY_RELEASED) {
				input_report_key(tvr_input_dev, tvr_keys[i][0], TVR_KEY_RELEASED);
				input_sync(tvr_input_dev);
				tvr_keys[i][1] = TVR_KEY_RELEASED;
			}
		}

		input_unregister_device(tvr_input_dev);
		tvr_input_dev = NULL;
	}
}

static const struct file_operations tvr_ops = {
	.owner = THIS_MODULE,
	.read = tvr_hidraw_read,
	.write = tvr_hidraw_write,
	.poll = tvr_hidraw_poll,
	.open = tvr_hidraw_open,
	.release = tvr_hidraw_release,
	.unlocked_ioctl = tvr_hidraw_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = tvr_hidraw_ioctl,
#endif
	.fasync = tvr_hidraw_fasync,
	.llseek = noop_llseek,
};

static int tvr_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	int retval;
	u8 ifproto;

	struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
	ifproto = intf->cur_altsetting->desc.bInterfaceProtocol;
	printk("TVR: probe ifproto %d\n", ifproto);
	if (ifproto == TVR_PROTOCOL_SENSOR) {

		retval = tvr_connect(hdev);
		if (retval) {
			hid_err(hdev, "TVR: Couldn't connect\n");
			goto exit;
		}

		retval = gearvr_connect(hdev, 1);
		if (retval) {
			hid_err(hdev, "TVR: Couldn't connect Gearvr\n");
			tvr_disconnect(hdev);
			goto exit;
		}

		retval = tvr_init_keys(hdev);
		if (retval) {
			hid_err(hdev, "TVR: Couldn't register keys\n");
			gearvr_disconnect(hdev);
			tvr_disconnect(hdev);
			goto exit;
		}

		isTvrConnected = TVR_CONNECTED;
	} else if (ifproto == TVR_PROTOCOL_CONTROL) {
		hdev->hidtvr = tvrraw;
	}

	retval = hid_parse(hdev);
	if (retval) {
		hid_err(hdev, "TVR: parse failed\n");
		goto exit;
	}

	retval = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (retval) {
		hid_err(hdev, "TVR: hw start failed\n");
		goto exit;
	}

	retval = hid_hw_power(hdev, PM_HINT_FULLON);
	if (retval < 0) {
		hid_err(hdev, "TVR: Couldn't feed power\n");
		hid_hw_stop(hdev);
		goto exit;
	}

	retval = hid_hw_open(hdev);
	if (retval < 0) {
		hid_err(hdev, "TVR: Couldn't open hid\n");
		hid_hw_power(hdev, PM_HINT_NORMAL);
		hid_hw_stop(hdev);
		goto exit;
	}

	return 0;

exit:
    if (ifproto == TVR_PROTOCOL_SENSOR) {
		isTvrConnected = TVR_DISCONNECTED;
        tvr_exit_keys();
        gearvr_disconnect(hdev);
        tvr_disconnect(hdev);
    }

	return retval;
}

static void tvr_remove(struct hid_device *hdev)
{
    u8 ifproto;
	struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
	ifproto = intf->cur_altsetting->desc.bInterfaceProtocol;
	printk("TVR: remove %d\n", ifproto);	
	if (ifproto == TVR_PROTOCOL_SENSOR) {
		isTvrConnected = TVR_DISCONNECTED;
		tvr_exit_keys();
		gearvr_disconnect(hdev);
		tvr_disconnect(hdev);
	}

	hid_hw_close(hdev);
	hid_hw_power(hdev, PM_HINT_NORMAL);
	hid_hw_stop(hdev);
}

static int tvr_raw_event(struct hid_device *hdev, struct hid_report *report, u8 *data, int size)
{
	u8 ifproto = 0;
	struct usb_interface *intf = to_usb_interface(hdev->dev.parent);

	if (size <= 0) {
		return 0;
	}

	ifproto = intf->cur_altsetting->desc.bInterfaceProtocol;
	if (ifproto == TVR_PROTOCOL_SENSOR) {
		gearvr_raw_event(hdev, report, data, size);
		if (tvr_data_on&(SUPPORT_TVR_GEARVR_DATA_TVR)) {
			tvr_report_event(hdev, data, size);
		}
	} else if (ifproto == TVR_PROTOCOL_CONTROL) {
		check_input_event(data, size);
		tvr_report_event(hdev, data, size);
	}

	return 0;
}

static ssize_t data_on_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", tvr_data_on);
}

static ssize_t data_on_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;

	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (isTvrConnected) {
		if (val&(SUPPORT_TVR_KEY_DEVICE) && !(tvr_data_on&(SUPPORT_TVR_KEY_DEVICE))) {
			printk("TVR: error - can't enable key device while TVR is running\n");
			return -EINVAL;
		}
		if (val&(SUPPORT_TVR_GEARVR_DEVICE) && !(tvr_data_on&(SUPPORT_TVR_GEARVR_DEVICE))) {
			printk("TVR: error - can't enable GearVR device while TVR is running\n");
			return -EINVAL;
		}
	}

	tvr_data_on = val;

	return count;
}

static DEVICE_ATTR(data_on, 0664, data_on_show, data_on_store);
static struct device_attribute *tvr_attrs[] = {
	&dev_attr_data_on,
	NULL,
};

static const struct hid_device_id tvr_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_SAMSUNG_ELECTRONICS, USB_DEVICE_ID_SAMSUNG_TVR_1) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_SAMSUNG_ELECTRONICS, USB_DEVICE_ID_SAMSUNG_TVR_2) },	
	{ }
};

MODULE_DEVICE_TABLE(hid, tvr_devices);

static struct hid_driver tvr_driver = {
		.name = "tvr",
		.id_table = tvr_devices,
		.probe = tvr_probe,
		.remove = tvr_remove,
		.raw_event = tvr_raw_event
};

static int __init tvr_init(void)
{
	int retval = 0;
	dev_t dev_id;

	tvr_class = class_create(THIS_MODULE, "tvr");
	if (IS_ERR(tvr_class)) {
		return PTR_ERR(tvr_class);
	}

	retval = hid_register_driver(&tvr_driver);
	if (retval < 0) {
		pr_warn("TVR: Can't register drive.\n");
		goto out_class;
	}

	retval = alloc_chrdev_region(&dev_id, 0,
			TVR_HIDRAW_MAX_DEVICES, "tvr");
	if (retval < 0) {
		pr_warn("TVR: Can't allocate chrdev region.\n");
		goto out_register;
	}

	tvr_major = MAJOR(dev_id);
	cdev_init(&tvr_cdev, &tvr_ops);
	cdev_add(&tvr_cdev, dev_id, TVR_HIDRAW_MAX_DEVICES);

	virtual_dir = virtual_device_parent(NULL);
	if (virtual_dir) {
		retval = sysfs_create_file(virtual_dir, &tvr_attrs[0]->attr);
		if (retval) {
			pr_warn("TVR: failed sysfs_create_file\n");
			kobject_put(virtual_dir);
			virtual_dir = NULL;
		}
	} 
	else {
		pr_warn("TVR: failed virtual_device_parent\n");
	}

	return 0;

out_register:
	hid_unregister_driver(&tvr_driver);

out_class:
	class_destroy(tvr_class);

	return retval;
}

static void __exit tvr_exit(void)
{
	dev_t dev_id = MKDEV(tvr_major, 0);

	cdev_del(&tvr_cdev);

	unregister_chrdev_region(dev_id, TVR_HIDRAW_MAX_DEVICES);

	hid_unregister_driver(&tvr_driver);

	class_destroy(tvr_class);

	sysfs_remove_file(virtual_dir, &tvr_attrs[0]->attr);
	kobject_put(virtual_dir);
	virtual_dir = NULL;    
}

module_init(tvr_init);
module_exit(tvr_exit);

MODULE_DESCRIPTION("USB TVR device driver.");
MODULE_LICENSE("GPL v2");
