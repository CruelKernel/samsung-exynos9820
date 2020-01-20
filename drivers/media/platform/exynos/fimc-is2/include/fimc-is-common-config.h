/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_COMMON_CONFIG_H
#define FIMC_IS_COMMON_CONFIG_H

/*
 * =================================================================================================
 * CONFIG - GLOBAL OPTIONS
 * =================================================================================================
 */
#define FIMC_IS_SENSOR_COUNT	6
#define FIMC_IS_STREAM_COUNT	9
#define FIMC_IS_STR_LEN		10

#define FIMC_IS_MAX_PRIO	(MAX_RT_PRIO)
#define NI_BACKUP_MAX		32

/*
 * =================================================================================================
 * CONFIG - FEATURE ENABLE
 * =================================================================================================
 */

/* #define FW_SUSPEND_RESUME */
#define ENABLE_CLOCK_GATE
#define HAS_FW_CLOCK_GATE
#define ENABLE_FULL_BYPASS
#define ENABLE_ONE_SLOT

/* disable the Fast Shot because of AF fluctuating issue when touch af */
/* #define ENABLE_FAST_SHOT */

#define ENABLE_FAULT_HANDLER
#define ENABLE_PANIC_HANDLER
#define ENABLE_REBOOT_HANDLER
/* #define ENABLE_PANIC_SFR_PRINT */
/* #define ENABLE_MIF_400 */
#define ENABLE_DTP
#define ENABLE_FLITE_OVERFLOW_STOP
#define ENABLE_DBG_FS
#define ENABLE_DBG_STATE
#define FIXED_SENSOR_DEBUG
#define ENABLE_RESERVED_MEM
#define ENABLE_DYNAMIC_MEM

#if defined(CONFIG_PM_DEVFREQ)
#define ENABLE_DVFS
#define START_DVFS_LEVEL FIMC_IS_SN_MAX
#endif /* CONFIG_PM_DEVFREQ */

/* Config related to control HW directly */
#if defined(CONFIG_CAMERA_MC_SCALER_VER1_USE)
#define MCS_USE_SCP_PARAM
#else
#undef MCS_USE_SCP_PARAM
#endif

#if defined(CONFIG_USE_DIRECT_IS_CONTROL)
#undef ENABLE_IS_CORE
#define ENABLE_FPSIMD_FOR_USER
#define ENABLE_CSIISR /* this feature is only available at csi v1 */
#else
#define ENABLE_IS_CORE
#define ENABLE_FW_SHARE_DUMP
#endif

/* notifier for MIF throttling */
#undef CONFIG_CPU_THERMAL_IPA
#if defined(CONFIG_CPU_THERMAL_IPA)
#define EXYNOS_MIF_ADD_NOTIFIER(nb) exynos_mif_add_notifier(nb)
#else
#define EXYNOS_MIF_ADD_NOTIFIER(nb)
#endif

#if defined(CONFIG_USE_HOST_FD_LIBRARY)
#ifndef ENABLE_FD_SW
#define ENABLE_FD_SW
#else
#undef ENABLE_FD_SW
#endif
#endif

/* BUG_ON | FIMC_BUG Macro control */
#define USE_FIMC_BUG

/*
 * =================================================================================================
 * CONFIG - DEBUG OPTIONS
 * =================================================================================================
 */

extern int debug_stream;
extern int debug_video;
extern int debug_hw;
extern int debug_device;
extern int debug_irq;
extern int debug_sensor;

#define DEBUG_LOG_MEMORY
/* #define DEBUG_HW_SIZE */
#define DBG_STREAM_ID 0x3F
/* #define DBG_JITTER */
#define FW_PANIC_ENABLE
/* #define SENSOR_PANIC_ENABLE */
#define OVERFLOW_PANIC_ENABLE_ISCHAIN
#define OVERFLOW_PANIC_ENABLE_CSIS
#define ENABLE_KERNEL_LOG_DUMP
/* #define FIXED_FPS_DEBUG */
/* #define FIXED_TDNR_NOISE_INDEX */

/* 5fps */
#define FIXED_FPS_VALUE (30 / 6)
#define FIXED_EXPOSURE_VALUE (200000) /* 33.333 * 6 */
#define FIXED_AGAIN_VALUE (150 * 6)
#define FIXED_DGAIN_VALUE (150 * 6)
#define FIXED_TDNR_NOISE_INDEX_VALUE (0)
/* #define DBG_CSIISR */
/* #define DBG_IMAGE_KMAPPING */
/* #define DBG_DRAW_DIGIT */
/* #define DBG_IMAGE_DUMP */
/* #define DBG_META_DUMP */
#define DBG_HAL_DEAD_PANIC_DELAY (500) /* ms */
#define DBG_DMA_DUMP_PATH	"/data"
#define DBG_DMA_DUMP_INTEVAL	33	/* unit : frame */
#define DBG_DMA_DUMP_VID_COND(vid)	((vid == FIMC_IS_VIDEO_SS0VC0_NUM) || \
					(vid == FIMC_IS_VIDEO_SS1VC0_NUM) || \
					(vid == FIMC_IS_VIDEO_M0P_NUM))
/* #define DEBUG_HW_SFR */
/* #define DBG_DUMPREG */
/* #define USE_ADVANCED_DZOOM */
/* #define PRINT_CAPABILITY */
/* #define PRINT_BUFADDR */
/* #define PRINT_DZOOM */
/* #define PRINT_PARAM */
/* #define PRINT_I2CCMD */
#define ISDRV_VERSION 244

/*
 * driver version extension
 */
#ifdef ENABLE_CLOCK_GATE
#define get_drv_clock_gate() 0x1
#else
#define get_drv_clock_gate() 0x0
#endif
#ifdef ENABLE_DVFS
#define get_drv_dvfs() 0x2
#else
#define get_drv_dvfs() 0x0
#endif

#define GET_SSX_ID(video) (video->id - FIMC_IS_VIDEO_SS0_NUM)
#define GET_PAFXS_ID(video) ((video->id < FIMC_IS_VIDEO_PAF1S_NUM) ? 0 : 1)
#define GET_3XS_ID(video) ((video->id < FIMC_IS_VIDEO_31S_NUM) ? 0 : 1)
#define GET_3XC_ID(video) ((video->id < FIMC_IS_VIDEO_31S_NUM) ? 0 : 1)
#define GET_3XP_ID(video) ((video->id < FIMC_IS_VIDEO_31S_NUM) ? 0 : 1)
#define GET_3XF_ID(video) ((video->id < FIMC_IS_VIDEO_31S_NUM) ? 0 : 1)
#define GET_3XG_ID(video) ((video->id < FIMC_IS_VIDEO_31S_NUM) ? 0 : 1)
#define GET_IXS_ID(video) ((video->id < FIMC_IS_VIDEO_I1S_NUM) ? 0 : 1)
#define GET_IXC_ID(video) ((video->id < FIMC_IS_VIDEO_I1S_NUM) ? 0 : 1)
#define GET_IXP_ID(video) ((video->id < FIMC_IS_VIDEO_I1S_NUM) ? 0 : 1)
#define GET_MEXC_ID(video) (video->id - FIMC_IS_VIDEO_ME0C_NUM)
#define GET_DXS_ID(video) ((video->id < FIMC_IS_VIDEO_D1S_NUM) ? 0 : 1)
#define GET_DXC_ID(video) ((video->id < FIMC_IS_VIDEO_D1S_NUM) ? 0 : 1)
#define GET_DCPXS_ID(video) ((video->id < FIMC_IS_VIDEO_DCP1S_NUM) ? 0 : 1)
#define GET_DCPXC_ID(video) ((video->id < FIMC_IS_VIDEO_DCP1S_NUM) ? 0 \
				: ((video->id < FIMC_IS_VIDEO_DCP2C_NUM) ? 1 : 2))
#define GET_MXS_ID(video) (video->id - FIMC_IS_VIDEO_M0S_NUM)
#define GET_MXP_ID(video) (video->id - FIMC_IS_VIDEO_M0P_NUM)

#define GET_STR(str) #str

/* sync log with HAL, FW */
#define log_sync(sync_id) info("FIMC_IS_SYNC %d\n", sync_id)

#define test_bit_variables(bit, var) \
	test_bit(((bit)% BITS_PER_LONG), (var))

/*
 * =================================================================================================
 * LOG - ERR
 * =================================================================================================
 */
#ifdef err
#undef err
#endif
#define err(fmt, args...) \
	err_common("[ERR]%s:%d:", fmt "\n", __func__, __LINE__, ##args)

/* multi-stream */
#define merr(fmt, object, args...) \
	merr_common("[%d][ERR]%s:%d:", fmt "\n", object->instance, __func__, __LINE__, ##args)

/* multi-stream & group error */
#define mgerr(fmt, object, group, args...) \
	merr_common("[%d][GP%d][ERR]%s:%d:", fmt "\n", object->instance, group->id, __func__, __LINE__, ##args)

/* multi-stream & subdev error */
#define mserr(fmt, object, subdev, args...) \
	merr_common("[%d][%s][ERR]%s:%d:", fmt "\n", object->instance, subdev->name, __func__, __LINE__, ##args)

/* multi-stream & video error */
#define mverr(fmt, object, video, args...) \
	merr_common("[%d][V%02d][ERR]%s:%d:", fmt "\n", object->instance, video->id, __func__, __LINE__, ##args)

/* multi-stream & runtime error */
#define mrerr(fmt, object, frame, args...) \
	merr_common("[%d][F%d][ERR]%s:%d:", fmt "\n", object->instance, frame->fcount, __func__, __LINE__, ##args)

/* multi-stream & group & runtime error */
#define mgrerr(fmt, object, group, frame, args...) \
	merr_common("[%d][GP%d][F%d][ERR]%s:%d:", fmt "\n", object->instance, group->id, frame->fcount, __func__, __LINE__, ##args)

/* multi-stream & pipe error */
#define mperr(fmt, object, pipe, video, args...) \
	merr_common("[%d][P%02d][V%02d]%s:%d:", fmt "\n", object->instance, pipe->id, video->id, __func__, __LINE__, ##args)

/*
 * =================================================================================================
 * LOG - WARN
 * =================================================================================================
 */
#ifdef warn
#undef warn
#endif
#define warn(fmt, args...) \
	warn_common("[WRN]", fmt "\n", ##args)

#define mwarn(fmt, object, args...) \
	mwarn_common("[%d][WRN]", fmt "\n", object->instance, ##args)

#define mvwarn(fmt, object, video, args...) \
	mwarn_common("[%d][V%02d][WRN]", fmt "\n", object->instance, video->id, ##args)

#define mgwarn(fmt, object, group, args...) \
	mwarn_common("[%d][GP%d][WRN]", fmt "\n", object->instance, group->id, ##args)

#define mrwarn(fmt, object, frame, args...) \
	mwarn_common("[%d][F%d][WRN]", fmt "\n", object->instance, frame->fcount, ##args)

#define mswarn(fmt, object, subdev, args...) \
	mwarn_common("[%d][%s][WRN]", fmt "\n", object->instance, subdev->name, ##args)

#define mgrwarn(fmt, object, group, frame, args...) \
	mwarn_common("[%d][GP%d][F%d][WRN]", fmt "\n", object->instance, group->id, frame->fcount, ##args)

#define msrwarn(fmt, object, subdev, frame, args...) \
	mwarn_common("[%d][%s][F%d][WRN]", fmt "\n", object->instance, subdev->name, frame->fcount, ##args)

#define mpwarn(fmt, object, pipe, video, args...) \
	mwarn_common("[%d][P%02d][V%02d]", fmt "\n", object->instance, pipe->id, video->id, ##args)

/*
 * =================================================================================================
 * LOG - INFO
 * =================================================================================================
 */
#define info(fmt, args...) \
	info_common("", fmt, ##args)

#define sfrinfo(fmt, args...) \
	info_common("[SFR]", fmt, ##args)

#define minfo(fmt, object, args...) \
	minfo_common("[%d]", fmt, object->instance, ##args)

#define mvinfo(fmt, object, video, args...) \
	minfo_common("[%d][V%02d]", fmt, object->instance, video->id, ##args)

#define msinfo(fmt, object, subdev, args...) \
	minfo_common("[%d][%s]", fmt, object->instance, subdev->name, ##args)

#define msrinfo(fmt, object, subdev, frame, args...) \
	minfo_common("[%d][%s][F%d]", fmt, object->instance, subdev->name, frame->fcount, ##args)

#define mginfo(fmt, object, group, args...) \
	minfo_common("[%d][GP%d]", fmt, object->instance, group->id, ##args)

#define mrinfo(fmt, object, frame, args...) \
	minfo_common("[%d][F%d]", fmt, object->instance, frame->fcount, ##args)

#define mgrinfo(fmt, object, group, frame, args...) \
	minfo_common("[%d][GP%d][F%d]", fmt, object->instance, group->id, frame->fcount, ##args)

#define mpinfo(fmt, object, video, args...) \
	minfo_common("[%d][PV%02d]", fmt, object->instance, video->id, ##args)

#define mdbg_pframe(fmt, object, subdev, frame, args...) \
	minfo_common("[%d][%s][F%d]", fmt, object->instance, subdev->name, frame->fcount, ##args)


/*
 * =================================================================================================
 * LOG - DEBUG_SENSOR
 * =================================================================================================
 */
#define dbg(fmt, args...)

#define dbg_sensor(level, fmt, args...) \
	dbg_common(((debug_sensor) >= (level)) && (debug_sensor < 3), "[SSD]", fmt, ##args)

#define dbg_actuator(fmt, args...) \
	dbg_common((debug_sensor >= 3) && (debug_sensor < 4), "[ACT]", fmt, ##args)

#define dbg_flash(fmt, args...) \
	dbg_common((debug_sensor >= 4) && (debug_sensor < 5), "[FLS]", fmt, ##args)

#define dbg_preproc(fmt, args...) \
	dbg_common((debug_sensor >= 5) && (debug_sensor < 6), "[PRE]", fmt, ##args)

#define dbg_aperture(fmt, args...) \
	dbg_common((debug_sensor >= 6) && (debug_sensor < 7), "[APERTURE]", fmt, ##args)

#define dbg_ois(fmt, args...) \
	dbg_common((debug_sensor >= 7) && (debug_sensor < 8), "[OIS]", fmt, ##args)

/*
 * =================================================================================================
 * LOG - DEBUG_VIDEO
 * =================================================================================================
 */
#define mdbgv_vid(fmt, args...) \
	dbg_common(debug_video, "[VID:V]", fmt, ##args)

#define mdbgv_pre(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][PRE%d:V]", fmt, ((struct fimc_is_device_preproc *)this->device)->instance, GET_SSX_ID(this->video), ##args)

#define mdbgv_sensor(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][SS%d:V]", fmt, ((struct fimc_is_device_sensor *)this->device)->instance, GET_SSX_ID(this->video), ##args)

#define mdbgv_3aa(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][3%dS:V]", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_3XS_ID(this->video), ##args)

#define mdbgv_3xc(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][3%dC:V]", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_3XC_ID(this->video), ##args)

#define mdbgv_3xp(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][3%dP:V]", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_3XP_ID(this->video), ##args)

#define mdbgv_3xf(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][3%dF:V]", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_3XF_ID(this->video), ##args)

#define mdbgv_3xg(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][3%dG:V]", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_3XG_ID(this->video), ##args)

#define mdbgv_isp(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][I%dS:V]", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_IXS_ID(this->video), ##args)

#define mdbgv_ixc(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][I%dC:V]", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_IXC_ID(this->video), ##args)

#define mdbgv_ixp(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][I%dP:V]", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_IXP_ID(this->video), ##args)

#define mdbgv_mexc(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][ME%dC:V]", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_IXP_ID(this->video), ##args)

#define mdbgv_scp(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][SCP:V]", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, ##args)

#define mdbgv_scc(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][SCC:V]", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, ##args)

#define mdbgv_dis(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][D%dS:V]", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_DXS_ID(this->video), ##args)

#define mdbgv_dxc(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][D%dC:V]", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_DXC_ID(this->video), ##args)

#define mdbgv_dcpxs(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][DCP%dS:V]", fmt, \
				((struct fimc_is_device_ischain *)this->device)->instance, \
				GET_DCPXS_ID(this->video), ##args)

#define mdbgv_dcpxc(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][DCP%dC:V]", fmt, \
				((struct fimc_is_device_ischain *)this->device)->instance, \
				GET_DCPXC_ID(this->video), ##args)

#define mdbgv_mcs(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][M%dS:V]", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_MXS_ID(this->video), ##args)

#define mdbgv_mxp(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][M%dP:V]", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_MXP_ID(this->video), ##args)

#define mdbgv_vra(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][VRA:V]", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, ##args)

#define mdbgv_ssxvc0(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][SSXVC0:V]", fmt, ((struct fimc_is_device_sensor *)this->device)->instance, ##args)

#define mdbgv_ssxvc1(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][SSXVC1:V]", fmt, ((struct fimc_is_device_sensor *)this->device)->instance, ##args)

#define mdbgv_ssxvc2(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][SSXVC2:V]", fmt, ((struct fimc_is_device_sensor *)this->device)->instance, ##args)

#define mdbgv_ssxvc3(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][SSXVC3:V]", fmt, ((struct fimc_is_device_sensor *)this->device)->instance, ##args)

#define mdbgv_paf(fmt, this, args...) \
	mdbg_common(debug_video, "[%d][PAF%dS:V]", fmt, \
				((struct fimc_is_device_ischain *)this->device)->instance, \
				GET_PAFXS_ID(this->video), ##args)

/*
 * =================================================================================================
 * LOG - DEBUG_DEVICE
 * =================================================================================================
 */
#define mdbgd_sensor(fmt, this, args...) \
	mdbg_common(debug_device, "[%d][SEN:D]", fmt, this->instance, ##args)

#define mdbgd_front(fmt, this, args...) \
	mdbg_common(debug_device, "[%d][FRT:D]", fmt, this->instance, ##args)

#define mdbgd_back(fmt, this, args...) \
	mdbg_common(debug_device, "[%d][BAK:D]", fmt, this->instance, ##args)

#define mdbgd_ischain(fmt, this, args...) \
	mdbg_common(debug_device, "[%d][ISC:D]", fmt, this->instance, ##args)

#define mdbgd_group(fmt, group, args...) \
	mdbg_common(debug_device, "[%d][GP%d:D]", fmt, group->device->instance, group->id, ##args)

#define dbgd_resource(fmt, args...) \
	dbg_common(debug_device, "[RSC:D]", fmt, ##args)

/*
 * =================================================================================================
 * LOG - DEBUG_STREAM
 * =================================================================================================
 */
#define dbg_interface(level, fmt, args...) \
	dbg_common((debug_stream) >= (level), "[ITF:S]", fmt, ##args)

#define mvdbgs(level, fmt, object, queue, args...) \
	mdbg_common((debug_stream) >= (level), "[%d][%s:S]", fmt, object->instance, (queue)->name, ##args)

#define mgrdbgs(level, fmt, object, group, frame, args...) \
	mdbg_common((debug_stream) >= (level), "[%d][GP%d:S][F%d]", fmt, object->instance, group->id, frame->fcount, ##args)

#define msrdbgs(level, fmt, object, subdev, frame, args...) \
	mdbg_common((debug_stream) >= (level), "[%d][%s:S][F%d]", fmt, object->instance, subdev->name, frame->fcount, ##args)

#define mdbgs_ischain(level, fmt, object, args...) \
	mdbg_common((debug_stream) >= (level), "[%d][ISC:S]", fmt, object->instance, ##args)

#define mdbgs_sensor(level, fmt, object, args...) \
	mdbg_common((debug_stream) >= (level), "[%d][SEN:S]", fmt, object->instance, ##args)

#define msdbgs_hw(level, fmt, instance, object, args...) \
	dbg_common((debug_stream) >= (level), "[%d][HW:%s:S]", fmt, instance, object->name, ##args)

/*
 * =================================================================================================
 * LOG - PROBE
 * =================================================================================================
 */
#define probe_info(fmt, ...)		\
	pr_info(fmt, ##__VA_ARGS__)
#define probe_err(fmt, args...)		\
	pr_err("[ERR]%s:%d:" fmt "\n", __func__, __LINE__, ##args)
#define probe_warn(fmt, args...)	\
	pr_warning("[WRN]" fmt "\n", ##args)

#if defined(DEBUG_LOG_MEMORY)
#define fimc_is_err(fmt, ...)	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define fimc_is_warn(fmt, ...)	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define fimc_is_dbg(fmt, ...)	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define fimc_is_info(fmt, ...)	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define fimc_is_cont(fmt, ...)	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#else
#define fimc_is_err(fmt, ...)	pr_err(fmt, ##__VA_ARGS__)
#define fimc_is_warn(fmt, ...)	pr_warning(fmt, ##__VA_ARGS__)
#define fimc_is_dbg(fmt, ...)	pr_info(fmt, ##__VA_ARGS__)
#define fimc_is_info(fmt, ...)	pr_info(fmt, ##__VA_ARGS__)
#define fimc_is_cont(fmt, ...)	pr_cont(fmt, ##__VA_ARGS__)
#endif

#define merr_common(prefix, fmt, instance, args...)				\
	do {									\
		if ((1<<(instance)) & DBG_STREAM_ID)				\
			fimc_is_err("[@]" prefix fmt, instance, ##args);	\
	} while (0)

#define mwarn_common(prefix, fmt, instance, args...)				\
	do {									\
		if ((1<<(instance)) & DBG_STREAM_ID)				\
			fimc_is_warn("[@]" prefix fmt, instance, ##args);	\
	} while (0)

#define minfo_common(prefix, fmt, instance, args...)				\
	do {									\
		if ((1<<(instance)) & DBG_STREAM_ID)				\
			fimc_is_info("[@]" prefix fmt, instance, ##args);	\
	} while (0)

#define mdbg_common(level, prefix, fmt, instance, args...)			\
	do {									\
		if (unlikely(level))						\
			fimc_is_dbg("[@][DBG]" prefix fmt, instance, ##args);	\
	} while (0)

#define err_common(prefix, fmt, args...)	\
	fimc_is_err("[@]" prefix fmt, ##args)

#define warn_common(prefix, fmt, args...)	\
	fimc_is_warn("[@]" prefix fmt, ##args)

#define info_common(prefix, fmt, args...)	\
	fimc_is_info("[@]" prefix fmt, ##args)

#define dbg_common(level, prefix, fmt, args...)			\
	do {							\
		if (unlikely(level))				\
			fimc_is_dbg("[@][DBG]" prefix fmt, ##args);	\
	} while (0)

/*
 * =================================================================================================
 * LOG - DEBUG_IRQ
 * =================================================================================================
 */
/* FIMC-BNS isr log */
#define dbg_fliteisr(fmt, args...)	\
	dbg_common(debug_irq, "[FBNS]", fmt, ##args)

/* Tasklet Msg log */
#define dbg_tasklet(fmt, args...)	\
	dbg_common(debug_irq, "[FBNS]", fmt, ##args)

#define dbg_isr(fmt, object, args...)		\
	dbg_common(debug_irq, "[%s]", fmt, object->name, ##args)

#if defined(CONFIG_USE_DIRECT_IS_CONTROL)
#define dbg_hw(level, fmt, args...) \
	dbg_common((debug_hw) >= (level), "[HW]", fmt, ##args)
#define mdbg_hw(level, fmt, instance, args...) \
	dbg_common((debug_hw) >= (level), "[%d][HW]", fmt, instance, ##args)
#define sdbg_hw(level, fmt, object, args...) \
	dbg_common((debug_hw) >= (level), "[HW:%s]", fmt, object->name, ##args)
#define msdbg_hw(level, fmt, instance, object, args...) \
	dbg_common((debug_hw) >= (level), "[%d][HW:%s]", fmt, instance, object->name, ##args)

#define info_hw(fmt, args...) \
	info_common("[HW]", fmt, ##args)
#define minfo_hw(fmt, instance, args...) \
	info_common("[%d][HW]", fmt, instance, ##args)
#define sinfo_hw(fmt, object, args...) \
	info_common("[HW:%s]", fmt, object->name, ##args)
#define msinfo_hw(fmt, instance, object, args...) \
	info_common("[%d][HW:%s]", fmt, instance, object->name, ##args)

#define warn_hw(fmt, args...) \
	warn_common("[HW][WRN]%d:", fmt "\n", __LINE__, ##args)
#define mwarn_hw(fmt, instance, args...) \
	warn_common("[%d][HW][WRN]%d:", fmt "\n", instance, __LINE__,  ##args)
#define swarn_hw(fmt, object, args...) \
	warn_common("[HW:%s][WRN]%d:", fmt "\n", object->name, __LINE__,  ##args)
#define mswarn_hw(fmt, instance, object, args...) \
	warn_common("[%d][HW:%s][WRN]%d:", fmt "\n", instance, object->name, __LINE__,  ##args)

#define err_hw(fmt, args...) \
	err_common("[HW][ERR]%s:%d:", fmt "\n", __func__, __LINE__, ##args)
#define merr_hw(fmt, instance, args...) \
	err_common("[%d][HW][ERR]%s:%d:", fmt "\n", instance, __func__, __LINE__, ##args)
#define serr_hw(fmt, object, args...) \
	err_common("[HW:%s][ERR]%s:%d:", fmt "\n", object->name, __func__, __LINE__, ##args)
#define mserr_hw(fmt, instance, object, args...) \
	err_common("[%d][HW:%s][ERR]%s:%d:", fmt "\n", instance, object->name, __func__, __LINE__, ##args)


#define dbg_lib(level, fmt, args...) \
	dbg_common((debug_hw) >= (level), "[LIB]", fmt, ##args)
#define info_lib(fmt, args...) \
	info_common("[LIB]", fmt, ##args)
#define minfo_lib(fmt, instance, args...) \
	info_common("[%d][LIB]", fmt, instance, ##args)
#define msinfo_lib(fmt, instance, object, args...) \
	info_common("[%d][LIB:%s]", fmt, instance, object->name, ##args)
#define warn_lib(fmt, args...) \
	warn_common("[LIB][WARN]%d:", fmt "\n", __LINE__, ##args)
#define err_lib(fmt, args...) \
	err_common("[LIB][ERR]%s:%d:", fmt "\n", __func__, __LINE__, ##args)
#define mserr_lib(fmt, instance, object, args...) \
	err_common("[%d][LIB:%s][ERR]%s:%d:", fmt "\n", instance, object->name, __func__, __LINE__, ##args)

#define dbg_itfc(fmt, args...) \
	dbg_common(debug_hw, "[ITFC]", fmt, ##args)
#define info_itfc(fmt, args...) \
	info_common("[ITFC]", fmt, ##args)
#define err_itfc(fmt, args...) \
	err_common("[ITFC][ERR]%s:%d:", fmt "\n", __func__, __LINE__, ##args)
#endif

#ifdef USE_FIMC_BUG
#define FIMC_BUG(condition)									\
	{											\
		if (unlikely(condition)) {							\
			info("[BUG][%s] %s:%d(%s)\n", __FILE__, __func__, __LINE__, #condition);\
			return -EINVAL;								\
		}										\
	}
#define FIMC_BUG_VOID(condition)								\
	{											\
		if (unlikely(condition)) {							\
			info("[BUG][%s] %s:%d(%s)\n", __FILE__, __func__, __LINE__, #condition);\
			return;									\
		}										\
	}
#define FIMC_BUG_NULL(condition)								\
	{											\
		if (unlikely(condition)) {							\
			info("[BUG][%s] %s:%d(%s)\n", __FILE__, __func__, __LINE__, #condition);\
			return NULL;								\
		}										\
	}
#define FIMC_BUG_FALSE(condition)								\
	{											\
		if (unlikely(condition)) {							\
			info("[BUG][%s] %s:%d(%s)\n", __FILE__, __func__, __LINE__, #condition);\
			return false;								\
		}										\
	}
#else
#define FIMC_BUG(condition)									\
	BUG_ON(condition);
#define FIMC_BUG_VOID(condition)								\
	BUG_ON(condition);
#define FIMC_BUG_NULL(condition)								\
	BUG_ON(condition);
#endif

#endif
