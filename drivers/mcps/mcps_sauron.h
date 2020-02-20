#ifndef MCTCP_SAURON_H
#define MCTCP_SAURON_H

#include <linux/rculist.h>

#ifdef CONFIG_MCPS_DEBUG
#include <uapi/linux/in6.h>
#include <linux/time.h>
#include <linux/timekeeping.h>
#endif // #ifdef CONFIG_MCPS_DEBUG

#include "mcps_device.h"

#define sauron_eyes_bits 9
#define sauron_eyes_num 1<<sauron_eyes_bits;
#define sauron_type 4
#define sauron_weight_types 2

enum {
	SAURON_ERR_UNKNOWN 	= 9,
	SAURON_ERR_DROP		= 8,
	SAURON_ERR_NOEXIST	= 7
};

enum {
	SAURON_NONE 		= 0,
	SAURON_HASH			= 0,
	SAURON_FLAG 		= 1,
	SAURON_BAND 		= 2,
	SAURON_TIME			= 3,
};

#ifdef CONFIG_MCTCP_DEBUG_PRINTK
#define PRINT_MOVE(HASH , TO , RESULT) { \
    printk(KERN_DEBUG "MCPSD %s : TO[%u] FLOW[%u] = %d \n", __FUNCTION__, TO, HASH, RESULT);  \
};
#define PRINT_MIGRATE(FROM , TO , RESULT) { \
    printk(KERN_DEBUG "MCPSD %s : TO[%u] FROM[%u] = %d \n", __FUNCTION__, TO, FROM, RESULT);  \
};
#else
#define PRINT_MOVE(HASH , TO , RESULT) {};
#define PRINT_MIGRATE(FROM , TO , RESULT)  {};
#endif
/* struct eye - information structure of packet flow (session)
 * Notice : eye must be allocated by zero with kzalloc
  * */
struct eye {
	u32 hash;
	u32 value;
	u32 capture;
	u32 pps;

	u32 cpu;

	unsigned int flag;
	unsigned int tog;

	u32 gro;

	unsigned long    t_stamp;
	unsigned long    t_capture;

	int              monitored;

	struct rcu_head rcu;
	struct hlist_node		eye_hash_node;

#ifdef CONFIG_MCPS_DEBUG
	unsigned int    dst_ipv4;
	struct in6_addr dst_ipv6;
	unsigned int    dst_port;
    struct timespec timestamp;
    atomic_t        input_num;
    atomic_t        output_num;
    atomic_t        drop_num;
    atomic_t        ofo_num;
    atomic_t        mig_count;
    atomic_t        l2l_count;
    atomic_t        l2b_count;
    atomic_t        b2l_count;
    atomic_t        b2b_count;
    struct timespec mig_1st_time;
    struct timespec mig_last_time;
#endif
};

static inline void init_eye(struct eye* eye , u32 hash , u32 gro) {
    unsigned long now = jiffies;
    eye->hash       = hash;
    eye->value      = eye->capture = eye->pps = 0;
    eye->t_stamp    = eye->t_capture = now;
    eye->gro        = gro;

    eye->monitored  = 0;

    init_rcu_head(&eye->rcu);
    INIT_HLIST_NODE(&eye->eye_hash_node);

#ifdef CONFIG_MCPS_DEBUG
    get_monotonic_boottime(&eye->timestamp);
    get_monotonic_boottime(&eye->mig_1st_time);
    get_monotonic_boottime(&eye->mig_last_time);
    atomic_set(&eye->input_num,0);
    atomic_set(&eye->output_num,0);
    atomic_set(&eye->drop_num,0);
    atomic_set(&eye->ofo_num,0);
    atomic_set(&eye->mig_count,0);
    atomic_set(&eye->l2l_count,0);
    atomic_set(&eye->l2b_count,0);
    atomic_set(&eye->b2l_count,0);
    atomic_set(&eye->b2b_count,0);
#endif
}

struct sauron {
	struct rcu_head rcu;
	spinlock_t sauron_eyes_lock;
	spinlock_t eye_pending_lock;

	struct hlist_head sauron_eyes[1 << (sauron_eyes_bits)];

	spinlock_t heavy_and_low_eyes_lock[NR_CPUS];
	struct eye *heavy_eyes[NR_CPUS];
	struct eye *low_eyes[NR_CPUS];

	unsigned int flow_cnt_by_cpus[NR_CPUS];
	unsigned int gro_flow_cnt_by_cpus[NR_CPUS];

	int sauron_len;

	unsigned int target_flow_cnt_by_cpus[NR_CPUS];
	unsigned int target_gro_flow_cnt_by_cpus[NR_CPUS];
	unsigned int target_cnt;

	unsigned int sauron_mode;
};

#define DEFAULT_MONITOR_PPS_TRESHOLD 100 // 1mbps = about 100 pps (mtu 1500)
#define MONITOR_CONDITION(pps) (pps >= DEFAULT_MONITOR_PPS_TRESHOLD)

static void del_monitor_flow(struct sauron* sauron, struct eye * eye , unsigned int from , u32 gro);
static void add_monitor_flow(struct sauron* sauron, struct eye * eye , unsigned int to, u32 gro);

/*It may need to be locked.*/
static inline int update_pps(struct sauron* sauron, struct eye* eye, u32 gro)
{
    if(!time_after(eye->t_stamp, eye->t_capture + 1 * HZ))
        return -EINVAL;

    eye->pps = (unsigned int)((eye->value - eye->capture) * HZ / (eye->t_stamp - eye->t_capture));

    eye->t_capture  = eye->t_stamp;
    eye->capture    = eye->value;

    if(MONITOR_CONDITION(eye->pps)) {
        spin_lock(&sauron->sauron_eyes_lock);
        add_monitor_flow(sauron, eye , eye->cpu, gro);
        spin_unlock(&sauron->sauron_eyes_lock);
    } else {
        spin_lock(&sauron->sauron_eyes_lock);
        del_monitor_flow(sauron, eye , eye->cpu, gro);
        spin_unlock(&sauron->sauron_eyes_lock);
    }

    return 0;
}

static inline void update_heavy_and_low(struct sauron * sauron, struct eye *eye)
{
    int cpu = eye->cpu;
    if (!MONITOR_CONDITION(eye->pps))
        return;

    spin_lock(&sauron->heavy_and_low_eyes_lock[cpu]);
    if (!sauron->heavy_eyes[cpu] || sauron->heavy_eyes[cpu]->pps < eye->pps) {
        sauron->heavy_eyes[cpu] = eye;
    }

    if (!sauron->low_eyes[cpu] || sauron->low_eyes[cpu]->pps > eye->pps) {
        sauron->low_eyes[cpu] = eye;
    }
    spin_unlock(&sauron->heavy_and_low_eyes_lock[cpu]);
}

static inline void remove_heavy_and_low(struct sauron * sauron, struct eye *eye)
{
    int cpu = eye->cpu;

    spin_lock(&sauron->heavy_and_low_eyes_lock[cpu]);
    if (sauron->heavy_eyes[cpu] == eye)
        sauron->heavy_eyes[cpu] = NULL;

    if (sauron->low_eyes[cpu] == eye)
        sauron->low_eyes[cpu] = NULL;
    spin_unlock(&sauron->heavy_and_low_eyes_lock[cpu]);
}

static inline struct eye* pick_heavy(struct sauron * sauron , int cpu)
{
    struct eye* eye = 0;
    spin_lock(&sauron->heavy_and_low_eyes_lock[cpu]);
    eye = sauron->heavy_eyes[cpu];
    spin_unlock(&sauron->heavy_and_low_eyes_lock[cpu]);
    return eye;
}

static inline struct eye* pick_low(struct sauron * sauron , int cpu)
{
    struct eye* eye = 0;
    spin_lock(&sauron->heavy_and_low_eyes_lock[cpu]);
    eye = sauron->low_eyes[cpu];
    spin_unlock(&sauron->heavy_and_low_eyes_lock[cpu]);
    return eye;
}

static void del_monitor_flow(struct sauron* sauron, struct eye * eye , unsigned int from , u32 gro)
{
    if(!eye->monitored)
        return;

    sauron->target_cnt--;
    if(gro) sauron->target_gro_flow_cnt_by_cpus[from]--;
    else    sauron->target_flow_cnt_by_cpus[from]--;

    eye->monitored = 0;
}

static void add_monitor_flow(struct sauron* sauron, struct eye * eye , unsigned int to, u32 gro)
{
    if(eye->monitored)
        return;

    sauron->target_cnt++;
    if(gro) sauron->target_gro_flow_cnt_by_cpus[to]++;
    else    sauron->target_flow_cnt_by_cpus[to]++;

    eye->monitored = 1;
}

void move_monitor_flow(struct sauron* sauron, struct eye *eye , unsigned int from , unsigned int to, u32 gro);

struct sauron_statistics
{
    unsigned int cpu_distribute;
};

DECLARE_PER_CPU(struct sauron_statistics , sauron_stats);

#define NORMAL_SHIFT 10   // about 10^3
#define NORMAL_SCALE (1L << NORMAL_SHIFT)

#define ASSIGN_MAX(A,B) (A = ((A < B)? B : A))
#define PERCENT(V,M) ((V * 100) / M)
#define NORMALIZATION(V,M) ((V * NORMAL_SCALE) / M)
#define PRINT_FACTOR(L,B,W,V,M) { \
    L += scnprintf(B + L , PAGE_SIZE , "+(%u*%3u/%3u)",W,V,M);   \
};

/* Declare functions */
void init_mcps_core(void);
void release_mcps_core(void);
void init_sauron(struct sauron* sauron);

int get_arps_cpu(struct mcps_config * mcps ,  struct sk_buff *skb , u32 hash , u32 gro);
struct eye* add_flow(struct sk_buff *skb, unsigned int hash, int cpu, u32 gro);
int flush_flows(int force);

void __delete_flow(struct sauron* sauron, struct eye * eye , int cpu , u32 gro);
void __add_flow(struct sauron* sauron, struct eye * eye , int cpu , u32 gro);
void delete_flow(struct sauron* sauron , struct eye* flow);

struct eye* search_flow_rcu(u32 hash);
int move_flow (struct sauron * sauron , unsigned int cpu , struct eye * flow);
int _move_flow(unsigned int hash , unsigned int to);

int migrate_flow (unsigned int from, unsigned int to , unsigned int option);

unsigned int light_cpu (u32 gro);
#endif
