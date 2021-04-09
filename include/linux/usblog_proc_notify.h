/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2016-2020 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

  /* usb notify layer v3.5 */

#ifndef __LINUX_USBLOG_PROC_NOTIFY_H__
#define __LINUX_USBLOG_PROC_NOTIFY_H__

enum usblog_type {
	NOTIFY_FUNCSTATE,
	NOTIFY_ALTERNATEMODE,
	NOTIFY_CCIC_EVENT,
	NOTIFY_MANAGER,
	NOTIFY_USBMODE,
	NOTIFY_USBMODE_EXTRA,
	NOTIFY_USBSTATE,
	NOTIFY_EVENT,
	NOTIFY_PORT_CONNECT,
	NOTIFY_PORT_DISCONNECT,
	NOTIFY_PORT_CLASS,
	NOTIFY_PCM_PLAYBACK,
	NOTIFY_PCM_CAPTURE,
	NOTIFY_EXTRA,
};

enum usblog_state {
	NOTIFY_CONFIGURED = 1,
	NOTIFY_CONNECTED,
	NOTIFY_DISCONNECTED,
	NOTIFY_RESET,
	NOTIFY_RESET_FULL,
	NOTIFY_RESET_HIGH,
	NOTIFY_RESET_SUPER,
	NOTIFY_ACCSTART,
	NOTIFY_PULLUP,
	NOTIFY_PULLUP_ENABLE,
	NOTIFY_PULLUP_EN_SUCCESS,
	NOTIFY_PULLUP_EN_FAIL,
	NOTIFY_PULLUP_DISABLE,
	NOTIFY_PULLUP_DIS_SUCCESS,
	NOTIFY_PULLUP_DIS_FAIL,
	NOTIFY_VBUS_SESSION,
	NOTIFY_VBUS_SESSION_ENABLE,
	NOTIFY_VBUS_EN_SUCCESS,
	NOTIFY_VBUS_EN_FAIL,
	NOTIFY_VBUS_SESSION_DISABLE,
	NOTIFY_VBUS_DIS_SUCCESS,
	NOTIFY_VBUS_DIS_FAIL,
	NOTIFY_HIGH,
	NOTIFY_SUPER,
	NOTIFY_GET_DES,
	NOTIFY_SET_CON,
	NOTIFY_CONNDONE_SSP,
	NOTIFY_CONNDONE_SS,
	NOTIFY_CONNDONE_HS,
	NOTIFY_CONNDONE_FS,
	NOTIFY_CONNDONE_LS,
};

enum usblog_status {
	NOTIFY_DETACH = 0,
	NOTIFY_ATTACH_DFP,
	NOTIFY_ATTACH_UFP,
	NOTIFY_ATTACH_DRP,
};

/*
 *	You should refer "linux/usb/typec/common/pdic_notifier.h"
 *	ccic_device, ccic_id may be different at each branch
 */
enum ccic_device {
	NOTIFY_DEV_INITIAL = 0,
	NOTIFY_DEV_USB,
	NOTIFY_DEV_BATTERY,
	NOTIFY_DEV_PDIC,
	NOTIFY_DEV_MUIC,
	NOTIFY_DEV_CCIC,
	NOTIFY_DEV_MANAGER,
	NOTIFY_DEV_DP,
	NOTIFY_DEV_USB_DP,
	NOTIFY_DEV_SUB_BATTERY,
	NOTIFY_DEV_SECOND_MUIC,
};

enum ccic_id {
	NOTIFY_ID_INITIAL = 0,
	NOTIFY_ID_ATTACH,
	NOTIFY_ID_RID,
	NOTIFY_ID_USB,
	NOTIFY_ID_POWER_STATUS,
	NOTIFY_ID_WATER,
	NOTIFY_ID_VCONN,
	NOTIFY_ID_OTG,
	NOTIFY_ID_TA,
	NOTIFY_ID_DP_CONNECT,
	NOTIFY_ID_DP_HPD,
	NOTIFY_ID_DP_LINK_CONF,
	NOTIFY_ID_USB_DP,
	NOTIFY_ID_ROLE_SWAP,
	NOTIFY_ID_FAC,
	NOTIFY_ID_CC_PIN_STATUS,
	NOTIFY_ID_WATER_CABLE,
};

enum ccic_rid {
	NOTIFY_RID_UNDEFINED = 0,
#if defined(CONFIG_USB_CCIC_NOTIFIER_USING_QC)
	NOTIFY_RID_GND,
	NOTIFY_RID_056K,
#else
	NOTIFY_RID_000K,
	NOTIFY_RID_001K,
#endif
	NOTIFY_RID_255K,
	NOTIFY_RID_301K,
	NOTIFY_RID_523K,
	NOTIFY_RID_619K,
	NOTIFY_RID_OPEN,
};

enum ccic_con {
	NOTIFY_CON_DETACH = 0,
	NOTIFY_CON_ATTACH,
};

enum ccic_rprd {
	NOTIFY_RD = 0,
	NOTIFY_RP,
};

enum ccic_rpstatus {
	NOTIFY_RP_NONE = 0,
	NOTIFY_RP_56K,	/* 80uA */
	NOTIFY_RP_22K,		/* 180uA */
	NOTIFY_RP_10K,		/* 330uA */
	NOTIFY_RP_ABNORMAL,
};

enum ccic_hpd {
	NOTIFY_HPD_LOW = 0,
	NOTIFY_HPD_HIGH,
	NOTIFY_HPD_IRQ,
};

enum ccic_pin_assignment {
	NOTIFY_DP_PIN_UNKNOWN = 0,
	NOTIFY_DP_PIN_A,
	NOTIFY_DP_PIN_B,
	NOTIFY_DP_PIN_C,
	NOTIFY_DP_PIN_D,
	NOTIFY_DP_PIN_E,
	NOTIFY_DP_PIN_F,
};

enum ccic_pin_status {
	NOTIFY_PIN_NOTERMINATION = 0,
	NOTIFY_PIN_CC1_ACTIVE,
	NOTIFY_PIN_CC2_ACTIVE,
	NOTIFY_PIN_AUDIO_ACCESSORY,
};

enum extra {
	NOTIFY_EXTRA_USBKILLER = 0,
	NOTIFY_EXTRA_HARDRESET_SENT,
	NOTIFY_EXTRA_HARDRESET_RECEIVED,
	NOTIFY_EXTRA_SYSERROR_BOOT_WDT,
	NOTIFY_EXTRA_SYSMSG_BOOT_POR,
	NOTIFY_EXTRA_SYSMSG_CC_SHORT,
	NOTIFY_EXTRA_SYSMSG_SBU_GND_SHORT,
	NOTIFY_EXTRA_SYSMSG_SBU_VBUS_SHORT,
	NOTIFY_EXTRA_UVDM_TIMEOUT,
	NOTIFY_EXTRA_CCOPEN_REQ_SET,
	NOTIFY_EXTRA_CCOPEN_REQ_CLEAR,
	NOTIFY_EXTRA_USB_ANALOGAUDIO,
	NOTIFY_EXTRA_USBHOST_OVERCURRENT,
	NOTIFY_EXTRA_ROOTHUB_SUSPEND_FAIL,
	NOTIFY_EXTRA_PORT_SUSPEND_FAIL,
	NOTIFY_EXTRA_PORT_SUSPEND_WAKEUP_FAIL,
	NOTIFY_EXTRA_PORT_SUSPEND_LTM_FAIL,
};

#define ALTERNATE_MODE_NOT_READY	(1 << 0)
#define ALTERNATE_MODE_READY		(1 << 1)
#define ALTERNATE_MODE_STOP		(1 << 2)
#define ALTERNATE_MODE_START		(1 << 3)
#define ALTERNATE_MODE_RESET		(1 << 4)

#ifdef CONFIG_USB_NOTIFY_PROC_LOG
extern void store_usblog_notify(int type, void *param1, void *param2);
extern void store_ccic_version(unsigned char *hw, unsigned char *sw_main,
			unsigned char *sw_boot);
extern unsigned long long show_ccic_version(void);
extern void store_ccic_bin_version(const unsigned char *sw_main,
					const unsigned char *sw_boot);
extern int register_usblog_proc(void);
extern void unregister_usblog_proc(void);
#else
static inline void store_usblog_notify(int type, void *param1, void *param2) {}
static inline void store_ccic_version(unsigned char *hw, unsigned char *sw_main,
			unsigned char *sw_boot) {}
static inline unsigned long long show_ccic_version(void) {return 0; }
static inline void store_ccic_bin_version(const unsigned char *sw_main,
			const unsigned char *sw_boot) {}
static inline int register_usblog_proc(void)
			{return 0; }
static inline void unregister_usblog_proc(void) {}
#endif
#endif

