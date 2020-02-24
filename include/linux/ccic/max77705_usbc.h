/*
 * Copyrights (C) 2017 Samsung Electronics, Inc.
 * Copyrights (C) 2017 Maxim Integrated Products, Inc.
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
 */

#ifndef __LINUX_MFD_MAX77705_UIC_H
#define __LINUX_MFD_MAX77705_UIC_H
#include <linux/ccic/max77705.h>
#include <linux/ccic/max77705_pd.h>
#include <linux/ccic/max77705_cc.h>
#include <linux/muic/muic.h>
#include <linux/muic/max77705-muic.h>
#include <linux/battery/battery_notifier.h>
#if defined(CONFIG_TYPEC)
#include <linux/usb/typec.h>
#endif

struct max77705_opcode {
	unsigned char opcode;
	unsigned char data[OPCODE_DATA_LENGTH];
	int read_length;
	int write_length;
};

typedef struct max77705_usbc_command_data {
	u8	opcode;
	u8  prev_opcode;
	u8	response;
	u8	read_data[OPCODE_DATA_LENGTH];
	u8	write_data[OPCODE_DATA_LENGTH];
	int read_length;
	int write_length;
	u8	reg;
	u8	val;
	u8	mask;
	u8  seq;
	int noti_cmd;
	u8	is_uvdm;
} usbc_cmd_data;

typedef struct max77705_usbc_command_node {
	usbc_cmd_data				cmd_data;
	struct max77705_usbc_command_node	*next;
} usbc_cmd_node;

typedef struct max77705_usbc_command_node	*usbc_cmd_node_p;

typedef struct max77705_usbc_command_queue {
	struct mutex			command_mutex;
	usbc_cmd_node			*front;
	usbc_cmd_node			*rear;
	usbc_cmd_node			tmp_cmd_node;
} usbc_cmd_queue_t;

#if defined(CONFIG_SEC_FACTORY)
#define FAC_ABNORMAL_REPEAT_STATE			12
#define FAC_ABNORMAL_REPEAT_RID				5
#define FAC_ABNORMAL_REPEAT_RID0			3
struct AP_REQ_GET_STATUS_Type {
	uint32_t FAC_Abnormal_Repeat_State;
	uint32_t FAC_Abnormal_Repeat_RID;
	uint32_t FAC_Abnormal_RID0;
};
#endif

struct max77705_usbc_platform_data {
	struct max77705_dev *max77705;
	struct device *dev;
	struct i2c_client *i2c; /*0xCC */
	struct i2c_client *muic; /*0x4A */
	struct i2c_client *charger; /*0x2A; Charger */

	int irq_base;

	/* interrupt pin */
	int irq_apcmd;
	int irq_sysmsg;

	/* VDM pin */
	int irq_vdm0;
	int irq_vdm1;
	int irq_vdm2;
	int irq_vdm3;
	int irq_vdm4;
	int irq_vdm5;
	int irq_vdm6;
	int irq_vdm7;

	int irq_vir0;

	/* register information */
	u8 usbc_status1;
	u8 usbc_status2;
	u8 bc_status;
	u8 cc_status0;
	u8 cc_status1;
	u8 pd_status0;
	u8 pd_status1;

	int watchdog_count;
	int por_count;

	u8 opcode_res;
	/* USBC System message interrupt */
	u8 sysmsg;
	u8 pd_msg;

	/* F/W state */
	u8 HW_Revision;
	u8 FW_Revision;
	u8 FW_Minor_Revision;
	u8 plug_attach_done;
	int op_code_done;
	enum max77705_connstat prev_connstat;
	enum max77705_connstat current_connstat;

	/* F/W opcode Thread */

	struct work_struct op_wait_work;
	struct work_struct op_send_work;
	struct work_struct cc_open_req_work;
	struct workqueue_struct	*op_wait_queue;
	struct workqueue_struct	*op_send_queue;
	struct completion op_completion;
	int op_code;
	int is_first_booting;
	usbc_cmd_data last_opcode;
	unsigned long opcode_stamp;
	struct mutex op_lock;

	/* F/W opcode command data */
	usbc_cmd_queue_t usbc_cmd_queue;

	uint32_t alternate_state;
	uint32_t acc_type;
	uint32_t Vendor_ID;
	uint32_t Product_ID;
	uint32_t Device_Version;
	uint32_t SVID_0;
	uint32_t SVID_1;
	struct delayed_work acc_detach_work;
	uint32_t dp_is_connect;
	uint32_t dp_hs_connect;
	uint32_t dp_selected_pin;
	u8 pin_assignment;
	uint32_t is_sent_pin_configuration;
	wait_queue_head_t host_turn_on_wait_q;
	wait_queue_head_t device_add_wait_q;
	int host_turn_on_event;
	int host_turn_on_wait_time;
	int device_add;
	int is_samsung_accessory_enter_mode;
	int send_enter_mode_req;
	u8 sbu[2];
	struct completion ccic_sysfs_completion;
//	u8 selftest;
//	struct completion is_selftest_done;

	struct max77705_muic_data *muic_data;
	struct max77705_pd_data *pd_data;
	struct max77705_cc_data *cc_data;
	struct pdic_notifier_struct *pd_noti;

	struct max77705_platform_data *max77705_data;
	struct wake_lock apcmd_wake_lock;
	struct wake_lock sysmsg_wake_lock;
#if defined(CONFIG_CCIC_NOTIFIER)
	struct workqueue_struct *ccic_wq;
	int manual_lpm_mode;
	int fac_water_enable;
	int cur_rid;
	int pd_state;
	u8  vconn_test;
	u8  vconn_en;
	u8  fw_update;
	int is_host;
	int is_client;
	bool auto_vbus_en;
	u8 cc_pin_status;
	int ccrp_state;
	int vsafe0v_status;
#endif
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
	struct dual_role_phy_instance *dual_role;
	struct dual_role_phy_desc *desc;
	struct completion reverse_completion;
	int power_role;
	int data_role;
	int try_state_change;
#elif defined(CONFIG_TYPEC)
	struct typec_port *port;
	struct typec_partner *partner;
	struct usb_pd_identity partner_identity;
	struct typec_capability typec_cap;
	struct completion typec_reverse_completion;
	int typec_power_role;
	int typec_data_role;
	int typec_try_state_change;
	int pwr_opmode;
#endif
	bool pd_support;
	struct delayed_work usb_external_notifier_register_work;
	struct notifier_block usb_external_notifier_nb;
	int mpsm_mode;
	bool mdm_block;
	int vbus_enable;
	int pd_pr_swap;
	int shut_down;
	struct delayed_work vbus_hard_reset_work;
	uint8_t ReadMSG[32];
	int ram_test_enable;
	int ram_test_retry;
	int ram_test_result;
	struct completion uvdm_longpacket_out_wait;
	struct completion uvdm_longpacket_in_wait;
	int is_in_first_sec_uvdm_req;
	int is_in_sec_uvdm_out;
	bool pn_flag;
	u8 uvdm_error;

#if defined(CONFIG_SEC_FACTORY)
	struct AP_REQ_GET_STATUS_Type factory_mode;
	struct delayed_work factory_state_work;
	struct delayed_work factory_rid_work;
#endif
	int detach_done_wait;
	int set_altmode;
	int set_altmode_error;

	u8 control3_reg;
	int cc_open_req;
};

/* Function Status from s2mm005 definition */
typedef enum {
	max77705_State_PE_Initial_detach	= 0,
	max77705_State_PE_SRC_Send_Capabilities = 3,
	max77705_State_PE_SNK_Wait_for_Capabilities = 17,
} max77705_pd_state_t;

typedef enum {
	MPSM_OFF = 0,
	MPSM_ON = 1,
} CCIC_DEVICE_MPSM;

#define DATA_ROLE_SWAP 1
#define POWER_ROLE_SWAP 2
#define VCONN_ROLE_SWAP 3
#define MANUAL_ROLE_SWAP 4

#define ROLE_ACCEPT			0x1
#define ROLE_REJECT			0x2
#define ROLE_BUSY			0x3

int max77705_pd_init(struct max77705_usbc_platform_data *usbc_data);
int max77705_cc_init(struct max77705_usbc_platform_data *usbc_data);
int max77705_muic_init(struct max77705_usbc_platform_data *usbc_data);
int max77705_i2c_opcode_read(struct max77705_usbc_platform_data *usbc_data,
		u8 opcode, u8 length, u8 *values);

void init_usbc_cmd_data(usbc_cmd_data *cmd_data);
void max77705_usbc_clear_queue(struct max77705_usbc_platform_data *usbc_data);
void max77705_usbc_opcode_rw(struct max77705_usbc_platform_data *usbc_data,
	usbc_cmd_data *opcode_r, usbc_cmd_data *opcode_w);
void max77705_usbc_opcode_write(struct max77705_usbc_platform_data *usbc_data,
	usbc_cmd_data *write_op);
void max77705_usbc_opcode_read(struct max77705_usbc_platform_data *usbc_data,
	usbc_cmd_data *read_op);
void max77705_usbc_opcode_push(struct max77705_usbc_platform_data *usbc_data,
	usbc_cmd_data *read_op);
void max77705_usbc_opcode_update(struct max77705_usbc_platform_data *usbc_data,
	usbc_cmd_data *read_op);

void max77705_ccic_event_work(void *data, int dest, int id,
		int attach, int event, int sub);
void max77705_notify_dr_status(struct max77705_usbc_platform_data *usbpd_data,
		uint8_t attach);
void max77705_pdo_list(struct max77705_usbc_platform_data *usbc_data,
		unsigned char *data);
#if defined(CONFIG_PDIC_PD30)
int max77705_response_apdo_request(struct max77705_usbc_platform_data *usbc_data,
		unsigned char *data);
void max77705_response_set_pps(struct max77705_usbc_platform_data *usbc_data,
		unsigned char *data);
#endif
void max77705_current_pdo(struct max77705_usbc_platform_data *usbc_data,
		unsigned char *data);
void max77705_detach_pd(struct max77705_usbc_platform_data *usbc_data);
void max77705_notify_rp_current_level(struct max77705_usbc_platform_data *usbc_data);
extern void max77705_manual_jig_on(struct max77705_usbc_platform_data *usbpd_data, int mode);
extern void max77705_vbus_turn_on_ctrl(struct max77705_usbc_platform_data *usbc_data, bool enable, bool swaped);
extern void max77705_dp_detach(void *data);
void max77705_usbc_disable_auto_vbus(struct max77705_usbc_platform_data *usbc_data);
extern void max77705_set_host_turn_on_event(int mode);
extern void pdic_manual_ccopen_request(int is_on);
#if defined(CONFIG_TYPEC)
int max77705_get_pd_support(struct max77705_usbc_platform_data *usbc_data);
#endif

extern const uint8_t BOOT_FLASH_FW_PASS2[];
extern const uint8_t BOOT_FLASH_FW_PASS3[];
extern const uint8_t BOOT_FLASH_FW_PASS4[];
extern const uint8_t BOOT_FLASH_FW_PASS5[];

#if defined(CONFIG_SEC_FACTORY)
void factory_execute_monitor(int);
#endif

#define DEBUG_MAX77705
#ifdef DEBUG_MAX77705
#define msg_maxim(format, args...) \
		pr_info("max77705: %s: " format "\n", __func__, ## args)
#else
#define msg_maxim(format, args...)
#endif /* DEBUG_MAX77766*/
#endif
