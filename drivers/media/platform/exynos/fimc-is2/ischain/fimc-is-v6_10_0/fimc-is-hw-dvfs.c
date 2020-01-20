/*
 * Samsung Exynos SoC series FIMC-IS driver
 *
 * exynos fimc-is2 core functions
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-core.h"
#include "fimc-is-dvfs.h"

/* for DT parsing */
DECLARE_DVFS_DT(FIMC_IS_SN_END,
		{"default_"		           , FIMC_IS_SN_DEFAULT},
		{"secure_front_"                   , FIMC_IS_SN_SECURE_FRONT},
		{"front_preview_"                  , FIMC_IS_SN_FRONT_PREVIEW},
		{"front_preview_full_"             , FIMC_IS_SN_FRONT_PREVIEW_FULL},
		{"front_capture_"                  , FIMC_IS_SN_FRONT_CAPTURE},
		{"front_video_"                    , FIMC_IS_SN_FRONT_CAMCORDING},
		{"front_video_capture_"            , FIMC_IS_SN_FRONT_CAMCORDING_CAPTURE},
		{"front_wide_selfie_" 	   , FIMC_IS_SN_FRONT_WIDE_SELFIE},
		{"front_vt1_"                      , FIMC_IS_SN_FRONT_VT1},
		{"front_vt2_"                      , FIMC_IS_SN_FRONT_VT2},
		{"front_vt4_"                      , FIMC_IS_SN_FRONT_VT4},
		{"rear_preview_full_"              , FIMC_IS_SN_REAR_PREVIEW_FULL},
		{"rear_preview_fhd_"               , FIMC_IS_SN_REAR_PREVIEW_FHD},
		{"rear_preview_hd_"                , FIMC_IS_SN_REAR_PREVIEW_HD},
		{"rear_preview_uhd_"               , FIMC_IS_SN_REAR_PREVIEW_UHD},
		{"rear_capture_"                   , FIMC_IS_SN_REAR_CAPTURE},
		{"rear_video_fhd_"                 , FIMC_IS_SN_REAR_CAMCORDING_FHD},
		{"rear_video_hd_"				   , FIMC_IS_SN_REAR_CAMCORDING_HD},
		{"rear_video_uhd_"                 , FIMC_IS_SN_REAR_CAMCORDING_UHD},
		{"rear_video_fhd_capture_"         , FIMC_IS_SN_REAR_CAMCORDING_FHD_CAPTURE},
		{"rear_video_hd_capture_"		   , FIMC_IS_SN_REAR_CAMCORDING_HD_CAPTURE},
		{"rear_video_uhd_capture_"         , FIMC_IS_SN_REAR_CAMCORDING_UHD_CAPTURE},
		{"pip_preview_"                    , FIMC_IS_SN_PIP_PREVIEW},
		{"pip_capture_"                    , FIMC_IS_SN_PIP_CAPTURE},
		{"pip_video_"                      , FIMC_IS_SN_PIP_CAMCORDING},
		{"pip_video_capture_"              , FIMC_IS_SN_PIP_CAMCORDING_CAPTURE},
		{"preview_high_speed_fps_"         , FIMC_IS_SN_PREVIEW_HIGH_SPEED_FPS},
		{"video_high_speed_60fps_"         , FIMC_IS_SN_VIDEO_HIGH_SPEED_60FPS},
		{"video_high_speed_480fps_" 	   , FIMC_IS_SN_VIDEO_HIGH_SPEED_480FPS},
		{"video_high_speed_60fps_capture_" , FIMC_IS_SN_VIDEO_HIGH_SPEED_60FPS_CAPTURE},
		{"ext_rear_"		     , FIMC_IS_SN_EXT_REAR},
		{"ext_front_"		     , FIMC_IS_SN_EXT_FRONT},
		{"ext_secure_"			 , FIMC_IS_SN_EXT_SECURE},
		{"max_"                            , FIMC_IS_SN_MAX});

/* dvfs scenario check logic data */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_SECURE_FRONT);

DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_PREVIEW_FULL);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_PREVIEW);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_CAPTURE);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_CAMCORDING);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_CAMCORDING_CAPTURE);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_WIDE_SELFIE);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_VT1);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_VT2);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_VT4);

DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_PREVIEW_FULL);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_PREVIEW_FHD);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_PREVIEW_HD);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_PREVIEW_UHD);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAPTURE);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAMCORDING_FHD);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAMCORDING_HD);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAMCORDING_UHD);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAMCORDING_FHD_CAPTURE);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAMCORDING_HD_CAPTURE);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAMCORDING_UHD_CAPTURE);

DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_PIP_PREVIEW);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_PIP_CAPTURE);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_PIP_CAMCORDING);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_PIP_CAMCORDING_CAPTURE);

DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_PREVIEW_HIGH_SPEED_FPS);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_VIDEO_HIGH_SPEED_60FPS);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_VIDEO_HIGH_SPEED_480FPS);
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_VIDEO_HIGH_SPEED_60FPS_CAPTURE);

/* external isp's dvfs */
DECLARE_EXT_DVFS_CHK_FUNC(FIMC_IS_SN_EXT_REAR);
DECLARE_EXT_DVFS_CHK_FUNC(FIMC_IS_SN_EXT_FRONT);
DECLARE_EXT_DVFS_CHK_FUNC(FIMC_IS_SN_EXT_SECURE);


#if defined(ENABLE_DVFS)
/*
 * Static Scenario Set
 * You should describe static scenario by priorities of scenario.
 * And you should name array 'static_scenarios'
 */

struct fimc_is_dvfs_scenario static_scenarios[] = {
	{
		.scenario_id		= FIMC_IS_SN_PREVIEW_HIGH_SPEED_FPS,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_PREVIEW_HIGH_SPEED_FPS),
		.check_func		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_PREVIEW_HIGH_SPEED_FPS),
	}, {
		.scenario_id		= FIMC_IS_SN_SECURE_FRONT,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_SECURE_FRONT),
		.check_func 		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_SECURE_FRONT),
	}, {
		.scenario_id		= FIMC_IS_SN_PIP_CAMCORDING,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_PIP_CAMCORDING),
		.check_func		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_PIP_CAMCORDING),
	}, {
		.scenario_id		= FIMC_IS_SN_PIP_PREVIEW,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_PIP_PREVIEW),
		.check_func		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_PIP_PREVIEW),
	}, {
		.scenario_id		= FIMC_IS_SN_VIDEO_HIGH_SPEED_60FPS,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_VIDEO_HIGH_SPEED_60FPS),
		.check_func		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_VIDEO_HIGH_SPEED_60FPS),
	}, {
		.scenario_id		= FIMC_IS_SN_VIDEO_HIGH_SPEED_480FPS,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_VIDEO_HIGH_SPEED_480FPS),
		.check_func 	= GET_DVFS_CHK_FUNC(FIMC_IS_SN_VIDEO_HIGH_SPEED_480FPS),
	}, {
		.scenario_id		= FIMC_IS_SN_REAR_CAMCORDING_HD,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_REAR_CAMCORDING_HD),
		.check_func 	= GET_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAMCORDING_HD),
	}, {
		.scenario_id		= FIMC_IS_SN_REAR_CAMCORDING_FHD,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_REAR_CAMCORDING_FHD),
		.check_func 	= GET_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAMCORDING_FHD),
	}, {
		.scenario_id		= FIMC_IS_SN_REAR_CAMCORDING_UHD,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_REAR_CAMCORDING_UHD),
		.check_func		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAMCORDING_UHD),
	}, {
		.scenario_id		= FIMC_IS_SN_REAR_PREVIEW_FULL,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_REAR_PREVIEW_FULL),
		.check_func 	= GET_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_PREVIEW_FULL),
	}, {
		.scenario_id		= FIMC_IS_SN_REAR_PREVIEW_HD,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_REAR_PREVIEW_HD),
		.check_func 	= GET_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_PREVIEW_HD),
	}, {
		.scenario_id		= FIMC_IS_SN_REAR_PREVIEW_FHD,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_REAR_PREVIEW_FHD),
		.check_func 	= GET_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_PREVIEW_FHD),
	}, {
		.scenario_id		= FIMC_IS_SN_REAR_PREVIEW_UHD,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_REAR_PREVIEW_UHD),
		.check_func		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_PREVIEW_UHD),
	}, {
		.scenario_id		= FIMC_IS_SN_FRONT_VT1,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_FRONT_VT1),
		.check_func		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_VT1),
	}, {
		.scenario_id		= FIMC_IS_SN_FRONT_VT2,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_FRONT_VT2),
		.check_func		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_VT2),
	}, {
		.scenario_id		= FIMC_IS_SN_FRONT_VT4,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_FRONT_VT4),
		.check_func		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_VT4),
	}, {
		.scenario_id		= FIMC_IS_SN_FRONT_CAMCORDING,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_FRONT_CAMCORDING),
		.check_func		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_CAMCORDING),
	}, {
		.scenario_id		= FIMC_IS_SN_FRONT_PREVIEW_FULL,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_FRONT_PREVIEW_FULL),
		.check_func 	= GET_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_PREVIEW_FULL),
	}, {
		.scenario_id		= FIMC_IS_SN_FRONT_PREVIEW,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_FRONT_PREVIEW),
		.check_func		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_PREVIEW),
	}, {
		.scenario_id		= FIMC_IS_SN_FRONT_WIDE_SELFIE,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_FRONT_WIDE_SELFIE),
		.check_func 	= GET_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_WIDE_SELFIE),
	}
};

/*
 * Dynamic Scenario Set
 * You should describe static scenario by priorities of scenario.
 * And you should name array 'dynamic_scenarios'
 */
static struct fimc_is_dvfs_scenario dynamic_scenarios[] = {
	{
		.scenario_id		= FIMC_IS_SN_PIP_CAMCORDING_CAPTURE,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_PIP_CAMCORDING_CAPTURE),
		.keep_frame_tick	= FIMC_IS_DVFS_DUAL_CAPTURE_TICK,
		.check_func		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_PIP_CAMCORDING_CAPTURE),
	}, {
		.scenario_id		= FIMC_IS_SN_PIP_CAPTURE,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_PIP_CAPTURE),
		.keep_frame_tick	= FIMC_IS_DVFS_DUAL_CAPTURE_TICK,
		.check_func 		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_PIP_CAPTURE),
	}, {
		.scenario_id		= FIMC_IS_SN_REAR_CAMCORDING_HD_CAPTURE,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_REAR_CAMCORDING_HD_CAPTURE),
		.keep_frame_tick	= FIMC_IS_DVFS_CAPTURE_TICK,
		.check_func 	= GET_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAMCORDING_HD_CAPTURE),
	}, {
		.scenario_id		= FIMC_IS_SN_REAR_CAMCORDING_FHD_CAPTURE,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_REAR_CAMCORDING_FHD_CAPTURE),
		.keep_frame_tick	= FIMC_IS_DVFS_CAPTURE_TICK,
		.check_func 	= GET_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAMCORDING_FHD_CAPTURE),
	}, {
		.scenario_id		= FIMC_IS_SN_VIDEO_HIGH_SPEED_60FPS_CAPTURE,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_VIDEO_HIGH_SPEED_60FPS_CAPTURE),
		.keep_frame_tick	= FIMC_IS_DVFS_CAPTURE_TICK,
		.check_func 	= GET_DVFS_CHK_FUNC(FIMC_IS_SN_VIDEO_HIGH_SPEED_60FPS_CAPTURE),
	}, {
		.scenario_id		= FIMC_IS_SN_REAR_CAMCORDING_UHD_CAPTURE,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_REAR_CAMCORDING_UHD_CAPTURE),
		.keep_frame_tick	= FIMC_IS_DVFS_CAPTURE_TICK,
		.check_func		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAMCORDING_UHD_CAPTURE),
	}, {
		.scenario_id		= FIMC_IS_SN_REAR_CAPTURE,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_REAR_CAPTURE),
		.keep_frame_tick	= FIMC_IS_DVFS_CAPTURE_TICK,
		.check_func		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAPTURE),
	}, {
		.scenario_id		= FIMC_IS_SN_FRONT_CAMCORDING_CAPTURE,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_FRONT_CAMCORDING_CAPTURE),
		.keep_frame_tick	= FIMC_IS_DVFS_CAPTURE_TICK,
		.check_func		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_CAMCORDING_CAPTURE),
	}, {
		.scenario_id		= FIMC_IS_SN_FRONT_CAPTURE,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_FRONT_CAPTURE),
		.keep_frame_tick	= FIMC_IS_DVFS_CAPTURE_TICK,
		.check_func		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_CAPTURE),
	},
};

/*
 * External Sensor/Vision Scenario Set
 * You should describe external scenario by priorities of scenario.
 * And you should name array 'external_scenarios'
 */
struct fimc_is_dvfs_scenario external_scenarios[] = {
	{
		.scenario_id		= FIMC_IS_SN_EXT_REAR,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_EXT_REAR),
		.ext_check_func		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_EXT_REAR),
	}, {
		.scenario_id		= FIMC_IS_SN_EXT_FRONT,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_EXT_FRONT),
		.ext_check_func		= GET_DVFS_CHK_FUNC(FIMC_IS_SN_EXT_FRONT),
	}, {
		.scenario_id		= FIMC_IS_SN_EXT_SECURE,
		.scenario_nm		= DVFS_SN_STR(FIMC_IS_SN_EXT_SECURE),
		.ext_check_func 	= GET_DVFS_CHK_FUNC(FIMC_IS_SN_EXT_SECURE),
	},
};
#else
/*
 * Default Scenario can not be seleted, this declaration is for static variable.
 */
static struct fimc_is_dvfs_scenario static_scenarios[] = {
	{
		.scenario_id		= FIMC_IS_SN_DEFAULT,
		.scenario_nm		= NULL,
		.keep_frame_tick	= 0,
		.check_func		= NULL,
	},
};

static struct fimc_is_dvfs_scenario dynamic_scenarios[] = {
	{
		.scenario_id		= FIMC_IS_SN_DEFAULT,
		.scenario_nm		= NULL,
		.keep_frame_tick	= 0,
		.check_func		= NULL,
	},
};

static struct fimc_is_dvfs_scenario external_scenarios[] = {
	{
		.scenario_id		= FIMC_IS_SN_DEFAULT,
		.scenario_nm		= NULL,
		.keep_frame_tick	= 0,
		.ext_check_func		= NULL,
	},
};
#endif

/* fastAE */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_PREVIEW_HIGH_SPEED_FPS)
{
	u32 mask = (device->setfile & FIMC_IS_SETFILE_MASK);
	bool setfile_flag = (mask == ISS_SUB_SCENARIO_FHD_60FPS) ||
		(mask == ISS_SUB_SCENARIO_VIDEO_HIGH_SPEED) ||
		(mask == ISS_SUB_SCENARIO_FHD_240FPS);

	if ((fps > 30) && !setfile_flag)
		return 1;
	else
		return 0;
}

/* secure front */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_SECURE_FRONT)
{
	u32 scen = (device->setfile & FIMC_IS_SCENARIO_MASK) >> FIMC_IS_SCENARIO_SHIFT;
	bool scenario_flag = (scen == FIMC_IS_SCENARIO_COLOR_IRIS);

	if (scenario_flag && stream_cnt > 1)
		return 1;
	else
		return 0;
}

/* pip camcording */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_PIP_CAMCORDING)
{
	return 0;
}

DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_PIP_CAMCORDING_CAPTURE)
{
	struct fimc_is_dvfs_scenario_ctrl *static_ctrl = device->resourcemgr->dvfs_ctrl.static_ctrl;

	if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state) &&
		(static_ctrl->cur_scenario_id == FIMC_IS_SN_PIP_CAMCORDING))
		return 1;
	else
		return 0;
}

/* pip preview */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_PIP_PREVIEW)
{
	if (stream_cnt > 1)
		return 1;
	else
		return 0;
}

DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_PIP_CAPTURE)
{
	struct fimc_is_dvfs_scenario_ctrl *static_ctrl = device->resourcemgr->dvfs_ctrl.static_ctrl;

	if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state) &&
		(static_ctrl->cur_scenario_id == FIMC_IS_SN_PIP_PREVIEW))
		return 1;
	else
		return 0;
}

/* 60fps recording */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_VIDEO_HIGH_SPEED_60FPS)
{
	u32 mask = (device->setfile & FIMC_IS_SETFILE_MASK);
	bool setfile_flag = (mask == ISS_SUB_SCENARIO_FHD_60FPS);

	if ((position == SENSOR_POSITION_REAR || position == SENSOR_POSITION_REAR2) &&
			(fps >= 60) &&
			(fps < 120) && setfile_flag)
		return 1;
	else
		return 0;
}

/* 480fps recording */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_VIDEO_HIGH_SPEED_480FPS)
{
	u32 mask = (device->setfile & FIMC_IS_SETFILE_MASK);
	bool setfile_flag = (mask == ISS_SUB_SCENARIO_FHD_240FPS);

	if ((position == SENSOR_POSITION_REAR || position == SENSOR_POSITION_REAR2) &&
			(fps >= 480) &&  setfile_flag)
		return 1;
	else
		return 0;
}

/* rear camcording FHD*/
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAMCORDING_FHD)
{
	u32 mask = (device->setfile & FIMC_IS_SETFILE_MASK);
	bool setfile_flag = ((mask == ISS_SUB_SCENARIO_VIDEO) ||
			(mask == ISS_SUB_SCENARIO_VIDEO_WDR_ON) ||
			(mask == ISS_SUB_SCENARIO_VIDEO_WDR_AUTO));

	if ((position == SENSOR_POSITION_REAR) &&
			(fps <= 30) &&
			(resol <= SIZE_12MP_FHD_BDS) &&
			setfile_flag)
		return 1;
	else
		return 0;
}

/* rear camcording HD*/
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAMCORDING_HD)
{
	u32 mask = (device->setfile & FIMC_IS_SETFILE_MASK);
	bool setfile_flag = ((mask == ISS_SUB_SCENARIO_VIDEO) ||
			(mask == ISS_SUB_SCENARIO_VIDEO_WDR_ON) ||
			(mask == ISS_SUB_SCENARIO_VIDEO_WDR_AUTO));

	if ((position == SENSOR_POSITION_REAR) &&
			(fps <= 30) &&
			(resol <= SIZE_HD) &&
			setfile_flag)
		return 1;
	else
		return 0;
}

/* rear camcording UHD*/
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAMCORDING_UHD)
{
	u32 mask = (device->setfile & FIMC_IS_SETFILE_MASK);
	bool setfile_flag = ((mask == ISS_SUB_SCENARIO_UHD_30FPS) ||
			(mask == ISS_SUB_SCENARIO_UHD_30FPS_WDR_ON));

	if ((position == SENSOR_POSITION_REAR || position == SENSOR_POSITION_REAR2) &&
			(fps <= 30) &&
			(resol > SIZE_12MP_QHD_BDS) &&
			setfile_flag)
		return 1;
	else
		return 0;
}

/* rear full resolution preview */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_PREVIEW_FULL)
{
	u32 scen = (device->setfile & FIMC_IS_SCENARIO_MASK) >> FIMC_IS_SCENARIO_SHIFT;
	bool scenario_flag = (scen == FIMC_IS_SCENARIO_FULL_SIZE);

	if ((position == SENSOR_POSITION_REAR || position == SENSOR_POSITION_REAR2) &&
		(scenario_flag))
		return 1;
	else
		return 0;
}

/* rear preview FHD */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_PREVIEW_FHD)
{
	u32 mask = (device->setfile & FIMC_IS_SETFILE_MASK);
	bool setfile_flag = ((mask == ISS_SUB_SCENARIO_VIDEO) ||
			(mask == ISS_SUB_SCENARIO_VIDEO_WDR_ON) ||
			(mask == ISS_SUB_SCENARIO_VIDEO_WDR_AUTO));

	if ((position == SENSOR_POSITION_REAR) &&
			(fps <= 30) &&
			(resol <= SIZE_12MP_FHD_BDS) &&
			(!setfile_flag))

		return 1;
	else
		return 0;
}

/* rear preview HD */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_PREVIEW_HD)
{
	u32 mask = (device->setfile & FIMC_IS_SETFILE_MASK);
	bool setfile_flag = ((mask == ISS_SUB_SCENARIO_VIDEO) ||
			(mask == ISS_SUB_SCENARIO_VIDEO_WDR_ON) ||
			(mask == ISS_SUB_SCENARIO_VIDEO_WDR_AUTO));

	if ((position == SENSOR_POSITION_REAR) &&
			(fps <= 30) &&
			(resol <= SIZE_HD) &&
			(!setfile_flag))

		return 1;
	else
		return 0;
}

/* rear preview UHD */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_PREVIEW_UHD)
{
	u32 mask = (device->setfile & FIMC_IS_SETFILE_MASK);
	bool setfile_flag = ((mask == ISS_SUB_SCENARIO_UHD_30FPS) ||
			(mask == ISS_SUB_SCENARIO_UHD_30FPS_WDR_ON));

	if ((position == SENSOR_POSITION_REAR || position == SENSOR_POSITION_REAR2) &&
			(fps <= 30) &&
			(resol > SIZE_12MP_QHD_BDS) &&
			(!setfile_flag))
		return 1;
	else
		return 0;
}

/* front vt1 */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_VT1)
{
	if ((position == SENSOR_POSITION_FRONT) &&
			((device->setfile & FIMC_IS_SETFILE_MASK) \
			 == ISS_SUB_SCENARIO_FRONT_VT1))
		return 1;
	else
		return 0;
}

/* front vt2 */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_VT2)
{
	if ((position == SENSOR_POSITION_FRONT) &&
			((device->setfile & FIMC_IS_SETFILE_MASK) \
			 == ISS_SUB_SCENARIO_FRONT_VT2))
		return 1;
	else
		return 0;
}

/* front vt4 */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_VT4)
{
	if ((position == SENSOR_POSITION_FRONT) &&
			((device->setfile & FIMC_IS_SETFILE_MASK) \
			 == ISS_SUB_SCENARIO_FRONT_VT4))
		return 1;
	else
		return 0;
}

/* front recording */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_CAMCORDING)
{
	u32 mask = (device->setfile & FIMC_IS_SETFILE_MASK);
	bool setfile_flag = ((mask == ISS_SUB_SCENARIO_VIDEO) ||
			(mask == ISS_SUB_SCENARIO_VIDEO_WDR_ON) ||
			(mask == ISS_SUB_SCENARIO_VIDEO_WDR_AUTO));

	if ((position == SENSOR_POSITION_FRONT) &&
		setfile_flag && (resol <= SIZE_8MP_FHD_BDS))
		return 1;
	else
		return 0;
}

/* front  full resolution preview */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_PREVIEW_FULL)
{
	u32 scen = (device->setfile & FIMC_IS_SCENARIO_MASK) >> FIMC_IS_SCENARIO_SHIFT;
	bool scenario_flag = (scen == FIMC_IS_SCENARIO_FULL_SIZE);

	if ((position == SENSOR_POSITION_FRONT) && (scenario_flag))
		return 1;
	else
		return 0;
}

/* front preview */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_PREVIEW)
{
	if (position == SENSOR_POSITION_FRONT)
		return 1;
	else
		return 0;
}

/* front capture */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_CAPTURE)
{
	if ((position == SENSOR_POSITION_FRONT) &&
		(test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state)))
		return 1;
	else
		return 0;
}

DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_CAMCORDING_CAPTURE)
{
	struct fimc_is_dvfs_scenario_ctrl *static_ctrl = device->resourcemgr->dvfs_ctrl.static_ctrl;

	if ((position == SENSOR_POSITION_FRONT) &&
		(test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state)) &&
		(static_ctrl->cur_scenario_id == FIMC_IS_SN_FRONT_CAMCORDING)
		)
		return 1;
	else
		return 0;
}

/* rear capture */
DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAPTURE)
{
	if ((position == SENSOR_POSITION_REAR) &&
		test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state)
		)
		return 1;
	else
		return 0;
}

DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAMCORDING_FHD_CAPTURE)
{
	struct fimc_is_dvfs_scenario_ctrl *static_ctrl = device->resourcemgr->dvfs_ctrl.static_ctrl;

	if ((position == SENSOR_POSITION_REAR) &&
		test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state) &&
		(static_ctrl->cur_scenario_id == FIMC_IS_SN_REAR_CAMCORDING_FHD)
		)
		return 1;
	else
		return 0;
}

DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAMCORDING_HD_CAPTURE)
{
	struct fimc_is_dvfs_scenario_ctrl *static_ctrl = device->resourcemgr->dvfs_ctrl.static_ctrl;

	if ((position == SENSOR_POSITION_REAR || position == SENSOR_POSITION_REAR2) &&
		test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state) &&
		(static_ctrl->cur_scenario_id == FIMC_IS_SN_REAR_CAMCORDING_HD)
		)
		return 1;
	else
		return 0;
}

DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_REAR_CAMCORDING_UHD_CAPTURE)
{
	struct fimc_is_dvfs_scenario_ctrl *static_ctrl = device->resourcemgr->dvfs_ctrl.static_ctrl;

	if ((position == SENSOR_POSITION_REAR || position == SENSOR_POSITION_REAR2) &&
		test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state) &&
		 (static_ctrl->cur_scenario_id == FIMC_IS_SN_REAR_CAMCORDING_UHD)
		)
		return 1;
	else
		return 0;
}

DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_VIDEO_HIGH_SPEED_60FPS_CAPTURE)
{
	struct fimc_is_dvfs_scenario_ctrl *static_ctrl = device->resourcemgr->dvfs_ctrl.static_ctrl;

	if ((position == SENSOR_POSITION_REAR) &&
		test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state) &&
		(static_ctrl->cur_scenario_id == FIMC_IS_SN_VIDEO_HIGH_SPEED_60FPS)
		)
		return 1;
	else
		return 0;
}

DECLARE_DVFS_CHK_FUNC(FIMC_IS_SN_FRONT_WIDE_SELFIE)
{
	if (position == SENSOR_POSITION_FRONT)
		return 1;
	else
		return 0;

}

DECLARE_EXT_DVFS_CHK_FUNC(FIMC_IS_SN_EXT_REAR)
{
	if (position == SENSOR_POSITION_REAR || position == SENSOR_POSITION_REAR2)
		return 1;
	else
		return 0;
}

DECLARE_EXT_DVFS_CHK_FUNC(FIMC_IS_SN_EXT_FRONT)
{
	if (position == SENSOR_POSITION_FRONT)
		return 1;
	else
		return 0;
}

DECLARE_EXT_DVFS_CHK_FUNC(FIMC_IS_SN_EXT_SECURE)
{
	if (position == SENSOR_POSITION_SECURE)
		return 1;
	else
		return 0;
}

int fimc_is_hw_dvfs_init(void *dvfs_data)
{
	int ret = 0;
	ulong i;
	struct fimc_is_dvfs_ctrl *dvfs_ctrl;

	dvfs_ctrl = (struct fimc_is_dvfs_ctrl *)dvfs_data;

	FIMC_BUG(!dvfs_ctrl);

	/* set priority by order */
	for (i = 0; i < ARRAY_SIZE(static_scenarios); i++)
		static_scenarios[i].priority = i;
	for (i = 0; i < ARRAY_SIZE(dynamic_scenarios); i++)
		dynamic_scenarios[i].priority = i;
	for (i = 0; i < ARRAY_SIZE(external_scenarios); i++)
		external_scenarios[i].priority = i;

	dvfs_ctrl->static_ctrl->cur_scenario_id	= -1;
	dvfs_ctrl->static_ctrl->cur_scenario_idx	= -1;
	dvfs_ctrl->static_ctrl->scenarios		= static_scenarios;
	if (static_scenarios[0].scenario_id == FIMC_IS_SN_DEFAULT)
		dvfs_ctrl->static_ctrl->scenario_cnt	= 0;
	else
		dvfs_ctrl->static_ctrl->scenario_cnt	= ARRAY_SIZE(static_scenarios);

	dvfs_ctrl->dynamic_ctrl->cur_scenario_id	= -1;
	dvfs_ctrl->dynamic_ctrl->cur_scenario_idx	= -1;
	dvfs_ctrl->dynamic_ctrl->cur_frame_tick	= -1;
	dvfs_ctrl->dynamic_ctrl->scenarios		= dynamic_scenarios;
	if (static_scenarios[0].scenario_id == FIMC_IS_SN_DEFAULT)
		dvfs_ctrl->dynamic_ctrl->scenario_cnt	= 0;
	else
		dvfs_ctrl->dynamic_ctrl->scenario_cnt	= ARRAY_SIZE(dynamic_scenarios);

	dvfs_ctrl->external_ctrl->cur_scenario_id	= -1;
	dvfs_ctrl->external_ctrl->cur_scenario_idx	= -1;
	dvfs_ctrl->external_ctrl->scenarios		= external_scenarios;
	if (external_scenarios[0].scenario_id == FIMC_IS_SN_DEFAULT)
		dvfs_ctrl->external_ctrl->scenario_cnt	= 0;
	else
		dvfs_ctrl->external_ctrl->scenario_cnt	= ARRAY_SIZE(external_scenarios);

	return ret;
}
