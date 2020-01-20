/* iva_vdma.c
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 * Authors:
 *	Ilkwan Kim <ilkwan.kim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/time.h>

#include "regs/iva_base_addr.h"
#include "regs/iva_vde_reg.h"
#include "iva_vdma.h"
#include "iva_pmu.h"

/* pixel/data types */
enum data_fmt_t {
	e_data_8b = 0,
	e_data_16b,
	e_data_32b,
	e_data_64b,
	e_data_128b,
	e_data_fmt_invalid = 255
};

enum vdma_ttype {
	vdma_ttype_ext_to_loc	= 0,
	vdma_ttype_loc_to_ext	= 1,
	vdma_ttype_loc_to_loc	= 2,
	vdma_ttype_ext_to_ext	= 3,
	vdma_ttype_max
};

enum vdma_addr_mode {
	vdma_addr_mode_2d	= 0,
	vdma_addr_mode_linear	= 1,
	vdma_addr_mode_max,
};

enum vdma_planar_mode {
	vdma_planar	= 0,
	vdma_2_planes	= 1,
	vdma_3_planes	= 2,
	vdma_planar_max,
	vdma_known	= vdma_planar_max,/* not supported */
};

enum vdma_yuv_mode {
	vdma_ym_yuyv	= 0,
	vdma_ym_yvyu	= 1,
	vdma_ym_uyvy	= 2,
	vdma_ym_vyuy	= 3,
};

#define GET_DMA_BASE_F_ID(id)	((id == vdma_id_0) ? IVA_VDMA0_BASE_ADDR :\
		IVA_VDMA1_BASE_ADDR)

#define GET_DMA_CH_BASE_F_HANDLE(hdl)	(GET_DMA_BASE_F_ID(HALDLE_TO_ID(hdl)) +\
		VDE_CH_BASE_ADDR(HALDLE_TO_CH(hdl)))

static void vdma_clear_planar_config(void *ch_base)
{
	/* Planar mode of transfer */
	writel(0x0, ch_base + VDE_CH_CFG_REG_YUV_ADDR);

	/* clear the Planar buffer configurations not to use flow-control */
	writel(0x0, ch_base + VDE_CH_P_CBWCFG_ADDR(0, 0));
	writel(0x0, ch_base + VDE_CH_P_CBRCFG_ADDR(0, 0));
	writel(0x0, ch_base + VDE_CH_P_CBWCFG_ADDR(1, 0));
	writel(0x0, ch_base + VDE_CH_P_CBRCFG_ADDR(1, 0));
	writel(0x0, ch_base + VDE_CH_P_CBWCFG_ADDR(2, 0));
	writel(0x0, ch_base + VDE_CH_P_CBRCFG_ADDR(2, 0));
}


void iva_vdma_start_cmd(struct iva_dev_data *iva, uint32_t handle)
{
	void	*ch_base = iva->iva_va + GET_DMA_CH_BASE_F_HANDLE(handle);

	/* start the DMA transfer really */
	writel(VDE_CH_START_REG_TR_START_M,
			ch_base + VDE_CH_START_REG_ADDR);
}

/* busy wait*/
void iva_vdma_wait_done(struct iva_dev_data *iva, uint32_t handle)
{
	void		*dma_base = iva->iva_va +
				GET_DMA_BASE_F_ID(HALDLE_TO_ID(handle));
	enum vdma_ch	ch = HALDLE_TO_CH(handle);
	uint32_t	val;

	do {
		val = readl(dma_base + VDE_IRQ_RAW_STATUS_ADDR);
	} while (!(val & (0x1 << ch)));		// RSTAT_DONE high = done

	/* clear */
	writel(0x1 << ch, dma_base + VDE_IRQ_RAW_STATUS_ADDR);
}


int32_t iva_vdma_config_dram_to_mcu(struct iva_dev_data *iva,
		uint32_t handle, uint32_t io_va,
		uint32_t mcu_addr, uint32_t x_size, bool start)
{
	uint32_t val;
	void	*ch_base = iva->iva_va + GET_DMA_CH_BASE_F_HANDLE(handle);

	/* config for external source */
	val = (vdma_addr_mode_linear << VDE_CH_CFG_REG_8_ADDR_MODE_S) |
		(vdma_ttype_ext_to_ext << VDE_CH_CFG_REG_8_TTYPE_S) |
		(e_data_8b << VDE_CH_CFG_REG_8_BPP_S) |
		(x_size);
	writel(val, ch_base + VDE_CH_CFG_REG_8_ADDR);
	writel(0x0, ch_base + VDE_CH_CFG_REG_6_ADDR);
	writel(0x0, ch_base + VDE_CH_CFG_REG_7_ADDR);

	/* global source and destination address in DRAM */
	writel(io_va, ch_base + VDE_CH_CFG_REG_0_ADDR);
	writel(mcu_addr + IVA_VMCU_MEM_BASE_ADDR,
			ch_base + VDE_CH_CFG_REG_2_ADDR);

	/* BCI mode of transfer (used with vDSP TCM) */
	writel(0x0, ch_base + VDE_CH_CFG_REG_BCI_ADDR);

	/* clear to planar mode w/o flow-control */
	vdma_clear_planar_config(ch_base);

	/* clear the vCF circular buffer RP/WP configuration */
	writel(0x0, ch_base + VDE_CH_CBRCFG_ADDR(0));
	writel(0x0, ch_base + VDE_CH_CBWCFG_ADDR(0));

	if (start)
		iva_vdma_start_cmd(iva, handle);
	return 0;
}

int32_t iva_vdma_enable(struct iva_dev_data *iva, enum vdma_id id, bool enable)
{
	enum iva_pmu_ip_id ip;

	switch (id) {
	case vdma_id_0:
		ip = pmu_vdma0_id;
		break;
	case vdma_id_1:
		ip = pmu_vdma1_id;
		break;
	default:
		dev_err(iva->dev, "%s() unknown vdma id(%d)\n", __func__, id);
		return -EINVAL;
	}

	if (enable) {
		iva_pmu_enable_clk(iva, ip, true);
		iva_pmu_reset_hwa(iva, ip, false);
		iva_pmu_reset_hwa(iva, ip, true);
	} else {
		iva_pmu_reset_hwa(iva, ip, false);
		iva_pmu_enable_clk(iva, ip, false);
	}

	return 0;
}

#define IVA_VDMA_PRINT(dev, ch_base, reg)				\
		dev_info(dev, #reg "(0x%x)\n", readl(ch_base + reg))

void iva_vdma_show_status(struct iva_dev_data *iva)
{
	struct device	*dev = iva->dev;
	uint32_t	vdma_hd;
	int		id, ch;
	void		*ch_base;
	uint32_t	val;
	bool		vdma_clk;

	for (id = vdma_id_0; id < vdma_id_max; id++) {
		vdma_clk = !!iva_pmu_enable_clk(iva, pmu_vdma0_id << id, true);

		for (ch = vdma_ch_0; ch < vdma_ch_max; ch++) {
			vdma_hd	= ID_CH_TO_HANDLE(id, ch);
			ch_base	= iva->iva_va + GET_DMA_CH_BASE_F_HANDLE(vdma_hd);
			val	= readl(ch_base + VDE_CH_STATUS_REG_0_ADDR);

			if (!(val & VDE_CH_STATUS_0_CH_BUSY_M))
				continue;

			dev_info(dev, "--- iva vdma id(%d), ch(%d)---\n",
					id, ch);
			IVA_VDMA_PRINT(dev, ch_base, VDE_CH_STATUS_REG_0_ADDR);
			IVA_VDMA_PRINT(dev, ch_base, VDE_CH_CFG_REG_0_ADDR);
			IVA_VDMA_PRINT(dev, ch_base, VDE_CH_CFG_REG_2_ADDR);
			IVA_VDMA_PRINT(dev, ch_base, VDE_CH_CFG_REG_4_ADDR);
			IVA_VDMA_PRINT(dev, ch_base, VDE_CH_CFG_REG_5_ADDR);
			IVA_VDMA_PRINT(dev, ch_base, VDE_CH_CFG_REG_6_ADDR);
			IVA_VDMA_PRINT(dev, ch_base, VDE_CH_CFG_REG_7_ADDR);
			IVA_VDMA_PRINT(dev, ch_base, VDE_CH_CFG_REG_8_ADDR);
		}

		if (!vdma_clk)
			iva_pmu_enable_clk(iva, pmu_vdma0_id << id, false);
	}
}
