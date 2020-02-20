#ifndef _MCTCP_DEVICE_H
#define _MCTCP_DEVICE_H
#include <linux/smp.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>

enum {
    MCPS_MODE_NONE              = 0,
    MCPS_MODE_APP_CLUSTER       = 1,
    MCPS_MODE_TP_THRESHOLD      = 2,
    MCPS_MODE_SINGLE_MIGRATE    = 4,
    MCPS_MODES                  = 4
};

enum {
    MCPS_NON_GRO_DEVICE         = 0,
    MCPS_GRO_DEVICE             = 1,
};

#define VALID_UCPU(c) (c < NR_CPUS)
#define VALID_CPU(c) (0 <= c && c < NR_CPUS)

#ifdef CONFIG_CLUSTER_MIGRATION
#define NR_CLUSTER 3
#define BIG_CLUSTER 2
#define LIT_CLUSTER 1
#define ALL_CLUSTER 0
#define BEGIN_BIG_CPU 4
#define CLUSTER(c) (c >= BEGIN_BIG_CPU ? BIG_CLUSTER : LIT_CLUSTER)
#endif

/*To avoid floating point. Must divide 10000*/
#define CPU_BIG_CAPACITY_RATIO 3789
#define CPU_BIG_CAPACITY_y0 -6308
#define CPU_LIT_CAPACITY_RATIO 1107
#define CPU_LIT_CAPACITY_y0 -9008
#define CAPACITY_BIG(Hz) ((unsigned long)(((CPU_BIG_CAPACITY_RATIO * Hz) + CPU_BIG_CAPACITY_y0)/10000))
#define CAPACITY_LIT(Hz) ((unsigned long)(((CPU_LIT_CAPACITY_RATIO * Hz) + CPU_LIT_CAPACITY_y0)/10000))

/*Declare mcps_enable .. Sync...*/
extern int mcps_enable;

extern cpumask_var_t mcps_cpu_online_mask;

#define mcps_cpu_online(cpu)        cpumask_test_cpu((cpu), mcps_cpu_online_mask)

struct mcps_modes
{
    unsigned int mode;
    struct rcu_head rcu;
};

extern struct mcps_modes __rcu * mcps_mode;

struct mcps_pantry
{
    struct sk_buff_head process_queue;

    unsigned int        received_arps;
    unsigned int        processed;
    unsigned int        enqueued;
    unsigned int        ignored;

    unsigned int        cpu;

    struct mcps_pantry  *ipi_list;
    struct mcps_pantry  *ipi_next;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
    call_single_data_t  csd ____cacheline_aligned_in_smp;
#else
    struct call_single_data csd ____cacheline_aligned_in_smp;
#endif

    unsigned int        dropped;
    struct sk_buff_head input_pkt_queue;
    struct napi_struct  rx_napi_struct;

    unsigned int        gro_processed;
    struct timespec     flush_time;

};

extern u32 cpu_distributed[NR_CPUS];

DECLARE_PER_CPU_ALIGNED(struct mcps_pantry , mcps_pantries);

DECLARE_PER_CPU_ALIGNED(struct mcps_pantry , mcps_gro_pantries);

static inline void pantry_lock(struct mcps_pantry *pantry)
{
    spin_lock(&pantry->input_pkt_queue.lock);
}

static inline void pantry_unlock(struct mcps_pantry *pantry)
{
    spin_unlock(&pantry->input_pkt_queue.lock);
}

struct arps_meta {
    struct rcu_head rcu;
    struct rps_map* maps[NR_CLUSTER];

    cpumask_var_t mask;
};

#define ARPS_MAP(m , t) (m->maps[t])
#define ARPS_MAP_RCU(m , t) (rcu_dereference(m)->maps[t])

extern struct arps_meta __rcu *static_arps;
extern struct arps_meta __rcu *dynamic_arps;
extern struct arps_meta __rcu *newflow_arps;
extern struct arps_meta* get_arps_rcu(void);
extern struct arps_meta* get_newflow_rcu(void);
extern struct arps_meta* get_static_arps_rcu(void);

#define NUM_FACTORS 5
#define FACTOR_QLEN 0
#define FACTOR_PROC 1
#define FACTOR_NFLO 2
#define FACTOR_DROP 3
#define FACTOR_DIST 4
struct arps_config {
    struct rcu_head rcu;

    unsigned int weights[NUM_FACTORS];
};

static inline void init_arps_config (struct arps_config * config) {
    int i;
    for(i = 0 ; i < NUM_FACTORS; i++) {
        config->weights[i] = 5;
    }
    config->weights[FACTOR_NFLO] = 2;
}

struct mcps_cpu_map {
    cpumask_var_t cpumask;
    struct rcu_head rcu;
};

struct mcps_config {
    struct sauron __rcu         *sauron_body;
    struct mcps_cpu_map __rcu   *dist_cpu_map;

    struct arps_config __rcu    *arps_config;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
#else
    struct notifier_block       *mcps_cpu_notifier;
    struct notifier_block       *mcps_gro_cpu_notifier;
#endif
    struct rcu_head rcu;
} ____cacheline_aligned_in_smp;

struct mcps_attribute {
    struct attribute attr;
    ssize_t (*show)(struct mcps_config *config,
        struct mcps_attribute *attr, char *buf);
    ssize_t (*store)(struct mcps_config *config,
        struct mcps_attribute *attr, const char *buf, size_t len);
};

extern struct mcps_config __rcu * mcps;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
void mcps_napi_complete(struct napi_struct *n);
int mcps_gro_cpu_startup_callback(unsigned int ocpu);
int mcps_gro_cpu_teardown_callback(unsigned int ocpu);
#else
int mcps_gro_cpu_callback(struct notifier_block *notifier , unsigned long action, void* ocpu);
#endif
int del_mcps(struct mcps_config* mcps);
int init_mcps(struct mcps_config* mcps);

#ifdef CONFIG_MCTCP_DEBUG_PRINTK
extern int mcps_print_BBB;

#define PRINT_WORKED(C, W, Q) {     \
    if(mcps_print_BBB)  \
        printk(KERN_DEBUG "BBB WORKED[%d] : WORK[%d] / QUOTA[%d]\n", C, W ,Q);  \
};
#define PRINT_GRO_WORKED(C, W, Q) {     \
    if(mcps_print_BBB)  \
        printk(KERN_DEBUG "BBB WORKED[%d] : WORK[%d] / QUOTA[%d] GRO\n", C, W ,Q);  \
};
#define PRINT_IPIQ(FROM, TO) { \
    if(mcps_print_BBB)  \
        printk(KERN_DEBUG "BBB IPIQ : TO[%d] FROM[%d]\n", TO , FROM);  \
};
#define PRINT_GRO_IPIQ(FROM, TO) { \
    if(mcps_print_BBB)  \
        printk(KERN_DEBUG "BBB IPIQ : TO[%d] FROM[%d] GRO\n", TO , FROM);  \
};
#define PRINT_DO_IPI(TO) { \
    if(mcps_print_BBB)  \
        printk(KERN_DEBUG "BBB DOIPI : TO[%d]\n", TO);  \
};
#else
#define PRINT_WORKED(C, W, Q) {};
#define PRINT_GRO_WORKED(C, W, Q) {};
#define PRINT_IPIQ(FROM, TO) {};
#define PRINT_GRO_IPIQ(FROM, TO) {};
#define PRINT_DO_IPI(TO) {};
#endif

static inline int smp_processor_id_safe(void)
{
    int cpu = get_cpu();
    put_cpu();
    return cpu;
}

#endif
