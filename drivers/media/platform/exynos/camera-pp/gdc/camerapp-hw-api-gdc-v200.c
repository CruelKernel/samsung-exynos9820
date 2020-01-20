/*
 * Samsung EXYNOS CAMERA PostProcessing GDC driver
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "camerapp-hw-api-gdc.h"
#include "camerapp-hw-reg-gdc-v200.h"


void camerapp_hw_gdc_start(void __iomem *base_addr)
{
	/* oneshot */
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_FRAMESTART_CMD], &gdc_fields[GDC_F_FRAMESTART_CMD], 1);
}

void camerapp_hw_gdc_stop(void __iomem *base_addr)
{
	/* camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_FRAMESTART_CMD], &gdc_fields[GDC_F_FRAMESTART_CMD], 0); */
}

u32 camerapp_hw_gdc_sw_reset(void __iomem *base_addr)
{
	u32 reset_count = 0;

	/* request to stop bus IF */
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_REQ_STOP], &gdc_fields[GDC_F_EN_RESET], 1);

	/* wait bus IF reset complete */
	do {
		reset_count++;
		if (reset_count > 10000)
			return reset_count;
	} while (camerapp_sfr_get_field(base_addr, &gdc_regs[GDC_R_REQ_STOP], &gdc_fields[GDC_F_RESET_OK]) != 1);

	/* request to gdc hw */
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_SW_RESET], &gdc_fields[GDC_F_SW_RESET], 1);

	/* wait reset complete */
	do {
		reset_count++;
		if (reset_count > 10000)
			return reset_count;
	} while (camerapp_sfr_get_field(base_addr, &gdc_regs[GDC_R_GDC_SW_RESET], &gdc_fields[GDC_F_SW_RESET]) != 0);

	return 0;
}

void camerapp_hw_gdc_clear_intr_all(void __iomem *base_addr)
{
	camerapp_sfr_set_reg(base_addr, &gdc_regs[GDC_R_GDC_INT_STATUS], 0x0001);
}

void camerapp_hw_gdc_mask_intr(void __iomem *base_addr, u32 intr_mask)
{
	camerapp_sfr_set_reg(base_addr, &gdc_regs[GDC_R_GDC_INT_MASK], intr_mask);
}

u32 camerapp_hw_gdc_get_intr_status(void __iomem *base_addr)
{
	u32 intStatus;

	intStatus = camerapp_sfr_get_reg(base_addr, &gdc_regs[GDC_R_GDC_INT_STATUS]);

	return intStatus;
}

u32 camerapp_hw_gdc_get_intr_status_and_clear(void __iomem *base_addr)
{
	u32 intStatus;

	intStatus = camerapp_sfr_get_reg(base_addr, &gdc_regs[GDC_R_GDC_INT_STATUS]);
	camerapp_sfr_set_reg(base_addr, &gdc_regs[GDC_R_GDC_INT_STATUS], intStatus);

	return intStatus;
}

void camerapp_hw_gdc_update_grid_param(void __iomem *base_addr, struct gdc_grid_param *grid_param)
{
	u32 i = 0, j = 0;
	u32 sfr_offset = 0x0004;
	u32 sfr_start_x = 0x0100;
	u32 sfr_start_y = 0x0200;

	if (grid_param->is_valid == true) {
		for (i = 0; i < 7; i++) {
			for (j = 0; j < 9; j++) {
                u32 cal_sfr_offset = (sfr_offset * i * 9) + (sfr_offset * j);
				writel((u32)grid_param->dx[i][j], base_addr + sfr_start_x + cal_sfr_offset);
				writel((u32)grid_param->dy[i][j], base_addr + sfr_start_y + cal_sfr_offset);
			}
		}
	}
}

void camerapp_hw_gdc_update_scale_parameters(void __iomem *base_addr, struct gdc_frame *s_frame,
						struct gdc_frame *d_frame, struct gdc_crop_param *crop_param)
{
	u32 gdc_input_width;
	u32 gdc_input_height;
	u32 gdc_crop_width;
	u32 gdc_crop_height;
	u32 gdc_input_width_start;
	u32 gdc_input_height_start;
	u32 gdc_scale_shifter_x;
	u32 gdc_scale_shifter_y;
	u32 gdc_scale_x;
	u32 gdc_scale_y;
	u32 gdc_inv_scale_x;
	u32 gdc_inv_scale_y;
	u32 gdc_output_width;
	u32 gdc_output_height;
	u32 gdc_output_width_start;
	u32 gdc_output_height_start;
	u32 scaleShifterX;
	u32 scaleShifterY;
	u32 imagewidth;
	u32 imageheight;
	u32 out_scaled_width;
	u32 out_scaled_height;

	/* grid size setting : assume no crop */
	gdc_input_width = s_frame->width;
	gdc_input_height = s_frame->height;
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_CONFIG], &gdc_fields[GDC_F_GDC_MIRROR_X], 0);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_CONFIG], &gdc_fields[GDC_F_GDC_MIRROR_Y], 0);

	/* Need to change : Crop_size */
	gdc_crop_width = gdc_input_width;
	gdc_crop_height = gdc_input_height;


	gdc_inv_scale_x = gdc_input_width;
	gdc_inv_scale_y = ((gdc_input_height << 3) + 3) / 6;
	gdc_input_width_start = 0;
	gdc_input_height_start = 0;

	scaleShifterX  = DS_SHIFTER_MAX;
	imagewidth = gdc_input_width << 1;
	while ((imagewidth <= MAX_VIRTUAL_GRID_X) && (scaleShifterX > 0)) {
		imagewidth <<= 1;
		scaleShifterX--;
	}
	gdc_scale_shifter_x = scaleShifterX;

	scaleShifterY  = DS_SHIFTER_MAX;
	imageheight = gdc_input_height << 1;
	while ((imageheight <= MAX_VIRTUAL_GRID_Y) && (scaleShifterY > 0)) {
		imageheight <<= 1;
		scaleShifterY--;
	}
	gdc_scale_shifter_y = scaleShifterY;

	gdc_scale_x = MIN(65535,
		((MAX_VIRTUAL_GRID_X << (DS_FRACTION_BITS + scaleShifterX)) + gdc_input_width / 2) / gdc_input_width);
	gdc_scale_y = MIN(65535,
		((MAX_VIRTUAL_GRID_Y << (DS_FRACTION_BITS + scaleShifterY)) + gdc_input_height / 2) / gdc_input_height);

	gdc_output_width = d_frame->width;
	gdc_output_height = d_frame->height;
	if ((gdc_output_width & 0x1) || (gdc_output_height & 0x1)) {
		gdc_dbg("gdc output size is not even.\n");
		gdc_output_width = ALIGN(gdc_output_width, 2);
		gdc_output_height = ALIGN(gdc_output_height, 2);
	}

	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_INPUT_ORG_SIZE],
		&gdc_fields[GDC_F_GDC_ORG_HEIGHT], gdc_input_height);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_INPUT_ORG_SIZE],
		&gdc_fields[GDC_F_GDC_ORG_WIDTH], gdc_input_width);

	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_INPUT_CROP_START],
		&gdc_fields[GDC_F_GDC_CROP_START_X], gdc_input_width_start);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_INPUT_CROP_START],
		&gdc_fields[GDC_F_GDC_CROP_START_Y], gdc_input_height_start);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_INPUT_CROP_SIZE],
		&gdc_fields[GDC_F_GDC_CROP_WIDTH], gdc_crop_width);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_INPUT_CROP_SIZE],
		&gdc_fields[GDC_F_GDC_CROP_HEIGHT], gdc_crop_height);

	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_SCALE], &gdc_fields[GDC_F_GDC_SCALE_X], gdc_scale_x);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_SCALE], &gdc_fields[GDC_F_GDC_SCALE_Y], gdc_scale_y);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_SCALE_SHIFTER],
		&gdc_fields[GDC_F_GDC_SCALE_SHIFTER_X], gdc_scale_shifter_x);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_SCALE_SHIFTER],
		&gdc_fields[GDC_F_GDC_SCALE_SHIFTER_Y], gdc_scale_shifter_y);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_INV_SCALE],
		&gdc_fields[GDC_F_GDC_INV_SCALE_X], gdc_inv_scale_x);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_INV_SCALE],
		&gdc_fields[GDC_F_GDC_INV_SCALE_Y], gdc_inv_scale_y);

	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_OUT_CROP_SIZE],
		&gdc_fields[GDC_F_GDC_IMAGE_ACTIVE_WIDTH], gdc_output_width);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_OUT_CROP_SIZE],
		&gdc_fields[GDC_F_GDC_IMAGE_ACTIVE_HEIGHT], gdc_output_height);
	/* Meaningful only when out_crop_bypass = 0, x <= (gdc_crop_width - gdc_image_active_width) */
	gdc_output_width_start = ALIGN((gdc_crop_width - gdc_output_width) >> 1, 2);
	gdc_output_height_start = ALIGN((gdc_crop_height - gdc_output_height) >> 1, 2);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_OUT_CROP_START],
		&gdc_fields[GDC_F_GDC_IMAGE_CROP_PRE_X], gdc_output_width_start);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_OUT_CROP_START],
		&gdc_fields[GDC_F_GDC_IMAGE_CROP_PRE_Y], gdc_output_height_start);

	/* if GDC is scaled up : 128(default) = no scaling, 64 = 2 times scaling */
	/* now is selected no scaling. => calcuration (128 * in / out) */
	if (gdc_crop_width < gdc_output_width)	/* only for scale up */
		out_scaled_width = 128 * gdc_input_width / gdc_output_width;
	else									/* default value */
		out_scaled_width = 128;

	if (gdc_crop_height < gdc_output_height)
		out_scaled_height = 128 * gdc_input_height / gdc_output_height;
	else
		out_scaled_height = 128;

	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_OUT_SCALE],
		&gdc_fields[GDC_F_GDC_OUT_SCALE_Y], out_scaled_height);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_OUT_SCALE],
		&gdc_fields[GDC_F_GDC_OUT_SCALE_X], out_scaled_width);

	gdc_dbg("gdc in(%dx%d) crop(%dx%d) -> out(%dx%d)\n",
		gdc_input_width, gdc_input_height, gdc_crop_width, gdc_crop_height,
		gdc_output_width, gdc_output_height);
}

void camerapp_hw_gdc_update_dma_size(void __iomem *base_addr, struct gdc_frame *s_frame, struct gdc_frame *d_frame)
{
	u32 input_stride_lum_w, input_stride_chroma_w;
	u32 input_stride_lum_w_2bit, input_stride_chroma_w_2bit;
	u32 output_stride_lum_w, output_stride_chroma_w;
	u32 output_stride_lum_w_2bit, output_stride_chroma_w_2bit;
	u32 in_dma_width, out_dma_width;
/*
 * supported format
 * - YUV422_2P_8bit/10bit
 * - YUV420_2P_8bit/10bit
 * - input 10bit : P010(P210), packed 10bit, 8+2bit
 * - output 10bit : P010(P210), 8+2bit
 */
	out_dma_width = camerapp_sfr_get_field(base_addr, &gdc_regs[GDC_R_GDC_OUT_CROP_SIZE],
		&gdc_fields[GDC_F_GDC_IMAGE_ACTIVE_WIDTH]);
	in_dma_width = camerapp_sfr_get_field(base_addr, &gdc_regs[GDC_R_GDC_INPUT_ORG_SIZE],
		&gdc_fields[GDC_F_GDC_ORG_WIDTH]);

	input_stride_lum_w_2bit = 0;
	input_stride_chroma_w_2bit = 0;
	output_stride_lum_w_2bit = 0;
	output_stride_chroma_w_2bit = 0;

	if ((s_frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV12M_P010)
			|| (s_frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV21M_P010)
			|| (s_frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV16M_P210)
			|| (s_frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV61M_P210)) {
		input_stride_lum_w = (u32)(((in_dma_width * 2 + 15) / 16) * 16);
		input_stride_chroma_w = (u32)(((in_dma_width * 2 + 15) / 16) * 16);
	} else {
		input_stride_lum_w = (u32)(((in_dma_width + 15) / 16) * 16);
		input_stride_chroma_w = (u32)(((in_dma_width + 15) / 16) * 16);
		if ((s_frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV16M_S10B)
				|| (s_frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV61M_S10B)
				|| (s_frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV12M_S10B)
				|| (s_frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV21M_S10B)) {
			input_stride_lum_w_2bit = (u32)((((in_dma_width * 2 + 7) / 8 + 15) / 16) * 16);
			input_stride_chroma_w_2bit = (u32)((((in_dma_width * 2 + 7) / 8 + 15) / 16) * 16);
		}
	}

	if ((d_frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV12M_P010)
			|| (d_frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV21M_P010)
			|| (d_frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV16M_P210)
			|| (d_frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV61M_P210)) {
		output_stride_lum_w = (u32)(((out_dma_width * 2 + 15) / 16) * 16);
		output_stride_chroma_w = (u32)(((out_dma_width * 2 + 15) / 16) * 16);
	} else {
		output_stride_lum_w = (u32)(((out_dma_width + 15) / 16) * 16);
		output_stride_chroma_w = (u32)(((out_dma_width + 15) / 16) * 16);
		if ((d_frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV16M_S10B)
				|| (d_frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV61M_S10B)
				|| (d_frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV12M_S10B)
				|| (d_frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV21M_S10B)) {
			output_stride_lum_w_2bit = (u32)((((out_dma_width * 2 + 7) / 8 + 15) / 16) * 16);
			output_stride_chroma_w_2bit = (u32)((((out_dma_width * 2 + 7) / 8 + 15) / 16) * 16);
		}
	}

/*
	src_10bit_format == packed10bit (10bit + 10bit + 10bit... no padding)
	input_stride_lum_w = (u32)(((in_dma_width * 10 + 7) / 8 + 15) / 16 * 16);
	input_stride_chroma_w = (u32)(((in_dma_width * 10 + 7) / 8 + 15) / 16 * 16);
*/

	gdc_dbg("s_w = %d, lum stride_w = %d, stride_2bit_w = %d\n",
		s_frame->width, input_stride_lum_w, input_stride_lum_w_2bit);
	gdc_dbg("s_w = %d, chroma stride_w = %d, stride_2bit_w = %d\n",
		s_frame->width, input_stride_chroma_w, input_stride_chroma_w_2bit);
	gdc_dbg("d_w = %d, lum stride_w = %d, stride_2bit_w = %d\n",
		d_frame->width, output_stride_lum_w, output_stride_lum_w_2bit);
	gdc_dbg("d_w = %d, chroma stride_w = %d, stride_2bit_w = %d\n",
		d_frame->width, output_stride_chroma_w, output_stride_chroma_w_2bit);

	/* about write DMA stride size : It should be aligned with 16 bytes */
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_WAXI_STRIDE_LUM],
		&gdc_fields[GDC_F_WRITE_STRIDE_LUMA], output_stride_lum_w);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_WAXI_STRIDE_CHROMA],
		&gdc_fields[GDC_F_WRITE_STRIDE_CHROMA], output_stride_chroma_w);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_WAXI_STRIDE_2BIT_LUM],
		&gdc_fields[GDC_F_WRITE_STRIDE_2BIT_LUMA], output_stride_lum_w_2bit);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_WAXI_STRIDE_2BIT_CHROMA],
		&gdc_fields[GDC_F_WRITE_STRIDE_2BIT_CHROMA], output_stride_chroma_w_2bit);

	/* about read DMA stride size */
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_PXC_STRIDE_LUM],
		&gdc_fields[GDC_F_PXCSTRIDELUM], input_stride_lum_w);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_PXC_STRIDE_CHROMA],
		&gdc_fields[GDC_F_PXCSTRIDECHROMA], input_stride_chroma_w);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_PXC_STRIDE_2BIT_LUM],
		&gdc_fields[GDC_F_PXCSTRIDE2BITLUM], input_stride_lum_w_2bit);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_PXC_STRIDE_2BIT_CHROMA],
		&gdc_fields[GDC_F_PXCSTRIDE2BITCHROMA], input_stride_chroma_w_2bit);

}

void camerapp_hw_gdc_set_dma_address(void __iomem *base_addr, struct gdc_frame *s_frame, struct gdc_frame *d_frame)
{
	/*Dst Base address of the first Y, UV plane in the DRAM.  must be [1:0] == 0 : Must be multiple of 4*/
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_DST_BASE_LUM],
		&gdc_fields[GDC_F_GDC_DST_BASE_LUM], d_frame->addr.y);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_DST_BASE_CHROMA],
		&gdc_fields[GDC_F_GDC_DST_BASE_CHROMA], d_frame->addr.cb);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_DST_2BIT_BASE_LUM],
		&gdc_fields[GDC_F_GDC_DST_2BIT_BASE_LUM], d_frame->addr.y_2bit);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_DST_2BIT_BASE_CHROMA],
		&gdc_fields[GDC_F_GDC_DST_2BIT_BASE_CHROMA], d_frame->addr.cbcr_2bit);

	/* RDMA address */
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_PXC_DPB_BASE_LUM],
		&gdc_fields[GDC_F_DPBLUMBASE], s_frame->addr.y);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_PXC_DPB_BASE_CHROMA],
		&gdc_fields[GDC_F_DPBCHRBASE], s_frame->addr.cb);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_PXC_DPB_2BIT_BASE_LUM],
		&gdc_fields[GDC_F_DPB2BITLUMBASE], s_frame->addr.y_2bit);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_PXC_DPB_2BIT_BASE_CHROMA],
		&gdc_fields[GDC_F_DPB2BITCHRBASE], s_frame->addr.cbcr_2bit);
}

void camerapp_hw_gdc_set_format(void __iomem *base_addr, struct gdc_frame *s_frame, struct gdc_frame *d_frame)
{
	u32 input_pix_format, input_10bit_format;
	u32 output_pix_format, output_10bit_format;
	u32 input_yuv_format, output_yuv_format;

	/*PIXEL_FORMAT		0: NV12 (2plane Y/UV order), 1: NV21 (2plane Y/VU order) */
	/* YUV bit depth : 0 - 8bit / 1 - P010 / 2 - 8+2 / 3 - packed 10bit */
	/* YUV format: 0 - YUV422, 1 - YUV420 */

	/* input */
	switch (s_frame->gdc_fmt->pixelformat) {
	case V4L2_PIX_FMT_NV12M_P010:
	case V4L2_PIX_FMT_NV21M_P010:
	case V4L2_PIX_FMT_NV16M_P210:
	case V4L2_PIX_FMT_NV61M_P210:
		input_10bit_format = 1;
		break;
	case V4L2_PIX_FMT_NV12M_S10B:
	case V4L2_PIX_FMT_NV21M_S10B:
	case V4L2_PIX_FMT_NV16M_S10B:
	case V4L2_PIX_FMT_NV61M_S10B:
		input_10bit_format = 2;
		break;
	default:
		input_10bit_format = 0;
		break;
	}

	switch (s_frame->gdc_fmt->pixelformat) {
	case V4L2_PIX_FMT_NV21M:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_NV61M:
	case V4L2_PIX_FMT_NV21M_P010:
	case V4L2_PIX_FMT_NV61M_P210:
	case V4L2_PIX_FMT_NV21M_S10B:
	case V4L2_PIX_FMT_NV61M_S10B:
		input_pix_format = 1;
		break;
	default:
		input_pix_format = 0;
		break;
	}

	switch (s_frame->gdc_fmt->pixelformat) {
	case V4L2_PIX_FMT_NV12M:
	case V4L2_PIX_FMT_NV21M:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_NV12M_P010:
	case V4L2_PIX_FMT_NV21M_P010:
	case V4L2_PIX_FMT_NV12M_S10B:
	case V4L2_PIX_FMT_NV21M_S10B:
		input_yuv_format = 1;
		break;
	default:
		input_yuv_format = 0;
		break;
	}

	/* output */
	switch (d_frame->gdc_fmt->pixelformat) {
	case V4L2_PIX_FMT_NV12M_P010:
	case V4L2_PIX_FMT_NV21M_P010:
	case V4L2_PIX_FMT_NV16M_P210:
	case V4L2_PIX_FMT_NV61M_P210:
		output_10bit_format = 1;
		break;
	case V4L2_PIX_FMT_NV12M_S10B:
	case V4L2_PIX_FMT_NV21M_S10B:
	case V4L2_PIX_FMT_NV16M_S10B:
	case V4L2_PIX_FMT_NV61M_S10B:
		output_10bit_format = 2;
		break;
	default:
		output_10bit_format = 0;
		break;
	}

	switch (d_frame->gdc_fmt->pixelformat) {
	case V4L2_PIX_FMT_NV21M:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_NV61M:
	case V4L2_PIX_FMT_NV21M_P010:
	case V4L2_PIX_FMT_NV61M_P210:
	case V4L2_PIX_FMT_NV21M_S10B:
	case V4L2_PIX_FMT_NV61M_S10B:
		output_pix_format = 1;
		break;
	default:
		output_pix_format = 0;
		break;
	}

	switch (d_frame->gdc_fmt->pixelformat) {
	case V4L2_PIX_FMT_NV12M:
	case V4L2_PIX_FMT_NV21M:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_NV12M_P010:
	case V4L2_PIX_FMT_NV21M_P010:
	case V4L2_PIX_FMT_NV12M_S10B:
	case V4L2_PIX_FMT_NV21M_S10B:
		output_yuv_format = 1;
		break;
	default:
		output_yuv_format = 0;
		break;
	}

	gdc_dbg("gdc format (10bit, pix, yuv) : in(%d, %d, %d), out(%d, %d, %d)\n",
		input_10bit_format, input_pix_format, input_yuv_format,
		output_10bit_format, output_pix_format, output_yuv_format);

	/* IN/OUT Format */
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_YUV_FORMAT],
		&gdc_fields[GDC_F_DST_PIXEL_FORMAT], output_pix_format);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_YUV_FORMAT],
		&gdc_fields[GDC_F_DST_10BIT_FORMAT], output_10bit_format);

	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_YUV_FORMAT],
		&gdc_fields[GDC_F_SRC_PIXEL_FORMAT], input_pix_format);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_YUV_FORMAT],
		&gdc_fields[GDC_F_SRC_10BIT_FORMAT], input_10bit_format);

	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_YUV_FORMAT],
		&gdc_fields[GDC_F_GDC_YUV_FORMAT], input_yuv_format);

}

void camerapp_hw_gdc_set_pixel_minmax(void __iomem *base_addr,
		u32 luma_min, u32 luma_max, u32 chroma_min, u32 chroma_max)
{
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_LUMA_MINMAX],
		&gdc_fields[GDC_F_GDC_LUMA_MIN], luma_min);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_LUMA_MINMAX],
		&gdc_fields[GDC_F_GDC_LUMA_MAX], luma_max);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_CHROMA_MINMAX],
		&gdc_fields[GDC_F_GDC_CHROMA_MIN], chroma_min);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_CHROMA_MINMAX],
		&gdc_fields[GDC_F_GDC_CHROMA_MAX], chroma_max);
}

void camerapp_hw_gdc_update_param(void __iomem *base_addr, struct gdc_dev *gdc)
{
	struct gdc_frame *d_frame, *s_frame;
	struct gdc_crop_param *crop_param;
	u32 input_dither = 0, output_dither = 0;
	u32 luma_min, luma_max, chroma_min, chroma_max;

	s_frame = &gdc->current_ctx->s_frame;
	d_frame = &gdc->current_ctx->d_frame;
	crop_param = &gdc->current_ctx->crop_param;

	/* Dither : 0 = off, 1 = on => read(inverse_dither), write (dither) */
	if (s_frame->pixel_size == CAMERAPP_PIXEL_SIZE_8_2BIT)
		input_dither = 1;
	if (d_frame->pixel_size == CAMERAPP_PIXEL_SIZE_8_2BIT)
		output_dither = 1;
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_DITHER], &gdc_fields[GDC_F_DST_DITHER], output_dither);
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_DITHER], &gdc_fields[GDC_F_SRC_INVDITHER], input_dither);

	/* interpolation type */
	/* 0 -all closest / 1 - Y bilinear, UV closest / 2 - all bilinear / 3 - Y bi-cubic, UV bilinear */
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_INTERPOLATION], &gdc_fields[GDC_F_GDC_INTERP_TYPE], 3);
	/* Clamping type: 0 - none / 1 - min/max / 2 - near pixels min/max */
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_INTERPOLATION], &gdc_fields[GDC_F_GDC_CLAMP_TYPE], 2);

	/* gdc grid scale setting */
	camerapp_hw_gdc_update_scale_parameters(base_addr, s_frame, d_frame, crop_param);
	/* gdc grid setting */
	camerapp_hw_gdc_update_grid_param(base_addr, &gdc->current_ctx->grid_param);

	/* dma buffer size & address setting */
	camerapp_hw_gdc_set_dma_address(base_addr, s_frame, d_frame);
	camerapp_hw_gdc_update_dma_size(base_addr, s_frame, d_frame);

	/* in / out data Format */
	camerapp_hw_gdc_set_format(base_addr, s_frame, d_frame);

	/* output pixcel min/max value clipping */
	luma_min = chroma_min = 0;
	luma_max = chroma_max = 0x3ff;
	camerapp_hw_gdc_set_pixel_minmax(base_addr, luma_min, luma_max, chroma_min, chroma_max);

	/* The values are multiples of 32, value : 32 ~ 512 */
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_PRO_SIZE], &gdc_fields[GDC_F_GDC_PRO_WIDTH], 64);
	/* The values are multiples of 8, value : 8 ~ 512 */
	camerapp_sfr_set_field(base_addr, &gdc_regs[GDC_R_GDC_PRO_SIZE], &gdc_fields[GDC_F_GDC_PRO_HEIGHT], 64);
	camerapp_sfr_set_reg(base_addr, &gdc_regs[GDC_R_GDC_PROCESSING], 1);
}


void camerapp_hw_gdc_status_read(void __iomem *base_addr)
{
	u32 ReqCntLum;
	u32 HitCntLum;
	u32 isPixelBusy;
	u32 isPortBusy;
	u32 cacheStatus0, cacheStatus1;
	u32 hitFifoStatus0, hitFifoStatus1;
	u32 missFifoStatus0, missFifoStatus1;
	u32 isPending;

	/* read Total request count of luminance cache channel #0 */
	ReqCntLum = camerapp_sfr_get_reg(base_addr, &gdc_regs[GDC_R_PXC_REQ_CNT_LUM_0]);
	HitCntLum = camerapp_sfr_get_reg(base_addr, &gdc_regs[GDC_R_PXC_HIT_CNT_LUM_0]);
	isPixelBusy = camerapp_sfr_get_field(base_addr, &gdc_regs[GDC_R_PXC_DEBUG_BUSY], &gdc_fields[GDC_F_PXCBUSYLUM]);
	isPortBusy = camerapp_sfr_get_field(base_addr, &gdc_regs[GDC_R_PXC_DEBUG_BUSY], &gdc_fields[GDC_F_PORT0BUSY]);
	cacheStatus0 = camerapp_sfr_get_field(base_addr,
		&gdc_regs[GDC_R_PXC_DEBUG_LUM0], &gdc_fields[GDC_F_CHROMACACHEST0]);
	hitFifoStatus0 = camerapp_sfr_get_field(base_addr,
		&gdc_regs[GDC_R_PXC_DEBUG_LUM0], &gdc_fields[GDC_F_HITFIFOST0]);
	missFifoStatus0 = camerapp_sfr_get_field(base_addr,
		&gdc_regs[GDC_R_PXC_DEBUG_LUM0], &gdc_fields[GDC_F_MISSFIFOST0]);
	cacheStatus1 = camerapp_sfr_get_field(base_addr,
		&gdc_regs[GDC_R_PXC_DEBUG_LUM1], &gdc_fields[GDC_F_CHROMACACHEST1]);
	hitFifoStatus1 = camerapp_sfr_get_field(base_addr,
		&gdc_regs[GDC_R_PXC_DEBUG_LUM1], &gdc_fields[GDC_F_HITFIFOST1]);
	missFifoStatus1 = camerapp_sfr_get_field(base_addr,
		&gdc_regs[GDC_R_PXC_DEBUG_LUM1], &gdc_fields[GDC_F_MISSFIFOST1]);

	isPending = camerapp_sfr_get_reg(base_addr, &gdc_regs[GDC_R_PEND]);

	pr_info("[GDC_Status] = %x /%x /%x /%x /%x /%x /%x /%x /%x /%x /%x\n",
		ReqCntLum, HitCntLum, isPixelBusy, isPortBusy, cacheStatus0, hitFifoStatus0,
		missFifoStatus0, cacheStatus1, hitFifoStatus1, missFifoStatus1, isPending);

}

void camerapp_gdc_sfr_dump(void __iomem *base_addr)
{
	u32 i = 0;
	u32 reg_value = 0;

	gdc_dbg("gdc v2.0");

	for (i = 0; i < GDC_REG_CNT; i++) {
		reg_value = readl(base_addr + gdc_regs[i].sfr_offset);
		pr_info("[DUMP] reg:[%s][0x%04X], value:[0x%08X]\n",
			gdc_regs[i].reg_name, gdc_regs[i].sfr_offset, reg_value);
	}
}
