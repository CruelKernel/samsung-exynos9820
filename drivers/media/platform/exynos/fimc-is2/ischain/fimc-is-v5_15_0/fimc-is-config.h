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
/*#define SOC_31S */
/* #define SOC_31C */
/* #define SOC_31P */
/* #define SOC_I1S */
/* #define SOC_I1C */
/* #define SOC_I1P */
/* #define SOC_DRC */
/* #define SOC_D0S */
/* #define SOC_D0C */
/* #define SOC_D1S */
/* #define SOC_D1C */
/* #define SOC_ODC */
#define SOC_DNR
/* #define SOC_SCC */
/* #define SOC_SCP */
#define SOC_MCS
#define SOC_VRA
#define SOC_SSVC0
#define SOC_SSVC1
#define SOC_SSVC2
/* #define SOC_DCP *//* TODO */

/* Post Processing Configruation */
/* #define ENABLE_DRC */
/* #define ENABLE_DIS */
/* #define ENABLE_DNR_IN_TPU */
#define ENABLE_DNR_IN_MCSC
#define ENABLE_VRA

#define USE_ONE_BINARY
#define USE_RTA_BINARY
#define ENABLE_IRQ_MULTI_TARGET
/* #define USE_MCUCTL */

/* #define SOC_3AAISP */
#define SOC_MCS0
/* #define SOC_MCS1 */
/* #define SOC_TPU0 */
/* #define SOC_TPU1 */

#define HW_SLOT_MAX            (4)
#define valid_hw_slot_id(slot_id) \
       (0 <= slot_id && slot_id < HW_SLOT_MAX)
/* #define DISABLE_SETFILE */
/* #define DISABLE_LIB */

#define USE_SENSOR_IF_DPHY
#undef ENABLE_CLOCK_GATE
/* #define ENABLE_DIRECT_CLOCK_GATE */

/* FIMC-IS task priority setting */
#define TASK_SENSOR_WORK_PRIO		(FIMC_IS_MAX_PRIO - 48) /* 52 */
#define TASK_GRP_OTF_INPUT_PRIO		(FIMC_IS_MAX_PRIO - 49) /* 51 */
#define TASK_GRP_DMA_INPUT_PRIO		(FIMC_IS_MAX_PRIO - 50) /* 50 */
#define TASK_MSHOT_WORK_PRIO		(FIMC_IS_MAX_PRIO - 43) /* 57 */
#define TASK_LIB_OTF_PRIO		(FIMC_IS_MAX_PRIO - 44) /* 56 */
#define TASK_LIB_AF_PRIO		(FIMC_IS_MAX_PRIO - 45) /* 55 */
#define TASK_LIB_ISP_DMA_PRIO		(FIMC_IS_MAX_PRIO - 46) /* 54 */
#define TASK_LIB_3AA_DMA_PRIO		(FIMC_IS_MAX_PRIO - 47) /* 53 */
#define TASK_LIB_AA_PRIO		(FIMC_IS_MAX_PRIO - 48) /* 52 */
#define TASK_LIB_RTA_PRIO		(FIMC_IS_MAX_PRIO - 49) /* 51 */
#define TASK_LIB_VRA_PRIO		(FIMC_IS_MAX_PRIO - 45) /* 55 */

/*
 * =================================================================================================
 * CONFIG - FEATURE ENABLE
 * =================================================================================================
 */

#define FIMC_IS_MAX_TASK               (20)

#if defined(CONFIG_ARM_EXYNOS8895_BUS_DEVFREQ)
#define CONFIG_FIMC_IS_BUS_DEVFREQ
#endif
#if defined(OVERFLOW_PANIC_ENABLE_ISCHAIN)
#define DDK_OVERFLOW_RECOVERY		(0)	/* 0: do not execute recovery, 1: execute recovery */
#else
#define DDK_OVERFLOW_RECOVERY		(1)	/* 0: do not execute recovery, 1: execute recovery */
#endif
#define CAPTURE_NODE_MAX		5
#define OTF_YUV_FORMAT			(OTF_INPUT_FORMAT_YUV422)
/* #define ENABLE_3AA_DMA_CROP */
/* #define ENABLE_ULTRA_FAST_SHOT */
/* #define ENABLE_HWFC */
/* #define FW_SUSPEND_RESUME */
/* #define TPU_COMPRESSOR */
/* #define USE_I2C_LOCK */
#undef ENABLE_FULL_BYPASS
/* #define SENSOR_CONTROL_DELAY		2 */

#ifdef ENABLE_IRQ_MULTI_TARGET
#define FIMC_IS_HW_IRQ_FLAG     IRQF_GIC_MULTI_TARGET
#else
#define FIMC_IS_HW_IRQ_FLAG     0
#endif

/* #define MULTI_SHOT_KTHREAD */
/* #define ENABLE_EARLY_SHOT */

#ifdef USE_I2C_LOCK
#define I2C_MUTEX_LOCK(lock)	mutex_lock(lock)
#define I2C_MUTEX_UNLOCK(lock)	mutex_unlock(lock)
#else
#define I2C_MUTEX_LOCK(lock)
#define I2C_MUTEX_UNLOCK(lock)
#endif

/* #define ENABLE_DBG_EVENT_PRINT */

#define CHAIN_USE_VC_TASKLET	1

#endif
