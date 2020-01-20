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

#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <linux/i2c.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/kthread.h>

#include <linux/completion.h>
#include <linux/atomic.h>

#include <linux/kallsyms.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/suspend.h>

#include "../../input/touchscreen/stm/fts_ts.h"
#include "tz_cdev.h"
#include "tztui.h"


MODULE_AUTHOR("Egor Uleyskiy <e.uleyskiy@samsung.com>");
MODULE_DESCRIPTION("TZTUI driver"); /* TrustZone Trusted User Interface */
MODULE_LICENSE("GPL");
int tztui_verbosity = 2;
module_param(tztui_verbosity, int, 0644);
MODULE_PARM_DESC(tztui_verbosity, "0: normal, 1: verbose, 2: debug");


#define TUI_OFFLINE                 0x0
#define TUI_ONLINE                  0x1

#define TZTUI_IOCTL_OPEN_SESSION    0x10
#define TZTUI_IOCTL_CLOSE_SESSION   0x11
#define TZTUI_IOCTL_CANCEL          0x12

#define TZTUI_IOCTL_BLANK_FB        0x20
#define TZTUI_IOCTL_UNBLANK_FB      0x21

#define TZTUI_IOCTL_GETINFO         0x30




struct dummy_qup_i2c_dev {
	struct device *dev;
};

static DEFINE_SPINLOCK(tui_event_lock);
static DEFINE_MUTEX(tui_mode_mutex);
static DECLARE_WAIT_QUEUE_HEAD(irq_wait_queue);

static bool tui_status = false;
static int tui_mode = TUI_OFFLINE;
static unsigned int tui_event_flag = 0;
static volatile int thread_handler_stop = 0;

static struct device *local_dev, *local_i2c_dev;
static struct fts_ts_info *local_fts_info;

static irq_handler_t local_irq_handler;

static struct task_struct *ts;
static const char i2c_dev_name[] = "1-0049";
static atomic_t touch_requested;

static struct completion irq_completion;
static struct wake_lock wakelock;

static int request_touch(void);
static int release_touch(void);

static int fts_pm_notify(struct notifier_block *b, unsigned long ev, void *p)
{
	(void)b;
	(void)p;
	DBG("%s: ev=%lu", __func__, ev);
	if (ev == PM_SUSPEND_PREPARE ||
		ev == PM_HIBERNATION_PREPARE)
		release_touch();
	return NOTIFY_DONE;
}

static struct notifier_block fts_pm_notifier;

int tui_get_current(void)
{
	int mode;

	mutex_lock(&tui_mode_mutex);
	mode = tui_mode;
	mutex_unlock(&tui_mode_mutex);
	return mode;
}

int tui_set_mode(int mode)
{
	int ret_val = (-EINVAL);

	DBG("%s(%d) : Enter", __func__, mode);
	mutex_lock(&tui_mode_mutex);
	if (mode == tui_mode)
		ret_val = (-EALREADY);
	else if (mode == TUI_ONLINE) {
		ret_val = request_touch();
		if (!ret_val) {
			tui_mode = mode;
			tui_event_flag = 0;
			thread_handler_stop = 0;
		}
	}
	else if (mode == TUI_OFFLINE) {
		ret_val = release_touch();
		if (!ret_val)
			tui_mode = mode;
	}
	mutex_unlock(&tui_mode_mutex);
	return ret_val;
}

void tui_set_event(unsigned int event)
{
	unsigned long flags;

	spin_lock_irqsave(&tui_event_lock, flags);
	tui_event_flag |= event;
	spin_unlock_irqrestore(&tui_event_lock, flags);
	wake_up(&irq_wait_queue);
}

/* Get pointer to synaptic driver on i2c bus */
struct device *get_fts_device(const char *name)
{
	struct device *dev = NULL;
	DBG("%s() : Enter", __func__);
	dev = bus_find_device_by_name(&i2c_bus_type, NULL, name);
	return dev;
}

int thread_handler(void *data)
{
	unsigned long flags, event;
	(void)data;

	do {
		if (wait_event_interruptible(irq_wait_queue, (tui_event_flag || thread_handler_stop)) == ERESTARTSYS)
			break;

		spin_lock_irqsave(&tui_event_lock, flags);
		event = tui_event_flag;
		tui_event_flag = 0;
		spin_unlock_irqrestore(&tui_event_lock, flags);

		if (thread_handler_stop || kthread_should_stop())
			break;

		if (event) {
			tztui_smc_tui_event(event);
			complete(&irq_completion);
		}
	} while (1);
	return 0;
}

/* GPIO irq handler */
static irqreturn_t tui_fts_irq(int irq, void *data)
{
	(void)irq;
	(void)data;

	tui_set_event(TUI_EVENT_TOUCH_IRQ);
	wait_for_completion_interruptible_timeout(&irq_completion, msecs_to_jiffies(10 * MSEC_PER_SEC));

	return IRQ_HANDLED;
}

static irq_handler_t find_irqhandler(int irq, void *dev_id)
{
	typedef struct irq_desc* (*irq_to_desc_func_t) (unsigned int irq);
	irq_to_desc_func_t irq_to_desc = NULL;
	struct irq_desc *desc;
	struct irqaction *action = NULL;
	struct irqaction *action_ptr = NULL;
	unsigned long flags;

	irq_to_desc = (irq_to_desc_func_t)((void **)kallsyms_lookup_name("irq_to_desc"));
	if (!irq_to_desc)
		return NULL;

	desc = irq_to_desc(irq);
	if (!desc)
		return NULL;

	raw_spin_lock_irqsave(&desc->lock, flags);
	action_ptr = desc->action;
	while (1) {
		action = action_ptr;
		if (!action) {
			raw_spin_unlock_irqrestore(&desc->lock, flags);
			return NULL;
		}
		if (action->dev_id == dev_id) {
			DBG("FOUND thread_fn=%pK", action->thread_fn);
			DBG("FOUND handler=%pK", action->handler);
			break;
		}

		action_ptr = action->next;
	}

	if (!action->handler) {
		raw_spin_unlock_irqrestore(&desc->lock, flags);
		return NULL;
	}

	raw_spin_unlock_irqrestore(&desc->lock, flags);
	return action->thread_fn;
}

/* Get pointer to qup-i2c device connected to touch controller */
struct device *get_i2c_device(struct fts_ts_info *fts_info)
{
	struct device *dev = NULL;
	struct i2c_adapter *i2c_adapt;
	struct dummy_qup_i2c_dev *qup_dev;

	DBG("%s() : Enter", __func__);
	if (fts_info && fts_info->client) {
		i2c_adapt = fts_info->client->adapter;
		if (i2c_adapt) {
			qup_dev = i2c_get_adapdata(i2c_adapt);
			if (qup_dev)
				dev = qup_dev->dev;
		}
	}
	return dev;
}

/* Find synaptic touch driver by name and try to disallow its working */
static int request_touch(void)
{
	int retval = -EFAULT;
	struct device *dev = NULL, *i2c_dev = NULL;
	struct fts_ts_info *fts_info;

	irq_handler_t irqhandler = NULL;

	(void)tui_fts_irq;

	DBG("%s() : Enter", __func__);
	if (atomic_read(&touch_requested) == 1)
		return -EALREADY;


	dev = get_fts_device(i2c_dev_name);
	if (!dev) {
		ERR("FTS Device %s not found.", i2c_dev_name);
		retval = -ENXIO;
		goto exit_err;
	}
	fts_info = dev_get_drvdata(dev);
	if (!fts_info) {
		ERR("Touchscreen driver internal data is empty.");
		retval = -EFAULT;
		goto exit_err;
	}
	wake_lock(&wakelock);
	wake_lock(&fts_info->wakelock);

	/*mutex_lock(&fts_info->device_mutex);*/
	if (fts_info->touch_stopped) {
		DBG("Sensor stopped.");
		retval = -EBUSY;
		goto exit_err;
	}

	if (pm_runtime_get(fts_info->client->adapter->dev.parent) < 0)
		ERR("pm_runtime_get error");

	INIT_COMPLETION(fts_info->st_powerdown);
	/*INIT_COMPLETION(info->st_interrupt);*/

	i2c_dev = get_i2c_device(fts_info);
	if (!i2c_dev) {
		ERR("QUP-I2C Device not found.");
		retval = -ENXIO;
		goto exit_err2;
	}

	DBG("start");

	irqhandler = find_irqhandler(fts_info->irq, fts_info);
	if (!irqhandler)
		ERR("irqhandler not found");


	synchronize_irq(fts_info->client->irq);
	local_irq_handler = irqhandler;

	fts_info->touch_stopped = true;
	/* disable irq */
	if (fts_info->irq_enabled) {
		disable_irq(fts_info->irq);
		free_irq(fts_info->irq, fts_info);
		DBG("Disable IRQ of fts_touch");
		fts_info->irq_enabled = false;
	}

	local_dev = dev;
	local_i2c_dev = i2c_dev;
	local_fts_info = fts_info;

	INIT_COMPLETION(irq_completion);
	retval = request_threaded_irq(fts_info->irq, NULL, tui_fts_irq, IRQF_TRIGGER_LOW | IRQF_ONESHOT, "tztui", tui_fts_irq);
	/*retval = request_irq(fts_info->irq, tui_fts_irq, IRQF_TRIGGER_LOW | IRQF_ONESHOT, "tztui", tui_fts_irq);*/

	if (retval < 0) {
		ERR("Request IRQ failed:%d", retval);
		goto exit_err3;
	}

	atomic_set(&touch_requested, 1);
	fts_pm_notifier.notifier_call = fts_pm_notify;
	fts_pm_notifier.priority = 0;
	retval = register_pm_notifier(&fts_pm_notifier);

	if (retval < 0) {
		DBG("register_pm_notifier: retval=%d", retval);
		goto exit_err4;
	}

	DBG("Touch requested");
	return 0;

exit_err4:
	retval = unregister_pm_notifier(&fts_pm_notifier);
	if (retval < 0)
		DBG("unregister_pm_notifier: retval=%d", retval);

exit_err3:
	disable_irq(fts_info->irq);
	retval = request_threaded_irq(fts_info->irq, NULL, irqhandler, IRQF_TRIGGER_LOW | IRQF_ONESHOT, FTS_TS_DRV_NAME, fts_info);
	if (retval < 0)
		ERR("Request IRQ FATAL:%d", retval);

	fts_info->irq_enabled = true;
	wake_unlock(&fts_info->wakelock);
	wake_unlock(&wakelock);
	complete(&fts_info->st_powerdown);
exit_err2:
	pm_runtime_put(fts_info->client->adapter->dev.parent);
exit_err:
	fts_info->touch_stopped = false;
	local_dev = NULL;
	local_fts_info = NULL;
	local_i2c_dev = NULL;
	local_irq_handler = NULL;

	return retval;
}


/* Release synaptic touch driver */
static int release_touch(void)
{
	int retval = -EFAULT;
	struct fts_ts_info *fts_info = local_fts_info;
	struct device *i2c_dev = local_i2c_dev, *dev = local_dev;
	irq_handler_t irqhandler = local_irq_handler;

	(void) i2c_dev;
	(void) dev;

	DBG("%s() : Enter", __func__);
	if (atomic_read(&touch_requested) != 1)
		return -EALREADY;

	synchronize_irq(fts_info->client->irq);
	fts_info->touch_stopped = false;
	if (!fts_info->irq_enabled) {
		disable_irq(fts_info->irq);
		free_irq(fts_info->irq, tui_fts_irq);
		retval = request_threaded_irq(fts_info->irq, NULL, irqhandler, IRQF_TRIGGER_LOW | IRQF_ONESHOT, FTS_TS_DRV_NAME, fts_info);
		if (retval < 0) {
			ERR("Request IRQ failed:%d", retval);
			goto exit_err;
		}
		fts_info->irq_enabled = true;
		DBG("fts_touch irqhandler restored");
	}
	retval = pm_runtime_put(fts_info->client->adapter->dev.parent);
	/*mutex_unlock(&fts_info->device_mutex);*/
	if (retval < 0)
		ERR("pm_runtime_put fauled:%d", retval);

	retval = unregister_pm_notifier(&fts_pm_notifier);
	DBG("%s: retval=%d", __func__, retval);
	wake_unlock(&fts_info->wakelock);
	wake_unlock(&wakelock);
	complete(&fts_info->st_powerdown);
exit_err:
	atomic_set(&touch_requested, 0);
	return retval;
}

static long tztui_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc = -ENOTTY;
	(void)arg;
	(void)file;

	switch (cmd) {
	case TZTUI_IOCTL_OPEN_SESSION:
		rc = tui_set_mode(TUI_ONLINE);
		break;

	case TZTUI_IOCTL_CLOSE_SESSION:
		rc = tui_set_mode(TUI_OFFLINE);
		break;

	case TZTUI_IOCTL_CANCEL:
		if (tui_get_current() == TUI_ONLINE) {
			tui_set_event(TUI_EVENT_CANCEL);
			rc = 0;
		} else
			rc = -EPERM;

		break;

	default:
		return -ENOTTY; /* Wrong or not-implemented function */
	}
	return rc;
}

static int tztui_open(struct inode *inode, struct file *filp)
{
	mutex_lock(&tui_mode_mutex);
	if (!tui_status) {
		tui_status = true;
		mutex_unlock(&tui_mode_mutex);
		return 0;
	}
	else {
		mutex_unlock(&tui_mode_mutex);
		ERR("TUI Device already used");
		return -EBUSY;
	}
}

static int tztui_release(struct inode *inode, struct file *filp)
{
	mutex_lock(&tui_mode_mutex);
	tui_status = false;
	mutex_unlock(&tui_mode_mutex);
	return 0;
}


static const struct file_operations tztui_fops = {
	.owner = THIS_MODULE,
	.open = tztui_open,
	.release = tztui_release,
	.unlocked_ioctl = tztui_ioctl,
};

static struct tz_cdev tztui_cdev = {
	.name = "tztui",
	.fops = &tztui_fops,
	.owner = THIS_MODULE,
};

static int __init tztui_init(void)
{
	int rc;
	int cpu_num = 0;
	(void)cpu_num;
	(void)ts;
	(void)thread_handler_stop;
	(void)irq_wait_queue;

	wake_lock_init(&wakelock, WAKE_LOCK_SUSPEND, "tztui_wakelock");
	ts = kthread_create(thread_handler, NULL, "per_cpu_thread");
	kthread_bind(ts, cpu_num);
	if (!IS_ERR(ts))
		wake_up_process(ts);
	else {
		ERR("Failed to bind thread to CPU %d", cpu_num);
		return -1;
	}

	init_completion(&irq_completion);

	rc = tz_cdev_register(&tztui_cdev);
	if (rc) {
		thread_handler_stop = 1;
		wake_up(&irq_wait_queue);
		kthread_stop(ts);
		ts = NULL;
	}

	return rc;
}

static void __exit tztui_exit(void)
{
	tui_set_mode(TUI_OFFLINE);
	wake_lock_destroy(&wakelock);
	if (ts) {
		thread_handler_stop = 1;
		wake_up(&irq_wait_queue);
		kthread_stop(ts);
		ts = NULL;
	}
	tz_cdev_unregister(&tztui_cdev);
}

module_init(tztui_init);
module_exit(tztui_exit);
