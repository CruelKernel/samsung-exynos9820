/*
 * include/linux/ccic/ccic_alternate.h
 *	- S2MM005 USB CCIC Alternate mode header file
 *
 * Copyright (C) 2016 Samsung Electronics
 * Author: Wookwang Lee <wookwang.lee@samsung.com>
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
#ifndef __LINUX_CCIC_ALTERNATE_MODE_H__
#define __LINUX_CCIC_ALTERNATE_MODE_H__

typedef union {
	uint32_t        DATA;
	struct {
		uint8_t     BDATA[4];
	} BYTES;
	struct {
		uint32_t	Number_of_obj:3,
				MSG_ID:3,
				Port_Power_Role:1,
				Specification_Rev:2,
				Port_Data_Role:1,
				Reserved:1,
				MSG_Type:4;
	} BITS;
} U_DATA_MSG_HEADER_Type;

typedef union {
	uint32_t        DATA;
	struct {
		uint8_t     BDATA[4];
	} BYTES;
	struct {
		uint32_t	VDM_command:5,
				Rsvd2_VDM_header:1,
				VDM_command_type:2,
				Object_Position:3,
				Rsvd_VDM_header:2,
				Structured_VDM_Version:2,
				VDM_Type:1,
				Standard_Vendor_ID:16;
	} BITS;
} U_DATA_MSG_VDM_HEADER_Type;

typedef union {
	uint32_t        DATA;
	struct {
		uint8_t     BDATA[4];
	} BYTES;
	struct {
		uint32_t	USB_Vendor_ID:16,
				Rsvd_ID_header:10,
				Modal_Operation_Supported:1,
				Product_Type:3,
				Data_Capable_USB_Device:1,
				Data_Capable_USB_Host:1;
	} BITS;
} U_DATA_MSG_ID_HEADER_Type;

typedef union {
	uint32_t	DATA;
	struct {
		uint8_t		BDATA[4];
	} BYTES;
	struct {
		uint32_t	Cert_TID:20,
				Rsvd_cert_VDOer:12;
	} BITS;
} U_CERT_STAT_VDO_Type;

typedef union {
	uint32_t        DATA;
	struct {
		uint8_t     BDATA[4];
	} BYTES;
	struct {
		uint32_t    Device_Version:16,
			    Product_ID:16;
	} BITS;
} U_PRODUCT_VDO_Type;

typedef union {
	uint32_t        DATA;
	struct {
		uint8_t     BDATA[4];
	} BYTES;
	struct {
		uint32_t	USB_Superspeed_Signaling_Support:3,
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
	} BITS;
} U_CABLE_VDO_Type;

typedef union {
	uint32_t        DATA;
	struct {
		uint8_t     BDATA[4];
	} BYTES;
	struct {
		uint32_t    SVID_1:16,
			    SVID_0:16;
	} BITS;
} U_VDO1_Type;

typedef union {
	uint32_t	DATA;
	struct {
		uint8_t	BDATA[4];
	} BYTES;
	struct {
		uint32_t	VENDOR_DEFINED_MESSAGE:15,
				VDM_TYPE:1,
				USB_Vendor_ID:16;
	} BITS;
} U_UNSTRUCTURED_VDM_HEADER_Type;

typedef union {
	uint32_t	DATA;
	struct {
		uint8_t	BDATA[4];
	} BYTES;
	struct {
		uint32_t	DATA:8,
				TOTAL_NUMBER_OF_UVDM_SET:4,
				RESERVED:1,
				COMMAND_TYPE:2,
				DATA_TYPE:1,
				PID:16;
	} BITS;
} U_SEC_UNSTRUCTURED_VDM_HEADER_Type;

typedef union {
	uint32_t	DATA;
	struct {
		uint8_t	BDATA[4];
	} BYTES;
	struct {
		uint32_t	VENDOR_DEFINED_MESSAGE:15,
				VDM_TYPE:1,
				USB_Vendor_ID:16;
	} BITS;
} U_SEC_DATA_HEADER_Type;

typedef union {
	uint32_t	DATA;
	struct {
		uint8_t	BDATA[4];
	} BYTES;
	struct {
		uint32_t	VENDOR_DEFINED_MESSAGE:15,
				VDM_TYPE:1,
				USB_Vendor_ID:16;
	} BITS;
} U_SEC_DATA_TAILER_Type;

typedef union {
	uint32_t	DATA;
	struct {
		uint8_t	BDATA[4];
	} BYTES;
	struct {
		uint32_t	ORDER_OF_CURRENT_UVDM_SET:4,
					RESERVED:9,
					COMMAND_TYPE:2,
					DATA_TYPE:1,
					PID:16;
	} BITS;
} U_SEC_UNSTRUCTURED_VDM_RESPONSE_HEADER_Type;

typedef union {
	uint32_t	DATA;
	struct {
		uint8_t	BDATA[4];
	} BYTES;
	struct {
		uint32_t	VENDOR_DEFINED_MESSAGE:15,
				VDM_TYPE:1,
				USB_Vendor_ID:16;
	} BITS;
} U_SEC_DATA_RESPONSE_HEADER_Type;

typedef struct {
	uint32_t	VDO[6];
} VDO_MESSAGE_Type;

/* For DP */
#define TypeC_POWER_SINK_INPUT     0
#define TypeC_POWER_SOURCE_OUTPUT     1
#define TypeC_DP_SUPPORT	(0xFF01)

/* For Dex */
#define TypeC_Dex_SUPPORT	(0x04E8)

/* For DP VDM Modes VDO Port_Capability */
typedef enum
{
    num_Reserved_Capable        = 0,
    num_UFP_D_Capable           = 1,
    num_DFP_D_Capable           = 2,
    num_DFP_D_and_UFP_D_Capable = 3
}Num_DP_Port_Capability_Type;

#endif
/* For DP VDM Modes VDO Receptacle_Indication */
typedef enum
{
    num_USB_TYPE_C_PLUG        = 0,
    num_USB_TYPE_C_Receptacle  = 1
}Num_DP_Receptacle_Indication_Type;


/* For DP_Status_Update Port_Connected */
typedef enum
{
    num_Adaptor_Disable         = 0,
    num_Connect_DFP_D           = 1,
    num_Connect_UFP_D           = 2,
    num_Connect_DFP_D_and_UFP_D = 3
}Num_DP_Port_Connected_Type;

/* For DP_Configure Select_Configuration */
typedef enum
{
    num_Cfg_for_USB             = 0,
    num_Cfg_UFP_U_as_DFP_D      = 1,
    num_Cfg_UFP_U_as_UFP_D      = 2,
    num_Cfg_Reserved            = 3
}Num_DP_Sel_Configuration_Type;

typedef union {
	uint32_t	DATA;
	struct {
		uint8_t     BDATA[4];
	} BYTES;
	struct {
		uint32_t	Port_Capability:2,
				Signalling_DP:4,
				Receptacle_Indication:1,
				USB_2p0_Not_Used:1,
				DFP_D_Pin_Assignments:8,
				UFP_D_Pin_Assignments:8,
				DP_MODE_VDO_Reserved:8;
	} BITS;
} U_VDO_MODE_DP_CAPABILITY_Type;

typedef union {
	uint32_t    DATA;
	struct {
		uint8_t     BDATA[4];
	} BYTES;
	struct {
		uint32_t    Port_Connected:2,
			    Power_Low:1,
			    Enabled:1,
			    Multi_Function_Preference:1,
			    USB_Configuration_Req:1,
			    Exit_DP_Mode_Req:1,
			    HPD_State:1,
			    HPD_Interrupt:1,
			    Reserved:23;
	} BITS;
} U_VDO_DP_STATUS_UPDATES_Type;

typedef union {
	uint32_t        DATA;
	struct {
		uint8_t     BDATA[4];
	} BYTES;
	struct {
		uint32_t	SEL_Configuration:2,
				Select_DP_V1p3:1,
				Select_USB_Gen2:1,
				Select_Reserved_1:2,
				Select_Reserved_2:2,
				DFP_D_PIN_Assign_A:1,
				DFP_D_PIN_Assign_B:1,
				DFP_D_PIN_Assign_C:1,
				DFP_D_PIN_Assign_D:1,
				DFP_D_PIN_Assign_E:1,
				DFP_D_PIN_Assign_F:1,
				DFP_D_PIN_Reserved:2,
				UFP_D_PIN_Assign_A:1,
				UFP_D_PIN_Assign_B:1,
				UFP_D_PIN_Assign_C:1,
				UFP_D_PIN_Assign_D:1,
				UFP_D_PIN_Assign_E:1,
				UFP_D_PIN_Assign_F:1,
				UFP_D_PIN_Reserved:2,
				DP_MODE_Reserved:8;
	} BITS;
} U_DP_CONFIG_UPDATE_Type;

typedef struct {
	U_DATA_MSG_HEADER_Type			MSG_HEADER;
	U_DATA_MSG_VDM_HEADER_Type		DATA_MSG_VDM_HEADER;
	U_VDO_MODE_DP_CAPABILITY_Type		DATA_MSG_MODE_VDO_DP;
} DIS_MODE_DP_CAPA_Type;

typedef struct {
	U_DATA_MSG_HEADER_Type			MSG_HEADER;
	U_DATA_MSG_VDM_HEADER_Type		DATA_MSG_VDM_HEADER;
	U_VDO_DP_STATUS_UPDATES_Type		DATA_DP_STATUS_UPDATE;
} DP_STATUS_UPDATE_Type;

typedef struct {
	U_DATA_MSG_HEADER_Type			MSG_HEADER;
	U_DATA_MSG_VDM_HEADER_Type		DATA_MSG_VDM_HEADER;
	U_VDO_DP_STATUS_UPDATES_Type		DATA_MSG_DP_STATUS;
} DIS_ATTENTION_MESSAGE_DP_STATUS_Type;


enum VDM_MSG_IRQ_State {
	VDM_DISCOVER_ID		=	(1 << 0),
	VDM_DISCOVER_SVIDS	=	(1 << 1),
	VDM_DISCOVER_MODES	=	(1 << 2),
	VDM_ENTER_MODE		=	(1 << 3),
	VDM_EXIT_MODE		=	(1 << 4),
	VDM_ATTENTION		=	(1 << 5),
	VDM_DP_STATUS_UPDATE	=	(1 << 6),
	VDM_DP_CONFIGURE	=	(1 << 7),
};

#define ALTERNATE_MODE_NOT_READY	(1 << 0)
#define ALTERNATE_MODE_READY		(1 << 1)
#define ALTERNATE_MODE_STOP		(1 << 2)
#define ALTERNATE_MODE_START		(1 << 3)
#define ALTERNATE_MODE_RESET		(1 << 4)

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

#define GEAR_VR_DETACH_WAIT_MS		1000
#define MODE_INT_CLEAR			0x01
#define PD_NEXT_STATE			0x02
#define MODE_INTERFACE			0x03
#define SVID_SELECT			0x07
#define REQ_PR_SWAP			0x10
#define REQ_DR_SWAP			0x11
#define SEL_SSM_MSG_REQ			0x20
#define DP_ALT_MODE_REQ			0x30
/* Samsung Acc VID */
#define SAMSUNG_VENDOR_ID		0x04E8
#define SAMSUNG_MPA_VENDOR_ID		0x04B4
/* Samsung Acc PID */
#define GEARVR_PRODUCT_ID		0xA500
#define GEARVR_PRODUCT_ID_1		0xA501
#define GEARVR_PRODUCT_ID_2		0xA502
#define GEARVR_PRODUCT_ID_3		0xA503
#define GEARVR_PRODUCT_ID_4		0xA504
#define GEARVR_PRODUCT_ID_5		0xA505
#define DEXDOCK_PRODUCT_ID		0xA020
#define HDMI_PRODUCT_ID			0xA025
#define MPA_PRODUCT_ID			0x2122
/* Samsung UVDM structure */
#define SEC_UVDM_SHORT_DATA		0x0
#define SEC_UVDM_LONG_DATA		0x1
#define SEC_UVDM_ININIATOR		0x0
#define SEC_UVDM_RESPONDER_ACK	0x1
#define SEC_UVDM_RESPONDER_NAK	0x2
#define SEC_UVDM_RESPONDER_BUSY	0x3
#define SEC_UVDM_UNSTRUCTURED_VDM	0x0

/*For DP Pin Assignment */
#define DP_PIN_ASSIGNMENT_NODE	0x00000000
#define DP_PIN_ASSIGNMENT_A	0x00000001	/* ( 1 << 0 ) */
#define DP_PIN_ASSIGNMENT_B	0x00000002	/* ( 1 << 1 ) */
#define DP_PIN_ASSIGNMENT_C	0x00000004	/* ( 1 << 2 ) */
#define DP_PIN_ASSIGNMENT_D	0x00000008	/* ( 1 << 3 ) */
#define DP_PIN_ASSIGNMENT_E	0x00000010	/* ( 1 << 4 ) */
#define DP_PIN_ASSIGNMENT_F	0x00000020	/* ( 1 << 5 ) */

void send_alternate_message(void *data, int cmd);
void receive_alternate_message(void *data, VDM_MSG_IRQ_STATUS_Type
			       *VDM_MSG_IRQ_State);
int ccic_register_switch_device(int mode);
void acc_detach_check(struct work_struct *work);
void send_unstructured_vdm_message(void * data, int cmd);
void send_dna_audio_unstructured_vdm_message(void * data, int cmd);
void receive_unstructured_vdm_message(void * data, SSM_MSG_IRQ_STATUS_Type *SSM_MSG_IRQ_State);
void do_alternate_mode_step_by_step(void * data, int cmd);
void send_dex_fan_unstructured_vdm_message(void * data, int cmd);
int send_samsung_unstructured_vdm_message(void * data, const char *buf, size_t size);
void set_enable_alternate_mode(int mode);
void set_clear_discover_mode(void);
void set_host_turn_on_event(int mode);
