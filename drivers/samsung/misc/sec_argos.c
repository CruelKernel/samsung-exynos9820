/*
 * argos.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/pm_qos.h>
#include <linux/reboot.h>
#include <linux/of.h>
#include <linux/gfp.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/cpumask.h>
#include <linux/interrupt.h>
#include <linux/sec_argos.h>
#include <linux/ologk.h>

#if defined(CONFIG_SCHED_EMS)
#include <linux/ems.h>
static struct gb_qos_request gb_req = {
	.name = "argos_global_boost",
};
#endif

#define ARGOS_NAME "argos"
#define TYPE_SHIFT 4
#define TYPE_MASK_BIT ((1 << TYPE_SHIFT) - 1)

static DEFINE_SPINLOCK(argos_irq_lock);
static DEFINE_SPINLOCK(argos_task_lock);

enum {
	THRESHOLD,
	BIG_MIN_FREQ,
	BIG_MAX_FREQ,
	MID_MIN_FREQ,
	MID_MAX_FREQ,
	LIT_MIN_FREQ,
	LIT_MAX_FREQ,
	MIFFREQ,
	INTFREQ,
	TASK_AFFINITY_EN,
	IRQ_AFFINITY_EN,
	HMP_BOOST_EN,
	ITEM_MAX,
};

struct boost_table {
	unsigned int items[ITEM_MAX];
};

struct argos_task_affinity {
	struct task_struct *p;
	struct cpumask *affinity_cpu_mask;
	struct cpumask *default_cpu_mask;
	struct list_head entry;
};

struct argos_irq_affinity {
	unsigned int irq;
	struct cpumask *affinity_cpu_mask;
	struct cpumask *default_cpu_mask;
	struct list_head entry;
};

struct argos_pm_qos {
	struct pm_qos_request big_min_qos_req;
	struct pm_qos_request big_max_qos_req;
	struct pm_qos_request mid_min_qos_req;
	struct pm_qos_request mid_max_qos_req;
	struct pm_qos_request lit_min_qos_req;
	struct pm_qos_request lit_max_qos_req;
	struct pm_qos_request mif_qos_req;
	struct pm_qos_request int_qos_req;
	struct pm_qos_request hotplug_min_qos_req;
};

struct argos {
	const char *desc;
	struct platform_device *pdev;
	struct boost_table *tables;
	int ntables;
	int prev_level;
	struct argos_pm_qos *qos;
	struct list_head task_affinity_list;
	bool task_hotplug_disable;
	struct list_head irq_affinity_list;
	bool irq_hotplug_disable;
	bool hmpboost_enable;
	bool argos_block;
	struct blocking_notifier_head argos_notifier;
	/* protect prev_level, qos, task/irq_hotplug_disable, hmpboost_enable */
	struct mutex level_mutex;
};

struct argos_platform_data {
	struct argos *devices;
	int ndevice;
	struct notifier_block pm_qos_nfb;
};

static struct argos_platform_data *argos_pdata;

static inline void UPDATE_PM_QOS(struct pm_qos_request *req, int class_id, int arg)
{
	if (arg) {
		if (pm_qos_request_active(req))
			pm_qos_update_request(req, arg);
		else
			pm_qos_add_request(req, class_id, arg);
	}
}

static inline void REMOVE_PM_QOS(struct pm_qos_request *req)
{
	if (pm_qos_request_active(req))
		pm_qos_remove_request(req);
}

static int argos_find_index(const char *label)
{
	int i;
	int dev_num = -1;

	if (!argos_pdata) {
		pr_err("%s argos not initialized\n", __func__);
		return -1;
	}

	for (i = 0; i < argos_pdata->ndevice; i++)
		if (strcmp(argos_pdata->devices[i].desc, label) == 0)
			dev_num = i;
	return dev_num;
}

int sec_argos_register_notifier(struct notifier_block *n, char *label)
{
	struct blocking_notifier_head *cnotifier;
	int dev_num;

	dev_num = argos_find_index(label);

	if (dev_num < 0) {
		pr_err("%s: No match found for label: %d", __func__, dev_num);
		return -ENODEV;
	}

	cnotifier = &argos_pdata->devices[dev_num].argos_notifier;

	if (!cnotifier) {
		pr_err("%s argos notifier not found(dev_num:%d)\n", __func__, dev_num);
		return -ENXIO;
	}

	pr_info("%s: %pf(dev_num:%d)\n", __func__, n->notifier_call, dev_num);

	return blocking_notifier_chain_register(cnotifier, n);
}
EXPORT_SYMBOL(sec_argos_register_notifier);

int sec_argos_unregister_notifier(struct notifier_block *n, char *label)
{
	struct blocking_notifier_head *cnotifier;
	int dev_num;

	dev_num = argos_find_index(label);

	if (dev_num < 0) {
		pr_err("%s: No match found for label: %d", __func__, dev_num);
		return -ENODEV;
	}

	cnotifier = &argos_pdata->devices[dev_num].argos_notifier;

	if (!cnotifier) {
		pr_err("%s argos notifier not found(dev_num:%d)\n", __func__, dev_num);
		return -ENXIO;
	}

	pr_info("%s: %pf(dev_num:%d)\n", __func__, n->notifier_call, dev_num);

	return blocking_notifier_chain_unregister(cnotifier, n);
}
EXPORT_SYMBOL(sec_argos_unregister_notifier);

static int argos_task_affinity_setup(struct task_struct *p, int dev_num,
				     struct cpumask *affinity_cpu_mask,
				     struct cpumask *default_cpu_mask)
{
	struct argos_task_affinity *this;
	struct list_head *head;

	if (!argos_pdata) {
		pr_err("%s argos not initialized\n", __func__);
		return -ENXIO;
	}

	if (dev_num < 0 || dev_num >= argos_pdata->ndevice) {
		pr_err("%s dev_num:%d should be dev_num:0 ~ %d in boundary\n",
		       __func__, dev_num, argos_pdata->ndevice - 1);
		return -EINVAL;
	}

	head = &argos_pdata->devices[dev_num].task_affinity_list;

	this = kzalloc(sizeof(*this), GFP_ATOMIC);
	if (!this)
		return -ENOMEM;

	this->p = p;
	this->affinity_cpu_mask = affinity_cpu_mask;
	this->default_cpu_mask = default_cpu_mask;

	spin_lock(&argos_task_lock);
	list_add(&this->entry, head);
	spin_unlock(&argos_task_lock);

	return 0;
}

int argos_task_affinity_setup_label(struct task_struct *p, const char *label,
				    struct cpumask *affinity_cpu_mask,
				    struct cpumask *default_cpu_mask)
{
	int dev_num;

	dev_num = argos_find_index(label);

	return argos_task_affinity_setup(p, dev_num, affinity_cpu_mask,
					 default_cpu_mask);
}

static int argos_irq_affinity_setup(unsigned int irq, int dev_num,
				    struct cpumask *affinity_cpu_mask,
				    struct cpumask *default_cpu_mask)
{
	struct argos_irq_affinity *this;
	struct list_head *head;

	if (!argos_pdata) {
		pr_err("%s argos not initialized\n", __func__);
		return -ENXIO;
	}

	if (dev_num < 0 || dev_num >= argos_pdata->ndevice) {
		pr_err("%s dev_num:%d should be dev_num:0 ~ %d in boundary\n",
		       __func__, dev_num, argos_pdata->ndevice - 1);
		return -EINVAL;
	}

	head = &argos_pdata->devices[dev_num].irq_affinity_list;

	this = kzalloc(sizeof(*this), GFP_ATOMIC);
	if (!this)
		return -ENOMEM;

	this->irq = irq;
	this->affinity_cpu_mask = affinity_cpu_mask;
	this->default_cpu_mask = default_cpu_mask;

	spin_lock(&argos_irq_lock);
	list_add(&this->entry, head);
	spin_unlock(&argos_irq_lock);

	return 0;
}

int argos_irq_affinity_setup_label(unsigned int irq, const char *label,
				   struct cpumask *affinity_cpu_mask,
				   struct cpumask *default_cpu_mask)
{
	int dev_num;

	dev_num = argos_find_index(label);

	return argos_irq_affinity_setup(irq, dev_num, affinity_cpu_mask,
			default_cpu_mask);
}

int argos_task_affinity_apply(int dev_num, bool enable)
{
	struct argos_task_affinity *this;
	struct list_head *head;
	int result = 0;
	struct cpumask *mask;
	bool *hotplug_disable;
	struct pm_qos_request *hotplug_min_qos_req;

	head = &argos_pdata->devices[dev_num].task_affinity_list;
	hotplug_disable = &argos_pdata->devices[dev_num].task_hotplug_disable;
	hotplug_min_qos_req =
		&argos_pdata->devices[dev_num].qos->hotplug_min_qos_req;

	if (list_empty(head)) {
		pr_debug("%s: task_affinity_list is empty\n", __func__);
		return result;
	}

	list_for_each_entry(this, head, entry) {
		if (enable) {
			if (!*hotplug_disable) {
				UPDATE_PM_QOS(hotplug_min_qos_req,
					      PM_QOS_CPU_ONLINE_MIN, num_possible_cpus());
				*hotplug_disable = true;
			}
			mask = this->affinity_cpu_mask;
		} else {
			if (*hotplug_disable) {
				REMOVE_PM_QOS(hotplug_min_qos_req);
				*hotplug_disable = false;
			}
			mask = this->default_cpu_mask;
		}

		result = set_cpus_allowed_ptr(this->p, mask);

		pr_info("%s: %s affinity %s to cpu_mask:0x%X\n",
			__func__, this->p->comm,
			(enable ? "enable" : "disable"),
			(int)*mask->bits);
	}

	return result;
}

int argos_irq_affinity_apply(int dev_num, bool enable)
{
	struct argos_irq_affinity *this;
	struct list_head *head;
	int result = 0;
	struct cpumask *mask;
	bool *hotplug_disable;
	struct pm_qos_request *hotplug_min_qos_req;

	head = &argos_pdata->devices[dev_num].irq_affinity_list;
	hotplug_disable = &argos_pdata->devices[dev_num].irq_hotplug_disable;
	hotplug_min_qos_req =
		&argos_pdata->devices[dev_num].qos->hotplug_min_qos_req;

	if (list_empty(head)) {
		pr_debug("%s: irq_affinity_list is empty\n", __func__);
		return result;
	}

	list_for_each_entry(this, head, entry) {
		if (enable) {
			if (!*hotplug_disable) {
				UPDATE_PM_QOS(hotplug_min_qos_req,
					      PM_QOS_CPU_ONLINE_MIN, num_possible_cpus());
				*hotplug_disable = true;
			}
			mask = this->affinity_cpu_mask;
		} else {
			if (*hotplug_disable) {
				REMOVE_PM_QOS(hotplug_min_qos_req);
				*hotplug_disable = false;
			}
			mask = this->default_cpu_mask;
		}

		result = irq_set_affinity(this->irq, mask);

		pr_info("%s: irq%d affinity %s to cpu_mask:0x%X\n",
			__func__, this->irq, (enable ? "enable" : "disable"),
			(int)*mask->bits);
	}

	return result;
}

int argos_hmpboost_apply(int dev_num, bool enable)
{
	bool *hmpboost_enable;

	hmpboost_enable = &argos_pdata->devices[dev_num].hmpboost_enable;

	if (enable) {
		/* disable -> enable */
		if (!*hmpboost_enable) {
#if defined(CONFIG_SCHED_EMS)
			/* set global boost */
			gb_qos_update_request(&gb_req, 100);
#endif
			*hmpboost_enable = true;
			pr_info("%s: hmp boost enable [%d]\n", __func__, dev_num);
		}
	} else {
		/* enable -> disable */
		if (*hmpboost_enable) {
#if defined(CONFIG_SCHED_EMS)
			/* unset global boost */
			gb_qos_update_request(&gb_req, 0);
#endif
			*hmpboost_enable = false;
			pr_info("%s: hmp boost disable [%d]\n", __func__, dev_num);
		}
	}

	return 0;
}

static void argos_freq_unlock(int type)
{
	struct argos_pm_qos *qos = argos_pdata->devices[type].qos;
	const char *cname;

	cname = argos_pdata->devices[type].desc;

	REMOVE_PM_QOS(&qos->big_min_qos_req);
	REMOVE_PM_QOS(&qos->big_max_qos_req);
	REMOVE_PM_QOS(&qos->mid_min_qos_req);
	REMOVE_PM_QOS(&qos->mid_max_qos_req);
	REMOVE_PM_QOS(&qos->lit_min_qos_req);
	REMOVE_PM_QOS(&qos->lit_max_qos_req);
	REMOVE_PM_QOS(&qos->mif_qos_req);
	REMOVE_PM_QOS(&qos->int_qos_req);

	pr_info("%s name:%s\n", __func__, cname);
}

static void argos_freq_lock(int type, int level)
{
	unsigned int big_min_freq, big_max_freq;
	unsigned int mid_min_freq, mid_max_freq;
	unsigned int lit_min_freq, lit_max_freq;
	unsigned int mif_freq, int_freq;
	struct boost_table *t = &argos_pdata->devices[type].tables[level];
	struct argos_pm_qos *qos = argos_pdata->devices[type].qos;
	const char *cname;

	cname = argos_pdata->devices[type].desc;

	big_min_freq = t->items[BIG_MIN_FREQ];
	big_max_freq = t->items[BIG_MAX_FREQ];
	mid_min_freq = t->items[MID_MIN_FREQ];
	mid_max_freq = t->items[MID_MAX_FREQ];
	lit_min_freq = t->items[LIT_MIN_FREQ];
	lit_max_freq = t->items[LIT_MAX_FREQ];
	mif_freq = t->items[MIFFREQ];
	int_freq = t->items[INTFREQ];

	if (big_min_freq) {
		UPDATE_PM_QOS(&qos->big_min_qos_req,
			      PM_QOS_CLUSTER2_FREQ_MIN, big_min_freq);
	} else {
		REMOVE_PM_QOS(&qos->big_min_qos_req);
	}

	if (big_max_freq) {
		UPDATE_PM_QOS(&qos->big_max_qos_req,
			      PM_QOS_CLUSTER2_FREQ_MAX, big_max_freq);
	} else {
		REMOVE_PM_QOS(&qos->big_max_qos_req);
	}

	if (mid_min_freq) {
		UPDATE_PM_QOS(&qos->mid_min_qos_req,
			      PM_QOS_CLUSTER1_FREQ_MIN, mid_min_freq);
	} else {
		REMOVE_PM_QOS(&qos->mid_min_qos_req);
	}

	if (mid_max_freq) {
		UPDATE_PM_QOS(&qos->mid_max_qos_req,
			      PM_QOS_CLUSTER1_FREQ_MAX, mid_max_freq);
	} else {
		REMOVE_PM_QOS(&qos->mid_max_qos_req);
	}

	if (lit_min_freq) {
		UPDATE_PM_QOS(&qos->lit_min_qos_req,
			      PM_QOS_CLUSTER0_FREQ_MIN, lit_min_freq);
	} else {
		REMOVE_PM_QOS(&qos->lit_min_qos_req);
	}

	if (lit_max_freq) {
		UPDATE_PM_QOS(&qos->lit_max_qos_req,
			      PM_QOS_CLUSTER0_FREQ_MAX, lit_max_freq);
	} else {
		REMOVE_PM_QOS(&qos->lit_max_qos_req);
	}

	if (mif_freq) {
		UPDATE_PM_QOS(&qos->mif_qos_req,
			      PM_QOS_BUS_THROUGHPUT, mif_freq);
	} else {
		REMOVE_PM_QOS(&qos->mif_qos_req);
	}

	if (int_freq) {
		UPDATE_PM_QOS(&qos->int_qos_req,
			      PM_QOS_DEVICE_THROUGHPUT, int_freq);
	} else {
		REMOVE_PM_QOS(&qos->int_qos_req);
	}

	pr_info("%s name:%s, BIG_MIN=%d, BIG_MAX=%d, MID_MIN=%d, MID_MAX=%d, LIT_MIN=%d, LIT_MAX=%d, MIF=%d, INT=%d\n",
		__func__, cname, big_min_freq, big_max_freq, mid_min_freq, mid_max_freq, lit_min_freq, lit_max_freq, mif_freq, int_freq);
}

void argos_block_enable(char *req_name, bool set)
{
	int dev_num;
	struct argos *cnode;

	dev_num = argos_find_index(req_name);

	if (dev_num < 0) {
		pr_err("%s: No match found for label: %s", __func__, req_name);
		return;
	}

	cnode = &argos_pdata->devices[dev_num];

	if (set) {
		cnode->argos_block = true;
		mutex_lock(&cnode->level_mutex);
		argos_freq_unlock(dev_num);
		argos_task_affinity_apply(dev_num, 0);
		argos_irq_affinity_apply(dev_num, 0);
		argos_hmpboost_apply(dev_num, 0);
		cnode->prev_level = -1;
		mutex_unlock(&cnode->level_mutex);
	} else {
		cnode->argos_block = false;
	}
	pr_info("%s req_name:%s block:%d\n",
		__func__, req_name, cnode->argos_block);
}

static int argos_cpuidle_reboot_notifier(struct notifier_block *this,
					 unsigned long event, void *_cmd)
{
	switch (event) {
	case SYSTEM_POWER_OFF:
	case SYS_RESTART:
		pr_info("%s called\n", __func__);
		pm_qos_remove_notifier(PM_QOS_NETWORK_THROUGHPUT,
				       &argos_pdata->pm_qos_nfb);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block argos_cpuidle_reboot_nb = {
	.notifier_call = argos_cpuidle_reboot_notifier,
};

static int argos_pm_qos_notify(struct notifier_block *nfb,
			       unsigned long speedtype, void *arg)

{
	int type, level, prev_level;
	unsigned long speed;
	bool argos_blocked;
	struct argos *cnode;

	type = (speedtype & TYPE_MASK_BIT) - 1;
	if (type < 0 || type > argos_pdata->ndevice) {
		pr_err("There is no type for devices type[%d], ndevice[%d]\n",
		       type, argos_pdata->ndevice);
		return NOTIFY_BAD;
	}

	speed = speedtype >> TYPE_SHIFT;
	cnode = &argos_pdata->devices[type];

	prev_level = cnode->prev_level;

	pr_debug("%s name:%s, speed:%ldMbps\n", __func__, cnode->desc, speed);
	if(speed >= 300) {
		perflog(PERFLOG_ARGOS, "name:%s, speed:%ldMbps", cnode->desc, speed);
	}

	argos_blocked = cnode->argos_block;

	/* Find proper level */
	for (level = 0; level < cnode->ntables; level++) {
		struct boost_table *t = &cnode->tables[level];

		if (speed < t->items[THRESHOLD]) {
			break;
		} else if (argos_pdata->devices[type].ntables == level) {
			level++;
			break;
		}
	}

	/* decrease 1 level to match proper table */
	level--;
	if (!argos_blocked) {
		if (level != prev_level) {
			if (mutex_trylock(&cnode->level_mutex) == 0) {
			/*
			 * If the mutex is already locked, it means this argos
			 * is being blocked or is handling another change.
			 * We don't need to wait.
			 */
				pr_warn("%s: skip name:%s, speed:%ldMbps, prev level:%d, request level:%d\n",
					__func__, cnode->desc, speed, prev_level, level);
				goto out;
			}
			pr_info("%s: name:%s, speed:%ldMbps, prev level:%d, request level:%d\n",
				__func__, cnode->desc, speed, prev_level, level);

			if (level == -1) {
				if (cnode->argos_notifier.head) {
					pr_debug("%s: Call argos notifier(%s lev:%d)\n",
						 __func__, cnode->desc, level);
					blocking_notifier_call_chain(&cnode->argos_notifier,
								     speed, &level);
				}
				argos_freq_unlock(type);
				argos_task_affinity_apply(type, 0);
				argos_irq_affinity_apply(type, 0);
				argos_hmpboost_apply(type, 0);
			} else {
				unsigned int enable_flag;

				argos_freq_lock(type, level);

				enable_flag = argos_pdata->devices[type].tables[level].items[TASK_AFFINITY_EN];
				argos_task_affinity_apply(type, enable_flag);
				enable_flag = argos_pdata->devices[type].tables[level].items[IRQ_AFFINITY_EN];
				argos_irq_affinity_apply(type, enable_flag);
				enable_flag =
					argos_pdata->devices[type].tables[level].items[HMP_BOOST_EN];
				argos_hmpboost_apply(type, enable_flag);

				if (cnode->argos_notifier.head) {
					pr_debug("%s: Call argos notifier(%s lev:%d)\n",
						 __func__, cnode->desc, level);
					blocking_notifier_call_chain(&cnode->argos_notifier,
								     speed, &level);
				}
			}

			cnode->prev_level = level;
			mutex_unlock(&cnode->level_mutex);
		} else {
			pr_debug("%s:same level (%d) is requested", __func__, level);
		}
	}
out:
	return NOTIFY_OK;
}

#ifdef CONFIG_OF
static int argos_parse_dt(struct device *dev)
{
	struct argos_platform_data *pdata = dev->platform_data;
	struct argos *cnode;
	struct device_node *np, *cnp;
	int device_count = 0, num_level = 0;
	int retval = 0, i, j;

	np = dev->of_node;
	pdata->ndevice = of_get_child_count(np);
	if (!pdata->ndevice) {
		dev_err(dev, "Failed to get child count\n");
		return -ENODEV;
	}

	pdata->devices = devm_kzalloc(dev, sizeof(struct argos) * pdata->ndevice, GFP_KERNEL);
	if (!pdata->devices)
		return -ENOMEM;

	for_each_child_of_node(np, cnp) {
		cnode = &pdata->devices[device_count];
		cnode->desc = of_get_property(cnp, "net_boost,label", NULL);
		if (of_property_read_u32(cnp, "net_boost,table_size", &num_level)) {
			dev_err(dev, "Failed to get table size: node not exist\n");
			retval = -EINVAL;
			goto err_out;
		}
		cnode->ntables = num_level;

		/* Allocation for freq and time table */
		if (!cnode->tables) {
			cnode->tables = devm_kzalloc(dev,
						     sizeof(struct boost_table) * cnode->ntables, GFP_KERNEL);
			if (!cnode->tables) {
				retval = -ENOMEM;
				goto err_out;
			}
		}

		/* Get and add frequency and time table */
		for (i = 0; i < num_level; i++) {
			for (j = 0; j < ITEM_MAX; j++) {
				retval = of_property_read_u32_index(cnp, "net_boost,table",
								    i * ITEM_MAX + j, &cnode->tables[i].items[j]);
				if (retval) {
					dev_err(dev, "Failed to get property\n");
					retval = -EINVAL;
					goto err_out;
				}
			}
		}

		INIT_LIST_HEAD(&cnode->task_affinity_list);
		INIT_LIST_HEAD(&cnode->irq_affinity_list);
		cnode->task_hotplug_disable = false;
		cnode->irq_hotplug_disable = false;
		cnode->hmpboost_enable = false;
		cnode->argos_block = false;
		cnode->prev_level = -1;
		mutex_init(&cnode->level_mutex);
		cnode->qos = devm_kzalloc(dev, sizeof(struct argos_pm_qos),
					  GFP_KERNEL);
		if (!cnode->qos) {
			retval = -ENOMEM;
			goto err_out;
		}
		BLOCKING_INIT_NOTIFIER_HEAD(&cnode->argos_notifier);

		device_count++;
	}

	return 0;

err_out:
	return retval;
}
#endif

static int argos_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct argos_platform_data *pdata;

	pr_info("%s: Start probe\n", __func__);
	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev,
				     sizeof(struct argos_platform_data),
				     GFP_KERNEL);

		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate platform data\n");
			return -ENOMEM;
		}

		pdev->dev.platform_data = pdata;
		ret = argos_parse_dt(&pdev->dev);
		if (ret) {
			dev_err(&pdev->dev, "Failed to parse dt data\n");
			return ret;
		}
		pr_info("%s: parse dt done\n", __func__);
	} else {
		pdata = pdev->dev.platform_data;
	}

	if (!pdata) {
		dev_err(&pdev->dev, "There are no platform data\n");
		return -EINVAL;
	}

	if (!pdata->ndevice || !pdata->devices) {
		dev_err(&pdev->dev, "There are no devices\n");
		return -EINVAL;
	}

	pdata->pm_qos_nfb.notifier_call = argos_pm_qos_notify;
	pm_qos_add_notifier(PM_QOS_NETWORK_THROUGHPUT, &pdata->pm_qos_nfb);
	register_reboot_notifier(&argos_cpuidle_reboot_nb);
	argos_pdata = pdata;
	platform_set_drvdata(pdev, pdata);

	return 0;
}

static int argos_remove(struct platform_device *pdev)
{
	struct argos_platform_data *pdata = platform_get_drvdata(pdev);

	if (!pdata || !argos_pdata)
		return 0;
	pm_qos_remove_notifier(PM_QOS_NETWORK_THROUGHPUT, &pdata->pm_qos_nfb);
	unregister_reboot_notifier(&argos_cpuidle_reboot_nb);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id argos_dt_ids[] = {
	{ .compatible = "samsung,argos"},
	{ }
};
#endif

static struct platform_driver argos_driver = {
	.driver = {
		.name = ARGOS_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table	= of_match_ptr(argos_dt_ids),
#endif
	},
	.probe = argos_probe,
	.remove = argos_remove
};

static int __init argos_init(void)
{
	return platform_driver_register(&argos_driver);
}

static void __exit argos_exit(void)
{
	return platform_driver_unregister(&argos_driver);
}

subsys_initcall(argos_init);
module_exit(argos_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SAMSUNG Electronics");
MODULE_DESCRIPTION("ARGOS DEVICE");
