/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/io.h>
#include <linux/delay.h>

#include "score_log.h"
#include "score_regs.h"
#include "score_dpmu.h"
#include "score_firmware.h"
#include "score_device.h"
#include "score_system.h"
#include "score_profiler.h"

/*
 * If value of cctrl_status sfr is not 0, check again after delay time
 *   The unit is microsecond
 */
#define SCORE_DCACHE_CHECK_DELAY	(10)
/**
 * If value of cctrl_status sfr is not 0, check again 'retry number' times
 * 'retry number' is 1 ms divided by delay time
 */
#define SCORE_DCACHE_RETRY		(1000/SCORE_DCACHE_CHECK_DELAY)
/*
 * If value of sw_reset sfr is not 0, check again after delay time
 *   The unit is microsecond
 */
#define SCORE_RESET_CHECK_DELAY		(10)
/**
 * If value of sw_reset sfr is not 0, check again 'retry number' times
 * 'retry number' is 1 ms divided by delay time
 */
#define SCORE_RESET_RETRY		(1000/SCORE_RESET_CHECK_DELAY)

/**
 * score_system_dcache_region_control - Control regional dcache
 * @system:	[in]	object about score_system structure
 * @id:		[in]	core id
 * @cmd:	[in]	command for cache control
 * @addr:	[in]	target address
 * @sizee:	[in]	memory size
 *
 * Returns 0 if succeede, otherwise errorno
 */
int score_system_dcache_region_control(struct score_system *system,
		int id, int cmd, dma_addr_t addr, size_t size)
{
	int ret = 0;
	int idx;
	size_t aligned_size;
	size_t cache_line;

	score_enter();
	if (!(cmd > DCACHE_CMD_START && cmd < DCACHE_CMD_END)) {
		ret = -EINVAL;
		score_err("Command for regional dc_control is wrong (%d,%d)\n",
				id, cmd);
		goto p_err;
	}

	/* aligned unit of cache is 64 byte */
	aligned_size = size + (addr & 0x3f);
	cache_line = ((aligned_size + 0x3f) >> 6) - 1;
	/*
	 * DC_RCTRL_SIZE[8:0] is one less than cache line size, not memory size.
	 * Up to 32KB can be used. (MAX : (0x1ff + 0x1) * 0x40 = 0x8000)
	 */
	if (cache_line & ~0x1ff) {
		ret = -EINVAL;
		score_err("Regional control size exceeded ([%d,%d] %zx, %zx)\n",
			id, cmd, cache_line, size);
		goto p_err;
	}

	/*
	 * The lower six bits[5:0] of DC_RCTRL_ADDR[31:6] are not used
	 * because of the aliged cache line. This six bits are always zero.
	 */
	spin_lock(&system->cache_slock);
	if (id == SCORE_MASTER) {
		writel(addr, system->sfr + MC_CACHE_DC_RCTRL_ADDR);
		writel(cache_line, system->sfr + MC_CACHE_DC_RCTRL_SIZE);
		writel(cmd, system->sfr + MC_CACHE_DC_RCTRL_REQ);
		for (idx = 0; idx < SCORE_DCACHE_RETRY; ++idx) {
			ret = readl(system->sfr + MC_CACHE_DC_RCTRL_STATUS);
			if (ret == 0)
				break;
			udelay(SCORE_DCACHE_CHECK_DELAY);
		}
	} else if (id == SCORE_KNIGHT1) {
		writel(addr, system->sfr + KC1_CACHE_DC_RCTRL_ADDR);
		writel(cache_line, system->sfr + KC1_CACHE_DC_RCTRL_SIZE);
		writel(cmd, system->sfr + KC1_CACHE_DC_RCTRL_REQ);
		for (idx = 0; idx < SCORE_DCACHE_RETRY; ++idx) {
			ret = readl(system->sfr + KC1_CACHE_DC_RCTRL_STATUS);
			if (ret == 0)
				break;
			udelay(SCORE_DCACHE_CHECK_DELAY);
		}
	} else if (id == SCORE_KNIGHT2) {
		writel(addr, system->sfr + KC2_CACHE_DC_RCTRL_ADDR);
		writel(cache_line, system->sfr + KC2_CACHE_DC_RCTRL_SIZE);
		writel(cmd, system->sfr + KC2_CACHE_DC_RCTRL_REQ);
		for (idx = 0; idx < SCORE_DCACHE_RETRY; ++idx) {
			ret = readl(system->sfr + KC2_CACHE_DC_RCTRL_STATUS);
			if (ret == 0)
				break;
			udelay(SCORE_DCACHE_CHECK_DELAY);
		}
	}
	spin_unlock(&system->cache_slock);

	if (ret) {
		ret = -ETIMEDOUT;
		score_err("Regional control failed ([%d,%d] %d us, %d times)\n",
				id, cmd, SCORE_DCACHE_CHECK_DELAY,
				SCORE_DCACHE_RETRY);
		goto p_err;
	}

	score_leave();
p_err:
	return ret;
}

/**
 * score_system_dcache_control - Control dcache of a particular core
 * @system:	[in]	object about score_system structure
 * @id:		[in]	core id
 * @cmd:	[in]	command for cache control
 *
 * Returns 0 if succeede, otherwise errorno
 */
int score_system_dcache_control(struct score_system *system, int id, int cmd)
{
	int idx;
	int ret = 0;

	score_enter();
	if (!(cmd > DCACHE_CMD_START && cmd < DCACHE_CMD_END)) {
		ret = -EINVAL;
		score_err("Command for dc_control is wrong ([%d]%d)\n",
				id, cmd);
		goto p_err;
	}

	spin_lock(&system->cache_slock);
	if (id == SCORE_MASTER) {
		writel(cmd, system->sfr + MC_CACHE_DC_CCTRL_REQ);
		for (idx = 0; idx < SCORE_DCACHE_RETRY; ++idx) {
			ret = readl(system->sfr + MC_CACHE_DC_CCTRL_STATUS);
			if (ret == 0)
				break;
			udelay(SCORE_DCACHE_CHECK_DELAY);
		}
	} else if (id == SCORE_KNIGHT1) {
		writel(cmd, system->sfr + KC1_CACHE_DC_CCTRL_REQ);
		for (idx = 0; idx < SCORE_DCACHE_RETRY; ++idx) {
			ret = readl(system->sfr + KC1_CACHE_DC_CCTRL_STATUS);
			if (ret == 0)
				break;
			udelay(SCORE_DCACHE_CHECK_DELAY);
		}
	} else if (id == SCORE_KNIGHT2) {
		writel(cmd, system->sfr + KC2_CACHE_DC_CCTRL_REQ);
		for (idx = 0; idx < SCORE_DCACHE_RETRY; ++idx) {
			ret = readl(system->sfr + KC2_CACHE_DC_CCTRL_STATUS);
			if (ret == 0)
				break;
			udelay(SCORE_DCACHE_CHECK_DELAY);
		}
	}
	spin_unlock(&system->cache_slock);

	if (ret) {
		ret = -ETIMEDOUT;
		score_err("dcache control failed ([%d,%d] %d us, %d times)\n",
				id, cmd, SCORE_DCACHE_CHECK_DELAY,
				SCORE_DCACHE_RETRY);
	}

	score_leave();
p_err:
	return ret;
}

/**
 * score_system_dcache_control_all - Control dcache of all SCore cores
 * @system:	[in]	object about score_system structure
 * @cmd:	[in]	command for cache control
 *
 * Returns 0 if succeede, otherwise errorno
 */
int score_system_dcache_control_all(struct score_system *system, int cmd)
{
	int ret;

	score_enter();
	ret = score_system_dcache_control(system, SCORE_MASTER, cmd);
	if (ret)
		goto p_end;
	ret = score_system_dcache_control(system, SCORE_KNIGHT1, cmd);
	if (ret)
		goto p_end;
	ret = score_system_dcache_control(system, SCORE_KNIGHT2, cmd);
	if (ret)
		goto p_end;

	score_leave();
p_end:
	return ret;
}

/**
 * score_system_halt -
 * @system:	[in]	object about score_system structure
 *
 * Returns 0 if succeede, otherwise errorno
 */
int score_system_halt(struct score_system *system)
{
	int ret = 0;

	score_enter();
	ret = score_interface_target_halt(&system->interface, SCORE_KNIGHT1);
	if (ret)
		goto p_end;

	ret = score_interface_target_halt(&system->interface, SCORE_KNIGHT2);
	if (ret)
		goto p_end;
	score_leave();
p_end:
	return ret;
}

/**
 * score_system_sw_reset - Reset SCore device
 * @system:	[in]	object about score_system structure
 *
 * Returns 0 if succeede, otherwise errorno
 */
int score_system_sw_reset(struct score_system *system)
{
	int idx;
	int ret = 0;

	score_enter();
	writel(0x1, system->sfr + SW_RESET);
	for (idx = 0; idx < SCORE_RESET_RETRY; ++idx) {
		ret = readl(system->sfr + SW_RESET);
		if (ret == 0)
			break;
		udelay(SCORE_RESET_CHECK_DELAY);
	}

	if (ret) {
		ret = -ETIMEDOUT;
		score_err("sw_reset failed (checked every %d us/%d times)\n",
				SCORE_RESET_CHECK_DELAY, SCORE_RESET_RETRY);
	}

	score_leave();
	return ret;
}

static int __score_system_set_firmware(struct score_system *system)
{
	int ret = 0;
	struct score_firmware_manager *fwmgr;
	dma_addr_t text_addr;
	dma_addr_t data_addr;

	score_enter();
	fwmgr = &system->fwmgr;

	ret = score_firmware_manager_get_dvaddr(fwmgr,
			SCORE_MASTER, &text_addr, &data_addr);
	if (ret)
		goto p_err;

	writel(text_addr, system->sfr + MC_CACHE_IC_BASEADDR);
	writel(data_addr, system->sfr + MC_CACHE_DC_BASEADDR);

	ret = score_firmware_manager_get_dvaddr(fwmgr, SCORE_KNIGHT1,
			&text_addr, &data_addr);
	if (ret)
		goto p_err;

	writel(text_addr, system->sfr + KC1_CACHE_IC_BASEADDR);
	writel(data_addr, system->sfr + KC1_CACHE_DC_BASEADDR);

	ret = score_firmware_manager_get_dvaddr(fwmgr, SCORE_KNIGHT2,
			&text_addr, &data_addr);
	if (ret)
		goto p_err;

	writel(text_addr, system->sfr + KC2_CACHE_IC_BASEADDR);
	writel(data_addr, system->sfr + KC2_CACHE_DC_BASEADDR);

	score_leave();
p_err:
	return ret;
}

/**
 * score_system_wakeup - Wake up SCore device
 * @system:	[in]	object about score_system structure
 */
void score_system_wakeup(struct score_system *system)
{
	score_enter();
	writel(0x1, system->sfr + MC_WAKEUP);
	writel(0x1, system->sfr + KC1_WAKEUP);
	writel(0x1, system->sfr + KC2_WAKEUP);
	score_leave();
}

static void __score_system_init(struct score_system *system)
{
	score_enter();
	score_mmu_init(&system->mmu);
	score_scq_init(&system->scq);
	score_print_init(&system->print);

	/*
	 * CACHE signal [3:0]
	 * [0]: Bufferable
	 * [1]: Cacheable
	 * [2]: Read allocate
	 * [3]: Write allocate
	 */
	writel(0x2, system->sfr + AXIM0_ARCACHE);
	writel(0x2, system->sfr + AXIM0_AWCACHE);
	writel(0x2, system->sfr + AXIM2_ARCACHE);
	writel(0x2, system->sfr + AXIM2_AWCACHE);

	writel(0x3ffffef, system->sfr + SM_MCGEN);
	writel(0x0, system->sfr + CLK_REQ);

	writel(0x0, system->sfr + MC_INIT_DONE_CHECK);
	writel(0x0, system->sfr + KC1_INIT_DONE_CHECK);
	writel(0x0, system->sfr + KC2_INIT_DONE_CHECK);

	score_profiler_init();
	score_leave();
}

/**
 * score_system_boot - Boot SCore device
 * @system:	[in]	object about score_system structure
 *
 * Returns 0 if succeeded, otherwise errorno
 */
int score_system_boot(struct score_system *system)
{
	int ret = 0;

	score_enter();
	ret = score_system_sw_reset(system);
	if (ret)
		goto p_err;

	score_dpmu_enable();

	ret = __score_system_set_firmware(system);
	if (ret)
		goto p_err;

	__score_system_init(system);
	score_system_wakeup(system);

	score_leave();
p_err:
	return ret;
}

int score_system_start(struct score_system *system, unsigned int firmware_id)
{
	int ret = 0;
	void *kvaddr;
	dma_addr_t dvaddr;

	score_enter();
	kvaddr = score_mmu_get_internal_mem_kvaddr(&system->mmu);
	dvaddr = score_mmu_get_internal_mem_dvaddr(&system->mmu);
	ret = score_firmware_manager_open(&system->fwmgr,
			firmware_id, kvaddr, dvaddr);
	if (ret)
		goto p_err_firmware_manager;

	ret = score_scq_open(&system->scq);
	if (ret)
		goto p_err_scq;

	ret = score_sm_open(&system->sm);
	if (ret)
		goto p_err_sm;

	ret = score_sr_open(&system->sr);
	if (ret)
		goto p_err_sr;

	ret = score_print_open(&system->print);
	if (ret)
		goto p_err_print;

	/*
	 * Profiler need to be opened before device boot up as
	 * we don't want override tracing bufer tail or head
	 */
	ret = score_profiler_open(
		score_mmu_get_profiler_kvaddr(&system->mmu, SCORE_MASTER),
		score_mmu_get_profiler_kvaddr(&system->mmu, SCORE_KNIGHT1),
		score_mmu_get_profiler_kvaddr(&system->mmu, SCORE_KNIGHT2));
	if (ret)
		goto p_err_profiler;

	ret = score_system_boot(system);
	if (ret)
		goto p_err_boot;

	score_leave();
	return ret;
p_err_boot:
	score_profiler_close();
p_err_profiler:
	score_print_close(&system->print);
p_err_print:
	score_sr_close(&system->sr);
p_err_sr:
	score_sm_close(&system->sm);
p_err_sm:
	score_scq_close(&system->scq);
p_err_scq:
	score_firmware_manager_close(&system->fwmgr);
p_err_firmware_manager:
	return ret;
}

void score_system_stop(struct score_system *system)
{
	score_enter();
	score_profiler_close();
	score_print_close(&system->print);

	score_system_halt(system);
	score_system_sw_reset(system);

	score_sr_close(&system->sr);
	score_sm_close(&system->sm);
	score_scq_close(&system->scq);

	score_firmware_manager_close(&system->fwmgr);
	score_leave();
}

int score_system_open(struct score_system *system)
{
	int ret = 0;

	score_enter();
	ret = score_mmu_open(&system->mmu);
	if (ret)
		goto p_err_mmu;

	ret = score_interface_open(&system->interface);
	if (ret)
		goto p_err_interface;

	score_leave();
	return ret;
p_err_interface:
	score_mmu_close(&system->mmu);
p_err_mmu:
	return ret;
}

void score_system_close(struct score_system *system)
{
	score_enter();
	score_interface_close(&system->interface);
	score_mmu_close(&system->mmu);
	score_leave();
}

int score_system_probe(struct score_device *device)
{
	int ret = 0;
	struct score_system *system;
	struct platform_device *pdev;
	struct resource *res;
	void __iomem *regs;

	score_enter();
	system = &device->system;
	pdev = device->pdev;
	system->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		score_err("platform_get_resource(0) is fail(%p)", res);
		ret = -EINVAL;
		goto p_err_resource;
	}

	regs = devm_ioremap_resource(system->dev, res);
	if (IS_ERR(regs)) {
		score_err("devm_ioremap_resource(0) is fail(%p)", regs);
		ret = PTR_ERR(regs);
		goto p_err_ioremap;
	}

	system->sfr = regs;
	system->sfr_size = resource_size(res);

	ret = score_interface_probe(system);
	if (ret)
		goto p_err_interface;

	ret = score_mmu_probe(system);
	if (ret)
		goto p_err_mmu;

	ret = score_firmware_manager_probe(system);
	if (ret)
		goto p_err_firmware_manager;

	ret = score_scq_probe(system);
	if (ret)
		goto p_err_scq;

	ret = score_sm_probe(system);
	if (ret)
		goto p_err_sm;

	ret = score_sr_probe(system);
	if (ret)
		goto p_err_sr;

	ret = score_print_probe(system);
	if (ret)
		goto p_err_print;

	score_dpmu_init(system->sfr);
	spin_lock_init(&system->cache_slock);

	score_leave();
	return ret;
p_err_print:
	score_sr_remove(&system->sr);
p_err_sr:
	score_sm_remove(&system->sm);
p_err_sm:
	score_scq_remove(&system->scq);
p_err_scq:
	score_firmware_manager_remove(&system->fwmgr);
p_err_firmware_manager:
	score_mmu_remove(&system->mmu);
p_err_mmu:
	score_interface_remove(&system->interface);
p_err_interface:
	devm_iounmap(system->dev, system->sfr);
p_err_ioremap:
p_err_resource:
	return ret;
}

void score_system_remove(struct score_system *system)
{
	score_enter();
	score_print_remove(&system->print);
	score_sr_remove(&system->sr);
	score_sm_remove(&system->sm);
	score_scq_remove(&system->scq);
	score_firmware_manager_remove(&system->fwmgr);
	score_mmu_remove(&system->mmu);
	score_interface_remove(&system->interface);
	devm_iounmap(system->dev, system->sfr);
	score_leave();
}
