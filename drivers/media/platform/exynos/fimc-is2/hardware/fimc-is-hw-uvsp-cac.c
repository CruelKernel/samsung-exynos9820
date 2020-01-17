/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-mcscaler-v2.h"
#include "api/fimc-is-hw-api-mcscaler-v2.h"
#include "../interface/fimc-is-interface-ischain.h"
#include "fimc-is-param.h"
#include "fimc-is-err.h"

/* NI from DDK needs to adjust scale factor (by multipling 10) */
#define MULTIPLIED_10(value)		(10 * (value))
#define INTRPL_SHFT_VAL			(12)
#define SUB(a, b)		(int)(((a) > (b)) ? ((a) - (b)) : ((b) - (a)))
#define LSHFT(a)		((int)((a) << INTRPL_SHFT_VAL))
#define RSHFT(a)		((int)((a) >> INTRPL_SHFT_VAL))
#define NUMERATOR(Y1, Y2, DXn)			(((Y2) - (Y1)) * (DXn))
#define CALC_LNR_INTRPL(Y1, Y2, X1, X2, X)	(LSHFT(NUMERATOR(Y1, Y2, SUB(X, X1))) / SUB(X2, X1) + LSHFT(Y1))
#define GET_LNR_INTRPL(Y1, Y2, X1, X2, X)	RSHFT(SUB(X2, X1) ? CALC_LNR_INTRPL(Y1, Y2, X1, X2, X) : LSHFT(Y1))

struct ref_ni {
	u32 min;
	u32 max;
};

struct ref_ni hw_mcsc_find_ni_idx_for_cac(struct fimc_is_hw_ip *hw_ip,
	struct cac_setfile_contents *cac, u32 cur_ni)
{
	struct ref_ni ret_idx = {0, 0};
	struct ref_ni ni_idx_range;
	u32 ni_idx;

	for (ni_idx = 0; ni_idx < cac->ni_max - 1; ni_idx++) {
		ni_idx_range.min = MULTIPLIED_10(cac->ni_vals[ni_idx]);
		ni_idx_range.max = MULTIPLIED_10(cac->ni_vals[ni_idx + 1]);

		if (ni_idx_range.min < cur_ni && cur_ni < ni_idx_range.max) {
			ret_idx.min = ni_idx;
			ret_idx.max = ni_idx + 1;
			break;
		} else if (cur_ni == ni_idx_range.min) {
			ret_idx.min = ni_idx;
			ret_idx.max = ni_idx;
			break;
		} else if (cur_ni == ni_idx_range.max) {
			ret_idx.min = ni_idx + 1;
			ret_idx.max = ni_idx + 1;
			break;
		}
	}

	sdbg_hw(2, "[CAC] find_ni_idx: cur_ni %d idx %d,%d range %d,%d\n", hw_ip, cur_ni,
		ret_idx.min, ret_idx.max,
		MULTIPLIED_10(cac->ni_vals[ret_idx.min]), MULTIPLIED_10(cac->ni_vals[ret_idx.max]));

	return ret_idx;
}

void hw_mcsc_calc_cac_map_thr(struct cac_cfg_by_ni *cac_cfg,
	struct cac_cfg_by_ni *cfg_min, struct cac_cfg_by_ni *cfg_max,
	struct ref_ni *ni, u32 cur_ni)
{
	struct cac_map_thr_cfg *min, *max;

	min = &cfg_min->map_thr_cfg;
	max = &cfg_max->map_thr_cfg;

	cac_cfg->map_thr_cfg.map_spot_thr_l = GET_LNR_INTRPL(
			min->map_spot_thr_l, max->map_spot_thr_l,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[CAC] map_thr.map_spot_thr_l: set_val %d range %d,%d\n",
			cac_cfg->map_thr_cfg.map_spot_thr_l,
			min->map_spot_thr_l, max->map_spot_thr_l);

	cac_cfg->map_thr_cfg.map_spot_thr_h = GET_LNR_INTRPL(
			min->map_spot_thr_h, max->map_spot_thr_h,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[CAC] map_thr.map_spot_thr_h: set_val %d range %d,%d\n",
			cac_cfg->map_thr_cfg.map_spot_thr_h,
			min->map_spot_thr_h, max->map_spot_thr_h);

	cac_cfg->map_thr_cfg.map_spot_thr = GET_LNR_INTRPL(
			min->map_spot_thr, max->map_spot_thr,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[CAC] map_thr.map_spot_thr: set_val %d range %d,%d\n",
			cac_cfg->map_thr_cfg.map_spot_thr,
			min->map_spot_thr, max->map_spot_thr);

	cac_cfg->map_thr_cfg.map_spot_nr_strength = GET_LNR_INTRPL(
			min->map_spot_nr_strength, max->map_spot_nr_strength,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[CAC] map_thr.map_spot_nr_strength: set_val %d range %d,%d\n",
			cac_cfg->map_thr_cfg.map_spot_nr_strength,
			min->map_spot_nr_strength, max->map_spot_nr_strength);
}

void hw_mcsc_calc_cac_crt_thr(struct cac_cfg_by_ni *cac_cfg,
	struct cac_cfg_by_ni *cfg_min, struct cac_cfg_by_ni *cfg_max,
	struct ref_ni *ni, u32 cur_ni)
{
	struct cac_crt_thr_cfg *min, *max;

	min = &cfg_min->crt_thr_cfg;
	max = &cfg_max->crt_thr_cfg;

	cac_cfg->crt_thr_cfg.crt_color_thr_l_dot = GET_LNR_INTRPL(
			min->crt_color_thr_l_dot, max->crt_color_thr_l_dot,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[CAC] crt_thr.crt_color_thr_l_dot: set_val %d range %d,%d\n",
			cac_cfg->crt_thr_cfg.crt_color_thr_l_dot,
			min->crt_color_thr_l_dot, max->crt_color_thr_l_dot);

	cac_cfg->crt_thr_cfg.crt_color_thr_l_line = GET_LNR_INTRPL(
			min->crt_color_thr_l_line, max->crt_color_thr_l_line,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[CAC] crt_thr.crt_color_thr_l_line: set_val %d range %d,%d\n",
			cac_cfg->crt_thr_cfg.crt_color_thr_l_line,
			min->crt_color_thr_l_line, max->crt_color_thr_l_line);

	cac_cfg->crt_thr_cfg.crt_color_thr_h = GET_LNR_INTRPL(
			min->crt_color_thr_h, max->crt_color_thr_h,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[CAC] crt_thr.crt_color_thr_h: set_val %d range %d,%d\n",
			cac_cfg->crt_thr_cfg.crt_color_thr_h,
			min->crt_color_thr_h, max->crt_color_thr_h);
}

void hw_mcsc_calc_cac_param_by_ni(struct fimc_is_hw_ip *hw_ip,
	struct cac_setfile_contents *cac, u32 cur_ni)
{
	struct ref_ni ni_range, ni_idx, ni;
	struct cac_cfg_by_ni cac_cfg, cfg_max, cfg_min;

	ni_range.min = MULTIPLIED_10(cac->ni_vals[0]);
	ni_range.max = MULTIPLIED_10(cac->ni_vals[cac->ni_max - 1]);

	if (cur_ni <= ni_range.min) {
		sdbg_hw(2, "[CAC] cur_ni(%d) <= ni_range.min(%d)\n", hw_ip, cur_ni, ni_range.min);
		ni_idx.min = 0;
		ni_idx.max = 0;
	} else if (cur_ni >= ni_range.max) {
		sdbg_hw(2, "[CAC] cur_ni(%d) >= ni_range.max(%d)\n", hw_ip, cur_ni, ni_range.max);
		ni_idx.min = cac->ni_max - 1;
		ni_idx.max = cac->ni_max - 1;
	} else {
		ni_idx = hw_mcsc_find_ni_idx_for_cac(hw_ip, cac, cur_ni);
	}

	cfg_min = cac->cfgs[ni_idx.min];
	cfg_max = cac->cfgs[ni_idx.max];
	ni.min = MULTIPLIED_10(cac->ni_vals[ni_idx.min]);
	ni.max = MULTIPLIED_10(cac->ni_vals[ni_idx.max]);

	hw_mcsc_calc_cac_map_thr(&cac_cfg, &cfg_min, &cfg_max, &ni, cur_ni);
	hw_mcsc_calc_cac_crt_thr(&cac_cfg, &cfg_min, &cfg_max, &ni, cur_ni);

	fimc_is_scaler_set_cac_map_crt_thr(hw_ip->regs, &cac_cfg);
	fimc_is_scaler_set_cac_enable(hw_ip->regs, 1);
}

int fimc_is_hw_mcsc_update_cac_register(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_frame *frame, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap;
	struct hw_mcsc_setfile *setfile;
#if defined(USE_UVSP_CAC)
	struct cac_setfile_contents *cac;
#endif
	enum exynos_sensor_position sensor_position;
	u32 ni, backup_in, core_num, ni_index;
	bool cac_en = true;

	FIMC_BUG(!hw_ip->priv_info);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	cap = GET_MCSC_HW_CAP(hw_ip);

	if (cap->cac != MCSC_CAP_SUPPORT)
		return ret;

	backup_in = hw_mcsc->cac_in;
	if (hw_ip->hardware->video_mode)
		hw_mcsc->cac_in = MCSC_CAC_IN_VIDEO_MODE;
	else
		hw_mcsc->cac_in = MCSC_CAC_IN_CAPTURE_MODE;

	/* The force means that sysfs control has higher priority than scenario. */
	fimc_is_hw_mcsc_get_force_block_control(hw_ip, SUBBLK_IP_CAC, cap->max_cac,
		&hw_mcsc->cac_in, &cac_en);

	if (backup_in != hw_mcsc->cac_in)
		sdbg_hw(0, "cac input_source changed %d-> %d\n", hw_ip,
			backup_in - DEV_HW_MCSC0, hw_mcsc->cac_in - DEV_HW_MCSC0);

	fimc_is_scaler_set_cac_input_source(hw_ip->regs, (hw_mcsc->cac_in - DEV_HW_MCSC0));

	if (hw_mcsc->cac_in != hw_ip->id)
		return ret;

	if (cac_en == false) {
		fimc_is_scaler_set_cac_enable(hw_ip->regs, cac_en);
		sinfo_hw("CAC off forcely\n", hw_ip);
		return ret;
	}

	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = hw_mcsc->cur_setfile[sensor_position][instance];

	/* calculate cac parameters */
	core_num = GET_CORE_NUM(hw_ip->id);
	ni_index = frame->fcount % NI_BACKUP_MAX;
#ifdef FIXED_TDNR_NOISE_INDEX
	ni = FIXED_TDNR_NOISE_INDEX_VALUE;
#else
	ni = hw_ip->hardware->ni_udm[core_num][ni_index].currentFrameNoiseIndex;
#endif
	if (hw_mcsc->cur_ni[SUBBLK_CAC] == ni)
		goto exit;

#if defined(USE_UVSP_CAC)
	cac = &setfile->cac;
	hw_mcsc_calc_cac_param_by_ni(hw_ip, cac, ni);
#endif
	msdbg_hw(2, "[CAC][F:%d]: ni(%d)\n",
		instance, hw_ip, frame->fcount, ni);

exit:
	hw_mcsc->cur_ni[SUBBLK_CAC] = ni;

	return 0;
}

int fimc_is_hw_mcsc_recovery_cac_register(struct fimc_is_hw_ip *hw_ip,
		struct is_param_region *param, u32 instance)
{
	int ret = 0;
	/* TODO */

	return ret;
}

struct ref_ni hw_mcsc_find_ni_idx_for_uvsp(struct fimc_is_hw_ip *hw_ip,
	struct uvsp_setfile_contents *uvsp, u32 cur_ni)
{
	struct ref_ni ret_idx = {0, 0};
	struct ref_ni ni_idx_range;
	u32 ni_idx;

	for (ni_idx = 0; ni_idx < uvsp->ni_max - 1; ni_idx++) {
		ni_idx_range.min = MULTIPLIED_10(uvsp->ni_vals[ni_idx]);
		ni_idx_range.max = MULTIPLIED_10(uvsp->ni_vals[ni_idx + 1]);

		if (ni_idx_range.min < cur_ni && cur_ni < ni_idx_range.max) {
			ret_idx.min = ni_idx;
			ret_idx.max = ni_idx + 1;
			break;
		} else if (cur_ni == ni_idx_range.min) {
			ret_idx.min = ni_idx;
			ret_idx.max = ni_idx;
			break;
		} else if (cur_ni == ni_idx_range.max) {
			ret_idx.min = ni_idx + 1;
			ret_idx.max = ni_idx + 1;
			break;
		}
	}

	sdbg_hw(2, "[UVSP] find_ni_idx: cur_ni %d idx %d,%d range %d,%d\n", hw_ip, cur_ni,
		ret_idx.min, ret_idx.max,
		MULTIPLIED_10(uvsp->ni_vals[ret_idx.min]), MULTIPLIED_10(uvsp->ni_vals[ret_idx.max]));

	return ret_idx;
}

void hw_mcsc_calc_uvsp_r2y_coef_cfg(struct uvsp_cfg_by_ni *uvsp_cfg,
	struct uvsp_cfg_by_ni *cfg_min, struct uvsp_cfg_by_ni *cfg_max,
	struct ref_ni *ni, u32 cur_ni)
{
	struct uvsp_r2y_coef_cfg *min, *max, *cfg;

	min = &cfg_min->r2y_coef_cfg;
	max = &cfg_max->r2y_coef_cfg;
	cfg = &uvsp_cfg->r2y_coef_cfg;

	cfg->r2y_coef_00 = GET_LNR_INTRPL(
			min->r2y_coef_00, max->r2y_coef_00,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] r2y_coef_cfg.r2y_coef_00: set_val %d range %d,%d\n",
			cfg->r2y_coef_00, min->r2y_coef_00, max->r2y_coef_00);

	cfg->r2y_coef_01 = GET_LNR_INTRPL(
			min->r2y_coef_01, max->r2y_coef_01,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] r2y_coef_cfg.r2y_coef_01: set_val %d range %d,%d\n",
			cfg->r2y_coef_01, min->r2y_coef_01, max->r2y_coef_01);

	cfg->r2y_coef_02 = GET_LNR_INTRPL(
			min->r2y_coef_02, max->r2y_coef_02,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] r2y_coef_cfg.r2y_coef_02: set_val %d range %d,%d\n",
			cfg->r2y_coef_02, min->r2y_coef_02, max->r2y_coef_02);

	cfg->r2y_coef_10 = GET_LNR_INTRPL(
			min->r2y_coef_10, max->r2y_coef_10,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] r2y_coef_cfg.r2y_coef_10: set_val %d range %d,%d\n",
			cfg->r2y_coef_10, min->r2y_coef_10, max->r2y_coef_10);

	cfg->r2y_coef_11 = GET_LNR_INTRPL(
			min->r2y_coef_11, max->r2y_coef_11,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] r2y_coef_cfg.r2y_coef_11: set_val %d range %d,%d\n",
			cfg->r2y_coef_11, min->r2y_coef_11, max->r2y_coef_11);

	cfg->r2y_coef_12 = GET_LNR_INTRPL(
			min->r2y_coef_12, max->r2y_coef_12,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] r2y_coef_cfg.r2y_coef_12: set_val %d range %d,%d\n",
			cfg->r2y_coef_12, min->r2y_coef_12, max->r2y_coef_12);

	cfg->r2y_coef_20 = GET_LNR_INTRPL(
			min->r2y_coef_20, max->r2y_coef_20,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] r2y_coef_cfg.r2y_coef_20: set_val %d range %d,%d\n",
			cfg->r2y_coef_20, min->r2y_coef_20, max->r2y_coef_20);

	cfg->r2y_coef_21 = GET_LNR_INTRPL(
			min->r2y_coef_21, max->r2y_coef_21,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] r2y_coef_cfg.r2y_coef_21: set_val %d range %d,%d\n",
			cfg->r2y_coef_21, min->r2y_coef_21, max->r2y_coef_21);

	cfg->r2y_coef_22 = GET_LNR_INTRPL(
			min->r2y_coef_22, max->r2y_coef_22,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] r2y_coef_cfg.r2y_coef_22: set_val %d range %d,%d\n",
			cfg->r2y_coef_22, min->r2y_coef_22, max->r2y_coef_22);

	cfg->r2y_coef_shift = GET_LNR_INTRPL(
			min->r2y_coef_shift, max->r2y_coef_shift,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] r2y_coef_cfg.r2y_coef_shift: set_val %d range %d,%d\n",
			cfg->r2y_coef_shift, min->r2y_coef_shift, max->r2y_coef_shift);
}

void hw_mcsc_calc_uvsp_desat_cfg(struct uvsp_cfg_by_ni *uvsp_cfg,
	struct uvsp_cfg_by_ni *cfg_min, struct uvsp_cfg_by_ni *cfg_max,
	struct ref_ni *ni, u32 cur_ni)
{
	struct uvsp_desat_cfg *min, *max, *cfg;

	min = &cfg_min->desat_cfg;
	max = &cfg_max->desat_cfg;
	cfg = &uvsp_cfg->desat_cfg;

	cfg->ctrl_en = GET_LNR_INTRPL(
			min->ctrl_en, max->ctrl_en,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] desat_cfg.ctrl_en: set_val %d range %d,%d\n",
			cfg->ctrl_en, min->ctrl_en, max->ctrl_en);

	cfg->ctrl_singleSide = GET_LNR_INTRPL(
			min->ctrl_singleSide, max->ctrl_singleSide,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] desat_cfg.ctrl_singleSide: set_val %d range %d,%d\n",
			cfg->ctrl_singleSide, min->ctrl_singleSide, max->ctrl_singleSide);

	cfg->ctrl_luma_offset = GET_LNR_INTRPL(
			min->ctrl_luma_offset, max->ctrl_luma_offset,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] desat_cfg.ctrl_luma_offset: set_val %d range %d,%d\n",
			cfg->ctrl_luma_offset, min->ctrl_luma_offset, max->ctrl_luma_offset);

	cfg->ctrl_gain_offset = GET_LNR_INTRPL(
			min->ctrl_gain_offset, max->ctrl_gain_offset,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] desat_cfg.ctrl_gain_offset: set_val %d range %d,%d\n",
			cfg->ctrl_gain_offset, min->ctrl_gain_offset, max->ctrl_gain_offset);

	cfg->y_shift = GET_LNR_INTRPL(
			min->y_shift, max->y_shift,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] desat_cfg.y_shift: set_val %d range %d,%d\n",
			cfg->y_shift, min->y_shift, max->y_shift);

	cfg->y_luma_max = GET_LNR_INTRPL(
			min->y_luma_max, max->y_luma_max,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] desat_cfg.y_luma_max: set_val %d range %d,%d\n",
			cfg->y_luma_max, min->y_luma_max, max->y_luma_max);

	cfg->u_low = GET_LNR_INTRPL(
			min->u_low, max->u_low,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] desat_cfg.u_low: set_val %d range %d,%d\n",
			cfg->u_low, min->u_low, max->u_low);

	cfg->u_high = GET_LNR_INTRPL(
			min->u_high, max->u_high,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] desat_cfg.u_high: set_val %d range %d,%d\n",
			cfg->u_high, min->u_high, max->u_high);

	cfg->v_low = GET_LNR_INTRPL(
			min->v_low, max->v_low,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] desat_cfg.v_low: set_val %d range %d,%d\n",
			cfg->v_low, min->v_low, max->v_low);

	cfg->v_high = GET_LNR_INTRPL(
			min->v_high, max->v_high,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] desat_cfg.v_high: set_val %d range %d,%d\n",
			cfg->v_high, min->v_high, max->v_high);
}

void hw_mcsc_calc_uvsp_offset_cfg(struct uvsp_cfg_by_ni *uvsp_cfg,
	struct uvsp_cfg_by_ni *cfg_min, struct uvsp_cfg_by_ni *cfg_max,
	struct ref_ni *ni, u32 cur_ni)
{
	struct uvsp_offset_cfg *min, *max, *cfg;

	min = &cfg_min->offset_cfg;
	max = &cfg_max->offset_cfg;
	cfg = &uvsp_cfg->offset_cfg;

	cfg->r = GET_LNR_INTRPL(
			min->r, max->r,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] offset_cfg.r: set_val %d range %d,%d\n",
			cfg->r, min->r, max->r);

	cfg->g = GET_LNR_INTRPL(
			min->g, max->g,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] offset_cfg.g: set_val %d range %d,%d\n",
			cfg->g, min->g, max->g);

	cfg->b = GET_LNR_INTRPL(
			min->b, max->b,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] offset_cfg.b: set_val %d range %d,%d\n",
			cfg->b, min->b, max->b);
}

void hw_mcsc_calc_uvsp_pedestal_cfg(struct uvsp_cfg_by_ni *uvsp_cfg,
	struct uvsp_cfg_by_ni *cfg_min, struct uvsp_cfg_by_ni *cfg_max,
	struct ref_ni *ni, u32 cur_ni)
{
	struct uvsp_pedestal_cfg *min, *max, *cfg;

	min = &cfg_min->pedestal_cfg;
	max = &cfg_max->pedestal_cfg;
	cfg = &uvsp_cfg->pedestal_cfg;

	cfg->r = GET_LNR_INTRPL(
			min->r, max->r,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] pedestal_cfg.r: set_val %d range %d,%d\n",
			cfg->r, min->r, max->r);

	cfg->g = GET_LNR_INTRPL(
			min->g, max->g,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] pedestal_cfg.g: set_val %d range %d,%d\n",
			cfg->g, min->g, max->g);

	cfg->b = GET_LNR_INTRPL(
			min->b, max->b,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] pedestal_cfg.b: set_val %d range %d,%d\n",
			cfg->b, min->b, max->b);
}

void hw_mcsc_calc_uvsp_radial_cfg(struct uvsp_cfg_by_ni *uvsp_cfg,
	struct uvsp_cfg_by_ni *cfg_min, struct uvsp_cfg_by_ni *cfg_max,
	struct ref_ni *ni, u32 cur_ni)
{
	struct uvsp_radial_cfg *min, *max, *cfg;

	min = &cfg_min->radial_cfg;
	max = &cfg_max->radial_cfg;
	cfg = &uvsp_cfg->radial_cfg;

	cfg->biquad_shift = GET_LNR_INTRPL(
			min->biquad_shift, max->biquad_shift,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] radial_cfg.biquad_shift: set_val %d range %d,%d\n",
			cfg->biquad_shift, min->biquad_shift, max->biquad_shift);

	cfg->random_en = GET_LNR_INTRPL(
			min->random_en, max->random_en,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] radial_cfg.random_en: set_val %d range %d,%d\n",
			cfg->random_en, min->random_en, max->random_en);

	cfg->random_power = GET_LNR_INTRPL(
			min->random_power, max->random_power,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] radial_cfg.random_power: set_val %d range %d,%d\n",
			cfg->random_power, min->random_power, max->random_power);

	cfg->refine_en = GET_LNR_INTRPL(
			min->refine_en, max->refine_en,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] radial_cfg.refine_en: set_val %d range %d,%d\n",
			cfg->refine_en, min->refine_en, max->refine_en);

	cfg->refine_luma_min = GET_LNR_INTRPL(
			min->refine_luma_min, max->refine_luma_min,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] radial_cfg.refine_luma_min: set_val %d range %d,%d\n",
			cfg->refine_luma_min, min->refine_luma_min, max->refine_luma_min);

	cfg->refine_denom = GET_LNR_INTRPL(
			min->refine_denom, max->refine_denom,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] radial_cfg.refine_denom: set_val %d range %d,%d\n",
			cfg->refine_denom, min->refine_denom, max->refine_denom);

	cfg->alpha_gain_add_en = GET_LNR_INTRPL(
			min->alpha_gain_add_en, max->alpha_gain_add_en,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] radial_cfg.alpha_gain_add_en: set_val %d range %d,%d\n",
			cfg->alpha_gain_add_en, min->alpha_gain_add_en, max->alpha_gain_add_en);

	cfg->alpha_green_en = GET_LNR_INTRPL(
			min->alpha_green_en, max->alpha_green_en,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] radial_cfg.alpha_gain_add_en: set_val %d range %d,%d\n",
			cfg->alpha_gain_add_en, min->alpha_gain_add_en, max->alpha_gain_add_en);

	cfg->alpha_r = GET_LNR_INTRPL(
			min->alpha_r, max->alpha_r,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] radial_cfg.alpha_r: set_val %d range %d,%d\n",
			cfg->alpha_r, min->alpha_r, max->alpha_r);

	cfg->alpha_g = GET_LNR_INTRPL(
			min->alpha_g, max->alpha_g,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] radial_cfg.alpha_g: set_val %d range %d,%d\n",
			cfg->alpha_g, min->alpha_g, max->alpha_g);

	cfg->alpha_b = GET_LNR_INTRPL(
			min->alpha_b, max->alpha_b,
			ni->min, ni->max, cur_ni);
	dbg_hw(2, "[UVSP] radial_cfg.alpha_b: set_val %d range %d,%d\n",
			cfg->alpha_b, min->alpha_b, max->alpha_b);
}

void hw_mcsc_calc_uvsp_param_by_ni(struct fimc_is_hw_ip *hw_ip,
	struct uvsp_setfile_contents *uvsp, u32 cur_ni)
{
	struct ref_ni ni_range, ni_idx, ni;
	struct uvsp_cfg_by_ni uvsp_cfg, cfg_max, cfg_min;

	ni_range.min = MULTIPLIED_10(uvsp->ni_vals[0]);
	ni_range.max = MULTIPLIED_10(uvsp->ni_vals[uvsp->ni_max - 1]);

	if (cur_ni <= ni_range.min) {
		sdbg_hw(2, "[UVSP] cur_ni(%d) <= ni_range.min(%d)\n", hw_ip, cur_ni, ni_range.min);
		ni_idx.min = 0;
		ni_idx.max = 0;
	} else if (cur_ni >= ni_range.max) {
		sdbg_hw(2, "[UVSP] cur_ni(%d) >= ni_range.max(%d)\n", hw_ip, cur_ni, ni_range.max);
		ni_idx.min = uvsp->ni_max - 1;
		ni_idx.max = uvsp->ni_max - 1;
	} else {
		ni_idx = hw_mcsc_find_ni_idx_for_uvsp(hw_ip, uvsp, cur_ni);
	}

	cfg_min = uvsp->cfgs[ni_idx.min];
	cfg_max = uvsp->cfgs[ni_idx.max];
	ni.min = MULTIPLIED_10(uvsp->ni_vals[ni_idx.min]);
	ni.max = MULTIPLIED_10(uvsp->ni_vals[ni_idx.max]);

	hw_mcsc_calc_uvsp_radial_cfg(&uvsp_cfg, &cfg_min, &cfg_max, &ni, cur_ni);
	hw_mcsc_calc_uvsp_pedestal_cfg(&uvsp_cfg, &cfg_min, &cfg_max, &ni, cur_ni);
	hw_mcsc_calc_uvsp_offset_cfg(&uvsp_cfg, &cfg_min, &cfg_max, &ni, cur_ni);
	hw_mcsc_calc_uvsp_desat_cfg(&uvsp_cfg, &cfg_min, &cfg_max, &ni, cur_ni);
	hw_mcsc_calc_uvsp_r2y_coef_cfg(&uvsp_cfg, &cfg_min, &cfg_max, &ni, cur_ni);

	fimc_is_scaler_set_uvsp_radial_cfg(hw_ip->regs, hw_ip->id, &uvsp_cfg.radial_cfg);
	fimc_is_scaler_set_uvsp_pedestal_cfg(hw_ip->regs, hw_ip->id, &uvsp_cfg.pedestal_cfg);
	fimc_is_scaler_set_uvsp_offset_cfg(hw_ip->regs, hw_ip->id, &uvsp_cfg.offset_cfg);
	fimc_is_scaler_set_uvsp_r2y_coef_cfg(hw_ip->regs, hw_ip->id, &uvsp_cfg.r2y_coef_cfg);
	fimc_is_scaler_set_uvsp_enable(hw_ip->regs, hw_ip->id, 1);
}

void hw_mcsc_calc_uvsp_radial_ctrl(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_hw_mcsc *hw_mcsc, struct fimc_is_frame *frame,
	struct cal_info *cal_info, u32 instance)
{
	struct uvsp_ctrl *uvsp_ctrl;
	struct camera2_shot_ext *shot_ext;
	u32 lsc_center_x, lsc_center_y;

	uvsp_ctrl = &hw_mcsc->uvsp_ctrl;
	lsc_center_x = cal_info->data[0];
	lsc_center_y = cal_info->data[1];
	uvsp_ctrl->biquad_a = cal_info->data[2];
	uvsp_ctrl->biquad_b = cal_info->data[3];
	msdbg_hw(2, "[UVSP][F:%d]: lsc_center_x,y %d,%d uvsp_ctrl->biquad_a,b %d,%d\n",
		instance, hw_ip, frame->fcount, lsc_center_x, lsc_center_y,
		uvsp_ctrl->biquad_a, uvsp_ctrl->biquad_b);

	shot_ext = frame->shot_ext;
	if (!frame->shot_ext) {
		sdbg_hw(2, "[F:%d] shot_ext(NULL)\n", hw_ip, frame->fcount);
		fimc_is_scaler_set_uvsp_enable(hw_ip->regs, hw_ip->id, 0);
		return;
	}

	msdbg_hw(2, "[UVSP][F:%d]: shot binning_r %d,%d crop_taa %d,%d bds %d,%d\n",
		instance, hw_ip, frame->fcount,
		shot_ext->binning_ratio_x, shot_ext->binning_ratio_y,
		shot_ext->crop_taa_x, shot_ext->crop_taa_y,
		shot_ext->bds_ratio_x, shot_ext->bds_ratio_y);

	uvsp_ctrl->binning_x = shot_ext->binning_ratio_x * shot_ext->bds_ratio_x * 1024;
	uvsp_ctrl->binning_y = shot_ext->binning_ratio_y * shot_ext->bds_ratio_y * 1024;
	if (shot_ext->bds_ratio_x && shot_ext->bds_ratio_y) {
		shot_ext->bds_ratio_x = 1;
		shot_ext->bds_ratio_y = 1;
	}
	uvsp_ctrl->radial_center_x = (u32)((lsc_center_x - shot_ext->crop_taa_x) / shot_ext->bds_ratio_x);
	uvsp_ctrl->radial_center_y = (u32)((lsc_center_y - shot_ext->crop_taa_y) / shot_ext->bds_ratio_y);

	fimc_is_scaler_set_uvsp_radial_ctrl(hw_ip->regs, hw_ip->id, uvsp_ctrl);
	msdbg_hw(2, "[UVSP][F:%d]: set_val binning %d,%d r_center %d,%d biquad %d,%d\n",
		instance, hw_ip, frame->fcount, uvsp_ctrl->binning_x, uvsp_ctrl->binning_y,
		uvsp_ctrl->radial_center_x, uvsp_ctrl->radial_center_y,
		uvsp_ctrl->biquad_a, uvsp_ctrl->biquad_b);
}

int fimc_is_hw_mcsc_update_uvsp_register(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_frame *frame, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap;
	struct hw_mcsc_setfile *setfile;
#if defined(USE_UVSP_CAC)
	struct uvsp_setfile_contents *uvsp;
#endif
	struct cal_info *cal_info;
	enum exynos_sensor_position sensor_position;
	u32 ni, core_num, ni_index;
	bool uvsp_en = true;

	FIMC_BUG(!hw_ip->priv_info);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	cap = GET_MCSC_HW_CAP(hw_ip);

	if (cap->uvsp != MCSC_CAP_SUPPORT)
		return ret;

	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = hw_mcsc->cur_setfile[sensor_position][instance];
	cal_info = &hw_ip->hardware->cal_info[sensor_position];

	sdbg_hw(10, "TEST: get_lnr_intprl(11, 20, 1, 10, 3) = %d\n",
			hw_ip, GET_LNR_INTRPL(11, 20, 1, 10, 3));
	sdbg_hw(10, "TEST: get_lnr_intprl(11, 20, 1, 1, 3) = %d\n",
			hw_ip, GET_LNR_INTRPL(11, 20, 1, 1, 3));
	sdbg_hw(10, "TEST: get_lnr_intprl(20, 11, 1, 10, 3) = %d\n",
			hw_ip, GET_LNR_INTRPL(20, 11, 1, 10, 3));
	sdbg_hw(10, "TEST: get_lnr_intprl(-20, -11, 1, 10, 3) = %d\n",
			hw_ip, GET_LNR_INTRPL(-20, -11, 1, 10, 3));

	/* calculate cac parameters */
	core_num = GET_CORE_NUM(hw_ip->id);
	ni_index = frame->fcount % NI_BACKUP_MAX;
#ifdef FIXED_TDNR_NOISE_INDEX
	ni = FIXED_TDNR_NOISE_INDEX_VALUE;
#else
	ni = hw_ip->hardware->ni_udm[core_num][ni_index].currentFrameNoiseIndex;
#endif
	if (hw_mcsc->cur_ni[SUBBLK_UVSP] == ni)
		goto exit;

	/* The force means that sysfs control has higher priority than scenario. */
	fimc_is_hw_mcsc_get_force_block_control(hw_ip, SUBBLK_IP_UVSP, cap->max_uvsp,
		NULL, &uvsp_en);

	if (uvsp_en == false) {
		fimc_is_scaler_set_uvsp_enable(hw_ip->regs, hw_ip->id, uvsp_en);
		sinfo_hw("UVSP off forcely\n", hw_ip);
		return ret;
	}

	hw_mcsc_calc_uvsp_radial_ctrl(hw_ip, hw_mcsc, frame, cal_info, instance);
#if defined(USE_UVSP_CAC)
	uvsp = &setfile->uvsp;
	hw_mcsc_calc_uvsp_param_by_ni(hw_ip, uvsp, ni);
#endif
	msdbg_hw(2, "[UVSP][F:%d]: ni(%d)\n",
		instance, hw_ip, frame->fcount, ni);

exit:
	hw_mcsc->cur_ni[SUBBLK_UVSP] = ni;

	return 0;
}
