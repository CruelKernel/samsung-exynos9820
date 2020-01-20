/*
 * drivers/media/video/exynos/fimc-is-mc2/fimc-is-interface.h
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * The header file related to camera
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_INTERFACE_H
#define FIMC_IS_INTERFACE_H
#include "fimc-is-metadata.h"
#include "fimc-is-framemgr.h"
#include "fimc-is-video.h"
#include "fimc-is-time.h"
#include "fimc-is-cmd.h"

/*#define TRACE_WORK*/
/* cam_ctrl : 1
   shot :     2 */
#define TRACE_WORK_ID_CAMCTRL	0x0000001
#define TRACE_WORK_ID_GENERAL	0x0000002
#define TRACE_WORK_ID_SHOT	0x0000004
#define TRACE_WORK_ID_30C	0x0000008
#define TRACE_WORK_ID_30P	0x0000010
#define TRACE_WORK_ID_30F	0x0000011
#define TRACE_WORK_ID_30G	0x0000012
#define TRACE_WORK_ID_31C	0x0000020
#define TRACE_WORK_ID_31P	0x0000040
#define TRACE_WORK_ID_31F	0x0000041
#define TRACE_WORK_ID_31G	0x0000042
#define TRACE_WORK_ID_I0C	0x0000080
#define TRACE_WORK_ID_I0P	0x0000100
#define TRACE_WORK_ID_I1C	0x0000200
#define TRACE_WORK_ID_I1P	0x0000400
#define TRACE_WORK_ID_D0C	0x0000800
#define TRACE_WORK_ID_D1C	0x0001000
#define TRACE_WORK_ID_DC1S	0x0002000
#define TRACE_WORK_ID_DC0C	0x0004000
#define TRACE_WORK_ID_DC1C	0x0008000
#define TRACE_WORK_ID_DC2C	0x0010000
#define TRACE_WORK_ID_DC3C	0x0020000
#define TRACE_WORK_ID_DC4C	0x0040000
#define TRACE_WORK_ID_M0P	0x0080000
#define TRACE_WORK_ID_M1P	0x0100000
#define TRACE_WORK_ID_M2P	0x0200000
#define TRACE_WORK_ID_M3P	0x0400000
#define TRACE_WORK_ID_M4P	0x0800000
#define TRACE_WORK_ID_M5P	0x1000000
#define TRACE_WORK_ID_ME0C	0x2000000
#define TRACE_WORK_ID_ME1C	0x4000000
#define TRACE_WORK_ID_32P	0x8000000
#define TRACE_WORK_ID_MASK	0xFFFFFFF

#define MAX_NBLOCKING_COUNT	3
#define MAX_WORK_COUNT		10

#define TRY_TIMEOUT_COUNT	2
#define SENSOR_TIMEOUT_COUNT	2
#define TRY_RECV_AWARE_COUNT	10000

#define LOWBIT_OF(num)	(num >= 32 ? 0 : (u32)1<<num)
#define HIGHBIT_OF(num)	(num >= 32 ? (u32)1<<(num-32) : 0)

enum fimc_is_interface_state {
	IS_IF_STATE_OPEN,
	IS_IF_STATE_READY,
	IS_IF_STATE_START,
	IS_IF_STATE_BUSY,
	IS_IF_STATE_LOGGING
};

#if defined(ENABLE_IS_CORE)
enum interrupt_map {
	INTR_GENERAL,
	INTR_SHOT_DONE,
	INTR_30C_FDONE,
	INTR_30P_FDONE,
	INTR_31C_FDONE,
	INTR_31P_FDONE,
	INTR_I0X_FDONE,
	INTR_I1X_FDONE,
	INTR_SCX_FDONE,
	INTR_M0X_FDONE,
	INTR_M1X_FDONE,
	INTR_MAX_MAP
};
#endif

enum work_map {
#if defined(ENABLE_IS_CORE)
	WORK_GENERAL,
#endif
	WORK_SHOT_DONE,
	WORK_30C_FDONE,
	WORK_30P_FDONE,
	WORK_30F_FDONE,
	WORK_30G_FDONE,
	WORK_31C_FDONE,
	WORK_31P_FDONE,
	WORK_31F_FDONE,
	WORK_31G_FDONE,
	WORK_32P_FDONE,
	WORK_I0C_FDONE,
	WORK_I0P_FDONE,
	WORK_I1C_FDONE,
	WORK_I1P_FDONE,
	WORK_ME0C_FDONE,	/* ME */
	WORK_ME1C_FDONE,	/* ME */
	WORK_D0C_FDONE,	/* TPU0 */
	WORK_D1C_FDONE,	/* TPU1 */
	WORK_DC1S_FDONE, /* DCP Master Input */
	WORK_DC0C_FDONE, /* DCP Master Capture */
	WORK_DC1C_FDONE, /* DCP Slave Capture */
	WORK_DC2C_FDONE, /* DCP Disparity Capture */
	WORK_DC3C_FDONE, /* DCP Master Sub Capture */
	WORK_DC4C_FDONE, /* DCP Slave Sub Capture */
	WORK_M0P_FDONE,
	WORK_M1P_FDONE,
	WORK_M2P_FDONE,
	WORK_M3P_FDONE,
	WORK_M4P_FDONE,
	WORK_M5P_FDONE,
	WORK_MAX_MAP
};

enum streaming_state {
	IS_IF_STREAMING_INIT,
	IS_IF_STREAMING_OFF,
	IS_IF_STREAMING_ON
};

enum processing_state {
	IS_IF_PROCESSING_INIT,
	IS_IF_PROCESSING_OFF,
	IS_IF_PROCESSING_ON
};

enum pdown_ready_state {
	IS_IF_POWER_DOWN_READY,
	IS_IF_POWER_DOWN_NREADY
};

enum launch_state {
	IS_IF_LAUNCH_FIRST,
	IS_IF_LAUNCH_SUCCESS,
};

enum fw_boot_state {
	IS_IF_RESUME,
	IS_IF_SUSPEND,
};

enum fimc_is_fw_boot {
	FIRST_LAUNCHING,
	WARM_BOOT,
	COLD_BOOT,
};

struct fimc_is_msg {
	u32	id;
	u32	command;
	u32	instance;
	u32	group;
	u32	param1;
	u32	param2;
	u32	param3;
	u32	param4;
};

struct fimc_is_work {
	struct list_head		list;
	struct fimc_is_msg		msg;
	u32				fcount;
	struct fimc_is_frame		*frame;
};

struct fimc_is_work_list {
	u32				id;
	struct fimc_is_work		work[MAX_WORK_COUNT];
	spinlock_t			slock_free;
	spinlock_t			slock_request;
	struct list_head		work_free_head;
	u32				work_free_cnt;
	struct list_head		work_request_head;
	u32				work_request_cnt;
	wait_queue_head_t		wait_queue;
};

struct fimc_is_interface {
	void __iomem			*regs;
	struct is_common_reg __iomem	*com_regs;
	unsigned long			state;
	spinlock_t			process_barrier;
	struct fimc_is_msg		process_msg;
	struct mutex			request_barrier;
	struct fimc_is_msg		request_msg;

	atomic_t			lock_pid;
	wait_queue_head_t		lock_wait_queue;
	wait_queue_head_t		init_wait_queue;
	wait_queue_head_t		idle_wait_queue;
	struct fimc_is_msg		reply;
#ifdef MEASURE_TIME
#ifdef INTERFACE_TIME
	struct fimc_is_interface_time	time[HIC_COMMAND_END];
#endif
#endif

	struct workqueue_struct		*workqueue;
	struct work_struct		work_wq[WORK_MAX_MAP];
	struct fimc_is_work_list	work_list[WORK_MAX_MAP];

	/* sensor streaming flag */
	enum streaming_state		streaming[FIMC_IS_STREAM_COUNT];
	/* firmware processing flag */
	enum processing_state		processing[FIMC_IS_STREAM_COUNT];
	/* frrmware power down ready flag */
	enum pdown_ready_state		pdown_ready;

	unsigned long			fw_boot;

	struct fimc_is_framemgr		*framemgr;

	struct fimc_is_work_list	nblk_cam_ctrl;

	/* shot timeout check */
	spinlock_t			shot_check_lock;
	atomic_t			shot_check[FIMC_IS_STREAM_COUNT];
	atomic_t			shot_timeout[FIMC_IS_STREAM_COUNT];
	/* sensor timeout check */
	atomic_t			sensor_check[FIMC_IS_SENSOR_COUNT];
	atomic_t			sensor_timeout[FIMC_IS_SENSOR_COUNT];
	struct timer_list		timer;

	/* callback func to handle error report for specific purpose */
	void				*err_report_data;
	int				(*err_report_vendor)(void *data, u32 err_report_type);

	struct camera2_uctl		isp_peri_ctl;
	/* check firsttime */
	unsigned long			launch_state;
	enum fimc_is_fw_boot		fw_boot_mode;
	ulong				itf_kvaddr;
	void				*core;
};

void fimc_is_itf_fwboot_init(struct fimc_is_interface *this);
int fimc_is_interface_probe(struct fimc_is_interface *this,
	struct fimc_is_minfo *minfo,
	ulong regs,
	u32 irq,
	void *core_data);
int fimc_is_interface_open(struct fimc_is_interface *this);
int fimc_is_interface_close(struct fimc_is_interface *this);
void fimc_is_interface_lock(struct fimc_is_interface *this);
void fimc_is_interface_unlock(struct fimc_is_interface *this);
void fimc_is_interface_reset(struct fimc_is_interface *this);

void fimc_is_storefirm(struct fimc_is_interface *this);
void fimc_is_restorefirm(struct fimc_is_interface *this);
int fimc_is_set_fwboot(struct fimc_is_interface *this, int val);

/*for debugging*/
int print_fre_work_list(struct fimc_is_work_list *this);
int print_req_work_list(struct fimc_is_work_list *this);

int fimc_is_hw_logdump(struct fimc_is_interface *this);
int fimc_is_hw_regdump(struct fimc_is_interface *this);
int fimc_is_hw_memdump(struct fimc_is_interface *this,
	ulong start,
	ulong end);
int fimc_is_hw_enum(struct fimc_is_interface *this);
int fimc_is_hw_fault(struct fimc_is_interface *this);
int fimc_is_hw_open(struct fimc_is_interface *this,
	u32 instance, u32 module, u32 info, u32 path, u32 flag,
	u32 *mwidth, u32 *mheight);
int fimc_is_hw_close(struct fimc_is_interface *this,
	u32 instance);
int fimc_is_hw_saddr(struct fimc_is_interface *interface,
	u32 instance, u32 *setfile_addr);
int fimc_is_hw_setfile(struct fimc_is_interface *interface,
	u32 instance);
int fimc_is_hw_process_on(struct fimc_is_interface *this,
	u32 instance, u32 group);
int fimc_is_hw_process_off(struct fimc_is_interface *this,
	u32 instance, u32 group, u32 mode);
int fimc_is_hw_stream_on(struct fimc_is_interface *interface,
	u32 instance);
int fimc_is_hw_stream_off(struct fimc_is_interface *interface,
	u32 instance);
int fimc_is_hw_s_param(struct fimc_is_interface *interface,
	u32 instance, u32 lindex, u32 hindex, u32 indexes);
int fimc_is_hw_a_param(struct fimc_is_interface *this,
	u32 instance, u32 group, u32 sub_mode);
int fimc_is_hw_g_capability(struct fimc_is_interface *this,
	u32 instance, u32 address);
int fimc_is_hw_map(struct fimc_is_interface *this,
	u32 instance, u32 group, u32 address, u32 size);
int fimc_is_hw_unmap(struct fimc_is_interface *this,
	u32 instance, u32 group);
int fimc_is_hw_power_down(struct fimc_is_interface *interface,
	u32 instance);
int fimc_is_hw_i2c_lock(struct fimc_is_interface *interface,
	u32 instance, int clk, bool lock);
int fimc_is_hw_sys_ctl(struct fimc_is_interface *this,
	u32 instance, int cmd, int val);
int fimc_is_hw_sensor_mode(struct fimc_is_interface *this,
	u32 instance, int cfg);

int fimc_is_hw_shot_nblk(struct fimc_is_interface *this,
	u32 instance, u32 group, u32 shot, u32 fcount, u32 rcount);
int fimc_is_hw_s_camctrl_nblk(struct fimc_is_interface *this,
	u32 instance, u32 address, u32 fcount);
int fimc_is_hw_msg_test(struct fimc_is_interface *this, u32 sync_id, u32 msg_test_id);

/* func to register error report callback */
int fimc_is_set_err_report_vendor(struct fimc_is_interface *itf,
		void *err_report_data,
		int (*err_report_vendor)(void *data, u32 err_report_type));

#endif
