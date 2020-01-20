/*
 * drivers/media/platform/exynos/mfc/mfc_reg.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/delay.h>

#include "mfc_reg_api.h"

void mfc_dbg_enable(struct mfc_dev *dev)
{
	mfc_debug(2, "MFC debug info enable\n");
	MFC_WRITEL(0x1, MFC_REG_DBG_INFO_ENABLE);
}

void mfc_dbg_disable(struct mfc_dev *dev)
{
	mfc_debug(2, "MFC debug info disable\n");
	MFC_WRITEL(0x0, MFC_REG_DBG_INFO_ENABLE);
}

void mfc_dbg_set_addr(struct mfc_dev *dev)
{
	struct mfc_ctx_buf_size *buf_size = dev->variant->buf_size->ctx_buf;

	memset((void *)dev->dbg_info_buf.vaddr, 0, buf_size->dbg_info_buf);

	MFC_WRITEL(dev->dbg_info_buf.daddr, MFC_REG_DBG_BUFFER_ADDR);
	MFC_WRITEL(buf_size->dbg_info_buf, MFC_REG_DBG_BUFFER_SIZE);
}

void mfc_otf_set_frame_addr(struct mfc_ctx *ctx, int num_planes)
{
	struct mfc_dev *dev = ctx->dev;
	struct _otf_handle *handle = ctx->otf_handle;
	struct _otf_buf_addr *buf_addr = &handle->otf_buf_addr;
	int index = handle->otf_buf_index;
	int i;

	for (i = 0; i < num_planes; i++) {
		mfc_debug(2, "[OTF][FRAME] set frame buffer[%d], 0x%08llx)\n",
				i, buf_addr->otf_daddr[index][i]);
		MFC_WRITEL(buf_addr->otf_daddr[index][i],
				MFC_REG_E_SOURCE_FIRST_ADDR + (i * 4));
	}
}

void mfc_otf_set_stream_size(struct mfc_ctx *ctx, unsigned int size)
{
	struct mfc_dev *dev = ctx->dev;
	struct _otf_handle *handle = ctx->otf_handle;
	struct _otf_debug *debug = &handle->otf_debug;
	struct mfc_special_buf *buf;

	mfc_debug(2, "[OTF] set stream buffer full size, %u\n", size);
	MFC_WRITEL(size, MFC_REG_E_STREAM_BUFFER_SIZE);

	if (otf_dump && !ctx->is_drm) {
		buf = &debug->stream_buf[debug->frame_cnt];
		mfc_debug(2, "[OTF] set stream addr for debugging\n");
		mfc_debug(2, "[OTF][STREAM] buf[%d] daddr: 0x%08llx\n",
				debug->frame_cnt, buf->daddr);
		MFC_WRITEL(buf->daddr, MFC_REG_E_STREAM_BUFFER_ADDR);
	}
}

void mfc_otf_set_hwfc_index(struct mfc_ctx *ctx, int job_id)
{
	struct mfc_dev *dev = ctx->dev;

	mfc_debug(2, "[OTF] set hwfc index, %d\n", job_id);
	HWFC_WRITEL(job_id, HWFC_ENCODING_IDX);
}

/* Set decoding frame buffer */
int mfc_set_dec_codec_buffers(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	unsigned int i;
	size_t frame_size_mv;
	dma_addr_t buf_addr;
	int buf_size;
	int align_gap;
	struct mfc_raw_info *raw;
	unsigned int reg = 0;

	raw = &ctx->raw_buf;
	buf_addr = ctx->codec_buf.daddr;
	buf_size = ctx->codec_buf.size;

	mfc_debug(2, "[MEMINFO] codec buf 0x%llx size: %d\n", buf_addr, buf_size);
	mfc_debug(2, "Total DPB COUNT: %d, display delay: %d\n",
			dec->total_dpb_count, dec->display_delay);

	/* set decoder DPB size, stride */
	MFC_WRITEL(dec->total_dpb_count, MFC_REG_D_NUM_DPB);
	for (i = 0; i < raw->num_planes; i++) {
		mfc_debug(2, "[FRAME] buf[%d] size: %d, stride: %d\n",
				i, raw->plane_size[i], raw->stride[i]);
		MFC_WRITEL(raw->plane_size[i], MFC_REG_D_FIRST_PLANE_DPB_SIZE + (i * 4));
		MFC_WRITEL(ctx->raw_buf.stride[i],
				MFC_REG_D_FIRST_PLANE_DPB_STRIDE_SIZE + (i * 4));
		if (ctx->is_10bit) {
			MFC_WRITEL(raw->stride_2bits[i], MFC_REG_D_FIRST_PLANE_2BIT_DPB_STRIDE_SIZE + (i * 4));
			MFC_WRITEL(raw->plane_size_2bits[i], MFC_REG_D_FIRST_PLANE_2BIT_DPB_SIZE + (i * 4));
			mfc_debug(2, "[FRAME][10BIT] 2bits buf[%d] size: %d, stride: %d\n",
					i, raw->plane_size_2bits[i], raw->stride_2bits[i]);
		}
	}

	/* set codec buffers */
	MFC_WRITEL(buf_addr, MFC_REG_D_SCRATCH_BUFFER_ADDR);
	MFC_WRITEL(ctx->scratch_buf_size, MFC_REG_D_SCRATCH_BUFFER_SIZE);
	buf_addr += ctx->scratch_buf_size;
	buf_size -= ctx->scratch_buf_size;

	if (IS_H264_DEC(ctx) || IS_H264_MVC_DEC(ctx) || IS_HEVC_DEC(ctx) || IS_BPG_DEC(ctx))
		MFC_WRITEL(ctx->mv_size, MFC_REG_D_MV_BUFFER_SIZE);

	if (IS_VP9_DEC(ctx)){
		MFC_WRITEL(buf_addr, MFC_REG_D_STATIC_BUFFER_ADDR);
		MFC_WRITEL(DEC_STATIC_BUFFER_SIZE, MFC_REG_D_STATIC_BUFFER_SIZE);
		buf_addr += DEC_STATIC_BUFFER_SIZE;
		buf_size -= DEC_STATIC_BUFFER_SIZE;
	}

	if (IS_MPEG4_DEC(ctx) && dec->loop_filter_mpeg4) {
		mfc_debug(2, "Add DPB for loop filter of MPEG4\n");
		for (i = 0; i < NUM_MPEG4_LF_BUF; i++) {
			MFC_WRITEL(buf_addr, MFC_REG_D_POST_FILTER_LUMA_DPB0 + (4 * i));
			buf_addr += ctx->loopfilter_luma_size;
			buf_size -= ctx->loopfilter_luma_size;

			MFC_WRITEL(buf_addr, MFC_REG_D_POST_FILTER_CHROMA_DPB0 + (4 * i));
			buf_addr += ctx->loopfilter_chroma_size;
			buf_size -= ctx->loopfilter_chroma_size;
		}
		reg |= ((dec->loop_filter_mpeg4 & MFC_REG_D_INIT_BUF_OPT_LF_CTRL_MASK)
				<< MFC_REG_D_INIT_BUF_OPT_LF_CTRL_SHIFT);
	}

	reg |= (0x1 << MFC_REG_D_INIT_BUF_OPT_DYNAMIC_DPB_SET_SHIFT);

	if (CODEC_NOT_CODED(ctx)) {
		reg |= (0x1 << MFC_REG_D_INIT_BUF_OPT_COPY_NOT_CODED_SHIFT);
		mfc_debug(2, "Notcoded frame copy mode start\n");
	}
	/* Enable 10bit Dithering when only 8+2 10bit format */
	if (ctx->is_10bit && !ctx->mem_type_10bit) {
		reg |= (0x1 << MFC_REG_D_INIT_BUF_OPT_DITHERING_EN_SHIFT);
		/* 64byte align, It is vaid only for VP9 */
		reg |= (0x1 << MFC_REG_D_INIT_BUF_OPT_STRIDE_SIZE_ALIGN);
	} else {
		/* 16byte align, It is vaid only for VP9 */
		reg &= ~(0x1 << MFC_REG_D_INIT_BUF_OPT_STRIDE_SIZE_ALIGN);
	}

	MFC_WRITEL(reg, MFC_REG_D_INIT_BUFFER_OPTIONS);

	frame_size_mv = ctx->mv_size;
	MFC_WRITEL(dec->mv_count, MFC_REG_D_NUM_MV);
	if (IS_H264_DEC(ctx) || IS_H264_MVC_DEC(ctx) || IS_HEVC_DEC(ctx) || IS_BPG_DEC(ctx)) {
		for (i = 0; i < dec->mv_count; i++) {
			/* To test alignment */
			align_gap = buf_addr;
			buf_addr = ALIGN(buf_addr, 16);
			align_gap = buf_addr - align_gap;
			buf_size -= align_gap;

			MFC_WRITEL(buf_addr, MFC_REG_D_MV_BUFFER0 + i * 4);
			buf_addr += frame_size_mv;
			buf_size -= frame_size_mv;
		}
	}

	mfc_debug(2, "[MEMINFO] codec buf 0x%llx, remained size: %d\n", buf_addr, buf_size);
	if (buf_size < 0) {
		mfc_debug(2, "[MEMINFO] Not enough memory has been allocated\n");
		return -ENOMEM;
	}

	return 0;
}

/* Set encoding ref & codec buffer */
int mfc_set_enc_codec_buffers(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	dma_addr_t buf_addr;
	int buf_size;
	int i;

	mfc_debug_enter();

	buf_addr = ctx->codec_buf.daddr;
	buf_size = ctx->codec_buf.size;

	mfc_debug(2, "[MEMINFO] codec buf 0x%llx, size: %d\n", buf_addr, buf_size);
	mfc_debug(2, "DPB COUNT: %d\n", ctx->dpb_count);

	MFC_WRITEL(buf_addr, MFC_REG_E_SCRATCH_BUFFER_ADDR);
	MFC_WRITEL(ctx->scratch_buf_size, MFC_REG_E_SCRATCH_BUFFER_SIZE);
	buf_addr += ctx->scratch_buf_size;
	buf_size -= ctx->scratch_buf_size;

	/* start address of per buffer is aligned */
	for (i = 0; i < ctx->dpb_count; i++) {
		MFC_WRITEL(buf_addr, MFC_REG_E_LUMA_DPB + (4 * i));
		buf_addr += enc->luma_dpb_size;
		buf_size -= enc->luma_dpb_size;
	}
	for (i = 0; i < ctx->dpb_count; i++) {
		MFC_WRITEL(buf_addr, MFC_REG_E_CHROMA_DPB + (4 * i));
		buf_addr += enc->chroma_dpb_size;
		buf_size -= enc->chroma_dpb_size;
	}
	for (i = 0; i < ctx->dpb_count; i++) {
		MFC_WRITEL(buf_addr, MFC_REG_E_ME_BUFFER + (4 * i));
		buf_addr += enc->me_buffer_size;
		buf_size -= enc->me_buffer_size;
	}

	MFC_WRITEL(buf_addr, MFC_REG_E_TMV_BUFFER0);
	buf_addr += enc->tmv_buffer_size >> 1;
	MFC_WRITEL(buf_addr, MFC_REG_E_TMV_BUFFER1);
	buf_addr += enc->tmv_buffer_size >> 1;
	buf_size -= enc->tmv_buffer_size;

	mfc_debug(2, "[MEMINFO] codec buf 0x%llx, remained size: %d\n", buf_addr, buf_size);
	if (buf_size < 0) {
		mfc_debug(2, "[MEMINFO] Not enough memory has been allocated\n");
		return -ENOMEM;
	}

	mfc_debug_leave();

	return 0;
}

/* Set registers for decoding stream buffer */
int mfc_set_dec_stream_buffer(struct mfc_ctx *ctx, struct mfc_buf *mfc_buf,
		  unsigned int start_num_byte, unsigned int strm_size)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	unsigned int cpb_buf_size;
	dma_addr_t addr;
	int index = -1;

	mfc_debug_enter();

	cpb_buf_size = ALIGN(dec->src_buf_size, STREAM_BUF_ALIGN);

	if (mfc_buf) {
		index = mfc_buf->vb.vb2_buf.index;
		addr = mfc_buf->addr[0][0];
		if (strm_size > set_strm_size_max(cpb_buf_size)) {
			mfc_info_ctx("Decrease strm_size because of %d align: %u -> %u\n",
				STREAM_BUF_ALIGN, strm_size, set_strm_size_max(cpb_buf_size));
			strm_size = set_strm_size_max(cpb_buf_size);
			mfc_buf->vb.vb2_buf.planes[0].bytesused = strm_size;
		}
	} else {
		addr = 0;
	}

	mfc_debug(2, "[BUFINFO] ctx[%d] set src index: %d, addr: 0x%08llx\n",
			ctx->num, index, addr);
	mfc_debug(2, "[STREAM] strm_size: %#x(%d), buf_size: %u, offset: %u\n",
			strm_size, strm_size, cpb_buf_size, start_num_byte);

	if (strm_size == 0)
		mfc_info_ctx("stream size is 0\n");

	MFC_WRITEL(strm_size, MFC_REG_D_STREAM_DATA_SIZE);
	MFC_WRITEL(addr, MFC_REG_D_CPB_BUFFER_ADDR);
	MFC_WRITEL(cpb_buf_size, MFC_REG_D_CPB_BUFFER_SIZE);
	MFC_WRITEL(start_num_byte, MFC_REG_D_CPB_BUFFER_OFFSET);

	if (mfc_buf)
		MFC_TRACE_CTX("Set src[%d] fd: %d, %#llx\n",
				index, mfc_buf->vb.vb2_buf.planes[0].m.fd, addr);

	mfc_debug_leave();
	return 0;
}

void mfc_set_enc_frame_buffer(struct mfc_ctx *ctx,
		struct mfc_buf *mfc_buf, int num_planes)
{
	struct mfc_dev *dev = ctx->dev;
	dma_addr_t addr[3] = { 0, 0, 0 };
	dma_addr_t addr_2bit[2] = { 0, 0 };
	int index, i;

	if (!mfc_buf) {
		mfc_debug(3, "enc zero buffer set\n");
		goto buffer_set;
	}

	index = mfc_buf->vb.vb2_buf.index;
	if (mfc_buf->num_valid_bufs > 0) {
		for (i = 0; i < num_planes; i++) {
			addr[i] = mfc_buf->addr[mfc_buf->next_index][i];
			mfc_debug(2, "[BUFCON][BUFINFO] ctx[%d] set src index:%d, batch[%d], addr[%d]: 0x%08llx\n",
					ctx->num, index, mfc_buf->next_index, i, addr[i]);
		}
		mfc_buf->next_index++;
	} else {
		for (i = 0; i < num_planes; i++) {
			addr[i] = mfc_buf->addr[0][i];
			mfc_debug(2, "[BUFINFO] ctx[%d] set src index:%d, addr[%d]: 0x%08llx\n",
					ctx->num, index, i, addr[i]);
		}
	}

buffer_set:
	for (i = 0; i < num_planes; i++)
		MFC_WRITEL(addr[i], MFC_REG_E_SOURCE_FIRST_ADDR + (i * 4));

	if (ctx->src_fmt->fourcc == V4L2_PIX_FMT_NV12M_S10B ||
		ctx->src_fmt->fourcc == V4L2_PIX_FMT_NV21M_S10B) {
		addr_2bit[0] = addr[0] + NV12N_10B_Y_8B_SIZE(ctx->img_width, ctx->img_height);
		addr_2bit[1] = addr[1] + NV12N_10B_CBCR_8B_SIZE(ctx->img_width, ctx->img_height);

		for (i = 0; i < num_planes; i++) {
			MFC_WRITEL(addr_2bit[i], MFC_REG_E_SOURCE_FIRST_2BIT_ADDR + (i * 4));
			mfc_debug(2, "[BUFINFO][10BIT] ctx[%d] set src 2bit addr[%d]: 0x%08llx\n",
					ctx->num, i, addr_2bit[i]);
		}
	} else if (ctx->src_fmt->fourcc == V4L2_PIX_FMT_NV16M_S10B ||
		ctx->src_fmt->fourcc == V4L2_PIX_FMT_NV61M_S10B) {
		addr_2bit[0] = addr[0] + NV16M_Y_SIZE(ctx->img_width, ctx->img_height);
		addr_2bit[1] = addr[1] + NV16M_CBCR_SIZE(ctx->img_width, ctx->img_height);

		for (i = 0; i < num_planes; i++) {
			MFC_WRITEL(addr_2bit[i], MFC_REG_E_SOURCE_FIRST_2BIT_ADDR + (i * 4));
			mfc_debug(2, "[BUFINFO][10BIT] ctx[%d] set src 2bit addr[%d]: 0x%08llx\n",
					ctx->num, i, addr_2bit[i]);
		}
	}
}

/* Set registers for encoding stream buffer */
int mfc_set_enc_stream_buffer(struct mfc_ctx *ctx,
		struct mfc_buf *mfc_buf)
{
	struct mfc_dev *dev = ctx->dev;
	dma_addr_t addr;
	unsigned int size, offset, index;

	index = mfc_buf->vb.vb2_buf.index;
	addr = mfc_buf->addr[0][0];
	offset = mfc_buf->vb.vb2_buf.planes[0].data_offset;
	size = (unsigned int)vb2_plane_size(&mfc_buf->vb.vb2_buf, 0);
	size = ALIGN(size, 512);

	MFC_WRITEL(addr, MFC_REG_E_STREAM_BUFFER_ADDR); /* 16B align */
	MFC_WRITEL(size, MFC_REG_E_STREAM_BUFFER_SIZE);
	MFC_WRITEL(offset, MFC_REG_E_STREAM_BUFFER_OFFSET);

	mfc_debug(2, "[BUFINFO] ctx[%d] set dst index: %d, addr: 0x%08llx\n",
			ctx->num, index, addr);
	mfc_debug(2, "[STREAM] buf_size: %u, offset: %d\n", size, offset);

	return 0;
}

void mfc_get_enc_frame_buffer(struct mfc_ctx *ctx,
		dma_addr_t addr[], int num_planes)
{
	struct mfc_dev *dev = ctx->dev;
	unsigned long enc_recon_y_addr, enc_recon_c_addr;
	int i, addr_offset;

	addr_offset = MFC_REG_E_ENCODED_SOURCE_FIRST_ADDR;

	for (i = 0; i < num_planes; i++)
		addr[i] = MFC_READL(addr_offset + (i * 4));

	enc_recon_y_addr = MFC_READL(MFC_REG_E_RECON_LUMA_DPB_ADDR);
	enc_recon_c_addr = MFC_READL(MFC_REG_E_RECON_CHROMA_DPB_ADDR);

	mfc_debug(2, "[MEMINFO] recon y: 0x%08lx c: 0x%08lx\n",
			enc_recon_y_addr, enc_recon_c_addr);
}

void mfc_set_enc_stride(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	int i;

	for (i = 0; i < ctx->raw_buf.num_planes; i++) {
		MFC_WRITEL(ctx->raw_buf.stride[i],
				MFC_REG_E_SOURCE_FIRST_STRIDE + (i * 4));
		mfc_debug(2, "[FRAME] enc src plane[%d] stride: %d\n",
				i, ctx->raw_buf.stride[i]);
	}
}

int mfc_set_dynamic_dpb(struct mfc_ctx *ctx, struct mfc_buf *dst_mb)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_raw_info *raw = &ctx->raw_buf;
	int dst_index;
	int i;

	dst_index = dst_mb->vb.vb2_buf.index;
	set_bit(dst_index, &dec->available_dpb);
	dec->dynamic_set = 1 << dst_index;
	mfc_debug(2, "[DPB] ADDING Flag after: 0x%lx\n", dec->available_dpb);

	/* for debugging about black bar detection */
	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->black_bar) && dec->detect_black_bar) {
		for (i = 0; i < raw->num_planes; i++) {
			dec->frame_vaddr[i][dec->frame_cnt] = vb2_plane_vaddr(&dst_mb->vb.vb2_buf, i);
			dec->frame_daddr[i][dec->frame_cnt] = dst_mb->addr[0][i];
			dec->frame_size[i][dec->frame_cnt] = raw->plane_size[i];
			dec->index[i][dec->frame_cnt] = dst_index;
			dec->fd[i][dec->frame_cnt] = dst_mb->vb.vb2_buf.planes[0].m.fd;
		}
		dec->frame_cnt++;
		if (dec->frame_cnt >= 30)
			dec->frame_cnt = 0;
	}

	/* decoder dst buffer CFW PROT */
	mfc_protect_dpb(ctx, dst_mb);

	for (i = 0; i < raw->num_planes; i++) {
		MFC_WRITEL(raw->plane_size[i],
				MFC_REG_D_FIRST_PLANE_DPB_SIZE + i * 4);
		MFC_WRITEL(dst_mb->addr[0][i],
				MFC_REG_D_FIRST_PLANE_DPB0 + (i * 0x100 + dst_index * 4));
		if (ctx->is_10bit)
			MFC_WRITEL(raw->plane_size_2bits[i],
					MFC_REG_D_FIRST_PLANE_2BIT_DPB_SIZE + (i * 4));
		mfc_debug(2, "[BUFINFO][DPB] ctx[%d] set dst index: %d, addr[%d]: 0x%08llx\n",
				ctx->num, dst_index, i, dst_mb->addr[0][i]);
	}

	MFC_TRACE_CTX("Set dst[%d] fd: %d, %#llx / avail %#lx used %#x\n",
			dst_index, dst_mb->vb.vb2_buf.planes[0].m.fd, dst_mb->addr[0][0],
			dec->available_dpb, dec->dynamic_used);

	return 0;
}

void mfc_set_pixel_format(struct mfc_ctx *ctx, unsigned int format)
{
	struct mfc_dev *dev = ctx->dev;
	unsigned int reg = 0;
	unsigned int pix_val;

	if (dev->pdata->P010_decoding && !ctx->is_drm)
		ctx->mem_type_10bit = 1;

	switch (format) {
	case V4L2_PIX_FMT_NV12M:
	case V4L2_PIX_FMT_NV12N:
	case V4L2_PIX_FMT_NV12MT_16X16:
	case V4L2_PIX_FMT_NV16M:
		pix_val = 0;
		break;
	case V4L2_PIX_FMT_NV21M:
	case V4L2_PIX_FMT_NV61M:
		pix_val = 1;
		break;
	case V4L2_PIX_FMT_YVU420M:
		pix_val = 2;
		break;
	case V4L2_PIX_FMT_YUV420M:
	case V4L2_PIX_FMT_YUV420N:
		pix_val = 3;
		break;
	/* For 10bit direct set */
	case V4L2_PIX_FMT_NV12N_10B:
	case V4L2_PIX_FMT_NV12M_S10B:
	case V4L2_PIX_FMT_NV16M_S10B:
		ctx->mem_type_10bit = 0;
		pix_val = 0;
		break;
	case V4L2_PIX_FMT_NV12M_P010:
	case V4L2_PIX_FMT_NV16M_P210:
		ctx->mem_type_10bit = 1;
		pix_val = 0;
		break;
	case V4L2_PIX_FMT_NV21M_S10B:
	case V4L2_PIX_FMT_NV61M_S10B:
		ctx->mem_type_10bit = 0;
		pix_val = 1;
		break;
	case V4L2_PIX_FMT_NV21M_P010:
	case V4L2_PIX_FMT_NV61M_P210:
		ctx->mem_type_10bit = 1;
		pix_val = 1;
		break;
	default:
		pix_val = 0;
		break;
	}
	reg |= pix_val;
	reg |= (ctx->mem_type_10bit << 4);
	MFC_WRITEL(reg, MFC_REG_PIXEL_FORMAT);
	mfc_debug(2, "[FRAME] pixel format: %d, mem_type_10bit for 10bit: %d (reg: %#x)\n",
			pix_val, ctx->mem_type_10bit, reg);
}

void mfc_print_hdr_plus_info(struct mfc_ctx *ctx, struct hdr10_plus_meta *sei_meta)
{
	struct mfc_dev *dev = ctx->dev;
	int num_distribution;
	int i, j;

	if (ctx->type == MFCINST_DECODER)
		mfc_debug(5, "[HDR+] ================= Decoder metadata =================\n");
	else
		mfc_debug(5, "[HDR+] ================= Encoder metadata =================\n");

	mfc_debug(5, "[HDR+] valid: %#x\n", sei_meta->valid);
	mfc_debug(5, "[HDR+] itu t35 country_code: %#x, provider_code %#x, oriented_code: %#x\n",
			sei_meta->t35_country_code, sei_meta->t35_terminal_provider_code,
			sei_meta->t35_terminal_provider_oriented_code);
	mfc_debug(5, "[HDR+] application identifier: %#x, version: %#x, num_windows: %#x\n",
			sei_meta->application_identifier, sei_meta->application_version,
			sei_meta->num_windows);
	mfc_debug(5, "[HDR+] target_maximum_luminance: %#x, target_actual_peak_luminance_flag %#x\n",
			sei_meta->target_maximum_luminance,
			sei_meta->target_actual_peak_luminance_flag);
	mfc_debug(5, "[HDR+] mastering_actual_peak_luminance_flag %#x\n",
			sei_meta->mastering_actual_peak_luminance_flag);

	for (i = 0; i < sei_meta->num_windows; i++) {
		mfc_debug(5, "[HDR+] -------- window[%d] info --------\n", i);
		for (j = 0; j < HDR_MAX_SCL; j++)
			mfc_debug(5, "[HDR+] maxscl[%d] %#x\n", j,
					sei_meta->win_info[i].maxscl[j]);
		mfc_debug(5, "[HDR+] average_maxrgb %#x, num_distribution_maxrgb_percentiles %#x\n",
				sei_meta->win_info[i].average_maxrgb,
				sei_meta->win_info[i].num_distribution_maxrgb_percentiles);
		num_distribution = sei_meta->win_info[i].num_distribution_maxrgb_percentiles;
		for (j = 0; j < num_distribution; j++)
			mfc_debug(5, "[HDR+] percentages[%d] %#x, percentiles[%d] %#x\n", j,
					sei_meta->win_info[i].distribution_maxrgb_percentages[j], j,
					sei_meta->win_info[i].distribution_maxrgb_percentiles[j]);

		mfc_debug(5, "[HDR+] fraction_bright_pixels %#x, tone_mapping_flag %#x\n",
				sei_meta->win_info[i].fraction_bright_pixels,
				sei_meta->win_info[i].tone_mapping_flag);
		if (sei_meta->win_info[i].tone_mapping_flag) {
			mfc_debug(5, "[HDR+] knee point x %#x, knee point y %#x\n",
					sei_meta->win_info[i].knee_point_x,
					sei_meta->win_info[i].knee_point_y);
			mfc_debug(5, "[HDR+] num_bezier_curve_anchors %#x\n",
					sei_meta->win_info[i].num_bezier_curve_anchors);
			for (j = 0; j < HDR_MAX_BEZIER_CURVES / 3; j++)
				mfc_debug(5, "[HDR+] anchors[%d] %#x, [%d] %#x, [%d] %#x\n",
						j * 3,
						sei_meta->win_info[i].bezier_curve_anchors[j * 3],
						j * 3 + 1,
						sei_meta->win_info[i].bezier_curve_anchors[j * 3 + 1],
						j * 3 + 2,
						sei_meta->win_info[i].bezier_curve_anchors[j * 3 + 2]);
		}
		mfc_debug(5, "[HDR+] color_saturation mapping_flag %#x, weight: %#x\n",
				sei_meta->win_info[i].color_saturation_mapping_flag,
				sei_meta->win_info[i].color_saturation_weight);
	}

	mfc_debug(5, "[HDR+] ================================================================\n");

	/* Do not need to dump register */
	if (dev->nal_q_handle)
		if (dev->nal_q_handle->nal_q_state == NAL_Q_STATE_STARTED)
			return;

	if (ctx->type == MFCINST_DECODER)
		print_hex_dump(KERN_ERR, "[HDR+] ", DUMP_PREFIX_ADDRESS, 32, 4,
				dev->regs_base + MFC_REG_D_ST_2094_40_SEI_0, 0x78, false);
	else
		print_hex_dump(KERN_ERR, "[HDR+] ", DUMP_PREFIX_ADDRESS, 32, 4,
				dev->regs_base + MFC_REG_E_ST_2094_40_SEI_0, 0x78, false);
}

void mfc_get_hdr_plus_info(struct mfc_ctx *ctx, struct hdr10_plus_meta *sei_meta)
{
	struct mfc_dev *dev = ctx->dev;
	unsigned int upper_value, lower_value;
	int num_win, num_distribution;
	int i, j;

	sei_meta->valid = 1;

	/* iru_t_t35 */
	sei_meta->t35_country_code = MFC_READL(MFC_REG_D_ST_2094_40_SEI_0) & 0xFF;
	sei_meta->t35_terminal_provider_code = MFC_READL(MFC_REG_D_ST_2094_40_SEI_0) >> 8 & 0xFF;
	upper_value = MFC_READL(MFC_REG_D_ST_2094_40_SEI_0) >> 24 & 0xFF;
	lower_value = MFC_READL(MFC_REG_D_ST_2094_40_SEI_1) & 0xFF;
	sei_meta->t35_terminal_provider_oriented_code = (upper_value << 8) | lower_value;

	/* application */
	sei_meta->application_identifier = MFC_READL(MFC_REG_D_ST_2094_40_SEI_1) >> 8 & 0xFF;
	sei_meta->application_version = MFC_READL(MFC_REG_D_ST_2094_40_SEI_1) >> 16 & 0xFF;

	/* window information */
	sei_meta->num_windows = MFC_READL(MFC_REG_D_ST_2094_40_SEI_1) >> 24 & 0x3;
	num_win = sei_meta->num_windows;
	if (num_win > dev->pdata->max_hdr_win) {
		mfc_err_ctx("[HDR+] num_window(%d) is exceeded supported max_num_window(%d)\n",
				num_win, dev->pdata->max_hdr_win);
		num_win = dev->pdata->max_hdr_win;
		sei_meta->num_windows = num_win;
	}

	/* luminance */
	sei_meta->target_maximum_luminance = MFC_READL(MFC_REG_D_ST_2094_40_SEI_2) & 0x7FFFFFF;
	sei_meta->target_actual_peak_luminance_flag = MFC_READL(MFC_REG_D_ST_2094_40_SEI_2) >> 27 & 0x1;
	sei_meta->mastering_actual_peak_luminance_flag = MFC_READL(MFC_REG_D_ST_2094_40_SEI_22) >> 10 & 0x1;

	/* per window setting */
	for (i = 0; i < num_win; i++) {
		/* scl */
		for (j = 0; j < HDR_MAX_SCL; j++) {
			sei_meta->win_info[i].maxscl[j] =
				MFC_READL(MFC_REG_D_ST_2094_40_SEI_3 + (4 * j)) & 0x1FFFF;
		}
		sei_meta->win_info[i].average_maxrgb =
			MFC_READL(MFC_REG_D_ST_2094_40_SEI_6) & 0x1FFFF;

		/* distribution */
		sei_meta->win_info[i].num_distribution_maxrgb_percentiles =
			MFC_READL(MFC_REG_D_ST_2094_40_SEI_6) >> 17 & 0xF;
		num_distribution = sei_meta->win_info[i].num_distribution_maxrgb_percentiles;
		for (j = 0; j < num_distribution; j++) {
			sei_meta->win_info[i].distribution_maxrgb_percentages[j] =
				MFC_READL(MFC_REG_D_ST_2094_40_SEI_7 + (4 * j)) & 0x7F;
			sei_meta->win_info[i].distribution_maxrgb_percentiles[j] =
				MFC_READL(MFC_REG_D_ST_2094_40_SEI_7 + (4 * j)) >> 7 & 0x1FFFF;
		}

		/* bright pixels */
		sei_meta->win_info[i].fraction_bright_pixels =
			MFC_READL(MFC_REG_D_ST_2094_40_SEI_22) & 0x3FF;

		/* tone mapping */
		sei_meta->win_info[i].tone_mapping_flag =
			MFC_READL(MFC_REG_D_ST_2094_40_SEI_22) >> 11 & 0x1;
		if (sei_meta->win_info[i].tone_mapping_flag) {
			sei_meta->win_info[i].knee_point_x =
				MFC_READL(MFC_REG_D_ST_2094_40_SEI_23) & 0xFFF;
			sei_meta->win_info[i].knee_point_y =
				MFC_READL(MFC_REG_D_ST_2094_40_SEI_23) >> 12 & 0xFFF;
			sei_meta->win_info[i].num_bezier_curve_anchors =
				MFC_READL(MFC_REG_D_ST_2094_40_SEI_23) >> 24 & 0xF;
			for (j = 0; j < HDR_MAX_BEZIER_CURVES / 3; j++) {
				sei_meta->win_info[i].bezier_curve_anchors[j * 3] =
					MFC_READL(MFC_REG_D_ST_2094_40_SEI_24 + (4 * j)) & 0x3FF;
				sei_meta->win_info[i].bezier_curve_anchors[j * 3 + 1] =
					MFC_READL(MFC_REG_D_ST_2094_40_SEI_24 + (4 * j)) >> 10 & 0x3FF;
				sei_meta->win_info[i].bezier_curve_anchors[j * 3 + 2] =
					MFC_READL(MFC_REG_D_ST_2094_40_SEI_24 + (4 * j)) >> 20 & 0x3FF;
			}
		}

		/* color saturation */
		sei_meta->win_info[i].color_saturation_mapping_flag =
			MFC_READL(MFC_REG_D_ST_2094_40_SEI_29) & 0x1;
		if (sei_meta->win_info[i].color_saturation_mapping_flag)
			sei_meta->win_info[i].color_saturation_weight =
				MFC_READL(MFC_REG_D_ST_2094_40_SEI_29) >> 1 & 0x3F;
	}

	if (debug_level >= 5)
		mfc_print_hdr_plus_info(ctx, sei_meta);
}

void mfc_set_hdr_plus_info(struct mfc_ctx *ctx, struct hdr10_plus_meta *sei_meta)
{
	struct mfc_dev *dev = ctx->dev;
	unsigned int reg = 0;
	int num_win, num_distribution;
	int i, j;

	reg = MFC_READL(MFC_REG_E_HEVC_NAL_CONTROL);
	reg &= ~(0x1 << 6);
	reg |= ((sei_meta->valid & 0x1) << 6);
	MFC_WRITEL(reg, MFC_REG_E_HEVC_NAL_CONTROL);

	/* iru_t_t35 */
	reg = 0;
	reg |= (sei_meta->t35_country_code & 0xFF);
	reg |= ((sei_meta->t35_terminal_provider_code & 0xFF) << 8);
	reg |= (((sei_meta->t35_terminal_provider_oriented_code >> 8) & 0xFF) << 24);
	MFC_WRITEL(reg, MFC_REG_E_ST_2094_40_SEI_0);

	/* window information */
	num_win = (sei_meta->num_windows & 0x3);
	if (!num_win || (num_win > dev->pdata->max_hdr_win)) {
		mfc_debug(3, "[HDR+] num_window is only supported till %d\n",
				dev->pdata->max_hdr_win);
		num_win = dev->pdata->max_hdr_win;
		sei_meta->num_windows = num_win;
	}

	/* application */
	reg = 0;
	reg |= (sei_meta->t35_terminal_provider_oriented_code & 0xFF);
	reg |= ((sei_meta->application_identifier & 0xFF) << 8);
	reg |= ((sei_meta->application_version & 0xFF) << 16);
	reg |= ((sei_meta->num_windows & 0x3) << 24);
	MFC_WRITEL(reg, MFC_REG_E_ST_2094_40_SEI_1);

	/* luminance */
	reg = 0;
	reg |= (sei_meta->target_maximum_luminance & 0x7FFFFFF);
	reg |= ((sei_meta->target_actual_peak_luminance_flag & 0x1) << 27);
	MFC_WRITEL(reg, MFC_REG_E_ST_2094_40_SEI_2);

	/* per window setting */
	for (i = 0; i < num_win; i++) {
		/* scl */
		for (j = 0; j < HDR_MAX_SCL; j++) {
			reg = (sei_meta->win_info[i].maxscl[j] & 0x1FFFF);
			MFC_WRITEL(reg, MFC_REG_E_ST_2094_40_SEI_3 + (4 * j));
		}

		/* distribution */
		reg = 0;
		reg |= (sei_meta->win_info[i].average_maxrgb & 0x1FFFF);
		reg |= ((sei_meta->win_info[i].num_distribution_maxrgb_percentiles & 0xF) << 17);
		MFC_WRITEL(reg, MFC_REG_E_ST_2094_40_SEI_6);
		num_distribution = (sei_meta->win_info[i].num_distribution_maxrgb_percentiles & 0xF);
		for (j = 0; j < num_distribution; j++) {
			reg = 0;
			reg |= (sei_meta->win_info[i].distribution_maxrgb_percentages[j] & 0x7F);
			reg |= ((sei_meta->win_info[i].distribution_maxrgb_percentiles[j] & 0x1FFFF) << 7);
			MFC_WRITEL(reg, MFC_REG_E_ST_2094_40_SEI_7 + (4 * j));
		}

		/* bright pixels, luminance */
		reg = 0;
		reg |= (sei_meta->win_info[i].fraction_bright_pixels & 0x3FF);
		reg |= ((sei_meta->mastering_actual_peak_luminance_flag & 0x1) << 10);

		/* tone mapping */
		reg |= ((sei_meta->win_info[i].tone_mapping_flag & 0x1) << 11);
		MFC_WRITEL(reg, MFC_REG_E_ST_2094_40_SEI_22);
		if (sei_meta->win_info[i].tone_mapping_flag & 0x1) {
			reg = 0;
			reg |= (sei_meta->win_info[i].knee_point_x & 0xFFF);
			reg |= ((sei_meta->win_info[i].knee_point_y & 0xFFF) << 12);
			reg |= ((sei_meta->win_info[i].num_bezier_curve_anchors & 0xF) << 24);
			MFC_WRITEL(reg, MFC_REG_E_ST_2094_40_SEI_23);
			for (j = 0; j < HDR_MAX_BEZIER_CURVES / 3; j++) {
				reg = 0;
				reg |= (sei_meta->win_info[i].bezier_curve_anchors[j * 3] & 0x3FF);
				reg |= ((sei_meta->win_info[i].bezier_curve_anchors[j * 3 + 1] & 0x3FF) << 10);
				reg |= ((sei_meta->win_info[i].bezier_curve_anchors[j * 3 + 2] & 0x3FF) << 20);
				MFC_WRITEL(reg, MFC_REG_E_ST_2094_40_SEI_24 + (4 * j));
			}

		}

		/* color saturation */
		if (sei_meta->win_info[i].color_saturation_mapping_flag & 0x1) {
			reg = 0;
			reg |= (sei_meta->win_info[i].color_saturation_mapping_flag & 0x1);
			reg |= ((sei_meta->win_info[i].color_saturation_weight & 0x3F) << 1);
			MFC_WRITEL(reg, MFC_REG_E_ST_2094_40_SEI_29);
		}
	}

	if (debug_level >= 5)
		mfc_print_hdr_plus_info(ctx, sei_meta);
}
