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
#define SOC_31S
#define SOC_31C
#define SOC_31P
#define SOC_I1S
#define SOC_I1C
#define SOC_I1P
/* #define SOC_DRC */
#define SOC_DIS
#define SOC_ODC
#define SOC_DNR
/* #define SOC_SCC */
/* #define SOC_SCP */
#define SOC_MCS
#define SOC_VRA
#ifdef CONFIG_USE_SENSOR_GROUP
#define SOC_SSVC0
#endif
#define SOC_SSVC1
#define SOC_SSVC2

/* Post Processing Configruation */
/* #define ENABLE_DRC */
/* #define ENABLE_DIS */
#define ENABLE_DNR
#define ENABLE_VRA

#if defined(CONFIG_USE_DIRECT_IS_CONTROL)
#define USE_ONE_BINARY
#define USE_RTA_BINARY
#define ENABLE_IRQ_MULTI_TARGET
#define USE_MCUCTL

/* #define SOC_3AAISP */
#define SOC_MCS0
#define SOC_MCS1
#define HW_SLOT_MAX            (8)
#define valid_hw_slot_id(slot_id) \
       (0 <= slot_id && slot_id < HW_SLOT_MAX)
/* #define DISABLE_SETFILE */
/* #define DISABLE_LIB */

#undef ENABLE_CLOCK_GATE
#endif

/*
 * =================================================================================================
 * CONFIG - FEATURE ENABLE
 * =================================================================================================
 */

#define FIMC_IS_MAX_TASK		(20)

#if defined(CONFIG_ARM_EXYNOS8890_BUS_DEVFREQ)
#define CONFIG_FIMC_IS_BUS_DEVFREQ
#endif
#define CAPTURE_NODE_MAX		5
#define OTF_YUV_FORMAT			(OTF_INPUT_FORMAT_YUV422)
/* #define ENABLE_3AA_DMA_CROP */
#ifdef ENABLE_IS_CORE
#define ENABLE_ULTRA_FAST_SHOT
#define ENABLE_FW_SYNC_LOG
#endif
#define ENABLE_HWFC
/* #define FW_SUSPEND_RESUME */
#define TPU_COMPRESSOR
#undef ENABLE_FULL_BYPASS

#ifdef ENABLE_IRQ_MULTI_TARGET
#define FIMC_IS_HW_IRQ_FLAG     IRQF_GIC_MULTI_TARGET
#else
#define FIMC_IS_HW_IRQ_FLAG     0
#endif

#define MULTI_SHOT_KTHREAD
/* #define ENABLE_EARLY_SHOT */

#define CHAIN_USE_VC_TASKLET	1

#endif
