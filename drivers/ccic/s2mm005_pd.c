/*
 * driver/../s2mm005.c - S2MM005 USB PD function driver
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
#include <linux/ccic/s2mm005_ext.h>
#include <linux/power_supply.h>
#if defined(CONFIG_BATTERY_NOTIFIER)
#include <linux/battery/battery_notifier.h>
#endif
#if defined(CONFIG_USB_HOST_NOTIFY)
#include <linux/usb_notify.h>
#endif

extern struct pdic_notifier_struct pd_noti;

////////////////////////////////////////////////////////////////////////////////
// function definition
////////////////////////////////////////////////////////////////////////////////
void s2mm005_select_pdo(int num);
void vbus_turn_on_ctrl(bool enable);
void process_pd(void *data, u8 plug_attach_done, u8 *pdic_attach, MSG_IRQ_STATUS_Type *MSG_IRQ_State);
////////////////////////////////////////////////////////////////////////////////
// PD function will be merged
////////////////////////////////////////////////////////////////////////////////
static inline struct power_supply *get_power_supply_by_name(char *name)
{
	if (!name)
		return (struct power_supply *)NULL;
	else
		return power_supply_get_by_name(name);
}

void s2mm005_select_pdo(int num)
{
	uint8_t CMD_DATA[3];
	struct s2mm005_data *usbpd_data = pd_noti.pusbpd;

	if (!usbpd_data) {
		pr_info(" %s : pusbpd is null\n", __func__);
		return;
	}

	if (pd_noti.sink_status.selected_pdo_num == num)
		return;
	else if (num > pd_noti.sink_status.available_pdo_num)
		pd_noti.sink_status.selected_pdo_num = pd_noti.sink_status.available_pdo_num;
	else if (num < 1)
		pd_noti.sink_status.selected_pdo_num = 1;
	else
		pd_noti.sink_status.selected_pdo_num = num;
	pr_info(" %s : PDO(%d) is selected to change\n", __func__, pd_noti.sink_status.selected_pdo_num);

	CMD_DATA[0] = 0x3;
	CMD_DATA[1] = 0x3;
	CMD_DATA[2] = pd_noti.sink_status.selected_pdo_num;
	s2mm005_write_byte(usbpd_data->i2c, REG_I2C_SLV_CMD, &CMD_DATA[0], 3);

	CMD_DATA[0] = 0x3;
	CMD_DATA[1] = 0x2;
	CMD_DATA[2] = State_PE_SNK_Wait_for_Capabilities;
	s2mm005_write_byte(usbpd_data->i2c, REG_I2C_SLV_CMD, &CMD_DATA[0], 3);
}

void vbus_turn_on_ctrl(bool enable)
{
	struct power_supply *psy_otg;
	union power_supply_propval val;
	int on = !!enable;
	int ret = 0;

	pr_info("%s %d, enable=%d\n", __func__, __LINE__, enable);
	psy_otg = get_power_supply_by_name("otg");
	if (psy_otg) {
		val.intval = enable;
		ret = psy_otg->desc->set_property(psy_otg, POWER_SUPPLY_PROP_ONLINE, &val);
	} else {
		pr_err("%s: Fail to get psy battery\n", __func__);
	}
	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	} else {
		pr_info("otg accessory power = %d\n", on);
	}

}

static int s2mm005_src_capacity_information(const struct i2c_client *i2c, uint32_t *RX_SRC_CAPA_MSG,
		PDIC_SINK_STATUS * pd_sink_status, uint8_t *do_power_nego)
{
	uint32_t RdCnt;
	uint32_t PDO_cnt;
	uint32_t PDO_sel;
	int available_pdo_num = 0;

	MSG_HEADER_Type *MSG_HDR;
	SRC_FIXED_SUPPLY_Typedef *MSG_FIXED_SUPPLY;
	SRC_VAR_SUPPLY_Typedef  *MSG_VAR_SUPPLY;
	SRC_BAT_SUPPLY_Typedef  *MSG_BAT_SUPPLY;

	dev_info(&i2c->dev, "\n");
	for(RdCnt=0;RdCnt<8;RdCnt++)
	{
		dev_info(&i2c->dev, "Rd_SRC_CAPA_%d : 0x%X\n", RdCnt, RX_SRC_CAPA_MSG[RdCnt]);
	}

	MSG_HDR = (MSG_HEADER_Type *)&RX_SRC_CAPA_MSG[0];
	dev_info(&i2c->dev, "=======================================\n");
	dev_info(&i2c->dev, "    MSG Header\n");

	dev_info(&i2c->dev, "    Rsvd_msg_header         : %d\n",MSG_HDR->Rsvd_msg_header );
	dev_info(&i2c->dev, "    Number_of_obj           : %d\n",MSG_HDR->Number_of_obj );
	dev_info(&i2c->dev, "    Message_ID              : %d\n",MSG_HDR->Message_ID );
	dev_info(&i2c->dev, "    Port_Power_Role         : %d\n",MSG_HDR->Port_Power_Role );
	dev_info(&i2c->dev, "    Specification_Revision  : %d\n",MSG_HDR->Specification_Revision );
	dev_info(&i2c->dev, "    Port_Data_Role          : %d\n",MSG_HDR->Port_Data_Role );
	dev_info(&i2c->dev, "    Rsvd2_msg_header        : %d\n",MSG_HDR->Rsvd2_msg_header );
	dev_info(&i2c->dev, "    Message_Type            : %d\n",MSG_HDR->Message_Type );

	for(PDO_cnt = 0;PDO_cnt < MSG_HDR->Number_of_obj;PDO_cnt++)
	{
		PDO_sel = (RX_SRC_CAPA_MSG[PDO_cnt + 1] >> 30) & 0x3;
		dev_info(&i2c->dev, "    =================\n");
		dev_info(&i2c->dev, "    PDO_Num : %d\n", (PDO_cnt + 1));

		if(PDO_sel == 0)        // *MSG_FIXED_SUPPLY
		{
			MSG_FIXED_SUPPLY = (SRC_FIXED_SUPPLY_Typedef *)&RX_SRC_CAPA_MSG[PDO_cnt + 1];
			if(MSG_FIXED_SUPPLY->Voltage_Unit <= (AVAILABLE_VOLTAGE/UNIT_FOR_VOLTAGE))
				available_pdo_num = PDO_cnt + 1;
			if (!(*do_power_nego) &&
				(pd_sink_status->power_list[PDO_cnt+1].max_voltage != MSG_FIXED_SUPPLY->Voltage_Unit * UNIT_FOR_VOLTAGE ||
				pd_sink_status->power_list[PDO_cnt+1].max_current != MSG_FIXED_SUPPLY->Maximum_Current * UNIT_FOR_CURRENT))
				*do_power_nego = 1;
			pd_sink_status->power_list[PDO_cnt+1].max_voltage = MSG_FIXED_SUPPLY->Voltage_Unit * UNIT_FOR_VOLTAGE;
			pd_sink_status->power_list[PDO_cnt+1].max_current = MSG_FIXED_SUPPLY->Maximum_Current * UNIT_FOR_CURRENT;

			dev_info(&i2c->dev, "    PDO_Parameter(FIXED_SUPPLY) : %d\n",MSG_FIXED_SUPPLY->PDO_Parameter );
			dev_info(&i2c->dev, "    Dual_Role_Power         : %d\n",MSG_FIXED_SUPPLY->Dual_Role_Power );
			dev_info(&i2c->dev, "    USB_Suspend_Support     : %d\n",MSG_FIXED_SUPPLY->USB_Suspend_Support );
			dev_info(&i2c->dev, "    Externally_POW          : %d\n",MSG_FIXED_SUPPLY->Externally_POW );
			dev_info(&i2c->dev, "    USB_Comm_Capable        : %d\n",MSG_FIXED_SUPPLY->USB_Comm_Capable );
			dev_info(&i2c->dev, "    Data_Role_Swap          : %d\n",MSG_FIXED_SUPPLY->Data_Role_Swap );
			dev_info(&i2c->dev, "    Reserved                : %d\n",MSG_FIXED_SUPPLY->Reserved );
			dev_info(&i2c->dev, "    Peak_Current            : %d\n",MSG_FIXED_SUPPLY->Peak_Current );
			dev_info(&i2c->dev, "    Voltage_Unit            : %d\n",MSG_FIXED_SUPPLY->Voltage_Unit );
			dev_info(&i2c->dev, "    Maximum_Current         : %d\n",MSG_FIXED_SUPPLY->Maximum_Current );
		}
		else if(PDO_sel == 2)   // *MSG_VAR_SUPPLY
		{
			MSG_VAR_SUPPLY = (SRC_VAR_SUPPLY_Typedef *)&RX_SRC_CAPA_MSG[PDO_cnt + 1];

			dev_info(&i2c->dev, "    PDO_Parameter(VAR_SUPPLY) : %d\n",MSG_VAR_SUPPLY->PDO_Parameter );
			dev_info(&i2c->dev, "    Maximum_Voltage          : %d\n",MSG_VAR_SUPPLY->Maximum_Voltage );
			dev_info(&i2c->dev, "    Minimum_Voltage          : %d\n",MSG_VAR_SUPPLY->Minimum_Voltage );
			dev_info(&i2c->dev, "    Maximum_Current          : %d\n",MSG_VAR_SUPPLY->Maximum_Current );
		}
		else if(PDO_sel == 1)   // *MSG_BAT_SUPPLY
		{
			MSG_BAT_SUPPLY = (SRC_BAT_SUPPLY_Typedef *)&RX_SRC_CAPA_MSG[PDO_cnt + 1];

			dev_info(&i2c->dev, "    PDO_Parameter(BAT_SUPPLY)  : %d\n",MSG_BAT_SUPPLY->PDO_Parameter );
			dev_info(&i2c->dev, "    Maximum_Voltage            : %d\n",MSG_BAT_SUPPLY->Maximum_Voltage );
			dev_info(&i2c->dev, "    Minimum_Voltage            : %d\n",MSG_BAT_SUPPLY->Minimum_Voltage );
			dev_info(&i2c->dev, "    Maximum_Allow_Power        : %d\n",MSG_BAT_SUPPLY->Maximum_Allow_Power );
		}

	}

	/* the number of available pdo list */
	pd_sink_status->available_pdo_num = available_pdo_num;
	dev_info(&i2c->dev, "=======================================\n");
	return available_pdo_num;
}

void process_pd(void *data, u8 plug_attach_done, u8 *pdic_attach, MSG_IRQ_STATUS_Type *MSG_IRQ_State)
{
	struct s2mm005_data *usbpd_data = data;
	struct i2c_client *i2c = usbpd_data->i2c;
	uint16_t REG_ADD;
	uint8_t rp_currentlvl, is_src;
	REQUEST_FIXED_SUPPLY_STRUCT_Typedef *request_power_number;
#if defined(CONFIG_USB_HOST_NOTIFY)
	struct otg_notify *o_notify = get_otg_notify();
#endif

	printk("%s\n",__func__);
	rp_currentlvl = ((usbpd_data->func_state >> 27) & 0x3);
	is_src = (usbpd_data->func_state & (0x1 << 25) ? 1 : 0);
	dev_info(&i2c->dev, "rp_currentlvl:0x%02X, is_source:0x%02X\n", rp_currentlvl, is_src);

	if (MSG_IRQ_State->BITS.Ctrl_Flag_PR_Swap)
	{
		usbpd_data->is_pr_swap++;
		dev_info(&i2c->dev, "PR_Swap requested to %s\n", is_src ? "SOURCE" : "SINK");
		if (is_src && (usbpd_data->power_role == DUAL_ROLE_PROP_PR_SNK)) {
			ccic_event_work(usbpd_data, CCIC_NOTIFY_DEV_BATTERY, CCIC_NOTIFY_ID_ATTACH, 0, 0, 0);
		}
		vbus_turn_on_ctrl(is_src);
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
		usbpd_data->power_role = is_src ? DUAL_ROLE_PROP_PR_SRC : DUAL_ROLE_PROP_PR_SNK;
#if defined(CONFIG_USB_HOST_NOTIFY)
		if( usbpd_data->power_role == DUAL_ROLE_PROP_PR_SRC)
			send_otg_notify(o_notify, NOTIFY_EVENT_POWER_SOURCE, 1);
		else if( usbpd_data->power_role == DUAL_ROLE_PROP_PR_SNK)
			send_otg_notify(o_notify, NOTIFY_EVENT_POWER_SOURCE, 0);
#endif
		ccic_event_work(usbpd_data, CCIC_NOTIFY_DEV_PDIC, CCIC_NOTIFY_ID_ROLE_SWAP, 0, 0, 0);
#endif
	}

	if (MSG_IRQ_State->BITS.Data_Flag_SRC_Capability)
	{
		uint8_t ReadMSG[32];
		int available_pdo_num;
		uint8_t do_power_nego = 0;
		pd_noti.event = PDIC_NOTIFY_EVENT_PD_SINK;

		REG_ADD = REG_RX_SRC_CAPA_MSG;
		s2mm005_read_byte(i2c, REG_ADD, ReadMSG, 32);
		available_pdo_num = s2mm005_src_capacity_information(i2c, (uint32_t *)ReadMSG, &pd_noti.sink_status, &do_power_nego);

		REG_ADD = REG_TX_REQUEST_MSG;
		s2mm005_read_byte(i2c, REG_ADD, ReadMSG, 32);
		request_power_number = (REQUEST_FIXED_SUPPLY_STRUCT_Typedef *)&ReadMSG[4];

		pr_info(" %s : Object_posision(%d), available_pdo_num(%d), selected_pdo_num(%d) \n", __func__,
			request_power_number->Object_Position, available_pdo_num, pd_noti.sink_status.selected_pdo_num);
		pd_noti.sink_status.current_pdo_num = request_power_number->Object_Position;

		if(available_pdo_num > 0)
		{
			if(request_power_number->Object_Position != pd_noti.sink_status.selected_pdo_num)
			{
				if (pd_noti.sink_status.selected_pdo_num == 0)
				{
					pr_info(" %s : PDO is not selected yet by default\n", __func__);
					pd_noti.sink_status.selected_pdo_num = pd_noti.sink_status.current_pdo_num;
				}
			} else {
				if (do_power_nego) {
					pr_info(" %s : PDO(%d) is selected, but power negotiation is requested\n",
						__func__, pd_noti.sink_status.selected_pdo_num);
					pd_noti.sink_status.selected_pdo_num = 0;
					pd_noti.event = PDIC_NOTIFY_EVENT_PD_SINK_CAP;
				} else {
					pr_info(" %s : PDO(%d) is selected, but same with previous list, so skip\n",
						__func__, pd_noti.sink_status.selected_pdo_num);
				}
			}
			*pdic_attach = 1;
		} else {
			pr_info(" %s : PDO is not selected\n", __func__);
		}
	}

	if (MSG_IRQ_State->BITS.Ctrl_Flag_Get_Sink_Cap)
	{
		pr_info(" %s : SRC requested SINK Cap\n", __func__);
	}

	/* notify to battery */
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	if (plug_attach_done) {
		if (*pdic_attach) {
			/* PD charger is detected by PDIC */
		} else if (!is_src && (usbpd_data->pd_state == State_PE_SNK_Wait_for_Capabilities ||
			usbpd_data->pd_state == State_ErrorRecovery) &&
			rp_currentlvl != pd_noti.sink_status.rp_currentlvl &&
			rp_currentlvl >= RP_CURRENT_LEVEL_DEFAULT) {
			if (rp_currentlvl == RP_CURRENT_LEVEL3) {
				/* 5V/3A RP charger is detected by CCIC */
				pd_noti.sink_status.rp_currentlvl = RP_CURRENT_LEVEL3;
				pd_noti.event = PDIC_NOTIFY_EVENT_CCIC_ATTACH;
			} else if (rp_currentlvl == RP_CURRENT_LEVEL2) {
				/* 5V/1.5A RP charger is detected by CCIC */
				pd_noti.sink_status.rp_currentlvl = RP_CURRENT_LEVEL2;
				pd_noti.event = PDIC_NOTIFY_EVENT_CCIC_ATTACH;
			} else if (rp_currentlvl == RP_CURRENT_LEVEL_DEFAULT) {
				/* 5V/0.5A RP charger is detected by CCIC */
				pd_noti.sink_status.rp_currentlvl = RP_CURRENT_LEVEL_DEFAULT;
				pd_noti.event = PDIC_NOTIFY_EVENT_CCIC_ATTACH;
			}
		} else
			return;
#ifdef CONFIG_SEC_FACTORY
		pr_info(" %s : debug pdic_attach(%d) event(%d)\n", __func__, *pdic_attach, pd_noti.event);
#endif
		ccic_event_work(usbpd_data, CCIC_NOTIFY_DEV_BATTERY, CCIC_NOTIFY_ID_POWER_STATUS, *pdic_attach, 0, 0);
	} else {
		pd_noti.sink_status.rp_currentlvl = RP_CURRENT_LEVEL_NONE;
		pd_noti.sink_status.available_pdo_num = 0;
		pd_noti.sink_status.selected_pdo_num = 0;
		pd_noti.sink_status.current_pdo_num = 0;
		pd_noti.event = PDIC_NOTIFY_EVENT_DETACH;
	}
#else
	if(plug_attach_done)
	{
		/* PD notify */
		if(*pdic_attach)
			pd_noti.event = PDIC_NOTIFY_EVENT_PD_SINK;
		else
			pd_noti.event = PDIC_NOTIFY_EVENT_CCIC_ATTACH;
	}
	else
	{ 
		pd_noti.sink_status.selected_pdo_num = 0;
		pd_noti.event = PDIC_NOTIFY_EVENT_DETACH;
	}
	pdic_notifier_call(pd_noti);
#endif
}
