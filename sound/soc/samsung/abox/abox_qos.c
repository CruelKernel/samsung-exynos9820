/* sound/soc/samsung/abox/abox_qos.c
 *
 * ALSA SoC Audio Layer - Samsung Abox QoS driver
 *
 * Copyright (c) 2018 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/debugfs.h>
#include <linux/pm_runtime.h>
#include <linux/sched/clock.h>

#include "abox.h"
#include "abox_dbg.h"
#include "abox_qos.h"

static DEFINE_SPINLOCK(abox_qos_lock);
static struct workqueue_struct *abox_qos_wq;
static struct device *dev_abox;

static struct abox_qos abox_qos_aud = {
	.qos_class = ABOX_QOS_AUD,
	.name = "AUD",
};
static struct abox_qos abox_qos_mif = {
	.qos_class = ABOX_QOS_MIF,
	.name = "MIF",
};
static struct abox_qos abox_qos_int = {
	.qos_class = ABOX_QOS_INT,
	.name = "INT",
};
static struct abox_qos abox_qos_cl0 = {
	.qos_class = ABOX_QOS_CL0,
	.name = "CL0",
};
static struct abox_qos abox_qos_cl1 = {
	.qos_class = ABOX_QOS_CL1,
	.name = "CL1",
};
static struct abox_qos abox_qos_cl2 = {
	.qos_class = ABOX_QOS_CL2,
	.name = "CL2",
};

static struct abox_qos *abox_qos_array[] = {
	&abox_qos_aud,
	&abox_qos_mif,
	&abox_qos_int,
	&abox_qos_cl0,
	&abox_qos_cl1,
	&abox_qos_cl2,
};

static struct abox_qos *abox_qos_get_qos(enum abox_qos_class qos_class)
{
	switch (qos_class) {
	case ABOX_QOS_AUD:
		return &abox_qos_aud;
	case ABOX_QOS_CL0:
		return &abox_qos_cl0;
	case ABOX_QOS_CL1:
		return &abox_qos_cl1;
	case ABOX_QOS_CL2:
		return &abox_qos_cl2;
	case ABOX_QOS_INT:
		return &abox_qos_int;
	case ABOX_QOS_MIF:
		return &abox_qos_mif;
	default:
		return NULL;
	}
}

static struct abox_qos_req *abox_qos_find(struct abox_qos *qos, unsigned int id)
{
	struct abox_qos_req *req, *first;
	size_t len = ARRAY_SIZE(qos->req_array);

	for (req = first = qos->req_array; req - first < len; req++) {
		if (req->id == id)
			return req;
	}

	return NULL;
}

static unsigned int abox_qos_target(struct abox_qos *qos)
{
	struct abox_qos_req *req, *first;
	size_t len = ARRAY_SIZE(qos->req_array);
	unsigned int target = 0;

	for (req = first = qos->req_array; req - first < len; req++) {
		if (!req->id)
			break;

		if (target < req->val)
			target = req->val;
	}

	return target;
}

static void abox_qos_apply(struct abox_qos *qos)
{
	struct pm_qos_request *pm_qos = &qos->pm_qos;
	int val = abox_qos_target(qos);
	enum abox_qos_class qos_class = qos->qos_class;

	if (qos->val != val) {
		if (qos->qos_class == ABOX_QOS_AUD) {
			if (!qos->val && val) {
				pm_runtime_get(dev_abox);
			} else if (qos->val && !val) {
				pm_runtime_mark_last_busy(dev_abox);
				pm_runtime_put_autosuspend(dev_abox);
			}
		}
		qos->val = val;
		if (!pm_qos_request_active(pm_qos))
			pm_qos_add_request(pm_qos, qos_class, val);
		pm_qos_update_request(pm_qos, val);
		pr_info("applying qos(%d, %dkHz): %dkHz\n", qos_class,
				val, pm_qos_request(qos_class));
	}
}

static void abox_qos_work_func(struct work_struct *work)
{
	struct abox_qos **p_qos;
	size_t len = ARRAY_SIZE(abox_qos_array);

	for (p_qos = abox_qos_array; p_qos - abox_qos_array < len; p_qos++)
		abox_qos_apply(*p_qos);
}

static DECLARE_WORK(abox_qos_work, abox_qos_work_func);

static struct abox_qos_req *abox_qos_new(struct device *dev,
		struct abox_qos *qos, unsigned int id, unsigned int val,
		const char *name)
{
	struct abox_qos_req *req, *first;
	size_t len = ARRAY_SIZE(qos->req_array);

	lockdep_assert_held(&abox_qos_lock);

	for (req = first = qos->req_array; req - first < len; req++) {
		if (req->id == 0) {
			strlcpy(req->name, name, sizeof(req->name));
			req->val = val;
			req->updated = local_clock();
			req->id = id;
			return req;
		}
	}

	dev_err(dev, "too many qos request: %d, %#x, %u, %s\n",
			qos->qos_class, id, val, name);
	return NULL;
}

int abox_qos_get_target(struct device *dev, enum abox_qos_class qos_class)
{
	struct abox_qos *qos = abox_qos_get_qos(qos_class);
	int ret;

	if (!qos) {
		dev_err(dev, "%s: invalid class: %d\n", __func__, qos_class);
		return -EINVAL;
	}

	ret = abox_qos_target(qos);
	dev_dbg(dev, "%s(%d): %d\n", __func__, qos_class, ret);

	return ret;
}

unsigned int abox_qos_get_value(struct device *dev,
		enum abox_qos_class qos_class, unsigned int id)
{
	struct abox_qos *qos = abox_qos_get_qos(qos_class);
	struct abox_qos_req *req;
	int ret;

	if (!qos) {
		dev_err(dev, "%s: invalid class: %d\n", __func__, qos_class);
		return -EINVAL;
	}

	req = abox_qos_find(qos, id);
	ret = req ? req->val : 0;

	dev_dbg(dev, "%s(%d, %#x): %d\n", __func__, qos_class, id, ret);

	return ret;
}

int abox_qos_is_active(struct device *dev, enum abox_qos_class qos_class,
		unsigned int id)
{
	struct abox_qos *qos = abox_qos_get_qos(qos_class);
	struct abox_qos_req *req;
	int ret;

	if (!qos) {
		dev_err(dev, "%s: invalid class: %d\n", __func__, qos_class);
		return -EINVAL;
	}

	req = abox_qos_find(qos, id);
	ret = (req && req->val);

	dev_dbg(dev, "%s(%d, %#x): %d\n", __func__, qos_class, id, ret);

	return ret;
}

void abox_qos_complete(void)
{
	flush_work(&abox_qos_work);
}

int abox_qos_request(struct device *dev, enum abox_qos_class qos_class,
		unsigned int id, unsigned int val, const char *name)
{
	static const unsigned int DEFAULT_ID = 0xAB0CDEFA;
	struct abox_qos *qos = abox_qos_get_qos(qos_class);
	struct abox_qos_req *req;
	unsigned long flags;
	int ret = 0;

	if (!qos) {
		dev_err(dev, "%s: invalid class: %d\n", __func__, qos_class);
		return -EINVAL;
	}

	if (id == 0)
		id = DEFAULT_ID;

	spin_lock_irqsave(&abox_qos_lock, flags);
	req = abox_qos_find(qos, id);
	if (!req) {
		req = abox_qos_new(dev, qos, id, val, name);
		if (!req)
			ret = -ENOMEM;
	} else if (req->val != val) {
		strlcpy(req->name, name, sizeof(req->name));
		req->val = val;
		req->updated = local_clock();
	} else {
		req = NULL;
	}
	spin_unlock_irqrestore(&abox_qos_lock, flags);

	if (req)
		queue_work(abox_qos_wq, &abox_qos_work);

	return ret;
}

void abox_qos_clear(struct device *dev, enum abox_qos_class qos_class)
{
	struct abox_qos *qos = abox_qos_get_qos(qos_class);
	struct abox_qos_req *req, *first;
	size_t len = ARRAY_SIZE(qos->req_array);
	unsigned long flags;
	bool dirty = false;

	if (!qos) {
		dev_err(dev, "%s: invalid class: %d\n", __func__, qos_class);
		return;
	}

	spin_lock_irqsave(&abox_qos_lock, flags);
	for (req = first = qos->req_array; req - first < len; req++) {
		if (req->val) {
			req->val = 0;
			strlcpy(req->name, "clear", sizeof(req->name));
			req->updated = local_clock();
			dev_info(dev, "clearing qos: %d, %#x\n",
					qos_class, req->id);
			dirty = true;
		}
	}
	spin_unlock_irqrestore(&abox_qos_lock, flags);

	if (dirty)
		queue_work(abox_qos_wq, &abox_qos_work);
}

void abox_qos_print(struct device *dev, enum abox_qos_class qos_class)
{
	struct abox_qos *qos = abox_qos_get_qos(qos_class);
	struct abox_qos_req *req, *first;
	size_t len = ARRAY_SIZE(qos->req_array);
	unsigned long flags;

	if (!qos) {
		dev_err(dev, "%s: invalid class: %d\n", __func__, qos_class);
		return;
	}

	spin_lock_irqsave(&abox_qos_lock, flags);
	for (req = first = qos->req_array; req - first < len; req++) {
		if (req->val) {
			dev_warn(dev, "qos: %d, %#x\n", qos_class, req->id);
#ifdef CONFIG_SND_SOC_SAMSUNG_AUDIO
			sec_audio_pmlog(3, dev, "qos: %d, %#x\n", qos_class, req->id);
#endif
		}
	}
	spin_unlock_irqrestore(&abox_qos_lock, flags);
}

static ssize_t abox_qos_read_qos(char *buf, size_t size, struct abox_qos *qos)
{
	struct abox_qos_req *req, *first;
	size_t len = ARRAY_SIZE(qos->req_array);
	ssize_t offset = 0;

	offset += snprintf(buf + offset, size - offset,
			"name=%s, requested=%u, value=%u\n",
			qos->name, qos->val, pm_qos_request(qos->qos_class));

	for (req = first = qos->req_array; req - first < len; req++) {
		if (req->id) {
			unsigned long long time = req->updated;
			unsigned long rem = do_div(time, NSEC_PER_SEC);

			offset += snprintf(buf + offset, size - offset,
					"%#X\t%u\t%s updated at %llu.%09lus\n",
					req->id, req->val, req->name,
					time, rem);
		}
	}
	offset += snprintf(buf + offset, size - offset, "\n");

	return offset;
}

#ifdef CONFIG_SND_SOC_SAMSUNG_AUDIO
ssize_t abox_qos_read_file(struct file *file, char __user *user_buf,
				    size_t count, loff_t *ppos)
#else
static ssize_t abox_qos_read_file(struct file *file, char __user *user_buf,
				    size_t count, loff_t *ppos)
#endif
{
	struct abox_qos **p_qos;
	const size_t size = PAGE_SIZE, len = ARRAY_SIZE(abox_qos_array);
	char *buf = kmalloc(size, GFP_KERNEL);
	ssize_t ret = 0;

	if (!buf)
		return -ENOMEM;

	for (p_qos = abox_qos_array; p_qos - abox_qos_array < len; p_qos++)
		ret += abox_qos_read_qos(buf + ret, size - ret, *p_qos);

	if (ret >= 0)
		ret = simple_read_from_buffer(user_buf, count, ppos, buf, ret);

	kfree(buf);

	return ret;
}

static const struct file_operations abox_qos_fops = {
	.open = simple_open,
	.read = abox_qos_read_file,
	.llseek = default_llseek,
};

void abox_qos_init(struct device *adev)
{
	dev_abox = adev;
	abox_qos_wq = alloc_ordered_workqueue("abox_qos",
			WQ_FREEZABLE | WQ_MEM_RECLAIM);
	debugfs_create_file("qos", 0660, abox_dbg_get_root_dir(), NULL,
			&abox_qos_fops);
}
