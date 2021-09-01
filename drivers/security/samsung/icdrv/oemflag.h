/*
 * Get/Set OEM flags
 *
 * Copyright (C) 2017 Samsung Electronics, Inc.
 * Jonghun Song, <justin.song@samsung.com>
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

#ifndef __OEM_FLAG_H
#define __OEM_FLAG_H

#define OEMFLAG_SUCCESS 0
#define OEMFLAG_FAIL -1

enum oemflag_id {
	OEMFLAG_NONE = 0,
	OEMFLAG_MIN_FLAG = 2,
	OEMFLAG_TZ_DRM,
	OEMFLAG_FIDO,
	OEMFLAG_CC,
	OEMFLAG_ETC,
	OEMFLAG_NUM_OF_FLAG,
};

#ifdef CONFIG_KUNIT
#include <kunit/mock.h>
#endif

#ifdef CONFIG_KUNIT
extern int oem_flags_set(enum oemflag_id index);
extern int oem_flags_get(enum oemflag_id index);
#else
int oem_flags_set(enum oemflag_id index);
int oem_flags_get(enum oemflag_id index);
#endif

#endif
