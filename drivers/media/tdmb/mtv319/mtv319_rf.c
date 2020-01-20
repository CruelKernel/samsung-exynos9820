/*
 *
 * File name: mtv319_rf.c
 *
 * Description : MTV319 RF services source driver.
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
#include "mtv319_rf_adc_data.h"

/* Down conversion Signal Monitoring. */
/* #define DEBUG_A_TEST_ZERO */

static const struct RTV_REG_INIT_INFO t_TDMB_INIT[] = {
	{0x21, 0x08},
	{0x22, 0x6c},
	{0x23, 0x54},
	{0x25, 0x0F}, /* FEBD[9:8], REFD[5:0] */
	{0x26, 0xE8}, /* FEBD[7:0] */
	{0x32, 0xff},
	{0x36, 0x88},
	{0x38, 0x49},
	{0x39, 0x31},
	{0x3A, 0xD0},
	{0x3B, 0x88},
	{0x3e, 0x71},
	{0x42, 0x06},
	{0x43, 0x4F},
	{0x44, 0x60},
	{0x45, 0x9A},
	{0x46, 0xd7},
	{0x48, 0x65},
	{0x49, 0x2c},
	{0x4a, 0x24},
	{0x4e, 0x60},
	{0x4f, 0x50},
	{0x51, 0x06},
	{0x52, 0x44},
	{0x56, 0xd8},
	{0x57, 0x86},
	{0x58, 0x1a},
	{0x59, 0x20},
	{0x63, 0x0c},
	{0x64, 0x0c},
	{0x65, 0x4c},
	{0x66, 0x0c},
	{0x67, 0x0c},
	{0x68, 0x4c},
	{0x69, 0x0c},
	{0x6a, 0x0c},
	{0x6b, 0x4c},
	{0x6d, 0x1e},
	{0x6e, 0x1e},
	{0x6f, 0x1e},
	{0x70, 0x1e},
	{0x71, 0x1e},
	{0x72, 0x1e},
	{0x73, 0x1f},
	{0x74, 0x1f},
	{0x78, 0x08},
	{0x7a, 0x07},
	{0x7b, 0x03},
	{0x7d, 0x08},
	{0x86, 0x84},
	{0x87, 0x34},
	{0x8e, 0xae},
	{0x8f, 0x94},
	{0x93, 0x1b},
	{0x94, 0xc3},
	{0x95, 0x70},
	{0x97, 0x7f},
	{0x99, 0x5c},
	{0x9a, 0xbf},
	{0x9b, 0xc6},
	{0x9d, 0x96},
	{0x9f, 0xd7},
	{0xa7, 0x11}
};

UINT g_dwRtvPrevChFreqKHz;


void rtvRF_InitOfdmTnco(void)
{
	const struct RTV_ADC_CFG_INFO *ptCfgTbl	= &g_atAdcCfgTbl_TDMB[0];

	RTV_REG_SET(0x38, ptCfgTbl->dwTNCO & 0xFF);
	RTV_REG_SET(0x39, (ptCfgTbl->dwTNCO>>8) & 0xFF);
	RTV_REG_SET(0x3A, (ptCfgTbl->dwTNCO>>16) & 0xFF);
	RTV_REG_SET(0x3B, (ptCfgTbl->dwTNCO>>24) & 0xFF);
}

static INT rtvRF_ChangeAdcClock(UINT nChIdx)
{
	UINT nRetryCnt = 10;
	U8 RD02;
	const struct RTV_ADC_CFG_INFO *ptCfgTbl	= &g_atAdcCfgTbl_TDMB[nChIdx];

	RTV_REG_MAP_SEL(RF_PAGE);
	RTV_REG_SET(0x25, (((ptCfgTbl->wFEBD&0x300)>>8)<<6) | ptCfgTbl->bREFD);
	RTV_REG_SET(0x26, ptCfgTbl->wFEBD & 0xFF);

	while (1) {
		RTV_DELAY_MS(1);

		RD02 = RTV_REG_GET(0x02);
		if (RD02 & 0x80)
			break;

		if (--nRetryCnt == 0) {
			RTV_DBGMSG0("Syn Unlocked!\n");
			return RTV_ADC_CLK_UNLOCKED;
		}
	}

	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_SET(0x38, ptCfgTbl->dwTNCO & 0xFF);
	RTV_REG_SET(0x39, (ptCfgTbl->dwTNCO>>8) & 0xFF);
	RTV_REG_SET(0x3A, (ptCfgTbl->dwTNCO>>16) & 0xFF);
	RTV_REG_SET(0x3B, (ptCfgTbl->dwTNCO>>24) & 0xFF);

	return RTV_SUCCESS;
}

INT rtvRF_SetFrequency(U32 dwChFreqKHz)
{
	UINT nChIdx, nRetryCnt = 10;
	U8 WR32, WR2A, RD00;
	U32 dwPLLN, dwPLLF, dwPLLNF;
	INT nRet;
	U32 PLLFREQkHz = dwChFreqKHz << 2; /* x4 */
#if (RTV_SRC_CLK_FREQ_KHz == 13000) || (RTV_SRC_CLK_FREQ_KHz == 27000)
	#define pllf_mul	1
	#define r_div		3
#else
	#define pllf_mul	0
	#define r_div		4
#endif

	switch (dwChFreqKHz) {
	case 175280: /* 7A */
	case 177008: /* 7B */
	case 178736: /* 7C */
	case 181280: /* 8A */
	case 183008: /* 8B */
	case 184736: /* 8C */
	case 187280: /* 9A */
	case 189008: /* 9B */
	case 190736: /* 9C */
	case 193280: /* 10A */
	case 195008: /* 10B */
	case 196736: /* 10C */
	case 199280: /* 11A */
	case 201008: /* 11B */
	case 202736: /* 11C */
	case 205280: /* 12A */
	case 207008: /* 12B */
	case 208736: /* 12C */
	case 211280: /* 13A */
	case 213008: /* 13B */
	case 214736: /* 13C */
		break;

	default:
		RTV_DBGMSG1("Invalid freqency(%u)\n", dwChFreqKHz);
		return RTV_INVALID_FREQENCY;
	}

	nChIdx = (dwChFreqKHz - TDMB_FREQ_START__KOREA) / TDMB_FREQ_STEP__KOREA;
	if (dwChFreqKHz >= 205280)
		nChIdx -= 2;
	else if (dwChFreqKHz >= 193280)
		nChIdx -= 1;

	nRet = rtvRF_ChangeAdcClock(nChIdx);
	if (nRet != RTV_SUCCESS)
		goto RF_SET_FREQ_ERR;

	/* Set the PLLNF and channel. */
	dwPLLN = PLLFREQkHz / RTV_SRC_CLK_FREQ_KHz;
	dwPLLF = PLLFREQkHz - (dwPLLN * RTV_SRC_CLK_FREQ_KHz);
	dwPLLNF = (dwPLLN<<20)
		+ (((dwPLLF<<16) / (RTV_SRC_CLK_FREQ_KHz>>r_div)) << pllf_mul);

	RTV_REG_MAP_SEL(RF_PAGE);
	RTV_REG_SET(0x21, (dwPLLNF>>22) & 0xFF);
	RTV_REG_SET(0x22, (dwPLLNF>>14) & 0xFF);
	RTV_REG_SET(0x23, (dwPLLNF>>6) & 0xFF);

	/* SAR */
	WR32 = RTV_REG_GET(0x32) & 0xFD;
	WR2A = RTV_REG_GET(0x2A) & 0x3F;
	RTV_REG_SET(0x32, (WR32 | 0x02)); /* ENVCOCAL_I2C = 1 */
	RTV_REG_SET(0x2A, (WR2A | 0x80)); /* RECONF_I2C = 1 */
	RTV_REG_SET(0x2A, (WR2A | 0xC0)); /* INIT_VCOCAL_I2C = 1 */
	RTV_DELAY_MS(1);
	RTV_REG_SET(0x2A, (WR2A | 0x80)); /* INIT_VCOCAL_I2C = 0 */
	RTV_REG_SET(0x2A, (WR2A | 0x00)); /* RECONF_I2C = 0 */

	while (1) {
		RTV_DELAY_MS(1);

		RD00 = RTV_REG_GET(0x00);
		if (RD00 & 0x20) {
			RTV_DELAY_MS(1);
			RD00 = RTV_REG_GET(0x00);
			if (RD00 & 0x20) {
				RTV_DELAY_MS(1);
				RD00 = RTV_REG_GET(0x00);
				if (RD00 & 0x20) {
					RTV_DELAY_MS(1);
					RD00 = RTV_REG_GET(0x00);
					if (RD00 & 0x20)
						break;
				}
			}
		}

		RTV_REG_SET(0x2A, (WR2A | 0x80)); /* RECONF_I2C = 1 */
		RTV_REG_SET(0x2A, (WR2A | 0xC0)); /* INIT_VCOCAL_I2C = 1 */
		RTV_DELAY_MS(1);
		RTV_REG_SET(0x2A, (WR2A | 0x80)); /* INIT_VCOCAL_I2C = 1 */
		RTV_REG_SET(0x2A, (WR2A | 0x00)); /* RECONF_I2C = 1 */

		if (--nRetryCnt == 0) {
			RTV_DBGMSG0("PLL Unlocked!\n");
			return RTV_PLL_UNLOCKED;
		}
	}

	g_dwRtvPrevChFreqKHz = dwChFreqKHz;

	return RTV_SUCCESS;

RF_SET_FREQ_ERR:
	RTV_DBGMSG1("Error(%d)\n", nRet);
	return nRet;
}

INT rtvRF_Initilize(void)
{
	UINT nNumTblEntry;
	const struct RTV_REG_INIT_INFO *ptInitTbl;
#ifdef DEBUG_A_TEST_ZERO
	U8 bA8;
#endif

	ptInitTbl = t_TDMB_INIT;
	nNumTblEntry = sizeof(t_TDMB_INIT) / sizeof(struct RTV_REG_INIT_INFO);

	RTV_REG_MAP_SEL(RF_PAGE);
	do {
		RTV_REG_SET(ptInitTbl->bReg, ptInitTbl->bVal);
		ptInitTbl++;
	} while (--nNumTblEntry);

#ifdef DEBUG_A_TEST_ZERO
	bA8 = RTV_REG_GET(0xA8);
	bA8 |= 0x01;
	RTV_REG_SET(0xA8, bA8);
#endif

	g_dwRtvPrevChFreqKHz = 0;

	return RTV_SUCCESS;
}

