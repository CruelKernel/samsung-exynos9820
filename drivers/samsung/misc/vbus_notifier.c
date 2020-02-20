#include <linux/device.h>
#include <linux/module.h>

#include <linux/notifier.h>
#include <linux/sec_class.h>
#include <linux/vbus_notifier.h>

#define SET_VBUS_NOTIFIER_BLOCK(nb, fn, dev) do {	\
		(nb)->notifier_call = (fn);		\
		(nb)->priority = (dev);			\
	} while (0)

#define DESTROY_VBUS_NOTIFIER_BLOCK(nb)			\
		SET_VBUS_NOTIFIER_BLOCK(nb, NULL, -1)

static struct vbus_notifier_struct vbus_notifier;
struct blocking_notifier_head vbus_notifier_head =
	BLOCKING_NOTIFIER_INIT(vbus_notifier_head);
static DEFINE_MUTEX(vbus_mutex);

int vbus_notifier_register(struct notifier_block *nb, notifier_fn_t notifier,
			vbus_notifier_device_t listener)
{
	int ret = 0;

	pr_info("%s: listener=%d register\n", __func__, listener);

	SET_VBUS_NOTIFIER_BLOCK(nb, notifier, listener);
	ret = blocking_notifier_chain_register(&vbus_notifier_head, nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_register error(%d)\n",
				__func__, ret);

	mutex_lock(&vbus_mutex);
	if (vbus_notifier.vbus_type != STATUS_VBUS_UNKNOWN)
		nb->notifier_call(nb, vbus_notifier.cmd, &(vbus_notifier.vbus_type));
	mutex_unlock(&vbus_mutex);

	return ret;
}

int vbus_notifier_unregister(struct notifier_block *nb)
{
	int ret = 0;

	pr_info("%s: listener=%d unregister\n", __func__, nb->priority);

	ret = blocking_notifier_chain_unregister(&vbus_notifier_head, nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_unregister error(%d)\n",
				__func__, ret);
	DESTROY_VBUS_NOTIFIER_BLOCK(nb);

	return ret;
}

static int vbus_notifier_notify(void)
{
	int ret = 0;

	pr_info("%s: CMD=%d, DATA=%d\n", __func__, vbus_notifier.cmd,
			vbus_notifier.vbus_type);

	ret = blocking_notifier_call_chain(&vbus_notifier_head,
			vbus_notifier.cmd, &(vbus_notifier.vbus_type));

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

void vbus_notifier_handle(vbus_status_t new_dev)
{
	pr_info("%s: (%d)->(%d)\n", __func__, vbus_notifier.vbus_type, new_dev);

	if (vbus_notifier.vbus_type == new_dev)
		return;

	mutex_lock(&vbus_mutex);
	if (new_dev == STATUS_VBUS_HIGH)
		vbus_notifier.cmd = VBUS_NOTIFY_CMD_RISING;
	else if (new_dev == STATUS_VBUS_LOW)
		vbus_notifier.cmd = VBUS_NOTIFY_CMD_FALLING;

	vbus_notifier.vbus_type = new_dev;
	mutex_unlock(&vbus_mutex);

	/* vbus attach broadcast */
	vbus_notifier_notify();
}

