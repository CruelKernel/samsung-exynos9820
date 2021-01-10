/*
 *
 * Copyright (C) 2017 Samsung Electronics, Inc.
 * Jonghun Song, <justin.song@samsung.com>
 * Nguyen Quy Minh <minh.nq1@samsung.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ICD_H
#define __ICD_H

#ifdef CONFIG_KUNIT
#include <kunit/mock.h>
#include <linux/file.h>
#endif
#ifdef CONFIG_KUNIT
extern bool contains_str(const char * const array[], const char *str);
#endif
#endif
