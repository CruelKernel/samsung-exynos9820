/*
 * Copyright (C) 2011 Samsung Electronics.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>

#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>

#include "modem_prj.h"

#define SPI_XMIT_DELEY	100

int check_cp_status(unsigned int gpio_cp_status, unsigned int count)
{
	int ret = 0;
	int cnt = 0;
	int val;

	while (1) {
		val = gpio_get_value(gpio_cp_status);
		mif_err("CP2AP_WAKEUP == %d (cnt %d)\n", val, cnt);

		if (val != 0) {
			ret = 0;
			break;
		}

		cnt++;
		if (cnt >= count) {
			mif_err("ERR! CP2AP_WAKEUP == 0 (cnt %d)\n", cnt);
			ret = -EFAULT;
			break;
		}

		msleep(20);
	}

	if (ret == 0)
		mif_info("CP2AP_WAKEUP == 1 cnt: %d\n", cnt);
	else
		mif_info("ERR: Checking count after sending bootloader: %d\n", cnt);

	if (cnt == 0)
		msleep(10);

	return ret;
}

int check_cp_status_low(unsigned int gpio_cp_status, unsigned int count)
{
	int ret = 0;
	int cnt = 0;

	mif_info("Checking CP2AP_WAKEUP LOW before sending bootloader\n");

	while (1) {
		if (gpio_get_value(gpio_cp_status) == 0) {
			ret = 0;
			break;
		}

		cnt++;
		if (cnt >= count) {
			mif_err("ERR! CP2AP_WAKEUP != 0 (cnt %d)\n", cnt);
			ret = -EFAULT;
			break;
		}

		msleep(20);
	}

	mif_info("CP2AP_WAKEUP == 0 : Checking count before sending bootloader: %d\n", cnt);
	if (cnt == 0)
		msleep(10);

	return ret;
}

static inline int spi_send(struct modem_boot_spi *loader, const char *buff,
			unsigned len)
{
	int ret;
	struct spi_message msg;
	struct spi_transfer xfer = {
		.len = len,
		.tx_buf = buff,
	};

	printk("Send CP bootloader with SPI : 0x%x bytes\n", len);

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

	ret = spi_sync(loader->spi_dev, &msg);
	if (ret < 0)
		mif_err("ERR! spi_sync fail (err %d)\n", ret);

	return ret;
}

static int spi_boot_write(struct modem_boot_spi *loader, const char *addr,
			const long len)
{
	int ret = 0;
	char *buff = NULL;
	mif_err("+++\n");

	buff = kzalloc(len, GFP_KERNEL);
	if (!buff) {
		mif_err("ERR! kzalloc(%ld) fail\n", len);
		ret = -ENOMEM;
		goto exit;
	}

	ret = copy_from_user(buff, (const void __user *)addr, len);
	if (ret) {
		mif_err("ERR! copy_from_user fail (err %d)\n", ret);
		ret = -EFAULT;
		goto exit;
	}

	printk("Send bootloader by SPI (Len:%ld)\n", len);

	ret = spi_send(loader, buff, len);
	if (ret < 0) {
		mif_err("ERR! spi_send fail (err %d)\n", ret);
		goto exit;
	}

exit:
	if (buff)
		kfree(buff);

	mif_err("---\n");
	return ret;
}

static int spi_boot_open(struct inode *inode, struct file *filp)
{
	struct modem_boot_spi *loader = to_modem_boot_spi(filp->private_data);
	filp->private_data = loader;
	return 0;
}

static long spi_boot_ioctl(struct file *filp, unsigned int cmd,
			unsigned long arg)
{
	int ret = 0;
	struct modem_firmware img;
	struct modem_boot_spi *loader = filp->private_data;

	mutex_lock(&loader->lock);
	switch (cmd) {
	case IOCTL_MODEM_XMIT_BOOT:
		ret = copy_from_user(&img, (const void __user *)arg,
					sizeof(struct modem_firmware));
		if (ret) {
			mif_err("ERR! copy_from_user fail (err %d)\n", ret);
			ret = -EFAULT;
			goto exit_err;
		}

		mif_info("IOCTL_MODEM_XMIT_BOOT (size %d)\n", img.size);
		if (img.size == 0) {
			mif_err("ERR! img.size is 0\n");
			break;
		}

		if (!loader->gpio_cp_status) {
			mif_err("ERR! gpio_cp_status is null\n");
			break;
		}

		mif_info("Ignore CP2AP_WAKEUP Low\n");
		msleep(30);

		ret = spi_boot_write(loader, (char *)img.binary, img.size);
		if (ret < 0) {
			mif_err("ERR! spi_boot_write fail (err %d)\n", ret);
			break;
		}

		ret = check_cp_status(loader->gpio_cp_status, 200);
		if (ret < 0)
			mif_err("ERR! check_cp_status fail (err %d)\n", ret);

		break;

	default:
		mif_err("ioctl cmd error\n");
		ret = -ENOIOCTLCMD;

		break;
	}

exit_err:
	mutex_unlock(&loader->lock);

	return ret;
}

static const struct file_operations modem_spi_boot_fops = {
	.owner = THIS_MODULE,
	.open = spi_boot_open,
	.unlocked_ioctl = spi_boot_ioctl,
};

static int __init modem_spi_boot_probe(struct spi_device *spi)
{
	int ret;
	struct device *dev = &spi->dev;
	struct modem_boot_spi *loader;
	mif_err("+++ SPI boot +++\n");

	loader = kzalloc(sizeof(*loader), GFP_KERNEL);
	if (!loader) {
		mif_err("failed to allocate for modem_boot_spi\n");
		ret = -ENOMEM;
		goto err_alloc;
	}
	mutex_init(&loader->lock);

	spi->bits_per_word = 8;

	if (spi_setup(spi)) {
		mif_err("ERR! spi_setup fail\n");
		ret = -EINVAL;
		goto err_setup;
	}
	loader->spi_dev = spi;

	if (dev->of_node) {
		struct device_node *np;
		unsigned gpio_cp_status;

		np = of_find_compatible_node(NULL, NULL,
					     "sec_modem,modem_pdata");
		if (!np) {
			mif_err("DT, failed to get node\n");
			ret = -EINVAL;
			goto err_setup;
		}

		gpio_cp_status = of_get_named_gpio(np, "gpio_cp2ap_wake_up", 0);
		if (!gpio_is_valid(gpio_cp_status)) {
			mif_err("gpio_cp_status: Invalied gpio pins\n");
			ret = -EINVAL;
			goto err_setup;
		}
		gpio_request(gpio_cp_status, "gpio_cp_status");

		mif_err("gpio_cp_status: %d\n", gpio_cp_status);

		loader->gpio_cp_status = gpio_cp_status;
	} else {
		struct modem_boot_spi_platform_data *pdata;

		pdata = dev->platform_data;
		if (!pdata) {
			mif_err("Non-DT, incorrect pdata!\n");
			ret = -EINVAL;
			goto err_setup;
		}

		loader->gpio_cp_status = pdata->gpio_cp_status;
	}

	spi_set_drvdata(spi, loader);

	loader->misc_dev.minor = MISC_DYNAMIC_MINOR;
	loader->misc_dev.name = "modem_boot_spi";
	loader->misc_dev.fops = &modem_spi_boot_fops;
	ret = misc_register(&loader->misc_dev);
	if (ret) {
		mif_err("ERR! misc_register fail (err %d)\n", ret);
		goto err_setup;
	}

#ifdef AIRPLANE_MODE_TEST
	lte_airplane_mode = 0;
#endif
	mif_err("---\n");
	return 0;

err_setup:
	mutex_destroy(&loader->lock);
	kfree(loader);

err_alloc:
	mif_err("xxx\n");
	return ret;
}

static int modem_spi_boot_remove(struct spi_device *spi)
{
	struct modem_boot_spi *loader = spi_get_drvdata(spi);

	misc_deregister(&loader->misc_dev);
	mutex_destroy(&loader->lock);
	kfree(loader);

	return 0;
}
#ifdef CONFIG_OF
static const struct of_device_id modem_boot_spi_dt_match[] = {
	{ .compatible = "modem_boot_spi" },
	{},
};
MODULE_DEVICE_TABLE(of, modem_boot_spi_dt_match);
#endif
static struct spi_driver modem_spi_boot_driver = {
	.driver = {
		.name = "modem_boot_spi",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(modem_boot_spi_dt_match),
	},
	.probe = modem_spi_boot_probe,
	.remove =modem_spi_boot_remove,
};

static int __init modem_spi_boot_init(void)
{
	int err;
	mif_err("+++\n");

	err = spi_register_driver(&modem_spi_boot_driver);
	if (err) {
		mif_err("spi_register_driver fail (err %d)\n", err);
		mif_err("xxx\n");
		return err;
	}

	mif_err("---\n");
	return 0;
}

static void __exit modem_spi_boot_exit(void)
{
	spi_unregister_driver(&modem_spi_boot_driver);
}

module_init(modem_spi_boot_init);
module_exit(modem_spi_boot_exit);

MODULE_DESCRIPTION("SPI Driver for Downloading Modem Bootloader");
MODULE_LICENSE("GPL");
