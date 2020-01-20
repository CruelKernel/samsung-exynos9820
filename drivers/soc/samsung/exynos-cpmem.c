/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/vmalloc.h>
#include <linux/of_reserved_mem.h>

static unsigned long rmem_addr;
static unsigned rmem_size;

static void __iomem *request_rmem(unsigned long addr, unsigned size)
{
	int i;
	unsigned int num_pages = (size >> PAGE_SHIFT);
	pgprot_t prot = pgprot_writecombine(PAGE_KERNEL);
	struct page **pages;
	void *v_addr;

	if (!addr)
		return NULL;

	if (size > (num_pages << PAGE_SHIFT))
		num_pages++;

	pages = kmalloc(sizeof(struct page*) * num_pages, GFP_ATOMIC);
	if (!pages) {
		pr_err("%s: pages allocation fail!\n", __func__);
			return NULL;
		}
	for (i = 0; i < (num_pages); i++) {
		pages[i] = phys_to_page(addr);
		addr += PAGE_SIZE;
	}

	v_addr = vmap(pages, num_pages, VM_MAP,	prot);
	if (v_addr == NULL)
		pr_err("%s: Failed to vmap pages\n", __func__);

	kfree(pages);

	return (void __iomem *)v_addr;
}

static int __init cp_linear_rmem_setup(struct reserved_mem *rmem)
{
	rmem_addr = rmem->base;
	rmem_size = rmem->size;

	pr_info("%s: memory reserved: addr=0x%08x, size=0x%08x\n",
			__func__, (u32)rmem_addr, (u32)rmem_size);

	return 0;
}
RESERVEDMEM_OF_DECLARE(cp_linear, "exynos,cp_linear", cp_linear_rmem_setup);


static int __init exynos_cpmem_probe(struct platform_device *pdev)
{
	void __iomem *rmem_base;

	if (!rmem_addr || !rmem_size)
		return 0;

	rmem_base = request_rmem(rmem_addr, rmem_size);
	if (rmem_base) {
		vunmap(rmem_base);
		rmem_base = NULL;
		pr_err("%s: rmem allocation fail\n", __func__);
	}

	return 0;
}

static int exynos_cpmem_remove(struct platform_device *pdev)
{
	platform_set_drvdata(pdev, NULL);

	pr_info("%s: exynos-cpmem is removed", __func__);

	return 0;
}

static struct platform_device_id exynos_cpmem_driver_ids[] = {
	{ .name = "exynos-cpmem", },
	{},
};
MODULE_DEVICE_TABLE(platform, exynos_cpmem_driver_ids);

static const struct of_device_id exynos_cpmem_match[] = {
	{ .compatible = "samsung,exynos-cpmem", },
	{},
};

static struct platform_driver exynos_cpmem_driver = {
	.remove = exynos_cpmem_remove,
	.id_table = exynos_cpmem_driver_ids,
	.driver = {
		.name = "exynos-cpmem",
		.owner = THIS_MODULE,
		.of_match_table = exynos_cpmem_match,
	},
};

module_platform_driver_probe(exynos_cpmem_driver, exynos_cpmem_probe);

MODULE_AUTHOR("");
MODULE_DESCRIPTION("Samsung CP memory reserve");
MODULE_LICENSE("GPL");
