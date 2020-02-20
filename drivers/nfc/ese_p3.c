/*
 * Copyright (C) 2015 Samsung Electronics. All rights reserved.
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
 * along with this program;
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/spi/spi.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/regulator/consumer.h>
#include <linux/ioctl.h>
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif

#include <linux/platform_data/spi-s3c64xx.h>
#include <linux/pm_runtime.h>
#include <linux/spi/spidev.h>
#include <linux/clk.h>
#include <linux/wakelock.h>
#include <linux/smc.h>
#include "../misc/tzdev/include/tzdev/tee_client_api.h"

#include "ese_p3.h"

/* Undef if want to keep eSE Power LDO ALWAYS ON */
#define FEATURE_ESE_POWER_ON_OFF

#define SPI_DEFAULT_SPEED 6500000L

#ifdef CONFIG_ESE_SECURE
static TEEC_UUID ese_drv_uuid = {
	0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x65, 0x73, 0x65, 0x44, 0x72, 0x76}
};

enum pm_mode {
	PM_SUSPEND,
	PM_RESUME,
	SECURE_CHECK,
};

enum secure_state {
	NOT_CHECKED,
	ESE_SECURED,
	ESE_NOT_SECURED,
};
#endif
/* size of maximum read/write buffer supported by driver */
#define MAX_BUFFER_SIZE   260U

/* Different driver debug lever */
enum P3_DEBUG_LEVEL {
	P3_DEBUG_OFF,
	P3_FULL_DEBUG
};

/* Variable to store current debug level request by ioctl */
static unsigned char debug_level = P3_FULL_DEBUG;

#define P3_DBG_MSG(msg...) do { \
		switch (debug_level) { \
		case P3_DEBUG_OFF: \
			break; \
		case P3_FULL_DEBUG: \
			pr_info("[ESE-P3] :  " msg); \
			break; \
			 /*fallthrough*/ \
		default: \
			pr_err("[ESE-P3] : debug level %d", debug_level);\
			break; \
		}; \
	} while (0)

#define P3_ERR_MSG(msg...) pr_err("[ESE-P3] : " msg)
#define P3_INFO_MSG(msg...) pr_info("[ESE-P3] : " msg)

static DEFINE_MUTEX(device_list_lock);

/* Device specific macro and structure */
struct p3_data {
	wait_queue_head_t read_wq; /* wait queue for read interrupt */
	struct mutex buffer_mutex; /* buffer mutex */
	struct spi_device *spi;  /* spi device structure */
	struct miscdevice p3_device; /* char device as misc driver */

	unsigned int users;

	bool device_opened;
#ifdef FEATURE_ESE_WAKELOCK
	struct wake_lock ese_lock;
#endif
	int cs_gpio;
	unsigned long speed;
	const char *vdd_1p8;
#ifdef CONFIG_ESE_SECURE
	struct clk *ese_spi_pclk;
	struct clk *ese_spi_sclk;
	int ese_secure_check;
#endif
};

#ifndef CONFIG_ESE_SECURE
static void p3_pinctrl_config(struct p3_data *data, bool onoff)
{
	struct spi_device *spi = data->spi;
	struct device *spi_dev = spi->dev.parent->parent;
	struct pinctrl *pinctrl = NULL;

	P3_INFO_MSG("%s: pinctrol - %s\n", __func__, onoff ? "on" : "off");

	if (onoff) {
		/* ON */
		pinctrl = devm_pinctrl_get_select(spi_dev, "ese_active");
		if (IS_ERR_OR_NULL(pinctrl))
			P3_ERR_MSG("%s: Failed to configure ese pin\n", __func__);
		else
			devm_pinctrl_put(pinctrl);
	} else {
		/* OFF */
		pinctrl = devm_pinctrl_get_select(spi_dev, "ese_suspend");
		if (IS_ERR_OR_NULL(pinctrl))
			P3_ERR_MSG("%s: Failed to configure ese pin\n", __func__);
		else
			devm_pinctrl_put(pinctrl);
	}
}
#endif

#ifdef CONFIG_ESE_SECURE
static uint32_t tz_tee_ese_drv(enum pm_mode mode)
{
	TEEC_Context context;
	TEEC_Session session;
	TEEC_Result result;
	uint32_t returnOrigin = TEEC_NONE;

	result = TEEC_InitializeContext(NULL, &context);
	if (result != TEEC_SUCCESS)
		goto out;

	result = TEEC_OpenSession(&context, &session, &ese_drv_uuid, TEEC_LOGIN_PUBLIC,
			NULL, NULL, &returnOrigin);
	if (result != TEEC_SUCCESS)
		goto finalize_context;

	/* test with valid cmd id, expected result : TEEC_SUCCESS */
	result = TEEC_InvokeCommand(&session, mode, NULL, &returnOrigin);
	if (result != TEEC_SUCCESS) {
		P3_ERR_MSG("%s with cmd %d : FAIL\n", __func__, mode);
		goto close_session;
	}

	P3_ERR_MSG("eSE tz_tee_dev return origin %d\n", returnOrigin);

close_session:
	TEEC_CloseSession(&session);
finalize_context:
	TEEC_FinalizeContext(&context);
out:
	P3_INFO_MSG("tz_tee_ese_drv, cmd %s result=%#x origin=%#x\n", mode ? "Resume" :"Suspend " , result, returnOrigin);

	return result;
}
extern int tz_tee_ese_secure_check(void);
int tz_tee_ese_secure_check()
{
	return	tz_tee_ese_drv(SECURE_CHECK);
}

static int p3_clk_control(struct p3_data *data, bool onoff)
{
	static bool old_value;

	if (old_value == onoff)
		return 0;

	if (onoff == true) {
		clk_prepare_enable(data->ese_spi_pclk);
		clk_prepare_enable(data->ese_spi_sclk);

		/* There is a quarter-multiplier before the USI_v2 SPI */
		clk_set_rate(data->ese_spi_sclk, data->speed * 4);
		usleep_range(5000, 5100);
		P3_DBG_MSG("%s clock:%lu\n", __func__, clk_get_rate(data->ese_spi_sclk));
	} else {
		clk_disable_unprepare(data->ese_spi_pclk);
		clk_disable_unprepare(data->ese_spi_sclk);
	}

	old_value = onoff;

	P3_INFO_MSG("clock %s\n", onoff ? "enabled" : "disabled");
	return 0;
}

static int p3_clk_setup(struct device *dev, struct p3_data *data)
{
	data->ese_spi_pclk = clk_get(dev, "pclk");
	if (IS_ERR(data->ese_spi_pclk)) {
		P3_ERR_MSG("Can't get %s\n", "pclk");
		data->ese_spi_pclk = NULL;
		goto err_pclk_get;
	}

	data->ese_spi_sclk = clk_get(dev, "sclk");
	if (IS_ERR(data->ese_spi_sclk)) {
		P3_ERR_MSG("Can't get %s\n", "sclk");
		data->ese_spi_sclk = NULL;
		goto err_sclk_get;
	}

	return 0;
err_sclk_get:
	clk_put(data->ese_spi_pclk);
err_pclk_get:
	return -EPERM;
}
#endif

static int p3_regulator_onoff(struct p3_data *data, int onoff)
{
	int rc = 0;
	struct regulator *regulator_vdd_1p8;

	if (!data->vdd_1p8) {
		pr_err("%s No vdd LDO name!\n", __func__);
		return -ENODEV;
	}

	regulator_vdd_1p8 = regulator_get(NULL, data->vdd_1p8);
	pr_err("%s %s\n", __func__, data->vdd_1p8);

	if (IS_ERR(regulator_vdd_1p8) || regulator_vdd_1p8 == NULL) {
		P3_ERR_MSG("%s - vdd_1p8 regulator_get fail\n", __func__);
		return -ENODEV;
	}

	P3_DBG_MSG("%s - onoff = %d\n", __func__, onoff);
	if (onoff == 1) {
		rc = regulator_enable(regulator_vdd_1p8);
		if (rc) {
			P3_ERR_MSG("%s - enable vdd_1p8 failed, rc=%d\n",
				__func__, rc);
			goto done;
		}
		msleep(20);
	} else {
		rc = regulator_disable(regulator_vdd_1p8);
		if (rc) {
			P3_ERR_MSG("%s - disable vdd_1p8 failed, rc=%d\n",
				__func__, rc);
			goto done;
		}
	}

	/*data->regulator_is_enable = (u8)onoff;*/

done:
	regulator_put(regulator_vdd_1p8);

	return rc;
}

#ifndef CONFIG_ESE_SECURE
static int p3_xfer(struct p3_data *p3_device, struct p3_ioctl_transfer *tr)
{
	int status = 0;
	struct spi_message m;
	struct spi_transfer t;
	unsigned char tx_buffer[MAX_BUFFER_SIZE] = {0x0, };
	unsigned char rx_buffer[MAX_BUFFER_SIZE] = {0x0, };

	P3_DBG_MSG("%s\n", __func__);

	if (p3_device == NULL || tr == NULL)
		return -EFAULT;

	if (tr->len > MAX_BUFFER_SIZE || !tr->len) {
		P3_ERR_MSG("%s invalid size\n", __func__);
		return -EMSGSIZE;
	}

	if (tr->tx_buffer != NULL) {
		if (copy_from_user(tx_buffer,
				tr->tx_buffer, tr->len) != 0)
			return -EFAULT;
	}

	spi_message_init(&m);
	memset(&t, 0, sizeof(t));

	t.tx_buf = tx_buffer;
	t.rx_buf = rx_buffer;
	t.len = tr->len;

	spi_message_add_tail(&t, &m);

	status = spi_sync(p3_device->spi, &m);
	if (status == 0) {
		if (tr->rx_buffer != NULL) {
			unsigned int missing = 0;

			missing = (unsigned int)copy_to_user(tr->rx_buffer,
					       rx_buffer, tr->len);

			if (missing != 0)
				tr->len = tr->len - missing;
		}
	}
	pr_debug("%s, length=%d\n", __func__, tr->len);
	return status;

} /* vfsspi_xfer */

static int p3_rw_spi_message(struct p3_data *p3_device,
				 unsigned long arg)
{
	struct p3_ioctl_transfer   *dup = NULL;
	int err = 0;

	dup = kmalloc(sizeof(struct p3_ioctl_transfer), GFP_KERNEL);
	if (dup == NULL)
		return -ENOMEM;

	if (copy_from_user(dup, (void *)arg,
			   sizeof(struct p3_ioctl_transfer)) != 0) {
		kfree(dup);
		return -EFAULT;
	}

	err = p3_xfer(p3_device, dup);
	if (err != 0) {
		kfree(dup);
		P3_ERR_MSG("%s xfer failed!\n", __func__);
		return err;
	}

	/*P3_ERR_MSG("%s len:%u\n", __func__, dup->len);*/
	if (copy_to_user((void *)arg, dup,
			 sizeof(struct p3_ioctl_transfer)) != 0)
		return -EFAULT;
	kfree(dup);
	return 0;
}

#if 0 //def CONFIG_COMPAT
static int p3_rw_spi_message_32(struct p3_data *p3_device, unsigned long arg)
{
	struct p3_ioctl_transfer dup;
	struct spip3_ioc_transfer_32 p3transfr_32;
	int err = 0;

	if (__copy_from_user(&p3transfr_32, (void __user *)arg,
				sizeof(struct spip3_ioc_transfer_32))) {
		P3_ERR_MSG("%s, failed to copy from user\n", __func__);
		return -EFAULT;
	}

	dup.tx_buffer = (unsigned char *)(unsigned long)(p3transfr_32.tx_buffer);
	dup.rx_buffer = (unsigned char *)(unsigned long)(p3transfr_32.rx_buffer);
	dup.len = p3transfr_32.len;

	err = p3_xfer(p3_device, &dup);
	if (err != 0) {
		P3_ERR_MSG("%s xfer failed!\n", __func__);
		return err;
	}
	P3_DBG_MSG("%s len:%u\n", __func__, dup.len);

	return 0;
}
#endif
#endif

static int spip3_open(struct inode *inode, struct file *filp)
{
	struct p3_data *p3_dev = container_of(filp->private_data,
			struct p3_data, p3_device);
	int ret = 0;

#ifdef CONFIG_ESE_SECURE
	if (p3_dev->ese_secure_check == NOT_CHECKED) {
		ret = tz_tee_ese_secure_check();
		if (ret) {
			p3_dev->ese_secure_check = ESE_NOT_SECURED;
			P3_ERR_MSG("eSE spi is not Secured\n"); 
			return -EBUSY;
		}
		p3_dev->ese_secure_check = ESE_SECURED;
	} else if (p3_dev->ese_secure_check == ESE_NOT_SECURED) {
			P3_ERR_MSG("eSE spi is not Secured\n"); 
			return -EBUSY;
	}
#endif

	/* for defence MULTI-OPEN */
	if (p3_dev->device_opened) {
		P3_ERR_MSG("%s - ALREADY opened!\n", __func__);
		return -EBUSY;
	}

	mutex_lock(&device_list_lock);
	p3_dev->device_opened = true;
	P3_INFO_MSG("open\n");

#ifdef FEATURE_ESE_WAKELOCK
	wake_lock(&p3_dev->ese_lock);
#endif

#ifdef CONFIG_ESE_SECURE
	p3_clk_control(p3_dev, true);
	tz_tee_ese_drv(PM_RESUME);
#else
	p3_pinctrl_config(p3_dev, true);
#endif

#ifdef FEATURE_ESE_POWER_ON_OFF
	ret = p3_regulator_onoff(p3_dev, 1);
	if (ret < 0)
		P3_ERR_MSG(" %s : failed to turn on LDO()\n", __func__);
	usleep_range(2000, 2500);
#endif

	filp->private_data = p3_dev;

	p3_dev->users++;
	mutex_unlock(&device_list_lock);

	return 0;
}

static int spip3_release(struct inode *inode, struct file *filp)
{
	struct p3_data *p3_dev = filp->private_data;
	int ret = 0;

	if (!p3_dev->device_opened) {
		P3_ERR_MSG("%s - was NOT opened....\n", __func__);
		return 0;
	}

	mutex_lock(&device_list_lock);

#ifdef FEATURE_ESE_WAKELOCK
	if (wake_lock_active(&p3_dev->ese_lock))
		wake_unlock(&p3_dev->ese_lock);
#endif

	filp->private_data = p3_dev;

	p3_dev->users--;
	if (!p3_dev->users) {
		p3_dev->device_opened = false;

#ifdef CONFIG_ESE_SECURE
	p3_clk_control(p3_dev, false);
	tz_tee_ese_drv(PM_SUSPEND);
	usleep_range(1000, 1500);
#else
	p3_pinctrl_config(p3_dev, false);
#endif
#ifdef FEATURE_ESE_POWER_ON_OFF
		ret = p3_regulator_onoff(p3_dev, 0);
		if (ret < 0)
			P3_ERR_MSG(" test: failed to turn off LDO()\n");

#endif
	}
	mutex_unlock(&device_list_lock);

	P3_DBG_MSG("%s, users:%d, Major Minor No:%d %d\n", __func__,
			p3_dev->users, imajor(inode), iminor(inode));
	return 0;
}

static long spip3_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	int ret = 0;
	struct p3_data *data = NULL;

	if (_IOC_TYPE(cmd) != P3_MAGIC) {
		P3_ERR_MSG("%s invalid magic. cmd=0x%X Received=0x%X Expected=0x%X\n",
				__func__, cmd, _IOC_TYPE(cmd), P3_MAGIC);
		return -ENOTTY;
	}

	data = filp->private_data;

	mutex_lock(&data->buffer_mutex);
	switch (cmd) {
	case P3_SET_DBG:
		debug_level = (unsigned char)arg;
		P3_DBG_MSG(KERN_INFO"[NXP-P3] -  Debug level %d", debug_level);
		break;
	case P3_ENABLE_SPI_CLK:
		P3_DBG_MSG("%s P3_ENABLE_SPI_CLK\n", __func__);
#ifdef CONFIG_ESE_SECURE
		ret = p3_clk_control(data, true);
		if (ret < 0)
			P3_ERR_MSG("%s: Unable to enable spi clk\n", __func__);
#endif
		break;
	case P3_DISABLE_SPI_CLK:
		P3_DBG_MSG("%s P3_DISABLE_SPI_CLK\n", __func__);
#ifdef CONFIG_ESE_SECURE
		ret = p3_clk_control(data, false);
		if (ret < 0)
			P3_ERR_MSG("%s: couldn't disable spi clks\n", __func__);
#endif
		break;
#ifndef CONFIG_ESE_SECURE
	case P3_RW_SPI_DATA:
		ret = p3_rw_spi_message(data, arg);
		if (ret < 0)
			P3_ERR_MSG("%s P3_RW_SPI_DATA failed [%d].\n",
					__func__, ret);
		break;
#endif
#if 0 //def CONFIG_COMPAT
	case P3_RW_SPI_DATA_32:
		ret = p3_rw_spi_message_32(data, arg);
		if (ret < 0)
			P3_ERR_MSG("%s P3_RW_SPI_DATA_32 failed [%d].\n",
					__func__, ret);
		break;
#endif

	case P3_SET_PWR:
	case P3_SET_POLL:
	case P3_SET_SPI_CLK:
	case P3_ENABLE_SPI_CS:
	case P3_DISABLE_SPI_CS:
	case P3_ENABLE_CLK_CS:
	case P3_DISABLE_CLK_CS:
	case P3_SWING_CS:
		P3_ERR_MSG("%s deprecated IOCTL:0x%X\n", __func__, cmd);
		break;

	default:
		P3_DBG_MSG("%s no matching ioctl! 0x%X\n", __func__, cmd);
		ret = -EINVAL;
	}
	mutex_unlock(&data->buffer_mutex);

	return ret;
}

#ifndef CONFIG_ESE_SECURE
static ssize_t spip3_write(struct file *filp, const char *buf, size_t count,
		loff_t *offset)
{
	int ret = -1;
	struct p3_data *p3_dev;
	struct spi_message m;
	struct spi_transfer t;
	unsigned char tx_buffer[MAX_BUFFER_SIZE] = {0x0, };
	unsigned char rx_buffer[MAX_BUFFER_SIZE] = {0x0, };

	P3_INFO_MSG("spip3_write -Enter count %zu\n", count);

	p3_dev = filp->private_data;

	if (count > MAX_BUFFER_SIZE) {
		P3_ERR_MSG("%s invalid size\n", __func__);
		return -EMSGSIZE;
	}
	mutex_lock(&p3_dev->buffer_mutex);

	if (copy_from_user(&tx_buffer[0], &buf[0], count)) {
		P3_ERR_MSG("%s : failed to copy from user space\n", __func__);
		mutex_unlock(&p3_dev->buffer_mutex);
		return -EFAULT;
	}

	/* Write data */
	spi_message_init(&m);
	memset(&t, 0, sizeof(t));

	t.tx_buf = tx_buffer;
	t.rx_buf = rx_buffer;
	t.len = count;

	spi_message_add_tail(&t, &m);

	gpio_set_value(p3_dev->cs_gpio, 0);
	ret = spi_sync(p3_dev->spi, &m);
	gpio_set_value(p3_dev->cs_gpio, 1);
	if (ret < 0)
		ret = -EIO;
	else
		ret = count;

	mutex_unlock(&p3_dev->buffer_mutex);
	P3_DBG_MSG(KERN_ALERT "spip3_write ret %d- Exit\n", ret);

	return ret;
}

static ssize_t spip3_read(struct file *filp, char *buf, size_t count,
		loff_t *offset)
{
	int ret = -EIO;
	struct spi_message m;
	struct spi_transfer t;
	struct p3_data *p3_dev = filp->private_data;
	unsigned char tx_buffer[MAX_BUFFER_SIZE] = {0x0, };
	unsigned char rx_buffer[MAX_BUFFER_SIZE] = {0x0, };

	if (count > MAX_BUFFER_SIZE) {
		P3_ERR_MSG("%s invalid size\n", __func__);
		return -EMSGSIZE;
	}
	P3_INFO_MSG("spip3_read count %zu - Enter\n", count);
	mutex_lock(&p3_dev->buffer_mutex);

	spi_message_init(&m);
	memset(&t, 0, sizeof(t));

	t.tx_buf = tx_buffer;
	t.rx_buf = rx_buffer;
	t.len = count;

	spi_message_add_tail(&t, &m);

	gpio_set_value(p3_dev->cs_gpio, 0);
	ret = spi_sync(p3_dev->spi, &m);
	gpio_set_value(p3_dev->cs_gpio, 1);

	if (copy_to_user(buf, &rx_buffer[0], count)) {
		P3_ERR_MSG("%s : failed to copy to user space\n", __func__);
		ret = -EFAULT;
		goto fail;
	}
	ret = count;
	P3_DBG_MSG("%s ret %d Exit\n", __func__, ret);

	mutex_unlock(&p3_dev->buffer_mutex);

	return ret;

fail:
	P3_ERR_MSG("Error %s ret %d Exit\n", __func__, ret);
	mutex_unlock(&p3_dev->buffer_mutex);
	return ret;
}
#endif
/* possible fops on the p3 device */
static const struct file_operations spip3_dev_fops = {
	.owner = THIS_MODULE,
#ifndef CONFIG_ESE_SECURE
	.read = spip3_read,
	.write = spip3_write,
#endif
	.open = spip3_open,
	.release = spip3_release,
	.unlocked_ioctl = spip3_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = spip3_ioctl,
#endif
};

static int p3_parse_dt(struct device *dev, struct p3_data *data)
{
	struct device_node *np = dev->of_node;
	int ret = 0;

	data->cs_gpio = of_get_named_gpio(np, "ese_p3,cs-gpio", 0);
	P3_INFO_MSG("cs-gpio : %d\n:", data->cs_gpio);

	if (of_property_read_string(np, "p3-vdd-1p8",
		&data->vdd_1p8) < 0) {
		pr_err("%s - getting vdd_1p8 error\n", __func__);
		data->vdd_1p8 = NULL;
	} else
		pr_info("%s success vdd:%s\n", __func__, data->vdd_1p8);

	return ret;
}

static int spip3_probe(struct spi_device *spi)
{
	int ret = -1;
	struct p3_data *data = NULL;

	P3_INFO_MSG("%s chip select : %d , bus number = %d\n",
		__func__, spi->chip_select, spi->master->bus_num);

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (data == NULL) {
		P3_ERR_MSG("failed to allocate memory for module data\n");
		ret = -ENOMEM;
		goto err_exit;
	}

	ret = p3_parse_dt(&spi->dev, data);
	if (ret) {
		P3_ERR_MSG("%s - Failed to parse DT\n", __func__);
		goto p3_parse_dt_failed;
	}

	ret = p3_regulator_onoff(data, 1);
	if (ret) {
		P3_ERR_MSG("%s - Failed to enable regulator\n", __func__);
		goto p3_parse_dt_failed;
	}

#ifdef CONFIG_ESE_SECURE
	ret = p3_clk_setup(&spi->dev, data);
	if (ret) {
		P3_ERR_MSG("%s - Failed to do clk_setup\n", __func__);
		goto p3_parse_dt_failed;
	}
#else
	spi->bits_per_word = 8;
	spi->mode = SPI_MODE_0;
	spi->max_speed_hz = SPI_DEFAULT_SPEED;
	ret = spi_setup(spi);
	if (ret < 0) {
		P3_ERR_MSG("failed to do spi_setup()\n");
		goto p3_parse_dt_failed;
	}
#endif
	data->speed = SPI_DEFAULT_SPEED;
	data->spi = spi;
	data->p3_device.minor = MISC_DYNAMIC_MINOR;
	data->p3_device.name = "p3";
	data->p3_device.fops = &spip3_dev_fops;
	data->p3_device.parent = &spi->dev;
#ifdef CONFIG_ESE_SECURE
	data->ese_secure_check = NOT_CHECKED;
#endif

	dev_set_drvdata(&spi->dev, data);

	/* init mutex and queues */
	init_waitqueue_head(&data->read_wq);
	mutex_init(&data->buffer_mutex);
#ifdef FEATURE_ESE_WAKELOCK
	wake_lock_init(&data->ese_lock,
		WAKE_LOCK_SUSPEND, "ese_wake_lock");
#endif

	data->device_opened = false;

	ret = misc_register(&data->p3_device);
	if (ret < 0) {
		P3_ERR_MSG("misc_register failed! %d\n", ret);
		goto err_misc_regi;
	}

	ret = gpio_request(data->cs_gpio, "ese_cs");
	if (ret)
		P3_ERR_MSG("failed to get gpio cs-gpio\n");

#ifdef FEATURE_ESE_POWER_ON_OFF
	ret = p3_regulator_onoff(data, 0);
	if (ret < 0) {
		P3_ERR_MSG("%s failed to turn off LDO. [%d]\n", __func__, ret);
		goto err_ldo_off;
	}
#endif
#ifndef CONFIG_ESE_SECURE
	p3_pinctrl_config(data, false);
#endif

	P3_INFO_MSG("%s finished...\n", __func__);
	return ret;

#ifdef FEATURE_ESE_POWER_ON_OFF
err_ldo_off:
	misc_deregister(&data->p3_device);
#endif
err_misc_regi:
#ifdef FEATURE_ESE_WAKELOCK
	wake_lock_destroy(&data->ese_lock);
#endif
	mutex_destroy(&data->buffer_mutex);
p3_parse_dt_failed:
	kfree(data);
err_exit:
	P3_DBG_MSG("ERROR: Exit : %s ret %d\n", __func__, ret);
	return ret;
}

static int spip3_remove(struct spi_device *spi)
{
	struct p3_data *p3_dev = dev_get_drvdata(&spi->dev);

	P3_DBG_MSG("Entry : %s\n", __func__);
	if (p3_dev == NULL) {
		P3_ERR_MSG("%s p3_dev is null!\n", __func__);
		return 0;
	}

#ifdef FEATURE_ESE_WAKELOCK
	wake_lock_destroy(&p3_dev->ese_lock);
#endif
	mutex_destroy(&p3_dev->buffer_mutex);
	misc_deregister(&p3_dev->p3_device);

	kfree(p3_dev);
	P3_DBG_MSG("Exit : %s\n", __func__);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id p3_match_table[] = {
	{ .compatible = "ese_p3",},
	{},
};
#else
#define ese_match_table NULL
#endif

static struct spi_driver spip3_driver = {
	.driver = {
		.name = "p3",
		.bus = &spi_bus_type,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = p3_match_table,
#endif
	},
	.probe =  spip3_probe,
	.remove = spip3_remove,
};

static int __init spip3_dev_init(void)
{
	debug_level = P3_FULL_DEBUG;

	P3_INFO_MSG("Entry : %s\n", __func__);
#if (!defined(CONFIG_ESE_FACTORY_ONLY) || defined(CONFIG_SEC_FACTORY))
	return spi_register_driver(&spip3_driver);
#else
	return -1;
#endif
}

static void __exit spip3_dev_exit(void)
{
	P3_INFO_MSG("Entry : %s\n", __func__);
	spi_unregister_driver(&spip3_driver);
}

module_init(spip3_dev_init);
module_exit(spip3_dev_exit);

MODULE_AUTHOR("Sec");
MODULE_DESCRIPTION("ese SPI driver");
MODULE_LICENSE("GPL");

/** @} */
