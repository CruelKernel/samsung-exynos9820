/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <exynos-fimc-is-sensor.h>
#include "fimc-is-config.h"
#include "fimc-is-err.h"
#include "fimc-is-metadata.h"
#include "fimc-is-interface-sensor.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-interface-preprocessor.h"
#include "fimc-is-preprocessor-c3.h"

#if 1 /* #ifdef CAMERA_C3_ADDR_MAP_V2 */
#include "fimc-is-preprocessor-c3-regmap_v2.h"
#else
#include "fimc-is-preprocessor-c3-regmap.h"
#endif

#include "fimc-is-device-preprocessor.h"
#include "fimc-is-module-preprocessor.h"

// [x][0]: bytes, [x][1]: address
static u32 C73C3_DbgSysRegAdrs[C73C3_NUM_OF_DBG_SYS_REG][2] = {
	{0x02, api_chip_id}, // 0x20000000
	{0x02, api_chip_rev},
	{0x04, api_svn_rev}, // 4b
	{0x02, api_init_done},// 2b
	{0x04, api_sys_rev}, // 4b
	{0x02, api_info_config_input_uWidth}, // 0x20000098
	{0x02, api_info_config_input_uHeight},
	{0x02, api_info_config_output_uWidth},
	{0x02, api_info_config_output_uHeight},
	{0x02, Mon_DBG_Counters_0_},
	{0x02, Mon_DBG_Counters_1_},
	{0x02, Mon_DBG_Counters_2_},
	{0x02, Mon_DBG_Counters_3_},
	{0x02, Mon_DBG_Counters_4_},
	{0x02, Mon_DBG_Counters_5_},
	{0x02, Mon_DBG_Counters_6_},
	{0x02, Mon_DBG_Counters_7_},
	{0x02, Mon_DBG_Counters_8_},
	{0x02, Mon_DBG_Counters_9_},
	{0x02, Mon_DBG_Counters_10_},
	{0x02, Mon_DBG_Counters_11_},
	{0x02, Mon_DBG_Counters_12_},
	{0x02, Mon_DBG_Counters_13_},
	{0x02, Mon_DBG_Counters_14_},
	{0x02, Mon_DBG_Counters_15_},
	{0x02, Mon_DBG_Counters_16_},
	{0x02, Mon_DBG_Counters_17_},
	{0x02, Mon_DBG_FirstErr},
	{0x02, Mon_DBG_FirstErrInfo},
	{0x02, Mon_DBG_LastErr},
	{0x02, Mon_DBG_LastErrInfo},
	{0x02, Mon_DBG_ErrCntr},
	{0x02, Mon_DBG_Counters_15_},
};

static const char* C73C3_DbgSysRegName[C73C3_NUM_OF_DBG_SYS_REG] = {
	"api_chip_id",
	"api_chip_rev",
	"api_svn_rev",
	"api_init_done",
	"api_sys_rev",
	"api_info_config_input_uWidth",
	"api_info_config_input_uHeight",
	"api_info_config_output_uWidth",
	"api_info_config_output_uHeight",
	"Mon_DBG_Counters_0_",
	"Mon_DBG_Counters_1_",
	"Mon_DBG_Counters_2_",
	"Mon_DBG_Counters_3_",
	"Mon_DBG_Counters_4_",
	"Mon_DBG_Counters_5_",
	"Mon_DBG_Counters_6_",
	"Mon_DBG_Counters_7_",
	"Mon_DBG_Counters_8_",
	"Mon_DBG_Counters_9_",
	"Mon_DBG_Counters_10_",
	"Mon_DBG_Counters_11_",
	"Mon_DBG_Counters_12_",
	"Mon_DBG_Counters_13_",
	"Mon_DBG_Counters_14_",
	"Mon_DBG_Counters_15_",
	"Mon_DBG_Counters_16_",
	"Mon_DBG_Counters_17_",
	"Mon_DBG_FirstErr",
	"Mon_DBG_FirstErrInfo",
	"Mon_DBG_LastErr",
	"Mon_DBG_LastErrInfo",
	"Mon_DBG_ErrCntr",
	"Mon_DBG_Counters_15_",
};

#define C73C3_I2C_ADR  0x7A
#define C73C3_ADDR_MIN 0x40000000
#define C73C3_ADDR_MAX 0x40010000

static const u32 pC73C3_AfAddr[] = {
	api_info_otf_af_preset_index,

	api_pafstat_roi_topLeft_x,
	api_pafstat_roi_topLeft_y,
	api_pafstat_roi_bottomRight_x,
	api_pafstat_roi_bottomRight_y,

	api_pafstat_numWindows,
	api_pafstat_windows_0__topLeft_x,
	api_pafstat_windows_0__topLeft_y,
	api_pafstat_windows_0__bottomRight_x,
	api_pafstat_windows_0__bottomRight_y,
	api_pafstat_windows_1__topLeft_x,
	api_pafstat_windows_1__topLeft_y,
	api_pafstat_windows_1__bottomRight_x,
	api_pafstat_windows_1__bottomRight_y,
	api_pafstat_windows_2__topLeft_x,
	api_pafstat_windows_2__topLeft_y,
	api_pafstat_windows_2__bottomRight_x,
	api_pafstat_windows_2__bottomRight_y,
	api_pafstat_windows_3__topLeft_x,
	api_pafstat_windows_3__topLeft_y,
	api_pafstat_windows_3__bottomRight_x,
	api_pafstat_windows_3__bottomRight_y,

	api_pafstat_mwm_c_x,
	api_pafstat_mwm_c_y,
	api_pafstat_mwm_topLeft_x,
	api_pafstat_mwm_topLeft_y,
	api_pafstat_mwm_bottomRight_x,
	api_pafstat_mwm_bottomRight_y,

	api_pafstat_mwsEnable,
	api_pafstat_mws_topLeft_x,
	api_pafstat_mws_topLeft_y,
	api_pafstat_mws_span_width,
	api_pafstat_mws_span_height,
	api_pafstat_mwsGap_x,
	api_pafstat_mwsGap_y,
	api_pafstat_mwsNo_x,
	api_pafstat_mwsNo_y,

	api_cafstat_numWindows,
	api_cafstat_windows_0__topLeft_x,
	api_cafstat_windows_0__topLeft_y,
	api_cafstat_windows_0__bottomRight_x,
	api_cafstat_windows_0__bottomRight_y,
	api_cafstat_windows_1__topLeft_x,
	api_cafstat_windows_1__topLeft_y,
	api_cafstat_windows_1__bottomRight_x,
	api_cafstat_windows_1__bottomRight_y,
	api_cafstat_windows_2__topLeft_x,
	api_cafstat_windows_2__topLeft_y,
	api_cafstat_windows_2__bottomRight_x,
	api_cafstat_windows_2__bottomRight_y,
	api_cafstat_windows_3__topLeft_x,
	api_cafstat_windows_3__topLeft_y,
	api_cafstat_windows_3__bottomRight_x,
	api_cafstat_windows_3__bottomRight_y,

	api_cafstat_mwm_c_x,
	api_cafstat_mwm_c_y,
	api_cafstat_mwm_topLeft_x,
	api_cafstat_mwm_topLeft_y,
	api_cafstat_mwm_bottomRight_x,
	api_cafstat_mwm_bottomRight_y,

	api_cafstat_mwsEnable,
	api_cafstat_mws_topLeft_x,
	api_cafstat_mws_topLeft_y,
	api_cafstat_mws_span_width,
	api_cafstat_mws_span_height,
	api_cafstat_mwsGap_x,
	api_cafstat_mwsGap_y,
	api_cafstat_mwsNo_x,
	api_cafstat_mwsNo_y,

	cafTuningParams_gr_coef,
	cafTuningParams_r_coef,
	cafTuningParams_b_coef,
	cafTuningParams_gb_coef,
	cafTuningParams_stat_sel,

	cafTuningParams_kn_inc_0_,
	cafTuningParams_kn_inc_1_,
	cafTuningParams_kn_inc_2_,
	cafTuningParams_kn_inc_3_,
	cafTuningParams_kn_inc_4_,
	cafTuningParams_kn_inc_5_,
	cafTuningParams_kn_inc_6_,
	cafTuningParams_kn_inc_7_,
	cafTuningParams_kn_inc_8_,
	cafTuningParams_kn_inc_9_,
	cafTuningParams_kn_inc_10_,
	cafTuningParams_kn_inc_11_,
	cafTuningParams_kn_offset_0_,
	cafTuningParams_kn_offset_1_,
	cafTuningParams_kn_offset_2_,
	cafTuningParams_kn_offset_3_,
	cafTuningParams_kn_offset_4_,
	cafTuningParams_kn_offset_5_,
	cafTuningParams_kn_offset_6_,
	cafTuningParams_kn_offset_7_,
	cafTuningParams_kn_offset_8_,
	cafTuningParams_kn_offset_9_,
	cafTuningParams_kn_offset_10_,

	cafTuningParams_bin1,
	cafTuningParams_bin2,

	cafTuningParams_i0_g0,
	cafTuningParams_i0_k01,
	cafTuningParams_i0_k02,
	cafTuningParams_i0_ftype0,
	cafTuningParams_i0_g1,
	cafTuningParams_i0_k11,
	cafTuningParams_i0_k12,
	cafTuningParams_i0_c11,
	cafTuningParams_i0_c12,
	cafTuningParams_i0_g2,
	cafTuningParams_i0_k21,
	cafTuningParams_i0_k22,
	cafTuningParams_i0_c21,
	cafTuningParams_i0_c22,
	cafTuningParams_i0_by0,
	cafTuningParams_i0_by1,
	cafTuningParams_i0_by2,
	cafTuningParams_i0_cor,

	cafTuningParams_i1_g0,
	cafTuningParams_i1_k01,
	cafTuningParams_i1_k02,
	cafTuningParams_i1_ftype0,
	cafTuningParams_i1_g1,
	cafTuningParams_i1_k11,
	cafTuningParams_i1_k12,
	cafTuningParams_i1_c11,
	cafTuningParams_i1_c12,
	cafTuningParams_i1_g2,
	cafTuningParams_i1_k21,
	cafTuningParams_i1_k22,
	cafTuningParams_i1_c21,
	cafTuningParams_i1_c22,
	cafTuningParams_i1_by0,
	cafTuningParams_i1_by1,
	cafTuningParams_i1_by2,
	cafTuningParams_i1_cor,

	cafTuningParams_i2_g0,
	cafTuningParams_i2_k01,
	cafTuningParams_i2_k02,
	cafTuningParams_i2_ftype0,
	cafTuningParams_i2_g1,
	cafTuningParams_i2_k11,
	cafTuningParams_i2_k12,
	cafTuningParams_i2_c11,
	cafTuningParams_i2_c12,
	cafTuningParams_i2_g2,
	cafTuningParams_i2_k21,
	cafTuningParams_i2_k22,
	cafTuningParams_i2_c21,
	cafTuningParams_i2_c22,
	cafTuningParams_i2_by0,
	cafTuningParams_i2_by1,
	cafTuningParams_i2_by2,
	cafTuningParams_i2_cor,

	cafTuningParams_i3_g,
	cafTuningParams_i3_k1,
	cafTuningParams_i3_k2,
	cafTuningParams_i3_ki,
	cafTuningParams_i3_ftype,
	cafTuningParams_i3_cor,

	pafTuningParams_xcor_on,
	pafTuningParams_af_cross,
	pafTuningParams_mpd_on,
	pafTuningParams_mpd_hbin,
	pafTuningParams_mpd_vbin,
	pafTuningParams_mpd_vsft,
	pafTuningParams_mpd_dp,
	pafTuningParams_mpd_dp_th,
	pafTuningParams_phase_range,
	pafTuningParams_dpc_on,
	pafTuningParams_lmv_on,
	pafTuningParams_lmv_shift,
	pafTuningParams_alc_on,
	pafTuningParams_alc_gap,
	pafTuningParams_b2_en,
	pafTuningParams_crop_on,

	pafTuningParams_i0_g0,
	pafTuningParams_i0_k01,
	pafTuningParams_i0_k02,
	pafTuningParams_i0_ftype0,
	pafTuningParams_i0_g1,
	pafTuningParams_i0_k11,
	pafTuningParams_i0_k12,
	pafTuningParams_i0_c11,
	pafTuningParams_i0_c12,
	pafTuningParams_i0_g2,
	pafTuningParams_i0_k21,
	pafTuningParams_i0_k22,
	pafTuningParams_i0_c21,
	pafTuningParams_i0_c22,
	pafTuningParams_i0_by0,
	pafTuningParams_i0_by1,
	pafTuningParams_i0_by2,

	pafTuningParams_i1_g0,
	pafTuningParams_i1_k01,
	pafTuningParams_i1_k02,
	pafTuningParams_i1_ftype0,
	pafTuningParams_i1_g1,
	pafTuningParams_i1_k11,
	pafTuningParams_i1_k12,
	pafTuningParams_i1_c11,
	pafTuningParams_i1_c12,
	pafTuningParams_i1_g2,
	pafTuningParams_i1_k21,
	pafTuningParams_i1_k22,
	pafTuningParams_i1_c21,
	pafTuningParams_i1_c22,
	pafTuningParams_i1_by0,
	pafTuningParams_i1_by1,
	pafTuningParams_i1_by2,

	pafTuningParams_i2_g0,
	pafTuningParams_i2_k01,
	pafTuningParams_i2_k02,
	pafTuningParams_i2_ftype0,
	pafTuningParams_i2_g1,
	pafTuningParams_i2_k11,
	pafTuningParams_i2_k12,
	pafTuningParams_i2_c11,
	pafTuningParams_i2_c12,
	pafTuningParams_i2_g2,
	pafTuningParams_i2_k21,
	pafTuningParams_i2_k22,
	pafTuningParams_i2_c21,
	pafTuningParams_i2_c22,
	pafTuningParams_i2_by0,
	pafTuningParams_i2_by1,
	pafTuningParams_i2_by2,

	pafTuningParams_il_g0,
	pafTuningParams_il_k01,
	pafTuningParams_il_k02,
	pafTuningParams_il_ftype0,
	pafTuningParams_il_g1,
	pafTuningParams_il_k11,
	pafTuningParams_il_k12,
	pafTuningParams_il_c11,
	pafTuningParams_il_c12,
	pafTuningParams_il_g2,
	pafTuningParams_il_k21,
	pafTuningParams_il_k22,
	pafTuningParams_il_c21,
	pafTuningParams_il_c22,
	pafTuningParams_il_by0,
	pafTuningParams_il_by1,
	pafTuningParams_il_by2,

	pafTuningParams_coring_ty,
	pafTuningParams_coring_th,
	pafTuningParams_i0_coring,
	pafTuningParams_i1_coring,
	pafTuningParams_i2_coring,
	pafTuningParams_il_coring,

	pafTuningParams_bin_first,
	pafTuningParams_binning_num_b0,
	pafTuningParams_binning_num_b1,
	pafTuningParams_binning_num_b2,
	pafTuningParams_binning_num_lmv,
	pafTuningParams_bin0_skip,
	pafTuningParams_bin1_skip,
	pafTuningParams_bin2_skip,
	pafTuningParams_binl_skip,

	pafTuningParams_af_debug_mode,
	pafTuningParams_lf_shift,
	pafTuningParams_pafsat_on,
	pafTuningParams_sat_lv,
	pafTuningParams_sat_lv1,
	pafTuningParams_sat_lv2,
	pafTuningParams_sat_lv3,
	pafTuningParams_sat_src,
	pafTuningParams_cor_type,
	pafTuningParams_g_ssd,
	pafTuningParams_ob_value,
	pafTuningParams_af_layout,
	pafTuningParams_af_pattern,

	pafTuningParams_knee_on,
	pafTuningParams_kn_inc_0_,
	pafTuningParams_kn_inc_1_,
	pafTuningParams_kn_inc_2_,
	pafTuningParams_kn_inc_3_,
	pafTuningParams_kn_inc_4_,
	pafTuningParams_kn_inc_5_,
	pafTuningParams_kn_inc_6_,
	pafTuningParams_kn_inc_7_,
	pafTuningParams_kn_offset_0_,
	pafTuningParams_kn_offset_1_,
	pafTuningParams_kn_offset_2_,
	pafTuningParams_kn_offset_3_,
	pafTuningParams_kn_offset_4_,
	pafTuningParams_kn_offset_5_,
	pafTuningParams_kn_offset_6_,

	pafTuningParams_wdr_on,
	pafTuningParams_wdr_coef_long,
	pafTuningParams_wdr_coef_short,
	pafTuningParams_wdr_shft_long,
	pafTuningParams_wdr_shft_short,

	yExtTuningParams_maxnoskippxg,
	yExtTuningParams_skiplevelth,
	yExtTuningParams_pxsat_g,
	yExtTuningParams_pxsat_r,
	yExtTuningParams_pxsat_b,
	yExtTuningParams_sat_no_g,
	yExtTuningParams_sat_no_r,
	yExtTuningParams_sat_no_b,
	yExtTuningParams_sat_g,
	yExtTuningParams_sat_r,
	yExtTuningParams_sat_b,
	yExtTuningParams_coef_r_short,
	yExtTuningParams_coef_g_short,
	yExtTuningParams_coef_b_short,
	yExtTuningParams_coef_r_long,
	yExtTuningParams_coef_g_long,
	yExtTuningParams_coef_b_long,
};

static const u32 pC73C3_AfAddr_Burst_Sect_PresetIndex[] = {
	api_info_otf_af_preset_index,
};

static const u32 pC73C3_AfAddr_Burst_Sect_PafROIUseSetting[] = {
	api_pafstat_roi_topLeft_x,
	api_pafstat_roi_topLeft_y,
	api_pafstat_roi_bottomRight_x,
	api_pafstat_roi_bottomRight_y,
};

static const u32 pC73C3_AfAddr_Burst_Sect_PafSingleWindows[] = {
	api_pafstat_numWindows,
	api_pafstat_windows_0__topLeft_x,
	api_pafstat_windows_0__topLeft_y,
	api_pafstat_windows_0__bottomRight_x,
	api_pafstat_windows_0__bottomRight_y,
	api_pafstat_windows_1__topLeft_x,
	api_pafstat_windows_1__topLeft_y,
	api_pafstat_windows_1__bottomRight_x,
	api_pafstat_windows_1__bottomRight_y,
	api_pafstat_windows_2__topLeft_x,
	api_pafstat_windows_2__topLeft_y,
	api_pafstat_windows_2__bottomRight_x,
	api_pafstat_windows_2__bottomRight_y,
	api_pafstat_windows_3__topLeft_x,
	api_pafstat_windows_3__topLeft_y,
	api_pafstat_windows_3__bottomRight_x,
	api_pafstat_windows_3__bottomRight_y,
};

static const u32 pC73C3_AfAddr_Burst_Sect_PafMultiMainWindows[] = {
	api_pafstat_mwm_c_x,
	api_pafstat_mwm_c_y,
	api_pafstat_mwm_topLeft_x,
	api_pafstat_mwm_topLeft_y,
	api_pafstat_mwm_bottomRight_x,
	api_pafstat_mwm_bottomRight_y,
};

static const u32 pC73C3_AfAddr_Burst_Sect_PafMultiSubWindows[] = {
	api_pafstat_mwsEnable,
	api_pafstat_mws_topLeft_x,
	api_pafstat_mws_topLeft_y,
	api_pafstat_mws_span_width,
	api_pafstat_mws_span_height,
	api_pafstat_mwsGap_x,
	api_pafstat_mwsGap_y,
	api_pafstat_mwsNo_x,
	api_pafstat_mwsNo_y,
};

static const u32 pC73C3_AfAddr_Burst_Sect_CafSingleWindows[] = {
	api_cafstat_numWindows,
	api_cafstat_windows_0__topLeft_x,
	api_cafstat_windows_0__topLeft_y,
	api_cafstat_windows_0__bottomRight_x,
	api_cafstat_windows_0__bottomRight_y,
	api_cafstat_windows_1__topLeft_x,
	api_cafstat_windows_1__topLeft_y,
	api_cafstat_windows_1__bottomRight_x,
	api_cafstat_windows_1__bottomRight_y,
	api_cafstat_windows_2__topLeft_x,
	api_cafstat_windows_2__topLeft_y,
	api_cafstat_windows_2__bottomRight_x,
	api_cafstat_windows_2__bottomRight_y,
	api_cafstat_windows_3__topLeft_x,
	api_cafstat_windows_3__topLeft_y,
	api_cafstat_windows_3__bottomRight_x,
	api_cafstat_windows_3__bottomRight_y,
};

static const u32 pC73C3_AfAddr_Burst_Sect_CafMultiMainWindows[] = {
	api_cafstat_mwm_c_x,
	api_cafstat_mwm_c_y,
	api_cafstat_mwm_topLeft_x,
	api_cafstat_mwm_topLeft_y,
	api_cafstat_mwm_bottomRight_x,
	api_cafstat_mwm_bottomRight_y,
};

static const u32 pC73C3_AfAddr_Burst_Sect_CafMultiSubWindows[] = {
	api_cafstat_mwsEnable,
	api_cafstat_mws_topLeft_x,
	api_cafstat_mws_topLeft_y,
	api_cafstat_mws_span_width,
	api_cafstat_mws_span_height,
	api_cafstat_mwsGap_x,
	api_cafstat_mwsGap_y,
	api_cafstat_mwsNo_x,
	api_cafstat_mwsNo_y,
};

static const u32 pC73C3_AfAddr_Burst_Sect_CafSetting[] = {
	cafTuningParams_gr_coef,
	cafTuningParams_r_coef,
	cafTuningParams_b_coef,
	cafTuningParams_gb_coef,
	cafTuningParams_stat_sel,
};

static const u32 pC73C3_AfAddr_Burst_Sect_CafKnee[] = {
	cafTuningParams_kn_inc_0_,
	cafTuningParams_kn_inc_1_,
	cafTuningParams_kn_inc_2_,
	cafTuningParams_kn_inc_3_,
	cafTuningParams_kn_inc_4_,
	cafTuningParams_kn_inc_5_,
	cafTuningParams_kn_inc_6_,
	cafTuningParams_kn_inc_7_,
	cafTuningParams_kn_inc_8_,
	cafTuningParams_kn_inc_9_,
	cafTuningParams_kn_inc_10_,
	cafTuningParams_kn_inc_11_,
	cafTuningParams_kn_offset_0_,
	cafTuningParams_kn_offset_1_,
	cafTuningParams_kn_offset_2_,
	cafTuningParams_kn_offset_3_,
	cafTuningParams_kn_offset_4_,
	cafTuningParams_kn_offset_5_,
	cafTuningParams_kn_offset_6_,
	cafTuningParams_kn_offset_7_,
	cafTuningParams_kn_offset_8_,
	cafTuningParams_kn_offset_9_,
	cafTuningParams_kn_offset_10_,
};

static const u32 pC73C3_AfAddr_Burst_Sect_CafBin[] = {
	cafTuningParams_bin1,
	cafTuningParams_bin2,
};

static const u32 pC73C3_AfAddr_Burst_Sect_CafIIR0[] = {
	cafTuningParams_i0_g0,
	cafTuningParams_i0_k01,
	cafTuningParams_i0_k02,
	cafTuningParams_i0_ftype0,
	cafTuningParams_i0_g1,
	cafTuningParams_i0_k11,
	cafTuningParams_i0_k12,
	cafTuningParams_i0_c11,
	cafTuningParams_i0_c12,
	cafTuningParams_i0_g2,
	cafTuningParams_i0_k21,
	cafTuningParams_i0_k22,
	cafTuningParams_i0_c21,
	cafTuningParams_i0_c22,
	cafTuningParams_i0_by0,
	cafTuningParams_i0_by1,
	cafTuningParams_i0_by2,
	cafTuningParams_i0_cor,
};

static const u32 pC73C3_AfAddr_Burst_Sect_CafIIR1[] = {
	cafTuningParams_i1_g0,
	cafTuningParams_i1_k01,
	cafTuningParams_i1_k02,
	cafTuningParams_i1_ftype0,
	cafTuningParams_i1_g1,
	cafTuningParams_i1_k11,
	cafTuningParams_i1_k12,
	cafTuningParams_i1_c11,
	cafTuningParams_i1_c12,
	cafTuningParams_i1_g2,
	cafTuningParams_i1_k21,
	cafTuningParams_i1_k22,
	cafTuningParams_i1_c21,
	cafTuningParams_i1_c22,
	cafTuningParams_i1_by0,
	cafTuningParams_i1_by1,
	cafTuningParams_i1_by2,
	cafTuningParams_i1_cor,
};

static const u32 pC73C3_AfAddr_Burst_Sect_CafIIR2[] = {
	cafTuningParams_i2_g0,
	cafTuningParams_i2_k01,
	cafTuningParams_i2_k02,
	cafTuningParams_i2_ftype0,
	cafTuningParams_i2_g1,
	cafTuningParams_i2_k11,
	cafTuningParams_i2_k12,
	cafTuningParams_i2_c11,
	cafTuningParams_i2_c12,
	cafTuningParams_i2_g2,
	cafTuningParams_i2_k21,
	cafTuningParams_i2_k22,
	cafTuningParams_i2_c21,
	cafTuningParams_i2_c22,
	cafTuningParams_i2_by0,
	cafTuningParams_i2_by1,
	cafTuningParams_i2_by2,
	cafTuningParams_i2_cor,
};

static const u32 pC73C3_AfAddr_Burst_Sect_CafIIR3[] = {
	cafTuningParams_i3_g,
	cafTuningParams_i3_k1,
	cafTuningParams_i3_k2,
	cafTuningParams_i3_ki,
	cafTuningParams_i3_ftype,
	cafTuningParams_i3_cor,
};

static const u32 pC73C3_AfAddr_Burst_Sect_PafSetting1[] = {
	pafTuningParams_xcor_on,
	pafTuningParams_af_cross,
	pafTuningParams_mpd_on,
	pafTuningParams_mpd_hbin,
	pafTuningParams_mpd_vbin,
	pafTuningParams_mpd_vsft,
	pafTuningParams_mpd_dp,
	pafTuningParams_mpd_dp_th,
	pafTuningParams_phase_range,
	pafTuningParams_dpc_on,
	pafTuningParams_lmv_on,
	pafTuningParams_lmv_shift,
	pafTuningParams_alc_on,
	pafTuningParams_alc_gap,
	pafTuningParams_b2_en,
	pafTuningParams_crop_on,
};

static const u32 pC73C3_AfAddr_Burst_Sect_PafFilterIIR0[] = {
	pafTuningParams_i0_g0,
	pafTuningParams_i0_k01,
	pafTuningParams_i0_k02,
	pafTuningParams_i0_ftype0,
	pafTuningParams_i0_g1,
	pafTuningParams_i0_k11,
	pafTuningParams_i0_k12,
	pafTuningParams_i0_c11,
	pafTuningParams_i0_c12,
	pafTuningParams_i0_g2,
	pafTuningParams_i0_k21,
	pafTuningParams_i0_k22,
	pafTuningParams_i0_c21,
	pafTuningParams_i0_c22,
	pafTuningParams_i0_by0,
	pafTuningParams_i0_by1,
	pafTuningParams_i0_by2,
};

static const u32 pC73C3_AfAddr_Burst_Sect_PafFilterIIR1[] = {
	pafTuningParams_i1_g0,
	pafTuningParams_i1_k01,
	pafTuningParams_i1_k02,
	pafTuningParams_i1_ftype0,
	pafTuningParams_i1_g1,
	pafTuningParams_i1_k11,
	pafTuningParams_i1_k12,
	pafTuningParams_i1_c11,
	pafTuningParams_i1_c12,
	pafTuningParams_i1_g2,
	pafTuningParams_i1_k21,
	pafTuningParams_i1_k22,
	pafTuningParams_i1_c21,
	pafTuningParams_i1_c22,
	pafTuningParams_i1_by0,
	pafTuningParams_i1_by1,
	pafTuningParams_i1_by2,
};

static const u32 pC73C3_AfAddr_Burst_Sect_PafFilterIIR2[] = {
	pafTuningParams_i2_g0,
	pafTuningParams_i2_k01,
	pafTuningParams_i2_k02,
	pafTuningParams_i2_ftype0,
	pafTuningParams_i2_g1,
	pafTuningParams_i2_k11,
	pafTuningParams_i2_k12,
	pafTuningParams_i2_c11,
	pafTuningParams_i2_c12,
	pafTuningParams_i2_g2,
	pafTuningParams_i2_k21,
	pafTuningParams_i2_k22,
	pafTuningParams_i2_c21,
	pafTuningParams_i2_c22,
	pafTuningParams_i2_by0,
	pafTuningParams_i2_by1,
	pafTuningParams_i2_by2,
};

static const u32 pC73C3_AfAddr_Burst_Sect_PafFilterIIRL[] = {
	pafTuningParams_il_g0,
	pafTuningParams_il_k01,
	pafTuningParams_il_k02,
	pafTuningParams_il_ftype0,
	pafTuningParams_il_g1,
	pafTuningParams_il_k11,
	pafTuningParams_il_k12,
	pafTuningParams_il_c11,
	pafTuningParams_il_c12,
	pafTuningParams_il_g2,
	pafTuningParams_il_k21,
	pafTuningParams_il_k22,
	pafTuningParams_il_c21,
	pafTuningParams_il_c22,
	pafTuningParams_il_by0,
	pafTuningParams_il_by1,
	pafTuningParams_il_by2,
};

static const u32 pC73C3_AfAddr_Burst_Sect_PafFilterCor[] = {
	pafTuningParams_coring_ty,
	pafTuningParams_coring_th,
	pafTuningParams_i0_coring,
	pafTuningParams_i1_coring,
	pafTuningParams_i2_coring,
	pafTuningParams_il_coring,
};

static const u32 pC73C3_AfAddr_Burst_Sect_PafFilterBin[] = {
	pafTuningParams_bin_first,
	pafTuningParams_binning_num_b0,
	pafTuningParams_binning_num_b1,
	pafTuningParams_binning_num_b2,
	pafTuningParams_binning_num_lmv,
	pafTuningParams_bin0_skip,
	pafTuningParams_bin1_skip,
	pafTuningParams_bin2_skip,
	pafTuningParams_binl_skip,
};

static const u32 pC73C3_AfAddr_Burst_Sect_PafSetting2[] = {
	pafTuningParams_af_debug_mode,
	pafTuningParams_lf_shift,
	pafTuningParams_pafsat_on,
	pafTuningParams_sat_lv,
	pafTuningParams_sat_lv1,
	pafTuningParams_sat_lv2,
	pafTuningParams_sat_lv3,
	pafTuningParams_sat_src,
	pafTuningParams_cor_type,
	pafTuningParams_g_ssd,
	pafTuningParams_ob_value,
	pafTuningParams_af_layout,
	pafTuningParams_af_pattern,
};

static const u32 pC73C3_AfAddr_Burst_Sect_PafKnee[] = {
	pafTuningParams_knee_on,
	pafTuningParams_kn_inc_0_,
	pafTuningParams_kn_inc_1_,
	pafTuningParams_kn_inc_2_,
	pafTuningParams_kn_inc_3_,
	pafTuningParams_kn_inc_4_,
	pafTuningParams_kn_inc_5_,
	pafTuningParams_kn_inc_6_,
	pafTuningParams_kn_inc_7_,
	pafTuningParams_kn_offset_0_,
	pafTuningParams_kn_offset_1_,
	pafTuningParams_kn_offset_2_,
	pafTuningParams_kn_offset_3_,
	pafTuningParams_kn_offset_4_,
	pafTuningParams_kn_offset_5_,
	pafTuningParams_kn_offset_6_,
};

static const u32 pC73C3_AfAddr_Burst_Sect_PafWdr[] = {
	pafTuningParams_wdr_on,
	pafTuningParams_wdr_coef_long,
	pafTuningParams_wdr_coef_short,
	pafTuningParams_wdr_shft_long,
	pafTuningParams_wdr_shft_short,
};

static const u32 pC73C3_AfAddr_Burst_Sect_YExtParam[] = {
	yExtTuningParams_maxnoskippxg,
	yExtTuningParams_skiplevelth,
	yExtTuningParams_pxsat_g,
	yExtTuningParams_pxsat_r,
	yExtTuningParams_pxsat_b,
	yExtTuningParams_sat_no_g,
	yExtTuningParams_sat_no_r,
	yExtTuningParams_sat_no_b,
	yExtTuningParams_sat_g,
	yExtTuningParams_sat_r,
	yExtTuningParams_sat_b,
	yExtTuningParams_coef_r_short,
	yExtTuningParams_coef_g_short,
	yExtTuningParams_coef_b_short,
	yExtTuningParams_coef_r_long,
	yExtTuningParams_coef_g_long,
	yExtTuningParams_coef_b_long,
};

static const u32* pC73C3_AfAddr_Burst[] = {
	pC73C3_AfAddr_Burst_Sect_PresetIndex,

	pC73C3_AfAddr_Burst_Sect_PafROIUseSetting,
	pC73C3_AfAddr_Burst_Sect_PafSingleWindows,
	pC73C3_AfAddr_Burst_Sect_PafMultiMainWindows,
	pC73C3_AfAddr_Burst_Sect_PafMultiSubWindows,
	pC73C3_AfAddr_Burst_Sect_CafSingleWindows,
	pC73C3_AfAddr_Burst_Sect_CafMultiMainWindows,
	pC73C3_AfAddr_Burst_Sect_CafMultiSubWindows,

	pC73C3_AfAddr_Burst_Sect_CafSetting,
	pC73C3_AfAddr_Burst_Sect_CafKnee,
	pC73C3_AfAddr_Burst_Sect_CafBin,
	pC73C3_AfAddr_Burst_Sect_CafIIR0,
	pC73C3_AfAddr_Burst_Sect_CafIIR1,
	pC73C3_AfAddr_Burst_Sect_CafIIR2,
	pC73C3_AfAddr_Burst_Sect_CafIIR3,

	pC73C3_AfAddr_Burst_Sect_PafSetting1,
	pC73C3_AfAddr_Burst_Sect_PafFilterIIR0,
	pC73C3_AfAddr_Burst_Sect_PafFilterIIR1,
	pC73C3_AfAddr_Burst_Sect_PafFilterIIR2,
	pC73C3_AfAddr_Burst_Sect_PafFilterIIRL,
	pC73C3_AfAddr_Burst_Sect_PafFilterCor,
	pC73C3_AfAddr_Burst_Sect_PafFilterBin,
	pC73C3_AfAddr_Burst_Sect_PafSetting2,
	pC73C3_AfAddr_Burst_Sect_PafKnee,
	pC73C3_AfAddr_Burst_Sect_PafWdr,

	pC73C3_AfAddr_Burst_Sect_YExtParam,
};

static const u32 pC73C3_AfAddr_Burst_Size[] = {
	sizeof(pC73C3_AfAddr_Burst_Sect_PresetIndex) / sizeof(pC73C3_AfAddr_Burst_Sect_PresetIndex[0]),

	sizeof(pC73C3_AfAddr_Burst_Sect_PafROIUseSetting) / sizeof(pC73C3_AfAddr_Burst_Sect_PafROIUseSetting[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_PafSingleWindows) / sizeof(pC73C3_AfAddr_Burst_Sect_PafSingleWindows[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_PafMultiMainWindows) / sizeof(pC73C3_AfAddr_Burst_Sect_PafMultiMainWindows[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_PafMultiSubWindows)  / sizeof(pC73C3_AfAddr_Burst_Sect_PafMultiSubWindows[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_CafSingleWindows) / sizeof(pC73C3_AfAddr_Burst_Sect_CafSingleWindows[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_CafMultiMainWindows) / sizeof(pC73C3_AfAddr_Burst_Sect_CafMultiMainWindows[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_CafMultiSubWindows)  / sizeof(pC73C3_AfAddr_Burst_Sect_CafMultiSubWindows[0]),

	sizeof(pC73C3_AfAddr_Burst_Sect_CafSetting)  / sizeof(pC73C3_AfAddr_Burst_Sect_CafSetting[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_CafKnee)  / sizeof(pC73C3_AfAddr_Burst_Sect_CafKnee[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_CafBin)  / sizeof(pC73C3_AfAddr_Burst_Sect_CafBin[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_CafIIR0)  / sizeof(pC73C3_AfAddr_Burst_Sect_CafIIR0[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_CafIIR1)  / sizeof(pC73C3_AfAddr_Burst_Sect_CafIIR1[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_CafIIR2)  / sizeof(pC73C3_AfAddr_Burst_Sect_CafIIR2[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_CafIIR3)  / sizeof(pC73C3_AfAddr_Burst_Sect_CafIIR3[0]),

	sizeof(pC73C3_AfAddr_Burst_Sect_PafSetting1)  / sizeof(pC73C3_AfAddr_Burst_Sect_PafSetting1[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_PafFilterIIR0)  / sizeof(pC73C3_AfAddr_Burst_Sect_PafFilterIIR0[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_PafFilterIIR1)  / sizeof(pC73C3_AfAddr_Burst_Sect_PafFilterIIR1[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_PafFilterIIR2)  / sizeof(pC73C3_AfAddr_Burst_Sect_PafFilterIIR2[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_PafFilterIIRL)  / sizeof(pC73C3_AfAddr_Burst_Sect_PafFilterIIRL[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_PafFilterCor)  / sizeof(pC73C3_AfAddr_Burst_Sect_PafFilterCor[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_PafFilterBin)  / sizeof(pC73C3_AfAddr_Burst_Sect_PafFilterBin[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_PafSetting2)  / sizeof(pC73C3_AfAddr_Burst_Sect_PafSetting2[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_PafKnee)  / sizeof(pC73C3_AfAddr_Burst_Sect_PafKnee[0]),
	sizeof(pC73C3_AfAddr_Burst_Sect_PafWdr)  / sizeof(pC73C3_AfAddr_Burst_Sect_PafWdr[0]),

	sizeof(pC73C3_AfAddr_Burst_Sect_YExtParam)  / sizeof(pC73C3_AfAddr_Burst_Sect_YExtParam[0]),
};

struct fimc_is_preproc_cfg config_preproc_sensor_2l1[] = {
	FIMC_IS_PREPROC_CFG(4032, 3024, 30, 0), /* 12Mp 4:3 (2PD Mode1) */
	FIMC_IS_PREPROC_CFG(4032, 2268, 30, 1), /* 12Mp 16:9 (2PD Mode1) */
	FIMC_IS_PREPROC_CFG(3024, 3024, 30, 2), /* 12Mp 16:9 (2PD Mode1) */
	FIMC_IS_PREPROC_CFG(4032, 2268, 60, 3), /* UHD 16:9 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(2016, 1134, 120, 4), /* FHD 16:9 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(2016, 1134, 30, 5), /* FHD 16:9 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(2016, 1134, 240, 6), /* HD 16:9 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(2016, 1512, 30, 7), /* 4:3 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(1504, 1504, 30, 8), /* 1:1 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(1008, 756, 120, 9), /* Fast AE 4:3 (2PD Mode2) */
};

struct fimc_is_preproc_cfg config_preproc_sensor_2l2[] = {
	FIMC_IS_PREPROC_CFG(4032, 3024, 30, 0), /* 12Mp 4:3 (2PD Mode1) */
	FIMC_IS_PREPROC_CFG(4032, 2268, 30, 1), /* 12Mp 16:9 (2PD Mode1) */
	FIMC_IS_PREPROC_CFG(3024, 3024, 30, 2), /* 12Mp 16:9 (2PD Mode1) */
	FIMC_IS_PREPROC_CFG(4032, 2268, 60, 3), /* UHD 16:9 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(2016, 1134, 120, 4), /* FHD 16:9 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(2016, 1134, 30, 5), /* FHD 16:9 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(2016, 1134, 240, 6), /* HD 16:9 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(2016, 1512, 30, 7), /* 4:3 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(1504, 1504, 30, 8), /* 1:1 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(1008, 756, 120, 9), /* Fast AE 4:3 (2PD Mode2) */
};

/* FixMe. Temp data. */
struct fimc_is_preproc_cfg config_preproc_sensor_imx260[] = {
	FIMC_IS_PREPROC_CFG(4032, 3024, 30, 0), /* 12Mp 4:3 (2PD Mode1) */
	FIMC_IS_PREPROC_CFG(4032, 2268, 30, 1), /* 12Mp 16:9 (2PD Mode1) */
	FIMC_IS_PREPROC_CFG(3024, 3024, 30, 2), /* 12Mp 16:9 (2PD Mode1) */
	FIMC_IS_PREPROC_CFG(4032, 2268, 60, 3), /* UHD 16:9 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(2016, 1134, 120, 4), /* FHD 16:9 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(2016, 1134, 30, 5), /* FHD 16:9 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(2016, 1134, 240, 6), /* HD 16:9 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(2016, 1512, 30, 7), /* 4:3 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(1504, 1504, 30, 8), /* 1:1 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(1008, 756, 120, 9), /* Fast AE 4:3 (2PD Mode2) */
};

struct fimc_is_preproc_cfg config_preproc_sensor_imx333[] = {
	FIMC_IS_PREPROC_CFG(4032, 3024, 30, 0), /* 12Mp 4:3 (2PD Mode1) */
	FIMC_IS_PREPROC_CFG(4032, 2268, 30, 1), /* 12Mp 16:9 (2PD Mode1) */
	FIMC_IS_PREPROC_CFG(3024, 3024, 30, 2), /* 12Mp 16:9 (2PD Mode1) */
	FIMC_IS_PREPROC_CFG(4032, 2268, 60, 3), /* UHD 16:9 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(2016, 1134, 120, 4), /* FHD 16:9 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(2016, 1134, 30, 5), /* FHD 16:9 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(2016, 1134, 240, 6), /* HD 16:9 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(2016, 1512, 30, 7), /* 4:3 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(1504, 1504, 30, 8), /* 1:1 (2PD Mode2) */
	FIMC_IS_PREPROC_CFG(1008, 756, 120, 9), /* Fast AE 4:3 (2PD Mode2) */
};

struct fimc_is_preproc_cfg config_preproc_sensor_4e6[] = {
	FIMC_IS_PREPROC_CFG(2608, 1960, 30, 11), /* Full mode */
	FIMC_IS_PREPROC_CFG(652, 488, 112, 12), /* 4x4 Binning mode */
};

struct fimc_is_preproc_cfg config_preproc_sensor_imx320[] = {
	FIMC_IS_PREPROC_CFG(3264, 2448, 30, 11), 	/* 8MP 4:3 */
	FIMC_IS_PREPROC_CFG(3264, 1836, 30, 12), 	/* 8MP 16:9 */
	FIMC_IS_PREPROC_CFG(2448, 2448, 30, 13), 	/* 8MP 1:1 */
	FIMC_IS_PREPROC_CFG(1632, 918, 60, 14), 	/* 2x2 binning mode 16:9 */
	FIMC_IS_PREPROC_CFG(1632, 918, 30, 15), 	/* 2x2 binning mode 16:9 */
	FIMC_IS_PREPROC_CFG(1632, 1224, 30, 16), 	/* 2x2 binning mode 4:3 */
	FIMC_IS_PREPROC_CFG(1224, 1224, 30, 17), 	/* 2x2 binning mode 1:1 */
	FIMC_IS_PREPROC_CFG(800, 600, 120, 18), 	/* 4x4 binning mode 4:3 */
};

struct fimc_is_preproc_cfg config_preproc_sensor_3H1[] = {
	FIMC_IS_PREPROC_CFG(3264, 2448, 30, 11), 	/* 8MP 4:3 */
	FIMC_IS_PREPROC_CFG(3264, 1836, 30, 12), 	/* 8MP 16:9 */
	FIMC_IS_PREPROC_CFG(2448, 2448, 30, 13), 	/* 8MP 1:1 */
	FIMC_IS_PREPROC_CFG(1632, 918, 60, 14), 	/* 2x2 binning mode 16:9 */
	FIMC_IS_PREPROC_CFG(1632, 918, 30, 15), 	/* 2x2 binning mode 16:9 */
	FIMC_IS_PREPROC_CFG(1632, 1224, 30, 16), 	/* 2x2 binning mode 4:3 */
	FIMC_IS_PREPROC_CFG(1224, 1224, 30, 17), 	/* 2x2 binning mode 1:1 */
	FIMC_IS_PREPROC_CFG(800, 600, 120, 18), 	/* 4x4 binning mode 4:3 */
};

struct fimc_is_preproc_cfg *config_preproc_sensor[] = {
	config_preproc_sensor_2l1,
	config_preproc_sensor_4e6,
	config_preproc_sensor_imx260,
	config_preproc_sensor_imx333,
	config_preproc_sensor_imx320,
	config_preproc_sensor_3H1,
	config_preproc_sensor_2l2,
};

u32 config_preproc_sensor_sizes[] = {
	ARRAY_SIZE(config_preproc_sensor_2l1),
	ARRAY_SIZE(config_preproc_sensor_4e6),
	ARRAY_SIZE(config_preproc_sensor_imx260),
	ARRAY_SIZE(config_preproc_sensor_imx333),
	ARRAY_SIZE(config_preproc_sensor_imx320),
	ARRAY_SIZE(config_preproc_sensor_3H1),
	ARRAY_SIZE(config_preproc_sensor_2l2),
};

static int C73C3_ReadSysReg(struct v4l2_subdev *subdev, preproc_setting_info* setinfo, cis_shared_data* cis_data);

static u32 *g_CrcLookupTable;

static void GenCrcTable(void)
{
    u32 i, j, k;

	for (i = 0; i < CRC_TABLE_SIZE; ++i) {
		k = i;
		for (j = 0; j < 8; ++j) {
			if (k & 1) {
				k = (k >> 1) ^ COMPANION_AF_CRC_POLYNOMIAL;
			} else {
				k >>= 1;
			}
		}
		g_CrcLookupTable[i] = k;
	}
}

static u32 CalcCRC(u8 *mem1, u32 count1, u8 *mem2, u32 count2)
{
	u32 i;
	u32 CRC = 0;

	/* Start CRC calc */
	CRC =~ CRC;

	for ( i = 0 ; i < count1 ; i++ ) {
		CRC = g_CrcLookupTable[(CRC ^ (mem1[i])) & 0xFF] ^ (CRC >> 8);
	}

	for ( i = 0 ; i < count2 ; i++ ) {
		CRC = g_CrcLookupTable[(CRC ^ (mem2[i])) & 0xFF] ^ (CRC >> 8);
	}

	CRC =~ CRC;

	return CRC;
}

int fimc_is_comp_i2c_read_2(struct i2c_client *client, u16 addr, u16 *data)
{
	const u32 addr_size = 2, data_size = 2, max_retry = 5;
	u8 addr_buf[addr_size], data_buf[data_size];
	int retries = max_retry;
	struct i2c_msg msg[2];
	int ret;

	if (!client) {
		err("%s: client is null\n", __func__);
		return -ENODEV;
	}

	addr_buf[0] = (addr & 0xff00) >> 8;
	addr_buf[1] = (addr & 0xff);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = addr_size;
	msg[0].buf = addr_buf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = data_size;
	msg[1].buf = data_buf;

	for (retries = max_retry; retries > 0; retries--) {
		ret = i2c_transfer(client->adapter, msg, 2);
		if (unlikely(ret == 2))
			break;

		info("%s: register read failed(%d), try %d\n", __func__, ret, retries);
		usleep_range(3000, 3100);
	}

	if (unlikely(ret != 2)) {
		err("%s: error %d, fail to read 0x%04X", __func__, ret, addr);
		return (ret >= 0) ? -ETIMEDOUT: ret;
	}

	*data = ((data_buf[0] << 8) | data_buf[1]);

	return 0;
}

int fimc_is_comp_i2c_read_4(struct i2c_client *client, u16 addr, u32 *data)
{
	const u32 addr_size = 2, data_size = 4, max_retry = 5;
	u8 addr_buf[addr_size], data_buf[data_size];
	int retries = max_retry;
	struct i2c_msg msg[2];
	int ret;

	if (!client) {
		err("%s: client is null\n", __func__);
		return -ENODEV;
	}

	addr_buf[0] = (addr & 0xff00) >> 8;
	addr_buf[1] = (addr & 0xff);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = addr_size;
	msg[0].buf = addr_buf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = data_size;
	msg[1].buf = data_buf;

	for (retries = max_retry; retries > 0; retries--) {
		ret = i2c_transfer(client->adapter, msg, 2);
		if (unlikely(ret == 2))
			break;

		info("%s: register read failed(%d), try %d\n", __func__, ret, retries);
		usleep_range(3000, 3100);
	}

	if (unlikely(ret != 2)) {
		err("%s: error %d, fail to read 0x%04X", __func__, ret, addr);
		return (ret >= 0) ? -ETIMEDOUT: ret;
	}

	*data = ((data_buf[0] << 24) | (data_buf[1] << 16) | (data_buf[2] << 8) | data_buf[3]);

	return 0;
}

int fimc_is_comp_i2c_read_multi(struct i2c_client *client, u16 addr, u8 *buf, int size)
{
	const u32 addr_size = 2, max_retry = 5;
	u8 addr_buf[addr_size];
	int retries = max_retry;
	int ret = 0;

	if (!client) {
		err("%s: client is null\n", __func__);
		return -ENODEV;
	}

	/* Send addr */
	addr_buf[0] = ((u16)addr) >> 8;
	addr_buf[1] = (u8)addr;

	for (retries = max_retry; retries > 0; retries--) {
		ret = i2c_master_send(client, addr_buf, addr_size);
		if (likely(addr_size == ret))
			break;

		info("%s: i2c_master_send failed(%d), try %d\n", __func__, ret, retries);
		usleep_range(3000, 3100);
	}

	if (unlikely(ret <= 0)) {
		err("%s: error %d, fail to write 0x%04X", __func__, ret, addr);
		return ret ? ret : -ETIMEDOUT;
	}

	/* Receive data */
	for (retries = max_retry; retries > 0; retries--) {
		ret = i2c_master_recv(client, buf, size);
		if (likely(ret == size))
			break;

		info("%s: i2c_master_recv failed(%d), try %d\n", __func__, ret, retries);
		usleep_range(3000, 3100);
	}

	if (unlikely(ret <= 0)) {
		err("%s: error %d, fail to read 0x%04X", __func__, ret, addr);
		return ret ? ret : -ETIMEDOUT;
	}

	return 0;
}

int fimc_is_comp_i2c_write_2(struct i2c_client *client, u16 addr, u16 data)
{
	const u32 buf_size = 4, max_retry = 5;
	u8 buf[buf_size];
	int retries = max_retry;
	int ret = 0;

	if (!client) {
		pr_info("%s: client is null\n", __func__);
		return -ENODEV;
	}

	/* Send addr+data */
	buf[0] = ((u16)addr) >> 8;
	buf[1] = (u8)addr;
	buf[2] = ((u16)data) >> 8;
	buf[3] = (u8)data;

	for (retries = max_retry; retries > 0; retries--) {
		ret = i2c_master_send(client, buf, buf_size);
		if (likely(ret == buf_size))
			break;

		pr_info("%s: i2c_master_send failed(%d), try %d\n", __func__, ret, retries);
		usleep_range(3000, 3100);
	}

	if (unlikely(ret <= 0)) {
		pr_err("%s: error %d, fail to write 0x%04X\n", __func__, ret, addr);
		return ret ? ret : -ETIMEDOUT;
	}

	return 0;
}

int fimc_is_comp_i2c_write_multi(struct i2c_client *client, u8 *buf, u32 size)
{
	const u32 max_retry = 5;
	int retries = max_retry;
	int ret = 0;

	if (!client) {
		pr_info("%s: client is null\n", __func__);
		return -ENODEV;
	}

	for (retries = max_retry; retries > 0; retries--) {
		ret = i2c_master_send(client, buf, size);
		if (likely(ret == size))
			break;

		pr_info("%s: i2c_master_send failed(%d), try %d\n", __func__, ret, retries);
		usleep_range(3000, 3100);
	}

	if (unlikely(ret <= 0)) {
		pr_err("%s: error %d, fail to write 0x%04X\n", __func__, ret, (buf[0] << 8) | buf[1]);
		return ret ? ret : -ETIMEDOUT;
	}

	return 0;
}

void fimc_is_get_preproc_cfg(struct fimc_is_preproc_cfg **cfg, u32 *size, cis_shared_data *cis_data)
{
	int index = 0;

	switch (cis_data->product_name) {
	case SENSOR_NAME_S5K2L1:
		index = 0;
		break;
	case SENSOR_NAME_S5K4E6:
		index = 1;
		break;
	case SENSOR_NAME_IMX260:
		index = 2;
		break;
	case SENSOR_NAME_IMX333:
		index = 3;
		break;
	case SENSOR_NAME_IMX320:
		index = 4;
		break;
	case SENSOR_NAME_S5K3H1:
		index = 5;
		break;
	case SENSOR_NAME_S5K2L2:
		index = 6;
		break;
	default:
		err("wrong product name. (%d)", cis_data->product_name);
		break;
	}

	*cfg = config_preproc_sensor[index];
	*size = config_preproc_sensor_sizes[index];

	return;
}

static bool C73C3SendByte_Reg2Data2(struct i2c_client *client, u32 regAddr, u16 data)
{
	return SendByte_Reg2Data2(client, (u16)(regAddr & 0x0000FFFF), data);
}

static bool C73C3SendByteArray(struct i2c_client *client, u8 *data, u32 size)
{
	return SendByteArray(client, data, size);
}

static int ISPFWLld_SPI_Write_For_73c1(struct spi_device *spi, u8 *buf)
{
	int ret = 0;
	u8 tx_buf[5];

	tx_buf[0] = *buf & 0xFF; /* write cmd */
	tx_buf[1] = (*(buf + 1)) & 0xFF; /* address */
	tx_buf[2] = (*(buf + 2)) & 0xFF; /* address */
	tx_buf[3] = (*(buf + 3)) & 0xFF; /* data */
	tx_buf[4] = (*(buf + 4)) & 0xFF; /* data */

	spi->bits_per_word = 8;
	ret = spi_write(spi, &tx_buf[0], 5);
	if (ret)
		err("spi sync error - can't read data");

	return ret;
}

static int ISPFWLld_SPI_ReadBurst_For_73c1(struct spi_device *spi, u16 addr, u8 *buf, int size)
{
	int ret = 0;
	u8 tx_buf[4];
	struct spi_transfer t_c;
	struct spi_transfer t_r;
	struct spi_message m;

	memset(&t_c, 0x00, sizeof(t_c));
	memset(&t_r, 0x00, sizeof(t_r));

	tx_buf[0] = 0x03; /* read cmd */
	tx_buf[1] = (addr >> 8) & 0xFF; /* address */
	tx_buf[2] = (addr >> 0) & 0xFF; /* address */
	tx_buf[3] = 0xFF; /* 8-bit dummy */

	t_c.tx_buf = tx_buf;
	t_c.len = 4;
	t_c.cs_change = 1;
	t_c.bits_per_word = 32;
	t_c.speed_hz = 12500000;

	t_r.rx_buf = buf;
	t_r.len = (u32)size;
	t_r.cs_change = 0;

	switch (size % 4) {
	case 0:
		t_r.bits_per_word = 32;
		break;
	case 2:
		t_r.bits_per_word = 16;
		break;
	default:
		t_r.bits_per_word = 8;
		break;
	}
	t_r.speed_hz = 12500000;

	spi_message_init(&m);
	spi_message_add_tail(&t_c, &m);
	spi_message_add_tail(&t_r, &m);

	ret = spi_sync(spi, &m);
	if (ret)
		err("spi sync error - can't read data");

	return ret;
}

static bool C73C3_IsPafStatEnableMode(cis_shared_data *cis_data)
{
	bool bRes = false;
	s32 mode = (s32)cis_data->companion_data.config_idx;

	switch (cis_data->product_name) {
	case SENSOR_NAME_S5K2L1:
		if (mode <= C73C3_S5K2L1_CONFIG_6048X3024_30) {
			bRes = true;
		}
		break;
	case SENSOR_NAME_S5K2L2:
		if (mode <= C73C3_S5K2L2_CONFIG_4032X2268_60) {
			bRes = true;
		}
		break;
	case SENSOR_NAME_IMX333:
		if (mode <= C73C3_IMX333_CONFIG_4032X2268_60) {
			bRes = true;
		}
		break;
	case SENSOR_NAME_IMX260:
		if (mode <= C73C3_IMX260_CONFIG_6048X3024_30) {
			bRes = true;
		}
		break;
	case SENSOR_NAME_S5K3H1:
	case SENSOR_NAME_IMX320:
		bRes = false;
		break;
	case SENSOR_NAME_S5K4E6:
		bRes = false;
		break;
	default:
		break;
	}

	cis_data->companion_data.paf_stat_enable = bRes;

	dbg_preproc("[COMP][S%d][%s] PAF stat %s(Companion mode: %d)\n", cis_data->stream_id, __func__,
		cis_data->companion_data.paf_stat_enable ? "on" : "off", mode);

	return bRes;
}

static bool C73C3_IsCafStatEnableMode(cis_shared_data *cis_data)
{
	bool bRes = false;
	s32 mode = (s32)cis_data->companion_data.config_idx;

	switch (cis_data->product_name) {
	case SENSOR_NAME_S5K2L1:
		if (mode != C73C3_S5K2L1_CONFIG_1008X756_120) {
			bRes = true;
		}
		break;
	case SENSOR_NAME_S5K2L2:
		if (mode != C73C3_S5K2L2_CONFIG_1008X756_120) {
			bRes = true;
		}
		break;
	case SENSOR_NAME_IMX333:
		if (mode != C73C3_IMX333_CONFIG_1008X756_120) {
			bRes = true;
		}
		break;
	case SENSOR_NAME_IMX260:
		if (mode != C73C3_IMX260_CONFIG_1008X756_120) {
			bRes = true;
		}
		break;
	case SENSOR_NAME_S5K3H1:
	case SENSOR_NAME_IMX320:
		bRes = true;
		break;
	case SENSOR_NAME_S5K4E6:
		bRes = false;
		break;
	default:
		break;
	}

	cis_data->companion_data.caf_stat_enable = bRes;

	dbg_preproc("[COMP][S%d][%s] CAF stat %s(Companion mode: %d)\n",
	cis_data->stream_id, __func__, cis_data->companion_data.caf_stat_enable ? "on" : "off", mode);

	return bRes;
}

static bool C73C3_IsWdrEnableMode(cis_shared_data *cis_data)
{
	bool bRes = false;
	s32 mode = (s32)cis_data->companion_data.config_idx;

	switch (cis_data->product_name) {
	case SENSOR_NAME_S5K3H1:
	switch (mode) {
		case C73C3_S5K3H1_CONFIG_3264X2448_30:
		case C73C3_S5K3H1_CONFIG_3264X1836_30:
		case C73C3_S5K3H1_CONFIG_2448X2448_30:
			bRes = true;
			break;
		default:
			bRes = false;
			break;
		break;
		}
		break;
	case SENSOR_NAME_IMX320:
		switch (mode) {
		case C73C3_IMX320_CONFIG_3264X2448_30:
		case C73C3_IMX320_CONFIG_3264X1836_30:
		case C73C3_IMX320_CONFIG_2448X2448_30:
			bRes = true;
			break;
		default:
			bRes = false;
			break;
		break;
		}
		break;
	case SENSOR_NAME_S5K4E6:
		switch (mode) {
		case C73C3_S5K4E6_CONFIG_2608X1960_30:
			bRes = true;
			break;
		default:
			bRes = false;
			break;
		}
		break;
	case SENSOR_NAME_S5K2L1:
		switch (mode) {
		case C73C3_S5K2L1_CONFIG_1008X756_120:
			bRes = false;
			break;
		default:
			bRes = true;
			break;
		}
		break;
	case SENSOR_NAME_S5K2L2:
		switch (mode) {
		case C73C3_S5K2L2_CONFIG_1008X756_120:
			bRes = false;
			break;
		default:
			bRes = true;
			break;
		}
		break;
	case SENSOR_NAME_IMX333:
		switch (mode) {
		case C73C3_IMX333_CONFIG_1008X756_120:
			bRes = false;
			break;
		default:
			bRes = true;
			break;
		}
		break;
	case SENSOR_NAME_IMX260:
		switch (mode) {
		case C73C3_IMX260_CONFIG_4032X2268_60:
		case C73C3_IMX260_CONFIG_2016X1134_120:
		case C73C3_IMX260_CONFIG_2016X1134_240:
		case C73C3_IMX260_CONFIG_1008X756_120:
			bRes = false;
			break;
		default:
			bRes = true;
			break;
		}
		break;
	}

	cis_data->companion_data.wdr_enable = bRes;

	return bRes;
}

static bool C73C3SendByte_WdrOnOff(struct i2c_client *client, u32 onOffStatus)
{
	bool bRes = true;

	// WDR Setting
	// api_otf_WdrMode : 0-Normal mode, 1-WDR mode
	// api_otf_PowerMode : 0-Normal, 1-WDR On, 2-WDR Auto, 3-Long Exposure(LE)

	if (onOffStatus == COMPANION_WDR_ON) {
		// WDR ON
		// api_otf_WdrMode=1-WDR mode
		bRes = C73C3SendByte_Reg2Data2(client, api_otf_WdrMode,   0x0001);
		// api_otf_PowerMode=1-WDR On
		bRes = C73C3SendByte_Reg2Data2(client, api_otf_PowerMode, 0x0001);
	} else if (onOffStatus == COMPANION_WDR_AUTO) {
		// WDR AUTO
		// api_otf_WdrMode=1-WDR mode
		bRes = C73C3SendByte_Reg2Data2(client, api_otf_WdrMode,   0x0001);
		// api_otf_PowerMode=1-WDR Auto
		bRes = C73C3SendByte_Reg2Data2(client, api_otf_PowerMode, 0x0002);
	} else {
		// WDR OFF
		// api_otf_WdrMode=0-Normal mode
		bRes = C73C3SendByte_Reg2Data2(client, api_otf_WdrMode,   0x0000);
		// api_otf_PowerMode=0-Normal
		bRes = C73C3SendByte_Reg2Data2(client, api_otf_PowerMode, 0x0000);
		// DRC "long : short" exposure ratio = "1:1"
		bRes = C73C3SendByte_Reg2Data2(client, api_otf_sceneDr,   0x0100);
	}

    	return bRes;
}

static bool C73C3SendByte_DisparityOnOff(struct i2c_client *client, u32 disparity_mode, s32 mode)
{
	bool bRes = true;

	// Disparity mode Setting
	// COMPANION_DISPARITY_OFF = 1,
	// COMPANION_DISPARITY_SAD,
	// COMPANION_DISPARITY_CENSUS_MEAN, // Disparity mode default
	// COMPANION_DISPARITY_CENSUS_CENTER

	if (disparity_mode != COMPANION_DISPARITY_OFF) {
		// Disparity mode on
		C73C3SendByte_Reg2Data2(client, api_config_0__PDAFMode, 0x0102);
		// Disparity mode on
		C73C3SendByte_Reg2Data2(client, api_config_1__PDAFMode, 0x0102);
		// Disparity mode on
		C73C3SendByte_Reg2Data2(client, api_config_2__PDAFMode, 0x0102);

		if (disparity_mode == COMPANION_DISPARITY_SAD) {
			//DISPARITY_SAD
			//SAD mode
			C73C3SendByte_Reg2Data2(client, pafTuningParams_depth_dmode,   0x0000);
			info("[COMP][%s] Turn On Disparity SAD mode\n",__func__);
		} else if (disparity_mode == COMPANION_DISPARITY_CENSUS_MEAN) {
			// DISPARITY_CENSUS_MEAN
			//CENSUS MEAN mode
			C73C3SendByte_Reg2Data2(client, pafTuningParams_depth_dmode,   0x0001);
			info("[COMP][%s] Turn On Disparity CENSUS MEAN mode\n",__func__);
		} else if (disparity_mode == COMPANION_DISPARITY_CENSUS_CENTER) {
			// DISPARITY_CENSUS_CENTER
			//CENSUS CENTER mode
			C73C3SendByte_Reg2Data2(client, pafTuningParams_depth_dmode,   0x0002);
			info("[COMP][%s] Turn On Disparity CENSUS CENTER mode\n",__func__);
		} else {
			err("[COMP][%s] Undefined Disparity mode %d !\n",__func__,disparity_mode);
		}
	} else {
		// Disparity mode off
		C73C3SendByte_Reg2Data2(client, api_config_0__PDAFMode, 0x0002);
		// Disparity mode off
		C73C3SendByte_Reg2Data2(client, api_config_1__PDAFMode, 0x0002);
		// Disparity mode off
		C73C3SendByte_Reg2Data2(client, api_config_2__PDAFMode, 0x0002);

		// Disparity mode off (mode3 only)
		if(mode == C73C3_IMX333_CONFIG_4032X2268_60 || mode == C73C3_S5K2L2_CONFIG_4032X2268_60)
		{
			C73C3SendByte_Reg2Data2(client, api_config_3__PDAFMode, 0x0003);
		}

		info("[COMP][%s] Disparity OFF\n", __func__);
	}

	return bRes;
}

static int C73C3_Init(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	info("[COMP][S%d][%s]\n", cis_data->stream_id, __func__);

	return ERROR_SENSOR_NO;
}

static int C73C3_SetSize(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	info("[COMP][S%d][%s]\n", cis_data->stream_id, __func__);

	return ERROR_SENSOR_NO;
}

static int C73C3_StreamOn(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	I2C_MUTEX_LOCK(preprocessor->i2c_lock);

	C73C3SendByte_Reg2Data2(client, HOST_INTRP_StreamEnable, 0x0001);

	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

	info("[COMP][S%d][%s]\n", cis_data->stream_id, __func__);

	return ERROR_SENSOR_NO;
}

static int C73C3_StreamOff(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	const u32 LIMIT_WAIT_TIME = 250;
	u32 uCheckWaitLimit = 0;
	unsigned char pReadArrayBuf[4];
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	if (cis_data->stream_on) {
		err("[COMP][S%d][%s] Sensor should be turned off before the C73C3 !\n",
			cis_data->stream_id, __func__);
		return ERROR_SENSOR_INVALID_SETTING;
	}

	I2C_MUTEX_LOCK(preprocessor->i2c_lock);
	C73C3SendByte_Reg2Data2(client, HOST_INTRP_StreamEnable, 0x0000);
	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

	while (1) {
		I2C_MUTEX_LOCK(preprocessor->i2c_lock);
		ReadByte_Reg2Data4(client, (u16)(api_info_frameCounter & 0x0000FFFF), (u32 *)pReadArrayBuf);
		I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

		// api_info_frameCounter starts from 0x00000001,
		// and the value is set to "0x00000000" under starem off status.
		if (pReadArrayBuf[0] == 0x00 && pReadArrayBuf[1] == 0x00 &&
			pReadArrayBuf[2] == 0x00 && pReadArrayBuf[3] == 0x00) {
			break;
		}

		usleep_range(4000, 4000); /* 4 ms */

		if (++uCheckWaitLimit == LIMIT_WAIT_TIME) {
			err("[COMP][S%d][%s] Time out! ActiveArea: %d, CheckWait Limit: %d,"
				"FrameCounter: 0x%02x%02x%02x%02x\n",
				cis_data->stream_id, __func__, cis_data->is_active_area,
				uCheckWaitLimit, pReadArrayBuf[0],
				pReadArrayBuf[1], pReadArrayBuf[2], pReadArrayBuf[3]);

			C73C3_ReadSysReg(subdev, setinfo, cis_data);

			return ERROR_SENSOR_SETTING_TIME_OUT;
		}
	}

	info("[COMP][S%d][%s] Success(FrameCounter:0x%02x%02x%02x%02x)\n",
		cis_data->stream_id, __func__, pReadArrayBuf[0], pReadArrayBuf[1],
		pReadArrayBuf[2], pReadArrayBuf[3]);

	return ERROR_SENSOR_NO;
}

static int C73C3_updateSupportMode(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	// PAF : update paf_stat_enable.
	C73C3_IsPafStatEnableMode(cis_data);

	// CAF : update caf_stat_enable.
	C73C3_IsCafStatEnableMode(cis_data);

	// WDR : update wdr mode
	C73C3_IsWdrEnableMode(cis_data);

	return ERROR_SENSOR_NO;
	}

static int C73C3_ModeChange(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	I2C_MUTEX_LOCK(preprocessor->i2c_lock);
	// Common settings
	C73C3SendByte_Reg2Data2(client, api_config_idx, cis_data->companion_data.config_idx);

	C73C3SendByte_WdrOnOff(client, cis_data->companion_data.wdr_mode);

	C73C3SendByte_DisparityOnOff(client, cis_data->companion_data.disparity_mode, (s32)cis_data->companion_data.config_idx);
	/*C73C3SendByte_Reg2Data2(client, api_otf_isVideoMode,
		cis_data->pOisCaptureState[0]);*/

	// Shading is controlled by a BV value.
	C73C3SendByte_Reg2Data2(client, grasTuning_useEVForPowerIndexLut, 0x0000);

	C73C3SendByte_Reg2Data2(client, api_pafstat_bBypass,
		cis_data->companion_data.paf_stat_enable == true ? 0 : 1); // Paf stat

	C73C3SendByte_Reg2Data2(client, api_cafstat_bBypass,
		cis_data->companion_data.caf_stat_enable == true ? 0 : 1); // Caf stat

	C73C3SendByte_Reg2Data2(client, HOST_INTRP_ChangeConfig, 0x0001);
	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

	info("[COMP][S%d][%s] Sensor mode:%d, preproc mode:%d\n",
		cis_data->stream_id, __func__,
		cis_data->sens_config_index_cur,
		cis_data->companion_data.config_idx);

	return ERROR_SENSOR_NO;
}

static int C73C3_LscSet3aaInfo(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	static u16 sLastAwbRedGain = 0;
	static u16 sLastAwbGreenGain = 0;
	static u16 sLastAwbBlueGain = 0;
	static u16 sLastOutdoorWeight = 0;
	static u16 sLastBv = 0;
	static u16 sLastOisCapture = 0;
	static u16 sLastLscPower = 0; // CMl AE sends the lscpower value to C3.
	static u16 sLastPGain = 0;
	u8 pBuf[16];
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	if (cis_data->companion_data.bypass == false) {
		SENCMD_CompanionSetLsc3aaInfoStr *pLsc = (SENCMD_CompanionSetLsc3aaInfoStr*)setinfo->param;

		if (pLsc->bHwSetEveryFrame == false) {
			// CMl AE sends the lscpower value to C3.
			if (pLsc->usAwbRedGain == sLastAwbRedGain && pLsc->usAwbGreenGain == sLastAwbGreenGain &&
				pLsc->usAwbBlueGain == sLastAwbBlueGain && pLsc->usOutdoorWeight == sLastOutdoorWeight &&
				pLsc->usBv == sLastBv && pLsc->usLscPower == sLastLscPower &&
				pLsc->usOisCapture == sLastOisCapture && pLsc->usPGain == sLastPGain) {
				return ERROR_SENSOR_NO;
			}
		}

		pBuf[0] = (api_otf_PedestalGain88 & 0x0000FF00) >> 8;
		pBuf[1] = (api_otf_PedestalGain88 & 0x000000FF);
		pBuf[2] = (pLsc->usPGain & 0xFF00) >> 8; // api_otf_PedestalGain88
		pBuf[3] = (pLsc->usPGain & 0x00FF);
		pBuf[4] = (pLsc->usAwbRedGain & 0xFF00) >> 8; // api_otf_WBGainRedComponent
		pBuf[5] = (pLsc->usAwbRedGain & 0x00FF);
		pBuf[6] = (pLsc->usAwbGreenGain & 0xFF00) >> 8; // api_otf_WBGainGreenComponent
		pBuf[7] = (pLsc->usAwbGreenGain & 0x00FF);
		pBuf[8] = (pLsc->usAwbBlueGain & 0xFF00) >> 8; // api_otf_WBGainBlueComponent
		pBuf[9] = (pLsc->usAwbBlueGain & 0x00FF);
		pBuf[10] = (pLsc->usOisCapture & 0xFF00) >> 8; // api_otf_isVideoMode
		pBuf[11] = (pLsc->usOisCapture & 0x00FF);
		pBuf[12] = (pLsc->usBv & 0xFF00) >> 8; // api_otf_brightY88
		pBuf[13] = (pLsc->usBv & 0x00FF);
		pBuf[14] = (pLsc->usOutdoorWeight & 0xFF00) >> 8; // api_otf_outDoorWeight
		pBuf[15] = (pLsc->usOutdoorWeight & 0x00FF);

		I2C_MUTEX_LOCK(preprocessor->i2c_lock);
		C73C3SendByteArray(client, pBuf, 16);
		I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

		sLastAwbRedGain = pLsc->usAwbRedGain;
		sLastAwbGreenGain = pLsc->usAwbGreenGain;
		sLastAwbBlueGain = pLsc->usAwbBlueGain;
		sLastOutdoorWeight = pLsc->usOutdoorWeight;
		sLastOisCapture = pLsc->usOisCapture;
		sLastBv = pLsc->usBv;
		sLastLscPower = pLsc->usLscPower; //CMl AE sends the lscpower value to C1;
		sLastPGain = pLsc->usPGain;
	}

	return ERROR_SENSOR_NO;
}

static int C73C3_WdrSetAeInfo(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	SENCMD_CompanionSetWdrAeInfoStr *pWdr = (SENCMD_CompanionSetWdrAeInfoStr*)setinfo->param;

	static u16 sLastCoarseLong  = 0;
	static u16 sLastAGainLong   = 0;
	static u16 sLastDGainLong   = 0;
	static u16 sLastCoarseShort = 0;
	static u16 sLastAGainShort  = 0;
	static u16 sLastDGainShort  = 0;
	u16 uAGainLong;
	u16 uDGainLong;
	u16 uAGainShort;
	u16 uDGainShort;
	u8 pBuf[18];
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	I2C_MUTEX_LOCK(preprocessor->i2c_lock);

	if (cis_data->companion_data.bypass == false)
	{
		if (cis_data->companion_data.wdr_mode == COMPANION_WDR_ON ||
			cis_data->companion_data.wdr_mode == COMPANION_WDR_AUTO) {
			if (pWdr->uiLongExposureCoarse == 0 || pWdr->uiLongExposureFine == 0 ||
				pWdr->uiLongExposureAnalogGain == 0 || pWdr->uiLongExposureDigitalGain == 0 ||
				pWdr->uiShortExposureCoarse == 0 || pWdr->uiShortExposureFine == 0 ||
				pWdr->uiShortExposureAnalogGain == 0 || pWdr->uiShortExposureDigitalGain == 0) {
				err("[COMP][S%d][%s] LongCI:%d, LongFI:%d, LongAG:%d, LongDG:%d,"
					"ShortCI:%d, ShortFI:%d, ShortAG %d, ShortDG %d\n",
				cis_data->stream_id, __func__, pWdr->uiLongExposureCoarse, pWdr->uiLongExposureFine,
				pWdr->uiLongExposureAnalogGain, pWdr->uiLongExposureDigitalGain, pWdr->uiShortExposureCoarse,
				pWdr->uiShortExposureFine, pWdr->uiShortExposureAnalogGain, pWdr->uiShortExposureDigitalGain);
				I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

				return ERROR_SENSOR_NO;
			}

			uAGainLong = GAIN_PERMILLE_TO_UNIT88(pWdr->uiLongExposureAnalogGain);
			uDGainLong = GAIN_PERMILLE_TO_UNIT88(pWdr->uiLongExposureDigitalGain);
			uAGainShort = GAIN_PERMILLE_TO_UNIT88(pWdr->uiShortExposureAnalogGain);
			uDGainShort = GAIN_PERMILLE_TO_UNIT88(pWdr->uiShortExposureDigitalGain);

			if (pWdr->bHwSetEveryFrame == false) {
				if (pWdr->uiLongExposureCoarse == sLastCoarseLong && uAGainLong == sLastAGainLong &&
					uDGainLong == sLastDGainLong && pWdr->uiShortExposureCoarse == sLastCoarseShort &&
					uAGainShort == sLastAGainShort && uDGainShort == sLastDGainShort) {
					I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);
					return ERROR_SENSOR_NO;
				}
			}

			pBuf[0]  = (api_otf_long_exposure_coarse & 0x0000FF00) >> 8;
			pBuf[1]  = (api_otf_long_exposure_coarse & 0x000000FF);
			pBuf[2]  = (pWdr->uiLongExposureCoarse & 0xFF00) >> 8;
			pBuf[3]  = (pWdr->uiLongExposureCoarse & 0x00FF);
			pBuf[4]  = (pWdr->uiLongExposureFine & 0xFF00) >> 8;
			pBuf[5]  = (pWdr->uiLongExposureFine & 0x00FF);
			pBuf[6]  = (uAGainLong & 0xFF00) >> 8;
			pBuf[7]  = (uAGainLong & 0x00FF);
			pBuf[8]  = (uDGainLong & 0xFF00) >> 8;
			pBuf[9]  = (uDGainLong & 0x00FF);
			pBuf[10] = (pWdr->uiShortExposureCoarse & 0xFF00) >> 8;
			pBuf[11] = (pWdr->uiShortExposureCoarse & 0x00FF);
			pBuf[12] = (pWdr->uiShortExposureFine & 0xFF00) >> 8;
			pBuf[13] = (pWdr->uiShortExposureFine & 0x00FF);
			pBuf[14] = (uAGainShort & 0xFF00) >> 8;
			pBuf[15] = (uAGainShort & 0x00FF);
			pBuf[16] = (uDGainShort & 0xFF00) >> 8;
			pBuf[17] = (uDGainShort & 0x00FF);

			C73C3SendByteArray(client, pBuf, 18);

			sLastCoarseLong = pWdr->uiLongExposureCoarse;
			sLastAGainLong = uAGainLong;
			sLastDGainLong = uDGainLong;
			sLastCoarseShort = pWdr->uiShortExposureCoarse;
			sLastAGainShort = uAGainShort;
			sLastDGainShort = uDGainShort;
		} else {
			if (pWdr->uiLongExposureCoarse == 0 || pWdr->uiLongExposureFine == 0 ||
				pWdr->uiLongExposureAnalogGain == 0 || pWdr->uiLongExposureDigitalGain == 0 ||
				pWdr->uiShortExposureCoarse == 0 || pWdr->uiShortExposureFine == 0 ||
				pWdr->uiShortExposureAnalogGain == 0 || pWdr->uiShortExposureDigitalGain == 0) {
				err("[COMP][S%d][%s] LongCI:%d, LongFI:%d, LongAG:%d, LongDG:%d, ShortCI:%d,"
					" ShortFI:%d, ShortAG %d, ShortDG %d\n",
					cis_data->stream_id, __func__, pWdr->uiLongExposureCoarse, pWdr->uiLongExposureFine,
					pWdr->uiLongExposureAnalogGain, pWdr->uiLongExposureDigitalGain, pWdr->uiShortExposureCoarse,
					pWdr->uiShortExposureFine, pWdr->uiShortExposureAnalogGain, pWdr->uiShortExposureDigitalGain);
				I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

				return ERROR_SENSOR_NO;
			}

			uAGainShort = GAIN_PERMILLE_TO_UNIT88(pWdr->uiShortExposureAnalogGain);
			uDGainShort = GAIN_PERMILLE_TO_UNIT88(pWdr->uiShortExposureDigitalGain);

			if (pWdr->bHwSetEveryFrame == false) {
				if (pWdr->uiShortExposureCoarse == sLastCoarseShort &&
					uAGainShort == sLastAGainShort && uDGainShort == sLastDGainShort) {
					I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);
					return ERROR_SENSOR_NO;
				}
			}

			pBuf[0] = (api_otf_short_exposure_coarse & 0x0000FF00) >> 8;
			pBuf[1] = (api_otf_short_exposure_coarse & 0x000000FF);
			pBuf[2] = (pWdr->uiShortExposureCoarse & 0xFF00) >> 8;
			pBuf[3] = (pWdr->uiShortExposureCoarse & 0x00FF);
			pBuf[4] = (pWdr->uiShortExposureFine & 0xFF00) >> 8;
			pBuf[5] = (pWdr->uiShortExposureFine & 0x00FF);
			pBuf[6] = (uAGainShort & 0xFF00) >> 8;
			pBuf[7] = (uAGainShort & 0x00FF);
			pBuf[8] = (uDGainShort & 0xFF00) >> 8;
			pBuf[9] = (uDGainShort & 0x00FF);

			C73C3SendByteArray(client, pBuf, 10);

			sLastCoarseShort = pWdr->uiShortExposureCoarse;
			sLastAGainShort = uAGainShort;
			sLastDGainShort = uDGainShort;
		}
	}
	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

	return ERROR_SENSOR_NO;
}

static int C73C3_AfSetInfo(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	u32 baseAddress = 0x4000; // default base for C73C3 reg
	static bool sInit = false;
	const u32* pAfAddr;
	u32 uNumOfEachSect;
	bool bUpdate = false;
	u32 uBufSize;
	u8* pBuf;
	int i;
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	I2C_MUTEX_LOCK(preprocessor->i2c_lock);

	if (cis_data->companion_data.bypass == false) {
		SENCMD_CompanionSetAfInfoStr_73C3 *pAf = (SENCMD_CompanionSetAfInfoStr_73C3*)setinfo->param;

		if (sInit == true) {
			for (i = 0; i < NUM_OF_73C3_AF_PARAMETERS; i++) {
				if (pAf->pAfUpdate[i] == 1) {
					u32 newBaseAddress;
					if ( (pC73C3_AfAddr[i] & 0xFFFF) > 0x6000 ) {
						newBaseAddress = (pC73C3_AfAddr[i] >> 16) & 0xFFFF;
					} else {
						newBaseAddress = 0x4000; // Use default base
					}

					if ( baseAddress != newBaseAddress ) {
						 // Change base address
						C73C3SendByte_Reg2Data2(client, 0xFCFC, newBaseAddress);
						baseAddress = newBaseAddress;
					}

					C73C3SendByte_Reg2Data2(client, pC73C3_AfAddr[i], pAf->pAfSetting[i]);
				}
			}
		} else {
			u32 i, j, k, s;
			u32 uNumOfSect = sizeof(pC73C3_AfAddr_Burst) / sizeof(pC73C3_AfAddr_Burst[0]);
			k = 0;

			for (s = 0; s < uNumOfSect; s++) {
				j = 0;
				pAfAddr = pC73C3_AfAddr_Burst[s];
				uNumOfEachSect = pC73C3_AfAddr_Burst_Size[s];
				bUpdate = false;

				for (i = k; i < uNumOfEachSect + k; i++) {
					if (pAf->pAfUpdate[i] == 1) {
						bUpdate = true;
						break;
					}
				}

				if (bUpdate == true) {
					u32 newBaseAddress;

					if ( (pC73C3_AfAddr[i] & 0xFFFF) > 0x6000 ) {
						newBaseAddress = (pC73C3_AfAddr[i] >> 16) & 0xFFFF;
					} else {
						newBaseAddress = 0x4000; // Use default base
					}

					if ( baseAddress != newBaseAddress ) {
						// Change base address
						C73C3SendByte_Reg2Data2(client, 0xFCFC, newBaseAddress);
						baseAddress = newBaseAddress;
					}

					uBufSize = (2 * uNumOfEachSect) + 2;

					/* FixMe: need to fix repeated alloc */
					pBuf = (u8 *)kmalloc(sizeof(u8) * uBufSize, GFP_KERNEL);
					if (!pBuf) {
						err("mem alloc failed. (%u bytes)", uBufSize);
						return -ENOMEM;
					}

					pBuf[j++] = (u8)((pAfAddr[0] & 0x0000FF00) >> 8);
					pBuf[j++] = (u8)(pAfAddr[0] & 0x000000FF);

					for (i = 0; i < uNumOfEachSect; i++) {
						pBuf[j++] = (u8)((pAf->pAfSetting[k] & 0x0000FF00) >> 8);
						pBuf[j++] = (u8)(pAf->pAfSetting[k++] & 0x000000FF);
					}

					C73C3SendByteArray(client, pBuf, uBufSize);
					kfree(pBuf);
				} else {
					k += uNumOfEachSect;
				}
			}

			sInit = true;
		}

		if ( baseAddress != 0x4000 ) {
			// Change base address
			C73C3SendByte_Reg2Data2(client, 0xFCFC, 0x4000);
		}

		C73C3SendByte_Reg2Data2(client, HOST_INTRP_ChangeConfig, 0x0001);
	}

	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

	return ERROR_SENSOR_NO;
}

static int C73C3_BlockOnOff(struct v4l2_subdev *subdev , preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	SENCMD_CompanionEnableBlockStr *pEnableBlock = (SENCMD_CompanionEnableBlockStr*)setinfo->param;
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	I2C_MUTEX_LOCK(preprocessor->i2c_lock);

	warn("[WARNING!!] Don't use this function. Test mode for block onoff.\n");

	// If theres nothing to update this setting is redundant.
	if (pEnableBlock->uUpdate > 0 && C73C3_IsWdrEnableMode(cis_data) == true) {
		C73C3SendByte_Reg2Data2(client, 0xFCFC, 0x2000); // Change base address
	}

	// DRC
	if (pEnableBlock->uUpdate & 0x1)
	{
		switch (cis_data->product_name) {
		case SENSOR_NAME_IMX333:
		case SENSOR_NAME_S5K2L1:
		case SENSOR_NAME_S5K2L2:
		case SENSOR_NAME_IMX260:
			if (pEnableBlock->uOnOff & 0x1) {
				// On = Enable
				C73C3SendByte_Reg2Data2(client, drcTuningParams_bBypass, 0x0000);
				cis_data->companion_data.enable_drc = true;
				dbg_preproc("[COMP][S%d][%s] Turn On DRC block\n", cis_data->stream_id, __func__);
			} else {
				// Off = Bypass;
				C73C3SendByte_Reg2Data2(client, drcTuningParams_bBypass, 0x0001);
				cis_data->companion_data.enable_drc = false;
				dbg_preproc("[COMP][S%d][%s] Turn Off DRC block\n", cis_data->stream_id, __func__);
			}
			break;
		default:
			if (pEnableBlock->uOnOff & 0x1) {
				// Need sensor develop team guide
				// On = Enable
				C73C3SendByte_Reg2Data2(client, Fs_drcTuningParams_bBypass, 0x0000);
				cis_data->companion_data.enable_drc = true;
				dbg_preproc("[COMP][S%d][%s] Turn On DRC block\n", cis_data->stream_id, __func__);
			} else {
				// Need sensor develop team guide
				// Off = Bypass;
				C73C3SendByte_Reg2Data2(client, Fs_drcTuningParams_bBypass, 0x0001);
				cis_data->companion_data.enable_drc = false;
				dbg_preproc("[COMP][S%d][%s] Turn Off DRC block\n", cis_data->stream_id, __func__);
			}
			break;
		}
	}

	// WDR
	// api_otf_WdrMode : 0-Normal mode, 1-WDR mode
	// api_otf_PowerMode : 0-Normal, 1-WDR On, 2-WDR Auto, 3-Long Exposure(LE)
	if (pEnableBlock->uUpdate & (0x1 << 1)) {
		if (pEnableBlock->uOnOff & (0x1 << 1)) {
			if (C73C3_IsWdrEnableMode(cis_data) == false) {
				//info("[COMP][S%d][%s] C73C3 does not support WDR in case of 60,
				//120, or FHD30 fps modes !\n",
				//cis_data->stream_id, __func__);
			} else {
				if (pEnableBlock->uMode & 0x1) {
					// WDR ON
					//api_otf_WdrMode=1-WDR mode
					C73C3SendByte_Reg2Data2(client, api_otf_WdrMode,   0x0001);
					//api_otf_PowerMode=1-WDR On
					C73C3SendByte_Reg2Data2(client, api_otf_PowerMode, 0x0001);
					cis_data->companion_data.wdr_mode = COMPANION_WDR_ON;
					dbg_preproc("[COMP][S%d][%s] Turn On WDR block\n", cis_data->stream_id, __func__);
				} else if (pEnableBlock->uMode & (0x1 << 1)) {
					// WDR AUTO
					// api_otf_WdrMode=1-WDR mode
					C73C3SendByte_Reg2Data2(client, api_otf_WdrMode,   0x0001);
					// api_otf_PowerMode=2-WDR Auto
					C73C3SendByte_Reg2Data2(client, api_otf_PowerMode, 0x0002);
					cis_data->companion_data.wdr_mode = COMPANION_WDR_AUTO;
					dbg_preproc("[COMP][S%d][%s] Turn On WDR Auto block\n", cis_data->stream_id, __func__);
				} else {
					err("[COMP][S%d][%s] Undefined WDR mode!\n", cis_data->stream_id, __func__);
				}
			}
		} else {
			// WDR OFF (=Single Exposure)
			// api_otf_WdrMode=0-Normal mode
			C73C3SendByte_Reg2Data2(client, api_otf_WdrMode,   0x0000);
			// api_otf_PowerMode=0-Normal
			C73C3SendByte_Reg2Data2(client, api_otf_PowerMode, 0x0000);
			cis_data->companion_data.wdr_mode = COMPANION_WDR_OFF;
			dbg_preproc("[COMP][S%d][%s] Turn Off WDR block\n", cis_data->stream_id, __func__);
		}
	}

	// PAF
	if (pEnableBlock->uUpdate & (0x1 << 2)) {
		if (pEnableBlock->uOnOff & (0x1 << 2)) {
			// Paf stat On
			C73C3SendByte_Reg2Data2(client, api_pafstat_bBypass, 0x0000);
			cis_data->companion_data.paf_stat_enable = true;
			dbg_preproc("[COMP][S%d][%s] PAF stat on \n", cis_data->stream_id, __func__);
		} else {
			// Paf stat Off
			C73C3SendByte_Reg2Data2(client, api_pafstat_bBypass, 0x0001);
			cis_data->companion_data.paf_stat_enable = false;
			dbg_preproc("[COMP][S%d][%s] PAF stat off\n", cis_data->stream_id, __func__);
		}
	}

	// CAF
	if (pEnableBlock->uUpdate & (0x1 << 3)) {
		if (pEnableBlock->uOnOff & (0x1 << 3)) {
			// Caf stat On
			C73C3SendByte_Reg2Data2(client, api_cafstat_bBypass, 0x0000);
			cis_data->companion_data.caf_stat_enable = true;
			dbg_preproc("[COMP][S%d][%s] CAF stat on \n", cis_data->stream_id, __func__);
		} else {
			// Caf stat Off
			C73C3SendByte_Reg2Data2(client, api_cafstat_bBypass, 0x0001);
			cis_data->companion_data.caf_stat_enable = false;
			dbg_preproc("[COMP][S%d][%s] CAF stat off\n", cis_data->stream_id, __func__);
		}
	}

	// LSC
	if (pEnableBlock->uUpdate & (0x1 << 4)) {
		switch (cis_data->product_name) {
		case SENSOR_NAME_S5K2L1:
		case SENSOR_NAME_S5K2L2:
		case SENSOR_NAME_IMX260:
		case SENSOR_NAME_IMX333:
			if (pEnableBlock->uOnOff & (0x1 << 4)) {
				// LSC On
				C73C3SendByte_Reg2Data2(client, grasTuning_uBypass, 0x0000);
				cis_data->companion_data.enable_lsc = true;
				dbg_preproc("[COMP][S%d][%s] Turn On LSC block\n", cis_data->stream_id, __func__);
			} else {
				// LSC Off
				C73C3SendByte_Reg2Data2(client, grasTuning_uBypass, 0x0001);
				cis_data->companion_data.enable_lsc = false;
				dbg_preproc("[COMP][S%d][%s] Turn Off LSC block\n", cis_data->stream_id, __func__);
			}
			break;
		default:
			if (pEnableBlock->uOnOff & (0x1 << 4)) {
				// LSC On
				C73C3SendByte_Reg2Data2(client, Fs_grasTuning_uBypass, 0x0000);
				cis_data->companion_data.enable_lsc = true;

				dbg_preproc("[COMP][S%d][%s] Turn On LSC block\n", cis_data->stream_id, __func__);
			} else {
				// LSC Off
				C73C3SendByte_Reg2Data2(client, Fs_grasTuning_uBypass, 0x0001);
				cis_data->companion_data.enable_lsc = false;

				dbg_preproc("[COMP][S%d][%s] Turn Off LSC block\n", cis_data->stream_id, __func__);
			}
			break;
		}
	}

	// BPC
	if (pEnableBlock->uUpdate & (0x1 << 5))
	{
		switch (cis_data->product_name) {
		case SENSOR_NAME_S5K2L1:
		case SENSOR_NAME_S5K2L2:
		case SENSOR_NAME_IMX260:
		case SENSOR_NAME_IMX333:
			if (pEnableBlock->uOnOff & (0x1 << 5)) {
				// BPC On
				C73C3SendByte_Reg2Data2(client, dspclTuning_bypass, 0x0000);
				cis_data->companion_data.enable_pdaf_bpc = true;
				dbg_preproc("[COMP][S%d][%s] Turn On BPC block\n", cis_data->stream_id, __func__);
			} else {
				// BPC Off
				C73C3SendByte_Reg2Data2(client, dspclTuning_bypass, 0x0001);
				cis_data->companion_data.enable_pdaf_bpc = false;
				dbg_preproc("[COMP][S%d][%s] Turn Off BPC block\n", cis_data->stream_id, __func__);
			}
			break;
		default:
			if (pEnableBlock->uOnOff & (0x1 << 5)) {
				// BPC On
				C73C3SendByte_Reg2Data2(client, Fs_dspclTuning_bypass, 0x0000);
				cis_data->companion_data.enable_pdaf_bpc = true;
				dbg_preproc("[COMP][S%d][%s] Turn On BPC block\n", cis_data->stream_id, __func__);
			} else {
				// BPC Off
				C73C3SendByte_Reg2Data2(client, Fs_dspclTuning_bypass, 0x0001);
				cis_data->companion_data.enable_pdaf_bpc = false;
				dbg_preproc("[COMP][S%d][%s] Turn Off BPC block\n", cis_data->stream_id, __func__);
			}
			break;
		}
	}

	// Bypass
	if (pEnableBlock->uUpdate & (0x1 << 8)) {
		if (pEnableBlock->uOnOff & (0x1 << 8)) {
			// ISP bypass mode on
			C73C3SendByte_Reg2Data2(client, api_debug_out, 0x0099);
			cis_data->companion_data.bypass = true;
			dbg_preproc("[COMP][S%d][%s] Bypass ON\n", cis_data->stream_id, __func__);
		} else {
			// ISP bypass mode off
			C73C3SendByte_Reg2Data2(client, api_debug_out, 0x0000);
			cis_data->companion_data.bypass = false;
			dbg_preproc("[COMP][S%d][%s] Bypass OFF\n", cis_data->stream_id, __func__);
		}
	}

	// Disparity
	if (pEnableBlock->uUpdate & (0x1 << 9)) {
		if (pEnableBlock->uOnOff & (0x1 << 9)) { //Disparity on
			// Disparity mode on
			C73C3SendByte_Reg2Data2(client, api_config_0__PDAFMode, 0x0102);
			// Disparity mode on
			C73C3SendByte_Reg2Data2(client, api_config_1__PDAFMode, 0x0102);
			// Disparity mode on
			C73C3SendByte_Reg2Data2(client, api_config_2__PDAFMode, 0x0102);
			info("[COMP][S%d][%s] Disparity ON\n", cis_data->stream_id, __func__);

			if (pEnableBlock->uMode & (0x1 << 2)) {
				//DISPARITY_SAD
				//SAD mode
				C73C3SendByte_Reg2Data2(client, pafTuningParams_depth_dmode,   0x0000);
				cis_data->companion_data.disparity_mode = COMPANION_DISPARITY_SAD;
				dbg_preproc("[COMP][S%d][%s] Turn On Disparity SAD mode\n", cis_data->stream_id, __func__);
			} else if (pEnableBlock->uMode & (0x1 << 3)) {
				// DISPARITY_CENSUS_MEAN
				//CENSUS MEAN mode
				C73C3SendByte_Reg2Data2(client, pafTuningParams_depth_dmode,   0x0001);
				cis_data->companion_data.disparity_mode = COMPANION_DISPARITY_CENSUS_MEAN;
				dbg_preproc("[COMP][S%d][%s] Turn On Disparity CENSUS MEAN mode\n", cis_data->stream_id, __func__);
			} else if (pEnableBlock->uMode & (0x1 << 4)) {
				// DISPARITY_CENSUS_CENTER
				//CENSUS CENTER mode
				C73C3SendByte_Reg2Data2(client, pafTuningParams_depth_dmode,   0x0002);
				cis_data->companion_data.disparity_mode = COMPANION_DISPARITY_CENSUS_CENTER;
				dbg_preproc("[COMP][S%d][%s] Turn On Disparity CENSUS CENTER mode\n", cis_data->stream_id, __func__);
			} else {
				cis_data->companion_data.disparity_mode = COMPANION_DISPARITY_OFF;
				err("[COMP][S%d][%s] Undefined Disparity mode!\n", cis_data->stream_id, __func__);
			}
		} else {
			// Disparity off
			C73C3SendByte_Reg2Data2(client, api_config_0__PDAFMode, 0x0002);
			C73C3SendByte_Reg2Data2(client, api_config_1__PDAFMode, 0x0002);
			C73C3SendByte_Reg2Data2(client, api_config_2__PDAFMode, 0x0002);

			cis_data->companion_data.disparity_mode = COMPANION_DISPARITY_OFF;
			dbg_preproc("[COMP][S%d][%s] Disparity OFF\n", cis_data->stream_id, __func__);
		}
	}

	if (pEnableBlock->uUpdate > 0 && C73C3_IsWdrEnableMode(cis_data) == true) {
		C73C3SendByte_Reg2Data2(client, 0xFCFC, 0x4000); // Change base address
	}

	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

	return ERROR_SENSOR_NO;
}

static int C73C3_MotionSetInfo(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	// Do not use. S5C73C3 calculates the motion information by itself.
	SENCMD_CompanionSetMotionInfoStr *pMotion = (SENCMD_CompanionSetMotionInfoStr*)setinfo->param;

	static u16 sLastMotion = 0;
	u16 motion = pMotion->motion;
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	if (pMotion->bHwSetEveryFrame == false) {
		if (sLastMotion == motion) {
			return ERROR_SENSOR_NO;
		}
	}

	I2C_MUTEX_LOCK(preprocessor->i2c_lock);
	C73C3SendByte_Reg2Data2(client, api_otf_motion88, motion);
	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

	sLastMotion = motion;

	return ERROR_SENSOR_NO;
}

static int C73C3_BypassSetInfo(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	SENCMD_CompanionSetBypassInfoStr *pBypass = (SENCMD_CompanionSetBypassInfoStr*)setinfo->param;

	u16 bypass = pBypass->uBypass;
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	I2C_MUTEX_LOCK(preprocessor->i2c_lock);
	if (bypass == true) {
		// Bypass on
		C73C3SendByte_Reg2Data2(client, api_debug_out, 0x0099);

		cis_data->companion_data.bypass = true;

		info("[COMP][S%d][%s] Bypass ON\n", cis_data->stream_id, __func__);
	} else {
		// Bypass off
		C73C3SendByte_Reg2Data2(client, api_debug_out, 0x0000);
		cis_data->companion_data.bypass = false;

		info("[COMP][S%d][%s] Bypass OFF\n", cis_data->stream_id, __func__);
	}
	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

	return ERROR_SENSOR_NO;
}

static int C73C3_LeModeSetInfo(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	SENCMD_CompanionSetLeModeInfoStr *pLeMode = (SENCMD_CompanionSetLeModeInfoStr*)setinfo->param;
	u16 leMode = pLeMode->uLeMode;
	bool config_idx_fastae = false;
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	switch (cis_data->product_name) {
		case SENSOR_NAME_S5K2L1:
			if (cis_data->companion_data.config_idx == C73C3_S5K2L1_CONFIG_1008X756_120) {
				config_idx_fastae = true;
			}
			break;
		case SENSOR_NAME_S5K2L2:
			if (cis_data->companion_data.config_idx == C73C3_S5K2L2_CONFIG_1008X756_120) {
				config_idx_fastae = true;
			}
			break;
		case SENSOR_NAME_IMX333:
			if (cis_data->companion_data.config_idx == C73C3_IMX333_CONFIG_1008X756_120) {
				config_idx_fastae = true;
			}
			break;
		case SENSOR_NAME_IMX260:
			if (cis_data->companion_data.config_idx == C73C3_IMX260_CONFIG_1008X756_120) {
				config_idx_fastae = true;
			}
			break;
		default:
			break;
	}

	I2C_MUTEX_LOCK(preprocessor->i2c_lock);
	// When fastae do not set.
	if (cis_data->companion_data.wdr_mode == COMPANION_WDR_OFF && !config_idx_fastae) {
		if (leMode == false) {
			// Long Exposure Mode OFF
			C73C3SendByte_Reg2Data2(client, api_otf_LEmode_Enable , 0x0000);
			info("[COMP][S%d][%s] Turn Off LE mode\n", cis_data->stream_id, __func__);
		} else {
			// Long Exposure Mode ON
			C73C3SendByte_Reg2Data2(client, api_otf_LEmode_Enable, 0x0001);
			info("[COMP][S%d][%s] Turn On LE mode\n", cis_data->stream_id, __func__);
		}
	}
	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

	return ERROR_SENSOR_NO;
}

static int C73C3_SeamlessModeSetInfo(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	SENCMD_CompanionSetSeamlessModeInfoStr *pSeamlessMode = (SENCMD_CompanionSetSeamlessModeInfoStr*)setinfo->param;

	u16 seamlessMode = pSeamlessMode->uSeamlessMode;
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	I2C_MUTEX_LOCK(preprocessor->i2c_lock);
	if (seamlessMode == true) {
		// Seamless Bypass On
		C73C3SendByte_Reg2Data2(client, api_bypassAutoAlgs, 0x003c);
		dbg_preproc("[COMP][S%d][%s] Turn On Seamless Bypass\n", cis_data->stream_id, __func__);
	} else {
		// Seamless Bypass Off
		C73C3SendByte_Reg2Data2(client, api_bypassAutoAlgs, 0x0000);
		dbg_preproc("[COMP][S%d][%s] Turn Off Seamless Bypass\n", cis_data->stream_id, __func__);
	}
	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

	return ERROR_SENSOR_NO;
}

static int C73C3_DrcSetInfo(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	SENCMD_CompanionSetDrcInfoStr *pDrc = (SENCMD_CompanionSetDrcInfoStr*)setinfo->param;

	static u32 sLastDrcGain = 0;
	u32 drcGain = pDrc->uDrcGain;
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	if (pDrc->bHwSetEveryFrame == false) {
		if (sLastDrcGain == drcGain) {
			return ERROR_SENSOR_NO;
		}
	}

	I2C_MUTEX_LOCK(preprocessor->i2c_lock);
	C73C3SendByte_Reg2Data2(client, api_otf_sceneDr, drcGain);
	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

	sLastDrcGain = drcGain;

	return ERROR_SENSOR_NO;
}

static int C73C3_IrSetInfo(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	SENCMD_CompanionSetIrInfoStr *pIr = (SENCMD_CompanionSetIrInfoStr*)setinfo->param;

	static u16 sLastIrStrength = 0;
	u16 irStrength = pIr->uIrStrength; // irStrength: [0, 256]
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	if (pIr->bHwSetEveryFrame == false) {
		if (sLastIrStrength == irStrength) {
			return ERROR_SENSOR_NO;
		}
	}

	I2C_MUTEX_LOCK(preprocessor->i2c_lock);
	C73C3SendByte_Reg2Data2(client, api_otf_InfraredStrength, irStrength);
	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

	sLastIrStrength = irStrength;

	return ERROR_SENSOR_NO;
}

static int C73C3_RoiSetInfo(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	SENCMD_CompanionSetRoiInfoStr *pRoi = (SENCMD_CompanionSetRoiInfoStr*)setinfo->param;

	static u16 sLastRoiScale = 0;
	u32 roiScale = pRoi->uRoiScale; // 0x4000 :x1, 0x2000: x2
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	if (pRoi->bHwSetEveryFrame == false) {
		if (sLastRoiScale == roiScale) {
			return ERROR_SENSOR_NO;
		}
	}

	I2C_MUTEX_LOCK(preprocessor->i2c_lock);
	C73C3SendByte_Reg2Data2(client, api_generalStat_grid_gridAreaScale, roiScale);
	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

	dbg_preproc("[COMP][S%d][%s] RoiScale %x is sent to a companion chip.\n", cis_data->stream_id, __func__, roiScale);

	sLastRoiScale = roiScale;

	return ERROR_SENSOR_NO;
}

static int C73C3_PowerMode(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	SENCMD_CompanionPowerModeCfgStr *pPowerMode = (SENCMD_CompanionPowerModeCfgStr*)setinfo->param;

	static u16 sLastPowerMode = 0;
	u16 powerMode = pPowerMode->uPowerMode;
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	if (pPowerMode->bHwSetEveryFrame == false) {
		if (sLastPowerMode == powerMode) {
			return ERROR_SENSOR_NO;
		}
	}

	I2C_MUTEX_LOCK(preprocessor->i2c_lock);
	if (powerMode == 1) {
		C73C3SendByte_Reg2Data2(client, api_otf_PowerMode, 0x0001);
	} else if (powerMode == 2) {
		C73C3SendByte_Reg2Data2(client, api_otf_PowerMode, 0x0002);
	} else if (powerMode == 3) {
		C73C3SendByte_Reg2Data2(client, api_otf_PowerMode, 0x0003);
	}
	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

	sLastPowerMode = powerMode;

	return ERROR_SENSOR_NO;
}

static int C73C3_ChangeConfig(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	SENCMD_CompanionChangeCfgStr *pChangeCfg = (SENCMD_CompanionChangeCfgStr*)setinfo->param;
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	I2C_MUTEX_LOCK(preprocessor->i2c_lock);
	C73C3SendByte_Reg2Data2(client, HOST_INTRP_ChangeConfig,
		pChangeCfg->cfgUpdateDelay);
	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

	return ERROR_SENSOR_NO;
}

static int C73C3_ReadSysReg(struct v4l2_subdev *subdev, preproc_setting_info* setinfo, cis_shared_data* cis_data)
{
	int i;
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	I2C_MUTEX_LOCK(preprocessor->i2c_lock);
	C73C3SendByte_Reg2Data2(client, 0xFCFC, 0x2000);

	for (i = 0; i < C73C3_NUM_OF_DBG_SYS_REG; i++) {
		if (C73C3_DbgSysRegAdrs[i][0] == 0x02) {
			u16 uRegVal;

			if (ReadByte_Reg2Data2(client,
				(u16)(C73C3_DbgSysRegAdrs[i][1] & 0x0000FFFF), &uRegVal) == true) {
				info("[COMP][S%d][%s] %s: 0x%08x\n", cis_data->stream_id,
					__func__, C73C3_DbgSysRegName[i], uRegVal);
			} else {
				err("[COMP][S%d][%s] ReadByte_Reg2Data2() error of %s!\n",
				cis_data->stream_id, __func__, C73C3_DbgSysRegName[i]);
			}
		} else if (C73C3_DbgSysRegAdrs[i][0] == 0x04) {
			u32 uRegVal;

			if (ReadByte_Reg2Data4(client,
				(u16)(C73C3_DbgSysRegAdrs[i][1] & 0x0000FFFF), &uRegVal) == true) {
				info("[COMP][S%d][%s] %s: 0x%08x\n", cis_data->stream_id, __func__,
					C73C3_DbgSysRegName[i], uRegVal);
			} else {
				err("[COMP][S%d][%s] ReadByte_Reg2Data2() error of %s!\n",
				cis_data->stream_id, __func__, C73C3_DbgSysRegName[i]);
			}
		} else {

		}
	}

	C73C3SendByte_Reg2Data2(client, 0xFCFC, 0x4000);
	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

	return ERROR_SENSOR_NO;
}

static int C73C3_ReadUserReg(struct v4l2_subdev *subdev, preproc_setting_info* setinfo, cis_shared_data* cis_data)
{
	static u8 sReadCnt = 0;
	int i;
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	I2C_MUTEX_LOCK(preprocessor->i2c_lock);
	if (sReadCnt == 10) {
		u8 pBuf[api_otf_reserved - api_otf] = {0};

		SendByte_Reg2Data2(client, 0xFCFC, 0x2000);

		ReadByte_Reg2DataN(client, (u16)(api_otf & 0x0000FFFF),
			pBuf, sizeof(pBuf) / sizeof(pBuf[0]));

		info("[COMP][S%d][%s] OTF Register Dump! api_otf(0x%08x) ~ api_otf_reserved(0x%08x)\n",
		cis_data->stream_id, __func__, api_otf, api_otf_reserved);

		for (i = 0; i < api_otf_reserved - api_otf; i++) {
			info("[COMP][S%d][%s] 0x%02x\n", cis_data->stream_id, __func__, pBuf[i]);
		}

		SendByte_Reg2Data2(client, 0xFCFC, 0x4000);

		sReadCnt = 0;
	} else {
		sReadCnt++;
	}
	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

	return ERROR_SENSOR_NO;
}

static int C73C3_ReadAfReg(struct v4l2_subdev *subdev, preproc_setting_info* setinfo, cis_shared_data* cis_data)
{
	u16 i;

	SENCMD_CompanionReadAfReg *pAfRegs = (SENCMD_CompanionReadAfReg *)setinfo->param;
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;

	I2C_MUTEX_LOCK(preprocessor->i2c_lock);
	SendByte_Reg2Data2(client, 0xFCFC, pAfRegs->readAddrOffset);

	for (i = 0 ; i < pAfRegs->bufSize ; i++) {
		ReadByte_Reg2Data2(client, pAfRegs->pAddrArray[i], &pAfRegs->pBufArray[i]);

		info("[COMP][S%d][%s] 0x%04x%04x: 0x%04x\n", cis_data->stream_id, __func__,
		pAfRegs->readAddrOffset, pAfRegs->pAddrArray[i], pAfRegs->pBufArray[i]);
	}

	SendByte_Reg2Data2(client, 0xFCFC, 0x4000);
	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);

	return ERROR_SENSOR_NO;
}

int C73C3_ReadAfStatFromSpi(struct spi_device *spi, preproc_setting_info* setinfo, cis_shared_data* cis_data, u8 **buf)
{
	u8 *pAfData = NULL;
	u32 maxAfDataSize = MAX_PDAF_STATS_SIZE ;
	u32 uAfDataSize, uCrc32, uCrc32Calc;
	u16 res;
	u32 uFrameCount;
	u16 usWindowsX, usWindowsY;
	u8 pReadTemp[4];
	u8 uAfCmd0[5]={0x02,0x64,0x2c,0x20,0x01};
	u8 uAfCmd1[5]={0x02,0x64,0x2e,0xFA,0x00};
	bool crc_result = false;
        int ret = ERROR_SENSOR_NO;

	SENCMD_CompanionReadAfStatFromSpiStr *pAfRegs =
		(SENCMD_CompanionReadAfStatFromSpiStr *)setinfo->param;

	pAfData = *buf;

	memset(pAfData, 0x0, maxAfDataSize);

	ISPFWLld_SPI_Write_For_73c1(spi, &uAfCmd0[0]);
	ISPFWLld_SPI_Write_For_73c1(spi, &uAfCmd1[0]);

	// Get Size
	res = ISPFWLld_SPI_ReadBurst_For_73c1(spi, 0x6f12, pReadTemp, 4);
	if ( res != 0 ) {
		err("Companion AF size read fail\n");
                ret = ERROR_SENSOR_SPI_FAIL;
		goto exit;
	}

	uAfDataSize = (pReadTemp[0] << 24) | (pReadTemp[1] << 16) | (pReadTemp[2] << 8) | pReadTemp[3];
        pAfRegs->pAfDataSize = uAfDataSize;

	// Add WA for MIPI error (check size value with promised value & print error log)
	if ( uAfDataSize == 0xDEADDEAD ) {
		err("Companion rx fail\n");
                ret = ERROR_SENSOR_INVALID_SIZE;
		goto exit;
	}

	if ( uAfDataSize > MAX_PDAF_STATS_SIZE  || uAfDataSize < MIN_PDAF_STATS_SIZE  ) {
		err("AF data size is not valid. size = 0x%08x", uAfDataSize);
                ret = ERROR_SENSOR_INVALID_SIZE;
		goto exit;
	}

	// Read uAfDataSize + CRC data(4 byte)
	res = ISPFWLld_SPI_ReadBurst_For_73c1(spi, 0x6f12, pAfData, uAfDataSize + 4);
	if ( res != 0 ) {
		err("Companion AF data read fail!Buf Size: %d\n", uAfDataSize + 4);
                ret = ERROR_SENSOR_SPI_FAIL;
		goto exit;
	}

	// Exclude meta info size. Now, uAfDataSize is real PDAF data size
	uAfDataSize -= 8;

	// Get Frame count
	uFrameCount = (pAfData[uAfDataSize] << 24) | (pAfData[uAfDataSize+1] << 16) |
		(pAfData[uAfDataSize+2] << 8) | pAfData[uAfDataSize+3];

	// Get ROI
	usWindowsX = (pAfData[uAfDataSize+4] << 8) |  pAfData[uAfDataSize+5];
	usWindowsY = (pAfData[uAfDataSize+6] << 8) |  pAfData[uAfDataSize+7];

	// Get CRC32
	uCrc32 = (pAfData[uAfDataSize+8] << 24) | (pAfData[uAfDataSize+9] << 16) |
		(pAfData[uAfDataSize+10] << 8) | pAfData[uAfDataSize+11];

	// Compute CRC32 from data
	// CRC calc region = Size info(4 byte) + PDAF data + Meta(8 byte)
	uCrc32Calc = CalcCRC(pReadTemp, 4, pAfData, uAfDataSize+8);

	if ( uCrc32 != uCrc32Calc ) {
		err("crc check fail!!CRC32(Read): 0x%08x, CRC32(Calc): 0x%08x\n",
			uCrc32, uCrc32Calc);
		ret = ERROR_SENSOR_CRC_FAIL;
	} else {
		crc_result = true;
	}

exit:
	pAfRegs->pAfData = pAfData;
	pAfRegs->crcResult = crc_result;
	pAfRegs->isValid = true;

	return ret;
}

static int C73C3_DeInit(struct v4l2_subdev *subdev, preproc_setting_info *setinfo, cis_shared_data *cis_data)
{
	struct fimc_is_preprocessor *preprocessor;
	struct i2c_client *client;

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	client = preprocessor->client;
#if 0 /* HACK */
	I2C_MUTEX_LOCK(preprocessor->i2c_lock);
	/* Run FW/Data CRC on Close */
	C73C3SendByte_Reg2Data2(client, HOST_INTRP_AutoCrc, 0x0001);
	I2C_MUTEX_UNLOCK(preprocessor->i2c_lock);
#endif
	info("[COMP][S%d][%s]\n", cis_data->stream_id, __func__);

	return ERROR_SENSOR_NO;
}

int init_comp_func_73c3(sencmd_func *func_list, cis_shared_data *cis_data, bool actuator, u32 sensor_name)
{
	func_list[COMP_CMD_INIT] = C73C3_Init;
	func_list[COMP_CMD_SET_SIZE] = C73C3_SetSize;
	func_list[COMP_CMD_STREAM_ON] = C73C3_StreamOn;
	func_list[COMP_CMD_STREAM_OFF] = C73C3_StreamOff;
	func_list[COMP_CMD_MODE_CHANGE] = C73C3_ModeChange;
	func_list[COMP_CMD_LSC_SET_3AA_INFO] = C73C3_LscSet3aaInfo;
	func_list[COMP_CMD_WDR_SET_AE_INFO] = C73C3_WdrSetAeInfo;
	func_list[COMP_CMD_AF_INFO] = C73C3_AfSetInfo;
	func_list[COMP_CMD_BLOCK_ON_OFF] = C73C3_BlockOnOff;
	// Do not use. S5C73C3 calculates the motion information by itself.
	func_list[COMP_CMD_MOTION_SET_INFO] = C73C3_MotionSetInfo;
	func_list[COMP_CMD_DRC_SET_INFO] = C73C3_DrcSetInfo;
	func_list[COMP_CMD_IR_SET_INFO] = C73C3_IrSetInfo;
	func_list[COMP_CMD_ROI_SET_INFO] = C73C3_RoiSetInfo;
	func_list[COMP_CMD_BYPASS_SET_INFO] = C73C3_BypassSetInfo;
	func_list[COMP_CMD_POWER_MODE] = C73C3_PowerMode;
	func_list[COMP_CMD_CHANGE_CONFIG] = C73C3_ChangeConfig;
	func_list[COMP_CMD_READ_SYS_REG] = C73C3_ReadSysReg;
	func_list[COMP_CMD_READ_USER_REG] = C73C3_ReadUserReg;
	func_list[COMP_CMD_READ_AF_REG] = C73C3_ReadAfReg;
	func_list[COMP_CMD_LEMODE_SET_INFO] = C73C3_LeModeSetInfo;
	func_list[COMP_CMD_SEAMLESSMODE_SET_INFO] = C73C3_SeamlessModeSetInfo;
	func_list[COMP_CMD_UPDATE_SUPPORT_MODE] = C73C3_updateSupportMode;
	func_list[COMP_CMD_DEINIT] = C73C3_DeInit;

	// Initialization
	switch (sensor_name) {
		case SENSOR_NAME_S5K2L1:
			cis_data->companion_data.config_idx = C73C3_S5K2L1_CONFIG_8064x3024_30;
			break;
		case SENSOR_NAME_S5K2L2:
			cis_data->companion_data.config_idx = C73C3_S5K2L2_CONFIG_8064x3024_30;
			break;
		case SENSOR_NAME_IMX333:
			cis_data->companion_data.config_idx = C73C3_IMX333_CONFIG_8064x3024_30;
			break;
		case SENSOR_NAME_IMX260:
			cis_data->companion_data.config_idx = C73C3_IMX260_CONFIG_8064x3024_30;
			break;
		case SENSOR_NAME_S5K3H1:
			cis_data->companion_data.config_idx = C73C3_S5K3H1_CONFIG_3264X2448_30;
			break;
		case SENSOR_NAME_S5K4E6:
			cis_data->companion_data.config_idx = C73C3_S5K4E6_CONFIG_2608X1960_30;
			break;
		case SENSOR_NAME_IMX320:
			cis_data->companion_data.config_idx = C73C3_IMX320_CONFIG_3264X2448_30;
			break;
		default:
			break;
	}

	cis_data->companion_data.bypass = false;
	cis_data->companion_data.paf_stat_enable = false;
	cis_data->companion_data.caf_stat_enable = false;
	cis_data->companion_data.enable_lsc = true;

	cis_data->companion_data.paf_mode = COMPANION_PAF_OFF;
	cis_data->companion_data.caf_mode = COMPANION_CAF_OFF;
	cis_data->companion_data.wdr_mode = COMPANION_WDR_OFF;
	cis_data->companion_data.disparity_mode = COMPANION_DISPARITY_OFF;

	cis_data->companion_data.enable_drc = true;
	cis_data->companion_data.enable_pdaf_bpc = true;
	cis_data->companion_data.enable_xtalk = true;

	g_CrcLookupTable = (u32 *)kmalloc(sizeof(u32) * CRC_TABLE_SIZE, GFP_KERNEL);
	if (!g_CrcLookupTable) {
		err("mem alloc failed. (%lu bytes)", CRC_TABLE_SIZE * sizeof(u32));
                return -ENOMEM;
	}

	GenCrcTable();

	return 0;
}
