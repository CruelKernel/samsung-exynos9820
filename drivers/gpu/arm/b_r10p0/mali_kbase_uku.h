/*
 *
 * (C) COPYRIGHT 2010, 2012-2015 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */


#ifndef _KBASE_UKU_H_
#define _KBASE_UKU_H_

#include "mali_uk.h"
#include "mali_base_kernel.h"

/* This file needs to support being included from kernel and userside (which use different defines) */
#if defined(CONFIG_MALI_ERROR_INJECT) || MALI_ERROR_INJECT_ON
#define SUPPORT_MALI_ERROR_INJECT
#endif /* defined(CONFIG_MALI_ERROR_INJECT) || MALI_ERROR_INJECT_ON */
#if defined(CONFIG_MALI_NO_MALI)
#define SUPPORT_MALI_NO_MALI
#elif defined(MALI_NO_MALI)
#if MALI_NO_MALI
#define SUPPORT_MALI_NO_MALI
#endif
#endif

#if defined(SUPPORT_MALI_NO_MALI) || defined(SUPPORT_MALI_ERROR_INJECT)
#include "backend/gpu/mali_kbase_model_dummy.h"
#endif

#include "mali_kbase_gpuprops_types.h"

enum kbase_uk_function_id {
	KBASE_FUNC_MEM_ALLOC = (UK_FUNC_ID + 0),
	KBASE_FUNC_MEM_IMPORT = (UK_FUNC_ID + 1),
	KBASE_FUNC_MEM_COMMIT = (UK_FUNC_ID + 2),
	KBASE_FUNC_MEM_QUERY = (UK_FUNC_ID + 3),
	KBASE_FUNC_MEM_FREE = (UK_FUNC_ID + 4),
	KBASE_FUNC_MEM_FLAGS_CHANGE = (UK_FUNC_ID + 5),
	KBASE_FUNC_MEM_ALIAS = (UK_FUNC_ID + 6),

	/* UK_FUNC_ID + 7 not in use since BASE_LEGACY_UK6_SUPPORT dropped */

	KBASE_FUNC_SYNC  = (UK_FUNC_ID + 8),

	KBASE_FUNC_POST_TERM = (UK_FUNC_ID + 9),

	KBASE_FUNC_HWCNT_SETUP = (UK_FUNC_ID + 10),
	KBASE_FUNC_HWCNT_DUMP = (UK_FUNC_ID + 11),
	KBASE_FUNC_HWCNT_CLEAR = (UK_FUNC_ID + 12),

	KBASE_FUNC_GPU_PROPS_REG_DUMP = (UK_FUNC_ID + 14),

	KBASE_FUNC_FIND_CPU_OFFSET = (UK_FUNC_ID + 15),

	KBASE_FUNC_GET_VERSION = (UK_FUNC_ID + 16),
	KBASE_FUNC_SET_FLAGS = (UK_FUNC_ID + 18),

	KBASE_FUNC_SET_TEST_DATA = (UK_FUNC_ID + 19),
	KBASE_FUNC_INJECT_ERROR = (UK_FUNC_ID + 20),
	KBASE_FUNC_MODEL_CONTROL = (UK_FUNC_ID + 21),

	/* UK_FUNC_ID + 22 not in use since BASE_LEGACY_UK8_SUPPORT dropped */

	KBASE_FUNC_FENCE_VALIDATE = (UK_FUNC_ID + 23),
	KBASE_FUNC_STREAM_CREATE = (UK_FUNC_ID + 24),
	KBASE_FUNC_GET_PROFILING_CONTROLS = (UK_FUNC_ID + 25),
	KBASE_FUNC_SET_PROFILING_CONTROLS = (UK_FUNC_ID + 26),
					    /* to be used only for testing
					    * purposes, otherwise these controls
					    * are set through gator API */

	KBASE_FUNC_DEBUGFS_MEM_PROFILE_ADD = (UK_FUNC_ID + 27),
	KBASE_FUNC_JOB_SUBMIT = (UK_FUNC_ID + 28),
	KBASE_FUNC_DISJOINT_QUERY = (UK_FUNC_ID + 29),

	KBASE_FUNC_GET_CONTEXT_ID = (UK_FUNC_ID + 31),

	KBASE_FUNC_TLSTREAM_ACQUIRE_V10_4 = (UK_FUNC_ID + 32),
#if MALI_UNIT_TEST
	KBASE_FUNC_TLSTREAM_TEST = (UK_FUNC_ID + 33),
	KBASE_FUNC_TLSTREAM_STATS = (UK_FUNC_ID + 34),
#endif /* MALI_UNIT_TEST */
	KBASE_FUNC_TLSTREAM_FLUSH = (UK_FUNC_ID + 35),

	KBASE_FUNC_HWCNT_READER_SETUP = (UK_FUNC_ID + 36),

#ifdef SUPPORT_MALI_NO_MALI
	KBASE_FUNC_SET_PRFCNT_VALUES = (UK_FUNC_ID + 37),
#endif

	KBASE_FUNC_SOFT_EVENT_UPDATE = (UK_FUNC_ID + 38),

	KBASE_FUNC_MEM_JIT_INIT = (UK_FUNC_ID + 39),

	KBASE_FUNC_TLSTREAM_ACQUIRE = (UK_FUNC_ID + 40),

	KBASE_FUNC_SET_MIN_LOCK,
	KBASE_FUNC_UNSET_MIN_LOCK,

	KBASE_FUNC_STEP_UP_MAX_GPU_LIMIT,
	KBASE_FUNC_RESTORE_MAX_GPU_LIMIT,

	KBASE_FUNC_SET_VK_BOOST_LOCK,
	KBASE_FUNC_UNSET_VK_BOOST_LOCK,

	KBASE_FUNC_MAX
};

#endif				/* _KBASE_UKU_H_ */

