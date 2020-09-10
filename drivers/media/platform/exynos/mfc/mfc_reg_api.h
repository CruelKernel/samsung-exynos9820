/*
 * drivers/media/platform/exynos/mfc/mfc_reg_api.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_REG_API_H
#define __MFC_REG_API_H __FILE__

#include "mfc_common.h"

#include "mfc_utils.h"

#define MFC_READL(offset)		readl(dev->regs_base + (offset))
#define MFC_WRITEL(data, offset)	writel((data), dev->regs_base + (offset))

#define MFC_RAW_READL(offset)		__raw_readl(dev->regs_base + (offset))
#define MFC_RAW_WRITEL(data, offset)	__raw_writel((data), dev->regs_base + (offset))

#define MFC_MMU0_READL(offset)		readl(dev->sysmmu0_base + (offset))
#define MFC_MMU1_READL(offset)		readl(dev->sysmmu1_base + (offset))

#define HWFC_WRITEL(data, offset)	writel((data), dev->hwfc_base + (offset))

#define MMCACHE_READL(offset)		readl(dev->mmcache.base + (offset))
#define MMCACHE_WRITEL(data, offset)	writel((data), dev->mmcache.base + (offset))

/* version */
#define mfc_get_fimv_info()		((MFC_READL(MFC_REG_FW_VERSION)		\
						>> MFC_REG_FW_VER_INFO_SHFT)		\
						& MFC_REG_FW_VER_INFO_MASK)
#define mfc_get_fw_ver_year()	((MFC_READL(MFC_REG_FW_VERSION)		\
						>> MFC_REG_FW_VER_YEAR_SHFT)		\
						& MFC_REG_FW_VER_YEAR_MASK)
#define mfc_get_fw_ver_month()	((MFC_READL(MFC_REG_FW_VERSION)		\
						>> MFC_REG_FW_VER_MONTH_SHFT)		\
						& MFC_REG_FW_VER_MONTH_MASK)
#define mfc_get_fw_ver_date()	((MFC_READL(MFC_REG_FW_VERSION)		\
						>> MFC_REG_FW_VER_DATE_SHFT)		\
						& MFC_REG_FW_VER_DATE_MASK)
#define mfc_get_fw_ver_all()	((MFC_READL(MFC_REG_FW_VERSION)		\
						>> MFC_REG_FW_VER_ALL_SHFT)		\
						& MFC_REG_FW_VER_ALL_MASK)
#define mfc_get_mfc_version()	((MFC_READL(MFC_REG_MFC_VERSION)		\
						>> MFC_REG_MFC_VER_SHFT)		\
						& MFC_REG_MFC_VER_MASK)


/* decoding & display information */
#define mfc_get_dec_status()	(MFC_READL(MFC_REG_D_DECODED_STATUS)		\
						& MFC_REG_DEC_STATUS_DECODED_STATUS_MASK)
#define mfc_get_disp_status()	(MFC_READL(MFC_REG_D_DISPLAY_STATUS)		\
						& MFC_REG_DISP_STATUS_DISPLAY_STATUS_MASK)
#define mfc_get_res_change()	((MFC_READL(MFC_REG_D_DISPLAY_STATUS)		\
						>> MFC_REG_DISP_STATUS_RES_CHANGE_SHIFT)	\
						& MFC_REG_DISP_STATUS_RES_CHANGE_MASK)
#define mfc_get_black_bar_detection()	((MFC_READL(MFC_REG_D_DISPLAY_STATUS)		\
						>> MFC_REG_DISP_STATUS_BLACK_BAR_DETECT_SHIFT)	\
						& MFC_REG_DISP_STATUS_BLACK_BAR_DETECT_MASK)
#define mfc_get_dpb_change()	((MFC_READL(MFC_REG_D_DISPLAY_STATUS)		\
						>> MFC_REG_DISP_STATUS_NEED_DPB_CHANGE_SHIFT)	\
						& MFC_REG_DISP_STATUS_NEED_DPB_CHANGE_MASK)
#define mfc_get_scratch_change()	((MFC_READL(MFC_REG_D_DISPLAY_STATUS)		\
						>> MFC_REG_DISP_STATUS_NEED_SCRATCH_CHANGE_SHIFT)	\
						& MFC_REG_DISP_STATUS_NEED_SCRATCH_CHANGE_MASK)
#define mfc_get_disp_frame_type()	(MFC_READL(MFC_REG_D_DISPLAY_FRAME_TYPE)	\
						& MFC_REG_DISPLAY_FRAME_MASK)
#define mfc_get_dec_frame_type()	(MFC_READL(MFC_REG_D_DECODED_FRAME_TYPE)	\
						& MFC_REG_DECODED_FRAME_MASK)
#define mfc_get_interlace_type()	((MFC_READL(MFC_REG_D_DISPLAY_FRAME_TYPE)	\
						>> MFC_REG_DISPLAY_TEMP_INFO_SHIFT)	\
						& MFC_REG_DISPLAY_TEMP_INFO_MASK)
#define mfc_is_interlace_picture()	((MFC_READL(MFC_REG_D_DISPLAY_STATUS)		\
						>> MFC_REG_DISP_STATUS_INTERLACE_SHIFT)\
						& MFC_REG_DISP_STATUS_INTERLACE_MASK)
#define mfc_is_mbaff_picture()	((MFC_READL(MFC_REG_D_H264_INFO)		\
						>> MFC_REG_D_H264_INFO_MBAFF_FRAME_FLAG_SHIFT)\
						& MFC_REG_D_H264_INFO_MBAFF_FRAME_FLAG_MASK)
#define mfc_get_img_width()		MFC_READL(MFC_REG_D_DISPLAY_FRAME_WIDTH)
#define mfc_get_img_height()	MFC_READL(MFC_REG_D_DISPLAY_FRAME_HEIGHT)
#define mfc_get_disp_y_addr()	MFC_READL(MFC_REG_D_DISPLAY_LUMA_ADDR)
#define mfc_get_dec_y_addr()	MFC_READL(MFC_REG_D_DECODED_LUMA_ADDR)


/* kind of interrupt */
#define mfc_get_int_err()		MFC_READL(MFC_REG_ERROR_CODE)


/* additional information */
#define mfc_get_consumed_stream()		MFC_READL(MFC_REG_D_DECODED_NAL_SIZE)
#define mfc_get_dpb_count()			MFC_READL(MFC_REG_D_MIN_NUM_DPB)
#define mfc_get_min_dpb_size(x)		MFC_READL(MFC_REG_D_MIN_FIRST_PLANE_DPB_SIZE + (x * 4))
#define mfc_get_scratch_size()		MFC_READL(MFC_REG_D_MIN_SCRATCH_BUFFER_SIZE)
#define mfc_get_mv_count()			MFC_READL(MFC_REG_D_MIN_NUM_MV)
#define mfc_get_inst_no()			MFC_READL(MFC_REG_RET_INSTANCE_ID)
#define mfc_get_enc_dpb_count()		MFC_READL(MFC_REG_E_NUM_DPB)
#define mfc_get_enc_scratch_size()		MFC_READL(MFC_REG_E_MIN_SCRATCH_BUFFER_SIZE)
#define mfc_get_enc_strm_size()		MFC_READL(MFC_REG_E_STREAM_SIZE)
#define mfc_get_enc_slice_type()		MFC_READL(MFC_REG_E_SLICE_TYPE)
#define mfc_get_enc_pic_count()		MFC_READL(MFC_REG_E_PICTURE_COUNT)
#define mfc_get_sei_avail()			MFC_READL(MFC_REG_D_SEI_AVAIL)
#define mfc_get_sei_content_light()		MFC_READL(MFC_REG_D_CONTENT_LIGHT_LEVEL_INFO_SEI)
#define mfc_get_sei_mastering0()		MFC_READL(MFC_REG_D_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_0)
#define mfc_get_sei_mastering1()		MFC_READL(MFC_REG_D_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_1)
#define mfc_get_sei_mastering2()		MFC_READL(MFC_REG_D_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_2)
#define mfc_get_sei_mastering3()		MFC_READL(MFC_REG_D_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_3)
#define mfc_get_sei_mastering4()		MFC_READL(MFC_REG_D_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_4)
#define mfc_get_sei_mastering5()		MFC_READL(MFC_REG_D_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_5)
#define mfc_get_sei_avail_frame_pack()	(MFC_READL(MFC_REG_D_SEI_AVAIL)	\
						& MFC_REG_D_SEI_AVAIL_FRAME_PACK_MASK)
#define mfc_get_sei_avail_content_light()	((MFC_READL(MFC_REG_D_SEI_AVAIL)	\
						>> MFC_REG_D_SEI_AVAIL_CONTENT_LIGHT_SHIFT)	\
						& MFC_REG_D_SEI_AVAIL_CONTENT_LIGHT_MASK)
#define mfc_get_sei_avail_mastering_display()	((MFC_READL(MFC_REG_D_SEI_AVAIL)	\
						>> MFC_REG_D_SEI_AVAIL_MASTERING_DISPLAY_SHIFT)	\
						& MFC_REG_D_SEI_AVAIL_MASTERING_DISPLAY_MASK)
#define mfc_get_sei_avail_st_2094_40()		((MFC_READL(MFC_REG_D_SEI_AVAIL)	\
						>> MFC_REG_D_SEI_AVAIL_ST_2094_40_SHIFT)	\
						& MFC_REG_D_SEI_AVAIL_ST_2094_40_MASK)
#define mfc_get_video_signal_type()		((MFC_READL(MFC_REG_D_VIDEO_SIGNAL_TYPE)	\
						>> MFC_REG_D_VIDEO_SIGNAL_TYPE_FLAG_SHIFT)	\
						& MFC_REG_D_VIDEO_SIGNAL_TYPE_FLAG_MASK)
#define mfc_get_colour_description()	((MFC_READL(MFC_REG_D_VIDEO_SIGNAL_TYPE)	\
						>> MFC_REG_D_COLOUR_DESCRIPTION_FLAG_SHIFT)	\
						& MFC_REG_D_COLOUR_DESCRIPTION_FLAG_MASK)
#define mfc_get_black_bar_pos_x()		((MFC_READL(MFC_REG_D_BLACK_BAR_START_POS)	\
						>> MFC_REG_D_BLACK_BAR_START_X_SHIFT)		\
						& MFC_REG_D_BLACK_BAR_START_X_MASK)
#define mfc_get_black_bar_pos_y()		((MFC_READL(MFC_REG_D_BLACK_BAR_START_POS)	\
						>> MFC_REG_D_BLACK_BAR_START_Y_SHIFT)		\
						& MFC_REG_D_BLACK_BAR_START_Y_MASK)
#define mfc_get_black_bar_image_w()		((MFC_READL(MFC_REG_D_BLACK_BAR_IMAGE_SIZE)	\
						>> MFC_REG_D_BLACK_BAR_IMAGE_W_SHIFT)	\
						& MFC_REG_D_BLACK_BAR_IMAGE_W_MASK)
#define mfc_get_black_bar_image_h()		((MFC_READL(MFC_REG_D_BLACK_BAR_IMAGE_SIZE)	\
						>> MFC_REG_D_BLACK_BAR_IMAGE_H_SHIFT)	\
						& MFC_REG_D_BLACK_BAR_IMAGE_H_MASK)
#define mfc_get_mvc_disp_view_id()		(MFC_READL(MFC_REG_D_MVC_VIEW_ID)		\
						& MFC_REG_D_MVC_VIEW_ID_DISP_MASK)
#define mfc_get_profile()			(MFC_READL(MFC_REG_D_DECODED_PICTURE_PROFILE)	\
						& MFC_REG_D_DECODED_PIC_PROFILE_MASK)
#define mfc_get_luma_bit_depth_minus8()	((MFC_READL(MFC_REG_D_DECODED_PICTURE_PROFILE)	\
						>> MFC_REG_D_BIT_DEPTH_LUMA_MINUS8_SHIFT)	\
						& MFC_REG_D_BIT_DEPTH_LUMA_MINUS8_MASK)
#define mfc_get_chroma_bit_depth_minus8()	((MFC_READL(MFC_REG_D_DECODED_PICTURE_PROFILE)	\
						>> MFC_REG_D_BIT_DEPTH_CHROMA_MINUS8_SHIFT)	\
						& MFC_REG_D_BIT_DEPTH_CHROMA_MINUS8_MASK)
#define mfc_get_dec_used_flag()		MFC_READL(MFC_REG_D_USED_DPB_FLAG_LOWER)
#define mfc_get_enc_nal_done_info()		((MFC_READL(MFC_REG_E_NAL_DONE_INFO) & (0x3 << 4)) >> 4)
#define mfc_get_chroma_format()		(MFC_READL(MFC_REG_D_CHROMA_FORMAT)		\
						& MFC_REG_D_CHROMA_FORMAT_MASK)
#define mfc_get_color_range()		((MFC_READL(MFC_REG_D_CHROMA_FORMAT)	\
						>> MFC_REG_D_COLOR_RANGE_SHIFT)	\
						& MFC_REG_D_COLOR_RANGE_MASK)
#define mfc_get_color_space()		((MFC_READL(MFC_REG_D_CHROMA_FORMAT)	\
						>> MFC_REG_D_COLOR_SPACE_SHIFT)	\
						& MFC_REG_D_COLOR_SPACE_MASK)
#define mfc_get_num_of_tile()		((MFC_READL(MFC_REG_D_DECODED_STATUS)		\
						>> MFC_REG_DEC_STATUS_NUM_OF_TILE_SHIFT)	\
						& MFC_REG_DEC_STATUS_NUM_OF_TILE_MASK)
#define mfc_get_lcu_size()			(MFC_READL(MFC_REG_D_HEVC_INFO)		\
						& MFC_REG_D_HEVC_INFO_LCU_SIZE_MASK)


/* nal queue information */
#define mfc_get_nal_q_input_count()		MFC_READL(MFC_REG_NAL_QUEUE_INPUT_COUNT)
#define mfc_get_nal_q_output_count()	MFC_READL(MFC_REG_NAL_QUEUE_OUTPUT_COUNT)
#define mfc_get_nal_q_input_exe_count()	MFC_READL(MFC_REG_NAL_QUEUE_INPUT_EXE_COUNT)
#define mfc_get_nal_q_info()		MFC_READL(MFC_REG_NAL_QUEUE_INFO)
#define mfc_get_nal_q_input_addr()		MFC_READL(MFC_REG_NAL_QUEUE_INPUT_ADDR)
#define mfc_get_nal_q_input_size()		MFC_READL(MFC_REG_NAL_QUEUE_INPUT_SIZE)
#define mfc_get_nal_q_output_addr()		MFC_READL(MFC_REG_NAL_QUEUE_OUTPUT_ADDR)
#define mfc_get_nal_q_output_ize()		MFC_READL(MFC_REG_NAL_QUEUE_OUTPUT_SIZE)

static inline void mfc_reset_nal_queue_registers(struct mfc_dev *dev)
{
	MFC_WRITEL(0x0, MFC_REG_NAL_QUEUE_INPUT_COUNT);
	MFC_WRITEL(0x0, MFC_REG_NAL_QUEUE_OUTPUT_COUNT);
	MFC_WRITEL(0x0, MFC_REG_NAL_QUEUE_INPUT_EXE_COUNT);
	MFC_WRITEL(0x0, MFC_REG_NAL_QUEUE_INFO);
}

static inline void mfc_update_nal_queue_input(struct mfc_dev *dev,
	dma_addr_t addr, unsigned int size)
{
	MFC_WRITEL(addr, MFC_REG_NAL_QUEUE_INPUT_ADDR);
	MFC_WRITEL(size, MFC_REG_NAL_QUEUE_INPUT_SIZE);
}

static inline void mfc_update_nal_queue_output(struct mfc_dev *dev,
	dma_addr_t addr, unsigned int size)
{
	MFC_WRITEL(addr, MFC_REG_NAL_QUEUE_OUTPUT_ADDR);
	MFC_WRITEL(size, MFC_REG_NAL_QUEUE_OUTPUT_SIZE);
}

static inline void mfc_update_nal_queue_input_count(struct mfc_dev *dev,
	unsigned int input_count)
{
	MFC_WRITEL(input_count, MFC_REG_NAL_QUEUE_INPUT_COUNT);
}

static inline void mfc_dec_get_crop_info(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	u32 left, right, top, bottom;

	left = MFC_READL(MFC_REG_D_DISPLAY_CROP_INFO1);
	right = left >> MFC_REG_D_SHARED_CROP_RIGHT_SHIFT;
	left = left & MFC_REG_D_SHARED_CROP_LEFT_MASK;
	top = MFC_READL(MFC_REG_D_DISPLAY_CROP_INFO2);
	bottom = top >> MFC_REG_D_SHARED_CROP_BOTTOM_SHIFT;
	top = top & MFC_REG_D_SHARED_CROP_TOP_MASK;

	dec->cr_left = left;
	dec->cr_right = right;
	dec->cr_top = top;
	dec->cr_bot = bottom;
}

static inline void mfc_clear_enc_res_change(struct mfc_dev *dev)
{
	unsigned int reg = 0;

	reg = MFC_READL(MFC_REG_E_PARAM_CHANGE);
	reg &= ~(0x7 << 6);
	MFC_WRITEL(reg, MFC_REG_E_PARAM_CHANGE);
}

static inline void mfc_clear_roi_enable(struct mfc_dev *dev)
{
	unsigned int reg = 0;

	reg = MFC_READL(MFC_REG_E_RC_ROI_CTRL);
	reg &= ~(0x1);
	MFC_WRITEL(reg, MFC_REG_E_RC_ROI_CTRL);
}

void mfc_dbg_enable(struct mfc_dev *dev);
void mfc_dbg_disable(struct mfc_dev *dev);
void mfc_dbg_set_addr(struct mfc_dev *dev);

void mfc_otf_set_frame_addr(struct mfc_ctx *ctx, int num_planes);
void mfc_otf_set_stream_size(struct mfc_ctx *ctx, unsigned int size);
void mfc_otf_set_hwfc_index(struct mfc_ctx *ctx, int job_id);

int mfc_set_dec_codec_buffers(struct mfc_ctx *ctx);
int mfc_set_enc_codec_buffers(struct mfc_ctx *mfc_ctx);

int mfc_set_dec_stream_buffer(struct mfc_ctx *ctx,
		struct mfc_buf *mfc_buf,
		unsigned int start_num_byte,
		unsigned int buf_size);

void mfc_set_enc_frame_buffer(struct mfc_ctx *ctx,
		struct mfc_buf *mfc_buf, int num_planes);
int mfc_set_enc_stream_buffer(struct mfc_ctx *ctx,
		struct mfc_buf *mfc_buf);

void mfc_get_enc_frame_buffer(struct mfc_ctx *ctx,
		dma_addr_t addr[], int num_planes);
void mfc_set_enc_stride(struct mfc_ctx *ctx);

int mfc_set_dynamic_dpb(struct mfc_ctx *ctx, struct mfc_buf *dst_vb);

void mfc_set_pixel_format(struct mfc_ctx *ctx, unsigned int format);

void mfc_print_hdr_plus_info(struct mfc_ctx *ctx, struct hdr10_plus_meta *sei_meta);
void mfc_get_hdr_plus_info(struct mfc_ctx *ctx, struct hdr10_plus_meta *sei_meta);
void mfc_set_hdr_plus_info(struct mfc_ctx *ctx, struct hdr10_plus_meta *sei_meta);

#endif /* __MFC_REG_API_H */
