/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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
/* This count is defined by count of fimc_is_sensor in the dtsi file */
#define FIMC_IS_HW_SENSOR_COUNT 4

#define SOC_30S
#define SOC_30C
#define SOC_30P
#define SOC_I0S
#define SOC_I0C
#define SOC_I0P
#define SOC_31S
#define SOC_31C
#define SOC_31P
/* #define SOC_I1S */
/* #define SOC_I1C */
/* #define SOC_I1P */
#define SOC_ME0C
/* #define SOC_DRC */
/* #define SOC_D0S */
/* #define SOC_D0C */
/* #define SOC_D1S */
/* #define SOC_D1C */
/* #define SOC_ODC */
#define SOC_DNR
/* #define SOC_DCP */
/* #define SOC_SCC */
/* #define SOC_SCP */
#define SOC_MCS
#define SOC_VRA
#define SOC_SSVC0
#define SOC_SSVC1
#define SOC_SSVC2
#define SOC_SSVC3
/* #define SOC_DCP *//* TODO */
#define SOC_PAF0	/* PAF_RDMA0 */
#define SOC_PAF1	/* PAF_RDMA1 */

/* Post Processing Configruation */
/* #define ENABLE_DRC */
/* #define ENABLE_DIS */
/* #define ENABLE_DNR_IN_TPU */
#define ENABLE_DNR_IN_MCSC
#define ENABLE_10BIT_MCSC
/* #define ENABLE_DJAG_IN_MCSC */
#define ENABLE_VRA
/* #define ENABLE_REPROCESSING_FD */
#define ENABLE_VRA_CHANGE_SETFILE_PARSING
#define ENABLE_HYBRID_FD

#define USE_ONE_BINARY
#define USE_RTA_BINARY
#define USE_BINARY_PADDING_DATA_ADDED	/* for DDK signature */
/* #define USE_DDK_SHUT_DOWN_FUNC */
#define ENABLE_IRQ_MULTI_TARGET
#define FIMC_IS_ONLINE_CPU_MIN	4
/* #define USE_MCUCTL */

/* #define SOC_3AAISP */
#define SOC_MCS0
/* #define SOC_MCS1 */
/* #define SOC_TPU0 */
/* #define SOC_TPU1 */

#define HW_SLOT_MAX            (7)
#define valid_hw_slot_id(slot_id) \
       (0 <= slot_id && slot_id < HW_SLOT_MAX)
/* #define DISABLE_SETFILE */
/* #define DISABLE_LIB */

/* #define USE_SENSOR_IF_DPHY */
#define USE_PHY_LINK
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

#define FIMC_IS_MAX_TASK		(40)

#if defined(CONFIG_ARM_EXYNOS9610_BUS_DEVFREQ)
#define CONFIG_FIMC_IS_BUS_DEVFREQ
#endif

/* #define ENABLE_FULLCHAIN_OVERFLOW_RECOVERY */

#if defined(ENABLE_FULLCHAIN_OVERFLOW_RECOVERY)
#undef OVERFLOW_PANIC_ENABLE_ISCHAIN
#endif

#if defined(OVERFLOW_PANIC_ENABLE_ISCHAIN) || defined(ENABLE_FULLCHAIN_OVERFLOW_RECOVERY)
#define DDK_OVERFLOW_RECOVERY		(0)	/* 0: do not execute recovery, 1: execute recovery */
#else
#define DDK_OVERFLOW_RECOVERY		(1)	/* 0: do not execute recovery, 1: execute recovery */
#endif

#define CAPTURE_NODE_MAX		12
#define OTF_YUV_FORMAT			(OTF_INPUT_FORMAT_YUV422)
#define MSB_OF_3AA_DMA_OUT		(11)
#define USE_YUV_RANGE_BY_ISP
/* #define ENABLE_3AA_DMA_CROP */

/* use bcrop1 to support DNG capture for pure bayer reporcessing */
#define USE_3AA_CROP_AFTER_BDS

/* #define ENABLE_ULTRA_FAST_SHOT */
#define ENABLE_HWFC
/* #define FW_SUSPEND_RESUME */
/* #define TPU_COMPRESSOR */
#define USE_I2C_LOCK
#undef ENABLE_FULL_BYPASS
#define SENSOR_REQUEST_DELAY		2
#define ENABLE_REMOSAIC_CAPTURE

#ifdef ENABLE_IRQ_MULTI_TARGET
#define FIMC_IS_HW_IRQ_FLAG     IRQF_GIC_MULTI_TARGET
#else
#define FIMC_IS_HW_IRQ_FLAG     0
#endif

/* #define MULTI_SHOT_KTHREAD */
#define MULTI_SHOT_TASKLET
/* #define ENABLE_EARLY_SHOT */

#if defined(USE_I2C_LOCK)
#define I2C_MUTEX_LOCK(lock)	mutex_lock(lock)
#define I2C_MUTEX_UNLOCK(lock)	mutex_unlock(lock)
#else
#define I2C_MUTEX_LOCK(lock)
#define I2C_MUTEX_UNLOCK(lock)
#endif

#define ENABLE_DBG_EVENT_PRINT
#define ENABLE_CAL_LOAD

#define QOS_INTCAM

/*
#define SECURE_CAMERA_EMULATE
#define SECURE_CAMERA_CH	(CSI_ID_D)
#define SECURE_CAMERA_MEM_ADDR	(0xD0000000)
#define SECURE_CAMERA_MEM_SIZE	(0x1400000)

#define MODULE_2L7_MODE2
#define MODULE_2L2_MODE2
*/
#define USE_NEW_PER_FRAME_CONTROL
/* #define ENABLE_HWACG_CONTROL */

/* #define BDS_DVFS */
#define ENABLE_HW_FAST_READ_OUT
#define FULL_OTF_TAIL_GROUP_ID		GROUP_ID_MCS0

#define CHAIN_USE_VC_TASKLET	0

#endif
