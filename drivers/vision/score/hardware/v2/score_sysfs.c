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
#include <linux/firmware.h>
#include <linux/delay.h>

#include "score_log.h"
#include "score_regs.h"
#include "score_firmware.h"
#include "score_dpmu.h"
#include "score_frame.h"
#include "score_mmu.h"
#include "score_sysfs.h"
#include "score_profiler.h"

static unsigned int request_fw_id;
static ssize_t show_sysfs_system_power(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	struct score_device *device;
	struct score_pm *pm;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	pm = &device->pm;
	if (score_pm_qos_active(pm)) {
		count += sprintf(buf + count, "on (count:%d, info:%#x/%u)\n",
				atomic_read(&device->open_count),
				device->state, device->cur_firmware_id);
	} else {
		count += sprintf(buf + count, "off (%u)\n", request_fw_id);
	}
	score_leave();
p_end:
	return count;
}

static ssize_t store_sysfs_system_power(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	struct score_device *device;
	struct score_core *core;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	core = &device->core;
	if (mutex_lock_interruptible(&core->device_lock))
		goto p_end;

	if (sysfs_streq(buf, "on")) {
		if (request_fw_id >= SCORE_FIRMWARE_END) {
			score_info("[sysfs] invalid firmware(%u)\n",
					request_fw_id);
			goto p_err;
		}
		score_info("[sysfs] power on(%u)\n", request_fw_id);
		score_device_get(device);
		ret = score_device_open(device);
		if (ret)
			goto p_err_put;

#if defined(CONFIG_PM_DEVFREQ)
		ret = score_device_start(device, request_fw_id,
				device->pm.default_qos, 0);
#else
		ret = score_device_start(device, request_fw_id, 0, 0);
#endif
		if (ret)
			goto p_err_put;
	} else if (sysfs_streq(buf, "off")) {
		score_info("[sysfs] power off\n");
		score_device_put(device);
	} else {
		unsigned int firmware_id;

		if (kstrtouint(buf, 10, &firmware_id)) {
			score_info("[sysfs] firmware_id setting failed\n");
			goto p_err;
		}
		request_fw_id = firmware_id % SCORE_FIRMWARE_END;
		score_info("[sysfs] firmware_id is set(%d)\n", request_fw_id);
	}
	mutex_unlock(&core->device_lock);
	score_leave();
p_end:
	return count;
p_err_put:
	score_device_put(device);
p_err:
	mutex_unlock(&core->device_lock);
	return count;
}

#if defined(CONFIG_PM_DEVFREQ)
static ssize_t show_sysfs_system_pm_qos(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	struct score_device *device;
	struct score_pm *pm;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	pm = &device->pm;
	if (pm->default_qos < 0)
		goto p_end;
	else if (pm->current_qos < 0)
		count += sprintf(buf + count, "-/L%d\n", pm->default_qos);
	else
		count += sprintf(buf + count, "L%d/L%d\n",
				pm->current_qos, pm->default_qos);
	score_leave();
p_end:
	return count;
}

static ssize_t store_sysfs_system_pm_qos(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct score_device *device;
	struct score_pm *pm;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	pm = &device->pm;
	if (sysfs_streq(buf, "L0")) {
		score_info("[sysfs] pm_qos_level L0\n");
		pm->default_qos = SCORE_PM_QOS_L0;
		score_pm_qos_update(pm, SCORE_PM_QOS_L0);
	} else if (sysfs_streq(buf, "L1")) {
		score_info("[sysfs] pm_qos_level L1\n");
		pm->default_qos = SCORE_PM_QOS_L1;
		score_pm_qos_update(pm, SCORE_PM_QOS_L1);
	} else if (sysfs_streq(buf, "L2")) {
		score_info("[sysfs] pm_qos_level L2\n");
		pm->default_qos = SCORE_PM_QOS_L2;
		score_pm_qos_update(pm, SCORE_PM_QOS_L2);
	} else if (sysfs_streq(buf, "L3")) {
		score_info("[sysfs] pm_qos_level L3\n");
		pm->default_qos = SCORE_PM_QOS_L3;
		score_pm_qos_update(pm, SCORE_PM_QOS_L3);
	} else if (sysfs_streq(buf, "L4")) {
		score_info("[sysfs] pm_qos_level L4\n");
		pm->default_qos = SCORE_PM_QOS_L4;
		score_pm_qos_update(pm, SCORE_PM_QOS_L4);
	} else {
		score_info("[sysfs] pm_qos invalid level\n");
		goto p_end;
	}

	score_leave();
p_end:
	return count;
}
#endif

#if defined(CONFIG_PM_SLEEP)
static ssize_t show_sysfs_system_pm_sleep(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;

	score_enter();
	count += sprintf(buf + count, "%s %s\n",
			"suspend", "resume");
	score_leave();
	return count;
}

static ssize_t store_sysfs_system_pm_sleep(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct score_device *device;
	const struct dev_pm_ops *pm_ops;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	pm_ops = dev->driver->pm;
	if (!pm_ops)
		goto p_end;

	if (sysfs_streq(buf, "suspend")) {
		if (pm_ops->suspend) {
			score_info("[sysfs] suspend\n");
			pm_ops->suspend(dev);
		}
	} else if (sysfs_streq(buf, "resume")) {
		if (pm_ops->resume) {
			score_info("[sysfs] resume\n");
			pm_ops->resume(dev);
		}
	}

	score_leave();
p_end:
	return count;
}
#endif

static ssize_t show_sysfs_system_clk(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	struct score_device *device;
	struct score_pm *pm;
	struct score_clk *clk;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	pm = &device->pm;
	clk = &device->clk;

	mutex_lock(&pm->lock);
	if (score_pm_qos_active(pm)) {
		count += sprintf(buf + count, "clkm %lu / ",
				score_clk_get(clk, SCORE_MASTER));
		count += sprintf(buf + count, "clks %lu\n",
				score_clk_get(clk, SCORE_KNIGHT1));
	} else {
		count += sprintf(buf + count, "clk 0\n");
	}
	mutex_unlock(&pm->lock);
	score_leave();
p_end:
	return count;
}

static ssize_t store_sysfs_system_clk(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	score_enter();
	score_leave();
	return count;
}

static ssize_t show_sysfs_system_print(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	struct score_device *device;
	struct score_print *print;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	print = &device->system.print;
	if (print->timer_en == SCORE_PRINT_TIMER_DISABLE)
		count += sprintf(buf + count, "timer_off, ");
	else
		count += sprintf(buf + count, "timer_on, ");

	if (!print->init) {
		count += sprintf(buf + count, "not inited\n");
		goto p_end;
	}

	if (score_print_buf_empty(print))
		count += sprintf(buf + count, "emptry\n");
	else if (score_print_buf_full(print))
		count += sprintf(buf + count, "full\n");
	else
		count += sprintf(buf + count, "exist\n");
	score_leave();
p_end:
	return count;
}

static ssize_t store_sysfs_system_print(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct score_device *device;
	struct score_print *print;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	print = &device->system.print;
	if (sysfs_streq(buf, "flush")) {
		if (print->init) {
			score_info("[sysfs] print flush\n");
			score_print_flush(print);
		} else {
			score_info("[sysfs] print is not initialized\n");
		}
	} else if (sysfs_streq(buf, "timer_on")) {
		if (print->init) {
			score_info("[sysfs] print is already initialized\n");
		} else if (print->timer_en == SCORE_PRINT_TIMER_DISABLE) {
			score_info("[sysfs] print timer_on\n");
			print->timer_en = SCORE_PRINT_TIMER_ENABLE;
		} else {
			score_info("[sysfs] print timer status %d\n",
					print->timer_en);
		}
	} else if (sysfs_streq(buf, "timer_off")) {
		if (print->init) {
			score_info("[sysfs] print is already initialized\n");
		} else if (print->timer_en == SCORE_PRINT_TIMER_ENABLE) {
			score_info("[sysfs] print timer_off\n");
			print->timer_en = SCORE_PRINT_TIMER_DISABLE;
		} else {
			score_info("[sysfs] print timer status %d\n",
					print->timer_en);
		}
	} else {
		score_info("[sysfs] print timer invalid command\n");
	}
	score_leave();
p_end:
	return count;
}

static ssize_t show_sysfs_system_sfr(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	struct score_device *device;
	struct score_pm *pm;
	struct score_system *system;
	int idx;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	pm = &device->pm;
	system = &device->system;

	mutex_lock(&pm->lock);
	if (!score_pm_qos_active(pm)) {
		count += sprintf(buf + count, "power off\n");
		goto p_unlock;
	}

	count += sprintf(buf + count, "%32s: %8X\n",
			"SW_RESET",
			readl(system->sfr + SW_RESET));
	count += sprintf(buf + count, "%32s: %8X\n",
			"RELEASE_DATE",
			readl(system->sfr + RELEASE_DATE));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"AXIM0_CACHE(r/w)",
			readl(system->sfr + AXIM0_ARCACHE),
			readl(system->sfr + AXIM0_AWCACHE));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"AXIM2_CACHE(r/w)",
			readl(system->sfr + AXIM2_ARCACHE),
			readl(system->sfr + AXIM2_AWCACHE));
	count += sprintf(buf + count, "%32s: %8X %8X %8X\n",
			"WAKEUP(MC/KC1/KC2)",
			readl(system->sfr + MC_WAKEUP),
			readl(system->sfr + KC1_WAKEUP),
			readl(system->sfr + KC2_WAKEUP));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"SM_MCGEN/CLK_REQ",
			readl(system->sfr + SM_MCGEN),
			readl(system->sfr + CLK_REQ));
	count += sprintf(buf + count, "%32s: %8X\n",
			"APCPU_INTR_ENABLE",
			readl(system->sfr + APCPU_INTR_ENABLE));
	count += sprintf(buf + count, "%32s: %8X %8X %8X\n",
			"INTR_ENABLE(MC/KC1/KC2)",
			readl(system->sfr + MC_INTR_ENABLE),
			readl(system->sfr + KC1_INTR_ENABLE),
			readl(system->sfr + KC2_INTR_ENABLE));
	count += sprintf(buf + count, "%32s: %8X\n",
			"IVA_INTR_ENABLE",
			readl(system->sfr + IVA_INTR_ENABLE));
	count += sprintf(buf + count, "%32s: %8X\n",
			"APCPU_SWI_STATUS",
			readl(system->sfr + APCPU_SWI_STATUS));
	count += sprintf(buf + count, "%32s: %8X %8X %8X\n",
			"SWI_STATUS(MC/KC1/KC2)",
			readl(system->sfr + MC_SWI_STATUS),
			readl(system->sfr + KC1_SWI_STATUS),
			readl(system->sfr + KC2_SWI_STATUS));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"APCPU_INTR_STATUS(raw/masked)",
			readl(system->sfr + APCPU_RAW_INTR_STATUS),
			readl(system->sfr + APCPU_MASKED_INTR_STATUS));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"MC_CACHE_BASEADDR(IC/DC)",
			readl(system->sfr + MC_CACHE_IC_BASEADDR),
			readl(system->sfr + MC_CACHE_DC_BASEADDR));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"KC1_CACHE_BASEADDR(IC/DC)",
			readl(system->sfr + KC1_CACHE_IC_BASEADDR),
			readl(system->sfr + KC1_CACHE_DC_BASEADDR));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"KC2_CACHE_BASEADDR(IC/DC)",
			readl(system->sfr + KC2_CACHE_IC_BASEADDR),
			readl(system->sfr + KC2_CACHE_DC_BASEADDR));
	count += sprintf(buf + count, "%32s: %8X %8X %8X\n",
			"DC_CCTRL_STATUS(MC/KC1/KC2)",
			readl(system->sfr + MC_CACHE_DC_CCTRL_STATUS),
			readl(system->sfr + KC1_CACHE_DC_CCTRL_STATUS),
			readl(system->sfr + KC2_CACHE_DC_CCTRL_STATUS));
	count += sprintf(buf + count, "%32s: %8X\n",
			"SCQ_INTR_ENABLE",
			readl(system->sfr + SCQ_INTR_ENABLE));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"SCQ_INTR_STATUS(raw/masked)",
			readl(system->sfr + SCQ_RAW_INTR_STATUS),
			readl(system->sfr + SCQ_MASKED_INTR_STATUS));
#ifdef SCORE_EVT1
	count += sprintf(buf + count, "%32s: %8X %8X %8X %8X %8X\n",
			"PACKET_NUM(AP/MC/KC1/KC2/IVA)",
			readl(system->sfr + SCQ_APCPU_PACKET_NUM),
			readl(system->sfr + SCQ_MC_PACKET_NUM),
			readl(system->sfr + SCQ_KC1_PACKET_NUM),
			readl(system->sfr + SCQ_KC2_PACKET_NUM),
			readl(system->sfr + SCQ_IVA_PACKET_NUM));
#endif
	count += sprintf(buf + count, "%32s: %8X %8X %8X\n",
			"PRINT(front/rear/count)",
			readl(system->sfr + PRINT_QUEUE_FRONT_ADDR),
			readl(system->sfr + PRINT_QUEUE_REAR_ADDR),
			readl(system->sfr + PRINT_COUNT));
	count += sprintf(buf + count, "%32s: %8X %8X %8X\n",
			"DONE_CHECK(MC/KC1/KC2)",
			readl(system->sfr + MC_INIT_DONE_CHECK),
			readl(system->sfr + KC1_INIT_DONE_CHECK),
			readl(system->sfr + KC2_INIT_DONE_CHECK));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"MC_MALLOC(addr/size)",
			readl(system->sfr + MC_MALLOC_BASE_ADDR),
			readl(system->sfr + MC_MALLOC_SIZE));
	count += sprintf(buf + count, "%32s: %8X %8X %8X\n",
			"KC_MALLOC(kc1/kc2/size)",
			readl(system->sfr + KC1_MALLOC_BASE_ADDR),
			readl(system->sfr + KC2_MALLOC_BASE_ADDR),
			readl(system->sfr + KC_MALLOC_SIZE));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"PRINT(addr/size)",
			readl(system->sfr + PRINT_BASE_ADDR),
			readl(system->sfr + PRINT_SIZE));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"PROFILER(mc/mc_size)",
			readl(system->sfr + PROFILER_MC_BASE_ADDR),
			readl(system->sfr + PROFILER_MC_SIZE));
	count += sprintf(buf + count, "%32s: %8X %8X %8X\n",
			"PROFILER(kc1/kc2/kc_size)",
			readl(system->sfr + PROFILER_KC1_BASE_ADDR),
			readl(system->sfr + PROFILER_KC2_BASE_ADDR),
			readl(system->sfr + PROFILER_KC_SIZE));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"PROFILER MC(head/tail)",
			readl(system->sfr + PROFILER_MC_HEAD),
			readl(system->sfr + PROFILER_MC_TAIL));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"PROFILER KC1(head/tail)",
			readl(system->sfr + PROFILER_KC1_HEAD),
			readl(system->sfr + PROFILER_KC1_TAIL));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"PROFILER KC2(head/tail)",
			readl(system->sfr + PROFILER_KC2_HEAD),
			readl(system->sfr + PROFILER_KC2_TAIL));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"PROLOGUE_DONE(KC1/KC2)",
			readl(system->sfr + KC1_PROLOGUE_DONE),
			readl(system->sfr + KC2_PROLOGUE_DONE));
	count += sprintf(buf + count, "%32s: %8X\n",
			"TOTAL_TILE_COUNT",
			readl(system->sfr + TOTAL_TILE_COUNT));
	count += sprintf(buf + count, "%32s: %8X\n",
			"TOTAL_TCM_SIZE",
			readl(system->sfr + TOTAL_TCM_SIZE));
	count += sprintf(buf + count, "%32s: %8X\n",
			"ENABLE_TCM_HALF",
			readl(system->sfr + ENABLE_TCM_HALF));
	count += sprintf(buf + count, "%32s: %8X %8X %8X %8X %8X\n",
			"MC_ABORT_STATE(0-4)",
			readl(system->sfr + MC_ABORT_STATE0),
			readl(system->sfr + MC_ABORT_STATE1),
			readl(system->sfr + MC_ABORT_STATE2),
			readl(system->sfr + MC_ABORT_STATE3),
			readl(system->sfr + MC_ABORT_STATE4));
	count += sprintf(buf + count, "%32s: %8X\n",
			"SCHEDULE_STATE",
			readl(system->sfr + SCHEDULE_STATE));
	count += sprintf(buf + count, "%32s: %8X\n",
			"CORE_STATE",
			readl(system->sfr + CORE_STATE));
	count += sprintf(buf + count, "%32s: %8X\n",
			"PRIORITY_STATE",
			readl(system->sfr + PRIORITY_STATE));
	count += sprintf(buf + count, "%32s: %8X\n",
			"MC_KERNEL_ID",
			readl(system->sfr + MC_KERNEL_ID));
	count += sprintf(buf + count, "%32s: %8X\n",
			"MC_ENTRY_FLAG",
			readl(system->sfr + MC_ENTRY_FLAG));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"INDIRECT(FUNC/ADDR)",
			readl(system->sfr + INDIRECT_FUNC),
			readl(system->sfr + INDIRECT_ADDR));
	count += sprintf(buf + count, "%32s: %8X\n",
			"MC_REQUEST_TYPE",
			readl(system->sfr + MC_REQUEST_TYPE));
	count += sprintf(buf + count, "%32s: %8X\n",
			"MC_TCM_MALLOC_STATE",
			readl(system->sfr + MC_TCM_MALLOC_STATE));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"CURRENT_KERNEL(KC1/KC2)",
			readl(system->sfr + KC1_CURRENT_KERNEL),
			readl(system->sfr + KC2_CURRENT_KERNEL));
	for (idx = 0; idx < 32; idx += 4)
		count += sprintf(buf + count,
				"%26s %02d-%02d: %8X %8X %8X %8X\n",
				"USERDEFINED", idx, idx + 3,
				readl(system->sfr + USERDEFINED(idx)),
				readl(system->sfr + USERDEFINED(idx + 1)),
				readl(system->sfr + USERDEFINED(idx + 2)),
				readl(system->sfr + USERDEFINED(idx + 3)));

p_unlock:
	mutex_unlock(&pm->lock);
	score_leave();
p_end:
	return count;
}

static ssize_t store_sysfs_system_sfr(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	score_enter();
	score_leave();
	return count;
}

static ssize_t show_sysfs_system_dpmu(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	struct score_device *device;
	struct score_pm *pm;
	struct score_system *system;
	int idx;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	pm = &device->pm;
	mutex_lock(&pm->lock);
	if (score_dpmu_debug_enable_check())
		count += sprintf(buf + count, "< debug enable ");
	else
		count += sprintf(buf + count, "< debug disable ");
	if (score_dpmu_performance_enable_check())
		count += sprintf(buf + count, "/ performance enable ");
	else
		count += sprintf(buf + count, "/ performance disable ");
	if (score_dpmu_stall_enable_check())
		count += sprintf(buf + count, "/ stall enable ");
	else
		count += sprintf(buf + count, "/ stall disable ");

	if (!score_pm_qos_active(pm)) {
		count += sprintf(buf + count, "/ power off >\n");
		goto p_unlock;
	}

	system = &device->system;
	writel(0x1, system->sfr + CPUID_OF_CCH);
	count += sprintf(buf + count, ">\n%32s: %8X\n",
			"DBG_INTR_ENABLE",
			readl(system->sfr + DBG_INTR_ENABLE));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"DBG_INTR_STATUS(raw/masked)",
			readl(system->sfr + DBG_RAW_INTR_STATUS),
			readl(system->sfr + DBG_MASKED_INTR_STATUS));
	writel(0x0, system->sfr + CPUID_OF_CCH);
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"MONITOR_ENABLE(dbg/perf)",
			readl(system->sfr + DBG_MONITOR_ENABLE),
			readl(system->sfr + PERF_MONITOR_ENABLE));
	count += sprintf(buf + count, "%32s: %8X\n",
			"SCQ_SLVERR_INFO",
			readl(system->sfr + SCQ_SLVERR_INFO));
	count += sprintf(buf + count, "%32s: %8X\n",
			"TCMBUS_SLVERR_INFO",
			readl(system->sfr + TCMBUS_SLVERR_INFO));
	count += sprintf(buf + count, "%32s: %8X\n",
			"CACHE_SLVERR_INFO",
			readl(system->sfr + CACHE_SLVERR_INFO));
	count += sprintf(buf + count, "%32s: %8X\n",
			"IBC_SLVERR_INFO",
			readl(system->sfr + IBC_SLVERR_INFO));
	count += sprintf(buf + count, "%32s: %8X\n",
			"AXI_SLVERR_INFO",
			readl(system->sfr + AXI_SLVERR_INFO));
	count += sprintf(buf + count, "%32s: %8X\n",
			"DSP_SLVERR_INFO",
			readl(system->sfr + DSP_SLVERR_INFO));
	count += sprintf(buf + count, "%32s: %8X %8X %8X\n",
			"DECERR_INFO(MC/KC1/KC2)",
			readl(system->sfr + MC_DECERR_INFO),
			readl(system->sfr + KC1_DECERR_INFO),
			readl(system->sfr + KC2_DECERR_INFO));

	for (idx = 0; idx < 7; ++idx)
		count += sprintf(buf + count, "%32s: %8X %8X %8X\n",
				"PROG_COUNTER(MC/KC1/KC2)",
				readl(system->sfr + MC_PROG_COUNTER),
				readl(system->sfr + KC1_PROG_COUNTER),
				readl(system->sfr + KC2_PROG_COUNTER));

	if (score_dpmu_performance_enable_check()) {
		count += sprintf(buf + count, "%32s: %8X %8X\n",
				"MC_ICACHE_ACC_COUNT(H/L)",
				readl(system->sfr + MC_ICACHE_ACC_COUNT_HIGH),
				readl(system->sfr + MC_ICACHE_ACC_COUNT_LOW));
		count += sprintf(buf + count, "%32s: %8X\n",
				"MC_ICACHE_MISS_COUNT",
				readl(system->sfr + MC_ICACHE_MISS_COUNT));
		count += sprintf(buf + count, "%32s: %8X %8X\n",
				"MC_DCACHE_COUNT(acc/miss)",
				readl(system->sfr + MC_DCACHE_ACC_COUNT),
				readl(system->sfr + MC_DCACHE_MISS_COUNT));
		count += sprintf(buf + count, "%32s: %8X %8X\n",
				"KC1_ICACHE_ACC_COUNT(H/L)",
				readl(system->sfr + KC1_ICACHE_ACC_COUNT_HIGH),
				readl(system->sfr + KC1_ICACHE_ACC_COUNT_LOW));
		count += sprintf(buf + count, "%32s: %8X\n",
				"KC1_ICACHE_MISS_COUNT",
				readl(system->sfr + KC1_ICACHE_MISS_COUNT));
		count += sprintf(buf + count, "%32s: %8X %8X\n",
				"KC1_DCACHE_COUNT(acc/miss)",
				readl(system->sfr + KC1_DCACHE_ACC_COUNT),
				readl(system->sfr + KC1_DCACHE_MISS_COUNT));
		count += sprintf(buf + count, "%32s: %8X %8X\n",
				"KC2_ICACHE_ACC_COUNT(H/L)",
				readl(system->sfr + KC2_ICACHE_ACC_COUNT_HIGH),
				readl(system->sfr + KC2_ICACHE_ACC_COUNT_LOW));
		count += sprintf(buf + count, "%32s: %8X\n",
				"KC2_ICACHE_MISS_COUNT",
				readl(system->sfr + KC2_ICACHE_MISS_COUNT));
		count += sprintf(buf + count, "%32s: %8X %8X\n",
				"KC2_DCACHE_COUNT(acc/miss)",
				readl(system->sfr + KC2_DCACHE_ACC_COUNT),
				readl(system->sfr + KC2_DCACHE_MISS_COUNT));
		count += sprintf(buf + count, "%32s: %8X %8X %8X %8X %8X\n",
				"RDBEAT_COUNT(m0/m1/m2/m3/s2)",
				readl(system->sfr + AXIM0_RDBEAT_COUNT),
				readl(system->sfr + AXIM1_RDBEAT_COUNT),
				readl(system->sfr + AXIM2_RDBEAT_COUNT),
				readl(system->sfr + AXIM3_RDBEAT_COUNT),
				readl(system->sfr + AXIS2_RDBEAT_COUNT));
		count += sprintf(buf + count, "%32s: %8X %8X %8X %8X %8X\n",
				"WRBEAT_COUNT(m0/m1/m2/m3/s2)",
				readl(system->sfr + AXIM0_WRBEAT_COUNT),
				readl(system->sfr + AXIM1_WRBEAT_COUNT),
				readl(system->sfr + AXIM2_WRBEAT_COUNT),
				readl(system->sfr + AXIM3_WRBEAT_COUNT),
				readl(system->sfr + AXIS2_WRBEAT_COUNT));
		count += sprintf(buf + count, "%32s: %8X %8X %8X %8X %8X\n",
				"RD_MAX_MO_COUNT(m0/m1/m2/m3/s2)",
				readl(system->sfr + AXIM0_RD_MAX_MO_COUNT),
				readl(system->sfr + AXIM1_RD_MAX_MO_COUNT),
				readl(system->sfr + AXIM2_RD_MAX_MO_COUNT),
				readl(system->sfr + AXIM3_RD_MAX_MO_COUNT),
				readl(system->sfr + AXIS2_RD_MAX_MO_COUNT));
		count += sprintf(buf + count, "%32s: %8X %8X %8X %8X %8X\n",
				"WR_MAX_MO_COUNT(m0/m1/m2/m3/s2)",
				readl(system->sfr + AXIM0_WR_MAX_MO_COUNT),
				readl(system->sfr + AXIM1_WR_MAX_MO_COUNT),
				readl(system->sfr + AXIM2_WR_MAX_MO_COUNT),
				readl(system->sfr + AXIM3_WR_MAX_MO_COUNT),
				readl(system->sfr + AXIS2_WR_MAX_MO_COUNT));
		count += sprintf(buf + count, "%32s: %8X %8X %8X %8X %8X\n",
				"RD_MAX_REQ_LAT(m0/m1/m2/m3/s2)",
				readl(system->sfr + AXIM0_RD_MAX_REQ_LAT),
				readl(system->sfr + AXIM1_RD_MAX_REQ_LAT),
				readl(system->sfr + AXIM2_RD_MAX_REQ_LAT),
				readl(system->sfr + AXIM3_RD_MAX_REQ_LAT),
				readl(system->sfr + AXIS2_RD_MAX_REQ_LAT));
		count += sprintf(buf + count, "%32s: %8X %8X %8X %8X %8X\n",
				"WR_MAX_REQ_LAT(m0/m1/m2/m3/s2)",
				readl(system->sfr + AXIM0_WR_MAX_REQ_LAT),
				readl(system->sfr + AXIM1_WR_MAX_REQ_LAT),
				readl(system->sfr + AXIM2_WR_MAX_REQ_LAT),
				readl(system->sfr + AXIM3_WR_MAX_REQ_LAT),
				readl(system->sfr + AXIS2_WR_MAX_REQ_LAT));
		count += sprintf(buf + count, "%32s: %8X %8X %8X %8X %8X\n",
				"RD_MAX_AR2R_LAT(m0/m1/m2/m3/s2)",
				readl(system->sfr + AXIM0_RD_MAX_AR2R_LAT),
				readl(system->sfr + AXIM1_RD_MAX_AR2R_LAT),
				readl(system->sfr + AXIM2_RD_MAX_AR2R_LAT),
				readl(system->sfr + AXIM3_RD_MAX_AR2R_LAT),
				readl(system->sfr + AXIS2_RD_MAX_AR2R_LAT));
		count += sprintf(buf + count, "%32s: %8X %8X %8X %8X %8X\n",
				"WR_MAX_R2R_LAT(m0/m1/m2/m3/s2)",
				readl(system->sfr + AXIM0_WR_MAX_R2R_LAT),
				readl(system->sfr + AXIM1_WR_MAX_R2R_LAT),
				readl(system->sfr + AXIM2_WR_MAX_R2R_LAT),
				readl(system->sfr + AXIM3_WR_MAX_R2R_LAT),
				readl(system->sfr + AXIS2_WR_MAX_R2R_LAT));

		count += sprintf(buf + count, "%32s: %8X %8X %8X %8X\n",
				"DMA0_EXEC_CYCLE 00~03",
				readl(system->sfr + DMA0_EXEC_CYCLE0),
				readl(system->sfr + DMA0_EXEC_CYCLE1),
				readl(system->sfr + DMA0_EXEC_CYCLE2),
				readl(system->sfr + DMA0_EXEC_CYCLE3));
		count += sprintf(buf + count, "%32s: %8X %8X %8X %8X\n",
				"DMA0_EXEC_CYCLE 04~07",
				readl(system->sfr + DMA0_EXEC_CYCLE4),
				readl(system->sfr + DMA0_EXEC_CYCLE5),
				readl(system->sfr + DMA0_EXEC_CYCLE6),
				readl(system->sfr + DMA0_EXEC_CYCLE7));
		count += sprintf(buf + count, "%32s: %8X %8X %8X %8X\n",
				"DMA0_EXEC_CYCLE 08~11",
				readl(system->sfr + DMA0_EXEC_CYCLE8),
				readl(system->sfr + DMA0_EXEC_CYCLE9),
				readl(system->sfr + DMA0_EXEC_CYCLE10),
				readl(system->sfr + DMA0_EXEC_CYCLE11));
		count += sprintf(buf + count, "%32s: %8X %8X %8X %8X\n",
				"DMA0_EXEC_CYCLE 12~15",
				readl(system->sfr + DMA0_EXEC_CYCLE12),
				readl(system->sfr + DMA0_EXEC_CYCLE13),
				readl(system->sfr + DMA0_EXEC_CYCLE14),
				readl(system->sfr + DMA0_EXEC_CYCLE15));
		count += sprintf(buf + count, "%32s: %8X %8X %8X %8X\n",
				"DMA1_EXEC_CYCLE 00~03",
				readl(system->sfr + DMA1_EXEC_CYCLE0),
				readl(system->sfr + DMA1_EXEC_CYCLE1),
				readl(system->sfr + DMA1_EXEC_CYCLE2),
				readl(system->sfr + DMA1_EXEC_CYCLE3));
		count += sprintf(buf + count, "%32s: %8X %8X %8X %8X\n",
				"DMA1_EXEC_CYCLE 04~07",
				readl(system->sfr + DMA1_EXEC_CYCLE4),
				readl(system->sfr + DMA1_EXEC_CYCLE5),
				readl(system->sfr + DMA1_EXEC_CYCLE6),
				readl(system->sfr + DMA1_EXEC_CYCLE7));
		count += sprintf(buf + count, "%32s: %8X %8X %8X %8X\n",
				"DMA1_EXEC_CYCLE 08~11",
				readl(system->sfr + DMA1_EXEC_CYCLE8),
				readl(system->sfr + DMA1_EXEC_CYCLE9),
				readl(system->sfr + DMA1_EXEC_CYCLE10),
				readl(system->sfr + DMA1_EXEC_CYCLE11));
		count += sprintf(buf + count, "%32s: %8X %8X %8X %8X\n",
				"DMA1_EXEC_CYCLE 12~15",
				readl(system->sfr + DMA1_EXEC_CYCLE12),
				readl(system->sfr + DMA1_EXEC_CYCLE13),
				readl(system->sfr + DMA1_EXEC_CYCLE14),
				readl(system->sfr + DMA1_EXEC_CYCLE15));
		count += sprintf(buf + count, "%32s: %8X %8X\n",
				"MC_INST_COUNT(H/L)",
				readl(system->sfr + MC_INST_COUNT_HIGH),
				readl(system->sfr + MC_INST_COUNT_LOW));
		count += sprintf(buf + count, "%32s: %8X %8X\n",
				"KC1_INST_COUNT(H/L)",
				readl(system->sfr + KC1_INST_COUNT_HIGH),
				readl(system->sfr + KC1_INST_COUNT_LOW));
		count += sprintf(buf + count, "%32s: %8X %8X\n",
				"KC2_INST_COUNT(H/L)",
				readl(system->sfr + KC2_INST_COUNT_HIGH),
				readl(system->sfr + KC2_INST_COUNT_LOW));
		count += sprintf(buf + count, "%32s: %8X %8X\n",
				"MC_INTR_COUNT(H/L)",
				readl(system->sfr + MC_INTR_COUNT_HIGH),
				readl(system->sfr + MC_INTR_COUNT_LOW));
		count += sprintf(buf + count, "%32s: %8X %8X\n",
				"KC1_INTR_COUNT(H/L)",
				readl(system->sfr + KC1_INTR_COUNT_HIGH),
				readl(system->sfr + KC1_INTR_COUNT_LOW));
		count += sprintf(buf + count, "%32s: %8X %8X\n",
				"KC2_INTR_COUNT(H/L)",
				readl(system->sfr + KC2_INTR_COUNT_HIGH),
				readl(system->sfr + KC2_INTR_COUNT_LOW));
	}
#ifdef SCORE_EVT1
	if (!score_dpmu_stall_enable_check())
		goto p_unlock;

	count += sprintf(buf + count, "%32s: %8X %8X %8X\n",
			"STALL ENABLE(MC/KC1/KC2)",
			readl(system->sfr + MC_STALL_COUNT_ENABLE),
			readl(system->sfr + KC1_STALL_COUNT_ENABLE),
			readl(system->sfr + KC2_STALL_COUNT_ENABLE));
	count += sprintf(buf + count, "%32s: %8X %8X\n",
			"SFR STALL ENABLE(KC1/KC2)",
			readl(system->sfr + KC1_SFR_STALL_COUNT_ENABLE),
			readl(system->sfr + KC2_SFR_STALL_COUNT_ENABLE));
	count += sprintf(buf + count, "%32s: %8X %8X %8X %8X %8X\n",
			"MC_STALL(PROC/IC/DC/TCM/SFR)",
			readl(system->sfr + MC_PROC_STALL_COUNT),
			readl(system->sfr + MC_IC_STALL_COUNT),
			readl(system->sfr + MC_DC_STALL_COUNT),
			readl(system->sfr + MC_TCM_STALL_COUNT),
			readl(system->sfr + MC_SFR_STALL_COUNT));
	count += sprintf(buf + count, "%32s: %8X %8X %8X %8X %8X\n",
			"KC1_STALL(PROC/IC/DC/TCM/SFR)",
			readl(system->sfr + KC1_PROC_STALL_COUNT),
			readl(system->sfr + KC1_IC_STALL_COUNT),
			readl(system->sfr + KC1_DC_STALL_COUNT),
			readl(system->sfr + KC1_TCM_STALL_COUNT),
			readl(system->sfr + KC1_SFR_STALL_COUNT));
	count += sprintf(buf + count, "%32s: %8X %8X %8X %8X %8s\n",
			"KC2_STALL(PROC/IC/DC/TCM/SFR)",
			readl(system->sfr + KC2_PROC_STALL_COUNT),
			readl(system->sfr + KC2_IC_STALL_COUNT),
			readl(system->sfr + KC2_DC_STALL_COUNT),
			readl(system->sfr + KC2_TCM_STALL_COUNT),
			"NOT USE");
#endif

p_unlock:
	mutex_unlock(&pm->lock);
	score_leave();
p_end:
	return count;
}

static ssize_t store_sysfs_system_dpmu(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct score_device *device;
	struct score_pm *pm;
	struct score_system *system;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	pm = &device->pm;
	mutex_lock(&pm->lock);
	if (score_pm_qos_active(pm)) {
		score_info("[sysfs] power is already on\n");
		goto p_unlock;
	}

	system = &device->system;
	if (sysfs_streq(buf, "on")) {
		score_info("[sysfs] dpmu/stall on\n");
		score_dpmu_debug_enable_on(1);
		score_dpmu_performance_enable_on(1);
		score_dpmu_stall_enable_on(1);
	} else if (sysfs_streq(buf, "off")) {
		score_info("[sysfs] dpmu/stall off\n");
		score_dpmu_debug_enable_on(0);
		score_dpmu_performance_enable_on(0);
		score_dpmu_stall_enable_on(0);
	} else if (sysfs_streq(buf, "debug_on")) {
		score_info("[sysfs] debug on\n");
		score_dpmu_debug_enable_on(1);
	} else if (sysfs_streq(buf, "debug_off")) {
		score_info("[sysfs] debug off\n");
		score_dpmu_debug_enable_on(0);
	} else if (sysfs_streq(buf, "performance_on")) {
		score_info("[sysfs] performance/stall on\n");
		score_dpmu_performance_enable_on(1);
		score_dpmu_stall_enable_on(1);
	} else if (sysfs_streq(buf, "performance_off")) {
		score_info("[sysfs] performance/stall off\n");
		score_dpmu_performance_enable_on(0);
		score_dpmu_stall_enable_on(0);
	} else if (sysfs_streq(buf, "stall_on")) {
		score_info("[sysfs] stall on\n");
		score_dpmu_stall_enable_on(1);
	} else if (sysfs_streq(buf, "stall_off")) {
		score_info("[sysfs] stall off\n");
		score_dpmu_stall_enable_on(0);
	} else {
		score_info("[sysfs] dpmu invalid command\n");
	}
p_unlock:
	mutex_unlock(&pm->lock);
	score_leave();
p_end:
	return count;
}

static ssize_t show_sysfs_system_scq(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	struct score_device *device;
	struct score_scq *scq;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	scq = &device->system.scq;
	count += sprintf(buf + count, "%d/%d\n",
			scq->count, MAX_HOST_TASK_COUNT);
	score_leave();
p_end:
	return count;
}

static ssize_t store_sysfs_system_scq(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	score_enter();
	score_leave();
	return count;
}

static ssize_t show_sysfs_system_frame_count(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	struct score_device *device;
	struct score_frame_manager *framemgr;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	framemgr = &device->system.interface.framemgr;
	count += sprintf(buf + count, "all %d / normal %d / abnormal %d\n",
			framemgr->all_count, framemgr->normal_count,
			framemgr->abnormal_count);
	score_leave();
p_end:
	return count;
}

static ssize_t store_sysfs_system_frame_count(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct score_device *device;
	struct score_frame_manager *framemgr;
	unsigned long flags;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	framemgr = &device->system.interface.framemgr;
	if (sysfs_streq(buf, "reset")) {
		score_info("[sysfs] frame_count reset\n");
		spin_lock_irqsave(&framemgr->slock, flags);
		framemgr->all_count = 0;
		framemgr->normal_count = 0;
		framemgr->abnormal_count = 0;
		spin_unlock_irqrestore(&framemgr->slock, flags);
	} else {
		score_info("[sysfs] frame_count invalid command\n");
	}
	score_leave();
p_end:
	return count;
}

static ssize_t show_sysfs_system_wait_time(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	struct score_device *device;
	struct score_core *core;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	core = &device->core;
	count += sprintf(buf + count, "wait_time %d ms\n", core->wait_time);
	score_leave();
p_end:
	return count;
}

static ssize_t store_sysfs_system_wait_time(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct score_device *device;
	struct score_core *core;
	unsigned int time;
	unsigned int max_time = 120000;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	core = &device->core;
	if (kstrtouint(buf, 10, &time)) {
		score_info("[sysfs] wait_time converting failed\n");
		goto p_end;
	}

	if (time == 0) {
		score_info("[sysfs] wait_time shouldn't be zero\n");
	} else if (time > max_time) {
		score_info("[sysfs] Max of wait_time is %u ms\n", max_time);
		core->wait_time = max_time;
	} else {
		score_info("[sysfs] wait_time is changed (%u ms -> %u ms)\n",
				core->wait_time, time);
		core->wait_time = time;
	}
	score_leave();
p_end:
	return count;
}

static ssize_t show_sysfs_system_dpmu_print(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	struct score_device *device;
	struct score_core *core;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	core = &device->core;
	if (core->dpmu_print)
		count += sprintf(buf + count, "dpmu_print on\n");
	else
		count += sprintf(buf + count, "dpmu_print off\n");
	score_leave();
p_end:
	return count;
}

static ssize_t store_sysfs_system_dpmu_print(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct score_device *device;
	struct score_core *core;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	core = &device->core;
	if (sysfs_streq(buf, "on")) {
		score_info("[sysfs] enable dpmu_print\n");
		core->dpmu_print = true;
	} else if (sysfs_streq(buf, "off")) {
		score_info("[sysfs] disable dpmu_print\n");
		core->dpmu_print = false;
	} else {
		score_info("[sysfs] dpmu_print invalid command\n");
	}
	score_leave();
p_end:
	return count;
}

static ssize_t show_sysfs_system_test(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;

	score_enter();
	count += sprintf(buf + count, "%s %s\n",
			"shutdown", "timeout");
	score_leave();
	return count;
}

static void __score_sysfs_timeout_frame(struct score_frame_manager *framemgr)
{
	struct score_frame *frame, *tframe;
	unsigned long flags;
	unsigned int id, state;
	bool timeout = false;

	score_enter();
	spin_lock_irqsave(&framemgr->slock, flags);
	list_for_each_entry_safe(frame, tframe, &framemgr->entire_list,
			entire_list) {
		if (frame->state == SCORE_FRAME_STATE_PROCESS) {
			id = frame->frame_id;
			state = frame->state;
			score_util_bitmap_clear_bit(framemgr->frame_map, id);
			frame->frame_id = SCORE_MAX_FRAME;
			timeout = true;
			break;
		}
	}

	if (timeout)
		score_info("[sysfs] Timeout will occur (frame %d/%d)\n",
				id, state);
	else
		score_info("[sysfs] Timeout frame is none\n");

	spin_unlock_irqrestore(&framemgr->slock, flags);
}

static ssize_t store_sysfs_system_test(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct score_device *device;
	struct platform_device *pdev;
	struct platform_driver *pdrv;
	struct score_pm *pm;
	struct score_system *system;
	struct score_frame_manager *framemgr;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	pdev = device->pdev;
	pdrv = to_platform_driver(dev->driver);
	pm = &device->pm;
	system = &device->system;
	framemgr = &system->interface.framemgr;

	if (sysfs_streq(buf, "shutdown")) {
		score_info("[sysfs] shutdown\n");
		pdrv->shutdown(pdev);

		if (score_pm_qos_active(pm)) {
			score_frame_manager_unblock(framemgr);
			score_system_boot(system);
		}
	} else if (sysfs_streq(buf, "timeout")) {
		__score_sysfs_timeout_frame(framemgr);
	} else {
		score_info("[sysfs] test invalid command\n");
	}

	score_leave();
p_end:
	return count;
}

#define SCORE_ASB_MASTER_TEXT_NAME	"PMw.bin"
#define SCORE_ASB_MASTER_DATA_NAME	"DMb.bin"
#define SCORE_ASB_KNIGHT1_TEXT_NAME	"PMw0.bin"
#define SCORE_ASB_KNIGHT1_DATA_NAME	"DMb0.bin"
#define SCORE_ASB_KNIGHT2_TEXT_NAME	"PMw1.bin"
#define SCORE_ASB_KNIGHT2_DATA_NAME	"DMb1.bin"
#define SCORE_ASB_DRAM_NAME		"DRAMb.bin"

#define SCORE_ASB_MASTER_TEXT_ADDR	(0x0)
#define SCORE_ASB_KNIGHT1_TEXT_ADDR	(0x3000)
#define SCORE_ASB_KNIGHT2_TEXT_ADDR	(0xa000)

#define SCORE_ASB_MASTER_DATA_ADDR	(0x10000)
#define SCORE_ASB_KNIGHT1_DATA_ADDR	(0x17000)
#define SCORE_ASB_KNIGHT2_DATA_ADDR	(0x77000)

#define SCORE_ASB_DRAM_BIN_ADDR		(0xd7000)
#define SCORE_ASB_FW_DATA_STACK_SIZE	(0x4000)

#define SCORE_ASB_MASTER_TEXT_SIZE	\
	(SCORE_ASB_KNIGHT1_TEXT_ADDR - SCORE_ASB_MASTER_TEXT_ADDR)
#define SCORE_ASB_KNIGHT1_TEXT_SIZE	\
	(SCORE_ASB_KNIGHT2_TEXT_ADDR - SCORE_ASB_KNIGHT1_TEXT_ADDR)
#define SCORE_ASB_KNIGHT2_TEXT_SIZE	\
	(SCORE_ASB_MASTER_DATA_ADDR - SCORE_ASB_KNIGHT2_TEXT_ADDR)

#define SCORE_ASB_MASTER_DATA_SIZE	\
	(SCORE_ASB_KNIGHT1_DATA_ADDR - SCORE_ASB_MASTER_DATA_ADDR)
#define SCORE_ASB_KNIGHT1_DATA_SIZE	\
	(SCORE_ASB_KNIGHT2_DATA_ADDR - SCORE_ASB_KNIGHT1_DATA_ADDR)
#define SCORE_ASB_KNIGHT2_DATA_SIZE	\
	(SCORE_ASB_DRAM_BIN_ADDR - SCORE_ASB_KNIGHT2_DATA_ADDR)

static ssize_t show_sysfs_system_asb(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;

	score_enter();
	count += sprintf(buf + count, "[sysfs] asb verification\n");
	count += sprintf(buf + count, "[sysfs] %10s: addr(%#7x), size(%#7x)\n",
			SCORE_ASB_MASTER_TEXT_NAME,
			SCORE_ASB_MASTER_TEXT_ADDR,
			SCORE_ASB_MASTER_TEXT_SIZE);
	count += sprintf(buf + count, "[sysfs] %10s: addr(%#7x), size(%#7x)\n",
			SCORE_ASB_KNIGHT1_TEXT_NAME,
			SCORE_ASB_KNIGHT1_TEXT_ADDR,
			SCORE_ASB_KNIGHT1_TEXT_SIZE);
	count += sprintf(buf + count, "[sysfs] %10s: addr(%#7x), size(%#7x)\n",
			SCORE_ASB_KNIGHT2_TEXT_NAME,
			SCORE_ASB_KNIGHT2_TEXT_ADDR,
			SCORE_ASB_KNIGHT2_TEXT_SIZE);
	count += sprintf(buf + count, "[sysfs] %10s: addr(%#7x), size(%#7x)\n",
			SCORE_ASB_MASTER_DATA_NAME,
			SCORE_ASB_MASTER_DATA_ADDR,
			SCORE_ASB_MASTER_DATA_SIZE);
	count += sprintf(buf + count, "[sysfs] %10s: addr(%#7x), size(%#7x)\n",
			SCORE_ASB_KNIGHT1_DATA_NAME,
			SCORE_ASB_KNIGHT1_DATA_ADDR,
			SCORE_ASB_KNIGHT1_DATA_SIZE);
	count += sprintf(buf + count, "[sysfs] %10s: addr(%#7x), size(%#7x)\n",
			SCORE_ASB_KNIGHT2_DATA_NAME,
			SCORE_ASB_KNIGHT2_DATA_ADDR,
			SCORE_ASB_KNIGHT2_DATA_SIZE);
	count += sprintf(buf + count, "[sysfs] %10s: addr(%#7x)\n",
			SCORE_ASB_DRAM_NAME,
			SCORE_ASB_DRAM_BIN_ADDR);
	score_leave();
	return count;
}

static int __score_sysfs_firmware_dram_load(struct score_firmware *fw)
{
	int ret = 0;
	const struct firmware *dram_fw;
	char fw_path[32];

	score_enter();
	snprintf(fw_path, 32, "%s", SCORE_ASB_DRAM_NAME);
	ret = request_firmware(&dram_fw, fw_path, fw->dev);
	if (ret) {
		score_info("[sysfs] dram is not loaded(%d)\n", ret);
		goto p_err;
	}

	memcpy(fw->text.kvaddr + SCORE_ASB_DRAM_BIN_ADDR,
			dram_fw->data, dram_fw->size);
	release_firmware(dram_fw);

	score_leave();
	return ret;
p_err:
	return ret;
}


static int __score_sysfs_set_asb(struct score_system *system)
{
	int ret;
	struct score_firmware_manager *fwmgr;
	struct score_firmware *mc, *kc1, *kc2;
	void *kvaddr;
	dma_addr_t dvaddr;
	void *dram_kvaddr;
	dma_addr_t dram_dvaddr;

	score_enter();
	fwmgr = &system->fwmgr;
	kvaddr = score_mmu_get_internal_mem_kvaddr(&system->mmu);
	dvaddr = score_mmu_get_internal_mem_dvaddr(&system->mmu);
	if (!kvaddr || !dvaddr) {
		ret = -EINVAL;
		score_info("[sysfs] memory for fw is invalid\n");
		goto p_err;
	}

	mc = &fwmgr->master;
	kc1 = &fwmgr->knight1;
	kc2 = &fwmgr->knight2;

	snprintf(mc->text.path, SCORE_FW_PATH_LEN, "%s",
			SCORE_ASB_MASTER_TEXT_NAME);
	mc->text.mem_size = SCORE_ASB_MASTER_TEXT_SIZE;
	snprintf(mc->data.path, SCORE_FW_PATH_LEN, "%s",
			SCORE_ASB_MASTER_DATA_NAME);
	mc->data.mem_size = SCORE_ASB_MASTER_DATA_SIZE;
	mc->data.stack = SCORE_ASB_FW_DATA_STACK_SIZE;

	snprintf(kc1->text.path, SCORE_FW_PATH_LEN, "%s",
			SCORE_ASB_KNIGHT1_TEXT_NAME);
	kc1->text.mem_size = SCORE_ASB_KNIGHT1_TEXT_SIZE;
	snprintf(kc1->data.path, SCORE_FW_PATH_LEN, "%s",
			SCORE_ASB_KNIGHT1_DATA_NAME);
	kc1->data.mem_size = SCORE_ASB_KNIGHT1_DATA_SIZE;
	kc1->data.stack = SCORE_ASB_FW_DATA_STACK_SIZE;

	snprintf(kc2->text.path, SCORE_FW_PATH_LEN, "%s",
			SCORE_ASB_KNIGHT2_TEXT_NAME);
	kc2->text.mem_size = SCORE_ASB_KNIGHT2_TEXT_SIZE;
	snprintf(kc2->data.path, SCORE_FW_PATH_LEN, "%s",
			SCORE_ASB_KNIGHT2_DATA_NAME);
	kc2->data.mem_size = SCORE_ASB_KNIGHT2_DATA_SIZE;
	kc2->data.stack = SCORE_ASB_FW_DATA_STACK_SIZE;

	mc->text.kvaddr = kvaddr + SCORE_ASB_MASTER_TEXT_ADDR;
	mc->text.dvaddr = dvaddr + SCORE_ASB_MASTER_TEXT_ADDR;
	kc1->text.kvaddr = kvaddr + SCORE_ASB_KNIGHT1_TEXT_ADDR;
	kc1->text.dvaddr = dvaddr + SCORE_ASB_KNIGHT1_TEXT_ADDR;
	kc2->text.kvaddr = kvaddr + SCORE_ASB_KNIGHT2_TEXT_ADDR;
	kc2->text.dvaddr = dvaddr + SCORE_ASB_KNIGHT2_TEXT_ADDR;

	mc->data.kvaddr = kvaddr + SCORE_ASB_MASTER_DATA_ADDR;
	mc->data.dvaddr = dvaddr + SCORE_ASB_MASTER_DATA_ADDR;
	kc1->data.kvaddr = kvaddr + SCORE_ASB_KNIGHT1_DATA_ADDR;
	kc1->data.dvaddr = dvaddr + SCORE_ASB_KNIGHT1_DATA_ADDR;
	kc2->data.kvaddr = kvaddr + SCORE_ASB_KNIGHT2_DATA_ADDR;
	kc2->data.dvaddr = dvaddr + SCORE_ASB_KNIGHT2_DATA_ADDR;

	dram_kvaddr = kvaddr + SCORE_ASB_DRAM_BIN_ADDR;
	dram_dvaddr = dvaddr + SCORE_ASB_DRAM_BIN_ADDR;

	ret = score_firmware_load(mc);
	if (ret)
		goto p_err;

	ret = score_firmware_load(kc1);
	if (ret)
		goto p_err;

	ret = score_firmware_load(kc2);
	if (ret)
		goto p_err;

	ret = __score_sysfs_firmware_dram_load(mc);
	if (ret)
		goto p_err;

	ret = score_system_sw_reset(system);
	if (ret)
		goto p_err;

	score_dpmu_enable();

	writel(mc->text.dvaddr, system->sfr + MC_CACHE_IC_BASEADDR);
	writel(mc->data.dvaddr, system->sfr + MC_CACHE_DC_BASEADDR);

	writel(kc1->text.dvaddr, system->sfr + KC1_CACHE_IC_BASEADDR);
	writel(kc1->data.dvaddr, system->sfr + KC1_CACHE_DC_BASEADDR);

	writel(kc2->text.dvaddr, system->sfr + KC2_CACHE_IC_BASEADDR);
	writel(kc2->data.dvaddr, system->sfr + KC2_CACHE_DC_BASEADDR);

	writel(dram_dvaddr, system->sfr + 0x8200);
	writel(dram_dvaddr, system->sfr + 0x8204);
	writel(dram_dvaddr, system->sfr + 0x8208);

	/* debug */
	score_info("[sysfs]  MC IC %8x\n",
			readl(system->sfr + MC_CACHE_IC_BASEADDR));
	score_info("[sysfs]  MC DC %8x\n",
			readl(system->sfr + MC_CACHE_DC_BASEADDR));
	score_info("[sysfs] KC1 IC %8x\n",
			readl(system->sfr + KC1_CACHE_IC_BASEADDR));
	score_info("[sysfs] KC1 DC %8x\n",
			readl(system->sfr + KC1_CACHE_DC_BASEADDR));
	score_info("[sysfs] KC2 IC %8x\n",
			readl(system->sfr + KC2_CACHE_IC_BASEADDR));
	score_info("[sysfs] KC2 DC %8x\n",
			readl(system->sfr + KC2_CACHE_DC_BASEADDR));
	score_info("[sysfs] 0x8200 %8x\n", readl(system->sfr + 0x8200));
	score_info("[sysfs] 0x8204 %8x\n", readl(system->sfr + 0x8204));
	score_info("[sysfs] 0x8208 %8x\n", readl(system->sfr + 0x8208));
	score_info("[sysfs]  stack %8x\n", SCORE_ASB_FW_DATA_STACK_SIZE);

	writel(0x2, system->sfr + AXIM0_ARCACHE);
	writel(0x2, system->sfr + AXIM0_AWCACHE);
	writel(0x2, system->sfr + AXIM2_ARCACHE);
	writel(0x2, system->sfr + AXIM2_AWCACHE);

	writel(0x3ffffef, system->sfr + SM_MCGEN);
	writel(0x0, system->sfr + CLK_REQ);

	writel(0x1, system->sfr + MC_WAKEUP);

	score_leave();
	return ret;
p_err:
	return ret;
}

static void __score_sysfs_check_asb(struct score_system *system)
{
	unsigned int value;
	int retry;

	score_info("[sysfs] KC1 check [%#8x]\n", 0x8124);
	retry = 0;
	do {
		value = readl(system->sfr + 0x8124);
		if ((value >> 16) == 0xdead)
			break;
		mdelay(100);
		retry++;
		if (retry == 100)
			break;
	} while (value != 0x900DC1DE);
	score_info("[sysfs] KC1 result [%#x] %8x\n", 0x8124,
			readl(system->sfr + 0x8124));

	score_info("[sysfs] KC2 check [%#8x]\n", 0x8128);
	retry = 0;
	do {
		value = readl(system->sfr + 0x8128);
		if ((value >> 16) == 0xdead)
			break;
		mdelay(100);
		retry++;
		if (retry == 100)
			break;
	} while (value != 0x900DC2DE);
	score_info("[sysfs] KC2 result [%#x] %8x\n", 0x8128,
			readl(system->sfr + 0x8128));

	score_info("[sysfs] MC check [%#8x]\n", 0x8120);
	retry = 0;
	do {
		value = readl(system->sfr + 0x8120);
		if ((value >> 16) == 0xdead)
			break;
		mdelay(100);
		retry++;
		if (retry == 100)
			break;
	} while (value != 0x900DC0DE);
	score_info("[sysfs] MC result [%#x] %8x\n", 0x8120,
			readl(system->sfr + 0x8120));
}

static ssize_t store_sysfs_system_asb(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	struct score_device *device;
	struct score_core *core;
	struct score_system *system;

	score_enter();
	if (!sysfs_streq(buf, "start"))
		goto p_end;

	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	core = &device->core;
	mutex_lock(&core->device_lock);
	if (atomic_read(&device->open_count) != 0) {
		score_info("[sysfs] device is already opend, [asb failed]\n");
		goto p_open_count;
	}

#if defined(CONFIG_PM)
	ret = pm_runtime_get_sync(dev);
#else
	ret = dev->driver->pm->runtime_resume(dev);
#endif
	if (ret) {
		score_info("[sysfs] power_on failed, [asb failed]\n");
		goto p_power;
	}

	system = &device->system;
	ret = score_mmu_open(&system->mmu);
	if (ret)
		goto p_err_mmu;

	ret = __score_sysfs_set_asb(system);
	if (ret)
		goto p_err_firmware;

	__score_sysfs_check_asb(system);
	score_dpmu_print_all();

	score_system_halt(system);
	score_system_sw_reset(system);
	score_firmware_manager_close(&system->fwmgr);
	score_mmu_close(&system->mmu);
#if defined(CONFIG_PM)
	pm_runtime_put_sync(dev);
#else
	dev->driver->pm->runtime_suspend(dev);
#endif

	mutex_unlock(&core->device_lock);
	score_leave();
	return count;
p_err_firmware:
	score_mmu_close(&system->mmu);
p_err_mmu:
#if defined(CONFIG_PM)
	pm_runtime_put_sync(dev);
#else
	dev->driver->pm->runtime_suspend(dev);
#endif
p_power:
p_open_count:
	mutex_unlock(&core->device_lock);
p_end:
	return count;
}

static DEVICE_ATTR(power, 0644,
		show_sysfs_system_power,
		store_sysfs_system_power);
#if defined(CONFIG_PM_DEVFREQ)
static DEVICE_ATTR(pm_qos, 0644,
		show_sysfs_system_pm_qos,
		store_sysfs_system_pm_qos);
#endif
#if defined(CONFIG_PM_SLEEP)
static DEVICE_ATTR(pm_sleep, 0644,
		show_sysfs_system_pm_sleep,
		store_sysfs_system_pm_sleep);
#endif
static DEVICE_ATTR(clk, 0644,
		show_sysfs_system_clk,
		store_sysfs_system_clk);
static DEVICE_ATTR(print, 0644,
		show_sysfs_system_print,
		store_sysfs_system_print);
static DEVICE_ATTR(sfr, 0644,
		show_sysfs_system_sfr,
		store_sysfs_system_sfr);
static DEVICE_ATTR(dpmu, 0644,
		show_sysfs_system_dpmu,
		store_sysfs_system_dpmu);
static DEVICE_ATTR(scq, 0644,
		show_sysfs_system_scq,
		store_sysfs_system_scq);
static DEVICE_ATTR(frame_count, 0644,
		show_sysfs_system_frame_count,
		store_sysfs_system_frame_count);
static DEVICE_ATTR(wait_time, 0644,
		show_sysfs_system_wait_time,
		store_sysfs_system_wait_time);
static DEVICE_ATTR(dpmu_print, 0644,
		show_sysfs_system_dpmu_print,
		store_sysfs_system_dpmu_print);
static DEVICE_ATTR(asb, 0644,
		show_sysfs_system_asb,
		store_sysfs_system_asb);
static DEVICE_ATTR(test, 0644,
		show_sysfs_system_test,
		store_sysfs_system_test);

static struct attribute *score_system_entries[] = {
	&dev_attr_power.attr,
#if defined(CONFIG_PM_DEVFREQ)
	&dev_attr_pm_qos.attr,
#endif
#if defined(CONFIG_PM_SLEEP)
	&dev_attr_pm_sleep.attr,
#endif
	&dev_attr_clk.attr,
	&dev_attr_print.attr,
	&dev_attr_sfr.attr,
	&dev_attr_dpmu.attr,
	&dev_attr_scq.attr,
	&dev_attr_frame_count.attr,
	&dev_attr_wait_time.attr,
	&dev_attr_dpmu_print.attr,
	&dev_attr_asb.attr,
	&dev_attr_test.attr,
	NULL,
};

static struct attribute_group score_system_attr_group = {
	.name   = "system",
	.attrs  = score_system_entries,
};

int score_sysfs_probe(struct score_device *device)
{
	int ret;

	score_enter();
	ret = sysfs_create_group(&device->dev->kobj, &score_system_attr_group);
	if (ret) {
		score_err("score_system_attr_group is not created (%d)\n", ret);
		goto p_err_sysfs;
	}

	ret = score_profiler_probe(device, &score_system_attr_group);
	if (ret)
		goto p_err_profiler;

	score_leave();
	return ret;
p_err_profiler:
	sysfs_remove_group(&device->dev->kobj, &score_system_attr_group);
p_err_sysfs:
	return ret;
}

void score_sysfs_remove(struct score_device *device)
{
	score_enter();
	score_profiler_remove();
	sysfs_remove_group(&device->dev->kobj, &score_system_attr_group);
	score_leave();
}
