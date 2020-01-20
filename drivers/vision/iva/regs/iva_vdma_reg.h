/*
 * Copyright (C) 2017 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#ifndef _VDMA_REGS_H_
#define _VDMA_REGS_H_


/* base addresses of vdma register sections */
#define VDMA_GBL_BASE_ADDR		(0x00000)
#define VDMA_CH_BASE_DIST		(0x00200)
#define VDMA_CH_BASE_ADDR(ch)		((ch) * VDMA_CH_BASE_DIST + 0x200)

/* read and write channel: 0x02000 */
#define VDMA_CH15_BASE_ADDR		(VDMA_CH_BASE_ADDR(15))

/* addresses for global registers */
#define IRQ_DONE		(1)
#define IRQ_READY		(2)
#define IRQ_ERR			(3)
#define IRQ_HOST1_DONE		(4)
#define IRQ_HOST1_READY		(5)
#define IRQ_HOST1_ERR		(6)

#define VDMA_IRQ_EN_ADDR(type)		((type) * 0x10 + 0x00)
#define VDMA_IRQ_RAW_STATUS_ADDR(type)	((type) * 0x10 + 0x04)
#define VDMA_IRQ_SOFT_ADDR(type)	((type) * 0x10 + 0x08)
#define VDMA_IRQ_STATUS_ADDR(type)	((type) * 0x10 + 0x0C)

#define VDMA_IRQ_RAW_STATUS_DONE_ADDR	(VDMA_IRQ_RAW_STATUS_ADDR(IRQ_DONE))

/* addresses for channel registers */
#define VDMA_CH_CMD_ADDR		(0x00000)
#define VDMA_CH_STATUS_ADDR		(0x00004)
#define VDMA_CH_CFG0_ADDR		(0x00010)
#define VDMA_CH_CFG1_ADDR		(0x00020)
#define VDMA_CH_CFG2_ADDR		(0x00030)
#define VDMA_CH_CFG3_ADDR		(0x00034)
#define VDMA_CH_CFG4_ADDR		(0x00038)
#define VDMA_CH_CFG5_ADDR		(0x0003C)
#define VDMA_CH_CFG_PRI_ADDR		(0x00044)

/* nr : 0 ~ 5 */
#define VDMA_CH_CBWCFG_BASE		(0x00050)
#define VDMA_CH_CBWCFG_ADDR(nr)		(VDMA_CH_CBWCFG_BASE + nr * 0x4)

/* special register for each channel */
/* for CH_RO_YUV */
#define VDMA_CH_RO_YUV_P1_BUF_DST_ADDR 	(0x0028)
#define VDMA_CH_RO_YUV_P2_BUF_DST_ADDR 	(0x002C)
#define VDMA_CH_CFG6_ADDR		(0x00040)

#define VDMA_CH_RO_YUV_P1_CBWCFG2_ADDR 	(0x0078)
#define VDMA_CH_RO_YUV_P2_CBWCFG2_ADDR 	(0x007C)
#define VDMA_CH_RO_YUV_UV_CBWCFG4_ADDR 	(0x0084)
#define VDMA_CH_RO_YUV_P1_CBWCFG5_ADDR 	(0x0088)
#define VDMA_CH_RO_YUV_P2_CBWCFG5_ADDR 	(0x008C)

/* for CH_RO_SPL */
#define VDMA_CH_RO_SPL_TILE_CFG0_ADDR 	(0x0090)
#define VDMA_CH_RO_SPL_TILE_CFG1_ADDR 	(0x0094)
#define VDMA_CH_RO_SPL_TILE_CFG2_ADDR 	(0x0098)
#define VDMA_CH_RO_SPL_TILE_CFG3_ADDR 	(0x009C)

/* for CH_WO */
/* nr : 0 ~ 5 */
#define VDMA_CH_CBRCFG_BASE		(0x00070)
#define VDMA_CH_CBRCFG_ADDR(nr)		(VDMA_CH_CBRCFG_BASE + nr * 0x4)

#define VDMA_CH_WO_ROICFG0_ADDR 	(0x00B0)
#define VDMA_CH_WO_ROICFG1_ADDR 	(0x00B4)

/* for CH_WO_YUV */
#define VDMA_CH_WO_YUV_P1_BUF_SRC_ADDR 	(0x0018)
#define VDMA_CH_WO_YUV_P2_BUF_SRC_ADDR 	(0x001C)

#define VDMA_CH_WO_YUV_P1_CBRCFG2_ADDR 	(0x0098)
#define VDMA_CH_WO_YUV_P2_CBRCFG2_ADDR 	(0x009C)
#define VDMA_CH_WO_YUV_UV_CBRCFG4_ADDR 	(0x00A4)
#define VDMA_CH_WO_YUV_UV_CBRCFG5_ADDR 	(0x00A8)

/* bit masks and shifts */
#define VDMA_CH_STAUS_CH_BUSY_M		(0x01)
#define VDMA_CH_STAUS_CH_BUSY_S		(0)
#define VDMA_CH_STAUS_CH_READY_M	(0x02)
#define VDMA_CH_STAUS_CH_READY_S	(1)
#define VDMA_CH_STAUS_CH_RSTATE_M	(0xF0)
#define VDMA_CH_STAUS_CH_RSTATE_S	(4)
#define VDMA_CH_STAUS_CH_WSTATE_M	(0x1F00)
#define VDMA_CH_STAUS_CH_WSTATE_S	(8)
#define VDMA_CH_CFG4_ADDR_MODE_S	(30)
#define VDMA_CH_CFG4_ADDR_MODE_M	(0x3)
#define VDMA_CH_CFG4_TTYPE_S		(28)
#define VDMA_CH_CFG4_TTYPE_M		(0x3)
#define VDMA_CH_CFG4_BPP_S		(24)
#define VDMA_CH_CFG4_BPP_M		(0x7)
#define VDMA_CH_CMD_TR_START		(0x1)

#if defined(CONFIG_SOC_EXYNOS9820)
/* convert path vCF or AXI */
#define VDMA_VCF_TO_AXI_CFG0                    (0xA0)
#define VDMA_VCF_TO_AXI_CFG0_ENABLE_S           (16)
#define VDMA_VCF_TO_AXI_CFG0_ENABLE_M           (0x00010000)
#define VDMA_VCF_TO_AXI_CFG0_SEL_CH_S           (0)
#define VDMA_VCF_TO_AXI_CFG0_SEL_CH_M           (0x0000000F)

#define VDMA_VCF_TO_AXI_CFG1                    (0xA4)
#define VDMA_VCF_TO_AXI_CFG1_BASE_ADD_S         (19)
#define VDMA_VCF_TO_AXI_CFG1_BASE_ADD_M         (0xFFF80000)

#define VDMA_AXI_TO_VCF_CFG0                    (0xA8)
#define VDMA_AXI_TO_VCF_CFG0_ENABLE_S           (16)
#define VDMA_AXI_TO_VCF_CFG0_ENABLE_M           (0x00010000)
#define VDMA_AXI_TO_VCF_CFG0_SEL_CH_S           (0)
#define VDMA_AXI_TO_VCF_CFG0_SEL_CH_M           (0x0000000F)

#define VDMA_AXI_TO_VCF_CFG1                    (0xAC)
#define VDMA_AXI_TO_VCF_CFG1_BASE_ADD_S         (19)
#define VDMA_AXI_TO_VCF_CFG1_BASE_ADD_M         (0xFFF80000)

#define VDMA_AXI_QOS_ADDRESS                    (0x00000080)
#define VDMA_AXI_WRITE_MO_TABLE0_ADDRESS        (0x00000084)
#define VDMA_AXI_WRITE_MO_TABLE1_ADDRESS        (0x00000088)

#define VDMA_IRQ_EN_HOST1_DONE_ADDRESS		    (0x00040)
#define VDMA_IRQ_EN_HOST1_DONE_ADDR		        (0x00040)
#define VDMA_IRQ_RAW_STATUS_HOST1_DONE_ADDRESS	(0x00044)
#define VDMA_IRQ_RAW_STATUS_HOST1_DONE_ADDR	    (0x00044)
#define VDMA_IRQ_SOFT_HOST1_DONE_ADDRESS	    (0x00048)
#define VDMA_IRQ_STATUS_HOST1_ADDRESS		    (0x0004C)
#define VDMA_IRQ_EN_HOST1_READY_ADDRESS		    (0x00050)
#define VDMA_IRQ_EN_HOST1_READY_ADDR		    (0x00050)
#define VDMA_IRQ_RAW_STATUS_HOST1_READY_ADDRESS	(0x00054)
#define VDMA_IRQ_RAW_STATUS_HOST1_READY_ADDR	(0x00054)
#define VDMA_IRQ_SOFT_HOST1_READY_ADDRESS	    (0x00058)
#define VDMA_IRQ_STATUS_HOST1_READY_ADDRESS	    (0x0005C)
#define VDMA_IRQ_EN_HOST1_ERR_ADDRESS		    (0x00060)
#define VDMA_IRQ_EN_HOST1_ERR_ADDR		        (0x00060)
#define VDMA_IRQ_RAW_STATUS_HOST1_ERR_ADDRESS	(0x00064)
#define VDMA_IRQ_RAW_STATUS_HOST1_ERR_ADDR	    (0x00064)
#define VDMA_IRQ_SOFT_HOST1_ERR_ADDRESS		    (0x00068)
#define VDMA_IRQ_STATUS_HOST1_ERR_ADDRESS	    (0x0006C)
#endif // #if defined(CONFIG_SOC_EXYNOS9820)
#endif /* _VDMA_REGS_H_ */
