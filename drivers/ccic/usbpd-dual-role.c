/*
 driver/usbpd/usbpd_cc.c - USB PD(Power Delivery) device driver
 *
 * Copyright (C) 2017 Samsung Electronics
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
 */

#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/usb_notify.h>

#include <linux/ccic/core.h>

#if 0
#if defined(CONFIG_CCIC_NOTIFIER)
static void ccic_event_notifier(struct work_struct *data)
{
	struct ccic_state_work *event_work =
		container_of(data, struct ccic_state_work, ccic_work);
	CC_NOTI_TYPEDEF ccic_noti;

	switch (event_work->dest) {
	case CCIC_NOTIFY_DEV_USB:
		pr_info("usb:%s, dest=%s, id=%s, attach=%s, drp=%s, event_work=%p\n", __func__,
				CCIC_NOTI_DEST_Print[event_work->dest],
				CCIC_NOTI_ID_Print[event_work->id],
				event_work->attach ? "Attached" : "Detached",
				CCIC_NOTI_USB_STATUS_Print[event_work->event],
				event_work);
		break;
	default:
		pr_info("usb:%s, dest=%s, id=%s, attach=%d, event=%d, event_work=%p\n", __func__,
			CCIC_NOTI_DEST_Print[event_work->dest],
			CCIC_NOTI_ID_Print[event_work->id],
			event_work->attach,
			event_work->event,
			event_work);
		break;
	}

	ccic_noti.src = CCIC_NOTIFY_DEV_CCIC;
	ccic_noti.dest = event_work->dest;
	ccic_noti.id = event_work->id;
	ccic_noti.sub1 = event_work->attach;
	ccic_noti.sub2 = event_work->event;
	ccic_noti.sub3 = 0;
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	ccic_noti.pd = &pd_noti;
#endif
	ccic_notifier_notify((CC_NOTI_TYPEDEF *)&ccic_noti, NULL, 0);

	kfree(event_work);
}

extern void ccic_event_work(void *data, int dest, int id, int attach, int event)
{
	struct s2mu004_usbpd_data *usbpd_data = data;
	struct ccic_state_work *event_work;


	event_work = kmalloc(sizeof(struct ccic_state_work), GFP_ATOMIC);
	pr_info("usb: %s,event_work(%p)\n", __func__, event_work);
	INIT_WORK(&event_work->ccic_work, ccic_event_notifier);

	event_work->dest = dest;
	event_work->id = id;
	event_work->attach = attach;
	event_work->event = event;

#if defined(CONFIG_DUAL_ROLE_USB_INTF)
	if (id == CCIC_NOTIFY_ID_USB) {
		pr_info("usb: %s, dest=%d, event=%d, usbpd_data->data_role_dual=%d, usbpd_data->try_state_change=%d\n",
			__func__, dest, event, usbpd_data->data_role_dual, usbpd_data->try_state_change);

		usbpd_data->data_role_dual = event;

		if (usbpd_data->dual_role != NULL)
			dual_role_instance_changed(usbpd_data->dual_role);

		if (usbpd_data->try_state_change &&
			(usbpd_data->data_role_dual != USB_STATUS_NOTIFY_DETACH)) {
			/* Role change try and new mode detected */
			pr_info("usb: %s, reverse_completion\n", __func__);
			complete(&usbpd_data->reverse_completion);
		}
	}
#endif

	if (queue_work(usbpd_data->ccic_wq, &event_work->ccic_work) == 0) {
		pr_info("usb: %s, event_work(%p) is dropped\n", __func__, event_work);
		kfree(event_work);
	}
}
#endif
#endif

#if defined(CONFIG_DUAL_ROLE_USB_INTF)
static enum dual_role_property fusb_drp_properties[] = {
	DUAL_ROLE_PROP_MODE,
	DUAL_ROLE_PROP_PR,
	DUAL_ROLE_PROP_DR,
	DUAL_ROLE_PROP_VCONN_SUPPLY,
};

void role_swap_check(struct work_struct *wk)
{
	struct delayed_work *delay_work =
		container_of(wk, struct delayed_work, work);
	struct usbpd_dev *udev =
		container_of(delay_work, struct usbpd_dev, role_swap_work);
	struct usbpd_info *pd_info = &udev->desc->pd_info;
	const struct usbpd_ops *ops = udev->desc->ops;

	dev_info(&udev->dev, "%s: ccic_set_dual_role check again.\n", __func__);
	pd_info->try_state_change = 0;

	if (pd_info->attach_mode == USBPD_DETACHED) {
		dev_err(&udev->dev,
		     "%s: ccic_set_dual_role reverse failed, set mode to DRP\n", __func__);
		if (ops->usbpd_disable_irq)
			ops->usbpd_disable_irq(udev);
		if (ops->usbpd_set_rprd_mode)
			ops->usbpd_set_rprd_mode(udev, TYPE_C_ATTACH_DRP);
		if (ops->usbpd_enable_irq)
			ops->usbpd_enable_irq(udev);
	}
}

static int ccic_set_dual_role(struct dual_role_phy_instance *dual_role,
				   enum dual_role_property prop,
				   const unsigned int *val)
{
	struct usbpd_dev *udev = dual_role_get_drvdata(dual_role);
	struct device *dev = &dual_role->dev;
	struct usbpd_info *pd_info = &udev->desc->pd_info;
	const struct usbpd_ops *ops = udev->desc->ops;
	USB_STATUS attached_state;
	int timeout = 0;
	int ret = 0;

	if (!udev) {
		dev_err(dev, "%s : usbpd_dev is null \n", __func__);
		return -ENOMEM;
	}

	/* Get Current Role */
	attached_state = pd_info->data_role;
	dev_info(dev, "%s : request prop = %d , attached_state = %d\n",
							__func__, prop, attached_state);

	if (attached_state != USB_STATUS_NOTIFY_ATTACH_DFP
	    && attached_state != USB_STATUS_NOTIFY_ATTACH_UFP) {
		dev_err(dev, "%s : current mode : %d - just return \n",
							__func__, attached_state);
		return 0;
	}

	if (attached_state == USB_STATUS_NOTIFY_ATTACH_DFP
	    && *val == DUAL_ROLE_PROP_MODE_DFP) {
		dev_err(dev, "%s : current mode : %d - request mode : %d just return \n",
							__func__, attached_state, *val);
		return 0;
	}

	if (attached_state == USB_STATUS_NOTIFY_ATTACH_UFP
	    && *val == DUAL_ROLE_PROP_MODE_UFP) {
		dev_err(dev, "%s : current mode : %d - request mode : %d just return \n",
							__func__, attached_state, *val);
		return 0;
	}

	if (attached_state == USB_STATUS_NOTIFY_ATTACH_DFP) {
		/* Current mode DFP and Source  */
		dev_info(dev, "%s: try reversing, from Source to Sink\n", __func__);
		/* turns off VBUS first */
		vbus_turn_on_ctrl(udev, VBUS_OFF);
#if defined(CONFIG_CCIC_NOTIFIER)
		/* muic */
		ccic_event_work(usbpd_data,
			CCIC_NOTIFY_DEV_MUIC, CCIC_NOTIFY_ID_ATTACH, 0/*attach*/, 0/*rprd*/);
#endif
		/* exit from Disabled state and set mode to UFP */
		pd_info->try_state_change = TYPE_C_ATTACH_UFP;
		if (ops->usbpd_set_rprd_mode)
			ops->usbpd_set_rprd_mode(udev, TYPE_C_ATTACH_UFP);
	} else {
		/* Current mode UFP and Sink  */
		dev_info(dev, "%s: try reversing, from Sink to Source\n", __func__);
		/* exit from Disabled state and set mode to UFP */
		pd_info->try_state_change = TYPE_C_ATTACH_DFP;
		if (ops->usbpd_set_rprd_mode)
			ops->usbpd_set_rprd_mode(udev, TYPE_C_ATTACH_DFP);
	}

	reinit_completion(&udev->reverse_completion);
	timeout =
	    wait_for_completion_timeout(&udev->reverse_completion,
					msecs_to_jiffies
					(DUAL_ROLE_SET_MODE_WAIT_MS));

	if (!timeout) {
		pd_info->try_state_change = 0;
		dev_err(dev, "%s: reverse failed, set mode to DRP\n", __func__);
		if (ops->usbpd_disable_irq)
			ops->usbpd_disable_irq(udev);
		/* exit from Disabled state and set mode to DRP */
		if (ops->usbpd_set_rprd_mode)
			ops->usbpd_set_rprd_mode(udev, TYPE_C_ATTACH_DRP);
		if (ops->usbpd_enable_irq)
			ops->usbpd_enable_irq(udev);
		ret = -EIO;
	} else {
		dev_err(dev, "%s: reverse success, one more check\n", __func__);
		schedule_delayed_work(&udev->role_swap_work, msecs_to_jiffies(DUAL_ROLE_SET_MODE_WAIT_MS));
	}

	dev_info(dev, "%s -> data role : %d\n", __func__, *val);
	return ret;
}

/* Decides whether userspace can change a specific property */
int dual_role_is_writeable(struct dual_role_phy_instance *drp,
				  enum dual_role_property prop)
{
	if (prop == DUAL_ROLE_PROP_MODE)
		return 1;
	else
		return 0;
}

/* Callback for "cat /sys/class/dual_role_usb/otg_default/<property>" */
int dual_role_get_local_prop(struct dual_role_phy_instance *dual_role,
				    enum dual_role_property prop,
				    unsigned int *val)
{
	struct usbpd_dev *udev = dual_role_get_drvdata(dual_role);
	struct usbpd_info *pd_info = &udev->desc->pd_info;
	int power_role = 0;
	USB_STATUS attached_state = 0;

	if (!udev) {
		pr_err("%s : usbpd_dev is null \n", __func__);
		return -ENOMEM;
	}

	pd_info = &udev->desc->pd_info;

	if (!pd_info) {
		pr_err("%s, pd_info is null\n", __func__);
		return -ENOMEM;
	}

	attached_state = pd_info->data_role;
	power_role = pd_info->power_role;

	pr_info("%s : request prop = %d , attached_state = %d, power_role = %d\n",
		__func__, prop, attached_state, power_role);

	if (prop == DUAL_ROLE_PROP_VCONN_SUPPLY) {
		if (pd_info->vconn_en)
			*val = DUAL_ROLE_PROP_VCONN_SUPPLY_YES;
		else
			*val = DUAL_ROLE_PROP_VCONN_SUPPLY_NO;
		return 0;
	}

	if (attached_state == USB_STATUS_NOTIFY_ATTACH_DFP) {
		if (prop == DUAL_ROLE_PROP_MODE)
			*val = DUAL_ROLE_PROP_MODE_DFP;
		else if (prop == DUAL_ROLE_PROP_PR)
			*val = power_role;
		else if (prop == DUAL_ROLE_PROP_DR)
			*val = DUAL_ROLE_PROP_DR_HOST;
		else
			return -EINVAL;
	} else if (attached_state == USB_STATUS_NOTIFY_ATTACH_UFP) {
		if (prop == DUAL_ROLE_PROP_MODE)
			*val = DUAL_ROLE_PROP_MODE_UFP;
		else if (prop == DUAL_ROLE_PROP_PR)
			*val = power_role;
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

/* Callback for "echo <value> >
 *                      /sys/class/dual_role_usb/<name>/<property>"
 * Block until the entire final state is reached.
 * Blocking is one of the better ways to signal when the operation
 * is done.
 * This function tries to switch to Attached.SRC or Attached.SNK
 * by forcing the mode into SRC or SNK.
 * On failure, we fall back to Try.SNK state machine.
 */
int dual_role_set_prop(struct dual_role_phy_instance *dual_role,
			      enum dual_role_property prop,
			      const unsigned int *val)
{
	dev_info(&dual_role->dev,
		"%s : request prop = %d , *val = %d \n", __func__, prop, *val);

	if (prop == DUAL_ROLE_PROP_MODE)
		return ccic_set_dual_role(dual_role, prop, val);
	else
		return -EINVAL;
}

int dual_role_init(struct usbpd_dev *udev)
{
	struct dual_role_phy_desc *desc;
	struct dual_role_phy_instance *dual_role;

	desc = devm_kzalloc(&udev->dev,
			 sizeof(struct dual_role_phy_desc), GFP_KERNEL);
	if (!desc) {
		pr_err("unable to allocate dual role descriptor\n");
		return -ENOMEM;
	}

	desc->name = "otg_default";
	desc->supported_modes = DUAL_ROLE_SUPPORTED_MODES_DFP_AND_UFP;
	desc->get_property = dual_role_get_local_prop;
	desc->set_property = dual_role_set_prop;
	desc->properties = fusb_drp_properties;
	desc->num_properties = ARRAY_SIZE(fusb_drp_properties);
	desc->property_is_writeable = dual_role_is_writeable;
	dual_role =
		devm_dual_role_instance_register(&udev->dev, desc);
	dual_role->drv_data = udev;
	udev->dual_role = dual_role;
	init_completion(&udev->reverse_completion);
	INIT_DELAYED_WORK(&udev->role_swap_work, role_swap_check);

	return 0;
}
#endif
