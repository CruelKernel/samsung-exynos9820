#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hashtable.h>
#include <linux/mutex.h>
#include <linux/netdevice.h>
#include <linux/version.h>

#include <linux/cpufreq.h>
#include <linux/sched.h>

#include "mcps_sauron.h"
#include "mcps_device.h"
#include "mcps_buffer.h"
#include "mcps_debug.h"

#include "../../kernel/sched/sched.h"

#define HEAVY_FLOW 0
#define LIGHT_FLOW 1

#define FNULL 11
#define FWRONGCPU 12
#define FNOCACHE 17

DEFINE_PER_CPU(struct sauron_statistics , sauron_stats);

unsigned int mcps_set_cluster_for_newflow __read_mostly = NR_CLUSTER;
module_param(mcps_set_cluster_for_newflow , uint , 0640);
unsigned int mcps_set_cluster_for_hotplug __read_mostly = BIG_CLUSTER;
module_param(mcps_set_cluster_for_hotplug , uint , 0640);

#ifdef CONFIG_MCTCP_DEBUG_PRINTK
#define PRINT_FLOW_STAT(S) {   \
    int i = 0;  \
    char buf[100];   \
    size_t len = 0; \
    for(i = 0;i < NR_CPUS; i++) {   \
        len += scnprintf(buf + len , 100 , "%2u|" , S->target_gro_flow_cnt_by_cpus[i]);   \
    }   \
    MCPS_DEBUG("FLOW-STATS : %s\n" , buf); \
};
#else
#define PRINT_FLOW_STAT(S) {};
#endif

/* Delete flow from cached cpu flow.
 * Notice that this function must be called in brace nested by spin lock
 * */
void __delete_flow(struct sauron* sauron, struct eye * eye , int cpu , u32 gro)
{
    sauron->sauron_len--;

    remove_heavy_and_low(sauron, eye);
    if(gro) sauron->gro_flow_cnt_by_cpus[cpu]--;
    else    sauron->flow_cnt_by_cpus[cpu]--;
}

void __add_flow(struct sauron* sauron, struct eye * eye , int cpu , u32 gro)
{
    sauron->sauron_len++;

    eye->cpu = cpu;
    if(gro) sauron->gro_flow_cnt_by_cpus[cpu]++;
    else    sauron->flow_cnt_by_cpus[cpu]++;

    MCPS_DEBUG("[%u] flow to [%d] \n" , eye->hash , cpu);
}

/* search_flow - search flow node from hash table with hash value.
 * @sauron: object in which flow nodes are managed.
 * @hash: hash value from skb
 *
 * return values:
 * NULL (no exist)
 * struct eye* (exist node)
 * */
struct eye* __search_flow_rcu(struct sauron* sauron, u32 hash)
{
    unsigned int idx = (unsigned int)(hash % HASH_SIZE(sauron->sauron_eyes));
    struct eye* eye = NULL;

    hlist_for_each_entry_rcu(eye , &sauron->sauron_eyes[idx] , eye_hash_node)
    {
        if(eye->hash == hash)
            return eye;
    }

    return NULL;
}

struct eye* search_flow_rcu(u32 hash)
{
    struct sauron* sauron;
    if(!mcps)
        goto error;

    sauron = rcu_dereference(mcps->sauron_body);

    if(!sauron)
        goto error;

    return __search_flow_rcu(sauron , hash);

error :
    return NULL;
}

struct eye* search_flow_lock(struct sauron * sauron , u32 hash) {
    unsigned int idx = (unsigned int)(hash % HASH_SIZE(sauron->sauron_eyes));
    struct eye* eye = NULL;
    struct hlist_node *temp = NULL;

    hlist_for_each_entry_safe(eye, temp , &sauron->sauron_eyes[idx], eye_hash_node)
    {
        if (eye->hash == hash)
            return eye;
    }

    return NULL;
}

/* delete_flow - Delete flow from sauron
 *
 * This function must be called inside of locking brace.
 *
 * */
void delete_flow(struct sauron* sauron , struct eye* flow)
{
    struct eye* eye = flow;

    if(eye == NULL) //must set debug on!
        return;

    hash_del_rcu(&eye->eye_hash_node);
    __delete_flow(sauron , eye , eye->cpu , eye->gro);
    kfree_rcu(eye , rcu);
}

/* __move_flow - move selected flow to selected core(to)
 * */
static int
__move_flow(struct sauron * sauron , unsigned int to, struct eye * flow)
{
    spin_lock(&sauron->sauron_eyes_lock);

    move_monitor_flow(sauron, flow, flow->cpu , to, flow->gro);

    __delete_flow(sauron , flow , flow->cpu , flow->gro);
    __add_flow(sauron, flow, to , flow->gro);
    update_heavy_and_low(sauron, flow);

    PRINT_MOVE(flow->hash , to , 0);
    PRINT_FLOW_STAT(sauron);

    spin_unlock(&sauron->sauron_eyes_lock);

    return 0;
}

// Must be called inside of rcu read section
int _move_flow(unsigned int hash , unsigned int to)
{
    struct sauron * sauron = rcu_dereference(mcps->sauron_body);
    struct eye * flow = __search_flow_rcu(sauron, hash);

    if(!flow)
        return -1;

    if(flow->cpu == to)
        return -FWRONGCPU;

    return __move_flow(sauron , to , flow);
}

int
move_flow(struct sauron * sauron , unsigned int to, struct eye * flow)
{
    int ret;

    if(!flow) {
        PRINT_MOVE(0 , to , -FNULL);
        return -FNULL;
    }

    if(to >= NR_CPUS) {
        PRINT_MOVE(flow->hash , to , -FWRONGCPU);
        return -FWRONGCPU;
    }

    ret = pending_migration(flow->hash, flow->cpu, to, flow->gro);
#ifdef CONFIG_MCPS_DEBUG
    if(!ret) {
        int vec = ((CLUSTER(flow->cpu)) | (CLUSTER(to) << 2));
        mcps_migrate_flow_history(flow , vec);
    }
#endif

    return ret;
}

void move_monitor_flow(struct sauron* sauron, struct eye *eye , unsigned int from , unsigned int to, u32 gro)
{
    if(!eye->monitored)
        return;

    del_monitor_flow(sauron, eye , from, gro);
    add_monitor_flow(sauron, eye , to ,gro);
}

int migrate_flow (unsigned int from, unsigned int to , unsigned int option)
{
    int ret = 0;
    struct eye * flow = NULL;
    struct sauron * sauron = NULL;

    if(!mcps_cpu_online(from) || !mcps_cpu_online(to))
        return -EINVAL;

    rcu_read_lock();
    sauron = rcu_dereference(mcps->sauron_body);
    if(!sauron) {
        ret = -EINVAL;
        rcu_read_unlock();
        goto pass;
    }

    switch(option) {
        case HEAVY_FLOW :
            flow = pick_heavy(sauron, from);
            break;
        case LIGHT_FLOW :
            flow = pick_low(sauron, from);
            break;
    }

    if (!flow) {
        ret = -FNOCACHE;
        rcu_read_unlock();
        goto pass;
    }

    ret = move_flow(sauron, to, flow);
    rcu_read_unlock();
pass:
    PRINT_MIGRATE(from, to, ret);

    return ret;
}

struct eye* add_flow(struct sk_buff* skb , unsigned int hash, int cpu, u32 gro)
{
    int ret = 0;
    unsigned int    idx;
    struct eye      *eye;
    struct sauron   *sauron;

    if(!mcps) {
        ret = -EINVAL;
        goto error;
    }

    eye = (struct eye*) kzalloc(sizeof(struct eye), GFP_ATOMIC);
    if (!eye) {
        ret = -ENOMEM;
        goto error;
    }

    init_eye(eye, hash, gro);
    update_protocol_info(skb, eye);

    rcu_read_lock();
    sauron = rcu_dereference(mcps->sauron_body);
    idx = (unsigned int)(hash % HASH_SIZE(sauron->sauron_eyes));
    spin_lock(&sauron->sauron_eyes_lock);
    // I think we should re-find hash value... cuz RCU
    if(search_flow_lock(sauron , hash)) {
        spin_unlock(&sauron->sauron_eyes_lock);
        rcu_read_unlock();
        kfree(eye);

        ret = -EINVAL;
        goto error;
    }
    hlist_add_head_rcu(&eye->eye_hash_node, &sauron->sauron_eyes[idx]);
    __add_flow(sauron , eye , cpu, eye->gro);
    spin_unlock(&sauron->sauron_eyes_lock);
    rcu_read_unlock();
    return eye;
error:
    MCPS_DEBUG("Fail to add flow : reason [%d]\n", ret);
    return NULL;
}

static unsigned long last_flush = 0;
int flush_flows(int force) {
    unsigned long now = jiffies;
    struct sauron * sauron;
    struct eye *old_eye;
    struct hlist_node *temp;
    int bkt = 0;
    int res = 0;

    if(force)
        goto skip;

    if (!time_after(now, last_flush + 10 * HZ))
        return 0;

skip:
    sauron = rcu_dereference(mcps->sauron_body);

    last_flush = jiffies;
    spin_lock(&sauron->sauron_eyes_lock);
    hash_for_each_safe(sauron->sauron_eyes , bkt, temp, old_eye , eye_hash_node)
    {
        if(old_eye && time_after(last_flush , old_eye->t_stamp + 10*HZ)) {
            dump(old_eye);
            del_monitor_flow(sauron, old_eye, old_eye->cpu , old_eye->gro);
            delete_flow(sauron , old_eye);
            res++;
        }
    }
    spin_unlock(&sauron->sauron_eyes_lock);
    MCPS_DEBUG("flush! %d flow\n" , res);
    PRINT_FLOW_STAT(sauron);
    return res;
}
EXPORT_SYMBOL(flush_flows);

static inline unsigned int app_core (unsigned int hash)
{
    unsigned int cpu_on_app = NR_CPUS;
    struct rps_sock_flow_table * sock_flow_table;
    sock_flow_table = rcu_dereference(rps_sock_flow_table);

    if(sock_flow_table) {
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

    if(cpu_on_app >= NR_CPUS)
        cpu_on_app = smp_processor_id_safe();

    return cpu_on_app;
}

static inline int
__get_rand_cpu (struct rps_map *map, u32 hash) {
    int cpu = -1;

    cpu = map->cpus[reciprocal_scale(hash, map->len)];
    if(mcps_cpu_online(cpu)) {
        return cpu;
    }

    return -1;
}

static int
get_rand_cpu (struct arps_meta * arps, u32 hash , unsigned int cluster)
{
    int cpu = 0;
    struct rps_map *map = NULL;

    if(cluster >= NR_CLUSTER)
        cluster = ALL_CLUSTER;

    map = ARPS_MAP(arps, cluster);
    if(!map->len) {
        map = ARPS_MAP(arps, ALL_CLUSTER);
    }

    cpu = __get_rand_cpu(map, hash);
    if(VALID_CPU(cpu))
        return cpu;

    for_each_cpu_and(cpu, arps->mask , mcps_cpu_online_mask) {
        if(cpu < 0)
            continue;
        return cpu;
    }

    //current
    return smp_processor_id_safe();
}

unsigned int light_cpu(u32 gro)
{
    unsigned int cpu = 0;
    struct arps_meta * arps;
    rcu_read_lock();
    arps = get_arps_rcu();
    if(!arps) {
        rcu_read_unlock();
        return 0;
    }

    cpu = get_rand_cpu(arps , (u32)jiffies , mcps_set_cluster_for_hotplug);
    rcu_read_unlock();

    return cpu;
}

void update_mask
(struct sauron* sauron, struct arps_meta *arps, struct eye* eye, unsigned int cluster)
{
    int cpu = get_rand_cpu(arps , eye->hash , cluster);
    __move_flow(sauron , cpu , eye);
}

int get_arps_cpu(struct mcps_config * mcps , struct sk_buff *skb , u32 hash , u32 gro) {
    struct sauron_statistics *stat = this_cpu_ptr(&sauron_stats);
    struct sauron    *sauron    = NULL;
    struct eye       *eye       = NULL;
    struct arps_meta *arps      = NULL;

    stat->cpu_distribute++;

    if(!mcps)
        goto error;

    //step.2 SEARCH
    sauron  = rcu_dereference(mcps->sauron_body);
    arps    = get_arps_rcu();

    if(!arps || !sauron) {
        goto error;
    }

    eye = __search_flow_rcu(sauron , hash);
    if(!eye) {
        unsigned int cluster = (mcps_set_cluster_for_newflow >= NR_CLUSTER) ? CLUSTER(smp_processor_id_safe()) : mcps_set_cluster_for_newflow;
        struct arps_meta *newflow_arps = get_newflow_rcu();
        int cpu = get_rand_cpu(newflow_arps , hash, cluster);

        rcu_read_unlock();
        eye = add_flow(skb, hash , cpu , gro);
        rcu_read_lock();
        if(!eye) {
            goto error;
        }

        arps        = get_arps_rcu();
        sauron      = rcu_dereference(mcps->sauron_body);

        if(!arps || !sauron) {
            goto error;
        }
    }

    mcps_in_packet(eye, skb);

    eye->value++;
    eye->t_stamp = jiffies;

    if(!update_pps(sauron , eye , gro)) {
        update_heavy_and_low(sauron , eye);
    }

    if(ON_PENDING(hash, skb, gro)) {// it should be moved into enqueue_to_pantry
        return NR_CPUS;
    }

    if(!cpumask_test_cpu(eye->cpu , arps->mask)) {
        update_mask(sauron, arps, eye, CLUSTER(eye->cpu));
        goto changed;
    }

    if(!mcps_cpu_online(eye->cpu)) {
        int ret = try_to_hqueue(eye->hash , eye->cpu, skb, gro);
        if(ret < 0)
            return NR_CPUS;

        goto changed;
    }

changed:
    return eye->cpu;
error:
    stat->cpu_distribute--;
    return -EINVAL;
}
EXPORT_SYMBOL(get_arps_cpu);

void init_mcps_core(void)
{
    init_mcps_buffer();
}

void release_mcps_core(void)
{
    release_mcps_buffer();
}
