/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef CRC32_H
#define CRC32_H

/* unit of count is 2byte */
unsigned long getCRC(volatile unsigned short *mem, signed long size,
	volatile unsigned short *crcH, volatile unsigned short *crcL);

#endif
