/*
 * drivers/media/platform/exynos/mfc/mfc_perf_measure.c
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "mfc_perf_measure.h"

#ifndef PERF_MEASURE

void mfc_perf_register(struct mfc_dev *dev) {}
void __mfc_measure_init(void) {}
void __mfc_measure_on(struct mfc_dev *dev) {}
void __mfc_measure_off(struct mfc_dev *dev) {}
void __mfc_measure_store(struct mfc_dev *dev, int diff) {}
void mfc_perf_print(void) {}

#else

#endif
