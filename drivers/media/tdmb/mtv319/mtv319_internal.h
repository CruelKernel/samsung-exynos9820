/*
 *
 * File name: mtv319_internal.h
 *
 * Description : MTV319 internal header file.
 *
 * Copyright (C) (2013, RAONTECH)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MTV319_INTERNAL_H__
#define __MTV319_INTERNAL_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "mtv319.h"
	#include "mtv319_cifdec.h"

#if defined(RTV_MULTIPLE_CHANNEL_MODE) && defined(RTV_MCHDEC_IN_DRIVER)
	#define TDMB_CIF_MODE_DRIVER /* Internal use only */
#endif


#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	#if defined(RTV_INTR_POLARITY_LOW_ACTIVE)
		#define SPI_INTR_POL_ACTIVE	0x00
	#elif defined(RTV_INTR_POLARITY_HIGH_ACTIVE)
		#define SPI_INTR_POL_ACTIVE	(1<<7)
	#endif
#else
	#if defined(RTV_INTR_POLARITY_LOW_ACTIVE)
		#define I2C_INTR_POL_ACTIVE 0x08 /* level low */
	#elif defined(RTV_INTR_POLARITY_HIGH_ACTIVE)
		#define I2C_INTR_POL_ACTIVE 0x18 /* level high */
	#endif
#endif

struct RTV_REG_INIT_INFO {
	U8	bReg;
	U8	bVal;
};


struct RTV_REG_MASK_INFO {
	U8	bReg;
	U8  bMask;
	U8	bVal;
};

enum RTV_TDMB_CH_IDX_TYPE {
	RTV_TDMB_CH_IDX_7A = 0, /* 175280: 7A */
	RTV_TDMB_CH_IDX_7B, /* 177008: 7B */
	RTV_TDMB_CH_IDX_7C, /* 178736: 7C */
	RTV_TDMB_CH_IDX_8A, /* 181280: 8A */
	RTV_TDMB_CH_IDX_8B, /* 183008: 8B */
	RTV_TDMB_CH_IDX_8C, /* 184736: 8C */
	RTV_TDMB_CH_IDX_9A, /* 187280: 9A */
	RTV_TDMB_CH_IDX_9B, /* 189008: 9B */
	RTV_TDMB_CH_IDX_9C, /* 190736: 9C */
	RTV_TDMB_CH_IDX_10A, /* 193280: 10A */
	RTV_TDMB_CH_IDX_10B, /* 195008: 10B */
	RTV_TDMB_CH_IDX_10C, /* 196736: 10C */
	RTV_TDMB_CH_IDX_11A, /* 199280: 11A */
	RTV_TDMB_CH_IDX_11B, /* 201008: 11B */
	RTV_TDMB_CH_IDX_11C, /* 202736: 11C */
	RTV_TDMB_CH_IDX_12A, /* 205280: 12A */
	RTV_TDMB_CH_IDX_12B, /* 207008: 12B */
	RTV_TDMB_CH_IDX_12C, /* 208736: 12C */
	RTV_TDMB_CH_IDX_13A, /* 211280: 13A */
	RTV_TDMB_CH_IDX_13B, /* 213008: 13B */
	RTV_TDMB_CH_IDX_13C, /* 214736: 13C */
	MAX_NUM_RTV_TDMB_CH_IDX
};

struct RTV_ADC_CFG_INFO {
	U16	wFEBD;
	U8	bREFD;
	U32	dwTNCO;
};

#if defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
	#if defined(RTV_TSIF_SPEED_3_Mbps)
		#define RTV_FEC_TSIF_OUT_SPEED	(12<<0)
	#elif defined(RTV_TSIF_SPEED_4_Mbps)
		#define RTV_FEC_TSIF_OUT_SPEED	(8<<0)
	#elif defined(RTV_TSIF_SPEED_5_Mbps)
		#define RTV_FEC_TSIF_OUT_SPEED	(6<<0)
	#elif defined(RTV_TSIF_SPEED_6_Mbps)
		#define RTV_FEC_TSIF_OUT_SPEED	(5<<0)
	#elif defined(RTV_TSIF_SPEED_8_Mbps)
		#define RTV_FEC_TSIF_OUT_SPEED	(4<<0)
	#elif defined(RTV_TSIF_SPEED_10_Mbps)
		#define RTV_FEC_TSIF_OUT_SPEED	(3<<0)
	#elif defined(RTV_TSIF_SPEED_15_Mbps)
		#define RTV_FEC_TSIF_OUT_SPEED	(2<<0)
	#elif defined(RTV_TSIF_SPEED_30_Mbps)
		#define RTV_FEC_TSIF_OUT_SPEED	(1<<0)
	#elif defined(RTV_TSIF_SPEED_60_Mbps)
		#define RTV_FEC_TSIF_OUT_SPEED	(0<<0)
	#else
		#error "Code not present"
	#endif
#endif /* #if defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE) */

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	#if (RTV_SRC_CLK_FREQ_KHz == 4000) /* FPGA */
		#define RTV_SPI_INTR_DEACT_PRD_VAL	0x51

	#elif (RTV_SRC_CLK_FREQ_KHz == 13000)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 16000)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 16384)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 18000)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 19200) /* 1 clk: 52.08ns */
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 24000)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 24576) /* 1 clk: 40.7ns */
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((7<<4)|2)/*about 10us*/

	#elif (RTV_SRC_CLK_FREQ_KHz == 26000)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 27000)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 32000)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 32768)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 36000) /* 1clk: 27.7 ns */
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((7<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 38400)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 40000)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 48000) /* 1clk: 20.8 ns */
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((9<<4)|0)
	#else
		#error "Code not present"
	#endif
#endif /* #if defined(RTV_IF_SPI) */

#if (RTV_TSP_XFER_SIZE == 188)
	#define N_DATA_LEN_BITVAL	0x02
	#define ONE_DATA_LEN_BITVAL	0x00
#elif (RTV_TSP_XFER_SIZE == 204)
	#define N_DATA_LEN_BITVAL	0x03
	#define ONE_DATA_LEN_BITVAL	(1<<5)
#endif

#ifdef RTV_MSC_HDR_ENABLED
		#define DMB_HEADER_LEN_BITVAL	(1<<2) /* 4bytes header */
#else
	#define DMB_HEADER_LEN_BITVAL	0x00
#endif



#define SPI_OVERFLOW_INTR       0x02
#define SPI_UNDERFLOW_INTR      0x20
#define SPI_THRESHOLD_INTR      0x08
#define SPI_INTR_BITS (SPI_THRESHOLD_INTR|SPI_UNDERFLOW_INTR|SPI_OVERFLOW_INTR)


#define TOP_PAGE	0x00
#define HOST_PAGE	0x00
#define OFDM_PAGE	0x08
#define FEC_PAGE	0x09
#define SPI_CTRL_PAGE	0x0E
#define RF_PAGE		0x0F
#define SPI_MEM_PAGE	0xFF /* Temp value. > 15 */


#define DEMOD_0SC_DIV2_ON  0x80
#define DEMOD_0SC_DIV2_OFF 0x00

#if (RTV_SRC_CLK_FREQ_KHz >= 32000)
	#define DEMOD_OSC_DIV2	DEMOD_0SC_DIV2_ON
#else
	#define DEMOD_OSC_DIV2	DEMOD_0SC_DIV2_OFF
#endif


#define MAP_SEL_REG	0x03
#define MAP_SEL_VAL(page)		(DEMOD_OSC_DIV2|page)

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	#define RTV_REG_MAP_SEL(page)	{ g_bRtvPage = page; }
	#define RTV_REG_GET_MAP_SEL	g_bRtvPage
#else
	#define RTV_REG_MAP_SEL(page)\
		do {\
			g_bRtvPage = MAP_SEL_REG;\
			RTV_REG_SET(MAP_SEL_REG, MAP_SEL_VAL(page));\
			g_bRtvPage = page;\
		} while (0)

	#define RTV_REG_GET_MAP_SEL\
		(RTV_REG_GET(MAP_SEL_REG) & ~DEMOD_OSC_DIV2)
#endif

#define TDMB_FREQ_START__KOREA		175280
#define TDMB_FREQ_STEP__KOREA		1728 /* about... */

/* To use at open FIC */
enum E_TDMB_STATE {
	TDMB_STATE_INIT = 0,
	TDMB_STATE_SCAN,
	TDMB_STATE_PLAY
};

enum E_RTV_FIC_OPENED_PATH_TYPE {
	FIC_NOT_OPENED = 0,
	FIC_OPENED_PATH_NOT_USE_IN_PLAY,
	FIC_OPENED_PATH_I2C_IN_SCAN,
	FIC_OPENED_PATH_TSIF_IN_SCAN,
	FIC_OPENED_PATH_I2C_IN_PLAY,
	FIC_OPENED_PATH_TSIF_IN_PLAY
};

extern BOOL g_fRtvFicOpened;


#if defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
extern enum E_RTV_FIC_OPENED_PATH_TYPE g_nRtvFicOpenedStatePath;
#endif

extern U8 g_bRtvIntrMaskReg;

/*
 *
 * Common inline functions.
 *
 *
 */

/* Forward prototype. */

static INLINE void rtv_DisableTSIF(void)
{
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0xAA, 0x00);
}

static INLINE void rtv_EnableTSIF(void)
{
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0xAA, 0x7F);
}

#if defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
static INLINE void rtv_ConfigureTsifFormat(void)
{
#if defined(RTV_TSIF_FORMAT_0) /* EN_high, CLK_rising */
	RTV_REG_SET(0xA4, 0x09);
	RTV_REG_SET(0xA5, 0x80);
	RTV_REG_SET(0xAF, 0x00);

#elif defined(RTV_TSIF_FORMAT_1) /* EN_high, CLK_falling */
	RTV_REG_SET(0xA4, 0x09);
	RTV_REG_SET(0xA5, 0x00);
	RTV_REG_SET(0xAF, 0x00);

#elif defined(RTV_TSIF_FORMAT_2) /* EN_low, CLK_rising */
	RTV_REG_SET(0xA4, 0x09);
	RTV_REG_SET(0xA5, 0x80);
	RTV_REG_SET(0xAF, 0x10);

#elif defined(RTV_TSIF_FORMAT_3) /* EN_low, CLK_falling */
	RTV_REG_SET(0xA4, 0x09);
	RTV_REG_SET(0xA5, 0x00);
	RTV_REG_SET(0xAF, 0x10);

#elif defined(RTV_TSIF_FORMAT_4) /* EN_high, CLK_rising + 1CLK add */
	RTV_REG_SET(0xA4, 0x09);
	RTV_REG_SET(0xA5, 0x84);
	RTV_REG_SET(0xAF, 0x00);

#elif defined(RTV_TSIF_FORMAT_5) /* EN_high, CLK_falling + 1CLK add */
	RTV_REG_SET(0xA4, 0x09);
	RTV_REG_SET(0xA5, 0x04);
	RTV_REG_SET(0xAF, 0x00);

#elif defined(RTV_TSIF_FORMAT_6) /* Parallel: EN_high, CLK_falling */
	RTV_REG_SET(0xA4, 0x01);
	RTV_REG_SET(0xA5, 0x00);
	RTV_REG_SET(0xAF, 0x00);
#else
	#error "Code not present"
#endif

#ifdef RTV_NULL_PID_GENERATE
{
	U8 b0xA4 = RTV_REG_GET(0xA4);

	RTV_REG_SET(0xA4, b0xA4|0x02);
}
#endif

#ifdef RTV_ERROR_TSP_OUTPUT_DISABLE
{
	U8 b0xA5 = RTV_REG_GET(0xA5);

	RTV_REG_SET(0xA5, b0xA5|0x40);
}
#endif
}
#endif /* #elif defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE) */

static INLINE void rtv_DisablePadIntrrupt(void)
{
	RTV_REG_MAP_SEL(HOST_PAGE);
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	/* <2> SPI_INT0(GPP0) disable  <4> I2C INT0 disable */
	RTV_REG_SET(0x1D, 0xC4);
#else
	/* <2> SPI_INT0(GPP0) disable  <4> I2C INT0 disable */
	RTV_REG_SET(0x1D, 0xD0);
#endif

}

static INLINE void rtv_EnablePadIntrrupt(void)
{
	RTV_REG_MAP_SEL(HOST_PAGE);
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	/* <2> SPI_INT0(GPP0) enable  <4> I2C INT0 enable */
	RTV_REG_SET(0x1D, 0xC0);
#else
	RTV_REG_SET(0x1D, 0xC0);
#endif
}

static INLINE void rtv_StopDemod(void)
{
	U8 FEC_F8, FEC_FB;

	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_SET(0x10, 0xCA);

	RTV_REG_MAP_SEL(FEC_PAGE);
	FEC_F8 = RTV_REG_GET(0xF8);
#ifdef RTV_FORCE_INSERT_SYNC_BYTE
	FEC_F8 |= 0x02;
#endif

	FEC_FB = RTV_REG_GET(0xFB);
	RTV_REG_SET(0xFB, (FEC_FB | 0x01));
	RTV_REG_SET(0xF8, (FEC_F8 | 0x80));
}


static INLINE void rtv_SoftReset(void)
{
	U8 OFDM_10, FEC_F8, FEC_FB;

	RTV_REG_MAP_SEL(OFDM_PAGE);
	OFDM_10 = RTV_REG_GET(0x10);
	RTV_REG_SET(0x10, (OFDM_10 & 0xFE));
	RTV_REG_SET(0x10, (OFDM_10 | 0x01));

	RTV_REG_MAP_SEL(FEC_PAGE);
	FEC_F8 = RTV_REG_GET(0xF8);
#ifdef RTV_FORCE_INSERT_SYNC_BYTE
	FEC_F8 |= 0x02;
#endif

	FEC_FB = RTV_REG_GET(0xFB);
	RTV_REG_SET(0xFB, (FEC_FB | 0x01));
	RTV_REG_SET(0xFB, (FEC_FB & 0xFE));
	RTV_REG_SET(0xF8, (FEC_F8 | 0x80));
	RTV_REG_SET(0xF8, (FEC_F8 & 0x7F));
}

static INLINE void rtv_SetupInterruptThreshold(UINT nThresholdSize)
{
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	UINT nMod;

	nMod = nThresholdSize % RTV_TSP_XFER_SIZE;
	if (nMod) /* next xfer align */
		nThresholdSize += (RTV_TSP_XFER_SIZE - nMod);

	RTV_REG_MAP_SEL(SPI_CTRL_PAGE);
	RTV_REG_SET(0x23, nThresholdSize/188);

	/* Save for rtvTDMB_GetInterruptLevelSize() */
	g_nRtvInterruptLevelSize = nThresholdSize;
#endif
}

/*
 *
 * T-DMB inline functions.
 *
 *
 */
static INLINE void rtv_Disable_FIC(void)
{
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0xB2, 0x80);
}

static INLINE void rtv_DisableFicTsifPath(void)
{
	U8 i2c0;

	RTV_REG_MAP_SEL(FEC_PAGE);
	i2c0 = RTV_REG_GET(0x26);
	i2c0 |= 0x01;
	RTV_REG_SET(0x26, i2c0);
}

static INLINE void rtv_EnableFicTsifPath(void)
{
	U8 i2c0;

	RTV_REG_MAP_SEL(FEC_PAGE);
	i2c0 = RTV_REG_GET(0x26);
	i2c0 &= ~0x01;
	RTV_REG_SET(0x26, i2c0);
}

static INLINE void rtv_DisableFicInterrupt(UINT nOpenedSubChNum)
{
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	if (!nOpenedSubChNum) {
		RTV_REG_MAP_SEL(SPI_CTRL_PAGE);
		g_bRtvIntrMaskReg |= SPI_INTR_BITS; /* for polling */
		RTV_REG_SET(0x24, g_bRtvIntrMaskReg); /* Disable interrupts. */

		/* To clear interrupt and data. */
		RTV_REG_SET(0x2A, 1);
		RTV_REG_SET(0x2A, 0);
	}
#else
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0x17, 0xFF);
#endif
}


static INLINE void rtv_EnableFicInterrupt(UINT nOpenedSubChNum)
{
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	#if defined(RTV_SPI_FIC_DECODE_IN_PLAY)\
	|| defined(RTV_FIC__SCAN_I2C__PLAY_TSIF)\
	|| defined(RTV_FIC__SCAN_TSIF__PLAY_TSIF)
	/* The size should equal to the size of MSC interrupt. */
	UINT nThresholdSize = RTV_SPI_CIF_MODE_INTERRUPT_SIZE;
	#else
	UINT nThresholdSize = 384;
	#endif

	if (!nOpenedSubChNum) {
		rtv_SetupInterruptThreshold(nThresholdSize);

		RTV_REG_SET(0x2A, 1);
		RTV_REG_SET(0x2A, 0);

		/* Enable interrupts. */
		g_bRtvIntrMaskReg &= ~(SPI_INTR_BITS);
		RTV_REG_SET(0x24, g_bRtvIntrMaskReg);
	}

#else
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0x17, 0xEF);
#endif
}

static INLINE void rtv_CloseFIC(UINT nOpenedSubChNum)
{
	rtv_Disable_FIC();

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	rtv_DisableFicInterrupt(nOpenedSubChNum);

#elif defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
	switch (g_nRtvFicOpenedStatePath) {
	case FIC_OPENED_PATH_I2C_IN_SCAN: /* No sub channel */
	case FIC_OPENED_PATH_I2C_IN_PLAY:
	/*#ifndef RTV_FIC_POLLING_MODE*/
	RTV_REG_MAP_SEL(HOST_PAGE);
	RTV_REG_SET(0x1A, 0x08); /* GPD3 PAD disable */
	rtv_DisableFicInterrupt(nOpenedSubChNum); /* From I2C intr */
	/*#endif*/
		break;

	case FIC_OPENED_PATH_TSIF_IN_SCAN: /* No sub channel */
	case FIC_OPENED_PATH_TSIF_IN_PLAY: /* Have sub channel */
		rtv_DisableFicTsifPath();
		break;

	case FIC_OPENED_PATH_NOT_USE_IN_PLAY:
	#if defined(RTV_FIC__SCAN_I2C__PLAY_NA)
		#ifndef RTV_FIC_POLLING_MODE
		rtv_DisableFicInterrupt(1);
		#endif
	#elif defined(RTV_FIC__SCAN_TSIF__PLAY_NA)
		rtv_DisableFicTsifPath();
	#endif
		break;

	default:
		break;
	}
#endif
}

static INLINE void rtv_OpenFIC_SPI_Play(void)
{
#ifdef RTV_PLAY_FIC_HDR_ENABLED
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0xB2, 0x04|N_DATA_LEN_BITVAL); /* out_en, hdr_on */

#else
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0xB2, 0x00|N_DATA_LEN_BITVAL); /* out_en, no_hdr*/
#endif
}

static INLINE void rtv_OpenFIC_SPI_Scan(void)
{
#ifdef RTV_FIC_POLLING_MODE
	rtv_DisablePadIntrrupt();
#else
	rtv_EnablePadIntrrupt();
#endif

	rtv_SetupInterruptThreshold(MTV319_FIC_BUF_SIZE);

	RTV_REG_SET(0x2A, 1);
	RTV_REG_SET(0x2A, 0);

	g_bRtvIntrMaskReg &= ~(SPI_INTR_BITS);
	RTV_REG_SET(0x24, g_bRtvIntrMaskReg); /* Enable interrupts. */

#ifdef RTV_SCAN_FIC_HDR_ENABLED
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0xB2, 0x04|N_DATA_LEN_BITVAL); /* out_en, hdr_on */

#else
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0xB2, 0x00|N_DATA_LEN_BITVAL); /* out_en, no_hdr*/
#endif
}

#if defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
static INLINE enum E_RTV_FIC_OPENED_PATH_TYPE rtv_OpenFIC_TSIF_Play(void)
{
	enum E_RTV_FIC_OPENED_PATH_TYPE ePath = FIC_OPENED_PATH_NOT_USE_IN_PLAY;

#if defined(RTV_FIC__SCAN_I2C__PLAY_I2C)\
|| defined(RTV_FIC__SCAN_TSIF__PLAY_I2C)
	rtv_DisableFicTsifPath(); /* For I2C path */

	#ifndef RTV_FIC_POLLING_MODE
	RTV_REG_MAP_SEL(HOST_PAGE);
	RTV_REG_SET(0x1A, 0x00); /* GPD3 PAD enable */
	#endif

	rtv_EnableFicInterrupt(1);

	ePath = FIC_OPENED_PATH_I2C_IN_PLAY;

#elif defined(RTV_FIC__SCAN_I2C__PLAY_TSIF)\
|| defined(RTV_FIC__SCAN_TSIF__PLAY_TSIF)
	rtv_EnableFicTsifPath();
	ePath = FIC_OPENED_PATH_TSIF_IN_PLAY;

#elif defined(RTV_FIC__SCAN_I2C__PLAY_NA)
	ePath = FIC_OPENED_PATH_NOT_USE_IN_PLAY;

#elif defined(RTV_FIC__SCAN_TSIF__PLAY_NA)
	rtv_DisableFicTsifPath();
	ePath = FIC_OPENED_PATH_NOT_USE_IN_PLAY;

#else
	#error "Code not present"
#endif

#ifdef RTV_PLAY_FIC_HDR_ENABLED
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0xB2, 0x04|N_DATA_LEN_BITVAL); /* out_en, hdr_on */

#else
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0xB2, 0x00|N_DATA_LEN_BITVAL); /* out_en, no_hdr*/
#endif

	return ePath;
}


static INLINE enum E_RTV_FIC_OPENED_PATH_TYPE rtv_OpenFIC_TSIF_Scan(void)
{
	enum E_RTV_FIC_OPENED_PATH_TYPE ePath = FIC_OPENED_PATH_NOT_USE_IN_PLAY;

#if defined(RTV_FIC__SCAN_I2C__PLAY_NA)\
|| defined(RTV_FIC__SCAN_I2C__PLAY_I2C)\
|| defined(RTV_FIC__SCAN_I2C__PLAY_TSIF)
	rtv_DisableFicTsifPath(); /* For I2C path */

	#ifndef RTV_FIC_POLLING_MODE
	RTV_REG_MAP_SEL(HOST_PAGE);
	RTV_REG_SET(0x1A, 0x00); /* GPD3 PAD enable */
	#endif

	rtv_EnableFicInterrupt(1);

	ePath = FIC_OPENED_PATH_I2C_IN_SCAN;

#elif defined(RTV_FIC__SCAN_TSIF__PLAY_NA)\
|| defined(RTV_FIC__SCAN_TSIF__PLAY_I2C)\
|| defined(RTV_FIC__SCAN_TSIF__PLAY_TSIF)
	rtv_EnableFicTsifPath();
	ePath = FIC_OPENED_PATH_TSIF_IN_SCAN;

#else
	#error "Code not present"
#endif

#ifdef RTV_SCAN_FIC_HDR_ENABLED
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0xB2, 0x04|N_DATA_LEN_BITVAL); /* out_en, hdr_on */

#else
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0xB2, 0x00|N_DATA_LEN_BITVAL); /* out_en, no_hdr*/
#endif

	return ePath;
}
#endif /* #if defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE) */

static INLINE INT rtv_OpenFIC(enum E_TDMB_STATE eTdmbState)
{
	switch (eTdmbState) {
	case TDMB_STATE_SCAN:
	#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
		rtv_OpenFIC_SPI_Scan();
	#else
		g_nRtvFicOpenedStatePath = rtv_OpenFIC_TSIF_Scan();
	#endif
		break;

	case TDMB_STATE_PLAY:
	#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
		rtv_OpenFIC_SPI_Play();
	#else
		g_nRtvFicOpenedStatePath = rtv_OpenFIC_TSIF_Play();
	#endif
		break;

	default:
		return RTV_INVALID_FIC_OPEN_STATE;
	}

	return RTV_SUCCESS;
}

/*
 * External functions for RAONTV driver core.
 *
 */
INT rtv_InitSystem(void);

#ifdef __cplusplus
}
#endif

#endif /* __MTV319_INTERNAL_H__ */


