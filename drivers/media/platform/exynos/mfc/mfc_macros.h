/*
 * drivers/media/platform/exynos/mfc/mfc_macros.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_MACROS_H
#define __MFC_MACROS_H __FILE__

#define WIDTH_MB(x_size)	((x_size + 15) / 16)
#define HEIGHT_MB(y_size)	((y_size + 15) / 16)

/*
   Note that lcu_width and lcu_height are defined as follows :
   lcu_width = (frame_width + lcu_size - 1)/lcu_size
   lcu_height = (frame_height + lcu_size - 1)/lcu_size.
   (lcu_size is 32(encoder) or 64(decoder))
*/
#define DEC_LCU_WIDTH(x_size)	((x_size + 63) / 64)
#define ENC_LCU_WIDTH(x_size)	((x_size + 31) / 32)
#define DEC_LCU_HEIGHT(y_size)	((y_size + 63) / 64)
#define ENC_LCU_HEIGHT(y_size)	((y_size + 31) / 32)

#define STREAM_BUF_ALIGN		512
#define MFC_LINEAR_BUF_SIZE		256
#define set_strm_size_max(cpb_max)	((cpb_max) - STREAM_BUF_ALIGN)

#define DEC_STATIC_BUFFER_SIZE	20480

#define DEC_MV_SIZE_MB(x, y)	(WIDTH_MB(x) * (((HEIGHT_MB(y)+1)/2)*2) * 64 + 1024)
#define DEC_HEVC_MV_SIZE(x, y)	(DEC_LCU_WIDTH(x) * DEC_LCU_HEIGHT(y) * 256 + 512)

#define ENC_HEVC_LUMA_DPB_10B_SIZE(x, y)				\
	((x + 63) / 64) * 64 * ((y + 31) / 32 ) * 32 +			\
	(((((ENC_LCU_WIDTH(x) * 32 + 3) / 4) + 15) / 16) * 16) *	\
	((y + 31) / 32 ) * 32 + 64
#define ENC_HEVC_CHROMA_DPB_10B_SIZE(x, y)				\
	((x + 63) / 64) * 64 * ((y + 31) / 32 ) * 32 +			\
	(((((ENC_LCU_WIDTH(x) * 32 + 3) / 4) + 15) / 16) * 16) *	\
	((y + 31) / 32 ) * 32 + 64
#define ENC_VP9_LUMA_DPB_10B_SIZE(x, y)					\
	(((x * 2 + 127) / 128) * 128) *					\
	((y + 31) / 32 ) * 32 + 64
#define ENC_VP9_CHROMA_DPB_10B_SIZE(x, y)				\
	(((x * 2 + 127) / 128) * 128) *					\
	((y + 31) / 32 ) * 32 + 64
#define ENC_LUMA_DPB_SIZE(x, y)						\
	((x + 63) / 64) * 64 * ((y + 31) / 32 ) * 32 + 64
#define ENC_CHROMA_DPB_SIZE(x, y)					\
	((x + 63) / 64) * 64 * ((((y + 31) / 32 ) * 32) / 2) + 64

#define ENC_V100_H264_ME_SIZE(x, y)				\
	(((x + 3) * (y + 3) * 8) + ((((x * y) + 63) / 64) * 32) + (((y * 64) + 2304) * (x + 7) / 8))
#define ENC_V100_MPEG4_ME_SIZE(x, y)				\
	(((x + 3) * (y + 3) * 8) + ((((x * y) + 127) / 128) * 16) + (((y * 64) + 2304) * (x + 7) / 8))
#define ENC_V100_VP8_ME_SIZE(x, y)				\
	(((x + 3) * (y + 3) * 8) + (((y * 64) + 2304) * (x + 7) / 8))
#define ENC_V100_VP9_ME_SIZE(x, y)				\
	((((x * 2) + 3) * ((y * 2) + 3) * 128) + (((y * 256) + 2304) * (x + 1) / 2))
#define ENC_V100_HEVC_ME_SIZE(x, y)				\
	(((x + 3) * (y + 3) * 32) + (((y * 128) + 2304) * (x + 3) / 4))

#endif /* __MFC_MACROS_H */
