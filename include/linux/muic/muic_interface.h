/*
 * Copyright (C) 2010 Samsung Electronics
 * Thomas Ryu <smilesr.ryu@samsung.com>
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
 */

#ifndef __MUIC_INTERNAL_H__
#define __MUIC_INTERNAL_H__

#include <linux/muic/muic_core.h>

#define muic_err(fmt, ...)					\
	do {							\
		pr_err(pr_fmt(fmt), ##__VA_ARGS__);		\
	} while (0)

#define muic_info(fmt, ...)					\
	do {							\
		pr_info(pr_fmt(fmt), ##__VA_ARGS__);		\
	} while (0)

#define muic_dbg(fmt, ...)					\
	do {							\
		pr_debug(pr_fmt(fmt), ##__VA_ARGS__);		\
	} while (0)

enum muic_op_mode {
	OPMODE_SMD_ARRAY = 0<<0,
	OPMODE_DEVICE = 1<<0,
};

/* Slave addr = 0x4A: MUIC */
enum ioctl_cmd {
	GET_COM_VAL = 0x01,
	GET_CTLREG = 0x02,
	GET_ADC = 0x03,
	GET_SWITCHING_MODE = 0x04,
	GET_INT_MASK = 0x05,
	GET_REVISION = 0x06,
	GET_OTG_STATUS = 0x7,
	GET_CHGTYPE = 0x08,
	GET_RESID3 = 0x09,
};

enum switching_mode {
	SWMODE_MANUAL = 0,
	SWMODE_AUTO = 1,
};

/*
 * Manual Switch
 * D- [7:5] / D+ [4:2] / Vbus [1:0]
 * 000: Open all / 001: USB / 010: AUDIO / 011: UART / 100: V_AUDIO
 * 00: Vbus to Open / 01: Vbus to Charger / 10: Vbus to MIC / 11: Vbus to VBout
 */

/* COM port index */
enum com_index {
	COM_OPEN = 1,
	COM_OPEN_WITH_V_BUS = 2,
	COM_UART_AP = 3,
	COM_UART_CP = 4,
	COM_USB_AP  = 5,
	COM_USB_CP  = 6,
	COM_AUDIO   = 7,
};

enum {
	ADC_SCANMODE_CONTINUOUS = 0x0,
	ADC_SCANMODE_ONESHOT = 0x1,
	ADC_SCANMODE_PULSE = 0x2,
};

enum vps_type {
	VPS_TYPE_SCATTERED = 0,
	VPS_TYPE_TABLE = 1,
};

/* VPS data from a chip. */
typedef struct _muic_vps_scatterred_type {
	u8 val1;
	u8 val2;
	u8 val3;
	u8 adc;
	u8 vbvolt;
} vps_scatterred_type;

typedef struct _muic_vps_table_t {
	u8  adc;
	u8  vbvolt;
	u8  adc1k;
	u8  adcerr;
	u8  adclow;
	u8  chgdetrun;
	u8  chgtyp;
	u8  DCDTimedout;
	const char *vps_name;
	const muic_attached_dev_t attached_dev;
	u8 status[3];
	u8 control[4];
	u8 hvcontrol[2];
} vps_table_type;

struct muic_intr_data {
	u8	intr1;
	u8	intr2;
};

struct muic_irq_t {
	int irq_adc1k;
	int irq_adcerr;
	int irq_adc;
	int irq_chgtyp;
	int irq_vbvolt;
	int irq_dcdtmr;
};

typedef union _muic_vps_t {
	vps_scatterred_type s;
	vps_table_type t;
	char vps_data[120];
} vps_data_t;

/* muic chip specific internal data structure
 * that setted at muic-xxxx.c file
 */
struct regmap_desc;

typedef enum {
	CCIC_RID_UNDEFINED = 0,
	CCIC_RID_000K,
	CCIC_RID_001K,
	CCIC_RID_255K,
	CCIC_RID_301K,
	CCIC_RID_523K,
	CCIC_RID_619K,
	CCIC_RID_OPEN,
} muic_ccic_rid_t;

struct ccic_rid_desc_t {
	char *name;
	int attached_dev;
};

struct ccic_desc_t {
	int ccic_evt_attached; /* 1: attached, -1: detached, 0: undefined */
	int ccic_evt_rid; /* the last rid */
	int ccic_evt_rprd; /*rprd */
	int ccic_evt_roleswap; /* check rprd role swap event */
	int ccic_evt_dcdcnt; /* count dcd timeout */
	int attached_dev; /* attached dev */
	struct ccic_rid_desc_t *rid_desc;
};

enum {
	MUIC_NORMAL_OTG,
	MUIC_ABNORMAL_OTG,
};

struct muic_interface_t {
	struct device *dev;
	struct i2c_client *i2c; /* i2c addr: 0x4A; MUIC */
	struct mutex muic_mutex;
	struct notifier_block nb;

	/* model dependant muic platform data */
	struct muic_platform_data *pdata;
	struct ccic_desc_t *ccic;

	void *muic_data;

	/* muic current attached device */
	muic_attached_dev_t attached_dev;

	vps_data_t vps;
	int vps_table;

	struct muic_intr_data intr;
	struct muic_irq_t irqs;

	/* regmap_desc_t */
	struct regmap_desc *regmapdesc;

	char *chip_name;

	int gpio_uart_sel;
	int gpio_usb_sel;

	/* muic Device ID */
	u8 muic_vendor;			/* Vendor ID */
	u8 muic_version;		/* Version ID */

	bool			is_gamepad;
	bool			is_factory_start;
	bool			is_rustproof;
	bool			is_otg_test;
	struct delayed_work	init_work;
	struct delayed_work	usb_work;

	bool			is_muic_ready;
	bool			undefined_range;
	bool			discard_interrupt;
	bool			is_dcdtmr_intr;
	bool			is_dcp_charger;
	bool			is_afc_reset;

	struct hv_data		*phv;

#if defined(CONFIG_USB_EXTERNAL_NOTIFY)
	/* USB Notifier */
	struct notifier_block	usb_nb;
#endif

#if defined(CONFIG_MUIC_MANAGER)
	/* legacy TA or USB for CCIC */
	muic_attached_dev_t	legacy_dev;

#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	struct notifier_block	manager_nb;
#else
	struct notifier_block	ccic_nb;
#endif
	struct delayed_work	ccic_work;
	bool afc_water_disable;
#endif
	/* Operation Mode */
	enum muic_op_mode	opmode;

	/* function pointer should be registered from each specific driver file */
	int (*set_com_to_open)(void *);
	int (*set_switch_to_usb)(void *);
	int (*set_switch_to_uart)(void *);
	void (*set_jig_state)(void *, bool val);
	int (*set_jig_ctrl_on)(void *);
	void (*set_cable_state)(void *, muic_attached_dev_t new_dev);
	void (*set_otg_detect_en)(void *, bool en);
	void (*set_dcd_rescan)(void *);
	void (*set_afc_ready)(void *, bool en);
	int (*bcd_rescan)(void *);
	int (*control_rid_adc)(void *, bool enable);
#if defined(CONFIG_MUIC_S2MU004_HV)
	int (*set_afc_reset)(void *);
	muic_attached_dev_t (*check_id_err)(void *, muic_attached_dev_t new_dev);
	int (*reset_hvcontrol_reg)(void *);
	int (*check_afc_ready)(void *);
	int (*reset_afc_register)(void *);
#endif
	void (*set_water_detect)(void *, bool val);
	int (*set_com_to_audio)(void *);
	int (*set_com_to_otg)(void *);
	int (*set_gpio_usb_sel)(void *, int usb_path);
	int (*set_gpio_uart_sel)(void *, int uart_path);
	int (*get_vbus)(void *);
	int (*get_adc)(void *);
};

extern struct device *switch_device;

struct muic_interface_t *muic_manager_init(void *pdata, void *drv_data);
void muic_manager_exit(struct muic_interface_t *muic_if);
int muic_manager_get_legacy_dev(struct muic_interface_t *muic_if);
void muic_manager_set_legacy_dev(struct muic_interface_t *muic_if, int new_dev);
void muic_manager_handle_ccic_detach(struct muic_interface_t *muic_if);
int muic_manager_dcd_rescan(struct muic_interface_t *muic_if);

#endif /* __MUIC_INTERNAL_H__ */
