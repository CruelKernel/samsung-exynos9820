/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * Author: Sung-Hyun Na <sunghyun.na@samsung.com>
 *
 * Chip Abstraction Layer for USB PHY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _PHY_EXYNOS_USBDP_R_CMN_H_
#define _PHY_EXYNOS_USBDP_R_CMN_H_

#define EXYNOS_USBDP_COMBO_CMN_R00	(0x0000)
#define USBDP_CMN00_SLAVE_ADDR_MASK		USBDP_COMBO_R_MSK(0, 8)
#define USBDP_CMN00_SLAVE_ADDR_CLR		USBDP_COMBO_R_CLR(0, 8)
#define USBDP_CMN00_SLAVE_ADDR_SET(_x)		USBDP_COMBO_R_SET(_x, 0, 8)
#define USBDP_CMN00_SLAVE_ADDR_GET(_R)		USBDP_COMBO_R_GET(_R, 0, 8)

#define EXYNOS_USBDP_COM_CMN_R01	(0x0004)
#define USBDP_CMN01_BIAS_REXT_EN		USBDP_COMBO_R_MSK(7, 1)
#define USBDP_CMN01_BIAS_CLK_SEL_MASK		USBDP_COMBO_R_MSK(5, 2)
#define USBDP_CMN01_BIAS_CLK_SEL_SET(_x)	USBDP_COMBO_R_SET(_x, 5, 2)
#define USBDP_CMN01_BIAS_CLK_SEL_GET(_r)	USBDP_COMBO_R_GET(_r, 5, 2)
#define USBDP_CMN01_BIAS_LADDER_SEL_MASK	USBDP_COMBO_R_MSK(2, 3)
#define USBDP_CMN01_BIAS_LADDER_SEL_SET(_x)	USBDP_COMBO_R_SET(_x, 2, 3)
#define USBDP_CMN01_BIAS_LADDER_SEL_GET(_r)	USBDP_COMBO_R_GET(_x, 2, 3)
#define USBDP_CMN01_BIAS_LADDER_EN		USBDP_COMBO_R_MSK(1, 1)
#define USBDP_CMN01_BIAS_CLK_EN			USBDP_COMBO_R_MSK(0, 1)

#define EXYNOS_USBDP_COM_CMN_R02	(0x0008)
#define USBDP_CMN02_MAN_BYPASS_LPF_EN		(1 << 7)
#define USBDP_CMN02_DIV_SEL_MASK		(0x3 << 5)
#define USBDP_CMN02_DIV_SEL_SET(_x)		((_x & 0x3) << 5)
#define USBDP_CMN02_DIV_SEL_GET(_r)		((_r & 0x3) >> 5)
#define USBDP_CMN02_BIAS_RES_CTRL_MASK		(0x7 << 2)
#define USBDP_CMN02_BIAS_RES_CTRL_SET(_x)	((_x & 0x7) << 2)
#define USBDP_CMN02_BIAS_RES_CTRL_GET(_r)	((_r & 0x7) >> 2)
#define USBDP_CMN02_BIAS_VBG_SEL_MASK		(0x3 << 0)
#define USBDP_CMN02_BIAS_VBG_SEL_SET(_x)	((_x & 0x3) << 0)
#define USBDP_CMN02_BIAS_VBG_SEL_GET(_r)	((_r & 0x3) >> 0)

#define EXYNOS_USBDP_COM_CMN_R03	(0x000C)
#define USBDP_CMN03_REFCLK_OUT_SOC_EN		(1 << 7)
#define USBDP_CMN03_CD_SW_CLKQ_VDD		(1 << 6)
#define USBDP_CMN03_CD_SW_CLKQ_PN		(1 << 5)
#define USBDP_CMN03_CD_SW_CLKI_PN		(1 << 4)
#define USBDP_CMN03_CD_SW_CLKI_CLKQ		(1 << 3)
#define USBDP_CMN03_CD_EN_PWM_CLK		(1 << 2)
#define USBDP_CMN03_CD_EN_CLKQ			(1 << 1)
#define USBDP_CMN03_MAN_BIAS_BYPASS_LPF		(1 << 0)

#define EXYNOS_USBDP_COM_CMN_R04	(0x0010)
#define USBDP_CMN04_CD_HSCLK_DIV1_PSTR_MASK	(0xf << 4)
#define USBDP_CMN04_CD_HSCLK_DIV1_PSTR_SET(_x)	((_x & 0xf) << 4)
#define USBDP_CMN04_CD_HSCLK_DIV1_PSTR_GET(_r)	((_r & 0xf) >> 4)
#define USBDP_CMN04_CD_HSCLK_DIV1_NSTR_MASK	(0xf << 0)
#define USBDP_CMN04_CD_HSCLK_DIV1_NSTR_SET(_x)	((_x & 0xf) << 0)
#define USBDP_CMN04_CD_HSCLK_DIV1_NSTR_GET(_r)	((_r & 0xf) >> 0)

#define EXYNOS_USBDP_COM_CMN_R05	(0x0014)
#define USBDP_CMN05_CD_HSCLK_DIV5_PSTR_MASK	(0xf << 4)
#define USBDP_CMN05_CD_HSCLK_DIV5_PSTR_SET(_x)	((_x & 0xf) << 4)
#define USBDP_CMN05_CD_HSCLK_DIV5_PSTR_GET(_r)	((_r & 0xf) >> 4)
#define USBDP_CMN05_CD_HSCLK_DIV5_NSTR_MASK	(0xf << 0)
#define USBDP_CMN05_CD_HSCLK_DIV5_NSTR_SET(_x)	((_x & 0xf) << 0)
#define USBDP_CMN05_CD_HSCLK_DIV5_NSTR_GET(_r)	((_r & 0xf) >> 0)

#define EXYNOS_USBDP_COM_CMN_R06	(0x0018)
#define USBDP_CMN06_CD_HSCLK_DIV10_PSTR_MASK	(0xf << 4)
#define USBDP_CMN06_CD_HSCLK_DIV10_PSTR_SET(_x)	((_x & 0xf) << 4)
#define USBDP_CMN06_CD_HSCLK_DIV10_PSTR_GET(_r)	((_r & 0xf) >> 4)
#define USBDP_CMN06_CD_HSCLK_DIV10_NSTR_MASK	(0xf << 0)
#define USBDP_CMN06_CD_HSCLK_DIV10_NSTR_SET(_x)	((_x & 0xf) << 0)
#define USBDP_CMN06_CD_HSCLK_DIV10_NSTR_GET(_r)	((_r & 0xf) >> 0)

#define EXYNOS_USBDP_COM_CMN_R07	(0x001C)
#define USBDP_CMN07_REFCLK_HYS_100M_IN_MASK	(0x3 << 6)
#define USBDP_CMN07_REFCLK_HYS_100M_IN_SET(_x)	((_x & 0x3) << 6)
#define USBDP_CMN07_REFCLK_HYS_100M_IN_GET(_r)	((_r & 0x3) >> 6)
#define USBDP_CMN07_MAN_PLL_REF_CLK_SEL_MASK	(0x3 << 4)
#define USBDP_CMN07_MAN_PLL_REF_CLK_SEL_SET(_x)	((_x & 0x3) << 4)
#define USBDP_CMN07_MAN_PLL_REF_CLK_SEL_GET(_r)	((_r & 0x3) >> 4)
#define USBDP_CMN07_MAN_REFCLK_OUT_SOURCE	(1 << 3)
#define USBDP_CMN07_MAN_REFCLK_OUT_EN		(1 << 2)
#define USBDP_CMN07_MAN_REFCLK_IN_EN		(1 << 1)
#define USBDP_CMN07_REFCLK_CTRL_EN		(1 << 0)

#define EXYNOS_USBDP_COM_CMN_R08	(0x0020)
#define USBDP_CMN08_REFCLK_OUT_STR_MASK		(0x7 << 5)
#define USBDP_CMN08_REFCLK_OUT_STR_SET(_x)	((_x & 0x7) << 5)
#define USBDP_CMN08_REFCLK_OUT_STR_GET(_r)	((_r & 0x7) >> 5)
#define USBDP_CMN08_REFCLK_LOCKDONE_SEL		(1 << 4)
#define USBDP_CMN08_MAN_PLL_REF_DIV_MASK	(0xf << 0)
#define USBDP_CMN08_MAN_PLL_REF_DIV_SET(_x)	((_x & 0xf) << 0)
#define USBDP_CMN08_MAN_PLL_REF_DIV_GET(_r)	((_r & 0xf) >> 0)

#define EXYNOS_USBDP_COM_CMN_R09	(0x0024)
#define USBDP_CMN09_REFCLK_TUNECODE_100M_IN_MASK	(0xf << 0)
#define USBDP_CMN09_REFCLK_TUNECODE_100M_IN_SET(_x)	((_x & 0xf) << 0)
#define USBDP_CMN09_REFCLK_TUNECODE_100M_IN_GET(_r)	((_r & 0xf) >> 0)
#define USBDP_CMN09_REFCLK_VCM_PN_MASK		(0xf << 0)
#define USBDP_CMN09_REFCLK_VCM_PN_SET(_x)	((_x & 0xf) << 0)
#define USBDP_CMN09_REFCLK_VCM_PN_GET(_r)	((_r & 0xf) >> 0)

#define EXYNOS_USBDP_COM_CMN_R0A	(0x0028)
#define USBDP_CMN0A_PLL_AFC_FAST_LOCK_SETTLE_NO_MASK	(0x7 << 5)
#define USBDP_CMN0A_PLL_AFC_FAST_LOCK_SETTLE_NO_SET(_x)	((_x & 0x7) << 5)
#define USBDP_CMN0A_PLL_AFC_FAST_LOCK_SETTLE_NO_GET(_r)	((_r & 0x7) >> 5)
#define USBDP_CMN0A_PLL_AFC_FAST_LOCK_BYPASS	(1 << 4)
#define USBDP_CMN0A_PLL_AFC_BSEL		(1 << 3)
#define USBDP_CMN0A_AFC_PRESET_VCO_CNT_MASK	(0x7 << 0)
#define USBDP_CMN0A_AFC_PRESET_VCO_CNT_SET(_x)	((_x & 0x7) << 0)
#define USBDP_CMN0A_AFC_PRESET_VCO_CNT_GET(_r)	((_r & 0x7) >> 0)

#define EXYNOS_USBDP_COM_CMN_R0B	(0x002C)
#define USBDP_CMN0B_PLL_AFC_VFORCE			(1 << 7)
#define USBDP_CMN0B_PLL_SLOW_LOCK_BYPASS		(1 << 6)
#define USBDP_CMN0B_PLL_AFC_LOCK_PPM_SET_MASK		(0x1f << 1)
#define USBDP_CMN0B_PLL_AFC_LOCK_PPM_SET_SET(_x)	((_x & 0x1f) << 1)
#define USBDP_CMN0B_PLL_AFC_LOCK_PPM_SET_GET(_r)	((_r & 0x1f) >> 1)
#define USBDP_CMN0B_PLL_AFC_NON_CONTINUOUS_MODE		(1 << 0)

#define EXYNOS_USBDP_COM_CMN_R0C	(0x0030)
#define USBDP_CMN0C_PLL_AFC_TOL_MASK		(0xf << 4)
#define USBDP_CMN0C_PLL_AFC_TOL_SET(_x)		((_x & 0xf) << 4)
#define USBDP_CMN0C_PLL_AFC_TOL_GET(_r)		((_r & 0xf) >> 4)
#define USBDP_CMN0C_PLL_AFC_STB_NUM_MASK	(0xf << 0)
#define USBDP_CMN0C_PLL_AFC_STB_NUM_SET(_x)	((_x & 0xf) << 0)
#define USBDP_CMN0C_PLL_AFC_STB_NUM_GET(_r)	((_r & 0xf) >> 0)

#define EXYNOS_USBDP_COM_CMN_R0D	(0x0034)
#define USBDP_CMN0D_PLL_AFC_VCO_CNT_WAIT_NO_MASK	(0xf << 0)
#define USBDP_CMN0D_PLL_AFC_VCO_CNT_WAIT_NO_SET(_x)	((_x & 0xf) << 0)
#define USBDP_CMN0D_PLL_AFC_VCO_CNT_WAIT_NO_GET(_r)	((_r & 0xf) >> 0)
#define USBDP_CMN0D_LC_VCO_MODE				(1 << 4)
#define USBDP_CMN0D_PLL_AFC_MAN_BSEL_M_MASK		(0xf << 0)
#define USBDP_CMN0D_PLL_AFC_MAN_BSEL_M_SET(_x)		((_x & 0xf) << 0)
#define USBDP_CMN0D_PLL_AFC_MAN_BSEL_M_GET(_r)		((_r & 0xf) >> 0)


#define EXYNOS_USBDP_COM_CMN_R11	(0x0044)
#define USBDP_CMN11_PLL_ATB_SEL_MASK		(0x7 << 5)
#define USBDP_CMN11_PLL_ATB_SEL_SET(_x)		((_x & 0x7) << 5)
#define USBDP_CMN11_PLL_ATB_SEL_GET(_r)		((_r & 0x7) >> 5)
#define USBDP_CMN11_PLL_ANA_TEST_EN		(1 << 4)
#define USBDP_CMN11_PLL_ANA_PI_STR_MASK		(0xf << 0)
#define USBDP_CMN11_PLL_ANA_PI_STR_SET(_x)	((_x & 0xf) << 0)
#define USBDP_CMN11_PLL_ANA_PI_STR_GET(_r)	((_r & 0xf) >> 0)

#define EXYNOS_USBDP_COM_CMN_R12	(0x0048)
#define USBDP_CMN12_PLL_EN_100M			(1 << 7)
#define USBDP_CMN12_PLL_BYPASS_CLK_SEL_MASK	(0x3 << 5)
#define USBDP_CMN12_PLL_BYPASS_CLK_SEL_SET(_x)	((_x & 0x3) << 5)
#define USBDP_CMN12_PLL_BYPASS_CLK_SEL_GET(_r)	((_r & 0x3) >> 5)
#define USBDP_CMN12_PLL_CKFB_MON_EN		(1 << 4)
#define USBDP_CMN12_PLL_CK_MON_EN		(1 << 3)
#define USBDP_CMN12_PLL_CHOPPER_CLK_EN		(1 << 2)
#define USBDP_CMN12_PLL_ANA_VCO_PI_CTRL_MASK	(0xf << 0)
#define USBDP_CMN12_PLL_ANA_VCO_PI_CTRL_SET(_x)	((_x & 0xf) << 0)
#define USBDP_CMN12_PLL_ANA_VCO_PI_CTRL_GET(_r)	((_r & 0xf) >> 0)

#define EXYNOS_USBDP_COM_CMN_R13	(0x004C)
#define USBDP_CMN13_PLL_AFC_EN			(1 << 7)
#define USBDP_CMN13_PLL_DIV_NUM_MASK		(0x1f << 2)
#define USBDP_CMN13_PLL_DIV_NUM_SET(_x)		((_x & 0x1f) << 2)
#define USBDP_CMN13_PLL_DIV_NUM_GET(_r)		((_r & 0x1f) >> 2)
#define USBDP_CMN13_PLL_EDGE_OPT_MASK		(0x3 << 0)
#define USBDP_CMN13_PLL_EDGE_OPT_SET(_x)	((_x & 0x3) << 0)
#define USBDP_CMN13_PLL_EDGE_OPT_GET(_r)	((_r & 0x3) >> 0)

#define EXYNOS_USBDP_COM_CMN_R14	(0x0050)
#define USBDP_CMN14_PLL_FLD_RP_CODE_2_0_MASK	(0x7 << 5)
#define USBDP_CMN14_PLL_FLD_RP_CODE_2_0_SET(_x)	((_x & 0x7) << 5)
#define USBDP_CMN14_PLL_FLD_RP_CODE_2_0_GET(_r)	((_r & 0x7) >> 5)
#define USBDP_CMN14_PLL_FLD_RP_CODE_MASK	(0x7 << 2)
#define USBDP_CMN14_PLL_FLD_RP_CODE_SET(_x)	((_x & 0x7) << 2)
#define USBDP_CMN14_PLL_FLD_RP_CODE_GET(_r)	((_r & 0x7) >> 2)
#define USBDP_CMN14_PLL_FLD_CK_DIV_MASK		(0x3 << 0)
#define USBDP_CMN14_PLL_FLD_CK_DIV_SET(_x)	((_x & 0x3) << 0)
#define USBDP_CMN14_PLL_FLD_CK_DIV_GET(_r)	((_r & 0x3) >> 0)

#define EXYNOS_USBDP_COM_CMN_R15	(0x0054)
#define USBDP_CMN15_PLL_PH_FIX			(1 << 7)
#define USBDP_CMN15_PLL_PCG_CLK_OUT_EN		(1 << 6)
#define USBDP_CMN15_PLL_FLD_TG_CODE_8_3_MASK	(0x3f << 0)
#define USBDP_CMN15_PLL_FLD_TG_CODE_8_3_SET(_x)	((_x & 0x3f) << 0)
#define USBDP_CMN15_PLL_FLD_TG_CODE_8_3_GET(_r)	((_r & 0x3f) >> 0)

#define EXYNOS_USBDP_COM_CMN_R16	(0x0058)
#define USBDP_CMN16_PLL_PH_SEL_MASK		(0xf << 4)
#define USBDP_CMN16_PLL_PH_SEL_SET(_x)		((_x & 0xf) << 4)
#define USBDP_CMN16_PLL_PH_SEL_GET(_r)		((_r & 0xf) >> 4)
#define USBDP_CMN16_PLL_FLD_TOL_MASK		(0xf << 0)
#define USBDP_CMN16_PLL_FLD_TOL_SET(_x)		((_x & 0xf) << 0)
#define USBDP_CMN16_PLL_FLD_TOL_GET(_r)		((_r & 0xf) >> 0)

#define EXYNOS_USBDP_COM_CMN_R17	(0x005C)
#define USBDP_CMN17_PLL_RING_CLK_DIV2		(1 << 7)
#define USBDP_CMN17_PLL_PH_SEL_3B		(1 << 6)
#define USBDP_CMN17_PLL_MOD_CODE_MASK		(0x3f << 0)
#define USBDP_CMN17_PLL_MOD_CODE_SET(_x)	((_x & 0x3f) << 0)
#define USBDP_CMN17_PLL_MOD_CODE_GET(_r)	((_r & 0x3f) >> 0)

#define EXYNOS_USBDP_COM_CMN_R18	(0x0060)
#define USBDP_CMN18_MAN_PLL_PMS_M_MASK		(0xff << 0)
#define USBDP_CMN18_MAN_PLL_PMS_M_SET(_x)	((_x & 0xff) << 0)
#define USBDP_CMN18_MAN_PLL_PMS_M_GET(_r)	((_r & 0xff) >> 0)

#define EXYNOS_USBDP_COM_CMN_R19	(0x0064)
#define USBDP_CMN19_MAN_PLL_PMS_S_MASK		(0xf << 4)
#define USBDP_CMN19_MAN_PLL_PMS_S_SET(_x)	((_x & 0xf) << 4)
#define USBDP_CMN19_MAN_PLL_PMS_S_GET(_r)	((_r & 0xf) >> 4)
#define USBDP_CMN19_MAN_PLL_PMS_P_MASK		(0xf << 0)
#define USBDP_CMN19_MAN_PLL_PMS_P_SET(_x)	((_x & 0xf) << 0)
#define USBDP_CMN19_MAN_PLL_PMS_P_GET(_r)	((_r & 0xf) >> 0)

#define EXYNOS_USBDP_COM_CMN_R1A	(0x0068)
#define USBDP_CMN1A_MAN_PLL_SDC_LC_0		(1 << 7)
#define USBDP_CMN1A_MAN_PLL_SDC_K_CODE_MASK	(0x3f << 1)
#define USBDP_CMN1A_MAN_PLL_SDC_K_CODE_SET(_x)	((_x & 0x3f) << 1)
#define USBDP_CMN1A_MAN_PLL_SDC_K_CODE_GET(_r)	((_r & 0x3f) >> 1)
#define USBDP_CMN1A_MAN_PLL_SDC_N2		(1 << 0)

#define EXYNOS_USBDP_COM_CMN_R1B	(0x006C)
#define USBDP_CMN1B_MAN_PLL_SDC_N_MASK		(0x7 << 1)
#define USBDP_CMN1B_MAN_PLL_SDC_N_SET(_x)	((_x & 0x7) << 1)
#define USBDP_CMN1B_MAN_PLL_SDC_N_GET(_r)	((_r & 0x7) >> 1)
#define USBDP_CMN1B_MAN_PLL_SDC_LC_5_1_MASK	(0x1f << 0)
#define USBDP_CMN1B_MAN_PLL_SDC_LC_5_1_SET(_x)	((_x & 0x1f) << 0)
#define USBDP_CMN1B_MAN_PLL_SDC_LC_5_1_GET(_r)	((_r & 0x1f) >> 0)

#define EXYNOS_USBDP_COM_CMN_R1C	(0x0070)
#define USBDP_CMN1C_MAN_PLL_SDM_K_CODE_MASK	(0xff << 0)
#define USBDP_CMN1C_MAN_PLL_SDM_K_CODE_SET(_x)	((_x & 0xff) << 0)
#define USBDP_CMN1C_MAN_PLL_SDM_K_CODE_GET(_r)	((_r & 0xff) >> 0)

#define EXYNOS_USBDP_COM_CMN_R1D	(0x0074)
#define USBDP_CMN1D_PLL_ANA_LPF_CISEL_MASK	(0xff << 0)
#define USBDP_CMN1D_PLL_ANA_LPF_CISEL_SET(_x)	((_x & 0xff) << 0)
#define USBDP_CMN1D_PLL_ANA_LPF_CISEL_GET(_r)	((_r & 0xff) >> 0)

#define EXYNOS_USBDP_COM_CMN_R1E	(0x0078)
#define USBDP_CMN1E_MAN_PLL_SDM_LC_CLR		USBDP_COMBO_REG_CLR(0, 8)
#define USBDP_CMN1E_MAN_PLL_SDM_LC_MASK		USBDP_COMBO_REG_MSK(0, 8)
#define USBDP_CMN1E_MAN_PLL_SDM_LC_SET(_x)	USBDP_COMBO_REG_SET(_x, 0, 8)
#define USBDP_CMN1E_MAN_PLL_SDM_LC_GET(_R)	USBDP_COMBO_REG_GET(_R, 0, 8)

#define EXYNOS_USBDP_COM_CMN_R1F	(0x007C)
#define USBDP_CMN1F_MAN_HS_MODE			USBDP_COMBO_REG_MSK(7, 1)
#define USBDP_CMN1F_MAN_HS_MODE_EN		USBDP_COMBO_REG_MSK(6, 1)
#define USBDP_CMN1F_CD_RSTN_DLY_CODE_MASK	USBDP_COMBO_REG_MSK(3, 3)
#define USBDP_CMN1F_CD_RSTN_DLY_CODE_SET(_x)	USBDP_COMBO_REG_SET(_x, 3, 3)
#define USBDP_CMN1F_CD_RSTN_DLY_CODE_GET(_R)	USBDP_COMBO_REG_GET(_R, 3, 3)
#define USBDP_CMN1F_PLL_ANA_LPF_CSEL_MASK	USBDP_COMBO_REG_MSK(0, 3)
#define USBDP_CMN1F_PLL_ANA_LPF_CSEL_SET(_x)	USBDP_COMBO_REG_SET(_x, 0, 3)
#define USBDP_CMN1F_PLL_ANA_LPF_CSEL_GET(_R)	USBDP_COMBO_REG_GET(_R, 0, 3)

#define EXYNOS_USBDP_COM_CMN_R20	(0x0080)
#define USBDP_CMN1F_MAN_MDIV_RSTN		USBDP_COMBO_REG_MSK(7, 1)
#define USBDP_CMN1F_MAN_PDIV_RSTN		USBDP_COMBO_REG_MSK(6, 1)
#define USBDP_CMN1F_MAN_RSTN_CTRL		USBDP_COMBO_REG_MSK(5, 1)

#define EXYNOS_USBDP_COM_CMN_R21	(0x0084)

#define EXYNOS_USBDP_COM_CMN_R22	(0x0088)

#define EXYNOS_USBDP_COM_CMN_R23	(0x008C)


#define EXYNOS_USBDP_COM_CMN_R28	(0x00A0)

#define EXYNOS_USBDP_COM_CMN_R29	(0x00A4)

#define EXYNOS_USBDP_COM_CMN_R2A	(0x00A8)

#define EXYNOS_USBDP_COM_CMN_R2B	(0x00AC)


#define EXYNOS_USBDP_COM_CMN_R2E	(0x00B8)


#endif /* _PHY_EXYNOS_USBDP_R_CMN_H_ */
