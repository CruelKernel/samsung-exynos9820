/*
* Samsung debugging features for Samsung's SoC's.
*
* Copyright (c) 2014-2017 Samsung Electronics Co., Ltd.
*      http://www.samsung.com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*/

#ifndef SEC_DEBUG_H
#define SEC_DEBUG_H

#include <linux/sizes.h>
#include <linux/reboot.h>
#include <linux/irq.h>
#include <linux/sec_ext.h>
#include <linux/sec_debug_types.h>

#define SEC_DEBUG_MAGIC_PA memblock_start_of_DRAM()
#define SEC_DEBUG_MAGIC_VA phys_to_virt(SEC_DEBUG_MAGIC_PA)
#define SEC_DEBUG_EXTRA_INFO_VA (SEC_DEBUG_MAGIC_VA + 0x400)
#define SEC_DEBUG_DUMPER_LOG_VA (SEC_DEBUG_MAGIC_VA + 0x800)
#define BUF_SIZE_MARGIN (SZ_1K - 0x80)

/*
 * SEC DEBUG
 */
#define SEC_DEBUG_MAGIC_INFORM		(EXYNOS_PMU_INFORM2)
#define SEC_DEBUG_PANIC_INFORM		(EXYNOS_PMU_INFORM3)

#ifdef CONFIG_SEC_DEBUG
extern void sec_debug_clear_magic_rambase(void);
extern int id_get_asb_ver(void);
extern int id_get_product_line(void);
extern void sec_debug_reboot_handler(void *p);
extern void sec_debug_panic_handler(void *buf, bool dump);
extern void sec_debug_post_panic_handler(void);

extern int sec_debug_get_debug_level(void);
extern int sec_debug_enter_upload(void);
extern void sec_debug_disable_printk_process(void);
extern char *verbose_reg(int cpu_type, int reg_name, unsigned long reg_val);
/* getlog support */
extern void sec_getlog_supply_kernel(void *klog_buf);
extern void sec_getlog_supply_platform(unsigned char *buffer, const char *name);
extern void sec_gaf_supply_rqinfo(unsigned short curr_offset, unsigned short rq_offset);
#else
#define id_get_asb_ver()			(-1)
#define id_get_product_line()			(-1)
#define sec_debug_reboot_handler(a)		do { } while (0)
#define sec_debug_panic_handler(a, b)		do { } while (0)
#define sec_debug_post_panic_handler()		do { } while (0)

#define sec_debug_get_debug_level()		(0)
#define sec_debug_disable_printk_process()	do { } while (0)
#define sec_debug_verbose_reg(a, b, c)		do { } while (0)

#define sec_getlog_supply_kernel(a)		do { } while (0)
#define sec_getlog_supply_platform(a, b)	do { } while (0)

#define sec_gaf_supply_rqinfo(a, b)		do { } while (0)
#endif /* CONFIG_SEC_DEBUG */

enum sec_debug_reset_reason_t {
	RR_S = 1,
	RR_W = 2,
	RR_D = 3,
	RR_K = 4,
	RR_M = 5,
	RR_P = 6,
	RR_R = 7,
	RR_B = 8,
	RR_N = 9,
	RR_T = 10,
	RR_C = 11,
};

extern unsigned reset_reason;

struct sec_debug_ksyms {
	uint32_t magic;
	uint32_t kallsyms_all;
	uint64_t addresses_pa;
	uint64_t names_pa;
	uint64_t num_syms;
	uint64_t token_table_pa;
	uint64_t token_index_pa;
	uint64_t markers_pa;
	struct ksect {
		uint64_t sinittext;
		uint64_t einittext;
		uint64_t stext;
		uint64_t etext;
		uint64_t end;
	} sect;
	uint64_t relative_base;
	uint64_t offsets_pa;
	uint64_t kimage_voffset;
	uint64_t reserved[4];
};

/* sec debug next */
struct basic_type_int {
	uint64_t pa;	/* physical address of the variable */
	uint32_t size;	/* size of basic type. eg sizeof(unsigned long) goes here */
	uint32_t count;	/* for array types */
};

struct sec_debug_kcnst {
	uint64_t nr_cpus;
	struct basic_type_int per_cpu_offset;

	uint64_t phys_offset;
	uint64_t phys_mask;
	uint64_t page_offset;
	uint64_t page_mask;
	uint64_t page_shift;

	uint64_t va_bits;
	uint64_t kimage_vaddr;
	uint64_t kimage_voffset;

	uint64_t pa_swapper;
	uint64_t pgdir_shift;
	uint64_t pud_shift;
	uint64_t pmd_shift;

	uint64_t ptrs_per_pgd;
	uint64_t ptrs_per_pud;
	uint64_t ptrs_per_pmd;
	uint64_t ptrs_per_pte;

	uint64_t kconfig_base;
	uint64_t kconfig_size;

	uint64_t pa_text;
	uint64_t pa_start_rodata;
	uint64_t reserved[4];
};

struct member_type {
	uint16_t size;
	uint16_t offset;
};

typedef struct member_type member_type_int;
typedef struct member_type member_type_long;
typedef struct member_type member_type_longlong;
typedef struct member_type member_type_ptr;
typedef struct member_type member_type_str;

struct struct_thread_info {
	uint32_t struct_size;
	member_type_long flags;
	member_type_ptr task;
	member_type_int cpu;
	member_type_long rrk;
};

struct struct_task_struct {
	uint32_t struct_size;
	member_type_long state;
	member_type_long exit_state;
	member_type_ptr stack;
	member_type_int flags;
	member_type_int on_cpu;
	member_type_int on_rq;
	member_type_int cpu;
	member_type_int pid;
	member_type_str comm;
	member_type_ptr tasks_next;	
	member_type_ptr thread_group_next;
	member_type_long fp;
	member_type_long sp;
	member_type_long pc;
	member_type_long sched_info__pcount;
	member_type_longlong sched_info__run_delay;
	member_type_longlong sched_info__last_arrival;
	member_type_longlong sched_info__last_queued;
	member_type_int ssdbg_wait__type;
	member_type_ptr ssdbg_wait__data;
};

struct irq_stack_info {
	uint64_t pcpu_stack; /* IRQ_STACK_PTR(0) */
	uint64_t size; /* IRQ_STACK_SIZE */
	uint64_t start_sp; /* IRQ_STACK_START_SP */
};

struct sec_debug_task {
	uint64_t stack_size; /* THREAD_SIZE */
	uint64_t start_sp; /* TRHEAD_START_SP */
	struct struct_thread_info ti;
	struct struct_task_struct ts;
	uint64_t init_task;
	struct irq_stack_info irq_stack;
};

struct sec_debug_spinlock_info {
	member_type_int owner_cpu;
	member_type_ptr owner;
	int debug_enabled;
};

#define SD_ESSINFO_KEY_SIZE	(32)
#define SD_NR_ESSINFO_ITEMS	(16)

struct ess_info_offset {
	char key[SD_ESSINFO_KEY_SIZE];
	unsigned long base;
	unsigned long last;
	unsigned int nr;
	unsigned int size;
	unsigned int per_core;
};

struct sec_debug_ess_info {
	struct ess_info_offset item[SD_NR_ESSINFO_ITEMS];
};

struct watchdogd_info {
	struct task_struct *tsk;
	struct thread_info *thr;
	struct rtc_time *tm;

	unsigned long long last_ping_time;
	int last_ping_cpu;
	bool init_done;

	unsigned long emerg_addr;
};

struct bad_stack_info {
	unsigned long magic;
	unsigned long esr;
	unsigned long far;
	unsigned long spel0;
	unsigned long cpu;
	unsigned long tsk_stk;
	unsigned long irq_stk;
	unsigned long ovf_stk;
};

struct suspend_dev_info {
	uint64_t suspend_func;
	uint64_t suspend_device;
	uint64_t shutdown_func;
	uint64_t shutdown_device;
};

struct sec_debug_kernel_data {
	uint64_t task_in_pm_suspend;
	uint64_t task_in_sys_reboot;
	uint64_t task_in_sys_shutdown;
	uint64_t task_in_dev_shutdown;
	uint64_t task_in_sysrq_crash;
	uint64_t task_in_soft_lockup;
	uint64_t cpu_in_soft_lockup;
	uint64_t task_in_hard_lockup;
	uint64_t cpu_in_hard_lockup;
	uint64_t unfrozen_task;
	uint64_t unfrozen_task_count;
	uint64_t sync_irq_task;
	uint64_t sync_irq_num;
	uint64_t sync_irq_name;
	uint64_t sync_irq_desc;
	uint64_t sync_irq_thread;
	uint64_t sync_irq_threads_active;
	uint64_t dev_shutdown_start;
	uint64_t dev_shutdown_end;
	uint64_t dev_shutdown_duration;
	uint64_t dev_shutdown_func;
	uint64_t sysrq_ptr;
	struct watchdogd_info wddinfo;
	struct bad_stack_info bsi;
	struct suspend_dev_info sdi;
};

enum sdn_map {
        SDN_MAP_DUMP_SUMMARY,
        SDN_MAP_AUTO_COMMENT,
        SDN_MAP_EXTRA_INFO,
        SDN_MAP_AUTO_ANALYSIS,
        SDN_MAP_INITTASK_LOG,
        SDN_MAP_DEBUG_PARAM,
	SDN_MAP_FIRST_KMSG,
        SDN_MAP_SPARED_BUFFER,
        NR_SDN_MAP,
};

struct sec_debug_buf {
	unsigned long base;
	unsigned long size;
};

struct outbuf {
	char buf[SZ_1K];
	int index;
	int already;
};

void secdbg_write_buf(struct outbuf *obuf, int len, const char *fmt, ...);

struct sec_debug_map {
	struct sec_debug_buf buf[NR_SDN_MAP];
};

#define SET_MEMBER_TYPE_INFO(PTR, TYPE, MEMBER) \
	{ \
		(PTR)->size = sizeof(((TYPE *)0)->MEMBER); \
		(PTR)->offset = offsetof(TYPE, MEMBER); \
	}

struct sec_debug_memtab {
	uint64_t table_start_pa;
	uint64_t table_end_pa;
	uint64_t reserved[4];
};

#define THREAD_START_SP			(THREAD_SIZE - 16)
#define IRQ_STACK_START_SP		THREAD_START_SP

#define SEC_DEBUG_MAGIC0	(0x11221133)
#define SEC_DEBUG_MAGIC1	(0x12121313)

enum {
	DSS_KEVENT_TASK,
	DSS_KEVENT_WORK,
	DSS_KEVENT_IRQ,
	DSS_KEVENT_FREQ,
	DSS_KEVENT_IDLE,
	DSS_KEVENT_THRM,
	DSS_KEVENT_ACPM,
	DSS_KEVENT_MFRQ,
};

extern unsigned int get_smpl_warn_number(void);
extern void (*mach_restart)(enum reboot_mode mode, const char *cmd);

extern void sec_debug_task_sched_log_short_msg(char *msg);
extern void sec_debug_task_sched_log(int cpu, struct task_struct *task);
extern void sec_debug_irq_sched_log(unsigned int irq, void *fn, int en);
extern void sec_debug_irq_enterexit_log(unsigned int irq, unsigned long long start_time);

extern void sec_debug_set_kallsyms_info(struct sec_debug_ksyms *ksyms, int magic);
extern int sec_debug_check_sj(void);

extern unsigned int sec_debug_get_kevent_paddr(int type);

extern char *get_bk_item_val(const char *key);
extern void get_bk_item_val_as_string(const char *key, char *buf);
extern void sec_debug_get_kevent_info(struct ess_info_offset *p, int type);
extern unsigned long sec_debug_get_kevent_index_addr(int type);

extern void sec_debug_set_task_in_pm_suspend(uint64_t task);
extern void sec_debug_set_task_in_sys_reboot(uint64_t task);
extern void sec_debug_set_task_in_sys_shutdown(uint64_t task);
extern void sec_debug_set_task_in_dev_shutdown(uint64_t task);
extern void sec_debug_set_sysrq_crash(struct task_struct *task);
extern void sec_debug_set_task_in_soft_lockup(uint64_t task);
extern void sec_debug_set_cpu_in_soft_lockup(uint64_t cpu);
extern void sec_debug_set_task_in_hard_lockup(uint64_t task);
extern void sec_debug_set_cpu_in_hard_lockup(uint64_t cpu);
extern void sec_debug_set_unfrozen_task(uint64_t task);
extern void sec_debug_set_unfrozen_task_count(uint64_t count);
extern void sec_debug_set_task_in_sync_irq(uint64_t task, unsigned int irq, const char *name, struct irq_desc *desc);
extern void sec_debug_set_device_shutdown_timeinfo(uint64_t start, uint64_t end, uint64_t duration, uint64_t func);
extern void sec_debug_clr_device_shutdown_timeinfo(void);

extern struct watchdogd_info *sec_debug_get_wdd_info(void);
extern struct bad_stack_info *sec_debug_get_bs_info(void);
extern void *sec_debug_get_debug_base(int type);
extern unsigned long sec_debug_get_buf_base(int type);
extern unsigned long sec_debug_get_buf_size(int type);

extern void sec_set_reboot_magic(int magic, int offset, int mask);

#ifdef CONFIG_SEC_DEBUG
extern void sec_debug_set_shutdown_device(const char *fname, const char *dname);
extern void sec_debug_set_suspend_device(const char *fname, const char *dname);
#else
#define sec_debug_set_shutdown_device(a, b)		do { } while (0)
#define sec_debug_set_suspend_device(a, b)		do { } while (0)
#endif

#ifdef CONFIG_SEC_DEBUG_PPMPU
extern void print_ppmpu_protection(struct pt_regs *regs);
#else
static inline void print_ppmpu_protection(struct pt_regs *regs) {}
#endif

#ifdef CONFIG_SEC_DEBUG_LAST_KMSG
#define SEC_LKMSG_MAGICKEY 0x0000000a6c6c7546
extern void sec_debug_save_last_kmsg(unsigned char *head_ptr,
				unsigned char *curr_ptr, size_t buf_size);
#else
#define sec_debug_save_last_kmsg(a, b, c)		do { } while (0)
#endif


/*
 * SEC DEBUG EXTRA INFO
 */

#define V3_TEST_PHYS_ADDR		(0x91700000)
#define OFFSET_SHARED_BUFFER 		(0xC00)
#define MAX_ITEM_KEY_LEN		(16)
#define MAX_ITEM_VAL_LEN		(1008)

enum shared_buffer_slot {
        SLOT_32,
        SLOT_64,
        SLOT_256,
        SLOT_1024,
        SLOT_MAIN_END = SLOT_1024,
        NR_MAIN_SLOT = 4,
        SLOT_BK_32 = NR_MAIN_SLOT,
        SLOT_BK_64,
        SLOT_BK_256,
        SLOT_BK_1024,
        SLOT_END = SLOT_BK_1024,
        NR_SLOT = 8,
};

struct sec_debug_sb_index {
	unsigned int paddr;		/* physical address of slot */
	unsigned int size;		/* size of a item */
	unsigned int nr;		/* number of items in slot */
	unsigned int cnt;		/* number of used items in slot */

	/* map to indicate which items are added by bootloader */
	unsigned long blmark;
};

struct sec_debug_shared_buffer {
	/* initial magic code */
	unsigned int magic[4];

	/* shared buffer index */
	struct sec_debug_sb_index sec_debug_sbidx[NR_SLOT];
};

#define MAX_EXTRA_INFO_HDR_LEN	6
#define SEC_DEBUG_BADMODE_MAGIC	0x6261646d

enum sec_debug_extra_fault_type {
	UNDEF_FAULT,                /* 0 */
	BAD_MODE_FAULT,             /* 1 */
	WATCHDOG_FAULT,             /* 2 */
	KERNEL_FAULT,               /* 3 */
	MEM_ABORT_FAULT,            /* 4 */
	SP_PC_ABORT_FAULT,          /* 5 */
	PAGE_FAULT,		    /* 6 */
	ACCESS_USER_FAULT,          /* 7 */
	EXE_USER_FAULT,             /* 8 */
	ACCESS_USER_OUTSIDE_FAULT,  /* 9 */
	BUG_FAULT,                  /* 10 */
	PANIC_FAULT,
	FAULT_MAX,
};

#if defined(CONFIG_SAMSUNG_VST_CAL)
extern int volt_vst_cal_bdata;
#endif

#define SEC_DEBUG_SHARED_MAGIC0 0xFFFFFFFF
#define SEC_DEBUG_SHARED_MAGIC1 0x95308180
#define SEC_DEBUG_SHARED_MAGIC2 0x14F014F0
#define SEC_DEBUG_SHARED_MAGIC3 0x00010001

#ifdef CONFIG_SEC_DEBUG_EXTRA_INFO

extern void simulate_extra_info_force_error(unsigned int magic);
extern struct exynos_chipid_info exynos_soc_info;
extern unsigned int get_smpl_warn_number(void);

extern void sec_debug_init_extra_info(struct sec_debug_shared_buffer *);
extern void sec_debug_finish_extra_info(void);
extern void sec_debug_store_extra_info_A(char *ptr);
extern void sec_debug_store_extra_info_B(char *ptr);
extern void sec_debug_store_extra_info_C(char *ptr);
extern void sec_debug_store_extra_info_M(char *ptr);
extern void sec_debug_store_extra_info_F(char *ptr);
extern void sec_debug_set_extra_info_fault(enum sec_debug_extra_fault_type,
						unsigned long addr, struct pt_regs *regs);
extern void sec_debug_set_extra_info_bug(const char *file, unsigned int line);
extern void sec_debug_set_extra_info_panic(char *str);
extern void sec_debug_set_extra_info_backtrace(struct pt_regs *regs);
extern void sec_debug_set_extra_info_backtrace_cpu(struct pt_regs *regs, int cpu);
extern void sec_debug_set_extra_info_backtrace_task(struct task_struct *tsk);
extern void sec_debug_set_extra_info_evt_version(void);
extern void sec_debug_set_extra_info_sysmmu(char *str);
extern void sec_debug_set_extra_info_busmon(char *str);
extern void sec_debug_set_extra_info_dpm_timeout(char *devname);
extern void sec_debug_set_extra_info_smpl(unsigned long count);
extern void sec_debug_set_extra_info_esr(unsigned int esr);
extern void sec_debug_set_extra_info_merr(char *merr);
extern void sec_debug_set_extra_info_hint(u64 hint);
extern void sec_debug_set_extra_info_decon(unsigned int err);
extern void sec_debug_set_extra_info_batt(int cap, int volt, int temp, int curr);
extern void sec_debug_set_extra_info_ufs_error(char *str);
extern void sec_debug_set_extra_info_zswap(char *str);
extern void sec_debug_set_extra_info_mfc_error(char *str);
extern void sec_debug_set_extra_info_aud(char *str);
extern void sec_debug_set_extra_info_unfz(char *tmp);
extern void sec_debug_set_extra_info_epd(char *str);

#else

#define sec_debug_init_extra_info(a)		do { } while (0)
#define sec_debug_finish_extra_info()		do { } while (0)
#define sec_debug_store_extra_info_A(a)		do { } while (0)
#define sec_debug_store_extra_info_C(a)		do { } while (0)
#define sec_debug_store_extra_info_M(a)		do { } while (0)
#define sec_debug_store_extra_info_F(a)		do { } while (0)
#define sec_debug_set_extra_info_fault(a, b, c)	do { } while (0)
#define sec_debug_set_extra_info_bug(a, b)	do { } while (0)
#define sec_debug_set_extra_info_panic(a)	do { } while (0)
#define sec_debug_set_extra_info_backtrace(a)	do { } while (0)
#define sec_debug_set_extra_info_backtrace_cpu(a, b)	do { } while (0)
#define sec_debug_set_extra_info_evt_version()	do { } while (0)
#define sec_debug_set_extra_info_sysmmu(a)	do { } while (0)
#define sec_debug_set_extra_info_busmon(a)	do { } while (0)
#define sec_debug_set_extra_info_dpm_timeout(a)	do { } while (0)
#define sec_debug_set_extra_info_smpl(a)	do { } while (0)
#define sec_debug_set_extra_info_esr(a)		do { } while (0)
#define sec_debug_set_extra_info_merr(a)	do { } while (0)
#define sec_debug_set_extra_info_hint(a)	do { } while (0)
#define sec_debug_set_extra_info_decon(a)	do { } while (0)
#define sec_debug_set_extra_info_batt(a, b, c, d)	do { } while (0)
#define sec_debug_set_extra_info_ufs_error(a)	do { } while (0)
#define sec_debug_set_extra_info_zswap(a)	do { } while (0)
#define sec_debug_set_extra_info_mfc_error(a)	do { } while (0)
#define sec_debug_set_extra_info_aud(a)		do { } while (0)
#define sec_debug_set_extra_info_unfz(a)	do { } while (0)
#define sec_debug_set_extra_info_epd(a)		do { } while (0)

#endif /* CONFIG_SEC_DEBUG_EXTRA_INFO */

/*
 * SEC DEBUG AUTO COMMENT
 */

#ifdef CONFIG_SEC_DEBUG_AUTO_COMMENT
#define AC_SIZE 0xf3c
#define AC_MAGIC 0xcafecafe
#define AC_TAIL_MAGIC 0x00c0ffee
#define AC_EDATA_MAGIC 0x43218765

enum {
	PRIO_LV0 = 0,
	PRIO_LV1,
	PRIO_LV2,
	PRIO_LV3,
	PRIO_LV4,
	PRIO_LV5,
	PRIO_LV6,
	PRIO_LV7,
	PRIO_LV8,
	PRIO_LV9
};

#define SEC_DEBUG_AUTO_COMM_BUF_SIZE 10

enum sec_debug_FREQ_INFO {
	FREQ_INFO_CLD0 = 0,
	FREQ_INFO_CLD1,
	FREQ_INFO_INT,
	FREQ_INFO_MIF,
	FREQ_INFO_MAX,
};

struct sec_debug_auto_comm_buf {
	int reserved_0;
	int reserved_1;
	int reserved_2;
	unsigned int offset;
	char buf[SZ_4K];
};

struct sec_debug_auto_comm_log_idx {
	atomic_t logging_entry;
	atomic_t logging_disable;
	atomic_t logging_count;
};

struct sec_debug_auto_comment {
	int header_magic;
	int fault_flag;
	int lv5_log_cnt;
	u64 lv5_log_order;
	int order_map_cnt;
	int order_map[SEC_DEBUG_AUTO_COMM_BUF_SIZE];
	struct sec_debug_auto_comm_buf auto_comm_buf[SEC_DEBUG_AUTO_COMM_BUF_SIZE];

	int tail_magic;
};

struct auto_comment_log_map {
	char prio_level;
	char max_count;
};

static const struct auto_comment_log_map init_data[SEC_DEBUG_AUTO_COMM_BUF_SIZE] = {
	{PRIO_LV0, 0},
	{PRIO_LV5, 8},
	{PRIO_LV9, 0},
	{PRIO_LV5, 0},
	{PRIO_LV5, 0},
	{PRIO_LV1, 7},
	{PRIO_LV2, 8},
	{PRIO_LV5, 0},
	{PRIO_LV5, 8},
	{PRIO_LV0, 0}
};

extern void sec_debug_auto_comment_log_disable(int type);
extern void sec_debug_auto_comment_log_once(int type);
extern void register_set_auto_comm_buf(void (*func)(int type, const char *buf, size_t size));
extern void register_set_auto_comm_lastfreq(void (*func)(int type,
						int old_freq, int new_freq, u64 time, int en));
#endif

#ifdef CONFIG_SEC_DEBUG_DTASK
extern void sec_debug_wtsk_print_info(struct task_struct *task, bool raw);
extern void sec_debug_wtsk_set_data(int type, void *data);
static inline void sec_debug_wtsk_clear_data(void)
{
	sec_debug_wtsk_set_data(DTYPE_NONE, NULL);
}
#else
#define sec_debug_wtsk_print_info(a, b)		do { } while (0)
#define sec_debug_wtsk_set_data(a, b)		do { } while (0)
#define sec_debug_wtsk_clear_data()		do { } while (0)
#endif

#ifdef CONFIG_SEC_DEBUG_INIT_LOG
extern void register_init_log_hook_func(void (*func)(const char *buf, size_t size));
#endif

/* increase if sec_debug_next is not changed and other feature is upgraded */
#define SEC_DEBUG_KERNEL_UPPER_VERSION		(0x0001)
/* increase if sec_debug_next is changed */
#define SEC_DEBUG_KERNEL_LOWER_VERSION		(0x0001)

/* SEC DEBUG NEXT DEFINITION */
struct sec_debug_next {
	unsigned int magic[2];
	unsigned int version[2];

	struct sec_debug_map map;
	struct sec_debug_memtab memtab;
	struct sec_debug_ksyms ksyms;
	struct sec_debug_kcnst kcnst;
	struct sec_debug_task task;
	struct sec_debug_ess_info ss_info;
	struct sec_debug_spinlock_info rlock;
	struct sec_debug_kernel_data kernd;

	struct sec_debug_auto_comment auto_comment;
	struct sec_debug_shared_buffer extra_info;
};

/*
 * Samsung TN Logging Options
 */
/**
 * sec_debug_tsp_log : Leave tsp log in tsp_msg file.
 * ( Timestamp + Tsp logs )
 * sec_debug_tsp_log_msg : Leave tsp log in tsp_msg file and
 * add additional message between timestamp and tsp log.
 * ( Timestamp + additional Message + Tsp logs )
 */
#ifdef CONFIG_SEC_DEBUG_TSP_LOG
extern void sec_debug_tsp_log(char *fmt, ...);
extern void sec_debug_tsp_log_msg(char *msg, char *fmt, ...);
extern void sec_debug_tsp_raw_data(char *fmt, ...);
extern void sec_debug_tsp_raw_data_msg(char *msg, char *fmt, ...);
extern void sec_tsp_raw_data_clear(void);
extern void sec_debug_tsp_command_history(char *buf);
#else
#define sec_debug_tsp_log(a, ...)		do { } while (0)
#define sec_debug_tsp_log_msg(a, b, ...)		do { } while (0)
#define sec_debug_tsp_raw_data(a, ...)			do { } while (0)
#define sec_debug_tsp_raw_data_msg(a, b, ...)		do { } while (0)
#define sec_tsp_raw_data_clear()			do { } while (0)
#define sec_debug_tsp_command_history()			do { } while (0)
#endif /* CONFIG_SEC_DEBUG_TSP_LOG */

#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
struct tsp_dump_callbacks {
	void (*inform_dump)(void);
};
#endif

#ifdef CONFIG_SEC_DEBUG_LIMIT_BACKTRACE
#define MAX_UNWINDING_LOOP 50 /* maximum number of unwind frame */
#endif

#ifdef CONFIG_SEC_DEBUG_SYSRQ_KMSG
extern size_t sec_debug_get_curr_init_ptr(void);
extern size_t dbg_snapshot_get_curr_ptr_for_sysrq(void);
#endif

/* sec_debug_memtab.c */
extern void secdbg_base_set_memtab_info(struct sec_debug_memtab *ksyms);

#ifdef CONFIG_SEC_DEBUG
#define SDBG_KNAME_LEN	64
struct secdbg_member_type {
	char member[SDBG_KNAME_LEN];
	uint16_t size;
	uint16_t offset;
	uint16_t unused[2];
};
#define SECDBG_DEFINE_MEMBER_TYPE(key, st, mem)					\
	const struct secdbg_member_type sdbg_##key				\
		__attribute__((__section__(".secdbg_mbtab." #key))) = {		\
		.member = #key,							\
		.size = FIELD_SIZEOF(struct st, mem),				\
		.offset = offsetof(struct st, mem),				\
	}
#else
#define SECDBG_DEFINE_MEMBER_TYPE(a, b, c)
#endif /* CONFIG_SEC_DEBUG */

#endif /* SEC_DEBUG_H */

