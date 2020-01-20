/*
 *
 * File name: mtv319_rf.h
 *
 * Description : MTV319 T-DMB services header file.
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

#ifndef __MTV319_RF_H__
#define __MTV319_RF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "mtv319_internal.h"

extern UINT g_dwRtvPrevChFreqKHz;

INT rtvRF_SetFrequency(U32 dwFreqKHz);
void rtvRF_InitOfdmTnco(void);
INT rtvRF_Initilize(void);

#ifdef __cplusplus
}
#endif

#endif /* __MTV319_RF_H__ */

