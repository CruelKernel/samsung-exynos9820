/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * IDIOT - Intutive Device driver integrated Operation Testing
 *
 */

#include <linux/time.h>
#include <linux/timekeeping.h>
#include "idiot-config.h"

#define IDIOT_ASSERT_EQ(test, expected, TNAME_FMT)		__IDIOT_ASSERT_CONDITION(-, ==, test, expected, TNAME_FMT)
#define IDIOT_ASSERT_NEQ(test, expected, TNAME_FMT)		__IDIOT_ASSERT_CONDITION(-, !=, test, expected, TNAME_FMT)
#define IDIOT_ASSERT_LE(test, expected, TNAME_FMT)		__IDIOT_ASSERT_CONDITION(-, <=, test, expected, TNAME_FMT)
#define IDIOT_ASSERT_EQ_T(TAG, test, expected, TNAME_FMT)	__IDIOT_ASSERT_CONDITION(TAG, ==, test, expected, TNAME_FMT)
#define IDIOT_ASSERT_NEQ_T(TAG, test, expected, TNAME_FMT)	__IDIOT_ASSERT_CONDITION(TAG, !=, test, expected, TNAME_FMT)
#define IDIOT_ASSERT_LE_T(TAG, test, expected, TNAME_FMT)	__IDIOT_ASSERT_CONDITION(TAG, <=, test, expected, TNAME_FMT)
#define IDIOT_ASSERT_GE_T(TAG, test, expected, TNAME_FMT)	__IDIOT_ASSERT_CONDITION(TAG, >=, test, expected, TNAME_FMT)

/* For each condition - Old IF */
#define IDIOT_ASSERT(TNAME, TNAME_FMT, test, expected)		__IDIOT_ASSERT_CONDITION(-, ==, test, expected, TNAME_FMT)
#define IDIOT_ASSERT_NOT(TNAME, TNAME_FMT, test, expected)	__IDIOT_ASSERT_CONDITION(-, !=, test, expected, TNAME_FMT)

/* Base implementation of assert macro */
#define __IDIOT_ASSERT_CONDITION(TAG, COND, test, expected, TNAME_FMT)			\
do {											\
	typeof(test) __idiot_actual = (test);						\
	typeof(test) __idiot_expected = (expected);					\
	int __idiot_this_msgLen = 0;							\
	if (!((__idiot_actual) COND(__idiot_expected))) {				\
		__idiot_this_msgLen = scnprintf(__idiot_msgbuf, __idiot_bufLen		\
			, "Assert at %s:%d(" #TAG ") (Cond[" #test			\
			  "]/Expected[" #COND #expected "]) - Actual [" #TNAME_FMT	\
			  "] " #COND " Expected [" #TNAME_FMT "]\n"			\
			, __FILE__, __LINE__, __idiot_actual, __idiot_expected);	\
		__idiot_msgbuf += __idiot_this_msgLen;					\
		__idiot_bufLen -= __idiot_this_msgLen;					\
		__idiot_return_value = 1;						\
		goto __idiot_testcase_termination;					\
	}										\
} while (0)										\

#define IDIOT_ASSERT_DONE_IN_US(US, ...)						\
{											\
	ktime_t __idiot_kt_start, __idiot_kt_finish;					\
	s64 __idiot_time_diff_us;							\
	__idiot_kt_start = ktime_get_boottime();					\
	{										\
		/* Test code */								\
		__VA_ARGS__								\
	}										\
	__idiot_kt_finish = ktime_get_boottime();					\
	/* Time diff in usec */								\
	__idiot_time_diff_us =								\
		ktime_to_us(ktime_sub(__idiot_kt_finish, __idiot_kt_start));		\
	IDIOT_MSG("Line:%d : Elapsed time = %lld us\n", __LINE__, __idiot_time_diff_us);\
	IDIOT_ASSERT_LE(__idiot_time_diff_us, (US), % lld);				\
}											\

/* Leave a message in test result buffer for reference */
#define IDIOT_MSG(...)									\
do {											\
	int __idiot_this_msgLen = 0;							\
	__idiot_this_msgLen = scnprintf(__idiot_msgbuf, __idiot_bufLen, __VA_ARGS__);	\
	__idiot_msgbuf += __idiot_this_msgLen;						\
	__idiot_bufLen -= __idiot_this_msgLen;						\
} while (0)


/* Insert Testcase definition */
#undef TESTDEF
#undef SETUP_CODE
#undef TEARDOWN_CODE
#define SETUP_CODE
#define TEARDOWN_CODE
#define TESTDEF(fname, ...) \
int TESTFUNCNAME(fname) (struct file *file, char *__idiot_msgbuf, size_t __idiot_bufLen) \
{	\
	int __idiot_return_value = 0;		\
	SETUP_CODE				\
						\
	{					\
		__VA_ARGS__			\
__idiot_testcase_termination : \
		{				\
			TEARDOWN_CODE		\
		}				\
	}					\
						\
	return __idiot_return_value;		\
}						\

/* Include declaration section of idiot file */
#ifndef IDIOT_DECL_SECTION
#define IDIOT_DECL_SECTION
#endif

#ifdef IDIOT_TESTCASE_IMPL
#include IDIOT_TESTCASE_IMPL
#else
#error "Please define IDIOT_TESTCASE_IMPL as an testcase implementation."
#endif
