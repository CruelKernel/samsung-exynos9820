/*
 * max77865-muic.h - MUIC for the Maxim 77865
 *
 * Copyright (C) 2016 Samsung Electrnoics
 * Insun Choi <insun77.choi@samsung.com>
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
 * This driver is based on max14577-muic.h
 *
 */

#ifndef __MAX77865_MUIC_H__
#define __MAX77865_MUIC_H__

#define MUIC_DEV_NAME			"muic-max77865"

/* muic chip specific internal data structure */
struct max77865_muic_data {
	struct device			*dev;
	struct i2c_client		*i2c; /* i2c addr: 0x4A; MUIC */
	struct mutex			muic_mutex;
	struct mutex			reset_mutex;

	/* model dependant mfd platform data */
	struct max77865_platform_data	*mfd_pdata;

	/* model dependant muic platform data */
	struct muic_platform_data	*pdata;

	int				irq_uid;
	int				irq_uidgnd;
	int				irq_chgtyp;
	int				irq_prchgtyp;
	int				irq_vbvolt;
	int				irq_vdnmon;
	int				irq_mrxrdy;
	int				irq_mpnack;
	int				irq_vbadc;

	/* muic current attached device */
	muic_attached_dev_t		attached_dev;

	/* muic support vps list */
	bool muic_support_list[ATTACHED_DEV_NUM];

	bool				is_muic_ready;
	bool				is_muic_reset;

	/* check is otg test for jig uart off + vb */
	bool				is_otg_test;

	bool				is_factory_start;

	bool				is_afc_muic_ready;
	bool				is_afc_handshaking;
	bool				is_afc_muic_prepare;
	bool				is_charger_ready;
	bool				is_qc_vb_settle;

	u8				is_boot_dpdnvden;
	u8				tx_data;
	bool				is_mrxrdy;
	int				afc_count;
	muic_afc_data_t			afc_data;
	u8				qc_hv;
	struct delayed_work		hv_muic_qc_vb_work;
	struct delayed_work		hv_muic_mping_miss_wa;

	/* muic status value */
	u8				bcstatus1;
        u8				bcstatus2;
        u8				gpstatus3;

	/* muic hvcontrol value */
	u8				hvcontrol1;
	u8				hvcontrol2;

	/* legacy TA or USB for CCIC */
	muic_attached_dev_t		legacy_dev;

};

/* max77865 muic register read/write related information defines. */

/* MAX77865 REGISTER ENABLE or DISABLE bit */
enum max77865_reg_bit_control {
	MAX77865_DISABLE_BIT		= 0,
	MAX77865_ENABLE_BIT,
};

typedef enum {
	PROCESS_ATTACH			= 0,
	PROCESS_LOGICALLY_DETACH,
	PROCESS_NONE,
} process_t;

/* CC detection disable for use Only CL65(without MQ81) */
#define CCDET_DISABLE			(0x0)

/* MAX77865 BC STATUS1 register */
#define BC_STATUS1_VBVOLT_SHIFT		7
#define BC_STATUS1_CHGDETRUN_SHIFT	6
#define BC_STATUS1_PRCHGTYP_SHIFT	3
#define BC_STATUS1_DCDTMR_SHIFT		2
#define BC_STATUS1_CHGTYP_SHIFT		0
#define BC_STATUS1_VBVOLT_MASK		(0x1 << BC_STATUS1_VBVOLT_SHIFT)
#define BC_STATUS1_CHGDETRUN_MASK	(0x1 << BC_STATUS1_CHGDETRUN_SHIFT)
#define BC_STATUS1_PRCHGTYP_MASK	(0x7 << BC_STATUS1_PRCHGTYP_SHIFT)
#define BC_STATUS1_DCDTMR_MASK		(0x1 << BC_STATUS1_DCDTMR_SHIFT)
#define BC_STATUS1_CHGTYP_MASK		(0x3 << BC_STATUS1_CHGTYP_SHIFT)

typedef enum {
	VB_LOW		= 0x00,
	VB_HIGH		= 0x01,

	VB_DONTCARE	= 0xff,
} vbvolt_t;

typedef enum {
	CHGDETRUN_FALSE		= 0x00,
	CHGDETRUN_TRUE		= (0x1 << BC_STATUS1_CHGDETRUN_SHIFT),

	CHGDETRUN_DONTCARE	= 0xff,
} chgdetrun_t;

/* MAX77865 BC STATUS2 register
 * [7:2] RSVD
 */
#define BC_STATUS2_DXOVP_SHIFT		1
#define BC_STATUS2_DNVDATREF_SHIFT	0
#define BC_STATUS2_DXOVP_MASK		(0x1 << BC_STATUS2_DXOVP_SHIFT)
#define BC_STATUS2_DNVDATREF_MASK	(0x1 << BC_STATUS2_DNVDATREF_SHIFT)

/* MAX77865 GP STATUS register */
#define GP_STATUS_UID_SHIFT		6
#define GP_STATUS_UIDGND_SHIFT		5
#define GP_STATUS_TSHUT_SHIFT		4
#define GP_STATUS_VBADC_SHIFT		0
#define GP_STATUS_UID_MASK		(0x3 << GP_STATUS_UID_SHIFT)
#define GP_STATUS_UIDGND_MASK		(0x1 << GP_STATUS_UIDGND_SHIFT)
#define GP_STATUS_TSHUT_MASK		(0x1 << GP_STATUS_TSHUT_SHIFT)
#define GP_STATUS_VBADC_MASK		(0xF << GP_STATUS_VBADC_SHIFT)

/* UID table */
typedef enum {
	UID_JIG_USB_ON		= 0x00, /* 301Kohm */
	UID_JIG_UART_OFF	= 0x40, /* 523Kohm */
	UID_JIG_RSVD		= 0x80,
	UID_OPEN		= 0xC0,

	UID_DONTCARE		= 0xFF,
} muic_uid_t;

/* MAX77865 BC CONTROL1 register */
#define BC_CONTROL1_DCDCPL_SHIFT	7
#define BC_CONTROL1_UIDEN_SHIFT		6
#define BC_CONTROL1_NOAUTOIBUS_SHIFT	5
#define BC_CONTROL1_3ADCPDET_SHIFT	4
#define BC_CONTROL1_SFOUTCTRL_SHIFT	2
#define BC_CONTROL1_CHGDETMAN_SHIFT	1
#define BC_CONTROL1_CHGDETEN_SHIFT	0
#define BC_CONTROL1_DCDCPL_MASK		(0x1 << BC_CONTROL1_DCDCPL_SHIFT)
#define BC_CONTROL1_UIDEN_MASK		(0x1 << BC_CONTROL1_UIDEN_SHIFT)
#define BC_CONTROL1_NOAUTOIBUS_MASK	(0x1 << BC_CONTROL1_NOAUTOIBUS_SHIFT)
#define BC_CONTROL1_3ADCPDET_MASK	(0x1 << BC_CONTROL1_3ADCPDET_SHIFT)
#define BC_CONTROL1_SFOUTCTRL_MASK	(0x3 << BC_CONTROL1_SFOUTCTRL_SHIFT)
#define BC_CONTROL1_CHGDETMAN_MASK	(0x1 << BC_CONTROL1_CHGDETMAN_SHIFT)
#define BC_CONTROL1_CHGDETEN_MASK	(0x1 << BC_CONTROL1_CHGDETEN_SHIFT)
#define BC_CONTROL1_RESET		(0xE5)

/* MAX77865 BC CONTROL2 register
 * [7:6] RSVD
 */
#define BC_CONTROL2_DNMONEN_SHIFT	5
#define BC_CONTROL2_DPDNMAN_SHIFT	4
#define BC_CONTROL2_DPDRV_SHIFT		2
#define BC_CONTROL2_DNDRV_SHIFT		0
#define BC_CONTROL2_DNMONEN_MASK	(0x1 << BC_CONTROL2_DNMONEN_SHIFT)
#define BC_CONTROL2_DPDNMAN_MASK	(0x1 << BC_CONTROL2_DPDNMAN_SHIFT)
#define BC_CONTROL2_DPDRV_MASK		(0x3 << BC_CONTROL2_DPDRV_SHIFT)
#define BC_CONTROL2_DNDRV_MASK		(0x3 << BC_CONTROL2_DNDRV_SHIFT)

/* MAX77865 CONTROL1 register */
#define CONTROL1_NOBCCOMP_SHIFT		7
#define CONTROL1_RCPS_SHIFT		6
#define CONTROL1_COMP2SW_SHIFT		3
#define CONTROL1_COMN1SW_SHIFT		0
#define CONTROL1_NOBCCOMP_MASK		(0x1 << CONTROL1_NOBCCOMP_SHIFT)
#define CONTROL1_RCPS_MASK		(0x1 << CONTROL1_RCPS_SHIFT)
#define CONTROL1_COM_OPEN		(0x0)
#define CONTROL1_COM_USB		(0x1)
#define CONTROL1_COM_AUDIO		(0x2)
#define CONTROL1_COM_UART		(0x3)

/* muic register value for COMN1, COMN2 in CTRL1 reg  */

/*
 * MAX77865 CONTROL1 register
 * NoBCComp [7] / RCPS [6] / D+ [5:3] / D- [2:0]
 * 000: Open / 001: USB / 010: Audio / 011: UART
 */
enum max77865_reg_ctrl1_val {
	MAX77865_MUIC_CTRL1_NO_BC_COMP_OFF	= 0x0,
	MAX77865_MUIC_CTRL1_NO_BC_COMP_ON	= 0x1,
	MAX77865_MUIC_CTRL1_COM_OPEN		= 0x00,
	MAX77865_MUIC_CTRL1_COM_USB		= 0x01,
	MAX77865_MUIC_CTRL1_COM_AUDIO		= 0x02,
	MAX77865_MUIC_CTRL1_COM_UART		= 0x03,
	MAX77865_MUIC_CTRL1_COM_USB_CP		= 0x04,
	MAX77865_MUIC_CTRL1_COM_UART_CP		= 0x05,
};

typedef enum {
	CTRL1_OPEN	= (MAX77865_MUIC_CTRL1_NO_BC_COMP_OFF << CONTROL1_NOBCCOMP_SHIFT) | \
			(MAX77865_MUIC_CTRL1_COM_OPEN << CONTROL1_COMP2SW_SHIFT) | \
			(MAX77865_MUIC_CTRL1_COM_OPEN << CONTROL1_COMN1SW_SHIFT),
	CTRL1_USB	= (MAX77865_MUIC_CTRL1_NO_BC_COMP_OFF << CONTROL1_NOBCCOMP_SHIFT) | \
			(MAX77865_MUIC_CTRL1_COM_USB << CONTROL1_COMP2SW_SHIFT) | \
			(MAX77865_MUIC_CTRL1_COM_USB << CONTROL1_COMN1SW_SHIFT),
	CTRL1_AUDIO	= (MAX77865_MUIC_CTRL1_NO_BC_COMP_OFF << CONTROL1_NOBCCOMP_SHIFT) | \
			(MAX77865_MUIC_CTRL1_COM_AUDIO << CONTROL1_COMP2SW_SHIFT) | \
			(MAX77865_MUIC_CTRL1_COM_AUDIO << CONTROL1_COMN1SW_SHIFT),
	CTRL1_UART	= (MAX77865_MUIC_CTRL1_NO_BC_COMP_OFF << CONTROL1_NOBCCOMP_SHIFT) | \
			(MAX77865_MUIC_CTRL1_COM_UART << CONTROL1_COMP2SW_SHIFT) | \
			(MAX77865_MUIC_CTRL1_COM_UART << CONTROL1_COMN1SW_SHIFT),
	CTRL1_USB_CP	= (MAX77865_MUIC_CTRL1_NO_BC_COMP_OFF << CONTROL1_NOBCCOMP_SHIFT) | \
			(MAX77865_MUIC_CTRL1_COM_USB_CP << CONTROL1_COMP2SW_SHIFT) | \
			(MAX77865_MUIC_CTRL1_COM_USB_CP << CONTROL1_COMN1SW_SHIFT),
	CTRL1_UART_CP	= (MAX77865_MUIC_CTRL1_NO_BC_COMP_OFF << CONTROL1_NOBCCOMP_SHIFT) | \
			(MAX77865_MUIC_CTRL1_COM_UART_CP << CONTROL1_COMP2SW_SHIFT) | \
			(MAX77865_MUIC_CTRL1_COM_UART_CP << CONTROL1_COMN1SW_SHIFT),
} max77865_reg_ctrl1_t;

/* MAX77865 CONTROL2 register
 * [7:3], [0] RSVD
 */
#define CONTROL2_CPEN_SHIFT		2
#define CONTROL2_CHGENCTRL_SHIFT	1
#define CONTROL2_CPEN_MASK		(0x1 << CONTROL2_CPEN_SHIFT)
#define CONTROL2_CHGENCTRL_MASK		(0x1 << CONTROL2_CHGENCTRL_SHIFT)

/* MAX77865 CONTROL3 register
 * [7:6] RSVD
 */
#define CONTROL3_RES3SET_SHIFT		5
#define CONTROL3_RES2SET_SHIFT		3
#define CONTROL3_RES1SET_SHIFT		1
#define CONTROL3_JIGSET_SHIFT		0
#define CONTROL3_RES3SET_MASK		(0x1 << CONTROL3_RES3SET_SHIFT)
#define CONTROL3_RES2SET_MASK		(0x3 << CONTROL3_RES2SET_SHIFT)
#define CONTROL3_RES1SET_MASK		(0x3 << CONTROL3_RES1SET_SHIFT)
#define CONTROL3_JIGSET_MASK		(0x1 << CONTROL3_JIGSET_SHIFT)

/* MAX77865 CONTROL4 register
 * [7:6], [3:0] RSVD
 */
#define CONTROL4_FCTAUTO_SHIFT		5
#define CONTROL4_USBAUTO_SHIFT		4
#define CONTROL4_FCTAUTO_MASK		(0x1 << CONTROL4_FCTAUTO_SHIFT)
#define CONTROL4_USBAUTO_MASK		(0x1 << CONTROL4_USBAUTO_SHIFT)

/* MAX77865 MUIC Output of USB Charger Detection */
typedef enum {
	/* No Valid voltage at VB (Vvb < Vvbdet) */
	CHGTYP_NO_VOLTAGE		= 0x00,
	/* Unknown (D+/D- does not present a valid USB charger signature) */
	CHGTYP_USB			= 0x01,
	/* Charging Downstream Port */
	CHGTYP_CDP			= 0x02,
	/* Dedicated Charger (D+/D- shorted) */
	CHGTYP_DEDICATED_CHARGER	= 0x03,

	/* Any charger w/o USB */
	CHGTYP_UNOFFICIAL_CHARGER	= 0xfc,
	/* Any charger type */
	CHGTYP_ANY			= 0xfd,
	/* Don't care charger type */
	CHGTYP_DONTCARE			= 0xfe,

	CHGTYP_MAX,
	CHGTYP_INIT,
	CHGTYP_MIN = CHGTYP_NO_VOLTAGE
} chgtyp_t;

/* MAX77833 MUIC Special Charger Type Detection Output value */
typedef enum {
	PRCHGTYP_UNKNOWN		= 0x0,
	PRCHGTYP_SAMSUNG_2A		= 0x1,
	PRCHGTYP_APPLE_500MA		= 0x2,
	PRCHGTYP_APPLE_1A		= 0x3,
	PRCHGTYP_APPLE_2A		= 0x4,
	PRCHGTYP_APPLE_12W		= 0x5,
	PRCHGTYP_3A_DCP			= 0x6,
	PRCHGTYP_RFU			= 0x7,
} prchgtyp_t;

extern struct device *switch_device;

#endif /* __MAX77865_MUIC_H__ */

