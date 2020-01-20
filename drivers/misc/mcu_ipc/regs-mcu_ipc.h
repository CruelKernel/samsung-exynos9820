/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 * http://www.samsung.com/
 *
 * Register definition file for Samsung MCU_IPC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_REGS_MCU_IPC_H
#define __ASM_ARCH_REGS_MCU_IPC_H

/***************************************************************/
/* MCU_IPC Registers part				*/
/***************************************************************/
#define EXYNOS_MCU_IPC_MCUCTLR			0x0
#define EXYNOS_MCU_IPC_INTGR0			0x8
#define EXYNOS_MCU_IPC_INTCR0			0xc
#define EXYNOS_MCU_IPC_INTMR0			0x10
#define EXYNOS_MCU_IPC_INTSR0			0x14
#define EXYNOS_MCU_IPC_INTMSR0			0x18
#define EXYNOS_MCU_IPC_INTGR1			0x1c
#define EXYNOS_MCU_IPC_INTCR1			0x20
#define EXYNOS_MCU_IPC_INTMR1			0x24
#define EXYNOS_MCU_IPC_INTSR1			0x28
#define EXYNOS_MCU_IPC_INTMSR1			0x2c
#define EXYNOS_MCU_IPC_ISSR0			0x80
#define EXYNOS_MCU_IPC_ISSR1			0x84
#define EXYNOS_MCU_IPC_ISSR2			0x88
#define EXYNOS_MCU_IPC_ISSR3			0x8c

/***************************************************************/
/* MCU_IPC Bit definition part					*/
/***************************************************************/
/* SYSREG Bit definition */
#define MCU_IPC_MCUCTLR_MSWRST	(0)		/* MCUCTRL S/W Reset */

#define MCU_IPC_RX_INT0		(1 << 16)
#define MCU_IPC_RX_INT1		(1 << 17)
#define MCU_IPC_RX_INT2		(1 << 18)
#define MCU_IPC_RX_INT3		(1 << 19)
#define MCU_IPC_RX_INT4		(1 << 20)
#define MCU_IPC_RX_INT5		(1 << 21)
#define MCU_IPC_RX_INT6		(1 << 22)
#define MCU_IPC_RX_INT7		(1 << 23)
#define MCU_IPC_RX_INT8		(1 << 24)
#define MCU_IPC_RX_INT9		(1 << 25)
#define MCU_IPC_RX_INT10	(1 << 26)
#define MCU_IPC_RX_INT11	(1 << 27)
#define MCU_IPC_RX_INT12	(1 << 28)
#define MCU_IPC_RX_INT13	(1 << 29)
#define MCU_IPC_RX_INT14	(1 << 30)
#define MCU_IPC_RX_INT15	(1 << 31)

#endif /* __ASM_ARCH_REGS_MCU_IPC_H */
