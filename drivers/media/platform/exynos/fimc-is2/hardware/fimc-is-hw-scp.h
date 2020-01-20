/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_SUBDEV_SCP_H
#define FIMC_IS_SUBDEV_SCP_H

#include "fimc-is-hw-control.h"
#include "fimc-is-interface-library.h"

#define SCALER_INTR_MASK	(0x00011110)
#define MAX_OTF_WIDTH		(3856)
#define USE_DMA_BUFFER_INDEX	(0) /* 0 ~ 7 */

enum scp_img_format {
	/* 2plane, little endian */
	SCP_YC420_2P_CRCB,
	SCP_YC420_2P_CBCR,
	/* 3plane */
	SCP_YC420_3P,

	/* 1plane, data stored order */
	SCP_YC422_1P_YCBYCR,
	SCP_YC422_1P_CBYCRY,
	SCP_YC422_1P_YCRYCB,
	SCP_YC422_1P_CRYCBY,
	/* 2plane, little endian */
	SCP_YC422_2P_CRCB,
	SCP_YC422_2P_CBCR,
	/* 3plane */
	SCP_YC422_3P,
	SCP_IMG_FMT_MAX
};

enum scp_param_change_flag {
	SCP_PARAM_OTFINPUT_CHANGE,
	SCP_PARAM_OTFOUTPUT_CHANGE,
	SCP_PARAM_DMAOUTPUT_CHANGE,
	SCP_PARAM_INPUTCROP_CHANGE,
	SCP_PARAM_OUTPUTCROP_CHANGE,
	SCP_PARAM_FLIP_CHANGE,
	SCP_PARAM_IMAGEEFFECT_CHANGE,
	SCP_PARAM_YUVRANGE_CHANGE,
	SCP_PARAM_CHANGE_MAX,
};

struct fimc_is_hw_scp {
	u32	img_format;
	bool	conv420_en;
	struct hw_api_scaler_setfile *setfile;
};

int fimc_is_hw_scp_probe(struct fimc_is_hw_ip *hw_ip, struct fimc_is_interface *itf,
	struct fimc_is_interface_ischain *itfc, int id, const char *name);
int fimc_is_hw_scp_update_register(u32 instance, struct fimc_is_hw_ip *hw_ip,
	struct scp_param *param);
int fimc_is_hw_scp_reset(struct fimc_is_hw_ip *hw_ip);
int fimc_is_hw_scp_otf_input(u32 instance, struct fimc_is_hw_ip *hw_ip);
int fimc_is_hw_scp_effect(u32 instance, struct fimc_is_hw_ip *hw_ip);
int fimc_is_hw_scp_input_crop(u32 instance, struct fimc_is_hw_ip *hw_ip);
int fimc_is_hw_scp_output_crop(u32 instance, struct fimc_is_hw_ip *hw_ip);
int fimc_is_hw_scp_flip(u32 instance, struct fimc_is_hw_ip *hw_ip);
int fimc_is_hw_scp_otf_output(u32 instance, struct fimc_is_hw_ip *hw_ip);
int fimc_is_hw_scp_dma_output(u32 instance, struct fimc_is_hw_ip *hw_ip);
int fimc_is_hw_scp_output_yuvrange(u32 instance, struct fimc_is_hw_ip *hw_ip);

int fimc_is_hw_scp_adjust_img_fmt(u32 format, u32 plane, u32 order,
	u32 *img_format, bool *conv420_flag);
void fimc_is_hw_scp_adjust_order(u32 plane, u32 img_format, u32 *order2p_out,
	u32 *order422_out);
void fimc_is_hw_scp_adjust_pre_ratio(u32 pre_width, u32 pre_height,
	u32 dst_width, u32 dst_height, u32 *pre_hratio, u32 *pre_vratio, u32 *sh_factor);
void fimc_is_hw_scp_calc_canv_size(u32 width, u32 height, u32 plane, bool conv420_flag,
	u32 *y_h_size, u32 *y_v_size, u32 *c_h_size, u32 *c_v_size);
#endif
