/*
 * Copyright (C) 2012-2016 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/errno.h>	/* cma.h needs this */
#if defined(CONFIG_DMA_CMA)
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
#elif defined(CONFIG_CMA)
#include <linux/cma.h>
#endif
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/highmem.h>
#include <linux/mm.h>

#include "sysdep.h"
#include "tzdev.h"

static dma_addr_t tzdev_cma_addr = 0;

#if defined(CONFIG_DMA_CMA)
static struct page *tzdev_page = NULL;
#endif /* CONFIG_DMA_CMA */

int tzdev_cma_mem_register(void)
{
	void *ptr;
	struct page *p;
	unsigned long pfn;

	if (!tzdev_cma_addr)
		return 0;

	for (pfn = __phys_to_pfn(tzdev_cma_addr);
		pfn < __phys_to_pfn(tzdev_cma_addr +
			CONFIG_TZDEV_MEMRESSZ - CONFIG_TZDEV_MEMRESSZPROT);
		pfn++) {
		p = pfn_to_page(pfn);
		ptr = kmap(p);
		/*
		 * We can just invalidate here, but kernel doesn't
		 * export cache invalidation functions.
		 */
		__flush_dcache_area(ptr, PAGE_SIZE);
		kunmap(p);
	}
	outer_inv_range(tzdev_cma_addr, tzdev_cma_addr +
		CONFIG_TZDEV_MEMRESSZ - CONFIG_TZDEV_MEMRESSZPROT);

	return tzdev_smc_mem_reg((unsigned long)__phys_to_pfn(tzdev_cma_addr),
		(unsigned long)(get_order(CONFIG_TZDEV_MEMRESSZ) + PAGE_SHIFT));
}

void tzdev_cma_mem_init(struct device *dev)
{
#if defined(CONFIG_DMA_CMA)
	static struct page *tzdev_page = NULL;

	tzdev_page = dma_alloc_from_contiguous(dev,
			(CONFIG_TZDEV_MEMRESSZ - CONFIG_TZDEV_MEMRESSZPROT) >> PAGE_SHIFT,
			get_order(CONFIG_TZDEV_MEMRESSZ));
	if (!tzdev_page) {
		tzdev_print(0, "Allocation CMA region failed;"
			" Memory will not be registered in SWd\n");
		return;
	}
	tzdev_cma_addr = page_to_phys(tzdev_page);
#elif defined(CONFIG_CMA)
	struct cma_info tzdev_cma_info;

	if (cma_info(&tzdev_cma_info, dev, NULL) < 0) {
		tzdev_print(0, "Getting CMA info failed;"
			" Memory will not be registered in SWd\n");
		return;
	}
	tzdev_cma_addr = cma_alloc(dev, NULL,
		tzdev_cma_info.total_size, 0);
#endif /* CONFIG_CMA */
	if (!tzdev_cma_addr || IS_ERR_VALUE(tzdev_cma_addr)) {
		tzdev_print(0, "Allocation CMA region failed;"
			" Memory will not be registered in SWd\n");
		return;
	}
}

void tzdev_cma_mem_release(struct device *dev)
{
	if (tzdev_cma_addr) {
#if defined(CONFIG_DMA_CMA)
		dma_release_from_contiguous(dev, tzdev_page,
				(CONFIG_TZDEV_MEMRESSZ - CONFIG_TZDEV_MEMRESSZPROT) >> PAGE_SHIFT);
#elif defined(CONFIG_CMA)
		cma_free(tzdev_cma_addr);
#endif
	}
	tzdev_cma_addr = 0;
}
