/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *
 * CPU Part
 *
 * CPU Hotplug driver for Exynos
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/cpu.h>
#include <linux/fb.h>
#include <linux/kthread.h>
#include <linux/pm_qos.h>
#include <linux/suspend.h>
#include <linux/debug-snapshot.h>

#include <soc/samsung/exynos-cpuhp.h>

#define CPUHP_USER_NAME_LEN	16

struct cpuhp_user {
	struct list_head	list;
	char name[CPUHP_USER_NAME_LEN];
	struct cpumask		online_cpus;
	int			type;
};

static struct {
	/* Control cpu hotplug operation */
	bool			enabled;

	/* flag for suspend */
	bool			suspended;

	/* flag for debug print */
	bool			debug;

	/* list head for requester */
	struct list_head	users;

	/* user for system */
	struct cpuhp_user	system_user;
	/* user for sysfs */
	struct cpuhp_user	sysfs_user;

	/* Synchronizes accesses to refcount and cpumask */
	struct mutex		lock;

	/* fast hotplug cpus */
	struct cpumask		fast_hp_cpus;

	/* user request mask */
	struct cpumask		online_cpus;

	/* cpuhp kobject */
	struct kobject		*kobj;
} cpuhp = {
	.lock = __MUTEX_INITIALIZER(cpuhp.lock),
};

/**********************************************************************************/
/*				   Helper					  */
/**********************************************************************************/
static int cpuhp_do(int fast_hp);

/*
 * Update pm_suspend status.
 * During suspend-resume, cpuhp driver is stop
 */
static inline void cpuhp_suspend(bool enable)
{
	/* This lock guarantees completion of cpuhp_do() */
	mutex_lock(&cpuhp.lock);
	cpuhp.suspended = enable;
	mutex_unlock(&cpuhp.lock);
}

/*
 * Update cpuhp enablestatus.
 * cpuhp driver is working when enabled big is TRUE
 */
static inline void cpuhp_enable(bool enable)
{
	mutex_lock(&cpuhp.lock);
	cpuhp.enabled = enable;
	mutex_unlock(&cpuhp.lock);
}

/* find user matched name. if return NULL, there is no user matched name */
static struct cpuhp_user* cpuhp_find_user(char *name)
{
	struct cpuhp_user *user;

	list_for_each_entry(user, &cpuhp.users, list)
		if (!strcmp(user->name, name))
			return user;
	return NULL;
}

/* update user's requesting  cpu mask */
static int cpuhp_update_user(char *name, struct cpumask mask, int type)
{
	struct cpuhp_user *user = cpuhp_find_user(name);

	if (!user)
		return -EINVAL;

	cpumask_copy(&user->online_cpus, &mask);
	user->type = type;

	return 0;
}

/* remove user from hotplug requesting user list */
int exynos_cpuhp_unregister(char *name, struct cpumask mask, int type)
{
	return 0;
}

/*
 * Register cpu-hp user
 * Users and IPs that want to use cpu-hp should register through this function.
 *	name: Must have a unique value, and panic will occur if you use an already
 *	      registered name.
 *	mask: The cpu mask that user wants to ONLINE and cpu OFF bits has more HIGH
 *	      priority than ONLINE bit. This mask is default cpu mask at registration
 *	      and it is reflected immediately after registration.
 *	type: cpu-hp type (0-> legacy cpu hp, 0xFA57-> fast cpu hp)
 */
int exynos_cpuhp_register(char *name, struct cpumask mask, int type)
{
	struct cpuhp_user *user;
	char buf[10];

	mutex_lock(&cpuhp.lock);

	/* check wether name is already register or not */
	if (cpuhp_find_user(name))
		panic("CPUHP: Failed to register cpuhp! this name already existed\n");

	/* allocate memory for new user */
	user = kzalloc(sizeof(struct cpuhp_user), GFP_KERNEL);
	if (!user) {
		mutex_unlock(&cpuhp.lock);
		return -ENOMEM;
	}

	/* init new user's information */
	cpumask_copy(&user->online_cpus, &mask);
	strcpy(user->name, name);
	user->type = type;
	/* register user list */
	list_add(&user->list, &cpuhp.users);

	scnprintf(buf, sizeof(buf), "%*pbl", cpumask_pr_args(&user->online_cpus));
	pr_info("CPUHP: reigstered new user(name:%s, mask:%s)\n", user->name, buf);;

	mutex_unlock(&cpuhp.lock);

	/* applying new user's request */
	return cpuhp_do(true);
}

/*
 * User requests cpu-hp.
 * The mask contains the requested cpu mask, and the type is hp operaton type.
 * The INTERSECTIONS of other user's request masks is determined by the final cpu-mask.
 */
int exynos_cpuhp_request(char *name, struct cpumask mask, int type)
{
	if (cpuhp_update_user(name, mask, type))
		return 0;

	/* use fast cpu hotplug sequence */
	if (type == FAST_HP)
		return cpuhp_do(true);

	return cpuhp_do(true);
}

/**********************************************************************************/
/*				 cpu hp operater				  */
/**********************************************************************************/
/* legacy hotplug in */
static int cpuhp_in(const struct cpumask *mask)
{
	int cpu, ret = 0;

	for_each_cpu(cpu, mask) {
		ret = cpu_up(cpu);
		if (ret) {
			/*
			 * If it fails to enable cpu,
			 * it cancels cpu hotplug request and retries later.
			 */
			pr_err("%s: Failed to hotplug in CPU%d with error %d\n",
								__func__, cpu, ret);
			break;
		}
	}

	return ret;
}

/* legacy hotplug out */
static int cpuhp_out(const struct cpumask *mask)
{
	int cpu, ret = 0;

	/*
	 * Reverse order of cpu,
	 * explore cpu7, cpu6, cpu5, ... cpu1
	 */
	for (cpu = nr_cpu_ids - 1; cpu > 0; cpu--) {
		if (!cpumask_test_cpu(cpu, mask))
			continue;

		ret = cpu_down(cpu);
		if (ret) {
			pr_err("%s: Failed to hotplug out CPU%d with error %d\n",
								__func__, cpu, ret);
			break;
		}
	}

	return ret;
}

/*
 * Return last target online cpu mask
 * Returns the cpu_mask INTERSECTIONS of all users in the user list.
 */
static struct cpumask cpuhp_get_online_cpus(void)
{
	struct cpumask mask;
	struct cpuhp_user *user;
	char buf[10];

	cpumask_setall(&mask);

	list_for_each_entry(user, &cpuhp.users, list)
		cpumask_and(&mask, &mask, &user->online_cpus);

	if (cpumask_empty(&mask) || !cpumask_test_cpu(0, &mask)) {
		scnprintf(buf, sizeof(buf), "%*pbl", cpumask_pr_args(&mask));
		panic("CPUHP: Online mask(%s) is wrong \n", buf);
	}

	return mask;
}

/*
 * Executes cpu_up
 * Run cpu_up according to the cpu control operation type.
 */
static int cpuhp_cpu_up(struct cpumask enable_cpus, int fast_hp)
{
	struct cpumask fast_cpus;
	int ret = 0;

	cpumask_clear(&fast_cpus);
	if (fast_hp)
		cpumask_copy(&fast_cpus, &cpuhp.fast_hp_cpus);

	cpumask_and(&fast_cpus, &enable_cpus, &fast_cpus);
	cpumask_andnot(&enable_cpus, &enable_cpus, &fast_cpus);
	if (!cpumask_empty(&enable_cpus))
		ret = cpuhp_in(&enable_cpus);
	if (ret)
		goto exit;

	if (fast_hp && !cpumask_empty(&fast_cpus))
		ret = cpus_up(fast_cpus);

	return ret;
exit:
	pr_info("failed to cpuhp_cpu_up(%d)\n", ret);
	return ret;
}

/*
 * Executes cpu_down
 * Run cpu_up according to the cpu control operation type.
 */
static int cpuhp_cpu_down(struct cpumask disable_cpus, int fast_hp)
{
	struct cpumask fast_cpus;
	int ret = 0;

	cpumask_clear(&fast_cpus);
	if (fast_hp)
		cpumask_copy(&fast_cpus, &cpuhp.fast_hp_cpus);

	cpumask_and(&fast_cpus, &disable_cpus, &fast_cpus);
	cpumask_andnot(&disable_cpus, &disable_cpus, &fast_cpus);
	if (fast_hp && !cpumask_empty(&fast_cpus))
		ret = cpus_down(fast_cpus);
	if (ret)
		goto exit;

	if (!cpumask_empty(&disable_cpus))
		ret = cpuhp_out(&disable_cpus);

	return ret;
exit:
	pr_info("failed to cpuhp_cpu_down(%d)\n", ret);
	return ret;
}

/* print cpu control informatoin for deubgging */
static void cpuhp_print_debug_info(struct cpumask online_cpus, int fast_hp)
{
	char new_buf[10], pre_buf[10];

	scnprintf(pre_buf, sizeof(pre_buf), "%*pbl", cpumask_pr_args(&cpuhp.online_cpus));
	scnprintf(new_buf, sizeof(new_buf), "%*pbl", cpumask_pr_args(&online_cpus));
	dbg_snapshot_printk("%s: %s -> %s fast_hp=%d\n", __func__, pre_buf, new_buf, fast_hp);

	/* print cpu control information */
	if (cpuhp.debug)
		pr_info("%s: %s -> %s fast_hp=%d\n", __func__, pre_buf, new_buf, fast_hp);
}

/*
 * cpuhp_do() is the main function for cpu hotplug. Only this function
 * enables or disables cpus, so all APIs in this driver call cpuhp_do()
 * eventually.
 */
static int cpuhp_do(int fast_hp)
{
	int ret = 0;
	struct cpumask online_cpus, enable_cpus, disable_cpus;

	mutex_lock(&cpuhp.lock);
	/*
	 * If cpu hotplug is disabled or suspended,
	 * cpuhp_do() do nothing.
	 */
	if (!cpuhp.enabled || cpuhp.suspended) {
		mutex_unlock(&cpuhp.lock);
		return 0;
	}

	online_cpus = cpuhp_get_online_cpus();
	cpuhp_print_debug_info(online_cpus, fast_hp);

	/* if there is no mask change, skip */
	if (cpumask_equal(&cpuhp.online_cpus, &online_cpus))
		goto out;

	/* get the enable cpu  mask for new online cpu */
	cpumask_andnot(&enable_cpus, &online_cpus, &cpuhp.online_cpus);
	/* get the disable cpu mask for new offline cpu */
	cpumask_andnot(&disable_cpus, &cpuhp.online_cpus, &online_cpus);

	if (!cpumask_empty(&enable_cpus))
		ret = cpuhp_cpu_up(enable_cpus, fast_hp);
	if (ret)
		goto out;

	if (!cpumask_empty(&disable_cpus))
		ret = cpuhp_cpu_down(disable_cpus, fast_hp);

	cpumask_copy(&cpuhp.online_cpus, &online_cpus);

out:
	mutex_unlock(&cpuhp.lock);

	return ret;
}

static int cpuhp_control(bool enable)
{
	struct cpumask mask;
	int ret = 0;

	if (enable) {
		cpuhp_enable(true);
		cpuhp_do(true);
	} else {
		mutex_lock(&cpuhp.lock);

		cpumask_setall(&mask);
		cpumask_andnot(&mask, &mask, cpu_online_mask);

		/*
		 * If it success to enable all CPUs, clear cpuhp.enabled flag.
		 * Since then all hotplug requests are ignored.
		 */
		ret = cpuhp_in(&mask);
		if (!ret) {
			/*
			 * In this position, can't use cpuhp_enable()
			 * because already taken cpuhp.lock
			 */
			cpuhp.enabled = false;
		} else {
			pr_err("Fail to disable cpu hotplug, please try again\n");
		}

		mutex_unlock(&cpuhp.lock);
	}

	return ret;
}

/**********************************************************************************/
/*				        SYSFS					  */
/**********************************************************************************/

/*
 * User can change the number of online cpu by using min_online_cpu and
 * max_online_cpu sysfs node. User input minimum and maxinum online cpu
 * to this node as below:
 *
 * #echo mask > /sys/power/cpuhp/set_online_cpu
 */
#define STR_LEN 6
#define attr_online_cpu(name)							\
static ssize_t show_##name##_online_cpu(struct kobject *kobj,			\
	struct kobj_attribute *attr, char *buf)					\
{										\
	unsigned int online_cpus;							\
	online_cpus = *(unsigned int *)cpumask_bits(&cpuhp.sysfs_user.online_cpus);	\
	return snprintf(buf, 30, #name " online cpu : 0x%x\n", online_cpus);	\
}										\
										\
static ssize_t store_##name##_online_cpu(struct kobject *kobj,			\
	struct kobj_attribute *attr, const char *buf,				\
	size_t count)								\
{										\
	char str[STR_LEN];							\
	int i;									\
	struct cpumask online_cpus;						\
										\
	if (strlen(buf) >= STR_LEN)						\
		return -EINVAL;							\
										\
	if (!sscanf(buf, "%s", str))						\
		return -EINVAL;							\
	if (str[0] == '0' && str[1] == 'x')					\
		for (i = 0; i+2 < STR_LEN; i++) {				\
			str[i] = str[i + 2];					\
			str[i+2] = '\n';					\
		}								\
	cpumask_parse(str, &online_cpus);					\
	if (!cpumask_test_cpu(0, &online_cpus)) {				\
		pr_warn("wrong format\n");					\
		return -EINVAL;							\
	}									\
	cpumask_copy(&cpuhp.sysfs_user.online_cpus, &online_cpus);		\
	cpuhp_do(true);								\
										\
	return count;								\
}										\
										\
static struct kobj_attribute cpuhp_##name##_online_cpu =			\
__ATTR(name##_online_cpu, 0644,							\
	show_##name##_online_cpu, store_##name##_online_cpu)

attr_online_cpu(set);

/*
 * It shows cpuhp driver requested online_cpu
 *
 * #cat /sys/power/cpuhp/online_cpu
 */
static ssize_t show_online_cpu(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	unsigned int online_cpus;
	online_cpus = *(unsigned int *)cpumask_bits(&cpuhp.online_cpus);

	return snprintf(buf, 30, "online cpu: 0x%x\n", online_cpus);
}

/*
 * It shows users information(name, requesting cpu_mask, type)
 * registered in cpu-hp user_list
 *
 * #cat /sys/power/cpuhp/users
 */
static ssize_t show_users(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	unsigned int online_cpus;							\
	struct cpuhp_user *user;
	ssize_t ret = 0;

	list_for_each_entry(user, &cpuhp.users, list) {
		online_cpus = *(unsigned int *)cpumask_bits(&user->online_cpus);
		ret += scnprintf(&buf[ret], 30, "%s: (0x%x)\n", user->name, online_cpus);
	}

	return ret;
}

/*
 * User can control the cpu hotplug operation as below:
 *
 * #echo 1 > /sys/power/cpuhp/enabled => enable
 * #echo 0 > /sys/power/cpuhp/enabled => disable
 *
 * If enabled become 0, hotplug driver enable the all cpus and no hotplug
 * operation happen from hotplug driver.
 */

static ssize_t show_enable(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 10, "%d\n", cpuhp.enabled);
}

static ssize_t store_enable(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf,
		size_t count)
{
	int input;

	if (!sscanf(buf, "%d", &input))
		return -EINVAL;

	cpuhp_control(!!input);

	return count;
}

/*
 * User can control en/disable debug mode
 *
 * #echo 1 > /sys/power/cpuhp/debug => enable
 * #echo 0 > /sys/power/cpuhp/debug => disable
 *
 * When it is enabled, information is printed every time there is a cpu control
 */
static ssize_t show_debug(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 10, "%d\n", cpuhp.debug);
}

static ssize_t store_debug(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf,
		size_t count)
{
	int input;

	if (!sscanf(buf, "%d", &input))
		return -EINVAL;

	cpuhp.debug = !!input;

	return count;
}

static struct kobj_attribute cpuhp_enabled =
__ATTR(enabled, 0644, show_enable, store_enable);
static struct kobj_attribute cpuhp_debug =
__ATTR(debug, 0644, show_debug, store_debug);
static struct kobj_attribute cpuhp_online_cpu =
__ATTR(online_cpu, 0444, show_online_cpu, NULL);
static struct kobj_attribute cpuhp_users =
__ATTR(users, 0444, show_users, NULL);

static struct attribute *cpuhp_attrs[] = {
	&cpuhp_online_cpu.attr,
	&cpuhp_set_online_cpu.attr,
	&cpuhp_enabled.attr,
	&cpuhp_debug.attr,
	&cpuhp_users.attr,
	NULL,
};

static const struct attribute_group cpuhp_group = {
	.attrs = cpuhp_attrs,
};

/**********************************************************************************/
/*				        PM_NOTI					  */
/**********************************************************************************/
static int exynos_cpuhp_pm_notifier(struct notifier_block *notifier,
				       unsigned long pm_event, void *v)
{
	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
		cpuhp_suspend(true);
		break;

	case PM_POST_SUSPEND:
		cpuhp_suspend(false);
		cpuhp_do(true);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block exynos_cpuhp_nb = {
	.notifier_call = exynos_cpuhp_pm_notifier,
};

static void __init cpuhp_dt_init(void)
{
	struct device_node *np = of_find_node_by_name(NULL, "cpuhp");
	const char *buf;

	if (of_property_read_string(np, "fast_hp_cpus", &buf)) {
		pr_info("fast_hp_cpus property is omitted!\n");
		return;
	}
	cpulist_parse(buf, &cpuhp.fast_hp_cpus);

	return;
}

/**********************************************************************************/
/*				        INIT					  */
/**********************************************************************************/
extern struct cpumask early_cpu_mask;
static void __init cpuhp_user_init(void)
{
	struct cpumask mask;

	/* init user list */
	INIT_LIST_HEAD(&cpuhp.users);

	cpumask_copy(&mask, cpu_possible_mask);
	cpumask_and(&mask, &mask, &early_cpu_mask);

	/* register user for SYSFS */
	cpumask_copy(&cpuhp.system_user.online_cpus, &mask);
	strcpy(cpuhp.system_user.name, "SYSTEM");
	cpuhp.system_user.type = 0;
	list_add(&cpuhp.system_user.list, &cpuhp.users);

	/* register user for SYSTEM */
	cpumask_copy(&cpuhp.sysfs_user.online_cpus, &mask);
	strcpy(cpuhp.sysfs_user.name, "SYSFS");
	cpuhp.sysfs_user.type = 0;
	list_add(&cpuhp.sysfs_user.list, &cpuhp.users);

	cpumask_copy(&cpuhp.online_cpus, cpu_online_mask);
}

static void __init cpuhp_sysfs_init(void)
{
	cpuhp.kobj = kobject_create_and_add("cpuhp", power_kobj);
	if (!cpuhp.kobj) {
		pr_err("Fail to create cpuhp kboject\n");
		return;
	}

	/* Create /sys/power/cpuhotplug */
	if (sysfs_create_group(cpuhp.kobj, &cpuhp_group)) {
		pr_err("Fail to create cpuhp group\n");
		return;
	}

	/* link cpuhotplug directory to /sys/devices/system/cpu/cpuhp */
	if (sysfs_create_link(&cpu_subsys.dev_root->kobj, cpuhp.kobj, "cpuhp"))
		pr_err("Fail to link cpuctrl directory");
}

static int __init cpuhp_init(void)
{
	/* Parse data from device tree */
	cpuhp_dt_init();

	/* Initialize pm_qos request and handler */
	cpuhp_user_init();

	/* Create sysfs */
	cpuhp_sysfs_init();

	/* register pm notifier */
	register_pm_notifier(&exynos_cpuhp_nb);

	/* Enable cpuhp */
	cpuhp_enable(true);

	return 0;
}
arch_initcall(cpuhp_init);
