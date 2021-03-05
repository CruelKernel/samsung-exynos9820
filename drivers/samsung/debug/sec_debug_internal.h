/*
 * drivers/staging/samsung/sec_debug_internal.h
 *
 * COPYRIGHT(C) 2017 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __SEC_DEBUG_INTERNAL_H__
#define __SEC_DEBUG_INTERNAL_H__

/* [[BEGIN>> sec_debug.c */
extern void dump_all_task_info(void);
extern void dump_cpu_stat(void);
/* <<END]] sec_debug.c */

/* [[BEGIN>> sec_debug_sched_log.c */
extern struct sec_debug_log *secdbg_log;
extern phys_addr_t secdbg_paddr;
extern size_t secdbg_size;
/* <<END]] sec_debug_sched_log.c */

/* out-of-sec_debug */

/* drivers/power/reset/msm-poweroff.c */
extern void set_dload_mode(int on);

/* kernel/init/version.c */
#include <linux/utsname.h>
extern struct uts_namespace init_uts_ns;

/* sec_debug_reset_reason.c */
extern int secdbg_rere_get_rstcnt_from_cmdline(void);
#endif /* __SEC_DEBUG_INTERNAL_H__ */
