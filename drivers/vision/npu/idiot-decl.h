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
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include "idiot-config.h"

#define STRINGFY(v)	#v

/* Buffers to store the result */
char IDIOT_result_buf[IDIOT_RESULT_BUF_LEN + 4] = {0};
size_t IDIOT_result_len;

/* type definition of testcase function table */
struct IDIOT_tc_list {
	int (*tc_func)(struct file*, char*, size_t);
	char *tc_name;
};

/* Check whether the idiot file name is available */
#ifndef IDIOT_ALL_TESTCASE_INCLUDE
#error "Please define IDIOT_ALL_TESTCASE_INCLUDE as an header file includes all the .idiot files."
#endif

/* Do not include declaration section of testcase here */
#ifdef IDIOT_DECL_SECTION
#undef IDIOT_DECL_SECTION
#endif

/* Insert prototype of test functions */
#ifdef TESTDEF
#undef TESTDEF
#endif
#define TESTDEF(fname, ...)	int TESTFUNCNAME(fname) (struct file*, char*, size_t);
#include IDIOT_ALL_TESTCASE_INCLUDE
#undef TESTDEF

/* Insert test function table */
#define TESTDEF(fname, ...)	{TESTFUNCNAME(fname), #fname},
static struct IDIOT_tc_list IDIOT_testcase_table[] = {
	#include IDIOT_ALL_TESTCASE_INCLUDE
};
#undef TESTDEF

/* Save list of testcases registered into result buffer */
static ssize_t IDIOT_list_tests(void)
{
	unsigned int i;
	int ret;
	const size_t tcnum = ARRAY_SIZE(IDIOT_testcase_table);
	const size_t max_line_len = TC_NAME_LEN + TC_NUM_DIGIT + 10;
	size_t writeLen = 0;
	char *pt;

	for (i = 0, pt = IDIOT_result_buf; (i < tcnum) && ((writeLen + 1) < (IDIOT_RESULT_BUF_LEN - max_line_len)); i++) {
		ret = scnprintf(pt, max_line_len, "[%08d] : %s\n",
			i + 1, IDIOT_testcase_table[i].tc_name);	/* TC id is 1-based */
		writeLen += ret;
		pt += ret;
	}
	*pt = '\0';

	IDIOT_result_len = writeLen + 1;	/* Add one more character for NULL termination */
	return writeLen;
}


/* Test harness functions - write and read */

static int IDIOT_open(struct inode *inode, struct file *file)
{
	/* No op */
	return 0;
}

static ssize_t IDIOT_read(struct file *file, char __user *userbuf, size_t count, loff_t *offp)
{
	size_t copyLen;
	unsigned long ret;

	if (IDIOT_result_len == 0) {
		return 0;	/* No result */
	}

	copyLen = min(count, IDIOT_result_len);
	ret = copy_to_user(userbuf, IDIOT_result_buf, copyLen);
	if (ret != 0) {
		printk(KERN_ERR "IDIOT: Result write error. Result message may be truncated\n");
	}

	IDIOT_result_len -= copyLen;
	return copyLen;
}


static ssize_t IDIOT_write(struct file *file, const char *__user userbuf, size_t count, loff_t *offp)
{
	unsigned int i;
	size_t writeLen;
	unsigned int tcidx = 0;
	const size_t tcnum = ARRAY_SIZE(IDIOT_testcase_table);
	int test_result;
	char *pt;
	char ch;

	/* Skip leading non-digit characters */
	for (i = 0; i < count && i < TC_NUM_DIGIT && get_user(ch, userbuf + i) == 0; i++) {
		if ('0' <= ch && ch <= '9') {
			break;
		}
	}

	/* Convert string to integer, up to TC_NUM_DIGIT */
	for (; i < count && i < TC_NUM_DIGIT && get_user(ch, userbuf + i) == 0; i++) {
		if ('0' <= ch && ch <= '9') {
			tcidx *= 10;
			tcidx += ch - '0';
		} else {
			break;
		}
	}
	writeLen = i + 1;

	/* execute the specified test */
	IDIOT_result_buf[0] = '\0';
	IDIOT_result_len = 0;

	if (0 < tcidx && tcidx <= tcnum) {
		/* Invoke testcase function here */
		test_result = IDIOT_testcase_table[tcidx - 1]	/* tcidx is 1-based index */
			.tc_func(file, IDIOT_result_buf, IDIOT_RESULT_BUF_LEN);

		for (pt = IDIOT_result_buf; *pt != '\0'; pt++);	/* Search end of buffer */
		if (pt > IDIOT_result_buf + IDIOT_RESULT_BUF_LEN - IDIOT_FINAL_RESULT_LEN) {
			/* Check whether there are enough space to store final result line */
			IDIOT_result_buf[0] = '!';
			pt = IDIOT_result_buf + 1;
		}

		/* Write result */
		scnprintf(pt, IDIOT_FINAL_RESULT_LEN, "@@%c TC# [%08u/%lu] %s : %s\n",
			(test_result == 0) ? 'P' : 'F',
			tcidx, tcnum, IDIOT_testcase_table[tcidx - 1].tc_name,
			(test_result == 0) ? "PASS" : "FAIL");

		/* Adjust result message length */
		for (; *pt != '\0'; pt++);
		IDIOT_result_len = (pt + 1) - IDIOT_result_buf;	/* Includes null termination */

		/* Successful return */
		return writeLen;
	} else if (tcidx == 0) {
		/* Get TC list */
		IDIOT_list_tests();
		return writeLen;
	} else {
		return -1;
	}
}

#if 0
int main(void)
{
	size_t i;
	const size_t tcnum = ARRAY_SIZE(IDIOT_testcase_table);
	unsigned int pass_cnt = 0;
	unsigned int fail_cnt = 0;

	printf("Number of TC is %lu\n", tcnum);
	for (i = 0; i < tcnum; ++i) {
		if (IDIOT_testcase_table[i]() == 0) {
			printf("@@ TC# [%08lu/%lu] : PASS\n", i+1, tcnum);
			pass_cnt++;
		} else {
			printf("@@ TC# [%08lu/%lu] : FAIL\n", i+1, tcnum);
			fail_cnt++;
		}
	}
	printf("@@ Among %lu testcases, %u tests have run, PASS = %u, FAIL = %u.\n",
		tcnum, pass_cnt + fail_cnt, pass_cnt, fail_cnt);

	return 0;
}
#endif
