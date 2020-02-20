/*
 * include/linux/sec_detect-conn.h
 *
 * COPYRIGHT(C) 2017 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef SEC_DETECT_CONN_H
#define SEC_DETECT_CONN_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/of_gpio.h>
#include <linux/sec_class.h>
#include <linux/sec_class.h>

#define DETECT_CONN_ENABLE_MAGIC 0xDECC
#define DET_CONN_MAX_NUM_GPIOS 32
#define UEVENT_CONN_MAX_DEV_NAME 32

struct sec_det_conn_p_data {
	const char *name[DET_CONN_MAX_NUM_GPIOS];
	int irq_gpio[DET_CONN_MAX_NUM_GPIOS];
	int irq_number[DET_CONN_MAX_NUM_GPIOS];
	unsigned int irq_type[DET_CONN_MAX_NUM_GPIOS];
	struct detect_conn_info *pinfo;
	int gpio_cnt;
};

struct detect_conn_info {
	struct device *dev;
	int irq_enabled[DET_CONN_MAX_NUM_GPIOS];
	struct sec_det_conn_p_data *pdata;
};

static char sec_detect_available_pins_string[15*10] = {0,};

#endif
