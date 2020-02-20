/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM power

#if !defined(_TRACE_POWER_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_POWER_H

#include <linux/ktime.h>
#include <linux/pm_qos.h>
#include <linux/tracepoint.h>
#include <linux/trace_events.h>

#define TPS(x)  tracepoint_string(x)

DECLARE_EVENT_CLASS(cpu,

	TP_PROTO(unsigned int state, unsigned int cpu_id),

	TP_ARGS(state, cpu_id),

	TP_STRUCT__entry(
		__field(	u32,		state		)
		__field(	u32,		cpu_id		)
	),

	TP_fast_assign(
		__entry->state = state;
		__entry->cpu_id = cpu_id;
	),

	TP_printk("state=%lu cpu_id=%lu", (unsigned long)__entry->state,
		  (unsigned long)__entry->cpu_id)
);

DEFINE_EVENT(cpu, cpupm,

	TP_PROTO(unsigned int state, unsigned int cpu_id),

	TP_ARGS(state, cpu_id)
);

DEFINE_EVENT(cpu, cpu_idle,

	TP_PROTO(unsigned int state, unsigned int cpu_id),

	TP_ARGS(state, cpu_id)
);

TRACE_EVENT(sugov_slack_func,

	TP_PROTO(int cpu),

	TP_ARGS(cpu),

	TP_STRUCT__entry(
		__field(int, cpu)
	),

	TP_fast_assign(
		__entry->cpu = cpu;
	),

	TP_printk("cpu=%d SLACK EXPIRED", __entry->cpu)
);

TRACE_EVENT(sugov_slack,

	TP_PROTO(int cpu, unsigned long util,
		unsigned long min, unsigned long action, int ret),

	TP_ARGS(cpu, util, min, action, ret),

	TP_STRUCT__entry(
		__field(int, cpu)
		__field(unsigned long, util)
		__field(unsigned long, min)
		__field(unsigned long, action)
		__field(int, ret)
	),

	TP_fast_assign(
		__entry->cpu = cpu;
		__entry->util = util;
		__entry->min = min;
		__entry->action = action;
		__entry->ret = ret;
	),

	TP_printk("cpu=%d util=%ld min=%ld action=%ld ret=%d", __entry->cpu,
			__entry->util, __entry->min, __entry->action, __entry->ret)
);

TRACE_EVENT(powernv_throttle,

	TP_PROTO(int chip_id, const char *reason, int pmax),

	TP_ARGS(chip_id, reason, pmax),

	TP_STRUCT__entry(
		__field(int, chip_id)
		__string(reason, reason)
		__field(int, pmax)
	),

	TP_fast_assign(
		__entry->chip_id = chip_id;
		__assign_str(reason, reason);
		__entry->pmax = pmax;
	),

	TP_printk("Chip %d Pmax %d %s", __entry->chip_id,
		  __entry->pmax, __get_str(reason))
);

TRACE_EVENT(pstate_sample,

	TP_PROTO(u32 core_busy,
		u32 scaled_busy,
		u32 from,
		u32 to,
		u64 mperf,
		u64 aperf,
		u64 tsc,
		u32 freq,
		u32 io_boost
		),

	TP_ARGS(core_busy,
		scaled_busy,
		from,
		to,
		mperf,
		aperf,
		tsc,
		freq,
		io_boost
		),

	TP_STRUCT__entry(
		__field(u32, core_busy)
		__field(u32, scaled_busy)
		__field(u32, from)
		__field(u32, to)
		__field(u64, mperf)
		__field(u64, aperf)
		__field(u64, tsc)
		__field(u32, freq)
		__field(u32, io_boost)
		),

	TP_fast_assign(
		__entry->core_busy = core_busy;
		__entry->scaled_busy = scaled_busy;
		__entry->from = from;
		__entry->to = to;
		__entry->mperf = mperf;
		__entry->aperf = aperf;
		__entry->tsc = tsc;
		__entry->freq = freq;
		__entry->io_boost = io_boost;
		),

	TP_printk("core_busy=%lu scaled=%lu from=%lu to=%lu mperf=%llu aperf=%llu tsc=%llu freq=%lu io_boost=%lu",
		(unsigned long)__entry->core_busy,
		(unsigned long)__entry->scaled_busy,
		(unsigned long)__entry->from,
		(unsigned long)__entry->to,
		(unsigned long long)__entry->mperf,
		(unsigned long long)__entry->aperf,
		(unsigned long long)__entry->tsc,
		(unsigned long)__entry->freq,
		(unsigned long)__entry->io_boost
		)

);

/* This file can get included multiple times, TRACE_HEADER_MULTI_READ at top */
#ifndef _PWR_EVENT_AVOID_DOUBLE_DEFINING
#define _PWR_EVENT_AVOID_DOUBLE_DEFINING

#define PWR_EVENT_EXIT -1
#endif

#define pm_verb_symbolic(event) \
	__print_symbolic(event, \
		{ PM_EVENT_SUSPEND, "suspend" }, \
		{ PM_EVENT_RESUME, "resume" }, \
		{ PM_EVENT_FREEZE, "freeze" }, \
		{ PM_EVENT_QUIESCE, "quiesce" }, \
		{ PM_EVENT_HIBERNATE, "hibernate" }, \
		{ PM_EVENT_THAW, "thaw" }, \
		{ PM_EVENT_RESTORE, "restore" }, \
		{ PM_EVENT_RECOVER, "recover" })

DEFINE_EVENT(cpu, cpu_frequency,

	TP_PROTO(unsigned int frequency, unsigned int cpu_id),

	TP_ARGS(frequency, cpu_id)
);

TRACE_EVENT(cpu_frequency_limits,

	TP_PROTO(unsigned int max_freq, unsigned int min_freq,
		unsigned int cpu_id),

	TP_ARGS(max_freq, min_freq, cpu_id),

	TP_STRUCT__entry(
		__field(	u32,		min_freq	)
		__field(	u32,		max_freq	)
		__field(	u32,		cpu_id		)
	),

	TP_fast_assign(
		__entry->min_freq = min_freq;
		__entry->max_freq = max_freq;
		__entry->cpu_id = cpu_id;
	),

	TP_printk("min=%lu max=%lu cpu_id=%lu",
		  (unsigned long)__entry->min_freq,
		  (unsigned long)__entry->max_freq,
		  (unsigned long)__entry->cpu_id)
);

TRACE_EVENT(cpu_frequency_filter,

	TP_PROTO(unsigned int target_freq, unsigned int req_type),

	TP_ARGS(target_freq, req_type),

	TP_STRUCT__entry(
		__field(	u32,		target_freq	)
		__field(	u32,		req_type	)
	),

	TP_fast_assign(
		__entry->target_freq = target_freq;
		__entry->req_type = req_type;
	),

	TP_printk("eff: target=%lu req_type=%lu",
		  (unsigned long)__entry->target_freq,
		  (unsigned long)__entry->req_type)
);

TRACE_EVENT(sugov_kair_freq,
	    TP_PROTO(unsigned int cpu, unsigned long util, unsigned long max,
		     int l1_rand, unsigned int legacy_freq, unsigned int freq),
	    TP_ARGS(cpu, util, max, l1_rand, legacy_freq, freq),
	    TP_STRUCT__entry(
		    __field(	unsigned int,	cpu)
		    __field(	unsigned long,	util)
		    __field(	unsigned long,	max)
		    __field(	int,		l1_rand)
		    __field(	unsigned int,	legacy_freq)
		    __field(	unsigned int,	freq)
	    ),
	    TP_fast_assign(
		    __entry->cpu = cpu;
		    __entry->util = util;
		    __entry->max = max;
		    __entry->l1_rand = l1_rand;
		    __entry->legacy_freq = legacy_freq;
		    __entry->freq = freq;
	    ),
	    TP_printk("cpu=%u util=%lu max=%lu l1_rand=%d legacy_freq=%u kair_freq=%u",
		      __entry->cpu,
		      __entry->util,
		      __entry->max,
		      __entry->l1_rand,
		      __entry->legacy_freq,
		      __entry->freq)
);

TRACE_EVENT(cpu_frequency_sugov,

	TP_PROTO(unsigned int freq, unsigned long util, unsigned int cpu_id),

	TP_ARGS(freq, util, cpu_id),

	TP_STRUCT__entry(
		__field(	u32,		freq	)
		__field(	u32,		util	)
		__field(	u32,		cpu_id	)
	),

	TP_fast_assign(
		__entry->freq = freq;
		__entry->util = util;
		__entry->cpu_id = cpu_id;
	),

	TP_printk("freq=%lu util=%lu cpu_id=%lu",
		  (unsigned long)__entry->freq,
		  (unsigned long)__entry->util,
		  (unsigned long)__entry->cpu_id)
);

TRACE_EVENT(device_pm_callback_start,

	TP_PROTO(struct device *dev, const char *pm_ops, int event),

	TP_ARGS(dev, pm_ops, event),

	TP_STRUCT__entry(
		__string(device, dev_name(dev))
		__string(driver, dev_driver_string(dev))
		__string(parent, dev->parent ? dev_name(dev->parent) : "none")
		__string(pm_ops, pm_ops ? pm_ops : "none ")
		__field(int, event)
	),

	TP_fast_assign(
		__assign_str(device, dev_name(dev));
		__assign_str(driver, dev_driver_string(dev));
		__assign_str(parent,
			dev->parent ? dev_name(dev->parent) : "none");
		__assign_str(pm_ops, pm_ops ? pm_ops : "none ");
		__entry->event = event;
	),

	TP_printk("%s %s, parent: %s, %s[%s]", __get_str(driver),
		__get_str(device), __get_str(parent), __get_str(pm_ops),
		pm_verb_symbolic(__entry->event))
);

TRACE_EVENT(device_pm_callback_end,

	TP_PROTO(struct device *dev, int error),

	TP_ARGS(dev, error),

	TP_STRUCT__entry(
		__string(device, dev_name(dev))
		__string(driver, dev_driver_string(dev))
		__field(int, error)
	),

	TP_fast_assign(
		__assign_str(device, dev_name(dev));
		__assign_str(driver, dev_driver_string(dev));
		__entry->error = error;
	),

	TP_printk("%s %s, err=%d",
		__get_str(driver), __get_str(device), __entry->error)
);

TRACE_EVENT(suspend_resume,

	TP_PROTO(const char *action, int val, bool start),

	TP_ARGS(action, val, start),

	TP_STRUCT__entry(
		__field(const char *, action)
		__field(int, val)
		__field(bool, start)
	),

	TP_fast_assign(
		__entry->action = action;
		__entry->val = val;
		__entry->start = start;
	),

	TP_printk("%s[%u] %s", __entry->action, (unsigned int)__entry->val,
		(__entry->start)?"begin":"end")
);

DECLARE_EVENT_CLASS(wakeup_source,

	TP_PROTO(const char *name, unsigned int state),

	TP_ARGS(name, state),

	TP_STRUCT__entry(
		__string(       name,           name            )
		__field(        u64,            state           )
	),

	TP_fast_assign(
		__assign_str(name, name);
		__entry->state = state;
	),

	TP_printk("%s state=0x%lx", __get_str(name),
		(unsigned long)__entry->state)
);

DEFINE_EVENT(wakeup_source, wakeup_source_activate,

	TP_PROTO(const char *name, unsigned int state),

	TP_ARGS(name, state)
);

DEFINE_EVENT(wakeup_source, wakeup_source_deactivate,

	TP_PROTO(const char *name, unsigned int state),

	TP_ARGS(name, state)
);

/*
 * The clock events are used for clock enable/disable and for
 *  clock rate change
 */
DECLARE_EVENT_CLASS(clock,

	TP_PROTO(const char *name, unsigned int state, unsigned int cpu_id),

	TP_ARGS(name, state, cpu_id),

	TP_STRUCT__entry(
		__string(       name,           name            )
		__field(        u64,            state           )
		__field(        u64,            cpu_id          )
	),

	TP_fast_assign(
		__assign_str(name, name);
		__entry->state = state;
		__entry->cpu_id = cpu_id;
	),

	TP_printk("%s state=%lu cpu_id=%lu", __get_str(name),
		(unsigned long)__entry->state, (unsigned long)__entry->cpu_id)
);

DEFINE_EVENT(clock, clock_enable,

	TP_PROTO(const char *name, unsigned int state, unsigned int cpu_id),

	TP_ARGS(name, state, cpu_id)
);

DEFINE_EVENT(clock, clock_disable,

	TP_PROTO(const char *name, unsigned int state, unsigned int cpu_id),

	TP_ARGS(name, state, cpu_id)
);

DEFINE_EVENT(clock, clock_set_rate,

	TP_PROTO(const char *name, unsigned int state, unsigned int cpu_id),

	TP_ARGS(name, state, cpu_id)
);

TRACE_EVENT(clock_set_parent,

	TP_PROTO(const char *name, const char *parent_name),

	TP_ARGS(name, parent_name),

	TP_STRUCT__entry(
		__string(       name,           name            )
		__string(       parent_name,    parent_name     )
	),

	TP_fast_assign(
		__assign_str(name, name);
		__assign_str(parent_name, parent_name);
	),

	TP_printk("%s parent=%s", __get_str(name), __get_str(parent_name))
);

/*
 * The power domain events are used for power domains transitions
 */
DECLARE_EVENT_CLASS(power_domain,

	TP_PROTO(const char *name, unsigned int state, unsigned int cpu_id),

	TP_ARGS(name, state, cpu_id),

	TP_STRUCT__entry(
		__string(       name,           name            )
		__field(        u64,            state           )
		__field(        u64,            cpu_id          )
	),

	TP_fast_assign(
		__assign_str(name, name);
		__entry->state = state;
		__entry->cpu_id = cpu_id;
),

	TP_printk("%s state=%lu cpu_id=%lu", __get_str(name),
		(unsigned long)__entry->state, (unsigned long)__entry->cpu_id)
);

DEFINE_EVENT(power_domain, power_domain_target,

	TP_PROTO(const char *name, unsigned int state, unsigned int cpu_id),

	TP_ARGS(name, state, cpu_id)
);

/*
 * The pm qos events are used for pm qos update
 */
DECLARE_EVENT_CLASS(pm_qos_request,

	TP_PROTO(int pm_qos_class, s32 value),

	TP_ARGS(pm_qos_class, value),

	TP_STRUCT__entry(
		__field( int,                    pm_qos_class   )
		__field( s32,                    value          )
	),

	TP_fast_assign(
		__entry->pm_qos_class = pm_qos_class;
		__entry->value = value;
	),

	TP_printk("pm_qos_class=%s value=%d",
		  __print_symbolic(__entry->pm_qos_class,
			{ PM_QOS_CPU_DMA_LATENCY,	"CPU_DMA_LATENCY" },
			{ PM_QOS_NETWORK_LATENCY,	"NETWORK_LATENCY" },
			{ PM_QOS_NETWORK_THROUGHPUT,	"NETWORK_THROUGHPUT" }),
		  __entry->value)
);

DEFINE_EVENT(pm_qos_request, pm_qos_add_request,

	TP_PROTO(int pm_qos_class, s32 value),

	TP_ARGS(pm_qos_class, value)
);

DEFINE_EVENT(pm_qos_request, pm_qos_update_request,

	TP_PROTO(int pm_qos_class, s32 value),

	TP_ARGS(pm_qos_class, value)
);

DEFINE_EVENT(pm_qos_request, pm_qos_remove_request,

	TP_PROTO(int pm_qos_class, s32 value),

	TP_ARGS(pm_qos_class, value)
);

TRACE_EVENT(pm_qos_update_request_timeout,

	TP_PROTO(int pm_qos_class, s32 value, unsigned long timeout_us),

	TP_ARGS(pm_qos_class, value, timeout_us),

	TP_STRUCT__entry(
		__field( int,                    pm_qos_class   )
		__field( s32,                    value          )
		__field( unsigned long,          timeout_us     )
	),

	TP_fast_assign(
		__entry->pm_qos_class = pm_qos_class;
		__entry->value = value;
		__entry->timeout_us = timeout_us;
	),

	TP_printk("pm_qos_class=%s value=%d, timeout_us=%ld",
		  __print_symbolic(__entry->pm_qos_class,
			{ PM_QOS_CPU_DMA_LATENCY,	"CPU_DMA_LATENCY" },
			{ PM_QOS_NETWORK_LATENCY,	"NETWORK_LATENCY" },
			{ PM_QOS_NETWORK_THROUGHPUT,	"NETWORK_THROUGHPUT" }),
		  __entry->value, __entry->timeout_us)
);

DECLARE_EVENT_CLASS(pm_qos_update,

	TP_PROTO(enum pm_qos_req_action action, int prev_value, int curr_value),

	TP_ARGS(action, prev_value, curr_value),

	TP_STRUCT__entry(
		__field( enum pm_qos_req_action, action         )
		__field( int,                    prev_value     )
		__field( int,                    curr_value     )
	),

	TP_fast_assign(
		__entry->action = action;
		__entry->prev_value = prev_value;
		__entry->curr_value = curr_value;
	),

	TP_printk("action=%s prev_value=%d curr_value=%d",
		  __print_symbolic(__entry->action,
			{ PM_QOS_ADD_REQ,	"ADD_REQ" },
			{ PM_QOS_UPDATE_REQ,	"UPDATE_REQ" },
			{ PM_QOS_REMOVE_REQ,	"REMOVE_REQ" }),
		  __entry->prev_value, __entry->curr_value)
);

DEFINE_EVENT(pm_qos_update, pm_qos_update_target,

	TP_PROTO(enum pm_qos_req_action action, int prev_value, int curr_value),

	TP_ARGS(action, prev_value, curr_value)
);

DEFINE_EVENT_PRINT(pm_qos_update, pm_qos_update_flags,

	TP_PROTO(enum pm_qos_req_action action, int prev_value, int curr_value),

	TP_ARGS(action, prev_value, curr_value),

	TP_printk("action=%s prev_value=0x%x curr_value=0x%x",
		  __print_symbolic(__entry->action,
			{ PM_QOS_ADD_REQ,	"ADD_REQ" },
			{ PM_QOS_UPDATE_REQ,	"UPDATE_REQ" },
			{ PM_QOS_REMOVE_REQ,	"REMOVE_REQ" }),
		  __entry->prev_value, __entry->curr_value)
);

DECLARE_EVENT_CLASS(dev_pm_qos_request,

	TP_PROTO(const char *name, enum dev_pm_qos_req_type type,
		 s32 new_value),

	TP_ARGS(name, type, new_value),

	TP_STRUCT__entry(
		__string( name,                    name         )
		__field( enum dev_pm_qos_req_type, type         )
		__field( s32,                      new_value    )
	),

	TP_fast_assign(
		__assign_str(name, name);
		__entry->type = type;
		__entry->new_value = new_value;
	),

	TP_printk("device=%s type=%s new_value=%d",
		  __get_str(name),
		  __print_symbolic(__entry->type,
			{ DEV_PM_QOS_RESUME_LATENCY, "DEV_PM_QOS_RESUME_LATENCY" },
			{ DEV_PM_QOS_FLAGS, "DEV_PM_QOS_FLAGS" }),
		  __entry->new_value)
);

DEFINE_EVENT(dev_pm_qos_request, dev_pm_qos_add_request,

	TP_PROTO(const char *name, enum dev_pm_qos_req_type type,
		 s32 new_value),

	TP_ARGS(name, type, new_value)
);

DEFINE_EVENT(dev_pm_qos_request, dev_pm_qos_update_request,

	TP_PROTO(const char *name, enum dev_pm_qos_req_type type,
		 s32 new_value),

	TP_ARGS(name, type, new_value)
);

DEFINE_EVENT(dev_pm_qos_request, dev_pm_qos_remove_request,

	TP_PROTO(const char *name, enum dev_pm_qos_req_type type,
		 s32 new_value),

	TP_ARGS(name, type, new_value)
);

TRACE_EVENT(ocp_max_limit,

	TP_PROTO(unsigned int clipped_freq, bool start),

	TP_ARGS(clipped_freq, start),

	TP_STRUCT__entry(
		__field(	u32,		clipped_freq	)
		__field(	bool,		start	)
	),

	TP_fast_assign(
		__entry->clipped_freq = clipped_freq;
		__entry->start = start;
	),

	TP_printk("clipped_freq=%lu %s",
		  (unsigned long)__entry->clipped_freq,
		  (__entry->start)?"begin":"end")
);

TRACE_EVENT(emc_cpu_load,

	TP_PROTO(int cpu, int load, int max_load),

	TP_ARGS(cpu, load, max_load),

	TP_STRUCT__entry(
		__field(int, cpu)
		__field(int, load)
		__field(int, max_load)
	),

	TP_fast_assign(
		__entry->cpu = cpu;
		__entry->load = load;
		__entry->max_load = max_load;
	),

	TP_printk("cpu=%d load=%d, max_load=%d",
		__entry->cpu, __entry->load, __entry->max_load)
);

TRACE_EVENT(emc_domain_load,

	TP_PROTO(const char *domain, int load, int max_load),

	TP_ARGS(domain, load, max_load),

	TP_STRUCT__entry(
		__string(domain, domain)
		__field(int, load)
		__field(int, max_load)
	),

	TP_fast_assign(
		__assign_str(domain, domain);
		__entry->load = load;
		__entry->max_load = max_load;
	),

	TP_printk("domain:%s load_sum=%d, max_load_sum=%d",
		__get_str(domain), __entry->load, __entry->max_load)
);

TRACE_EVENT(emc_domain_status,

	TP_PROTO(const char *domain, unsigned int sys_heavy, unsigned int sys_busy,
			unsigned int dom_heavy, unsigned int dom_busy),

	TP_ARGS(domain, sys_heavy, sys_busy, dom_heavy, dom_busy),

	TP_STRUCT__entry(
		__string(domain, domain)
		__field(unsigned int, sys_heavy)
		__field(unsigned int, sys_busy)
		__field(unsigned int, dom_heavy)
		__field(unsigned int, dom_busy)
	),

	TP_fast_assign(
		__assign_str(domain, domain);
		__entry->sys_heavy = sys_heavy;
		__entry->sys_busy = sys_busy;
		__entry->dom_heavy = dom_heavy;
		__entry->dom_busy = dom_busy;
	),

	TP_printk("domain:%s s_heavy =%x, s_busy=%x, d_heavy=%x, d_busy=%x",
			__get_str(domain), __entry->sys_heavy, __entry->sys_busy,
			__entry->dom_heavy, __entry->dom_busy)
);

TRACE_EVENT(emc_update_system_status,

	TP_PROTO(unsigned int prev_heavy, unsigned int prev_busy,
			unsigned int heavy, unsigned int busy),

	TP_ARGS(prev_heavy, prev_busy, heavy, busy),

	TP_STRUCT__entry(
		__field(unsigned int, prev_heavy)
		__field(unsigned int, prev_busy)
		__field(unsigned int, heavy)
		__field(unsigned int, busy)
	),

	TP_fast_assign(
		__entry->prev_heavy = prev_heavy;
		__entry->prev_busy = prev_busy;
		__entry->heavy = heavy;
		__entry->busy = busy;
	),

	TP_printk("prev_heavy=%x, prev_busy=%x, heavy=%x busy=%x",
			__entry->prev_heavy, __entry->prev_busy,
			__entry->heavy, __entry->busy)
);

TRACE_EVENT(emc_select_mode,

	TP_PROTO(const char *mode, int need_online_cnt),

	TP_ARGS(mode, need_online_cnt),

	TP_STRUCT__entry(
		__string(mode, mode)
		__field(int, need_online_cnt)
	),

	TP_fast_assign(
		__assign_str(mode, mode);
		__entry->need_online_cnt = need_online_cnt;
	),

	TP_printk("mode:%s need_online_cnt=%d",
		__get_str(mode), __entry->need_online_cnt)
);

TRACE_EVENT(emc_domain_busy,

	TP_PROTO(const char *domain, unsigned long load, int busy),

	TP_ARGS(domain, load, busy),

	TP_STRUCT__entry(
		__string(domain, domain)
		__field(unsigned long, load)
		__field(int, busy)
	),

	TP_fast_assign(
		__assign_str(domain, domain);
		__entry->load = load;
		__entry->busy = busy;
	),

	TP_printk("domain=%s, load=%ld, busy=%d",
			__get_str(domain), __entry->load, __entry->busy)
);

TRACE_EVENT(emc_start_timer,

	TP_PROTO(const char *mode, unsigned int change_latency),

	TP_ARGS(mode, change_latency),

	TP_STRUCT__entry(
		__string(mode, mode)
		__field(unsigned int, change_latency)
	),

	TP_fast_assign(
		__assign_str(mode, mode);
		__entry->change_latency = change_latency;
	),

	TP_printk("mode=%s, change_latency=%d",
			__get_str(mode), __entry->change_latency)
);

TRACE_EVENT(emc_do_mode_change,

	TP_PROTO(const char *prev_mode, const char *new_mode, unsigned int event),

	TP_ARGS(prev_mode, new_mode, event),

	TP_STRUCT__entry(
		__string(prev_mode, prev_mode)
		__string(new_mode, new_mode)
		__field(unsigned int, event)
	),

	TP_fast_assign(
		__assign_str(prev_mode, prev_mode);
		__assign_str(new_mode, new_mode);
		__entry->event = event;
	),

	TP_printk("mode: %s->%s (event 0x%x)", __get_str(prev_mode),
					__get_str(new_mode), __entry->event)
);

TRACE_EVENT(emc_update_cpu_pwr,

	TP_PROTO(unsigned int pre_cpu_mask, unsigned int cpu, unsigned int on),

	TP_ARGS(pre_cpu_mask, cpu, on),

	TP_STRUCT__entry(
		__field(unsigned int, pre_cpu_mask)
		__field(unsigned int, cpu)
		__field(unsigned int, on)
	),

	TP_fast_assign(
		__entry->pre_cpu_mask = pre_cpu_mask;
		__entry->cpu = cpu;
		__entry->on = on;
	),

	TP_printk("pre_cpu_mask=%x, cpu=%u, on=%s", __entry->pre_cpu_mask,
					__entry->cpu, __entry->on? "ON" : "OFF")
);

TRACE_EVENT(cpus_up_enter,

	TP_PROTO(int cpu),

	TP_ARGS(cpu),

	TP_STRUCT__entry(
		__field(int, cpu)
	),

	TP_fast_assign(
		__entry->cpu = cpu;
	),

	TP_printk("enter cpus_up cpu%d", __entry->cpu)
);

TRACE_EVENT(cpus_up_exit,

	TP_PROTO(int cpu),

	TP_ARGS(cpu),

	TP_STRUCT__entry(
		__field(int, cpu)
	),

	TP_fast_assign(
		__entry->cpu = cpu;
	),

	TP_printk("exit cpus_up cpu%d", __entry->cpu)
);

TRACE_EVENT(cpus_down_enter,

	TP_PROTO(int cpu),

	TP_ARGS(cpu),

	TP_STRUCT__entry(
		__field(int, cpu)
	),

	TP_fast_assign(
		__entry->cpu = cpu;
	),

	TP_printk("enter cpus_down cpu%d", __entry->cpu)
);

TRACE_EVENT(cpus_down_exit,

	TP_PROTO(int cpu),

	TP_ARGS(cpu),

	TP_STRUCT__entry(
		__field(int, cpu)
	),

	TP_fast_assign(
		__entry->cpu = cpu;
	),

	TP_printk("exit cpus_down cpu%d", __entry->cpu)
);

#endif /* _TRACE_POWER_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
