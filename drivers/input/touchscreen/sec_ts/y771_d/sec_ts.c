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

struct sec_ts_data *tsp_info;

#include "sec_ts.h"

struct sec_ts_data *ts_dup;

#ifdef USE_POWER_RESET_WORK
static void sec_ts_reset_work(struct work_struct *work);
#endif
static void sec_ts_read_info_work(struct work_struct *work);
static void sec_ts_print_info_work(struct work_struct *work);

#ifdef USE_OPEN_CLOSE
static int sec_ts_input_open(struct input_dev *dev);
static void sec_ts_input_close(struct input_dev *dev);
#endif

int sec_ts_read_information(struct sec_ts_data *ts);

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
static irqreturn_t sec_ts_irq_thread(int irq, void *ptr);

static irqreturn_t secure_filter_interrupt(struct sec_ts_data *ts)
{
	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_ENABLE) {
		if (atomic_cmpxchg(&ts->secure_pending_irqs, 0, 1) == 0) {
			sysfs_notify(&ts->input_dev->dev.kobj, NULL, "secure_touch");

#if defined(CONFIG_TRUSTONIC_TRUSTED_UI_QC)
			complete(&ts->st_irq_received);
#endif
		} else {
			input_info(true, &ts->client->dev, "%s: pending irq:%d\n",
					__func__, (int)atomic_read(&ts->secure_pending_irqs));
		}

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

/**
 * Sysfs attr group for secure touch & interrupt handler for Secure world.
 * @atomic : syncronization for secure_enabled
 * @pm_runtime : set rpm_resume or rpm_ilde
 */
static ssize_t secure_touch_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d", atomic_read(&ts->secure_enabled));
}

static ssize_t secure_touch_enable_store(struct device *dev,
		struct device_attribute *addr, const char *buf, size_t count)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);
	int ret;
	unsigned long data;
	struct input_dev *input_device = NULL;

	if (count > 2) {
		input_err(true, &ts->client->dev,
				"%s: cmd length is over (%s,%d)!!\n",
				__func__, buf, (int)strlen(buf));
		return -EINVAL;
	}

	ret = kstrtoul(buf, 10, &data);
	if (ret != 0) {
		input_err(true, &ts->client->dev, "%s: failed to read:%d\n",
				__func__, ret);
		return -EINVAL;
	}

	if (data == 1) {
		if (ts->reset_is_on_going) {
			input_err(true, &ts->client->dev, "%s: reset is on going because i2c fail\n", __func__);
			return -EBUSY;
		}

		/* Enable Secure World */
		if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_ENABLE) {
			input_err(true, &ts->client->dev, "%s: already enabled\n", __func__);
			return -EBUSY;
		}

		/* syncronize_irq -> disable_irq + enable_irq
		 * concern about timing issue.
		 */
		disable_irq(ts->client->irq);

		/* Fix normal active mode : idle mode is failed to i2c for 1 time */
		ret = sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);
		if (ret < 0) {
			enable_irq(ts->client->irq);
			input_err(true, &ts->client->dev, "%s: failed to fix tmode\n",
					__func__);
			return -EIO;
		}

		/* Release All Finger */
		sec_ts_unlocked_release_all_finger(ts);

		if (pm_runtime_get_sync(ts->client->adapter->dev.parent) < 0) {
			enable_irq(ts->client->irq);
			input_err(true, &ts->client->dev, "%s: failed to get pm_runtime\n", __func__);
			return -EIO;
		}

		list_for_each_entry(input_device, &ts->input_dev->node, node) {
			if (strncmp(input_device->name, "sec_e-pen", 9) == 0) {
				if (input_device->close)
					input_device->close(input_device);
				break;
			}
		}

		reinit_completion(&ts->secure_powerdown);
		reinit_completion(&ts->secure_interrupt);
#if defined(CONFIG_TRUSTONIC_TRUSTED_UI_QC)
		reinit_completion(&ts->st_irq_received);
#endif
		atomic_set(&ts->secure_enabled, 1);
		atomic_set(&ts->secure_pending_irqs, 0);

		enable_irq(ts->client->irq);

		input_info(true, &ts->client->dev, "%s: secure touch enable\n", __func__);
	} else if (data == 0) {
		/* Disable Secure World */
		if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_DISABLE) {
			input_err(true, &ts->client->dev, "%s: already disabled\n", __func__);
			return count;
		}

		pm_runtime_put_sync(ts->client->adapter->dev.parent);
		atomic_set(&ts->secure_enabled, 0);

		sysfs_notify(&ts->input_dev->dev.kobj, NULL, "secure_touch");

		sec_ts_delay(10);

		sec_ts_irq_thread(ts->client->irq, ts);
		complete(&ts->secure_interrupt);
		complete(&ts->secure_powerdown);
#if defined(CONFIG_TRUSTONIC_TRUSTED_UI_QC)
		complete(&ts->st_irq_received);
#endif

		input_info(true, &ts->client->dev, "%s: secure touch disable\n", __func__);

		list_for_each_entry(input_device, &ts->input_dev->node, node) {
			if (strncmp(input_device->name, "sec_e-pen", 9) == 0) {
				if (input_device->open)
					input_device->open(input_device);
				break;
			}
		}

		ret = sec_ts_release_tmode(ts);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: failed to release tmode\n",
					__func__);
			return -EIO;
		}

	} else {
		input_err(true, &ts->client->dev, "%s: unsupport value:%d\n", __func__, data);
		return -EINVAL;
	}

	return count;
}

#if defined(CONFIG_TRUSTONIC_TRUSTED_UI_QC)
static int secure_get_irq(struct device *dev)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);
	int val = 0;

	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_DISABLE) {
		input_err(true, &ts->client->dev, "%s: disabled\n", __func__);
		return -EBADF;
	}

	if (atomic_cmpxchg(&ts->secure_pending_irqs, -1, 0) == -1) {
		input_err(true, &ts->client->dev, "%s: pending irq -1\n", __func__);
		return -EINVAL;
	}

	if (atomic_cmpxchg(&ts->secure_pending_irqs, 1, 0) == 1)
		val = 1;

	input_err(true, &ts->client->dev, "%s: pending irq is %d\n",
			__func__, atomic_read(&ts->secure_pending_irqs));

	complete(&ts->secure_interrupt);

	return val;
}
#endif

static ssize_t secure_touch_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);
	int val = 0;

	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_DISABLE) {
		input_err(true, &ts->client->dev, "%s: disabled\n", __func__);
		return -EBADF;
	}

	if (atomic_cmpxchg(&ts->secure_pending_irqs, -1, 0) == -1) {
		input_err(true, &ts->client->dev, "%s: pending irq -1\n", __func__);
		return -EINVAL;
	}

	if (atomic_cmpxchg(&ts->secure_pending_irqs, 1, 0) == 1) {
		val = 1;
		input_err(true, &ts->client->dev, "%s: pending irq is %d\n",
				__func__, atomic_read(&ts->secure_pending_irqs));
	}

	complete(&ts->secure_interrupt);

	return snprintf(buf, PAGE_SIZE, "%u", val);
}

static ssize_t secure_ownership_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "1");
}

static DEVICE_ATTR(secure_touch_enable, (S_IRUGO | S_IWUSR | S_IWGRP),
		secure_touch_enable_show, secure_touch_enable_store);
static DEVICE_ATTR(secure_touch, S_IRUGO, secure_touch_show, NULL);

static DEVICE_ATTR(secure_ownership, S_IRUGO, secure_ownership_show, NULL);

static struct attribute *secure_attr[] = {
	&dev_attr_secure_touch_enable.attr,
	&dev_attr_secure_touch.attr,
	&dev_attr_secure_ownership.attr,
	NULL,
};

static struct attribute_group secure_attr_group = {
	.attrs = secure_attr,
};


static int secure_touch_init(struct sec_ts_data *ts)
{
	input_info(true, &ts->client->dev, "%s\n", __func__);

	init_completion(&ts->secure_interrupt);
	init_completion(&ts->secure_powerdown);
#if defined(CONFIG_TRUSTONIC_TRUSTED_UI_QC)
	init_completion(&ts->st_irq_received);
	register_tui_hal_ts(&ts->input_dev->dev, &ts->secure_enabled,
			&ts->st_irq_received, secure_get_irq,
			secure_touch_enable_store);
#endif

	return 0;
}

static void secure_touch_stop(struct sec_ts_data *ts, bool stop)
{
	if (atomic_read(&ts->secure_enabled)) {
		atomic_set(&ts->secure_pending_irqs, -1);

		sysfs_notify(&ts->input_dev->dev.kobj, NULL, "secure_touch");

#if defined(CONFIG_TRUSTONIC_TRUSTED_UI_QC)
		complete(&ts->st_irq_received);
#endif

		if (stop)
			wait_for_completion_interruptible(&ts->secure_powerdown);

		input_info(true, &ts->client->dev, "%s: %d\n", __func__, stop);
	}
}
#endif

int sec_ts_i2c_write(struct sec_ts_data *ts, u8 reg, u8 *data, int len)
{
	u8 *buf;
	int ret;
	unsigned char retry;
	struct i2c_msg msg;
	int i;
	const int len_max = 0xffff;

	if (len + 1 > len_max) {
		input_err(true, &ts->client->dev,
				"%s: The i2c buffer size is exceeded.\n", __func__);
		return -ENOMEM;
	}

	if (!ts->resume_done.done) {
		ret = wait_for_completion_interruptible_timeout(&ts->resume_done, msecs_to_jiffies(500));
		if (ret <= 0) {
			input_err(true, &ts->client->dev, "%s: LPM: pm resume is not handled:%d\n", __func__, ret);
			return -EIO;
		}
	}

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_ENABLE) {
		input_err(true, &ts->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EBUSY;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif
	buf = kzalloc(len + 1, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF\n", __func__);
		goto err;
	}

	buf[0] = reg;
	memcpy(buf + 1, data, len);

	msg.addr = ts->client->addr;
	msg.flags = 0;
	msg.len = len + 1;
	msg.buf = buf;

	mutex_lock(&ts->i2c_mutex);
	for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
		ret = i2c_transfer(ts->client->adapter, &msg, 1);
		if (ret == 1)
			break;

		if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
			input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
			mutex_unlock(&ts->i2c_mutex);
			goto err;
		}

		usleep_range(1 * 1000, 1 * 1000);

		if (retry > 1) {
			input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n", __func__, retry + 1, ret);
			ts->comm_err_count++;
			if (ts->debug_flag & SEC_TS_DEBUG_SEND_UEVENT) {
				char result[32];

				snprintf(result, sizeof(result), "RESULT=I2C");
				sec_cmd_send_event_to_user(&ts->sec, NULL, result);
			}
		}
	}

	mutex_unlock(&ts->i2c_mutex);

	if (retry == SEC_TS_I2C_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: I2C write over retry limit\n", __func__);
		ret = -EIO;
#ifdef USE_POR_AFTER_I2C_RETRY
		if (ts->probe_done && !ts->reset_is_on_going)
			schedule_delayed_work(&ts->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
#endif
	}

	if (ts->debug_flag & SEC_TS_DEBUG_PRINT_I2C_WRITE_CMD) {
		pr_info("sec_input:i2c_cmd: W: %02X | ", reg);
		for (i = 0; i < len; i++)
			pr_cont("%02X ", data[i]);
		pr_cont("\n");
	}

	if (ret == 1) {
		kfree(buf);
		return 0;
	}
err:
	kfree(buf);
	return -EIO;
}

int sec_ts_i2c_read(struct sec_ts_data *ts, u8 reg, u8 *data, int len)
{
	u8 *buf;
	int ret;
	unsigned char retry;
	struct i2c_msg msg[2];
	int remain = len;
	int i;
	u8 *buff;

	if (!ts->resume_done.done) {
		ret = wait_for_completion_interruptible_timeout(&ts->resume_done, msecs_to_jiffies(500));
		if (ret <= 0) {
			input_err(true, &ts->client->dev, "%s: LPM: pm resume is not handled:%d\n", __func__, ret);
			return -EIO;
		}
	}

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_ENABLE) {
		input_err(true, &ts->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled!\n", __func__);
		return -EBUSY;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif
	buf = kzalloc(1, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	buff = kzalloc(len, GFP_KERNEL);
	if (!buff) {
		kfree(buf);
		return -ENOMEM;
	}

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF\n", __func__);
		goto err;
	}

	buf[0] = reg;

	msg[0].addr = ts->client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;

	msg[1].addr = ts->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = buff;

	mutex_lock(&ts->i2c_mutex);
	if (len <= ts->i2c_burstmax) {
		msg[1].len = len;
		for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
			ret = i2c_transfer(ts->client->adapter, msg, 2);
			if (ret == 2)
				break;
			usleep_range(1 * 1000, 1 * 1000);
			if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
				input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
				mutex_unlock(&ts->i2c_mutex);
				goto err;
			}

			if (retry > 1) {
				input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n",
					__func__, retry + 1, ret);
				ts->comm_err_count++;
				if (ts->debug_flag & SEC_TS_DEBUG_SEND_UEVENT) {
					char result[32];

					snprintf(result, sizeof(result), "RESULT=I2C");
					sec_cmd_send_event_to_user(&ts->sec, NULL, result);
				}
			}
		}
	} else {
		/*
		 * I2C read buffer is 256 byte. do not support long buffer over than 256.
		 * So, try to seperate reading data about 256 bytes.
		 */
		for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
			ret = i2c_transfer(ts->client->adapter, msg, 1);
			if (ret == 1)
				break;
			usleep_range(1 * 1000, 1 * 1000);
			if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
				input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
				mutex_unlock(&ts->i2c_mutex);
				goto err;
			}

			if (retry > 1) {
				input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n",
					__func__, retry + 1, ret);
				ts->comm_err_count++;
			}
		}

		do {
			if (remain > ts->i2c_burstmax)
				msg[1].len = ts->i2c_burstmax;
			else
				msg[1].len = remain;

			remain -= ts->i2c_burstmax;

			for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
				ret = i2c_transfer(ts->client->adapter, &msg[1], 1);
				if (ret == 1)
					break;
				usleep_range(1 * 1000, 1 * 1000);
				if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
					input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
					mutex_unlock(&ts->i2c_mutex);
					goto err;
				}

				if (retry > 1) {
					input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n",
						__func__, retry + 1, ret);
					ts->comm_err_count++;
				}
			}
			msg[1].buf += msg[1].len;
		} while (remain > 0);
	}

	mutex_unlock(&ts->i2c_mutex);

	if (retry == SEC_TS_I2C_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: I2C read over retry limit\n", __func__);
		ret = -EIO;
#ifdef USE_POR_AFTER_I2C_RETRY
		if (ts->probe_done && !ts->reset_is_on_going)
			schedule_delayed_work(&ts->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
#endif
	}

	memcpy(data, buff, len);
	if (ts->debug_flag & SEC_TS_DEBUG_PRINT_I2C_READ_CMD) {
		pr_info("sec_input:i2c_cmd: R: %02X | ", reg);
		for (i = 0; i < len; i++)
			pr_cont("%02X ", data[i]);
		pr_cont("\n");
	}

	kfree(buf);
	kfree(buff);
	return ret;
err:
	kfree(buf);
	kfree(buff);
	return -EIO;
}

static int sec_ts_i2c_write_burst(struct sec_ts_data *ts, u8 *data, int len)
{
	int ret;
	int retry;
	u8 *buf;
	const int len_max = 0xffff;

	if (len > len_max) {
		input_err(true, &ts->client->dev,
				"%s: The i2c buffer size is exceeded.\n", __func__);
		return -ENOMEM;
	}

	if (!ts->resume_done.done) {
		ret = wait_for_completion_interruptible_timeout(&ts->resume_done, msecs_to_jiffies(500));
		if (ret <= 0) {
			input_err(true, &ts->client->dev, "%s: LPM: pm resume is not handled:%d\n", __func__, ret);
			return -EIO;
		}
	}

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_ENABLE) {
		input_err(true, &ts->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled\n", __func__);
		return -EBUSY;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif
	buf = kzalloc(len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	memcpy(buf, data, len);
	mutex_lock(&ts->i2c_mutex);

	for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
		ret = i2c_master_send(ts->client, buf , len);
		if (ret == len)
			break;

		usleep_range(1 * 1000, 1 * 1000);

		if (retry > 1) {
			input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n", __func__, retry + 1, ret);
			ts->comm_err_count++;
		}
	}

	mutex_unlock(&ts->i2c_mutex);
	if (retry == SEC_TS_I2C_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: I2C write over retry limit\n", __func__);
		ret = -EIO;
	}

	kfree(buf);

	return ret;
}

static int sec_ts_i2c_read_bulk(struct sec_ts_data *ts, u8 *data, int len)
{
	int ret;
	unsigned char retry;
	int remain = len;
	struct i2c_msg msg;
	u8 *buff;

	if (!ts->resume_done.done) {
		ret = wait_for_completion_interruptible_timeout(&ts->resume_done, msecs_to_jiffies(500));
		if (ret <= 0) {
			input_err(true, &ts->client->dev, "%s: LPM: pm resume is not handled:%d\n", __func__, ret);
			return -EIO;
		}
	}

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_ENABLE) {
		input_err(true, &ts->client->dev,
				"%s: TSP no accessible from Linux, TUI is enabled\n", __func__);
		return -EBUSY;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return -EBUSY;
#endif
	buff = kzalloc(len, GFP_KERNEL);
	if (!buff)
		return -ENOMEM;

	msg.addr = ts->client->addr;
	msg.flags = I2C_M_RD;
	msg.buf = buff;

	mutex_lock(&ts->i2c_mutex);

	do {
		if (remain > ts->i2c_burstmax)
			msg.len = ts->i2c_burstmax;
		else
			msg.len = remain;

		remain -= ts->i2c_burstmax;

		for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
			ret = i2c_transfer(ts->client->adapter, &msg, 1);
			if (ret == 1)
				break;
			usleep_range(1 * 1000, 1 * 1000);

			if (retry > 1) {
				input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n",
					__func__, retry + 1, ret);
				ts->comm_err_count++;
			}
		}

		if (retry == SEC_TS_I2C_RETRY_CNT) {
			input_err(true, &ts->client->dev, "%s: I2C read over retry limit\n", __func__);
			ret = -EIO;

			break;
		}
		msg.buf += msg.len;
	} while (remain > 0);

	mutex_unlock(&ts->i2c_mutex);

	if (ret == 1) {
		memcpy(data, buff, len);
		kfree(buff);
		return 0;
	}

	kfree(buff);
	return -EIO;
}

static int sec_ts_read_from_sponge(struct sec_ts_data *ts, u8 *data, int len)
{
	int ret;

	if (!ts->use_sponge)
		return -ENXIO;

	mutex_lock(&ts->sponge_mutex);
	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SPONGE_READ_PARAM, data, 2);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: fail to read sponge command\n", __func__);

	ret = sec_ts_i2c_read(ts, SEC_TS_CMD_SPONGE_READ_PARAM, (u8 *)data, len);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: fail to read sponge command\n", __func__);
	mutex_unlock(&ts->sponge_mutex);

	return ret;
}

static int sec_ts_write_to_sponge(struct sec_ts_data *ts, u8 *data, int len)
{
	int ret;

	if (!ts->use_sponge)
		return -ENXIO;

	mutex_lock(&ts->sponge_mutex);
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SPONGE_WRITE_PARAM, data, len);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to write offset\n", __func__);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SPONGE_NOTIFY_PACKET, NULL, 0);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to send notify\n", __func__);
	mutex_unlock(&ts->sponge_mutex);

	return ret;
}

static void sec_ts_set_utc_sponge(struct sec_ts_data *ts)
{
	struct timeval current_time;
	int ret;
	u8 data[6] = {SEC_TS_CMD_SPONGE_OFFSET_UTC, 0};

	if (!ts->use_sponge)
		return;

	do_gettimeofday(&current_time);
	data[2] = (0xFF & (u8)((current_time.tv_sec) >> 0));
	data[3] = (0xFF & (u8)((current_time.tv_sec) >> 8));
	data[4] = (0xFF & (u8)((current_time.tv_sec) >> 16));
	data[5] = (0xFF & (u8)((current_time.tv_sec) >> 24));
	input_info(true, &ts->client->dev, "Write UTC to Sponge = %X\n", (int)(current_time.tv_sec));

	ret = ts->sec_ts_write_sponge(ts, data, 6);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to write sponge\n", __func__);
}

static void sec_ts_set_prox_power_off(struct sec_ts_data *ts, u8 data)
{
	int ret;

	input_info(true, &ts->client->dev, "Write prox set = %d\n", data);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_PROX_POWER_OFF, &data, 1);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to write prox_power_off set\n", __func__);
}

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
#include <linux/sec_debug.h>
extern struct tsp_dump_callbacks dump_callbacks;
static struct delayed_work *p_ghost_check;

static void sec_ts_check_rawdata(struct work_struct *work)
{
	struct sec_ts_data *ts = container_of(work, struct sec_ts_data, ghost_check.work);

	if (ts->tsp_dump_lock == 1) {
		input_err(true, &ts->client->dev, "%s: ignored ## already checking..\n", __func__);
		return;
	}
	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: ignored ## IC is power off\n", __func__);
		return;
	}

	sec_ts_run_rawdata_all(ts, true);
}

static void dump_tsp_log(void)
{
	pr_info("%s: %s %s: start\n", SEC_TS_I2C_NAME, SECLOG, __func__);

#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge == 1) {
		pr_err("%s: %s %s: ignored ## lpm charging Mode!!\n", SEC_TS_I2C_NAME, SECLOG, __func__);
		return;
	}
#endif
	if (p_ghost_check == NULL) {
		pr_err("%s: %s %s: ignored ## tsp probe fail!!\n", SEC_TS_I2C_NAME, SECLOG, __func__);
		return;
	}
	schedule_delayed_work(p_ghost_check, msecs_to_jiffies(100));
}
#endif


void sec_ts_delay(unsigned int ms)
{
	if (ms < 20)
		usleep_range(ms * 1000, ms * 1000);
	else
		msleep(ms);
}

int sec_ts_set_touch_function(struct sec_ts_data *ts)
{
	int ret = 0;

	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TOUCHFUNCTION, (u8 *)&(ts->touch_functions), 2);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to send command(0x%x)",
				__func__, SEC_TS_CMD_SET_TOUCHFUNCTION);

	schedule_delayed_work(&ts->work_read_functions, msecs_to_jiffies(30));

	return ret;
}

void sec_ts_get_touch_function(struct work_struct *work)
{
	struct sec_ts_data *ts = container_of(work, struct sec_ts_data,
			work_read_functions.work);
	int ret = 0;

	ret = sec_ts_i2c_read(ts, SEC_TS_CMD_SET_TOUCHFUNCTION, (u8 *)&(ts->ic_status), 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read touch functions(%d)\n",
				__func__, ret);
		return;
	}

	input_info(true, &ts->client->dev,
			"%s: touch_functions:%x ic_status:%x\n", __func__,
			ts->touch_functions, ts->ic_status);
}

int sec_ts_wait_for_ready(struct sec_ts_data *ts, unsigned int ack)
{
	int rc = -1;
	int retry = 0;
	u8 tBuff[SEC_TS_EVENT_BUFF_SIZE] = {0,};

	while (retry <= SEC_TS_WAIT_RETRY_CNT) {
		if (gpio_get_value(ts->plat_data->irq_gpio) == 0) {
			if (sec_ts_i2c_read(ts, SEC_TS_READ_ONE_EVENT, tBuff, SEC_TS_EVENT_BUFF_SIZE) > 0) {
				if (((tBuff[0] >> 2) & 0xF) == TYPE_STATUS_EVENT_INFO) {
					if (tBuff[1] == ack) {
						rc = 0;
						break;
					}
				} else if (((tBuff[0] >> 2) & 0xF) == TYPE_STATUS_EVENT_VENDOR_INFO) {
					if (tBuff[1] == ack) {
						rc = 0;
						break;
					}
				}
			}
		}
		sec_ts_delay(20);
		retry++;
	}

	if (retry > SEC_TS_WAIT_RETRY_CNT)
		input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);

	input_info(true, &ts->client->dev,
			"%s: %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X [%d]\n",
			__func__, tBuff[0], tBuff[1], tBuff[2], tBuff[3],
			tBuff[4], tBuff[5], tBuff[6], tBuff[7], retry);

	return rc;
}

int sec_ts_read_calibration_report(struct sec_ts_data *ts)
{
	int ret;
	u8 buf[5] = { 0 };

	buf[0] = SEC_TS_READ_CALIBRATION_REPORT;

	ret = sec_ts_i2c_read(ts, buf[0], &buf[1], 4);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to read, %d\n", __func__, ret);
		return ret;
	}

	input_info(true, &ts->client->dev, "%s: count:%d, pass count:%d, fail count:%d, status:0x%X\n",
			__func__, buf[1], buf[2], buf[3], buf[4]);

	return buf[4];
}

void sec_ts_reinit(struct sec_ts_data *ts)
{
	u8 w_data[2] = {0x00, 0x00};
	int ret = 0;

	input_info(true, &ts->client->dev,
		"%s: charger=0x%x, touch_functions=0x%x, Power mode=0x%x\n",
		__func__, ts->charger_mode, ts->touch_functions, ts->power_status);

	ts->touch_noise_status = 0;
	ts->touch_pre_noise_status = 0;
	ts->wet_mode = 0;

	/* charger mode */
	if (ts->charger_mode != SEC_TS_BIT_CHARGER_MODE_NO) {
		w_data[0] = ts->charger_mode;
		ret = ts->sec_ts_i2c_write(ts, SET_TS_CMD_SET_CHARGER_MODE, &w_data[0], 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: Failed to send command(0x%x)",
					__func__, SET_TS_CMD_SET_CHARGER_MODE);
			return;
		}
	}

	/* Cover mode */
	if (ts->touch_functions & SEC_TS_BIT_SETFUNC_COVER) {
		w_data[0] = ts->cover_cmd;
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_COVERTYPE, &w_data[0], 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: Failed to send command(0x%x)",
					__func__, SEC_TS_CMD_SET_COVERTYPE);
			return;
		}

	}

	ret = sec_ts_set_touch_function(ts);
	if (ret < 0) { 
		input_err(true, &ts->client->dev, "%s: Failed to send command(0x%x)",
				__func__, SEC_TS_CMD_SET_TOUCHFUNCTION);
		return;
	}

	sec_ts_set_custom_library(ts);
	sec_ts_set_press_property(ts);

	if (ts->plat_data->support_fod && ts->use_sponge && ts->fod_set_val) {
		sec_ts_set_fod_rect(ts);
	}

	/* Power mode */
	if (ts->power_status == SEC_TS_STATE_LPM) {
		w_data[0] = TO_LOWPOWER_MODE;
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &w_data[0], 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: Failed to send command(0x%x)",
					__func__, SEC_TS_CMD_SET_POWER_MODE);
			return;
		}

		sec_ts_delay(50);
		sec_ts_set_aod_rect(ts);
	} else {
		sec_ts_set_grip_type(ts, ONLY_EDGE_HANDLER);

		sec_ts_set_external_noise_mode(ts, EXT_NOISE_MODE_MAX);

		if (ts->brush_mode) {
			input_info(true, &ts->client->dev, "%s: set brush mode\n", __func__);
			ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_BRUSH_MODE, &ts->brush_mode, 1);
			if (ret < 0) {
				input_err(true, &ts->client->dev,
						"%s: failed to set brush mode\n", __func__);
				return;
			}
		}

		if (ts->touchable_area) {
			input_info(true, &ts->client->dev, "%s: set 16:9 mode\n", __func__);
			ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TOUCHABLE_AREA, &ts->touchable_area, 1);
			if (ret < 0) {
				input_err(true, &ts->client->dev,
						"%s: failed to set 16:9 mode\n", __func__);
				return;
			}
		}
	}

	if (ts->ed_enable) {
		input_info(true, &ts->client->dev, "%s: set ear detect mode\n", __func__);
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_SET_EAR_DETECT_MODE, &ts->ed_enable, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev,
					"%s: failed to set ear detect mode\n", __func__);
			return;
		}
	}
}

void sec_ts_print_info(struct sec_ts_data *ts)
{
	if (!ts)
		return;

	if (!ts->client)
		return;

	ts->print_info_cnt_open++;

	if (ts->print_info_cnt_open > 0xfff0)
		ts->print_info_cnt_open = 0;

	if (ts->touch_count == 0)
		ts->print_info_cnt_release++;

	input_info(true, &ts->client->dev,
			"mode:%04X tc:%d noise:%d%d ext_n:%d wet:%d wc:%x(%d) lp:(%x) fod:%d D%05X fn:%04X/%04X ED:%d // v:%02X%02X cal:%02X(%02X) C%02XT%04X.%4s%s Cal_flag:%s fail_cnt:%d // sp:%d sip:%d id(%d,%d) tmp(%d)// #%d %d\n",
			ts->print_info_currnet_mode, ts->touch_count,
			ts->touch_noise_status, ts->touch_pre_noise_status,
			ts->external_noise_mode, ts->wet_mode,
			ts->charger_mode, ts->force_charger_mode,
			ts->lowpower_mode, ts->fod_set_val, ts->defect_probability,
			ts->touch_functions, ts->ic_status, ts->ed_enable,
			ts->plat_data->img_version_of_ic[2], ts->plat_data->img_version_of_ic[3],
			ts->cal_status, ts->nv,
#ifdef TCLM_CONCEPT
			ts->tdata->nvdata.cal_count, ts->tdata->nvdata.tune_fix_ver,
			ts->tdata->tclm_string[ts->tdata->nvdata.cal_position].f_name,
			(ts->tdata->tclm_level == TCLM_LEVEL_LOCKDOWN) ? ".L" : " ",
			(ts->tdata->nvdata.cal_fail_falg == SEC_CAL_PASS) ? "Success" : "Fail",
			ts->tdata->nvdata.cal_fail_cnt,
#else
			0,0," "," "," ",0,
#endif
			ts->spen_mode_val, ts->sip_mode, ts->tspid_val, ts->tspicid_val,
			ts->tsp_temp_data,
			ts->print_info_cnt_open, ts->print_info_cnt_release);
}

/************************************************************
*  720  * 1480 : <48 96 60> indicator: 24dp navigator:48dp edge:60px dpi=320
* 1080  * 2220 :  4096 * 4096 : <133 266 341>  (approximately value)
************************************************************/
void location_detect(struct sec_ts_data *ts, char *loc, int x, int y)
{
	int i;

	for (i = 0; i < SEC_TS_LOCATION_DETECT_SIZE; ++i)
		loc[i] = 0;

	if (x < ts->plat_data->area_edge)
		strcat(loc, "E.");
	else if (x < (ts->plat_data->max_x - ts->plat_data->area_edge))
		strcat(loc, "C.");
	else
		strcat(loc, "e.");

	if (y < ts->plat_data->area_indicator)
		strcat(loc, "S");
	else if (y < (ts->plat_data->max_y - ts->plat_data->area_navigation))
		strcat(loc, "C");
	else
		strcat(loc, "N");
}

#define MAX_EVENT_COUNT 31
static void sec_ts_read_event(struct sec_ts_data *ts)
{
	int ret;
	u8 t_id;
	u8 event_id;
	u8 left_event_count;
	u8 read_event_buff[MAX_EVENT_COUNT][SEC_TS_EVENT_BUFF_SIZE] = { { 0 } };
	u8 *event_buff;
	struct sec_ts_event_coordinate *p_event_coord;
	struct sec_ts_gesture_status *p_gesture_status;
	struct sec_ts_event_status *p_event_status;
	int curr_pos;
	int remain_event_count = 0;
	int pre_ttype = 0;
	int pre_action = 0;
	static bool error_report;
	u8 force_strength = 1;
	char location[SEC_TS_LOCATION_DETECT_SIZE] = { 0, };

	if (ts->power_status == SEC_TS_STATE_LPM) {
		wake_lock_timeout(&ts->wakelock, msecs_to_jiffies(500));

		/* waiting for blsp block resuming, if not occurs i2c error */
		ret = wait_for_completion_interruptible_timeout(&ts->resume_done, msecs_to_jiffies(500));
		if (ret == 0) {
			input_err(true, &ts->client->dev, "%s: LPM: pm resume is not handled\n", __func__);
			return;
		}

		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: LPM: -ERESTARTSYS if interrupted, %d\n", __func__, ret);
			return;
		}

		input_info(true, &ts->client->dev, "%s: run LPM interrupt handler, %d\n", __func__, ret);
		/* run lpm interrupt handler */
	}

	ret = t_id = event_id = curr_pos = remain_event_count = 0;
	/* repeat READ_ONE_EVENT until buffer is empty(No event) */
	ret = sec_ts_i2c_read(ts, SEC_TS_READ_ONE_EVENT, (u8 *)read_event_buff[0], SEC_TS_EVENT_BUFF_SIZE);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: i2c read one event failed\n", __func__);
		return;
	}

	if (ts->debug_flag & SEC_TS_DEBUG_PRINT_ONEEVENT)
		input_info(true, &ts->client->dev, "ONE: %02X %02X %02X %02X %02X %02X %02X %02X\n",
				read_event_buff[0][0], read_event_buff[0][1],
				read_event_buff[0][2], read_event_buff[0][3],
				read_event_buff[0][4], read_event_buff[0][5],
				read_event_buff[0][6], read_event_buff[0][7]);

	if (read_event_buff[0][0] == 0) {
		input_info(true, &ts->client->dev, "%s: event buffer is empty\n", __func__);
		return;
	}

	if((read_event_buff[0][0] & 0x3) == 0x3) {
		input_err(true, &ts->client->dev, "%s: event buffer not vaild %02X %02X %02X %02X %02X %02X %02X %02X\n",
				__func__, read_event_buff[0][0], read_event_buff[0][1],
				read_event_buff[0][2], read_event_buff[0][3],
				read_event_buff[0][4], read_event_buff[0][5],
				read_event_buff[0][6], read_event_buff[0][7]);

		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
		if (ret < 0)
			input_err(true, &ts->client->dev, "%s: i2c write clear event failed\n", __func__);

		sec_ts_unlocked_release_all_finger(ts);
		return;
	}

	left_event_count = read_event_buff[0][7] & 0x1F;
	remain_event_count = left_event_count;

	if (left_event_count > MAX_EVENT_COUNT - 1) {
		input_err(true, &ts->client->dev, "%s: event buffer overflow\n", __func__);

		/* write clear event stack command when read_event_count > MAX_EVENT_COUNT */
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
		if (ret < 0)
			input_err(true, &ts->client->dev, "%s: i2c write clear event failed\n", __func__);

		sec_ts_unlocked_release_all_finger(ts);

		return;
	}

	if (left_event_count > 0) {
		ret = sec_ts_i2c_read(ts, SEC_TS_READ_ALL_EVENT, (u8 *)read_event_buff[1],
				sizeof(u8) * (SEC_TS_EVENT_BUFF_SIZE) * (left_event_count));
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: i2c read one event failed\n", __func__);
			return;
		}
	}

	do {
		event_buff = read_event_buff[curr_pos];
		event_id = event_buff[0] & 0x3;

		if (ts->debug_flag & SEC_TS_DEBUG_PRINT_ALLEVENT)
			input_info(true, &ts->client->dev, "ALL: %02X %02X %02X %02X %02X %02X %02X %02X\n",
					event_buff[0], event_buff[1], event_buff[2], event_buff[3],
					event_buff[4], event_buff[5], event_buff[6], event_buff[7]);

		switch (event_id) {
		case SEC_TS_STATUS_EVENT:
			p_event_status = (struct sec_ts_event_status *)event_buff;

			/* tchsta == 0 && ttype == 0 && eid == 0 : buffer empty */
			if (p_event_status->stype > 0)
				input_info(true, &ts->client->dev, "%s: STATUS %x %x %x %x %x %x %x %x\n", __func__,
						event_buff[0], event_buff[1], event_buff[2],
						event_buff[3], event_buff[4], event_buff[5],
						event_buff[6], event_buff[7]);

			/* watchdog reset -> send SENSEON command */
			if ((p_event_status->stype == TYPE_STATUS_EVENT_INFO) &&
					(p_event_status->status_id == SEC_TS_ACK_BOOT_COMPLETE) &&
					(p_event_status->status_data_1 == 0x20)) {
				ts->ic_reset_count++;
				sec_ts_unlocked_release_all_finger(ts);

				sec_ts_reinit(ts);

				ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
				if (ret < 0)
					input_err(true, &ts->client->dev, "%s: fail to write Sense_on\n", __func__);
			}

			/* event queue full-> all finger release */
			if ((p_event_status->stype == TYPE_STATUS_EVENT_ERR) &&
					(p_event_status->status_id == SEC_TS_ERR_EVENT_QUEUE_FULL)) {
				input_err(true, &ts->client->dev, "%s: IC Event Queue is full\n", __func__);
				sec_ts_unlocked_release_all_finger(ts);
			}

			if ((p_event_status->stype == TYPE_STATUS_EVENT_ERR) &&
					(p_event_status->status_id == SEC_TS_ERR_EVENT_ESD)) {
				input_err(true, &ts->client->dev, "%s: ESD detected. run reset\n", __func__);
#ifdef USE_RESET_DURING_POWER_ON
				schedule_work(&ts->reset_work.work);
#endif
			}

			if ((p_event_status->stype == TYPE_STATUS_EVENT_INFO) &&
					(p_event_status->status_id == SEC_TS_ACK_WET_MODE)) {
				ts->wet_mode = p_event_status->status_data_1;
				input_info(true, &ts->client->dev, "%s: water wet mode %d\n",
						__func__, ts->wet_mode);
				if (ts->wet_mode)
					ts->wet_count++;
			}

			if ((p_event_status->stype == TYPE_STATUS_EVENT_VENDOR_INFO) &&
					(p_event_status->status_id == SEC_TS_VENDOR_STATE_CHANGED)) {
				ts->print_info_currnet_mode = ((event_buff[2] & 0xFF) << 8) + (event_buff[3] & 0xFF);

				if (p_event_status->status_data_1 == 2 && p_event_status->status_data_2 == 2) {
					ts->scrub_id = EVENT_TYPE_TSP_SCAN_UNBLOCK;
					input_report_key(ts->input_dev, KEY_BLACK_UI_GESTURE, 1);
					input_sync(ts->input_dev);
					input_info(true, &ts->client->dev, "%s: Normal changed(%d)\n", __func__, ts->scrub_id);
				} else if (p_event_status->status_data_1 == 5 && p_event_status->status_data_2 == 2) {
					ts->scrub_id = EVENT_TYPE_TSP_SCAN_UNBLOCK;
					input_report_key(ts->input_dev, KEY_BLACK_UI_GESTURE, 1);
					input_sync(ts->input_dev);
					input_info(true, &ts->client->dev, "%s: lp changed(%d)\n", __func__, ts->scrub_id);
				} else if (p_event_status->status_data_1 == 6) {
					ts->scrub_id = EVENT_TYPE_TSP_SCAN_BLOCK;
					input_report_key(ts->input_dev, KEY_BLACK_UI_GESTURE, 1);
					input_sync(ts->input_dev);
					input_info(true, &ts->client->dev, "%s: sleep changed(%d)\n", __func__, ts->scrub_id);
				}
				input_report_key(ts->input_dev, KEY_BLACK_UI_GESTURE, 0);
				input_sync(ts->input_dev);

			}

			if ((p_event_status->stype == TYPE_STATUS_EVENT_VENDOR_INFO) &&
					(p_event_status->status_id == SEC_TS_VENDOR_ACK_NOISE_STATUS_NOTI)) {
				ts->touch_noise_status = !!p_event_status->status_data_1;
				input_info(true, &ts->client->dev, "%s: TSP NOISE MODE %s[%02X %02X]\n",
						__func__, ts->touch_noise_status == 0 ? "OFF" : "ON",
						p_event_status->status_data_2, p_event_status->status_data_3);

				if (ts->touch_noise_status)
					ts->noise_count++;
			}

			if ((p_event_status->stype == TYPE_STATUS_EVENT_VENDOR_INFO) &&
					(p_event_status->status_id == SEC_TS_VENDOR_ACK_PRE_NOISE_STATUS_NOTI)) {
				ts->touch_pre_noise_status = !!p_event_status->status_data_1;
				input_info(true, &ts->client->dev, "%s: TSP PRE NOISE MODE %s\n",
						__func__, ts->touch_pre_noise_status == 0 ? "OFF" : "ON");
			}

			if(ts->plat_data->support_ear_detect && ts->ed_enable){
				if ((p_event_status->stype == TYPE_STATUS_EVENT_VENDOR_INFO) &&
					(p_event_status->status_id == STATUS_EVENT_VENDOR_PROXIMITY)) {
						input_info(true, &ts->client->dev, "%s: EAR_DETECT(%d)\n",
							__func__, p_event_status->status_data_1);
						input_report_abs(ts->input_dev_proximity, ABS_MT_CUSTOM,
									p_event_status->status_data_1);
						input_sync(ts->input_dev_proximity);
				}
			}
			break;

		case SEC_TS_COORDINATE_EVENT:
			if (ts->power_status != SEC_TS_STATE_POWER_ON) {
				input_err(true, &ts->client->dev,
						"%s: device is closed %x %x %x %x %x %x %x %x\n", __func__,
						event_buff[0], event_buff[1], event_buff[2],
						event_buff[3], event_buff[4], event_buff[5],
						event_buff[6], event_buff[7]);
				if (!error_report) {
					error_report = true;
				}
				break;
			}
			error_report = false;

			p_event_coord = (struct sec_ts_event_coordinate *)event_buff;

			t_id = (p_event_coord->tid - 1);

#ifdef CONFIG_INPUT_WACOM
			if (ts->spen_mode_val == SPEN_ENABLE_MODE) {
				input_err(true, &ts->client->dev, "%s: COORDINATE_EVENT[%d:%d] in spen_mode(1)\n",
						__func__, t_id, p_event_coord->tchsta);
			}
#endif

			if (t_id < MAX_SUPPORT_TOUCH_COUNT + MAX_SUPPORT_HOVER_COUNT) {
				pre_ttype = ts->coord[t_id].ttype;
				ts->coord[t_id].id = t_id;
				pre_action = ts->coord[t_id].action;
				ts->coord[t_id].action = p_event_coord->tchsta;
				ts->coord[t_id].x = (p_event_coord->x_11_4 << 4) | (p_event_coord->x_3_0);
				ts->coord[t_id].y = (p_event_coord->y_11_4 << 4) | (p_event_coord->y_3_0);
				ts->coord[t_id].z = p_event_coord->z & 0x3F;
				ts->coord[t_id].ttype = p_event_coord->ttype_3_2 << 2 | p_event_coord->ttype_1_0 << 0;
				ts->coord[t_id].major = p_event_coord->major;
				ts->coord[t_id].minor = p_event_coord->minor;

				if (!ts->coord[t_id].palm && (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_PALM))
					ts->coord[t_id].palm_count++;

				ts->coord[t_id].palm = (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_PALM);
				ts->coord[t_id].left_event = p_event_coord->left_event;

				ts->coord[t_id].noise_level = max(ts->coord[t_id].noise_level, p_event_coord->noise_level);
				ts->coord[t_id].max_strength = max(ts->coord[t_id].max_strength, p_event_coord->max_strength);
				ts->coord[t_id].hover_id_num = max(ts->coord[t_id].hover_id_num, (u8)p_event_coord->hover_id_num);
				ts->coord[t_id].noise_status = p_event_coord->noise_status;

				if (ts->coord[t_id].z <= 0)
					ts->coord[t_id].z = 1;

				if ((ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_NORMAL)
						|| (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_PALM)
						|| (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_WET)
						|| (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_GLOVE)) {
					if (ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_RELEASE) {

						input_mt_slot(ts->input_dev, t_id);
						if (ts->plat_data->support_mt_pressure)
							input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);
						input_report_abs(ts->input_dev, ABS_MT_CUSTOM, 0);
						input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);

						if (ts->touch_count > 0)
							ts->touch_count--;
						if (ts->touch_count == 0) {
							input_report_key(ts->input_dev, BTN_TOUCH, 0);
							input_report_key(ts->input_dev, BTN_TOOL_FINGER, 0);
							ts->check_multi = 0;
							ts->print_info_cnt_release = 0;
						}

						location_detect(ts, location, ts->coord[t_id].x, ts->coord[t_id].y);
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
						input_info(true, &ts->client->dev,
								"[R] tID:%d loc:%s dd:%d,%d mc:%d tc:%d lx:%d ly:%d mx:%d my:%d p:%d noise:(%x,%d%d) nlvl:%d, maxS:%d, hid:%d\n",
								t_id, location,
								ts->coord[t_id].x - ts->coord[t_id].p_x,
								ts->coord[t_id].y - ts->coord[t_id].p_y,
								ts->coord[t_id].mcount, ts->touch_count,
								ts->coord[t_id].x, ts->coord[t_id].y,
								ts->coord[t_id].max_energy_x, ts->coord[t_id].max_energy_y,
								ts->coord[t_id].palm_count,
								ts->coord[t_id].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status,
								ts->coord[t_id].noise_level, ts->coord[t_id].max_strength, ts->coord[t_id].hover_id_num);
#else
						input_info(true, &ts->client->dev,
								"[R] tID:%d loc:%s dd:%d,%d mp:%d,%d mc:%d tc:%d p:%d noise:(%x,%d%d) nlvl:%d, maxS:%d, hid:%d\n",
								t_id, location,
								ts->coord[t_id].x - ts->coord[t_id].p_x,
								ts->coord[t_id].y - ts->coord[t_id].p_y,
								ts->coord[t_id].max_energy_x - ts->coord[t_id].p_x,
								ts->coord[t_id].max_energy_y - ts->coord[t_id].p_y,
								ts->coord[t_id].mcount, ts->touch_count,
								ts->coord[t_id].palm_count,
								ts->coord[t_id].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status,
								ts->coord[t_id].noise_level, ts->coord[t_id].max_strength, ts->coord[t_id].hover_id_num);
#endif
						ts->coord[t_id].action = SEC_TS_COORDINATE_ACTION_NONE;
						ts->coord[t_id].mcount = 0;
						ts->coord[t_id].palm_count = 0;
						ts->coord[t_id].noise_level = 0;
						ts->coord[t_id].max_strength = 0;
						ts->coord[t_id].hover_id_num = 0;

					} else if (ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_PRESS) {
						ts->touch_count++;
						ts->all_finger_count++;
						ts->coord[t_id].max_energy_x = 0;
						ts->coord[t_id].max_energy_y = 0;

						input_mt_slot(ts->input_dev, t_id);
						input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 1);
						input_report_key(ts->input_dev, BTN_TOUCH, 1);
						input_report_key(ts->input_dev, BTN_TOOL_FINGER, 1);

						input_report_abs(ts->input_dev, ABS_MT_POSITION_X, ts->coord[t_id].x);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, ts->coord[t_id].y);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, ts->coord[t_id].major);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MINOR, ts->coord[t_id].minor);

						if (ts->brush_mode) {
							input_report_abs(ts->input_dev, ABS_MT_CUSTOM,
									(p_event_coord->max_energy_flag << 16) | (force_strength << 8) | (ts->coord[t_id].z << 1) | ts->coord[t_id].palm);
						} else {
							input_report_abs(ts->input_dev, ABS_MT_CUSTOM,
									(p_event_coord->max_energy_flag << 16) | (force_strength << 8) | (BRUSH_Z_DATA << 1) | ts->coord[t_id].palm);
						}

						if (ts->plat_data->support_mt_pressure)
							input_report_abs(ts->input_dev, ABS_MT_PRESSURE, ts->coord[t_id].z);

						if ((ts->touch_count > 4) && (ts->check_multi == 0)) {
							ts->check_multi = 1;
							ts->multi_count++;
						}
						location_detect(ts, location, ts->coord[t_id].x, ts->coord[t_id].y);
						ts->coord[t_id].p_x = ts->coord[t_id].x;
						ts->coord[t_id].p_y = ts->coord[t_id].y;

						if (p_event_coord->max_energy_flag) {
							ts->coord[t_id].max_energy_x = ts->coord[t_id].x;
							ts->coord[t_id].max_energy_y = ts->coord[t_id].y;
						}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
						input_info(true, &ts->client->dev,
								"[P] tID:%d.%d x:%d y:%d z:%d major:%d minor:%d loc:%s tc:%d type:%X noise:(%x,%d%d), nlvl:%d, maxS:%d, hid:%d\n",
								t_id, (ts->input_dev->mt->trkid - 1) & TRKID_MAX,
								ts->coord[t_id].x, ts->coord[t_id].y, ts->coord[t_id].z,
								ts->coord[t_id].major, ts->coord[t_id].minor,
								location, ts->touch_count,
								ts->coord[t_id].ttype,
								ts->coord[t_id].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status,
								ts->coord[t_id].noise_level, ts->coord[t_id].max_strength, ts->coord[t_id].hover_id_num);
#else
						input_info(true, &ts->client->dev,
								"[P] tID:%d.%d z:%d major:%d minor:%d loc:%s tc:%d type:%X noise:(%x,%d%d), nlvl:%d, maxS:%d, hid:%d\n",
								t_id, (ts->input_dev->mt->trkid - 1) & TRKID_MAX,
								ts->coord[t_id].z, ts->coord[t_id].major,
								ts->coord[t_id].minor, location, ts->touch_count,
								ts->coord[t_id].ttype,
								ts->coord[t_id].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status,
								ts->coord[t_id].noise_level, ts->coord[t_id].max_strength, ts->coord[t_id].hover_id_num);
#endif
					} else if (ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_MOVE) {
						if (p_event_coord->max_energy_flag) {
							ts->coord[t_id].max_energy_x = ts->coord[t_id].x;
							ts->coord[t_id].max_energy_y = ts->coord[t_id].y;
						}

						if (pre_action == SEC_TS_COORDINATE_ACTION_NONE || pre_action == SEC_TS_COORDINATE_ACTION_RELEASE) {
							location_detect(ts, location, ts->coord[t_id].x, ts->coord[t_id].y);
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
							input_info(true, &ts->client->dev,
									"[M] tID:%d.%d x:%d y:%d z:%d major:%d minor:%d loc:%s tc:%d type:%X noise:(%x,%d%d), nlvl:%d, maxS:%d, hid:%d\n",
									t_id, ts->input_dev->mt->trkid & TRKID_MAX,
									ts->coord[t_id].x, ts->coord[t_id].y, ts->coord[t_id].z,
									ts->coord[t_id].major, ts->coord[t_id].minor, location, ts->touch_count,
									ts->coord[t_id].ttype, ts->coord[t_id].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status,
									ts->coord[t_id].noise_level, ts->coord[t_id].max_strength, ts->coord[t_id].hover_id_num);
#else
							input_info(true, &ts->client->dev,
									"[M] tID:%d.%d z:%d major:%d minor:%d loc:%s tc:%d type:%X noise:(%x,%d%d), nlvl:%d, maxS:%d, hid:%d\n",
									t_id, ts->input_dev->mt->trkid & TRKID_MAX, ts->coord[t_id].z, 
									ts->coord[t_id].major, ts->coord[t_id].minor, location, ts->touch_count,
									ts->coord[t_id].ttype, ts->coord[t_id].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status,
									ts->coord[t_id].noise_level, ts->coord[t_id].max_strength, ts->coord[t_id].hover_id_num);
#endif
						}

						input_mt_slot(ts->input_dev, t_id);
						input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 1);
						input_report_key(ts->input_dev, BTN_TOUCH, 1);
						input_report_key(ts->input_dev, BTN_TOOL_FINGER, 1);

						input_report_abs(ts->input_dev, ABS_MT_POSITION_X, ts->coord[t_id].x);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, ts->coord[t_id].y);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, ts->coord[t_id].major);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MINOR, ts->coord[t_id].minor);

						if (ts->brush_mode) {
							input_report_abs(ts->input_dev, ABS_MT_CUSTOM,
									(p_event_coord->max_energy_flag << 16) |(force_strength << 8) | (ts->coord[t_id].z << 1) | ts->coord[t_id].palm);
						} else {
							input_report_abs(ts->input_dev, ABS_MT_CUSTOM,
									(p_event_coord->max_energy_flag << 16) |(force_strength << 8) | (BRUSH_Z_DATA << 1) | ts->coord[t_id].palm);
						}

						if (ts->plat_data->support_mt_pressure)
							input_report_abs(ts->input_dev, ABS_MT_PRESSURE, ts->coord[t_id].z);
						ts->coord[t_id].mcount++;
					} else {
						input_dbg(true, &ts->client->dev,
								"%s: do not support coordinate action(%d)\n", __func__, ts->coord[t_id].action);
					}

					if ((ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_PRESS)
							|| (ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_MOVE)) {
						if (ts->coord[t_id].ttype != pre_ttype) {
							input_info(true, &ts->client->dev, "%s : tID:%d ttype(%x->%x)\n",
									__func__, ts->coord[t_id].id,
									pre_ttype, ts->coord[t_id].ttype);
						}
					}
				} else {
					input_dbg(true, &ts->client->dev,
							"%s: do not support coordinate type(%d)\n", __func__, ts->coord[t_id].ttype);
				}
			} else {
				input_err(true, &ts->client->dev, "%s: tid(%d) is out of range\n", __func__, t_id);
			}
			break;

		case SEC_TS_GESTURE_EVENT:
			p_gesture_status = (struct sec_ts_gesture_status *)event_buff;

			switch (p_gesture_status->stype) {
			case SEC_TS_GESTURE_CODE_SWIPE:
				ts->scrub_id = SPONGE_EVENT_TYPE_SPAY;
				input_info(true, &ts->client->dev, "%s: SPAY: %d\n", __func__, ts->scrub_id);
				input_report_key(ts->input_dev, KEY_BLACK_UI_GESTURE, 1);
				ts->all_spay_count++;
				break;
			case SEC_TS_GESTURE_CODE_DOUBLE_TAP:
				if (p_gesture_status->gesture_id == SEC_TS_GESTURE_ID_AOD) {
					ts->scrub_id = SPONGE_EVENT_TYPE_AOD_DOUBLETAB;
					ts->scrub_x = (p_gesture_status->gesture_data_1 << 4)
								| (p_gesture_status->gesture_data_3 >> 4);
					ts->scrub_y = (p_gesture_status->gesture_data_2 << 4)
								| (p_gesture_status->gesture_data_3 & 0x0F);
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
					input_info(true, &ts->client->dev, "%s: AOD: %d\n", __func__, ts->scrub_id);
#else
					input_info(true, &ts->client->dev, "%s: AOD: %d, %d, %d\n",
							__func__, ts->scrub_id, ts->scrub_x, ts->scrub_y);
#endif
					input_report_key(ts->input_dev, KEY_BLACK_UI_GESTURE, 1);
					ts->all_aod_tap_count++;
				} else if (p_gesture_status->gesture_id == SEC_TS_GESTURE_ID_DOUBLETAP_TO_WAKEUP) {
					input_info(true, &ts->client->dev, "%s: AOT\n", __func__);
					input_report_key(ts->input_dev, KEY_WAKEUP, 1);
					input_sync(ts->input_dev);
					input_report_key(ts->input_dev, KEY_WAKEUP, 0);
				}
				break;
			case SEC_TS_GESTURE_CODE_SINGLE_TAP:
				ts->scrub_id = SPONGE_EVENT_TYPE_SINGLE_TAP;
				ts->scrub_x = (p_gesture_status->gesture_data_1 << 4)
							| (p_gesture_status->gesture_data_3 >> 4);
				ts->scrub_y = (p_gesture_status->gesture_data_2 << 4)
							| (p_gesture_status->gesture_data_3 & 0x0F);
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
				input_info(true, &ts->client->dev, "%s: SINGLE TAP: %d\n", __func__, ts->scrub_id);
#else
				input_info(true, &ts->client->dev, "%s: SINGLE TAP: %d, %d, %d\n",
						__func__, ts->scrub_id, ts->scrub_x, ts->scrub_y);
#endif
				input_report_key(ts->input_dev, KEY_BLACK_UI_GESTURE, 1);
				break;
			case SEC_TS_GESTURE_CODE_PRESS:
				input_info(true, &ts->client->dev, "%s: FOD: %sPRESS\n",
						__func__, p_gesture_status->gesture_id ? "" : "LONG");
				break;
			}

			input_sync(ts->input_dev);
			input_report_key(ts->input_dev, KEY_BLACK_UI_GESTURE, 0);
			break;
		default:
			input_err(true, &ts->client->dev, "%s: unknown event %x %x %x %x %x %x\n", __func__,
					event_buff[0], event_buff[1], event_buff[2],
					event_buff[3], event_buff[4], event_buff[5]);
			break;
		}

		curr_pos++;
		remain_event_count--;
	} while (remain_event_count >= 0);

	input_sync(ts->input_dev);

	if(ts->touch_count == 0 && ts->tsp_temp_data_skip){
		ts->tsp_temp_data_skip = false;
		sec_ts_set_temp(ts, false);
		input_err(true, &ts->client->dev, "%s: sec_ts_set_temp, no touch\n", __func__);	
	}
}

static irqreturn_t sec_ts_irq_thread(int irq, void *ptr)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)ptr;

	if (gpio_get_value(ts->plat_data->irq_gpio) == 1)
		return IRQ_HANDLED;

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (secure_filter_interrupt(ts) == IRQ_HANDLED) {
		wait_for_completion_interruptible_timeout(&ts->secure_interrupt,
				msecs_to_jiffies(5 * MSEC_PER_SEC));

		input_info(true, &ts->client->dev,
				"%s: secure interrupt handled\n", __func__);

		return IRQ_HANDLED;
	}
#endif
#ifdef CONFIG_SAMSUNG_TUI
	if (STUI_MODE_TOUCH_SEC & stui_get_mode())
		return IRQ_HANDLED;
#endif
	mutex_lock(&ts->eventlock);

	sec_ts_read_event(ts);

	mutex_unlock(&ts->eventlock);

	return IRQ_HANDLED;
}

int sec_ts_set_cover_type(struct sec_ts_data *ts, bool enable)
{
	int ret;

	input_info(true, &ts->client->dev, "%s: %d\n", __func__, ts->cover_type);

	switch (ts->cover_type) {
	case SEC_TS_VIEW_WIRELESS:
	case SEC_TS_VIEW_COVER:
	case SEC_TS_VIEW_WALLET:
	case SEC_TS_FLIP_WALLET:
	case SEC_TS_LED_COVER:
	case SEC_TS_MONTBLANC_COVER:
	case SEC_TS_CLEAR_FLIP_COVER:
	case SEC_TS_QWERTY_KEYBOARD_US:
	case SEC_TS_QWERTY_KEYBOARD_KOR:
	case SEC_TS_CLEAR_SIDE_VIEW_COVER:
		ts->cover_cmd = (u8)ts->cover_type;
		break;
	case SEC_TS_CHARGER_COVER:
	case SEC_TS_COVER_NOTHING1:
	case SEC_TS_COVER_NOTHING2:
	case SEC_TS_NEON_COVER:
	case SEC_TS_ALCANTARA_COVER:
	case SEC_TS_GAMEPACK_COVER:
	case SEC_TS_LED_BACK_COVER:
	default:
		ts->cover_cmd = 0;
		input_err(true, &ts->client->dev, "%s: not chage touch state, %d\n",
				__func__, ts->cover_type);
		break;
	}

	if (enable)
		ts->touch_functions = (ts->touch_functions | SEC_TS_BIT_SETFUNC_COVER | SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC);
	else
		ts->touch_functions = ((ts->touch_functions & (~SEC_TS_BIT_SETFUNC_COVER)) | SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: pwr off, close:%d, status:%x\n", __func__,
				enable, ts->touch_functions);
		goto cover_enable_err;
	}

	if (enable) {
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_COVERTYPE, &ts->cover_cmd, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: Failed to send covertype command: %d", __func__, ts->cover_cmd);
			goto cover_enable_err;
		}
	}

	ret = sec_ts_set_touch_function(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: Failed to send command", __func__);
		goto cover_enable_err;
	}

	input_info(true, &ts->client->dev, "%s: close:%d, status:%x\n", __func__,
			enable, ts->touch_functions);

	return 0;

cover_enable_err:
	return -EIO;
}
EXPORT_SYMBOL(sec_ts_set_cover_type);

void sec_ts_set_grip_type(struct sec_ts_data *ts, u8 set_type)
{
	u8 mode = G_NONE;

	input_info(true, &ts->client->dev, "%s: re-init grip(%d), edh:%d, edg:%d, lan:%d\n", __func__,
			set_type, ts->grip_edgehandler_direction, ts->grip_edge_range, ts->grip_landscape_mode);

	/* edge handler */
	if (ts->grip_edgehandler_direction != 0)
		mode |= G_SET_EDGE_HANDLER;

	if (set_type == GRIP_ALL_DATA) {
		/* edge */
		if (ts->grip_edge_range != 60)
			mode |= G_SET_EDGE_ZONE;

		/* dead zone */
		if (ts->grip_landscape_mode == 1)	/* default 0 mode, 32 */
			mode |= G_SET_LANDSCAPE_MODE;
		else
			mode |= G_SET_NORMAL_MODE;
	}

	if (mode)
		set_grip_data_to_ic(ts, mode);
}

struct sec_ts_data *g_ts;

static ssize_t sec_ts_tsp_cmoffset_all_read(struct file *file, char __user *buf,
					size_t len, loff_t *offset)
{
	struct sec_ts_data *ts;
	static ssize_t retlen = 0;
	ssize_t retlen_sdc = 0, retlen_sub = 0, retlen_main = 0;
	ssize_t count = 0;
	loff_t pos = *offset;
#if defined(CONFIG_SEC_FACTORY)
	int ret;
#endif

	if (!g_ts) {
		pr_err("%s %s: dev is null\n", SECLOG, __func__);
		return 0;
	}
	ts = g_ts;

	if (ts->proc_cmoffset_size == 0) {
		pr_err("%s %s: proc_cmoffset_size is 0\n", SECLOG, __func__);
		return 0;
	}

	if (pos == 0) {
#if defined(CONFIG_SEC_FACTORY)
		ret = get_cmoffset_dump_all(ts, ts->cmoffset_sdc_proc, OFFSET_FW_SDC);
		if (ret < 0)
			input_err(true, &ts->client->dev,
				"%s : sec_ts_get_cmoffset_dump SDC fail use boot time value\n", __func__);
		ret = get_cmoffset_dump_all(ts, ts->cmoffset_sub_proc, OFFSET_FW_SUB);
		if (ret < 0)
			input_err(true, &ts->client->dev,
				"%s : sec_ts_get_cmoffset_dump SUB fail use boot time value\n", __func__);
		ret = get_cmoffset_dump_all(ts, ts->cmoffset_main_proc, OFFSET_FW_MAIN);
		if (ret < 0)
			input_err(true, &ts->client->dev,
				"%s : sec_ts_get_cmoffset_dump MAIN fail use boot time value\n", __func__);
#endif
		retlen_sdc = strlen(ts->cmoffset_sdc_proc);
		retlen_sub = strlen(ts->cmoffset_sub_proc);
		retlen_main = strlen(ts->cmoffset_main_proc);

		ts->cmoffset_all_proc = kzalloc(ts->proc_cmoffset_all_size, GFP_KERNEL);
		if (!ts->cmoffset_all_proc){
			input_err(true, &ts->client->dev, "%s : kzalloc fail (cmoffset_all_proc)\n", __func__);
			return count;
		}

		strlcat(ts->cmoffset_all_proc, ts->cmoffset_sdc_proc, ts->proc_cmoffset_all_size);
		strlcat(ts->cmoffset_all_proc, ts->cmoffset_sub_proc, ts->proc_cmoffset_all_size);
		strlcat(ts->cmoffset_all_proc, ts->cmoffset_main_proc, ts->proc_cmoffset_all_size);

		retlen = strlen(ts->cmoffset_all_proc);

		input_info(true, &ts->client->dev, "%s : retlen[%d], retlen_sdc[%d], retlen_sub[%d], retlen_main[%d]\n",
						__func__, retlen, retlen_sdc, retlen_sub, retlen_main);
	}

	if (pos >= retlen)
		return 0;

	count = min(len, (size_t)(retlen - pos));

	input_info(true, &ts->client->dev, "%s : total:%d pos:%d count:%d\n", __func__, retlen, pos, count);

	if (copy_to_user(buf, ts->cmoffset_all_proc + pos, count)) {
		input_err(true, &ts->client->dev, "%s : copy_to_user error!\n", __func__, retlen, pos);
		return -EFAULT;
	}

	*offset += count;

	if (count < len) {
		input_info(true, &ts->client->dev, "%s : print all & free cmoffset_all_proc [%d][%d]\n",
					__func__, retlen, offset);
		if (ts->cmoffset_all_proc)
			kfree(ts->cmoffset_all_proc);
		retlen = 0;
	}

	return count;
}

static ssize_t sec_ts_tsp_fail_hist_all_read(struct file *file, char __user *buf,
					size_t len, loff_t *offset)
{
	struct sec_ts_data *ts;

	static ssize_t retlen = 0;
	ssize_t retlen_sdc = 0, retlen_sub = 0, retlen_main = 0;
	ssize_t count = 0;
	loff_t pos = *offset;
#if defined(CONFIG_SEC_FACTORY)
	int ret;
#endif

	if (!g_ts) {
		pr_err("%s %s: dev is null\n", SECLOG, __func__);
		return 0;
	}
	ts = g_ts;

	if (ts->proc_fail_hist_size == 0) {
		pr_err("%s %s: proc_fail_hist_size is 0\n", SECLOG, __func__);
		return 0;
	}

	if (pos == 0) {
#if defined(CONFIG_SEC_FACTORY)
		ret = get_selftest_fail_hist_dump_all(ts, ts->fail_hist_sdc_proc, OFFSET_FW_SDC);
		if (ret < 0)
			input_err(true, &ts->client->dev,
				"%s : sec_ts_get_fail_hist_dump SDC fail use boot time value\n", __func__);
		ret = get_selftest_fail_hist_dump_all(ts, ts->fail_hist_sub_proc, OFFSET_FW_SUB);
		if (ret < 0)
			input_err(true, &ts->client->dev,
				"%s : sec_ts_get_fail_hist_dump SUB fail use boot time value\n", __func__);
		ret = get_selftest_fail_hist_dump_all(ts, ts->fail_hist_main_proc, OFFSET_FW_MAIN);
		if (ret < 0)
			input_err(true, &ts->client->dev,
				"%s : sec_ts_get_fail_hist_dump MAIN fail use boot time value\n", __func__);
#endif
		retlen_sdc = strlen(ts->fail_hist_sdc_proc);
		retlen_sub = strlen(ts->fail_hist_sub_proc);
		retlen_main = strlen(ts->fail_hist_main_proc);

		ts->fail_hist_all_proc = kzalloc(ts->proc_fail_hist_all_size, GFP_KERNEL);
		if (!ts->fail_hist_all_proc){
			input_err(true, &ts->client->dev, "%s : kzalloc fail (fail_hist_all_proc)\n", __func__);
			return count;
		}

		strlcat(ts->fail_hist_all_proc, ts->fail_hist_sdc_proc, ts->proc_fail_hist_all_size);
		strlcat(ts->fail_hist_all_proc, ts->fail_hist_sub_proc, ts->proc_fail_hist_all_size);
		strlcat(ts->fail_hist_all_proc, ts->fail_hist_main_proc, ts->proc_fail_hist_all_size);

		retlen = strlen(ts->fail_hist_all_proc);

		input_info(true, &ts->client->dev, "%s : retlen[%d], retlen_sdc[%d], retlen_sub[%d], retlen_main[%d]\n",
						__func__, retlen, retlen_sdc, retlen_sub, retlen_main);
	}

	if (pos >= retlen)
		return 0;

	count = min(len, (size_t)(retlen - pos));

	input_info(true, &ts->client->dev, "%s : total:%d pos:%d count:%d\n", __func__, retlen, pos, count);

	if (copy_to_user(buf, ts->fail_hist_all_proc + pos, count)) {
		input_err(true, &ts->client->dev, "%s : copy_to_user error!\n", __func__, retlen, pos);
		return -EFAULT;
	}

	*offset += count;

	if (count < len) {
		input_info(true, &ts->client->dev, "%s : print all & free fail_hist_all_proc [%d][%d]\n",
					__func__, retlen, offset);
		if (ts->fail_hist_all_proc)
			kfree(ts->fail_hist_all_proc);
		retlen = 0;
	}

	return count;
}

static ssize_t sec_ts_tsp_cmoffset_read(struct file *file, char __user *buf,
					size_t len, loff_t *offset)
{
	pr_info("[sec_input] %s called offset:%d\n", __func__, (int)*offset);
	return sec_ts_tsp_cmoffset_all_read(file, buf, len, offset);
}

static ssize_t sec_ts_tsp_fail_hist_read(struct file *file, char __user *buf,
					size_t len, loff_t *offset)
{
	pr_info("[sec_input] %s called fail_hist:%d\n", __func__, (int)*offset);
	return sec_ts_tsp_fail_hist_all_read(file, buf, len, offset);
}

static const struct file_operations tsp_cmoffset_all_file_ops = {
	.owner = THIS_MODULE,
	.read = sec_ts_tsp_cmoffset_read,
	.llseek = generic_file_llseek,
};

static const struct file_operations tsp_fail_hist_all_file_ops = {
	.owner = THIS_MODULE,
	.read = sec_ts_tsp_fail_hist_read,
	.llseek = generic_file_llseek,
};

static void sec_ts_init_proc(struct sec_ts_data *ts)
{
	struct proc_dir_entry *entry_cmoffset_all;
	struct proc_dir_entry *entry_fail_hist_all;

	ts->proc_cmoffset_size = (ts->tx_count * ts->rx_count * 4 + 100) * 3;	/* cm1 cm2 cm3 */
	ts->proc_cmoffset_all_size = ts->proc_cmoffset_size * 3;	/* sdc sub main */

	ts->proc_fail_hist_size = ((ts->tx_count + ts->rx_count) * 4 + 100) * 6;	/* have to check */
	ts->proc_fail_hist_all_size = ts->proc_fail_hist_size * 3;	/* sdc sub main */

	ts->cmoffset_sdc_proc = kzalloc(ts->proc_cmoffset_size, GFP_KERNEL);
	if (!ts->cmoffset_sdc_proc)
		return;

	ts->cmoffset_sub_proc = kzalloc(ts->proc_cmoffset_size, GFP_KERNEL);
	if (!ts->cmoffset_sub_proc)
		goto err_alloc_sub;

	ts->cmoffset_main_proc = kzalloc(ts->proc_cmoffset_size, GFP_KERNEL);
	if (!ts->cmoffset_main_proc)
		goto err_alloc_main;

	ts->fail_hist_sdc_proc = kzalloc(ts->proc_fail_hist_size, GFP_KERNEL);
	if (!ts->fail_hist_sdc_proc)
		goto err_alloc_fail_hist_sdc;

	ts->fail_hist_sub_proc = kzalloc(ts->proc_fail_hist_size, GFP_KERNEL);
	if (!ts->fail_hist_sub_proc)
		goto err_alloc_fail_hist_sub;

	ts->fail_hist_main_proc = kzalloc(ts->proc_fail_hist_size, GFP_KERNEL);
	if (!ts->fail_hist_main_proc)
		goto err_alloc_fail_hist_main;

	entry_cmoffset_all = proc_create("tsp_cmoffset_all", S_IFREG | 0444, NULL, &tsp_cmoffset_all_file_ops);
	if (!entry_cmoffset_all) {
		input_err(true, &ts->client->dev, "%s: failed to create /proc/tsp_cmoffset_all\n", __func__);
		goto err_cmoffset_proc_create;
	}
	proc_set_size(entry_cmoffset_all, ts->proc_cmoffset_all_size);

	entry_fail_hist_all = proc_create("tsp_fail_hist_all", S_IFREG | 0444, NULL, &tsp_fail_hist_all_file_ops);
	if (!entry_fail_hist_all) {
		input_err(true, &ts->client->dev, "%s: failed to create /proc/tsp_fail_hist_all\n", __func__);
		goto err_fail_hist_proc_create;
	}
	proc_set_size(entry_fail_hist_all, ts->proc_fail_hist_all_size);

	g_ts = ts;
	input_info(true, &ts->client->dev, "%s: done\n", __func__);
	return;

err_fail_hist_proc_create:
	proc_remove(entry_cmoffset_all);
err_cmoffset_proc_create:
	kfree(ts->fail_hist_main_proc);
err_alloc_fail_hist_main:
	kfree(ts->fail_hist_sub_proc);
err_alloc_fail_hist_sub:
	kfree(ts->fail_hist_sdc_proc);
err_alloc_fail_hist_sdc:
	kfree(ts->cmoffset_main_proc);
err_alloc_main:
	kfree(ts->cmoffset_sub_proc);
err_alloc_sub:
	kfree(ts->cmoffset_sdc_proc);

	ts->cmoffset_sdc_proc = NULL;
	ts->cmoffset_sub_proc = NULL;
	ts->cmoffset_main_proc = NULL;
	ts->cmoffset_all_proc = NULL;
	ts->proc_cmoffset_size = 0;
	ts->proc_cmoffset_all_size = 0;

	ts->fail_hist_sdc_proc = NULL;
	ts->fail_hist_sub_proc = NULL;
	ts->fail_hist_main_proc = NULL;
	ts->fail_hist_all_proc = NULL;
	ts->proc_fail_hist_size = 0;
	ts->proc_fail_hist_all_size = 0;

	input_err(true, &ts->client->dev, "%s: failed\n", __func__);
}

/* for debugging--------------------------------------------------------------------------------------*/
static int sec_ts_pinctrl_configure(struct sec_ts_data *ts, bool enable)
{
	struct pinctrl_state *state;

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, enable ? "ACTIVE" : "SUSPEND");

	if (enable) {
		state = pinctrl_lookup_state(ts->plat_data->pinctrl, "on_state");
		if (IS_ERR(ts->plat_data->pinctrl))
			input_err(true, &ts->client->dev, "%s: could not get active pinstate\n", __func__);
	} else {
		state = pinctrl_lookup_state(ts->plat_data->pinctrl, "off_state");
		if (IS_ERR(ts->plat_data->pinctrl))
			input_err(true, &ts->client->dev, "%s: could not get suspend pinstate\n", __func__);
	}

	if (!IS_ERR_OR_NULL(state))
		return pinctrl_select_state(ts->plat_data->pinctrl, state);

	return 0;
}

int sec_ts_power(void *data, bool on)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)data;
	const struct sec_ts_plat_data *pdata = ts->plat_data;
	struct regulator *regulator_dvdd = NULL;
	struct regulator *regulator_avdd = NULL;
	static bool enabled;
	static bool boot_on = true;
	int ret = 0;

	if (enabled == on)
		return ret;

	regulator_dvdd = regulator_get(NULL, pdata->regulator_dvdd);
	if (IS_ERR_OR_NULL(regulator_dvdd)) {
		input_err(true, &ts->client->dev, "%s: Failed to get %s regulator.\n",
				__func__, pdata->regulator_dvdd);
		ret = PTR_ERR(regulator_dvdd);
		goto error;
	}

	regulator_avdd = regulator_get(NULL, pdata->regulator_avdd);
	if (IS_ERR_OR_NULL(regulator_avdd)) {
		input_err(true, &ts->client->dev, "%s: Failed to get %s regulator.\n",
				__func__, pdata->regulator_avdd);
		ret = PTR_ERR(regulator_avdd);
		goto error;
	}

	if (on) {
		if (!regulator_is_enabled(regulator_dvdd) || boot_on) {
			ret = regulator_enable(regulator_dvdd);
			if (ret) {
				input_err(true, &ts->client->dev, "%s: Failed to enable avdd: %d\n", __func__, ret);
				goto out;
			}
		} else {
			input_err(true, &ts->client->dev, "%s: dvdd is already enabled\n", __func__);
		}

		sec_ts_delay(1);

		if (!regulator_is_enabled(regulator_avdd) || boot_on) {
			ret = regulator_enable(regulator_avdd);
			if (ret) {
				input_err(true, &ts->client->dev, "%s: Failed to enable vdd: %d\n", __func__, ret);
				regulator_disable(regulator_dvdd);
				goto out;
			}
		} else {
			input_err(true, &ts->client->dev, "%s: avdd is already enabled\n", __func__);
		}
	} else {
		if (regulator_is_enabled(regulator_avdd)) {
			ret = regulator_disable(regulator_avdd);
			if (ret) {
				input_err(true, &ts->client->dev, "%s: failed to disable avdd: %d\n", __func__, ret);
				goto out;
			}
		} else {
			input_err(true, &ts->client->dev, "%s: avdd is already disabled\n", __func__);
		}

		sec_ts_delay(4);

		if (regulator_is_enabled(regulator_dvdd)) {
			ret = regulator_disable(regulator_dvdd);
			if (ret) {
				input_err(true, &ts->client->dev, "%s: failed to disable dvdd: %d\n", __func__, ret);
				regulator_enable(regulator_avdd);
				goto out;
			}
		} else {
			input_err(true, &ts->client->dev, "%s: dvdd is already disabled\n", __func__);
		}
	}

	enabled = on;

out:
	input_err(true, &ts->client->dev, "%s: %s: avdd:%s, dvdd:%s\n", __func__, on ? "on" : "off",
			regulator_is_enabled(regulator_avdd) ? "on" : "off",
			regulator_is_enabled(regulator_dvdd) ? "on" : "off");

error:
	regulator_put(regulator_dvdd);
	regulator_put(regulator_avdd);

	boot_on = false;

	return ret;
}

static int sec_ts_parse_dt(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct sec_ts_plat_data *pdata = dev->platform_data;
	struct device_node *np = dev->of_node;
	u32 coords[2];
	int ret = 0;
	int count = 0;
	u32 ic_match_value;
	int lcdtype = 0;
#if defined(CONFIG_EXYNOS_DPU20)
	int connected;
#endif
	u32 px_zone[3] = { 0 };

	pdata->tsp_icid = of_get_named_gpio(np, "sec,tsp-icid_gpio", 0);
	if (gpio_is_valid(pdata->tsp_icid)) {
		input_info(true, dev, "%s: TSP_ICID : %d\n", __func__, gpio_get_value(pdata->tsp_icid));
		if (of_property_read_u32(np, "sec,icid_match_value", &ic_match_value)) {
			input_err(true, dev, "%s: Failed to get icid match value\n", __func__);
			return -EINVAL;
		}

		if (gpio_get_value(pdata->tsp_icid) != ic_match_value) {
			input_err(true, dev, "%s: Do not match TSP_ICID\n", __func__);
			return -EINVAL;
		}
	} else {
		input_err(true, dev, "%s: Failed to get tsp-icid gpio\n", __func__);
	}

	pdata->irq_gpio = of_get_named_gpio(np, "sec,irq_gpio", 0);
	if (gpio_is_valid(pdata->irq_gpio)) {
		ret = gpio_request_one(pdata->irq_gpio, GPIOF_DIR_IN, "sec,tsp_int");
		if (ret) {
			input_err(true, &client->dev, "%s: Unable to request tsp_int [%d]\n", __func__, pdata->irq_gpio);
			return -EINVAL;
		}
	} else {
		input_err(true, &client->dev, "%s: Failed to get irq gpio\n", __func__);
		return -EINVAL;
	}

	client->irq = gpio_to_irq(pdata->irq_gpio);

	if (of_property_read_u32(np, "sec,irq_type", &pdata->irq_type)) {
		input_err(true, dev, "%s: Failed to get irq_type property\n", __func__);
		pdata->irq_type = IRQF_TRIGGER_LOW | IRQF_ONESHOT;
	} else {
		input_info(true, dev, "%s: irq_type property:%X, %d\n", __func__,
				pdata->irq_type, pdata->irq_type);
	}

	if (of_property_read_u32(np, "sec,i2c-burstmax", &pdata->i2c_burstmax)) {
		input_dbg(false, &client->dev, "%s: Failed to get i2c_burstmax property\n", __func__);
		pdata->i2c_burstmax = 0xffff;
	}

	if (of_property_read_u32_array(np, "sec,max_coords", coords, 2)) {
		input_err(true, &client->dev, "%s: Failed to get max_coords property\n", __func__);
		return -EINVAL;
	}
	pdata->max_x = coords[0] - 1;
	pdata->max_y = coords[1] - 1;

	if (of_property_read_u32(np, "sec,bringup", &pdata->bringup) < 0)
		pdata->bringup = 0;

#if defined(CONFIG_DISPLAY_SAMSUNG)
	lcdtype = get_lcd_attached("GET");
	if (lcdtype == 0xFFFFFF) {
		input_err(true, &client->dev, "%s: lcd is not attached\n", __func__);
		return -ENODEV;
	}
#endif

#if defined(CONFIG_EXYNOS_DPU20)
	connected = get_lcd_info("connected");
	if (connected < 0) {
		input_err(true, dev, "%s: Failed to get lcd info\n", __func__);
		return -EINVAL;
	}

	if (!connected) {
		input_err(true, &client->dev, "%s: lcd is disconnected\n", __func__);
		return -ENODEV;
	}

	input_info(true, &client->dev, "%s: lcd is connected\n", __func__);

	lcdtype = get_lcd_info("id");
	if (lcdtype < 0) {
		input_err(true, dev, "%s: Failed to get lcd info\n", __func__);
		return -EINVAL;
	}
#endif
	input_info(true, &client->dev, "%s: lcdtype 0x%08X\n", __func__, lcdtype);

#if 0
	pdata->tsp_id = of_get_named_gpio(np, "sec,tsp-id_gpio", 0);
	if (gpio_is_valid(pdata->tsp_id))
		input_info(true, dev, "%s: TSP_ID : %d\n", __func__, gpio_get_value(pdata->tsp_id));
	else
		input_err(true, dev, "%s: Failed to get tsp-id gpio\n", __func__);

	count = of_property_count_strings(np, "sec,firmware_name");
	if (count <= 0) {
		pdata->firmware_name = NULL;
	} else {
		if (gpio_is_valid(pdata->tsp_id))
			of_property_read_string_index(np, "sec,firmware_name", gpio_get_value(pdata->tsp_id), &pdata->firmware_name);
		else
			of_property_read_string_index(np, "sec,firmware_name", 0, &pdata->firmware_name);
	}
#else
	/* [ temporary code for old octa panel ]*/
	pdata->tsp_id = of_get_named_gpio(np, "sec,tsp-id_gpio", 0);
	if (gpio_is_valid(pdata->tsp_id))
		input_info(true, dev, "%s: TSP_ID : %d\n", __func__, gpio_get_value(pdata->tsp_id));
	else
		input_err(true, dev, "%s: Failed to get tsp-id gpio\n", __func__);

	count = of_property_count_strings(np, "sec,firmware_name");
	if (count <= 0) {
		pdata->firmware_name = NULL;

	} else if (count == 1) {
		of_property_read_string_index(np, "sec,firmware_name", 0, &pdata->firmware_name);

	/* for D2 */
	} else if (count == 3) {
		if ((lcdtype & 0xFFFF) == 0x0012) {	// based on bx panel
			of_property_read_string_index(np, "sec,firmware_name", 0, &pdata->firmware_name);

		} else if ((lcdtype & 0xFFFF) == 0x0013) {	// based on bx panel + change tx #0 & #2
			of_property_read_string_index(np, "sec,firmware_name", 1, &pdata->firmware_name);

		} else if ((lcdtype & 0xFFFF) == 0x0410) {	// D2 panel
			of_property_read_string_index(np, "sec,firmware_name", 2, &pdata->firmware_name);

		} else {
			pdata->firmware_name = NULL;
			input_err(true, &client->dev, "%s: Abnormal lcd type:%06X\n", __func__, lcdtype);
		}

	} else {
		pdata->firmware_name = NULL;
		input_err(true, &client->dev, "%s: Abnormal fw count(%0X)\n", __func__, count);
	}
	/* [ need to remove before pvr ] */

#endif

	if (of_property_read_string_index(np, "sec,project_name", 0, &pdata->project_name))
		input_err(true, &client->dev, "%s: skipped to get project_name property\n", __func__);
	if (of_property_read_string_index(np, "sec,project_name", 1, &pdata->model_name))
		input_err(true, &client->dev, "%s: skipped to get model_name property\n", __func__);

	if (of_property_read_string(np, "sec,regulator_dvdd", &pdata->regulator_dvdd)) {
		input_err(true, dev, "%s: Failed to get regulator_dvdd name property\n", __func__);
		return -EINVAL;
	}

	if (of_property_read_string(np, "sec,regulator_avdd", &pdata->regulator_avdd)) {
		input_err(true, dev, "%s: Failed to get regulator_avdd name property\n", __func__);
		return -EINVAL;
	}

	pdata->power = sec_ts_power;

	pdata->regulator_boot_on = of_property_read_bool(np, "sec,regulator_boot_on");
	pdata->support_dex = of_property_read_bool(np, "support_dex_mode");
	pdata->support_fod = of_property_read_bool(np, "support_fod");
	pdata->enable_settings_aot = of_property_read_bool(np, "enable_settings_aot");
	pdata->sync_reportrate_120 = of_property_read_bool(np, "sync-reportrate-120");
	pdata->support_ear_detect = of_property_read_bool(np, "support_ear_detect_mode");
	pdata->support_open_short_test = of_property_read_bool(np, "support_open_short_test");
	pdata->support_mis_calibration_test = of_property_read_bool(np, "support_mis_calibration_test");

	if (of_property_read_u32_array(np, "sec,area-size", px_zone, 3)) {
		input_info(true, &client->dev, "Failed to get zone's size\n");
		pdata->area_indicator = 48;
		pdata->area_navigation = 96;
		pdata->area_edge = 60;
	} else {
		pdata->area_indicator = px_zone[0];
		pdata->area_navigation = px_zone[1];
		pdata->area_edge = px_zone[2];
	}
	input_info(true, &client->dev, "%s : zone's size - indicator:%d, navigation:%d, edge:%d\n",
		__func__, pdata->area_indicator, pdata->area_navigation ,pdata->area_edge);

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	of_property_read_u32(np, "sec,ss_touch_num", &pdata->ss_touch_num);
	input_err(true, dev, "%s: ss_touch_num:%d\n", __func__, pdata->ss_touch_num);
#endif
#ifdef CONFIG_SEC_FACTORY
	pdata->support_mt_pressure = true;
#endif
	input_err(true, &client->dev, "%s: i2c buffer limit: %d, lcd_id:%06X, bringup:%d, FW:%s(%d),"
			" id:%d,%d, dex:%d, max(%d/%d), FOD:%d, AOT:%d, ED:%d\n",
			__func__, pdata->i2c_burstmax, lcdtype, pdata->bringup, pdata->firmware_name,
			count, pdata->tsp_id, pdata->tsp_icid,
			pdata->support_dex, pdata->max_x, pdata->max_y,
			pdata->support_fod, pdata->enable_settings_aot, pdata->support_ear_detect);
	return ret;
}

void sec_tclm_parse_dt(struct i2c_client *client, struct sec_tclm_data *tdata)
{
	struct device *dev = &client->dev;
	struct device_node *np = dev->of_node;

	if (of_property_read_u32(np, "sec,tclm_level", &tdata->tclm_level) < 0) {
		tdata->tclm_level = 0;
		input_err(true, dev, "%s: Failed to get tclm_level property\n", __func__);
	}

	if (of_property_read_u32(np, "sec,afe_base", &tdata->afe_base) < 0) {
		tdata->afe_base = 0;
		input_err(true, dev, "%s: Failed to get afe_base property\n", __func__);
	}

	tdata->support_tclm_test = of_property_read_bool(np, "support_tclm_test");

	input_err(true, &client->dev, "%s: tclm_level %d, sec_afe_base %04X\n", __func__, tdata->tclm_level, tdata->afe_base);
}

int sec_ts_read_information(struct sec_ts_data *ts)
{
	unsigned char data[13] = { 0 };
	int ret;

	memset(data, 0x0, 3);
	ret = sec_ts_i2c_read(ts, SEC_TS_READ_ID, data, 3);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read device id(%d)\n",
				__func__, ret);
		return ret;
	}

	input_info(true, &ts->client->dev,
			"%s: %X, %X, %X\n",
			__func__, data[0], data[1], data[2]);
	memset(data, 0x0, 11);
	ret = sec_ts_i2c_read(ts,  SEC_TS_READ_PANEL_INFO, data, 11);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read sub id(%d)\n",
				__func__, ret);
		return ret;
	}

	/* Set X,Y Resolution from IC information. */
	if (((data[0] << 8) | data[1]) > 0)
		ts->plat_data->max_x = ((data[0] << 8) | data[1]) - 1;

	if (((data[2] << 8) | data[3]) > 0)
		ts->plat_data->max_y = ((data[2] << 8) | data[3]) - 1;

	/* Set X,Y Display Resolution from IC information. */
	ts->plat_data->display_x = ((data[4] << 8) | data[5]) - 1;
	ts->plat_data->display_y = ((data[6] << 8) | data[7]) - 1;

	ts->tx_count = min((int)data[8], TOUCH_TX_CHANNEL_NUM);
	ts->rx_count = min((int)data[9], TOUCH_RX_CHANNEL_NUM);

	input_info(true, &ts->client->dev,
			"%s: nTX:%d, nRX:%d,  rX:%d, rY:%d, dX:%d, dY:%d\n",
			__func__, ts->tx_count , ts->rx_count,
			ts->plat_data->max_x, ts->plat_data->max_y,
			ts->plat_data->display_x, ts->plat_data->display_y);

	data[0] = 0;
	ret = sec_ts_i2c_read(ts, SEC_TS_READ_BOOT_STATUS, data, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read sub id(%d)\n",
				__func__, ret);
		return ret;
	}

	input_info(true, &ts->client->dev,
			"%s: STATUS : %X\n",
			__func__, data[0]);

	memset(data, 0x0, 4);
	ret = sec_ts_i2c_read(ts, SEC_TS_READ_TS_STATUS, data, 4);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read sub id(%d)\n",
				__func__, ret);
		return ret;
	}

	input_info(true, &ts->client->dev,
			"%s: TOUCH STATUS : %02X, %02X, %02X, %02X\n",
			__func__, data[0], data[1], data[2], data[3]);
	ret = sec_ts_i2c_read(ts, SEC_TS_CMD_SET_TOUCHFUNCTION,  (u8 *)&(ts->touch_functions), 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read touch functions(%d)\n",
				__func__, ret);
		return ret;
	}

	input_info(true, &ts->client->dev,
			"%s: Functions : %02X\n",
			__func__, ts->touch_functions);

	return ret;
}

int sec_ts_set_custom_library(struct sec_ts_data *ts)
{
	u8 data[3] = { 0 };
	int ret;
	u8 force_fod_enable = 0;

	if (!ts->use_sponge)
		return 0;

#ifdef CONFIG_SEC_FACTORY
	/* enable FOD when LCD on state */
	if (ts->plat_data->support_fod && ts->input_closed == false)
		force_fod_enable = SEC_TS_MODE_SPONGE_PRESS;
#endif

	input_err(true, &ts->client->dev, "%s: Sponge (0x%02x)%s\n",
			__func__, ts->lowpower_mode,
			force_fod_enable ? ", force fod enable" : "");

	data[2] = ts->lowpower_mode | force_fod_enable;

	ret = ts->sec_ts_write_sponge(ts, data, 3);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to write sponge\n", __func__);

	return ret;
}

int sec_ts_check_custom_library(struct sec_ts_data *ts)
{
	u8 data[10] = { 0 };
	int ret = -1;

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_SPONGE_GET_INFO, &data[0], 10);

	input_info(true, &ts->client->dev,
			"%s: (%d) %c%c%c%c, || %02X, %02X, %02X, %02X, || %02X, %02X\n",
			__func__, ret, data[0], data[1], data[2], data[3], data[4],
			data[5], data[6], data[7], data[8], data[9]);

	/* compare model name with device tree */
	if (ts->plat_data->model_name)
		ret = strncmp(data, ts->plat_data->model_name, 4);

	if (ret == 0)
		ts->use_sponge = true;
	else
		ts->use_sponge = false;

	input_err(true, &ts->client->dev, "%s: use %s\n",
			__func__, ts->use_sponge ? "SPONGE" : "VENDOR");

	sec_ts_set_custom_library(ts);

	return ret;
}

static void sec_ts_set_input_prop(struct sec_ts_data *ts, struct input_dev *dev, u8 propbit)
{
	static char sec_ts_phys[64] = { 0 };

	snprintf(sec_ts_phys, sizeof(sec_ts_phys), "%s/input1",
			dev->name);
	dev->phys = sec_ts_phys;
	dev->id.bustype = BUS_I2C;
	dev->dev.parent = &ts->client->dev;

	set_bit(EV_SYN, dev->evbit);
	set_bit(EV_KEY, dev->evbit);
	set_bit(EV_ABS, dev->evbit);
	set_bit(EV_SW, dev->evbit);
	set_bit(BTN_TOUCH, dev->keybit);
	set_bit(BTN_TOOL_FINGER, dev->keybit);
	set_bit(KEY_BLACK_UI_GESTURE, dev->keybit);
	set_bit(KEY_INT_CANCEL, dev->keybit);

	set_bit(propbit, dev->propbit);
	set_bit(KEY_WAKEUP, dev->keybit);

	input_set_abs_params(dev, ABS_MT_POSITION_X, 0, ts->plat_data->max_x, 0, 0);
	input_set_abs_params(dev, ABS_MT_POSITION_Y, 0, ts->plat_data->max_y, 0, 0);
	input_set_abs_params(dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(dev, ABS_MT_TOUCH_MINOR, 0, 255, 0, 0);
	input_set_abs_params(dev, ABS_MT_CUSTOM, 0, 0xFFFFFFFF, 0, 0);
	if (ts->plat_data->support_mt_pressure)
		input_set_abs_params(dev, ABS_MT_PRESSURE, 0, 255, 0, 0);

	if (propbit == INPUT_PROP_POINTER)
		input_mt_init_slots(dev, MAX_SUPPORT_TOUCH_COUNT, INPUT_MT_POINTER);
	else
		input_mt_init_slots(dev, MAX_SUPPORT_TOUCH_COUNT, INPUT_MT_DIRECT);

	input_set_drvdata(dev, ts);
}

static void sec_ts_set_input_prop_proximity(struct sec_ts_data *ts, struct input_dev *dev)
{
	static char sec_ts_phys[64] = { 0 };

	snprintf(sec_ts_phys, sizeof(sec_ts_phys), "%s/input1", dev->name);
	dev->phys = sec_ts_phys;
	dev->id.bustype = BUS_I2C;
	dev->dev.parent = &ts->client->dev;

	set_bit(EV_SYN, dev->evbit);
	set_bit(EV_SW, dev->evbit);

	set_bit(INPUT_PROP_DIRECT, dev->propbit);

	input_set_abs_params(dev, ABS_MT_CUSTOM, 0, 0xFFFFFFFF, 0, 0);
	input_set_drvdata(dev, ts);
}

static int sec_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct sec_ts_data *ts;
	struct sec_ts_plat_data *pdata;
	struct sec_tclm_data *tdata = NULL;
	int ret = 0;
	bool force_update = false;
	bool valid_firmware_integrity = false;
	unsigned char data[5] = { 0 };
	unsigned char deviceID[5] = { 0 };
	unsigned char result = 0;

	input_info(true, &client->dev, "%s\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		input_err(true, &client->dev, "%s: EIO err!\n", __func__);
		return -EIO;
	}

	/* parse dt */
	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
				sizeof(struct sec_ts_plat_data), GFP_KERNEL);

		if (!pdata) {
			input_err(true, &client->dev, "%s: Failed to allocate platform data\n", __func__);
			goto error_allocate_pdata;
		}

		client->dev.platform_data = pdata;

		ret = sec_ts_parse_dt(client);
		if (ret) {
			input_err(true, &client->dev, "%s: Failed to parse dt\n", __func__);
			goto error_allocate_mem;
		}
		tdata = devm_kzalloc(&client->dev,
				sizeof(struct sec_tclm_data), GFP_KERNEL);
		if (!tdata)
			goto error_allocate_tdata;

		sec_tclm_parse_dt(client, tdata);
	} else {
		pdata = client->dev.platform_data;
		if (!pdata) {
			input_err(true, &client->dev, "%s: No platform data found\n", __func__);
			goto error_allocate_pdata;
		}
	}

	if (!pdata->power) {
		input_err(true, &client->dev, "%s: No power contorl found\n", __func__);
		goto error_allocate_mem;
	}

	pdata->pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(pdata->pinctrl))
		input_err(true, &client->dev, "%s: could not get pinctrl\n", __func__);

	ts = kzalloc(sizeof(struct sec_ts_data), GFP_KERNEL);
	if (!ts)
		goto error_allocate_mem;

	ts->client = client;
	ts->plat_data = pdata;
	ts->crc_addr = 0x0001FE00;
	ts->fw_addr = 0x00002000;
	ts->para_addr = 0x18000;
	ts->flash_page_size = SEC_TS_FW_BLK_SIZE_DEFAULT;
	ts->sec_ts_i2c_read = sec_ts_i2c_read;
	ts->sec_ts_i2c_write = sec_ts_i2c_write;
	ts->sec_ts_i2c_write_burst = sec_ts_i2c_write_burst;
	ts->sec_ts_i2c_read_bulk = sec_ts_i2c_read_bulk;
	ts->i2c_burstmax = pdata->i2c_burstmax;
	ts->tdata = tdata;
	if (!ts->tdata)
		goto err_null_tdata;

#ifdef TCLM_CONCEPT
	sec_tclm_initialize(ts->tdata);
	ts->tdata->client = ts->client;
	ts->tdata->tclm_read = sec_tclm_data_read;
	ts->tdata->tclm_write = sec_tclm_data_write;
	ts->tdata->tclm_execute_force_calibration = sec_tclm_execute_force_calibration;
	ts->tdata->tclm_parse_dt = sec_tclm_parse_dt;
#endif

#ifdef USE_POWER_RESET_WORK
	INIT_DELAYED_WORK(&ts->reset_work, sec_ts_reset_work);
#endif
	INIT_DELAYED_WORK(&ts->work_read_info, sec_ts_read_info_work);
	INIT_DELAYED_WORK(&ts->work_print_info, sec_ts_print_info_work);
	INIT_DELAYED_WORK(&ts->work_read_functions, sec_ts_get_touch_function);

	i2c_set_clientdata(client, ts);

	if (gpio_is_valid(ts->plat_data->tsp_id))
		ts->tspid_val = gpio_get_value(ts->plat_data->tsp_id);

	if (gpio_is_valid(ts->plat_data->tsp_icid))
		ts->tspicid_val = gpio_get_value(ts->plat_data->tsp_icid);

	ts->input_dev = input_allocate_device();
	if (!ts->input_dev) {
		input_err(true, &ts->client->dev, "%s: allocate device err!\n", __func__);
		ret = -ENOMEM;
		goto err_allocate_input_dev;
	}

	if (ts->plat_data->support_dex) {
		ts->input_dev_pad = input_allocate_device();
		if (!ts->input_dev_pad) {
			input_err(true, &ts->client->dev, "%s: allocate device err!\n", __func__);
			ret = -ENOMEM;
			goto err_allocate_input_dev_pad;
		}
	}

	if (ts->plat_data->support_ear_detect) {
		ts->input_dev_proximity = input_allocate_device();
		if (!ts->input_dev_proximity) {
			input_err(true, &ts->client->dev, "%s: allocate input_dev_proximity err!\n", __func__);
			ret = -ENOMEM;
			goto err_allocate_input_dev_proximity;
		}
	}

	ts->touch_count = 0;
	ts->sec_ts_i2c_write = sec_ts_i2c_write;
	ts->sec_ts_i2c_read = sec_ts_i2c_read;
	ts->sec_ts_read_sponge = sec_ts_read_from_sponge;
	ts->sec_ts_write_sponge = sec_ts_write_to_sponge;

	mutex_init(&ts->device_mutex);
	mutex_init(&ts->i2c_mutex);
	mutex_init(&ts->eventlock);
	mutex_init(&ts->modechange);
	mutex_init(&ts->sponge_mutex);

	wake_lock_init(&ts->wakelock, WAKE_LOCK_SUSPEND, "tsp_wakelock");
	init_completion(&ts->resume_done);
	complete_all(&ts->resume_done);

	input_info(true, &client->dev, "%s: init resource\n", __func__);

	sec_ts_pinctrl_configure(ts, true);

	/* power enable */
	sec_ts_power(ts, true);
	if (!pdata->regulator_boot_on)
		sec_ts_delay(70);
	ts->power_status = SEC_TS_STATE_POWER_ON;
	ts->tdata->external_factory = false;

	sec_ts_wait_for_ready(ts, SEC_TS_ACK_BOOT_COMPLETE);

	input_info(true, &client->dev, "%s: power enable\n", __func__);

	ret = sec_ts_i2c_read(ts, SEC_TS_READ_DEVICE_ID, deviceID, 5);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: failed to read device ID(%d)\n", __func__, ret);
	else
		input_info(true, &ts->client->dev,
				"%s: TOUCH DEVICE ID : %02X, %02X, %02X, %02X, %02X\n", __func__,
				deviceID[0], deviceID[1], deviceID[2], deviceID[3], deviceID[4]);

	ret = sec_ts_i2c_read(ts, SEC_TS_READ_FIRMWARE_INTEGRITY, &result, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to integrity check (%d)\n", __func__, ret);
	} else {
		if (result & 0x80) {
			valid_firmware_integrity = true;
		} else if (result & 0x40) {
			valid_firmware_integrity = false;
			input_err(true, &ts->client->dev, "%s: invalid firmware (0x%x)\n", __func__, result);
		} else {
			valid_firmware_integrity = false;
			input_err(true, &ts->client->dev, "%s: invalid integrity result (0x%x)\n", __func__, result);
		}
	}

	ret = sec_ts_i2c_read(ts, SEC_TS_READ_BOOT_STATUS, &data[0], 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read sub id(%d)\n",
				__func__, ret);
	} else {
		ret = sec_ts_i2c_read(ts, SEC_TS_READ_TS_STATUS, &data[1], 4);
		if (ret < 0) {
			input_err(true, &ts->client->dev,
					"%s: failed to touch status(%d)\n",
					__func__, ret);
		}
	}
	input_info(true, &ts->client->dev,
			"%s: TOUCH STATUS : %02X || %02X, %02X, %02X, %02X\n",
			__func__, data[0], data[1], data[2], data[3], data[4]);

	if (data[0] == SEC_TS_STATUS_BOOT_MODE)
		ts->checksum_result = 1;

	if ((data[0] == SEC_TS_STATUS_APP_MODE && data[2] == TOUCH_SYSTEM_MODE_FLASH) ||
			(valid_firmware_integrity == false))
		force_update = true;
	else
		force_update = false;

	ret = sec_ts_firmware_update_on_probe(ts, force_update);
	if (ret < 0)
		goto err_init;

	ret = sec_ts_read_information(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: fail to read information 0x%x\n", __func__, ret);
		goto err_init;
	}

	ts->touch_functions |= SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC;
	ret = sec_ts_set_touch_function(ts);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to send touch func_mode command", __func__);

	/* Sense_on */
	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: fail to write Sense_on\n", __func__);
		goto err_init;
	}

	ts->pFrame = kzalloc(ts->tx_count * ts->rx_count * 2, GFP_KERNEL);
	if (!ts->pFrame) {
		ret = -ENOMEM;
		goto err_allocate_frame;
	}

	sec_ts_init_proc(ts);

	if (ts->plat_data->support_dex) {
		ts->input_dev_pad->name = "sec_touchpad";
		sec_ts_set_input_prop(ts, ts->input_dev_pad, INPUT_PROP_POINTER);
	}

	if (ts->plat_data->support_ear_detect) {
		ts->input_dev_proximity->name = "sec_touchproximity";
		sec_ts_set_input_prop_proximity(ts, ts->input_dev_proximity);
	}

	ts->input_dev->name = "sec_touchscreen";
	sec_ts_set_input_prop(ts, ts->input_dev, INPUT_PROP_DIRECT);
#ifdef USE_OPEN_CLOSE
	ts->input_dev->open = sec_ts_input_open;
	ts->input_dev->close = sec_ts_input_close;
#endif
	ts->input_dev_touch = ts->input_dev;

	ret = input_register_device(ts->input_dev);
	if (ret) {
		input_err(true, &ts->client->dev, "%s: Unable to register %s input device\n", __func__, ts->input_dev->name);
		goto err_input_register_device;
	}

	if (ts->plat_data->support_dex) {
		ret = input_register_device(ts->input_dev_pad);
		if (ret) {
			input_err(true, &ts->client->dev, "%s: Unable to register %s input device\n", __func__, ts->input_dev_pad->name);
			goto err_input_pad_register_device;
		}
	}

	if (ts->plat_data->support_ear_detect) {
		ret = input_register_device(ts->input_dev_proximity);
		if (ret) {
			input_err(true, &ts->client->dev, "%s: Unable to register %s input device\n", __func__, ts->input_dev_proximity->name);
			goto err_input_proximity_register_device;
		}
	}

	input_info(true, &ts->client->dev, "%s: request_irq = %d\n", __func__, client->irq);

	ret = request_threaded_irq(client->irq, NULL, sec_ts_irq_thread,
			ts->plat_data->irq_type, SEC_TS_I2C_NAME, ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: Unable to request threaded irq\n", __func__);
		goto err_irq;
	}

#ifdef CONFIG_SAMSUNG_TUI
	tsp_info = ts;
#endif
	/* need remove below resource @ remove driver */
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	sec_ts_raw_device_init(ts);
#endif
	sec_ts_fn_init(ts);

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (sysfs_create_group(&ts->input_dev->dev.kobj, &secure_attr_group) < 0)
		input_err(true, &ts->client->dev, "%s: do not make secure group\n", __func__);
	else
		secure_touch_init(ts);
#endif
	sec_ts_check_custom_library(ts);

	device_init_wakeup(&client->dev, true);

	schedule_delayed_work(&ts->work_read_info, msecs_to_jiffies(50));

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
	dump_callbacks.inform_dump = dump_tsp_log;
	INIT_DELAYED_WORK(&ts->ghost_check, sec_ts_check_rawdata);
	p_ghost_check = &ts->ghost_check;
#endif

	/* register dev for ltp */
	sec_ts_ioctl_init(ts);

	ts_dup = ts;
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	sec_secure_touch_register(ts, ts->plat_data->ss_touch_num, &ts->input_dev->dev.kobj);
#endif

	ts->psy = power_supply_get_by_name("battery");
	if (!ts->psy)
		input_err(true, &ts->client->dev, "%s: Cannot find power supply\n", __func__);

	ts->probe_done = true;

	input_err(true, &ts->client->dev, "%s: done\n", __func__);
	input_log_fix();

	return 0;

	/* need to be enabled when new goto statement is added */
#if 0
	sec_ts_fn_remove(ts);
	free_irq(client->irq, ts);
#endif
err_irq:
	if (ts->plat_data->support_ear_detect) {
		input_unregister_device(ts->input_dev_proximity);
		ts->input_dev_proximity = NULL;
	}
err_input_proximity_register_device:
	if (ts->plat_data->support_dex) {
		input_unregister_device(ts->input_dev_pad);
		ts->input_dev_pad = NULL;
	}
err_input_pad_register_device:
	input_unregister_device(ts->input_dev);
	ts->input_dev = NULL;
	ts->input_dev_touch = NULL;
err_input_register_device:
	kfree(ts->pFrame);
err_allocate_frame:
err_init:
	wake_lock_destroy(&ts->wakelock);
	sec_ts_power(ts, false);
	if (ts->plat_data->support_ear_detect) {
		if (ts->input_dev_proximity)
			input_free_device(ts->input_dev_proximity);
	}
err_allocate_input_dev_proximity:
	if (ts->plat_data->support_dex) {
		if (ts->input_dev_pad)
			input_free_device(ts->input_dev_pad);
	}
err_allocate_input_dev_pad:
	if (ts->input_dev)
		input_free_device(ts->input_dev);
err_allocate_input_dev:
err_null_tdata:
	kfree(ts);

error_allocate_mem:
	if (gpio_is_valid(pdata->irq_gpio))
		gpio_free(pdata->irq_gpio);
	if (gpio_is_valid(pdata->tsp_id))
		gpio_free(pdata->tsp_id);
	if (gpio_is_valid(pdata->tsp_icid))
		gpio_free(pdata->tsp_icid);

error_allocate_tdata:
error_allocate_pdata:
	if (ret == -ECONNREFUSED)
		sec_ts_delay(100);
	ret = -ENODEV;
#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
	p_ghost_check = NULL;
#endif
	ts_dup = NULL;
#ifdef CONFIG_SAMSUNG_TUI
	tsp_info = NULL;
#endif
	input_err(true, &client->dev, "%s: failed(%d)\n", __func__, ret);
	input_log_fix();
	return ret;
}

void sec_ts_unlocked_release_all_finger(struct sec_ts_data *ts)
{
	int i;
	char location[SEC_TS_LOCATION_DETECT_SIZE] = { 0, };

	for (i = 0; i < MAX_SUPPORT_TOUCH_COUNT; i++) {
		input_mt_slot(ts->input_dev, i);
		input_report_abs(ts->input_dev, ABS_MT_CUSTOM, 0);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);

		if ((ts->coord[i].action == SEC_TS_COORDINATE_ACTION_PRESS) ||
				(ts->coord[i].action == SEC_TS_COORDINATE_ACTION_MOVE)) {
			location_detect(ts, location, ts->coord[i].x, ts->coord[i].y);

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			input_info(true, &ts->client->dev,
					"[RA] tID:%d loc:%s dd:%d,%d mc:%d tc:%d lx:%d ly:%d p:%d noise:(%x,%d%d)\n",
					i, location,
					ts->coord[i].x - ts->coord[i].p_x,
					ts->coord[i].y - ts->coord[i].p_y,
					ts->coord[i].mcount, ts->touch_count,
					ts->coord[i].x, ts->coord[i].y,
					ts->coord[i].palm_count,
					ts->coord[i].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status);
#else
			input_info(true, &ts->client->dev,
					"[RA] tID:%d loc:%s dd:%d,%d mc:%d tc:%d p:%d noise:(%x,%d%d)\n",
					i, location,
					ts->coord[i].x - ts->coord[i].p_x,
					ts->coord[i].y - ts->coord[i].p_y,
					ts->coord[i].mcount, ts->touch_count,
					ts->coord[i].palm_count,
					ts->coord[i].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status);
#endif
			ts->coord[i].action = SEC_TS_COORDINATE_ACTION_RELEASE;
		}
		ts->coord[i].mcount = 0;
		ts->coord[i].palm_count = 0;
		ts->coord[i].noise_level = 0;
		ts->coord[i].max_strength = 0;
		ts->coord[i].hover_id_num = 0;
	}

	input_mt_slot(ts->input_dev, 0);

	input_report_key(ts->input_dev, BTN_TOUCH, false);
	input_report_key(ts->input_dev, BTN_TOOL_FINGER, false);
	ts->touch_count = 0;
	ts->check_multi = 0;

	input_sync(ts->input_dev);
}

void sec_ts_locked_release_all_finger(struct sec_ts_data *ts)
{
	int i;
	char location[6] = { 0, };

	mutex_lock(&ts->eventlock);

	for (i = 0; i < MAX_SUPPORT_TOUCH_COUNT; i++) {
		input_mt_slot(ts->input_dev, i);
		input_report_abs(ts->input_dev, ABS_MT_CUSTOM, 0);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);

		if ((ts->coord[i].action == SEC_TS_COORDINATE_ACTION_PRESS) ||
				(ts->coord[i].action == SEC_TS_COORDINATE_ACTION_MOVE)) {
			location_detect(ts, location, ts->coord[i].x, ts->coord[i].y);
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			input_info(true, &ts->client->dev,
					"[RA] tID:%d loc:%s dd:%d,%d mc:%d tc:%d lx:%d ly:%d p:%d noise:(%x,%d%d)\n",
					i, location,
					ts->coord[i].x - ts->coord[i].p_x,
					ts->coord[i].y - ts->coord[i].p_y,
					ts->coord[i].mcount, ts->touch_count,
					ts->coord[i].x, ts->coord[i].y,
					ts->coord[i].palm_count,
					ts->coord[i].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status);
#else
			input_info(true, &ts->client->dev,
					"[RA] tID:%d loc:%s dd:%d,%d mc:%d tc:%d p:%d noise:(%x,%d%d)\n",
					i, location,
					ts->coord[i].x - ts->coord[i].p_x,
					ts->coord[i].y - ts->coord[i].p_y,
					ts->coord[i].mcount, ts->touch_count,
					ts->coord[i].palm_count,
					ts->coord[i].noise_status, ts->touch_noise_status, ts->touch_pre_noise_status);
#endif
			ts->coord[i].action = SEC_TS_COORDINATE_ACTION_RELEASE;
		}
		ts->coord[i].mcount = 0;
		ts->coord[i].palm_count = 0;
		ts->coord[i].noise_level = 0;
		ts->coord[i].max_strength = 0;
		ts->coord[i].hover_id_num = 0;
	}

	input_mt_slot(ts->input_dev, 0);

	input_report_key(ts->input_dev, BTN_TOUCH, false);
	input_report_key(ts->input_dev, BTN_TOOL_FINGER, false);
	ts->touch_count = 0;
	ts->check_multi = 0;

	input_sync(ts->input_dev);

	mutex_unlock(&ts->eventlock);
}

#ifdef USE_POWER_RESET_WORK
static void sec_ts_reset_work(struct work_struct *work)
{
	struct sec_ts_data *ts = container_of(work, struct sec_ts_data,
			reset_work.work);
	int ret;

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	if (atomic_read(&ts->secure_enabled) == SECURE_TOUCH_ENABLE) {
		input_err(true, &ts->client->dev, "%s: secure touch enabled\n", __func__);
		return;
	}
#endif
	if (ts->reset_is_on_going) {
		input_err(true, &ts->client->dev, "%s: reset is ongoing\n", __func__);
		return;
	}

	mutex_lock(&ts->modechange);
	wake_lock(&ts->wakelock);

	ts->reset_is_on_going = true;
	input_info(true, &ts->client->dev, "%s\n", __func__);

	sec_ts_stop_device(ts);

	sec_ts_delay(30);

	ret = sec_ts_start_device(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to reset, ret:%d\n", __func__, ret);
		ts->reset_is_on_going = false;
		cancel_delayed_work(&ts->reset_work);
		schedule_delayed_work(&ts->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
		mutex_unlock(&ts->modechange);

		if (ts->debug_flag & SEC_TS_DEBUG_SEND_UEVENT) {
			char result[32];

			snprintf(result, sizeof(result), "RESULT=RESET");
			sec_cmd_send_event_to_user(&ts->sec, NULL, result);
		}

		wake_unlock(&ts->wakelock);

		return;
	}

	if (ts->input_dev_touch->disabled) {
		input_err(true, &ts->client->dev, "%s: call input_close\n", __func__);

		if (ts->lowpower_mode || ts->ed_enable) {
			ret = sec_ts_set_lowpowermode(ts, TO_LOWPOWER_MODE);
			if (ret < 0) {
				input_err(true, &ts->client->dev, "%s: failed to reset, ret:%d\n", __func__, ret);
				ts->reset_is_on_going = false;
				cancel_delayed_work(&ts->reset_work);
				schedule_delayed_work(&ts->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
				mutex_unlock(&ts->modechange);
				wake_unlock(&ts->wakelock);
				return;
			}

			sec_ts_set_aod_rect(ts);
		} else {
			sec_ts_stop_device(ts);
		}
	}

	ts->reset_is_on_going = false;
	mutex_unlock(&ts->modechange);

	if (ts->power_status == SEC_TS_STATE_POWER_ON) {
		if (ts->fix_active_mode)
			sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);
	}

	if (ts->debug_flag & SEC_TS_DEBUG_SEND_UEVENT) {
		char result[32];

		snprintf(result, sizeof(result), "RESULT=RESET");
		sec_cmd_send_event_to_user(&ts->sec, NULL, result);
	}

	wake_unlock(&ts->wakelock);
}
#endif

static void sec_ts_print_info_work(struct work_struct *work)
{
	struct sec_ts_data *ts = container_of(work, struct sec_ts_data,
			work_print_info.work);
	sec_ts_print_info(ts);

	if (ts->sec.cmd_is_running) {
		input_err(true, &ts->client->dev, "%s: skip sec_ts_set_temp, cmd running\n", __func__);	
	} else {
		if (ts->touch_count) {
			ts->tsp_temp_data_skip = true;
			input_err(true, &ts->client->dev, "%s: skip sec_ts_set_temp, t_cnt(%d)\n", __func__, ts->touch_count);	
		} else {
			ts->tsp_temp_data_skip = false;
			sec_ts_set_temp(ts, false);
		}
	}
	
	schedule_delayed_work(&ts->work_print_info, msecs_to_jiffies(TOUCH_PRINT_INFO_DWORK_TIME));
}

static void sec_ts_read_info_work(struct work_struct *work)
{
	struct sec_ts_data *ts = container_of(work, struct sec_ts_data,
			work_read_info.work);
#ifdef TCLM_CONCEPT
	int ret;

	disable_irq(ts->client->irq);

	ret = sec_tclm_check_cal_case(ts->tdata);
	input_info(true, &ts->client->dev, "%s: sec_tclm_check_cal_case ret: %d \n", __func__, ret);

	enable_irq(ts->client->irq);
#endif
	ts->nv = get_tsp_nvm_data(ts, SEC_TS_NVM_OFFSET_FAC_RESULT);
	input_info(true, &ts->client->dev, "%s: fac_nv:%02X\n", __func__, ts->nv);
	input_log_fix();

	sec_ts_run_rawdata_all(ts, false);

	/* read cmoffset & fail history data at booting time */
	if (ts->proc_cmoffset_size) {
		get_cmoffset_dump_all(ts, ts->cmoffset_sdc_proc, OFFSET_FW_SDC);
		get_cmoffset_dump_all(ts, ts->cmoffset_sub_proc, OFFSET_FW_SUB);
		get_cmoffset_dump_all(ts, ts->cmoffset_main_proc, OFFSET_FW_MAIN);
	} else {
		input_err(true, &ts->client->dev, "%s: read cmoffset fail : alloc fail\n", __func__);
	}

	if (ts->proc_fail_hist_size) {
		get_selftest_fail_hist_dump_all(ts, ts->fail_hist_sdc_proc, OFFSET_FW_SDC);
		get_selftest_fail_hist_dump_all(ts, ts->fail_hist_sub_proc, OFFSET_FW_SUB);
		get_selftest_fail_hist_dump_all(ts, ts->fail_hist_main_proc, OFFSET_FW_MAIN);
	} else {
		input_err(true, &ts->client->dev, "%s: read fail-history fail : alloc fail\n", __func__);
	}

	ts->info_work_done = true;

	if (ts->shutdown_is_on_going) {
		input_err(true, &ts->client->dev, "%s done, do not run work\n", __func__);
		return;
	}

	schedule_work(&ts->work_print_info.work);

}

int sec_ts_set_lowpowermode(struct sec_ts_data *ts, u8 mode)
{
	int ret;
	int retrycnt = 0;
	char para = 0;

	input_err(true, &ts->client->dev, "%s: %s(%X)\n", __func__,
			mode == TO_LOWPOWER_MODE ? "ENTER" : "EXIT", ts->lowpower_mode);

	if (mode) {
		if (ts->prox_power_off) 
			sec_ts_set_prox_power_off(ts, 1);

		sec_ts_set_utc_sponge(ts);

		ret = sec_ts_set_custom_library(ts);
		if (ret < 0)
			goto i2c_error;
	} else {
		schedule_work(&ts->work_read_functions.work);
		sec_ts_set_prox_power_off(ts, 0);
	}

retry_pmode:
	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &mode, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed\n", __func__);
		goto i2c_error;
	}

	sec_ts_delay(50);

	ret = sec_ts_i2c_read(ts, SEC_TS_CMD_SET_POWER_MODE, &para, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: read power mode failed!\n", __func__);
		goto i2c_error;
	} else {
		input_info(true, &ts->client->dev, "%s: power mode - write(%d) read(%d)\n", __func__, mode, para);
	}

	if (mode != para) {
		retrycnt++;
		ts->mode_change_failed_count++;
		if (retrycnt < 5)
			goto retry_pmode;
	}

	if (mode) {
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: i2c write clear event failed\n", __func__);
			goto i2c_error;
		}
	}

	sec_ts_locked_release_all_finger(ts);

	if (device_may_wakeup(&ts->client->dev)) {
		if (mode)
			enable_irq_wake(ts->client->irq);
		else
			disable_irq_wake(ts->client->irq);
	}

	if (mode == TO_LOWPOWER_MODE)
		ts->power_status = SEC_TS_STATE_LPM;
	else
		ts->power_status = SEC_TS_STATE_POWER_ON;

i2c_error:
	input_info(true, &ts->client->dev, "%s: end %d\n", __func__, ret);

	return ret;
}

#ifdef USE_OPEN_CLOSE
static int sec_ts_input_open(struct input_dev *dev)
{
	struct sec_ts_data *ts = input_get_drvdata(dev);
	int ret;

	if (!ts->info_work_done) {
		input_err(true, &ts->client->dev, "%s not finished info work\n", __func__);
		return 0;
	}

	mutex_lock(&ts->modechange);

	ts->input_closed = false;
	ts->prox_power_off = 0;

#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	secure_touch_stop(ts, 0);
#endif

	if (ts->power_status == SEC_TS_STATE_LPM) {
#ifdef USE_RESET_EXIT_LPM
		schedule_delayed_work(&ts->reset_work, msecs_to_jiffies(TOUCH_RESET_DWORK_TIME));
#else
		sec_ts_set_lowpowermode(ts, TO_TOUCH_MODE);
		sec_ts_set_grip_type(ts, ONLY_EDGE_HANDLER);
#endif
	} else {
		ret = sec_ts_start_device(ts);
		if (ret < 0)
			input_err(true, &ts->client->dev, "%s: Failed to start device\n", __func__);
	}

	if (ts->fix_active_mode)
		sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);

	sec_ts_set_temp(ts, true);

	mutex_unlock(&ts->modechange);

	cancel_delayed_work(&ts->work_print_info);
	ts->print_info_cnt_open = 0;
	ts->print_info_cnt_release = 0;
	schedule_work(&ts->work_print_info.work);
	return 0;
}

static void sec_ts_input_close(struct input_dev *dev)
{
	struct sec_ts_data *ts = input_get_drvdata(dev);

	if (!ts->info_work_done) {
		input_err(true, &ts->client->dev, "%s not finished info work\n", __func__);
		return;
	}
	if (ts->shutdown_is_on_going) {
		input_err(true, &ts->client->dev, "%s shutdown was called\n", __func__);
		return;
	}

	mutex_lock(&ts->modechange);

	ts->input_closed = true;
	ts->sip_mode = 0;

#ifdef TCLM_CONCEPT
	sec_tclm_debug_info(ts->tdata);
#endif
#ifdef MINORITY_REPORT
	minority_report_sync_latest_value(ts);
#endif
	cancel_delayed_work(&ts->work_print_info);
	sec_ts_print_info(ts);
#ifdef CONFIG_INPUT_SEC_SECURE_TOUCH
	secure_touch_stop(ts, 1);
#endif

#ifdef USE_POWER_RESET_WORK
	cancel_delayed_work(&ts->reset_work);
#endif

	if (ts->lowpower_mode || ts->ed_enable)
		sec_ts_set_lowpowermode(ts, TO_LOWPOWER_MODE);
	else
		sec_ts_stop_device(ts);

	mutex_unlock(&ts->modechange);
}
#endif

static int sec_ts_remove(struct i2c_client *client)
{
	struct sec_ts_data *ts = i2c_get_clientdata(client);

	input_info(true, &ts->client->dev, "%s\n", __func__);
	ts->shutdown_is_on_going = true;

	sec_ts_ioctl_remove(ts);

	cancel_delayed_work_sync(&ts->work_read_info);
	cancel_delayed_work_sync(&ts->work_print_info);
	cancel_delayed_work_sync(&ts->work_read_functions);
	disable_irq_nosync(ts->client->irq);
	free_irq(ts->client->irq, ts);
	input_info(true, &ts->client->dev, "%s: irq disabled\n", __func__);

#ifdef USE_POWER_RESET_WORK
	cancel_delayed_work_sync(&ts->reset_work);
	flush_delayed_work(&ts->reset_work);

	input_info(true, &ts->client->dev, "%s: flush queue\n", __func__);

#endif
	sec_ts_fn_remove(ts);

#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
	p_ghost_check = NULL;
#endif
	device_init_wakeup(&client->dev, false);
	wake_lock_destroy(&ts->wakelock);

	ts->lowpower_mode = false;
	ts->probe_done = false;

	if (ts->plat_data->support_dex) {
		input_mt_destroy_slots(ts->input_dev_pad);
		input_unregister_device(ts->input_dev_pad);
	}
	if (ts->plat_data->support_ear_detect) {
		input_mt_destroy_slots(ts->input_dev_proximity);
		input_unregister_device(ts->input_dev_proximity);
	}

	ts->input_dev = ts->input_dev_touch;
	input_mt_destroy_slots(ts->input_dev);
	input_unregister_device(ts->input_dev);

	ts->input_dev_pad = NULL;
	ts->input_dev = NULL;
	ts->input_dev_touch = NULL;
	ts_dup = NULL;
	ts->plat_data->power(ts, false);

#ifdef CONFIG_SAMSUNG_TUI
	tsp_info = NULL;
#endif

	kfree(ts);
	return 0;
}

static void sec_ts_shutdown(struct i2c_client *client)
{
	struct sec_ts_data *ts = i2c_get_clientdata(client);

	input_info(true, &ts->client->dev, "%s\n", __func__);

	sec_ts_remove(client);
}

int sec_ts_stop_device(struct sec_ts_data *ts)
{
	input_info(true, &ts->client->dev, "%s\n", __func__);

	mutex_lock(&ts->device_mutex);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: already power off\n", __func__);
		goto out;
	}

	disable_irq(ts->client->irq);

	ts->power_status = SEC_TS_STATE_POWER_OFF;

	if (ts->prox_power_off) {
		input_report_key(ts->input_dev, KEY_INT_CANCEL, 1);
		input_sync(ts->input_dev);
		input_report_key(ts->input_dev, KEY_INT_CANCEL, 0);
		input_sync(ts->input_dev);
	}

	sec_ts_locked_release_all_finger(ts);

	ts->plat_data->power(ts, false);

	sec_ts_pinctrl_configure(ts, false);

out:
	mutex_unlock(&ts->device_mutex);
	return 0;
}

int sec_ts_start_device(struct sec_ts_data *ts)
{
	int ret = -1;

	input_info(true, &ts->client->dev, "%s\n", __func__);

	sec_ts_pinctrl_configure(ts, true);

	mutex_lock(&ts->device_mutex);

	if (ts->power_status == SEC_TS_STATE_POWER_ON) {
		input_err(true, &ts->client->dev, "%s: already power on\n", __func__);
		goto out;
	}

	sec_ts_locked_release_all_finger(ts);

	ts->plat_data->power(ts, true);
	sec_ts_delay(70);
	ts->power_status = SEC_TS_STATE_POWER_ON;
	ts->touch_noise_status = 0;
	ts->touch_pre_noise_status = 0;

	ret = sec_ts_wait_for_ready(ts, SEC_TS_ACK_BOOT_COMPLETE);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: Failed to wait_for_ready\n", __func__);
		goto err;
	}

	sec_ts_reinit(ts);

#ifdef CONFIG_INPUT_WACOM
	/* spen mode for note models */
	if (ts->spen_mode_val != SPEN_DISABLE_MODE) {
		input_info(true, &ts->client->dev, "%s: spen_mode: 0x%X\n", __func__, ts->spen_mode_val);

		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_SCAN_MODE, (u8 *)&ts->spen_mode_val, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: Failed to send spen mode", __func__);
		}
	}
#endif

err:
	/* Sense_on */
	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: fail to write Sense_on\n", __func__);

	enable_irq(ts->client->irq);
out:
	mutex_unlock(&ts->device_mutex);
	return ret;
}

#ifdef CONFIG_PM
static int sec_ts_pm_suspend(struct device *dev)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);
#if 0
	int retval;

	if (ts->input_dev) {
		retval = mutex_lock_interruptible(&ts->input_dev->mutex);
		if (retval) {
			input_err(true, &ts->client->dev,
					"%s : mutex error\n", __func__);
			goto out;
		}

		if (!ts->input_dev->disabled) {
			ts->input_dev->disabled = true;
			if (ts->input_dev->users && ts->input_dev->close) {
				input_err(true, &ts->client->dev,
						"%s called without input_close\n",
						__func__);
				ts->input_dev->close(ts->input_dev);
			}
			ts->input_dev->users = 0;
		}

		mutex_unlock(&ts->input_dev->mutex);
	}

out:
#endif
	if (ts->lowpower_mode)
		reinit_completion(&ts->resume_done);

	return 0;
}

static int sec_ts_pm_resume(struct device *dev)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);

	if (ts->lowpower_mode)
		complete_all(&ts->resume_done);

	return 0;
}
#endif

#ifdef CONFIG_SAMSUNG_TUI
extern int stui_i2c_lock(struct i2c_adapter *adap);
extern int stui_i2c_unlock(struct i2c_adapter *adap);
extern void epen_disable_mode(int mode);

int stui_tsp_enter(void)
{
	int ret = 0;

	if (!tsp_info)
		return -EINVAL;

	/* Disable wacom interrupt during tui */
	epen_disable_mode(1);

	disable_irq(tsp_info->client->irq);
	sec_ts_unlocked_release_all_finger(tsp_info);

	ret = stui_i2c_lock(tsp_info->client->adapter);
	if (ret) {
		pr_err("[STUI] stui_i2c_lock failed : %d\n", ret);
		enable_irq(tsp_info->client->irq);
		return -1;
	}

	return 0;
}

int stui_tsp_exit(void)
{
	int ret = 0;

	if (!tsp_info)
		return -EINVAL;

	ret = stui_i2c_unlock(tsp_info->client->adapter);
	if (ret)
		pr_err("[STUI] stui_i2c_unlock failed : %d\n", ret);

	enable_irq(tsp_info->client->irq);

	/* Enable wacom interrupt after tui exit */
	epen_disable_mode(0);

	return ret;
}
#endif

static const struct i2c_device_id sec_ts_id[] = {
	{ SEC_TS_I2C_NAME, 0 },
	{ },
};

#ifdef CONFIG_PM
static const struct dev_pm_ops sec_ts_dev_pm_ops = {
	.suspend = sec_ts_pm_suspend,
	.resume = sec_ts_pm_resume,
};
#endif

#ifdef CONFIG_OF
static const struct of_device_id sec_ts_match_table[] = {
	{ .compatible = "sec,sec_ts",},
	{ },
};
#else
#define sec_ts_match_table NULL
#endif

static struct i2c_driver sec_ts_driver = {
	.probe		= sec_ts_probe,
	.remove		= sec_ts_remove,
	.shutdown	= sec_ts_shutdown,
	.id_table	= sec_ts_id,
	.driver = {
		.owner	= THIS_MODULE,
		.name	= SEC_TS_I2C_NAME,
#ifdef CONFIG_OF
		.of_match_table = sec_ts_match_table,
#endif
#ifdef CONFIG_PM
		.pm = &sec_ts_dev_pm_ops,
#endif
	},
};

static int __init sec_ts_init(void)
{
#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge == 1) {
		pr_err("%s %s: Do not load driver due to : lpm %d\n",
				SECLOG, __func__, lpcharge);
		return -ENODEV;
	}
#endif
	pr_err("%s %s\n", SECLOG, __func__);

	return i2c_add_driver(&sec_ts_driver);
}

static void __exit sec_ts_exit(void)
{
	i2c_del_driver(&sec_ts_driver);
}

MODULE_AUTHOR("Hyobae, Ahn<hyobae.ahn@samsung.com>");
MODULE_DESCRIPTION("Samsung Electronics TouchScreen driver");
MODULE_LICENSE("GPL");

module_init(sec_ts_init);
module_exit(sec_ts_exit);
