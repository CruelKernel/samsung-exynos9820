/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Header file for Exynos CAMERA-PP GDC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef CAMERAPP_GDC__H_
#define CAMERAPP_GDC__H_

#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_media.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/io.h>
#include <linux/pm_qos.h>
#include <linux/dma-buf.h>
#include <media/videobuf2-core.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-ctrls.h>
#if defined(CONFIG_VIDEOBUF2_DMA_SG)
#include <media/videobuf2-dma-sg.h>
#endif
#include "camerapp-video.h"

/* #define ENABLE_GDC_FRAMEWISE_DISTORTION_CORRECTION */

extern int gdc_log_level;
#define gdc_dbg(fmt, args...)						\
	do {								\
		if (gdc_log_level)					\
			pr_info("[%s:%d] "				\
			fmt, __func__, __LINE__, ##args);		\
	} while (0)

#define MODULE_NAME		"camerapp-gdc"
#define GDC_TIMEOUT		(2 * HZ)	/* 2 seconds */
#define GDC_WDT_CNT		3

#define GDC_MAX_PLANES		3

/* GDC hardware device state */
#define DEV_RUN		1
#define DEV_SUSPEND	2

/* GDC context state */
#define CTX_PARAMS	1
#define CTX_STREAMING	2
#define CTX_RUN		3
#define CTX_ABORT	4
#define CTX_SRC_FMT	5
#define CTX_DST_FMT	6
#define CTX_INT_FRAME	7 /* intermediate frame available */

/* GDC grid size (x, y) */
#if defined(CONFIG_CAMERA_PP_GDC_V2_1_0_OBJ)
#define GRID_X_SIZE		33
#define GRID_Y_SIZE		33
#else
#define GRID_X_SIZE		9
#define GRID_Y_SIZE		7
#endif

#define fh_to_gdc_ctx(__fh)	container_of(__fh, struct gdc_ctx, fh)
#define gdc_fmt_is_rgb888(x)	((x == V4L2_PIX_FMT_RGB32) || \
		(x == V4L2_PIX_FMT_BGR32))
#define gdc_fmt_is_yuv422(x)	((x == V4L2_PIX_FMT_YUYV) || \
			(x == V4L2_PIX_FMT_UYVY) || (x == V4L2_PIX_FMT_YVYU) || \
			(x == V4L2_PIX_FMT_YUV422P) || (x == V4L2_PIX_FMT_NV16) || \
			(x == V4L2_PIX_FMT_NV61))
#define gdc_fmt_is_yuv420(x)	((x == V4L2_PIX_FMT_YUV420) || \
			(x == V4L2_PIX_FMT_YVU420) || (x == V4L2_PIX_FMT_NV12) || \
			(x == V4L2_PIX_FMT_NV21) || (x == V4L2_PIX_FMT_NV12M) || \
			(x == V4L2_PIX_FMT_NV21M) || (x == V4L2_PIX_FMT_YUV420M) || \
			(x == V4L2_PIX_FMT_YVU420M) || (x == V4L2_PIX_FMT_NV12MT_16X16))
#define gdc_fmt_is_ayv12(x)	((x) == V4L2_PIX_FMT_YVU420)

enum gdc_clk_status {
	GDC_CLK_ON,
	GDC_CLK_OFF,
};

enum gdc_clocks {
	GDC_GATE_CLK,
	GDC_CHLD_CLK,
	GDC_PARN_CLK
};

/*
 * struct gdc_size_limit - GDC variant size information
 *
 * @min_w: minimum pixel width size
 * @min_h: minimum pixel height size
 * @max_w: maximum pixel width size
 * @max_h: maximum pixel height size
 */
struct gdc_size_limit {
	u16 min_w;
	u16 min_h;
	u16 max_w;
	u16 max_h;
};

struct gdc_variant {
	struct gdc_size_limit limit_input;
	struct gdc_size_limit limit_output;
	u32 version;
};

/*
 * struct gdc_fmt - the driver's internal color format data
 * @name: format description
 * @pixelformat: the fourcc code for this format, 0 if not applicable
 * @num_planes: number of physically non-contiguous data planes
 * @num_comp: number of color components(ex. RGB, Y, Cb, Cr)
 * @h_div: horizontal division value of C against Y for crop
 * @v_div: vertical division value of C against Y for crop
 * @bitperpixel: bits per pixel
 * @color: the corresponding gdc_color_fmt
 */
struct gdc_fmt {
	char	*name;
	u32	pixelformat;
	u32	cfg_val;
	u8	bitperpixel[GDC_MAX_PLANES];
	u8	num_planes:2; /* num of buffer */
	u8	num_comp:2;	/* num of hw_plane */
	u8	h_shift:1;
	u8	v_shift:1;
};

struct gdc_addr {
	dma_addr_t	y;
	dma_addr_t	cb;
	dma_addr_t	cr;
	unsigned int	ysize;
	unsigned int	cbsize;
	unsigned int	crsize;
	dma_addr_t	y_2bit;
	dma_addr_t	cbcr_2bit;
	unsigned int	ysize_2bit;
	unsigned int	cbcrsize_2bit;
};

/*
 * struct gdc_frame - source/target frame properties
 * @fmt:	buffer format(like virtual screen)
 * @crop:	image size / position
 * @addr:	buffer start address(access using GDC_ADDR_XXX)
 * @bytesused:	image size in bytes (w x h x bpp)
 */
struct gdc_frame {
	const struct gdc_fmt		*gdc_fmt;
	unsigned short		width;
	unsigned short		height;
	__u32			pixelformat;
	struct gdc_addr			addr;
	__u32			bytesused[GDC_MAX_PLANES];
	enum camerapp_pixel_size pixel_size;
};

/*
 * struct gdc_m2m_device - v4l2 memory-to-memory device data
 * @v4l2_dev: v4l2 device
 * @vfd: the video device node
 * @m2m_dev: v4l2 memory-to-memory device data
 * @in_use: the open count
 */
struct gdc_m2m_device {
	struct v4l2_device	v4l2_dev;
	struct video_device	*vfd;
	struct v4l2_m2m_dev	*m2m_dev;
	atomic_t		in_use;
};

struct gdc_wdt {
	struct timer_list	timer;
	atomic_t		cnt;
};

struct gdc_grid_param {
	bool is_valid;
	u32 prev_width;
	u32 prev_height;
	int dx[GRID_Y_SIZE][GRID_X_SIZE];
	int dy[GRID_Y_SIZE][GRID_X_SIZE];
};

struct gdc_crop_param {
	u32 sensor_num;
	u32 sensor_width;
	u32 sensor_height;
	u32 crop_start_x;
	u32 crop_start_y;
	u32 crop_width;
	u32 crop_height;
	bool is_crop_dzoom;
	bool is_scaled;
	bool use_calculated_grid;
	int calculated_grid_x[GRID_Y_SIZE][GRID_X_SIZE];
	int calculated_grid_y[GRID_Y_SIZE][GRID_X_SIZE];
	int reserved[32];
};

/*
 * gdc_ctx - the abstration for GDC open context
 * @node:		list to be added to gdc_dev.context_list
 * @gdc_dev:		the GDC device this context applies to
 * @m2m_ctx:		memory-to-memory device context
 * @frame:		source frame properties
 * @fh:			v4l2 file handle
 * @flip_rot_cfg:	rotation and flip configuration
 * @bl_op:		image blend mode
 * @dith:		image dithering mode
 * @g_alpha:		global alpha value
 * @color_fill:		enable color fill
 * @flags:		context state flags
 */
struct gdc_ctx {
	struct list_head		node;
	struct gdc_dev		*gdc_dev;
	struct v4l2_m2m_ctx		*m2m_ctx;
	struct gdc_frame		s_frame;
	struct gdc_frame		d_frame;
	struct v4l2_fh		fh;
	unsigned long		flags;
	struct gdc_grid_param		grid_param;
	struct gdc_crop_param		crop_param;
};

struct gdc_priv_buf {
	dma_addr_t	daddr;
	void	*vaddr;
	size_t	size;
};

/*
 * struct gdc_dev - the abstraction for GDC device
 * @dev:	pointer to the GDC device
 * @variant:	the IP variant information
 * @m2m:	memory-to-memory V4L2 device information
 * @aclk:	aclk required for gdc operation
 * @pclk:	pclk required for gdc operation
 * @clk_chld:	child clk of mux required for gdc operation
 * @clk_parn:	parent clk of mux required for gdc operation
 * @regs_base:	the mapped hardware registers
 * @regs_rsc:	the resource claimed for IO registers
 * @wait:	interrupt handler waitqueue
 * @ws:		work struct
 * @state:	device state flags
 * @alloc_ctx:	videobuf2 memory allocator context
 * @slock:	the spinlock pscecting this data structure
 * @lock:	the mutex pscecting this data structure
 * @wdt:	watchdog timer information
 * @version:	IP version number
 * @cfw:	cfw flag
 * @pb_disable:	prefetch-buffer disable flag
 */
struct gdc_dev {
	struct device			*dev;
	const struct gdc_variant		*variant;
	struct gdc_m2m_device		m2m;
	struct clk			*aclk;
	struct clk			*pclk;
	struct clk			*clk_chld;
	struct clk			*clk_parn;
	void __iomem			*regs_base;
	struct resource			*regs_rsc;
	struct workqueue_struct		*qogdc_int_wq;
	wait_queue_head_t		wait;
	unsigned long			state;
	struct vb2_alloc_ctx		*alloc_ctx;
	spinlock_t			slock;
	struct mutex			lock;
	struct gdc_wdt			wdt;
	spinlock_t			ctxlist_lock;
	struct gdc_ctx			*current_ctx;
	struct list_head		context_list; /* for gdc_ctx_abs.node */
	struct pm_qos_request		qosreq_intcam;
	s32				qosreq_intcam_level;
	int				dev_id;
	u32				version;
};

static inline struct gdc_frame *ctx_get_frame(struct gdc_ctx *ctx,
						enum v4l2_buf_type type)
{
	struct gdc_frame *frame;

	if (V4L2_TYPE_IS_MULTIPLANAR(type)) {
		if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
			frame = &ctx->s_frame;
		else
			frame = &ctx->d_frame;
	} else {
		dev_err(ctx->gdc_dev->dev,
				"Wrong V4L2 buffer type %d\n", type);
		return ERR_PTR(-EINVAL);
	}

	return frame;
}

void camerapp_hw_gdc_start(void __iomem *base_addr);
u32 camerapp_hw_gdc_sw_reset(void __iomem *base_addr);
void camerapp_hw_gdc_update_param(void __iomem *base_addr, struct gdc_dev *gdc);
void camerapp_hw_gdc_status_read(void __iomem *base_addr);
void camerapp_gdc_sfr_dump(void __iomem *base_addr);
u32 camerapp_hw_gdc_get_intr_status_and_clear(void __iomem *base_addr);
void camerapp_gdc_grid_setting(struct gdc_dev *gdc);

#ifdef CONFIG_VIDEOBUF2_DMA_SG
static inline dma_addr_t gdc_get_dma_address(struct vb2_buffer *vb2_buf, u32 plane)
{
	return vb2_dma_sg_plane_dma_addr(vb2_buf, plane);
}

static inline void  *gdc_get_kvaddr(struct vb2_buffer *vb2_buf, u32 plane)
{
	return vb2_plane_vaddr(vb2_buf, plane);
}
#else
static inline dma_addr_t gdc_get_dma_address(void *cookie, dma_addr_t *addr)
{
	return NULL;
}

static inline void *gdc_get_kernel_address(void *cookie)
{
	return NULL;
}
#endif
#endif /* CAMERAPP_GDC__H_ */
