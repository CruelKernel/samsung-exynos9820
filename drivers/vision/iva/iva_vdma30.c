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
#include "regs/iva_vdma_reg.h"
#include "iva_pmu.h"
#if defined(CONFIG_SOC_EXYNOS8895)
#include "iva_vdma.h"
#elif defined(CONFIG_SOC_EXYNOS9810)
#include "iva_vdma20.h"
#elif defined(CONFIG_SOC_EXYNOS9820)
#include "iva_vdma30.h"
#endif

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

#define GET_DMA_CH_BASE_F_HANDLE(hdl)	(IVA_VDMA0_BASE_ADDR +\
		VDMA_CH_BASE_ADDR(HALDLE_TO_CH(hdl)))

void iva_vdma_path_dram_to_mcu(struct iva_dev_data *iva, uint32_t handle,
                bool enable)
{
    void        *ch_base = iva->iva_va + GET_DMA_CH_BASE_F_HANDLE(handle);
    uint32_t	val;

    /* Set Max.
     * MO = 92 when only 1 AXI Write channel enabled:
     * This is from the HW limitation
     */
    writel(0x1f293f5c, ch_base + VDMA_AXI_WRITE_MO_TABLE1_ADDRESS);

    if(enable) {
        /* convert path from vCF to AXI */
        val = (vdma_conversion_enable << VDMA_VCF_TO_AXI_CFG0_ENABLE_S) |
            (0x0 << VDMA_VCF_TO_AXI_CFG0_SEL_CH_S);
        writel(val, ch_base + VDMA_VCF_TO_AXI_CFG0);
        writel(VDMA_TCM_BASE_ADDRESS, ch_base + VDMA_VCF_TO_AXI_CFG1);
    } else {
        /* convert path from AXI to vCF */
        val = (vdma_conversion_disable << VDMA_VCF_TO_AXI_CFG0_ENABLE_S) |
            (0x0 << VDMA_VCF_TO_AXI_CFG0_SEL_CH_S);
        writel(val, ch_base + VDMA_VCF_TO_AXI_CFG0);
    }

    return;
}

void iva_vdma_start_cmd(struct iva_dev_data *iva, uint32_t handle)
{
	void	*ch_base = iva->iva_va + GET_DMA_CH_BASE_F_HANDLE(handle);

	/* start the DMA transfer really */
	writel(VDMA_CH_CMD_TR_START, ch_base + VDMA_CH_CMD_ADDR);
}

/* busy wait*/
void iva_vdma_wait_done(struct iva_dev_data *iva, uint32_t handle)
{
	void		*dma_base = iva->iva_va + IVA_VDMA0_BASE_ADDR;
	uint32_t	ch_mask	= 0x1 << HALDLE_TO_CH(handle);
	uint32_t	val;

	do {
		val = readl(dma_base + VDMA_IRQ_RAW_STATUS_DONE_ADDR);
	} while (!(val & ch_mask));		/* RSTAT_DONE high = done */

	/* clear */
	writel(ch_mask, dma_base + VDMA_IRQ_RAW_STATUS_DONE_ADDR);
}

int32_t iva_vdma_config_dram_to_mcu(struct iva_dev_data *iva,
		uint32_t handle, uint32_t io_va,
		uint32_t mcu_addr, uint32_t x_size, bool start)
{
	uint32_t val;
	void	*ch_base = iva->iva_va + GET_DMA_CH_BASE_F_HANDLE(handle);

	/* config for external source */
	val = (vdma_addr_mode_linear << VDMA_CH_CFG4_ADDR_MODE_S) |
			(vdma_ttype_ext_to_ext << VDMA_CH_CFG4_TTYPE_S) |
			(e_data_8b << VDMA_CH_CFG4_BPP_S);
	writel(val, ch_base + VDMA_CH_CFG4_ADDR);

	/* linear transfer size, TO DO: size check*/
	writel(x_size, ch_base + VDMA_CH_CFG5_ADDR);

	/* channel interrupt enable for channel READY */
	writel(0x0, ch_base + VDMA_IRQ_EN_HOST1_READY_ADDR);

	/* global source (iova) and mcu */
	writel(io_va, ch_base + VDMA_CH_CFG0_ADDR);
	writel(mcu_addr, ch_base + VDMA_CH_CFG1_ADDR);

	if (start)
		iva_vdma_start_cmd(iva, handle);
	return 0;
}

int32_t iva_vdma_enable(struct iva_dev_data *iva, bool enable)
{
	if (enable) {
		iva_pmu_enable_clk(iva,	pmu_vdma_id, true);
		iva_pmu_reset_hwa(iva,	pmu_vdma_id, false);
		iva_pmu_reset_hwa(iva,	pmu_vdma_id, true);
	} else {
		iva_pmu_reset_hwa(iva,	pmu_vdma_id, false);
		iva_pmu_enable_clk(iva,	pmu_vdma_id, false);
	}

	return 0;
}

static const char *iva_vdma_get_irq_type_str(int32_t irq_type)
{
	switch (irq_type) {
	case IRQ_DONE:		return "DONE";
	case IRQ_READY:		return "READY";
	case IRQ_ERR:		return "ERR";
	case IRQ_HOST1_DONE:	return "HOST1_DONE";
	case IRQ_HOST1_READY:	return "HOST1_READY";
	case IRQ_HOST1_ERR:	return "HOST1_ERR";
	}

	return "unknown";
}

struct iva_vdma_dump_reg {
	const char	*name;
	const uint32_t	reg_offset;
};

struct iva_vdma_dump_regs {
	const struct iva_vdma_dump_reg	*regs;
	const uint32_t			reg_num;
};

static const struct iva_vdma_dump_reg iva_vdma_common_reg[] = {
	{ "STATUS",	VDMA_CH_STATUS_ADDR	},
	{ "CFG0",	VDMA_CH_CFG0_ADDR	},
	{ "CFG1",	VDMA_CH_CFG1_ADDR	},
	{ "CFG2",	VDMA_CH_CFG2_ADDR	},
	{ "CFG3",	VDMA_CH_CFG3_ADDR	},
	{ "CFG4",	VDMA_CH_CFG4_ADDR	},
	{ "CFG5",	VDMA_CH_CFG5_ADDR	},
	{ "PRI",	VDMA_CH_CFG_PRI_ADDR	},
};

static const struct iva_vdma_dump_reg iva_vdma_ro_reg[] = {
	{ "CBWCFG0",	VDMA_CH_CBWCFG_ADDR(0)	},
	{ "CBWCFG1",	VDMA_CH_CBWCFG_ADDR(1)	},
	{ "CBWCFG2",	VDMA_CH_CBWCFG_ADDR(2)	},
	{ "CBWCFG3",	VDMA_CH_CBWCFG_ADDR(3)	},
	{ "CBWCFG4",	VDMA_CH_CBWCFG_ADDR(4)	},
};

static const struct iva_vdma_dump_reg iva_vdma_ro_yuv_reg[] = {
	{ "P1_DST",	VDMA_CH_RO_YUV_P1_BUF_DST_ADDR	},
	{ "P2_DST",	VDMA_CH_RO_YUV_P2_BUF_DST_ADDR	},
	{ "CFG6",	VDMA_CH_CFG6_ADDR		},
	{ "P1_CBW_2",	VDMA_CH_RO_YUV_P1_CBWCFG2_ADDR	},
	{ "P2_CBW_2",	VDMA_CH_RO_YUV_P2_CBWCFG2_ADDR	},
	{ "UV_CBW_4",	VDMA_CH_RO_YUV_UV_CBWCFG4_ADDR	},
	{ "P1_CBW_5",	VDMA_CH_RO_YUV_P1_CBWCFG5_ADDR	},
	{ "P2_CBW_5",	VDMA_CH_RO_YUV_P2_CBWCFG5_ADDR	},
};

static const struct iva_vdma_dump_reg iva_vdma_ro_spl_reg[] = {
	{ "TILECFG0",	VDMA_CH_RO_SPL_TILE_CFG0_ADDR	},
	{ "TILECFG1",	VDMA_CH_RO_SPL_TILE_CFG1_ADDR	},
	{ "TILECFG2",	VDMA_CH_RO_SPL_TILE_CFG2_ADDR	},
	{ "TILECFG3",	VDMA_CH_RO_SPL_TILE_CFG3_ADDR	},
};

static const struct iva_vdma_dump_reg iva_vdma_wo_reg[] = {
	{ "CBRCFG0",	VDMA_CH_CBRCFG_ADDR(0)	},
	{ "CBRCFG1",	VDMA_CH_CBRCFG_ADDR(1)	},
	{ "CBRCFG2",	VDMA_CH_CBRCFG_ADDR(2)	},
	{ "CBRCFG3",	VDMA_CH_CBRCFG_ADDR(3)	},
	{ "CBRCFG4",	VDMA_CH_CBRCFG_ADDR(4)	},
	{ "ROICFG0",	VDMA_CH_WO_ROICFG0_ADDR	},
	{ "ROICFG1",	VDMA_CH_WO_ROICFG1_ADDR	},
};

static const struct iva_vdma_dump_reg iva_vdma_wo_yuv_reg[] = {
	{ "P1_SRC",	VDMA_CH_WO_YUV_P1_BUF_SRC_ADDR	},
	{ "P2_SRC",	VDMA_CH_WO_YUV_P2_BUF_SRC_ADDR	},
	{ "CFG6",	VDMA_CH_CFG6_ADDR		},
	{ "P1_CBR_2",	VDMA_CH_WO_YUV_P1_CBRCFG2_ADDR	},
	{ "P2_CBR_2",	VDMA_CH_WO_YUV_P2_CBRCFG2_ADDR	},
	{ "UV_CBR_4",	VDMA_CH_WO_YUV_UV_CBRCFG4_ADDR	},
	{ "UV_CBR_5",	VDMA_CH_WO_YUV_UV_CBRCFG5_ADDR	},
};

static const struct iva_vdma_dump_regs ro_list[] = {
	{ iva_vdma_common_reg,	ARRAY_SIZE(iva_vdma_common_reg)	},
	{ iva_vdma_ro_reg,	ARRAY_SIZE(iva_vdma_ro_reg)	},
	{ NULL,			0				},
};

static const struct iva_vdma_dump_regs ro_yuv_list[] = {
	{ iva_vdma_common_reg,	ARRAY_SIZE(iva_vdma_common_reg)	},
	{ iva_vdma_ro_reg,	ARRAY_SIZE(iva_vdma_ro_reg)	},
	{ iva_vdma_ro_yuv_reg,	ARRAY_SIZE(iva_vdma_ro_yuv_reg)	},
	{ NULL,			0				},
};

static const struct iva_vdma_dump_regs ro_spl_list[] = {
	{ iva_vdma_common_reg,	ARRAY_SIZE(iva_vdma_common_reg)	},
	{ iva_vdma_ro_reg,	ARRAY_SIZE(iva_vdma_ro_reg)	},
	{ iva_vdma_ro_spl_reg,	ARRAY_SIZE(iva_vdma_ro_spl_reg)	},
	{ NULL,			0				},
};

static const struct iva_vdma_dump_regs wo_list[] = {
	{ iva_vdma_common_reg,	ARRAY_SIZE(iva_vdma_common_reg)	},
	{ iva_vdma_wo_reg,	ARRAY_SIZE(iva_vdma_wo_reg)	},
	{ NULL,			0				},
};

static const struct iva_vdma_dump_regs wo_yuv_list[] = {
	{ iva_vdma_common_reg,	ARRAY_SIZE(iva_vdma_common_reg)	},
	{ iva_vdma_wo_reg,	ARRAY_SIZE(iva_vdma_wo_reg)	},
	{ iva_vdma_wo_yuv_reg,	ARRAY_SIZE(iva_vdma_wo_yuv_reg)	},
	{ NULL,			0				},
};

static const struct iva_vdma_dump_regs *ch_dump_list[vdma_ch_max] = {
	[vdma_ch0_type_ro] = ro_list,
	[vdma_ch1_type_ro] = ro_list,
	[vdma_ch2_type_ro] = ro_list,
	[vdma_ch3_type_ro] = ro_list,
	[vdma_ch4_type_ro] = ro_list,
	[vdma_ch5_type_ro_spl] = ro_spl_list,
	[vdma_ch6_type_ro_yuv] = ro_yuv_list,
	[vdma_ch7_type_ro_yuv] = ro_yuv_list,
	[vdma_ch8_type_wo] = wo_list,
	[vdma_ch9_type_wo] = wo_list,
	[vdma_ch10_type_wo] = wo_list,
	[vdma_ch11_type_wo] = wo_list,
	[vdma_ch12_type_wo] = wo_list,
	[vdma_ch13_type_wo] = wo_list,
	[vdma_ch14_type_wo_yuv] = wo_yuv_list,
	[vdma_ch15_type_wo_yuv] = wo_yuv_list,
};

static const struct iva_vdma_dump_regs *iva_vdma_get_ch_dump_regs(enum vdma_ch ch)
{
	if (ch < vdma_ch0_type_ro || ch >= vdma_ch_max)
		return NULL;

	return ch_dump_list[ch];
}


void iva_vdma_show_status(struct iva_dev_data *iva)
{
	struct device	*dev = iva->dev;
	void		*dma_base = iva->iva_va + IVA_VDMA0_BASE_ADDR;
	void		*ch_base;
	u32		val;
	int		irq_type;
	bool		vdma_sfr_clk;
	char		line_buf[128];
	char		line_buf1[128];
	int		line_str;
	int		busy_ch_cnt = 0;
	uint32_t	ch, i;
	int		line_num, line_num1;
	int		breaker;
	const struct iva_vdma_dump_regs *ch_regs;

	vdma_sfr_clk = !!iva_pmu_enable_core_clk(iva, pmu_vdma_id, true);
	if (!vdma_sfr_clk)
		dev_info(dev, "vdma sfr clock was off.\n");

	/* GBL */
	dev_info(dev, "------------- VDMA3.0 GBL ------------\n");
	dev_info(dev, "              IRQ_EN  IRQ_RAW   STATUS\n");
	for (irq_type = IRQ_DONE; irq_type <= IRQ_HOST1_ERR; irq_type++) {
		line_str = snprintf(line_buf, sizeof(line_buf),
				"%11s %08X %08X %08X\n",
				iva_vdma_get_irq_type_str(irq_type),
				readl(dma_base + VDMA_IRQ_EN_ADDR(irq_type)),
				readl(dma_base + VDMA_IRQ_RAW_STATUS_ADDR(irq_type)),
				readl(dma_base + VDMA_IRQ_STATUS_ADDR(irq_type)));
		if (line_str == sizeof(line_buf))
			line_buf[sizeof(line_buf) - 1] = 0x0;

		dev_info(dev, "%s", line_buf);
	}

	/* per CH */
	for (ch = vdma_ch0_type_ro; ch < vdma_ch_max; ch++) {
		ch_base	= iva->iva_va + GET_DMA_CH_BASE_F_HANDLE(CH_TO_HANDLE(ch));
		val = readl(ch_base + VDMA_CH_STATUS_ADDR);
		if (!(val & VDMA_CH_STAUS_CH_BUSY_M))
			continue;
		busy_ch_cnt++;
	}

	if (!busy_ch_cnt) {
		dev_info(dev, "No busy VDMA3.0 Channels found\n");
		goto out;
	}

#define PAD_NEW_LINE(str_buf, str_num)				\
	do {							\
		int new_line_offset;				\
		if (str_num < sizeof(str_buf) - 1)		\
			new_line_offset = str_num;		\
		else						\
			new_line_offset = sizeof(str_buf) - 2;	\
		str_buf[new_line_offset] = '\n';		\
		str_buf[new_line_offset + 1] = 0;		\
	} while (0)

	for (ch = vdma_ch0_type_ro; ch < vdma_ch_max; ch++) {
		ch_base = iva->iva_va + GET_DMA_CH_BASE_F_HANDLE(CH_TO_HANDLE(ch));
		val = readl(ch_base + VDMA_CH_STATUS_ADDR);
		if (!(val & VDMA_CH_STAUS_CH_BUSY_M))
			continue;

		dev_info(dev, "+- CH[%2d] -------------------------------------------------------------------------------+\n", ch);
		line_num = 0;
		line_num1 = 0;
		breaker = 0;

		ch_regs = iva_vdma_get_ch_dump_regs(ch);
		if (!ch_regs)
			continue;

		while (ch_regs->reg_num != 0) {
			for (i = 0 ; i < ch_regs->reg_num; i++) {
				const struct iva_vdma_dump_reg *reg = &ch_regs->regs[i];
				line_num += snprintf(line_buf + line_num,
						sizeof(line_buf) - line_num, " %8s",
						reg->name);
				line_num1 += snprintf(line_buf1 + line_num1,
						sizeof(line_buf1) - line_num1, " %08X",
						readl(ch_base + reg->reg_offset));

				breaker++;
				if (breaker != 0 && (breaker % 10) == 0) {
					PAD_NEW_LINE(line_buf, line_num);
					PAD_NEW_LINE(line_buf1, line_num1);
					dev_info(dev, "%s", line_buf);
					dev_info(dev, "%s", line_buf1);
					line_num = 0;
					line_num1 = 0;
				}
			}
			ch_regs++;
		}

		if (line_num > 1) {
			PAD_NEW_LINE(line_buf, line_num);
			PAD_NEW_LINE(line_buf1, line_num1);
			dev_info(dev, "%s", line_buf);
			dev_info(dev, "%s", line_buf1);
		}
	}
	dev_info(dev, "+----------------------------------------------------------------------------------------+\n");

out:
	if (!vdma_sfr_clk)
		iva_pmu_enable_core_clk(iva, pmu_vdma_id, false);

}
