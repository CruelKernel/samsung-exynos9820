/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * The H/W handling file of Samsung Exynos SMFC Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#include <media/v4l2-ioctl.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-sg.h>

#include "smfc.h"

void smfc_hwconfigure_reset(struct smfc_dev *smfc)
{
	u32 cfg = __raw_readl(smfc->reg + REG_MAIN_JPEG_CNTL);
	cfg &= ~((1 << 29) | 3);
	__raw_writel(cfg, smfc->reg + REG_MAIN_JPEG_CNTL);
	__raw_writel(cfg | (1 << 29), smfc->reg + REG_MAIN_JPEG_CNTL);
}

/*
 * Quantization tables in ZIG-ZAG order provided by IOC/IEC 10918-1 K.1 and K.2
 * for YUV420 and YUV422
 */
static const u8 default_luma_qtbl[SMFC_MCU_SIZE] __aligned(64) = {
	 16,  11,  12,  14,  12,  10,  16,  14,
	 13,  14,  18,  17,  16,  19,  24,  40,
	 26,  24,  22,  22,  24,  49,  35,  37,
	 29,  40,  58,  51,  61,  60,  57,  51,
	 56,  55,  64,  72,  92,  78,  64,  68,
	 87,  69,  55,  56,  80, 109,  81,  87,
	 95,  98, 103, 104, 103,  62,  77, 113,
	121, 112, 100, 120,  92, 101, 103,  99,
};

static const u8 default_chroma_qtbl[SMFC_MCU_SIZE] __aligned(64) = {
	17,  18,  18,  24,  21,  24,  47,  26,
	26,  47,  99,  66,  56,  66,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
};

/* ITU Luminace Huffman Table */
static unsigned int ITU_H_TBL_LEN_DC_LUMINANCE[] = {
	0x01050100, 0x01010101, 0x00000001, 0x00000000
};
static unsigned int ITU_H_TBL_VAL_DC_LUMINANCE[] = {
	0x03020100, 0x07060504, 0x0b0a0908, 0x00000000
};

/* ITU Chrominace Huffman Table */
static unsigned int ITU_H_TBL_LEN_DC_CHROMINANCE[] = {
	0x01010300, 0x01010101, 0x00010101, 0x00000000
};
static unsigned int ITU_H_TBL_VAL_DC_CHROMINANCE[] = {
	0x03020100, 0x07060504, 0x0b0a0908, 0x00000000
};

/* ITU Luminace Huffman Table */
static unsigned int ITU_H_TBL_LEN_AC_LUMINANCE[] = {
	0x03010200, 0x03040203, 0x04040505, 0x7d010000
};

static unsigned int ITU_H_TBL_VAL_AC_LUMINANCE[] = {
	0x00030201, 0x12051104, 0x06413121, 0x07615113,
	0x32147122, 0x08a19181, 0xc1b14223, 0xf0d15215,
	0x72623324, 0x160a0982, 0x1a191817, 0x28272625,
	0x35342a29, 0x39383736, 0x4544433a, 0x49484746,
	0x5554534a, 0x59585756, 0x6564635a, 0x69686766,
	0x7574736a, 0x79787776, 0x8584837a, 0x89888786,
	0x9493928a, 0x98979695, 0xa3a29a99, 0xa7a6a5a4,
	0xb2aaa9a8, 0xb6b5b4b3, 0xbab9b8b7, 0xc5c4c3c2,
	0xc9c8c7c6, 0xd4d3d2ca, 0xd8d7d6d5, 0xe2e1dad9,
	0xe6e5e4e3, 0xeae9e8e7, 0xf4f3f2f1, 0xf8f7f6f5,
	0x0000faf9
};

/* ITU Chrominace Huffman Table */
static u32 ITU_H_TBL_LEN_AC_CHROMINANCE[] = {
	0x02010200, 0x04030404, 0x04040507, 0x77020100
};
static u32 ITU_H_TBL_VAL_AC_CHROMINANCE[] = {
	0x03020100, 0x21050411, 0x41120631, 0x71610751,
	0x81322213, 0x91421408, 0x09c1b1a1, 0xf0523323,
	0xd1726215, 0x3424160a, 0x17f125e1, 0x261a1918,
	0x2a292827, 0x38373635, 0x44433a39, 0x48474645,
	0x54534a49, 0x58575655, 0x64635a59, 0x68676665,
	0x74736a69, 0x78777675, 0x83827a79, 0x87868584,
	0x928a8988, 0x96959493, 0x9a999897, 0xa5a4a3a2,
	0xa9a8a7a6, 0xb4b3b2aa, 0xb8b7b6b5, 0xc3c2bab9,
	0xc7c6c5c4, 0xd2cac9c8, 0xd6d5d4d3, 0xdad9d8d7,
	0xe5e4e3e2, 0xe9e8e7e6, 0xf4f3f2ea, 0xf8f7f6f5,
	0x0000faf9
};

/* to suppress warning or error message about out-of-array-bound */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
/* tables and qunatization tables configuration to H/W for compression */
static inline u32 smfc_calc_quantizers(unsigned int idx, unsigned int factor,
					const unsigned char tbl[])
{
	u32 val = max(min((tbl[idx] * factor + 50) / 100, 255U), 1U);
	val |= max(min((tbl[idx + 1] * factor + 50) / 100, 255U), 1U) << 8;
	val |= max(min((tbl[idx + 2] * factor + 50) / 100, 255U), 1U) << 16;
	val |= max(min((tbl[idx + 3] * factor + 50) / 100, 255U), 1U) << 24;
	return val;
}
#pragma GCC diagnostic pop

static void smfc_hwconfigure_qtable(void __iomem *reg, unsigned int factor,
				    const u8 table[])
{
	size_t i;

	for (i = 0; i < SMFC_MCU_SIZE; i += 4)
		__raw_writel(smfc_calc_quantizers(i, factor, table), reg + i);
}

static void smfc_hwconfigure_custom_qtable(void __iomem *reg, const u8 table[])
{
	size_t i;

	for (i = 0; i < SMFC_MCU_SIZE; i += 4) {
		u32 val;

		val = table[i];
		val |= table[i + 1] << 8;
		val |= table[i + 2] << 16;
		val |= table[i + 3] << 24;
		__raw_writel(val, reg + i);
	}
}

void smfc_hwconfigure_tables(struct smfc_ctx *ctx,
			     unsigned int qfactor, const u8 qtbl[])
{
	size_t i;
	void __iomem *base = ctx->smfc->reg;

	if (qfactor > 0) {
		qfactor = (qfactor < 50) ? 5000 / qfactor : 200 - qfactor * 2;
		smfc_hwconfigure_qtable(base + REG_QTBL_BASE,
					qfactor, default_luma_qtbl);
		smfc_hwconfigure_qtable(base + REG_QTBL_BASE + SMFC_MCU_SIZE,
					qfactor, default_chroma_qtbl);
	} else {
		smfc_hwconfigure_custom_qtable(base + REG_QTBL_BASE, qtbl);
		smfc_hwconfigure_custom_qtable(
					base + REG_QTBL_BASE + SMFC_MCU_SIZE,
					qtbl + SMFC_MCU_SIZE);
	}

	/* Huffman tables */
	for (i = 0; i < 4; i++) {
		__raw_writel(ITU_H_TBL_LEN_DC_LUMINANCE[i],
				base + REG_HTBL_LUMA_DCLEN + i * sizeof(u32));
		__raw_writel(ITU_H_TBL_VAL_DC_LUMINANCE[i],
				base + REG_HTBL_LUMA_DCVAL + i * sizeof(u32));
		__raw_writel(ITU_H_TBL_LEN_DC_CHROMINANCE[i],
				base + REG_HTBL_CHROMA_DCLEN + i * sizeof(u32));
		__raw_writel(ITU_H_TBL_VAL_DC_CHROMINANCE[i],
				base + REG_HTBL_CHROMA_DCVAL + i * sizeof(u32));
		__raw_writel(ITU_H_TBL_LEN_AC_LUMINANCE[i],
				base + REG_HTBL_LUMA_ACLEN + i * sizeof(u32));
		__raw_writel(ITU_H_TBL_LEN_AC_CHROMINANCE[i],
				base + REG_HTBL_CHROMA_ACLEN + i * sizeof(u32));
	}

	for (i = 0; i < ARRAY_SIZE(ITU_H_TBL_VAL_AC_LUMINANCE); i++)
		__raw_writel(ITU_H_TBL_VAL_AC_LUMINANCE[i],
				base + REG_HTBL_LUMA_ACVAL + i * sizeof(u32));

	for (i = 0; i < ARRAY_SIZE(ITU_H_TBL_VAL_AC_CHROMINANCE); i++)
		__raw_writel(ITU_H_TBL_VAL_AC_CHROMINANCE[i],
				base + REG_HTBL_CHROMA_ACVAL + i * sizeof(u32));

	__raw_writel(VAL_MAIN_TABLE_SELECT, base + REG_MAIN_TABLE_SELECT);
	__raw_writel(SMFC_DHT_LEN, base + REG_MAIN_DHT_LEN);
}

void smfc_hwconfigure_2nd_tables(struct smfc_ctx *ctx, unsigned int qfactor)
{
	/* Qunatiazation table 2 and 3 will be used by the secondary image */
	void __iomem *base = ctx->smfc->reg;
	void __iomem *qtblbase = base + REG_QTBL_BASE + SMFC_MCU_SIZE * 2;
	qfactor = (qfactor < 50) ? 5000 / qfactor : 200 - qfactor * 2;

	smfc_hwconfigure_qtable(qtblbase, qfactor, default_luma_qtbl);
	smfc_hwconfigure_qtable(qtblbase + SMFC_MCU_SIZE,
				qfactor, default_chroma_qtbl);
	/* Huffman table for the secondary image is the same as the main image */
	__raw_writel(VAL_SEC_TABLE_SELECT, base + REG_SEC_TABLE_SELECT);
	__raw_writel(SMFC_DHT_LEN, base + REG_SEC_DHT_LEN);
}

void smfc_hwconfigure_tables_for_decompression(struct smfc_ctx *ctx)
{
	void __iomem *base = ctx->smfc->reg;
	void __iomem *qtbl_base = ctx->smfc->reg + REG_QTBL_BASE;
	u32 tblsel = ctx->num_components << 16;
	int i;

	/* Huffman table selector configuration */
	for (i = 0; i < ctx->num_components; i++) {
		u32 val = (ctx->huffman_tables->compsel[i].idx_dc |
			(ctx->huffman_tables->compsel[i].idx_ac << 1)) & 3;
		tblsel |= val << (i * 2 + 4);
	}

	/* quantization table configuration */
	for (i = 0; i < ctx->num_components; i++) {
		if (ctx->quantizer_tables->compsel[i] != INVALID_QTBLIDX) {
			u8 *table = ctx->quantizer_tables->table[i];
			int j;

			for (j = 0; j < SMFC_MCU_SIZE; j += 4) {
				u32 quants;

				quants  = table[j + 0] << 0;
				quants |= table[j + 1] << 8;
				quants |= table[j + 2] << 16;
				quants |= table[j + 3] << 24;
				__raw_writel(quants,
					qtbl_base + SMFC_MCU_SIZE * i + j);
			}
			/* quantization table selector */
			tblsel |= ctx->quantizer_tables->compsel[i] << (i * 2);
		}
	}

	/* Huffman table configuration */
	for (i = 0; i < 4; i++) {
		__raw_writel(ctx->huffman_tables->dc[0].code32[i],
				base + REG_HTBL_LUMA_DCLEN + i * sizeof(u32));
		__raw_writel(ctx->huffman_tables->dc[0].value32[i],
				base + REG_HTBL_LUMA_DCVAL + i * sizeof(u32));
		__raw_writel(ctx->huffman_tables->dc[1].code32[i],
				base + REG_HTBL_CHROMA_DCLEN + i * sizeof(u32));
		__raw_writel(ctx->huffman_tables->dc[1].value32[i],
				base + REG_HTBL_CHROMA_DCVAL + i * sizeof(u32));
		__raw_writel(ctx->huffman_tables->ac[0].code32[i],
				base + REG_HTBL_LUMA_ACLEN + i * sizeof(u32));
		__raw_writel(ctx->huffman_tables->ac[1].code32[i],
				base + REG_HTBL_CHROMA_ACLEN + i * sizeof(u32));
	}

	for (i = 0; i < (SMFC_NUM_AC_HVAL / 4); i++) {
		__raw_writel(ctx->huffman_tables->ac[0].value32[i],
				base + REG_HTBL_LUMA_ACVAL + i * sizeof(u32));
		__raw_writel(ctx->huffman_tables->ac[1].value32[i],
				base + REG_HTBL_CHROMA_ACVAL + i * sizeof(u32));
	}

	__raw_writel(tblsel, base + REG_MAIN_TABLE_SELECT);
}

static void smfc_hwconfigure_image_base(struct smfc_ctx *ctx,
					struct vb2_buffer *vb2buf,
					bool thumbnail)
{
	dma_addr_t addr;
	unsigned int i;
	unsigned int num_buffers = ctx->img_fmt->num_buffers;
	bool multiplane = (num_buffers == ctx->img_fmt->num_planes);
	u32 off = thumbnail ? REG_SEC_IMAGE_BASE : REG_MAIN_IMAGE_BASE;

	if (multiplane) {
		/* Note that this includes a single-plane format such as YUYV */
		for (i = 0; i < num_buffers; i++) {
			addr = vb2_dma_sg_plane_dma_addr(vb2buf,
					thumbnail ? i + num_buffers : i);
			__raw_writel((u32)addr,
				ctx->smfc->reg + REG_IMAGE_BASE(off, i));
		}
	} else {
		u32 width = thumbnail ? ctx->thumb_width : ctx->width;
		u32 height = thumbnail ? ctx->thumb_height : ctx->height;
		addr = vb2_dma_sg_plane_dma_addr(vb2buf, thumbnail ? 1 : 0);

		for (i = 0; i < ctx->img_fmt->num_planes; i++) {
			__raw_writel((u32)addr,
				ctx->smfc->reg + REG_IMAGE_BASE(off, i));
			addr += (width * height * ctx->img_fmt->bpp_pix[i]) / 8;
		}
	}
}

static u32 smfc_hwconfigure_jpeg_base(struct smfc_ctx *ctx,
					struct vb2_buffer *vb2buf,
					u32 offset, bool thumbnail)
{
	dma_addr_t addr;
	u32 off = thumbnail ? REG_SEC_JPEG_BASE : REG_MAIN_JPEG_BASE;

	addr = vb2_dma_sg_plane_dma_addr(vb2buf, thumbnail ? 1 : 0);
	addr += offset;
	__raw_writel((u32)addr, ctx->smfc->reg + off);
	return (u32)addr;
}

/* [hfactor - 1][vfactor - 1]: 444, 422V, 422, 420 */
static u32 smfc_get_jpeg_format(unsigned int hfactor, unsigned int vfactor)
{
	switch ((hfactor << 4) | vfactor) {
	case 0x00: return 0 << 24;
	case 0x11: return 1 << 24;
	case 0x21: return 2 << 24;
	case 0x22: return 3 << 24;
	case 0x12: return 4 << 24;
	case 0x41: return 5 << 24;
	}

	return 2 << 24; /* default: YUV422 */
}

void smfc_hwconfigure_2nd_image(struct smfc_ctx *ctx, bool hwfc_enabled)
{
	struct vb2_buffer *vb2buf_img, *vb2buf_jpg;
	u32 format;

	if (!(ctx->flags & SMFC_CTX_COMPRESS))
		return;

	__raw_writel(ctx->thumb_width | (ctx->thumb_height << 16),
			ctx->smfc->reg + REG_SEC_IMAGE_SIZE);

	vb2buf_img = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
	vb2buf_jpg = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);

	smfc_hwconfigure_image_base(ctx, vb2buf_img, true);
	/*
	 * secondary image stream base is not required because there is no
	 * MAX_COMPRESSED_SIZE register for the secondary image
	 */
	smfc_hwconfigure_jpeg_base(ctx, vb2buf_jpg, 0, true);

	format = ctx->img_fmt->regcfg;
	/*
	 * Chroma subsampling is always 1/2 for both of horizontal and vertical
	 * directions to reduce the compressed size of the secondary image.
	 * If HWFC is enabled, the chroma subsampling is not allowed.
	 */
	if (hwfc_enabled)
		format |= smfc_get_jpeg_format(ctx->img_fmt->chroma_hfactor,
						ctx->img_fmt->chroma_vfactor);
	else
		format |= smfc_get_jpeg_format(2, 2);

	__raw_writel(format, ctx->smfc->reg + REG_SEC_IMAGE_FORMAT);
}

void smfc_hwconfigure_image(struct smfc_ctx *ctx,
			    unsigned int hfactor, unsigned int vfactor)
{
	struct vb2_v4l2_buffer *vb2buf_img, *vb2buf_jpg;
	u32 stream_address;
	u32 format = ctx->img_fmt->regcfg;
	unsigned char num_plane = ctx->img_fmt->num_planes;
	u32 burstlen = 1 << ctx->smfc->devdata->burstlenth_bits;
	unsigned int i;

	if (!(ctx->flags & SMFC_CTX_COMPRESS)) {
		__raw_writel(ctx->width | (ctx->height << 16),
					ctx->smfc->reg + REG_MAIN_IMAGE_SIZE);

		vb2buf_img = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);
		vb2buf_jpg = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
		format |= smfc_get_jpeg_format(hfactor, vfactor);
	} else {
		__raw_writel(ctx->crop.width | (ctx->crop.height << 16),
					ctx->smfc->reg + REG_MAIN_IMAGE_SIZE);

		for (i = 0; i < num_plane; i++) {
			__raw_writel(ctx->crop.po[i],
				ctx->smfc->reg + REG_MAIN_IMAGE_PO_PLANE(i));
			__raw_writel(ctx->crop.so[i],
				ctx->smfc->reg + REG_MAIN_IMAGE_SO_PLANE(i));
		}

		vb2buf_img = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
		vb2buf_jpg = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);
		/*
		 * H/W JPEG does not allow upscaling of chroma components
		 * during compression
		 */
		format |= smfc_get_jpeg_format(
				max_t(unsigned int, hfactor,
					ctx->img_fmt->chroma_hfactor),
				max_t(unsigned int, vfactor,
					ctx->img_fmt->chroma_vfactor));
	}

	smfc_hwconfigure_image_base(ctx, &vb2buf_img->vb2_buf, false);
	__raw_writel(format, ctx->smfc->reg + REG_MAIN_IMAGE_FORMAT);

	stream_address = smfc_hwconfigure_jpeg_base(ctx, &vb2buf_jpg->vb2_buf,
						    ctx->offset_of_sos, false);
	if (!(ctx->flags & SMFC_CTX_COMPRESS)) {
		u32 streamsize = (u32)vb2_plane_size(&vb2buf_jpg->vb2_buf, 0);

		streamsize -= ctx->offset_of_sos;
		streamsize += stream_address & SMFC_ADDR_ALIGN_MASK(burstlen);
		streamsize = ALIGN(streamsize, burstlen);
		streamsize >>= ctx->smfc->devdata->burstlenth_bits;
		__raw_writel(streamsize, ctx->smfc->reg + REG_MAIN_STREAM_SIZE);
	}
}

void smfc_hwconfigure_start(struct smfc_ctx *ctx,
			    unsigned int rst_int, bool hwfc_en)
{
	u32 cfg;
	void __iomem *base = ctx->smfc->reg;

	if (smfc_is_capable(ctx->smfc, V4L2_CAP_EXYNOS_JPEG_B2B_COMPRESSION))
		__raw_writel(!(ctx->flags & SMFC_CTX_B2B_COMPRESS) ? 0 : 1,
				base + REG_SEC_JPEG_CNTL);

	/* configure "Error max compressed size" interrupt */
	cfg = __raw_readl(base + REG_INT_EN);
	if (hwfc_en) {
		/*
		 * Timer interrupt should be disabled if HWFC is enabled.
		 * Instead OTF_EMU_HURRY_THRESHOLD interrupt is used for HWFC
		 * that is asserted when the synchronization between SMFC and
		 * MC-Scaler is broken.
		 */
		cfg &= ~(1 << 5);
		cfg |= 1 << 12;
	}
	__raw_writel(cfg | (1 << 11), base + REG_INT_EN);

	cfg = __raw_readl(base + REG_MAIN_JPEG_CNTL) & ~JPEG_CNTL_CODECON_MASK;
	cfg |= !(ctx->flags & SMFC_CTX_COMPRESS) ?
		JPEG_CNTL_CODECON_DECOMPRESS : JPEG_CNTL_CODECON_COMPRESS;
	cfg |= 1 << 19; /* update huffman table from SFR */
	cfg |= 1 << JPEG_CNTL_INT_EN_SHIFT; /* enable global interrupt */
	cfg |= 1 << 29; /* Release reset */
	if (hwfc_en) /* Enable OTF mode (default: emulation) */
		cfg |= 1 << JPEG_CNTL_HWFC_EN_SHIFT;
	if (rst_int != 0)
		cfg |= (rst_int << 3) | (1 << 2);
	if (!!(ctx->flags & SMFC_CTX_B2B_COMPRESS))
		cfg |= 1 << JPEG_CNTL_BTB_EN_SHIFT; /* back-to-back enable */

	writel(cfg, base + REG_MAIN_JPEG_CNTL);
}

bool smfc_hwstatus_okay(struct smfc_dev *smfc, struct smfc_ctx *ctx)
{
	u32 val, reg;
	u32 val2 = 0;

	/* Disable global interrupt */
	reg = __raw_readl(smfc->reg + REG_MAIN_JPEG_CNTL);
	reg ^= 1 << JPEG_CNTL_INT_EN_SHIFT;
	__raw_writel(reg, smfc->reg + REG_MAIN_JPEG_CNTL);

	val = __raw_readl(smfc->reg + REG_MAIN_INT_STATUS);
	if (smfc_is_capable(smfc, V4L2_CAP_EXYNOS_JPEG_B2B_COMPRESSION))
		val2 = __raw_readl(smfc->reg + REG_SEC_INT_STATUS);

	if (!val && !val2) {
		dev_err(smfc->dev, "Interrupt with no state change\n");
		return false;
	}

	if ((val & ~2)) {
		dev_err(smfc->dev, "Error main interrupt %#010x.\n", val);
		return false;
	}

	if ((val2 & ~2)) {
		dev_err(smfc->dev, "Error secondary interrupt %#010x.\n", val);
		return false;
	}

	if (ctx && !!(ctx->flags & SMFC_CTX_B2B_COMPRESS) && !val2) {
		dev_err(smfc->dev, "Secondary image is not completed\n");
		return false;
	}

	/* reset codec state to compression/decompression disabled */
	reg = __raw_readl(smfc->reg + REG_MAIN_JPEG_CNTL);
	reg &= ~JPEG_CNTL_RESET_MASK,
	__raw_writel(reg, smfc->reg + REG_MAIN_JPEG_CNTL);

	return true;
}

void smfc_dump_registers(struct smfc_dev *smfc)
{
	u32 val;

	/* Register dump based on Istor */
	pr_info("DUMPING REGISTERS OF H/W JPEG...\n");
	pr_info("------------------------------------------------\n");
	/* JPEG_CNTL ~ FIFO_STATUS */
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 4,
			smfc->reg + 0x000, 0xD4, false);
	/* Reading quantization tables */
	val = __raw_readl(smfc->reg + REG_MAIN_TABLE_SELECT);
	__raw_writel(val | SMFC_TABLE_READ_REQ_MASK,
			smfc->reg + REG_MAIN_TABLE_SELECT);
	for (val = 0; val < 512; val++) {
		if (!!(__raw_readl(smfc->reg + REG_MAIN_TABLE_SELECT)
						& SMFC_TABLE_READ_OK_MASK))
			break;
		cpu_relax();
	}

	if ((val == 512) &&
		!(__raw_readl(smfc->reg + REG_MAIN_TABLE_SELECT)
						& SMFC_TABLE_READ_OK_MASK)) {
		pr_info("** FAILED TO READ HUFFMAN and QUANTIZER TABLES **\n");
		return;
	}

	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 4,
			smfc->reg + 0x100, 0x2C0, false);
}
