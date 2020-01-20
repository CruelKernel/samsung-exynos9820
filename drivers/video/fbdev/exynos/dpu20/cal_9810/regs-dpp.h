/* linux/drivers/video/fbdev/dpu_9810/regs-dpp.h
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Register definition file for Samsung dpp driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef DPP_REGS_H_
#define DPP_REGS_H_


/****************************[ DPU ]****************************/

/*
 * 1 - DMA.base
 *  : 0x1607_0000
 */
#define DPU_DMA_VERSION			0x0000

#define DPU_DMA_QCH_EN				0x000C
#define DMA_QCH_EN				(1 << 0)

#define DPU_DMA_SWRST				0x0010
#define DMA_CH2_SWRST				(1 << 3)
#define DMA_CH1_SWRST				(1 << 2)
#define DMA_CH0_SWRST				(1 << 1)
#define DMA_ALL_SWRST				(1 << 0)
#define DMA_CH_SWRST(_ch)			(1 << ((_ch)))

#define DPU_DMA_GLB_CGEN			0x0014
#define DMA_SFR_CGEN(_v)			((_v) << 31)
#define DMA_SFR_CGEN_MASK			(1 << 31)
#define DMA_INT_CGEN(_v)			((_v) << 0)
#define DMA_INT_CGEN_MASK			(0x1FFFFFFF << 0)
#define DMA_ALL_CGEN_MASK			(0x9FFFFFFF)

#define DPU_DMA_TEST_PATTERN0_3		0x0020
#define DPU_DMA_TEST_PATTERN0_2		0x0024
#define DPU_DMA_TEST_PATTERN0_1		0x0028
#define DPU_DMA_TEST_PATTERN0_0		0x002C
#define DPU_DMA_TEST_PATTERN1_3		0x0030
#define DPU_DMA_TEST_PATTERN1_2		0x0034
#define DPU_DMA_TEST_PATTERN1_1		0x0038
#define DPU_DMA_TEST_PATTERN1_0		0x003C

/* _n: [0,7], _v: [0x0, 0xF] */
#define DPU_DMA_IN_QOS_LUT07_00			0x0070
#define DPU_DMA_IN_QOS_LUT15_08			0x0074
#define DPU_DMA_IN_QOS_LUT(_n, _v)		((_v) << (4*(_n)))
#define DPU_DMA_IN_QOS_LUT_MASK(_n)	(0xF << (4*(_n)))

#define DPU_DMA_PERFORMANCE_CON0			0x0078
#define DPU_DMA_DEGRADATION_TIME(_v)		((_v) << 16)
#define DPU_DMA_DEGRADATION_TIME_MASK		(0xFFFF << 16)
#define DPU_DMA_IN_IC_MAX_DEG(_v)		((_v) << 4)
#define DPU_DMA_IN_IC_MAX_DEG_MASK		(0x7F << 4)
#define DPU_DMA_DEGRADATION_EN			(1 << 0)

/*
 * Recovery constraint
 * only AFBC &  only EVT1
 */
#define DPU_DMA_RECOVERY_NUM_CTRL			0x0080
#define DPU_DMA_RECOVERY_NUM(_v)		((_v) << 0)
#define DPU_DMA_RECOVERY_NUM_MASK		(0x7FFFFFFF << 0)

#define DPU_DMA_PSLV_ERR_CTRL		0x0090
#define DPU_DMA_PSLV_ERR_EN			(1 << 0)

#define DPU_DMA_DEBUG_CONTROL			0x0100
#define DPU_DMA_DEBUG_CONTROL_SEL(_v)		((_v) << 16)
#define DPU_DMA_DEBUG_CONTROL_EN		(0x1 << 0)

#define DPU_DMA_DEBUG_DATA			0x0104

/*
 * 1.1 - IDMA Register
 * < DMA.offset >
 *  G0      G1      VG0     VG1     VGF0    VGRF
 *  0x1000  0x2000  0x3000  0x4000  0x5000  0x6000
 */
#define IDMA_ENABLE				0x0000
#define IDMA_SRSET				(1 << 24)
#define IDMA_SRAM_CLOCK_GATE_EN		(1 << 9)
#define IDMA_ALL_CLOCK_GATE_EN_MASK		(0x1 << 9)
#define IDMA_SFR_UPDATE_FORCE			(1 << 4)
#define IDMA_OP_STATUS				(1 << 2)
#define OP_STATUS_IDLE				(0)
#define OP_STATUS_BUSY				(1)
#define IDMA_INSTANT_OFF_PENDING	(1 << 1)
#define INSTANT_OFF_PENDING			(1)
#define INSTANT_OFF_NOT_PENDING		(0)

#define IDMA_IRQ				0x0004
#define IDMA_MO_CONFLICT_IRQ			(1 << 24)
#define IDMA_AFBC_TIMEOUT_IRQ			(1 << 23)
#define IDMA_RECOVERY_START_IRQ		(1 << 22)
#define IDMA_CONFIG_ERROR			(1 << 21)
#define IDMA_LOCAL_HW_RESET_DONE		(1 << 20)
#define IDMA_READ_SLAVE_ERROR			(1 << 19)
#define IDMA_STATUS_DEADLOCK_IRQ		(1 << 17)
#define IDMA_STATUS_FRAMEDONE_IRQ		(1 << 16)
#define IDMA_ALL_IRQ_CLEAR			(0x1fb << 16)
#define IDMA_MO_CONFLICT_MASK		(1 << 9)
#define IDMA_AFBC_TIMEOUT_MASK		(1 << 8)
#define IDMA_RECOVERY_START_MASK		(1 << 7)
#define IDMA_CONFIG_ERROR_MASK		(1 << 6)
#define IDMA_LOCAL_HW_RESET_DONE_MASK	(1 << 5)
#define IDMA_READ_SLAVE_ERROR_MASK		(1 << 4)
#define IDMA_IRQ_DEADLOCK_MASK		(1 << 2)
#define IDMA_IRQ_FRAMEDONE_MASK		(1 << 1)
#define IDMA_ALL_IRQ_MASK			(0x1fb << 1)
#define IDMA_IRQ_ENABLE			(1 << 0)

#define IDMA_IN_CON				0x0008
#define IDMA_VR_MODE_EN				(1 << 31)
#define IDMA_IN_IC_MAX(_v)			((_v) << 19)
#define IDMA_IN_IC_MAX_MASK			(0xff << 19)
#define IDMA_IMG_FORMAT(_v)			((_v) << 11)
#define IDMA_IMG_FORMAT_MASK			(0x1f << 11)
#define IDMA_IMG_FORMAT_ARGB8888		(0)
#define IDMA_IMG_FORMAT_ABGR8888		(1)
#define IDMA_IMG_FORMAT_RGBA8888		(2)
#define IDMA_IMG_FORMAT_BGRA8888		(3)
#define IDMA_IMG_FORMAT_XRGB8888		(4)
#define IDMA_IMG_FORMAT_XBGR8888		(5)
#define IDMA_IMG_FORMAT_RGBX8888		(6)
#define IDMA_IMG_FORMAT_BGRX8888		(7)
#define IDMA_IMG_FORMAT_RGB565			(8)
#define IDMA_IMG_FORMAT_BGR565			(9)
#define IDMA_IMG_FORMAT_ARGB1555		(12)
#define IDMA_IMG_FORMAT_ARGB4444		(13)
#define IDMA_IMG_FORMAT_ARGB2101010		(16)
#define IDMA_IMG_FORMAT_ABGR2101010		(17)
#define IDMA_IMG_FORMAT_RGBA2101010		(18)
#define IDMA_IMG_FORMAT_BGRA2101010		(19)
#define IDMA_IMG_FORMAT_YUV420_2P		(24)
#define IDMA_IMG_FORMAT_YVU420_2P		(25)
#define IDMA_IMG_FORMAT_YUV420_8P2		(26)
#define IDMA_IMG_FORMAT_YVU420_8P2		(27)
#define IDMA_IMG_FORMAT_YUV420_P010		(29)
#define IDMA_IMG_FORMAT_YVU420_P010		(28)

#define IDMA_ROTATION(_v)		((_v) << 8)
#define IDMA_ROTATION_MASK		(7 << 8)
#define IDMA_ROTATION_X_FLIP		(1 << 8)
#define IDMA_ROTATION_Y_FLIP		(2 << 8)
#define IDMA_ROTATION_180		(3 << 8)
#define IDMA_ROTATION_90		(4 << 8)
#define IDMA_ROTATION_90_X_FLIP		(5 << 8)
#define IDMA_ROTATION_90_Y_FLIP		(6 << 8)
#define IDMA_ROTATION_270		(7 << 8)

#define IDMA_IN_FLIP(_v)			((_v) << 8)
#define IDMA_IN_FLIP_MASK			(0x3 << 8)
#define IDMA_AFBC_EN				(1 << 7)
#define IDMA_AFBC_TO_EN			(1 << 6)
#define IDMA_IN_CHROMINANCE_STRIDE_SEL	(1 << 4)
#define IDMA_BLOCK_EN				(1 << 3)

#define IDMA_OUT_CON				0x000C
#define IDMA_OUT_FRAME_ALPHA(_v)		((_v) << 24)
#define IDMA_OUT_FRAME_ALPHA_MASK		(0xff << 24)

#define IDMA_SRC_SIZE				0x0010
#define IDMA_SRC_HEIGHT(_v)			((_v) << 16)
#define IDMA_SRC_HEIGHT_MASK			(0x3FFF << 16)
#define IDMA_SRC_WIDTH(_v)			((_v) << 0)
#define IDMA_SRC_WIDTH_MASK			(0xFFFF << 0)

#define IDMA_SRC_OFFSET				0x0014
#define IDMA_SRC_OFFSET_Y(_v)			((_v) << 16)
#define IDMA_SRC_OFFSET_Y_MASK		(0x1FFF << 16)
#define IDMA_SRC_OFFSET_X(_v)			((_v) << 0)
#define IDMA_SRC_OFFSET_X_MASK		(0x1FFF << 0)

#define IDMA_IMG_SIZE				0x0018
#define IDMA_IMG_HEIGHT(_v)			((_v) << 16)
#define IDMA_IMG_HEIGHT_MASK			(0x1FFF << 16)
#define IDMA_IMG_WIDTH(_v)			((_v) << 0)
#define IDMA_IMG_WIDTH_MASK			(0x1FFF << 0)

#define IDMA_CHROMINANCE_STRIDE		0x0020
#define IDMA_CHROMA_STRIDE(_v)		((_v) << 0)
#define IDMA_CHROMA_STRIDE_MASK		(0xFFFF << 0)

#define IDMA_BLOCK_OFFSET			0x0024
#define IDMA_BLK_OFFSET_Y(_v)			((_v) << 16)
#define IDMA_BLK_OFFSET_Y_MASK		(0x1FFF << 16)
#define IDMA_BLK_OFFSET_X(_v)			((_v) << 0)
#define IDMA_BLK_OFFSET_X_MASK		(0x1FFF << 0)

#define IDMA_BLOCK_SIZE				0x0028
#define IDMA_BLK_HEIGHT(_v)			((_v) << 16)
#define IDMA_BLK_HEIGHT_MASK			(0x1FFF << 16)
#define IDMA_BLK_WIDTH(_v)			((_v) << 0)
#define IDMA_BLK_WIDTH_MASK			(0x1FFF << 0)

#define IDMA_2BIT_STRIDE			0x0030
#define IDMA_CHROMA_2B_STRIDE(_v)		((_v) << 16)
#define IDMA_CHROMA_2B_STRIDE_MASK		(0xFFFF << 16)
#define IDMA_LUMA_2B_STRIDE(_v)		((_v) << 0)
#define IDMA_LUMA_2B_STRIDE_MASK		(0xFFFF << 0)


#define IDMA_IN_BASE_ADDR_Y			0x0040
#define IDMA_IN_BASE_ADDR_C			0x0044

#define IDMA_IN_BASE_ADDR_Y2			0x0048
#define IDMA_IN_BASE_ADDR_C2			0x004C

#define IDMA_DEADLOCK_NUM			0x0050
#define IDMA_DEADLOCK_VAL(_v)			((_v) << 1)
#define IDMA_DEADLOCK_VAL_MASK		(0x7FFFFFFF << 1)
#define IDMA_DEADLOCK_EN			(1 << 0)

#define IDMA_BUS_CON				0x0054

#define IDMA_DYNAMIC_GATING_EN		0x0058
#define IDMA_DG_EN(_n, _v)			((_v) << (_n))
#define IDMA_DG_EN_MASK(_n)			(1 << (_n))
#define IDMA_DG_EN_ALL				(0x7FFFFFF << 0)

/* should be applied on EVT1 */
#define IDMA_RECOVERY_CTRL			0x005C
#define IDMA_RECOVERY_EN			(1 << 0)

#define IDMA_DEBUG_CONTROL			0x0060
#define IDMA_DEBUG_CONTROL_SEL(_v)		((_v) << 16)
#define IDMA_DEBUG_CONTROL_EN			(0x1 << 0)

#define IDMA_DEBUG_DATA				0x0064

#define IDMA_IN_REQ_DEST			0x0068
#define IDMA_IN_REG_DEST_SEL(_v)			((_v) << 0)
#define IDMA_IN_REG_DEST_SEL_MASK	(0x3 << 0)

#define IDMA_CFG_ERR_STATE			0x0870
#define IDMA_CFG_ERR_SRC_WIDTH		(1 << 10)
#define IDMA_CFG_ERR_CHROM_STRIDE		(1 << 9)
#define IDMA_CFG_ERR_BASE_ADDR_Y		(1 << 8)
#define IDMA_CFG_ERR_BASE_ADDR_C		(1 << 7)
#define IDMA_CFG_ERR_IMG_WIDTH_AFBC		(1 << 6)
#define IDMA_CFG_ERR_IMG_WIDTH		(1 << 5)
#define IDMA_CFG_ERR_IMG_HEIGHT_ROTATION	(1 << 4)
#define IDMA_CFG_ERR_IMG_HEIGHT		(1 << 3)
#define IDMA_CFG_ERR_BLOCKING			(1 << 2)
#define IDMA_CFG_ERR_SRC_OFFSET_X		(1 << 1)
#define IDMA_CFG_ERR_SRC_OFFSET_Y		(1 << 0)
#define IDMA_CFG_ERR_GET(_v)			(((_v) >> 0) & 0x7FF)

/*
 * 1.2 - ODMA(WB) SFR
 * < DMA.offset >
 *  WB = 0x7000
 */
#define ODMA_ENABLE				0x0000
#define ODMA_SRSET				(1 << 24)
#define ODMA_SFR_CLOCK_GATE_EN		(1 << 10)
#define ODMA_SRAM_CLOCK_GATE_EN		(1 << 9)
#define ODMA_ALL_CLOCK_GATE_EN_MASK		(0x3 << 9)
#define ODMA_FRAME_START_FORCE		(1 << 5)
#define ODMA_SFR_UPDATE_FORCE			(1 << 4)
#define ODMA_OP_STATUS				(1 << 2)

#define ODMA_IRQ				0x0004
#define ODMA_CONFIG_ERROR			(1 << 28)
#define ODMA_SLICE_DONE(_n)			(1 << (21 + (_n)))
#define ODMA_ALL_SLICE_DONE_CLEAR		(0x7F << 21)
#define ODMA_LOCAL_HW_RESET_DONE		(1 << 20)
#define ODMA_WRITE_SLAVE_ERROR		(1 << 19)
#define ODMA_STATUS_DEADLOCK_IRQ		(1 << 17)
#define ODMA_STATUS_FRAMEDONE_IRQ		(1 << 16)
#define ODMA_ALL_IRQ_CLEAR			(0x1FFB << 16)

#define ODMA_CONFIG_ERROR_MASK		(1 << 13)
#define ODMA_SLICE_DONE_MASK(_n)		(1 << (6 + (_n)))
#define ODMA_ALL_SLICE_DONE_MASK		(0x7F << 6)
#define ODMA_LOCAL_HW_RESET_DONE_MASK	(1 << 5)
#define ODMA_WRITE_SLAVE_ERROR_MASK		(1 << 4)
#define ODMA_IRQ_DEADLOCK_MASK		(1 << 2)
#define ODMA_IRQ_FRAMEDONE_MASK		(1 << 1)
#define ODMA_ALL_IRQ_MASK			(0x1FFB << 1)
#define ODMA_IRQ_ENABLE			(1 << 0)

#define ODMA_CHROMINANCE_STRIDE		0x0020
#define ODMA_CHROMA_STRIDE(_v)		((_v) << 0)
#define ODMA_CHROMA_STRIDE_MASK		(0xFFFF << 0)

#define ODMA_PERFORMANCE_CON0			0x0030
#define ODMA_DEGRADATION_TIME(_v)		((_v) << 16)
#define ODMA_DEGRADATION_TIME_MASK		(0xFFFF << 16)
#define ODMA_DEGRADATION_EN			(1 << 15)
#define ODMA_IN_IC_MAX_DEG(_v)		((_v) << 0)
#define ODMA_IN_IC_MAX_DEG_MASK		(0x7F << 0)

#define ODMA_OUT_CON0				0x004C
#define ODMA_IN_IC_MAX(_v)			((_v) << 19)
#define ODMA_IN_IC_MAX_MASK			(0x7f << 19)
#define ODMA_IMG_FORMAT(_v)			((_v) << 11)
#define ODMA_IMG_FORMAT_MASK			(0x1f << 11)
#define ODMA_IN_CHROMINANCE_STRIDE_SEL	(1 << 4)

#define ODMA_OUT_CON1				0x0050
#define ODMA_OUT_FRAME_ALPHA(_v)		((_v) << 24)
#define ODMA_OUT_FRAME_ALPHA_MASK		(0xff << 24)

#define ODMA_DST_SIZE				0x0054
#define ODMA_DST_HEIGHT(_v)			((_v) << 16)
#define ODMA_DST_HEIGHT_MASK			(0x3FFF << 16)
#define ODMA_DST_WIDTH(_v)			((_v) << 0)
#define ODMA_DST_WIDTH_MASK			(0x3FFF << 0)

#define ODMA_DST_OFFSET			0x0058
#define ODMA_DST_OFFSET_Y(_v)			((_v) << 16)
#define ODMA_DST_OFFSET_Y_MASK		(0x1FFF << 16)
#define ODMA_DST_OFFSET_X(_v)			((_v) << 0)
#define ODMA_DST_OFFSET_X_MASK		(0x1FFF << 0)

#define ODMA_OUT_IMG_SIZE			0x005C
#define ODMA_OUT_IMG_HEIGHT(_v)		((_v) << 16)
#define ODMA_OUT_IMG_HEIGHT_MASK		(0x1FFF << 16)
#define ODMA_OUT_IMG_WIDTH(_v)		((_v) << 0)
#define ODMA_OUT_IMG_WIDTH_MASK		(0x1FFF << 0)

#define ODMA_OUT_QOS_LUT07_00			0x0060
#define ODMA_OUT_QOS_LUT15_08			0x0064
#define ODMA_OUT_QOS_LUT(_n, _v)		((_v) << (4*(_n)))
#define ODMA_OUT_QOS_LUT_MASK(_n)		(0xF << (4*(_n)))

#define ODMA_IN_BASE_ADDR_Y			0x0074
#define ODMA_IN_BASE_ADDR_C			0x0094

#define ODMA_SLICE0_BYTE_CNT			0x0100
#define ODMA_SLICE1_BYTE_CNT			0x0104
#define ODMA_SLICE2_BYTE_CNT			0x0108
#define ODMA_SLICE3_BYTE_CNT			0x010C
#define ODMA_SLICE4_BYTE_CNT			0x0110
#define ODMA_SLICE5_BYTE_CNT			0x0114
#define ODMA_SLICE6_BYTE_CNT			0x0118
#define ODMA_FRAME_BYTE_CNT			0x011C
#define ODMA_SLICE_BYTE_CNT(_n)		(0x0100 + ((_n) * 0x4))

#define ODMA_USB_TV_WB_CON			0x0120
#define ODMA_USB_WB_PATH_SEL			(1 << 3)
#define USB_WB_PATH_MEM			(0)
#define USB_WB_PATH_OTF			(1)
#define ODMA_USB_WB_EN				(1 << 2)

#define ODMA_DEADLOCK_NUM			0x0300
#define ODMA_DEADLOCK_VAL(_v)			((_v) << 1)
#define ODMA_DEADLOCK_VAL_MASK		(0x7FFFFFFF << 1)
#define ODMA_DEADLOCK_EN			(1 << 0)

#define ODMA_BUS_CON				0x0304

/* _n: [0,4], v: [0,1] */
#define ODMA_DYNAMIC_GATING_EN		0x0354
#define ODMA_DG_EN(_n, _v)			((_v) << (_n))
#define ODMA_DG_EN_MASK(_n)			(1 << (_n))
#define ODMA_DG_EN_ALL				(0x7FF << 0) /* 9820 */

#define ODMA_CHAN_CONTROL			0x0360
#define ODMA_CHAN_DATA				0x0364

#define ODMA_CFG_ERR_STATE			0x0C00
#define ODMA_CFG_ERR_GET(_v)			(((_v) >> 0) & 0x7FF)
// ADD field def

/*
 * 2 - DPU_WB_MUX.base
 *  Non-secure        : 0x1289_0000
 */
#define DPU_WB_ENABLE				0x0000
#define WB_SRSET				(1 << 24)
#define WB_SFR_CLOCK_GATE_EN			(1 << 10)
#define WB_SRAM_CLOCK_GATE_EN			(1 << 9)
#define WB_INT_CLOCK_GATE_EN			(1 << 8)
#define WB_ALL_CLOCK_GATE_EN_MASK		(0x7 << 8)
#define WB_SFR_UPDATE_FORCE			(1 << 4)
#define WB_QCHANNEL_EN				(1 << 3)
#define WB_OP_STATUS				(1 << 2)

#define DPU_WB_OUT_CON0			0x004C
#define WB_RGB_TYPE_MASK			(0x3 << 17)
#define WB_RGB_TYPE(_v)			((_v) << 17)
#define WB_CSC_R2Y_MASK			(0x1 << 0)
#define WB_CSC_R2Y(_v)				((_v) << 0)

#define DPU_WB_OUT_CON1			0x0050
#define WB_OUT_FRAME_ALPHA(_v)		((_v) << 24)
#define WB_OUT_FRAME_ALPHA_MASK		(0xff << 24)
#define WB_UV_OFFSET_Y(_v)			((_v) << 5)
#define WB_UV_OFFSET_Y_MASK			(0x7 << 5)
#define WB_UV_OFFSET_X(_v)			((_v) << 0)
#define WB_UV_OFFSET_X_MASK			(0x7 << 0)

#define DPU_WB_DST_SIZE			0x0054
#define WB_DST_HEIGHT(_v)			((_v) << 16)
#define WB_DST_HEIGHT_MASK			(0x1FFF << 16)
#define WB_DST_WIDTH(_v)			((_v) << 0)
#define WB_DST_WIDTH_MASK			(0x1FFF << 0)

#define DPU_WB_USB_TV_WB_SIZE			0x091C
#define WB_FRAME_BYTE_CNT(_v)			((_v) << 0)
#define WB_FRAME_BYTE_CNT_MASK		(0xFFFFFFFF << 0)

#define DPU_WB_USB_TV_WB_CON			0x0920
#define WB_USB_WB_EN(_v)			((_v) << 2)
#define WB_USB_WB_EN_MASK			(0x1 << 2)
#define WB_SWAP_OPTION(_v)			((_v) << 0)
#define WB_SWAP_OPTION_MASK			(0x3 << 0)

/* _n: [0,6], v: [0,1] */
#define DPU_WB_DYNAMIC_GATING_EN		0x0A54
#define WB_DG_EN(_n, _v)			((_v) << (_n))
#define WB_DG_EN_MASK(_n)			(1 << (_n))
#define WB_DG_EN_ALL				(0xF << 0) /* 9820 */

#define DPU_WB_CFG_ERR_STATE			0x0D08
#define WB_CFG_ERR_GET(_v)			(((_v) >> 0) & 0xF)
#define WB_CFG_ERR_WRONG_PATH			(1 << 3)
#define WB_CFG_ERR_ODD_SIZE			(1 << 2)
#define WB_CFG_ERR_MAX_SIZE			(1 << 1)
#define WB_CFG_ERR_MIN_SIZE			(1 << 0)

/*
 * 2 - DPP.base
 *  :  0x1602_0000
 */
#define DPP_ENABLE				0x0000
#define DPP_SRSET				(1 << 24)
#define DPP_SFR_CLOCK_GATE_EN			(1 << 10)
#define DPP_SRAM_CLOCK_GATE_EN		(1 << 9)
#define DPP_INT_CLOCK_GATE_EN			(1 << 8)
#define DPP_ALL_CLOCK_GATE_EN_MASK		(0x7 << 8)
#define DPP_PSLVERR_EN				(1 << 5)
#define DPP_SFR_UPDATE_FORCE			(1 << 4)
#define DPP_QCHANNEL_EN			(1 << 3)
#define DPP_OP_STATUS				(1 << 2)
#define DPP_SECURE_MODE			(1 << 0)

#define DPP_IRQ					0x0004
#define DPP_CONFIG_ERROR			(1 << 21)
#define DPP_STATUS_FRAMEDONE_IRQ		(1 << 16)
#define DPP_ALL_IRQ_CLEAR			(0x21 << 16)
#define DPP_CONFIG_ERROR_MASK			(1 << 6)
#define DPP_IRQ_FRAMEDONE_MASK		(1 << 1)
#define DPP_ALL_IRQ_MASK			(0x21 << 1)
#define DPP_IRQ_ENABLE				(1 << 0)

#define DPP_IN_CON				0x0008
#define DPP_CSC_TYPE(_v)			((_v) << 18)
#define DPP_CSC_TYPE_MASK			(3 << 18)
#define DPP_CSC_RANGE(_v)			((_v) << 17)
#define DPP_CSC_RANGE_MASK			(1 << 17)
#define DPP_CSC_MODE(_v)			((_v) << 16)
#define DPP_CSC_MODE_MASK			(1 << 16)
#define DPP_DITH_MASK_SEL			(1 << 5)
#define DPP_DITH_MASK_SPIN			(1 << 4)
#define DPP_ALPHA_SEL(_v)			((_v) << 3)
#define DPP_ALPHA_SEL_MASK			(1 << 3)
#define DPP_IMG_FORMAT(_v)			((_v) << 0)
#define DPP_IMG_FORMAT_MASK			(0x7 << 0)
#define DPP_IMG_FORMAT_ARGB8888		(0 << 0)
#define DPP_IMG_FORMAT_ARGB8101010	(1 << 0)
#define DPP_IMG_FORMAT_YUV420_8P	(2 << 0)
#define DPP_IMG_FORMAT_YUV420_P010	(3 << 0)
#define DPP_IMG_FORMAT_YUV420_8P2	(4 << 0)
#define DPP_IMG_SIZE				0x0018
#define DPP_IMG_HEIGHT(_v)			((_v) << 16)
#define DPP_IMG_HEIGHT_MASK			(0x1FFF << 16)
#define DPP_IMG_WIDTH(_v)			((_v) << 0)
#define DPP_IMG_WIDTH_MASK			(0x1FFF << 0)

/* VGF only */
#define DPP_SCALED_IMG_SIZE			0x002C
#define DPP_SCALED_IMG_HEIGHT(_v)		((_v) << 16)
#define DPP_SCALED_IMG_HEIGHT_MASK		(0x1FFF << 16)
#define DPP_SCALED_IMG_WIDTH(_v)		((_v) << 0)
#define DPP_SCALED_IMG_WIDTH_MASK		(0x1FFF << 0)

/*
 * VG & VGF only
 * (00-01-02) : Reg0.L-Reg0.H-Reg1.L
 * (10-11-12) : Reg1.H-Reg2.L-Reg2.H
 * (20-21-22) : Reg3.L-Reg3.H-Reg4.L
 */
#define DPP_CSC_COEF0				0x0030
#define DPP_CSC_COEF1				0x0034
#define DPP_CSC_COEF2				0x0038
#define DPP_CSC_COEF3				0x003C
#define DPP_CSC_COEF4				0x0040
#define DPP_CSC_COEF_H(_v)			((_v) << 16)
#define DPP_CSC_COEF_H_MASK			(0xFFFF << 16)
#define DPP_CSC_COEF_L(_v)			((_v) << 0)
#define DPP_CSC_COEF_L_MASK			(0xFFFF << 0)
#define DPP_CSC_COEF_XX(_n, _v)		((_v) << (0 + (16 * (_n))))
#define DPP_CSC_COEF_XX_MASK(_n)		(0xFFF << (0 + (16 * (_n))))

#define DPP_MAIN_H_RATIO			0x0044
#define DPP_H_RATIO(_v)			((_v) << 0)
#define DPP_H_RATIO_MASK			(0xFFFFFF << 0)

#define DPP_MAIN_V_RATIO			0x0048
#define DPP_V_RATIO(_v)			((_v) << 0)
#define DPP_V_RATIO_MASK			(0xFFFFFF << 0)

#define DPP_Y_VCOEF_0A				0x0200
#define DPP_Y_HCOEF_0A				0x0290
#define DPP_C_VCOEF_0A				0x0400
#define DPP_C_HCOEF_0A				0x0490
#define DPP_SCL_COEF(_v)			((_v) << 0)
#define DPP_SCL_COEF_MASK			(0x7FF << 0)
#define DPP_H_COEF(n, s, x)	(0x290 + (n) * 0x4 + (s) * 0x24 + (x) * 0x200)
#define DPP_V_COEF(n, s, x)	(0x200 + (n) * 0x4 + (s) * 0x24 + (x) * 0x200)

#define DPP_YHPOSITION				0x05B0
#define DPP_YVPOSITION				0x05B4
#define DPP_CHPOSITION				0x05B8
#define DPP_CVPOSITION				0x05BC
#define DPP_POS_I(_v)				((_v) << 20)
#define DPP_POS_I_MASK				(0xFFF << 20)
#define DPP_POS_I_GET(_v)			(((_v) >> 20) & 0xFFF)
#define DPP_POS_F(_v)				((_v) << 0)
#define DPP_POS_F_MASK				(0xFFFFF << 0)
#define DPP_POS_F_GET(_v)			(((_v) >> 0) & 0xFFFFF)


#define DPP_DYNAMIC_GATING_EN			0x0A54
#define DPP_DG_EN(_n, _v)			((_v) << (_n))
#define DPP_DG_EN_MASK(_n)			(1 << (_n))
#define DPP_DG_EN_ALL				(0x7F << 0)

#define DPP_LINECNT_CON			0x0d00
#define DPP_LC_CAPTURE(_v)			((_v) << 2)
#define DPP_LC_CAPTURE_MASK			(1 << 2)
#define DPP_LC_MODE(_V)				((_V) << 1)
#define DPP_LC_MODE_MASK			(1 << 1)
#define DPP_LC_ENABLE(_v)			((_v) << 0)
#define DPP_LC_ENABLE_MASK			(1 << 0)

#define DPP_LINECNT_VAL			0x0d04
#define DPP_LC_COUNTER(_v)			((_v) << 0)
#define DPP_LC_COUNTER_MASK			(0x1FFF << 0)
#define DPP_LC_COUNTER_GET(_v)		(((_v) >> 0) & 0x1FFF)

#define DPP_CFG_ERR_STATE			0x0d08
/*
 * #define DPP_CFG_ERR_SCL_POS			(1 << 4)
 * #define DPP_CFG_ERR_SCALE_RATIO		(1 << 3)
 * #define DPP_CFG_ERR_ODD_SIZE			(1 << 2)
 * #define DPP_CFG_ERR_MAX_SIZE			(1 << 1)
 * #define DPP_CFG_ERR_MIN_SIZE			(1 << 0)
 */

#define DPP_CFG_ERR_GET(_v)			(((_v) >> 0) & 0x1F)

/* HDR section */
/* Enable/Disable HDR processing */
#define DPP_VGRF_HDR_CON		0x600
#define DPP_TM_ON(_v)			((_v) << 3)
#define DPP_TM_ON_MASK			(1 << 3)
#define DPP_GM_ON(_v)			((_v) << 2)
#define DPP_GM_ON_MASK			(1 << 2)
#define DPP_EOTF_ON(_v)			((_v) << 1)
#define DPP_EOTF_ON_MASK		(1 << 1)
#define DPP_HDR_ON(_v)			((_v) << 0)
#define DPP_HDR_ON_MASK			(1 << 0)

/* EOTF */
#define DPP_HDR_EOTF_X_AXIS_ADDR(_n)	(((_n) / 2) * (0x4) + (0x610))
#define DPP_HDR_EOTF_X_AXIS_VAL(_n, _v)	\
	(((_n) % (2)) ? (((_v) & 0x3FFF) << 16) : (((_v) & 0x3FFF) << 0))

#define DPP_HDR_EOTF_Y_AXIS_ADDR(_n)	(((_n) / 2) * (0x4) + (0x694))
#define DPP_HDR_EOTF_Y_AXIS_VAL(_n, _v)	\
	(((_n) % (2)) ? (((_v) & 0x3FFF) << 16) : (((_v) & 0x3FFF) << 0))

#define DPP_HDR_EOTF_MASK(_n)		(((_n) % 2) ? (0x3FFF << 16) : (0x3FFF << 0))


/* GM */
#define DPP_HDR_GM_COEF_ADDR(_n)	((_n) * (0x4) + (0x720))
#define DPP_HDR_GM_COEF_MASK	(0x1FFFF << 0)

/* TM */
#define DPP_HDR_TM_X_AXIS_ADDR(_n)	(((_n) / 2) * (0x4) + (0x750))
#define DPP_HDR_TM_X_AXIS_VAL(_n, _v)	\
	(((_n) % (2)) ? (((_v) & 0x3FFF) << 16) : (((_v) & 0x3FFF) << 0))

#define DPP_HDR_TM_Y_AXIS_ADDR(_n)	(((_n) / 2) * (0x4) + (0x794))
#define DPP_HDR_TM_Y_AXIS_VAL(_n, _v)	\
	(((_n) % (2)) ? (((_v) & 0x3FFF) << 16) : (((_v) & 0x3FFF) << 0))

#define DPP_HDR_TM_MASK(_n)		(((_n) % 2) ? (0x3FFF << 16) : (0x3FFF << 0))

#define DPP_VGRF_HDR_EOTF_X_AXIS_0	0x6610
#define DPP_VGRF_HDR_EOTF_X_AXIS_1	0x6614
#define DPP_VGRF_HDR_EOTF_X_AXIS_2	0x6618
#define DPP_VGRF_HDR_EOTF_X_AXIS_3	0x661C
#define DPP_VGRF_HDR_EOTF_X_AXIS_4	0x6620
#define DPP_VGRF_HDR_EOTF_X_AXIS_5	0x6624
#define DPP_VGRF_HDR_EOTF_X_AXIS_6	0x6628
#define DPP_VGRF_HDR_EOTF_X_AXIS_7	0x662C
#define DPP_VGRF_HDR_EOTF_X_AXIS_8	0x6630
#define DPP_VGRF_HDR_EOTF_X_AXIS_9	0x6634
#define DPP_VGRF_HDR_EOTF_X_AXIS_10	0x6638
#define DPP_VGRF_HDR_EOTF_X_AXIS_11	0x663C
#define DPP_VGRF_HDR_EOTF_X_AXIS_12	0x6640
#define DPP_VGRF_HDR_EOTF_X_AXIS_13	0x6644
#define DPP_VGRF_HDR_EOTF_X_AXIS_14	0x6648
#define DPP_VGRF_HDR_EOTF_X_AXIS_15	0x664C
#define DPP_VGRF_HDR_EOTF_X_AXIS_16	0x6650
#define DPP_VGRF_HDR_EOTF_X_AXIS_17	0x6654
#define DPP_VGRF_HDR_EOTF_X_AXIS_18	0x6658
#define DPP_VGRF_HDR_EOTF_X_AXIS_19	0x665C
#define DPP_VGRF_HDR_EOTF_X_AXIS_20	0x6660
#define DPP_VGRF_HDR_EOTF_X_AXIS_21	0x6664
#define DPP_VGRF_HDR_EOTF_X_AXIS_22	0x6668
#define DPP_VGRF_HDR_EOTF_X_AXIS_23	0x666C
#define DPP_VGRF_HDR_EOTF_X_AXIS_24	0x6670
#define DPP_VGRF_HDR_EOTF_X_AXIS_25	0x6674
#define DPP_VGRF_HDR_EOTF_X_AXIS_26	0x6678
#define DPP_VGRF_HDR_EOTF_X_AXIS_27	0x667C
#define DPP_VGRF_HDR_EOTF_X_AXIS_28	0x6680
#define DPP_VGRF_HDR_EOTF_X_AXIS_29	0x6684
#define DPP_VGRF_HDR_EOTF_X_AXIS_30	0x6688
#define DPP_VGRF_HDR_EOTF_X_AXIS_31	0x668C
#define DPP_VGRF_HDR_EOTF_X_AXIS_32	0x6690

#define DPP_VGRF_HDR_EOTF_Y_AXIS_0	0x6694
#define DPP_VGRF_HDR_EOTF_Y_AXIS_1	0x6698
#define DPP_VGRF_HDR_EOTF_Y_AXIS_2	0x669C
#define DPP_VGRF_HDR_EOTF_Y_AXIS_3	0x66A0
#define DPP_VGRF_HDR_EOTF_Y_AXIS_4	0x66A4
#define DPP_VGRF_HDR_EOTF_Y_AXIS_5	0x66A8
#define DPP_VGRF_HDR_EOTF_Y_AXIS_6	0x66AC
#define DPP_VGRF_HDR_EOTF_Y_AXIS_7	0x66B0
#define DPP_VGRF_HDR_EOTF_Y_AXIS_8	0x66B4
#define DPP_VGRF_HDR_EOTF_Y_AXIS_9	0x66B8
#define DPP_VGRF_HDR_EOTF_Y_AXIS_10	0x66BC
#define DPP_VGRF_HDR_EOTF_Y_AXIS_11	0x66C0
#define DPP_VGRF_HDR_EOTF_Y_AXIS_12	0x66C4
#define DPP_VGRF_HDR_EOTF_Y_AXIS_13	0x66C8
#define DPP_VGRF_HDR_EOTF_Y_AXIS_14	0x66CC
#define DPP_VGRF_HDR_EOTF_Y_AXIS_15	0x66D0
#define DPP_VGRF_HDR_EOTF_Y_AXIS_16	0x66D4
#define DPP_VGRF_HDR_EOTF_Y_AXIS_17	0x66D8
#define DPP_VGRF_HDR_EOTF_Y_AXIS_18	0x66DC
#define DPP_VGRF_HDR_EOTF_Y_AXIS_19	0x66E0
#define DPP_VGRF_HDR_EOTF_Y_AXIS_20	0x66E4
#define DPP_VGRF_HDR_EOTF_Y_AXIS_21	0x66E8
#define DPP_VGRF_HDR_EOTF_Y_AXIS_22	0x66EC
#define DPP_VGRF_HDR_EOTF_Y_AXIS_23	0x66F0
#define DPP_VGRF_HDR_EOTF_Y_AXIS_24	0x66F4
#define DPP_VGRF_HDR_EOTF_Y_AXIS_25	0x66F8
#define DPP_VGRF_HDR_EOTF_Y_AXIS_26	0x66FC
#define DPP_VGRF_HDR_EOTF_Y_AXIS_27	0x6700
#define DPP_VGRF_HDR_EOTF_Y_AXIS_28	0x6704
#define DPP_VGRF_HDR_EOTF_Y_AXIS_29	0x6708
#define DPP_VGRF_HDR_EOTF_Y_AXIS_30	0x670C
#define DPP_VGRF_HDR_EOTF_Y_AXIS_31	0x6710
#define DPP_VGRF_HDR_EOTF_Y_AXIS_32	0x6714

#define DPP_VGRF_HDR_GM_COEF_0_0	0x6720
#define DPP_VGRF_HDR_GM_COEF_0_1	0x6724
#define DPP_VGRF_HDR_GM_COEF_0_2	0x6728
#define DPP_VGRF_HDR_GM_COEF_1_0	0x672C
#define DPP_VGRF_HDR_GM_COEF_1_1	0x6730
#define DPP_VGRF_HDR_GM_COEF_1_2	0x6734
#define DPP_VGRF_HDR_GM_COEF_2_0	0x6738
#define DPP_VGRF_HDR_GM_COEF_2_1	0x673C
#define DPP_VGRF_HDR_GM_COEF_2_2	0x6740

#define DPP_VGRF_HDR_TM_X_AXIS_0	0x6750
#define DPP_VGRF_HDR_TM_X_AXIS_1	0x6754
#define DPP_VGRF_HDR_TM_X_AXIS_2	0x6758
#define DPP_VGRF_HDR_TM_X_AXIS_3	0x675C
#define DPP_VGRF_HDR_TM_X_AXIS_4	0x6760
#define DPP_VGRF_HDR_TM_X_AXIS_5	0x6764
#define DPP_VGRF_HDR_TM_X_AXIS_6	0x6768
#define DPP_VGRF_HDR_TM_X_AXIS_7	0x676C
#define DPP_VGRF_HDR_TM_X_AXIS_8	0x6770
#define DPP_VGRF_HDR_TM_X_AXIS_9	0x6774
#define DPP_VGRF_HDR_TM_X_AXIS_10	0x6778
#define DPP_VGRF_HDR_TM_X_AXIS_11	0x677C
#define DPP_VGRF_HDR_TM_X_AXIS_12	0x6780
#define DPP_VGRF_HDR_TM_X_AXIS_13	0x6784
#define DPP_VGRF_HDR_TM_X_AXIS_14	0x6788
#define DPP_VGRF_HDR_TM_X_AXIS_15	0x678C
#define DPP_VGRF_HDR_TM_X_AXIS_16	0x6790

#define DPP_VGRF_HDR_TM_Y_AXIS_0	0x6794
#define DPP_VGRF_HDR_TM_Y_AXIS_1	0x6798
#define DPP_VGRF_HDR_TM_Y_AXIS_2	0x679C
#define DPP_VGRF_HDR_TM_Y_AXIS_3	0x67A0
#define DPP_VGRF_HDR_TM_Y_AXIS_4	0x67A4
#define DPP_VGRF_HDR_TM_Y_AXIS_5	0x67A8
#define DPP_VGRF_HDR_TM_Y_AXIS_6	0x67AC
#define DPP_VGRF_HDR_TM_Y_AXIS_7	0x67B0
#define DPP_VGRF_HDR_TM_Y_AXIS_8	0x67B4
#define DPP_VGRF_HDR_TM_Y_AXIS_9	0x67B8
#define DPP_VGRF_HDR_TM_Y_AXIS_10	0x67BC
#define DPP_VGRF_HDR_TM_Y_AXIS_11	0x67C0
#define DPP_VGRF_HDR_TM_Y_AXIS_12	0x67C4
#define DPP_VGRF_HDR_TM_Y_AXIS_13	0x67C8
#define DPP_VGRF_HDR_TM_Y_AXIS_14	0x67CC
#define DPP_VGRF_HDR_TM_Y_AXIS_15	0x67D0
#define DPP_VGRF_HDR_TM_Y_AXIS_16	0x67D4

#endif
