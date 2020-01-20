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
	INT_SWI,
	INT_SR,
	INT_TS_CACHE,
	INT_BARON_CACHE,
	INT_INVALID
};

/* Number of APCPU swi */
enum score_swi_num {
	APCPU_SWI_DBGERR,
	APCPU_SWI_END = 31
};

/* Number of target swi */
enum score_target_swi_num {
	SWI_CORE_HALT,
	SWI_END = 7
};

/* Definition for interrupt bit */
#define INTERRUPT_BIT(X)		(0x1 << (X))

int score_interface_target_halt(struct score_interface *itf, int core_id)
{
	int ret = 0;
	int cache, intr1, intr2 = 0;
	int idx;

	score_enter();
	if (core_id == SCORE_TS) {
		writel(INTERRUPT_BIT(SWI_CORE_HALT), itf->sfr + TS_SWI_STATUS);

		for (idx = 0; idx < CHECK_RETRY_COUNT; ++idx) {
			cache = readl(itf->sfr + TS_CACHE_STATUS);
			intr1 = readl(itf->sfr + TS_INTR_ENABLE);
			if (!(cache || intr1))
				break;
			udelay(CHECK_RETRY_DELAY);
		}
	} else if (core_id == SCORE_BARON) {
		writel(INTERRUPT_BIT(SWI_CORE_HALT), itf->sfr + BR1_SWI_STATUS);
		writel(INTERRUPT_BIT(SWI_CORE_HALT), itf->sfr + BR2_SWI_STATUS);

		for (idx = 0; idx < CHECK_RETRY_COUNT; ++idx) {
			cache = readl(itf->sfr + BR_CACHE_STATUS);
			intr1 = readl(itf->sfr + BR1_INTR_ENABLE);
			intr2 = readl(itf->sfr + BR2_INTR_ENABLE);
			if (!(cache || intr1 || intr2))
				break;
			udelay(CHECK_RETRY_DELAY);
		}
	} else {
		ret = -EINVAL;
		score_err("Halt failed. core_id(%d) is invalid\n", core_id);
		goto p_err;
	}

	if (cache || intr1 || intr2) {
		score_warn("Halt failed. core(%d) is unstable (%#x/%#x/%#x)\n",
				core_id, cache, intr1, intr2);
		ret = -ETIMEDOUT;
		goto p_err;
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
	bool priority;

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
		priority = temp.priority;
		goto pending_check;
	}

	if (frame->priority != temp.priority)
		score_warn("priority is unstable (%d/%d)\n",
				frame->priority, temp.priority);
	priority = frame->priority;

	frame->timestamp[SCORE_TIME_ISR] = itf->start_time;
	score_frame_trans_process_to_complete(frame, temp.ret);
	if (frame->ret) {
		score_warn("result(%d) of target(%u) is unstable(%u/%u/%u)\n",
				frame->ret, frame->kernel_id,
				fmgr->all_count, fmgr->normal_count,
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

pending_check:
	if (score_frame_get_pending_count(fmgr, priority)) {
		pending_frame = score_frame_get_first_pending(fmgr, priority);
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

	score_enter();
	swi_status = readl(itf->sfr + APCPU_SWI_STATUS);
	if (swi_status & INTERRUPT_BIT(APCPU_SWI_DBGERR)) {
		score_err("DBGERR interrupt occurred (%#x)\n", swi_status);
		score_dpmu_print_system();
		score_dpmu_print_debug();
		score_dpmu_print_target();
		score_dpmu_print_userdefined();
	} else {
		score_warn("Not supported APCPU SWI (%#x)\n", swi_status);
	}
}

static void __score_interface_isr(struct score_interface *itf, int interrupt)
{
	struct score_system *system;

	score_enter();
	system = itf->system;

	if (interrupt & INTERRUPT_BIT(INT_SCQ)) {
		/*
		 * Because interrupt must be cleared before reading data
		 * from SCQ, SCQ interrupt is cleared first.
		 */
		writel(0x1, itf->sfr + SCQ_INTR_CLEAR);
		__score_interface_scq_read(itf);
	} else if (interrupt & INTERRUPT_BIT(INT_SWI)) {
		__score_interface_check_swi(itf);
	} else if (interrupt & INTERRUPT_BIT(INT_SR)) {
		score_warn("sr interrupt occurred (%#x)\n", interrupt);
	} else if (interrupt & INTERRUPT_BIT(INT_TS_CACHE)) {
		score_warn("ts cache interrupt occurred (%#x)\n", interrupt);
	} else if (interrupt & INTERRUPT_BIT(INT_BARON_CACHE)) {
		score_warn("baron cache interrupt occurred (%#x)\n", interrupt);
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
		if (swi_status & INTERRUPT_BIT(APCPU_SWI_DBGERR)) {
			fmgr = &itf->framemgr;
			spin_lock_irqsave(&fmgr->slock, flags);
			score_frame_flush_process(fmgr, -EINTR);
			wake_up_all(&fmgr->done_wq);
#if defined(SCORE_EVT0)
			score_system_halt(itf->system);
#endif
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
	itf->start_time = score_util_get_timespec();

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
