/*
 * sec_debug_memtab.c
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/sec_debug.h>

#define DEFINE_MEMBER_TYPE	SECDBG_DEFINE_MEMBER_TYPE

extern struct secdbg_member_type __start__secdbg_member_table[];
extern struct secdbg_member_type __stop__secdbg_member_table[];

void __init secdbg_base_set_memtab_info(struct sec_debug_memtab *mtab)
{
	mtab->table_start_pa = __pa(__start__secdbg_member_table);
	mtab->table_end_pa = __pa(__stop__secdbg_member_table);
}

DEFINE_MEMBER_TYPE(task_struct_pid, task_struct, pid);
DEFINE_MEMBER_TYPE(task_struct_comm, task_struct, comm);
DEFINE_MEMBER_TYPE(thread_info_flags, thread_info, flags);
DEFINE_MEMBER_TYPE(task_struct_state, task_struct, state);
DEFINE_MEMBER_TYPE(task_struct_exit_state, task_struct, exit_state);
DEFINE_MEMBER_TYPE(task_struct_stack, task_struct, stack);
DEFINE_MEMBER_TYPE(task_struct_flags, task_struct, flags);
DEFINE_MEMBER_TYPE(task_struct_on_cpu, task_struct, on_cpu);
DEFINE_MEMBER_TYPE(task_struct_on_rq, task_struct, on_rq);
DEFINE_MEMBER_TYPE(task_struct_sched_class, task_struct, sched_class);
DEFINE_MEMBER_TYPE(task_struct_cpu, task_struct, cpu);
#ifdef CONFIG_SEC_DEBUG_DTASK
DEFINE_MEMBER_TYPE(task_struct_ssdbg_wait__type, task_struct, ssdbg_wait.type);
DEFINE_MEMBER_TYPE(task_struct_ssdbg_wait__data, task_struct, ssdbg_wait.data);
#endif
#ifdef CONFIG_DEBUG_SPINLOCK
DEFINE_MEMBER_TYPE(raw_spinlock_owner_cpu, raw_spinlock, owner_cpu);
DEFINE_MEMBER_TYPE(raw_spinlock_owner, raw_spinlock, owner);
#endif
#ifdef CONFIG_RWSEM_SPIN_ON_OWNER
DEFINE_MEMBER_TYPE(rw_semaphore_owner, rw_semaphore, owner);
#endif
DEFINE_MEMBER_TYPE(task_struct_cpus_allowed, task_struct, cpus_allowed);
DEFINE_MEMBER_TYPE(task_struct_normal_prio, task_struct, normal_prio);
DEFINE_MEMBER_TYPE(task_struct_rt_priority, task_struct, rt_priority);
#ifdef CONFIG_FAST_TRACK
DEFINE_MEMBER_TYPE(task_struct_se__ftt_mark, task_struct, se.ftt_mark);
#endif
DEFINE_MEMBER_TYPE(task_struct_se__vruntime, task_struct, se.vruntime);
DEFINE_MEMBER_TYPE(task_struct_se__avg__load_avg, task_struct, se.avg.load_avg);
DEFINE_MEMBER_TYPE(task_struct_se__avg__util_avg, task_struct, se.avg.util_avg);
DEFINE_MEMBER_TYPE(task_struct_rt__avg__load_avg, task_struct, rt.avg.load_avg);
DEFINE_MEMBER_TYPE(task_struct_rt__avg__util_avg, task_struct, rt.avg.util_avg);
