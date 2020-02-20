/* drivers/video/exynos_decon/lcd.h
 *
 * Copyright (c) 2011 Samsung Electronics
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __DECON_LCD__
#define __DECON_LCD__

enum decon_psr_mode {
	DECON_VIDEO_MODE = 0,
	DECON_DP_PSR_MODE = 1,
	DECON_MIPI_COMMAND_MODE = 2,
};

/* Mic ratio: 0: 1/2 ratio, 1: 1/3 ratio */
enum decon_mic_comp_ratio {
	MIC_COMP_RATIO_1_2 = 0,
	MIC_COMP_RATIO_1_3 = 1,
	MIC_COMP_BYPASS
};

enum mic_ver {
	MIC_VER_1_1,
	MIC_VER_1_2,
	MIC_VER_2_0,
};

enum type_of_ddi {
	TYPE_OF_SM_DDI = 0,
	TYPE_OF_MAGNA_DDI,
	TYPE_OF_NORMAL_DDI,
};

enum {
	DSU_MODE_NONE = 0,
	DSU_MODE_1,
	DSU_MODE_2,
	DSU_MODE_3,
	DSU_MODE_MAX,
};

#define MAX_RES_NUMBER		5
#define HDR_CAPA_NUM		4

#define MAX_COLOR_MODE		5

struct lcd_res_info {
	unsigned int width;
	unsigned int height;
	unsigned int dsc_en;
	unsigned int dsc_width;
	unsigned int dsc_height;
};

/* multi-resolution */
struct lcd_mres_info {
	unsigned int mres_en;
	unsigned int mres_number;
	struct lcd_res_info res_info[MAX_RES_NUMBER];
};

struct lcd_hdr_info {
	unsigned int hdr_num;
	unsigned int hdr_type[HDR_CAPA_NUM];
	unsigned int hdr_max_luma;
	unsigned int hdr_max_avg_luma;
	unsigned int hdr_min_luma;
};

/*
 * dec_sw : slice width in DDI side
 * enc_sw : slice width in AP(DECON & DSIM) side
 */
struct dsc_slice {
	unsigned int dsc_dec_sw[MAX_RES_NUMBER];
	unsigned int dsc_enc_sw[MAX_RES_NUMBER];
};

struct stdphy_pms {
	unsigned int p;
	unsigned int m;
	unsigned int s;
	unsigned int k;
#if defined(CONFIG_EXYNOS_DSIM_DITHER)
	unsigned int mfr;
	unsigned int mrr;
	unsigned int sel_pf;
	unsigned int icp;
	unsigned int afc_enb;
	unsigned int extafc;
	unsigned int feed_en;
	unsigned int fsel;
	unsigned int fout_mask;
	unsigned int rsel;
#endif
};

#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
#define MAX_ADAPTABLE_FREQ	5
#if defined(CONFIG_EXYNOS_DSIM_DITHER)
#define MAX_PMSK_CNT	14
#else
#define MAX_PMSK_CNT	4
#endif

struct adaptive_freq_info {
	unsigned int hs_clk;
	struct stdphy_pms dphy_pms;
	unsigned int esc_clk;
	unsigned int cmd_underrun_lp_ref[MAX_RES_NUMBER];
};

struct adaptive_idx {
	int req_freq_idx;	/* requested frequency */
	int cur_freq_idx;	/* dsim current frequency */
};

struct adaptive_info {
	int freq_cnt;
	struct adaptive_freq_info freq_info[MAX_ADAPTABLE_FREQ];
	struct adaptive_idx *adap_idx;
};
#endif



#ifdef CONFIG_DYNAMIC_FREQ

#define MAX_DYNAMIC_FREQ	5

struct df_status_info {
	bool enabled;

	u32 request_df;
	u32 target_df;
	u32 current_df;
	u32 ffc_df;
	u32 context;
};


struct df_setting_info {
	unsigned int hs;
	struct stdphy_pms dphy_pms;
	unsigned int cmd_underrun_lp_ref[MAX_RES_NUMBER];
};

struct df_dt_info {
	int dft_index;
	int df_cnt;
	struct df_setting_info setting_info[MAX_DYNAMIC_FREQ];
};

struct df_param {
	struct stdphy_pms pms;
	unsigned int cmd_underrun_lp_ref[MAX_RES_NUMBER];
	u32 context;
};

#endif

struct decon_lcd {
	enum decon_psr_mode mode;
	unsigned int vfp;
	unsigned int vbp;
	unsigned int hfp;
	unsigned int hbp;

	unsigned int vsa;
	unsigned int hsa;

	unsigned int xres;
	unsigned int yres;

	unsigned int width;
	unsigned int height;

	unsigned int hs_clk;
	struct stdphy_pms dphy_pms;
	unsigned int esc_clk;

	unsigned int fps;
	unsigned int mic_enabled;
	enum decon_mic_comp_ratio mic_ratio;
	unsigned int dsc_enabled;
	unsigned int dsc_cnt;
	unsigned int dsc_slice_num;
	unsigned int dsc_slice_h;
	unsigned int dsc_dec_sw;
	unsigned int dsc_enc_sw;
	enum mic_ver mic_ver;
	enum type_of_ddi ddi_type;
	unsigned int data_lane;
	unsigned int cmd_underrun_lp_ref[MAX_RES_NUMBER];
	unsigned int vt_compensation;
	unsigned int mres_mode;
	struct lcd_mres_info dt_lcd_mres;
	struct lcd_hdr_info dt_lcd_hdr;
	struct dsc_slice dt_dsc_slice;
	unsigned int bpc;
	unsigned int partial_width[MAX_RES_NUMBER];
	unsigned int partial_height[MAX_RES_NUMBER];
	unsigned int color_mode_cnt;
	unsigned int color_mode[MAX_COLOR_MODE];
#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
	struct adaptive_info adaptive_info;
#endif

#ifdef CONFIG_DYNAMIC_FREQ
	struct df_dt_info df_set_info;
#endif

};

struct decon_dsc {
/* 04 */	unsigned int comp_cfg;
/* 05 */	unsigned int bit_per_pixel;
/* 06-07 */	unsigned int pic_height;
/* 08-09 */	unsigned int pic_width;
/* 10-11 */	unsigned int slice_height;
/* 12-13 */	unsigned int slice_width;
/* 14-15 */	unsigned int chunk_size;
/* 16-17 */	unsigned int initial_xmit_delay;
/* 18-19 */	unsigned int initial_dec_delay;
/* 21 */	unsigned int initial_scale_value;
/* 22-23 */	unsigned int scale_increment_interval;
/* 24-25 */	unsigned int scale_decrement_interval;
/* 27 */	unsigned int first_line_bpg_offset;
/* 28-29 */	unsigned int nfl_bpg_offset;
/* 30-31 */	unsigned int slice_bpg_offset;
/* 32-33 */	unsigned int initial_offset;
/* 34-35 */	unsigned int final_offset;
/* 58-59 */	unsigned int rc_range_parameters;

		unsigned int overlap_w;
		unsigned int width_per_enc;
		unsigned char *dec_pps_t;
};
#endif
