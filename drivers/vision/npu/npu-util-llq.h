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

#ifndef _NPU_UTIL_LLQ_H_
#define _NPU_UTIL_LLQ_H_

#include <linux/kfifo.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>

#include "npu-config.h"

#define LLQ_TASK_LIST_LEN	16

typedef void (*LLQ_task_t)(void);

#define LLQ_TIME_TRACKING
#ifdef LLQ_TIME_TRACKING
struct llq_time_stat {
	struct timespec	ts_init;	/* First initialization */
	struct timespec	ts_attempt_get;	/* Last invocation of GET */
	struct timespec	ts_success_get;	/* Last successful invcation of GET */
	struct timespec	ts_attempt_put;	/* Last invocation of PUT */
	struct timespec	ts_success_put;	/* Last successful invcation of PUT */
	struct timespec	ts_is_empty;	/* Last invocation of is_available */
};

/* Time stamp update */
#define __LLQ_UPDATE_STAT(QNAME, TARGET_STAT)					\
do {										\
	((QNAME).stat.TARGET_STAT) = ktime_to_timespec(ktime_get_boottime());	\
} while (0)									\

/* Convinient macro for setting two timestamp at once */
#define __LLQ_UPDATE_STAT2(QNAME, TARGET_STAT_1, TARGET_STAT_2)			\
do {										\
	((QNAME).stat.TARGET_STAT_1) = ktime_to_timespec(ktime_get_boottime());	\
	(QNAME).stat.TARGET_STAT_2 = (QNAME).stat.TARGET_STAT_1;		\
} while (0)									\

#else	/* !LLQ_TIME_TRACKING */
/* Skip time stat collection */
struct llq_time_stat {};
#define __LLQ_UPDATE_STAT(QNAME, TARGET_STAT)
#define __LLQ_UPDATE_STAT2(QNAME, TARGET_STAT_1, TARGET_STAT_2)
#endif	/* LLQ_TIME_TRACKING */

/*
 * LLQ has two kinds of initialization method:
 *
 * Method 1:
 *  Declare a 'type' of LLQ and declare its instance
 *  eg)
 *   LLQ_TYPE_DECLARE(my_llq_type_t, struct data, 32);
 *   ...
 *   my_llq_type_t my_llq;
 *
 *
 * Method 2:
 *  Declare instance directly
 *  eg)
 *   LLQ_DECLARE(my_llq, struct data, 32);
 *
 * For both cases, instance name of LLQ is 'my_llq' and accessor functions can
 * be invoked as following:
 * LLQ_INIT(my_llq);
 * LLQ_PUT(my_llq, data);
 *
 * Note: If you want to share the LLQ across difference source files,
 *       you need to use 'Method 1'.
 */
#define LLQ_TYPE_DECLARE(LLQ_TYPE_NAME, type, len)	\
typedef struct {					\
	DECLARE_KFIFO(kfifo, type, len);		\
	LLQ_task_t task_list[LLQ_TASK_LIST_LEN + 2];	\
	struct llq_time_stat	stat;			\
} LLQ_TYPE_NAME;					\

#define LLQ_DECLARE(qname, type, len)			\
struct {						\
	DECLARE_KFIFO(kfifo, type, len);		\
	LLQ_task_t task_list[LLQ_TASK_LIST_LEN + 2];	\
	struct llq_time_stat	stat;			\
} qname;						\

#define LLQ_INIT(qname)				\
({						\
	INIT_KFIFO((qname).kfifo);		\
	(qname).task_list[0] = NULL;		\
	__LLQ_UPDATE_STAT((qname), ts_init);	\
})						\

/*
 * When an 'invalid initializer' error occured on LLQ_PUT(..),
 * please make sure the elem is scalar type (not pointer).
 */
#define LLQ_PUT(qname, elem)						\
({									\
	unsigned int ret;						\
	LLQ_task_t *q_task = (qname).task_list;				\
	ret = kfifo_put(&((qname).kfifo), (elem));			\
	if (ret) {							\
		__LLQ_UPDATE_STAT2((qname), ts_attempt_put, ts_success_put);	\
		while (*q_task != NULL)					\
			(*q_task++)();					\
	} else {							\
		__LLQ_UPDATE_STAT((qname), ts_attempt_put);		\
	}								\
	ret;								\
})									\

#define LLQ_GET(qname, elem)						\
__kfifo_uint_must_check_helper(						\
({									\
	unsigned int ret;						\
	ret = kfifo_get(&((qname).kfifo), (elem));			\
	if (ret)	{\
		__LLQ_UPDATE_STAT2((qname), ts_attempt_get, ts_success_get);	\
	} else {\
		__LLQ_UPDATE_STAT((qname), ts_attempt_get);			\
	 } \
	ret;								\
})									\
)									\


#define LLQ_IS_EMPTY(qname)			\
({						\
	__LLQ_UPDATE_STAT((qname), ts_is_empty);	\
	kfifo_is_empty(&((qname).kfifo));	\
})						\


#define LLQ_PURGE(qname)			\
({						\
	(qname).task_list[0] = NULL;		\
	kfifo_reset(&((qname).kfifo));		\
})						\


#define LLQ_DESTROY(qname)			\
({						\
	(qname).task_list[0] = NULL;		\
	kfifo_reset(&((qname).kfifo));		\
	kfifo_free(&((qname).kfifo));		\
})						\

#define LLQ_REGISTER_TASK(qname, task_func)			\
({								\
	int i, ret = -ENOENT;					\
	for (i = 0; i < LLQ_TASK_LIST_LEN; ++i) {		\
		if ((qname).task_list[i] == NULL) {		\
			(qname).task_list[i] = task_func;	\
			(qname).task_list[i+1] = NULL;		\
			ret = 0;				\
			break;					\
		}						\
	}							\
	ret;							\
})								\

#define LLQ_DUMP(qname, out_buf, len)
#endif	/* _NPU_UTIL_LLQ_H_ */
