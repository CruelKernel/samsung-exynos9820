/* linux/drivers/iommu/exynos_pcie_iommu.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/iommu.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/of.h>
#include <linux/of_iommu.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/smc.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/genalloc.h>
#include <linux/memblock.h>

#include <asm/cacheflush.h>
#include <asm/pgtable.h>

#include <dt-bindings/sysmmu/sysmmu.h>

#include "exynos-pcie-iommu.h"

#ifdef CONFIG_SEC_DEBUG_EXTRA_INFO
#include <linux/sec_debug.h>
#endif

#if defined(CONFIG_SOC_EXYNOS9820)
#define MAX_RC_NUM	2
#endif

static u8 *gen_buff;
static struct kmem_cache *lv2table_kmem_cache;
#ifndef USE_DYNAMIC_MEM_ALLOC
static struct gen_pool *lv2table_pool;
#endif

static struct sysmmu_drvdata g_sysmmu_drvdata[MAX_RC_NUM];
static u32 alloc_counter;
static u32 lv2table_counter;
static u32 max_req_cnt;
static struct dram_region ch_dram_region[NUM_DRAM_REGION];
static u32 max_dram_region;
static u32 wrong_pf_cnt;
#ifdef CONFIG_PCIE_IOMMU_HISTORY_LOG
struct history_buff pcie_map_history, pcie_unmap_history;
#endif

int pcie_iommu_map(int ch_num, unsigned long iova, phys_addr_t paddr,
			size_t size, int prot);
size_t pcie_iommu_unmap(int ch_num, unsigned long iova, size_t size);

/* For ARM64 only */
static inline void pgtable_flush(void *vastart, void *vaend)
{
	__dma_flush_area(vastart, vaend - vastart);
}

static void __sysmmu_tlb_invalidate_all(void __iomem *sfrbase)
{
	writel(0x1, sfrbase + REG_MMU_FLUSH);
}

static void __sysmmu_set_ptbase(void __iomem *sfrbase, phys_addr_t pfn_pgtable)
{
	writel_relaxed(pfn_pgtable, sfrbase + REG_PT_BASE_PPN);

	__sysmmu_tlb_invalidate_all(sfrbase);
}

#if defined(CONFIG_SOC_EXYNOS9810)
static void __sysmmu_tlb_pinning(struct sysmmu_drvdata *drvdata)
{
	dma_addr_t mem_start_addr;
	size_t mem_size;
	u32 start_addr, end_addr;
	u32 upper_addr, reg_val;
	void __iomem *sfrbase = drvdata->sfrbase;

	/* Set TLB1 pinning address */
	get_atomic_pool_info(&mem_start_addr, &mem_size);

	upper_addr = (mem_start_addr >> 32) & 0xf;
	start_addr = mem_start_addr & 0xffffffff;
	end_addr = start_addr + mem_size;

	reg_val = readl_relaxed(sfrbase + REG_MMU_TLB_MATCH_CFG(1));
	/* Clear EVA/SVA_UPPER */
	reg_val &= ~(0xff);
	reg_val |= (upper_addr << 4) | (upper_addr << 0);
	writel_relaxed(reg_val, sfrbase + REG_MMU_TLB_MATCH_CFG(1));

	/* Set Start/End TLB MATCH */
	writel_relaxed(start_addr, sfrbase + REG_MMU_TLB_MATCH_SVA(1));
	writel_relaxed(end_addr, sfrbase + REG_MMU_TLB_MATCH_EVA(1));

	pr_info("Set TLB MATCH address for TLB1 Pinning : 0x%x_%x ~ 0x%x_%x\n",
			readl_relaxed(sfrbase + REG_MMU_TLB_MATCH_CFG(1)) & 0xf,
			readl_relaxed(sfrbase + REG_MMU_TLB_MATCH_SVA(1)),
			upper_addr,
			readl_relaxed(sfrbase + REG_MMU_TLB_MATCH_EVA(1)));
}
#endif

static void __sysmmu_init_config(struct sysmmu_drvdata *drvdata)
{
	unsigned long cfg = 0;

	writel_relaxed(CTRL_BLOCK, drvdata->sfrbase + REG_MMU_CTRL);

	/* Set TLB0 */
	writel_relaxed(0x0, drvdata->sfrbase + REG_MMU_TLB_CFG(0));
	/* Enable TLB1 to be used by both PCIe channel 0 and 1*/
#if defined(CONFIG_SOC_EXYNOS9810)
	writel_relaxed(0x0, drvdata->sfrbase + REG_MMU_TLB_CFG(1));
	writel_relaxed(TLB_USED_ALL_PCIE_PORT | TLB_USED_RW_REQ,
			drvdata->sfrbase + REG_MMU_TLB_MATCH_CFG(1));

	if (drvdata->use_tlb_pinning)
		__sysmmu_tlb_pinning(drvdata);
#endif
	if (drvdata->qos != DEFAULT_QOS_VALUE)
		cfg |= CFG_QOS_OVRRIDE | CFG_QOS(drvdata->qos);

	cfg |= __raw_readl(drvdata->sfrbase + REG_MMU_CFG) & ~CFG_MASK;
	writel_relaxed(cfg, drvdata->sfrbase + REG_MMU_CFG);
}

static void __sysmmu_enable_nocount(struct sysmmu_drvdata *drvdata)
{
	clk_enable(drvdata->clk);

	__sysmmu_init_config(drvdata);

	__sysmmu_set_ptbase(drvdata->sfrbase, drvdata->pgtable / PAGE_SIZE);

	writel(CTRL_ENABLE, drvdata->sfrbase + REG_MMU_CTRL);
}

static void __sysmmu_disable_nocount(struct sysmmu_drvdata *drvdata)
{
	writel_relaxed(0, drvdata->sfrbase + REG_MMU_CFG);
#ifdef USE_BLOCK_DISABE
	writel_relaxed(CTRL_BLOCK_DISABLE, drvdata->sfrbase + REG_MMU_CTRL);
	BUG_ON(readl_relaxed(drvdata->sfrbase + REG_MMU_CTRL)
						!= CTRL_BLOCK_DISABLE);
#else
	/* PCIe SysMMU use full disable(by pass) as default */
	writel_relaxed(CTRL_DISABLE, drvdata->sfrbase + REG_MMU_CTRL);
#endif

	clk_disable(drvdata->clk);
}

void pcie_sysmmu_enable(int ch_num)
{
	struct sysmmu_drvdata *drvdata = &g_sysmmu_drvdata[ch_num];

	__sysmmu_enable_nocount(drvdata);
	set_sysmmu_active(drvdata);
}
EXPORT_SYMBOL(pcie_sysmmu_enable);

void pcie_sysmmu_disable(int ch_num)
{
	struct sysmmu_drvdata *drvdata = &g_sysmmu_drvdata[ch_num];

	set_sysmmu_inactive(drvdata);
	__sysmmu_disable_nocount(drvdata);
	pr_info("SysMMU alloc num : %d(Max:%d), lv2_alloc : %d, fault : %d\n",
				alloc_counter, max_req_cnt,
				lv2table_counter, wrong_pf_cnt);
}
EXPORT_SYMBOL(pcie_sysmmu_disable);

void pcie_sysmmu_all_buff_free(int ch_num)
{
	struct sysmmu_drvdata *drvdata = &g_sysmmu_drvdata[ch_num];
	struct exynos_iommu_domain *domain;
	void __maybe_unused *virt_addr;
	int i;

	domain = drvdata->domain;

	for (i = 0; i < NUM_LV1ENTRIES; i++) {
		if (lv1ent_page(domain->pgtable + i)) {
#ifdef USE_DYNAMIC_MEM_ALLOC
			virt_addr = phys_to_virt(
					lv2table_base(domain->pgtable + i));
			kmem_cache_free(lv2table_kmem_cache, virt_addr);
#else
			gen_pool_free(lv2table_pool,
					lv1ent_page(domain->pgtable + i),
					LV2TABLE_REFCNT_SZ);
#endif
		}
	}

}
EXPORT_SYMBOL(pcie_sysmmu_all_buff_free);

static int get_hw_version(struct sysmmu_drvdata *drvdata, void __iomem *sfrbase)
{
	int ret;

	__sysmmu_enable_nocount(drvdata);

	ret = MMU_RAW_VER(__raw_readl(sfrbase + REG_MMU_VERSION));

	__sysmmu_disable_nocount(drvdata);

	return ret;
}

static char *sysmmu_fault_name[SYSMMU_FAULTS_NUM] = {
	"PTW ACCESS FAULT",
	"PAGE FAULT",
	"L1TLB MULTI-HIT FAULT",
	"ACCESS FAULT",
	"SECURITY FAULT",
	"UNKNOWN FAULT"
};

static unsigned int dump_tlb_entry_port_type(void __iomem *sfrbase,
						int idx_way, int idx_set)
{
	if (MMU_TLB_ENTRY_VALID(__raw_readl(sfrbase + REG_CAPA1_TLB_ATTR))) {
		pr_crit("[%02d][%02d] VPN: %#010x, PPN: %#010x, ATTR: %#010x\n",
			idx_way, idx_set,
			__raw_readl(sfrbase + REG_CAPA1_TLB_VPN),
			__raw_readl(sfrbase + REG_CAPA1_TLB_PPN),
			__raw_readl(sfrbase + REG_CAPA1_TLB_ATTR));
		return 1;
	}
	return 0;
}

#define MMU_NUM_TLB_SUBLINE		4
static void dump_sysmmu_tlb_port(struct sysmmu_drvdata *drvdata)
{
	int t, i, j, k;
	u32 capa0, capa1, info;
	unsigned int cnt;
	int num_tlb, num_port, num_sbb;
	void __iomem *sfrbase = drvdata->sfrbase;

	capa0 = __raw_readl(sfrbase + REG_MMU_CAPA0_V7);
	capa1 = __raw_readl(sfrbase + REG_MMU_CAPA1_V7);

	num_tlb = MMU_CAPA1_NUM_TLB(capa1);
	num_port = MMU_CAPA1_NUM_PORT(capa1);
	num_sbb = 1 << MMU_CAPA_NUM_SBB_ENTRY(capa0);

	pr_crit("SysMMU has %d TLBs, %d ports, %d sbb entries\n",
					num_tlb, num_port, num_sbb);

	for (t = 0; t < num_tlb; t++) {
		int num_set, num_way;

		info = __raw_readl(sfrbase + MMU_TLB_INFO(t));
		num_way = MMU_CAPA1_NUM_TLB_WAY(info);
		num_set = MMU_CAPA1_NUM_TLB_SET(info);

		pr_crit("TLB.%d has %d way, %d set.\n", t, num_way, num_set);
		pr_crit("------------- TLB[WAY][SET][ENTRY] -------------\n");
		for (i = 0, cnt = 0; i < num_way; i++) {
			for (j = 0; j < num_set; j++) {
				for (k = 0; k < MMU_NUM_TLB_SUBLINE; k++) {
					__raw_writel(MMU_CAPA1_SET_TLB_READ_ENTRY(t, j, i, k),
							sfrbase + REG_CAPA1_TLB_READ);
					cnt += dump_tlb_entry_port_type(sfrbase, i, j);
				}
			}
		}
	}
	if (!cnt)
		pr_crit(">> No Valid TLB Entries\n");

	pr_crit("--- SBB(Second-Level Page Table Base Address Buffer ---\n");
	for (i = 0, cnt = 0; i < num_sbb; i++) {
		__raw_writel(i, sfrbase + REG_CAPA1_SBB_READ);
		if (MMU_SBB_ENTRY_VALID(__raw_readl(sfrbase + REG_CAPA1_SBB_VPN))) {
			pr_crit("[%02d] VPN: %#010x, PPN: %#010x, ATTR: %#010x\n",
				i, __raw_readl(sfrbase + REG_CAPA1_SBB_VPN),
				__raw_readl(sfrbase + REG_CAPA1_SBB_LINK),
				__raw_readl(sfrbase + REG_CAPA1_SBB_ATTR));
			cnt++;
		}
	}
	if (!cnt)
		pr_crit(">> No Valid SBB Entries\n");
}

void print_pcie_sysmmu_tlb(int ch_num)
{
	struct sysmmu_drvdata *drvdata = &g_sysmmu_drvdata[ch_num];
	phys_addr_t pgtable;

	pgtable = __raw_readl(drvdata->sfrbase + REG_PT_BASE_PPN);
	pgtable <<= PAGE_SHIFT;
	pr_crit("Page Table Base Address : 0x%llx\n", pgtable);

	dump_sysmmu_tlb_port(drvdata);
}
EXPORT_SYMBOL(print_pcie_sysmmu_tlb);

static int show_fault_information(struct sysmmu_drvdata *drvdata,
				   int flags, unsigned long fault_addr)
{
	unsigned int info;
	phys_addr_t pgtable;
	int fault_id = SYSMMU_FAULT_ID(flags);
	const char *port_name = NULL;
	int ret = 0;
#ifdef CONFIG_SEC_DEBUG_EXTRA_INFO
	char temp_buf[SZ_128];
#endif

	pgtable = __raw_readl(drvdata->sfrbase + REG_PT_BASE_PPN);
	pgtable <<= PAGE_SHIFT;

	if (MMU_MAJ_VER(drvdata->version) >= 7) {
		info = __raw_readl(drvdata->sfrbase + REG_FAULT_TRANS_INFO);
	} else {
		/* TODO: Remove me later */
		info = __raw_readl(drvdata->sfrbase +
			((flags & IOMMU_FAULT_WRITE) ?
			 REG_FAULT_AW_TRANS_INFO : REG_FAULT_AR_TRANS_INFO));
	}

	of_property_read_string(drvdata->sysmmu->of_node,
					"port-name", &port_name);

	pr_crit("----------------------------------------------------------\n");
	pr_crit("From [%s], SysMMU %s %s at %#010lx (page table @ %pa)\n",
		port_name ? port_name : dev_name(drvdata->sysmmu),
		(flags & IOMMU_FAULT_WRITE) ? "WRITE" : "READ",
		sysmmu_fault_name[fault_id], fault_addr, &pgtable);

#ifdef CONFIG_SEC_DEBUG_EXTRA_INFO
		snprintf(temp_buf, SZ_128, "%s %s %s at %#010lx (%pa)",
		port_name ? port_name : dev_name(drvdata->sysmmu),
		(flags & IOMMU_FAULT_WRITE) ? "WRITE" : "READ",
		sysmmu_fault_name[fault_id], fault_addr, &pgtable);
		sec_debug_set_extra_info_sysmmu(temp_buf);
#endif

	if (fault_id == SYSMMU_FAULT_UNKNOWN) {
		pr_crit("The fault is not caused by this System MMU.\n");
		pr_crit("Please check IRQ and SFR base address.\n");
		goto finish;
	}

	pr_crit("AxID: %#x, AxLEN: %#x\n", info & 0xFFFF, (info >> 16) & 0xF);

	if (pgtable != drvdata->pgtable)
		pr_crit("Page table base of driver: %pa\n",
			&drvdata->pgtable);

	if (fault_id == SYSMMU_FAULT_PTW_ACCESS)
		pr_crit("System MMU has failed to access page table\n");

	if (!pfn_valid(pgtable >> PAGE_SHIFT)) {
		pr_crit("Page table base is not in a valid memory region\n");
	} else {
		sysmmu_pte_t *ent;

		ent = section_entry(phys_to_virt(pgtable), fault_addr);
		pr_crit("Lv1 entry: %#010x\n", *ent);

		if (lv1ent_page(ent)) {
			u64 sft_ent_addr, sft_fault_addr;

			ent = page_entry(ent, fault_addr);
			pr_crit("Lv2 entry: %#010x\n", *ent);

			sft_ent_addr = (*ent) >> 8;
			sft_fault_addr = fault_addr >> 12;

			if (sft_ent_addr == sft_fault_addr) {
				pr_crit("ent(%#llx) == faddr(%#llx)...\n",
						sft_ent_addr, sft_fault_addr);
				pr_crit("Try to IGNORE Page fault panic...\n");
				ret = SYSMMU_NO_PANIC;
				wrong_pf_cnt++;
			}
		}
	}

	dump_sysmmu_tlb_port(drvdata);
finish:
	pr_crit("----------------------------------------------------------\n");

	return ret;
}

#ifdef CONFIG_BCMDHD_PCIE
extern void dhd_smmu_fault_handler(u32 axid, unsigned long fault_addr);
#endif /* CONFIG_BCMDHD_PCIE */

static irqreturn_t exynos_sysmmu_irq(int irq, void *dev_id)
{
	struct sysmmu_drvdata *drvdata = dev_id;
	unsigned int itype;
	unsigned long addr = -1;
	int flags = 0;
	u32 info;
	u32 int_status;
	int ret;

	dev_info(drvdata->sysmmu, "%s:%d: irq(%d) happened\n",
					__func__, __LINE__, irq);

	WARN(!is_sysmmu_active(drvdata),
		"Fault occurred while System MMU %s is not enabled!\n",
		dev_name(drvdata->sysmmu));

	int_status = __raw_readl(drvdata->sfrbase + REG_INT_STATUS);
	itype =  __ffs(int_status);
	if (WARN_ON(!(itype < SYSMMU_FAULT_UNKNOWN)))
		itype = SYSMMU_FAULT_UNKNOWN;
	else
		addr = __raw_readl(drvdata->sfrbase + REG_FAULT_ADDR);

	/* It support 36bit address */
	addr |= (unsigned long)(__raw_readl(drvdata->sfrbase +
				REG_FAULT_TRANS_INFO) >> 28) << 32;

	info = __raw_readl(drvdata->sfrbase + REG_FAULT_TRANS_INFO);
	flags = MMU_IS_READ_FAULT(info) ?
		IOMMU_FAULT_READ : IOMMU_FAULT_WRITE;
	flags |= SYSMMU_FAULT_FLAG(itype);

	/* Clear interrupts */
	writel_relaxed(int_status, drvdata->sfrbase + REG_INT_CLEAR);

	ret = show_fault_information(drvdata, flags, addr);
	if (ret == SYSMMU_NO_PANIC)
		return IRQ_HANDLED;

#ifdef CONFIG_BCMDHD_PCIE
	if (drvdata->ch_num == 0) {
		/* Kernel Panic will be triggered by dump handler */
		dhd_smmu_fault_handler(info & 0xFFFF, addr);
		disable_irq_nosync(irq);
	}
#endif /* CONFIG_BCMDHD_PCIE */

	atomic_notifier_call_chain(&drvdata->fault_notifiers, addr, &flags);

#ifndef CONFIG_BCMDHD_PCIE
	panic("Unrecoverable System MMU Fault!!");
#endif /* !CONFIG_BCMDHD_PCIE */

	return IRQ_HANDLED;
}

static void __sysmmu_tlb_invalidate(struct sysmmu_drvdata *drvdata,
				dma_addr_t iova, size_t size)
{
	void * __iomem sfrbase = drvdata->sfrbase;
	u32 set_val = 0;
	u32 lower_addr = lower_32_bits(iova);
	u32 upper_addr = upper_32_bits(iova);

	set_val = lower_addr & 0xfffff000;
	set_val |= (upper_addr & 0xf) << 8;

	__raw_writel(set_val, sfrbase + REG_FLUSH_RANGE_START);

	set_val = (u32)(size - 1 + lower_addr);
	set_val &= ~(0xf << 8);
	set_val |= (upper_addr & 0xf) << 8;
	set_val &= ~(0xff);

	__raw_writel(set_val, sfrbase + REG_FLUSH_RANGE_END);
	writel(0x1, sfrbase + REG_MMU_FLUSH_RANGE);
}

static void exynos_sysmmu_tlb_invalidate(int ch_num, dma_addr_t d_start, size_t size)
{
	struct sysmmu_drvdata *drvdata = &g_sysmmu_drvdata[ch_num];
	sysmmu_iova_t start = (sysmmu_iova_t)d_start;

	spin_lock(&drvdata->lock);
	if (!is_sysmmu_active(drvdata)) {
		spin_unlock(&drvdata->lock);
		dev_dbg(drvdata->sysmmu,
			"Skip TLB invalidation %#zx@%#llx\n", size, start);
		return ;
	}

	dev_dbg(drvdata->sysmmu,
		"TLB invalidation %#zx@%#llx\n", size, start);

	__sysmmu_tlb_invalidate(drvdata, start, size);

	spin_unlock(&drvdata->lock);
}

static void clear_lv2_page_table(sysmmu_pte_t *ent, int n)
{
	if (n > 0)
		memset(ent, 0, sizeof(*ent) * n);
}

static int lv1set_section(struct exynos_iommu_domain *domain,
			  sysmmu_pte_t *sent, sysmmu_iova_t iova,
			  phys_addr_t paddr, int prot, atomic_t *pgcnt)
{
	bool shareable = !!(prot & IOMMU_CACHE);

	if (lv1ent_section(sent)) {
		WARN(1, "Trying mapping on 1MiB@%#09llx that is mapped",
			iova);
		return -EADDRINUSE;
	}

	if (lv1ent_page(sent)) {
		if (WARN_ON(atomic_read(pgcnt) != NUM_LV2ENTRIES)) {
			WARN(1, "Trying mapping on 1MiB@%#09llx that is mapped",
				iova);
			return -EADDRINUSE;
		}
		/* TODO: for v7, free lv2 page table */
	}

	*sent = mk_lv1ent_sect(paddr);
	if (shareable)
		set_lv1ent_shareable(sent);
	pgtable_flush(sent, sent + 1);

	return 0;
}

static int lv2set_page(sysmmu_pte_t *pent, phys_addr_t paddr, size_t size,
						int prot, atomic_t *pgcnt)
{
	bool shareable = !!(prot & IOMMU_CACHE);

	if (size == SPAGE_SIZE) {
		if (!lv2ent_fault(pent)) {
			sysmmu_pte_t *refcnt_buf;

			/* Duplicated IOMMU map 4KB */
			refcnt_buf = pent + NUM_LV2ENTRIES;
			*refcnt_buf = *refcnt_buf + 1;
			atomic_dec(pgcnt);
			return 0;
		}

		*pent = mk_lv2ent_spage(paddr);
		if (shareable)
			set_lv2ent_shareable(pent);
		pgtable_flush(pent, pent + 1);
		atomic_dec(pgcnt);
	} else { /* size == LPAGE_SIZE */
		int i;

		pr_err("Allocate LPAGE_SIZE!!!\n");
		for (i = 0; i < SPAGES_PER_LPAGE; i++, pent++) {
			if (WARN_ON(!lv2ent_fault(pent))) {
				clear_lv2_page_table(pent - i, i);
				return -EADDRINUSE;
			}

			*pent = mk_lv2ent_lpage(paddr);
			if (shareable)
				set_lv2ent_shareable(pent);
		}
		pgtable_flush(pent - SPAGES_PER_LPAGE, pent);
		atomic_sub(SPAGES_PER_LPAGE, pgcnt);
	}

	return 0;
}

#ifdef USE_DYNAMIC_MEM_ALLOC
static sysmmu_pte_t *alloc_extend_buff(struct exynos_iommu_domain *domain)
{
	sysmmu_pte_t *alloc_buff = NULL;
	int i;

	/* Find Empty buffer */
	for (i = 0; i < MAX_EXT_BUFF_NUM; i++) {
		if (domain->ext_buff[i].used == 0) {
			pr_err("Use extend buffer index : %d...\n", i);
			break;
		}
	}

	if (i == MAX_EXT_BUFF_NUM) {
		pr_err("WARNNING - Extend buffers are full!!!\n");
		return NULL;
	}

	domain->ext_buff[i].used = 1;
	alloc_buff = domain->ext_buff[i].buff;

	return alloc_buff;
}
#endif

static sysmmu_pte_t *alloc_lv2entry(struct exynos_iommu_domain *domain,
				sysmmu_pte_t *sent, sysmmu_iova_t iova,
				atomic_t *pgcounter, gfp_t gfpmask)
{
	sysmmu_pte_t *pent = NULL;

	if (lv1ent_section(sent)) {
		WARN(1, "Trying mapping on %#09llx mapped with 1MiB page",
						iova);
		return ERR_PTR(-EADDRINUSE);
	}

	if (lv1ent_fault(sent)) {
		lv2table_counter++;

#ifdef USE_DYNAMIC_MEM_ALLOC
		pent = kmem_cache_zalloc(lv2table_kmem_cache,
				gfpmask);
		if (!pent) {
			/* Use extended Buffer */
			pent = alloc_extend_buff(domain);

			if (!pent)
				return ERR_PTR(-ENOMEM);
		}
#else
		if (gen_pool_avail(lv2table_pool) >= LV2TABLE_REFCNT_SZ) {
			pent = phys_to_virt(gen_pool_alloc(lv2table_pool,
						LV2TABLE_REFCNT_SZ));
		} else {
			pr_err("Gen_pool is full!! Try dynamic alloc\n");
			pent = kmem_cache_zalloc(lv2table_kmem_cache, gfpmask);
			if (!pent)
				return ERR_PTR(-ENOMEM);
		}
#endif

		*sent = mk_lv1ent_page(virt_to_phys(pent));
		pgtable_flush(sent, sent + 1);
		pgtable_flush(pent, pent + NUM_LV2ENTRIES);
		atomic_set(pgcounter, NUM_LV2ENTRIES);
		kmemleak_ignore(pent);
	}

	return page_entry(sent, iova);
}

static size_t iommu_pgsize(int ch_num, unsigned long addr_merge, size_t size)
{
	struct sysmmu_drvdata *drvdata = &g_sysmmu_drvdata[ch_num];
	struct exynos_iommu_domain *domain = drvdata->domain;
	unsigned int pgsize_idx;
	size_t pgsize;

	/* Max page size that still fits into 'size' */
	pgsize_idx = __fls(size);

	/* need to consider alignment requirements ? */
	if (likely(addr_merge)) {
		/* Max page size allowed by address */
		unsigned int align_pgsize_idx = __ffs(addr_merge);
		pgsize_idx = min(pgsize_idx, align_pgsize_idx);
	}

	/* build a mask of acceptable page sizes */
	pgsize = (1UL << (pgsize_idx + 1)) - 1;

	/* throw away page sizes not supported by the hardware */
	pgsize &= domain->pgsize_bitmap;

	/* make sure we're still sane */
	BUG_ON(!pgsize);

	/* pick the biggest page */
	pgsize_idx = __fls(pgsize);
	pgsize = 1UL << pgsize_idx;

	return pgsize;
}

static int exynos_iommu_map(int ch_num, unsigned long l_iova, phys_addr_t paddr,
		size_t size, int prot)
{
	struct sysmmu_drvdata *drvdata = &g_sysmmu_drvdata[ch_num];
	struct exynos_iommu_domain *domain = drvdata->domain;
	sysmmu_pte_t *entry;
	sysmmu_iova_t iova = (sysmmu_iova_t)l_iova;
	int ret = -ENOMEM;

	BUG_ON(domain->pgtable == NULL);

	entry = section_entry(domain->pgtable, iova);

	if (size == SECT_SIZE) {
		ret = lv1set_section(domain, entry, iova, paddr, prot,
				     &domain->lv2entcnt[lv1ent_offset(iova)]);
	} else {
		sysmmu_pte_t *pent;
		pent = alloc_lv2entry(domain, entry, iova,
			      &domain->lv2entcnt[lv1ent_offset(iova)],
			      GFP_ATOMIC);

		if (IS_ERR(pent))
			ret = PTR_ERR(pent);
		else
			ret = lv2set_page(pent, paddr, size, prot,
				       &domain->lv2entcnt[lv1ent_offset(iova)]);
	}

	if (ret)
		pr_err("%s: Failed(%d) to map %#zx bytes @ %#llx\n",
			__func__, ret, size, iova);

	return ret;
}

static size_t exynos_iommu_unmap(int ch_num, unsigned long l_iova, size_t size)
{
	struct sysmmu_drvdata *drvdata = &g_sysmmu_drvdata[ch_num];
	struct exynos_iommu_domain *domain = drvdata->domain;
	sysmmu_iova_t iova = (sysmmu_iova_t)l_iova;
	sysmmu_pte_t *sent, *pent;
	size_t err_pgsize;
	atomic_t *lv2entcnt = &domain->lv2entcnt[lv1ent_offset(iova)];

	BUG_ON(domain->pgtable == NULL);

	sent = section_entry(domain->pgtable, iova);

	if (lv1ent_section(sent)) {
		if (WARN_ON(size < SECT_SIZE)) {
			err_pgsize = SECT_SIZE;
			goto err;
		}

		*sent = 0;
		pgtable_flush(sent, sent + 1);
		size = SECT_SIZE;
		goto done;
	}

	if (unlikely(lv1ent_fault(sent))) {
		if (size > SECT_SIZE)
			size = SECT_SIZE;
		goto done;
	}

	/* lv1ent_page(sent) == true here */

	pent = page_entry(sent, iova);

	if (unlikely(lv2ent_fault(pent))) {
		size = SPAGE_SIZE;
		goto done;
	}

	if (lv2ent_small(pent)) {
		/* Check Duplicated IOMMU Unmp */
		sysmmu_pte_t *refcnt_buf;

		refcnt_buf = (pent + NUM_LV2ENTRIES);
		if (*refcnt_buf != 0) {
			*refcnt_buf = *refcnt_buf - 1;
			atomic_inc(lv2entcnt);
			goto done;
		}

		*pent = 0;
		size = SPAGE_SIZE;
		pgtable_flush(pent, pent + 1);
		atomic_inc(lv2entcnt);
		goto unmap_flpd;
	}

	/* lv1ent_large(pent) == true here */
	if (WARN_ON(size < LPAGE_SIZE)) {
		err_pgsize = LPAGE_SIZE;
		goto err;
	}

	clear_lv2_page_table(pent, SPAGES_PER_LPAGE);
	pgtable_flush(pent, pent + SPAGES_PER_LPAGE);
	size = LPAGE_SIZE;
	atomic_add(SPAGES_PER_LPAGE, lv2entcnt);

unmap_flpd:
	/* TODO: for v7, remove all */
	if (atomic_read(lv2entcnt) == NUM_LV2ENTRIES) {
		sysmmu_pte_t* free_buff;
		phys_addr_t __maybe_unused paddr;

		free_buff = page_entry(sent, 0);
		paddr = virt_to_phys(free_buff);
		lv2table_counter--;

		*sent = 0;
		pgtable_flush(sent, sent + 1);
		atomic_set(lv2entcnt, 0);

#ifdef USE_DYNAMIC_MEM_ALLOC
		if (free_buff >= domain->ext_buff[0].buff && free_buff
				<= domain->ext_buff[MAX_EXT_BUFF_NUM - 1].buff) {
			/* Allocated from extend buffer */
			u64 index =
				(u64)free_buff - (u64)domain->ext_buff[0].buff;
			index /= SZ_2K;
			if (index < MAX_EXT_BUFF_NUM) {
				pr_err("Extend buffer %llu free...\n", index);
				domain->ext_buff[index].used = 0;
			} else
				pr_err("WARN - Lv2table free ERR!!!\n");
		} else {
			kmem_cache_free(lv2table_kmem_cache,
					free_buff);
		}
#else
		if (addr_in_gen_pool(lv2table_pool, paddr, LV2TABLE_REFCNT_SZ))
			gen_pool_free(lv2table_pool, paddr, LV2TABLE_REFCNT_SZ);
		else
			kmem_cache_free(lv2table_kmem_cache, free_buff);
#endif
	}

done:

	return size;
err:
	pr_err("%s: Failed: size(%#zx)@%#llx is smaller than page size %#zx\n",
		__func__, size, iova, err_pgsize);

	return 0;
}

#ifdef CONFIG_PCIE_IOMMU_HISTORY_LOG
static inline void add_history_buff(struct history_buff *hbuff,
			phys_addr_t addr, phys_addr_t orig_addr,
			size_t size, size_t orig_size)
{
	ktime_t current_time;

	current_time = ktime_get();
	
	hbuff->save_addr[hbuff->index] = (u32)((addr >> 0x4) & 0xffffffff);
	hbuff->orig_addr[hbuff->index] = (u32)((orig_addr >> 0x4) & 0xffffffff);
	hbuff->size[hbuff->index] = size;
	hbuff->orig_size[hbuff->index] = orig_size;
	hbuff->time_ns[hbuff->index] = ktime_to_us(current_time);
	
	hbuff->index++;
	if (hbuff->index >= MAX_HISTROY_BUFF)
		hbuff->index = 0;
}
#endif

static inline int check_memory_validation(phys_addr_t paddr)
{
	int i;

	for (i = 0; i < max_dram_region; i++) {
		if (paddr >= ch_dram_region[i].start &&
				paddr <= ch_dram_region[i].end) {
			return 0;
		}
	}

	pr_err("Requested address %llx is NOT in DRAM rergion!!\n", paddr);

	return -EINVAL;
}

int pcie_iommu_map(int ch_num, unsigned long iova, phys_addr_t paddr, size_t size, int prot)
{
	struct sysmmu_drvdata *drvdata = &g_sysmmu_drvdata[ch_num];
	struct exynos_iommu_domain *domain = drvdata->domain;
	unsigned long orig_iova = iova;
	unsigned int min_pagesz;
	size_t orig_size = size;
	phys_addr_t __maybe_unused orig_paddr = paddr;
	int ret = 0;
	unsigned long changed_iova, changed_size;
	unsigned long flags;

	if (check_memory_validation(paddr) != 0) {
		pr_err("WARN - Unexpected address request : 0x%llx\n", paddr);
		return -EINVAL;
	}

	/* Make sure start address align least 4KB */
	if ((iova & SYSMMU_4KB_MASK) != 0) {
		size += iova & SYSMMU_4KB_MASK;
		iova &= ~(SYSMMU_4KB_MASK);
		paddr &= ~(SYSMMU_4KB_MASK);
	}
	/* Make sure size align least 4KB */
	if ((size & SYSMMU_4KB_MASK) != 0) {
		size &= ~(SYSMMU_4KB_MASK);
		size += SZ_4K;
	}

	/* Check for debugging */
	changed_iova = iova;
	changed_size = size;

	/* find out the minimum page size supported */
	min_pagesz = 1 << __ffs(domain->pgsize_bitmap);

	/*
	 * both the virtual address and the physical one, as well as
	 * the size of the mapping, must be aligned (at least) to the
	 * size of the smallest page supported by the hardware
	 */
	if (!IS_ALIGNED(iova | paddr | size, min_pagesz)) {
		pr_err("unaligned: iova 0x%lx pa %p sz 0x%zx min_pagesz 0x%x\n",
		       iova, &paddr, size, min_pagesz);
		return -EINVAL;
	}

	spin_lock_irqsave(&domain->pgtablelock, flags);
	while (size) {
		size_t pgsize = iommu_pgsize(ch_num, iova | paddr, size);
		pr_debug("mapping: iova 0x%lx pa %pa pgsize 0x%zx\n",
			 iova, &paddr, pgsize);

		alloc_counter++;
		if (alloc_counter > max_req_cnt)
			max_req_cnt = alloc_counter;
		ret = exynos_iommu_map(ch_num, iova, paddr, pgsize, prot);
		exynos_sysmmu_tlb_invalidate(ch_num, iova, pgsize);

#ifdef CONFIG_PCIE_IOMMU_HISTORY_LOG
		add_history_buff(&pcie_map_history, paddr, orig_paddr,
				changed_size, orig_size);
#endif
		if (ret)
			break;

		iova += pgsize;
		paddr += pgsize;
		size -= pgsize;
	}
	spin_unlock_irqrestore(&domain->pgtablelock, flags);

	/* unroll mapping in case something went wrong */
	if (ret) {
		pr_err("PCIe SysMMU mapping Error!\n");
		pcie_iommu_unmap(ch_num, orig_iova, orig_size - size);
	}

	pr_debug("mapped: req 0x%lx(org : 0x%lx)  size 0x%zx(0x%zx)\n",
			changed_iova, orig_iova, changed_size, orig_size);

	return ret;
}
EXPORT_SYMBOL_GPL(pcie_iommu_map);

size_t pcie_iommu_unmap(int ch_num, unsigned long iova, size_t size)
{
	struct sysmmu_drvdata *drvdata = &g_sysmmu_drvdata[ch_num];
	struct exynos_iommu_domain *domain = drvdata->domain;
	size_t unmapped_page, unmapped = 0;
	unsigned int min_pagesz;
	unsigned long __maybe_unused orig_iova = iova;
	unsigned long __maybe_unused changed_iova;
	size_t __maybe_unused orig_size = size;
	unsigned long flags;

	/* Make sure start address align least 4KB */
	if ((iova & SYSMMU_4KB_MASK) != 0) {
		size += iova & SYSMMU_4KB_MASK;
		iova &= ~(SYSMMU_4KB_MASK);
	}
	/* Make sure size align least 4KB */
	if ((size & SYSMMU_4KB_MASK) != 0) {
		size &= ~(SYSMMU_4KB_MASK);
		size += SZ_4K;
	}

	changed_iova = iova;

	/* find out the minimum page size supported */
	min_pagesz = 1 << __ffs(domain->pgsize_bitmap);

	/*
	 * The virtual address, as well as the size of the mapping, must be
	 * aligned (at least) to the size of the smallest page supported
	 * by the hardware
	 */
	if (!IS_ALIGNED(iova | size, min_pagesz)) {
		pr_err("unaligned: iova 0x%lx size 0x%zx min_pagesz 0x%x\n",
		       iova, size, min_pagesz);
		return -EINVAL;
	}

	pr_debug("unmap this: iova 0x%lx size 0x%zx\n", iova, size);

	spin_lock_irqsave(&domain->pgtablelock, flags);
	/*
	 * Keep iterating until we either unmap 'size' bytes (or more)
	 * or we hit an area that isn't mapped.
	 */
	while (unmapped < size) {
		size_t pgsize = iommu_pgsize(ch_num, iova, size - unmapped);

		alloc_counter--;
		unmapped_page = exynos_iommu_unmap(ch_num, iova, pgsize);
		exynos_sysmmu_tlb_invalidate(ch_num, iova, pgsize);

#ifdef CONFIG_PCIE_IOMMU_HISTORY_LOG
		add_history_buff(&pcie_unmap_history, iova, orig_iova,
				size, orig_size);
#endif
		if (!unmapped_page)
			break;

		pr_debug("unmapped: iova 0x%lx size 0x%zx\n",
			 iova, unmapped_page);

		iova += unmapped_page;
		unmapped += unmapped_page;
	}

	spin_unlock_irqrestore(&domain->pgtablelock, flags);

	pr_debug("UNMAPPED : req 0x%lx(0x%lx) size 0x%zx(0x%zx)\n",
				changed_iova, orig_iova, size, orig_size);

	return unmapped;
}
EXPORT_SYMBOL_GPL(pcie_iommu_unmap);

static int __init sysmmu_parse_dt(struct device *sysmmu,
				struct sysmmu_drvdata *drvdata)
{
	unsigned int qos = DEFAULT_QOS_VALUE;
#if defined(CONFIG_SOC_EXYNOS9810)
	const char *use_tlb_pinning;
#endif
	int ret = 0;

	/* Parsing QoS */
	ret = of_property_read_u32_index(sysmmu->of_node, "qos", 0, &qos);
	if (!ret && (qos > 15)) {
		dev_err(sysmmu, "Invalid QoS value %d, use default.\n", qos);
		qos = DEFAULT_QOS_VALUE;
	}
	drvdata->qos = qos;

	if (of_property_read_bool(sysmmu->of_node, "sysmmu,no-suspend"))
		dev_pm_syscore_device(sysmmu, true);
#if defined(CONFIG_SOC_EXYNOS9810)
	if (!of_property_read_string(sysmmu->of_node,
				"use-tlb-pinning", &use_tlb_pinning)) {
		if (!strcmp(use_tlb_pinning, "true")) {
			dev_err(sysmmu, "Enable TLB Pinning.\n");
			drvdata->use_tlb_pinning = true;
		} else if (!strcmp(use_tlb_pinning, "false")) {
			drvdata->use_tlb_pinning = false;
		} else {
			dev_err(sysmmu, "Invalid TLB pinning value"
				       "(set to default -> false)\n");
			drvdata->use_tlb_pinning = false;
		}
	} else {
		drvdata->use_tlb_pinning = false;
	}
#endif
	return 0;
}

static struct exynos_iommu_domain *exynos_iommu_domain_alloc(void)
{
	struct exynos_iommu_domain *domain;
	int __maybe_unused i;

	domain = kzalloc(sizeof(*domain), GFP_KERNEL);
	if (!domain)
		return NULL;

	/* 36bit VA FLPD must be aligned in 256KB */
	domain->pgtable =
		(sysmmu_pte_t *)__get_free_pages(GFP_KERNEL | __GFP_ZERO, 6);
	if (!domain->pgtable)
		goto err_pgtable;

	domain->lv2entcnt = (atomic_t *)
			__get_free_pages(GFP_KERNEL | __GFP_ZERO, 6);
	if (!domain->lv2entcnt)
		goto err_counter;

	pgtable_flush(domain->pgtable, domain->pgtable + NUM_LV1ENTRIES);

	spin_lock_init(&domain->lock);
	spin_lock_init(&domain->pgtablelock);
	INIT_LIST_HEAD(&domain->clients_list);

	/* TODO: get geometry from device tree */
	domain->domain.geometry.aperture_start = 0;
	domain->domain.geometry.aperture_end   = ~0UL;
	domain->domain.geometry.force_aperture = true;

	domain->pgsize_bitmap = SECT_SIZE | LPAGE_SIZE | SPAGE_SIZE;

#ifdef USE_DYNAMIC_MEM_ALLOC
	domain->ext_buff[0].buff =
		kzalloc(SZ_2K * MAX_EXT_BUFF_NUM, GFP_KERNEL);

	if (!domain->ext_buff[0].buff) {
		pr_err("Can't allocate extend buffer!\n");
		goto err_counter;
	}

	/* ext_buff[0] is already initialized */
	for (i = 1; i < MAX_EXT_BUFF_NUM; i ++) {
		domain->ext_buff[i].index = i;
		domain->ext_buff[i].used = 0;
		domain->ext_buff[i].buff =
			domain->ext_buff[i - 1].buff +
			(SZ_2K / sizeof(sysmmu_pte_t));
	}
#endif

	return domain;

err_counter:
	free_pages((unsigned long)domain->pgtable, 6);
err_pgtable:
	kfree(domain);
	return NULL;
}


static int __init exynos_sysmmu_probe(struct platform_device *pdev)
{
	int irq, ret;
	struct device *dev = &pdev->dev;
	struct sysmmu_drvdata *data;
	struct resource *res;
	int __maybe_unused i;
	int ch_num;
	struct device_node *np = pdev->dev.of_node;

	if (of_property_read_u32(np, "ch-num", &ch_num)) {
		dev_err(dev, "Failed to parse the channel number\n");
		return -EINVAL;
	}
	data = &g_sysmmu_drvdata[ch_num];
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "Failed to get resource info\n");
		return -ENOENT;
	}

	data->sfrbase = devm_ioremap_resource(dev, res);
	if (IS_ERR(data->sfrbase))
		return PTR_ERR(data->sfrbase);

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		dev_err(dev, "Unable to find IRQ resource\n");
		return irq;
	}

	ret = devm_request_irq(dev, irq, exynos_sysmmu_irq, 0,
				dev_name(dev), data);
	if (ret) {
		dev_err(dev, "Unabled to register handler of irq %d\n", irq);
		return ret;
	}

	data->clk = devm_clk_get(dev, "aclk");
	if (IS_ERR(data->clk)) {
		dev_err(dev, "Failed to get clock!\n");
		return PTR_ERR(data->clk);
	} else  {
		ret = clk_prepare(data->clk);
		if (ret) {
			dev_err(dev, "Failed to prepare clk\n");
			return ret;
		}
	}
	data->ch_num = ch_num;
	data->sysmmu = dev;
	spin_lock_init(&data->lock);
	ATOMIC_INIT_NOTIFIER_HEAD(&data->fault_notifiers);

	platform_set_drvdata(pdev, data);

	data->qos = DEFAULT_QOS_VALUE;
	data->version = get_hw_version(data, data->sfrbase);

	ret = sysmmu_parse_dt(data->sysmmu, data);
	if (ret) {
		dev_err(dev, "Failed to parse DT\n");
		return ret;
	}
	/* Create Temp Domain */
	data->domain = exynos_iommu_domain_alloc();
	data->pgtable = virt_to_phys(data->domain->pgtable);

	dev_info(dev, "L1Page Table Address : 0x%llx(phys)\n", data->pgtable);

	dev_info(data->sysmmu, "is probed. Version %d.%d.%d\n",
			MMU_MAJ_VER(data->version),
			MMU_MIN_VER(data->version),
			MMU_REV_VER(data->version));

	max_dram_region = memblock.memory.cnt;
	if (max_dram_region >= NUM_DRAM_REGION) {
		dev_err(dev, "DRAM region for validation is too big!!!\n");
		return -EINVAL;
	}

	for (i = 0; i < max_dram_region; i++) {
		ch_dram_region[i].start = memblock.memory.regions[i].base;
		ch_dram_region[i].end = memblock.memory.regions[i].base +
				memblock.memory.regions[i].size;
		dev_info(dev, "Valid DRAM region%d : 0x%llx ~ 0x%llx\n", i,
				ch_dram_region[i].start, ch_dram_region[i].end);
	}
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int exynos_sysmmu_suspend(struct device *dev)
{
	unsigned long flags;
	struct sysmmu_drvdata *drvdata = dev_get_drvdata(dev);

	spin_lock_irqsave(&drvdata->lock, flags);
	drvdata->is_suspended = true;
	spin_unlock_irqrestore(&drvdata->lock, flags);

	return 0;
}

static int exynos_sysmmu_resume(struct device *dev)
{
	unsigned long flags;
	struct sysmmu_drvdata *drvdata = dev_get_drvdata(dev);

	spin_lock_irqsave(&drvdata->lock, flags);
	drvdata->is_suspended = false;
	spin_unlock_irqrestore(&drvdata->lock, flags);

	return 0;
}
#endif

static const struct dev_pm_ops sysmmu_pm_ops = {
	SET_LATE_SYSTEM_SLEEP_PM_OPS(exynos_sysmmu_suspend,
					exynos_sysmmu_resume)
};

static const struct of_device_id sysmmu_of_match[] = {
	{ .compatible	= "samsung,pcie-sysmmu", },
	{ },
};

static struct platform_driver exynos_sysmmu_driver __refdata = {
	.probe	= exynos_sysmmu_probe,
	.driver	= {
		.name		= "pcie-sysmmu",
		.of_match_table	= sysmmu_of_match,
		.pm		= &sysmmu_pm_ops,
	}
};

static int __init pcie_iommu_init(void)
{
	phys_addr_t buff_paddr;
	int ret;

	if (lv2table_kmem_cache)
		return 0;

	lv2table_kmem_cache = kmem_cache_create("pcie-iommu-lv2table",
			LV2TABLE_REFCNT_SZ, LV2TABLE_REFCNT_SZ, 0, NULL);
	if (!lv2table_kmem_cache) {
		pr_err("%s: Failed to create kmem cache\n", __func__);
		return -ENOMEM;
	}

	gen_buff = kzalloc(LV2_GENPOOL_SZIE, GFP_KERNEL);
	if (gen_buff == NULL) {
		pr_err("Failed to allocate pool buffer\n");
		ret = -ENOMEM;
		goto err3;
	}
	buff_paddr = virt_to_phys(gen_buff);
#ifndef USE_DYNAMIC_MEM_ALLOC
	lv2table_pool = gen_pool_create(ilog2(LV2TABLE_REFCNT_SZ), -1);
	if (!lv2table_pool) {
		pr_err("Failed to allocate lv2table gen pool\n");
		ret = -ENOMEM;
		goto err2;
	}

	ret = gen_pool_add(lv2table_pool, (unsigned long)buff_paddr,
			LV2_GENPOOL_SZIE, -1);
	if (ret) {
		ret = -ENOMEM;
		goto err1;
	}
#endif


	ret = platform_driver_probe(&exynos_sysmmu_driver, exynos_sysmmu_probe);
	if (ret == 0)
		return ret;

err1:
#ifndef USE_DYNAMIC_MEM_ALLOC
	gen_pool_destroy(lv2table_pool);
#endif
err2:
	kzfree(gen_buff);
err3:
	kmem_cache_destroy(lv2table_kmem_cache);

	return ret;

}
subsys_initcall(pcie_iommu_init);

