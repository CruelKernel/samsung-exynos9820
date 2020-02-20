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
#include "fimc-is-work.h"

#define MAX_NBLOCKING_COUNT	3

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
