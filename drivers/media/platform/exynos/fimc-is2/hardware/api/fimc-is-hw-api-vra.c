/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-api-vra.h"
#if defined(CONFIG_VRA_V2_0)
#include "sfr/fimc-is-sfr-vra-v20.h"
#else
#include "sfr/fimc-is-sfr-vra-v11.h"
#endif

u32 fimc_is_vra_chain0_get_all_intr(void __iomem *base_addr)
{
	return fimc_is_hw_get_field(base_addr,
			&vra_chain0_regs[VRA_R_CHAIN0_CURRENT_INT],
			&vra_chain0_fields[VRA_F_CHAIN0_CURRENT_INT]);
}

u32 fimc_is_vra_chain0_get_status_intr(void __iomem *base_addr)
{
	return fimc_is_hw_get_field(base_addr,
			&vra_chain0_regs[VRA_R_CHAIN0_STATUS_INT],
			&vra_chain0_fields[VRA_F_CHAIN0_STATUS_INT]);
}

u32 fimc_is_vra_chain0_get_enable_intr(void __iomem *base_addr)
{
	return fimc_is_hw_get_field(base_addr,
			&vra_chain0_regs[VRA_R_CHAIN0_ENABLE_INT],
			&vra_chain0_fields[VRA_F_CHAIN0_ENABLE_INT]);
}

void fimc_is_vra_chain0_set_clear_intr(void __iomem *base_addr, u32 value)
{
	fimc_is_hw_set_field(base_addr,
			&vra_chain0_regs[VRA_R_CHAIN0_CLEAR_INT],
			&vra_chain0_fields[VRA_F_CHAIN0_CLEAR_INT],
			value);
}

u32 fimc_is_vra_chain1_get_all_intr(void __iomem *base_addr)
{
	return fimc_is_hw_get_field(base_addr,
			&vra_chain1_regs[VRA_R_CHAIN1_CURRENT_INT],
			&vra_chain1_fields[VRA_F_CHAIN1_CURRENT_INT]);
}

u32 fimc_is_vra_chain1_get_status_intr(void __iomem *base_addr)
{
	return fimc_is_hw_get_field(base_addr,
			&vra_chain1_regs[VRA_R_CHAIN1_STATUS_INT],
			&vra_chain1_fields[VRA_F_CHAIN1_STATUS_INT]);
}

u32 fimc_is_vra_chain1_get_enable_intr(void __iomem *base_addr)
{
	return fimc_is_hw_get_field(base_addr,
			&vra_chain1_regs[VRA_R_CHAIN1_ENABLE_INT],
			&vra_chain1_fields[VRA_F_CHAIN1_ENABLE_INT]);
}


void fimc_is_vra_chain1_set_clear_intr(void __iomem *base_addr, u32 value)
{
	fimc_is_hw_set_field(base_addr,
			&vra_chain1_regs[VRA_R_CHAIN1_CLEAR_INT],
			&vra_chain1_fields[VRA_F_CHAIN1_CLEAR_INT],
			value);
}

u32 fimc_is_vra_chain1_get_image_mode(void __iomem *base_addr)
{
	return fimc_is_hw_get_field(base_addr,
			&vra_chain1_regs[VRA_R_CHAIN1_IMAGE_MODE],
			&vra_chain1_fields[VRA_F_CHAIN1_IMAGE_MODE]);
}
