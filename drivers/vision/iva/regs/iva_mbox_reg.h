/*
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _IVA_MAILBOX_REGS_H_
#define _IVA_MAILBOX_REGS_H_

#define M0_VMCU_OFFSET		(0x00)	/* AP to VMCU */
#define M1_VMCU_OFFSET		(0x04)	/* SCore to VMCU */
#define M0_DSP_OFFSET		(0x08)	/* AP to SCore */
#define M1_DSP_OFFSET		(0x0C)	/* VMCU to SCore */
#define M0_AP_OFFSET		(0x10)	/* VMCU to AP */
#define M1_AP_OFFSET		(0x14)	/* SCore to AP */

#define MBOX_CMD_M0_VMCU_ADDR			(0x00000)
#define MBOX_CMD_M0_VMCU_ADDR			(0x00000)
#define MBOX_CMD_M1_VMCU_ADDR			(0x00004)
#define MBOX_CMD_M0_DSP_ADDR			(0x00008)
#define MBOX_CMD_M1_DSP_ADDR			(0x0000C)
#define MBOX_CMD_M0_AP_ADDR			(0x00010)
#define MBOX_CMD_M1_AP_ADDR			(0x00014)
#define MBOX_M1_AP_VMCU_ADDR			(0x00014)

#define MBOX_STATUS_0_ADDR			(0x00040)
#define MBOX_STATUS_1_ADDR			(0x00044)
#define MBOX_IRQ_EN_ADDR			(0x00080)
#define MBOX_IRQ_RAW_STATUS_ADDR		(0x00084)
#define MBOX_IRQ_SOFT_ADDR			(0x00088)
#define MBOX_IRQ_STATUS_ADDR			(0x0008C)
#define MBOX_MESSAGE_BASE_ADDR			(0x00100)

#define MBOX_MESSAGE_PENDING_ADDR		(0x00200)
#define MBOX_SPARE_0_ADDR			(0x0FFE0)
#define MBOX_SPARE_1_ADDR			(0x0FFE4)
#define MBOX_SPARE_2_ADDR			(0x0FFE8)
#define MBOX_SPARE_3_ADDR			(0x0FFEC)

/* for CMD_M0_VMCU(0x00) ~ CMD_M1_AP(0x14)*/
#define MBOX_CMD_VMCU_RESET_S		(0)
#define MBOX_CMD_VMCU_RESET_M		(0x1)
#define MBOX_CMD_VMCU_MSG_TYPE_S	(1)
#define MBOX_CMD_VMCU_MSG_TYPE_M	(0x1)

/* for MBOX_STATUS_0_ADDR(0x40)*/
#define MBOX_STATUS_0_OCC_M1_AP_S	(20)
#define MBOX_STATUS_0_OCC_M0_AP_S	(16)
#define MBOX_STATUS_0_OCC_M1_DSP_S	(12)
#define MBOX_STATUS_0_OCC_M0_DSP_S	(8)
#define MBOX_STATUS_0_OCC_M1_VMCU_S	(4)
#define MBOX_STATUS_0_OCC_M0_VMCU_S	(0)

#define MBOX_STATUS_0_OCC_M1_AP_M	(0x7)
#define MBOX_STATUS_0_OCC_M0_AP_M	(0x7)
#define MBOX_STATUS_0_OCC_M1_DSP_M	(0x7)
#define MBOX_STATUS_0_OCC_M0_DSP_M	(0x7)
#define MBOX_STATUS_0_OCC_M1_VMCU_M	(0x7)
#define MBOX_STATUS_0_OCC_M0_VMCU_M	(0x7)

#define MBOX_M1_AP_WRDY_IRQ_EN_S	(11)
#define MBOX_M1_AP_WRDY_IRQ_EN_M	(0x1 << MBOX_M1_AP_WRDY_IRQ_EN_S)
#define MBOX_M1_AP_PEND_IRQ_EN_S	(10)
#define MBOX_M1_AP_PEND_IRQ_EN_M	(0x1 << MBOX_M1_AP_PEND_IRQ_EN_S)
#define MBOX_M0_AP_WRDY_IRQ_EN_S	(9)
#define MBOX_M0_AP_WRDY_IRQ_EN_M	(0x1 << MBOX_M0_AP_WRDY_IRQ_EN_S)
#define MBOX_M0_AP_PEND_IRQ_EN_S	(8)
#define MBOX_M0_AP_PEND_IRQ_EN_M	(0x1 << MBOX_M0_AP_PEND_IRQ_EN_S)

#define MBOX_IRQ_RAW_STATUS_M1_AP_WRDY_INTR_STAT_S	(11)
#define MBOX_IRQ_RAW_STATUS_M1_AP_PEND_INTR_STAT_S	(10)
#define MBOX_IRQ_RAW_STATUS_M0_AP_WRDY_INTR_STAT_S	(9)
#define MBOX_IRQ_RAW_STATUS_M0_AP_PEND_INTR_STAT_S	(8)
#define MBOX_IRQ_RAW_STATUS_M1_DSP_WRDY_INTR_STAT_S	(7)
#define MBOX_IRQ_RAW_STATUS_M1_DSP_PEND_INTR_STAT_S	(6)
#define MBOX_IRQ_RAW_STATUS_M0_DSP_WRDY_INTR_STAT_S	(5)
#define MBOX_IRQ_RAW_STATUS_M0_DSP_PEND_INTR_STAT_S	(4)
#define MBOX_IRQ_RAW_STATUS_M1_VMCU_WRDY_INTR_STAT_S	(3)
#define MBOX_IRQ_RAW_STATUS_M1_VMCU_PEND_INTR_STAT_S	(2)
#define MBOX_IRQ_RAW_STATUS_M0_VMCU_WRDY_INTR_STAT_S	(1)

#define MBOX_IRQ_RAW_STATUS_M	(0x1)

#endif /* _IVA_MAILBOX_REGS_H_ */
