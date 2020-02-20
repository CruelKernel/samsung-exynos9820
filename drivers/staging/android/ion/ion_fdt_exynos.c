/*
 * drivers/staging/android/ion/ion_fdt_exynos.c
 *
 * Copyright (C) 2018 Samsung Electronics Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/cma.h>
#include <linux/kmemleak.h>

#include "ion.h"
#include "ion_exynos.h"

unsigned int reserved_mem_count __initdata;
struct ion_reserved_mem_struct {
	char		*heapname;
	struct cma	*cma;
	phys_addr_t	base;
	phys_addr_t	size;
	unsigned int	alloc_align;
	unsigned int	protection_id;
	bool		secure;
	bool		recyclable;
	bool		untouchable;
} ion_reserved_mem[ION_NUM_HEAP_IDS - 1] __initdata;

static int __init exynos_ion_reserved_mem_setup(struct reserved_mem *rmem)
{
	bool untch, reusable, secure, recyclable;
	size_t alloc_align = PAGE_SIZE;
	char *heapname;
	const __be32 *prop;
	__u32 protection_id = 0;
	int len;

	reusable = !!of_get_flat_dt_prop(rmem->fdt_node, "ion,reusable", NULL);
	untch = !!of_get_flat_dt_prop(rmem->fdt_node, "ion,untouchable", NULL);
	secure = !!of_get_flat_dt_prop(rmem->fdt_node, "ion,secure", NULL);
	recyclable = !!of_get_flat_dt_prop(rmem->fdt_node, "ion,recyclable", NULL);

	prop = of_get_flat_dt_prop(rmem->fdt_node, "ion,protection_id", &len);
	if (prop)
		protection_id = be32_to_cpu(prop[0]);

	prop = of_get_flat_dt_prop(rmem->fdt_node, "ion,alignment", &len);
	if (prop && (be32_to_cpu(prop[0]) >= PAGE_SIZE)) {
		alloc_align = be32_to_cpu(prop[0]);
		if ((alloc_align & (alloc_align - 1)) != 0) {
			long order = get_order(alloc_align);

			alloc_align = 1UL << (order + PAGE_SHIFT);
		}
	}

	prop = of_get_flat_dt_prop(rmem->fdt_node, "ion,heapname", &len);
	if (!prop) {
		perrfn("'ion,heapname' is missing in '%s' node", rmem->name);
		return -EINVAL;
	}
	heapname = (char *)prop;

	if (reserved_mem_count == ARRAY_SIZE(ion_reserved_mem)) {
		perrfn("Not enough reserved_mem slot for %s", rmem->name);
		return -ENOMEM;
	}

	if (untch && reusable) {
		perrfn("'reusable', 'untouchable' should not be together");
		return -EINVAL;
	}

	if (reusable) {
		struct cma *cma;
		int ret;

		ret = cma_init_reserved_mem(rmem->base, rmem->size, 0,
					    heapname, &cma);
		if (ret < 0) {
			perrfn("failed to init cma for '%s'", heapname);
			return ret;
		}

		ion_reserved_mem[reserved_mem_count].cma = cma;

		kmemleak_ignore_phys(rmem->base);
		rmem->reusable = true;
	}

	ion_reserved_mem[reserved_mem_count].base = rmem->base;
	ion_reserved_mem[reserved_mem_count].size = rmem->size;
	ion_reserved_mem[reserved_mem_count].heapname = heapname;
	ion_reserved_mem[reserved_mem_count].alloc_align = (unsigned int)alloc_align;
	ion_reserved_mem[reserved_mem_count].protection_id = protection_id;
	ion_reserved_mem[reserved_mem_count].secure = secure;
	ion_reserved_mem[reserved_mem_count].recyclable = recyclable;
	ion_reserved_mem[reserved_mem_count].untouchable = untch;
	reserved_mem_count++;

	return 0;
}

RESERVEDMEM_OF_DECLARE(ion, "exynos9820-ion", exynos_ion_reserved_mem_setup);

#define MAX_HPA_EXCEPTION_AREAS 4

static int hpa_num_exception_areas;
static phys_addr_t hpa_alloc_exceptions[MAX_HPA_EXCEPTION_AREAS][2];

static bool __init register_hpa_heap(struct device_node *np,
				     unsigned int prot_id_map)
{
	struct ion_platform_heap pheap;
	struct ion_heap *heap;
	u32 align;

	if (of_property_read_string(np, "ion,heapname", &pheap.name)) {
		perrfn("failed to read ion,heapname in '%s'", np->name);
		return false;
	}

	pheap.secure = of_property_read_bool(np, "ion,secure");

	if (pheap.secure) {
		if (of_property_read_u32(np, "ion,protection_id", &pheap.id)) {
			perrfn("failed to read ion,protection_id in '%s'", np->name);
			return false;
		}

		if (pheap.id > 32) {
			perrfn("too large protection id %d of '%s'",
			       pheap.id, pheap.name);
			return false;
		}

		if ((1 << pheap.id) & prot_id_map) {
			perrfn("protection_id %d in '%s' already exists",
			       pheap.id, np->name);
			return false;
		}
	}

	if (!of_property_read_u32(np, "ion,alignment", &align))
		pheap.align = align;
	else
		pheap.align = SZ_64K;

	pheap.type = ION_HEAP_TYPE_HPA;
	heap = ion_hpa_heap_create(&pheap, hpa_alloc_exceptions,
				   hpa_num_exception_areas);
	if (IS_ERR(heap)) {
		perrfn("failed to register '%s' heap",
		       pheap.name);
		return false;
	}

	ion_device_add_heap(heap);
	pr_info("ION: registered '%s' heap\n", pheap.name);

	return pheap.secure;

}

static bool __init exynos_ion_register_hpa_heaps(unsigned int prot_id_map)
{
	struct device_node *np, *child;
	bool secure = false;

	for_each_node_by_name(np, "ion-hpa-heap") {
		int naddr = of_n_addr_cells(np);
		int nsize = of_n_size_cells(np);
		phys_addr_t base, size;
		const __be32 *prop;
		int len;
		int i;

		/*
		 * If 'ion-hpa-heap' node defines its own range properties,
		 * override the range properties defined by its parent.
		 */
		prop = of_get_property(np, "#address-cells", NULL);
		if (prop)
			naddr = be32_to_cpup(prop);

		prop = of_get_property(np, "#size-cells", NULL);
		if (prop)
			nsize = be32_to_cpup(prop);

		prop = of_get_property(np, "ion,hpa_limit", &len);
		if (prop && len > 0 &&
		    hpa_num_exception_areas < MAX_HPA_EXCEPTION_AREAS) {
			base = (phys_addr_t)of_read_number(prop, naddr);

			i = hpa_num_exception_areas++;
			hpa_alloc_exceptions[i][0] = base;
			hpa_alloc_exceptions[i][1] = -1;
		}

		prop = of_get_property(np, "ion,hpa_alloc_exception", &len);
		if (prop && len > 0) {
			int n_area = len / (sizeof(*prop) * (nsize + naddr));

			n_area += hpa_num_exception_areas;
			n_area = min(n_area, MAX_HPA_EXCEPTION_AREAS);

			for (i = hpa_num_exception_areas; i < n_area ; i++) {
				base = (phys_addr_t)of_read_number(prop, naddr);
				prop += naddr;
				size = (phys_addr_t)of_read_number(prop, nsize);
				prop += nsize;

				hpa_alloc_exceptions[i][0] = base;
				hpa_alloc_exceptions[i][1] = base + size - 1;
			}

			hpa_num_exception_areas = n_area;
		}

		for_each_child_of_node(np, child)
			if (of_device_is_compatible(child, "exynos9820-ion"))
				secure |= register_hpa_heap(child, prot_id_map);
	}

	return secure;
}

static int __init exynos_ion_register_heaps(void)
{
	unsigned int i;
	bool secure = false;
	unsigned int prot_id_map = 0;

	for (i = 0; i < reserved_mem_count; i++) {
		struct ion_platform_heap pheap;
		struct ion_heap *heap;

		pheap.name	  = ion_reserved_mem[i].heapname;
		pheap.id	  = ion_reserved_mem[i].protection_id;
		pheap.base	  = ion_reserved_mem[i].base;
		pheap.size	  = ion_reserved_mem[i].size;
		pheap.align	  = ion_reserved_mem[i].alloc_align;
		pheap.secure	  = ion_reserved_mem[i].secure;
		pheap.untouchable = ion_reserved_mem[i].untouchable;

		if (pheap.id > 32) {
			perrfn("too large protection id %d of '%s'",
			       pheap.id, pheap.name);
			continue;
		}

		if (pheap.secure && ((1 << pheap.id) & prot_id_map)) {
			perrfn("protection id %d of '%s' already exists",
			       pheap.id, pheap.name);
			continue;
		}

		if (ion_reserved_mem[i].cma) {
			pheap.type = ION_HEAP_TYPE_DMA;
			heap = ion_cma_heap_create(ion_reserved_mem[i].cma,
						   &pheap);
#ifdef CONFIG_ION_RBIN_HEAP
		} else if (ion_reserved_mem[i].recyclable) {
			pheap.type = ION_HEAP_TYPE_RBIN;
			heap = ion_rbin_heap_create(&pheap);
#endif
		} else {
			pheap.type = ION_HEAP_TYPE_CARVEOUT;
			heap = ion_carveout_heap_create(&pheap);
		}

		if (IS_ERR(heap)) {
			perrfn("failed to register '%s' heap", pheap.name);
			continue;
		}

		if (pheap.secure)
			prot_id_map |= 1 << pheap.id;

		ion_device_add_heap(heap);
		pr_info("ION: registered '%s' heap\n", pheap.name);

		secure |= pheap.secure;
	}

	secure |= exynos_ion_register_hpa_heaps(prot_id_map);

	/*
	 * ion_secure_iova_pool_create() should success. If it fails, it is
	 * because of design flaw or out of memory. Nothing to do with the
	 * failure. Just debug. ion_secure_iova_pool_create() disables
	 * protection if it fails.
	 */
	if (secure)
		ion_secure_iova_pool_create();

	exynos_ion_init_camera_heaps();
	return 0;
}
subsys_initcall_sync(exynos_ion_register_heaps);
