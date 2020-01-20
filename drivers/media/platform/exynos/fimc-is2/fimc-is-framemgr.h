/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_FRAME_MGR_H
#define FIMC_IS_FRAME_MGR_H

#include <linux/kthread.h>
#include <linux/videodev2.h>
#include "fimc-is-time.h"
#include "fimc-is-config.h"

#define FIMC_IS_MAX_BUFS	VIDEO_MAX_FRAME
#define FIMC_IS_MAX_PLANES	17

#define FRAMEMGR_ID_INVALID	0x00000000
#define FRAMEMGR_ID_SSX		0x00000100
#define FRAMEMGR_ID_SSXVC0	0x00000011
#define FRAMEMGR_ID_SSXVC1	0x00000012
#define FRAMEMGR_ID_SSXVC2	0x00000013
#define FRAMEMGR_ID_SSXVC3	0x00000014
#define FRAMEMGR_ID_3XS		0x00000200
#define FRAMEMGR_ID_3XC		0x00000400
#define FRAMEMGR_ID_3XP		0x00000800
#define FRAMEMGR_ID_3XF		0x00000801
#define FRAMEMGR_ID_3XG		0x00000802
#define FRAMEMGR_ID_IXS		0x00001000
#define FRAMEMGR_ID_IXC		0x00002000
#define FRAMEMGR_ID_IXP		0x00004000
#define FRAMEMGR_ID_DXS		0x00008000
#define FRAMEMGR_ID_DXC		0x00010000
#define FRAMEMGR_ID_MXS		0x00020000
#define FRAMEMGR_ID_M0P		0x00040000
#define FRAMEMGR_ID_M1P		0x00080000
#define FRAMEMGR_ID_M2P		0x00100000
#define FRAMEMGR_ID_M3P		0x00200000
#define FRAMEMGR_ID_M4P		0x00400000
#define FRAMEMGR_ID_M5P		0x00800000
#define FRAMEMGR_ID_VRA		0x01000000
#if defined(SOC_DCP)
#define FRAMEMGR_ID_DCP0S	0x02000000
#define FRAMEMGR_ID_DCP1S	0x04000000
#define FRAMEMGR_ID_DCP0C	0x08000000
#define FRAMEMGR_ID_DCP1C	0x10000000
#define FRAMEMGR_ID_DCP2C	0x20000000
#define FRAMEMGR_ID_MEXC	0x40000000	/* for ME */
#define FRAMEMGR_ID_HW		0x80000000
#define FRAMEMGR_ID_SHOT	(FRAMEMGR_ID_SSX | FRAMEMGR_ID_3XS | \
				 FRAMEMGR_ID_IXS | FRAMEMGR_ID_DXS | \
				 FRAMEMGR_ID_MXS | FRAMEMGR_ID_VRA | \
				 FRAMEMGR_ID_DCP0S)
#define FRAMEMGR_ID_STREAM	(FRAMEMGR_ID_3XC | FRAMEMGR_ID_3XP | \
				 FRAMEMGR_ID_DXS | FRAMEMGR_ID_DXC | \
				 FRAMEMGR_ID_M0P | FRAMEMGR_ID_M1P | \
				 FRAMEMGR_ID_M2P | FRAMEMGR_ID_M3P | \
				 FRAMEMGR_ID_M4P | FRAMEMGR_ID_M5P | \
				 FRAMEMGR_ID_DCP1S | FRAMEMGR_ID_DCP0C | \
				 FRAMEMGR_ID_DCP1C | FRAMEMGR_ID_DCP2C | \
				 FRAMEMGR_ID_SSXVC0 | FRAMEMGR_ID_SSXVC1 | \
				 FRAMEMGR_ID_SSXVC2 | FRAMEMGR_ID_SSXVC3)
#else
#define FRAMEMGR_ID_MEXC	0x02000000	/* for ME */
#define FRAMEMGR_ID_PAFXS	0x04000000	/* for PAF_RDMA */
#define FRAMEMGR_ID_HW		0x08000000
#define FRAMEMGR_ID_SHOT	(FRAMEMGR_ID_SSX | FRAMEMGR_ID_3XS | \
				 FRAMEMGR_ID_IXS | FRAMEMGR_ID_DXS | \
				 FRAMEMGR_ID_MXS | FRAMEMGR_ID_VRA | \
				 FRAMEMGR_ID_PAFXS)
#define FRAMEMGR_ID_STREAM	(FRAMEMGR_ID_3XC | FRAMEMGR_ID_3XP | \
				 FRAMEMGR_ID_DXS | FRAMEMGR_ID_DXC | \
				 FRAMEMGR_ID_M0P | FRAMEMGR_ID_M1P | \
				 FRAMEMGR_ID_M2P | FRAMEMGR_ID_M3P | \
				 FRAMEMGR_ID_M4P | FRAMEMGR_ID_M5P | \
				 FRAMEMGR_ID_SSXVC0 | FRAMEMGR_ID_SSXVC1 | \
				 FRAMEMGR_ID_SSXVC2 | FRAMEMGR_ID_SSXVC3)
#endif
/* #define TRACE_FRAME */
#define TRACE_ID		(FRAMEMGR_ID_SHOT | FRAMEMGR_ID_STREAM | FRAMEMGR_ID_HW)

#define FMGR_IDX_0		(1 << 0 )
#define FMGR_IDX_1		(1 << 1 )
#define FMGR_IDX_2		(1 << 2 )
#define FMGR_IDX_3		(1 << 3 )
#define FMGR_IDX_4		(1 << 4 )
#define FMGR_IDX_5		(1 << 5 )
#define FMGR_IDX_6		(1 << 6 )
#define FMGR_IDX_7		(1 << 7 )
#define FMGR_IDX_8		(1 << 8 )
#define FMGR_IDX_9		(1 << 9 )
#define FMGR_IDX_10		(1 << 10)
#define FMGR_IDX_11		(1 << 11)
#define FMGR_IDX_12		(1 << 12)
#define FMGR_IDX_13		(1 << 13)
#define FMGR_IDX_14		(1 << 14)
#define FMGR_IDX_15		(1 << 15)
#define FMGR_IDX_16		(1 << 16)
#define FMGR_IDX_17		(1 << 17)
#define FMGR_IDX_18		(1 << 18)
#define FMGR_IDX_19		(1 << 19)
#define FMGR_IDX_20		(1 << 20)
#define FMGR_IDX_21		(1 << 21)
#define FMGR_IDX_22		(1 << 22)
#define FMGR_IDX_23		(1 << 23)
#define FMGR_IDX_24		(1 << 24)
#define FMGR_IDX_25		(1 << 25)
#define FMGR_IDX_26		(1 << 26)
#define FMGR_IDX_27		(1 << 27)
#define FMGR_IDX_28		(1 << 28)
#define FMGR_IDX_29		(1 << 29)
#define FMGR_IDX_30		(1 << 30)
#define FMGR_IDX_31		(1 << 31)

#define framemgr_e_barrier_irqs(this, index, flag)		\
	do {							\
		this->sindex |= index;				\
		spin_lock_irqsave(&this->slock, flag);		\
	} while (0)
#define framemgr_x_barrier_irqr(this, index, flag)		\
	do {							\
		spin_unlock_irqrestore(&this->slock, flag);	\
		this->sindex &= ~index;				\
	} while (0)
#define framemgr_e_barrier_irq(this, index)			\
	do {							\
		this->sindex |= index;				\
		spin_lock_irq(&this->slock);			\
	} while (0)
#define framemgr_x_barrier_irq(this, index)			\
	do {							\
		spin_unlock_irq(&this->slock);			\
		this->sindex &= ~index;				\
	} while (0)
#define framemgr_e_barrier(this, index)				\
	do {							\
		this->sindex |= index;				\
		spin_lock(&this->slock);			\
	} while (0)
#define framemgr_x_barrier(this, index)				\
	do {							\
		spin_unlock(&this->slock);			\
		this->sindex &= ~index;				\
	} while (0)

enum fimc_is_frame_state {
	FS_FREE,
	FS_REQUEST,
	FS_PROCESS,
	FS_COMPLETE,
	FS_INVALID
};

#define FS_HW_FREE	FS_FREE
#define FS_HW_REQUEST	FS_REQUEST
#define FS_HW_CONFIGURE	FS_PROCESS
#define FS_HW_WAIT_DONE	FS_COMPLETE
#define FS_HW_INVALID	FS_INVALID

#define NR_FRAME_STATE FS_INVALID

enum fimc_is_frame_mem_state {
	/* initialized memory */
	FRAME_MEM_INIT,
	/* mapped memory */
	FRAME_MEM_MAPPED
};

#ifndef ENABLE_IS_CORE
#define MAX_FRAME_INFO		(4)
enum fimc_is_frame_info_index {
	INFO_FRAME_START,
	INFO_CONFIG_LOCK,
	INFO_FRAME_END_PROC
};

struct fimc_is_frame_info {
	int			cpu;
	int			pid;
	unsigned long long	when;
};
#endif

struct fimc_is_frame {
	struct list_head	list;
	struct kthread_work	work;
	void			*groupmgr;
	void			*group;
	void			*subdev; /* fimc_is_subdev */

	/* group leader use */
	struct camera2_shot_ext	*shot_ext;
	struct camera2_shot	*shot;
	size_t			shot_size;

	/* stream use */
	struct camera2_stream	*stream;

	/* common use */
	u32			planes; /* total planes include multi-buffers */
	dma_addr_t		dvaddr_buffer[FIMC_IS_MAX_PLANES];
	ulong			kvaddr_buffer[FIMC_IS_MAX_PLANES];

	/*
	 * target address for capture node
	 * [0] invalid address, stop
	 * [others] valid address
	 */
	u32 sourceAddress[FIMC_IS_MAX_PLANES]; /* DC1S: DCP slave input DMA */
	u32 txcTargetAddress[FIMC_IS_MAX_PLANES]; /* 3AA capture DMA */
	u32 txpTargetAddress[FIMC_IS_MAX_PLANES]; /* 3AA preview DMA */
	u32 mrgTargetAddress[FIMC_IS_MAX_PLANES];
	u32 efdTargetAddress[FIMC_IS_MAX_PLANES];
	u32 ixcTargetAddress[FIMC_IS_MAX_PLANES];
	u32 ixpTargetAddress[FIMC_IS_MAX_PLANES];
	u64 mexcTargetAddress[FIMC_IS_MAX_PLANES]; /* ME out DMA */
	u32 sccTargetAddress[FIMC_IS_MAX_PLANES]; /* DC0S: DCP master capture DMA */
	u32 scpTargetAddress[FIMC_IS_MAX_PLANES]; /* DC1S: DCP slave capture DMA */
	u32 sc0TargetAddress[FIMC_IS_MAX_PLANES];
	u32 sc1TargetAddress[FIMC_IS_MAX_PLANES];
	u32 sc2TargetAddress[FIMC_IS_MAX_PLANES];
	u32 sc3TargetAddress[FIMC_IS_MAX_PLANES];
	u32 sc4TargetAddress[FIMC_IS_MAX_PLANES];
	u32 sc5TargetAddress[FIMC_IS_MAX_PLANES];
	u32 dxcTargetAddress[FIMC_IS_MAX_PLANES]; /* DC2S: DCP disparity capture DMA */

	/* multi-buffer use */
	u32			num_buffers; /* total number of buffers per frame */
	u32			cur_buf_index; /* current processed buffer index */

	/* internal use */
	unsigned long		mem_state;
	u32			state;
	u32			fcount;
	u32			rcount;
	u32			index;
	u32			lindex;
	u32			hindex;
	u32			result;
	unsigned long		out_flag;
	unsigned long		bak_flag;

#ifndef ENABLE_IS_CORE
	struct fimc_is_frame_info frame_info[MAX_FRAME_INFO];
	u32			instance; /* device instance */
	u32			type;
	unsigned long		core_flag;
	atomic_t		shot_done_flag;
#else
	/* group leader use */
	dma_addr_t		dvaddr_shot;
#endif

#ifdef ENABLE_SYNC_REPROCESSING
	struct list_head	sync_list;
#endif
	struct list_head	votf_list;

	/* for NI(noise index from DDK) use */
	u32			noise_idx; /* Noise index */

#ifdef MEASURE_TIME
	/* time measure externally */
	struct timeval	*tzone;
	/* time measure internally */
	struct fimc_is_monitor	mpoint[TMS_END];
#endif

#ifdef DBG_DRAW_DIGIT
	u32			width;
	u32			height;
#endif
};

struct fimc_is_framemgr {
	u32			id;
	char			name[FIMC_IS_STR_LEN];
	spinlock_t		slock;
	ulong			sindex;

	u32			num_frames;
	struct fimc_is_frame	*frames;

	u32			queued_count[NR_FRAME_STATE];
	struct list_head	queued_list[NR_FRAME_STATE];
};

static const char * const hw_frame_state_name[NR_FRAME_STATE] = {
	"Free",
	"Request",
	"Configure",
	"Wait_Done"
};

static const char * const frame_state_name[NR_FRAME_STATE] = {
	"Free",
	"Request",
	"Process",
	"Complete"
};

ulong frame_fcount(struct fimc_is_frame *frame, void *data);
int put_frame(struct fimc_is_framemgr *this, struct fimc_is_frame *frame,
			enum fimc_is_frame_state state);
struct fimc_is_frame *get_frame(struct fimc_is_framemgr *this,
			enum fimc_is_frame_state state);
int trans_frame(struct fimc_is_framemgr *this, struct fimc_is_frame *frame,
			enum fimc_is_frame_state state);
struct fimc_is_frame *peek_frame(struct fimc_is_framemgr *this,
			enum fimc_is_frame_state state);
struct fimc_is_frame *peek_frame_tail(struct fimc_is_framemgr *this,
			enum fimc_is_frame_state state);
struct fimc_is_frame *find_frame(struct fimc_is_framemgr *this,
			enum fimc_is_frame_state state,
			ulong (*fn)(struct fimc_is_frame *, void *), void *data);
void print_frame_queue(struct fimc_is_framemgr *this,
			enum fimc_is_frame_state state);

int frame_manager_probe(struct fimc_is_framemgr *this, u32 id, const char *name);
int frame_manager_open(struct fimc_is_framemgr *this, u32 buffers);
int frame_manager_close(struct fimc_is_framemgr *this);
int frame_manager_flush(struct fimc_is_framemgr *this);
void frame_manager_print_queues(struct fimc_is_framemgr *this);
void frame_manager_print_info_queues(struct fimc_is_framemgr *this);

#endif
