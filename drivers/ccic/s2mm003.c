/*
 * driver/../s2mm003.c - S2MM003 USBPD device driver
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
#include <linux/ccic/s2mm003.h>
#include <linux/power_supply.h>
#include <linux/battery/sec_charger.h>
#if defined(CONFIG_BATTERY_NOTIFIER)
#include <linux/battery/battery_notifier.h>
#endif
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
#include <linux/usb/class-dual-role.h>
#endif

/* switch device header */
#ifdef CONFIG_SWITCH
#include <linux/switch.h>
#endif /* CONFIG_SWITCH */

#ifdef CONFIG_SWITCH
static struct switch_dev switch_dock = {
	.name = "ccic_dock",
};
#endif /* CONFIG_SWITCH */

/* CCIC Dock Observer Callback parameter */
enum {
	CCIC_DOCK_DETACHED	= 0,
	CCIC_DOCK_HMT		= 105,
	CCIC_DOCK_ABNORMAL	= 106,
};

extern struct device *ccic_device;

static void ccic_send_dock_intent(int type)
{
	pr_info("%s: CCIC dock type(%d)\n", __func__, type);
#ifdef CONFIG_SWITCH
	switch_set_state(&switch_dock, type);
#endif
}

static int ccic_dock_attach_notify(int type, const char *name)
{
	pr_info("%s: %s\n", __func__, name);
	ccic_send_dock_intent(type);
	return NOTIFY_OK;
}

static int ccic_dock_detach_notify(void)
{
	pr_info("%s\n", __func__);
	ccic_send_dock_intent(CCIC_DOCK_DETACHED);
	return NOTIFY_OK;
}

static int s2mm003_read_byte(const struct i2c_client *i2c, u8 reg, u8 *val)
{
	int ret; u8 wbuf;
	struct i2c_msg msg[2];
	struct s2mm003_data *usbpd_data = i2c_get_clientdata(i2c);

	mutex_lock(&usbpd_data->i2c_mutex);
	msg[0].addr = i2c->addr;
	msg[0].flags = i2c->flags;
	msg[0].len = 1;
	msg[0].buf = &wbuf;
	msg[1].addr = i2c->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 2;
	msg[1].buf = val;

	wbuf = (reg & 0xFF);

	ret = i2c_transfer(i2c->adapter, msg, 2);
	if (ret < 0)
		dev_err(&i2c->dev, "i2c reading fail reg(0x%x), error %d\n",
			reg, ret);
	mutex_unlock(&usbpd_data->i2c_mutex);

	return ret;
}

static int s2mm003_read_byte_16(const struct i2c_client *i2c, u16 reg, u8 *val)
{
	int ret; u8 wbuf[2], rbuf;
	struct i2c_msg msg[2];
	struct s2mm003_data *usbpd_data = i2c_get_clientdata(i2c);

	mutex_lock(&usbpd_data->i2c_mutex);
	msg[0].addr = 0x43;
	msg[0].flags = i2c->flags;
	msg[0].len = 2;
	msg[0].buf = wbuf;
	msg[1].addr = 0x43;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = &rbuf;

	wbuf[0] = (reg & 0xFF00) >> 8;
	wbuf[1] = (reg & 0xFF);

	ret = i2c_transfer(i2c->adapter, msg, 2);
	if (ret < 0)
		dev_err(&i2c->dev, "i2c read16 fail reg(0x%x), error %d\n",
			reg, ret);
	mutex_unlock(&usbpd_data->i2c_mutex);

	*val = rbuf;
	return rbuf;
}

static int s2mm003_write_byte(const struct i2c_client *i2c, u8 reg, u8 val)
{
	int ret = 0; u8 wbuf[2];
	struct i2c_msg msg[1];
	struct s2mm003_data *usbpd_data = i2c_get_clientdata(i2c);

	mutex_lock(&usbpd_data->i2c_mutex);
	msg[0].addr = i2c->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = wbuf;

	wbuf[0] = (reg & 0xFF);
	wbuf[1] = (val & 0xFF);

	ret = i2c_transfer(i2c->adapter, msg, 1);
	if (ret < 0)
		dev_err(&i2c->dev, "i2c write fail reg(0x%x:%x), error %d\n",
				reg, val, ret);
	mutex_unlock(&usbpd_data->i2c_mutex);

	return ret;
}

static int s2mm003_write_byte_16(const struct i2c_client *i2c, u16 reg, u8 val)
{
	int ret = 0; u8 wbuf[3];
	struct i2c_msg msg[1];
	struct s2mm003_data *usbpd_data = i2c_get_clientdata(i2c);

	mutex_lock(&usbpd_data->i2c_mutex);
	msg[0].addr = 0x43;
	msg[0].flags = 0;
	msg[0].len = 3;
	msg[0].buf = wbuf;

	wbuf[0] = (reg & 0xFF00) >> 8;
	wbuf[1] = (reg & 0xFF);
	wbuf[2] = (val & 0xFF);

	ret = i2c_transfer(i2c->adapter, msg, 1);
	if (ret < 0)
		dev_err(&i2c->dev, "i2c write fail reg(0x%x:%x), error %d\n",
				reg, val, ret);
	mutex_unlock(&usbpd_data->i2c_mutex);

	return ret;
}

static void s2mm003_int_clear(struct s2mm003_data *usbpd_data)
{
	struct i2c_client *i2c = usbpd_data->i2c;

	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x1);
	s2mm003_write_byte(i2c, IRQ_RD_ADDR, 0xFF);
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x0);

}

static int s2mm003_indirect_read(struct s2mm003_data *usbpd_data, u8 address)
{
	u8 value = 0;
	int ret = 0;
	struct i2c_client *i2c = usbpd_data->i2c;

	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x1);
	s2mm003_write_byte(i2c, IRQ_RD_ADDR, address);
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x0);

	ret  = s2mm003_read_byte(i2c, IRQ_RD_DATA, &value);
	if (ret < 0)
		dev_err(&i2c->dev, "indirect read error value:%02x ret:%d\n", value, ret);

	return value;
}

static void s2mm003_indirect_write(struct s2mm003_data *usbpd_data, u8 address, u8 val)
{
	struct i2c_client *i2c = usbpd_data->i2c;

	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x1);
	s2mm003_write_byte(i2c, IRQ_WR_ADDR, address);
	s2mm003_write_byte(i2c, IRQ_WR_DATA, val);
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x0);
}

static int s2mm003_src_capacity_information(const struct i2c_client *i2c, uint32_t *RX_SRC_CAPA_MSG,
		PDIC_SINK_STATUS * pd_sink_status)
{
	uint32_t RdCnt;
	uint32_t PDO_cnt;
	uint32_t PDO_sel;
	int available_pdo_num = 0;

	MSG_HEADER_Type *MSG_HDR;
	SRC_FIXED_SUPPLY_Typedef *MSG_FIXED_SUPPLY;
	SRC_VAR_SUPPLY_Typedef  *MSG_VAR_SUPPLY;
	SRC_BAT_SUPPLY_Typedef  *MSG_BAT_SUPPLY;

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
	dev_info(&i2c->dev, "=======================================\n\r");
	dev_info(&i2c->dev, "\n\r");
	return available_pdo_num;
}
#if 0
static void s2mm003_request_select_type(uint32_t * REQ_MSG , int num)
{
	REQUEST_FIXED_SUPPLY_STRUCT_Typedef *REQ_FIXED_SPL;

	REQ_FIXED_SPL = (REQUEST_FIXED_SUPPLY_STRUCT_Typedef *)REQ_MSG;

	//REQ_FIXED_SPL->Reserved_2                   = 0;
	REQ_FIXED_SPL->Object_Position             = (num & 0x7);
	//REQ_FIXED_SPL->GiveBack_Flag               = 0;     // GiveBack Support set to 1
	//REQ_FIXED_SPL->Capa_Mismatch            = 0;
	//REQ_FIXED_SPL->USB_Comm_Capable    = 0;
	//REQ_FIXED_SPL->No_USB_Suspend        = 0;
	//REQ_FIXED_SPL->Reserved_1                  = 0;     // Set to Zero
	REQ_FIXED_SPL->OP_Current                   = 200;    // 10mA
	REQ_FIXED_SPL->Maximum_OP_Current   = 200;    // 10mA
}
#endif
static void Modify_TX_SRC_CAPA(struct s2mm003_data *usbpd_data)
{
	uint32_t i;
	U_MSG_HEADER_Type *MSG_HEADER;
	U_SRC_FIXED_SUPPLY_Typedef *MSG_TX_SRC_FIXED_SUPPLY;
	uint8_t data_arr[8]={0,};

	// Set Message Index - TX Source Capability
	s2mm003_indirect_write(usbpd_data, IND_REG_READ_MSG_INDEX, MSG_Idx_TX_SRC_CAPA);

	// Read Default TX source Capability message - Size 8 Byte
	for(i=0; i<8; i++)
		data_arr[i] = s2mm003_indirect_read(usbpd_data, i+0x50);

	// Link TX Source Capability Data struct
	MSG_HEADER = (U_MSG_HEADER_Type *)&data_arr[0];
	MSG_TX_SRC_FIXED_SUPPLY = (U_SRC_FIXED_SUPPLY_Typedef *)&data_arr[4];

	// ===== For Debug Print
	printk(KERN_ERR "MSG_TX_SRC_FIXED_SUPPLY->BITS.Maximum_Current) : %d\n", MSG_TX_SRC_FIXED_SUPPLY->BITS.Maximum_Current);
	// =====================

	// ===== For Modified Data Field
	MSG_TX_SRC_FIXED_SUPPLY->BITS.Maximum_Current = 50; // 10mA unit * 50 (500mA)
	// =====================

	// Write TX source Capability message - Size 8 Byte
	for(i=0; i<8; i++)
		s2mm003_indirect_write(usbpd_data, (i+0x50), data_arr[i]);

	return;
}

static void VBUS_TURN_ON_CTRL(bool enable)
{
	union power_supply_propval val;
	printk(KERN_ERR "%s -> enable : %d\n", __func__, enable);
	val.intval = enable;
	psy_do_property("otg", set,
			POWER_SUPPLY_PROP_ONLINE, val);
}

static void PDIC_Function_State_for_VBUS(u8 val)
{
	switch(val)
	{
	case 0: // PE_Initial_Detach
		VBUS_TURN_ON_CTRL(0);
		break;
		//  case 1: // PE_SRC_Startup
		//         VBUS_TURN_ON_CTRL(VBUS_ON);
		//      break;
		//  case 2: // PE_SRC_Discovery
		//          VBUS_TURN_ON_CTRL(VBUS_ON);
		//      break;
	case 3: // PE_SRC_Send_Capabilities
		VBUS_TURN_ON_CTRL(1);
		break;
	case 4: // PE_SRC_Negotiate_Capability
		VBUS_TURN_ON_CTRL(1);
		break;
	case 5: // PE_SRC_Transition_Supply
		VBUS_TURN_ON_CTRL(1);
		break;
	case 6: // PE_SRC_Ready
		VBUS_TURN_ON_CTRL(1);
		break;
	case 7: // PE_SRC_Disabled
		VBUS_TURN_ON_CTRL(1);
		break;
	//  case 8: // PE_SRC_Capability_Response
	//          VBUS_TURN_ON_CTRL(VBUS_ON);
	//      break;
	//  case 9: // PE_SRC_Hard_Reset
	//          VBUS_TURN_ON_CTRL(VBUS_ON);
	//      break;
	//  case 10:    // PE_SRC_Hard_Reset_Received
	//          VBUS_TURN_ON_CTRL(VBUS_ON);
	//      break;
	//  case 11:    // PE_SRC_Transition_to_default
	//          VBUS_TURN_ON_CTRL(VBUS_ON);
	//      break;
	//  case 12:    // PE_SRC_Give_Source_Cap
	//          VBUS_TURN_ON_CTRL(VBUS_ON);
	//      break;
	//  case 13:    // PE_SRC_Get_Sink_Cap
	//          VBUS_TURN_ON_CTRL(VBUS_ON);
	//      break;
	//  case 14:    // PE_SRC_Wait_New_Capabilities
	//          VBUS_TURN_ON_CTRL(VBUS_ON);
	//      break;

	//  case 15:    // PE_SNK_Startup
	//          VBUS_TURN_ON_CTRL(VBUS_OFF);
	//      break;
	//  case 16:    // PE_SNK_Discovery
	//          VBUS_TURN_ON_CTRL(VBUS_OFF);
	//      break;
	case 17:    // PE_SNK_Wait_for_Capabilities
		VBUS_TURN_ON_CTRL(0);
		break;
	case 18:    // PE_SNK_Evaluate_Capability
		VBUS_TURN_ON_CTRL(0);
		break;
	case 19:    // PE_SNK_Select_Capability
		VBUS_TURN_ON_CTRL(0);
		break;
	case 20:    // PE_SNK_Transition_Sink
		VBUS_TURN_ON_CTRL(0);
		break;
	case 21:    // PE_SNK_Ready
		VBUS_TURN_ON_CTRL(0);
		break;
	//  case 22:    // PE_SNK_Hard_Reset
	//          VBUS_TURN_ON_CTRL(VBUS_OFF);
	//      break;
	//  case 23:    // PE_SNK_Transition_to_default
	//          VBUS_TURN_ON_CTRL(VBUS_OFF);
	//      break;
	//  case 24:    // PE_SNK_Give_Sink_Cap
	//          VBUS_TURN_ON_CTRL(VBUS_OFF);
	//      break;
	//  case 25:    // PE_SNK_Get_Source_Cap
	//          VBUS_TURN_ON_CTRL(VBUS_OFF);
	//      break;
	//  case 26:    // PE_SRC_CABLE_VDM_Identity_Request
	//          VBUS_TURN_ON_CTRL(VBUS_OFF);
	//      break;
	//  case 27:    // PE_SRC_CABLE_VDM_Identity_ACKed
	//          VBUS_TURN_ON_CTRL(VBUS_OFF);
	//      break;
	//  case 28:    // PE_SRC_CABLE_VDM_Identity_NAKed
	//          VBUS_TURN_ON_CTRL(VBUS_OFF);
	//      break;
	case 29:    // ErrorRecovery
		VBUS_TURN_ON_CTRL(0);
		break;
	default:
		VBUS_TURN_ON_CTRL(0);
		break;
	}


	return;
}

static struct pdic_notifier_struct pd_noti;


static void send_usb_notify_message(struct s2mm003_data *usbpd_data, u8 mode)
{
	CC_NOTI_USB_STATUS_TYPEDEF usb_status_notifier;
	usb_status_notifier.id = CCIC_NOTIFY_ID_USB;
	usb_status_notifier.src = CCIC_NOTIFY_DEV_CCIC;
	usb_status_notifier.dest = CCIC_NOTIFY_DEV_USB;
	if(mode == USB_STATUS_NOTIFY_DETACH)
		usb_status_notifier.attach = CCIC_NOTIFY_DETACH;
	else
		usb_status_notifier.attach = CCIC_NOTIFY_ATTACH;
	usb_status_notifier.drp = mode;
	usbpd_data->Pdic_usb_state = mode;
	ccic_notifier_notify((CC_NOTI_TYPEDEF*)&usb_status_notifier, NULL, 0);
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
	if (usbpd_data->try_state_change && (usbpd_data->Pdic_usb_state != USB_STATUS_NOTIFY_DETACH))
	{
		// Role change try and new mode detected
		complete(&usbpd_data->reverse_completion);
	}
	dual_role_instance_changed(usbpd_data->dual_role);
#endif
}

/* for alternate mode */
#if defined(CONFIG_CCIC_ALTERNATE_MODE)
static void Msg_Read_Discover_Identity(struct s2mm003_data *usbpd_data)
{
    uint32_t                i;
    U_MSG_HEADER_Type             *MSG_HEADER;
    U_DATA_MSG_VDM_HEADER_Type    *DATA_MSG_VDM;
    U_DATA_MSG_ID_HEADER_Type     *DATA_MSG_ID;
	U_CERT_STAT_VDO_Type		  *DATA_MSG_CERT;
	U_PRODUCT_VDO_Type			  *DATA_MSG_PRODUCT;
	uint8_t data_arr[32]={0,};

	MSG_HEADER = (U_MSG_HEADER_Type *)&data_arr[0];
	DATA_MSG_VDM = (U_DATA_MSG_VDM_HEADER_Type *)&data_arr[4];
	DATA_MSG_ID = (U_DATA_MSG_ID_HEADER_Type *)&data_arr[8];
	DATA_MSG_CERT = (U_CERT_STAT_VDO_Type *)&data_arr[12];
	DATA_MSG_PRODUCT = (U_PRODUCT_VDO_Type *)&data_arr[16];
	s2mm003_indirect_write(usbpd_data, IND_REG_READ_MSG_INDEX, MSG_Idx_RX_DISC_ID_RESP);
	for(i=0; i<32; i++)
		data_arr[i] = s2mm003_indirect_read(usbpd_data, i+0x50);
	usbpd_data->Vendor_ID = DATA_MSG_ID->BITS.USB_Vendor_ID;
	usbpd_data->Product_ID = DATA_MSG_PRODUCT->BITS.Product_ID;
	printk(KERN_ERR "[%s] Vendor_ID : 0x%X, Product_ID : 0x%X\n", __func__, usbpd_data->Vendor_ID, usbpd_data->Product_ID);
	if(usbpd_data->Vendor_ID ==0x04E8 && usbpd_data->Product_ID==0xA500) // Gear VR VID, PID
		usbpd_data->acc_type = CCIC_DOCK_HMT;
    if( (DATA_MSG_ID->BITS.Data_Capable_USB_Host == 0x01)
        && (DATA_MSG_ID->BITS.Data_Capable_USB_Device == 0x00)
        && (DATA_MSG_ID->BITS.Product_Type == 0x02)
        ){
    	printk(KERN_ERR "[%s] Working with USB Host Device.\n", __func__);
        s2mm003_indirect_write(usbpd_data, 0x4C, 0x10); // Detach and re-attach
        VBUS_TURN_ON_CTRL(0);
        usbpd_data->Pdic_state_machine = b_Cmd_Intial_State;
        usbpd_data->func_state = FUNCTION_STATUS_INITIAL_DETACH;
		send_usb_notify_message(usbpd_data, USB_STATUS_NOTIFY_DETACH);
    }
    else
	    usbpd_data->Pdic_state_machine = b_Cmd_Discover_Identity;
}

static void Msg_Read_Discover_SVIDs(struct s2mm003_data *usbpd_data)
{
    uint32_t i;
    U_MSG_HEADER_Type             *MSG_HEADER;
    U_DATA_MSG_VDM_HEADER_Type    *DATA_MSG_VDM;
    U_VDO1_Type                   *DATA_MSG_VDO1;
	uint8_t data_arr[32]={0,};

	MSG_HEADER = (U_MSG_HEADER_Type *)&data_arr[0];
	DATA_MSG_VDM = (U_DATA_MSG_VDM_HEADER_Type *)&data_arr[4];
	DATA_MSG_VDO1 = (U_VDO1_Type *)&data_arr[8];
	s2mm003_indirect_write(usbpd_data, IND_REG_READ_MSG_INDEX, MSG_Idx_RX_DISC_SVIDs_RESP);
	for(i=0; i<32; i++)
		data_arr[i] = s2mm003_indirect_read(usbpd_data, i+0x50);
	usbpd_data->SVID_0 = DATA_MSG_VDO1->BITS.SVID_0;
	usbpd_data->SVID_1 = DATA_MSG_VDO1->BITS.SVID_1;
	printk(KERN_ERR "[%s] SVID_0 : 0x%X, SVID_1 : 0x%X\n", __func__, usbpd_data->SVID_0, usbpd_data->SVID_1); 
	usbpd_data->Pdic_state_machine = b_Cmd_Discover_SVIDs;
}

static void Msg_Read_Discover_Modes(struct s2mm003_data *usbpd_data)
{
    uint32_t i;
    U_MSG_HEADER_Type             *MSG_HEADER;
    U_DATA_MSG_VDM_HEADER_Type    *DATA_MSG_VDM;
	uint8_t data_arr[32]={0,};

	MSG_HEADER = (U_MSG_HEADER_Type *)&data_arr[0];
	DATA_MSG_VDM = (U_DATA_MSG_VDM_HEADER_Type *)&data_arr[4];
	s2mm003_indirect_write(usbpd_data, IND_REG_READ_MSG_INDEX, MSG_Idx_RX_DISC_MODE_RESP);
	for(i=0; i<32; i++)
		data_arr[i] = s2mm003_indirect_read(usbpd_data, i+0x50);
	printk(KERN_ERR "[%s]\n", __func__);
	usbpd_data->Pdic_state_machine = b_Cmd_Discover_Modes;
}

static void Msg_Read_Enter_Mode(struct s2mm003_data *usbpd_data)
{
    U_MSG_HEADER_Type             *MSG_HEADER;
    U_DATA_MSG_VDM_HEADER_Type    *DATA_MSG_VDM;
	int i;
	uint8_t data_arr[32]={0,};
	const char name[11] = "HMT Attach";

	MSG_HEADER = (U_MSG_HEADER_Type *)&data_arr[0];
	DATA_MSG_VDM = (U_DATA_MSG_VDM_HEADER_Type *)&data_arr[4];
	s2mm003_indirect_write(usbpd_data, IND_REG_READ_MSG_INDEX, MSG_Idx_RX_DISC_ENTER_MODE_RESP);
	for(i=0; i<32; i++)
		data_arr[i] = s2mm003_indirect_read(usbpd_data, i+0x50);
	if(DATA_MSG_VDM->BITS.VDM_command_type == 1)
		printk(KERN_ERR "[%s] --> EnterMode Ack\n", __func__);
	else
		printk(KERN_ERR "[%s] --> EnterMode Nak\n", __func__);
	usbpd_data->Pdic_state_machine = b_Cmd_Enter_Mode;
	if (usbpd_data->acc_type != CCIC_DOCK_DETACHED)
		ccic_dock_attach_notify(usbpd_data->acc_type, name);
}

static void Msg_Send_Discover_Idendity(struct s2mm003_data *usbpd_data)
{
	uint32_t i=0;
	U_MSG_HEADER_Type             *MSG_HEADER;
	U_DATA_MSG_VDM_HEADER_Type    *DATA_MSG_VDM;
	uint8_t data_arr[32]={0,};

	printk(KERN_ERR "[%s]\n", __func__);

	MSG_HEADER = (U_MSG_HEADER_Type *)&data_arr[0];
	DATA_MSG_VDM = (U_DATA_MSG_VDM_HEADER_Type *)&data_arr[4];
	s2mm003_indirect_write(usbpd_data, IND_REG_READ_MSG_INDEX, MSG_Idx_VDM_MSG_REQUEST);
	for(i=0; i<8; i++)
		data_arr[i] = s2mm003_indirect_read(usbpd_data, i+0x50);
	MSG_HEADER->BITS.Number_of_obj = 1;
	DATA_MSG_VDM->BITS.Standard_Vendor_ID = 0xFF00;
	DATA_MSG_VDM->BITS.Object_Position = 0;
	for(i = 0; i < 8; i++)
		s2mm003_indirect_write(usbpd_data, i+0x50, data_arr[i]);
	s2mm003_indirect_write(usbpd_data, 0x4E, 1);
}

static void Msg_Send_Discover_SVIDs(struct s2mm003_data *usbpd_data)
{
    uint32_t i;
    U_MSG_HEADER_Type             *MSG_HEADER;
    U_DATA_MSG_VDM_HEADER_Type    *DATA_MSG_VDM;
	uint8_t data_arr[32]={0,};

	printk(KERN_ERR "[%s]\n", __func__);

	MSG_HEADER = (U_MSG_HEADER_Type *)&data_arr[0];
	DATA_MSG_VDM = (U_DATA_MSG_VDM_HEADER_Type *)&data_arr[4];
	s2mm003_indirect_write(usbpd_data, IND_REG_READ_MSG_INDEX, MSG_Idx_VDM_MSG_REQUEST);
	for(i=0; i<8; i++)
		data_arr[i] = s2mm003_indirect_read(usbpd_data, i+0x50);
	MSG_HEADER->BITS.Number_of_obj = 1;
	DATA_MSG_VDM->BITS.Standard_Vendor_ID = 0xFF00;
	DATA_MSG_VDM->BITS.Object_Position = 0;
	for(i=0; i<8; i++)
		s2mm003_indirect_write(usbpd_data, i+0x50, data_arr[i]);
	s2mm003_indirect_write(usbpd_data, 0x4E, 2);
}

static void Msg_Send_Discover_Modes(struct s2mm003_data *usbpd_data)
{
    uint32_t i;
    U_MSG_HEADER_Type             *MSG_HEADER;
    U_DATA_MSG_VDM_HEADER_Type    *DATA_MSG_VDM;
	uint8_t data_arr[32]={0,};

	printk(KERN_ERR "[%s]\n", __func__);
	
	MSG_HEADER = (U_MSG_HEADER_Type *)&data_arr[0];
	DATA_MSG_VDM = (U_DATA_MSG_VDM_HEADER_Type *)&data_arr[4];
	s2mm003_indirect_write(usbpd_data, IND_REG_READ_MSG_INDEX, MSG_Idx_VDM_MSG_REQUEST);
	for(i=0; i<8; i++)
		data_arr[i] = s2mm003_indirect_read(usbpd_data, i+0x50);
	MSG_HEADER->BITS.Number_of_obj = 1;
	DATA_MSG_VDM->BITS.Standard_Vendor_ID = usbpd_data->SVID_1;
	DATA_MSG_VDM->BITS.Object_Position = 0;
	for(i=0; i<8; i++)
		s2mm003_indirect_write(usbpd_data, i+0x50, data_arr[i]);
	s2mm003_indirect_write(usbpd_data, 0x4E, 3);
}

static void Msg_Send_Enter_Mode(struct s2mm003_data *usbpd_data)
{
    uint32_t    i;
    U_MSG_HEADER_Type             *MSG_HEADER;
    U_DATA_MSG_VDM_HEADER_Type    *DATA_MSG_VDM;
	uint8_t data_arr[32]={0,};

	printk(KERN_ERR "[%s]\n", __func__);

	MSG_HEADER = (U_MSG_HEADER_Type *)&data_arr[0];
	DATA_MSG_VDM = (U_DATA_MSG_VDM_HEADER_Type *)&data_arr[4];
	s2mm003_indirect_write(usbpd_data, IND_REG_READ_MSG_INDEX, MSG_Idx_VDM_MSG_REQUEST);
	for(i=0; i<8; i++)
		data_arr[i] = s2mm003_indirect_read(usbpd_data, i+0x50);
	MSG_HEADER->BITS.Number_of_obj = 1;
	DATA_MSG_VDM->BITS.Standard_Vendor_ID = usbpd_data->SVID_1;
	DATA_MSG_VDM->BITS.Object_Position = 1;
	for(i=0; i<8; i++)
		s2mm003_indirect_write(usbpd_data, i+0x50, data_arr[i]);
	s2mm003_indirect_write(usbpd_data, 0x4E, 4);
}
#endif

static irqreturn_t s2mm003_usbpd_irq_thread(int irq, void *data)
{
	struct s2mm003_data *usbpd_data = data;
	struct i2c_client *i2c = usbpd_data->i2c;
	unsigned char rid, plug_state_monitor;
	int cc1_valid, cc2_valid, plug_attach_done;
	int value, usbpd_state;
	int pdic_attach = 0;
	int is_dr_swap = 0;
	static CC_NOTI_ATTACH_TYPEDEF attach_notifier;
	static CC_NOTI_RID_TYPEDEF rid_notifier;
	REQUEST_FIXED_SUPPLY_STRUCT_Typedef *request_power_number;

	dev_err(&i2c->dev, "%d times\n", ++usbpd_data->wq_times);

	usbpd_state = s2mm003_indirect_read(usbpd_data, IND_REG_PDIC_PD_FUNC_STATE);
	dev_info(&i2c->dev, "USBPD_STATE I2C read: 0x%02d\n", usbpd_state);
	plug_state_monitor = s2mm003_indirect_read(usbpd_data, IND_REG_PDIC_PLUG_STATE);
	cc1_valid = S2MM003_REG_MASK(plug_state_monitor, S_CC1_VALID);
	cc2_valid = S2MM003_REG_MASK(plug_state_monitor, S_CC2_VALID);
	usbpd_data->plug_rprd_sel = S2MM003_REG_MASK(plug_state_monitor, PLUG_RPRD_SEL_MONITOR);
	plug_attach_done = S2MM003_REG_MASK(plug_state_monitor, PLUG_ATTACH_DONE);
	dev_info(&i2c->dev, "PLUG_STATE_MONITOR	I2C read:%x\n"
		 "CC1:%x CC2:%x rprd:%x attach:%x\n",
		 plug_state_monitor,
		 cc1_valid, cc2_valid, usbpd_data->plug_rprd_sel, plug_attach_done);

	/* To confirm whether PD charger is attached or not */
	value = s2mm003_indirect_read(usbpd_data, IND_REG_PDIC_MAIN_INT_NUM);
	dev_info(&i2c->dev, "INT_STATUS I2C read: 0x%02X\n", value);

	if((value & b_RESET_START) != 0x00)
	{
		dev_err(&i2c->dev, "START\n");
		s2mm003_indirect_write(usbpd_data, IND_REG_HOST_CMD_ON, 0x01);
		return IRQ_HANDLED;
	}

	if( (value & b_PD_FUNC_FLAG) != 0x00) { // Need PD Function State Read
		int address = IND_REG_PDIC_PD_FUNC_STATE; // Function State Number Read
		int ret = 0x00;
		ret = s2mm003_indirect_read(usbpd_data, address);
		usbpd_data->func_state = ret; // store the function status

		PDIC_Function_State_for_VBUS(usbpd_data->func_state);   // For VBUS control

		/* If it isn't PD charger, return value is 29*/
		dev_info(&i2c->dev, " %s : func_state = %d\n", __func__, usbpd_data->func_state);
#if defined(CONFIG_CCIC_ALTERNATE_MODE)
		if(usbpd_data->func_state == FUNCTION_STATUS_INITIAL_DETACH)
		{
			usbpd_data->Pdic_state_machine = b_Cmd_Intial_State;
			if(usbpd_data->acc_type != CCIC_DOCK_DETACHED)
			{
				ccic_dock_detach_notify();
				usbpd_data->acc_type = CCIC_DOCK_DETACHED;
			}
		}
#endif
	}

	if( (value & b_INT_1_FLAG) != 0x00)
	{
		int INT_1_value = 0;
		INT_1_value = s2mm003_indirect_read(usbpd_data, IND_REG_PDIC_1_INT); // Interrupt State 1 Read
		dev_info(&i2c->dev, "INT_State1 = 0x%X\n", INT_1_value);
#if defined(CONFIG_CCIC_ALTERNATE_MODE)
		if(INT_1_value & b_Cmd_Discover_Identity){
			Msg_Read_Discover_Identity(usbpd_data);
			Msg_Send_Discover_SVIDs(usbpd_data);
		}else if(INT_1_value & b_Cmd_Discover_SVIDs){
			Msg_Read_Discover_SVIDs(usbpd_data);
			Msg_Send_Discover_Modes(usbpd_data);
		}else if(INT_1_value & b_Cmd_Discover_Modes){
			Msg_Read_Discover_Modes(usbpd_data);
			Msg_Send_Enter_Mode(usbpd_data);
		}else if(INT_1_value & b_Cmd_Enter_Mode){
			Msg_Read_Enter_Mode(usbpd_data);
		}
#endif
	}

	if( (value & b_INT_2_FLAG) != 0x00)
	{
		int INT_2_value = 0;
		INT_2_value = s2mm003_indirect_read(usbpd_data, IND_REG_PDIC_2_INT); // Interrupt State 2 Read
		dev_info(&i2c->dev, "INT_State2 = 0x%X\n", INT_2_value);
		if(INT_2_value & b_Msg_PR_SWAP)
		{
			dev_err(&i2c->dev, "PR_Swap Wait Delay (Any)ms\n");
			//  msleep(300);
			s2mm003_indirect_write(usbpd_data, 0x4C, 6);    // Call PS_RDY Command
			VBUS_TURN_ON_CTRL(0);
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
			usbpd_data->power_role = DUAL_ROLE_PROP_PR_SNK;
#endif
		}
		else if(INT_2_value & b_Msg_DR_SWAP)
		{
			dev_err(&i2c->dev, "DR_Swap\n");
			is_dr_swap = 1;
			// toggling the state for usb host and device
#if defined(CONFIG_CCIC_NOTIFIER)
	if(usbpd_data->Pdic_usb_state == USB_STATUS_NOTIFY_ATTACH_DFP)
		send_usb_notify_message(usbpd_data, USB_STATUS_NOTIFY_ATTACH_UFP);
	else if(usbpd_data->Pdic_usb_state == USB_STATUS_NOTIFY_ATTACH_UFP)
		send_usb_notify_message(usbpd_data, USB_STATUS_NOTIFY_ATTACH_DFP);
#endif

		}
	}
	if((value & b_INT_3_FLAG) != 0x00)
	{
		int INT_3_value = 0;
		INT_3_value = s2mm003_indirect_read(usbpd_data, IND_REG_PDIC_3_INT); // Interrupt State 3 Read
		dev_info(&i2c->dev, "INT_State3 = 0x%X\n", INT_3_value);

		if((INT_3_value & b_RID_Detect_done) != 0x00)  // RID Message Detect
		{
			rid = s2mm003_indirect_read(usbpd_data, IND_REG_PDIC_RID); // RID Value Read
			dev_info(&i2c->dev, "\nRID_Value = 0x%X\n", rid);
			usbpd_data->cur_rid = rid;
			if (usbpd_data->cur_rid == usbpd_data->prev_rid) {
				dev_err(&i2c->dev, "same rid detected, -ignore-\n");
			}
			usbpd_data->p_prev_rid = usbpd_data->prev_rid;
			usbpd_data->prev_rid = usbpd_data->cur_rid;

#if defined(CONFIG_CCIC_NOTIFIER)
			rid_notifier.src = CCIC_NOTIFY_DEV_CCIC;
			rid_notifier.dest = CCIC_NOTIFY_DEV_MUIC;
			rid_notifier.id = CCIC_NOTIFY_ID_RID;
			rid_notifier.rid = rid;
			ccic_notifier_notify((CC_NOTI_TYPEDEF*)&rid_notifier, NULL, pdic_attach);
			if(rid == RID_000K)
			{
				VBUS_TURN_ON_CTRL(1);
				send_usb_notify_message(usbpd_data, USB_STATUS_NOTIFY_ATTACH_DFP);
			}
			else if(rid == RID_OPEN || rid == RID_UNDEFINED || rid == RID_523K || rid == RID_619K)
			{
				VBUS_TURN_ON_CTRL(0);
				send_usb_notify_message(usbpd_data, USB_STATUS_NOTIFY_DETACH);
			}
#endif
		}

		if((INT_3_value & b_Msg_SRC_CAP) != 0x00)  // Read Source Capability MSG.
		{
			int cnt;
			uint32_t ReadMSG[8];
			int current_pdo_num;
			u8 *p_MSG_BUF;

			s2mm003_indirect_write(usbpd_data, IND_REG_READ_MSG_INDEX, MSG_Idx_RX_SRC_CAPA);// Idx Transceiver Request
			// Select PDO
			if(usbpd_data->func_state == FUNCTION_STATUS_SINK_READY)
			{
				p_MSG_BUF = (u8 *)ReadMSG;
				for(cnt = 0; cnt < 32; cnt++)
				{
					*(p_MSG_BUF + cnt) = s2mm003_indirect_read(usbpd_data, IND_REG_MSG_ACCESS_BASE+cnt);
				}
				current_pdo_num = s2mm003_src_capacity_information(i2c, ReadMSG, &pd_noti.sink_status);

				s2mm003_indirect_write(usbpd_data, IND_REG_READ_MSG_INDEX, MSG_Idx_TX_REQUEST);// Idx Transceiver Request
				p_MSG_BUF = (u8 *)ReadMSG;

				for(cnt = 0; cnt < MSG_BUF_REQUEST_SIZE; cnt++)
				{
					*(p_MSG_BUF + cnt) = s2mm003_indirect_read(usbpd_data, IND_REG_MSG_ACCESS_BASE+cnt);
				}
				request_power_number = (REQUEST_FIXED_SUPPLY_STRUCT_Typedef *)&p_MSG_BUF[4]; // 

				pr_info(" %s : Object_posision(%d),current pdo_num(%d), selected_pdo_num(%d) \n", __func__,
						request_power_number->Object_Position, current_pdo_num, pd_noti.sink_status.selected_pdo_num);

				if(current_pdo_num > 0) {
					if((current_pdo_num != pd_noti.sink_status.selected_pdo_num) && (current_pdo_num > 0))
					{
						pd_noti.sink_status.selected_pdo_num = current_pdo_num;
						// choice : 1, 2, 3
						pr_info(" %s : PDO(%d) is selected\n", __func__, pd_noti.sink_status.selected_pdo_num);
						/* Idx Transceiver Request */
						s2mm003_indirect_write(usbpd_data, 0x40, pd_noti.sink_status.selected_pdo_num);
						s2mm003_indirect_write(usbpd_data, 0x10, 0x11); // Function state 17 setting
					} else {
						pr_info(" %s : PDO(%d) is selected, but same with previous list, so skip\n",
								__func__, pd_noti.sink_status.selected_pdo_num);
					}
					pdic_attach = 1;
				} else {
					pr_info(" %s : PDO is not selected\n", __func__);
				}
			}
		}
	}
	if( (value & b_INT_4_FLAG) != 0x00)
	{
		int INT_4_value = 0;
		INT_4_value = s2mm003_indirect_read(usbpd_data, IND_REG_PDIC_4_INT); // Interrupt State 4 Read
		dev_info(&i2c->dev, "INT_State4 = 0x%X\n", INT_4_value);
	}
	if( (value & b_INT_5_FLAG) != 0x00)
	{
		int INT_5_value = 0;
		INT_5_value = s2mm003_indirect_read(usbpd_data, IND_REG_PDIC_5_INT); // Interrupt State 5 Read
		dev_info(&i2c->dev, "INT_State5 = 0x%X\n", INT_5_value);
	}
#if defined(CONFIG_CCIC_ALTERNATE_MODE)
		if (usbpd_data->func_state == FUNCTION_STATUS_SRC_READY && usbpd_data->Pdic_state_machine == b_Cmd_Intial_State){
			Msg_Send_Discover_Idendity(usbpd_data);
		}
#endif
#if defined(CONFIG_CCIC_NOTIFIER)
	attach_notifier.attach = plug_attach_done;

#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	if(pdic_attach)
		attach_notifier.id = CCIC_NOTIFY_ID_POWER_STATUS;
	else
		attach_notifier.id = CCIC_NOTIFY_ID_ATTACH;
#else
	attach_notifier.id = CCIC_NOTIFY_ID_ATTACH;
#endif
	attach_notifier.src = CCIC_NOTIFY_DEV_CCIC;
	attach_notifier.dest = CCIC_NOTIFY_DEV_MUIC;
	attach_notifier.rprd = usbpd_data->plug_rprd_sel;

#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	ccic_notifier_notify((CC_NOTI_TYPEDEF*)&attach_notifier, &pd_noti, pdic_attach);
#else
	ccic_notifier_notify((CC_NOTI_TYPEDEF*)&attach_notifier, NULL, pdic_attach);
#endif

#if defined(CONFIG_DUAL_ROLE_USB_INTF)
	if(usbpd_data->func_state == FUNCTION_STATUS_SRC_SEND_CAPABILITY && !is_dr_swap)
	{	
		usbpd_data->power_role = DUAL_ROLE_PROP_PR_SRC;
		send_usb_notify_message(usbpd_data, USB_STATUS_NOTIFY_ATTACH_DFP);
	}
	else if(usbpd_data->func_state == FUNCTION_STATUS_SINK_DISCOVERY && !is_dr_swap)
	{
		usbpd_data->power_role = DUAL_ROLE_PROP_PR_SNK;
		send_usb_notify_message(usbpd_data, USB_STATUS_NOTIFY_ATTACH_UFP);
	}
	else if(usbpd_data->func_state == FUNCTION_STATUS_INITIAL_DETACH)
	{
		send_usb_notify_message(usbpd_data, USB_STATUS_NOTIFY_DETACH);

		// OTP mode set to DRP when capble pluged out 
		if (!usbpd_data->try_state_change)
		{
			s2mm003_indirect_write(usbpd_data, IND_REG_PDIC_MODE, TYPE_C_ATTACH_DRP);
		}
	}
#else
	if(usbpd_data->func_state == FUNCTION_STATUS_SRC_SEND_CAPABILITY && !is_dr_swap)
		send_usb_notify_message(usbpd_data, USB_STATUS_NOTIFY_ATTACH_DFP);
	else if(usbpd_data->func_state == FUNCTION_STATUS_SINK_DISCOVERY && !is_dr_swap)
		send_usb_notify_message(usbpd_data, USB_STATUS_NOTIFY_ATTACH_UFP);
	else if(usbpd_data->func_state == FUNCTION_STATUS_INITIAL_DETACH)
		send_usb_notify_message(usbpd_data, USB_STATUS_NOTIFY_DETACH);
#endif

#ifndef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	if(plug_attach_done) {
		/* PD notify */
		if(pdic_attach)
			pd_noti.event = PDIC_NOTIFY_EVENT_PD_SINK;
		else
			pd_noti.event = PDIC_NOTIFY_EVENT_CCIC_ATTACH;
	} else { 
		pd_noti.sink_status.selected_pdo_num = 0;
		pd_noti.event = PDIC_NOTIFY_EVENT_DETACH;
	}
	pdic_notifier_call(&pd_noti);
#endif
#endif
	s2mm003_int_clear(usbpd_data);
	return IRQ_HANDLED;
}

#if defined(CONFIG_OF)
static int of_s2mm003_usbpd_dt(struct device *dev,
			       struct s2mm003_data *usbpd_data)
{
	struct device_node *np_usbpd = dev->of_node;

	usbpd_data->irq_gpio = of_get_named_gpio(np_usbpd, "usbpd,usbpd_int", 0);
	usbpd_data->redriver_en = of_get_named_gpio(np_usbpd, "usbpd,redriver_en", 0);
	dev_err(dev, "usbpd_irq = %d redriver_en = %d\n",
		usbpd_data->irq_gpio, usbpd_data->redriver_en);

	return 0;
}
#endif /* CONFIG_OF */

#if defined(CONFIG_SEC_CCIC_FW_FIX)
void s2mm003_firmware_ver_check(struct s2mm003_data *usbpd_data, char *name)
{
	struct i2c_client *i2c = usbpd_data->i2c;

	usbpd_data->firm_ver[0] = (u8)s2mm003_indirect_read(usbpd_data, 0x01);
	usbpd_data->firm_ver[1] = (u8)s2mm003_indirect_read(usbpd_data, 0x02);
	usbpd_data->firm_ver[2] = (u8)s2mm003_indirect_read(usbpd_data, 0x03);
	usbpd_data->firm_ver[3] = (u8)s2mm003_indirect_read(usbpd_data, 0x04);
	dev_err(&i2c->dev, "%s %s version %02x %02x %02x %02x\n",
		USBPD_DEV_NAME, name, usbpd_data->firm_ver[0], usbpd_data->firm_ver[1],
		usbpd_data->firm_ver[2], usbpd_data->firm_ver[3]);
}
#endif

static int s2mm003_firmware_update(struct s2mm003_data *usbpd_data)
{
	const struct firmware *fw_entry;
	const unsigned char *p;
	int ret, i;
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 test, reg_check[3] = { 0, };

	ret = s2mm003_read_byte(i2c, I2C_SYSREG_SET, &reg_check[0]);
	ret = s2mm003_read_byte(i2c, I2C_SRAM_SET, &reg_check[1]);
	ret = s2mm003_read_byte(i2c, IF_S_CODE_E, &reg_check[2]);
	if ((reg_check[0] != 0) || (reg_check[1] != 0) || (reg_check[2] != 0))
	{
		dev_err(&usbpd_data->i2c->dev, "firmware register error %02x %02x %02x\n",
			reg_check[0], reg_check[1], reg_check[2]);
		s2mm003_write_byte(i2c, USB_PD_RST, 0x40);	/*FULL RESET*/
		msleep(20);
	}


	s2mm003_write_byte(i2c, I2C_SRAM_SET, 0x1);	/* 32 */
	s2mm003_read_byte_16(i2c, 0x1854, &test);
	s2mm003_write_byte(i2c, I2C_SRAM_SET, 0x0);	/* 32 */
	if (test > 7 || test < 0)
		dev_err(&usbpd_data->i2c->dev, "fw update err rid : %02x\n", test);

	ret = request_firmware(&fw_entry,
			       FIRMWARE_PATH, usbpd_data->dev);
	if (ret < 0)
		goto done;

	msleep(5);
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x1);	/* 31 */
	s2mm003_write_byte(i2c, OTP_DUMP_DISABLE, 0x1); /* 30 */
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x0);	/* 31 */
	msleep(10);

	s2mm003_write_byte(i2c, I2C_SRAM_SET, 0x1);	/* 32 */
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x1);	/* 31 */
	s2mm003_write_byte(i2c, IF_S_CODE_E, 0x4);	/* sram update enable */
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x0);	/* 31 */

/* update firmware */
	for(p = fw_entry->data, i=0; i < fw_entry->size; p++, i++)
		s2mm003_write_byte_16(i2c,(u16)i, *p);

	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x1);	/* 31 */
	s2mm003_write_byte(i2c, IF_S_CODE_E, 0x0);
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x0);	/* 31 */

	s2mm003_write_byte(i2c, I2C_SRAM_SET, 0x0);	/* 32 */
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x1);	/* 31 */
	s2mm003_write_byte(i2c, USB_PD_RST, 0x80);	/* 0D */
	s2mm003_write_byte(i2c, OTP_BLK_RST, 0x1);	/* 0C OTP controller
							   system register reset
							   to stop otp copy */
	msleep(5);
	s2mm003_write_byte(i2c, USB_PD_RST, 0x80);	/* 0D */
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x0);	/* 31 */

done:
	dev_err(&usbpd_data->i2c->dev, "firmware size: %d, error %d\n",
		(int)fw_entry->size, ret);
	if (fw_entry)
		release_firmware(fw_entry);
	return ret;
}
#if defined(CONFIG_DUAL_ROLE_USB_INTF)

static enum dual_role_property fusb_drp_properties[] = {
	DUAL_ROLE_PROP_MODE,
	DUAL_ROLE_PROP_PR,
	DUAL_ROLE_PROP_DR,
};

 /* Callback for "cat /sys/class/dual_role_usb/otg_default/<property>" */
static int dual_role_get_local_prop(struct dual_role_phy_instance *dual_role,
				    enum dual_role_property prop,
				    unsigned int *val)
{
	struct s2mm003_data *usbpd_data = dual_role_get_drvdata(dual_role);
	USB_STATUS attached_state;
	int power_role;

	if (!usbpd_data) {
		pr_err("%s : usbpd_data is null : request prop = %d \n",\
								__func__, prop);
		return -EINVAL;
	}
	attached_state = usbpd_data->Pdic_usb_state;
	power_role = usbpd_data->power_role;
	pr_info("%s : request prop = %d , attached_state = %d, prop_mode =%d, prop_pr = %d \n",\
		__func__, prop, attached_state,usbpd_data->Pdic_usb_state, power_role);

	if (attached_state == USB_STATUS_NOTIFY_ATTACH_DFP) {
		if (prop == DUAL_ROLE_PROP_MODE)
			*val = DUAL_ROLE_PROP_MODE_DFP;
		else if (prop == DUAL_ROLE_PROP_PR) {
			if (power_role)
				*val = DUAL_ROLE_PROP_PR_SNK;
			else
				*val = DUAL_ROLE_PROP_PR_SRC;
		}
		else if (prop == DUAL_ROLE_PROP_DR)
			*val = DUAL_ROLE_PROP_DR_HOST;
		else
			return -EINVAL;
	} else if (attached_state == USB_STATUS_NOTIFY_ATTACH_UFP) {
		if (prop == DUAL_ROLE_PROP_MODE)
			*val = DUAL_ROLE_PROP_MODE_UFP;
		else if (prop == DUAL_ROLE_PROP_PR) {
			if (power_role)
				*val = DUAL_ROLE_PROP_PR_SNK;
			else
				*val = DUAL_ROLE_PROP_PR_SRC;
		}
		else if (prop == DUAL_ROLE_PROP_DR)
			*val = DUAL_ROLE_PROP_DR_DEVICE;
		else
			return -EINVAL;
	} else {
		if (prop == DUAL_ROLE_PROP_MODE)
			*val = DUAL_ROLE_PROP_MODE_NONE;
		else if (prop == DUAL_ROLE_PROP_PR)
			*val = DUAL_ROLE_PROP_PR_NONE;
		else if (prop == DUAL_ROLE_PROP_DR)
			*val = DUAL_ROLE_PROP_DR_NONE;
		else
			return -EINVAL;
	}

	return 0;
}

/* Decides whether userspace can change a specific property */
static int dual_role_is_writeable(struct dual_role_phy_instance *drp,
				  enum dual_role_property prop)
{
	if (prop == DUAL_ROLE_PROP_MODE)
		return 1;
	else
		return 0;
}


int set_data_role(struct dual_role_phy_instance *dual_role,
				   enum dual_role_property prop,
				   const unsigned int *val)
{
	struct s2mm003_data *usbpd_data = dual_role_get_drvdata(dual_role);
	struct i2c_client *i2c = usbpd_data->i2c;

	USB_STATUS attached_state;
	int mode;
	int timeout = 0;
	int ret = 0;

	if (!usbpd_data) {
		pr_err("%s : usbpd_data is null \n", __func__);
		return -EINVAL;
	}
	attached_state = usbpd_data->Pdic_usb_state;
	pr_info("%s: request prop = %d , attached_state = %d , %d \n",__func__,\
	       prop, attached_state, usbpd_data->Pdic_usb_state);

	if (attached_state != USB_STATUS_NOTIFY_ATTACH_DFP
	    && attached_state != USB_STATUS_NOTIFY_ATTACH_UFP) {
		pr_info("%s : current mode : %d - just return \n",__func__,\
		       attached_state);
		return 0;
	}

	if (attached_state == USB_STATUS_NOTIFY_ATTACH_DFP
	    && *val == DUAL_ROLE_PROP_MODE_DFP) {
		pr_info("%s : current mode : %d - request mode : %d just return \n",__func__, attached_state, *val);
		return 0;
	}

	if (attached_state == USB_STATUS_NOTIFY_ATTACH_UFP
	    && *val == DUAL_ROLE_PROP_MODE_UFP)	{
		pr_info("%s : current mode : %d - request mode : %d just return \n",__func__, attached_state, *val);
		return 0;
	}

	/* Current mode DFP and Source  */
	if ( attached_state == USB_STATUS_NOTIFY_ATTACH_DFP)
	{
		pr_info("%s: try reversing, form Source to Sink\n", __func__);

		disable_irq(usbpd_data->irq_gpio);
		/* turns off VBUS first */
		VBUS_TURN_ON_CTRL(0);

		usbpd_data->power_role = 0;
		/* exit from Disabled state and set mode to UFP */
		mode =  TYPE_C_ATTACH_UFP;
		s2mm003_indirect_write(usbpd_data, IND_REG_PDIC_MODE, mode);
		usbpd_data->try_state_change = TYPE_C_ATTACH_UFP;

		enable_irq(usbpd_data->irq_gpio);
	}
	/* Current mode UFP and Sink  */
	else
	{
		pr_info("%s: try reversing, form Sink to Source\n", __func__);
		/* transition to Disabled state */
		disable_irq(usbpd_data->irq_gpio);
		/* exit from Disabled state and set mode to UFP */
		mode =  TYPE_C_ATTACH_DFP;
		s2mm003_indirect_write(usbpd_data, IND_REG_PDIC_MODE, mode);
		usbpd_data->try_state_change = TYPE_C_ATTACH_DFP;

		enable_irq(usbpd_data->irq_gpio);
	}

	reinit_completion(&usbpd_data->reverse_completion);
	timeout =
	    wait_for_completion_timeout(&usbpd_data->reverse_completion,
					msecs_to_jiffies
					(DUAL_ROLE_SET_MODE_WAIT_MS));
	// clear try_state_change 
	usbpd_data->try_state_change = 0;
	if (!timeout) {
		pr_err("%s: reverse failed, set mode to DRP\n", __func__);
		disable_irq(usbpd_data->irq_gpio);

		/* exit from Disabled state and set mode to DRP */
		mode =  TYPE_C_ATTACH_DRP;
		s2mm003_indirect_write(usbpd_data, IND_REG_PDIC_MODE, mode);
		usbpd_data->try_state_change = TYPE_C_ATTACH_DRP;
		
		enable_irq(usbpd_data->irq_gpio);

		pr_info("%s: wait for the attached state\n", __func__);
		reinit_completion(&usbpd_data->reverse_completion);
		wait_for_completion_timeout(&usbpd_data->reverse_completion,
					    msecs_to_jiffies
					    (DUAL_ROLE_SET_MODE_WAIT_MS));
		usbpd_data->try_state_change = 0;
		ret = -EIO;
	}
	dev_info(&i2c->dev, "%s -> data role : %d\n", __func__, *val);
	return ret;
}

/* Callback for "echo <value> >
 *                      /sys/class/dual_role_usb/<name>/<property>"
 * Block until the entire final state is reached.
 * Blocking is one of the better ways to signal when the operation
 * is done.
 * This function tries to switch to Attached.SRC or Attached.SNK
 * by forcing the mode into SRC or SNK.
 * On failure, we fall back to Try.SNK state machine.
 */
static int dual_role_set_prop(struct dual_role_phy_instance *dual_role,
			      enum dual_role_property prop,
			      const unsigned int *val)
{
	if (prop == DUAL_ROLE_PROP_MODE)
		return set_data_role(dual_role, prop, val);
	else
		return -EINVAL;
}

#endif

static int s2mm003_usbpd_probe(struct i2c_client *i2c,
			       const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(i2c->dev.parent);
	struct s2mm003_data *usbpd_data;
	int ret = 0;
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
	struct dual_role_phy_desc *desc;
	struct dual_role_phy_instance *dual_role;
#endif
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&i2c->dev, "i2c functionality check error\n");
		return -EIO;
	}
	usbpd_data = devm_kzalloc(&i2c->dev, sizeof(struct s2mm003_data), GFP_KERNEL);
	if (!usbpd_data) {
		dev_err(&i2c->dev, "Failed to allocate driver data\n");
		return -ENOMEM;
	}

#if defined(CONFIG_OF)
	if (i2c->dev.of_node)
		of_s2mm003_usbpd_dt(&i2c->dev, usbpd_data);
	else
		dev_err(&i2c->dev, "not found ccic dt! ret:%d\n", ret);
#endif
	ret = gpio_request(usbpd_data->irq_gpio, "s2mm003_irq");
	if (ret)
		goto err_free_irq_gpio;
	ret = gpio_request(usbpd_data->redriver_en, "s2mm003_redriver_en");
	if (ret)
		goto err_free_redriver_gpio;

	gpio_direction_input(usbpd_data->irq_gpio);
	i2c->irq = gpio_to_irq(usbpd_data->irq_gpio);
	dev_info(&i2c->dev, "%s:IRQ NUM %d\n", __func__, i2c->irq);

	/* TODO REMOVE redriver always enable, Add sleep/resume */
	ret = gpio_direction_output(usbpd_data->redriver_en, 1);
	if (ret) {
		dev_err(&i2c->dev, "Unable to set input gpio direction, error %d\n", ret);
		goto err_free_redriver_gpio;
	}

	usbpd_data->i2c = i2c;
	i2c_set_clientdata(i2c, usbpd_data);
	dev_set_drvdata(ccic_device, usbpd_data);
	device_init_wakeup(&i2c->dev, 1);
	mutex_init(&usbpd_data->i2c_mutex);

	/* Init */
	usbpd_data->p_prev_rid = -1;
	usbpd_data->prev_rid = -1;
	usbpd_data->cur_rid = -1;
#if defined(CONFIG_CCIC_ALTERNATE_MODE)
	usbpd_data->Pdic_state_machine = 0;
	usbpd_data->acc_type = 0;
#endif
	usbpd_data->func_state = 0;
	usbpd_data->Pdic_usb_state = 0;

#if defined(CONFIG_SEC_CCIC_FW_FIX)
	s2mm003_firmware_ver_check(usbpd_data, "otp");
#else
	usbpd_data->firm_ver[0] = s2mm003_indirect_read(usbpd_data, 0x01);
	usbpd_data->firm_ver[1] = s2mm003_indirect_read(usbpd_data, 0x02);
	usbpd_data->firm_ver[2] = s2mm003_indirect_read(usbpd_data, 0x03);
	usbpd_data->firm_ver[3] = s2mm003_indirect_read(usbpd_data, 0x04);
	dev_err(&i2c->dev, "%s otp version %02x %02x %02x %02x\n",
		USBPD_DEV_NAME, usbpd_data->firm_ver[0], usbpd_data->firm_ver[1],
		usbpd_data->firm_ver[2], usbpd_data->firm_ver[3]);
#endif

	s2mm003_firmware_update(usbpd_data);

#if defined(CONFIG_SEC_CCIC_FW_FIX)
	s2mm003_firmware_ver_check(usbpd_data, "fw");
#else
	usbpd_data->firm_ver[0] = s2mm003_indirect_read(usbpd_data, 0x01);
	usbpd_data->firm_ver[1] = s2mm003_indirect_read(usbpd_data, 0x02);
	usbpd_data->firm_ver[2] = s2mm003_indirect_read(usbpd_data, 0x03);
	usbpd_data->firm_ver[3] = s2mm003_indirect_read(usbpd_data, 0x04);
	dev_err(&i2c->dev, "%s fw version %02x %02x %02x %02x\n",
		USBPD_DEV_NAME, usbpd_data->firm_ver[0], usbpd_data->firm_ver[1],
		usbpd_data->firm_ver[2], usbpd_data->firm_ver[3]);
#endif
	Modify_TX_SRC_CAPA(usbpd_data);

	ret = request_threaded_irq(i2c->irq, NULL, s2mm003_usbpd_irq_thread,
		(IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND | IRQF_ONESHOT), "s2mm003-usbpd", usbpd_data);
	if (ret) {
		dev_err(&i2c->dev, "Failed to request IRQ %d, error %d\n", i2c->irq, ret);
		goto err_init_irq;
	}


	dev_err(&i2c->dev, "probed, irq %d\n", usbpd_data->irq_gpio);

#if defined(CONFIG_DUAL_ROLE_USB_INTF)
	desc =
		devm_kzalloc(&i2c->dev,
			     sizeof(struct dual_role_phy_desc), GFP_KERNEL);
	if (!desc) {
		pr_err("unable to allocate dual role descriptor\n");
		goto err_init_irq;
	}

	desc->name = "otg_default";
	desc->supported_modes = DUAL_ROLE_SUPPORTED_MODES_DFP_AND_UFP;
	desc->get_property = dual_role_get_local_prop;
	desc->set_property = dual_role_set_prop;
	desc->properties = fusb_drp_properties;
	desc->num_properties = ARRAY_SIZE(fusb_drp_properties);
	desc->property_is_writeable = dual_role_is_writeable;
	dual_role =
		devm_dual_role_instance_register(&i2c->dev, desc);
	dual_role->drv_data = usbpd_data;
	usbpd_data->dual_role = dual_role;
	usbpd_data->desc = desc;
	init_completion(&usbpd_data->reverse_completion);
#endif
#ifdef CONFIG_SWITCH
		/* for DockObserver */
		ret = switch_dev_register(&switch_dock);
		if (ret < 0) {
			pr_err("%s: Failed to register dock switch(%d)\n",
				__func__, ret);
			return -ENODEV;
		}
#endif /* CONFIG_SWITCH */

	return ret;

err_init_irq:
	if (i2c->irq)
		free_irq(i2c->irq, usbpd_data);
err_free_redriver_gpio:
	gpio_free(usbpd_data->redriver_en);
err_free_irq_gpio:
	gpio_free(usbpd_data->irq_gpio);
	kfree(usbpd_data);
	return ret;
}

static int s2mm003_usbpd_remove(struct i2c_client *i2c)
{
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
	struct s2mm003_data *usbpd_data = dev_get_drvdata(ccic_device);

	devm_dual_role_instance_unregister(usbpd_data->dev, usbpd_data->dual_role);
	devm_kfree(usbpd_data->dev, usbpd_data->desc);
#endif
#ifdef CONFIG_SWITCH
		/* for DockObserver */
		switch_dev_unregister(&switch_dock);
#endif /* CONFIG_SWITCH */
	return 0;
}

static const struct i2c_device_id s2mm003_usbpd_id[] = {
	{ USBPD_DEV_NAME, 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, s2mm003_usbpd_id);

#if defined(CONFIG_OF)
static struct of_device_id s2mm003_i2c_dt_ids[] = {
	{ .compatible = "sec-s2mm003,i2c" },
	{ }
};
#endif /* CONFIG_OF */

static struct i2c_driver s2mm003_usbpd_driver = {
	.driver		= {
		.name	= USBPD_DEV_NAME,
#if defined(CONFIG_OF)
		.of_match_table	= s2mm003_i2c_dt_ids,
#endif /* CONFIG_OF */
	},
	.probe		= s2mm003_usbpd_probe,
	//.remove		= __devexit_p(s2mm003_usbpd_remove),
	.remove		= s2mm003_usbpd_remove,
	.id_table	= s2mm003_usbpd_id,
};

static int __init s2mm003_usbpd_init(void)
{
	return i2c_add_driver(&s2mm003_usbpd_driver);
}
module_init(s2mm003_usbpd_init);

static void __exit s2mm003_usbpd_exit(void)
{

	i2c_del_driver(&s2mm003_usbpd_driver);
}
module_exit(s2mm003_usbpd_exit);

MODULE_DESCRIPTION("S2MM003 USB PD driver");
MODULE_LICENSE("GPL");
