#include <linux/device.h>
#include <linux/module.h>

#include <linux/notifier.h>
#include <linux/ccic/ccic_notifier.h>
#include <linux/sec_class.h>
#include <linux/ccic/ccic_sysfs.h>
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
#include <linux/battery/battery_notifier.h>
#endif
#include <linux/usb_notify.h>
#include <linux/ccic/ccic_core.h>

#define DEBUG
#define SET_CCIC_NOTIFIER_BLOCK(nb, fn, dev) do {	\
		(nb)->notifier_call = (fn);		\
		(nb)->priority = (dev);			\
	} while (0)

#define DESTROY_CCIC_NOTIFIER_BLOCK(nb)			\
		SET_CCIC_NOTIFIER_BLOCK(nb, NULL, -1)

static struct ccic_notifier_struct ccic_notifier;

extern struct device *ccic_device;
static int ccic_notifier_init_done;
int ccic_notifier_init(void);

char CCIC_NOTI_DEST_Print[CCIC_NOTI_DEST_NUM][10] = {
    {"INITIAL"},
    {"USB"},
    {"BATTERY"},
    {"PDIC"},
    {"MUIC"},
    {"CCIC"},
    {"MANAGER"},
    {"DP"},
    {"DPUSB"},
    {"BATTERY2"},
    {"MUIC2"},
    {"ALL"},
};

char CCIC_NOTI_ID_Print[CCIC_NOTI_ID_NUM][20] = {
    {"ID_INITIAL"},
    {"ID_ATTACH"},
    {"ID_RID"},
    {"ID_USB"},
    {"ID_POWER_STATUS"},
    {"ID_WATER"},
    {"ID_VCONN"},
    {"ID_DP_CONNECT"},
    {"ID_DP_HPD"},
    {"ID_DP_LINK_CONF"},
    {"ID_DP_USB"},
    {"ID_ROLE_SWAP"},
    {"ID_FAC"},
    {"ID_PIN_STATUS"},
    {"ID_WATER_CABLE"},
};

char CCIC_NOTI_RID_Print[CCIC_NOTI_RID_NUM][15] = {
    {"RID_UNDEFINED"},
    {"RID_000K"},
    {"RID_001K"},
    {"RID_255K"},
    {"RID_301K"},
    {"RID_523K"},
    {"RID_619K"},
    {"RID_OPEN"},
};

char CCIC_NOTI_USB_STATUS_Print[CCIC_NOTI_USB_STATUS_NUM][20] = {
    {"USB_DETACH"},
    {"USB_ATTACH_DFP"},
    {"USB_ATTACH_UFP"},
    {"USB_ATTACH_DRP"},
    {"USB_ATTACH_NO_USB"},
};

char CCIC_NOTI_PIN_STATUS_Print[CCIC_NOTI_PIN_STATUS_NUM][20] = {
    {"NO_DETERMINATION"},
    {"CC1_ACTIVE"},
    {"CC2_ACTIVE"},
    {"AUDIO_ACCESSORY"},
    {"DEBUG_ACCESSORY"},
    {"CCIC_ERROR"},
    {"DISABLED"},
    {"RFU"},
};

int ccic_notifier_register(struct notifier_block *nb, notifier_fn_t notifier,
			ccic_notifier_device_t listener)
{
	int ret = 0;

	pr_info("%s: listener=%d register\n", __func__, listener);

	/* Check if CCIC Notifier is ready. */
	if (!ccic_notifier_init_done)
		ccic_notifier_init();

	if (!ccic_device) {
		pr_err("%s: Not Initialized...\n", __func__);
		return -1;
	}

	SET_CCIC_NOTIFIER_BLOCK(nb, notifier, listener);
	ret = blocking_notifier_chain_register(&(ccic_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_register error(%d)\n",
				__func__, ret);

	/* current ccic's attached_device status notify */
	nb->notifier_call(nb, 0,
			&(ccic_notifier.ccic_template));

	return ret;
}

int ccic_notifier_unregister(struct notifier_block *nb)
{
	int ret = 0;

	pr_info("%s: listener=%d unregister\n", __func__, nb->priority);

	ret = blocking_notifier_chain_unregister(&(ccic_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_unregister error(%d)\n",
				__func__, ret);
	DESTROY_CCIC_NOTIFIER_BLOCK(nb);

	return ret;
}

static void ccic_uevent_work(int id, int state)
{
	char *water[2] = { "CCIC=WATER", NULL };
	char *dry[2] = { "CCIC=DRY", NULL };
	char *vconn[2] = { "CCIC=VCONN", NULL };
#if defined(CONFIG_SEC_FACTORY)
	char ccicrid[15] = {0,};
	char *rid[2] = {ccicrid, NULL};
	char ccicFacErr[20] = {0,};
	char *facErr[2] = {ccicFacErr, NULL};
	char ccicPinStat[20] = {0,};
	char *pinStat[2] = {ccicPinStat, NULL};
#endif

	pr_info("usb: %s: id=%s state=%d\n", __func__, CCIC_NOTI_ID_Print[id], state);

	switch (id) {
	case CCIC_NOTIFY_ID_WATER:
		if (state)
			kobject_uevent_env(&ccic_device->kobj, KOBJ_CHANGE, water);
		else
			kobject_uevent_env(&ccic_device->kobj, KOBJ_CHANGE, dry);
		break;
	case CCIC_NOTIFY_ID_VCONN:
		kobject_uevent_env(&ccic_device->kobj, KOBJ_CHANGE, vconn);
		break;
#if defined(CONFIG_SEC_FACTORY)
	case CCIC_NOTIFY_ID_RID:
		snprintf(ccicrid, sizeof(ccicrid), "%s",
			(state < CCIC_NOTI_RID_NUM) ? CCIC_NOTI_RID_Print[state] : CCIC_NOTI_RID_Print[0]);
		kobject_uevent_env(&ccic_device->kobj, KOBJ_CHANGE, rid);
		break;
	case CCIC_NOTIFY_ID_FAC:
		snprintf(ccicFacErr, sizeof(ccicFacErr), "%s:%d",
			"ERR_STATE", state);
		kobject_uevent_env(&ccic_device->kobj, KOBJ_CHANGE, facErr);
		break;
	case CCIC_NOTIFY_ID_CC_PIN_STATUS:
		snprintf(ccicPinStat, sizeof(ccicPinStat), "%s",
			(state < CCIC_NOTI_PIN_STATUS_NUM) ?
				CCIC_NOTI_PIN_STATUS_Print[state] : CCIC_NOTI_PIN_STATUS_Print[0]);
		kobject_uevent_env(&ccic_device->kobj, KOBJ_CHANGE, pinStat);
		break;
#endif
	default:
		break;
	}
}

/* ccic's attached_device attach broadcast */
int ccic_notifier_notify(CC_NOTI_TYPEDEF *p_noti, void *pd, int pdic_attach)
{
	int ret = 0;
	ccic_notifier.ccic_template = *p_noti;

	switch (p_noti->id) {
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	case CCIC_NOTIFY_ID_POWER_STATUS:		// PDIC_NOTIFY_EVENT_PD_SINK
		pr_info("%s: src:%01x dest:%01x id:%02x "
			"attach:%02x cable_type:%02x rprd:%01x\n", __func__,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->src,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->dest,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->id,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->attach,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->cable_type,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->rprd);

		if (pd != NULL) {
			if (!((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->attach &&
				((struct pdic_notifier_struct *)pd)->event != PDIC_NOTIFY_EVENT_CCIC_ATTACH) {
				((struct pdic_notifier_struct *)pd)->event = PDIC_NOTIFY_EVENT_DETACH;
			}
			ccic_notifier.ccic_template.pd = pd;

			pr_info("%s: PD event:%d, num:%d, sel:%d \n", __func__,
				((struct pdic_notifier_struct *)pd)->event,
				((struct pdic_notifier_struct *)pd)->sink_status.available_pdo_num,
				((struct pdic_notifier_struct *)pd)->sink_status.selected_pdo_num);
		}
		break;
#endif
	case CCIC_NOTIFY_ID_ATTACH:
		pr_info("%s: src:%01x dest:%01x id:%02x "
			"attach:%02x cable_type:%02x rprd:%01x\n", __func__,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->src,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->dest,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->id,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->attach,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->cable_type,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->rprd);
		break;
	case CCIC_NOTIFY_ID_RID:
		pr_info("%s: src:%01x dest:%01x id:%02x rid:%02x\n", __func__,
			((CC_NOTI_RID_TYPEDEF *)p_noti)->src,
			((CC_NOTI_RID_TYPEDEF *)p_noti)->dest,
			((CC_NOTI_RID_TYPEDEF *)p_noti)->id,
			((CC_NOTI_RID_TYPEDEF *)p_noti)->rid);
#if defined(CONFIG_SEC_FACTORY)
			ccic_uevent_work(CCIC_NOTIFY_ID_RID,((CC_NOTI_RID_TYPEDEF *)p_noti)->rid);
#endif
		break;
#ifdef CONFIG_SEC_FACTORY
	case CCIC_NOTIFY_ID_FAC:
		pr_info("%s: src:%01x dest:%01x id:%02x ErrState:%02x\n", __func__,
			p_noti->src, p_noti->dest, p_noti->id, p_noti->sub1);
			ccic_uevent_work(CCIC_NOTIFY_ID_FAC, p_noti->sub1);
			return 0;
#endif
	case CCIC_NOTIFY_ID_WATER:
		pr_info("%s: src:%01x dest:%01x id:%02x attach:%02x\n", __func__,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->src,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->dest,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->id,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->attach);
			ccic_uevent_work(CCIC_NOTIFY_ID_WATER, ((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->attach);
		break;
	case CCIC_NOTIFY_ID_VCONN:
		ccic_uevent_work(CCIC_NOTIFY_ID_VCONN, 0);
		break;
	case CCIC_NOTIFY_ID_ROLE_SWAP:
		pr_info("%s: src:%01x dest:%01x id:%02x sub1:%02x\n", __func__,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->src,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->dest,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->id,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->attach);
		break;
#ifdef CONFIG_SEC_FACTORY
	case CCIC_NOTIFY_ID_CC_PIN_STATUS:
		pr_info("%s: src:%01x dest:%01x id:%02x pinStatus:%02x\n", __func__,
			p_noti->src, p_noti->dest, p_noti->id, p_noti->sub1);
			ccic_uevent_work(CCIC_NOTIFY_ID_CC_PIN_STATUS, p_noti->sub1);
			return 0;
#endif
	default:
		pr_info("%s: src:%01x dest:%01x id:%02x "
			"sub1:%d sub2:%02x sub3:%02x\n", __func__,
			((CC_NOTI_TYPEDEF *)p_noti)->src,
			((CC_NOTI_TYPEDEF *)p_noti)->dest,
			((CC_NOTI_TYPEDEF *)p_noti)->id,
			((CC_NOTI_TYPEDEF *)p_noti)->sub1,
			((CC_NOTI_TYPEDEF *)p_noti)->sub2,
			((CC_NOTI_TYPEDEF *)p_noti)->sub3);
		break;
	}
#ifdef CONFIG_USB_NOTIFY_PROC_LOG
	store_usblog_notify(NOTIFY_CCIC_EVENT, (void *)p_noti, NULL);
#endif
	ret = blocking_notifier_call_chain(&(ccic_notifier.notifier_call_chain),
			p_noti->id, &(ccic_notifier.ccic_template));


	switch (ret) {
	case NOTIFY_STOP_MASK:
	case NOTIFY_BAD:
		pr_err("%s: notify error occur(0x%x)\n", __func__, ret);
		break;
	case NOTIFY_DONE:
	case NOTIFY_OK:
		pr_info("%s: notify done(0x%x)\n", __func__, ret);
		break;
	default:
		pr_info("%s: notify status unknown(0x%x)\n", __func__, ret);
		break;
	}

	return ret;

}

int ccic_notifier_init(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);
	if (ccic_notifier_init_done) {
		pr_err("%s already registered\n", __func__);
		goto out;
	}
	ccic_notifier_init_done = 1;
	ccic_core_init();
	BLOCKING_INIT_NOTIFIER_HEAD(&(ccic_notifier.notifier_call_chain));

out:
	return ret;
}

static void __exit ccic_notifier_exit(void)
{
	pr_info("%s: exit\n", __func__);
}

device_initcall(ccic_notifier_init);
module_exit(ccic_notifier_exit);
