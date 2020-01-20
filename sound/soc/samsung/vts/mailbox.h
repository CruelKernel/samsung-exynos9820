/* sound/soc/samsung/vts/mailbox.h
 *
 * ALSA SoC - Samsung Mailbox driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
  *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MAILBOX_H
#define __MAILBOX_H

/* MAILBOX */
#ifdef CONFIG_SOC_EXYNOS8895
#define MAILBOX_MCUCTRL			(0x0000)
#define MAILBOX_INTGR0			(0x0008)
#define MAILBOX_INTCR0			(0x000C)
#define MAILBOX_INTMR0			(0x0010)
#define MAILBOX_INTSR0			(0x0014)
#define MAILBOX_INTMSR0			(0x0018)
#define MAILBOX_INTGR1			(0x001C)
#define MAILBOX_INTCR1			(0x0020)
#define MAILBOX_INTMR1			(0x0024)
#define MAILBOX_INTSR1			(0x0028)
#define MAILBOX_INTMSR1			(0x002C)
#define MAILBOX_MIF_INIT		(0x004c)
#define MAILBOX_IS_VERSION		(0x0050)
#define MAILBOX_ISSR0			(0x0080)
#define MAILBOX_ISSR1			(0x0084)
#define MAILBOX_ISSR2			(0x0088)
#define MAILBOX_ISSR3			(0x008C)
#define MAILBOX_SEMAPHORE0		(0x0180)
#define MAILBOX_SEMAPHORE1		(0x0184)
#define MAILBOX_SEMAPHORE2		(0x0188)
#define MAILBOX_SEMAPHORE3		(0x018C)
#define MAILBOX_SEMA0CON		(0x01C0)
#define MAILBOX_SEMA0STATE		(0x01C8)
#define MAILBOX_SEMA1CON		(0x01E0)
#define MAILBOX_SEMA1STATE		(0x01E8)
#elif defined(CONFIG_SOC_EXYNOS9810)
#define MAILBOX_MCUCTRL			(0x0000)
#define MAILBOX_INTGR0			(0x001C)
#define MAILBOX_INTCR0			(0x0020)
#define MAILBOX_INTMR0			(0x0024)
#define MAILBOX_INTSR0			(0x0028)
#define MAILBOX_INTMSR0			(0x002C)
#define MAILBOX_INTGR1			(0x0008)
#define MAILBOX_INTCR1			(0x000C)
#define MAILBOX_INTMR1			(0x0010)
#define MAILBOX_INTSR1			(0x0014)
#define MAILBOX_INTMSR1			(0x0018)
#define MAILBOX_MIF_INIT		(0x004C)
#define MAILBOX_IS_VERSION		(0x0050)
#define MAILBOX_ISSR0			(0x0080)
#define MAILBOX_ISSR1			(0x0084)
#define MAILBOX_ISSR2			(0x0088)
#define MAILBOX_ISSR3			(0x008C)
#define MAILBOX_ISSR4			(0x0090)
#define MAILBOX_ISSR5			(0x0094)
#define MAILBOX_SEMAPHORE0		(0x0180)
#define MAILBOX_SEMAPHORE1		(0x0184)
#define MAILBOX_SEMAPHORE2		(0x0188)
#define MAILBOX_SEMAPHORE3		(0x018C)
#define MAILBOX_SEMA0CON		(0x01C0)
#define MAILBOX_SEMA0STATE		(0x01C8)
#define MAILBOX_SEMA1CON		(0x01E0)
#define MAILBOX_SEMA1STATE		(0x01E8)
#elif defined(CONFIG_SOC_EXYNOS9820)
#define MAILBOX_MCUCTRL                 (0x0000)
#define MAILBOX_INTGR0                  (0x0008)
#define MAILBOX_INTCR0                  (0x000C)
#define MAILBOX_INTMR0                  (0x0010)
#define MAILBOX_INTSR0                  (0x0014)
#define MAILBOX_INTMSR0                 (0x0018)
#define MAILBOX_INTGR1                  (0x001C)
#define MAILBOX_INTCR1                  (0x0020)
#define MAILBOX_INTMR1                  (0x0024)
#define MAILBOX_INTSR1                  (0x0028)
#define MAILBOX_INTMSR1                 (0x002C)
#define MAILBOX_MIF_INIT                (0x004c)
#define MAILBOX_IS_VERSION              (0x0070)
#define MAILBOX_ISSR0                   (0x0080)
#define MAILBOX_ISSR1                   (0x0084)
#define MAILBOX_ISSR2                   (0x0088)
#define MAILBOX_ISSR3                   (0x008C)
#define MAILBOX_ISSR4                   (0x0090)
#define MAILBOX_ISSR5                   (0x0094)
#define MAILBOX_SEMAPHORE0              (0x0180)
#define MAILBOX_SEMAPHORE1              (0x0184)
#define MAILBOX_SEMAPHORE2              (0x0188)
#define MAILBOX_SEMAPHORE3              (0x018C)
#define MAILBOX_SEMA0CON                (0x01C0)
#define MAILBOX_SEMA0STATE              (0x01C8)
#define MAILBOX_SEMA1CON                (0x01E0)
#define MAILBOX_SEMA1STATE              (0x01E8)
#endif

/* MAILBOX_MCUCTRL */
#define MAILBOX_MSWRST_OFFSET		(0)
#define MAILBOX_MSWRST_SIZE		(1)
/* MAILBOX_INT*R0 */
#define MAILBOX_INT0_OFFSET		(0)
#define MAILBOX_INT0_SIZE		(16)
/* MAILBOX_INT*R1 */
#define MAILBOX_INT1_OFFSET		(16)
#define MAILBOX_INT1_SIZE		(16)
/* MAILBOX_SEMA?CON */
#define MAILBOX_INT_EN_OFFSET		(3)
#define MAILBOX_INT_EN_SIZE		(1)
/* MAILBOX_SEMA?STATE */
#define MAILBOX_LOCK_OFFSET		(0)
#define MAILBOX_LOCK_SIZE		(1)

#endif /* __MAILBOX_H */
