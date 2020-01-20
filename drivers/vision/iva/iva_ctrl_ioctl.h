/*
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#ifndef _IVA_CTRL_IOCRL_H_
#define _IVA_CTRL_IOCRL_H_

#include <linux/ioctl.h>
#include "iva_ipc_param.h"

#define IVA_CTRL_BOOT_FILE_NAME_LEN	(256)

#define IVA_CTRL_MAGIC			'i'

#define IVA_CTRL_INIT_IVA		_IOW(IVA_CTRL_MAGIC, 0, uint32_t)
#define IVA_CTRL_DEINIT_IVA		_IOW(IVA_CTRL_MAGIC, 1, uint32_t)

#define IVA_CTRL_BOOT_MCU		_IOW(IVA_CTRL_MAGIC, 2, char[IVA_CTRL_BOOT_FILE_NAME_LEN])
#define IVA_CTRL_EXIT_MCU		_IOW(IVA_CTRL_MAGIC, 3, uint32_t)

#define IVA_CTRL_GET_STATUS		_IOR(IVA_CTRL_MAGIC, 4, struct iva_status_param)

#define IVA_IPC_QUEUE_SEND_CMD		_IOW(IVA_CTRL_MAGIC, 5, struct ipc_cmd_param)
#define IVA_IPC_QUEUE_RECEIVE_RSP	_IOR(IVA_CTRL_MAGIC, 6, struct ipc_res_param)

#define IVA_ION_GET_IOVA		_IOWR(IVA_CTRL_MAGIC, 7, struct iva_ion_param)
#define IVA_ION_PUT_IOVA		_IOWR(IVA_CTRL_MAGIC, 8, struct iva_ion_param)

#define IVA_ION_ALLOC			_IOWR(IVA_CTRL_MAGIC, 9, struct iva_ion_param)
#define IVA_ION_FREE			_IOWR(IVA_CTRL_MAGIC, 10, struct iva_ion_param)
#define IVA_ION_SYNC_FOR_CPU		_IOWR(IVA_CTRL_MAGIC, 11, struct iva_ion_param)
#define IVA_ION_SYNC_FOR_DEVICE		_IOWR(IVA_CTRL_MAGIC, 12, struct iva_ion_param)

#define IVA_CTRL_SET_CLK		_IOWR(IVA_CTRL_MAGIC, 13, uint32_t)

#define SYNC_ALL_TYPE_LOC		(30)

/* for iva_ion struct */
struct iva_ion_param {
	int		ion_client_fd;
	int		shared_fd;
	unsigned int	iova;	/* fix to 32 bit */
	unsigned int	size;
	unsigned int	align;
	unsigned int	cacheflag;
};

struct iva_status_param {
	unsigned int	status;
	unsigned int	clk_rate;
};

#endif /* _IVA_CTRL_IOCRL_H_ */
