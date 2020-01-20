/*
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __IVA_MBOX_H
#define __IVA_MBOX_H

#include "iva_ctrl.h"


/* msg_hd : 8 MSBs identifier*/
enum mbox_msg_hd {
	mbox_msg_default	= 0x0,
	mbox_msg_ipcq_ctrl	= 0x1,
	mbox_msg_fatal		= 0xdd,
};

#define MBOX_MAIL_HEADER_MASK		(0xFF)
#define MBOX_MAIL_HEADER_SHIFT		(24)
#define MBOX_MAIL_DATA_MASK		((0x1 << MBOX_MAIL_HEADER_SHIFT) - 1)
#define MBOX_MAIL_DATA_SHIFT		(0)
#define GET_MBOX_MAIL_HD(mail)		(((mail) >> MBOX_MAIL_HEADER_SHIFT) &\
			MBOX_MAIL_HEADER_MASK)
#define GET_MBOX_MAIL_DATA(mail)	(((mail) >> MBOX_MAIL_DATA_SHIFT) &\
			MBOX_MAIL_DATA_MASK)
#define MAKE_MBOX_MAIL(hd, data)				\
		(((uint32_t)hd << MBOX_MAIL_HEADER_SHIFT) |	\
				(data & MBOX_MAIL_DATA_MASK))

/* ipc queue control */
#define IPCQ_CTRL_MSG_RSP_QUEUE_FREE		(0x1)
#define IPCQ_CTRL_MSG_CMD_QUEUE_FREE		(0x2)

#define MBOX_CTRL_IPCQ_CTRL_RSP_QUEUE_FREE	\
		MAKE_MBOX_MAIL(mbox_msg_ipcq_ctrl, IPCQ_CTRL_MSG_RSP_QUEUE_FREE)
#define MBOX_CTRL_IPCQ_CTRL_CMD_QUEUE_FREE	\
		MAKE_MBOX_MAIL(mbox_msg_ipcq_ctrl, IPCQ_CTRL_MSG_CMD_QUEUE_FREE)


struct iva_mbox_cb_block {
	/* if return true, stop calling the further callback */
	bool (*callback)(struct iva_mbox_cb_block *, uint32_t);
	struct iva_mbox_cb_block __rcu *next;
	uint32_t	msg_hd;			/* 8 MSBs */
	void		*priv_data;
	int		priority;
};

extern int	iva_mbox_callback_register(uint8_t msg_hd,
			struct iva_mbox_cb_block *cb);
extern int	iva_mbox_callback_unregister(struct iva_mbox_cb_block *cb);

extern int	iva_mbox_send_mail_to_mcu(struct iva_dev_data *iva, uint32_t msg);
extern int	iva_mbox_send_mail_to_mcu_usr(struct iva_dev_data *iva,
			uint32_t __user *msg_usr);

extern int	iva_mbox_init(struct iva_dev_data *iva);
extern void	iva_mbox_deinit(struct iva_dev_data *iva);

extern int	iva_mbox_probe(struct iva_dev_data *iva);
extern void	iva_mbox_remove(struct iva_dev_data *iva);

#endif /* __IVA_MBOX_H */
