/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 *
 * (C) COPYRIGHT 2021 Samsung Electronics. All rights reserved.
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

#ifndef _UAPI_MALI_EXYNOS_IOCTL_H_
#define _UAPI_MALI_EXYNOS_IOCTL_H_

#ifdef __cpluscplus
extern "C" {
#endif

#include <asm-generic/ioctl.h>
#include <linux/types.h>

#define KBASE_IOCTL_TYPE 0x80
/* Customer extension range */
#define KBASE_IOCTL_EXTRA_TYPE (KBASE_IOCTL_TYPE + 2)

/*
 * struct mali_exynos_ioctl_amigo_flags - Read amigo flags
 * @flags: Amigo flag set by Amigo module
 */
struct mali_exynos_ioctl_amigo_flags {
	__u32 flags;
};

#define MALI_EXYNOS_IOCTL_AMIGO_FLAGS \
	_IOR(KBASE_IOCTL_EXTRA_TYPE, 0, struct mali_exynos_ioctl_amigo_flags)

/*
 * struct kbase_ioctl_slsi_util_info - Read gpu utilization info
 */
struct mali_exynos_ioctl_gts_info {
	__u32 util_avg;
	__u32 hcm_mode;
	__u64 out_data[4];
	__u64 input;
	__u64 input2;
	__u64 freq;
	__u64 flimit;
};

#define MALI_EXYNOS_IOCTL_READ_GTS_INFO \
	_IOR(KBASE_IOCTL_EXTRA_TYPE, 1, struct mali_exynos_ioctl_gts_info)

struct mali_exynos_ioctl_hcm_pmqos {
	__u32 mode;
};

#define MALI_EXYNOS_IOCTL_HCM_PMQOS \
	_IOW(KBASE_IOCTL_EXTRA_TYPE, 2, struct mali_exynos_ioctl_hcm_pmqos)

/**
 * struct mali_exynos_ioctl_mem_usage_add - Provide gpu memory usage information to kernel
 * @gl_mem_usage: gpu memory used
 *
 * The data provided is accessible through a sysfs file
 */
struct mali_exynos_ioctl_mem_usage_add {
	__u64 gl_mem_usage;
};

#define MALI_EXYNOS_IOCTL_MEM_USAGE_ADD \
	_IOW(KBASE_IOCTL_EXTRA_TYPE, 3, struct mali_exynos_ioctl_mem_usage_add)

/*
 * DEFAULT: is in default sched (SCHED_NORMAL)
 * USER_RT: is in RT sched set by user
 * SET_RT: attempt to set to USER_RT
 * SET_DEFAULT: attempt to set back to default sched
 */
typedef enum cmar_boost_flag {
	CMAR_BOOST_DEFAULT = 1u << 0,
	CMAR_BOOST_USER_RT = 1u << 1,
	CMAR_BOOST_SET_RT = 1u << 2,
	CMAR_BOOST_SET_DEFAULT = 1u << 3,
} cmar_boost_flag;

/*
 * struct mali_exynos_ioctl_cmar_boost - Update status of cmar boost
 * @flags: stores cmar_boost_flag
 */
struct mali_exynos_ioctl_cmar_boost {
	__u32 flags;
};

#define MALI_EXYNOS_IOCTL_CMAR_BOOST \
	_IOW(KBASE_IOCTL_EXTRA_TYPE, 4, struct mali_exynos_ioctl_cmar_boost)

/* GPU Profiler - CONFIG_MALI_TSG */
struct kbase_ioctl_slsi_egp {
	__u64 start_timestamp;
	__u64 end_timestamp;
};

#define KBASE_IOCTL_SLSI_EGP \
       _IOW(KBASE_IOCTL_EXTRA_TYPE, 5, struct kbase_ioctl_slsi_egp)

/*** Legacy IOCTLs for backward compatibility ***/

/*
 * struct kbase_ioctl_slsi_singlebuffer_boost_flags - Update the status of singlebuffer boost flag
 * @flags: Flags for future expansion
 */
struct kbase_ioctl_slsi_singlebuffer_boost_flags {
	__u32 flags;
};

#define KBASE_IOCTL_SLSI_SINGLEBUFFER_BOOST_FLAGS \
	_IOW(KBASE_IOCTL_TYPE, 46, struct kbase_ioctl_slsi_singlebuffer_boost_flags)

#ifdef __cpluscplus
}
#endif

#endif /* _UAPI_MALI_EXYNOS_IOCTL_H_ */
