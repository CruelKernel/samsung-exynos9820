/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _NPU_LOG_H_
#define _NPU_LOG_H_

#include <linux/types.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/wait.h>
#include <linux/atomic.h>
#include "npu-device.h"

#define NPU_RING_LOG_BUFFER_MAGIC	0x3920FFAC


/* Log level definition */
typedef enum {
	NPU_LOG_TRACE	= 0,
	NPU_LOG_DBG,	/* Default. Set in npu_log.c */
	NPU_LOG_INFO,
	NPU_LOG_WARN,
	NPU_LOG_ERR,
	NPU_LOG_NONE,
	NPU_LOG_INVALID,
} npu_log_level_e;

/* Default tag definition */
#ifndef NPU_LOG_TAG
#define NPU_LOG_TAG	"*"
#endif

/* Structure for log management object */
struct npu_log {
	/* kernel logger level */
	npu_log_level_e		pr_level;
	/* memory storage logger level */
	npu_log_level_e		st_level;

	/* Buffer for log storage */
	char			*st_buf;
	size_t			wr_pos;
	size_t			st_size;
	size_t			line_cnt;

	/* Save last dump postion to gracefully handle consecutive error case */
	size_t			last_dump_line_cnt;

	/* Position where the last dump log has read */
	size_t			last_dump_mark_pos;

	/* Reference counter for debugfs interface */
	atomic_t		fs_ref;

	/* Wait queue to notify readers */
	wait_queue_head_t	wq;
};
extern struct npu_log	npu_log;
extern struct npu_log	fw_report;

/*
 * Define interval mask to printout sync mark on kernel log
 * to sync with memory stored log.
 * sync mark is printed if (npu_log.line_cnt & MASK == 0)
 */
#define NPU_STORE_LOG_SYNC_MARK_INTERVAL_MASK	0x7F	/* Sync mark on every 128 messages */
#define NPU_STORE_LOG_SYNC_MARK_MSG		"NPU log sync [%zu]\n"

/* Amount of log dumped on an error occurred */
#define NPU_STORE_LOG_DUMP_SIZE_ON_ERROR	(16 * 1024)

/* Log flush wait interval and count when the store log is de-initialized */
#define NPU_STORE_LOG_FLUSH_INTERVAL_MS		500

/* Chunk size used in bitmap_scnprintf */
#define CHUNKSZ                 32

/* Log size used in memdump with memcpy */
#define MEMLOGSIZE                             16


/* Structure to represent ring buffer log storage */
struct npu_ring_log_buffer {
	u32			magic;
	const char *name;
	struct mutex		lock;
	struct completion	read_wq;
	DECLARE_KFIFO_PTR(fifo, char);
	size_t			size;
	size_t			wPtr;
};

/* Per file descriptor object for ring log buffer */
struct npu_ring_log_buffer_fd_obj {
	struct npu_ring_log_buffer *ring_buf;
	size_t				last_pos;
};

/* Prototypes */
int npu_store_log(npu_log_level_e loglevel, const char *fmt, ...);	/* Shall not be used directly */
void npu_store_log_init(char *buf_addr, size_t size);
void npu_store_log_deinit(void);
void npu_log_on_error(void);

void npu_fw_report_init(char *buf_addr, const size_t size);
void npu_fw_report_store(char *strRep, int nSize);
void dbg_print_fw_report_st(struct npu_log stLog);
void npu_fw_report_deinit(void);

/* Utilities */
s32 atoi(const char *psz_buf);
int bitmap_scnprintf(char *buf, unsigned int buflen, const unsigned long *maskp, int nmaskbits);

/* Memory dump */
int npu_debug_memdump8(u8 *start, u8 *end);
int npu_debug_memdump16(u16 *start, u16 *end);
int npu_debug_memdump32(u32 *start, u32 *end);
int npu_debug_memdump32_by_memcpy(u32 *start, u32 *end);

#define ISPRINTABLE(strValue)			(isascii(strValue) && isprint(strValue) ? strValue : '.')

#define probe_info(fmt, ...)            pr_info("NPU:" fmt, ##__VA_ARGS__)
#define probe_warn(fmt, args...)        pr_warn("NPU:[WRN]" fmt, ##args)
#define probe_err(fmt, args...)         pr_err("NPU:[ERR]%s:%d:" fmt, __func__, __LINE__, ##args)
#define probe_trace(fmt, ...)           ((npu_log.pr_level <= NPU_LOG_TRACE)?pr_info(fmt, ##__VA_ARGS__):0)

#ifdef DEBUG_LOG_MEMORY
#define npu_log_on_lv_target(LV, PR_FUNC, fmt, ...)	PR_FUNC(fmt, ##__VA_ARGS__)
#else
#define npu_log_on_lv_target(LV, PR_FUNC, fmt, ...)		\
	do {	\
		int __npu_ret = 0;	\
		if (npu_log.st_level <= (LV))	\
			__npu_ret = npu_store_log((LV), fmt, ##__VA_ARGS__);	\
		if (__npu_ret)		\
			pr_err(fmt, ##__VA_ARGS__);	\
		else {			\
			if (npu_log.pr_level <= (LV))		\
				PR_FUNC(fmt, ##__VA_ARGS__);	\
		}	\
	} while (0)
#endif

#define npu_err_target(fmt, ...)	\
	do {	\
		npu_log_on_lv_target(NPU_LOG_ERR, pr_err, fmt, ##__VA_ARGS__);	\
		npu_log_on_error();	\
	} while (0)
#define npu_warn_target(fmt, ...)	npu_log_on_lv_target(NPU_LOG_WARN, pr_warning, fmt, ##__VA_ARGS__)
#define npu_info_target(fmt, ...)	npu_log_on_lv_target(NPU_LOG_INFO, pr_notice, fmt, ##__VA_ARGS__)
#define npu_dbg_target(fmt, ...)	npu_log_on_lv_target(NPU_LOG_DBG, pr_info, fmt, ##__VA_ARGS__)
#define npu_trace_target(fmt, ...)	npu_log_on_lv_target(NPU_LOG_TRACE, pr_debug, fmt, ##__VA_ARGS__)

#define npu_err(fmt, args...) \
	npu_err_target("NPU:[" NPU_LOG_TAG "][ERR]%s(%d):" fmt, __func__, __LINE__, ##args)

#define npu_warn(fmt, args...) \
	npu_warn_target("NPU:[" NPU_LOG_TAG "][WRN]%s(%d):" fmt, __func__, __LINE__, ##args)

#define npu_info(fmt, args...) \
	npu_info_target("NPU:[" NPU_LOG_TAG "]%s(%d):" fmt, __func__, __LINE__, ##args)

#define npu_dbg(fmt, args...) \
	npu_dbg_target("NPU:[" NPU_LOG_TAG "]%s(%d):" fmt, __func__, __LINE__, ##args)

#define npu_trace(fmt, args...) \
	npu_trace_target("NPU:[T][" NPU_LOG_TAG "]%s(%d):" fmt, __func__, __LINE__, ##args)

/* Printout context ID */
#define npu_ierr(fmt, vctx, args...) \
	npu_err_target("NPU:[" NPU_LOG_TAG "][I%d][ERR]%s(%d):" fmt, (vctx)->id, __func__, __LINE__, ##args)

#define npu_iwarn(fmt, vctx, args...) \
	npu_warn_target("NPU:[" NPU_LOG_TAG "][I%d][WRN]%s(%d):" fmt, (vctx)->id, __func__, __LINE__, ##args)

#define npu_iinfo(fmt, vctx, args...) \
	npu_info_target("NPU:[" NPU_LOG_TAG "][I%d]%s(%d):" fmt, (vctx)->id, __func__, __LINE__, ##args)

#define npu_idbg(fmt, vctx, args...) \
	npu_dbg_target("NPU:[" NPU_LOG_TAG "][I%d]%s(%d):" fmt, (vctx)->id, __func__, __LINE__, ##args)

#define npu_itrace(fmt, vctx, args...) \
	npu_trace_target("NPU:[T][" NPU_LOG_TAG "][I%d]%s(%d):" fmt, (vctx)->id, __func__, __LINE__, ##args)

/* Printout unique ID */
#define npu_uerr(fmt, uid_obj, args...) \
	npu_err_target("NPU:[" NPU_LOG_TAG "][U%d][ERR]%s(%d):" fmt, (uid_obj)->uid, __func__, __LINE__, ##args)

#define npu_uwarn(fmt, uid_obj, args...) \
	npu_warn_target("NPU:[" NPU_LOG_TAG "][U%d][WRN]%s(%d):" fmt, (uid_obj)->uid, __func__, __LINE__, ##args)

#define npu_uinfo(fmt, uid_obj, args...) \
	npu_info_target("NPU:[" NPU_LOG_TAG "][U%d]%s(%d):" fmt, (uid_obj)->uid, __func__, __LINE__, ##args)

#define npu_udbg(fmt, uid_obj, args...) \
	npu_dbg_target("NPU:[" NPU_LOG_TAG "][U%d]%s(%d):" fmt, (uid_obj)->uid, __func__, __LINE__, ##args)

#define npu_utrace(fmt, uid_obj, args...) \
	npu_trace_target("NPU:[T][" NPU_LOG_TAG "][U%d]%s(%d):" fmt, (uid_obj)->uid, __func__, __LINE__, ##args)

/* Printout unique ID and frame ID */
#define npu_uferr(fmt, obj_ptr, args...) \
({	typeof(*obj_ptr) *__pt = (obj_ptr); npu_err_target("NPU:[" NPU_LOG_TAG "][U%u][F%u][ERR]%s(%d):" fmt, (__pt)->uid, (__pt)->frame_id, __func__, __LINE__, ##args); })

#define npu_ufwarn(fmt, obj_ptr, args...) \
({	typeof(*obj_ptr) *__pt = (obj_ptr); npu_warn_target("NPU:[" NPU_LOG_TAG "][U%u][F%u][WRN]%s(%d):" fmt, (__pt)->uid, (__pt)->frame_id, __func__, __LINE__, ##args); })

#define npu_ufinfo(fmt, obj_ptr, args...) \
({	typeof(*obj_ptr) *__pt = (obj_ptr); npu_info_target("NPU:[" NPU_LOG_TAG "][U%u][F%u]%s(%d):" fmt, (__pt)->uid, (__pt)->frame_id, __func__, __LINE__, ##args); })

#define npu_ufdbg(fmt, obj_ptr, args...) \
({	typeof(*obj_ptr) *__pt = (obj_ptr); npu_dbg_target("NPU:[" NPU_LOG_TAG "][U%u][F%u]%s(%d):" fmt, (__pt)->uid, (__pt)->frame_id, __func__, __LINE__, ##args); })

#define npu_uftrace(fmt, obj_ptr, args...) \
({	typeof(*obj_ptr) *__pt = (obj_ptr); npu_trace_target("NPU:[T][" NPU_LOG_TAG "][U%u][F%u]%s(%d):" fmt, (__pt)->uid, (__pt)->frame_id, __func__, __LINE__, ##args); })

/* Exported functions */
struct npu_device;
int npu_log_probe(struct npu_device *npu_device);
int npu_log_release(struct npu_device *npu_device);
int npu_log_open(struct npu_device *npu_device);
int npu_log_close(struct npu_device *npu_device);
int npu_log_start(struct npu_device *npu_device);
int npu_log_stop(struct npu_device *npu_device);
int fw_will_note(size_t len);

#endif /* _NPU_LOG_H_ */
