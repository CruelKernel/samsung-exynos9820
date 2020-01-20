/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __EXYNOS_BTS_H_
#define __EXYNOS_BTS_H_

#include <soc/samsung/exynos-pd.h>

#if defined(CONFIG_EXYNOS9820_BTS)
#define BUS_WIDTH		16
#define DISP_UTIL		75

enum bts_scen_type {
	BS_DEFAULT,
	BS_MIF_CHANGE,
	BS_MFC_UHD,
	BS_MFC_UHD_10BIT,
	BS_G3D_PERFORMANCE,
	BS_DP_DEFAULT,
	BS_CAMERA_DEFAULT,
	BS_MFC_UHD_ENC60,
	BS_MAX,
};

enum bts_bw_type {
	/* RT */
	BTS_BW_DECON0,
	BTS_BW_DECON1,
	BTS_BW_DECON2,
	BTS_BW_CAMERA,
	BTS_BW_AUDIO,
	BTS_BW_CP,
	/* non-RT */
	BTS_BW_G2D,
	BTS_BW_MFC,
	BTS_BW_MCSL,
	BTS_BW_MAX,
	BTS_BW_RT = BTS_BW_G2D,
};

enum bts_dpp_type {
	BTS_DPP0,
	BTS_DPP1,
	BTS_DPP2,
	BTS_DPP3,
	BTS_DPP4,
	BTS_DPP5,
	BTS_DPP_MAX,
};

enum bts_dpu_type {
	BTS_DPU0,
	BTS_DPU1,
	BTS_DPU2,
	BTS_DPU_MAX,
};

struct bts_layer_position {
	unsigned int x1;
	/* x2 = x1 + width */
	unsigned int x2;
	unsigned int y1;
	/* y2 = y1 + height */
	unsigned int y2;
};

struct bts_dpp_info {
	bool used;
	unsigned int bpp;
	unsigned int src_h;
	unsigned int src_w;
	struct bts_layer_position dst;
	unsigned int bw;
	bool rotation;
};

struct bts_decon_info {
	struct bts_dpp_info dpp[BTS_DPP_MAX];
	/* Khz */
	unsigned int vclk;
	unsigned int lcd_w;
	unsigned int lcd_h;
};

struct bts_bw {
	unsigned int peak;
	unsigned int read;
	unsigned int write;
};

void bts_update_scen(enum bts_scen_type type, unsigned int val);
void bts_pd_sync(unsigned int id, int on);
/* bandwidth (KB/s) */
void bts_update_bw(enum bts_bw_type type, struct bts_bw bw);
unsigned int bts_calc_bw(enum bts_bw_type type, void *data);

#elif defined(CONFIG_EXYNOS8895_BTS)
#define BUS_WIDTH		16
#define DISP_UTIL		75

enum bts_scen_type {
	BS_DEFAULT,
	BS_MIF_CHANGE,
	BS_MFC_UHD,
	BS_G3D_PEFORMANCE,
	BS_DP_DEFAULT,
	BS_CAMERA_DEFAULT,
	BS_MAX,
};

enum bts_bw_type {
	/* RT */
	BTS_BW_DECON0,
	BTS_BW_DECON1,
	BTS_BW_DECON2,
	BTS_BW_CAMERA,
	BTS_BW_AUDIO,
	BTS_BW_CP,
	/* non-RT */
	BTS_BW_G2D,
	BTS_BW_MFC,
	BTS_BW_MCSL,
#ifndef CONFIG_SOC_EMULATOR8895
	BTS_BW_IVA,
	BTS_BW_DSP,
#endif
	BTS_BW_MAX,
	BTS_BW_RT = BTS_BW_G2D,
};

enum bts_dpp_type {
	BTS_DPP0,
	BTS_DPP1,
	BTS_DPP2,
	BTS_DPP3,
	BTS_DPP4,
	BTS_DPP5,
	BTS_DPP_MAX,
};

enum bts_dpu_type {
	BTS_DPU0,
	BTS_DPU1,
	BTS_DPU2,
	BTS_DPU_MAX,
};

struct bts_layer_position {
	unsigned int x1;
	/* x2 = x1 + width */
	unsigned int x2;
	unsigned int y1;
	/* y2 = y1 + height */
	unsigned int y2;
};

struct bts_dpp_info {
	bool used;
	unsigned int bpp;
	unsigned int src_h;
	unsigned int src_w;
	struct bts_layer_position dst;
	unsigned int bw;
};

struct bts_decon_info {
	struct bts_dpp_info dpp[BTS_DPP_MAX];
	/* Khz */
	unsigned int vclk;
	unsigned int lcd_w;
	unsigned int lcd_h;
};

struct bts_bw {
	unsigned int peak;
	unsigned int read;
	unsigned int write;
};

void bts_update_scen(enum bts_scen_type type, unsigned int val);
/* bandwidth (KB/s) */
void bts_update_bw(enum bts_bw_type type, struct bts_bw bw);
unsigned int bts_calc_bw(enum bts_bw_type type, void *data);

#else
#define bts_update_scen(a, b) do {} while(0)
#define bts_update_bw(a, b) do {} while(0)
#define bts_calc_bw(a, b) do {} while(0)
#define bts_pd_sync(a, b) do {} while(0)
#endif

#if defined(CONFIG_EXYNOS5422_BTS) || defined(CONFIG_EXYNOS5433_BTS)	\
	|| defined(CONFIG_EXYNOS7420_BTS) || defined(CONFIG_EXYNOS7890_BTS) \
	|| defined(CONFIG_EXYNOS8890_BTS)
enum bts_scen_type {
	TYPE_MFC_UD_ENCODING = 0,
	TYPE_MFC_UD_DECODING,
	TYPE_LAYERS,
	TYPE_G3D_FREQ,
	TYPE_G3D_SCENARIO,
	TYPE_ROTATION,
	TYPE_HIGHPERF,
	TYPE_URGENT_OFF,
};

void bts_scen_update(enum bts_scen_type type, unsigned int val);
#else
#define bts_scen_update(a, b) do {} while(0)
#endif

#ifdef CONFIG_EXYNOS8890_BTS_OPTIMIZATION
#define MULTI_FACTOR		(1 << 10)
#define MIF_UTIL		70
#define INT_UTIL		70
#define BUS_WIDTH		16

#define RGB_FACTOR		40
#define RGB16_FACTOR		20
#define YUV_FACTOR		15
#define PIXEL_BUFFER		28000
#define RGBBUF_FACTOR		20
#define RGB16BUF_FACTOR		28
#define YUVBUF_FACTOR		36
#define PEAK_FACTOR		12
#define FPS			60
#define VBI_FACTOR		16000
#define ROT_FACTOR		1

enum bts_media_type {
	TYPE_VPP0 = 0,
	TYPE_VPP1,
	TYPE_VPP2,
	TYPE_VPP3,
	TYPE_VPP4,
	TYPE_VPP5,
	TYPE_VPP6,
	TYPE_VPP7,
	TYPE_VPP8,
	TYPE_CAM,
	TYPE_MFC,
	TYPE_G3D,
};

enum bts_ip_type {
	IP_VPP = 0,
	IP_CAM,
	IP_MFC,
	IP_NUM,
};

struct bts_hw {
	enum bts_media_type *ip_type;
};

struct bts_vpp_info {
	struct bts_hw	hw;
	unsigned int	src_h;
	unsigned int	src_w;
	unsigned int	dst_h;
	unsigned int	dst_w;
	unsigned int	bpp;
	bool		is_rotation;
	unsigned int	mic;
	unsigned int	pix_per_clk;
	unsigned int	vclk;

	u64		cur_bw;
	u64		peak_bw;
	u64		shw_cur_bw;
	u64		shw_peak_bw;
};

#define to_bts_vpp(_hw)	container_of(_hw, struct bts_vpp_info, hw)

#if defined(CONFIG_EXYNOS8890_BTS)
void exynos_update_bw(enum bts_media_type ip_type,
		unsigned int sum_bw, unsigned int peak_bw);
void bts_ext_scenario_set(enum bts_media_type ip_type,
				enum bts_scen_type scen_type, bool on);
struct bts_vpp_info *exynos_bw_calc(enum bts_media_type ip_type, struct bts_hw *bw);
int bts_update_gpu_mif(unsigned int freq);
int bts_update_winlayer(unsigned int layers);
#else
#define exynos_update_bw(a, b, c) do {} while (0)
#define bts_ext_scenario_set(a, b, c) do {} while (0)
#define exynos_bw_calc(a, b) do {} while (0)
#define bts_update_gpu_mif(a) do {} while (0)
#define bts_update_winlayer(a) do {} while (0)
#endif

#else /* BTS_OPTIMIZATION */

#if defined(CONFIG_EXYNOS8890_BTS)
#define CAM_FACTOR		3
#define MIF_UTIL		50
#define MIF_UTIL2		55
#define MIF_DECODING		828000	// TBD
#define MIF_ENCODING		1068000	// TBD
#define INT_UTIL		70
#define BUS_WIDTH		16
#define SIZE_FACTOR(a)		a = ((a) * 16 / 10)
#define FULLHD_SRC		1920 * 1080
#elif defined(CONFIG_EXYNOS7420_BTS) || defined(CONFIG_EXYNOS7890_BTS)
#define DECON_NOCNT		10
#define VPP_ROT			4
#define CAM_FACTOR		3
#define MIF_UTIL		50
#define MIF_UTIL2		60
#define MIF_UPPER_UTIL		44
#define MIF_DECODING		828000
#define MIF_ENCODING		1068000
#define INT_UTIL		65
#define NO_CNT_TH		100
#define BUS_WIDTH		16
#define SIZE_FACTOR(a)		a = ((a) * 16 / 10)
#define FULLHD_SRC		1920 * 1080
#else
#define FULLHD_SRC		1920 * 1080
#endif

enum bts_media_type {
	TYPE_DECON_INT,
	TYPE_DECON_EXT,
	TYPE_VPP0,
	TYPE_VPP1,
	TYPE_VPP2,
	TYPE_VPP3,
	TYPE_VPP4,
	TYPE_VPP5,
	TYPE_VPP6,
	TYPE_VPP7,
	TYPE_VPP8,
	TYPE_CAM,
	TYPE_YUV,
	TYPE_UD_ENC,
	TYPE_UD_DEC,
	TYPE_SPDMA,
};

enum vpp_bw_type {
	BW_DEFAULT,
	BW_ROT,
	BW_FULLHD_ROT,
};

#if defined(CONFIG_EXYNOS8890_BTS)
void exynos_update_media_scenario(enum bts_media_type media_type,
		unsigned int bw, int bw_type);
int bts_update_gpu_mif(unsigned int freq);
#else
#define exynos_update_media_scenario(a, b, c) do {} while (0)
#define bts_update_gpu_mif(a) do {} while (0)
#endif

#endif /* BTS_OPTIMIZATION */

#if defined(CONFIG_EXYNOS8890_BTS)
void exynos_bts_scitoken_setting(bool on);
#else
#define exynos_bts_scitoken_setting(a) do {} while (0)
#endif

#if defined(CONFIG_EXYNOS7420_BTS) || defined(CONFIG_EXYNOS7890_BTS)
void exynos7_update_media_scenario(enum bts_media_type media_type,
		unsigned int bw, int bw_type);
int exynos7_update_bts_param(int target_idx, int work);
int exynos7_bts_register_notifier(struct notifier_block *nb);
int exynos7_bts_unregister_notifier(struct notifier_block *nb);
#else
#define exynos7_update_media_scenario(a, b, c) do {} while (0)
#define exynos7_update_bts_param(a, b) do {} while (0)
#define exynos7_bts_register_notifier(a) do {} while (0)
#define exynos7_bts_unregister_notifier(a) do {} while (0)
#endif

#if defined(CONFIG_EXYNOS5430_BTS) || defined(CONFIG_EXYNOS5422_BTS)	\
	|| defined(CONFIG_EXYNOS5433_BTS)|| defined(CONFIG_EXYNOS7420_BTS) \
	|| defined(CONFIG_EXYNOS7890_BTS) || defined(CONFIG_EXYNOS8890_BTS)
void bts_initialize(const char *pd_name, bool on);
#else
#define bts_initialize(a, b) do {} while (0)
#endif

#if defined(CONFIG_EXYNOS7420_BTS) || defined(CONFIG_EXYNOS7890_BTS)
void exynos7_bts_show_mo_status(void);
#else
#define exynos7_bts_show_mo_status() do {} while (0)
#endif

#if defined(CONFIG_EXYNOS5430_BTS)
void exynos5_bts_show_mo_status(void);
#else
#define exynos5_bts_show_mo_status() do { } while (0)
#endif

#if defined(CONFIG_EXYNOS5430_BTS) || defined(CONFIG_EXYNOS5433_BTS)
void bts_otf_initialize(unsigned int id, bool on);
#else
#define bts_otf_initialize(a, b) do {} while (0)
#endif

#endif
