/*
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __IVA_IPQ_QUEUE_H__
#define __IVA_IPQ_QUEUE_H__

#include "iva_ipc_param.h"
#include "iva_ctrl.h"

struct iva_ipcq_trans {
	struct list_head	node;
	u64			ts_nsec;
	bool			send;	/* true: cmd, false: response */
	union {
		struct ipc_cmd_param	cmd_param;
		struct ipc_res_param	res_param;
	};
};


extern int	iva_ipcq_send_cmd_usr(struct iva_proc *proc,
			struct ipc_cmd_param __user *ipc_param);

extern int	iva_ipcq_wait_res_usr(struct iva_proc *proc,
			struct ipc_res_param __user *r_param);
extern int	iva_ipcq_send_rsp_k(struct iva_ipcq *ipcq,
			struct ipc_res_param *res_param, bool from_irq);

extern int	iva_ipcq_init(struct iva_ipcq *ipcq, struct ipc_cmd_queue *cmd_q,
			struct ipc_res_queue *rsp_q);
extern void	iva_ipcq_deinit(struct iva_ipcq *ipcq);

extern int	iva_ipcq_probe(struct iva_dev_data *iva);
extern void	iva_ipcq_remove(struct iva_dev_data *iva);

extern int	iva_ipcq_init_pending_mail(struct iva_ipcq *ipcq);
#endif /* __IVA_IPQ_QUEUE_H__*/
