/*
 * max77865-private.h - Voltage regulator driver for the Maxim 77865
 *
 *  Copyright (C) 2016 Samsung Electrnoics
 *  Insun Choi <insun77.choi@samsung.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __LINUX_MFD_MAX77865_PRIV_H
#define __LINUX_MFD_MAX77865_PRIV_H

#include <linux/i2c.h>

#define MAX77865_I2C_ADDR		(0x92)
#define MAX77865_REG_INVALID		(0xff)

#define MAX77865_IRQSRC_CHG		(1 << 0)
#define MAX77865_IRQSRC_TOP		(1 << 1)
#define MAX77865_IRQSRC_FG              (1 << 2)

enum max77865_reg {
	/* Slave addr = 0xCC */
	/* PMIC Top-Level Registers */
	MAX77865_PMIC_REG_PMICID1			= 0x00,
	MAX77865_PMIC_REG_PMICREV			= 0x01,
	MAX77865_PMIC_REG_INTSRC			= 0x22,
	MAX77865_PMIC_REG_INTSRC_MASK			= 0x23,
	MAX77865_PMIC_REG_SYSTEM_INT			= 0x24,
	MAX77865_PMIC_REG_RESERVED_25			= 0x25,
	MAX77865_PMIC_REG_SYSTEM_INT_MASK		= 0x26,
	MAX77865_PMIC_REG_RESERVED_27			= 0x27,
	MAX77865_PMIC_REG_RESERVED_28			= 0x28,
	MAX77865_PMIC_REG_RESERVED_29			= 0x29,
	MAX77865_PMIC_REG_BOOSTCONTROL1			= 0x4C,
	MAX77865_PMIC_REG_BSTOUT_MASK			= 0x03,
	MAX77865_PMIC_REG_BOOSTCONTROL2			= 0x4F,
	MAX77865_PMIC_REG_FORCE_EN_MASK			= 0x08,
	MAX77865_PMIC_REG_SW_RESET			= 0x50,
	MAX77865_PMIC_REG_USBC_RESET			= 0x51,

	MAX77865_PMIC_REG_MAINCTRL1			= 0x02,

//	MAX77865_PMIC_REG_LSCNFG			= 0x2B,
//	MAX77865_PMIC_REG_RESERVED_2C			= 0x2C,
//	MAX77865_PMIC_REG_RESERVED_2D			= 0x2D,

	/* Haptic motor driver Registers */
	MAX77865_PMIC_REG_MCONFIG			= 0x10,

	MAX77865_CHG_REG_INT			= 0xB0,
	MAX77865_CHG_REG_INT_MASK			= 0xB1,
	MAX77865_CHG_REG_INT_OK			= 0xB2,
	MAX77865_CHG_REG_DETAILS_00			= 0xB3,
	MAX77865_CHG_REG_DETAILS_01			= 0xB4,
	MAX77865_CHG_REG_DETAILS_02			= 0xB5,
	MAX77865_CHG_REG_DTLS_03			= 0xB6,
	MAX77865_CHG_REG_CNFG_00			= 0xB7,
	MAX77865_CHG_REG_CNFG_01			= 0xB8,
	MAX77865_CHG_REG_CNFG_02			= 0xB9,
	MAX77865_CHG_REG_CNFG_03			= 0xBA,
	MAX77865_CHG_REG_CNFG_04			= 0xBB,
	MAX77865_CHG_REG_CNFG_05			= 0xBC,
	MAX77865_CHG_REG_CNFG_06			= 0xBD,
	MAX77865_CHG_REG_CNFG_07			= 0xBE,
	MAX77865_CHG_REG_CNFG_08			= 0xBF,
	MAX77865_CHG_REG_CNFG_09			= 0xC0,
	MAX77865_CHG_REG_CNFG_10			= 0xC1,
	MAX77865_CHG_REG_CNFG_11			= 0xC2,
	MAX77865_CHG_REG_CNFG_12			= 0xC3,
	MAX77865_CHG_REG_CNFG_13			= 0xC4,
	MAX77865_CHG_REG_CNFG_14			= 0xC5,
	MAX77865_CHG_REG_SAFEOUT_CTRL			= 0xC6,

	MAX77865_PMIC_REG_END,
};

/* Slave addr = 0x6C : Fuelgauge */
enum max77865_fuelgauge_reg {
	STATUS_REG                                   = 0x00,
	VALRT_THRESHOLD_REG                          = 0x01,
	TALRT_THRESHOLD_REG                          = 0x02,
	SALRT_THRESHOLD_REG                          = 0x03,
	REMCAP_REP_REG                               = 0x05,
	SOCREP_REG                                   = 0x06,
	TEMPERATURE_REG                              = 0x08,
	VCELL_REG                                    = 0x09,
	TIME_TO_EMPTY_REG                            = 0x11,
	FULLSOCTHR_REG                               = 0x13,
	CURRENT_REG                                  = 0x0A,
	AVG_CURRENT_REG                              = 0x0B,
	SOCMIX_REG                                   = 0x0D,
	SOCAV_REG                                    = 0x0E,
	REMCAP_MIX_REG                               = 0x0F,
	FULLCAP_REG                                  = 0x10,
	RFAST_REG                                    = 0x15,
	AVR_TEMPERATURE_REG                          = 0x16,
	CYCLES_REG                                   = 0x17,
	DESIGNCAP_REG                                = 0x18,
	AVR_VCELL_REG                                = 0x19,
	TIME_TO_FULL_REG                             = 0x20,
	CONFIG_REG                                   = 0x1D,
	ICHGTERM_REG                                 = 0x1E,
	REMCAP_AV_REG                                = 0x1F,
	FULLCAP_NOM_REG                              = 0x23,
	FILTER_CFG_REG                               = 0x29,
	MISCCFG_REG                                  = 0x2B,
	QRTABLE20_REG                                = 0x32,
	FULLCAP_REP_REG                              = 0x35,
	RCOMP_REG                                    = 0x38,
	VEMPTY_REG				     = 0x3A,
	FSTAT_REG                                    = 0x3D,
	DISCHARGE_THRESHOLD_REG			     = 0x40,
	QRTABLE30_REG                                = 0x42,
	DQACC_REG                                    = 0x45,
	DPACC_REG                                    = 0x46,
	QH_REG       	                             = 0x4D,
	CONFIG2_REG                                  = 0xBB,
	OCV_REG                                      = 0xEE,
	VFOCV_REG                                    = 0xFB,
	VFSOC_REG                                    = 0xFF,

	MAX77865_FG_END,
};

#define MAX77865_REG_MAINCTRL1_BIASEN		(1 << 7)

/* Slave addr = 0x4A: MUIC */
enum max77865_muic_reg {
	MAX77865_MUIC_REG_DEVICEID		= 0x00,
	MAX77865_MUIC_REG_INT_MAIN		= 0x01,
	MAX77865_MUIC_REG_INT_BC		= 0x02,
	MAX77865_MUIC_REG_INT_FC		= 0x03,
	MAX77865_MUIC_REG_INT_GP		= 0x04,
	MAX77865_MUIC_REG_INTMASK_MAIN		= 0x07,
	MAX77865_MUIC_REG_INTMASK_BC		= 0x08,
	MAX77865_MUIC_REG_INTMASK_FC		= 0x09,
	MAX77865_MUIC_REG_INTMASK_GP		= 0x0A,
	MAX77865_MUIC_REG_STATUS1_BC		= 0x0D,
	MAX77865_MUIC_REG_STATUS2_BC		= 0x0E,
	MAX77865_MUIC_REG_STATUS_GP		= 0x0F,
	MAX77865_MUIC_REG_CONTROL1_BC		= 0x13,
	MAX77865_MUIC_REG_CONTROL2_BC		= 0x14,
	MAX77865_MUIC_REG_CCDET			= 0x15,
	MAX77865_MUIC_REG_CONTROL1		= 0x19,
	MAX77865_MUIC_REG_CONTROL2		= 0x1A,
	MAX77865_MUIC_REG_CONTROL3		= 0x1B,
	MAX77865_MUIC_REG_CONTROL4		= 0x1C,
	MAX77865_MUIC_REG_HVCONTROL1		= 0x1D,
	MAX77865_MUIC_REG_HVCONTROL2		= 0x1E,
	MAX77865_MUIC_REG_HVTXBYTE		= 0x21,
	MAX77865_MUIC_REG_HVRXBYTE1		= 0x22,

	MAX77865_MUIC_REG_END,
};

/* Slave addr = 0x94: RGB LED */
enum max77865_led_reg {
	MAX77865_RGBLED_REG_LEDEN			= 0x30,
	MAX77865_RGBLED_REG_LED0BRT			= 0x31,
	MAX77865_RGBLED_REG_LED1BRT			= 0x32,
	MAX77865_RGBLED_REG_LED2BRT			= 0x33,
	MAX77865_RGBLED_REG_LED3BRT			= 0x34,
	MAX77865_RGBLED_REG_LEDRMP			= 0x36,
	MAX77865_RGBLED_REG_LEDBLNK			= 0x38,
	MAX77865_LED_REG_END,
};

enum max77865_irq_source {
	SYS_INT = 0,
	CHG_INT,
	FUEL_INT,

	MAX77865_IRQ_GROUP_NR,
};

enum max77865_irq {
	/* PMIC; TOPSYS */
	MAX77865_SYSTEM_IRQ_BSTEN_INT,
	MAX77865_SYSTEM_IRQ_SYSUVLO_INT,
	MAX77865_SYSTEM_IRQ_SYSOVLO_INT,
	MAX77865_SYSTEM_IRQ_TSHDN_INT,
	MAX77865_SYSTEM_IRQ_TM_INT,

	/* PMIC; Charger */
	MAX77865_CHG_IRQ_BYP_I,
	MAX77865_CHG_IRQ_BATP_I,
	MAX77865_CHG_IRQ_BAT_I,
	MAX77865_CHG_IRQ_CHG_I,
	MAX77865_CHG_IRQ_WCIN_I,
	MAX77865_CHG_IRQ_CHGIN_I,
	MAX77865_CHG_IRQ_AICL_I,

	/* Fuelgauge */
	MAX77865_FG_IRQ_ALERT,

	MAX77865_IRQ_NR,
};

struct max77865_dev {
	struct device *dev;
	struct i2c_client *i2c; /* 0x92; Haptic, PMIC */
	struct i2c_client *charger; /* 0xD2; Charger */
	struct i2c_client *fuelgauge; /* 0x6C; Fuelgauge */
	struct i2c_client *muic; /* 0x4A; MUIC */
	struct mutex i2c_lock;

	int type;

	int irq;
	int irq_base;
	int irq_gpio;
	bool wakeup;
	struct mutex irqlock;
	int irq_masks_cur[MAX77865_IRQ_GROUP_NR];
	int irq_masks_cache[MAX77865_IRQ_GROUP_NR];

#ifdef CONFIG_HIBERNATION
	/* For hibernation */
	u8 reg_pmic_dump[MAX77865_PMIC_REG_END];
	u8 reg_muic_dump[MAX77865_MUIC_REG_END];
	u8 reg_led_dump[MAX77865_LED_REG_END];
#endif

	/* pmic VER/REV register */
	u8 pmic_rev;	/* pmic Rev */
	u8 pmic_ver;	/* pmic version */

	struct max77865_platform_data *pdata;
};

enum max77865_types {
	TYPE_MAX77865,
};

extern int max77865_irq_init(struct max77865_dev *max77865);
extern void max77865_irq_exit(struct max77865_dev *max77865);

/* MAX77865 shared i2c API function */
extern int max77865_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest);
extern int max77865_bulk_read(struct i2c_client *i2c, u8 reg, int count,
				u8 *buf);
extern int max77865_write_reg(struct i2c_client *i2c, u8 reg, u8 value);
extern int max77865_bulk_write(struct i2c_client *i2c, u8 reg, int count,
				u8 *buf);
extern int max77865_write_word(struct i2c_client *i2c, u8 reg, u16 value);
extern int max77865_read_word(struct i2c_client *i2c, u8 reg);

extern int max77865_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask);

/* MAX77865 check muic path fucntion */
extern bool is_muic_usb_path_ap_usb(void);
extern bool is_muic_usb_path_cp_usb(void);

/* for charger api */
extern void max77865_hv_muic_charger_init(void);

#endif /* __LINUX_MFD_MAX77865_PRIV_H */

