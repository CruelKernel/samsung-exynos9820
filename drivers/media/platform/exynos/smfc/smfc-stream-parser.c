/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * The main source file of Samsung Exynos SMFC Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/dma-buf.h>
#include <linux/slab.h>

#include <media/videobuf2-core.h>

#include "smfc.h"

static bool smfc_alloc_tables(struct smfc_ctx *ctx)
{
	int i;

	if (!ctx->quantizer_tables) {
		ctx->quantizer_tables = kmalloc(
				sizeof(*ctx->quantizer_tables), GFP_KERNEL);
		if (!ctx->quantizer_tables)
			return false;
	}
	memset(ctx->quantizer_tables, 0, sizeof(*ctx->quantizer_tables));
	for (i = 0; i < SMFC_MAX_QTBL_COUNT; i++)
		ctx->quantizer_tables->compsel[i] = INVALID_QTBLIDX;

	if (!ctx->huffman_tables) {
		ctx->huffman_tables = kmalloc(
				sizeof(*ctx->huffman_tables), GFP_KERNEL);
		if (!ctx->huffman_tables)
			return false;
	}
	memset(ctx->huffman_tables, 0, sizeof(*ctx->huffman_tables));

	return true;
}

union jpeg_hword_t {
	u16 hword;
	u8 byte[2];
};

static int smfc_get_segment_length(struct smfc_ctx *ctx,
				   unsigned long addr, u8 marker, u16 *length)
{
	union jpeg_hword_t len;
	int ret;

	if (IS_ENABLED(CONFIG_CPU_BIG_ENDIAN)) {
		ret = get_user(len.byte[0], (u16 *)addr++);
		ret |= get_user(len.byte[1], (u16 *)addr);
	} else {
		ret = get_user(len.byte[1], (u16 *)addr++);
		ret |= get_user(len.byte[0], (u16 *)addr);
	}
	if (ret) {
		dev_err(ctx->smfc->dev,
			"Failed to read length 0xFF%02X\n", marker);
		return -EFAULT;
	}

	*length = len.hword;

	return 0;
}

#define __get_u16(pos)				\
({						\
	__typeof__(*(pos)) *__p = (pos);	\
	u16 __v = (*__p++ << 8) & 0xFF00;	\
	__v |= *__p++ & 0xFF;			\
	pos = __p;				\
	__v;					\
})

#define __halfbytes_larger_than(val, maxval) \
			(max((val) & 0xF, ((val) >> 4) & 0xF) > (maxval))

unsigned int smfc_get_num_huffval(u8 *hufflen)
{
	int i;
	unsigned int num = 0;

	for (i = 0; i < SMFC_NUM_HCODE; i++)
		num += hufflen[i];

	return num;
}

static int smfc_parse_dht(struct smfc_ctx *ctx, unsigned long *cursor)
{
	u8 __user *pcursor = (u8 __user *)*cursor;
	unsigned long segend;
	int ret;
	u16 len;

	ret = smfc_get_segment_length(ctx, *cursor, 0xc4, &len);
	if (ret)
		return ret;

	segend = *cursor + len;
	pcursor += 2;

	/* 17 : TcTh, L1...L16 */
	while (((unsigned long)pcursor) < (segend - 17)) {
		u8 *table;
		unsigned int num_values;
		u8 tcth;
		bool dc;

		ret = get_user(tcth, pcursor++);
		if (ret) {
			dev_err(ctx->smfc->dev, "Failed to read TcTh in DHT\n");
			return ret;
		} else if (__halfbytes_larger_than(tcth, 1)) {
			dev_err(ctx->smfc->dev,
					"Unsupported TcTh %#x in DHT\n", tcth);
			return -EINVAL;
		}

		/* HUFFLEN */
		dc = (((tcth >> 4) & 0xF) == 0);
		table = dc ? ctx->huffman_tables->dc[tcth & 1].code
			   : ctx->huffman_tables->ac[tcth & 1].code;
		ret = copy_from_user(table, pcursor, SMFC_NUM_HCODE);
		pcursor += SMFC_NUM_HCODE;
		if (ret) {
			dev_err(ctx->smfc->dev,
					"Failed to read HUFFLEN of %d,%d\n",
					tcth >> 4, tcth & 1);
			return ret;
		}

		num_values = smfc_get_num_huffval(table);
		if ((dc && (num_values > SMFC_NUM_DC_HVAL)) ||
				(!dc && (num_values > SMFC_NUM_AC_HVAL))) {
			dev_err(ctx->smfc->dev,
				"Too many values %u in huffman table %d,%d\n",
				num_values, tcth >> 4, tcth & 1);
			return -EINVAL;
		}

		if (((unsigned long)pcursor + num_values) > segend)
			break;

		/* HUFFVAL */
		table = dc ? ctx->huffman_tables->dc[tcth & 1].value
			   : ctx->huffman_tables->ac[tcth & 1].value;
		ret = copy_from_user(table, pcursor, num_values);
		pcursor += (unsigned long)num_values;
		if (ret) {
			dev_err(ctx->smfc->dev,
				"Failed to read huffval of %d,%d\n",
				tcth >> 4, tcth & 1);
			return -EINVAL;
		}
	}

	if ((*cursor + len) != (unsigned long)pcursor) {
		dev_err(ctx->smfc->dev, "Incorrect DHT length %d\n", len);
		return -EINVAL;
	}

	*cursor += len;

	return 0;
}

static int smfc_parse_dqt(struct smfc_ctx *ctx, unsigned long *cursor)
{
	u8 __user *pcursor = (u8 __user *)*cursor;
	unsigned long segend;
	int ret;
	u16 len;

	ret = smfc_get_segment_length(ctx, *cursor, 0xdb, &len);
	if (ret)
		return ret;

	segend = *cursor + len;
	pcursor += 2;

	/* 17 : TcTh, L1...L16 */
	while ((unsigned long)pcursor < segend) {
		u8 pqtq;

		ret = get_user(pqtq, pcursor++);
		if (ret) {
			dev_err(ctx->smfc->dev, "Failed to read PqTq in DQT\n");
			return ret;
		} else if (pqtq >= SMFC_MAX_QTBL_COUNT) {
			/* Pq should be 0, Tq should be < 4 */
			dev_err(ctx->smfc->dev,
					"Invalid PqTq %02xin DQT\n", pqtq);
			return -EINVAL;
		}

		ret = copy_from_user(ctx->quantizer_tables->table[pqtq],
							pcursor, SMFC_MCU_SIZE);
		pcursor += (unsigned long)SMFC_MCU_SIZE;
		if (ret) {
			dev_err(ctx->smfc->dev,
				"Failed to read %dth Q-Table\n", pqtq);
			return ret;
		}
	}

	*cursor += len;

	return 0;
}

#define SOF0_LENGTH 17 /* Lf+P+Y+X+Nf+Nf*Comp */
static int smfc_parse_frameheader(struct smfc_ctx *ctx, unsigned long *cursor)
{
	u8 *pos;
	int i, ret;
	u8 sof0[SOF0_LENGTH];

	pos = sof0;

	ret = copy_from_user(sof0, (void __user *)*cursor, sizeof(sof0));
	if (ret) {
		dev_err(ctx->smfc->dev, "Failed to read SOF0\n");
		return ret;
	}

	if (__get_u16(pos) != SOF0_LENGTH) {
		dev_err(ctx->smfc->dev, "Unsupported data in SOF0\n");
		return ret;
	}

	if (*pos != 8) { /* bits per sample */
		dev_err(ctx->smfc->dev, "Unsupported bits per sample %d", *pos);
		return -EINVAL;
	}
	pos++;

	ctx->stream_height = __get_u16(pos);
	ctx->stream_width = __get_u16(pos);

	if ((*pos != 3) && (*pos != 1)) { /* Nf: number of components */
		dev_err(ctx->smfc->dev, "Unsupported component count %d", *pos);
		return -EINVAL;
	}

	/* ctx->num_components is not 0 if SOS appeared earlier than SOF0 */
	if ((ctx->num_components != 0) && (ctx->num_components != *pos)) {
		dev_err(ctx->smfc->dev,
			"comp. count differs in Ns(%u) and Nf(%u)\n",
			ctx->num_components, *pos);
		return -EINVAL;
	}

	ctx->num_components = *pos;

	pos++;

	for (i = 0; i < ctx->num_components; i++, pos += 3) {
		u8 h = (pos[1] >> 4) & 0xF;
		u8 v = pos[1] & 0xF;

		if ((pos[0] < 1) || (pos[0] > 4) || (pos[2] > 4)
						|| (h > 4) || (v > 4)) {
			dev_err(ctx->smfc->dev,
				"Invalid component data in SOF0 %02x%02x%02x",
				pos[0], pos[1], pos[2]);
			return -EINVAL;
		}

		if (pos[0] == 1) { /* Luma component */
			ctx->stream_hfactor = h;
			ctx->stream_vfactor = v;
		} else if ((h != 1) || (v != 1)) { /* Chroma component */
			dev_err(ctx->smfc->dev,
				"Unsupported chroma factor %dx%d\n", h, v);
			return -EINVAL;
		}

		ctx->quantizer_tables->compsel[pos[0] - 1] = pos[2];
	}

	*cursor += SOF0_LENGTH;

	return 0;
}

#define SOS_LENGTH 12 /* Ls+Ns+Ns*Comp+Ss+Se+AhAl */
static int smfc_parse_scanheader(struct smfc_ctx *ctx,
				unsigned long streambase, unsigned long *cursor)
{
	u8 *pos;
	int i, ret;
	u8 sos[SOS_LENGTH];

	pos = sos;

	ctx->offset_of_sos = (unsigned int)(*cursor - streambase - SMFC_JPEG_MARKER_LEN);

	ret = copy_from_user(sos, (void __user *)*cursor, sizeof(sos));
	if (ret) {
		dev_err(ctx->smfc->dev, "Failed to read SOS\n");
		return ret;
	}

	if (__get_u16(pos) != SOS_LENGTH) {
		dev_err(ctx->smfc->dev, "Unsupported length of SOS segment.\n");
		return ret;
	}

	if ((*pos != 3) && (*pos != 1)) { /* Ns: number of components */
		dev_err(ctx->smfc->dev, "Unsupported component count %d", *pos);
		return -EINVAL;
	}

	/* ctx->num_components is not 0 if SOF0 appeared earlier than SOS */
	if ((ctx->num_components != 0) && (ctx->num_components != *pos)) {
		dev_err(ctx->smfc->dev,
			"comp. count differs in Nf(%u) and Ns(%u)\n",
			ctx->num_components, *pos);
		return -EINVAL;
	}

	ctx->num_components = *pos;

	pos++;

	for (i = 0; i < ctx->num_components; i++, pos += 2) {
		if ((pos[0] > 3) || __halfbytes_larger_than(pos[1], 1)) {
			dev_err(ctx->smfc->dev,
				"Invalid component %d data %02x%02x in SOS\n",
				i, pos[0], pos[1]);
			return -EINVAL;
		}

		ctx->huffman_tables->compsel[pos[0] - 1].idx_dc =
							(pos[1] >> 4) & 0xF;
		ctx->huffman_tables->compsel[pos[0] - 1].idx_ac = pos[1] & 0xF;
	}

	/*
	 * Skipping checking if all qauntizer tables and huffman tables
	 * specified in SOF0 and SOS, respectively because unspecified tables
	 * are initialized as zero and HWJPEG is capable of handling the cases
	 * during decompression.
	 */

	*cursor += SOS_LENGTH;

	return 0;
}

int smfc_parse_jpeg_header(struct smfc_ctx *ctx, struct vb2_buffer *vb)
{
	int ret;
	union jpeg_hword_t marker;
	u16 len;
	unsigned long streambase = vb->planes[0].m.userptr;
	unsigned long cursor = streambase;
	unsigned long streamend = streambase + vb2_get_plane_payload(vb, 0);

	ctx->num_components = 0;

	if (!smfc_alloc_tables(ctx))
		return -ENOMEM;

	/* the buffer in vb the entire JPEG stream from SOI */

	/* userptr: get_user/copy_from_user to read stream headers */
	/* dmabuf: map the buffer in the kernelspace to read stream headers */

	/* SOI */
	ret = get_user(marker.byte[0], (u16 *)cursor++);
	ret |= get_user(marker.byte[1], (u16 *)cursor++);
	if (ret || (marker.byte[0] != 0xFF) || (marker.byte[1] != 0xD8)) {
		dev_err(ctx->smfc->dev, "SOS maker is not found\n");
		return -EINVAL;
	}

	while (cursor < (streamend - SMFC_JPEG_MARKER_LEN)) {
		ret = get_user(marker.byte[0], (u16 *)cursor++);
		ret |= get_user(marker.byte[1], (u16 *)cursor++);
		if (ret) {
			dev_err(ctx->smfc->dev, "Failed to read JPEG maker\n");
			return -EFAULT;
		}

		if (marker.byte[0] != 0xFF) {
			dev_err(ctx->smfc->dev, "Error found in JPEG stream\n");
			return -EINVAL;
		}

		switch (marker.byte[1]) {
		case 0xC4: /* DHT */
			ret = smfc_parse_dht(ctx, &cursor);
			if (ret)
				return ret;
			break;
		case 0xDB: /* DQT */
			ret = smfc_parse_dqt(ctx, &cursor);
			if (ret)
				return ret;
			break;
		case 0xC0: /* SOF0 */
			ret = smfc_parse_frameheader(ctx, &cursor);
			if (ret)
				return ret;
			break;
		case 0xDA: /**** SOS - THE END OF HEADER PARSING ****/
			return smfc_parse_scanheader(ctx, streambase, &cursor);
		case 0xD9: /* EOI */
			dev_err(ctx->smfc->dev,
				"EOI found during header parsing\n");
			return -EINVAL;
		default: /* error checking */
			if ((marker.byte[1] & 0xF0) == 0xC0) {
				dev_err(ctx->smfc->dev,
					"Unsupported marker 0xFF%02X found\n",
					marker.byte[1]);
				return -EINVAL;
			}

			/* Ignores all other markers */
			ret = smfc_get_segment_length(
					ctx, cursor, marker.byte[1], &len);
			if (ret)
				return ret;

			cursor += len;
		}
	}

	dev_err(ctx->smfc->dev, "SOS is not found in the stream\n");

	return -EINVAL;
}
