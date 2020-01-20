/*
 *
 * drivers/media/tdmb/tdmb.c
 *
 * tdmb driver
 *
 * Copyright (C) (2011, Samsung Electronics)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>

#include <linux/types.h>
#include <linux/fcntl.h>

/* for delay(sleep) */
#include <linux/delay.h>

/* for mutex */
#include <linux/mutex.h>

/*using copy to user */
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>

#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/input.h>
#include <linux/pm_qos.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>

#include <linux/of_gpio.h>
#if defined(CONFIG_TDMB_QUALCOMM)
#include <linux/cpuidle.h>
#include <soc/qcom/pm.h>
#endif

#if defined(CONFIG_TDMB_NOTIFIER)
#include <linux/tdmb_notifier.h>
#endif

#if defined(CONFIG_TDMB_ANT_DET)
static struct wake_lock tdmb_ant_wlock;
#endif

#define TDMB_WAKE_LOCK_ENABLE
#ifdef TDMB_WAKE_LOCK_ENABLE
#if defined(CONFIG_TDMB_QUALCOMM)
static struct pm_qos_request tdmb_pm_qos_req;
#endif
static struct wake_lock tdmb_wlock;
#endif
#include "tdmb.h"
#define TDMB_PRE_MALLOC 1

#ifndef VM_RESERVED	/* for kernel 3.10 */
#define VM_RESERVED (VM_DONTEXPAND | VM_DONTDUMP)
#endif

static struct class *tdmb_class;

/* ring buffer */
char *ts_ring;
unsigned int *tdmb_ts_head;
unsigned int *tdmb_ts_tail;
char *tdmb_ts_buffer;
unsigned int tdmb_ts_size;

unsigned int *cmd_head;
unsigned int *cmd_tail;
static char *cmd_buffer;
static unsigned int cmd_size;

static unsigned long tdmb_last_ch;

static struct tdmb_drv_func *tdmbdrv_func;
static struct tdmb_drv_data *tdmbdrv_data;
static struct tdmb_dt_platform_data *dt_pdata;
static struct device  *dmb_device;

static bool tdmb_pwr_on;

char *tdmb_utcmd_to_str(u32 cmd_type)
{
	switch (cmd_type) {
#ifdef TDMB_UNIT_TEST
	case TDMB_UTCMD_GET_ID:
		return TDMB_ENUM_STR(TDMB_UTCMD_GET_ID);
#endif
	default:
		return "unknown";
	}
}

#ifdef CONFIG_TDMB_VREG_SUPPORT
static int tdmb_vreg_init(struct device *dev)
{
	int rc = 0;

	DPRINTK("vdd_name : %s", dt_pdata->tdmb_vreg_name);
	dt_pdata->tdmb_vreg = regulator_get(dev, dt_pdata->tdmb_vreg_name);
	if (IS_ERR(dt_pdata->tdmb_vreg)) {
		DPRINTK("Failed to get tdmb_vreg\n");
		rc = -ENXIO;
		return rc;
	}
	rc = regulator_set_voltage(dt_pdata->tdmb_vreg, 1800000, 1800000);
	if (rc) {
		DPRINTK("regulator set_vtg failed rc=%d\n", rc);
		regulator_put(dt_pdata->tdmb_vreg);
		return rc;
	}
	return rc;
}

static void tdmb_vreg_onoff(bool onoff)
{
	int rc;

	if (onoff) {
		if (!regulator_is_enabled(dt_pdata->tdmb_vreg)) {
			rc = regulator_enable(dt_pdata->tdmb_vreg);
			if (rc)
				DPRINTK("tdmb_vreg enable failed rc=%d\n", rc);
		}
	} else {
		if (regulator_is_enabled(dt_pdata->tdmb_vreg)) {
			rc = regulator_disable(dt_pdata->tdmb_vreg);
			if (rc)
				DPRINTK("tdmb_vreg disable failed rc=%d\n", rc);
		}
	}
	DPRINTK("%s : (%d)\n", __func__, onoff);
}
#endif

static void tdmb_set_config_poweron(void)
{
	int rc;

	rc = gpio_request(dt_pdata->tdmb_en, "gpio_tdmb_en");
	if (rc < 0) {
		DPRINTK("%s: gpio %d request failed (%d)\n",
			__func__, dt_pdata->tdmb_en, rc);
		return;
	}
	if (dt_pdata->tdmb_use_irq) {
		rc = gpio_request(dt_pdata->tdmb_irq, "gpio_tdmb_irq");
		if (rc < 0) {
			DPRINTK("%s: gpio %d request failed (%d)\n",
				__func__, dt_pdata->tdmb_irq, rc);
			gpio_free(dt_pdata->tdmb_en);
			return;
		}
	}
	if (pinctrl_select_state(dt_pdata->tdmb_pinctrl, dt_pdata->pwr_on)) {
		DPRINTK("%s: Failed to configure tdmb_on\n", __func__);
		gpio_free(dt_pdata->tdmb_en);
		if (dt_pdata->tdmb_use_irq)
			gpio_free(dt_pdata->tdmb_irq);
	}

}

static void tdmb_set_config_poweroff(void)
{
	if (pinctrl_select_state(dt_pdata->tdmb_pinctrl, dt_pdata->pwr_off))
		DPRINTK("%s: Failed to configure tdmb_off\n", __func__);

	gpio_free(dt_pdata->tdmb_en);
	if (dt_pdata->tdmb_use_irq)
		gpio_free(dt_pdata->tdmb_irq);
}

static void tdmb_gpio_on(void)
{
	DPRINTK("%s\n", __func__);
#ifdef CONFIG_TDMB_VREG_SUPPORT
	tdmb_vreg_onoff(true);
#endif
	tdmb_set_config_poweron();

	gpio_direction_output(dt_pdata->tdmb_en, 0);
	usleep_range(1000, 1200);
	gpio_direction_output(dt_pdata->tdmb_en, 1);
	usleep_range(25000, 26000);

	if (dt_pdata->tdmb_use_rst) {
		gpio_direction_output(dt_pdata->tdmb_rst, 0);
		usleep_range(2000, 2100);
		gpio_direction_output(dt_pdata->tdmb_rst, 1);
		usleep_range(10000, 11000);
	}
}

static void tdmb_gpio_off(void)
{
	DPRINTK("%s\n", __func__);
#ifdef CONFIG_TDMB_VREG_SUPPORT
	tdmb_vreg_onoff(false);
#endif
	gpio_direction_output(dt_pdata->tdmb_en, 0);

	usleep_range(1000, 1200);
	if (dt_pdata->tdmb_use_rst)
		gpio_direction_output(dt_pdata->tdmb_rst, 0);

	tdmb_set_config_poweroff();
}

static bool tdmb_power_on(void)
{
	int param = 0;

	if (tdmb_create_databuffer(tdmbdrv_func->get_int_size()) == false) {
		DPRINTK("tdmb_create_databuffer fail\n");
		goto create_databuffer_fail;
	}
	if (tdmb_create_workqueue() == false) {
		DPRINTK("tdmb_create_workqueue fail\n");
		goto create_workqueue_fail;
	}
#ifdef CONFIG_TDMB_XTAL_FREQ
	param = dt_pdata->tdmb_xtal_freq;
#endif
	if (tdmbdrv_func->power_on(param) == false) {
		DPRINTK("power_on fail\n");
		goto power_on_fail;
	}

	DPRINTK("power_on success\n");
#ifdef TDMB_WAKE_LOCK_ENABLE
#if defined(CONFIG_TDMB_QUALCOMM)
	pm_qos_update_request(&tdmb_pm_qos_req,
				  msm_cpuidle_get_deep_idle_latency());
#endif
	wake_lock(&tdmb_wlock);
#endif
	tdmb_pwr_on = true;
	return true;

power_on_fail:
	tdmb_destroy_workqueue();
create_workqueue_fail:
	tdmb_destroy_databuffer();
create_databuffer_fail:
	tdmb_pwr_on = false;

	return false;
}

static DEFINE_MUTEX(tdmb_lock);
static bool tdmb_power_off(void)
{
	DPRINTK("%s : tdmb_pwr_on(%d)\n", __func__, tdmb_pwr_on);
	if (tdmb_pwr_on) {
		tdmb_pwr_on = false;
		tdmbdrv_func->power_off();
		tdmb_destroy_workqueue();
		tdmb_destroy_databuffer();
#ifdef TDMB_WAKE_LOCK_ENABLE
		wake_unlock(&tdmb_wlock);
#if defined(CONFIG_TDMB_QUALCOMM)
		pm_qos_update_request(&tdmb_pm_qos_req, PM_QOS_DEFAULT_VALUE);
#endif
#endif
	}
	tdmb_last_ch = 0;

	return true;
}

static int tdmb_open(struct inode *inode, struct file *filp)
{
	DPRINTK("%s\n", __func__);
	return 0;
}

static ssize_t
tdmb_read(struct file *file, char __user *buf, size_t nbytes, loff_t *ppos)
{
	DPRINTK("%s\n", __func__);

	return 0;
}

static int tdmb_release(struct inode *inode, struct file *filp)
{
	DPRINTK("%s\n", __func__);
	mutex_lock(&tdmb_lock);
	tdmb_power_off();
	mutex_unlock(&tdmb_lock);

#if TDMB_PRE_MALLOC
	tdmb_ts_size = 0;
	cmd_size = 0;
#else
	if (ts_ring != 0) {
		kfree(ts_ring);
		ts_ring = 0;
		tdmb_ts_size = 0;
		cmd_size = 0;
	}
#endif
	return 0;
}

#if TDMB_PRE_MALLOC
static void tdmb_make_ring_buffer(void)
{
	size_t size = TDMB_RING_BUFFER_MAPPING_SIZE;

	/* size should aligned in PAGE_SIZE */
	if (size % PAGE_SIZE) /* klaatu hard coding */
		size = size + size % PAGE_SIZE;

	ts_ring = kzalloc(size, GFP_KERNEL);
	if (!ts_ring) {
		DPRINTK("RING Buff Create Fail\n");
		return;
	}
	DPRINTK("RING Buff Create OK\n");
}

#endif

static int tdmb_mmap(struct file *filp, struct vm_area_struct *vma)
{
	size_t size;
	unsigned long pfn;

	DPRINTK("%s\n", __func__);

	vma->vm_flags |= VM_RESERVED;
	size = vma->vm_end - vma->vm_start;
	if (size > TDMB_RING_BUFFER_MAPPING_SIZE) {
		DPRINTK("over size given : %lx\n", size);
		return -EAGAIN;
	}
	DPRINTK("size given : %lx\n", size);

#if TDMB_PRE_MALLOC
	size = TDMB_RING_BUFFER_MAPPING_SIZE;
	if (!ts_ring) {
		DPRINTK("RING Buff ReAlloc(%ld)!!\n", size);
#endif
		/* size should aligned in PAGE_SIZE */
		if (size % PAGE_SIZE) /* klaatu hard coding */
			size = size + size % PAGE_SIZE;

		ts_ring = kzalloc(size, GFP_KERNEL);
		if (!ts_ring) {
			DPRINTK("RING Buff ReAlloc Fail\n");
			return -ENOMEM;
		}
#if TDMB_PRE_MALLOC
	}
#endif

	pfn = virt_to_phys(ts_ring) >> PAGE_SHIFT;

	if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot))
		return -EAGAIN;

	tdmb_ts_head = (unsigned int *)ts_ring;
	tdmb_ts_tail = (unsigned int *)(ts_ring + 4);
	tdmb_ts_buffer = ts_ring + 8;

	*tdmb_ts_head = 0;
	*tdmb_ts_tail = 0;

	tdmb_ts_size = size-8; /* klaatu hard coding */
	tdmb_ts_size
	= ((tdmb_ts_size / DMB_TS_SIZE) * DMB_TS_SIZE) - (30 * DMB_TS_SIZE);

	cmd_buffer = tdmb_ts_buffer + tdmb_ts_size + 8;
	cmd_head = (unsigned int *)(cmd_buffer - 8);
	cmd_tail = (unsigned int *)(cmd_buffer - 4);

	*cmd_head = 0;
	*cmd_tail = 0;

	cmd_size = 30 * DMB_TS_SIZE - 8; /* klaatu hard coding */

	DPRINTK("succeeded\n");
	return 0;
}


static int _tdmb_cmd_update(
	unsigned char *cmd_header,
	unsigned char cmd_header_size,
	unsigned char *data,
	unsigned short data_size)
{
	unsigned int size;
	unsigned int head;
	unsigned int tail;
	unsigned int dist;
	unsigned int temp_size;
	unsigned int data_size_tmp;

	if (data_size > cmd_size) {
		DPRINTK(" Error - cmd size too large\n");
		return false;
	}

	head = *cmd_head;
	tail = *cmd_tail;
	size = cmd_size;
	data_size_tmp = data_size + cmd_header_size;

	if (head >= tail)
		dist = head-tail;
	else
		dist = size + head-tail;

	if (size - dist <= data_size_tmp) {
		DPRINTK("too small space is left in Cmd Ring Buffer!!\n");
		return false;
	}

	DPRINTK("head %d tail %d\n", head, tail);

	if (head+data_size_tmp <= size) {
		memcpy((cmd_buffer + head),
			(char *)cmd_header, cmd_header_size);
		memcpy((cmd_buffer + head + cmd_header_size),
			(char *)data, data_size);
		head += data_size_tmp;
		if (head == size)
			head = 0;
	} else {
		temp_size = size - head;
		if (temp_size < cmd_header_size) {
			memcpy((cmd_buffer+head),
				(char *)cmd_header, temp_size);
			memcpy((cmd_buffer),
				(char *)cmd_header+temp_size,
				(cmd_header_size - temp_size));
			head = cmd_header_size - temp_size;
		} else {
			memcpy((cmd_buffer+head),
				(char *)cmd_header, cmd_header_size);
			head += cmd_header_size;
			if (head == size)
				head = 0;
		}

		temp_size = size - head;
		if (temp_size < data_size) {
			memcpy((cmd_buffer+head),
				(char *)data, temp_size);
			memcpy((cmd_buffer),
				(char *)data+temp_size,
				(data_size - temp_size));
			head = data_size - temp_size;
		} else {
			memcpy((cmd_buffer+head),
				(char *)data, data_size);
			head += data_size;
			if (head == size)
				head = 0;
		}
	}

	*cmd_head = head;

	return true;
}

unsigned char tdmb_make_result(
	unsigned char cmd,
	unsigned short data_len,
	unsigned char  *data)
{
	unsigned char cmd_header[4] = {0,};

	cmd_header[0] = TDMB_CMD_START_FLAG;
	cmd_header[1] = cmd;
	cmd_header[2] = (data_len>>8)&0xff;
	cmd_header[3] = data_len&0xff;

	_tdmb_cmd_update(cmd_header, 4, data, data_len);

	return true;
}

unsigned long tdmb_get_chinfo(void)
{
	return tdmb_last_ch;
}

void tdmb_pull_data(struct work_struct *work)
{
	if (tdmb_pwr_on)
		tdmbdrv_func->pull_data();
}

bool tdmb_control_irq(bool set)
{
	bool ret = true;
	int irq_ret;

	if (!dt_pdata->tdmb_use_irq)
		return false;

	if (set) {
		irq_set_irq_type(gpio_to_irq(dt_pdata->tdmb_irq), IRQ_TYPE_EDGE_FALLING);
		irq_ret = request_irq(gpio_to_irq(dt_pdata->tdmb_irq)
						, tdmb_irq_handler
						, 0
						, TDMB_DEV_NAME
						, NULL);
		if (irq_ret < 0) {
			DPRINTK("request_irq failed !! \r\n");
			ret = false;
		}
	} else {
		free_irq(gpio_to_irq(dt_pdata->tdmb_irq), NULL);
	}

	return ret;
}

void tdmb_control_gpio(bool poweron)
{
#if defined(CONFIG_TDMB_NOTIFIER)
	struct tdmb_notifier_struct tdmb_notifier;

	tdmb_notifier.event = TDMB_NOTIFY_EVENT_TUNNER;
	tdmb_notifier.tdmb_status.pwr = poweron;
	tdmb_notifier_call(&tdmb_notifier);
#endif

	if (poweron)
		tdmb_gpio_on();
	else
		tdmb_gpio_off();
}

static long tdmb_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	unsigned long fig_freq = 0;
	struct ensemble_info_type *ensemble_info;
	struct tdmb_dm dm_buff;

	DPRINTK("call %s : 0x%x\n", __func__, cmd);

	if (_IOC_TYPE(cmd) != IOCTL_MAGIC) {
		DPRINTK("%s : _IOC_TYPE error\n", __func__);
		return -EINVAL;
	}
	if (_IOC_NR(cmd) >= IOCTL_MAXNR) {
		DPRINTK("%s : _IOC_NR(cmd) 0x%x\n", __func__, _IOC_NR(cmd));
		return -EINVAL;
	}

	switch (cmd) {
	case IOCTL_TDMB_GET_DATA_BUFFSIZE:
		DPRINTK("IOCTL_TDMB_GET_DATA_BUFFSIZE %d\n", tdmb_ts_size);
		ret = copy_to_user((unsigned int *)arg,
				&tdmb_ts_size, sizeof(unsigned int));
		break;

	case IOCTL_TDMB_GET_CMD_BUFFSIZE:
		DPRINTK("IOCTL_TDMB_GET_CMD_BUFFSIZE %d\n", cmd_size);
		ret = copy_to_user((unsigned int *)arg,
				&cmd_size, sizeof(unsigned int));
		break;

	case IOCTL_TDMB_POWER_ON:
		DPRINTK("IOCTL_TDMB_POWER_ON\n");
		mutex_lock(&tdmb_lock);
		ret = tdmb_power_on();
		mutex_unlock(&tdmb_lock);
		break;

	case IOCTL_TDMB_POWER_OFF:
		DPRINTK("IOCTL_TDMB_POWER_OFF\n");
		mutex_lock(&tdmb_lock);
		ret = tdmb_power_off();
		mutex_unlock(&tdmb_lock);
		break;

	case IOCTL_TDMB_SCAN_FREQ_ASYNC:
#if 0
		mutex_lock(&tdmb_lock);
		if (!tdmb_pwr_on) {
			DPRINTK("IOCTL_TDMB_SCAN_FREQ_ASYNC-Not ready\n");
			mutex_unlock(&tdmb_lock);
			break;
		}
		DPRINTK("IOCTL_TDMB_SCAN_FREQ_ASYNC\n");
		fig_freq = arg;

		ensemble_info = vmalloc(sizeof(struct ensemble_info_type));
		memset((char *)ensemble_info, 0x00, sizeof(struct ensemble_info_type));

		ret = tdmbdrv_func->scan_ch(ensemble_info, fig_freq);
		if (ret == true)
			tdmb_make_result(DMB_FIC_RESULT_DONE,
				sizeof(struct ensemble_info_type),
				(unsigned char *)ensemble_info);
		else
			tdmb_make_result(DMB_FIC_RESULT_FAIL,
				sizeof(unsigned long),
				(unsigned char *)&fig_freq);

		vfree(ensemble_info);
		tdmb_last_ch = 0;
		mutex_unlock(&tdmb_lock);
#endif
		DPRINTK("IOCTL_TDMB_SCAN_FREQ_ASYNC - blocked\n");
		break;

	case IOCTL_TDMB_SCAN_FREQ_SYNC:
		mutex_lock(&tdmb_lock);
		if (!tdmb_pwr_on) {
			DPRINTK("IOCTL_TDMB_SCAN_FREQ_SYNC-Not ready\n");
			mutex_unlock(&tdmb_lock);
			break;
		}
		ensemble_info = vmalloc(sizeof(struct ensemble_info_type));
		if (ensemble_info == NULL) {
			DPRINTK("IOCTL_TDMB_SCAN_FREQ_SYNC-vmalloc fail\n");
			mutex_unlock(&tdmb_lock);
			break;
		}	
		if (copy_from_user(ensemble_info, (void *)arg, sizeof(struct ensemble_info_type)))
			DPRINTK("cmd(%x):copy_from_user failed\n", cmd);
		else
			fig_freq = ensemble_info->ensem_freq;
		DPRINTK("IOCTL_TDMB_SCAN_FREQ_SYNC %ld\n", fig_freq);
		memset((char *)ensemble_info, 0x00, sizeof(struct ensemble_info_type));

		ret = tdmbdrv_func->scan_ch(ensemble_info, fig_freq);
		if (ret == true) {
			if (copy_to_user((struct ensemble_info_type *)arg,
					ensemble_info,
					sizeof(struct ensemble_info_type))
				)
				DPRINTK("cmd(%x):copy_to_user failed\n", cmd);
		}

		vfree(ensemble_info);
		tdmb_last_ch = 0;
		mutex_unlock(&tdmb_lock);
		break;

	case IOCTL_TDMB_SCANSTOP:
		DPRINTK("IOCTL_TDMB_SCANSTOP\n");
		ret = false;
		break;

	case IOCTL_TDMB_ASSIGN_CH:
		mutex_lock(&tdmb_lock);
		if (!tdmb_pwr_on) {
			DPRINTK("IOCTL_TDMB_ASSIGN_CH-Not ready\n");
			mutex_unlock(&tdmb_lock);
			break;
		}
		DPRINTK("IOCTL_TDMB_ASSIGN_CH %ld\n", arg);
		tdmb_init_data();
		ret = tdmbdrv_func->set_ch(arg, (arg % 1000), false);
		if (ret == true)
			tdmb_last_ch = arg;
		else
			tdmb_last_ch = 0;
		mutex_unlock(&tdmb_lock);
		break;

	case IOCTL_TDMB_ASSIGN_CH_TEST:
		mutex_lock(&tdmb_lock);
		if (!tdmb_pwr_on) {
			DPRINTK("IOCTL_TDMB_ASSIGN_CH_TEST-Not ready\n");
			mutex_unlock(&tdmb_lock);
			break;
		}
		DPRINTK("IOCTL_TDMB_ASSIGN_CH_TEST %ld\n", arg);
		tdmb_init_data();
		ret = tdmbdrv_func->set_ch(arg, (arg % 1000), true);
		if (ret == true)
			tdmb_last_ch = arg;
		else
			tdmb_last_ch = 0;
		mutex_unlock(&tdmb_lock);
		break;

	case IOCTL_TDMB_GET_DM:
		mutex_lock(&tdmb_lock);
		if (!tdmb_pwr_on) {
			DPRINTK("IOCTL_TDMB_GET_DM-Not ready\n");
			mutex_unlock(&tdmb_lock);
			break;
		}
		tdmbdrv_func->get_dm(&dm_buff);
		if (copy_to_user((struct tdmb_dm *)arg, &dm_buff, sizeof(struct tdmb_dm)))
			DPRINTK("IOCTL_TDMB_GET_DM : copy_to_user failed\n");
		ret = true;
		DPRINTK("rssi %d, ber %d, ANT %d\n",
			dm_buff.rssi, dm_buff.ber, dm_buff.antenna);
		mutex_unlock(&tdmb_lock);
		break;
	case IOCTL_TDMB_SET_AUTOSTART:
		DPRINTK("IOCTL_TDMB_SET_AUTOSTART : %ld\n", arg);
#if defined(CONFIG_TDMB_ANT_DET)
		tdmb_ant_det_irq_set(arg);
#endif
		break;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long tdmb_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	DPRINTK("call %s : 0x%x\n", __func__, cmd);
	arg = (unsigned long) compat_ptr(arg);

	switch (cmd) {
	case IOCTL_TDMB_GET_DATA_BUFFSIZE:
	case IOCTL_TDMB_GET_CMD_BUFFSIZE:
	case IOCTL_TDMB_POWER_ON:
	case IOCTL_TDMB_POWER_OFF:
	case IOCTL_TDMB_SCAN_FREQ_ASYNC:
	case IOCTL_TDMB_SCAN_FREQ_SYNC:
	case IOCTL_TDMB_SCANSTOP:
	case IOCTL_TDMB_ASSIGN_CH:
	case IOCTL_TDMB_ASSIGN_CH_TEST:
	case IOCTL_TDMB_GET_DM:
	case IOCTL_TDMB_SET_AUTOSTART:
		return tdmb_ioctl(filp, cmd, arg);
	}
	return -ENOIOCTLCMD;
}
#endif

static const struct file_operations tdmb_ctl_fops = {
	.owner          = THIS_MODULE,
	.open           = tdmb_open,
	.read           = tdmb_read,
	.unlocked_ioctl  = tdmb_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= tdmb_compat_ioctl,
#endif
	.mmap           = tdmb_mmap,
	.release	    = tdmb_release,
	.llseek         = no_llseek,
};

static struct tdmb_drv_func *tdmb_get_drv_func(void)
{
	struct tdmb_drv_func * (*func)(void);

#if defined(CONFIG_TDMB_FC8050)
	func = fc8050_drv_func;
#elif defined(CONFIG_TDMB_FC8080)
	func = fc8080_drv_func;
#elif defined(CONFIG_TDMB_MTV318)
	func = mtv318_drv_func;
#elif defined(CONFIG_TDMB_MTV319)
	func = mtv319_drv_func;
#elif defined(CONFIG_TDMB_TCC3170)
	func = tcc3170_drv_func;
#else
	#error what???
#endif
	return func();
}

#if defined(CONFIG_TDMB_ANT_DET)
enum {
	TDMB_ANT_OPEN = 0,
	TDMB_ANT_CLOSE,
	TDMB_ANT_UNKNOWN,
};
enum {
	TDMB_ANT_DET_LOW = 0,
	TDMB_ANT_DET_HIGH,
};

static struct input_dev *tdmb_ant_input;
static int tdmb_check_ant;
static int ant_prev_status;
static int ant_irq_ret = -1;

#define TDMB_ANT_WAIT_INIT_TIME	500000 /* us */
#define TDMB_ANT_CHECK_DURATION 50000 /* us */
#define TDMB_ANT_CHECK_COUNT 10
#define TDMB_ANT_WLOCK_TIMEOUT \
		((TDMB_ANT_CHECK_DURATION * TDMB_ANT_CHECK_COUNT * 2) / 500000)
static int tdmb_ant_det_check_value(void)
{
	int loop = 0, cur_val = 0;
	int ret = TDMB_ANT_UNKNOWN;

	tdmb_check_ant = 1;

	DPRINTK("%s ant_prev_status(%d)\n",
		__func__, ant_prev_status);

	usleep_range(TDMB_ANT_WAIT_INIT_TIME, TDMB_ANT_WAIT_INIT_TIME + 1000); /* wait initial noise */

	for (loop = 0; loop < TDMB_ANT_CHECK_COUNT; loop++) {
		usleep_range(TDMB_ANT_CHECK_DURATION, TDMB_ANT_CHECK_DURATION + 1000);
		cur_val = gpio_get_value_cansleep(dt_pdata->tdmb_ant_irq);
		if (ant_prev_status == cur_val)
			break;
	}

	if (loop == TDMB_ANT_CHECK_COUNT) {
		if (ant_prev_status == TDMB_ANT_DET_LOW
				&& cur_val == TDMB_ANT_DET_HIGH) {
			ret = TDMB_ANT_OPEN;
		} else if (ant_prev_status == TDMB_ANT_DET_HIGH
				&& cur_val == TDMB_ANT_DET_LOW) {
			ret = TDMB_ANT_CLOSE;
		}

		ant_prev_status = cur_val;
	}

	tdmb_check_ant = 0;

	DPRINTK("%s cnt(%d) cur(%d) prev(%d)\n",
		__func__, loop, cur_val, ant_prev_status);

	return ret;
}

static int tdmb_ant_det_ignore_irq(void)
{
	DPRINTK("chk_ant=%d\n", tdmb_check_ant);
	return tdmb_check_ant;
}

static void tdmb_ant_det_work_func(struct work_struct *work)
{
	if (!tdmb_ant_input) {
		DPRINTK("%s: input device is not registered\n", __func__);
		return;
	}

	switch (tdmb_ant_det_check_value()) {
	case TDMB_ANT_OPEN:
		input_report_key(tdmb_ant_input, KEY_DMB_ANT_DET_UP, 1);
		input_report_key(tdmb_ant_input, KEY_DMB_ANT_DET_UP, 0);
		input_sync(tdmb_ant_input);
		DPRINTK("%s : TDMB_ANT_OPEN\n", __func__);
		break;
	case TDMB_ANT_CLOSE:
		input_report_key(tdmb_ant_input, KEY_DMB_ANT_DET_DOWN, 1);
		input_report_key(tdmb_ant_input, KEY_DMB_ANT_DET_DOWN, 0);
		input_sync(tdmb_ant_input);
		DPRINTK("%s : TDMB_ANT_CLOSE\n", __func__);
		break;
	case TDMB_ANT_UNKNOWN:
		DPRINTK("%s : TDMB_ANT_UNKNOWN\n", __func__);
		break;
	default:
		break;
	}
}

static struct workqueue_struct *tdmb_ant_det_wq;
static DECLARE_WORK(tdmb_ant_det_work, tdmb_ant_det_work_func);
static bool tdmb_ant_det_reg_input(struct platform_device *pdev)
{
	struct input_dev *input;
	int err;

	DPRINTK("%s\n", __func__);

	input = input_allocate_device();
	if (!input) {
		DPRINTK("Can't allocate input device\n");
		err = -ENOMEM;
	}
	set_bit(EV_KEY, input->evbit);
	set_bit(KEY_DMB_ANT_DET_UP & KEY_MAX, input->keybit);
	set_bit(KEY_DMB_ANT_DET_DOWN & KEY_MAX, input->keybit);
	input->name = "sec_dmb_key";
	input->phys = "sec_dmb_key/input0";
	input->dev.parent = &pdev->dev;

	err = input_register_device(input);
	if (err) {
		DPRINTK("Can't register dmb_ant_det key: %d\n", err);
		goto free_input_dev;
	}
	tdmb_ant_input = input;

	return true;

free_input_dev:
	input_free_device(input);
	return false;
}

static void tdmb_ant_det_unreg_input(void)
{
	DPRINTK("%s\n", __func__);
	if (tdmb_ant_input) {
		input_unregister_device(tdmb_ant_input);
		tdmb_ant_input = NULL;
	}
}
static bool tdmb_ant_det_create_wq(void)
{
	DPRINTK("%s\n", __func__);
	tdmb_ant_det_wq = create_singlethread_workqueue("tdmb_ant_det_wq");
	if (tdmb_ant_det_wq)
		return true;
	else
		return false;
}

static bool tdmb_ant_det_destroy_wq(void)
{
	DPRINTK("%s\n", __func__);
	if (tdmb_ant_det_wq) {
		flush_workqueue(tdmb_ant_det_wq);
		destroy_workqueue(tdmb_ant_det_wq);
		tdmb_ant_det_wq = NULL;
	}
	return true;
}

static irqreturn_t tdmb_ant_det_irq_handler(int irq, void *dev_id)
{
	int ret = 0;

	if (tdmb_ant_det_ignore_irq())
		return IRQ_HANDLED;

	wake_lock_timeout(&tdmb_ant_wlock, TDMB_ANT_WLOCK_TIMEOUT * HZ);

	if (tdmb_ant_det_wq) {
		ret = queue_work(tdmb_ant_det_wq, &tdmb_ant_det_work);
		if (ret == 0)
			DPRINTK("%s queue_work fail\n", __func__);
	}

	return IRQ_HANDLED;
}

bool tdmb_ant_det_irq_set(bool set)
{
	bool ret = true;

	DPRINTK("%s : set(%d) ant_irq(%d)\n", __func__, set, ant_irq_ret);

	if (set) {
		if (ant_irq_ret < 0) {
			ant_prev_status =
				gpio_get_value_cansleep(dt_pdata->tdmb_ant_irq);

			irq_set_irq_type(gpio_to_irq(dt_pdata->tdmb_ant_irq)
					, IRQ_TYPE_EDGE_BOTH);

			ant_irq_ret = request_irq(gpio_to_irq(dt_pdata->tdmb_ant_irq)
						, tdmb_ant_det_irq_handler
						, IRQF_DISABLED
						, "tdmb_ant_det"
						, NULL);
			if (ant_irq_ret < 0) {
				DPRINTK("%s %d\r\n", __func__, ant_irq_ret);
				ret = false;
			} else {
				enable_irq_wake(gpio_to_irq(dt_pdata->tdmb_ant_irq));
			}
		}
	} else {
		if (ant_irq_ret >= 0) {
			disable_irq_wake(gpio_to_irq(dt_pdata->tdmb_ant_irq));
			free_irq(gpio_to_irq(dt_pdata->tdmb_ant_irq), NULL);
			ant_irq_ret = -1;
			ret = false;
		}
	}

	return ret;
}
#endif
static struct tdmb_dt_platform_data *get_tdmb_dt_pdata(struct device *dev)
{
	struct tdmb_dt_platform_data *pdata;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		DPRINTK("%s : could not allocate memory for platform data\n", __func__);
		goto err;
	}
	pdata->tdmb_en = of_get_named_gpio(dev->of_node, "tdmb_pwr_en", 0);
	if (!gpio_is_valid(pdata->tdmb_en)) {
		DPRINTK("Failed to get is valid tdmb_en\n");
		goto alloc_err;
	}
	pdata->tdmb_use_rst = of_property_read_bool(dev->of_node, "tdmb_use_rst");
	if (pdata->tdmb_use_rst) {
		pdata->tdmb_rst = of_get_named_gpio(dev->of_node, "tdmb_rst", 0);
		if (!gpio_is_valid(pdata->tdmb_rst)) {
			DPRINTK("Failed to get is valid tdmb_rst\n");
			goto alloc_err;
		}
	} else {
		DPRINTK("%s : without tdmb_use_rst\n", __func__);
	}
	pdata->tdmb_use_irq = of_property_read_bool(dev->of_node, "tdmb_use_irq");
	if (pdata->tdmb_use_irq) {
		pdata->tdmb_irq = of_get_named_gpio(dev->of_node, "tdmb_irq", 0);
		if (!gpio_is_valid(pdata->tdmb_irq)) {
			DPRINTK("Failed to get is valid tdmb_irq\n");
			goto alloc_err;
		}
	} else {
		DPRINTK("%s : without tdmb_use_irq\n", __func__);
	}
#ifdef CONFIG_TDMB_XTAL_FREQ
	if (of_property_read_u32(dev->of_node, "tdmb_xtal_freq",
							&pdata->tdmb_xtal_freq)) {
		DPRINTK("Failed to get tdmb_xtal_freq\n");
		goto alloc_err;
	}
#endif
	pdata->tdmb_pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(pdata->tdmb_pinctrl)) {
		DPRINTK("devm_pinctrl_get is fail");
		goto alloc_err;
	}
	pdata->pwr_on = pinctrl_lookup_state(pdata->tdmb_pinctrl, "tdmb_on");
	if (IS_ERR(pdata->pwr_on)) {
		DPRINTK("%s : could not get pins tdmb_pwr_on state (%li)\n",
			__func__, PTR_ERR(pdata->pwr_on));
		goto err_pinctrl_lookup_state;
	}
	pdata->pwr_off = pinctrl_lookup_state(pdata->tdmb_pinctrl, "tdmb_off");
	if (IS_ERR(pdata->pwr_off)) {
		DPRINTK("%s : could not get pins tdmb_pwr_off state (%li)\n",
			__func__, PTR_ERR(pdata->pwr_off));
		goto err_pinctrl_lookup_state;
	}

#ifdef CONFIG_TDMB_ANT_DET
	pdata->tdmb_ant_irq = of_get_named_gpio(dev->of_node, "tdmb_ant_irq", 0);
	if (!gpio_is_valid(pdata->tdmb_ant_irq)) {
		DPRINTK("Failed to get is valid tdmb_ant_irq\n");
		goto alloc_err;
	}
#endif
#ifdef CONFIG_TDMB_VREG_SUPPORT
	if (of_property_read_string(dev->of_node,
			"tdmb_vreg_supply", &pdata->tdmb_vreg_name)) {
		DPRINTK("Unable to find tdmb_vdd_supply\n");
		goto alloc_err;
	}
#endif
	return pdata;

err_pinctrl_lookup_state:
	devm_pinctrl_put(pdata->tdmb_pinctrl);
alloc_err:
	devm_kfree(dev, pdata);
err:
	return NULL;
}

#ifdef TDMB_UNIT_TEST
static bool tdmb_unit_test_get_id(void)
{
	bool ret = false;

	DPRINTK("%s: +++\n", __func__);

	//.TODO:





	return ret;
}

static ssize_t tdmb_unit_test_show(struct class *class,
				struct class_attribute *attr, char *buf)
{
	struct tdmb_drv_data *pdata = tdmbdrv_data;
	int rc, cmd = pdata->test_cmd;
	bool res = false;

	DPRINTK("%s: test_cmd: %s\n", __func__, tdmb_utcmd_to_str(cmd));

	switch (cmd) {
	case TDMB_UTCMD_GET_ID:
		res = tdmb_unit_test_get_id();
		break;
	default:
		DPRINTK("%s: invalid test_cmd: %d\n", __func__, cmd);
		break;
	}

	rc = scnprintf(buf, 3, "%d\n", res ? 1 : 0);
	return rc;
}

static ssize_t tdmb_unit_test_store(struct class *dev,
				struct class_attribute *attr, const char *buf, size_t size)
{
	struct tdmb_drv_data *pdata = tdmbdrv_data;
	int val[10] = {0, };

	get_options(buf, 10, val);
	pdata->test_cmd = val[1];

	DPRINTK("%s: test_cmd: %d...%s\n", __func__, pdata->test_cmd,
		tdmb_utcmd_to_str(pdata->test_cmd));

	return size;
}

static CLASS_ATTR_RW(tdmb_unit_test);
#endif

static int tdmb_probe(struct platform_device *pdev)
{
	int ret;
	struct device *tdmb_dev;
	struct tdmb_drv_data *pdata;

	DPRINTK("call %s\n", __func__);

#if defined(CONFIG_TDMB_TSIF_QC)
	tdmb_tsi_init();
#endif

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		DPRINTK("%s : Fail to allocate memory for drv data\n", __func__);
		return -ENOMEM;
	}

	dt_pdata = get_tdmb_dt_pdata(&pdev->dev);
	if (!dt_pdata) {
		pr_err("%s : tdmb_dt_pdata is NULL.\n", __func__);
		return -ENODEV;
	}
	if (pinctrl_select_state(dt_pdata->tdmb_pinctrl, dt_pdata->pwr_off))
		DPRINTK("%s: Failed to configure tdmb_gpio_init\n", __func__);

	dmb_device = &pdev->dev;

	ret = register_chrdev(TDMB_DEV_MAJOR, TDMB_DEV_NAME, &tdmb_ctl_fops);
	if (ret < 0)
		DPRINTK("register_chrdev(TDMB_DEV) failed!\n");

	tdmb_class = class_create(THIS_MODULE, TDMB_DEV_NAME);
	if (IS_ERR(tdmb_class)) {
		unregister_chrdev(TDMB_DEV_MAJOR, TDMB_DEV_NAME);
		DPRINTK("class_create failed!\n");

		return -EFAULT;
	}
#ifdef TDMB_UNIT_TEST
	ret = class_create_file(tdmb_class, &class_attr_tdmb_unit_test);
	if (ret)
		pr_err("failed to create attr_unit_test(%d)\n", ret);
#endif

	tdmb_dev = device_create(tdmb_class, NULL,
				MKDEV(TDMB_DEV_MAJOR, TDMB_DEV_MINOR),
				NULL, TDMB_DEV_NAME);
	if (IS_ERR(tdmb_dev)) {
		DPRINTK("device_create failed!\n");

		unregister_chrdev(TDMB_DEV_MAJOR, TDMB_DEV_NAME);
		class_destroy(tdmb_class);

		return -EFAULT;
	}
	tdmbdrv_func = tdmb_get_drv_func();
	if (tdmbdrv_func->init)
		tdmbdrv_func->init();

#if TDMB_PRE_MALLOC
	tdmb_make_ring_buffer();
#endif
#ifdef TDMB_WAKE_LOCK_ENABLE
#if defined(CONFIG_TDMB_QUALCOMM)
	pm_qos_add_request(&tdmb_pm_qos_req, PM_QOS_CPU_DMA_LATENCY,
				PM_QOS_DEFAULT_VALUE);
#endif
	wake_lock_init(&tdmb_wlock, WAKE_LOCK_SUSPEND, "tdmb_wlock");
#endif

#ifdef CONFIG_TDMB_VREG_SUPPORT
	ret = tdmb_vreg_init(&pdev->dev);
	if (ret) {
		DPRINTK("tdmb_vreg_init failed!\n");
		return -ENXIO;
	}
#endif

#if defined(CONFIG_TDMB_ANT_DET)
	wake_lock_init(&tdmb_ant_wlock, WAKE_LOCK_SUSPEND, "tdmb_ant_wlock");

	if (!tdmb_ant_det_reg_input(pdev))
		goto err_reg_input;
	if (!tdmb_ant_det_create_wq())
		goto free_reg_input;
	return 0;

free_reg_input:
	tdmb_ant_det_unreg_input();
err_reg_input:
	return -EFAULT;
#endif

	tdmbdrv_data = pdata;
	return 0;
}

static int tdmb_remove(struct platform_device *pdev)
{
	DPRINTK("%s!\n", __func__);
#if defined(CONFIG_TDMB_ANT_DET)
	tdmb_ant_det_unreg_input();
	tdmb_ant_det_destroy_wq();
	tdmb_ant_det_irq_set(false);
	wake_lock_destroy(&tdmb_ant_wlock);
#endif

#if defined(CONFIG_TDMB_TSIF_QC)
	tdmb_tsi_deinit();
#endif
#ifdef CONFIG_TDMB_VREG_SUPPORT
	regulator_put(dt_pdata->tdmb_vreg);
#endif

	if (!tdmbdrv_data) {
		devm_kfree(&pdev->dev, tdmbdrv_data);
		tdmbdrv_data = NULL;
	}
	return 0;
}

static int tdmb_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int tdmb_resume(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id tdmb_match_table[] = {
	{.compatible = "samsung,tdmb"},
	{}
};

static struct platform_driver tdmb_driver = {
	.probe	= tdmb_probe,
	.remove = tdmb_remove,
	.suspend = tdmb_suspend,
	.resume = tdmb_resume,
	.driver = {
		.owner	= THIS_MODULE,
		.name = "tdmb",
		.of_match_table = tdmb_match_table,
	},
};

static int __init tdmb_init(void)
{
	int ret;

	DPRINTK("<klaatu TDMB> module init\n");
	ret = platform_driver_register(&tdmb_driver);
	if (ret)
		return ret;
	return 0;
}

static void __exit tdmb_exit(void)
{
	DPRINTK("<klaatu TDMB> module exit\n");
#if TDMB_PRE_MALLOC
	if (ts_ring != 0) {
		kfree(ts_ring);
		ts_ring = 0;
	}
#endif
	unregister_chrdev(TDMB_DEV_MAJOR, "tdmb");
	device_destroy(tdmb_class, MKDEV(TDMB_DEV_MAJOR, TDMB_DEV_MINOR));
	class_destroy(tdmb_class);

	platform_driver_unregister(&tdmb_driver);

#ifdef TDMB_WAKE_LOCK_ENABLE
#if defined(CONFIG_TDMB_QUALCOMM)
	pm_qos_remove_request(&tdmb_pm_qos_req);
#endif
	wake_lock_destroy(&tdmb_wlock);
#endif
}

module_init(tdmb_init);
module_exit(tdmb_exit);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("TDMB Driver");
MODULE_LICENSE("GPL v2");
