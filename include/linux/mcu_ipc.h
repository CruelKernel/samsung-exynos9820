/*
 * Copyright (C) 2014 Samsung Electronics Co.Ltd
 * http://www.samsung.com
 *
 * MCU IPC driver
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
*/
#ifndef MCU_IPC_H
#define MCU_IPC_H

#define MCU_IPC_INT0    (0)
#define MCU_IPC_INT1    (1)
#define MCU_IPC_INT2    (2)
#define MCU_IPC_INT3    (3)
#define MCU_IPC_INT4    (4)
#define MCU_IPC_INT5    (5)
#define MCU_IPC_INT6    (6)
#define MCU_IPC_INT7    (7)
#define MCU_IPC_INT8    (8)
#define MCU_IPC_INT9    (9)
#define MCU_IPC_INT10   (10)
#define MCU_IPC_INT11   (11)
#define MCU_IPC_INT12   (12)
#define MCU_IPC_INT13   (13)
#define MCU_IPC_INT14   (14)
#define MCU_IPC_INT15   (15)

/* FIXME: will be removed */
/* Shared register with 64 * 32 words */
#define MAX_MBOX_NUM	64

enum mcu_ipc_region {
	MCU_CP,
	MCU_GNSS,
	MCU_MAX,
};

int mbox_request_irq(enum mcu_ipc_region id, u32 int_num,
		void (*handler)(void *), void *data);
int mbox_enable_irq(enum mcu_ipc_region id, u32 int_num);
int mbox_check_irq(enum mcu_ipc_region id, u32 int_num);
int mbox_disable_irq(enum mcu_ipc_region id, u32 int_num);
int mcu_ipc_unregister_handler(enum mcu_ipc_region id, u32 int_num,
		void (*handler)(void *));
void mbox_set_interrupt(enum mcu_ipc_region id, u32 int_num);
void mcu_ipc_send_command(enum mcu_ipc_region id, u32 int_num, u16 cmd);
u32 mbox_get_value(enum mcu_ipc_region id, u32 mbx_num);
void mbox_set_value(enum mcu_ipc_region id, u32 mbx_num, u32 msg);
void mbox_update_value(enum mcu_ipc_region id, u32 mbx_num,
					u32 msg, u32 mask, u32 pos);
u32 mbox_extract_value(enum mcu_ipc_region id, u32 mbx_num, u32 mask, u32 pos);
void mbox_sw_reset(enum mcu_ipc_region id);
void mcu_ipc_reg_dump(enum mcu_ipc_region id);

#endif
