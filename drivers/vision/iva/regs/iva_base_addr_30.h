/*
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __IVA_BASE_ADDR_30_H__
#define __IVA_BASE_ADDR_30_H__

#define KB	(1024)
#define MB	(KB * KB)

/* from Zynq ARM A9 : will be described in device tree */
#define IVA_CFG_PHY_BASE_ADDR	(0x40000000)
/* HWA - totally 2MB range*/
#define IVA_CFG_SIZE		(2 * MB)

/* System Peripherals from host : in FPGA */
#define PERI_PHY_BASE_ADDR	(0xA2000000)
#define PERI_SIZE		(16*4 * KB)

/* SCore APB from host */
#define SCORE_APB_PHY_BASE_ADDR	(0x41000000)
#define SCORE_APB_SIZE		(512 * KB)
#define SCORE_APB_END_ADDR	(SCORE_APB_PHY_BASE_ADDR + SCORE_APB_SIZE)

#define IVA_HWA_ADDR_GAP	(0x10000)

/* offset from iva base address */
#define IVA_BAX_BASE_ADDR	(0x00000000)
#define IVA_CSC_BASE_ADDR	(0x00010000)
#define IVA_SCL0_BASE_ADDR	(0x00020000)
#define IVA_SCL1_BASE_ADDR	(0x00030000)
#define IVA_SCL2_BASE_ADDR	(0x00040000)
#define IVA_ORB_BASE_ADDR	(0x00050000)
#define IVA_MCH_BASE_ADDR	(0x00060000)
#define IVA_LMD_BASE_ADDR	(0x00070000)
#define IVA_LKT_BASE_ADDR	(0x00080000)
#define IVA_WIG0_BASE_ADDR	(0x00090000)
#define IVA_WIG1_BASE_ADDR	(0x000A0000)
#define IVA_WIG2_BASE_ADDR	(0x000B0000)
#define IVA_ENF_BASE_ADDR	(0x000C0000)
#define IVA_VDMA0_BASE_ADDR	(0x000D0000)
#define IVA_MOT_BASE_ADDR	(0x000E0000)
#define IVA_PMU_BASE_ADDR	(0x000F0000)
#define IVA_VMCU_MEM_BASE_ADDR	(0x00100000)	/* mcu sram */
#define IVA_VMCU_DTCM_BASE_ADDR	(0x00110000)
#define IVA_SYS_REG_BASE_ADDR	(0x00130000)
#define IVA_STA0_BASE_ADDR	(0x00131000)
#define IVA_STA1_BASE_ADDR	(0x00132000)
#define IVA_IMU_BASE_ADDR	(0x0013F000)
#define IVA_MBOX_BASE_ADDR	(0x00140000)
#define IVA_VCM_BASE_ADDR	(0x00150000)
#define IVA_BLD_BASE_ADDR	(0x00160000)
#define IVA_DIF_BASE_ADDR	(0x00170000)
#define IVA_VCF_BASE_ADDR	(0x00180000)	/* vcf */

#define VCF_MEM_SIZE		(512 * KB)
#define VMCU_MEM_SIZE		(128 * KB)
#define SHMEM_SIZE		(0x1000)	/* PAGE_SIZE */
#define VMCU_TCM_MEM_SIZE	(64 * KB)

#if defined(CONFIG_SOC_EXYNOS9820)
#define EXTEND_VMCU_MEM_SIZE	(256 * KB)
#define EXTEND_SHMEM_SIZE	(0x2000)	/* 8KB */
#endif

#define IVA_VCM_SCH_BUF_OFFSET	(0x00000200)
#define IVA_VCM_CMD_BUF_OFFSET	(0x00002000)

#define IVA_NUM_VCM_SCH (8)
#define IVA_VCM_SCH_SINGLE_BUF_SIZE     (0X200)
#define IVA_VCM_CMD_SINGLE_BUF_SIZE     (0X1000)
#define IVA_VCM_SCH_BUF_SIZE    (IVA_VCM_SCH_SINGLE_BUF_SIZE * IVA_NUM_VCM_SCH)
#define IVA_VCM_CMD_BUF_SIZE    (IVA_VCM_CMD_SINGLE_BUF_SIZE * IVA_NUM_VCM_SCH)
#endif /* __IVA_BASE_ADDR_30_H__ */
