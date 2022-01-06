/*
 * max77705-muic.h - MUIC for the Maxim 77705
 *
 * Copyright (C) 2015 Samsung Electrnoics
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * This driver is based on max14577-muic.h
 *
 */

#ifndef __MAX77705_MUIC_H__
#define __MAX77705_MUIC_H__

#include <linux/workqueue.h>
#include <linux/wakelock.h>

#define MUIC_DEV_NAME			"muic-max77705"

#define MUIC_IRQ_INIT_DETECT		(-1)
#define MUIC_IRQ_CCIC_HANDLER		(-2)
#define MUIC_IRQ_VBUS_WA		(-3)

enum max77705_adc {
	MAX77705_UIADC_GND		= 0x00,
	MAX77705_UIADC_255K		= 0x03,
	MAX77705_UIADC_301K		= 0x04,
	MAX77705_UIADC_523K		= 0x05,
	MAX77705_UIADC_619K		= 0x06,
	MAX77705_UIADC_OPEN		= 0x07,

	MAX77705_UIADC_DONTCARE		= 0xfe, /* ADC don't care for MHL */
	MAX77705_UIADC_ERROR		= 0xff, /* ADC value read error */
};

enum max77705_vbadc {
	MAX77705_VBADC_3_8V_UNDER	= 0x0,
	MAX77705_VBADC_3_8V_TO_4_5V	= 0x1,
	MAX77705_VBADC_4_5V_TO_5_5V	= 0x2,
	MAX77705_VBADC_5_5V_TO_6_5V	= 0x3,
	MAX77705_VBADC_6_5V_TO_7_5V	= 0x4,
	MAX77705_VBADC_7_5V_TO_8_5V	= 0x5,
	MAX77705_VBADC_8_5V_TO_9_5V	= 0x6,
	MAX77705_VBADC_9_5V_TO_10_5V	= 0x7,
	MAX77705_VBADC_10_5V_TO_11_5V	= 0x8,
	MAX77705_VBADC_11_5V_TO_12_5V	= 0x9,
	MAX77705_VBADC_12_5V_OVER	= 0xa,
};

enum max77705_muic_command_opcode {
	COMMAND_BC_CTRL1_READ		= 0x01,
	COMMAND_BC_CTRL1_WRITE		= 0x02,
	COMMAND_BC_CTRL2_READ		= 0x03,
	COMMAND_BC_CTRL2_WRITE		= 0x04,
	COMMAND_CONTROL1_READ		= 0x05,
	COMMAND_CONTROL1_WRITE		= 0x06,
	COMMAND_CONTROL2_READ		= 0x07,
	COMMAND_CONTROL2_WRITE		= 0x08,
	/* AFC */
	COMMAND_HV_CONTROL_READ		= 0x11,
	COMMAND_HV_CONTROL_WRITE	= 0x12,
	COMMAND_AFC_HV_WRITE		= 0x20,
	COMMAND_AFC_RESULT_READ		= 0x21,
	COMMAND_QC_2_0_SET		= 0x22,

	/* not cmd opcode, for notifier */
	NOTI_ATTACH			= 0xfa,
	NOTI_DETACH			= 0xfb,
	NOTI_LOGICALLY_ATTACH		= 0xfc,
	NOTI_LOGICALLY_DETACH		= 0xfd,

	COMMAND_NONE			= 0xff,
};

#define AFC_OP_OUT_LEN 11 /* OPCODE(1) + Result(1) + VBADC(1) + RX Data(8) */

#if defined(CONFIG_HICCUP_CHARGER)
enum MUIC_HICCUP_MODE {
	MUIC_HICCUP_MODE_OFF	=	0,
	MUIC_HICCUP_MODE_NOTY,
	MUIC_HICCUP_MODE_ON,
};
#endif

#if defined(CONFIG_MUIC_MAX77705_CCIC)
#define MUIC_CCIC_NOTI_ATTACH (1)
#define MUIC_CCIC_NOTI_DETACH (-1)
#define MUIC_CCIC_NOTI_UNDEFINED (0)

struct max77705_muic_ccic_evt {
	int ccic_evt_attached; /* 1: attached, -1: detached, 0: undefined */
	int ccic_evt_rid; /* the last rid */
	int ccic_evt_rprd; /*rprd */
	int ccic_evt_roleswap; /* check rprd role swap event */
	int ccic_evt_dcdcnt; /* count dcd timeout */
};
#endif /* CONFIG_MUIC_MAX77705_CCIC */

/* muic chip specific internal data structure */
struct max77705_muic_data {
	struct device			*dev;
	struct i2c_client		*i2c; /* i2c addr: 0x4A; MUIC */
	struct mutex			muic_mutex;
	struct wake_lock		muic_wake_lock;

	/* model dependent mfd platform data */
	struct max77705_platform_data		*mfd_pdata;
	struct max77705_usbc_platform_data	*usbc_pdata;

	int				irq_uiadc;
	int				irq_chgtyp;
	int				irq_fakvb;
	int				irq_spr;
	int				irq_dcdtmo;
	int				irq_vbadc;
	int				irq_vbusdet;

	/* model dependent muic platform data */
	struct muic_platform_data	*pdata;

	/* muic current attached device */
	muic_attached_dev_t		attached_dev;
	void				*attached_func;

	/* muic support vps list */
	bool				muic_support_list[ATTACHED_DEV_NUM];

	bool				is_muic_ready;
	bool				is_muic_reset;

	u8				adcmode;
	u8				switch_val;

	/* check is otg test */
	bool				is_otg_test;

	/* muic HV charger */
	bool				is_factory_start;
	bool				is_check_hv;
	bool				is_charger_ready;
	bool				is_afc_reset;
	bool				is_skip_bigdata;
	bool				is_charger_mode;

	u8				is_boot_dpdnvden;

	struct delayed_work		afc_work;
	struct work_struct		afc_handle_work;
	unsigned char			afc_op_dataout[AFC_OP_OUT_LEN];
	int				hv_voltage;
	int				afc_retry;
	int				dcdtmo_retry;
	int				bc1p2_retry_count;

 	/* hiccup mode flag */
 	int				is_hiccup_mode;

	/* muic status value */
	u8				status1;
	u8				status2;
	u8				status3;
	u8                              status4;
	u8                              status5;
	u8                              status6;

	struct delayed_work		debug_work;
	struct delayed_work		vbus_wa_work;

	/* Fake vbus flag */
	int				fake_chgtyp;

#if defined(CONFIG_USB_EXTERNAL_NOTIFY)
	/* USB Notifier */
	struct notifier_block		usb_nb;
#endif
#if defined(CONFIG_MUIC_MAX77705_CCIC)
	struct max77705_muic_ccic_evt	ccic_info_data;
	struct work_struct		ccic_info_data_work;
	bool				afc_water_disable;
	int				ccic_evt_id;
	/* CCIC Notifier */
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	struct notifier_block		manager_nb;
#else
	struct notifier_block		ccic_nb;
#endif
#endif /* CONFIG_MUIC_MAX77705_CCIC */
};

/* max77705 muic register read/write related information defines. */
#define REG_NONE			0xff
#define REG_FULL_MASKING		0xff

#define INTMASK3_RESET			0x40
#define INTMASK3_INIT			0xc3

/* MAX77705 REGISTER ENABLE or DISABLE bit */
enum max77705_reg_bit_control {
	MAX77705_DISABLE_BIT		= 0,
	MAX77705_ENABLE_BIT,
};

/* MAX77705 STATUS1 register */
#define USBC_STATUS1_UIADC_SHIFT	0
#define USBC_STATUS1_FAKVB_SHIFT	3
#define USBC_STATUS1_VBADC_SHIFT	4
#define USBC_STATUS1_UIADC_MASK		(0x7 << USBC_STATUS1_UIADC_SHIFT)
#define USBC_STATUS1_FAKVB_MASK		(0x1 << USBC_STATUS1_FAKVB_SHIFT)
#define USBC_STATUS1_VBADC_MASK		(0xf << USBC_STATUS1_VBADC_SHIFT)

/* MAX77705 BC STATUS register */
#define BC_STATUS_CHGTYP_SHIFT		0
#define BC_STATUS_DCDTMO_SHIFT		2
#define BC_STATUS_PRCHGTYP_SHIFT	3
#define BC_STATUS_VBUSDET_SHIFT		7
#define BC_STATUS_CHGTYP_MASK		(0x3 << BC_STATUS_CHGTYP_SHIFT)
#define BC_STATUS_DCDTMO_MASK		(0x1 << BC_STATUS_DCDTMO_SHIFT)
#define BC_STATUS_PRCHGTYP_MASK		(0x7 << BC_STATUS_PRCHGTYP_SHIFT)
#define BC_STATUS_VBUSDET_MASK		(0x1 << BC_STATUS_VBUSDET_SHIFT)

/* MAX77705 USBC_STATUS2 register - include ERROR message. */
#define USBC_STATUS2_SYSMSG_SHIFT	0
#define USBC_STATUS2_SYSMSG_MASK	(0xff << USBC_STATUS2_SYSMSG_SHIFT)

/* MAX77705 DAT_IN register */
#define DAT_IN_SHIFT			0
#define DAT_IN_MASK			(0xff << DAT_IN_SHIFT)

/* MAX77705 DAT_OUT register */
#define DAT_OUT_SHIFT			0
#define DAT_OUT_MASK			(0xff << DAT_OUT_SHIFT)

/* MAX77705 BC_CTRL1 */
#define BC_CTRL1_DCDCpl_SHIFT		7
#define BC_CTRL1_UIDEN_SHIFT		6
#define BC_CTRL1_NoAutoIBUS_SHIFT	5
#define	BC_CTRL1_3ADCPDet_SHIFT		4
#define BC_CTRL1_CHGDetMan_SHIFT	1
#define BC_CTRL1_CHGDetEn_SHIFT		0
#define BC_CTRL1_DCDCpl_MASK		(0x1 << BC_CTRL1_DCDCpl_SHIFT)
#define BC_CTRL1_UIDEN_MASK		(0x1 << BC_CTRL1_UIDEN_SHIFT)
#define BC_CTRL1_NoAutoIBUS_MASK	(0x1 << BC_CTRL1_NoAutoIBUS_SHIFT)
#define	BC_CTRL1_3ADCPDet_MASK		(0x1 << BC_CTRL1_3ADCPDet_SHIFT)
#define BC_CTRL1_CHGDetMan_MASK		(0x1 << BC_CTRL1_CHGDetMan_SHIFT)
#define BC_CTRL1_CHGDetEn_MASK		(0x1 << BC_CTRL1_CHGDetEn_SHIFT)

/* MAX77705 BC_CTRL2 */
#define BC_CTRL2_DNMonEn_SHIFT		5
#define BC_CTRL2_DPDNMan_SHIFT		4
#define BC_CTRL2_DPDrv_SHIFT		2
#define	BC_CTRL2_DNDrv_SHIFT		0
#define BC_CTRL2_DNMonEn_MASK		(0x1 << BC_CTRL2_DNMonEn_SHIFT)
#define BC_CTRL2_DPDNMan_MASK		(0x1 << BC_CTRL2_DPDNMan_SHIFT)
#define BC_CTRL2_DPDrv_MASK		(0x3 << BC_CTRL2_DPDrv_SHIFT)
#define	BC_CTRL2_DNDrv_MASK		(0x3 << BC_CTRL2_DNDrv_SHIFT)

/* MAX77705 SWITCH COMMAND */
#define COMN1SW_SHIFT			0
#define COMP2SW_SHIFT			3
#define RCPS_SHIFT			6
#define NOBCCOMP_SHIFT			7
#define COMN1SW_MASK			(0x7 << COMN1SW_SHIFT)
#define COMP2SW_MASK			(0x7 << COMP2SW_SHIFT)
#define RCPS_MASK			(0x1 << RCPS_SHIFT)
#define NOBCCOMP_MASK			(0x1 << NOBCCOMP_SHIFT)

/* MAX77705 ID Monitor Config */
#define MODE_SHIFT			2
#define MODE_MASK			(0x3 << MODE_SHIFT)

enum {
	VB_LOW			= 0x00,
	VB_HIGH			= (0x1 << BC_STATUS_VBUSDET_SHIFT),

	VB_DONTCARE		= 0xff,
};

/* MAX77705 MUIC Charger Type Detection Output Value */
enum {
	/* No Valid voltage at VB (Vvb < Vvbdet) */
	CHGTYP_NO_VOLTAGE		= 0x00,
	/* Unknown (D+/D- does not present a valid USB charger signature) */
	CHGTYP_USB			= 0x01,
	/* Charging Downstream Port */
	CHGTYP_CDP			= 0x02,
	/* Dedicated Charger (D+/D- shorted) */
	CHGTYP_DEDICATED_CHARGER	= 0x03,

 	/* Hiccup mode, Set D+/D- to GND */
 	CHGTYP_HICCUP_MODE		= 0xfa,
	/* DCD Timeout, Open D+/D- */
	CHGTYP_TIMEOUT_OPEN		= 0xfb,
	/* Any charger w/o USB */
	CHGTYP_UNOFFICIAL_CHARGER	= 0xfc,
	/* Any charger type */
	CHGTYP_ANY			= 0xfd,
	/* Don't care charger type */
	CHGTYP_DONTCARE			= 0xfe,

	CHGTYP_MAX,
	CHGTYP_INIT,
	CHGTYP_MIN = CHGTYP_NO_VOLTAGE
};

/* MAX77705 MUIC Special Charger Type Detection Output value */
enum {
	PRCHGTYP_UNKNOWN		= 0x00,
	PRCHGTYP_SAMSUNG_2A		= 0x01,
	PRCHGTYP_APPLE_500MA		= 0x02,
	PRCHGTYP_APPLE_1A		= 0x03,
	PRCHGTYP_APPLE_2A		= 0x04,
	PRCHGTYP_APPLE_12W		= 0x05,
	PRCHGTYP_3A_DCP			= 0x06,
	PRCHGTYP_RFU			= 0x07,
};

enum {
	PROCESS_ATTACH		= 0,
	PROCESS_LOGICALLY_DETACH,
	PROCESS_NONE,
};

/* muic register value for COMN1, COMN2 in Switch command  */

/*
 * MAX77705 Switch values
 * NoBCComp [7] / RCPS [6] / D+ [5:3] / D- [2:0]
 * 0: Compare with BC1.2 / 1: Ignore BC1.2, Manual control
 * 0: Disable / 1: Enable
 * 000: Open / 001, 100: USB / 011, 101: UART
 */
enum max77705_switch_val {
	MAX77705_MUIC_NOBCCOMP_DIS	= 0x0,
	MAX77705_MUIC_NOBCCOMP_EN	= 0x1,

	MAX77705_MUIC_RCPS_DIS		= 0x0,
	MAX77705_MUIC_RCPS_EN		= 0x1,
	MAX77705_MUIC_RCPS_VAL		= MAX77705_MUIC_RCPS_DIS,

	MAX77705_MUIC_COM_USB		= 0x01,
	MAX77705_MUIC_COM_AUDIO		= 0x02,
	MAX77705_MUIC_COM_UART		= 0x03,
	MAX77705_MUIC_COM_USB_CP	= 0x04,
	MAX77705_MUIC_COM_UART_CP	= 0x05,
	MAX77705_MUIC_COM_OPEN		= 0x07,
};

enum {
	COM_OPEN	= (MAX77705_MUIC_NOBCCOMP_DIS << NOBCCOMP_SHIFT) |
			(MAX77705_MUIC_RCPS_VAL << RCPS_SHIFT) |
			(MAX77705_MUIC_COM_OPEN << COMP2SW_SHIFT) |
			(MAX77705_MUIC_COM_OPEN << COMN1SW_SHIFT),
	COM_USB		= (MAX77705_MUIC_NOBCCOMP_DIS << NOBCCOMP_SHIFT) |
			(MAX77705_MUIC_RCPS_VAL << RCPS_SHIFT) |
			(MAX77705_MUIC_COM_USB << COMP2SW_SHIFT) |
			(MAX77705_MUIC_COM_USB << COMN1SW_SHIFT),
	COM_UART	= (MAX77705_MUIC_NOBCCOMP_EN << NOBCCOMP_SHIFT) |
			(MAX77705_MUIC_RCPS_VAL << RCPS_SHIFT) |
			(MAX77705_MUIC_COM_UART << COMP2SW_SHIFT) |
			(MAX77705_MUIC_COM_UART << COMN1SW_SHIFT),
	COM_USB_CP	= (MAX77705_MUIC_NOBCCOMP_EN << NOBCCOMP_SHIFT) |
			(MAX77705_MUIC_RCPS_VAL << RCPS_SHIFT) |
			(MAX77705_MUIC_COM_USB_CP << COMP2SW_SHIFT) |
			(MAX77705_MUIC_COM_USB_CP << COMN1SW_SHIFT),
	COM_UART_CP	= (MAX77705_MUIC_NOBCCOMP_EN << NOBCCOMP_SHIFT) |
			(MAX77705_MUIC_RCPS_VAL << RCPS_SHIFT) |
			(MAX77705_MUIC_COM_UART_CP << COMP2SW_SHIFT) |
			(MAX77705_MUIC_COM_UART_CP << COMN1SW_SHIFT),
};

extern struct muic_platform_data muic_pdata;
extern struct device *switch_device;
extern int max77705_muic_probe(struct max77705_usbc_platform_data *usbc_data);
extern int max77705_muic_remove(struct max77705_usbc_platform_data *usbc_data);
extern void max77705_muic_shutdown(struct max77705_usbc_platform_data *usbc_data);
extern int max77705_muic_suspend(struct max77705_usbc_platform_data *usbc_data);
extern int max77705_muic_resume(struct max77705_usbc_platform_data *usbc_data);
#if defined(CONFIG_HV_MUIC_MAX77705_AFC)
extern bool max77705_muic_check_is_enable_afc(struct max77705_muic_data *muic_data, muic_attached_dev_t new_dev);
extern void max77705_muic_check_afc_disabled(struct max77705_muic_data *muic_data);
extern void max77705_muic_clear_hv_control(struct max77705_muic_data *muic_data);
extern void max77705_muic_afc_hv_set(struct max77705_muic_data *muic_data, int voltage);
extern void max77705_muic_qc_hv_set(struct max77705_muic_data *muic_data, int voltage);
extern void max77705_muic_handle_detect_dev_afc(struct max77705_muic_data *muic_data, unsigned char *data);
extern void max77705_muic_handle_detect_dev_qc(struct max77705_muic_data *muic_data, unsigned char *data);
extern void max77705_muic_handle_detect_dev_hv(struct max77705_muic_data *muic_data, unsigned char *data);
extern void max77705_muic_disable_afc_protocol(struct max77705_muic_data *muic_data);
#endif /* CONFIG_HV_MUIC_MAX77705_AFC */
#if defined(CONFIG_MUIC_MAX77705_CCIC)
extern void max77705_muic_register_ccic_notifier(struct max77705_muic_data *muic_data);
extern void max77705_muic_unregister_ccic_notifier(struct max77705_muic_data *muic_data);
#endif /* CONFIG_MUIC_MAX77705_CCIC */
#if defined(CONFIG_USB_EXTERNAL_NOTIFY)
extern void muic_send_dock_intent(int type);
#endif /* CONFIG_USB_EXTERNAL_NOTIFY */

#endif /* __MAX77705_MUIC_H__ */

