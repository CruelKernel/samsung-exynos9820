#ifndef _MCTCP_DEVICE_H
#define _MCTCP_DEVICE_H
#include <linux/smp.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>

#include "mcps_sauron.h"

enum {
	MCPS_MODE_NONE			    = 0,
	MCPS_MODE_APP_CLUSTER	    = 1,
	MCPS_MODE_TP_THRESHOLD	  = 2,
	MCPS_MODE_SINGLE_MIGRATE	= 4,
	MCPS_MODES				  = 4
};

enum {
	// ~ NR_CPUS - 1
	MCPS_CPU_ON_PENDING = NR_CPUS,
#if defined(CONFIG_MCPS_V2)
	MCPS_CPU_DIRECT_GRO,
#endif // #if defined(CONFIG_MCPS_V2)
	MCPS_CPU_GRO_BYPASS,
	MCPS_CPU_ERROR,
};

enum {
	MCPS_ARPS_LAYER = 0,
#if CONFIG_MCPS_GRO_ENABLE
	MCPS_AGRO_LAYER,
#endif
	MCPS_TYPE_LAYER,
};

#define VALID_UCPU(c) (c < NR_CPUS)
#define VALID_CPU(c) (0 <= c && c < NR_CPUS)

#define NR_CLUSTER 4
#define MID_CLUSTER 3
#define BIG_CLUSTER 2
#define LIT_CLUSTER 1
#define ALL_CLUSTER 0

#if defined(CONFIG_SOC_EXYNOS9830) || defined(CONFIG_SOC_EXYNOS9820)
#define CLUSTER_MAP {LIT_CLUSTER, LIT_CLUSTER, LIT_CLUSTER, LIT_CLUSTER, MID_CLUSTER, MID_CLUSTER, BIG_CLUSTER, BIG_CLUSTER}
#else
#define CLUSTER_MAP {LIT_CLUSTER, LIT_CLUSTER, LIT_CLUSTER, LIT_CLUSTER, BIG_CLUSTER, BIG_CLUSTER, BIG_CLUSTER, BIG_CLUSTER}
#endif

const static int __mcps_cpu_cluster_map[NR_CPUS] = CLUSTER_MAP;

#define CLUSTER(c) __mcps_cpu_cluster_map[(c)]

/*Declare mcps_enable .. Sync...*/
extern int mcps_enable;

extern cpumask_var_t mcps_cpu_online_mask;

#define mcps_cpu_online(cpu)		cpumask_test_cpu((cpu), mcps_cpu_online_mask)

struct mcps_modes {
	unsigned int mode;
	struct rcu_head rcu;
};

extern struct mcps_modes __rcu *mcps_mode;

struct mcps_pantry {
	struct sk_buff_head process_queue;

	unsigned int		received_arps;
	unsigned int		processed;
	unsigned int		enqueued;
	unsigned int		ignored;

	unsigned int		cpu;

	struct mcps_pantry  *ipi_list;
	struct mcps_pantry  *ipi_next;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	call_single_data_t  csd ____cacheline_aligned_in_smp;
#else
	struct call_single_data csd ____cacheline_aligned_in_smp;
#endif

	unsigned int		dropped;
	struct sk_buff_head input_pkt_queue;
	struct napi_struct  rx_napi_struct;

	unsigned int		gro_processed;
#if defined(CONFIG_MCPS_V2)
	struct timespec	 gro_flush_time;
#endif // #if defined(CONFIG_MCPS_V2)
	struct timespec	 agro_flush_time;

	struct napi_struct  ipi_napi_struct;
#if defined(CONFIG_MCPS_V2)
	struct napi_struct  gro_napi_struct; // never be touched by other core.
#endif // #if defined(CONFIG_MCPS_V2)
#if defined(CONFIG_MCPS_CHUNK_GRO)
	int modem_napi_quota;
	int modem_napi_work;
#endif // #if defined(CONFIG_MCPS_CHUNK_GRO)
	int				 offline;
};

DECLARE_PER_CPU_ALIGNED(struct mcps_pantry, mcps_pantries);

DECLARE_PER_CPU_ALIGNED(struct mcps_pantry, mcps_gro_pantries);

static inline void pantry_lock(struct mcps_pantry *pantry)
{
	spin_lock(&pantry->input_pkt_queue.lock);
}

static inline void pantry_unlock(struct mcps_pantry *pantry)
{
	spin_unlock(&pantry->input_pkt_queue.lock);
}

static inline void pantry_ipi_lock(struct mcps_pantry *pantry)
{
	spin_lock(&pantry->process_queue.lock);
}

static inline void pantry_ipi_unlock(struct mcps_pantry *pantry)
{
	spin_unlock(&pantry->process_queue.lock);
}

static inline unsigned int pantry_read_enqueued(struct mcps_pantry *p)
{
	unsigned int ret = 0;

	pantry_lock(p);
	ret = p->enqueued;
	pantry_unlock(p);
	return ret;
}

struct arps_meta {
	struct rcu_head rcu;
	struct rps_map *maps[NR_CLUSTER];

	cpumask_var_t mask;
};

#define ARPS_MAP(m, t) (m->maps[t])
#define ARPS_MAP_RCU(m, t) (rcu_dereference(m)->maps[t])

extern struct arps_meta __rcu *static_arps;
extern struct arps_meta __rcu *dynamic_arps;
extern struct arps_meta __rcu *newflow_arps;
extern struct arps_meta *get_arps_rcu(void);
extern struct arps_meta *get_newflow_rcu(void);
extern struct arps_meta *get_static_arps_rcu(void);

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

static inline void init_arps_config(struct arps_config *config)
{
	int i;
	for (i = 0; i < NUM_FACTORS; i++) {
		config->weights[i] = 5;
	}
	config->weights[FACTOR_NFLO] = 2;
}

struct mcps_config {
	struct sauron			   sauron;

	struct arps_config __rcu	*arps_config;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
#else
	struct notifier_block	   *mcps_cpu_notifier;
	struct notifier_block	   *mcps_gro_cpu_notifier;
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

extern struct mcps_config *mcps;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
void mcps_napi_complete(struct napi_struct *n);
int mcps_gro_cpu_startup_callback(unsigned int ocpu);
int mcps_gro_cpu_teardown_callback(unsigned int ocpu);
#else
#define mcps_napi_complete(n) __napi_complete(n)
int mcps_gro_cpu_callback(struct notifier_block *notifier, unsigned long action, void *ocpu);
#endif
int del_mcps(struct mcps_config *mcps);
int init_mcps(struct mcps_config *mcps);

static inline int smp_processor_id_safe(void)
{
	int cpu = get_cpu();

	put_cpu();
	return cpu;
}

#if defined(CONFIG_MCPS_ICB)
void mcps_boost_clock(int cluster);
void init_mcps_icb(void);
#else
static inline void mcps_boost_clock(int cluster)
{
	return;
}

static inline void init_mcps_icb(void)
{
	return;
}
#endif // #if defined(CONFIG_MCPS_ICB)

#if defined(CONFIG_MCPS_ICGB) && \
	!defined(CONFIG_KLAT) && \
	(defined(CONFIG_SOC_EXYNOS9830) || defined(CONFIG_SOC_EXYNOS9820))
int check_mcps_in_addr(unsigned int addr);
int check_mcps_in6_addr(struct in6_addr *addr);
#else
static inline int check_mcps_in_addr(unsigned int addr)
{
	return 0;
}
static inline int check_mcps_in6_addr(struct in6_addr *addr)
{
	return 0;
}
#endif // #if defined(CONFIG_MCPS_ICGB) && !defined(CONFIG_KLAT) && (defined(CONFIG_SOC_EXYNOS9820) || defined(CONFIG_SOC_EXYNOS9630) || defined(CONFIG_SOC_EXYNOS9830))
#endif
