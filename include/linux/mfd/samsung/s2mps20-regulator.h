/*
 * s2mps20-private.h - Voltage regulator driver for the s2mps20
 *
 *  Copyright (C) 2015 Samsung Electrnoics
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

#ifndef __LINUX_MFD_S2MPS20_REGULATOR_H
#define __LINUX_MFD_S2MPS20_REGULATOR_H
#include <linux/i2c.h>

#define S2MPS20_REG_INVALID             (0xFF)
#define S2MPS20_IRQSRC_PMIC				(1 << 0)

/* PMIC Top-Level Registers */
#define	S2MPS20_PMIC_REG_PMICID			0x00
#define	S2MPS20_PMIC_REG_INTSRC			0x01
#define	S2MPS20_PMIC_REG_INTSRC_MASK	0x02
#define S2MPS20_PMIC_REG_OTP_TEST		0x13

/* PMIC Registers */
#define S2MPS20_PMIC_REG_INT1		0x00
#define S2MPS20_PMIC_REG_INT2		0x01
#define S2MPS20_PMIC_REG_INT1M		0x02
#define S2MPS20_PMIC_REG_INT2M		0x03
#define S2MPS20_PMIC_REG_STATUS1	0x04
#define S2MPS20_PMIC_REG_OFFSRC		0x05

#define S2MPS20_PMIC_REG_CTRL1		0x07

#define S2MPS20_PMIC_REG_B1S_CTRL	0x0E
#define S2MPS20_PMIC_REG_B1S_OUT	0x0F
#define S2MPS20_PMIC_REG_B2S_CTRL	0x10
#define S2MPS20_PMIC_REG_B2S_OUT	0x11
#define S2MPS20_PMIC_REG_B3S_CTRL	0x12
#define S2MPS20_PMIC_REG_B3S_OUT	0x13

#define S2MPS20_PMIC_REG_BUCKRAMP	0x14

#define S2MPS20_PMIC_REG_L1CTRL		0x17
#define S2MPS20_PMIC_REG_L2CTRL		0x18
#define S2MPS20_PMIC_REG_L3CTRL		0x19
#define S2MPS20_PMIC_REG_L4CTRL		0x1A
#define S2MPS20_PMIC_REG_L5CTRL		0x1B
#define S2MPS20_PMIC_REG_L6CTRL		0x1C
#define S2MPS20_PMIC_REG_L7CTRL		0x1D
#define S2MPS20_PMIC_REG_L8CTRL		0x1E
#define S2MPS20_PMIC_REG_L9CTRL		0x1F
#define S2MPS20_PMIC_REG_L10CTRL	0x20
#define S2MPS20_PMIC_REG_L11CTRL	0x21
#define S2MPS20_PMIC_REG_L12CTRL	0x22

#define S2MPS20_REG_ADC_CTRL1		0x49
#define S2MPS20_REG_ADC_CTRL2		0x4A
#define S2MPS20_REG_ADC_CTRL3		0x4B
#define S2MPS20_REG_ADC_DATA		0x4C

#define S2MPS20_PMIC_REG_BUCK_OI_EN1	0x45
#define S2MPS20_PMIC_REG_BUCK_OI_CTRL1	0x46
#define S2MPS20_PMIC_REG_BUCK_OI_CTRL2	0x47

#define S2MPS20_PMIC_REG_OCP_WARN3		0x41

/* S2MPS20regulator ids */
enum S2MPS20_regulators {
	S2MPS20_LDO1,
	S2MPS20_LDO2,
/*	S2MPS20_LDO3,
	S2MPS20_LDO4,
	S2MPS20_LDO5,
	S2MPS20_LDO6,
*/	S2MPS20_LDO7,
	S2MPS20_LDO8,
	S2MPS20_LDO9,
	S2MPS20_LDO10,
	S2MPS20_LDO11,
	S2MPS20_LDO12,
	S2MPS20_BUCK1,
	S2MPS20_BUCK2,
//	S2MPS20_BUCK3,
	S2MPS20_REG_MAX,
};

#define MOSCEN_SHIFT		5
#define MOSCEN_MASK			(1 << MOSCEN_SHIFT)

#define S2MPS20_BUCK_MIN1	300000
#define S2MPS20_BUCK_STEP1	6250
#define S2MPS20_LDO_MIN1	300000
#define S2MPS20_LDO_STEP1	25000
#define S2MPS20_LDO_MIN2	700000
#define S2MPS20_LDO_STEP2	25000
#define S2MPS20_LDO_MIN3	1800000
#define S2MPS20_LDO_STEP3	25000
#define S2MPS20_LDO_MIN4	700000
#define S2MPS20_LDO_STEP4	12500

#define S2MPS20_LDO_VSEL_MASK	0x3F
#define S2MPS20_BUCK_VSEL_MASK	0xFF
#define S2MPS20_ENABLE_SHIFT	0x06
#define S2MPS20_ENABLE_MASK	(0x03 << S2MPS20_ENABLE_SHIFT)
#define S2MPS20_SW_ENABLE_MASK	0x03
#define S2MPS20_RAMP_DELAY	6000

#define S2MPS20_ENABLE_TIME_LDO		128
#define S2MPS20_ENABLE_TIME_BUCK	130

#define S2MPS20_LDO_N_VOLTAGES	(S2MPS20_LDO_VSEL_MASK + 1)
#define S2MPS20_BUCK_N_VOLTAGES (S2MPS20_BUCK_VSEL_MASK + 1)

#define S2MPS20_PMIC_EN_SHIFT	6
#define S2MPS20_REGULATOR_MAX (S2MPS20_REG_MAX)
#define SEC_PMIC_REV(iodev)    (iodev)->pmic_rev

#define S2MPS20_OCP_WARN_EN_SHIFT		7
#define S2MPS20_OCP_WARN_CNT_SHIFT		6
#define S2MPS20_OCP_WARN_DVS_MASK_SHIFT	5
#define S2MPS20_OCP_WARN_LV_SHIFT		0


#define S2MPS20_OCP_WARN_EN_SHIFT	7

#define CURRENT_BS			15625000
#define CURRENT_BD			31250000
#define CURRENT_BV			15625000
#define CURRENT_BB			15625000
#define CURRENT_L150		 1171815
#define CURRENT_L300		 2343750
#define CURRENT_L450		 3515625
#define CURRENT_L600		 4687500
#define CURRENT_L800		 6250000

#define POWER_BS			25000
#define POWER_BD			50000
#define POWER_BV			50000
#define POWER_BB			50000
#define POWER_N150			3750
#define POWER_N300			7500
#define POWER_N450		 	11250
#define POWER_N600		 	15000
#define POWER_N800		 	20000
#define POWER_P150			7500
#define POWER_P300			15000
#define POWER_P450		 	22500
#define POWER_P600		 	60000
#define POWER_P800		 	80000
#define POWER_D150			7500
#define POWER_D300			15000
#define POWER_D450			22500

#define ADC_EN_MASK				0x80
#define ADC_ASYNCRD_MASK		0x80
#define ADC_CAL_EN_MASK			0x40
#define ADC_PTR_MASK			0x3F

#define MUX_PTR_BASE			0x10
#define CURRENT_PTR_BASE		0x00
#define POWER_PTR_BASE			0x06

#define S2MPS20_LDO_START		0x11
#define S2MPS20_LDO_END			0x1C
#define S2MPS20_BUCK_START		0x01
#define S2MPS20_BUCK_END		0x03
#define S2MPS20_LDO_CNT			12
#define S2MPS20_BUCK_CNT		3

#define SMPNUM_MASK			0x0F

#define SEC_PMIC_REV(iodev)	(iodev)->pmic_rev
#define S2MPS20_MAX_ADC_CHANNEL		3
/*
 * sec_opmode_data - regulator operation mode data
 * @id: regulator id
 * @mode: regulator operation mode
 */

enum s2mps20_irq_source {
	S2MPS20_PMIC_INT1 = 0,
	S2MPS20_PMIC_INT2,

	S2MPS20_IRQ_GROUP_NR,
};

#define S2MPS20_NUM_IRQ_PMIC_REGS	2

enum s2mps20_irq {
	/* PMIC */
	S2MPS20_PMIC_IRQ_PWRONF_INT1,
	S2MPS20_PMIC_IRQ_PWRONR_INT1,
	S2MPS20_PMIC_IRQ_OC0_INT1,
	S2MPS20_PMIC_IRQ_OC1_INT1,
	S2MPS20_PMIC_IRQ_OC2_INT1,
	S2MPS20_PMIC_IRQ_WTSR_INT1,
	S2MPS20_PMIC_IRQ_ADCDONE_INT1,
	S2MPS20_PMIC_IRQ_WRSTB_INT1,

	S2MPS20_PMIC_IRQ_INT120C_INT2,
	S2MPS20_PMIC_IRQ_INT140C_INT2,
	S2MPS20_PMIC_IRQ_TSD_INT2,
	S2MPS20_PMIC_IRQ_OVP_INT2,
	S2MPS20_PMIC_IRQ_OCP_B1_INT2,
	S2MPS20_PMIC_IRQ_OCP_B2_INT2,
	S2MPS20_PMIC_IRQ_OCP_B3_INT2,

	S2MPS20_IRQ_NR,
};


enum sec_device_type {
	S2MPS20X,
};

struct s2mps20_dev {
	struct device *dev;
	struct i2c_client *i2c;
	struct i2c_client *pmic;
	struct i2c_client *debug_i2c;
	struct mutex i2c_lock;
	struct apm_ops *ops;

	int type;
	int device_type;
	int irq;
	int irq_base;
	int irq_gpio;
	bool wakeup;
	struct mutex irqlock;
	int irq_masks_cur[S2MPS20_IRQ_GROUP_NR];
	int irq_masks_cache[S2MPS20_IRQ_GROUP_NR];

	/* pmic VER/REV register */
	u8 pmic_rev;	/* pmic Rev */
	int adc_mode;
	int adc_sync_mode;

	struct s2mps20_platform_data *pdata;
};

enum s2mps20_types {
	TYPE_S2MPS20,
};

extern int s2mps20_irq_init(struct s2mps20_dev *s2mps20);
extern void s2mps20_irq_exit(struct s2mps20_dev *s2mps20);

extern void s2mps20_powermeter_init(struct s2mps20_dev *s2mps20);
extern void s2mps20_powermeter_deinit(struct s2mps20_dev *s2mps20);

/* S2MPS20 shared i2c API function */
extern int s2mps20_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest);
extern int s2mps20_bulk_read(struct i2c_client *i2c, u8 reg, int count,
				u8 *buf);
extern int s2mps20_write_reg(struct i2c_client *i2c, u8 reg, u8 value);
extern int s2mps20_bulk_write(struct i2c_client *i2c, u8 reg, int count,
				u8 *buf);
extern int s2mps20_write_word(struct i2c_client *i2c, u8 reg, u16 value);
extern int s2mps20_read_word(struct i2c_client *i2c, u8 reg);

extern int s2mps20_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask);

#ifdef CONFIG_EXYNOS_OCP
extern void get_s2mps20_i2c(struct i2c_client **i2c);
#endif

#ifdef CONFIG_SEC_PM
extern struct device *ap_sub_pmic_dev;
#endif /* CONFIG_SEC_PM */
#endif /* __LINUX_MFD_S2MPS20_REGULATOR_H */

