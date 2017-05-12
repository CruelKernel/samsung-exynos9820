/*
 * Copyright (C) 2014-2017, Sultanxda <sultanxda@gmail.com>
 *           (C) 2017, Joe Maples <joe@frap129.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/* drivers/cpufreq/fp-boost.c: Fingerprint boost driver
 *
 * fp-boost is a simple cpufreq driver that boosts all CPU frequencies
 * to max when an input notification is recieved from the devices
 * fingerprint sensor. This aims to wake speed-up device unlock,
 * especially from the deep sleep state.
 *
 * fp-boost is based on cpu_input_boost by Sultanxda, and all copyright
 * information has been retained. Huge thanks to him for writing the
 * initial driver this was based on.
 */

#define pr_fmt(fmt) "fp-boost: " fmt

#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/input.h>
#include <linux/slab.h>

/* Available bits for boost_policy state */
#define DRIVER_ENABLED        (1U << 0)
#define FINGERPRINT_BOOST           (1U << 1)

/* Fingerprint sensor input key */
#define FINGERPRINT_KEY 0x2ee

/* The duration in milliseconds for the fingerprint boost */
#define FP_BOOST_MS (2000)

/*
 * "fp_config" = "fingerprint boost configuration". This contains the data and
 * workers used for a single input-boost event.
 */
struct fp_config {
	struct delayed_work boost_work;
	struct delayed_work unboost_work;
	uint32_t adj_duration_ms;
	uint32_t cpus_to_boost;
	uint32_t duration_ms;
	uint32_t freq[2];
};

/*
 * This is the struct that contains all of the data for the entire driver. It
 * encapsulates all of the other structs, so all data can be accessed through
 * this struct.
 */
struct boost_policy {
	spinlock_t lock;
	struct fp_config fp;
	struct workqueue_struct *wq;
	uint32_t state;
};

/* Global pointer to all of the data for the driver */
static struct boost_policy *boost_policy_g;

static uint32_t get_boost_state(struct boost_policy *b);
static void set_boost_bit(struct boost_policy *b, uint32_t state);
static void clear_boost_bit(struct boost_policy *b, uint32_t state);
static void unboost_all_cpus(struct boost_policy *b);
static void update_online_cpu_policy(void);

/* Boolean to let us know if input is already recieved */
static bool touched;

static void fp_boost_main(struct work_struct *work)
{
	struct boost_policy *b = boost_policy_g;
	struct fp_config *fp = &b->fp;

	/* All CPUs will be boosted to policy->max */
	set_boost_bit(b, FINGERPRINT_BOOST);

	/* Immediately boost the online CPUs */
	update_online_cpu_policy();

	queue_delayed_work(b->wq, &fp->unboost_work,
				msecs_to_jiffies(FP_BOOST_MS));

}

static void fp_unboost_main(struct work_struct *work)
{
	struct boost_policy *b = boost_policy_g;
	pr_info("Unboosting\n");
	touched = false;
	/* This clears the wake-boost bit and unboosts everything */
	unboost_all_cpus(b);
}

static int do_cpu_boost(struct notifier_block *nb,
		unsigned long action, void *data)
{
	struct cpufreq_policy *policy = data;
	struct boost_policy *b = boost_policy_g;
	uint32_t state;

	if (action != CPUFREQ_ADJUST)
		return NOTIFY_OK;

	state = get_boost_state(b);

	/*
	 * Don't do anything when the driver is disabled, unless there are
	 * still CPUs that need to be unboosted.
	 */
	if (!(state & DRIVER_ENABLED) &&
		policy->min == policy->cpuinfo.min_freq)
		return NOTIFY_OK;

	/* Boost CPU to max frequency for fingerprint boost */
	if (state & FINGERPRINT_BOOST) {
		pr_info("Boosting\n");
		policy->cur = policy->max;
		policy->min = policy->max;
		return NOTIFY_OK;
	}

	return NOTIFY_OK;
}

static struct notifier_block do_cpu_boost_nb = {
	.notifier_call = do_cpu_boost,
	.priority = INT_MAX,
};

static void cpu_fp_input_event(struct input_handle *handle, unsigned int type,
		unsigned int code, int value)
{
	struct boost_policy *b = boost_policy_g;
	struct fp_config *fp = &b->fp;
	uint32_t state;

	state = get_boost_state(b);

	if (!(state & DRIVER_ENABLED) || touched)
		return;

	pr_info("Recieved input event\n");
	touched = true;
	set_boost_bit(b, FINGERPRINT_BOOST);

	/* Delaying work to ensure all CPUs are online */
	queue_delayed_work(b->wq, &fp->boost_work,
				msecs_to_jiffies(20));
}

static int cpu_fp_input_connect(struct input_handler *handler,
		struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int ret;

	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "cpu_fp_handle";

	ret = input_register_handle(handle);
	if (ret)
		goto err2;

	ret = input_open_device(handle);
	if (ret)
		goto err1;

	return 0;

err1:
	input_unregister_handle(handle);
err2:
	kfree(handle);
	return ret;
}

static void cpu_fp_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id cpu_fp_ids[] = {
	/* fingerprint sensor */
	{
		.flags = INPUT_DEVICE_ID_MATCH_KEYBIT,
		.keybit = { [BIT_WORD(FINGERPRINT_KEY)] = BIT_MASK(FINGERPRINT_KEY) },
	},
	{ },
};

static struct input_handler cpu_fp_input_handler = {
	.event      = cpu_fp_input_event,
	.connect    = cpu_fp_input_connect,
	.disconnect = cpu_fp_input_disconnect,
	.name       = "cpu_fp_handler",
	.id_table   = cpu_fp_ids,
};

static uint32_t get_boost_state(struct boost_policy *b)
{
	uint32_t state;

	spin_lock(&b->lock);
	state = b->state;
	spin_unlock(&b->lock);

	return state;
}

static void set_boost_bit(struct boost_policy *b, uint32_t state)
{
	spin_lock(&b->lock);
	b->state |= state;
	spin_unlock(&b->lock);
}

static void clear_boost_bit(struct boost_policy *b, uint32_t state)
{
	spin_lock(&b->lock);
	b->state &= ~state;
	spin_unlock(&b->lock);
}

static void unboost_all_cpus(struct boost_policy *b)
{
	struct fp_config *fp = &b->fp;

	/* Clear boost bit */
	clear_boost_bit(b, FINGERPRINT_BOOST);

	/* Clear cpus_to_boost bits for all CPUs */
	fp->cpus_to_boost = 0;

	/* Immediately unboost the online CPUs */
	update_online_cpu_policy();
}

static void update_online_cpu_policy(void)
{
	uint32_t cpu;

	/* Trigger cpufreq notifier for online CPUs */
	get_online_cpus();
	for_each_online_cpu(cpu)
		cpufreq_update_policy(cpu);
	put_online_cpus();
}

static ssize_t enabled_write(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct boost_policy *b = boost_policy_g;
	uint32_t data;
	int ret;

	ret = kstrtou32(buf, 10, &data);
	if (ret)
		return -EINVAL;

	if (data) {
		set_boost_bit(b, DRIVER_ENABLED);
	} else {
		clear_boost_bit(b, DRIVER_ENABLED);
		/* Stop everything */
		unboost_all_cpus(b);
	}

	return size;
}

static ssize_t enabled_read(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct boost_policy *b = boost_policy_g;

	return snprintf(buf, PAGE_SIZE, "%u\n",
				get_boost_state(b) & DRIVER_ENABLED);
}

static DEVICE_ATTR(enabled, 0644,
			enabled_read, enabled_write);

static struct attribute *cpu_fp_attr[] = {
	&dev_attr_enabled.attr,
	NULL
};

static struct attribute_group cpu_fp_attr_group = {
	.attrs = cpu_fp_attr,
};

static int sysfs_fp_init(void)
{
	struct kobject *kobj;
	int ret;

	kobj = kobject_create_and_add("fp_boost", kernel_kobj);
	if (!kobj) {
		pr_err("Failed to create kobject\n");
		return -ENOMEM;
	}

	ret = sysfs_create_group(kobj, &cpu_fp_attr_group);
	if (ret) {
		pr_err("Failed to create sysfs interface\n");
		kobject_put(kobj);
	}

	return ret;
}

static struct boost_policy *alloc_boost_policy(void)
{
	struct boost_policy *b;

	b = kzalloc(sizeof(*b), GFP_KERNEL);
	if (!b)
		return NULL;

	b->wq = alloc_workqueue("cpu_fp_wq", WQ_HIGHPRI, 0);
	if (!b->wq) {
		pr_err("Failed to allocate workqueue\n");
		goto free_b;
	}

	return b;

free_b:
	kfree(b);
	return NULL;
}

static int __init cpu_fp_init(void)
{
	struct boost_policy *b;
	int ret;
	touched = false;

	b = alloc_boost_policy();
	if (!b) {
		pr_err("Failed to allocate boost policy\n");
		return -ENOMEM;
	}

	spin_lock_init(&b->lock);

	INIT_DELAYED_WORK(&b->fp.boost_work, fp_boost_main);
	INIT_DELAYED_WORK(&b->fp.unboost_work, fp_unboost_main);

	/* Allow global boost config access */
	boost_policy_g = b;

	ret = input_register_handler(&cpu_fp_input_handler);
	if (ret) {
		pr_err("Failed to register input handler, err: %d\n", ret);
		goto free_mem;
	}

	ret = sysfs_fp_init();
	if (ret)
		goto input_unregister;

	cpufreq_register_notifier(&do_cpu_boost_nb, CPUFREQ_POLICY_NOTIFIER);

	return 0;

input_unregister:
	input_unregister_handler(&cpu_fp_input_handler);
free_mem:
	kfree(b);
	return ret;
}
late_initcall(cpu_fp_init);

