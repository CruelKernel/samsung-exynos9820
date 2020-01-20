#include <linux/device.h>
#include <linux/module.h>

#include <linux/notifier.h>
#include <linux/ifconn/ifconn_notifier.h>
#include <linux/ifconn/ifconn_manager.h>

#define DEBUG
#define SET_IFCONN_NOTIFIER_BLOCK(nb, fn, dev) do {	\
		(nb)->notifier_call = (fn);		\
		(nb)->priority = (dev);			\
} while (0)

#define DESTROY_IFCONN_NOTIFIER_BLOCK(nb)			\
		SET_IFCONN_NOTIFIER_BLOCK(nb, NULL, -1)

static char IFCONN_NOTI_DEV_Print[IFCONN_NOTIFY_MAX][10] = {
	{"MANAGER"},
	{"USB"},
	{"BATTERY"},
	{"PDIC"},
	{"MUIC"},
	{"VBUS"},
	{"CCIC"},
	{"DP"},
	{"DPUSB"},
	{"ALL"},
};

static struct ifconn_notifier pnoti[IFCONN_NOTIFY_MAX];

void _ifconn_show_attr(struct ifconn_notifier_template *t)
{
	if (t == NULL)
		return;

	pr_info("%s, src:%s, dest:%s, id:%d, event:%d, data:0x%p\n",
		__func__, IFCONN_NOTI_DEV_Print[t->src], IFCONN_NOTI_DEV_Print[t->dest], t->id, t->event, t->data);
}

int ifconn_notifier_register(struct notifier_block *nb, notifier_fn_t notifier,
			     ifconn_notifier_t listener, ifconn_notifier_t src)
{
	int ret = 0;
	struct ifconn_notifier_template *template = NULL;

	pr_info("%s: src : %d =? listner : %d, nb : %p  register\n", __func__,
		src, listener, nb);

	if (src >= IFCONN_NOTIFY_ALL || src < 0
	    || listener >= IFCONN_NOTIFY_ALL || listener < 0) {
		pr_err("%s: dev index err\n", __func__);
		return -1;
	}

	SET_IFCONN_NOTIFIER_BLOCK(nb, notifier, listener);
	ret = blocking_notifier_chain_register(&(pnoti[src].notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_register error(%d)\n",
		       __func__, ret);

	pnoti[src].nb[listener] = nb;
	if (pnoti[src].sent[listener] || pnoti[src].sent[IFCONN_NOTIFY_ALL]) {
		pr_info("%s: src : %d, listner : %d, sent : %d, sent all : %d\n", __func__,
			src, listener, pnoti[src].sent[listener], pnoti[src].sent[IFCONN_NOTIFY_ALL]);

		if (!pnoti[src].sent[listener] && pnoti[src].sent[IFCONN_NOTIFY_ALL]) {
			template = &pnoti[src].ifconn_template[IFCONN_NOTIFY_ALL];
			_ifconn_show_attr(template);
			template->src = src;
			template->dest = listener;
		} else {
			template = &(pnoti[src].ifconn_template[listener]);
		}
		_ifconn_show_attr(template);
		ret = nb->notifier_call(nb,	template->id, template);
	}

	return ret;
}

int ifconn_notifier_unregister(ifconn_notifier_t src,
			       ifconn_notifier_t listener)
{
	int ret = 0;
	struct notifier_block *nb = NULL;

	if (src >= IFCONN_NOTIFY_ALL || src < 0
	    || listener >= IFCONN_NOTIFY_ALL || listener < 0) {
		pr_err("%s: dev index err\n", __func__);
		return -1;
	}
	nb = pnoti[src].nb[listener];

	pr_info("%s: listener=%d unregister\n", __func__, nb->priority);

	ret = blocking_notifier_chain_unregister(&(pnoti[src].notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_unregister error(%d)\n",
		       __func__, ret);
	DESTROY_IFCONN_NOTIFIER_BLOCK(nb);

	return ret;
}

int ifconn_notifier_notify(ifconn_notifier_t src,
			   ifconn_notifier_t listener,
			   int noti_id,
			   int event,
			   ifconn_notifier_data_param_t param_type, void *data)
{
	int ret = 0;
	struct ifconn_notifier_template *template = NULL;
	struct notifier_block *nb = NULL;

	pr_info("%s: enter\n", __func__);

	if (src > IFCONN_NOTIFY_ALL || src < 0
	    || listener > IFCONN_NOTIFY_ALL || listener < 0) {
		pr_err("%s: dev index err\n", __func__);
		return -1;
	}
	pr_info("%s: src = %s, listener = %s, nb = 0x%p notify\n", __func__,
		IFCONN_NOTI_DEV_Print[src], IFCONN_NOTI_DEV_Print[listener],
		pnoti[src].nb[listener]);

	if (param_type == IFCONN_NOTIFY_PARAM_TEMPLATE) {
		template = (struct ifconn_notifier_template *)data;
	} else {
		template = &(pnoti[src].ifconn_template[listener]);
		template->id = (uint64_t) noti_id;
		template->up_src = template->src = (uint64_t) src;
		template->dest = (uint64_t) listener;
		template->cable_type = template->event = event;
		template->data = data;
	}

	_ifconn_show_attr(template);

	if (pnoti[src].nb[listener] == NULL)
		pnoti[src].sent[listener] = true;

	if (listener == IFCONN_NOTIFY_ALL) {
		ret = blocking_notifier_call_chain(&(pnoti[src].notifier_call_chain),
				(unsigned long)noti_id, template);
	} else {
		if (pnoti[src].nb[listener] == NULL) {
			if (param_type == IFCONN_NOTIFY_PARAM_TEMPLATE) {
				memcpy(&(pnoti[src].ifconn_template[listener]), template,
					   sizeof(struct ifconn_notifier_template));
			}
			return -1;
		}

		nb = pnoti[src].nb[listener];
		ret = nb->notifier_call(nb, (unsigned long)noti_id, template);
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

static int __init ifconn_notifier_init(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);
	return ret;
}

device_initcall(ifconn_notifier_init);
