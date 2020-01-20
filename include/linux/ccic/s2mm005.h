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
#ifndef __S2MM005_H
#define __S2MM005_H __FILE__

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
#include <linux/wakelock.h>

#if defined(CONFIG_CCIC_NOTIFIER)
#include <linux/ccic/ccic_notifier.h>
#endif
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
#include <linux/usb/class-dual-role.h>
#endif

#define REG_I2C_SLV_CMD		0x10
#define REG_TX_SINK_CAPA_MSG    0x0220
#define REG_TX_REQUEST_MSG	0x0240
#define REG_RX_SRC_CAPA_MSG	0x0260

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
/* Define to prevent recursive inclusion -------------------------------------*/
typedef union
{
	uint32_t DATA;
	struct
	{
		uint32_t Message_Type:4,
			Rsvd2_msg_header:1,
			Port_Data_Role:1,
			Specification_Revision:2,
			Port_Power_Role:1,
			Message_ID:3,
			Number_of_obj:3,
			Rsvd_msg_header:1,
			Reserved:16;
	} BITS;
} MSG_HEADER_Typedef;

typedef union
{
	uint32_t DATA;
	uint8_t  BYTES[4];
	struct
	{
		uint32_t Maximum_Current:10,
			Voltage_Unit:10,
			Peak_Current:2,
			Reserved:3,
			Data_Role_Swap:1,
			USB_Comm_Capable:1,
			Externally_POW:1,
			Higher_Capability:1,
			Dual_Role_Power:1,
			PDO_Parameter:2;
	} BITS;
} SINK_FIXED_SUPPLY_Typedef;

typedef union
{
	uint32_t DATA;
	uint8_t BYTES[4];
	struct {
		uint32_t Operational_Current:10,
		Minimum_Voltage:10,
		Maximum_Voltage:10,
		PDO_Parameter:2;
	} BITS;
} SINK_VAR_SUPPLY_Typedef;

typedef union
{
	MSG_HEADER_Typedef MSG_HEADER;
	SINK_FIXED_SUPPLY_Typedef MSG_FIXED_SUPPLY;
	SINK_VAR_SUPPLY_Typedef MSG_VAR_SUPPLY;
}SINK_CAPABILITY_MESSAGE_Typedef;

typedef union
{
    uint32_t DATA;
    struct {
        uint32_t   Reserved:32;
	}BITS;
} EXT_SRAM_Reserved_Type;


typedef union
{
	uint32_t        DATA;
    struct {
        uint32_t HW_VERSION_Reserved:32;
	}BITS;
} HW_VERSION_Type;

typedef union
{
	uint32_t        DATA;
    struct {
        uint32_t SW_VERSION_Reserved:32;
	}BITS;
} SW_VERSION_Type;

typedef union
{
	uint32_t        DATA;
    struct {
        uint32_t    CMD_MODE_SEL:8,
                    CMD_HOST:8,
                    CMD_WR_DATA1:8,
                    CMD_WR_DATA2:8;
	}BITS;
} I2C_SLV_CMD_Type;

typedef union
{
	uint32_t        DATA;
    struct {
        uint32_t    RSP_WORD1:16,
                    RSP_WORD2:16;
    } WORDS;
    struct {
        uint32_t    RSP_BYTE1:8,
                    RSP_BYTE2:8,
                    RSP_BYTE3:8,
                    RSP_BYTE4:8;
	}BYTES;
} I2C_SLV_RSP_Type;

typedef union
{
	uint32_t	DATA;
	uint8_t	BYTE[4];
	struct {
        uint32_t    PD_State:8,
                    RSP_BYTE1:8,
                    PD_Next_State:8,
                    RSP_BYTE2:8;
	}BYTES;
    struct {
        uint32_t    PD_State:8,
                    CC1_PLUG_STATE:3,
                    RSP_BYTE1:1,
                    CC2_PLUG_STATE:3,
                    RSP_BYTE2:1,
                    PD_Next_State:8,
                    ATTACH_DONE:1,
                    IS_SOURCE:1,
                    IS_DFP:1,
                    RP_CurrentLvl:2,
                    VBUS_CC_Short:1,
                    RSP_BYTE3:1,
                    RESET:1;
	}BITS;
} FUNC_STATE_Type;

typedef union
{
	uint32_t	DATA;
	uint8_t	BYTE[4];
	struct {
        uint32_t    AUTO_LP_ENABLE_BIT:1,
                    LOW_POWER_BIT:1,
                    Force_LP_BIT:1,
                    WATER_DET:1,
                    SW_JIGON:1,
                    RUN_DRY:1,
                    removing_charge_by_sbu_low:1,
                    BOOTING_RUN_DRY:1,
			Sleep_Cable_Detect:1,
			RSP_BYTE:23;
	} BITS;
} LP_STATE_Type;

typedef union
{
	uint32_t        DATA;
	uint8_t	BYTE[4];
    struct {
        uint32_t    Flash_State:8,
                    Reserved:24;
	}BITS;
} FLASH_STATE_Type;


typedef union
{
    uint32_t DATA;
    uint8_t  BYTES[4];
    struct {
        uint32_t    SOP2p_Debug:1,
                    SOPp_Debug:1,
                    SOP2p:1,
                    SOPp:1,
                    SOP:1,
                    RSVD_1:27;
  } BITS;
} SYNC_STATUS_Type;

typedef union
{
    uint32_t DATA;
    uint8_t  BYTES[4];
    struct {
        uint32_t    FR_Swap:1,
                    Get_Source_Status:1,
                    Get_Source_Extended:1,
                    Not_Supported:1,
                    Soft_Reset:1,
                    Wait:1,
                    VCONN_Swap:1,
                    PR_Swap:1,
                    DR_Swap:1,
                    Get_Sink_Cap:1,
                    Get_Source_Cap:1,
                    PS_RDY:1,
                    Ping:1,
                    Reject:1,
                    Accept:1,
                    GotoMin:1,
                    GoodCRC:1,
                    RSVD_1:15;
  } BITS;
} CTRL_MSG_STATUS_Type;

typedef union
{
    uint32_t DATA;
    uint8_t  BYTES[4];
    struct {
        uint32_t    Attention:1,
                    Exit_Mode:1,
                    Enter_Mode:1,
                    Discover_Modes:1,
                    Discover_SVIDs:1,
                    Discover_Identity:1,
                    Source_Alert:1,
                    Battery_Status:1,
                    Vendor_Defined:1,
                    BIST_TEST_Data:1,
                    BIST_Carrier_Mode2:1,
                    Request:1,
                    Sink_Capability:1,
                    Source_Capability:1,
                    RSVD_1:18;
  } BITS;
} DATA_MSG_STATUS_Type;

typedef union
{
    uint32_t DATA;
    uint8_t  BYTES[4];
    struct {
        uint32_t    FW_UPDATE_Response:1,
                    FW_UPDATA_Request:1,
                    Security_Response:1,
                    Security_Request:1,
                    Manufacturer_Info:1,
                    Battery_Capabilities:1,
                    Get_Battery_Status:1,
                    Get_Battery_Cap:1,
                    Source_Status:1,
                    Source_Cap_Extended:1,
                    RSVD_1:21;
  } BITS;
} EXTENDED_MSG_STATUS_Type;


typedef union
{
    uint32_t DATA;
    uint8_t  BYTES[4];
    struct {
        uint32_t    Ctrl_Flag_Reserved_b0:1,        // b0
                    Ctrl_Flag_GoodCRC:1,            // b1
                    Ctrl_Flag_GotoMin:1,            // b2
                    Ctrl_Flag_Accept:1,             // b3
                    Ctrl_Flag_Reject:1,             // b4
                    Ctrl_Flag_Ping:1,               // b5
                    Ctrl_Flag_PS_RDY:1,             // b6
                    Ctrl_Flag_Get_Source_Cap:1,     // b7
                    Ctrl_Flag_Get_Sink_Cap:1,       // b8
                    Ctrl_Flag_DR_Swap:1,            // b9
                    Ctrl_Flag_PR_Swap:1,            // b10
                    Ctrl_Flag_VCONN_Swap:1,         // b11
                    Ctrl_Flag_Wait:1,               // b12
                    Ctrl_Flag_Soft_Reset:1,         // b13
                    Ctrl_Flag_Reserved_b14:1,       // b14
                    Ctrl_Flag_Reserved_b15:1,       // b15
                    Data_Flag_Reserved_b16:1,       // b16
                    Data_Flag_SRC_Capability:1,     // b17
                    Data_Flag_Request:1,            // b18
                    Data_Flag_BIST:1,               // b19
                    Data_Flag_SNK_Capability:1,     // b20
                    Data_Flag_Reserved_05:1,        // b21
                    Data_Flag_Reserved_06:1,        // b22
                    Data_Flag_Reserved_07:1,        // b23
                    Data_Flag_Reserved_08:1,        // b24
                    Data_Flag_Reserved_09:1,        // b25
                    Data_Flag_Reserved_10:1,        // b26
                    Data_Flag_Reserved_11:1,        // b27
                    Data_Flag_Reserved_12:1,        // b28
                    Data_Flag_Reserved_13:1,        // b29
                    Data_Flag_Reserved_14:1,        // b30
                    Data_Flag_Vender_Defined:1;     // b31

    }BITS;
} MSG_IRQ_STATUS_Type;


typedef union
{
	uint32_t DATA;
	uint8_t  BYTES[4];
	struct {
	    uint32_t	Vdm_Flag_Reserve_b0:1,		// b0
			Vdm_Flag_Discover_ID:1,		// b1
			Vdm_Flag_Discover_SVIDs:1,	// b2
			Vdm_Flag_Discover_MODEs:1,	// b3
			Vdm_Flag_Enter_Mode:1,		// b4
			Vdm_Flag_Exit_Mode:1,		// b5
			Vdm_Flag_Attention:1,		// b6
			Vdm_Flag_Reserved:9,		// b7 - b15
			Vdm_Flag_DP_Status_Update:1,	// b16
			Vdm_Flag_DP_Configure:1,	// b17
			Vdm_Flag_Reserved2:14;		// b18 - b31
	}BITS;
} VDM_MSG_IRQ_STATUS_Type;

typedef union
{
    uint32_t DATA;
    uint8_t  BYTES[4];
    struct {
        uint32_t    AP_Req_Get:2,                   // b0-b1
                    UPSM_By_I2C:1,                  // b2
                    Reserved:1,                     // b3
                    Is_HardReset:1,                 // b4
                    FAC_Abnormal_Repeat_State:1,    // b5
                    FAC_Abnormal_Repeat_RID:1,      // b6
                    FAC_Abnormal_RID0:1,            // b7
                    SBU1_CNT:8,                     // b8 - b15
                    SBU2_CNT:8,                     // b16 - b23
                    SBU_LOW_CNT:4,                  // b24 - b27
                    Alt_Mode_By_I2C:2,              // b28 - b29
                    AP_Req_Reserved_H:1,            // b30
                    Func_Abnormal_State:1;          // b31
  } BITS;
} AP_REQ_GET_STATUS_Type;

typedef union
{
    uint32_t DATA;
    uint8_t  BYTES[4];
    struct {
        uint32_t    Ssm_Flag_Reserve_b0:1,		// b0
                    Ssm_Flag_Identification:1,		// b1
                    Ssm_Flag_RandomNumber:1,		// b2
                    Ssm_Flag_Encrypted_Data:1,		// b3
                    Ssm_Flag_Unstructured_Data:1,	// b4
                    Ssm_Flag_Reserved:26,		// b5 - b30
                    Ssm_Flag_AES_Done:1;
  } BITS;
} SSM_MSG_IRQ_STATUS_Type;

typedef union
    {
    uint32_t DATA;
    uint8_t  BYTES[4];
    struct {
        uint32_t    RID:8,
                    KID:8,
                    ACC_CMD:8,
                    Reserved:8;
  } BITS;
} SSM_HW_ID_VALUE_Type;

typedef union
    {
    uint32_t DATA;
    uint8_t  BYTES[4];
    struct {
        uint32_t    Product_ID:16,
                    Reserved:16;
    }BITS;
} SSM_HW_PID_VALUE_Type;

typedef union
{
    uint32_t DATA;
    uint8_t  BYTES[4];
    struct {
        uint8_t Use_CMD1   : 8,
                Use_CMD2   : 8,
                Use_Data1  : 8,
                Use_Data2  : 8;
    }BITS;
} SSM_HW_USE_MSG_Type;

typedef struct
{
    HW_VERSION_Type		HW_VERSION_1;		// 0x0000h
    HW_VERSION_Type		HW_VERSION_2;		// 0x0004h
    SW_VERSION_Type		SW_VERSION_1;		// 0x0008h
    SW_VERSION_Type		SW_VERSION_2;		// 0x000Ch
    I2C_SLV_CMD_Type		I2C_SLV_CMD;		// 0x0010h
    I2C_SLV_RSP_Type		I2C_SLV_RSP;		// 0x0014h
    EXT_SRAM_Reserved_Type	Reserved_18h;		// 0x0018h
    EXT_SRAM_Reserved_Type	Reserved_1Ch;		// 0x001Ch
    FUNC_STATE_Type		FUNC_STATE;		// 0x0020h
    FLASH_STATE_Type		FLASH_STATE;		// 0x0024h
    EXT_SRAM_Reserved_Type	Reserved_28h;		// 0x0028h
    EXT_SRAM_Reserved_Type	Reserved_2Ch;		// 0x002Ch
    SYNC_STATUS_Type		SYNC_STATUS;		// 0x0030h
    CTRL_MSG_STATUS_Type	CTRL_MSG_STATUS;	// 0x0034h
    DATA_MSG_STATUS_Type	DATA_MSG_STATUS;	// 0x0038h
    EXTENDED_MSG_STATUS_Type	EXTENDED_MSG_STATUS;	// 0x003Ch
    MSG_IRQ_STATUS_Type		MSG_IRQ_STATUS;		// 0x0040h
    VDM_MSG_IRQ_STATUS_Type	VDM_MSG_IRQ_STATUS;	// 0x0044h
    SSM_MSG_IRQ_STATUS_Type	SSM_MSG_IRQ_STATUS;	// 0x0048h
    AP_REQ_GET_STATUS_Type      AP_REQ_GET_STATUS;      // 0x004Ch
    SSM_HW_ID_VALUE_Type	SSM_HW_ID_VALUE;	// 0x0050h
    SSM_HW_PID_VALUE_Type	SSM_HW_PID_VALUE;	// 0x0054h
    SSM_HW_USE_MSG_Type		SSM_HW_USE_MSG;		// 0x0058h
} EXT_SRAM_Type;

typedef struct
{
    uint32_t	SW_Version;
    uint32_t	SW_Start_Addr;
    uint32_t	SW_End_Addr;
    uint32_t	SW_CRC_VALUE;
} SW_IMAGE_HEADER_Type;


////////////////////////////////////////////////////////////////////////////////////////////
/*********************************** SAMSUNG Mobile Add ************************************/
////////////////////////////////////////////////////////////////////////////////////////////

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

// Function Status (0x20)
typedef enum {
	State_PE_Initial_detach			= 0,

	// source Port
	State_PE_SRC_Startup			= 1,
	State_PE_SRC_Discovery			= 2,
	State_PE_SRC_Send_Capabilities		= 3,
	State_PE_SRC_Negotiate_Capability	= 4,
	State_PE_SRC_Transition_Supply		= 5,
	State_PE_SRC_Ready			= 6,
	State_PE_SRC_Disabled			= 7,
	State_PE_SRC_Capability_Response	= 8,
	State_PE_SRC_Hard_Rest			= 9,
	State_PE_SRC_Hard_Rest_Received		= 10,
	State_PE_SRC_Transition_to_default	= 11,
	State_PE_SRC_Give_Source_Cap		= 12,
	State_PE_SRC_Get_Sink_Cap		= 13,
	State_PE_SRC_Wait_New_Capabilities	= 14,

	// Sink Port
	State_PE_SNK_Startup			= 15,
	State_PE_SNK_Discovery			= 16,
	State_PE_SNK_Wait_for_Capabilities	= 17,
	State_PE_SNK_Evaluate_Capability	= 18,
	State_PE_SNK_Select_Capability		= 19,
	State_PE_SNK_Transition_Sink		= 20,
	State_PE_SNK_Ready			= 21,
	State_PE_SNK_Hard_Reset			= 22,
	State_PE_SNK_Transition_to_default	= 23,
	State_PE_SNK_Give_Sink_Cap		= 24,
	State_PE_SNK_Get_Source_Cap		= 25,

	// Source Startup Structured VDM Discover Indentity
	State_PE_SRC_CABLE_VDM_Identity_Request	= 26,
	State_PE_SRC_CABLE_VDM_Identity_ACKed	= 27,
	State_PE_SRC_CABLE_VDM_Identity_NAKed	= 28,

	// Type-C referenced states
	State_ErrorRecovery			= 29,
	State_PE_PRS_SRC_SNK_Transition_to_off	= 52,
	State_PE_PRS_SNK_SRC_Source_on		= 64,
} function_status_t;

typedef enum
{
	Rp_None = 0,
	Rp_56K = 1,	/* 80uA */
	Rp_22K = 2,	/* 180uA */
	Rp_10K = 3,	/* 330uA */
	Rp_Abnormal = 4,
} CCIC_RP_CurrentLvl;

#define S2MM005_REG_MASK(reg, mask)	((reg & mask##_MASK) >> mask##_SHIFT)


struct s2mm005_data {
	struct device *dev;
	struct i2c_client *i2c;
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
#if defined(CONFIG_SEC_FACTORY)
	int fac_booting_dry_check;
#endif

	u8 firm_ver[4];

	int pd_state;
	uint32_t func_state;

	int is_host;
	int is_client;
	int is_dr_swap;
	int is_pr_swap;

	int plug_rprd_sel;
	uint32_t data_role;
#if defined(CONFIG_CCIC_ALTERNATE_MODE)
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

	int s2mm005_fw_product_id;
	u8 fw_product_id;

#if defined(CONFIG_SEC_FACTORY)
	int fac_water_enable;
#endif
	struct delayed_work ccic_init_work;
	int ccic_check_at_booting;
	struct delayed_work usb_external_notifier_register_work;
	struct notifier_block usb_external_notifier_nb;

};
#endif /* __S2MM005_H */
