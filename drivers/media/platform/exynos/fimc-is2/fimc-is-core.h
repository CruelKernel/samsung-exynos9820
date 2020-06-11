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

#ifndef FIMC_IS_CORE_H
#define FIMC_IS_CORE_H

#include <linux/version.h>
#include <linux/sched.h>
#include <uapi/linux/sched/types.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
#include <linux/sched/rt.h>
#endif
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/pm_runtime.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>

#include <exynos-fimc-is.h>
#include "fimc-is-interface.h"
#include "fimc-is-framemgr.h"
#include "fimc-is-resourcemgr.h"
#include "fimc-is-devicemgr.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-ischain.h"
#ifndef ENABLE_IS_CORE
#include "hardware/fimc-is-hw-control.h"
#include "interface/fimc-is-interface-ischain.h"
#endif
#include "fimc-is-device-preprocessor.h"
#include "fimc-is-spi.h"
#include "fimc-is-video.h"
#include "fimc-is-mem.h"
#include "fimc-is-vender.h"
#include "exynos-fimc-is-module.h"

#define FIMC_IS_DRV_NAME			"exynos-fimc-is"
#define FIMC_IS_COMMAND_TIMEOUT			(30*HZ)
#define FIMC_IS_STARTUP_TIMEOUT			(3*HZ)
#define FIMC_IS_SHUTDOWN_TIMEOUT		(10*HZ)
#define FIMC_IS_FLITE_STOP_TIMEOUT		(3*HZ)

#define FIMC_IS_SENSOR_MAX_ENTITIES		(1)
#define FIMC_IS_SENSOR_PAD_SOURCE_FRONT		(0)
#define FIMC_IS_SENSOR_PADS_NUM			(1)

#define FIMC_IS_FRONT_MAX_ENTITIES		(1)
#define FIMC_IS_FRONT_PAD_SINK			(0)
#define FIMC_IS_FRONT_PAD_SOURCE_BACK		(1)
#define FIMC_IS_FRONT_PAD_SOURCE_BAYER		(2)
#define FIMC_IS_FRONT_PAD_SOURCE_SCALERC	(3)
#define FIMC_IS_FRONT_PADS_NUM			(4)

#define FIMC_IS_BACK_MAX_ENTITIES		(1)
#define FIMC_IS_BACK_PAD_SINK			(0)
#define FIMC_IS_BACK_PAD_SOURCE_3DNR		(1)
#define FIMC_IS_BACK_PAD_SOURCE_SCALERP		(2)
#define FIMC_IS_BACK_PADS_NUM			(3)

#define FIMC_IS_MAX_SENSOR_NAME_LEN		(16)

#define MAX_ODC_INTERNAL_BUF_WIDTH	(2560)  /* 4808 in HW */
#define MAX_ODC_INTERNAL_BUF_HEIGHT	(1920)  /* 3356 in HW */
#define SIZE_ODC_INTERNAL_BUF \
	(MAX_ODC_INTERNAL_BUF_WIDTH * MAX_ODC_INTERNAL_BUF_HEIGHT * 3)

#define MAX_DIS_INTERNAL_BUF_WIDTH	(2400)
#define MAX_DIS_INTERNAL_BUF_HEIGHT	(1360)
#define SIZE_DIS_INTERNAL_BUF \
	(MAX_DIS_INTERNAL_BUF_WIDTH * MAX_DIS_INTERNAL_BUF_HEIGHT * 2)

#define MAX_3DNR_INTERNAL_BUF_WIDTH	(4032)
#define MAX_3DNR_INTERNAL_BUF_HEIGHT	(2268)
#ifdef TPU_COMPRESSOR
#define SIZE_DNR_INTERNAL_BUF \
	ALIGN((MAX_3DNR_INTERNAL_BUF_WIDTH * MAX_3DNR_INTERNAL_BUF_HEIGHT * 22 / 2 / 8), 16)
#else
#define SIZE_DNR_INTERNAL_BUF \
	ALIGN((MAX_3DNR_INTERNAL_BUF_WIDTH * MAX_3DNR_INTERNAL_BUF_HEIGHT * 18 / 8), 16)
#endif

#define MAX_TNR_INTERNAL_BUF_WIDTH	(4032)
#define MAX_TNR_INTERNAL_BUF_HEIGHT	(3024)
#define SIZE_TNR_IMAGE_BUF \
	(MAX_TNR_INTERNAL_BUF_WIDTH * MAX_TNR_INTERNAL_BUF_HEIGHT * 12 / 8) /* 12 bittage */
#define SIZE_TNR_WEIGHT_BUF \
	(MAX_TNR_INTERNAL_BUF_WIDTH * MAX_TNR_INTERNAL_BUF_HEIGHT / 2 / 2) /* 8 bittage */


#define NUM_LHFD_INTERNAL_BUF		(3)
#define MAX_LHFD_INTERNEL_BUF_WIDTH	(640)
#define MAX_LHFD_INTERNEL_BUF_HEIGHT	(480)
#define MAX_LHFD_INTERNEL_BUF_SIZE \
	(MAX_LHFD_INTERNEL_BUF_WIDTH * MAX_LHFD_INTERNEL_BUF_HEIGHT)
#define SIZE_LHFD_INTERNEL_BUF \
	((MAX_LHFD_INTERNEL_BUF_SIZE * 45 / 4) + (4096 + 1024))
/*
 * FD one buffer size: 3.4 MB
 * FD_ map_data_1: MAX_FD_INTERNEL_BUF_SIZE * 3 / 2) byte
 * FD_ map_data_2: MAX_FD_INTERNEL_BUF_SIZE * 4) byte
 * FD_ map_data_3: MAX_FD_INTERNEL_BUF_SIZE * 4) byte
 * FD_ map_data_4: MAX_FD_INTERNEL_BUF_SIZE / 4) byte
 * FD_ map_data_5: MAX_FD_INTERNEL_BUF_SIZE) byte
 * FD_ map_data_6: 1024 byte
 * FD_ map_data_7: 256 byte
 */

#define SIZE_LHFD_SHOT_BUF		sizeof(struct camera2_shot)

#define MAX_LHFD_SHOT_BUF		(2)

#define NUM_VRA_INTERNAL_BUF		(1)
#define SIZE_VRA_INTERNEL_BUF		(0x00650000)

#define NUM_ODC_INTERNAL_BUF		(2)
#define NUM_DIS_INTERNAL_BUF		(1)
#define NUM_DNR_INTERNAL_BUF		(2)

#define GATE_IP_ISP			(0)
#define GATE_IP_DRC			(1)
#define GATE_IP_FD			(2)
#define GATE_IP_SCC			(3)
#define GATE_IP_SCP			(4)
#define GATE_IP_ODC			(0)
#define GATE_IP_DIS			(1)
#define GATE_IP_DNR			(2)

#ifndef ENABLE_IS_CORE
/* use sysfs for actuator */
#define INIT_MAX_SETTING					5
#endif

enum fimc_is_debug_device {
	FIMC_IS_DEBUG_MAIN = 0,
	FIMC_IS_DEBUG_EC,
	FIMC_IS_DEBUG_SENSOR,
	FIMC_IS_DEBUG_ISP,
	FIMC_IS_DEBUG_DRC,
	FIMC_IS_DEBUG_FD,
	FIMC_IS_DEBUG_SDK,
	FIMC_IS_DEBUG_SCALERC,
	FIMC_IS_DEBUG_ODC,
	FIMC_IS_DEBUG_DIS,
	FIMC_IS_DEBUG_TDNR,
	FIMC_IS_DEBUG_SCALERP
};

enum fimc_is_debug_target {
	FIMC_IS_DEBUG_UART = 0,
	FIMC_IS_DEBUG_MEMORY,
	FIMC_IS_DEBUG_DCC3
};

enum fimc_is_secure_camera_type {
	FIMC_IS_SECURE_CAMERA_IRIS = 1,
	FIMC_IS_SECURE_CAMERA_FACE = 2,
};

enum fimc_is_front_input_entity {
	FIMC_IS_FRONT_INPUT_NONE = 0,
	FIMC_IS_FRONT_INPUT_SENSOR,
};

enum fimc_is_front_output_entity {
	FIMC_IS_FRONT_OUTPUT_NONE = 0,
	FIMC_IS_FRONT_OUTPUT_BACK,
	FIMC_IS_FRONT_OUTPUT_BAYER,
	FIMC_IS_FRONT_OUTPUT_SCALERC,
};

enum fimc_is_back_input_entity {
	FIMC_IS_BACK_INPUT_NONE = 0,
	FIMC_IS_BACK_INPUT_FRONT,
};

enum fimc_is_back_output_entity {
	FIMC_IS_BACK_OUTPUT_NONE = 0,
	FIMC_IS_BACK_OUTPUT_3DNR,
	FIMC_IS_BACK_OUTPUT_SCALERP,
};

enum fimc_is_front_state {
	FIMC_IS_FRONT_ST_POWERED = 0,
	FIMC_IS_FRONT_ST_STREAMING,
	FIMC_IS_FRONT_ST_SUSPENDED,
};

enum fimc_is_clck_gate_mode {
	CLOCK_GATE_MODE_HOST = 0,
	CLOCK_GATE_MODE_FW,
};

#if defined(CONFIG_SECURE_CAMERA_USE)
enum fimc_is_secure_state {
	FIMC_IS_STATE_UNSECURE,
	FIMC_IS_STATE_SECURING,
	FIMC_IS_STATE_SECURED,
};
#endif

enum fimc_is_dual_mode {
	FIMC_IS_DUAL_MODE_NOTHING,
	FIMC_IS_DUAL_MODE_BYPASS,
	FIMC_IS_DUAL_MODE_SYNC,
	FIMC_IS_DUAL_MODE_SWITCH,
};

enum fimc_is_hal_debug_mode {
	FIMC_IS_HAL_DEBUG_SUDDEN_DEAD_DETECT,
	FIMC_IS_HAL_DEBUG_PILE_REQ_BUF,
	FIMC_IS_HAL_DEBUG_NDONE_REPROCESSING,
};

struct fimc_is_sysfs_debug {
	unsigned int en_dvfs;
	unsigned int en_clk_gate;
	unsigned int clk_gate_mode;
	unsigned int pattern_en;
	unsigned int pattern_fps;
	unsigned long hal_debug_mode;
	unsigned int hal_debug_delay;
};

#ifndef ENABLE_IS_CORE
struct fimc_is_sysfs_actuator {
	unsigned int init_step;
	int init_positions[INIT_MAX_SETTING];
	int init_delays[INIT_MAX_SETTING];
	bool enable_fixed;
	int fixed_position;
};
#endif

struct fimc_is_dual_info {
	int pre_mode;
	int mode;
	int max_fps_master;
	int max_fps_slave;
	int tick_count;
	int max_fps[SENSOR_POSITION_MAX];
};

#ifdef FIXED_SENSOR_DEBUG
struct fimc_is_sysfs_sensor {
	bool		is_en;
	unsigned int	frame_duration;
	unsigned int	long_exposure_time;
	unsigned int	short_exposure_time;
	unsigned int	long_analog_gain;
	unsigned int	short_analog_gain;
	unsigned int	long_digital_gain;
	unsigned int	short_digital_gain;
};
#endif

struct fimc_is_core {
	struct platform_device			*pdev;
	struct resource				*regs_res;
	void __iomem				*regs;
	int					irq;
	u32					current_position;
	atomic_t				rsccount;
	unsigned long				state;
	bool					shutdown;
	bool					reboot;
	struct fimc_is_sysfs_debug sysfs_debug;
	struct work_struct			wq_data_print_clk;

	/* depended on isp */
	struct exynos_platform_fimc_is		*pdata;

	struct fimc_is_resourcemgr		resourcemgr;
	struct fimc_is_groupmgr			groupmgr;
	struct fimc_is_devicemgr		devicemgr;
	struct fimc_is_interface		interface;

	struct fimc_is_device_preproc		preproc;
	struct mutex				sensor_lock;
	struct fimc_is_device_sensor		sensor[FIMC_IS_SENSOR_COUNT];
	struct fimc_is_device_csi_dma		csi_dma;
	u32					chain_config;
	struct fimc_is_device_ischain		ischain[FIMC_IS_STREAM_COUNT];
#ifndef ENABLE_IS_CORE
	struct fimc_is_hardware			hardware;
	struct fimc_is_interface_ischain	interface_ischain;
#endif
	struct v4l2_device			v4l2_dev;

	struct fimc_is_video			video_30s;
	struct fimc_is_video			video_30c;
	struct fimc_is_video			video_30p;
	struct fimc_is_video			video_30f;
	struct fimc_is_video			video_30g;
	struct fimc_is_video			video_31s;
	struct fimc_is_video			video_31c;
	struct fimc_is_video			video_31p;
	struct fimc_is_video			video_31f;
	struct fimc_is_video			video_31g;
	struct fimc_is_video			video_32s;
	struct fimc_is_video			video_32p;
	struct fimc_is_video			video_i0s;
	struct fimc_is_video			video_i0c;
	struct fimc_is_video			video_i0p;
	struct fimc_is_video			video_i1s;
	struct fimc_is_video			video_i1c;
	struct fimc_is_video			video_i1p;
	struct fimc_is_video			video_me0c;
	struct fimc_is_video			video_me1c;
	struct fimc_is_video			video_scc;
	struct fimc_is_video			video_scp;
	struct fimc_is_video			video_d0s;
	struct fimc_is_video			video_d0c;
	struct fimc_is_video			video_d1s;
	struct fimc_is_video			video_d1c;
	struct fimc_is_video			video_dcp0s;
	struct fimc_is_video			video_dcp1s;
	struct fimc_is_video			video_dcp0c;
	struct fimc_is_video			video_dcp1c;
	struct fimc_is_video			video_dcp2c;
	struct fimc_is_video			video_dcp3c;
	struct fimc_is_video			video_dcp4c;
	struct fimc_is_video			video_m0s;
	struct fimc_is_video			video_m1s;
	struct fimc_is_video			video_m0p;
	struct fimc_is_video			video_m1p;
	struct fimc_is_video			video_m2p;
	struct fimc_is_video			video_m3p;
	struct fimc_is_video			video_m4p;
	struct fimc_is_video			video_m5p;
	struct fimc_is_video			video_vra;
	struct fimc_is_video			video_paf0s;
	struct fimc_is_video			video_paf1s;

	/* spi */
	struct fimc_is_spi			spi0;
	struct fimc_is_spi			spi1;

	struct mutex				i2c_lock[SENSOR_CONTROL_I2C_MAX];

	spinlock_t				shared_rsc_slock[MAX_SENSOR_SHARED_RSC];
	atomic_t				shared_rsc_count[MAX_SENSOR_SHARED_RSC];
	atomic_t				i2c_rsccount[SENSOR_CONTROL_I2C_MAX];

	struct fimc_is_vender			vender;
#if defined(CONFIG_SECURE_CAMERA_USE)
	struct mutex				secure_state_lock;
	unsigned long				secure_state;
#endif
	ulong					secure_mem_info[2];	/* size, addr */
	ulong					non_secure_mem_info[2];	/* size, addr */
	u32					scenario;

	unsigned long                           sensor_map;
	struct fimc_is_dual_info		dual_info;
	struct mutex				ois_mode_lock;
};

int fimc_is_secure_func(struct fimc_is_core *core,
	struct fimc_is_device_sensor *device, u32 type, u32 scenario, ulong smc_cmd);
struct fimc_is_device_sensor *fimc_is_get_sensor_device(struct fimc_is_core *core);
int fimc_is_put_sensor_device(struct fimc_is_core *core);
void fimc_is_print_frame_dva(struct fimc_is_subdev *subdev);
void fimc_is_cleanup(struct fimc_is_core *core);

#define CALL_POPS(s, op, args...) (((s) && (s)->pdata && (s)->pdata->op) ? ((s)->pdata->op(&(s)->pdev->dev)) : -EPERM)

#endif /* FIMC_IS_CORE_H_ */
