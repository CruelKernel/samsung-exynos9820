/*
 * s2mm005.h - S2MM005 USBPD device driver header
 *
 * Copyright (C) 2015 Samsung Electronics
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
#ifndef __S2MM005_USBPD_H
#define __S2MM005_USBPD_H __FILE__

#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/usb_notify.h>
#include <linux/wakelock.h>

#include <linux/ccic/core.h>

#if defined(CONFIG_CCIC_NOTIFIER)
#include <linux/ccic/ccic_notifier.h>
#endif
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
#include <linux/usb/class-dual-role.h>
#endif

#define S2MM005_USBPD_PORT_NAME "s2mm005_usbpd_port"

#define AVAILABLE_VOLTAGE 12000
#define UNIT_FOR_VOLTAGE 50
#define UNIT_FOR_CURRENT 10

#define REG_I2C_SLV_CMD		0x10
#define REG_TX_SINK_CAPA_MSG    0x0220
#define REG_TX_REQUEST_MSG	0x0240
#define REG_RX_SRC_CAPA_MSG	0x0260

/* VMD Message Register I2C address by S.LSI */
#define REG_VDM_MSG_REQ			0x02C0
#define REG_SSM_MSG_READ		0x0340
#define REG_SSM_MSG_SEND		0x0360

#define REG_TX_DIS_ID_RESPONSE		0x0400
#define REG_TX_DIS_SVID_RESPONSE	0x0420
#define REG_TX_DIS_MODE_RESPONSE	0x0440
#define REG_TX_ENTER_MODE_RESPONSE	0x0460
#define REG_TX_EXIT_MODE_RESPONSE	0x0480
#define REG_TX_DIS_ATTENTION_RESPONSE	0x04A0

#define REG_RX_DIS_ID_CABLE		0x0500
#define REG_RX_DIS_ID			0x0520
#define REG_RX_DIS_SVID			0x0540
#define REG_RX_MODE			0x0560
#define REG_RX_ENTER_MODE		0x0580
#define REG_RX_EXIT_MODE		0x05A0
#define REG_RX_DIS_ATTENTION		0x05C0
#define REG_RX_DIS_DP_STATUS_UPDATE	0x0600
#define REG_RX_DIS_DP_CONFIGURE		0x0620

enum s2mm005_sram_reg {
	S2MM005_REG_HW_VER			= 0x00,
	S2MM005_REG_SW_VER			= 0x08,
	S2MM005_REG_I2C_CMD			= 0x10,
	S2MM005_REG_I2C_RESPONSE		= 0x14,
	S2MM005_REG_FUNC_STATE			= 0x20,
	S2MM005_REG_FLASH_STATE			= 0x24,
	S2MM005_REG_SYNC_STATUS			= 0x30,
	S2MM005_REG_CTRL_MSG_STATUS		= 0x34,
	S2MM005_REG_DATA_MSG_STATUS		= 0x38,
	S2MM005_REG_EXTENDED_MSG_STATUS		= 0x3C,
	S2MM005_REG_MSG_IRQ_STATUS		= 0x40,
	S2MM005_REG_VDM_MSG_IRQ_STATUS		= 0x44,
	S2MM005_REG_SSM_MSG_IRQ_STATUS		= 0x48,
	S2MM005_REG_AP_REQ_GET_STATUS		= 0x4C,
	S2MM005_REG_RID				= 0x50,
	S2MM005_REG_SSM_HW_PID			= 0x54,
	S2MM005_REG_SSM_HW_USE_MSG		= 0x58,
	S2MM005_REG_LP_STATE			= 0x60,
};

enum s2mm005_cmd_mode_sel {
	S2MM005_MODE_BLANK			= 0x00,
	S2MM005_MODE_INT_CLEAR,
	S2MM005_MODE_REGISTER,
	S2MM005_MODE_INTERFACE,

	S2MM005_MODE_SELECT_SLEEP		= 0x0F,
	S2MM005_FLASH_UPDATE			= 0x10,
	S2MM005_FLASH_EXIT			= 0x20,
	S2MM005_FLASH_CMD_ON			= 0x40,
	S2MM005_FLASH_ACCESS			= 0x42,
	S2MM005_FLASH_FULL_ERASE		= 0x44,
	S2MM005_FLASH_TRIM_UPDATE		= 0x4F,
};

enum s2mm005_mode_interface {
	S2MM005_HOST_CMD_ON			= 0x01,
	S2MM005_PD_NEXT_STATE,
	S2MM005_PDO_SELECT,
	S2MM005_CTRL_MSG,
	S2MM005_VDM_MSG,
	S2MM005_INTERNAL_POLICY,
	S2MM005_SVID_SELECT,
	S2MM005_SEL_MAX_VOLTAGE,

	S2MM005_PR_SWAP_REQ			= 0x10,
	S2MM005_DR_SWAP_REQ,
	S2MM005_CC_DETACH_REQ,
	S2MM005_CC_SOURCE_REQ,
	S2MM005_CC_SINK_REQ,
	S2MM005_CC_DRP_REQ,

	S2MM005_SSM_UNSTRUCTURED_MSG_REQ	= 0x20,
	S2MM005_SSM_EID_MSG_REQ,
	S2MM005_SSM_RID_MSG_REQ,
	S2MM005_SSM_KID_MSG_REQ,
	S2MM005_SSM_ACC_MSG_REQ,
	S2MM005_SSM_USER_MSG_REQ,

	S2MM005_DP_ALT_MODE_REQ			= 0x30,
	S2MM005_SEL_START_ALT_MODE_REQ,
	S2MM005_SEL_CONTINUE_ALT_REQ,
	S2MM005_SEL_STOP_ALT_MODE_REQ,

	S2MM005_SEL_DPM_UPSM_ON			= 0x40,

	S2MM005_FAC_VCONN_ON_REQ		= 0x81,
	S2MM005_FAC_VCONN_OFF_REQ,
	S2MM005_FAC_WATER_ON_REQ,
	S2MM005_FAC_WATER_OFF_REQ,
	S2MM005_SEL_FAC_SBU_OPEN_ON,
	S2MM005_SEL_FAC_SBU_OPEN_OFF,
};

enum s2mm005_mode_select_sleep {
	S2MM005_SLEEP_NORMAL			= 0x01,
	S2MM005_SLEEP_DEEP,
	S2MM005_WAKEUP,
	S2MM005_JIGON_LOW,
	S2MM005_JIGON_HIGH,
	S2MM005_AUTO_LP_ENABLE,
	S2MM005_AUTO_LP_DISABLE,
	S2MM005_SLEEP_CTRL_NORMAL_NO_WATER_WAKEUP,
	S2MM005_SLEEP_CTRL_NORMAL_WATER_WAKEUP_NO_I2C_WAKEUP,
	S2MM005_SLEEP_CTRL_NORMAL_NO_WATER_WAKEUP_NO_I2C_WAKEUP,
};

enum {
	RID_UNDEFINED = 0,
	RID_56K,
	RID_130K,
	RID_240K,
	RID_390K,
	RID_560K,
	RID_910K,
	RID_OPEN,
};

#define CCIC_FW_VERSION_INVALID -1

/******************************************************************************/
/* definitions & structures                                                   */
/******************************************************************************/
#define USBPD005_DEV_NAME  "usbpd-s2mm005"

/*
******************************************************************************
* @file    EXT_SRAM.h
* @author  Power Device Team
* @version V1.0.0
* @date    2016.03.28
* @brief   Header for EXT SRAM map
******************************************************************************
*/


#define S2MM005_REG_MASK(reg, mask)	((reg & mask##_MASK) >> mask##_SHIFT)

#if defined(CONFIG_CCIC_NOTIFIER)
struct ccic_state_work {
	struct work_struct ccic_work;
	int dest;
	int id;
	int attach;
	int event;
	int sub;
};
#endif

struct s2mm005_data {
	struct device *dev;
	struct device *ccic_device;
	struct i2c_client *i2c;
	struct usbpd_dev *udev;
#if defined(CONFIG_CCIC_NOTIFIER)
	struct workqueue_struct *ccic_wq;
#endif
	int irq;
	int irq_gpio;
	int redriver_en;
	int s2mm005_om;
	int s2mm005_sda;
	int s2mm005_scl;
	u32 hw_rev;
	struct mutex i2c_mutex;
	u8 attach;
	u8 vbus_detach;
	struct wake_lock wlock;

	int wq_times;
	int p_prev_rid;
	int prev_rid;
	int cur_rid;
	int water_det;
	int run_dry;
	int booting_run_dry;

	u8 firm_ver[4];

	int pd_state;
	uint32_t func_state;

	int is_attached;
	int is_dr_swap;
	int is_pr_swap;

	int plug_rprd_sel;
	uint32_t data_role;
#if defined(CONFIG_USBPD_ALTERNATE_MODE)
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
	int host_turn_on_event;
	int host_turn_on_wait_time;
	int is_samsung_accessory_enter_mode;
#endif
	int manual_lpm_mode;
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
	struct dual_role_phy_instance *dual_role;
	struct dual_role_phy_desc *desc;
	struct completion reverse_completion;
	int power_role;
	int try_state_change;
	struct delayed_work role_swap_work;
#endif

	u8 fw_product_num;

#if defined(CONFIG_SEC_FACTORY)
	int fac_water_enable;
#endif
	struct delayed_work ccic_init_work;
	int ccic_check_at_booting;
	int cnt;

};

typedef enum i2c_slv_cmd {
	SEL_WRITE_BYTE = 0x01,
	SEL_WRITE_WORD = 0x02,
	SEL_WRITE_LONG = 0x04,

	SEL_READ_BYTE = 0x10,
	SEL_READ_WORD = 0x20,
	SEL_READ_LONG = 0x40,
} i2c_slv_cmd_t;

extern void s2mm005_int_clear(struct s2mm005_data *usbpd_data);
extern int s2mm005_read_byte(const struct i2c_client *i2c, u16 reg, u8 *val, u16 size);
extern int s2mm005_write_byte(const struct i2c_client *i2c, u16 reg, u8 *val, u16 size);
extern void s2mm005_usbpd_ops_register(struct usbpd_desc *desc);
extern void s2mm005_system_reset(struct s2mm005_data *usbpd_data);
extern void s2mm005_sel_write(struct i2c_client *i2c, u16 reg,
				u8 *buf, i2c_slv_cmd_t cmd);
extern void s2mm005_sel_read(struct i2c_client *i2c, u16 reg,
				u8 *buf, i2c_slv_cmd_t cmd);
#endif /* __S2MM005_H */
