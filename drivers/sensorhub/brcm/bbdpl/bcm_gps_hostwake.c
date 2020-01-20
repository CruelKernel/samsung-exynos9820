/******************************************************************************
 * Copyright (C) 2013 Broadcom Corporation
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *****************************************************************************
 */

#include <linux/device.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/unistd.h>
#include <linux/bug.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>

#include <linux/broadcom/bcm_gps_hostwake.h>

#define GPS_VERSION	"1.01"
#define PFX			"bcmgps: "

/// gps daemon will access "/dev/gps_geofence_wake"
#define HOST_WAKE_MODULE_NAME "gps_geofence_wake"

/// driver structure for HOST_WAKE module
struct gps_geofence_wake {
	/// irq from gpio_to_irq()
	int irq;
	/// HOST_WAKE_GPIO
	int host_req_pin;
	/// misc driver structure
	struct miscdevice misc;
	/// wake_lock
	struct wake_lock wake_lock;
};
static struct gps_geofence_wake g_geofence_wake;

static int gps_geofence_wake_open(struct inode *inode, struct file *filp)
{
	pr_debug(PFX "%s\n", __func__);
	return 0;
}

static int gps_geofence_wake_release(struct inode *inode, struct file *filp)
{
	pr_debug(PFX "%s\n", __func__);
	return 0;
}

static long gps_geofence_wake_ioctl(struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	pr_debug(PFX "%s\n", __func__);
	return 0;
}

static const struct file_operations gps_geofence_wake_fops = {
	.owner = THIS_MODULE,
	.open = gps_geofence_wake_open,
	.release = gps_geofence_wake_release,
	.unlocked_ioctl = gps_geofence_wake_ioctl
};

/// set/reset wake lock by HOST_WAKE level
/// \param gpio the value of HOST_WAKE_GPIO
static void gps_geofence_wake_lock(int gpio)
{
	struct gps_geofence_wake *ac_data = &g_geofence_wake;

	pr_debug(PFX "%s : gpio value = %d\n", __func__, gpio);

	if (gpio)
		wake_lock(&ac_data->wake_lock);
	else
		wake_unlock(&ac_data->wake_lock);
}

static irqreturn_t gps_host_wake_isr(int irq, void *dev)
{
	struct gps_geofence_wake *ac_data = &g_geofence_wake;
	int gps_host_wake = ac_data->host_req_pin;
	char gpio_value = 0x00;

	pr_debug(PFX "%s\n", __func__);

	gpio_value = gpio_get_value(gps_host_wake);

	// wake_lock
	gps_geofence_wake_lock(gpio_value);

	return IRQ_HANDLED;
}

/// initialize GPIO and IRQ
/// \param gpio the GPIO of HOST_WAKE
/// \return if SUCCESS, return the id of IRQ, if FAIL, return -EIO
static int gps_gpio_irq_init(int gpio)
{
	int ret = 0;
	int irq = 0;

	pr_debug(PFX "%s\n", __func__);
	// 1. Set GPIO
	if ((gpio_request(gpio, "HOST_WAKE"))) {
		pr_err(PFX "Can't request HOST_REQ GPIO %d.It may be already registered in
				init.xyz.3rdparty.rc/init.xyz.rc\n", gpio);
		return -EIO;
	}
	gpio_export(gpio, 1);
	gpio_direction_input(gpio);

	// 2. Set IRQ
	irq = gpio_to_irq(gpio);
	if (irq < 0) {
		pr_err(PFX "Could not get HOST_WAKE_GPIO = %d!, err = %d\n", gpio, irq);
		gpio_free(gpio);
		return -EIO;
	}

	ret = request_irq(irq, gps_host_wake_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOTa,
			"gps_host_wake", NULL);
	if (ret) {
		pr_err(PFX "Request_host wake irq failed.\n");
		gpio_free(gpio);
		return -EIO;
	}

	ret = irq_set_irq_wake(irq, 1);

	if (ret) {
		pr_err(PFX "Set_irq_wake failed.\n");
		gpio_free(gpio);
		free_irq(irq, NULL);
		return -EIO;
	}

	return irq;
}

/// cleanup GPIO and IRQ
static void gps_gpio_irq_cleanup(int gpio, int irq)
{
	pr_debug(PFX "%s\n", __func__);
	gpio_free(gpio);
	free_irq(irq, NULL);
}

static int gps_hostwake_probe(struct platform_device *pdev)
{
	int ret = 0;
	int irq = 0;

	struct gps_geofence_wake *ac_data = &g_geofence_wake;
	struct bcm_gps_hostwake_platform_data *pdata = 0;

   // unsigned int gpio_hostwake; /* HOST_WAKE : to indicate that ASIC has an geofence event. */

	pr_debug(PFX "%s\n", __func__);

	//  Read device tree information
	if (pdev->dev.platform_data) {
		pdata = pdev->dev.platform_data;
	} else {
		pr_debug(PFX "%s : fail to get platform_data.\n", __func__);
		return -1;
	}

	// 1. Init GPIO and IRQ for HOST_WAKE
	irq = gps_gpio_irq_init(pdata->gpio_hostwake);

	if (irq < 0)
		return -EIO;

	// 2. Register Driver
	memset(ac_data, 0, sizeof(struct gps_geofence_wake));

	// 2.1 Misc device setup
	ac_data->misc.minor = MISC_DYNAMIC_MINOR;
	ac_data->misc.name = HOST_WAKE_MODULE_NAME;
	ac_data->misc.fops = &gps_geofence_wake_fops;

	// 2.2 Information that be used later
	ac_data->irq = irq;
	ac_data->host_req_pin = pdata->gpio_hostwake;

	pr_notice(PFX "misc register, name %s, irq %d, host req pin num %d\n",
			ac_data->misc.name, irq, ac_data->host_req_pin);
	// 2.3 Register misc driver
	ret = misc_register(&ac_data->misc);
	if (ret != 0) {
		pr_err(PFX "cannot register gps geofence wake miscdev on minor=%d (%d)\n", MISC_DYNAMIC_MINOR, ret);
		return ret;
	}

	// 3. Init wake_lock
	wake_lock_init(&ac_data->wake_lock, WAKE_LOCK_SUSPEND, "gps_geofence_wakelock");
	return 0;
}

static int gps_hostwake_remove(struct platform_device *pdev)
{
	struct gps_geofence_wake *ac_data = &g_geofence_wake;
	int ret = 0;

	pr_debug(PFX "%s\n", __func__);
	// 1. Cleanup GPIO and IRQ
	gps_gpio_irq_cleanup(ac_data->host_req_pin, ac_data->irq);

	// 2. Cleanup driver
	ret = misc_deregister(&ac_data->misc);
	if (ret != 0) {
		pr_err(PFX "cannot unregister gps geofence wake miscdev on minor=%d (%d)\n", MISC_DYNAMIC_MINOR, ret);
		return ret;
	}

	return 0;
}

static int gps_hostwake_suspend(struct platform_device *pdev, pm_message_t state)
{
	pr_debug(PFX "%s\n", __func__);
	return 0;
}

static int gps_hostwake_resume(struct platform_device *pdev)
{
	pr_debug(PFX "%s\n", __func__);
	return 0;
}

static struct platform_driver gps_driver = {
	.probe		= gps_hostwake_probe,
	.remove		= gps_hostwake_remove,
	.suspend	= gps_hostwake_suspend,
	.resume		= gps_hostwake_resume,
	.driver = {
		.name = "bcm-gps-hostwake",
		.owner = THIS_MODULE,
	},
};

static int __init gps_hostwake_init(void)
{
	pr_notice(PFX "Init: Broadcom GPS HostWake Geofence Driver v%s\n", GPS_VERSION);
	return platform_driver_register(&gps_driver);
}

static void __exit gps_hostwake_exit(void)
{
	pr_notice(PFX "Exit: Broadcom GPS HostWake Geofence Driver v%s\n", GPS_VERSION);
	platform_driver_unregister(&gps_driver);
}

module_init(gps_hostwake_init);
module_exit(gps_hostwake_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Broadcom GPS Driver with hostwake interrupt");
