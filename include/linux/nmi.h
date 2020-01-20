/* SPDX-License-Identifier: GPL-2.0 */
/*
 *  linux/include/linux/nmi.h
 */
#ifndef LINUX_NMI_H
#define LINUX_NMI_H

#include <linux/sched.h>
#include <asm/irq.h>
#if defined(CONFIG_HAVE_NMI_WATCHDOG)
#include <asm/nmi.h>
#endif

#ifdef CONFIG_SEC_DEBUG
#define TASK_COMM_LEN 16
#define SOFTIRQ_TYPE_LEN 16

enum hardlockup_type {
	HL_TASK_STUCK = 1,
	HL_IRQ_STUCK,
	HL_IDLE_STUCK,
	HL_SMC_CALL_STUCK,
	HL_IRQ_STORM,
	HL_HRTIMER_ERROR,
	HL_UNKNOWN_STUCK
};

struct task_info {
	char task_comm[TASK_COMM_LEN];
};

struct cpuidle_info {
	char *mode;
};

struct smc_info {
	int cmd;
};

struct irq_info {
	int irq;
	void *fn;
	unsigned long long avg_period;
};

struct hardlockup_info {
	enum hardlockup_type hl_type;
	unsigned long long delay_time;
	union {
		struct task_info task_info;
		struct cpuidle_info cpuidle_info;
		struct smc_info smc_info;
		struct irq_info irq_info;
	};
};

struct softirq_info {
	ktime_t last_arrival;
	char softirq_type[SOFTIRQ_TYPE_LEN];
	void *fn;
};

enum softlockup_type {
	SL_SOFTIRQ_STUCK = 1,
	SL_TASK_STUCK,
	SL_UNKNOWN_STUCK
};

struct softlockup_info {
	enum softlockup_type sl_type;
	unsigned long long delay_time;
	int preempt_count;
	union {
		struct softirq_info softirq_info;
		struct task_info task_info;
	};
};
#endif

#ifdef CONFIG_LOCKUP_DETECTOR
void lockup_detector_init(void);
void lockup_detector_soft_poweroff(void);
void lockup_detector_cleanup(void);
bool is_hardlockup(void);

extern int watchdog_user_enabled;
extern int nmi_watchdog_user_enabled;
extern int soft_watchdog_user_enabled;
extern int watchdog_thresh;
extern unsigned long watchdog_enabled;

extern struct cpumask watchdog_cpumask;
extern unsigned long *watchdog_cpumask_bits;
#ifdef CONFIG_SMP
extern int sysctl_softlockup_all_cpu_backtrace;
extern int sysctl_hardlockup_all_cpu_backtrace;
#else
#define sysctl_softlockup_all_cpu_backtrace 0
#define sysctl_hardlockup_all_cpu_backtrace 0
#endif /* !CONFIG_SMP */

#else /* CONFIG_LOCKUP_DETECTOR */
static inline void lockup_detector_init(void) { }
static inline void lockup_detector_soft_poweroff(void) { }
static inline void lockup_detector_cleanup(void) { }
#endif /* !CONFIG_LOCKUP_DETECTOR */

#ifdef CONFIG_SOFTLOCKUP_DETECTOR
extern void touch_softlockup_watchdog_sched(void);
extern void touch_softlockup_watchdog(void);
extern void touch_softlockup_watchdog_sync(void);
extern void touch_all_softlockup_watchdogs(void);
extern unsigned int  softlockup_panic;
#else
static inline void touch_softlockup_watchdog_sched(void) { }
static inline void touch_softlockup_watchdog(void) { }
static inline void touch_softlockup_watchdog_sync(void) { }
static inline void touch_all_softlockup_watchdogs(void) { }
#endif

#if defined(CONFIG_SEC_DEBUG) && defined(CONFIG_SOFTLOCKUP_DETECTOR)
extern void sl_softirq_entry(const char *, void *);
extern void sl_softirq_exit(void);
unsigned long long get_dss_softlockup_thresh(void);
#else
static inline void void sl_softirq_entry(const char *, void *) { }
static inline void sl_softirq_exit(void) { }
#endif

#ifdef CONFIG_DETECT_HUNG_TASK
void reset_hung_task_detector(void);
#else
static inline void reset_hung_task_detector(void) { }
#endif

/*
 * The run state of the lockup detectors is controlled by the content of the
 * 'watchdog_enabled' variable. Each lockup detector has its dedicated bit -
 * bit 0 for the hard lockup detector and bit 1 for the soft lockup detector.
 *
 * 'watchdog_user_enabled', 'nmi_watchdog_user_enabled' and
 * 'soft_watchdog_user_enabled' are variables that are only used as an
 * 'interface' between the parameters in /proc/sys/kernel and the internal
 * state bits in 'watchdog_enabled'. The 'watchdog_thresh' variable is
 * handled differently because its value is not boolean, and the lockup
 * detectors are 'suspended' while 'watchdog_thresh' is equal zero.
 */
#define NMI_WATCHDOG_ENABLED_BIT   0
#define SOFT_WATCHDOG_ENABLED_BIT  1
#define NMI_WATCHDOG_ENABLED      (1 << NMI_WATCHDOG_ENABLED_BIT)
#define SOFT_WATCHDOG_ENABLED     (1 << SOFT_WATCHDOG_ENABLED_BIT)

#if defined(CONFIG_HARDLOCKUP_DETECTOR) || defined(CONFIG_HARDLOCKUP_DETECTOR_OTHER_CPU)
extern void hardlockup_detector_disable(void);
extern unsigned int hardlockup_panic;
#else
static inline void hardlockup_detector_disable(void) {}
#endif

#if defined(CONFIG_HAVE_NMI_WATCHDOG) || defined(CONFIG_HARDLOCKUP_DETECTOR)
# define NMI_WATCHDOG_SYSCTL_PERM	0644
#else
# define NMI_WATCHDOG_SYSCTL_PERM	0444
#endif

#if defined(CONFIG_HARDLOCKUP_DETECTOR_PERF)
extern void arch_touch_nmi_watchdog(void);
extern void hardlockup_detector_perf_stop(void);
extern void hardlockup_detector_perf_restart(void);
extern void hardlockup_detector_perf_disable(void);
extern void hardlockup_detector_perf_enable(void);
extern void hardlockup_detector_perf_cleanup(void);
extern int hardlockup_detector_perf_init(void);
#else
static inline void hardlockup_detector_perf_stop(void) { }
static inline void hardlockup_detector_perf_restart(void) { }
static inline void hardlockup_detector_perf_disable(void) { }
static inline void hardlockup_detector_perf_enable(void) { }
static inline void hardlockup_detector_perf_cleanup(void) { }
# if !defined(CONFIG_HAVE_NMI_WATCHDOG)
static inline int hardlockup_detector_perf_init(void) { return -ENODEV; }
static inline void arch_touch_nmi_watchdog(void) {}
# else
static inline int hardlockup_detector_perf_init(void) { return 0; }
# endif
#endif

void watchdog_nmi_stop(void);
void watchdog_nmi_start(void);
int watchdog_nmi_probe(void);
int watchdog_nmi_enable(unsigned int cpu);
void watchdog_nmi_disable(unsigned int cpu);

/**
 * touch_nmi_watchdog - restart NMI watchdog timeout.
 *
 * If the architecture supports the NMI watchdog, touch_nmi_watchdog()
 * may be used to reset the timeout - for code which intentionally
 * disables interrupts for a long time. This call is stateless.
 */
#if defined(CONFIG_HARDLOCKUP_DETECTOR_OTHER_CPU)
extern void touch_nmi_watchdog(void);

#if defined(CONFIG_SEC_DEBUG)
extern void update_hardlockup_type(unsigned int cpu);
unsigned long long get_hardlockup_thresh(void);
#endif
#else
static inline void touch_nmi_watchdog(void)
{
	arch_touch_nmi_watchdog();
	touch_softlockup_watchdog();
}
#endif

/*
 * Create trigger_all_cpu_backtrace() out of the arch-provided
 * base function. Return whether such support was available,
 * to allow calling code to fall back to some other mechanism:
 */
#ifdef arch_trigger_cpumask_backtrace
static inline bool trigger_all_cpu_backtrace(void)
{
	arch_trigger_cpumask_backtrace(cpu_online_mask, false);
	return true;
}

static inline bool trigger_allbutself_cpu_backtrace(void)
{
	arch_trigger_cpumask_backtrace(cpu_online_mask, true);
	return true;
}

static inline bool trigger_cpumask_backtrace(struct cpumask *mask)
{
	arch_trigger_cpumask_backtrace(mask, false);
	return true;
}

static inline bool trigger_single_cpu_backtrace(int cpu)
{
	arch_trigger_cpumask_backtrace(cpumask_of(cpu), false);
	return true;
}

/* generic implementation */
void nmi_trigger_cpumask_backtrace(const cpumask_t *mask,
				   bool exclude_self,
				   void (*raise)(cpumask_t *mask));
bool nmi_cpu_backtrace(struct pt_regs *regs);

#else
static inline bool trigger_all_cpu_backtrace(void)
{
	return false;
}
static inline bool trigger_allbutself_cpu_backtrace(void)
{
	return false;
}
static inline bool trigger_cpumask_backtrace(struct cpumask *mask)
{
	return false;
}
static inline bool trigger_single_cpu_backtrace(int cpu)
{
	return false;
}
#endif

#ifdef CONFIG_HARDLOCKUP_DETECTOR_PERF
u64 hw_nmi_get_sample_period(int watchdog_thresh);
#endif

#if defined(CONFIG_HARDLOCKUP_CHECK_TIMESTAMP) && \
    defined(CONFIG_HARDLOCKUP_DETECTOR)
void watchdog_update_hrtimer_threshold(u64 period);
#else
static inline void watchdog_update_hrtimer_threshold(u64 period) { }
#endif

struct ctl_table;
extern int proc_watchdog(struct ctl_table *, int ,
			 void __user *, size_t *, loff_t *);
extern int proc_nmi_watchdog(struct ctl_table *, int ,
			     void __user *, size_t *, loff_t *);
extern int proc_soft_watchdog(struct ctl_table *, int ,
			      void __user *, size_t *, loff_t *);
extern int proc_watchdog_thresh(struct ctl_table *, int ,
				void __user *, size_t *, loff_t *);
extern int proc_watchdog_cpumask(struct ctl_table *, int,
				 void __user *, size_t *, loff_t *);

#ifdef CONFIG_HAVE_ACPI_APEI_NMI
#include <asm/nmi.h>
#endif

#endif
