/* drivers/gpu/arm/.../platform/mali_kbase_platform.h
 *
 * Copyright 2011 by S.LSI. Samsung Electronics Inc.
 * San#24, Nongseo-Dong, Giheung-Gu, Yongin, Korea
 *
 * Samsung SoC Mali-T Series platform-dependent codes
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software FoundatIon.
 */

/**
 * @file mali_kbase_platform.h
 * Platform-dependent init
 */

#ifndef _GPU_PLATFORM_H_
#define _GPU_PLATFORM_H_

#include <soc/samsung/exynos-pd.h>

#ifdef CONFIG_MALI_EXYNOS_TRACE
#define GPU_LOG(level, code, gpu_addr, info_val, msg, args...) \
do { \
	if (level >= gpu_get_debug_level()) { \
		printk(KERN_INFO "[G3D] "msg, ## args); \
	} \
	if (gpu_check_trace_code(KBASE_KTRACE_CODE(code))) { \
		KBASE_KTRACE_ADD_EXYNOS(gpu_get_device_structure(), code, NULL, info_val); \
	} \
} while (0)
#else /* CONFIG_MALI_EXYNOS_TRACE */
#define GPU_LOG(level, code, gpu_addr, info_val, msg, args...) \
do { \
	if (level >= gpu_get_debug_level()) { \
		printk(KERN_INFO msg, ## args); \
	} \
} while (0)
#endif /* CONFIG_MALI_EXYNOS_TRACE */

#define GPU_DVFS_TABLE_LIST_SIZE(X)  ARRAY_SIZE(X)

#define BMAX_RETRY_CNT 10

#define CPU_MAX INT_MAX
#define DVFS_TABLE_COL_NUM 8
#define DVFS_TABLE_ROW_MAX 20
#define OF_DATA_NUM_MAX 160

typedef enum {
	DVFS_DEBUG_START = 0,
	DVFS_DEBUG,
	DVFS_INFO,
	DVFS_WARNING,
	DVFS_ERROR,
	DVFS_DEBUG_END,
} gpu_dvfs_debug_level;

typedef enum {
	GPU_L0,
	GPU_L1,
	GPU_L2,
	GPU_L3,
	GPU_L4,
	GPU_L5,
	GPU_L6,
	GPU_L7,
	GPU_MAX_LEVEL,
} gpu_clock_level;

typedef enum {
	TRACE_START = 0,
	TRACE_NONE,
	TRACE_DEFAULT,
	TRACE_CLK,
	TRACE_VOL,
	TRACE_NOTIFIER,
	TRACE_DVFS,
	TRACE_DUMP,
	TRACE_ALL,
	TRACE_END,
} gpu_dvfs_trace_level;

typedef enum {
	TMU_LOCK = 0,
	SYSFS_LOCK,
#ifdef CONFIG_CPU_THERMAL_IPA
	IPA_LOCK,
#endif /* CONFIG_CPU_THERMAL_IPA */
	BOOST_LOCK,
	PMQOS_LOCK,
#ifdef CONFIG_MALI_ASV_CALIBRATION_SUPPORT
	ASV_CALI_LOCK,
#endif
	NUMBER_LOCK
} gpu_dvfs_lock_type;

typedef enum {
	THROTTLING1 = 0,
	THROTTLING2,
	THROTTLING3,
	THROTTLING4,
	THROTTLING5,
	TRIPPING,
	TMU_LOCK_CLK_END,
} tmu_lock_clk;

typedef enum {
	GPU_JOB_CONFIG_FAULT,
	GPU_JOB_POWER_FAULT,
	GPU_JOB_READ_FAULT,
	GPU_JOB_WRITE_FAULT,
	GPU_JOB_AFFINITY_FAULT,
	GPU_JOB_BUS_FAULT,
	GPU_DATA_INVALIDATE_FAULT,
	GPU_TILE_RANGE_FAULT,
	GPU_OUT_OF_MEMORY_FAULT,
	GPU_DELAYED_BUS_FAULT,
	GPU_SHAREABILITY_FAULT,
	GPU_MMU_TRANSLATION_FAULT,
	GPU_MMU_PERMISSION_FAULT,
	GPU_MMU_TRANSTAB_BUS_FAULT,
	GPU_MMU_ACCESS_FLAG_FAULT,
	GPU_MMU_ADDRESS_SIZE_FAULT,
	GPU_MMU_MEMORY_ATTRIBUTES_FAULT,
	GPU_UNKNOWN,
	GPU_SOFT_STOP,
	GPU_HARD_STOP,
	GPU_RESET,
	GPU_EXCEPTION_LIST_END,
} gpu_excention_type;

typedef struct _gpu_attribute {
	int id;
	uintptr_t data;
} gpu_attribute;

typedef struct _gpu_dvfs_info {
	unsigned int clock;
	unsigned int voltage;
	int asv_abb;
	int min_threshold;
	int max_threshold;
	int down_staycount;
	unsigned long long time;
	int mem_freq;
	int int_freq;
	int cpu_little_min_freq;
	int cpu_middle_min_freq;
	int cpu_big_max_freq;
	int g3dm_voltage;
} gpu_dvfs_info;

typedef struct _gpu_dvfs_governor_info {
	int id;
	char *name;
	void *governor;
	gpu_dvfs_info *table;
	int table_size;
	int start_clk;
} gpu_dvfs_governor_info;

typedef struct _gpu_dvfs_env_data {
	int utilization;
	int perf;
	int hwcnt;
} gpu_dvfs_env_data;

struct exynos_context {
	/* lock variables */
	struct mutex gpu_clock_lock;
	struct mutex gpu_dvfs_handler_lock;
	spinlock_t gpu_dvfs_spinlock;
#if (defined(CONFIG_SCHED_EMS) || defined(CONFIG_SCHED_EHMP) || defined(CONFIG_SCHED_HMP))
	struct mutex gpu_sched_hmp_lock;
#endif
	/* clock & voltage related variables */
	int clk_g3d_status;
#ifdef CONFIG_MALI_RT_PM
	struct exynos_pm_domain *exynos_pm_domain;
#endif /* CONFIG_MALI_RT_PM */

	/* dvfs related variables */
	gpu_dvfs_info *table;
	int table_size;
	int step;
	gpu_dvfs_env_data env_data;
	struct workqueue_struct *dvfs_wq;
	struct delayed_work *delayed_work;
#if defined(SET_MINLOCK)
	int custom_cpu_max_lock;
#endif
#ifdef CONFIG_MALI_DVFS
	bool dvfs_status;
	int utilization;
	int max_lock;
	int min_lock;
	int user_max_lock[NUMBER_LOCK];
	int user_min_lock[NUMBER_LOCK];
	int down_requirement;
	int governor_type;
	bool wakeup_lock;
	int dvfs_pending;

	/* For the interactive governor */
	struct {
		int highspeed_clock;
		int highspeed_load;
		int highspeed_delay;
		int delay_count;
	} interactive;
#ifdef CONFIG_CPU_THERMAL_IPA
	int norm_utilisation;
	int freq_for_normalisation;
	unsigned long long power;
	int time_tick;
	u32 time_busy;
	u32 time_idle;

	int ipa_power_coeff_gpu;
	int gpu_dvfs_time_interval;
#endif /* CONFIG_CPU_THERMAL_IPA */
#endif /* CONFIG_MALI_DVFS */

	/* status */
	int cur_clock;
	int cur_voltage;
	int voltage_margin;

	/* gpu configuration */
	bool using_max_limit_clock;
	int gpu_max_clock;
	int gpu_max_clock_limit;
	int gpu_min_clock;
	int gpu_dvfs_start_clock;
	int gpu_dvfs_config_clock;
	int user_max_lock_input;
	int user_min_lock_input;

	/* gpu boost lock */
	int boost_gpu_min_lock;
	int boost_egl_min_lock;
	bool boost_is_enabled;
	bool tmu_status;
	int tmu_lock_clk[TMU_LOCK_CLK_END];
	int cold_min_vol;
	int gpu_default_vol;
	int gpu_default_vol_margin;

	bool dynamic_abb_status;
	bool early_clk_gating_status;
	bool dvs_status;
	bool dvs_is_enabled;
	bool inter_frame_pm_feature;
	bool inter_frame_pm_status;
	bool inter_frame_pm_is_poweron;

	bool power_status;
	int power_runtime_suspend_ret;
	int power_runtime_resume_ret;


	int polling_speed;
	int runtime_pm_delay_time;
	bool pmqos_int_disable;

	int pmqos_mif_max_clock;
	int pmqos_mif_max_clock_base;

	int cl_dvfs_start_base;

	int debug_level;
	int trace_level;

	int fault_count;
	bool bigdata_uevent_is_sent;
	int gpu_exception_count[GPU_EXCEPTION_LIST_END];
	int balance_retry_count[BMAX_RETRY_CNT];
	gpu_attribute *attrib;
#ifdef CONFIG_EXYNOS_BTS
	int mo_min_clock;
#ifdef CONFIG_EXYNOS9630_BTS
	unsigned int bts_scen_idx;
	unsigned int is_set_bts; // Check the pair of bts scenario.
#endif
#ifdef CONFIG_MALI_CAMERA_EXT_BTS
	unsigned int bts_camera_ext_idx;
	unsigned int is_set_bts_camera_ext;
#endif
#endif
	int *save_cpu_max_freq;

	unsigned int g3d_cmu_cal_id;
#ifdef CONFIG_MALI_PM_QOS
	bool is_pm_qos_init;
#endif /* CONFIG_MALI_PM_QOS */
	const struct kbase_pm_policy *cur_policy;

#ifdef CONFIG_MALI_ASV_CALIBRATION_SUPPORT
	bool gpu_auto_cali_status;
#endif

#if (defined(CONFIG_SCHED_EMS) || defined(CONFIG_SCHED_EHMP) || defined(CONFIG_SCHED_HMP))
	bool ctx_need_qos;
#endif

#ifdef CONFIG_MALI_SEC_VK_BOOST
	bool ctx_vk_need_qos;
	struct mutex gpu_vk_boost_lock;
	int gpu_vk_boost_max_clk_lock;
	int gpu_vk_boost_mif_min_clk_lock;
#endif

	int gpu_pmqos_cpu_cluster_num;

#ifdef CONFIG_MALI_SUSTAINABLE_OPT
	struct {
		bool status;
		int info_array[5];
	} sustainable;
#endif

#ifdef CONFIG_MALI_SEC_CL_BOOST
	bool cl_boost_disable;
#endif

	int gpu_set_pmu_duration_reg;
	int gpu_set_pmu_duration_val;
	bool gpu_bts_support;
	char g3d_genpd_name[30];
	int gpu_dss_freq_id;

#if IS_ENABLED(CONFIG_MALI_EXYNOS_SECURE_RENDERING_LEGACY) || IS_ENABLED(CONFIG_MALI_EXYNOS_SECURE_RENDERING_ARM)
	bool exynos_smc_enabled;
	spinlock_t exynos_smc_lock;
#endif

	/* Callback to call when for GPU clock changes */
	struct notifier_block *nb_clock_change;
};

struct kbase_device *gpu_get_device_structure(void);
void gpu_set_debug_level(int level);
int gpu_get_debug_level(void);
void gpu_set_trace_level(int level);
bool gpu_check_trace_level(int level);
bool gpu_check_trace_code(int code);
void *gpu_get_config_attributes(void);
uintptr_t gpu_get_attrib_data(gpu_attribute *attrib, int id);
int gpu_platform_context_init(struct exynos_context *platform);

int gpu_set_rate_for_pm_resume(struct kbase_device *kbdev, int clk);
void gpu_clock_disable(struct kbase_device *kbdev);

bool balance_init(struct kbase_device *kbdev);
int exynos_gpu_init_hw(void *dev);

#ifdef CONFIG_OF
void gpu_update_config_data_bool(struct device_node *np, const char *of_string, bool *of_data);
void gpu_update_config_data_int(struct device_node *np, const char *of_string, int *of_data);
void gpu_update_config_data_string(struct device_node *np, const char *of_string, const char **of_data);
void gpu_update_config_data_int_array(struct device_node *np, const char *of_string, int *of_data, int sz);
#endif

#endif /* _GPU_PLATFORM_H_ */
