/*
 * s2mps19.h - Driver for the s2mps19
 *
 *  Copyright (C) 201666666 Samsung Electrnoics
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
 *
 */

#ifndef __S2MPS19_MFD_H__
#define __S2MPS19_MFD_H__
#include <linux/platform_device.h>
#include <linux/regmap.h>

#define MFD_DEV_NAME "s2mps19"
/**
 * sec_regulator_data - regulator data
 * @id: regulator id
 * @initdata: regulator init data (contraints, supplies, ...)
 */
struct s2mps19_regulator_data {
	int id;
	struct regulator_init_data *initdata;
	struct device_node *reg_node;
};

/*
 * sec_opmode_data - regulator operation mode data
 * @id: regulator id
 * @mode: regulator operation mode
 */
struct sec_opmode_data {
	int id;
	unsigned int mode;
};

/*
 * samsung regulator operation mode
 * SEC_OPMODE_OFF	Regulator always OFF
 * SEC_OPMODE_ON	Regulator always ON
 * SEC_OPMODE_LOWPOWER  Regulator is on in low-power mode
 * SEC_OPMODE_SUSPEND   Regulator is changed by PWREN pin
 *			If PWREN is high, regulator is on
 *			If PWREN is low, regulator is off
 */
enum sec_opmode {
	SEC_OPMODE_OFF,
	SEC_OPMODE_SUSPEND,
	SEC_OPMODE_LOWPOWER,
	SEC_OPMODE_ON,
	SEC_OPMODE_TCXO = 0x2,
	SEC_OPMODE_MIF = 0x2,
};

/**
 * struct sec_wtsr_smpl - settings for WTSR/SMPL
 * @wtsr_en:		WTSR Function Enable Control
 * @smpl_en:		SMPL Function Enable Control
 * @wtsr_timer_val:	Set the WTSR timer Threshold
 * @smpl_timer_val:	Set the SMPL timer Threshold
 * @check_jigon:	if this value is true, do not enable SMPL function when
 *			JIGONB is low(JIG cable is attached)
 */
struct sec_wtsr_smpl {
	bool wtsr_en;
	bool smpl_en;
	bool sub_smpl_en;
	int wtsr_timer_val;
	int smpl_timer_val;
	bool check_jigon;
};

struct s2mps19_platform_data {
	/* IRQ */
	int irq_base;
	int irq_gpio;
	bool wakeup;

	int num_regulators;
	struct	s2mps19_regulator_data *regulators;
	struct	sec_opmode_data		*opmode;
	struct	mfd_cell *sub_devices;
	int 	num_subdevs;

	int	(*cfg_pmic_irq)(void);
	int	device_type;
	int	ono;
	int	buck_ramp_delay;

	int				smpl_warn;
	bool				smpl_warn_en;
	bool				smpl_warn_dev2;
#ifdef CONFIG_SEC_PM
	bool				smpl_warn_en_by_evt;
#endif
	unsigned int                    smpl_warn_vth;
	unsigned int                    smpl_warn_hys;
	bool			ocp_warn1_en;
	bool			ocp_warn2_en;
	int				ocp_warn1_cnt;
	int				ocp_warn2_cnt;
	bool			ocp_warn1_dvs_mask;
	bool			ocp_warn2_dvs_mask;
	int				ocp_warn1_lv;
	int				ocp_warn2_lv;
	bool			support_vdd_pll_1p7;
	/* adc_mode
	 * 0 : not use
	 * 1 : current meter
	 * 2 : power meter
	*/
	int		adc_mode;
	/* 1 : sync mode, 2 : async mode  */
	int		adc_sync_mode;

	/* ---- RTC ---- */
	struct sec_wtsr_smpl *wtsr_smpl;
/*	struct sec_rtc_time *init_time; */
	struct rtc_time *init_time;
	int	osc_bias_up;
	int	cap_sel;
	int	osc_xin;
	int	osc_xout;

	bool	use_i2c_speedy;
};

struct s2mps19 {
	struct regmap *regmap;
};

#endif /* __S2MPS19_MFD_H__ */

