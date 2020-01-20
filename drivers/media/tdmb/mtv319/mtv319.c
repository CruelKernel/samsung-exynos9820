/*
 *
 * File name: mtv319.c
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

U8 g_bRtvIntrMaskReg;
U8 g_bRtvPage;


int REV_ID;

INT rtv_InitSystem(void)
{
	int i;
	U8 read0, read1;

	g_bRtvIntrMaskReg = 0xFF;

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	#ifdef RTV_IF_SPI
	#define WR27_VAL	0x10
	#else
	#define WR27_VAL	0x12
	#endif

	#define WR29_VAL	0x31

	RTV_REG_MAP_SEL(SPI_CTRL_PAGE);
	for (i = 0; i < 100; i++) {
		RTV_REG_SET(0x29, WR29_VAL); /* BUFSEL first! */
		RTV_REG_SET(0x27, WR27_VAL);

		read0 = RTV_REG_GET(0x27);
		read1 = RTV_REG_GET(0x29);
		RTV_DBGMSG2("read0(0x%02X), read1(0x%02X)\n", read0, read1);

		if ((read0 == WR27_VAL) && (read1 == WR29_VAL))
			goto RTV_POWER_ON_SUCCESS;

		RTV_DBGMSG1("Power On wait: %d\n", i);
		RTV_DELAY_MS(5);
	}
#else
	RTV_REG_MAP_SEL(HOST_PAGE);
	for (i = 0; i < 100; i++) {
		RTV_REG_SET(0x05, 0x00);
		RTV_REG_SET(0x08, 0x10);

		read0 = RTV_REG_GET(0x05);
		read1 = RTV_REG_GET(0x08);
		RTV_DBGMSG2("read0(0x%02X), read1(0x%02X)\n", read0, read1);

		if ((read0 == 0x00) && (read1 == 0x10))
			goto RTV_POWER_ON_SUCCESS;

		RTV_REG_SET(0x07, 0x00);

		RTV_DBGMSG1("Power On wait: %d\n", i);
		RTV_DELAY_MS(5);
	}
#endif

	RTV_DBGMSG1("Power On Check error: %d\n", i);
	return RTV_POWER_ON_CHECK_ERROR;

RTV_POWER_ON_SUCCESS:

	return RTV_SUCCESS;
}


