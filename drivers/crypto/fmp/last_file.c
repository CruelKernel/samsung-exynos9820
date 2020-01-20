/*
 * Last file for Exynos FMP FIPS integrity check
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/init.h>

__attribute__ ((section(".rodata"), unused))
const unsigned char last_fmp_rodata = 0x20;

__attribute__ ((section(".text"), unused))
void last_fmp_text(void){}

#ifdef CC_USE_CLANG
__attribute__ ((section(".init.text"), unused))
#else
__attribute__ ((section(".init.text"), optimize("-O0"), unused))
#endif
void last_fmp_init(void){};
