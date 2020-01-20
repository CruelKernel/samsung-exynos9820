/*
 * drivers/battery/s2mu003_charger.h
 *
 * Header of Richtek S2MU003 Fuelgauge Driver
 *
 * Copyright (C) 2013 Richtek Technology Corp.
 * Patrick Chang <patrick_chang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef S2MU003_CHARGER_H
#define S2MU003_CHARGER_H
#include <linux/mfd/samsung/s2mu003.h>
#include <linux/mfd/samsung/s2mu003_irq.h>

#define S2MU003_CHG_STATUS1	0x00
#define S2MU003_CHG_CTRL1       0x01
#define S2MU003_CHG_CTRL2       0x02
#define S2MU003_CHG_CTRL3       0x04
#define S2MU003_CHG_CTRL4       0x05
#define S2MU003_CHG_CTRL5       0x06
#define S2MU003_SOFTRESET       0x07
#define S2MU003_CHG_CTRL6       0x08
#define S2MU003_CHG_CTRL7       0x09
#define S2MU003_CHG_CTRL8       0x0A
#define S2MU003_CHG_STATUS2	0x0B
#define S2MU003_CHG_STATUS3	0x0C
#define S2MU003_CHG_STATUS4	0x0D
#define S2MU003_CHG_CTRL9	0x0E


#define S2MU003_OTG_SS_ENB_MASK  (1 << 0)
#define S2MU003_OPAMODE_MASK (1 << 0)
#define S2MU003_CHG_EN_MASK  (1 << 6)
#define S2MU003_TIMEREN_MASK (1 << 0)
#define S2MU003_SEL_SWFREQ_MASK   (1 << 2)
#define S2MU003_TEEN_MASK    (1 << 3)
#define S2MU003_AICR_LIMIT_MASK (0x7 << 5)
#define S2MU003_AICR_LIMIT_SHIFT 5
#define S2MU003_MIVR_MASK (0x7 << 5)
#define S2MU003_MIVR_SHIFT 5
#define S2MU003_VOREG_MASK (0x3f << 2)
#define S2MU003_VOREG_SHIFT 2
#define S2MU003_IEOC_MASK 0x07
#define S2MU003_IEOC_SHIFT 0
#define S2MU003_ICHRG_MASK 0xf0
#define S2MU003_ICHRG_SHIFT 4

#define S2MU003_CHG_IRQ1     0x60
#define S2MU003_CHG_IRQ2     0x61
#define S2MU003_CHG_IRQ3     0x62

#define S2MU003_CHG_IRQ_CTRL1     0x63
#define S2MU003_CHG_IRQ_CTRL2     0x64
#define S2MU003_CHG_IRQ_CTRL3     0x65

/* S2MU003_CHG_STAT */
#define S2MU003_EXT_PMOS_CTRL				0x1
#define S2MU003_EXT_PMOS_CTRL_SHIFT			7
#define S2MU003SW_HW_CTRL					0x1
#define S2MU003SW_HW_CTRL_SHIFT				2
#define S2MU003_OTG_SS_DISABLE				0x1
#define S2MU003_OTG_SS_DISABLE_SHIFT			1

/* S2MU003_CHR_CTRL1 */
#define S2MU003_IAICR						0x101
#define S2MU003_IAICR_SHIFT					5
#define S2MU003_HIGHER_OCP					0x1
#define S2MU003_HIGHER_OCP_SHIFT				4
#define S2MU003_TERMINATION_EN				0x0
#define S2MU003_TERMINATION_EN_SHIFT			3
#define S2MU003_SEL_SWFREQ					0x1
#define S2MU003_SEL_SWFREQ_SHIFT				2
#define S2MU003_HIGH_IMPEDANCE				0
#define S2MU003_HIGH_IMPEDANCE_SHIFT			1
#define S2MU003_OPA_MODE						2
#define S2MU003_OPA_MODE_SHIFT				0

/* S2MU003_CHG_CTRL2 */
#define S2MU003_REG_VOLTAGE					0x011100
#define S2MU003_REG_VOLTAGE_SHIFT			2
#define S2MU003_TDEG_EOC						0x0
#define S2MU003_TDEG_EOC_SHIFT				0

/* S2MU003_CHG_CTRL3 */
#define S2MU003_PPC_TE						0x0
#define S2MU003_PPC_TE_SHIFT					7
#define S2MU003_CHG_EN						0x1
#define S2MU003_CHG_EN_SHIFT					6

enum {
	CHG_REG = 0,
	CHG_DATA,
	CHG_REGS,
};

struct charger_info {
	int dummy;
};

#endif /*S2MU003_CHARGER_H*/
