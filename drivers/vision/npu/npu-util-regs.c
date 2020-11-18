/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/platform_device.h>
#include <linux/delay.h>

#include "npu-log.h"
#include "npu-device.h"
#include "npu-system.h"
#include "npu-util-regs.h"

void npu_write_hw_reg(const struct npu_iomem_area *base, u32 offset, u32 val, u32 mask)
{
	volatile u32 v;
	void __iomem *reg_addr;

	BUG_ON(!base);
	BUG_ON(!base->vaddr);

	if (offset > base->size) {
		npu_err("offset(%u) exceeds iomem region size(%llu), starting at (%u)\n",
				offset, base->size, base->paddr);
		BUG_ON(1);
	}
	reg_addr = base->vaddr + offset;
	v = readl(reg_addr);
	npu_dbg("setting register pa(0x%08x) va(%pK) cur(0x%08x) val(0x%08x) mask(0x%08x)\n",
		base->paddr + offset, reg_addr,	v, val, mask);

	v = (v & (~mask)) | (val & mask);
	writel(v, (void *)(reg_addr));
	npu_dbg("written (0x%08x) at (%pK)\n", v, reg_addr);
}

/*
 * Set the set of register value specified in set_map,
 * with the base specified at base.
 * if regset_mdelay != 0, it will be the delay between register set.
 */
int npu_set_hw_reg(const struct npu_iomem_area *base, const struct reg_set_map *set_map,
		      size_t map_len, int regset_mdelay)
{
	size_t i;

	BUG_ON(!base);
	BUG_ON(!set_map);
	BUG_ON(!base->vaddr);

	for (i = 0; i < map_len; ++i) {
		npu_write_hw_reg(base, set_map[i].offset, set_map[i].val, set_map[i].mask);
		/* Insert delay between register setting */
		if (regset_mdelay > 0)
			mdelay(regset_mdelay);
	}
	return 0;
}

int npu_set_hw_reg_2(const struct reg_set_map_2 *set_map, size_t map_len, int regset_mdelay)
{
	size_t i;

	BUG_ON(!set_map);

	for (i = 0; i < map_len; ++i) {
		npu_write_hw_reg(set_map[i].iomem_area, set_map[i].offset, set_map[i].val, set_map[i].mask);

		/* Insert delay between register setting */
		if (regset_mdelay > 0)
			mdelay(regset_mdelay);
	}
	return 0;
}

int npu_set_sfr(const u32 sfr_addr, const u32 value, const u32 mask)
{
	int			ret;
	void __iomem		*iomem = NULL;
	struct npu_iomem_area	area_info;	/* Save iomem result */

	iomem = ioremap_nocache(sfr_addr, sizeof(u32));
	if (IS_ERR_OR_NULL(iomem)) {
		probe_err("fail(%pK) in ioremap_nocache(0x%08x)\n",
			  iomem, sfr_addr);
		ret = -EFAULT;
		goto err_exit;
	}
	area_info.vaddr = iomem;
	area_info.paddr = sfr_addr;
	area_info.size = sizeof(u32);

	npu_write_hw_reg(&area_info, 0, value, mask);

	ret = 0;
err_exit:
	if (iomem)
		iounmap(iomem);

	return ret;
}

int npu_get_sfr(const u32 sfr_addr)
{
	int			ret = 0;
	void __iomem		*iomem = NULL;
	struct npu_iomem_area	area_info;	/* Save iomem result */
	volatile u32 v;
	void __iomem *reg_addr;

	iomem = ioremap_nocache(sfr_addr, sizeof(u32));
	if (IS_ERR_OR_NULL(iomem)) {
		probe_err("fail(%pK) in ioremap_nocache(0x%08x)\n",
			  iomem, sfr_addr);
		ret = -EFAULT;
		goto err_exit;
	}
	area_info.vaddr = iomem;
	area_info.paddr = sfr_addr;
	area_info.size = sizeof(u32);

	reg_addr = area_info.vaddr;

	v = readl(reg_addr);
	npu_trace("get_sfr, vaddr(0x%pK), paddr(0x%08x), val(0x%x)\n",
		area_info.vaddr, area_info.paddr, v);

	ret = 0;
err_exit:
	if (iomem)
		iounmap(iomem);

	return ret;
}

