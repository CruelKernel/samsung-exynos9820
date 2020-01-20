 /*
 *  sec_battery_misc.c
 *  Samsung Mobile Battery Misc Driver
 *  Author: Yeongmi Ha <yeongmi86.ha@samsung.com>
 *  Copyright (C) 2018 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/poll.h>

#include "include/sec_battery.h"
#include "include/sec_battery_misc.h"

static struct sec_bat_misc_dev *c_dev;

#define SEC_BATT_MISC_DBG 1
#define MAX_BUF 255
#define NODE_OF_MISC "batt_misc"
#define BATT_IOCTL_SWAM _IOWR('B', 0, struct swam_data)

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

static int sec_bat_misc_open(struct inode *inode, struct file *file)
{
	int ret = 0;

	pr_info("%s %s + open success\n",WC_AUTH_MSG, __func__);
	if (!c_dev) {
		pr_err("%s %s - error : c_dev is NULL\n", WC_AUTH_MSG, __func__);
		ret = -ENODEV;
		goto err;
	}

	if (_lock(&c_dev->open_excl)) {
		pr_err("%s %s - error : device busy\n", WC_AUTH_MSG, __func__);
		ret = -EBUSY;
		goto err;
	}

#if 0
	/* check if there is some connection */
	if (!c_dev->swam_ready()) {
		_unlock(&c_dev->open_excl);
		pr_err("%s - error : swam is not ready\n", __func__);
		ret = -EBUSY;
		goto err;
	}
#endif
	pr_info("%s %s- open success\n", WC_AUTH_MSG, __func__);

	return 0;
err:
	return ret;
}

static int sec_bat_misc_close(struct inode *inode, struct file *file)
{
	if (c_dev)
		_unlock(&c_dev->open_excl);
	//c_dev->swam_close();
	pr_info("%s %s - close success\n", WC_AUTH_MSG, __func__);
	return 0;
}

static int send_swam_message(void *data, int size)
{
	int ret;

	ret = c_dev->swam_write(data, size);
	pr_info("%s %s - size : %d, ret : %d\n", WC_AUTH_MSG, __func__, size, ret);
	return ret;
}

static int receive_swam_message(void *data, int size)
{
	int ret;

	ret = c_dev->swam_read(data);
	pr_info("%s %s - size : %d, ret : %d\n", WC_AUTH_MSG, __func__, size, ret);
	return ret;
}

static long
sec_bat_misc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	void *buf = NULL;
#if SEC_BATT_MISC_DBG
	uint8_t *p_buf;
	int i;
#endif

	if (_lock(&c_dev->ioctl_excl)) {
		pr_err("%s %s - error : ioctl busy - cmd : %d\n", WC_AUTH_MSG, __func__, cmd);
		return  -EBUSY;
	}

	switch (cmd) {
	case BATT_IOCTL_SWAM:
		pr_info("%s %s - BATT_IOCTL_SWAM cmd\n", WC_AUTH_MSG, __func__);
		if (copy_from_user(&c_dev->u_data, (void __user *) arg,
				sizeof(struct swam_data))) {
			ret = -EIO;
			pr_err("%s %s - copy_from_user error\n", WC_AUTH_MSG, __func__);
			goto err;
		}
		buf = kzalloc(MAX_BUF, GFP_KERNEL);
		if (!buf)
			return -EINVAL;
		if (c_dev->u_data.size > MAX_BUF) {
			ret = -ENOMEM;
			pr_err("%s %s - user data size is %d error\n", WC_AUTH_MSG, __func__, c_dev->u_data.size);
			goto err;
		}

		if (c_dev->u_data.dir == DIR_OUT) {
			if (copy_from_user(buf, c_dev->u_data.pData, c_dev->u_data.size)) {
				ret = -EIO;
				pr_err("%s %s - copy_from_user error\n", WC_AUTH_MSG, __func__);
				goto err;
			}
#if SEC_BATT_MISC_DBG
			pr_info("%s %s = send_swam_message - size : %d\n", WC_AUTH_MSG, __func__, c_dev->u_data.size);
			p_buf = buf;
			for (i = 0 ; i < c_dev->u_data.size ; i++)
				pr_info("%x ", (uint32_t)p_buf[i]);
			pr_info("\n");
#endif
			ret = send_swam_message(buf, c_dev->u_data.size);
			if (ret < 0) {
				pr_err("%s %s- send_swam_message error\n", WC_AUTH_MSG, __func__);
				ret = -EINVAL;
				goto err;
			}
		} else {
#if SEC_BATT_MISC_DBG
			pr_info("%s %s = received_swam_message - size : %d\n", WC_AUTH_MSG, __func__, c_dev->u_data.size);
#endif
			ret = receive_swam_message(buf, c_dev->u_data.size);
			if (ret < 0) {
				pr_err("%s %s - receive_swam_message error\n", WC_AUTH_MSG, __func__);
				ret = -EINVAL;
				goto err;
			}
#if SEC_BATT_MISC_DBG
			p_buf = buf;
			pr_info("%s %s = received_swam_message - ret : %d\n", WC_AUTH_MSG, __func__, ret);
			for (i = 0; i < ret ; i++)
				pr_info("%x ", (uint32_t)p_buf[i]);
			pr_info("\n");
#endif
			if (copy_to_user((void __user *)c_dev->u_data.pData,
					 buf, ret)) {
				ret = -EIO;
				pr_err("%s %s - copy_to_user error\n", WC_AUTH_MSG, __func__);
				goto err;
			}
		}
		break;

	default:
		pr_err("%s %s - unknown ioctl cmd : %d\n", WC_AUTH_MSG, __func__, cmd);
		ret = -ENOIOCTLCMD;
		goto err;
	}
err:
	kfree(buf);
	_unlock(&c_dev->ioctl_excl);
	return ret;
}

#if 0
u8 htoi(const char *hexa)
{
	char ch = 0;
	u8 deci = 0;
	const char *sp = hexa;
	while (*sp) {
		deci *= 16;
		if ('0' <= *sp && *sp <= '9')
			ch = *sp - '0';
		if ('A' <= *sp && *sp <= 'F')
			ch = *sp - 'A' + 10;
		if ('a' <= *sp && *sp <= 'f')
			ch = *sp - 'a' + 10;
		deci += ch;
		sp++;
	}
	return deci;
}

void itoh(u8 *dest, char *hex, int len)
{
	int i = 0;

	while (i < len) {
		snprintf(hex + i*2, len*2, "%x", dest[i]);
		i++;
	}
}
#endif

#ifdef CONFIG_COMPAT
static long
sec_bat_misc_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	pr_info("%s %s - cmd : %d\n", WC_AUTH_MSG, __func__, cmd);
	ret = sec_bat_misc_ioctl(file, cmd, (unsigned long)compat_ptr(arg));

	return ret;
}
#endif

int sec_bat_swam_out_request_message(void *data, int size)
{
	union power_supply_propval value = {0, };
	//int i = 0;
	u8 *p_data;

	pr_info("%s %s : auth service writes data\n", WC_AUTH_MSG, __func__);

	if (data == NULL) {
		pr_info("%s %s: given data is not valid !\n", WC_AUTH_MSG, __func__);
		return -EINVAL;
	}

	pr_info("%s %s : size = %d \n", WC_AUTH_MSG, __func__, size);

	/* clear received event */
	value.intval = WIRELESS_AUTH_SENT;
	psy_do_property("wireless", set,
		POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS, value);

	p_data = (u8 *)data;
	//for(i=0; i< size; i++)
	//	pr_info("%s: auth read data = %x", __func__, p_data[i]);

	if(size > 1 ) {
		/* set data size first */
		value.intval = size;
		psy_do_property("mfc-charger", set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_SIZE, value);

		value.strval = (u8 *)data;
		/* set data */
		psy_do_property("mfc-charger", set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_DATA, value);
	} else if (size == 1 ) {
		if (p_data[0] == 0x1) {
			pr_info("%s %s : auth has been passed \n", WC_AUTH_MSG, __func__);
			value.intval = WIRELESS_AUTH_PASS;
			psy_do_property("mfc-charger", set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS, value);
		} else if (p_data[0] == 0x2) {
			pr_info("%s %s : auth has been failed \n", WC_AUTH_MSG, __func__);
			value.intval = WIRELESS_AUTH_FAIL;
			psy_do_property("mfc-charger", set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS, value);			
		} else
			pr_info("%s %s : invalid arg %d \n", WC_AUTH_MSG, __func__, p_data[0]);
	}
	return size;
}

void sec_bat_swam_copy_data(u8 *src, u8 *dest, int size)
{
	int i = 0;

	for(i=0; i < size; i++) {
		dest[i] = src[i];
		pr_info("%s %s : auth read data (for debug) = %x", WC_AUTH_MSG, __func__, dest[i]);		
	}
}

int sec_bat_swam_in_request_message(void *data)
{
	union power_supply_propval value = {0, };
	int size = 0;
	//int i = 0;
	//u8 in_data[MAX_BUF] = {0, };

	pr_info("%s %s : auth service reads data\n", WC_AUTH_MSG, __func__);

	if (data == NULL) {
		pr_info("%s %s : given data is not valid !\n", WC_AUTH_MSG, __func__);
		return -EINVAL;
	}
	pr_info("%s %s\n", WC_AUTH_MSG, __func__);

	/* get data size first */
	psy_do_property("mfc-charger", get,
		POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_SIZE, value);
	size = value.intval;
	/* get data */
	psy_do_property("mfc-charger", get,
		POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_DATA, value);

	if(value.intval == 0) {
		pr_info("%s: data hasn't been received yet!\n", __func__);
		return -EINVAL;
	}

	//in_data = (u8 *)value.strval;
	sec_bat_swam_copy_data((u8 *)value.strval, data, size);

	//for(i=0; i< size; i++)
	//	pr_info("%s: auth read data (for debug) = %x", __func__, in_data[i]);

	return size;
}

static const struct file_operations sec_bat_misc_fops = {
	.owner		= THIS_MODULE,
	.open		= sec_bat_misc_open,
	.release	= sec_bat_misc_close,
	.llseek		= no_llseek,
	.unlocked_ioctl = sec_bat_misc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = sec_bat_misc_compat_ioctl,
#endif
};

static struct miscdevice sec_bat_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name	= NODE_OF_MISC,
	.fops	= &sec_bat_misc_fops,
};

int sec_bat_misc_init(struct sec_battery_info *battery)
{
	int ret = 0;

	ret = misc_register(&sec_bat_misc_device);
	if (ret) {
		pr_err("%s %s - return error : %d\n", WC_AUTH_MSG, __func__, ret);
		goto err;
	}

	c_dev = kzalloc(sizeof(struct sec_bat_misc_dev), GFP_KERNEL);
	if (!c_dev) {
		ret = -ENOMEM;
		pr_err("%s %s - kzalloc failed : %d\n", WC_AUTH_MSG, __func__, ret);
		goto err1;
	}
	atomic_set(&c_dev->open_excl, 0);
	atomic_set(&c_dev->ioctl_excl, 0);

	battery->misc_dev = c_dev;

	c_dev->swam_read = sec_bat_swam_in_request_message;
	c_dev->swam_write = sec_bat_swam_out_request_message;
	//c_dev->swam_ready = ;
	//c_dev->swam_close = ;

	pr_info("%s %s - register success\n", WC_AUTH_MSG, __func__);
	return 0;
err1:
	misc_deregister(&sec_bat_misc_device);
err:
	return ret;
}
EXPORT_SYMBOL(sec_bat_misc_init);

void sec_bat_misc_exit(void)
{
	pr_info("%s %s() called\n", WC_AUTH_MSG, __func__);
	if (!c_dev)
		return;
	kfree(c_dev);
	misc_deregister(&sec_bat_misc_device);
}
EXPORT_SYMBOL(sec_bat_misc_exit);
