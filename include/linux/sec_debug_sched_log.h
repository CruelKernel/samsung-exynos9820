/*
 * include/linux/sec_debug_sched_log.h
 *
 * COPYRIGHT(C) 2017 Samsung Electronics Co., Ltd. All Right Reserved.
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

#ifndef __SEC_DEBUG_SCHED_LOG_INDIRECT
#warning "sec_debug_sched_log.h is included directly."
#error "please include sec_debug.h instead of this file"
#endif

#ifndef __SEC_DEBUG_SCHED_LOG_H__
#define __SEC_DEBUG_SCHED_LOG_H__

#define NO_ATOMIC_IDX	//stlxr & ldaxr free

#ifndef CONFIG_SEC_DEBUG_SCHED_LOG

static inline void sec_debug_save_last_pet(unsigned long long last_pet) {}
static inline void sec_debug_save_last_ns(unsigned long long last_ns) {}
static inline void sec_debug_irq_enterexit_log(unsigned int irq,
		u64 start_time) {}
static inline void sec_debug_task_sched_log_short_msg(char *msg) {}
static inline void sec_debug_task_sched_log(int cpu, bool preempt,
		struct task_struct *task, struct task_struct *prev) {}
static inline void sec_debug_timer_log(unsigned int type, int int_lock,
		void *fn) {}
static inline void sec_debug_secure_log(u32 svc_id, u32 cmd_id) {}
static inline void sec_debug_irq_sched_log(unsigned int irq, void *fn,
		char *name, unsigned int en) {}

/* CONFIG_SEC_DEBUG_MSG_LOG */
#define sec_debug_msg_log(caller, fmt, ...)
#define secdbg_msg(fmt, ...)

/* CONFIG_SEC_DEBUG_AVC_LOG */
#define sec_debug_avc_log(fmt, ...)
#define secdbg_avc(fmt, ...)

/* CONFIG_SEC_DEBUG_DCVS_LOG */
static inline void sec_debug_dcvs_log(int cpu_no, unsigned int prev_freq,
		unsigned int new_freq) {}

/* CONFIG_SEC_DEBUG_FUELGAUGE_LOG */
static inline void sec_debug_fuelgauge_log(unsigned int voltage,
		unsigned short soc, unsigned short charging_status) {}

/* CONFIG_SEC_DEBUG_POWER_LOG */
static inline void sec_debug_cpu_lpm_log(int cpu, unsigned int index,
		bool success, int entry_exit) {}
static inline void sec_debug_cluster_lpm_log(const char *name, int index,
		unsigned long sync_cpus, unsigned long child_cpus,
		bool from_idle, int entry_exit) {}
static inline void sec_debug_clock_log(const char *name, unsigned int state,
		unsigned int cpu_id, int complete) {}
#define sec_debug_clock_rate_log(name, state, cpu_id)
#define sec_debug_clock_rate_complete_log(name, state, cpu_id)

#else /* CONFIG_SEC_DEBUG_SCHED_LOG */

void sec_debug_save_last_pet(unsigned long long last_pet);
void sec_debug_save_last_ns(unsigned long long last_ns);

#define SCHED_LOG_MAX			512
#define TZ_LOG_MAX			64

struct irq_log {
	u64 time;
	unsigned int irq;
	void *fn;
	char *name;
	int en;
	int preempt_count;
	void *context;
	pid_t pid;
	unsigned int entry_exit;
};

struct secure_log {
	u64 time;
	u32 svc_id, cmd_id;
	pid_t pid;
};

struct irq_exit_log {
	unsigned int irq;
	u64 time;
	u64 end_time;
	u64 elapsed_time;
	pid_t pid;
};

struct sched_log {
	u64 time;
	char comm[TASK_COMM_LEN];
	pid_t pid;
	struct task_struct *pTask;
	char prev_comm[TASK_COMM_LEN];
	int prio;
	pid_t prev_pid;
	int prev_prio;
	int prev_state;
};

struct timer_log {
	u64 time;
	unsigned int type;
	int int_lock;
	void *fn;
	pid_t pid;
};

#define secdbg_sched_msg(fmt, ...)					\
do {									\
	char ___buf[16];						\
	snprintf(___buf, sizeof(___buf), fmt, ##__VA_ARGS__);		\
	sec_debug_task_sched_log_short_msg(___buf);			\
} while (0)

void sec_debug_irq_enterexit_log(unsigned int irq, u64 start_time);
void sec_debug_task_sched_log_short_msg(char *msg);
void sec_debug_task_sched_log(int cpu, bool preempt,
		struct task_struct *task, struct task_struct *prev);
void sec_debug_timer_log(unsigned int type, int int_lock, void *fn);
void sec_debug_secure_log(u32 svc_id, u32 cmd_id);
void sec_debug_irq_sched_log(unsigned int irq, void *fn,
		char *name, unsigned int en);

#ifdef CONFIG_SEC_DEBUG_MSG_LOG
#define MSG_LOG_MAX			1024

struct secmsg_log {
	u64 time;
	char msg[64];
	void *caller0;
	void *caller1;
	char *task;
};

int sec_debug_msg_log(void *caller, const char *fmt, ...);

#define secdbg_msg(fmt, ...) \
	sec_debug_msg_log(__builtin_return_address(0), fmt, ##__VA_ARGS__)
#else /* CONFIG_SEC_DEBUG_MSG_LOG */
#define sec_debug_msg_log(caller, fmt, ...)
#define secdbg_msg(fmt, ...)
#endif /* CONFIG_SEC_DEBUG_MSG_LOG */

/* KNOX_SEANDROID_START */
#ifdef CONFIG_SEC_DEBUG_AVC_LOG
#define AVC_LOG_MAX			256
struct secavc_log {
	char msg[256];
};

int sec_debug_avc_log(const char *fmt, ...);

#define secdbg_avc(fmt, ...) \
	sec_debug_avc_log(fmt, ##__VA_ARGS__)
#else /* CONFIG_SEC_DEBUG_AVC_LOG */
#define sec_debug_avc_log(fmt, ...)
#define secdbg_avc(fmt, ...)
#endif /* CONFIG_SEC_DEBUG_AVC_LOG */
/* KNOX_SEANDROID_END */

#ifdef CONFIG_SEC_DEBUG_DCVS_LOG
#define DCVS_LOG_MAX			256

struct dcvs_debug {
	u64 time;
	int cpu_no;
	unsigned int prev_freq;
	unsigned int new_freq;
};
void sec_debug_dcvs_log(int cpu_no, unsigned int prev_freq,
			unsigned int new_freq);
#else /* CONFIG_SEC_DEBUG_DCVS_LOG */
static inline void sec_debug_dcvs_log(int cpu_no, unsigned int prev_freq,
		unsigned int new_freq) {}
#endif /* CONFIG_SEC_DEBUG_DCVS_LOG */

#ifdef CONFIG_SEC_DEBUG_FUELGAUGE_LOG
#define FG_LOG_MAX			128

struct fuelgauge_debug {
	u64 time;
	unsigned int voltage;
	unsigned short soc;
	unsigned short charging_status;
};

void sec_debug_fuelgauge_log(unsigned int voltage, unsigned short soc,
		unsigned short charging_status);
#else /* CONFIG_SEC_DEBUG_FUELGAUGE_LOG */
static inline void sec_debug_fuelgauge_log(unsigned int voltage,
		unsigned short soc, unsigned short charging_status) {}
#endif /* CONFIG_SEC_DEBUG_FUELGAUGE_LOG */

#ifdef CONFIG_SEC_DEBUG_POWER_LOG
#define POWER_LOG_MAX			256

enum {
	CLUSTER_POWER_TYPE,
	CPU_POWER_TYPE,
	CLOCK_RATE_TYPE,
};

struct cluster_power {
	char *name;
	int index;
	unsigned long sync_cpus;
	unsigned long child_cpus;
	bool from_idle;
	int entry_exit;
};

struct cpu_power {
	int index;
	int entry_exit;
	bool success;
};

struct clock_rate {
	char *name;
	u64 state;
	u64 cpu_id;
	int complete;
};

struct power_log {
	u64 time;
	pid_t pid;
	unsigned int type;
	union {
		struct cluster_power cluster;
		struct cpu_power cpu;
		struct clock_rate clk_rate;
	};
};

void sec_debug_cpu_lpm_log(int cpu, unsigned int index,
		bool success, int entry_exit);
void sec_debug_cluster_lpm_log(const char *name, int index,
		unsigned long sync_cpus, unsigned long child_cpus,
		bool from_idle, int entry_exit);
void sec_debug_clock_log(const char *name, unsigned int state,
		unsigned int cpu_id, int complete);

#define sec_debug_clock_rate_log(name, state, cpu_id)			\
	sec_debug_clock_log((name), (state), (cpu_id), 0)
#define sec_debug_clock_rate_complete_log(name, state, cpu_id)		\
	sec_debug_clock_log((name), (state), (cpu_id), 1)
#else /* CONFIG_SEC_DEBUG_POWER_LOG */
static inline void sec_debug_cpu_lpm_log(int cpu, unsigned int index,
		bool success, int entry_exit) {}
static inline void sec_debug_cluster_lpm_log(const char *name, int index,
		unsigned long sync_cpus, unsigned long child_cpus,
		bool from_idle, int entry_exit) {}
static inline void sec_debug_clock_log(const char *name, unsigned int state,
		unsigned int cpu_id, int complete) {}

#define sec_debug_clock_rate_log(name, state, cpu_id)
#define sec_debug_clock_rate_complete_log(name, state, cpu_id)
#endif	/* CONFIG_SEC_DEBUG_POWER_LOG */

struct sec_debug_log {
#ifdef NO_ATOMIC_IDX
	int idx_sched[NR_CPUS];
	struct sched_log sched[NR_CPUS][SCHED_LOG_MAX];

	int idx_irq[NR_CPUS];
	struct irq_log irq[NR_CPUS][SCHED_LOG_MAX];

	int idx_secure[NR_CPUS];
	struct secure_log secure[NR_CPUS][TZ_LOG_MAX];

	int idx_irq_exit[NR_CPUS];
	struct irq_exit_log irq_exit[NR_CPUS][SCHED_LOG_MAX];

	int idx_timer[NR_CPUS];
	struct timer_log timer_log[NR_CPUS][SCHED_LOG_MAX];

#ifdef CONFIG_SEC_DEBUG_MSG_LOG
	int idx_secmsg[NR_CPUS];
	struct secmsg_log secmsg[NR_CPUS][MSG_LOG_MAX];
#endif

#ifdef CONFIG_SEC_DEBUG_AVC_LOG
	int idx_secavc[NR_CPUS];
	struct secavc_log secavc[NR_CPUS][AVC_LOG_MAX];
#endif

#ifdef CONFIG_SEC_DEBUG_DCVS_LOG
	int dcvs_log_idx[NR_CPUS];
	struct dcvs_debug dcvs_log[NR_CPUS][DCVS_LOG_MAX];
#endif

#ifdef CONFIG_SEC_DEBUG_FUELGAUGE_LOG
	int fg_log_idx;
	struct fuelgauge_debug fg_log[FG_LOG_MAX];
#endif

#ifdef CONFIG_SEC_DEBUG_POWER_LOG
	int idx_power[NR_CPUS];
	struct power_log pwr_log[NR_CPUS][POWER_LOG_MAX];
#endif

	/* zwei variables -- last_pet und last_ns */
	unsigned long long last_pet;
	atomic64_t last_ns;

#else
	atomic_t idx_sched[NR_CPUS];
	struct sched_log sched[NR_CPUS][SCHED_LOG_MAX];

	atomic_t idx_irq[NR_CPUS];
	struct irq_log irq[NR_CPUS][SCHED_LOG_MAX];

	atomic_t idx_secure[NR_CPUS];
	struct secure_log secure[NR_CPUS][TZ_LOG_MAX];

	atomic_t idx_irq_exit[NR_CPUS];
	struct irq_exit_log irq_exit[NR_CPUS][SCHED_LOG_MAX];

	atomic_t idx_timer[NR_CPUS];
	struct timer_log timer_log[NR_CPUS][SCHED_LOG_MAX];

#ifdef CONFIG_SEC_DEBUG_MSG_LOG
	atomic_t idx_secmsg[NR_CPUS];
	struct secmsg_log secmsg[NR_CPUS][MSG_LOG_MAX];
#endif

#ifdef CONFIG_SEC_DEBUG_AVC_LOG
	atomic_t idx_secavc[NR_CPUS];
	struct secavc_log secavc[NR_CPUS][AVC_LOG_MAX];
#endif

#ifdef CONFIG_SEC_DEBUG_DCVS_LOG
	atomic_t dcvs_log_idx[NR_CPUS];
	struct dcvs_debug dcvs_log[NR_CPUS][DCVS_LOG_MAX];
#endif

#ifdef CONFIG_SEC_DEBUG_FUELGAUGE_LOG
	atomic_t fg_log_idx;
	struct fuelgauge_debug fg_log[FG_LOG_MAX];
#endif

#ifdef CONFIG_SEC_DEBUG_POWER_LOG
	atomic_t idx_power[NR_CPUS];
	struct power_log pwr_log[NR_CPUS][POWER_LOG_MAX];
#endif

	/* zwei variables -- last_pet und last_ns */
	unsigned long long last_pet;
	atomic64_t last_ns;
#endif
};
#endif /* CONFIG_SEC_DEBUG_SCHED_LOG */

#endif /* __SEC_DEBUG_SCHED_LOG_H__ */
