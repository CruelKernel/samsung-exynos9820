/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * The SFR definition file of Samsung Exynos SMFC Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _MEDIA_EXYNOS_SMFC_REGS_H_
#define _MEDIA_EXYNOS_SMFC_REGS_H_

/********** JPEG STANDARD *****************************************************/
#define SMFC_JPEG_MARKER_LEN 2

#define SMFC_MCU_SIZE	64
#define SMFC_MAX_QTBL_COUNT	4
#define SMFC_NUM_HCODE		16
#define SMFC_NUM_DC_HVAL	16
#define SMFC_NUM_AC_HVAL	172

/********** H/W RESTRICTIONS **************************************************/
#define SMFC_MAX_NUM_COMP	3
#define SMFC_MAX_WIDTH	16368U
#define SMFC_MAX_HEIGHT	16368U
#define SMFC_MIN_WIDTH	8U
#define SMFC_MIN_HEIGHT	8U
#define SMFC_ADDR_ALIGN_MASK(burstlen) ((burstlen) - 1)
#define SMFC_STREAMSIZE_ALIGN 64
#define SMFC_STREAMSIZE_ALIGN_MASK (64 - 1)
#define SMFC_EXTRA_STREAMSIZE(base, burstlen)	\
	(SMFC_STREAMSIZE_ALIGN - ((base) & SMFC_ADDR_ALIGN_MASK(burstlen)))

/********** H/W REGISTERS and DEFAULT VALUES **********************************/
#define REG_MAIN_JPEG_CNTL		0x000
#  define JPEG_CNTL_CODECON_MASK	3
#  define JPEG_CNTL_CODECON_COMPRESS	2
#  define JPEG_CNTL_CODECON_DECOMPRESS	1
#  define JPEG_CNTL_INT_EN_SHIFT	28
#  define JPEG_CNTL_HWFC_EN_SHIFT	30
#  define JPEG_CNTL_BTB_EN_SHIFT	31
#  define JPEG_CNTL_RESET_MASK		\
	((1 <<JPEG_CNTL_HWFC_EN_SHIFT) | (1 << JPEG_CNTL_BTB_EN_SHIFT) | \
	 JPEG_CNTL_CODECON_MASK)

#define REG_INT_EN			0x004
#define REG_TIMER_COUNT			0x008
#define REG_MAIN_INT_STATUS		0x00C
#define REG_MAIN_IMAGE_SIZE		0x014
#define REG_MAIN_JPEG_BASE		0x010
#define REG_MAIN_IMAGE_BASE		0x018
#define REG_MAIN_IMAGE_SO_PLANE1	0x01C
#define REG_MAIN_IMAGE_PO_PLANE1	0x020
#define REG_MAIN_IMAGE_SO_PLANE(n)	(REG_MAIN_IMAGE_SO_PLANE1 + (n) * 12)
#define REG_MAIN_IMAGE_PO_PLANE(n)	(REG_MAIN_IMAGE_PO_PLANE1 + (n) * 12)
#define REG_SEC_JPEG_CNTL		0x080
#define REG_SEC_JPEG_BASE		0x088
#define REG_SEC_IMAGE_BASE		0x090
#define REG_SEC_IMAGE_FORMAT		0x0B8
#define REG_SEC_IMAGE_SIZE		0x08C
#define REG_SEC_INT_STATUS		0x084
#define REG_IMAGE_BASE(off, n)		((off) + (n) * 12)
#define REG_MAIN_IMAGE_FORMAT		0x040
#define REG_MAIN_STREAM_SIZE		0x044
#define REG_SEC_STREAM_SIZE		0x0BC

#define REG_IP_VERSION_NUMBER		0x064

#define REG_MAIN_TABLE_SELECT		0x03C
#define REG_SEC_TABLE_SELECT		0x0B4
/*
 * Component 0: Q-table 0, AC/DC table 0
 * Component 1 and 2: Q-table 1, AC/DC table 1
 */
#define VAL_MAIN_TABLE_SELECT		0xF14
/*
 * Component 0: Q-table 2, AC/DC table 0
 * Component 1 and 2: Q-table 3, AC/DC table 1
 */
#define VAL_SEC_TABLE_SELECT		0xF3E
#define SMFC_TABLE_READ_REQ_MASK	(1 << 13)
#define SMFC_TABLE_READ_OK_MASK		(1 << 12)

/* 64 reigsters for four quantization tables are prepared */
#define REG_QTBL_BASE			0x100
/*
 * Total 112 registers for huffman tables:
 * 0x200 - 4 DC Luma code lengths registers
 * 0x210 - 4 DC Luma values registers
 * 0x220 - 4 DC Chroma code lengths registers
 * 0x230 - 4 DC Chroma values registers
 * 0x240 - 4 AC Luma code lengths registers
 * 0x250 - 44 AC Luma values registers
 * 0x300 - 4 AC Chroma code lengths registers
 * 0x310 - 44 ACChroma values registers
 */
#define REG_HTBL_BASE			0x200
#  define REG_HTBL_LUMA_DCLEN	REG_HTBL_BASE
#  define REG_HTBL_LUMA_DCVAL	(REG_HTBL_LUMA_DCLEN + 4 * sizeof(u32))
#  define REG_HTBL_CHROMA_DCLEN	(REG_HTBL_LUMA_DCVAL + 4 * sizeof(u32))
#  define REG_HTBL_CHROMA_DCVAL	(REG_HTBL_CHROMA_DCLEN + 4 * sizeof(u32))
#  define REG_HTBL_LUMA_ACLEN	(REG_HTBL_CHROMA_DCVAL + 4 * sizeof(u32))
#  define REG_HTBL_LUMA_ACVAL	(REG_HTBL_LUMA_ACLEN + 4 * sizeof(u32))
#  define REG_HTBL_CHROMA_ACLEN	(REG_HTBL_LUMA_ACVAL + 44 * sizeof(u32))
#  define REG_HTBL_CHROMA_ACVAL	(REG_HTBL_CHROMA_ACLEN + 4 * sizeof(u32))
#define REG_MAIN_DHT_LEN		0x04C
#define REG_SEC_DHT_LEN			0x0C4
#define SMFC_DHT_LEN			0x1A2

/* Value definitions of MAIN/SECONDARY_IMAGE_FORMAT */
enum {
	IMGSEL_GRAY,
	IMGSEL_RGB,
	IMGSEL_444,	/* YUV */
	IMGSEL_422,	/* YUV */
	IMGSEL_420,	/* YUV */
	IMGSEL_422V,	/* YUV */
	IMGSEL_411	/* YUV */
};
#define IMGFMT_RGBSHIFT		6
enum {
	RGBSEL_RGB565 = 4,
	RGBSEL_ARGB32,
	RGBSEL_RGB24,
};
#define IMGFMT_444SHIFT		9
#define IMGFMT_422SHIFT		12
#define IMGFMT_420SHIFT		15
#define IMGFMT_422VSHIFT	18
#define IMGFMT_411SHIFT		21
enum {
	YUVSEL_2P = 4,
	YUVSEL_3P = 5,
	YUV422SEL_1P = 4,
	YUV422SEL_2P = 5,
	YUV422SEL_3P = 6,
};

#define IMGFMT_RGBSWAP		(1 << 30)
#define IMGFMT_UVSWAP		(1 << 27)
enum {
	YUVSEL_YUYV = 0,
	YUVSEL_YVYU = 1 << 28,
	YUVSEL_UYVY = 2 << 28,
	YUVSEL_VYUY = 3 << 28,
};

#define SMFC_IMGFMT_RGB565 (IMGSEL_RGB | (RGBSEL_RGB565 << IMGFMT_RGBSHIFT))
#define SMFC_IMGFMT_BGR565 (IMGSEL_RGB | (RGBSEL_RGB565 << IMGFMT_RGBSHIFT)\
				| IMGFMT_RGBSWAP)
#define SMFC_IMGFMT_RGB24  (IMGSEL_RGB | (RGBSEL_RGB24 << IMGFMT_RGBSHIFT))
#define SMFC_IMGFMT_BGR24  (IMGSEL_RGB | (RGBSEL_RGB24 << IMGFMT_RGBSHIFT) \
				| IMGFMT_RGBSWAP)
#define SMFC_IMGFMT_BGRA32  (IMGSEL_RGB | (RGBSEL_ARGB32 << IMGFMT_RGBSHIFT))
#define SMFC_IMGFMT_RGBA32  (IMGSEL_RGB | (RGBSEL_ARGB32 << IMGFMT_RGBSHIFT)\
				| IMGFMT_RGBSWAP)
#define SMFC_IMGFMT_NV12    (IMGSEL_420 | (YUVSEL_2P << IMGFMT_420SHIFT))
#define SMFC_IMGFMT_NV21    (IMGSEL_420 | (YUVSEL_2P << IMGFMT_420SHIFT) \
				| IMGFMT_UVSWAP)
#define SMFC_IMGFMT_YUV420P (IMGSEL_420 | (YUVSEL_3P << IMGFMT_420SHIFT))
#define SMFC_IMGFMT_YVU420P (IMGSEL_420 | (YUVSEL_3P << IMGFMT_420SHIFT) \
				| IMGFMT_UVSWAP)
#define SMFC_IMGFMT_YUYV    (IMGSEL_422 | (YUV422SEL_1P << IMGFMT_422SHIFT) \
				| YUVSEL_YUYV)
#define SMFC_IMGFMT_YVYU    (IMGSEL_422 | (YUV422SEL_1P << IMGFMT_422SHIFT) \
				| YUVSEL_YVYU)
#define SMFC_IMGFMT_UYVY    (IMGSEL_422 | (YUV422SEL_1P << IMGFMT_422SHIFT) \
				| YUVSEL_UYVY)
#define SMFC_IMGFMT_VYUY    (IMGSEL_422 | (YUV422SEL_1P << IMGFMT_422SHIFT) \
				| YUVSEL_VYUY)
#define SMFC_IMGFMT_NV16    (IMGSEL_422 | (YUV422SEL_2P << IMGFMT_422SHIFT))
#define SMFC_IMGFMT_NV61    (IMGSEL_422 | (YUV422SEL_2P << IMGFMT_422SHIFT) \
				| IMGFMT_UVSWAP)
#define SMFC_IMGFMT_YUV422  (IMGSEL_422 | (YUV422SEL_3P << IMGFMT_422SHIFT))
#define SMFC_IMGFMT_YUV444  (IMGSEL_444 | (YUVSEL_3P << IMGFMT_444SHIFT))

#define SMFC_IMGFMT_JPEG444  (1 << 24)
#define SMFC_IMGFMT_JPEG422  (2 << 24)
#define SMFC_IMGFMT_JPEG420  (3 << 24)
#define SMFC_IMGFMT_JPEG422V (4 << 24)
#define SMFC_IMGFMT_JPEG411  (5 << 24)

#endif /* _MEDIA_EXYNOS_SMFC_REGS_H_ */
