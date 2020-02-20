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

#ifndef FIMC_IS_DEVICE_SENSOR_H
#define FIMC_IS_DEVICE_SENSOR_H

#include <dt-bindings/camera/fimc_is.h>

#include <exynos-fimc-is-sensor.h>
#include "fimc-is-mem.h"
#include "fimc-is-video.h"
#include "fimc-is-resourcemgr.h"
#include "fimc-is-groupmgr.h"
#include "fimc-is-device-flite.h"
#include "fimc-is-device-csi.h"
#include "fimc-is-helper-i2c.h"

struct fimc_is_video_ctx;
struct fimc_is_device_ischain;

#define EXPECT_FRAME_START	0
#define EXPECT_FRAME_END	1
#define LOG_INTERVAL_OF_DROPS 30

#define FLITE_NOTIFY_FSTART		0
#define FLITE_NOTIFY_FEND		1
#define FLITE_NOTIFY_DMA_END		2
#define CSIS_NOTIFY_FSTART		3
#define CSIS_NOTIFY_FEND		4
#define CSIS_NOTIFY_DMA_END		5
#define CSIS_NOTIFY_DMA_END_VC_EMBEDDED	6
#define CSIS_NOTIFY_DMA_END_VC_MIPISTAT	7
#define CSIS_NOTIFY_LINE		8

#define SENSOR_MAX_ENUM			4 /* 4 modules(2 rear, 2 front) for same csis */
#define ACTUATOR_MAX_ENUM		FIMC_IS_SENSOR_COUNT
#define SENSOR_DEFAULT_FRAMERATE	30

#define SENSOR_MODE_MASK		0xFFFF0000
#define SENSOR_MODE_SHIFT		16
#define SENSOR_MODE_DEINIT		0xFFFF

#define SENSOR_SCENARIO_MASK		0xF0000000
#define SENSOR_SCENARIO_SHIFT		28
#define ASB_SCENARIO_MASK		0xF000
#define ASB_SCENARIO_SHIFT        	12
#define SENSOR_MODULE_MASK		0x00000FFF
#define SENSOR_MODULE_SHIFT		0

#define SENSOR_SSTREAM_MASK		0x0000000F
#define SENSOR_SSTREAM_SHIFT		0
#define SENSOR_USE_STANDBY_MASK		0x000000F0
#define SENSOR_USE_STANDBY_SHIFT	4
#define SENSOR_INSTANT_MASK		0x0FFF0000
#define SENSOR_INSTANT_SHIFT		16
#define SENSOR_NOBLOCK_MASK		0xF0000000
#define SENSOR_NOBLOCK_SHIFT		28

#define SENSOR_I2C_CH_MASK		0xFF
#define SENSOR_I2C_CH_SHIFT		0
#define ACTUATOR_I2C_CH_MASK		0xFF00
#define ACTUATOR_I2C_CH_SHIFT		8
#define OIS_I2C_CH_MASK 		0xFF0000
#define OIS_I2C_CH_SHIFT		16

#define SENSOR_I2C_ADDR_MASK		0xFF
#define SENSOR_I2C_ADDR_SHIFT		0
#define ACTUATOR_I2C_ADDR_MASK		0xFF00
#define ACTUATOR_I2C_ADDR_SHIFT		8
#define OIS_I2C_ADDR_MASK		0xFF0000
#define OIS_I2C_ADDR_SHIFT		16

#define SENSOR_SIZE_WIDTH_MASK		0xFFFF0000
#define SENSOR_SIZE_WIDTH_SHIFT		16
#define SENSOR_SIZE_HEIGHT_MASK		0xFFFF
#define SENSOR_SIZE_HEIGHT_SHIFT	0

#define FIMC_IS_TIMESTAMP_HASH_KEY	20

#define FIMC_IS_SENSOR_CFG(w, h, f, s, m, l, ls, itlv, pd,			\
	vc0_in, vc0_out, vc1_in, vc1_out, vc2_in, vc2_out, vc3_in, vc3_out) {	\
	.width				= w,					\
	.height				= h,					\
	.framerate			= f,					\
	.settle				= s,					\
	.mode				= m,					\
	.lanes				= l,					\
	.mipi_speed			= ls,					\
	.interleave_mode		= itlv,					\
	.pd_mode			= pd,					\
	.ex_mode			= EX_NONE,					\
	.input[CSI_VIRTUAL_CH_0]	= vc0_in,				\
	.output[CSI_VIRTUAL_CH_0]	= vc0_out,				\
	.input[CSI_VIRTUAL_CH_1]	= vc1_in,				\
	.output[CSI_VIRTUAL_CH_1]	= vc1_out,				\
	.input[CSI_VIRTUAL_CH_2]	= vc2_in,				\
	.output[CSI_VIRTUAL_CH_2]	= vc2_out,				\
	.input[CSI_VIRTUAL_CH_3]	= vc3_in,				\
	.output[CSI_VIRTUAL_CH_3]	= vc3_out,				\
}

#define FIMC_IS_SENSOR_CFG_EX(w, h, f, s, m, l, ls, itlv, pd, ex,			\
	vc0_in, vc0_out, vc1_in, vc1_out, vc2_in, vc2_out, vc3_in, vc3_out) {	\
	.width				= w,					\
	.height				= h,					\
	.framerate			= f,					\
	.settle				= s,					\
	.mode				= m,					\
	.lanes				= l,					\
	.mipi_speed			= ls,					\
	.interleave_mode		= itlv,					\
	.pd_mode			= pd,					\
	.ex_mode			= ex,					\
	.input[CSI_VIRTUAL_CH_0]	= vc0_in,				\
	.output[CSI_VIRTUAL_CH_0]	= vc0_out,				\
	.input[CSI_VIRTUAL_CH_1]	= vc1_in,				\
	.output[CSI_VIRTUAL_CH_1]	= vc1_out,				\
	.input[CSI_VIRTUAL_CH_2]	= vc2_in,				\
	.output[CSI_VIRTUAL_CH_2]	= vc2_out,				\
	.input[CSI_VIRTUAL_CH_3]	= vc3_in,				\
	.output[CSI_VIRTUAL_CH_3]	= vc3_out,				\
}

/*
 * @map:	VC parsing info.
 *		This is determined by sensor output format.
 * @hwformat:	DT parsing info.
 *		This is determined by sensor output format.
 * @width:	Real sensor output. For example, 2PD width is twice of DMA ouput width.
 *		If there is not VC output of sensor, set "0".
 * @heith:	Real sensor output.
 *		If there is not VC output of sensor, set "0".
 */
#define VC_IN(m, hwf, w, h) {			\
	.map = m,				\
	.hwformat = hwf,			\
	.width = w,				\
	.height = h,				\
}

/*
 * @hwformat:	It is same value width input hwformat, when input & output is same.
 *		But when the PDP is used, input & ouput is differnet.
 * @type:	It is used only for internal VC. There are VC_TAILPDAF, VC_MIPISTAT.
 *		If the buffer is used by HAL, set VC_NOTHING.
 * @width:	It is used only for internal VC. It is VC data size.
 *		If the buffer is used by HAL, set "0".
 * @height:	It is used only for internal VC. It is VC data size.
 *		If the buffer is used by HAL, set "0".
 */
#define VC_OUT(hwf, t, w, h) {			\
	.hwformat = hwf,			\
	.type = t,				\
	.width = w,				\
	.height = h,				\
}

enum fimc_is_sensor_subdev_ioctl {
	SENSOR_IOCTL_DMA_CANCEL,
	SENSOR_IOCTL_PATTERN_ENABLE,
	SENSOR_IOCTL_PATTERN_DISABLE,
};

#if defined(SECURE_CAMERA_IRIS)
enum fimc_is_sensor_smc_state {
        FIMC_IS_SENSOR_SMC_INIT = 0,
        FIMC_IS_SENSOR_SMC_CFW_ENABLE,
        FIMC_IS_SENSOR_SMC_PREPARE,
        FIMC_IS_SENSOR_SMC_UNPREPARE,
};
#endif

enum fimc_is_sensor_output_entity {
	FIMC_IS_SENSOR_OUTPUT_NONE = 0,
	FIMC_IS_SENSOR_OUTPUT_FRONT,
};

enum fimc_is_sensor_force_stop {
	FIMC_IS_BAD_FRAME_STOP = 0,
	FIMC_IS_MIF_THROTTLING_STOP = 1,
	FIMC_IS_FLITE_OVERFLOW_STOP = 2
};

enum fimc_is_module_state {
	FIMC_IS_MODULE_GPIO_ON,
	FIMC_IS_MODULE_STANDBY_ON
};

enum fimc_is_pd_mode {
	PD_MSPD = 0,
	PD_MOD1 = 1,
	PD_MOD2 = 2,
	PD_MOD3 = 3,
	PD_MSPD_TAIL = 4,
	PD_NONE,
};

/* refer to EXTEND_SENSOR_MODE on HAL side*/
enum fimc_is_ex_mode {
	EX_NONE = 0,
	EX_DRAMTEST = 1,
	EX_LIVEFOCUS = 2,
	EX_DUALFPS_960 = 3,
	EX_DUALFPS_480 = 4,
	EX_PDAF_OFF = 5,
	EX_3DHDR = 6,
	EX_PDSTAT_OFF = 7,
};

struct fimc_is_sensor_cfg {
	u32 width;
	u32 height;
	u32 framerate;
	u32 settle;
	int mode;
	u32 lanes;
	u32 mipi_speed;
	u32 interleave_mode;
	enum fimc_is_pd_mode pd_mode;
	enum fimc_is_ex_mode ex_mode;
	struct fimc_is_vci_config input[CSI_VIRTUAL_CH_MAX];
	struct fimc_is_vci_config output[CSI_VIRTUAL_CH_MAX];
};


struct fimc_is_sensor_vc_extra_info {
	int stat_type;
	int sensor_mode;
	u32 max_width;
	u32 max_height;
	u32 max_element;
};

struct fimc_is_sensor_ops {
	int (*stream_on)(struct v4l2_subdev *subdev);
	int (*stream_off)(struct v4l2_subdev *subdev);

	int (*s_duration)(struct v4l2_subdev *subdev, u64 duration);
	int (*g_min_duration)(struct v4l2_subdev *subdev);
	int (*g_max_duration)(struct v4l2_subdev *subdev);

	int (*s_exposure)(struct v4l2_subdev *subdev, u64 exposure);
	int (*g_min_exposure)(struct v4l2_subdev *subdev);
	int (*g_max_exposure)(struct v4l2_subdev *subdev);

	int (*s_again)(struct v4l2_subdev *subdev, u64 sensivity);
	int (*g_min_again)(struct v4l2_subdev *subdev);
	int (*g_max_again)(struct v4l2_subdev *subdev);

	int (*s_dgain)(struct v4l2_subdev *subdev);
	int (*g_min_dgain)(struct v4l2_subdev *subdev);
	int (*g_max_dgain)(struct v4l2_subdev *subdev);

	int (*s_shutterspeed)(struct v4l2_subdev *subdev, u64 shutterspeed);
	int (*g_min_shutterspeed)(struct v4l2_subdev *subdev);
	int (*g_max_shutterspeed)(struct v4l2_subdev *subdev);
};

struct fimc_is_module_enum {
	u32						sensor_id;
	struct v4l2_subdev				*subdev; /* connected module subdevice */
	u32						device; /* connected sensor device */
	unsigned long					state;
	u32						pixel_width;
	u32						pixel_height;
	u32						active_width;
	u32						active_height;
	u32                                             margin_left;
	u32                                             margin_right;
	u32                                             margin_top;
	u32                                             margin_bottom;
	u32						max_framerate;
	u32						position;
	u32						bitwidth;
	u32						cfgs;
	struct fimc_is_sensor_cfg			*cfg;
	struct fimc_is_sensor_vc_extra_info		vc_extra_info[VC_BUF_DATA_TYPE_MAX];
	struct i2c_client				*client;
	struct sensor_open_extended			ext;
	struct fimc_is_sensor_ops			*ops;
	char						*sensor_maker;
	char						*sensor_name;
	char						*setfile_name;
	struct hrtimer					vsync_timer;
	struct work_struct				vsync_work;
	void						*private_data;
	struct exynos_platform_fimc_is_module		*pdata;
	struct device					*dev;
};

enum fimc_is_sensor_state {
	FIMC_IS_SENSOR_PROBE,
	FIMC_IS_SENSOR_OPEN,
	FIMC_IS_SENSOR_MCLK_ON,
	FIMC_IS_SENSOR_ICLK_ON,
	FIMC_IS_SENSOR_GPIO_ON,
	FIMC_IS_SENSOR_S_INPUT,
	FIMC_IS_SENSOR_S_CONFIG,
	FIMC_IS_SENSOR_DRIVING,		/* Deprecated: from device-sensor_v2 */
	FIMC_IS_SENSOR_STAND_ALONE,	/* SOC sensor, Iris sensor, Vision mode without IS chain */
	FIMC_IS_SENSOR_FRONT_START,
	FIMC_IS_SENSOR_FRONT_DTP_STOP,
	FIMC_IS_SENSOR_BACK_START,
	FIMC_IS_SENSOR_BACK_NOWAIT_STOP,	/* Deprecated: There is no special meaning from device-sensor_v2 */
	FIMC_IS_SENSOR_SUBDEV_MODULE_INIT,	/* Deprecated: from device-sensor_v2 */
	FIMC_IS_SENSOR_OTF_OUTPUT,
	FIMC_IS_SENSOR_ITF_REGISTER,	/* to check whether sensor interfaces are registered */
	FIMC_IS_SENSOR_WAIT_STREAMING,
	SENSOR_MODULE_GOT_INTO_TROUBLE,
};

enum sensor_subdev_internel_use {
	SUBDEV_SSVC0_INTERNAL_USE,
	SUBDEV_SSVC1_INTERNAL_USE,
	SUBDEV_SSVC2_INTERNAL_USE,
	SUBDEV_SSVC3_INTERNAL_USE,
};

struct fimc_is_device_sensor {
	struct v4l2_device				v4l2_dev;
	struct platform_device				*pdev;
	struct fimc_is_mem				mem;

	u32						instance;
	u32						position;
	struct fimc_is_image				image;

	struct fimc_is_video_ctx			*vctx;
	struct fimc_is_video				video;

	struct fimc_is_device_ischain   		*ischain;
	struct fimc_is_groupmgr				*groupmgr;
	struct fimc_is_resourcemgr			*resourcemgr;
	struct fimc_is_devicemgr			*devicemgr;
	struct fimc_is_module_enum			module_enum[SENSOR_MAX_ENUM];
	struct fimc_is_sensor_cfg			*cfg;

	/* current control value */
	struct camera2_sensor_ctl			sensor_ctl;
	struct camera2_lens_ctl				lens_ctl;
	struct camera2_flash_ctl			flash_ctl;
	u64						timestamp[FIMC_IS_TIMESTAMP_HASH_KEY];
	u64						timestampboot[FIMC_IS_TIMESTAMP_HASH_KEY];
	u32						frame_id[FIMC_IS_TIMESTAMP_HASH_KEY];

	u32						fcount;
	u32						line_fcount;
	u32						instant_cnt;
	int						instant_ret;
	wait_queue_head_t				instant_wait;
	struct work_struct				instant_work;
	unsigned long					state;
	spinlock_t					slock_state;
	struct mutex					mlock_state;
	atomic_t					group_open_cnt;
#if defined(SECURE_CAMERA_IRIS)
	enum fimc_is_sensor_smc_state			smc_state;
#endif

	/* hardware configuration */
	struct v4l2_subdev				*subdev_module;
	struct v4l2_subdev				*subdev_csi;
	struct v4l2_subdev				*subdev_flite;

	/* sensor dma video node */
	struct fimc_is_video				video_ssxvc0;
	struct fimc_is_video				video_ssxvc1;
	struct fimc_is_video				video_ssxvc2;
	struct fimc_is_video				video_ssxvc3;

	/* subdev for dma */
	struct fimc_is_subdev				ssvc0;
	struct fimc_is_subdev				ssvc1;
	struct fimc_is_subdev				ssvc2;
	struct fimc_is_subdev				ssvc3;
	struct fimc_is_subdev				bns;

	/* gain boost */
	int						min_target_fps;
	int						max_target_fps;
	int						scene_mode;

	/* for vision control */
	int						exposure_time;
	u64						frame_duration;

	/* ENABLE_DTP */
	bool						dtp_check;
	struct timer_list				dtp_timer;
	unsigned long					force_stop;

	/* for early buffer done */
	u32						early_buf_done_mode;
	struct hrtimer					early_buf_timer;

	struct exynos_platform_fimc_is_sensor		*pdata;
	atomic_t					module_count;
	struct v4l2_subdev 				*subdev_actuator[ACTUATOR_MAX_ENUM];
	struct fimc_is_actuator				*actuator[ACTUATOR_MAX_ENUM];
	struct v4l2_subdev				*subdev_flash;
	struct fimc_is_flash				*flash;
	struct v4l2_subdev				*subdev_ois;
	struct fimc_is_ois				*ois;
	struct v4l2_subdev				*subdev_mcu;
	struct fimc_is_mcu				*mcu;
	struct v4l2_subdev				*subdev_aperture;
	struct fimc_is_aperture				*aperture;
	void						*private_data;
	struct fimc_is_group				group_sensor;
	struct fimc_is_path_info			path;

	u32						sensor_width;
	u32						sensor_height;

	int						num_of_ch_mode;
	bool						dma_abstract;
	u32						use_standby;
	u32						sstream;
	u32						num_buffers;
	u32						ex_mode;
	u32						ex_scenario;

#ifdef ENABLE_INIT_AWB
	/* backup AWB gains for use initial gain */
	float					init_wb[WB_GAIN_COUNT];
	float					last_wb[WB_GAIN_COUNT];
	float					chk_wb[WB_GAIN_COUNT];
	u32					init_wb_cnt;
#endif
};

int fimc_is_sensor_open(struct fimc_is_device_sensor *device,
	struct fimc_is_video_ctx *vctx);
int fimc_is_sensor_close(struct fimc_is_device_sensor *device);
#ifdef CONFIG_USE_SENSOR_GROUP
int fimc_is_sensor_s_input(struct fimc_is_device_sensor *device,
	u32 input,
	u32 scenario,
	u32 video_id);
#else
int fimc_is_sensor_s_input(struct fimc_is_device_sensor *device,
	u32 input,
	u32 scenario);
#endif
int fimc_is_sensor_s_ctrl(struct fimc_is_device_sensor *device,
	struct v4l2_control *ctrl);
int fimc_is_sensor_s_ext_ctrls(struct fimc_is_device_sensor *device,
	struct v4l2_ext_controls *ctrls);
int fimc_is_sensor_subdev_buffer_queue(struct fimc_is_device_sensor *device,
	enum fimc_is_subdev_id subdev_id,
	u32 index);
int fimc_is_sensor_buffer_queue(struct fimc_is_device_sensor *device,
	struct fimc_is_queue *queue,
	u32 index);
int fimc_is_sensor_buffer_finish(struct fimc_is_device_sensor *device,
	u32 index);

int fimc_is_sensor_front_start(struct fimc_is_device_sensor *device,
	u32 instant_cnt,
	u32 nonblock);
int fimc_is_sensor_front_stop(struct fimc_is_device_sensor *device);
void fimc_is_sensor_group_force_stop(struct fimc_is_device_sensor *device, u32 group_id);

int fimc_is_sensor_s_framerate(struct fimc_is_device_sensor *device,
	struct v4l2_streamparm *param);
int fimc_is_sensor_s_bns(struct fimc_is_device_sensor *device,
	u32 reatio);

int fimc_is_sensor_s_frame_duration(struct fimc_is_device_sensor *device,
	u32 frame_duration);
int fimc_is_sensor_s_exposure_time(struct fimc_is_device_sensor *device,
	u32 exposure_time);
int fimc_is_sensor_s_fcount(struct fimc_is_device_sensor *device);
int fimc_is_sensor_s_again(struct fimc_is_device_sensor *device, u32 gain);
int fimc_is_sensor_s_shutterspeed(struct fimc_is_device_sensor *device, u32 shutterspeed);

struct fimc_is_sensor_cfg * fimc_is_sensor_g_mode(struct fimc_is_device_sensor *device);
int fimc_is_sensor_mclk_on(struct fimc_is_device_sensor *device, u32 scenario, u32 channel);
int fimc_is_sensor_mclk_off(struct fimc_is_device_sensor *device, u32 scenario, u32 channel);
int fimc_is_sensor_gpio_on(struct fimc_is_device_sensor *device);
int fimc_is_sensor_gpio_off(struct fimc_is_device_sensor *device);
int fimc_is_sensor_gpio_dbg(struct fimc_is_device_sensor *device);
void fimc_is_sensor_dump(struct fimc_is_device_sensor *device);

int fimc_is_sensor_g_ctrl(struct fimc_is_device_sensor *device,
	struct v4l2_control *ctrl);
int fimc_is_sensor_g_instance(struct fimc_is_device_sensor *device);
int fimc_is_sensor_g_ex_mode(struct fimc_is_device_sensor *device);
int fimc_is_sensor_g_framerate(struct fimc_is_device_sensor *device);
int fimc_is_sensor_g_fcount(struct fimc_is_device_sensor *device);
int fimc_is_sensor_g_width(struct fimc_is_device_sensor *device);
int fimc_is_sensor_g_height(struct fimc_is_device_sensor *device);
int fimc_is_sensor_g_bns_width(struct fimc_is_device_sensor *device);
int fimc_is_sensor_g_bns_height(struct fimc_is_device_sensor *device);
int fimc_is_sensor_g_bns_ratio(struct fimc_is_device_sensor *device);
int fimc_is_sensor_g_bratio(struct fimc_is_device_sensor *device);
int fimc_is_sensor_g_module(struct fimc_is_device_sensor *device,
	struct fimc_is_module_enum **module);
int fimc_is_sensor_deinit_module(struct fimc_is_module_enum *module);
int fimc_is_sensor_g_position(struct fimc_is_device_sensor *device);
int fimc_is_search_sensor_module_with_sensorid(struct fimc_is_device_sensor *device,
	u32 sensor_id, struct fimc_is_module_enum **module);
int fimc_is_search_sensor_module_with_position(struct fimc_is_device_sensor *device,
	u32 position, struct fimc_is_module_enum **module);
int fimc_is_sensor_dm_tag(struct fimc_is_device_sensor *device,
	struct fimc_is_frame *frame);
int fimc_is_sensor_buf_tag(struct fimc_is_device_sensor *device,
	struct fimc_is_subdev *f_subdev,
	struct v4l2_subdev *v_subdev,
	struct fimc_is_frame *ldr_frame);
int fimc_is_sensor_g_csis_error(struct fimc_is_device_sensor *device);
int fimc_is_sensor_register_itf(struct fimc_is_device_sensor *device);
int fimc_is_sensor_group_tag(struct fimc_is_device_sensor *device,
	struct fimc_is_frame *frame,
	struct camera2_node *ldr_node);
int fimc_is_sensor_dma_cancel(struct fimc_is_device_sensor *device);
extern const struct fimc_is_queue_ops fimc_is_sensor_ops;
extern const struct fimc_is_queue_ops fimc_is_sensor_subdev_ops;

#define CALL_MOPS(s, op, args...) (((s)->ops->op) ? ((s)->ops->op(args)) : 0)

#endif
