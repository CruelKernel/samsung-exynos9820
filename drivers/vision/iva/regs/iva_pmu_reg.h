/*
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _PMU_REGS_H_
#define _PMU_REGS_H_

#define IVA_PMU_CTRL_ADDR			(0x00000)
#define IVA_PMU_STATUS_ADDR			(0x00004)
#if defined(CONFIG_SOC_EXYNOS9810) || defined(CONFIG_SOC_EXYNOS9820)
#define IVA_PMU_ID_ADDR				(0x00008)
#endif
#define IVA_PMU_IRQ_EN_ADDR			(0x00010)
#define IVA_PMU_IRQ_RAW_STATUS_ADDR		(0x00014)
#define IVA_PMU_IRQ_SOFT_ADDR			(0x00018)
#define IVA_PMU_IRQ_STATUS_ADDR			(0x0001C)
#define IVA_PMU_CLOCK_CONTROL_ADDR		(0x00020)
#define IVA_PMU_SOFT_RESET_ADDR			(0x00024)
#define IVA_PMU_VMCU_CTRL_ADDR			(0x00028)
#if defined(CONFIG_SOC_EXYNOS9810)
#define IVA_PMU_HWACG_EN_ADDR			(0x0002C)
#endif
#define IVA_PMU_HWA_ACTIVITY_ADDR		(0x00030)
#if defined(CONFIG_SOC_EXYNOS9820)
#define IVA_PMU_SFR_HWACG_EN_ADDR		(0x0002C)
#define IVA_PMU_CORE_HWACG_EN_ADDR		(0x00038)
#define IVA_PMU_QACTIVE_MON_ADDR		(0x00064)
#endif
#define IVA_PMU_GP_IRQ_EN_ADDR			(0x00040)
#define IVA_PMU_GP_IRQ_RAW_STATUS_ADDR		(0x00044)
#define IVA_PMU_GP_IRQ_SOFT_ADDR		(0x00048)
#define IVA_PMU_GP_IRQ_STATUS_ADDR		(0x0004C)
#if defined(CONFIG_SOC_EXYNOS9810) || defined(CONFIG_SOC_EXYNOS9820)
#define IVA_PMU_GP_SOURCE_OFFSET		(0x00050)
#endif
#define IVA_PMU_SPARE_0_ADDR			(0x0FFE0)
#define IVA_PMU_SPARE_1_ADDR			(0x0FFE4)
#define IVA_PMU_SPARE_2_ADDR			(0x0FFE8)
#define IVA_PMU_SPARE_3_ADDR			(0x0FFEC)
#define IVA_PMU_DBGSEL_ADDR			(0x0FFF0)
#define IVA_PMU_DBGVAL_ADDR			(0x0FFF4)

#endif /* _PMU_REGS_H_ */
