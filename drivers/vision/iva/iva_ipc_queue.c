/* iva_ipc_queue.c
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
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/sched/clock.h>

#include "iva_ipc_header.h"
#include "iva_mcu.h"
#include "iva_mbox.h"
#include "iva_ipc_param_ll.h"
#include "iva_ipc_queue.h"
#include "iva_sh_mem.h"
#include "iva_ram_dump.h"
#include "iva_vdma.h"
#include "iva_pmu.h"

#undef DEBUG

#undef CHECK_PARAM_REGION_ALIGN
#undef ALLOW_SIGNAL_RECEPTION

#define IPCQ_CMD_ALLOC_RETRY_CNT	(3)
#define IPCQ_CMD_ALLOC_SLEEP_MS		(10)

#define IPCQ_TIMEOUT_MS			(2000)

#define WAIT_DELAY_US			(1000)
#define VALUE_CHECK_DECREMENT	(10)	/* us */

#define CMD_PARAM_TO_LL_CMD_PARAM(c)	\
		(container_of(c, struct _ipcq_cmd_param_ll, cmd_param))
#define RES_PARAM_TO_LL_RES_PARAM(r)	\
		(container_of(r, struct _ipcq_res_param_ll, res_param))

#define GET_CMD_HEAD_ID(flg)	((flg >> F_IPC_QUEUE_HEAD_ID_S) &\
						F_IPC_QUEUE_HEAD_ID_M)
#define GET_CMD_NEXT_ID(flg)	((flg >> F_IPC_QUEUE_NEXT_ID_S) &\
						F_IPC_QUEUE_NEXT_ID_M)

#define IQ_USED_F		(0x1)	/* used */
#define IQ_RESERVED_F		(0x10)	/* from reserved region */
#define IQ_KMALLOC_F		(0x20)	/* from allocation */
#define IQ_ISR_F		(0x100)	/* from isr */
#define IQ_MBOX_F		(0x200)	/* from iva mbox */

#define MAX_IPCQ_PEND_NR	(32)

struct iq_pend_mail {
	struct list_head node;
	uint64_t	flags;
	uint64_t	mail;	/* can hold va, as well as mbox msg */
};

/* convert mcu sram virtual address in ap point of view into address from mcu */
static inline uint32_t __iva_ipcq_get_mcu_from_va(const struct iva_dev_data *iva,
		void __iomem *mcu_va)
{
	uint32_t conv_addr;

#if defined(CONFIG_SOC_EXYNOS9810)
	conv_addr = (uint32_t) (mcu_va - iva->shmem_va) +
			(iva->mcu_mem_size - iva->mcu_shmem_size);
#elif defined(CONFIG_SOC_EXYNOS9820)
    conv_addr = (uint32_t) (mcu_va - iva->shmem_va);
#endif

    dev_dbg(iva->dev, "%s() conv_addr(0x%x)\n", __func__, conv_addr);
	return conv_addr;
}

/* convert mcu sram address in mcu point of view to virtual address on ap */
static inline void __iomem *__iva_ipcq_get_va_from_mcu(const struct iva_dev_data *iva,
		uint32_t mcu)
{
	void __iomem *conv_addr;

#if defined(CONFIG_SOC_EXYNOS9810)
	conv_addr = mcu - (iva->mcu_mem_size - iva->mcu_shmem_size)
			+ iva->shmem_va;
#elif defined(CONFIG_SOC_EXYNOS9820)
    conv_addr = mcu + iva->shmem_va;
#endif

	dev_dbg(iva->dev, "%s() conv_addr(0x%p), mcu(0x%x)\n", __func__,
			conv_addr, mcu);
	return conv_addr;
}

static inline bool __ipc_queue_is_used_cmd_slot(uint32_t flags)
{
	if (flags & F_IPC_QUEUE_USED_SLOT)
		return true;
	return false;
}

static inline bool __ipc_queue_is_head_cmd_slot(uint32_t flags)
{
	if (flags & F_IPC_QUEUE_HEAD)
		return true;
	return false;
}

static inline int32_t __ipc_queue_get_cmd_index(struct ipc_cmd_queue *cmd_q,
		struct _ipcq_cmd_param_ll *param)
{
	int offset;

	offset = (int32_t) (param - cmd_q->params);
	if (offset < 0 || offset > IPC_CMD_Q_PARAM_SIZE)
		return -EINVAL;

	return offset;
}

static inline struct iq_pend_mail *__iva_ipcq_alloc_pend_mail_from_rsv(
		struct iva_ipcq *ipcq)
{
	struct device		*dev = ipcq->iva_data->dev;
	struct iq_pend_mail	*rsv = ipcq->rsv;
	struct iq_pend_mail	*cur = NULL;
	uint32_t		hint_idx;
	int			i;
	unsigned long		flags;

	spin_lock_irqsave(&ipcq->rsv_slock, flags);
	hint_idx = ipcq->rsv_hint;
	for (i = 0; i < MAX_IPCQ_PEND_NR; i++, hint_idx++) {
		if (hint_idx == MAX_IPCQ_PEND_NR)
			hint_idx = 0;
		if (!(rsv[hint_idx].flags & IQ_USED_F)) {
			cur = &rsv[hint_idx];
			break;
		}
	}

	if (!cur) {
		dev_info(dev, "%s() not found in rsv (all busy)\n", __func__);
		spin_unlock_irqrestore(&ipcq->rsv_slock, flags);
		return NULL;
	}

	cur->flags = IQ_USED_F | IQ_RESERVED_F;
	ipcq->rsv_hint++;
	if (ipcq->rsv_hint == MAX_IPCQ_PEND_NR)
		ipcq->rsv_hint = 0;
	spin_unlock_irqrestore(&ipcq->rsv_slock, flags);
	return cur;
}

static inline struct iq_pend_mail *__iva_ipcq_alloc_pend_mail_from_kmem(
		struct iva_ipcq *ipcq, bool from_irq)
{
	struct iq_pend_mail	*pend_mail;

	/* try to alloc from kernel mem */
	pend_mail = kmalloc(sizeof(*pend_mail), from_irq ? GFP_ATOMIC : GFP_KERNEL);
	if (!pend_mail)	/* fail at last trial */
		return NULL;

	pend_mail->flags = IQ_USED_F | IQ_KMALLOC_F;

	return pend_mail;
}


static struct iq_pend_mail *__iva_ipcq_alloc_pend_mail(struct iva_ipcq *ipcq,
			bool from_irq)
{
	struct iq_pend_mail	*pend_mail;

	pend_mail = __iva_ipcq_alloc_pend_mail_from_rsv(ipcq);
	if (pend_mail)
		goto success;

	/* try to alloc from kernel mem */
	pend_mail = __iva_ipcq_alloc_pend_mail_from_kmem(ipcq, from_irq);
	if (!pend_mail)
		return NULL;
success:
	if (from_irq)
		pend_mail->flags |= IQ_ISR_F;
	return pend_mail;
}


static void __iva_ipcq_free_pend_mail(struct iva_ipcq *ipcq,
		struct iq_pend_mail *pend_mail)
{
	unsigned long		flags;

	/* reserved */
	spin_lock_irqsave(&ipcq->rsv_slock, flags);
	pend_mail->flags &= ~IQ_USED_F;
	spin_unlock_irqrestore(&ipcq->rsv_slock, flags);

	if (pend_mail->flags & IQ_RESERVED_F)
		return;

	/* allocated from kmalloc */
	kfree(pend_mail);
}


static int iva_ipcq_insert_pending_mail(struct iva_ipcq *ipcq,
		uint64_t mail, bool from_mbox, bool from_irq)
{
	struct iq_pend_mail	*pend_mail;
	struct iva_dev_data	*iva = ipcq->iva_data;
	struct device		*dev = iva->dev;
	unsigned long		flags;

	pend_mail = __iva_ipcq_alloc_pend_mail(ipcq, from_irq);
	if (!pend_mail) {
		dev_err(dev, "%s() fail to alloc atomic size(%d) for mail\n",
			__func__, (uint32_t) sizeof(*pend_mail));
		return -ENOMEM;
	}

	if (from_mbox)
		pend_mail->flags |= IQ_MBOX_F;

	pend_mail->mail	= mail;
	dev_dbg(dev, "%s() pend_mail(0x%p), mail(0x%llx) flags(0x%llx)\n",
			__func__, pend_mail, pend_mail->mail, pend_mail->flags);

	spin_lock_irqsave(&ipcq->ipcq_slock, flags);
	list_add_tail(&pend_mail->node, &ipcq->ipcq_pend_list);
	spin_unlock_irqrestore(&ipcq->ipcq_slock, flags);

	return 0;
}


int iva_ipcq_init_pending_mail(struct iva_ipcq *ipcq)
{
	struct iq_pend_mail	*pend_mail;
	struct iva_dev_data	*iva = ipcq->iva_data;
	struct device		*dev = iva->dev;

	if (!ipcq) {
		dev_info(dev, "%s(%d) no ipcq\n", __func__, __LINE__);
		return 0;
	}

	mutex_lock(&ipcq->ipcq_mutex);
	while (!list_empty(&ipcq->ipcq_pend_list)) {
		dev_info(dev, "%s(%d) one mail is remaining in plist: free\n",
				__func__, __LINE__);
		pend_mail = list_first_entry(&ipcq->ipcq_pend_list,
				struct iq_pend_mail, node);
		list_del(&pend_mail->node);
		__iva_ipcq_free_pend_mail(ipcq, pend_mail);
	}
	mutex_unlock(&ipcq->ipcq_mutex);

	return 0;
}

#ifdef ENABLE_IPCQ_WORK_QUEUE
static void iva_ipcq_do_work(struct work_struct *work)
{
	struct iva_ipcq_work	*ipcq_work = container_of(work,
					struct iva_ipcq_work, work);
	struct iva_ipcq		*ipcq = container_of(ipcq_work,
					struct iva_dev_data, ipcq_work);
	struct device		*dev = ipcq->iva_data->dev;
	int			ret;

	ret = iva_ipcq_insert_pending_mail(ipcq,
			(uint64_t) ipcq_work->msg, true, false);
	if (ret) {	/* error */
		dev_err(dev, "%s() fail to insert mail. ret(%d)\n", ret);
		return;
	}

	dev_dbg(dev, "%s()- msg(0x%x)\n", __func__, ipcq_work->msg);
}
#endif

/*  called from threaded irq handler */
static bool iva_ipcq_callback(struct iva_mbox_cb_block *ctrl_blk, uint32_t msg)
{
	struct iva_ipcq		*ipcq	= (struct iva_ipcq *) ctrl_blk->priv_data;
	struct iva_dev_data	*iva	= ipcq->iva_data;
#ifdef ENABLE_IPCQ_WORK_QUEUE
	struct iva_ipcq_work	*ipcq_work = &iva->ipcq_work;
#endif
	bool			ret;

	dev_dbg(iva->dev, "%s() 0x%x\n", __func__, msg);
#ifdef ENABLE_IPCQ_WORK_QUEUE
	ipcq_work->msg = msg;
	schedule_work(&ipcq_work->work);
#else
	ret = iva_ipcq_insert_pending_mail(ipcq, (uint64_t) msg, true, true);
	if (ret) {	/* error */
		dev_err(iva->dev, "%s() fail to insert mail.\n", __func__);
		return true;
	}

	dev_dbg(iva->dev, "%s() wake up... 0x%x\n", __func__, msg);

	wake_up(&ipcq->ipcq_wait_queue);
#endif
	return true;
}

static struct iva_mbox_cb_block iva_ipcq_cb = {
	.callback	= iva_ipcq_callback,
	.priority	= 0,
};


static inline int32_t iva_ipcq_wait_is_active(struct ipc_cmd_queue *ap_cmd_q,
		uint32_t to_us)
{
	int ret	= (int) to_us;

	while ((ap_cmd_q->flags & IPC_QUEUE_ACTIVE_FLAG)
			!= IPC_QUEUE_ACTIVE_FLAG) {
		if (ret < 0)
			return 0;
		usleep_range(VALUE_CHECK_DECREMENT, VALUE_CHECK_DECREMENT+1);
		ret -= VALUE_CHECK_DECREMENT;
	}

	return 1;
}

static inline uint32_t iva_ipcq_find_free_slot(struct ipc_cmd_queue *ap_cmd_q,
			uint32_t start_slot)
{
	int32_t	i;
	struct _ipcq_cmd_param_ll *cmd_param = ap_cmd_q->params;

	for (i = 0; i < IPC_CMD_Q_PARAM_SIZE; i++, start_slot++) {
		if (start_slot >= IPC_CMD_Q_PARAM_SIZE)
			start_slot = 0;
		if (!(cmd_param[start_slot].flags & F_IPC_QUEUE_USED_SLOT))
			return start_slot;
	}

	return IPC_CMD_Q_PARAM_SIZE;	/* means no free slot is found */
}

static int8_t __maybe_unused iva_ipcq_get_slot_index(
		struct ipc_cmd_queue *ap_cmd_q,	struct _ipcq_cmd_param_ll *param)
{
	int offset;

	offset = (int32_t) (param - ap_cmd_q->params);
	if (offset < 0 || offset > IPC_CMD_Q_PARAM_SIZE)
		return -1;

	return offset;
}

static struct ipc_cmd_param __iomem *iva_ipcq_alloc_cmds(struct iva_ipcq *ipcq,
			uint32_t nr_cmds)
{
	struct iva_dev_data		*iva = ipcq->iva_data;
	struct device			*dev = iva->dev;
	struct ipc_cmd_queue		*ap_cmd_q = ipcq->cmd_q;
	struct _ipcq_cmd_param_ll	*found_param, *prev_param;
	uint32_t	free_slot;
	uint32_t	remain_cmds;
	uint8_t		head_slot_id = 0;	/* no meaning */
	uint32_t	slot_flags;
	uint32_t	ret_cnt = IPCQ_CMD_ALLOC_RETRY_CNT;

	if (!iva_ipcq_wait_is_active(ap_cmd_q, WAIT_DELAY_US))
		return NULL;

	/* only half of total command queue slots are allowed */
	if ((nr_cmds > (IPC_CMD_Q_PARAM_SIZE/2)) || nr_cmds == 0) {
		dev_err(dev,
			"%s() too many or zero command (%d) is requested\n",
			__func__, nr_cmds);
		return NULL;
	}

	remain_cmds	= nr_cmds;
	prev_param	= NULL;

	mutex_lock(&ipcq->ipcq_mutex);
again:
	free_slot = iva_ipcq_find_free_slot(ap_cmd_q, ipcq->ipcq_next_slot);
	if (free_slot >= IPC_CMD_Q_PARAM_SIZE) {	/* not found */
		/* do polling */
		ret_cnt--;
		if (ret_cnt) {
			dev_warn(dev, "%s() retry to alloc cmd, try_cnt(%d)\n",
					__func__, ret_cnt);
			msleep(IPCQ_CMD_ALLOC_SLEEP_MS);
			goto again;
		}
		mutex_unlock(&ipcq->ipcq_mutex);
		dev_warn(dev, "%s() command queue wait exit\n", __func__);
		return NULL;
	}

	remain_cmds--;
	found_param = &ap_cmd_q->params[free_slot];
	slot_flags = F_IPC_QUEUE_USED_SLOT;	/* mark as used */
	if (!prev_param) {	/* head */
		head_slot_id = (uint8_t) free_slot;
		slot_flags |= F_IPC_QUEUE_HEAD;	/* mark as head */
	} else {
		/* update prev param flags to hold next pram idx */
		prev_param->flags |= (free_slot & F_IPC_QUEUE_NEXT_ID_M);
	}
	slot_flags |= (head_slot_id << F_IPC_QUEUE_HEAD_ID_S);

	ipcq->ipcq_next_slot = free_slot + 1;
	if (ipcq->ipcq_next_slot >= IPC_CMD_Q_PARAM_SIZE)
		ipcq->ipcq_next_slot = 0;

	dev_dbg(dev, "%s() remain_cmds(%d) found slot(%d), slot_flags(0x%x)\n",
			__func__, remain_cmds, free_slot, slot_flags);

	if (remain_cmds > 0) {
		found_param->cmd_param.cmd_id = ipcq->req_id;
		found_param->flags = slot_flags | F_IPC_QUEUE_NEXT;
		prev_param = found_param;
		goto again;
	}
	/* last command element : remain_cmds == 0*/
	found_param->flags = slot_flags | F_IPC_QUEUE_TAIL;
	ipcq->req_id++;
	/* mark newly allocated slot is in use */
	mutex_unlock(&ipcq->ipcq_mutex);

	/* return head slot */
	return &ap_cmd_q->params[head_slot_id].cmd_param;

}

static struct ipc_cmd_param __iomem *iva_ipcq_alloc_cmd(struct iva_ipcq *ipcq)
{
	return iva_ipcq_alloc_cmds(ipcq, 1);
}


static int __maybe_unused iva_ipcq_free_cmd(struct iva_ipcq *ipcq,
			struct ipc_cmd_param __iomem *c_param)
{
	struct iva_dev_data		*iva = ipcq->iva_data;
	struct device			*dev = iva->dev;
	struct ipc_cmd_queue		*ap_cmd_q = ipcq->cmd_q;
	struct _ipcq_cmd_param_ll	*c_param_ll;
	uint32_t	flags;
	int32_t		slot_id;
	int		ret = 0;

	if (!c_param) {
		dev_err(dev, "%s() try to free with null command\n",
				__func__);
		return -EINVAL;
	}

	c_param_ll = CMD_PARAM_TO_LL_CMD_PARAM(c_param);
	flags = c_param_ll->flags;
	if (!__ipc_queue_is_used_cmd_slot(flags)) {
		dev_err(dev, "%s() ERR: already freed(%p) flags(0x%08x)\n",
				__func__, c_param_ll, flags);
		return -EINVAL;
	}

	if (!__ipc_queue_is_head_cmd_slot(flags)) {
		dev_warn(dev, "%s() WARN this slot is not head, flags(0x%08x)\n",
				__func__, flags);
		c_param_ll = &ap_cmd_q->params[GET_CMD_HEAD_ID(flags)];
		flags = c_param_ll->flags;
	}

	mutex_lock(&ipcq->ipcq_mutex);
again:
	slot_id = __ipc_queue_get_cmd_index(ap_cmd_q, c_param_ll);
	if (slot_id < 0) {
		dev_err(dev, "%s() unrecognized slot id(%d)\n",
				__func__, slot_id);
		ret = -EINVAL;
		goto out;
	}
	c_param_ll->flags &= ~F_IPC_QUEUE_USED_SLOT;

	/* Clear information to avoid wrong action. */
	c_param->ipc_cmd_type = IPC_CMD_NONE;

	dev_dbg(dev, "%s() id %d, c_param_ll(%p) flag(0x%08x)\n", __func__,
			slot_id, c_param_ll, c_param_ll->flags);

	if (flags & F_IPC_QUEUE_TAIL) {
		dev_dbg(dev,
			"%s() success to free command queue, final slot id(%d)\n",
			__func__, slot_id);
		ret = 0;	/* success */
		goto out;
	}

	if (flags & F_IPC_QUEUE_NEXT) {
		c_param_ll = &ap_cmd_q->params[GET_CMD_NEXT_ID(flags)];
		flags = c_param_ll->flags;
		if (!__ipc_queue_is_used_cmd_slot(flags)) {
			dev_err(dev, "%s() ERR: already freed(%p) flags(0x%08x)\n",
				__func__, c_param_ll, flags);
			ret = -EINVAL;
			goto out;
		}
		goto again;
	}

	/* do not reach here */
	dev_err(dev, "c_param_ll(%p) flags(0x%08x)\n", c_param_ll, flags);

out:
	mutex_unlock(&ipcq->ipcq_mutex);
	return ret;
}

static void iva_ipcq_save_transaction(struct iva_ipcq *ipcq,
			bool cmd, void *data)
{
	struct device		*dev	= ipcq->iva_data->dev;
	struct iva_ipcq_stat	*stat	= &ipcq->ipcq_stat;
	struct iva_ipcq_trans	*trans_item;

	if (stat->trans_nr_max == 0)
		return;

	mutex_lock(&stat->trans_mtx);
	if (stat->trans_nr >= stat->trans_nr_max) {
		/* reuse*/
		trans_item = list_first_entry(&stat->trans_head,
				struct iva_ipcq_trans, node);
		list_del(&trans_item->node);
		stat->trans_nr--;
	} else {
		trans_item = devm_kmalloc(dev, sizeof(*trans_item), GFP_KERNEL);
		if (!trans_item) {
			dev_err(dev, "%s() fail to alloc mem for trans_item\n",
				__func__);
			mutex_unlock(&stat->trans_mtx);
			return;
		}
	}

	trans_item->ts_nsec = local_clock();
	if (cmd) {
		trans_item->send	= true;
		trans_item->cmd_param	= *(struct ipc_cmd_param *)data;
	} else {
		trans_item->send	= false;
		trans_item->res_param	= *(struct ipc_res_param *)data;
	}

	list_add_tail(&trans_item->node, &stat->trans_head);
	stat->trans_nr++;
	mutex_unlock(&stat->trans_mtx);
}

static int iva_ipcq_send_cmd(struct iva_ipcq *ipcq,
		struct ipc_cmd_param __iomem *ipc_param)
{
	struct iva_dev_data *iva = ipcq->iva_data;
	struct _ipcq_cmd_param_ll __iomem *cmd_param;
	uint32_t conv_param_p;

	iva_ipcq_save_transaction(ipcq, true, ipc_param);

	cmd_param = CMD_PARAM_TO_LL_CMD_PARAM(ipc_param);
#ifdef DEBUG
	cmd_param->flags |= F_IPC_QUEUE_SENDING;	/* mark sending */
#endif
	/* send really */
	conv_param_p = __iva_ipcq_get_mcu_from_va(iva, cmd_param);

	return iva_mbox_send_mail_to_mcu(iva, conv_param_p);
}

static inline int iva_ipcq_copy_from_user(uint32_t *dest,
			uint32_t __user *src, uint32_t size)
{
#ifdef CONFIG_64BIT
	int	i;
	/*
	 * avoid alignment fault in case of 32-bit :
	 * assume at leat 4-byte alignment
	 */
	for (i = 0; i < size >> 2; i++)
		get_user(dest[i], src + i);

	return 0;
#else
	if (copy_from_user(dest, src, size))
		return -EFAULT;

	return 0;
#endif
}

static bool iva_ipcq_handle_inside_func(struct iva_proc *proc,
		struct iva_ipcq *ipcq, struct ipc_cmd_param *cmd_param)
{

	struct device		*dev = ipcq->iva_data->dev;
	struct ipc_res_param	rsp_param;
	uint32_t		hdr = cmd_param->header;

	if (IPC_GET_FUNC(hdr) != ipc_func_iva_ctrl)
		return false;

	switch (IPC_GET_SUB_FUNC(hdr)) {
	case ipc_iva_ctrl_release_wait:
		rsp_param.header	= IPC_MK_HDR(ipc_func_iva_ctrl,
				ipc_iva_ctrl_release_wait, IPC_GET_EXTRA(hdr));
		rsp_param.rep_id	= 0x0ace0ace;
		rsp_param.ret		= (uint32_t) 0;
		rsp_param.ipc_cmd_type	= IPC_CMD_DEINIT;
		rsp_param.extra		= (uint16_t) 0;
		iva_ipcq_send_rsp_k(ipcq, &rsp_param, false);
		break;

	case ipc_iva_ctrl_dump_sfrs:
		iva_ram_dump_copy_iva_sfrs(proc, (int) cmd_param->param_0);
		break;

	default:
		dev_warn(dev, "%s() can't handle inside func(0x%x)\n",
				__func__, hdr);
		break;
	}

	return true;
}

int iva_ipcq_send_cmd_usr(struct iva_proc *proc,
			struct ipc_cmd_param __user *ipc_param)
{
	struct iva_dev_data	*iva	= proc->iva_data;
	struct device		*dev	= iva->dev;
	struct iva_ipcq		*ipcq	= &iva->mcu_ipcq;
	struct ipc_cmd_param	tmp_param;
	struct ipc_cmd_param	*c_param;
	int			ret;

	if (!ipc_param) {
		dev_err(dev, "%s() null from user\n", __func__);
		return -EINVAL;
	}

	if (iva_ipcq_copy_from_user((uint32_t *) &tmp_param,
			(uint32_t __user *) ipc_param, sizeof(tmp_param))) {
		return -EINVAL;
	}

	/* comm bewteen us and iva driver */
	if (iva_ipcq_handle_inside_func(proc, ipcq, &tmp_param))
		return 0;

	if (!iva_ctrl_is_boot(iva)) {
		dev_err(dev, "%s() iva mcu is not booted, state(0x%lx)\n",
				__func__, ipcq->iva_data->state);
		return -EPERM;
	}

	c_param = iva_ipcq_alloc_cmd(ipcq);
	if (!c_param) {
		dev_err(dev, "%s() fail to alloc cmd buffer\n", __func__);
		return -EBUSY;
	}

	dev_dbg(dev, "%s() ipc_param(%p), c_param(%p), sizeof(cmd=0x%x)\n",
			__func__, ipc_param, c_param, (int) sizeof(*c_param));

	*c_param = tmp_param;

	ret = iva_ipcq_send_cmd(ipcq, c_param);
	if (ret < 0) {
		dev_err(dev, "%s() fail to send ipcq cmd\n", __func__);
		return ret;
	}

	if (IPC_GET_FUNC(tmp_param.header) == ipc_func_sched_table) {
		if (tmp_param.ipc_cmd_type == IPC_CMD_FRAME_START) {
			/* clear err count */
			atomic_set(&iva->mcu_err_cnt, 0);

			if (tmp_param.param_0 != 0/* no TO */) {
				ipcq->to_in_ms = tmp_param.param_0;
				dev_dbg(dev, "%s() time out(%d ms) req on FS\n",
					__func__, ipcq->to_in_ms);
				ret = mod_timer(&ipcq->to_timer,
					jiffies + msecs_to_jiffies(ipcq->to_in_ms));
				if (ret < 0) {
					dev_err(dev,
						"%s() fail to set to timer(%d ms), ret(%d)\n",
						__func__, ipcq->to_in_ms, ret);
				}
			}
		}
	}

	return 0;
}

/*  called from threaded irq handler */
static bool iva_ipcq_ctrl_callback(struct iva_mbox_cb_block *ctrl_blk, uint32_t msg)
{
	struct iva_ipcq		*ipcq	= (struct iva_ipcq *) ctrl_blk->priv_data;
	struct device		*dev	= ipcq->iva_data->dev;

	dev_info(dev, "%s() received free rsp queue ctrl mail\n", __func__);

	ipcq->ipcq_ctrl_req = true;
	return true;
}


static struct iva_mbox_cb_block iva_ipcq_ctrl_cb = {
	.callback	= iva_ipcq_ctrl_callback,
	.priority	= 0,
};


static int32_t iva_ipcq_free_rsp(struct iva_ipcq *ipcq,
			struct ipc_res_param __iomem *r_param)
{
	struct iva_dev_data	*iva = ipcq->iva_data;
	struct device		*dev = iva->dev;
	struct _ipcq_res_param_ll *r_param_ll =
				RES_PARAM_TO_LL_RES_PARAM(r_param);
	unsigned long			flags;

	dev_dbg(dev,
		"%s() clear, r_param_ll(%p) flag(0x%08x) ipcq_ctrl_req(%d)\n",
		__func__, r_param_ll, r_param_ll->flags, ipcq->ipcq_ctrl_req);

	if (r_param_ll->flags & F_IPC_QUEUE_MALLOC) {
		kfree(r_param_ll);
	} else {
		r_param_ll->flags &= ~F_IPC_QUEUE_USED_SLOT;
		r_param->ipc_cmd_type = IPC_CMD_NONE;
		spin_lock_irqsave(&ipcq->rspq_slock, flags);

		if (ipcq->ipcq_ctrl_req) {
			ipcq->ipcq_ctrl_req = false;
			spin_unlock_irqrestore(&ipcq->rspq_slock, flags);
			iva_mbox_send_mail_to_mcu(iva,
				MBOX_CTRL_IPCQ_CTRL_RSP_QUEUE_FREE);
		} else {
			spin_unlock_irqrestore(&ipcq->rspq_slock, flags);
		}
	}

	return 0;
}


static struct ipc_res_param __iomem *iva_ipcq_read_rsp(struct iva_ipcq *ipcq)
{
	int	ret;
	struct iva_dev_data		*iva = ipcq->iva_data;
	struct device			*dev = iva->dev;
	struct iq_pend_mail		*work_mail;
	struct _ipcq_res_param_ll __iomem *r_param_ll;
	struct list_head		*pend_mlist = &ipcq->ipcq_pend_list;
	uint64_t			mail;
	unsigned long			flags;

	spin_lock_irqsave(&ipcq->ipcq_slock, flags);
	while (list_empty(pend_mlist)) {
		spin_unlock_irqrestore(&ipcq->ipcq_slock, flags);

		/* wait */
		if (ipcq->wait_to_jiffies) {
		#ifdef ALLOW_SIGNAL_RECEPTION
			ret = wait_event_interruptible_timeout(
					ipcq->ipcq_wait_queue,
					!list_empty(pend_mlist),
					ipcq->wait_to_jiffies);
		#else
			ret = wait_event_timeout(
					ipcq->ipcq_wait_queue,
					!list_empty(pend_mlist),
					ipcq->wait_to_jiffies);
		#endif
			if (ret < 0) {
				dev_err(dev, "%s() ret(%d) on wait_timeout\n",
						__func__, ret);
				return ERR_PTR(ret);

			} else if (ret == 0) {
				/* time out : intended or not*/
				dev_dbg(dev, "%s() timeout - %d ms(%ld)\n",
						__func__,
						(int) IPCQ_TIMEOUT_MS,
						ipcq->wait_to_jiffies);
				return ERR_PTR(-EIO);
			} /* others success : remaining jiffies or 1(expire)*/
		} else {
		#ifdef ALLOW_SIGNAL_RECEPTION
			ret = wait_event_interruptible(
					ipcq->ipcq_wait_queue,
					!list_empty(pend_mlist));
			if (ret < 0) {
				dev_err(dev, "%s() ret(%d) on wait\n",
						__func__, ret);
				return ERR_PTR(ret);
			}
		#else
			wait_event(ipcq->ipcq_wait_queue,
					!list_empty(pend_mlist));
		#endif
		}

		dev_dbg(dev,
			"%s() received on wait state, ret(%d), list_empty(%d)\n",
			__func__, ret, list_empty(pend_mlist));

		/* wait exit successfully */
		spin_lock_irqsave(&ipcq->ipcq_slock, flags);
	}

	work_mail = list_first_entry(pend_mlist, struct iq_pend_mail, node);
	mail = work_mail->mail;
	list_del(&work_mail->node);
	spin_unlock_irqrestore(&ipcq->ipcq_slock, flags);

	if (work_mail->flags & IQ_MBOX_F) {
		/* convert to va */
		r_param_ll = (struct _ipcq_res_param_ll __iomem *)
				__iva_ipcq_get_va_from_mcu(iva, (uint32_t) mail);
	} else {
		/* no convert */
		r_param_ll = (struct _ipcq_res_param_ll *)(long) mail;
	}

	iva_ipcq_save_transaction(ipcq, false, &r_param_ll->res_param);

	dev_dbg(dev, "%s() mail(0x%llx) res_param_ll(0x%p-0x%x) flags(0x%llx)\n",
			__func__, mail,
			r_param_ll, r_param_ll->flags,
			work_mail->flags);

	__iva_ipcq_free_pend_mail(ipcq, work_mail);

	return &r_param_ll->res_param;
}


static inline int iva_ipcq_copy_to_user(uint32_t __user *dest,
			uint32_t *src, uint32_t size)
{
#ifdef CONFIG_64BIT
	int	i;
	/*
	 * avoid aignment fault in case of 32-bit :
	 * assume at leat 4-byte alignment
	 */
	for (i = 0; i < size >> 2; i++)
		put_user(src[i], dest + i);

	return 0;
#else
	if (copy_to_user(dest, src, size))
		return -EINVAL;
	return 0;
#endif
}

/* called from interrupt context */
static void iva_ipcq_no_rsp_timer(unsigned long data)
{
	struct iva_ipcq		*ipcq	= (struct iva_ipcq *)data;
	struct iva_dev_data	*iva	= ipcq->iva_data;
	struct device		*dev	= ipcq->iva_data->dev;

	dev_err(dev, "%s() no repsonse from mcu during (%d) ms\n",
			__func__, ipcq->to_in_ms);

	iva_mcu_handle_error_k(iva, mcu_err_from_irq, 32);
}

int iva_ipcq_wait_res_usr(struct iva_proc *proc,
			struct ipc_res_param __user *r_param)
{
	struct iva_dev_data	*iva	= proc->iva_data;
	struct device		*dev	= iva->dev;
	struct iva_ipcq		*ipcq	= &iva->mcu_ipcq;
	struct ipc_res_param	*ret_param;
	struct ipc_res_param	tmp_resp;
	int			ret;

	if (!r_param) {
		dev_err(dev, "%s() @res_param is null\n", __func__);
		return -EINVAL;
	}

	ret_param = iva_ipcq_read_rsp(ipcq);
	if (IS_ERR(ret_param)) {
		del_timer_sync(&ipcq->to_timer);
		ipcq->to_in_ms = 0;

		memset(&tmp_resp, 0x0, sizeof(tmp_resp));
		tmp_resp.header	= IPC_MK_HDR(ipc_func_iva_ctrl, 0x0, 0x0);
		tmp_resp.rep_id	= 0x0ace0ace;
		tmp_resp.ret	= (uint32_t) 0;
		if (PTR_ERR(ret_param) == (long) -EIO)
			tmp_resp.ipc_cmd_type	= IPC_CMD_TIMEOUT_NOTIFY;
		else if (PTR_ERR(ret_param) == (long) -ERESTARTSYS)
			tmp_resp.ipc_cmd_type	= IPC_CMD_SIGNAL_NOTIFY;
		else
			tmp_resp.ipc_cmd_type	= IPC_CMD_UNKNOWN_NOTIFY;
		ret_param = &tmp_resp;

		if (!!iva->state) {
			iva_mcu_print_flush_pending_mcu_log(iva);

			iva_pmu_show_status(iva);
			iva_vdma_show_status(iva);
		}
	} else {
		switch (IPC_GET_FUNC(ret_param->header)) {
		case ipc_func_sched_table:
			if (ret_param->ipc_cmd_type == IPC_CMD_FRAME_DONE) {
				dev_dbg(dev, "%s() FD, so delete timer\n",
						__func__);
				del_timer_sync(&ipcq->to_timer);
				ipcq->to_in_ms = 0;
			}
			break;
		case ipc_func_iva_ctrl:
			if (ret_param->ipc_cmd_type >= IPC_CMD_ERR_BASE) {
				dev_err(dev, "%s() err cmd(%d) cnt(%d)\n",
					__func__, ret_param->ipc_cmd_type,
					atomic_read(&iva->mcu_err_cnt));
				del_timer_sync(&ipcq->to_timer);
				ipcq->to_in_ms = 0;

				if (!!iva->state) {
					iva_mcu_print_flush_pending_mcu_log(iva);

					iva_pmu_show_status(iva);
					iva_vdma_show_status(iva);
				}
			}
			break;
		default:
			/* no error */
			break;
		}
	}

	ret = iva_ipcq_copy_to_user((uint32_t __user *) r_param,
			(uint32_t *) ret_param, sizeof(*r_param));
	if (ret < 0) {
		dev_err(dev, "%s() fail to copy_to_user(%p, %p)\n",
				__func__, r_param, ret_param);
	}

	if (ret_param != &tmp_resp)
		iva_ipcq_free_rsp(ipcq, ret_param);

	return ret;
}

int iva_ipcq_send_rsp_k(struct iva_ipcq *ipcq,
		struct ipc_res_param *res_param, bool from_irq)
{
	struct iva_dev_data	*iva = ipcq->iva_data;
	struct device		*dev = iva->dev;
	struct _ipcq_res_param_ll *r_param_ll;
#ifdef ENABLE_IPCQ_WORK_QUEUE
	struct iva_ipcq_work	*ipcq_work = &iva->ipcq_work;
#else
	int			ret;
#endif
	r_param_ll = kmalloc(sizeof(*r_param_ll),
			from_irq ? GFP_ATOMIC : GFP_KERNEL);
	if (!r_param_ll) {
		dev_err(dev, "%s() fail to alloc mem for r_param_ll(%p)\n",
			__func__, __builtin_return_address(0));
		return -ENOMEM;
	}

	r_param_ll->flags = (F_IPC_QUEUE_USED_SLOT | F_IPC_QUEUE_MALLOC |
			F_IPC_QUEUE_HEAD | F_IPC_QUEUE_TAIL);
	r_param_ll->res_param = *res_param;


#ifdef ENABLE_IPCQ_WORK_QUEUE
	ipcq_work->msg = r_param_ll;
	schedule_work(&ipcq_work->work);
#else
	ret = iva_ipcq_insert_pending_mail(ipcq,
	#ifdef CONFIG_64BIT
			(uint64_t) r_param_ll,
	#else
			(uint32_t) r_param_ll,
	#endif
			false, false);

	if (ret) {
		dev_err(dev, "%s() fail to insert pend mail, from %pF\n",
				__func__, __builtin_return_address(0));
		return ret;
	}

	wake_up(&ipcq->ipcq_wait_queue);
#endif
	return 0;
}


static inline int __maybe_unused iva_ipcq_check_param_regions(
		struct iva_dev_data *iva,
		struct ipc_cmd_queue *cmd_q, struct ipc_res_queue *rsp_q)
{
	struct device *dev = iva->dev;
	unsigned long addr;
	int align = sizeof(unsigned long);
	int ret = 0;
	int i;

	dev_dbg(dev, "%s() unsigned long size(%d)\n", __func__,
			align);

	dev_dbg(dev, "%s() cmd_ll size(0x%x), cmd size(0x%x)\n", __func__,
			(int) sizeof(struct _ipcq_cmd_param_ll),
			(int) sizeof(struct ipc_cmd_param));
	dev_dbg(dev, "%s() res_ll size(0x%x), res size(0x%x)\n", __func__,
			(int) sizeof(struct _ipcq_res_param_ll),
			(int) sizeof(struct ipc_res_param));

	/* cmd params */
	for (i = 0; i < IPC_CMD_Q_PARAM_SIZE; i++) {
		addr = (unsigned long) &cmd_q->params[i].cmd_param;
		if (addr % align) {
			dev_err(dev, "%s() [%d] addr(0x%lx)/align(0x%x) != 0 on  arch %s\n",
				       __func__, i, addr, align,
		#ifdef CONFIG_64BIT
					"64");
		#else
					"32");
		#endif
			ret = -EACCES;
		}
#if 0
		else {
			dev_dbg(dev, "%s() [%d] addr(0x%lx)/align(0x%x) == 0 on arch %s\n",
					__func__, i, addr, align,
		#ifdef CONFIG_64BIT
					"64");
		#else
					"32");
		#endif
		}
#endif
	}

	/* res params */
	for (i = 0; i < IPC_RES_Q_PARAM_SIZE; i++) {
		addr = (unsigned long) &rsp_q->params[i].res_param;
		if (addr % align) {
			dev_err(dev, "%s() [%d] addr(0x%lx)/align(0x%x) != 0 on arch %s\n",
				       __func__, i, addr, align,
	#ifdef CONFIG_64BIT
					"64");
	#else
					"32");
	#endif
			ret = -EACCES;
		}
#if 0
		else {
			dev_dbg(dev, "%s() [%d] addr(0x%lx)/align(0x%x) == 0 on arch %s\n",
					__func__, i, addr, align,
	#ifdef CONFIG_64BIT
					"64");
	#else
					"32");
	#endif
		}
#endif
	}



	return ret;
}


static inline void iva_ipcq_init_param_regions(struct ipc_res_queue *ap_rsp_q)
{
	int i;

	for (i = 0; i < IPC_RES_Q_PARAM_SIZE; i++)
		ap_rsp_q->params[i].flags = 0x0;
}

int iva_ipcq_init(struct iva_ipcq *ipcq, struct ipc_cmd_queue *cmd_q,
			struct ipc_res_queue *rsp_q)
{
	struct iva_dev_data	*iva = ipcq->iva_data;
#ifdef CHECK_PARAM_REGION_ALIGN
	int ret;
#endif
	/* clear slot & req id */
	ipcq->cmd_q		= cmd_q;
	ipcq->res_q		= rsp_q;
	ipcq->ipcq_next_slot	= 0;
	ipcq->req_id		= 0;


#ifdef CHECK_PARAM_REGION_ALIGN
	ret = iva_ipcq_check_param_regions(iva, cmd_q, rsp_q);
	if (ret) {
		dev_err(iva->dev, "%s() fail to init ipc queue, ret(%d)\n",
				__func__, ret);
		return ret;
	}
#endif
	/* confirm command queue is free */
	iva_ipcq_init_param_regions(rsp_q);
	/* ready to respond */
	rsp_q->flags = IPC_QUEUE_ACTIVE_FLAG;

	dev_dbg(iva->dev, "%s() success !!!\n", __func__);
	return 0;
}

void iva_ipcq_deinit(struct iva_ipcq *ipcq)
{
	struct iva_dev_data	*iva = ipcq->iva_data;
	struct ipc_res_queue	*ap_rsp_q = ipcq->res_q;

	ap_rsp_q->flags = ~IPC_QUEUE_ACTIVE_FLAG;

	ipcq->cmd_q	= NULL;
	ipcq->res_q	= NULL;

	dev_dbg(iva->dev, "%s() success !!!\n", __func__);
}

int iva_ipcq_probe(struct iva_dev_data *iva)
{
	struct iva_ipcq		*ipcq = &iva->mcu_ipcq;
	struct iva_ipcq_stat	*ipcq_stat = &ipcq->ipcq_stat;

	ipcq->rsv = (struct iq_pend_mail *) devm_kzalloc(iva->dev,
			sizeof(struct iq_pend_mail) * MAX_IPCQ_PEND_NR,
			GFP_KERNEL);
	if (!ipcq->rsv) {
		dev_err(iva->dev, "%s() fail to alloc space for pend mail\n",
				__func__);
		return -ENOMEM;
	}
	ipcq->rsv_hint = 0;

	/* initialize data structure */
	ipcq->iva_data	= iva;
	mutex_init(&ipcq->ipcq_mutex);
	spin_lock_init(&ipcq->ipcq_slock);
	spin_lock_init(&ipcq->rsv_slock);
	spin_lock_init(&ipcq->rspq_slock);
	init_waitqueue_head(&ipcq->ipcq_wait_queue);
	INIT_LIST_HEAD(&ipcq->ipcq_pend_list);
#ifdef ENABLE_IPCQ_WORK_QUEUE
	INIT_WORK(&ipcq->ipcq_work.work, iva_ipcq_do_work);
#endif
	ipcq->wait_to_jiffies = msecs_to_jiffies(IPCQ_TIMEOUT_MS);
	setup_timer(&ipcq->to_timer, iva_ipcq_no_rsp_timer, (unsigned long)ipcq);

	iva_ipcq_cb.priv_data = (void *) ipcq;
	iva_mbox_callback_register(mbox_msg_default, &iva_ipcq_cb);
	iva_ipcq_ctrl_cb.priv_data = (void *) ipcq;
	iva_mbox_callback_register(mbox_msg_ipcq_ctrl, &iva_ipcq_ctrl_cb);

	/* ipcq stat */
	ipcq_stat->trans_nr_max = 0;
	ipcq_stat->trans_nr = 0;
	mutex_init(&ipcq_stat->trans_mtx);
	INIT_LIST_HEAD(&ipcq_stat->trans_head);

	return 0;
}

void iva_ipcq_remove(struct iva_dev_data *iva)
{
	struct iva_ipcq *ipcq = &iva->mcu_ipcq;

	dev_dbg(iva->dev, "%s()\n", __func__);
	mutex_destroy(&ipcq->ipcq_mutex);

	iva_mbox_callback_unregister(&iva_ipcq_ctrl_cb);
	iva_mbox_callback_unregister(&iva_ipcq_cb);
	iva_ipcq_cb.priv_data = NULL;

	devm_kfree(iva->dev, ipcq->rsv);
}
