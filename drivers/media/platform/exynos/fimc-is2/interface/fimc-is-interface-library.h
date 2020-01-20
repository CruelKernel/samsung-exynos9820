/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_API_COMMON_H
#define FIMC_IS_API_COMMON_H

#include <linux/vmalloc.h>

#include "fimc-is-core.h"
#include "fimc-is-debug.h"
#include "fimc-is-regs.h"
#include "fimc-is-binary.h"
#include "fimc-is-param.h"


#define SET_CPU_AFFINITY	/* enable @ Exynos3475 */

#if !defined(DISABLE_LIB)
/* #define LIB_MEM_TRACK */
#endif

#if defined(CONFIG_STACKTRACE) && defined(LIB_MEM_TRACK)
#define LIB_MEM_TRACK_CALLSTACK
#endif

#define LIB_ISP_OFFSET		(0x00000080)
#define LIB_ISP_CODE_SIZE	(0x00340000)

#define LIB_VRA_OFFSET		(0x00000400)
#define LIB_VRA_CODE_SIZE	(0x00080000)

#define LIB_RTA_OFFSET		(0x00000000)
#define LIB_RTA_CODE_SIZE	(0x00200000)

#define LIB_MAX_TASK		(FIMC_IS_MAX_TASK)

#define TASK_TYPE_DDK	(1)
#define TASK_TYPE_RTA	(2)

/* depends on FIMC-IS version */
enum task_priority_magic {
	TASK_PRIORITY_BASE = 10,
	TASK_PRIORITY_1ST,	/* highest */
	TASK_PRIORITY_2ND,
	TASK_PRIORITY_3RD,
	TASK_PRIORITY_4TH,
	TASK_PRIORITY_5TH,	/* lowest */
	TASK_PRIORITY_6TH	/* for RTA */
};

/* Task index */
enum task_index {
	TASK_OTF = 0,
	TASK_AF,
	TASK_ISP_DMA,
	TASK_3AA_DMA,
	TASK_AA,
	TASK_RTA,
	TASK_INDEX_MAX
};
#define TASK_VRA		(TASK_INDEX_MAX)

/* Task affinity */
#define TASK_OTF_AFFINITY		(3)
#define TASK_AF_AFFINITY		(1)
#define TASK_ISP_DMA_AFFINITY		(2)
#define TASK_3AA_DMA_AFFINITY		(TASK_ISP_DMA_AFFINITY)
#define TASK_AA_AFFINITY		(TASK_AF_AFFINITY)
/* #define TASK_RTA_AFFINITY		(1) */ /* There is no need to set of cpu affinity for RTA task */
#define TASK_VRA_AFFINITY		(2)

#define CAMERA_BINARY_VRA_DATA_OFFSET   0x080000
#define CAMERA_BINARY_VRA_DATA_SIZE     0x040000
#ifdef USE_ONE_BINARY
#define CAMERA_BINARY_DDK_CODE_OFFSET   VRA_LIB_SIZE
#define CAMERA_BINARY_DDK_DATA_OFFSET   (VRA_LIB_SIZE + LIB_ISP_CODE_SIZE)
#else
#define CAMERA_BINARY_DDK_CODE_OFFSET   0x000000
#define CAMERA_BINARY_DDK_DATA_OFFSET   LIB_ISP_CODE_SIZE
#endif
#define CAMERA_BINARY_RTA_DATA_OFFSET   LIB_RTA_CODE_SIZE

enum BinLoadType{
    BINARY_LOAD_ALL,
    BINARY_LOAD_DATA
};

#define BINARY_LOAD_DDK_DONE		0x01
#define BINARY_LOAD_RTA_DONE		0x02
#define BINARY_LOAD_ALL_DONE	(BINARY_LOAD_DDK_DONE | BINARY_LOAD_RTA_DONE)

typedef u32 (*task_func)(void *params);

typedef u32 (*start_up_func_t)(void **func);
typedef u32 (*rta_start_up_func_t)(void *bootargs, void **func);
typedef void(*os_system_func_t)(void);

#define DDK_SHUT_DOWN_FUNC_ADDR	(DDK_LIB_ADDR + 0x100)
typedef int (*ddk_shut_down_func_t)(void *data);
#define RTA_SHUT_DOWN_FUNC_ADDR	(RTA_LIB_ADDR + 0x100)
typedef int (*rta_shut_down_func_t)(void *data);

struct fimc_is_task_work {
	struct kthread_work		work;
	task_func			func;
	void				*params;
};

struct fimc_is_lib_task {
	struct kthread_worker		worker;
	struct task_struct		*task;
	spinlock_t			work_lock;
	u32				work_index;
	struct fimc_is_task_work	work[LIB_MAX_TASK];
};

enum memory_track_type {
	/* each O/S facility */
	MT_TYPE_SEMA	= 0x01,
	MT_TYPE_MUTEX,
	MT_TYPE_TIMER,
	MT_TYPE_SPINLOCK,

	/* memory block */
	MT_TYPE_MB_HEAP	= 0x10,
	MT_TYPE_MB_DMA_TAAISP,
	MT_TYPE_MB_DMA_MEDRC,
	MT_TYPE_MB_DMA_TNR,

	MT_TYPE_MB_VRA	= 0x20,

	/* etc */
	MT_TYPE_DMA	= 0x30,
};

#ifdef LIB_MEM_TRACK
#define MT_STATUS_ALLOC	0x1
#define MT_STATUS_FREE	0x2

#define MEM_TRACK_COUNT	5000
#define MEM_TRACK_ADDRS_COUNT 16

struct _lib_mem_track {
	ulong			lr;
#ifdef LIB_MEM_TRACK_CALLSTACK
	ulong			addrs[MEM_TRACK_ADDRS_COUNT];
#endif
	int			cpu;
	int			pid;
	unsigned long long	when;
};

struct lib_mem_track {
	int			type;
	ulong			addr;
	size_t			size;
	int			status;
	struct _lib_mem_track	alloc;
	struct _lib_mem_track	free;
};

struct lib_mem_tracks {
	struct list_head	list;
	int			num_of_track;
	struct lib_mem_track	track[MEM_TRACK_COUNT];
};
#endif

#define MEM_BLOCK_NAME_LEN	16
struct lib_mem_block {
	spinlock_t		lock;
	struct list_head	list;
	u32			used;	/* size was allocated totally */
	u32			end;	/* last allocation position */

	size_t			align;
	ulong			kva_base;
	dma_addr_t		dva_base;

	struct fimc_is_priv_buf *pb;

	char			name[MEM_BLOCK_NAME_LEN];
	u32			type;
};

struct lib_buf {
	u32			size;
	ulong			kva;
	dma_addr_t		dva;
	struct list_head	list;
	void			*priv;
};

struct general_intr_handler {
	int irq;
	int id;
	int (*intr_func)(void);
};

enum general_intr_id {
	ID_GENERAL_INTR_PREPROC_PDAF = 0,
	ID_GENERAL_INTR_PDP0_STAT,
	ID_GENERAL_INTR_PDP1_STAT,
	ID_GENERAL_INTR_MAX
};

#define SIZE_LIB_LOG_PREFIX	18	/* [%5lu.%06lu] [%d]  */
#define SIZE_LIB_LOG_BUF	256
#define MAX_LIB_LOG_BUF		(SIZE_LIB_LOG_PREFIX + SIZE_LIB_LOG_BUF)
struct fimc_is_lib_support {
	struct fimc_is_minfo			*minfo;

	struct fimc_is_interface_ischain	*itfc;
	struct hwip_intr_handler		*intr_handler_taaisp[ID_3AAISP_MAX][INTR_HWIP_MAX];
	struct fimc_is_lib_task			task_taaisp[TASK_INDEX_MAX];

	bool					binary_load_flg;
	int					binary_load_fatal;
	int					binary_code_load_flg;

	/* memory blocks */
	struct lib_mem_block			mb_heap_rta;
	struct lib_mem_block			mb_dma_taaisp;
	struct lib_mem_block			mb_dma_medrc;
	struct lib_mem_block			mb_dma_tnr;
	struct lib_mem_block			mb_vra;
	/* non-memory block */
	spinlock_t				slock_nmb;
	struct list_head			list_of_nmb;

	/* for log */
	spinlock_t				slock_debug;
	ulong					kvaddr_debug;
	ulong					kvaddr_debug_cnt;
	ulong					log_ptr;
	char					log_buf[MAX_LIB_LOG_BUF];
	char					string[MAX_LIB_LOG_BUF];

	/* for event */
	ulong					event_ptr;
	char					event_buf[256];
	char					event_string[256];

#ifdef ENABLE_DBG_EVENT
	char					debugmsg[SIZE_LIB_LOG_BUG];
#endif
	/* for library load */
	struct platform_device			*pdev;
#ifdef LIB_MEM_TRACK
	spinlock_t				slock_mem_track;
	struct list_head			list_of_tracks;
	struct lib_mem_tracks			*cur_tracks;
#endif
	struct general_intr_handler		intr_handler[ID_GENERAL_INTR_MAX];
	char		fw_name[50];		/* DDK */
	char		rta_fw_name[50];	/* RTA */
};

struct fimc_is_memory_attribute {
	pgprot_t state;
	int numpages;
	ulong vaddr;
};

struct fimc_is_memory_change_data {
	pgprot_t set_mask;
	pgprot_t clear_mask;
};

/* Fast FDAE & FDAF */
 struct fd_point32 {
	int x;
	int y;
};

struct fd_rectangle {
	u32 width;
	u32 height;
};

struct fd_areabyrect {
	struct fd_point32 top_left;
	struct fd_rectangle span;
};

struct fd_faceinfo {
	struct fd_areabyrect face_area;
	int faceId;
	int score;
	u32 rotation;
};

struct fd_info {
	struct fd_faceinfo face[MAX_FACE_COUNT];
	int face_num;
	u32 frame_count;
};

int fimc_is_load_bin(void);
void fimc_is_load_clear(void);
int fimc_is_init_ddk_thread(void);
void fimc_is_flush_ddk_thread(void);
void check_lib_memory_leak(void);

int fimc_is_log_write(const char *str, ...);
int fimc_is_log_write_console(char *str);
int fimc_is_lib_logdump(void);

int fimc_is_spin_lock_init(void **slock);
int fimc_is_spin_lock_finish(void *slock_lib);
int fimc_is_spin_lock(void *slock_lib);
int fimc_is_spin_unlock(void *slock_lib);
int fimc_is_spin_lock_irq(void *slock_lib);
int fimc_is_spin_unlock_irq(void *slock_lib);
ulong fimc_is_spin_lock_irqsave(void *slock_lib);
int fimc_is_spin_unlock_irqrestore(void *slock_lib, ulong flag);

void *fimc_is_alloc_vra(u32 size);
void fimc_is_free_vra(void *buf);
int fimc_is_dva_vra(ulong kva, u32 *dva);
void fimc_is_inv_vra(ulong kva, u32 size);
void fimc_is_clean_vra(ulong kva, u32 size);

bool fimc_is_lib_in_irq(void);

int fimc_is_load_bin_on_boot(void);
void fimc_is_load_ctrl_unlock(void);
void fimc_is_load_ctrl_lock(void);
void fimc_is_load_ctrl_init(void);
int fimc_is_set_fw_names(char *fw_name, char *rta_fw_name);
#endif
