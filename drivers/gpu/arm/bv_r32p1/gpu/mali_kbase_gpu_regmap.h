/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 *
 * (C) COPYRIGHT 2010-2021 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 */

#ifndef _KBASE_GPU_REGMAP_H_
#define _KBASE_GPU_REGMAP_H_

#include <uapi/gpu/arm/bv_r32p1/gpu/mali_kbase_gpu_regmap.h>
#if MALI_USE_CSF
#include <uapi/gpu/arm/bv_r32p1/gpu/backend/mali_kbase_gpu_regmap_csf.h>
#else
#include <uapi/gpu/arm/bv_r32p1/gpu/backend/mali_kbase_gpu_regmap_jm.h>
#endif

/* GPU_U definition */
#ifdef __ASSEMBLER__
#define GPU_U(x) x
#define GPU_UL(x) x
#define GPU_ULL(x) x
#else
#define GPU_U(x) x##u
#define GPU_UL(x) x##ul
#define GPU_ULL(x) x##ull
#endif /* __ASSEMBLER__ */
/* AS_LOCKADDR register */
#define AS_LOCKADDR_LOCKADDR_SIZE_SHIFT GPU_U(0)
#define AS_LOCKADDR_LOCKADDR_SIZE_MASK                                         \
	(GPU_U(0x3F) << AS_LOCKADDR_LOCKADDR_SIZE_SHIFT)
#define AS_LOCKADDR_LOCKADDR_SIZE_GET(reg_val)                                 \
	(((reg_val)&AS_LOCKADDR_LOCKADDR_SIZE_MASK) >>                               \
	 AS_LOCKADDR_LOCKADDR_SIZE_SHIFT)
#define AS_LOCKADDR_LOCKADDR_SIZE_SET(reg_val, value)                          \
	(((reg_val) & ~AS_LOCKADDR_LOCKADDR_SIZE_MASK) |                             \
	 (((value) << AS_LOCKADDR_LOCKADDR_SIZE_SHIFT) &                             \
	 AS_LOCKADDR_LOCKADDR_SIZE_MASK))
#define AS_LOCKADDR_LOCKADDR_BASE_SHIFT GPU_U(12)
#define AS_LOCKADDR_LOCKADDR_BASE_MASK                                                             \
	(GPU_ULL(0xFFFFFFFFFFFFF) << AS_LOCKADDR_LOCKADDR_BASE_SHIFT)
#define AS_LOCKADDR_LOCKADDR_BASE_GET(reg_val)                                 \
	(((reg_val)&AS_LOCKADDR_LOCKADDR_BASE_MASK) >>                               \
	 AS_LOCKADDR_LOCKADDR_BASE_SHIFT)
#define AS_LOCKADDR_LOCKADDR_BASE_SET(reg_val, value)                          \
	(((reg_val) & ~AS_LOCKADDR_LOCKADDR_BASE_MASK) |                             \
	 (((value) << AS_LOCKADDR_LOCKADDR_BASE_SHIFT) &                             \
	 AS_LOCKADDR_LOCKADDR_BASE_MASK))
#define AS_LOCKADDR_FLUSH_SKIP_LEVELS_SHIFT (6)
#define AS_LOCKADDR_FLUSH_SKIP_LEVELS_MASK ((0xF) << AS_LOCKADDR_FLUSH_SKIP_LEVELS_SHIFT)
#define AS_LOCKADDR_FLUSH_SKIP_LEVELS_SET(reg_val, value)                                          \
	(((reg_val) & ~AS_LOCKADDR_FLUSH_SKIP_LEVELS_MASK) |                                       \
	 ((value << AS_LOCKADDR_FLUSH_SKIP_LEVELS_SHIFT) & AS_LOCKADDR_FLUSH_SKIP_LEVELS_MASK))
/* Include POWER_CHANGED_SINGLE in debug builds for use in irq latency test. */
#ifdef CONFIG_MALI_DEBUG
#undef GPU_IRQ_REG_ALL
#define GPU_IRQ_REG_ALL (GPU_IRQ_REG_COMMON | POWER_CHANGED_SINGLE)
#endif /* CONFIG_MALI_DEBUG */

#endif /* _KBASE_GPU_REGMAP_H_ */
