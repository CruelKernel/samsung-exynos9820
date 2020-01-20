#ifndef __CORE_H
#define __CORE_H

#include <linux/device.h>
#include <linux/types.h>
#include <linux/ccic/usbpd_msg.h>
#include <linux/ccic/usbpd_typec.h>
#include <linux/ccic/usbpd_config.h>
#if defined(CONFIG_SAMSUNG_BATTERY)
#include <linux/battery/battery_notifier.h>
#endif

#if defined(CONFIG_IFCONN_NOTIFIER)
#include <linux/ifconn/ifconn_notifier.h>
#endif

#if defined(CONFIG_DUAL_ROLE_USB_INTF)
#include <linux/usb/class-dual-role.h>
#endif

#define PDO_MAX	7

struct usbpd_init_data;
struct usbpd_info;
struct usbpd_ops;
struct usbpd_desc;
struct usbpd_dev;

typedef enum usbpd_attach_mode {
	USBPD_DETACHED,
	USBPD_SRC_MODE,
	USBPD_SNK_MODE,
} usbpd_attach_mode_t;

typedef enum usbpd_msg_state {
	USBPD_MSG_SRC_CAP,
	USBPD_MSG_PR_SWAP,
	USBPD_MSG_DR_SWAP,
	USBPD_MSG_DISCOVER_ID,
	USBPD_MSG_DISCOVER_SVIDS,
	USBPD_MSG_DISCOVER_MODES,
	USBPD_MSG_ENTER_MODE,
	USBPD_MSG_EXIT_MODE,
	USBPD_MSG_DP_STATUS_UPDATE,
	USBPD_MSG_DP_CONFIGURE,
	USBPD_MSG_ATTENTION,
} usbpd_msg_state_t;

typedef struct _usbpd_power_list_t {
	int max_voltage;
	int max_current;
} usbpd_power_list_t;

typedef struct _usbpd_pdic_sink_state_t {
	usbpd_power_list_t power_list[PDO_MAX];
	int available_pdo_num; // the number of available PDO
	int selected_pdo_num; // selected number of PDO to change
	int current_pdo_num; // current number of PDO
	unsigned int rp_currentlvl; // rp current level by ccic
} usbpd_pdic_sink_state_t;

struct pdic_notifier_data {
	usbpd_pdic_sink_state_t sink_status;
	void *pusbpd;
};

/**
 * struct usbpd_init_data - usbpd dtsi initialisatioan data.
 *
 * Initialisation default request message, source & sink power information
 *
 */
struct usbpd_init_data {
	/* define first request */
	int max_power;
	int op_power;
	int max_current;
	int min_current;
	int op_current;
	bool giveback;
	bool usb_com_capable;
	bool no_usb_suspend;

	/* device capable charing voltage */
	int max_charging_volt;

	/* source */
	int source_max_volt;
	int source_min_volt;
	int source_max_power;

	/* sink */
	int sink_max_volt;
	int sink_min_volt;
	int sink_max_power;

	/* sink cap */
	int sink_cap_max_volt;
	int sink_cap_min_volt;
	int sink_cap_operational_current;
	int sink_cap_power_type;

	/* power role swap*/
	bool power_role_swap;
	/* data role swap*/
	bool data_role_swap;
	bool vconn_source_swap;
};

struct usbpd_info {
	msg_header_type msg_header;
	data_obj_type data_obj[7];

	usbpd_msg_state_t msg_state;
	usbpd_attach_mode_t attach_mode;
	int rp_currentlvl;
	int is_host;
	int is_client;
	int is_pr_swap;
	int power_role;
	int data_role;
	int request_power_number;
	int Vendor_ID;
	int Device_Version;
	unsigned int SVID_0;
	unsigned int SVID_1;
	unsigned int Product_ID;
	bool plug_attached;
	bool pd_charger_attached;
	bool dp_is_connect;
	bool dp_hs_connect;
	bool is_src;
	bool is_sent_pin_configuration;
	bool is_samsung_accessory_enter_mode;

	int do_power_nego;

	unsigned int vdm_msg;
	unsigned int pin_assignment;
	unsigned int dp_selected_pin;
	unsigned int hpd;
	unsigned int hpdirq;
	unsigned int acc_type;

	int try_state_change;
	bool vconn_en;
};

/**
 * struct usbpd_ops - usbpd operations.
 *
 *
 * This struct describes usbpd operations which can be implemented by
 * usbpd IC drivers.
 */
struct usbpd_ops {
	int (*usbpd_select_pdo) (struct usbpd_dev *, unsigned int selector);

	int (*usbpd_process_water) (struct usbpd_dev *);
	void (*usbpd_set_upsm) (struct usbpd_dev *);
	int (*usbpd_set_rprd_mode) (struct usbpd_dev *, unsigned int);
	int (*usbpd_set_dp_pin) (struct usbpd_dev *, unsigned int);

	int (*usbpd_control_option) (struct usbpd_dev *, unsigned int);
	int (*usbpd_pd_next_state) (struct usbpd_dev *);
	void (*usbpd_clear_discover_mode) (struct usbpd_dev *);
	int (*usbpd_dp_pin_assignment) (struct usbpd_dev *, unsigned int);
	int (*usbpd_select_svid) (struct usbpd_dev *, unsigned int);
	void (*usbpd_process_pd_dr_swap) (struct usbpd_dev *);
	void (*usbpd_process_pd_pr_swap) (struct usbpd_dev *);
	void (*usbpd_process_pd_src_cap) (struct usbpd_dev *);
	void (*usbpd_process_pd_get_snk_cap) (struct usbpd_dev *);
	int (*usbpd_process_cc_attach) (struct usbpd_dev *, usbpd_attach_mode_t);
	int (*usbpd_process_alternate_mode) (struct usbpd_dev *, usbpd_msg_state_t);
	int (*usbpd_process_check_accessory) (struct usbpd_dev *);
	int (*usbpd_process_discover_identity) (struct usbpd_dev *);
	int (*usbpd_process_discover_svids) (struct usbpd_dev *);
	int (*usbpd_process_discover_modes) (struct usbpd_dev *);
	int (*usbpd_process_enter_mode) (struct usbpd_dev *);
	int (*usbpd_process_dp_status_update) (struct usbpd_dev *);
	int (*usbpd_process_dp_configure) (struct usbpd_dev *);
	int (*usbpd_process_attention) (struct usbpd_dev *);
	int (*usbpd_process_uvdm_message) (struct usbpd_dev *);
	void (*usbpd_disable_irq) (struct usbpd_dev *);
	void (*usbpd_enable_irq) (struct usbpd_dev *);
};

/**
 * struct usbpd_desc - information for usbpd control
 * @name: Identifying name for the usbpd.
 * @ops: Usbpd operations table.
 */
struct usbpd_desc {
	const char *name;
	const struct usbpd_ops *ops;
	struct usbpd_info pd_info;
};

/*
 * struct usbpd_dev
 *
 * This should *not* be used directly by anything except the usbpd-core
 */
struct usbpd_dev {
	struct usbpd_desc *desc;
	void *drv_data;
	struct device dev;
	struct dual_role_phy_instance *dual_role;
	struct completion reverse_completion;
	struct delayed_work role_swap_work;
	struct usbpd_init_data *init_data;
	struct pdic_notifier_data pd_noti;
	struct notifier_block nb;
#if defined(CONFIG_IFCONN_NOTIFIER)
	struct ifconn_notifier_template ifconn_template;
#endif

	struct mutex mutex;
};

#define _STR(x) #x

#if defined(CONFIG_IFCONN_NOTIFIER)
#define USBPD_SEND_DNOTI(listener, id, event, udata)	\
{	\
	int ret;	\
	ret = ifconn_notifier_notify( \
					IFCONN_NOTIFY_CCIC,	\
					listener,	\
					IFCONN_NOTIFY_ID_##id,	\
					event,	\
					IFCONN_NOTIFY_PARAM_DATA,	\
					udata);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti\n", \
				__func__);	\
	}	\
}

#define USBPD_SEND_NOTI(listener, id, event, udata)	\
{	\
	int ret;	\
	struct ifconn_notifier_template template;	\
	template.dest = listener;	\
	template.data = (void *)udata;	\
	ret = ifconn_notifier_notify( \
					IFCONN_NOTIFY_CCIC,	\
					IFCONN_NOTIFY_MANAGER,	\
					IFCONN_NOTIFY_ID_##id,	\
					event,	\
					IFCONN_NOTIFY_PARAM_DATA,	\
					&template);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti\n", \
				__func__);	\
	}	\
}

#define USBPD_SEND_DATA_NOTI(listener, id, event, udata) \
{	\
	int ret;	\
	ret = ifconn_notifier_notify( \
					IFCONN_NOTIFY_PDIC,	\
					IFCONN_NOTIFY_ALL,	\
					IFCONN_NOTIFY_ID_##id,	\
					event,	\
					IFCONN_NOTIFY_PARAM_DATA,	\
					udata);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti\n", \
				__func__);	\
	}	\
}

#define USBPD_SEND_DATA_NOTI_DP(id, event, udata) \
{	\
	int ret;	\
	ret = ifconn_notifier_notify( \
					IFCONN_NOTIFY_PDIC,	\
					IFCONN_NOTIFY_MANAGER,	\
					IFCONN_NOTIFY_ID_##id,	\
					event,	\
					IFCONN_NOTIFY_PARAM_DATA,	\
					udata);	\
	if (ret < 0) {	\
		pr_err("%s: Fail to send noti\n", \
				__func__);	\
	}	\
}


#else
#define USBPD_SEND_NOTI(listener, id, event, udata)	\
{	\
}
#define USBPD_SEND_PD_NOTI(listener, id, event, udata)	\
{	\
}
#define USBPD_SEND_DNOTI(listener, id, event, udata)	\
{	\
}
#define USBPD_SEND_DATA_NOTI(listener, id, event, udata) \
{	\
}
#define USBPD_SEND_DATA_NOTI_DP(id, event, udata) \
{	\
}
typedef enum {
	IFCONN_NOTIFY_MANAGER,
	IFCONN_NOTIFY_USB,
	IFCONN_NOTIFY_BATTERY,
	IFCONN_NOTIFY_PDIC,
	IFCONN_NOTIFY_MUIC,
	IFCONN_NOTIFY_VBUS,
	IFCONN_NOTIFY_CCIC,
	IFCONN_NOTIFY_DP,
	IFCONN_NOTIFY_USB_DP,
	IFCONN_NOTIFY_ALL,
	IFCONN_NOTIFY_MAX,
} ifconn_notifier_t;

typedef enum {
	IFCONN_NOTIFY_LOW = 0,
	IFCONN_NOTIFY_HIGH,
	IFCONN_NOTIFY_IRQ,
} ifconn_notifier_dp_hpd_t;

typedef enum {
	IFCONN_NOTIFY_ID_INITIAL = 0,
	IFCONN_NOTIFY_ID_ATTACH,
	IFCONN_NOTIFY_ID_DETACH,
	IFCONN_NOTIFY_ID_RID,
	IFCONN_NOTIFY_ID_USB,
	IFCONN_NOTIFY_ID_POWER_STATUS,
	IFCONN_NOTIFY_ID_WATER,
	IFCONN_NOTIFY_ID_OTG,
	IFCONN_NOTIFY_ID_VCONN,
	IFCONN_NOTIFY_ID_DP_CONNECT,
	IFCONN_NOTIFY_ID_DP_HPD,
	IFCONN_NOTIFY_ID_DP_LINK_CONF,
	IFCONN_NOTIFY_ID_USB_DP,
	IFCONN_NOTIFY_ID_ROLE_SWAP,
	IFCONN_NOTIFY_ID_SELECT_PDO,
	IFCONN_NOTIFY_ID_VBUS,
} ifconn_notifier_id_t;

typedef enum {
	IFCONN_NOTIFY_DETACH = 0,
	IFCONN_NOTIFY_ATTACH,
} ifconn_notifier_attach_t;

typedef enum {
	IFCONN_NOTIFY_VBUS_UNKNOWN = 0,
	IFCONN_NOTIFY_VBUS_LOW,
	IFCONN_NOTIFY_VBUS_HIGH,
} ifconn_notifier_vbus_t;

typedef enum {
	IFCONN_NOTIFY_STATUS_NOT_READY = 0,
	IFCONN_NOTIFY_STATUS_NOT_READY_DETECT,
	IFCONN_NOTIFY_STATUS_READY,
} ifconn_notifier_vbus_stat_t;

typedef enum {
	IFCONN_NOTIFY_EVENT_DETACH = 0,
	IFCONN_NOTIFY_EVENT_ATTACH,
	IFCONN_NOTIFY_EVENT_USB_ATTACH_DFP, // Host
	IFCONN_NOTIFY_EVENT_USB_ATTACH_UFP, // Device
	IFCONN_NOTIFY_EVENT_USB_ATTACH_DRP, // Dual role
	IFCONN_NOTIFY_EVENT_USB_ATTACH_HPD, // DP : Hot Plugged Detect
	IFCONN_NOTIFY_EVENT_PD_SINK,
	IFCONN_NOTIFY_EVENT_PD_SOURCE,
	IFCONN_NOTIFY_EVENT_PD_SINK_CAP,
	IFCONN_NOTIFY_EVENT_DP_LOW,
	IFCONN_NOTIFY_EVENT_DP_HIGH,
	IFCONN_NOTIFY_EVENT_DP_IRQ,
	IFCONN_NOTIFY_EVENT_MUIC_LOGICALLY_DETACH,
	IFCONN_NOTIFY_EVENT_MUIC_LOGICALLY_ATTACH,
} ifconn_notifier_event_t;

typedef enum {
	IFCONN_NOTIFY_DP_PIN_UNKNOWN =0,
	IFCONN_NOTIFY_DP_PIN_A,
	IFCONN_NOTIFY_DP_PIN_B,
	IFCONN_NOTIFY_DP_PIN_C,
	IFCONN_NOTIFY_DP_PIN_D,
	IFCONN_NOTIFY_DP_PIN_E,
	IFCONN_NOTIFY_DP_PIN_F,
} ifconn_notifier_dp_pinconf_t;
#endif

struct usbpd_dev *
usbpd_register(struct device *dev, struct usbpd_desc *usbpd_desc, void *_data);
void usbpd_unregister(struct usbpd_dev *udev);
void *udev_get_drvdata(struct usbpd_dev *udev);

/* for usbpd manager */
void usbpd_process_pd(struct usbpd_dev *udev, usbpd_msg_state_t msg_state);
void usbpd_process_cc_water(struct usbpd_dev *udev, CCIC_WATER water_state);
int usbpd_process_cc_attach(struct usbpd_dev *udev, usbpd_attach_mode_t attach_mode);
int usbpd_process_alternate_mode(struct usbpd_dev *udev, usbpd_msg_state_t msg_state);
int usbpd_process_uvdm_message(struct usbpd_dev *udev);
void vbus_turn_on_ctrl(struct usbpd_dev *udev, bool enable);

#if defined(CONFIG_DUAL_ROLE_USB_INTF)
extern int dual_role_init(struct usbpd_dev *udev);
extern void role_swap_check(struct work_struct *work);
extern int dual_role_get_local_prop(struct dual_role_phy_instance *dual_role,
				    enum dual_role_property prop,
				    unsigned int *val);
extern int dual_role_set_prop(struct dual_role_phy_instance *dual_role,
			      enum dual_role_property prop,
			      const unsigned int *val);
extern int dual_role_is_writeable(struct dual_role_phy_instance *drp,
				  enum dual_role_property prop);
extern void dp_detach(void *data);
#endif
#endif
