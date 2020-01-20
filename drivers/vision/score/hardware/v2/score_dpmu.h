/*
 * Samsung Exynos SoC series SCore driver
 *
 * Debug and Performance Monitoring Unit (DPMU)
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
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

void score_dpmu_debug_enable_on(int on);
int score_dpmu_debug_enable_check(void);
void score_dpmu_performance_enable_on(int on);
int score_dpmu_performance_enable_check(void);
void score_dpmu_stall_enable_on(int on);
int score_dpmu_stall_enable_check(void);

void score_dpmu_print_all(void);
void score_dpmu_print_dbg_intr(void);
void score_dpmu_print_monitor_status(void);
void score_dpmu_print_mc_cache(void);
void score_dpmu_print_kc1_cache(void);
void score_dpmu_print_kc2_cache(void);
void score_dpmu_print_axi(void);
void score_dpmu_print_dma0(void);
void score_dpmu_print_dma1(void);
void score_dpmu_print_instruction_count(void);
void score_dpmu_print_interrupt_count(void);
void score_dpmu_print_slave_err(void);
void score_dpmu_print_decode_err(void);
void score_dpmu_print_pc(int count);

void score_dpmu_print_system(void);
void score_dpmu_print_stall_count(void);
void score_dpmu_print_target(void);
void score_dpmu_print_debug(void);

#endif
