/*
 *
 * File name: mtv319_rf_adc_data.h
 *
 * Description : MTV319 T-DMB RF ADC data header file.
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

#if (RTV_SRC_CLK_FREQ_KHz == 19200)
	static const struct RTV_ADC_CFG_INFO g_atAdcCfgTbl_TDMB[] = {
		/* FEBD REFD	TNCO */
		{0x051, 0x04, 0x40BA16D6} /* 175280: 7A */,
		{0x051, 0x04, 0x40BA16D6} /* 177008: 7B */,
		{0x052, 0x04, 0x3FF003D4} /* 178736: 7C */,
		{0x051, 0x04, 0x40BA16D6} /* 181280: 8A */,
		{0x052, 0x04, 0x3FF003D4} /* 183008: 8B */,
		{0x053, 0x04, 0x3F2ACF59} /* 184736: 8C */,
		{0x052, 0x04, 0x3FF003D4} /* 187280: 9A */,
		{0x053, 0x04, 0x3F2ACF59} /* 189008: 9B */,
		{0x054, 0x04, 0x3E6A4CE1} /* 190736: 9C */,
		{0x055, 0x04, 0x3DAE5200} /* 193280: 10A */,
		{0x050, 0x04, 0x41893720} /* 195008: 10B */,
		{0x055, 0x04, 0x3DAE5200} /* 196736: 10C */,
		{0x04F, 0x04, 0x425D95CC} /* 199280: 11A */,
		{0x050, 0x04, 0x41893720} /* 201008: 11B */,
		{0x050, 0x04, 0x41893720} /* 202736: 11C */,
		{0x051, 0x04, 0x40BA16D6} /* 205280: 12A */,
		{0x052, 0x04, 0x3FF003D4} /* 207008: 12B */,
		{0x053, 0x04, 0x3F2ACF59} /* 208736: 12C */,
		{0x054, 0x04, 0x3E6A4CE1} /* 211280: 13A */,
		{0x054, 0x04, 0x3E6A4CE1} /* 213008: 13B */,
		{0x053, 0x04, 0x3F2ACF59} /* 214736: 13C */
	};

#elif (RTV_SRC_CLK_FREQ_KHz == 24000)
	static const struct RTV_ADC_CFG_INFO g_atAdcCfgTbl_TDMB[] = {
		/* FEBD REFD	TNCO */
		{0x0ED, 0x0F, 0x425D95EB} /* 175280: 7A */,
		{0x0AE, 0x0B, 0x424A0EF3} /* 177008: 7B */,
		{0x062, 0x06, 0x4032D2BD} /* 178736: 7C */,
		{0x080, 0x08, 0x4189374B} /* 181280: 8A */,
		{0x0EA, 0x0E, 0x3EBC3DE2} /* 183008: 8B */,
		{0x067, 0x06, 0x3D150476} /* 184736: 8C */,
		{0x0FE, 0x0F, 0x3DEC7D07} /* 187280: 9A */,
		{0x066, 0x06, 0x3DAE5229} /* 189008: 9B */,
		{0x0B0, 0x0A, 0x3B9403A3} /* 190736: 9C */,
		{0x0CB, 0x0C, 0x3DFC1B3B} /* 193280: 10A */,
		{0x0CD, 0x0C, 0x3D614B5D} /* 195008: 10B */,
		{0x080, 0x08, 0x4189374B} /* 196736: 10C */,
		{0x07F, 0x08, 0x420D51EF} /* 199280: 11A */,
		{0x080, 0x08, 0x4189374B} /* 201008: 11B */,
		{0x0E6, 0x0D, 0x3B44702D} /* 202736: 11C */,
		{0x0EF, 0x0F, 0x41CF69DA} /* 205280: 12A */,
		{0x0B7, 0x0B, 0x3F0776D7} /* 207008: 12B */,
		{0x0B3, 0x0B, 0x407008EB} /* 208736: 12C */,
		{0x0B7, 0x0B, 0x3F0776D7} /* 211280: 13A */,
		{0x0BA, 0x0B, 0x3E033748} /* 213008: 13B */,
		{0x0BA, 0x0B, 0x3E033748} /* 214736: 13C */
	};

#elif (RTV_SRC_CLK_FREQ_KHz == 24576)
	static const struct RTV_ADC_CFG_INFO g_atAdcCfgTbl_TDMB[] = {
		/* FEBD REFD    TNCO */
		{0x0E8, 0x0F, 0x4234F722} /* 175280: 7A */,
		{0x0AA, 0x0B, 0x42424238} /* 177008: 7B */,
		{0x060, 0x06, 0x3FFFFFF6} /* 178736: 7C */,
		{0x07D, 0x08, 0x41893742} /* 181280: 8A */,
		{0x0E5, 0x0E, 0x3E9A4546} /* 183008: 8B */,
		{0x063, 0x06, 0x3E0F83D7} /* 184736: 8C */,
		{0x0F8, 0x0F, 0x3DEF7BD5} /* 187280: 9A */,
		{0x063, 0x06, 0x3E0F83D7} /* 189008: 9B */,
		{0x0AC, 0x0A, 0x3B88EE1B} /* 190736: 9C */,
		{0x0C6, 0x0C, 0x3E0F83D7} /* 193280: 10A */,
		{0x0C8, 0x0C, 0x3D70A3CE} /* 195008: 10B */,
		{0x07D, 0x08, 0x41893742} /* 196736: 10C */,
		{0x07C, 0x08, 0x42108417} /* 199280: 11A */,
		{0x07D, 0x08, 0x41893742} /* 201008: 11B */,
		{0x0E1, 0x0D, 0x3B2A18FF} /* 202736: 11C */,
		{0x0EF, 0x0F, 0x40448D7C} /* 205280: 12A */,
		{0x0B3, 0x0B, 0x3EED685D} /* 207008: 12B */,
		{0x0AF, 0x0B, 0x405D9F6A} /* 208736: 12C */,
		{0x0B3, 0x0B, 0x3EED685D} /* 211280: 13A */,
		{0x0B6, 0x0B, 0x3DE3DE34} /* 213008: 13B */,
		{0x0B6, 0x0B, 0x3DE3DE34} /* 214736: 13C */
	};
#else
	#error "Unsupport clock freqency!"
#endif
