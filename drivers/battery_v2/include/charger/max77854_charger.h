/*
 * max77854_charger.h
 * Samsung MAX77854 Charger Header
 *
 * Copyright (C) 2015 Samsung Electronics, Inc.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MAX77854_CHARGER_H
#define __MAX77854_CHARGER_H __FILE__

#include <linux/mfd/core.h>
#include <linux/mfd/max77854.h>
#include <linux/mfd/max77854-private.h>
#include <linux/regulator/machine.h>
#include "../sec_charging_common.h"

enum {
	CHIP_ID = 0,
	DATA,
};

ssize_t max77854_chg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t max77854_chg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define MAX77854_CHARGER_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = max77854_chg_show_attrs,			\
	.store = max77854_chg_store_attrs,			\
}

#define MAX77854_CHG_SAFEOUT2                0x80

/* MAX77854_CHG_REG_CHG_INT */
#define MAX77854_BYP_I                  (1 << 0)
#define MAX77854_INP_LIMIT_I		(1 << 1)
#define MAX77854_BATP_I                 (1 << 2)
#define MAX77854_BAT_I                  (1 << 3)
#define MAX77854_CHG_I                  (1 << 4)
#define MAX77854_WCIN_I                 (1 << 5)
#define MAX77854_CHGIN_I                (1 << 6)
#define MAX77854_AICL_I                 (1 << 7)

/* MAX77854_CHG_REG_CHG_INT_MASK */
#define MAX77854_BYP_IM                 (1 << 0)
#define MAX77854_INP_LIMIT_IM		(1 << 1)
#define MAX77854_BATP_IM                (1 << 2)
#define MAX77854_BAT_IM                 (1 << 3)
#define MAX77854_CHG_IM                 (1 << 4)
#define MAX77854_WCIN_IM                (1 << 5)
#define MAX77854_CHGIN_IM               (1 << 6)
#define MAX77854_AICL_IM                (1 << 7)

/* MAX77854_CHG_REG_CHG_INT_OK */
#define MAX77854_BYP_OK                 0x01
#define MAX77854_BYP_OK_SHIFT           0
#define MAX77854_INP_LIMIT_OK		0x02
#define MAX77854_INP_LIMIT_OK_SHIFT	1
#define MAX77854_BATP_OK		0x04
#define MAX77854_BATP_OK_SHIFT		2
#define MAX77854_BAT_OK                 0x08
#define MAX77854_BAT_OK_SHIFT           3
#define MAX77854_CHG_OK                 0x10
#define MAX77854_CHG_OK_SHIFT           4
#define MAX77854_WCIN_OK		0x20
#define MAX77854_WCIN_OK_SHIFT		5
#define MAX77854_CHGIN_OK               0x40
#define MAX77854_CHGIN_OK_SHIFT         6
#define MAX77854_AICL_OK                0x80
#define MAX77854_AICL_OK_SHIFT          7
#define MAX77854_DETBAT                 0x04
#define MAX77854_DETBAT_SHIFT           2

/* MAX77854_CHG_REG_CHG_DTLS_00 */
#define MAX77854_BATP_DTLS		0x01
#define MAX77854_BATP_DTLS_SHIFT	0
#define MAX77854_WCIN_DTLS		0x18
#define MAX77854_WCIN_DTLS_SHIFT	3
#define MAX77854_CHGIN_DTLS             0x60
#define MAX77854_CHGIN_DTLS_SHIFT       5

/* MAX77854_CHG_REG_CHG_DTLS_01 */
#define MAX77854_CHG_DTLS               0x0F
#define MAX77854_CHG_DTLS_SHIFT         0
#define MAX77854_BAT_DTLS               0x70
#define MAX77854_BAT_DTLS_SHIFT         4

/* MAX77854_CHG_REG_CHG_DTLS_02 */
#define MAX77854_BYP_DTLS               0x0F
#define MAX77854_BYP_DTLS_SHIFT         0
#define MAX77854_BYP_DTLS0      0x1
#define MAX77854_BYP_DTLS1      0x2
#define MAX77854_BYP_DTLS2      0x4
#define MAX77854_BYP_DTLS3      0x8

/* MAX77854_CHG_REG_CHG_CNFG_00 */
#define CHG_CNFG_00_MODE_SHIFT		        0
#define CHG_CNFG_00_CHG_SHIFT		        0
#define CHG_CNFG_00_UNO_SHIFT		        1
#define CHG_CNFG_00_OTG_SHIFT		        1
#define CHG_CNFG_00_BUCK_SHIFT		        2
#define CHG_CNFG_00_BOOST_SHIFT		        3
#define CHG_CNFG_00_MODE_MASK		        (0x0F << CHG_CNFG_00_MODE_SHIFT)
#define CHG_CNFG_00_CHG_MASK		        (1 << CHG_CNFG_00_CHG_SHIFT)
#define CHG_CNFG_00_UNO_MASK		        (1 << CHG_CNFG_00_UNO_SHIFT)
#define CHG_CNFG_00_OTG_MASK		        (1 << CHG_CNFG_00_OTG_SHIFT)
#define CHG_CNFG_00_BUCK_MASK		        (1 << CHG_CNFG_00_BUCK_SHIFT)
#define CHG_CNFG_00_BOOST_MASK		        (1 << CHG_CNFG_00_BOOST_SHIFT)
#define CHG_CNFG_00_UNO_CTRL			(CHG_CNFG_00_UNO_MASK | CHG_CNFG_00_BOOST_MASK)
#define CHG_CNFG_00_OTG_CTRL			(CHG_CNFG_00_OTG_MASK | CHG_CNFG_00_BOOST_MASK)
#define MAX77854_MODE_DEFAULT                   0x04
#define MAX77854_MODE_CHGR                      0x01
#define MAX77854_MODE_UNO			0x01
#define MAX77854_MODE_OTG                       0x02
#define MAX77854_MODE_BUCK                      0x04
#define MAX77854_MODE_BOOST		        0x08
#define MAX77854_WDTEN							0x10

/* MAX&7843_CHG_REG_CHG_CNFG_01 */
#define MAX77854_CHG_FQ_2MHz                    (1 << 3)
/* MAX77854_CHG_REG_CHG_CNFG_02 */
#define MAX77854_CHG_CC                         0x3F

/* MAX77854_CHG_REG_CHG_CNFG_03 */
#define MAX77854_CHG_TO_ITH		        0x07

/* MAX77854_CHG_REG_CHG_CNFG_04 */
#define MAX77854_CHG_MINVSYS_MASK               0xC0
#define MAX77854_CHG_MINVSYS_SHIFT		6 
#define MAX77854_CHG_PRM_MASK                   0x1F
#define MAX77854_CHG_PRM_SHIFT                  0

#define CHG_CNFG_04_CHG_CV_PRM_SHIFT            0
#define CHG_CNFG_04_CHG_CV_PRM_MASK             (0x3F << CHG_CNFG_04_CHG_CV_PRM_SHIFT)

/* MAX77854_CHG_CNFG_06 */
#define MAX77854_WDTCLR							0x1

/* MAX77854_CHG_REG_CHG_CNFG_07 */
#define MAX77854_CHG_FMBST			0x04
#define CHG_CNFG_07_REG_FMBST_SHIFT		2
#define CHG_CNFG_07_REG_FMBST_MASK		(0x1 << CHG_CNFG_07_REG_FMBST_SHIFT)
#define CHG_CNFG_07_REG_FGSRC_SHIFT		1
#define CHG_CNFG_07_REG_FGSRC_MASK		(0x1 << CHG_CNFG_07_REG_FGSRC_SHIFT)

/* MAX77854_CHG_REG_CHG_CNFG_09 */
#define MAX77854_CHG_CHGIN_LIM                  0x7F

/* MAX77854_CHG_REG_CHG_CNFG_10 */
#define MAX77854_CHG_WCIN_LIM                   0x3F

/* MAX77854_CHG_REG_CHG_CNFG_12 */
#define MAX77854_CHG_WCINSEL			0x40
#define CHG_CNFG_12_CHGINSEL_SHIFT		5
#define CHG_CNFG_12_CHGINSEL_MASK		(0x1 << CHG_CNFG_12_CHGINSEL_SHIFT)
#define CHG_CNFG_12_WCINSEL_SHIFT		6
#define CHG_CNFG_12_WCINSEL_MASK		(0x1 << CHG_CNFG_12_WCINSEL_SHIFT)
#define CHG_CNFG_12_VCHGIN_REG_MASK		(0x3 << 3)
#define CHG_CNFG_12_WCIN_REG_MASK		(0x3 << 1)
#define CHG_CNFG_12_DISSKIP				(0x1 << 0)

/* MAX77854_CHG_REG_CHG_SWI_INT */
#define MAX77854_SLAVE_TREG_I			(1 << 0)
#define MAX77854_CV_I				(1 << 1)
#define MAX77854_SLAVE_FAULT_I			(1 << 2)

/* MAX77854_CHG_REG_CHG_SWI_INT_MASK */
#define MAX77854_SLAVE_TREG_IM			(1 << 0)
#define MAX77854_CV_IM				(1 << 1)
#define MAX77854_SLAVE_FAULT_IM			(1 << 2)

/* MAX77854_CHG_REG_CHG_SWI_STATUS */
#define MAX77854_SLAVE_TREG_S			0x00
#define MAX77854_CV_S				0x01

/* MAX77854_CHG_REG_CHG_SWI_STATUS */
#define MAX77854_DIS_MIN_SELECTOR		0x80

/* MAX77854_CHG_REG_CHG_SLAVE_READBACK */
#define MAX77854_SWI_READBACK			0x3F

/* MAX77854_CHG_REG_CHG_SLAVE_CNTL */
#define MAX77854_BOVE				0x03

#define REDUCE_CURRENT_STEP						100
#define MINIMUM_INPUT_CURRENT					300
#define SLOW_CHARGING_CURRENT_STANDARD          400

#define WC_CURRENT_STEP		100
#define WC_CURRENT_START	480

struct max77854_charger_data {
	struct device           *dev;
	struct i2c_client       *i2c;
	struct i2c_client       *pmic_i2c;
	struct mutex            charger_mutex;

	struct max77854_platform_data *max77854_pdata;

	struct power_supply	*psy_chg;
	struct power_supply	*psy_otg;

	struct workqueue_struct *wqueue;
	struct work_struct	chgin_work;
	struct delayed_work	aicl_work;
	struct delayed_work	isr_work;
	struct delayed_work	recovery_work;	/*  softreg recovery work */
	struct delayed_work	wpc_work;	/*  wpc detect work */
	struct delayed_work	chgin_init_work;	/*  chgin init work */
	struct delayed_work wc_current_work;

/* mutex */
	struct mutex irq_lock;
	struct mutex ops_lock;

	/* wakelock */
	struct wake_lock recovery_wake_lock;
	struct wake_lock wpc_wake_lock;
	struct wake_lock chgin_wake_lock;
	struct wake_lock wc_current_wake_lock;
	struct wake_lock aicl_wake_lock;

	unsigned int	is_charging;
	unsigned int	charging_type;
	unsigned int	battery_state;
	unsigned int	battery_present;
	unsigned int	cable_type;
	unsigned int	input_current;
	unsigned int	charging_current;
	unsigned int	vbus_state;
	int		aicl_on;
	bool	slow_charging;
	int		status;
	int		charge_mode;
	int uvlo_attach_flag;
	int uvlo_attach_cable_type;

	int		irq_bypass;
	int		irq_batp;

	int		irq_battery;
	int		irq_chg;
	int		irq_wcin;
	int		irq_chgin;
	int		irq_aicl;
	/* software regulation */
	bool		soft_reg_state;
	int		soft_reg_current;

	/* unsufficient power */
	bool		reg_loop_deted;

	/* wireless charge, w(wpc), v(vbus) */
	int		wc_w_gpio;
	int		wc_w_irq;
	int		wc_w_state;
	int		wc_v_gpio;
	int		wc_v_irq;
	int		wc_v_state;
	bool		wc_pwr_det;
	int		soft_reg_recovery_cnt;
	int		wc_current;
	int		wc_pre_current;

	int		jig_gpio;

	bool is_mdock;
	bool otg_on;

	int pmic_ver;
	int input_curr_limit_step;
	int wpc_input_curr_limit_step;
	int charging_curr_step;
	int float_voltage;

	sec_charger_platform_data_t *pdata;
};

#endif /* __MAX77854_CHARGER_H */
