#include <linux/device.h>

#include <linux/notifier.h>
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#include <linux/sec_class.h>

#define NOTI_ADDR_DST (0xf)
#define NOTI_ID_ATTACH (1)

#define SET_MUIC_NOTIFIER_BLOCK(nb, fn, dev) do {	\
		(nb)->notifier_call = (fn);		\
		(nb)->priority = (dev);			\
	} while (0)

#define DESTROY_MUIC_NOTIFIER_BLOCK(nb)			\
		SET_MUIC_NOTIFIER_BLOCK(nb, NULL, -1)

static struct muic_notifier_struct muic_notifier;
struct device *switch_device;

static void __set_noti_cxt(CC_NOTI_ATTACH_TYPEDEF *pcxt, int attach, int type)
{
	pcxt->cable_type = type % SECOND_MUIC_DEV;
	pcxt->attach = attach;
}

int muic_notifier_register(struct notifier_block *nb, notifier_fn_t notifier,
	muic_notifier_device_t listener)
{
	int ret = 0;
	CC_NOTI_ATTACH_TYPEDEF *pcxt = &(muic_notifier.cxt);
#if IS_ENABLED(CONFIG_USE_SECOND_MUIC)
	CC_NOTI_ATTACH_TYPEDEF *pcxt2 = &(muic_notifier.cxt2);
#endif

	pr_info("%s: listener=%d register\n", __func__, listener);

	SET_MUIC_NOTIFIER_BLOCK(nb, notifier, listener);
	ret = blocking_notifier_chain_register(&(muic_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_register error(%d)\n",
				__func__, ret);

	/* current muic's attached_device status notify */
	nb->notifier_call(nb, pcxt->attach, pcxt);
#if IS_ENABLED(CONFIG_USE_SECOND_MUIC)
	nb->notifier_call(nb, pcxt2->attach, pcxt2);
#endif

	return ret;
}

int muic_notifier_unregister(struct notifier_block *nb)
{
	int ret = 0;

	pr_info("%s: listener=%d unregister\n", __func__, nb->priority);

	ret = blocking_notifier_chain_unregister(&(muic_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_unregister error(%d)\n",
				__func__, ret);
	DESTROY_MUIC_NOTIFIER_BLOCK(nb);

	return ret;
}

static int muic_notifier_notify(CC_NOTI_ATTACH_TYPEDEF *pcxt)
{
	int ret = 0;

	pr_info("%s (%d)%stach\n",
		__func__, pcxt->cable_type,
		pcxt->attach ? "At" : "De");

#ifdef CONFIG_SEC_FACTORY
	if (pcxt->attach != 0)
		muic_send_attached_muic_cable_intent(pcxt->cable_type);
	else
		muic_send_attached_muic_cable_intent(0);
#endif

	ret = blocking_notifier_call_chain(&(muic_notifier.notifier_call_chain),
			pcxt->attach, pcxt);

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

void muic_notifier_attach_attached_dev(muic_attached_dev_t new_dev)
{
	CC_NOTI_ATTACH_TYPEDEF *pcxt = &(muic_notifier.cxt);

	mutex_lock(&muic_notifier.mutex);

#if IS_ENABLED(CONFIG_USE_SECOND_MUIC)
	if (new_dev >= SECOND_MUIC_DEV)
		pcxt = &(muic_notifier.cxt2);
#endif

	__set_noti_cxt(pcxt, MUIC_NOTIFY_CMD_ATTACH, new_dev);

	/* muic's attached_device attach broadcast */
	muic_notifier_notify(pcxt);

	mutex_unlock(&muic_notifier.mutex);
}

void muic_notifier_detach_attached_dev(muic_attached_dev_t cur_dev)
{
	CC_NOTI_ATTACH_TYPEDEF *pcxt = &(muic_notifier.cxt);

	mutex_lock(&muic_notifier.mutex);

#if IS_ENABLED(CONFIG_USE_SECOND_MUIC)
	if (cur_dev >= SECOND_MUIC_DEV)
		pcxt = &(muic_notifier.cxt2);
#endif

	pr_info("attached_dev(%d), cur_dev(%d)\n",
		pcxt->cable_type, cur_dev);

	pcxt->attach = MUIC_NOTIFY_CMD_DETACH;

	if (pcxt->cable_type != ATTACHED_DEV_NONE_MUIC)
		muic_notifier_notify(pcxt);

	__set_noti_cxt(pcxt, MUIC_NOTIFY_CMD_DETACH, ATTACHED_DEV_NONE_MUIC);

	mutex_unlock(&muic_notifier.mutex);
}

void muic_notifier_logically_attach_attached_dev(muic_attached_dev_t new_dev)
{
	CC_NOTI_ATTACH_TYPEDEF *pcxt = &(muic_notifier.cxt);

	mutex_lock(&muic_notifier.mutex);

	__set_noti_cxt(pcxt, MUIC_NOTIFY_CMD_ATTACH, new_dev);

	/* muic's attached_device attach broadcast */
	muic_notifier_notify(pcxt);

	mutex_unlock(&muic_notifier.mutex);
}

void muic_notifier_logically_detach_attached_dev(muic_attached_dev_t cur_dev)
{
	CC_NOTI_ATTACH_TYPEDEF *pcxt = &(muic_notifier.cxt);

	mutex_lock(&muic_notifier.mutex);

	__set_noti_cxt(pcxt, MUIC_NOTIFY_CMD_DETACH, cur_dev);

	/* muic's attached_device detach broadcast */
	muic_notifier_notify(pcxt);

	__set_noti_cxt(pcxt, MUIC_NOTIFY_CMD_DETACH, ATTACHED_DEV_NONE_MUIC);

	mutex_unlock(&muic_notifier.mutex);
}

static int __init muic_notifier_init(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	switch_device = sec_device_create(NULL, "switch");
	if (IS_ERR(switch_device)) {
		pr_err("%s Failed to create device(switch)!\n", __func__);
		ret = -ENODEV;
		goto out;
	}

	muic_notifier.cxt.dest = NOTI_ADDR_DST;
	muic_notifier.cxt.id = NOTI_ID_ATTACH;
	muic_notifier.cxt.rprd = 0;
	muic_notifier.cxt.src = 	CCIC_NOTIFY_DEV_MUIC;
	muic_notifier.cxt.cable_type = ATTACHED_DEV_UNKNOWN_MUIC;
	muic_notifier.cxt.attach = MUIC_NOTIFY_CMD_DETACH;

#if IS_ENABLED(CONFIG_USE_SECOND_MUIC)
	muic_notifier.cxt2.dest = NOTI_ADDR_DST;
	muic_notifier.cxt2.id = NOTI_ID_ATTACH;
	muic_notifier.cxt2.rprd = 0;
	muic_notifier.cxt2.src = 	CCIC_NOTIFY_DEV_SECOND_MUIC;
	muic_notifier.cxt2.cable_type = ATTACHED_DEV_UNKNOWN_MUIC;
	muic_notifier.cxt2.attach = MUIC_NOTIFY_CMD_DETACH;
#endif

	mutex_init(&muic_notifier.mutex);
	BLOCKING_INIT_NOTIFIER_HEAD(&(muic_notifier.notifier_call_chain));
out:
	return ret;
}

device_initcall(muic_notifier_init);

