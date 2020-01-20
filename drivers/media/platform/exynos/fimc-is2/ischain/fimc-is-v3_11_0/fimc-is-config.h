/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_CONFIG_H
#define FIMC_IS_CONFIG_H

#include <fimc-is-common-config.h>
#include <fimc-is-vendor-config.h>

/*
 * =================================================================================================
 * CONFIG -PLATFORM CONFIG
 * =================================================================================================
 */

#define SOC_30S
#define SOC_30C
#define SOC_30P
#define SOC_I0S
#define SOC_I0C
#define SOC_I0P
/* #define SOC_31S */
/* #define SOC_31C */
/* #define SOC_31P */
/* #define SOC_I1S */
/* #define SOC_I1C */
/* #define SOC_I1P */
/* #define SOC_DRC */
/* #define SOC_DIS */
/* #define SOC_ODC */
/* #define SOC_DNR */
#define SOC_SCC
#define SOC_SCP
#define SOC_VRA

/* Post Processing Configruation */
/* #define ENABLE_DRC */
/* #define ENABLE_DIS */
/* #define ENABLE_DNR */
#define ENABLE_VRA

#define USE_ONE_BINARY
#define ENABLE_IRQ_MULTI_TARGET
#define FIMC_IS_ONLINE_CPU_MIN	CONFIG_NR_CPUS
/* #define USE_MCUCTL */

#define SOC_3AAISP
/* #define SOC_MCS0 */
/* #define SOC_MCS1 */
#define HW_SLOT_MAX		(3)
#define valid_hw_slot_id(slot_id) \
	(0 <= slot_id && slot_id < HW_SLOT_MAX)
/* #define DISABLE_SETFILE */
/* #define DISABLE_LIB */

/*
 * =================================================================================================
 * CONFIG - FEATURE ENABLE
 * =================================================================================================
 */

#if defined(CONFIG_ARM_EXYNOS7870_BUS_DEVFREQ)
#define CONFIG_FIMC_IS_BUS_DEVFREQ
#endif
#define CAPTURE_NODE_MAX		5
#define OTF_YUV_FORMAT			(OTF_INPUT_FORMAT_YUV422)
#undef ENABLE_CLOCK_GATE

#ifdef ENABLE_IRQ_MULTI_TARGET
#define FIMC_IS_HW_IRQ_FLAG     IRQF_GIC_MULTI_TARGET
#else
#define FIMC_IS_HW_IRQ_FLAG     0
#endif

#define CHAIN_USE_VC_TASKLET	1

#endif
