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

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include "fimc-is-config.h"
#include "fimc-is-type.h"
#include "fimc-is-regs.h"
#include "fimc-is-hw.h"

#define CSI_REG_CTRL				(0x04)
#define CSI_REG_CLKCTRL				(0x08)
#define CSI_REG_DPHYCTRL			(0x24)
#define CSI_REG_INTMSK				(0x10)
#define CSI_REG_INTSRC				(0x14)
#define CSI_REG_CONFIG0				(0x40)
#define CSI_REG_CONFIG1				(0x50)
#define CSI_REG_CONFIG2				(0x60)
#define CSI_REG_RESOL0				(0x44)
#define CSI_REG_RESOL1				(0x54)
#define CSI_REG_RESOL2				(0x64)
#define CSI_REG_DPHYCTRL0_H			(0x30)
#define CSI_REG_DPHYCTRL0_L			(0x34)
#define CSI_REG_DPHYCTRL1_H			(0x38)
#define CSI_REG_DPHYCTRL1_L			(0x3c)

int csi_hw_reset(u32 __iomem *base_reg)
{
	int ret = 0;
	u32 retry = 10;
	u32 val;

	val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_CTRL));
	writel(val | (1 << 1), base_reg + TO_WORD_OFFSET(CSI_REG_CTRL));

	while (--retry) {
		udelay(10);
		val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_CTRL));
		if (!(val & (1 << 1)))
			break;
	}

	if (!retry) {
		err("reset is fail(%d)", retry);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

int csi_hw_s_settle(u32 __iomem *base_reg,
	u32 settle)
{
	u32 val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL));
	val = (val & ~(0xFF << 24)) | (settle << 24);
	writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL));

	return 0;
}

int csi_hw_s_dphyctrl0(u32 __iomem *base_reg,
	u64 ctrl)
{
	u32 val;

	val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL0_L));
	val = (val & ~(0xFFFFFFFF << 0)) | (ctrl << 0);
	writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL0_L));

	val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL0_H));
	val = (val & ~(0xFFFFFFFF << 0)) | (ctrl >>16);
	writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL0_H));

	return 0;
}

int csi_hw_s_dphyctrl1(u32 __iomem *base_reg,
	u64 ctrl)
{
	u32 val;

	val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL1_L));
	val = (val & ~(0xFFFFFFFF << 0)) | (ctrl << 0);
	writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL1_L));

	val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL1_H));
	val = (val & ~(0xFFFFFFFF << 0)) | (ctrl >>16);
	writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL1_H));


	return 0;
}

int csi_hw_s_lane(u32 __iomem *base_reg, struct fimc_is_image *image, u32 lanes, u32 mipi_speed)
{
	int ret = 0;
	u32 deskew = 0;
	u64 lane_speed;
	u32 val;
	u32 lane;

	lane_speed = (((image->window.width * image->window.height * image->framerate) / 1000000) *
		image->format.bitwidth * 115) / (100 * (lanes+ 1));
	if (lane_speed > 1500) {
		info("[CSI] lane speed : %lld(%dx%d@%d, %d bit, %d lanes, deskew enable)\n",
			lane_speed, image->window.width, image->window.height, image->framerate,
			image->format.bitwidth, lanes);
		deskew = 1;
	}

	val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_CTRL));
	val = (val & ~(0x3 << 8)) | (lanes << 8);
	val = (val & ~(0x1 << 12)) | (deskew << 12);
	writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_CTRL));

	val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_CLKCTRL));
	/* all channel use extclk for wrapper clock source */
	val |= (0xF << 0);
	/* dynamic clock gating off */
	val &= ~(0xF << 4);
	writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_CLKCTRL));

	switch (lanes) {
	case CSI_DATA_LANES_1:
		/* lane 0 */
		lane = (0x1);
		break;
	case CSI_DATA_LANES_2:
		/* lane 0 + lane 1 */
		lane = (0x3);
		break;
	case CSI_DATA_LANES_3:
		/* lane 0 + lane 1 + lane 2 */
		lane = (0x7);
		break;
	case CSI_DATA_LANES_4:
		/* lane 0 + lane 1 + lane 2 + lane 3 */
		lane = (0xF);
		break;
	default:
		err("lanes is invalid(%d)", lanes);
		lane = (0xF);
		break;
	}

	/* lane set to DPHY */
	val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL));
	val |= (lane << 1);
	writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL));

	return ret;
}

int csi_hw_s_control(u32 __iomem *base_reg, u32 id, u32 value)
{
	int ret = 0;
	u32 val;

	switch(id) {
	case CSIS_CTRL_INTERLEAVE_MODE:
		/* interleave mode */
		val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_CTRL));
		val = (val & ~(0x3 << 10)) | (value << 10);
		writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_CTRL));
		break;
	default:
		err("control id is invalid(%d)", id);
		break;
	}

	return ret;
}

int csi_hw_s_config(u32 __iomem *base_reg,
	u32 channel, struct fimc_is_vci_config *config, u32 width, u32 height)
{
	int ret = 0;
	u32 val, parallel;

	if ((config->hwformat == HW_FORMAT_YUV420_8BIT) ||
		(config->hwformat == HW_FORMAT_YUV422_8BIT))
		parallel = 1;
	else
		parallel = 0;

	switch (channel) {
	case CSI_VIRTUAL_CH_0:
		val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_CONFIG0));
		val = (val & ~(0x3 << 0)) | (config->map << 0);
		val = (val & ~(0x3f << 2)) | (config->hwformat << 2);
		val = (val & ~(0x1 << 11)) | (parallel << 11);
		writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_CONFIG0));

		val = (width << 0) | (height << 16);
		writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_RESOL0));
		break;
	case CSI_VIRTUAL_CH_1:
		val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_CONFIG1));
		val = (val & ~(0x3 << 0)) | (config->map << 0);
		val = (val & ~(0x3f << 2)) | (config->hwformat << 2);
		val = (val & ~(0x1 << 11)) | (parallel << 11);
		writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_CONFIG1));

		val = (width << 0) | (height << 16);
		writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_RESOL1));
		break;
	case CSI_VIRTUAL_CH_2:
		val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_CONFIG2));
		val = (val & ~(0x3 << 0)) | (config->map << 0);
		val = (val & ~(0x3f << 2)) | (config->hwformat << 2);
		val = (val & ~(0x1 << 11)) | (parallel << 11);
		writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_CONFIG2));

		val = (width << 0) | (height << 16);
		writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_RESOL2));
		break;
	case CSI_VIRTUAL_CH_3:
		break;
	default:
		err("invalid channel(%d)", channel);
		ret = -EINVAL;
		goto p_err;
		break;
	}

p_err:
	return ret;
}

int csi_hw_s_irq_msk(u32 __iomem *base_reg, bool on)
{
	u32 val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_INTMSK));
	val = on ? 0x0FFFFFFF : 0; /* all ineterrupt enable or disable */
	writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_INTMSK));

	return 0;
}

int csi_hw_g_irq_src(u32 __iomem *base_reg, struct csis_irq_src *src, bool clear)
{
	u32 val;

	val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_INTSRC));
	writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_INTSRC));

	return val;
}

int csi_hw_enable(u32 __iomem *base_reg)
{
	u32 val;

	/* update shadow */
	val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_CTRL));
	val |= (0xF << 16);
	writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_CTRL));

	/* DPHY on */
	val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL));
	val |= (1 << 0);
	writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL));

	/* csi enable */
	val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_CTRL));
	val |= (0x1 << 0);
	writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_CTRL));

	return 0;
}

int csi_hw_disable(u32 __iomem *base_reg)
{
	u32 val;

	/* DPHY off */
	val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL));
	val &= ~(0x1f << 0);
	writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL));

	/* csi disable */
	val = readl(base_reg + TO_WORD_OFFSET(CSI_REG_CTRL));
	val &= ~(1 << 0);
	writel(val, base_reg + TO_WORD_OFFSET(CSI_REG_CTRL));

	return 0;
}

int csi_hw_dump(u32 __iomem *base_reg)
{
	info("CSI 3.4 DUMP\n");
	info("[0x%04X] : 0x%08X\n", CSI_REG_CTRL, readl(base_reg + TO_WORD_OFFSET(CSI_REG_CTRL)));
	info("[0x%04X] : 0x%08X\n", CSI_REG_CLKCTRL, readl(base_reg + TO_WORD_OFFSET(CSI_REG_CLKCTRL)));
	info("[0x%04X] : 0x%08X\n", CSI_REG_DPHYCTRL, readl(base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL)));
	info("[0x%04X] : 0x%08X\n", CSI_REG_INTMSK, readl(base_reg + TO_WORD_OFFSET(CSI_REG_INTMSK)));
	info("[0x%04X] : 0x%08X\n", CSI_REG_INTSRC, readl(base_reg + TO_WORD_OFFSET(CSI_REG_INTSRC)));
	info("[0x%04X] : 0x%08X\n", CSI_REG_CONFIG0, readl(base_reg + TO_WORD_OFFSET(CSI_REG_CONFIG0)));
	info("[0x%04X] : 0x%08X\n", CSI_REG_CONFIG1, readl(base_reg + TO_WORD_OFFSET(CSI_REG_CONFIG1)));
	info("[0x%04X] : 0x%08X\n", CSI_REG_CONFIG2, readl(base_reg + TO_WORD_OFFSET(CSI_REG_CONFIG2)));
	info("[0x%04X] : 0x%08X\n", CSI_REG_RESOL0, readl(base_reg + TO_WORD_OFFSET(CSI_REG_RESOL0)));
	info("[0x%04X] : 0x%08X\n", CSI_REG_RESOL1, readl(base_reg + TO_WORD_OFFSET(CSI_REG_RESOL1)));
	info("[0x%04X] : 0x%08X\n", CSI_REG_RESOL2, readl(base_reg + TO_WORD_OFFSET(CSI_REG_RESOL2)));
	info("[0x%04X] : 0x%08X\n", CSI_REG_DPHYCTRL0_H, readl(base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL0_H)));
	info("[0x%04X] : 0x%08X\n", CSI_REG_DPHYCTRL0_L, readl(base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL0_L)));
	info("[0x%04X] : 0x%08X\n", CSI_REG_DPHYCTRL1_H, readl(base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL1_H)));
	info("[0x%04X] : 0x%08X\n", CSI_REG_DPHYCTRL1_L, readl(base_reg + TO_WORD_OFFSET(CSI_REG_DPHYCTRL1_L)));

	return 0;
}
