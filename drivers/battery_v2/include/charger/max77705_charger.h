/*
 * max77705_charger.h
 * Samsung max77705 Charger Header
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

#ifndef __MAX77705_CHARGER_H
#define __MAX77705_CHARGER_H __FILE__

#include <linux/mfd/core.h>
#include <linux/mfd/max77705.h>
#include <linux/mfd/max77705-private.h>
#include <linux/regulator/machine.h>
#include "../sec_charging_common.h"

enum {
	CHIP_ID = 0,
	DATA,
};

extern unsigned int lpcharge;
extern bool mfc_fw_update;

extern void max77705_set_fw_noautoibus(int enable);

ssize_t max77705_chg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t max77705_chg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define MAX77705_CHARGER_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = max77705_chg_show_attrs,			\
	.store = max77705_chg_store_attrs,			\
}

#define MAX77705_CHG_SAFEOUT2                0x80

/* MAX77705_CHG_REG_CHG_INT */
#define MAX77705_BYP_I                  (1 << 0)
#define MAX77705_INP_LIMIT_I		(1 << 1)
#define MAX77705_BATP_I                 (1 << 2)
#define MAX77705_BAT_I                  (1 << 3)
#define MAX77705_CHG_I                  (1 << 4)
#define MAX77705_WCIN_I                 (1 << 5)
#define MAX77705_CHGIN_I                (1 << 6)
#define MAX77705_AICL_I                 (1 << 7)

/* MAX77705_CHG_REG_CHG_INT_MASK */
#define MAX77705_BYP_IM                 (1 << 0)
#define MAX77705_INP_LIMIT_IM		(1 << 1)
#define MAX77705_BATP_IM                (1 << 2)
#define MAX77705_BAT_IM                 (1 << 3)
#define MAX77705_CHG_IM                 (1 << 4)
#define MAX77705_WCIN_IM                (1 << 5)
#define MAX77705_CHGIN_IM               (1 << 6)
#define MAX77705_AICL_IM                (1 << 7)

/* MAX77705_CHG_REG_CHG_INT_OK */
#define MAX77705_BYP_OK                 0x01
#define MAX77705_BYP_OK_SHIFT           0
#define MAX77705_DISQBAT_OK		0x02
#define MAX77705_DISQBAT_OK_SHIFT	1
#define MAX77705_BATP_OK		0x04
#define MAX77705_BATP_OK_SHIFT		2
#define MAX77705_BAT_OK                 0x08
#define MAX77705_BAT_OK_SHIFT           3
#define MAX77705_CHG_OK                 0x10
#define MAX77705_CHG_OK_SHIFT           4
#define MAX77705_WCIN_OK		0x20
#define MAX77705_WCIN_OK_SHIFT		5
#define MAX77705_CHGIN_OK               0x40
#define MAX77705_CHGIN_OK_SHIFT         6
#define MAX77705_AICL_OK                0x80
#define MAX77705_AICL_OK_SHIFT          7
#define MAX77705_DETBAT                 0x04
#define MAX77705_DETBAT_SHIFT           2

/* MAX77705_CHG_REG_CHG_DTLS_00 */
#define MAX77705_BATP_DTLS		0x01
#define MAX77705_BATP_DTLS_SHIFT	0
#define MAX77705_WCIN_DTLS		0x18
#define MAX77705_WCIN_DTLS_SHIFT	3
#define MAX77705_CHGIN_DTLS             0x60
#define MAX77705_CHGIN_DTLS_SHIFT       5

/* MAX77705_CHG_REG_CHG_DTLS_01 */
#define MAX77705_CHG_DTLS               0x0F
#define MAX77705_CHG_DTLS_SHIFT         0
#define MAX77705_BAT_DTLS               0x70
#define MAX77705_BAT_DTLS_SHIFT         4

/* MAX77705_CHG_REG_CHG_DTLS_02 */
#define MAX77705_BYP_DTLS               0x0F
#define MAX77705_BYP_DTLS_SHIFT         0
#define MAX77705_BYP_DTLS0      0x1
#define MAX77705_BYP_DTLS1      0x2
#define MAX77705_BYP_DTLS2      0x4
#define MAX77705_BYP_DTLS3      0x8

#if 1
/* MAX77705_CHG_REG_CHG_CNFG_00 */
#define CHG_CNFG_00_MODE_SHIFT		        0
#define CHG_CNFG_00_CHG_SHIFT		        0
#define CHG_CNFG_00_UNO_SHIFT		        1
#define CHG_CNFG_00_OTG_SHIFT		        1
#define CHG_CNFG_00_BUCK_SHIFT		        2
#define CHG_CNFG_00_BOOST_SHIFT		        3
#define CHG_CNFG_00_WDTEN_SHIFT		        4
#define CHG_CNFG_00_MODE_MASK		        (0x0F << CHG_CNFG_00_MODE_SHIFT)
#define CHG_CNFG_00_CHG_MASK		        (1 << CHG_CNFG_00_CHG_SHIFT)
#define CHG_CNFG_00_UNO_MASK		        (1 << CHG_CNFG_00_UNO_SHIFT)
#define CHG_CNFG_00_OTG_MASK		        (1 << CHG_CNFG_00_OTG_SHIFT)
#define CHG_CNFG_00_BUCK_MASK		        (1 << CHG_CNFG_00_BUCK_SHIFT)
#define CHG_CNFG_00_BOOST_MASK		        (1 << CHG_CNFG_00_BOOST_SHIFT)
#define CHG_CNFG_00_WDTEN_MASK		        (1 << CHG_CNFG_00_WDTEN_SHIFT)
#define CHG_CNFG_00_UNO_CTRL			(CHG_CNFG_00_UNO_MASK | CHG_CNFG_00_BOOST_MASK)
#define CHG_CNFG_00_OTG_CTRL			(CHG_CNFG_00_OTG_MASK | CHG_CNFG_00_BOOST_MASK)
#define MAX77705_MODE_DEFAULT			0x04
#define MAX77705_MODE_CHGR			0x01
#define MAX77705_MODE_UNO			0x01
#define MAX77705_MODE_OTG			0x02
#define MAX77705_MODE_BUCK			0x04
#define MAX77705_MODE_BOOST			0x08
#endif
#define CHG_CNFG_00_MODE_SHIFT		        0
#define CHG_CNFG_00_MODE_MASK		        (0x0F << CHG_CNFG_00_MODE_SHIFT)
#define CHG_CNFG_00_WDTEN_SHIFT		        4
#define CHG_CNFG_00_WDTEN_MASK		        (1 << CHG_CNFG_00_WDTEN_SHIFT)

/* MAX77705_CHG_REG_CHG_CNFG_00 MODE[3:0] */
#define MAX77705_MODE_0_ALL_OFF						0x0
#define MAX77705_MODE_1_ALL_OFF						0x1
#define MAX77705_MODE_2_ALL_OFF						0x2
#define MAX77705_MODE_3_ALL_OFF						0x3
#define MAX77705_MODE_4_BUCK_ON						0x4
#define MAX77705_MODE_5_BUCK_CHG_ON					0x5
#define MAX77705_MODE_6_BUCK_CHG_ON					0x6
#define MAX77705_MODE_7_BUCK_CHG_ON					0x7
#define MAX77705_MODE_8_BOOST_UNO_ON				0x8
#define MAX77705_MODE_9_BOOST_ON					0x9
#define MAX77705_MODE_A_BOOST_OTG_ON				0xA
#define MAX77705_MODE_B_RESERVED					0xB
#define MAX77705_MODE_C_BUCK_BOOST_UNO_ON				0xC
#define MAX77705_MODE_D_BUCK_CHG_BOOST_UNO_ON			0xD
#define MAX77705_MODE_E_BUCK_BOOST_OTG_ON				0xE
#define MAX77705_MODE_F_BUCK_CHG_BOOST_OTG_ON			0xF

/* MAX77705_CHG_REG_CHG_CNFG_01 */
#define CHG_CNFG_01_FCHGTIME_SHIFT			0
#define CHG_CNFG_01_FCHGTIME_MASK			(0x7 << CHG_CNFG_01_FCHGTIME_SHIFT)
#define MAX77705_FCHGTIME_DISABLE			0x0

#define CHG_CNFG_01_RECYCLE_EN_SHIFT	3
#define CHG_CNFG_01_RECYCLE_EN_MASK	(0x1 << CHG_CNFG_01_RECYCLE_EN_SHIFT)
#define MAX77705_RECYCLE_EN_ENABLE	0x1

#define CHG_CNFG_01_CHG_RSTRT_SHIFT	4
#define CHG_CNFG_01_CHG_RSTRT_MASK	(0x3 << CHG_CNFG_01_CHG_RSTRT_SHIFT)
#define MAX77705_CHG_RSTRT_DISABLE	0x3

#define CHG_CNFG_01_PQEN_SHIFT			7
#define CHG_CNFG_01_PQEN_MASK			(0x1 << CHG_CNFG_01_PQEN_SHIFT)
#define MAX77705_CHG_PQEN_DISABLE		0x0
#define MAX77705_CHG_PQEN_ENABLE		0x1

/* MAX77705_CHG_REG_CHG_CNFG_02 */
#define CHG_CNFG_02_OTG_ILIM_SHIFT		6
#define CHG_CNFG_02_OTG_ILIM_MASK		(0x3 << CHG_CNFG_02_OTG_ILIM_SHIFT)
#define MAX77705_OTG_ILIM_500		0x0
#define MAX77705_OTG_ILIM_900		0x1
#define MAX77705_OTG_ILIM_1200		0x2
#define MAX77705_OTG_ILIM_1500		0x3
#define MAX77705_CHG_CC                         0x3F

/* MAX77705_CHG_REG_CHG_CNFG_03 */
#define CHG_CNFG_03_TO_ITH_SHIFT		0
#define CHG_CNFG_03_TO_ITH_MASK			(0x7 << CHG_CNFG_03_TO_ITH_SHIFT)
#define MAX77705_TO_ITH_150MA			0x0

#define CHG_CNFG_03_TO_TIME_SHIFT		3
#define CHG_CNFG_03_TO_TIME_MASK			(0x7 << CHG_CNFG_03_TO_TIME_SHIFT)
#define MAX77705_TO_TIME_30M			0x3
#define MAX77705_TO_TIME_70M			0x7

#define CHG_CNFG_03_REG_AUTO_SHIPMODE_SHIFT		6
#define CHG_CNFG_03_REG_AUTO_SHIPMODE_MASK		(0x1 << CHG_CNFG_03_REG_AUTO_SHIPMODE_SHIFT)

#define CHG_CNFG_03_SYS_TRACK_DIS_SHIFT		7
#define CHG_CNFG_03_SYS_TRACK_DIS_MASK		(0x1 << CHG_CNFG_03_SYS_TRACK_DIS_SHIFT)
#define MAX77705_SYS_TRACK_ENABLE	        0x0
#define MAX77705_SYS_TRACK_DISABLE	        0x1

/* MAX77705_CHG_REG_CHG_CNFG_04 */
#define MAX77705_CHG_MINVSYS_MASK               0xC0
#define MAX77705_CHG_MINVSYS_SHIFT		6
#define MAX77705_CHG_PRM_MASK                   0x1F
#define MAX77705_CHG_PRM_SHIFT                  0

#define CHG_CNFG_04_CHG_CV_PRM_SHIFT            0
#define CHG_CNFG_04_CHG_CV_PRM_MASK             (0x3F << CHG_CNFG_04_CHG_CV_PRM_SHIFT)

/* MAX77705_CHG_REG_CHG_CNFG_05 */
#define CHG_CNFG_05_REG_B2SOVRC_SHIFT	0
#define CHG_CNFG_05_REG_B2SOVRC_MASK	(0xF << CHG_CNFG_05_REG_B2SOVRC_SHIFT)
#define MAX77705_B2SOVRC_DISABLE	0x0
#define MAX77705_B2SOVRC_4_6A		0x7
#define MAX77705_B2SOVRC_4_8A		0x8
#define MAX77705_B2SOVRC_5_0A		0x9
#define MAX77705_B2SOVRC_5_2A		0xA
#define MAX77705_B2SOVRC_5_4A		0xB
#define MAX77705_B2SOVRC_5_6A		0xC
#define MAX77705_B2SOVRC_5_8A		0xD
#define MAX77705_B2SOVRC_6_0A		0xE
#define MAX77705_B2SOVRC_6_2A		0xF

#define CHG_CNFG_05_REG_UNOILIM_SHIFT	4
#define CHG_CNFG_05_REG_UNOILIM_MASK	(0x7 << CHG_CNFG_05_REG_UNOILIM_SHIFT)
#define MAX77705_UNOILIM_200		0x1
#define MAX77705_UNOILIM_300		0x2
#define MAX77705_UNOILIM_400		0x3
#define MAX77705_UNOILIM_600		0x4
#define MAX77705_UNOILIM_800		0x5
#define MAX77705_UNOILIM_1000		0x6
#define MAX77705_UNOILIM_1500		0x7

/* MAX77705_CHG_CNFG_06 */
#define CHG_CNFG_06_WDTCLR_SHIFT		0
#define CHG_CNFG_06_WDTCLR_MASK			(0x3 << CHG_CNFG_06_WDTCLR_SHIFT)
#define MAX77705_WDTCLR				0x01
#define CHG_CNFG_06_DIS_AICL_SHIFT		4
#define CHG_CNFG_06_DIS_AICL_MASK		(0x1 << CHG_CNFG_06_DIS_AICL_SHIFT)
#define MAX77705_DIS_AICL			0x0
#define CHG_CNFG_06_B2SOVRC_DTC_SHIFT	7
#define CHG_CNFG_06_B2SOVRC_DTC_MASK	(0x1 << CHG_CNFG_06_B2SOVRC_DTC_SHIFT)
#define MAX77705_B2SOVRC_DTC_100MS		0x1

/* MAX77705_CHG_REG_CHG_CNFG_07 */
#define MAX77705_CHG_FMBST			0x04
#define CHG_CNFG_07_REG_FMBST_SHIFT		2
#define CHG_CNFG_07_REG_FMBST_MASK		(0x1 << CHG_CNFG_07_REG_FMBST_SHIFT)
#define CHG_CNFG_07_REG_FGSRC_SHIFT		1
#define CHG_CNFG_07_REG_FGSRC_MASK		(0x1 << CHG_CNFG_07_REG_FGSRC_SHIFT)
#define CHG_CNFG_07_REG_SHIPMODE_SHIFT		0
#define CHG_CNFG_07_REG_SHIPMODE_MASK		(0x1 << CHG_CNFG_07_REG_SHIPMODE_SHIFT)

/* MAX77705_CHG_REG_CHG_CNFG_08 */
#define CHG_CNFG_08_REG_FSW_SHIFT	0
#define CHG_CNFG_08_REG_FSW_MASK	(0x3 << CHG_CNFG_08_REG_FSW_SHIFT)
#define MAX77705_CHG_FSW_3MHz		0x00
#define MAX77705_CHG_FSW_2MHz		0x01
#define MAX77705_CHG_FSW_1_5MHz		0x02

/* MAX77705_CHG_REG_CHG_CNFG_09 */
#define MAX77705_CHG_CHGIN_LIM                  0x7F
#define MAX77705_CHG_EN                         0x80

/* MAX77705_CHG_REG_CHG_CNFG_10 */
#define MAX77705_CHG_WCIN_LIM                   0x3F

/* MAX77705_CHG_REG_CHG_CNFG_11 */
#define CHG_CNFG_11_VBYPSET_SHIFT		0
#define CHG_CNFG_11_VBYPSET_MASK		(0x7F << CHG_CNFG_11_VBYPSET_SHIFT)

/* MAX77705_CHG_REG_CHG_CNFG_12 */
#define MAX77705_CHG_WCINSEL			0x40
#define CHG_CNFG_12_CHGINSEL_SHIFT		5
#define CHG_CNFG_12_CHGINSEL_MASK		(0x1 << CHG_CNFG_12_CHGINSEL_SHIFT)
#define CHG_CNFG_12_WCINSEL_SHIFT		6
#define CHG_CNFG_12_WCINSEL_MASK		(0x1 << CHG_CNFG_12_WCINSEL_SHIFT)
#define CHG_CNFG_12_VCHGIN_REG_MASK		(0x3 << 3)
#define CHG_CNFG_12_WCIN_REG_MASK		(0x3 << 1)
#define CHG_CNFG_12_REG_DISKIP_SHIFT		0
#define CHG_CNFG_12_REG_DISKIP_MASK		(0x1 << CHG_CNFG_12_REG_DISKIP_SHIFT)
#define MAX77705_DISABLE_SKIP			0x1
#define MAX77705_AUTO_SKIP			0x0

/* MAX77705_CHG_REG_CHG_SWI_INT */
#define MAX77705_SLAVE_TREG_I			(1 << 0)
#define MAX77705_CV_I				(1 << 1)
#define MAX77705_SLAVE_FAULT_I			(1 << 2)

/* MAX77705_CHG_REG_CHG_SWI_INT_MASK */
#define MAX77705_SLAVE_TREG_IM			(1 << 0)
#define MAX77705_CV_IM				(1 << 1)
#define MAX77705_SLAVE_FAULT_IM			(1 << 2)

/* MAX77705_CHG_REG_CHG_SWI_STATUS */
#define MAX77705_SLAVE_TREG_S			0x00
#define MAX77705_CV_S				0x01

/* MAX77705_CHG_REG_CHG_SWI_STATUS */
#define MAX77705_DIS_MIN_SELECTOR		0x80

/* MAX77705_CHG_REG_CHG_SLAVE_READBACK */
#define MAX77705_SWI_READBACK			0x3F

/* MAX77705_CHG_REG_CHG_SLAVE_CNTL */
#define MAX77705_BOVE				0x03

#define REDUCE_CURRENT_STEP						100
#define MINIMUM_INPUT_CURRENT					300
#define SLOW_CHARGING_CURRENT_STANDARD          400

#define WC_CURRENT_STEP		100
#define WC_CURRENT_START	480

struct max77705_charger_data {
	struct device           *dev;
	struct i2c_client       *i2c;
	struct i2c_client       *pmic_i2c;
	struct mutex            charger_mutex;

	struct max77705_platform_data *max77705_pdata;

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
	struct wake_lock wpc_wake_lock;
	struct wake_lock chgin_wake_lock;
	struct wake_lock wc_current_wake_lock;
	struct wake_lock aicl_wake_lock;
	struct wake_lock otg_wake_lock;

	unsigned int	is_charging;
	unsigned int	charging_type;
	unsigned int	battery_state;
	unsigned int	battery_present;
	unsigned int	cable_type;
	unsigned int	input_current;
	unsigned int	charging_current;
	unsigned int	vbus_state;
	bool	prev_aicl_mode;
	int		aicl_on;
	bool	slow_charging;
	int		status;
	int		charge_mode;
	u8		cnfg00_mode;
	int uvlo_attach_flag;
	int uvlo_attach_cable_type;
	int switching_freq;

	int		irq_bypass;
	int		irq_batp;
#if defined(CONFIG_MAX77705_CHECK_B2SOVRC)
	int		irq_bat;
#endif
	int		irq_battery;
	int		irq_chg;
	int		irq_wcin;
	int		irq_chgin;
	int		irq_aicl;
	int             irq_aicl_enabled;
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

	bool            jig_low_active;
	int		jig_gpio;

	bool enable_sysovlo_irq;
	bool enable_noise_wa;
	int irq_sysovlo;
	struct wake_lock sysovlo_wake_lock;

	bool is_mdock;
	bool otg_on;
	bool uno_on;

	int pmic_ver;
	int input_curr_limit_step;
	int wpc_input_curr_limit_step;
	int charging_curr_step;
	int float_voltage;

#if defined(CONFIG_BATTERY_SAMSUNG_MHS)
	int charging_port;
#endif

	sec_charger_platform_data_t *pdata;
};

#endif /* __MAX77705_CHARGER_H */
