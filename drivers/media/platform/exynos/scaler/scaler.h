/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Header file for Exynos Scaler driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef SCALER__H_
#define SCALER__H_

#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_media.h>
#include <linux/io.h>
#include <linux/pm_qos.h>
#include <linux/dma-buf.h>
#include <media/videobuf2-core.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-ctrls.h>
#include <soc/samsung/exynos-itmon.h>

extern int sc_log_level;
#define sc_dbg(fmt, args...)						\
	do {								\
		if (sc_log_level)					\
			pr_debug("[%s:%d] "				\
			fmt, __func__, __LINE__, ##args);		\
	} while (0)

#define MODULE_NAME		"exynos5-scaler"
#define SC_MAX_DEVS		1
#define SC_TIMEOUT		(2 * HZ)	/* 2 seconds */
#define SC_WDT_CNT		3
#define SC_MAX_CTRL_NUM		11

#define SC_MAX_PLANES		3
/* Address index */
#define SC_ADDR_RGB		0
#define SC_ADDR_Y		0
#define SC_ADDR_CB		1
#define SC_ADDR_CBCR		1
#define SC_ADDR_CR		2

/* Scaler hardware device state */
#define DEV_RUN		1
#define DEV_SUSPEND	2
#define DEV_CP		4 /* contents path protection */

/* Scaler m2m context state */
#define CTX_PARAMS	1
#define CTX_STREAMING	2
#define CTX_RUN		3
#define CTX_ABORT	4
#define CTX_SRC_FMT	5
#define CTX_DST_FMT	6
#define CTX_INT_FRAME	7 /* intermediate frame available */
#define CTX_INT_FRAME_CP 8 /* intermediate frame available */


/* CSC equation */
#define SC_CSC_NARROW	0
#define SC_CSC_WIDE	1

/* Scaler Crop Fixed Point value */
#define SC_CROP_FRACT_SHIFT	15
#define SC_CROP_FRACT_MULTI	16
#define SC_CROP_FRACT_MASK	((1 << SC_CROP_FRACT_MULTI) - 1)
#define SC_CROP_INT_MASK	((1 << SC_CROP_FRACT_SHIFT) - 1)
#define SC_CROP_GET_FR_VAL(x)	((x >> SC_CROP_FRACT_SHIFT) & SC_CROP_FRACT_MASK)
#define SC_CROP_MAKE_FR_VAL(i, f)	((((f) & SC_CROP_FRACT_MASK) \
			<< SC_CROP_FRACT_SHIFT) | ((i) & SC_CROP_INT_MASK))

#define fh_to_sc_ctx(__fh)	container_of(__fh, struct sc_ctx, fh)
#define sc_fmt_is_rgb888(x)	((x == V4L2_PIX_FMT_RGB32) || \
		(x == V4L2_PIX_FMT_BGR32))
#define sc_fmt_is_yuv422(x)	((x == V4L2_PIX_FMT_YUYV) || \
		(x == V4L2_PIX_FMT_UYVY) || (x == V4L2_PIX_FMT_YVYU) || \
		(x == V4L2_PIX_FMT_YUV422P) || (x == V4L2_PIX_FMT_NV16) || \
		(x == V4L2_PIX_FMT_NV61))
#define sc_fmt_is_yuv420(x)	((x == V4L2_PIX_FMT_YUV420) || \
		(x == V4L2_PIX_FMT_YVU420) || (x == V4L2_PIX_FMT_NV12) || \
		(x == V4L2_PIX_FMT_NV21) || (x == V4L2_PIX_FMT_NV12M) || \
		(x == V4L2_PIX_FMT_NV21M) || (x == V4L2_PIX_FMT_YUV420M) || \
		(x == V4L2_PIX_FMT_YVU420M) || (x == V4L2_PIX_FMT_NV12MT_16X16))
#define sc_fmt_is_ayv12(x)	((x) == V4L2_PIX_FMT_YVU420)
#define sc_fmt_is_s10bit_yuv(x)	((x == V4L2_PIX_FMT_NV12M_S10B) || \
		(x == V4L2_PIX_FMT_NV12N_10B) || (x == V4L2_PIX_FMT_NV16M_S10B) || \
		(x == V4L2_PIX_FMT_NV61M_S10B))
#define sc_dith_val(a, b, c)	((a << SCALER_DITH_R_SHIFT) |	\
		(b << SCALER_DITH_G_SHIFT) | (c << SCALER_DITH_B_SHIFT))

#define SCALER_VERSION(x, y, z) (((x) << 16) | ((y) << 8) | (z))

#define SC_FMT_PREMULTI_FLAG	10

/* for blend operation */
#define V4L2_CID_GLOBAL_ALPHA		(V4L2_CID_EXYNOS_BASE + 1)
#define V4L2_CID_2D_BLEND_OP		(V4L2_CID_EXYNOS_BASE + 103)
#define V4L2_CID_2D_COLOR_FILL		(V4L2_CID_EXYNOS_BASE + 104)
#define V4L2_CID_2D_DITH		(V4L2_CID_EXYNOS_BASE + 105)
#define V4L2_CID_2D_FMT_PREMULTI	(V4L2_CID_EXYNOS_BASE + 106)

/* for performance */
#define SC_CID_FRAMERATE		(V4L2_CID_EXYNOS_BASE + 110)
#define SC_FRAMERATE_MAX		(500)

/* for denoising filter */
#define SC_CID_DNOISE_FT		(V4L2_CID_EXYNOS_BASE + 150)
#define SC_M2M1SHOT_OP_FILTER_SHIFT	(28)
#define SC_M2M1SHOT_OP_FILTER_MASK	(0xf << 28)

enum sc_csc_idx {
	NO_CSC,
	CSC_Y2R,
	CSC_R2Y,
};

struct sc_csc_tab {
	int narrow_601[9];
	int wide_601[9];
	int narrow_709[9];
	int wide_709[9];
	int narrow_2020[9];
	int wide_2020[9];
};

enum sc_clk_status {
	SC_CLK_ON,
	SC_CLK_OFF,
};

enum sc_clocks {
	SC_GATE_CLK,
	SC_CHLD_CLK,
	SC_PARN_CLK
};

enum sc_dith {
	SC_DITH_NO,
	SC_DITH_8BIT,
	SC_DITH_6BIT,
	SC_DITH_5BIT,
	SC_DITH_4BIT,
};

/*
 * blending operation
 * The order is from Android PorterDuff.java
 */
enum sc_blend_op {
	/* [0, 0] */
	BL_OP_CLR = 1,
	/* [Sa, Sc] */
	BL_OP_SRC,
	/* [Da, Dc] */
	BL_OP_DST,
	/* [Sa + (1 - Sa)*Da, Rc = Sc + (1 - Sa)*Dc] */
	BL_OP_SRC_OVER,
	/* [Sa + (1 - Sa)*Da, Rc = Dc + (1 - Da)*Sc] */
	BL_OP_DST_OVER,
	/* [Sa * Da, Sc * Da] */
	BL_OP_SRC_IN,
	/* [Sa * Da, Sa * Dc] */
	BL_OP_DST_IN,
	/* [Sa * (1 - Da), Sc * (1 - Da)] */
	BL_OP_SRC_OUT,
	/* [Da * (1 - Sa), Dc * (1 - Sa)] */
	BL_OP_DST_OUT,
	/* [Da, Sc * Da + (1 - Sa) * Dc] */
	BL_OP_SRC_ATOP,
	/* [Sa, Sc * (1 - Da) + Sa * Dc ] */
	BL_OP_DST_ATOP,
	/* [-(Sa * Da), Sc * (1 - Da) + (1 - Sa) * Dc] */
	BL_OP_XOR,
	/* [Sa + Da - Sa*Da, Sc*(1 - Da) + Dc*(1 - Sa) + min(Sc, Dc)] */
	BL_OP_DARKEN,
	/* [Sa + Da - Sa*Da, Sc*(1 - Da) + Dc*(1 - Sa) + max(Sc, Dc)] */
	BL_OP_LIGHTEN,
	/** [Sa * Da, Sc * Dc] */
	BL_OP_MULTIPLY,
	/* [Sa + Da - Sa * Da, Sc + Dc - Sc * Dc] */
	BL_OP_SCREEN,
	/* Saturate(S + D) */
	BL_OP_ADD,
};

/*
 * Co = <src color op> * Cs + <dst color op> * Cd
 * Ao = <src_alpha_op> * As + <dst_color_op> * Ad
 */
#define BL_INV_BIT_OFFSET	0x10

enum sc_bl_comp {
	ONE = 0,
	SRC_A,
	SRC_C,
	DST_A,
	SRC_GA = 0x5,
	INV_SA = 0x11,
	INV_SC,
	INV_DA,
	INV_SAGA = 0x17,
	ZERO = 0xff,
};

struct sc_bl_op_val {
	u32 src_color;
	u32 src_alpha;
	u32 dst_color;
	u32 dst_alpha;
};

/*
 * struct sc_size_limit - Scaler variant size information
 *
 * @min_w: minimum pixel width size
 * @min_h: minimum pixel height size
 * @max_w: maximum pixel width size
 * @max_h: maximum pixel height size
 */
struct sc_size_limit {
	u16 min_w;
	u16 min_h;
	u16 max_w;
	u16 max_h;
};

struct sc_variant {
	struct sc_size_limit limit_input;
	struct sc_size_limit limit_output;
	u32 version;
	u32 sc_up_max;
	u32 sc_down_min;
	u32 sc_up_swmax;
	u32 sc_down_swmin;
	u32 minsize_srcplane;		/* zero if no limitation exists */
	u8 blending:1;
	u8 prescale:1;
	u8 ratio_20bit:1;
	u8 initphase:1;
	u8 pixfmt_10bit:1;
	u8 extra_buf:1;
};

/*
 * struct sc_fmt - the driver's internal color format data
 * @name: format description
 * @pixelformat: the fourcc code for this format, 0 if not applicable
 * @num_planes: number of physically non-contiguous data planes
 * @num_comp: number of color components(ex. RGB, Y, Cb, Cr)
 * @h_div: horizontal division value of C against Y for crop
 * @v_div: vertical division value of C against Y for crop
 * @bitperpixel: bits per pixel
 * @color: the corresponding sc_color_fmt
 */
struct sc_fmt {
	char	*name;
	u32	pixelformat;
	u32	cfg_val;
	u8	bitperpixel[SC_MAX_PLANES];
	u8	num_planes:2;
	u8	num_comp:2;
	u8	h_shift:1;
	u8	v_shift:1;
	u8	is_rgb:1;
	u8	cspan:1;
};

enum {
	SC_PLANE_Y = 0,
	SC_PLANE_CB,
	SC_PLANE_CR,
};

struct sc_addr {
	dma_addr_t	ioaddr[SC_MAX_PLANES];
	unsigned int	size[SC_MAX_PLANES];
};

/*
 * struct sc_frame - source/target frame properties
 * @fmt:	buffer format(like virtual screen)
 * @crop:	image size / position
 * @addr:	buffer start address(access using SC_ADDR_XXX)
 * @bytesused:	image size in bytes (w x h x bpp)
 */
struct sc_frame {
	const struct sc_fmt		*sc_fmt;
	unsigned short		width;
	unsigned short		height;
	__u32			pixelformat;
	struct v4l2_rect	crop;

	struct sc_addr			addr;
	__u32			bytesused[SC_MAX_PLANES];
	bool			pre_multi;
};

struct sc_int_frame {
	struct sc_frame			frame;
	struct sc_addr			src_addr;
	struct sc_addr			dst_addr;
	struct sg_table			*sgt[3];
	struct dma_buf			*dma_buf[3];
	struct dma_buf_attachment	*attachment[3];
};

/*
 * struct sc_m2m_device - v4l2 memory-to-memory device data
 * @v4l2_dev: v4l2 device
 * @vfd: the video device node
 * @m2m_dev: v4l2 memory-to-memory device data
 * @in_use: the open count
 */
struct sc_m2m_device {
	struct v4l2_device	v4l2_dev;
	struct video_device	*vfd;
	struct v4l2_m2m_dev	*m2m_dev;
	atomic_t		in_use;
};

struct sc_wdt {
	struct timer_list	timer;
	atomic_t		cnt;
};

struct sc_csc {
	unsigned int		csc_eq;
	bool			csc_range;
};

struct sc_init_phase {
	u32			yh;
	u32			yv;
	u32			ch;
	u32			cv;
	u32			w;
	u32			h;
};

enum sc_ft {
	SC_FT_NONE = 0,
	SC_FT_BLUR,
	SC_FT_240,
	SC_FT_480,
	SC_FT_720,
	SC_FT_960,
	SC_FT_1080,
	SC_FT_MAX,
};

struct sc_dnoise_filter {
	u32			strength;
	u32			w;
	u32			h;
};

struct sc_qos_table {
	unsigned int freq_mif;
	unsigned int freq_int;
	unsigned int data_size;
};

struct sc_ppc_table {
	unsigned int bpp;
	unsigned int ppc[2];
};

struct sc_ctx;

/*
 * struct sc_dev - the abstraction for Rotator device
 * @dev:	pointer to the Rotator device
 * @variant:	the IP variant information
 * @m2m:	memory-to-memory V4L2 device information
 * @aclk:	aclk required for scaler operation
 * @pclk:	pclk required for scaler operation
 * @clk_chld:	child clk of mux required for scaler operation
 * @clk_parn:	parent clk of mux required for scaler operation
 * @regs:	the mapped hardware registers
 * @regs_res:	the resource claimed for IO registers
 * @wait:	interrupt handler waitqueue
 * @ws:		work struct
 * @state:	device state flags
 * @alloc_ctx:	videobuf2 memory allocator context
 * @slock:	the spinlock pscecting this data structure
 * @lock:	the mutex pscecting this data structure
 * @wdt:	watchdog timer information
 * @version:	IP version number
 * @cfw:	cfw flag
 * @pb_disable:       prefetch-buffer disable flag
 */
struct sc_dev {
	struct device			*dev;
	const struct sc_variant		*variant;
	struct sc_m2m_device		m2m;
	struct m2m1shot_device		*m21dev;
	struct clk			*aclk;
	struct clk			*pclk;
	struct clk			*clk_chld;
	struct clk			*clk_parn;
	void __iomem			*regs;
	struct resource			*regs_res;
	struct workqueue_struct		*qosclr_int_wq;
	wait_queue_head_t		wait;
	unsigned long			state;
	struct vb2_alloc_ctx		*alloc_ctx;
	spinlock_t			slock;
	struct mutex			lock;
	struct sc_wdt			wdt;
	spinlock_t			ctxlist_lock;
	struct sc_ctx			*current_ctx;
	struct list_head		context_list; /* for sc_ctx_abs.node */
	struct pm_qos_request		qosreq_int;
	s32				qosreq_int_level;
	int				dev_id;
	u32				version;
	bool				pb_disable;
	u32				cfw;
	struct sc_qos_table		*qos_table;
	struct sc_ppc_table		*ppc_table;
	int qos_table_cnt;
	int ppc_table_cnt;
	struct notifier_block itmon_nb;
};

enum SC_CONTEXT_TYPE {
	SC_CTX_V4L2_TYPE,
	SC_CTX_M2M1SHOT_TYPE
};

struct sc_qos_request {
	struct pm_qos_request mif_req;
	struct pm_qos_request int_req;
};

/*
 * sc_ctx - the abstration for Rotator open context
 * @node:		list to be added to sc_dev.context_list
 * @context_type	determines if the context is @m2m_ctx or @m21_ctx.
 * @sc_dev:		the Rotator device this context applies to
 * @m2m_ctx:		memory-to-memory device context
 * @m21_ctx:		m2m1shot context
 * @frame:		source frame properties
 * @ctrl_handler:	v4l2 controls handler
 * @fh:			v4l2 file handle
 * @ktime:		start time of a task of m2m1shot
 * @flip_rot_cfg:	rotation and flip configuration
 * @bl_op:		image blend mode
 * @dith:		image dithering mode
 * @g_alpha:		global alpha value
 * @color_fill:		enable color fill
 * @flags:		context state flags
 * @pre_multi:		pre-multiplied format
 * @csc:		csc equation value
 */
struct sc_ctx {
	struct list_head		node;
	enum SC_CONTEXT_TYPE		context_type;
	struct sc_dev			*sc_dev;
	union {
		struct v4l2_m2m_ctx	*m2m_ctx;
		struct m2m1shot_context	*m21_ctx;
	};
	struct sc_frame			s_frame;
	struct sc_int_frame		*i_frame;
	struct sc_frame			d_frame;
	struct v4l2_ctrl_handler	ctrl_handler;
	union {
		struct v4l2_fh		fh;
		ktime_t			ktime_m2m1shot;
	};
	u32				flip_rot_cfg; /* SCALER_ROT_CFG */
	enum sc_blend_op		bl_op;
	u32				dith;
	u32				g_alpha;
	bool				color_fill;
	unsigned int			h_ratio;
	unsigned int			v_ratio;
	unsigned int			pre_h_ratio;
	unsigned int			pre_v_ratio;
	unsigned long			flags;
	bool				pre_multi;
	bool				cp_enabled;
	struct sc_csc			csc;
	struct sc_init_phase		init_phase;
	struct sc_dnoise_filter		dnoise_ft;
	struct delayed_work		qos_work;
	struct sc_qos_request		pm_qos;
	struct mutex			pm_qos_lock;
	int				pm_qos_lv;
	int				framerate;
};

static inline struct sc_frame *ctx_get_frame(struct sc_ctx *ctx,
						enum v4l2_buf_type type)
{
	struct sc_frame *frame;

	if (V4L2_TYPE_IS_MULTIPLANAR(type)) {
		if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
			frame = &ctx->s_frame;
		else
			frame = &ctx->d_frame;
	} else {
		dev_err(ctx->sc_dev->dev,
				"Wrong V4L2 buffer type %d\n", type);
		return ERR_PTR(-EINVAL);
	}

	return frame;
}

int sc_hwset_src_image_format(struct sc_dev *sc, const struct sc_fmt *);
int sc_hwset_dst_image_format(struct sc_dev *sc, const struct sc_fmt *);
void sc_hwset_pre_multi_format(struct sc_dev *sc, bool src, bool dst);
void sc_hwset_blend(struct sc_dev *sc, enum sc_blend_op bl_op, bool pre_multi,
		unsigned char g_alpha);
void sc_hwset_color_fill(struct sc_dev *sc, unsigned int val);
void sc_hwset_dith(struct sc_dev *sc, unsigned int val);
void sc_hwset_csc_coef(struct sc_dev *sc, enum sc_csc_idx idx,
		struct sc_csc *csc);
void sc_hwset_src_imgsize(struct sc_dev *sc, struct sc_frame *frame);
void sc_hwset_dst_imgsize(struct sc_dev *sc, struct sc_frame *frame);
void sc_hwset_src_crop(struct sc_dev *sc, struct v4l2_rect *rect,
		       const struct sc_fmt *fmt,
		       unsigned int pre_h_ratio, unsigned int pre_v_ratio);
void sc_hwset_dst_crop(struct sc_dev *sc, struct v4l2_rect *rect);
void sc_hwset_src_addr(struct sc_dev *sc, struct sc_frame *frame);
void sc_hwset_dst_addr(struct sc_dev *sc, struct sc_frame *frame);
void sc_hwset_hcoef(struct sc_dev *sc, unsigned int coef);
void sc_hwset_vcoef(struct sc_dev *sc, unsigned int coef);

void sc_hwregs_dump(struct sc_dev *sc);
void sc_ctx_dump(struct sc_ctx *ctx);

#endif /* SCALER__H_ */
