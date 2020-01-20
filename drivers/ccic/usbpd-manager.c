#include <linux/ccic/core.h>
#include <linux/ccic/usbpd_msg.h>
#include <linux/ccic/usbpd_typec.h>
#include <linux/power_supply.h>
#include <linux/delay.h>

static char DP_Pin_Assignment_Print[7][40] = {
    {"DP_Pin_Assignment_None"},
    {"DP_Pin_Assignment_A"},
    {"DP_Pin_Assignment_B"},
    {"DP_Pin_Assignment_C"},
    {"DP_Pin_Assignment_D"},
    {"DP_Pin_Assignment_E"},
    {"DP_Pin_Assignment_F"},

};

void vbus_turn_on_ctrl(struct usbpd_dev *udev, bool enable)
{
	struct power_supply *psy_otg;
	union power_supply_propval val;
	int ret = 0;

	pr_info("%s %d, enable=%d\n", __func__, __LINE__, enable);
	psy_otg = power_supply_get_by_name("otg");

	if (psy_otg) {
		val.intval = enable;
		ret = psy_otg->desc->set_property(psy_otg, POWER_SUPPLY_PROP_ONLINE, &val);
	} else {
		dev_err(&udev->dev, "%s: Fail to get psy battery\n", __func__);
	}
	if (ret) {
		dev_err(&udev->dev, "%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	} else {
		dev_info(&udev->dev, "%s, otg accessory power = %d\n", __func__, enable);
	}
}

void usbpd_process_cc_water(struct usbpd_dev *udev, CCIC_WATER water_state)
{
	/* TODO
	 * notify water state
	 */
}

static int usbpd_get_src_capacity_info(struct usbpd_dev *udev)
{
	struct usbpd_desc *desc = udev->desc;
	struct usbpd_info *pd_info = &desc->pd_info;
	struct usbpd_init_data *init_data = udev->init_data;
	msg_header_type *msg_header = &pd_info->msg_header;
	data_obj_type *data_obj	= pd_info->data_obj;
	usbpd_pdic_sink_state_t *pd_sink_status = &udev->pd_noti.sink_status;
	power_supply_type PDO_type = 0;
	data_obj_type *pdo_data = NULL;
	int available_pdo_num = 0, PDO_cnt = 0;

	dev_info(&udev->dev, "    =======================================\n");
	dev_info(&udev->dev, "    MSG Header\n");

	dev_info(&udev->dev, "    Number_of_obj           : %d\n", msg_header->num_data_objs);
	dev_info(&udev->dev, "    Message_ID              : %d\n", msg_header->msg_id);
	dev_info(&udev->dev, "    Port_Power_Role         : %d\n", msg_header->port_power_role);
	dev_info(&udev->dev, "    Specification_Revision  : %d\n", msg_header->spec_revision);
	dev_info(&udev->dev, "    Port_Data_Role          : %d\n", msg_header->port_data_role);
	dev_info(&udev->dev, "    Message_Type            : %d\n", msg_header->msg_type);

	if (msg_header->msg_type != USBPD_Source_Capabilities) {
		dev_info(&udev->dev, "%s, Message Type Matching error!\n", __func__);
		return -EINVAL;
	}

	for (PDO_cnt = 0; PDO_cnt < msg_header->num_data_objs; PDO_cnt++) {
		PDO_type = data_obj[PDO_cnt].power_data_obj_supply_type.supply_type;
		pdo_data = &data_obj[PDO_cnt];
		dev_info(&udev->dev, "    =================\n");
		dev_info(&udev->dev, "    PDO_Num : %d\n", (PDO_cnt + 1));

		switch (PDO_type) {
		case POWER_TYPE_FIXED:
			if (pdo_data->power_data_obj_fixed.voltage <= (init_data->max_charging_volt/UNIT_FOR_VOLTAGE))
				available_pdo_num = PDO_cnt + 1;

			if (!(pd_info->do_power_nego) &&
				(pd_sink_status->power_list[PDO_cnt+1].max_voltage != pdo_data->power_data_obj_fixed.voltage * UNIT_FOR_VOLTAGE ||
				pd_sink_status->power_list[PDO_cnt+1].max_current != pdo_data->power_data_obj_fixed.max_current * UNIT_FOR_CURRENT))
				pd_info->do_power_nego = 1;

			pd_sink_status->power_list[PDO_cnt+1].max_voltage = pdo_data->power_data_obj_fixed.voltage * UNIT_FOR_VOLTAGE;
			pd_sink_status->power_list[PDO_cnt+1].max_current = pdo_data->power_data_obj_fixed.max_current * UNIT_FOR_CURRENT;

			dev_info(&udev->dev, "    PDO_Parameter(FIXED_SUPPLY) : %d\n", pdo_data->power_data_obj_fixed.supply_type);
			dev_info(&udev->dev, "    Dual_Role_Power         : %d\n", pdo_data->power_data_obj_fixed.dual_role_power);
			dev_info(&udev->dev, "    USB_Suspend_Support     : %d\n", pdo_data->power_data_obj_fixed.usb_suspend_support);
			dev_info(&udev->dev, "    Externally_POW          : %d\n", pdo_data->power_data_obj_fixed.externally_powered);
			dev_info(&udev->dev, "    USB_Comm_Capable        : %d\n", pdo_data->power_data_obj_fixed.usb_comm_capable);
			dev_info(&udev->dev, "    Data_Role_Swap          : %d\n", pdo_data->power_data_obj_fixed.data_role_swap);
			dev_info(&udev->dev, "    Peak_Current            : %d\n", pdo_data->power_data_obj_fixed.peak_current);
			dev_info(&udev->dev, "    Voltage_Unit            : %d\n", pdo_data->power_data_obj_fixed.voltage);
			dev_info(&udev->dev, "    Maximum_Current         : %d\n", pdo_data->power_data_obj_fixed.max_current);
			break;
		case POWER_TYPE_BATTERY:
			dev_info(&udev->dev, "    PDO_Parameter(BAT_SUPPLY)  : %d\n", pdo_data->power_data_obj_battery.supply_type);
			dev_info(&udev->dev, "    Maximum_Voltage            : %d\n", pdo_data->power_data_obj_battery.max_voltage);
			dev_info(&udev->dev, "    Minimum_Voltage            : %d\n", pdo_data->power_data_obj_battery.min_voltage);
			dev_info(&udev->dev, "    Maximum_Allow_Power        : %d\n", pdo_data->power_data_obj_battery.max_power);
			break;
		case POWER_TYPE_VARIABLE:
			dev_info(&udev->dev, "    PDO_Parameter(VAR_SUPPLY) : %d\n", pdo_data->power_data_obj_variable.supply_type);
			dev_info(&udev->dev, "    Maximum_Voltage          : %d\n", pdo_data->power_data_obj_variable.max_voltage);
			dev_info(&udev->dev, "    Minimum_Voltage          : %d\n", pdo_data->power_data_obj_variable.min_voltage);
			dev_info(&udev->dev, "    Maximum_Current          : %d\n", pdo_data->power_data_obj_variable.max_current);
			break;
		default:
			dev_err(&udev->dev, "%s, pdo message supply type error(%d)\n", __func__, PDO_type);
			break;
		}

	}

	/* the number of available pdo list */
	pd_sink_status->available_pdo_num = available_pdo_num;
	dev_info(&udev->dev, "=======================================\n");

	return available_pdo_num;
}

static void usbpd_process_pd_dr_swap(struct usbpd_dev *udev)
{
	struct usbpd_desc *desc = udev->desc;
	const struct usbpd_ops *ops = desc->ops;
	struct usbpd_info *pd_info = &desc->pd_info;

	if (desc == NULL)
		return;

	if (ops->usbpd_process_pd_dr_swap)
		return ops->usbpd_process_pd_dr_swap(udev);

	dev_info(&udev->dev, "PR_Swap requested to %s\n", pd_info->is_src ? "SOURCE" : "SINK");

	if (pd_info->dp_is_connect) {
		dev_info(&udev->dev, "dr_swap is skiped, current status is dp mode !!\n");
		return;
	}

	if (pd_info->is_host == HOST_ON_BY_RD) {
		USBPD_SEND_DNOTI(IFCONN_NOTIFY_USB, USB,
			IFCONN_NOTIFY_EVENT_DETACH, NULL);
		msleep(300);
		USBPD_SEND_DNOTI(IFCONN_NOTIFY_MUIC, ATTACH,
			IFCONN_NOTIFY_EVENT_DETACH, NULL);
		USBPD_SEND_DNOTI(IFCONN_NOTIFY_USB, USB,
			IFCONN_NOTIFY_EVENT_USB_ATTACH_UFP, NULL);
		pd_info->is_host = HOST_OFF;
		pd_info->is_client = CLIENT_ON;
	} else if (pd_info->is_client == CLIENT_ON) {
		USBPD_SEND_DNOTI(IFCONN_NOTIFY_USB, USB,
			IFCONN_NOTIFY_EVENT_DETACH, NULL);
		msleep(300);
		USBPD_SEND_DNOTI(IFCONN_NOTIFY_MUIC, ATTACH,
			IFCONN_NOTIFY_EVENT_ATTACH, NULL);
		USBPD_SEND_DNOTI(IFCONN_NOTIFY_USB, USB,
			IFCONN_NOTIFY_EVENT_USB_ATTACH_DFP, NULL);

		pd_info->is_host = HOST_ON_BY_RD;
		pd_info->is_client = CLIENT_OFF;
	}
}

static void usbpd_process_pd_pr_swap(struct usbpd_dev *udev)
{
	struct usbpd_desc *desc = udev->desc;
	const struct usbpd_ops *ops = desc->ops;
	struct usbpd_info *pd_info = &desc->pd_info;

	if (desc == NULL)
		return;

	if (ops->usbpd_process_pd_pr_swap)
		return ops->usbpd_process_pd_pr_swap(udev);

	dev_info(&udev->dev, "PR_Swap requested to %s\n", pd_info->is_src ? "SOURCE" : "SINK");

	vbus_turn_on_ctrl(udev, pd_info->is_src);

#if defined(CONFIG_DUAL_ROLE_USB_INTF)
	if (pd_info->is_src &&
		(pd_info->power_role == DUAL_ROLE_PROP_PR_SNK))
		USBPD_SEND_DNOTI(IFCONN_NOTIFY_BATTERY, ATTACH,
			IFCONN_NOTIFY_EVENT_DETACH, NULL);

	pd_info->power_role = pd_info->is_src ? DUAL_ROLE_PROP_PR_SRC : DUAL_ROLE_PROP_PR_SNK;
	USBPD_SEND_DNOTI(IFCONN_NOTIFY_BATTERY, ROLE_SWAP,
		IFCONN_NOTIFY_EVENT_DETACH, NULL);
#endif
}

static void usbpd_process_pd_src_cap(struct usbpd_dev *udev)
{
	struct usbpd_desc *desc = udev->desc;
	const struct usbpd_ops *ops = desc->ops;
	struct usbpd_info *pd_info = &desc->pd_info;
#if defined(CONFIG_IFCONN_NOTIFIER)
	struct ifconn_notifier_template *template = &udev->ifconn_template;
#endif
	int available_pdo_num = 0;
	usbpd_pdic_sink_state_t *sink_status = &udev->pd_noti.sink_status;

	pd_info->do_power_nego = 0;

	if (ops->usbpd_process_pd_src_cap)
		return ops->usbpd_process_pd_src_cap(udev);

#if defined(CONFIG_IFCONN_NOTIFIER)
	template->event = IFCONN_NOTIFY_EVENT_PD_SINK;
#endif
	available_pdo_num = usbpd_get_src_capacity_info(udev);

	dev_info(&udev->dev, "%s : Object_posision(%d), available_pdo_num(%d), selected_pdo_num(%d) \n", __func__,
			pd_info->request_power_number, available_pdo_num, sink_status->selected_pdo_num);

	sink_status->current_pdo_num = pd_info->request_power_number;

	if (available_pdo_num > 0) {
		if (pd_info->request_power_number != sink_status->selected_pdo_num) {
			if (sink_status->selected_pdo_num == 0) {
				pr_info(" %s : PDO is not selected yet by default\n", __func__);
				sink_status->selected_pdo_num = sink_status->current_pdo_num;
			}
		} else {
			if (pd_info->do_power_nego) {
				pr_info(" %s : PDO(%d) is selected, but power negotiation is requested\n",
					__func__, sink_status->selected_pdo_num);
				sink_status->selected_pdo_num = 0;
#if defined(CONFIG_IFCONN_NOTIFIER)
				template->event = IFCONN_NOTIFY_EVENT_PD_SINK_CAP;
#endif
			} else
				pr_info(" %s : PDO(%d) is selected, but same with previous list, so skip\n",
					__func__, sink_status->selected_pdo_num);
		}
		pd_info->pd_charger_attached = 1;
	} else
		pr_info(" %s : PDO is not selected\n", __func__);

#if defined(CONFIG_IFCONN_NOTIFIER)
	template->data = sink_status;

	pr_info(" %s : start sending ifconn noti\n", __func__);
	if (pd_info->pd_charger_attached)
		USBPD_SEND_DATA_NOTI(IFCONN_NOTIFY_BATTERY, POWER_STATUS,
				IFCONN_NOTIFY_EVENT_ATTACH, sink_status);
#endif
}

void usbpd_process_pd(struct usbpd_dev *udev, usbpd_msg_state_t msg_state)
{
	struct usbpd_desc *desc = udev->desc;

	if (desc == NULL)
		return;

	dev_info(&udev->dev, "%s, msg state(%d)\n", __func__, msg_state);

	switch (msg_state) {
	case USBPD_MSG_DR_SWAP:
		usbpd_process_pd_dr_swap(udev);
		break;
	case USBPD_MSG_PR_SWAP:
		usbpd_process_pd_pr_swap(udev);
		break;
	case USBPD_MSG_SRC_CAP:
		usbpd_process_pd_src_cap(udev);
		break;
	default:
		dev_err(&udev->dev, "%s, invalid msg state(%d)\n", __func__, msg_state);
	}
}

#if defined(CONFIG_USBPD_ALTERNATE_MODE)
void usbpd_dp_detach(struct usbpd_dev *udev)
{
	struct usbpd_info *pd_info = &udev->desc->pd_info;

	dev_info(&udev->dev, "%s: dp_is_connect %d\n", __func__, pd_info->dp_is_connect);

	USBPD_SEND_DATA_NOTI_DP(USB_DP, pd_info->dp_hs_connect, NULL);
	USBPD_SEND_DATA_NOTI_DP(DP_CONNECT, IFCONN_NOTIFY_EVENT_DETACH, NULL);

	pd_info->dp_is_connect = 0;
	pd_info->dp_hs_connect = 0;
	pd_info->is_sent_pin_configuration = 0;

	return;
}
#endif

int usbpd_process_cc_attach(struct usbpd_dev *udev, usbpd_attach_mode_t attach_mode)
{
	struct usbpd_desc *desc = udev->desc;
	const struct usbpd_ops *ops = desc->ops;
	struct usbpd_info *pd_info = &desc->pd_info;
	usbpd_pdic_sink_state_t *sink_status = &udev->pd_noti.sink_status;

	if (ops->usbpd_process_cc_attach)
		return ops->usbpd_process_cc_attach(udev, attach_mode);

	if (pd_info->attach_mode != attach_mode) {
		pd_info->attach_mode = attach_mode;
		switch (attach_mode) {
		case USBPD_SRC_MODE:
			dev_info(&udev->dev, "%s %d: pd attach mode:%02d, is_host = %d, is_client = %d\n",
						__func__, __LINE__, pd_info->attach_mode,
								pd_info->is_host, pd_info->is_client);
			if (pd_info->is_client == CLIENT_ON) {
				USBPD_SEND_DNOTI(IFCONN_NOTIFY_MUIC, ATTACH,
					IFCONN_NOTIFY_EVENT_DETACH, NULL);
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
				pd_info->power_role = DUAL_ROLE_PROP_PR_NONE;
#endif
				USBPD_SEND_DNOTI(IFCONN_NOTIFY_USB, USB,
					IFCONN_NOTIFY_EVENT_DETACH, NULL);

				pd_info->data_role = USB_STATUS_NOTIFY_DETACH;

				pd_info->is_client = CLIENT_OFF;
				msleep(300);
			}
			if (pd_info->is_host == HOST_OFF) {
				/* muic */
				USBPD_SEND_DNOTI(IFCONN_NOTIFY_MUIC, OTG,
					IFCONN_NOTIFY_EVENT_ATTACH, NULL);
				/* otg */
				pd_info->is_host = HOST_ON_BY_RD;
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
				pd_info->power_role = DUAL_ROLE_PROP_PR_SRC;
#endif
				USBPD_SEND_DNOTI(IFCONN_NOTIFY_USB, USB,
					IFCONN_NOTIFY_EVENT_USB_ATTACH_DFP, NULL);

				pd_info->data_role = USB_STATUS_NOTIFY_ATTACH_DFP;

				/* add to turn on external 5V */
				vbus_turn_on_ctrl(udev, VBUS_ON);
#if defined(CONFIG_USBPD_ALTERNATE_MODE)
				/* only start alternate mode at DFP state
				send_alternate_message(pd_info, VDM_DISCOVER_ID); */
#if 0
				if (pd_info->acc_type != CCIC_DOCK_DETACHED) {
					pr_info("%s: cancel_delayed_work_sync - pd_state : %d\n", __func__, pd_info->pd_state);
					cancel_delayed_work_sync(&pd_info->acc_detach_work);
				}
#endif
#endif
			}
			break;
		case USBPD_SNK_MODE:
			dev_info(&udev->dev, "%s %d: pd_state:%02d, is_host = %d, is_client = %d\n",
						__func__, __LINE__, pd_info->msg_state, pd_info->is_host, pd_info->is_client);

			if (pd_info->is_host == HOST_ON_BY_RD) {
				dev_info(&udev->dev, "%s %d: pd_state:%02d,  turn off host\n",
						__func__, __LINE__, pd_info->msg_state);
#if defined(CONFIG_USBPD_ALTERNATE_MODE)
				if (pd_info->dp_is_connect == 1) {
					usbpd_dp_detach(udev);
				}
#endif
				USBPD_SEND_DNOTI(IFCONN_NOTIFY_MUIC, ATTACH,
					IFCONN_NOTIFY_EVENT_ATTACH, NULL);

#if defined(CONFIG_DUAL_ROLE_USB_INTF)
				pd_info->power_role = DUAL_ROLE_PROP_PR_NONE;
#endif
				/* add to turn off external 5V */
				vbus_turn_on_ctrl(udev, VBUS_OFF);
				USBPD_SEND_DNOTI(IFCONN_NOTIFY_USB, USB,
					IFCONN_NOTIFY_EVENT_DETACH, NULL);

				pd_info->data_role = USB_STATUS_NOTIFY_DETACH;

				pd_info->is_host = HOST_OFF;
				msleep(300);
			}
			/* TODO check cc short w/a
			USBPD_SEND_DNOTI(IFCONN_NOTIFY_MUIC, ATTACH, DETACH,
					Func_DATA.BITS.VBUS_CC_Short? Rp_Abnormal:Func_DATA.BITS.RP_CurrentLvl);
			*/

			if (pd_info->is_client == CLIENT_OFF && pd_info->is_host == HOST_OFF) {
				/* usb */
				pd_info->is_client = CLIENT_ON;
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
				pd_info->power_role = DUAL_ROLE_PROP_PR_SNK;
#endif
				USBPD_SEND_DNOTI(IFCONN_NOTIFY_USB, USB,
					IFCONN_NOTIFY_EVENT_USB_ATTACH_UFP, NULL);
				pd_info->data_role = USB_STATUS_NOTIFY_ATTACH_UFP;
			}
			break;
		case USBPD_DETACHED:
#if defined(CONFIG_USBPD_ALTERNATE_MODE)
			if (pd_info->dp_is_connect)
				usbpd_dp_detach(udev);

#if 0
			if (pd_info->acc_type != CCIC_DOCK_DETACHED) {
				pr_info("%s: schedule_delayed_work - pd_state : %d\n", __func__, pd_info->msg_state);
				if (pd_info->acc_type == CCIC_DOCK_HMT) {
					schedule_delayed_work(&pd_info->acc_detach_work, msecs_to_jiffies(GEAR_VR_DETACH_WAIT_MS));
				} else {
					schedule_delayed_work(&pd_info->acc_detach_work, msecs_to_jiffies(0));
				}
			}
#endif
#endif
			if (pd_info->pd_charger_attached) {
				pd_info->pd_charger_attached = false;
				sink_status->selected_pdo_num = 0;
				sink_status->current_pdo_num = 0;
				USBPD_SEND_NOTI(IFCONN_NOTIFY_MANAGER, DETACH,
						IFCONN_NOTIFY_EVENT_IN_HURRY, NULL);
			}

			/* muic */
			USBPD_SEND_DNOTI(IFCONN_NOTIFY_MUIC, ATTACH,
					IFCONN_NOTIFY_EVENT_DETACH, NULL);
			if (pd_info->is_host > HOST_OFF || pd_info->is_client > CLIENT_OFF) {
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
				if (pd_info->is_host > HOST_OFF || pd_info->power_role == DUAL_ROLE_PROP_PR_SRC)
					vbus_turn_on_ctrl(udev, VBUS_OFF);
#else
				if (pd_info->is_host > HOST_OFF)
					vbus_turn_on_ctrl(udev, VBUS_OFF);
#endif
				/* usb or otg */
				dev_info(&udev->dev, "%s %d: pd_state:%02d, is_host = %d, is_client = %d\n",
						__func__, __LINE__, pd_info->msg_state, pd_info->is_host, pd_info->is_client);
				pd_info->is_host = HOST_OFF;
				pd_info->is_client = CLIENT_OFF;
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
				pd_info->power_role = DUAL_ROLE_PROP_PR_NONE;
#endif
				USBPD_SEND_DNOTI(IFCONN_NOTIFY_USB, USB,
					IFCONN_NOTIFY_EVENT_DETACH, NULL);
				pd_info->data_role = USB_STATUS_NOTIFY_DETACH;
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
				if (!pd_info->try_state_change)
					ops->usbpd_set_rprd_mode(udev, TYPE_C_ATTACH_DRP);
#endif
			}

			break;
		default:
			break;
		}
	}

	if (pd_info->try_state_change &&
		pd_info->data_role != USB_STATUS_NOTIFY_DETACH) {
		dev_info(&udev->dev, "%s, reverse_completion\n", __func__);
		complete(&udev->reverse_completion);
	}

	return 0;
}

int usbpd_process_check_accessory(struct usbpd_dev *udev)
{
	struct usbpd_desc *desc = udev->desc;
	const struct usbpd_ops *ops = desc->ops;

	if (desc == NULL)
		return -EINVAL;

	if (ops->usbpd_process_check_accessory)
		return ops->usbpd_process_check_accessory(udev);

	return 0;
}


static int usbpd_process_discover_identity(struct usbpd_dev *udev)
{
	struct usbpd_desc *desc = udev->desc;
	const struct usbpd_ops *ops = desc->ops;
	struct usbpd_info *pd_info = &desc->pd_info;

	if (desc == NULL)
		return -EINVAL;

	if (ops->usbpd_process_discover_identity)
		return ops->usbpd_process_discover_identity(udev);

	pd_info->is_sent_pin_configuration = 0;
	pd_info->is_samsung_accessory_enter_mode = 0;
	pd_info->Vendor_ID = pd_info->data_obj[1].discover_identity_id_header.usb_vendor_id;
	pd_info->Product_ID = pd_info->data_obj[3].discover_identity_product_vdo.usb_product_id;
	pd_info->Device_Version = pd_info->data_obj[3].discover_identity_product_vdo.device_version;

	dev_info(&udev->dev, "%s Vendor_ID : 0x%X, Product_ID : 0x%X Device Version 0x%X \n",\
		 __func__, pd_info->Vendor_ID, pd_info->Product_ID, pd_info->Device_Version);

	if (usbpd_process_check_accessory(udev))
		dev_info(&udev->dev, "%s : Samsung Accessory Connected.\n", __func__);

	return 0;
}

static int usbpd_process_discover_svids(struct usbpd_dev *udev)
{
	struct usbpd_desc *desc = udev->desc;
	const struct usbpd_ops *ops = desc->ops;
	struct usbpd_info *pd_info = &desc->pd_info;
	/* struct ifconn_notifier_template *template = &udev->ifconn_template; */

	if (desc == NULL)
		return -EINVAL;

	if (ops->usbpd_process_discover_svids)
		return ops->usbpd_process_discover_svids(udev);

	pd_info->SVID_0 = pd_info->data_obj[1].discover_svids_vdo.svid_0;
	pd_info->SVID_1 = pd_info->data_obj[1].discover_svids_vdo.svid_1;

	if (pd_info->SVID_0 == TypeC_DP_SUPPORT) {
		if (pd_info->is_client == CLIENT_ON) {
			USBPD_SEND_DNOTI(IFCONN_NOTIFY_MUIC, ATTACH,
				IFCONN_NOTIFY_EVENT_DETACH, NULL);
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
			pd_info->power_role = DUAL_ROLE_PROP_PR_NONE;
#endif
			USBPD_SEND_DNOTI(IFCONN_NOTIFY_USB, USB,
				IFCONN_NOTIFY_EVENT_DETACH, NULL);
			pd_info->is_client = CLIENT_OFF;
		}

		if (pd_info->is_host == HOST_OFF) {
			/* muic */
			USBPD_SEND_DNOTI(IFCONN_NOTIFY_MUIC, ATTACH,
				IFCONN_NOTIFY_EVENT_ATTACH, NULL);
			/* otg */
			pd_info->is_host = HOST_ON_BY_RD;

			USBPD_SEND_DNOTI(IFCONN_NOTIFY_USB, USB,
				IFCONN_NOTIFY_EVENT_USB_ATTACH_DFP, NULL);
		}
		pd_info->dp_is_connect = 1;
		/* If you want to support USB SuperSpeed when you connect
		 * Display port dongle, You should change dp_hs_connect depend
		 * on Pin assignment.If DP use 4lane(Pin Assignment C,E,A),
		 * dp_hs_connect is 1. USB can support HS.If DP use 2lane(Pin Assigment B,D,F), dp_hs_connect is 0. USB
		 * can support SS */
		pd_info->dp_hs_connect = 1;

		/* sub is only used here to pass the Product_ID */
		/* template->sub1 = pd_info->Product_ID; */
		/* USBPD_SEND_DATA_NOTI_DP(DP_CONNECT,
				pd_info->Vendor_ID, &pd_info->Product_ID); */
		USBPD_SEND_DATA_NOTI_DP(DP_CONNECT, IFCONN_NOTIFY_EVENT_ATTACH, pd_info);

		USBPD_SEND_DATA_NOTI_DP(USB_DP,
				pd_info->dp_hs_connect, pd_info);
	}

	dev_info(&udev->dev, "%s SVID_0 : 0x%X, SVID_1 : 0x%X\n",
		__func__, pd_info->SVID_0, pd_info->SVID_1);
	return 0;
}

static int usbpd_process_discover_modes(struct usbpd_dev *udev)
{
	struct usbpd_desc *desc = udev->desc;
	const struct usbpd_ops *ops = desc->ops;
	struct usbpd_info *pd_info = &desc->pd_info;
	/* TODO add error exception */
	u8 port_capability = pd_info->data_obj[1].discover_mode_dp_capability.port_capability;
	u8 receptacle_indication = pd_info->data_obj[1].discover_mode_dp_capability.receptacle_indication;

	if (desc == NULL)
		return -EINVAL;

	if (ops->usbpd_process_discover_modes)
		return ops->usbpd_process_discover_modes(udev);

	dev_info(&udev->dev, "%s : vendor_id = 0x%04x , svid_1 = 0x%04x\n",
			__func__, pd_info->data_obj[0].structured_vdm.svid, pd_info->SVID_1);
	if (pd_info->data_obj[0].structured_vdm.svid == TypeC_DP_SUPPORT &&
					pd_info->SVID_0 == TypeC_DP_SUPPORT) {
		dev_info(&udev->dev, "MSG_HEADER.DATA = 0x%04X\n\r",
							pd_info->msg_header.word);
		dev_info(&udev->dev, "MSG_VDM_HEADER.DATA = 0x%08X\n\r",
							pd_info->data_obj[0].object);
		dev_info(&udev->dev, "MSG_MODE_VDO_DP.DATA = 0x%08X\n\r",
							pd_info->data_obj[1].object);

		if (pd_info->msg_header.num_data_objs > 1) {
			if (((port_capability == num_UFP_D_Capable)
			      && (receptacle_indication == num_USB_TYPE_C_Receptacle))
			    ||  ((port_capability == num_DFP_D_Capable)
				  && (receptacle_indication == num_USB_TYPE_C_PLUG))) {

				pd_info->pin_assignment =
					pd_info->data_obj[1].discover_mode_dp_capability.ufp_d_pin_assignments;
				dev_info(&udev->dev, "%s support UFP_D 0x%08x\n",
						__func__, pd_info->pin_assignment);
			} else if (((port_capability == num_DFP_D_Capable)
				     && (receptacle_indication == num_USB_TYPE_C_Receptacle))
				   ||  ((port_capability == num_UFP_D_Capable)
					 && (receptacle_indication == num_USB_TYPE_C_PLUG))) {
				pd_info->pin_assignment  =
					pd_info->data_obj[1].discover_mode_dp_capability.dfp_d_pin_assignments;
				dev_info(&udev->dev, "%s support DFP_D 0x%08x\n",
							 __func__, pd_info->pin_assignment);
			} else if (port_capability == num_DFP_D_and_UFP_D_Capable) {
				if (receptacle_indication == num_USB_TYPE_C_PLUG) {
					pd_info->pin_assignment	=
						pd_info->data_obj[1].discover_mode_dp_capability.dfp_d_pin_assignments;
					dev_info(&udev->dev, "%s support DFP_D 0x%08x\n",
								__func__, pd_info->pin_assignment);
				} else {
					pd_info->pin_assignment	=
						pd_info->data_obj[1].discover_mode_dp_capability.ufp_d_pin_assignments;
					dev_info(&udev->dev, "%s support UFP_D 0x%08x\n",
								__func__, pd_info->pin_assignment);
				}
			} else{
				pd_info->pin_assignment = DP_PIN_ASSIGNMENT_NODE;
				dev_info(&udev->dev, "%s there is not valid object information!!! \n", __func__);
			}
		}
	}

	if (pd_info->data_obj[0].structured_vdm.svid == TypeC_Dex_SUPPORT && pd_info->SVID_1 == TypeC_Dex_SUPPORT) {
		dev_info(&udev->dev, "%s : dex mode discover_mode ack status!\n", __func__);
		dev_info(&udev->dev, "pDP_DIS_MODE->MSG_HEADER.DATA = 0x%04X\n\r", pd_info->msg_header.word);
		dev_info(&udev->dev, "pDP_DIS_MODE->DATA_MSG_VDM_HEADER.DATA = 0x%08X\n\r", pd_info->data_obj[0].object);
		dev_info(&udev->dev, "pDP_DIS_MODE->DATA_MSG_MODE_VDO_DP.DATA = 0x%08X\n\r", pd_info->data_obj[1].object);

		if (ops->usbpd_pd_next_state)
			ops->usbpd_pd_next_state(udev);
	}

	dev_info(&udev->dev, "%s\n", __func__);

	if (ops->usbpd_clear_discover_mode)
		ops->usbpd_clear_discover_mode(udev);

	return 0;
}

static int usbpd_process_enter_mode(struct usbpd_dev *udev)
{
	struct usbpd_desc *desc = udev->desc;
	const struct usbpd_ops *ops = desc->ops;
	struct usbpd_info *pd_info = &desc->pd_info;

	if (desc == NULL)
		return -EINVAL;

	if (ops->usbpd_process_enter_mode)
		return ops->usbpd_process_enter_mode(udev);

	if (pd_info->data_obj[0].structured_vdm.command_type == Responder_ACK) {
		dev_info(&udev->dev, "%s : EnterMode ACK.\n", __func__);
		if (pd_info->data_obj[0].structured_vdm.svid == TypeC_Dex_SUPPORT &&
						pd_info->SVID_1 == TypeC_Dex_SUPPORT) {
			pd_info->is_samsung_accessory_enter_mode = 1;
			dev_info(&udev->dev, "%s : dex mode enter_mode ack status!\n", __func__);
		}
	} else
		dev_info(&udev->dev, "%s : EnterMode NAK.\n", __func__);

	return 0;
}

static int usbpd_process_dp_status_update(struct usbpd_dev *udev)
{
	struct usbpd_desc *desc = udev->desc;
	const struct usbpd_ops *ops = desc->ops;
	struct usbpd_info *pd_info = &desc->pd_info;
	unsigned int pin_assignment = 0, pin_info = 0;
	u8 multi_func_preference = 0;
	int ret = 0, i = 0;

	if (desc == NULL)
		return -EINVAL;

	if (ops->usbpd_process_dp_status_update)
		return ops->usbpd_process_dp_status_update(udev);

	if (pd_info->SVID_0 == TypeC_DP_SUPPORT) {
		dev_info(&udev->dev, "%s DP_STATUS_UPDATE = 0x%08X\n",
					__func__, pd_info->data_obj[1].object);

		if (pd_info->data_obj[1].dp_status.port_connected == 0x00) {
			dev_info(&udev->dev, "%s : port disconnected!\n", __func__);
		} else {
			if (pd_info->is_sent_pin_configuration == 0) {
				multi_func_preference =
					pd_info->data_obj[1].dp_status.multi_function_preferred;
				if (multi_func_preference == 1) {
					if (pd_info->pin_assignment & DP_PIN_ASSIGNMENT_D) {
						pin_assignment = DP_PIN_ASSIGNMENT_D;
						pin_info = IFCONN_NOTIFY_DP_PIN_D;
					} else if (pd_info->pin_assignment & DP_PIN_ASSIGNMENT_B) {
						pin_assignment = DP_PIN_ASSIGNMENT_B;
						pin_info = IFCONN_NOTIFY_DP_PIN_B;
					} else if (pd_info->pin_assignment & DP_PIN_ASSIGNMENT_F) {
						pin_assignment = DP_PIN_ASSIGNMENT_F;
						pin_info = IFCONN_NOTIFY_DP_PIN_F;
					} else {
						pr_info("wrong pin assignment value\n");
					}
				} else {
					if (pd_info->pin_assignment & DP_PIN_ASSIGNMENT_C) {
						pin_assignment = DP_PIN_ASSIGNMENT_C;
						pin_info = IFCONN_NOTIFY_DP_PIN_C;
					} else if (pd_info->pin_assignment & DP_PIN_ASSIGNMENT_E) {
						pin_assignment = DP_PIN_ASSIGNMENT_E;
						pin_info = IFCONN_NOTIFY_DP_PIN_E;
					} else if (pd_info->pin_assignment & DP_PIN_ASSIGNMENT_A) {
						pin_assignment = DP_PIN_ASSIGNMENT_A;
						pin_info = IFCONN_NOTIFY_DP_PIN_A;
					} else if (pd_info->pin_assignment & DP_PIN_ASSIGNMENT_D) {
						pin_assignment = DP_PIN_ASSIGNMENT_D;
						pin_info = IFCONN_NOTIFY_DP_PIN_D;
					} else if (pd_info->pin_assignment & DP_PIN_ASSIGNMENT_B) {
						pin_assignment = DP_PIN_ASSIGNMENT_B;
						pin_info = IFCONN_NOTIFY_DP_PIN_B;
					} else if (pd_info->pin_assignment & DP_PIN_ASSIGNMENT_F) {
						pin_assignment = DP_PIN_ASSIGNMENT_F;
						pin_info = IFCONN_NOTIFY_DP_PIN_F;
					} else {
						pr_info("wrong pin assignment value\n");
					}
				}

				pd_info->dp_selected_pin = pin_info;

				dev_info(&udev->dev, "%s multi_func_preference %d  %s\n", __func__,
					 multi_func_preference, DP_Pin_Assignment_Print[pin_assignment]);
				if (ops->usbpd_dp_pin_assignment) {
					ret = ops->usbpd_dp_pin_assignment(udev, pin_assignment);
					if (ret < 0) {
						dev_err(&udev->dev, "%s has i2c write error.\n",
							__func__);
						return ret;
					}
				}
				pd_info->is_sent_pin_configuration = 1;
			} else {
				dev_info(&udev->dev, "%s : pin configuration is already sent as %s!\n", __func__,
					 DP_Pin_Assignment_Print[pd_info->dp_selected_pin]);
			}
		}

		if (pd_info->data_obj[1].dp_status.hpd_state == 1)
			pd_info->hpd = IFCONN_NOTIFY_HIGH;
		else
			pd_info->hpd = IFCONN_NOTIFY_LOW;

		if (pd_info->data_obj[1].dp_status.irq_hpd == 1)
			pd_info->hpdirq = IFCONN_NOTIFY_IRQ;

		USBPD_SEND_DATA_NOTI_DP(DP_HPD, pd_info->hpdirq, pd_info);
	} else {
		for (i = 0; i < 6; i++)
			dev_info(&udev->dev, "%s : VDO_%d : %d\n", __func__,
				 i + 1, pd_info->data_obj[i + 1].object);
	}
	return 0;
}

static int usbpd_process_dp_configure(struct usbpd_dev *udev)
{
	struct usbpd_desc *desc = udev->desc;
	const struct usbpd_ops *ops = desc->ops;
	struct usbpd_info *pd_info = &desc->pd_info;
	int ret = 0;

	if (desc == NULL)
		return -EINVAL;

	if (ops->usbpd_process_dp_configure)
		return ops->usbpd_process_dp_configure(udev);

	dev_info(&udev->dev, "%s : vendor_id = 0x%04x , svid_1 = 0x%04x\n", __func__,
				pd_info->data_obj[0].structured_vdm.svid, pd_info->SVID_1);

	if (pd_info->SVID_0 == TypeC_DP_SUPPORT)
		USBPD_SEND_DATA_NOTI_DP(DP_LINK_CONF, IFCONN_NOTIFY_EVENT_ATTACH, pd_info);

	if (pd_info->data_obj[0].structured_vdm.svid == TypeC_DP_SUPPORT &&
					pd_info->SVID_1 == TypeC_Dex_SUPPORT) {
		/* write s2mm005 with TypeC_Dex_SUPPORT SVID */
		/* It will start discover mode with that svid */
		dev_info(&udev->dev, "%s : svid1 is dex station\n", __func__);

		if (ops->usbpd_select_svid) {
			ret = ops->usbpd_select_svid(udev, 1);
			if (ret < 0) {
				dev_err(&udev->dev, "%s select svid error.\n",
					__func__);
				return ret;
			}
		}
	}

	return 0;
}

static int usbpd_process_attention(struct usbpd_dev *udev)
{
	struct usbpd_desc *desc = udev->desc;
	const struct usbpd_ops *ops = desc->ops;
	struct usbpd_info *pd_info = &desc->pd_info;
	unsigned int pin_assignment = 0, pin_info = 0;
	u8 multi_func_preference = 0;
	int ret = 0, i = 0;

	if (desc == NULL)
		return -EINVAL;

	if (ops->usbpd_process_attention)
		return ops->usbpd_process_attention(udev);

	if (pd_info->SVID_0 == TypeC_DP_SUPPORT) {
		dev_info(&udev->dev, "%s DP_ATTENTION = 0x%08X\n", __func__,
			 pd_info->data_obj[1].object);

		if (pd_info->is_sent_pin_configuration == 0) {
			multi_func_preference =
				pd_info->data_obj[1].dp_status.multi_function_preferred;
			if (multi_func_preference == 1) {
				if (pd_info->pin_assignment & DP_PIN_ASSIGNMENT_D) {
					pin_assignment = DP_PIN_ASSIGNMENT_D;
					pin_info = IFCONN_NOTIFY_DP_PIN_D;
				} else if (pd_info->pin_assignment & DP_PIN_ASSIGNMENT_B) {
					pin_assignment = DP_PIN_ASSIGNMENT_B;
					pin_info = IFCONN_NOTIFY_DP_PIN_B;
				} else if (pd_info->pin_assignment & DP_PIN_ASSIGNMENT_F) {
					pin_assignment = DP_PIN_ASSIGNMENT_F;
					pin_info = IFCONN_NOTIFY_DP_PIN_F;
				} else {
					pr_info("wrong pin assignment value\n");
				}
			} else {
				if (pd_info->pin_assignment & DP_PIN_ASSIGNMENT_C) {
					pin_assignment = DP_PIN_ASSIGNMENT_C;
					pin_info = IFCONN_NOTIFY_DP_PIN_C;
				} else if (pd_info->pin_assignment & DP_PIN_ASSIGNMENT_E) {
					pin_assignment = DP_PIN_ASSIGNMENT_E;
					pin_info = IFCONN_NOTIFY_DP_PIN_E;
				} else if (pd_info->pin_assignment & DP_PIN_ASSIGNMENT_A) {
					pin_assignment = DP_PIN_ASSIGNMENT_A;
					pin_info = IFCONN_NOTIFY_DP_PIN_A;
				} else if (pd_info->pin_assignment & DP_PIN_ASSIGNMENT_D) {
					pin_assignment = DP_PIN_ASSIGNMENT_D;
					pin_info = IFCONN_NOTIFY_DP_PIN_D;
				} else if (pd_info->pin_assignment & DP_PIN_ASSIGNMENT_B) {
					pin_assignment = DP_PIN_ASSIGNMENT_B;
					pin_info = IFCONN_NOTIFY_DP_PIN_B;
				} else if (pd_info->pin_assignment & DP_PIN_ASSIGNMENT_F) {
					pin_assignment = DP_PIN_ASSIGNMENT_F;
					pin_info = IFCONN_NOTIFY_DP_PIN_F;
				} else {
					pr_info("wrong pin assignment value\n");
				}
			}

			pd_info->dp_selected_pin = pin_info;

			dev_info(&udev->dev, "%s multi_func_preference %d  %s\n", __func__,
				 multi_func_preference, DP_Pin_Assignment_Print[pin_assignment]);
			if (ops->usbpd_dp_pin_assignment) {
				ret = ops->usbpd_dp_pin_assignment(udev, pin_assignment);
				if (ret < 0) {
					dev_err(&udev->dev, "%s has i2c write error.\n",
						__func__);
					return ret;
				}
			}
			pd_info->is_sent_pin_configuration = 1;
		} else {
			dev_info(&udev->dev, "%s : pin configuration is already sent as %s!\n", __func__,
					 DP_Pin_Assignment_Print[pd_info->dp_selected_pin]);
		}

		if (pd_info->data_obj[1].dp_status.hpd_state == 1)
			pd_info->hpd = IFCONN_NOTIFY_HIGH;
		else
			pd_info->hpd = IFCONN_NOTIFY_LOW;

		if (pd_info->data_obj[1].dp_status.irq_hpd == 1)
			pd_info->hpdirq = IFCONN_NOTIFY_IRQ;


		USBPD_SEND_DATA_NOTI_DP(DP_HPD, pd_info->hpdirq, pd_info);
	} else {
		for (i = 0; i < 6; i++)
			dev_info(&udev->dev, "%s : VDO_%d : %d\n", __func__,
				 i + 1, pd_info->data_obj[i + 1].object);
	}
	return 0;
}

int usbpd_process_alternate_mode(struct usbpd_dev *udev, usbpd_msg_state_t msg_state)
{
	struct usbpd_desc *desc = udev->desc;
	const struct usbpd_ops *ops = desc->ops;
	int ret = 0;

	if (desc == NULL)
		return -EINVAL;

	if (ops->usbpd_process_alternate_mode)
		return ops->usbpd_process_alternate_mode(udev, msg_state);

	dev_info(&udev->dev, "%s, mode : 0x%x\n", __func__, msg_state);

	switch (msg_state) {
	case USBPD_MSG_DISCOVER_ID:
		ret = usbpd_process_discover_identity(udev);
		break;
	case USBPD_MSG_DISCOVER_SVIDS:
		ret = usbpd_process_discover_svids(udev);
		break;
	case USBPD_MSG_DISCOVER_MODES:
		ret = usbpd_process_discover_modes(udev);
		break;
	case USBPD_MSG_ENTER_MODE:
		ret = usbpd_process_enter_mode(udev);
		break;
	case USBPD_MSG_DP_STATUS_UPDATE:
		ret = usbpd_process_dp_status_update(udev);
		break;
	case USBPD_MSG_DP_CONFIGURE:
		ret = usbpd_process_dp_configure(udev);
		break;
	case USBPD_MSG_ATTENTION:
		ret = usbpd_process_attention(udev);
		break;
	default:
		dev_err(&udev->dev, "%s, invalid vdm msg state(%d)\n", __func__, msg_state);
		break;
	}

	return ret;
}

int usbpd_process_uvdm_message(struct usbpd_dev *udev)
{
	struct usbpd_desc *desc = udev->desc;
	const struct usbpd_ops *ops = desc->ops;

	if (desc == NULL)
		return -EINVAL;

	if (ops->usbpd_process_uvdm_message)
		return ops->usbpd_process_uvdm_message(udev);

	return 0;
}
