/*
 *
 * File name: mtv319_tdmb.c
 *
 * Description : MTV319 T-DMB services source file.
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

#include "mtv319_rf.h"
#include "mtv319_internal.h"

#define TDMB_MAX_NUM_ANTENNA_LEVEL	7
#define MAX_NUM_TDMB_SUBCH	64
#define TDMB_MAX_CER_VALUE	2000

/* #define DEBUG_LOG_FOR_RSSI_FITTING */

#define DIV32(x)	((x) >> 5) /* Divide by 32 */
#define MOD32(x)	((x) & 31)


static enum E_TDMB_STATE g_eTdmbState;

/* NOTE: Do not modify the order! */
enum E_TDMB_HW_SUBCH_IDX_TYPE {
	TDMB_HW_CH_IDX_TDMB = 0,
	TDMB_HW_CH_IDX_DAB_0 = 1,
#if (RTV_MAX_NUM_USE_SUBCHANNEL >= 2)
	TDMB_HW_CH_IDX_DAB_1,
#endif
	TDMB_HW_CH_IDX_DABPLUS,
	MAX_NUM_TDMB_HW_CH_IDX
};
static UINT g_nUsedHwSubChIdxBits;

/* Registered sub channel information. */
struct RTV_TDMB_REG_SUBCH_INFO {
	UINT				nSubChID;
	enum E_TDMB_HW_SUBCH_IDX_TYPE	eHwSubChIdx;
	enum E_RTV_SERVICE_TYPE		eServiceType;
};
static struct RTV_TDMB_REG_SUBCH_INFO
		g_atRegSubchInfo[RTV_MAX_NUM_USE_SUBCHANNEL];

static UINT g_nRegSubChArrayIdxBits;
static UINT g_nTdmbPrevAntennaLevel;
static UINT g_nOpenedSubChNum;

/* Used sub channel ID bits. [0]: 0 ~ 31, [1]: 32 ~ 63 */
static U32 g_aRegSubChIdBits[2];

static BOOL g_fTdmbFastScanEnabled;

#if defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
enum E_RTV_FIC_OPENED_PATH_TYPE g_nRtvFicOpenedStatePath;
#endif

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
UINT g_nRtvInterruptLevelSize;
#endif

BOOL g_fRtvFicOpened;

static void Disable_DABP_SUBCH(void)
{
	g_nUsedHwSubChIdxBits &= ~(1<<TDMB_HW_CH_IDX_DABPLUS);

	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0x83, 00);

#ifdef RTV_MSC_HDR_ENABLED
	RTV_REG_SET(0xB5, 0x84); /* TSIF_DABP */
#else
	RTV_REG_SET(0xB7, 0x00); /* TSIF_ONE: no header, no dummy */
#endif
}

static void Enable_DABP_SUBCH(UINT nSubChID)
{
	RTV_REG_MAP_SEL(FEC_PAGE);

#ifdef RTV_MSC_HDR_ENABLED
	RTV_REG_SET(0xB5, 0x04|N_DATA_LEN_BITVAL); /* TSIF_DABP */
#else
	RTV_REG_SET(0xB7, 0x00|ONE_DATA_LEN_BITVAL); /* TSIF_ONE */
#endif
	RTV_REG_SET(0x83, (0xC0 | nSubChID));

	g_nUsedHwSubChIdxBits |= (1<<TDMB_HW_CH_IDX_DABPLUS);
}

#if (RTV_MAX_NUM_USE_SUBCHANNEL >= 2)
static void Disable_DAB_SUBCH1(void)
{
	RTV_REG_MAP_SEL(FEC_PAGE);

	g_nUsedHwSubChIdxBits &= ~(1<<TDMB_HW_CH_IDX_DAB_1);

	/* Disable data path if the all DAB was closed. */
	if (!(g_nUsedHwSubChIdxBits
		& (((1<<TDMB_HW_CH_IDX_DAB_0))|((1<<TDMB_HW_CH_IDX_DAB_1))))) {
#ifdef RTV_MSC_HDR_ENABLED
		RTV_REG_SET(0xB3, 0x86); /* TSIF_MSC */
#else
		RTV_REG_SET(0xB7, 0x10); /* TSIF_ONE */
#endif
	}

	RTV_REG_SET(0xB9, 00);
}

static void Enable_DAB_SUBCH1(UINT nSubChID)
{
	RTV_REG_MAP_SEL(FEC_PAGE);

	/* Enable data path if the all DAB was closed. */
	if (!(g_nUsedHwSubChIdxBits
		& (((1<<TDMB_HW_CH_IDX_DAB_0))|((1<<TDMB_HW_CH_IDX_DAB_1))))) {
	#ifdef RTV_MSC_HDR_ENABLED
		RTV_REG_SET(0xB3, 0x04|N_DATA_LEN_BITVAL); /* TSIF_MSC */
	#else
		RTV_REG_MAP_SEL(FEC_PAGE);
		RTV_REG_SET(0xB7, 0x90|ONE_DATA_LEN_BITVAL); /* TSIF_ONE */
	#endif
	}

	RTV_REG_SET(0xB9, 0x80|nSubChID);

	g_nUsedHwSubChIdxBits |= (1<<TDMB_HW_CH_IDX_DAB_1);
}
#endif /* #if (RTV_MAX_NUM_USE_SUBCHANNEL >= 2) */

static void Disable_DAB_SUBCH0(void)
{
	RTV_REG_MAP_SEL(FEC_PAGE);

	g_nUsedHwSubChIdxBits &= ~(1<<TDMB_HW_CH_IDX_DAB_0);

#if (RTV_MAX_NUM_USE_SUBCHANNEL >= 2)
	/* Disable data path if the all DAB was closed. */
	if (!(g_nUsedHwSubChIdxBits
		& (((1<<TDMB_HW_CH_IDX_DAB_0))|((1<<TDMB_HW_CH_IDX_DAB_1))))) {
#endif
	#ifdef RTV_MSC_HDR_ENABLED
		RTV_REG_SET(0xB3, 0x86); /* TSIF_MSC */
	#else
		RTV_REG_SET(0xB7, 0x10); /* TSIF_ONE */
	#endif
#if (RTV_MAX_NUM_USE_SUBCHANNEL >= 2)
	}
#endif

	RTV_REG_SET(0xB8, 0x00);
}

static void Enable_DAB_SUBCH0(UINT nSubChID)
{
	RTV_REG_MAP_SEL(FEC_PAGE);

#if (RTV_MAX_NUM_USE_SUBCHANNEL >= 2)
	/* Enable data path if the all DAB was closed. */
	if (!(g_nUsedHwSubChIdxBits
		& (((1<<TDMB_HW_CH_IDX_DAB_0))|((1<<TDMB_HW_CH_IDX_DAB_1))))) {
#endif
	#ifdef RTV_MSC_HDR_ENABLED
		RTV_REG_SET(0xB3, 0x04|N_DATA_LEN_BITVAL); /* TSIF_MSC */
	#else
		RTV_REG_SET(0xB7, 0x90|ONE_DATA_LEN_BITVAL); /* TSIF_ONE */
	#endif
#if (RTV_MAX_NUM_USE_SUBCHANNEL >= 2)
	}
#endif

	RTV_REG_SET(0xB8, 0x80|nSubChID);

	g_nUsedHwSubChIdxBits |= (1<<TDMB_HW_CH_IDX_DAB_0);
}

static void Disable_TDMB_SUBCH(void)
{
	g_nUsedHwSubChIdxBits &= ~(1<<TDMB_HW_CH_IDX_TDMB);

	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0x81, 0x00);
	RTV_REG_SET(0xB4, 0x80);
}

static void Enable_TDMB_SUBCH(UINT nSubChID)
{
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0x81, (0xC0 | nSubChID));
	RTV_REG_SET(0xB4, 0x10|DMB_HEADER_LEN_BITVAL|N_DATA_LEN_BITVAL);

	g_nUsedHwSubChIdxBits |= (1<<TDMB_HW_CH_IDX_TDMB);
}

static void (*g_pfnDisableSUBCH[MAX_NUM_TDMB_HW_CH_IDX])(void) = {
	Disable_TDMB_SUBCH,
	Disable_DAB_SUBCH0,
#if (RTV_MAX_NUM_USE_SUBCHANNEL >= 2)
	Disable_DAB_SUBCH1,
#endif
	Disable_DABP_SUBCH
};

static void (*g_pfnEnableSUBCH[MAX_NUM_TDMB_HW_CH_IDX])(UINT nSubChID) = {
	Enable_TDMB_SUBCH,
	Enable_DAB_SUBCH0,
#if (RTV_MAX_NUM_USE_SUBCHANNEL >= 2)
	Enable_DAB_SUBCH1,
#endif
	Enable_DABP_SUBCH
};

static void tdmb_DisableFastScanMode(void)
{
	if (!g_fTdmbFastScanEnabled)
		return;

	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0x2D, 0x80);
	RTV_REG_SET(0x2E, 0x5F);

	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_SET(0x14, 0x46);
	RTV_REG_SET(0x50, 0x02);
	RTV_REG_SET(0x40, 0x42);
	RTV_REG_SET(0x4C, 0x42);

	g_fTdmbFastScanEnabled = FALSE;
}

static void tdmb_EnableFastScanMode(void)
{
	if (g_fTdmbFastScanEnabled)
		return;

	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_SET(0x14, 0x26); /* Force Stable Test */
	RTV_REG_SET(0x19, 0x34);
	RTV_REG_SET(0x40, 0x40);
	/* TLock Mode <3:0> + 1 ; 40 = 1EA COUNT, 42 = 3EA COUNT */
	RTV_REG_SET(0x4C, 0x40);
	RTV_REG_SET(0x50, 0x06); /* Scan On */

	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0x2D, 0x0C);
	RTV_REG_SET(0x2E, 0x07);

	g_fTdmbFastScanEnabled = TRUE;
}

UINT tdmb_GetOfdmLockStatus(void)
{
	U8 OFDM_B8;
	UINT lock_st = 0;

	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_MASK_SET(0x12, 0x80, 0x80);
	RTV_REG_MASK_SET(0x12, 0x80, 0x00);

	OFDM_B8 = RTV_REG_GET(0xB8);
	if (OFDM_B8 & 0x01)
		lock_st = RTV_TDMB_OFDM_LOCK_MASK;

	if (OFDM_B8 & 0x08)
		lock_st |= RTV_TDMB_AGC_LOCK_MASK;

	return lock_st;
}

static INLINE UINT tdmb_GetFecLockStatus(void)
{
	U8 FECL;
	UINT lock_st = 0;

	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_MASK_SET(0x11, 0x04, 0x04);
	RTV_REG_MASK_SET(0x11, 0x04, 0x00);

	FECL = RTV_REG_GET(0x10); /* FEC Lock (Viterbi Lock) */
	if (FECL & 0x01)
		lock_st = RTV_TDMB_FEC_LOCK_MASK;

	return lock_st;
}

UINT rtvTDMB_GetLockStatus(void)
{
	UINT lock_st;

	RTV_GUARD_LOCK;

	lock_st = tdmb_GetOfdmLockStatus();
	lock_st |= tdmb_GetFecLockStatus();

	RTV_GUARD_FREE;

	return lock_st;
}

UINT rtvTDMB_GetAntennaLevel(U32 dwCER)
{
	UINT nCurLevel = 0;
	UINT nPrevLevel = g_nTdmbPrevAntennaLevel;
	const UINT aAntLvlTbl[TDMB_MAX_NUM_ANTENNA_LEVEL] = {
		/*       0    1    2    3   4    5    6 */
			900, 800, 700, 600, 500, 400, 0};

	if (dwCER == TDMB_MAX_CER_VALUE)
		return 0;

	do {
		if (dwCER >= aAntLvlTbl[nCurLevel]) /* Use equal for CER 0 */
			break;
	} while (++nCurLevel != TDMB_MAX_NUM_ANTENNA_LEVEL);

	if (nCurLevel != nPrevLevel) {
		if (nCurLevel < nPrevLevel)
			nPrevLevel--;
		else
			nPrevLevel++;

		g_nTdmbPrevAntennaLevel = nPrevLevel;
	}

	return nPrevLevel;
}

/* FIC CER */
U32 rtvTDMB_GetFicCER(void)
{
	U8 fec_sync, prd0, prd1, cnt0, cnt1;
	U32 count = 0, period, cer;

	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_MASK_SET(0x11, 0x04, 0x04);
	RTV_REG_MASK_SET(0x11, 0x04, 0x00);

	fec_sync = RTV_REG_GET(0x10);
	if (fec_sync & 0x01) {
		prd0 = RTV_REG_GET(0x3A);
		prd1 = RTV_REG_GET(0x3B);
		period = (prd0<<8) | prd1;

		cnt0 = RTV_REG_GET(0x3C);
		cnt1 = RTV_REG_GET(0x3D);
		count = (cnt0<<8) | cnt1;
	} else
		period = 0;

	RTV_GUARD_FREE;

	if (period) {
	#if 0
		RTV_DBGMSG2("count(%u), period(%u)\n",
					count, period);
	#endif
		cer = (count * 10000) / period;
		if (cer > 1000)
			cer = TDMB_MAX_CER_VALUE; /* No service */
	} else
		cer = TDMB_MAX_CER_VALUE; /* No service */

	return cer;
}

/* MSC CER */
U32 rtvTDMB_GetCER(void)
{
	U8 fec_sync, prd0, prd1, prd2, prd3, cnt0, cnt1, cnt2, cnt3;
	U32 count = 0, period, cer;

	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_MASK_SET(0x11, 0x04, 0x04);
	RTV_REG_MASK_SET(0x11, 0x04, 0x00);

	fec_sync = RTV_REG_GET(0x10);
	if (fec_sync & 0x01) {
		prd0 = RTV_REG_GET(0x3E);
		prd1 = RTV_REG_GET(0x3F);
		prd2 = RTV_REG_GET(0x40);
		prd3 = RTV_REG_GET(0x41);
		period = (prd0<<24) | (prd1<<16) | (prd2<<8) | prd3;

		cnt0 = RTV_REG_GET(0x42);
		cnt1 = RTV_REG_GET(0x43);
		cnt2 = RTV_REG_GET(0x44);
		cnt3 = RTV_REG_GET(0x45);
		count = (cnt0<<24) | (cnt1<<16) | (cnt2<<8) | cnt3;
	} else
		period = 0;

	RTV_GUARD_FREE;

	if (period) {
	#if 0
		RTV_DBGMSG2("count(%u), period(%u)\n",
					count, period);
	#endif
		cer = (count * 10000) / period;
		if (cer > 1000)
			cer = TDMB_MAX_CER_VALUE; /* No service */
	} else
		cer = TDMB_MAX_CER_VALUE; /* No service */

	return cer;
}

#define RSSI_RFAGC_VAL(rfagc, coeffi)\
	((rfagc) * (S32)((coeffi)*RTV_TDMB_RSSI_DIVIDER))

#define RSSI_GVBB_VAL(gvbb, coeffi)\
	((gvbb) * (S32)((coeffi)*RTV_TDMB_RSSI_DIVIDER))

S32 rtvTDMB_GetRSSI(void)
{
	U8 RD00, GVBB, LNAGAIN, RFAGC;
#ifdef DEBUG_LOG_FOR_RSSI_FITTING
	U8 CH_FLAG;
#endif
	S32 nRssi = 0;

	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(RF_PAGE);
	RD00 = RTV_REG_GET(0x00);
	GVBB = RTV_REG_GET(0x01);

	RTV_GUARD_FREE;

#ifdef DEBUG_LOG_FOR_RSSI_FITTING
	CH_FLAG = ((RD00 & 0xC0) >> 6);
#endif
	LNAGAIN = ((RD00 & 0x18) >> 3);
	RFAGC = (RD00 & 0x07);

	switch (LNAGAIN) {
	case 0:
		nRssi = -(RSSI_RFAGC_VAL(RFAGC, 2.8)
				+ RSSI_GVBB_VAL(GVBB, 0.44) + 0)
				+ 5 * RTV_TDMB_RSSI_DIVIDER;
		break;

	case 1:
		nRssi = -(RSSI_RFAGC_VAL(RFAGC, 3)
		+ RSSI_GVBB_VAL(GVBB, 0.3) + (19 * RTV_TDMB_RSSI_DIVIDER))
		+ 0 * RTV_TDMB_RSSI_DIVIDER;
		break;

	case 2:
		nRssi = -(RSSI_RFAGC_VAL(RFAGC, 3)
		+ RSSI_GVBB_VAL(GVBB, 0.3) + (16 * 2 * RTV_TDMB_RSSI_DIVIDER))
		+ 0 * RTV_TDMB_RSSI_DIVIDER;
		break;

	case 3:
		nRssi = -(RSSI_RFAGC_VAL(RFAGC, 2.6)
		+ RSSI_GVBB_VAL(GVBB, 0.4) + (11 * 3 * RTV_TDMB_RSSI_DIVIDER))
		+ 0 * RTV_TDMB_RSSI_DIVIDER;
		break;

	default:
		break;
	}

#ifdef DEBUG_LOG_FOR_RSSI_FITTING
	RTV_DBGMSG3("CH_FLAG(%d) RD00(0x%02x) GVBB(0x%02x)\n",
		CH_FLAG, RD00, GVBB);
	RTV_DBGMSG3("LNAGAIN = %d, RFAGC = %d GVBB : %d\n",
		LNAGAIN, RFAGC, GVBB);
#endif

	return	nRssi;
}

/* SNR */
U32 rtvTDMB_GetCNR(void)
{
	U8 data1, data2, fec_sync;
	U32 data, snr = 0;

	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_MASK_SET(0x12, 0x80, 0x80);
	RTV_REG_MASK_SET(0x12, 0x80, 0x00);

	data1 = RTV_REG_GET(0xE0);
	data2 = RTV_REG_GET(0xE1);
	data = (((data2&0x3F)<<8) | data1) * 4;

	RTV_REG_MAP_SEL(FEC_PAGE);
	fec_sync = RTV_REG_GET(0x10);

	RTV_GUARD_FREE;

	if (fec_sync & 0x01) {
		if (data > 7800)
			snr = 0;
		else if ((data > 6600)  && (data <= 7800))
			snr = (-4 * (S32)data/5200 * RTV_TDMB_CNR_DIVIDER)
				+ (U32)(8.3*RTV_TDMB_CNR_DIVIDER);
		else if ((data > 4100)  && (data <= 6600))
			snr = (-5 * (S32)data/4300 * RTV_TDMB_CNR_DIVIDER)
				+ (U32)(14.0*RTV_TDMB_CNR_DIVIDER);
		else if ((data > 1400)  && (data <= 4100))
			snr = (-5 * (S32)data/3500 * RTV_TDMB_CNR_DIVIDER)
				+ (U32)(16.0*RTV_TDMB_CNR_DIVIDER);
		else if ((data > 600) && (data <= 1400))
			snr = (-10 * (S32)data/1200 * RTV_TDMB_CNR_DIVIDER)
				+ (U32)(27.0*RTV_TDMB_CNR_DIVIDER);
		else if ((data > 200) && (data <= 600))
			snr = (-15 * (S32)data/400 * RTV_TDMB_CNR_DIVIDER)
				+ (U32)(42.5*RTV_TDMB_CNR_DIVIDER);
		else /* if (data <= 200) */
			snr = 35 * RTV_TDMB_CNR_DIVIDER;
	} else
		snr = 5 * RTV_TDMB_CNR_DIVIDER;

	return snr;
}

U32 rtvTDMB_GetPER(void)
{
	U8 rdata0, rdata1, rs_sync;
	U32 per = 700;

	RTV_GUARD_LOCK;

	if (g_nUsedHwSubChIdxBits & (1<<TDMB_HW_CH_IDX_TDMB)) {
		RTV_REG_MAP_SEL(FEC_PAGE);
		rs_sync = (RTV_REG_GET(0xA2) >> 3) & 0x01;
		if (rs_sync) {
			rdata1 = RTV_REG_GET(0x5C);
			rdata0 = RTV_REG_GET(0x5D);
			per = (rdata1 << 8) | rdata0;
		}
	} else if (g_nUsedHwSubChIdxBits & (1<<TDMB_HW_CH_IDX_DABPLUS)) {
		RTV_REG_MAP_SEL(FEC_PAGE);
		rs_sync = (RTV_REG_GET(0x10) >> 1) & 0x01;
		if (rs_sync) {
			rdata1 = RTV_REG_GET(0x6F);
			rdata0 = RTV_REG_GET(0x70);
			per = (rdata1 << 8) | rdata0;
		}
	} else
		per = 0;

	RTV_GUARD_FREE;

	return  per;

}

/* After Viterbi BER */
U32 rtvTDMB_GetBER(void)
{
	U8 rs_sync, prd0, prd1, cnt0, cnt1, cnt2;
	U32 count = 0, period, ber = RTV_TDMB_BER_DIVIDER;

	RTV_GUARD_LOCK;

	if (g_nUsedHwSubChIdxBits & (1<<TDMB_HW_CH_IDX_TDMB)) {
		RTV_REG_MAP_SEL(FEC_PAGE);
		rs_sync = (RTV_REG_GET(0xA2) >> 3) & 0x01;
		if (rs_sync) {
			prd0 = RTV_REG_GET(0x34);
			prd1 = RTV_REG_GET(0x35);
			period = (prd0<<8) | prd1;

			cnt0 = RTV_REG_GET(0x56);
			cnt1 = RTV_REG_GET(0x57);
			cnt2 = RTV_REG_GET(0x58);
			count = ((cnt0&0x7f)<<16) | (cnt1<<8) | cnt2;
		} else
			period = 0;
	} else if (g_nUsedHwSubChIdxBits & (1<<TDMB_HW_CH_IDX_DABPLUS)) {
		RTV_REG_MAP_SEL(FEC_PAGE);
		rs_sync = (RTV_REG_GET(0x10) >> 1) & 0x01;
		if (rs_sync) {
			prd0 = RTV_REG_GET(0x37);
			prd1 = RTV_REG_GET(0x38);
			period = (prd0<<8) | prd1;

			cnt0 = RTV_REG_GET(0x69);
			cnt1 = RTV_REG_GET(0x6A);
			cnt2 = RTV_REG_GET(0x6B);
			count = ((cnt0&0x7f)<<16) | (cnt1<<8) | cnt2;
		} else
			period = 0;
	} else {
		period = 0;
		ber = 0;
	}

	RTV_GUARD_FREE;

	if (period)
		ber = (count * (U32)RTV_TDMB_BER_DIVIDER) / (period*8*204);

	return ber;
}

UINT rtvTDMB_GetOpenedSubChannelCount(void)
{
	return g_nOpenedSubChNum;
}

U32 rtvTDMB_GetPreviousFrequency(void)
{
	return g_dwRtvPrevChFreqKHz;
}

static void tdmb_CloseSubChannel(UINT nRegSubChArrayIdx)
{
	UINT nSubChID;
	enum E_TDMB_HW_SUBCH_IDX_TYPE eHwSubChIdx;

	nSubChID = g_atRegSubchInfo[nRegSubChArrayIdx].nSubChID;
	eHwSubChIdx = g_atRegSubchInfo[nRegSubChArrayIdx].eHwSubChIdx;

	/* Call the specified disabling of SUBCH function. */
	g_pfnDisableSUBCH[eHwSubChIdx]();

	g_nRegSubChArrayIdxBits &= ~(1<<nRegSubChArrayIdx);
	g_aRegSubChIdBits[DIV32(nSubChID)] &= ~(1 << MOD32(nSubChID));
	g_nOpenedSubChNum--;

	if (!g_nOpenedSubChNum && !g_fRtvFicOpened) {
	#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
		RTV_REG_MAP_SEL(SPI_CTRL_PAGE);
		g_bRtvIntrMaskReg |= SPI_INTR_BITS; /* for polling */
		RTV_REG_SET(0x24, g_bRtvIntrMaskReg); /* Disable interrupts. */

		/* To clear interrupt and data. */
		RTV_REG_SET(0x2A, 1);
		RTV_REG_SET(0x2A, 0);
	#endif

	#ifdef TDMB_CIF_MODE_DRIVER
		rtvCIFDEC_Deinit(); /* Skip rtvCIFDEC_DeleteSubChannelID() */
	#endif

		/* Update the state of TDMB. */
		g_eTdmbState = TDMB_STATE_INIT;
	} else {
	#ifdef TDMB_CIF_MODE_DRIVER
		rtvCIFDEC_DeleteSubChannelID(nSubChID);
	#endif
	}
}

void rtvTDMB_CloseAllSubChannels(void)
{
#if (RTV_MAX_NUM_USE_SUBCHANNEL >= 2)
	UINT i = 0;
	UINT nRegSubChArrayIdxBits = g_nRegSubChArrayIdxBits;
#endif

	if (g_nOpenedSubChNum == 0)
		return; /* not opened! already closed! */

	RTV_GUARD_LOCK;

#if (RTV_MAX_NUM_USE_SUBCHANNEL == 1) /* Single Sub Channel */
	tdmb_CloseSubChannel(0);
#elif (RTV_MAX_NUM_USE_SUBCHANNEL >= 2)
	while (nRegSubChArrayIdxBits) {
		if (nRegSubChArrayIdxBits & 0x01)
			tdmb_CloseSubChannel(i);

		nRegSubChArrayIdxBits >>= 1;
		i++;
	}
#endif

	RTV_GUARD_FREE;
}

INT rtvTDMB_CloseSubChannel(UINT nSubChID)
{
#if (RTV_MAX_NUM_USE_SUBCHANNEL >= 2)
	UINT i = 0;
	UINT nRegSubChArrayIdxBits = g_nRegSubChArrayIdxBits;
#endif

	if (nSubChID > (MAX_NUM_TDMB_SUBCH-1))
		return RTV_INVAILD_SUBCHANNEL_ID;

	if (g_nOpenedSubChNum == 0)
		return RTV_SUCCESS; /* not opened! already closed! */

	RTV_GUARD_LOCK;

#if (RTV_MAX_NUM_USE_SUBCHANNEL == 1) /* Single Sub Channel */
	tdmb_CloseSubChannel(0);
#else
	while (nRegSubChArrayIdxBits) {
		if (nRegSubChArrayIdxBits & 0x01) {
			if (nSubChID == g_atRegSubchInfo[i].nSubChID)
				tdmb_CloseSubChannel(i);
		}

		nRegSubChArrayIdxBits >>= 1;
		i++;
	}
#endif

	RTV_GUARD_FREE;

	return RTV_SUCCESS;
}

static void tdmb_OpenSubChannel(UINT nSubChID,
		enum E_RTV_SERVICE_TYPE eServiceType, UINT nThresholdSize,
		enum E_TDMB_HW_SUBCH_IDX_TYPE eHwSubChIdx)
{
	UINT i;

	if (g_nOpenedSubChNum == 0) { /* The first open */
	#ifdef TDMB_CIF_MODE_DRIVER
		rtvCIFDEC_Init();
		rtvCIFDEC_AddSubChannelID(nSubChID, eServiceType);
	#endif

	#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
		rtv_SetupInterruptThreshold(nThresholdSize);

		RTV_REG_SET(0x2A, 1);
		RTV_REG_SET(0x2A, 0);

		/* Enable SPI interrupts */
		g_bRtvIntrMaskReg &= ~(SPI_INTR_BITS);
		RTV_REG_SET(0x24, g_bRtvIntrMaskReg);

		rtv_EnablePadIntrrupt();
	#endif
	} else {
	#ifdef TDMB_CIF_MODE_DRIVER
		rtvCIFDEC_AddSubChannelID(nSubChID, eServiceType);
	#endif
	}

	/* Call the specified enabling of SUBCH function. */
	g_pfnEnableSUBCH[eHwSubChIdx](nSubChID);

	g_aRegSubChIdBits[DIV32(nSubChID)] |= (1 << MOD32(nSubChID));

	/* To use when close .*/
#if (RTV_MAX_NUM_USE_SUBCHANNEL == 1)
	i = 0;
#else
	for (i = 0; i < RTV_MAX_NUM_USE_SUBCHANNEL; i++) {
		if ((g_nRegSubChArrayIdxBits & (1<<i)) == 0) {
#endif
			g_nRegSubChArrayIdxBits |= (1<<i);
			g_atRegSubchInfo[i].nSubChID = nSubChID;
			g_atRegSubchInfo[i].eHwSubChIdx = eHwSubChIdx;
			g_atRegSubchInfo[i].eServiceType = eServiceType;
#if (RTV_MAX_NUM_USE_SUBCHANNEL >= 2)
			break;
		}
	}
#endif

	g_nOpenedSubChNum++;
}

/* For Test */
INT rtvTDMB_OpenSubChannelExt(U32 dwChFreqKHz, UINT nSubChID,
		enum E_RTV_SERVICE_TYPE eServiceType, UINT nThresholdSize)
{
	INT nRet;

	rtvTDMB_CloseFIC();
	rtvTDMB_CloseAllSubChannels();

	nRet = rtvTDMB_ScanFrequency(dwChFreqKHz);
	rtvTDMB_CloseFIC();
	if (nRet == RTV_SUCCESS)
		nRet = rtvTDMB_OpenSubChannel(dwChFreqKHz, nSubChID,
					eServiceType, nThresholdSize);
	else
		RTV_DBGMSG1("rtvTDMB_OpenSubChannelExt() fail: %d\n", nRet);

	return nRet;
}

INT rtvTDMB_OpenSubChannel(U32 dwChFreqKHz, UINT nSubChID,
		enum E_RTV_SERVICE_TYPE eServiceType, UINT nThresholdSize)
{
	INT nRet = RTV_SUCCESS;
	enum E_TDMB_HW_SUBCH_IDX_TYPE eHwChIdx;

	if (nSubChID > (MAX_NUM_TDMB_SUBCH - 1))
		return RTV_INVAILD_SUBCHANNEL_ID;

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	if (!nThresholdSize || (nThresholdSize > (32 * RTV_TSP_XFER_SIZE)))
		return RTV_INVAILD_THRESHOLD_SIZE;
#endif

#ifndef RTV_MULTIPLE_CHANNEL_MODE
	if (g_fRtvFicOpened)
		return RTV_SHOULD_CLOSE_FIC;
#endif

	if (g_nOpenedSubChNum > RTV_MAX_NUM_USE_SUBCHANNEL)
		return RTV_NO_MORE_SUBCHANNEL;

#if (defined(RTV_IF_SPI) || defined(RTV_IF_EBI2))\
&& defined(RTV_MULTIPLE_CHANNEL_MODE)
	nThresholdSize = RTV_SPI_CIF_MODE_INTERRUPT_SIZE;
#endif

	RTV_GUARD_LOCK;

	/* Check if the specified subch opened or closed? */
	if (dwChFreqKHz == g_dwRtvPrevChFreqKHz) {
		/* Is registerd sub ch ID? */
		if (g_aRegSubChIdBits[DIV32(nSubChID)] & (1<<MOD32(nSubChID))) {
			RTV_DBGMSG0("[rtvTDMB_OpenSubChannel]Already opened\n");
			nRet = RTV_ALREADY_OPENED_SUBCHANNEL_ID;
			goto tdmb_open_subch_exit; /* OK */
		}
	} else { /* Different freq */
		if (g_nOpenedSubChNum) {
			nRet = RTV_SHOULD_CLOSE_SUBCHANNELS;
			goto tdmb_open_subch_exit;
		}
	}

	switch (eServiceType) {
	case RTV_SERVICE_DMB:
		eHwChIdx = TDMB_HW_CH_IDX_TDMB;
		break;
	case RTV_SERVICE_DAB: /* DAB */
		if (!(g_nUsedHwSubChIdxBits & (1<<TDMB_HW_CH_IDX_DAB_0)))
			eHwChIdx = TDMB_HW_CH_IDX_DAB_0;
	#if (RTV_MAX_NUM_USE_SUBCHANNEL >= 2)
		else if (!(g_nUsedHwSubChIdxBits & (1<<TDMB_HW_CH_IDX_DAB_1)))
			eHwChIdx = TDMB_HW_CH_IDX_DAB_1;
	#endif
		else {
			nRet = RTV_OPENING_SERVICE_FULL;
			goto tdmb_open_subch_exit;
		}
		break;
	case RTV_SERVICE_DABPLUS: /* DAB+ */
		eHwChIdx = TDMB_HW_CH_IDX_DABPLUS;
		break;
	default:
		nRet = RTV_INVAILD_SERVICE_TYPE;
		goto tdmb_open_subch_exit;
	}

	/* NOTE: The below code should placed after checking of subch opened. */
	if (g_nUsedHwSubChIdxBits & (1<<eHwChIdx)) {
		nRet = RTV_OPENING_SERVICE_FULL;
		goto tdmb_open_subch_exit;
	}

	tdmb_DisableFastScanMode();

	/* Check if new freq equal to previous freq or not. */
	if (g_dwRtvPrevChFreqKHz == dwChFreqKHz) {
	#if (RTV_MAX_NUM_USE_SUBCHANNEL >= 2)
		if (!g_nOpenedSubChNum) {
	#endif
			RTV_REG_MAP_SEL(OFDM_PAGE);
			RTV_REG_SET(0x10, 0xCA);
			RTV_REG_SET(0x10, 0xCB);
	#if (RTV_MAX_NUM_USE_SUBCHANNEL >= 2)
		}
	#endif

		tdmb_OpenSubChannel(nSubChID, eServiceType,
					nThresholdSize, eHwChIdx);
	} else {
		rtv_StopDemod(); /* To hold until rf set */

		tdmb_OpenSubChannel(nSubChID, eServiceType,
					nThresholdSize, eHwChIdx);

		/* Must place in the last */
		nRet = rtvRF_SetFrequency(dwChFreqKHz);
		rtv_SoftReset();
		if (nRet != RTV_SUCCESS)
			goto tdmb_open_subch_exit;
	}

	/* Update the state of TDMB. */
	g_eTdmbState = TDMB_STATE_PLAY;

tdmb_open_subch_exit:
	RTV_GUARD_FREE;

	return nRet;
}

static INLINE BOOL tdmb_CheckScanStatus(INT *sucess_flag,
			U8 DAB_Mode, U8 OFDM_L,
			U8 FIC_CRC, int ScanT, int *scan_status)
{
	BOOL fBreak = FALSE;

	if (DAB_Mode == 0x00) {
		if (OFDM_L) {
			if (FIC_CRC < 100) {
				sucess_flag = RTV_SUCCESS;
				fBreak = TRUE;
			} else if (FIC_CRC != 255) {
				*scan_status = 5;
				*sucess_flag = RTV_CHANNEL_NOT_DETECTED;
				fBreak = TRUE;
			}
		} else if (ScanT > 500) { /* Tuning pointer */
			*scan_status = 3;
			*sucess_flag = RTV_CHANNEL_NOT_DETECTED;
			fBreak = TRUE;
		}
	} else {
		*scan_status = 2;
		*sucess_flag = RTV_CHANNEL_NOT_DETECTED;
		fBreak = TRUE;
	}

	return fBreak;
}

/* SCAN debuging log enable */
#define DEBUG_LOG_FOR_SCAN

/* NOTE: When this rountine executed, all sub channel and FIC should closed */
INT rtvTDMB_ScanFrequency(U32 dwChFreqKHz)
{
	U8 Mon_FSM = 0;
	int peak_pwr = 0;
	U8 DAB_Mode, peak_pwr_Msb = 0, peak_pwr_Lsb = 0;
	U8 OFDM_L = 0;
	U8 AGC_L = 0;
	U8 FIC_CRC = 255;
	U8 Sdone = 0, Slock = 0;
	INT sucess_flag = 0;
	UINT ScanT = 0;
	int pwr_threshold = 600; /* OPT */
	int scan_status = 0;
	U8 FIC_Sync = 0;
	U8 Ccnt = 0, Tcnt = 0;
	UINT nDelayTime;
#if defined(__KERNEL__) /* Linux kernel */
	unsigned long start_jiffies, end_jiffies;
	UINT diff_time = 0;
#endif

	sucess_flag = RTV_CHANNEL_NOT_DETECTED;

	if (g_fRtvFicOpened) /* For openFIC[hdr_on] => Scan[hdr_off] */
		return RTV_SHOULD_CLOSE_FIC; /* for header On/Off */

	if (g_nOpenedSubChNum)
		return RTV_SHOULD_CLOSE_SUBCHANNELS;

	RTV_GUARD_LOCK;

	/* Update the state of TDMB. */
	g_eTdmbState = TDMB_STATE_SCAN;

	tdmb_EnableFastScanMode();

	rtv_StopDemod();

	g_fRtvFicOpened = TRUE;
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	rtv_OpenFIC_SPI_Scan();
#else
	g_nRtvFicOpenedStatePath = rtv_OpenFIC_TSIF_Scan();
		RTV_REG_MAP_SEL(FEC_PAGE);
		RTV_REG_MASK_SET(0x11, 0x04, 0x04);
		RTV_REG_MASK_SET(0x11, 0x04, 0x00);
#endif

	sucess_flag = rtvRF_SetFrequency(dwChFreqKHz);
	if (sucess_flag != RTV_SUCCESS)
		goto TDMB_SCAN_EXIT;

#if defined(RTV_SCAN_FIC_HDR_ENABLED) && defined(RTV_MCHDEC_IN_DRIVER)
	rtvCIFDEC_Init();
#endif

	RTV_DELAY_MS(50);
	rtv_SoftReset();

	while (1) {
#if defined(__KERNEL__) /* Linux kernel */
		start_jiffies = get_jiffies_64();
#endif

		RTV_REG_MAP_SEL(OFDM_PAGE);
		RTV_REG_MASK_SET(0x82, 0x02, 0x02);
		RTV_REG_MASK_SET(0x82, 0x02, 0x00);

		DAB_Mode = RTV_REG_GET(0xBA);
		DAB_Mode = (DAB_Mode>>4) & 0x03;

		RTV_REG_MASK_SET(0x12, 0x80, 0x80);
		RTV_REG_MASK_SET(0x12, 0x80, 0x00);

		RTV_REG_MAP_SEL(OFDM_PAGE);
		peak_pwr_Msb = RTV_REG_GET(0xD6);
		peak_pwr_Lsb = RTV_REG_GET(0xD5);
		peak_pwr = ((peak_pwr_Msb&0xff)<<8) | (peak_pwr_Lsb&0xff);

		OFDM_L = RTV_REG_GET(0xB8);
		OFDM_L = (OFDM_L>>0) & 0x01;

		RTV_REG_MAP_SEL(FEC_PAGE);
		FIC_CRC = RTV_REG_GET(0x30);

		RTV_REG_MAP_SEL(OFDM_PAGE);
		AGC_L = RTV_REG_GET(0xB8);
		AGC_L = (AGC_L>>3) & 0x01;

		Mon_FSM = RTV_REG_GET(0xB8);
		Mon_FSM = (Mon_FSM>>5)&0x07;

		Sdone = RTV_REG_GET(0xD4);
		Slock = RTV_REG_GET(0xD4);
		Sdone = (Sdone&0x02)>>1;
		Slock = (Slock&0x01)>>0;

		RTV_REG_MAP_SEL(FEC_PAGE);
		FIC_Sync = RTV_REG_GET(0x10);
		FIC_Sync = FIC_Sync & 0x01;

		RTV_REG_MAP_SEL(OFDM_PAGE);
		Ccnt = RTV_REG_GET(0xC1);
		Tcnt = RTV_REG_GET(0xC2);
		Ccnt = (Ccnt>>3)&0x1f;
		Tcnt = Tcnt & 0x1f;

		if (AGC_L) {
			if (Sdone || ScanT > 200) {
				if (Slock || (peak_pwr > pwr_threshold)) {
					if (tdmb_CheckScanStatus(&sucess_flag,
						DAB_Mode, OFDM_L,
						FIC_CRC, ScanT, &scan_status))
						break;
				} else {
					scan_status = 4;
					sucess_flag = RTV_CHANNEL_NOT_DETECTED;
					break;
				}
			}
		}

#if defined(__KERNEL__) /* Linux kernel */
		end_jiffies = get_jiffies_64();
		diff_time = jiffies_to_msecs(end_jiffies - start_jiffies);
		if (diff_time < 4)
			nDelayTime = 4 - diff_time;
		else
			nDelayTime = 0;

		ScanT += diff_time;
#else
		nDelayTime = 4;
#endif

		if (ScanT >= 1000) {
			RTV_DBGMSG0("Scan Timeout!\n");
			scan_status = 7;
			sucess_flag = RTV_CHANNEL_NOT_DETECTED;
			break;
		}

		if (nDelayTime) { /* Up to 4ms delay */
			ScanT += nDelayTime;
			RTV_DELAY_MS(nDelayTime);
		}
	}

TDMB_SCAN_EXIT:
	tdmb_DisableFastScanMode();

	if (sucess_flag != RTV_SUCCESS) {
#if defined(RTV_SCAN_FIC_HDR_ENABLED) && defined(RTV_MCHDEC_IN_DRIVER)
		rtvCIFDEC_Deinit();
#endif
		rtv_CloseFIC(0); /* rtvTDMB_CloseFIC() was called when lock_s */
		g_eTdmbState = TDMB_STATE_INIT;
		g_fRtvFicOpened = FALSE;
	}

	RTV_GUARD_FREE;

#ifdef DEBUG_LOG_FOR_SCAN
	RTV_DBGMSG3("%u Power_Peak(%d), AGCL(%d)\n",
			dwChFreqKHz, peak_pwr, AGC_L);
	RTV_DBGMSG2("\tOFDML = %d, CRC = %d\n", OFDM_L, FIC_CRC);
	RTV_DBGMSG3("\tScanT = %d Status = %d scan_done = %d\n",
			ScanT, scan_status, Sdone);
	RTV_DBGMSG3("\tscan_lock = %d FSM = %d scan_success = %d\n\n",
			Slock, Mon_FSM, sucess_flag);
#endif

	return sucess_flag;

}

#define RTV_TDMB_READ_FIC_TIMEOUT_CNT	500

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
static INLINE INT tdmb_ReadFIC_SPI(U8 *pbBuf)
{
	int ret_size;
	U8 istatus;
	UINT lock_s;
	UINT elapsed_cnt = 0;
	UINT timeout_cnt = RTV_TDMB_READ_FIC_TIMEOUT_CNT;

	while (1) {
		RTV_REG_MAP_SEL(SPI_CTRL_PAGE);
		istatus = RTV_REG_GET(0x10);
		if (istatus & SPI_INTR_BITS) {
			if (!(istatus & SPI_UNDERFLOW_INTR)) {
				RTV_REG_MAP_SEL(SPI_MEM_PAGE);
				RTV_REG_BURST_GET(0x10, pbBuf,
						g_nRtvInterruptLevelSize);

#ifdef RTV_SCAN_FIC_HDR_ENABLED
				ret_size = g_nRtvInterruptLevelSize;
#else
				ret_size = 384;
#endif

				break;
			}
		}

		lock_s = tdmb_GetOfdmLockStatus();
		if (!(lock_s & RTV_TDMB_OFDM_LOCK_MASK)) {
			RTV_DELAY_MS(30);
			RTV_DBGMSG2("##lock_s(0x%02X)[%u]\n",
					lock_s, elapsed_cnt);
			ret_size = -55;
			break;
		}

		if (timeout_cnt--)
			RTV_DELAY_MS(1);
		else {
			ret_size = RTV_FIC_READ_TIMEOUT;
			break;
		}

		elapsed_cnt = RTV_TDMB_READ_FIC_TIMEOUT_CNT - timeout_cnt;
	}

	return ret_size;
}
#elif defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
static INLINE INT tdmb_ReadFIC_I2C(U8 *pbBuf)
{
#ifdef RTV_FIC_POLLING_MODE
	U8 istatus;
	int ret_size;
	UINT lock_s;
	UINT elapsed_cnt = 0;
	UINT timeout_cnt = RTV_TDMB_READ_FIC_TIMEOUT_CNT;

	while (1) {
		RTV_REG_MAP_SEL(FEC_PAGE);
		istatus = RTV_REG_GET(0x13) & 0x10; /* [4] */
	#if 0
		RTV_DBGMSG1("istatus(0x%02X)\n", istatus);
	#endif
		if (istatus) {
			RTV_REG_MASK_SET(0x26, 0x10, 0x10);
			RTV_REG_BURST_GET(0x29, pbBuf, 400);
			RTV_REG_SET(0x26, 0x01);

			/* FIC I2C interrupt status clear. */
			RTV_REG_SET(0x11, I2C_INTR_POL_ACTIVE|0x04);
			RTV_REG_SET(0x11, I2C_INTR_POL_ACTIVE);

			/* FIC I2C memory clear. */
			RTV_REG_MASK_SET(0x26, 0x04, 0x04);
			RTV_REG_MASK_SET(0x26, 0x04, 0x00);

			memmove(pbBuf+215, pbBuf+231, 169);
			ret_size = 384;
			break;
		}

		lock_s = tdmb_GetOfdmLockStatus();
		if (!(lock_s & RTV_TDMB_OFDM_LOCK_MASK)) {
			RTV_DELAY_MS(30);
			RTV_DBGMSG2("##lock_s(0x%02X)[%u]\n",
					lock_s, elapsed_cnt);
			ret_size = -55;
			break;
		}

		if (timeout_cnt--)
			RTV_DELAY_MS(1);
		else {
			ret_size = RTV_FIC_READ_TIMEOUT;
			break;
		}

		elapsed_cnt = RTV_TDMB_READ_FIC_TIMEOUT_CNT - timeout_cnt;
	}

	return ret_size;

#else
	RTV_REG_MAP_SEL(FEC_PAGE);

	RTV_REG_MASK_SET(0x26, 0x10, 0x10);
	RTV_REG_BURST_GET(0x29, pbBuf, 400);
	RTV_REG_SET(0x26, 0x01);

	/* FIC I2C interrupt status clear. */
	RTV_REG_SET(0x11, I2C_INTR_POL_ACTIVE|0x04);
	RTV_REG_SET(0x11, I2C_INTR_POL_ACTIVE);

	/* FIC I2C memory clear. */
	RTV_REG_MASK_SET(0x26, 0x04, 0x04);
	RTV_REG_MASK_SET(0x26, 0x04, 0x00);

	memmove(pbBuf+215, pbBuf+231, 169);

#if 1
RTV_DBGMSG("[READ] 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X [0x%02X 0x%02X] | 0x%02X 0x%02X\n",
pbBuf[0], pbBuf[1], pbBuf[2], pbBuf[3], pbBuf[4], pbBuf[5],
pbBuf[382], pbBuf[383], pbBuf[398], pbBuf[399]);
#endif

	return 384;
#endif
}
#endif

/**
 * NOTE: Do NOT read at the ISR.
 * This function should called when scan state.
 */
INT rtvTDMB_ReadFIC(U8 *pbBuf)
{
	int ret_size = 0;

	if (!g_fRtvFicOpened) {
		RTV_DBGMSG0("NOT OPEN FIC\n");
		return RTV_NOT_OPENED_FIC;
	}

	RTV_GUARD_LOCK;

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	ret_size = tdmb_ReadFIC_SPI(pbBuf);

#elif defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
	ret_size = tdmb_ReadFIC_I2C(pbBuf);
#endif

	RTV_GUARD_FREE;

	return ret_size;
}

void rtvTDMB_CloseFIC(void)
{
	if (!g_fRtvFicOpened)
		return;

	RTV_GUARD_LOCK;

	rtv_CloseFIC(g_nOpenedSubChNum);

	if (!g_nOpenedSubChNum) {
#if defined(TDMB_CIF_MODE_DRIVER)
		rtvCIFDEC_Deinit(); /* Skip rtvCIFDEC_DeleteSubChannelID() */
#elif defined(RTV_SCAN_FIC_HDR_ENABLED) && defined(RTV_MCHDEC_IN_DRIVER)
		rtvCIFDEC_Deinit();
#endif
		g_eTdmbState = TDMB_STATE_INIT; /* Update the state of TDMB. */
	}

	g_fRtvFicOpened = FALSE;

	RTV_GUARD_FREE;
}

INT rtvTDMB_OpenFIC(void)
{
	INT nRet = RTV_SUCCESS;

	/*RTV_DBGMSG1("[rtvTDMB_OpenFIC] g_eTdmbState(%d)\n", g_eTdmbState);*/

	if (g_fRtvFicOpened)
		return RTV_SUCCESS;

	RTV_GUARD_LOCK;

	nRet = rtv_OpenFIC(g_eTdmbState);
	if (nRet == RTV_SUCCESS)
		g_fRtvFicOpened = TRUE;

#if 0
#if defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
	RTV_DBGMSG1("Opened with FIC_state_path(%d)\n",
				g_nRtvFicOpenedStatePath);
#endif
#endif

	RTV_GUARD_FREE;

	return nRet;
}

static void tdmb_InitHOST(void)
{
	RTV_REG_MAP_SEL(HOST_PAGE);

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	RTV_REG_SET(0x05, (1<<7) | 0x00);
	RTV_REG_SET(0x04, 0x40); /* demod INT dis. */
#endif

#if defined(RTV_CHIP_PKG_CSP) && defined(RTV_FIC_I2C_INTR_ENABLED)
	RTV_DBGMSG2("0x08(0x%02X), 0x1A(0x%02X)\n",
		RTV_REG_GET(0x08), RTV_REG_GET(0x1A));

#if defined(RTV_FIC_I2C_INTR_ENABLED)
	RTV_REG_SET(0x04, 0x40);
	RTV_REG_SET(0x1A, 0x08); /* GPD3 PAD disable */
	RTV_REG_SET(0x08, (1<<5)|0x10); /* GPD3 => INT0 */
#else
	RTV_REG_SET(0x08, (3<<5)|0x10);
#endif

#else
#if defined(RTV_FIC_I2C_INTR_ENABLED)
	RTV_REG_SET(0x08, 0x10); /* INTR pin used */
#else
	RTV_REG_SET(0x08, 0x70); /* SYNC pin used */
#endif
#endif

	RTV_REG_SET(0x09, 0x00);
	RTV_REG_SET(0x10, 0xA0);
	RTV_REG_SET(0x13, 0x04);
	RTV_REG_SET(0x0B, 0xCE);

	RTV_REG_SET(0x0D, 0x17);

	RTV_REG_SET(0x19, 0x20);
	RTV_REG_SET(0x20, 0x19);

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	RTV_REG_SET(0x05, (1<<7) | 0x3F);

	RTV_REG_SET(0x07, 0x41);
	RTV_REG_SET(0x07, 0x40);

#else
	RTV_REG_SET(0x05, 0x3F);

	RTV_REG_SET(0x07, 0x01);
	RTV_REG_SET(0x07, 0x00);
#endif

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	/* <2> SPI_INT0(GPP0) disable  <4> I2C INT0 disable */
	RTV_REG_SET(0x1D, 0xC4);
#endif

#if defined(RTV_CHIP_PKG_CSP) && defined(RTV_FIC_I2C_INTR_ENABLED)
	#ifdef RTV_INTR_POLARITY_LOW_ACTIVE
	RTV_REG_SET(0x15, 0x04); /* 2013: default high */
	#else
	RTV_REG_SET(0x15, 0x00); /* 2013 */
	#endif
#endif
}

static void tdmb_InitOFDM(void)
{
	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_SET(0x10, 0xCB);
	RTV_REG_SET(0x1F, 0x08);

	RTV_REG_SET(0x31, 0xFF);
	RTV_REG_SET(0x32, 0xFF);

	rtvRF_InitOfdmTnco();

	RTV_REG_SET(0x3F, 0x00);
	RTV_REG_SET(0x6C, 0x0C);
	RTV_REG_SET(0x6F, 0x40);
	RTV_REG_SET(0x86, 0x20);
	RTV_REG_SET(0x87, 0x3F);

	RTV_REG_SET(0x40, 0x42);
	RTV_REG_SET(0x96, 0x02);
}

static void tdmb_InitFEC(void)
{
	RTV_REG_MAP_SEL(FEC_PAGE);

	RTV_REG_SET(0x2D, 0x80);
	RTV_REG_SET(0x2E, 0x5F);

	RTV_REG_SET(0x34, 0x00);
	RTV_REG_SET(0x37, 0x00);

	RTV_REG_SET(0xA8, 0x7F);

	RTV_REG_SET(0xAA, 0x00);
	RTV_REG_SET(0xB0, 0x07);
	RTV_REG_SET(0xD5, 0x80); /* DEFAULT Time Slice OFF. */

#if defined(RTV_SCAN_FIC_HDR_ENABLED) || defined(RTV_PLAY_FIC_HDR_ENABLED)\
|| defined(RTV_MSC_HDR_ENABLED)
	RTV_REG_SET(0xEA, RTV_MCH_HEADER_SYNC_BYTE); /* Header sync byte */
#endif

#ifdef RTV_FORCE_INSERT_SYNC_BYTE
	RTV_REG_SET(0xF8, 0x04|0x02);
#else
	RTV_REG_SET(0xF8, 0x04);
#endif

	RTV_REG_SET(0xFA, 0x07);

	/* all disable output */
	RTV_REG_SET(0xB2, 0x80); /* FIC:  */
	RTV_REG_SET(0xB3, 0xF6); /* MSC */
	RTV_REG_SET(0xB4, 0x86); /* TDMB */
	RTV_REG_SET(0xB5, 0xB6); /* FIDC, DABP */

	RTV_REG_SET(0xAA, 0x00); /* TSIF OFF */

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	#ifdef RTV_NULL_PID_GENERATE
	RTV_REG_SET(0xA4, 0x81|0x02);
	#else
	RTV_REG_SET(0xA4, 0x81);
	#endif

	#ifdef RTV_ERROR_TSP_OUTPUT_DISABLE
	RTV_REG_SET(0xA5, 0xC0);
	#else
	RTV_REG_SET(0xA5, 0x80);
	#endif

	RTV_REG_SET(0xAF, 0x00);
	RTV_REG_SET(0xB0, 0x04);
#else
	rtv_ConfigureTsifFormat();
	RTV_REG_SET(0xB0, 0x00|RTV_FEC_TSIF_OUT_SPEED);
#endif

	RTV_REG_SET(0xAA, 0x7F);

#if defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0x16, 0xFF);
	RTV_REG_SET(0x17, 0xFF); /* I2C intr disable */
#endif

	RTV_REG_SET(0xD3, 0x00);

}

void rtvOEM_ConfigureInterrupt(void)
{
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	RTV_REG_MAP_SEL(SPI_CTRL_PAGE);
	RTV_REG_SET(0x21, SPI_INTR_POL_ACTIVE|0x02);

	#ifdef RTV_IF_SPI
	RTV_REG_SET(0x27, 0x00); /* AUTO_INTR: 0 */
	#else
	RTV_REG_SET(0x27, 0x02); /* AUTO_INTR: 0 */
	#endif
	RTV_REG_SET(0x2B, RTV_SPI_INTR_DEACT_PRD_VAL);

	RTV_REG_SET(0x2A, 1); /* SRAM init */
	RTV_REG_SET(0x2A, 0);
#else
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0x11, I2C_INTR_POL_ACTIVE);
#endif
}

static void tdmb_InitDemod(void)
{
	tdmb_InitHOST();
	tdmb_InitOFDM();
	tdmb_InitFEC();
	rtvOEM_ConfigureInterrupt();
}

INT rtvTDMB_Initialize(unsigned long interface)
{
	INT nRet;

#if defined(RTV_IF_SPI) || defined(RTV_IF_TSIF)
	mtv319_set_port_if(interface);
#endif
	g_nOpenedSubChNum = 0;
	g_nRegSubChArrayIdxBits = 0x0;

	g_aRegSubChIdBits[0] = 0x00000000;
	g_aRegSubChIdBits[1] = 0x00000000;

	g_nUsedHwSubChIdxBits = 0x00;

	g_nTdmbPrevAntennaLevel = 0;

	g_eTdmbState = TDMB_STATE_INIT;

	g_fRtvFicOpened = FALSE;

	g_fTdmbFastScanEnabled = FALSE;

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	g_nRtvInterruptLevelSize = 0;
#elif defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
	g_nRtvFicOpenedStatePath = FIC_NOT_OPENED;
#endif

	nRet = rtv_InitSystem();
	if (nRet != RTV_SUCCESS)
		return nRet;

	/* Must after rtv_InitSystem(). */
	tdmb_InitDemod();

	nRet = rtvRF_Initilize();
	if (nRet != RTV_SUCCESS)
		goto tdmb_init_exit;

	rtv_SoftReset();

tdmb_init_exit:

	return RTV_SUCCESS;
}



