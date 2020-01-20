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

#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/delay.h>

#include "score_log.h"
#include "score_dpmu.h"
#include "score_regs.h"
#include "score_frame.h"
#include "score_scq.h"
#include "score_system.h"
#include "score_interface.h"

/* Number of interrupt status */
enum score_interrupt_num {
	INT_SCQ,
	INT_RESERVED,
	INT_SWI,
	INT_SR,
	INT_INVALID
};

/* Number of debug error status */
enum score_dbgerr_num {
	DBGERR_DECERR,
	DBGERR_SCQ_SLVERR,
	DBGERR_TCM_BUS_SLVERR,
	DBGERR_CACHE_SLVERR,
	DBGERR_IBC_SLVERR,
	DBGERR_AXI_SLVERR,
	DBGERR_DSP_CORE_SLVERR
};

/* Number of target swi */
enum score_target_swi_num {
	SWI_CORE_HALT
};

/* Definition for checking debug error of swi */
#define DBG_ERR_INTR_STATUS		(0x7f)

/* Definition for interrupt bit */
#define INTERRUPT_BIT(X)		(0x1 << (X))

/*
 * If it is determined that the core is not halted, check again after delay time
 *   The unit is microsecond
 */
#define SCORE_HALT_CHECK_DELAY  (10)
/*
 * If it is determined that the core is not halted, check again 'retry number'
 * times. 'retry number' is 1 ms divided by delay time
 */
#define SCORE_HALT_RETRY        (1000/SCORE_HALT_CHECK_DELAY)

/* TODO this is temp code */
static struct timespec isr_time;

int score_interface_target_halt(struct score_interface *itf, int core_id)
{
	int ret = 0;
	int cache = 0, interrupt = 0;
	int idx;

	score_enter();
	if (core_id == SCORE_MASTER) {
		writel(INTERRUPT_BIT(SWI_CORE_HALT), itf->sfr + MC_SWI_STATUS);

		for (idx = 0; idx < SCORE_HALT_RETRY; ++idx) {
			cache = readl(itf->sfr + MC_CACHE_STATUS);
			interrupt = readl(itf->sfr + MC_INTR_ENABLE) & 0x3ff;
			if (!cache && !interrupt)
				break;
			udelay(SCORE_HALT_CHECK_DELAY);
		}
	} else if (core_id == SCORE_KNIGHT1) {
		writel(INTERRUPT_BIT(SWI_CORE_HALT), itf->sfr + KC1_SWI_STATUS);

		for (idx = 0; idx < SCORE_HALT_RETRY; ++idx) {
			cache = readl(itf->sfr + KC1_CACHE_STATUS);
			interrupt = readl(itf->sfr + KC1_INTR_ENABLE) & 0xff;
			if (!cache && !interrupt)
				break;
			udelay(SCORE_HALT_CHECK_DELAY);
		}
	} else if (core_id == SCORE_KNIGHT2) {
		writel(INTERRUPT_BIT(SWI_CORE_HALT), itf->sfr + KC2_SWI_STATUS);

		for (idx = 0; idx < SCORE_HALT_RETRY; ++idx) {
			cache = readl(itf->sfr + KC2_CACHE_STATUS);
			interrupt = readl(itf->sfr + KC2_INTR_ENABLE) & 0xff;
			if (!cache && !interrupt)
				break;
			udelay(SCORE_HALT_CHECK_DELAY);
		}
	} else {
		ret = -EINVAL;
		score_err("Halt failed as core_id(%d) is invalid\n", core_id);
		goto p_err;
	}
	if (cache || interrupt) {
		score_warn("cache(%d) status is unstable (%#x,%#x)\n",
				core_id, cache, interrupt);
		ret = -ETIMEDOUT;
	}

	score_leave();
p_err:
	return ret;
}

static void __score_interface_scq_read(struct score_interface *itf)
{
	int ret;
	unsigned long flags;
	struct score_scq *scq;
	struct score_frame_manager *fmgr;
	struct score_frame *frame, temp, *pending_frame;

	score_enter();
	scq = &itf->system->scq;
	fmgr = &itf->framemgr;

	spin_lock_irqsave(&fmgr->slock, flags);
	ret = score_scq_read(scq, &temp);
	if (ret) {
		spin_unlock_irqrestore(&fmgr->slock, flags);
		return;
	}

	frame = score_frame_get_process_by_id(fmgr, temp.frame_id);
	if (!frame) {
		spin_unlock_irqrestore(&fmgr->slock, flags);
		return;
	}

	frame->timestamp[SCORE_TIME_ISR] = isr_time;
	score_frame_trans_process_to_complete(frame, temp.ret);
	if (frame->ret) {
		score_warn("result(%d) of target is unstable (%d/%d/%d)\n",
				ret, fmgr->all_count, fmgr->normal_count,
				fmgr->abnormal_count);
	}
	if (score_frame_check_type(frame, TYPE_NONBLOCK_NOWAIT))
		score_frame_set_type_remove(frame);

	spin_unlock_irqrestore(&fmgr->slock, flags);

	if (score_frame_check_type(frame, TYPE_NONBLOCK_REMOVE))
		score_frame_destroy(frame);
	else
		wake_up(&fmgr->done_wq);

	spin_lock_irqsave(&fmgr->slock, flags);
	if (score_frame_get_pending_count(fmgr, false)) {
		pending_frame = score_frame_get_first_pending(fmgr, false);
		if (pending_frame) {
			score_frame_trans_pending_to_ready(pending_frame);
			kthread_queue_work(&scq->write_worker,
					&pending_frame->work);
		}
	}
	spin_unlock_irqrestore(&fmgr->slock, flags);

	score_leave();
}

static inline int __score_interface_interrupt_check(struct score_interface *itf)
{
	return readl(itf->sfr + APCPU_MASKED_INTR_STATUS);
}

static void __score_interface_check_swi(struct score_interface *itf)
{
	int swi_status;
	int error_info;

	score_enter();
	swi_status = readl(itf->sfr + APCPU_SWI_STATUS);

	if (swi_status & INTERRUPT_BIT(DBGERR_DECERR)) {
		score_err("Decode error occurred (%#x)\n", swi_status);
		score_err("MC Decode error address %#x\n",
				readl(itf->sfr + MC_DECERR_INFO));
		score_err("KC1 Decode error address %#x\n",
				readl(itf->sfr + KC1_DECERR_INFO));
		score_err("KC2 Decode error address %#x\n",
				readl(itf->sfr + KC2_DECERR_INFO));
	}

	if (swi_status & INTERRUPT_BIT(DBGERR_SCQ_SLVERR)) {
		error_info = readl(itf->sfr + SCQ_SLVERR_INFO);
		score_err("SCQ error occurred (%#x/%#x)\n",
			swi_status, error_info);
		score_err("Core id %#x, PC of DSP core(28bit) %#x\n",
			error_info & 0x7, error_info >> 4);
	}

	if (swi_status & INTERRUPT_BIT(DBGERR_TCM_BUS_SLVERR)) {
		error_info = readl(itf->sfr + SCQ_SLVERR_INFO);
		score_err("TCM_BUS error occurred (%#x/%#x)\n",
				swi_status, error_info);
		score_err("Core id %#x, TYPE %#x, ADDR %#x\n",
				error_info & 0x7, (error_info >> 3) & 0x1,
				error_info >> 4);
	}

	if (swi_status & INTERRUPT_BIT(DBGERR_CACHE_SLVERR)) {
		error_info = readl(itf->sfr + CACHE_SLVERR_INFO);
		score_err("CACHE error occurred (%#x/%#x)\n",
				swi_status, error_info);
		score_err("Core id %#x, TYPE %#x, ADDR %#x\n",
				error_info & 0x3, (error_info >> 2) & 0x7,
				error_info >> 5);
	}

	if (swi_status & INTERRUPT_BIT(DBGERR_IBC_SLVERR)) {
		error_info = readl(itf->sfr + IBC_SLVERR_INFO);
		score_err("IBC error occurred (%#x/%#x)\n",
				swi_status, error_info);
		score_err("Channel %1x, Direction %1x\n",
				error_info & 0x3, (error_info >> 2) & 0x1);
	}

	if (swi_status & INTERRUPT_BIT(DBGERR_AXI_SLVERR)) {
		error_info = readl(itf->sfr + AXI_SLVERR_INFO);
		score_err("AXI error occurred (%#x/%#x)\n",
				swi_status, error_info);
		score_err("Port %#x, Addr %#x\n",
				error_info & 0x3, error_info >> 4);
	}

	if (swi_status & INTERRUPT_BIT(DBGERR_DSP_CORE_SLVERR)) {
		error_info = readl(itf->sfr + DSP_SLVERR_INFO);
		score_err("DSP_CORE error occurred (%#x/%#x)\n",
				swi_status, error_info);
		score_err("Core id %#x, TYPE %#x, ADDR %#x\n",
				error_info & 0x3, (error_info >> 2) & 0x1,
				error_info >> 4);
	}

	if (swi_status & DBG_ERR_INTR_STATUS)
		score_dpmu_print_all();

	score_leave();
}

static void __score_interface_isr(struct score_interface *itf, int interrupt)
{
	struct score_system *system;

	score_enter();
	system = itf->system;

	if (interrupt & INTERRUPT_BIT(INT_SWI)) {
		__score_interface_check_swi(itf);
	} else if (interrupt & INTERRUPT_BIT(INT_SCQ)) {
		/*
		 * Because interrupt must be cleared before reading data
		 * from SCQ, SCQ interrupt is cleared first.
		 */
		writel(0x1, itf->sfr + SCQ_INTR_CLEAR);
		__score_interface_scq_read(itf);
	} else if (interrupt & INTERRUPT_BIT(INT_RESERVED)) {
		score_err("reserved interrupt occurred (%#x)\n", interrupt);
	} else if (interrupt & INTERRUPT_BIT(INT_SR)) {
		score_err("sr interrupt occurred (%#x)\n", interrupt);
	} else {
		score_err("invalid interrupt occurred (%#x)\n", interrupt);
	}
	score_leave();
}

static void __score_interface_interrupt_clear(struct score_interface *itf,
		int interrupt)
{
	struct score_frame_manager *fmgr;
	unsigned long flags;
	int swi_status;

	score_enter();

	if (interrupt & INTERRUPT_BIT(INT_SWI)) {
		swi_status = readl(itf->sfr + APCPU_SWI_STATUS);
		if (swi_status & DBG_ERR_INTR_STATUS) {
			/*
			 * If debug error occurs,
			 * do sw_reset, not interrupt clear.
			 */
			fmgr = &itf->framemgr;
			spin_lock_irqsave(&fmgr->slock, flags);
			score_frame_flush_process(fmgr, -EINTR);
			wake_up_all(&fmgr->done_wq);
			score_system_halt(itf->system);
			score_system_boot(itf->system);
			spin_unlock_irqrestore(&fmgr->slock, flags);
		}
	}

	score_leave();
}

static irqreturn_t score_interface_isr(int irq, void *data)
{
	struct score_interface *itf = (struct score_interface *)data;
	int interrupt;

	score_enter();
	isr_time = score_util_get_timespec();

	interrupt = __score_interface_interrupt_check(itf);

	__score_interface_isr(itf, interrupt);
	__score_interface_interrupt_clear(itf, interrupt);

	score_leave();
	return IRQ_HANDLED;
}

#if defined(CONFIG_EXYNOS_SCORE_FPGA)
static bool isr_detector_work;
static void score_interface_interrupt_detector(struct kthread_work *work)
{
	struct score_interface *itf = container_of(work,
			struct score_interface, isr_work);

	while (isr_detector_work) {
		if (readl(itf->sfr + APCPU_MASKED_INTR_STATUS))
			score_interface_isr(0, itf);
		else
			usleep_range(1000, 2000);
	}
}
#endif

int score_interface_open(struct score_interface *itf)
{
	score_enter();
#if defined(CONFIG_EXYNOS_SCORE_FPGA)
	isr_detector_work = true;
	kthread_queue_work(&itf->isr_worker, &itf->isr_work);
#endif
	score_leave();
	return 0;
}

void score_interface_close(struct score_interface *itf)
{
	score_enter();
#if defined(CONFIG_EXYNOS_SCORE_FPGA)
	isr_detector_work = false;
#endif
	score_leave();
}

int score_interface_probe(struct score_system *system)
{
	int ret = 0;
	struct score_interface *itf;
	struct platform_device *pdev;
#if !defined(CONFIG_EXYNOS_SCORE_FPGA)
	int irq;
#endif

	score_enter();
	itf = &system->interface;
	itf->system = system;
	pdev = to_platform_device(system->dev);

	itf->sfr = system->sfr;
#if !defined(CONFIG_EXYNOS_SCORE_FPGA)
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		score_err("platform_get_irq(0) is fail(%d)", irq);
		ret = -EINVAL;
		goto p_err_irq;
	}

	itf->irq0 = irq;
	itf->dev = system->dev;

	ret = devm_request_irq(itf->dev, itf->irq0,
			score_interface_isr, 0, dev_name(itf->dev),
			itf);
	if (ret) {
		score_err("devm_request_irq(0) is fail(%d)\n", ret);
		goto p_err_irq;
	}
#else
	kthread_init_worker(&itf->isr_worker);
	itf->isr_task = kthread_run(kthread_worker_fn,
			&itf->isr_worker, "score_isr_detector");
	if (IS_ERR(itf->isr_task)) {
		ret = PTR_ERR(itf->isr_task);
		score_err("isr_detector kthread is not running (%d)\n", ret);
		goto p_err_irq;
	}
	kthread_init_work(&itf->isr_work,
			score_interface_interrupt_detector);
#endif
	ret = score_frame_manager_probe(&itf->framemgr);
	if (ret)
		goto p_err_framemgr;

	score_leave();
	return ret;
p_err_framemgr:
#if !defined(CONFIG_EXYNOS_SCORE_FPGA)
	devm_free_irq(itf->dev, itf->irq0, itf);
#else
	kthread_stop(itf->isr_task);
#endif
p_err_irq:
	return ret;
}

void score_interface_remove(struct score_interface *itf)
{
	score_enter();
	score_frame_manager_remove(&itf->framemgr);
#if !defined(CONFIG_EXYNOS_SCORE_FPGA)
	devm_free_irq(itf->dev, itf->irq0, itf);
#else
	kthread_stop(itf->isr_task);
#endif
	score_leave();
}
