 /*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * BTS file for Samsung EXYNOS DPU driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "decon.h"
#include "dpp.h"

#include <soc/samsung/bts.h>
#include <media/v4l2-subdev.h>
#if defined(CONFIG_CAL_IF)
#include <soc/samsung/cal-if.h>
#endif
#if defined(CONFIG_SOC_EXYNOS9810)
#include <dt-bindings/clock/exynos9810.h>
#elif defined(CONFIG_SOC_EXYNOS9820)
#include <dt-bindings/clock/exynos9820.h>
#endif

#ifdef FEATURE_DP_UNDERRUN_TEST
#include "displayport.h"
#include <linux/time.h>
#include <linux/sched/clock.h>
#endif

#ifdef CONFIG_AVOID_TSP_NOISE
#define TSP_INTER_MIN		134000
#define TSP_INTER_MAX		333000
#define ACLK_AVOID_INTER 	333000
#endif


#define DISP_FACTOR		100UL
#define LCD_REFRESH_RATE	63UL
#define MULTI_FACTOR 		(1UL << 10)
/* bus utilization 75% */
#define BUS_UTIL		75

#define DPP_SCALE_NONE		0
#define DPP_SCALE_UP		1
#define DPP_SCALE_DOWN		2

#define ACLK_100MHZ_PERIOD	10000UL
#define ACLK_MHZ_INC_STEP	50UL	/* 50Mhz */
#define FRAME_TIME_NSEC		1000000000UL	/* 1sec */
#define TOTAL_BUS_LATENCY	3000UL	/* 3us: BUS(1) + PWT(1) + Requst(1) */

/* tuning parameters for rotation */
#define ROTATION_FACTOR_BPP	32UL
#define ROTATION_FACTOR_SCUP	1332UL	/* 1.3x */
#define ROTATION_FACTOR_SCDN	1434UL	/* 1.4x */
#define RESOL_QHDP_21_TO_9	3440*1440UL	/* for MIF min-lock */

#ifdef FEATURE_DP_UNDERRUN_TEST
#define DP_MAX_IDX 32
#define DP_MAX_LINE 128
static char bts_test_buf[DP_MAX_IDX][DP_MAX_LINE];
static int bts_test_idx;
static int bts_test_log_en = 1;

static void dpu_bts_test_add_log(int decon_id, u32 total_bts, u32 prev_total, int is_after)
{
	u64 time;
	unsigned long nsec;

	if (!bts_test_log_en)
		return;

	time = local_clock();
	nsec = do_div(time, 1000000000);

	snprintf(bts_test_buf[bts_test_idx], DP_MAX_LINE, "[%5lu.%06ld] decon%d(%d) bw(%d->%d), mif(%lu), int(%lu), disp(%lu)\n",
			(unsigned long)time, nsec / 1000, decon_id, is_after, prev_total, total_bts,
			cal_dfs_get_rate(ACPM_DVFS_MIF),
			cal_dfs_get_rate(ACPM_DVFS_INT),
			cal_dfs_get_rate(ACPM_DVFS_DISP));

	if (++bts_test_idx >= DP_MAX_IDX)
		bts_test_idx = 0;
}
void dpu_bts_log_enable(int en)
{
	bts_test_log_en = en;
}

void dpu_bts_print_log(void)
{
	int i;

	for (i = 0; i < DP_MAX_IDX; i++) {
		displayport_info("%s", bts_test_buf[i]);
	}
}
#endif

/* unit : usec x 1000 -> 5592 (5.592us) for WQHD+ case */
static inline u32 dpu_bts_get_one_line_time(struct decon_lcd *lcd_info)
{
	u32 tot_v;
	int tmp;

	tot_v = lcd_info->yres + lcd_info->vfp + lcd_info->vsa + lcd_info->vbp;
	tmp = (FRAME_TIME_NSEC + lcd_info->fps - 1) / lcd_info->fps;

	return (tmp / tot_v);
}

/* lmc : line memory count (usually 4) */
static inline u32 dpu_bts_afbc_latency(u32 src_w, u32 ppc, u32 lmc)
{
	return ((src_w * lmc) / ppc);
}

/*
 * line memory max size : 4096
 * lmc : line memory count (usually 4)
 */
static inline u32 dpu_bts_scale_latency(u32 is_s, u32 src_w, u32 dst_w,
		u32 ppc, u32 lmc)
{
	u64 lat_scale = 0UL;
	u64 line_w;

	/*
	 * line_w : reflecting scale-ratio
	 * INC for scale-down & DEC for scale-up
	 */
	if (is_s == DPP_SCALE_DOWN)
		line_w = src_w * (src_w * 1000UL) / dst_w;
	else
		line_w = src_w * 1000UL;
	lat_scale = (line_w * lmc) / (ppc * 1000UL);

	return (u32)lat_scale;
}

/*
 * src_h : height of original input source image
 * cpl : cycles per line
 */
static inline u32 dpu_bts_rotate_latency(u32 src_h, u32 cpl)
{
	return (src_h * cpl * 12 / 10);
}

/*
 * [DSC]
 * Line memory is necessary like followings.
 *  1EA : 2-line for 2-slice, 1-line for 1-slice
 *  2EA : 3.5-line for 4-slice (DSCC 0.5-line + DSC 3-line)
 *        2.5-line for 2-slice (DSCC 0.5-line + DSC 2-line)
 *
 * [DECON] none
 * When 1H is filled at OUT_FIFO, it immediately transfers to DSIM.
 */
static inline u32 dpu_bts_dsc_latency(u32 slice_num, u32 dsc_cnt, u32 dst_w)
{
	u32 lat_dsc = dst_w;

	switch (slice_num) {
	case 1:
		/* DSC: 1EA */
		lat_dsc = dst_w * 1;
		break;
	case 2:
		if (dsc_cnt == 1)
			lat_dsc = dst_w * 2;
		else
			lat_dsc = (dst_w * 25) / 10;
		break;
	case 4:
		/* DSC: 2EA */
		lat_dsc = (dst_w * 35) / 10;
		break;
	default:
		break;
	}

	return lat_dsc;
}

/*
 * unit : cycles
 * rotate and afbc are incompatible
 */
static u32 dpu_bts_get_initial_latency(bool is_r, u32 is_s, bool is_c,
		struct decon_lcd *lcd_info, u32 src_w, u32 dst_w, u32 ppc,
		u32 cpl, u32 lmc)
{
	u32 lat_cycle = 0;
	u32 tmp;

	if (lcd_info->dsc_enabled) {
		lat_cycle = dpu_bts_dsc_latency(lcd_info->dsc_slice_num,
				lcd_info->dsc_cnt, dst_w);
		DPU_DEBUG_BTS("\tDSC_lat_cycle(%d)\n", lat_cycle);
	}

	/* src_w : rotation reflected value */
	if (is_r) {
		tmp = dpu_bts_rotate_latency(src_w, cpl);
		DPU_DEBUG_BTS("\tR_lat_cycle(%d)\n", tmp);
		lat_cycle += tmp;
	}
	if (is_s) {
		tmp = dpu_bts_scale_latency(is_s, src_w, dst_w, ppc, lmc);
		DPU_DEBUG_BTS("\tS_lat_cycle(%d)\n", tmp);
		lat_cycle += tmp;
	}
	if (is_c) {
		tmp = dpu_bts_afbc_latency(src_w, ppc, lmc);
		DPU_DEBUG_BTS("\tC_lat_cycle(%d)\n", tmp);
		lat_cycle += tmp;
	}

	return lat_cycle;
}

/*
 * unit : nsec x 1000
 * reference aclk : 100MHz (-> 10ns x 1000)
 */
static inline u32 dpu_bts_get_aclk_period_time(u32 aclk_mhz)
{
	return ((ACLK_100MHZ_PERIOD * 100) / aclk_mhz);
}

/* check 10-bit yuv format image */
static u32 dpu_bts_check_yuv_10bit(enum decon_pixel_format fmt, bool is_yuv)
{
	u32 bpp = 32;
	u32 is_yuv10 = 0;

	bpp = dpu_get_bpp(fmt);
	if (is_yuv && ((bpp == 15 || bpp == 24) || (bpp == 20 || bpp == 32)))
		is_yuv10 = 1;

	return is_yuv10;
}

/* find min-ACLK to meet latency */
static u32 dpu_bts_find_latency_meet_aclk(u32 lat_cycle, u32 line_time,
		u32 criteria_v, u32 aclk_disp,
		bool is_yuv10, bool is_rotate, u32 rot_factor)
{
	u64 aclk_mhz = aclk_disp / 1000UL;
	u64 aclk_period, lat_time;
	u64 lat_time_max;

	DPU_DEBUG_BTS("\t(rot_factor = %d) (is_yuv10 = %d)\n",
			rot_factor, is_yuv10);

	/* lat_time_max: usec x 1000 */
	lat_time_max = (u64)line_time * (u64)criteria_v;

	/* find min-ACLK to able to cover initial latency */
	while (1) {
		/* aclk_period: nsec x 1000 */
		aclk_period = dpu_bts_get_aclk_period_time(aclk_mhz);
		lat_time = (lat_cycle * aclk_period) / 1000UL;
		lat_time = lat_time << is_yuv10;
		lat_time += TOTAL_BUS_LATENCY;
		if (is_rotate)
			lat_time = (lat_time * rot_factor) / MULTI_FACTOR;

		DPU_DEBUG_BTS("\tloop: (aclk_period = %d) (lat_time = %d)\n",
			(u32)aclk_period, (u32)lat_time);
		if (lat_time < lat_time_max)
			break;

		aclk_mhz += ACLK_MHZ_INC_STEP;
	}

	DPU_DEBUG_BTS("\t(lat_time = %d) (lat_time_max = %d)\n",
		(u32)lat_time, (u32)lat_time_max);

	return (u32)(aclk_mhz * 1000UL);
}


u64 dpu_bts_calc_aclk_disp(struct decon_device *decon,
		struct decon_win_config *config, u64 resol_clock)
{
	u64 s_ratio_h, s_ratio_v;
	u64 aclk_disp;
	u64 ppc;
	struct decon_frame *src = &config->src;
	struct decon_frame *dst = &config->dst;
	u32 src_w, src_h;
	bool is_rotate = is_rotation(config) ? true : false;
	bool is_comp = is_afbc(config) ? true : false;
	bool yuv_fmt = is_yuv(config) ? true : false;
	u32 is_scale, is_yuv10;
	u32 lat_cycle, line_time;
	u32 cycle_per_line, line_mem_cnt;
	u32 criteria_v, tot_v;
	u32 rot_factor = ROTATION_FACTOR_SCUP;

	if (is_rotate) {
		src_w = src->h;
		src_h = src->w;
	} else {
		src_w = src->w;
		src_h = src->h;
	}

	s_ratio_h = (src_w <= dst->w) ? MULTI_FACTOR : MULTI_FACTOR * (u64)src_w / (u64)dst->w;
	s_ratio_v = (src_h <= dst->h) ? MULTI_FACTOR : MULTI_FACTOR * (u64)src_h / (u64)dst->h;

	/* case for using dsc encoder 1ea at decon0 or decon1 */
	if ((decon->id != 2) && (decon->lcd_info->dsc_cnt == 1))
		ppc = ((decon->bts.ppc / 2UL) >= 1UL) ? (decon->bts.ppc / 2UL) : 1UL;
	else
		ppc = decon->bts.ppc;

	aclk_disp = resol_clock * s_ratio_h * s_ratio_v * DISP_FACTOR  / 100UL
		/ ppc * (MULTI_FACTOR * (u64)dst->w / (u64)decon->lcd_info->xres)
		/ (MULTI_FACTOR * MULTI_FACTOR * MULTI_FACTOR);

	if (aclk_disp < (resol_clock / ppc))
		aclk_disp = resol_clock / ppc;

	DPU_DEBUG_BTS("BEFORE latency calc: aclk_disp = %d\n", (u32)aclk_disp);

	/* scaling latency: width only */
	if (src_w < dst->w)
		is_scale = DPP_SCALE_UP;
	else if (src_w > dst->w) {
		is_scale = DPP_SCALE_DOWN;
		rot_factor = ROTATION_FACTOR_SCDN;
	} else
		is_scale = DPP_SCALE_NONE;

	/* to check initial latency */
	cycle_per_line = decon->bts.cycle_per_line;
	line_mem_cnt = decon->bts.line_mem_cnt;
	lat_cycle = dpu_bts_get_initial_latency(is_rotate, is_scale, is_comp,
			decon->lcd_info, src_w, dst->w, (u32)ppc, cycle_per_line,
			line_mem_cnt);
	line_time = dpu_bts_get_one_line_time(decon->lcd_info);

	if (decon->lcd_info->mode == DECON_VIDEO_MODE)
		criteria_v = decon->lcd_info->vbp;
	else {
		/* command mode margin : apply 20% of v-blank time */
		tot_v = decon->lcd_info->vfp + decon->lcd_info->vsa
			+ decon->lcd_info->vbp;
		criteria_v = tot_v - (tot_v * 20 / 100);
	}

	is_yuv10 = dpu_bts_check_yuv_10bit(config->format, yuv_fmt);

	aclk_disp = dpu_bts_find_latency_meet_aclk(lat_cycle, line_time,
			criteria_v, aclk_disp, is_yuv10, is_rotate, rot_factor);

#ifdef CONFIG_AVOID_TSP_NOISE
	if ((decon->lcd_info->mres_mode == DSU_MODE_2) ||
		(decon->lcd_info->mres_mode == DSU_MODE_3)) {

		if ((aclk_disp > TSP_INTER_MIN) &&
			(aclk_disp < TSP_INTER_MAX)) {
			decon_dbg("aclk : %d -> %d\n", aclk_disp, ACLK_AVOID_INTER);
			aclk_disp = ACLK_AVOID_INTER;
		}
	}
#endif

	DPU_DEBUG_BTS("\t[R:%d C:%d S:%d] (lat_cycle=%d) (line_time=%d)\n",
		is_rotate, is_comp, is_scale, lat_cycle, line_time);
	DPU_DEBUG_BTS("AFTER latency calc: aclk_disp = %d\n", (u32)aclk_disp);

	return aclk_disp;
}

static void dpu_bts_sum_all_decon_bw(struct decon_device *decon, u32 ch_bw[])
{
	int i, j;

	if (decon->id < 0 || decon->id >= decon->dt.decon_cnt) {
		decon_warn("[%s] undefined decon id(%d)!\n", __func__, decon->id);
		return;
	}

	for (i = 0; i < BTS_DPU_MAX; ++i)
		decon->bts.ch_bw[decon->id][i] = ch_bw[i];

	for (i = 0; i < decon->dt.decon_cnt; ++i) {
		if (decon->id == i)
			continue;

		for (j = 0; j < BTS_DPU_MAX; ++j)
			ch_bw[j] += decon->bts.ch_bw[i][j];
	}
}

static void dpu_bts_find_max_disp_freq(struct decon_device *decon,
		struct decon_reg_data *regs)
{
	int i, j;
	u32 disp_ch_bw[BTS_DPU_MAX];
	u32 max_disp_ch_bw;
	u32 disp_op_freq = 0, freq = 0;
	u64 resol_clock;
	u64 op_fps = LCD_REFRESH_RATE;
	struct decon_win_config *config = regs->dpp_config;

	memset(disp_ch_bw, 0, sizeof(disp_ch_bw));

	for (i = 0; i < BTS_DPP_MAX; ++i)
		for (j = 0; j < BTS_DPU_MAX; ++j)
			if (decon->bts.bw[i].ch_num == j)
				disp_ch_bw[j] += decon->bts.bw[i].val;

	/* must be considered other decon's bw */
	dpu_bts_sum_all_decon_bw(decon, disp_ch_bw);

	for (i = 0; i < BTS_DPU_MAX; ++i)
		if (disp_ch_bw[i])
			DPU_DEBUG_BTS("\tCH%d = %d\n", i, disp_ch_bw[i]);

	max_disp_ch_bw = disp_ch_bw[0];
	for (i = 1; i < BTS_DPU_MAX; ++i)
		if (max_disp_ch_bw < disp_ch_bw[i])
			max_disp_ch_bw = disp_ch_bw[i];

	decon->bts.peak = max_disp_ch_bw;
	decon->bts.max_disp_freq = max_disp_ch_bw * 100 / (16 * BUS_UTIL) + 1;

	if (decon->dt.out_type == DECON_OUT_DP)
		op_fps = decon->lcd_info->fps;

	/* 1.1: 10% margin, 1000: for KHZ, 1: for raising to a unit */
	resol_clock = decon->lcd_info->xres * decon->lcd_info->yres *
		op_fps * 11 / 10 / 1000 + 1;
	decon->bts.resol_clk = resol_clock;

	DPU_DEBUG_BTS("\tDECON%d : resol clock = %d Khz\n",
		decon->id, decon->bts.resol_clk);

	for (i = 0; i < decon->dt.max_win; ++i) {
		if ((config[i].state != DECON_WIN_STATE_BUFFER) &&
				(config[i].state != DECON_WIN_STATE_COLOR))
			continue;

		freq = dpu_bts_calc_aclk_disp(decon, &config[i], resol_clock);
		if (disp_op_freq < freq)
			disp_op_freq = freq;
	}

	DPU_DEBUG_BTS("\tDISP bus freq(%d), operating freq(%d)\n",
			decon->bts.max_disp_freq, disp_op_freq);

	if (decon->bts.max_disp_freq < disp_op_freq)
		decon->bts.max_disp_freq = disp_op_freq;

	DPU_DEBUG_BTS("\tMAX DISP CH FREQ = %d\n", decon->bts.max_disp_freq);
}

static void dpu_bts_share_bw_info(int id)
{
	int i, j;
	struct decon_device *decon[3];
	int decon_cnt;

	decon_cnt = get_decon_drvdata(0)->dt.decon_cnt;

	for (i = 0; i < MAX_DECON_CNT; i++)
		decon[i] = NULL;

	for (i = 0; i < decon_cnt; i++)
		decon[i] = get_decon_drvdata(i);

	for (i = 0; i < decon_cnt; ++i) {
		if (id == i || decon[i] == NULL)
			continue;

		for (j = 0; j < BTS_DPU_MAX; ++j)
			decon[i]->bts.ch_bw[id][j] = decon[id]->bts.ch_bw[id][j];
	}
}

void dpu_bts_calc_bw(struct decon_device *decon, struct decon_reg_data *regs)
{
	struct decon_win_config *config = regs->dpp_config;
	struct bts_decon_info bts_info;
	enum dpp_rotate rot;
	enum decon_pixel_format fmt;
	int idx, i;

	if (!decon->bts.enabled)
		return;

	DPU_DEBUG_BTS("\n");
	DPU_DEBUG_BTS("%s + : DECON%d\n", __func__, decon->id);

	memset(&bts_info, 0, sizeof(struct bts_decon_info));
	for (i = 0; i < decon->dt.max_win; ++i) {
		if (config[i].state == DECON_WIN_STATE_BUFFER) {
			idx = DPU_CH2DMA(config[i].idma_type); /* ch */
			/*
			 * TODO: Array index of bts_info structure uses dma type.
			 * This array index will be changed to DPP channel number
			 * in the future.
			 */
			bts_info.dpp[idx].used = true;
		} else {
			continue;
		}

		fmt = config[i].format;
		bts_info.dpp[idx].bpp = dpu_get_bpp(fmt);
		bts_info.dpp[idx].src_w = config[i].src.w;
		bts_info.dpp[idx].src_h = config[i].src.h;
		bts_info.dpp[idx].dst.x1 = config[i].dst.x;
		bts_info.dpp[idx].dst.x2 = config[i].dst.x + config[i].dst.w;
		bts_info.dpp[idx].dst.y1 = config[i].dst.y;
		bts_info.dpp[idx].dst.y2 = config[i].dst.y + config[i].dst.h;
		rot = config[i].dpp_parm.rot;
		bts_info.dpp[idx].rotation = (rot > DPP_ROT_180) ? true : false;

		/*
		 * [ GUIDE : #if 0 ]
		 * Need to apply twice instead of x1.25 as many bandwidth
		 * of 8-bit YUV if it is a 8P2 format rotation.
		 *
		 * [ APPLY : #else ]
		 * In case of rotation, MIF & INT and DISP clock frequencies
		 * are important factors related to the issue of underrun.
		 * So, relatively high frequency is required to avoid underrun.
		 * By using 32 instead of 12/15/24 as bpp(bit-per-pixel) value,
		 * MIF & INT frequency can be increased because
		 * those are decided by the bandwidth.
		 * ROTATION_FACTOR_BPP(= ARGB8888 value) is a tunable value.
		 */
		if (bts_info.dpp[idx].rotation) {
			#if 0
			if (fmt == DECON_PIXEL_FORMAT_NV12M_S10B ||
				fmt == DECON_PIXEL_FORMAT_NV21M_S10B)
					bts_info.dpp[idx].bpp = 24;
			#else
			bts_info.dpp[idx].bpp = ROTATION_FACTOR_BPP;
			#endif
		}

		DPU_DEBUG_BTS("\tDPP%d : bpp(%d) src w(%d) h(%d) rot(%d)\n",
				idx, bts_info.dpp[idx].bpp,
				bts_info.dpp[idx].src_w, bts_info.dpp[idx].src_h,
				bts_info.dpp[idx].rotation);
		DPU_DEBUG_BTS("\t\t\t\tdst x(%d) right(%d) y(%d) bottom(%d)\n",
				bts_info.dpp[idx].dst.x1,
				bts_info.dpp[idx].dst.x2,
				bts_info.dpp[idx].dst.y1,
				bts_info.dpp[idx].dst.y2);
	}

	bts_info.vclk = decon->bts.resol_clk;
	bts_info.lcd_w = decon->lcd_info->xres;
	bts_info.lcd_h = decon->lcd_info->yres;
	decon->bts.total_bw = bts_calc_bw(decon->bts.type, &bts_info);
	memcpy(&decon->bts.bts_info, &bts_info, sizeof(struct bts_decon_info));

	for (i = 0; i < BTS_DPP_MAX; ++i) {
		decon->bts.bw[i].val = bts_info.dpp[i].bw;
		if (decon->bts.bw[i].val)
			DPU_DEBUG_BTS("\tDPP%d bandwidth = %d\n",
					i, decon->bts.bw[i].val);
	}

	DPU_DEBUG_BTS("\tDECON%d total bandwidth = %d\n", decon->id,
			decon->bts.total_bw);

	dpu_bts_find_max_disp_freq(decon, regs);

	/* update bw for other decons */
	dpu_bts_share_bw_info(decon->id);

	DPU_DEBUG_BTS("%s -\n", __func__);
}

void dpu_bts_update_bw(struct decon_device *decon, struct decon_reg_data *regs,
		u32 is_after)
{
	struct bts_bw bw = { 0, };
#if defined(CONFIG_EXYNOS_DISPLAYPORT)
	struct displayport_device *displayport = get_displayport_drvdata();
	videoformat cur = displayport->cur_video;
	__u64 pixelclock = supported_videos[cur].dv_timings.bt.pixelclock;
#endif

	DPU_DEBUG_BTS("%s +\n", __func__);

	if (!decon->bts.enabled)
		return;

	/* update peak & read bandwidth per DPU port */
	bw.peak = decon->bts.peak;
	bw.read = decon->bts.total_bw;
	DPU_DEBUG_BTS("\tpeak = %d, read = %d\n", bw.peak, bw.read);

	if (bw.read == 0)
		bw.peak = 0;

	if (is_after) { /* after DECON h/w configuration */
		if (decon->bts.total_bw <= decon->bts.prev_total_bw)
			bts_update_bw(decon->bts.type, bw);

#if defined(CONFIG_EXYNOS_DISPLAYPORT)
		if ((displayport->state == DISPLAYPORT_STATE_ON)
			&& (pixelclock >= UHD_60HZ_PIXEL_CLOCK)) /* 4K DP case */
			return;
#endif
		if (decon->bts.max_disp_freq <= decon->bts.prev_max_disp_freq)
			pm_qos_update_request(&decon->bts.disp_qos,
					decon->bts.max_disp_freq);
#ifdef FEATURE_DP_UNDERRUN_TEST
		dpu_bts_test_add_log(decon->id, decon->bts.total_bw, decon->bts.prev_total_bw, 1);
#endif
		decon->bts.prev_total_bw = decon->bts.total_bw;
		decon->bts.prev_max_disp_freq = decon->bts.max_disp_freq;
	} else {
		if (decon->bts.total_bw > decon->bts.prev_total_bw)
			bts_update_bw(decon->bts.type, bw);

#if defined(CONFIG_EXYNOS_DISPLAYPORT)
		if ((displayport->state == DISPLAYPORT_STATE_ON)
			&& (pixelclock >= UHD_60HZ_PIXEL_CLOCK)) /* 4K DP case */
			return;
#endif

		if (decon->bts.max_disp_freq > decon->bts.prev_max_disp_freq)
			pm_qos_update_request(&decon->bts.disp_qos,
					decon->bts.max_disp_freq);
#ifdef FEATURE_DP_UNDERRUN_TEST
		dpu_bts_test_add_log(decon->id, decon->bts.total_bw, decon->bts.prev_total_bw, 0);
#endif
	}

	DPU_DEBUG_BTS("%s -\n", __func__);
}

void dpu_bts_acquire_bw(struct decon_device *decon)
{
#if defined(CONFIG_DECON_BTS_LEGACY) && defined(CONFIG_EXYNOS_DISPLAYPORT)
	struct displayport_device *displayport = get_displayport_drvdata();
	videoformat cur = displayport->cur_video;
	__u64 pixelclock = supported_videos[cur].dv_timings.bt.pixelclock;
#endif
	struct decon_win_config config;
	u64 resol_clock;
	u32 aclk_freq = 0;

	DPU_DEBUG_BTS("%s +\n", __func__);

	if (!decon->bts.enabled)
		return;

	if (decon->dt.out_type == DECON_OUT_DSI) {
		memset(&config, 0, sizeof(struct decon_win_config));
		config.src.w = config.dst.w = decon->lcd_info->xres;
		config.src.h = config.dst.h = decon->lcd_info->yres;
		resol_clock = decon->lcd_info->xres * decon->lcd_info->yres *
			LCD_REFRESH_RATE * 11 / 10 / 1000 + 1;
		aclk_freq = dpu_bts_calc_aclk_disp(decon, &config, resol_clock);
		DPU_DEBUG_BTS("Initial calculated disp freq(%lu)\n", aclk_freq);
		/*
		 * If current disp freq is higher than calculated freq,
		 * it must not be set. if not, underrun can occur.
		 */
		if (cal_dfs_get_rate(ACPM_DVFS_DISP) < aclk_freq)
			pm_qos_update_request(&decon->bts.disp_qos, aclk_freq);

		DPU_DEBUG_BTS("Get initial disp freq(%lu)\n",
				cal_dfs_get_rate(ACPM_DVFS_DISP));

		return;
	}

#if defined(CONFIG_DECON_BTS_LEGACY) && defined(CONFIG_EXYNOS_DISPLAYPORT)
	if (decon->dt.out_type != DECON_OUT_DP)
		return;

	if (pixelclock >= UHD_60HZ_PIXEL_CLOCK) {
		if (pm_qos_request_active(&decon->bts.mif_qos))
			pm_qos_update_request(&decon->bts.mif_qos, 1794 * 1000);
		else
			DPU_ERR_BTS("%s mif qos setting error\n", __func__);

		if (pm_qos_request_active(&decon->bts.int_qos))
			pm_qos_update_request(&decon->bts.int_qos, 534 * 1000);
		else
			DPU_ERR_BTS("%s int qos setting error\n", __func__);

		if (pm_qos_request_active(&decon->bts.disp_qos))
			pm_qos_update_request(&decon->bts.disp_qos, 400 * 1000);
		else
			DPU_ERR_BTS("%s int qos setting error\n", __func__);

		if (!decon->bts.scen_updated) {
			decon->bts.scen_updated = 1;
			bts_update_scen(BS_DP_DEFAULT, 1);
		}
	} else { /* pixelclock < 533000000 ? */ 
		if (pm_qos_request_active(&decon->bts.mif_qos))
			pm_qos_update_request(&decon->bts.mif_qos, 1352 * 1000);
		else
			DPU_ERR_BTS("%s mif qos setting error\n", __func__);
	}

	DPU_DEBUG_BTS("%s: decon%d, pixelclock(%u)\n", __func__, decon->id,
			pixelclock);
#endif
}

void dpu_bts_release_bw(struct decon_device *decon)
{
	struct bts_bw bw = { 0, };
	DPU_DEBUG_BTS("%s +\n", __func__);

	if (!decon->bts.enabled)
		return;

	if (decon->dt.out_type == DECON_OUT_DSI) {
		bts_update_bw(decon->bts.type, bw);
		decon->bts.prev_total_bw = 0;
		pm_qos_update_request(&decon->bts.disp_qos, 0);
		decon->bts.prev_max_disp_freq = 0;
	} else if (decon->dt.out_type == DECON_OUT_DP) {
#if defined(CONFIG_DECON_BTS_LEGACY) && defined(CONFIG_EXYNOS_DISPLAYPORT)
		if (pm_qos_request_active(&decon->bts.mif_qos))
			pm_qos_update_request(&decon->bts.mif_qos, 0);
		else
			DPU_ERR_BTS("%s mif qos setting error\n", __func__);

		if (pm_qos_request_active(&decon->bts.int_qos))
			pm_qos_update_request(&decon->bts.int_qos, 0);
		else
			DPU_ERR_BTS("%s int qos setting error\n", __func__);

		if (pm_qos_request_active(&decon->bts.disp_qos))
			pm_qos_update_request(&decon->bts.disp_qos, 0);
		else
			DPU_ERR_BTS("%s int qos setting error\n", __func__);

		if (decon->bts.scen_updated) {
			decon->bts.scen_updated = 0;
			bts_update_scen(BS_DP_DEFAULT, 0);
		}
#endif
	}

	DPU_DEBUG_BTS("%s -\n", __func__);
}

void dpu_bts_init(struct decon_device *decon)
{
	int comp_ratio;
	int i;
	struct v4l2_subdev *sd = NULL;

	DPU_DEBUG_BTS("%s +\n", __func__);

	decon->bts.enabled = false;

	if (!IS_ENABLED(CONFIG_EXYNOS_BTS)) {
		DPU_ERR_BTS("decon%d bts feature is disabled\n", decon->id);
		return;
	}

	if (decon->id == 1)
		decon->bts.type = BTS_BW_DECON1;
	else if (decon->id == 2)
		decon->bts.type = BTS_BW_DECON2;
	else
		decon->bts.type = BTS_BW_DECON0;

	for (i = 0; i < BTS_DPU_MAX; i++)
		decon->bts.ch_bw[decon->id][i] = 0;

	DPU_DEBUG_BTS("BTS_BW_TYPE(%d) -\n", decon->bts.type);

	if (decon->lcd_info->dsc_enabled)
		comp_ratio = 3;
	else
		comp_ratio = 1;

	if (decon->dt.out_type == DECON_OUT_DP) {
		/*
		* Decon2-DP : various resolutions are available
		* therefore, set max resolution clock at init phase to avoid underrun
		*/
		decon->bts.resol_clk = (u32)((u64)4096 * 2160 * 60 * 11
				/ 10 / 1000 + 1);
	} else {
		/*
		 * Resol clock(KHZ) = lcd width x lcd height x 63(refresh rate) x
		 *               1.1(10% margin) x comp_ratio(1/3 DSC) / 2(2PPC) /
		 *		1000(for KHZ) + 1(for raising to a unit)
		 */
		decon->bts.resol_clk = (u32)((u64)decon->lcd_info->xres *
				(u64)decon->lcd_info->yres *
				LCD_REFRESH_RATE * 11 / 10 / 1000 + 1);
	}
	DPU_DEBUG_BTS("[Init: D%d] resol clock = %d Khz\n",
		decon->id, decon->bts.resol_clk);

	pm_qos_add_request(&decon->bts.mif_qos, PM_QOS_BUS_THROUGHPUT, 0);
	pm_qos_add_request(&decon->bts.int_qos, PM_QOS_DEVICE_THROUGHPUT, 0);
	pm_qos_add_request(&decon->bts.disp_qos, PM_QOS_DISPLAY_THROUGHPUT, 0);
	decon->bts.scen_updated = 0;

	for (i = 0; i < BTS_DPP_MAX; ++i) {
		sd = decon->dpp_sd[DPU_DMA2CH(i)];
		v4l2_subdev_call(sd, core, ioctl, DPP_GET_PORT_NUM,
				&decon->bts.bw[i].ch_num);
		DPU_INFO_BTS("IDMA_TYPE(%d) CH(%d) Port(%d)\n", i,
				DPU_DMA2CH(i), decon->bts.bw[i].ch_num);
	}

	decon->bts.enabled = true;

	DPU_INFO_BTS("decon%d bts feature is enabled\n", decon->id);
}

void dpu_bts_deinit(struct decon_device *decon)
{
	if (!decon->bts.enabled)
		return;

	DPU_DEBUG_BTS("%s +\n", __func__);
	pm_qos_remove_request(&decon->bts.disp_qos);
	pm_qos_remove_request(&decon->bts.int_qos);
	pm_qos_remove_request(&decon->bts.mif_qos);
	DPU_DEBUG_BTS("%s -\n", __func__);
}

struct decon_bts_ops decon_bts_control = {
	.bts_init		= dpu_bts_init,
	.bts_calc_bw		= dpu_bts_calc_bw,
	.bts_update_bw		= dpu_bts_update_bw,
	.bts_acquire_bw		= dpu_bts_acquire_bw,
	.bts_release_bw		= dpu_bts_release_bw,
	.bts_deinit		= dpu_bts_deinit,
};
