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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/mod_devicetable.h>
#include <linux/mfd/max77705-private.h>
#include <linux/ccic/max77705_usbc.h>
#include <linux/ccic/max77705_alternate.h>
#include <linux/completion.h>

#if defined(CONFIG_USB_HOST_NOTIFY)
#include <linux/usb_notify.h>
#endif
#if defined(CONFIG_CCIC_NOTIFIER)
#include <linux/ccic/ccic_notifier.h>
#include <linux/ccic/ccic_core.h>
#endif

#define UVDM_DEBUG (0)
#define SEC_UVDM_ALIGN		(4)
#define MAX_DATA_FIRST_UVDMSET	12
#define MAX_DATA_NORMAL_UVDMSET	16
#define CHECKSUM_DATA_COUNT		20
#define MAX_INPUT_DATA (255)

extern struct max77705_usbc_platform_data *g_usbc_data;

const struct DP_DP_DISCOVER_IDENTITY DP_DISCOVER_IDENTITY = {
	{
		.BITS.Num_Of_VDO = 1,
		.BITS.Cmd_Type = ACK,
		.BITS.Reserved = 0
	},

	{
		.BITS.VDM_command = Discover_Identity,
		.BITS.Rsvd2_VDM_header = 0,
		.BITS.VDM_command_type = REQ,
		.BITS.Object_Position = 0,
		.BITS.Rsvd_VDM_header = 0,
		.BITS.Structured_VDM_Version = Version_1_0,
		.BITS.VDM_Type = STRUCTURED_VDM,
		.BITS.Standard_Vendor_ID = 0xFF00
	}

};
const struct DP_DP_DISCOVER_ENTER_MODE DP_DISCOVER_ENTER_MODE = {
	{
		.BITS.Num_Of_VDO = 1,
		.BITS.Cmd_Type = ACK,
		.BITS.Reserved = 0
	},
	{
		.BITS.VDM_command = Enter_Mode,
		.BITS.Rsvd2_VDM_header = 0,
		.BITS.VDM_command_type = REQ,
		.BITS.Object_Position = 1,
		.BITS.Rsvd_VDM_header = 0,
		.BITS.Structured_VDM_Version = Version_1_0,
		.BITS.VDM_Type = STRUCTURED_VDM,
		.BITS.Standard_Vendor_ID = 0xFF01
	}
};

struct DP_DP_CONFIGURE DP_CONFIGURE = {
	{
		.BITS.Num_Of_VDO = 2,
		.BITS.Cmd_Type = ACK,
		.BITS.Reserved = 0
	},
	{
		.BITS.VDM_command = 17, /* SVID Specific Command */
		.BITS.Rsvd2_VDM_header = 0,
		.BITS.VDM_command_type = REQ,
		.BITS.Object_Position = 1,
		.BITS.Rsvd_VDM_header = 0,
		.BITS.Structured_VDM_Version = Version_1_0,
		.BITS.VDM_Type = STRUCTURED_VDM,
		.BITS.Standard_Vendor_ID = 0xFF01
	},
	{
		.BITS.SEL_Configuration = num_Cfg_UFP_U_as_UFP_D,
		.BITS.Select_DP_V1p3 = 1,
		.BITS.Select_USB_Gen2 = 0,
		.BITS.Select_Reserved_1 = 0,
		.BITS.Select_Reserved_2 = 0,
		.BITS.DFP_D_PIN_Assign_A = 0,
		.BITS.DFP_D_PIN_Assign_B = 0,
		.BITS.DFP_D_PIN_Assign_C = 0,
		.BITS.DFP_D_PIN_Assign_D = 1,
		.BITS.DFP_D_PIN_Assign_E = 0,
		.BITS.DFP_D_PIN_Assign_F = 0,
		.BITS.DFP_D_PIN_Reserved = 0,
		.BITS.UFP_D_PIN_Assign_A = 0,
		.BITS.UFP_D_PIN_Assign_B = 0,
		.BITS.UFP_D_PIN_Assign_C = 0,
		.BITS.UFP_D_PIN_Assign_D = 0,
		.BITS.UFP_D_PIN_Assign_E = 0,
		.BITS.UFP_D_PIN_Assign_F = 0,
		.BITS.UFP_D_PIN_Reserved = 0,
		.BITS.DP_MODE_Reserved = 0
	}
};

struct SS_DEX_DISCOVER_MODE SS_DEX_DISCOVER_MODE_MODE = {
	{
		.BITS.Num_Of_VDO = 1,
		.BITS.Cmd_Type = ACK,
		.BITS.Reserved = 0
	},
	{
		.BITS.VDM_command = Discover_Modes,
		.BITS.Rsvd2_VDM_header = 0,
		.BITS.VDM_command_type = REQ,
		.BITS.Object_Position = 0,
		.BITS.Rsvd_VDM_header = 0,
		.BITS.Structured_VDM_Version = Version_1_0,
		.BITS.VDM_Type = STRUCTURED_VDM,
		.BITS.Standard_Vendor_ID = SAMSUNG_VENDOR_ID
	}
};

struct SS_DEX_ENTER_MODE SS_DEX_DISCOVER_ENTER_MODE = {
	{
		.BITS.Num_Of_VDO = 1,
		.BITS.Cmd_Type = ACK,
		.BITS.Reserved = 0
	},
	{
		.BITS.VDM_command = Enter_Mode,
		.BITS.Rsvd2_VDM_header = 0,
		.BITS.VDM_command_type = REQ,
		.BITS.Object_Position = 1,
		.BITS.Rsvd_VDM_header = 0,
		.BITS.Structured_VDM_Version = Version_1_0,
		.BITS.VDM_Type = STRUCTURED_VDM,
		.BITS.Standard_Vendor_ID = SAMSUNG_VENDOR_ID
	}
};

struct SS_UNSTRUCTURED_VDM_MSG SS_DEX_UNSTRUCTURED_VDM;

static char VDM_MSG_IRQ_State_Print[9][40] = {
	{"bFLAG_Vdm_Reserve_b0"},
	{"bFLAG_Vdm_Discover_ID"},
	{"bFLAG_Vdm_Discover_SVIDs"},
	{"bFLAG_Vdm_Discover_MODEs"},
	{"bFLAG_Vdm_Enter_Mode"},
	{"bFLAG_Vdm_Exit_Mode"},
	{"bFLAG_Vdm_Attention"},
	{"bFlag_Vdm_DP_Status_Update"},
	{"bFlag_Vdm_DP_Configure"},
};
static char DP_Pin_Assignment_Print[7][40] = {
	{"DP_Pin_Assignment_None"},
	{"DP_Pin_Assignment_A"},
	{"DP_Pin_Assignment_B"},
	{"DP_Pin_Assignment_C"},
	{"DP_Pin_Assignment_D"},
	{"DP_Pin_Assignment_E"},
	{"DP_Pin_Assignment_F"},
};

static uint8_t DP_Pin_Assignment_Data[7] = {
	DP_PIN_ASSIGNMENT_NODE,
	DP_PIN_ASSIGNMENT_A,
	DP_PIN_ASSIGNMENT_B,
	DP_PIN_ASSIGNMENT_C,
	DP_PIN_ASSIGNMENT_D,
	DP_PIN_ASSIGNMENT_E,
};

int max77705_process_check_accessory(void *data)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
#if defined(CONFIG_USB_HW_PARAM)
	struct otg_notify *o_notify = get_otg_notify();
#endif
	uint16_t vid = usbpd_data->Vendor_ID;
	uint16_t pid = usbpd_data->Product_ID;
	uint16_t acc_type = CCIC_DOCK_DETACHED;

	/* detect Gear VR */
	if (usbpd_data->acc_type == CCIC_DOCK_DETACHED) {
		if (vid == SAMSUNG_VENDOR_ID) {
			switch (pid) {
			/* GearVR: Reserved GearVR PID+6 */
			case GEARVR_PRODUCT_ID:
			case GEARVR_PRODUCT_ID_1:
			case GEARVR_PRODUCT_ID_2:
			case GEARVR_PRODUCT_ID_3:
			case GEARVR_PRODUCT_ID_4:
			case GEARVR_PRODUCT_ID_5:
				acc_type = CCIC_DOCK_HMT;
				msg_maxim("Samsung Gear VR connected.");
#if defined(CONFIG_USB_HW_PARAM)
				if (o_notify)
					inc_hw_param(o_notify, USB_CCIC_VR_USE_COUNT);
#endif
				break;
			case DEXDOCK_PRODUCT_ID:
				acc_type = CCIC_DOCK_DEX;
				msg_maxim("Samsung DEX connected.");
#if defined(CONFIG_USB_HW_PARAM)
				if (o_notify)
					inc_hw_param(o_notify, USB_CCIC_DEX_USE_COUNT);
#endif
				break;
			case DEXPAD_PRODUCT_ID:
				acc_type = CCIC_DOCK_DEXPAD;
				msg_maxim("Samsung DEX PAD connected.");
#if defined(CONFIG_USB_HW_PARAM)
				if (o_notify)
					inc_hw_param(o_notify, USB_CCIC_DEX_USE_COUNT);
#endif
				break;
			case HDMI_PRODUCT_ID:
				acc_type = CCIC_DOCK_HDMI;
				msg_maxim("Samsung HDMI connected.");
				break;
			default:
				msg_maxim("default device connected.");
				acc_type = CCIC_DOCK_NEW;
				break;
			}
		} else if (vid == SAMSUNG_MPA_VENDOR_ID) {
			switch (pid) {
			case MPA_PRODUCT_ID:
				acc_type = CCIC_DOCK_MPA;
				msg_maxim("Samsung MPA connected.");
				break;
			default:
				msg_maxim("default device connected.");
				acc_type = CCIC_DOCK_NEW;
				break;
			}
		}
		usbpd_data->acc_type = acc_type;
	} else
		acc_type = usbpd_data->acc_type;

	if (acc_type != CCIC_DOCK_NEW)
		ccic_send_dock_intent(acc_type);

	ccic_send_dock_uevent(vid, pid, acc_type);
	return 1;
}

void max77705_vdm_process_printf(char *type, char *vdm_data, int len)
{
#if 0
	int i = 0;

	for (i = 2; i < len; i++)
		msg_maxim("[%s] , %d, [0x%x]", type, i, vdm_data[i]);
#endif
}

void max77705_vdm_process_set_identity_req(void *data)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	/* send the opcode */
	usbc_cmd_data write_data;
	int len = sizeof(DP_DISCOVER_IDENTITY);
	int vdm_header_num = sizeof(UND_DATA_MSG_VDM_HEADER_Type);
	int vdo0_num = 0;
	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_VDM_DISCOVER_SET_VDM_REQ;
	memcpy(write_data.write_data, &DP_DISCOVER_IDENTITY, sizeof(DP_DISCOVER_IDENTITY));
	write_data.write_length = len;
	vdo0_num = DP_DISCOVER_IDENTITY.byte_data.BITS.Num_Of_VDO * 4;
	write_data.read_length = OPCODE_SIZE + OPCODE_HEADER_SIZE + vdm_header_num + vdo0_num;
	max77705_usbc_opcode_write(usbpd_data, &write_data);
}

void max77705_vdm_process_set_DP_enter_mode_req(void *data)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	usbc_cmd_data write_data;
	int len = sizeof(DP_DISCOVER_ENTER_MODE);
	int vdm_header_num = sizeof(UND_DATA_MSG_VDM_HEADER_Type);
	int vdo0_num = 0;
	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_VDM_DISCOVER_SET_VDM_REQ;
	memcpy(write_data.write_data, &DP_DISCOVER_ENTER_MODE, sizeof(DP_DISCOVER_ENTER_MODE));
	write_data.write_length = len;
	vdo0_num = DP_DISCOVER_ENTER_MODE.byte_data.BITS.Num_Of_VDO * 4;
	write_data.read_length = OPCODE_SIZE + OPCODE_HEADER_SIZE + vdm_header_num + vdo0_num;
	max77705_usbc_opcode_push(usbpd_data, &write_data);
}

void max77705_vdm_process_set_DP_configure_mode_req(void *data, uint8_t W_DATA)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	usbc_cmd_data write_data;
	int len = sizeof(DP_CONFIGURE);
	int vdm_header_num = sizeof(UND_DATA_MSG_VDM_HEADER_Type);
	int vdo0_num = 0;
	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_VDM_DISCOVER_SET_VDM_REQ;
	memcpy(write_data.write_data, &DP_CONFIGURE, sizeof(DP_CONFIGURE));
	write_data.write_data[6] = W_DATA;
	write_data.write_length = len;
	vdo0_num = DP_CONFIGURE.byte_data.BITS.Num_Of_VDO * 4;
	write_data.read_length = OPCODE_SIZE + OPCODE_HEADER_SIZE + vdm_header_num + vdo0_num;
	max77705_usbc_opcode_push(usbpd_data, &write_data);
}

void max77705_vdm_process_set_Dex_Disover_mode_req(void *data)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	usbc_cmd_data write_data;
	int len = sizeof(SS_DEX_DISCOVER_MODE_MODE);
	int vdm_header_num = sizeof(UND_DATA_MSG_VDM_HEADER_Type);
	int vdo0_num = 0;
	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_VDM_DISCOVER_SET_VDM_REQ;
	memcpy(write_data.write_data, &SS_DEX_DISCOVER_MODE_MODE, sizeof(SS_DEX_DISCOVER_MODE_MODE));
	write_data.write_length = len;
	vdo0_num = SS_DEX_DISCOVER_MODE_MODE.byte_data.BITS.Num_Of_VDO * 4;
	write_data.read_length = OPCODE_SIZE + OPCODE_HEADER_SIZE + vdm_header_num + vdo0_num;
	max77705_usbc_opcode_push(usbpd_data, &write_data);
}

void max77705_vdm_process_set_Dex_enter_mode_req(void *data)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	usbc_cmd_data write_data;
	int len = sizeof(SS_DEX_DISCOVER_ENTER_MODE);
	int vdm_header_num = sizeof(UND_DATA_MSG_VDM_HEADER_Type);
	int vdo0_num = 0;
	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_VDM_DISCOVER_SET_VDM_REQ;
	memcpy(write_data.write_data, &SS_DEX_DISCOVER_ENTER_MODE, sizeof(SS_DEX_DISCOVER_ENTER_MODE));
	write_data.write_length = len;
	vdo0_num = SS_DEX_DISCOVER_ENTER_MODE.byte_data.BITS.Num_Of_VDO * 4;
	write_data.read_length = OPCODE_SIZE + OPCODE_HEADER_SIZE + vdm_header_num + vdo0_num;
	max77705_usbc_opcode_push(usbpd_data, &write_data);
}

static int max77705_vdm_process_discover_svids(void *data, char *vdm_data, int len)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	int timeleft = 0, i = 0;
	uint16_t svid = 0;
#if defined(CONFIG_USB_HOST_NOTIFY)
		struct otg_notify *o_notify = get_otg_notify();
#endif
	DIS_MODE_DP_CAPA_Type *pDP_DIS_MODE = (DIS_MODE_DP_CAPA_Type *)&vdm_data[0];
	int num_of_vdos = (pDP_DIS_MODE->MSG_HEADER.BITS.Number_of_obj - 2) * 2;
	UND_VDO1_Type  *DATA_MSG_VDO1 = (UND_VDO1_Type  *)&vdm_data[8];
	usbpd_data->SVID_DP = 0;
	usbpd_data->SVID_0 = DATA_MSG_VDO1->BITS.SVID_0;
	usbpd_data->SVID_1 = DATA_MSG_VDO1->BITS.SVID_1;

	for (i = 0; i < num_of_vdos; i++) {
		memcpy(&svid, &vdm_data[8 + i * 2], 2);
		if (svid == TypeC_DP_SUPPORT) {
			msg_maxim("svid_%d : 0x%X", i, svid);
			usbpd_data->SVID_DP = svid;
			break;
		}
	}

	if (usbpd_data->SVID_DP == TypeC_DP_SUPPORT) {
		if (usbpd_data->is_client == CLIENT_ON) {
			max77705_ccic_event_work(usbpd_data,
				CCIC_NOTIFY_DEV_MUIC,
				CCIC_NOTIFY_ID_ATTACH, 0/*attach*/, 0/*rprd*/, 0);
			max77705_ccic_event_work(usbpd_data,
				CCIC_NOTIFY_DEV_USB, CCIC_NOTIFY_ID_USB,
				0/*attach*/, USB_STATUS_NOTIFY_DETACH/*drp*/, 0);
			usbpd_data->is_client = CLIENT_OFF;
		}

		if (usbpd_data->is_host == HOST_OFF) {
			/* muic */
			max77705_ccic_event_work(usbpd_data,
				CCIC_NOTIFY_DEV_MUIC,
				CCIC_NOTIFY_ID_ATTACH, 1/*attach*/, 1/*rprd*/, 0);
			/* otg */
			usbpd_data->is_host = HOST_ON;
			max77705_ccic_event_work(usbpd_data,
				CCIC_NOTIFY_DEV_USB, CCIC_NOTIFY_ID_USB,
				1/*attach*/, USB_STATUS_NOTIFY_ATTACH_DFP/*drp*/, 0);
		}
		usbpd_data->dp_is_connect = 1;
		/* If you want to support USB SuperSpeed when you connect
		 * Display port dongle, You should change dp_hs_connect depend
		 * on Pin assignment.If DP use 4lane(Pin Assignment C,E,A),
		 * dp_hs_connect is 1. USB can support HS.If DP use 2lane(Pin Assignment B,D,F), dp_hs_connect is 0. USB
		 * can support SS
		 */
		 usbpd_data->dp_hs_connect = 1;

#if 1
		max77705_ccic_event_work(usbpd_data,
				CCIC_NOTIFY_DEV_DP, CCIC_NOTIFY_ID_DP_CONNECT,
				CCIC_NOTIFY_ATTACH, usbpd_data->Vendor_ID, usbpd_data->Product_ID);
#if defined(CONFIG_USB_HOST_NOTIFY)
		if (o_notify) {
#if defined(CONFIG_USB_HW_PARAM)
			inc_hw_param(o_notify, USB_CCIC_DP_USE_COUNT);
#endif
			timeleft = wait_event_interruptible_timeout(usbpd_data->host_turn_on_wait_q,
					usbpd_data->host_turn_on_event && !usbpd_data->detach_done_wait, (usbpd_data->host_turn_on_wait_time)*HZ);
			msg_maxim("%s host turn on wait = %d\n", __func__, timeleft);
		}
#endif
		max77705_ccic_event_work(usbpd_data,
			CCIC_NOTIFY_DEV_USB_DP, CCIC_NOTIFY_ID_USB_DP,
			usbpd_data->dp_is_connect /*attach*/, usbpd_data->dp_hs_connect, 0);
#endif
	}
	msg_maxim("SVID_0 : 0x%X, SVID_1 : 0x%X",
			usbpd_data->SVID_0, usbpd_data->SVID_1);
	return 0;
}

static int max77705_vdm_process_discover_mode(void *data, char *vdm_data, int len)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	DIS_MODE_DP_CAPA_Type *pDP_DIS_MODE = (DIS_MODE_DP_CAPA_Type *)&vdm_data[0];
	UND_DATA_MSG_VDM_HEADER_Type *DATA_MSG_VDM = (UND_DATA_MSG_VDM_HEADER_Type *)&vdm_data[4];

	msg_maxim("vendor_id = 0x%04x , svid_1 = 0x%04x", DATA_MSG_VDM->BITS.Standard_Vendor_ID, usbpd_data->SVID_1);
	if (DATA_MSG_VDM->BITS.Standard_Vendor_ID == TypeC_DP_SUPPORT && usbpd_data->SVID_DP == TypeC_DP_SUPPORT) {
		/*  pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS. */
		msg_maxim("pDP_DIS_MODE->MSG_HEADER.DATA = 0x%08X", pDP_DIS_MODE->MSG_HEADER.DATA);
		msg_maxim("pDP_DIS_MODE->DATA_MSG_VDM_HEADER.DATA = 0x%08X", pDP_DIS_MODE->DATA_MSG_VDM_HEADER.DATA);
		msg_maxim("pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.DATA = 0x%08X", pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.DATA);

		if (pDP_DIS_MODE->MSG_HEADER.BITS.Number_of_obj > 1) {
			if ((pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS.Port_Capability == num_UFP_D_Capable)
				&& (pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS.Receptacle_Indication == num_USB_TYPE_C_Receptacle)) {
				usbpd_data->pin_assignment = pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS.UFP_D_Pin_Assignments;
				msg_maxim("1. support UFP_D 0x%08x", usbpd_data->pin_assignment);
			} else if (((pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS.Port_Capability == num_UFP_D_Capable)
				&& (pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS.Receptacle_Indication == num_USB_TYPE_C_PLUG))) {
				usbpd_data->pin_assignment = pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS.DFP_D_Pin_Assignments;
				msg_maxim("2. support DFP_D 0x%08x", usbpd_data->pin_assignment);
			} else if (pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS.Port_Capability == num_DFP_D_and_UFP_D_Capable) {
				if (pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS.Receptacle_Indication == num_USB_TYPE_C_PLUG) {
					usbpd_data->pin_assignment = pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS.DFP_D_Pin_Assignments;
					msg_maxim("3. support DFP_D 0x%08x", usbpd_data->pin_assignment);
				} else {
					usbpd_data->pin_assignment = pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS.UFP_D_Pin_Assignments;
					msg_maxim("4. support UFP_D 0x%08x", usbpd_data->pin_assignment);
				}
			} else if (pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS.Port_Capability == num_DFP_D_Capable) {
				usbpd_data->pin_assignment = DP_PIN_ASSIGNMENT_NODE;
				msg_maxim("do not support Port_Capability num_DFP_D_Capable!!!");
				return -EINVAL;
			} else {
				usbpd_data->pin_assignment = DP_PIN_ASSIGNMENT_NODE;
				msg_maxim("there is not valid object information!!!");
				return -EINVAL;
			}
		}
	}

	if (DATA_MSG_VDM->BITS.Standard_Vendor_ID == SAMSUNG_VENDOR_ID) {
		msg_maxim("dex mode discover_mode ack status!");
		/*  pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.BITS. */
		msg_maxim("pDP_DIS_MODE->MSG_HEADER.DATA = 0x%08X", pDP_DIS_MODE->MSG_HEADER.DATA);
		msg_maxim("pDP_DIS_MODE->DATA_MSG_VDM_HEADER.DATA = 0x%08X", pDP_DIS_MODE->DATA_MSG_VDM_HEADER.DATA);
		msg_maxim("pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.DATA = 0x%08X", pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.DATA);
		/*Samsung Enter Mode */
		if (usbpd_data->send_enter_mode_req == 0) {
			msg_maxim("dex: second enter mode request");
			usbpd_data->send_enter_mode_req = 1;
			max77705_vdm_process_set_Dex_enter_mode_req(usbpd_data);
		}
	} else {
		max77705_vdm_process_set_DP_enter_mode_req(usbpd_data);
	}

	return 0;
}

static int max77705_vdm_process_enter_mode(void *data, char *vdm_data, int len)
{
	struct max77705_usbc_platform_data *usbpd_data = data;

	UND_DATA_MSG_VDM_HEADER_Type *DATA_MSG_VDM = (UND_DATA_MSG_VDM_HEADER_Type *)&vdm_data[4];

	if (DATA_MSG_VDM->BITS.VDM_command_type == 1) {
		msg_maxim("EnterMode ACK.");
		if (DATA_MSG_VDM->BITS.Standard_Vendor_ID == SAMSUNG_VENDOR_ID) {
			usbpd_data->is_samsung_accessory_enter_mode = 1;
			msg_maxim("dex mode enter_mode ack status!");
		}
	} else {
		msg_maxim("EnterMode NAK.");
	}
	return 0;
}

static int max77705_vdm_dp_select_pin(void *data, int multi)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	int pin_sel = 0;

	if (multi) {
		if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_D)
			pin_sel = CCIC_NOTIFY_DP_PIN_D;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_B)
			pin_sel = CCIC_NOTIFY_DP_PIN_B;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_F)
			pin_sel = CCIC_NOTIFY_DP_PIN_F;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_C)
			pin_sel = CCIC_NOTIFY_DP_PIN_C;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_E)
			pin_sel = CCIC_NOTIFY_DP_PIN_E;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_A)
			pin_sel = CCIC_NOTIFY_DP_PIN_A;
		else
			msg_maxim("wrong pin assignment value");
	} else {
		if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_C)
			pin_sel = CCIC_NOTIFY_DP_PIN_C;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_E)
			pin_sel = CCIC_NOTIFY_DP_PIN_E;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_A)
			pin_sel = CCIC_NOTIFY_DP_PIN_A;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_D)
			pin_sel = CCIC_NOTIFY_DP_PIN_D;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_B)
			pin_sel = CCIC_NOTIFY_DP_PIN_B;
		else if (usbpd_data->pin_assignment & DP_PIN_ASSIGNMENT_F)
			pin_sel = CCIC_NOTIFY_DP_PIN_F;
		else
			msg_maxim("wrong pin assignment value");
	}
	return pin_sel;
}

static int max77705_vdm_dp_status_update(void *data, char *vdm_data, int len)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	int i;
	uint8_t multi_func = 0;
	int pin_sel = 0;
	int hpd = 0;
	int hpdirq = 0;
	VDO_MESSAGE_Type *VDO_MSG;
	DP_STATUS_UPDATE_Type *DP_STATUS;
	uint8_t W_DATA = 0x0;

	if (usbpd_data->SVID_DP == TypeC_DP_SUPPORT) {
		DP_STATUS = (DP_STATUS_UPDATE_Type *)&vdm_data[0];

		msg_maxim("DP_STATUS_UPDATE = 0x%08X", DP_STATUS->DATA_DP_STATUS_UPDATE.DATA);

		if (DP_STATUS->DATA_DP_STATUS_UPDATE.BITS.Port_Connected == 0x00) {
			msg_maxim("port disconnected!");
		} else {
			if (usbpd_data->is_sent_pin_configuration == 0) {
				multi_func = DP_STATUS->DATA_DP_STATUS_UPDATE.BITS.Multi_Function_Preference;
				pin_sel = max77705_vdm_dp_select_pin(usbpd_data, multi_func);
				usbpd_data->dp_selected_pin = pin_sel;
				W_DATA = DP_Pin_Assignment_Data[pin_sel];

				msg_maxim("multi_func_preference %d,  %s, W_DATA : %d",
					multi_func, DP_Pin_Assignment_Print[pin_sel], W_DATA);

				max77705_vdm_process_set_DP_configure_mode_req(data, W_DATA);

				usbpd_data->is_sent_pin_configuration = 1;
			} else {
				msg_maxim("pin configuration is already sent as %s!",
					DP_Pin_Assignment_Print[usbpd_data->dp_selected_pin]);
			}
		}

		if (DP_STATUS->DATA_DP_STATUS_UPDATE.BITS.HPD_State == 1)
			hpd = CCIC_NOTIFY_HIGH;
		else if (DP_STATUS->DATA_DP_STATUS_UPDATE.BITS.HPD_State == 0)
			hpd = CCIC_NOTIFY_LOW;

		if (DP_STATUS->DATA_DP_STATUS_UPDATE.BITS.HPD_Interrupt == 1)
			hpdirq = CCIC_NOTIFY_IRQ;

		max77705_ccic_event_work(usbpd_data,
				CCIC_NOTIFY_DEV_DP, CCIC_NOTIFY_ID_DP_HPD,
				hpd, hpdirq, 0);
	} else {
		/* need to check F/W code */
		VDO_MSG = (VDO_MESSAGE_Type *)&vdm_data[8];
		for (i = 0; i < 6; i++)
			msg_maxim("VDO_%d : %d", i+1, VDO_MSG->VDO[i]);
	}
	return 0;
}

static int max77705_vdm_dp_attention(void *data, char *vdm_data, int len)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	int i;
	int hpd = 0;
	int hpdirq = 0;
	uint8_t multi_func = 0;
	int pin_sel = 0;

	VDO_MESSAGE_Type *VDO_MSG;
	DIS_ATTENTION_MESSAGE_DP_STATUS_Type *DP_ATTENTION;
	uint8_t W_DATA = 0;

	if (usbpd_data->SVID_DP == TypeC_DP_SUPPORT) {
		DP_ATTENTION = (DIS_ATTENTION_MESSAGE_DP_STATUS_Type *)&vdm_data[0];

		msg_maxim("%s DP_ATTENTION = 0x%08X\n", __func__,
			DP_ATTENTION->DATA_MSG_DP_STATUS.DATA);
		if (usbpd_data->is_sent_pin_configuration == 0) {
			multi_func = DP_ATTENTION->DATA_MSG_DP_STATUS.BITS.Multi_Function_Preference;
			pin_sel = max77705_vdm_dp_select_pin(usbpd_data, multi_func);
			usbpd_data->dp_selected_pin = pin_sel;
			W_DATA = DP_Pin_Assignment_Data[pin_sel];

			msg_maxim("multi_func_preference %d,  %s, W_DATA : %d\n",
				multi_func, DP_Pin_Assignment_Print[pin_sel], W_DATA);

			max77705_vdm_process_set_DP_configure_mode_req(data, W_DATA);

			usbpd_data->is_sent_pin_configuration = 1;
		} else {
			msg_maxim("%s : pin configuration is already sent as %s!\n", __func__,
				DP_Pin_Assignment_Print[usbpd_data->dp_selected_pin]);
		}
		if (DP_ATTENTION->DATA_MSG_DP_STATUS.BITS.HPD_State == 1)
			hpd = CCIC_NOTIFY_HIGH;
		else if (DP_ATTENTION->DATA_MSG_DP_STATUS.BITS.HPD_State == 0)
			hpd = CCIC_NOTIFY_LOW;

		if (DP_ATTENTION->DATA_MSG_DP_STATUS.BITS.HPD_Interrupt == 1)
			hpdirq = CCIC_NOTIFY_IRQ;

		max77705_ccic_event_work(usbpd_data,
			CCIC_NOTIFY_DEV_DP, CCIC_NOTIFY_ID_DP_HPD,
			hpd, hpdirq, 0);
	} else {
	/* need to check the F/W code. */
		VDO_MSG = (VDO_MESSAGE_Type *)&vdm_data[8];

		for (i = 0; i < 6; i++)
			msg_maxim("%s : VDO_%d : %d\n", __func__,
				i+1, VDO_MSG->VDO[i]);
	}

	return 0;
}

static int max77705_vdm_dp_configure(void *data, char *vdm_data, int len)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	UND_DATA_MSG_VDM_HEADER_Type *DATA_MSG_VDM = (UND_DATA_MSG_VDM_HEADER_Type *)&vdm_data[4];
	int timeleft = 0;

	msg_maxim("vendor_id = 0x%04x , svid_1 = 0x%04x", DATA_MSG_VDM->BITS.Standard_Vendor_ID, usbpd_data->SVID_1);
	if (usbpd_data->SVID_DP == TypeC_DP_SUPPORT) {
		timeleft = wait_event_interruptible_timeout(usbpd_data->device_add_wait_q,
				usbpd_data->device_add, HZ/2);
		msg_maxim("%s timeleft = %d\n", __func__, timeleft);
		max77705_ccic_event_work(usbpd_data, CCIC_NOTIFY_DEV_DP,
			CCIC_NOTIFY_ID_DP_LINK_CONF, usbpd_data->dp_selected_pin, 0, 0);
	}
	if (DATA_MSG_VDM->BITS.Standard_Vendor_ID == TypeC_DP_SUPPORT && usbpd_data->SVID_1 == TypeC_Dex_SUPPORT) {
		/* Samsung Discover Modes packet */
		usbpd_data->send_enter_mode_req = 0;
		max77705_vdm_process_set_Dex_Disover_mode_req(usbpd_data);
	}

	return 0;
}

void max77705_vdm_message_handler(struct max77705_usbc_platform_data *usbpd_data,
	char *opcode_data, int len)
{
	unsigned char vdm_data[OPCODE_DATA_LENGTH] = {0,};
	UND_DATA_MSG_ID_HEADER_Type *DATA_MSG_ID = NULL;
	UND_PRODUCT_VDO_Type *DATA_MSG_PRODUCT = NULL;
	UND_DATA_MSG_VDM_HEADER_Type vdm_header;

	memset(&vdm_header, 0, sizeof(UND_DATA_MSG_VDM_HEADER_Type));
	memcpy(vdm_data, opcode_data, len);
	memcpy(&vdm_header, &vdm_data[4], sizeof(vdm_header));
	if ((vdm_header.BITS.VDM_command_type) == SEC_UVDM_RESPONDER_NAK) {
		msg_maxim("IGNORE THE NAK RESPONSE !!![%d]", vdm_data[1]);
		return;
	}

	switch (vdm_data[1]) {
	case OPCODE_ID_VDM_DISCOVER_IDENTITY:
		max77705_vdm_process_printf("VDM_DISCOVER_IDENTITY", vdm_data, len);
		/* Message Type Definition */
		DATA_MSG_ID = (UND_DATA_MSG_ID_HEADER_Type *)&vdm_data[8];
		DATA_MSG_PRODUCT = (UND_PRODUCT_VDO_Type *)&vdm_data[16];
		usbpd_data->is_sent_pin_configuration = 0;
		usbpd_data->is_samsung_accessory_enter_mode = 0;
		usbpd_data->Vendor_ID = DATA_MSG_ID->BITS.USB_Vendor_ID;
		usbpd_data->Product_ID = DATA_MSG_PRODUCT->BITS.Product_ID;
		usbpd_data->Device_Version = DATA_MSG_PRODUCT->BITS.Device_Version;
		msg_maxim("Vendor_ID : 0x%X, Product_ID : 0x%X Device Version 0x%X",
			usbpd_data->Vendor_ID, usbpd_data->Product_ID, usbpd_data->Device_Version);
		if (max77705_process_check_accessory(usbpd_data))
			msg_maxim("Samsung Accessory Connected.");
	break;
	case OPCODE_ID_VDM_DISCOVER_SVIDS:
		max77705_vdm_process_printf("VDM_DISCOVER_SVIDS", vdm_data, len);
		max77705_vdm_process_discover_svids(usbpd_data, vdm_data, len);
	break;
	case OPCODE_ID_VDM_DISCOVER_MODES:
		max77705_vdm_process_printf("VDM_DISCOVER_MODES", vdm_data, len);
		vdm_data[0] = vdm_data[2];
		vdm_data[1] = vdm_data[3];
		max77705_vdm_process_discover_mode(usbpd_data, vdm_data, len);
	break;
	case OPCODE_ID_VDM_ENTER_MODE:
		max77705_vdm_process_printf("VDM_ENTER_MODE", vdm_data, len);
		max77705_vdm_process_enter_mode(usbpd_data, vdm_data, len);
	break;
	case OPCODE_ID_VDM_SVID_DP_STATUS:
		max77705_vdm_process_printf("VDM_SVID_DP_STATUS", vdm_data, len);
		vdm_data[0] = vdm_data[2];
		vdm_data[1] = vdm_data[3];
		max77705_vdm_dp_status_update(usbpd_data, vdm_data, len);
	break;
	case OPCODE_ID_VDM_SVID_DP_CONFIGURE:
		max77705_vdm_process_printf("VDM_SVID_DP_CONFIGURE", vdm_data, len);
		max77705_vdm_dp_configure(usbpd_data, vdm_data, len);
	break;
	case OPCODE_ID_VDM_ATTENTION:
		max77705_vdm_process_printf("VDM_ATTENTION", vdm_data, len);
		vdm_data[0] = vdm_data[2];
		vdm_data[1] = vdm_data[3];
		max77705_vdm_dp_attention(usbpd_data, vdm_data, len);
	break;
	case OPCODE_ID_VDM_EXIT_MODE:

	break;

	default:
		break;
	}
}

static int max77705_process_discover_identity(void *data)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	/* send the opcode */
	usbc_cmd_data write_data;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_VDM_DISCOVER_GET_VDM_RESP;
	write_data.write_data[0] = OPCODE_ID_VDM_DISCOVER_IDENTITY;
	write_data.write_length = 0x1;
	write_data.read_length = 31;
	max77705_usbc_opcode_write(usbpd_data, &write_data);
	return 0;
}

static int max77705_process_discover_svids(void *data)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	/* send the opcode */
	usbc_cmd_data write_data;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_VDM_DISCOVER_GET_VDM_RESP;
	write_data.write_data[0] = OPCODE_ID_VDM_DISCOVER_SVIDS;
	write_data.write_length = 0x1;
	write_data.read_length = 31;
	max77705_usbc_opcode_write(usbpd_data, &write_data);
	return 0;
}

static int max77705_process_discover_modes(void *data)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	/* send the opcode */
	usbc_cmd_data write_data;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_VDM_DISCOVER_GET_VDM_RESP;
	write_data.write_data[0] = OPCODE_ID_VDM_DISCOVER_MODES;
	write_data.write_length = 0x1;
	write_data.read_length = 11;
	max77705_usbc_opcode_write(usbpd_data, &write_data);
	return 0;
}

static int max77705_process_enter_mode(void *data)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	/* send the opcode */
	usbc_cmd_data write_data;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_VDM_DISCOVER_GET_VDM_RESP;
	write_data.write_data[0] = OPCODE_ID_VDM_ENTER_MODE;
	write_data.write_length = 0x1;
	write_data.read_length = 7;
	max77705_usbc_opcode_write(usbpd_data, &write_data);
	return 0;
}

static int max77705_process_attention(void *data)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	/* send the opcode */
	usbc_cmd_data write_data;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_VDM_DISCOVER_GET_VDM_RESP;
	write_data.write_data[0] = OPCODE_ID_VDM_ATTENTION;
	write_data.write_length = 0x1;
	write_data.read_length = 11;
	max77705_usbc_opcode_write(usbpd_data, &write_data);
	return 0;
}

static int max77705_process_dp_status_update(void *data)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	/* send the opcode */
	usbc_cmd_data write_data;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_VDM_DISCOVER_GET_VDM_RESP;
	write_data.write_data[0] = OPCODE_ID_VDM_SVID_DP_STATUS;
	write_data.write_length = 0x1;
	write_data.read_length = 11;
	max77705_usbc_opcode_write(usbpd_data, &write_data);
	return 0;
}

static int max77705_process_dp_configure(void *data)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	/* send the opcode */
	usbc_cmd_data write_data;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_VDM_DISCOVER_GET_VDM_RESP;
	write_data.write_data[0] = OPCODE_ID_VDM_SVID_DP_CONFIGURE;
	write_data.write_length = 0x1;
	write_data.read_length = 11;
	max77705_usbc_opcode_write(usbpd_data, &write_data);
	return 0;
}

static void max77705_process_alternate_mode(void *data)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	uint32_t mode = usbpd_data->alternate_state;
	int	ret = 0;
#if defined(CONFIG_USB_HOST_NOTIFY)
	struct otg_notify *o_notify = get_otg_notify();
#endif

	if (mode) {
		msg_maxim("mode : 0x%x", mode);
#if defined(CONFIG_USB_HOST_NOTIFY)
		if (is_blocked(o_notify, NOTIFY_BLOCK_TYPE_HOST)) {
			msg_maxim("host is blocked, skip all the alternate mode.");
			goto process_error;
		}
#endif
		if (usbpd_data->mdm_block) {
			msg_maxim("is blocked by mdm, skip all the alternate mode.");
			goto process_error;
		}

		if (mode & VDM_DISCOVER_ID)
			ret = max77705_process_discover_identity(usbpd_data);
		if (ret)
			goto process_error;
		if (mode & VDM_DISCOVER_SVIDS)
			ret = max77705_process_discover_svids(usbpd_data);
		if (ret)
			goto process_error;
		if (mode & VDM_DISCOVER_MODES)
			ret = max77705_process_discover_modes(usbpd_data);
		if (ret)
			goto process_error;
		if (mode & VDM_ENTER_MODE)
			ret = max77705_process_enter_mode(usbpd_data);
		if (ret)
			goto process_error;
		if (mode & VDM_DP_STATUS_UPDATE)
			ret = max77705_process_dp_status_update(usbpd_data);
		if (ret)
			goto process_error;
		if (mode & VDM_DP_CONFIGURE)
			ret = max77705_process_dp_configure(usbpd_data);
		if (ret)
			goto process_error;
		if (mode & VDM_ATTENTION)
			ret = max77705_process_attention(usbpd_data);
process_error:
		usbpd_data->alternate_state = 0;
	}
}

void max77705_receive_alternate_message(struct max77705_usbc_platform_data *data, MAX77705_VDM_MSG_IRQ_STATUS_Type *VDM_MSG_IRQ_State)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	static int last_alternate = 0;

DISCOVER_ID:
	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Discover_ID) {
		msg_maxim(": %s", &VDM_MSG_IRQ_State_Print[1][0]);
		usbpd_data->alternate_state |= VDM_DISCOVER_ID;
		last_alternate = VDM_DISCOVER_ID;
	}

DISCOVER_SVIDS:
	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Discover_SVIDs) {
		msg_maxim(": %s", &VDM_MSG_IRQ_State_Print[2][0]);
		if (last_alternate != VDM_DISCOVER_ID) {
			msg_maxim("%s vdm miss\n", __func__);
			VDM_MSG_IRQ_State->BITS.Vdm_Flag_Discover_ID = 1;
			goto DISCOVER_ID;
		}
		usbpd_data->alternate_state |= VDM_DISCOVER_SVIDS;
		last_alternate = VDM_DISCOVER_SVIDS;
	}

DISCOVER_MODES:
	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Discover_MODEs) {
		msg_maxim(": %s", &VDM_MSG_IRQ_State_Print[3][0]);
		if (last_alternate != VDM_DISCOVER_SVIDS &&
				last_alternate != VDM_DP_CONFIGURE) {
			msg_maxim("%s vdm miss\n", __func__);
			VDM_MSG_IRQ_State->BITS.Vdm_Flag_Discover_SVIDs = 1;
			goto DISCOVER_SVIDS;
		}
		usbpd_data->alternate_state |= VDM_DISCOVER_MODES;
		last_alternate = VDM_DISCOVER_MODES;
	}

	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Enter_Mode) {
		msg_maxim(": %s", &VDM_MSG_IRQ_State_Print[4][0]);
		if (last_alternate != VDM_DISCOVER_MODES) {
			msg_maxim("%s vdm miss\n", __func__);
			VDM_MSG_IRQ_State->BITS.Vdm_Flag_Discover_MODEs = 1;
			goto DISCOVER_MODES;
		}
		usbpd_data->alternate_state |= VDM_ENTER_MODE;
		last_alternate = VDM_ENTER_MODE;
	}
	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Exit_Mode) {
		msg_maxim(": %s", &VDM_MSG_IRQ_State_Print[5][0]);
		usbpd_data->alternate_state |= VDM_EXIT_MODE;
	}
	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Attention) {
		msg_maxim(": %s", &VDM_MSG_IRQ_State_Print[6][0]);
		usbpd_data->alternate_state |= VDM_ATTENTION;
	}
	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_DP_Status_Update) {
		msg_maxim(": %s", &VDM_MSG_IRQ_State_Print[7][0]);
		usbpd_data->alternate_state |= VDM_DP_STATUS_UPDATE;
		last_alternate = VDM_DP_STATUS_UPDATE;
	}
	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_DP_Configure) {
		msg_maxim(": %s", &VDM_MSG_IRQ_State_Print[8][0]);
		usbpd_data->alternate_state |= VDM_DP_CONFIGURE;
		last_alternate = VDM_DP_CONFIGURE;
	}

	max77705_process_alternate_mode(usbpd_data);
}

void max77705_send_dex_fan_unstructured_vdm_message(void *data, int cmd)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	uint8_t SendMSG[32] = {0,};
	uint32_t message = (uint32_t)cmd;
	usbc_cmd_data write_data;
	int len = sizeof(SS_DEX_UNSTRUCTURED_VDM);
	VDO_MESSAGE_Type *VDO_MSG = (VDO_MESSAGE_Type *)&SendMSG[5];

	SS_DEX_UNSTRUCTURED_VDM.byte_data.BITS.Num_Of_VDO = 2;
	SS_DEX_UNSTRUCTURED_VDM.byte_data.BITS.Cmd_Type = ACK;
	SS_DEX_UNSTRUCTURED_VDM.byte_data.BITS.Reserved = 0;
	SS_DEX_UNSTRUCTURED_VDM.VDO_MSG.VDO[0] = 0x04E80000;
	SS_DEX_UNSTRUCTURED_VDM.VDO_MSG.VDO[1] = 0xA0200110;
	pr_info("%s: cmd : 0x%x\n", __func__, cmd);
	memcpy(SendMSG, &SS_DEX_UNSTRUCTURED_VDM, len);

	VDO_MSG->VDO[0] = message;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_VDM_DISCOVER_SET_VDM_REQ;

	memcpy(write_data.write_data, SendMSG, sizeof(SendMSG));
	write_data.write_length = len;
	write_data.read_length = 31;
	max77705_usbc_opcode_write(usbpd_data, &write_data);
	msg_maxim("message : 0x%x", message);
}

void max77705_acc_detach_check(struct work_struct *wk)
{
	struct delayed_work *delay_work =
		container_of(wk, struct delayed_work, work);
	struct max77705_usbc_platform_data *usbpd_data =
		container_of(delay_work, struct max77705_usbc_platform_data, acc_detach_work);

	pr_info("%s : pd_state : %d, acc_type : %d\n", __func__,
		usbpd_data->pd_state, usbpd_data->acc_type);
	if (usbpd_data->pd_state == max77705_State_PE_Initial_detach) {
		if (usbpd_data->acc_type != CCIC_DOCK_DETACHED) {
			if (usbpd_data->acc_type != CCIC_DOCK_NEW)
				ccic_send_dock_intent(CCIC_DOCK_DETACHED);
			ccic_send_dock_uevent(usbpd_data->Vendor_ID,
					usbpd_data->Product_ID,
					CCIC_DOCK_DETACHED);
			usbpd_data->acc_type = CCIC_DOCK_DETACHED;
			usbpd_data->Vendor_ID = 0;
			usbpd_data->Product_ID = 0;
			usbpd_data->send_enter_mode_req = 0;
		}
	}
}

void max77705_vdm_process_set_samsung_alternate_mode(void *data, int mode)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	usbc_cmd_data write_data;

	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_SET_ALTERNATEMODE;
	write_data.write_data[0] = mode;
	write_data.write_length = 0x1;
	write_data.read_length = 0x1;
	max77705_usbc_opcode_write(usbpd_data, &write_data);
}

void max77705_set_enable_alternate_mode(int mode)
{
	struct max77705_usbc_platform_data *usbpd_data = NULL;
	static int check_is_driver_loaded;
	static int prev_alternate_mode;
	int is_first_booting = 0;
	struct max77705_pd_data *pd_data = NULL;
	u8 status[11] = {0, };

	usbpd_data = g_usbc_data;

	if (!usbpd_data)
		return;
	is_first_booting = usbpd_data->is_first_booting;
	pd_data = usbpd_data->pd_data;

	msg_maxim("is_first_booting  : %x mode %x",
			usbpd_data->is_first_booting, mode);
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	store_usblog_notify(NOTIFY_ALTERNATEMODE, (void *)&mode, NULL);
#endif
	usbpd_data->set_altmode = mode;

	if ((mode & ALTERNATE_MODE_NOT_READY) &&
	    (mode & ALTERNATE_MODE_READY)) {
		msg_maxim("mode is invalid!");
		return;
	}
	if ((mode & ALTERNATE_MODE_START) && (mode & ALTERNATE_MODE_STOP)) {
		msg_maxim("mode is invalid!");
		return;
	}
	if (mode & ALTERNATE_MODE_RESET) {
		msg_maxim("mode is reset! check_is_driver_loaded=%d, prev_alternate_mode=%d",
			check_is_driver_loaded, prev_alternate_mode);
		if (check_is_driver_loaded &&
		    (prev_alternate_mode == ALTERNATE_MODE_START)) {

			msg_maxim("[No process] alternate mode is reset as start!");
			prev_alternate_mode = ALTERNATE_MODE_START;
		} else if (check_is_driver_loaded &&
			   (prev_alternate_mode == ALTERNATE_MODE_STOP)) {
			msg_maxim("[No process] alternate mode is reset as stop!");
			prev_alternate_mode = ALTERNATE_MODE_STOP;
		} else {
			;
		}
	} else {
		if (mode & ALTERNATE_MODE_NOT_READY) {
			check_is_driver_loaded = 0;
			msg_maxim("alternate mode is not ready!");
		} else if (mode & ALTERNATE_MODE_READY) {
			check_is_driver_loaded = 1;
			msg_maxim("alternate mode is ready!");
		} else {
			;
		}

		if (check_is_driver_loaded) {
			switch (is_first_booting) {
			case 0: /*this routine is calling after complete a booting.*/
				if (mode & ALTERNATE_MODE_START) {
					max77705_vdm_process_set_samsung_alternate_mode(usbpd_data,
						MAXIM_ENABLE_ALTERNATE_SRC_VDM);
					msg_maxim("[NO BOOTING TIME] !!!alternate mode is started!");
					if (usbpd_data->cc_data->current_pr == SNK && (pd_data->current_dr == DFP)) {
						max77705_vdm_process_set_identity_req(usbpd_data);
						msg_maxim("[NO BOOTING TIME] SEND THE PACKET (DEX HUB) ");
					}

				} else if (mode & ALTERNATE_MODE_STOP) {
					max77705_vdm_process_set_samsung_alternate_mode(usbpd_data,
						MAXIM_ENABLE_ALTERNATE_SRCCAP);
					msg_maxim("[NO BOOTING TIME] alternate mode is stopped!");
				}

				break;
			case 1:
				if (mode & ALTERNATE_MODE_START) {
					msg_maxim("[ON BOOTING TIME] !!!alternate mode is started!");
					prev_alternate_mode = ALTERNATE_MODE_START;
					max77705_vdm_process_set_samsung_alternate_mode(usbpd_data,
						MAXIM_ENABLE_ALTERNATE_SRC_VDM);
					msg_maxim("!![ON BOOTING TIME] SEND THE PACKET REGARDING IN CASE OF VR/DP ");
					/* FOR THE DEX FUNCTION. */
					max77705_read_reg(usbpd_data->muic, MAX77705_USBC_REG_USBC_STATUS1, &status[0]);
					max77705_read_reg(usbpd_data->muic, MAX77705_USBC_REG_USBC_STATUS2, &status[1]);
					max77705_read_reg(usbpd_data->muic, MAX77705_USBC_REG_BC_STATUS, &status[2]);
					max77705_read_reg(usbpd_data->muic, MAX77705_USBC_REG_CC_STATUS0, &status[3]);
					max77705_read_reg(usbpd_data->muic, MAX77705_USBC_REG_CC_STATUS1, &status[4]);
					max77705_read_reg(usbpd_data->muic, MAX77705_USBC_REG_PD_STATUS0, &status[5]);
					max77705_read_reg(usbpd_data->muic, MAX77705_USBC_REG_PD_STATUS1, &status[6]);
					max77705_read_reg(usbpd_data->muic, MAX77705_USBC_REG_UIC_INT_M, &status[7]);
					max77705_read_reg(usbpd_data->muic, MAX77705_USBC_REG_CC_INT_M, &status[8]);
					max77705_read_reg(usbpd_data->muic, MAX77705_USBC_REG_PD_INT_M, &status[9]);
					max77705_read_reg(usbpd_data->muic, MAX77705_USBC_REG_VDM_INT_M, &status[10]);
					msg_maxim("USBC1:0x%02x, USBC2:0x%02x, BC:0x%02x",
						status[0], status[1], status[2]);
					msg_maxim("CC_STATUS0:0x%x, CC_STATUS1:0x%x, PD_STATUS0:0x%x, PD_STATUS1:0x%x",
						status[3], status[4], status[5], status[6]);
					msg_maxim("UIC_INT_M:0x%x, CC_INT_M:0x%x, PD_INT_M:0x%x, VDM_INT_M:0x%x",
						status[7], status[8], status[9], status[10]);
					if (usbpd_data->cc_data->current_pr == SNK && (pd_data->current_dr == DFP)
						&& usbpd_data->is_first_booting) {
						max77705_vdm_process_set_identity_req(usbpd_data);
						msg_maxim("[ON BOOTING TIME] SEND THE PACKET (DEX HUB)  ");
					}
					max77705_write_reg(usbpd_data->muic, REG_VDM_INT_M, 0xF0);
					max77705_write_reg(usbpd_data->muic, REG_PD_INT_M, 0x0);
					max77705_read_reg(usbpd_data->muic, MAX77705_USBC_REG_PD_INT_M, &status[9]);
					max77705_read_reg(usbpd_data->muic, MAX77705_USBC_REG_VDM_INT_M, &status[10]);
					msg_maxim("UIC_INT_M:0x%x, CC_INT_M:0x%x, PD_INT_M:0x%x, VDM_INT_M:0x%x",
						status[7], status[8], status[9], status[10]);
					usbpd_data->is_first_booting = 0;
				} else if (mode & ALTERNATE_MODE_STOP) {
					msg_maxim("[ON BOOTING TIME] alternate mode is stopped!");
				}
				break;

			default:
				msg_maxim("Never calling");
				msg_maxim("[Never calling] is_first_booting [ %d]", is_first_booting);
				break;

			}
		}
	}
}

void max77705_set_host_turn_on_event(int mode)
{
	struct max77705_usbc_platform_data *usbpd_data = NULL;

	usbpd_data = g_usbc_data;

	if (!usbpd_data)
		return;

	pr_info("%s : current_set is %d!\n", __func__, mode);
	if (mode) {
		usbpd_data->device_add = 0;
		usbpd_data->detach_done_wait = 0;
		usbpd_data->host_turn_on_event = 1;
		wake_up_interruptible(&usbpd_data->host_turn_on_wait_q);
	} else {
		usbpd_data->device_add = 0;
		usbpd_data->detach_done_wait = 0;
		usbpd_data->host_turn_on_event = 0;
	}
}

static void set_endian(char *src, char *dest, int size)
{
	int i, j;
	int loop;
	int dest_pos;
	int src_pos;

	loop = size / SAMSUNGUVDM_ALIGN;
	loop += (((size % SAMSUNGUVDM_ALIGN) > 0) ? 1:0);

	for (i = 0 ; i < loop ; i++)
		for (j = 0 ; j < SAMSUNGUVDM_ALIGN ; j++) {
			src_pos = SAMSUNGUVDM_ALIGN * i + j;
			dest_pos = SAMSUNGUVDM_ALIGN * i + SAMSUNGUVDM_ALIGN - j - 1;
			dest[dest_pos] = src[src_pos];
		}
}

static int get_checksum(char *data, int start_addr, int size)
{
	int checksum = 0;
	int i;
	//	printk("[MAX77705] %s", __func__);

	for (i = 0; i < size; i++) {
		checksum += data[start_addr+i];
		//printk(" %x", (uint32_t)data[start_addr+i]);
	}
	//	printk("\n");
	return checksum;
}

static int set_uvdmset_count(int size)
{
	int ret = 0;

	if (size <= SAMSUNGUVDM_MAXDATA_FIRST_UVDMSET)
		ret = 1;
	else {
		ret = ((size-SAMSUNGUVDM_MAXDATA_FIRST_UVDMSET) / SAMSUNGUVDM_MAXDATA_NORMAL_UVDMSET);
		if (((size-SAMSUNGUVDM_MAXDATA_FIRST_UVDMSET) % SAMSUNGUVDM_MAXDATA_NORMAL_UVDMSET) == 0)
			ret += 1;
		else
			ret += 2;
	}

	return ret;
}

static int get_datasize_of_currentset(int first_set, int remained_data_size)
{
	int ret = 0;

	if (first_set)
		ret = (remained_data_size <= SAMSUNGUVDM_MAXDATA_FIRST_UVDMSET) ?
			remained_data_size : SAMSUNGUVDM_MAXDATA_FIRST_UVDMSET;
	else
		ret = (remained_data_size <= SAMSUNGUVDM_MAXDATA_NORMAL_UVDMSET) ?
			remained_data_size : SAMSUNGUVDM_MAXDATA_NORMAL_UVDMSET;

	return ret;
}

static void set_sec_uvdm_txdataheader(void *data, int first_set, int cur_uvdmset,
		int total_data_size, int remained_data_size)
{
	U_SEC_TX_DATA_HEADER *SEC_TX_DATA_HEADER;
	uint8_t *SendMSG = (uint8_t *)data;

	if (first_set)
		SEC_TX_DATA_HEADER = (U_SEC_TX_DATA_HEADER *)&SendMSG[12];
	else
		SEC_TX_DATA_HEADER = (U_SEC_TX_DATA_HEADER *)&SendMSG[8];

	SEC_TX_DATA_HEADER->BITS.data_size_of_current_set =
		get_datasize_of_currentset(first_set, remained_data_size);
	SEC_TX_DATA_HEADER->BITS.total_data_size = total_data_size;
	SEC_TX_DATA_HEADER->BITS.order_of_current_uvdm_set = cur_uvdmset;
	SEC_TX_DATA_HEADER->BITS.reserved = 0;
}

static void set_msgheader(void *data, int msg_type, int obj_num)
{
	/* Common : Fill the VDM OpCode MSGHeader */
	SEND_VDM_BYTE_DATA *MSG_HDR;
	uint8_t *SendMSG = (uint8_t *)data;

	MSG_HDR = (SEND_VDM_BYTE_DATA *)&SendMSG[0];
	MSG_HDR->BITS.Num_Of_VDO = obj_num;
	if (msg_type == NAK)
		MSG_HDR->BITS.Reserved = 7;
	MSG_HDR->BITS.Cmd_Type = msg_type;
}

static void set_uvdmheader(void *data, int vendor_id, int vdm_type)
{
	/* Common : Fill the UVDMHeader */
	UND_UNSTRUCTURED_VDM_HEADER_Type *UVDM_HEADER;
	U_DATA_MSG_VDM_HEADER_Type *VDM_HEADER;
	uint8_t *SendMSG = (uint8_t *)data;

	UVDM_HEADER = (UND_UNSTRUCTURED_VDM_HEADER_Type *)&SendMSG[4];
	UVDM_HEADER->BITS.USB_Vendor_ID = vendor_id;
	UVDM_HEADER->BITS.VDM_TYPE = vdm_type;
	UVDM_HEADER->BITS.VENDOR_DEFINED_MESSAGE = SEC_UVDM_UNSTRUCTURED_VDM;
	VDM_HEADER = (U_DATA_MSG_VDM_HEADER_Type *)&SendMSG[4];
	VDM_HEADER->BITS.VDM_command = 4; //from s2mm005 concept
}

static void set_sec_uvdmheader(void *data, int pid, bool data_type, int cmd_type,
		bool dir, int total_uvdmset_num, uint8_t received_data)
{
	U_SEC_UVDM_HEADER *SEC_VDM_HEADER;
	uint8_t *SendMSG = (uint8_t *)data;

	SEC_VDM_HEADER = (U_SEC_UVDM_HEADER *)&SendMSG[8];
	SEC_VDM_HEADER->BITS.pid = pid;
	SEC_VDM_HEADER->BITS.data_type = data_type;
	SEC_VDM_HEADER->BITS.command_type = cmd_type;
	SEC_VDM_HEADER->BITS.direction = dir;
	if (dir == DIR_OUT)
		SEC_VDM_HEADER->BITS.total_number_of_uvdm_set = total_uvdmset_num;
	if (data_type == TYPE_SHORT)
		SEC_VDM_HEADER->BITS.data = received_data;
	else
		SEC_VDM_HEADER->BITS.data = 0;
#if 0
	msg_maxim("pid = 0x%x  data_type=%d ,cmd_type =%d,direction= %d, total_num_of_uvdm_set = %d",
		SEC_VDM_HEADER->BITS.pid,
		SEC_VDM_HEADER->BITS.data_type,
		SEC_VDM_HEADER->BITS.command_type,
		SEC_VDM_HEADER->BITS.direction,
		SEC_VDM_HEADER->BITS.total_number_of_uvdm_set);
#endif
}

static void set_sec_uvdm_txdata_tailer(void *data)
{
	U_SEC_TX_DATA_TAILER *SEC_TX_DATA_TAILER;
	uint8_t *SendMSG = (uint8_t *)data;

	SEC_TX_DATA_TAILER = (U_SEC_TX_DATA_TAILER *)&SendMSG[28];
	SEC_TX_DATA_TAILER->BITS.checksum = get_checksum(SendMSG, 8, SAMSUNGUVDM_CHECKSUM_DATA_COUNT);
	SEC_TX_DATA_TAILER->BITS.reserved = 0;
}

static void set_sec_uvdm_rxdata_header(void *data, int cur_uvdmset_num, int cur_uvdmset_data, int ack)
{
	U_SEC_RX_DATA_HEADER *SEC_UVDM_RX_HEADER;
	uint8_t *SendMSG = (uint8_t *)data;

	SEC_UVDM_RX_HEADER = (U_SEC_RX_DATA_HEADER *)&SendMSG[8];
	SEC_UVDM_RX_HEADER->BITS.order_of_current_uvdm_set = cur_uvdmset_num;
	SEC_UVDM_RX_HEADER->BITS.received_data_size_of_current_set = cur_uvdmset_data;
	SEC_UVDM_RX_HEADER->BITS.result_value = ack;
	SEC_UVDM_RX_HEADER->BITS.reserved = 0;
}

static int check_is_wait_ack_accessroy(int vid, int pid, int svid)
{
	int should_wait = true;

	if (vid == SAMSUNG_VENDOR_ID && pid == DEXDOCK_PRODUCT_ID && svid == TypeC_DP_SUPPORT) {
		msg_maxim("no need to wait ack response!");
		should_wait = false;
	}
	return should_wait;
}

static int max77705_send_vdm_write_message(void *data)
{
	struct SS_UNSTRUCTURED_VDM_MSG vdm_opcode_msg;
	struct max77705_usbc_platform_data *usbpd_data = g_usbc_data;
	uint8_t *SendMSG = (uint8_t *)data;
	usbc_cmd_data write_data;
	int len = sizeof(struct SS_UNSTRUCTURED_VDM_MSG);

	memset(&vdm_opcode_msg, 0, len);
	/* Common : MSGHeader */
	memcpy(&vdm_opcode_msg.byte_data, &SendMSG[0], 1);
	/* 8. Copy the data from SendMSG buffer */
	memcpy(&vdm_opcode_msg.VDO_MSG.VDO[0], &SendMSG[4], 28);
	/* 9. Write SendMSG buffer via I2C */
	init_usbc_cmd_data(&write_data);
	write_data.opcode = OPCODE_VDM_DISCOVER_SET_VDM_REQ;
	write_data.is_uvdm = 1;
	memcpy(write_data.write_data, &vdm_opcode_msg, sizeof(vdm_opcode_msg));
	write_data.write_length = len;
	write_data.read_length = len;
	max77705_usbc_opcode_write(usbpd_data, &write_data);
	msg_maxim("opcode is sent");
	return 0;
}

static int max77705_send_sec_unstructured_short_vdm_message(void *data, void *buf, size_t size)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	uint8_t SendMSG[32] = {0,};
	/* Message Type Definition */
	uint8_t received_data = 0;
	int time_left;
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	int event;
#endif

	if ((buf == NULL) || size <= 0) {
		msg_maxim("given data is not valid !");
		return -EINVAL;
	}

	if (!usbpd_data->is_samsung_accessory_enter_mode) {
		msg_maxim("samsung_accessory mode is not ready!");
		return -ENXIO;
	}
	/* 1. Calc the receivced data size and determin the uvdm set count and last data of uvdm set. */
	received_data = *(char *)buf;
	/* 2. Common : Fill the MSGHeader */
	set_msgheader(SendMSG, ACK, 2);
	/* 3. Common : Fill the UVDMHeader*/
	set_uvdmheader(SendMSG, SAMSUNG_VENDOR_ID, 0);
	/* 4. Common : Fill the First SEC_VDMHeader*/
	set_sec_uvdmheader(SendMSG, usbpd_data->Product_ID, TYPE_SHORT,	SEC_UVDM_ININIATOR, DIR_OUT, 1, received_data);
	usbpd_data->is_in_first_sec_uvdm_req = true;

	usbpd_data->uvdm_error = 0;
	max77705_send_vdm_write_message(SendMSG);

	if (check_is_wait_ack_accessroy(usbpd_data->Vendor_ID, usbpd_data->Product_ID, usbpd_data->SVID_DP)) {
		reinit_completion(&usbpd_data->uvdm_longpacket_out_wait);
		/* Wait Response*/
		time_left =
			wait_for_completion_interruptible_timeout(&usbpd_data->uvdm_longpacket_out_wait,
							  msecs_to_jiffies(SAMSUNGUVDM_WAIT_MS));
		if (time_left <= 0) {
			usbpd_data->is_in_first_sec_uvdm_req = false;
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
			event = NOTIFY_EXTRA_UVDM_TIMEOUT;
			store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#endif
			return -ETIME;
		}
		if (usbpd_data->uvdm_error) {
			usbpd_data->is_in_first_sec_uvdm_req = false;
			return usbpd_data->uvdm_error;
		}
	}
	msg_maxim("exit : short data transfer complete!");
	usbpd_data->is_in_first_sec_uvdm_req = false;
	return size;
}

static int max77705_send_sec_unstructured_long_vdm_message(void *data, void *buf, size_t size)
{
	struct max77705_usbc_platform_data *usbpd_data;
	uint8_t SendMSG[32] = {0,};
	uint8_t *SEC_DATA;
	int need_uvdmset_count = 0;
	int cur_uvdmset_data = 0;
	int cur_uvdmset_num = 0;
	int accumulated_data_size = 0;
	int remained_data_size = 0;
	uint8_t received_data[MAX_INPUT_DATA] = {0,};
	int time_left;
	int i;
	int received_data_index;
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	int event;
#endif

	usbpd_data = data;
	if (!usbpd_data)
		return -ENXIO;

	if (!buf) {
		msg_maxim("given data is not valid !");
		return -EINVAL;
	}

	/* 1. Calc the receivced data size and determin the uvdm set count and last data of uvdm set. */
	set_endian(buf, received_data, size);
	need_uvdmset_count = set_uvdmset_count(size);
	msg_maxim("need_uvdmset_count = %d", need_uvdmset_count);
	usbpd_data->is_in_first_sec_uvdm_req = true;
	usbpd_data->is_in_sec_uvdm_out = DIR_OUT;
	cur_uvdmset_num = 1;
	accumulated_data_size = 0;
	remained_data_size = size;
	received_data_index = 0;
	/* 2. Common : Fill the MSGHeader */
	set_msgheader(SendMSG, ACK, 7);
	/* 3. Common : Fill the UVDMHeader*/
	set_uvdmheader(SendMSG, SAMSUNG_VENDOR_ID, 0);
	/* 4. Common : Fill the First SEC_VDMHeader*/
	if (usbpd_data->is_in_first_sec_uvdm_req)
		set_sec_uvdmheader(SendMSG, usbpd_data->Product_ID,
						TYPE_LONG, SEC_UVDM_ININIATOR, DIR_OUT, need_uvdmset_count, 0);

	while (cur_uvdmset_num <= need_uvdmset_count) {
		cur_uvdmset_data = 0;
		time_left = 0;

		set_sec_uvdm_txdataheader(SendMSG, usbpd_data->is_in_first_sec_uvdm_req,
				cur_uvdmset_num, size, remained_data_size);

		cur_uvdmset_data = get_datasize_of_currentset(usbpd_data->is_in_first_sec_uvdm_req, remained_data_size);
		msg_maxim("data_size_of_current_set = %d ,total_data_size = %ld, order_of_current_uvdm_set = %d",
			cur_uvdmset_data, size, cur_uvdmset_num);
		/* 6. Common : Fill the DATA */
		if (usbpd_data->is_in_first_sec_uvdm_req) {
			SEC_DATA = (uint8_t *)&SendMSG[8+8];
			for (i = 0; i < SAMSUNGUVDM_MAXDATA_FIRST_UVDMSET; i++)
				SEC_DATA[i] = received_data[received_data_index++];
		} else {
			SEC_DATA = (uint8_t *)&SendMSG[8+4];
			for (i = 0; i < SAMSUNGUVDM_MAXDATA_NORMAL_UVDMSET; i++)
				SEC_DATA[i] = received_data[received_data_index++];
		}
		/* 7. Common : Fill the TX_DATA_Tailer */
		set_sec_uvdm_txdata_tailer(SendMSG);
		/* 8. Send data to PDIC */
		usbpd_data->uvdm_error = 0;
		max77705_send_vdm_write_message(SendMSG);
		/* 9. Wait Response*/
		reinit_completion(&usbpd_data->uvdm_longpacket_out_wait);
		time_left =
			wait_for_completion_interruptible_timeout(&usbpd_data->uvdm_longpacket_out_wait,
							  msecs_to_jiffies(SAMSUNGUVDM_WAIT_MS));
		if (time_left <= 0) {
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
			event = NOTIFY_EXTRA_UVDM_TIMEOUT;
			store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#endif
			return -ETIME;
		}

		if (usbpd_data->uvdm_error)
			return usbpd_data->uvdm_error;

		accumulated_data_size += cur_uvdmset_data;
		remained_data_size -= cur_uvdmset_data;
		if (usbpd_data->is_in_first_sec_uvdm_req)
			usbpd_data->is_in_first_sec_uvdm_req = false;
		cur_uvdmset_num++;
	}
	return size;
}

void max77705_sec_unstructured_message_handler(struct max77705_usbc_platform_data *usbpd_data,
		char *opcode_data, int len)
{
	int unstructured_len = sizeof(struct SS_UNSTRUCTURED_VDM_MSG);
	uint8_t ReadMSG[32] = {0,};
	U_SEC_UVDM_RESPONSE_HEADER *SEC_UVDM_RESPONSE_HEADER;
	U_SEC_RX_DATA_HEADER *SEC_UVDM_RX_HEADER;
	U_SEC_TX_DATA_HEADER *SEC_UVDM_TX_HEADER;

	if (len != unstructured_len + OPCODE_SIZE) {
		msg_maxim("This isn't UVDM message!");
		return;
	}

	memcpy(ReadMSG, opcode_data, OPCODE_DATA_LENGTH);
	usbpd_data->uvdm_error = 0;

	switch (usbpd_data->is_in_sec_uvdm_out) {
	case DIR_OUT:
		if (usbpd_data->is_in_first_sec_uvdm_req) {
			SEC_UVDM_RESPONSE_HEADER = (U_SEC_UVDM_RESPONSE_HEADER *)&ReadMSG[6];
#if (UVDM_DEBUG)
			msg_maxim("DIR_OUT SEC_UVDM_RESPONSE_HEADER : 0x%x", SEC_UVDM_RESPONSE_HEADER->data);
#endif
			if (SEC_UVDM_RESPONSE_HEADER->BITS.data_type == TYPE_LONG) {
				if (SEC_UVDM_RESPONSE_HEADER->BITS.command_type == SEC_UVDM_RESPONDER_ACK) {
					SEC_UVDM_RX_HEADER = (U_SEC_RX_DATA_HEADER *)&ReadMSG[10];
#if (UVDM_DEBUG)
					msg_maxim("DIR_OUT SEC_UVDM_RX_HEADER : 0x%x", SEC_UVDM_RX_HEADER->data);
#endif
					if (SEC_UVDM_RX_HEADER->BITS.result_value == RX_ACK) {
						/* do nothing */
					} else if (SEC_UVDM_RX_HEADER->BITS.result_value == RX_NAK) {
						msg_maxim("DIR_OUT RX_NAK received");
						usbpd_data->uvdm_error = -ENODATA;
					} else if (SEC_UVDM_RX_HEADER->BITS.result_value == RX_BUSY) {
						msg_maxim("DIR_OUT RX_BUSY received");
						usbpd_data->uvdm_error = -EBUSY;
					} else {
						msg_maxim("DIR_OUT Undefined RX value");
						usbpd_data->uvdm_error = -EPROTO;
					}
				} else if (SEC_UVDM_RESPONSE_HEADER->BITS.command_type == SEC_UVDM_RESPONDER_NAK) {
					msg_maxim("DIR_OUT SEC_UVDM_RESPONDER_NAK received");
					usbpd_data->uvdm_error = -ENODATA;
				} else if (SEC_UVDM_RESPONSE_HEADER->BITS.command_type == SEC_UVDM_RESPONDER_BUSY) {
					msg_maxim("DIR_OUT SEC_UVDM_RESPONDER_BUSY received");
					usbpd_data->uvdm_error = -EBUSY;
				} else {
					msg_maxim("DIR_OUT Undefined RESPONDER value");
					usbpd_data->uvdm_error = -EPROTO;
				}
			} else { /* TYPE_SHORT */
				if (SEC_UVDM_RESPONSE_HEADER->BITS.command_type == SEC_UVDM_RESPONDER_ACK) {
					/* do nothing */
				} else if (SEC_UVDM_RESPONSE_HEADER->BITS.command_type == SEC_UVDM_RESPONDER_NAK) {
					msg_maxim("DIR_OUT SHORT SEC_UVDM_RESPONDER_NAK received");
					usbpd_data->uvdm_error = -ENODATA;
				} else if (SEC_UVDM_RESPONSE_HEADER->BITS.command_type == SEC_UVDM_RESPONDER_BUSY) {
					msg_maxim("DIR_OUT SHORT SEC_UVDM_RESPONDER_BUSY received");
					usbpd_data->uvdm_error = -EBUSY;
				} else {
					msg_maxim("DIR_OUT Undefined RESPONDER value");
					usbpd_data->uvdm_error = -EPROTO;
				}
			}
		} else { /* after 2nd packet for TYPE_LONG */
			SEC_UVDM_RX_HEADER = (U_SEC_RX_DATA_HEADER *)&ReadMSG[6];
			if (SEC_UVDM_RX_HEADER->BITS.result_value == RX_ACK) {
				/* do nothing */
			} else if (SEC_UVDM_RX_HEADER->BITS.result_value == RX_NAK) {
				msg_maxim("DIR_OUT RX_NAK received");
				usbpd_data->uvdm_error = -ENODATA;
			} else if (SEC_UVDM_RX_HEADER->BITS.result_value == RX_BUSY) {
				msg_maxim("DIR_OUT RX_BUSY received");
				usbpd_data->uvdm_error = -EBUSY;
			} else {
				msg_maxim("DIR_OUT Undefined RX value");
				usbpd_data->uvdm_error = -EPROTO;
			}
		}
		complete(&usbpd_data->uvdm_longpacket_out_wait);
		msg_maxim("DIR_OUT complete!");
	break;
	case DIR_IN:
		if (usbpd_data->is_in_first_sec_uvdm_req) { /* LONG and SHORT response is same */
			SEC_UVDM_RESPONSE_HEADER = (U_SEC_UVDM_RESPONSE_HEADER *)&ReadMSG[6];
			SEC_UVDM_TX_HEADER = (U_SEC_TX_DATA_HEADER *)&ReadMSG[10];
#if (UVDM_DEBUG)
			msg_maxim("DIR_IN data_type = %d , command_type = %d, direction=%d, total_number_of_uvdm_set=%d",
				SEC_UVDM_RESPONSE_HEADER->BITS.data_type,
				SEC_UVDM_RESPONSE_HEADER->BITS.command_type,
				SEC_UVDM_RESPONSE_HEADER->BITS.direction,
				SEC_UVDM_RESPONSE_HEADER->BITS.total_number_of_uvdm_set);
#endif
			if (SEC_UVDM_RESPONSE_HEADER->BITS.command_type == SEC_UVDM_RESPONDER_ACK) {
				memcpy(usbpd_data->ReadMSG, ReadMSG, OPCODE_DATA_LENGTH);
#if (UVDM_DEBUG)
				msg_maxim("DIR_IN order_of_current_uvdm_set = %d , total_data_size = %d, data_size_of_current_set=%d",
					SEC_UVDM_TX_HEADER->BITS.order_of_current_uvdm_set,
					SEC_UVDM_TX_HEADER->BITS.total_data_size,
					SEC_UVDM_TX_HEADER->BITS.data_size_of_current_set);
#endif
				msg_maxim("DIR_IN 1st Response");
			} else if (SEC_UVDM_RESPONSE_HEADER->BITS.command_type == SEC_UVDM_RESPONDER_NAK) {
				msg_maxim("DIR_IN SEC_UVDM_RESPONDER_NAK received");
				usbpd_data->uvdm_error = -ENODATA;
			} else if (SEC_UVDM_RESPONSE_HEADER->BITS.command_type == SEC_UVDM_RESPONDER_BUSY) {
				msg_maxim("DIR_IN SEC_UVDM_RESPONDER_BUSY received");
				usbpd_data->uvdm_error = -EBUSY;
			} else {
				msg_maxim("DIR_IN Undefined RESPONDER value");
				usbpd_data->uvdm_error = -EPROTO;
			}
		} else {
			/* don't have ack packet after 2nd sec_tx_data_header */
			memcpy(usbpd_data->ReadMSG, ReadMSG, OPCODE_DATA_LENGTH);
			SEC_UVDM_TX_HEADER = (U_SEC_TX_DATA_HEADER *)&usbpd_data->ReadMSG[6];
#if (UVDM_DEBUG)
			msg_maxim("DIR_IN order_of_current_uvdm_set = %d , total_data_size = %d, data_size_of_current_set=%d",
				SEC_UVDM_TX_HEADER->BITS.order_of_current_uvdm_set,
				SEC_UVDM_TX_HEADER->BITS.total_data_size,
				SEC_UVDM_TX_HEADER->BITS.data_size_of_current_set);
#endif
			if (SEC_UVDM_TX_HEADER->data != 0)
				msg_maxim("DIR_IN %dth Response", SEC_UVDM_TX_HEADER->BITS.order_of_current_uvdm_set);
			else
				msg_maxim("DIR_IN Last Response. It's zero");
		}
		complete(&usbpd_data->uvdm_longpacket_in_wait);
		msg_maxim("DIR_IN complete!");
	break;
	default:
		msg_maxim("Never Call!!!");
	break;
	}
}

int max77705_sec_uvdm_out_request_message(void *data, int size)
{
	struct max77705_usbc_platform_data *usbpd_data;
	struct i2c_client *i2c;
	int ret;

	usbpd_data = g_usbc_data;
	if (!usbpd_data)
		return -ENXIO;

	i2c = usbpd_data->muic;
	if (i2c == NULL) {
		msg_maxim("usbpd_data->i2c is not valid!");
		return -EINVAL;
	}

	if (data == NULL) {
		msg_maxim("given data is not valid !");
		return -EINVAL;
	}

	if (size >= SAMSUNGUVDM_MAX_LONGPACKET_SIZE) {
		msg_maxim("size %d is too big to send data", size);
		ret = -EFBIG;
	} else if (size <= SAMSUNGUVDM_MAX_SHORTPACKET_SIZE)
		ret = max77705_send_sec_unstructured_short_vdm_message(usbpd_data, data, size);
	else
		ret = max77705_send_sec_unstructured_long_vdm_message(usbpd_data, data, size);

	return ret;
}

int max77705_sec_uvdm_in_request_message(void *data)
{
	struct max77705_usbc_platform_data *usbpd_data;
	uint8_t SendMSG[32] = {0,};
	uint8_t ReadMSG[32] = {0,};
	uint8_t IN_DATA[MAX_INPUT_DATA] = {0,};

	/* Send Request */
	/* 1  Message Type Definition */
	U_SEC_UVDM_RESPONSE_HEADER	*SEC_RES_HEADER;
	U_SEC_TX_DATA_HEADER		*SEC_UVDM_TX_HEADER;
	U_SEC_TX_DATA_TAILER		*SEC_UVDM_TX_TAILER;

	int cur_uvdmset_data = 0;
	int cur_uvdmset_num = 0;
	int total_uvdmset_num = 0;
	int received_data_size = 0;
	int total_received_data_size = 0;
	int ack = 0;
	int size = 0;
	int time_left = 0;
	int cal_checksum = 0;
	int i = 0;
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	int event;
#endif

	usbpd_data = g_usbc_data;
	if (!usbpd_data)
		return -ENXIO;

	if (data == NULL) {
		msg_maxim("%s given data is not valid !", __func__);
		return -EINVAL;
	}

	usbpd_data->is_in_sec_uvdm_out = DIR_IN;
	usbpd_data->is_in_first_sec_uvdm_req = true;

	/* 1. Common : Fill the MSGHeader */
	set_msgheader(SendMSG, ACK, 2);
	/* 2. Common : Fill the UVDMHeader*/
	set_uvdmheader(SendMSG, SAMSUNG_VENDOR_ID, 0);
	/* 3. Common : Fill the First SEC_VDMHeader*/
	if (usbpd_data->is_in_first_sec_uvdm_req)
		set_sec_uvdmheader(SendMSG, usbpd_data->Product_ID, TYPE_LONG, SEC_UVDM_ININIATOR, DIR_IN, 0, 0);
	/* 8. Send data to PDIC */
	usbpd_data->uvdm_error = 0;
	max77705_send_vdm_write_message(SendMSG);

	cur_uvdmset_num = 0;
	total_uvdmset_num = 1;

	do {
		reinit_completion(&usbpd_data->uvdm_longpacket_in_wait);
		time_left =
			wait_for_completion_interruptible_timeout(&usbpd_data->uvdm_longpacket_in_wait,
					msecs_to_jiffies(SAMSUNGUVDM_WAIT_MS));

		if (time_left <= 0) {
			msg_maxim("timeout");
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
			event = NOTIFY_EXTRA_UVDM_TIMEOUT;
			store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#endif
			return -ETIME;
		}
		if (usbpd_data->uvdm_error)
			return usbpd_data->uvdm_error;
		/* read data */
		memset(ReadMSG, 0, 32);

		/* read data */
		memcpy(ReadMSG, usbpd_data->ReadMSG, OPCODE_DATA_LENGTH);

		if (usbpd_data->is_in_first_sec_uvdm_req) {
			SEC_RES_HEADER = (U_SEC_UVDM_RESPONSE_HEADER *)&ReadMSG[6];
			SEC_UVDM_TX_HEADER = (U_SEC_TX_DATA_HEADER *)&ReadMSG[10];
#if (UVDM_DEBUG)
			msg_maxim("SEC_RES_HEADER : 0x%x, SEC_UVDM_TX_HEADER : 0x%x", SEC_RES_HEADER->data, SEC_UVDM_TX_HEADER->data);
#endif
			if (SEC_RES_HEADER->BITS.data_type == TYPE_LONG) {
				/* 1. check the data size received */
				size = SEC_UVDM_TX_HEADER->BITS.total_data_size;
				cur_uvdmset_data = SEC_UVDM_TX_HEADER->BITS.data_size_of_current_set;
				cur_uvdmset_num = SEC_UVDM_TX_HEADER->BITS.order_of_current_uvdm_set;
				total_uvdmset_num =
					SEC_RES_HEADER->BITS.total_number_of_uvdm_set;

				usbpd_data->is_in_first_sec_uvdm_req = false;
				/* 2. copy data to buffer */
				for (i = 0; i < SAMSUNGUVDM_MAXDATA_FIRST_UVDMSET; i++)
					IN_DATA[received_data_size++] = ReadMSG[14 + i];

				total_received_data_size += cur_uvdmset_data;
				usbpd_data->is_in_first_sec_uvdm_req = false;
			} else {
				IN_DATA[received_data_size++] = SEC_RES_HEADER->BITS.data;
				return received_data_size;
			}
		} else {
			SEC_UVDM_TX_HEADER = (U_SEC_TX_DATA_HEADER *)&ReadMSG[6];
			cur_uvdmset_data = SEC_UVDM_TX_HEADER->BITS.data_size_of_current_set;
			cur_uvdmset_num = SEC_UVDM_TX_HEADER->BITS.order_of_current_uvdm_set;
			/* 2. copy data to buffer */
			for (i = 0 ; i < SAMSUNGUVDM_MAXDATA_NORMAL_UVDMSET ; i++)
				IN_DATA[received_data_size++] = ReadMSG[10 + i];
			total_received_data_size += cur_uvdmset_data;
		}
		/* 3. Check Checksum */
		SEC_UVDM_TX_TAILER = (U_SEC_TX_DATA_TAILER *)&ReadMSG[26];
		cal_checksum = get_checksum(ReadMSG, 6, SAMSUNGUVDM_CHECKSUM_DATA_COUNT);
		ack = (cal_checksum == SEC_UVDM_TX_TAILER->BITS.checksum) ?
			SEC_UVDM_RX_HEADER_ACK : SEC_UVDM_RX_HEADER_NAK;
#if (UVDM_DEBUG)
		msg_maxim("SEC_UVDM_TX_TAILER : 0x%x, cal_checksum : 0x%x, size : %d", SEC_UVDM_TX_TAILER->data, cal_checksum, size);
#endif
		/* 1. Common : Fill the MSGHeader */
		if (cur_uvdmset_num == total_uvdmset_num)
			set_msgheader(SendMSG, NAK, 2);
		else
			set_msgheader(SendMSG, ACK, 2);
		/* 2. Common : Fill the UVDMHeader*/
		set_uvdmheader(SendMSG, SAMSUNG_VENDOR_ID, 0);
		/* 5-3. Common : Fill the SEC RXHeader */
		set_sec_uvdm_rxdata_header(SendMSG, cur_uvdmset_num, cur_uvdmset_data, ack);

		/* 8. Send data to PDIC */
		usbpd_data->uvdm_error = 0;
		max77705_send_vdm_write_message(SendMSG);
	} while (cur_uvdmset_num < total_uvdmset_num);
	set_endian(IN_DATA, data, size);

	reinit_completion(&usbpd_data->uvdm_longpacket_in_wait);
	time_left =
		wait_for_completion_interruptible_timeout(&usbpd_data->uvdm_longpacket_in_wait,
				msecs_to_jiffies(SAMSUNGUVDM_WAIT_MS));
	if (time_left <= 0) {
		msg_maxim("last in request timeout");
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
		event = NOTIFY_EXTRA_UVDM_TIMEOUT;
		store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#endif
		return -ETIME;
	}
	if (usbpd_data->uvdm_error)
		return usbpd_data->uvdm_error;

	return size;
}

int max77705_sec_uvdm_ready(void)
{
	int uvdm_ready = false;
	struct max77705_usbc_platform_data *usbpd_data;

	usbpd_data = g_usbc_data;

	msg_maxim("power_nego is%s done, accessory_enter_mode is%s done",
			usbpd_data->pn_flag ? "" : " not",
			usbpd_data->is_samsung_accessory_enter_mode? "" : " not");

	if (usbpd_data->pn_flag &&
			usbpd_data->is_samsung_accessory_enter_mode)
		uvdm_ready = true;

	msg_maxim("uvdm ready = %d", uvdm_ready);
	return uvdm_ready;
}

void max77705_sec_uvdm_close(void)
{
	struct max77705_usbc_platform_data *usbpd_data;

	usbpd_data = g_usbc_data;
	complete(&usbpd_data->uvdm_longpacket_out_wait);
	complete(&usbpd_data->uvdm_longpacket_in_wait);
	msg_maxim("success");
}
