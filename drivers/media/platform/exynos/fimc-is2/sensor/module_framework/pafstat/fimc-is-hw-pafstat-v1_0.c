/*
 * Samsung Exynos SoC series FIMC-IS2 driver
 *
 * exynos fimc-is2 hw csi control functions
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-pafstat.h"
#include "fimc-is-hw-pafstat-v1_0.h"
#include "fimc-is-device-sensor.h"

u32 pafstat_hw_g_reg_cnt(void)
{
	return PAFSTAT_REG_CNT;
}

void pafstat_hw_g_floating_size(u32 *width, u32 *height, u32 *element)
{
	*width = PAFSTAT_FLOATING_MAXWIDTH;
	*height = PAFSTAT_FLOATING_MAXHEIGHT;
	*element = PAFSTAT_FLOATING_ELEMENT_SIZE;
}

void pafstat_hw_g_static_size(u32 *width, u32 *height, u32 *element)
{
	*width = PAFSTAT_STATIC_MAXWIDTH;
	*height = PAFSTAT_STATIC_MAXHEIGHT;
	*element = PAFSTAT_STATIC_ELEMENT_SIZE;
}

u32 pafstat_hw_com_g_version(void __iomem *base_reg)
{
	return fimc_is_hw_get_field(base_reg, &pafstat_regs[PAFSTAT_R_COM_VERSION],
			&pafstat_fields[PAFSTAT_F_COM_VERSION]);
}

u32 pafstat_hw_g_irq_src(void __iomem *base_reg)
{
	return fimc_is_hw_get_reg(base_reg, &pafstat_regs[PAFSTAT_R_CTX_INTERRUPT_SRC]);
}

void pafstat_hw_s_irq_src(void __iomem *base_reg, u32 status)
{
	fimc_is_hw_set_reg(base_reg, &pafstat_regs[PAFSTAT_R_CTX_INTERRUPT_SRC], status);
}

u32 pafstat_hw_g_irq_mask(void __iomem *base_reg)
{
	return fimc_is_hw_get_reg(base_reg, &pafstat_regs[PAFSTAT_R_CTX_INTERRUPT_MASK]);
}

void pafstat_hw_s_irq_mask(void __iomem *base_reg, u32 mask)
{
	fimc_is_hw_set_reg(base_reg, &pafstat_regs[PAFSTAT_R_CTX_INTERRUPT_SRC], 0);
	fimc_is_hw_set_reg(base_reg, &pafstat_regs[PAFSTAT_R_CTX_INTERRUPT_MASK], mask);
}

void pafstat_hw_com_s_output_mask(void __iomem *base_reg, u32 val)
{
	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_COM_DISABLE_OUTPUTMASK],
			&pafstat_fields[PAFSTAT_F_COM_DISABLE_OUTPUTMASK], val);
}

int pafstat_hw_s_sensor_mode(void __iomem *base_reg, u32 pd_mode)
{
	u32 sensor_mode;
	u32 enable;

	switch (pd_mode) {
	case PD_MSPD:
		sensor_mode = SENSOR_MODE_MSPD;
		enable = 1;
		break;
	case PD_MOD1:
		sensor_mode = SENSOR_MODE_2PD_MODE1;
		enable = 1;
		break;
	case PD_NONE:
	case PD_MOD2:
		sensor_mode = SENSOR_MODE_2PD_MODE2;
		enable = 0;
		break;
	case PD_MOD3:
		sensor_mode = SENSOR_MODE_2PD_MODE3;
		enable = 1;
		break;
	case PD_MSPD_TAIL:
		sensor_mode = SENSOR_MODE_MSPD_TAIL;
		enable = 0;
		break;
	default:
		warn("PD MODE(%d) is invalid", pd_mode);
		sensor_mode = SENSOR_MODE_2PD_MODE2;
		enable = 0;
		break;
	}
	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_CTX_SENSOR_MODE],
			&pafstat_fields[PAFSTAT_F_CTX_SENSOR_MODE], sensor_mode);

	return enable;
}

void pafstat_hw_disable(void __iomem *base_reg)
{
	pafstat_hw_s_irq_mask(base_reg, (1 << PAFSTAT_INT_MAX) - 1);
}

void pafstat_hw_s_input_path(void __iomem *base_reg, enum pafstat_input_path input)
{
	u32 value = (input == PAFSTAT_INPUT_DMA ? 1 : 0);

	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_CTX_SEL_OTF],
			&pafstat_fields[PAFSTAT_F_CTX_SEL_OTF], input);

	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_CTX_LIC_IS_DMA],
			&pafstat_fields[PAFSTAT_F_CTX_LIC_IS_DMA], value);
}

void pafstat_hw_com_s_lic_mode(void __iomem *base_reg, u32 id,
	enum pafstat_lic_mode lic_mode, enum pafstat_input_path input)
{
	u32 offset = (input == PAFSTAT_INPUT_DMA ? 2 : 0);

	switch (lic_mode) {
	case LIC_MODE_INTERLEAVING: /* Fall through */
	case LIC_MODE_SINGLE_BUFFER:
		info("[PAFSTAT:%d] LIC_MODE(%d)\n", id, lic_mode);
		break;
	default:
		err("lic_mode(%d) not defined", lic_mode);
		lic_mode = 1;
		break;
	}
	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_COM_LIC_SEL_SINGLE_INPUT],
			&pafstat_fields[PAFSTAT_F_COM_LIC_SEL_SINGLE_INPUT], id + offset);
	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_COM_LIC_MODE],
			&pafstat_fields[PAFSTAT_F_COM_LIC_MODE], lic_mode);
	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_COM_LIC_BYPASS],
			&pafstat_fields[PAFSTAT_F_COM_LIC_BYPASS], 0); /* TODO */
}

void pafstat_hw_s_4ppc(void __iomem *base_reg, u32 mipi_speed)
{
	u32 pixel_mode = (mipi_speed > 2000 ? 1: 0); /* 0: 2PPC, 1: 4PPC */

	info("[PAFSTAT] LIC_4PPC(%d)\n", pixel_mode);
	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_CTX_LIC_4PPC],
			&pafstat_fields[PAFSTAT_F_CTX_LIC_4PPC], pixel_mode);
}

void pafstat_hw_com_s_fro(void __iomem *base_reg, u32 fro_cnt)
{
	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_COM_FRAME_NUM],
			&pafstat_fields[PAFSTAT_F_COM_FRAME_NUM], fro_cnt);
	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_COM_FRAME_NUM_STAT],
			&pafstat_fields[PAFSTAT_F_COM_FRAME_NUM_STAT], fro_cnt);
	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_COM_FRAME_NUM_MED_INT],
			&pafstat_fields[PAFSTAT_F_COM_FRAME_NUM_MED_INT], 0);
}

void pafstat_hw_s_img_size(void __iomem *base_reg, u32 width, u32 height)
{
	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_CTX_INPUT_WIDTH],
			&pafstat_fields[PAFSTAT_F_CTX_INPUT_WIDTH], width);
	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_CTX_INPUT_HEIGHT],
			&pafstat_fields[PAFSTAT_F_CTX_INPUT_HEIGHT], height);
}

void pafstat_hw_s_pd_size(void __iomem *base_reg, u32 width, u32 height)
{
	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_CTX_PD_PXL_WIDTH],
			&pafstat_fields[PAFSTAT_F_CTX_PD_PXL_WIDTH], width);
	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_CTX_PD_PXL_HEIGHT],
			&pafstat_fields[PAFSTAT_F_CTX_PD_PXL_HEIGHT], height);
}

u32 pafstat_hw_g_current(void __iomem *base_reg)
{
	return fimc_is_hw_get_field(base_reg, &pafstat_regs[PAFSTAT_R_CTX_CURRENT_USE_A],
			&pafstat_fields[PAFSTAT_F_CTX_CURRENT_USE_A]);
}

void pafstat_hw_s_ready(void __iomem *base_reg, u32 val)
{
	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_CTX_SFR_READY],
			&pafstat_fields[PAFSTAT_F_CTX_SFR_READY], val);
}

u32 pafstat_hw_g_ready(void __iomem *base_reg)
{
	return fimc_is_hw_get_field(base_reg, &pafstat_regs[PAFSTAT_R_CTX_SFR_READY],
			&pafstat_fields[PAFSTAT_F_CTX_SFR_READY]);
}

void pafstat_hw_s_enable(void __iomem *base_reg, int enable)
{
	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_CTX_ONESHOT],
			&pafstat_fields[PAFSTAT_F_CTX_ONESHOT], 0);
	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_CTX_GLOBAL_ENABLE],
			&pafstat_fields[PAFSTAT_F_CTX_GLOBAL_ENABLE], enable);
}

void pafstat_hw_com_init(void __iomem *base_reg)
{
	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_COM_QACTIVE_SET],
			&pafstat_fields[PAFSTAT_F_COM_QACTIVE_SET], 1);

	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_COM_3AA_STALL_ENABLE],
			&pafstat_fields[PAFSTAT_F_COM_3AA_STALL_ENABLE], 1);

	pafstat_hw_s_intr_mask_all_context();

	pafstat_hw_com_s_lic_mode(base_reg, 0, 0, PAFSTAT_INPUT_OTF);

	pafstat_hw_com_s_fro(base_reg, 0);
}

void pafstat_hw_s_timeout_cnt_clear(void __iomem *base_reg)
{
	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_CTX_TIMEOUT_CNT_INIT],
			&pafstat_fields[PAFSTAT_F_CTX_TIMEOUT_CNT_INIT], 1);
}

void pafstat_hw_s_intr_mask_all_context(void)
{
	void __iomem *ctx0_regs;
	void __iomem *ctx1_regs;

	ctx0_regs = ioremap_nocache(0x14440090, SZ_4);
	ctx1_regs = ioremap_nocache(0x14444090, SZ_4);

	writel(0xFFFFFFFF, ctx0_regs);
	writel(0xFFFFFFFF, ctx1_regs);

	iounmap(ctx0_regs);
	iounmap(ctx1_regs);
}

#define IS_PAFSTAT_RESET_DONE(addr, reg, field)	\
	(fimc_is_hw_get_field(addr, &pafstat_regs[reg], &pafstat_fields[field]) != 0)

int pafstat_hw_sw_reset(void __iomem *base_reg)
{
	u32 reset_count = 0;

	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_COM_RESET_REQ],
			&pafstat_fields[PAFSTAT_F_COM_RESET_REQ], 1);

	/* wait reset complete */
	do {
		reset_count++;
		if (reset_count > 10000)
			return reset_count;
	} while (IS_PAFSTAT_RESET_DONE(base_reg, PAFSTAT_R_COM_RESET_REQ, PAFSTAT_F_COM_RESET_REQ));

	return 0;
}

/* PAF RDMA */
void fimc_is_hw_paf_oneshot_enable(void __iomem *base_reg, int enable)
{
	pafstat_hw_s_ready(base_reg, 1);

	fimc_is_hw_set_field(base_reg, &pafstat_regs[PAFSTAT_R_CTX_ONESHOT],
			&pafstat_fields[PAFSTAT_F_CTX_ONESHOT], 1);
}

void fimc_is_hw_paf_common_config(void __iomem *base_reg_com, void __iomem *base_reg, u32 paf_ch, u32 width, u32 height)
{
	int enable = 0;
	enum pafstat_input_path input = PAFSTAT_INPUT_DMA;

	/* PAFSTAT core setting */
	enable = pafstat_hw_s_sensor_mode(base_reg, PD_NONE);
	pafstat_hw_s_irq_mask(base_reg, PAFSTAT_INT_MASK);

	pafstat_hw_s_img_size(base_reg, width, height);

	pafstat_hw_s_input_path(base_reg, input);

	pafstat_hw_s_timeout_cnt_clear(base_reg);
}

void fimc_is_hw_paf_rdma_reset(void __iomem *base_reg)
{
	int ret = 0;
	u32 timeout = 1000;

	fimc_is_hw_set_field(base_reg, &pafstat_rdma_regs[PAFSTAT_RDMA_R_RDMA_COM_RESET],
			&pafstat_rdma_fields[PAFSTAT_RDMA_F_COM_SW_RESET], 1);

	do {
		ret = fimc_is_hw_get_field(base_reg, &pafstat_rdma_regs[PAFSTAT_RDMA_R_RDMA_COM_RESET_STATUS],
				&pafstat_rdma_fields[PAFSTAT_RDMA_F_COM_SW_RESET_STATUS]);

		if (ret == 0x1)
			break;

		timeout--;
	} while (timeout);

	if (timeout == 0)
		err("PAFSTAT RDMA reset fail\n");
}

void fimc_is_hw_paf_rdma_config(void __iomem *base_reg, u32 hw_format, u32 bitwidth, u32 width, u32 height)
{
	enum pafstat_rdma_format dma_format = PAFSTAT_RDMA_FORMAT_12BIT_PACK_LSB_ALIGN;
	u32 stride_size;

	switch (hw_format) {
	case DMA_OUTPUT_FORMAT_BAYER_PACKED:
		if (bitwidth == DMA_OUTPUT_BIT_WIDTH_10BIT)
			dma_format = PAFSTAT_RDMA_FORMAT_10BIT_PACK;
		else if (bitwidth == DMA_OUTPUT_BIT_WIDTH_12BIT)
			dma_format = PAFSTAT_RDMA_FORMAT_12BIT_PACK_LSB_ALIGN;
		break;
	case DMA_OUTPUT_FORMAT_BAYER:
		if (bitwidth == DMA_OUTPUT_BIT_WIDTH_8BIT)
			dma_format = PAFSTAT_RDMA_FORMAT_8BIT_PACK;
		break;
	default:
		dma_format = PAFSTAT_RDMA_FORMAT_12BIT_PACK_LSB_ALIGN;
		break;
	}

	/* same as CSIS WDMA align */
	switch (dma_format) {
	case PAFSTAT_RDMA_FORMAT_8BIT_PACK:
		stride_size = round_up(width, DMA_OUTPUT_BIT_WIDTH_16BIT);
		break;
	case PAFSTAT_RDMA_FORMAT_10BIT_PACK:
	case PAFSTAT_RDMA_FORMAT_ANDROID10:
		stride_size = round_up((width * 5 / 4), DMA_OUTPUT_BIT_WIDTH_16BIT);
		break;
	case PAFSTAT_RDMA_FORMAT_12BIT_PACK_LSB_ALIGN:
	case PAFSTAT_RDMA_FORMAT_12BIT_PACK_MSB_ALIGN:
		stride_size = round_up((width * 3 / 2), DMA_OUTPUT_BIT_WIDTH_16BIT);
		break;
	case PAFSTAT_RDMA_FORMAT_16BIT_PACK_LSB_ALIGN:
		stride_size = round_up((width * 2), DMA_OUTPUT_BIT_WIDTH_16BIT);
		break;
	default:
		stride_size = round_up(width, DMA_OUTPUT_BIT_WIDTH_16BIT);
		break;
	}

	fimc_is_hw_set_field(base_reg, &pafstat_rdma_regs[PAFSTAT_RDMA_R_RDMA_I_DATA_FORMAT],
			&pafstat_rdma_fields[PAFSTAT_RDMA_F_RDMA_I_DATA_FORMAT], dma_format);

	fimc_is_hw_set_field(base_reg, &pafstat_rdma_regs[PAFSTAT_RDMA_R_RDMA_I_IMG_WIDTH],
			&pafstat_rdma_fields[PAFSTAT_RDMA_F_RDMA_I_IMG_WIDTH], width);
	fimc_is_hw_set_field(base_reg, &pafstat_rdma_regs[PAFSTAT_RDMA_R_RDMA_I_IMG_HEIGHT],
			&pafstat_rdma_fields[PAFSTAT_RDMA_F_RDMA_I_IMG_HEIGHT], height);

	fimc_is_hw_set_field(base_reg, &pafstat_rdma_regs[PAFSTAT_RDMA_R_RDMA_I_IMG_STRIDE],
			&pafstat_rdma_fields[PAFSTAT_RDMA_F_RDMA_I_IMG_STRIDE], stride_size);
}

void fimc_is_hw_paf_rdma_set_addr(void __iomem *base_reg, u32 addr)
{
	fimc_is_hw_set_field(base_reg, &pafstat_rdma_regs[PAFSTAT_RDMA_R_RDMA_I_BASE_ADDR],
			&pafstat_rdma_fields[PAFSTAT_RDMA_F_RDMA_I_BASE_ADDR], addr);
}

void fimc_is_hw_paf_rdma_enable(void __iomem *base_reg_com, void __iomem *base_reg, u32 enable)
{
	fimc_is_hw_set_field(base_reg_com, &pafstat_rdma_regs[PAFSTAT_RDMA_R_RDMA_COM_QACTIVE],
			&pafstat_rdma_fields[PAFSTAT_RDMA_F_COM_RDMA_QACTIVE], enable);
	fimc_is_hw_set_field(base_reg, &pafstat_rdma_regs[PAFSTAT_RDMA_R_RDMA_I_EN],
			&pafstat_rdma_fields[PAFSTAT_RDMA_F_RDMA_I_EN], enable);
}

void fimc_is_hw_paf_sfr_dump(void __iomem *base_reg_com, void __iomem *base_reg)
{
	info("pafstat sfr dump\n");

	info("PAFSTAT SFR DUMP : %p\n", base_reg_com);
	fimc_is_hw_dump_regs(base_reg_com, pafstat_regs, PAFSTAT_R_PAFSTAT_YEXTR_HDRMODE);

	info("PAFSTAT RDMA SFR DUMP : %p\n", base_reg);
	fimc_is_hw_dump_regs(base_reg, pafstat_rdma_regs, PAFSTAT_RDMA_REG_CNT);
}

