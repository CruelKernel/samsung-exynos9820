/*
 * drivers/media/platform/exynos/mfc/mfc_mmcache.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/smc.h>

#include "mfc_mmcache.h"

#include "mfc_reg_api.h"

static const unsigned char mmcache_SFR_0x0010[] = {
	0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x2A, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x00, 0x00, 0x37, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char mmcache_SFR_0x0040[] = {
	0x0E, 0x00, 0x1E, 0x00, 0x0E, 0x00, 0x1E, 0x00, 0x0E, 0x00, 0x1E, 0x00, 0x0E, 0x00, 0x1E, 0x00,
	0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
	0x10, 0x00, 0x1F, 0x00, 0x10, 0x00, 0x1F, 0x00, 0x10, 0x00, 0x1F, 0x00, 0x10, 0x00, 0x1F, 0x00,
	0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
	0x0C, 0x00, 0x1F, 0x00, 0x0C, 0x00, 0x1F, 0x00, 0x0C, 0x00, 0x1F, 0x00, 0x0C, 0x00, 0x1F, 0x00,
	0x12, 0x00, 0x3F, 0x00, 0x08, 0x00, 0x3F, 0x00, 0x0C, 0x00, 0x3D, 0x00, 0x0C, 0x00, 0x3D, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static void mmcache_print_config(struct mfc_dev *dev)
{
	void __iomem *addr;
	unsigned int size;

	mfc_debug_enter();

	if (!mmcache_dump)
		return;

	size = sizeof(mmcache_SFR_0x0010);
	addr = dev->mmcache.base + MMCACHE_WAY0_CTRL;

	print_hex_dump(KERN_ERR, "[MMCACHE] Config:", DUMP_PREFIX_ADDRESS, 32, 4, addr, size, false);

	size = sizeof(mmcache_SFR_0x0040);
	addr = dev->mmcache.base + MMCACHE_MASTER_GRP0_RPATH0;

	print_hex_dump(KERN_ERR, "[MMCACHE] Config:", DUMP_PREFIX_ADDRESS, 32, 4, addr, size, false);

	mfc_debug_leave();
}

static void mmcache_set_config(struct mfc_dev *dev)
{
	unsigned int data, i, size;
	const unsigned int *sfr_dump;

	mfc_debug_enter();

	size = sizeof(mmcache_SFR_0x0010);
	sfr_dump = (const unsigned int *)mmcache_SFR_0x0010;

	for (i = 0; i < size; i += 4) {
		data = sfr_dump[i / 4];
		MMCACHE_WRITEL(data, MMCACHE_WAY0_CTRL + i);
	}

	size = sizeof(mmcache_SFR_0x0040);
	sfr_dump = (const unsigned int *)mmcache_SFR_0x0040;

	for (i = 0; i < size; i += 4) {
		data = sfr_dump[i / 4];
		MMCACHE_WRITEL(data, MMCACHE_MASTER_GRP0_RPATH0 + i);
	}

	mfc_debug(2, "[MMCACHE] mmcache config setting is done\n");

	mfc_debug_leave();
}

static void mmcache_reset_config(struct mfc_dev *dev)
{
	void __iomem *addr;
	unsigned int data;

	mfc_debug_enter();

	addr = dev->mmcache.base + MMCACHE_MASTER_GRP_CTRL2;
	data = 0;

	mfc_debug(2, "[MMCACHE] before write 0x%X: (0x%08llX) 0x%X\n",
			data, (unsigned long long)(addr),
			MMCACHE_READL(MMCACHE_MASTER_GRP_CTRL2));

	MMCACHE_WRITEL(data, MMCACHE_MASTER_GRP_CTRL2);

	mfc_debug(2, "[MMCACHE] after write 0x%X: (0x%08llX) 0x%X\n",
			data, (unsigned long long)(addr),
			MMCACHE_READL(MMCACHE_MASTER_GRP_CTRL2));

	mfc_debug_leave();
}

static void mmcache_update_master_grp(struct mfc_dev *dev)
{
	void __iomem *addr;
	unsigned int data;

	mfc_debug_enter();

	addr = dev->mmcache.base + MMCACHE_GLOBAL_CTRL;
	data = MMCACHE_GLOBAL_CTRL_VALUE;

	mfc_debug(2, "[MMCACHE] before write 0x%X: (0x%08llX) 0x%X\n",
			data, (unsigned long long)(addr),
			MMCACHE_READL(MMCACHE_GLOBAL_CTRL));

	MMCACHE_WRITEL(data, MMCACHE_GLOBAL_CTRL);

	mfc_debug(2, "[MMCACHE] after write 0x%X: (0x%08llX) 0x%X\n",
			data, (unsigned long long)(addr),
			MMCACHE_READL(MMCACHE_GLOBAL_CTRL));

	mfc_debug_leave();
}

static void mmcache_enable_clock_gating(struct mfc_dev *dev)
{
	void __iomem *addr;
	unsigned int data;

	mfc_debug_enter();

	addr = dev->mmcache.base + MMCACHE_CG_CONTROL;
	data = MMCACHE_CG_CONTROL_VALUE;

	mfc_debug(2, "[MMCACHE] before write 0x%X: (0x%08llX) 0x%X\n",
			data, (unsigned long long)(addr),
			MMCACHE_READL(MMCACHE_CG_CONTROL));

	MMCACHE_WRITEL(data, MMCACHE_CG_CONTROL);

	mfc_debug(2, "[MMCACHE] after write 0x%X: (0x%08llX) 0x%X\n",
			data, (unsigned long long)(addr),
			MMCACHE_READL(MMCACHE_CG_CONTROL));

	mfc_debug_leave();
}

void mfc_mmcache_enable(struct mfc_dev *dev)
{
	mfc_debug_enter();

	if (mmcache_disable)
		return;

	mmcache_set_config(dev);
	mmcache_print_config(dev);
	mmcache_update_master_grp(dev);
	mmcache_enable_clock_gating(dev);

	dev->mmcache.is_on_status = 1;
	mfc_info_dev("[MMCACHE] enabled\n");
	MFC_TRACE_DEV("[MMCACHE] enabled\n");

	mfc_debug_leave();
}

void mfc_mmcache_disable(struct mfc_dev *dev)
{
	mfc_debug_enter();

	mmcache_reset_config(dev);
	mmcache_update_master_grp(dev);

	dev->mmcache.is_on_status = 0;
	mfc_info_dev("[MMCACHE] disabled\n");
	MFC_TRACE_DEV("[MMCACHE] disabled\n");

	mfc_debug_leave();
}

void mfc_mmcache_dump_info(struct mfc_dev *dev)
{
	if (dev->has_mmcache) {
		pr_err("-----------dumping MMCACHE registers (SFR base = 0x%#lx)\n", (unsigned long)dev->mmcache.base);
		print_hex_dump(KERN_ERR, "[MMCACHE] ", DUMP_PREFIX_ADDRESS, 32, 4, dev->mmcache.base, 0x10, false);
	}

	if (dev->has_cmu) {
		pr_err("-----------dumping CMU BUSC registers (SFR base = 0x%#lx)\n", (unsigned long)dev->cmu_busc_base);
		/* PLL_CON0_MUX_CLKCMU_BUSC_BUS_USER (0x140) */
		print_hex_dump(KERN_ERR, "[MMCACHE][BUSC] ", DUMP_PREFIX_ADDRESS, 32, 4, dev->cmu_busc_base + 0x140, 0xc, false);
		/* CMU_BUSC (0x60ec) */
		print_hex_dump(KERN_ERR, "[MMCACHE][BUSC] ", DUMP_PREFIX_ADDRESS, 32, 4, dev->cmu_busc_base + 0x60e0, 0x10, false);
		/* DBG_NFO_QCH_CON_MMCACHE_QCH (0x7184) */
		print_hex_dump(KERN_ERR, "[MMCACHE][BUSC] ", DUMP_PREFIX_ADDRESS, 32, 4, dev->cmu_busc_base + 0x7180, 0x10, false);

		pr_err("-----------dumping CMU MIF0~3 registers (SFR base = 0x%#lx)\n", (unsigned long)dev->cmu_mif0_base);
		/* CMU_MIF0 (0x7018 ~ 0x7024) */
		print_hex_dump(KERN_ERR, "[MMCACHE][MIF0] ", DUMP_PREFIX_ADDRESS, 32, 4, dev->cmu_mif0_base + 0x7018, 0x10, false);
		/* CMU_MIF1 (0x7018 ~ 0x7024) */
		print_hex_dump(KERN_ERR, "[MMCACHE][MIF1] ", DUMP_PREFIX_ADDRESS, 32, 4, dev->cmu_mif1_base + 0x7018, 0x10, false);
		/* CMU_MIF2 (0x7018 ~ 0x7024) */
		print_hex_dump(KERN_ERR, "[MMCACHE][MIF2] ", DUMP_PREFIX_ADDRESS, 32, 4, dev->cmu_mif2_base + 0x7018, 0x10, false);
		/* CMU_MIF3 (0x7018 ~ 0x7024) */
		print_hex_dump(KERN_ERR, "[MMCACHE][MIF3] ", DUMP_PREFIX_ADDRESS, 32, 4, dev->cmu_mif3_base + 0x7018, 0x10, false);
	}
}

void mfc_invalidate_mmcache(struct mfc_dev *dev)
{
	int ret;

	mfc_debug_enter();

	/* The secure OS can flush all normal and secure data */
	ret = exynos_smc(SMC_CMD_MM_CACHE_OPERATION, MMCACHE_GROUP2, 0x0, 0x0);
	if (ret != DRMDRV_OK) {
		mfc_err_dev("[MMCACHE] Fail to invalidation 0x%x\n", ret);
		mfc_mmcache_dump_info(dev);
		call_dop(dev, dump_and_stop_debug_mode, dev);
	}
	mfc_debug(2, "[MMCACHE] invalidated\n");
	MFC_TRACE_DEV("[MMCACHE] invalidated\n");

	mfc_debug_leave();
}
