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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hashtable.h>
#include <linux/mutex.h>
#include <linux/netdevice.h>
#include <linux/version.h>
#include <linux/rculist.h>

#include <linux/ip.h>
#include <linux/tcp.h>
#include <net/ip.h>

#include "mcps_sauron.h"
#include "mcps_device.h"
#include "mcps_buffer.h"
#include "mcps_debug.h"

#define HEAVY_FLOW 0
#define LIGHT_FLOW 1

#define FNULL 11
#define FWRONGCPU 12
#define FNOCACHE 17

DEFINE_PER_CPU(struct sauron_statistics, sauron_stats);

unsigned int mcps_set_cluster_for_newflow __read_mostly = NR_CLUSTER;
module_param(mcps_set_cluster_for_newflow, uint, 0640);
unsigned int mcps_set_cluster_for_hotplug __read_mostly = BIG_CLUSTER;
module_param(mcps_set_cluster_for_hotplug, uint, 0640);

#if defined(CONFIG_MCPS_ICB)
unsigned int mcps_boost_pkt_num __read_mostly;
module_param(mcps_boost_pkt_num, uint, 0640);

static inline int check_boost_condition(int rxcnt)
{
	return rxcnt == mcps_boost_pkt_num;
}
#endif // #if defined(CONFIG_MCPS_ICB)

#if defined(CONFIG_MCPS_GRO_PER_SESSION)
/*
 *                     *-cnt0  *-cnt1  *-pps0  *-pps1  *-pps2
 * GRO level : non-gro | fixed | step0 | step1 | step2 | step3
 *
 * list template : {base, ppsx, cnt0, fixed, cnt1, step0, pps0, step1, pps2, step2, pps2, step3}
 * size : 12
 */
#define MCPS_GRO_POLICY_SIZE 12
unsigned int mcps_gro_policy0[MCPS_GRO_POLICY_SIZE] __read_mostly = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
module_param_array(mcps_gro_policy0, uint, NULL, 0640);
unsigned int mcps_gro_policy1[MCPS_GRO_POLICY_SIZE] __read_mostly = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
module_param_array(mcps_gro_policy1, uint, NULL, 0640);
unsigned int mcps_l2b_pps __read_mostly = UINT_MAX;
module_param(mcps_l2b_pps, uint, 0640);
#endif // #if defined(CONFIG_MCPS_GRO_PER_SESSION)

#if defined(CONFIG_MCPS_V2)
int mcps_agro_enable = MCPS_CPU_DIRECT_GRO;
module_param(mcps_agro_enable, int, 0640);
#endif

#define PRINT_MOVE(HASH, TO, RESULT) MCPS_DEBUG("FLOW[%10u] -> %2u = %d \n", HASH, TO, RESULT)
#define PRINT_MIGRATE(FROM, TO, RESULT) MCPS_DEBUG("%2u -> %2u = %d \n", FROM, TO, RESULT)

#ifdef CONFIG_MCPS_DEBUG_PRINTK
#define PRINT_FLOW_STAT(S) do {   \
	int i = 0;  \
	size_t len = 0; \
	char buf[100];  \
	for (i = 0; i < NR_CPUS; i++) {   \
		len += scnprintf(buf + len, 100, "%2u|", (S)->flow_cnt_by_cpus[i]);   \
	}   \
	MCPS_DEBUG("%s\n", buf); \
} while (0)

#define PRINT_TARGET_STAT(S) do {   \
	int i = 0;  \
	size_t len = 0; \
	char buf[100];  \
	for (i = 0; i < NR_CPUS; i++) {   \
		len += scnprintf(buf + len, 100, "%2u|", (S)->target_flow_cnt_by_cpus[i]);   \
	}   \
	MCPS_DEBUG("%s\n", buf); \
} while (0)
#else
#define PRINT_FLOW_STAT(S)   do { } while (0)
#define PRINT_TARGET_STAT(S) do { } while (0)
#endif

struct eye *pick_heavy(struct sauron *sauron, int cpu)
{
	struct eye *eye = NULL;
	spin_lock(&sauron->cached_eyes_lock[cpu]);
	eye = sauron->heavy_eyes[cpu];
	spin_unlock(&sauron->cached_eyes_lock[cpu]);
	return eye;
}

struct eye *pick_light(struct sauron *sauron, int cpu)
{
	struct eye *eye = NULL;
	spin_lock(&sauron->cached_eyes_lock[cpu]);
	eye = sauron->light_eyes[cpu];
	spin_unlock(&sauron->cached_eyes_lock[cpu]);
	return eye;
}

void del_monitor_flow(struct sauron *sauron, struct eye *eye)
{
	if (!eye->monitored)
		return;

	sauron->target_flow_cnt_by_cpus[eye->cpu]--;
	eye->monitored = 0;
}

void add_monitor_flow(struct sauron *sauron, struct eye *eye)
{
	if (eye->monitored)
		return;

	sauron->target_flow_cnt_by_cpus[eye->cpu]++;
	eye->monitored = 1;
}

void __delete_flow(struct sauron *sauron, struct eye *eye)
{
	int cpu = eye->cpu;

	sauron->flow_cnt_by_cpus[cpu]--;
}

void __add_flow(struct sauron *sauron, struct eye *eye)
{
	int cpu = eye->cpu;

	sauron->flow_cnt_by_cpus[cpu]++;
}

/* search_flow - search flow node from hash table with hash value.
 * @sauron: object in which flow nodes are managed.
 * @hash: hash value from skb
 *
 * return values:
 * NULL (no exist)
 * struct eye *(exist node)
 */
struct eye *search_flow(struct sauron *sauron, u32 hash)
{
	unsigned int idx = (unsigned int)(hash % HASH_SIZE(sauron->sauron_eyes));
	struct eye *eye = NULL;

	hlist_for_each_entry_rcu(eye, &sauron->sauron_eyes[idx], eye_hash_node)	{
		if (eye->hash == hash)
			return eye;
	}
	return NULL;
}

static void eye_rcu_call(struct rcu_head *p)
{
	struct eye *e = container_of(p, struct eye, rcu);
	discard_buffer(&e->pendings);
	kfree(e);
}

void delete_flow(struct sauron *sauron, struct eye *flow)
{
	hash_del_rcu(&flow->eye_hash_node);
	remove_heavy_and_light(sauron, flow);
	del_monitor_flow(sauron, flow);
	__delete_flow(sauron, flow);
	call_rcu(&flow->rcu, eye_rcu_call);
}

struct pending_queue *find_pendings(unsigned long data)
{
	unsigned int hash = (unsigned int) data;
	struct eye *e = NULL;
	e = search_flow(&mcps->sauron, hash);
	if (!e) {
		return NULL;
	}
	return &e->pendings;
}

/* __move_flow - move selected flow to selected core(to)
 */
static int __move_flow(struct sauron *sauron, unsigned int to, struct eye *flow)
{
	sauron_lock(sauron);

	remove_heavy_and_light(sauron, flow);
	del_monitor_flow(sauron, flow);
	__delete_flow(sauron, flow);

	flow->cpu = to;

	__add_flow(sauron, flow);
	add_monitor_flow(sauron, flow);
	update_heavy_and_light(sauron, flow);

	PRINT_MOVE(flow->hash, to, 0);
	PRINT_FLOW_STAT(sauron);
	PRINT_TARGET_STAT(sauron);

	sauron_unlock(sauron);

	return 0;
}

// Must be called inside of rcu read section
int _move_flow(unsigned int hash, unsigned int to)
{
	int ret = 0;
	struct sauron *sauron = &mcps->sauron;
	struct eye *flow = NULL;

	rcu_read_lock();
	flow = search_flow(sauron, hash);

	if (!flow) {
		ret = -1;
		goto error;
	}

	if (flow->cpu == to) {
		ret = -FWRONGCPU;
		goto error;
	}
	ret = __move_flow(sauron, to, flow);
error:
	rcu_read_unlock();
	return ret;
}

int move_flow(unsigned int to, struct eye *flow)
{
	int ret;
	unsigned int from;

	if (!flow) {
		PRINT_MOVE(0, to, -FNULL);
		return -FNULL;
	}

	from = flow->cpu;

	if (to >= NR_CPUS || from == to) {
		PRINT_MOVE(flow->hash, to, -FWRONGCPU);
		return -FWRONGCPU;
	}

	// move agro
	ret = pending_migration(&flow->pendings, flow->hash, from, to);

#ifdef CONFIG_MCPS_DEBUG
	if (!ret) {
		int vec = (CLUSTER(from) | (CLUSTER(to) << 2));
		mcps_migrate_flow_history(flow, vec);
	}
#endif

	return ret;
}

int migrate_flow(unsigned int from, unsigned int to, unsigned int option)
{
	int ret = 0;
	struct eye *flow = NULL;
	struct sauron *sauron = &mcps->sauron;

	if (!mcps_cpu_online(from) || !mcps_cpu_online(to))
		return -EINVAL;

	rcu_read_lock();
	if (option & LIGHT_FLOW) {
		flow = pick_light(sauron, from);
	} else {
		flow = pick_heavy(sauron, from);
	}

	if (!flow) {
		ret = -FNOCACHE;
		goto pass;
	}

	ret = move_flow(to, flow);
pass:
	rcu_read_unlock();
	PRINT_MIGRATE(from, to, ret);
	return ret;
}

static inline int mcps_check_skb_can_gro(struct sk_buff *skb)
{
	switch (skb->data[0] & 0xF0) {
	case 0x40:
		return (ip_hdr(skb)->protocol == IPPROTO_TCP);
	case 0x60:
		return (ipv6_hdr(skb)->nexthdr == IPPROTO_TCP);
	}
	return 0;
}

void __update_protocol_ipv4(struct sk_buff *skb, struct eye *flow)
{
	struct iphdr *hdr = ip_hdr(skb);
	unsigned int dst_ip = (unsigned int)hdr->daddr;
	unsigned int dst_port = 0;

#if defined(CONFIG_MCPS_ICGB)
	if (check_mcps_in_addr(hdr->daddr)) {
		flow->policy = EYE_POLICY_FAST;
	} else {
		flow->policy = EYE_POLICY_SLOW;
	}
#endif // #if defined(CONFIG_MCPS_ICGB)

	if (ip_is_fragment(hdr)) {
		goto fragmented;
	}
	if (hdr->protocol == IPPROTO_TCP) {
		struct tcphdr *th = NULL;
		th = (struct tcphdr *)(skb_header_pointer(skb, sizeof(struct iphdr), sizeof(struct tcphdr), th));
		if (th) {
			dst_port = (unsigned int)ntohs(th->dest);
		}
	} else if (hdr->protocol == IPPROTO_UDP) {
		struct udphdr *uh = NULL;
		uh = (struct udphdr *)(skb_header_pointer(skb, sizeof(struct iphdr), sizeof(struct udphdr), uh));
		if (uh) {
			dst_port = (unsigned int)ntohs(uh->dest);
		}
	}

fragmented:
	flow->dst_ipv4 = dst_ip;
	flow->dst_port = dst_port;

}

void __update_protocol_ipv6(struct sk_buff *skb, struct eye *flow)
{
	struct ipv6hdr *hdr = ipv6_hdr(skb);
	unsigned int dst_port = 0;

#if defined(CONFIG_MCPS_ICGB)
	if (check_mcps_in6_addr(&hdr->daddr)) {
		flow->policy = EYE_POLICY_FAST;
	} else {
		flow->policy = EYE_POLICY_SLOW;
	}
#endif // #if defined(CONFIG_MCPS_ICGB)

	if (hdr->nexthdr == IPPROTO_TCP) {
		struct tcphdr *th = NULL;
		th = (struct tcphdr *)(skb_header_pointer(skb, sizeof(struct ipv6hdr), sizeof(struct tcphdr), th));
		if (th) {
			dst_port = (unsigned int)ntohs(th->dest);
		}
	} else if (hdr->nexthdr == IPPROTO_UDP) {
		struct udphdr *uh = NULL;
		uh = (struct udphdr *)(skb_header_pointer(skb, sizeof(struct ipv6hdr), sizeof(struct udphdr), uh));
		if (uh) {
			dst_port = (unsigned int)ntohs(uh->dest);
		}
	} /*else if (hdr->nexthdr == IPPROTO_FRAGMENT) {}*/

	memcpy(&flow->dst_ipv6, &hdr->daddr, sizeof(struct in6_addr));
	flow->dst_port = dst_port;

}

void update_protocol_info(struct sk_buff *skb, struct eye *flow)
{
	switch (skb->protocol) {
	case htons(ETH_P_IP):
			__update_protocol_ipv4(skb, flow);
			break;
	case htons(ETH_P_IPV6):
			__update_protocol_ipv6(skb, flow);
			break;
	default:
		return;
	}
}

struct eye *create_and_init_eye(struct sk_buff *skb)
{
	unsigned long now = jiffies;
	struct eye *eye = NULL;

	eye = (struct eye *) kzalloc(sizeof(struct eye), GFP_ATOMIC);
	if (!eye) {
		return NULL;
	}

	eye->hash = skb->hash;
	eye->t_stamp = eye->t_capture = now;

	init_rcu_head(&eye->rcu);
	INIT_HLIST_NODE(&eye->eye_hash_node);

	update_protocol_info(skb, eye);

	init_pendings(&eye->pendings);
#ifdef CONFIG_MCPS_DEBUG
	get_monotonic_boottime(&eye->timestamp);
	get_monotonic_boottime(&eye->mig_1st_time);
	get_monotonic_boottime(&eye->mig_last_time);
	atomic_set(&eye->input_num, 0);
	atomic_set(&eye->output_num, 0);
	atomic_set(&eye->drop_num, 0);
	atomic_set(&eye->ofo_num, 0);
	atomic_set(&eye->mig_count, 0);
	atomic_set(&eye->l2l_count, 0);
	atomic_set(&eye->l2b_count, 0);
	atomic_set(&eye->b2l_count, 0);
	atomic_set(&eye->b2b_count, 0);
#endif
	return eye;
}

struct eye *add_flow(struct sauron *sauron, struct sk_buff *skb, int cpu)
{
	unsigned int	idx;
	unsigned int hash = skb->hash;
	struct eye	*eye = create_and_init_eye(skb);

	if (eye == NULL) {
		return NULL;
	}

	eye->cpu = cpu;
	eye->state = 0;

	idx = (unsigned int)(hash % HASH_SIZE(sauron->sauron_eyes));
	sauron_lock(sauron);
	hlist_add_head_rcu(&eye->eye_hash_node, &sauron->sauron_eyes[idx]);
	__add_flow(sauron, eye);
	sauron_unlock(sauron);
	return eye;
}

static unsigned long last_flush;
int flush_flows(int force)
{
	unsigned long now = jiffies;
	struct sauron *sauron;
	int idx = 0;
	int res = 0;

	if (force)
		goto skip;

	if (!time_after(now, last_flush + 10 * HZ))
		return 0;

skip:
	sauron = &mcps->sauron;

	last_flush = jiffies;
	sauron_lock(sauron);
	for (idx = 0; idx < HASH_SIZE(sauron->sauron_eyes); idx++) {
		struct eye *e = NULL;
		hlist_for_each_entry_rcu(e, &sauron->sauron_eyes[idx], eye_hash_node) {
			if (e && time_after(last_flush, e->t_stamp + 10*HZ)) {
				dump(e);
				delete_flow(sauron, e);
				res++;
			}
		}
	}
	sauron_unlock(sauron);
	MCPS_VERBOSE("flush! %d flow\n", res);
	return res;
}
EXPORT_SYMBOL(flush_flows);

static inline unsigned int app_core(unsigned int hash)
{
	unsigned int cpu_on_app = NR_CPUS;
	struct rps_sock_flow_table *sock_flow_table;
	sock_flow_table = rcu_dereference(rps_sock_flow_table);

	if (sock_flow_table) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
		u32 ident;
		/* First check into global flow table if there is a match */
		ident = sock_flow_table->ents[hash & sock_flow_table->mask];
		if ((ident ^ hash) & ~rps_cpu_mask)
			cpu_on_app = NR_CPUS;
		else
			cpu_on_app = ident & rps_cpu_mask;
#else
		cpu_on_app = sock_flow_table->ents[hash & sock_flow_table->mask];
#endif
	}

	if (cpu_on_app >= NR_CPUS)
		cpu_on_app = smp_processor_id_safe();

	return cpu_on_app;
}

static inline int
__get_rand_cpu(struct rps_map *map, u32 hash) {
	int cpu = -1;

	cpu = map->cpus[reciprocal_scale(hash, map->len)];
	if (mcps_cpu_online(cpu)) {
		return cpu;
	}

	return -1;
}

static int
get_rand_cpu(struct arps_meta *arps, u32 hash, unsigned int cluster)
{
	int cpu = 0;
	struct rps_map *map = NULL;

	if (cluster >= NR_CLUSTER)
		cluster = ALL_CLUSTER;

	map = ARPS_MAP(arps, cluster);
	if (!map->len) {
		map = ARPS_MAP(arps, ALL_CLUSTER);
	}

	cpu = __get_rand_cpu(map, hash);
	if (VALID_CPU(cpu))
		return cpu;

	for_each_cpu_and(cpu, arps->mask, mcps_cpu_online_mask) {
		if (cpu < 0)
			continue;
		return cpu;
	}

	//current
	return smp_processor_id_safe();
}

unsigned int light_cpu(void)
{
	unsigned int cpu = 0;
	struct arps_meta *arps;
	rcu_read_lock();
	arps = get_arps_rcu();
	if (!arps) {
		rcu_read_unlock();
		return 0;
	}

	cpu = get_rand_cpu(arps, (u32)jiffies, mcps_set_cluster_for_hotplug);
	rcu_read_unlock();

	return cpu;
}

void update_mask
(struct sauron *sauron, struct arps_meta *arps, struct eye *eye, unsigned int cluster)
{
	int cpu = get_rand_cpu(arps, eye->hash, cluster);
	__move_flow(sauron, cpu, eye);
}

static inline int mcps_check_overhead_avoidance(struct eye *eye)
{
	struct arps_meta *newflow_arps = get_newflow_rcu();
	if (!newflow_arps) {
		return 0;
	}
	return cpumask_test_cpu(eye->cpu, newflow_arps->mask);
}

#define HALFSEC (HZ/2)

static inline unsigned int __update_eye_pps(struct eye *eye)
{
	if (eye->t_stamp - eye->t_capture == 0) {
		return eye->pps;
	}

	return (unsigned int)((eye->value - eye->capture) * HZ / (eye->t_stamp - eye->t_capture));
}

static int update_eye_pps(struct eye *eye)
{
	struct arps_meta *arps = NULL;
	if (!time_after(eye->t_stamp, eye->t_capture + HALFSEC)) {
		return 0;
	}

	eye->pps = __update_eye_pps(eye);

	eye->t_capture  = eye->t_stamp;
	eye->capture	= eye->value;

	arps = get_arps_rcu();
	if (!arps) {
		return 1;
	}

	// inter migration
	if (!mcps_check_overhead_avoidance(eye)) {
		if (CLUSTER(eye->cpu) == LIT_CLUSTER) {
			move_flow(get_rand_cpu(arps, eye->hash, MID_CLUSTER), eye);
		} else {
			move_flow(get_rand_cpu(arps, eye->hash, LIT_CLUSTER), eye);
		}
	} else if (eye->pps > mcps_l2b_pps) {
		if (CLUSTER(eye->cpu) == LIT_CLUSTER) {
			move_flow(get_rand_cpu(arps, eye->hash, MID_CLUSTER), eye);
		}
	}
	return 1;
}

#if defined(CONFIG_MCPS_GRO_PER_SESSION)
static void __update_gro_policy(struct eye *eye, int updated, unsigned int policy[MCPS_GRO_POLICY_SIZE])
{
	unsigned int pps = eye->pps;
	if (eye->value == policy[2]) {
		eye->gro_nskb = policy[3];
		return;
	}

	if (eye->value == policy[4]) {
		pps = __update_eye_pps(eye);
		goto update;
	}

	if (!updated) {
		return;
	}
update:
	if (pps > policy[10]) {
		eye->gro_nskb = policy[11];
	} else if (pps > policy[8]) {
		eye->gro_nskb = policy[9];
	} else if (pps > policy[6]) {
		eye->gro_nskb = policy[7];
	} else {
		eye->gro_nskb = policy[5];
	}

	if (pps > policy[1] && eye->gro_nskb < policy[0]) {
		eye->gro_nskb = policy[0];
	}

	if (eye->value < policy[4]) {
		eye->gro_nskb = policy[3];
	}
}
#else //#if defined(CONFIG_MCPS_GRO_PER_SESSION)
#define __update_gro_policy(eye, updated, policy) do { } while (0)
#endif // #if defined(CONFIG_MCPS_GRO_PER_SESSION)

#if defined(CONFIG_MCPS_ICGB)
static void update_gro_policy_slow(struct eye *eye, int updated)
{
#if defined(CONFIG_MCPS_ICB)
	if (check_boost_condition(eye->value)) {
		mcps_boost_clock(0x1 | 0x2);
	}
#endif // #if defined(CONFIG_MCPS_ICB)
	__update_gro_policy(eye, updated, mcps_gro_policy1);
}

static void update_gro_policy_fast(struct eye *eye, int updated)
{
#if defined(CONFIG_MCPS_ICB)
	if (check_boost_condition(eye->value)) {
		mcps_boost_clock(0x1);
	}
#endif // #if defined(CONFIG_MCPS_ICB)
	__update_gro_policy(eye, updated, mcps_gro_policy0);
}
#else // #if defined(CONFIG_MCPS_ICGB)
static void update_gro_policy(struct eye *eye, int updated)
{
#if defined(CONFIG_MCPS_ICB)
	if (check_boost_condition(eye->value)) {
		mcps_boost_clock(0x1);
	}
#endif // #if defined(CONFIG_MCPS_ICB)
	__update_gro_policy(eye, updated, mcps_gro_policy0);
}
#endif // #if defined(CONFIG_MCPS_ICGB)

static int update_policy(struct eye *eye)
{
	int updated = update_eye_pps(eye);
#if defined(CONFIG_MCPS_ICGB)
	if (eye->policy == EYE_POLICY_SLOW) {
		update_gro_policy_slow(eye, updated);
	} else {
		update_gro_policy_fast(eye, updated);
	}
#else // #if defined(CONFIG_MCPS_ICGB)
	update_gro_policy(eye, updated);
#endif // #if defined(CONFIG_MCPS_ICGB)

	return updated;
}

static void update_eye(struct sauron *sauron, struct eye *eye)
{
	eye->value++;
	eye->t_stamp = jiffies;

	if (!update_policy(eye)) {
		return;
	}

	sauron_lock(sauron);
	if (MONITOR_CONDITION(eye->pps)) {
		add_monitor_flow(sauron, eye);
		update_heavy_and_light(sauron, eye);
	} else {
		del_monitor_flow(sauron, eye);
	}
	sauron_unlock(sauron);
	return;
}

int get_arps_cpu(struct sauron *sauron, struct sk_buff *skb)
{
	int cpu = 0;
	struct eye	   *eye	   = NULL;
	struct arps_meta *arps	  = NULL;

	u32 hash = skb->hash;

	if (!mcps)
		goto error;

	//step.2 SEARCH
	eye = search_flow(sauron, hash);
	if (!eye) {
		unsigned int cluster = (mcps_set_cluster_for_newflow >= NR_CLUSTER) ? CLUSTER(smp_processor_id_safe()) : mcps_set_cluster_for_newflow;
		struct arps_meta *newflow_arps = get_newflow_rcu();
		int rcpu = 0;
		if (!newflow_arps) {
			goto error;
		}

		rcpu = get_rand_cpu(newflow_arps, hash, cluster);

		rcu_read_unlock();
		eye = add_flow(sauron, skb, rcpu);
		rcu_read_lock();
		if (!eye) {
			goto error;
		}
	}
	cpu = eye->cpu;

	arps	= get_arps_rcu();
	if (!arps) {
		goto error;
	}

	if (!cpumask_test_cpu(cpu, arps->mask)) {
		update_mask(sauron, arps, eye, CLUSTER(cpu));
		goto changed;
	}

	if (!mcps_cpu_online(cpu)) {
		int ret = try_to_hqueue(eye->hash, cpu, skb, MCPS_ARPS_LAYER);
		if (ret < 0)
			return MCPS_CPU_ON_PENDING;
	}

changed:
	return eye->cpu;
error:
	return -EINVAL;
}
EXPORT_SYMBOL(get_arps_cpu);

int get_agro_cpu(struct sauron *sauron, struct sk_buff *skb)
{
	struct sauron_statistics *stat = this_cpu_ptr(&sauron_stats);
	struct eye	   *eye	   = NULL;

	u32 hash = skb->hash;

	stat->cpu_distribute++;

	if (!mcps)
		goto error;

	eye = search_flow(sauron, hash);
	if (!eye) {
		unsigned int cluster = (mcps_set_cluster_for_newflow >= NR_CLUSTER) ? CLUSTER(smp_processor_id_safe()) : mcps_set_cluster_for_newflow;
		struct arps_meta *newflow_arps = get_newflow_rcu();
		int rcpu = 0;
		if (!newflow_arps) {
			goto error;
		}
		rcpu = get_rand_cpu(newflow_arps, hash, cluster);

		rcu_read_unlock();
		eye = add_flow(sauron, skb, rcpu);
		rcu_read_lock();
		if (!eye) {
			goto error;
		}
	}
	update_eye(sauron, eye);
	mcps_eye_gro_stamp(eye, skb);

	if (!mcps_check_skb_can_gro(skb) || eye->state == MCPS_CPU_GRO_BYPASS) {
		return MCPS_CPU_GRO_BYPASS;
	}

#if defined(CONFIG_MCPS_V2)
	if (mcps_agro_enable == MCPS_CPU_DIRECT_GRO) {
		return MCPS_CPU_DIRECT_GRO;
	}
#endif // #if defined(CONFIG_MCPS_V2)

	if (check_pending(&eye->pendings, skb)) {
		return MCPS_CPU_ON_PENDING;
	}

	if (!mcps_cpu_online(eye->cpu)) {
		int ret = try_to_hqueue(eye->hash, eye->cpu, skb, MCPS_AGRO_LAYER);

		if (ret < 0)
			return MCPS_CPU_ON_PENDING;
	}

	return eye->cpu;
error:
	stat->cpu_distribute--;
	return -EINVAL;
}
EXPORT_SYMBOL(get_agro_cpu);

void init_mcps_core(void)
{
}

void release_mcps_core(void)
{
}
