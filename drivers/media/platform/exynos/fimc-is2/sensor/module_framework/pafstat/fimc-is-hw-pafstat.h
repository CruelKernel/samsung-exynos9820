/*
 * Samsung Exynos SoC series FIMC-IS2 driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_PAFSTAT_H
#define FIMC_IS_HW_PAFSTAT_H

#include "fimc-is-device-sensor-peri.h"

enum pafstat_sfr_state {
	PAFSTAT_SFR_UNAPPLIED = 0,
	PAFSTAT_SFR_APPLIED,
	PAFSTAT_SFR_STATE_MAX
};

enum pafstat_reg_select {
	PAFSTAT_SEL_SET_B = 0,
	PAFSTAT_SEL_SET_A,
	PAFSTAT_SEL_SET_MAX
};

enum pafstat_lic_mode {
	LIC_MODE_INTERLEAVING = 0,
	LIC_MODE_SINGLE_BUFFER,
	LIC_MODE_MAX
};

enum pafstat_input_path {
	PAFSTAT_INPUT_DMA = 0,
	PAFSTAT_INPUT_OTF = 1,
	PAFSTAT_INPUT_MAX
};

enum pafstat_sensor_mode {
	SENSOR_MODE_MSPD = 0,
	SENSOR_MODE_2PD_MODE1,
	SENSOR_MODE_2PD_MODE2,
	SENSOR_MODE_2PD_MODE3,
	SENSOR_MODE_MSPD_TAIL
};

enum pafstat_interrupt_map {
	PAFSTAT_INT_FRAME_START = 0,
	PAFSTAT_INT_TIMEOUT,
	PAFSTAT_INT_BAYER_FRAME_END,
	PAFSTAT_INT_STAT_FRAME_END,
	PAFSTAT_INT_TOTAL_FRAME_END,
	PAFSTAT_INT_FRAME_LINE,
	PAFSTAT_INT_FRAME_FAIL,		/* DMA OVERFLOW */
	PAFSTAT_INT_LIC_BUFFER_FULL,
	PAFSTAT_INT_MAX
};

/* INT_MASK 0: means enable interrupt, 1: means disable interrupt */
#define PAFSTAT_INT_MASK	((1 << PAFSTAT_INT_TIMEOUT) \
				| (1 << PAFSTAT_INT_FRAME_LINE) \
				| (1 << PAFSTAT_INT_BAYER_FRAME_END) \
				| (1 << PAFSTAT_INT_STAT_FRAME_END))

u32 pafstat_hw_g_reg_cnt(void);
void pafstat_hw_g_floating_size(u32 *width, u32 *height, u32 *element);
void pafstat_hw_g_static_size(u32 *width, u32 *height, u32 *element);
u32 pafstat_hw_com_g_version(void __iomem *base_reg);
void pafstat_hw_com_s_output_mask(void __iomem *base_reg, u32 val);
u32 pafstat_hw_g_current(void __iomem *base_reg);
void pafstat_hw_s_ready(void __iomem *base_reg, u32 sel);
u32 pafstat_hw_g_ready(void __iomem *base_reg);
void pafstat_hw_s_enable(void __iomem *base_reg, int enable);
void pafstat_hw_s_irq_src(void __iomem *base_reg, u32 status);
u32 pafstat_hw_g_irq_src(void __iomem *base_reg);
void pafstat_hw_s_irq_mask(void __iomem *base_reg, u32 mask);
u32 pafstat_hw_g_irq_mask(void __iomem *base_reg);
int pafstat_hw_s_sensor_mode(void __iomem *base_reg, u32 pd_mode);
void pafstat_hw_com_s_lic_mode(void __iomem *base_reg, u32 id,
	enum pafstat_lic_mode lic_mode, enum pafstat_input_path input);
void pafstat_hw_com_s_fro(void __iomem *base_reg, u32 fro_cnt);
void pafstat_hw_s_4ppc(void __iomem *base_reg, u32 mipi_speed);
void pafstat_hw_s_img_size(void __iomem *base_reg, u32 width, u32 height);
void pafstat_hw_s_pd_size(void __iomem *base_reg, u32 width, u32 height);
void pafstat_hw_s_input_path(void __iomem *base_reg, enum pafstat_input_path input);
void pafstat_hw_com_init(void __iomem *base_reg);
void pafstat_hw_s_timeout_cnt_clear(void __iomem *base_reg);
void pafstat_hw_s_intr_mask_all_context(void);
int pafstat_hw_sw_reset(void __iomem *base_reg);

/* PAF RDMA */
void fimc_is_hw_paf_common_config(void __iomem *base_reg_com, void __iomem *base_reg,
	u32 paf_ch, u32 width, u32 height);
void fimc_is_hw_paf_rdma_reset(void __iomem *base_reg);
void fimc_is_hw_paf_rdma_config(void __iomem *base_reg, u32 hw_format, u32 bitwidth, u32 width, u32 height);
void fimc_is_hw_paf_rdma_set_addr(void __iomem *base_reg, u32 addr);
void fimc_is_hw_paf_rdma_enable(void __iomem *base_reg_com, void __iomem *base_reg, u32 enable);
void fimc_is_hw_paf_sfr_dump(void __iomem *base_reg_com, void __iomem *base_reg);
void fimc_is_hw_paf_oneshot_enable(void __iomem *base_reg, int enable);

#endif
