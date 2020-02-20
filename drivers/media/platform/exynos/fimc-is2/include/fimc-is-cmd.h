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

#ifndef FIMC_IS_CMD_H
#define FIMC_IS_CMD_H

#include "fimc-is-config.h"

#define IS_COMMAND_VER 132 /* IS COMMAND VERSION 1.32 */

enum is_cmd {
	/* HOST -> IS */
	HIC_PREVIEW_STILL = 0x1,
	HIC_PREVIEW_VIDEO,
	HIC_CAPTURE_STILL,
	HIC_CAPTURE_VIDEO,
	HIC_PROCESS_START,
	HIC_PROCESS_STOP,
	HIC_STREAM_ON /* 7 */,
	HIC_STREAM_OFF /* 8 */,
	HIC_SHOT,
	HIC_GET_STATIC_METADATA /* 10 */,
	HIC_SET_CAM_CONTROL,
	HIC_GET_CAM_CONTROL,
	HIC_SET_PARAMETER /* 13 */,
	HIC_GET_PARAMETER,
	HIC_SET_A5_MAP /* 15 */,
	HIC_SET_A5_UNMAP /* 16 */,
	HIC_FAULT,
	/* SENSOR PART*/
	HIC_OPEN_SENSOR,
	HIC_CLOSE_SENSOR,
	HIC_SIMMIAN_INIT /* 20 */,
	HIC_SIMMIAN_WRITE,
	HIC_SIMMIAN_READ,
	HIC_POWER_DOWN,
	HIC_GET_SET_FILE_ADDR,
	HIC_LOAD_SET_FILE,
	HIC_MSG_CONFIG,
	HIC_MSG_TEST,
	HIC_ISP_I2C_CONTROL,
	HIC_CALIBRATE_ACTUATOR,
	HIC_GET_IP_STATUS /* 30 */,
	HIC_I2C_CONTROL_LOCK,
	HIC_SYSTEM_CONTROL,
	HIC_SENSOR_MODE_CHANGE,
	HIC_ADJUST_SET_FILE,
	HIC_CHECK_A5_TASK_CPU_USAGE,
	HIC_COMMAND_END,

	/* IS -> HOST */
	IHC_GET_SENSOR_NUMBER = 0x1000,
	/* Parameter1 : Address of space to copy a setfile */
	/* Parameter2 : Space szie */
	IHC_SET_SHOT_MARK,
	/* PARAM1 : a frame number */
	/* PARAM2 : confidence level(smile 0~100) */
	/* PARMA3 : confidence level(blink 0~100) */
	IHC_SET_FACE_MARK,
	/* PARAM1 : coordinate count */
	/* PARAM2 : coordinate buffer address */
	IHC_FRAME_DONE,
	/* PARAM1 : frame start number */
	/* PARAM2 : frame count */
	IHC_AA_DONE,
	IHC_NOT_READY,
	IHC_FLASH_READY,
	IHC_REPORT_ERR,
	IHC_COMMAND_END
};

enum is_reply {
	ISR_DONE	= 0x2000,
	ISR_NDONE
};

enum is_scenario_id {
	ISS_PREVIEW_STILL,
	ISS_PREVIEW_VIDEO,
	ISS_CAPTURE_STILL,
	ISS_CAPTURE_VIDEO,
	ISS_END
};

enum is_subscenario_id {
	ISS_SUB_SCENARIO_STILL_PREVIEW = 0,	/* 0: still preview */
	ISS_SUB_SCENARIO_VIDEO = 1,		/* 1: video */
	ISS_SUB_SCENARIO_UHD_30FPS_WDR_ON = 2,	/* 2: video UHD QHD 30fps (HDR ON) */
	ISS_SUB_SCENARIO_UHD_30FPS_WDR_AUTO = 3,/* 3: video UHD QHD 30fps (HDR AUTO) */
	ISS_SUB_SCENARIO_VIDEO_HIGH_SPEED = 4,	/* 4: video high speed */
	ISS_SUB_SCENARIO_STILL_CAPTURE = 5,	/* 5: still capture */
	ISS_SUB_SCENARIO_FHD_60FPS = 6,		/* 6: video FHD 60fps (HDR OFF)*/
	ISS_SUB_SCENARIO_UHD_30FPS = 7,		/* 7: video UHD QHD 30fps (HDR OFF) */
	ISS_SUB_SCENARIO_WVGA_300FPS = 8,	/* 8: video WVGA 300fps */
	ISS_SUB_SCENARIO_STILL_PREVIEW_WDR_ON = 9,
	ISS_SUB_SCENARIO_VIDEO_WDR_ON = 10,
	ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON = 11,
	ISS_SUB_SCENARIO_STILL_CAPTURE_ZOOM = 13,
	ISS_SUB_SCENARIO_STILL_CAPTURE_ZOOM_INDOOR = 14,
	ISS_SUB_SCENARIO_STILL_CAPTURE_ZOOM_OUTDOOR = 15,
	ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON_ZOOM = 16,
	ISS_SUB_SCENARIO_STILL_CAPTURE_DNG_REPROCESSING = 17,
	ISS_SUB_SCENARIO_FHD_60FPS_WDR_ON = 18,		/* 18: video FHD 60fps (HDR ON) */
	ISS_SUB_SCENARIO_STILL_CAPTURE_LLS = 19,
	ISS_SUB_SCENARIO_MERGED_STILL_CAPTURE_WDR_AUTO = ISS_SUB_SCENARIO_STILL_CAPTURE_LLS,
	ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON_LLS = 20,
	ISS_SUB_SCENARIO_MERGED_STILL_CAPTURE_WDR_ON = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON_LLS,
	ISS_SUB_SCENARIO_UHD_60FPS_WDR_ON = 21,		/* 21: video UHD 60fps (HDR ON) */
	ISS_SUB_SCENARIO_UHD_60FPS = 22,		/* 22: video UHD 60fps (HDR OFF) */
	ISS_SUB_SCENARIO_UHD_60FPS_WDR_AUTO = 23,	/* 23: video UHD 60fps (HDR AUTO) */
	ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_AUTO = 24,
	ISS_SUB_SCENARIO_VIDEO_WDR_AUTO = 25,
	ISS_SUB_SCENARIO_STILL_PREVIEW_WDR_AUTO = 26,
	ISS_SUB_SCENARIO_FHD_240FPS = 27,
	ISS_SUB_SCENARIO_MERGED_STILL_CAPTURE = 28,
	ISS_SUB_SCENARIO_FHD_60FPS_WDR_AUTO = 29,	/* 29: video FHD 60fps (HDR AUTO) */
	ISS_SUB_SCENARIO_STILL_CAPTURE_LONG = 30,
	ISS_SUB_SCENARIO_STILL_PREVIEW_BINNING = 40,	/* 40: binning mode for low power */
	ISS_SUB_SCENARIO_STILL_CAPTURE_BDS = 41,	/* 41: still capture bds */
	ISS_SUB_SCENARIO_STILL_PREVIEW_LLS = 42,	/* 42: still preview lls */
	ISS_SUB_SCENARIO_LIVE_OUTFOCUS_PREVIEW = 44,	/* 44 ~ 52: Bokeh (HDR off/auto/on) */
	ISS_SUB_SCENARIO_LIVE_OUTFOCUS_CAPTURE = 45,
	ISS_SUB_SCENARIO_LIVE_OUTFOCUS_PREVIEW_WDR_AUTO = 47,
	ISS_SUB_SCENARIO_LIVE_OUTFOCUS_PREVIEW_WDR_ON = 48,
	ISS_SUB_SCENARIO_LIVE_OUTFOCUS_CAPTURE_WDR_AUTO = 49,
	ISS_SUB_SCENARIO_LIVE_OUTFOCUS_CAPTURE_WDR_ON = 50,
	ISS_SUB_SCENARIO_STILL_PREVIEW_3RD_PARTY_WDR_AUTO = 51,
	ISS_SUB_SCENARIO_VIDEO_3RD_PARTY_WDR_AUTO = 52,
	ISS_SUB_SCENARIO_MERGED_STILL_CAPTURE_MFHDR = 53,
	ISS_SUB_SCENARIO_MERGED_STILL_CAPTURE_LLHDR = 54,
	ISS_SUB_SCENARIO_SUPER_NIGHT_SHOT = 55,
	ISS_SUB_SCENARIO_FHD_VIDEO_HDR10_WDR_ON = 56,
	ISS_SUB_SCENARIO_UHD_VIDEO_HDR10_WDR_ON = 57,
	ISS_SUB_SCENARIO_BINNING = 58,
	ISS_SUB_SCENARIO_VIDEO_ISPLP_TNR = 59,
	ISS_SUB_SCENARIO_STILL_NIGHT_HDR = 60,
	ISS_SUB_SCENARIO_VIDEO_SUPER_STEADY = 61,
	ISS_SUB_SCENARIO_STILL_FLASH_LLS = 62,
	ISS_SUB_SCENARIO_STILL_CAPTURE_3RD_PARTY_WDR_AUTO = 63,

	ISS_SUB_SCENARIO_FRONT_VT1 = 31,			/* 31: front camera VT1 */
	ISS_SUB_SCENARIO_FRONT_VT2 = 32,			/* 32: front camera VT2 */
	ISS_SUB_SCENARIO_FRONT_SMART_STAY = 33,			/* 33: front camera smart stay */
	ISS_SUB_SCENARIO_FRONT_PANORAMA = 34,			/* 34: front camera front panorama */
	ISS_SUB_SCENARIO_FRONT_C2_OFF_STILL_PREVIEW = 35,	/* 35: C2 off front still preview */
	ISS_SUB_SCENARIO_FRONT_C2_OFF_STILL_CAPTURE = 36,	/* 36: C2 off front still capture */
	ISS_SUB_SCENARIO_FRONT_C2_OFF_VIDEO = 37,		/* 37: C2 off front video */
	ISS_SUB_SCENARIO_FRONT_VT4 = 38,			/* 38: front camera VT4 */
	ISS_SUB_SCENARIO_FRONT_VT1_STILL_CAPTURE = 39,		/* 39: front camera VT1 still capture */
	ISS_SUB_SCENARIO_FRONT_STILL_PREVIEW_BINNING = 40,	/* 40: front camera binning mode for low power */
	ISS_SUB_SCENARIO_FRONT_COLOR_IRIS_PREVIEW = 43,         /* 43: front camera Color Iris preview */
	ISS_SUB_END,
};

#define IS_VIDEO_SCENARIO(setfile)				\
	(((setfile) == ISS_SUB_SCENARIO_VIDEO)			\
	|| ((setfile) == ISS_SUB_SCENARIO_VIDEO_WDR_AUTO)	\
	|| ((setfile) == ISS_SUB_SCENARIO_VIDEO_WDR_ON)		\
	|| ((setfile) == ISS_SUB_SCENARIO_FHD_60FPS)		\
	|| ((setfile) == ISS_SUB_SCENARIO_FHD_60FPS_WDR_AUTO)	\
	|| ((setfile) == ISS_SUB_SCENARIO_UHD_30FPS)		\
	|| ((setfile) == ISS_SUB_SCENARIO_UHD_30FPS_WDR_AUTO)	\
	|| ((setfile) == ISS_SUB_SCENARIO_UHD_30FPS_WDR_ON)	\
	|| ((setfile) == ISS_SUB_SCENARIO_UHD_60FPS)		\
	|| ((setfile) == ISS_SUB_SCENARIO_UHD_60FPS_WDR_AUTO)	\
	|| ((setfile) == ISS_SUB_SCENARIO_WVGA_300FPS)		\
	|| ((setfile) == ISS_SUB_SCENARIO_FRONT_C2_OFF_VIDEO)	\
	|| ((setfile) == ISS_SUB_SCENARIO_FHD_240FPS))

enum is_scenario_is {
	FIMC_IS_SCENARIO_SWVDIS = 1,
	FIMC_IS_SCENARIO_COLOR_IRIS = 2,
	FIMC_IS_SCENARIO_AUTO_DUAL = 3,
	FIMC_IS_SCENARIO_FULL_SIZE = 4,
	FIMC_IS_SCENARIO_HIGH_SPEED_DUALFPS = 5, /* FPS is changed from normal to high speed */
	FIMC_IS_SCENARIO_SECURE = 6,
	FIMC_IS_SCENAIRO_REMOSAIC = 7,
};

enum is_system_control_id {
	IS_SYS_CLOCK_GATE	= 0,
	IS_SYS_END		= 1,
};

enum is_system_control_cmd {
	SYS_CONTROL_DISABLE	= 0,
	SYS_CONTROL_ENABLE	= 1,
};

enum is_msg_test_id {
	IS_MSG_TEST_SYNC_LOG = 1,
};

struct is_setfile_header_element {
	u32 binary_addr;
	u32 binary_size;
};

#ifdef CONFIG_USE_SENSOR_GROUP
#define MAX_ACTIVE_GROUP 8
#else
#define MAX_ACTIVE_GROUP 6
#endif
struct fimc_is_path_info {
	u32 sensor_name;
	u32 mipi_csi;
	u32 fimc_lite;
	/*
	 * uActiveGroup[0] = GROUP_ID_0/GROUP_ID_1/0XFFFFFFFF
	 * uActiveGroup[1] = GROUP_ID_2/GROUP_ID_3/0XFFFFFFFF
	 * uActiveGroup[2] = GROUP_ID_4/0XFFFFFFFF
	 * uActiveGroup[3] = GROUP_ID_5(MCSC0)/GROUP_ID_6(MCSC1)/0XFFFFFFFF
	 * uActiveGroup[4] = GROUP_ID_7/0XFFFFFFFF
	 */
	u32 group[MAX_ACTIVE_GROUP];
};

struct is_setfile_header {
	struct is_setfile_header_element isp[ISS_END];
	struct is_setfile_header_element drc[ISS_END];
	struct is_setfile_header_element fd[ISS_END];
};

#define HOST_SET_INT_BIT	0x00000001
#define HOST_CLR_INT_BIT	0x00000001
#define IS_SET_INT_BIT		0x00000001
#define IS_CLR_INT_BIT		0x00000001

#define HOST_SET_INTERRUPT(base)	(base->uiINTGR0 |= HOST_SET_INT_BIT)
#define HOST_CLR_INTERRUPT(base)	(base->uiINTCR0 |= HOST_CLR_INT_BIT)
#define IS_SET_INTERRUPT(base)		(base->uiINTGR1 |= IS_SET_INT_BIT)
#define IS_CLR_INTERRUPT(base)		(base->uiINTCR1 |= IS_CLR_INT_BIT)

struct is_common_reg {
	u32 hicmd;
	u32 hic_stream;
	u32 hic_param1;
	u32 hic_param2;
	u32 hic_param3;
	u32 hic_param4;

	u32 ihcmd;
	u32 ihc_stream;
	u32 ihc_param1;
	u32 ihc_param2;
	u32 ihc_param3;
	u32 ihc_param4;

	u32 shot_stream;
	u32 shot_param1;
	u32 shot_param2;

	u32 t0c_stream;
	u32 t0c_param1;
	u32 t0c_param2;
	u32 t0p_stream;
	u32 t0p_param1;
	u32 t0p_param2;

	u32 t1c_stream;
	u32 t1c_param1;
	u32 t1c_param2;
	u32 t1p_stream;
	u32 t1p_param1;
	u32 t1p_param2;

	u32 i0x_stream;
	u32 i0x_param1;
	u32 i0x_param2;
	u32 i0x_param3;

	u32 i1x_stream;
	u32 i1x_param1;
	u32 i1x_param2;
	u32 i1x_param3;

	u32 scx_stream;
	u32 scx_param1;
	u32 scx_param2;
	u32 scx_param3;

	u32 m0x_stream;
	u32 m0x_param1;
	u32 m0x_param2;
	u32 m0x_param3;

	u32 m1x_stream;
	u32 m1x_param1;
	u32 m1x_param2;
	u32 m1x_param3;

	u32 reserved[10];
	u32 set_fwboot;

	u32 cgate_status;
	u32 pdown_status;
	u32 fcount_sen3;
	u32 fcount_sen2;
	u32 fcount_sen1;
	u32 fcount_sen0;
};

struct is_mcuctl_reg {
	u32 mcuctl;
	u32 bboar;

	u32 intgr0;
	u32 intcr0;
	u32 intmr0;
	u32 intsr0;
	u32 intmsr0;

	u32 intgr1;
	u32 intcr1;
	u32 intmr1;
	u32 intsr1;
	u32 intmsr1;

	u32 intcr2;
	u32 intmr2;
	u32 intsr2;
	u32 intmsr2;

	u32 gpoctrl;
	u32 cpoenctlr;
	u32 gpictlr;

	u32 pad[0xD];

	struct is_common_reg common_reg;
};
#endif
