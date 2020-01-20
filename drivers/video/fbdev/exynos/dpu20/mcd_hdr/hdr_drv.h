/* linux/drivers/video/fbdev/exynos/mcd_hdr/hdr_drv.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *
 * header file for Samsung EXYNOS SoC HDR driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __HDR_DRV_H__
#define __HDR_DRV_H__

#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-core.h>

#include <linux/io.h>
#include <linux/pm_runtime.h>
#include <linux/pm_qos.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/exynos_iovmm.h>

#include "../decon.h"

extern int hdr_log_level;

#define HDR_MAX_SUBDEV		6

#define MCD_HDR_MODULE_NAME		"mcd-hdr"

#define hdr_err(fmt, ...)							\
	do {									\
		if (hdr_log_level >= 3) {					\
			pr_err(pr_fmt(fmt), ##__VA_ARGS__);			\
		}								\
	} while (0)

#define hdr_warn(fmt, ...)							\
	do {									\
		if (hdr_log_level >= 4) {					\
			pr_warn(pr_fmt(fmt), ##__VA_ARGS__);			\
		}								\
	} while (0)

#define hdr_info(fmt, ...)							\
	do {									\
		if (hdr_log_level >= 6)					\
			pr_info(pr_fmt(fmt), ##__VA_ARGS__);			\
	} while (0)

#define hdr_dbg(fmt, ...)							\
	do {									\
		if (hdr_log_level >= 7)					\
			pr_info(pr_fmt(fmt), ##__VA_ARGS__);			\
	} while (0)

struct mcd_hdr_device {
	int id;
	struct device *dev;
	void __iomem *regs;
	struct v4l2_subdev sd;
	int attr;
};

enum {
	MCD_GF0 = 0,
	MCD_VGRFS,
	MCD_GF1,
	MCD_VGF,
	MCD_VG,
	MCD_VGS
};


struct wcg_config {
	int id;
#if 0
//Todo remove 
	int in_dataspace;
	int out_dataspace;
//
#endif
	/*dst out color mode*/
	unsigned int color_mode;
	/* src info */
	unsigned int eq_mode;
	unsigned int hdr_mode;
};


struct hdr10_config {
	/*out data space from wcg mode*/
	unsigned int color_mode;
	
	unsigned int eq_mode;
	unsigned int hdr_mode;

	/* max luminance for hdr contents */
	unsigned int src_max_luminance;
	/* max luminance for display device (ex. lcd, TV, ..)*/
	unsigned int dst_max_luminance;
	unsigned int *lut;
};


#define SUPPORT_WCG			0x01
#define SUPPORT_HDR10 		0x01 << 1
#define SUPPORT_HDR10P 		0x01 << 2

#define IS_SUPPORT_WCG(type)		(type & SUPPORT_WCG)
#define IS_SUPPORT_HDR10(type)		(type & SUPPORT_HDR10)
#define IS_SUPPORT_HDR10P(type)		(type & SUPPORT_HDR10P)


#define CONFIG_WCG				_IOW('H', 0, struct wcg_config)
#define CONFIG_HDR10			_IOW('H', 1, struct hdr10_config)
#define CONFIG_HDR10P			_IOW('H', 2, struct hdr10_config)

#define STOP_MCD_IP				_IOW('H', 11, u32)
#define DUMP_MCD_IP				_IOW('H', 12, u32)
#define RESET_MCD_IP			_IOW('H', 14, u32)

#define GET_ATTR				_IOR('H', 21, u32)

#endif
