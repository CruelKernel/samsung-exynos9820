/* linux/drivers/media/platform/exynos/jsqz-regs.h
 *
 * Register definition file for Samsung JPEG Squeezer driver
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Jungik Seo <jungik.seo@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef JSQZ_REGS_H_
#define JSQZ_REGS_H_
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/module.h>
#include "jsqz-core.h"

// SFR
#define     REG_0_Y_ADDR			0x000
#define     REG_1_U_ADDR			0x004
#define     REG_2_V_ADDR			0x008
#define     REG_3_INPUT_FORMAT		0x00C

#define     REG_4_TO_19_Y_Q_MAT		0x010
#define     REG_20_TO_35_C_Q_MAT	0x050

#define     REG_36_SW_RESET			0x090
#define     REG_37_INTERRUPT_EN		0x094
#define     REG_38_INTERRUPT_CLEAR	0x098
#define     REG_39_JSQZ_HW_START	0x09C
#define     REG_40_JSQZ_HW_DONE		0x100
#define     REG_42_BUS_CONFIG		0x108

#define     REG_43_TO_58_INIT_Y_Q	0x10C
#define     REG_59_TO_74_INIT_C_Q	0x14C


static inline void jsqz_sw_reset(void __iomem *base)
{
	writel(0x0, base + REG_36_SW_RESET);
	writel(0x1, base + REG_36_SW_RESET);
}

static inline void jsqz_interrupt_enable(void __iomem *base)
{
	writel(0x1, base + REG_37_INTERRUPT_EN);
	writel(0x0, base + REG_38_INTERRUPT_CLEAR);
}

static inline void jsqz_interrupt_disable(void __iomem *base)
{
	writel(0x0, base + REG_37_INTERRUPT_EN);
}

static inline void jsqz_interrupt_clear(void __iomem *base)
{
	writel(0x1, base + REG_38_INTERRUPT_CLEAR);
}

static inline u32 jsqz_get_interrupt_status(void __iomem *base)
{
	return readl(base + REG_38_INTERRUPT_CLEAR);
}

static inline void jsqz_hw_start(void __iomem *base)
{
	writel(0x1, base + REG_39_JSQZ_HW_START);
}

static inline u32 jsqz_check_done(void __iomem *base)
{
	return readl(base + REG_40_JSQZ_HW_DONE) & 0x1;
}
static inline void jsqz_on_off_time_out(void __iomem *base, u32 value)
{
	u32 sfr = 0;
	sfr = readl(base + REG_42_BUS_CONFIG);
    if (value == 0)
        writel((sfr & 0xfeffffff), base + REG_42_BUS_CONFIG);
    else
        writel((sfr | 0x01000000), base + REG_42_BUS_CONFIG);
}

static inline void jsqz_set_stride_on_n_value(void __iomem *base, u32 value)
{
	u32 sfr = 0;
	sfr = readl(base + REG_42_BUS_CONFIG);
    if (value == 0)
        writel((sfr & 0xfffee000), base + REG_42_BUS_CONFIG);
    else
        writel(((sfr & 0xfffe0000) | (0x00010000 | (value & 0x1fff))), base + REG_42_BUS_CONFIG);
}

static inline void jsqz_set_input_format(void __iomem *base, u32 format)
{
    writel(format, base + REG_3_INPUT_FORMAT);
}

static inline void jsqz_set_input_addr_luma(void __iomem *base, dma_addr_t y_addr)
{
    writel(y_addr, base + REG_0_Y_ADDR);
}

static inline void jsqz_set_input_addr_chroma(void __iomem *base, dma_addr_t u_addr, dma_addr_t v_addr)
{
    writel(u_addr, base + REG_1_U_ADDR);
    writel(v_addr, base + REG_2_V_ADDR);
}

static inline void jsqz_set_input_qtbl(void __iomem *base, u32 * input_qt)
{
	int i;
    for (i = 0; i < 16; i++)
    {
        writel(input_qt[i], base + REG_43_TO_58_INIT_Y_Q + (i * 0x4));
        writel(input_qt[i+16], base + REG_59_TO_74_INIT_C_Q + (i * 0x4));
    }
}

static inline u32 jsqz_get_bug_config(void __iomem *base)
{
	return readl(base + REG_42_BUS_CONFIG);
}

static inline void jsqz_get_init_qtbl(void __iomem *base, u32 * init_qt)
{
	int i;
    for (i = 0; i < 16; i++)
    {
        init_qt[i] = readl(base + REG_43_TO_58_INIT_Y_Q + (i * 0x4));
        init_qt[i+16] = readl(base + REG_59_TO_74_INIT_C_Q + (i * 0x4));
    }
}

static inline void jsqz_get_output_regs(void __iomem *base, u32 * output_qt)
{
	int i;
    for (i = 0; i < 16; i++)
    {
        output_qt[i] = readl(base + REG_4_TO_19_Y_Q_MAT + (i * 0x4));
        output_qt[i+16] = readl(base + REG_20_TO_35_C_Q_MAT + (i * 0x4));
    }
}

static inline void jsqz_print_all_regs(struct jsqz_dev *jsqz)
{
	int i;
	void __iomem *base = jsqz->regs;
	dev_info(jsqz->dev, "%s: BEGIN\n", __func__);
	for (i = 0; i < 4; i++)
	{
		dev_info(jsqz->dev, "%s: 0x%08x : %08x\n", __func__, (i*0x4), readl(base + (i*0x4)));
	}
	for (i = 0; i < 4; i++)
	{
		dev_info(jsqz->dev, "%s: 0x%08x : %08x\n", __func__,
			(REG_36_SW_RESET + (i*0x4)), readl(base + REG_36_SW_RESET + (i*0x4)));
	}
	dev_info(jsqz->dev, "%s: 0x00000100 : %08x\n", __func__, readl(base + REG_40_JSQZ_HW_DONE));
	dev_info(jsqz->dev, "%s: 0x00000104 : %08x\n", __func__, readl(base + 0x104));
	dev_info(jsqz->dev, "%s: 0x00000108 : %08x\n", __func__, readl(base + REG_42_BUS_CONFIG));
	dev_info(jsqz->dev, "%s: END\n", __func__);
}



/*
static inline int get_hw_enc_status(void __iomem *base)
{
	unsigned int status = 0;

	status = readl(base + JSQZ_ENC_STAT_REG) & (KBit0 | KBit1);
	return (status != 0 ? -1:0);
}
*/


#endif /* JSQZ_REGS_H_ */

