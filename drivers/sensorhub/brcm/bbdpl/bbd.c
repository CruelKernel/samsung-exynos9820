/*
 * Copyright 2014 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * The BBD (Broadcom Bridge Driver)
 *
 * tabstop = 8
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/circ_buf.h>
#include <linux/fs.h>
#include <linux/wakelock.h>
#include <linux/suspend.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/string.h>
#include <linux/sched/signal.h>	/* For send_sig(), same_thread_group(), etc. */
#include "bbd.h"

#ifdef CONFIG_SENSORS_SSP
#include <linux/spi/spi.h>	//Needs because SSP is tightly coupled with SPI
//#include <linux/spidev.h>
#include <linux/spu-verify.h>

extern struct spi_driver *pssp_driver;
extern bool ssp_dbg;
extern bool ssp_pkt_dbg;

static struct spi_device dummy_spi = {
	.dev = {
		.init_name = "dummy",
	},
};
#endif

#ifdef CONFIG_BCM_GPS_SPI_DRIVER
extern bool ssi_dbg;
extern bool ssi_dbg_pzc;
extern bool ssi_dbg_rng;
#endif

void bbd_log_hex(const char *pIntroduction, const unsigned char *pData, unsigned long ulDataLen);

//--------------------------------------------------------------
//
//			   BBD device struct
//
//--------------------------------------------------------------
#define BBD_BUFF_SIZE (PAGE_SIZE*2)
struct bbd_cdev_priv {
	const char *name;
	struct cdev dev;			/* char device */
	bool busy;
	struct circ_buf read_buf;		/* LHD reads from BBD */
	struct mutex lock;			/* Lock for read_buf */
	char _read_buf[BBD_BUFF_SIZE];		/* LHD reads from BBD */
	char write_buf[BBD_BUFF_SIZE];		/* LHD writes into BBD */
	wait_queue_head_t poll_wait;		/* for poll */
};

#define CONFIG_LHD_KILLER

struct lhd_killer {
	bool enabled;
	long timeout_sec;

	struct task_struct *lhd;
	bool resetting;

	bool timer_enabled;
	struct timer_list timer;
	struct work_struct work;
	struct workqueue_struct *workq;
	spinlock_t lock;

};

struct bbd_device {
	struct kobject *kobj;			/* for sysfs register */
	struct class *class;			/* for device_create */
	unsigned int major;

	struct bbd_cdev_priv priv[BBD_DEVICE_INDEX];/* individual structures */
	struct wake_lock bbd_wake_lock;
	struct wake_lock patch_wake_lock;
	bool db;				/* debug flag */

	void *ssp_priv;				/* private data pointer */
	bbd_callbacks *ssp_cb;			/* callbacks for SSP */

	struct lhd_killer lk;
};

/*
 * Character device names of BBD
 */
static const char *bbd_dev_name[BBD_DEVICE_INDEX] = {
	"bbd_shmd",
	"bbd_sensor",
	"bbd_control",
	"bbd_patch",
	"bbd_urgent_patch",
	//"bbd_ssi_spi_debug"
};

//--------------------------------------------------------------
//
//			   Globals
//
//--------------------------------------------------------------
/*
 * The global BBD device which has all necessary information.
 * It's not beautiful but useful when we debug by Trace32.
 */
static struct bbd_device bbd;
/*
 * Embedded patch file provided as /dev/bbd_patch
 */
static unsigned char bbd_patch_old[] = {
#if defined(CONFIG_SENSORS_SSP_BEYOND)
#include "p_os/bbd_patch_file_beyond_old.h"
#else
#include "p_os/bbd_patch_file_beyond_old.h"
#endif
};

static unsigned char bbd_patch_new_old[] = { // hw_rev 20 ~ hw_rev 23
#if defined(CONFIG_SENSORS_SSP_BEYOND)
#if ANDROID_VERSION < 100000
#include "p_os/bbd_patch_file_beyond_new_old.h"
#elif ANDROID_VERSION < 110000
#include "q_os/bbd_patch_file_beyond_new_old.h"
#else
#include "r_os/bbd_patch_file_beyond_new_old.h"
#endif
#else
#include "p_os/bbd_patch_file_beyond_new_old.h"
#endif
};

static unsigned char bbd_patch[] = {
#if defined(CONFIG_SENSORS_SSP_BEYOND)
#if ANDROID_VERSION < 100000
#include "p_os/bbd_patch_file_beyond.h"
#elif ANDROID_VERSION < 110000
#include "q_os/bbd_patch_file_beyond.h"
#else
#include "r_os/bbd_patch_file_beyond.h"
#endif
#elif defined(CONFIG_SENSORS_SSP_DAVINCI)
#if ANDROID_VERSION < 100000
#include "p_os/bbd_patch_file_davinci.h"
#elif ANDROID_VERSION < 110000
#include "q_os/bbd_patch_file_davinci.h"
#else
#include "r_os/bbd_patch_file_davinci.h"
#endif
#else
#include "p_os/bbd_patch_file_beyond.h"
#endif
};

/* Function to push read data into any bbd device's read buf */
ssize_t bbd_on_read(unsigned int minor, const unsigned char *buf, size_t size);

#ifdef DEBUG_1HZ_STAT

static const char *bbd_stat_name[STAT_MAX] = {
	"tx@lhd",
	"tx@ssp",
	"tx@rpc",
	"tx@tl",
	"tx@ssi",
	"rx@ssi",
	"rx@tl",
	"rx@rpc",
	"rx@ssp",
	"rx@lhd"
};

struct bbd_stat stat1hz;

//--------------------------------------------------------------
//
//			   bbd 1hz statistics Functions
//
//--------------------------------------------------------------
static void bbd_init_stat(void)
{
	memset(&stat1hz, 0, sizeof(stat1hz));

	stat1hz.min_rx_lat = (u64)-1;
	stat1hz.min_rx_dur = (u64)-1;
	stat1hz.workq = create_singlethread_workqueue("BBD_1HZ_TICK");
}

static void bbd_exit_stat(void)
{
	bbd_disable_stat();
	if (stat1hz.workq) {
		flush_workqueue(stat1hz.workq);
		destroy_workqueue(stat1hz.workq);
		stat1hz.workq = 0;
	}
}

static void bbd_report_stat(struct work_struct *work)
{
	char buf[512];
	char *p = buf;
	int i;

	p += sprintf(p, "BBD:");
	for (i = 0; i < STAT_MAX; i++)
		p += sprintf(p, " %s=%llu", bbd_stat_name[i],
					stat1hz.stat[i]);
	p += sprintf(p, " rxlat_min=%llu rxlat_max=%llu",
		stat1hz.min_rx_lat, stat1hz.max_rx_lat);
	p += sprintf(p, " rxdur_min=%llu rxdur_max=%llu",
		stat1hz.min_rx_dur, stat1hz.max_rx_dur);

	//report only in case we had SSI traffic
	if (stat1hz.stat[STAT_TX_SSI] || stat1hz.stat[STAT_RX_SSI])
		bbd_on_read(BBD_MINOR_CONTROL, buf, strlen(buf)+1);

	for (i = 0; i < STAT_MAX; i++)
		stat1hz.stat[i] = 0;

	stat1hz.min_rx_lat = (u64)-1;
	stat1hz.min_rx_dur = (u64)-1;
	stat1hz.max_rx_lat = 0;
	stat1hz.max_rx_dur = 0;
}

static void bbd_stat_timer_func(unsigned long p)
{
	if (stat1hz.workq)
		queue_work(stat1hz.workq, &stat1hz.work);
	mod_timer(&stat1hz.timer, jiffies + HZ);
}

void bbd_update_stat(int idx, unsigned int count)
{
	stat1hz.stat[idx] += count;
}

void bbd_enable_stat(void)
{
	if (stat1hz.enabled) {
		pr_info("%s() 1HZ stat already enable. skipping.\n", __func__);
		return;
	}

	INIT_WORK(&stat1hz.work, bbd_report_stat);
	setup_timer(&stat1hz.timer, bbd_stat_timer_func, 0);
	mod_timer(&stat1hz.timer, jiffies + HZ);
	stat1hz.enabled = true;
}

void bbd_disable_stat(void)
{
	if (!stat1hz.enabled) {
		pr_info("%s() 1HZ stat already disabled. skipping.\n", __func__);
		return;
	}
	del_timer_sync(&stat1hz.timer);
	cancel_work_sync(&stat1hz.work);
	stat1hz.enabled = false;
}
#endif /* DEBUG_1HZ_STAT */

#ifdef CONFIG_LHD_KILLER

static void bbd_lk_work(struct work_struct *work)
{
	if (bbd.lk.timer_enabled)
		bbd_mcu_reset(false);
}

static void bbd_lk_timer_func(unsigned long p)
{
	if (bbd.lk.workq)
		queue_work(bbd.lk.workq, &bbd.lk.work);
}

static void bbd_enable_lk(void)
{
	pr_info("%s timeout:%ld sec\n", __func__, bbd.lk.timeout_sec);
	if (bbd.lk.timer_enabled) {
		pr_info("%s() lhd kill timer aready enabled. reset timer\n", __func__);
		del_timer_sync(&bbd.lk.timer);
	}

	INIT_WORK(&bbd.lk.work, bbd_lk_work);
	setup_timer(&bbd.lk.timer, bbd_lk_timer_func, 0);
	mod_timer(&bbd.lk.timer, jiffies + HZ*bbd.lk.timeout_sec);
	bbd.lk.timer_enabled = true;
}

static void bbd_disable_lk(void)
{
	pr_info("%s\n", __func__);
	if (!bbd.lk.timer_enabled) {
		pr_info("%s() lhd killer already disabled. skipping.\n", __func__);
		return;
	}
	del_timer_sync(&bbd.lk.timer);
	bbd.lk.timer_enabled = false;
}

static void bbd_init_lk(void)
{
	memset(&bbd.lk, 0, sizeof(bbd.lk));
	bbd.lk.enabled = true;
	bbd.lk.timeout_sec = 10;
	bbd.lk.workq = create_singlethread_workqueue("BBD_LHD_KILLER");
	spin_lock_init(&bbd.lk.lock);
}

static void bbd_exit_lk(void)
{
	bbd_disable_lk();
	if (bbd.lk.workq) {
		flush_workqueue(bbd.lk.workq);
		destroy_workqueue(bbd.lk.workq);
		bbd.lk.workq = 0;
	}
}
#endif
//--------------------------------------------------------------
//
//			   SHMD Interface Functions
//
//--------------------------------------------------------------

/**
 * bbd_register - Interface function called from SHMD to register itself to BBD
 *
 * @priv: SHMD's private data provided back to SHMD as callback argument
 * @cb: SHMD's functions to be called
 */
void bbd_register(void *priv, bbd_callbacks *cb)
{
	bbd.ssp_priv = priv;
	bbd.ssp_cb = cb;
}
EXPORT_SYMBOL(bbd_register);

/**
 * bbd_send_packet - Interface function called from SHMD to send sensor packet.
 *
 *	 The sensor packet is pushed into /dev/bbd_sensor to be read by gpsd/lhd.
 *	 gpsd/lhd wrap the packet into RPC and sends to chip directly.
 *
 * @buf: buffer containing sensor packet
 * @size: size of sensor packet
 * @return: pushed data length = success
 */
ssize_t bbd_send_packet(unsigned char *buf, size_t size)
{
	struct sensor_pkt {
		unsigned short size;
		unsigned char buf[1022];	/*We assume max SSP packet less than 1KB */
	} __attribute__((__packed__)) pkt;

	pkt.size = (unsigned short)size;
	memcpy(pkt.buf, buf, size);

#ifdef DEBUG_1HZ_STAT
	bbd_update_stat(STAT_TX_SSP, size);
#endif
	return bbd_on_read(BBD_MINOR_SENSOR, (unsigned char *)&pkt, size + 2); /* +2 for pkt.size */
}
EXPORT_SYMBOL(bbd_send_packet);


/**
 * bbd_pull_packet - Interface function called from SHMD to read sensor packet.
 *
 *	 Read packet consists of sensor packet from gpsd/lhd and from BBD.
 *
 * @buf: buffer to receive packet
 * @size:
 * @timeout_ms: if specified, this function waits for sensor packet during given time
 *
 * @return: popped data length = success
 */
ssize_t bbd_pull_packet(unsigned char *buf, size_t size, unsigned int timeout_ms)
{
	struct circ_buf *circ = &bbd.priv[BBD_MINOR_SHMD].read_buf;
	size_t rd_size = 0;

	WARN_ON(!buf);
	WARN_ON(!size);

	if (timeout_ms) {
		int ret = wait_event_interruptible_timeout(
					bbd.priv[BBD_MINOR_SHMD].poll_wait,
					circ->head != circ->tail,
					msecs_to_jiffies(timeout_ms));
		if (!ret)
			return -ETIMEDOUT;
	}

	mutex_lock(&bbd.priv[BBD_MINOR_SHMD].lock);

	/* Copy from circ buffer to linear buffer
	 * Because SHMD's buffer is linear, we may require 2 copies from [tail..end] and [start..head]
	 */
	do {
		size_t cnt_to_end = CIRC_CNT_TO_END(circ->head, circ->tail, BBD_BUFF_SIZE);
		size_t copied = min(cnt_to_end, size);

		memcpy(buf + rd_size, (void *) circ->buf + circ->tail, copied);
		size -= copied;
		rd_size += copied;
		circ->tail = (circ->tail + copied) & (BBD_BUFF_SIZE - 1);

	} while (size > 0 && CIRC_CNT(circ->head, circ->tail, BBD_BUFF_SIZE));

	mutex_unlock(&bbd.priv[BBD_MINOR_SHMD].lock);

	return rd_size;
}
EXPORT_SYMBOL(bbd_pull_packet);

/**
 * bbd_mcu_reset - Interface function called from SHMD to reset chip
 *
 *	  BBD pushes reset request into /dev/bbd_control and actual reset is done by gpsd/lhd when it reads the request
 *
 * @return: 0 = success, -1 = failure
 */
int bbd_mcu_reset(bool IsNoResp)
{
	pr_info("reset request from sensor hub\n");
	if (bbd.ssp_cb && bbd.ssp_cb->on_mcu_reset)
		bbd.ssp_cb->on_mcu_reset(bbd.ssp_priv, IsNoResp);

	if (bbd.lk.enabled) {
		int ret;
		/* to fix lhd hang issue, we don't use lhd's control interface to reset mcu.  */
		spin_lock(&bbd.lk.lock);
		if (bbd.lk.lhd == NULL || bbd.lk.resetting) {
			pr_err("[SSP] lhd is NULL or resetting");
			ret = -1;
		} else {
			bbd.lk.resetting = true;
			ret = send_sig(SIGKILL, bbd.lk.lhd, 0);
		}
		spin_unlock(&bbd.lk.lock);
		return ret;
	} else
		return bbd_on_read(BBD_MINOR_CONTROL, BBD_CTRL_RESET_REQ, strlen(BBD_CTRL_RESET_REQ)+1);
}
EXPORT_SYMBOL(bbd_mcu_reset);



//--------------------------------------------------------------
//
//			   BBD device struct
//
//--------------------------------------------------------------

/**
 * bbd_control - Handles command string from lhd
 *
 *
 */
ssize_t bbd_control(const char *buf, ssize_t len)
{
	pr_info("%s : %s\n", __func__, buf);

	if (strstr(buf, ESW_CTRL_READY)) {

		if (bbd.ssp_cb && bbd.ssp_cb->on_mcu_ready)
			bbd.ssp_cb->on_mcu_ready(bbd.ssp_priv, true);

		if (wake_lock_active(&bbd.patch_wake_lock))
			wake_unlock(&bbd.patch_wake_lock);
	} else if (strstr(buf, ESW_CTRL_NOTREADY)) {
		struct circ_buf *circ = &bbd.priv[BBD_MINOR_SENSOR].read_buf;

		circ->head = circ->tail = 0;
		if (bbd.ssp_cb && bbd.ssp_cb->on_mcu_ready)
			bbd.ssp_cb->on_mcu_ready(bbd.ssp_priv, false);

		if (wake_lock_active(&bbd.patch_wake_lock))
			wake_unlock(&bbd.patch_wake_lock);
	} else if (strstr(buf, ESW_CTRL_CRASHED)) {
		struct circ_buf *circ = &bbd.priv[BBD_MINOR_SENSOR].read_buf;

		circ->head = circ->tail = 0;

		if (bbd.ssp_cb && bbd.ssp_cb->on_mcu_ready)
			bbd.ssp_cb->on_mcu_ready(bbd.ssp_priv, false);

		if (bbd.ssp_cb && bbd.ssp_cb->on_control)
			bbd.ssp_cb->on_control(bbd.ssp_priv, buf);
	} else if (strstr(buf, BBD_CTRL_SSI_PATCH_BEGIN)) {
		wake_lock(&bbd.patch_wake_lock);
#if 0
	} else if (strstr(buf, BBD_CTRL_DEBUG_ON)) {
		bbd.db = true;
#endif
	} else if (strstr(buf, BBD_CTRL_DEBUG_OFF)) {
		bbd.db = false;
#ifdef CONFIG_SENSORS_SSP
	} else if (strstr(buf, SSP_DEBUG_ON)) {
		ssp_dbg = true;
		ssp_pkt_dbg = true;
	} else if (strstr(buf, SSP_DEBUG_OFF)) {
		ssp_dbg = false;
		ssp_pkt_dbg = false;
#endif
#ifdef CONFIG_BCM_GPS_SPI_DRIVER
	} else if (strstr(buf, SSI_DEBUG_ON)) {
		ssi_dbg = true;
	} else if (strstr(buf, SSI_DEBUG_OFF)) {
		ssi_dbg = false;
	} else if (strstr(buf, PZC_DEBUG_ON)) {
		ssi_dbg_pzc = true;
	} else if (strstr(buf, PZC_DEBUG_OFF)) {
		ssi_dbg_pzc = false;
	} else if (strstr(buf, RNG_DEBUG_ON)) {
		ssi_dbg_rng = true;
	} else if (strstr(buf, RNG_DEBUG_OFF)) {
		ssi_dbg_rng = false;
#endif
#ifdef CONFIG_LHD_KILLER
	} else if (strstr(buf, BBD_CTRL_PASSTHRU_ON)) {
		if (current == bbd.lk.lhd) {
			pr_info("[SSPBBD] PatchTimer Start\n");
			bbd_enable_lk();
		}
		bbd.ssp_cb->on_control(bbd.ssp_priv, SSP_OIS_NOTIFY_RESET);
	} else if (strstr(buf, BBD_CTRL_PASSTHRU_OFF)) {
		if (current == bbd.lk.lhd) {
			pr_info("[SSPBBD] Patch Done Timer Off\n");
			bbd_disable_lk();
		}
#endif /* CONFIG_LHD_KILLER */
	} else if (bbd.ssp_cb && bbd.ssp_cb->on_control) {
		/* Tell SHMD about the unknown control string */
		bbd.ssp_cb->on_control(bbd.ssp_priv, buf);
	}

	return len;
}



//--------------------------------------------------------------
//
//			   BBD Common File Functions
//
//--------------------------------------------------------------

/**
 * bbd_common_open - Common open function for BBD devices
 *
 */
int bbd_common_open(struct inode *inode, struct file *filp)
{
	unsigned int minor = iminor(inode);
	struct circ_buf *circ = &bbd.priv[minor].read_buf;

	pr_info("%s++\n", __func__);

	if (minor >= BBD_DEVICE_INDEX)
		return -ENODEV;

	pr_info("%s", bbd.priv[minor].name);

	if (bbd.priv[minor].busy && minor != BBD_MINOR_CONTROL)
		return -EBUSY;

	bbd.priv[minor].busy = true;

	/* Reset circ buffer */
	circ->head = circ->tail = 0;

	filp->private_data = &bbd;

	pr_info("%s--\n", __func__);
	if (minor == BBD_MINOR_SENSOR) {
		spin_lock(&bbd.lk.lock);
		bbd.lk.lhd = current;
		bbd.lk.resetting = false;
		spin_unlock(&bbd.lk.lock);
	}
	return 0;
}

/**
 * bbd_common_release - Common release function for BBD devices
 */
static int bbd_common_release(struct inode *inode, struct file *filp)
{
	unsigned int minor = iminor(inode);

	BUG_ON(minor >= BBD_DEVICE_INDEX);

	pr_info("%s[%s]++\n", __func__, bbd.priv[minor].name);
	bbd.priv[minor].busy = false;
	if (minor == BBD_MINOR_SENSOR) {
		spin_lock(&bbd.lk.lock);
		bbd.lk.lhd = NULL;
		spin_unlock(&bbd.lk.lock);
	}
	pr_info("%s[%s]--\n", __func__, bbd.priv[minor].name);
	return 0;
}

/**
 * bbd_common_read - Common read function for BBD devices
 *
 * lhd reads from BBD devices via this function
 *
 */
static ssize_t bbd_common_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	unsigned int minor = iminor(filp->f_path.dentry->d_inode);
	//struct bbd_device *bbd = filp->private_data;
	struct circ_buf *circ = &bbd.priv[minor].read_buf;
	size_t rd_size = 0;

	BUG_ON(minor >= BBD_DEVICE_INDEX);
	//pr_info("%s[%s]++\n", __func__, bbd.priv[minor].name);

	mutex_lock(&bbd.priv[minor].lock);

	/* Copy from circ buffer to lhd
	 * Because lhd's buffer is linear, we may require 2 copies from [tail..end] and [end..head]
	 */
	do {
		size_t cnt_to_end = CIRC_CNT_TO_END(circ->head, circ->tail, BBD_BUFF_SIZE);
		size_t copied = min(cnt_to_end, size);

		WARN_ON(copy_to_user(buf + rd_size, (void *) circ->buf + circ->tail, copied));
		size -= copied;
		rd_size += copied;
		circ->tail = (circ->tail + copied) & (BBD_BUFF_SIZE - 1);

	} while (size > 0 && CIRC_CNT(circ->head, circ->tail, BBD_BUFF_SIZE));

	mutex_unlock(&bbd.priv[minor].lock);

	bbd_log_hex(bbd_dev_name[minor], buf, rd_size);

#ifdef DEBUG_1HZ_STAT
	bbd_update_stat(STAT_RX_LHD, rd_size);
#endif
	//pr_info("%s[%s]--\n", __func__, bbd.priv[minor].name);
	return rd_size;
}

/**
 * bbd_common_write - Common write function for BBD devices *
 * lhd writes to BBD devices via this function
 *
 */
static ssize_t bbd_common_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	unsigned int minor = iminor(filp->f_path.dentry->d_inode);
	//struct bbd_device *bbd = filp->private_data;

	//BUG_ON(size >= BBD_BUFF_SIZE);
	if (size >= BBD_BUFF_SIZE)
		return -EINVAL;

	WARN_ON(copy_from_user(bbd.priv[minor].write_buf, buf, size));

#ifdef DEBUG_1HZ_STAT
	bbd_update_stat(STAT_TX_LHD, size);
#endif
	return size;
}

/**
 * bbd_common_poll - Common poll function for BBD devices
 *
 */
static unsigned int bbd_common_poll(struct file *filp, poll_table *wait)
{
	unsigned int minor = iminor(filp->f_path.dentry->d_inode);
	//struct bbd_device *bbd = filp->private_data;
	struct circ_buf *circ = &bbd.priv[minor].read_buf;
	unsigned int mask = 0;

	BUG_ON(minor >= BBD_DEVICE_INDEX);

	poll_wait(filp, &bbd.priv[minor].poll_wait, wait);

	if (CIRC_CNT(circ->head, circ->tail, BBD_BUFF_SIZE))
		mask |= POLLIN;

	return mask;
}




//--------------------------------------------------------------
//
//			   BBD Device Specific File Functions
//
//--------------------------------------------------------------

/**
 * bbd_sensor_write - BBD's RPC calls this function to send sensor packet
 *
 * @buf: contains sensor packet coming from gpsd/lhd
 *
 */
static unsigned long init_time = 0;
static unsigned long clock_get_ms(void)
{
	struct timeval t;
	unsigned long now;

	do_gettimeofday(&t);
	now = t.tv_usec / 1000 + t.tv_sec * 1000;
	if (init_time == 0)
		init_time = now;

	return now - init_time;
}

static struct timespec bbd_sensor_time;
ssize_t bbd_sensor_write(const char *buf, size_t size)
{
	unsigned long start_time = 0, end_time = 0, delta = 0;

	start_time = clock_get_ms();

	bbd_sensor_time = ktime_to_timespec(ktime_get_boottime());
	/* Copies into /dev/bbd_shmd. If SHMD was sleeping in poll_wait, bbd_on_read() wakes it up also */

#ifdef DEBUG_1HZ_STAT
	bbd_update_stat(STAT_RX_SSP, size);
#endif
	/* OK. Now call pre-registered SHMD callbacks */
	if (bbd.ssp_cb->on_packet)
		bbd.ssp_cb->on_packet(bbd.ssp_priv, buf, size);
	else if (bbd.ssp_cb->on_packet_alarm) {
		bbd_on_read(BBD_MINOR_SHMD, buf, size);
		bbd.ssp_cb->on_packet_alarm(bbd.ssp_priv);
	} else
		pr_err("%s no SSP on_packet callback registered. Dropped %u bytes\n", __func__, (unsigned int)size);

	end_time = clock_get_ms();
	delta = end_time - start_time;
	
	if (delta > 100)
		pr_err("[SSPBBD] SSP consumed %lu msec in handling %u bytes sensor data", delta, (int)size);

	return size;
}

s64 get_sensor_time_delta_us(void)
{
	struct timespec curr_ts = ktime_to_timespec(ktime_get_boottime());
	struct timespec delta = timespec_sub(curr_ts, bbd_sensor_time);

	return timespec_to_ns(&delta)/NSEC_PER_USEC;
}


/**
 * bbd_control_write - Write function for BBD control (/dev/bbd_control)
 *
 *  Receives control string from lhd and handles it
 *
 */
ssize_t bbd_control_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	unsigned int minor = iminor(filp->f_path.dentry->d_inode);
	//struct bbd_device *bbd = filp->private_data;

	/* get command string first */
	ssize_t len = bbd_common_write(filp, buf, size, ppos);

	if (len <= 0)
		return len;

	/* Process received command string */
	return bbd_control(bbd.priv[minor].write_buf, len);
}
/* bbd patch version define only for beyond*/
#define bbd_old     0
#define bbd_new_old     1
#define bbd_current     2

extern int get_patch_version(int ap_type, int hw_rev);

ssize_t bbd_patch_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	ssize_t rd_size = size;
	size_t  offset = filp->f_pos;
	int hw_rev = bbd.ssp_cb->on_control(bbd.ssp_priv, SSP_GET_HW_REV);
        int ap_type = bbd.ssp_cb->on_control(bbd.ssp_priv, SSP_GET_AP_REV);
        int patch_ver = get_patch_version(ap_type, hw_rev);
        int patch_size = (patch_ver == bbd_current ? sizeof(bbd_patch) 
                                    : (patch_ver == bbd_new_old) ? sizeof(bbd_patch_new_old) : sizeof(bbd_patch_old));
    
	if (offset >= patch_size) {	   /* signal EOF */
		*ppos = 0;
		return 0;
	}
    
	if (offset+size > patch_size)
		rd_size = patch_size - offset;
        if (copy_to_user(buf, (patch_ver == bbd_current ? bbd_patch : 
                (patch_ver == bbd_new_old) ? bbd_patch_new_old : bbd_patch_old) + offset, rd_size))
		rd_size = -EFAULT;
	else
		*ppos = filp->f_pos + rd_size;

	return rd_size;
}

#define URGENT_FIRMWARE_PATH "/spu/sensorhub/urgent.patch"

static bool is_signed = false;
static int urgent_patch_size = 0;

ssize_t bbd_urgent_patch_read(struct file *user_filp, char __user *buf, size_t size, loff_t *ppos)
{
	ssize_t rd_size = size;
	size_t offset = user_filp->f_pos;
   
	int ret = 0;
	struct file *filp = NULL;
	loff_t fsize = 0;

	char *urgent_buffer = NULL;

	mm_segment_t old_fs = get_fs();
	
	// 02. read binary from urgent.patch
	set_fs(KERNEL_DS);
	
	filp = filp_open(URGENT_FIRMWARE_PATH, O_RDONLY, 0);

	if(IS_ERR(filp)){
		pr_err("[SSPBBD] %s : filp_open failed", __func__);
		set_fs(old_fs);
		ret = PTR_ERR(filp);
		return ret;
	}

	// 02-1. checking size of urgent.patch except for digest and signature
	fsize = i_size_read(file_inode(filp));
	urgent_buffer = kzalloc(fsize, GFP_KERNEL);

	if (!urgent_buffer) {
		pr_err("[SSPBBD] %s) can't alloc urgent_buffer\n", __func__);
		ret = PTR_ERR(urgent_buffer);
	
		filp_close(filp, NULL);
		set_fs(old_fs);

		return ret;
	}

	ret = vfs_read(filp, urgent_buffer, fsize, &filp->f_pos);

	filp_close(filp, NULL);
	set_fs(old_fs);

	if (ret < 0) {
		pr_info("[SSPBBD] %s : fail to read urgent firmware file %d", __func__, ret);
	} else {
		is_signed = false;
	
		ret = spu_firmware_signature_verify("SENSORHUB", urgent_buffer, fsize);
	
		if(ret < 0){
			pr_err("[SSPBBD] %s : fail to spu_fireware_signature_verify %d", __func__, ret);
			set_fs(old_fs);
			//ret = PTR_ERR(filp);
			kfree(urgent_buffer);
			return ret;
		} else
			is_signed = true;

			if (is_signed == false) {
				pr_err("[SSPBBD] %s : urgent_patch is not signed", __func__);
				kfree(urgent_buffer);
				return 0;
			}

			urgent_patch_size = ret;
			pr_err("[SSPBBD] %s : total: %d  patch size: %d", __func__, fsize, urgent_patch_size);

			if (offset >= urgent_patch_size) {	// signal EOF 
				pr_err("[SSPBBD] %s : signal EOF", __func__);

				*ppos = 0;
				kfree(urgent_buffer);
				return 0;
			}

		if (offset + size > urgent_patch_size)
			rd_size = urgent_patch_size - offset;

		// 02-3. read requested size of urget_patch
		pr_info("[SSPBBD] %s : download in progress (%d/%d)", __func__, offset  + rd_size, urgent_patch_size);

		if(copy_to_user(buf, (void *)(urgent_buffer + offset), rd_size)) {
			pr_info("[SSPBBD] %s : copy to user from urgent_buffer", __func__);
			rd_size = -EFAULT;
		}
		else
			*ppos = user_filp->f_pos + rd_size;

	}

	kfree(urgent_buffer);

	return rd_size;
}

//--------------------------------------------------------------
//
//			   Sysfs
//
//--------------------------------------------------------------
static ssize_t store_sysfs_bbd_control(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	bbd_control(buf, strlen(buf)+1);
	return len;
}

static ssize_t show_sysfs_bbd_pl(struct device *dev, struct device_attribute *attr, char *buf)
{
#if 0
		const struct stTransportLayerStats *p = 0;

		p = TransportLayer_GetStats(&bbd.engine.bridge.m_otTL);
		if (!p)
			return 0;

	return sprintf(buf,
					"RxGarbageBytes=%lu\n"
					"RxPacketLost=%lu\n"
					"RemotePacketLost=%lu\n"
					"RemoteGarbage=%lu\n"
					"PacketSent=%lu\n"
					"PacketReceived=%lu\n"
					"AckReceived=%lu\n"
					"ReliablePacketSent=%lu\n"
					"ReliableRetransmit=%lu\n"
					"ReliablePacketReceived=%lu\n"
					"MaxRetransmitCount=%lu\n",
			p->ulRxGarbageBytes,
			p->ulRxPacketLost,
			p->ulRemotePacketLost, /* approximate */
			p->ulRemoteGarbage, /* approximate */
			p->ulPacketSent,		 /* number of normal packets sent */
			p->ulPacketReceived,
			p->ulAckReceived,
			p->ulReliablePacketSent,
			p->ulReliableRetransmit,
			p->ulReliablePacketReceived,
			p->ulMaxRetransmitCount);
#endif
	return 0;
}

static ssize_t store_sysfs_lk_timeout(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	int status = kstrtol(buf, 0, &bbd.lk.timeout_sec);

	if (bbd.lk.timeout_sec < 10)
		bbd.lk.timeout_sec = 10; //base timeout 10sec
	return status ? : len;
}
static ssize_t show_sysfs_lk_timeout(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "timeout for lhd killer : %ld sec\n", bbd.lk.timeout_sec);
}
static ssize_t store_sysfs_lk_enable(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	if (len)
		bbd.lk.enabled = buf[0] == '0' ? false : true;

	return len;
}
static ssize_t show_sysfs_lk_enable(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "lhd killer %s\n", bbd.lk.enabled ? "enabled" : "disabled");
}

static DEVICE_ATTR(bbd, 0220, NULL,				store_sysfs_bbd_control);
static DEVICE_ATTR(pl, 0440, show_sysfs_bbd_pl,	   NULL);
static DEVICE_ATTR(lk_timeout, 0660, show_sysfs_lk_timeout, store_sysfs_lk_timeout);
static DEVICE_ATTR(lk_enable, 0660, show_sysfs_lk_enable, store_sysfs_lk_enable);

static struct attribute *bbd_attributes[] = {
	&dev_attr_bbd.attr,
	&dev_attr_pl.attr,
	&dev_attr_lk_timeout.attr,
	&dev_attr_lk_enable.attr,
	NULL
};

static const struct attribute_group bbd_group = {
	.attrs = bbd_attributes,
};


//--------------------------------------------------------------
//
//			   Misc Functions
//
//--------------------------------------------------------------
void bbd_log_hex(const char *pIntroduction,
		const unsigned char *pData,
		unsigned long ulDataLen)
{
	const unsigned char *pDataEnd = pData + ulDataLen;

	if (likely(!bbd.db))
		return;
	if (!pIntroduction)
		pIntroduction = "...unknown...";

	while (pData < pDataEnd) {
		char buf[128];
		size_t bufsize = sizeof(buf) - 3;
		size_t lineLen = pDataEnd - pData;
		size_t perLineCount = lineLen;

		if (lineLen > 32) {
			lineLen = 32;
			perLineCount = lineLen;
		}

		snprintf(buf, bufsize, "%s [%u] { ", pIntroduction,
				(unsigned int)lineLen);

		for (; perLineCount > 0; ++pData, --perLineCount) {
			size_t len = strlen(buf);

			snprintf(buf+len, bufsize - len, "%02X ", *pData);
		}
		pr_info("%s}\n", buf);
	}
}

/**
 *
 * bbd_on_read - Push data into read buffer of specified char device.
 *   if minor is bbd_sensor
 *
 * @buf: linear buffer
 */
ssize_t bbd_on_read(unsigned int minor, const unsigned char *buf, size_t size)
{
	struct circ_buf *circ = &bbd.priv[minor].read_buf;
	size_t wr_size = 0;

	bbd_log_hex(bbd_dev_name[minor], buf, size);

	mutex_lock(&bbd.priv[minor].lock);

	/* If there's not enough speace, drop it but try waking up reader */
	if (CIRC_SPACE(circ->head, circ->tail, BBD_BUFF_SIZE) < size) {
		pr_err("%s read buffer full. Dropping %u bytes\n",
				bbd_dev_name[minor], (unsigned int)size);
		goto skip;
	}

	/* Copy into circ buffer from linear buffer
	 * We may require 2 copies from [head..end] and [start..head]
	 */
	do {
		size_t space_to_end = CIRC_SPACE_TO_END(circ->head, circ->tail, BBD_BUFF_SIZE);
		size_t copied = min(space_to_end, size);

		memcpy((void *) circ->buf + circ->head, buf + wr_size, copied);
		size -= copied;
		wr_size += copied;
		circ->head = (circ->head + copied) & (BBD_BUFF_SIZE - 1);

	} while (size > 0 && CIRC_SPACE(circ->head, circ->tail, BBD_BUFF_SIZE));
skip:
	mutex_unlock(&bbd.priv[minor].lock);

	/* Wake up reader */
	wake_up(&bbd.priv[minor].poll_wait);


	return wr_size;
}

ssize_t bbd_request_mcu(bool on)
{
	pr_info("%s(%s) called", __func__, (on) ? "On" : "Off");
	if (on)
		return bbd_on_read(BBD_MINOR_CONTROL, GPSD_SENSOR_ON, strlen(GPSD_SENSOR_ON) + 1);

	bbd.ssp_cb->on_mcu_ready(bbd.ssp_priv, false);
	return bbd_on_read(BBD_MINOR_CONTROL, GPSD_SENSOR_OFF, strlen(GPSD_SENSOR_OFF) + 1);
}
EXPORT_SYMBOL(bbd_request_mcu);

//--------------------------------------------------------------
//
//			   PM operation
//
//--------------------------------------------------------------
static int bbd_suspend(pm_message_t state)
{
	pr_info("[SSPBBD]: %s ++\n", __func__);

#ifdef DEBUG_1HZ_STAT
	bbd_disable_stat();
#endif
#ifdef CONFIG_SENSORS_SSP
	/* Call SSP suspend */
	if (pssp_driver->driver.pm && pssp_driver->driver.pm->suspend)
		pssp_driver->driver.pm->suspend(&dummy_spi.dev);
#endif
	mdelay(20);

	pr_info("[SSPBBD]: %s --\n", __func__);
	return 0;
}

static int bbd_resume(void)
{
#ifdef CONFIG_SENSORS_SSP
	/* Call SSP resume */
	if (pssp_driver->driver.pm && pssp_driver->driver.pm->suspend)
		pssp_driver->driver.pm->resume(&dummy_spi.dev);
#endif
#ifdef DEBUG_1HZ_STAT
	bbd_enable_stat();
#endif
	wake_lock_timeout(&bbd.bbd_wake_lock, HZ/2);

	return 0;
}

static int bbd_notifier(struct notifier_block *nb, unsigned long event, void *data)
{
	pm_message_t state = {0};

	switch (event) {
	case PM_SUSPEND_PREPARE:
		pr_info("%s going to sleep", __func__);
		state.event = event;
		bbd_suspend(state);
		break;
	case PM_POST_SUSPEND:
		pr_info("%s waking up", __func__);
		bbd_resume();
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block bbd_notifier_block = {
	.notifier_call = bbd_notifier,
};

//--------------------------------------------------------------
//
//			   BBD Device Init and Exit
//
//--------------------------------------------------------------


static const struct file_operations bbd_fops[BBD_DEVICE_INDEX] = {
	/* bbd shmd file operations */
	{
		.owner		  =  THIS_MODULE,
	},
	/* bbd sensor file operations */
	{
		.owner		=  THIS_MODULE,
		.open		=  bbd_common_open,
		.release	=  bbd_common_release,
		.read		=  bbd_common_read,
		.write		=  NULL,
		.poll		=  bbd_common_poll,
	},
	/* bbd control file operations */
	{
		.owner		=  THIS_MODULE,
		.open		=  bbd_common_open,
		.release	=  bbd_common_release,
		.read		=  bbd_common_read,
		.write		=  bbd_control_write,
		.poll		=  bbd_common_poll,
	},
	/* bbd patch file operations */
	{
		.owner		=  THIS_MODULE,
		.open		=  bbd_common_open,
		.release	=  bbd_common_release,
		.read		=  bbd_patch_read,
		.write		=  NULL, /* /dev/bbd_patch is read-only */
		.poll		=  NULL,
	},
	{
		.owner		=  THIS_MODULE,
		.open		=  bbd_common_open,
		.release	=  bbd_common_release,
		.read		=  bbd_urgent_patch_read,
		.write		=  NULL, /* /dev/bbd_patch is read-only */
		.poll		=  NULL,
	},

	/*
	 *bbd ssi spi debug operations
	 *{
	 *        .owner		  =  THIS_MODULE,
	 *        .open		=  bbd_common_open,
	 *        .release	=  bbd_common_release,
	 *        .read		   =  NULL,
	 *        .write		  =  bbd_ssi_spi_debug_write,
	 *        .poll		   =  NULL,
	 *}
	 */
};

int bbd_init(struct device *dev)
{
	int minor, ret = -ENOMEM;
	struct timespec ts1;
	unsigned long start, elapsed;
	dev_t devno = 0;

	ts1 = ktime_to_timespec(ktime_get_boottime());
	start = ts1.tv_sec * 1000000000ULL + ts1.tv_nsec;

	/* Initialize BBD device */
	memset(&bbd, 0, sizeof(bbd));
	wake_lock_init(&bbd.bbd_wake_lock, WAKE_LOCK_SUSPEND, "bbd_wake_lock");
	wake_lock_init(&bbd.patch_wake_lock, WAKE_LOCK_SUSPEND, "patch_wake_lock");

	/* Create class which is required for device_create() */
	bbd.class = class_create(THIS_MODULE, "bbd");
	if (IS_ERR(bbd.class)) {
		WARN("BBD:%s() failed to create class \"bbd\"", __func__);
		goto exit;
	}

	ret = alloc_chrdev_region(&devno, 0, BBD_DEVICE_INDEX, "BBD");
	if (ret < 0) {
		pr_err("BBD:%s() alloc_chrdev_region failed, ret=%d", __func__, ret);
		goto exit;
	}
	bbd.major = MAJOR(devno);

	/* Create BBD char devices */
	for (minor = 0; minor < BBD_DEVICE_INDEX; minor++) {
		struct cdev *cdev = &bbd.priv[minor].dev;
		const char *name = bbd_dev_name[minor];
		struct device *dev;

		/* Init buf, waitqueue, mutex, etc. */
		bbd.priv[minor].name = bbd_dev_name[minor];
		bbd.priv[minor].read_buf.buf = bbd.priv[minor]._read_buf;

		init_waitqueue_head(&bbd.priv[minor].poll_wait);
		mutex_init(&bbd.priv[minor].lock);

		/* Don't register /dev/bbd_shmd */
		if (minor == BBD_MINOR_SHMD)
			continue;

		devno = MKDEV(bbd.major, minor);
		/* Register cdev which relates above device number with this BBD device */
		cdev_init(cdev, &bbd_fops[minor]);
		cdev->owner = THIS_MODULE;
		cdev->ops = &bbd_fops[minor];
		ret = cdev_add(cdev, devno, 1);
		if (ret) {
			pr_err("BBD:%s()) failed to cdev_add() \"%s\", ret=%d",
							__func__, name, ret);
			goto free_class;
		}

		/* Let it show in FS */
		dev = device_create(bbd.class, NULL, devno, NULL, "%s", name);
		if (IS_ERR_OR_NULL(dev)) {
			pr_err("BBD:%s() failed to device_create() \"%s\", ret=%d", __func__, name, ret);
			cdev_del(&bbd.priv[minor].dev);
			goto free_class;
		}

		/* Done. Put success log and init BBD specific fields */
		pr_info("BBD:%s(%d,%d) registered /dev/%s\n",
				  __func__, bbd.major, minor, name);
	}

	/* Register sysfs entry */
	bbd.kobj = kobject_create_and_add("bbd", NULL);
	BUG_ON(!bbd.kobj);
	ret = sysfs_create_group(bbd.kobj, &bbd_group);
	if (ret < 0) {
		pr_err("%s failed to sysfs_create_group \"bbd\", ret = %d",
							__func__, ret);
		goto free_kobj;
	}


	/* Register PM */
	ret = register_pm_notifier(&bbd_notifier_block);
	BUG_ON(ret);

#ifdef CONFIG_SENSORS_SSP
	/* Now, we can initialize SSP */
	BUG_ON(device_register(&dummy_spi.dev));
	{
		struct spi_device *spi = to_spi_device(dev);
		void *org_priv, *new_priv;

		org_priv = spi_get_drvdata(spi);
		pssp_driver->probe(spi);
		new_priv = spi_get_drvdata(spi);
		spi_set_drvdata(spi, org_priv);
		spi_set_drvdata(&dummy_spi, new_priv);

	}
#endif
	ts1 = ktime_to_timespec(ktime_get_boottime());
	elapsed = (ts1.tv_sec * 1000000000ULL + ts1.tv_nsec) - start;
	pr_info("BBD:%s %lu nsec elapsed\n", __func__, elapsed);

#ifdef DEBUG_1HZ_STAT
	bbd_init_stat();
#endif

#ifdef CONFIG_LHD_KILLER
	bbd_init_lk();
#endif
	return 0;

free_kobj:
	kobject_put(bbd.kobj);
free_class:
	while (--minor > BBD_MINOR_SHMD) {
		struct cdev *cdev = &bbd.priv[minor].dev;
		device_destroy(bbd.class, MKDEV(bbd.major, minor));
		cdev_del(cdev);
	}
	unregister_chrdev_region(MKDEV(bbd.major, 0), BBD_DEVICE_INDEX);
	class_destroy(bbd.class);
exit:
	return ret;
}

static void __exit bbd_exit(void)
{
	int minor;

	pr_info("%s ++\n", __func__);

#ifdef CONFIG_SENSORS_SSP
	/* Shutdown SSP first*/
	pssp_driver->shutdown(&dummy_spi);
#endif

	/* Remove sysfs entry */
	sysfs_remove_group(bbd.kobj, &bbd_group);

	/* Remove BBD char devices */
	for (minor = BBD_MINOR_SENSOR; minor < BBD_DEVICE_INDEX; minor++) {
		dev_t devno = MKDEV(bbd.major, minor);
		struct cdev *cdev = &bbd.priv[minor].dev;
		const char *name = bbd_dev_name[minor];

		device_destroy(bbd.class, devno);
		cdev_del(cdev);

		pr_info("%s(%d,%d) unregistered /dev/%s\n",
				__func__, MAJOR(devno), minor, name);
	}
	unregister_chrdev_region(MKDEV(bbd.major, 0), BBD_DEVICE_INDEX);

#ifdef DEBUG_1HZ_STAT
	bbd_exit_stat();
#endif

#ifdef CONFIG_LHD_KILLER
	bbd_exit_lk();
#endif
	/* Remove class */
	class_destroy(bbd.class);
	/* Done. Put success log */
	pr_info("%s --\n", __func__);
}

MODULE_AUTHOR("Broadcom");
MODULE_LICENSE("Dual BSD/GPL");
//subsys_initcall(bbd_init);
//module_exit(bbd_exit);
