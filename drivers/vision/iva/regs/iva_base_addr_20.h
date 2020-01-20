/*
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __IVA_BASE_ADDR_20_H__
#define __IVA_BASE_ADDR_20_H__

#define KB	(1024)
#define MB	(KB * KB)

/* from Zynq ARM A9 : will be described in device tree */
#define IVA_CFG_PHY_BASE_ADDR	(0xA0000000)
/* HWA - totally 2MB range*/
#define IVA_CFG_SIZE		(2 * MB)

/* System Peripherals from host : in FPGA */
#define PERI_PHY_BASE_ADDR	(0xA2000000)
#define PERI_SIZE		(16*4 * KB)

/* SCore APB from host */
#define SCORE_APB_PHY_BASE_ADDR	(0xA1000000)
#define SCORE_APB_SIZE		(512 * KB)
#define SCORE_APB_END_ADDR	(SCORE_APB_PHY_BASE_ADDR + SCORE_APB_SIZE)

#define IVA_HWA_ADDR_GAP	(0x10000)

/* offset from iva base address */
#define IVA_BAX_BASE_ADDR	(0x00000000)
#define IVA_CSC_BASE_ADDR	(0x00010000)
#define IVA_SCL0_BASE_ADDR	(0x00020000)
#define IVA_SCL1_BASE_ADDR	(0x00030000)
#define IVA_ORB_BASE_ADDR	(0x00040000)
#define IVA_MCH_BASE_ADDR	(0x00050000)
#define IVA_LMD_BASE_ADDR	(0x00060000)
#define IVA_LKT_BASE_ADDR	(0x00070000)
#define IVA_WIG0_BASE_ADDR	(0x00080000)
#define IVA_WIG1_BASE_ADDR	(0x00090000)
#define IVA_ENF_BASE_ADDR	(0x000A0000)
#define IVA_VDMA0_BASE_ADDR	(0x000B0000)
#define IVA_MOT_BASE_ADDR	(0x000C0000)
#define IVA_PMU_BASE_ADDR	(0x000D0000)
#define IVA_MBOX_BASE_ADDR	(0x000E0000)
#define IVA_VCM_BASE_ADDR	(0x000F0000)
#define IVA_VMCU_MEM_BASE_ADDR	(0x00100000)	/* mcu sram */
#define IVA_BLD_BASE_ADDR	(0x00140000)
#define IVA_DIF_BASE_ADDR	(0x00150000)
#define IVA_WIG2_BASE_ADDR	(0x00160000)
#define IVA_WIG3_BASE_ADDR	(0x00170000)
#define IVA_VCF_BASE_ADDR	(0x00180000)	/* vcf */


#define VMCU_MEM_SIZE		(128 * KB)
#define VCF_MEM_SIZE		(512 * KB)
#define SHMEM_SIZE		(0x1000)	/* PAGE_SIZE */

#define IVA_VCM_SCH_BUF_OFFSET	(0x00000200)
#define IVA_VCM_CMD_BUF_OFFSET	(0x00002000)
#define IVA_VCM_SCH_BUF_SIZE	(IVA_VCM_CMD_BUF_OFFSET - IVA_VCM_SCH_BUF_OFFSET)
#define IVA_VCM_CMD_BUF_SIZE	(0xA000 - IVA_VCM_CMD_BUF_OFFSET)

#endif /* __IVA_BASE_ADDR_20_H__ */
