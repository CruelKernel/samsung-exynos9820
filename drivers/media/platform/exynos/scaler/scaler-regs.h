/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Register header file for Exynos Scaler driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __SCALER_REGS_H__
#define __SCALER_REGS_H__

#include "scaler.h"

/* Status */
#define SCALER_STATUS			0x00

/* Configuration */
#define SCALER_CFG			0x04
#define SCALER_CFG_DRCG_EN		(1 << 31)
#define SCALER_CFG_CORE_BYP_EN		(1 << 29)
#define SCALER_CFG_SRAM_CG_EN		(1 << 28)
#define SCALER_CFG_FILL_EN		(1 << 24)
#define SCALER_CFG_BL_DIV_ALPHA_EN	(1 << 17)
#define SCALER_CFG_BLEND_EN		(1 << 16)
#define SCALER_CFG_CSC_Y_OFFSET_SRC	(1 << 10)
#define SCALER_CFG_BURST_WR		(1 << 8)
#define SCALER_CFG_BURST_RD		(1 << 7)
#define SCALER_CFG_CSC_Y_OFFSET_DST	(1 << 9)
#define SCALER_CFG_STOP_REQ		(1 << 3)
#define SCALER_CFG_RESET_OKAY		(1 << 2)
#define SCALER_CFG_SOFT_RST		(1 << 1)
#define SCALER_CFG_START_CMD		(1 << 0)

/* Interrupt */
#define SCALER_INT_EN			0x08
#define SCALER_INT_EN_FRAME_END		(1 << 0)
#define SCALER_INT_EN_ALL		0x807fffff
#define SCALER_INT_EN_ALL_v3		0x82ffffff
#define SCALER_INT_EN_ALL_v4		0xb2ffffff
#define SCALER_INT_OK(status)		((status) == SCALER_INT_EN_FRAME_END)

#define SCALER_INT_STATUS		0x0c
#define SCALER_INT_STATUS_FRAME_END	(1 << 0)

#define SCALER_SRC_CFG			0x10

/* Source Image Configuration */
#define SCALER_CFG_TILE_EN		(1 << 10)
#define SCALER_CFG_VHALF_PHASE_EN	(1 << 9)
#define SCALER_CFG_BIG_ENDIAN		(1 << 8)
#define SCALER_CFG_10BIT_P010		(1 << 7)
#define SCALER_CFG_10BIT_S10		(2 << 7)
#define SCALER_CFG_SWAP_MASK		(3 << 5)
#define SCALER_CFG_BYTE_SWAP		(1 << 5)
#define SCALER_CFG_HWORD_SWAP		(2 << 5)
#define SCALER_CFG_BYTE_HWORD_SWAP	(3 << 5)
#define SCALER_CFG_FMT_MASK		(0x1f << 0)
#define SCALER_CFG_FMT_YCBCR420_2P	(0 << 0)
#define SCALER_CFG_FMT_YUYV		(0xa << 0)
#define SCALER_CFG_FMT_UYVY		(0xb << 0)
#define SCALER_CFG_FMT_YVYU		(9 << 0)
#define SCALER_CFG_FMT_YCBCR422_2P	(2 << 0)
#define SCALER_CFG_FMT_YCBCR444_2P	(3 << 0)
#define SCALER_CFG_FMT_RGB565		(4 << 0)
#define SCALER_CFG_FMT_ARGB1555		(5 << 0)
#define SCALER_CFG_FMT_ARGB4444		(0xc << 0)
#define SCALER_CFG_FMT_ARGB8888		(6 << 0)
#define SCALER_CFG_FMT_RGBA8888		(0xe << 0)
#define SCALER_CFG_FMT_P_ARGB8888	(7 << 0)
#define SCALER_CFG_FMT_L8A8		(0xd << 0)
#define SCALER_CFG_FMT_L8		(0xf << 0)
#define SCALER_CFG_FMT_YCRCB420_2P	(0x10 << 0)
#define SCALER_CFG_FMT_YCRCB422_2P	(0x12 << 0)
#define SCALER_CFG_FMT_YCRCB444_2P	(0x13 << 0)
#define SCALER_CFG_FMT_YCBCR420_3P	(0x14 << 0)
#define SCALER_CFG_FMT_YCBCR422_3P	(0x16 << 0)
#define SCALER_CFG_FMT_YCBCR444_3P	(0x17 << 0)

/* Source Y Base Address */
#define SCALER_SRC_Y_BASE		0x14
#define SCALER_SRC_CB_BASE		0x18
#define SCALER_SRC_CR_BASE		0x294
#define SCALER_SRC_SPAN			0x1c
#define SCALER_SRC_CSPAN_MASK		(0xffff << 16)
#define SCALER_SRC_YSPAN_MASK		(0xffff << 0)

#define SCALER_SRC_Y_POS		0x20
#define SCALER_SRC_WH			0x24
#define SCALER_SRC_PRESC_WH		0x2C

#define SCALER_SRC_C_POS		0x28

#define SCALER_DST_CFG			0x30
#define SCALER_DST_Y_BASE		0x34
#define SCALER_DST_CB_BASE		0x38
#define SCALER_DST_CR_BASE		0x298
#define SCALER_DST_SPAN			0x3c
#define SCALER_DST_CSPAN_MASK		(0xffff << 16)
#define SCALER_DST_YSPAN_MASK		(0xffff << 0)

#define SCALER_DST_WH			0x40

#define SCALER_DST_POS			0x44

#define SCALER_H_RATIO			0x50
#define SCALER_V_RATIO			0x54

#define SCALER_ROT_CFG			0x58
#define SCALER_ROT_MASK			(3 << 0)
#define SCALER_FLIP_MASK		(3 << 2)
#define SCALER_FLIP_X_EN		(1 << 3)
#define SCALER_FLIP_Y_EN		(1 << 2)
#define SCALER_ROT_90			(1 << 0)
#define SCALER_ROT_180			(2 << 0)
#define SCALER_ROT_270			(3 << 0)

#define SCALER_LAT_CON			0x5c

#define SCALER_YHCOEF			0x60
#define SCALER_YVCOEF			0xf0
#define SCALER_CHCOEF			0x140
#define SCALER_CVCOEF			0x1d0

#define SCALER_CSC_COEF00		0x220
#define SCALER_CSC_COEF10		0x224
#define SCALER_CSC_COEF20		0x228
#define SCALER_CSC_COEF01		0x22c
#define SCALER_CSC_COEF11		0x230
#define SCALER_CSC_COEF21		0x234
#define SCALER_CSC_COEF02		0x238
#define SCALER_CSC_COEF12		0x23c
#define SCALER_CSC_COEF22		0x240
#define SCALER_CSC_COEF_MASK		(0xffff << 0)

#define SCALER_DITH_CFG			0x250
#define SCALER_DITH_R_MASK		(7 << 6)
#define SCALER_DITH_G_MASK		(7 << 3)
#define SCALER_DITH_B_MASK		(7 << 0)
#define SCALER_DITH_R_SHIFT		(6)
#define SCALER_DITH_G_SHIFT		(3)
#define SCALER_DITH_B_SHIFT		(0)
#define SCALER_DITH_SRC_INV		(1 << 1)
#define SCALER_DITH_DST_EN		(1 << 0)

#define SCALER_VER			0x260

#define SCALER_CRC_COLOR01		0x270
#define SCALER_CRC_COLOR23		0x274
#define SCALER_CYCLE_COUNT		0x278

#define SCALER_SRC_BLEND_COLOR		0x280
#define SCALER_SRC_BLEND_ALPHA		0x284
#define SCALER_DST_BLEND_COLOR		0x288
#define SCALER_DST_BLEND_ALPHA		0x28c
#define SCALER_SEL_INV_MASK		(1 << 31)
#define SCALER_SEL_MASK			(2 << 29)
#define SCALER_OP_SEL_INV_MASK		(1 << 28)
#define SCALER_OP_SEL_MASK		(0xf << 24)
#define SCALER_SEL_INV_SHIFT		(31)
#define SCALER_SEL_SHIFT		(29)
#define SCALER_OP_SEL_INV_SHIFT		(28)
#define SCALER_OP_SEL_SHIFT		(24)

#define SCALER_SRC_2BIT_Y_BASE		0x280
#define SCALER_SRC_2BIT_C_BASE		0x284
#define SCALER_SRC_2BIT_SPAN		0x288
#define SCALER_SRC_2BIT_YSPAN_MASK	(0x7fff << 0)
#define SCALER_SRC_2BIT_CSPAN_MASK	(0x7fff << 16)

#define SCALER_DST_2BIT_Y_BASE		0x2A0
#define SCALER_DST_2BIT_C_BASE		0x2A4
#define SCALER_DST_2BIT_SPAN		0x2A8
#define SCALER_DST_2BIT_YSPAN_MASK	(0x7fff << 0)
#define SCALER_DST_2BIT_CSPAN_MASK	(0x7fff << 16)

#define SCALER_FILL_COLOR		0x290

#define SCALER_TIMEOUT_CTRL		0x2c0
#define SCALER_TIMEOUT_CNT		0x2c4
#define SCALER_CLK_REQ			0x2cc

#define SCALER_SRC_YH_INIT_PHASE	0x2d0
#define SCALER_SRC_YV_INIT_PHASE	0x2d4
#define SCALER_SRC_CH_INIT_PHASE	0x2d8
#define SCALER_SRC_CV_INIT_PHASE	0x2dc

/* macros to make words to SFR */
#define SCALER_VAL_WH(w, h)	 (((w) & 0x3FFF) << 16) | ((h) & 0x3FFF)
#define SCALER_VAL_SRC_POS(l, t) (((l) & 0x3FFF) << 18) | (((t) & 0x3FFF) << 2)
#define SCALER_VAL_DST_POS(l, t) (((l) & 0x3FFF) << 16) | ((t) & 0x3FFF)

static inline void sc_hwset_clk_request(struct sc_dev *sc, bool enable)
{
	if (sc->version >= SCALER_VERSION(5, 0, 1))
		__raw_writel(enable ? 1 : 0, sc->regs + SCALER_CLK_REQ);
}

static inline void sc_hwset_src_pos(struct sc_dev *sc, __s32 left, __s32 top,
				unsigned int chshift, unsigned int cvshift)
{
	/* SRC pos have fractional part of 2 bits which is not used */
	__raw_writel(SCALER_VAL_SRC_POS(left, top),
			sc->regs + SCALER_SRC_Y_POS);
	__raw_writel(SCALER_VAL_SRC_POS(left >> chshift, top >> cvshift),
			sc->regs + SCALER_SRC_C_POS);
}

static inline void sc_hwset_src_wh(struct sc_dev *sc, __s32 width, __s32 height,
			unsigned int pre_h_ratio, unsigned int pre_v_ratio,
			unsigned int chshift, unsigned int cvshift)
{
	__s32 pre_width = round_down(width >> pre_h_ratio, 1 << chshift);
	__s32 pre_height = round_down(height >> pre_v_ratio, 1 << cvshift);
	sc_dbg("width %d, height %d\n", pre_width, pre_height);

	if (sc->variant->prescale) {
		/*
		 * crops the width and height if the pre-scaling result violates
		 * the width/height constraints:
		 *  - result width or height is not a natural number
		 *  - result width or height violates the constrains
		 *    of YUV420/422
		 */
		width = pre_width << pre_h_ratio;
		height = pre_height << pre_v_ratio;
		__raw_writel(SCALER_VAL_WH(width, height),
				sc->regs + SCALER_SRC_PRESC_WH);
	}

	__raw_writel(SCALER_VAL_WH(pre_width, pre_height),
				sc->regs + SCALER_SRC_WH);
}

static inline void sc_hwset_dst_pos(struct sc_dev *sc, __s32 left, __s32 top)
{
	__raw_writel(SCALER_VAL_DST_POS(left, top), sc->regs + SCALER_DST_POS);
}

static inline void sc_hwset_dst_wh(struct sc_dev *sc, __s32 width, __s32 height)
{
	__raw_writel(SCALER_VAL_WH(width, height), sc->regs + SCALER_DST_WH);
}

static inline void sc_hwset_hratio(struct sc_dev *sc, u32 ratio, u32 pre_ratio)
{
	__raw_writel((pre_ratio << 28) | ratio, sc->regs + SCALER_H_RATIO);
}

static inline void sc_hwset_vratio(struct sc_dev *sc, u32 ratio, u32 pre_ratio)
{
	__raw_writel((pre_ratio << 28) | ratio, sc->regs + SCALER_V_RATIO);
}

static inline void sc_hwset_flip_rotation(struct sc_dev *sc, u32 flip_rot_cfg)
{
	__raw_writel(flip_rot_cfg & 0xF, sc->regs + SCALER_ROT_CFG);
}

static inline void sc_hwset_int_en(struct sc_dev *sc)
{
	unsigned int val;

	if (sc->version < SCALER_VERSION(3, 0, 0))
		val = SCALER_INT_EN_ALL;
	else if (sc->version < SCALER_VERSION(4, 0, 1) ||
			sc->version == SCALER_VERSION(4, 2, 0))
		val = SCALER_INT_EN_ALL_v3;
	else
		val = SCALER_INT_EN_ALL_v4;
	__raw_writel(val, sc->regs + SCALER_INT_EN);
}

static inline void sc_clear_aux_power_cfg(struct sc_dev *sc)
{
	/* Clearing all power saving features */
	__raw_writel(__raw_readl(sc->regs + SCALER_CFG) & ~SCALER_CFG_DRCG_EN,
			sc->regs + SCALER_CFG);
}

static inline void sc_hwset_bus_idle(struct sc_dev *sc)
{
	if (sc->version == SCALER_VERSION(5, 0, 1)) {
		int cnt = 1000;

		__raw_writel(SCALER_CFG_STOP_REQ, sc->regs + SCALER_CFG);

		while (cnt-- > 0)
			if (__raw_readl(sc->regs + SCALER_CFG)
						& SCALER_CFG_RESET_OKAY)
				break;

		WARN_ON(cnt <= 0);
	}
}

static inline void sc_hwset_init(struct sc_dev *sc)
{
	unsigned long cfg;

	sc_hwset_bus_idle(sc);

#ifdef SC_NO_SOFTRST
	cfg = (SCALER_CFG_CSC_Y_OFFSET_SRC | SCALER_CFG_CSC_Y_OFFSET_DST);
#else
	cfg = SCALER_CFG_SOFT_RST;
#endif
	writel(cfg, sc->regs + SCALER_CFG);

	if (sc->version >= SCALER_VERSION(3, 0, 1))
		__raw_writel(
			__raw_readl(sc->regs + SCALER_CFG) | SCALER_CFG_DRCG_EN,
			sc->regs + SCALER_CFG);
	if (sc->version >= SCALER_VERSION(4, 0, 1) &&
			sc->version != SCALER_VERSION(4, 2, 0)) {
		__raw_writel(
			__raw_readl(sc->regs + SCALER_CFG) | SCALER_CFG_BURST_RD,
			sc->regs + SCALER_CFG);
		__raw_writel(
			__raw_readl(sc->regs + SCALER_CFG) & ~SCALER_CFG_BURST_WR,
			sc->regs + SCALER_CFG);
	}
}

static inline void sc_hwset_soft_reset(struct sc_dev *sc)
{
	sc_hwset_bus_idle(sc);
	writel(SCALER_CFG_SOFT_RST, sc->regs + SCALER_CFG);
}

static inline void sc_hwset_start(struct sc_dev *sc)
{
	unsigned long cfg = __raw_readl(sc->regs + SCALER_CFG);

	cfg |= SCALER_CFG_START_CMD;
	if (sc->version >= SCALER_VERSION(3, 0, 1)) {
		cfg |= SCALER_CFG_CORE_BYP_EN;
		cfg |= SCALER_CFG_SRAM_CG_EN;
	}
	writel(cfg, sc->regs + SCALER_CFG);
}

u32 sc_hwget_and_clear_irq_status(struct sc_dev *sc);

#define SCALER_FRACT_VAL(x)		(x << (20 - SC_CROP_FRACT_MULTI))
#define SCALER_INIT_PHASE_VAL(i, f)	(((i) & 0xf << 20) | \
					((SCALER_FRACT_VAL(f)) & 0xfffff))
static inline void sc_hwset_src_init_phase(struct sc_dev *sc, struct sc_init_phase *ip)
{
	if (ip->yh) {
		__raw_writel(SCALER_INIT_PHASE_VAL(0, ip->yh),
			sc->regs + SCALER_SRC_YH_INIT_PHASE);
		__raw_writel(SCALER_INIT_PHASE_VAL(0, ip->ch),
			sc->regs + SCALER_SRC_CH_INIT_PHASE);
		sc_dbg("initial phase value is yh 0x%x, ch 0x%x\n",
			SCALER_FRACT_VAL(ip->yh), SCALER_FRACT_VAL(ip->ch));
	}

	if (ip->yv) {
		__raw_writel(SCALER_INIT_PHASE_VAL(0, ip->yv),
			sc->regs + SCALER_SRC_YV_INIT_PHASE);
		__raw_writel(SCALER_INIT_PHASE_VAL(0, ip->cv),
			sc->regs + SCALER_SRC_CV_INIT_PHASE);
		sc_dbg("initial phase value is yv 0x%x, cv 0x%x\n",
			SCALER_FRACT_VAL(ip->yv), SCALER_FRACT_VAL(ip->cv));
	}
}

void sc_hwset_polyphase_hcoef(struct sc_dev *sc,
		unsigned int yratio, unsigned int cratio, unsigned int filter);
void sc_hwset_polyphase_vcoef(struct sc_dev *sc,
		unsigned int yratio, unsigned int cratio, unsigned int filter);

#endif /*__SCALER_REGS_H__*/
