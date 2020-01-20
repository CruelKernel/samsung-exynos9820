/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include "fimc-is-hw-virtual-otf.h"

void fimc_is_hw_c2sync_s1_clk_enable(void __iomem *base_addr, bool enable)
{
	fimc_is_hw_set_reg(base_addr, &c2sync_s1_regs[C2SYNC_M0_S1_R_C2COM_RING_CLK_EN], enable);
}

void fimc_is_hw_c2sync_s2_clk_enable(void __iomem *base_addr, bool enable)
{
	fimc_is_hw_set_reg(base_addr, &c2sync_s2_regs[C2SYNC_M0_S2_R_C2COM_RING_CLK_EN], enable);
}

void fimc_is_hw_virtual_otf_clk_enable(bool enable)
{
	void __iomem *s1_regs;
	void __iomem *s2_regs;

	s1_regs = ioremap_nocache(C2SYNC_1SLV_BASE_ADDR, 0x100);
	s2_regs = ioremap_nocache(C2SYNC_2SLV_BASE_ADDR, 0x100);

	fimc_is_hw_c2sync_s1_clk_enable(s1_regs, enable);
	fimc_is_hw_c2sync_s2_clk_enable(s2_regs, enable);

	iounmap(s1_regs);
	iounmap(s2_regs);
}

void fimc_is_hw_virtual_otf_sw_reset(void __iomem *base_addr, bool enable)
{

}

void fimc_is_hw_c2sync_s1_cfg(bool trs_en, u32 token_line, u32 height)
{
	void __iomem *s1_regs;
	u32 token_limit = 0;

	s1_regs = ioremap_nocache(C2SYNC_1SLV_BASE_ADDR, 0x100);

	fimc_is_hw_set_reg(s1_regs, &c2sync_s1_regs[C2SYNC_M0_S1_R_SELREGISTER], 1);
	fimc_is_hw_set_reg(s1_regs, &c2sync_s1_regs[C2SYNC_M0_S1_R_SELREGISTERMODE], 1);

	token_limit = (height / token_line) / 2;
	fimc_is_hw_set_reg(s1_regs, &c2sync_s1_regs[C2SYNC_M0_S1_R_TRS_LIMIT0], token_limit);
	fimc_is_hw_set_reg(s1_regs, &c2sync_s1_regs[C2SYNC_M0_S1_R_LINES_COUNT0], height);
	fimc_is_hw_set_reg(s1_regs, &c2sync_s1_regs[C2SYNC_M0_S1_R_TRS_LINES_IN_TOKEN0], token_line);
	fimc_is_hw_set_reg(s1_regs, &c2sync_s1_regs[C2SYNC_M0_S1_R_TRS_LINES_IN_FIRST_TOKEN0], token_line);

	fimc_is_hw_set_reg(s1_regs, &c2sync_s1_regs[C2SYNC_M0_S1_R_TRS_ENABLE0], trs_en);

	iounmap(s1_regs);
}

void fimc_is_hw_c2sync_s2_cfg(bool trs_en_0, u32 token_line_0, u32 height_0,
	bool trs_en_1, u32 token_line_1, u32 height_1)
{
	void __iomem *s2_regs;
	u32 token_limit = 0;

	s2_regs = ioremap_nocache(C2SYNC_2SLV_BASE_ADDR, 0x100);

	fimc_is_hw_set_reg(s2_regs, &c2sync_s2_regs[C2SYNC_M0_S2_R_SELREGISTER], 1);
	fimc_is_hw_set_reg(s2_regs, &c2sync_s2_regs[C2SYNC_M0_S2_R_SELREGISTERMODE], 1);

	token_limit = (height_0 / token_line_0) / 2;
	fimc_is_hw_set_reg(s2_regs, &c2sync_s2_regs[C2SYNC_M0_S2_R_TRS_LIMIT0], token_limit);
	fimc_is_hw_set_reg(s2_regs, &c2sync_s2_regs[C2SYNC_M0_S2_R_LINES_COUNT0], height_0);
	fimc_is_hw_set_reg(s2_regs, &c2sync_s2_regs[C2SYNC_M0_S2_R_TRS_LINES_IN_TOKEN0], token_line_0);
	fimc_is_hw_set_reg(s2_regs, &c2sync_s2_regs[C2SYNC_M0_S2_R_TRS_LINES_IN_FIRST_TOKEN0], token_line_0);

	token_limit = (height_1 / token_line_1) / 2;
	fimc_is_hw_set_reg(s2_regs, &c2sync_s2_regs[C2SYNC_M0_S2_R_TRS_LIMIT1], token_limit);
	fimc_is_hw_set_reg(s2_regs, &c2sync_s2_regs[C2SYNC_M0_S2_R_LINES_COUNT1], height_1);
	fimc_is_hw_set_reg(s2_regs, &c2sync_s2_regs[C2SYNC_M0_S2_R_TRS_LINES_IN_TOKEN1], token_line_1);
	fimc_is_hw_set_reg(s2_regs, &c2sync_s2_regs[C2SYNC_M0_S2_R_TRS_LINES_IN_FIRST_TOKEN1], token_line_1);

	fimc_is_hw_set_reg(s2_regs, &c2sync_s2_regs[C2SYNC_M0_S2_R_TRS_ENABLE0], trs_en_0);
	fimc_is_hw_set_reg(s2_regs, &c2sync_s2_regs[C2SYNC_M0_S2_R_TRS_ENABLE1], trs_en_1);

	iounmap(s2_regs);
}
