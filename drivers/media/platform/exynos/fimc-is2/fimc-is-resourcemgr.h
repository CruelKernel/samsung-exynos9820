/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_RESOURCE_MGR_H
#define FIMC_IS_RESOURCE_MGR_H

#include <linux/notifier.h>
#include <linux/reboot.h>
#include "fimc-is-groupmgr.h"
#include "fimc-is-interface.h"

#define RESOURCE_TYPE_SENSOR0	0
#define RESOURCE_TYPE_SENSOR1	1
#define RESOURCE_TYPE_SENSOR2	2
#define RESOURCE_TYPE_SENSOR3	3
#define RESOURCE_TYPE_SENSOR4	4
#define RESOURCE_TYPE_SENSOR5	5
#define RESOURCE_TYPE_ISCHAIN	6
#define RESOURCE_TYPE_PREPROC	7
#define RESOURCE_TYPE_MAX	8

enum fimc_is_resourcemgr_state {
	FIMC_IS_RM_COM_POWER_ON,
	FIMC_IS_RM_SS0_POWER_ON,
	FIMC_IS_RM_SS1_POWER_ON,
	FIMC_IS_RM_SS2_POWER_ON,
	FIMC_IS_RM_SS3_POWER_ON,
	FIMC_IS_RM_SS4_POWER_ON,
	FIMC_IS_RM_SS5_POWER_ON,
	FIMC_IS_RM_ISC_POWER_ON,
	FIMC_IS_RM_POWER_ON
};

enum fimc_is_dvfs_state {
	FIMC_IS_DVFS_SEL_TABLE
};

enum fimc_is_binary_state {
	FIMC_IS_BINARY_LOADED
};

struct fimc_is_dvfs_ctrl {
	struct mutex lock;
#if defined(QOS_INTCAM)
	int cur_int_cam_qos;
#endif
	int cur_int_qos;
	int cur_mif_qos;
	int cur_cam_qos;
	int cur_i2c_qos;
	int cur_disp_qos;
	int cur_hpg_qos;
	int cur_hmp_bst;
	u32 dvfs_table_idx;
	u32 dvfs_table_max;
	ulong state;

	struct fimc_is_dvfs_scenario_ctrl *static_ctrl;
	struct fimc_is_dvfs_scenario_ctrl *dynamic_ctrl;
	struct fimc_is_dvfs_scenario_ctrl *external_ctrl;
};

struct fimc_is_clk_gate_ctrl {
	spinlock_t lock;
	unsigned long msk_state;
	int msk_cnt[GROUP_ID_MAX];
	u32 msk_lock_by_ischain[FIMC_IS_STREAM_COUNT];
	struct exynos_fimc_is_clk_gate_info *gate_info;
	u32 msk_clk_on_off_state; /* on/off(1/0) state per ip */
	/*
	 * For check that there's too long clock-on period.
	 * This var will increase when clock on,
	 * And will decrease when clock off.
	 */
	unsigned long chk_on_off_cnt[GROUP_ID_MAX];
};

struct fimc_is_resource {
        struct platform_device                  *pdev;
        void __iomem                            *regs;
        atomic_t                                rsccount;
        u32                                     private_data;
};

struct fimc_is_global_param {
	struct mutex				lock;
	bool					video_mode;
	ulong					state;
};

struct fimc_is_resourcemgr {
	unsigned long				state;
	atomic_t				rsccount;
	atomic_t				qos_refcount; /* For multi-instance with SOC sensor */
	struct fimc_is_resource			resource_preproc;
	struct fimc_is_resource			resource_sensor0;
	struct fimc_is_resource			resource_sensor1;
	struct fimc_is_resource			resource_sensor2;
	struct fimc_is_resource			resource_sensor3;
	struct fimc_is_resource			resource_sensor4;
	struct fimc_is_resource			resource_sensor5;
	struct fimc_is_resource			resource_ischain;

	struct fimc_is_mem			mem;
	struct fimc_is_minfo			minfo;

	struct fimc_is_dvfs_ctrl		dvfs_ctrl;
	struct fimc_is_clk_gate_ctrl		clk_gate_ctrl;
	u32					cluster0;
	u32					cluster1;
	u32					cluster2;
	u32					hal_version;
#ifdef ENABLE_FW_SHARE_DUMP
	ulong					fw_share_dump_buf;
#endif

	/* tmu */
	struct notifier_block			tmu_notifier;
	u32					tmu_state;
	u32					limited_fps;

	/* bus monitor */
	struct notifier_block			bm_notifier;

	void					*private_data;
	/* binary loading */
#ifndef ENABLE_IS_CORE
	unsigned long				binary_state;
#endif

#ifdef ENABLE_SHARED_METADATA
	/* shared meta data */
	spinlock_t			shared_meta_lock;
	struct camera2_shot			shared_shot;
#endif
#ifdef ENABLE_KERNEL_LOG_DUMP
	unsigned long long			kernel_log_time;
	void					*kernel_log_buf;
#endif
	struct fimc_is_global_param		global_param;

	/* for critical section at get/put */
	struct mutex				rsc_lock;
	/* for sysreg setting */
	struct mutex				sysreg_lock;

	u32					shot_timeout;
	int					shot_timeout_tick;

	u32					streaming_cnt;
};

int fimc_is_resourcemgr_probe(struct fimc_is_resourcemgr *resourcemgr, void *private_data, struct platform_device *pdev);
int fimc_is_resource_open(struct fimc_is_resourcemgr *resourcemgr, u32 rsc_type, void **device);
int fimc_is_resource_get(struct fimc_is_resourcemgr *resourcemgr, u32 rsc_type);
int fimc_is_resource_put(struct fimc_is_resourcemgr *resourcemgr, u32 rsc_type);
int fimc_is_resource_ioctl(struct fimc_is_resourcemgr *resourcemgr, struct v4l2_control *ctrl);
int fimc_is_logsync(struct fimc_is_interface *itf, u32 sync_id, u32 msg_test_id);
void fimc_is_resource_set_global_param(struct fimc_is_resourcemgr *resourcemgr, void *device);
void fimc_is_resource_clear_global_param(struct fimc_is_resourcemgr *resourcemgr, void *device);
int fimc_is_resource_dump(void);
int fimc_is_kernel_log_dump(bool overwrite);
#ifdef ENABLE_HWACG_CONTROL
extern void fimc_is_hw_csi_qchannel_enable_all(bool enable);
#endif

#define GET_RESOURCE(resourcemgr, type) \
	((type == RESOURCE_TYPE_SENSOR0) ? &resourcemgr->resource_sensor0 : \
	((type == RESOURCE_TYPE_SENSOR1) ? &resourcemgr->resource_sensor1 : \
	((type == RESOURCE_TYPE_SENSOR2) ? &resourcemgr->resource_sensor2 : \
	((type == RESOURCE_TYPE_SENSOR3) ? &resourcemgr->resource_sensor3 : \
	((type == RESOURCE_TYPE_SENSOR4) ? &resourcemgr->resource_sensor4 : \
	((type == RESOURCE_TYPE_SENSOR5) ? &resourcemgr->resource_sensor5 : \
	((type == RESOURCE_TYPE_ISCHAIN) ? &resourcemgr->resource_ischain : \
	((type == RESOURCE_TYPE_PREPROC) ? &resourcemgr->resource_preproc : \
	NULL))))))))

#endif
