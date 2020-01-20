/*
 * Samsung Exynos SoC series FIMC-IS2 driver
 *
 * exynos fimc-is2 hw csi control functions
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-pdp-v1_0.h"
#include "fimc-is-hw-pdp.h"
#include "fimc-is-device-sensor.h"

static int pdp_hw_s_irq_msk(void __iomem *base, u32 msk)
{
	fimc_is_hw_set_reg(base, &pdp_regs[PDP_R_PDP_INT], 0);
	fimc_is_hw_set_reg(base, &pdp_regs[PDP_R_PDP_INT_MASK], msk);

	return 0;
}

void pdp_hw_s_sensor_type(void __iomem *base, u32 sensor_type)
{
	fimc_is_hw_set_field(base, &pdp_regs[PDP_R_SENSOR_TYPE],
			&pdp_fields[PDP_F_SENSOR_TYPE], sensor_type);
}

void pdp_hw_s_core(void __iomem *base, bool enable)
{
	if (enable)
		pdp_hw_s_irq_msk(base, 0x1F & ~PDP_INT_FRAME_PAF_STAT0);
	else
		pdp_hw_s_irq_msk(base, 0x1F);

	fimc_is_hw_set_field(base, &pdp_regs[PDP_R_PDP_CORE_ENABLE],
			&pdp_fields[PDP_F_PDP_CORE_ENABLE], enable);
}

unsigned int pdp_hw_g_irq_state(void __iomem *base, bool clear)
{
	u32 src;

	src = fimc_is_hw_get_reg(base, &pdp_regs[PDP_R_PDP_INT]);
	if (clear)
		fimc_is_hw_set_reg(base, &pdp_regs[PDP_R_PDP_INT], src);

	return src;
}

unsigned int pdp_hw_g_irq_mask(void __iomem *base)
{
	return fimc_is_hw_get_reg(base, &pdp_regs[PDP_R_PDP_INT_MASK]);
}

int pdp_hw_g_stat0(void __iomem *base, void *buf, size_t len)
{
	/* PDP_R_PAF_STAT0_END be excepted from STAT0 */
	size_t stat0_len = pdp_regs[PDP_R_PAF_STAT0_END].sfr_offset
			- pdp_regs[PDP_R_PAF_STAT0_START].sfr_offset;

	if (len < stat0_len) {
		stat0_len = len;
		warn("the size of STAT0 buffer is too small: %zd < %zd",
							len, stat0_len);
	}

	memcpy((void *)buf, base + pdp_regs[PDP_R_PAF_STAT0_START].sfr_offset,
								stat0_len);

	return stat0_len;
}

void pdp_hw_s_config_default(void __iomem *base)
{
	int i;
	u32 index;
	u32 value;
	int count = ARRAY_SIZE(pdp_global_init_table);

	for (i = 0; i < count; i++) {
		index = pdp_global_init_table[i].index;
		value = pdp_global_init_table[i].init_value;
		fimc_is_hw_set_reg(base, &pdp_regs[index], value);
	}
}

bool pdp_hw_to_sensor_type(u32 pd_mode, u32 *sensor_type)
{
	bool enable;

	switch (pd_mode) {
#if defined(CONFIG_SOC_EXYNOS9820) && !defined(CONFIG_SOC_EXYNOS9820_EVT0)
	case PD_MSPD_TAIL:
#endif
	case PD_MSPD:
		*sensor_type = SENSOR_TYPE_MSPD;
		enable = true;
		break;
	case PD_MOD1:
		*sensor_type = SENSOR_TYPE_MOD1;
		enable = true;
		break;
	case PD_MOD2:
#if !defined(CONFIG_SOC_EXYNOS9820) || defined(CONFIG_SOC_EXYNOS9820_EVT0)
	case PD_MSPD_TAIL:
#endif
	case PD_NONE:
		*sensor_type = SENSOR_TYPE_MOD2;
		enable = false;
		break;
	case PD_MOD3:
		*sensor_type = SENSOR_TYPE_MOD3;
		enable = true;
		break;
	default:
		warn("PD MODE(%d) is invalid", pd_mode);
		*sensor_type = SENSOR_TYPE_MOD2;
		enable = false;
		break;
	}

	return enable;
}

bool pdp_hw_is_occured(unsigned int state, enum pdp_event_type type)
{
	return state & (1 << type);
}

int pdp_hw_dump(void __iomem *base)
{
	info("PDP SFR DUMP (v1.0)\n");
	fimc_is_hw_dump_regs(base, pdp_regs, PDP_REG_CNT);

	return 0;
}
