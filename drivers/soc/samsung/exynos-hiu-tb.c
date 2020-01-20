/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * HIU driver for HAFM-TB(AFM with HIU TB) support
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

#include <soc/samsung/exynos-cpupm.h>

#include "exynos-hiu.h"
#include "../../cpufreq/exynos-ff.h"
#include "../../cpufreq/exynos-acme.h"

static struct exynos_hiu_data *data;

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

static bool check_hiu_boosted(void)
{
	return !!hiu_read_reg(HIUTBCTL, BOOSTED_MASK, BOOSTED_SHIFT);
}

static void clear_hiu_boosted(void)
{
	hiu_update_reg(HIUTBCTL, BOOSTED_MASK, BOOSTED_SHIFT, 0);
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

	/*
	 * If req_freq == boost_threshold, HIU could request turbo boost
	 * That's why in case of req_freq == boost_threshold,
	 * requested frequency update is consdered as done,
	 * if act_dvfs is larger than or equal to boost threshold
	 */
	if (req_freq == data->boost_threshold)
		return cur_freq >= data->boost_threshold;

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
static int hiu_sr1_check_loop(void *unused);
static void __exynos_hiu_update_data(struct cpufreq_policy *policy);

int exynos_hiu_set_freq(unsigned int id, unsigned int req_freq)
{
	bool need_update_cur_freq = true;

	if (unlikely(!data))
		return -ENODEV;

	if (!data->enabled)
		return -ENODEV;

	pr_debug("exynos-hiu: update data->cur_freq:%d to %d\n", data->cur_freq, req_freq);

	/*
	 * If HIU H/W communicates with ACPM depending on middle core's p-state,
	 * there might be ITMON error due to big core's cpd and releated register access at the same time
	 * These codes prevent that case.
	 */
        if (req_freq >= data->boost_threshold && !data->cpd_blocked) {
                data->cpd_blocked = true;

                disable_power_mode(cpumask_any(&data->cpus), POWERMODE_TYPE_CLUSTER);
        }
        else if (req_freq < data->boost_threshold && data->cpd_blocked) {
                data->cpd_blocked = false;

                enable_power_mode(cpumask_any(&data->cpus), POWERMODE_TYPE_CLUSTER);
        }

	mutex_lock(&data->lock);

	if (check_hiu_need_register_restore())
		__exynos_hiu_update_data(NULL);

	/* PM QoS could make req_freq bigger than boost_threshold */
	if (req_freq >= data->boost_threshold) {
		/*
		 * 1) If turbo boost is already activated
		 *    just update cur_freq and return.
		 * 2) If not, req_freq should be boost_threshold;
		 *    DO NOT allow req_freq to be bigger than boost_threshold.
		 */
		if (data->cur_freq >= data->boost_threshold) {
			data->cur_freq = req_freq;
			mutex_unlock(&data->lock);
			return 0;
		}
		else {
			data->cur_freq = req_freq;
			req_freq = data->boost_threshold;
			need_update_cur_freq = false;
		}
	}

	/* In interrupt mode, need to set normal dvfs request flag */
	if (data->operation_mode == INTERRUPT_MODE) {
		data->normdvfs_req = true;
		data->norm_target_freq = req_freq;
	}

	/* Write req_freq on SR0 to request DVFS */
	request_dvfs_on_sr0(req_freq);

	if (data->operation_mode == POLLING_MODE) {
		while (!check_hiu_normal_req_done(req_freq) &&
			!check_hiu_mailbox_err_pending())
			usleep_range(POLL_PERIOD, 2 * POLL_PERIOD);

		if (check_hiu_mailbox_err_pending()) {
			hiu_mailbox_err_handler();
			BUG_ON(1);
		}

		if (need_update_cur_freq)
			data->cur_freq = req_freq;
		clear_hiu_sr1_irq_pending();

		if (req_freq == data->boost_threshold && !data->boosting_activated) {
			data->boosting_activated = true;
			wake_up(&data->polling_wait);
		}

		mutex_unlock(&data->lock);

	} else if (data->operation_mode == INTERRUPT_MODE) {
		mutex_unlock(&data->lock);
		wait_event(data->normdvfs_wait, data->normdvfs_done);

		data->normdvfs_done = false;

		if (need_update_cur_freq)
			data->cur_freq = req_freq;
	}

	pr_debug("exynos-hiu: set REQDVFS to HIU : %ukHz\n", req_freq);

	return 0;
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
	if (!cpumask_test_cpu(cpu, &data->cpus))
		return 0;

	return data->boost_max;
}

void (*exynos_ocp_irq_handling_on_behalf)(void);
void exynos_register_ocp_irq_callback(void (*callback)(void))
{
	exynos_ocp_irq_handling_on_behalf = callback;
}

/****************************************************************/
/*			HIU SR1 WRITE HANDLER			*/
/****************************************************************/
#define SWI_ENABLE	(1)
#define SWI_DISABLE	(0)

static int hiu_select_scaling_cpu_for_hwidvfs(void)
{
	int cpu;
	cpumask_t mask;

	cpumask_clear(&mask);
	cpumask_and(&mask, cpu_coregroup_mask(0), cpu_online_mask);

	/* Idle core of the boot cluster is selected to scaling cpu */
	for_each_cpu(cpu, &mask)
		if (idle_cpu(cpu))
			return cpu;

	/* if panic_cpu is not Little core, mask will be empty */
	if (unlikely(!cpumask_weight(&mask))) {
		cpu = atomic_read(&panic_cpu);
		if (cpu != PANIC_CPU_INVALID)
			return cpu;
	}

	return cpumask_any(&mask);
}

static void exynos_hiu_work(struct work_struct *work)
{
	int cpu;

	mutex_lock(&data->lock);
	control_hiu_sr1_irq_pending(SWI_DISABLE);
	control_hiu_mailbox_err_pending(SWI_DISABLE);

	/* If HIU mailbox error occurs, raise kernel panic. */
	if (check_hiu_mailbox_err_pending()) {
		hiu_mailbox_err_handler();
		BUG_ON(1);
	}

	/*
	 * There could be sr1 irq pending in hafm due to
	 * 1) normal dvfs request
	 * 2) HIU hwidvfs request (Turbo boost)
	 */
	if (!data->normdvfs_req) {
		clear_hiu_sr1_irq_pending();

		if (check_hiu_boosted()) {
			cpu = hiu_select_scaling_cpu_for_hwidvfs();

			if (cpu < 0)
				cpu = cpumask_any(cpu_coregroup_mask(0));

			schedule_work_on(cpu, &data->hwi_work);
			clear_hiu_boosted();
		}
	} else if (check_hiu_req_freq_updated(data->norm_target_freq)) {
		data->normdvfs_req = false;
		data->normdvfs_done = true;
		clear_hiu_sr1_irq_pending();
		wake_up(&data->normdvfs_wait);
	}

	control_hiu_sr1_irq_pending(SWI_ENABLE);
	control_hiu_mailbox_err_pending(SWI_ENABLE);
	mutex_unlock(&data->lock);
}

static void exynos_hiu_hwi_work(struct work_struct *work)
{
	unsigned int boost_freq, level;
	struct cpufreq_policy *policy;
	struct hiu_stats *stats = data->stats;

	level = hiu_get_act_dvfs();
	boost_freq = stats->freq_table[level - data->level_offset];

	/*
	 * Only when TB bit is set, this work callback is called.
	 * However, while this callback is waiting to start,
	 * well... turbo boost could be released.
	 * So, acting frequency coulde be lower than turbo boost threshold.
	 * This condition code is for treating that case.
	 */
	if (boost_freq < data->boost_threshold)
		goto done;

	policy = cpufreq_cpu_get(cpumask_first(&data->cpus));
	if (!policy) {
		pr_debug("Failed to get CPUFreq policy in HIU work\n");
		goto done;
	}

	__cpufreq_driver_target(policy, boost_freq,
                                CPUFREQ_RELATION_H | CPUFREQ_HW_DVFS_REQ);

	cpufreq_cpu_put(policy);
done:
	if (data->operation_mode == POLLING_MODE) {
		data->hwidvfs_done = true;
		wake_up(&data->hwidvfs_wait);
	}
}

static irqreturn_t exynos_hiu_irq_handler(int irq, void *id)
{
	if (check_hiu_sr1_irq_pending() || check_hiu_mailbox_err_pending())
		schedule_work_on(cpumask_any(cpu_coregroup_mask(0)), &data->work);

	if (exynos_ocp_irq_handling_on_behalf)
		exynos_ocp_irq_handling_on_behalf();

	return IRQ_HANDLED;
}

static bool hiu_need_hw_request(void)
{
       unsigned int cur_level, cur_freq;

       cur_level = hiu_get_act_dvfs();
       cur_freq = data->stats->freq_table[cur_level - data->level_offset];

       return cur_freq >= data->boost_threshold;
}

static int hiu_sr1_check_loop(void *unused)
{
wait:
	wait_event(data->polling_wait, data->boosting_activated);
poll:
	mutex_lock(&data->lock);

	if (data->cur_freq < data->boost_threshold)
		goto done;

	while (!check_hiu_sr1_irq_pending() &&
		!check_hiu_mailbox_err_pending()) {
		mutex_unlock(&data->lock);
		usleep_range(POLL_PERIOD, 2 * POLL_PERIOD);
		mutex_lock(&data->lock);

		if (data->cur_freq < data->boost_threshold)
			goto done;
	}

	if (check_hiu_mailbox_err_pending()) {
		hiu_mailbox_err_handler();
		BUG_ON(1);
	}

	if (hiu_need_hw_request()) {
		schedule_work_on(cpumask_first(cpu_coregroup_mask(0)), &data->hwi_work);
		clear_hiu_sr1_irq_pending();
		mutex_unlock(&data->lock);

		wait_event(data->hwidvfs_wait, data->hwidvfs_done);
		data->hwidvfs_done = false;

		goto poll;
	}

done:
	data->boosting_activated = false;
	mutex_unlock(&data->lock);
	goto wait;

	/* NEVER come here */

	return 0;
}

/****************************************************************/
/*			EXTERNAL EVENT HANDLER			*/
/****************************************************************/
static void hiu_dummy_request(void)
{
	if (data->operation_mode == INTERRUPT_MODE) {
		mutex_lock(&data->lock);
		data->normdvfs_req = true;
	}

	request_dvfs_on_sr0(data->boost_threshold);

	if (data->operation_mode == POLLING_MODE) {
		while (!check_hiu_normal_req_done(data->boost_threshold) &&
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
	/* Explicitly disable the whole HW */
	/*	ex) hiu_control_pc, tb(0), hiu_control_mailbox(0) */

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
	if (!data->enabled)
		hiu_dummy_request();
}

static int exynos_hiu_update_data(struct cpufreq_policy *policy)
{
	if (!cpumask_test_cpu(data->cpu, policy->cpus))
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

static bool check_hiu_need_boost_thrott(void)
{
	return data->cur_freq > data->boost_threshold &&
		data->cur_freq > data->clipped_freq;
}

static int exynos_hiu_policy_callback(struct notifier_block *nb,
				unsigned long event, void *info)
{
	struct cpufreq_policy *policy = info;

	if (policy->cpu != data->cpu)
		return NOTIFY_DONE;

	if (policy->max == data->clipped_freq)
		return NOTIFY_DONE;

	switch (event) {
	case CPUFREQ_NOTIFY:

		/* Note : MUST write LIMIT_DVFS to HIU SFR */
		mutex_lock(&data->lock);
		if (policy->max >= data->boost_threshold) {
			data->clipped_freq = policy->max;
			hiu_set_limit_dvfs(data->clipped_freq);
		}
		mutex_unlock(&data->lock);

		pr_debug("exynos-hiu: update clipped freq:%d\n", data->clipped_freq);
		if (check_hiu_need_boost_thrott())
			atomic_inc(&boost_throttling);
		break;
	default:
		;
	}

	return NOTIFY_OK;
}

static struct notifier_block exynos_hiu_policy_notifier = {
	.notifier_call = exynos_hiu_policy_callback,
};

static int exynos_hiu_transition_callback(struct notifier_block *nb,
				unsigned long event, void *info)
{
	struct cpufreq_freqs *freq = info;
	int cpu = freq->cpu;

	if (cpu != data->cpu)
		return NOTIFY_DONE;

	if (event != CPUFREQ_POSTCHANGE)
		return NOTIFY_DONE;

	if (atomic_read(&boost_throttling) &&
	    data->cur_freq <= data->clipped_freq) {
		atomic_dec(&boost_throttling);
	}

	return NOTIFY_OK;
}

static struct notifier_block exynos_hiu_transition_notifier = {
	.notifier_call = exynos_hiu_transition_callback,
	.priority = INT_MIN,
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

static DEVICE_ATTR(enabled, 0644, hiu_enable_show, hiu_enable_store);
static DEVICE_ATTR(boosted, 0444, hiu_boosted_show, NULL);
static DEVICE_ATTR(boost_threshold, 0444, hiu_boost_threshold_show, NULL);
static DEVICE_ATTR(dvfs_limit, 0644, hiu_dvfs_limit_show, hiu_dvfs_limit_store);

static struct attribute *exynos_hiu_attrs[] = {
	&dev_attr_enabled.attr,
	&dev_attr_boosted.attr,
	&dev_attr_boost_threshold.attr,
	&dev_attr_dvfs_limit.attr,
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
	int ret = 0;

	ret |= of_property_read_u32(dn, "operation-mode", &data->operation_mode);
	ret |= of_property_read_u32(dn, "boot-freq", &data->cur_freq);
	ret |= of_property_read_u32(dn, "boost-threshold", &data->boost_threshold);
	ret |= of_property_read_u32(dn, "boost-max", &data->boost_max);
	ret |= of_property_read_u32(dn, "sw-pbl", &data->sw_pbl);
	ret |= of_property_read_string(dn, "sibling-cpus", &buf);
	if (ret)
		return ret;

	if (of_property_read_bool(dn, "pc-enabled"))
		data->pc_enabled = true;

	if (of_property_read_bool(dn, "tb-enabled"))
		data->tb_enabled = true;

	cpulist_parse(buf, &data->cpus);
	cpumask_and(&data->cpus, &data->cpus, cpu_possible_mask);
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
	struct task_struct *polling_thread;
	struct device_node *dn = pdev->dev.of_node;
	int ret;

	data = kzalloc(sizeof(struct exynos_hiu_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	mutex_init(&data->lock);

	platform_set_drvdata(pdev, data);

	data->base = ioremap(GCU_BASE, SZ_4K);

	ret = hiu_dt_parsing(dn);
	if (ret) {
		dev_err(&pdev->dev, "Failed to parse HIU data\n");
		return -ENODEV;
	}

	data->dn = dn;

	if (data->operation_mode == INTERRUPT_MODE) {
		init_waitqueue_head(&data->normdvfs_wait);
		data->normdvfs_done = false;

		data->irq = irq_of_parse_and_map(dn, 0);
		if (data->irq <= 0) {
			dev_err(&pdev->dev, "Failed to get IRQ\n");
			return -ENODEV;
		}

		ret = devm_request_irq(&pdev->dev, data->irq, exynos_hiu_irq_handler,
				IRQF_TRIGGER_RISING | IRQF_SHARED, dev_name(&pdev->dev), data);
		if (ret) {
			dev_err(&pdev->dev, "Failed to request IRQ handler: %d\n", data->irq);
			return -ENODEV;
		}
	} else {
		init_waitqueue_head(&data->polling_wait);
		data->boosting_activated = false;

		polling_thread = kthread_create(hiu_sr1_check_loop, NULL, "hiu_polling");
		kthread_bind_mask(polling_thread, cpu_coregroup_mask(0));
		wake_up_process(polling_thread);
	}

	cpufreq_register_notifier(&exynos_hiu_policy_notifier, CPUFREQ_POLICY_NOTIFIER);
	cpufreq_register_notifier(&exynos_hiu_transition_notifier, CPUFREQ_TRANSITION_NOTIFIER);

	INIT_WORK(&data->work, exynos_hiu_work);
	INIT_WORK(&data->hwi_work, exynos_hiu_hwi_work);
	init_waitqueue_head(&data->hwidvfs_wait);

	ret = sysfs_create_group(&pdev->dev.kobj, &exynos_hiu_attr_group);
	if (ret)
		dev_err(&pdev->dev, "Failed to create Exynos HIU attr group");

	exynos_cpufreq_ready_list_add(&exynos_hiu_ready);

	dev_info(&pdev->dev, "HIU Handler initialization complete\n");
	return 0;
}

static const struct of_device_id of_exynos_hiu_match[] = {
	{ .compatible = "samsung,exynos-hiu-tb", },
	{ },
};

static const struct platform_device_id exynos_hiu_ids[] = {
	{ "exynos-hiu-tb", },
	{ }
};

static struct platform_driver exynos_hiu_driver = {
	.driver = {
		.name = "exynos-hiu-tb",
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
arch_initcall(exynos_hiu_init);
