/*
 *
 * File name: mtv319_cifdec.h
 *
 * Description : MTV319 CIF decoder header file.
 *
 * Copyright (C) (2014, RAONTECH)
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

/******************************************************************************
 * REVISION HISTORY
 *
 *    DATE         NAME          REMARKS
 * ----------  -------------    ------------------------------------------------
 * 07/12/2012  Ko, Kevin        Created.
 ******************************************************************************/

#ifndef __MTV319_CIFDEC_H__
#define __MTV319_CIFDEC_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "mtv319.h"

#define RTV_CIFDEC_INVALID_BUF_IDX	0xFF

struct RTV_CIF_DEC_INFO {
	unsigned int fic_size; /* Result size. */

	/* Source buffer address to be decoded. (Input/Output) */
	unsigned char *fic_buf_ptr;

	/* Decoded MSC sub channel size (Output) */
	unsigned int subch_size[RTV_MAX_NUM_USE_SUBCHANNEL];

	/* Decoded MSC sub channel ID.(Output) */
	unsigned int subch_id[RTV_MAX_NUM_USE_SUBCHANNEL];

	/* Source MSC buffer address to be decoded. (Input/Output) */
	unsigned char *subch_buf_ptr[RTV_MAX_NUM_USE_SUBCHANNEL];

	/* Source MSC buffer size. (Input) */
	unsigned int subch_buf_size[RTV_MAX_NUM_USE_SUBCHANNEL];
};

int rtvCIFDEC_Decode(struct RTV_CIF_DEC_INFO *ptDecInfo,
			const U8 *pbTsBuf, UINT nTsLen);
UINT rtvCIFDEC_SetDiscardTS(int nFicMscType, U8 *pbTsBuf, UINT nTsLen);
UINT rtvCIFDEC_GetDecBufIndex(UINT nSubChID);
void rtvCIFDEC_DeleteSubChannelID(UINT nSubChID);
BOOL rtvCIFDEC_AddSubChannelID(UINT nSubChID,
				enum E_RTV_SERVICE_TYPE eServiceType);
void rtvCIFDEC_Deinit(void);
void rtvCIFDEC_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __MTV319_CIFDEC_H__ */


