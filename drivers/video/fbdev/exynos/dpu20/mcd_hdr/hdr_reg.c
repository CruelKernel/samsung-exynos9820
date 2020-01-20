/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Samsung EXYNOS9SoC series HDR driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/io.h>

#include "hdr_drv.h"
#include "hdr_reg.h"


u32 mcd_reg_read(struct mcd_hdr_device *hdr, u32 reg_id)
{
	return readl(hdr->regs + reg_id);
}

void mcd_reg_write(struct mcd_hdr_device *hdr, u32 reg_id, u32 val)
{
	//hdr_info("write : addr : 0x%x, value : 0x%x\n", hdr->regs + reg_id, val);	
	return writel(val, hdr->regs + reg_id);
}

u32 mcd_reg_read_mask(struct mcd_hdr_device *hdr, u32 reg_id, u32 mask)
{
	u32 val = mcd_reg_read(hdr, reg_id);

	val &= mask;

	return val;
}

void mcd_reg_write_mask(struct mcd_hdr_device *hdr, u32 reg_id, u32 val, u32 mask)
{
	u32 old = mcd_reg_read(hdr, reg_id);

	val = (val & mask) | (old & ~mask);

	writel(val, hdr->regs + reg_id);
}

u32 mcd_reg_writes(struct mcd_hdr_device *hdr, u32 start_id, u32 *val, u32 size)
{
	u32 i, id = start_id;

	if (val == NULL) {
		hdr_err("HDR:ERR:%s: val is null\n", __func__);
		return 0;
	}

	for (i = 0; i < size; i++) {
		mcd_reg_write(hdr, id, val[i]);
		id += 4;
	}
	return size;
}




