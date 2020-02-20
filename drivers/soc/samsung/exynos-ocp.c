/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS - OCP(Over Current Protection) support
 * Auther : LEE DAEYEONG (daeyeong.lee@samsung.com)
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/regmap.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/mfd/syscon.h>
#include <linux/io.h>
#include <linux/irq_work.h>
#include <linux/mfd/samsung/s2mps19-regulator.h>
#include <trace/events/power.h>
#include <linux/debug-snapshot.h>

#include <soc/samsung/exynos-cpuhp.h>

#include "../../../kernel/sched/sched.h"
#include "../../cpufreq/exynos-acme.h"
#include "../../../kernel/sched/ems/ems.h"

#define GCU_BASE_ADDR			(0x1E4C0000)

/* OCP_THROTTLE_CONTROL */
#define OCPTHROTTCTL			(0x9F0)
#define TEW_MASK			(0x1)
#define TEW_SHIFT			(2)
#define TRW_MASK			(0x1f)
#define TRW_SHIFT			(3)
#define BPC_MASK			(0x1f)
#define BPC_SHIFT			(16)
#define TCRST_MASK			(0x7)
#define TCRST_SHIFT			(21)
#define DPM_THROTTSET_MASK		(0x1)
#define DPM_THROTTSET_SHIFT		(24)
#define OCPBPCHIT_MASK			(0x1)
#define OCPBPCHIT_SHIFT			(26)
#define OCPTHROTTERRA_MASK		(0x1)
#define OCPTHROTTERRA_SHIFT		(27)

/* GCU_CONTROL_REGISTER */
#define GCUCTL				(0x22C)
#define OCPBPCEN_MASK			(0x1)
#define OCPBPCEN_SHIFT			(25)
#define OCPTHROTTERRAEN_MASK		(0x1)
#define OCPTHROTTERRAEN_SHIFT		(24)

/* OCP_INTEGRATION_TOP_POWER_THRESHOLD */
#define OCPTOPPWRTHRESH 		(0x83C)
#define IRP_MASK			(0x3f)
#define IRP_SHIFT			(24)

/* OCP_THROTTLE_COUNTER_ALL */
#define OCPTHROTTCNTA			(0x938)
#define TDC_MASK			(0xfff)
#define TDC_SHIFT			(4)

struct ocp_stats {
	unsigned int		max_state;
	unsigned int		last_index;
	unsigned long long	last_time;
	unsigned long long	total_trans;
	unsigned int		*freq_table;
	unsigned long long	*time_in_state;
};

struct exynos_ocp_data {
	bool			enabled;
	bool			updated;
	bool			flag;

	int			irq;
	struct work_struct	work;
	struct delayed_work 	delayed_work;
	struct irq_work		irq_work;

	void __iomem *		base;

	struct i2c_client	*i2c;

	struct ocp_stats	*stats;

	struct cpumask		cpus;
	unsigned int		cpu;

	unsigned int		clipped_freq;
	unsigned int		max_freq;
	unsigned int		max_freq_wo_ocp;
	unsigned int		min_freq;
	unsigned int		cur_freq;
	unsigned int		down_step;

	unsigned int		release_mode;
	unsigned int		release_threshold;
	unsigned int		release_duration;
};
struct exynos_ocp_data *data;

/****************************************************************/
/*			HELPER FUNCTION				*/
/****************************************************************/

#define SYS_READ(reg, val)	do { val = __raw_readl(data->base + reg); } while (0);
#define SYS_WRITE(reg, val)	do { __raw_writel(val, data->base + reg); } while (0);
#define CONTROL_OCP_WARN(enable)	s2mps19_update_reg(data->i2c, S2MPS19_PMIC_REG_OCP_WARN1,\
		((enable) << S2MPS19_OCP_WARN_EN_SHIFT), (1 << S2MPS19_OCP_WARN_EN_SHIFT));

static int get_ocp_freq_index(struct ocp_stats *stats, unsigned int freq)
{
	int index;

	for (index = 0; index < stats->max_state; index++)
		if (stats->freq_table[index] == freq)
			return index;

	return -1;
}

static void update_ocp_stats(struct ocp_stats *stats)
{
	unsigned long long cur_time = get_jiffies_64();

	/* If OCP operation is disabled, do not update OCP stats */
	if (data->enabled == false)
		return;

	stats->time_in_state[stats->last_index] += cur_time - stats->last_time;
	stats->last_time = cur_time;
}

static unsigned int get_ocp_target_max_limit(unsigned int down_step)
{
	struct ocp_stats *stats = data->stats;
	unsigned int index, ret_freq;

	/* Find the position of the current frequency in the frequency table. */
	index = get_ocp_freq_index(stats, data->clipped_freq);

	/* Find target max limit that lower by "down_step" than current max limit */
	index += down_step;
	if (index > stats->max_state)
		index = stats->max_state;
	ret_freq = stats->freq_table[index];

	return ret_freq;
}

static void set_ocp_max_limit(unsigned int down_step)
{
	unsigned int target_max;

	if (data->cpu >= nr_cpu_ids) {
		dbg_snapshot_printk("OCP_cpus_off\n");
		return;
	}

	/*
	 * If the down step is greater than 0,
	 * the target max limit is set to a frequency
	 * that is lower than the current frequency by a down step.
	 * Otherwise release the target max limit to max frequency.
	 */
	if (down_step) {
		target_max = get_ocp_target_max_limit(down_step);
		if (target_max) {
			data->clipped_freq = target_max;
			pr_debug("OCP max limit is set to %u kHz\n", data->clipped_freq);
		} else
			return;
	} else {
		data->clipped_freq = data->max_freq;
		pr_debug("OCP max limit is released\n");
	}

	dbg_snapshot_printk("OCP_enter:%ukHz(%s)\n",
			data->clipped_freq, down_step ? "throttle" : "release");
	trace_ocp_max_limit(data->clipped_freq, 1);
	cpufreq_update_policy(data->cpu);
	dbg_snapshot_printk("OCP_exit:%ukHz(%s)\n",
			data->clipped_freq, down_step ? "throttle" : "release");
	trace_ocp_max_limit(data->clipped_freq, 0);

	/* Whenever ocp max limit is changed, ocp stats should be updated. */
	update_ocp_stats(data->stats);
	data->stats->last_index = get_ocp_freq_index(data->stats, data->clipped_freq);
	data->stats->total_trans++;
}

#define CURRENT_METER_MODE		(0)
#define CPU_UTIL_MODE			(1)
#define BUCK2_COEFF			(46875)
#define ONE_HUNDRED			(100)

/*
 * Check BPC condition based on currentmeter information.
 * If current value of big cluster is lower than configured current value,
 * BPC condition is true.
 */
static bool is_currentmeter_condition(void)
{
	unsigned int val;
	unsigned char temp;

	s2mps19_update_reg(data->i2c, S2MPS19_REG_ADC_CTRL3, 1, ADC_PTR_MASK);
	s2mps19_read_reg(data->i2c, S2MPS19_REG_ADC_DATA, &temp);
	val = temp * BUCK2_COEFF;

	return val < data->release_threshold;
}

/*
 * Check BPC condition based on cpu load information.
 * If sum util_avg of each core is lower than configured ratio of capacity,
 * BPC condition is true.
 */
static bool is_cpuutil_condition(void)
{
	unsigned int cpu, count = 0;
	unsigned long util = 0;
	unsigned long capacity;

	if (data->cpu >= nr_cpu_ids)
		return true;

	capacity = capacity_orig_of(data->cpu);

	for_each_cpu(cpu, &data->cpus) {
		util += ml_cpu_util(cpu);
		count++;
	}

	capacity *= count;

	/* If util < capacity * release_threshold(%), return true */
	return (util * ONE_HUNDRED) < (capacity * data->release_threshold);
}

static bool is_bpc_condition(void)
{
	switch (data->release_mode) {
		case CURRENT_METER_MODE :
			return is_currentmeter_condition();
		case CPU_UTIL_MODE :
			return is_cpuutil_condition();
		default :
			return 1;
	}
}

static void control_ocp_operation(bool enable)
{
	if (data->enabled == enable)
		return;

	if (data->cpu >= nr_cpu_ids) {
		pr_info("all OCP releated cpus are off\n");
		return;
	}

	/*
	 * When enabling OCP operation, first enable OCP_WARN and release OCP max limit.
	 * Conversely, when disabling OCP operation, first press OCP max limit and disable OCP_WARN.
	 */
	if (enable) {
		unsigned long long cur_time = get_jiffies_64();

		CONTROL_OCP_WARN(1);

		/* Release OCP max limit */
		data->clipped_freq = data->max_freq;
		cpufreq_update_policy(data->cpu);

		/* Re-init OCP stats */
		data->stats->last_index = get_ocp_freq_index(data->stats, data->clipped_freq);
		data->stats->last_time = cur_time;
	} else {
		cancel_delayed_work_sync(&data->delayed_work);
		data->flag = false;

		/* Update OCP stats before disabling OCP operation */
		update_ocp_stats(data->stats);

		/* Press OCP max limit to max frequency without OCP */
		data->clipped_freq = data->max_freq_wo_ocp;
		cpufreq_update_policy(data->cpu);

		CONTROL_OCP_WARN(0);
	}

	data->enabled = enable;

	pr_info("OCP operation is %s\n", (enable)?"enabled":"disabled");
	dbg_snapshot_printk("OCP_%s\n", (enable)?"enabled":"disabled");
}

/****************************************************************/
/*			OCP INTERRUPT HANDLER			*/
/****************************************************************/

static void control_ocp_interrupt(int enable)
{
	int val;

	SYS_READ(GCUCTL, val);

	if (enable)
		val |= (OCPTHROTTERRAEN_MASK << OCPTHROTTERRAEN_SHIFT);
	else
		val &= ~(OCPTHROTTERRAEN_MASK << OCPTHROTTERRAEN_SHIFT);

	SYS_WRITE(GCUCTL, val);
}

static int check_ocp_interrupt(void)
{
	int val;

	SYS_READ(OCPTHROTTCTL, val);
	val = (val >> OCPTHROTTERRA_SHIFT) & OCPTHROTTERRA_MASK;

	return val;
}

static void clear_ocp_interrupt(void)
{
	int val;

	SYS_READ(OCPTHROTTCTL, val);
	val &= ~(OCPTHROTTERRA_MASK << OCPTHROTTERRA_SHIFT);
	SYS_WRITE(OCPTHROTTCTL, val);
}

static int check_dpm_throttset(void)
{
	int val;

	SYS_READ(OCPTHROTTCTL, val);
	val = (val >> DPM_THROTTSET_SHIFT) & DPM_THROTTSET_MASK;

	return val;
}

static void clear_dpm_throttset(void)
{
	int val;

	SYS_READ(OCPTHROTTCTL, val);
	val &= ~(DPM_THROTTSET_MASK << DPM_THROTTSET_SHIFT);
	SYS_WRITE(OCPTHROTTCTL, val);
}

static void clear_ocp_throttling_duration_counter(void)
{
	int val;

	SYS_READ(OCPTHROTTCNTA, val);
	val &= ~(TDC_MASK << TDC_SHIFT);
	SYS_WRITE(OCPTHROTTCNTA, val);
}

#define SWI_ENABLE			(1)
#define SWI_DISABLE			(0)

static void exynos_ocp_work(struct work_struct *work)
{
	if (!cpumask_test_cpu(smp_processor_id(), &data->cpus))
		return;

	data->flag = true;
	set_ocp_max_limit(data->down_step);

	/* Before enabling OCP interrupt, clear releated register fields */
	clear_ocp_throttling_duration_counter();
	clear_dpm_throttset();
	clear_ocp_interrupt();

	/* After finish interrupt handling, enable OCP interrupt. */
	control_ocp_interrupt(SWI_ENABLE);

	cancel_delayed_work_sync(&data->delayed_work);
	schedule_delayed_work(&data->delayed_work, msecs_to_jiffies(data->release_duration));
}

static void exynos_ocp_work_release(struct work_struct *work)
{
	/*
	 * If BPC condition is true, release ocp max limit.
	 * Otherwise extend ocp max limit as release duration.
	 */
	if (is_bpc_condition()) {
		data->flag = false;
		set_ocp_max_limit(0);
	}
	else
		schedule_delayed_work(&data->delayed_work, msecs_to_jiffies(data->release_duration));
}

static irqreturn_t exynos_ocp_irq_handler(int irq, void *id)
{
	/* Check the source of interrupt */
	if (!check_ocp_interrupt())
		return IRQ_NONE;

	/*
	 * We use DPM only in power constraint method.
	 * Don't throttle max frequency when interrupt is pending by dpm.
	 * Only u-throttling and clk div-2 are used for throttling.
	 */
	if (check_dpm_throttset())
		clear_dpm_throttset();
	else if (data->cpu < nr_cpu_ids) {
		/* Before start interrupt handling, disable OCP interrupt. */
		control_ocp_interrupt(SWI_DISABLE);

		schedule_work_on(data->cpu, &data->work);
	}

	clear_ocp_throttling_duration_counter();
	clear_ocp_interrupt();

	return IRQ_HANDLED;
}

/****************************************************************/
/*			EXTERNAL EVENT HANDLER			*/
/****************************************************************/

static int exynos_ocp_policy_callback(struct notifier_block *nb,
				unsigned long event, void *info)
{
	struct cpufreq_policy *policy = info;

	if (!data || !data->updated)
		return NOTIFY_DONE;

	if (!cpumask_test_cpu(policy->cpu, &data->cpus))
		return NOTIFY_DONE;

	if (event != CPUFREQ_ADJUST)
		return NOTIFY_DONE;

	if (policy->max > data->clipped_freq)
		cpufreq_verify_within_limits(policy, 0, data->clipped_freq);

	return NOTIFY_OK;
}

static struct notifier_block exynos_ocp_policy_notifier = {
	.notifier_call = exynos_ocp_policy_callback,
};

static void exynos_ocp_irq_work(struct irq_work *irq_work)
{
	int irp, val;

	/* Set IRP for current DVFS level to OCP controller */
	irp = (data->cur_freq * 2)/data->min_freq - 2;
	if (irp > IRP_MASK)
		irp = IRP_MASK;

	SYS_READ(OCPTOPPWRTHRESH, val);
	val &= ~(IRP_MASK << IRP_SHIFT);
	val |= (irp << IRP_SHIFT);
	SYS_WRITE(OCPTOPPWRTHRESH, val);
}

static int exynos_ocp_cpufreq_callback(struct notifier_block *nb,
				unsigned long event, void *info)
{
	struct cpufreq_freqs *freq = info;
	int cpu = freq->cpu;

	if (!data->enabled)
		return NOTIFY_DONE;

	if (!cpumask_test_cpu(cpu, &data->cpus))
		return NOTIFY_DONE;

	if (event != CPUFREQ_PRECHANGE)
		return NOTIFY_DONE;

	if (data->cpu >= nr_cpu_ids)
		return NOTIFY_DONE;

	data->cur_freq = freq->new;
	irq_work_queue_on(&data->irq_work, data->cpu);

	return NOTIFY_OK;
}

static struct notifier_block exynos_ocp_cpufreq_notifier = {
	.notifier_call = exynos_ocp_cpufreq_callback,
};

static void ocp_stats_create_table(struct cpufreq_policy *policy);

static int exynos_ocp_update_data(struct cpufreq_policy *policy)
{
	if (data->cpu >= nr_cpu_ids)
		return 0;

	if (!cpumask_test_cpu(data->cpu, policy->cpus))
		return 0;

	if (data->updated)
		return 0;

	data->enabled = true;
	data->flag = false;
	data->min_freq = policy->user_policy.min;
	data->max_freq = policy->user_policy.max;
	data->clipped_freq = data->max_freq;
	ocp_stats_create_table(policy);
	control_ocp_interrupt(1);

	data->updated = true;

	pr_info("exynos-ocp: OCP data structure update complete\n");

	return 0;
}

static struct exynos_cpufreq_ready_block exynos_ocp_ready = {
	.update = exynos_ocp_update_data,
};

static int exynos_ocp_cpu_up_callback(unsigned int cpu)
{
	struct cpumask mask;

	if (!cpumask_test_cpu(cpu, &data->cpus))
		return 0;

	/* The first incomming cpu in ocp data binds ocp interrupt on data->cpus. */
	cpumask_and(&mask, &data->cpus, cpu_online_mask);
	if (cpumask_weight(&mask) == 1) {
		data->cpu = cpu;
		irq_set_affinity_hint(data->irq, &data->cpus);
	}

	return 0;
}

static int exynos_ocp_cpu_down_callback(unsigned int cpu)
{
	struct cpumask mask;

	if (!cpumask_test_cpu(cpu, &data->cpus))
		return 0;

	/* If all data->cpus off, update data->cpu as nr_cpu_ids */
	cpumask_and(&mask, &data->cpus, cpu_online_mask);
	cpumask_clear_cpu(cpu, &mask);
	if (cpumask_empty(&mask))
		data->cpu = nr_cpu_ids;
	else
		data->cpu = cpumask_any(&mask);

	return 0;
}

unsigned int get_ocp_clipped_freq(void)
{
	return data->clipped_freq;
}

/****************************************************************/
/*			SYSFS INTERFACE				*/
/****************************************************************/

static ssize_t
ocp_enable_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", data->enabled);
}

static ssize_t
ocp_enable_store(struct device *dev, struct device_attribute *devattr,
			const char *buf, size_t count)
{
	unsigned int input;

	if (kstrtos32(buf, 10, &input))
		return -EINVAL;

	control_ocp_operation(!!input);

	return count;
}

static ssize_t
ocp_flag_show(struct device *dev, struct device_attribute *devattr,
		char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", data->flag);
}

static ssize_t
down_step_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", data->down_step);
}

static ssize_t
down_step_store(struct device *dev, struct device_attribute *devattr,
			const char *buf, size_t count)
{
	unsigned int val;

	if (kstrtos32(buf, 10, &val))
		return -EINVAL;
	data->down_step = val;

	return count;
}

static ssize_t
release_mode_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	switch (data->release_mode) {
		case CURRENT_METER_MODE :
			return snprintf(buf, PAGE_SIZE, "CURRENT METER MODE (%d)\n", CURRENT_METER_MODE);
		case CPU_UTIL_MODE :
			return snprintf(buf, PAGE_SIZE, "CPU UTIL MODE (%d)\n", CPU_UTIL_MODE);
		default :
			return snprintf(buf, PAGE_SIZE, "error (%x)\n", data->release_mode);
	}
}

static ssize_t
release_threshold_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", data->release_threshold);
}

static ssize_t
release_threshold_store(struct device *dev, struct device_attribute *devattr,
			const char *buf, size_t count)
{
	unsigned int val;

	if (kstrtos32(buf, 10, &val))
		return -EINVAL;
	data->release_threshold = val;

	return count;
}

static ssize_t
release_duration_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", data->release_duration);
}

static ssize_t
release_duration_store(struct device *dev, struct device_attribute *devattr,
			const char *buf, size_t count)
{
	unsigned int val;

	if (kstrtos32(buf, 10, &val))
		return -EINVAL;
	data->release_duration = val;

	return count;
}

static ssize_t
clipped_freq_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	return sprintf(buf, "%d\n", data->clipped_freq);
}

static ssize_t
total_trans_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	return sprintf(buf, "%llu\n", data->stats->total_trans);
}

static ssize_t
time_in_state_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	struct ocp_stats *stats = data->stats;
	ssize_t len = 0;
	int i;

	update_ocp_stats(stats);

	for (i = 0; i < stats->max_state; i++) {
		len += sprintf(buf + len, "%u %llu\n", stats->freq_table[i],
			(unsigned long long)jiffies_64_to_clock_t(stats->time_in_state[i]));
	}

	return len;
}

static DEVICE_ATTR(enabled, 0644, ocp_enable_show, ocp_enable_store);
static DEVICE_ATTR(ocp_flag, 0444, ocp_flag_show, NULL);
static DEVICE_ATTR(down_step, 0644, down_step_show, down_step_store);
static DEVICE_ATTR(release_mode, 0444, release_mode_show, NULL);
static DEVICE_ATTR(release_threshold, 0644, release_threshold_show, release_threshold_store);
static DEVICE_ATTR(release_duration, 0644, release_duration_show, release_duration_store);
static DEVICE_ATTR(clipped_freq, 0444, clipped_freq_show, NULL);
static DEVICE_ATTR(total_trans, 0444, total_trans_show, NULL);
static DEVICE_ATTR(time_in_state, 0444, time_in_state_show, NULL);

static struct attribute *exynos_ocp_attrs[] = {
	&dev_attr_enabled.attr,
	&dev_attr_ocp_flag.attr,
	&dev_attr_down_step.attr,
	&dev_attr_release_mode.attr,
	&dev_attr_release_threshold.attr,
	&dev_attr_release_duration.attr,
	&dev_attr_clipped_freq.attr,
	&dev_attr_total_trans.attr,
	&dev_attr_time_in_state.attr,
	NULL,
};

static struct attribute_group exynos_ocp_attr_group = {
	.name = "ocp",
	.attrs = exynos_ocp_attrs,
};

/****************************************************************/
/*		INITIALIZE EXYNOS OCP DRIVER			*/
/****************************************************************/

static int ocp_dt_parsing(struct device_node *dn)
{
	const char *buf;
	int ret = 0;

	ret |= of_property_read_u32(dn, "down-step", &data->down_step);
	ret |= of_property_read_u32(dn, "max-freq-wo-ocp", &data->max_freq_wo_ocp);
	ret |= of_property_read_u32(dn, "release-mode", &data->release_mode);
	ret |= of_property_read_u32(dn, "release-threshold", &data->release_threshold);
	ret |= of_property_read_u32(dn, "release-duration", &data->release_duration);

	ret |= of_property_read_string(dn, "sibling-cpus", &buf);
	if (ret)
		return ret;

	cpulist_parse(buf, &data->cpus);
	cpumask_and(&data->cpus, &data->cpus, cpu_possible_mask);
	cpumask_and(&data->cpus, &data->cpus, cpu_online_mask);
	if (cpumask_weight(&data->cpus) == 0) {
		CONTROL_OCP_WARN(0);
		return -ENODEV;
	}

	data->cpu = cpumask_first(&data->cpus);

	return 0;
}

static void ocp_stats_create_table(struct cpufreq_policy *policy)
{
	unsigned int i = 0, count = 0, alloc_size;
	struct ocp_stats *stats;
	struct cpufreq_frequency_table *pos, *table;

	if (data && data->stats)
		return;

	table = policy->freq_table;
	if (unlikely(!table))
		return;

	stats = kzalloc(sizeof(*stats), GFP_KERNEL);
	if (!stats)
		return;

	cpufreq_for_each_valid_entry(pos, table)
		count++;

	alloc_size = count * sizeof(int) + count * sizeof(u64);

	stats->time_in_state = kzalloc(alloc_size, GFP_KERNEL);
	if (!stats->time_in_state)
		goto free_stat;

	stats->freq_table = (unsigned int *)(stats->time_in_state + count);

	stats->max_state = count;

	cpufreq_for_each_valid_entry(pos, table)
		stats->freq_table[i++] = pos->frequency;

	stats->last_time = get_jiffies_64();
	stats->last_index = get_ocp_freq_index(stats, data->clipped_freq);

	data->stats = stats;
	return;
free_stat:
	kfree(stats);
}

static int exynos_ocp_probe(struct platform_device *pdev)
{
	struct device_node *dn = pdev->dev.of_node;
	int ret;

	data = kzalloc(sizeof(struct exynos_ocp_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->updated = false;

	platform_set_drvdata(pdev, data);

	data->base = ioremap(GCU_BASE_ADDR, SZ_4K);

	get_s2mps19_i2c(&data->i2c);
	if (data->i2c == NULL) {
		dev_err(&pdev->dev, "Failed to get s2mps19 i2c_client\n");
		goto free_data;
	}

	ret = ocp_dt_parsing(dn);
	if (ret) {
		dev_err(&pdev->dev, "Failed to parse OCP data\n");
		goto free_data;
	}

	cpufreq_register_notifier(&exynos_ocp_policy_notifier, CPUFREQ_POLICY_NOTIFIER);
	cpufreq_register_notifier(&exynos_ocp_cpufreq_notifier, CPUFREQ_TRANSITION_NOTIFIER);

	data->irq = irq_of_parse_and_map(dn, 0);
	if (data->irq <= 0) {
		dev_err(&pdev->dev, "Failed to get IRQ\n");
		goto free_data;
	}

	control_ocp_interrupt(0);

	ret = devm_request_irq(&pdev->dev, data->irq, exynos_ocp_irq_handler,
			IRQF_TRIGGER_HIGH | IRQF_SHARED, dev_name(&pdev->dev), data);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request IRQ handler: %d\n", data->irq);
		goto free_data;
	}

	INIT_WORK(&data->work, exynos_ocp_work);
	INIT_DELAYED_WORK(&data->delayed_work, exynos_ocp_work_release);
	init_irq_work(&data->irq_work, exynos_ocp_irq_work);

	irq_set_affinity_hint(data->irq, &data->cpus);

	cpuhp_setup_state_nocalls(CPUHP_AP_EXYNOS_OCP,
					"exynos:ocp",
					exynos_ocp_cpu_up_callback,
					exynos_ocp_cpu_down_callback);

	ret = sysfs_create_group(&pdev->dev.kobj, &exynos_ocp_attr_group);
	if (ret)
		dev_err(&pdev->dev, "Failed to create Exynos OCP attr group");

	exynos_cpufreq_ready_list_add(&exynos_ocp_ready);

	dev_info(&pdev->dev, "Complete OCP Handler initialization\n");
	return 0;

free_data:
	kfree(data);
	return -ENODEV;
}

static const struct of_device_id of_exynos_ocp_match[] = {
	{ .compatible = "samsung,exynos-ocp", },
	{ },
};

static const struct platform_device_id exynos_ocp_ids[] = {
	{ "exynos-ocp", },
	{ }
};

static struct platform_driver exynos_ocp_driver = {
	.driver = {
		.name = "exynos-ocp",
		.owner = THIS_MODULE,
		.of_match_table = of_exynos_ocp_match,
	},
	.probe		= exynos_ocp_probe,
	.id_table	= exynos_ocp_ids,
};

int __init exynos_ocp_init(void)
{
	return platform_driver_register(&exynos_ocp_driver);
}
device_initcall(exynos_ocp_init);
