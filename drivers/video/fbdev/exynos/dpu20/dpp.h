/* linux/drivers/video/fbdev/exynos/dpu/dpp.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *
 * header file for Samsung EXYNOS SoC DPP driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SAMSUNG_DPP_H__
#define __SAMSUNG_DPP_H__

#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/io.h>
#include <linux/pm_runtime.h>
#include <linux/pm_qos.h>
#include <linux/of.h>
#include <linux/of_address.h>
#if defined(CONFIG_SUPPORT_LEGACY_ION)
#include <linux/exynos_iovmm.h>
#else
#include <linux/ion_exynos.h>
#endif
#if defined(CONFIG_EXYNOS_BTS)
#include <soc/samsung/bts.h>
#endif

#include "decon.h"
/* TODO: SoC dependency will be removed */
#if defined(CONFIG_SOC_EXYNOS9810)
#include "./cal_9810/regs-dpp.h"
#include "./cal_9810/dpp_cal.h"
#elif defined(CONFIG_SOC_EXYNOS9820)
#include "./cal_9820/regs-dpp.h"
#include "./cal_9820/dpp_cal.h"
#elif defined(CONFIG_SOC_EXYNOS9110)
#include "./cal_9110/regs-dpp.h"
#include "./cal_9110/dpp_cal.h"
#endif
#ifdef CONFIG_EXYNOS_MCD_HDR
#include "./mcd_hdr/hdr_drv.h"
#endif

extern int dpp_log_level;

#define DPP_MODULE_NAME		"exynos-dpp"
#define MAX_DPP_CNT		7 /* + ODMA case */
#define MAX_FMT_CNT		64
#define DEFAULT_FMT_CNT		10

/* about 1msec @ ACLK=630MHz */
#define INIT_RCV_NUM		630000

#define P010_Y_SIZE(w, h)		((w) * (h) * 2)
#define P010_CBCR_SIZE(w, h)		((w) * (h))
#define P010_CBCR_BASE(base, w, h)	((base) + P010_Y_SIZE((w), (h)))

#define check_align(width, height, align_w, align_h)\
	(IS_ALIGNED(width, align_w) && IS_ALIGNED(height, align_h))

#define is_normal(config) (DPP_ROT_NORMAL)
#define is_rotation(config) (config->dpp_parm.rot > DPP_ROT_180)
#define is_yuv(config) ((config->format >= DECON_PIXEL_FORMAT_NV16) \
			&& (config->format < DECON_PIXEL_FORMAT_MAX))
#define is_yuv422(config) ((config->format >= DECON_PIXEL_FORMAT_NV16) \
			&& (config->format <= DECON_PIXEL_FORMAT_YVU422_3P))
#define is_yuv420(config) ((config->format >= DECON_PIXEL_FORMAT_NV12) \
			&& (config->format <= DECON_PIXEL_FORMAT_YVU420M))
#define is_afbc(config) (config->compression)

#define dpp_err(fmt, ...)							\
	do {									\
		if (dpp_log_level >= 3) {					\
			pr_err(pr_fmt(fmt), ##__VA_ARGS__);			\
		}								\
	} while (0)

#define dpp_warn(fmt, ...)							\
	do {									\
		if (dpp_log_level >= 4) {					\
			pr_warn(pr_fmt(fmt), ##__VA_ARGS__);			\
		}								\
	} while (0)

#define dpp_info(fmt, ...)							\
	do {									\
		if (dpp_log_level >= 6)					\
			pr_info(pr_fmt(fmt), ##__VA_ARGS__);			\
	} while (0)

#define dpp_dbg(fmt, ...)							\
	do {									\
		if (dpp_log_level >= 7)					\
			pr_info(pr_fmt(fmt), ##__VA_ARGS__);			\
	} while (0)

/* TODO: This will be removed */

enum dpp_csc_defs {
	/* csc_type */
	DPP_CSC_BT_601 = 0,
	DPP_CSC_BT_709 = 1,
	/* csc_range */
	DPP_CSC_NARROW = 0,
	DPP_CSC_WIDE = 1,
	/* csc_mode */
	CSC_COEF_HARDWIRED = 0,
	CSC_COEF_CUSTOMIZED = 1,
	/* csc_id used in csc_3x3_t[] : increase by even value */
	DPP_CSC_ID_BT_2020 = 0,
	DPP_CSC_ID_DCI_P3 = 2,
	CSC_CUSTOMIZED_START = 4,
};

enum dpp_state {
	DPP_STATE_ON,
	DPP_STATE_OFF,
};

enum dpp_reg_area {
	REG_AREA_DPP = 0,
	REG_AREA_DMA,
	REG_AREA_DMA_COM,
};

#ifdef CONFIG_EXYNOS_MCD_HDR
enum hdr_path {
	HDR_PATH_LSI = 0,
	HDR_PATH_MCD,
};

#endif
enum dpp_attr {
	DPP_ATTR_AFBC		= 0,
	DPP_ATTR_BLOCK		= 1,
	DPP_ATTR_FLIP		= 2,
	DPP_ATTR_ROT		= 3,
	DPP_ATTR_CSC		= 4,
	DPP_ATTR_SCALE		= 5,
	DPP_ATTR_HDR		= 6,
	DPP_ATTR_C_HDR		= 7,
	DPP_ATTR_C_HDR10_PLUS	= 8,
	DPP_ATTR_WCG		= 9,
	DPP_ATTR_IDMA		= 16,
	DPP_ATTR_ODMA		= 17,
	DPP_ATTR_DPP		= 18,
};

struct dpp_resources {
	struct clk *gate;
	void __iomem *regs;
	void __iomem *dma_regs;
	void __iomem *dma_com_regs;
	int irq;
	int dma_irq;
};

struct dpp_debug {
	struct timer_list op_timer;
	u32 done_count;
	u32 recovery_cnt;
};

struct dpp_config {
	struct decon_win_config config;
	unsigned long rcv_num;
#ifdef CONFIG_EXYNOS_MCD_HDR
	u32 wcg_mode;
	//struct exynos_video_meta meta;
	struct dpp_hdr10_info hdr_info;
#endif
};

struct dpp_size_range {
	u32 min;
	u32 max;
	u32 align;
};

struct dpp_restriction {
	struct dpp_size_range src_f_w;
	struct dpp_size_range src_f_h;
	struct dpp_size_range src_w;
	struct dpp_size_range src_h;
	u32 src_x_align;
	u32 src_y_align;

	struct dpp_size_range dst_f_w;
	struct dpp_size_range dst_f_h;
	struct dpp_size_range dst_w;
	struct dpp_size_range dst_h;
	u32 dst_x_align;
	u32 dst_y_align;

	struct dpp_size_range blk_w;
	struct dpp_size_range blk_h;
	u32 blk_x_align;
	u32 blk_y_align;

	u32 src_h_rot_max; /* limit of source img height in case of rotation */

	u32 format[MAX_FMT_CNT]; /* supported format list for each DPP channel */
	int format_cnt;

	u32 scale_down;
	u32 scale_up;

	u32 reserved[6];
};

struct dpp_ch_restriction {
	int id;
	unsigned long attr;

	struct dpp_restriction restriction;
	u32 reserved[4];
};

struct dpp_restrictions_info {
	u32 ver; /* version of dpp_restrictions_info structure */
	struct dpp_ch_restriction dpp_ch[MAX_DPP_CNT];
	int dpp_cnt;
	u32 reserved[4];
};

struct dpp_device {
	int id;
	int port;
	unsigned long attr;
	enum dpp_state state;
	struct device *dev;
	struct v4l2_subdev sd;
	struct dpp_resources res;
	struct dpp_debug d;
	wait_queue_head_t framedone_wq;
	struct dpp_config *dpp_config;
	spinlock_t slock;
	spinlock_t dma_slock;
	struct mutex lock;
	struct dpp_restriction restriction;
#ifdef CONFIG_EXYNOS_MCD_HDR
	struct v4l2_subdev *mcd_sd;
	u32 wcg_src_cm;
	u32 wcg_dst_cm;
#endif
};

extern struct dpp_device *dpp_drvdata[MAX_DPP_CNT];

static inline struct dpp_device *get_dpp_drvdata(u32 id)
{
	return dpp_drvdata[id];
}

static inline u32 dpp_read(u32 id, u32 reg_id)
{
	struct dpp_device *dpp = get_dpp_drvdata(id);
	return readl(dpp->res.regs + reg_id);
}

static inline u32 dpp_read_mask(u32 id, u32 reg_id, u32 mask)
{
	u32 val = dpp_read(id, reg_id);
	val &= (~mask);
	return val;
}

static inline void dpp_write(u32 id, u32 reg_id, u32 val)
{
	struct dpp_device *dpp = get_dpp_drvdata(id);
	writel(val, dpp->res.regs + reg_id);
}

static inline void dpp_write_mask(u32 id, u32 reg_id, u32 val, u32 mask)
{
	struct dpp_device *dpp = get_dpp_drvdata(id);
	u32 old = dpp_read(id, reg_id);

	val = (val & mask) | (old & ~mask);
	writel(val, dpp->res.regs + reg_id);
}

/* DPU_DMA Common part */
static inline u32 dma_com_read(u32 id, u32 reg_id)
{
	struct dpp_device *dpp = get_dpp_drvdata(0);
	return readl(dpp->res.dma_com_regs + reg_id);
}

static inline u32 dma_com_read_mask(u32 id, u32 reg_id, u32 mask)
{
	u32 val = dma_com_read(id, reg_id);
	val &= (~mask);
	return val;
}

static inline void dma_com_write(u32 id, u32 reg_id, u32 val)
{
	/* get reliable address when probing IDMA_G0 */
	struct dpp_device *dpp = get_dpp_drvdata(0);
	writel(val, dpp->res.dma_com_regs + reg_id);
}

static inline void dma_com_write_mask(u32 id, u32 reg_id, u32 val, u32 mask)
{
	struct dpp_device *dpp = get_dpp_drvdata(0);
	u32 old = dma_com_read(id, reg_id);

	val = (val & mask) | (old & ~mask);
	writel(val, dpp->res.dma_com_regs + reg_id);
}

/* DPU_DMA */
static inline u32 dma_read(u32 id, u32 reg_id)
{
	struct dpp_device *dpp = get_dpp_drvdata(id);
	return readl(dpp->res.dma_regs + reg_id);
}

static inline u32 dma_read_mask(u32 id, u32 reg_id, u32 mask)
{
	u32 val = dma_read(id, reg_id);
	val &= (~mask);
	return val;
}

static inline void dma_write(u32 id, u32 reg_id, u32 val)
{
	struct dpp_device *dpp = get_dpp_drvdata(id);
	writel(val, dpp->res.dma_regs + reg_id);
}

static inline void dma_write_mask(u32 id, u32 reg_id, u32 val, u32 mask)
{
	struct dpp_device *dpp = get_dpp_drvdata(id);
	u32 old = dma_read(id, reg_id);

	val = (val & mask) | (old & ~mask);
	writel(val, dpp->res.dma_regs + reg_id);
}

static inline void dpp_select_format(struct dpp_device *dpp,
			struct dpp_img_format *vi, struct dpp_params_info *p)
{
	struct decon_win_config *config = &dpp->dpp_config->config;

	vi->normal = is_normal(dpp);
	vi->rot = p->rot;
	vi->scale = p->is_scale;
	vi->format = p->format;
	vi->afbc_en = p->is_comp;
	vi->yuv = is_yuv(config);
	vi->yuv422 = is_yuv422(config);
	vi->yuv420 = is_yuv420(config);
	vi->wb = test_bit(DPP_ATTR_ODMA, &dpp->attr);
}

void dpp_dump(struct dpp_device *dpp);

#define DPP_WIN_CONFIG			_IOW('P', 0, struct dpp_config)
#define DPP_STOP			_IOW('P', 1, unsigned long)
#define DPP_DUMP			_IOW('P', 2, u32)
#define DPP_WB_WAIT_FOR_FRAMEDONE	_IOR('P', 3, u32)
#define DPP_WAIT_IDLE			_IOR('P', 4, unsigned long)
#define DPP_SET_RECOVERY_NUM		_IOR('P', 5, unsigned long)
#define DPP_AFBC_ATTR_ENABLED		_IOR('P', 6, unsigned long)
#define DPP_GET_PORT_NUM		_IOR('P', 7, unsigned long)
#define DPP_GET_RESTRICTION		_IOR('P', 8, unsigned long)
#define DPP_GET_RECOVERY_CNT		_IOR('P', 20, unsigned long)

#endif /* __SAMSUNG_DPP_H__ */
