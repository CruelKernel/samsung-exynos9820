/*
 * s2mu004-muic-hv.h - MUIC for the Samsung s2mu004
 *
 * Copyright (C) 2015 Samsung Electrnoics
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * This driver is based on max77843-muic.h
 *
 */

#ifndef __S2MU004_MUIC_HV_H__
#define __S2MU004_MUIC_HV_H__

#define MUIC_HV_DEV_NAME		"muic-s2mu004-hv"

enum s2mu004_reg_bit_control {
	S2MU004_DISABLE_BIT		= 0,
	S2MU004_ENABLE_BIT,
};

/* S2MU004 AFC INTMASK register */
#define INTMASK_VBADCM_SHIFT		0
#define INTMASK_VDNMONM_SHIFT		1
#define INTMASK_DNRESM_SHIFT		2
#define INTMASK_MPNACKM_SHIFT		3
#define INTMASK_MRXBUFOWM_SHIFT	4
#define INTMASK_MRXTRFM_SHIFT		5
#define INTMASK_MRXPERRM_SHIFT		6
#define INTMASK_MRXRDYM_SHIFT		7
#define INTMASK_VBADCM_MASK		(S2MU004_ENABLE_BIT << INTMASK_VBADCM_SHIFT)
#define INTMASK_VDNMONM_MASK		(S2MU004_ENABLE_BIT << INTMASK_VDNMONM_SHIFT)
#define INTMASK_DNRESM_MASK		(S2MU004_ENABLE_BIT << INTMASK_DNRESM_SHIFT)
#define INTMASK_MPNACKM_MASK		(S2MU004_ENABLE_BIT << INTMASK_MPNACKM_SHIFT)
#define INTMASK_MRXBUFOWM_MASK		(S2MU004_ENABLE_BIT << INTMASK_MRXBUFOWM_SHIFT)
#define INTMASK_MRXTRFM_MASK		(S2MU004_ENABLE_BIT << INTMASK_MRXTRFM_SHIFT)
#define INTMASK_MRXPERRM_MASK		(S2MU004_ENABLE_BIT << INTMASK_MRXPERRM_SHIFT)
#define INTMASK_MRXRDYM_MASK		(S2MU004_ENABLE_BIT << INTMASK_MRXRDYM_SHIFT)

/* S2MU004 AFC STATUS register */
#define STATUS_VBADC_SHIFT		0
#define STATUS_VDNMON_SHIFT		4
#define STATUS_DNRES_SHIFT		5
#define STATUS_MPNACK_SHIFT		6
#define STATUS_VBADC_MASK		(0xf << STATUS_VBADC_SHIFT)
#define STATUS_VDNMON_MASK		(0x1 << STATUS_VDNMON_SHIFT)
#define STATUS_DNRES_MASK		(0x1 << STATUS_DNRES_SHIFT)
#define STATUS_MPNACK_MASK		(0x1 << STATUS_MPNACK_SHIFT)

/* S2MU004 HVCONTROL1 register */
#define HVCONTROL1_DPDNVDEN_SHIFT	0
#define HVCONTROL1_DNVD_SHIFT		1
#define HVCONTROL1_DPVD_SHIFT		3
#define HVCONTROL1_VBUSADCEN_SHIFT	5
#define HVCONTROL1_CTRLIDMON_SHIFT	6
#define HVCONTROL1_AFCEN_SHIFT	7
#define HVCONTROL1_DPDNVDEN_MASK	(0x1 << HVCONTROL1_DPDNVDEN_SHIFT)
#define HVCONTROL1_DNVD_MASK		(0x3 << HVCONTROL1_DNVD_SHIFT)
#define HVCONTROL1_DPVD_MASK		(0x3 << HVCONTROL1_DPVD_SHIFT)
#define HVCONTROL1_VBUSADCEN_MASK	(0x1 << HVCONTROL1_VBUSADCEN_SHIFT)
#define HVCONTROL1_CTRLIDMON_MASK	(0x1 << HVCONTROL1_CTRLIDMON_SHIFT)
#define HVCONTROL1_AFCEN_MASK	(0x1 << HVCONTROL1_AFCEN_SHIFT)

/* S2MU004 HVCONTROL2 register */
//#define HVCONTROL2_HVDIGEN_SHIFT	0
#define HVCONTROL2_DP06EN_SHIFT		1
#define HVCONTROL2_DNRESEN_SHIFT	2
#define HVCONTROL2_MPING_SHIFT		3
#define HVCONTROL2_MTXEN_SHIFT		4
#define HVCONTROL2_RSTDM100UI_SHIFT		7
//#define HVCONTROL2_MTXBUSRES_SHIFT	5
//#define HVCONTROL2_MPNGENB_SHIFT	6
//#define HVCONTROL2_HVDIGEN_MASK		(0x1 << HVCONTROL2_HVDIGEN_SHIFT)
#define HVCONTROL2_DP06EN_MASK		(0x1 << HVCONTROL2_DP06EN_SHIFT)
#define HVCONTROL2_DNRESEN_MASK		(0x1 << HVCONTROL2_DNRESEN_SHIFT)
#define HVCONTROL2_MPING_MASK		(0x1 << HVCONTROL2_MPING_SHIFT)
#define HVCONTROL2_MTXEN_MASK		(0x1 << HVCONTROL2_MTXEN_SHIFT)
#define HVCONTROL2_RSTDM100UI_MASK		(0x1 << HVCONTROL2_RSTDM100UI_SHIFT)
//#define HVCONTROL2_MTXBUSRES_MASK	(0x1 << HVCONTROL2_MTXBUSRES_SHIFT)
//#define HVCONTROL2_MPNGENB_MASK		(0x1 << HVCONTROL2_MPNGENB_SHIFT)

#define HVTXBYTE_5V 0x0
#define HVTXBYTE_6V 0x1
#define HVTXBYTE_7V 0x2
#define HVTXBYTE_8V 0x3
#define HVTXBYTE_9V 0x4
#define HVTXBYTE_10V 0x5
#define HVTXBYTE_11V 0x6
#define HVTXBYTE_12V 0x7
#define HVTXBYTE_13V 0x8
#define HVTXBYTE_14V 0x9
#define HVTXBYTE_15V 0xA
#define HVTXBYTE_16V 0xB
#define HVTXBYTE_17V 0xC
#define HVTXBYTE_18V 0xD
#define HVTXBYTE_19V 0xE
#define HVTXBYTE_20V 0xF

#define HVTXBYTE_0_75A 0x0
#define HVTXBYTE_0_90A 0x1
#define HVTXBYTE_1_05A 0x2
#define HVTXBYTE_1_20A 0x3
#define HVTXBYTE_1_35A 0x4
#define HVTXBYTE_1_50A 0x5
#define HVTXBYTE_1_65A 0x6
#define HVTXBYTE_1_80A 0x7
#define HVTXBYTE_1_95A 0x8
#define HVTXBYTE_2_10A 0x9
#define HVTXBYTE_2_25A 0xA
#define HVTXBYTE_2_40A 0xB
#define HVTXBYTE_2_55A 0xC
#define HVTXBYTE_2_70A 0xD
#define HVTXBYTE_2_85A 0xE
#define HVTXBYTE_3_00A 0xF

/* S2MU004 HVRXBYTE register */
#define HVRXBYTE_MAX			16

/* S2MU004 AFC charger W/A Check NUM */
#define AFC_CHARGER_WA_PING		5

/* S2MU004 MPing miss SW Workaround - delay time */
#define MPING_MISS_WA_TIME		2000

#define MUIC_HV_5V	0x08
#define MUIC_HV_9V	0x46

#if 0
typedef enum {
	DPDNVDEN_DISABLE	= 0x00,
	DPDNVDEN_ENABLE		= (0x1 << HVCONTROL1_DPDNVDEN_SHIFT),

	DPDNVDEN_DONTCARE	= 0xff,
} dpdnvden_t;
#endif

typedef enum {
	VDNMON_LOW		= 0x00,
	VDNMON_HIGH		= (0x1 << STATUS_VDNMON_SHIFT),

	VDNMON_DONTCARE		= 0xff,
} vdnmon_t;

typedef enum {
//	VBADC_VBDET		= 0x00,
	VBADC_5_3V		= 0x00,
	VBADC_5_7V_6_3V		= (0x1 << STATUS_VBADC_SHIFT),
	VBADC_6_7V_7_3V		= (0x2 << STATUS_VBADC_SHIFT),
	VBADC_7_7V_8_3V		= (0x3 << STATUS_VBADC_SHIFT),
	VBADC_8_7V_9_3V		= (0x4 << STATUS_VBADC_SHIFT),
	VBADC_9_7V_10_3V		= (0x5 << STATUS_VBADC_SHIFT),
	VBADC_10_7V_11_3V		= (0x6 << STATUS_VBADC_SHIFT),
	VBADC_11_7V_12_3V		= (0x7 << STATUS_VBADC_SHIFT),
	VBADC_12_7V_13_3V		= (0x8 << STATUS_VBADC_SHIFT),
	VBADC_13_7V_14_3V		= (0x9 << STATUS_VBADC_SHIFT),
	VBADC_14_7V_15_3V		= (0xA << STATUS_VBADC_SHIFT),
	VBADC_15_7V_16_3V		= (0xB << STATUS_VBADC_SHIFT),
	VBADC_16_7V_17_3V		= (0xC << STATUS_VBADC_SHIFT),
	VBADC_17_7V_18_3V		= (0xD << STATUS_VBADC_SHIFT),
	VBADC_18_7V_19_3V		= (0xE << STATUS_VBADC_SHIFT),
	VBADC_19_7V		= (0xF << STATUS_VBADC_SHIFT),

//	VBADC_QC_5V		= 0xeb,
//	VBADC_QC_9V		= 0xec,
//	VBADC_QC_12V		= 0xed,
//	VBADC_QC_20V		= 0xee,

	VBADC_AFC_5V		= 0xfa,
	VBADC_AFC_9V		= 0xfb,
	VBADC_QC_9V		= 0xfb,
	VBADC_AFC_ERR_V		= 0xfc,
	VBADC_AFC_ERR_V_NOT_0	= 0xfd,

	VBADC_ANY		= 0xfe,
	VBADC_DONTCARE		= 0xff,
} vbadc_t;

#if 0
enum {
	HV_SUPPORT_QC_5V	= 5,
	HV_SUPPORT_QC_9V	= 9,
	HV_SUPPORT_QC_12V	= 12,
	HV_SUPPORT_QC_20V	= 20,
};
#endif

/* MUIC afc irq type */
typedef enum {
	MUIC_AFC_IRQ_VDNMON = 0,
	MUIC_AFC_IRQ_MRXRDY,
	MUIC_AFC_IRQ_VBADC,
	MUIC_AFC_IRQ_MPNACK,
	MUIC_AFC_IRQ_DONTCARE = 0xff,
} muic_afc_irq_t;

/* muic chip specific internal data structure */
typedef struct s2mu004_muic_afc_data {
	muic_attached_dev_t		new_dev;
	const char			*afc_name;
	muic_afc_irq_t			afc_irq;
//	u8				hvcontrol1_dpdnvden;
	u8				status_vbadc;
	u8				status_vdnmon;
	int				function_num;
	struct s2mu004_muic_afc_data	*next;
} muic_afc_data_t;

enum s2mu004_reg_hv_val {
#if 0
	S2MU004_MUIC_HVCONTROL1_DPVD_06	= (0x2 << HVCONTROL1_DPVD_SHIFT),
	S2MU004_MUIC_HVCONTROL1_11	= (S2MU004_MUIC_HVCONTROL1_DPVD_06 |
					HVCONTROL1_DPDNVDEN_MASK),
	S2MU004_MUIC_HVCONTROL1_31	= (HVCONTROL1_VBUSADCEN_MASK |
					S2MU004_MUIC_HVCONTROL1_DPVD_06 |
					HVCONTROL1_DPDNVDEN_MASK),
	S2MU004_MUIC_HVCONTROL2_06	= (HVCONTROL2_DP06EN_MASK | HVCONTROL2_DNRESEN_MASK),
	S2MU004_MUIC_HVCONTROL2_13	= (HVCONTROL2_MTXEN_MASK | HVCONTROL2_DP06EN_MASK |
					HVCONTROL2_HVDIGEN_MASK),
	S2MU004_MUIC_HVCONTROL2_1B	= (HVCONTROL2_HVDIGEN_MASK | HVCONTROL2_DP06EN_MASK |
					HVCONTROL2_MPING_MASK | HVCONTROL2_MTXEN_MASK),
	S2MU004_MUIC_HVCONTROL2_1F	= (HVCONTROL2_HVDIGEN_MASK | HVCONTROL2_DP06EN_MASK |
					HVCONTROL2_DNRESEN_MASK | HVCONTROL2_MPING_MASK | HVCONTROL2_MTXEN_MASK),
	S2MU004_MUIC_HVCONTROL2_5B	= (HVCONTROL2_HVDIGEN_MASK | HVCONTROL2_DP06EN_MASK |
					HVCONTROL2_MPING_MASK | HVCONTROL2_MTXEN_MASK | HVCONTROL2_MPNGENB_MASK),
	S2MU004_MUIC_INTMASK3_FB	= (INTMASK3_MRXRDYM_MASK | INTMASK3_MRXPERRM_MASK |
					INTMASK3_MRXTRFM_MASK | INTMASK3_MRXBUFOWM_MASK |
					INTMASK3_MPNACKM_MASK | INTMASK3_VDNMONM_MASK |
					INTMASK3_VBADCM_MASK),
#endif
	S2MU004_MUIC_HVCONTROL1_A0	= (HVCONTROL1_AFCEN_MASK |
					HVCONTROL1_VBUSADCEN_MASK),

	S2MU004_MUIC_HVCONTROL2_06	= (HVCONTROL2_DNRESEN_MASK |
					HVCONTROL2_DP06EN_MASK),

	S2MU004_MUIC_HVCONTROL1_E0	= (HVCONTROL1_AFCEN_MASK | HVCONTROL1_CTRLIDMON_MASK |
					HVCONTROL1_VBUSADCEN_MASK),

	S2MU004_MUIC_HVCONTROL2_0E	= (HVCONTROL2_MPING_MASK | HVCONTROL2_DNRESEN_MASK |
					HVCONTROL2_DP06EN_MASK),
};

struct s2mu004_muic_data;
extern int s2mu004_muic_reset_afc_register(struct s2mu004_muic_data *muic_data);
extern bool muic_check_dev_ta(struct s2mu004_muic_data *muic_data);
extern bool muic_check_is_hv_dev(struct s2mu004_muic_data *muic_data);
extern muic_attached_dev_t hv_muic_check_id_err
	(struct s2mu004_muic_data *muic_data, muic_attached_dev_t new_dev);
extern void s2mu004_hv_muic_reset_hvcontrol_reg(struct s2mu004_muic_data *muic_data);
#if defined(CONFIG_OF)
extern int of_s2mu004_hv_muic_dt(struct s2mu004_muic_data *muic_data);
#endif

extern int s2mu004_afc_muic_irq_init(struct s2mu004_muic_data *muic_data);
extern void s2mu004_hv_muic_free_irqs(struct s2mu004_muic_data *muic_data);

extern int s2mu004_muic_hv_update_reg(struct i2c_client *i2c,
		const u8 reg, const u8 val, const u8 mask, const bool debug_en);
extern void s2mu004_muic_set_afc_ready(struct s2mu004_muic_data *muic_data, bool value);

extern void s2mu004_hv_muic_init_check_dpdnvden(struct s2mu004_muic_data *muic_data);
extern void s2mu004_hv_muic_init_detect(struct s2mu004_muic_data *muic_data);
extern void s2mu004_hv_muic_initialize(struct s2mu004_muic_data *muic_data);
extern void s2mu004_hv_muic_remove(struct s2mu004_muic_data *muic_data);
//extern void s2mu004_hv_muic_remove_wo_free_irq(struct s2mu004_muic_data *muic_data);
#if 0
extern void s2mu004_muic_set_adcmode_always(struct s2mu004_muic_data *muic_data);
#if !defined(CONFIG_SEC_FACTORY)
extern void s2mu004_muic_set_adcmode_oneshot(struct s2mu004_muic_data *muic_data);
#endif /* !CONFIG_SEC_FACTORY */
extern void s2mu004_hv_muic_adcmode_oneshot(struct s2mu004_muic_data *muic_data);
#endif
extern void s2mu004_muic_prepare_afc_charger(struct s2mu004_muic_data *muic_data);
extern bool s2mu004_muic_check_change_dev_afc_charger
	(struct s2mu004_muic_data *muic_data, muic_attached_dev_t new_dev);

#endif /* __S2MU004_MUIC_HV_H__ */

