/*
 * Samsung Exynos SoC series SCore driver
 *
 * Debug and Performance Monitoring Unit (DPMU)
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_DPMU_H__
#define __SCORE_DPMU_H__

void score_dpmu_init(void __iomem *sfr);
void score_dpmu_enable(void);
void score_dpmu_disable(void);

void score_dpmu_debug_control(int on);
int score_dpmu_debug_check(void);
void score_dpmu_performance_control(int on);
int score_dpmu_performance_check(void);

void score_dpmu_print_all(void);
void score_dpmu_print_debug(void);
void score_dpmu_print_pc(int count);
void score_dpmu_print_userdefined(void);
void score_dpmu_print_performance(void);
void score_dpmu_print_system(void);
void score_dpmu_print_target(void);

#endif
