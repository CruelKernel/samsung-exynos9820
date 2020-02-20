/* drivers/input/touchscreen/sec_ts.c
 *
 * Copyright (C) 2011 Samsung Electronics Co., Ltd.
 * http://www.samsungsemi.com/
 *
 * Core file for Samsung TSC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sec_ts.h"
#include <linux/miscdevice.h>

#define VENDOR_NAME			"SLSI"

#define IOCTL_TYPE_DEVICE		'T'
#define IOCTL_NUMBER_RESET		0
#define IOCTL_NUMBER_ON			1
#define IOCTL_NUMBER_OFF		2
#define IOCTL_NUMBER_OPEN		3
#define IOCTL_NUMBER_RELEASE		4
#define IOCTL_NUMBER_READ		5
#define IOCTL_NUMBER_WRITE		6
#define IOCTL_NUMBER_READ_STATUS	7
#define IOCTL_NUMBER_LPM		8
#define IOCTL_NUMBER_PWR		9
#define IOCTL_NUMBER_INIT		10
#define IOCTL_NUMBER_RESERVED		11

#define IOCTL_TSP_READ		_IOR(IOCTL_TYPE_DEVICE, IOCTL_NUMBER_READ, struct tsp_ioctl)
/*
 * #define IOCTL_TSP_BYTE_READ		_IOR('T', 5, sizeof(struct_ts_ioctl))
 * 0x804C5405 : [10]	[00 0000 0100 1100]	[0101 0100]	[0000 0101]
 *		direction	size		type		number
 *		R		76:0x4C		'T':0x54	5
 */

#define IOCTL_TSP_WRITE		_IOW(IOCTL_TYPE_DEVICE, IOCTL_NUMBER_WRITE, struct tsp_ioctl)
/*
 * #define IOCTL_TSP_BURST_WRITE	_IOW('T', 6, sizeof(struct_ts_ioctl))
 * 0x404C5408 : [01]	[00 0000 0100 1100]	[0101 0100]	[0000 1000]
 *		direction	size		type		number
 *		W		76:0x4C		'T':0x54	6
 */
#define IOCTL_TSP_READ_STATUS	_IOW(IOCTL_TYPE_DEVICE, IOCTL_NUMBER_READ_STATUS, struct tsp_ioctl)
#define IOCTL_TSP_LPM		_IOW(IOCTL_TYPE_DEVICE, IOCTL_NUMBER_LPM, struct tsp_ioctl)
#define IOCTL_TSP_PWR		_IOW(IOCTL_TYPE_DEVICE, IOCTL_NUMBER_PWR, struct tsp_ioctl)
#define IOCTL_TSP_INIT		_IOW(IOCTL_TYPE_DEVICE, IOCTL_NUMBER_INIT, struct tsp_ioctl)
#define IOCTL_TSP_RESET		_IOW(IOCTL_TYPE_DEVICE, IOCTL_NUMBER_RESET, struct tsp_ioctl)

#define VENDOR_LEN_MAX		5
#define REG_ADDR_LEN_MAX	4
#define DATA_LEN_MAX		64
#define RESULT_LEN_MAX		5
struct tsp_ioctl {
	u8 vendor[VENDOR_LEN_MAX];
	u8 reg_addr_len;
	u8 reg_addr[REG_ADDR_LEN_MAX];
	u8 data_len;
	u8 data[DATA_LEN_MAX];
	u8 result[RESULT_LEN_MAX];
};

static long tsp_read(struct sec_ts_data *ts, struct tsp_ioctl *ioctl_data)
{
	u8 *reg_addr = ioctl_data->reg_addr;
	u16 data_len = ioctl_data->data_len;
	u8 *data = ioctl_data->data;

	if ((ioctl_data->reg_addr_len > REG_ADDR_LEN_MAX) || (data_len > DATA_LEN_MAX))
		return -EINVAL;

	memset(data, 0x00, sizeof(ioctl_data->data));

	/* If register address size is larger than 1 byte, use reg_addr_len */
	return ts->sec_ts_i2c_read(ts, reg_addr[0], data, data_len);
}

static long tsp_write(struct sec_ts_data *ts, struct tsp_ioctl *ioctl_data)
{
	u8 *reg_addr = ioctl_data->reg_addr;
	u16 data_len = ioctl_data->data_len;
	u8 *data = ioctl_data->data;

	if ((ioctl_data->reg_addr_len > REG_ADDR_LEN_MAX) || (data_len > DATA_LEN_MAX))
		return -EINVAL;

	/* If register address size is larger than 1 byte, use reg_addr_len */
	return ts->sec_ts_i2c_write(ts, reg_addr[0], data, data_len);
}

static long tsp_ioctl_handler(struct file *file, unsigned int cmd,
				void __user *p, int compat_mode)
{
	struct miscdevice *c = (struct miscdevice *)file->private_data;
	struct device *dev = (struct device *)c->parent;
	struct sec_ts_data *ts = dev_get_drvdata(dev);
	struct tsp_ioctl ioctl_data;
	int ret = 0;
	u8 mode;
	bool on;

	if (_IOC_TYPE(cmd) != IOCTL_TYPE_DEVICE) {
		input_err(true, dev, "%s: invalid device type(%d)\n",
			__func__, _IOC_TYPE(cmd));
		ret = -ENOTTY;
		goto out;
	}

	if (copy_from_user(&ioctl_data, p, sizeof(struct tsp_ioctl))) {
		input_err(true, dev, "%s: failed to copy from user", __func__);
		ret = -EFAULT;
		goto out;
	}

	if (strncmp(ioctl_data.vendor, VENDOR_NAME, sizeof(ioctl_data.vendor))) {
		input_err(true, dev, "%s: invalid vendor\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	switch (cmd) {
	case IOCTL_TSP_READ:
		ret = tsp_read(ts, &ioctl_data);
		if (ret <= 0) {
			input_err(true, dev, "%s: failed to read data(%d)\n",
			__func__, ret);
			goto out;
		}

		if (copy_to_user(p, (void *)&ioctl_data,
				sizeof(struct tsp_ioctl))) {
			input_err(true, dev, "%s: failed to copy to user",
					__func__);
			ret = -EFAULT;
			goto out;
		}
		break;

	case IOCTL_TSP_WRITE:
		ret = tsp_write(ts, &ioctl_data);
		if (ret < 0) {
			input_err(true, dev, "%s: failed to write data(%d)\n",
				__func__, ret);
			goto out;
		}
		break;

	case IOCTL_TSP_READ_STATUS:
		input_info(true, dev, "%s: STATUS\n", __func__);

		ioctl_data.reg_addr[0] = 0x55;
		ioctl_data.reg_addr_len = 1;
		ioctl_data.data_len = 1;

		ret = tsp_read(ts, &ioctl_data);
		if (ret <= 0) {
			input_err(true, dev, "%s: failed to read data(%d)\n",
			__func__, ret);
			goto out;
		}

		if (ioctl_data.data[0] == 0x20)
			snprintf(&ioctl_data.data[0], 3, "OK");

		if (copy_to_user(p, (void *)&ioctl_data,
				sizeof(struct tsp_ioctl))) {
			input_err(true, dev, "%s: failed to copy to user",
					__func__);
			ret = -EFAULT;
			goto out;
		}
		break;

	case IOCTL_TSP_LPM:
		input_info(true, dev, "%s: LPM %d\n", __func__, ioctl_data.data[0]);

		if (ioctl_data.data[0])
			mode = TO_LOWPOWER_MODE;
		else
			mode = TO_TOUCH_MODE;

		ret = sec_ts_set_lowpowermode(ts, mode);
		if (ret < 0)
			snprintf(&ioctl_data.data[0], 5, "FAIL");
		else
			snprintf(&ioctl_data.data[0], 3, "OK");

		if (copy_to_user(p, (void *)&ioctl_data,
				sizeof(struct tsp_ioctl))) {
			input_err(true, dev, "%s: failed to copy to user",
					__func__);
			ret = -EFAULT;
			goto out;
		}
		break;

	case IOCTL_TSP_PWR:
		input_info(true, dev, "%s: PWR %d\n", __func__, ioctl_data.data[0]);

		if (ioctl_data.data[0])
			on = true;
		else
			on = false;

		if (!on)
			disable_irq(ts->client->irq);

		ret = sec_ts_power(ts, on);
		if (ret < 0)
			snprintf(&ioctl_data.data[0], 5, "FAIL");
		else
			snprintf(&ioctl_data.data[0], 3, "OK");

		sec_ts_delay(100);

		if (on)
			enable_irq(ts->client->irq);

		if (copy_to_user(p, (void *)&ioctl_data,
				sizeof(struct tsp_ioctl))) {
			input_err(true, dev, "%s: failed to copy to user",
					__func__);
			ret = -EFAULT;
			goto out;
		}
		break;

	case IOCTL_TSP_INIT:
		input_info(true, dev, "%s: INIT\n", __func__);

		sec_ts_reinit(ts);
		snprintf(&ioctl_data.data[0], 3, "OK");
		if (copy_to_user(p, (void *)&ioctl_data,
				sizeof(struct tsp_ioctl))) {
			input_err(true, dev, "%s: failed to copy to user",
					__func__);
			ret = -EFAULT;
			goto out;
		}
		break;

	case IOCTL_TSP_RESET:
		input_info(true, dev, "%s: RESET\n", __func__);

		sec_ts_stop_device(ts);
		sec_ts_delay(30);
		sec_ts_start_device(ts);

		snprintf(&ioctl_data.data[0], 3, "OK");
		if (copy_to_user(p, (void *)&ioctl_data,
				sizeof(struct tsp_ioctl))) {
			input_err(true, dev, "%s: failed to copy to user",
					__func__);
			ret = -EFAULT;
			goto out;
		}
		break;
	default:
		input_err(true, dev, "%s: invalid cmd(0x%X)\n", __func__, cmd);
		ret = -EINVAL;
		goto out;
	}

out:
	return ret;
}

static long tsp_ioctl(struct file *file,
			unsigned int cmd, unsigned long arg)
{
	return tsp_ioctl_handler(file, cmd, (void __user *)arg, 0);
}

#ifdef CONFIG_COMPAT
static long tsp_ioctl_compat(struct file *file,
				unsigned int cmd, unsigned long arg)
{
	struct miscdevice *c = (struct miscdevice *)file->private_data;
	struct device *dev = (struct device *)c->parent;

	input_err(true, dev, "%s\n", __func__);

	return -EPERM;
}
#endif

int tsp_open(struct inode *inode, struct file *file)
{
	struct miscdevice *c = (struct miscdevice *)file->private_data;
	struct device *dev = (struct device *)c->parent;

	input_info(true, dev, "%s\n", __func__);

	return 0;
}

int tsp_close(struct inode *inode, struct file *file)
{
	struct miscdevice *c = (struct miscdevice *)file->private_data;
	struct device *dev = (struct device *)c->parent;

	input_info(true, dev, "%s\n", __func__);

	return 0;
}

static const struct file_operations tsp_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= tsp_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= tsp_ioctl_compat,
#endif
	.open		= tsp_open,
	.release	= tsp_close,
};

static struct miscdevice tsp_misc = {
	.fops	= &tsp_fops,
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "tspio",
};
MODULE_ALIAS_MISCDEV(MISC_DYNAMIC_MINOR);

void sec_ts_ioctl_init(struct sec_ts_data *ts)
{
	tsp_misc.parent = &ts->client->dev;
	misc_register(&tsp_misc);
}

void sec_ts_ioctl_remove(struct sec_ts_data *ts)
{
	misc_deregister(&tsp_misc);
}
