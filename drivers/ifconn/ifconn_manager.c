#include <linux/device.h>
#include <linux/module.h>
#include <linux/ifconn/ifconn_notifier.h>
#include <linux/ifconn/ifconn_manager.h>
#include <linux/muic/muic.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#if defined(CONFIG_CCIC_NOTIFIER)
#include <linux/ccic/core.h>
#endif

static int _ifconn_manager_template_notify(struct ifconn_notifier_template *template)
{
	int ret;

	if (template == NULL) {
		pr_info("%s: err, template is NULL\n", __func__);
		return -1;
	}

	ret = ifconn_notifier_notify(template->src,
				     template->dest,
				     template->id,
				     template->event,
				     IFCONN_NOTIFY_PARAM_TEMPLATE, template);

	if (ret < 0)
		pr_err("%s: Fail to send noti\n", __func__);

	return ret;
}

static void ifconn_manager_noti_work(struct ifconn_manager_data *ifconn_manager,
				     struct ifconn_notifier_template *template)
{

	pr_info("%s: enter, template : %p\n", __func__, template);
	mutex_lock(&ifconn_manager->noti_mutex);

	template->up_src = template->src;
	template->src = IFCONN_NOTIFY_MANAGER;

	switch (template->up_src) {
	case IFCONN_NOTIFY_MUIC:
		if (ifconn_manager->ccic_drp_state == IFCONN_MANAGER_USB_STATUS_DETACH) {
			if (template->attach && ifconn_manager->vbus_state == IFCONN_MANAGER_VBUS_STATUS_HIGH) {
				msleep(2000);
				template->drp = IFCONN_MANAGER_USB_STATUS_ATTACH_UFP;
				ifconn_manager->ccic_drp_state = IFCONN_MANAGER_USB_STATUS_ATTACH_UFP;
			} else {
				template->drp = IFCONN_MANAGER_USB_STATUS_DETACH;
				ifconn_manager->ccic_drp_state = IFCONN_MANAGER_USB_STATUS_DETACH;
			}
			template->sub3 = 0;
			template->data = NULL;
		}
		_ifconn_manager_template_notify(template);
		break;
	case IFCONN_NOTIFY_CCIC:
		pr_info("%s: dbg, line : %d\n", __func__, __LINE__);
		if ((ifconn_manager->ccic_drp_state !=
		     IFCONN_MANAGER_USB_STATUS_ATTACH_UFP)
		    || ifconn_manager->is_UFPS) {
			pr_info("usb: [M] %s: skip case\n", __func__);
			goto EXIT_NOTI_WORK;
		}
		template->src = IFCONN_NOTIFY_MANAGER;
		template->data = NULL;
		template->id = IFCONN_NOTIFY_ID_USB;
		pr_info("usb: [M] %s: usb=%d, pd=%d\n", __func__,
			ifconn_manager->usb_enum_state,
			ifconn_manager->pd_con_state);
		if (!ifconn_manager->usb_enum_state
		    || (ifconn_manager->muic_data_refresh
			&& ifconn_manager->cable_type ==
			IFCONN_CABLE_TYPE_MUIC_CHARGER)) {

			/* TA cable Type */
			template->dest = IFCONN_NOTIFY_USB;
			template->attach = IFCONN_NOTIFY_DETACH;
			template->drp = IFCONN_MANAGER_USB_STATUS_DETACH;
			template->sub3 = 0;
		} else {
			/* USB cable Type */
			template->dest = IFCONN_NOTIFY_BATTERY;
			template->attach = 0;
			template->rprd = 0;
			template->cable_type = IFCONN_PD_USB_TYPE;
		}
		_ifconn_manager_template_notify(template);
		break;
	case IFCONN_NOTIFY_PDIC:
		pr_info("%s: pdic case, up_src : %d\n", __func__,
			template->up_src);
		template->src = IFCONN_NOTIFY_MANAGER;
		template->dest = IFCONN_NOTIFY_BATTERY;
		template->id = IFCONN_NOTIFY_ID_POWER_STATUS;
		template->event = IFCONN_NOTIFY_EVENT_ATTACH;
		_ifconn_manager_template_notify(template);
		break;

	default:
		break;
	}
EXIT_NOTI_WORK:
	mutex_unlock(&ifconn_manager->noti_mutex);
}

static int ifconn_manager_manage_muic(struct ifconn_manager_data *ifconn_manager,
				      struct ifconn_notifier_template *template)
{
	if (ifconn_manager == NULL || template == NULL)
		return -1;

	ifconn_manager->muic_action = template->attach;
	ifconn_manager->muic_cable_type = template->cable_type;
	ifconn_manager->muic_data_refresh = 1;

	pr_info("%s, usb: [M] %s: cable type : (%d), id : %d\n", __func__,
		__func__, template->cable_type,
		template->id);

	if (template->id == IFCONN_NOTIFY_ID_ATTACH) {
		switch (template->cable_type) {
		case ATTACHED_DEV_USB_MUIC:
		case ATTACHED_DEV_CDP_MUIC:
		case ATTACHED_DEV_UNOFFICIAL_ID_USB_MUIC:
		case ATTACHED_DEV_UNOFFICIAL_ID_CDP_MUIC:
		case ATTACHED_DEV_JIG_USB_OFF_MUIC:
		case ATTACHED_DEV_JIG_USB_ON_MUIC:
			if (template->attach)
				ifconn_manager->cable_type = IFCONN_CABLE_TYPE_MUIC_USB;
			template->dest = IFCONN_NOTIFY_USB;
			template->id = IFCONN_NOTIFY_ID_USB;
			ifconn_manager_noti_work(ifconn_manager, template);
			break;
		case ATTACHED_DEV_TA_MUIC:
			pr_info("usb: [M] %s: TA(%d) %s\n", __func__,
				template->cable_type,
				template->attach ? "Attached" : "Detached");

			if (ifconn_manager->muic_action)
				ifconn_manager->cable_type = IFCONN_CABLE_TYPE_MUIC_CHARGER;

			if (template->attach
			    && ifconn_manager->ccic_drp_state == IFCONN_MANAGER_USB_STATUS_ATTACH_UFP) {
				/* Turn off the USB Phy when connected to the charger */
				template->src = IFCONN_NOTIFY_MUIC;
				template->dest = IFCONN_NOTIFY_USB;
				template->id = IFCONN_NOTIFY_ID_USB;
				template->attach = IFCONN_NOTIFY_DETACH;
				template->drp = IFCONN_NOTIFY_EVENT_DETACH;
				template->sub3 = 0;
				template->data = NULL;
				ifconn_manager_noti_work(ifconn_manager,
							 template);
			}
			break;

		case ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC:
		case ATTACHED_DEV_QC_CHARGER_PREPARE_MUIC:
			pr_info("usb: [M] %s: AFC or QC Prepare(%d) %s\n",
				__func__, template->cable_type,
				template->attach ? "Attached" : "Detached");
			break;

		case ATTACHED_DEV_TIMEOUT_OPEN_MUIC:
			pr_info
			    ("usb: [M] %s: DCD Timeout device is detected(%d) %s\n",
			     __func__, template->cable_type,
			     template->attach ? "Attached" : "Detached");

			if (ifconn_manager->muic_action) {
				ifconn_manager->cable_type =
				    IFCONN_CABLE_TYPE_MUIC_TIMEOUT_OPEN_DEVICE;
			}
			break;

		default:
			break;
		}
		template->id = IFCONN_NOTIFY_ID_ATTACH;
		template->src = IFCONN_NOTIFY_MUIC;
		template->dest = IFCONN_NOTIFY_BATTERY;
		ifconn_manager_noti_work(ifconn_manager, template);

	} else if (template->id == IFCONN_NOTIFY_ID_DETACH) {
		template->src = IFCONN_NOTIFY_MUIC;
		template->dest = IFCONN_NOTIFY_BATTERY;
		ifconn_manager_noti_work(ifconn_manager, template);
	}

	return 0;
}

static int ifconn_manager_manage_battery(struct ifconn_manager_data *ifconn_manager,
					 struct ifconn_notifier_template *template)
{
	int ret;

	if (ifconn_manager == NULL || template == NULL)
		return -1;

	switch (template->id) {
	case IFCONN_NOTIFY_ID_SELECT_PDO:
		ret = ifconn_notifier_notify(IFCONN_NOTIFY_MANAGER,
					     IFCONN_NOTIFY_CCIC,
					     template->id,
					     template->event,
					     IFCONN_NOTIFY_PARAM_TEMPLATE,
					     template);
		break;
	default:
		break;
	}
	return 0;
}

static int ifconn_manager_manage_pdic(struct ifconn_manager_data *ifconn_manager,
				      struct ifconn_notifier_template *template)
{
	if (ifconn_manager == NULL || template == NULL)
		return -1;

	pr_info("%s, usb: [M] %s: cable type : (%d), id : %d\n", __func__,
		__func__, template->cable_type,
		template->id);

	switch (template->id) {
	case IFCONN_NOTIFY_ID_POWER_STATUS:
		if (template->attach) {
			ifconn_manager->pd_con_state = 1;
			if ((ifconn_manager->ccic_drp_state == IFCONN_MANAGER_USB_STATUS_ATTACH_UFP)
			    && !ifconn_manager->is_UFPS) {
				pr_info("usb: [M] %s: PD charger + UFP\n",
					__func__);
			}
		}
		template->dest = IFCONN_NOTIFY_BATTERY;
		if (ifconn_manager->pd == NULL)
			ifconn_manager->pd = template->data;
		ifconn_manager_noti_work(ifconn_manager, template);
		break;
	case IFCONN_NOTIFY_ID_DP_CONNECT:
	case IFCONN_NOTIFY_ID_DP_HPD:
	case IFCONN_NOTIFY_ID_DP_LINK_CONF:
		template->src = IFCONN_NOTIFY_MANAGER;
		template->dest = IFCONN_NOTIFY_DP;
		_ifconn_manager_template_notify(template);
		break;
	default:
		break;
	}
	return 0;
}

static int ifconn_manager_manage_ccic(struct ifconn_manager_data *ifconn_manager,
				      struct ifconn_notifier_template *template)
{
	if (ifconn_manager == NULL || template == NULL)
		return -1;

	switch (template->id) {
	case IFCONN_NOTIFY_ID_ATTACH:
		if (ifconn_manager->ccic_attach_state != template->sub1) {
			/*attach */
			ifconn_manager->ccic_attach_state = template->sub1;
			ifconn_manager->muic_data_refresh = 0;
			ifconn_manager->is_UFPS = 0;
			if (ifconn_manager->ccic_attach_state == IFCONN_NOTIFY_ATTACH) {
				pr_info("usb: [M] %s: IFCONN_NOTIFY_ATTACH\n", __func__);
				ifconn_manager->water_det = 0;
				ifconn_manager->pd_con_state = 0;
			} else {	/* IFCONN_NOTIFY_DETACH */
				pr_info
				    ("usb: [M] %s: IFCONN_NOTIFY_DETACH (pd=%d, cable_type=%d)\n",
				     __func__, ifconn_manager->pd_con_state,
				     ifconn_manager->cable_type);
				if (ifconn_manager->pd_con_state) {
					ifconn_manager->pd_con_state = 0;
					template->src = IFCONN_NOTIFY_CCIC;
					template->dest = IFCONN_NOTIFY_BATTERY;
					template->id = IFCONN_NOTIFY_ID_ATTACH;
					template->attach = IFCONN_NOTIFY_DETACH;
					template->rprd = 0;
					template->cable_type = ATTACHED_DEV_UNOFFICIAL_ID_ANY_MUIC;
					template->data = NULL;
				}
			}
		}
		break;

	case IFCONN_NOTIFY_ID_DETACH:
		template->src = IFCONN_NOTIFY_MANAGER;
		template->dest = IFCONN_NOTIFY_BATTERY;
		_ifconn_manager_template_notify(template);
		return 0;

	case IFCONN_NOTIFY_ID_RID:
		ifconn_manager->ccic_rid_state = template->sub1;
		break;

	case IFCONN_NOTIFY_ID_USB:
		if ((ifconn_manager->cable_type ==
		     IFCONN_CABLE_TYPE_MUIC_CHARGER)
		    || (template->sub2 != IFCONN_MANAGER_USB_STATUS_DETACH	/*drp */
			&& (ifconn_manager->ccic_rid_state ==
			    IFCONN_MANAGER_RID_523K
			    || ifconn_manager->ccic_rid_state ==
			    IFCONN_MANAGER_RID_619K))) {
			return 0;
		}
		break;

	case IFCONN_NOTIFY_ID_WATER:
		if (template->sub1) {	/* attach */
#ifdef CONFIG_IFCONN_WATER_EVENT_ENABLE
			if (!ifconn_manager->water_det) {
				ifconn_manager->water_det = 1;
				ifconn_manager->water_count++;
				complete(&ccic_attach_done);

				template->src = IFCONN_NOTIFY_CCIC;
				template->dest = IFCONN_NOTIFY_MUIC;
				template->id = IFCONN_NOTIFY_ID_WATER;
				template->attach = IFCONN_NOTIFY_ATTACH;
				template->rprd = 0;
				template->cable_type = 0;
				template->pd = NULL;
				ifconn_manager_noti_work(ifconn_manager, template);

				/*update water time */
				water_dry_time_update((int)template->sub1);

				if (ifconn_manager->muic_action == IFCONN_NOTIFY_ATTACH) {
					template->sub3 = ATTACHED_DEV_UNDEFINED_RANGE_MUIC;	/* cable_type */
				} else {
					/* If the cable is not connected, skip the battery event. */
					return 0;
				}
			} else {
				/* Ignore duplicate events */
				return 0;
			}
#else
			if (!ifconn_manager->water_det) {
				ifconn_manager->water_det = 1;
				if (ifconn_manager->muic_action == IFCONN_NOTIFY_ATTACH) {
					template->src = IFCONN_NOTIFY_CCIC;
					template->dest = IFCONN_NOTIFY_BATTERY;
					template->id = IFCONN_NOTIFY_ID_ATTACH;
					template->attach = IFCONN_NOTIFY_DETACH;
					template->rprd = 0;
					template->cable_type =
					    ifconn_manager->muic_cable_type;
					template->data = NULL;
					ifconn_manager_noti_work(ifconn_manager,
								 template);
				}
			}
			return 0;
#endif
		} else {
			ifconn_manager->water_det = 0;
			ifconn_manager->dry_count++;
			return 0;
		}
		break;
	default:
		break;
	}

	ifconn_manager_noti_work(ifconn_manager, template);
	return 0;
}

static int ifconn_manager_manage_vbus(struct ifconn_manager_data *ifconn_manager,
				      struct ifconn_notifier_template *template)
{
	ifconn_notifier_vbus_t vbus_type = template->vbus_type;

	pr_info("usb: [M] %s: vbus_type=%s, WATER DET=%d ATTACH=%s (%d)\n",
		__func__, vbus_type == IFCONN_NOTIFY_VBUS_HIGH ? "HIGH" : "LOW",
		ifconn_manager->water_det,
		ifconn_manager->ccic_attach_state ==
		IFCONN_NOTIFY_ATTACH ? "ATTACH" : "DETATCH",
		ifconn_manager->muic_attach_state_without_ccic);

	ifconn_manager->vbus_state = vbus_type;

#ifdef CONFIG_IFCONN_WATER_EVENT_ENABLE
	init_completion(&ccic_attach_done);
	if ((ifconn_manager->water_det == 1)
	    && (template->vbus_type == IFCONN_NOTIFY_VBUS_HIGH))
		wait_for_completion_timeout(&ccic_attach_done,
					    msecs_to_jiffies(2000));

	switch (vbus_type) {
	case IFCONN_NOTIFY_VBUS_HIGH:
		if (ifconn_manager->water_det) {
			ifconn_manager->waterChg_count++;
			template->src = IFCONN_NOTIFY_CCIC;
			template->dest = IFCONN_NOTIFY_BATTERY;
			template->id = IFCONN_NOTIFY_ID_WATER;
			template->attach = IFCONN_NOTIFY_ATTACH;
			template->rprd = 0;
			template->cable_type =
			    ATTACHED_DEV_UNDEFINED_RANGE_MUIC;
			template->data = NULL;
			_ifconn_manager_template_notify(template);
		}
		break;
	case IFCONN_NOTIFY_VBUS_LOW:
		if (ifconn_manager->water_det) {
			template->src = IFCONN_NOTIFY_CCIC;
			template->dest = IFCONN_NOTIFY_BATTERY;
			template->id = IFCONN_NOTIFY_ID_ATTACH;
			template->attach = IFCONN_NOTIFY_DETACH;
			template->rprd = 0;
			template->cable_type =
			    ATTACHED_DEV_UNDEFINED_RANGE_MUIC;
			template->data = NULL;
			_ifconn_manager_template_notify(template);
		}
		break;
	default:
		break;
	}
#endif
	return 0;
}

static int ifconn_manager_parse_dt(struct ifconn_manager_data *ifconn_manager)
{
	struct device_node *np = ifconn_manager->dev->of_node;
	struct ifconn_manager_platform_data *pdata = ifconn_manager->pdata;
	int ret = 0;

	if (!np)
		return -1;

	ret = of_property_read_string(np,
				      "ifconn,usbpd",
				      (char const **)&pdata->usbpd_name);

	if (ret < 0)
		goto err;

	ret = of_property_read_string(np,
				      "ifconn,muic",
				      (char const **)&pdata->muic_name);

err:
	return ret;
}

#ifdef CONFIG_IFCONN_SUPPORT_THREAD
static struct ifconn_notifier_template *_ifconn_manager_dequeue_template
	(struct ifconn_manager_data *ifconn_manager)
{
	struct ifconn_notifier_template *target = NULL;
	struct ifconn_manager_template *step = NULL;

	if (ifconn_manager->hp) {
		pr_info("%s: ifconn_manager->hp : %p!!!!!\n", __func__, ifconn_manager->hp);
		pr_info("%s: ifconn_manager->hp->node : %p!!!!!\n", __func__, &ifconn_manager->hp->node);
		_ifconn_show_attr(&ifconn_manager->hp->node);

		if (ifconn_manager->hp == ifconn_manager->tp)
			step = NULL;
		else
			step = (struct ifconn_manager_template *)ifconn_manager->hp->np;
		target = &ifconn_manager->hp->node;
		//kfree(ifconn_manager->hp);
		ifconn_manager->hp = step;
		ifconn_manager->template_cnt--;
	}
	pr_info("%s: target : %p!!!!!\n", __func__, target);
	_ifconn_show_attr(target);
	return target;
}

static void _ifconn_manager_enqueue_template
	(struct ifconn_manager_data *ifconn_manager, struct ifconn_notifier_template *template)
{
	struct ifconn_manager_template *template_ifc = NULL;

	template_ifc = kzalloc(sizeof(struct ifconn_manager_template), GFP_KERNEL);
	pr_info("%s: enter --- line : %d!!!!!\n", __func__,  __LINE__);

	memcpy(&template_ifc->node, template,
		sizeof(struct ifconn_notifier_template));

	if (template->data != NULL) {
		switch (template->id) {
		case IFCONN_NOTIFY_ID_POWER_STATUS:
			memcpy(template_ifc->node.data, template->data,
				sizeof(usbpd_pdic_sink_state_t));
			break;
		case IFCONN_NOTIFY_ID_DP_CONNECT:
		case IFCONN_NOTIFY_ID_DP_HPD:
		case IFCONN_NOTIFY_ID_DP_LINK_CONF:
			memcpy(template_ifc->node.data, template->data,
				sizeof(struct usbpd_info));
			break;
		default:
			break;
		}
	}

	if (ifconn_manager->hp == NULL) {
		ifconn_manager->tp = ifconn_manager->hp = template_ifc;
		ifconn_manager->template_cnt++;
	} else {
		if (ifconn_manager->template_cnt < TEMPLATE_MAX)
			ifconn_manager->template_cnt++;
		else
			_ifconn_manager_dequeue_template(ifconn_manager);
		template_ifc->rp = (void *)ifconn_manager->tp;
		ifconn_manager->tp->np = (void *)template_ifc;
		ifconn_manager->tp = template_ifc;
	}
	pr_info("%s: hp : %p!!!!!\n", __func__, ifconn_manager->hp);
	pr_info("%s: tp : %p!!!!!\n", __func__, ifconn_manager->tp);
	pr_info("%s: ifconn_manager->hp->node : %p!!!!!\n", __func__, &ifconn_manager->hp->node);
	_ifconn_show_attr(&ifconn_manager->hp->node);
	_ifconn_show_attr(&ifconn_manager->tp->node);
}
#endif

static void ifconn_manager_work(struct work_struct *work)
{
	struct ifconn_manager_data *ifconn_manager =
	    container_of(work, struct ifconn_manager_data, noti_work);
	struct ifconn_notifier_template *template = NULL;


	pr_info("%s: enter --- line : %d!!!!!\n", __func__,  __LINE__);

	template = ifconn_manager->template;

#ifdef CONFIG_IFCONN_SUPPORT_THREAD
	if (ifconn_manager->template != NULL)
		template = ifconn_manager->template;
	else
		template = _ifconn_manager_dequeue_template(ifconn_manager);
	pr_info("%s: enter	line : %d!!!!!\n", __func__, __LINE__);

#endif

	switch (template->src) {
	case IFCONN_NOTIFY_PDIC:
		ifconn_manager_manage_pdic(ifconn_manager, template);
		break;
	case IFCONN_NOTIFY_MUIC:
		ifconn_manager_manage_muic(ifconn_manager, template);
		break;
	case IFCONN_NOTIFY_CCIC:
		ifconn_manager_manage_ccic(ifconn_manager, template);
		break;
	case IFCONN_NOTIFY_VBUS:
		ifconn_manager_manage_vbus(ifconn_manager, template);
		break;
	case IFCONN_NOTIFY_BATTERY:
		ifconn_manager_manage_battery(ifconn_manager, template);
		break;
	case IFCONN_NOTIFY_DP:
		break;
	case IFCONN_NOTIFY_USB_DP:
		break;
	default:
		break;
	}
}

static int ifconn_manager_isr(struct notifier_block *nb,
			      unsigned long action, void *data)
{
	struct ifconn_manager_data *ifconn_manager =
	    container_of(nb, struct ifconn_manager_data, nb);
	struct ifconn_notifier_template *template =
	    (struct ifconn_notifier_template *)data;

	if (data == NULL) {
		pr_info("%s: data NULL\n", __func__);
		return -1;
	}

	pr_info("%s: enter src : %d\n", __func__, template->src);
#ifdef CONFIG_IFCONN_SUPPORT_THREAD
	mutex_lock(&ifconn_manager->enqueue_mutex);
	ifconn_manager->template = NULL;
	if (template->event	== IFCONN_NOTIFY_EVENT_IN_HURRY) {
#endif
		ifconn_manager->template = template;
		ifconn_manager_work(&ifconn_manager->noti_work);
#ifdef CONFIG_IFCONN_SUPPORT_THREAD
	} else {
		pr_info("%s: enter src : %d, line : %d!!!!!\n", __func__, template->src, __LINE__);

		_ifconn_manager_enqueue_template(ifconn_manager, template);
		schedule_work(&ifconn_manager->noti_work);
	}

	mutex_unlock(&ifconn_manager->enqueue_mutex);
#endif

	return 0;
}

static int ifconn_manager_probe(struct platform_device *pdev)
{
	struct ifconn_manager_data *ifconn_manager;
	struct ifconn_manager_platform_data *pdata = NULL;
	int ret = 0, i = 0;

	pr_info("%s: enter\n", __func__);

	ifconn_manager = kzalloc(sizeof(*ifconn_manager), GFP_KERNEL);
	if (!ifconn_manager)
		return -ENOMEM;

	ifconn_manager->dev = &pdev->dev;
	dev_set_drvdata(ifconn_manager->dev, ifconn_manager);

	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev,
				     sizeof(struct ifconn_manager_platform_data),
				     GFP_KERNEL);
		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto ERROR_PDATA_FREE;
		}

		ifconn_manager->pdata = pdata;

		if (ifconn_manager_parse_dt(ifconn_manager)) {
			dev_err(&pdev->dev,
				"%s: Failed to get ifconn manager dt\n", __func__);
			ret = -EINVAL;
			goto ERROR_IFCONN_FREE;
		}
	} else {
		pdata = dev_get_platdata(&pdev->dev);
		ifconn_manager->pdata = pdata;
	}

	if (ret < 0)
		goto ERROR_PDATA_FREE;

	mutex_init(&ifconn_manager->noti_mutex);
	mutex_init(&ifconn_manager->workqueue_mutex);
#ifdef CONFIG_IFCONN_SUPPORT_THREAD
	mutex_init(&ifconn_manager->enqueue_mutex);
	INIT_WORK(&ifconn_manager->noti_work, ifconn_manager_work);
	ifconn_manager->hp = NULL;
	ifconn_manager->tp = NULL;
	ifconn_manager->template = NULL;
	ifconn_manager->template_cnt = 0;
#endif
	for (i = IFCONN_NOTIFY_USB; i <= IFCONN_NOTIFY_USB_DP; i++) {
		ret = ifconn_notifier_register(&ifconn_manager->nb,
					       ifconn_manager_isr,
					       IFCONN_NOTIFY_MANAGER, i);
	}

	return 0;

ERROR_PDATA_FREE:
	kfree(pdata);
ERROR_IFCONN_FREE:
	kfree(ifconn_manager);

	return ret;
}

static int ifconn_manager_remove(struct platform_device *pdev)
{
	struct ifconn_manager_data *ifconn_manager = platform_get_drvdata(pdev);

	mutex_destroy(&ifconn_manager->noti_mutex);
	return 0;
}

static void ifconn_manager_shutdown(struct device *dev)
{
}

#ifdef CONFIG_OF
static struct of_device_id ifconn_manager_dt_ids[] = {
	{.compatible = "samsung,ifconn"},
	{}
};

MODULE_DEVICE_TABLE(of, ifconn_manager_dt_ids);
#endif /* CONFIG_OF */

static struct platform_driver ifconn_manager_driver = {
	.driver = {
		   .name = "ifconn",
		   .owner = THIS_MODULE,
		   .shutdown = ifconn_manager_shutdown,
#ifdef CONFIG_OF
		   .of_match_table = ifconn_manager_dt_ids,
#endif
		   },
	.probe = ifconn_manager_probe,
	.remove = ifconn_manager_remove,
};

static int __init ifconn_manager_init(void)
{
	return platform_driver_register(&ifconn_manager_driver);
}

static void __exit ifconn_manager_exit(void)
{
	platform_driver_unregister(&ifconn_manager_driver);
}

late_initcall(ifconn_manager_init);
module_exit(ifconn_manager_exit);

MODULE_DESCRIPTION("ifconn manager");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
