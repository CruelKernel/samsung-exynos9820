/* SPDX-License-Identifier: GPL-2.0 */
/*
 *  usb notify header
 *
 * Copyright (C) 2011-2020 Samsung, Inc.
 * Author: Dongrak Shin <dongrak.shin@samsung.com>
 *
 */

 /* usb notify layer v3.5 */

#ifndef __LINUX_USB_NOTIFY_H__
#define __LINUX_USB_NOTIFY_H__

#include <linux/notifier.h>
#include <linux/host_notify.h>
#include <linux/external_notify.h>
#include <linux/usblog_proc_notify.h>
#if defined(CONFIG_USB_HW_PARAM)
#include <linux/usb_hw_param.h>
#endif
#include <linux/usb.h>

enum otg_notify_events {
	NOTIFY_EVENT_NONE,
	NOTIFY_EVENT_VBUS,
	NOTIFY_EVENT_HOST,
	NOTIFY_EVENT_CHARGER,
	NOTIFY_EVENT_SMARTDOCK_TA,
	NOTIFY_EVENT_SMARTDOCK_USB,
	NOTIFY_EVENT_AUDIODOCK,
	NOTIFY_EVENT_LANHUB,
	NOTIFY_EVENT_LANHUB_TA,
	NOTIFY_EVENT_MMDOCK,
	NOTIFY_EVENT_HMT,
	NOTIFY_EVENT_GAMEPAD,
	NOTIFY_EVENT_POGO,
	NOTIFY_EVENT_HOST_RELOAD,
	NOTIFY_EVENT_DRIVE_VBUS,
	NOTIFY_EVENT_ALL_DISABLE,
	NOTIFY_EVENT_HOST_DISABLE,
	NOTIFY_EVENT_CLIENT_DISABLE,
	NOTIFY_EVENT_MDM_ON_OFF,
	NOTIFY_EVENT_OVERCURRENT,
	NOTIFY_EVENT_SMSC_OVC,
	NOTIFY_EVENT_SMTD_EXT_CURRENT,
	NOTIFY_EVENT_MMD_EXT_CURRENT,
	NOTIFY_EVENT_HMD_EXT_CURRENT,
	NOTIFY_EVENT_DEVICE_CONNECT,
	NOTIFY_EVENT_GAMEPAD_CONNECT,
	NOTIFY_EVENT_LANHUB_CONNECT,
	NOTIFY_EVENT_POWER_SOURCE,
	NOTIFY_EVENT_PD_CONTRACT,
	NOTIFY_EVENT_VBUS_RESET,
	NOTIFY_EVENT_RESERVE_BOOSTER,
	NOTIFY_EVENT_USB_CABLE,
	NOTIFY_EVENT_USBD_SUSPENDED,
	NOTIFY_EVENT_USBD_UNCONFIGURED,
	NOTIFY_EVENT_USBD_CONFIGURED,
	NOTIFY_EVENT_VBUSPOWER,
	NOTIFY_EVENT_VIRTUAL,
};

#define VIRT_EVENT(a) (a+NOTIFY_EVENT_VIRTUAL)
#define PHY_EVENT(a) (a%NOTIFY_EVENT_VIRTUAL)
#define IS_VIRTUAL(a) (a >= NOTIFY_EVENT_VIRTUAL ? 1 : 0)

enum otg_notify_event_status {
	NOTIFY_EVENT_DISABLED,
	NOTIFY_EVENT_DISABLING,
	NOTIFY_EVENT_ENABLED,
	NOTIFY_EVENT_ENABLING,
	NOTIFY_EVENT_BLOCKED,
	NOTIFY_EVENT_BLOCKING,
};

enum otg_notify_evt_type {
	NOTIFY_EVENT_EXTRA = (1 << 0),
	NOTIFY_EVENT_STATE = (1 << 1),
	NOTIFY_EVENT_DELAY = (1 << 2),
	NOTIFY_EVENT_NEED_VBUSDRIVE = (1 << 3),
	NOTIFY_EVENT_NOBLOCKING = (1 << 4),
	NOTIFY_EVENT_NOSAVE = (1 << 5),
	NOTIFY_EVENT_NEED_HOST = (1 << 6),
	NOTIFY_EVENT_NEED_CLIENT = (1 << 7),
};

enum otg_notify_block_type {
	NOTIFY_BLOCK_TYPE_NONE = 0,
	NOTIFY_BLOCK_TYPE_HOST = (1 << 0),
	NOTIFY_BLOCK_TYPE_CLIENT = (1 << 1),
	NOTIFY_BLOCK_TYPE_ALL = (1 << 0 | 1 << 1),
};

enum otg_notify_mdm_type {
	NOTIFY_MDM_TYPE_OFF,
	NOTIFY_MDM_TYPE_ON,
};

enum otg_notify_gpio {
	NOTIFY_VBUS,
	NOTIFY_REDRIVER,
};

enum otg_op_pos {
	NOTIFY_OP_OFF,
	NOTIFY_OP_POST,
	NOTIFY_OP_PRE,
};

enum ovc_check_value {
	HNOTIFY_LOW,
	HNOTIFY_HIGH,
	HNOTIFY_INITIAL,
};

enum otg_notify_power_role {
	HNOTIFY_SINK,
	HNOTIFY_SOURCE,
};

enum otg_notify_data_role {
	HNOTIFY_UFP,
	HNOTIFY_DFP,
};

enum usb_certi_type {
	USB_CERTI_UNSUPPORT_ACCESSORY,
	USB_CERTI_NO_RESPONSE,
	USB_CERTI_HUB_DEPTH_EXCEED,
	USB_CERTI_HUB_POWER_EXCEED,
	USB_CERTI_HOST_RESOURCE_EXCEED,
};

enum usb_err_type {
	USB_ERR_ABNORMAL_RESET,
};

enum usb_itracker_type {
	NOTIFY_USB_CC_REPEAT,
};

enum usb_current_state {
	NOTIFY_USB_SUSPENDED,
	NOTIFY_USB_UNCONFIGURED,
	NOTIFY_USB_CONFIGURED,
};

struct otg_notify {
	int vbus_detect_gpio;
	int redriver_en_gpio;
	int is_wakelock;
	int is_host_wakelock;
	int unsupport_host;
	int smsc_ovc_poll_sec;
	int auto_drive_vbus;
	int booting_delay_sec;
	int disable_control;
	int device_check_sec;
	int pre_peri_delay_us;
	int speed;
	int (*pre_gpio)(int gpio, int use);
	int (*post_gpio)(int gpio, int use);
	int (*vbus_drive)(bool enable);
	int (*set_host)(bool enable);
	int (*set_peripheral)(bool enable);
	int (*set_charger)(bool enable);
	int (*post_vbus_detect)(bool on);
	int (*set_lanhubta)(int enable);
	int (*set_battcall)(int event, int enable);
	int (*set_chg_current)(int state);
	void (*set_ldo_onoff)(void *data, unsigned int onoff);
	int (*get_gadget_speed)(void);
	void *o_data;
	void *u_notify;
};

struct otg_booster {
	char *name;
	int (*booster)(bool enable);
};

#if IS_ENABLED(CONFIG_USB_NOTIFY_LAYER)
extern const char *event_string(enum otg_notify_events event);
extern const char *status_string(enum otg_notify_event_status status);
extern void send_usb_mdm_uevent(void);
extern void send_usb_certi_uevent(int usb_certi);
extern void send_usb_err_uevent(int usb_certi, int mode);
extern void send_usb_itracker_uevent(int err_type);
extern int usb_check_whitelist_for_mdm(struct usb_device *dev);
extern int usb_otg_restart_accessory(struct usb_device *dev);
extern void send_otg_notify(struct otg_notify *n,
					unsigned long event, int enable);
extern int get_typec_status(struct otg_notify *n, int event);
extern struct otg_booster *find_get_booster(struct otg_notify *n);
extern int register_booster(struct otg_notify *n, struct otg_booster *b);
extern int register_ovc_func(struct otg_notify *n,
				int (*check_state)(void *), void *data);
extern int get_booster(struct otg_notify *n);
extern int get_usb_mode(struct otg_notify *n);
extern unsigned long get_cable_type(struct otg_notify *n);
extern int is_usb_host(struct otg_notify *n);
extern bool is_blocked(struct otg_notify *n, int type);
extern bool is_snkdfp_usb_device_connected(struct otg_notify *n);
extern int is_known_usbaudio(struct usb_device *dev);
extern void set_usb_audio_cardnum(int card_num, int bundle, int attach);
extern void send_usb_audio_uevent(struct usb_device *dev,
		int cardnum, int attach);
extern int send_usb_notify_uevent
		(struct otg_notify *n, char *envp_ext[]);
#if defined(CONFIG_USB_HW_PARAM)
extern unsigned long long *get_hw_param(struct otg_notify *n,
					enum usb_hw_param index);
extern int inc_hw_param(struct otg_notify *n,
					enum usb_hw_param index);
extern int inc_hw_param_host(struct host_notify_dev *dev,
					enum usb_hw_param index);
extern int register_hw_param_manager(struct otg_notify *n,
					unsigned long (*fptr)(int));
#endif
extern void *get_notify_data(struct otg_notify *n);
extern void set_notify_data(struct otg_notify *n, void *data);
extern struct otg_notify *get_otg_notify(void);
extern int set_otg_notify(struct otg_notify *n);
extern void put_otg_notify(struct otg_notify *n);
#else
static inline const char *event_string(enum otg_notify_events event)
			{return NULL; }
static inline const char *status_string(enum otg_notify_event_status status)
			{return NULL; }
static inline void send_usb_mdm_uevent(void) {}
static inline void send_usb_certi_uevent(int usb_certi) {}
static inline void send_usb_err_uevent(int usb_certi, int mode) {}
static inline void send_usb_itracker_uevent(int err_type) {}
static inline int usb_check_whitelist_for_mdm(struct usb_device *dev)
			{return 0; }
static inline int usb_otg_restart_accessory(struct usb_device *dev)
			{return 0; }
static inline void send_otg_notify(struct otg_notify *n,
					unsigned long event, int enable) { }
static inline int get_typec_status(struct otg_notify *n, int event) {return 0; }
static inline struct otg_booster *find_get_booster(struct otg_notify *n)
			{return NULL; }
static inline int register_booster(struct otg_notify *n,
					struct otg_booster *b) {return 0; }
static inline int register_ovc_func(struct otg_notify *n,
			int (*check_state)(void *), void *data) {return 0; }
static inline int get_booster(struct otg_notify *n) {return 0; }
static inline int get_usb_mode(struct otg_notify *n) {return 0; }
static inline unsigned long get_cable_type(struct otg_notify *n) {return 0; }
static inline int is_usb_host(struct otg_notify *n) {return 0; }
static inline bool is_blocked(struct otg_notify *n, int type) {return false; }
static inline bool is_snkdfp_usb_device_connected(struct otg_notify *n)
			{return false; }
static inline int is_known_usbaudio(struct usb_device *dev) {return 0; }
static inline void set_usb_audio_cardnum(int card_num,
		int bundle, int attach) {}
static inline void send_usb_audio_uevent(struct usb_device *dev,
		int cardnum, int attach) {}
static inline int send_usb_notify_uevent
			(struct otg_notify *n, char *envp_ext[]) {return 0; }
#if defined(CONFIG_USB_HW_PARAM)
static inline unsigned long long *get_hw_param(struct otg_notify *n,
			enum usb_hw_param index) {return NULL; }
static inline int inc_hw_param(struct otg_notify *n,
			enum usb_hw_param index) {return 0; }
static inline int inc_hw_param_host(struct host_notify_dev *dev,
			enum usb_hw_param index) {return 0; }
static inline int register_hw_param_manager(struct otg_notify *n,
			unsigned long (*fptr)(int)) {return 0; }
#endif
static inline void *get_notify_data(struct otg_notify *n) {return NULL; }
static inline void set_notify_data(struct otg_notify *n, void *data) {}
static inline struct otg_notify *get_otg_notify(void) {return NULL; }
static inline int set_otg_notify(struct otg_notify *n) {return 0; }
static inline void put_otg_notify(struct otg_notify *n) {}
#endif
#endif /* __LINUX_USB_NOTIFY_H__ */
