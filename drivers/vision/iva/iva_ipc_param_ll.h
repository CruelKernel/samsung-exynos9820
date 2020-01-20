/*
 *
 *  Copyright (C) 2016 Samsung Electrnoics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __IVA_IPC_PARAM_LL_H__
#define __IVA_IPC_PARAM_LL_H__

#include "iva_ipc_param.h"

/* flags for xxx_param_ll.flgas */
#define F_IPC_QUEUE_MALLOC	(0x1 << 30)	/* allocated outside ipc queue */
#define F_IPC_QUEUE_SENDING	(0x1 << 29)	/* whether already sent to mcu */
#define F_IPC_QUEUE_USED_SLOT	(0x1 << 28)
#define F_IPC_QUEUE_HEAD	(0x1 << 24)
#define F_IPC_QUEUE_NEXT	(0x1 << 20)
#define F_IPC_QUEUE_TAIL	(0x1 << 16)
#define F_IPC_QUEUE_HEAD_ID_S	(8)
#define F_IPC_QUEUE_HEAD_ID_M	(0xFF)		/* 8 bits */
#define F_IPC_QUEUE_NEXT_ID_S	(0)
#define F_IPC_QUEUE_NEXT_ID_M	(0xFF)		/* 8 bits */

struct _ipcq_cmd_param_ll {
	uint32_t		flags;
#ifdef IVA_IPC_PARAM_ARCH64_SUPPORT
	uint32_t		rsv;
#endif
	struct ipc_cmd_param	cmd_param;
};

struct _ipcq_res_param_ll {
	uint32_t		flags;
#ifdef IVA_IPC_PARAM_ARCH64_SUPPORT
	uint32_t		rsv;
#endif
	struct ipc_res_param	res_param;
};

struct ipc_cmd_queue {
#ifdef CONFIG_TEST_PARAM
#define IPC_CMD_Q_PARAM_SIZE	(8)
#else
#define IPC_CMD_Q_PARAM_SIZE	(32)
#endif
#define IPC_QUEUE_ACTIVE_FLAG	(0x1 << 28)
	uint32_t		flags;
#ifdef IVA_IPC_PARAM_ARCH64_SUPPORT
	uint32_t		rsv;
#endif
	struct _ipcq_cmd_param_ll	params[IPC_CMD_Q_PARAM_SIZE];
};

struct ipc_res_queue {
#ifdef CONFIG_TEST_PARAM
#define IPC_RES_Q_PARAM_SIZE	(8)
#else
#define IPC_RES_Q_PARAM_SIZE	(32)
#endif
	uint32_t			flags;
#ifdef IVA_IPC_PARAM_ARCH64_SUPPORT
	uint32_t			rsv;
#endif
	struct _ipcq_res_param_ll	params[IPC_RES_Q_PARAM_SIZE];
};

#endif //__IVA_IPC_PARAM_LL_H__
