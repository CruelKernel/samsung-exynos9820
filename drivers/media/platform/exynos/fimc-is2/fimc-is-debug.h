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

#ifndef FIMC_IS_DEBUG_H
#define FIMC_IS_DEBUG_H

#include "fimc-is-video.h"

#define DEBUG_SENTENCE_MAX		300
#define LOG_INTERVAL_OF_WARN		30

#ifdef DBG_DRAW_DIGIT
#define DBG_DIGIT_CNT		20	/* Max count of total digit */
#define DBG_DIGIT_W		16
#define DBG_DIGIT_H		32
#define DBG_DIGIT_MARGIN_W_INIT	128
#define DBG_DIGIT_MARGIN_H_INIT	64
#define DBG_DIGIT_MARGIN_W	8
#define DBG_DIGIT_MARGIN_H	2
#define DBG_DIGIT_TAG(row, col, queue, frame, digit, increase_unit)	\
	do {			\
		int i, j;   \
		for (i = 0, j = digit; i < frame->num_buffers; i++, j += increase_unit) {   \
			ulong addr;	\
			u32 width, height, pixelformat, bitwidth;		\
			addr = queue->buf_kva[frame->index][i];			\
			width = (frame->width) ? frame->width : queue->framecfg.width;	\
			height = (frame->height) ? frame->height : queue->framecfg.height;	\
			pixelformat = queue->framecfg.format->pixelformat;	\
			bitwidth = queue->framecfg.format->hw_bitwidth;		\
			fimc_is_draw_digit(addr, width, height, pixelformat, bitwidth,	\
					row, col, j);			\
		}   \
	} while(0)
#else
#define DBG_DIGIT_TAG(row, col, queue, frame, digit, increase_unit)
#endif

enum fimc_is_debug_state {
	FIMC_IS_DEBUG_OPEN
};

enum dbg_dma_dump_type {
	DBG_DMA_DUMP_IMAGE,
	DBG_DMA_DUMP_META,
};

struct fimc_is_debug {
	struct dentry		*root;
	struct dentry		*imgfile;
	struct dentry		*event_dir;
	struct dentry		*logfile;

	/* log dump */
	size_t			read_vptr;
	struct fimc_is_minfo	*minfo;

	/* debug message */
	size_t			dsentence_pos;
	char			dsentence[DEBUG_SENTENCE_MAX];

	unsigned long		state;

};

extern struct fimc_is_debug fimc_is_debug;

int fimc_is_debug_probe(void);
int fimc_is_debug_open(struct fimc_is_minfo *minfo);
int fimc_is_debug_close(void);

void fimc_is_dmsg_init(void);
void fimc_is_dmsg_concate(const char *fmt, ...);
char *fimc_is_dmsg_print(void);
void fimc_is_print_buffer(char *buffer, size_t len);
int fimc_is_debug_dma_dump(struct fimc_is_queue *queue, u32 index, u32 vid, u32 type);
#ifdef DBG_DRAW_DIGIT
void fimc_is_draw_digit(ulong addr, int width, int height, u32 pixelformat,
		u32 bitwidth, int row_index, int col_index, u64 digit);
#endif

int imgdump_request(ulong cookie, ulong kvaddr, size_t size);

#define FIMC_IS_EVENT_MAX_NUM	SZ_4K
#define EVENT_STR_MAX		SZ_128

typedef enum fimc_is_event_store_type {
	/* normal log - full then replace first normal log buffer */
	FIMC_IS_EVENT_NORMAL = 0x1,
	/* critical log - full then stop to add log to critical log buffer*/
	FIMC_IS_EVENT_CRITICAL = 0x2,
	FIMC_IS_EVENT_OVERFLOW_CSI = 0x3,
	FIMC_IS_EVENT_OVERFLOW_3AA = 0x4,
	FIMC_IS_EVENT_ALL,
} fimc_is_event_store_type_t;

struct fimc_is_debug_event_log {
	unsigned int log_num;
	ktime_t time;
	fimc_is_event_store_type_t event_store_type;
	char dbg_msg[EVENT_STR_MAX];
	void (*callfunc)(void *);

	/* This parameter should be used in non-atomic context */
	void *ptrdata;
	int cpu;
};

struct fimc_is_debug_event {
	struct dentry			*log;

	struct dentry			*logfilter;
	u32				log_filter;

	struct dentry			*logenable;
	u32				log_enable;

#ifdef ENABLE_DBG_EVENT_PRINT
	atomic_t			event_index;
	atomic_t			critical_log_tail;
	atomic_t			normal_log_tail;
	struct fimc_is_debug_event_log	event_log_critical[FIMC_IS_EVENT_MAX_NUM];
	struct fimc_is_debug_event_log	event_log_normal[FIMC_IS_EVENT_MAX_NUM];
#endif

	atomic_t			overflow_csi;
	atomic_t			overflow_3aa;
};

#ifdef ENABLE_DBG_EVENT_PRINT
void fimc_is_debug_event_print(u32 event_type, void (*callfunc)(void *), void *ptrdata, size_t datasize, const char *fmt, ...);
#else
#define fimc_is_debug_event_print(...)	do { } while(0)
#endif
void fimc_is_debug_event_count(u32 event_type);
void fimc_is_dbg_print(char *fmt, ...);
int fimc_is_debug_info_dump(struct seq_file *s, struct fimc_is_debug_event *debug_event);
#endif
