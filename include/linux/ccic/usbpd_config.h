#ifndef _USBPD_CONFIG_H
#define _USBPD_CONFIG_H

#define PD_SID		(0xFF00)
#define PD_SID_1	(0xFF01)

#define AVAILABLE_VOLTAGE 12000
#define UNIT_FOR_VOLTAGE 50
#define UNIT_FOR_CURRENT 10

#define ALTERNATE_MODE_NOT_READY	(1 << 0)
#define ALTERNATE_MODE_READY		(1 << 1)
#define ALTERNATE_MODE_STOP		(1 << 2)
#define ALTERNATE_MODE_START		(1 << 3)
#define ALTERNATE_MODE_RESET		(1 << 4)

#define GEAR_VR_DETACH_WAIT_MS		1000

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

typedef enum {
	POWER_TYPE_FIXED = 0,
	POWER_TYPE_BATTERY,
	POWER_TYPE_VARIABLE,
} power_supply_type;

typedef enum {
	SOP_TYPE_SOP,
	SOP_TYPE_SOP1,
	SOP_TYPE_SOP2,
	SOP_TYPE_SOP1_DEBUG,
	SOP_TYPE_SOP2_DEBUG
} sop_type;

enum usbpd_control_msg_type {
	USBPD_GoodCRC        = 0x1,
	USBPD_GotoMin        = 0x2,
	USBPD_Accept         = 0x3,
	USBPD_Reject         = 0x4,
	USBPD_Ping           = 0x5,
	USBPD_PS_RDY         = 0x6,
	USBPD_Get_Source_Cap = 0x7,
	USBPD_Get_Sink_Cap   = 0x8,
	USBPD_DR_Swap        = 0x9,
	USBPD_PR_Swap        = 0xA,
	USBPD_VCONN_Swap     = 0xB,
	USBPD_Wait           = 0xC,
	USBPD_Soft_Reset     = 0xD
};

enum usbpd_check_msg_pass {
	NONE_CHECK_MSG_PASS,
	CHECK_MSG_PASS,
};

enum usbpd_port_data_role {
	USBPD_UFP,
	USBPD_DFP,
};

enum usbpd_port_power_role {
	USBPD_SINK,
	USBPD_SOURCE,
};

enum usbpd_port_vconn_role {
	USBPD_VCONN_OFF,
	USBPD_VCONN_ON,
};

enum usbpd_port_role {
	USBPD_Rp	= 0x01,
	USBPD_Rd	= 0x01 << 1,
	USBPD_Ra	= 0x01 << 2,
};

enum vdm_command_type{
	Initiator       = 0,
	Responder_ACK   = 1,
	Responder_NAK   = 2,
	Responder_BUSY  = 3
};

enum vdm_type{
	Unstructured_VDM = 0,
	Structured_VDM = 1
};

enum vdm_configure_type{
	USB		= 0,
	USB_U_AS_DFP_D	= 1,
	USB_U_AS_UFP_D	= 2
};

enum vdm_displayport_protocol{
	UNSPECIFIED	= 0,
	DP_V_1_3	= 1,
	GEN_2		= 1 << 1
};

enum vdm_pin_assignment{
	DE_SELECT_PIN		= 0,
	PIN_ASSIGNMENT_A	= 1,
	PIN_ASSIGNMENT_B	= 1 << 1,
	PIN_ASSIGNMENT_C	= 1 << 2,
	PIN_ASSIGNMENT_D	= 1 << 3,
	PIN_ASSIGNMENT_E	= 1 << 4,
	PIN_ASSIGNMENT_F	= 1 << 5,
};

/* For DP */
#define TypeC_POWER_SINK_INPUT     0
#define TypeC_POWER_SOURCE_OUTPUT     1
#define TypeC_DP_SUPPORT	(0xFF01)

/* For Dex */
#define TypeC_Dex_SUPPORT	(0x04E8)

/* For DP VDM Modes VDO Port_Capability */
typedef enum {
    num_Reserved_Capable        = 0,
    num_UFP_D_Capable           = 1,
    num_DFP_D_Capable           = 2,
    num_DFP_D_and_UFP_D_Capable = 3
} Num_DP_Port_Capability_Type;

/* For DP VDM Modes VDO Receptacle_Indication */
typedef enum {
    num_USB_TYPE_C_PLUG        = 0,
    num_USB_TYPE_C_Receptacle  = 1
} Num_DP_Receptacle_Indication_Type;


/* For DP_Status_Update Port_Connected */
typedef enum {
    num_Adaptor_Disable         = 0,
    num_Connect_DFP_D           = 1,
    num_Connect_UFP_D           = 2,
    num_Connect_DFP_D_and_UFP_D = 3
} Num_DP_Port_Connected_Type;

/* For DP_Configure Select_Configuration */
typedef enum {
    num_Cfg_for_USB             = 0,
    num_Cfg_UFP_U_as_DFP_D      = 1,
    num_Cfg_UFP_U_as_UFP_D      = 2,
    num_Cfg_Reserved            = 3
} Num_DP_Sel_Configuration_Type;

enum vdm_command_msg {
	Discover_Identity		= 1 << 0,
	Discover_SVIDs			= 1 << 1,
	Discover_Modes			= 1 << 2,
	Enter_Mode			= 1 << 3,
	Exit_Mode			= 1 << 4,
	Attention			= 1 << 5,
	DisplayPort_Status_Update	= 1 << 6,
	DisplayPort_Configure		= 1 << 7,
};

enum usbpd_data_msg_type {
	USBPD_Source_Capabilities	= 0x1,
	USBPD_Request		        = 0x2,
	USBPD_BIST			= 0x3,
	USBPD_Sink_Capabilities		= 0x4,
	USBPD_Vendor_Defined		= 0xF,
};
#define DP_PIN_ASSIGNMENT_F	0x00000020	/* ( 1 << 5 ) */

/* CCIC Dock Observer Callback parameter */
enum {
	CCIC_DOCK_DETACHED	= 0,
	CCIC_DOCK_HMT		= 105,	/* Samsung Gear VR */
	CCIC_DOCK_ABNORMAL	= 106,
	CCIC_DOCK_MPA		= 109,	/* Samsung Multi Port Adaptor */
	CCIC_DOCK_DEX		= 110,	/* Samsung Dex */
	CCIC_DOCK_HDMI		= 111,	/* Samsung HDMI Dongle */
};
#endif
