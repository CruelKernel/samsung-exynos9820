/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/pm_qos.h>
#include <linux/pm_opp.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_platform.h>

#include "npu-vs4l.h"
#include "npu-device.h"
#include "npu-memory.h"
#include "npu-system.h"
#include "npu-qos.h"


#ifdef CONFIG_ARCH_EXYNOS
#if defined(CONFIG_SOC_EXYNOS9820)
#define NPU_PM_QOS_NPU		(PM_QOS_NPU_THROUGHPUT)
#define NPU_PM_QOS_DEFAULT_FREQ (666000)
#define NPU_PM_NPU_MIN		(222000)
#define NPU_PM_NPU_MAX		(933000)


#define NPU_PM_QOS_MIF		(PM_QOS_BUS_THROUGHPUT)
#define NPU_PM_MIF_MIN		(421000)
#define NPU_PM_MIF_REQ		(1352000)
#define NPU_PM_MIF_MAX		(2093000)

#define NPU_PM_QOS_INT		(PM_QOS_DEVICE_THROUGHPUT)
#define NPU_PM_INT_MIN		(100000)
#define NPU_PM_INT_MAX		(534000)

#define NPU_PM_CPU_CL0		(PM_QOS_CLUSTER0_FREQ_MIN)
#define NPU_PM_CPU_CL0_MIN	(442000)
#define NPU_PM_CPU_CL0_MAX	(1742000)

#define NPU_PM_CPU_CL1		(PM_QOS_CLUSTER1_FREQ_MIN)
#define NPU_PM_CPU_CL1_MIN	(507000)
#define NPU_PM_CPU_CL1_MAX	(1898000)

#define NPU_PM_CPU_CL2		(PM_QOS_CLUSTER2_FREQ_MIN)
#define NPU_PM_CPU_CL2_MIN	(520000)
#define NPU_PM_CPU_CL2_MAX	(2470000)

#define NPU_PM_CPU_ONLINE_MIN	0
#define NPU_PM_CPU_ONLINE_MAX	6


#else
#undef NPU_PM_QOS_NPU
#define NPU_PM_NPU_MIN		(167000)
#endif
#endif

static struct npu_qos_setting *qos_setting;
static LIST_HEAD(qos_list);
static int npu_sysfs_print(char *buf, size_t buf_sz, const char *fmt, ...)
{
	int	strlen;
	va_list	ap;

	va_start(ap, fmt);
	strlen = vsnprintf(buf, buf_sz, fmt, ap);
	va_end(ap);

	return strlen;
}

#define NPU_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size, format, ...)	\
	do {									\
		tmp_size = npu_sysfs_print(tmp_buf, sizeof(tmp_buf),		\
						format, ## __VA_ARGS__);	\
		if ((buf_size + tmp_size + 1) > PAGE_SIZE)			\
			goto exit;	/* no space */				\
		strncat(buf, tmp_buf, tmp_size);				\
		buf_size += tmp_size;						\
	} while (0)


static ssize_t npu_sysfs_show_qos(struct device *dev,
		struct device_attribute *dev_attr, char *buf)
{
	struct device *dvfs_dev;
	struct dev_pm_opp *opp;
	ssize_t		buf_size = 0;
	char		tmp_buf[256];
	int		tmp_size;
	unsigned long	freq = 0;

	buf[0] = 0;

	if (!qos_setting) {
		dev_warn(dev, "No qos_setting in device.\n");
		goto exit;
	}

	dvfs_dev = &qos_setting->dvfs_npu_dev->dev;

	NPU_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
			"# DVFS table:\n  ");
	do {
		opp = dev_pm_opp_find_freq_ceil(dvfs_dev, &freq);
		if (IS_ERR(opp))
			break;

		NPU_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
				"%lu%s%s ", freq,
				(freq == qos_setting->current_freq) ?
				"(*)" : "",
				(freq == qos_setting->request_freq) ?
				"(R)" : "");
		freq++;
		dev_pm_opp_put(opp);
	} while (1);

	NPU_SYSFS_PRINT(buf, buf_size, tmp_buf, tmp_size,
			"\n\n# default: %d KHz / request: %d KHz / current: %d KHz\n\n",
			qos_setting->default_freq,
			qos_setting->request_freq, qos_setting->current_freq);

exit:
	return buf_size;
}

static ssize_t npu_sysfs_set_qos(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct device *dvfs_dev;
	struct dev_pm_opp *opp;
	unsigned long freq = 0;
	bool hit = false;
	int  value, err;

	err = kstrtoint(buf, 10, &value);
	if (err) {
		dev_err(dev, "only decimal value allowed (input: %s)\n", buf);
		return count;
	}

	if (!qos_setting->dvfs_npu_dev) {
		dev_warn(dev, "npu drivers does not connect with dvfs table\n");
		return count;
	}

	dvfs_dev = &qos_setting->dvfs_npu_dev->dev;

	do {
		opp = dev_pm_opp_find_freq_ceil(dvfs_dev, &freq);
		if (IS_ERR(opp))
			break;

		if (freq == (int)value) {
			hit = true;
			dev_pm_opp_put(opp);
			break;
		}
		dev_pm_opp_put(opp);
		freq++;

	} while (1);

	if (hit)
		qos_setting->request_freq = (s32)value;
	else
		dev_warn(dev, "fail to set qos_rate (req %d KHz) and"
			"(current %d KHz)\n", value, qos_setting->current_freq);

	return count;
}

static DEVICE_ATTR(qos_rate, 0644, npu_sysfs_show_qos, npu_sysfs_set_qos);
static struct attribute *npu_qos_attributes[] = {
	&dev_attr_qos_rate.attr,
	NULL,
};

static struct attribute_group npu_attr_group = {
	.attrs = npu_qos_attributes,
};

int npu_qos_probe(struct npu_system *system)
{
	struct device *dev = &system->pdev->dev;
	struct device_node *of_node = dev->of_node;
	struct device_node *child_node;

	qos_setting = &(system->qos_setting);

	mutex_init(&qos_setting->npu_qos_lock);

	/* parse dts and fill data */
	child_node = of_parse_phandle(of_node, "dvfs-dev", 0);
	qos_setting->dvfs_npu_dev = of_find_device_by_node(child_node);
	if (!qos_setting->dvfs_npu_dev)
		dev_warn(dev, "fail to get npu dvfs_dev\n");

	if (of_property_read_s32(of_node, "npu_default_qos_rate",
				&qos_setting->default_freq))
		qos_setting->default_freq = NPU_PM_NPU_MIN;

	qos_setting->request_freq = qos_setting->default_freq;

	/* qos add request(default_freq) */
#ifdef NPU_PM_QOS_NPU
	pm_qos_add_request(&qos_setting->npu_qos_req_npu, NPU_PM_QOS_NPU, 0);
	pm_qos_add_request(&qos_setting->npu_qos_req_mif, NPU_PM_QOS_MIF, 0);
	pm_qos_add_request(&qos_setting->npu_qos_req_int, NPU_PM_QOS_INT, 0);

	pm_qos_add_request(&qos_setting->npu_qos_req_cpu_cl0, NPU_PM_CPU_CL0, 0);
	pm_qos_add_request(&qos_setting->npu_qos_req_cpu_cl1, NPU_PM_CPU_CL1, 0);
	pm_qos_add_request(&qos_setting->npu_qos_req_cpu_cl2, NPU_PM_CPU_CL2, 0);

	qos_setting->current_freq = qos_setting->default_freq;
#else
	qos_setting->default_freq = NPU_PM_NPU_MIN;
	qos_setting->current_freq = NPU_PM_NPU_MIN;
#endif
	qos_setting->req_cl0_freq = 0;
	qos_setting->req_cl1_freq = 0;
	qos_setting->req_cl2_freq = 0;

	return sysfs_create_group(&dev->kobj, &npu_attr_group);
}

int npu_qos_release(struct npu_system *system)
{
	struct device *dev = &system->pdev->dev;

	sysfs_remove_group(&dev->kobj, &npu_attr_group);

	return 0;
}

int npu_qos_start(struct npu_system *system)
{
	int cur_value;

	BUG_ON(!system);

	mutex_lock(&qos_setting->npu_qos_lock);

	if (qos_setting->current_freq != qos_setting->request_freq)
		qos_setting->current_freq = qos_setting->request_freq;
#ifdef NPU_PM_QOS_NPU
	if (qos_setting->req_npu_freq <= qos_setting->current_freq)
		pm_qos_update_request(&qos_setting->npu_qos_req_npu, qos_setting->current_freq);
#endif
	mutex_unlock(&qos_setting->npu_qos_lock);
	cur_value = (s32)pm_qos_read_req_value(qos_setting->npu_qos_req_npu.pm_qos_class,
		&qos_setting->npu_qos_req_npu);
	npu_info("%s() npu_qos_rate(%d)\n", __func__, cur_value);
	qos_setting->req_cl0_freq = 0;
	qos_setting->req_cl1_freq = 0;
	qos_setting->req_cl2_freq = 0;
	return 0;
}

int npu_qos_stop(struct npu_system *system)
{
	struct list_head *pos, *q;
	struct npu_session_qos_req *qr;
	int cur_value;

	BUG_ON(!system);

	mutex_lock(&qos_setting->npu_qos_lock);

	list_for_each_safe(pos, q, &qos_list) {
		qr = list_entry(pos, struct npu_session_qos_req, list);
		list_del(pos);
		if (qr)
			kfree(qr);
	}
	list_del_init(&qos_list);

#ifdef NPU_PM_QOS_NPU
	pm_qos_update_request(&qos_setting->npu_qos_req_npu, NPU_PM_NPU_MIN);
	pm_qos_update_request(&qos_setting->npu_qos_req_mif, 0);
	pm_qos_update_request(&qos_setting->npu_qos_req_int, 0);

	pm_qos_update_request(&qos_setting->npu_qos_req_cpu_cl0, 0);
	pm_qos_update_request(&qos_setting->npu_qos_req_cpu_cl1, 0);
	pm_qos_update_request(&qos_setting->npu_qos_req_cpu_cl2, 0);
#endif
	qos_setting->req_npu_freq = 0;
	mutex_unlock(&qos_setting->npu_qos_lock);
	cur_value = (s32)pm_qos_read_req_value(qos_setting->npu_qos_req_npu.pm_qos_class,
		&qos_setting->npu_qos_req_npu);
	npu_info("%s() npu_qos_rate(%d)\n", __func__, cur_value);

	return 0;
}


s32 __update_freq_from_showcase(__u32 nCategory)
{
	s32 nValue = 0;
	struct list_head *pos, *q;
	struct npu_session_qos_req *qr;

	list_for_each_safe(pos, q, &qos_list) {
		qr = list_entry(pos, struct npu_session_qos_req, list);
		if (qr->eCategory == nCategory) {
			nValue = qr->req_freq > nValue ? qr->req_freq : nValue;
			npu_dbg("[U%u]Candidate Freq. category : %u  freq : %d\n",
				qr->sessionUID, nCategory, nValue);
		}
	}

	return nValue;
}

int __req_param_qos(int uid, __u32 nCategory, struct pm_qos_request *req, s32 new_value)
{
	int ret = 0;
	s32 cur_value, rec_value;
	struct list_head *pos, *q;
	struct npu_session_qos_req *qr;

	//Check that same uid, and category whether already registered.
	list_for_each_safe(pos, q, &qos_list) {
		qr = list_entry(pos, struct npu_session_qos_req, list);
		if ((qr->sessionUID == uid) && (qr->eCategory == nCategory)) {
			cur_value = qr->req_freq;
			npu_dbg("[U%u]Change Req Freq. category : %u, from freq : %d to %d\n",
					uid, nCategory, cur_value, new_value);
			list_del(pos);
			qr->sessionUID = uid;
			qr->req_freq = new_value;
			qr->eCategory = nCategory;
			list_add_tail(&qr->list, &qos_list);
			rec_value = __update_freq_from_showcase(nCategory);

			if (new_value > rec_value) {
				pm_qos_update_request(req, new_value);
				npu_dbg("[U%u]Changed Freq. category : %u, from freq : %d to %d\n",
					uid, nCategory, cur_value, new_value);
			} else {
				pm_qos_update_request(req, rec_value);
				npu_dbg("[U%u]Recovered Freq. category : %u, from freq : %d to %d\n",
					uid, nCategory, cur_value, rec_value);
			}
			return ret;
		}
	}
	//No Same uid, and category. Add new item
	qr = kmalloc(sizeof(struct npu_session_qos_req), GFP_KERNEL);
	if (!qr) {
		npu_err("memory alloc fail.\n");
		return -ENOMEM;
	}
	qr->sessionUID = uid;
	qr->req_freq = new_value;
	qr->eCategory = nCategory;
	list_add_tail(&qr->list, &qos_list);

	//If new_value is lager than current value, update the freq
	cur_value = (s32)pm_qos_read_req_value(req->pm_qos_class, req);
	npu_dbg("[U%u]New Freq. category : %u freq : %u\n",
		qr->sessionUID, qr->eCategory, qr->req_freq);
	if (cur_value < new_value) {
		npu_dbg("[U%u]Update Freq. category : %u freq : %u\n",
			qr->sessionUID, qr->eCategory, qr->req_freq);
		pm_qos_update_request(req, new_value);
	}
	return ret;
}

npu_s_param_ret npu_qos_param_handler(struct npu_session *sess, struct vs4l_param *param, int *retval)
{
	BUG_ON(!sess);
	BUG_ON(!param);

	mutex_lock(&qos_setting->npu_qos_lock);

	switch (param->target) {
	case NPU_S_PARAM_QOS_NPU:
		qos_setting->req_npu_freq = param->offset;
		//Set minimum safe clock as default  to avoid NDONE
		if (qos_setting->req_npu_freq < qos_setting->default_freq)
			qos_setting->req_npu_freq = qos_setting->default_freq;
		__req_param_qos(sess->uid, param->target, &qos_setting->npu_qos_req_npu, qos_setting->req_npu_freq);

		mutex_unlock(&qos_setting->npu_qos_lock);
		return S_PARAM_HANDLED;
	case NPU_S_PARAM_QOS_MIF:
		qos_setting->req_mif_freq = param->offset;
		__req_param_qos(sess->uid, param->target, &qos_setting->npu_qos_req_mif, qos_setting->req_mif_freq);

		mutex_unlock(&qos_setting->npu_qos_lock);
		return S_PARAM_HANDLED;
	case NPU_S_PARAM_QOS_INT:
		qos_setting->req_int_freq = param->offset;
		__req_param_qos(sess->uid, param->target, &qos_setting->npu_qos_req_int, qos_setting->req_int_freq);

		mutex_unlock(&qos_setting->npu_qos_lock);
		return S_PARAM_HANDLED;
	case NPU_S_PARAM_QOS_CL0:
		qos_setting->req_cl0_freq = param->offset;
		__req_param_qos(sess->uid, param->target, &qos_setting->npu_qos_req_cpu_cl0, qos_setting->req_cl0_freq);

		mutex_unlock(&qos_setting->npu_qos_lock);
		return S_PARAM_HANDLED;
	case NPU_S_PARAM_QOS_CL1:
		qos_setting->req_cl1_freq = param->offset;
		__req_param_qos(sess->uid, param->target, &qos_setting->npu_qos_req_cpu_cl1, qos_setting->req_cl1_freq);

		mutex_unlock(&qos_setting->npu_qos_lock);
		return S_PARAM_HANDLED;
	case NPU_S_PARAM_QOS_CL2:
		qos_setting->req_cl2_freq = param->offset;
		__req_param_qos(sess->uid, param->target, &qos_setting->npu_qos_req_cpu_cl2, qos_setting->req_cl2_freq);

		mutex_unlock(&qos_setting->npu_qos_lock);
		return S_PARAM_HANDLED;
	case NPU_S_PARAM_CPU_AFF:
	case NPU_S_PARAM_QOS_RST:
	default:
		mutex_unlock(&qos_setting->npu_qos_lock);
		return S_PARAM_NOMB;
	}
}
