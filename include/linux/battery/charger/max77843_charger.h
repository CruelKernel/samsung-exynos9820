/*
 * max77843_charger.h
 * Samsung MAX77843 Charger Header
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
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

#ifndef __MAX77843_CHARGER_H
#define __MAX77843_CHARGER_H __FILE__

#include <linux/mfd/core.h>
#include <linux/mfd/max77843.h>
#include <linux/mfd/max77843-private.h>
#include <linux/regulator/machine.h>

#define MAX77843_CHG_SAFEOUT2                0x80

/* MAX77843_CHG_REG_CHG_INT */
#define MAX77843_BYP_I                  (1 << 0)
#define MAX77843_BATP_I			(1 << 2)
#define MAX77843_BAT_I                  (1 << 3)
#define MAX77843_CHG_I                  (1 << 4)
#define MAX77843_WCIN_I			(1 << 5)
#define MAX77843_CHGIN_I                (1 << 6)

/* MAX77843_CHG_REG_CHG_INT_MASK */
#define MAX77843_BYP_IM                 (1 << 0)
#define MAX77843_THM_IM                 (1 << 2)
#define MAX77843_BAT_IM                 (1 << 3)
#define MAX77843_CHG_IM                 (1 << 4)
#define MAX77843_WCIN_IM		(1 << 5)
#define MAX77843_CHGIN_IM               (1 << 6)

/* MAX77843_CHG_REG_CHG_INT_OK */
#define MAX77843_BYP_OK                 0x01
#define MAX77843_BYP_OK_SHIFT           0
#define MAX77843_BATP_OK		0x04
#define MAX77843_BATP_OK_SHIFT		2
#define MAX77843_BAT_OK                 0x08
#define MAX77843_BAT_OK_SHIFT           3
#define MAX77843_CHG_OK                 0x10
#define MAX77843_CHG_OK_SHIFT           4
#define MAX77843_WCIN_OK		0x20
#define MAX77843_WCIN_OK_SHIFT		5
#define MAX77843_CHGIN_OK               0x40
#define MAX77843_CHGIN_OK_SHIFT         6
#define MAX77843_DETBAT                 0x04
#define MAX77843_DETBAT_SHIFT           2

/* MAX77843_CHG_REG_CHG_DTLS_00 */
#define MAX77843_BATP_DTLS		0x01
#define MAX77843_BATP_DTLS_SHIFT	0
#define MAX77843_WCIN_DTLS		0x18
#define MAX77843_WCIN_DTLS_SHIFT	3
#define MAX77843_CHGIN_DTLS             0x60
#define MAX77843_CHGIN_DTLS_SHIFT       5

/* MAX77843_CHG_REG_CHG_DTLS_01 */
#define MAX77843_CHG_DTLS               0x0F
#define MAX77843_CHG_DTLS_SHIFT         0
#define MAX77843_BAT_DTLS               0x70
#define MAX77843_BAT_DTLS_SHIFT         4

/* MAX77843_CHG_REG_CHG_DTLS_02 */
#define MAX77843_BYP_DTLS               0x0F
#define MAX77843_BYP_DTLS_SHIFT         0
#define MAX77843_BYP_DTLS0      0x1
#define MAX77843_BYP_DTLS1      0x2
#define MAX77843_BYP_DTLS2      0x4
#define MAX77843_BYP_DTLS3      0x8

/* MAX77843_CHG_REG_CHG_CNFG_00 */
#define CHG_CNFG_00_MODE_SHIFT		        0
#define CHG_CNFG_00_CHG_SHIFT		        0
#define CHG_CNFG_00_OTG_SHIFT		        1
#define CHG_CNFG_00_BUCK_SHIFT		        2
#define CHG_CNFG_00_BOOST_SHIFT		        3
#define CHG_CNFG_00_MODE_MASK		        (0xf << CHG_CNFG_00_MODE_SHIFT)
#define CHG_CNFG_00_CHG_MASK		        (1 << CHG_CNFG_00_CHG_SHIFT)
#define CHG_CNFG_00_OTG_MASK		        (1 << CHG_CNFG_00_OTG_SHIFT)
#define CHG_CNFG_00_BUCK_MASK		        (1 << CHG_CNFG_00_BUCK_SHIFT)
#define CHG_CNFG_00_BOOST_MASK		        (1 << CHG_CNFG_00_BOOST_SHIFT)
#define CHG_CNFG_00_OTG_CTRL			(CHG_CNFG_00_OTG_MASK | CHG_CNFG_00_BOOST_MASK)
#define MAX77843_MODE_DEFAULT                   0x04
#define MAX77843_MODE_CHGR                      0x01
#define MAX77843_MODE_OTG                       0x02
#define MAX77843_MODE_BUCK                      0x04
#define MAX77843_MODE_BOOST		        0x08

/* MAX&7843_CHG_REG_CHG_CNFG_01 */
#define MAX77843_CHG_FQ_2MHz                    (1 << 3)
/* MAX77843_CHG_REG_CHG_CNFG_02 */
#define MAX77843_CHG_CC                         0x3F

/* MAX77843_CHG_REG_CHG_CNFG_03 */
#define MAX77843_CHG_TO_ITH		        0x07

/* MAX77843_CHG_REG_CHG_CNFG_04 */
#define MAX77843_CHG_MINVSYS_MASK               0xE0
#define MAX77843_CHG_MINVSYS_SHIFT              5
#define MAX77843_CHG_PRM_MASK                   0x1F
#define MAX77843_CHG_PRM_SHIFT                  0

#define CHG_CNFG_04_CHG_CV_PRM_SHIFT            0
#define CHG_CNFG_04_CHG_CV_PRM_MASK             (0x3F << CHG_CNFG_04_CHG_CV_PRM_SHIFT)

/* MAX77843_CHG_REG_CHG_CNFG_09 */
#define MAX77843_CHG_CHGIN_LIM                  0x7F

/* MAX77843_CHG_REG_CHG_CNFG_10 */
#define MAX77843_CHG_WCIN_LIM                   0x3F

/* MAX77843_CHG_REG_CHG_CNFG_12 */
#define MAX77843_CHG_WCINSEL		        0x40

/* MAX77843_CHG_REG_CHG_DTLS_00 */
#define MAX77843_BATP_DTLS		        0x01
#define MAX77843_BATP_DTLS_SHIFT	        0
#define MAX77843_WCIN_DTLS		        0x18
#define MAX77843_WCIN_DTLS_SHIFT	        3
#define MAX77843_CHGIN_DTLS                     0x60
#define MAX77843_CHGIN_DTLS_SHIFT               5

/* MAX77843_CHG_DETAILS_01 */
#define MAX77843_CHG_DTLS                       0x0F
#define MAX77843_CHG_DTLS_SHIFT                 0
#define MAX77843_BAT_DTLS                       0x70
#define MAX77843_BAT_DTLS_SHIFT                 4

#define SIOP_INPUT_LIMIT_CURRENT                1200
#define SIOP_CHARGING_LIMIT_CURRENT             1000
#define SIOP_WIRELESS_INPUT_LIMIT_CURRENT       660
#define SIOP_WIRELESS_CHARGING_LIMIT_CURRENT    780
#define SLOW_CHARGING_CURRENT_STANDARD          400

struct max77843_charger_data {
	struct device           *dev;
	struct i2c_client       *i2c;
	struct i2c_client       *pmic_i2c;
	struct mutex            charger_mutex;

	struct max77843_platform_data *max77843_pdata;

	struct power_supply	psy_chg;

	struct workqueue_struct *wqueue;
	struct work_struct	chgin_work;
	struct delayed_work	isr_work;
	struct delayed_work	recovery_work;	/*  softreg recovery work */
	struct delayed_work	wpc_work;	/*  wpc detect work */
	struct delayed_work	chgin_init_work;	/*  chgin init work */
	struct delayed_work     afc_work;

/* mutex */
	struct mutex irq_lock;
	struct mutex ops_lock;

	/* wakelock */
	struct wake_lock recovery_wake_lock;
	struct wake_lock wpc_wake_lock;
	struct wake_lock chgin_wake_lock;

	unsigned int	is_charging;
	unsigned int	charging_type;
	unsigned int	battery_state;
	unsigned int	battery_present;
	unsigned int	cable_type;
	unsigned int	charging_current_max;
	unsigned int	charging_current;
	unsigned int	input_current_limit;
	unsigned int	vbus_state;
	int		aicl_on;
	int		status;
	int		siop_level;
	int uvlo_attach_flag;
	int uvlo_attach_cable_type;

	int		irq_bypass;
	int		irq_batp;

	int		irq_battery;
	int		irq_chg;
	int		irq_wcin;
	int		irq_chgin;

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

	bool afc_detect;
	bool is_mdock;

	int pmic_ver;
	int input_curr_limit_step;
	int wpc_input_curr_limit_step;
	int charging_curr_step;

	sec_battery_platform_data_t	*pdata;
};

#endif /* __MAX77843_CHARGER_H */
