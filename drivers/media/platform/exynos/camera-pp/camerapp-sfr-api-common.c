/*
 * Samsung EXYNOS CAMERA PostProcessing driver
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>

#include "camerapp-sfr-api-common.h"

/* #define DEBUG_CAMERA_PP_HW_SFR */

int camerapp_log_level;
module_param_named(camerapp_log_level, camerapp_log_level, uint, 0644);

u32 camerapp_sfr_get_reg(void __iomem *base_addr, const struct camerapp_sfr_reg *reg)
{
	u32 reg_value;

	reg_value = readl(base_addr + reg->sfr_offset);

	camerapp_dbg("[GET_REG] reg:[%s][0x%04X], reg_value(R):[0x%08X]\n",
		reg->reg_name, reg->sfr_offset, reg_value);

	return reg_value;
}

void camerapp_sfr_set_reg(void __iomem *base_addr, const struct camerapp_sfr_reg *reg, u32 val)
{
	camerapp_dbg("[SET_REG] reg:[%s][0x%04X], reg_value(W):[0x%08X]\n",
		reg->reg_name, reg->sfr_offset, val);

	writel(val, base_addr + reg->sfr_offset);
	return;
}

u32 camerapp_sfr_get_field(void __iomem *base_addr, const struct camerapp_sfr_reg *reg,
	const struct camerapp_sfr_field *field)
{
	u32 reg_value;
	u32 field_value;

	reg_value = readl(base_addr + reg->sfr_offset);
	field_value = camerapp_sfr_get_field_value(reg_value, field);

	camerapp_dbg("[GET_FIELD] reg:[%s][0x%04X], field:[%s] reg_value(R):[0x%08X] val(R):[%d]\n",
		reg->reg_name, reg->sfr_offset, field->field_name, reg_value, field_value);

	return field_value;
}

void camerapp_sfr_set_field(void __iomem *base_addr, const struct camerapp_sfr_reg *reg,
	const struct camerapp_sfr_field *field, u32 val)
{
	u32 reg_value;

	/* previous value reading */
	reg_value = readl(base_addr + (reg->sfr_offset));

	reg_value = camerapp_sfr_set_field_value(reg_value, field, val);

	camerapp_dbg("[SET_FIELD] reg:[%s][0x%04X], field:[%s] reg_value(W):[0x%08X] val(W):[%d]\n",
		reg->reg_name, reg->sfr_offset, field->field_name, reg_value, val);

	/* store reg value */
	writel(reg_value, base_addr + (reg->sfr_offset));
	return;
}

u32 camerapp_sfr_get_field_value(u32 reg_value, const struct camerapp_sfr_field *field)
{
	u32 field_mask = 0;
	u32 field_value = 0;

	field_mask = (field->bit_width >= 32) ? 0xFFFFFFFF : ((1 << field->bit_width) - 1);
	field_value = (reg_value >> (field->bit_start)) & (field_mask);

	return field_value;
}

u32 camerapp_sfr_set_field_value(u32 reg_value, const struct camerapp_sfr_field *field, u32 val)
{
	u32 field_mask = 0;

	field_mask = (field->bit_width >= 32) ? 0xFFFFFFFF : ((1 << field->bit_width) - 1);

	/* bit clear */
	reg_value &= ~(field_mask << field->bit_start);

	/* setting value */
	reg_value |= (val & field_mask) << (field->bit_start);

	return reg_value;
}

void camerapp_sfr_dump_regs(void __iomem *base_addr, const struct camerapp_sfr_reg *regs, u32 total_cnt)
{
	u32 i = 0;
	u32 reg_value = 0;

	for(i = 0; i < total_cnt; i++) {
		reg_value = readl(base_addr + regs[i].sfr_offset);

		camerapp_dbg("[DUMP] reg:[%s][0x%04X], value:[0x%08X]\n",
			regs[i].reg_name, regs[i].sfr_offset, reg_value);
	}
	return;
}
