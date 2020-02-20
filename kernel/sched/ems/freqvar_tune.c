/*
 * Frequency variant cpufreq driver
 *
 * Copyright (C) 2017 Samsung Electronics Co., Ltd
 * Park Bumgyu <bumgyu.park@samsung.com>
 */

#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <trace/events/ems.h>

#include "../sched.h"
#include "ems.h"

/**********************************************************************
 * common APIs                                                        *
 **********************************************************************/
struct freqvar_table {
	int frequency;
	int value;
};

static int freqvar_get_value(int freq, struct freqvar_table *table)
{
	struct freqvar_table *pos = table;
	int value = -EINVAL;

	for (; pos->frequency != CPUFREQ_TABLE_END; pos++)
		if (freq == pos->frequency) {
			value = pos->value;
			break;
		}

	return value;
}

static int freqvar_get_table_size(struct cpufreq_policy *policy)
{
	struct cpufreq_frequency_table *cpufreq_table, *pos;
	int size = 0;

	cpufreq_table = policy->freq_table;
	if (unlikely(!cpufreq_table)) {
		pr_debug("%s: Unable to find frequency table\n", __func__);
		return -ENOENT;
	}

	cpufreq_for_each_valid_entry(pos, cpufreq_table)
		size++;

	return size;
}

static int freqvar_fill_frequency_table(struct cpufreq_policy *policy,
					struct freqvar_table *table)
{
	struct cpufreq_frequency_table *cpufreq_table, *pos;
	int index;

	cpufreq_table = policy->freq_table;
	if (unlikely(!cpufreq_table)) {
		pr_debug("%s: Unable to find frequency table\n", __func__);
		return -ENOENT;
	}

	index = 0;
	cpufreq_for_each_valid_entry(pos, cpufreq_table) {
		table[index].frequency = pos->frequency;
		index++;
	}
	table[index].frequency = CPUFREQ_TABLE_END;

	return 0;
}

static int freqvar_update_table(unsigned int *src, int src_size,
					struct freqvar_table *dst)
{
	struct freqvar_table *pos, *last_pos = dst;
	unsigned int value = 0, freq = 0;
	int i;

	for (i = src_size - 1; i >= 0; i--) {
		value = src[i];
		freq  = (i <= 0) ? 0 : src[i - 1];

		for (pos = last_pos; pos->frequency != CPUFREQ_TABLE_END; pos++)
			if (pos->frequency >= freq) {
				pos->value = value;
			} else {
				last_pos = pos;
				break;
			}
	}

	return 0;
}

static int freqvar_parse_value_dt(struct device_node *dn, const char *table_name,
						struct freqvar_table *table)
{
	int size, ret = 0;
	unsigned int *temp;

	/* get the table from device tree source */
	size = of_property_count_u32_elems(dn, table_name);
	if (size <= 0)
		return size;

	temp = kzalloc(sizeof(unsigned int) * size, GFP_KERNEL);
	if (!temp)
		return -ENOMEM;

	ret = of_property_read_u32_array(dn, table_name, temp, size);
	if (ret)
		goto fail_parsing;

	freqvar_update_table(temp, size, table);

fail_parsing:
	kfree(temp);
	return ret;
}

static void freqvar_free(void *data)
{
	if (data)
		kfree(data);
}

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

#define attr_freqvar(type, name, table)						\
static ssize_t freqvar_##name##_show(struct gov_attr_set *attr_set, char *buf)	\
{										\
	struct cpufreq_policy *policy = sugov_get_attr_policy(attr_set);	\
	struct freqvar_##type *data = per_cpu(freqvar_##type, policy->cpu);	\
	struct freqvar_table *pos = data->table;				\
	int ret = 0;								\
										\
	for (; pos->frequency != CPUFREQ_TABLE_END; pos++)			\
		ret += sprintf(buf + ret, "%8d ratio:%3d \n",			\
					pos->frequency, pos->value);		\
										\
	return ret;								\
}										\
										\
static ssize_t freqvar_##name##_store(struct gov_attr_set *attr_set,		\
				      const char *buf, size_t count)		\
{										\
	struct cpufreq_policy *policy = sugov_get_attr_policy(attr_set);	\
	struct freqvar_##type *data = per_cpu(freqvar_##type, policy->cpu);	\
	struct freqvar_table *old_table = data->table;				\
	int *new_table = NULL;							\
	int ntokens;								\
										\
	new_table = get_tokenized_data(buf, &ntokens);				\
	if (IS_ERR(new_table))							\
		return PTR_RET(new_table);					\
										\
	freqvar_update_table(new_table, ntokens, old_table);			\
	kfree(new_table);							\
										\
	return count;								\
}										\

int sugov_sysfs_add_attr(struct cpufreq_policy *policy, const struct attribute *attr);
struct cpufreq_policy *sugov_get_attr_policy(struct gov_attr_set *attr_set);

/**********************************************************************
 * freqvar boost                                                      *
 **********************************************************************/
struct freqvar_boost {
	struct freqvar_table *table;
	unsigned int ratio;
	unsigned long step_max_util;
};
DEFINE_PER_CPU(struct freqvar_boost *, freqvar_boost);

attr_freqvar(boost, boost, table);
static struct governor_attr freqvar_boost_attr = __ATTR_RW(freqvar_boost);

unsigned long freqvar_boost_vector(int cpu, unsigned long util)
{
	struct freqvar_boost *boost = per_cpu(freqvar_boost, cpu);
	unsigned long boosted_util = 0;

	if (!boost || !boost->ratio)
		return util;

	if (util > boost->step_max_util) {
		trace_ems_freqvar_boost(cpu, boost->ratio, boost->step_max_util, util, 0);
		return util;
	}

	boosted_util = util * boost->ratio / 100;

	if (boost->step_max_util)
		boosted_util = min_t(unsigned long, boosted_util, boost->step_max_util);

	trace_ems_freqvar_boost(cpu, boost->ratio, boost->step_max_util, util, boosted_util);

	return boosted_util;
}

static void freqvar_boost_free(struct freqvar_boost *boost)
{
	if (boost)
		freqvar_free(boost->table);

	freqvar_free(boost);
}

static struct
freqvar_boost *freqvar_boost_alloc(struct cpufreq_policy *policy)
{
	struct freqvar_boost *boost;
	int size;

	boost = kzalloc(sizeof(*boost), GFP_KERNEL);
	if (!boost)
		return NULL;

	size = freqvar_get_table_size(policy);
	if (size <= 0)
		goto fail_alloc;

	boost->table = kzalloc(sizeof(struct freqvar_table) * (size + 1), GFP_KERNEL);
	if (!boost->table)
		goto fail_alloc;

	return boost;

fail_alloc:
	freqvar_boost_free(boost);
	return NULL;
}

static void freqvar_boost_update(int cpu, int new_freq);
static int freqvar_boost_init(struct device_node *dn, const struct cpumask *mask)
{
	struct freqvar_boost *boost;
	struct cpufreq_policy *policy;
	int cpu, ret = 0;

	policy = cpufreq_cpu_get(cpumask_first(mask));
	if (!policy)
		return -ENODEV;

	boost = freqvar_boost_alloc(policy);
	if (!boost) {
		ret = -ENOMEM;
		goto fail_init;
	}

	ret = freqvar_fill_frequency_table(policy, boost->table);
	if (ret)
		goto fail_init;

	ret = freqvar_parse_value_dt(dn, "boost_table", boost->table);
	if (ret)
		goto fail_init;

	for_each_cpu(cpu, mask)
		per_cpu(freqvar_boost, cpu) = boost;

	freqvar_boost_update(policy->cpu, policy->cur);

	ret = sugov_sysfs_add_attr(policy, &freqvar_boost_attr.attr);
	if (ret)
		goto fail_init;

	return 0;

fail_init:
	cpufreq_cpu_put(policy);
	freqvar_boost_free(boost);

	return ret;
}

/**********************************************************************
 * freqvar rate limit                                                 *
 **********************************************************************/
struct freqvar_rate_limit {
	struct freqvar_table *up_table;
	struct freqvar_table *down_table;
	struct freqvar_table *st_table;
	int ratio;
};
DEFINE_PER_CPU(struct freqvar_rate_limit *, freqvar_rate_limit);

attr_freqvar(rate_limit, up_rate_limit, up_table);
attr_freqvar(rate_limit, down_rate_limit, down_table);
attr_freqvar(rate_limit, st_boost, st_table);
static struct governor_attr freqvar_up_rate_limit = __ATTR_RW(freqvar_up_rate_limit);
static struct governor_attr freqvar_down_rate_limit = __ATTR_RW(freqvar_down_rate_limit);
static struct governor_attr freqvar_st_boost = __ATTR_RW(freqvar_st_boost);

void sugov_update_rate_limit_us(struct cpufreq_policy *policy,
			int up_rate_limit_ms, int down_rate_limit_ms);
static void freqvar_rate_limit_update(int cpu, int new_freq)
{
	struct freqvar_rate_limit *rate_limit;
	int up_rate_limit, down_rate_limit;
	struct cpufreq_policy *policy;

	rate_limit = per_cpu(freqvar_rate_limit, cpu);
	if (!rate_limit)
		return;

	up_rate_limit = freqvar_get_value(new_freq, rate_limit->up_table);
	down_rate_limit = freqvar_get_value(new_freq, rate_limit->down_table);

	policy = cpufreq_cpu_get(cpu);
	if (!policy)
		return;

	sugov_update_rate_limit_us(policy, up_rate_limit, down_rate_limit);

	cpufreq_cpu_put(policy);
}

static void freqvar_rate_limit_free(struct freqvar_rate_limit *rate_limit)
{
	if (rate_limit) {
		freqvar_free(rate_limit->up_table);
		freqvar_free(rate_limit->down_table);
	}

	freqvar_free(rate_limit);
}

static struct
freqvar_rate_limit *freqvar_rate_limit_alloc(struct cpufreq_policy *policy)
{
	struct freqvar_rate_limit *rate_limit;
	int size;

	rate_limit = kzalloc(sizeof(*rate_limit), GFP_KERNEL);
	if (!rate_limit)
		return NULL;

	size = freqvar_get_table_size(policy);
	if (size <= 0)
		goto fail_alloc;

	rate_limit->up_table = kzalloc(sizeof(struct freqvar_table)
					* (size + 1), GFP_KERNEL);
	if (!rate_limit->up_table)
		goto fail_alloc;

	rate_limit->down_table = kzalloc(sizeof(struct freqvar_table)
					* (size + 1), GFP_KERNEL);
	if (!rate_limit->down_table)
		goto fail_alloc;

	rate_limit->st_table= kzalloc(sizeof(struct freqvar_table)
					* (size + 1), GFP_KERNEL);
	if (!rate_limit->st_table)
		goto fail_alloc;

	return rate_limit;

fail_alloc:
	freqvar_rate_limit_free(rate_limit);
	return NULL;
}

static int freqvar_rate_limit_init(struct device_node *dn, const struct cpumask *mask)
{
	struct freqvar_rate_limit *rate_limit;
	struct cpufreq_policy *policy;
	int cpu, ret = 0;

	policy = cpufreq_cpu_get(cpumask_first(mask));
	if (!policy)
		return -ENODEV;

	rate_limit = freqvar_rate_limit_alloc(policy);
	if (!rate_limit) {
		ret = -ENOMEM;
		goto fail_init;
	}

	ret = freqvar_fill_frequency_table(policy, rate_limit->up_table);
	if (ret)
		goto fail_init;

	ret = freqvar_fill_frequency_table(policy, rate_limit->down_table);
	if (ret)
		goto fail_init;

	ret = freqvar_fill_frequency_table(policy, rate_limit->st_table);
	if (ret)
		goto fail_init;

	ret = freqvar_parse_value_dt(dn, "up_rate_limit_table", rate_limit->up_table);
	if (ret)
		goto fail_init;

	ret = freqvar_parse_value_dt(dn, "down_rate_limit_table", rate_limit->down_table);
	if (ret)
		goto fail_init;

	ret = freqvar_parse_value_dt(dn, "st_table", rate_limit->st_table);
	if (ret)
		goto fail_init;

	ret = sugov_sysfs_add_attr(policy, &freqvar_up_rate_limit.attr);
	if (ret)
		goto fail_init;

	ret = sugov_sysfs_add_attr(policy, &freqvar_down_rate_limit.attr);
	if (ret)
		goto fail_init;

	ret = sugov_sysfs_add_attr(policy, &freqvar_st_boost.attr);
	if (ret)
		goto fail_init;

	for_each_cpu(cpu, mask)
		per_cpu(freqvar_rate_limit, cpu) = rate_limit;

	freqvar_rate_limit_update(policy->cpu, policy->cur);

	return 0;

fail_init:
	freqvar_rate_limit_free(rate_limit);
	cpufreq_cpu_put(policy);

	return ret;
}

static void freqvar_boost_update(int cpu, int new_freq)
{
	struct freqvar_boost *boost;
	struct freqvar_rate_limit *rate_limit;

	boost = per_cpu(freqvar_boost, cpu);
	if (!boost)
		return;

	boost->ratio = freqvar_get_value(new_freq, boost->table);
	boost->step_max_util = get_freq_cap(cpu, new_freq, 0) * boost->ratio / 100;

	rate_limit = per_cpu(freqvar_rate_limit, cpu);
	if (!rate_limit)
		return;
	rate_limit->ratio = freqvar_get_value(new_freq, rate_limit->st_table);

}

unsigned long freqvar_st_boost_vector(int cpu)
{
	struct freqvar_rate_limit *boost = per_cpu(freqvar_rate_limit, cpu);

	if (!boost)
		return 0;

	trace_ems_freqvar_st_boost(cpu, boost->ratio);

	return boost->ratio;
}

/**********************************************************************
 * cpufreq notifier callback                                          *
 **********************************************************************/
static int freqvar_cpufreq_callback(struct notifier_block *nb,
					unsigned long val, void *data)
{
	struct cpufreq_freqs *freq = data;

	if (freq->flags & CPUFREQ_CONST_LOOPS)
		return NOTIFY_OK;

	if (val != CPUFREQ_POSTCHANGE)
		return NOTIFY_OK;

	freqvar_boost_update(freq->cpu, freq->new);
	freqvar_rate_limit_update(freq->cpu, freq->new);

	return 0;
}

static struct notifier_block freqvar_cpufreq_notifier = {
	.notifier_call  = freqvar_cpufreq_callback,
};

/**********************************************************************
 * initialization                                                     *
 **********************************************************************/
static int __init freqvar_tune_init(void)
{
	struct device_node *dn = NULL;
	struct cpumask shared_mask;
	const char *buf;

	while ((dn = of_find_node_by_type(dn, "freqvar-tune"))) {
		/*
		 * shared-cpus includes cpus scaling at the sametime.
		 * it is called "sibling cpus" in the CPUFreq and
		 * masked on the realated_cpus of the policy
		 */
		if (of_property_read_string(dn, "shared-cpus", &buf))
			continue;

		cpumask_clear(&shared_mask);
		cpulist_parse(buf, &shared_mask);
		cpumask_and(&shared_mask, &shared_mask, cpu_possible_mask);
		if (cpumask_weight(&shared_mask) == 0)
			continue;

		freqvar_boost_init(dn, &shared_mask);
		freqvar_rate_limit_init(dn, &shared_mask);
	}

	cpufreq_register_notifier(&freqvar_cpufreq_notifier,
					CPUFREQ_TRANSITION_NOTIFIER);

	return 0;
}
late_initcall(freqvar_tune_init);
