/*
 * drivers/media/platform/exynos/mfc/mfc_cmd.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <trace/events/mfc.h>

#include "mfc_cmd.h"

#include "mfc_perf_measure.h"
#include "mfc_reg_api.h"
#include "mfc_hw_reg_api.h"
#include "mfc_enc_param.h"
#include "mfc_mmcache.h"

#include "mfc_utils.h"
#include "mfc_buf.h"

void mfc_cmd_sys_init(struct mfc_dev *dev,
					enum mfc_buf_usage_type buf_type)
{
	struct mfc_ctx_buf_size *buf_size;
	struct mfc_special_buf *ctx_buf;

	mfc_debug_enter();

	mfc_clean_dev_int_flags(dev);

	buf_size = dev->variant->buf_size->ctx_buf;
	ctx_buf = &dev->common_ctx_buf;
#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	if (buf_type == MFCBUF_DRM)
		ctx_buf = &dev->drm_common_ctx_buf;
#endif
	MFC_WRITEL(ctx_buf->daddr, MFC_REG_CONTEXT_MEM_ADDR);
	MFC_WRITEL(buf_size->dev_ctx, MFC_REG_CONTEXT_MEM_SIZE);

	mfc_cmd_host2risc(dev, MFC_REG_H2R_CMD_SYS_INIT);

	mfc_debug_leave();
}

void mfc_cmd_sleep(struct mfc_dev *dev)
{
	mfc_debug_enter();

	mfc_clean_dev_int_flags(dev);
	mfc_cmd_host2risc(dev, MFC_REG_H2R_CMD_SLEEP);

	mfc_debug_leave();
}

void mfc_cmd_wakeup(struct mfc_dev *dev)
{
	mfc_debug_enter();

	mfc_clean_dev_int_flags(dev);
	mfc_cmd_host2risc(dev, MFC_REG_H2R_CMD_WAKEUP);

	mfc_debug_leave();
}

/* Open a new instance and get its number */
void mfc_cmd_open_inst(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	unsigned int reg;

	mfc_debug_enter();

	/* Preparing decoding - getting instance number */
	mfc_debug(2, "Getting instance number\n");
	mfc_clean_ctx_int_flags(ctx);

	reg = MFC_READL(MFC_REG_CODEC_CONTROL);
	/* Clear OTF_CONTROL[2:1] & OTF_DEBUG[3] */
	reg &= ~(0x7 << 1);
	if (ctx->otf_handle) {
		/* Set OTF_CONTROL[2:1], 0: Non-OTF, 1: OTF+HWFC, 2: OTF only */
		reg |= (0x1 << 1);
		mfc_info_ctx("[OTF] HWFC + OTF enabled\n");
		if (otf_dump && !ctx->is_drm) {
			/* Set OTF_DEBUG[3] for OTF path dump */
			reg |= (0x1 << 3);
			mfc_info_ctx("[OTF] Debugging mode enabled\n");
		}
	}
	MFC_WRITEL(reg, MFC_REG_CODEC_CONTROL);

	mfc_debug(2, "Requested codec mode: %d\n", ctx->codec_mode);
	reg = ctx->codec_mode & MFC_REG_CODEC_TYPE_MASK;
	reg |= (0x1 << MFC_REG_CLEAR_CTX_MEM_SHIFT);
	mfc_debug(2, "Enable to clear context memory: %#x\n", reg);
	MFC_WRITEL(reg, MFC_REG_CODEC_TYPE);

	MFC_WRITEL(ctx->instance_ctx_buf.daddr, MFC_REG_CONTEXT_MEM_ADDR);
	MFC_WRITEL(ctx->instance_ctx_buf.size, MFC_REG_CONTEXT_MEM_SIZE);
	if (ctx->type == MFCINST_DECODER)
		MFC_WRITEL(ctx->dec_priv->crc_enable, MFC_REG_D_CRC_CTRL);

	mfc_cmd_host2risc(dev, MFC_REG_H2R_CMD_OPEN_INSTANCE);

	mfc_debug_leave();
}

/* Close instance */
int mfc_cmd_close_inst(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;

	mfc_debug_enter();

	/* Closing decoding instance  */
	mfc_debug(2, "Returning instance number\n");
	mfc_clean_ctx_int_flags(ctx);
	if (ctx->state == MFCINST_FREE) {
		mfc_err_ctx("ctx already free status\n");
		return -EINVAL;
	}

	MFC_WRITEL(ctx->inst_no, MFC_REG_INSTANCE_ID);

	mfc_cmd_host2risc(dev, MFC_REG_H2R_CMD_CLOSE_INSTANCE);

	mfc_debug_leave();

	return 0;
}

void mfc_cmd_abort_inst(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;

	mfc_clean_ctx_int_flags(ctx);

	MFC_WRITEL(ctx->inst_no, MFC_REG_INSTANCE_ID);
	mfc_cmd_host2risc(dev, MFC_REG_H2R_CMD_NAL_ABORT);
}

void mfc_cmd_dpb_flush(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;

	if (ON_RES_CHANGE(ctx))
		mfc_err_ctx("dpb flush on res change(state:%d)\n", ctx->state);

	mfc_clean_ctx_int_flags(ctx);

	MFC_WRITEL(ctx->inst_no, MFC_REG_INSTANCE_ID);
	mfc_cmd_host2risc(dev, MFC_REG_H2R_CMD_DPB_FLUSH);
}

void mfc_cmd_cache_flush(struct mfc_dev *dev)
{
	mfc_clean_dev_int_flags(dev);
	mfc_cmd_host2risc(dev, MFC_REG_H2R_CMD_CACHE_FLUSH);
}

/* Initialize decoding */
void mfc_cmd_dec_seq_header(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	unsigned int reg = 0;
	int fmo_aso_ctrl = 0;

	mfc_debug_enter();

	mfc_debug(2, "InstNo: %d/%d\n", ctx->inst_no, MFC_REG_H2R_CMD_SEQ_HEADER);
	mfc_debug(2, "BUFs: %08x\n", MFC_READL(MFC_REG_D_CPB_BUFFER_ADDR));

	/*
	 * When user sets desplay_delay to 0,
	 * It works as "display_delay enable" and delay set to 0.
	 * If user wants display_delay disable, It should be
	 * set to negative value.
	 */
	if (dec->display_delay >= 0) {
		reg |= (0x1 << MFC_REG_D_DEC_OPT_DISPLAY_DELAY_EN_SHIFT);
		MFC_WRITEL(dec->display_delay, MFC_REG_D_DISPLAY_DELAY);
	}

	/* FMO_ASO_CTRL - 0: Enable, 1: Disable */
	reg |= ((fmo_aso_ctrl & MFC_REG_D_DEC_OPT_FMO_ASO_CTRL_MASK)
			<< MFC_REG_D_DEC_OPT_FMO_ASO_CTRL_SHIFT);

	reg |= ((dec->idr_decoding & MFC_REG_D_DEC_OPT_IDR_DECODING_MASK)
			<< MFC_REG_D_DEC_OPT_IDR_DECODING_SHIFT);

	/* VC1 RCV: Discard to parse additional header as default */
	if (IS_VC1_RCV_DEC(ctx))
		reg |= (0x1 << MFC_REG_D_DEC_OPT_DISCARD_RCV_HEADER_SHIFT);

	/* conceal control to specific color */
	reg |= (0x4 << MFC_REG_D_DEC_OPT_CONCEAL_CONTROL_SHIFT);

	/* Disable parallel processing if nal_q_parallel_disable was set */
	if (nal_q_parallel_disable)
		reg |= (0x2 << MFC_REG_D_DEC_OPT_PARALLEL_DISABLE_SHIFT);

	/* Realloc buffer for resolution decrease case in NAL QUEUE mode */
	reg |= (0x1 << MFC_REG_D_DEC_OPT_REALLOC_CONTROL_SHIFT);

	/* Parsing all including PPS */
	reg |= (0x1 << MFC_REG_D_DEC_OPT_SPECIAL_PARSING_SHIFT);

	/* Enabe decoding order */
	if (dec->decoding_order)
		reg |= (0x1 << MFC_REG_D_DEC_OPT_DECODING_ORDER_ENABLE);

	MFC_WRITEL(reg, MFC_REG_D_DEC_OPTIONS);

	MFC_WRITEL(MFC_CONCEAL_COLOR, MFC_REG_D_FORCE_PIXEL_VAL);

	if (IS_FIMV1_DEC(ctx)) {
		mfc_debug(2, "Setting FIMV1 resolution to %dx%d\n",
					ctx->img_width, ctx->img_height);
		MFC_WRITEL(ctx->img_width, MFC_REG_D_SET_FRAME_WIDTH);
		MFC_WRITEL(ctx->img_height, MFC_REG_D_SET_FRAME_HEIGHT);
	}

	mfc_set_pixel_format(ctx, ctx->dst_fmt->fourcc);

	reg = 0;
	/* Enable realloc interface if SEI is enabled */
	if (dec->sei_parse)
		reg |= (0x1 << MFC_REG_D_SEI_ENABLE_NEED_INIT_BUFFER_SHIFT);
	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->static_info_dec)) {
		reg |= (0x1 << MFC_REG_D_SEI_ENABLE_CONTENT_LIGHT_SHIFT);
		reg |= (0x1 << MFC_REG_D_SEI_ENABLE_MASTERING_DISPLAY_SHIFT);
	}
	reg |= (0x1 << MFC_REG_D_SEI_ENABLE_RECOVERY_PARSING_SHIFT);
	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->hdr10_plus))
		reg |= (0x1 << MFC_REG_D_SEI_ENABLE_ST_2094_40_SEI_SHIFT);

	MFC_WRITEL(reg, MFC_REG_D_SEI_ENABLE);
	mfc_debug(2, "SEI enable was set, 0x%x\n", MFC_READL(MFC_REG_D_SEI_ENABLE));

	MFC_WRITEL(ctx->inst_no, MFC_REG_INSTANCE_ID);

	if (sfr_dump & MFC_DUMP_DEC_SEQ_START)
		call_dop(dev, dump_regs, dev);

	mfc_cmd_host2risc(dev, MFC_REG_H2R_CMD_SEQ_HEADER);

	mfc_debug_leave();
}

int mfc_cmd_enc_seq_header(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	int ret;

	mfc_debug(2, "++\n");

	ret = mfc_set_enc_params(ctx);
	if (ret) {
		mfc_debug(2, "fail to set enc params\n");
		return ret;
	}

	MFC_WRITEL(ctx->inst_no, MFC_REG_INSTANCE_ID);

	if (reg_test)
		mfc_set_test_params(dev);

	if (sfr_dump & MFC_DUMP_ENC_SEQ_START)
		call_dop(dev, dump_regs, dev);

	mfc_cmd_host2risc(dev, MFC_REG_H2R_CMD_SEQ_HEADER);

	mfc_debug(2, "--\n");

	return 0;
}

int mfc_cmd_dec_init_buffers(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	int ret;

	mfc_set_pixel_format(ctx, ctx->dst_fmt->fourcc);

	mfc_clean_ctx_int_flags(ctx);
	ret = mfc_set_dec_codec_buffers(ctx);
	if (ret) {
		mfc_info_ctx("isn't enough codec buffer size, re-alloc!\n");

		if (dev->has_mmcache && dev->mmcache.is_on_status)
			mfc_invalidate_mmcache(dev);

		mfc_release_codec_buffers(ctx);
		ret = mfc_alloc_codec_buffers(ctx);
		if (ret) {
			mfc_err_ctx("Failed to allocate decoding buffers\n");
			return ret;
		}
		ret = mfc_set_dec_codec_buffers(ctx);
		if (ret) {
			mfc_err_ctx("Failed to alloc frame mem\n");
			return ret;
		}
	}

	MFC_WRITEL(ctx->inst_no, MFC_REG_INSTANCE_ID);

	if (sfr_dump & MFC_DUMP_DEC_INIT_BUFS)
		call_dop(dev, dump_regs, dev);

	mfc_cmd_host2risc(dev, MFC_REG_H2R_CMD_INIT_BUFFERS);

	return ret;
}

int mfc_cmd_enc_init_buffers(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	int ret;

	/*
	 * Header was generated now starting processing
	 * First set the reference frame buffers
	 */
	if (!ctx->codec_buffer_allocated) {
		mfc_info_ctx("there isn't codec buffer, re-alloc!\n");
		ret = mfc_alloc_codec_buffers(ctx);
		if (ret) {
			mfc_err_ctx("Failed to allocate encoding buffers\n");
			mfc_change_state(ctx, MFCINST_ERROR);
			return ret;
		}
	}

	mfc_clean_ctx_int_flags(ctx);
	ret = mfc_set_enc_codec_buffers(ctx);
	if (ret) {
		mfc_info_ctx("isn't enough codec buffer size, re-alloc!\n");

		if (dev->has_mmcache && dev->mmcache.is_on_status)
			mfc_invalidate_mmcache(dev);

		mfc_release_codec_buffers(ctx);
		ret = mfc_alloc_codec_buffers(ctx);
		if (ret) {
			mfc_err_ctx("Failed to allocate encoding buffers\n");
			return ret;
		}
		ret = mfc_set_enc_codec_buffers(ctx);
		if (ret) {
			mfc_err_ctx("Failed to set enc codec buffers\n");
			return ret;
		}
	}

	MFC_WRITEL(ctx->inst_no, MFC_REG_INSTANCE_ID);

	if (sfr_dump & MFC_DUMP_ENC_INIT_BUFS)
		call_dop(dev, dump_regs, dev);

	mfc_cmd_host2risc(dev, MFC_REG_H2R_CMD_INIT_BUFFERS);

	return ret;
}

/* Decode a single frame */
void mfc_cmd_dec_one_frame(struct mfc_ctx *ctx, int last_frame)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	u32 reg = 0;

	mfc_debug(2, "[DPB] Dynamic:0x%08x, Available:0x%lx\n",
			dec->dynamic_set, dec->available_dpb);

	reg = MFC_READL(MFC_REG_D_NAL_START_OPTIONS);
	reg &= ~(0x1 << MFC_REG_D_NAL_START_OPT_BLACK_BAR_SHIFT);
	reg |= ((dec->detect_black_bar & 0x1) << MFC_REG_D_NAL_START_OPT_BLACK_BAR_SHIFT);
	MFC_WRITEL(reg, MFC_REG_D_NAL_START_OPTIONS);
	mfc_debug(3, "[BLACKBAR] black bar detect set: %#x\n", reg);

	MFC_WRITEL(dec->dynamic_set, MFC_REG_D_DYNAMIC_DPB_FLAG_LOWER);
	MFC_WRITEL(0x0, MFC_REG_D_DYNAMIC_DPB_FLAG_UPPER);
	MFC_WRITEL(dec->available_dpb, MFC_REG_D_AVAILABLE_DPB_FLAG_LOWER);
	MFC_WRITEL(0x0, MFC_REG_D_AVAILABLE_DPB_FLAG_UPPER);
	MFC_WRITEL(dec->slice_enable, MFC_REG_D_SLICE_IF_ENABLE);
	MFC_WRITEL(MFC_TIMEOUT_VALUE, MFC_REG_DEC_TIMEOUT_VALUE);

	MFC_WRITEL(ctx->inst_no, MFC_REG_INSTANCE_ID);

	if ((sfr_dump & MFC_DUMP_DEC_NAL_START) && !ctx->check_dump) {
		call_dop(dev, dump_regs, dev);
		ctx->check_dump = 1;
	}

	/*
	 * Issue different commands to instance basing on whether it
	 * is the last frame or not.
	 */
	switch (last_frame) {
	case 0:
		mfc_perf_measure_on(dev);

		mfc_cmd_host2risc(dev, MFC_REG_H2R_CMD_NAL_START);
		break;
	case 1:
		mfc_cmd_host2risc(dev, MFC_REG_H2R_CMD_LAST_FRAME);
		break;
	}

	mfc_debug(2, "Decoding a usual frame\n");
}

/* Encode a single frame */
void mfc_cmd_enc_one_frame(struct mfc_ctx *ctx, int last_frame)
{
	struct mfc_dev *dev = ctx->dev;

	mfc_debug(2, "++\n");

	MFC_WRITEL(ctx->inst_no, MFC_REG_INSTANCE_ID);

	if ((sfr_dump & MFC_DUMP_ENC_NAL_START) && !ctx->check_dump) {
		call_dop(dev, dump_regs, dev);
		ctx->check_dump = 1;
	}

	/*
	 * Issue different commands to instance basing on whether it
	 * is the last frame or not.
	 */
	switch (last_frame) {
	case 0:
		mfc_perf_measure_on(dev);

		mfc_cmd_host2risc(dev, MFC_REG_H2R_CMD_NAL_START);
		break;
	case 1:
		mfc_cmd_host2risc(dev, MFC_REG_H2R_CMD_LAST_FRAME);
		break;
	}

	mfc_debug(2, "--\n");
}
