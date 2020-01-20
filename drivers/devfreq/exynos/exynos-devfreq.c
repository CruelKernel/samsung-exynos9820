/* linux/drivers/devfreq/exynos-devfreq.c
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Samsung EXYNOS SoC series devfreq common driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 2 of the License,
 * or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/reboot.h>
#include <linux/suspend.h>
#include <linux/debug-snapshot.h>
#include <linux/io.h>
#include <linux/sched/clock.h>
#include <linux/clk.h>
#include <soc/samsung/cal-if.h>
#include <soc/samsung/bts.h>
#include <linux/of_platform.h>
#include <dt-bindings/soc/samsung/exynos9810-devfreq.h>
#include "../../soc/samsung/cal-if/acpm_dvfs.h"
#include <soc/samsung/exynos-pd.h>

#include <soc/samsung/exynos-devfreq.h>
#include <soc/samsung/ect_parser.h>
#ifdef CONFIG_EXYNOS_DVFS_MANAGER
#include <soc/samsung/exynos-dm.h>
#endif
#ifdef CONFIG_EXYNOS_ACPM
#include "../../soc/samsung/acpm/acpm.h"
#include "../../soc/samsung/acpm/acpm_ipc.h"
#endif

#include "../governor.h"

#include "exynos-ppc.h"

static struct exynos_devfreq_data **devfreq_data;

static u32 freq_array[6];
static u32 boot_array[2];

#ifdef CONFIG_EXYNOS_ALT_DVFS
static struct srcu_notifier_head exynos_alt_notifier;

void exynos_alt_call_chain(void)
{
	srcu_notifier_call_chain(&exynos_alt_notifier, 0, NULL);
}

static int exynos_alt_register_notifier(struct notifier_block *nb)
{
	return srcu_notifier_chain_register(&exynos_alt_notifier, nb);
}

static int exynos_alt_unregister_notifier(struct notifier_block *nb)
{
	return srcu_notifier_chain_unregister(&exynos_alt_notifier, nb);
}

static int __init init_alt_notifier_list(void)
{
	srcu_init_notifier_head(&exynos_alt_notifier);
	return 0;
}
pure_initcall(init_alt_notifier_list);

#endif
#ifdef CONFIG_EXYNOS_DVFS_MANAGER
static unsigned int ect_find_constraint_freq(struct ect_minlock_domain *ect_domain,
					unsigned int freq)
{
	unsigned int i;

	for (i = 0; i < ect_domain->num_of_level; i++)
		if (ect_domain->level[i].main_frequencies == freq)
			break;

	return ect_domain->level[i].sub_frequencies;
}
#endif

static int exynos_constraint_parse(struct exynos_devfreq_data *data,
		unsigned int min_freq, unsigned int max_freq)
{
	struct device_node *np, *child;
	u32 num_child, constraint_dm_type, constraint_type;
	const char *devfreq_domain_name;
	int i = 0, j, const_flag = 1;
	void *min_block, *dvfs_block;
	struct ect_dvfs_domain *dvfs_domain;
	struct ect_minlock_domain *ect_domain;
#ifdef CONFIG_EXYNOS_DVFS_MANAGER
	struct exynos_dm_freq *const_table;
#endif
	np = of_find_node_by_name(data->dev->of_node, "skew");
	if (!np)
		return 0;
	num_child = of_get_child_count(np);
#ifdef CONFIG_EXYNOS_DVFS_MANAGER
	data->nr_constraint = num_child;
	data->constraint = kzalloc(sizeof(struct exynos_dm_constraint *) * num_child, GFP_KERNEL);
#endif
	if (of_property_read_string(data->dev->of_node, "devfreq_domain_name", &devfreq_domain_name))
		return -ENODEV;

	dvfs_block = ect_get_block(BLOCK_DVFS);
	if (dvfs_block == NULL)
		return -ENODEV;

	dvfs_domain = ect_dvfs_get_domain(dvfs_block, (char *)devfreq_domain_name);
	if (dvfs_domain == NULL)
		return -ENODEV;

	/* Although there is not any constraint, MIF table should be sent to FVP */
	min_block = ect_get_block(BLOCK_MINLOCK);
	if (min_block == NULL) {
		dev_info(data->dev, "There is not a min block in ECT\n");
		const_flag = 0;
	}

	ect_domain = ect_minlock_get_domain(min_block, (char *)devfreq_domain_name);
	if (ect_domain == NULL) {
		dev_info(data->dev, "There is not a domain in min block\n");
		const_flag = 0;
	}

	for_each_available_child_of_node(np, child) {
		int use_level = 0;

		if (of_property_read_u32(child, "constraint_dm_type", &constraint_dm_type))
			return -ENODEV;
		if (of_property_read_u32(child, "constraint_type", &constraint_type))
			return -ENODEV;
#ifdef CONFIG_EXYNOS_DVFS_MANAGER
		if (const_flag) {
			data->constraint[i] =
				kzalloc(sizeof(struct exynos_dm_constraint), GFP_KERNEL);
			if (data->constraint[i] == NULL) {
				dev_err(data->dev, "failed to allocate constraint\n");
				return -ENOMEM;
			}

			const_table = kzalloc(sizeof(struct exynos_dm_freq) * ect_domain->num_of_level, GFP_KERNEL);
			if (const_table == NULL) {
				dev_err(data->dev, "failed to allocate constraint\n");
				kfree(data->constraint[i]);
				return -ENOMEM;
			}

			data->constraint[i]->guidance = true;
			data->constraint[i]->constraint_type = constraint_type;
			data->constraint[i]->constraint_dm_type = constraint_dm_type;
			data->constraint[i]->table_length = ect_domain->num_of_level;
			data->constraint[i]->freq_table = const_table;
		}
#endif
		for (j = 0; j < dvfs_domain->num_of_level; j++) {
			if (data->opp_list[j].freq > max_freq ||
					data->opp_list[j].freq < min_freq)
				continue;

#ifdef CONFIG_EXYNOS_DVFS_MANAGER
			if (const_flag) {
				const_table[use_level].master_freq = data->opp_list[j].freq;
				const_table[use_level].constraint_freq
					= ect_find_constraint_freq(ect_domain, data->opp_list[j].freq);
			}
#endif
			use_level++;
		}
		i++;
	}
	return 0;
}

static int exynos_devfreq_update_fvp(struct exynos_devfreq_data *data, u32 min_freq, u32 max_freq)
{
	int ret, ch_num, size, i, use_level = 0;
	u32 cmd[4];
	struct ipc_config config;
	int nr_constraint = 0;
#ifdef CONFIG_EXYNOS_DVFS_MANAGER
	int j;
	struct exynos_dm_constraint *constraint;

	nr_constraint = data->nr_constraint;
#endif
	ret = acpm_ipc_request_channel(data->dev->of_node, NULL, &ch_num, &size);
	if (ret) {
		dev_err(data->dev, "acpm request channel is failed, id:%u, size:%u\n", ch_num, size);
		return -EINVAL;
	}
	config.cmd = cmd;
	config.response = true;
	config.indirection = false;

	/* constraint info update */
	if (nr_constraint == 0) {
		for (i = 0; i < data->max_state; i++) {
			if (data->opp_list[i].freq > max_freq ||
					data->opp_list[i].freq < min_freq)
				continue;

			config.cmd[0] = use_level;
			config.cmd[1] = data->opp_list[i].freq;
			config.cmd[2] = DATA_INIT;
			config.cmd[3] = 0;
			ret = acpm_ipc_send_data(ch_num, &config);
			if (ret) {
				dev_err(data->dev, "make constraint table is failed");
				return -EINVAL;
			}
			use_level++;
		}
	}
#ifdef CONFIG_EXYNOS_DVFS_MANAGER
	else {
		for (i = 0; i < data->nr_constraint; i++) {
			constraint = data->constraint[i];
			for (j = 0; j < data->max_state; j++) {
				if (data->opp_list[j].freq > max_freq ||
						data->opp_list[j].freq < min_freq)
					continue;

				config.cmd[0] = use_level;
				config.cmd[1] = data->opp_list[j].freq;
				config.cmd[2] = DATA_INIT;
				config.cmd[3] = constraint->freq_table[use_level].constraint_freq;
				ret = acpm_ipc_send_data(ch_num, &config);
				if (ret) {
					dev_err(data->dev, "make constraint table is failed");
					return -EINVAL;
				}
				use_level++;
			}
		}
		/* Send MIF initial freq and the number of constraint data to FVP */
		config.cmd[0] = use_level;
		config.cmd[1] = (unsigned int)data->devfreq_profile.initial_freq;
		config.cmd[2] = DATA_INIT;
		config.cmd[3] = SET_CONST;

		ret = acpm_ipc_send_data(ch_num, &config);
		if (ret) {
			dev_err(data->dev, "failed to send nr_constraint and init freq");
			return -EINVAL;
		}
	}
#endif

	return 0;
}

static int exynos_devfreq_reboot(struct exynos_devfreq_data *data)
{
	if (pm_qos_request_active(&data->default_pm_qos_max))
		pm_qos_update_request(&data->default_pm_qos_max,
				data->reboot_freq);
	return 0;

}

static int exynos_devfreq_get_freq(struct device *dev, u32 *cur_freq,
		struct clk *clk, struct exynos_devfreq_data *data)
{
	if (data->pm_domain) {
		if (!exynos_pd_status(data->pm_domain)) {
			dev_err(dev, "power domain %s is offed\n", data->pm_domain->name);
			*cur_freq = 0;
			return -EINVAL;
		}
	}

	*cur_freq = (u32)cal_dfs_get_rate(data->dfs_id);
	if (*cur_freq == 0) {
		dev_err(dev, "failed get frequency from CAL\n");
		return -EINVAL;
	}

	return 0;
}

static int exynos_devfreq_set_freq(struct device *dev, u32 new_freq,
		struct clk *clk, struct exynos_devfreq_data *data)
{
	if (data->bts_update) {
		if (data->new_freq < data->old_freq)
			bts_update_scen(BS_MIF_CHANGE, data->new_freq);
	}

	if (data->pm_domain) {
		if (!exynos_pd_status(data->pm_domain)) {
			dev_err(dev, "power domain %s is offed\n", data->pm_domain->name);
			return -EINVAL;
		}
	}

	if (cal_dfs_set_rate(data->dfs_id, (unsigned long)new_freq)) {
		dev_err(dev, "failed set frequency to CAL (%uKhz)\n",
				new_freq);
		return -EINVAL;
	}

	if (data->bts_update) {
		if (data->new_freq > data->old_freq)
			bts_update_scen(BS_MIF_CHANGE, data->new_freq);
	}

	return 0;
}

static int exynos_devfreq_init_freq_table(struct exynos_devfreq_data *data)
{
	u32 max_freq, min_freq;
	unsigned long tmp_max, tmp_min;
	struct dev_pm_opp *target_opp;
	u32 flags = 0;
	int i, ret;

	max_freq = (u32)cal_dfs_get_max_freq(data->dfs_id);
	if (!max_freq) {
		dev_err(data->dev, "failed get max frequency\n");
		return -EINVAL;
	}

	dev_info(data->dev, "max_freq: %uKhz, get_max_freq: %uKhz\n",
			data->max_freq, max_freq);

	if (max_freq < data->max_freq) {
		flags |= DEVFREQ_FLAG_LEAST_UPPER_BOUND;
		tmp_max = (unsigned long)max_freq;
		target_opp = devfreq_recommended_opp(data->dev, &tmp_max, flags);
		if (IS_ERR(target_opp)) {
			dev_err(data->dev, "not found valid OPP for max_freq\n");
			return PTR_ERR(target_opp);
		}

		data->max_freq = (u32)dev_pm_opp_get_freq(target_opp);
		dev_pm_opp_put(target_opp);
	}

	/* min ferquency must be equal or under max frequency */
	if (data->min_freq > data->max_freq)
		data->min_freq = data->max_freq;

	min_freq = (u32)cal_dfs_get_min_freq(data->dfs_id);
	if (!min_freq) {
		dev_err(data->dev, "failed get min frequency\n");
		return -EINVAL;
	}

	dev_info(data->dev, "min_freq: %uKhz, get_min_freq: %uKhz\n",
			data->min_freq, min_freq);

	if (min_freq > data->min_freq) {
		flags &= ~DEVFREQ_FLAG_LEAST_UPPER_BOUND;
		tmp_min = (unsigned long)min_freq;
		target_opp = devfreq_recommended_opp(data->dev, &tmp_min, flags);
		if (IS_ERR(target_opp)) {
			dev_err(data->dev, "not found valid OPP for min_freq\n");
			return PTR_ERR(target_opp);
		}

		data->min_freq = (u32)dev_pm_opp_get_freq(target_opp);
		dev_pm_opp_put(target_opp);
	}

	dev_info(data->dev, "min_freq: %uKhz, max_freq: %uKhz\n",
			data->min_freq, data->max_freq);

	for (i = 0; i < data->max_state; i++) {
		if (data->opp_list[i].freq > data->max_freq ||
			data->opp_list[i].freq < data->min_freq)
			dev_pm_opp_disable(data->dev, (unsigned long)data->opp_list[i].freq);
	}

	data->devfreq_profile.initial_freq = cal_dfs_get_boot_freq(data->dfs_id);
	data->devfreq_profile.suspend_freq = cal_dfs_get_resume_freq(data->dfs_id);

	ret = exynos_constraint_parse(data, min_freq, max_freq);
	if (ret) {
		dev_err(data->dev, "failed to parse constraint table\n");
		return -EINVAL;
	}

	if (data->update_fvp)
		exynos_devfreq_update_fvp(data, min_freq, max_freq);

	if (data->use_acpm) {
		ret = exynos_acpm_set_init_freq(data->dfs_id, data->devfreq_profile.initial_freq);
		if (ret) {
			dev_err(data->dev, "failed to set init freq\n");
			return -EINVAL;
		}
	}

	return 0;
}

#ifdef CONFIG_ARM_EXYNOS_DEVFREQ_DEBUG
static ssize_t show_exynos_devfreq_info(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	int i;

	count = snprintf(buf, PAGE_SIZE, "[Exynos DEVFREQ Data]\n"
			 "devfreq dev name : %20s\n"
			 "devfreq type     : %20d\n"
			 "Exynos SS flag   : %20u\n",
			 dev_name(data->dev), data->devfreq_type, data->ess_flag);

	count += snprintf(buf + count, PAGE_SIZE, "\n<Frequency data>\n"
			  "OPP list length  : %20u\n", data->max_state);
	count += snprintf(buf + count, PAGE_SIZE, "freq opp table\n");
	count += snprintf(buf + count, PAGE_SIZE, "\t  idx      freq       volt\n");

	for (i = 0; i < data->max_state; i++)
		count += snprintf(buf + count, PAGE_SIZE, "\t%5u %10u %10u\n",
				  data->opp_list[i].idx, data->opp_list[i].freq,
				  data->opp_list[i].volt);

	count += snprintf(buf + count, PAGE_SIZE,
			  "default_qos     : %20u\n" "initial_freq    : %20lu\n"
			  "min_freq        : %20u\n" "max_freq        : %20u\n"
			  "boot_timeout(s) : %20u\n" "max_state       : %20u\n",
			  data->default_qos, data->devfreq_profile.initial_freq,
			  data->min_freq, data->max_freq, data->boot_qos_timeout, data->max_state);

	count += snprintf(buf + count, PAGE_SIZE, "\n<Governor data>\n");
	count += snprintf(buf + count, PAGE_SIZE,
			  "governor_name   : %20s\n",
			  data->governor_name);
	return count;
}

static ssize_t show_exynos_devfreq_get_freq(struct device *dev,
					    struct device_attribute *attr, char *buf)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	u32 get_freq = 0;

	if (exynos_devfreq_get_freq(data->dev, &get_freq, data->clk, data))
		dev_err(data->dev, "failed get freq\n");

	count = snprintf(buf, PAGE_SIZE, "%10u Khz\n", get_freq);

	return count;
}

static int exynos_devfreq_cmu_dump(struct exynos_devfreq_data *data)
{
	mutex_lock(&data->devfreq->lock);
	cal_vclk_dbg_info(data->dfs_id);
	mutex_unlock(&data->devfreq->lock);

	return 0;
}

static ssize_t show_exynos_devfreq_cmu_dump(struct device *dev,
					    struct device_attribute *attr, char *buf)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;

	mutex_lock(&data->lock);
	if (exynos_devfreq_cmu_dump(data))
		dev_err(data->dev, "failed CMU Dump\n");
	mutex_unlock(&data->lock);

	count = snprintf(buf, PAGE_SIZE, "Done\n");

	return count;
}

static ssize_t show_debug_scaling_devfreq_max(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	int val;

	if (data->pm_qos_class_max) {
		val = pm_qos_read_req_value(data->pm_qos_class_max, &data->debug_pm_qos_max);
		if (val < 0) {
			dev_err(dev, "failed to read requested value\n");
			return count;
		}
		count += snprintf(buf, PAGE_SIZE, "%d\n", val);
	}

	return count;
}

static ssize_t store_debug_scaling_devfreq_max(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	int ret;
	u32 qos_value;

	ret = sscanf(buf, "%u", &qos_value);
	if (ret != 1)
		return -EINVAL;

	if (data->pm_qos_class_max) {
		if (pm_qos_request_active(&data->debug_pm_qos_max))
			pm_qos_update_request(&data->debug_pm_qos_max, qos_value);
	}

	return count;
}

static ssize_t show_debug_scaling_devfreq_min(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	int val;

	val = pm_qos_read_req_value(data->pm_qos_class, &data->debug_pm_qos_min);
	if (val < 0) {
		dev_err(dev, "failed to read requested value\n");
		return count;
	}

	count += snprintf(buf, PAGE_SIZE, "%d\n", val);

	return count;
}

static ssize_t store_debug_scaling_devfreq_min(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	int ret;
	u32 qos_value;

	ret = sscanf(buf, "%u", &qos_value);
	if (ret != 1)
		return -EINVAL;

	if (pm_qos_request_active(&data->debug_pm_qos_min))
		pm_qos_update_request(&data->debug_pm_qos_min, qos_value);

	return count;
}

static ssize_t show_exynos_devfreq_disable_pm_qos(struct device *dev,
						  struct device_attribute *attr, char *buf)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;

	count += snprintf(buf, PAGE_SIZE, "%s\n",
			  data->devfreq->disabled_pm_qos ? "disabled" : "enabled");

	return count;
}

static ssize_t store_exynos_devfreq_disable_pm_qos(struct device *dev,
						   struct device_attribute *attr,
						   const char *buf, size_t count)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	int ret;
	u32 disable;

	ret = sscanf(buf, "%u", &disable);
	if (ret != 1)
		return -EINVAL;

	if (disable)
		data->devfreq->disabled_pm_qos = true;
	else
		data->devfreq->disabled_pm_qos = false;

	return count;
}

static ssize_t show_alt_dvfs_info(struct device *dev,
					struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_EXYNOS_ALT_DVFS)
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent,
						    struct platform_device,
						    dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	int i;

	mutex_lock(&data->devfreq->lock);

	for (i = 0; i < data->simple_interactive_data.alt_data.num_target_load; i++) {
		count += snprintf(buf + count, PAGE_SIZE, "%d%s",
				  data->simple_interactive_data.alt_data.target_load[i],
				  (i == data->simple_interactive_data.alt_data.num_target_load - 1) ?
				  "" : (i % 2) ? ":" : " ");
	}
	count += snprintf(buf + count, PAGE_SIZE, "\n");
	/* Parameters */
	count += snprintf(buf + count, PAGE_SIZE, "MIN SAMPLE TIME: %u\n",
				data->simple_interactive_data.alt_data.min_sample_time);
	count += snprintf(buf + count, PAGE_SIZE, "HOLD SAMPLE TIME: %u\n",
				data->simple_interactive_data.alt_data.hold_sample_time);
	count += snprintf(buf + count, PAGE_SIZE, "HISPEED LOAD: %u\n",
				data->simple_interactive_data.alt_data.hispeed_load);
	count += snprintf(buf + count, PAGE_SIZE, "HISPEED FREQ: %u\n",
				data->simple_interactive_data.alt_data.hispeed_freq);

	mutex_unlock(&data->devfreq->lock);

	return count;
#else
	return 0;
#endif
}

static DEVICE_ATTR(alt_dvfs_info, 0640, show_alt_dvfs_info, NULL);

static DEVICE_ATTR(exynos_devfreq_info, 0640, show_exynos_devfreq_info, NULL);
static DEVICE_ATTR(exynos_devfreq_get_freq, 0640, show_exynos_devfreq_get_freq, NULL);
static DEVICE_ATTR(exynos_devfreq_cmu_dump, 0640, show_exynos_devfreq_cmu_dump, NULL);
static DEVICE_ATTR(debug_scaling_devfreq_min, 0640, show_debug_scaling_devfreq_min, store_debug_scaling_devfreq_min);
static DEVICE_ATTR(debug_scaling_devfreq_max, 0640, show_debug_scaling_devfreq_max,
						store_debug_scaling_devfreq_max);
static DEVICE_ATTR(disable_pm_qos, 0640, show_exynos_devfreq_disable_pm_qos,
		   store_exynos_devfreq_disable_pm_qos);

static struct attribute *exynos_devfreq_sysfs_entries[] = {
	&dev_attr_exynos_devfreq_info.attr,
	&dev_attr_exynos_devfreq_get_freq.attr,
	&dev_attr_exynos_devfreq_cmu_dump.attr,
	&dev_attr_debug_scaling_devfreq_min.attr,
	&dev_attr_debug_scaling_devfreq_max.attr,
	&dev_attr_disable_pm_qos.attr,
	&dev_attr_alt_dvfs_info.attr,
	NULL,
};

static struct attribute_group exynos_devfreq_attr_group = {
	.name = "exynos_data",
	.attrs = exynos_devfreq_sysfs_entries,
};
#endif

static ssize_t show_scaling_devfreq_min(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	int val;

	val = pm_qos_read_req_value(data->pm_qos_class, &data->sys_pm_qos_min);
	if (val < 0) {
		dev_err(dev, "failed to read requested value\n");
		return count;
	}

	count += snprintf(buf, PAGE_SIZE, "%d\n", val);

	return count;
}

static ssize_t store_scaling_devfreq_min(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	int ret;
	u32 qos_value;

	ret = sscanf(buf, "%u", &qos_value);
	if (ret != 1)
		return -EINVAL;

	if (pm_qos_request_active(&data->sys_pm_qos_min))
		pm_qos_update_request(&data->sys_pm_qos_min, qos_value);

	return count;
}

static DEVICE_ATTR(scaling_devfreq_min, 0640, show_scaling_devfreq_min, store_scaling_devfreq_min);

/* get frequency and delay time data from string */
static unsigned int *get_tokenized_data(const char *buf, int *num_tokens)
{
	const char *cp;
	int i;
	int ntokens = 1;
	unsigned int *tokenized_data;
	int err = -EINVAL;

	cp = buf;
	while ((cp = strpbrk(cp + 1, " :")))
		ntokens++;

	if (!(ntokens & 0x1))
		goto err;

	tokenized_data = kmalloc(ntokens * sizeof(unsigned int), GFP_KERNEL);
	if (!tokenized_data) {
		err = -ENOMEM;
		goto err;
	}

	cp = buf;
	i = 0;
	while (i < ntokens) {
		if (sscanf(cp, "%u", &tokenized_data[i++]) != 1)
			goto err_kfree;

		cp = strpbrk(cp, " :");
		if (!cp)
			break;
		cp++;
	}

	if (i != ntokens)
		goto err_kfree;

	*num_tokens = ntokens;
	return tokenized_data;

err_kfree:
	kfree(tokenized_data);
err:
	return ERR_PTR(err);
}

static ssize_t show_use_delay_time(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;

	mutex_lock(&data->devfreq->lock);
	count += snprintf(buf, PAGE_SIZE, "%s\n",
			  (data->simple_interactive_data.use_delay_time) ? "true" : "false");
	mutex_unlock(&data->devfreq->lock);
	return count;
}

static ssize_t store_use_delay_time(struct device *dev,
				    struct device_attribute *attr, const char *buf, size_t count)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	int ret, use_delay_time;

	ret = sscanf(buf, "%d", &use_delay_time);

	if (ret != 1)
		return -EINVAL;

	if (use_delay_time == 0 || use_delay_time == 1) {
		mutex_lock(&data->devfreq->lock);
		data->simple_interactive_data.use_delay_time = use_delay_time ? true : false;
		mutex_unlock(&data->devfreq->lock);
	} else {
		dev_info(data->dev, "This is invalid value: %d\n", use_delay_time);
	}

	return count;
}

static ssize_t show_delay_time(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	int i;

	mutex_lock(&data->devfreq->lock);
	for (i = 0; i < data->simple_interactive_data.ndelay_time; i++) {
		count += snprintf(buf + count, PAGE_SIZE, "%d%s",
				data->simple_interactive_data.delay_time[i],
				(i == data->simple_interactive_data.ndelay_time - 1) ?
				"" : (i % 2) ? ":" : " ");
	}
	count += snprintf(buf + count, PAGE_SIZE, "\n");
	mutex_unlock(&data->devfreq->lock);
	return count;
}

static ssize_t store_delay_time(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	int ntokens;
	int *new_delay_time = NULL;

	new_delay_time = get_tokenized_data(buf , &ntokens);
	if (IS_ERR(new_delay_time))
		return PTR_RET(new_delay_time);

	mutex_lock(&data->devfreq->lock);
	kfree(data->simple_interactive_data.delay_time);
	data->simple_interactive_data.delay_time = new_delay_time;
	data->simple_interactive_data.ndelay_time = ntokens;
	mutex_unlock(&data->devfreq->lock);

	return count;
}

#if defined(CONFIG_EXYNOS_ALT_DVFS)
static ssize_t show_target_load(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent,
						    struct platform_device,
						    dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	int i;

	mutex_lock(&data->devfreq->lock);
	for (i = 0; i < data->simple_interactive_data.alt_data.num_target_load; i++) {
		count += snprintf(buf + count, PAGE_SIZE, "%d%s",
				  data->simple_interactive_data.alt_data.target_load[i],
				  (i == data->simple_interactive_data.alt_data.num_target_load - 1) ?
				  "" : (i % 2) ? ":" : " ");
	}
	count += snprintf(buf + count, PAGE_SIZE, "\n");
	mutex_unlock(&data->devfreq->lock);
	return count;
}

static ssize_t store_target_load(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent,
						    struct platform_device,
						    dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	int ntokens;
	int *new_target_load = NULL;

	new_target_load = get_tokenized_data(buf , &ntokens);
	if (IS_ERR(new_target_load))
		return PTR_RET(new_target_load);

	mutex_lock(&data->devfreq->lock);
	kfree(data->simple_interactive_data.alt_data.target_load);
	data->simple_interactive_data.alt_data.target_load = new_target_load;
	data->simple_interactive_data.alt_data.num_target_load = ntokens;
	mutex_unlock(&data->devfreq->lock);

	return count;
}

static ssize_t show_hold_sample_time(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent,
						    struct platform_device,
						    dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;

	mutex_lock(&data->devfreq->lock);
	count += snprintf(buf, PAGE_SIZE, "%u\n",
			  data->simple_interactive_data.alt_data.hold_sample_time);
	mutex_unlock(&data->devfreq->lock);
	return count;
}

static ssize_t store_hold_sample_time(struct device *dev,
			     struct device_attribute *attr, const char *buf,
			     size_t count)
{
	struct device *parent = dev->parent;
	struct platform_device *pdev = container_of(parent,
						    struct platform_device,
						    dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	int ret;

	mutex_lock(&data->devfreq->lock);
	ret = sscanf(buf, "%u", &data->simple_interactive_data.alt_data.hold_sample_time);
	mutex_unlock(&data->devfreq->lock);
	if (ret != 1)
		return -EINVAL;

	return count;
}
#endif


static DEVICE_ATTR(use_delay_time, 0640, show_use_delay_time, store_use_delay_time);
static DEVICE_ATTR(delay_time, 0640, show_delay_time, store_delay_time);
#if defined(CONFIG_EXYNOS_ALT_DVFS)
static DEVICE_ATTR(target_load, 0640, show_target_load, store_target_load);
static DEVICE_ATTR(hold_sample_time, 0640, show_hold_sample_time, store_hold_sample_time);
#endif

static struct attribute *devfreq_interactive_sysfs_entries[] = {
	&dev_attr_use_delay_time.attr,
	&dev_attr_delay_time.attr,
#if defined(CONFIG_EXYNOS_ALT_DVFS)
	&dev_attr_target_load.attr,
	&dev_attr_hold_sample_time.attr,
#endif
	NULL,
};

static struct attribute_group devfreq_delay_time_attr_group = {
	.name = "interactive",
	.attrs = devfreq_interactive_sysfs_entries,
};

#ifdef CONFIG_EXYNOS_DVFS_MANAGER
int find_exynos_devfreq_dm_type(struct device *dev, int *dm_type)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);

	*dm_type = data->dm_type;

	return 0;
}

struct devfreq *find_exynos_devfreq_device(void *devdata)
{
	struct exynos_devfreq_data *data = devdata;

	if (!devdata) {
		pr_err("%s: failed get Devfreq type\n", __func__);
		return ERR_PTR(-EINVAL);
	}

	return data->devfreq;
}
#endif

#ifdef CONFIG_OF
#if defined(CONFIG_ECT)
static int exynos_devfreq_parse_ect(struct exynos_devfreq_data *data, const char *dvfs_domain_name)
{
	int i;
	void *dvfs_block;
	struct ect_dvfs_domain *dvfs_domain;

	dvfs_block = ect_get_block(BLOCK_DVFS);
	if (dvfs_block == NULL)
		return -ENODEV;

	dvfs_domain = ect_dvfs_get_domain(dvfs_block, (char *)dvfs_domain_name);
	if (dvfs_domain == NULL)
		return -ENODEV;

	data->max_state = dvfs_domain->num_of_level;
	data->opp_list = kzalloc(sizeof(struct exynos_devfreq_opp_table) * data->max_state, GFP_KERNEL);
	if (!data->opp_list) {
		pr_err("%s: failed to allocate opp_list\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < dvfs_domain->num_of_level; ++i) {
		data->opp_list[i].idx = i;
		data->opp_list[i].freq = dvfs_domain->list_level[i].level;
		data->opp_list[i].volt = 0;
	}

	return 0;
}
#endif

static int exynos_devfreq_parse_dt(struct device_node *np, struct exynos_devfreq_data *data)
{
	const char *use_acpm, *bts_update;
	const char *use_get_dev;
#if defined(CONFIG_ECT)
	const char *devfreq_domain_name;
#endif
	const char *buf;
	const char *use_delay_time;
	const char *pd_name;
	const char *update_fvp;
	int ntokens;
	int not_using_ect = true;
#if defined(CONFIG_EXYNOS_ALT_DVFS)
	struct devfreq_alt_dvfs_data *alt_data;
#endif

	if (!np)
		return -ENODEV;

	if (of_property_read_u32(np, "devfreq_type", &data->devfreq_type))
		return -ENODEV;
	if (of_property_read_u32(np, "pm_qos_class", &data->pm_qos_class))
		return -ENODEV;
	if (of_property_read_u32(np, "pm_qos_class_max", &data->pm_qos_class_max))
		return -ENODEV;
	if (of_property_read_u32(np, "ess_flag", &data->ess_flag))
		return -ENODEV;

#if defined(CONFIG_ECT)
	if (of_property_read_string(np, "devfreq_domain_name", &devfreq_domain_name))
		return -ENODEV;
	not_using_ect = exynos_devfreq_parse_ect(data, devfreq_domain_name);
#endif
	if (not_using_ect) {
		dev_err(data->dev, "cannot parse the DVFS info in ECT");
		return -ENODEV;
	}

	if (of_property_read_string(np, "pd_name", &pd_name)) {
		dev_info(data->dev, "no power domain\n");
		data->pm_domain = NULL;
	} else {
		dev_info(data->dev, "power domain: %s\n", pd_name);
		data->pm_domain = exynos_pd_lookup_name(pd_name);
	}


	if (of_property_read_u32_array(np, "freq_info", (u32 *)&freq_array,
				       (size_t)(ARRAY_SIZE(freq_array))))
		return -ENODEV;

	data->devfreq_profile.initial_freq = freq_array[0];
	data->default_qos = freq_array[1];
	data->devfreq_profile.suspend_freq = freq_array[2];
	data->min_freq = freq_array[3];
	data->max_freq = freq_array[4];
	data->reboot_freq = freq_array[5];

	if (of_property_read_u32_array(np, "boot_info", (u32 *)&boot_array,
				       (size_t)(ARRAY_SIZE(boot_array)))) {
		data->boot_qos_timeout = 0;
		data->boot_freq = 0;
		dev_info(data->dev, "This doesn't use boot value\n");
	} else {
		data->boot_qos_timeout = boot_array[0];
		data->boot_freq = boot_array[1];
	}

	if (of_property_read_u32(np, "governor", &data->gov_type))
		return -ENODEV;
	if (data->gov_type == SIMPLE_INTERACTIVE)
		data->governor_name = "interactive";
	else {
		dev_err(data->dev, "invalid governor name (%s)\n", data->governor_name);
		return -EINVAL;
	}

	if (!of_property_read_string(np, "use_acpm", &use_acpm)) {
		if (!strcmp(use_acpm, "true")) {
			data->use_acpm = true;
		} else {
			data->use_acpm = false;
			dev_info(data->dev, "This does not use acpm\n");
		}
	} else {
		dev_info(data->dev, "This does not use acpm\n");
		data->use_acpm = false;
	}

	if (!of_property_read_string(np, "bts_update", &bts_update)) {
		if (!strcmp(bts_update, "true")) {
			data->bts_update = true;
		} else {
			data->bts_update = false;
			dev_info(data->dev, "This does not bts update\n");
		}
	} else {
		dev_info(data->dev, "This does not bts update\n");
		data->bts_update = false;
	}

	if (!of_property_read_string(np, "update_fvp", &update_fvp)) {
		if (!strcmp(update_fvp, "true")) {
			data->update_fvp = true;
		} else {
			data->update_fvp = false;
			dev_info(data->dev, "This does not update fvp\n");
		}
	} else {
		dev_info(data->dev, "This does not update fvp\n");
		data->update_fvp = false;
	}

	if (of_property_read_u32(np, "dfs_id", &data->dfs_id) &&
			of_property_match_string(np, "clock-names", buf))
		return -ENODEV;

	if (!of_property_read_string(np, "use_get_dev", &use_get_dev)) {
		if (!strcmp(use_get_dev, "true")) {
			data->use_get_dev = true;
		} else if (!strcmp(use_get_dev, "false")) {
			data->use_get_dev = false;
		} else {
			dev_err(data->dev, "invalid use_get_dev string (%s)\n", use_get_dev);
			return -EINVAL;
		}
	} else {
		dev_info(data->dev, "Operation function get_dev_status will not be registed.\n");
		data->use_get_dev = false;
	}

	of_property_read_u32(np, "polling_ms", &data->devfreq_profile.polling_ms);

	if (data->gov_type == SIMPLE_INTERACTIVE) {
		if (of_property_read_string(np, "use_delay_time", &use_delay_time))
			return -ENODEV;

		if (!strcmp(use_delay_time, "true")) {
			data->simple_interactive_data.use_delay_time = true;
		} else if (!strcmp(use_delay_time, "false")) {
			data->simple_interactive_data.use_delay_time = false;
		} else {
			dev_err(data->dev, "invalid use_delay_time : (%s)\n", use_delay_time);
			return -EINVAL;
		}

		if (data->simple_interactive_data.use_delay_time) {
			if (of_property_read_string(np, "delay_time_list", &buf)) {
				/*
				 * If there is not delay time list,
				 * delay time will be filled with default time
				 */
				data->simple_interactive_data.delay_time =
					kmalloc(sizeof(unsigned int), GFP_KERNEL);
				if (!data->simple_interactive_data.delay_time) {
					dev_err(data->dev, "Fail to allocate delay_time memory\n");
					return -ENOMEM;
				}
				*(data->simple_interactive_data.delay_time)
					= DEFAULT_DELAY_TIME;
				data->simple_interactive_data.ndelay_time =
					DEFAULT_NDELAY_TIME;
				dev_info(data->dev, "set default delay time %d ms\n",
						DEFAULT_DELAY_TIME);
			} else {
				data->simple_interactive_data.delay_time =
					get_tokenized_data(buf, &ntokens);
				data->simple_interactive_data.ndelay_time = ntokens;
			}
		}

		/* Polling ms must be registered */
		/*	 */
		if (data->use_get_dev) {
			/* Register um_data and um_list for tracing load */
			int i;

			if (of_property_read_u32(np, "um_count", &data->um_data.um_count))
				return -ENODEV;

			data->um_data.pa_base = kzalloc(sizeof(u32) *
							data->um_data.um_count, GFP_KERNEL);

			if (data->um_data.pa_base == NULL) {
				dev_err(data->dev, "failed to allocate memory for PA base\n");
				return -ENOMEM;
			}

			if (of_property_read_u32_array(np, "um_list",
							(u32 *)data->um_data.pa_base,
							(size_t)(data->um_data.um_count)))
				return -ENODEV;

			data->um_data.va_base = kzalloc(sizeof(void __iomem *) *
							data->um_data.um_count, GFP_KERNEL);

			if (data->um_data.va_base == NULL) {
				dev_err(data->dev, "failed to allocate memory for va base");
				kfree(data->um_data.pa_base);
				return -ENOMEM;
			}

			for (i = 0; i < data->um_data.um_count; i++) {
				data->um_data.va_base[i] =
					ioremap(data->um_data.pa_base[i], SZ_4K);
			}
		} else {
			dev_info(data->dev, "Not registed UM monitor data\n");
		}

#if defined(CONFIG_EXYNOS_ALT_DVFS)
		/* Parse ALT-DVFS related parameters */
		if (of_property_read_bool(np, "use_alt_dvfs")) {

			alt_data = &(data->simple_interactive_data.alt_data);

			if (!of_property_read_string(np, "target_load", &buf)) {
				/* Parse target load table */
				alt_data->target_load =
					get_tokenized_data(buf, &ntokens);
				alt_data->num_target_load = ntokens;
			} else {
				/* Fix target load as defined ALTDVFS_TARGET_LOAD */
				alt_data->target_load =
					kmalloc(sizeof(unsigned int), GFP_KERNEL);
				if(!alt_data->target_load) {
					dev_err(data->dev, "Failed to allocate memory\n");
					return -ENOMEM;
				}
				*(alt_data->target_load) = ALTDVFS_TARGET_LOAD;
				alt_data->num_target_load = ALTDVFS_NUM_TARGET_LOAD;
			}

			if (of_property_read_u32(np, "min_sample_time", &alt_data->min_sample_time))
				alt_data->min_sample_time = ALTDVFS_MIN_SAMPLE_TIME;
			if (of_property_read_u32(np, "hold_sample_time", &alt_data->hold_sample_time))
				alt_data->hold_sample_time = ALTDVFS_HOLD_SAMPLE_TIME;
			if (of_property_read_u32(np, "hispeed_load", &alt_data->hispeed_load))
				alt_data->hispeed_load = ALTDVFS_HISPEED_LOAD;
			if (of_property_read_u32(np, "hispeed_freq", &alt_data->hispeed_freq))
				alt_data->hispeed_freq = ALTDVFS_HISPEED_FREQ;
			if (of_property_read_u32(np, "tolerance", &alt_data->tolerance))
				alt_data->tolerance = ALTDVFS_TOLERANCE;

			/* Initial buffer and load setup */
			alt_data->front = alt_data->buffer;
			alt_data->rear = alt_data->buffer;
			alt_data->min_load = 100;

			/* Initial governor freq setup */
			data->simple_interactive_data.governor_freq = 0;

		} else {
			dev_info(data->dev, "ALT-DVFS is not declared by device tree.\n");
		}
#endif
	} else {
		dev_err(data->dev, "not support governor type %u\n", data->gov_type);
		return -EINVAL;
	}

#ifdef CONFIG_EXYNOS_DVFS_MANAGER
	if (of_property_read_u32(np, "dm-index", &data->dm_type)) {
		dev_err(data->dev, "not support dvfs manager\n");
		return -ENODEV;
	}
#endif
	return 0;
}
#else
static int exynos_devfreq_parse_dt(struct device_node *np, struct exynos_devfrq_data *data)
{
	return -EINVAL;
}
#endif

s32 exynos_devfreq_get_opp_idx(struct exynos_devfreq_opp_table *table, unsigned int size, u32 freq)
{
	int i;

	for (i = 0; i < size; ++i) {
		if (table[i].freq == freq)
			return i;
	}

	return -ENODEV;
}

static int exynos_init_freq_table(struct exynos_devfreq_data *data)
{
	int i, ret;
	u32 freq, volt;

	for (i = 0; i < data->max_state; i++) {
		freq = data->opp_list[i].freq;
		volt = data->opp_list[i].volt;

		data->devfreq_profile.freq_table[i] = freq;

		ret = dev_pm_opp_add(data->dev, freq, volt);
		if (ret) {
			dev_err(data->dev, "failed to add opp entries %uKhz\n", freq);
			return ret;
		} else {
			dev_info(data->dev, "DEVFREQ : %8uKhz, %8uuV\n", freq, volt);
		}
	}

	ret = exynos_devfreq_init_freq_table(data);
	if (ret) {
		dev_err(data->dev, "failed init frequency table\n");
		return ret;
	}

	return 0;
}

static int exynos_devfreq_reboot_notifier(struct notifier_block *nb, unsigned long val, void *v)
{
	struct exynos_devfreq_data *data = container_of(nb, struct exynos_devfreq_data,
							reboot_notifier);

	if (pm_qos_request_active(&data->default_pm_qos_min))
		pm_qos_update_request(&data->default_pm_qos_min, data->reboot_freq);

	if (exynos_devfreq_reboot(data)) {
		dev_err(data->dev, "failed reboot\n");
		return NOTIFY_BAD;
	}

	return NOTIFY_OK;
}

#if defined(CONFIG_EXYNOS_ALT_DVFS)
static int exynos_devfreq_notifier(struct notifier_block *nb, unsigned long val, void *v)
{
	struct devfreq_notifier_block *um_nb = container_of(nb, struct devfreq_notifier_block, nb);
	int err;

	mutex_lock(&um_nb->df->lock);
	err = update_devfreq(um_nb->df);
	if (err && err != -EAGAIN) {
		dev_err(&um_nb->df->dev, "devfreq failed with (%d) error\n", err);
		mutex_unlock(&um_nb->df->lock);
		return NOTIFY_BAD;
	}
	mutex_unlock(&um_nb->df->lock);

	return NOTIFY_OK;
}

#endif
static int exynos_devfreq_target(struct device *dev, unsigned long *target_freq, u32 flags)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	struct timeval before_target, after_target, before_setfreq, after_setfreq;
	struct dev_pm_opp *target_opp;
	u32 target_volt;
	s32 target_idx;
	s32 target_time = 0;
	int ret = 0;

	if (data->devfreq_disabled)
		return -EAGAIN;

	do_gettimeofday(&before_target);

	mutex_lock(&data->lock);

	target_opp = devfreq_recommended_opp(dev, target_freq, flags);
	if (IS_ERR(target_opp)) {
		dev_err(dev, "not found valid OPP table\n");
		ret = PTR_ERR(target_opp);
		goto out;
	}

	*target_freq = dev_pm_opp_get_freq(target_opp);
	target_volt = (u32)dev_pm_opp_get_voltage(target_opp);
	dev_pm_opp_put(target_opp);

	target_idx = exynos_devfreq_get_opp_idx(data->opp_list, data->max_state, *target_freq);
	if (target_idx < 0) {
		ret = -EINVAL;
		goto out;
	}

	data->new_freq = (u32)(*target_freq);
	data->new_idx = target_idx;
	data->new_volt = target_volt;

	if (data->old_freq == data->new_freq)
		goto out;

	dev_dbg(dev, "LV_%d, %uKhz, %uuV ======> LV_%d, %uKhz, %uuV\n",
		data->old_idx, data->old_freq, data->old_volt,
		data->new_idx, data->new_freq, data->new_volt);

	dbg_snapshot_freq(data->ess_flag, data->old_freq, data->new_freq, DSS_FLAG_IN);
	do_gettimeofday(&before_setfreq);

	ret = exynos_devfreq_set_freq(dev, data->new_freq, data->clk, data);
	if (ret) {
		dev_err(dev, "failed set frequency (%uKhz --> %uKhz)\n",
			data->old_freq, data->new_freq);
		goto out;
	}

	do_gettimeofday(&after_setfreq);
	dbg_snapshot_freq(data->ess_flag, data->old_freq, data->new_freq, DSS_FLAG_OUT);

	data->old_freq = data->new_freq;
	data->old_idx = data->new_idx;
	data->old_volt = data->new_volt;

out:
	mutex_unlock(&data->lock);

	do_gettimeofday(&after_target);

	target_time = (after_target.tv_sec - before_target.tv_sec) * USEC_PER_SEC +
	    (after_target.tv_usec - before_target.tv_usec);

	data->target_delay = target_time;

	dev_dbg(dev, "target time: %d usec\n", target_time);

	return ret;
}

static int exynos_devfreq_suspend(struct device *dev)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
	int ret = 0;
#ifdef CONFIG_EXYNOS_DVFS_MANAGER
#ifdef CONFIG_EXYNOS_ACPM
	int size, ch_num;
	unsigned int cmd[4];
	struct ipc_config config;
#endif
#endif
	u32 get_freq = 0;

#ifdef CONFIG_EXYNOS_DVFS_MANAGER
	if (data->use_acpm) {
		mutex_lock(&data->devfreq->lock);
		//send flag
#ifdef CONFIG_EXYNOS_ACPM
		ret = acpm_ipc_request_channel(dev->of_node, NULL, &ch_num, &size);
		if (ret) {
			dev_err(dev, "acpm request channel is failed, id:%u, size:%u\n", ch_num, size);
			mutex_unlock(&data->devfreq->lock);
			return -EINVAL;
		}
		/* Initial value of release flag is true.
		 * "true" means state of AP is running
		 * "false means state of AP is sleep.
		 */
		config.cmd = cmd;
		config.response = true;
		config.indirection = false;
		config.cmd[0] = data->devfreq_type;
		config.cmd[1] = false;
		config.cmd[2] = DATA_INIT;
		config.cmd[3] = RELEASE;

		ret = acpm_ipc_send_data(ch_num, &config);
		if (ret) {
			dev_err(dev, "failed to send release infomation to FVP");
			mutex_unlock(&data->devfreq->lock);
			return -EINVAL;
		}
#endif
		data->devfreq->str_freq = data->devfreq_profile.suspend_freq;
		ret = update_devfreq(data->devfreq);
		if (ret && ret != -EAGAIN) {
			dev_err(&data->devfreq->dev, "devfreq failed with (%d) error\n", ret);
			mutex_unlock(&data->devfreq->lock);
			return NOTIFY_BAD;
		}
		mutex_unlock(&data->devfreq->lock);
	}
#endif
	if (!data->use_acpm && pm_qos_request_active(&data->default_pm_qos_min))
		pm_qos_update_request(&data->default_pm_qos_min,
				data->devfreq_profile.suspend_freq);
	if (exynos_devfreq_get_freq(data->dev, &get_freq, data->clk, data))
		dev_err(data->dev, "failed get freq\n");

#if defined(CONFIG_EXYNOS_ALT_DVFS)
	exynos_devfreq_um_exit(data);
#endif

	dev_info(data->dev, "Suspend_frequency is %u\n", get_freq);

	return ret;
}

static int exynos_devfreq_resume(struct device *dev)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
#ifdef CONFIG_EXYNOS_DVFS_MANAGER
#ifdef CONFIG_EXYNOS_ACPM
	int size, ch_num;
	unsigned int cmd[4];
	struct ipc_config config;
#endif
#endif
	int ret = 0;
	u32 cur_freq;

	if (!exynos_devfreq_get_freq(data->dev, &cur_freq, data->clk, data))
		dev_info(data->dev, "Resume frequency is %u\n", cur_freq);
#ifdef CONFIG_EXYNOS_DVFS_MANAGER
	if (data->use_acpm) {
		mutex_lock(&data->devfreq->lock);
		//send flag
#ifdef CONFIG_EXYNOS_ACPM
		ret = acpm_ipc_request_channel(dev->of_node, NULL, &ch_num, &size);
		if (ret) {
			dev_err(dev, "acpm request channel is failed, id:%u, size:%u\n", ch_num, size);
			mutex_unlock(&data->devfreq->lock);
			return -EINVAL;
		}

		config.cmd = cmd;
		config.response = true;
		config.indirection = false;
		config.cmd[0] = data->devfreq_type;
		config.cmd[1] = true;
		config.cmd[2] = DATA_INIT;
		config.cmd[3] = RELEASE;

		ret = acpm_ipc_send_data(ch_num, &config);
		if (ret) {
			dev_err(dev, "failed to send release infomation to FVP");
			mutex_unlock(&data->devfreq->lock);
			return -EINVAL;
		}
#endif
		data->devfreq->str_freq = 0;
		ret = update_devfreq(data->devfreq);
		if (ret && ret != -EAGAIN) {
			dev_err(&data->devfreq->dev, "devfreq failed with (%d) error\n", ret);
			mutex_unlock(&data->devfreq->lock);
			return NOTIFY_BAD;
		}
		mutex_unlock(&data->devfreq->lock);
	}
#endif
	if (!data->use_acpm && pm_qos_request_active(&data->default_pm_qos_min))
		pm_qos_update_request(&data->default_pm_qos_min, data->default_qos);

#if defined(CONFIG_EXYNOS_ALT_DVFS)
	ret = exynos_devfreq_um_init(data);
	if (ret)
		dev_err(data->dev, "failed to restart um\n");
#endif

	return ret;
}

static int exynos_devfreq_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct exynos_devfreq_data *data;
	struct dev_pm_opp *init_opp;
	unsigned long init_freq = 0;
#ifdef CONFIG_EXYNOS_DVFS_MANAGER
	int nr_constraint;
#endif

	data = kzalloc(sizeof(struct exynos_devfreq_data), GFP_KERNEL);
	if (data == NULL) {
		dev_err(&pdev->dev, "failed to allocate devfreq data\n");
		ret = -ENOMEM;
		goto err_data;
	}

	data->dev = &pdev->dev;

	mutex_init(&data->lock);

	/* parsing devfreq dts data for exynos */
	ret = exynos_devfreq_parse_dt(data->dev->of_node, data);
	if (ret) {
		dev_err(data->dev, "failed to parse private data\n");
		goto err_parse_dt;
	}

	data->devfreq_profile.max_state = data->max_state;
	data->devfreq_profile.target = exynos_devfreq_target;
	/* Register device tracing for ALT-DVFS */
	if (data->use_get_dev)
		register_get_dev_status(data);
	if (data->gov_type == SIMPLE_INTERACTIVE) {
		data->simple_interactive_data.pm_qos_class = data->pm_qos_class;
		data->simple_interactive_data.pm_qos_class_max = data->pm_qos_class_max;
		data->governor_data = &data->simple_interactive_data;
	}

	data->devfreq_profile.freq_table = kzalloc(sizeof(*(data->devfreq_profile.freq_table)) * data->max_state, GFP_KERNEL);
	if (data->devfreq_profile.freq_table == NULL) {
		dev_err(data->dev, "failed to allocate for freq_table\n");
		ret = -ENOMEM;
		goto err_freqtable;
	}

	ret = exynos_init_freq_table(data);
	if (ret) {
		dev_err(data->dev, "failed initailize freq_table\n");
		goto err_init_table;
	}

	devfreq_data[data->devfreq_type] = data;
	platform_set_drvdata(pdev, data);

	data->old_freq = (u32)data->devfreq_profile.initial_freq;
	data->old_idx = exynos_devfreq_get_opp_idx(data->opp_list, data->max_state, data->old_freq);
	if (data->old_idx < 0) {
		ret = -EINVAL;
		goto err_old_idx;
	}

	init_freq = (unsigned long)data->old_freq;
	init_opp = devfreq_recommended_opp(data->dev, &init_freq, 0);
	if (IS_ERR(init_opp)) {
		dev_err(data->dev, "not found valid OPP table for sync\n");
		ret = PTR_ERR(init_opp);
		goto err_get_opp;
	}
	data->new_volt = (u32)dev_pm_opp_get_voltage(init_opp);
	dev_pm_opp_put(init_opp);

	dev_info(data->dev, "Initial Frequency: %ld, Initial Voltage: %d\n", init_freq,
		 data->new_volt);

	data->old_volt = data->new_volt;
#ifdef CONFIG_EXYNOS_DVFS_MANAGER
	ret = exynos_dm_data_init(data->dm_type, data, data->min_freq, data->max_freq, data->old_freq);
	if (ret) {
		dev_err(data->dev, "failed DVFS Manager data init\n");
		goto err_dm_data_init;
	}

	for (nr_constraint = 0; nr_constraint < data->nr_constraint; nr_constraint++) {
		if(data->constraint[nr_constraint]) {
			ret = register_exynos_dm_constraint_table(data->dm_type,
				data->constraint[nr_constraint]);
			if (ret) {
				dev_err(data->dev,"failed registration constraint table(%d)\n",
					nr_constraint);
				goto err_dm_table;
			}
		}
	}
#endif
	/* This flag guarantees initial frequency during boot time */
	data->devfreq_disabled = true;

	data->devfreq = devfreq_add_device(data->dev, &data->devfreq_profile,
					   data->governor_name, data->governor_data);
	if (IS_ERR(data->devfreq)) {
		dev_err(data->dev, "failed devfreq device added\n");
		ret = -EINVAL;
		goto err_devfreq;
	}

	data->devfreq->min_freq = data->min_freq;
	data->devfreq->max_freq = data->max_freq;

	pm_qos_add_request(&data->sys_pm_qos_min, (int)data->pm_qos_class, data->min_freq);
#ifdef CONFIG_ARM_EXYNOS_DEVFREQ_DEBUG
	pm_qos_add_request(&data->debug_pm_qos_min, (int)data->pm_qos_class, data->min_freq);
	pm_qos_add_request(&data->debug_pm_qos_max, (int)data->pm_qos_class_max, data->max_freq);
#endif
	if (data->pm_qos_class_max)
		pm_qos_add_request(&data->default_pm_qos_max, (int)data->pm_qos_class_max,
				   data->max_freq);
	pm_qos_add_request(&data->default_pm_qos_min, (int)data->pm_qos_class, data->default_qos);
	pm_qos_add_request(&data->boot_pm_qos, (int)data->pm_qos_class,
			   data->devfreq_profile.initial_freq);

	/* Initialize ALT-DVFS */
#if defined(CONFIG_EXYNOS_ALT_DVFS)
	if (data->use_get_dev) {
		/* if polling_ms is 0, update_devfreq function is called by um */
		if (data->devfreq_profile.polling_ms == 0) {
			data->um_nb = kzalloc(sizeof(struct devfreq_notifier_block), GFP_KERNEL);
			if (data->um_nb == NULL) {
				dev_err(data->dev, "failed to allocate notifier block\n");
				ret = -ENOMEM;
				goto err_um_nb;
			}

			data->um_nb->df = data->devfreq;
			data->um_nb->nb.notifier_call = exynos_devfreq_notifier;
			exynos_alt_register_notifier(&data->um_nb->nb);
			data->last_monitor_time = sched_clock();
		}

		/*
		 * UM data should be register.
		 * And if polling_ms is 0, um notifier should be register in callback.
		 */
		ret = exynos_devfreq_um_init(data);
		if (ret) {
			dev_err(data->dev, "failed register um\n");
			goto err_um;
		}
	}

#endif
	ret = devfreq_register_opp_notifier(data->dev, data->devfreq);
	if (ret) {
		dev_err(data->dev, "failed register opp notifier\n");
		goto err_opp_noti;
	}

	data->reboot_notifier.notifier_call = exynos_devfreq_reboot_notifier;
	ret = register_reboot_notifier(&data->reboot_notifier);
	if (ret) {
		dev_err(data->dev, "failed register reboot notifier\n");
		goto err_reboot_noti;
	}

	ret = sysfs_create_file(&data->devfreq->dev.kobj, &dev_attr_scaling_devfreq_min.attr);
	if (ret)
		dev_warn(data->dev, "failed create sysfs for devfreq pm_qos_min\n");

#ifdef CONFIG_ARM_EXYNOS_DEVFREQ_DEBUG
	ret = sysfs_create_group(&data->devfreq->dev.kobj, &exynos_devfreq_attr_group);
	if (ret)
		dev_warn(data->dev, "failed create sysfs for devfreq data\n");
#endif
	ret = sysfs_create_group(&data->devfreq->dev.kobj, &devfreq_delay_time_attr_group);
	if (ret)
		dev_warn(data->dev, "failed create sysfs for devfreq data\n");

	data->devfreq_disabled = false;

	if (!data->pm_domain) {
		/* set booting frequency during booting time */
		pm_qos_update_request_timeout(&data->boot_pm_qos, data->boot_freq,
					data->boot_qos_timeout * USEC_PER_SEC);
	} else {
		pm_runtime_enable(&pdev->dev);
		pm_runtime_get_sync(&pdev->dev);
		pm_qos_update_request(&data->boot_pm_qos, data->default_qos);
		pm_runtime_put_sync(&pdev->dev);
	}

	dev_info(data->dev, "devfreq is initialized!!\n");

	return 0;

err_reboot_noti:
	devfreq_unregister_opp_notifier(data->dev, data->devfreq);
err_opp_noti:
	pm_qos_remove_request(&data->boot_pm_qos);
	pm_qos_remove_request(&data->default_pm_qos_min);
	if (data->pm_qos_class_max)
		pm_qos_remove_request(&data->default_pm_qos_max);
#ifdef CONFIG_ARM_EXYNOS_DEVFREQ_DEBUG
	pm_qos_remove_request(&data->debug_pm_qos_min);
	pm_qos_remove_request(&data->debug_pm_qos_max);
#endif
	pm_qos_remove_request(&data->sys_pm_qos_min);
	devfreq_remove_device(data->devfreq);
#ifdef CONFIG_EXYNOS_ALT_DVFS
err_um:
	if (data->um_nb) {
		exynos_alt_unregister_notifier(&data->um_nb->nb);
		kfree(data->um_nb);
	}
err_um_nb:
#endif
err_devfreq:
#ifdef CONFIG_EXYNOS_DVFS_MANAGER
	for (; nr_constraint >= 0; nr_constraint--) {
		if (data->constraint[nr_constraint])
			unregister_exynos_dm_constraint_table(data->dm_type,
				data->constraint[nr_constraint]);
	}
err_dm_table:
err_dm_data_init:
#endif
err_get_opp:
err_old_idx:
	platform_set_drvdata(pdev, NULL);
err_init_table:
	kfree(data->devfreq_profile.freq_table);
err_freqtable:
err_parse_dt:
	mutex_destroy(&data->lock);
	kfree(data);
err_data:

	return ret;
}

static int exynos_devfreq_remove(struct platform_device *pdev)
{
	struct exynos_devfreq_data *data = platform_get_drvdata(pdev);
#ifdef CONFIG_EXYNOS_DVFS_MANAGER
	int nr_constraint;
#endif
	sysfs_remove_file(&data->devfreq->dev.kobj, &dev_attr_scaling_devfreq_min.attr);
#ifdef CONFIG_ARM_EXYNOS_DEVFREQ_DEBUG
	sysfs_remove_group(&data->devfreq->dev.kobj, &exynos_devfreq_attr_group);
#endif
	sysfs_remove_group(&data->devfreq->dev.kobj, &devfreq_delay_time_attr_group);

	unregister_reboot_notifier(&data->reboot_notifier);
	devfreq_unregister_opp_notifier(data->dev, data->devfreq);

	pm_qos_remove_request(&data->boot_pm_qos);
	pm_qos_remove_request(&data->default_pm_qos_min);
	if (data->pm_qos_class_max)
		pm_qos_remove_request(&data->default_pm_qos_max);
#ifdef CONFIG_ARM_EXYNOS_DEVFREQ_DEBUG
	pm_qos_remove_request(&data->debug_pm_qos_min);
	pm_qos_remove_request(&data->debug_pm_qos_max);
#endif
	pm_qos_remove_request(&data->sys_pm_qos_min);
#if defined(CONFIG_EXYNOS_ALT_DVFS)
	exynos_alt_unregister_notifier(&data->um_nb->nb);
	exynos_devfreq_um_exit(data);
#endif
	devfreq_remove_device(data->devfreq);
#ifdef CONFIG_EXYNOS_DVFS_MANAGER
	for (nr_constraint = 0; nr_constraint < data->nr_constraint; nr_constraint++) {
		if (data->constraint[nr_constraint])
			unregister_exynos_dm_constraint_table(data->dm_type,
				data->constraint[nr_constraint]);
	}
#endif
	platform_set_drvdata(pdev, NULL);
	kfree(data->devfreq_profile.freq_table);
	mutex_destroy(&data->lock);
	kfree(data);

	return 0;
}

static struct platform_device_id exynos_devfreq_driver_ids[] = {
	{
	 .name = EXYNOS_DEVFREQ_MODULE_NAME,
	 },
	{},
};

MODULE_DEVICE_TABLE(platform, exynos_devfreq_driver_ids);

static const struct of_device_id exynos_devfreq_match[] = {
	{
	 .compatible = "samsung,exynos-devfreq",
	 },
	{},
};

MODULE_DEVICE_TABLE(of, exynos_devfreq_match);

static const struct dev_pm_ops exynos_devfreq_pm_ops = {
	.suspend_late = exynos_devfreq_suspend,
	.resume_early = exynos_devfreq_resume,
};

static struct platform_driver exynos_devfreq_driver = {
	.probe = exynos_devfreq_probe,
	.remove = exynos_devfreq_remove,
	.id_table = exynos_devfreq_driver_ids,
	.driver = {
		.name = EXYNOS_DEVFREQ_MODULE_NAME,
		.owner = THIS_MODULE,
		.pm = &exynos_devfreq_pm_ops,
		.of_match_table = exynos_devfreq_match,
	},
};

static int exynos_devfreq_root_probe(struct platform_device *pdev)
{
	struct device_node *np;
	int num_domains;

	np = pdev->dev.of_node;

	platform_driver_register(&exynos_devfreq_driver);

	/* alloc memory for devfreq data structure */
	num_domains = of_get_child_count(np);
	devfreq_data = (struct exynos_devfreq_data **)kzalloc(sizeof(struct exynos_devfreq_data *)
			* num_domains, GFP_KERNEL);

	/* probe each devfreq node */
	of_platform_populate(np, NULL, NULL, NULL);

	return 0;
}

static const struct of_device_id exynos_devfreq_root_match[] = {
	{
		.compatible = "samsung,exynos-devfreq-root",
	},
	{},
	};

static struct platform_driver exynos_devfreq_root_driver = {
	.probe = exynos_devfreq_root_probe,
	.driver = {
		.name = "exynos-devfreq-root",
		.owner = THIS_MODULE,
		.of_match_table = exynos_devfreq_root_match,
	},
};

module_platform_driver(exynos_devfreq_root_driver);
MODULE_AUTHOR("Taekki Kim <taekki.kim@samsung.com>");
MODULE_DESCRIPTION("Samsung EXYNOS Soc series devfreq common driver");
MODULE_LICENSE("GPL");
