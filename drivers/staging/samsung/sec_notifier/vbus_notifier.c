#include <linux/device.h>
#include <linux/module.h>

#include <linux/notifier.h>
#include <linux/sec_sysfs.h>
#include <linux/vbus_notifier.h>

#define SET_VBUS_NOTIFIER_BLOCK(nb, fn, dev) do {	\
		(nb)->notifier_call = (fn);		\
		(nb)->priority = (dev);			\
	} while (0)

#define DESTROY_VBUS_NOTIFIER_BLOCK(nb)			\
		SET_VBUS_NOTIFIER_BLOCK(nb, NULL, -1)

static struct vbus_notifier_struct vbus_notifier;

int vbus_notifier_register(struct notifier_block *nb, notifier_fn_t notifier,
			vbus_notifier_device_t listener)
{
	int ret = 0;

	pr_info("%s: listener=%d register\n", __func__, listener);

	SET_VBUS_NOTIFIER_BLOCK(nb, notifier, listener);
	ret = blocking_notifier_chain_register(&(vbus_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_register error(%d)\n",
				__func__, ret);

	mutex_lock(&vbus_notifier.vbus_mutex);
	if (vbus_notifier.status != VBUS_NOTIFIER_READY) {
		pr_err("%s vbus_notifier is not ready!\n", __func__);
	} else {
		/* current vbus type status notify */
		nb->notifier_call(nb, vbus_notifier.cmd,
				&(vbus_notifier.vbus_type));
	}
	mutex_unlock(&vbus_notifier.vbus_mutex);

	return ret;
}

int vbus_notifier_unregister(struct notifier_block *nb)
{
	int ret = 0;

	pr_info("%s: listener=%d unregister\n", __func__, nb->priority);

	ret = blocking_notifier_chain_unregister(&(vbus_notifier.notifier_call_chain), nb);
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

	ret = blocking_notifier_call_chain(&(vbus_notifier.notifier_call_chain),
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

	mutex_lock(&vbus_notifier.vbus_mutex);
	if (new_dev == STATUS_VBUS_HIGH)
		vbus_notifier.cmd = VBUS_NOTIFY_CMD_RISING;
	else if (new_dev == STATUS_VBUS_LOW)
		vbus_notifier.cmd = VBUS_NOTIFY_CMD_FALLING;

	vbus_notifier.vbus_type = new_dev;

	if (vbus_notifier.status != VBUS_NOTIFIER_READY) {
		pr_err("%s vbus_notifier is not ready!\n", __func__);
		vbus_notifier.status = VBUS_NOTIFIER_NOT_READY_DETECT;
		mutex_unlock(&vbus_notifier.vbus_mutex);
		return;
	}
	mutex_unlock(&vbus_notifier.vbus_mutex);

	/* vbus attach broadcast */
	vbus_notifier_notify();
}

static int __init vbus_notifier_init(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	mutex_init(&vbus_notifier.vbus_mutex);

	BLOCKING_INIT_NOTIFIER_HEAD(&(vbus_notifier.notifier_call_chain));

	mutex_lock(&vbus_notifier.vbus_mutex);
	if (vbus_notifier.status == VBUS_NOTIFIER_NOT_READY_DETECT) {
		pr_info("%s init vbus_notifier_notify\n", __func__);
		vbus_notifier.status = VBUS_NOTIFIER_READY;
		vbus_notifier_notify();
	} else {
		vbus_notifier.cmd = VBUS_NOTIFY_CMD_NONE;
		vbus_notifier.vbus_type = STATUS_VBUS_UNKNOWN;
		vbus_notifier.status = VBUS_NOTIFIER_READY;
	}
	mutex_unlock(&vbus_notifier.vbus_mutex);

	return ret;
}
device_initcall(vbus_notifier_init);

static void __exit vbus_notifier_exit(void)
{
	pr_info("%s\n", __func__);
	mutex_destroy(&vbus_notifier.vbus_mutex);
}
module_exit(vbus_notifier_exit);
