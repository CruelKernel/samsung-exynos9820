/*
 * driver/../s2mm005.c - S2MM005 USB CC function driver
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

#include <linux/ccic/s2mm005.h>
#include <linux/ccic/s2mm005_phy.h>
#include <linux/ccic/usbpd_typec.h>
#include <linux/ccic/usbpd_msg.h>

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

static void s2mm005_usbpd_process_cc_water(void *data, LP_STATE_Type *Lp_DATA)
{
	struct s2mm005_data *usbpd_data = data;
	struct usbpd_dev *udev = usbpd_data->udev;
	struct i2c_client *i2c = usbpd_data->i2c;

	pr_info("%s\n", __func__);

	/* read reg for water and dry state */
	s2mm005_read_byte(i2c, S2MM005_REG_LP_STATE, Lp_DATA->BYTE, 4);
	dev_info(&i2c->dev, "%s: WATER reg:0x%02X WATER=%d DRY=%d\n", __func__,
		Lp_DATA->BYTE[0],
		Lp_DATA->BITS.WATER_DET,
		Lp_DATA->BITS.RUN_DRY);

#if 0
defined(CONFIG_BATTERY_SAMSUNG)
	if (usbpd_data->firm_ver[3] == 0x7 && usbpd_data->firm_ver[2] >= 0x22) { /* temp code */
		if (lpcharge) {
			dev_info(&i2c->dev, "%s: BOOTING_RUN_DRY=%d\n", __func__,
				Lp_DATA->BITS.BOOTING_RUN_DRY);
			usbpd_data->booting_run_dry  = Lp_DATA->BITS.BOOTING_RUN_DRY;
		}
	}
#endif

#if defined(CONFIG_SEC_FACTORY)
	if (!Lp_DATA->BITS.WATER_DET) {
		Lp_DATA->BITS.RUN_DRY = 1;
	}
#endif

	/* check for dry case */
	if (Lp_DATA->BITS.RUN_DRY && !usbpd_data->run_dry) {
		dev_info(&i2c->dev, "== WATER RUN-DRY DETECT ==\n");
		usbpd_process_cc_water(udev, WATER_DRY);
	}

	if (usbpd_data->firm_ver[3] >= 0x7)
		usbpd_data->run_dry = Lp_DATA->BITS.RUN_DRY;

	/* check for water case */
	if ((Lp_DATA->BITS.WATER_DET & !usbpd_data->water_det)) {
		dev_info(&i2c->dev, "== WATER DETECT ==\n");
		usbpd_process_cc_water(udev, WATER_DETECT);
	}

	usbpd_data->water_det = Lp_DATA->BITS.WATER_DET;
}

void s2mm005_usbpd_state_detect(struct s2mm005_data *usbpd_data)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	struct usbpd_dev *udev = usbpd_data->udev;
	FUNC_STATE_Type Func_DATA;

	dev_info(&udev->dev, "%s\n", __func__);

	s2mm005_read_byte(i2c, S2MM005_REG_FUNC_STATE, Func_DATA.BYTE, 4);
	dev_info(&i2c->dev, "%s, Rsvd_H:0x%02X   PD_Nxt_State:0x%02X   Rsvd_L:0x%02X   PD_State:%02d\n",
		__func__, Func_DATA.BYTES.RSP_BYTE2, Func_DATA.BYTES.PD_Next_State,
				Func_DATA.BYTES.RSP_BYTE1, Func_DATA.BYTES.PD_State);

	usbpd_data->pd_state = Func_DATA.BYTES.PD_State;
	usbpd_data->func_state = Func_DATA.DATA;

	dev_info(&i2c->dev, "func_state :0x%X, is_dfp : %d, is_src : %d\n", usbpd_data->func_state, \
		(usbpd_data->func_state & (0x1 << 26) ? 1 : 0), (usbpd_data->func_state & (0x1 << 25) ? 1 : 0));

	if (usbpd_data->pd_state !=  State_PE_Initial_detach) {
		usbpd_data->is_attached = true;
		if (usbpd_data->is_dr_swap || usbpd_data->is_pr_swap) {
			dev_info(&i2c->dev, "%s - ignore all pd_state by %s\n",
				__func__, (usbpd_data->is_dr_swap ? "dr_swap" : "pr_swap"));
			return;
		}
		switch (usbpd_data->pd_state) {
		case State_PE_SRC_Send_Capabilities:
		case State_PE_SRC_Negotiate_Capability:
		case State_PE_SRC_Transition_Supply:
		case State_PE_SRC_Ready:
		case State_PE_SRC_Disabled:
		case State_PE_SRC_Wait_New_Capabilities:
			usbpd_data->is_attached = USBPD_SRC_MODE;
			usbpd_process_cc_attach(udev, USBPD_SRC_MODE);
			break;
		case State_PE_SNK_Wait_for_Capabilities:
		case State_PE_SNK_Evaluate_Capability:
		case State_PE_SNK_Ready:
		case State_ErrorRecovery:
			usbpd_data->is_attached = USBPD_SNK_MODE;
			usbpd_process_cc_attach(udev, USBPD_SNK_MODE);
			break;
		case State_PE_PRS_SRC_SNK_Transition_to_off:
			dev_info(&i2c->dev, "%s State_PE_PRS_SRC_SNK_Transition_to_off! \n", __func__);
			vbus_turn_on_ctrl(udev, VBUS_OFF);
			break;
		case State_PE_PRS_SNK_SRC_Source_on:
			dev_info(&i2c->dev, "%s State_PE_PRS_SNK_SRC_Source_on! \n", __func__);
			vbus_turn_on_ctrl(udev, VBUS_ON);
			break;
		default:
			break;
		}
	} else {
		usbpd_data->is_dr_swap = 0;
		usbpd_data->is_pr_swap = 0;
		usbpd_data->is_attached = false;
		usbpd_data->cnt = 0;
		usbpd_process_cc_attach(udev, USBPD_DETACHED);
	}
}

void s2mm005_receive_alternate_message(struct s2mm005_data *usbpd_data,
				VDM_MSG_IRQ_STATUS_Type *VDM_MSG_IRQ_State)
{
	struct usbpd_dev *udev = usbpd_data->udev;
	struct usbpd_info *pd_info = &udev->desc->pd_info;

	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Discover_ID) {
		dev_info(&udev->dev, "%s : %s\n", __func__,
			 &VDM_MSG_IRQ_State_Print[1][0]);
		s2mm005_reg_to_msg(usbpd_data, &pd_info->msg_header,
					pd_info->data_obj, REG_RX_DIS_ID);
		usbpd_process_alternate_mode(udev, USBPD_MSG_DISCOVER_ID);
	}

	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Discover_SVIDs) {
		dev_info(&udev->dev, "%s : %s\n", __func__,
			 &VDM_MSG_IRQ_State_Print[2][0]);
		s2mm005_reg_to_msg(usbpd_data, &pd_info->msg_header,
					pd_info->data_obj, REG_RX_DIS_SVID);
		usbpd_process_alternate_mode(udev, USBPD_MSG_DISCOVER_SVIDS);
	}

	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Discover_MODEs) {
		dev_info(&udev->dev, "%s : %s\n", __func__,
			 &VDM_MSG_IRQ_State_Print[3][0]);
		s2mm005_reg_to_msg(usbpd_data, &pd_info->msg_header,
					pd_info->data_obj, REG_RX_MODE);
		usbpd_process_alternate_mode(udev, USBPD_MSG_DISCOVER_MODES);
	}

	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Enter_Mode) {
		dev_info(&udev->dev, "%s : %s\n", __func__,
			 &VDM_MSG_IRQ_State_Print[4][0]);
		s2mm005_reg_to_msg(usbpd_data, &pd_info->msg_header,
					pd_info->data_obj, REG_RX_ENTER_MODE);
		usbpd_process_alternate_mode(udev, USBPD_MSG_ENTER_MODE);
	}

	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Exit_Mode) {
		dev_info(&udev->dev, "%s : %s\n", __func__,
			 &VDM_MSG_IRQ_State_Print[5][0]);
	}

	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_DP_Status_Update) {
		dev_info(&udev->dev, "%s : %s\n", __func__,
			 &VDM_MSG_IRQ_State_Print[7][0]);
		s2mm005_reg_to_msg(usbpd_data, &pd_info->msg_header,
					pd_info->data_obj, REG_RX_DIS_DP_STATUS_UPDATE);
		usbpd_process_alternate_mode(udev, USBPD_MSG_DP_STATUS_UPDATE);
	}

	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_DP_Configure) {
		dev_info(&udev->dev, "%s : %s\n", __func__,
			 &VDM_MSG_IRQ_State_Print[8][0]);
		s2mm005_reg_to_msg(usbpd_data, &pd_info->msg_header,
					pd_info->data_obj, REG_RX_DIS_DP_CONFIGURE);
		usbpd_process_alternate_mode(udev, USBPD_MSG_DP_CONFIGURE);
	}

	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Attention) {
		dev_info(&udev->dev, "%s : %s\n", __func__,
			 &VDM_MSG_IRQ_State_Print[6][0]);
		s2mm005_reg_to_msg(usbpd_data, &pd_info->msg_header,
					pd_info->data_obj, REG_RX_DIS_ATTENTION);
		usbpd_process_alternate_mode(udev, USBPD_MSG_ATTENTION);
	}
}

void s2mm005_receive_unstructured_vdm_message(struct s2mm005_data *usbpd_data,
					SSM_MSG_IRQ_STATUS_Type *SSM_MSG_IRQ_State)
{
	/* TODO
	struct i2c_client *i2c = usbpd_data->i2c;
	msg_header_type msg_header;
	data_obj_type data_obj[7];
	int i;

	s2mm005_reg_to_msg(usbpd_data, &msg_header,
				data_obj, REG_SSM_MSG_READ);
	dev_info(&i2c->dev, "%s : VID : 0x%x\n", __func__, *VID);
	for(i = 0; i < 6; i++)
		dev_info(&i2c->dev, "%s : VDO_%d : %d\n", __func__, i + 1, VDO_MSG->VDO[i]);

	REG_ADD = REG_I2C_SLV_CMD;
	W_DATA[0] = MODE_INT_CLEAR;
	s2mm005_write_byte(i2c, REG_ADD, &W_DATA[0], 1);
	*/
}

void set_enable_alternate_mode(struct s2mm005_data *usbpd_data, int mode)
{
	static int check_is_driver_loaded = 0;
	static int prev_alternate_mode = 0;
	u8 W_DATA[2];

	if (!usbpd_data)
		return;

#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	store_usblog_notify(NOTIFY_ALTERNATEMODE, (void *)&mode, NULL);
#endif
	if ((mode & ALTERNATE_MODE_NOT_READY) && (mode & ALTERNATE_MODE_READY)) {
		pr_info("%s : mode is invalid! \n", __func__);
		return;
	}
	if ((mode & ALTERNATE_MODE_START) && (mode & ALTERNATE_MODE_STOP)) {
		pr_info("%s : mode is invalid! \n", __func__);
		return;
	}
	if (mode & ALTERNATE_MODE_RESET) {
		pr_info("%s : mode is reset! check_is_driver_loaded=%d, prev_alternate_mode=%d\n",
			__func__, check_is_driver_loaded, prev_alternate_mode);
		if (check_is_driver_loaded && (prev_alternate_mode == ALTERNATE_MODE_START)) {
			W_DATA[0] = S2MM005_MODE_INTERFACE;
			W_DATA[1] = S2MM005_SEL_START_ALT_MODE_REQ;
			s2mm005_write_byte(usbpd_data->i2c, S2MM005_REG_I2C_CMD, W_DATA, 2);
			pr_info("%s : alternate mode is reset as start! \n",	__func__);
			prev_alternate_mode = ALTERNATE_MODE_START;
		} else if (check_is_driver_loaded && (prev_alternate_mode == ALTERNATE_MODE_STOP)) {
			W_DATA[0] = S2MM005_MODE_INTERFACE;
			W_DATA[1] = S2MM005_SEL_STOP_ALT_MODE_REQ;
			s2mm005_write_byte(usbpd_data->i2c, S2MM005_REG_I2C_CMD, W_DATA, 2);
			pr_info("%s : alternate mode is reset as stop! \n", __func__);
			prev_alternate_mode = ALTERNATE_MODE_STOP;
		} else
			;
	} else {
		if (mode & ALTERNATE_MODE_NOT_READY) {
			check_is_driver_loaded = 0;
			pr_info("%s : alternate mode is not ready! \n", __func__);
		} else if (mode & ALTERNATE_MODE_READY) {
			check_is_driver_loaded = 1;
			pr_info("%s : alternate mode is ready! \n", __func__);
		} else
			;

		if (check_is_driver_loaded) {
			if (mode & ALTERNATE_MODE_START) {
				W_DATA[0] = S2MM005_MODE_INTERFACE;
				W_DATA[1] = S2MM005_SEL_START_ALT_MODE_REQ;
				s2mm005_write_byte(usbpd_data->i2c, S2MM005_REG_I2C_CMD, W_DATA, 2);
				pr_info("%s : alternate mode is started! \n",	__func__);
				prev_alternate_mode = ALTERNATE_MODE_START;
			} else if (mode & ALTERNATE_MODE_STOP) {
				W_DATA[0] = S2MM005_MODE_INTERFACE;
				W_DATA[1] = S2MM005_SEL_STOP_ALT_MODE_REQ;
				s2mm005_write_byte(usbpd_data->i2c, S2MM005_REG_I2C_CMD, W_DATA, 2);
				pr_info("%s : alternate mode is stopped! \n", __func__);
				prev_alternate_mode = ALTERNATE_MODE_STOP;
			}
		}
	}
	return;
}

void s2mm005_usbpd_msg_detect(struct s2mm005_data *usbpd_data)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	struct usbpd_info *pd_info = &usbpd_data->udev->desc->pd_info;
	uint8_t	R_INT_STATUS[48];
	uint32_t *pPRT_MSG = NULL;
	MSG_IRQ_STATUS_Type	MSG_IRQ_State;
	VDM_MSG_IRQ_STATUS_Type VDM_MSG_IRQ_State;
	SSM_MSG_IRQ_STATUS_Type SSM_MSG_IRQ_State;
	AP_REQ_GET_STATUS_Type AP_REQ_GET_State;
	msg_header_type msg_header;
	data_obj_type data_obj[7];

	pr_info("%s\n", __func__);

	s2mm005_read_byte(i2c, S2MM005_REG_SYNC_STATUS, R_INT_STATUS, 48);

	s2mm005_int_clear(usbpd_data);

	pPRT_MSG = (uint32_t *)&R_INT_STATUS[0];
	dev_info(&i2c->dev, "SYNC     Status = 0x%08X\n", pPRT_MSG[0]);
	dev_info(&i2c->dev, "CTRL MSG Status = 0x%08X\n", pPRT_MSG[1]);
	dev_info(&i2c->dev, "DATA MSG Status = 0x%08X\n", pPRT_MSG[2]);
	dev_info(&i2c->dev, "EXTD MSG Status = 0x%08X\n", pPRT_MSG[3]);
	dev_info(&i2c->dev, "MSG IRQ Status = 0x%08X\n", pPRT_MSG[4]);
	dev_info(&i2c->dev, "VDM IRQ Status = 0x%08X\n", pPRT_MSG[5]);
	dev_info(&i2c->dev, "SSM_MSG IRQ Status = 0x%08X\n", pPRT_MSG[6]);
	dev_info(&i2c->dev, "AP REQ GET Status = 0x%08X\n", pPRT_MSG[7]);

	dev_info(&i2c->dev, "0x50 IRQ Status = 0x%08X\n", pPRT_MSG[8]);
	dev_info(&i2c->dev, "0x54 IRQ Status = 0x%08X\n", pPRT_MSG[9]);
	dev_info(&i2c->dev, "0x58 IRQ Status = 0x%08X\n", pPRT_MSG[10]);
	MSG_IRQ_State.DATA = pPRT_MSG[4];
	VDM_MSG_IRQ_State.DATA = pPRT_MSG[5];
	SSM_MSG_IRQ_State.DATA = pPRT_MSG[6];
	AP_REQ_GET_State.DATA = pPRT_MSG[7];

	if (MSG_IRQ_State.BITS.Ctrl_Flag_DR_Swap) {
		usbpd_data->is_dr_swap++;
		dev_info(&i2c->dev, "is_dr_swap count : 0x%x\n", usbpd_data->is_dr_swap);
		usbpd_process_pd(usbpd_data->udev, USBPD_MSG_DR_SWAP);
	}

	if (MSG_IRQ_State.BITS.Ctrl_Flag_PR_Swap) {
		usbpd_data->is_pr_swap = 1;
		dev_info(&i2c->dev, "is_pr_swap count : 0x%x\n", usbpd_data->is_pr_swap);
		usbpd_process_pd(usbpd_data->udev, USBPD_MSG_PR_SWAP);
	}

	if (MSG_IRQ_State.BITS.Data_Flag_SRC_Capability) {
		s2mm005_reg_to_msg(usbpd_data, &msg_header,
				data_obj, REG_TX_REQUEST_MSG);
		pd_info->request_power_number =
				data_obj[0].request_data_object.object_position;
		s2mm005_reg_to_msg(usbpd_data, &pd_info->msg_header,
				pd_info->data_obj, REG_RX_SRC_CAPA_MSG);
		usbpd_process_pd(usbpd_data->udev, USBPD_MSG_SRC_CAP);
	}
/*
	if (MSG_IRQ_State.BITS.CTRL_Flag_Get_Sink_Cap)
		pr_info(" %s : SRC requested SINK Cap\n", __func__);
*/

#if defined(CONFIG_CCIC_ALTERNATE_MODE)
	if (VDM_MSG_IRQ_State.DATA)
		s2mm005_receive_alternate_message(usbpd_data, &VDM_MSG_IRQ_State);
	if (SSM_MSG_IRQ_State.BITS.Ssm_Flag_Unstructured_Data)
		s2mm005_receive_unstructured_vdm_message(usbpd_data, &SSM_MSG_IRQ_State);

	/* TODO set_enable_alternate_mode func */
	if (!AP_REQ_GET_State.BITS.Alt_Mode_By_I2C)
		set_enable_alternate_mode(usbpd_data, ALTERNATE_MODE_RESET);
#endif
}

void s2mm005_usbpd_rid_detect(struct s2mm005_data *usbpd_data)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	int prev_rid = usbpd_data->cur_rid;
	u8 rid = 0;

	s2mm005_read_byte(i2c, S2MM005_REG_RID, &rid, 1);
	dev_info(&i2c->dev, "prev_rid : %x, rid : %x\n", prev_rid, rid);
	usbpd_data->cur_rid = rid;
}

static void s2mm005_usbpd_check_reset(struct s2mm005_data *usbpd_data, LP_STATE_Type *Lp_DATA)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	FUNC_STATE_Type Func_DATA;

	s2mm005_read_byte(i2c, S2MM005_REG_FUNC_STATE, Func_DATA.BYTE, 4);
	dev_info(&i2c->dev, "Rsvd_H:0x%02X   PD_Nxt_State:0x%02X   Rsvd_L:0x%02X   PD_State:%02d\n",
		Func_DATA.BYTES.RSP_BYTE2,
		Func_DATA.BYTES.PD_Next_State,
		Func_DATA.BYTES.RSP_BYTE1,
		Func_DATA.BYTES.PD_State);

	if (Func_DATA.BITS.RESET) {
		dev_info(&i2c->dev, "ccic reset detected\n");
		if (!Lp_DATA->BITS.AUTO_LP_ENABLE_BIT) {
			/* AUTO LPM Enable */
			s2mm005_manual_LPM(usbpd_data, 6);
		}
	}
}

void s2mm005_usbpd_process_cc_water_det(struct s2mm005_data *usbpd_data)
{
	pr_info("%s\n", __func__);
	s2mm005_int_clear(usbpd_data);
#if defined(CONFIG_SEC_FACTORY)
	if (!usbpd_data->fac_water_enable)
#endif
	{
		if (usbpd_data->water_det)
			s2mm005_manual_LPM(usbpd_data, 0x9);
	}
}

void s2mm005_usbpd_check_pd_state(struct s2mm005_data *usbpd_data)
{
	LP_STATE_Type Lp_DATA;

	s2mm005_usbpd_process_cc_water(usbpd_data, &Lp_DATA);

	if (usbpd_data->water_det || !usbpd_data->run_dry || !usbpd_data->booting_run_dry) {
		s2mm005_usbpd_process_cc_water_det(usbpd_data);
		return;
	}

	if (usbpd_data->cnt == 0) {
		usbpd_data->cnt++;
		set_enable_alternate_mode(usbpd_data, ALTERNATE_MODE_START | ALTERNATE_MODE_READY);
	}

	s2mm005_usbpd_check_reset(usbpd_data, &Lp_DATA);

	s2mm005_usbpd_state_detect(usbpd_data);

	if (usbpd_data->is_attached == false)
		return;

	s2mm005_usbpd_msg_detect(usbpd_data);

	s2mm005_usbpd_rid_detect(usbpd_data);

	return;
}

void s2mm005_hard_reset(struct s2mm005_data *usbpd_data)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	struct device *i2c_dev = i2c->dev.parent->parent;

	struct pinctrl *i2c_pinctrl;

	i2c_lock_adapter(i2c->adapter);
	i2c_pinctrl = devm_pinctrl_get_select(i2c_dev, "hard_reset");
	if (IS_ERR(i2c_pinctrl))
		pr_err("could not set reset pins\n");
	printk("hard_reset: %04d %1d %01d\n", __LINE__, gpio_get_value(usbpd_data->s2mm005_sda), gpio_get_value(usbpd_data->s2mm005_scl));

	usleep_range(10 * 1000, 10 * 1000);
	i2c_pinctrl = devm_pinctrl_get_select(i2c_dev, "default");
	if (IS_ERR(i2c_pinctrl))
		pr_err("could not set default pins\n");
	usleep_range(8 * 1000, 8 * 1000);
	i2c_unlock_adapter(i2c->adapter);
	printk("hard_reset: %04d %1d %01d\n", __LINE__, gpio_get_value(usbpd_data->s2mm005_sda), gpio_get_value(usbpd_data->s2mm005_scl));
}

void s2mm005_manual_JIGON(struct s2mm005_data *usbpd_data, int mode)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 W_DATA[2], R_DATA;
	int i;
	pr_info("usb: %s mode=%s (fw=0x%x)\n", __func__, mode ? "High":"Low", usbpd_data->firm_ver[2]);
	/* for Wake up*/
	for (i = 0; i < 5; i++) {
		R_DATA = 0x00;
		s2mm005_read_byte(i2c, S2MM005_REG_SW_VER, &R_DATA, 1);  /* dummy read */
	}
	udelay(10);

	W_DATA[0] = S2MM005_MODE_SELECT_SLEEP;
	if (mode)
		W_DATA[1] = S2MM005_JIGON_HIGH;   /* JIGON High */
	else
		W_DATA[1] = S2MM005_JIGON_LOW;   /* JIGON Low */

	s2mm005_write_byte(i2c, S2MM005_REG_I2C_CMD, W_DATA, 2);

}

void s2mm005_manual_LPM(struct s2mm005_data *usbpd_data, int cmd)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 W_DATA[2];
	u8 R_DATA;
	int i;
	pr_info("usb: %s cmd=0x%x (fw=0x%x)\n", __func__, cmd, usbpd_data->firm_ver[2]);

	/* for Wake up*/
	for (i = 0; i < 5; i++) {
		R_DATA = 0x00;
		s2mm005_read_byte(i2c, S2MM005_REG_SW_VER, &R_DATA, 1);  /* dummy read */
	}
	udelay(10);

	W_DATA[0] = S2MM005_MODE_SELECT_SLEEP;
	W_DATA[1] = cmd;
	s2mm005_write_byte(i2c, S2MM005_REG_I2C_CMD, &W_DATA[0], 2);
}

void s2mm005_control_option_command(struct s2mm005_data *usbpd_data, int cmd)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 W_DATA[2];
	u8 R_DATA;
	int i;
	printk("usb: %s cmd=0x%x (fw=0x%x)\n", __func__, cmd, usbpd_data->firm_ver[2]);

	/* for Wake up*/
	for (i = 0; i < 5; i++) {
		R_DATA = 0x00;
		s2mm005_read_byte(i2c, S2MM005_REG_SW_VER, &R_DATA, 1);  /* dummy read */
	}
	udelay(10);

/* 0x81 : Vconn control option command ON
** 0x82 : Vconn control option command OFF
** 0x83 : Water Detect option command ON
** 0x84 : Water Detect option command OFF
*/
#if defined(CONFIG_SEC_FACTORY)
	if ((cmd & 0xF) == 0x3)
		usbpd_data->fac_water_enable = 1;
	else if ((cmd & 0xF) == 0x4)
		usbpd_data->fac_water_enable = 0;
#endif

	W_DATA[0] = S2MM005_MODE_INTERFACE;
	W_DATA[1] = 0x80 | (cmd & 0xF);
	s2mm005_write_byte(i2c, S2MM005_REG_I2C_CMD, W_DATA, 2);
}

void s2mm005_usbpd_sink_capable_init(struct s2mm005_data *usbpd_data)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	struct usbpd_dev *udev = usbpd_data->udev;
	struct usbpd_init_data *init_data = udev->init_data;
	uint8_t MSG_BUF[32] = {0,};
	data_obj_type *data_obj;
	uint32_t *MSG_DATA;
	int cnt = 0, ret = 0;

	if (init_data == NULL) {
		dev_err(&i2c->dev, "%s, init data is Null pointer\n", __func__);
		return;
	}

	ret = s2mm005_read_byte(i2c, REG_TX_SINK_CAPA_MSG, MSG_BUF, 32);

	MSG_DATA = (uint32_t *)&MSG_BUF[0];
	dev_info(&i2c->dev, "--- Read Data on TX_SNK_CAPA_MSG(0x220)\n\r");
	for (cnt = 0; cnt < 8; cnt++)
		dev_info(&i2c->dev, "   0x%08X\n\r", MSG_DATA[cnt]);

	data_obj = (data_obj_type *)&MSG_BUF[8];
	data_obj->power_data_obj_variable.max_current = init_data->sink_cap_operational_current;
	data_obj->power_data_obj_variable.min_voltage = init_data->sink_cap_min_volt;
	data_obj->power_data_obj_variable.max_voltage = init_data->sink_cap_max_volt;
	data_obj->power_data_obj_variable.supply_type = init_data->sink_cap_power_type;

	dev_info(&i2c->dev, "--- Write DATA\n\r");
	for (cnt = 0; cnt < 8; cnt++)
		dev_info(&i2c->dev, "   0x%08X\n\r", MSG_DATA[cnt]);

	s2mm005_write_byte(i2c, REG_TX_SINK_CAPA_MSG, &MSG_BUF[0], 32);

	for (cnt = 0; cnt < 32; cnt++)
		MSG_BUF[cnt] = 0;

	for (cnt = 0; cnt < 8; cnt++)
		dev_info(&i2c->dev, "   0x%08X\n\r", MSG_DATA[cnt]);

	ret = s2mm005_read_byte(i2c, REG_TX_SINK_CAPA_MSG, MSG_BUF, 32);

	dev_info(&i2c->dev, "--- Read 2 new Data on TX_SNK_CAPA_MSG(0x220)\n\r");
	for (cnt = 0; cnt < 8; cnt++)
		dev_info(&i2c->dev, "   0x%08X\n\r", MSG_DATA[cnt]);
}

void s2mm005_reset(struct s2mm005_data *usbpd_data)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 buf;
	u8 R_DATA;
	int i;

	pr_info("%s\n", __func__);
	/* for Wake up*/
	for (i = 0; i < 5; i++) {
		R_DATA = 0x00;
		s2mm005_read_byte(i2c, S2MM005_REG_SW_VER, &R_DATA, 1);
	}
	udelay(10);

	printk("%s\n", __func__);

	buf = 0x01;
	s2mm005_sel_write(i2c, 0x101C, &buf, SEL_WRITE_BYTE);
	/* reset stable time */
	msleep(100);
}

void s2mm005_reset_enable(struct s2mm005_data *usbpd_data)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 buf;

	printk("%s\n", __func__);

	buf = 0x01;
	s2mm005_sel_write(i2c, 0x105C, &buf, SEL_WRITE_BYTE);
}

void s2mm005_reg_to_msg(struct s2mm005_data *usbpd_data, msg_header_type *msg_header,
						data_obj_type *data_obj, int reg)
{
	struct i2c_client *i2c = usbpd_data->i2c;
	uint8_t ReadMSG[32];
	uint32_t *pPRT_MSG;

	s2mm005_read_byte(i2c, reg, ReadMSG, 32);

	msg_header->byte[0] = ReadMSG[0];
	msg_header->byte[1] = ReadMSG[1];

	pPRT_MSG = (uint32_t *)&ReadMSG[4];

	data_obj[0].object = pPRT_MSG[0];
	data_obj[1].object = pPRT_MSG[1];
	data_obj[2].object = pPRT_MSG[2];
	data_obj[3].object = pPRT_MSG[3];
	data_obj[4].object = pPRT_MSG[4];
	data_obj[5].object = pPRT_MSG[5];
	data_obj[6].object = pPRT_MSG[6];
}

void s2mm005_usbpd_phy_init(struct s2mm005_data *usbpd_data)
{
#ifdef CONFIG_CCIC_LPM_ENABLE
	struct i2c_client *i2c = usbpd_data->i2c;
	u8 check[8] = {0,};
#endif

	s2mm005_usbpd_sink_capable_init(usbpd_data);

#ifdef CONFIG_CCIC_LPM_ENABLE
	check[0] = S2MM005_MODE_INTERFACE;
	check[1] = 0x42;
	s2mm005_write_byte(i2c, S2MM005_REG_I2C_CMD, &check[0], 2);

	dev_info(usbpd_data->dev, "LPM_ENABLE\n");
	check[0] = S2MM005_MODE_SELECT_SLEEP;
	check[1] = S2MM005_AUTO_LP_ENABLE;
	s2mm005_write_byte(i2c, S2MM005_REG_I2C_CMD, &check[0], 2);
#endif
}
