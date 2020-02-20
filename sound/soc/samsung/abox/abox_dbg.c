/* sound/soc/samsung/abox/abox_dbg.c
 *
 * ALSA SoC Audio Layer - Samsung Abox Debug driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/* #define DEBUG */
#include <linux/io.h>
#include <linux/device.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/iommu.h>
#include <linux/of_reserved_mem.h>
#include <linux/pm_runtime.h>
#include <linux/sched/clock.h>
#include <linux/mm_types.h>
#include <asm/cacheflush.h>
#include "abox_dbg.h"
#include "abox_gic.h"
#include "abox_core.h"

#define ABOX_DBG_DUMP_MAGIC_SRAM	0x3935303030504D44ull /* DMP00059 */
#define ABOX_DBG_DUMP_MAGIC_DRAM	0x3231303038504D44ull /* DMP80012 */
#define ABOX_DBG_DUMP_MAGIC_SFR		0x5246533030504D44ull /* DMP00SFR */
#define ABOX_DBG_DUMP_LIMIT_NS		(5 * NSEC_PER_SEC)

static struct dentry *abox_dbg_root_dir __read_mostly;

struct dentry *abox_dbg_get_root_dir(void)
{
	pr_debug("%s\n", __func__);

	if (abox_dbg_root_dir == NULL)
		abox_dbg_root_dir = debugfs_create_dir("abox", NULL);

	return abox_dbg_root_dir;
}

void abox_dbg_print_gpr_from_addr(struct device *dev, struct abox_data *data,
		unsigned int *addr)
{
	abox_core_print_gpr_dump(addr);
}

void abox_dbg_print_gpr(struct device *dev, struct abox_data *data)
{
	abox_core_print_gpr();
}

struct abox_dbg_dump_sram {
	unsigned long long magic;
	char dump[SZ_512K];
} __packed;

struct abox_dbg_dump_dram {
	unsigned long long magic;
	char dump[DRAM_FIRMWARE_SIZE];
} __packed;

struct abox_dbg_dump_sfr {
	unsigned long long magic;
	u32 dump[SZ_64K / sizeof(u32)];
} __packed;

struct abox_dbg_dump {
	struct abox_dbg_dump_sram sram;
	struct abox_dbg_dump_dram dram;
	struct abox_dbg_dump_sfr sfr;
	u32 sfr_gic_gicd[SZ_4K / sizeof(u32)];
	unsigned int gpr[SZ_128];
	long long time;
	char reason[SZ_32];
} __packed;

struct abox_dbg_dump_min {
	struct abox_dbg_dump_sram sram;
	struct abox_dbg_dump_dram *dram;
	struct abox_dbg_dump_sfr sfr;
	u32 sfr_gic_gicd[SZ_4K / sizeof(u32)];
	unsigned int gpr[SZ_128];
	long long time;
	char reason[SZ_32];
	struct page **pages;
	struct vm_struct *abox_dram_vmarea;
} __packed;

static struct abox_dbg_dump (*p_abox_dbg_dump)[ABOX_DBG_DUMP_COUNT];
static struct abox_dbg_dump_min (*p_abox_dbg_dump_min)[ABOX_DBG_DUMP_COUNT];
static struct reserved_mem *abox_rmem;

static void *abox_rmem_vmap(struct reserved_mem *rmem)
{
	phys_addr_t phys = rmem->base;
	size_t size = rmem->size;
	unsigned int num_pages = (unsigned int)DIV_ROUND_UP(size, PAGE_SIZE);
	pgprot_t prot = pgprot_writecombine(PAGE_KERNEL);
	struct page **pages, **page;
	void *vaddr = NULL;

	pages = kcalloc(num_pages, sizeof(pages[0]), GFP_KERNEL);
	if (!pages) {
		pr_err("%s: malloc failed\n", __func__);
		goto out;
	}

	for (page = pages; (page - pages < num_pages); page++) {
		*page = phys_to_page(phys);
		phys += PAGE_SIZE;
	}

	vaddr = vmap(pages, num_pages, VM_MAP, prot);
	kfree(pages);
out:
	return vaddr;
}

static int __init abox_rmem_setup(struct reserved_mem *rmem)
{
	pr_info("%s: base=%pa, size=%pa\n", __func__, &rmem->base, &rmem->size);
	abox_rmem = rmem;
	return 0;
}

RESERVEDMEM_OF_DECLARE(abox_rmem, "exynos,abox_rmem", abox_rmem_setup);

static void *abox_dbg_alloc_mem_atomic(struct device *dev,
		struct abox_dbg_dump_min *p_dump)
{
	int i, j;
	int npages = DIV_ROUND_UP(sizeof(*p_dump->dram), PAGE_SIZE);
	pgprot_t prot = pgprot_writecombine(PAGE_KERNEL);
	struct page **tmp;
	gfp_t alloc_gfp_flag = GFP_ATOMIC;

	p_dump->pages = kzalloc(sizeof(struct page *) * npages, alloc_gfp_flag);
	if (!p_dump->pages) {
		dev_info(dev, "Failed to allocate array of struct pages\n");
		return NULL;
	}

	tmp = p_dump->pages;
	for (i = 0; i < npages; i++, tmp++) {
		*tmp = alloc_page(alloc_gfp_flag);
		if (*tmp == NULL) {
			pr_err("Failed to allocate pages for abox debug\n");
			goto free_pg;
		}
	}

	if (map_vm_area(p_dump->abox_dram_vmarea, prot, p_dump->pages)) {
		dev_err(dev, "Failed to map ABOX DRAM into kernel vaddr\n");
			goto free_pg;
	}

	memset(p_dump->abox_dram_vmarea->addr, 0x0, npages * PAGE_SIZE);

	return p_dump->abox_dram_vmarea->addr;

free_pg:
	tmp = p_dump->pages;
	for (j = 0; j < i; j++, tmp++)
		__free_pages(*tmp, 0);
	kfree(p_dump->pages);
	p_dump->pages = NULL;
	return NULL;
}

void abox_dbg_dump_gpr_from_addr(struct device *dev, unsigned int *addr,
		enum abox_dbg_dump_src src, const char *reason)
{
	static unsigned long long called[ABOX_DBG_DUMP_COUNT];
	unsigned long long time = sched_clock();

	dev_dbg(dev, "%s\n", __func__);

	if (!abox_is_on()) {
		dev_info(dev, "%s is skipped due to no power\n", __func__);
		return;
	}

	if (called[src] && time - called[src] < ABOX_DBG_DUMP_LIMIT_NS) {
		dev_dbg_ratelimited(dev, "%s(%d): skipped\n", __func__, src);
		called[src] = time;
		return;
	}
	called[src] = time;

	if (p_abox_dbg_dump) {
		struct abox_dbg_dump *p_dump = &(*p_abox_dbg_dump)[src];

		p_dump->time = time;
		strncpy(p_dump->reason, reason, sizeof(p_dump->reason) - 1);
		abox_core_dump_gpr_dump(p_dump->gpr, addr);
	} else if (p_abox_dbg_dump_min) {
		struct abox_dbg_dump_min *p_dump = &(*p_abox_dbg_dump_min)[src];

		p_dump->time = time;
		strncpy(p_dump->reason, reason, sizeof(p_dump->reason) - 1);
		abox_core_dump_gpr_dump(p_dump->gpr, addr);
	}
}

void abox_dbg_dump_gpr(struct device *dev, struct abox_data *data,
		enum abox_dbg_dump_src src, const char *reason)
{
	static unsigned long long called[ABOX_DBG_DUMP_COUNT];
	unsigned long long time = sched_clock();

	dev_dbg(dev, "%s\n", __func__);

	if (!abox_is_on()) {
		dev_info(dev, "%s is skipped due to no power\n", __func__);
		return;
	}

	if (called[src] && time - called[src] < ABOX_DBG_DUMP_LIMIT_NS) {
		dev_dbg_ratelimited(dev, "%s(%d): skipped\n", __func__, src);
		called[src] = time;
		return;
	}
	called[src] = time;

	if (p_abox_dbg_dump) {
		struct abox_dbg_dump *p_dump = &(*p_abox_dbg_dump)[src];

		p_dump->time = time;
		strncpy(p_dump->reason, reason, sizeof(p_dump->reason) - 1);
		abox_core_dump_gpr(p_dump->gpr);
	} else if (p_abox_dbg_dump_min) {
		struct abox_dbg_dump_min *p_dump = &(*p_abox_dbg_dump_min)[src];

		p_dump->time = time;
		strncpy(p_dump->reason, reason, sizeof(p_dump->reason) - 1);
		abox_core_dump_gpr(p_dump->gpr);
	}
}

void abox_dbg_dump_mem(struct device *dev, struct abox_data *data,
		enum abox_dbg_dump_src src, const char *reason)
{
	static unsigned long long called[ABOX_DBG_DUMP_COUNT];
	unsigned long long time = sched_clock();
	struct abox_gic_data *gic_data = dev_get_drvdata(data->dev_gic);

	dev_dbg(dev, "%s\n", __func__);

	if (!abox_is_on()) {
		dev_info(dev, "%s is skipped due to no power\n", __func__);
		return;
	}

	if (called[src] && time - called[src] < ABOX_DBG_DUMP_LIMIT_NS) {
		dev_dbg_ratelimited(dev, "%s(%d): skipped\n", __func__, src);
		called[src] = time;
		return;
	}
	called[src] = time;

	if (p_abox_dbg_dump) {
		struct abox_dbg_dump *p_dump = &(*p_abox_dbg_dump)[src];

		p_dump->time = time;
		strncpy(p_dump->reason, reason, sizeof(p_dump->reason) - 1);
		memcpy_fromio(p_dump->sram.dump, data->sram_base,
				data->sram_size);
		p_dump->sram.magic = ABOX_DBG_DUMP_MAGIC_SRAM;
		memcpy(p_dump->dram.dump, data->dram_base, DRAM_FIRMWARE_SIZE);
		p_dump->dram.magic = ABOX_DBG_DUMP_MAGIC_DRAM;
		memcpy_fromio(p_dump->sfr.dump, data->sfr_base,
				sizeof(p_dump->sfr.dump));
		p_dump->sfr.magic = ABOX_DBG_DUMP_MAGIC_SFR;
		memcpy_fromio(p_dump->sfr_gic_gicd, gic_data->gicd_base,
				sizeof(p_dump->sfr_gic_gicd));
	} else if (p_abox_dbg_dump_min) {
		struct abox_dbg_dump_min *p_dump = &(*p_abox_dbg_dump_min)[src];

		p_dump->time = time;
		strncpy(p_dump->reason, reason, sizeof(p_dump->reason) - 1);
		memcpy_fromio(p_dump->sram.dump, data->sram_base,
				data->sram_size);
		p_dump->sram.magic = ABOX_DBG_DUMP_MAGIC_SRAM;
		memcpy_fromio(p_dump->sfr.dump, data->sfr_base,
				sizeof(p_dump->sfr.dump));
		p_dump->sfr.magic = ABOX_DBG_DUMP_MAGIC_SFR;
		memcpy_fromio(p_dump->sfr_gic_gicd, gic_data->gicd_base,
				sizeof(p_dump->sfr_gic_gicd));
		if (!p_dump->dram)
			p_dump->dram = abox_dbg_alloc_mem_atomic(dev, p_dump);

		if (!IS_ERR_OR_NULL(p_dump->dram)) {

			memcpy(p_dump->dram->dump, data->dram_base,
					DRAM_FIRMWARE_SIZE);
			p_dump->dram->magic = ABOX_DBG_DUMP_MAGIC_DRAM;
			flush_cache_all();
		} else {
			dev_info(dev, "Failed to save ABOX dram\n");
		}
	}
}

void abox_dbg_dump_gpr_mem(struct device *dev, struct abox_data *data,
		enum abox_dbg_dump_src src, const char *reason)
{
	abox_dbg_dump_gpr(dev, data, src, reason);
	abox_dbg_dump_mem(dev, data, src, reason);
}

struct abox_dbg_dump_simple {
	struct abox_dbg_dump_sram sram;
	struct abox_dbg_dump_sfr sfr;
	u32 sfr_gic_gicd[SZ_4K / sizeof(u32)];
	unsigned int gpr[SZ_128];
	long long time;
	char reason[SZ_32];
};

static struct abox_dbg_dump_simple abox_dump_simple;

void abox_dbg_dump_simple(struct device *dev, struct abox_data *data,
		const char *reason)
{
	static unsigned long long called;
	unsigned long long time = sched_clock();
	struct abox_gic_data *gic_data = dev_get_drvdata(data->dev_gic);

	dev_info(dev, "%s\n", __func__);

	if (!abox_is_on()) {
		dev_info(dev, "%s is skipped due to no power\n", __func__);
		return;
	}

	if (called && time - called < ABOX_DBG_DUMP_LIMIT_NS) {
		dev_dbg_ratelimited(dev, "%s: skipped\n", __func__);
		called = time;
		return;
	}
	called = time;

	abox_dump_simple.time = time;
	strncpy(abox_dump_simple.reason, reason,
			sizeof(abox_dump_simple.reason) - 1);
	abox_core_dump_gpr(abox_dump_simple.gpr);
	memcpy_fromio(abox_dump_simple.sram.dump, data->sram_base,
			data->sram_size);
	abox_dump_simple.sram.magic = ABOX_DBG_DUMP_MAGIC_SRAM;
	memcpy_fromio(abox_dump_simple.sfr.dump, data->sfr_base,
			sizeof(abox_dump_simple.sfr.dump));
	abox_dump_simple.sfr.magic = ABOX_DBG_DUMP_MAGIC_SFR;
	memcpy_fromio(abox_dump_simple.sfr_gic_gicd, gic_data->gicd_base,
			sizeof(abox_dump_simple.sfr_gic_gicd));
}

static struct abox_dbg_dump_simple abox_dump_suspend;

void abox_dbg_dump_suspend(struct device *dev, struct abox_data *data)
{
	struct abox_gic_data *gic_data = dev_get_drvdata(data->dev_gic);

	dev_dbg(dev, "%s\n", __func__);

	abox_dump_suspend.time = sched_clock();
	strncpy(abox_dump_suspend.reason, "suspend",
			sizeof(abox_dump_suspend.reason) - 1);
	abox_core_dump_gpr(abox_dump_suspend.gpr);
	memcpy_fromio(abox_dump_suspend.sram.dump, data->sram_base,
			data->sram_size);
	abox_dump_suspend.sram.magic = ABOX_DBG_DUMP_MAGIC_SRAM;
	memcpy_fromio(abox_dump_suspend.sfr.dump, data->sfr_base,
			sizeof(abox_dump_suspend.sfr.dump));
	abox_dump_suspend.sfr.magic = ABOX_DBG_DUMP_MAGIC_SFR;
	memcpy_fromio(abox_dump_suspend.sfr_gic_gicd, gic_data->gicd_base,
			sizeof(abox_dump_suspend.sfr_gic_gicd));
}

static atomic_t abox_error_count = ATOMIC_INIT(0);

void abox_dbg_report_status(struct device *dev, bool ok)
{
	char env[32] = {0,};
	char *envp[2] = {env, NULL};

	dev_info(dev, "%s\n", __func__);

	if (ok)
		atomic_set(&abox_error_count, 0);
	else
		atomic_inc(&abox_error_count);

	snprintf(env, sizeof(env), "ERR_CNT=%d",
			atomic_read(&abox_error_count));
	kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, envp);
}

int abox_dbg_get_error_count(struct device *dev)
{
	int count = atomic_read(&abox_error_count);

	dev_dbg(dev, "%s: %d\n", __func__, count);

	return count;
}

static ssize_t calliope_sram_read(struct file *file, struct kobject *kobj,
		struct bin_attribute *battr, char *buf,
		loff_t off, size_t size)
{
	struct device *dev = kobj_to_dev(kobj);
	struct device *dev_abox = dev->parent;

	dev_dbg(dev, "%s(%lld, %zu)\n", __func__, off, size);

	if (pm_runtime_get_if_in_use(dev_abox) > 0) {
		if (off == 0)
			abox_core_flush();
		memcpy_fromio(buf, battr->private + off, size);
		pm_runtime_put(dev_abox);
	} else {
		memset(buf, 0x0, size);
	}

	return size;
}

static ssize_t calliope_dram_read(struct file *file, struct kobject *kobj,
		struct bin_attribute *battr, char *buf,
		loff_t off, size_t size)
{
	struct device *dev = kobj_to_dev(kobj);
	struct device *dev_abox = dev->parent;

	dev_dbg(dev, "%s(%lld, %zu)\n", __func__, off, size);

	if (pm_runtime_get_if_in_use(dev_abox) > 0) {
		if (off == 0)
			abox_core_flush();
		pm_runtime_put(dev_abox);
	}
	memcpy(buf, battr->private + off, size);
	return size;
}

static ssize_t calliope_priv_read(struct file *file, struct kobject *kobj,
		struct bin_attribute *battr, char *buf,
		loff_t off, size_t size)
{
	return calliope_dram_read(file, kobj, battr, buf, off, size);
}

/* size will be updated later */
static BIN_ATTR_RO(calliope_sram, 0);
static BIN_ATTR_RO(calliope_dram, DRAM_FIRMWARE_SIZE);
static BIN_ATTR_RO(calliope_priv, PRIVATE_SIZE);
static struct bin_attribute *calliope_bin_attrs[] = {
	&bin_attr_calliope_sram,
	&bin_attr_calliope_dram,
	&bin_attr_calliope_priv,
};

static ssize_t gpr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (!abox_is_on()) {
		dev_info(dev, "%s is skipped due to no power\n", __func__);
		return -EFAULT;
	}

	return abox_core_show_gpr(buf);
}

static DEVICE_ATTR_RO(gpr);

static int samsung_abox_debug_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device *abox_dev = dev->parent;
	struct abox_data *data = dev_get_drvdata(abox_dev);
	int i, ret;

	dev_dbg(dev, "%s\n", __func__);

	if (abox_rmem) {
		if (sizeof(*p_abox_dbg_dump) <= abox_rmem->size) {
			p_abox_dbg_dump = abox_rmem_vmap(abox_rmem);
			data->dump_base = p_abox_dbg_dump;
			memset(data->dump_base, 0x0, abox_rmem->size);
		} else if (sizeof(*p_abox_dbg_dump_min) <= abox_rmem->size) {
			p_abox_dbg_dump_min = abox_rmem_vmap(abox_rmem);
			data->dump_base = p_abox_dbg_dump_min;
			memset(data->dump_base, 0x0, abox_rmem->size);
			for (i = 0; i < ABOX_DBG_DUMP_COUNT; i++) {
				struct abox_dbg_dump_min *p_dump = &(*p_abox_dbg_dump_min)[i];

				p_dump->abox_dram_vmarea =
					get_vm_area(sizeof(*p_dump->dram), VM_ALLOC);
			}
		}

		data->dump_base_phys = abox_rmem->base;
		abox_iommu_map(abox_dev, IOVA_DUMP_BUFFER, abox_rmem->base,
				abox_rmem->size, data->dump_base);
	}

	ret = device_create_file(dev, &dev_attr_gpr);
	bin_attr_calliope_sram.size = data->sram_size;
	bin_attr_calliope_sram.private = data->sram_base;
	bin_attr_calliope_dram.private = data->dram_base;
	bin_attr_calliope_priv.private = data->priv_base;
	for (i = 0; i < ARRAY_SIZE(calliope_bin_attrs); i++) {
		struct bin_attribute *battr = calliope_bin_attrs[i];

		ret = device_create_bin_file(dev, battr);
		if (ret < 0)
			dev_warn(dev, "Failed to create file: %s\n",
					battr->attr.name);
	}

	return ret;
}

static int samsung_abox_debug_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int i;

	dev_dbg(dev, "%s\n", __func__);
	for (i = 0; i < ABOX_DBG_DUMP_COUNT; i++) {
		struct page **tmp = p_abox_dbg_dump_min[i]->pages;

		if (p_abox_dbg_dump_min[i]->dram)
			vm_unmap_ram(p_abox_dbg_dump_min[i]->dram,
			    DRAM_FIRMWARE_SIZE);
		if (tmp) {
			int j;

			for (j = 0; j < DRAM_FIRMWARE_SIZE / PAGE_SIZE; j++, tmp++)
				__free_pages(*tmp, 0);
			kfree(p_abox_dbg_dump_min[i]->pages);
			p_abox_dbg_dump_min[i]->pages = NULL;
		}
	}

	return 0;
}

static const struct of_device_id samsung_abox_debug_match[] = {
	{
		.compatible = "samsung,abox-debug",
	},
	{},
};
MODULE_DEVICE_TABLE(of, samsung_abox_debug_match);

static struct platform_driver samsung_abox_debug_driver = {
	.probe  = samsung_abox_debug_probe,
	.remove = samsung_abox_debug_remove,
	.driver = {
		.name = "samsung-abox-debug",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(samsung_abox_debug_match),
	},
};

module_platform_driver(samsung_abox_debug_driver);

MODULE_AUTHOR("Gyeongtaek Lee, <gt82.lee@samsung.com>");
MODULE_DESCRIPTION("Samsung ASoC A-Box Debug Driver");
MODULE_ALIAS("platform:samsung-abox-debug");
MODULE_LICENSE("GPL");
