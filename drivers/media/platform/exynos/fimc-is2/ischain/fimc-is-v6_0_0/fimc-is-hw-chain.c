/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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
#if defined(CONFIG_SECURE_CAMERA_USE)
#include <linux/smc.h>
#endif

#include <asm/neon.h>

#include "fimc-is-config.h"
#include "fimc-is-param.h"
#include "fimc-is-type.h"
#include "fimc-is-regs.h"
#include "fimc-is-core.h"
#include "fimc-is-hw-chain.h"
#include "fimc-is-hw-settle-10nm-lpp.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-csi.h"
#include "fimc-is-device-ischain.h"

#include "../../interface/fimc-is-interface-ischain.h"
#include "../../hardware/fimc-is-hw-control.h"
#include "../../hardware/fimc-is-hw-mcscaler-v2.h"

#if defined(CONFIG_SECURE_CAMERA_USE)
#define SECURE_ISCHAIN_PATH_MCSC_SRC0	0   /* ISPLP -> MCSC0 */

#define itfc_to_core(x) \
	container_of(x, struct fimc_is_core, interface_ischain)
#endif

static struct fimc_is_reg sysreg_isppre_regs[SYSREG_ISPPRE_REG_CNT] = {
	{0x0400, "ISPPRE_USER_CON"},
	{0x0404, "ISPPRE_SC_CON0"},
	{0x0408, "ISPPRE_SC_CON1"},
	{0x040c, "ISPPRE_SC_CON2"},
	{0x0414, "ISPPRE_SC_CON4"},
	{0x041c, "ISPPRE_SC_CON6"},
	{0x0424, "ISPPRE_SC_CON8"},
	{0x0430, "ISPPRE_SC_PDP_IN_EN"},
	{0x0500, "ISPPRE_MIPI_PHY_CON"},
	{0x0504, "ISPPRE_MIPI_PHY_MUX"},
};

static struct fimc_is_reg sysreg_isplp_regs[SYSREG_ISPLP_REG_CNT] = {
	{0x0404, "ISPLP_USER_CON"},
};

static struct fimc_is_field sysreg_isppre_fields[SYSREG_ISPPRE_REG_FIELD_CNT] = {
	{"ISPPRE_USER_CON",		9,	1,	RW,	0x1},	/* GLUEMUX_ISPHQ_VAL */
	{"ISPPRE_USER_CON",		8,	1,	RW,	0x0},	/* GLUEMUX_ISPLP_VAL */
	{"ISPPRE_USER_CON",		4,	4,	RW, 0x2},	/* AWCACHE_PORT_ISPPRE */
	{"ISPPRE_USER_CON",		0,	4,	RW, 0x2},	/* ARCACHE_PORT_ISPPRE */
	{"ISPPRE_SC_CON0",		0,	2,	RW,	0x0},	/* GLUEMUX_3AA0_VAL */
	{"ISPPRE_SC_CON1",		0,	2,	RW, 0x0},	/* GLUEMUX_3AA1_VAL */
	{"ISPPRE_SC_CON2",	0,	2,	RW, 0x0},	/* GLUEMUX_PDP_DMA0_OTF_SEL */
	{"ISPPRE_SC_CON4",	0,	2,	RW, 0x0},	/* GLUEMUX_PDP_DMA1_OTF_SEL */
	{"ISPPRE_SC_CON6",	0,	2,	RW, 0x0},	/* GLUEMUX_PDP_DMA2_OTF_SEL */
	{"ISPPRE_SC_CON8",	0,	2,	RW, 0x0},	/* GLUEMUX_PDP_DMA3_OTF_SEL */
	{"ISPPRE_SC_PDP_IN_EN",	7,	1,	RW, 0x0},	/* PDP_CORE1_IN_CSIS3_EN */
	{"ISPPRE_SC_PDP_IN_EN",	6,	1,	RW, 0x0},	/* PDP_CORE1_IN_CSIS2_EN */
	{"ISPPRE_SC_PDP_IN_EN",	5,	1,	RW, 0x0},	/* PDP_CORE1_IN_CSIS1_EN */
	{"ISPPRE_SC_PDP_IN_EN",	4,	1,	RW, 0x0},	/* PDP_CORE1_IN_CSIS0_EN */
	{"ISPPRE_SC_PDP_IN_EN",	3,	1,	RW, 0x0},	/* PDP_CORE0_IN_CSIS3_EN */
	{"ISPPRE_SC_PDP_IN_EN",	2,	1,	RW, 0x0},	/* PDP_CORE0_IN_CSIS2_EN */
	{"ISPPRE_SC_PDP_IN_EN",	1,	1,	RW, 0x0},	/* PDP_CORE0_IN_CSIS1_EN */
	{"ISPPRE_SC_PDP_IN_EN",	0,	1,	RW, 0x0},	/* PDP_CORE0_IN_CSIS0_EN */
	{"MIPI_PHY_CON",	4,	1,	RW, 0x0},	/* MIPI_RESETN_M0S4S4S2_S1 */
	{"MIPI_PHY_CON",	3,	1,	RW, 0x0},	/* MIPI_RESETN_M0S4S2_S1 */
	{"MIPI_PHY_CON",	2,	1,	RW, 0x0},	/* MIPI_RESETN_M0S4S2_S */
	{"MIPI_PHY_CON",	1,	1,	RW, 0x0},	/* MIPI_RESETN_M4S4_MODULE */
	{"MIPI_PHY_CON",	0,	1,	RW, 0x0},	/* MIPI_RESETN_M4S4_TOP */
	{"MIPI_PHY_MUX",	2,	1,	RW, 0x0},	/* CSIS2_DCPHY_S1_MUX_SEL */
	{"MIPI_PHY_MUX",	1,	1,	RW, 0x0},	/* CSIS1_DPHY_S_MUX_SEL */
	{"MIPI_PHY_MUX",	0,	1,	RW, 0x0},	/* CSIS0_DCPHY_S_MUX_SEL */
};

static struct fimc_is_field sysreg_isplp_fields[SYSREG_ISPLP_REG_FIELD_CNT] = {
	{"ISPLP_USER_CON",		20,	1,	RW,	0x1},	/* C2COM_SW_RESET */
	{"ISPLP_USER_CON",		18,	2,	RW,	0x1},	/* GLUEMUX_MCSC_SC1_VAL */
	{"ISPLP_USER_CON",		16,	2,	RW,	0x1},	/* GLUEMUX_MCSC_SC0_VAL */
	{"ISPLP_USER_CON",		12,	4,	RW,	0x2},	/* AWCACHE_ISPLP1 */
	{"ISPLP_USER_CON",		8, 4,	RW, 0x2},	/* ARCACHE_ISPLP1 */
	{"ISPLP_USER_CON",		4, 4,	RW, 0x2},	/* AWCACHE_ISPLP0 */
	{"ISPLP_USER_CON",		0, 4,	RW, 0x2},	/* ARCACHE_ISPLP0 */
};

#ifdef ENABLE_DCF
static struct fimc_is_reg sysreg_dcf_regs[SYSREG_DCF_REG_CNT] = {
	{0x0408, "DCF_CON"},
};

static struct fimc_is_field sysreg_dcf_fields[SYSREG_DCF_REG_FIELD_CNT] = {
	{"DCF_CON",	0,	1,	RW,	0x0},	/* CIP_SEL */
};
#endif

/* C2SYNC 1SLV */
enum c2sync_m0_s1_reg_name {
	C2SYNC_R_M0_S1_TRS_ENABLE0,
	C2SYNC_R_M0_S1_TRS_LIMIT0,
	C2SYNC_R_M0_S1_TRS_TOKENS_CROP_START0,
	C2SYNC_R_M0_S1_TRS_CROP_ENABLE0,
	C2SYNC_R_M0_S1_TRS_LINES_IN_FIRST_TOKEN0,
	C2SYNC_R_M0_S1_TRS_LINES_IN_TOKEN0,
	C2SYNC_R_M0_S1_TRS_LINES_COUNT0,
	C2SYNC_R_M0_S1_TRS_FLUSH0,
	C2SYNC_R_M0_S1_TRS_BUSY0,
	C2SYNC_R_M0_S1_TRS_C2COM_DEBUG,
	C2SYNC_R_M0_S1_TRS_C2COM_DEBUG_DOUT,
	C2SYNC_R_M0_S1_TRS_C2COM_RING_CLK_EN,
	C2SYNC_R_M0_S1_TRS_C2COM_LOCAL_IP,
	C2SYNC_R_M0_S1_TRS_C2COM_SW_RESET,
	C2SYNC_R_M0_S1_TRS_C2COM_SW_CORE_RESET,
	C2SYNC_R_M0_S1_TRS_C2COM_FORCE_INTERNAL_CLOCK,
	C2SYNC_R_M0_S1_TRS_C2COM_SEL_REGISTER,
	C2SYNC_R_M0_S1_TRS_C2COM_SEL_REGISTER_MODE,
	C2SYNC_R_M0_S1_TRS_C2COM_SHADOW_SW_TRIGGER,
	C2SYNC_M0_S1_REG_CNT,
};

static struct fimc_is_reg c2sync_m0_s1_regs[C2SYNC_M0_S1_REG_CNT] = {
	{0x0000, "C2SYNC_M0_S1_TRS_ENABLE0"},
	{0x0004, "C2SYNC_M0_S1_TRS_LIMIT0"},
	{0x0008, "C2SYNC_M0_S1_TRS_TOKENS_CROP_START0"},
	{0x000C, "C2SYNC_M0_S1_TRS_CROP_ENABLE0"},
	{0x0010, "C2SYNC_M0_S1_TRS_LINES_IN_FIRST_TOKEN0"},
	{0x0014, "C2SYNC_M0_S1_TRS_LINES_IN_TOKEN0"},
	{0x0018, "C2SYNC_M0_S1_TRS_LINES_COUNT0"},
	{0x001C, "C2SYNC_M0_S1_TRS_FLUSH0"},
	{0x0020, "C2SYNC_M0_S1_TRS_BUSY0"},
	{0x0024, "C2SYNC_M0_S1_TRS_C2COM_DEBUG"},
	{0x0028, "C2SYNC_M0_S1_TRS_C2COM_DEBUG_DOUT"},
	{0x002C, "C2SYNC_M0_S1_TRS_C2COM_RING_CLK_EN"},
	{0x0030, "C2SYNC_M0_S1_TRS_C2COM_LOCAL_IP"},
	{0x0034, "C2SYNC_M0_S1_TRS_C2COM_SW_RESET"},
	{0x0038, "C2SYNC_M0_S1_TRS_C2COM_SW_CORE_RESET"},
	{0x003C, "C2SYNC_M0_S1_TRS_C2COM_FORCE_INTERNAL_CLOCK"},
	{0x0040, "C2SYNC_M0_S1_TRS_C2COM_SEL_REGISTER"},
	{0x0044, "C2SYNC_M0_S1_TRS_C2COM_SEL_REGISTER_MODE"},
	{0x0048, "C2SYNC_M0_S1_TRS_C2COM_SHADOW_SW_TRIGGER"},
};

enum c2sync_m0_s1_reg_field {
	C2SYNC_F_M0_S1_TRS_ENABLE0,
	C2SYNC_F_M0_S1_TRS_LIMIT0,
	C2SYNC_F_M0_S1_TRS_TOKENS_CROP_START0,
	C2SYNC_F_M0_S1_TRS_CROP_ENABLE0,
	C2SYNC_F_M0_S1_TRS_LINES_IN_FIRST_TOKEN0,
	C2SYNC_F_M0_S1_TRS_LINES_IN_TOKEN0,
	C2SYNC_F_M0_S1_TRS_LINES_COUNT0,
	C2SYNC_F_M0_S1_TRS_FLUSH0,
	C2SYNC_F_M0_S1_TRS_BUSY0,
	C2SYNC_F_M0_S1_TRS_C2COM_DEBUG_EN,
	C2SYNC_F_M0_S1_TRS_C2COM_DEBUG_SEL,
	C2SYNC_F_M0_S1_TRS_C2COM_DEBUG_DOUT,
	C2SYNC_F_M0_S1_TRS_C2COM_RING_CLK_EN,
	C2SYNC_F_M0_S1_TRS_C2COM_LOCAL_IP,
	C2SYNC_F_M0_S1_TRS_C2COM_SW_RESET,
	C2SYNC_F_M0_S1_TRS_C2COM_SW_CORE_RESET,
	C2SYNC_F_M0_S1_TRS_C2COM_FORCE_INTERNAL_CLOCK,
	C2SYNC_F_M0_S1_TRS_C2COM_SEL_REGISTER,
	C2SYNC_F_M0_S1_TRS_C2COM_SEL_REGISTER_MODE,
	C2SYNC_F_M0_S1_TRS_C2COM_SHADOW_SW_TRIGGER,
	C2SYNC_M0_S1_REG_FIELD_CNT,
};

static struct fimc_is_field c2sync_m0_s1_fields[C2SYNC_M0_S1_REG_FIELD_CNT] = {
	{"C2SYNC_M0_S1_TRS_ENABLE0",				0,	1,	RWS,	0x0001},
	{"C2SYNC_M0_S1_TRS_LIMIT0",				0,	8,	RWS,	0x0019},
	{"C2SYNC_M0_S1_TRS_TOKENS_CROP_START0",			0,	12,	RWS,	0x0000},
	{"C2SYNC_M0_S1_TRS_CROP_ENABLE0",			0,	1,	RWS,	0x0001},
	{"C2SYNC_M0_S1_TRS_LINES_IN_FIRST_TOKEN0",		0,	8,	RWS,	0x0000},
	{"C2SYNC_M0_S1_TRS_LINES_IN_TOKEN0",			0,	8,	RWS,	0x0000},
	{"C2SYNC_M0_S1_TRS_LINES_COUNT0",			0,	14,	RWS,	0x1FFF},
	{"C2SYNC_M0_S1_TRS_FLUSH0",				0,	1,	RWS,	0x0000},
	{"C2SYNC_M0_S1_TRS_BUSY0",				0,	1,	W1C,	0x0000},
	{"C2SYNC_M0_S1_TRS_C2COM_DEBUG_EN",			0,	1,	RO,	0x0000},
	{"C2SYNC_M0_S1_TRS_C2COM_DEBUG_SEL",			0,	6,	RW,	0x0000},
	{"C2SYNC_M0_S1_TRS_C2COM_DEBUG_DOUT",			0,	16,	RW,	0x0000},
	{"C2SYNC_M0_S1_TRS_C2COM_RING_CLK_EN",			0,	1,	RW,	0x0000},
	{"C2SYNC_M0_S1_TRS_C2COM_LOCAL_IP",			0,	4,	RW,	0x0000},
	{"C2SYNC_M0_S1_TRS_C2COM_SW_RESET",			0,	1,	RW,	0x0000},
	{"C2SYNC_M0_S1_TRS_C2COM_SW_CORE_RESET",		0,	1,	RW,	0x0000},
	{"C2SYNC_M0_S1_TRS_C2COM_FORCE_INTERNAL_CLOCK",		0,	1,	RW,	0x0000},
	{"C2SYNC_M0_S1_TRS_C2COM_SEL_REGISTER",			0,	1,	RW,	0x0000},
	{"C2SYNC_M0_S1_TRS_C2COM_SEL_REGISTER_MODE",		0,	1,	RW,	0x0000},
	{"C2SYNC_M0_S1_TRS_C2COM_SHADOW_SW_TRIGGER",		0,	1,	RW,	0x0000},
};

/* C2SYNC 2SLV */
enum c2sync_m0_s2_reg_name {
	C2SYNC_R_M0_S2_TRS_ENABLE0,
	C2SYNC_R_M0_S2_TRS_LIMIT0,
	C2SYNC_R_M0_S2_TRS_TOKENS_CROP_START0,
	C2SYNC_R_M0_S2_TRS_CROP_ENABLE0,
	C2SYNC_R_M0_S2_TRS_LINES_IN_FIRST_TOKEN0,
	C2SYNC_R_M0_S2_TRS_LINES_IN_TOKEN0,
	C2SYNC_R_M0_S2_TRS_LINES_COUNT0,
	C2SYNC_R_M0_S2_TRS_FLUSH0,
	C2SYNC_R_M0_S2_TRS_BUSY0,
	C2SYNC_R_M0_S2_TRS_ENABLE1,
	C2SYNC_R_M0_S2_TRS_LIMIT1,
	C2SYNC_R_M0_S2_TRS_TOKENS_CROP_START1,
	C2SYNC_R_M0_S2_TRS_CROP_ENABLE1,
	C2SYNC_R_M0_S2_TRS_LINES_IN_FIRST_TOKEN1,
	C2SYNC_R_M0_S2_TRS_LINES_IN_TOKEN1,
	C2SYNC_R_M0_S2_TRS_LINES_COUNT1,
	C2SYNC_R_M0_S2_TRS_FLUSH1,
	C2SYNC_R_M0_S2_TRS_BUSY1,
	C2SYNC_R_M0_S2_TRS_C2COM_DEBUG,
	C2SYNC_R_M0_S2_TRS_C2COM_DEBUG_DOUT,
	C2SYNC_R_M0_S2_TRS_C2COM_RING_CLK_EN,
	C2SYNC_R_M0_S2_TRS_C2COM_LOCAL_IP,
	C2SYNC_R_M0_S2_TRS_C2COM_SW_RESET,
	C2SYNC_R_M0_S2_TRS_C2COM_SW_CORE_RESET,
	C2SYNC_R_M0_S2_TRS_C2COM_FORCE_INTERNAL_CLOCK,
	C2SYNC_R_M0_S2_TRS_C2COM_SEL_REGISTER,
	C2SYNC_R_M0_S2_TRS_C2COM_SEL_REGISTER_MODE,
	C2SYNC_R_M0_S2_TRS_C2COM_SHADOW_SW_TRIGGER,
	C2SYNC_M0_S2_REG_CNT,
};

static struct fimc_is_reg c2sync_m0_s2_regs[C2SYNC_M0_S2_REG_CNT] = {
	{0x0000, "C2SYNC_M0_S2_TRS_ENABLE0"},
	{0x0004, "C2SYNC_M0_S2_TRS_LIMIT0"},
	{0x0008, "C2SYNC_M0_S2_TRS_TOKENS_CROP_START0"},
	{0x000C, "C2SYNC_M0_S2_TRS_CROP_ENABLE0"},
	{0x0010, "C2SYNC_M0_S2_TRS_LINES_IN_FIRST_TOKEN0"},
	{0x0014, "C2SYNC_M0_S2_TRS_LINES_IN_TOKEN0"},
	{0x0018, "C2SYNC_M0_S2_TRS_LINES_COUNT0"},
	{0x001C, "C2SYNC_M0_S2_TRS_FLUSH0"},
	{0x0020, "C2SYNC_M0_S2_TRS_BUSY0"},
	{0x0024, "C2SYNC_M0_S2_TRS_ENABLE1"},
	{0x0028, "C2SYNC_M0_S2_TRS_LIMIT1"},
	{0x002C, "C2SYNC_M0_S2_TRS_TOKENS_CROP_START1"},
	{0x0030, "C2SYNC_M0_S2_TRS_CROP_ENABLE1"},
	{0x0034, "C2SYNC_M0_S2_TRS_LINES_IN_FIRST_TOKEN1"},
	{0x0038, "C2SYNC_M0_S2_TRS_LINES_IN_TOKEN1"},
	{0x003C, "C2SYNC_M0_S2_TRS_LINES_COUNT1"},
	{0x0040, "C2SYNC_M0_S2_TRS_FLUSH1"},
	{0x0044, "C2SYNC_M0_S2_TRS_BUSY1"},
	{0x0048, "C2SYNC_M0_S2_TRS_C2COM_DEBUG"},
	{0x004C, "C2SYNC_M0_S2_TRS_C2COM_DEBUG_DOUT"},
	{0x0050, "C2SYNC_M0_S2_TRS_C2COM_RING_CLK_EN"},
	{0x0054, "C2SYNC_M0_S2_TRS_C2COM_LOCAL_IP"},
	{0x0058, "C2SYNC_M0_S2_TRS_C2COM_SW_RESET"},
	{0x005C, "C2SYNC_M0_S2_TRS_C2COM_SW_CORE_RESET"},
	{0x0060, "C2SYNC_M0_S2_TRS_C2COM_FORCE_INTERNAL_CLOCK"},
	{0x0064, "C2SYNC_M0_S2_TRS_C2COM_SEL_REGISTER"},
	{0x0068, "C2SYNC_M0_S2_TRS_C2COM_SEL_REGISTER_MODE"},
	{0x006C, "C2SYNC_M0_S2_TRS_C2COM_SHADOW_SW_TRIGGER"},
};

enum c2sync_m0_s2_reg_field {
	C2SYNC_F_M0_S2_TRS_ENABLE0,
	C2SYNC_F_M0_S2_TRS_LIMIT0,
	C2SYNC_F_M0_S2_TRS_TOKENS_CROP_START0,
	C2SYNC_F_M0_S2_TRS_CROP_ENABLE0,
	C2SYNC_F_M0_S2_TRS_LINES_IN_FIRST_TOKEN0,
	C2SYNC_F_M0_S2_TRS_LINES_IN_TOKEN0,
	C2SYNC_F_M0_S2_TRS_LINES_COUNT0,
	C2SYNC_F_M0_S2_TRS_FLUSH0,
	C2SYNC_F_M0_S2_TRS_BUSY0,
	C2SYNC_F_M0_S2_TRS_ENABLE1,
	C2SYNC_F_M0_S2_TRS_LIMIT1,
	C2SYNC_F_M0_S2_TRS_TOKENS_CROP_START1,
	C2SYNC_F_M0_S2_TRS_CROP_ENABLE1,
	C2SYNC_F_M0_S2_TRS_LINES_IN_FIRST_TOKEN1,
	C2SYNC_F_M0_S2_TRS_LINES_IN_TOKEN1,
	C2SYNC_F_M0_S2_TRS_LINES_COUNT1,
	C2SYNC_F_M0_S2_TRS_FLUSH1,
	C2SYNC_F_M0_S2_TRS_BUSY1,
	C2SYNC_F_M0_S2_TRS_C2COM_DEBUG_EN,
	C2SYNC_F_M0_S2_TRS_C2COM_DEBUG_SEL,
	C2SYNC_F_M0_S2_TRS_C2COM_DEBUG_DOUT,
	C2SYNC_F_M0_S2_TRS_C2COM_RING_CLK_EN,
	C2SYNC_F_M0_S2_TRS_C2COM_LOCAL_IP,
	C2SYNC_F_M0_S2_TRS_C2COM_SW_RESET,
	C2SYNC_F_M0_S2_TRS_C2COM_SW_CORE_RESET,
	C2SYNC_F_M0_S2_TRS_C2COM_FORCE_INTERNAL_CLOCK,
	C2SYNC_F_M0_S2_TRS_C2COM_SEL_REGISTER,
	C2SYNC_F_M0_S2_TRS_C2COM_SEL_REGISTER_MODE,
	C2SYNC_F_M0_S2_TRS_C2COM_SHADOW_SW_TRIGGER,
	C2SYNC_M0_S2_REG_FIELD_CNT,
};

static struct fimc_is_field c2sync_m0_s2_fields[C2SYNC_M0_S2_REG_FIELD_CNT] = {
	{"C2SYNC_M0_S2_TRS_ENABLE0",				0,	1,	RWS,	0x0001},
	{"C2SYNC_M0_S2_TRS_LIMIT0",				0,	8,	RWS,	0x0019},
	{"C2SYNC_M0_S2_TRS_TOKENS_CROP_START0",			0,	12,	RWS,	0x0000},
	{"C2SYNC_M0_S2_TRS_CROP_ENABLE0",			0,	1,	RWS,	0x0001},
	{"C2SYNC_M0_S2_TRS_LINES_IN_FIRST_TOKEN0",		0,	8,	RWS,	0x0000},
	{"C2SYNC_M0_S2_TRS_LINES_IN_TOKEN0",			0,	8,	RWS,	0x0000},
	{"C2SYNC_M0_S2_TRS_LINES_COUNT0",			0,	14,	RWS,	0x1FFF},
	{"C2SYNC_M0_S2_TRS_FLUSH0",				0,	1,	RWS,	0x0000},
	{"C2SYNC_M0_S2_TRS_BUSY0",				0,	1,	W1C,	0x0000},
	{"C2SYNC_M0_S2_TRS_ENABLE1",				0,	1,	RWS,	0x0001},
	{"C2SYNC_M0_S2_TRS_LIMIT1",				0,	8,	RWS,	0x0019},
	{"C2SYNC_M0_S2_TRS_TOKENS_CROP_START1",			0,	12,	RWS,	0x0000},
	{"C2SYNC_M0_S2_TRS_CROP_ENABLE1",			0,	1,	RWS,	0x0001},
	{"C2SYNC_M0_S2_TRS_LINES_IN_FIRST_TOKEN1",		0,	8,	RWS,	0x0000},
	{"C2SYNC_M0_S2_TRS_LINES_IN_TOKEN1",			0,	8,	RWS,	0x0000},
	{"C2SYNC_M0_S2_TRS_LINES_COUNT1",			0,	14,	RWS,	0x1FFF},
	{"C2SYNC_M0_S2_TRS_FLUSH1",				0,	1,	RWS,	0x0000},
	{"C2SYNC_M0_S2_TRS_BUSY1",				0,	1,	W1C,	0x0000},
	{"C2SYNC_M0_S2_TRS_C2COM_DEBUG_EN",			0,	1,	RO,	0x0000},
	{"C2SYNC_M0_S2_TRS_C2COM_DEBUG_SEL",			0,	6,	RW,	0x0000},
	{"C2SYNC_M0_S2_TRS_C2COM_DEBUG_DOUT",			0,	16,	RW,	0x0000},
	{"C2SYNC_M0_S2_TRS_C2COM_RING_CLK_EN",			0,	1,	RW,	0x0000},
	{"C2SYNC_M0_S2_TRS_C2COM_LOCAL_IP",			0,	4,	RW,	0x0000},
	{"C2SYNC_M0_S2_TRS_C2COM_SW_RESET",			0,	1,	RW,	0x0000},
	{"C2SYNC_M0_S2_TRS_C2COM_SW_CORE_RESET",		0,	1,	RW,	0x0000},
	{"C2SYNC_M0_S2_TRS_C2COM_FORCE_INTERNAL_CLOCK",		0,	1,	RW,	0x0000},
	{"C2SYNC_M0_S2_TRS_C2COM_SEL_REGISTER",			0,	1,	RW,	0x0000},
	{"C2SYNC_M0_S2_TRS_C2COM_SEL_REGISTER_MODE",		0,	1,	RW,	0x0000},
	{"C2SYNC_M0_S2_TRS_C2COM_SHADOW_SW_TRIGGER",		0,	1,	RW,	0x0000},
};

/* Define default subdev ops if there are not used subdev IP */
const struct fimc_is_subdev_ops fimc_is_subdev_scc_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_scp_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_dis_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_dxc_ops;

struct fimc_is_clk_gate clk_gate_3aa0;
struct fimc_is_clk_gate clk_gate_3aa1;
struct fimc_is_clk_gate clk_gate_isp0;
struct fimc_is_clk_gate clk_gate_isp1;
struct fimc_is_clk_gate clk_gate_mcsc;
struct fimc_is_clk_gate clk_gate_vra;
struct fimc_is_clk_gate clk_gate_dcp;

void __iomem *hwfc_rst;
void __iomem *reg_c2sync_1slv;
void __iomem *reg_c2sync_2slv;
void __iomem *reg_cip1_clk;
void __iomem *reg_cip2_clk;

void fimc_is_enter_lib_isr(void)
{
	kernel_neon_begin();
}

void fimc_is_exit_lib_isr(void)
{
	kernel_neon_end();
}

void fimc_is_hw_group_init(struct fimc_is_group *group)
{
	group->subdev[ENTRY_SENSOR] = NULL;
	group->subdev[ENTRY_SSVC0] = NULL;
	group->subdev[ENTRY_SSVC1] = NULL;
	group->subdev[ENTRY_SSVC2] = NULL;
	group->subdev[ENTRY_SSVC3] = NULL;
	group->subdev[ENTRY_3AA] = NULL;
	group->subdev[ENTRY_3AC] = NULL;
	group->subdev[ENTRY_3AP] = NULL;
	group->subdev[ENTRY_ISP] = NULL;
	group->subdev[ENTRY_IXC] = NULL;
	group->subdev[ENTRY_IXP] = NULL;
	group->subdev[ENTRY_DRC] = NULL;
	group->subdev[ENTRY_DIS] = NULL;
	group->subdev[ENTRY_DXC] = NULL;
	group->subdev[ENTRY_ODC] = NULL;
	group->subdev[ENTRY_DNR] = NULL;
	group->subdev[ENTRY_DCP] = NULL;
	group->subdev[ENTRY_DC1S] = NULL;
	group->subdev[ENTRY_DC0C] = NULL;
	group->subdev[ENTRY_DC1C] = NULL;
	group->subdev[ENTRY_DC2C] = NULL;
	group->subdev[ENTRY_DC3C] = NULL;
	group->subdev[ENTRY_DC4C] = NULL;
	group->subdev[ENTRY_MCS] = NULL;
	group->subdev[ENTRY_M0P] = NULL;
	group->subdev[ENTRY_M1P] = NULL;
	group->subdev[ENTRY_M2P] = NULL;
	group->subdev[ENTRY_M3P] = NULL;
	group->subdev[ENTRY_M4P] = NULL;
	group->subdev[ENTRY_M5P] = NULL;
	group->subdev[ENTRY_VRA] = NULL;

	INIT_LIST_HEAD(&group->subdev_list);
}

int fimc_is_hw_group_cfg(void *group_data)
{
	int ret = 0;
	struct fimc_is_group *group;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_ischain *device;

	FIMC_BUG(!group_data);

	group = (struct fimc_is_group *)group_data;

#ifdef CONFIG_USE_SENSOR_GROUP
	if (group->slot == GROUP_SLOT_SENSOR) {
		sensor = group->sensor;
		if (!sensor) {
			err("device is NULL");
			BUG();
		}

		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_SENSOR] = &sensor->group_sensor.leader;
		group->subdev[ENTRY_SSVC0] = &sensor->ssvc0;
		group->subdev[ENTRY_SSVC1] = &sensor->ssvc1;
		group->subdev[ENTRY_SSVC2] = &sensor->ssvc2;
		group->subdev[ENTRY_SSVC3] = &sensor->ssvc3;

		list_add_tail(&sensor->group_sensor.leader.list, &group->subdev_list);
		list_add_tail(&sensor->ssvc0.list, &group->subdev_list);
		list_add_tail(&sensor->ssvc1.list, &group->subdev_list);
		list_add_tail(&sensor->ssvc2.list, &group->subdev_list);
		list_add_tail(&sensor->ssvc3.list, &group->subdev_list);
		return ret;
	}
#endif

	device = group->device;

	if (!device) {
		err("device is NULL");
		BUG();
	}

	switch (group->slot) {
	case GROUP_SLOT_3AA:
		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_3AA] = &device->group_3aa.leader;
		group->subdev[ENTRY_3AC] = &device->txc;
		group->subdev[ENTRY_3AP] = &device->txp;

		list_add_tail(&device->group_3aa.leader.list, &group->subdev_list);
		list_add_tail(&device->txc.list, &group->subdev_list);
		list_add_tail(&device->txp.list, &group->subdev_list);

		device->txc.param_dma_ot = PARAM_3AA_VDMA4_OUTPUT;
		device->txp.param_dma_ot = PARAM_3AA_VDMA2_OUTPUT;
		break;
	case GROUP_SLOT_ISP:
		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_ISP] = &device->group_isp.leader;
		group->subdev[ENTRY_IXC] = &device->ixc;
		group->subdev[ENTRY_IXP] = &device->ixp;

		list_add_tail(&device->group_isp.leader.list, &group->subdev_list);
		list_add_tail(&device->ixc.list, &group->subdev_list);
		list_add_tail(&device->ixp.list, &group->subdev_list);
		break;
	case GROUP_SLOT_DIS:
		break;
	case GROUP_SLOT_DCP:
		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_DCP] = &device->group_dcp.leader;
		group->subdev[ENTRY_DC1S] = &device->dc1s;
		group->subdev[ENTRY_DC0C] = &device->dc0c;
		group->subdev[ENTRY_DC1C] = &device->dc1c;
		group->subdev[ENTRY_DC2C] = &device->dc2c;
		group->subdev[ENTRY_DC3C] = &device->dc3c;
		group->subdev[ENTRY_DC4C] = &device->dc4c;

		list_add_tail(&device->group_dcp.leader.list, &group->subdev_list);
		list_add_tail(&device->dc1s.list, &group->subdev_list);
		list_add_tail(&device->dc0c.list, &group->subdev_list);
		list_add_tail(&device->dc1c.list, &group->subdev_list);
		list_add_tail(&device->dc2c.list, &group->subdev_list);
		list_add_tail(&device->dc3c.list, &group->subdev_list);
		list_add_tail(&device->dc4c.list, &group->subdev_list);
		break;
	case GROUP_SLOT_MCS:
		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_MCS] = &device->group_mcs.leader;
		group->subdev[ENTRY_M0P] = &device->m0p;
		group->subdev[ENTRY_M1P] = &device->m1p;
		group->subdev[ENTRY_M2P] = &device->m2p;
		group->subdev[ENTRY_M3P] = &device->m3p;
		group->subdev[ENTRY_M4P] = &device->m4p;
		group->subdev[ENTRY_M5P] = &device->m5p;

		list_add_tail(&device->group_mcs.leader.list, &group->subdev_list);
		list_add_tail(&device->m0p.list, &group->subdev_list);
		list_add_tail(&device->m1p.list, &group->subdev_list);
		list_add_tail(&device->m2p.list, &group->subdev_list);
		list_add_tail(&device->m3p.list, &group->subdev_list);
		list_add_tail(&device->m4p.list, &group->subdev_list);
		list_add_tail(&device->m5p.list, &group->subdev_list);

		device->m0p.param_dma_ot = PARAM_MCS_OUTPUT0;
		device->m1p.param_dma_ot = PARAM_MCS_OUTPUT1;
		device->m2p.param_dma_ot = PARAM_MCS_OUTPUT2;
		device->m3p.param_dma_ot = PARAM_MCS_OUTPUT3;
		device->m4p.param_dma_ot = PARAM_MCS_OUTPUT4;
		device->m5p.param_dma_ot = PARAM_MCS_OUTPUT5;
		break;
	case GROUP_SLOT_VRA:
		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_VRA] = &device->group_vra.leader;

		list_add_tail(&device->group_vra.leader.list, &group->subdev_list);
		break;
	default:
		probe_err("group slot(%d) is invalid", group->slot);
		BUG();
		break;
	}

	/* for hwfc: reset all REGION_IDX registers and outputs */
	hwfc_rst = ioremap(HWFC_INDEX_RESET_ADDR, SZ_4);
	reg_c2sync_1slv = ioremap(0x16BF0000, SZ_256); /* OOF */
	reg_c2sync_2slv = ioremap(0x16AF0000, SZ_256); /* HBZ */
	reg_cip1_clk = ioremap(0x16A80050, SZ_4);	/* CIP1 ip_processing */
	reg_cip2_clk = ioremap(0x16B80050, SZ_4);	/* CIP2 ip_processing */

	return ret;
}

int fimc_is_hw_group_open(void *group_data)
{
	int ret = 0;
	u32 group_id;
	struct fimc_is_subdev *leader;
	struct fimc_is_group *group;
	struct fimc_is_device_ischain *device;

	FIMC_BUG(!group_data);

	group = group_data;
	leader = &group->leader;
	device = group->device;
	group_id = group->id;

	switch (group_id) {
#ifdef CONFIG_USE_SENSOR_GROUP
	case GROUP_ID_SS0:
	case GROUP_ID_SS1:
	case GROUP_ID_SS2:
	case GROUP_ID_SS3:
	case GROUP_ID_SS4:
	case GROUP_ID_SS5:
#endif
	case GROUP_ID_3AA0:
	case GROUP_ID_3AA1:
	case GROUP_ID_ISP0:
	case GROUP_ID_ISP1:
	case GROUP_ID_MCS0:
	case GROUP_ID_MCS1:
	case GROUP_ID_DCP:
		leader->constraints_width = 5376;
		leader->constraints_height = 4032;
		break;
	case GROUP_ID_VRA0:
		leader->constraints_width = 640;
		leader->constraints_height = 480;
		break;
	default:
		merr("group id is invalid(%d)", group, group_id);
		break;
	}

	return ret;
}

void fimc_is_hw_camif_init(void)
{
}

int fimc_is_hw_camif_cfg(void *sensor_data)
{
	int ret = 0;
	int vc;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_csi *csi;
	struct fimc_is_device_ischain *ischain;
	struct fimc_is_subdev *dma_subdev;
	struct fimc_is_sensor_cfg *sensor_cfg;
#if !defined(CONFIG_SECURE_CAMERA_USE)
	void __iomem *isppre_reg = ioremap_nocache(SYSREG_ISPPRE_BASE_ADDR, 0x1000);
#else
	void __iomem *isppre_reg = ioremap_nocache(SYSREG_ISPPRE_BASE_ADDR, 0x424);
#endif
	u32 pdp_ch = 0;
	u32 csi_ch = 0;
	u32 dma_ch = 0;
	u32 mux_val = 0;
	u32 logical_dma_offset;

	FIMC_BUG(!sensor_data);

	sensor = sensor_data;
	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);
	if (!csi) {
		merr("csi is null\n", sensor);
		ret = -ENODEV;
		goto p_err;
	}

	csi_ch = csi->instance;
	if (csi_ch > CSI_ID_MAX) {
		merr("CSI channel is invalid(%d)\n", sensor, csi_ch);
		ret = -ERANGE;
		goto p_err;
	}

	/* CSIS to PDP MUX */
	ischain = sensor->ischain;
	if (ischain) {
		pdp_ch = (ischain->group_3aa.id == GROUP_ID_3AA1) ? 1 : 0;

		switch (pdp_ch) {
		case 0:
			fimc_is_hw_set_reg(isppre_reg, &sysreg_isppre_regs[SYSREG_ISPPRE_R_SC_CON0], csi_ch);
			break;
		case 1:
			fimc_is_hw_set_reg(isppre_reg, &sysreg_isppre_regs[SYSREG_ISPPRE_R_SC_CON1], csi_ch);
			break;
		}
		minfo("CSI(%d) --> PDP(%d)\n", sensor, csi_ch, pdp_ch);
	}

	/* DMA input MUX */
	for (vc = CSI_VIRTUAL_CH_0; vc < CSI_VIRTUAL_CH_MAX; vc++) {
		dma_subdev = csi->dma_subdev[vc];
		if (!dma_subdev)
			continue;

		if (!test_bit(FIMC_IS_SUBDEV_OPEN, &dma_subdev->state))
			continue;

		sensor_cfg = sensor->cfg;
		if (!sensor_cfg) {
			merr("sensor_cfg is null\n", sensor);
			ret = -ENODEV;
			goto p_err;
		}

		if (sensor_cfg->pd_mode == PD_NONE) {
			mux_val = 1;
			logical_dma_offset = 0;
		} else {
			mux_val = pdp_ch + 2;
			logical_dma_offset = 1;
		}

		dma_ch = dma_subdev->dma_ch[logical_dma_offset];
		switch (dma_ch) {
		case 0:
			fimc_is_hw_set_reg(isppre_reg, &sysreg_isppre_regs[SYSREG_ISPPRE_R_SC_CON2], mux_val);
			break;
		case 1:
			fimc_is_hw_set_reg(isppre_reg, &sysreg_isppre_regs[SYSREG_ISPPRE_R_SC_CON4], mux_val);
			break;
		case 2:
			fimc_is_hw_set_reg(isppre_reg, &sysreg_isppre_regs[SYSREG_ISPPRE_R_SC_CON6], mux_val);
			break;
		case 3:
#if !defined(CONFIG_SECURE_CAMERA_USE)
			fimc_is_hw_set_reg(isppre_reg, &sysreg_isppre_regs[SYSREG_ISPPRE_R_SC_CON8], mux_val);
#endif
			break;
		case 4:
		case 5:
			break;
		default:
			merr("DMA channel is invalid(%d)", sensor, dma_ch);
			ret = -ERANGE;
			goto p_err;
		}
		minfo("DMA #%d VC #%d input(%d)\n", sensor, dma_ch, vc, mux_val);
	}

p_err:
	iounmap(isppre_reg);
	return ret;
}

int fimc_is_hw_camif_open(void *sensor_data)
{
	int ret = 0;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_csi *csi;

	FIMC_BUG(!sensor_data);

	sensor = sensor_data;
	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);

	switch (csi->instance) {
	case 0:
	case 1:
	case 2:
	case 3:
		set_bit(CSIS_DMA_ENABLE, &csi->state);
#ifdef SOC_SSVC0
		csi->dma_subdev[CSI_VIRTUAL_CH_0] = &sensor->ssvc0;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_0] = NULL;
#endif
#ifdef SOC_SSVC1
		csi->dma_subdev[CSI_VIRTUAL_CH_1] = &sensor->ssvc1;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_1] = NULL;
#endif
#ifdef SOC_SSVC2
		csi->dma_subdev[CSI_VIRTUAL_CH_2] = &sensor->ssvc2;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_2] = NULL;
#endif
#ifdef SOC_SSVC3
		csi->dma_subdev[CSI_VIRTUAL_CH_3] = &sensor->ssvc3;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_3] = NULL;
#endif
		break;
	default:
		merr("sensor id is invalid(%d)", sensor, csi->instance);
		break;
	}

	return ret;
}

void fimc_is_hw_ischain_qe_cfg(void)
{
#if 0
	void __iomem *qe_3aa0_regs;
	void __iomem *qe_isplp_regs;
	void __iomem *qe_3aa1_regs;
	void __iomem *qe_pdp_regs;

	qe_3aa0_regs = ioremap_nocache(QE_3AA0_BASE_ADDR, 0x80);
	qe_isplp_regs = ioremap_nocache(QE_ISPLP_BASE_ADDR, 0x80);
	qe_3aa1_regs = ioremap_nocache(QE_3AA1_BASE_ADDR, 0x80);
	qe_pdp_regs = ioremap_nocache(QE_PDP_BASE_ADDR, 0x80);

	writel(0x00004000, qe_3aa0_regs + 0x20);
	writel(0x00000010, qe_3aa0_regs + 0x24);
	writel(0x00004000, qe_3aa0_regs + 0x40);
	writel(0x00000010, qe_3aa0_regs + 0x44);

	writel(0x00004001, qe_3aa0_regs + 0x20);
	writel(0x00004001, qe_3aa0_regs + 0x40);
	writel(1, qe_3aa0_regs);

	writel(0x00002000, qe_isplp_regs + 0x20);
	writel(0x00000010, qe_isplp_regs + 0x24);
	writel(0x00002000, qe_isplp_regs + 0x40);
	writel(0x00000010, qe_isplp_regs + 0x44);

	writel(0x00002001, qe_isplp_regs + 0x20);
	writel(0x00002001, qe_isplp_regs + 0x40);
	writel(1, qe_isplp_regs);

	writel(0x00008000, qe_3aa1_regs + 0x20);
	writel(0x00000010, qe_3aa1_regs + 0x24);
	writel(0x00008000, qe_3aa1_regs + 0x40);
	writel(0x00000010, qe_3aa1_regs + 0x44);

	writel(0x00008001, qe_3aa1_regs + 0x20);
	writel(0x00008001, qe_3aa1_regs + 0x40);
	writel(1, qe_3aa1_regs);

	writel(0x00001000, qe_pdp_regs + 0x20);
	writel(0x00000010, qe_pdp_regs + 0x24);
	writel(0x00001000, qe_pdp_regs + 0x40);
	writel(0x00000010, qe_pdp_regs + 0x44);

	writel(0x00001001, qe_pdp_regs + 0x20);
	writel(0x00001001, qe_pdp_regs + 0x40);
	writel(1, qe_pdp_regs);

	iounmap(qe_3aa0_regs);
	iounmap(qe_isplp_regs);
	iounmap(qe_3aa1_regs);
	iounmap(qe_pdp_regs);
#endif
	dbg_hw(2, "%s() \n", __func__);
}

int fimc_is_hw_ischain_cfg(void *ischain_data)
{
	int ret = 0;
	struct fimc_is_device_ischain *device;

	void __iomem *isppre_regs;
	void __iomem *isplp_regs;

	u32 isplp_val = 0, isppre_val = 0;
	u32 isplp_backup = 0, isppre_backup = 0;

	u32 input_isp0 = 0, input_isp1 = 1;
	u32 input_mcsc_src0 = 0, input_mcsc_src1 = 0;
#ifdef ENABLE_DCF
	void __iomem *dcf_regs;
	u32 dcf_val = 0, dcf_backup = 0;
	u32 output_dcf = 0;
#endif

	FIMC_BUG(!ischain_data);

	device = (struct fimc_is_device_ischain *)ischain_data;
	if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
		return ret;

	isppre_regs   = ioremap_nocache(SYSREG_ISPPRE_BASE_ADDR, 0x1000);
	isplp_regs = ioremap_nocache(SYSREG_ISPLP_BASE_ADDR, 0x1000);

	isppre_val = fimc_is_hw_get_reg(isppre_regs, &sysreg_isppre_regs[SYSREG_ISPPRE_R_USER_CON]);
	isplp_val = fimc_is_hw_get_reg(isplp_regs, &sysreg_isplp_regs[SYSREG_ISPLP_R_USER_CON]);

	isppre_backup = isppre_val;
	isplp_backup = isplp_val;

	/*
	 * 1) Select ISPLP(ISP0) input
	 *    3AAW(3AA0) : 0 <= Always
	 *    3AA(3AA1)  : 1

	 * 2) Select ISPHQ(ISP1) input
	 *    3AAW(3AA0) : 0
	 *    3AA(3AA1)  : 1 <= Always
	 */
	if ((device->group_3aa.id == GROUP_ID_3AA0 && device->group_isp.id == GROUP_ID_ISP0)
		|| (device->group_3aa.id == GROUP_ID_3AA1 && device->group_isp.id == GROUP_ID_ISP1)) {
		input_isp0 = 0;
		input_isp1 = 1;
	} else {
		input_isp0 = 1;
		input_isp1 = 0;
	}
	isppre_val = fimc_is_hw_set_field_value(isppre_val,
		&sysreg_isppre_fields[SYSREG_ISPPRE_F_GLUEMUX_ISPHQ_VAL], input_isp0);

	isppre_val = fimc_is_hw_set_field_value(isppre_val,
		&sysreg_isppre_fields[SYSREG_ISPPRE_F_GLUEMUX_ISPLP_VAL], input_isp1);

	/*
	 * 3) Select MC-Scaler SRC0 input
	 *    ISPLP -> MCSC0: 1  <= basic scenario
	 *    DCF -> MCSC0 : 3
	 * 4) Select MC-Scaler SRC1 input
	 *    ISPHQ -> MCSC1: 1  <= basic scenario
	 *    DCF -> MCSC1 : 3
	 */
	input_mcsc_src0 = 1;
	input_mcsc_src1 = 1;
	/* It will be changed in fimc_is_hardware_shot */
	isplp_val = fimc_is_hw_set_field_value(isplp_val,
				&sysreg_isplp_fields[SYSREG_ISPLP_F_GLUEMUX_MCSC_SC0_VAL], input_mcsc_src0);
	isplp_val = fimc_is_hw_set_field_value(isplp_val,
				&sysreg_isplp_fields[SYSREG_ISPLP_F_GLUEMUX_MCSC_SC1_VAL], input_mcsc_src1);

	/* 6) Write result to register */

	if (isplp_backup != isplp_val) {
		minfo("SYSREG_ISPLP_R_USER_CON:(0x%08X)->(0x%08X)\n", device, isplp_backup, isplp_val);
		fimc_is_hw_set_reg(isplp_regs, &sysreg_isplp_regs[SYSREG_ISPLP_R_USER_CON], isplp_val);	/* MCSC input */
	}

	iounmap(isppre_regs);
	iounmap(isplp_regs);

#ifdef ENABLE_DCF
	dcf_regs = ioremap_nocache(SYSREG_DCF_BASE_ADDR, 0x1000);
	dcf_val = fimc_is_hw_get_reg(dcf_regs, &sysreg_dcf_regs[SYSREG_DCF_R_DCF_CON]);
	dcf_backup = dcf_val;

	/*
	 * 5) Select CIP1/CIP2 output
	 *    CIP1 -> MCSC OTF : 0	<-Use Fusion
	 *    CIP2 -> MCSC OTF : 1	<-Use OOF
	 */
	/* TODO */
	output_dcf = 0;
	dcf_val = fimc_is_hw_set_field_value(dcf_val,
				&sysreg_dcf_fields[SYSREG_DCF_F_CIP_SEL], output_dcf);


	if (dcf_backup != dcf_val) {
		minfo("SYSREG_DCF_R_DCF_CON:(0x%08X)->(0x%08X)\n", device, dcf_backup, dcf_val);
		fimc_is_hw_set_reg(dcf_regs, &sysreg_dcf_regs[SYSREG_DCF_R_DCF_CON], dcf_val);	/* CIP output*/
	}

	iounmap(dcf_regs);
#endif

	return ret;
}

#ifdef ENABLE_HWACG_CONTROL
void fimc_is_hw_csi_qchannel_enable_all(bool enable)
{
	void __iomem *csi0_regs;
	void __iomem *csi1_regs;
	void __iomem *csi2_regs;
#if !defined(CONFIG_SECURE_CAMERA_USE)
	void __iomem *csi3_regs;
#endif
	u32 reg_val;

	csi0_regs = ioremap_nocache(CSIS0_QCH_EN_ADDR, SZ_4);
	csi1_regs = ioremap_nocache(CSIS1_QCH_EN_ADDR, SZ_4);
	csi2_regs = ioremap_nocache(CSIS2_QCH_EN_ADDR, SZ_4);
#if !defined(CONFIG_SECURE_CAMERA_USE)
	csi3_regs = ioremap_nocache(CSIS3_QCH_EN_ADDR, SZ_4);
#endif

	reg_val = readl(csi0_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi0_regs);

	reg_val = readl(csi1_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi1_regs);

	reg_val = readl(csi2_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi2_regs);

#if !defined(CONFIG_SECURE_CAMERA_USE)
	reg_val = readl(csi3_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi3_regs);
#endif

	iounmap(csi0_regs);
	iounmap(csi1_regs);
	iounmap(csi2_regs);
#if !defined(CONFIG_SECURE_CAMERA_USE)
	iounmap(csi3_regs);
#endif
}
#endif

static inline void fimc_is_isr1_ddk(void *data)
{
	struct fimc_is_interface_hwip *itf_hw = NULL;
	struct hwip_intr_handler *intr_hw = NULL;

	itf_hw = (struct fimc_is_interface_hwip *)data;
	intr_hw = &itf_hw->handler[INTR_HWIP1];

	if (intr_hw->valid) {
		fimc_is_enter_lib_isr();
		intr_hw->handler(intr_hw->id, intr_hw->ctx);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[ID:%d](1) empty handler!!", itf_hw->id);
	}
}

static inline void fimc_is_isr2_ddk(void *data)
{
	struct fimc_is_interface_hwip *itf_hw = NULL;
	struct hwip_intr_handler *intr_hw = NULL;

	itf_hw = (struct fimc_is_interface_hwip *)data;
	intr_hw = &itf_hw->handler[INTR_HWIP2];

	if (intr_hw->valid) {
		fimc_is_enter_lib_isr();
		intr_hw->handler(intr_hw->id, intr_hw->ctx);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[ID:%d](2) empty handler!!", itf_hw->id);
	}
}

static inline void fimc_is_isr1_host(void *data)
{
	struct fimc_is_interface_hwip *itf_hw = NULL;
	struct hwip_intr_handler *intr_hw = NULL;

	itf_hw = (struct fimc_is_interface_hwip *)data;
	intr_hw = &itf_hw->handler[INTR_HWIP1];

	if (intr_hw->valid)
		intr_hw->handler(intr_hw->id, (void *)itf_hw->hw_ip);
	else
		err_itfc("[ID:%d] empty handler!!", itf_hw->id);

}

static irqreturn_t fimc_is_isr1_3aa0(int irq, void *data)
{
	fimc_is_isr1_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr2_3aa0(int irq, void *data)
{
	fimc_is_isr2_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr1_3aa1(int irq, void *data)
{
	fimc_is_isr1_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr2_3aa1(int irq, void *data)
{
	fimc_is_isr2_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr1_isp0(int irq, void *data)
{
	fimc_is_isr1_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr2_isp0(int irq, void *data)
{
	fimc_is_isr2_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr1_isp1(int irq, void *data)
{
	fimc_is_isr1_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr2_isp1(int irq, void *data)
{
	fimc_is_isr2_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr1_mcs0(int irq, void *data)
{
	fimc_is_isr1_host(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr1_mcs1(int irq, void *data)
{
	fimc_is_isr1_host(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr1_vra(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_hw = NULL;
	struct hwip_intr_handler *intr_hw = NULL;

	itf_hw = (struct fimc_is_interface_hwip *)data;
	intr_hw = &itf_hw->handler[INTR_HWIP2];

	if (intr_hw->valid) {
		fimc_is_enter_lib_isr();
		intr_hw->handler(intr_hw->id, itf_hw->hw_ip);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[ID:%d](2) empty handler!!", itf_hw->id);
	}

	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr1_dcp(int irq, void *data)
{
	fimc_is_isr1_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr2_dcp(int irq, void *data)
{
	fimc_is_isr2_ddk(data);
	return IRQ_HANDLED;
}

inline int fimc_is_hw_slot_id(int hw_id)
{
	int slot_id = -1;

	switch (hw_id) {
	case DEV_HW_3AA0: /* 3AAW */
		slot_id = 0;
		break;
	case DEV_HW_3AA1:
		slot_id = 1;
		break;
	case DEV_HW_ISP0: /* ISPLP */
		slot_id = 2;
		break;
	case DEV_HW_ISP1: /* ISPHQ */
		slot_id = 3;
		break;
	case DEV_HW_MCSC0:
		slot_id = 4;
		break;
	case DEV_HW_MCSC1:
		slot_id = 5;
		break;
	case DEV_HW_VRA:
		slot_id = 6;
		break;
	case DEV_HW_DCP:
		slot_id = 7;
		break;
	default:
		break;
	}

	return slot_id;
}

int fimc_is_get_hw_list(int group_id, int *hw_list)
{
	int i;
	int hw_index = 0;

	/* initialization */
	for (i = 0; i < GROUP_HW_MAX; i++)
		hw_list[i] = -1;

	switch (group_id) {
	case GROUP_ID_3AA0:
		hw_list[hw_index] = DEV_HW_3AA0; hw_index++;
		break;
	case GROUP_ID_3AA1:
		hw_list[hw_index] = DEV_HW_3AA1; hw_index++;
		break;
	case GROUP_ID_ISP0:
		hw_list[hw_index] = DEV_HW_ISP0; hw_index++;
		break;
	case GROUP_ID_ISP1:
		hw_list[hw_index] = DEV_HW_ISP1; hw_index++;
		break;
	case GROUP_ID_DIS0:
		hw_list[hw_index] = DEV_HW_TPU0; hw_index++;
		break;
	case GROUP_ID_DIS1:
		hw_list[hw_index] = DEV_HW_TPU1; hw_index++;
		break;
	case GROUP_ID_DCP:
		hw_list[hw_index] = DEV_HW_DCP; hw_index++;
		break;
	case GROUP_ID_MCS0:
		hw_list[hw_index] = DEV_HW_MCSC0; hw_index++;
		break;
	case GROUP_ID_MCS1:
		hw_list[hw_index] = DEV_HW_MCSC1; hw_index++;
		break;
	case GROUP_ID_VRA0:
		hw_list[hw_index] = DEV_HW_VRA; hw_index++;
		break;
	default:
		break;
	}

	return hw_index;
}

static int fimc_is_hw_get_clk_gate(struct fimc_is_hw_ip *hw_ip, int hw_id)
{
	int ret = 0;
	struct fimc_is_clk_gate *clk_gate = NULL;

	switch (hw_id) {
	case DEV_HW_3AA0:
		clk_gate = &clk_gate_3aa0;
		clk_gate->regs = ioremap_nocache(0x16290084, 0x4);	/*Q-channel protocol related register*/
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_3AA1:
		clk_gate = &clk_gate_3aa1;
		clk_gate->regs = ioremap_nocache(0x162A0084, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_ISP0:
		clk_gate = &clk_gate_isp0;
		clk_gate->regs = ioremap_nocache(0x16430094, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_ISP1:
		clk_gate = &clk_gate_isp1;
		clk_gate->regs = ioremap_nocache(0x16630084, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_MCSC0:
	case DEV_HW_MCSC1:
		clk_gate = &clk_gate_mcsc;
		clk_gate->regs = ioremap_nocache(0x16440004, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
		}
		/* TODO : check bit :QACTIVE_ENABLE bit is [1] */

		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_VRA:
		clk_gate = &clk_gate_vra;
		clk_gate->regs = ioremap_nocache(0x1651b0dc, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_DCP:
		clk_gate = &clk_gate_dcp;
		clk_gate->regs = ioremap_nocache(0x1688007c, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	default:
		probe_err("hw_id(%d) is invalid", hw_id);
		ret = -EINVAL;
	}

	hw_ip->clk_gate = clk_gate;

	return ret;
}

int fimc_is_hw_get_address(void *itfc_data, void *pdev_data, int hw_id)
{
	int ret = 0;
	struct resource *mem_res = NULL;
	struct platform_device *pdev = NULL;
	struct fimc_is_interface_hwip *itf_hwip = NULL;

	FIMC_BUG(!itfc_data);
	FIMC_BUG(!pdev_data);

	itf_hwip = (struct fimc_is_interface_hwip *)itfc_data;
	pdev = (struct platform_device *)pdev_data;

	switch (hw_id) {
	case DEV_HW_3AA0:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_3AAW);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] 3AA VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_3AA1:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_3AA);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] 3AA VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_ISP0:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_ISPLP);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] ISP VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_ISP1:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_ISPHQ);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}
		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] ISP1 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_MCSC0:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_MCSC);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] MCSC0 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_MCSC1:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_MCSC);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] MCSC1 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_VRA:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_VRA_CH0);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] VRA0 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);

		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_VRA_CH1);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_b_start = mem_res->start;
		itf_hwip->hw_ip->regs_b_end = mem_res->end;
		itf_hwip->hw_ip->regs_b = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs_b) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] VRA1 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs_b);
		break;
	case DEV_HW_DCP:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_DCP);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] DCP VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	default:
		probe_err("hw_id(%d) is invalid", hw_id);
		return -EINVAL;
		break;
	}

	ret = fimc_is_hw_get_clk_gate(itf_hwip->hw_ip, hw_id);;
	if (ret)
		dev_err(&pdev->dev, "fimc_is_hw_get_clk_gate is fail\n");

	return ret;
}

int fimc_is_hw_get_irq(void *itfc_data, void *pdev_data, int hw_id)
{
	struct fimc_is_interface_hwip *itf_hwip = NULL;
	struct platform_device *pdev = NULL;
	int ret = 0;

	FIMC_BUG(!itfc_data);

	itf_hwip = (struct fimc_is_interface_hwip *)itfc_data;
	pdev = (struct platform_device *)pdev_data;

	switch (hw_id) {
	case DEV_HW_3AA0: /* 3AAW */
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 0);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq 3aa0-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 1);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq 3aa0-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_3AA1:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 2);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq 3aa1-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 3);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq 3aa0-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_ISP0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 4);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq isp0-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 5);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq isp0-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_ISP1:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 6);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq isp1-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 7);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq isp1-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_MCSC0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 8);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq scaler\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_MCSC1:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 9);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq scaler\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_VRA:
		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 10);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq vra \n");
			return -EINVAL;
		}
		break;
	case DEV_HW_DCP:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 11);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq dcp \n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 12);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq dcp \n");
			return -EINVAL;
		}
		break;
	default:
		probe_err("hw_id(%d) is invalid", hw_id);
		return -EINVAL;
		break;
	}

	return ret;
}

//#define DECLARE_FUNC_NAME(NUM, NAME) fimc_is_isr##NUM_##NAME
static inline int __fimc_is_hw_request_irq(struct fimc_is_interface_hwip *itf_hwip,
	const char *name,
	int isr_num,
	irqreturn_t (*func)(int, void *))
{
	size_t name_len = 0;
	int ret = 0;

	name_len = sizeof(itf_hwip->irq_name[isr_num]);
	snprintf(itf_hwip->irq_name[isr_num], name_len, "fimc%s-%d", name, isr_num);
	ret = request_irq(itf_hwip->irq[isr_num], func,
		FIMC_IS_HW_IRQ_FLAG,
		itf_hwip->irq_name[isr_num],
		itf_hwip);
	if (ret) {
		err_itfc("[HW:%s] request_irq [%d] fail", name, isr_num);
		return -EINVAL;
	}
	itf_hwip->handler[isr_num].id = isr_num;
	itf_hwip->handler[isr_num].valid = true;

	return ret;
}

int fimc_is_hw_request_irq(void *itfc_data, int hw_id)
{
	struct fimc_is_interface_hwip *itf_hwip = NULL;
	int ret = 0;

	FIMC_BUG(!itfc_data);


	itf_hwip = (struct fimc_is_interface_hwip *)itfc_data;

	switch (hw_id) {
	case DEV_HW_3AA0:
		ret = __fimc_is_hw_request_irq(itf_hwip, "3aa0", INTR_HWIP1, fimc_is_isr1_3aa0);
		ret = __fimc_is_hw_request_irq(itf_hwip, "3aa0", INTR_HWIP2, fimc_is_isr2_3aa0);
		break;
	case DEV_HW_3AA1:
		ret = __fimc_is_hw_request_irq(itf_hwip, "3aa1", INTR_HWIP1, fimc_is_isr1_3aa1);
		ret = __fimc_is_hw_request_irq(itf_hwip, "3aa1", INTR_HWIP2, fimc_is_isr2_3aa1);
		break;
	case DEV_HW_ISP0:
		ret = __fimc_is_hw_request_irq(itf_hwip, "isp0", INTR_HWIP1, fimc_is_isr1_isp0);
		ret = __fimc_is_hw_request_irq(itf_hwip, "isp0", INTR_HWIP2, fimc_is_isr2_isp0);
		break;
	case DEV_HW_ISP1:
		ret = __fimc_is_hw_request_irq(itf_hwip, "isp1", INTR_HWIP1, fimc_is_isr1_isp1);
		ret = __fimc_is_hw_request_irq(itf_hwip, "isp1", INTR_HWIP2, fimc_is_isr2_isp1);
		break;
	case DEV_HW_MCSC0:
		ret = __fimc_is_hw_request_irq(itf_hwip, "mcs0", INTR_HWIP1, fimc_is_isr1_mcs0);
		break;
	case DEV_HW_MCSC1:
		ret = __fimc_is_hw_request_irq(itf_hwip, "mcs1", INTR_HWIP1, fimc_is_isr1_mcs1);
		break;
	case DEV_HW_VRA:
		ret = __fimc_is_hw_request_irq(itf_hwip, "vra", INTR_HWIP2, fimc_is_isr1_vra); /* VRA CH1 */
		break;
	case DEV_HW_DCP:
		ret = __fimc_is_hw_request_irq(itf_hwip, "dcp", INTR_HWIP1, fimc_is_isr1_dcp);
		ret = __fimc_is_hw_request_irq(itf_hwip, "dcp", INTR_HWIP2, fimc_is_isr2_dcp);
		break;
	default:
		probe_err("hw_id(%d) is invalid", hw_id);
		return -EINVAL;
		break;
	}

	return ret;
}

void set_hw_mcsc_input(int hw_id, u32 mode)
{
	u32 isplp_val = 0;
	u32 isplp_backup = 0;
	u32 input_mcsc0, input_mcsc1;
	void __iomem *isplp_regs;

	isplp_regs = ioremap_nocache(SYSREG_ISPLP_BASE_ADDR, 0x1000);

	isplp_val = fimc_is_hw_get_reg(isplp_regs, &sysreg_isplp_regs[SYSREG_ISPLP_R_USER_CON]);
	isplp_backup = isplp_val;
	input_mcsc0 = (isplp_val >> 16) & 0x00000003;
	input_mcsc1 = (isplp_val >> 18) & 0x00000003;

	/*
	 *  MCSC SRC0 Input Select[19:18]
	 *    ISPHQ -> MCSC0 : 1
	 *    DCF -> MCSC0 : 3
	 *
	 *  MCSC SRC1 Input Select[17:16]
	 *    ISPLP -> MCSC1 : 1
	 *    DCF -> MCSC1 : 3
	 */
	switch (hw_id) {
	case GROUP_ID_MCS0:
		input_mcsc0 = mode;
		if (input_mcsc0 == MCSC_INPUT_FROM_DCP_PATH && input_mcsc1 == MCSC_INPUT_FROM_DCP_PATH) {
			input_mcsc1 = MCSC_INPUT_FROM_ISP_PATH;
			isplp_val = fimc_is_hw_set_field_value(isplp_val,
					&sysreg_isplp_fields[SYSREG_ISPLP_F_GLUEMUX_MCSC_SC1_VAL], input_mcsc1);

			info_itfc("DCF -> MCSC0, ISP -> MCSC1\n");
		}

		isplp_val = fimc_is_hw_set_field_value(isplp_val,
				&sysreg_isplp_fields[SYSREG_ISPLP_F_GLUEMUX_MCSC_SC0_VAL], input_mcsc0);
		break;
	case GROUP_ID_MCS1:
		input_mcsc1 = mode;
		if (input_mcsc1 == MCSC_INPUT_FROM_DCP_PATH && input_mcsc0 == MCSC_INPUT_FROM_DCP_PATH) {
			input_mcsc0 = MCSC_INPUT_FROM_ISP_PATH;
			isplp_val = fimc_is_hw_set_field_value(isplp_val,
				&sysreg_isplp_fields[SYSREG_ISPLP_F_GLUEMUX_MCSC_SC0_VAL], input_mcsc0);

			info_itfc("ISP -> MCSC0, DCF -> MCSC1\n");
		}
		isplp_val = fimc_is_hw_set_field_value(isplp_val,
				&sysreg_isplp_fields[SYSREG_ISPLP_F_GLUEMUX_MCSC_SC1_VAL], input_mcsc1);
		break;
	default:
		break;
	}

	if (isplp_backup != isplp_val) {
		info_itfc("%s: SYSREG_isplp_R_USER_CON:(0x%08X)->(0x%08X), mode(%d)\n",
			__func__, isplp_backup, isplp_val, mode);
		fimc_is_hw_set_reg(isplp_regs, &sysreg_isplp_regs[SYSREG_ISPLP_R_USER_CON], isplp_val);	/* MCSC input */
	}

	iounmap(isplp_regs);
}

int fimc_is_hw_s_ctrl(void *itfc_data, int hw_id, enum hw_s_ctrl_id id, void *val)
{
	int ret = 0;

	switch (id) {
	case HW_S_CTRL_FULL_BYPASS:
		break;
	case HW_S_CTRL_CHAIN_IRQ:
		break;
	case HW_S_CTRL_HWFC_IDX_RESET:
		if (hw_id == FIMC_IS_VIDEO_M3P_NUM) {
			struct fimc_is_video_ctx *vctx = (struct fimc_is_video_ctx *)itfc_data;
			struct fimc_is_device_ischain *device;
			unsigned long data = (unsigned long)val;

			FIMC_BUG(!vctx);
			FIMC_BUG(!GET_DEVICE(vctx));

			device = GET_DEVICE(vctx);

			/* reset if this instance is reprocessing */
			if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
				writel(data, hwfc_rst);
		}
		break;
	case HW_S_CTRL_MCSC_SET_INPUT:
		{
			unsigned long mode = (unsigned long)val;

			set_hw_mcsc_input(hw_id, mode);
		}
		break;
	default:
		break;
	}

	return ret;
}

int fimc_is_hw_g_ctrl(void *itfc_data, int hw_id, enum hw_g_ctrl_id id, void *val)
{
	int ret = 0;

	switch (id) {
	case HW_G_CTRL_FRM_DONE_WITH_DMA:
		*(bool *)val = true;
		break;
	case HW_G_CTRL_HAS_MCSC:
		*(bool *)val = true;
		break;
	case HW_G_CTRL_HAS_VRA_CH1_ONLY:
		*(bool *)val = true;
		break;
	}

	return ret;
}

int fimc_is_hw_query_cap(void *cap_data, int hw_id)
{
	int ret = 0;

	FIMC_BUG(!cap_data);

	switch (hw_id) {
	case DEV_HW_MCSC0:
	case DEV_HW_MCSC1:
		{
			struct fimc_is_hw_mcsc_cap *cap = (struct fimc_is_hw_mcsc_cap *)cap_data;
			/* v4.00 */
			cap->hw_ver = HW_SET_VERSION(4, 0, 0, 0);
			cap->max_output = 6;
			cap->hwfc = MCSC_CAP_SUPPORT;
			cap->in_otf = MCSC_CAP_SUPPORT;
			cap->in_dma = MCSC_CAP_SUPPORT;
			cap->out_dma[0] = MCSC_CAP_SUPPORT;
			cap->out_dma[1] = MCSC_CAP_SUPPORT;
			cap->out_dma[2] = MCSC_CAP_SUPPORT;
			cap->out_dma[3] = MCSC_CAP_SUPPORT;
			cap->out_dma[4] = MCSC_CAP_SUPPORT;
			cap->out_dma[5] = MCSC_CAP_SUPPORT;
			cap->out_otf[0] = MCSC_CAP_NOT_SUPPORT;
			cap->out_otf[1] = MCSC_CAP_NOT_SUPPORT;
			cap->out_otf[2] = MCSC_CAP_NOT_SUPPORT;
			cap->out_otf[3] = MCSC_CAP_NOT_SUPPORT;
			cap->out_otf[4] = MCSC_CAP_NOT_SUPPORT;
			cap->out_otf[5] = MCSC_CAP_NOT_SUPPORT;
			cap->out_hwfc[3] = MCSC_CAP_SUPPORT;
			cap->out_hwfc[4] = MCSC_CAP_SUPPORT;
			cap->enable_shared_output = true;
			cap->tdnr = MCSC_CAP_SUPPORT;
			cap->djag = MCSC_CAP_SUPPORT;
			cap->ysum = MCSC_CAP_SUPPORT;
			cap->ds_vra = MCSC_CAP_SUPPORT;
		}
		break;
	default:
		break;
	}

	return ret;
}

void __iomem *fimc_is_hw_get_sysreg(ulong core_regs)
{
	if (core_regs)
		err_itfc("%s: core_regs(%p)\n", __func__, (void *)core_regs);

	return ioremap_nocache(SYSREG_ISPPRE_BASE_ADDR, 0x10000);
}

u32 fimc_is_hw_find_settle(u32 mipi_speed)
{
	u32 align_mipi_speed;
	u32 find_mipi_speed;
	size_t max;
	int s, e, m;

	align_mipi_speed = ALIGN(mipi_speed, 10);
	max = sizeof(fimc_is_csi_settle_table) / sizeof(u32);

	s = 0;
	e = max - 2;

	if (fimc_is_csi_settle_table[s] < align_mipi_speed)
		return fimc_is_csi_settle_table[s + 1];

	if (fimc_is_csi_settle_table[e] > align_mipi_speed)
		return fimc_is_csi_settle_table[e + 1];

	/* Binary search */
	while (s <= e) {
		m = ALIGN((s + e) / 2, 2);
		find_mipi_speed = fimc_is_csi_settle_table[m];

		if (find_mipi_speed == align_mipi_speed)
			break;
		else if (find_mipi_speed > align_mipi_speed)
			s = m + 2;
		else
			e = m - 2;
	}

	return fimc_is_csi_settle_table[m + 1];
}

unsigned int get_dma(struct fimc_is_device_sensor *device, u32 *dma_ch)
{
	struct fimc_is_core *core;
	u32 open_sensor_count = 0;
	u32 position;
	int i;
	int ret = 0;

	*dma_ch = 0;
	core = device->private_data;
	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		if (test_bit(FIMC_IS_SENSOR_OPEN, &(core->sensor[i].state))) {
			open_sensor_count++;
			position = device->position;
			switch (position) {
			case SENSOR_POSITION_REAR:
				*dma_ch |= 1 << 0;
				*dma_ch |= 1 << 4;
				break;
			case SENSOR_POSITION_FRONT:
				*dma_ch |= 1 << 1;
				break;
			case SENSOR_POSITION_REAR2:
				*dma_ch |= 1 << 2;
				break;
			case SENSOR_POSITION_SECURE:
				*dma_ch |= 1 << 3;
				break;
			default:
				err("invalid sensor(%d)", position);
				ret = -EINVAL;
				goto p_err;
			}
		}
	}
	if (open_sensor_count != 1 && open_sensor_count != 2) {
		err("invalid open sensor limit(%d)", open_sensor_count);
		ret = -EINVAL;
	}
p_err:
	return ret;
}

void fimc_is_hw_cip_clk_enable(bool enable)
{
	/* Only for EVT0 */
	/* in order to access the C2SYNC SFR, CIP ip_processing bit should be set to 1. */
	writel(enable, reg_cip1_clk);
	writel(enable, reg_cip2_clk);
}

void fimc_is_hw_c2sync_ring_clock(enum c2sync_type type, bool enable)
{
	u32 __iomem *base_reg;
	struct fimc_is_reg *reg;
	struct fimc_is_field *field;

	if (type == OOF) {
		base_reg = reg_c2sync_1slv;
		reg = &c2sync_m0_s1_regs[C2SYNC_R_M0_S1_TRS_C2COM_RING_CLK_EN];
		field = &c2sync_m0_s1_fields[C2SYNC_F_M0_S1_TRS_C2COM_RING_CLK_EN];
	} else {
		base_reg = reg_c2sync_2slv;
		reg = &c2sync_m0_s2_regs[C2SYNC_R_M0_S2_TRS_C2COM_RING_CLK_EN];
		field = &c2sync_m0_s2_fields[C2SYNC_F_M0_S2_TRS_C2COM_RING_CLK_EN];
	}

	/* ring clock enable */
	fimc_is_hw_set_field(base_reg, reg, field, enable);

}

void fimc_is_hw_c2sync_enable(u32 __iomem *base_reg,
	enum c2sync_type type,
	int token_line,
	int image_height,
	bool enable)
{
	if (type == OOF) {
		base_reg = reg_c2sync_1slv;
		/* size config: line_in_token, limit, image height */
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s1_regs[C2SYNC_R_M0_S1_TRS_LIMIT0],
			&c2sync_m0_s1_fields[C2SYNC_F_M0_S1_TRS_LIMIT0], image_height / 2);
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s1_regs[C2SYNC_R_M0_S1_TRS_CROP_ENABLE0],
			&c2sync_m0_s1_fields[C2SYNC_F_M0_S1_TRS_CROP_ENABLE0], 0);
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s1_regs[C2SYNC_R_M0_S1_TRS_TOKENS_CROP_START0],
			&c2sync_m0_s1_fields[C2SYNC_F_M0_S1_TRS_TOKENS_CROP_START0], 0);
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s1_regs[C2SYNC_R_M0_S1_TRS_LINES_IN_FIRST_TOKEN0],
			&c2sync_m0_s1_fields[C2SYNC_F_M0_S1_TRS_LINES_IN_FIRST_TOKEN0], token_line);
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s1_regs[C2SYNC_R_M0_S1_TRS_LINES_IN_TOKEN0],
			&c2sync_m0_s1_fields[C2SYNC_F_M0_S1_TRS_LINES_IN_TOKEN0], token_line);
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s1_regs[C2SYNC_R_M0_S1_TRS_LINES_COUNT0],
			&c2sync_m0_s1_fields[C2SYNC_F_M0_S1_TRS_LINES_COUNT0], image_height);

		/* local IP */
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s1_regs[C2SYNC_R_M0_S1_TRS_C2COM_LOCAL_IP],
			&c2sync_m0_s1_fields[C2SYNC_F_M0_S1_TRS_C2COM_LOCAL_IP], VOTF_IP_CIP2);

		/* setA immediately settings */
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s1_regs[C2SYNC_R_M0_S1_TRS_C2COM_SEL_REGISTER_MODE],
			&c2sync_m0_s1_fields[C2SYNC_F_M0_S1_TRS_C2COM_SEL_REGISTER_MODE], 1);
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s1_regs[C2SYNC_R_M0_S1_TRS_C2COM_SEL_REGISTER],
			&c2sync_m0_s1_fields[C2SYNC_F_M0_S1_TRS_C2COM_SEL_REGISTER], 1);

		/* enable */
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s1_regs[C2SYNC_R_M0_S1_TRS_ENABLE0],
			&c2sync_m0_s1_fields[C2SYNC_F_M0_S1_TRS_ENABLE0], enable);
	} else {
		/* size config: line_in_token, limit, image height */
		/* 0 */
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s2_regs[C2SYNC_R_M0_S2_TRS_LIMIT0],
			&c2sync_m0_s2_fields[C2SYNC_F_M0_S2_TRS_LIMIT0], image_height / 2);
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s2_regs[C2SYNC_R_M0_S2_TRS_CROP_ENABLE0],
			&c2sync_m0_s2_fields[C2SYNC_F_M0_S2_TRS_CROP_ENABLE0], 0);
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s2_regs[C2SYNC_R_M0_S2_TRS_TOKENS_CROP_START0],
			&c2sync_m0_s2_fields[C2SYNC_F_M0_S2_TRS_TOKENS_CROP_START0], 0);
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s2_regs[C2SYNC_R_M0_S2_TRS_LINES_IN_FIRST_TOKEN0],
			&c2sync_m0_s2_fields[C2SYNC_F_M0_S2_TRS_LINES_IN_FIRST_TOKEN0], token_line);
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s2_regs[C2SYNC_R_M0_S2_TRS_LINES_IN_TOKEN0],
			&c2sync_m0_s2_fields[C2SYNC_F_M0_S2_TRS_LINES_IN_TOKEN0], token_line);
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s2_regs[C2SYNC_R_M0_S2_TRS_LINES_COUNT0],
			&c2sync_m0_s2_fields[C2SYNC_F_M0_S2_TRS_LINES_COUNT0], image_height);

		/* 1 */
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s2_regs[C2SYNC_R_M0_S2_TRS_LIMIT1],
			&c2sync_m0_s2_fields[C2SYNC_F_M0_S2_TRS_LIMIT1], image_height / 2);
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s2_regs[C2SYNC_R_M0_S2_TRS_CROP_ENABLE1],
			&c2sync_m0_s2_fields[C2SYNC_F_M0_S2_TRS_CROP_ENABLE1], 0);
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s2_regs[C2SYNC_R_M0_S2_TRS_TOKENS_CROP_START1],
			&c2sync_m0_s2_fields[C2SYNC_F_M0_S2_TRS_TOKENS_CROP_START1], 0);
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s2_regs[C2SYNC_R_M0_S2_TRS_LINES_IN_FIRST_TOKEN1],
			&c2sync_m0_s2_fields[C2SYNC_F_M0_S2_TRS_LINES_IN_FIRST_TOKEN1], token_line);
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s2_regs[C2SYNC_R_M0_S2_TRS_LINES_IN_TOKEN1],
			&c2sync_m0_s2_fields[C2SYNC_F_M0_S2_TRS_LINES_IN_TOKEN1], token_line);
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s2_regs[C2SYNC_R_M0_S2_TRS_LINES_COUNT1],
			&c2sync_m0_s2_fields[C2SYNC_F_M0_S2_TRS_LINES_COUNT1], image_height);

		/* local IP */
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s2_regs[C2SYNC_R_M0_S1_TRS_C2COM_LOCAL_IP],
			&c2sync_m0_s2_fields[C2SYNC_F_M0_S1_TRS_C2COM_LOCAL_IP], VOTF_IP_CIP1);

		/* setA immediately settings */
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s2_regs[C2SYNC_R_M0_S2_TRS_C2COM_SEL_REGISTER_MODE],
			&c2sync_m0_s2_fields[C2SYNC_F_M0_S2_TRS_C2COM_SEL_REGISTER_MODE], 1);
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s2_regs[C2SYNC_R_M0_S2_TRS_C2COM_SEL_REGISTER],
			&c2sync_m0_s2_fields[C2SYNC_F_M0_S2_TRS_C2COM_SEL_REGISTER], 1);

		/* enable */
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s2_regs[C2SYNC_R_M0_S2_TRS_ENABLE0],
			&c2sync_m0_s2_fields[C2SYNC_F_M0_S2_TRS_ENABLE0], enable);
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s2_regs[C2SYNC_R_M0_S2_TRS_ENABLE1],
			&c2sync_m0_s2_fields[C2SYNC_F_M0_S2_TRS_ENABLE1], enable);
	}
}

void fimc_is_hw_c2sync_sw_reset(u32 __iomem *base_reg,
	enum c2sync_type type)
{
	if (type == OOF) {
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s1_regs[C2SYNC_R_M0_S1_TRS_C2COM_SW_RESET],
			&c2sync_m0_s1_fields[C2SYNC_F_M0_S1_TRS_C2COM_SW_RESET], 1);
	} else {
		fimc_is_hw_set_field(base_reg, &c2sync_m0_s2_regs[C2SYNC_R_M0_S2_TRS_C2COM_SW_RESET],
			&c2sync_m0_s2_fields[C2SYNC_F_M0_S2_TRS_C2COM_SW_RESET], 1);
	}
}

void fimc_is_hw_djag_get_input(struct fimc_is_device_ischain *ischain, u32 *djag_in)
{
	struct fimc_is_global_param *g_param;

	if (!ischain) {
		err_hw("device is NULL");
		return;
	}

	g_param = &ischain->resourcemgr->global_param;

	dbg_hw(2, "%s:video_mode %d\n", __func__, g_param->video_mode);

	if (g_param->video_mode)
		*djag_in = MCSC_DJAG_IN_VIDEO_MODE;
	else
		*djag_in = MCSC_DJAG_IN_CAPTURE_MODE;
}

void fimc_is_hw_djag_adjust_out_size(struct fimc_is_device_ischain *ischain,
					u32 in_width, u32 in_height,
					u32 *out_width, u32 *out_height)
{
	/* Do nothing. */
}
