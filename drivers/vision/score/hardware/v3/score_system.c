/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
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
		score_err("Command for dc_control is wrong (%d/%d)\n",
				id, cmd);
		goto p_err;
	}

	spin_lock(&system->cache_slock);
	if (id == SCORE_TS) {
		writel(cmd, system->sfr + TS_CACHE_DC_CTRL_REQ);
		for (idx = 0; idx < CHECK_RETRY_COUNT; ++idx) {
			ret = readl(system->sfr + TS_CACHE_DC_CTRL_STATUS);
			if (ret == 0)
				break;
			udelay(CHECK_RETRY_DELAY);
		}
	} else if (id == SCORE_BARON1) {
		writel(cmd, system->sfr + BR1_CACHE_DC_CTRL_REQ);
		for (idx = 0; idx < CHECK_RETRY_COUNT; ++idx) {
			ret = readl(system->sfr + BR1_CACHE_DC_CTRL_STATUS);
			if (ret == 0)
				break;
			udelay(CHECK_RETRY_DELAY);
		}
	} else if (id == SCORE_BARON2) {
		writel(cmd, system->sfr + BR2_CACHE_DC_CTRL_REQ);
		for (idx = 0; idx < CHECK_RETRY_COUNT; ++idx) {
			ret = readl(system->sfr + BR2_CACHE_DC_CTRL_STATUS);
			if (ret == 0)
				break;
			udelay(CHECK_RETRY_DELAY);
		}
	}
	spin_unlock(&system->cache_slock);

	if (ret) {
		score_err("dcache control failed (%d/%d/%d/%d/%#x)\n",
				id, cmd, CHECK_RETRY_DELAY,
				CHECK_RETRY_COUNT, ret);
		ret = -ETIMEDOUT;
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
	ret = score_system_dcache_control(system, SCORE_TS, cmd);
	if (ret)
		goto p_end;
	ret = score_system_dcache_control(system, SCORE_BARON1, cmd);
	if (ret)
		goto p_end;
	ret = score_system_dcache_control(system, SCORE_BARON2, cmd);
	if (ret)
		goto p_end;

	score_leave();
p_end:
	return ret;
}

static int __score_system_check_dma_idle(struct score_system *system)
{
	int ret = 0;
	int idx;

	score_enter();
	for (idx = 0; idx < CHECK_RETRY_COUNT; ++idx) {
		ret = readl(system->sfr + DMA0_QUEUE_STATUS) & (1 << 12);
		if (ret == 0)
			break;
		udelay(CHECK_RETRY_DELAY);
	}
	if (ret) {
		score_warn("dma0 is unstable\n");
		ret = -ETIMEDOUT;
		goto p_end;
	}

	for (idx = 0; idx < CHECK_RETRY_COUNT; ++idx) {
		ret = readl(system->sfr + DMA1_QUEUE_STATUS) & (1 << 12);
		if (ret == 0)
			break;
		udelay(CHECK_RETRY_DELAY);
	}
	if (ret) {
		score_warn("dma1 is unstable\n");
		ret = -ETIMEDOUT;
		goto p_end;
	}

	for (idx = 0; idx < CHECK_RETRY_COUNT; ++idx) {
		ret = readl(system->sfr + DMA2_QUEUE_STATUS) & (1 << 12);
		if (ret == 0)
			break;
		udelay(CHECK_RETRY_DELAY);
	}
	if (ret) {
		score_warn("dma2 is unstable\n");
		ret = -ETIMEDOUT;
		goto p_end;
	}

	for (idx = 0; idx < CHECK_RETRY_COUNT; ++idx) {
		ret = readl(system->sfr + DMA3_QUEUE_STATUS) & (1 << 12);
		if (ret == 0)
			break;
		udelay(CHECK_RETRY_DELAY);
	}
	if (ret) {
		score_warn("dma3 is unstable\n");
		ret = -ETIMEDOUT;
		goto p_end;
	}

	score_leave();
p_end:
	return ret;
}

/**
 * score_system_halt - Requests the target to stop by interrupt
 * @system:     [in]    object about score_system structure
 *
 * Returns 0 if succeede, otherwise errorno
 */
int score_system_halt(struct score_system *system)
{
	int ret = 0;

	score_enter();
	ret = score_interface_target_halt(&system->interface, SCORE_TS);
	if (ret)
		goto p_end;
	ret = score_interface_target_halt(&system->interface, SCORE_BARON);
	if (ret)
		goto p_end;

	ret = __score_system_check_dma_idle(system);
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
	for (idx = 0; idx < CHECK_RETRY_COUNT; ++idx) {
		ret = readl(system->sfr + SW_RESET);
		if (ret == 0)
			break;
		udelay(CHECK_RETRY_DELAY);
	}

	if (ret) {
		ret = -ETIMEDOUT;
		score_err("sw_reset failed (checked every %d us/%d times)\n",
				CHECK_RETRY_DELAY, CHECK_RETRY_COUNT);
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
			SCORE_TS, &text_addr, &data_addr);
	if (ret)
		goto p_err;

	writel(text_addr, system->sfr + TS_CACHE_IC_BASE_ADDR);
	writel(data_addr, system->sfr + TS_CACHE_DC_BASE_ADDR);

	ret = score_firmware_manager_get_dvaddr(fwmgr, SCORE_BARON1,
			&text_addr, &data_addr);
	if (ret)
		goto p_err;

	writel(text_addr, system->sfr + BR_CACHE_IC_BASE_ADDR);
	writel(data_addr, system->sfr + BR1_CACHE_DC_BASE_ADDR);

	ret = score_firmware_manager_get_dvaddr(fwmgr, SCORE_BARON2,
			&text_addr, &data_addr);
	if (ret)
		goto p_err;

	writel(data_addr, system->sfr + BR2_CACHE_DC_BASE_ADDR);

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
	writel(0x1, system->sfr + TS_WAKEUP);
	writel(0x1, system->sfr + BR1_WAKEUP);
	writel(0x1, system->sfr + BR2_WAKEUP);
	score_leave();
}

static void __score_system_init(struct score_system *system)
{
	score_enter();
	score_mmu_init(&system->mmu);
	score_scq_init(&system->scq);
	score_print_init(&system->print);

	writel(0x22fff, system->sfr + AXIM0_SIGNAL);
	writel(0x22fff, system->sfr + AXIM2_SIGNAL);
#if defined(SCORE_EVT0)
	writel(0x1, system->sfr + DSPM_ALWAYS_ON);
#endif

	writel(0x0, system->sfr + TS_INIT_DONE_CHECK);
	writel(0x0, system->sfr + BR1_INIT_DONE_CHECK);
	writel(0x0, system->sfr + BR2_INIT_DONE_CHECK);

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
	ret = score_profiler_open();
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

#if defined(SCORE_EVT0)
	score_system_halt(system);
#endif
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
