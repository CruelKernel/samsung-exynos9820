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
#include <linux/power_supply.h>
#include <linux/of.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/mfd/max77705-private.h>
#include <linux/platform_device.h>
#include <linux/ccic/max77705_usbc.h>
#if defined(CONFIG_CCIC_NOTIFIER)
#include <linux/workqueue.h>
#include <linux/ccic/ccic_notifier.h>
#endif
#include <linux/ccic/ccic_core.h>
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
#include <linux/usb/class-dual-role.h>
#elif defined(CONFIG_TYPEC)
#include <linux/usb/typec.h>
#endif
#if defined(CONFIG_USB_HOST_NOTIFY)
#include <linux/usb_notify.h>
#endif
#include "../battery_v2/include/sec_charging_common.h"
#include <linux/ccic/max77705_alternate.h>
#if defined(CONFIG_COMBO_REDRIVER_PTN36502)
#include <linux/combo_redriver/ptn36502.h>
#endif

extern struct pdic_notifier_struct pd_noti;

#if defined(CONFIG_CCIC_NOTIFIER)
static void max77705_ccic_event_notifier(struct work_struct *data)
{
	struct ccic_state_work *event_work =
		container_of(data, struct ccic_state_work, ccic_work);
	CC_NOTI_TYPEDEF ccic_noti;

	switch (event_work->dest) {
	case CCIC_NOTIFY_DEV_USB:
			msg_maxim("usb: dest=%s, id=%s, attach=%s, drp=%s",
				CCIC_NOTI_DEST_Print[event_work->dest],
				CCIC_NOTI_ID_Print[event_work->id],
				event_work->attach ? "Attached" : "Detached",
				CCIC_NOTI_USB_STATUS_Print[event_work->event]);
			break;
	default:
			msg_maxim("usb: dest=%s, id=%s, attach=%d, event=%d",
				CCIC_NOTI_DEST_Print[event_work->dest],
				CCIC_NOTI_ID_Print[event_work->id],
				event_work->attach,
				event_work->event);
			break;
	}

	ccic_noti.src = CCIC_NOTIFY_DEV_CCIC;
	ccic_noti.dest = event_work->dest;
	ccic_noti.id = event_work->id;
	ccic_noti.sub1 = event_work->attach;
	ccic_noti.sub2 = event_work->event;
	ccic_noti.sub3 = event_work->sub;
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	ccic_noti.pd = &pd_noti;
#endif
	ccic_notifier_notify((CC_NOTI_TYPEDEF *) &ccic_noti, NULL, 0);

	kfree(event_work);
}

void max77705_ccic_event_work(void *data, int dest, int id, int attach, int event, int sub)
{
	struct max77705_usbc_platform_data *usbpd_data = data;
	struct ccic_state_work *event_work;
#if defined(CONFIG_TYPEC)
	struct typec_partner_desc desc;
	enum typec_pwr_opmode mode = TYPEC_PWR_MODE_USB;
#endif

	msg_maxim("usb: DIAES %d-%d-%d-%d-%d", dest, id, attach, event, sub);
	event_work = kmalloc(sizeof(struct ccic_state_work), GFP_ATOMIC);
	INIT_WORK(&event_work->ccic_work, max77705_ccic_event_notifier);

	event_work->dest = dest;
	event_work->id = id;
	event_work->attach = attach;
	event_work->event = event;
	event_work->sub = sub;
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
	if (id == CCIC_NOTIFY_ID_USB) {
		msg_maxim("usb: dest=%d, event=%d", dest, event);
		msg_maxim("usb: pd->data_role=%d, pd->try_state_change=%d",
				usbpd_data->data_role,
				usbpd_data->try_state_change);

		usbpd_data->data_role = event;

		if (usbpd_data->dual_role != NULL)
			dual_role_instance_changed(usbpd_data->dual_role);

		if (usbpd_data->try_state_change &&
			(usbpd_data->data_role != USB_STATUS_NOTIFY_DETACH)) {
			/* Role change try and new mode detected */
			msg_maxim("usb: reverse_completion");
			usbpd_data->try_state_change = TYPE_C_DETACH;
			complete(&usbpd_data->reverse_completion);
		}
	} else if (id == CCIC_NOTIFY_ID_ROLE_SWAP) {
		if (usbpd_data->dual_role != NULL)
			dual_role_instance_changed(usbpd_data->dual_role);
	}
#elif defined(CONFIG_TYPEC)
	if (id == CCIC_NOTIFY_ID_USB) {
		if (usbpd_data->partner == NULL) {
			msg_maxim("typec_register_partner, typec_power_role=%d typec_data_role=%d event=%d",
				usbpd_data->typec_power_role,usbpd_data->typec_data_role, event);
			if (event == USB_STATUS_NOTIFY_ATTACH_UFP) {
				mode = max77705_get_pd_support(usbpd_data);
				typec_set_pwr_opmode(usbpd_data->port, mode);
				desc.usb_pd = mode == TYPEC_PWR_MODE_PD;
				desc.accessory = TYPEC_ACCESSORY_NONE; /* XXX: handle accessories */
				desc.identity = NULL;
				usbpd_data->typec_data_role = TYPEC_DEVICE;
				typec_set_pwr_role(usbpd_data->port, usbpd_data->typec_power_role);
				typec_set_data_role(usbpd_data->port, usbpd_data->typec_data_role);
				usbpd_data->partner = typec_register_partner(usbpd_data->port, &desc);
			} else if (event == USB_STATUS_NOTIFY_ATTACH_DFP) {
				mode = max77705_get_pd_support(usbpd_data);
				typec_set_pwr_opmode(usbpd_data->port, mode);
				desc.usb_pd = mode == TYPEC_PWR_MODE_PD;
				desc.accessory = TYPEC_ACCESSORY_NONE; /* XXX: handle accessories */
				desc.identity = NULL;
				usbpd_data->typec_data_role = TYPEC_HOST;
				typec_set_pwr_role(usbpd_data->port, usbpd_data->typec_power_role);
				typec_set_data_role(usbpd_data->port, usbpd_data->typec_data_role);
				usbpd_data->partner = typec_register_partner(usbpd_data->port, &desc);
			} else
				msg_maxim("detach case");
		} else {
			msg_maxim("data_role changed, typec_power_role=%d typec_data_role=%d, event=%d",
				usbpd_data->typec_power_role,usbpd_data->typec_data_role, event);
			if (event == USB_STATUS_NOTIFY_ATTACH_UFP) {
				usbpd_data->typec_data_role = TYPEC_DEVICE;
				typec_set_data_role(usbpd_data->port, usbpd_data->typec_data_role);
			} else if (event == USB_STATUS_NOTIFY_ATTACH_DFP) {
				usbpd_data->typec_data_role = TYPEC_HOST;
				typec_set_data_role(usbpd_data->port, usbpd_data->typec_data_role);
			} else
				msg_maxim("detach case");
		}
	}
#endif
	queue_work(usbpd_data->ccic_wq, &event_work->ccic_work);
}
#endif

void max77705_dp_detach(void *data)
{
	struct max77705_usbc_platform_data *usbpd_data = data;

	pr_info("%s: dp_is_connect %d\n", __func__, usbpd_data->dp_is_connect);

	max77705_ccic_event_work(usbpd_data, CCIC_NOTIFY_DEV_USB_DP,
		CCIC_NOTIFY_ID_USB_DP, 0/*attach*/, usbpd_data->dp_hs_connect/*drp*/, 0);
	max77705_ccic_event_work(usbpd_data, CCIC_NOTIFY_DEV_DP,
		CCIC_NOTIFY_ID_DP_CONNECT, 0/*attach*/, 0/*drp*/, 0);

	usbpd_data->dp_is_connect = 0;
	usbpd_data->dp_hs_connect = 0;
	usbpd_data->is_sent_pin_configuration = 0;
}

void max77705_notify_dr_status(struct max77705_usbc_platform_data *usbpd_data, uint8_t attach)
{
	struct max77705_pd_data *pd_data = usbpd_data->pd_data;

	msg_maxim("Data Role: %s Power Role: %s State: %s",
		pd_data->current_dr ? "DFP":"UFP",
		usbpd_data->cc_data->current_pr ? "SRC":"SNK",
		attach ? "ATTACHED":"DETACHED");

	if (attach == CCIC_NOTIFY_ATTACH) {
		if (usbpd_data->current_connstat == WATER) {
			pr_info("%s: blocked by WATER\n", __func__);
			return;
		}
		if (pd_data->current_dr == UFP) {
			if (usbpd_data->is_host == HOST_ON) {
				msg_maxim("pd_state:%02d,	turn off host",
						usbpd_data->pd_state);
				if (usbpd_data->dp_is_connect == 1)
					max77705_dp_detach(usbpd_data);
				max77705_ccic_event_work(usbpd_data,
						CCIC_NOTIFY_DEV_USB, CCIC_NOTIFY_ID_USB,
						0/*attach*/, USB_STATUS_NOTIFY_DETACH/*drp*/, 0);
				usbpd_data->is_host = HOST_OFF;
			}
			if (usbpd_data->is_client == CLIENT_OFF) {
				usbpd_data->is_client = CLIENT_ON;
				/* muic */
				max77705_ccic_event_work(usbpd_data,
					CCIC_NOTIFY_DEV_MUIC, CCIC_NOTIFY_ID_ATTACH,
					1/*attach*/, 0/*rprd*/, 0);
				/* USB */
				max77705_ccic_event_work(usbpd_data,
						CCIC_NOTIFY_DEV_USB, CCIC_NOTIFY_ID_USB,
						1/*attach*/, USB_STATUS_NOTIFY_ATTACH_UFP/*drp*/, 0);
			}

		} else if (pd_data->current_dr == DFP) {
			if (usbpd_data->is_client == CLIENT_ON) {
				msg_maxim("pd_state:%02d, turn off client",
						usbpd_data->pd_state);
				max77705_ccic_event_work(usbpd_data,
					CCIC_NOTIFY_DEV_USB, CCIC_NOTIFY_ID_USB,
					0/*attach*/, USB_STATUS_NOTIFY_DETACH/*drp*/, 0);
				usbpd_data->is_client = CLIENT_OFF;
			}
			if (usbpd_data->is_host == HOST_OFF) {
				usbpd_data->is_host = HOST_ON;
#if defined(CONFIG_USB_AUDIO_ENHANCED_DETECT_TIME)
				max77705_clk_booster_set(usbpd_data, 1);
#endif
				/* muic */
				max77705_ccic_event_work(usbpd_data,
					CCIC_NOTIFY_DEV_MUIC,
					CCIC_NOTIFY_ID_ATTACH, 1/*attach*/, 1/*rprd*/, 0);
				/* USB */
				max77705_ccic_event_work(usbpd_data,
					CCIC_NOTIFY_DEV_USB, CCIC_NOTIFY_ID_USB,
					1/*attach*/, USB_STATUS_NOTIFY_ATTACH_DFP/*drp*/, 0);
			}

		} else {
			msg_maxim("Unknown dr type (%d) no action",
				pd_data->current_dr);
		}
	} else { /* CCIC_NOTIFY_DETACH  */
		if (usbpd_data->dp_is_connect == 1)
			max77705_dp_detach(usbpd_data);
		if (usbpd_data->acc_type != CCIC_DOCK_DETACHED) {
			pr_info("%s: schedule_delayed_work - pd_state : %d\n",
					__func__, usbpd_data->pd_state);
			if (usbpd_data->acc_type == CCIC_DOCK_HMT)
				schedule_delayed_work(&usbpd_data->acc_detach_work,
					msecs_to_jiffies(GEAR_VR_DETACH_WAIT_MS));
			else
				schedule_delayed_work(&usbpd_data->acc_detach_work,
					msecs_to_jiffies(0));
		}
#if defined(CONFIG_USB_AUDIO_ENHANCED_DETECT_TIME)
		max77705_clk_booster_set(usbpd_data, 0);
#endif
		usbpd_data->mdm_block = 0;
		usbpd_data->is_host = HOST_OFF;
		usbpd_data->is_client = CLIENT_OFF;
		/* muic */
		max77705_ccic_event_work(usbpd_data,
			CCIC_NOTIFY_DEV_MUIC, CCIC_NOTIFY_ID_ATTACH,
			0/*attach*/, 0/*rprd*/, 0);
		/* USB */
		max77705_ccic_event_work(usbpd_data,
			CCIC_NOTIFY_DEV_USB, CCIC_NOTIFY_ID_USB,
			0/*attach*/, USB_STATUS_NOTIFY_DETACH/*drp*/, 0);
	}

}

static irqreturn_t max77705_vconncop_irq(int irq, void *data)
{
	struct max77705_usbc_platform_data *usbc_data = data;
	struct max77705_cc_data *cc_data = usbc_data->cc_data;

	max77705_read_reg(usbc_data->muic, REG_CC_STATUS1, &cc_data->cc_status1);
	cc_data->vconnocp = (cc_data->cc_status1 & BIT_VCONNOCPI)
			>> FFS(BIT_VCONNOCPI);
	msg_maxim("New VCONNOCP Status Interrupt (%d)",
		cc_data->vconnocp);

	return IRQ_HANDLED;
}

static irqreturn_t max77705_vsafe0v_irq(int irq, void *data)
{
	struct max77705_usbc_platform_data *usbc_data = data;
	struct max77705_cc_data *cc_data = usbc_data->cc_data;
	u8 ccpinstat = 0;
	u8 connstat = 0;

	pr_debug("%s: IRQ(%d)_IN\n", __func__, irq);
	max77705_read_reg(usbc_data->muic, REG_BC_STATUS, &cc_data->bc_status);
	max77705_read_reg(usbc_data->muic, REG_CC_STATUS0, &cc_data->cc_status0);
	max77705_read_reg(usbc_data->muic, REG_CC_STATUS1, &cc_data->cc_status1);
	ccpinstat = (cc_data->cc_status0 & BIT_CCPinStat)
				>> FFS(BIT_CCPinStat);
	cc_data->vsafe0v = (cc_data->cc_status1 & BIT_VSAFE0V)
				>> FFS(BIT_VSAFE0V);
	connstat = (cc_data->cc_status1 & BIT_ConnStat)
				>> FFS(BIT_ConnStat);

	msg_maxim("New VSAFE0V Status Interrupt (%d)",
		cc_data->vsafe0v);
	pr_debug("%s: IRQ(%d)_OUT\n", __func__, irq);

	return IRQ_HANDLED;
}


static irqreturn_t max77705_vconnsc_irq(int irq, void *data)
{
	struct max77705_usbc_platform_data *usbc_data = data;
	struct max77705_cc_data *cc_data = usbc_data->cc_data;
	u8 connstat = 0;

	pr_debug("%s: IRQ(%d)_IN\n", __func__, irq);
	max77705_read_reg(usbc_data->muic, REG_CC_STATUS1, &cc_data->cc_status1);
	connstat = (cc_data->cc_status1 & BIT_ConnStat)
				>> FFS(BIT_ConnStat);

	switch (connstat) {
	case DRY:
		msg_maxim("== WATER RUN-DRY DETECT ==");
		if (usbc_data->current_connstat != DRY) {
			usbc_data->prev_connstat = usbc_data->current_connstat;
			usbc_data->current_connstat = DRY;
			if(!usbc_data->max77705->blocking_waterevent)
				max77705_ccic_event_work(usbc_data,
					CCIC_NOTIFY_DEV_BATTERY,
					CCIC_NOTIFY_ID_WATER,
					0/*attach*/,
					0,
					0);
		}
		break;

	case WATER:
		msg_maxim("== WATER DETECT ==");

		if (usbc_data->current_connstat != WATER) {
			usbc_data->prev_connstat = usbc_data->current_connstat;
			usbc_data->current_connstat = WATER;
			if(!usbc_data->max77705->blocking_waterevent)
				max77705_ccic_event_work(usbc_data,
					CCIC_NOTIFY_DEV_BATTERY,
					CCIC_NOTIFY_ID_WATER,
					1/*attach*/,
					0,
					0);
		}
		break;
	default:
		break;

	}

	pr_debug("%s: IRQ(%d)_OUT\n", __func__, irq);

	return IRQ_HANDLED;
}

static irqreturn_t max77705_ccpinstat_irq(int irq, void *data)
{
	struct max77705_usbc_platform_data *usbc_data = data;
	struct max77705_cc_data *cc_data = usbc_data->cc_data;
	u8 ccpinstat = 0;

	max77705_read_reg(usbc_data->muic, REG_CC_STATUS0, &cc_data->cc_status0);

	pr_debug("%s: IRQ(%d)_IN\n", __func__, irq);
	ccpinstat = (cc_data->cc_status0 & BIT_CCPinStat)
		>> FFS(BIT_CCPinStat);

	switch (ccpinstat) {
	case NO_DETERMINATION:
			msg_maxim("CCPINSTAT (NO_DETERMINATION)");
#if defined(CONFIG_CCIC_NOTIFIER)
		if (usbc_data->ccrp_state) {
			usbc_data->ccrp_state = 0;
			max77705_ccic_event_work(usbc_data,
				CCIC_NOTIFY_DEV_BATTERY, CCIC_NOTIFY_ID_WATER_CABLE,
				CCIC_NOTIFY_DETACH, 0/*rprd*/, 0);
		}
#endif
			break;
	case CC1_ACTIVE:
			msg_maxim("CCPINSTAT (CC1_ACTIVE)");
			break;

	case CC2_ACTVIE:
			msg_maxim("CCPINSTAT (CC2_ACTIVE)");
			break;

	case AUDIO_ACCESSORY:
			msg_maxim("CCPINSTAT (AUDIO_ACCESSORY)");
			break;

	default:
			msg_maxim("CCPINSTAT [%d]", ccpinstat);
			break;

	}
	cc_data->ccpinstat = ccpinstat;
	usbc_data->cc_pin_status  = ccpinstat;
#if defined(CONFIG_SEC_FACTORY)
	max77705_ccic_event_work(usbc_data, CCIC_NOTIFY_DEV_CCIC,
		CCIC_NOTIFY_ID_CC_PIN_STATUS, ccpinstat, 0, 0);
#endif
	pr_debug("%s: IRQ(%d)_OUT\n", __func__, irq);

	return IRQ_HANDLED;
}

static irqreturn_t max77705_ccistat_irq(int irq, void *data)
{
	struct max77705_usbc_platform_data *usbc_data = data;
	struct max77705_cc_data *cc_data = usbc_data->cc_data;
	u8 ccistat = 0;
#if defined(CONFIG_TYPEC)
	enum typec_pwr_opmode mode = TYPEC_PWR_MODE_USB;
#endif
	usbc_cmd_data value;

	max77705_read_reg(usbc_data->muic, REG_CC_STATUS0, &cc_data->cc_status0);
	pr_debug("%s: IRQ(%d)_IN\n", __func__, irq);
	ccistat = (cc_data->cc_status0 & BIT_CCIStat) >> FFS(BIT_CCIStat);
	switch (ccistat) {
	case NOT_IN_UFP_MODE:
		msg_maxim("Not in UFP");
		break;

	case CCI_500mA:
		msg_maxim("Vbus Current is 500mA!");
		break;

	case CCI_1_5A:
		msg_maxim("Vbus Current is 1.5A!");
#if defined(CONFIG_TYPEC)
		mode = TYPEC_PWR_MODE_1_5A;
#endif
		break;

	case CCI_3_0A:
		msg_maxim("Vbus Current is 3.0A!");
#if defined(CONFIG_TYPEC)
		mode = TYPEC_PWR_MODE_3_0A;
#endif
		if (usbc_data->srcccap_request_retry) {
			usbc_data->pn_flag = false;			
			usbc_data->srcccap_request_retry = false;
			value.opcode = OPCODE_SRCCAP_REQUEST;
			value.write_data[0] = pd_noti.sink_status.selected_pdo_num;
			value.write_length = 1;
			value.read_length = 1;
			max77705_usbc_opcode_write(usbc_data, &value);
			pr_info("%s : OPCODE(0x%02x) W_LENGTH(%d) R_LENGTH(%d) NUM(%d)\n",
				__func__, value.opcode, value.write_length, value.read_length,
				pd_noti.sink_status.selected_pdo_num);
		}
		break;

	default:
		msg_maxim("CCINSTAT(Never Call this routine) !");
		break;

	}
	cc_data->ccistat = ccistat;
	pr_debug("%s: IRQ(%d)_OUT\n", __func__, irq);

	max77705_notify_rp_current_level(usbc_data);

#if defined(CONFIG_TYPEC)
	if (!usbc_data->pd_support) {
		usbc_data->pwr_opmode = mode;
		typec_set_pwr_opmode(usbc_data->port, mode);
	}
#endif

	return IRQ_HANDLED;
}


static irqreturn_t max77705_ccvnstat_irq(int irq, void *data)
{
	struct max77705_usbc_platform_data *usbc_data = data;
	struct max77705_cc_data *cc_data = usbc_data->cc_data;
	u8 ccvcnstat = 0;

	max77705_read_reg(usbc_data->muic, REG_CC_STATUS0, &cc_data->cc_status0);

	pr_debug("%s: IRQ(%d)_IN\n", __func__, irq);
	ccvcnstat = (cc_data->cc_status0 & BIT_CCVcnStat) >> FFS(BIT_CCVcnStat);

	switch (ccvcnstat) {
	case 0:
		msg_maxim("Vconn Disabled");
		if (cc_data->current_vcon != OFF) {
			cc_data->previous_vcon = cc_data->current_vcon;
			cc_data->current_vcon = OFF;
		}
		break;

	case 1:
		msg_maxim("Vconn Enabled");
		if (cc_data->current_vcon != ON) {
			cc_data->previous_vcon = cc_data->current_vcon;
			cc_data->current_vcon = ON;
		}
		break;

	default:
		msg_maxim("ccvnstat(Never Call this routine) !");
		break;

	}
	cc_data->ccvcnstat = ccvcnstat;
	pr_debug("%s: IRQ(%d)_OUT\n", __func__, irq);


	return IRQ_HANDLED;
}

static void max77705_ccstat_irq_handler(void *data, int irq)
{
	struct power_supply *psy_charger;
	union power_supply_propval val;
	struct max77705_usbc_platform_data *usbc_data = data;
	struct max77705_cc_data *cc_data = usbc_data->cc_data;
	u8 ccstat = 0;
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
	int prev_power_role = usbc_data->power_role;
#elif defined(CONFIG_TYPEC)
	int prev_power_role = usbc_data->typec_power_role;
#endif
#if defined(CONFIG_USB_HOST_NOTIFY)
	struct otg_notify *o_notify = get_otg_notify();
#endif
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	int event;
#endif

	max77705_read_reg(usbc_data->muic, REG_CC_STATUS0, &cc_data->cc_status0);
	ccstat =  (cc_data->cc_status0 & BIT_CCStat) >> FFS(BIT_CCStat);
	if (irq == CCIC_IRQ_INIT_DETECT) {
		if (ccstat == cc_SINK)
			msg_maxim("initial time : SNK");
		else
			return;
	}
#if defined(CONFIG_CCIC_NOTIFIER)
	if (ccstat == cc_No_Connection)
		usbc_data->pd_state = max77705_State_PE_Initial_detach;
	else if (ccstat == cc_SOURCE)
		usbc_data->pd_state = max77705_State_PE_SRC_Send_Capabilities;
	else if (ccstat == cc_SINK)
		usbc_data->pd_state = max77705_State_PE_SNK_Wait_for_Capabilities;
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	store_usblog_notify(NOTIFY_FUNCSTATE, (void *)&usbc_data->pd_state, NULL);
#endif
#endif
	if (!ccstat) {
		if (usbc_data->plug_attach_done) {
			msg_maxim("PLUG_DETACHED ---");
#if defined(CONFIG_TYPEC)
			if (usbc_data->partner) {
				msg_maxim("ccstat : typec_unregister_partner");
				if (!IS_ERR(usbc_data->partner))
					typec_unregister_partner(usbc_data->partner);
				usbc_data->partner = NULL;
				usbc_data->typec_power_role = TYPEC_SINK;
				usbc_data->typec_data_role = TYPEC_DEVICE;
				usbc_data->pwr_opmode = TYPEC_PWR_MODE_USB;
			}
			if (usbc_data->typec_try_state_change == TRY_ROLE_SWAP_PR ||
				usbc_data->typec_try_state_change == TRY_ROLE_SWAP_DR) {
				/* Role change try and new mode detected */
				msg_maxim("typec_reverse_completion, detached while pd_swap");
				usbc_data->typec_try_state_change = TRY_ROLE_SWAP_NONE;
				complete(&usbc_data->typec_reverse_completion);
			}
#endif
			max77705_notify_dr_status(usbc_data, 0);
			usbc_data->plug_attach_done = 0;
			usbc_data->cc_data->current_pr = 0xFF;
			usbc_data->pd_data->current_dr = 0xFF;
			usbc_data->cc_data->current_vcon = 0xFF;
			usbc_data->detach_done_wait = 1;
#if defined(CONFIG_COMBO_REDRIVER_PTN36502)
			ptn36502_config(SAFE_STATE, 0);
#endif
		}
	} else {
		if (!usbc_data->plug_attach_done) {
			msg_maxim("PLUG_ATTACHED +++");
			usbc_data->plug_attach_done = 1;
		}
	}

	switch (ccstat) {
	case cc_No_Connection:
			msg_maxim("ccstat : cc_No_Connection");
			usbc_data->pd_data->cc_status = CC_NO_CONN;
			usbc_data->is_samsung_accessory_enter_mode = 0;
			usbc_data->pn_flag = false;
			usbc_data->pd_support = false;
			usbc_data->srcccap_request_retry = false;
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
			if (!usbc_data->try_state_change)
#elif defined(CONFIG_TYPEC)
			if (!usbc_data->typec_try_state_change)
#endif
				max77705_usbc_clear_queue(usbc_data);
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
			usbc_data->power_role = DUAL_ROLE_PROP_PR_NONE;
#elif defined(CONFIG_TYPEC)
			usbc_data->typec_power_role = TYPEC_SINK;
#endif
#if defined(CONFIG_USB_HOST_NOTIFY)
			send_otg_notify(o_notify, NOTIFY_EVENT_POWER_SOURCE, 0);
#endif
			max77705_detach_pd(usbc_data);
			usbc_data->pd_pr_swap = cc_No_Connection;
			max77705_vbus_turn_on_ctrl(usbc_data, OFF, false);
#if defined(CONFIG_SEC_FACTORY)
			factory_execute_monitor(FAC_ABNORMAL_REPEAT_STATE);
#endif
			break;
	case cc_SINK:
			msg_maxim("ccstat : cc_SINK");
			usbc_data->pd_data->cc_status = CC_SNK;
			usbc_data->pn_flag = false;
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
			usbc_data->power_role = DUAL_ROLE_PROP_PR_SNK;
			if (usbc_data->dual_role != NULL &&
				usbc_data->data_role != USB_STATUS_NOTIFY_DETACH)
				dual_role_instance_changed(usbc_data->dual_role);
#elif defined(CONFIG_TYPEC)
			usbc_data->typec_power_role = TYPEC_SINK;
			typec_set_pwr_role(usbc_data->port, TYPEC_SINK);
#endif
#if defined(CONFIG_USB_HOST_NOTIFY)
			send_otg_notify(o_notify, NOTIFY_EVENT_POWER_SOURCE, 0);
#endif
			if (cc_data->current_pr != SNK) {
				cc_data->previous_pr = cc_data->current_pr;
				cc_data->current_pr = SNK;
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
				if (prev_power_role == DUAL_ROLE_PROP_PR_SRC)
#elif defined(CONFIG_TYPEC)
				if (prev_power_role == TYPEC_SOURCE)
#endif
					max77705_vbus_turn_on_ctrl(usbc_data, OFF, true);
			}
			psy_charger = power_supply_get_by_name("max77705-charger");
			if (psy_charger) {
				val.intval = 1;
				psy_do_property("max77705-charger", set, POWER_SUPPLY_EXT_PROP_CHGINSEL, val);
			} else {
				pr_err("%s: Fail to get psy charger\n", __func__);
			}
			max77705_notify_rp_current_level(usbc_data);
#if defined(CONFIG_SEC_FACTORY)
			factory_execute_monitor(FAC_ABNORMAL_REPEAT_STATE);
#endif
			break;
	case cc_SOURCE:
			msg_maxim("ccstat : cc_SOURCE");
			usbc_data->pd_data->cc_status = CC_SRC;
			usbc_data->pn_flag = false;
			usbc_data->srcccap_request_retry = false;
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
			usbc_data->power_role = DUAL_ROLE_PROP_PR_SRC;
			if (usbc_data->dual_role != NULL &&
				usbc_data->data_role != USB_STATUS_NOTIFY_DETACH)
				dual_role_instance_changed(usbc_data->dual_role);
#elif defined(CONFIG_TYPEC)
			usbc_data->typec_power_role = TYPEC_SOURCE;
			typec_set_pwr_role(usbc_data->port, TYPEC_SOURCE);
#endif
#if defined(CONFIG_USB_HOST_NOTIFY)
			send_otg_notify(o_notify, NOTIFY_EVENT_POWER_SOURCE, 1);
#endif
			if (cc_data->current_pr != SRC) {
				cc_data->previous_pr = cc_data->current_pr;
				cc_data->current_pr = SRC;
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
				if (prev_power_role == DUAL_ROLE_PROP_PR_NONE)
#elif defined(CONFIG_TYPEC)
				if (prev_power_role == TYPEC_SINK)
#endif
					max77705_vbus_turn_on_ctrl(usbc_data, ON, false);
#if defined(CONFIG_DUAL_ROLE_USB_INTF)
				else if (prev_power_role == DUAL_ROLE_PROP_PR_SNK)
					max77705_vbus_turn_on_ctrl(usbc_data, ON, true);
#endif
			}
			break;
	case cc_Audio_Accessory:
			msg_maxim("ccstat : cc_Audio_Accessory");
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
			event = NOTIFY_EXTRA_USB_ANALOGAUDIO;
			store_usblog_notify(NOTIFY_EXTRA, (void *)&event, NULL);
#endif
			usbc_data->acc_type = CCIC_DOCK_TYPEC_ANALOG_EARPHONE;
			max77705_process_check_accessory(usbc_data);
			break;
	case cc_Debug_Accessory:
			msg_maxim("ccstat : cc_Debug_Accessory");
			break;
	case cc_Error:
			msg_maxim("ccstat : cc_Error");
			break;
	case cc_Disabled:
			msg_maxim("ccstat : cc_Disabled");
			break;
	case cc_RFU:
			msg_maxim("ccstat : cc_RFU");
			break;
	default:
			break;
	}
}

static irqreturn_t max77705_ccstat_irq(int irq, void *data)
{
	pr_debug("%s: IRQ(%d)_IN\n", __func__, irq);
	max77705_ccstat_irq_handler(data, irq);
	pr_debug("%s: IRQ(%d)_OUT\n", __func__, irq);
	return IRQ_HANDLED;
}

int max77705_cc_init(struct max77705_usbc_platform_data *usbc_data)
{
	struct max77705_cc_data *cc_data = NULL;
	int ret;

	msg_maxim("IN");

	cc_data = usbc_data->cc_data;

	wake_lock_init(&cc_data->vconncop_wake_lock, WAKE_LOCK_SUSPEND,
			   "cc->vconncop");
	wake_lock_init(&cc_data->vsafe0v_wake_lock, WAKE_LOCK_SUSPEND,
			   "cc->vsafe0v");
	wake_lock_init(&cc_data->detabrt_wake_lock, WAKE_LOCK_SUSPEND,
			   "cc->detabrt");
	wake_lock_init(&cc_data->vconnsc_wake_lock, WAKE_LOCK_SUSPEND,
			   "cc->vconnsc");

	wake_lock_init(&cc_data->ccpinstat_wake_lock, WAKE_LOCK_SUSPEND,
			  "cc->ccpinstat");
	wake_lock_init(&cc_data->ccistat_wake_lock, WAKE_LOCK_SUSPEND,
			  "cc->ccistat");
	wake_lock_init(&cc_data->ccvcnstat_wake_lock, WAKE_LOCK_SUSPEND,
			  "cc->ccvcnstat");
	wake_lock_init(&cc_data->ccstat_wake_lock, WAKE_LOCK_SUSPEND,
			  "cc->ccstat");

	cc_data->irq_vconncop = usbc_data->irq_base + MAX77705_CC_IRQ_VCONNCOP_INT;
	if (cc_data->irq_vconncop) {
		ret = request_threaded_irq(cc_data->irq_vconncop,
			   NULL, max77705_vconncop_irq,
			   0,
			   "cc-vconncop-irq", usbc_data);
		if (ret) {
			pr_err("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			goto err_irq;
		}
	}

	cc_data->irq_vsafe0v = usbc_data->irq_base + MAX77705_CC_IRQ_VSAFE0V_INT;
	if (cc_data->irq_vsafe0v) {
		ret = request_threaded_irq(cc_data->irq_vsafe0v,
			   NULL, max77705_vsafe0v_irq,
			   0,
			   "cc-vsafe0v-irq", usbc_data);
		if (ret) {
			pr_err("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			goto err_irq;
		}
	}

	cc_data->irq_vconnsc = usbc_data->irq_base + MAX77705_CC_IRQ_VCONNSC_INT;
	if (cc_data->irq_vconnsc) {
		ret = request_threaded_irq(cc_data->irq_vconnsc,
			   NULL, max77705_vconnsc_irq,
			   0,
			   "cc-vconnsc-irq", usbc_data);
		if (ret) {
			pr_err("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			goto err_irq;
		}
	}
	cc_data->irq_ccpinstat = usbc_data->irq_base + MAX77705_CC_IRQ_CCPINSTAT_INT;
	if (cc_data->irq_ccpinstat) {
		ret = request_threaded_irq(cc_data->irq_ccpinstat,
			   NULL, max77705_ccpinstat_irq,
			   0,
			   "cc-ccpinstat-irq", usbc_data);
		if (ret) {
			pr_err("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			goto err_irq;
		}
	}
	cc_data->irq_ccistat = usbc_data->irq_base + MAX77705_CC_IRQ_CCISTAT_INT;
	if (cc_data->irq_ccistat) {
		ret = request_threaded_irq(cc_data->irq_ccistat,
			   NULL, max77705_ccistat_irq,
			   0,
			   "cc-ccistat-irq", usbc_data);
		if (ret) {
			pr_err("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			goto err_irq;
		}
	}
	cc_data->irq_ccvcnstat = usbc_data->irq_base + MAX77705_CC_IRQ_CCVCNSTAT_INT;
	if (cc_data->irq_ccvcnstat) {
		ret = request_threaded_irq(cc_data->irq_ccvcnstat,
			   NULL, max77705_ccvnstat_irq,
			   0,
			   "cc-ccvcnstat-irq", usbc_data);
		if (ret) {
			pr_err("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			goto err_irq;
		}
	}
	cc_data->irq_ccstat = usbc_data->irq_base + MAX77705_CC_IRQ_CCSTAT_INT;
	if (cc_data->irq_ccstat) {
		ret = request_threaded_irq(cc_data->irq_ccstat,
			   NULL, max77705_ccstat_irq,
			   0,
			   "cc-ccstat-irq", usbc_data);
		if (ret) {
			pr_err("%s: Failed to Request IRQ (%d)\n", __func__, ret);
			goto err_irq;
		}
	}
	/* check CC Pin state for cable attach booting scenario */
	max77705_ccstat_irq_handler(usbc_data, CCIC_IRQ_INIT_DETECT);
	max77705_read_reg(usbc_data->muic, REG_CC_STATUS1, &cc_data->cc_status1);
	usbc_data->current_connstat = (cc_data->cc_status1 & BIT_ConnStat)
				>> FFS(BIT_ConnStat);
	pr_info("%s: water state : %s\n", __func__, usbc_data->current_connstat ? "WATER" : "DRY");

	if (usbc_data->current_connstat) {
		if(!usbc_data->max77705->blocking_waterevent)
			max77705_ccic_event_work(usbc_data,
				CCIC_NOTIFY_DEV_BATTERY,
				CCIC_NOTIFY_ID_WATER,
				1/*attach*/,
				0,
				0);
	}
	msg_maxim("OUT");

	return 0;

err_irq:
	kfree(cc_data);
	return ret;

}
