/*
 * Copyright (C) 2019 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/moduleparam.h>

#include <net/ip.h>
#include <linux/inet.h>

#include <linux/sched.h>

// ++ file write
#include <linux/fs.h>
#include <linux/uaccess.h>
// -- file write

#include "mcps_device.h"
#include "mcps_sauron.h"
#include "mcps_debug.h"

#if defined(CONFIG_MCPS_ICGB) && \
	!defined(CONFIG_KLAT) && \
	(defined(CONFIG_SOC_EXYNOS9830) || defined(CONFIG_SOC_EXYNOS9820))
#define MCPS_IF_ADDR_MAX 8
static __be32 __mcps_in_addr[MCPS_IF_ADDR_MAX];
static struct in6_addr __mcps_in6_addr[MCPS_IF_ADDR_MAX];

int check_mcps_in6_addr(struct in6_addr *addr)
{
	return (ipv6_addr_equal(addr, &__mcps_in6_addr[0])
		 || ipv6_addr_equal(addr, &__mcps_in6_addr[1])
		 || ipv6_addr_equal(addr, &__mcps_in6_addr[2])
		 || ipv6_addr_equal(addr, &__mcps_in6_addr[3])
		 || ipv6_addr_equal(addr, &__mcps_in6_addr[4])
		 || ipv6_addr_equal(addr, &__mcps_in6_addr[5])
		 || ipv6_addr_equal(addr, &__mcps_in6_addr[6])
		 || ipv6_addr_equal(addr, &__mcps_in6_addr[7])
		 );
}

int check_mcps_in_addr(__be32 addr)
{
	return (addr == __mcps_in_addr[0]
		 || addr == __mcps_in_addr[1]
		 || addr == __mcps_in_addr[2]
		 || addr == __mcps_in_addr[3]
		 || addr == __mcps_in_addr[4]
		 || addr == __mcps_in_addr[5]
		 || addr == __mcps_in_addr[6]
		 || addr == __mcps_in_addr[7]
		 );
}

static int set_mcps_in_addr(const char *buf, const struct kernel_param *kp)
{
	unsigned int num = 0;

	int len = strlen(buf);

	char *substr = NULL;
	char *copy = NULL;
	char *temp = NULL;

	copy = kstrdup(buf, GFP_KERNEL);
	if (!copy)
		return len;

	temp = copy;
	substr = strsep(&temp, "@");
	if (!substr) {
		MCPS_ERR("No device num substring\n");
		goto error;
	}

	if (kstrtouint(substr, 0, &num)) {
		MCPS_ERR("Fail to parse uint\n");
		goto error;
	}

	if (num >= MCPS_IF_ADDR_MAX) {
		MCPS_ERR("Wrong number of netdevice : %u\n", num);
		goto error;
	}

	substr = strsep(&temp, "@");
	if (!substr) {
		MCPS_ERR("No ip addr substring\n");
		goto error;
	}

	__mcps_in_addr[num] = in_aton(substr);

error:
	kfree(copy);

	return len;
}

static int get_mcps_in_addr(char *buf, const struct kernel_param *kp)
{
	int i = 0;
	int len = 0;

	for (i = 0; i < MCPS_IF_ADDR_MAX; i++) {
		len += scnprintf(buf + len, PAGE_SIZE, "%pI4\n", &__mcps_in_addr[i]);
	}
	return len;
};

const struct kernel_param_ops mcps_in_addr_ops = {
		.set = &set_mcps_in_addr,
		.get = &get_mcps_in_addr
};

int mcps_in_addr __read_mostly;
module_param_cb(mcps_in_addr,
				&mcps_in_addr_ops,
				&mcps_in_addr,
				0640);

int in6_pton(const char *src, int srclen, u8 *dst, int delim, const char **end);

static int set_mcps_in6_addr(const char *buf, const struct kernel_param *kp)
{
	struct in6_addr val;
	unsigned int num = 0;

	int len = strlen(buf);

	char *substr = NULL;
	char *copy = NULL;
	char *temp = NULL;

	copy = kstrdup(buf, GFP_KERNEL);
	if (!copy)
		return len;

	temp = copy;
	substr = strsep(&temp, "@");
	if (!substr) {
		MCPS_ERR("No device num substring\n");
		goto error;
	}

	if (kstrtouint(substr, 0, &num)) {
		MCPS_ERR("Fail to parse uint\n");
		goto error;
	}

	if (num >= MCPS_IF_ADDR_MAX) {
		MCPS_ERR("Wrong number of netdevice : %u\n", num);
		goto error;
	}

	substr = strsep(&temp, "@");
	if (!substr) {
		MCPS_ERR("No ip addr substring\n");
		goto error;
	}

	if (in6_pton(substr, -1, val.s6_addr, -1, NULL) == 0) {
		goto error;
	}
	__mcps_in6_addr[num] = val;

error:
	kfree(copy);

	return len;
}

static int get_mcps_in6_addr(char *buf, const struct kernel_param *kp)
{
	int i = 0;
	int len = 0;

	for (i = 0; i < MCPS_IF_ADDR_MAX; i++) {
		len += scnprintf(buf + len, PAGE_SIZE, "%pI6\n", &__mcps_in6_addr[i]);
	}
	return len;
};

const struct kernel_param_ops mcps_in6_addr_ops = {
		.set = &set_mcps_in6_addr,
		.get = &get_mcps_in6_addr
};

int mcps_in6_addr __read_mostly;
module_param_cb(mcps_in6_addr,
				&mcps_in6_addr_ops,
				&mcps_in6_addr,
				0640);
#endif // #if defined(CONFIG_MCPS_ICGB) && !defined(CONFIG_KLAT) && (defined(CONFIG_SOC_EXYNOS9820) || defined(CONFIG_SOC_EXYNOS9630) || defined(CONFIG_SOC_EXYNOS9830))

int set_mcps_flush(const char *buf, const struct kernel_param *kp)
{
	return 0;
}

int get_mcps_flush(char *buf, const struct kernel_param *kp)
{
	size_t len = 0;

	return len;
}

const struct kernel_param_ops mcps_flush_ops = {
		.set = &set_mcps_flush,
		.get = &get_mcps_flush
};

int mcps_flush_flag __read_mostly;
module_param_cb(mcps_flush,
		&mcps_flush_ops,
		&mcps_flush_flag,
		0640);

spinlock_t lock_arps_meta;
#define MCPS_ARPS_META_STATIC 0
#define MCPS_ARPS_META_DYNAMIC 1
#define MCPS_ARPS_META_NEWFLOW 2

struct arps_meta __rcu *static_arps;
struct arps_meta __rcu *dynamic_arps;
struct arps_meta __rcu *newflow_arps;

cpumask_var_t cpu_big_mask;
cpumask_var_t cpu_little_mask;
cpumask_var_t cpu_mid_mask;

cpumask_var_t mcps_cpu_online_mask;

struct arps_meta *get_arps_rcu(void)
{
	struct arps_meta *arps = rcu_dereference(dynamic_arps);

	if (arps) {
		return arps;
	}
	arps = rcu_dereference(static_arps);
	return arps;
}

struct arps_meta *get_newflow_rcu(void)
{
	struct arps_meta *arps = rcu_dereference(newflow_arps);

	if (arps) {
		return arps;
	}

	return get_arps_rcu();
}

static int create_arps_mask(const char *buf, cpumask_var_t *mask)
{
	int len, err;

	if (!zalloc_cpumask_var(mask, GFP_KERNEL)) {
		MCPS_DEBUG("failed to alloc kmem\n");
		return -ENOMEM;
	}

	len = strlen(buf);
	err = bitmap_parse(buf, len, cpumask_bits(*mask), NR_CPUS);
	if (err) {
		MCPS_DEBUG("failed to parse %s\n", buf);
		free_cpumask_var(*mask);
		return -EINVAL;
	}

	MCPS_DEBUG("%*pb\n", cpumask_pr_args(*mask));

	return 0;
}

static struct rps_map *create_arps_map_and(cpumask_var_t mask, const cpumask_var_t ref)
{
	int i, cpu;
	struct rps_map *map;

	MCPS_DEBUG("%*pb | %*pb\n", cpumask_pr_args(mask), cpumask_pr_args(ref));

	map = kzalloc(max_t(unsigned long,
	RPS_MAP_SIZE(cpumask_weight(mask)), L1_CACHE_BYTES),
	GFP_KERNEL);

	if (!map) {
		MCPS_DEBUG("failed to alloc kmem\n");
		return NULL;
	}

	i = 0;
	for_each_cpu_and(cpu, mask, ref) {
		if (cpu < 0)
			continue;
		map->cpus[i++] = cpu;
	}
	map->len = i;

	MCPS_DEBUG("map len -> %d\n", map->len);

	return map;
}

static struct rps_map *create_arps_map(cpumask_var_t mask)
{
	return create_arps_map_and(mask, cpu_possible_mask);
}

static void release_arps_meta(struct arps_meta *meta)
{
	if (!meta)
		return;

	if (cpumask_available(meta->mask))
		free_cpumask_var(meta->mask);
	if (ARPS_MAP(meta, ALL_CLUSTER))
		kfree(ARPS_MAP(meta, ALL_CLUSTER));
	if (ARPS_MAP(meta, LIT_CLUSTER))
		kfree(ARPS_MAP(meta, LIT_CLUSTER));
	if (ARPS_MAP(meta, BIG_CLUSTER))
		kfree(ARPS_MAP(meta, BIG_CLUSTER));
	if (ARPS_MAP(meta, MID_CLUSTER))
		kfree(ARPS_MAP(meta, MID_CLUSTER));

	kfree(meta);
}

struct arps_meta *create_arps_meta(const char *buf)
{
	struct arps_meta *meta;

	meta = (struct arps_meta *)kzalloc(sizeof(struct arps_meta), GFP_KERNEL);
	if (!meta) {
		MCPS_DEBUG("Fail to zalloc\n");
		return NULL;
	}

	if (create_arps_mask(buf, &meta->mask)) {
		MCPS_DEBUG("Fail create_arps_mask\n");
		goto fail;
	}

	if (!cpumask_weight(meta->mask)) {
		MCPS_DEBUG(" : Fail cpumask_weight\n");
		goto fail;
	}

	ARPS_MAP(meta, ALL_CLUSTER) = create_arps_map(meta->mask);
	if (!ARPS_MAP(meta, ALL_CLUSTER))
		goto fail;
	ARPS_MAP(meta, LIT_CLUSTER) = create_arps_map_and(meta->mask, cpu_little_mask);
	if (!ARPS_MAP(meta, LIT_CLUSTER))
		goto fail;
	ARPS_MAP(meta, BIG_CLUSTER) = create_arps_map_and(meta->mask, cpu_big_mask);
	if (!ARPS_MAP(meta, BIG_CLUSTER))
		goto fail;
	ARPS_MAP(meta, MID_CLUSTER) = create_arps_map_and(meta->mask, cpu_mid_mask);
	if (!ARPS_MAP(meta, MID_CLUSTER))
		goto fail;

	init_rcu_head(&meta->rcu);

	return meta;
fail:
	release_arps_meta(meta);
	return NULL;
}

int update_arps_meta(const char *buf, int flag)
{
	struct arps_meta *arps, *old = arps = NULL;

	int len = strlen(buf);

	if (len == 0)
		return 0;

	arps = create_arps_meta(buf);
	switch (flag) {
	case MCPS_ARPS_META_STATIC:
		spin_lock(&lock_arps_meta);
		old = rcu_dereference_protected(static_arps,
			lockdep_is_held(&lock_arps_meta));
		rcu_assign_pointer(static_arps, arps);
		spin_unlock(&lock_arps_meta);
	break;
	case MCPS_ARPS_META_DYNAMIC:
		spin_lock(&lock_arps_meta);
		old = rcu_dereference_protected(dynamic_arps,
				lockdep_is_held(&lock_arps_meta));
		rcu_assign_pointer(dynamic_arps, arps);
		spin_unlock(&lock_arps_meta);
	break;
	case MCPS_ARPS_META_NEWFLOW:
		spin_lock(&lock_arps_meta);
		old = rcu_dereference_protected(newflow_arps,
				lockdep_is_held(&lock_arps_meta));
		rcu_assign_pointer(newflow_arps, arps);
		spin_unlock(&lock_arps_meta);
	break;
	default:
		if(arps) {
			release_arps_meta(arps);
		}
	break;
	}

	if (old) {
		synchronize_rcu();
		release_arps_meta(old);
	}

	return len;
}

int get_arps_meta(char *buf, int flag)
{
	int len = 0;
	struct arps_meta *arps = NULL;

	rcu_read_lock();
	switch (flag) {
	case MCPS_ARPS_META_STATIC:
		arps = rcu_dereference(static_arps);
	break;
	case MCPS_ARPS_META_DYNAMIC:
		arps = rcu_dereference(dynamic_arps);
	break;
	case MCPS_ARPS_META_NEWFLOW:
		arps = rcu_dereference(newflow_arps);
	break;
	}

	if (!arps)
		goto error;

	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
		len += snprintf(buf + len, PAGE_SIZE, "%*pb", cpumask_pr_args(arps->mask));
	#else
		len += cpumask_scnprintf(buf + len, PAGE_SIZE, arps->mask);
	#endif
	len += snprintf(buf + len, PAGE_SIZE, "\n");
	len += snprintf(buf + len, PAGE_SIZE, "[%d|%d|%d|%d]",
		ARPS_MAP(arps, ALL_CLUSTER)->len, ARPS_MAP(arps, LIT_CLUSTER)->len, ARPS_MAP(arps, BIG_CLUSTER)->len, ARPS_MAP(arps, MID_CLUSTER)->len);
	len += snprintf(buf + len, PAGE_SIZE, "\n");

	rcu_read_unlock();

	if (PAGE_SIZE - len < 3) {
		return -EINVAL;
	}

	return len;
error:
	rcu_read_unlock();
	len += snprintf(buf + len, PAGE_SIZE, "0\n[0|0|0|0]\n");
	return len;
}

void init_mcps_arps_meta(void)
{
	struct arps_meta *old;

	lock_arps_meta = __SPIN_LOCK_UNLOCKED(lock_arps_meta);

	//online mask
	if (!zalloc_cpumask_var(&mcps_cpu_online_mask, GFP_KERNEL)) {
		MCPS_INFO("Fail to zalloc mcps_cpu_online_mask\n");
		return;
	}
	cpumask_copy(mcps_cpu_online_mask, cpu_online_mask);

	//big/lit mask
#if defined(CONFIG_SOC_EXYNOS9830) || defined(CONFIG_SOC_EXYNOS9820)
	create_arps_mask("f0", &cpu_big_mask);
	create_arps_mask("f", &cpu_little_mask);
	create_arps_mask("30", &cpu_mid_mask);
#endif

	//MCPS_ARPS_META_STATIC
	spin_lock(&lock_arps_meta);
	old = rcu_dereference_protected(static_arps,
			lockdep_is_held(&lock_arps_meta));
	rcu_assign_pointer(static_arps, NULL);
	spin_unlock(&lock_arps_meta);

	if (old) {
		synchronize_rcu();
		release_arps_meta(old);
	}

	//MCPS_ARPS_META_DYNAMIC
	spin_lock(&lock_arps_meta);
	old = rcu_dereference_protected(dynamic_arps,
			lockdep_is_held(&lock_arps_meta));
	rcu_assign_pointer(dynamic_arps, NULL);
	spin_unlock(&lock_arps_meta);

	if (old) {
		synchronize_rcu();
		release_arps_meta(old);
	}

	//MCPS_ARPS_META_NEWFLOW
	spin_lock(&lock_arps_meta);
	old = rcu_dereference_protected(newflow_arps,
			lockdep_is_held(&lock_arps_meta));
	rcu_assign_pointer(newflow_arps, NULL);
	spin_unlock(&lock_arps_meta);

	if (old) {
		synchronize_rcu();
		release_arps_meta(old);
	}
}

void release_mcps_arps_meta(void)
{
	struct arps_meta *old;

	free_cpumask_var(cpu_big_mask);
	free_cpumask_var(cpu_little_mask);
	free_cpumask_var(cpu_mid_mask);

	//MCPS_ARPS_META_STATIC
	spin_lock(&lock_arps_meta);
	old = rcu_dereference_protected(static_arps,
			lockdep_is_held(&lock_arps_meta));
	rcu_assign_pointer(static_arps, NULL);
	spin_unlock(&lock_arps_meta);

	if (old) {
		synchronize_rcu();
		release_arps_meta(old);
	}

	//MCPS_ARPS_META_DYNAMIC
	spin_lock(&lock_arps_meta);
	old = rcu_dereference_protected(dynamic_arps,
			lockdep_is_held(&lock_arps_meta));
	rcu_assign_pointer(dynamic_arps, NULL);
	spin_unlock(&lock_arps_meta);

	if (old) {
		synchronize_rcu();
		release_arps_meta(old);
	}

	//MCPS_ARPS_META_NEWFLOW
	spin_lock(&lock_arps_meta);
	old = rcu_dereference_protected(newflow_arps,
			lockdep_is_held(&lock_arps_meta));
	rcu_assign_pointer(newflow_arps, NULL);
	spin_unlock(&lock_arps_meta);

	if (old) {
		synchronize_rcu();
		release_arps_meta(old);
	}
}

#define PATH_RFS_BUCKETS "/proc/sys/net/core/rps_sock_flow_entries"
void mcps_file_write(const char *p, int flag, const char *v)
{
	struct file *f;
	mm_segment_t old_fs;

	MCPS_DEBUG("%s -> %s\n", v, p);

	old_fs = get_fs();
	set_fs(get_ds());

	f = filp_open(p, flag, 0644);
	if (IS_ERR(f)) {
		MCPS_DEBUG("fail to open file [%s]\n", p);
		goto end;
	}

	vfs_write(f, v, strlen(v), &f->f_pos);

	filp_close(f, NULL);
end:
	set_fs(old_fs);
}

int set_mcps_rfs_buckets(const char *val, const struct kernel_param *kp)
{
	int len = strlen(val);

	if (len == 0) {
		MCPS_DEBUG("len 0\n");
		return len;
	}

	mcps_file_write(PATH_RFS_BUCKETS, O_RDWR, val);
	return len;
}

const struct kernel_param_ops mcps_rfs_buckets_ops = {
		.set = &set_mcps_rfs_buckets,
};

int dummy_mcps_rfs_buckets;
module_param_cb(mcps_rfs_buckets,
		&mcps_rfs_buckets_ops,
		&dummy_mcps_rfs_buckets,
		0640);
// -- file write

int get_mcps_heavy_flow(char *buffer, const struct kernel_param *kp)
{
	int len = 0;
	int cpu = 0;
	struct sauron *sauron = &mcps->sauron;
	int pps[NR_CPUS] = {0,};

	if (!mcps) {
		MCPS_DEBUG("fail - mcps null\n");
		return 0;
	}

	local_bh_disable();
	rcu_read_lock();
	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		struct eye *flow = pick_heavy(sauron, cpu);

		if (flow) {
			pps[cpu] = flow->pps;
		} else {
			pps[cpu] = -1;
		}
	}
	rcu_read_unlock();
	local_bh_enable();

	for (cpu = 0 ; cpu < NR_CPUS; cpu++) {
		len += scnprintf(buffer + len, PAGE_SIZE, "%d ", pps[cpu]);
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	len += scnprintf(buffer + len, PAGE_SIZE, "\n");
#endif

	return len;
}

const struct kernel_param_ops mcps_heavy_flow_ops = {
	.get = &get_mcps_heavy_flow,
};

int dummy_mcps_heavy_flows;
module_param_cb(mcps_heavy_flows,
		&mcps_heavy_flow_ops,
		&dummy_mcps_heavy_flows,
		0640);

int get_mcps_light_flow(char *buffer, const struct kernel_param *kp)
{
	int len = 0;
	int cpu = 0;
	struct sauron *sauron = &mcps->sauron;
	int pps[NR_CPUS] = {0,};

	if (!mcps) {
		MCPS_DEBUG("fail - mcps null\n");
		return 0;
	}

	local_bh_disable();
	rcu_read_lock();
	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		struct eye *flow = pick_light(sauron, cpu);

		if (flow) {
			pps[cpu] = flow->pps;
		} else {
			pps[cpu] = -1;
		}
	}
	rcu_read_unlock();
	local_bh_enable();

	for (cpu = 0 ; cpu < NR_CPUS; cpu++) {
		len += scnprintf(buffer + len, PAGE_SIZE, "%d ", pps[cpu]);
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	len += scnprintf(buffer + len, PAGE_SIZE, "\n");
#endif

	return len;
}

const struct kernel_param_ops mcps_light_flow_ops = {
	.get = &get_mcps_light_flow,
};

int dummy_mcps_light_flows;
module_param_cb(mcps_light_flows,
		&mcps_light_flow_ops,
		&dummy_mcps_light_flows,
		0640);

int set_mcps_move(const char *val, const struct kernel_param *kp)
{
	char *copy, *tmp, *sub;
	int len = strlen(val);
	int i, in[3] = {0,}; // from, to, option

	if (len == 0) {
		MCPS_DEBUG("%s - size 0\n", val);
		goto end;
	}

	i = 0;
	copy = (char *)kzalloc(sizeof(char) * len, GFP_KERNEL);
	if (!copy) {
		MCPS_DEBUG("set_mcps_move : fail to kzalloc\n");
		goto end;
	}
	memcpy(copy, val, sizeof(char) * len);

	tmp = copy;

	while ((sub = strsep(&tmp, " ")) != NULL) {
		int cpu, ret;

		if (i >= 3)
			break;

		ret = kstrtoint(sub, 0, &cpu);
		if (ret) {
			MCPS_DEBUG("kstrtoint fail\n");
			goto fail;
		}
		in[i] = cpu;
		i++;
	}

	if (i != 3) {
		MCPS_DEBUG("params are not satisfied.\n");
		goto fail;
	}
	MCPS_DEBUG("%d -> %d [%d]\n", in[0], in[1], in[2]);

	migrate_flow(in[0], in[1], in[2]);
fail:
	kfree(copy);
end:
	return len;
}

const struct kernel_param_ops mcps_move_ops = {
		.set = &set_mcps_move,
};

int dummy_mcps_moveif;
module_param_cb(mcps_move,
		&mcps_move_ops,
		&dummy_mcps_moveif,
		0640);

static int mcps_store_rps_map(struct netdev_rx_queue *queue, const char *buf, size_t len)
{
	struct rps_map *old_map, *map;
	cpumask_var_t mask;
	int err, cpu, i;
	static DEFINE_SPINLOCK(rps_map_lock);

	if (!alloc_cpumask_var(&mask, GFP_KERNEL)) {
		MCPS_DEBUG("failed to alloc_cpumask\n");
		return -ENOMEM;
	}

	err = bitmap_parse(buf, len, cpumask_bits(mask), nr_cpumask_bits);
	if (err) {
		free_cpumask_var(mask);
		MCPS_DEBUG("failed to parse bitmap\n");
		return err;
	}

	map = kzalloc(max_t(unsigned long,
		RPS_MAP_SIZE(cpumask_weight(mask)), L1_CACHE_BYTES),
		GFP_KERNEL);
	if (!map) {
		free_cpumask_var(mask);
		MCPS_DEBUG("failed to alloc kmem\n");
		return -ENOMEM;
	}

	i = 0;
	for_each_cpu_and(cpu, mask, cpu_online_mask) {
		if (cpu < 0)
			continue;
		map->cpus[i++] = cpu;
	}

	if (i) {
		map->len = i;
	} else {
		kfree(map);
		map = NULL;
	}

	MCPS_DEBUG("%*pb (%d)\n", cpumask_pr_args(mask), i);

	spin_lock(&rps_map_lock);
	old_map = rcu_dereference_protected(queue->rps_map,
		lockdep_is_held(&rps_map_lock));
	rcu_assign_pointer(queue->rps_map, map);

	if (map)
		static_key_slow_inc(&rps_needed);
	if (old_map)
		static_key_slow_dec(&rps_needed);

	spin_unlock(&rps_map_lock);

	if (old_map)
		kfree_rcu(old_map, rcu);

	free_cpumask_var(mask);
	return i;
}

int set_mcps_rps_config(const char *val, const struct kernel_param *kp)
{
	struct net_device *ndev;
	char *iface, *mask, *pstr;
	char copy[50];
	int ret;

	int len = 0;

	if (!val)
		return 0;

	scnprintf(copy, sizeof(copy), "%s", val);
	pstr = copy;

	iface = strsep(&pstr, ",");
	if (!iface)
		goto error;

	mask = strsep(&pstr, ",");
	if (!mask)
		goto error;

	ndev = dev_get_by_name(&init_net, iface);
	if (!ndev) {
		goto error;
	}

	ret = mcps_store_rps_map(ndev->_rx, mask, strlen(mask));
	dev_put(ndev);

	if (ret < 0) {
		MCPS_DEBUG("Fail %s(%s)\n", iface, mask);
		goto error;
	}

	MCPS_DEBUG("Successes %s(%s)\n", iface, mask);
error:
	return len;
}

const struct kernel_param_ops mcps_rps_config_ops = {
		.set = &set_mcps_rps_config,
};

int dummy_mcps_rps_config;
module_param_cb(mcps_rps_config,
		&mcps_rps_config_ops,
		&dummy_mcps_rps_config,
		0640);

int set_mcps_arps_cpu(const char *val, const struct kernel_param *kp)
{
	// int len = __set_mcps_cpu(val, &mcps->arps_map);
	size_t len = update_arps_meta(val, MCPS_ARPS_META_STATIC);
	return len;
}

int get_mcps_arps_cpu(char *buffer, const struct kernel_param *kp)
{
	// size_t len = 0;
	// len = __get_mcps_cpu(buffer, &mcps->arps_map);
	size_t len = get_arps_meta(buffer, MCPS_ARPS_META_STATIC);
	return len;
}

const struct kernel_param_ops mcps_arps_cpu_ops = {
		.set = &set_mcps_arps_cpu,
		.get = &get_mcps_arps_cpu,
};

int dummy_mcps_arps_cpu;
module_param_cb(mcps_arps_cpu,
		&mcps_arps_cpu_ops,
		&dummy_mcps_arps_cpu,
		0640);

int set_mcps_newflow_cpu(const char *val, const struct kernel_param *kp)
{
	size_t len = update_arps_meta(val, MCPS_ARPS_META_NEWFLOW);
	return len;
}

int get_mcps_newflow_cpu(char *buffer, const struct kernel_param *kp)
{
	size_t len = get_arps_meta(buffer, MCPS_ARPS_META_NEWFLOW);
	return len;
}

const struct kernel_param_ops mcps_newflow_cpu_ops = {
	.set = &set_mcps_newflow_cpu,
	.get = &get_mcps_newflow_cpu,
};

int dummy_mcps_newflow_cpu;
module_param_cb(mcps_newflow_cpu,
		&mcps_newflow_cpu_ops,
		&dummy_mcps_newflow_cpu,
		0640);

int set_mcps_dynamic_cpu(const char *val, const struct kernel_param *kp)
{
	// int len = __set_mcps_cpu(val, &mcps->dynamic_map);
	size_t len = update_arps_meta(val, MCPS_ARPS_META_DYNAMIC);
	return len;
}

int get_mcps_dynamic_cpu(char *buffer, const struct kernel_param *kp)
{
	// size_t len = __get_mcps_cpu(buffer, &mcps->dynamic_map);
	size_t len = get_arps_meta(buffer, MCPS_ARPS_META_DYNAMIC);
	return len;
}

const struct kernel_param_ops mcps_dynamic_cpu_ops = {
	.set = &set_mcps_dynamic_cpu,
	.get = &get_mcps_dynamic_cpu,
};

int dummy_mcps_dynamic_cpu;
module_param_cb(mcps_dynamic_cpu,
		&mcps_dynamic_cpu_ops,
		&dummy_mcps_dynamic_cpu,
		0640);

static DEFINE_SPINLOCK(lock_mcps_arps_config);
int set_mcps_arps_config(const char *val, const struct kernel_param *kp)
{
	struct arps_config *config = NULL;
	struct arps_config *old_config = NULL;
	char *token;
	int idx = 0;
	int len = strlen(val);

	char *tval, *tval_copy;

	tval = tval_copy = kstrdup(val, GFP_KERNEL);
	if (!tval) {
		MCPS_DEBUG("kstrdup - fail\n");
		goto error;
	}

	token = strsep(&tval_copy, " ");
	if (!token) {
		MCPS_DEBUG("token - fail\n");
		goto error;
	}

	config = (struct arps_config *) kzalloc(sizeof(struct arps_config), GFP_KERNEL);
	if (!config)
		goto error;

	while (token) {
		if (idx >= NUM_FACTORS)
			break;

		if (kstrtouint(token, 0, &config->weights[idx])) {
			MCPS_DEBUG("kstrtouint - fail %s\n", token);
			goto error;
		}
		token = strsep(&tval_copy, " ");
		idx++;
	}

	spin_lock(&lock_mcps_arps_config);
	old_config = rcu_dereference_protected(mcps->arps_config,
				lockdep_is_held(&lock_mcps_arps_config));
	rcu_assign_pointer(mcps->arps_config, config);
	spin_unlock(&lock_mcps_arps_config);

	if (old_config) {
		synchronize_rcu();
		kfree(old_config);
	}

	kfree(tval);

return len;

error:
	kfree(tval);
	kfree(config);

return len;
}

int get_mcps_arps_config(char *buffer, const struct kernel_param *kp)
{
	struct arps_config *config;
	size_t len = 0;

	int i = 0;

	rcu_read_lock(); // rcu read critical section start
	config = rcu_dereference(mcps->arps_config);

	if (!config) {
		rcu_read_unlock(); // rcu read critical section pause.

		config = (struct arps_config *) kzalloc(sizeof(struct arps_config), GFP_KERNEL);
		if (!config)
			return len;

		init_arps_config(config);
		spin_lock(&lock_mcps_arps_config);
		rcu_assign_pointer(mcps->arps_config, config);
		spin_unlock(&lock_mcps_arps_config);
		rcu_read_lock(); // rcu read critical section restart

		config = rcu_dereference(mcps->arps_config);
	}

	for (i = 0 ; i < NUM_FACTORS; i++) {
		len += scnprintf(buffer + len, PAGE_SIZE, "%u ",
					config->weights[i]);
	}

	rcu_read_unlock();
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	len += scnprintf(buffer + len, PAGE_SIZE, "\n");
#endif
	return len;
}

const struct kernel_param_ops mcps_arps_config_ops = {
		.set = &set_mcps_arps_config,
		.get = &get_mcps_arps_config,
};

int dummy_mcps_arps_config;
module_param_cb(mcps_arps_config,
		&mcps_arps_config_ops,
		&dummy_mcps_arps_config,
		0640);


int get_mcps_enqueued(char *buffer, const struct kernel_param *kp)
{
	int cpu = 0;
	size_t len = 0;
	struct mcps_pantry *pantry;

	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		pantry = &per_cpu(mcps_pantries, cpu);
		len += scnprintf(buffer + len, PAGE_SIZE, "%u ", pantry->enqueued);
	}

#ifdef CONFIG_MCPS_GRO_ENABLE
	len += scnprintf(buffer + len, PAGE_SIZE, "\n");

	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		pantry = &per_cpu(mcps_gro_pantries, cpu);
		len += scnprintf(buffer + len, PAGE_SIZE, "%u ", pantry->enqueued);
	}
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	len += scnprintf(buffer + len, PAGE_SIZE, "\n");
#endif
	return len;
}

const struct kernel_param_ops mcps_enqueued_ops = {
		.get = &get_mcps_enqueued,
};

int dummy_mcps_enqueued;
module_param_cb(mcps_stat_enqueued,
		&mcps_enqueued_ops,
		&dummy_mcps_enqueued,
		0640);

int get_mcps_processed(char *buffer, const struct kernel_param *kp)
{
	int cpu = 0;
	size_t len = 0;
	struct mcps_pantry *pantry;

	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		pantry = &per_cpu(mcps_pantries, cpu);
		len += scnprintf(buffer + len, PAGE_SIZE, "%u ", pantry->processed);
	}

#ifdef CONFIG_MCPS_GRO_ENABLE
	len += scnprintf(buffer + len, PAGE_SIZE, "\n");

	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		pantry = &per_cpu(mcps_gro_pantries, cpu);
		len += scnprintf(buffer + len, PAGE_SIZE, "%u ", pantry->processed);
	}
	len += scnprintf(buffer + len, PAGE_SIZE, "\n");

	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		pantry = &per_cpu(mcps_gro_pantries, cpu);
		len += scnprintf(buffer + len, PAGE_SIZE, "%u ", pantry->gro_processed);
	}
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	len += scnprintf(buffer + len, PAGE_SIZE, "\n");
#endif
	return len;
}

const struct kernel_param_ops mcps_processed_ops = {
		.get = &get_mcps_processed,
};

int dummy_mcps_processed;
module_param_cb(mcps_stat_processed,
		&mcps_processed_ops,
		&dummy_mcps_processed,
		0640);

int get_mcps_dropped(char *buffer, const struct kernel_param *kp)
{
	int cpu = 0;
	size_t len = 0;
	struct mcps_pantry *pantry;

	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		pantry = &per_cpu(mcps_pantries, cpu);
		len += scnprintf(buffer + len, PAGE_SIZE, "%u", pantry->dropped);
		if (cpu == NR_CPUS - 1)
			break;
		len += scnprintf(buffer + len, PAGE_SIZE, " ");
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	len += scnprintf(buffer + len, PAGE_SIZE, "\n");
#endif
	return len;
}

const struct kernel_param_ops mcps_dropped_ops = {
		.get = &get_mcps_dropped,
};

int dummy_mcps_dropped;
module_param_cb(mcps_stat_dropped,
		&mcps_dropped_ops,
		&dummy_mcps_dropped,
		0640);

int get_mcps_ignored(char *buffer, const struct kernel_param *kp)
{
	int cpu = 0;
	size_t len = 0;
	struct mcps_pantry *pantry;

	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		pantry = &per_cpu(mcps_pantries, cpu);
		len += scnprintf(buffer + len, PAGE_SIZE, "%u", pantry->ignored);
		if (cpu == NR_CPUS - 1)
			break;
		len += scnprintf(buffer + len, PAGE_SIZE, " ");
	}

#ifdef CONFIG_MCPS_GRO_ENABLE
	len += scnprintf(buffer + len, PAGE_SIZE, "\n");
	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		pantry = &per_cpu(mcps_gro_pantries, cpu);
		len += scnprintf(buffer + len, PAGE_SIZE, "%u", pantry->ignored);
		if (cpu == NR_CPUS - 1)
			break;
		len += scnprintf(buffer + len, PAGE_SIZE, " ");
	}
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	len += scnprintf(buffer + len, PAGE_SIZE, "\n");
#endif
	return len;
}

const struct kernel_param_ops mcps_ignored_ops = {
		.get = &get_mcps_ignored,
};

int dummy_mcps_ignored;
module_param_cb(mcps_stat_ignored,
		&mcps_ignored_ops,
		&dummy_mcps_ignored,
		0640);

int get_mcps_distributed(char *buffer, const struct kernel_param *kp)
{
	int cpu = 0;
	size_t len = 0;

	struct sauron_statistics *stat;

	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		stat = &per_cpu(sauron_stats, cpu);
		len += scnprintf(buffer + len, PAGE_SIZE, "%u", stat->cpu_distribute);
		if (cpu == NR_CPUS - 1)
			break;
		len += scnprintf(buffer + len, PAGE_SIZE, " ");
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	len += scnprintf(buffer + len, PAGE_SIZE, "\n");
#endif
	return len;
}

const struct kernel_param_ops mcps_distributed_ops = {
		.get = &get_mcps_distributed,
};

int dummy_mcps_distributed;
module_param_cb(mcps_stat_distributed,
		&mcps_distributed_ops,
		&dummy_mcps_distributed,
		0640);

int get_mcps_sauron_target_flow(char *buffer, const struct kernel_param *kp)
{
	int cpu = 0;
	size_t len = 0;
	struct sauron *sauron = &mcps->sauron;

	for (cpu = 0 ; cpu < NR_CPUS; cpu++) {
		len += scnprintf(buffer + len, PAGE_SIZE, "%u ", sauron->target_flow_cnt_by_cpus[cpu]);
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	len += scnprintf(buffer + len, PAGE_SIZE, "\n");
#endif
	return len;
}

const struct kernel_param_ops mcps_sauron_target_flow_ops = {
		.get = &get_mcps_sauron_target_flow,
};

int dummy_mcps_sauron_target_flow;
module_param_cb(mcps_stat_sauron_target_flow,
		&mcps_sauron_target_flow_ops,
		&dummy_mcps_sauron_target_flow,
		0640);

int get_mcps_sauron_flow(char *buffer, const struct kernel_param *kp)
{
	int cpu = 0;
	size_t len = 0;
	struct sauron *sauron = &mcps->sauron;

	for (cpu = 0 ; cpu < NR_CPUS; cpu++) {
		len += scnprintf(buffer + len, PAGE_SIZE, "%u ", sauron->flow_cnt_by_cpus[cpu]);
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	len += scnprintf(buffer + len, PAGE_SIZE, "\n");
#endif
	return len;
}

const struct kernel_param_ops mcps_sauron_flow_ops = {
		.get = &get_mcps_sauron_flow,
};

int dummy_mcps_sauron_flow;
module_param_cb(mcps_stat_sauron_flow,
		&mcps_sauron_flow_ops,
		&dummy_mcps_sauron_flow,
		0640);


static DEFINE_SPINLOCK(lock_mcps_mode);
struct mcps_modes *mcps_mode;
EXPORT_SYMBOL(mcps_mode);

static int create_and_init_mcps_mode(int mode)
{
	struct mcps_modes *new, *old;

	new = (struct mcps_modes *)kzalloc(sizeof(struct mcps_modes), GFP_KERNEL);
	if (!new)
		return -ENOMEM;

	new->mode = mode;

	spin_lock(&lock_mcps_mode);
	old = rcu_dereference_protected(mcps_mode,
			lockdep_is_held(&lock_mcps_mode));
	rcu_assign_pointer(mcps_mode, new);
	spin_unlock(&lock_mcps_mode);

	if (old) {
		synchronize_rcu();
		kfree(old);
	}

	return 1;
}

static int release_mcps_mode(void)
{
	struct mcps_modes *old;

	spin_lock(&lock_mcps_mode);
	old = rcu_dereference_protected(mcps_mode,
			lockdep_is_held(&lock_mcps_mode));
	rcu_assign_pointer(mcps_mode, NULL);
	spin_unlock(&lock_mcps_mode);

	if (old) {
		synchronize_rcu();
		kfree(old);
	}

	return 1;
}

int set_mcps_mode(const char *val, const struct kernel_param *kp)
{
	int len = 0;
	unsigned int mode;

	mode = val[len++] - '0';

	if (mode >= 10)
		return len;

	create_and_init_mcps_mode(mode);

	return len;
}

int get_mcps_mode(char *buffer, const struct kernel_param *kp)
{
	struct mcps_modes *mode;
	int len = 0 ;

	rcu_read_lock();
	mode = rcu_dereference(mcps_mode);
	if (mode)
		len += scnprintf(buffer + len, PAGE_SIZE, "%d", mode->mode);
	else
		len += scnprintf(buffer + len, PAGE_SIZE, "0");
	rcu_read_unlock();
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	len += scnprintf(buffer + len, PAGE_SIZE, "\n");
#endif
	return len;
}

const struct kernel_param_ops mcps_mode_ops = {
		.set = &set_mcps_mode,
		.get = &get_mcps_mode,
};

int dummy_mcps_mode;
module_param_cb(mcps_mode,
		&mcps_mode_ops,
		&dummy_mcps_mode,
		0640);

static int create_and_init_arps_config(struct mcps_config *mcps)
{
	struct arps_config *config, *old_config;

	config = (struct arps_config *) kzalloc(sizeof(struct arps_config), GFP_KERNEL);
	if (!config)
		return -ENOMEM;

	init_arps_config(config);

	spin_lock(&lock_mcps_arps_config);
	old_config = rcu_dereference_protected(mcps->arps_config,
			lockdep_is_held(&lock_mcps_arps_config));
	rcu_assign_pointer(mcps->arps_config, config);
	spin_unlock(&lock_mcps_arps_config);

	if (old_config) {
		synchronize_rcu();
		kfree(old_config);
	}

	return 1;
}

static int release_arps_config(struct mcps_config *mcps)
{
	struct arps_config *old_config;

	spin_lock(&lock_mcps_arps_config);
	old_config = rcu_dereference_protected(mcps->arps_config,
			lockdep_is_held(&lock_mcps_arps_config));
	rcu_assign_pointer(mcps->arps_config, NULL);
	spin_unlock(&lock_mcps_arps_config);

	if (old_config) {
		synchronize_rcu();
		kfree(old_config);
	}

	return 1;
}

void init_sauron(struct sauron *sauron)
{
	int i = 0;

	hash_init(sauron->sauron_eyes);

	sauron->sauron_eyes_lock = __SPIN_LOCK_UNLOCKED(sauron_eyes_lock);

	for_each_possible_cpu(i) {
		if (VALID_CPU(i)) {
			sauron->cached_eyes_lock[i] = __SPIN_LOCK_UNLOCKED(cached_eyes_lock[i]);
		}
	}
}

int del_mcps(struct mcps_config *mcps)
{
	int ret;

	ret = release_arps_config(mcps);
	if (ret < 0) {
		MCPS_DEBUG("fail to release arps config\n");
		return -EINVAL;
	}

	ret = release_mcps_mode();
	if (ret < 0) {
		MCPS_DEBUG("fail to release mcps mode\n");
		return -EINVAL;
	}

	release_mcps_arps_meta();
	release_mcps_debug_manager();
	return ret;
}

int init_mcps(struct mcps_config *mcps)
{
	int ret;

	init_mcps_debug_manager();
	ret = create_and_init_arps_config(mcps);
	if (ret < 0) {
		return -EINVAL;
	}

	init_sauron(&mcps->sauron);
	ret = create_and_init_mcps_mode(MCPS_MODE_NONE);
	if (ret < 0) {
		return -EINVAL;
	}

	init_mcps_arps_meta();

	return ret;
}
