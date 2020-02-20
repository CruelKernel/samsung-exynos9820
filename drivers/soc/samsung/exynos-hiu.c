/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * HIU driver for HAFM(AFM with HIU but TB) support
 * Auther : PARK CHOONGHOON (choong.park@samsung.com)
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/cpumask.h>
#include <linux/regmap.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/debug-snapshot.h>

#include <soc/samsung/exynos-cpupm.h>
#include <soc/samsung/cal-if.h>

#include "exynos-hiu.h"
#include "../../cpufreq/exynos-ff.h"
#include "../../cpufreq/exynos-acme.h"

static struct exynos_hiu_data *data;
atomic_t hiu_normdvfs_req = ATOMIC_INIT(0);

static void hiu_stats_create_table(struct cpufreq_policy *policy);

#define POLL_PERIOD 100

/****************************************************************/
/*			HIU HELPER FUNCTION			*/
/****************************************************************/
static unsigned int hiu_get_freq_level(unsigned int freq)
{
	int level;
	struct hiu_stats *stats = data->stats;

	if (unlikely(!stats))
		return 0;

	for (level = 0; level < stats->last_level; level++)
		if (stats->freq_table[level] == freq)
			return level + data->level_offset;

	return -EINVAL;
}

static unsigned int hiu_get_power_budget(unsigned int freq)
{
	/* SW deliver fixed value as power budget */
	return data->sw_pbl;
}

static void hiu_update_reg(int offset, int mask, int shift, unsigned int val)
{
	unsigned int reg_val;

	reg_val = __raw_readl(data->base + offset);
	reg_val &= ~(mask << shift);
	reg_val |= val << shift;
	__raw_writel(reg_val, data->base + offset);
}

static unsigned int hiu_read_reg(int offset, int mask, int shift)
{
	unsigned int reg_val;

	reg_val = __raw_readl(data->base + offset);
	return (reg_val >> shift) & mask;
}

static unsigned int hiu_get_act_dvfs(void)
{
	return hiu_read_reg(HIUTOPCTL1, ACTDVFS_MASK, ACTDVFS_SHIFT);
}

static void hiu_control_err_interrupts(int enable)
{
	if (enable)
		hiu_update_reg(HIUTOPCTL1, ENB_ERR_INTERRUPTS_MASK, 0, ENB_ERR_INTERRUPTS_MASK);
	else
		hiu_update_reg(HIUTOPCTL1, ENB_ERR_INTERRUPTS_MASK, 0, 0);
}

static void hiu_control_mailbox(int enable)
{
	hiu_update_reg(HIUTOPCTL1, ENB_SR1INTR_MASK, ENB_SR1INTR_SHIFT, !!enable);
	hiu_update_reg(HIUTOPCTL1, ENB_ACPM_COMM_MASK, ENB_ACPM_COMM_SHIFT, !!enable);
}

static void hiu_set_limit_dvfs(unsigned int freq)
{
	unsigned int level;

	level = hiu_get_freq_level(freq);

	hiu_update_reg(HIUTOPCTL2, LIMITDVFS_MASK, LIMITDVFS_SHIFT, level);
}

static void hiu_set_tb_dvfs(unsigned int freq)
{
	unsigned int level;

	level = hiu_get_freq_level(freq);

	hiu_update_reg(HIUTBCTL, TBDVFS_MASK, TBDVFS_SHIFT, level);
}

static void hiu_control_tb(int enable)
{
	hiu_update_reg(HIUTBCTL, TB_ENB_MASK, TB_ENB_SHIFT, !!enable);
}

static void hiu_control_pc(int enable)
{
	hiu_update_reg(HIUTBCTL, PC_DISABLE_MASK, PC_DISABLE_SHIFT, !enable);
}

static void hiu_set_boost_level_inc(void)
{
	unsigned int inc;
	struct device_node *dn = data->dn;

	if (!of_property_read_u32(dn, "bl1-inc", &inc))
		hiu_update_reg(HIUTBCTL, B1_INC_MASK, B1_INC_SHIFT, inc);
	if (!of_property_read_u32(dn, "bl2-inc", &inc))
		hiu_update_reg(HIUTBCTL, B2_INC_MASK, B2_INC_SHIFT, inc);
	if (!of_property_read_u32(dn, "bl3-inc", &inc))
		hiu_update_reg(HIUTBCTL, B3_INC_MASK, B3_INC_SHIFT, inc);
}

static void hiu_set_tb_ps_cfg_each(int index, unsigned int cfg_val)
{
	int offset;

	offset = HIUTBPSCFG_BASE + index * HIUTBPSCFG_OFFSET;
	hiu_update_reg(offset, HIUTBPSCFG_MASK, 0, cfg_val);
}

static int hiu_set_tb_ps_cfg(void)
{
	int size, index;
	unsigned int val;
	struct hiu_cfg *table;
	struct device_node *dn = data->dn;

	size = of_property_count_u32_elems(dn, "config-table");
	if (size < 0)
		return size;

	table = kzalloc(sizeof(struct hiu_cfg) * size / 4, GFP_KERNEL);
	if (!table)
		return -ENOMEM;

	of_property_read_u32_array(dn, "config-table", (unsigned int *)table, size);

	for (index = 0; index < size / 4; index++) {
		val = 0;
		val |= table[index].power_borrowed << PB_SHIFT;
		val |= table[index].boost_level << BL_SHIFT;
		val |= table[index].power_budget_limit << PBL_SHIFT;
		val |= table[index].power_threshold_inc << TBPWRTHRESH_INC_SHIFT;

		hiu_set_tb_ps_cfg_each(index, val);
	}

	kfree(table);

	return 0;
}

static void control_hiu_sr1_irq_pending(int enable)
{
	hiu_update_reg(GCUCTL, HIUINTR_EN_MASK, HIUINTR_EN_SHIFT, enable);
}

static bool check_hiu_sr1_irq_pending(void)
{
	return !!hiu_read_reg(HIUTOPCTL1, HIU_MBOX_RESPONSE_MASK, SR1INTR_SHIFT);
}

static void clear_hiu_sr1_irq_pending(void)
{
	hiu_update_reg(HIUTOPCTL1, HIU_MBOX_RESPONSE_MASK, SR1INTR_SHIFT, 0);
}

static void control_hiu_mailbox_err_pending(int enable)
{
	hiu_update_reg(GCUCTL, HIUERR_EN_MASK, HIUERR_EN_SHIFT, enable);
}

static bool check_hiu_mailbox_err_pending(void)
{
	return !!hiu_read_reg(HIUTOPCTL1, HIU_MBOX_ERR_MASK, HIU_MBOX_ERR_SHIFT);
}

static unsigned int get_hiu_mailbox_err(void)
{
	return hiu_read_reg(HIUTOPCTL1, HIU_MBOX_ERR_MASK, HIU_MBOX_ERR_SHIFT);
}

static void hiu_mailbox_err_handler(void)
{
	unsigned int err, val;

	err = get_hiu_mailbox_err();

	if (err & SR1UXPERR_MASK)
		pr_err("exynos-hiu: unexpected error occurs\n");

	if (err & SR1SNERR_MASK) {
		val = __raw_readl(data->base + HIUTOPCTL2);
		val = (val >> SEQNUM_SHIFT) & SEQNUM_MASK;
		pr_err("exynos-hiu: erroneous sequence num %d\n", val);
	}

	if (err & SR1TIMEOUT_MASK)
		pr_err("exynos-hiu: TIMEOUT on SR1 write\n");

	if (err & SR0RDERR_MASK)
		pr_err("exynos-hiu: SR0 read twice or more\n");
}

static bool check_hiu_req_freq_updated(unsigned int req_freq)
{
	unsigned int cur_level, cur_freq;

	cur_level = hiu_get_act_dvfs();
	cur_freq = data->stats->freq_table[cur_level - data->level_offset];

	return cur_freq == req_freq;
}

static bool check_hiu_normal_req_done(unsigned int req_freq)
{
	return check_hiu_sr1_irq_pending() &&
		check_hiu_req_freq_updated(req_freq);
}

static bool check_hiu_need_register_restore(void)
{
	return !hiu_read_reg(HIUTOPCTL1, ENB_SR1INTR_MASK, ENB_SR1INTR_SHIFT);
}

static int request_dvfs_on_sr0(unsigned int req_freq)
{
	unsigned int val, level, budget;

	/* Get dvfs level */
	level = hiu_get_freq_level(req_freq);
	data->last_req_level = level;
	data->last_req_freq = req_freq;
	if (level < 0)
		return -EINVAL;

	/* Get power budget */
	budget = hiu_get_power_budget(req_freq);
	if (budget < 0)
		return -EINVAL;

	/* write REQDVFS & REQPBL to HIU SFR */
	val = __raw_readl(data->base + HIUTOPCTL2);
	val &= ~(REQDVFS_MASK << REQDVFS_SHIFT | REQPBL_MASK << REQPBL_SHIFT);
	val |= (level << REQDVFS_SHIFT | budget << REQPBL_SHIFT);
	__raw_writel(val, data->base + HIUTOPCTL2);

	return 0;
}

static void hiu_set_bl_tbpwr_threshold_each(int level)
{
	char buffer[20];
	unsigned int val, offset;
	unsigned int table[HIUTBPWRTHRESH_FIELDS];

	snprintf(buffer, 20, "bl%d-tbpwr-threshold", level);

	if (of_property_read_u32_array(data->dn, buffer, table, HIUTBPWRTHRESH_FIELDS))
		return;

	val = 0;
	val |= table[0] << R_SHIFT;
	val |= table[1] << MONINTERVAL_SHIFT;
	val |= table[2] << TBPWRTHRESH_EXP_SHIFT;
	val |= table[3] << TBPWRTHRESH_FRAC_SHIFT;

	offset = HIUTBPWRTHRESH_BASE + (level - 1) * HIUTBPWRTHRESH_OFFSET;
	hiu_update_reg(offset, HIUTBPWRTHRESH_MASK, 0, val);
}

static int hiu_set_bl_tbpwr_thresholds(void)
{
	int index;

	for (index = 0; index < HIUTBPWRTHRESH_NUM; index++)
		hiu_set_bl_tbpwr_threshold_each(index + 1);

	return 0;
}

/****************************************************************/
/*			     HIU API				*/
/****************************************************************/
static void __exynos_hiu_update_data(struct cpufreq_policy *policy);
int exynos_hiu_set_freq(unsigned int id, unsigned int req_freq)
{
	bool cpd_blocked_changed = false;

	if (unlikely(!data))
		return -ENODEV;

	if (!data->enabled)
		return -ENODEV;

	if (check_hiu_need_register_restore())
		__exynos_hiu_update_data(NULL);

	/*
	 * If HIU H/W communicates with ACPM depending on middle core's p-state,
	 * there might be ITMON error due to big core's cpd and releated register access at the same time
	 * These codes below prevent that case.
	 */
	if (req_freq >= data->boost_threshold && !data->cpd_blocked) {
		data->cpd_blocked = true;
		cpd_blocked_changed = true;

		disable_power_mode(cpumask_any(&data->cpus), POWERMODE_TYPE_CLUSTER);
	} else if (req_freq < data->boost_threshold && data->cpd_blocked) {
		data->cpd_blocked = false;
		cpd_blocked_changed = true;

		enable_power_mode(cpumask_any(&data->cpus), POWERMODE_TYPE_CLUSTER);
	}

	/* In interrupt mode, need to set normal dvfs request flag */
	if (data->operation_mode == INTERRUPT_MODE)
		atomic_inc(&hiu_normdvfs_req);

	/* Write req_freq on SR0 to request DVFS */
	if (request_dvfs_on_sr0(req_freq))
		goto fail_request_on_sr0;

	dbg_snapshot_printk("HIU_enter:%u->%u\n", data->cur_freq, req_freq);
	if (data->operation_mode == POLLING_MODE) {
		while (!check_hiu_normal_req_done(req_freq) &&
			!check_hiu_mailbox_err_pending())
			usleep_range(POLL_PERIOD, 2 * POLL_PERIOD);

		if (check_hiu_mailbox_err_pending()) {
			hiu_mailbox_err_handler();
			BUG_ON(1);
		}

		clear_hiu_sr1_irq_pending();
	} else if (data->operation_mode == INTERRUPT_MODE) {
		wait_event(data->normdvfs_wait, data->normdvfs_done);

		data->normdvfs_done = false;
	}
	dbg_snapshot_printk("HIU_exit:%u->%u\n", data->cur_freq, req_freq);

	data->cur_freq = req_freq;

	pr_debug("exynos-hiu: set REQDVFS to HIU : %ukHz\n", req_freq);

	return 0;

fail_request_on_sr0:
	if (cpd_blocked_changed) {
		if (data->cpd_blocked)
			enable_power_mode(cpumask_any(&data->cpus), POWERMODE_TYPE_CLUSTER);
		else
			disable_power_mode(cpumask_any(&data->cpus), POWERMODE_TYPE_CLUSTER);

		data->cpd_blocked = !data->cpd_blocked;
	}

	dbg_snapshot_printk("HIU_request_on_sr0_fail:%ukHz\n", req_freq);

	return -EIO;
}

int exynos_hiu_get_freq(unsigned int id)
{
	if (unlikely(!data))
		return -ENODEV;

	return data->cur_freq;
}

int exynos_hiu_get_max_freq(void)
{
	if (unlikely(!data))
		return -1;

	return data->clipped_freq;
}

unsigned int exynos_pstate_get_boost_freq(int cpu)
{
	if (unlikely(!data))
		return 0;

	if (!cpumask_test_cpu(cpu, &data->cpus))
		return 0;

	return data->boost_max;
}

/****************************************************************/
/*			HIU SR1 WRITE HANDLER			*/
/****************************************************************/
#define SWI_ENABLE	(1)
#define SWI_DISABLE	(0)

static bool check_hiu_act_freq_changed(void)
{
	unsigned int act_freq, level;
	struct hiu_stats *stats = data->stats;

	level = hiu_get_act_dvfs();
	act_freq = stats->freq_table[level - data->level_offset];

	return data->cur_freq != act_freq;
}

static void exynos_hiu_work(struct work_struct *work)
{
	/* To do if needed */
	while (!check_hiu_act_freq_changed())
		usleep_range(POLL_PERIOD, 2 * POLL_PERIOD);

	data->normdvfs_done = true;
	wake_up(&data->normdvfs_wait);
}

static void exynos_hiu_hwi_work(struct work_struct *work)
{
	/* To do if needed */
}

static irqreturn_t exynos_hiu_irq_handler(int irq, void *id)
{
	irqreturn_t ret = IRQ_NONE;

	if (!check_hiu_sr1_irq_pending() && !check_hiu_mailbox_err_pending())
		return ret;

	if (check_hiu_mailbox_err_pending()) {
		hiu_mailbox_err_handler();
		BUG_ON(1);
	}

	ret = IRQ_HANDLED;

	if (atomic_read(&hiu_normdvfs_req)) {
		atomic_dec(&hiu_normdvfs_req);
		schedule_work_on(cpumask_any(cpu_coregroup_mask(0)), &data->work);
	}

	clear_hiu_sr1_irq_pending();

	return ret;
}

/****************************************************************/
/*			EXTERNAL EVENT HANDLER			*/
/****************************************************************/
static void hiu_dummy_request(unsigned int dummy_freq)
{
	if (data->operation_mode == INTERRUPT_MODE) {
		mutex_lock(&data->lock);
		data->normdvfs_req = true;
	}

	request_dvfs_on_sr0(dummy_freq);

	if (data->operation_mode == POLLING_MODE) {
		while (!check_hiu_normal_req_done(dummy_freq) &&
			!check_hiu_mailbox_err_pending())
			usleep_range(POLL_PERIOD, 2 * POLL_PERIOD);

		if (check_hiu_mailbox_err_pending()) {
			hiu_mailbox_err_handler();
			BUG_ON(1);
		}

		clear_hiu_sr1_irq_pending();

	} else if (data->operation_mode == INTERRUPT_MODE) {
		mutex_unlock(&data->lock);
		wait_event(data->normdvfs_wait, data->normdvfs_done);

		data->normdvfs_req = false;
		data->normdvfs_done = false;
	}
}

static void __exynos_hiu_update_data(struct cpufreq_policy *policy)
{
	/* Set dvfs limit and TB threshold */
	hiu_set_limit_dvfs(data->clipped_freq);
	hiu_set_tb_dvfs(data->boost_threshold);

	/* Initialize TB level offset */
	hiu_set_boost_level_inc();

	/* Initialize TB power state config */
	hiu_set_tb_ps_cfg();

	/* Enable TB */
	hiu_control_pc(data->pc_enabled);
	hiu_control_tb(data->tb_enabled);

	if (data->pc_enabled)
		hiu_set_bl_tbpwr_thresholds();

	/* Enable error interrupts */
	hiu_control_err_interrupts(1);
	/* Enable mailbox communication with ACPM */
	hiu_control_mailbox(1);

	if (data->operation_mode == INTERRUPT_MODE) {
		control_hiu_sr1_irq_pending(SWI_ENABLE);
		control_hiu_mailbox_err_pending(SWI_ENABLE);
	}

	/*
	 * This request is dummy to give plugin threshold
	 * when hiu driver is not enabled; it is only called when cpufreq driver is registered.
	 * This request is not to change real frequency
	 */
	if (!data->enabled) {
		hiu_dummy_request(data->boost_threshold);
		hiu_dummy_request(data->cur_freq);
	}
}

static int exynos_hiu_update_data(struct cpufreq_policy *policy)
{
	if (!cpumask_test_cpu(data->cpu, policy->cpus))
		return 0;

	if (data->enabled)
		return 0;

	data->clipped_freq = data->boost_max;
	hiu_stats_create_table(policy);

	__exynos_hiu_update_data(policy);

	data->enabled = true;

	pr_info("exynos-hiu: HIU data structure update complete\n");

	return 0;
}

static struct exynos_cpufreq_ready_block exynos_hiu_ready = {
	.update = exynos_hiu_update_data,
};

/****************************************************************/
/*			SYSFS INTERFACE				*/
/****************************************************************/
static ssize_t
hiu_enable_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", data->enabled);
}

static ssize_t
hiu_enable_store(struct device *dev, struct device_attribute *devattr,
			const char *buf, size_t count)
{
	unsigned int input;

	if (kstrtos32(buf, 10, &input))
		return -EINVAL;

	return count;
}

static ssize_t
hiu_boosted_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	unsigned int boosted = hiu_read_reg(HIUTBCTL, BOOSTED_MASK, BOOSTED_SHIFT);

	return snprintf(buf, PAGE_SIZE, "%d\n", boosted);
}

static ssize_t
hiu_boost_threshold_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", data->boost_threshold);
}

static ssize_t
hiu_dvfs_limit_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	unsigned int dvfs_limit = hiu_read_reg(HIUTOPCTL2, LIMITDVFS_MASK, LIMITDVFS_SHIFT);

	return snprintf(buf, PAGE_SIZE, "%d\n", dvfs_limit);
}

static ssize_t
hiu_dvfs_limit_store(struct device *dev, struct device_attribute *devattr,
			const char *buf, size_t count)
{
	unsigned int input;

	if (kstrtos32(buf, 10, &input))
		return -EINVAL;

	hiu_update_reg(HIUTOPCTL2, LIMITDVFS_MASK, LIMITDVFS_SHIFT, input);

	return count;
}

static ssize_t
hiu_pb_enabled_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s sw-pbl:%x\n",
				data->tb_enabled?"enabled":"disabled", data->sw_pbl);
}

static DEVICE_ATTR(enabled, 0644, hiu_enable_show, hiu_enable_store);
static DEVICE_ATTR(boosted, 0444, hiu_boosted_show, NULL);
static DEVICE_ATTR(boost_threshold, 0444, hiu_boost_threshold_show, NULL);
static DEVICE_ATTR(dvfs_limit, 0644, hiu_dvfs_limit_show, hiu_dvfs_limit_store);
static DEVICE_ATTR(pb_enabled, 0444, hiu_pb_enabled_show, NULL);

static struct attribute *exynos_hiu_attrs[] = {
	&dev_attr_enabled.attr,
	&dev_attr_boosted.attr,
	&dev_attr_boost_threshold.attr,
	&dev_attr_dvfs_limit.attr,
	&dev_attr_pb_enabled.attr,
	NULL,
};

static struct attribute_group exynos_hiu_attr_group = {
	.name = "hiu",
	.attrs = exynos_hiu_attrs,
};

/****************************************************************/
/*		INITIALIZE EXYNOS HIU DRIVER			*/
/****************************************************************/
static int hiu_dt_parsing(struct device_node *dn)
{
	const char *buf;
	unsigned int val;
	int ret = 0;

	ret |= of_property_read_u32(dn, "operation-mode", &data->operation_mode);
	ret |= of_property_read_u32(dn, "boot-freq", &data->cur_freq);
	ret |= of_property_read_u32(dn, "boost-threshold", &data->boost_threshold);
	ret |= of_property_read_u32(dn, "sw-pbl", &data->sw_pbl);
	ret |= of_property_read_string(dn, "sibling-cpus", &buf);
	if (ret)
		return ret;

	if (!of_property_read_u32(dn, "cal-id", &data->cal_id))
		val = cal_dfs_get_max_freq(data->cal_id);
	if (of_property_read_u32(dn, "boost-max", &data->boost_max))
		data->boost_max = UINT_MAX;
	data->boost_max = min(val, data->boost_max);
	if (data->boost_max == UINT_MAX)
		return -ENODEV;

	if (of_property_read_u32(dn, "pc-enabled", &data->pc_enabled))
		data->pc_enabled = 0;

	if (of_property_read_u32(dn, "tb-enabled", &data->tb_enabled))
		data->tb_enabled = 0;

	cpulist_parse(buf, &data->cpus);
	cpumask_and(&data->cpus, &data->cpus, cpu_possible_mask);
	cpumask_and(&data->cpus, &data->cpus, cpu_online_mask);
	if (cpumask_weight(&data->cpus) == 0)
		return -ENODEV;

	data->cpu = cpumask_first(&data->cpus);

	return 0;
}

static void hiu_stats_create_table(struct cpufreq_policy *policy)
{
	unsigned int i = 0, count = 0, alloc_size;
	struct hiu_stats *stats;
	struct cpufreq_frequency_table *pos, *table;

	table = policy->freq_table;
	if (unlikely(!table))
		return;

	stats = kzalloc(sizeof(*stats), GFP_KERNEL);
	if (!stats)
		return;

	cpufreq_for_each_valid_entry(pos, table)
		count++;

	alloc_size = count * (sizeof(unsigned int) + sizeof(u64));

	stats->freq_table = kzalloc(alloc_size, GFP_KERNEL);
	if (!stats->freq_table)
		goto free_stat;

	stats->time_in_state = (unsigned long long *)(stats->freq_table + count);

	stats->last_level = count;

	cpufreq_for_each_valid_entry(pos, table)
		stats->freq_table[i++] = pos->frequency;

	data->stats = stats;

	cpufreq_for_each_valid_entry(pos, table) {
		data->level_offset = pos->driver_data;
		break;
	}

	return;
free_stat:
	kfree(stats);
}

static int exynos_hiu_probe(struct platform_device *pdev)
{
	struct device_node *dn = pdev->dev.of_node;
	int ret;

	data = kzalloc(sizeof(struct exynos_hiu_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->enabled = false;

	mutex_init(&data->lock);

	platform_set_drvdata(pdev, data);

	data->base = ioremap(GCU_BASE, SZ_4K);

	ret = hiu_dt_parsing(dn);
	if (ret) {
		dev_err(&pdev->dev, "Failed to parse HIU data\n");
		goto free_data;
	}

	data->dn = dn;

	if (data->operation_mode == INTERRUPT_MODE) {
		init_waitqueue_head(&data->normdvfs_wait);
		data->normdvfs_done = false;

		data->irq = irq_of_parse_and_map(dn, 0);
		if (data->irq <= 0) {
			dev_err(&pdev->dev, "Failed to get IRQ\n");
			goto free_data;
		}

		ret = devm_request_irq(&pdev->dev, data->irq, exynos_hiu_irq_handler,
					IRQF_TRIGGER_HIGH | IRQF_SHARED, dev_name(&pdev->dev), data);

		if (ret) {
			dev_err(&pdev->dev, "Failed to request IRQ handler: %d\n", data->irq);
			goto free_data;
		}
	}

	INIT_WORK(&data->work, exynos_hiu_work);
	INIT_WORK(&data->hwi_work, exynos_hiu_hwi_work);

	ret = sysfs_create_group(&pdev->dev.kobj, &exynos_hiu_attr_group);
	if (ret)
		dev_err(&pdev->dev, "Failed to create Exynos HIU attr group");

	exynos_cpufreq_ready_list_add(&exynos_hiu_ready);

	dev_info(&pdev->dev, "HIU Handler initialization complete\n");
	return 0;

free_data:
	kfree(data);
	data = NULL;

	return -ENODEV;
}

static const struct of_device_id of_exynos_hiu_match[] = {
	{ .compatible = "samsung,exynos-hiu", },
	{ },
};

static const struct platform_device_id exynos_hiu_ids[] = {
	{ "exynos-hiu", },
	{ }
};

static struct platform_driver exynos_hiu_driver = {
	.driver = {
		.name = "exynos-hiu",
		.owner = THIS_MODULE,
		.of_match_table = of_exynos_hiu_match,
	},
	.probe		= exynos_hiu_probe,
	.id_table	= exynos_hiu_ids,
};

int __init exynos_hiu_init(void)
{
	return platform_driver_register(&exynos_hiu_driver);
}
device_initcall(exynos_hiu_init);
