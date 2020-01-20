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
		if (device->pm.default_qos < 0) {
			ret = score_device_start(device, request_fw_id,
					SCORE_PM_QOS_L0, 0);
		} else {
			ret = score_device_start(device, request_fw_id,
					device->pm.default_qos, 0);
		}
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
		count += sprintf(buf + count, "-/-\n");
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
	} else if (sysfs_streq(buf, "L5")) {
		score_info("[sysfs] pm_qos_level L5\n");
		pm->default_qos = SCORE_PM_QOS_L5;
		score_pm_qos_update(pm, SCORE_PM_QOS_L5);
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
				score_clk_get(clk, SCORE_TS));
		count += sprintf(buf + count, "clks %lu\n",
				score_clk_get(clk, SCORE_BARON));
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
	void __iomem *sfr;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	pm = &device->pm;
	sfr = device->system.sfr;

	mutex_lock(&pm->lock);
	if (!score_pm_qos_active(pm)) {
		count += sprintf(buf + count, "power off\n");
		goto p_unlock;
	}

	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"RELEASE_DATE",
			readl(sfr + RELEASE_DATE),
			"SW_RESET",
			readl(sfr + SW_RESET));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"DSPM_MCGEN",
			readl(sfr + DSPM_MCGEN),
			"DSPM_ALWAYS_ON",
			readl(sfr + DSPM_ALWAYS_ON));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"DSPS_MCGEN",
			readl(sfr + DSPS_MCGEN),
			"DSPS_ALWAYS_ON",
			readl(sfr + DSPS_ALWAYS_ON));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"DSPM_QCH_INFO",
			readl(sfr + DSPM_QCH_INFO),
			"DSPS_QCH_INFO",
			readl(sfr + DSPS_QCH_INFO));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"AXIM0_SIGNAL",
			readl(sfr + AXIM0_SIGNAL),
			"AXIM2_SIGNAL",
			readl(sfr + AXIM2_SIGNAL));

	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"APCPU_INTR_ENABLE",
			readl(sfr + APCPU_INTR_ENABLE),
			"TS_INTR_ENABLE",
			readl(sfr + TS_INTR_ENABLE));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"IVA_INTR_ENABLE",
			readl(sfr + IVA_INTR_ENABLE),
			"NPU_INTR_ENABLE",
			readl(sfr + NPU_INTR_ENABLE));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"BR1_INTR_ENABLE",
			readl(sfr + BR1_INTR_ENABLE),
			"BR2_INTR_ENABLE",
			readl(sfr + BR2_INTR_ENABLE));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"TS_CACHE_INTR_ENABLE",
			readl(sfr + TS_CACHE_INTR_ENABLE),
			"BR_CACHE_INTR_ENABLE",
			readl(sfr + BR_CACHE_INTR_ENABLE));
	count += sprintf(buf + count, "%24s:%8x\n",
			"SCQ_INTR_ENABLE",
			readl(sfr + SCQ_INTR_ENABLE));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"DSPM_DBG_INTR_ENABLE",
			readl(sfr + DSPM_DBG_INTR_ENABLE),
			"DSPS_DBG_INTR_ENABLE",
			readl(sfr + DSPS_DBG_INTR_ENABLE));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"DSPM_DBG_MONITOR_ENABLE",
			readl(sfr + DSPM_DBG_MONITOR_ENABLE),
			"DSPS_DBG_MONITOR_ENABLE",
			readl(sfr + DSPS_DBG_MONITOR_ENABLE));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"DSPM_PERF_MONITOR_ENABLE",
			readl(sfr + DSPM_PERF_MONITOR_ENABLE),
			"DSPS_PERF_MONITOR_ENABLE",
			readl(sfr + DSPS_PERF_MONITOR_ENABLE));

	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"APCPU_INTR_STATUS",
			readl(sfr + APCPU_RAW_INTR_STATUS),
			"SCQ_INTR_STATUS",
			readl(sfr + SCQ_INTR_STATUS));

	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"ISP_INTR_STATUS",
			readl(sfr + ISP_INTR_STATUS),
			"SHUB_INTR_STATUS",
			readl(sfr + SHUB_INTR_STATUS));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"APCPU_SWI_STATUS",
			readl(sfr + APCPU_SWI_STATUS),
			"TS_SWI_STATUS",
			readl(sfr + TS_SWI_STATUS));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"BR1_SWI_STATUS",
			readl(sfr + BR1_SWI_STATUS),
			"BR2_SWI_STATUS",
			readl(sfr + BR2_SWI_STATUS));
	count += sprintf(buf + count, "%24s:%8x\n",
			"NPU_SWI_STATUS",
			readl(sfr + NPU_SWI_STATUS));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"TS_CACHE_INTR_STATUS",
			readl(sfr + TS_CACHE_RAW_INTR),
			"BR_CACHE_INTR_STATUS",
			readl(sfr + BR_CACHE_RAW_INTR));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"DSPM_DBG_INTR_STATUS",
			readl(sfr + DSPM_DBG_RAW_INTR_STATUS),
			"DSPS_DBG_INTR_STATUS",
			readl(sfr + DSPS_DBG_RAW_INTR_STATUS));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"BR0_STA_MONITOR",
			readl(sfr + BR0_STA_MONITOR),
			"BR1_STA_MONITOR",
			readl(sfr + BR1_STA_MONITOR));
	count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
			"MPU_MONITOR(0-3)",
			readl(sfr + MPU_MONITOR0),
			readl(sfr + MPU_MONITOR1),
			readl(sfr + MPU_MONITOR2),
			readl(sfr + MPU_MONITOR3));
	count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
			"MPU_MONITOR(4-7)",
			readl(sfr + MPU_MONITOR4),
			readl(sfr + MPU_MONITOR5),
			readl(sfr + MPU_MONITOR6),
			readl(sfr + MPU_MONITOR7));
	count += sprintf(buf + count, "%24s:%8x\n",
			"TS_CACHE_STATUS",
			readl(sfr + TS_CACHE_STATUS));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"TS_CACHE_IC_BASE_ADDR",
			readl(sfr + TS_CACHE_IC_BASE_ADDR),
			"TS_CACHE_IC_CODE_SIZE",
			readl(sfr + TS_CACHE_IC_CODE_SIZE));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"TS_CACHE_DC_BASE_ADDR",
			readl(sfr + TS_CACHE_DC_BASE_ADDR),
			"TS_CACHE_DC_DATA_SIZE",
			readl(sfr + TS_CACHE_DC_DATA_SIZE));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"TS_CACHE_STACK_START",
			readl(sfr + TS_CACHE_DC_STACK_START_ADDR),
			"TS_CACHE_STACK_END",
			readl(sfr + TS_CACHE_DC_STACK_END_ADDR));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"TS_CACHE_IC_CTRL_STATUS",
			readl(sfr + TS_CACHE_IC_CTRL_STATUS),
			"TS_CACHE_DC_CTRL_STATUS",
			readl(sfr + TS_CACHE_DC_CTRL_STATUS));

	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"BR_CACHE_MODE",
			readl(sfr + BR_CACHE_MODE),
			"BR_CACHE_STATUS",
			readl(sfr + BR_CACHE_STATUS));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"BR_CACHE_IC_BASE_ADDR",
			readl(sfr + BR_CACHE_IC_BASE_ADDR),
			"BR_CACHE_IC_CODE_SIZE",
			readl(sfr + BR_CACHE_IC_CODE_SIZE));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"BR1_CACHE_DC_BASE_ADDR",
			readl(sfr + BR1_CACHE_DC_BASE_ADDR),
			"BR1_CACHE_DC_DATA_SIZE",
			readl(sfr + BR1_CACHE_DC_DATA_SIZE));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"BR2_CACHE_DC_BASE_ADDR",
			readl(sfr + BR2_CACHE_DC_BASE_ADDR),
			"BR2_CACHE_DC_DATA_SIZE",
			readl(sfr + BR2_CACHE_DC_DATA_SIZE));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"BR1_CACHE_STACK_START",
			readl(sfr + BR1_CACHE_DC_STACK_START_ADDR),
			"BR1_CACHE_STACK_END",
			readl(sfr + BR1_CACHE_DC_STACK_END_ADDR));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"BR2_CACHE_STACK_START",
			readl(sfr + BR2_CACHE_DC_STACK_START_ADDR),
			"BR2_CACHE_STACK_END",
			readl(sfr + BR2_CACHE_DC_STACK_END_ADDR));
	count += sprintf(buf + count, "%24s:%8x\n",
			"BR_CACHE_IC_CTRL_STATUS",
			readl(sfr + BR_CACHE_IC_CTRL_STATUS));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"BR1_CACHE_DC_CTRL_STATUS",
			readl(sfr + BR1_CACHE_DC_CTRL_STATUS),
			"BR2_CACHE_DC_CTRL_STATUS",
			readl(sfr + BR2_CACHE_DC_CTRL_STATUS));

	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"SCQ_CMD_NUMBER",
			readl(sfr + SCQ_QUEUE_CMD_NUMBER),
			"SCQ_CMD_VALID",
			readl(sfr + SCQ_QUEUE_CMD_VALID));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"SCQ_CMD_NUMBER1",
			readl(sfr + SCQ_QUEUE_CMD_NUMBER1),
			"SCQ_CMD_NUMBER2",
			readl(sfr + SCQ_QUEUE_CMD_NUMBER2));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"SCQ_READ_LEFT_COUNT",
			readl(sfr + SCQ_READ_SRAM_LEFT_COUNT),
			"SCQ_WRITE_LEFT_COUNT",
			readl(sfr + SCQ_WRITE_SRAM_LEFT_COUNT));

	count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
			"DMA_QUEUE_STATUS(0-3)",
			readl(sfr + DMA0_QUEUE_STATUS),
			readl(sfr + DMA1_QUEUE_STATUS),
			readl(sfr + DMA2_QUEUE_STATUS),
			readl(sfr + DMA3_QUEUE_STATUS));
	count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
			"DMA_PUSH_COUNT(0-3)",
			readl(sfr + DMA0_PUSH_COUNT),
			readl(sfr + DMA1_PUSH_COUNT),
			readl(sfr + DMA2_PUSH_COUNT),
			readl(sfr + DMA3_PUSH_COUNT));
	count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
			"DMA_POP_COUNT(0-3)",
			readl(sfr + DMA0_POP_COUNT),
			readl(sfr + DMA1_POP_COUNT),
			readl(sfr + DMA2_POP_COUNT),
			readl(sfr + DMA3_POP_COUNT));

	count += sprintf(buf + count, "%24s:%8x %8x %8x\n",
			"INIT_DONE(ts/b1/b2)",
			readl(sfr + TS_INIT_DONE_CHECK),
			readl(sfr + BR1_INIT_DONE_CHECK),
			readl(sfr + BR2_INIT_DONE_CHECK));
	count += sprintf(buf + count, "%24s:%8x %8x\n",
			"TS_MALLOC(addr/size)",
			readl(sfr + TS_MALLOC_BASE_ADDR),
			readl(sfr + TS_MALLOC_SIZE));
	count += sprintf(buf + count, "%24s:%8x %8x %8x\n",
			"BR_MALLOC(b1/b2/size)",
			readl(sfr + BR1_MALLOC_BASE_ADDR),
			readl(sfr + BR2_MALLOC_BASE_ADDR),
			readl(sfr + BR_MALLOC_SIZE));
	count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
			"PRINT(addr/size/f/r)",
			readl(sfr + PRINT_BASE_ADDR),
			readl(sfr + PRINT_SIZE),
			readl(sfr + PRINT_QUEUE_FRONT_ADDR),
			readl(sfr + PRINT_QUEUE_REAR_ADDR));
	count += sprintf(buf + count, "%24s:%8x %8x %8x\n",
			"PROFILER(ts/b1/b2)",
			readl(sfr + PROFILER_TS_BASE_ADDR),
			readl(sfr + PROFILER_BR1_BASE_ADDR),
			readl(sfr + PROFILER_BR2_BASE_ADDR));
	count += sprintf(buf + count, "%24s:%8x %8x %8x\n",
			"PROFILER_HEAD(ts/b1/b2)",
			readl(sfr + PROFILER_TS_HEAD),
			readl(sfr + PROFILER_BR1_HEAD),
			readl(sfr + PROFILER_BR2_HEAD));
	count += sprintf(buf + count, "%24s:%8x %8x\n",
			"PROLOGUE_DONE(b1/b2)",
			readl(sfr + BR1_PROLOGUE_DONE),
			readl(sfr + BR2_PROLOGUE_DONE));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"TOTAL_TILE_COUNT",
			readl(sfr + TOTAL_TILE_COUNT),
			"TOTAL_TCM_SIZE",
			readl(sfr + TOTAL_TCM_SIZE));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"ENABLE_TCM_HALF",
			readl(sfr + ENABLE_TCM_HALF),
			"TS_VSIZE_TYPE",
			readl(sfr + TS_VSIZE_TYPE));
	count += sprintf(buf + count, "%24s:%8x %8x\n",
			"SCHEDULE_COUNT(n/h)",
			readl(sfr + SCHEDULE_NORMAL_COUNT),
			readl(sfr + SCHEDULE_HIGH_COUNT));
	count += sprintf(buf + count, "%24s:%8x\n",
			"SCHEDULE_BITMAP",
			readl(sfr + SCHEDULE_BITMAP));
	count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
			"SCHEDULE_WRPOS(n/h)",
			readl(sfr + SCHEDULE_NORMAL_WPOS),
			readl(sfr + SCHEDULE_NORMAL_RPOS),
			readl(sfr + SCHEDULE_HIGH_WPOS),
			readl(sfr + SCHEDULE_HIGH_RPOS));
	count += sprintf(buf + count, "%24s:%8x %8x %8x %8x %8x\n",
			"TS_ABORT_STATE(0-4)",
			readl(sfr + TS_ABORT_STATE0),
			readl(sfr + TS_ABORT_STATE1),
			readl(sfr + TS_ABORT_STATE2),
			readl(sfr + TS_ABORT_STATE3),
			readl(sfr + TS_ABORT_STATE4));
	count += sprintf(buf + count, "%24s:%8x\n",
			"SCHEDULE_STATE",
			readl(sfr + SCHEDULE_STATE));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"CORE_STATE",
			readl(sfr + CORE_STATE),
			"PRIORITY_STATE",
			readl(sfr + PRIORITY_STATE));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"TS_KERNEL_ID",
			readl(sfr + TS_KERNEL_ID),
			"TS_ENTRY_FLAG",
			readl(sfr + TS_ENTRY_FLAG));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"INDIRECT_FUNC",
			readl(sfr + INDIRECT_FUNC),
			"INDIRECT_ADDR",
			readl(sfr + INDIRECT_ADDR));
	count += sprintf(buf + count, "%24s:%8x %24s:%8x\n",
			"TS_REQUEST_TYPE",
			readl(sfr + TS_REQUEST_TYPE),
			"TS_TCM_MALLOC_STATE",
			readl(sfr + TS_TCM_MALLOC_STATE));
	count += sprintf(buf + count, "%24s:%8x %8x\n",
			"CURRENT_KERNEL(b1/b2)",
			readl(sfr + BR1_CURRENT_KERNEL),
			readl(sfr + BR2_CURRENT_KERNEL));

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
	void __iomem *sfr;
	int idx;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	pm = &device->pm;
	mutex_lock(&pm->lock);
	if (score_dpmu_debug_check())
		count += sprintf(buf + count, "< debug enable ");
	else
		count += sprintf(buf + count, "< debug disable ");
	if (score_dpmu_performance_check())
		count += sprintf(buf + count, "/ performance enable ");
	else
		count += sprintf(buf + count, "/ performance disable ");

	if (!score_pm_qos_active(pm)) {
		count += sprintf(buf + count, "/ power off >\n");
		goto p_unlock;
	} else {
		count += sprintf(buf + count, "/ power on >\n");
	}

	sfr = device->system.sfr;

	if (score_dpmu_debug_check()) {
		for (idx = 0; idx < 5; ++idx)
			count += sprintf(buf + count, "%21s[%d]:%8x %8x\n",
					"PC(ts/b1)", idx,
					readl(sfr + TS_PROGRAM_COUNTER),
					readl(sfr + BR1_PROGRAM_COUNTER));

		count += sprintf(buf + count, "%24s:%8x\n",
				"TS_THREAD_INFO",
				readl(sfr + TS_INFO));
		count += sprintf(buf + count, "%24s:%8x\n",
				"SCQ_SLVERR_INFO",
				readl(sfr + SCQ_SLVERR_INFO));
		count += sprintf(buf + count, "%24s:%8x\n",
				"TS_CACHE_SLVERR_INFO",
				readl(sfr + TSCACHE_SLVERR_INFO0));
		count += sprintf(buf + count, "%24s:%8x %8x %8x\n",
				"TS_CACHE_ERR(pm/dm/sp)",
				readl(sfr + TSCACHE_SLVERR_INFO1),
				readl(sfr + TSCACHE_SLVERR_INFO2),
				readl(sfr + TSCACHE_SLVERR_INFO3));
#if defined(SCORE_EVT1)
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"TS_ACTIVE_COUNT",
				readl(sfr + TS_ACTIVE_COUNT_HIGH),
				readl(sfr + TS_ACTIVE_COUNT_LOW));
#endif
		count += sprintf(buf + count, "%24s:%8x\n",
				"BR_CACHE_SLVERR_INFO",
				readl(sfr + BRCACHE_SLVERR_INFO0));
		count += sprintf(buf + count, "%24s:%8x %8x %8x\n",
				"BR1_CACHE_ERR(pm/dm/sp)",
				readl(sfr + BRCACHE_SLVERR_INFO1),
				readl(sfr + BRCACHE_SLVERR_INFO2),
				readl(sfr + BRCACHE_SLVERR_INFO3));
		count += sprintf(buf + count, "%24s:%8x %8x %8x\n",
				"BR2_CACHE_ERR(pm/dm/sp)",
				readl(sfr + BRCACHE_SLVERR_INFO4),
				readl(sfr + BRCACHE_SLVERR_INFO5),
				readl(sfr + BRCACHE_SLVERR_INFO6));
		count += sprintf(buf + count, "%24s:%8x\n",
				"TCM_BOUNDARY_ERRINFO",
				readl(sfr + TCMBOUNDARY_ERRINFO));
#if defined(SCORE_EVT1)
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"BR1_ACTIVE_COUNT",
				readl(sfr + BR1_ACTIVE_COUNT_HIGH),
				readl(sfr + BR1_ACTIVE_COUNT_LOW));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"BR2_ACTIVE_COUNT",
				readl(sfr + BR2_ACTIVE_COUNT_HIGH),
				readl(sfr + BR2_ACTIVE_COUNT_LOW));
#endif
	}

	if (score_dpmu_debug_check()) {
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"RDBEAT_COUNT(m0-3)",
				readl(sfr + AXIM0_RDBEAT_COUNT),
				readl(sfr + AXIM1_RDBEAT_COUNT),
				readl(sfr + AXIM2_RDBEAT_COUNT),
				readl(sfr + AXIM3_RDBEAT_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"RDBEAT_COUNT(s0-1)",
				readl(sfr + AXIS1_RDBEAT_COUNT),
				readl(sfr + AXIS2_RDBEAT_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"WRBEAT_COUNT(m0-3)",
				readl(sfr + AXIM0_WRBEAT_COUNT),
				readl(sfr + AXIM1_WRBEAT_COUNT),
				readl(sfr + AXIM2_WRBEAT_COUNT),
				readl(sfr + AXIM3_WRBEAT_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"WRBEAT_COUNT(s0-1)",
				readl(sfr + AXIS1_WRBEAT_COUNT),
				readl(sfr + AXIS2_WRBEAT_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"RD_MAX_MO_COUNT(m0-3)",
				readl(sfr + AXIM0_RD_MAX_MO_COUNT),
				readl(sfr + AXIM1_RD_MAX_MO_COUNT),
				readl(sfr + AXIM2_RD_MAX_MO_COUNT),
				readl(sfr + AXIM3_RD_MAX_MO_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"RD_MAX_MO_COUNT(s0-1)",
				readl(sfr + AXIS1_RD_MAX_MO_COUNT),
				readl(sfr + AXIS2_RD_MAX_MO_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"WR_MAX_MO_COUNT(m0-3)",
				readl(sfr + AXIM0_WR_MAX_MO_COUNT),
				readl(sfr + AXIM1_WR_MAX_MO_COUNT),
				readl(sfr + AXIM2_WR_MAX_MO_COUNT),
				readl(sfr + AXIM3_WR_MAX_MO_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"WR_MAX_MO_COUNT(s0-1)",
				readl(sfr + AXIS1_WR_MAX_MO_COUNT),
				readl(sfr + AXIS2_WR_MAX_MO_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"RD_MAX_REQ_LAT(m0-3)",
				readl(sfr + AXIM0_RD_MAX_REQ_LAT),
				readl(sfr + AXIM1_RD_MAX_REQ_LAT),
				readl(sfr + AXIM2_RD_MAX_REQ_LAT),
				readl(sfr + AXIM3_RD_MAX_REQ_LAT));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"RD_MAX_REQ_LAT(s0-1)",
				readl(sfr + AXIS1_RD_MAX_REQ_LAT),
				readl(sfr + AXIS2_RD_MAX_REQ_LAT));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"WR_MAX_REQ_LAT(m0-3)",
				readl(sfr + AXIM0_WR_MAX_REQ_LAT),
				readl(sfr + AXIM1_WR_MAX_REQ_LAT),
				readl(sfr + AXIM2_WR_MAX_REQ_LAT),
				readl(sfr + AXIM3_WR_MAX_REQ_LAT));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"WR_MAX_REQ_LAT(s0-1)",
				readl(sfr + AXIS1_WR_MAX_REQ_LAT),
				readl(sfr + AXIS2_WR_MAX_REQ_LAT));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"RD_MAX_AR2R_LAT(m0-3)",
				readl(sfr + AXIM0_RD_MAX_AR2R_LAT),
				readl(sfr + AXIM1_RD_MAX_AR2R_LAT),
				readl(sfr + AXIM2_RD_MAX_AR2R_LAT),
				readl(sfr + AXIM3_RD_MAX_AR2R_LAT));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"RD_MAX_AR2R_LAT(s0-1)",
				readl(sfr + AXIS1_RD_MAX_AR2R_LAT),
				readl(sfr + AXIS2_RD_MAX_AR2R_LAT));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"RD_MAX_R2R_LAT(m0-3)",
				readl(sfr + AXIM0_RD_MAX_R2R_LAT),
				readl(sfr + AXIM1_RD_MAX_R2R_LAT),
				readl(sfr + AXIM2_RD_MAX_R2R_LAT),
				readl(sfr + AXIM3_RD_MAX_R2R_LAT));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"RD_MAX_R2R_LAT(s0-1)",
				readl(sfr + AXIS1_RD_MAX_R2R_LAT),
				readl(sfr + AXIS2_RD_MAX_R2R_LAT));
#if defined(SCORE_EVT1)
		count += sprintf(buf + count, "%24s:%8x\n",
				"BRCACHE_RDBEAT_COUNT",
				readl(sfr + AXIM0_BRCACHE_RDBEAT_COUNT));
		count += sprintf(buf + count, "%24s:%8x\n",
				"BRCACHE_WRBEAT_COUNT",
				readl(sfr + AXIM0_BRCACHE_WRBEAT_COUNT));
		count += sprintf(buf + count, "%24s:%8x\n",
				"BRCACHE_RD_MAX_MO_COUNT",
				readl(sfr + AXIM0_BRCACHE_RD_MAX_MO_COUNT));
		count += sprintf(buf + count, "%24s:%8x\n",
				"BRCACHE_WR_MAX_MO_COUNT",
				readl(sfr + AXIM0_BRCACHE_WR_MAX_MO_COUNT));
		count += sprintf(buf + count, "%24s:%8x\n",
				"BRCACHE_RD_MAX_REQ_LAT",
				readl(sfr + AXIM0_BRCACHE_RD_MAX_REQ_LAT));
		count += sprintf(buf + count, "%24s:%8x\n",
				"BRCACHE_WR_MAX_REQ_LAT",
				readl(sfr + AXIM0_BRCACHE_WR_MAX_REQ_LAT));
		count += sprintf(buf + count, "%24s:%8x\n",
				"BRCACHE_RD_MAX_AR2R_LAT",
				readl(sfr + AXIM0_BRCACHE_RD_MAX_AR2R_LAT));
		count += sprintf(buf + count, "%24s:%8x\n",
				"BRCACHE_RD_MAX_R2R_LAT",
				readl(sfr + AXIM0_BRCACHE_RD_MAX_R2R_LAT));
#endif
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"TS_INST_COUNT",
				readl(sfr + TS_INST_COUNT_HIGH),
				readl(sfr + TS_INST_COUNT_LOW));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"TS_INTR_COUNT",
				readl(sfr + TS_INTR_COUNT_HIGH),
				readl(sfr + TS_INTR_COUNT_LOW));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"BR1_INST_COUNT",
				readl(sfr + BR1_INST_COUNT_HIGH),
				readl(sfr + BR1_INST_COUNT_LOW));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"BR2_INST_COUNT",
				readl(sfr + BR2_INST_COUNT_HIGH),
				readl(sfr + BR2_INST_COUNT_LOW));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"BR1_INTR_COUNT",
				readl(sfr + BR1_INTR_COUNT_HIGH),
				readl(sfr + BR1_INTR_COUNT_LOW));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"BR2_INTR_COUNT",
				readl(sfr + BR2_INTR_COUNT_HIGH),
				readl(sfr + BR2_INTR_COUNT_LOW));
		count += sprintf(buf + count, "%24s:%8x %8x %8x\n",
				"TS_ICACHE(access/miss)",
				readl(sfr + TS_ICACHE_ACCESS_COUNT_HIGH),
				readl(sfr + TS_ICACHE_ACCESS_COUNT_LOW),
				readl(sfr + TS_ICACHE_MISS_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"TS_DCACHE(access/miss)",
				readl(sfr + TS_DCACHE_ACCESS_COUNT),
				readl(sfr + TS_DCACHE_MISS_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x %8x\n",
				"BR1_ICACHE(access/miss)",
				readl(sfr + BR1_ICACHE_ACCESS_COUNT_HIGH),
				readl(sfr + BR1_ICACHE_ACCESS_COUNT_LOW),
				readl(sfr + BR1_ICACHE_MISS_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x %8x\n",
				"BR2_ICACHE(access/miss)",
				readl(sfr + BR2_ICACHE_ACCESS_COUNT_HIGH),
				readl(sfr + BR2_ICACHE_ACCESS_COUNT_LOW),
				readl(sfr + BR2_ICACHE_MISS_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"BRL2_ICACHE(access/miss)",
				readl(sfr + BRL2_ICACHE_ACCESS_COUNT),
				readl(sfr + BRL2_ICACHE_MISS_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"BR1_DCACHE(access/miss)",
				readl(sfr + BR1_DCACHE_ACCESS_COUNT),
				readl(sfr + BR1_DCACHE_MISS_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"BR2_DCACHE(access/miss)",
				readl(sfr + BR2_DCACHE_ACCESS_COUNT),
				readl(sfr + BR2_DCACHE_MISS_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"TS_STALL(core/pm/dm/sp)",
				readl(sfr + TS_CORE_STALL_COUNT),
				readl(sfr + TS_PM_STALL_COUNT),
				readl(sfr + TS_DM_STALL_COUNT),
				readl(sfr + TS_SP_STALL_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"BR1_STALL(core/pm/dm/sp)",
				readl(sfr + BR1_CORE_STALL_COUNT),
				readl(sfr + BR1_PM_STALL_COUNT),
				readl(sfr + BR1_DM_STALL_COUNT),
				readl(sfr + BR1_SP_STALL_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"BR2_STALL(core/pm/dm/sp)",
				readl(sfr + BR2_CORE_STALL_COUNT),
				readl(sfr + BR2_PM_STALL_COUNT),
				readl(sfr + BR2_DM_STALL_COUNT),
				readl(sfr + BR2_SP_STALL_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x %8x\n",
				"SFR_STALL(ts/b1/b2)",
				readl(sfr + TS_SFR_STALL_COUNT),
				readl(sfr + BR1_SFR_STALL_COUNT),
				readl(sfr + BR2_SFR_STALL_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"BR1_TCM(stall/access)",
				readl(sfr + BR1_TCM_STALL_COUNT),
				readl(sfr + BR1_TCM_ACCESS_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"BR2_TCM(stall/access)",
				readl(sfr + BR2_TCM_STALL_COUNT),
				readl(sfr + BR2_TCM_ACCESS_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"DMARD_TCM(stall/access)",
				readl(sfr + DMARD_TCM_STALL_COUNT),
				readl(sfr + DMARD_TCM_ACCESS_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"DMAWR_TCM(stall/access)",
				readl(sfr + DMAWR_TCM_STALL_COUNT),
				readl(sfr + DMAWR_TCM_ACCESS_COUNT));
		count += sprintf(buf + count, "%24s:%8x %8x\n",
				"IVA_TCM(stall/access)",
				readl(sfr + IVA_TCM_STALL_COUNT),
				readl(sfr + IVA_TCM_ACCESS_COUNT));
		count += sprintf(buf + count, "%24s:%8x\n",
				"DMA_EXEC_INDEX_INFO",
				readl(sfr + DMA_EXEC_INDEX_INFO));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"DMA0_EXEC_COUNT(00-03)",
				readl(sfr + DMA0_EXEC_COUNT0),
				readl(sfr + DMA0_EXEC_COUNT1),
				readl(sfr + DMA0_EXEC_COUNT2),
				readl(sfr + DMA0_EXEC_COUNT3));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"DMA0_EXEC_COUNT(04-07)",
				readl(sfr + DMA0_EXEC_COUNT4),
				readl(sfr + DMA0_EXEC_COUNT5),
				readl(sfr + DMA0_EXEC_COUNT6),
				readl(sfr + DMA0_EXEC_COUNT7));

		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"DMA0_EXEC_COUNT(08-11)",
				readl(sfr + DMA0_EXEC_COUNT8),
				readl(sfr + DMA0_EXEC_COUNT9),
				readl(sfr + DMA0_EXEC_COUNT10),
				readl(sfr + DMA0_EXEC_COUNT11));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"DMA0_EXEC_COUNT(12-15)",
				readl(sfr + DMA0_EXEC_COUNT12),
				readl(sfr + DMA0_EXEC_COUNT13),
				readl(sfr + DMA0_EXEC_COUNT14),
				readl(sfr + DMA0_EXEC_COUNT15));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"DMA1_EXEC_COUNT(00-03)",
				readl(sfr + DMA1_EXEC_COUNT0),
				readl(sfr + DMA1_EXEC_COUNT1),
				readl(sfr + DMA1_EXEC_COUNT2),
				readl(sfr + DMA1_EXEC_COUNT3));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"DMA1_EXEC_COUNT(04-07)",
				readl(sfr + DMA1_EXEC_COUNT4),
				readl(sfr + DMA1_EXEC_COUNT5),
				readl(sfr + DMA1_EXEC_COUNT6),
				readl(sfr + DMA1_EXEC_COUNT7));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"DMA1_EXEC_COUNT(08-11)",
				readl(sfr + DMA1_EXEC_COUNT8),
				readl(sfr + DMA1_EXEC_COUNT9),
				readl(sfr + DMA1_EXEC_COUNT10),
				readl(sfr + DMA1_EXEC_COUNT11));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"DMA1_EXEC_COUNT(12-15)",
				readl(sfr + DMA1_EXEC_COUNT12),
				readl(sfr + DMA1_EXEC_COUNT13),
				readl(sfr + DMA1_EXEC_COUNT14),
				readl(sfr + DMA1_EXEC_COUNT15));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"DMA2_EXEC_COUNT(00-03)",
				readl(sfr + DMA2_EXEC_COUNT0),
				readl(sfr + DMA2_EXEC_COUNT1),
				readl(sfr + DMA2_EXEC_COUNT2),
				readl(sfr + DMA2_EXEC_COUNT3));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"DMA2_EXEC_COUNT(04-07)",
				readl(sfr + DMA2_EXEC_COUNT4),
				readl(sfr + DMA2_EXEC_COUNT5),
				readl(sfr + DMA2_EXEC_COUNT6),
				readl(sfr + DMA2_EXEC_COUNT7));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"DMA2_EXEC_COUNT(08-11)",
				readl(sfr + DMA2_EXEC_COUNT8),
				readl(sfr + DMA2_EXEC_COUNT9),
				readl(sfr + DMA2_EXEC_COUNT10),
				readl(sfr + DMA2_EXEC_COUNT11));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"DMA2_EXEC_COUNT(12-15)",
				readl(sfr + DMA2_EXEC_COUNT12),
				readl(sfr + DMA2_EXEC_COUNT13),
				readl(sfr + DMA2_EXEC_COUNT14),
				readl(sfr + DMA2_EXEC_COUNT15));

		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"DMA3_EXEC_COUNT(00-03)",
				readl(sfr + DMA3_EXEC_COUNT0),
				readl(sfr + DMA3_EXEC_COUNT1),
				readl(sfr + DMA3_EXEC_COUNT2),
				readl(sfr + DMA3_EXEC_COUNT3));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"DMA3_EXEC_COUNT(04-07)",
				readl(sfr + DMA3_EXEC_COUNT4),
				readl(sfr + DMA3_EXEC_COUNT5),
				readl(sfr + DMA3_EXEC_COUNT6),
				readl(sfr + DMA3_EXEC_COUNT7));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"DMA3_EXEC_COUNT(08-11)",
				readl(sfr + DMA3_EXEC_COUNT8),
				readl(sfr + DMA3_EXEC_COUNT9),
				readl(sfr + DMA3_EXEC_COUNT10),
				readl(sfr + DMA3_EXEC_COUNT11));
		count += sprintf(buf + count, "%24s:%8x %8x %8x %8x\n",
				"DMA3_EXEC_COUNT(12-15)",
				readl(sfr + DMA3_EXEC_COUNT12),
				readl(sfr + DMA3_EXEC_COUNT13),
				readl(sfr + DMA3_EXEC_COUNT14),
				readl(sfr + DMA3_EXEC_COUNT15));
	}

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
		score_info("[sysfs] dpmu on\n");
		score_dpmu_debug_control(1);
		score_dpmu_performance_control(1);
	} else if (sysfs_streq(buf, "off")) {
		score_info("[sysfs] dpmu off\n");
		score_dpmu_debug_control(0);
		score_dpmu_performance_control(0);
	} else if (sysfs_streq(buf, "debug_on")) {
		score_info("[sysfs] debug on\n");
		score_dpmu_debug_control(1);
	} else if (sysfs_streq(buf, "debug_off")) {
		score_info("[sysfs] debug off\n");
		score_dpmu_debug_control(0);
	} else if (sysfs_streq(buf, "performance_on")) {
		score_info("[sysfs] performance on\n");
		score_dpmu_performance_control(1);
	} else if (sysfs_streq(buf, "performance_off")) {
		score_info("[sysfs] performance off\n");
		score_dpmu_performance_control(0);
	} else {
		score_info("[sysfs] dpmu invalid command\n");
	}
p_unlock:
	mutex_unlock(&pm->lock);
	score_leave();
p_end:
	return count;
}

static ssize_t show_sysfs_system_userdefined(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	struct score_device *device;
	struct score_pm *pm;
	void __iomem *sfr;
	int idx;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	pm = &device->pm;
	sfr = device->system.sfr;

	mutex_lock(&pm->lock);
	if (!score_pm_qos_active(pm)) {
		count += sprintf(buf + count, "power off\n");
		goto p_unlock;
	}

	sfr = device->system.sfr;

	for (idx = 0; idx < USERDEFINED_COUNT; idx += 4)
		count += sprintf(buf + count,
				"%18s %02d-%02d:%8x %8x %8x %8x\n",
				"USERDEFINED", idx, idx + 3,
				readl(sfr + USERDEFINED(idx)),
				readl(sfr + USERDEFINED(idx + 1)),
				readl(sfr + USERDEFINED(idx + 2)),
				readl(sfr + USERDEFINED(idx + 3)));

p_unlock:
	mutex_unlock(&pm->lock);
	score_leave();
p_end:
	return count;
}

static ssize_t store_sysfs_system_userdefined(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct score_device *device;
	struct score_pm *pm;
	int num = -1;
	unsigned int value = 0xdeadc0de;
	unsigned int old;
	int ret;

	score_enter();
	device = dev_get_drvdata(dev);
	if (!device)
		goto p_end;

	ret = sscanf(buf, "%d 0x%x", &num, &value);
	if ((ret != 2) || (num < 0) || (num >= USERDEFINED_COUNT)) {
		score_info("[sysfs] invalid value (%d/%8x)\n", num, value);
		goto p_end;
	}

	pm = &device->pm;
	mutex_lock(&pm->lock);
	if (!score_pm_qos_active(pm)) {
		score_info("[sysfs] Failed to write on userdefined sfr\n");
		goto p_unlock;
	}

	old = readl(device->system.sfr + USERDEFINED(num));
	writel(value, device->system.sfr + USERDEFINED(num));
	score_info("[sysfs] USERDEFINED(%d) is updated from %#x to %#x\n",
			num, old, value);

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
	count += sprintf(buf + count, "%u/%u/%u/%u/%u\n",
			scq->high_count, scq->normal_count, scq->count,
			scq->high_max_count, scq->normal_max_count);
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
	count += sprintf(buf + count, "all:%d, normal:%d, abnormal:%d\n",
			framemgr->all_count, framemgr->normal_count,
			framemgr->abnormal_count);
	count += sprintf(buf + count, "entire:%d, ready:%d, ",
			framemgr->entire_count, framemgr->ready_count);
	count += sprintf(buf + count, "process:%d(H:%d/N:%d), ",
			framemgr->process_count, framemgr->process_high_count,
			framemgr->process_normal_count);
	count += sprintf(buf + count, "pending:%d(H:%d/N:%d), complete:%d\n",
			framemgr->pending_count, framemgr->pending_high_count,
			framemgr->pending_normal_count,
			framemgr->complete_count);

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

static ssize_t show_sysfs_system_asb(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t store_sysfs_system_asb(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
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
static DEVICE_ATTR(userdefined, 0644,
		show_sysfs_system_userdefined,
		store_sysfs_system_userdefined);
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
	&dev_attr_userdefined.attr,
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
