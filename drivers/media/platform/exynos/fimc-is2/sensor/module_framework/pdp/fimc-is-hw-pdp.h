/*
 * Samsung Exynos SoC series FIMC-IS2 driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_PDP_H
#define FIMC_IS_HW_PDP_H

enum pdp_event_type {
	PE_START,
	PE_END,
	PE_PAF_STAT0,
	PE_PAF_STAT1,
	PE_PAF_STAT2,
};

void pdp_hw_s_sensor_type(void __iomem *base, u32 sensor_type);
void pdp_hw_s_core(void __iomem *base, bool enable);
unsigned int pdp_hw_g_irq_state(void __iomem *base, bool clear);
unsigned int pdp_hw_g_irq_mask(void __iomem *base);
int pdp_hw_g_stat0(void __iomem *base, void *buf, size_t len);
void pdp_hw_s_config_default(void __iomem *base);

bool pdp_hw_to_sensor_type(u32 pd_mode, u32 *sensor_type);
bool pdp_hw_is_occured(unsigned int state, enum pdp_event_type type);
int pdp_hw_dump(void __iomem *base);
#endif
