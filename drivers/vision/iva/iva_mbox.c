/* iva_mbox.c
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 * Authors:
 *	Ilkwan Kim <ilkwan.kim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/rtc.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/uaccess.h>

#include "iva_ctrl_ioctl.h"
#include "iva_mbox.h"
#include "regs/iva_base_addr.h"
#include "regs/iva_mbox_reg.h"

enum mbox_id {
	mbox_id_mcu	= 0,
	mbox_id_dsp,
	mbox_id_max
};

enum mbox_irq_type {
	mbox_irq_pend	= (0x1 << 0),
	mbox_irq_wrdy	= (0x1 << 1)
};


static inline bool iva_mbox_pending_mail_from_mcu(struct iva_dev_data *iva)
{
	uint32_t val = readl(iva->mbox_va + MBOX_IRQ_RAW_STATUS_ADDR);

	val = (val >> MBOX_IRQ_RAW_STATUS_M0_AP_PEND_INTR_STAT_S) &
				MBOX_IRQ_RAW_STATUS_M;

	dev_dbg(iva->dev, "%s() val(0x%x)\n", __func__, val);

	if (val)
		return true;
	else
		return false;
}

static inline void iva_mbox_clear_pending_mail_from_mcu(struct iva_dev_data *iva)
{
	uint32_t val = MBOX_IRQ_RAW_STATUS_M <<
			MBOX_IRQ_RAW_STATUS_M0_AP_PEND_INTR_STAT_S;
	writel(val, iva->mbox_va + MBOX_IRQ_RAW_STATUS_ADDR);
}

static inline uint32_t iva_mbox_read_mail_from_mcu(struct iva_dev_data *iva)
{
	return readl(iva->mbox_va + MBOX_MESSAGE_BASE_ADDR + M0_AP_OFFSET);
}

static void iva_mbox_initialize(struct iva_dev_data *iva)
{
	uint32_t        reg;
	uint32_t	val;
	void __iomem	*mbox_va = iva->mbox_va;

	/*
	 * clear irq enable bits : not clear automatically.
	 */
	val = readl(mbox_va + MBOX_IRQ_EN_ADDR);
	writel(val, mbox_va + MBOX_IRQ_EN_ADDR);

	/* mcu <-> ap, mcu <-> dsp, dsp <-> ap */
	val = (MBOX_CMD_VMCU_RESET_M << MBOX_CMD_VMCU_RESET_S);
	for (reg = MBOX_CMD_M0_VMCU_ADDR; reg <= MBOX_M1_AP_VMCU_ADDR;
			reg += (uint32_t) sizeof(uint32_t)) {
		/* reset the write/read pointers */
		writel(0, mbox_va + reg);

		/* release the write/read pointers */
		writel(val, mbox_va + reg);
	}

	/* reset/clear the pending interrupts */
	writel(0xffff, mbox_va + MBOX_IRQ_RAW_STATUS_ADDR);

	val = readl(mbox_va + MBOX_IRQ_RAW_STATUS_ADDR);
	dev_dbg(iva->dev, "mbox irq_raw_st (0x%08x)\n", val);

}

static int iva_mbox_set_mbox_irq_en(struct iva_dev_data *iva,
		enum mbox_id m_id, enum mbox_irq_type irq_type, bool enable)
{
	uint32_t	val;
	uint32_t	mask = 0x0;
	struct device	*dev = iva->dev;
	void __iomem	*mbox_va = iva->mbox_va;

	if (m_id == mbox_id_mcu) {
		if (irq_type & mbox_irq_pend)
			mask |= MBOX_M0_AP_PEND_IRQ_EN_M;
		if (irq_type & mbox_irq_wrdy)
			mask |= MBOX_M0_AP_WRDY_IRQ_EN_M;
	} else if (m_id == mbox_id_dsp) {
		if (irq_type & mbox_irq_pend)
			mask |= MBOX_M1_AP_PEND_IRQ_EN_M;
		if (irq_type & mbox_irq_wrdy)
			mask |= MBOX_M1_AP_WRDY_IRQ_EN_M;
	} else {
		dev_info(dev, "%s() m_id(%d) can not be handled.\n",
					__func__, m_id);
		return -EINVAL;
	}

	val = readl(mbox_va + MBOX_IRQ_EN_ADDR);
	if (enable)
		val = (~val) & mask;
	else
		val &= mask;
	writel(val, mbox_va + MBOX_IRQ_EN_ADDR);

#ifdef DEBUG
	val = readl(mbox_va + MBOX_IRQ_EN_ADDR);
	dev_dbg(dev, "%s() final mbox irq en(0x%x)\n", __func__, val);
#endif
	return 0;

}

#define MAX_PENDING_MSG_SIZE	(4)
#define GET_MBOX_MSG_SIZE(val) \
	(((val) >> MBOX_STATUS_0_OCC_M0_VMCU_S) & MBOX_STATUS_0_OCC_M0_VMCU_M)
static int iva_mbox_mcu_queue_full(struct iva_dev_data *iva, uint32_t n_retry,
		uint32_t msleep_range)
{
	volatile uint32_t	val;
	uint32_t		i;
	void __iomem		*mbox_va = iva->mbox_va;

	for (i = 0; i < n_retry; i++) {
		if (msleep_range)
			msleep(msleep_range);
		val = readl(mbox_va + MBOX_STATUS_0_ADDR);
		/* check mbox mcu queue is available or full. */
		if (GET_MBOX_MSG_SIZE(val) < MAX_PENDING_MSG_SIZE)
			return 0;
	}
	return -1;
}

#define MAX_RETRY_WO_DELAY	(200)
#define MAX_RETRY_W_DELAY	(5)
#define MBOX_DELAY		(2)
int iva_mbox_send_mail_to_mcu(struct iva_dev_data *iva, uint32_t msg)
{
	void __iomem	*mbox_va = iva->mbox_va;

	if (!mbox_va) {
		dev_err(iva->dev, "mbox disabled. iva_state(0x%lx)\n",
				iva->state);
		return -ENODEV;
	}

	if (iva_mbox_mcu_queue_full(iva, MAX_RETRY_WO_DELAY, 0)) {
		if (iva_mbox_mcu_queue_full(iva, MAX_RETRY_W_DELAY, MBOX_DELAY)) {
			dev_err(iva->dev, "%s() Fail to get MCU mbox slot.\n",
					__func__);
			return -EINVAL;
		}
	}

	writel(msg, mbox_va + MBOX_MESSAGE_BASE_ADDR + M0_VMCU_OFFSET);
	dev_dbg(iva->dev, "%s() msg(0x%x) raw(0x%x)\n", __func__,
			msg, readl(mbox_va + MBOX_IRQ_RAW_STATUS_ADDR));

	return 0;
}

int iva_mbox_send_mail_to_mcu_usr(struct iva_dev_data *iva, uint32_t __user *msg_usr)
{
	uint32_t msg;

	get_user(msg, msg_usr);

	return iva_mbox_send_mail_to_mcu(iva, msg);
}

/* 8 MSBs out of msg are used for callbacks : sync with iva f/w*/
#define IVA_MBOX_CB_MSB_S		(24)	/* header */
#define IVA_MBOX_CB_MSB_M		(0xFF)
#define IVA_MBOX_CB_VALUE_S		(0)	/* payload */
#define IVA_MBOX_CB_VALUE_M		(0xFFFFFF)

struct iva_mbox_cb_head {
	spinlock_t			lock;
	struct iva_mbox_cb_block __rcu	*head;
};


#define IVA_MBOX_CB_HEAD(name)					\
	struct iva_mbox_cb_head name = {			\
		.lock = __SPIN_LOCK_UNLOCKED(name.lock),	\
		.head = NULL					\
	}


static IVA_MBOX_CB_HEAD(iva_mbox_chain);

/* return number of call backs which is actually called */
static int iva_mbox_call_cb_chain(uint32_t msg)
{
	struct iva_mbox_cb_head *ch = &iva_mbox_chain;
	struct iva_mbox_cb_block **cl = &ch->head;
	struct iva_mbox_cb_block *cb = NULL, *next_cb;
	int32_t		cb_count = 0;
	uint32_t	msg_hd = (msg >> IVA_MBOX_CB_MSB_S) & IVA_MBOX_CB_MSB_M;
	int		cb_ret;

	rcu_read_lock();
	cb = rcu_dereference_raw(*cl);
	while (cb) {
		next_cb = rcu_dereference_raw(cb->next);
		if (cb->msg_hd == msg_hd) {
			cb_ret = cb->callback(cb, msg);
			cb_count++;
			if (cb_ret)	/* stop transfer msg further*/
				goto out;
		}
		cb = next_cb;
	}
out:
	rcu_read_unlock();
	return cb_count;
}


int iva_mbox_callback_register(uint8_t msg_hd, struct iva_mbox_cb_block *cb)
{
	struct iva_mbox_cb_head *ch = &iva_mbox_chain;
	struct iva_mbox_cb_block **cl = &ch->head;
	unsigned long flags;

	if (!cb || !cb->callback)
		return -EINVAL;

	spin_lock_irqsave(&ch->lock, flags);
	while ((*cl) != NULL) {
		if ((*cl) == cb)
			return 0;
		if (cb->priority > (*cl)->priority)
			break;
		cl = &((*cl)->next);
	}
	cb->msg_hd	= msg_hd;
	cb->next	= *cl;

	rcu_assign_pointer(*cl, cb);
	spin_unlock_irqrestore(&ch->lock, flags);

	return 0;
}


int iva_mbox_callback_unregister(struct iva_mbox_cb_block *cb)
{
	struct iva_mbox_cb_head *ch = &iva_mbox_chain;
	struct iva_mbox_cb_block **cl = &ch->head;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&ch->lock, flags);
	while ((*cl) != NULL) {
		if ((*cl) == cb) {
			rcu_assign_pointer(*cl, cb->next);
			goto out;
		}
		cl = &((*cl)->next);
	}
	ret = -ENOENT;
out:
	spin_unlock_irqrestore(&ch->lock, flags);
	synchronize_rcu();
	return ret;
}

static inline void __iva_mbox_handle_irq(struct iva_dev_data *iva)
{
	uint32_t msg;

	if (!iva_mbox_pending_mail_from_mcu(iva))
		return;

	/* read msg and clear intr */
	msg = iva_mbox_read_mail_from_mcu(iva);
	iva_mbox_clear_pending_mail_from_mcu(iva);

	dev_dbg(iva->dev, "%s() iva(%p) cb->msg(0x%x)\n", __func__, iva, msg);

	/* transfer it to customer */
	iva_mbox_call_cb_chain(msg);
}


/* polling mode case */
static void iva_mbox_monitor_intr(struct work_struct *work)
{
	struct delayed_work	*m_dw = to_delayed_work(work);
	struct iva_dev_data	*iva = container_of(m_dw,
			struct iva_dev_data, mbox_dwork);

	__iva_mbox_handle_irq(iva);

	queue_delayed_work(system_wq, m_dw, msecs_to_jiffies(1));
}

static void iva_mbox_monitor_start(struct iva_dev_data *iva)
{
	struct delayed_work *m_dw = &iva->mbox_dwork;

	INIT_DELAYED_WORK(m_dw, iva_mbox_monitor_intr);
	queue_delayed_work(system_wq, m_dw, msecs_to_jiffies(1));
}

static void iva_mbox_monitor_stop(struct iva_dev_data *iva)
{
	cancel_delayed_work_sync(&iva->mbox_dwork);
	dev_dbg(iva->dev, "%s()\n", __func__);
}

/* interrupt mode case */
static irqreturn_t iva_mbox_irq_handler(int irq, void *dev_id)
{
	struct iva_dev_data *iva = (struct iva_dev_data *)dev_id;

	dev_dbg(iva->dev, "%s()\n", __func__);

	__iva_mbox_handle_irq(iva);

	return IRQ_HANDLED;
}


int iva_mbox_init(struct iva_dev_data *iva)
{
	struct device *dev = iva->dev;

	iva_mbox_initialize(iva);

	/* polling mode case */
	if (iva->mbox_irq_nr == IVA_MBOX_IRQ_NOT_DEFINED) {
		iva_mbox_monitor_start(iva);
		dev_dbg(dev, "%s() start polling mode\n", __func__);
	} else {
		iva_mbox_set_mbox_irq_en(iva, mbox_id_mcu,
				mbox_irq_pend | mbox_irq_wrdy, true);
		enable_irq(iva->mbox_irq_nr);
	}

	return 0;
}


void iva_mbox_deinit(struct iva_dev_data *iva)
{
	struct device *dev = iva->dev;

	if (iva->mbox_irq_nr == IVA_MBOX_IRQ_NOT_DEFINED) {
		iva_mbox_monitor_stop(iva);
		dev_dbg(dev, "%s() stop polling mode\n", __func__);
	} else {
		disable_irq(iva->mbox_irq_nr);
		iva_mbox_set_mbox_irq_en(iva, mbox_id_mcu,
				mbox_irq_pend | mbox_irq_wrdy, false);
	}
}


int iva_mbox_probe(struct iva_dev_data *iva)
{
	struct device	*dev = iva->dev;
	int		ret = 0;

	if (iva->mbox_va) {
		dev_err(dev, "%s() already mapped into mbox_va(%p)\n", __func__,
				iva->mbox_va);
		return 0;
	}

	if (!iva->iva_va) {
		dev_err(dev, "%s() null iva_va\n", __func__);
		return -EINVAL;
	}

	/* interrupt mode case */
	if (iva->mbox_irq_nr != IVA_MBOX_IRQ_NOT_DEFINED) {
		irq_set_status_flags(iva->mbox_irq_nr, IRQ_NOAUTOEN);
		ret = devm_request_irq(dev, iva->mbox_irq_nr,
					iva_mbox_irq_handler,
					IRQF_TRIGGER_NONE,
					"iva_mbox", iva);
		if (ret) {
			dev_warn(dev, "irq request failed(%d)->forced polling\n",
					ret);
			/* force to poll */
			iva->mbox_irq_nr = IVA_MBOX_IRQ_NOT_DEFINED;
		} else {
			dev_dbg(dev,
				"%s() succeed to request irq(%d) and disable\n",
				__func__, iva->mbox_irq_nr);
		}
	}

	iva->mbox_va = iva->iva_va + IVA_MBOX_BASE_ADDR;

	dev_dbg(dev, "%s() iva(%p) succeed to map iva mbox reg(%p)\n",
			__func__, iva, iva->mbox_va);

	return 0;

}

void iva_mbox_remove(struct iva_dev_data *iva)
{
	struct device	*dev = iva->dev;

	if (iva->mbox_irq_nr != IVA_MBOX_IRQ_NOT_DEFINED) {
		devm_free_irq(dev, iva->mbox_irq_nr, iva);
		dev_dbg(dev, "%s() free mbox irq\n", __func__);
	}

	if (iva->mbox_va)
		iva->mbox_va = NULL;

	dev_dbg(iva->dev, "%s() succeed to release mbox resources\n", __func__);
}
