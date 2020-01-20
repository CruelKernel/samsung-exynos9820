#ifndef _USBPD_TYPEC_H
#define _USBPD_TYPEC_H

typedef enum {
	TYPE_C_DETACH = 0,
	TYPE_C_ATTACH_DFP = 1, // Host
	TYPE_C_ATTACH_UFP = 2, // Device
	TYPE_C_ATTACH_DRP = 3, // Dual role
} CCIC_OTP_MODE;

typedef enum {
	USB_STATUS_NOTIFY_DETACH = 0,
	USB_STATUS_NOTIFY_ATTACH_DFP = 1, // Host
	USB_STATUS_NOTIFY_ATTACH_UFP = 2, // Device
	USB_STATUS_NOTIFY_ATTACH_DRP = 3, // Dual role
	USB_STATUS_NOTIFY_ATTACH_HPD = 4, // DP : Hot Plugged Detect
} USB_STATUS;

#define DUAL_ROLE_SET_MODE_WAIT_MS 1500
typedef enum {
	CLIENT_OFF = 0,
	CLIENT_ON = 1,
} CCIC_DEVICE_REASON;

typedef enum {
	HOST_OFF = 0,
	HOST_ON_BY_RD = 1, // Rd detection
	HOST_ON_BY_RID000K = 2, // RID000K detection
} CCIC_HOST_REASON;

typedef enum {
	VBUS_OFF = 0,
	VBUS_ON = 1,
} CCIC_VBUS_SEL;

typedef enum {
	Rp_None = 0,
	Rp_56K = 1,	/* 80uA */
	Rp_22K = 2,	/* 180uA */
	Rp_10K = 3,	/* 330uA */
	Rp_Abnormal = 4,
} CCIC_RP_CurrentLvl;

typedef enum {
	WATER_DRY = 0,
	WATER_DETECT = 1,
} CCIC_WATER;

typedef enum {
	CCIC_NOTIFY_DP_PIN_UNKNOWN = 0,
	CCIC_NOTIFY_DP_PIN_A,
	CCIC_NOTIFY_DP_PIN_B,
	CCIC_NOTIFY_DP_PIN_C,
	CCIC_NOTIFY_DP_PIN_D,
	CCIC_NOTIFY_DP_PIN_E,
	CCIC_NOTIFY_DP_PIN_F,
} ccic_notifier_dp_pinconf_t;

typedef enum {
	CCIC_NOTIFY_LOW = 0,
	CCIC_NOTIFY_HIGH,
	CCIC_NOTIFY_IRQ,
} ccic_notifier_dp_hpd_t;
#endif
