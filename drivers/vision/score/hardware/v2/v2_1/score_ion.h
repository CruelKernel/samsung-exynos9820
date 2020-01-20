/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_ION_H__
#define __SCORE_ION_H__

#include <linux/exynos_ion.h>

struct score_memory;

int score_ion_alloc(void *alloc_ctx, size_t size,
		dma_addr_t *dvaddr, void **kvaddr);
void score_ion_free(void *alloc_ctx);

int score_ion_create_context(struct score_memory *mem);
void score_ion_destroy_context(void *alloc_ctx);

#endif
