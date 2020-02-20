
#ifndef DEBUG_SNAPSHOT_LOG_H
#define DEBUG_SNAPSHOT_LOG_H

#include <generated/autoconf.h>

#ifdef CONFIG_DEBUG_SNAPSHOT_BINDER
#include <linux/debug-snapshot-binder.h>
#endif

#ifdef DSS_ANALYZER

#define TASK_COMM_LEN 16
#define NR_CPUS 8
#undef CONFIG_DEBUG_SNAPSHOT_LINUX_BUILD
#include <stdio.h>
#include <stdlib.h>

#else // DSS_ANALYZER

#include <linux/clk-provider.h>
#include <linux/debug-snapshot.h>
#include <linux/debug-snapshot-helper.h>

#endif // DSS_ANALYZER

#include <dt-bindings/soc/samsung/debug-snapshot-table.h>

/*  Size domain */
#define DSS_KEEP_HEADER_SZ		(SZ_256 * 3)
#define DSS_HEADER_SZ			SZ_4K
#define DSS_MMU_REG_SZ			SZ_4K
#define DSS_CORE_REG_SZ			SZ_4K
#define DSS_DBGC_LOG_SZ			SZ_4K
#define DSS_HEADER_TOTAL_SZ		(DSS_HEADER_SZ + DSS_MMU_REG_SZ + DSS_CORE_REG_SZ + DSS_DBGC_LOG_SZ)
#define DSS_SPARE_SZ			(DSS_HEADER_SIZE - DSS_HEADER_TOTAL_SZ)

/*  Length domain */
#define DSS_LOG_STRING_LEN		SZ_128
#define DSS_LOG_GEN_LEN			SZ_16
#define DSS_MMU_REG_OFFSET		SZ_512
#define DSS_CORE_REG_OFFSET		SZ_512
#define DSS_LOG_MAX_NUM			SZ_1K
#define DSS_API_MAX_NUM			SZ_2K
#define DSS_EX_MAX_NUM			SZ_8
#define DSS_IN_MAX_NUM			SZ_8
#define DSS_CALLSTACK_MAX_NUM		4
#define DSS_ITERATION			5
#define DSS_NR_CPUS			NR_CPUS
#define DSS_ITEM_MAX_NUM		16

/* Sign domain */
#define DSS_SIGN_RESET			0x0
#define DSS_SIGN_RESERVED		0x1
#define DSS_SIGN_SCRATCH		0xD
#define DSS_SIGN_ALIVE			0xFACE
#define DSS_SIGN_DEAD			0xDEAD
#define DSS_SIGN_PANIC			0xBABA
#define DSS_SIGN_SAFE_FAULT		0xFAFA
#define DSS_SIGN_NORMAL_REBOOT		0xCAFE
#define DSS_SIGN_FORCE_REBOOT		0xDAFE
#define DSS_SIGN_LOCKUP			0xDEADBEEF
#define DSS_SIGN_DEBUG_TEST		0xDB6

/*  Specific Address Information */
#define DSS_OFFSET_SCRATCH		(0x100)
#define DSS_OFFSET_DEBUG_TEST		(0x110)
#define DSS_OFFSET_DEBUG_TEST_CASE	(0x120)
#define DSS_OFFSET_DEBUG_TEST_NEXT	(0x128)
#define DSS_OFFSET_DEBUG_TEST_PANIC	(0x130)
#define DSS_OFFSET_DEBUG_TEST_WDT	(0x140)
#define DSS_OFFSET_DEBUG_TEST_WTSR	(0x150)
#define DSS_OFFSET_DEBUG_TEST_SMPL	(0x160)
#define DSS_OFFSET_DEBUG_TEST_CURR	(0x170)
#define DSS_OFFSET_DEBUG_TEST_TOTAL	(0x178)
#define DSS_OFFSET_DEBUG_LEVEL		(0x180)
#define DSS_OFFSET_DEBUG_TEST_RUN	(0x138)
#define DSS_OFFSET_LAST_LOGBUF		(0x200)
#define DSS_OFFSET_EMERGENCY_REASON	(0x300)
#define DSS_OFFSET_CORE_POWER_STAT	(0x400)
#define DSS_OFFSET_PANIC_STAT		(0x500)
#define DSS_OFFSET_CORE_LAST_PC		(0x600)
/* S5P_VA_SS_BASE + 0x700 -- 0x8FF is reserved */
#define DSS_OFFSET_LINUX_BANNER		(0x700)
#define DSS_OFFSET_ITEM_INFO		(0x900)

/* S5P_VA_SS_BASE + 0xC00 -- 0xFFF is reserved */
#define DSS_OFFSET_PANIC_STRING		(0xC00)
#define DSS_OFFSET_SPARE_BASE		(DSS_HEADER_TOTAL_SZ)

struct dbg_snapshot_log {
	struct __task_log {
		unsigned long long time;
		unsigned long sp;
#ifdef CONFIG_DEBUG_SNAPSHOT_LINUX_BUILD
		struct task_struct *task;
#else
		void *task;
#endif
		char task_comm[TASK_COMM_LEN];
		int pid;
	} task[DSS_NR_CPUS][DSS_LOG_MAX_NUM];

	struct __work_log {
		unsigned long long time;
		unsigned long sp;
#ifdef CONFIG_DEBUG_SNAPSHOT_LINUX_BUILD
		struct worker *worker;
		work_func_t fn;
#else
		void *worker;
		void *fn;
#endif
		char task_comm[TASK_COMM_LEN];
		int en;
	} work[DSS_NR_CPUS][DSS_LOG_MAX_NUM];

	struct __cpuidle_log {
		unsigned long long time;
		unsigned long sp;
		char *modes;
		unsigned int state;
#ifdef CONFIG_DEBUG_SNAPSHOT_LINUX_BUILD
		u32 num_online_cpus;
#else
		unsigned int num_online_cpus;
#endif
		int delta;
		int en;
	} cpuidle[DSS_NR_CPUS][DSS_LOG_MAX_NUM];

	struct __suspend_log {
		unsigned long long time;
		unsigned long sp;
		char log[DSS_LOG_GEN_LEN];
		void *fn;
#ifdef CONFIG_DEBUG_SNAPSHOT_LINUX_BUILD
		struct device *dev;
#else
		void *dev;
#endif
		int en;
		int state;
		int core;
	} suspend[DSS_LOG_MAX_NUM * 4];

	struct __irq_log {
		unsigned long long time;
		unsigned long sp;
		int irq;
		void *fn;
#ifdef CONFIG_DEBUG_SNAPSHOT_LINUX_BUILD
		struct irq_desc *desc;
#else
		void *desc;
#endif
		unsigned long long latency;
		int en;
	} irq[DSS_NR_CPUS][DSS_LOG_MAX_NUM * 2];
#ifdef CONFIG_DEBUG_SNAPSHOT_SPINLOCK
	struct __spinlock_log {
		unsigned long long time;
		unsigned long sp;
		unsigned long long jiffies;
#ifdef CONFIG_DEBUG_SNAPSHOT_LINUX_BUILD
		raw_spinlock_t *lock;
#ifdef CONFIG_DEBUG_SPINLOCK
		u16 next;
		u16 owner;
#endif
#else
		void *lock;
#ifdef CONFIG_DEBUG_SPINLOCK
		unsigned short next;
		unsigned short owner;
#endif
#endif
		int en;
		void *caller[DSS_CALLSTACK_MAX_NUM];
	} spinlock[DSS_NR_CPUS][DSS_LOG_MAX_NUM];
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_IRQ_DISABLED
	struct __irqs_disabled_log {
		unsigned long long time;
		unsigned long index;
#ifdef CONFIG_DEBUG_SNAPSHOT_LINUX_BUILD
		struct task_struct *task;
#else
		void *task;
#endif
		char *task_comm;
		void *caller[DSS_CALLSTACK_MAX_NUM];
	} irqs_disabled[DSS_NR_CPUS][SZ_32];
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_CLK
	struct __clk_log {
		unsigned long long time;
#ifdef CONFIG_DEBUG_SNAPSHOT_LINUX_BUILD
		struct clk_hw *clk;
#else
		void *clk;
#endif
		const char *f_name;
		int mode;
		unsigned long arg;
	} clk[DSS_LOG_MAX_NUM];
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_PMU
	struct __pmu_log {
		unsigned long long time;
		unsigned int id;
		const char *f_name;
		int mode;
	} pmu[DSS_LOG_MAX_NUM];
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_FREQ
	struct __freq_log {
		unsigned long long time;
		int cpu;
		int freq_type;
		char *freq_name;
		unsigned long old_freq;
		unsigned long target_freq;
		int en;
	} freq[DSS_LOG_MAX_NUM];
	struct __freq_misc_log {
		unsigned long long time;
		int cpu;
		int freq_type;
		char *freq_name;
		unsigned long old_freq;
		unsigned long target_freq;
		int en;
	} freq_misc[DSS_LOG_MAX_NUM];
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_DM
	struct __dm_log {
		unsigned long long time;
		int cpu;
		int dm_num;
		unsigned long min_freq;
		unsigned long max_freq;
#ifdef CONFIG_DEBUG_SNAPSHOT_LINUX_BUILD
		s32 wait_dmt;
		s32 do_dmt;
#else
		int wait_dmt;
		int do_dmt;
#endif
	} dm[DSS_LOG_MAX_NUM];
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_REG
	struct __reg_log {
		unsigned long long time;
		int read;
		size_t val;
		size_t reg;
		int en;
		void *caller[DSS_CALLSTACK_MAX_NUM];
	} reg[DSS_NR_CPUS][DSS_LOG_MAX_NUM];
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_HRTIMER
	struct __hrtimer_log {
		unsigned long long time;
		unsigned long long now;
#ifdef CONFIG_DEBUG_SNAPSHOT_LINUX_BUILD
		struct hrtimer *timer;
#else
		void *timer;
#endif
		void *fn;
		int en;
	} hrtimers[DSS_NR_CPUS][DSS_LOG_MAX_NUM];
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_REGULATOR
	struct __regulator_log {
		unsigned long long time;
		unsigned long long acpm_time;
		int cpu;
		char name[SZ_16];
		unsigned int reg;
		unsigned int voltage;
		unsigned int raw_volt;
		int en;
	} regulator[DSS_LOG_MAX_NUM];
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_THERMAL
	struct __thermal_log {
		unsigned long long time;
		int cpu;
#ifdef CONFIG_DEBUG_SNAPSHOT_LINUX_BUILD
		struct exynos_tmu_platform_data *data;
#else
		void *data;
#endif
		unsigned int temp;
		char *cooling_device;
		unsigned int cooling_state;
	} thermal[DSS_LOG_MAX_NUM];
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_ACPM
	struct __acpm_log {
		unsigned long long time;
		unsigned long long acpm_time;
		char log[9];
		unsigned int data;
	} acpm[DSS_LOG_MAX_NUM];
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_I2C
	struct __i2c_log {
		unsigned long long time;
		int cpu;
#ifdef CONFIG_DEBUG_SNAPSHOT_LINUX_BUILD
		struct i2c_adapter *adap;
		struct i2c_msg *msgs;
#else
		void *adp;
		void *msgs;
#endif
		int num;
		int en;
	} i2c[DSS_LOG_MAX_NUM];
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_SPI
	struct __spi_log {
		unsigned long long time;
		int cpu;
#ifdef CONFIG_DEBUG_SNAPSHOT_LINUX_BUILD
		struct spi_controller *ctlr;
		struct spi_message *cur_msg;
#else
		void *ctlr;
		void *cur_msg;
#endif
		int en;
	} spi[DSS_LOG_MAX_NUM];
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_BINDER
	struct __binder_log {
		unsigned long long time;
		int cpu;
		struct trace_binder_transaction_base base;
		struct trace_binder_transaction transaction;
		struct trace_binder_transaction_error error;
	} binder[DSS_API_MAX_NUM << 2];
#endif

#ifndef CONFIG_DEBUG_SNAPSHOT_MINIMIZED_MODE
	struct __printkl_log {
		unsigned long long time;
		int cpu;
		size_t msg;
		size_t val;
		void *caller[DSS_CALLSTACK_MAX_NUM];
	} printkl[DSS_API_MAX_NUM];

	struct __printk_log {
		unsigned long long time;
		int cpu;
		char log[DSS_LOG_STRING_LEN];
		void *caller[DSS_CALLSTACK_MAX_NUM];
	} printk[DSS_API_MAX_NUM];
#endif
};

#endif
