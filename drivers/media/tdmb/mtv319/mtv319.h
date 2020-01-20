/*
 *
 * File name: mtv319.h
 *
 * Description : MTV319 device driver API header file.
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

#ifndef __MTV319_H__
#define __MTV319_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "mtv319_port.h"

/*
 *
 * Common definitions and types.
 *
 *
 */
#define MTV319_CHIP_ID			0xC6
#define MTV319_FIC_BUF_SIZE		(3 * 188)
#define MTV319_SPI_CMD_SIZE		3

#ifndef NULL
	#define NULL		0
#endif

#ifndef FALSE
	#define FALSE		0
#endif

#ifndef TRUE
	#define TRUE		1
#endif

#ifndef MAX
	#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
	#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#endif

#ifndef ABS
	#define ABS(x)		(((x) < 0) ? -(x) : (x))
#endif


/* Error codes. */
#define RTV_SUCCESS				0
#define RTV_POWER_ON_CHECK_ERROR		-1
#define RTV_ADC_CLK_UNLOCKED			-2
#define RTV_PLL_UNLOCKED			-3
#define RTV_CHANNEL_NOT_DETECTED		-4
#define RTV_NOT_OPENED_FIC			-5
#define RTV_INVAILD_SUBCHANNEL_ID		-6
#define RTV_NO_MORE_SUBCHANNEL			-7
#define RTV_ALREADY_OPENED_SUBCHANNEL_ID	-8
#define RTV_INVAILD_THRESHOLD_SIZE		-9
#define RTV_OPENING_SERVICE_FULL		-10
#define RTV_INVAILD_SERVICE_TYPE		-11
#define RTV_FIC_READ_TIMEOUT			-12
#define RTV_SHOULD_CLOSE_SUBCHANNELS		-13
#define RTV_SHOULD_CLOSE_FIC			-14
#define RTV_INVALID_FIC_OPEN_STATE		-15
#define RTV_INVALID_FREQENCY			-16

/* Do not modify the value! */
enum E_RTV_SERVICE_TYPE {
	RTV_SERVICE_DMB = 0, /* DMB Video */
	RTV_SERVICE_DAB = 1, /* DAB */
	RTV_SERVICE_DABPLUS = 2 /* DAB+ */
};

/* Cont flag of multi channel header */
enum RTV_MCH_HDR_CONT_FLAG_T {
	RTV_MCH_HDR_CONT_MED = 0,
	RTV_MCH_HDR_CONT_LAST = 1,
	RTV_MCH_HDR_CONT_FIRST = 2,
	RTV_MCH_HDR_CONT_ALONE = 3
};

/* ID flag of multi channel header */
enum RTV_MCH_HDR_ID_FLAG_T {
	RTV_MCH_HDR_ID_DAB = 0,
	RTV_MCH_HDR_ID_DMB = 1,
	RTV_MCH_HDR_ID_FIDC = 2,
	RTV_MCH_HDR_ID_FIC = 3,
	MAX_NUM_MCH_HDR_ID_TYPE
};

#define RTV_MCH_HDR_SIZE	4 /* Size, in bytes,  of MCH header */
#define MAX_MCH_PAYLOAD_SIZE	(RTV_TSP_XFER_SIZE - RTV_MCH_HDR_SIZE)

/*
 *
 * TDMB definitions, types and APIs.
 *
 *
 */
#define RTV_TDMB_AGC_LOCK_MASK		0x1
#define RTV_TDMB_OFDM_LOCK_MASK		0x2
#define RTV_TDMB_FEC_LOCK_MASK		0x4

#define RTV_TDMB_CHANNEL_LOCK_OK	\
	(RTV_TDMB_AGC_LOCK_MASK|RTV_TDMB_OFDM_LOCK_MASK|RTV_TDMB_FEC_LOCK_MASK)

#define RTV_TDMB_BER_DIVIDER		100000
#define RTV_TDMB_CNR_DIVIDER		1000
#define RTV_TDMB_RSSI_DIVIDER		1000

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
extern UINT g_nRtvInterruptLevelSize;
static INLINE UINT rtvTDMB_GetInterruptLevelSize(void)
{
	return g_nRtvInterruptLevelSize;
}
#endif

INT rtvTDMB_OpenSubChannelExt(U32 dwChFreqKHz, UINT nSubChID,
		enum E_RTV_SERVICE_TYPE eServiceType, UINT nThresholdSize);

UINT rtvTDMB_GetLockStatus(void);
UINT rtvTDMB_GetAntennaLevel(U32 dwCER);
U32  rtvTDMB_GetFicCER(void);
U32  rtvTDMB_GetCER(void);
U32  rtvTDMB_GetPER(void);
S32  rtvTDMB_GetRSSI(void);
U32  rtvTDMB_GetCNR(void);
U32  rtvTDMB_GetBER(void);
U32  rtvTDMB_GetPreviousFrequency(void);
UINT rtvTDMB_GetOpenedSubChannelCount(void);
void rtvTDMB_CloseAllSubChannels(void);
INT  rtvTDMB_CloseSubChannel(UINT nSubChID);
INT  rtvTDMB_OpenSubChannel(U32 dwChFreqKHz, UINT nSubChID,
		enum E_RTV_SERVICE_TYPE eServiceType, UINT nThresholdSize);
INT  rtvTDMB_ReadFIC(U8 *pbBuf);
void rtvTDMB_CloseFIC(void);
INT  rtvTDMB_OpenFIC(void);
INT  rtvTDMB_ScanFrequency(U32 dwChFreqKHz);
INT  rtvTDMB_Initialize(unsigned long interface);
UINT tdmb_GetOfdmLockStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* __MTV319_H__ */

