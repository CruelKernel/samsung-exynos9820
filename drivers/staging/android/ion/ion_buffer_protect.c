/*
 * drivers/staging/android/ion/ion_buffer_protect.c
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

#include <linux/slab.h>
#include <linux/genalloc.h>
#include <linux/smc.h>

#include <asm/cacheflush.h>

#include "ion.h"
#include "ion_exynos.h"

#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION

#define ION_SECURE_DMA_BASE	0x80000000
#define ION_SECURE_DMA_END	0xE0000000

static struct gen_pool *secure_iova_pool;
static DEFINE_SPINLOCK(siova_pool_lock);

#define MAX_IOVA_ALIGNMENT      12

static unsigned long find_first_fit_with_align(unsigned long *map,
					       unsigned long size,
					       unsigned long start,
					       unsigned int nr, void *data,
					       struct gen_pool *pool)
{
	unsigned long align = ((*(unsigned int *)data) >> PAGE_SHIFT);

	if (align > (1 << MAX_IOVA_ALIGNMENT))
		align = (1 << MAX_IOVA_ALIGNMENT);

	return bitmap_find_next_zero_area(map, size, start, nr, (align - 1));
}

static int ion_secure_iova_alloc(unsigned long *addr, unsigned long size,
				 unsigned int align)
{
	unsigned long out_addr;

	if (!secure_iova_pool) {
		perrfn("Secure IOVA pool is not created");
		return -ENODEV;
	}

	spin_lock(&siova_pool_lock);
	if (align > PAGE_SIZE) {
		gen_pool_set_algo(secure_iova_pool,
				  find_first_fit_with_align, &align);
		out_addr = gen_pool_alloc(secure_iova_pool, size);
		gen_pool_set_algo(secure_iova_pool, NULL, NULL);
	} else {
		out_addr = gen_pool_alloc(secure_iova_pool, size);
	}
	spin_unlock(&siova_pool_lock);

	if (out_addr == 0) {
		perrfn("failed alloc secure iova. %zu/%zu bytes used",
		       gen_pool_avail(secure_iova_pool),
		       gen_pool_size(secure_iova_pool));
		return -ENOMEM;
	}

	*addr = out_addr;

	return 0;
}

static void ion_secure_iova_free(unsigned long addr, unsigned long size)
{
	if (!secure_iova_pool) {
		perrfn("Secure IOVA pool is not created");
		return;
	}

	spin_lock(&siova_pool_lock);
	gen_pool_free(secure_iova_pool, addr, size);
	spin_unlock(&siova_pool_lock);
}

int __init ion_secure_iova_pool_create(void)
{
	secure_iova_pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!secure_iova_pool) {
		perrfn("failed to create Secure IOVA pool");
		return -ENOMEM;
	}

	if (gen_pool_add(secure_iova_pool, ION_SECURE_DMA_BASE,
			 ION_SECURE_DMA_END - ION_SECURE_DMA_BASE, -1)) {
		perrfn("failed to set address range of Secure IOVA pool");
		return -ENOMEM;
	}

	return 0;
}

static int ion_secure_protect(struct ion_buffer_prot_info *protdesc,
			      unsigned int protalign)
{
	unsigned long size = protdesc->chunk_count * protdesc->chunk_size;
	unsigned long dma_addr = 0;
	enum drmdrv_result_t drmret = DRMDRV_OK;
	int ret;

	ret = ion_secure_iova_alloc(&dma_addr, size,
				    max_t(u32, protalign, PAGE_SIZE));
	if (ret)
		goto err_iova;

	protdesc->dma_addr = (unsigned int)dma_addr;

	__flush_dcache_area(protdesc, sizeof(*protdesc));
	if (protdesc->chunk_count > 1)
		__flush_dcache_area(phys_to_virt(protdesc->bus_address),
				sizeof(unsigned long) * protdesc->chunk_count);

	drmret = exynos_smc(SMC_DRM_PPMP_PROT, virt_to_phys(protdesc), 0, 0);
	if (drmret != DRMDRV_OK) {
		ret = -EACCES;
		goto err_smc;
	}

	return 0;
err_smc:
	ion_secure_iova_free(dma_addr, size);
err_iova:
	perrfn("PROT:%#x (err=%d,va=%#lx,len=%#lx,cnt=%u,flg=%u)",
	       SMC_DRM_PPMP_PROT, drmret, dma_addr, size,
	       protdesc->chunk_count, protdesc->flags);

	return ret;
}

static int ion_secure_unprotect(struct ion_buffer_prot_info *protdesc)
{
	unsigned long size = protdesc->chunk_count * protdesc->chunk_size;
	int ret;
	/*
	 * No need to flush protdesc for unprotection because it is never
	 * modified since the buffer is protected.
	 */
	ret = exynos_smc(SMC_DRM_PPMP_UNPROT, virt_to_phys(protdesc), 0, 0);

	if (ret != DRMDRV_OK) {
		perrfn("UNPROT:%d(err=%d,va=%#x,len=%#lx,cnt=%u,flg=%u)",
		       SMC_DRM_PPMP_UNPROT, ret, protdesc->dma_addr,
		       size, protdesc->chunk_count, protdesc->flags);
		return -EACCES;
	}
	/*
	 * retain the secure device address if unprotection to its area fails.
	 * It might be unusable forever since we do not know the state o ft he
	 * secure world before returning error from exynos_smc() above.
	 */
	ion_secure_iova_free(protdesc->dma_addr, size);

	return 0;
}

#else /* !CONFIG_EXYNOS_CONTENT_PATH_PROTECTION */

static int ion_secure_protect(struct ion_buffer_prot_info *prot,
			      unsigned int protalign)
{
	return -ENODEV;
}

static int ion_secure_unprotect(struct ion_buffer_prot_info *prot)
{
	return -ENODEV;
}

#endif /* CONFIG_EXYNOS_CONTENT_PATH_PROTECTION */

void *ion_buffer_protect_single(unsigned int protection_id, unsigned int size,
				unsigned long phys, unsigned int protalign)
{
	struct ion_buffer_prot_info *protdesc;
	int ret;

	if (!IS_ENABLED(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION))
		return NULL;

	protdesc = kmalloc(sizeof(*protdesc), GFP_KERNEL);
	if (!protdesc)
		return ERR_PTR(-ENOMEM);

	protdesc->chunk_count = 1,
	protdesc->flags = protection_id;
	protdesc->chunk_size = ALIGN(size, protalign);
	protdesc->bus_address = phys;

	ret = ion_secure_protect(protdesc, protalign);
	if (ret) {
		perrfn("protection failure (id%u,len%u,base%#lx,align%#x",
		       protection_id, size, phys, protalign);
		kfree(protdesc);
		return ERR_PTR(ret);
	}

	return protdesc;
}

void *ion_buffer_protect_multi(unsigned int protection_id, unsigned int count,
			       unsigned int chunk_size, unsigned long *phys_arr,
			       unsigned int protalign)
{
	struct ion_buffer_prot_info *protdesc;
	int ret;

	if (!IS_ENABLED(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION))
		return NULL;

	if (count == 1)
		return ion_buffer_protect_single(protection_id, chunk_size,
						 *phys_arr, protalign);

	protdesc = kmalloc(sizeof(*protdesc), GFP_KERNEL);
	if (!protdesc)
		return ERR_PTR(-ENOMEM);

	/*
	 * The address pointed by phys_arr is stored to the protection metadata
	 * after conversion to its physical address.
	 */
	kmemleak_ignore(phys_arr);

	protdesc->chunk_count = count,
	protdesc->flags = protection_id;
	protdesc->chunk_size = chunk_size;
	protdesc->bus_address = virt_to_phys(phys_arr);

	ret = ion_secure_protect(protdesc, protalign);
	if (ret) {
		perrfn("protection failure (id%u,chk%u,count%u,align%#x",
		       protection_id, chunk_size, count, protalign);
		kfree(protdesc);
		return ERR_PTR(ret);
	}

	return protdesc;
}

int ion_buffer_unprotect(void *priv)
{
	struct ion_buffer_prot_info *protdesc = priv;
	int ret = 0;

	if (priv) {
		ret = ion_secure_unprotect(protdesc);
		if (protdesc->chunk_count > 1)
			kfree(phys_to_virt(protdesc->bus_address));
		kfree(protdesc);
	}

	return ret;
}
