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
 * <Declaration>
 *
 */

#ifndef _IDIOT_CONFIG_H_
#define _IDIOT_CONFIG_H_

/* Max number of digit for testcase id (1~99999999) */
#define TC_NUM_DIGIT	8
#define TC_NAME_LEN	32
#define TESTFUNCNAME(name) IDIOT_TESTCASE_##name

/* Buffer to store the result */
#define IDIOT_RESULT_BUF_LEN	4096
#define IDIOT_FINAL_RESULT_LEN	64

#endif

