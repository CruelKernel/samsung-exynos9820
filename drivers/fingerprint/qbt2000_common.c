/*
 * Copyright (C) 2018 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "fingerprint.h"
#include "qbt2000_common.h"

static struct qbt2000_drvdata *g_data = NULL;

/*
 * struct ipc_msg_type_to_fw_event -
 *      entry in mapping between an IPC message type to a firmware event
 * @msg_type - IPC message type, as reported by firmware
 * @fw_event - corresponding firmware event code to report to driver client
 */
struct ipc_msg_type_to_fw_event {
	uint32_t msg_type;
	enum qbt2000_fw_event fw_event;
};

/* mapping between firmware IPC message types to HLOS firmware events */
struct ipc_msg_type_to_fw_event g_msg_to_event[] = {
	{IPC_MSG_ID_CBGE_REQUIRED, FW_EVENT_CBGE_REQUIRED},
	{IPC_MSG_ID_FINGER_ON_SENSOR, FW_EVENT_FINGER_DOWN},
	{IPC_MSG_ID_FINGER_OFF_SENSOR, FW_EVENT_FINGER_UP},
};

static ssize_t qbt2000_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t qbt2000_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t qbt2000_adm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", DETECT_ADM);
}

static ssize_t qbt2000_bfs_values_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct qbt2000_drvdata *drvdata = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "\"FP_SPICLK\":\"%d\"\n",
			drvdata->spi_speed);
}

static ssize_t qbt2000_type_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct qbt2000_drvdata *drvdata = dev_get_drvdata(dev);

	pr_info("%s\n", sensor_status[drvdata->sensortype + 2]);
	return snprintf(buf, PAGE_SIZE, "%d\n", drvdata->sensortype);
}

static ssize_t qbt2000_position_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct qbt2000_drvdata *drvdata = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%s\n", drvdata->sensor_position);
}

static ssize_t qbt2000_cbgecnt_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct qbt2000_drvdata *drvdata = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%d\n", drvdata->cbge_count);
}

static ssize_t qbt2000_cbgecnt_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	struct qbt2000_drvdata *drvdata = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "c")) {
		drvdata->cbge_count = 0;
#ifdef QBT2000_AVOID_NOISE
		drvdata->ignored_cbge_count = 0;
#endif
		pr_info("initialization is done\n");
	} else if (sysfs_streq(buf, "wuhb")) {
	/* For User HwModuleTest IntTest */
		drvdata->wuhb_test_flag = 1;
		drvdata->wuhb_test_result = 0;

		if (drvdata->fd_gpio.gpio) {
			if (drvdata->enabled_wuhb) {
				drvdata->wuhb_test_flag = 0;
				drvdata->wuhb_test_result = -1;
				pr_err("wuhb test procedure can not perform.\n");
			} else {
				enable_irq(drvdata->fd_gpio.irq);
				drvdata->enabled_wuhb = true;
				pr_info("wuhb test start.flag:%d,result:%d,en:%d\n",
					drvdata->wuhb_test_flag,
					drvdata->wuhb_test_result,
					drvdata->enabled_wuhb);
			}
		} else {
			drvdata->wuhb_test_flag = 0;
			drvdata->wuhb_test_result = -1;
			pr_err("fd_gpio does not supports this hw rev.\n");
		}
	}
	return size;
}

static ssize_t qbt2000_intcnt_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct qbt2000_drvdata *drvdata = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%d\n", drvdata->wuhb_count);
}

static ssize_t qbt2000_intcnt_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	struct qbt2000_drvdata *drvdata = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "c")) {
		drvdata->wuhb_count = 0;
		pr_info("initialization is done\n");
	}
	return size;
}

static ssize_t qbt2000_resetcnt_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct qbt2000_drvdata *drvdata = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%d\n", drvdata->reset_count);
}

static ssize_t qbt2000_resetcnt_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	struct qbt2000_drvdata *drvdata = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "c")) {
		drvdata->reset_count = 0;
		pr_info("initialization is done\n");
	}
	return size;
}

static ssize_t qbt2000_wuhbtest_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct qbt2000_drvdata *drvdata = dev_get_drvdata(dev);
	int rc = drvdata->wuhb_test_result;

	if (drvdata->wuhb_test_flag == 1) {
		if (drvdata->enabled_wuhb) {
			disable_irq(drvdata->fd_gpio.irq);
			drvdata->enabled_wuhb = false;
		}
	}
	drvdata->wuhb_test_result = 0;
	drvdata->wuhb_test_flag = 0;
	pr_info("wuhb test complete.rc=%d,wuhb_en:%d\n", rc, drvdata->enabled_wuhb);
	return snprintf(buf, PAGE_SIZE, "%d\n", rc);
}

static DEVICE_ATTR(bfs_values, 0444, qbt2000_bfs_values_show, NULL);
static DEVICE_ATTR(type_check, 0444, qbt2000_type_check_show, NULL);
static DEVICE_ATTR(vendor, 0444, qbt2000_vendor_show, NULL);
static DEVICE_ATTR(name, 0444, qbt2000_name_show, NULL);
static DEVICE_ATTR(adm, 0444, qbt2000_adm_show, NULL);
static DEVICE_ATTR(position, 0444, qbt2000_position_show, NULL);
static DEVICE_ATTR(cbgecnt, 0664, qbt2000_cbgecnt_show, qbt2000_cbgecnt_store);
static DEVICE_ATTR(intcnt, 0664, qbt2000_intcnt_show, qbt2000_intcnt_store);
static DEVICE_ATTR(resetcnt, 0664, qbt2000_resetcnt_show, qbt2000_resetcnt_store);
static DEVICE_ATTR(wuhbtest, 0444, qbt2000_wuhbtest_show, NULL);

static struct device_attribute *fp_attrs[] = {
	&dev_attr_bfs_values,
	&dev_attr_type_check,
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_adm,
	&dev_attr_position,
	&dev_attr_cbgecnt,
	&dev_attr_intcnt,
	&dev_attr_resetcnt,
	&dev_attr_wuhbtest,
	NULL,
};

#if defined(ENABLE_SENSORS_FPRINT_SECURE)
int fpsensor_goto_suspend = 0;

int fps_resume_set(void) {
	int rc = 0;

	if (fpsensor_goto_suspend)
		fpsensor_goto_suspend = 0;
	return rc;
}
#endif

#ifdef QBT2000_AVOID_NOISE
static int qbt2000_noise_control(struct qbt2000_drvdata *drvdata, int control)
{
	int retry = 3;
	int rc = 0;

	if (control == QBT2000_NOISE_UNBLOCK) {
		rc = set_wacom_ble_charge_mode(true); /* 0:pass, etc:fail */
		if (rc == 0)
			drvdata->noise_onoff_flag = QBT2000_NOISE_UNBLOCK;
		pr_info("%d, rc:%d, flag:%d\n", control, rc, drvdata->noise_onoff_flag);
	} else if ((control == QBT2000_NOISE_BLOCK) && (drvdata->noise_onoff_flag == QBT2000_NOISE_UNBLOCK)) {
		drvdata->noise_onoff_flag = QBT2000_NOISE_BLOCK;
		while (retry--) {
			rc = set_wacom_ble_charge_mode(false);
			pr_info("%d, retry:%d, rc:%d\n", control, retry, rc);
			if (rc == 0)
				break;
			usleep_range(4950, 5000);
		}
	} else {
		pr_err("invalid value %d,%d\n", control, drvdata->noise_onoff_flag);
	}

	if (rc < 0) {
		drvdata->noise_onoff_flag = control ? QBT2000_NOISE_BLOCK : QBT2000_NOISE_UNBLOCK;
		drvdata->i2c_error_set++;
	}

	return rc;
}
#endif

static int qbt2000_power_control(struct qbt2000_drvdata *drvdata, int onoff)
{
	int rc = 0;
	if (drvdata->ldogpio >= 0) {
		gpio_set_value(drvdata->ldogpio, onoff);
		drvdata->enabled_ldo = onoff;
#if !defined(ENABLE_SENSORS_FPRINT_SECURE) || defined(DISABLED_GPIO_PROTECTION)
		if (onoff) {
			if (drvdata->pins_poweron) {
				rc = pinctrl_select_state(drvdata->p, drvdata->pins_poweron);
				pr_debug("pinctrl for poweron\n");
			}
		} else {
			if (drvdata->pins_poweroff) {
				rc = pinctrl_select_state(drvdata->p, drvdata->pins_poweroff);
				pr_debug("pinctrl for poweroff\n");
			}
		}
#endif
		pr_info("%s\n", onoff ? "ON" : "OFF");
	} else if (drvdata->regulator_1p8 != NULL) {
		if (onoff) {
			rc = regulator_enable(drvdata->regulator_1p8);
			if (rc)
				pr_err("regulator enable failed, rc=%d\n", rc);
		} else {
			rc = regulator_disable(drvdata->regulator_1p8);
			if (rc)
				pr_err("regulator disable failed, rc=%d\n", rc);
		}
#if !defined(ENABLE_SENSORS_FPRINT_SECURE) || defined(DISABLED_GPIO_PROTECTION)
		if (onoff) {
			if (drvdata->pins_poweron) {
				rc = pinctrl_select_state(drvdata->p, drvdata->pins_poweron);
				pr_debug("pinctrl for poweron\n");
			}
		} else {
			if (drvdata->pins_poweroff) {
				rc = pinctrl_select_state(drvdata->p, drvdata->pins_poweroff);
				pr_debug("pinctrl for poweroff\n");
			}
		}
#endif
		drvdata->enabled_ldo = onoff;
		pr_info("%s\n", onoff ? "ON" : "OFF");
	} else {
		pr_info("This hw revision does not support power control\n");
	}
	return rc;
}

static int qbt2000_enable_spi_clock(struct qbt2000_drvdata *drvdata)
{
	int rc = 0;

	rc = qbt2000_set_clk(drvdata, 1);
	return rc;
}

static int qbt2000_disable_spi_clock(struct qbt2000_drvdata *drvdata)
{
	int rc = 0;

	rc = qbt2000_set_clk(drvdata, 0);
	return rc;
}

static int qbt2000_enable_ipc(struct qbt2000_drvdata *drvdata)
{
	int rc = 0;

	if (drvdata->fw_ipc.gpio) {
		if (drvdata->enabled_ipc) {
			rc = -EINVAL;
			pr_err("already enabled ipc\n");
		} else {
			enable_irq(drvdata->fw_ipc.irq);
#ifdef ENABLE_SENSORS_FPRINT_SECURE
			enable_irq_wake(drvdata->fw_ipc.irq);
#endif
			drvdata->enabled_ipc = true;
		}
	}
	return rc;
}

static int qbt2000_disable_ipc(struct qbt2000_drvdata *drvdata)
{
	int rc = 0;

	if (drvdata->fw_ipc.gpio) {
		if (drvdata->enabled_ipc) {
#ifdef ENABLE_SENSORS_FPRINT_SECURE
			disable_irq_wake(drvdata->fw_ipc.irq);
#endif
			disable_irq(drvdata->fw_ipc.irq);
			drvdata->enabled_ipc = false;
		} else {
			rc = -EINVAL;
			pr_err("already disabled ipc\n");			
		}
	}
	return rc;
}

static int qbt2000_enable_wuhb(struct qbt2000_drvdata *drvdata)
{
	int rc = 0;
	int gpio = 0;

	if (drvdata->fd_gpio.gpio) {
		if (drvdata->enabled_wuhb) {
			rc = -EINVAL;
			pr_err("already enabled wuhb\n");
		} else {
			enable_irq(drvdata->fd_gpio.irq);
			enable_irq_wake(drvdata->fd_gpio.irq);
			drvdata->enabled_wuhb = true;
			/* To prevent FingerUp Missing issue. */
			gpio = gpio_get_value(drvdata->fd_gpio.gpio);
			if (drvdata->fd_gpio.last_gpio_state == FINGER_DOWN_GPIO_STATE &&
				gpio == FINGER_LEAVE_GPIO_STATE) {
				pr_info("Finger leave event already occured. %d, %d\n",
					drvdata->fd_gpio.last_gpio_state, gpio);

				wake_lock_timeout(&drvdata->fp_signal_lock,
						msecs_to_jiffies(QBT2000_WAKELOCK_HOLD_TIME));
				schedule_work(&drvdata->fd_gpio.work);
			}
		}
	}
	return rc;
}

static int qbt2000_disable_wuhb(struct qbt2000_drvdata *drvdata)
{
	int rc = 0;

	if (drvdata->fd_gpio.gpio) {
		if (drvdata->enabled_wuhb) {
			disable_irq(drvdata->fd_gpio.irq);
			disable_irq_wake(drvdata->fd_gpio.irq);
			drvdata->enabled_wuhb = false;
		} else {
			rc = -EINVAL;
			pr_err("already disabled wuhb\n");
		}
	}
	return rc;
}

/**
 * qbt2000_open() - Function called when user space opens device.
 * Successful if driver not currently open.
 * @inode:	ptr to inode object
 * @file:	ptr to file object
 *
 * Return: 0 on success. Error code on failure.
 */
static int qbt2000_open(struct inode *inode, struct file *file)
{
	struct qbt2000_drvdata *drvdata = NULL;
	int rc = 0;
	int minor_no = iminor(inode);

	if (minor_no == MINOR_NUM_FD) {
		drvdata = container_of(inode->i_cdev, struct qbt2000_drvdata, qbt2000_fd_cdev);
	} else if (minor_no == MINOR_NUM_IPC) {
		drvdata = container_of(inode->i_cdev, struct qbt2000_drvdata, qbt2000_ipc_cdev);
	} else {
		pr_err("Invalid minor number\n");
		return -EINVAL;
	}
	file->private_data = drvdata;

	/* disallowing concurrent opens */
	if (minor_no == MINOR_NUM_FD && !atomic_dec_and_test(&drvdata->fd_available)) {
		atomic_inc(&drvdata->fd_available);
		pr_err("fd_unavailable\n");
		rc = -EBUSY;
	} else if (minor_no == MINOR_NUM_IPC && !atomic_dec_and_test(&drvdata->ipc_available)) {
		atomic_inc(&drvdata->ipc_available);
		pr_err("ipc_unavailable\n");
		rc = -EBUSY;
	}

	pr_debug("minor_no=%d, rc=%d,%d,%d\n", minor_no, rc,
		atomic_read(&drvdata->fd_available),
		atomic_read(&drvdata->ipc_available));
	return rc;
}

/**
 * qbt2000_release() - Function called when user space closes device.

 * @inode:	ptr to inode object
 * @file:	ptr to file object
 *
 * Return: 0 on success. Error code on failure.
 */
static int qbt2000_release(struct inode *inode, struct file *file)
{
	struct qbt2000_drvdata *drvdata;
	int minor_no;
	int rc = 0;

	if (!file || !file->private_data) {
		pr_err("qbt2000_release: NULL pointer passed\n");
		return -EINVAL;
	}
	drvdata = file->private_data;
	minor_no = iminor(inode);
	if (minor_no == MINOR_NUM_FD) {
		atomic_inc(&drvdata->fd_available);
	} else if (minor_no == MINOR_NUM_IPC) {
		atomic_inc(&drvdata->ipc_available);
	} else {
		pr_err("Invalid minor number\n");
		rc = -EINVAL;
	}
	pr_debug("minor_no=%d, rc=%d,%d,%d\n", minor_no, rc,
		atomic_read(&drvdata->fd_available),
		atomic_read(&drvdata->ipc_available));
	return rc;
}

/**
 * qbt2000_ioctl() - Function called when user space calls ioctl.
 * @file:	struct file - not used
 * @cmd:	cmd identifier:QBT2000_LOAD_APP,QBT2000_UNLOAD_APP,
 *              QBT2000_SEND_TZCMD
 * @arg:	ptr to relevant structe: either qbt2000_app or
 *              qbt2000_send_tz_cmd depending on which cmd is passed
 *
 * Return: 0 on success. Error code on failure.
 */
static long qbt2000_ioctl(
		struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc = 0;
	int data = 0;
	void __user *priv_arg = (void __user *)arg;
	struct qbt2000_drvdata *drvdata;

	if (!file || !file->private_data) {
		pr_err("qbt2000_ioctl: NULL pointer passed\n");
		return -EINVAL;
	}

	drvdata = file->private_data;

	if (IS_ERR(priv_arg)) {
		pr_err("invalid user space pointer %lu\n", arg);
		return -EINVAL;
	}

	mutex_lock(&drvdata->ioctl_mutex);

	switch (cmd) {
	case QBT2000_IS_WHUB_CONNECTED:
		break;
	case QBT2000_POWER_CONTROL:
		if (copy_from_user(&data, (void *)arg, sizeof(int)) != 0) {
			pr_err("Failed copy from user.(POWER_CONTROL)\n");
			rc = -EFAULT;
			goto ioctl_failed;
		}
		if (drvdata->enabled_ldo != data) {
			pr_debug("POWER_CONTROL\n");
			qbt2000_power_control(drvdata, data);
		}
		break;
	case QBT2000_ENABLE_SPI_CLOCK:
		pr_info("ENABLE_SPI_CLOCK\n");
		rc = qbt2000_enable_spi_clock(drvdata);
		break;
	case QBT2000_DISABLE_SPI_CLOCK:
		pr_info("DISABLE_SPI_CLOCK\n");
		rc = qbt2000_disable_spi_clock(drvdata);
		break;
	case QBT2000_ENABLE_IPC:
		pr_info("ENABLE_IPC\n");
		rc = qbt2000_enable_ipc(drvdata);
		break;
	case QBT2000_DISABLE_IPC:
		pr_info("DISABLE_IPC\n");
		rc = qbt2000_disable_ipc(drvdata);
		break;
	case QBT2000_ENABLE_WUHB:
		pr_info("ENABLE_WUHB\n");
		drvdata->wuhb_test_flag = 0;
		rc = qbt2000_enable_wuhb(drvdata);
		break;
	case QBT2000_DISABLE_WUHB:
		pr_info("DISABLE_WUHB\n");
		/* initialize IntTest flag */
		drvdata->wuhb_test_flag = 0;
		rc = qbt2000_disable_wuhb(drvdata);
		break;
	case QBT2000_CPU_SPEEDUP:
		if (copy_from_user(&data, (void *)arg, sizeof(int)) != 0) {
			pr_err("Failed copy from user.(SPEEDUP)\n");
			rc = -EFAULT;
			goto ioctl_failed;
		}
		rc = qbt2000_set_cpu_speedup(drvdata, data);
		break;
	case QBT2000_SET_SENSOR_TYPE:
		if (copy_from_user(&data, (void *)arg, sizeof(int)) != 0) {
			pr_err("Failed to copy sensor type from user to kernel\n");
			rc = -EFAULT;
			goto ioctl_failed;
		}
		if (data >= SENSOR_OOO && data < SENSOR_MAXIMUM) {
			if (data == SENSOR_OOO && drvdata->sensortype == SENSOR_FAILED) {
				pr_err("Maintain type check from out of oder :%s\n",
						sensor_status[drvdata->sensortype + 2]);
			} else {
				drvdata->sensortype = data;
				pr_info("SET_SENSOR_TYPE :%s\n",
						sensor_status[drvdata->sensortype + 2]);
			}
		} else {
			pr_err("SET_SENSOR_TYPE : invalid value %d\n", data);
			drvdata->sensortype = SENSOR_UNKNOWN;
		}
		break;
	case QBT2000_SET_LOCKSCREEN:
		break;
	case QBT2000_SENSOR_RESET:
		drvdata->reset_count++;
		pr_err("SENSOR_RESET\n");
		break;
	case QBT2000_SENSOR_TEST:
		if (copy_from_user(&data, (void *)arg, sizeof(int)) != 0) {
			pr_err("Failed to copy BGECAL from user to kernel\n");
			rc = -EFAULT;
			goto ioctl_failed;
		}
#ifndef ENABLE_SENSORS_FPRINT_SECURE  //only for factory
#ifdef QBT2000_AVOID_NOISE
		if (data == QBT2000_SENSORTEST_DONE) {
			pr_info("SENSORTEST Finished\n");
			qbt2000_noise_control(drvdata, QBT2000_NOISE_BLOCK);
		} else {
			pr_info("SENSORTEST Start : 0x%x\n", data);
			qbt2000_noise_control(drvdata, QBT2000_NOISE_UNBLOCK);
		}
#endif
#endif
		break;
	case QBT2000_NOISE_REQUEST_STOP:
#ifdef QBT2000_AVOID_NOISE
		pr_info("QBT2000_NOISE_REQUEST_STOP. entry\n");
		drvdata->noise_i2c_result = 1;
		schedule_work(&drvdata->work_noise_control);
		msleep(50);
#endif
		break;
	case QBT2000_NOISE_I2C_RESULT_GET:
#ifdef QBT2000_AVOID_NOISE
		pr_info("QBT2000_NOISE_I2C_RESULT_GET : %d\n", drvdata->noise_i2c_result);
		if (copy_to_user((void __user *)priv_arg, &drvdata->noise_i2c_result, sizeof(drvdata->noise_status)) != 0) {
			pr_err("Failed to copy I2C_RESULT to user\n");
			rc = -EFAULT;
		}
#endif
		break;
	case QBT2000_NOISE_STATUS_GET:
#ifdef QBT2000_AVOID_NOISE
		drvdata->noise_status = get_wacom_scan_info(false);
		if (drvdata->noise_status == QBT2000_NOISE_MODE_CHANGED) {
			drvdata->ignored_cbge_count++;
		} else if (drvdata->noise_status == QBT2000_NOISE_CHARGING) {
			drvdata->i2c_charging++;
		} else if (drvdata->noise_status < 0) {
			drvdata->i2c_error_get++;
		}
		pr_info("QBT2000_NOISE_STATUS_GET : %d\n", drvdata->noise_status);
		if (copy_to_user((void __user *)priv_arg, &drvdata->noise_status, sizeof(drvdata->noise_status)) != 0) {
			pr_err("Failed to copy NOISE_STATUS to user\n");
			rc = -EFAULT;
		}
#endif
		break;
	case QBT2000_NOISE_REQUEST_START:
#ifdef QBT2000_AVOID_NOISE
		pr_info("QBT2000_NOISE_REQUEST_START\n");
		qbt2000_noise_control(drvdata, QBT2000_NOISE_UNBLOCK);
#endif
		break;
	case QBT2000_NOISE_REQUEST_STATUS:
#ifdef QBT2000_AVOID_NOISE
		pr_info("QBT2000_NOISE_REQUEST_STATUS : %d\n", drvdata->noise_onoff_flag);
		if (copy_to_user((void __user *)priv_arg, &drvdata->noise_onoff_flag, sizeof(drvdata->noise_onoff_flag)) != 0) {
			pr_err("Failed to copy REQUEST_STATUS to user\n");
			rc = -EFAULT;
		}
#endif
		break;
	case QBT2000_GET_MODELINFO:
		pr_info("QBT2000_GET_MODELINFO : %s\n", drvdata->model_info);
		if (copy_to_user((void __user *)priv_arg, drvdata->model_info, 10)) {
			pr_err("Failed to copy GET_MODELINFO to user\n");
			rc = -EFAULT;
		}
		break;
	default:
		pr_err("invalid cmd %d\n", cmd);
		rc = -ENOIOCTLCMD;
	}

ioctl_failed:
	mutex_unlock(&drvdata->ioctl_mutex);
	return rc;
}


static int get_events_fifo_len_locked(struct qbt2000_drvdata *drvdata, int minor_no)
{
	int len = 0;

	if (minor_no == MINOR_NUM_FD) {
		mutex_lock(&drvdata->fd_events_mutex);
		len = kfifo_len(&drvdata->fd_events);
		mutex_unlock(&drvdata->fd_events_mutex);
	} else if (minor_no == MINOR_NUM_IPC) {
		mutex_lock(&drvdata->ipc_events_mutex);
		len = kfifo_len(&drvdata->ipc_events);
		mutex_unlock(&drvdata->ipc_events_mutex);
	}

	return len;
}

static ssize_t qbt2000_read(struct file *filp, char __user *ubuf,
		size_t cnt, loff_t *ppos)
{
	struct fw_event_desc fw_event;
	struct qbt2000_drvdata *drvdata = filp->private_data;
	wait_queue_head_t *read_wait_queue = NULL;
	int rc = 0;
	int minor_no = iminor(filp->f_path.dentry->d_inode);
	int fifo_len = get_events_fifo_len_locked(drvdata, minor_no);

	if (cnt < sizeof(fw_event.ev)) {
		pr_err("Num bytes to read is too small, numBytes=%zd\n", cnt);
		return -EINVAL;
	}

	if (minor_no == MINOR_NUM_FD) {
		read_wait_queue = &drvdata->read_wait_queue_fd;
	} else if (minor_no == MINOR_NUM_IPC) {
		read_wait_queue = &drvdata->read_wait_queue_ipc;
	} else {
		pr_err("Invalid minor number\n");
		return -EINVAL;
	}

	while (fifo_len == 0) {
		if (filp->f_flags & O_NONBLOCK) {
			pr_debug("fw_events fifo: empty, returning\n");
			return -EAGAIN;
		}
		pr_debug("fw_events fifo: empty, waiting\n");
		if (wait_event_interruptible(*read_wait_queue,
				(get_events_fifo_len_locked(drvdata, minor_no) > 0)))
			return -ERESTARTSYS;

		fifo_len = get_events_fifo_len_locked(drvdata, minor_no);
	}

	if (minor_no == MINOR_NUM_FD) {
		mutex_lock(&drvdata->fd_events_mutex);
		rc = kfifo_get(&drvdata->fd_events, &fw_event);
		mutex_unlock(&drvdata->fd_events_mutex);
	} else if (minor_no == MINOR_NUM_IPC) {
		mutex_lock(&drvdata->ipc_events_mutex);
		rc = kfifo_get(&drvdata->ipc_events, &fw_event);
		mutex_unlock(&drvdata->ipc_events_mutex);
	} else {
		pr_err("Invalid minor number\n");
	}

	if (!rc) {
		pr_err("fw_events fifo: unexpectedly empty\n");
		return -EINVAL;
	}

	rc = copy_to_user(ubuf, &fw_event.ev, sizeof(fw_event.ev));
	if (rc != 0) {
		pr_err("Failed to copy_to_user:%d - event:%d, minor:%d\n",
			rc, (int)fw_event.ev, minor_no);
	} else {
		if (minor_no == MINOR_NUM_FD) {
#ifdef QBT2000_AVOID_NOISE
#ifdef ENABLE_SENSORS_FPRINT_SECURE
			mutex_unlock(&drvdata->fod_event_mutex);
#endif
#endif
			pr_info("Firmware event %d at minor no %d read at time %lu uS, mutex_unlock\n",
				(int)fw_event.ev, minor_no,
				(unsigned long)ktime_to_us(ktime_get()));
		} else {
			pr_info("Firmware event %d at minor no %d read at time %lu uS\n",
				(int)fw_event.ev, minor_no,
				(unsigned long)ktime_to_us(ktime_get()));
		}
	}
	return rc;
}

static unsigned int qbt2000_poll(struct file *filp,
	struct poll_table_struct *wait)
{
	struct qbt2000_drvdata *drvdata = filp->private_data;
	unsigned int mask = 0;
	int minor_no = iminor(filp->f_path.dentry->d_inode);

	if (minor_no == MINOR_NUM_FD) {
		poll_wait(filp, &drvdata->read_wait_queue_fd, wait);
		if (kfifo_len(&drvdata->fd_events) > 0) {
			mask |= (POLLIN | POLLRDNORM);
		}
	} else if (minor_no == MINOR_NUM_IPC) {
		poll_wait(filp, &drvdata->read_wait_queue_ipc, wait);
		if (kfifo_len(&drvdata->ipc_events) > 0) {
			mask |= (POLLIN | POLLRDNORM);
		}
	} else {
		pr_err("Invalid minor number\n");
		return -EINVAL;
	}

	return mask;
}

static const struct file_operations qbt2000_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = qbt2000_ioctl,
	.open = qbt2000_open,
	.release = qbt2000_release,
	.read = qbt2000_read,
	.poll = qbt2000_poll
};

static int qbt2000_dev_register(struct qbt2000_drvdata *drvdata)
{
	dev_t dev_no, major_no;
	int rc = 0;
	struct device *dev = drvdata->dev;

	rc = alloc_chrdev_region(&dev_no, 0, 2, QBT2000_DEV);
	if (rc) {
		pr_err("alloc_chrdev_region failed %d\n", rc);
		goto err_alloc;
	}
	major_no = MAJOR(dev_no);

	cdev_init(&drvdata->qbt2000_fd_cdev, &qbt2000_fops);
	drvdata->qbt2000_fd_cdev.owner = THIS_MODULE;
	rc = cdev_add(&drvdata->qbt2000_fd_cdev, MKDEV(major_no, MINOR_NUM_FD), 1);
	if (rc) {
		pr_err("cdev_add failed for fd %d\n", rc);
		goto err_cdev_add;
	}

	cdev_init(&drvdata->qbt2000_ipc_cdev, &qbt2000_fops);
	drvdata->qbt2000_ipc_cdev.owner = THIS_MODULE;
	rc = cdev_add(&drvdata->qbt2000_ipc_cdev, MKDEV(major_no, MINOR_NUM_IPC), 1);
	if (rc) {
		pr_err("cdev_add failed for ipc %d\n", rc);
		goto err_cdev_add;
	}

	drvdata->qbt2000_class = class_create(THIS_MODULE, QBT2000_DEV);
	if (IS_ERR(drvdata->qbt2000_class)) {
		rc = PTR_ERR(drvdata->qbt2000_class);
		pr_err("class_create failed %d\n", rc);
		goto err_class_create;
	}

	dev = device_create(drvdata->qbt2000_class, NULL, drvdata->qbt2000_fd_cdev.dev,
			drvdata, "%s_fd", QBT2000_DEV);
	if (IS_ERR(dev)) {
		rc = PTR_ERR(dev);
		pr_err("fd device_create failed %d\n", rc);
		goto err_dev_create_fd;
	}

	dev = device_create(drvdata->qbt2000_class, NULL, drvdata->qbt2000_ipc_cdev.dev,
			drvdata, "%s_ipc", QBT2000_DEV);
	if (IS_ERR(dev)) {
		rc = PTR_ERR(dev);
		pr_err("ipc device_create failed %d\n", rc);
		goto err_dev_create_ipc;
	}
	pr_info("finished\n");
	return 0;
err_dev_create_ipc:
	device_destroy(drvdata->qbt2000_class, drvdata->qbt2000_fd_cdev.dev);
err_dev_create_fd:
	class_destroy(drvdata->qbt2000_class);
err_class_create:
	cdev_del(&drvdata->qbt2000_fd_cdev);
	cdev_del(&drvdata->qbt2000_ipc_cdev);
err_cdev_add:
	unregister_chrdev_region(drvdata->qbt2000_fd_cdev.dev, 1);
	unregister_chrdev_region(drvdata->qbt2000_ipc_cdev.dev, 1);
err_alloc:
	return rc;
}

int qbt2000_pinctrl_register(struct qbt2000_drvdata *drvdata)
{
	int rc = 0;

	drvdata->p = pinctrl_get_select_default(drvdata->dev);
	if (IS_ERR(drvdata->p)) {
		rc = -EINVAL;
		pr_err("failed pinctrl_get\n");
		goto pinctrl_register_default_exit;
	}

#if !defined(ENABLE_SENSORS_FPRINT_SECURE) || defined(DISABLED_GPIO_PROTECTION)
	drvdata->pins_poweroff = pinctrl_lookup_state(drvdata->p, "pins_poweroff");
	if (IS_ERR(drvdata->pins_poweroff)) {
		pr_err("could not get pins sleep_state (%li)\n",
			PTR_ERR(drvdata->pins_poweroff));
		drvdata->pins_poweroff = NULL;
		drvdata->pins_poweron = NULL;
		goto pinctrl_register_exit;
	}

	drvdata->pins_poweron = pinctrl_lookup_state(drvdata->p, "pins_poweron");
	if (IS_ERR(drvdata->pins_poweron)) {
		pr_err("could not get pins idle_state (%li)\n",
			PTR_ERR(drvdata->pins_poweron));
		drvdata->pins_poweron = NULL;
		goto pinctrl_register_exit;
	}
#endif
	pr_info("finished\n");
	return rc;
#if !defined(ENABLE_SENSORS_FPRINT_SECURE) || defined(DISABLED_GPIO_PROTECTION)
pinctrl_register_exit:
	pinctrl_put(drvdata->p);
#endif
pinctrl_register_default_exit:
	pr_err("failed %d\n", rc);
	return rc;
}

static void qbt2000_gpio_report_event(struct qbt2000_drvdata *drvdata)
{
	int state;
	struct fw_event_desc fw_event;

	state = (__gpio_get_value(drvdata->fd_gpio.gpio) ? FINGER_DOWN_GPIO_STATE : FINGER_LEAVE_GPIO_STATE)
		^ drvdata->fd_gpio.active_low;

	if (state == drvdata->fd_gpio.last_gpio_state) {
#ifdef QBT2000_AVOID_NOISE
#ifdef ENABLE_SENSORS_FPRINT_SECURE
		pr_debug("skip the report_event. this event already reported, last_gpio:%d\n", state);
		mutex_unlock(&drvdata->fod_event_mutex);
#endif
#endif
		return;
	}

	drvdata->fd_gpio.last_gpio_state = state;

	fw_event.ev = (state ? FW_EVENT_FINGER_DOWN : FW_EVENT_FINGER_UP);

	mutex_lock(&drvdata->fd_events_mutex);

	kfifo_reset(&drvdata->fd_events);

	if (!kfifo_put(&drvdata->fd_events, fw_event))
		pr_err("fw events fifo: error adding item\n");

	mutex_unlock(&drvdata->fd_events_mutex);
	wake_up_interruptible(&drvdata->read_wait_queue_fd);
	pr_info("state: %s\n", state ? "Finger Down" : "Finger Leave");
}

static void qbt2000_wuhb_work_func(struct work_struct *work)
{
	struct qbt2000_drvdata *drvdata =
		container_of(work, struct qbt2000_drvdata, fd_gpio.work);
#ifdef QBT2000_AVOID_NOISE
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	mutex_lock(&drvdata->fod_event_mutex);
	pr_debug("mutex_lock\n");
#endif
#endif
	qbt2000_gpio_report_event(drvdata);
}

#ifdef QBT2000_AVOID_NOISE
static void qbt2000_wuhb_work_noise_down_func(struct work_struct *work)
{
	struct qbt2000_drvdata *drvdata =
		container_of(work, struct qbt2000_drvdata, fd_gpio.work_noise_down);
	int delay_time = QBT2000_NOISE_BLOCK_DELAY;

#ifdef ENABLE_SENSORS_FPRINT_SECURE
	mutex_lock(&drvdata->fod_event_mutex);
	pr_debug("mutex_lock\n");
#endif

	schedule_delayed_work(&drvdata->fd_gpio.delayed_noise_down_work, msecs_to_jiffies(delay_time));
	qbt2000_noise_control(drvdata, QBT2000_NOISE_BLOCK);
}
static void qbt2000_ipc_handler_noise_status(struct work_struct *work)
{
	struct qbt2000_drvdata *drvdata =
			container_of(work, struct qbt2000_drvdata, work_ipc_noise_status);

	drvdata->noise_status = get_wacom_scan_info(true);
	pr_info("entry : %d\n", drvdata->noise_status);
	if (drvdata->noise_status < 0)
		drvdata->i2c_error_get++;
	else if (drvdata->noise_status == QBT2000_NOISE_CHARGING)
		drvdata->i2c_charging++;

}

static void qbt2000_work_noise_control(struct work_struct *work)
{
	struct qbt2000_drvdata *drvdata =
		container_of(work, struct qbt2000_drvdata, work_noise_control);
	int rc = 0;

	pr_info("entry\n");
	rc = qbt2000_noise_control(drvdata, QBT2000_NOISE_BLOCK);
	drvdata->noise_i2c_result = rc;
	pr_info("done : %d\n", drvdata->noise_i2c_result);
}

static void qbt2000_gpio_report_event_delayed(struct qbt2000_drvdata *drvdata, int state)
{
	struct fw_event_desc fw_event;
	int rc = 0;

	if (state == drvdata->fd_gpio.last_gpio_state) {
		pr_info("already done %d %d, rc = %d\n", state, drvdata->fd_gpio.last_gpio_state, rc);
#ifdef ENABLE_SENSORS_FPRINT_SECURE
		mutex_unlock(&drvdata->fod_event_mutex);
#endif
		return;
	}
	drvdata->fd_gpio.last_gpio_state = state;

	fw_event.ev = (state ? FW_EVENT_FINGER_DOWN : FW_EVENT_FINGER_UP);

	mutex_lock(&drvdata->fd_events_mutex);

	kfifo_reset(&drvdata->fd_events);

	if (!kfifo_put(&drvdata->fd_events, fw_event))
		pr_err("fw events fifo: error adding item\n");

	mutex_unlock(&drvdata->fd_events_mutex);
	wake_up_interruptible(&drvdata->read_wait_queue_fd);
	pr_info("state: %s\n", state ? "Finger Down" : "Finger Leave");
}

static void qbt2000_wuhb_delayed_work_func(struct work_struct *work)
{
	struct qbt2000_drvdata *drvdata =
		container_of(work, struct qbt2000_drvdata, fd_gpio.delayed_noise_down_work.work);

	qbt2000_gpio_report_event_delayed(drvdata, drvdata->now_state);
}
#endif

static irqreturn_t qbt2000_wuhb_irq_handler(int irq, void *dev_id)
{
	struct qbt2000_drvdata *drvdata = dev_id;

	if (irq != drvdata->fd_gpio.irq) {
		pr_warn("invalid irq %d (expected %d)\n", irq, drvdata->fd_gpio.irq);
		return IRQ_HANDLED;
	}

	if (drvdata->wuhb_test_flag == 1) {
		pr_info("IntTest. interrupt occured.flag:%d,result:%d,en:%d\n",
					drvdata->wuhb_test_flag,
					drvdata->wuhb_test_result,
					drvdata->enabled_wuhb);
		drvdata->wuhb_test_result = 1;
		return IRQ_HANDLED;
	}

	drvdata->wuhb_count++;
	wake_lock_timeout(&drvdata->fp_signal_lock,
			msecs_to_jiffies(QBT2000_WAKELOCK_HOLD_TIME));
#ifndef QBT2000_AVOID_NOISE // not use digitizer
	schedule_work(&drvdata->fd_gpio.work);
#else
#ifndef ENABLE_SENSORS_FPRINT_SECURE // use digitizer but nontz
	schedule_work(&drvdata->fd_gpio.work);
#else // use digitizer and tz
	drvdata->now_state = (__gpio_get_value(drvdata->fd_gpio.gpio) ? FINGER_DOWN_GPIO_STATE : FINGER_LEAVE_GPIO_STATE) ^ drvdata->fd_gpio.active_low;
	pr_info("gpio state : %d\n", drvdata->now_state);
	if (drvdata->now_state == FINGER_DOWN_GPIO_STATE) {  // in case of finger down
		schedule_work(&drvdata->fd_gpio.work_noise_down);
	} else {
		schedule_work(&drvdata->fd_gpio.work);
	}
#endif //ENABLE_SENSORS_FPRINT_SECURE
#endif //QBT2000_AVOID_NOISE

	return IRQ_HANDLED;
}

/*
 * qbt2000_ipc_irq_handler() - function processes IPC
 * interrupts on its own thread
 * @irq:	the interrupt that occurred
 * @dev_id: pointer to the qbt2000_drvdata
 *
 * Return: IRQ_HANDLED when complete
 */
static irqreturn_t qbt2000_ipc_irq_handler(int irq, void *dev_id)
{
	struct qbt2000_drvdata *drvdata = (struct qbt2000_drvdata *)dev_id;
	enum qbt2000_fw_event ev = FW_EVENT_CBGE_REQUIRED;
	struct fw_event_desc fw_ev_des;

	wake_lock_timeout(&drvdata->fp_signal_lock,
			msecs_to_jiffies(QBT2000_WAKELOCK_HOLD_TIME));
	mutex_lock(&drvdata->mutex);

	if (irq != drvdata->fw_ipc.irq) {
		pr_warn("invalid irq %d (expected %d)\n", irq, drvdata->fw_ipc.irq);
		goto ipc_irq_failed;
	}
#ifdef ENABLE_SENSORS_FPRINT_SECURE
#ifdef QBT2000_AVOID_NOISE
	schedule_work(&drvdata->work_ipc_noise_status);
#endif
#endif

	mutex_lock(&drvdata->ipc_events_mutex);
	fw_ev_des.ev = ev;
	if (!kfifo_put(&drvdata->ipc_events, fw_ev_des))
		pr_err("fw events: fifo full, drop event %d\n", (int) ev);

	drvdata->cbge_count++;
	mutex_unlock(&drvdata->ipc_events_mutex);

	wake_up_interruptible(&drvdata->read_wait_queue_ipc);
	pr_debug("ipc interrupt received, irq=%d, event=%d\n", irq, (int)ev);
ipc_irq_failed:
	mutex_unlock(&drvdata->mutex);
	return IRQ_HANDLED;
}

static int qbt2000_setup_fd_gpio_irq(struct platform_device *pdev,
		struct qbt2000_drvdata *drvdata)
{
	int rc = 0;
	int irq;
	const char *desc = "qbt_finger_detect";

	rc = devm_gpio_request_one(&pdev->dev, drvdata->fd_gpio.gpio,
		GPIOF_IN, desc);
	if (rc < 0) {
		pr_err("failed to request gpio %d, error %d\n",
			drvdata->fd_gpio.gpio, rc);
		goto fd_gpio_failed;
	}

	irq = gpio_to_irq(drvdata->fd_gpio.gpio);
	if (irq < 0) {
		rc = irq;
		pr_err("unable to get irq number for gpio %d, error %d\n",
			drvdata->fd_gpio.gpio, rc);
		goto fd_gpio_failed_request;
	}

	drvdata->fd_gpio.irq = irq;
	INIT_WORK(&drvdata->fd_gpio.work, qbt2000_wuhb_work_func);
#ifdef QBT2000_AVOID_NOISE
	INIT_WORK(&drvdata->fd_gpio.work_noise_down, qbt2000_wuhb_work_noise_down_func);
	INIT_DELAYED_WORK(&drvdata->fd_gpio.delayed_noise_down_work, qbt2000_wuhb_delayed_work_func);
	INIT_WORK(&drvdata->work_ipc_noise_status, qbt2000_ipc_handler_noise_status);
	INIT_WORK(&drvdata->work_noise_control, qbt2000_work_noise_control);
#endif
	rc = devm_request_any_context_irq(&pdev->dev, drvdata->fd_gpio.irq,
		qbt2000_wuhb_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
		desc, drvdata);
	if (rc < 0) {
		pr_err("unable to claim irq %d; error %d\n",
			drvdata->fd_gpio.irq, rc);
		goto fd_gpio_failed_request;
	}
	enable_irq_wake(drvdata->fd_gpio.irq);
	drvdata->enabled_wuhb = true;
	qbt2000_disable_wuhb(drvdata);
	pr_debug("irq=%d,gpio=%d,rc=%d\n", drvdata->fd_gpio.irq, drvdata->fd_gpio.gpio, rc);
fd_gpio_failed_request:
	gpio_free(drvdata->fd_gpio.gpio);
fd_gpio_failed:
	return rc;
}

static int qbt2000_setup_ipc_irq(struct platform_device *pdev,
	struct qbt2000_drvdata *drvdata)
{
	int rc = 0;
	const char *desc = "qbt_ipc";

	drvdata->fw_ipc.irq = gpio_to_irq(drvdata->fw_ipc.gpio);
	if (drvdata->fw_ipc.irq < 0) {
		rc = drvdata->fw_ipc.irq;
		pr_err("no irq for gpio %d, error=%d\n",
		  drvdata->fw_ipc.gpio, rc);
		goto ipc_gpio_failed;
	}

	rc = devm_gpio_request_one(&pdev->dev, drvdata->fw_ipc.gpio,
			GPIOF_IN, desc);

	if (rc < 0) {
		pr_err("failed to request gpio %d, error %d\n",
			drvdata->fw_ipc.gpio, rc);
		goto ipc_gpio_failed;
	}

	rc = devm_request_threaded_irq(&pdev->dev,
		drvdata->fw_ipc.irq,
		NULL,
		qbt2000_ipc_irq_handler,
		IRQF_ONESHOT | IRQF_TRIGGER_FALLING,
		desc,
		drvdata);

	if (rc < 0) {
		pr_err("failed to register for ipc irq %d, rc = %d\n",
			drvdata->fw_ipc.irq, rc);
		goto ipc_gpio_failed_request;
	}

#ifdef ENABLE_SENSORS_FPRINT_SECURE
	enable_irq_wake(drvdata->fw_ipc.irq);
#endif
	drvdata->enabled_ipc = true;
	qbt2000_disable_ipc(drvdata);
	pr_debug("irq=%d,gpio=%d,rc=%d\n", drvdata->fw_ipc.irq, drvdata->fw_ipc.gpio, rc);
ipc_gpio_failed_request:
	gpio_free(drvdata->fw_ipc.gpio);
ipc_gpio_failed:
	return rc;
}

/**
 * qbt2000_read_device_tree() - Function reads device tree
 * properties into driver data
 * @pdev:	ptr to platform device object
 * @drvdata:	ptr to driver data
 *
 * Return: 0 on success. Error code on failure.
 */
static int qbt2000_read_device_tree(struct platform_device *pdev,
	struct qbt2000_drvdata *drvdata)
{
	int rc = 0;
	enum of_gpio_flags flags;

	/* read IPC gpio */
	drvdata->fw_ipc.gpio = of_get_named_gpio(pdev->dev.of_node,
		"qcom,ipc-gpio", 0);
	if (drvdata->fw_ipc.gpio < 0) {
		rc = drvdata->fw_ipc.gpio;
		pr_err("ipc gpio not found, error=%d\n", rc);
		goto dt_failed;
	}

	/* read WUHB gpio */
	drvdata->fd_gpio.gpio = of_get_named_gpio_flags(pdev->dev.of_node,
				"qcom,wuhb-gpio", 0, &flags);
	if (drvdata->fd_gpio.gpio < 0) {
		rc = drvdata->fd_gpio.gpio;
		pr_err("wuhb gpio not found, error=%d\n", rc);
		goto dt_failed;
	} else {
		drvdata->fd_gpio.active_low = flags & OF_GPIO_ACTIVE_LOW;
	}

	drvdata->ldogpio = of_get_named_gpio(pdev->dev.of_node, "qcom,ldo-gpio", 0);
	if (drvdata->ldogpio < 0) {
		pr_info("ldo gpio not found. %d\n", drvdata->ldogpio);
	} else {			
		rc = gpio_request(drvdata->ldogpio, "qbt_ldo_en");
		gpio_direction_output(drvdata->ldogpio, 0);
		drvdata->enabled_ldo = 0;
	}

	if (of_property_read_string(pdev->dev.of_node, "qcom,btp-regulator", &drvdata->btp_vdd) < 0) {
		pr_info("not use btp_regulator\n");
		drvdata->btp_vdd = NULL;
	} else {
		drvdata->regulator_1p8 = regulator_get(NULL, drvdata->btp_vdd);
		if (IS_ERR(drvdata->regulator_1p8) ||
				(drvdata->regulator_1p8) == NULL) {
			pr_info("not use regulator_1p8\n");
			drvdata->regulator_1p8 = NULL;
		} else {
			pr_info("btp_regulator ok\n");
		}
	}

	if (of_property_read_u32(pdev->dev.of_node, "qcom,min_cpufreq_limit",
				&drvdata->min_cpufreq_limit))
		drvdata->min_cpufreq_limit = 0;

	if (of_property_read_string_index(pdev->dev.of_node, "qcom,position", 0,
			(const char **)&drvdata->sensor_position))
		drvdata->sensor_position = "13.77,0.00,9.00,4.00,14.80,14.80,11.00,11.00,5.00";
	pr_info("Sensor Position: %s\n", drvdata->sensor_position);

	if (of_property_read_string_index(pdev->dev.of_node, "qcom,modelinfo", 0,
			(const char **)&drvdata->model_info)) {
		drvdata->model_info = "NONE";
	}
	pr_info("modelinfo: %s\n", drvdata->model_info);

	pr_info("finished\n");
	return rc;
dt_failed:
	pr_err("failed:%d\n", rc);
	return rc;
}

static void qbt2000_work_func_debug(struct work_struct *work)
{
#ifndef QBT2000_AVOID_NOISE
	pr_info("ldo:%d,ipc:%d,wuhb:%d,tz:%d,type:%s,int:%d,%d\n",
		g_data->enabled_ldo, g_data->enabled_ipc,
		g_data->enabled_wuhb, g_data->tz_mode,
		sensor_status[g_data->sensortype + 2],
		g_data->cbge_count, g_data->wuhb_count);
#else
	pr_info("ldo:%d,ipc:%d,wuhb:%d,tz:%d,type:%s,int:%d,%d,%d,%d,%d,%d,%d\n",
		g_data->enabled_ldo, g_data->enabled_ipc,
		g_data->enabled_wuhb, g_data->tz_mode,
		sensor_status[g_data->sensortype + 2],
		g_data->cbge_count, g_data->ignored_cbge_count,
		g_data->wuhb_count, g_data->i2c_error_set,
		g_data->i2c_error_get, g_data->i2c_charging,
		g_data->noise_onoff_flag);
#endif
}

static void qbt2000_enable_debug_timer(void)
{
	mod_timer(&g_data->dbg_timer,
		round_jiffies_up(jiffies + FPSENSOR_DEBUG_TIMER_SEC));
}

static void qbt2000_disable_debug_timer(void)
{
	del_timer_sync(&g_data->dbg_timer);
	cancel_work_sync(&g_data->work_debug);
}

static void qbt2000_timer_func(unsigned long ptr)
{
	queue_work(g_data->wq_dbg, &g_data->work_debug);
	mod_timer(&g_data->dbg_timer,
		round_jiffies_up(jiffies + FPSENSOR_DEBUG_TIMER_SEC));
}

static int qbt2000_set_timer(struct qbt2000_drvdata *drvdata)
{
	int rc = 0;

	setup_timer(&drvdata->dbg_timer, qbt2000_timer_func,
			(unsigned long)drvdata);
	drvdata->wq_dbg = create_singlethread_workqueue("qbt2000_debug_wq");
	if (!drvdata->wq_dbg) {
		rc = -ENOMEM;
		pr_err("could not create workqueue\n");
		return rc;
	}
	INIT_WORK(&drvdata->work_debug, qbt2000_work_func_debug);

	return rc;
}

/**
 * qbt2000_probe() - Function loads hardware config from device tree
 * @pdev:	ptr to platform device object
 *
 * Return: 0 on success. Error code on failure.
 */
static int qbt2000_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct qbt2000_drvdata *drvdata;
	int rc = 0;

	pr_info("Start\n");
#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge) {
		pr_info("Do not load driver due to : lpm %d\n", lpcharge);
		return rc;
	}
#endif
	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->dev = &pdev->dev;
	platform_set_drvdata(pdev, drvdata);

	rc = qbt2000_read_device_tree(pdev, drvdata);
	if (rc < 0)
		goto probe_failed_dt;

	atomic_set(&drvdata->fd_available, 1);
	atomic_set(&drvdata->ipc_available, 1);

	mutex_init(&drvdata->mutex);
	mutex_init(&drvdata->ioctl_mutex);
	mutex_init(&drvdata->fd_events_mutex);
	mutex_init(&drvdata->ipc_events_mutex);
#ifdef QBT2000_AVOID_NOISE
	mutex_init(&drvdata->fod_event_mutex);
#endif

	rc = qbt2000_dev_register(drvdata);
	if (rc < 0)
		goto probe_failed_dev_register;

	INIT_KFIFO(drvdata->fd_events);
	INIT_KFIFO(drvdata->ipc_events);
	init_waitqueue_head(&drvdata->read_wait_queue_fd);
	init_waitqueue_head(&drvdata->read_wait_queue_ipc);

	wake_lock_init(&drvdata->fp_spi_lock,
			WAKE_LOCK_SUSPEND, "qbt2000_spi_lock");
	wake_lock_init(&drvdata->fp_signal_lock,
			WAKE_LOCK_SUSPEND, "qbt2000_signal_lock");

	rc = qbt2000_pinctrl_register(drvdata);
	if (rc < 0)
		goto probe_failed_pinctrl;

	rc = qbt2000_setup_fd_gpio_irq(pdev, drvdata);
	if (rc < 0)
		goto probe_failed_fd_gpio;

	rc = qbt2000_setup_ipc_irq(pdev, drvdata);
	if (rc < 0)
		goto probe_failed_ipc_gpio;

	rc = device_init_wakeup(&pdev->dev, 1);
	if (rc < 0)
		goto probe_failed_device_init_wakeup;

	rc = qbt2000_register_platform_variable(drvdata);
	if (rc < 0)
		goto probe_failed_platform_variable;

	rc = fingerprint_register(drvdata->fp_device,
		drvdata, fp_attrs, "fingerprint");
	if (rc ) {
		pr_err("sysfs register failed\n");
		goto probe_failed_sysfs_register;
	}

#ifndef ENABLE_SENSORS_FPRINT_SECURE
	qbt2000_power_control(drvdata, 1);
#endif

	g_data = drvdata;
	drvdata->spi_speed = SPI_CLOCK_MAX;
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	drvdata->enabled_clk = false;
	drvdata->tz_mode = true;
#else
	drvdata->enabled_clk = true;
	drvdata->tz_mode = false;
#endif
	drvdata->sensortype = SENSOR_QBT2000;
	drvdata->cbge_count = 0;
#ifdef QBT2000_AVOID_NOISE
	drvdata->ignored_cbge_count = 0;
	drvdata->i2c_error_set = 0;
	drvdata->i2c_error_get = 0;
	drvdata->i2c_charging = 0;
	drvdata->noise_status = QBT2000_NOISE_NO_CHARGING;
	drvdata->noise_onoff_flag = QBT2000_NOISE_UNBLOCK;
#endif
	drvdata->wuhb_count = 0;
	drvdata->reset_count = 0;
	drvdata->wuhb_test_flag = 0;
	drvdata->wuhb_test_result = 0;

	qbt2000_set_timer(drvdata);
	qbt2000_enable_debug_timer();

	pr_info("finished\n");
	return 0;

probe_failed_sysfs_register:
probe_failed_platform_variable:
probe_failed_device_init_wakeup:
	gpio_free(drvdata->fw_ipc.gpio);
probe_failed_ipc_gpio:
	gpio_free(drvdata->fd_gpio.gpio);
probe_failed_fd_gpio:
	pinctrl_put(drvdata->p);
probe_failed_pinctrl:
	wake_lock_destroy(&drvdata->fp_spi_lock);
	wake_lock_destroy(&drvdata->fp_signal_lock);
	device_destroy(drvdata->qbt2000_class, drvdata->qbt2000_ipc_cdev.dev);
	device_destroy(drvdata->qbt2000_class, drvdata->qbt2000_fd_cdev.dev);
	class_destroy(drvdata->qbt2000_class);
	cdev_del(&drvdata->qbt2000_fd_cdev);
	cdev_del(&drvdata->qbt2000_ipc_cdev);
	unregister_chrdev_region(drvdata->qbt2000_fd_cdev.dev, 1);
	unregister_chrdev_region(drvdata->qbt2000_ipc_cdev.dev, 1);
probe_failed_dev_register:
	if (drvdata->regulator_1p8)
		regulator_put(drvdata->regulator_1p8);
probe_failed_dt:
	kfree(drvdata);
	pr_err("failed: %d\n", rc);
	return rc;
}

static int qbt2000_remove(struct platform_device *pdev)
{
	struct qbt2000_drvdata *drvdata = platform_get_drvdata(pdev);

	pr_info("called\n");

	mutex_destroy(&drvdata->mutex);
	mutex_destroy(&drvdata->ioctl_mutex);
	mutex_destroy(&drvdata->fd_events_mutex);
	mutex_destroy(&drvdata->ipc_events_mutex);
#ifdef QBT2000_AVOID_NOISE
	mutex_destroy(&drvdata->fod_event_mutex);
#endif

	device_destroy(drvdata->qbt2000_class, drvdata->qbt2000_fd_cdev.dev);
	device_destroy(drvdata->qbt2000_class, drvdata->qbt2000_ipc_cdev.dev);

	qbt2000_disable_debug_timer();
	if (drvdata->regulator_1p8)
		regulator_put(drvdata->regulator_1p8);
	wake_lock_destroy(&drvdata->fp_spi_lock);
	wake_lock_destroy(&drvdata->fp_signal_lock);
	class_destroy(drvdata->qbt2000_class);
	cdev_del(&drvdata->qbt2000_fd_cdev);
	cdev_del(&drvdata->qbt2000_ipc_cdev);
	unregister_chrdev_region(drvdata->qbt2000_fd_cdev.dev, 1);
	unregister_chrdev_region(drvdata->qbt2000_ipc_cdev.dev, 1);
	fingerprint_unregister(drvdata->fp_device, fp_attrs);
	device_init_wakeup(&pdev->dev, 0);
	qbt2000_unregister_platform_variable(drvdata);
	pinctrl_put(drvdata->p);
	kfree(drvdata);

	return 0;
}

static int qbt2000_suspend(struct platform_device *pdev, pm_message_t state)
{
	int rc = 0;
	struct qbt2000_drvdata *drvdata = platform_get_drvdata(pdev);

#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge)
		return rc;
#endif

#if defined(ENABLE_SENSORS_FPRINT_SECURE)
	fpsensor_goto_suspend = 1;
#endif

	/*
	 * Returning an error code if driver currently making a TZ call.
	 * Note: The purpose of this driver is to ensure that the clocks are on
	 * while making a TZ call. Hence the clock check to determine if the
	 * driver will allow suspend to occur.
	 */
	if (!mutex_trylock(&drvdata->mutex))
		return -EBUSY;
	else {
#ifndef ENABLE_SENSORS_FPRINT_SECURE
		qbt2000_power_control(drvdata, 0);
#endif
		qbt2000_disable_debug_timer();
		pr_info("ret = %d\n", rc);
	}
	mutex_unlock(&drvdata->mutex);

	return 0;
}

static int qbt2000_resume(struct platform_device *pdev)
{
	int rc = 0;

#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge)
		return rc;
#endif

#if defined(ENABLE_SENSORS_FPRINT_SECURE)
	if (fpsensor_goto_suspend) {
		fps_resume_set();
	}
#else
	qbt2000_power_control(g_data, 1);
#endif
	qbt2000_enable_debug_timer();
	pr_info("ret = %d\n", rc);

	return 0;
}

static const struct of_device_id qbt2000_match[] = {
	{ .compatible = "qcom,qbt2000" },
	{}
};

static struct platform_driver qbt2000_plat_driver = {
	.probe = qbt2000_probe,
	.remove = qbt2000_remove,
	.suspend = qbt2000_suspend,
	.resume = qbt2000_resume,
	.driver = {
		.name = QBT2000_DEV,
		.owner = THIS_MODULE,
		.of_match_table = qbt2000_match,
	},
};

module_platform_driver(qbt2000_plat_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kangwook.Her");
MODULE_DESCRIPTION("Samsung Electronics Inc. QBT2000 driver");
