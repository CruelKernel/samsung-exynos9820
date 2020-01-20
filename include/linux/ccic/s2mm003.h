/*
 * s2mm003.h - S2MM003 USBPD device driver header
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
#ifndef __S2MM003_H
#define __S2MM003_H __FILE__

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

#if defined(CONFIG_CCIC_NOTIFIER)
#include <linux/ccic/ccic_notifier.h>
#endif

#define AVAILABLE_VOLTAGE 12000
#define UNIT_FOR_VOLTAGE 50
#define UNIT_FOR_CURRENT 10
//#define CONFIG_SAMSUNG_S2MM003_SYSFS

/******************************************************************************/
/* definitions & structures                                                   */
/******************************************************************************/
#define USBPD_DEV_NAME  "usbpd-s2mm003"
#define FIRMWARE_PATH "usbpd/USB_PD_FULL_DRIVER.bin"

#define	OTP_ADD_LOW		0x00
#define	OTP_ADD_HIGH		0x01
#define	OTP_RW			0x02
#define	OTP_BCHK_TEST		0x03
#define	OTP_READ_DATA		0x04
#define	OM			0x05
#define	IF_S_CODE_E		0x05
#define	MON_EN			0x06
#define	BUS_IF_SELECT		0x07
#define	I2C_PIN_SELECT		0x08
#define	I2C_DEV_ADDR		0x09
#define	I2C_DEV16_ADDR		0x0A
#define	I2C_8BIT_SLV_RST	0x0B
#define	OTP_BLK_RST		0x0C
#define	USB_PD_RST		0x0D
#define	IRQ_WR_ADDR		0x0E
#define	IRQ_WR_DATA		0x0F
#define	IRQ_RD_ADDR		0x10
#define	IRQ_RD_DATA		0x11
#define	A_TEST_MUX_SEL		0x12
#define	MON0_SEL		0x13
#define	MON1_SEL		0x14
#define	ANALOG_00		0x15
#define	ANALOG_01		0x16
#define	ANALOG_02		0x17
#define	ANALOG_03		0x18
#define	ANALOG_04		0x19
#define	ANALOG_05		0x1A
#define	ANALOG_06		0x1B
#define	ANALOG_07		0x1C
#define	ANALOG_08		0x1D
#define	ANALOG_09		0x1E
#define	ANALOG_10		0x1F
#define	ANALOG_11		0x20
#define	EXT_VCONN_TIMER		0x21
#define	OCP_CC_TIMER		0x22
#define	SELECTED_EXT_NFET_EN	0x23
#define	REDRVEN1_CC_SEL		0x24
#define	OTP_DUMP_DISABLE	0x30
#define	I2C_SYSREG_SET		0x31
#define	I2C_SRAM_SET		0x32

#define	INTERRUPT_CLEAR	0xFF

#define	INTERRUPT_STATUS1	0x1830
#define	INTERRUPT_STATUS2	0x1834
#define	INTERRUPT_STATUS3	0x1838
#define	INTERRUPT_STATUS4	0x183C
#define	INTERRUPT_STATUS5	0x1840

#define	PLUG_STATE_MONITOR	0x1850
#define	RID_GET_MONITOR		0x1854

/* PLUG_STATE_MONITOR : 0x1850 */
#define	S_CC1_VALID_MASK		0x07
#define	S_CC1_VALID_SHIFT		0x00
#define	S_CC2_VALID_MASK		0x38
#define	S_CC2_VALID_SHIFT		0x03
#define	PLUG_RPRD_SEL_MONITOR_MASK	0x40
#define	PLUG_RPRD_SEL_MONITOR_SHIFT	0x06
#define	PLUG_ATTACH_DONE_MASK		0x80
#define	PLUG_ATTACH_DONE_SHIFT		0x07

/* From S.LSI */
#define IND_REG_HOST_CMD_ON	0x05
#define IND_REG_PD_FUNC_STATE   0x16
#define IND_REG_INT_NUMBER      0x17
#define IND_REG_INT_STATUS_3    0x1A
#define IND_REG_INT_STATUS_5    0x1C
#define IND_REG_READ_MSG_INDEX  0x4F
#define IND_REG_MSG_ACCESS_BASE 0x50
#define MSG_BUF_REQUEST_SIZE    0x8 // 1-Byte Length
#define MSG_REQUEST_SIZE        0x2 // 4-Byte Length

#define IND_REG_PDIC_MODE           0x13    // 2'b00:Factory   2'b01:DFP  2'b10:UFP  2'b11:DRP(Default)
#define IND_REG_PDIC_PLUG_STATE     0x15    // b7:Attach b6:RpRd_sel b5~b3:CC2(100:Ra 010:Rd 001:Rp)  b2~b0:CC1(100:Ra 010:Rd 001:Rp)
#define IND_REG_PDIC_PD_FUNC_STATE  0x16
#define IND_REG_PDIC_MAIN_INT_NUM   0x17

#define IND_REG_PDIC_1_INT          0x18
#define IND_REG_PDIC_2_INT          0x19
#define IND_REG_PDIC_3_INT          0x1A
#define IND_REG_PDIC_4_INT          0x1B
#define IND_REG_PDIC_5_INT          0x1C

#define IND_REG_PDIC_RID            0x30

#define b_RESET_START   (1 << 7)
#define b_Reserved      (1 << 6)
#define b_PD_FUNC_FLAG  (1 << 5)
#define b_INT_5_FLAG    (1 << 4)
#define b_INT_4_FLAG    (1 << 3)
#define b_INT_3_FLAG    (1 << 2)
#define b_INT_2_FLAG    (1 << 1)
#define b_INT_1_FLAG    (1 << 0)

//  Interrupt Status1
#define b_Cmd_Intial_State			0
#define b_Cmd_Discover_Identity     0x1
#define b_Cmd_Discover_SVIDs        0x2
#define b_Cmd_Discover_Modes        0x4
#define b_Cmd_Enter_Mode            0x8
#define b_Cmd_Exit_Mode             0x10
#define b_Cmd_Attention             0x20
#define b_Msg_GoodCRC               0x40
#define b_Msg_Accept                0x80

// Interrupt Status2
#define b_Msg_DR_SWAP           0x40
#define b_Msg_PR_SWAP           0x80

// Interrupt Status3
#define b_Msg_VCONN_SWAP            0x1
#define b_Msg_Wait                  0x2
#define b_Msg_SRC_CAP               0x4
#define b_Msg_SNK_CAP               0x8
#define b_Msg_Request               0x10
#define b_msg_soft_reset            0x20
#define b_RID_Detect_done           0x40

// Function Status
#define FUNCTION_STATUS_INITIAL_DETACH		 0
#define FUNCTION_STATUS_SRC_SEND_CAPABILITY  3
#define FUNCTION_STATUS_SRC_READY            6
#define FUNCTION_STATUS_SRC_TIMEOUT          7
#define FUNCTION_STATUS_SINK_DISCOVERY	     17
#define FUNCTION_STATUS_SINK_READY           21
#define FUNCTION_STATUS_SINK_TIMEOUT		 29

typedef union
{
	uint16_t        DATA;
	struct
	{
		uint8_t     BDATA[2];
	}BYTES;
	struct
	{
		uint16_t    Message_Type:4,
				    Rsvd2_msg_header:1,
				    Port_Data_Role:1,
				    Specification_Revision:2,
				    Port_Power_Role:1,
			    	Message_ID:3,
				    Number_of_obj:3,
				    Rsvd_msg_header:1;
	}BITS;
} U_MSG_HEADER_Type;

/* for alternate mode */
#if defined(CONFIG_CCIC_ALTERNATE_MODE)
typedef union
{
	uint32_t        DATA;
	struct
	{
		uint8_t     BDATA[4];
	}BYTES;
	struct
	{
		uint32_t    VDM_command:5,
				    Rsvd2_VDM_header:1,
				    VDM_command_type:2,
				    Object_Position:3,
				    Rsvd_VDM_header:2,
			    	Structured_VDM_Version:2,
				    VDM_Type:1,
				    Standard_Vendor_ID:16;
	}BITS;
}U_DATA_MSG_VDM_HEADER_Type;

typedef union
{
	uint32_t        DATA;
	struct
	{
		uint8_t     BDATA[4];
	}BYTES;
	struct
	{
		uint32_t    USB_Vendor_ID:16,
			    	Rsvd_ID_header:10,
				    Modal_Operation_Supported:1,
				    Product_Type:3,
				    Data_Capable_USB_Device:1,
				    Data_Capable_USB_Host:1;
	}BITS;
}U_DATA_MSG_ID_HEADER_Type;

typedef union
{
	uint32_t        DATA;
	struct
	{
		uint8_t     BDATA[4];
	}BYTES;
	struct
	{
		uint32_t    Cert_TID:20,
			    	Rsvd_cert_VDOer:12;
	}BITS;
}U_CERT_STAT_VDO_Type;

typedef union
{
	uint32_t        DATA;
	struct
	{
		uint8_t     BDATA[4];
	}BYTES;
	struct
	{
		uint32_t    Device_Version:16,
			    	Product_ID:16;
	}BITS;
}U_PRODUCT_VDO_Type;

typedef union
{
	uint32_t        DATA;
	struct
	{
		uint8_t     BDATA[4];
	}BYTES;
	struct
	{
		uint32_t    USB_Superspeed_Signaling_Support:3,
				    SOP_contoller_present:1,
				    Vbus_through_cable:1,
				    Vbus_Current_Handling_Capability:2,
				    SSRX2_Directionality_Support:1,
				    SSRX1_Directionality_Support:1,
				    SSTX2_Directionality_Support:1,
				    SSTX1_Directionality_Support:1,
				    Cable_Termination_Type:2,
				    Cable_Latency:4,
				    TypeC_to_Plug_Receptacle:1,
				    TypeC_to_ABC:2,
				    Rsvd_CABLE_VDO:4,
				    Cable_Firmware_Version:4,
				    Cable_HW_Version:4;
	}BITS;
}U_CABLE_VDO_Type;

typedef union
{
	uint32_t        DATA;
	struct
	{
		uint8_t     BDATA[4];
	}BYTES;
	struct
	{
		uint32_t    SVID_0:16,
				    SVID_1:16;
	}BITS;
}U_VDO1_Type;
#endif
typedef enum
{
	// PDO Message
	MSG_Idx_TX_SRC_CAPA             = 1,
	MSG_Idx_TX_SINK_CAPA            = 2,
	MSG_Idx_TX_REQUEST              = 3,

	MSG_Idx_RX_SRC_CAPA             = 4,
	MSG_Idx_RX_SINK_CAPA            = 5,
	MSG_Idx_RX_REQUEST              = 6,

	// VDM User Message
	MSG_Idx_VDM_MSG_REQUEST         = 9,
	MSG_Idx_CTRL_MSG                = 10,

	// VDM TX Message
	MSG_Idx_TX_DISC_ID_RESP         = 17,
	MSG_Idx_TX_DISC_SVIDs_RESP      = 18,
	MSG_Idx_TX_DISC_MODE_RESP       = 19,
	MSG_Idx_TX_DISC_ENTER_MODE_RESP = 20,
	MSG_Idx_TX_DISC_EXIT_MODE_RESP  = 21,
	MSG_Idx_TX_DISC_ATTENTION_RESP  = 22,

	// VDM RX Message
	MSG_Idx_RX_DISC_ID_CABLE        = 25,
	MSG_Idx_RX_DISC_ID_RESP         = 26,
	MSG_Idx_RX_DISC_SVIDs_RESP      = 27,
	MSG_Idx_RX_DISC_MODE_RESP       = 28,
	MSG_Idx_RX_DISC_ENTER_MODE_RESP = 29,
	MSG_Idx_RX_DISC_EXIT_MODE_RESP  = 30,
	MSG_Idx_RX_DISC_ATTENTION_RESP  = 31

} Num_MSG_INDEX;

#if defined(CONFIG_DUAL_ROLE_USB_INTF)
typedef enum
{
	TYPE_C_DETACH = 0,
	TYPE_C_ATTACH_DFP = 1, // Host
	TYPE_C_ATTACH_UFP = 2, // Device
	TYPE_C_ATTACH_DRP = 3, // Dual role
} CCIC_OTP_MODE;
#endif

typedef struct
{
	uint16_t	Message_Type:4;
	uint16_t	Rsvd2_msg_header:1;
	uint16_t	Port_Data_Role:1;
	uint16_t	Specification_Revision:2;
	uint16_t	Port_Power_Role:1;
	uint16_t	Message_ID:3;
	uint16_t	Number_of_obj:3;
	uint16_t	Rsvd_msg_header:1;
} MSG_HEADER_Type;

typedef struct
{
	uint32_t    Maximum_Allow_Power:10;
	uint32_t    Minimum_Voltage:10;
	uint32_t    Maximum_Voltage:10;
	uint32_t    PDO_Parameter:2;
} SRC_BAT_SUPPLY_Typedef;

// ================== Capabilities Message ==============
// Source Capabilities
typedef struct
{
	uint32_t    Maximum_Current:10;
	uint32_t    Voltage_Unit:10;
	uint32_t    Peak_Current:2;
	uint32_t    Reserved:3;
	uint32_t    Data_Role_Swap:1;
	uint32_t    USB_Comm_Capable:1;
	uint32_t    Externally_POW:1;
	uint32_t    USB_Suspend_Support:1;
	uint32_t    Dual_Role_Power:1;
	uint32_t    PDO_Parameter:2;
}SRC_FIXED_SUPPLY_Typedef;

typedef struct
{
	uint32_t    Maximum_Current:10;
	uint32_t    Minimum_Voltage:10;
	uint32_t    Maximum_Voltage:10;
	uint32_t    PDO_Parameter:2;
}SRC_VAR_SUPPLY_Typedef;


typedef union
{
    uint16_t        DATA;

    struct
    {
        uint8_t     BDATA[2];
    }BYTES;

    struct
    {
        uint32_t    Maximum_Current:10,
                    Voltage_Unit:10,
                    Peak_Current:2,
                    Reserved:3,
                    Data_Role_Swap:1,
                    USB_Comm_Capable:1,
                    Externally_POW:1,
                    USB_Suspend_Support:1,
                    Dual_Role_Power:1,
                    PDO_Parameter:2;
    }BITS;
} U_SRC_FIXED_SUPPLY_Typedef;


typedef union
{
    uint16_t        DATA;

    struct
    {
        uint8_t     BDATA[2];
    }BYTES;

    struct
    {
        uint32_t    Maximum_Current:10,
                    Minimum_Voltage:10,
                    Maximum_Voltage:10,
                    PDO_Parameter:2;
    }BITS;
} U_SRC_VAR_SUPPLY_Typedef;


typedef union
{
    uint16_t        DATA;

    struct
    {
        uint8_t     BDATA[2];
    }BYTES;

    struct
    {
        uint32_t    Maximum_Allow_Power:10,
                    Minimum_Voltage:10,
                    Maximum_Voltage:10,
                    PDO_Parameter:2;
    }BITS;
} U_SRC_BAT_SUPPLY_Typedef;


    // Sink Capabilities
typedef union
{
    uint16_t        DATA;

    struct
    {
        uint8_t     BDATA[2];
    }BYTES;

    struct
    {
        uint32_t    Maximum_Current:10,
                    Voltage_Unit:10,
                    Peak_Current:2,
                    Reserved:3,
                    Data_Role_Swap:1,
                    USB_Comm_Capable:1,
                    Externally_POW:1,
                    Higher_Capability:1,
                    Dual_Role_Power:1,
                    PDO_Parameter:2;
    }BITS;
} U_SINK_FIXED_SUPPLY_Typedef;


typedef union
{
    uint16_t        DATA;

    struct
    {
        uint8_t     BDATA[2];
    }BYTES;

    struct
    {
        uint32_t    Operational_Current:10,
                    Minimum_Voltage:10,
                    Maximum_Voltage:10,
                    PDO_Parameter:2;
    }BITS;
} U_SINK_VAR_SUPPLY_Typedef;


typedef union
{
    uint16_t        DATA;

    struct
    {
        uint8_t     BDATA[2];
    }BYTES;

    struct
    {
        uint32_t    Operational_Power:10,
                    Minimum_Voltage:10,
                    Maximum_Voltage:10,
                    PDO_Parameter:2;
    }BITS;
} U_SINK_BAT_SUPPLY_Typedef;

// ================== Request Message ================

typedef struct
{
	uint32_t    Maximum_OP_Current:10;  // 10mA
	uint32_t    OP_Current:10;          // 10mA
	uint32_t    Reserved_1:4;           // Set to Zero
	uint32_t    No_USB_Suspend:1;
	uint32_t    USB_Comm_Capable:1;
	uint32_t    Capa_Mismatch:1;
	uint32_t    GiveBack_Flag:1;        // GiveBack Support set to 1
	uint32_t    Object_Position:3;      // 000 is reserved
	uint32_t    Reserved_2:1;
} REQUEST_FIXED_SUPPLY_STRUCT_Typedef;

typedef union
{
    uint32_t        DATA;

    struct
    {
        uint8_t     BDATA[4];
    }BYTES;

    struct
    {
        uint32_t    Minimum_OP_Current:10,  // 10mA
                    OP_Current:10,          // 10mA
                    Reserved_1:4,           // Set to Zero
                    No_USB_Suspend:1,
                    USB_Comm_Capable:1,
                    Capa_Mismatch:1,
                    GiveBack_Flag:1,        // GiveBack Support set to 1
                    Object_Position:3,      // 000 is reserved
                    Reserved_2:1;
    }BITS;
}U_REQUEST_FIXED_SUPPLY_STRUCT_Typedef;

#define S2MM003_REG_MASK(reg, mask)	((reg & mask##_MASK) >> mask##_SHIFT)


#if defined(CONFIG_SAMSUNG_S2MM003_SYSFS)
static ssize_t s2mm003_register_show(struct device *dev,
		struct device_attribute *att, char *buf);

static DEVICE_ATTR(s2mm003_register, S_IRUGO, s2mm003_register_show, NULL);
#endif

struct s2mm003_data {
	struct device *dev;
	struct i2c_client *i2c;
	int irq_gpio;
	int redriver_en;
	struct mutex i2c_mutex;
	u8 attach;
	u8 vbus_detach;

	int wq_times;
	int p_prev_rid;
	int prev_rid;
	int cur_rid;

	u8 firm_ver[4];

	int func_state;	

	int plug_rprd_sel;
	uint32_t Pdic_usb_state;
#if defined(CONFIG_CCIC_ALTERNATE_MODE)
	uint32_t Pdic_state_machine;
	uint32_t SVID_0;
	uint32_t SVID_1;
	uint32_t Vendor_ID;
	uint32_t Product_ID;
	int acc_type;
#endif
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
	struct dual_role_phy_instance *dual_role;
	struct dual_role_phy_desc *desc;
	struct completion reverse_completion;
	int power_role;
	int try_state_change;
#endif
};

#define DUAL_ROLE_SET_MODE_WAIT_MS 1500

enum S2MM003_POWER {
	S2MM003_CPU_RESET,
	S2MM003_FULL_RESET,
	S2MM003_START,
};

//int irq_times;
//int wq_times;
//int p_prev_rid = -1;
//int prev_rid = -1;
//int cur_rid = -1;
#endif /* __S2MM003_H */
