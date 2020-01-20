/*
 * Samsung Exynos SoC series FIMC-IS2 driver
 *
 * exynos fimc-is2 hw csi control functions
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
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
#if IS_ENABLED(CONFIG_EXYNOS_OTP)
#include <linux/exynos_otp.h>
#endif

#include "fimc-is-hw-api-common.h"
#include "fimc-is-config.h"
#include "fimc-is-type.h"
#include "fimc-is-regs.h"
#include "fimc-is-device-csi.h"
#include "fimc-is-hw.h"
#include "fimc-is-hw-csi-v4_0.h"

void csi_hw_phy_otp_config(u32 __iomem *base_reg, u32 instance)
{
#if IS_ENABLED(CONFIG_EXYNOS_OTP)
	int ret;
	int i;
	u16 magic_code;
	u8 type;
	u8 index_count;
	struct tune_bits *data;

	magic_code = OTP_MAGIC_MIPI_CSI0 + instance;

	ret = otp_tune_bits_parsed(magic_code, &type, &index_count, &data);
	if (ret) {
		err("otp_tune_bits_parsed is fail(%d)", ret);
		goto p_err;
	}

	for (i = 0; i < index_count; i++){
		writel(data[i].value, base_reg + (data[i].index * 4));
		info("[CSI%d]set OTP(index = 0x%X, value = 0x%X)\n", instance, data[i].index, data[i].value);
	}

p_err:
	return;
#endif
}

int csi_hw_reset(u32 __iomem *base_reg)
{
	int ret = 0;
	u32 retry = 10;
	u32 i;

	fimc_is_hw_set_field(base_reg, &csi_regs[CSIS_R_CSIS_CMN_CTRL],
			&csi_fields[CSIS_F_SW_RESET], 1);

	while (--retry) {
		udelay(10);
		if (fimc_is_hw_get_field(base_reg, &csi_regs[CSIS_R_CSIS_CMN_CTRL],
			&csi_fields[CSIS_F_SW_RESET]) != 1)
			break;
	}

	if (!retry) {
		err("reset is fail(%d)", retry);
		ret = -EINVAL;
		goto p_err;
	}

	/* disable all virtual ch dma */
	for (i = 0; i < CSI_VIRTUAL_CH_MAX; i++)
		csi_hw_s_output_dma(base_reg, i, false);

p_err:
	return ret;
}

int csi_hw_s_settle(u32 __iomem *base_reg,
	u32 settle)
{
	fimc_is_hw_set_field(base_reg, &csi_regs[CSIS_R_DPHY_CMN_CTRL],
			&csi_fields[CSIS_F_HSSETTLE], settle);
	return 0;
}

/* TODO: renamed ctrl0 -> b_dphyctrl */
int csi_hw_s_dphyctrl0(u32 __iomem *base_reg,
	u64 ctrl)
{
	fimc_is_hw_set_field(base_reg, &csi_regs[CSIS_R_DPHY_BCTRL_L],
			&csi_fields[CSIS_F_B_DPHYCTRL_L], (ctrl << 0));
	fimc_is_hw_set_field(base_reg, &csi_regs[CSIS_R_DPHY_BCTRL_H],
			&csi_fields[CSIS_F_B_DPHYCTRL_H], (ctrl << 16));

	return 0;
}

/* TODO: renamed ctrl1 -> s_dphyctrl */
int csi_hw_s_dphyctrl1(u32 __iomem *base_reg,
	u64 ctrl)
{
	fimc_is_hw_set_field(base_reg, &csi_regs[CSIS_R_DPHY_SCTRL_L],
			&csi_fields[CSIS_F_S_DPHYCTRL_L], (ctrl << 0));
	fimc_is_hw_set_field(base_reg, &csi_regs[CSIS_R_DPHY_SCTRL_H],
			&csi_fields[CSIS_F_S_DPHYCTRL_H], (ctrl << 16));

	return 0;
}

int csi_hw_s_lane(u32 __iomem *base_reg,
	struct fimc_is_image *image, u32 lanes, u32 mipi_speed)
{
	bool deskew = false;
	u32 calc_lane_speed = 0;
	u32 bit_width = 0;
	u32 lane;
	u32 width, height, fps, pixelformat;
	u32 val = fimc_is_hw_get_reg(base_reg, &csi_regs[CSIS_R_CSIS_CMN_CTRL]);

	width = image->window.width;
	height = image->window.height;
	bit_width = image->format.bitwidth;
	fps = image->framerate;
	pixelformat = image->format.pixelformat;

	/* lane number */
	if (CSIS_V4_2 == fimc_is_hw_get_reg(base_reg, &csi_regs[CSIS_R_CSIS_VERSION]))
		/* MIPI CSI V4.2 support only 2 or 4 lane */
		val = fimc_is_hw_set_field_value(val, &csi_fields[CSIS_F_LANE_NUMBER], lanes | (1 << 0));
	else
		val = fimc_is_hw_set_field_value(val, &csi_fields[CSIS_F_LANE_NUMBER], lanes);

	/* deskew enable (only over than 1.5Gbps) */
	if (mipi_speed > 1500) {
		u32 dphy_val = 0;
		deskew = true;

		/*
		 * 1. D-phy Slave S_DPHYCTL[63:0] setting
		 *   [33]    = 0b1       / Skew Calibration Enable
		 *   [39:34] = 0b10_0100 / RX Skew Calibration Max Code Control.
		 *   [63:60] = 0b11      / RX Skew Calibration Compare-run time Control
		 */
		dphy_val = fimc_is_hw_get_reg(base_reg, &csi_regs[CSIS_R_DPHY_SCTRL_H]);
		dphy_val &= ~(0xF00000FE);
		dphy_val |= ((1 << 1) | (0x24 << 2) | (0x3 << 28));
		fimc_is_hw_set_reg(base_reg, &csi_regs[CSIS_R_DPHY_SCTRL_H], dphy_val);

		/*
		 * 2. D-phy Slave byte clock control register enable
		 */
		dphy_val = fimc_is_hw_get_reg(base_reg, &csi_regs[CSIS_R_DPHY_CMN_CTRL]);
		dphy_val = fimc_is_hw_set_field_value(dphy_val, &csi_fields[CSIS_F_S_BYTE_CLK_ENABLE], 1);
		fimc_is_hw_set_reg(base_reg, &csi_regs[CSIS_R_DPHY_CMN_CTRL], dphy_val);
	}

	val = fimc_is_hw_set_field_value(val, &csi_fields[CSIS_F_DESKEW_ENABLE], deskew);
	fimc_is_hw_set_reg(base_reg, &csi_regs[CSIS_R_CSIS_CMN_CTRL], val);

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

	fimc_is_hw_set_field(base_reg, &csi_regs[CSIS_R_DPHY_CMN_CTRL],
			&csi_fields[CSIS_F_ENABLE_DAT], lane);

	/* just for reference */
	calc_lane_speed = CSI_GET_LANE_SPEED(width, height, fps, bit_width, (lanes + 1), 15);

	info("[CSI] (%uX%u@%u/%ulane:%ubit/deskew(%d) %s%uMbps)\n",
			width, height, fps, lanes + 1, bit_width, deskew,
			deskew ? "" : "aproximately ",
			deskew ? mipi_speed : calc_lane_speed);

	return 0;
}

int csi_hw_s_control(u32 __iomem *base_reg, u32 id, u32 value)
{
	switch(id) {
	case CSIS_CTRL_INTERLEAVE_MODE:
		/* interleave mode */
		fimc_is_hw_set_field(base_reg, &csi_regs[CSIS_R_CSIS_CMN_CTRL],
				&csi_fields[CSIS_F_INTERLEAVE_MODE], value);
		break;
	case CSIS_CTRL_LINE_RATIO:
		/* line irq ratio */
		fimc_is_hw_set_field(base_reg, &csi_regs[CSIS_R_LINE_INTR_CH0],
				&csi_fields[CSIS_F_LINE_INTR_CH_N], value);
		break;
	case CSIS_CTRL_DMA_ABORT_REQ:
		/* dma abort req */
		fimc_is_hw_set_field(base_reg, &csi_regs[CSIS_R_DMA_CMN_CTRL],
				&csi_fields[CSIS_F_DMA_ABORT_REQ], value);
		break;
	case CSIS_CTRL_ENABLE_LINE_IRQ:
		fimc_is_hw_set_field(base_reg, &csi_regs[CSIS_R_CSIS_INT_MSK1],
				&csi_fields[CSIS_F_MSK_LINE_END], value);
		break;
	default:
		err("control id is invalid(%d)", id);
		break;
	}

	return 0;
}

int csi_hw_s_config(u32 __iomem *base_reg,
	u32 channel, struct fimc_is_vci_config *config, u32 width, u32 height)
{
	int ret = 0;
	u32 val, parallel;

	if (channel > CSI_VIRTUAL_CH_3) {
		err("invalid channel(%d)", channel);
		ret = -EINVAL;
		goto p_err;
	}

	if ((config->hwformat == HW_FORMAT_YUV420_8BIT) ||
		(config->hwformat == HW_FORMAT_YUV422_8BIT))
		parallel = 1;
	else
		parallel = 0;

	val = fimc_is_hw_get_reg(base_reg, &csi_regs[CSIS_R_ISP_CONFIG_CH0 + (channel * 3)]);
	val = fimc_is_hw_set_field_value(val, &csi_fields[CSIS_F_VIRTUAL_CHANNEL], config->map);
	val = fimc_is_hw_set_field_value(val, &csi_fields[CSIS_F_DATAFORMAT], config->hwformat);
	val = fimc_is_hw_set_field_value(val, &csi_fields[CSIS_F_PARALLEL], parallel);
	val = fimc_is_hw_set_field_value(val, &csi_fields[CSIS_F_DOUBLE_CMPNT], CSIS_REG_DOUBLE_CLK_EN);
	fimc_is_hw_set_reg(base_reg, &csi_regs[CSIS_R_ISP_CONFIG_CH0 + (channel * 3)], val);

	val = fimc_is_hw_get_reg(base_reg, &csi_regs[CSIS_R_ISP_RESOL_CH0 + (channel * 3)]);
	val = fimc_is_hw_set_field_value(val, &csi_fields[CSIS_F_VRESOL], height);
	val = fimc_is_hw_set_field_value(val, &csi_fields[CSIS_F_HRESOL], width);
	fimc_is_hw_set_reg(base_reg, &csi_regs[CSIS_R_ISP_RESOL_CH0 + (channel * 3)], val);

p_err:
	return ret;
}

int csi_hw_s_config_dma(u32 __iomem *base_reg, u32 channel, struct fimc_is_image *image, u32 hwformat)
{
	int ret = 0;
	u32 val;
	u32 dma_dim = 0;
	u32 dma_pack12 = 0;

	if (channel > CSI_VIRTUAL_CH_3) {
		err("invalid channel(%d)", channel);
		ret = -EINVAL;
		goto p_err;
	}

	if (image->format.pixelformat == V4L2_PIX_FMT_SBGGR10 ||
		image->format.pixelformat == V4L2_PIX_FMT_SBGGR12 ||
		image->format.pixelformat == V4L2_PIX_FMT_PRIV_MAGIC)
		dma_pack12 = CSIS_REG_DMA_PACK12;
	else
		dma_pack12 = CSIS_REG_DMA_NORMAL;

	switch (image->format.pixelformat) {
	case V4L2_PIX_FMT_SGRBG8:
	case V4L2_PIX_FMT_SBGGR8:
		dma_dim = CSIS_REG_DMA_1D_DMA;
		break;
	default:
		dma_dim = CSIS_REG_DMA_2D_DMA;
		break;
	}

	val = fimc_is_hw_get_reg(base_reg, &csi_regs[CSIS_R_DMA0_FMT + (channel * 15)]);
	val = fimc_is_hw_set_field_value(val, &csi_fields[CSIS_F_DMA_N_DIM], dma_dim);
	val = fimc_is_hw_set_field_value(val, &csi_fields[CSIS_F_DMA_N_PACK12], dma_pack12);
	fimc_is_hw_set_reg(base_reg, &csi_regs[CSIS_R_DMA0_FMT + (channel * 15)], val);

p_err:
	return ret;
}

int csi_hw_s_irq_msk(u32 __iomem *base_reg, bool on)
{
	u32 otf_msk, dma_msk;

	/* default setting */
	if (on) {
		/* base interrupt setting */
		otf_msk = CSIS_IRQ_MASK0;
		dma_msk = CSIS_IRQ_MASK1;
#if defined(SUPPORTED_EARLYBUF_DONE_SW) || defined(SUPPORTED_EARLYBUF_DONE_HW)
#if defined(SUPPORTED_EARLYBUF_DONE_HW)
		dma_msk = fimc_is_hw_set_field_value(dma_msk, &csi_fields[CSIS_F_MSK_LINE_END], 0x1);
#endif
		otf_msk = fimc_is_hw_set_field_value(otf_msk, &csi_fields[CSIS_F_FRAMEEND], 0x0);
#endif
	} else {
		otf_msk = 0;
		dma_msk = 0;
	}

	fimc_is_hw_set_reg(base_reg, &csi_regs[CSIS_R_CSIS_INT_MSK0], otf_msk);
	fimc_is_hw_set_reg(base_reg, &csi_regs[CSIS_R_CSIS_INT_MSK1], dma_msk);

	return 0;
}

static void csi_hw_g_err_types_from_err(u32 val, u32 *err_id)
{
	int i = 0;
	u32 sot_hs_err = 0;
	u32 lost_fs_err = 0;
	u32 lost_fe_err = 0;
	u32 ovf_err = 0;
	u32 wrong_cfg_err = 0;
	u32 err_ecc_err = 0;
	u32 crc_err = 0;
	u32 err_id_err = 0;

	sot_hs_err    = fimc_is_hw_get_field_value(val, &csi_fields[CSIS_F_ERR_SOT_HS   ]);
	lost_fs_err   = fimc_is_hw_get_field_value(val, &csi_fields[CSIS_F_ERR_LOST_FS  ]);
	lost_fe_err   = fimc_is_hw_get_field_value(val, &csi_fields[CSIS_F_ERR_LOST_FE  ]);
	ovf_err       = fimc_is_hw_get_field_value(val, &csi_fields[CSIS_F_ERR_OVER     ]);
	wrong_cfg_err = fimc_is_hw_get_field_value(val, &csi_fields[CSIS_F_ERR_WRONG_CFG]);
	err_ecc_err   = fimc_is_hw_get_field_value(val, &csi_fields[CSIS_F_ERR_ECC      ]);
	crc_err       = fimc_is_hw_get_field_value(val, &csi_fields[CSIS_F_ERR_CRC      ]);
	err_id_err    = fimc_is_hw_get_field_value(val, &csi_fields[CSIS_F_ERR_ID       ]);

	for (i = 0; i < CSI_VIRTUAL_CH_MAX; i++) {
		err_id[i] |= ((sot_hs_err    & (1 << i)) ? (1 << CSIS_ERR_SOT_VC) : 0);
		err_id[i] |= ((lost_fs_err   & (1 << i)) ? (1 << CSIS_ERR_LOST_FS_VC) : 0);
		err_id[i] |= ((lost_fe_err   & (1 << i)) ? (1 << CSIS_ERR_LOST_FE_VC) : 0);
		err_id[i] |= ((ovf_err       & (1 << i)) ? (1 << CSIS_ERR_OVERFLOW_VC) : 0);
		err_id[i] |= ((wrong_cfg_err & (1 << i)) ? (1 << CSIS_ERR_WRONG_CFG) : 0);
		err_id[i] |= ((err_ecc_err   & (1 << i)) ? (1 << CSIS_ERR_ECC) : 0);
		err_id[i] |= ((crc_err       & (1 << i)) ? (1 << CSIS_ERR_CRC) : 0);
		err_id[i] |= ((err_id_err    & (1 << i)) ? (1 << CSIS_ERR_ID) : 0);
	}
}

static void csi_hw_g_err_types_from_err_dma(u32 __iomem *base_reg, u32 val, u32 *err_id)
{
	int i;
	u32 dma_otf_overlap = 0;
	u32 dmafifo_full_err = 0;
	u32 trxfifo_full_err = 0;
	u32 bresp_err = 0;

	dma_otf_overlap = fimc_is_hw_get_field_value(val, &csi_fields[CSIS_F_DMA_OTF_OVERLAP]);

	if (fimc_is_hw_get_field_value(val,
				&csi_fields[CSIS_F_DMA_ERROR])) {
		/* if dma error occured, do read DMA_ERROR_CODE sfr */
		u32 err_cd_val = fimc_is_hw_get_reg(base_reg, &csi_regs[CSIS_R_DMA_ERR_CODE]);

		dmafifo_full_err = fimc_is_hw_get_field_value(err_cd_val, &csi_fields[CSIS_F_DMAFIFO_FULL]);
		trxfifo_full_err = fimc_is_hw_get_field_value(err_cd_val, &csi_fields[CSIS_F_TRXFIFO_FULL]);
		bresp_err = fimc_is_hw_get_field_value(err_cd_val, &csi_fields[CSIS_F_BRESP_ERROR]);
	}

	for (i = 0; i < CSI_VIRTUAL_CH_MAX; i++) {
		err_id[i] |= ((dma_otf_overlap   & (1 << i)) ? (1 << CSIS_ERR_DMA_OTF_OVERLAP_VC) : 0);
		err_id[i] |= ((dmafifo_full_err  & 1) ? (1 << CSIS_ERR_DMA_DMAFIFO_FULL) : 0);
		err_id[i] |= ((trxfifo_full_err  & 1) ? (1 << CSIS_ERR_DMA_TRXFIFO_FULL) : 0);
		err_id[i] |= ((bresp_err         & (1 << i)) ? (1 << CSIS_ERR_DMA_BRESP_VC) : 0);
	}
}

static bool csi_hw_g_value_of_err(u32 __iomem *base_reg, u32 otf_src, u32 dma_src, u32 *err_id)
{
	u32 err_src = (otf_src & CSIS_ERR_MASK0);
	bool err_flag = false;

	if (err_src) {
		csi_hw_g_err_types_from_err(err_src, err_id);
		err_flag = true;
	}

	err_src = (dma_src & CSIS_ERR_MASK1);
	if (err_src) {
		csi_hw_g_err_types_from_err_dma(base_reg, err_src, err_id);
		err_flag = true;
	}

	return err_flag;
}

int csi_hw_g_irq_src(u32 __iomem *base_reg, struct csis_irq_src *src, bool clear)
{
	u32 otf_src;
	u32 dma_src;

#ifdef CONFIG_CSIS_V4_0
	otf_src = fimc_is_hw_get_reg(base_reg, &csi_regs[CSIS_R_CSIS_INT_SRC0]);
	dma_src = fimc_is_hw_get_reg(base_reg, &csi_regs[CSIS_R_CSIS_INT_SRC1]);
#else
	otf_src = fimc_is_hw_get_reg(base_reg, &csi_regs[CSIS_R_CSIS_INT_SRC0]);
	dma_src = fimc_is_hw_get_reg(base_reg, &csi_regs[CSIS_R_CSIS_INT_SRC1]);
#endif
	if (clear) {
#ifdef CONFIG_CSIS_V4_0
		/* HACK: For dual scanario in EVT0, we should clear only DMA interrupt */
		/* fimc_is_hw_set_reg(base_reg, &csi_regs[CSIS_R_CSIS_INT_SRC0], otf_src); */
		fimc_is_hw_set_reg(base_reg, &csi_regs[CSIS_R_CSIS_INT_SRC1], dma_src);
#else
		fimc_is_hw_set_reg(base_reg, &csi_regs[CSIS_R_CSIS_INT_SRC0], otf_src);
		fimc_is_hw_set_reg(base_reg, &csi_regs[CSIS_R_CSIS_INT_SRC1], dma_src);
#endif
	}

	src->dma_start = fimc_is_hw_get_field_value(dma_src, &csi_fields[CSIS_F_MSK_DMA_FRM_START]);
	src->dma_end = fimc_is_hw_get_field_value(dma_src, &csi_fields[CSIS_F_MSK_DMA_FRM_END]);
#ifdef CONFIG_CSIS_V4_0
	/* HACK: For dual scanario in EVT0, we should use only DMA interrupt */
	src->otf_start = src->dma_start;
	src->otf_end = src->dma_end;
#else
	src->otf_start = fimc_is_hw_get_field_value(otf_src, &csi_fields[CSIS_F_FRAMESTART]);
	src->otf_end = fimc_is_hw_get_field_value(otf_src, &csi_fields[CSIS_F_FRAMEEND]);
#endif
	src->line_end = fimc_is_hw_get_field_value(dma_src, &csi_fields[CSIS_F_MSK_LINE_END]);
	src->err_flag = csi_hw_g_value_of_err(base_reg, otf_src, dma_src, (u32 *)src->err_id);

	return 0;
}

void csi_hw_s_frameptr(u32 __iomem *base_reg, u32 vc, u32 number, bool clear)
{
	u32 frame_ptr = 0;
	u32 val = fimc_is_hw_get_reg(base_reg, &csi_regs[CSIS_R_DMA0_CTRL + (vc * 15)]);
	frame_ptr = fimc_is_hw_get_field_value(val, &csi_fields[CSIS_F_DMA_N_UPDT_FRAMEPTR]);
	if (clear)
		frame_ptr |= (1 << number);
	else
		frame_ptr &= ~(1 << number);
	val = fimc_is_hw_set_field_value(val, &csi_fields[CSIS_F_DMA_N_UPDT_FRAMEPTR], frame_ptr);
	fimc_is_hw_set_reg(base_reg, &csi_regs[CSIS_R_DMA0_CTRL + (vc * 15)], val);
}

u32 csi_hw_g_frameptr(u32 __iomem *base_reg, u32 vc)
{
	u32 frame_ptr = 0;
	u32 val = fimc_is_hw_get_reg(base_reg, &csi_regs[CSIS_R_DMA0_ACT_CTRL + (vc * 15)]);
	frame_ptr = fimc_is_hw_get_field_value(val, &csi_fields[CSIS_F_ACTIVE_DMA_N_FRAMEPTR]);

	return frame_ptr;
}

void csi_hw_s_dma_addr(u32 __iomem *base_reg, u32 vc, u32 number, u32 addr)
{
	u32 i = 0;

	csi_hw_s_frameptr(base_reg, vc, number, false);

	/*
	 * SW W/R for CSIS frameptr reset problem.
	 * If CSIS interrupt hanler's action delayed due to busy system,
	 * DMA enable state still applied in CSIS. But the frameptr can't be updated..
	 * So CSIS H/W automatically will increase the frameprt from 0 to 1.
	 * 1's dma address was zero..so page fault problam can be happened.
	 */
	for (i = 0; i < 8; i++)
		fimc_is_hw_set_reg(base_reg, &csi_regs[CSIS_R_DMA0_ADDR1 + (vc * 15) + i], addr);
}

void csi_hw_s_multibuf_dma_addr(u32 __iomem *base_reg, u32 vc, u32 number, u32 addr)
{
	fimc_is_hw_set_reg(base_reg, &csi_regs[CSIS_R_DMA0_ADDR1 + (vc * 15) + number], addr);
}

void csi_hw_s_output_dma(u32 __iomem *base_reg, u32 vc, bool enable)
{
	u32 val = fimc_is_hw_get_reg(base_reg, &csi_regs[CSIS_R_DMA0_CTRL + (vc * 15)]);
	val = fimc_is_hw_set_field_value(val, &csi_fields[CSIS_F_DMA_N_DISABLE], !enable);
	val = fimc_is_hw_set_field_value(val, &csi_fields[CSIS_F_DMA_N_UPDT_PTR_EN], enable);
	fimc_is_hw_set_reg(base_reg, &csi_regs[CSIS_R_DMA0_CTRL + (vc * 15)], val);
}

bool csi_hw_g_output_dma_enable(u32 __iomem *base_reg, u32 vc)
{
	/* if DMA_DISABLE field value is 1, this means dma output is disabled */
	if (fimc_is_hw_get_field(base_reg, &csi_regs[CSIS_R_DMA0_CTRL + (vc * 15)],
			&csi_fields[CSIS_F_DMA_N_DISABLE]))
		return false;
	else
		return true;
}

bool csi_hw_g_output_cur_dma_enable(u32 __iomem *base_reg, u32 vc)
{
	u32 val = fimc_is_hw_get_reg(base_reg, &csi_regs[CSIS_R_DMA0_ACT_CTRL + (vc * 15)]);
	/* if DMA_DISABLE field value is 1, this means dma output is disabled */
	bool dma_disable = fimc_is_hw_get_field_value(val, &csi_fields[CSIS_F_ACTIVE_DMA_N_DISABLE]);

	/*
	 * HACK: active_dma_n_disable filed has reset value(0x0), it means that dma enable was default
	 * So, if frameptr was 0x7(reset value), it means that dma was disable in the vc.
	 * CSIS driver control the frameptr by one.
	 */
	if (fimc_is_hw_get_field_value(val, &csi_fields[CSIS_F_ACTIVE_DMA_N_FRAMEPTR]) == 0x7)
		dma_disable = true;

	return !dma_disable;
}

void csi_hw_set_start_addr(u32 __iomem *base_reg, u32 number, u32 addr)
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

int csi_hw_enable(u32 __iomem *base_reg)
{
	/* update shadow */
	fimc_is_hw_set_field(base_reg, &csi_regs[CSIS_R_CSIS_CMN_CTRL],
			&csi_fields[CSIS_F_UPDATE_SHADOW], 0xF);

	/* DPHY on */
	fimc_is_hw_set_field(base_reg, &csi_regs[CSIS_R_DPHY_CMN_CTRL],
			&csi_fields[CSIS_F_ENABLE_CLK], 1);

	/* csi enable */
	fimc_is_hw_set_field(base_reg, &csi_regs[CSIS_R_CSIS_CMN_CTRL],
			&csi_fields[CSIS_F_CSI_EN], 1);

	return 0;
}

int csi_hw_disable(u32 __iomem *base_reg)
{
	/* DPHY off */
	fimc_is_hw_set_field(base_reg, &csi_regs[CSIS_R_DPHY_CMN_CTRL],
			&csi_fields[CSIS_F_ENABLE_CLK], 0);
	fimc_is_hw_set_field(base_reg, &csi_regs[CSIS_R_DPHY_CMN_CTRL],
			&csi_fields[CSIS_F_ENABLE_DAT], 0);

	/* csi disable */
	fimc_is_hw_set_field(base_reg, &csi_regs[CSIS_R_CSIS_CMN_CTRL],
			&csi_fields[CSIS_F_CSI_EN], 0);

	return 0;
}

int csi_hw_dump(u32 __iomem *base_reg)
{
	info("CSI 4.0 DUMP\n");
	fimc_is_hw_dump_regs(base_reg, csi_regs, CSIS_REG_CNT);

	return 0;
}

