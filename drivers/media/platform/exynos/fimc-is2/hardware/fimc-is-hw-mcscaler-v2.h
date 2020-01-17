/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_MCSC_H
#define FIMC_IS_HW_MCSC_H

#include "fimc-is-hw-control.h"
#include "fimc-is-hw-tdnr.h"
#include "fimc-is-hw-djag.h"
#include "fimc-is-hw-uvsp-cac.h"
#include "fimc-is-interface-library.h"
#include "fimc-is-param.h"

#define MCSC_ROUND_UP(x, d) \
	((d) * (((x) + ((d) - 1)) / (d)))

#define GET_MCSC_HW_CAP(hwip) \
	((hwip->priv_info) ? &((struct fimc_is_hw_mcsc *)hw_ip->priv_info)->cap : NULL)
#define GET_ENTRY_FROM_OUTPUT_ID(output_id) \
	(output_id + ENTRY_M0P)
#define GET_DJAG_ZOOM_RATIO(in, out) (u32)(((in * 1000 / out) << MCSC_PRECISION) / 1000)

/*
 * [0:3]: DJAG
 * [4:7]: CAC
 * [8:11]: UVSP
 * [12:31]: Reserved
 */
#define SUBBLK_IP_DJAG			(0) /* [0:3] */
#define SUBBLK_IP_CAC			(4) /* [4:7] */
#define SUBBLK_IP_UVSP			(8) /* [8:11] */

#define SUBBLK_CTRL_SYSFS		(0)
#define SUBBLK_CTRL_EN			(1)
#define SUBBLK_CTRL_INPUT		(3)

#define GET_SUBBLK_CTRL_BIT(value, IP_OFFSET, CTRL_OFFSET)	\
	(((value) >> ((IP_OFFSET) + (CTRL_OFFSET))) & 0x1)

#define GET_CORE_NUM(id)	((id) - DEV_HW_MCSC0)

enum mcsc_img_format {
	MCSC_YUV422_1P_YUYV = 0,
	MCSC_YUV422_1P_YVYU,
	MCSC_YUV422_1P_UYVY,
	MCSC_YUV422_1P_VYUY,
	MCSC_YUV422_2P_UFIRST,
	MCSC_YUV422_2P_VFIRST,
	MCSC_YUV422_3P,
	MCSC_YUV420_2P_UFIRST,
	MCSC_YUV420_2P_VFIRST,
	MCSC_YUV420_3P,
	MCSC_RGB_ARGB8888,
	MCSC_RGB_BGRA8888,
	MCSC_RGB_RGBA8888,
	MCSC_RGB_ABGR8888,
	MCSC_MONO_Y8,
};

enum mcsc_io_type {
	HW_MCSC_OTF_INPUT,
	HW_MCSC_OTF_OUTPUT,
	HW_MCSC_DMA_INPUT,
	HW_MCSC_DMA_OUTPUT,
};

enum mcsc_cap_enum {
	MCSC_CAP_NOT_SUPPORT = 0,
	MCSC_CAP_SUPPORT,
};

enum mcsc_shadow_ctrl {
	SHADOW_WRITE_START = 0,
	SHADOW_WRITE_FINISH,
};

enum yic_mode {
	TDNR_YIC_ENABLE = 0,
	TDNR_YIC_DISABLE,
};

struct scaler_setfile_contents {
	/* Brightness/Contrast control param */
	u32 y_offset;
	u32 y_gain;

	/* Hue/Saturation control param */
	u32 c_gain00;
	u32 c_gain01;
	u32 c_gain10;
	u32 c_gain11;
};

struct scaler_coef_cfg {
	u32 ratio_x8_8;		/* x8/8, Ratio <= 1048576 (~8:8) */
	u32 ratio_x7_8;		/* x7/8, 1048576 < Ratio <= 1198373 (~7/8) */
	u32 ratio_x6_8;		/* x6/8, 1198373 < Ratio <= 1398101 (~6/8) */
	u32 ratio_x5_8;		/* x5/8, 1398101 < Ratio <= 1677722 (~5/8) */
	u32 ratio_x4_8;		/* x4/8, 1677722 < Ratio <= 2097152 (~4/8) */
	u32 ratio_x3_8;		/* x3/8, 2097152 < Ratio <= 2796203 (~3/8) */
	u32 ratio_x2_8;		/* x2/8, 2796203 < Ratio <= 4194304 (~2/8) */
};

struct scaler_bchs_clamp_cfg {
	u32 y_max;
	u32 y_min;
	u32 c_max;
	u32 c_min;
};

struct hw_mcsc_setfile {
	u32 setfile_version;

	/* contents for Full/Narrow mode
	 * 0 : SCALER_OUTPUT_YUV_RANGE_FULL
	 * 1 : SCALER_OUTPUT_YUV_RANGE_NARROW
	 */
	struct scaler_setfile_contents	sc_base[2];
	struct tdnr_setfile_contents	tdnr_contents;
	/* Setfile tuning parameters for DJAG (Lhotse)
	 * 0 : Scaling ratio = x1.0
	 * 1 : Scaling ratio = x1.1~x1.4
	 * 2 : Scaling ratio = x1.5~x2.0
	 * 3 : Scaling ratio = x2.1~
	 */
	struct djag_setfile_contents	djag[MAX_SCALINGRATIOINDEX_DEPENDED_CONFIGS];
	struct scaler_bchs_clamp_cfg	sc_bchs[2];	/* 0: YUV_FULL, 1: YUV_NARROW */
	struct scaler_coef_cfg		sc_coef;
	struct djag_wb_thres_cfg	djag_wb[MAX_SCALINGRATIOINDEX_DEPENDED_CONFIGS];
	struct cac_setfile_contents	cac;
	struct uvsp_setfile_contents	uvsp;
};

/**
 * struct fimc_is_fw_mcsc_cap - capability of mcsc
 *  This Structure specified the spec of mcsc.
 * @hw_ver: type is hexa. eg. 1.22.0 -> 0b0001_0022_0000_0000
 * @max_output: the number of output port to support
 * <below fields has the value in enum mcsc_cap_enum>
 * @in_otf: capability of input otf
 * @in_dma: capability of input dma
 * @hw_fc: capability of hardware flow control
 * @out_otf: capability of output otf
 * @out_dma: capability of output dma
 * @out_hwfc: capability of output dma (each output)
 * @tdnr: capability of 3DNR feature
 */
struct fimc_is_hw_mcsc_cap {
	u32			hw_ver;
	u32			max_output;
	u32			max_djag;
	u32			max_cac;
	u32			max_uvsp;
	enum mcsc_cap_enum	in_otf;
	enum mcsc_cap_enum	in_dma;
	enum mcsc_cap_enum	hwfc;
	enum mcsc_cap_enum	out_otf[MCSC_OUTPUT_MAX];
	enum mcsc_cap_enum	out_dma[MCSC_OUTPUT_MAX];
	enum mcsc_cap_enum	out_hwfc[MCSC_OUTPUT_MAX];
	bool 			enable_shared_output;
	enum mcsc_cap_enum	tdnr;
	enum mcsc_cap_enum	djag;
	enum mcsc_cap_enum	cac;
	enum mcsc_cap_enum	uvsp;
	enum mcsc_cap_enum	ysum;
	enum mcsc_cap_enum	ds_vra;
};

#define SUBBLK_TDNR	(0)
#define SUBBLK_CAC	(1)
#define SUBBLK_UVSP	(2)
#define SUBBLK_MAX	(3)
struct fimc_is_hw_mcsc {
	struct	hw_mcsc_setfile setfile[SENSOR_POSITION_MAX][FIMC_IS_MAX_SETFILE];
	struct	hw_mcsc_setfile *cur_setfile[SENSOR_POSITION_MAX][FIMC_IS_STREAM_COUNT];
	struct	fimc_is_hw_mcsc_cap cap;

	u32	in_img_format;
	u32	out_img_format[MCSC_OUTPUT_MAX];
	bool	conv420_en[MCSC_OUTPUT_MAX];
	bool	rep_flag[FIMC_IS_STREAM_COUNT];
	int	yuv_range;
	u32	instance;
	ulong	out_en;		/* This flag save whether the capture video node of MCSC is opened or not. */
	u32	prev_hwfc_output_ids;
	/* noise_index also needs to use in TDNR, CAC, DJAG */
	u32			cur_ni[SUBBLK_MAX];

	/* for tdnr use */
	enum mcsc_output_index	tdnr_output;
	bool			tdnr_first;
	bool			tdnr_internal_buf;
	dma_addr_t		dvaddr_tdnr[2];
	enum tdnr_mode		cur_tdnr_mode;
	enum yic_mode		yic_en;
	struct tdnr_configs	tdnr_cfgs;
	/* for uvsp */
	struct uvsp_ctrl	uvsp_ctrl;

	/* for CAC*/
	u32			cac_in;

	/* for Djag */
	u32			djag_in;

	/* for full otf overflow recovery */
	struct is_param_region	*back_param;
};

int fimc_is_hw_mcsc_probe(struct fimc_is_hw_ip *hw_ip, struct fimc_is_interface *itf,
	struct fimc_is_interface_ischain *itfc, int id, const char *name);
void fimc_is_hw_mcsc_get_force_block_control(struct fimc_is_hw_ip *hw_ip, u32 ip_offset, u32 num_of_block,
	u32 *input_sel, bool *en);

int fimc_is_hw_mcsc_update_param(struct fimc_is_hw_ip *hw_ip,
	struct mcs_param *param, u32 instance);
void fimc_is_hw_mcsc_frame_done(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	int done_type);
int fimc_is_hw_mcsc_reset(struct fimc_is_hw_ip *hw_ip);
int fimc_is_hw_mcsc_clear_interrupt(struct fimc_is_hw_ip *hw_ip);

int fimc_is_hw_mcsc_otf_input(struct fimc_is_hw_ip *hw_ip, struct param_mcs_input *input,
	u32 instance);
int fimc_is_hw_mcsc_dma_input(struct fimc_is_hw_ip *hw_ip, struct param_mcs_input *input,
	u32 instance);
int fimc_is_hw_mcsc_poly_phase(struct fimc_is_hw_ip *hw_ip, struct param_mcs_input *input,
	struct param_mcs_output *output, u32 output_id, u32 instance);
int fimc_is_hw_mcsc_post_chain(struct fimc_is_hw_ip *hw_ip, struct param_mcs_input *input,
	struct param_mcs_output *output, u32 output_id, u32 instance);
int fimc_is_hw_mcsc_flip(struct fimc_is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance);
int fimc_is_hw_mcsc_otf_output(struct fimc_is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance);
int fimc_is_hw_mcsc_dma_output(struct fimc_is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance);
int fimc_is_hw_mcsc_output_yuvrange(struct fimc_is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance);
int fimc_is_hw_mcsc_hwfc_mode(struct fimc_is_hw_ip *hw_ip, struct param_mcs_input *input,
	u32 hwfc_output_ids, u32 dma_output_ids, u32 instance);
int fimc_is_hw_mcsc_hwfc_output(struct fimc_is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance);

int fimc_is_hw_mcsc_adjust_input_img_fmt(u32 format, u32 plane, u32 order, u32 *img_format);
int fimc_is_hw_mcsc_adjust_output_img_fmt(u32 format, u32 plane, u32 order, u32 *img_format,
	bool *conv420_flag);
int fimc_is_hw_mcsc_check_format(enum mcsc_io_type type, u32 format, u32 bit_width,
	u32 width, u32 height);
u32 fimc_is_scaler_get_idle_status(void __iomem *base_addr, u32 hw_id);

void fimc_is_hw_mcsc_tdnr_init(struct fimc_is_hw_ip *hw_ip,
	struct mcs_param *mcs_param, u32 instance);
void fimc_is_hw_mcsc_tdnr_deinit(struct fimc_is_hw_ip *hw_ip,
	struct mcs_param *mcs_param, u32 instance);
int fimc_is_hw_mcsc_update_tdnr_register(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_frame *frame,
	struct is_param_region *param,
	bool start_flag);
int fimc_is_hw_mcsc_recovery_tdnr_register(struct fimc_is_hw_ip *hw_ip,
		struct is_param_region *param, u32 instance);

void fimc_is_hw_mcsc_set_size_for_uvsp(struct fimc_is_hardware *hardware,
	struct fimc_is_frame *frame, ulong hw_map);
void fimc_is_hw_mcsc_set_ni(struct fimc_is_hardware *hardware, struct fimc_is_frame *frame,
	u32 instance);
void fimc_is_hw_mcsc_adjust_size_with_djag(struct fimc_is_hw_ip *hw_ip, struct param_mcs_input *input,
	struct fimc_is_hw_mcsc_cap *cap, u32 *x, u32 *y, u32 *width, u32 *height);
int fimc_is_hw_mcsc_update_djag_register(struct fimc_is_hw_ip *hw_ip,
		struct mcs_param *param,
		u32 instance);
int fimc_is_hw_mcsc_update_cac_register(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_frame *frame, u32 instance);
int fimc_is_hw_mcsc_update_uvsp_register(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_frame *frame, u32 instance);
int fimc_is_hw_mcsc_update_ysum_register(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_group *head, struct mcs_param *mcs_param,
	u32 instance, struct camera2_shot *shot);

int fimc_is_hw_mcsc_update_dsvra_register(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_group *head, struct mcs_param *mcs_param,
	u32 instance, struct camera2_shot *shot);

#ifdef DEBUG_HW_SIZE
#define hw_mcsc_check_size(hw_ip, param, instance, output_id) \
	fimc_is_hw_mcsc_check_size(hw_ip, param, instance, output_id)
#else
#define hw_mcsc_check_size(hw_ip, param, instance, output_id)
#endif
#endif
