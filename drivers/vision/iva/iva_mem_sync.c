/* iva_mem_sync.c
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 * Authors:
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <asm/cacheflush.h>
#include <linux/ion.h>
#include <linux/dma-mapping.h>

#include "iva_mem.h"

/* [INTERNAL USE ONLY] threshold value for whole cache flush */
#ifndef SZ_8M
#define SZ_8M				0x800000
#endif

#define ION_FLUSH_ALL_HIGHLIMIT		SZ_8M

void iva_ion_sync_sg_for_cpu(struct iva_dev_data *iva, struct iva_mem_map *iva_map_node)
{
	struct device *dev = iva->dev;

	if (!(GET_IVA_CACHE_TYPE(iva_map_node->flags) & ION_FLAG_CACHED)) {
		dev_warn(dev, "no need to sync (no set cacheflag)\n");
		return;
	}

	if (!iva_map_node->sg_table) {
		iva_map_node->sg_table = ion_sg_table(iva->ion_client,
				iva_map_node->handle);
		if (!iva_map_node->sg_table) {
			dev_err(dev, "%s() sg_table is null\n",	__func__);
			return;
		}
	}

	if (iva_map_node->act_size >= ION_FLUSH_ALL_HIGHLIMIT)
		flush_cache_all();
	else
		dma_sync_sg_for_cpu(NULL, iva_map_node->sg_table->sgl,
			      iva_map_node->sg_table->nents, DMA_BIDIRECTIONAL);
}

void iva_ion_sync_sg_for_device(struct iva_dev_data *iva, struct iva_mem_map *iva_map_node, bool clean_only)
{
	struct device *dev = iva->dev;

	if (!(GET_IVA_CACHE_TYPE(iva_map_node->flags) & ION_FLAG_CACHED)) {
		dev_warn(dev, "no need to sync (no set cacheflag)\n");
		return;
	}

	if (iva_map_node->act_size >= ION_FLUSH_ALL_HIGHLIMIT)
		flush_cache_all();
	else {
		if (!iva_map_node->sg_table) {
			iva_map_node->sg_table = ion_sg_table(iva->ion_client,
					iva_map_node->handle);
			if (!iva_map_node->sg_table) {
				dev_err(dev, "%s() sg_table is null\n",	__func__);
				return;
			}
		}

		dma_sync_sg_for_device(NULL, iva_map_node->sg_table->sgl,
			       iva_map_node->sg_table->nents, DMA_BIDIRECTIONAL);
	}
}
