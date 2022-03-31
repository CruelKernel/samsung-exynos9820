/*
 * drivers/media/platform/exynos/mfc/mfc_common.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_COMMON_H
#define __MFC_COMMON_H __FILE__

#include <linux/exynos_iovmm.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/sched/clock.h>
#include <linux/ion_exynos.h>
#include <linux/dma-buf-container.h>
#include <media/videobuf2-dma-sg.h>
#include <asm/cacheflush.h>

#include "mfc_regs.h"
#include "mfc_macros.h"
#include "mfc_debug.h"
#include "exynos_mfc_media.h"
#include "mfc_data_struct.h"

#define MFC_DRIVER_INFO		180315

#define MFC_MAX_REF_BUFS	2
#define MFC_FRAME_PLANES	2
#define MFC_INFO_INIT_FD	-1

#define MFC_MAX_DRM_CTX		2

/* Interrupt timeout */
#define MFC_INT_TIMEOUT		4000
/* Interrupt short timeout */
#define MFC_INT_SHORT_TIMEOUT	800
/* hwlock timeout */
#define MFC_HWLOCK_TIMEOUT	5000
/* Busy wait timeout */
#define MFC_BW_TIMEOUT		500
/* MMCache invalidation timeout */
#define MMCACHE_INVAL_TIMEOUT	1000
/* Interrupt timeout count*/
#define MFC_INT_TIMEOUT_CNT	2

/* This value guarantees 299.4msec ~ 2.25sec according to MFC clock (668MHz ~ 89MHz)
 * releated with MFC_REG_DEC_TIMEOUT_VALUE */
#define MFC_TIMEOUT_VALUE	200000000

#define NUM_MPEG4_LF_BUF	2

#define FRAME_RATE_RESOLUTION	1000

#define DEFAULT_TAG		(0xE05)
#define IGNORE_TAG		(0xD5C) /* ex) encoder DRC */
#define HEADER_TAG		(0xC5D)

#define MFC_NO_INSTANCE_SET	-1

#define MFC_ENC_CAP_PLANE_COUNT	1
#define MFC_ENC_OUT_PLANE_COUNT	2

#define MFC_NAME_LEN		16
#define MFC_FW_NAME		"mfc_fw.bin"

#define STUFF_BYTE		4

#define MFC_BASE_MASK		((1 << 17) - 1)

/* Error & Warning */
#define mfc_get_err(x)		(((x) >> MFC_REG_ERR_STATUS_SHIFT)	\
						& MFC_REG_ERR_STATUS_MASK)
#define mfc_get_warn(x)		(((x) >> MFC_REG_WARN_STATUS_SHIFT)	\
						& MFC_REG_WARN_STATUS_MASK)

/* MFC conceal color is black */
#define MFC_CONCEAL_COLOR	0x8020000

#define vb_to_mfc_buf(x)		\
	container_of(x, struct mfc_buf, vb.vb2_buf)

#define fh_to_mfc_ctx(x)		\
	container_of(x, struct mfc_ctx, fh)

#define call_bop(b, op, args...)	\
	(b->op ? b->op(args) : 0)

#define call_cop(c, op, args...)	\
	(((c)->c_ops->op) ?		\
		((c)->c_ops->op(args)) : 0)

#define call_dop(d, op, args...)	\
	(((d)->dump_ops->op) ?		\
		((d)->dump_ops->op(args)) : 0)

#define	MFC_CTRL_TYPE_GET	(MFC_CTRL_TYPE_GET_SRC | MFC_CTRL_TYPE_GET_DST)
#define	MFC_CTRL_TYPE_SRC	(MFC_CTRL_TYPE_SET | MFC_CTRL_TYPE_GET_SRC)
#define	MFC_CTRL_TYPE_DST	(MFC_CTRL_TYPE_GET_DST)

#define MFC_FMT_STREAM		(1 << 0)
#define MFC_FMT_FRAME		(1 << 1)
#define MFC_FMT_10BIT		(1 << 2)
#define MFC_FMT_422		(1 << 3)
#define MFC_FMT_RGB		(1 << 4)

/* node check */
#define IS_DEC_NODE(n)		((n == EXYNOS_VIDEONODE_MFC_DEC) ||	\
				(n == EXYNOS_VIDEONODE_MFC_DEC_DRM))
#define IS_ENC_NODE(n)		((n == EXYNOS_VIDEONODE_MFC_ENC) ||	\
				(n == EXYNOS_VIDEONODE_MFC_ENC_DRM) ||	\
				(n == EXYNOS_VIDEONODE_MFC_ENC_OTF) ||	\
				(n == EXYNOS_VIDEONODE_MFC_ENC_OTF_DRM))

/* Decoder codec mode check */
#define IS_H264_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_H264_DEC)
#define IS_H264_MVC_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_H264_MVC_DEC)
#define IS_MPEG4_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_MPEG4_DEC)
#define IS_FIMV1_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_FIMV1_DEC)
#define IS_FIMV2_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_FIMV2_DEC)
#define IS_FIMV3_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_FIMV3_DEC)
#define IS_FIMV4_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_FIMV4_DEC)
#define IS_VC1_DEC(ctx)		((ctx)->codec_mode == MFC_REG_CODEC_VC1_DEC)
#define IS_VC1_RCV_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_VC1_RCV_DEC)
#define IS_MPEG2_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_MPEG2_DEC)
#define IS_HEVC_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_HEVC_DEC)
#define IS_VP9_DEC(ctx)		((ctx)->codec_mode == MFC_REG_CODEC_VP9_DEC)
#define IS_BPG_DEC(ctx)		((ctx)->codec_mode == MFC_REG_CODEC_BPG_DEC)

/* Encoder codec mode check */
#define IS_H264_ENC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_H264_ENC)
#define IS_MPEG4_ENC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_MPEG4_ENC)
#define IS_H263_ENC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_H263_ENC)
#define IS_VP8_ENC(ctx)		((ctx)->codec_mode == MFC_REG_CODEC_VP8_ENC)
#define IS_HEVC_ENC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_HEVC_ENC)
#define IS_VP9_ENC(ctx)		((ctx)->codec_mode == MFC_REG_CODEC_VP9_ENC)
#define IS_BPG_ENC(ctx)		((ctx)->codec_mode == MFC_REG_CODEC_BPG_ENC)

#define CODEC_NOT_CODED(ctx)	(IS_MPEG4_DEC(ctx) || IS_VC1_DEC(ctx) ||	\
				IS_VC1_RCV_DEC(ctx) || IS_VP9_DEC(ctx))
#define CODEC_INTERLACED(ctx)	(IS_H264_DEC(ctx) || IS_H264_MVC_DEC(ctx) ||	\
				IS_MPEG2_DEC(ctx) || IS_MPEG4_DEC(ctx) ||	\
				IS_VC1_DEC(ctx) || IS_VC1_RCV_DEC(ctx))
#define CODEC_MBAFF(ctx)	(IS_H264_DEC(ctx) || IS_H264_MVC_DEC(ctx))
#define CODEC_MULTIFRAME(ctx)	(IS_MPEG4_DEC(ctx) || IS_VP9_DEC(ctx) ||	\
				IS_FIMV2_DEC(ctx) || IS_FIMV3_DEC(ctx) || IS_FIMV4_DEC(ctx))
#define CODEC_10BIT(ctx)	(IS_HEVC_DEC(ctx) || IS_HEVC_ENC(ctx) ||	\
				IS_VP9_DEC(ctx) || IS_VP9_ENC(ctx) ||		\
				IS_BPG_DEC(ctx) || IS_BPG_ENC(ctx))
#define CODEC_422FORMAT(ctx)	(IS_HEVC_DEC(ctx) || IS_HEVC_ENC(ctx) ||	\
				IS_VP9_DEC(ctx) || IS_VP9_ENC(ctx) ||		\
				IS_BPG_DEC(ctx) || IS_BPG_ENC(ctx))
#define CODEC_HIGH_PERF(ctx)	(IS_H264_DEC(ctx) || IS_H264_MVC_DEC(ctx) || IS_HEVC_DEC(ctx))
#define ON_RES_CHANGE(ctx)	(((ctx)->state >= MFCINST_RES_CHANGE_INIT) &&	\
				 ((ctx)->state <= MFCINST_RES_CHANGE_END))
#define IS_NO_ERROR(err)	((err) == 0 ||		\
				(mfc_get_warn(err)	\
				 == MFC_REG_ERR_SYNC_POINT_NOT_RECEIVED))

#define IS_BUFFER_BATCH_MODE(ctx)	((ctx)->batch_mode == 1)

/* UHD resoluition */
#define MFC_UHD_RES		(3840 * 2160)
#define IS_UHD_RES(ctx)		(((ctx)->crop_width * (ctx)->crop_height) == MFC_UHD_RES)
#define OVER_UHD_ENC60(ctx)	((((ctx)->crop_width * (ctx)->crop_height) == MFC_UHD_RES) && \
				((ctx)->type == MFCINST_ENCODER) &&	\
				((ctx)->framerate / 1000) >= 60)

#define IS_SUPER64_BFRAME(ctx, size, type)	((ctx->is_10bit) && (size >= 2) && (type == 3))

/* Extra information for Decoder */
#define	DEC_SET_DUAL_DPB		(1 << 0)
#define	DEC_SET_DYNAMIC_DPB		(1 << 1)
#define	DEC_SET_LAST_FRAME_INFO		(1 << 2)
#define	DEC_SET_SKYPE_FLAG		(1 << 3)
#define	DEC_SET_HDR10_PLUS		(1 << 4)
#define	DEC_SET_OPERATING_FPS		(1 << 8)
#define	DEC_SET_PRIORITY		(1 << 23)

/* Extra information for Encoder */
#define	ENC_SET_RGB_INPUT		(1 << 0)
#define	ENC_SET_SPARE_SIZE		(1 << 1)
#define	ENC_SET_TEMP_SVC_CH		(1 << 2)
#define	ENC_SET_SKYPE_FLAG		(1 << 3)
#define	ENC_SET_ROI_CONTROL		(1 << 4)
#define	ENC_SET_QP_BOUND_PB		(1 << 5)
#define	ENC_SET_FIXED_SLICE		(1 << 6)
#define	ENC_SET_PVC_MODE		(1 << 7)
#define	ENC_SET_RATIO_OF_INTRA		(1 << 8)
#define	ENC_SET_COLOR_ASPECT		(1 << 9)
#define	ENC_SET_HP_BITRATE_CONTROL	(1 << 10)
#define	ENC_SET_STATIC_INFO		(1 << 11)
#define	ENC_SET_HDR10_PLUS		(1 << 12)
#define	ENC_SET_VP9_PROFILE_LEVEL	(1 << 13)
#define	ENC_SET_DROP_CONTROL		(1 << 14)
#define	ENC_SET_OPERATING_FPS		(1 << 18)
#define	ENC_SET_PRIORITY		(1 << 23)

#define MFC_VER_MAJOR(dev)	((dev->pdata->ip_ver >> 8) & 0xFF)
#define MFC_VER_MINOR(dev)	(dev->pdata->ip_ver & 0xFF)

#define MFC_FEATURE_SUPPORT(dev, f)	((f).support && ((dev)->fw.date >= (f).version))

/* Low memory check */
#define IS_LOW_MEM			(totalram_pages <= ((SZ_1G + SZ_512M) >> PAGE_SHIFT))
#define SZ_600M				(6 * 1024 * 1024)

#endif /* __MFC_COMMON_H */
