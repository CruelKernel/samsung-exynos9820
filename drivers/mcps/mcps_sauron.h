#ifndef MCTCP_SAURON_H
#define MCTCP_SAURON_H

#include <linux/rculist.h>
#if defined(CONFIG_MCPS_GRO_PER_SESSION)
#include <linux/skbuff.h>
#endif // #if defined(CONFIG_MCPS_GRO_PER_SESSION)

#ifdef CONFIG_MCPS_DEBUG
#include <uapi/linux/in6.h>
#include <linux/time.h>
#include <linux/timekeeping.h>
#endif // #ifdef CONFIG_MCPS_DEBUG

#include "mcps_buffer.h"

#define sauron_eyes_bits 9
#define sauron_eyes_num (1<<sauron_eyes_bits)

#if defined(CONFIG_MCPS_ICGB)
enum {
	EYE_POLICY_FAST = 0,
	EYE_POLICY_SLOW,
};
#endif // #if defined(CONFIG_MCPS_ICGB)

/* struct eye - information structure of packet flow (session)
 * Notice : eye must be allocated by zero with kzalloc
 */
struct eye {
	u32 hash;
	u32 value;
	u32 capture;
	u32 pps;

	u32 cpu;
	u32 state;
#if defined(CONFIG_MCPS_GRO_PER_SESSION)
	u32 gro_nskb;
	u32 gro_tog;
#endif // #if defined(CONFIG_MCPS_GRO_PER_SESSION)

#if defined(CONFIG_MCPS_ICGB)
	u32 policy;
#endif // #if defined(CONFIG_MCPS_ICGB)
	unsigned int	dst_ipv4;
	struct in6_addr dst_ipv6;
	unsigned int	dst_port;

	unsigned long	t_stamp;
	unsigned long	t_capture;

	int			  monitored;

	struct rcu_head rcu;
	struct hlist_node		eye_hash_node;

#ifdef CONFIG_MCPS_DEBUG
	struct timespec timestamp;
	atomic_t		input_num;
	atomic_t		output_num;
	atomic_t		drop_num;
	atomic_t		ofo_num;
	atomic_t		mig_count;
	atomic_t		l2l_count;
	atomic_t		l2b_count;
	atomic_t		b2l_count;
	atomic_t		b2b_count;
	struct timespec mig_1st_time;
	struct timespec mig_last_time;
#endif
	struct pending_queue pendings;
};

struct sauron {
	spinlock_t sauron_eyes_lock;

	struct hlist_head sauron_eyes[sauron_eyes_num];

	spinlock_t cached_eyes_lock[NR_CPUS];

	struct eye *heavy_eyes[NR_CPUS];				// cached_eyes_lock, sauron_eyes_lock(?)
	struct eye *light_eyes[NR_CPUS];				// cached_eyes_lock, sauron_eyes_lock(?)
	unsigned int flow_cnt_by_cpus[NR_CPUS];			  // sauron_eyes_lock
	unsigned int target_flow_cnt_by_cpus[NR_CPUS];	   // sauron_eyes_lock
};

#define DEFAULT_MONITOR_PPS_THRESHOLD 100 // 1mbps = about 100 pps (mtu 1500)
#define MONITOR_CONDITION(pps) (pps >= DEFAULT_MONITOR_PPS_THRESHOLD)

static inline void sauron_lock(struct sauron *s)
{
	spin_lock(&s->sauron_eyes_lock);
}

static inline void sauron_unlock(struct sauron *s)
{
	spin_unlock(&s->sauron_eyes_lock);
}

static inline void update_heavy_and_light(struct sauron  *sauron, struct eye *eye)
{
	int cpu = eye->cpu;
	if (!MONITOR_CONDITION(eye->pps))
		return;

	spin_lock(&sauron->cached_eyes_lock[cpu]);
	if (!sauron->heavy_eyes[cpu] || sauron->heavy_eyes[cpu]->pps < eye->pps) {
		sauron->heavy_eyes[cpu] = eye;
	}

	if (!sauron->light_eyes[cpu] || sauron->light_eyes[cpu]->pps > eye->pps) {
		sauron->light_eyes[cpu] = eye;
	}
	spin_unlock(&sauron->cached_eyes_lock[cpu]);
}

static inline void remove_heavy_and_light(struct sauron  *sauron, struct eye *eye)
{
	int cpu = eye->cpu;

	spin_lock(&sauron->cached_eyes_lock[cpu]);
	if (sauron->heavy_eyes[cpu] == eye)
		sauron->heavy_eyes[cpu] = NULL;

	if (sauron->light_eyes[cpu] == eye)
		sauron->light_eyes[cpu] = NULL;
	spin_unlock(&sauron->cached_eyes_lock[cpu]);
}

struct eye *pick_heavy(struct sauron  *sauron, int cpu);
struct eye *pick_light(struct sauron  *sauron, int cpu);

struct sauron_statistics {
	unsigned int cpu_distribute;
};

DECLARE_PER_CPU(struct sauron_statistics, sauron_stats);

#if defined(CONFIG_MCPS_GRO_PER_SESSION)
struct eye_skb {
	unsigned int count;
	unsigned int limit;
};

#define EYESKB(skb) ((struct eye_skb *)((skb)->cb))
static inline void mcps_eye_gro_stamp(struct eye *e, struct sk_buff *skb)
{
	e->gro_tog = (e->gro_nskb != 0) ? (e->gro_tog+1)%e->gro_nskb : 0;
	EYESKB(skb)->limit = e->gro_nskb;
	EYESKB(skb)->count = e->gro_tog;
}

static inline int mcps_eye_gro_skip(struct sk_buff *skb)
{
	return (EYESKB(skb)->limit == 0);
}

static inline int mcps_eye_gro_flush(struct sk_buff *skb)
{
	return (EYESKB(skb)->count == 0);
}
#else // #if defined(CONFIG_MCPS_GRO_PER_SESSION)
#define mcps_eye_gro_stamp(e, skb) do { } while (0)
#define mcps_eye_gro_skip(skb)	 do { } while (0)
#define mcps_eye_gro_flush(skb)	do { } while (0)
#endif // #if defined(CONFIG_MCPS_GRO_PER_SESSION)

/* Declare functions */
void init_mcps_core(void);
void release_mcps_core(void);

int get_arps_cpu(struct sauron *sauron, struct sk_buff *skb);
int get_agro_cpu(struct sauron *sauron, struct sk_buff *skb);
int flush_flows(int force);

struct eye *search_flow(struct sauron *sauron, u32 hash);
int _move_flow(unsigned int hash, unsigned int to);

int migrate_flow(unsigned int from, unsigned int to, unsigned int option);

unsigned int light_cpu(void);
#endif
