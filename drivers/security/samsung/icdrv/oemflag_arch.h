/*
 * Get/Set OEM flags
 *
 * Copyright (C) 2017 Samsung Electronics, Inc.
 * Jonghun Song, <justin.song@samsung.com>
 * Egor Ulesykiy, <e.uelyskiy@samsung.com>
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
#ifndef __OEMFLAG_ARCH_H
#define __OEMFLAG_ARCH_H

#include <linux/types.h>
#include "oemflag.h"

/* Sets FUSE flags in TEE
 * name - FUSE flag name
 *
 * Returns value which was returned by TEE
 */
int set_tamper_fuse(enum oemflag_id name);

/* Gets FUSE flags from TEE
 * name - FUSE flag name
 *
 * Returns the value which was returned by TEE * or * 0 if this operation
 * isn't supported
 */
int get_tamper_fuse(enum oemflag_id name);

#endif /* __OEMFLAG_ARCH_H */
