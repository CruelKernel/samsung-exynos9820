/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd
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
#define SOC_30F
#define SOC_30G
#define SOC_I0S
#define SOC_I0C
#define SOC_I0P
#define SOC_31S
#define SOC_31C
#define SOC_31P
#define SOC_31F
#define SOC_31G
#define SOC_32S
#define SOC_32P
#define SOC_I1S
#define SOC_I1C
#define SOC_I1P
#define SOC_ME0C
#define SOC_ME1C
/* #define SOC_DRC */
/* #define SOC_D0S */
/* #define SOC_D0C */
/* #define SOC_D1S */
/* #define SOC_D1C */
/* #define SOC_ODC */
/* #define SOC_DNR */
/* #define SOC_DCP */
/* #define SOC_SCC */
/* #define SOC_SCP */
#define SOC_MCS
#define SOC_VRA
#define SOC_SSVC0
#define SOC_SSVC1
#define SOC_SSVC2
#define SOC_SSVC3

/* Post Processing Configruation */
/* #define ENABLE_DRC */
/* #define ENABLE_DIS */
/* #define ENABLE_DNR_IN_TPU */
/* #define ENABLE_DNR_IN_MCSC */
#define ENABLE_TNR	/* DNR in ISP */
#define NUM_OF_TNR_BUF	6 /* triple(3) & double buffering(2) */
#define ENABLE_10BIT_MCSC
#define ENABLE_DJAG_IN_MCSC
#define ENABLE_VRA
#define ENABLE_REPROCESSING_FD
#define ENABLE_VRA_CHANGE_SETFILE_PARSING
#define ENABLE_HYBRID_FD

#define USE_ONE_BINARY
#define USE_RTA_BINARY
#ifndef USE_BINARY_PADDING_DATA_ADDED
#define USE_BINARY_PADDING_DATA_ADDED	/* for DDK signature */
#endif
#define USE_DDK_SHUT_DOWN_FUNC
#define ENABLE_IRQ_MULTI_TARGET
#define FIMC_IS_ONLINE_CPU_MIN	4
/* #define USE_MCUCTL */

/* #define SOC_3AAISP */
#define SOC_MCS0
#define SOC_MCS1
/* #define SOC_TPU0 */
/* #define SOC_TPU1 */

#define HW_SLOT_MAX            (10)
#define valid_hw_slot_id(slot_id) \
       (0 <= slot_id && slot_id < HW_SLOT_MAX)
/* #define DISABLE_SETFILE */
/* #define DISABLE_LIB */

#define USE_SENSOR_IF_DPHY
#undef ENABLE_CLOCK_GATE
/* #define ENABLE_DIRECT_CLOCK_GATE */
#define ENABLE_HMP_BOOST

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

#undef OVERFLOW_PANIC_ENABLE_ISCHAIN
#undef OVERFLOW_PANIC_ENABLE_CSIS
#if defined(CONFIG_ARM_EXYNOS_DEVFREQ)
#define CONFIG_FIMC_IS_BUS_DEVFREQ
#endif
#if defined(OVERFLOW_PANIC_ENABLE_ISCHAIN)
#define DDK_OVERFLOW_RECOVERY		(0)	/* 0: do not execute recovery, 1: execute recovery */
#else
#define DDK_OVERFLOW_RECOVERY		(1)	/* 0: do not execute recovery, 1: execute recovery */
#endif
#define CAPTURE_NODE_MAX		12
#define OTF_YUV_FORMAT			(OTF_INPUT_FORMAT_YUV422)
#define MSB_OF_3AA_DMA_OUT		(11)
#define MSB_OF_DNG_DMA_OUT		(9)
/* #define USE_YUV_RANGE_BY_ISP */
/* #define ENABLE_3AA_DMA_CROP */

/* use bcrop1 to support DNG capture for pure bayer reporcessing */
#define USE_3AA_CROP_AFTER_BDS

/* #define ENABLE_ULTRA_FAST_SHOT */
#define ENABLE_HWFC
/* #define FW_SUSPEND_RESUME */
#define TPU_COMPRESSOR
#define USE_I2C_LOCK
#undef ENABLE_FULL_BYPASS
#define SENSOR_REQUEST_DELAY		2

#ifdef ENABLE_IRQ_MULTI_TARGET
#define FIMC_IS_HW_IRQ_FLAG     IRQF_GIC_MULTI_TARGET
#else
#define FIMC_IS_HW_IRQ_FLAG     0
#endif

/* #define MULTI_SHOT_KTHREAD */
/* #define MULTI_SHOT_TASKLET */
/* #define ENABLE_EARLY_SHOT */
#define ENABLE_HW_FAST_READ_OUT
#define FULL_OTF_TAIL_GROUP_ID		GROUP_ID_MCS0

#if defined(USE_I2C_LOCK)
#define I2C_MUTEX_LOCK(lock)	mutex_lock(lock)
#define I2C_MUTEX_UNLOCK(lock)	mutex_unlock(lock)
#else
#define I2C_MUTEX_LOCK(lock)
#define I2C_MUTEX_UNLOCK(lock)
#endif

#define ENABLE_DBG_EVENT_PRINT

#ifdef CONFIG_SECURE_CAMERA_USE
#ifdef SECURE_CAMERA_IRIS
#undef SECURE_CAMERA_IRIS
#endif
#define SECURE_CAMERA_FACE	/* For face Detection and face authentication */
#define SECURE_CAMERA_CH		((1 << CSI_ID_C) | (1 << CSI_ID_E))
#define SECURE_CAMERA_HEAP_ID		(11)
#define SECURE_CAMERA_MEM_ADDR		(0xA1000000)	/* secure_camera_heap */
#define SECURE_CAMERA_MEM_SIZE		(0x02B00000)
#define NON_SECURE_CAMERA_MEM_ADDR	(0xA3B00000)	/* camera_heap */
#define NON_SECURE_CAMERA_MEM_SIZE	(0x18C00000)

//#define SECURE_CAMERA_FACE_SEQ_CHK	/* To check sequence before applying secure protection */
#endif

#define MODULE_2L7_MODE2
#define MODULE_2L2_MODE2
#define QOS_INTCAM
#define USE_NEW_PER_FRAME_CONTROL

#define ENABLE_HWACG_CONTROL

#define BDS_DVFS
/* #define ENABLE_VIRTUAL_OTF */
/* #define ENABLE_DCF */

#define ENABLE_PDP_STAT_DMA

#define FAST_FDAE
#undef RESERVED_MEM_IN_DT

#define SKIP_VRA_SFR_DUMP
#define CHAIN_SKIP_GFRAME_FOR_VRA

/* init AWB */
/* #define ENABLE_INIT_AWB */
#define WB_GAIN_COUNT		(4)
#define INIT_AWB_COUNT_REAR	(3)
#define INIT_AWB_COUNT_FRONT	(8)

#define USE_CAMIF_FIX_UP	1
#define CHAIN_USE_VC_TASKLET	0

#endif
