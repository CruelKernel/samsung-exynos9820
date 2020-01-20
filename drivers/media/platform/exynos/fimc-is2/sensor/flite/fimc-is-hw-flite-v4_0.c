/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/slab.h>

#include <exynos-fimc-is-sensor.h>
#include "fimc-is-regs.h"
#include "fimc-is-hw.h"
#include "fimc-is-interface.h"
#include "fimc-is-hw-flite-v4_0.h"

static void flite_hw_enable_bns(u32 __iomem *base_reg, bool enable)
{
	u32 cfg = 0;

	/* enable */
	cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_BINNINGON));
	cfg |= FLITE_REG_BINNINGON_CLKGATE_ON(enable);
	cfg |= FLITE_REG_BINNINGON_EN(enable);
	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_BINNINGON));
}

static void flite_hw_s_coeff_bns(u32 __iomem *base_reg,
	u32 factor_x, u32 factor_y)
{
	int i;
	int index;
	u32 cfg = 0;
	u32 x_cfg = 0;
	u32 y_cfg = 0;
	u32 unity_size;
	u32 unity;
	u32 weight_x[16] = {0, }, weight_y[16] = {0, };

	if (factor_x != factor_y)
		err("[BNS] x y factor is not the same (%d, %d)", factor_x, factor_y);

	/* control */
	cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_BINNINGCTRL));
	cfg |= FLITE_REG_BINNINGCTRL_FACTOR_Y(factor_y);
	cfg |= FLITE_REG_BINNINGCTRL_FACTOR_X(factor_x);
	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_BINNINGCTRL));

	unity_size = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_BINNINGCTRL)) & 0xF;
	unity = 1 << unity_size;

	for(i = 0; bns_scales[factor_x][i] != 0; i++)
		weight_x[i] = bns_scales[factor_x][i] * unity / 1000;

	for(i = 0; bns_scales[factor_y][i] != 0; i++)
		weight_y[i] = bns_scales[factor_y][i] * unity / 1000;

	for (i = 0; i < 8; i++) {
		index = i * 2;
		x_cfg = (weight_x[index + 1] << 16) | (weight_x[index] << 0);
		y_cfg = (weight_y[index + 1] << 16) | (weight_y[index] << 0);

		writel(x_cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_WEIGHTX01) + i);
		writel(y_cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_WEIGHTY01) + i);
	}
}

static void flite_hw_s_size_bns(u32 __iomem *base_reg,
	u32 width, u32 height, u32 otf_width, u32 otf_height)
{
	u32 cfg = 0;

	/* size */
	cfg = FLITE_REG_BINNINGTOTAL_HEIGHT(height);
	cfg |= FLITE_REG_BINNINGTOTAL_WIDTH(width);
	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_BINNINGTOTAL));

	cfg = FLITE_REG_BINNINGINPUT_HEIGHT(height);
	cfg |= FLITE_REG_BINNINGINPUT_WIDTH(width);
	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_BINNINGINPUT));

	cfg = FLITE_REG_BINNINGMARGIN_TOP(0);
	cfg |= FLITE_REG_BINNINGMARGIN_LEFT(0);
	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_BINNINGMARGIN));

	cfg = FLITE_REG_BINNINGOUTPUT_HEIGHT(otf_height);
	cfg |= FLITE_REG_BINNINGOUTPUT_WIDTH(otf_width);
	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_BINNINGOUTPUT));
}

static void flite_hw_set_cam_source_size(u32 __iomem *base_reg,
	struct fimc_is_image *image)
{
	u32 cfg = 0;

	FIMC_BUG(!image);

#ifdef COLORBAR_MODE
	cfg |= FLITE_REG_CISRCSIZE_SIZE_H(640);
	cfg |= FLITE_REG_CISRCSIZE_SIZE_V(480);
#else
	cfg |= FLITE_REG_CISRCSIZE_SIZE_H(image->window.o_width);
	cfg |= FLITE_REG_CISRCSIZE_SIZE_V(image->window.o_height);
#endif

	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_CISRCSIZE));
}

static void flite_hw_set_dma_offset(u32 __iomem *base_reg,
	struct fimc_is_image *image)
{
	u32 cfg = 0;

	FIMC_BUG(!image);

	switch (image->format.pixelformat) {
	case V4L2_PIX_FMT_SBGGR8:
	case V4L2_PIX_FMT_SGBRG8:
	case V4L2_PIX_FMT_SGRBG8:
	case V4L2_PIX_FMT_SRGGB8:
		cfg |= FLITE_REG_CIOCAN_OCAN_H(roundup(image->window.o_width, 16));
		break;
	case V4L2_PIX_FMT_SBGGR10:
	case V4L2_PIX_FMT_SGBRG10:
	case V4L2_PIX_FMT_SGRBG10:
	case V4L2_PIX_FMT_SRGGB10:
	case V4L2_PIX_FMT_SBGGR12:
	case V4L2_PIX_FMT_SGBRG12:
	case V4L2_PIX_FMT_SGRBG12:
	case V4L2_PIX_FMT_SRGGB12:
		/* always input format is raw10 but this means padding bit */
		cfg |= FLITE_REG_CIOCAN_OCAN_H(roundup((image->window.o_width * 3) / 2, 16));
		break;
	case V4L2_PIX_FMT_SBGGR16:
	case V4L2_PIX_FMT_YUYV:
		cfg |= FLITE_REG_CIOCAN_OCAN_H(roundup(image->window.o_width * 2, 16));
		break;
	default:
		warn("[BNS] unsupprted format(%d)", image->format.pixelformat);
		break;
	}

	cfg |= FLITE_REG_CIOCAN_OCAN_V(image->window.o_height);

	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_CIOCAN));
}

static void flite_hw_set_lineint_ratio(u32 __iomem *base_reg, u32 ratio)
{
	writel(ratio, base_reg + TO_WORD_OFFSET(FLITE_REG_LINEINT));
}

static void flite_hw_set_capture_start(u32 __iomem *base_reg)
{
	u32 cfg;

	cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIIMGCPT));
	cfg |= FLITE_REG_CIIMGCPT_IMGCPTEN;
	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_CIIMGCPT));
}

static void flite_hw_set_capture_stop(u32 __iomem *base_reg)
{
	u32 cfg;

	cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIIMGCPT));
	cfg &= ~FLITE_REG_CIIMGCPT_IMGCPTEN;
	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_CIIMGCPT));
}

static int flite_hw_set_source_format(u32 __iomem *base_reg, struct fimc_is_image *image)
{
	int ret = 0;
	u32 pixelformat, format, cfg;

	FIMC_BUG(!image);

	pixelformat = image->format.pixelformat;
	cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));

	switch (pixelformat) {
	case V4L2_PIX_FMT_SBGGR8:
	case V4L2_PIX_FMT_SGBRG8:
	case V4L2_PIX_FMT_SGRBG8:
	case V4L2_PIX_FMT_SRGGB8:
		format = HW_FORMAT_RAW8;
		break;
	case V4L2_PIX_FMT_SBGGR10:
	case V4L2_PIX_FMT_SGBRG10:
	case V4L2_PIX_FMT_SGRBG10:
	case V4L2_PIX_FMT_SRGGB10:
		format = HW_FORMAT_RAW10;
		break;
	case V4L2_PIX_FMT_SBGGR12:
	case V4L2_PIX_FMT_SGBRG12:
	case V4L2_PIX_FMT_SGRBG12:
	case V4L2_PIX_FMT_SRGGB12:
	case V4L2_PIX_FMT_SBGGR16:
		format = HW_FORMAT_RAW10;
		/*
		 * HACK : hal send RAW10 for RAW12
		 * formt = HW_FORMAT_RAW12 << 24;
		 */
		break;
	case V4L2_PIX_FMT_YUYV:
		format = HW_FORMAT_YUV422_8BIT;
		break;
	case V4L2_PIX_FMT_JPEG:
		format = HW_FORMAT_USER;
		break;
	default:
		err("[BNS] unsupported format(%X)", pixelformat);
		format = HW_FORMAT_RAW10;
		ret = -EINVAL;
		break;
	}

#ifdef COLORBAR_MODE
	cfg |= (HW_FORMAT_YUV422_8BIT << 24);
#else
	cfg |= (format << 24);
#endif

	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));

	return ret;
}

static void flite_hw_set_dma_fmt(u32 __iomem *base_reg,
	u32 pixelformat)
{
	u32 cfg = 0;

	cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIODMAFMT));

	if (pixelformat == V4L2_PIX_FMT_SBGGR10 || pixelformat == V4L2_PIX_FMT_SBGGR12)
		cfg |= FLITE_REG_CIODMAFMT_PACK12;
	else
		cfg |= FLITE_REG_CIODMAFMT_NORMAL;

	if (pixelformat == V4L2_PIX_FMT_SGRBG8)
		cfg |= FLITE_REG_CIODMAFMT_1D_DMA;
	else
		cfg |= FLITE_REG_CIODMAFMT_2D_DMA;

	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_CIODMAFMT));
}

static void flite_hw_set_test_pattern_enable(u32 __iomem *base_reg)
{
	u32 cfg = 0;

	/* will use for pattern generation testing */
	cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));
	cfg |= FLITE_REG_CIGCTRL_TEST_PATTERN_COLORBAR;

	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));
}

static void flite_hw_force_reset(u32 __iomem *base_reg)
{
	u32 cfg = 0, retry = 100;
	cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));

	/* request sw reset */
	cfg |= FLITE_REG_CIGCTRL_SWRST_REQ;
	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));

	/* checking reset ready */
	cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));
	while (retry-- && !(cfg & FLITE_REG_CIGCTRL_SWRST_RDY))
		cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));

	if (!(cfg & FLITE_REG_CIGCTRL_SWRST_RDY))
		warn("[BNS] sw reset is not read but forcelly");

	/* sw reset */
	cfg |= FLITE_REG_CIGCTRL_SWRST;
	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));
}

static void flite_hw_set_camera_type(u32 __iomem *base_reg)
{
	u32 cfg = 0;
	cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));

	cfg |= FLITE_REG_CIGCTRL_SELCAM_MIPI;

	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));
}

static void flite_hw_set_window_offset(u32 __iomem *base_reg,
	struct fimc_is_image *image)
{
	u32 cfg = 0;
	u32 hoff2, voff2;

	FIMC_BUG(!image);

	cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIWDOFST));
	cfg &= ~(FLITE_REG_CIWDOFST_HOROFF_MASK |
		FLITE_REG_CIWDOFST_VEROFF_MASK);
	cfg |= FLITE_REG_CIWDOFST_WINOFSEN |
		FLITE_REG_CIWDOFST_WINHOROFST(image->window.offs_h) |
		FLITE_REG_CIWDOFST_WINVEROFST(image->window.offs_v);

	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_CIWDOFST));

	hoff2 = image->window.o_width - image->window.width - image->window.offs_h;
	voff2 = image->window.o_height - image->window.height - image->window.offs_v;
	cfg = FLITE_REG_CIWDOFST2_WINHOROFST2(hoff2) |
		FLITE_REG_CIWDOFST2_WINVEROFST2(voff2);

	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_CIWDOFST2));
}

static void flite_hw_set_last_capture_end_clear(u32 __iomem *base_reg)
{
	u32 cfg = 0;

	cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CISTATUS2));
	cfg &= ~FLITE_REG_CISTATUS2_LASTCAPEND;

	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_CISTATUS2));
}

static int flite_hw_get_status1(u32 __iomem *base_reg)
{
	u32 status = 0;

	status = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CISTATUS));

	return status;
}

static void flite_hw_set_status1(u32 __iomem *base_reg, u32 val)
{
	writel(val, base_reg + TO_WORD_OFFSET(FLITE_REG_CISTATUS));
}

static void flite_hw_set_start_addr(u32 __iomem *base_reg, u32 number, u32 addr)
{
	u32 __iomem *target_reg;

	if (number == 0) {
		target_reg = base_reg + TO_WORD_OFFSET(0x30);
	} else {
		number--;
		target_reg = base_reg + TO_WORD_OFFSET(0x200 + (0x4*number));
	}

	writel(addr, target_reg);
}

static void flite_hw_set_use_buffer(u32 __iomem *base_reg, u32 number)
{
	u32 buffer;
	buffer = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIFCNTSEQ));
	buffer |= 1<<number;
	writel(buffer, base_reg + TO_WORD_OFFSET(FLITE_REG_CIFCNTSEQ));
}

static void flite_hw_set_unuse_buffer(u32 __iomem *base_reg, u32 number)
{
	u32 buffer;
	buffer = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIFCNTSEQ));
	buffer &= ~(1<<number);
	writel(buffer, base_reg + TO_WORD_OFFSET(FLITE_REG_CIFCNTSEQ));
}

static int init_fimc_lite(u32 __iomem *base_reg)
{
	int i;

	writel(0, base_reg + TO_WORD_OFFSET(FLITE_REG_CIFCNTSEQ));

	for (i = 0; i < 32; i++)
		flite_hw_set_start_addr(base_reg , i, 0xffffffff);

	return 0;
}

static void flite_hw_set_through_put_sel(u32 __iomem *base_reg)
{
	u32 cfg = 0;

	cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));
	cfg |= (FLITE_REG_CIGCTRL_THROUGHPUT_SEL);
	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));
}

/*
 * ****************************
 *  FIMC-BNS HW API FUNCTIONS
 * ****************************
 */
void flite_hw_reset(u32 __iomem *base_reg)
{
	flite_hw_force_reset(base_reg);
	flite_hw_set_camera_type(base_reg);
	init_fimc_lite(base_reg);
}

void flite_hw_enable(u32 __iomem *base_reg)
{
	flite_hw_set_through_put_sel(base_reg);
	flite_hw_set_last_capture_end_clear(base_reg);
	flite_hw_set_capture_start(base_reg);
}

void flite_hw_disable(u32 __iomem *base_reg)
{
	flite_hw_set_capture_stop(base_reg);
}

void flite_hw_s_control(u32 __iomem *base_reg, u32 id, u32 value)
{
	switch(id) {
	case FLITE_CTRL_TEST_PATTERN:
		flite_hw_set_test_pattern_enable(base_reg);
		break;
	case FLITE_CTRL_LINE_RATIO:
		flite_hw_set_lineint_ratio(base_reg, value);
		break;
	default:
		err("[BNS] control id is invalid(%d)", id);
		break;
	}
}

int flite_hw_set_bns(u32 __iomem *base_reg, bool enable, struct fimc_is_image *image)
{
	int ret = 0;
	u32 width, height;
	u32 otf_width, otf_height;
	u32 factor_x, factor_y;
	u32 binning_ratio;

	FIMC_BUG(!image);

	width = image->window.width;
	height = image->window.height;
	otf_width = image->window.otf_width;
	otf_height = image->window.otf_height;

	if (otf_width == width && otf_height == height) {
		info("[BNS] input & output sizes are same(%d, %d)\n", otf_width, otf_height);
		goto exit;
	}

	if (otf_width == 0 || otf_height == 0) {
		warn("[BNS] size is zero. s_ctrl(V4L2_CID_IS_S_BNS) first");
		goto exit;
	}

	factor_x = 2 * width / otf_width;
	factor_y = 2 * height / otf_height;

	binning_ratio = BINNING(width, otf_width);
	switch (binning_ratio) {
	case 1250:
		factor_x = 0;
		factor_y = 0;
		break;
	case 1750:
		factor_x = 1;
		factor_y = 1;
		break;
	}

	flite_hw_s_size_bns(base_reg, width, height, otf_width, otf_height);

	flite_hw_s_coeff_bns(base_reg, factor_x, factor_y);

	flite_hw_enable_bns(base_reg, true);

	info("[BNS] in(%d, %d), out(%d, %d), ratio(%d, %d)\n",
		width, height, otf_width, otf_height, factor_x, factor_y);
exit:
	return ret;
}

void flite_hw_set_fmt_source(u32 __iomem *base_reg, struct fimc_is_image *image)
{
	/* source size */
	flite_hw_set_cam_source_size(base_reg, image);
	/* window offset */
	flite_hw_set_window_offset(base_reg, image);
	/* source format */
	flite_hw_set_source_format(base_reg, image);
}

void flite_hw_set_fmt_dma(u32 __iomem *base_reg, struct fimc_is_image *image)
{
	/* dma size */
	flite_hw_set_dma_offset(base_reg, image);
	/* dma format */
	flite_hw_set_dma_fmt(base_reg, image->format.pixelformat);
}

void flite_hw_set_output_dma(u32 __iomem *base_reg, bool enable)
{
	u32 cfg = 0;
	cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));

	if (enable)
		cfg &= ~FLITE_REG_CIGCTRL_ODMA_DISABLE;
	else
		cfg |= FLITE_REG_CIGCTRL_ODMA_DISABLE;

	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));
}

bool flite_hw_get_output_dma(u32 __iomem *base_reg)
{
	u32 cfg = 0;
	cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));
	return (cfg & FLITE_REG_CIGCTRL_ODMA_DISABLE) ? false : true;
}

void flite_hw_set_dma_addr(u32 __iomem *base_reg, u32 number, bool use, u32 addr)
{
	/* dma addr set */
	if (use) {
		flite_hw_set_start_addr(base_reg, number, addr);
		flite_hw_set_use_buffer(base_reg, number);
		flite_hw_set_output_dma(base_reg, true);
	} else {
	/* dma addr set */
		flite_hw_set_unuse_buffer(base_reg, number);
		flite_hw_set_start_addr(base_reg, number, 0);
		flite_hw_set_output_dma(base_reg, false);
	}
}

void flite_hw_set_output_otf(u32 __iomem *base_reg, bool enable)
{
	u32 cfg = 0;
	cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));

	if (enable)
		cfg &= ~FLITE_REG_CIGCTRL_OLOCAL_DISABLE;
	else
		cfg |= FLITE_REG_CIGCTRL_OLOCAL_DISABLE;

	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));
}

void flite_hw_set_interrupt_mask(u32 __iomem *base_reg, bool enable, u32 irq_ids)
{
	u32 cfg = 0;
	u32 i = 0;
	cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));

	for (i = 0; i < FLITE_MASK_IRQ_ALL; i++) {
		if (!((1 << i) & irq_ids))
			continue;

		switch (i) {
		case FLITE_MASK_IRQ_START:
			if (enable)
				cfg &= ~FLITE_REG_CIGCTRL_IRQ_STARTEN0_DISABLE;
			else
				cfg |= FLITE_REG_CIGCTRL_IRQ_STARTEN0_DISABLE;
			break;
		case FLITE_MASK_IRQ_END:
			if (enable)
				cfg &= ~FLITE_REG_CIGCTRL_IRQ_ENDEN0_DISABLE;
			else
				cfg |= FLITE_REG_CIGCTRL_IRQ_ENDEN0_DISABLE;
			break;
		case FLITE_MASK_IRQ_OVERFLOW:
			if (enable)
				cfg &= ~FLITE_REG_CIGCTRL_IRQ_OVFEN0_DISABLE;
			else
				cfg |= FLITE_REG_CIGCTRL_IRQ_OVFEN0_DISABLE;
			break;
		case FLITE_MASK_IRQ_LAST_CAPTURE:
			if (enable)
				cfg &= ~FLITE_REG_CIGCTRL_IRQ_LASTEN0_DISABLE;
			else
				cfg |= FLITE_REG_CIGCTRL_IRQ_LASTEN0_DISABLE;
			break;
		case FLITE_MASK_IRQ_LINE:
			if (enable)
				cfg &= ~FLITE_REG_CIGCTRL_IRQ_LINEEN0_DISABLE;
			else
				cfg |= FLITE_REG_CIGCTRL_IRQ_LINEEN0_DISABLE;
			break;
		}
	}

	writel(cfg, base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));
}

u32 flite_hw_get_interrupt_mask(u32 __iomem *base_reg, u32 irq_ids)
{
	u32 cfg = 0;
	u32 ret = 0;
	u32 i = 0;
	cfg = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL));

	for (i = 0; i < FLITE_MASK_IRQ_ALL; i++) {
		if (!((1 << i) & irq_ids))
			continue;

		switch (i) {
		case FLITE_MASK_IRQ_START:
			if (cfg & FLITE_REG_CIGCTRL_IRQ_STARTEN0_DISABLE)
				ret |= (1 << i);
			break;
		case FLITE_MASK_IRQ_END:
			if (cfg & FLITE_REG_CIGCTRL_IRQ_ENDEN0_DISABLE)
				ret |= (1 << i);
			break;
		case FLITE_MASK_IRQ_OVERFLOW:
			if (cfg & FLITE_REG_CIGCTRL_IRQ_OVFEN0_DISABLE)
				ret |= (1 << i);
			break;
		case FLITE_MASK_IRQ_LAST_CAPTURE:
			if (cfg & FLITE_REG_CIGCTRL_IRQ_LASTEN0_DISABLE)
				ret |= (1 << i);
			break;
		case FLITE_MASK_IRQ_LINE:
			if (cfg & FLITE_REG_CIGCTRL_IRQ_LINEEN0_DISABLE)
				ret |= (1 << i);
			break;
		}
	}

	return ret;
}


u32 flite_hw_get_status(u32 __iomem *base_reg, u32 status_ids, bool clear)
{
	u32 i = 0;
	u32 ret = 0;
	u32 status1;
	u32 clear_status1 = 0;
	u32 ciwdofst;

	status1 = flite_hw_get_status1(base_reg);

	for (i = 0; i < FLITE_STATUS_ALL; i++) {
		if (!((1 << i) & status_ids))
			continue;

		switch (i) {
		case FLITE_STATUS_IRQ_SRC_START:
			if (status1 & (1 << 5)) {
				ret |= (1 << i);
				clear_status1 |= (1 << 5);
			}
			break;
		case FLITE_STATUS_IRQ_SRC_END:
			if (status1 & (1 << 4)) {
				ret |= (1 << i);
				clear_status1 |= (1 << 4);
			}
			break;
		case FLITE_STATUS_IRQ_SRC_OVERFLOW:
			if (status1 & (1 << 7)) {
				ret |= (1 << i);
				clear_status1 |= (1 << 7);
			}
			break;
		case FLITE_STATUS_IRQ_SRC_LAST_CAPTURE:
			if (status1 & (1 << 6)) {
				ret |= (1 << i);
				clear_status1 |= (1 << 6);
			}
			break;
		case FLITE_STATUS_IRQ_SRC_LINE:
			if (status1 & (1 << 8)) {
				ret |= (1 << i);
				clear_status1 |= (1 << 8);
			}
			break;
		case FLITE_STATUS_OFY:
			if (status1 & (1 << 10)) {
				ret |= (1 << i);
				if (clear) {
					ciwdofst = readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIWDOFST));
					ciwdofst  |= (FLITE_REG_CIWDOFST_CLROVIY);
					writel(ciwdofst, base_reg + TO_WORD_OFFSET(FLITE_REG_CIWDOFST));
					ciwdofst  &= ~(FLITE_REG_CIWDOFST_CLROVIY);
					writel(ciwdofst, base_reg + TO_WORD_OFFSET(FLITE_REG_CIWDOFST));
				}
			}
			break;
		case FLITE_STATUS_MIPI_VALID:
			if (status1 & (7 << 20))
				ret |= (1 << i);
			break;
		default:
			break;
		}
	}

	if (clear && (clear_status1 > 0))
		flite_hw_set_status1(base_reg, clear_status1);

	return ret;
}

void flite_hw_set_path(u32 __iomem *base_reg, int instance,
		int csi_ch, int flite_ch, int taa_ch, int isp_ch)
{
	/* GENERAL reg was moved to MCUCTL */
	return;
}

void flite_hw_dump(u32 __iomem *base_reg)
{
	info("BNS 4.0 DUMP\n");
	info("[0x%04X] : 0x%08X\n", FLITE_REG_CISRCSIZE, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CISRCSIZE)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_CIGCTRL, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIGCTRL)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_CIIMGCPT, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIIMGCPT)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_CICPTSEQ, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CICPTSEQ)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_CIWDOFST, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIWDOFST)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_CIWDOFST2, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIWDOFST2)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_CIODMAFMT, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIODMAFMT)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_CIOCAN, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIOCAN)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_CIOOFF, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIOOFF)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_CIOSA, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIOSA)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_CISTATUS, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CISTATUS)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_CISTATUS2, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CISTATUS2)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_CISTATUS3, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CISTATUS3)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_LINEINT, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_LINEINT)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_CITHOLD, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CITHOLD)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_CIFCNTSEQ, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_CIFCNTSEQ)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_BINNINGON, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_BINNINGON)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_BINNINGCTRL, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_BINNINGCTRL)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_PEDESTAL, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_PEDESTAL)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_BINNINGTOTAL, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_BINNINGTOTAL)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_BINNINGINPUT, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_BINNINGINPUT)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_BINNINGMARGIN, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_BINNINGMARGIN)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_BINNINGOUTPUT, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_BINNINGOUTPUT)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_WEIGHTX01, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_WEIGHTX01)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_WEIGHTX23, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_WEIGHTX23)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_WEIGHTY01, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_WEIGHTY01)));
	info("[0x%04X] : 0x%08X\n", FLITE_REG_WEIGHTY23, readl(base_reg + TO_WORD_OFFSET(FLITE_REG_WEIGHTY23)));
}


