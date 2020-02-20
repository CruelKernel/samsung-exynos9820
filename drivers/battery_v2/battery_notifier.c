#include <linux/device.h>

#include <linux/notifier.h>
#include <linux/battery/battery_notifier.h>
#include <linux/sec_class.h>

#define DEBUG
#define SET_BATTERY_NOTIFIER_BLOCK(nb, fn, dev) do {	\
		(nb)->notifier_call = (fn);		\
		(nb)->priority = (dev);			\
	} while (0)

#define DESTROY_BATTERY_NOTIFIER_BLOCK(nb)			\
		SET_BATTERY_NOTIFIER_BLOCK(nb, NULL, -1)

static struct charger_notifier_struct charger_notifier;
static struct pdic_notifier_struct pdic_notifier;

struct device *charger_device;
struct device *pdic_device;

int charger_notifier_register(struct notifier_block *nb, notifier_fn_t notifier,
			charger_notifier_device_t listener)
{
	int ret = 0;

	pr_info("%s: listener=%d register\n", __func__, listener);

	/* Check if CHARGER Notifier is ready. */
	if (!charger_device) {
		pr_err("%s: Not Initialized...\n", __func__);
		return -1;
	}

	SET_BATTERY_NOTIFIER_BLOCK(nb, notifier, listener);
	ret = blocking_notifier_chain_register(&(charger_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_register error(%d)\n",
				__func__, ret);

	/* current charger's attached_device status notify */
	nb->notifier_call(nb, charger_notifier.event,
			&(charger_notifier));

	return ret;
}

int charger_notifier_unregister(struct notifier_block *nb)
{
	int ret = 0;

	pr_info("%s: listener=%d unregister\n", __func__, nb->priority);

	ret = blocking_notifier_chain_unregister(&(charger_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_unregister error(%d)\n",
				__func__, ret);
	DESTROY_BATTERY_NOTIFIER_BLOCK(nb);

	return ret;
}

int pdic_notifier_register(struct notifier_block *nb, notifier_fn_t notifier,
			pdic_notifier_device_t listener)
{
	int ret = 0;

	pr_info("%s: listener=%d register\n", __func__, listener);

	/* Check if CHARGER Notifier is ready. */
	if (!pdic_device) {
		pr_err("%s: Not Initialized...\n", __func__);
		return -1;
	}

	SET_BATTERY_NOTIFIER_BLOCK(nb, notifier, listener);
	ret = blocking_notifier_chain_register(&(pdic_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_register error(%d)\n",
				__func__, ret);

	/* current pdic's attached_device status notify */
	nb->notifier_call(nb, pdic_notifier.event,
			&(pdic_notifier));

	return ret;
}

int pdic_notifier_unregister(struct notifier_block *nb)
{
	int ret = 0;

	pr_info("%s: listener=%d unregister\n", __func__, nb->priority);

	ret = blocking_notifier_chain_unregister(&(pdic_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_unregister error(%d)\n",
				__func__, ret);
	DESTROY_BATTERY_NOTIFIER_BLOCK(nb);

	return ret;
}

static int battery_notifier_notify(int type)
{
	int ret = 0;

	switch (type) {
	case CHARGER_NOTIFY:
		ret = blocking_notifier_call_chain(&(charger_notifier.notifier_call_chain),
				charger_notifier.event, &(charger_notifier));
		break;
	case PDIC_NOTIFY:
		ret = blocking_notifier_call_chain(&(pdic_notifier.notifier_call_chain),
				pdic_notifier.event, &(pdic_notifier));
		break;
	default:
		pr_info("%s: notify status unknown(0x%x)\n", __func__, ret);
		break;
	}

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

static void charger_notifier_set_property(struct charger_notifier_struct * value)
{
	charger_notifier.event = value->event;
	switch(value->event) {
		case CHARGER_NOTIFY_EVENT_AICL:
			charger_notifier.aicl_status.input_current = value->aicl_status.input_current;
			break;
		default:
			break;
	}
}

void charger_notifier_call(struct charger_notifier_struct *value)
{
	/* charger's event broadcast */
	pr_info("%s: CHARGER_NOTIFY_EVENT :%d\n", __func__, value->event);
	charger_notifier_set_property(value);
	battery_notifier_notify(CHARGER_NOTIFY);
}

static void pdic_notifier_set_property(struct pdic_notifier_struct *value)
{
	pdic_notifier.event = value->event;
	switch(value->event) {
		case PDIC_NOTIFY_EVENT_PD_SINK:
			pdic_notifier.sink_status = value->sink_status;
			break;
		default:
			break;
	}
}

void pdic_notifier_call(struct pdic_notifier_struct *value)
{
	/* pdic's event broadcast */
	pdic_notifier_set_property(value);
	battery_notifier_notify(PDIC_NOTIFY);
}

int battery_notifier_init(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	charger_device = sec_device_create(NULL, "charger_notifier");
	pdic_device = sec_device_create(NULL, "pdic_notifier");
	if (IS_ERR(charger_device)) {
		pr_err("%s Failed to create device(charger_notifier)!\n", __func__);
		ret = -ENODEV;
		goto out;
	}
	if (IS_ERR(pdic_device)) {
		pr_err("%s Failed to create device(pdic_notifier)!\n", __func__);
		ret = -ENODEV;
		goto out;
	}

	BLOCKING_INIT_NOTIFIER_HEAD(&(charger_notifier.notifier_call_chain));
	BLOCKING_INIT_NOTIFIER_HEAD(&(pdic_notifier.notifier_call_chain));

out:
	return ret;
}
device_initcall(battery_notifier_init);

