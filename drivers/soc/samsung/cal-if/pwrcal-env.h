#ifndef __PWRCAL_INCLUDE_H__
#define __PWRCAL_INCLUDE_H__

#ifdef CONFIG_CAL_IF

#define PWRCAL_TARGET_LINUX

#include <linux/spinlock.h>
#include <linux/math64.h>
#include <linux/smc.h>
#include <linux/delay.h>
#include <soc/samsung/exynos-el3_mon.h>

#else

#define PWRCAL_TARGET_FW

#include "types.h"
#include "console.h"
#include "kernel/spinlock.h"
#include <string.h>
#include <common.h>
#include <kernel/timer.h>
#include <kernel/panic.h>
#include <compat.h>

#define pr_err(_msg, args...)	\
	console_printf(0, "\033[1;31;5merror::func=%s, "_msg"\033[0m\n", \
							__func__, ##args);
#define pr_warn(_msg, args...)	\
	console_printf(1, "\033[1;31;5mwarning::func=%s, "_msg"\033[0m\n", \
							__func__, ##args);
#define pr_info(_msg, args...)	\
	console_printf(4, _msg, ##args)

#define do_div(a, b)		(a /= b)

#define spin_lock_init(x)	initialize_spinlock(x)
#define cpu_relax()		udelay(1)

#define INVALID_HWID		ULONG_MAX

#define MPIDR_UP_BITMASK	(0x1 << 30)
#define MPIDR_MT_BITMASK	(0x1 << 24)
#define MPIDR_HWID_BITMASK	0xff01ffffff

#define MPIDR_SMP_BITMASK	(0x3 << 30)
#define MPIDR_SMP_VALUE		(0x2 << 30)
#define MPIDR_MT_BITMASK	(0x1 << 24)

#define MPIDR_LEVEL_BITS_SHIFT	3
#define MPIDR_LEVEL_BITS	(1 << MPIDR_LEVEL_BITS_SHIFT)
#define MPIDR_LEVEL_MASK	((1 << MPIDR_LEVEL_BITS) - 1)

#define MPIDR_LEVEL_SHIFT(level) \
	(((1 << level) >> 1) << MPIDR_LEVEL_BITS_SHIFT)

#define MPIDR_AFFINITY_LEVEL(mpidr, level) \
	((mpidr >> MPIDR_LEVEL_SHIFT(level)) & MPIDR_LEVEL_MASK)

#endif

#endif
