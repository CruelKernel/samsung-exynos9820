/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <video/videonode.h>
#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <linux/firmware.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/videodev2_exynos_media.h>
#include <linux/v4l2-mediabus.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/pm_qos.h>
#include <linux/syscalls.h>
#include <linux/bug.h>
#include <linux/smc.h>
#if defined(CONFIG_PCI_EXYNOS)
#include <linux/exynos-pci-ctrl.h>
#endif

#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/gpio.h>
#if defined(CONFIG_SCHED_EHMP)
#include <linux/ehmp.h>
#elif defined(CONFIG_SCHED_EMS)
#include <linux/ems.h>
#endif
#if defined(CONFIG_EXYNOS_BTS)
#include <soc/samsung/bts.h>
#endif

#include "fimc-is-binary.h"
#include "fimc-is-time.h"
#include "fimc-is-core.h"
#include "fimc-is-param.h"
#include "fimc-is-cmd.h"
#include "fimc-is-regs.h"
#include "fimc-is-err.h"
#include "fimc-is-video.h"
#include "fimc-is-hw.h"
#include "fimc-is-spi.h"
#include "fimc-is-groupmgr.h"
#include "fimc-is-interface-wrap.h"
#include "fimc-is-device-ischain.h"
#include "fimc-is-clk-gate.h"
#include "fimc-is-dvfs.h"
#include "fimc-is-device-preprocessor.h"
#include "fimc-is-vender-specific.h"
#include "exynos-fimc-is-module.h"
#include "./sensor/module_framework/modules/fimc-is-device-module-base.h"

#include "fimc-is-vender-specific.h"

/* Default setting values */
#define DEFAULT_PREVIEW_STILL_WIDTH		(1280) /* sensor margin : 16 */
#define DEFAULT_PREVIEW_STILL_HEIGHT		(720) /* sensor margin : 12 */
#define DEFAULT_CAPTURE_VIDEO_WIDTH		(1920)
#define DEFAULT_CAPTURE_VIDEO_HEIGHT		(1080)
#define DEFAULT_CAPTURE_STILL_WIDTH		(2560)
#define DEFAULT_CAPTURE_STILL_HEIGHT		(1920)
#define DEFAULT_CAPTURE_STILL_CROP_WIDTH	(2560)
#define DEFAULT_CAPTURE_STILL_CROP_HEIGHT	(1440)
#define DEFAULT_PREVIEW_VIDEO_WIDTH		(640)
#define DEFAULT_PREVIEW_VIDEO_HEIGHT		(480)

/* sysfs variable for debug */
extern struct fimc_is_sysfs_debug sysfs_debug;
extern int debug_clk;

#if defined(QOS_INTCAM)
extern struct pm_qos_request exynos_isp_qos_int_cam;
#endif
extern struct pm_qos_request exynos_isp_qos_int;
extern struct pm_qos_request exynos_isp_qos_mem;
extern struct pm_qos_request exynos_isp_qos_cam;
extern struct pm_qos_request exynos_isp_qos_hpg;
extern struct pm_qos_request exynos_isp_qos_cpu_online_min;

#if defined(CONFIG_SCHED_EHMP) || defined(CONFIG_SCHED_EMS)
extern struct gb_qos_request gb_req;
#endif

extern const struct fimc_is_subdev_ops fimc_is_subdev_paf_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_3aa_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_3ac_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_3ap_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_3af_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_3ag_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_isp_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_ixc_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_ixp_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_mexc_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_dis_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_dxc_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_scc_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_scp_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_dcp_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_dcxs_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_dcxc_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_dcxc_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_mcs_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_mcsp_ops;
extern const struct fimc_is_subdev_ops fimc_is_subdev_vra_ops;

#ifdef ENABLE_FD_SW
extern void *fd_vaddr;
#endif

static int fimc_is_ischain_paf_stop(void *qdevice,
	struct fimc_is_queue *queue);
static int fimc_is_ischain_3aa_stop(void *qdevice,
	struct fimc_is_queue *queue);
static int fimc_is_ischain_isp_stop(void *qdevice,
	struct fimc_is_queue *queue);
static int fimc_is_ischain_dis_stop(void *qdevice,
	struct fimc_is_queue *queue);
static int fimc_is_ischain_dcp_stop(void *qdevice,
	struct fimc_is_queue *queue);
static int fimc_is_ischain_mcs_stop(void *qdevice,
	struct fimc_is_queue *queue);
static int fimc_is_ischain_vra_stop(void *qdevice,
	struct fimc_is_queue *queue);

static int fimc_is_ischain_paf_shot(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame);
static int fimc_is_ischain_3aa_shot(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame);
static int fimc_is_ischain_isp_shot(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame);
static int fimc_is_ischain_dis_shot(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame);
static int fimc_is_ischain_dcp_shot(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame);
static int fimc_is_ischain_mcs_shot(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame);
static int fimc_is_ischain_vra_shot(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame);

static const struct sensor_param init_sensor_param = {
	.config = {
#ifdef FIXED_FPS_DEBUG
		.frametime = (1000 * 1000) / FIXED_FPS_VALUE,
		.min_target_fps = FIXED_FPS_VALUE,
		.max_target_fps = FIXED_FPS_VALUE,
#else
		.frametime = (1000 * 1000) / 30,
		.min_target_fps = 15,
		.max_target_fps = 30,
#endif
	},
};

static const struct taa_param init_taa_param = {
	.control = {
		.cmd = CONTROL_COMMAND_START,
		.bypass = CONTROL_BYPASS_DISABLE,
		.err = CONTROL_ERROR_NO,
	},
	.otf_input = {
		.cmd = OTF_INPUT_COMMAND_ENABLE,
		.format = OTF_INPUT_FORMAT_BAYER,
		.bitwidth = OTF_INPUT_BIT_WIDTH_10BIT,
		.order = OTF_INPUT_ORDER_BAYER_GR_BG,
		.width = DEFAULT_CAPTURE_STILL_WIDTH,
		.height = DEFAULT_CAPTURE_STILL_HEIGHT,
		.bayer_crop_offset_x = 0,
		.bayer_crop_offset_y = 0,
		.bayer_crop_width = 0,
		.bayer_crop_height = 0,
		.err = OTF_INPUT_ERROR_NO,
	},
	.vdma1_input = {
		.cmd = DMA_INPUT_COMMAND_DISABLE,
		.format = 0,
		.bitwidth = 0,
		.order = 0,
		.plane = 0,
		.width = 0,
		.height = 0,
		.bayer_crop_offset_x = 0,
		.bayer_crop_offset_y = 0,
		.bayer_crop_width = 0,
		.bayer_crop_height = 0,
		.err = 0,
	},
	.ddma_input = {
		.cmd = DMA_INPUT_COMMAND_DISABLE,
	},
	.otf_output = {
		.cmd = OTF_OUTPUT_COMMAND_DISABLE,
	},
	.vdma4_output = {
		.cmd = DMA_OUTPUT_COMMAND_DISABLE,
		.width = DEFAULT_CAPTURE_STILL_WIDTH,
		.height = DEFAULT_CAPTURE_STILL_HEIGHT,
		.format = DMA_INPUT_FORMAT_YUV444,
		.bitwidth = DMA_INPUT_BIT_WIDTH_8BIT,
		.plane = DMA_INPUT_PLANE_1,
		.order = DMA_INPUT_ORDER_YCbCr,
		.err = DMA_OUTPUT_ERROR_NO,
	},
	.vdma2_output = {
		.cmd = DMA_OUTPUT_COMMAND_DISABLE,
		.width = DEFAULT_CAPTURE_STILL_WIDTH,
		.height = DEFAULT_CAPTURE_STILL_HEIGHT,
		.format = DMA_OUTPUT_FORMAT_BAYER,
		.bitwidth = DMA_OUTPUT_BIT_WIDTH_12BIT,
		.plane = DMA_OUTPUT_PLANE_1,
		.order = DMA_OUTPUT_ORDER_GB_BG,
		.err = DMA_OUTPUT_ERROR_NO,
	},
	.efd_output = {
		.cmd = DMA_OUTPUT_COMMAND_DISABLE,
		.width = DEFAULT_CAPTURE_STILL_WIDTH,
		.height = DEFAULT_CAPTURE_STILL_HEIGHT,
		.format = DMA_OUTPUT_FORMAT_YUV422,
		.bitwidth = DMA_OUTPUT_BIT_WIDTH_8BIT,
		.plane = DMA_OUTPUT_PLANE_2,
		.order = DMA_OUTPUT_ORDER_CbCr,
		.err = DMA_OUTPUT_ERROR_NO,
	},
	.mrg_output = {
		.cmd = DMA_OUTPUT_COMMAND_DISABLE,
		.width = DEFAULT_CAPTURE_STILL_WIDTH,
		.height = DEFAULT_CAPTURE_STILL_HEIGHT,
		.format = DMA_OUTPUT_FORMAT_BAYER,
		.bitwidth = DMA_OUTPUT_BIT_WIDTH_16BIT,
		.plane = DMA_OUTPUT_PLANE_1,
		.order = 0,
		.err = DMA_OUTPUT_ERROR_NO,
	},
	.ddma_output = {

		.cmd = DMA_OUTPUT_COMMAND_DISABLE,
	},
};

static const struct isp_param init_isp_param = {
	.control = {
		.cmd = CONTROL_COMMAND_START,
		.bypass = CONTROL_BYPASS_DISABLE,
		.err = CONTROL_ERROR_NO,
	},
	.otf_input = {
		.cmd = OTF_INPUT_COMMAND_DISABLE,
	},
	.vdma1_input = {
		.cmd = DMA_INPUT_COMMAND_DISABLE,
		.width = 0,
		.height = 0,
		.format = 0,
		.bitwidth = 0,
		.plane = 0,
		.order = 0,
		.err = 0,
	},
	.vdma3_input = {
		.cmd = DMA_INPUT_COMMAND_DISABLE,
	},
	.otf_output = {
		.cmd = OTF_OUTPUT_COMMAND_ENABLE,
		.width = DEFAULT_CAPTURE_STILL_WIDTH,
		.height = DEFAULT_CAPTURE_STILL_HEIGHT,
		.format = OTF_YUV_FORMAT,
		.bitwidth = OTF_OUTPUT_BIT_WIDTH_12BIT,
		.order = OTF_OUTPUT_ORDER_BAYER_GR_BG,
		.err = OTF_OUTPUT_ERROR_NO,
	},
	.vdma4_output = {
		.cmd = DMA_OUTPUT_COMMAND_DISABLE,
	},
	.vdma5_output = {
		.cmd = DMA_OUTPUT_COMMAND_DISABLE,
	},
};

static const struct drc_param init_drc_param = {
	.control = {
		.cmd = CONTROL_COMMAND_START,
		.bypass = CONTROL_BYPASS_ENABLE,
		.err = CONTROL_ERROR_NO,
	},
	.otf_input = {
		.cmd = OTF_INPUT_COMMAND_ENABLE,
		.width = DEFAULT_CAPTURE_STILL_WIDTH,
		.height = DEFAULT_CAPTURE_STILL_HEIGHT,
		.format = OTF_YUV_FORMAT,
		.bitwidth = OTF_INPUT_BIT_WIDTH_12BIT,
		.order = OTF_INPUT_ORDER_BAYER_GR_BG,
		.err = OTF_INPUT_ERROR_NO,
	},
	.dma_input = {
		.cmd = DMA_INPUT_COMMAND_DISABLE,
		.width = DEFAULT_CAPTURE_STILL_WIDTH,
		.height = DEFAULT_CAPTURE_STILL_HEIGHT,
		.format = DMA_INPUT_FORMAT_YUV444,
		.bitwidth = DMA_INPUT_BIT_WIDTH_8BIT,
		.plane = DMA_INPUT_PLANE_1,
		.order = DMA_INPUT_ORDER_YCbCr,
		.err = 0,
	},
	.otf_output = {
		.cmd = OTF_OUTPUT_COMMAND_ENABLE,
		.width = DEFAULT_CAPTURE_STILL_WIDTH,
		.height = DEFAULT_CAPTURE_STILL_HEIGHT,
		.format = OTF_OUTPUT_FORMAT_YUV444,
		.bitwidth = OTF_INPUT_BIT_WIDTH_12BIT,
		.order = OTF_OUTPUT_ORDER_BAYER_GR_BG,
		.err = OTF_OUTPUT_ERROR_NO,
	},
};

static const struct scc_param init_scc_param = {
	.control = {
		.cmd = CONTROL_COMMAND_START,
		.bypass = CONTROL_BYPASS_ENABLE,
		.err = CONTROL_ERROR_NO,
	},
	.otf_input = {
		.cmd = OTF_INPUT_COMMAND_ENABLE,
		.width = DEFAULT_CAPTURE_STILL_WIDTH,
		.height = DEFAULT_CAPTURE_STILL_HEIGHT,
		.format = OTF_YUV_FORMAT,
		.bitwidth = OTF_INPUT_BIT_WIDTH_12BIT,
		.order = OTF_INPUT_ORDER_BAYER_GR_BG,
		.bayer_crop_offset_x = 0,
		.bayer_crop_offset_y = 0,
		.bayer_crop_width = 0,
		.bayer_crop_height = 0,
		.err = OTF_INPUT_ERROR_NO,
	},
	.effect = {
		.cmd = 0,
		.arbitrary_cb = 128, /* default value : 128 */
		.arbitrary_cr = 128, /* default value : 128 */
		.yuv_range = SCALER_OUTPUT_YUV_RANGE_FULL,
		.err = 0,
	},
	.input_crop = {
		.cmd = OTF_INPUT_COMMAND_DISABLE,
		.pos_x = 0,
		.pos_y = 0,
		.crop_width = DEFAULT_CAPTURE_STILL_CROP_WIDTH,
		.crop_height = DEFAULT_CAPTURE_STILL_CROP_HEIGHT,
		.in_width = DEFAULT_CAPTURE_STILL_WIDTH,
		.in_height = DEFAULT_CAPTURE_STILL_HEIGHT,
		.out_width = DEFAULT_CAPTURE_VIDEO_WIDTH,
		.out_height = DEFAULT_CAPTURE_VIDEO_HEIGHT,
		.err = 0,
	},
	.output_crop = {
		.cmd = SCALER_CROP_COMMAND_DISABLE,
		.pos_x = 0,
		.pos_y = 0,
		.crop_width = DEFAULT_CAPTURE_STILL_WIDTH,
		.crop_height = DEFAULT_CAPTURE_STILL_HEIGHT,
		.format = DMA_OUTPUT_FORMAT_YUV422,
		.err = 0,
	},
	.otf_output = {
		.cmd = OTF_OUTPUT_COMMAND_ENABLE,
		.width = DEFAULT_CAPTURE_VIDEO_WIDTH,
		.height = DEFAULT_CAPTURE_VIDEO_HEIGHT,
		.format = OTF_YUV_FORMAT,
		.bitwidth = OTF_OUTPUT_BIT_WIDTH_8BIT,
		.order = OTF_OUTPUT_ORDER_BAYER_GR_BG,
		.err = OTF_OUTPUT_ERROR_NO,
	},
	.dma_output = {
		.cmd = DMA_OUTPUT_COMMAND_DISABLE,
		.width = DEFAULT_CAPTURE_STILL_WIDTH,
		.height = DEFAULT_CAPTURE_STILL_HEIGHT,
		.format = DMA_OUTPUT_FORMAT_YUV422,
		.bitwidth = DMA_OUTPUT_BIT_WIDTH_8BIT,
		.plane = DMA_OUTPUT_PLANE_1,
		.order = DMA_OUTPUT_ORDER_YCbYCr,
		.err = DMA_OUTPUT_ERROR_NO,
	},
};

static const struct odc_param init_odc_param = {
	.control = {
		.cmd = CONTROL_COMMAND_START,
		.bypass = CONTROL_BYPASS_ENABLE,
		.err = CONTROL_ERROR_NO,
	},
	.otf_input = {
		.cmd = OTF_INPUT_COMMAND_ENABLE,
		.width = DEFAULT_CAPTURE_VIDEO_WIDTH,
		.height = DEFAULT_CAPTURE_VIDEO_HEIGHT,
		.format = OTF_YUV_FORMAT,
		.bitwidth = OTF_INPUT_BIT_WIDTH_8BIT,
		.order = OTF_INPUT_ORDER_BAYER_GR_BG,
		.bayer_crop_offset_x = 0,
		.bayer_crop_offset_y = 0,
		.bayer_crop_width = 0,
		.bayer_crop_height = 0,
		.err = OTF_INPUT_ERROR_NO,
	},
	.otf_output = {
		.cmd = OTF_OUTPUT_COMMAND_ENABLE,
		.width = DEFAULT_CAPTURE_VIDEO_WIDTH,
		.height = DEFAULT_CAPTURE_VIDEO_HEIGHT,
		.format = OTF_YUV_FORMAT,
		.bitwidth = OTF_OUTPUT_BIT_WIDTH_8BIT,
		.order = OTF_OUTPUT_ORDER_BAYER_GR_BG,
		.err = OTF_OUTPUT_ERROR_NO,
	},
};

static const struct tpu_param init_tpu_param = {
	.control = {
		.cmd = CONTROL_COMMAND_STOP,
		.bypass = CONTROL_BYPASS_ENABLE,
		.err = CONTROL_ERROR_NO,
	},
	.config = {
		.odc_bypass = true,
		.dis_bypass = true,
		.tdnr_bypass = true,
		.err = 0
	},
	.dma_input = {
		.cmd = DMA_INPUT_COMMAND_DISABLE,
		.width = DEFAULT_CAPTURE_VIDEO_WIDTH,
		.height = DEFAULT_CAPTURE_VIDEO_HEIGHT,
		.format = DMA_INPUT_FORMAT_YUV420,
		.order = 0,
		.err = OTF_INPUT_ERROR_NO,
	},
	.otf_input = {
		.cmd = OTF_INPUT_COMMAND_DISABLE,
		.width = DEFAULT_CAPTURE_VIDEO_WIDTH,
		.height = DEFAULT_CAPTURE_VIDEO_HEIGHT,
		.format = OTF_YUV_FORMAT,
		.bitwidth = OTF_INPUT_BIT_WIDTH_8BIT,
		.order = OTF_INPUT_ORDER_BAYER_GR_BG,
		.bayer_crop_offset_x = 0,
		.bayer_crop_offset_y = 0,
		.bayer_crop_width = 0,
		.bayer_crop_height = 0,
		.err = OTF_INPUT_ERROR_NO,
	},
	.otf_output = {
		.cmd = OTF_OUTPUT_COMMAND_ENABLE,
		.width = DEFAULT_CAPTURE_VIDEO_WIDTH,
		.height = DEFAULT_CAPTURE_VIDEO_HEIGHT,
		.format = OTF_YUV_FORMAT,
		.bitwidth = OTF_OUTPUT_BIT_WIDTH_8BIT,
		.order = OTF_OUTPUT_ORDER_BAYER_GR_BG,
		.err = OTF_OUTPUT_ERROR_NO,
	},
};

static const struct tdnr_param init_tdnr_param = {
	.control = {
		.cmd = CONTROL_COMMAND_START,
		.bypass = CONTROL_BYPASS_ENABLE,
		.err = CONTROL_ERROR_NO,
	},
	.otf_input = {
		.cmd = OTF_INPUT_COMMAND_ENABLE,
		.width = DEFAULT_CAPTURE_VIDEO_WIDTH,
		.height = DEFAULT_CAPTURE_VIDEO_HEIGHT,
		.format = OTF_YUV_FORMAT,
		.bitwidth = OTF_INPUT_BIT_WIDTH_8BIT,
		.order = OTF_INPUT_ORDER_BAYER_GR_BG,
		.err = OTF_INPUT_ERROR_NO,
	},
	.frame = {
		.cmd = 0,
		.err = 0,
	},
	.otf_output = {
		.cmd = OTF_OUTPUT_COMMAND_ENABLE,
		.width = DEFAULT_CAPTURE_VIDEO_WIDTH,
		.height = DEFAULT_CAPTURE_VIDEO_HEIGHT,
		.format = OTF_YUV_FORMAT,
		.bitwidth = OTF_INPUT_BIT_WIDTH_8BIT,
		.order = OTF_OUTPUT_ORDER_BAYER_GR_BG,
		.err = OTF_OUTPUT_ERROR_NO,
	},
	.dma_output = {
		.cmd = DMA_OUTPUT_COMMAND_DISABLE,
		.width = DEFAULT_CAPTURE_VIDEO_WIDTH,
		.height = DEFAULT_CAPTURE_VIDEO_HEIGHT,
		.format = DMA_OUTPUT_FORMAT_YUV420,
		.bitwidth = DMA_OUTPUT_BIT_WIDTH_8BIT,
		.plane = DMA_OUTPUT_PLANE_2,
		.order = DMA_OUTPUT_ORDER_CrCb,
		.err = DMA_OUTPUT_ERROR_NO,
	},
};

#ifdef SOC_SCP
static const struct scp_param init_scp_param = {
	.control = {
		.cmd = CONTROL_COMMAND_START,
		.bypass = CONTROL_BYPASS_ENABLE,
		.err = CONTROL_ERROR_NO,
	},
	.otf_input = {
		.cmd = OTF_INPUT_COMMAND_ENABLE,
		.width = DEFAULT_CAPTURE_VIDEO_WIDTH,
		.height = DEFAULT_CAPTURE_VIDEO_HEIGHT,
		.format = OTF_YUV_FORMAT,
		.bitwidth = OTF_INPUT_BIT_WIDTH_8BIT,
		.order = OTF_INPUT_ORDER_BAYER_GR_BG,
		.bayer_crop_offset_x = 0,
		.bayer_crop_offset_y = 0,
		.bayer_crop_width = 0,
		.bayer_crop_height = 0,
		.err = OTF_INPUT_ERROR_NO,
	},
	.effect = {
		.cmd = 0,
		.arbitrary_cb = 128, /* default value : 128 */
		.arbitrary_cr = 128, /* default value : 128 */
		.yuv_range = SCALER_OUTPUT_YUV_RANGE_FULL,
		.err = 0,
	},
	.input_crop = {
		.cmd = OTF_INPUT_COMMAND_ENABLE,
		.pos_x = 0,
		.pos_y = 0,
		.crop_width = DEFAULT_CAPTURE_VIDEO_WIDTH,
		.crop_height = DEFAULT_CAPTURE_VIDEO_HEIGHT,
		.in_width = DEFAULT_CAPTURE_VIDEO_WIDTH,
		.in_height = DEFAULT_CAPTURE_VIDEO_HEIGHT,
		.out_width = DEFAULT_PREVIEW_STILL_WIDTH,
		.out_height = DEFAULT_PREVIEW_STILL_HEIGHT,
		.err = 0,
	},
	.output_crop = {
		.cmd = SCALER_CROP_COMMAND_DISABLE,
		.pos_x = 0,
		.pos_y = 0,
		.crop_width = DEFAULT_PREVIEW_STILL_WIDTH,
		.crop_height = DEFAULT_PREVIEW_STILL_HEIGHT,
		.format = OTF_OUTPUT_FORMAT_YUV420,
		.err = 0,
	},
	.rotation = {
		.cmd = 0,
		.err = 0,
	},
	.flip = {
		.cmd = 0,
		.err = 0,
	},
	.otf_output = {
		.cmd = OTF_OUTPUT_COMMAND_ENABLE,
		.width = DEFAULT_PREVIEW_STILL_WIDTH,
		.height = DEFAULT_PREVIEW_STILL_HEIGHT,
		.format = OTF_YUV_FORMAT,
		.bitwidth = OTF_INPUT_BIT_WIDTH_8BIT,
		.order = OTF_OUTPUT_ORDER_BAYER_GR_BG,
		.err = OTF_OUTPUT_ERROR_NO,
	},
	.dma_output = {
		.cmd = DMA_OUTPUT_COMMAND_DISABLE,
		.width = DEFAULT_PREVIEW_STILL_WIDTH,
		.height = DEFAULT_PREVIEW_STILL_HEIGHT,
		.format = OTF_OUTPUT_FORMAT_YUV420,
		.bitwidth = DMA_OUTPUT_BIT_WIDTH_8BIT,
		.plane = DMA_OUTPUT_PLANE_2,
		.order = DMA_OUTPUT_ORDER_CrCb,
		.selection = 0,
		.err = DMA_OUTPUT_ERROR_NO,
	},
};
#endif

#ifdef SOC_MCS
static const struct mcs_param init_mcs_param = {
	.control = {
		.cmd = CONTROL_COMMAND_START,
		.bypass = CONTROL_BYPASS_DISABLE,
		.err = CONTROL_ERROR_NO,
	},
	.input = {
		.otf_cmd = OTF_INPUT_COMMAND_ENABLE,
		.otf_format = OTF_INPUT_FORMAT_YUV422,
		.otf_bitwidth = OTF_INPUT_BIT_WIDTH_8BIT,
		.otf_order = OTF_INPUT_ORDER_BAYER_GR_BG,
		.dma_cmd = DMA_INPUT_COMMAND_DISABLE,
		.dma_format = DMA_INPUT_FORMAT_YUV422,
		.dma_bitwidth = DMA_INPUT_BIT_WIDTH_8BIT,
		.dma_order = DMA_INPUT_ORDER_YCbYCr,
		.plane = DMA_INPUT_PLANE_1,
		.width = 0,
		.height = 0,
		.dma_crop_offset_x = 0,
		.dma_crop_offset_y = 0,
		.dma_crop_width = 0,
		.dma_crop_height = 0,
		.err = 0,
	},
	.output[MCSC_OUTPUT0] = {
		.otf_cmd = OTF_OUTPUT_COMMAND_DISABLE,
		.otf_format = OTF_OUTPUT_FORMAT_YUV422,
		.otf_bitwidth = OTF_OUTPUT_BIT_WIDTH_8BIT,
		.otf_order = OTF_OUTPUT_ORDER_BAYER_GR_BG,
		.dma_cmd = DMA_OUTPUT_COMMAND_DISABLE,
		.dma_format = DMA_OUTPUT_FORMAT_YUV420,
		.dma_bitwidth = DMA_OUTPUT_BIT_WIDTH_8BIT,
		.dma_order = DMA_OUTPUT_ORDER_CrCb,
		.plane = 0,
		.crop_offset_x = 0,
		.crop_offset_y = 0,
		.crop_width = 0,
		.crop_height = 0,
		.width = 0,
		.height = 0,
		.dma_stride_y = 0,
		.dma_stride_c = 0,
		.yuv_range = SCALER_OUTPUT_YUV_RANGE_FULL,
		.flip = SCALER_FLIP_COMMAND_NORMAL,
		.hwfc = 0,
		.err = 0,
	},
	.output[MCSC_OUTPUT1] = {
		.otf_cmd = OTF_OUTPUT_COMMAND_DISABLE,
		.otf_format = OTF_OUTPUT_FORMAT_YUV422,
		.otf_bitwidth = OTF_OUTPUT_BIT_WIDTH_8BIT,
		.otf_order = OTF_OUTPUT_ORDER_BAYER_GR_BG,
		.dma_cmd = DMA_OUTPUT_COMMAND_DISABLE,
		.dma_format = DMA_OUTPUT_FORMAT_YUV420,
		.dma_bitwidth = DMA_OUTPUT_BIT_WIDTH_8BIT,
		.dma_order = DMA_OUTPUT_ORDER_CrCb,
		.plane = 0,
		.crop_offset_x = 0,
		.crop_offset_y = 0,
		.crop_width = 0,
		.crop_height = 0,
		.width = 0,
		.height = 0,
		.dma_stride_y = 0,
		.dma_stride_c = 0,
		.yuv_range = SCALER_OUTPUT_YUV_RANGE_FULL,
		.flip = SCALER_FLIP_COMMAND_NORMAL,
		.hwfc = 0,
		.err = 0,
	},
	.output[MCSC_OUTPUT2] = {
		.otf_cmd = OTF_OUTPUT_COMMAND_DISABLE,
		.otf_format = OTF_OUTPUT_FORMAT_YUV422,
		.otf_bitwidth = OTF_OUTPUT_BIT_WIDTH_8BIT,
		.otf_order = OTF_OUTPUT_ORDER_BAYER_GR_BG,
		.dma_cmd = DMA_OUTPUT_COMMAND_DISABLE,
		.dma_format = DMA_OUTPUT_FORMAT_YUV420,
		.dma_bitwidth = DMA_OUTPUT_BIT_WIDTH_8BIT,
		.dma_order = DMA_OUTPUT_ORDER_CrCb,
		.plane = 0,
		.crop_offset_x = 0,
		.crop_offset_y = 0,
		.crop_width = 0,
		.crop_height = 0,
		.width = 0,
		.height = 0,
		.dma_stride_y = 0,
		.dma_stride_c = 0,
		.yuv_range = SCALER_OUTPUT_YUV_RANGE_FULL,
		.flip = SCALER_FLIP_COMMAND_NORMAL,
		.hwfc = 0,
		.err = 0,
	},
	.output[MCSC_OUTPUT3] = {
		.otf_cmd = OTF_OUTPUT_COMMAND_DISABLE,
		.otf_format = OTF_OUTPUT_FORMAT_YUV422,
		.otf_bitwidth = OTF_OUTPUT_BIT_WIDTH_8BIT,
		.otf_order = OTF_OUTPUT_ORDER_BAYER_GR_BG,
		.dma_cmd = DMA_OUTPUT_COMMAND_DISABLE,
		.dma_format = DMA_OUTPUT_FORMAT_YUV420,
		.dma_bitwidth = DMA_OUTPUT_BIT_WIDTH_8BIT,
		.dma_order = DMA_OUTPUT_ORDER_CrCb,
		.plane = 0,
		.crop_offset_x = 0,
		.crop_offset_y = 0,
		.crop_width = 0,
		.crop_height = 0,
		.width = 0,
		.height = 0,
		.dma_stride_y = 0,
		.dma_stride_c = 0,
		.yuv_range = SCALER_OUTPUT_YUV_RANGE_FULL,
		.flip = SCALER_FLIP_COMMAND_NORMAL,
		.hwfc = 0,
		.err = 0,
	},
	.output[MCSC_OUTPUT4] = {
		.otf_cmd = OTF_OUTPUT_COMMAND_DISABLE,
		.otf_format = OTF_OUTPUT_FORMAT_YUV422,
		.otf_bitwidth = OTF_OUTPUT_BIT_WIDTH_8BIT,
		.otf_order = OTF_OUTPUT_ORDER_BAYER_GR_BG,
		.dma_cmd = DMA_OUTPUT_COMMAND_DISABLE,
		.dma_format = DMA_OUTPUT_FORMAT_YUV420,
		.dma_bitwidth = DMA_OUTPUT_BIT_WIDTH_8BIT,
		.dma_order = DMA_OUTPUT_ORDER_CrCb,
		.plane = 0,
		.crop_offset_x = 0,
		.crop_offset_y = 0,
		.crop_width = 0,
		.crop_height = 0,
		.width = 0,
		.height = 0,
		.dma_stride_y = 0,
		.dma_stride_c = 0,
		.yuv_range = SCALER_OUTPUT_YUV_RANGE_FULL,
		.flip = SCALER_FLIP_COMMAND_NORMAL,
		.hwfc = 0,
		.err = 0,
	},
};
#endif

static const struct vra_param init_vra_param = {
	.control = {
		.cmd = CONTROL_COMMAND_STOP,
		.bypass = CONTROL_BYPASS_DISABLE,
		.err = CONTROL_ERROR_NO,
	},
	.otf_input = {
		.cmd = OTF_INPUT_COMMAND_ENABLE,
		.width = DEFAULT_PREVIEW_STILL_WIDTH,
		.height = DEFAULT_PREVIEW_STILL_HEIGHT,
		.format = OTF_YUV_FORMAT,
		.bitwidth = OTF_INPUT_BIT_WIDTH_8BIT,
		.order = OTF_INPUT_ORDER_BAYER_GR_BG,
		.err = OTF_INPUT_ERROR_NO,
	},
	.dma_input = {
		.cmd = DMA_INPUT_COMMAND_DISABLE,
		.format = 0,
		.bitwidth = 0,
		.order = 0,
		.plane = 0,
		.width = 0,
		.height = 0,
		.err = 0,
	},
	.config = {
		.cmd = FD_CONFIG_COMMAND_MAXIMUM_NUMBER |
			FD_CONFIG_COMMAND_ROLL_ANGLE |
			FD_CONFIG_COMMAND_YAW_ANGLE |
			FD_CONFIG_COMMAND_SMILE_MODE |
			FD_CONFIG_COMMAND_BLINK_MODE |
			FD_CONFIG_COMMAND_EYES_DETECT |
			FD_CONFIG_COMMAND_MOUTH_DETECT |
			FD_CONFIG_COMMAND_ORIENTATION |
			FD_CONFIG_COMMAND_ORIENTATION_VALUE,
#ifdef ENABLE_FD_SW
		.mode = FD_CONFIG_MODE_HWONLY,
#else
		.mode = FD_CONFIG_MODE_NORMAL,
#endif
		.max_number = CAMERA2_MAX_FACES,
		.roll_angle = FD_CONFIG_ROLL_ANGLE_FULL,
		.yaw_angle = FD_CONFIG_YAW_ANGLE_45_90,
		.smile_mode = FD_CONFIG_SMILE_MODE_DISABLE,
		.blink_mode = FD_CONFIG_BLINK_MODE_DISABLE,
		.eye_detect = FD_CONFIG_EYES_DETECT_ENABLE,
		.mouth_detect = FD_CONFIG_MOUTH_DETECT_DISABLE,
		.orientation = FD_CONFIG_ORIENTATION_DISABLE,
		.orientation_value = 0,
#ifdef ENABLE_FD_SW
		.map_width = FD_MAP_WIDTH,
		.map_height = FD_MAP_HEIGHT,
#else
		.map_width = 0,
		.map_height = 0,
#endif
		.err = ERROR_FD_NO,
	},
};

#if !defined(DISABLE_SETFILE)
static void fimc_is_ischain_cache_flush(struct fimc_is_device_ischain *this,
	u32 offset, u32 size)
{
#ifdef ENABLE_IS_CORE
	CALL_BUFOP(this->minfo->pb_fw, sync_for_device,
		this->minfo->pb_fw, offset, size, DMA_TO_DEVICE);
#endif
}
#endif

static void fimc_is_ischain_region_invalid(struct fimc_is_device_ischain *device)
{
#ifdef ENABLE_IS_CORE
	CALL_BUFOP(device->minfo->pb_fw, sync_for_cpu,
		device->minfo->pb_fw,
		(FW_MEM_SIZE - ((device->instance + 1) * PARAM_REGION_SIZE)),
		sizeof(struct is_region),
		DMA_FROM_DEVICE);
#endif
}

static void fimc_is_ischain_region_flush(struct fimc_is_device_ischain *device)
{
#ifdef ENABLE_IS_CORE
	CALL_BUFOP(device->minfo->pb_fw, sync_for_device,
		device->minfo->pb_fw,
		(FW_MEM_SIZE - ((device->instance + 1) * PARAM_REGION_SIZE)),
		sizeof(struct is_region),
		DMA_TO_DEVICE);
#endif
}

void fimc_is_ischain_meta_flush(struct fimc_is_frame *frame)
{

}

void fimc_is_ischain_meta_invalid(struct fimc_is_frame *frame)
{

}

void fimc_is_ischain_savefirm(struct fimc_is_device_ischain *this)
{
#ifdef DEBUG_DUMP_FIRMWARE
	loff_t pos;

	CALL_BUFOP(this->minfo->pb_fw, sync_for_cpu,
		this->minfo->pb_fw,
		0,
		(size_t)FW_MEM_SIZE,
		DMA_FROM_DEVICE);

	write_data_to_file("/data/firmware.bin", (char *)this->minfo->kvaddr,
		(size_t)FW_MEM_SIZE, &pos);
#endif
}

#ifdef ENABLE_IS_CORE
static int fimc_is_ischain_loadfirm(struct fimc_is_device_ischain *device,
	struct fimc_is_vender *vender)
{
	const struct firmware *fw_blob = NULL;
	u8 *buf = NULL;
	int ret = 0;
	int retry_count = 0;
	struct file *fp = NULL;
	mm_segment_t old_fs;
	long fsize, nread;
	int fw_requested = 1;
	int fw_load_ret = 0;

	mdbgd_ischain("%s\n", device, __func__);

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(vender->fw_path, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		fw_load_ret = fimc_is_vender_fw_filp_open(vender, &fp, IS_BIN_FW);
		if (fw_load_ret == FW_SKIP) {
			goto request_fw;
		} else if (fw_load_ret == FW_FAIL) {
			fw_requested = 0;
			goto out;
		}
	}

	fw_requested = 0;
	fsize = fp->f_path.dentry->d_inode->i_size;
	info("start, file path %s, size %ld Bytes\n",vender->fw_path, fsize);

	buf = vmalloc(fsize);
	if (!buf) {
		dev_err(&device->pdev->dev,
			"failed to allocate memory\n");
		ret = -ENOMEM;
		goto out;
	}
	nread = vfs_read(fp, (char __user *)buf, fsize, &fp->f_pos);
	if (nread != fsize) {
		dev_err(&device->pdev->dev,
			"failed to read firmware file, %ld Bytes\n", nread);
		ret = -EIO;
		goto out;
	}

	memcpy((void *)device->minfo->kvaddr, (void *)buf, fsize);
	fimc_is_ischain_cache_flush(device, 0, fsize + 1);

request_fw:
	if (fw_requested) {
		set_fs(old_fs);
		retry_count = 3;

		ret = request_firmware(&fw_blob, vender->request_fw_path, &device->pdev->dev);
		while (--retry_count && ret == -EAGAIN) {
			err("request_firmware retry(count:%d)", retry_count);
			ret = request_firmware(&fw_blob, vender->request_fw_path, &device->pdev->dev);
		}

		if (ret) {
			err("request_firmware(%s) is fail(%d)", vender->request_fw_path, ret);
			ret = -EINVAL;
			goto out;
		}

		if (!fw_blob) {
			merr("fw_blob is NULL", device);
			ret = -EINVAL;
			goto out;
		}

		if (!fw_blob->data) {
			merr("fw_blob->data is NULL", device);
			ret = -EINVAL;
			goto out;
		}

		memcpy((void *)device->minfo->kvaddr, fw_blob->data, fw_blob->size);
		fimc_is_ischain_cache_flush(device, 0, fw_blob->size + 1);
	}

out:
	if (!fw_requested) {
		if (buf) {
			vfree(buf);
		}
		if (!IS_ERR_OR_NULL(fp)) {
			filp_close(fp, current->files);
		}
		set_fs(old_fs);
	} else {
		if (fw_blob)
			release_firmware(fw_blob);
	}
	if (ret)
		err("firmware loading is fail");
	else
		info("Camera: the FW were applied successfully.\n");

	return ret;
}
#endif

#ifdef ENABLE_FD_SW
int fimc_is_ischain_loadfd(struct fimc_is_device_ischain *device)
{
	int ret = 0;
	u8 *buf = NULL;
	int fw_requested = 1;
	int retry_count = 0;
	long fsize, nread;
	ulong *fd_bin = NULL;
	struct file *fp = NULL;
	const struct firmware *fw_blob = NULL;
	mm_segment_t old_fs;
	FD_INFO *fd_info;
	fd_lib_str *fd_lib_func = NULL;

	FIMC_BUG(!device);
#ifdef FD_USE_SHARED_REGION
	fd_bin = (ulong *)fd_vaddr;
#else
	fd_bin = FD_LIB_BASE;
#endif
	mdbgd_ischain("%s: binary address: %#lx\n", device, __func__, (ulong)fd_bin);

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(FD_SW_SDCARD, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		mdbgd_ischain("%s request fd library\n", device, __func__);
		goto request_fw;
	}

	fw_requested = 0;
	fsize = fp->f_path.dentry->d_inode->i_size;
	mdbgd_ischain("start, file path %s, size %ld Bytes\n",
			device, "/data/fimc_is_fd.bin", fsize);

	buf = vmalloc(fsize);
	if (!buf) {
		dev_err(&device->pdev->dev,
				"failed to allocate memory\n");
		ret = -ENOMEM;
		goto err;
	}

	nread = vfs_read(fp, (char __user *)buf, fsize, &fp->f_pos);
	if (nread != fsize) {
		dev_err(&device->pdev->dev,
				"failed to binary file, %ld Bytes\n", nread);
		ret = -EIO;
		goto err;
	}

	memcpy((void *)fd_bin, (void *)buf, fsize);

request_fw:
	if (fw_requested) {
		set_fs(old_fs);

		retry_count = 4;

		ret = request_firmware(&fw_blob, FD_SW_BIN_NAME, &device->pdev->dev);
		while (--retry_count && ret) {
			mwarn("request_fd_library retry(count:%d)", device, retry_count);
			ret = request_firmware(&fw_blob, FD_SW_BIN_NAME, &device->pdev->dev);
		}

		if (ret) {
			merr("request_firmware is fail(%d)", device, ret);
			ret = -EINVAL;
			goto err;
		}

		if (!fw_blob) {
			merr("fw_blob is NULL\n", device);
			ret = -EINVAL;
			goto err;
		}

		if (!fw_blob->data) {
			merr("fw_blob data is NULL\n", device);
			ret = -EINVAL;
			goto err;
		}

		memcpy((void *)fd_bin, fw_blob->data, fw_blob->size);
	}
err:
	if (!fw_requested) {
		if (buf) {
			vfree(buf);
		}
		if (!IS_ERR_OR_NULL(fp)) {
			filp_close(fp, current->files);
		}
		set_fs(old_fs);
	} else {
		if (fw_blob)
			release_firmware(fw_blob);
	}

	if (ret) {
		err("FD library loading is fail");
	} else {
		fd_lib_func = ((fd_lib_func_t)(fd_bin))((void **)NULL);
		fd_info = fd_lib_func->fd_version_get_func();
		info("fimc_is_fd.bin version: %#x %s\n",
				fd_info->apiVer, fd_info->buildDate);
	}

	return ret;
}
#endif

#if !defined(DISABLE_SETFILE)
static int fimc_is_ischain_loadsetf(struct fimc_is_device_ischain *device,
	struct fimc_is_vender *vender,
	ulong load_addr)
{
	int ret = 0;
	void *address;
	const struct firmware *fw_blob = NULL;
	u8 *buf = NULL;
	struct file *fp = NULL;
	mm_segment_t old_fs;
	long fsize, nread;
	int fw_requested = 1;
	u32 retry;
	int fw_load_ret = 0;
	int position;

	mdbgd_ischain("%s\n", device, __func__);

	if (IS_ERR_OR_NULL(device->sensor)) {
		err("sensor device is NULL");
		ret = -EINVAL;
		goto out;
	}

	position = device->sensor->position;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(vender->setfile_path[position], O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		fw_load_ret = fimc_is_vender_fw_filp_open(vender, &fp, IS_BIN_SETFILE);
		if (fw_load_ret == FW_SKIP) {
			goto request_fw;
		} else if (fw_load_ret == FW_FAIL) {
			fw_requested = 0;
			goto out;
		}
	}

	fw_requested = 0;
	fsize = fp->f_path.dentry->d_inode->i_size;
	info("start(%d), file path %s, size %ld Bytes\n",
		fw_load_ret, vender->setfile_path[position], fsize);

	buf = vmalloc(fsize);
	if (!buf) {
		dev_err(&device->pdev->dev,
			"failed to allocate memory\n");
		ret = -ENOMEM;
		goto out;
	}
	nread = vfs_read(fp, (char __user *)buf, fsize, &fp->f_pos);
	if (nread != fsize) {
		dev_err(&device->pdev->dev,
			"failed to read firmware file, %ld Bytes\n", nread);
		ret = -EIO;
		goto out;
	}

	address = (void *)(device->minfo->kvaddr + load_addr);
	memcpy((void *)address, (void *)buf, fsize);
	fimc_is_ischain_cache_flush(device, load_addr, fsize + 1);
	carve_binary_version(IS_BIN_SETFILE, position, buf, fsize);

request_fw:
	if (fw_requested) {
		set_fs(old_fs);

		retry = 4;
		ret = request_firmware((const struct firmware **)&fw_blob,
			vender->request_setfile_path[position], &device->pdev->dev);
		while (--retry && ret) {
			mwarn("request_firmware is fail(%d)", device, ret);
			ret = request_firmware((const struct firmware **)&fw_blob,
				vender->request_setfile_path[position], &device->pdev->dev);
		}

		if (!retry) {
			merr("request_firmware is fail(%d)", device, ret);
			ret = -EINVAL;
			goto out;
		}

		if (!fw_blob) {
			merr("fw_blob is NULL", device);
			ret = -EINVAL;
			goto out;
		}

		if (!fw_blob->data) {
			merr("fw_blob->data is NULL", device);
			ret = -EINVAL;
			goto out;
		}

		address = (void *)(device->minfo->kvaddr + load_addr);
		memcpy(address, fw_blob->data, fw_blob->size);
		fimc_is_ischain_cache_flush(device, load_addr, fw_blob->size + 1);
		carve_binary_version(IS_BIN_SETFILE, position,
				(void *)fw_blob->data, fw_blob->size);
	}

out:
	if (!fw_requested) {
		if (buf) {
			vfree(buf);
		}
		if (!IS_ERR_OR_NULL(fp)) {
			filp_close(fp, current->files);
		}
		set_fs(old_fs);
	} else {
		if (fw_blob)
			release_firmware(fw_blob);
	}

	if (ret)
		err("setfile loading is fail");
	else
		info("Camera: the Setfile were applied successfully.\n");

	return ret;
}
#endif

#ifdef ENABLE_IS_CORE
static int fimc_is_ischain_config_secure(struct fimc_is_device_ischain *device)
{
	int ret = 0;

#ifdef CONFIG_ARM_TRUSTZONE
	u32 i;

	exynos_smc(SMC_CMD_REG, SMC_REG_ID_SFR_W(PA_FIMC_IS_GIC_C + 0x4), 0x000000FF, 0);
	for (i = 0; i < 5; i++)
		exynos_smc(SMC_CMD_REG, SMC_REG_ID_SFR_W(PA_FIMC_IS_GIC_D + 0x80 + (i * 4)), 0xFFFFFFFF, 0);
	for (i = 0; i < 40; i++)
		exynos_smc(SMC_CMD_REG, SMC_REG_ID_SFR_W(PA_FIMC_IS_GIC_D + 0x400 + (i * 4)), 0x10101010, 0);

#ifdef CONFIG_ARM64
	{
		ulong debug;
		exynos_smc_readsfr(PA_FIMC_IS_GIC_C + 0x4, &debug);
		info("%s : PA_FIMC_IS_GIC_C : 0x%08lx\n", __func__, debug);
		if (debug == 0x00) {
			merr("secure configuration is fail[0x131E0004:%08lX]", device, debug);
			ret = -EINVAL;
		}
	}
#else
	{
		u32 debug;
		exynos_smc_readsfr(PA_FIMC_IS_GIC_C + 0x4, &debug);
		info("%s : PA_FIMC_IS_GIC_C : 0x%08x\n", __func__, debug);
		if (debug == 0x00) {
			merr("secure configuration is fail[0x131E0004:%08X]", device, debug);
			ret = -EINVAL;
		}
	}
#endif
#endif

	return ret;
}
#endif

static u32 fimc_is_itf_g_group_info(struct fimc_is_device_ischain *device,
	struct fimc_is_path_info *path)
{
	u32 group = 0;

	if (path->group[GROUP_SLOT_PAF] != GROUP_ID_MAX)
		group |= (GROUP_ID(path->group[GROUP_SLOT_PAF]) & GROUP_ID_PARM_MASK);

	if (path->group[GROUP_SLOT_3AA] != GROUP_ID_MAX)
		group |= (GROUP_ID(path->group[GROUP_SLOT_3AA]) & GROUP_ID_PARM_MASK);

	if (path->group[GROUP_SLOT_ISP] != GROUP_ID_MAX)
		group |= (GROUP_ID(path->group[GROUP_SLOT_ISP]) & GROUP_ID_PARM_MASK);

	if (path->group[GROUP_SLOT_DIS] != GROUP_ID_MAX)
		group |= (GROUP_ID(path->group[GROUP_SLOT_DIS]) & GROUP_ID_PARM_MASK);

	if (path->group[GROUP_SLOT_DCP] != GROUP_ID_MAX)
		group |= (GROUP_ID(path->group[GROUP_SLOT_DCP]) & GROUP_ID_PARM_MASK);

	if (path->group[GROUP_SLOT_MCS] != GROUP_ID_MAX)
		group |= (GROUP_ID(path->group[GROUP_SLOT_MCS]) & GROUP_ID_PARM_MASK);

	if (path->group[GROUP_SLOT_VRA] != GROUP_ID_MAX)
		group |= (GROUP_ID(path->group[GROUP_SLOT_VRA]) & GROUP_ID_PARM_MASK);

	return group;
}

int fimc_is_itf_s_param(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame,
	u32 lindex,
	u32 hindex,
	u32 indexes)
{
	int ret = 0;
	u32 flag, index;
	ulong dst_base, src_base;

	FIMC_BUG(!device);

	if (frame) {
		frame->lindex |= lindex;
		frame->hindex |= hindex;

		dst_base = (ulong)&device->is_region->parameter;
		src_base = (ulong)frame->shot->ctl.vendor_entry.parameter;

		frame->shot->ctl.vendor_entry.lowIndexParam |= lindex;
		frame->shot->ctl.vendor_entry.highIndexParam |= hindex;

		for (index = 0; lindex && (index < 32); index++) {
			flag = 1 << index;
			if (lindex & flag) {
				memcpy((ulong *)(dst_base + (index * PARAMETER_MAX_SIZE)),
					(ulong *)(src_base + (index * PARAMETER_MAX_SIZE)),
					PARAMETER_MAX_SIZE);
				lindex &= ~flag;
			}
		}

		for (index = 0; hindex && (index < 32); index++) {
			flag = 1 << index;
			if (hindex & flag) {
				memcpy((u32 *)(dst_base + ((32 + index) * PARAMETER_MAX_SIZE)),
					(u32 *)(src_base + ((32 + index) * PARAMETER_MAX_SIZE)),
					PARAMETER_MAX_SIZE);
				hindex &= ~flag;
			}
		}

		fimc_is_ischain_region_flush(device);
	} else {
		/*
		 * this check code is commented until per-frame control is worked fully
		 *
		 * if ( test_bit(FIMC_IS_ISCHAIN_START, &device->state)) {
		 *	merr("s_param is fail, device already is started", device);
		 *	BUG();
		 * }
		 */

		fimc_is_ischain_region_flush(device);

		if (lindex || hindex) {
			ret = fimc_is_itf_s_param_wrap(device,
				lindex,
				hindex,
				indexes);
		}
	}

	return ret;
}

void * fimc_is_itf_g_param(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame,
	u32 index)
{
	ulong dst_base, src_base, dst_param, src_param;

	FIMC_BUG_NULL(!device);

	if (frame) {
		dst_base = (ulong)&frame->shot->ctl.vendor_entry.parameter[0];
		dst_param = (dst_base + (index * PARAMETER_MAX_SIZE));
		src_base = (ulong)&device->is_region->parameter;
		src_param = (src_base + (index * PARAMETER_MAX_SIZE));
		memcpy((ulong *)dst_param, (ulong *)src_param, PARAMETER_MAX_SIZE);
	} else {
		dst_base = (ulong)&device->is_region->parameter;
		dst_param = (dst_base + (index * PARAMETER_MAX_SIZE));
	}

	return (void *)dst_param;
}

int fimc_is_itf_a_param(struct fimc_is_device_ischain *device,
	u32 group)
{
	int ret = 0;

	FIMC_BUG(!device);

	ret = fimc_is_itf_a_param_wrap(device, group);

	return ret;
}

static int fimc_is_itf_f_param(struct fimc_is_device_ischain *device)
{
	int ret = 0;
	u32 group = 0;
	struct fimc_is_path_info *path;

	path = &device->path;
	group = fimc_is_itf_g_group_info(device, path);

	ret = fimc_is_itf_f_param_wrap(device, group);

	return ret;
}

static int fimc_is_itf_enum(struct fimc_is_device_ischain *device)
{
	int ret = 0;

	mdbgd_ischain("%s()\n", device, __func__);

	ret = fimc_is_itf_enum_wrap(device);

	return ret;
}

void fimc_is_itf_storefirm(struct fimc_is_device_ischain *device)
{
	mdbgd_ischain("%s()\n", device, __func__);

	fimc_is_itf_storefirm_wrap(device);
}

void fimc_is_itf_restorefirm(struct fimc_is_device_ischain *device)
{
	mdbgd_ischain("%s()\n", device, __func__);

	fimc_is_itf_restorefirm_wrap(device);
}

int fimc_is_itf_set_fwboot(struct fimc_is_device_ischain *device, u32 val)
{
	int ret = 0;

	mdbgd_ischain("%s()\n", device, __func__);

	ret = fimc_is_itf_set_fwboot_wrap(device, val);

	return ret;
}

static void fimc_is_itf_param_init(struct is_region *region)
{
	memset(&region->parameter, 0x0, sizeof(struct is_param_region));

	memcpy(&region->parameter.sensor, &init_sensor_param,
		sizeof(struct sensor_param));
	memcpy(&region->parameter.taa, &init_taa_param,
		sizeof(struct taa_param));
	memcpy(&region->parameter.isp, &init_isp_param,
		sizeof(struct isp_param));
/* TODO */
#if 0
	memcpy(&region->parameter.drc, &init_drc_param,
		sizeof(struct drc_param));
#endif
#ifdef SOC_SCC
	memcpy(&region->parameter.scalerc, &init_scc_param,
		sizeof(struct scc_param));
#endif
	memcpy(&region->parameter.tpu, &init_tpu_param,
		sizeof(struct tpu_param));
#ifdef SOC_SCP
	memcpy(&region->parameter.scalerp, &init_scp_param,
		sizeof(struct scp_param));
#endif
#ifdef SOC_MCS
	memcpy(&region->parameter.mcs, &init_mcs_param,
		sizeof(struct mcs_param));
#endif
	memcpy(&region->parameter.vra, &init_vra_param,
		sizeof(struct vra_param));
}

static int fimc_is_itf_open(struct fimc_is_device_ischain *device,
	u32 module_id,
	u32 flag,
	struct fimc_is_path_info *path,
	struct sensor_open_extended *ext_info)
{
	int ret = 0;
	u32 offset_ext, offset_path;
	struct is_region *region;
	struct fimc_is_core *core;
	struct fimc_is_vender *vender;

	FIMC_BUG(!device);
	FIMC_BUG(!device->is_region);
	FIMC_BUG(!device->sensor);
	FIMC_BUG(!device->interface);
	FIMC_BUG(!ext_info);

	if (test_bit(FIMC_IS_ISCHAIN_OPEN_STREAM, &device->state)) {
		merr("stream is already open", device);
		ret = -EINVAL;
		goto p_err;
	}

	region = device->is_region;
	offset_ext = 0;

	core = (struct fimc_is_core *)platform_get_drvdata(device->pdev);
	vender = &core->vender;
	fimc_is_vender_itf_open(vender, ext_info);

	memcpy(&region->shared[offset_ext], ext_info, sizeof(struct sensor_open_extended));

	offset_path = (sizeof(struct sensor_open_extended) / 4) + 1;
	memcpy(&region->shared[offset_path], path, sizeof(struct fimc_is_path_info));

	fimc_is_ischain_region_flush(device);

	ret = fimc_is_itf_open_wrap(device,
		module_id,
		flag,
		offset_path);
	if (ret)
		goto p_err;

	mdbgd_ischain("margin %dx%d\n", device, device->margin_width, device->margin_height);

	fimc_is_ischain_region_invalid(device);

	if (region->shared[MAX_SHARED_COUNT-1] != MAGIC_NUMBER) {
		merr("MAGIC NUMBER error", device);
		ret = -EINVAL;
		goto p_err;
	}

	set_bit(FIMC_IS_ISCHAIN_OPEN_STREAM, &device->state);

p_err:
	return ret;
}

static int fimc_is_itf_close(struct fimc_is_device_ischain *device)
{
	int ret = 0;

	FIMC_BUG(!device);
	FIMC_BUG(!device->interface);

	if (!test_bit(FIMC_IS_ISCHAIN_OPEN_STREAM, &device->state)) {
		mwarn("stream is already close", device);
		goto p_err;
	}

	ret = fimc_is_itf_close_wrap(device);

	clear_bit(FIMC_IS_ISCHAIN_OPEN_STREAM, &device->state);

p_err:
	return ret;
}

static DEFINE_MUTEX(setf_lock);
static int fimc_is_itf_setfile(struct fimc_is_device_ischain *device,
	struct fimc_is_vender *vender)
{
	int ret = 0;
	ulong setfile_addr = 0;
	struct fimc_is_interface *itf;

	FIMC_BUG(!device);
	FIMC_BUG(!device->interface);
	FIMC_BUG(!vender);

	itf = device->interface;

	mutex_lock(&setf_lock);

#ifdef FW_SUSPEND_RESUME
	if (test_bit(IS_IF_RESUME, &itf->fw_boot) && device->sensor->position == SENSOR_POSITION_REAR)
		goto p_err;
#endif
	ret = fimc_is_itf_setaddr_wrap(itf, device, &setfile_addr);
	if (ret) {
		merr("fimc_is_hw_saddr is fail(%d)", device, ret);
		goto p_err;
	}

#if !defined(DISABLE_SETFILE)
	if (!setfile_addr) {
		merr("setfile address is NULL", device);
		pr_err("cmd : %08X\n", readl(&itf->com_regs->ihcmd));
		pr_err("id : %08X\n", readl(&itf->com_regs->ihc_stream));
		pr_err("param1 : %08X\n", readl(&itf->com_regs->ihc_param1));
		pr_err("param2 : %08X\n", readl(&itf->com_regs->ihc_param2));
		pr_err("param3 : %08X\n", readl(&itf->com_regs->ihc_param3));
		pr_err("param4 : %08X\n", readl(&itf->com_regs->ihc_param4));
		goto p_err;
	}

	mdbgd_ischain("%s(0x%08lX)\n", device, __func__, setfile_addr);

	ret = fimc_is_ischain_loadsetf(device, vender, setfile_addr);
	if (ret) {
		merr("fimc_is_ischain_loadsetf is fail(%d)", device, ret);
		goto p_err;
	}
#endif

	ret = fimc_is_itf_setfile_wrap(itf, (device->minfo->kvaddr + setfile_addr), device);
	if (ret)
		goto p_err;

p_err:
	mutex_unlock(&setf_lock);
	return ret;
}

#ifdef ENABLE_IS_CORE
int fimc_is_itf_map(struct fimc_is_device_ischain *device,
	u32 group, dma_addr_t shot_addr, size_t shot_size)
{
	int ret = 0;

	FIMC_BUG(!device);

	mdbgd_ischain("%s()\n", device, __func__);

	ret = fimc_is_itf_map_wrap(device, group, (u32)shot_addr,
						(u32)shot_size);

	return ret;
}

static int fimc_is_itf_unmap(struct fimc_is_device_ischain *device,
	u32 group)
{
	int ret = 0;

	mdbgd_ischain("%s()\n", device, __func__);

	ret = fimc_is_itf_unmap_wrap(device, group);

	return ret;
}
#endif

int fimc_is_itf_stream_on(struct fimc_is_device_ischain *device)
{
	int ret = 0;
	u32 retry = 30000;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group_leader;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_resourcemgr *resourcemgr;
	u32 scount, init_shots, qcount;

	FIMC_BUG(!device);
	FIMC_BUG(!device->groupmgr);
	FIMC_BUG(!device->resourcemgr);
	FIMC_BUG(!device->sensor);
	FIMC_BUG(!device->sensor->pdata);

	groupmgr = device->groupmgr;
	resourcemgr = device->resourcemgr;
	group_leader = groupmgr->leader[device->instance];
	if (!group_leader) {
		merr("stream leader is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	init_shots = group_leader->init_shots;
	scount = atomic_read(&group_leader->scount);

	framemgr = GET_HEAD_GROUP_FRAMEMGR(group_leader);
	if (!framemgr) {
		merr("leader framemgr is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	qcount = framemgr->queued_count[FS_REQUEST] + framemgr->queued_count[FS_PROCESS];

	if (init_shots) {
		/* 3ax  group should be started */
		if (!test_bit(FIMC_IS_GROUP_START, &group_leader->state)) {
			merr("stream leader is NOT started", device);
			ret = -EINVAL;
			goto p_err;
		}

		while (--retry && (scount < init_shots)) {
			udelay(100);
			scount = atomic_read(&group_leader->scount);
		}
	}

	if (retry)
		minfo("[ISC:D] stream on ready(%d, %d, %d)\n", device, scount, init_shots, qcount);
	else
		merr("[ISC:D] stream on NOT ready(%d, %d, %d)\n", device, scount, init_shots, qcount);

#ifdef ENABLE_DVFS
	if ((!pm_qos_request_active(&device->user_qos)) && (sysfs_debug.en_dvfs)) {
		struct fimc_is_dvfs_ctrl *dvfs_ctrl;
		int scenario_id;

		dvfs_ctrl = &resourcemgr->dvfs_ctrl;

		mutex_lock(&dvfs_ctrl->lock);

		/* try to find dynamic scenario to apply */
		scenario_id = fimc_is_dvfs_sel_static(device);
		if (scenario_id >= 0) {
			struct fimc_is_dvfs_scenario_ctrl *static_ctrl = dvfs_ctrl->static_ctrl;
			minfo("[ISC:D] tbl[%d] static scenario(%d)-[%s]\n", device,
				dvfs_ctrl->dvfs_table_idx, scenario_id,
				static_ctrl->scenarios[static_ctrl->cur_scenario_idx].scenario_nm);
			fimc_is_set_dvfs((struct fimc_is_core *)device->interface->core, device, scenario_id);
		}

		mutex_unlock(&dvfs_ctrl->lock);
	}
#endif

	fimc_is_resource_set_global_param(resourcemgr, device);

	if (debug_clk > 0)
		CALL_POPS(device, print_clk);

	ret = fimc_is_itf_stream_on_wrap(device);

p_err:
	return ret;
}

int fimc_is_itf_stream_off(struct fimc_is_device_ischain *device)
{
	int ret = 0;
	struct fimc_is_resourcemgr *resourcemgr;

	FIMC_BUG(!device);

	minfo("[ISC:D] stream off ready\n", device);

	ret = fimc_is_itf_stream_off_wrap(device);

	resourcemgr = device->resourcemgr;
	fimc_is_resource_clear_global_param(resourcemgr, device);

	return ret;
}

int fimc_is_itf_process_start(struct fimc_is_device_ischain *device,
	u32 group)
{
	int ret = 0;

	ret = fimc_is_itf_process_on_wrap(device, group);

	return ret;
}

int fimc_is_itf_process_stop(struct fimc_is_device_ischain *device,
	u32 group)
{
	int ret = 0;

#ifdef ENABLE_CLOCK_GATE
	struct fimc_is_core *core = (struct fimc_is_core *)device->interface->core;
	if (sysfs_debug.en_clk_gate &&
			sysfs_debug.clk_gate_mode == CLOCK_GATE_MODE_HOST) {
		fimc_is_clk_gate_lock_set(core, device->instance, true);
		fimc_is_wrap_clk_gate_set(core, (1 << GROUP_ID_MAX) - 1, true);
	}
#endif

	ret = fimc_is_itf_process_off_wrap(device, group, 0);

#ifdef ENABLE_CLOCK_GATE
	if (sysfs_debug.en_clk_gate &&
		sysfs_debug.clk_gate_mode == CLOCK_GATE_MODE_HOST)
		fimc_is_clk_gate_lock_set(core, device->instance, false);
#endif

	return ret;
}

int fimc_is_itf_force_stop(struct fimc_is_device_ischain *device,
	u32 group)
{
	int ret = 0;

#ifdef ENABLE_CLOCK_GATE
	struct fimc_is_core *core = (struct fimc_is_core *)device->interface->core;
#endif

#ifdef ENABLE_CLOCK_GATE
	if (sysfs_debug.en_clk_gate &&
			sysfs_debug.clk_gate_mode == CLOCK_GATE_MODE_HOST) {
		fimc_is_clk_gate_lock_set(core, device->instance, true);
		fimc_is_wrap_clk_gate_set(core, (1 << GROUP_ID_MAX) - 1, true);
	}
#endif

	ret = fimc_is_itf_process_off_wrap(device, group, 1);

#ifdef ENABLE_CLOCK_GATE
	if (sysfs_debug.en_clk_gate &&
		sysfs_debug.clk_gate_mode == CLOCK_GATE_MODE_HOST)
		fimc_is_clk_gate_lock_set(core, device->instance, false);
#endif

	return ret;
}

static int fimc_is_itf_init_process_start(struct fimc_is_device_ischain *device)
{
	int ret = 0;
	u32 group = 0;
	struct fimc_is_path_info *path;

	path = &device->path;
	group = fimc_is_itf_g_group_info(device, path);

	ret = fimc_is_itf_process_on_wrap(device, group);

	return ret;
}

#ifdef ENABLE_IS_CORE
static int fimc_is_itf_init_process_stop(struct fimc_is_device_ischain *device)
{
	int ret = 0;
	u32 group = 0;
	struct fimc_is_path_info *path;
#ifdef ENABLE_CLOCK_GATE
	struct fimc_is_core *core = (struct fimc_is_core *)device->interface->core;
#endif

	path = &device->path;
	group = fimc_is_itf_g_group_info(device, path);

#ifdef ENABLE_CLOCK_GATE
	if (sysfs_debug.en_clk_gate &&
			sysfs_debug.clk_gate_mode == CLOCK_GATE_MODE_HOST) {
		fimc_is_clk_gate_lock_set(core, device->instance, true);
		fimc_is_wrap_clk_gate_set(core, (1 << GROUP_ID_MAX) - 1, true);
	}
#endif

	ret = fimc_is_itf_process_off_wrap(device, group, 0);

#ifdef ENABLE_CLOCK_GATE
	if (sysfs_debug.en_clk_gate &&
		sysfs_debug.clk_gate_mode == CLOCK_GATE_MODE_HOST)
		fimc_is_clk_gate_lock_set(core, device->instance, false);
#endif
	return ret;
}
#endif

int fimc_is_itf_i2c_lock(struct fimc_is_device_ischain *this,
	int i2c_clk, bool lock)
{
	int ret = 0;
	struct fimc_is_interface *itf = this->interface;

	if (lock)
		fimc_is_interface_lock(itf);

	ret = fimc_is_hw_i2c_lock(itf, this->instance, i2c_clk, lock);

	if (!lock)
		fimc_is_interface_unlock(itf);

	return ret;
}

static int fimc_is_itf_g_capability(struct fimc_is_device_ischain *this)
{
	int ret = 0;
#ifdef PRINT_CAPABILITY
	u32 metadata;
	u32 index;
	struct camera2_sm *capability;
#endif

	ret = fimc_is_hw_g_capability(this->interface, this->instance,
		(u32)(this->kvaddr_shared - this->minfo->kvaddr));

	fimc_is_ischain_region_invalid(this);

#ifdef PRINT_CAPABILITY
	memcpy(&this->capability, &this->is_region->shared,
		sizeof(struct camera2_sm));
	capability = &this->capability;

	printk(KERN_INFO "===ColorC================================\n");
	printk(KERN_INFO "===ToneMapping===========================\n");
	metadata = capability->tonemap.maxCurvePoints;
	printk(KERN_INFO "maxCurvePoints : %d\n", metadata);

	printk(KERN_INFO "===Scaler================================\n");
	printk(KERN_INFO "foramt : %d, %d, %d, %d\n",
		capability->scaler.availableFormats[0],
		capability->scaler.availableFormats[1],
		capability->scaler.availableFormats[2],
		capability->scaler.availableFormats[3]);

	printk(KERN_INFO "===StatisTicsG===========================\n");
	index = 0;
	metadata = capability->stats.availableFaceDetectModes[index];
	while (metadata) {
		printk(KERN_INFO "availableFaceDetectModes : %d\n", metadata);
		index++;
		metadata = capability->stats.availableFaceDetectModes[index];
	}
	printk(KERN_INFO "maxFaceCount : %d\n",
		capability->stats.maxFaceCount);
	printk(KERN_INFO "histogrambucketCount : %d\n",
		capability->stats.histogramBucketCount);
	printk(KERN_INFO "maxHistogramCount : %d\n",
		capability->stats.maxHistogramCount);
	printk(KERN_INFO "sharpnessMapSize : %dx%d\n",
		capability->stats.sharpnessMapSize[0],
		capability->stats.sharpnessMapSize[1]);
	printk(KERN_INFO "maxSharpnessMapValue : %d\n",
		capability->stats.maxSharpnessMapValue);

	printk(KERN_INFO "===3A====================================\n");
	printk(KERN_INFO "maxRegions : %d\n", capability->aa.maxRegions);

	index = 0;
	metadata = capability->aa.aeAvailableModes[index];
	while (metadata) {
		printk(KERN_INFO "aeAvailableModes : %d\n", metadata);
		index++;
		metadata = capability->aa.aeAvailableModes[index];
	}
	printk(KERN_INFO "aeCompensationStep : %d,%d\n",
		capability->aa.aeCompensationStep.num,
		capability->aa.aeCompensationStep.den);
	printk(KERN_INFO "aeCompensationRange : %d ~ %d\n",
		capability->aa.aeCompensationRange[0],
		capability->aa.aeCompensationRange[1]);
	index = 0;
	metadata = capability->aa.aeAvailableTargetFpsRanges[index][0];
	while (metadata) {
		printk(KERN_INFO "TargetFpsRanges : %d ~ %d\n", metadata,
			capability->aa.aeAvailableTargetFpsRanges[index][1]);
		index++;
		metadata = capability->aa.aeAvailableTargetFpsRanges[index][0];
	}
	index = 0;
	metadata = capability->aa.aeAvailableAntibandingModes[index];
	while (metadata) {
		printk(KERN_INFO "aeAvailableAntibandingModes : %d\n",
			metadata);
		index++;
		metadata = capability->aa.aeAvailableAntibandingModes[index];
	}
	index = 0;
	metadata = capability->aa.awbAvailableModes[index];
	while (metadata) {
		printk(KERN_INFO "awbAvailableModes : %d\n", metadata);
		index++;
		metadata = capability->aa.awbAvailableModes[index];
	}
	index = 0;
	metadata = capability->aa.afAvailableModes[index];
	while (metadata) {
		printk(KERN_INFO "afAvailableModes : %d\n", metadata);
		index++;
		metadata = capability->aa.afAvailableModes[index];
	}
#endif
	return ret;
}

int fimc_is_itf_power_down(struct fimc_is_interface *interface)
{
	int ret = 0;
#ifdef ENABLE_CLOCK_GATE
	/* HACK */
	struct fimc_is_core *core = (struct fimc_is_core *)interface->core;
	if (sysfs_debug.en_clk_gate &&
			sysfs_debug.clk_gate_mode == CLOCK_GATE_MODE_HOST) {
		fimc_is_clk_gate_lock_set(core, 0, true);
		fimc_is_wrap_clk_gate_set(core, (1 << GROUP_ID_MAX) - 1, true);
	}
#endif

	ret = fimc_is_itf_power_down_wrap(interface, 0);

#ifdef ENABLE_CLOCK_GATE
	if (sysfs_debug.en_clk_gate &&
		sysfs_debug.clk_gate_mode == CLOCK_GATE_MODE_HOST)
		fimc_is_clk_gate_lock_set(core, 0, false);
#endif

	return ret;
}

int fimc_is_itf_sys_ctl(struct fimc_is_device_ischain *this,
			int cmd, int val)
{
	int ret = 0;

	ret = fimc_is_itf_sys_ctl_wrap(this, cmd, val);

	return ret;
}

static int fimc_is_itf_sensor_mode(struct fimc_is_device_ischain *device)
{
	int ret = 0;
	struct fimc_is_interface *interface;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_sensor_cfg *cfg;
	u32 instance, module;

	FIMC_BUG(!device);
	FIMC_BUG(!device->sensor);
	FIMC_BUG(!device->interface);

	if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
		goto p_err;

	sensor = device->sensor;
	module = device->module;
	instance = device->instance;
	interface = device->interface;

#ifdef ENABLE_IS_CORE
	ret = fimc_is_sensor_s_fcount(sensor);
	if (ret)
		mwarn("fimc_is_sensor_s_fcount is fail", device);
#endif

	cfg = fimc_is_sensor_g_mode(sensor);
	if (!cfg) {
		merr("fimc_is_sensor_g_mode is fail", device);
		ret = -EINVAL;
		goto p_err;
	}

	ret = fimc_is_itf_sensor_mode_wrap(device, cfg);
	if (ret)
		goto p_err;

p_err:
	return ret;
}

int fimc_is_itf_grp_shot(struct fimc_is_device_ischain *device,
	struct fimc_is_group *group,
	struct fimc_is_frame *frame)
{
	int ret = 0;
#ifdef CONFIG_USE_SENSOR_GROUP
	unsigned long flags;
	struct fimc_is_group *head;
	struct fimc_is_framemgr *framemgr;
#endif
	FIMC_BUG(!device);
	FIMC_BUG(!group);
	FIMC_BUG(!frame);
	FIMC_BUG(!frame->shot);

	/* Cache Flush */
	fimc_is_ischain_meta_flush(frame);

	if (frame->shot->magicNumber != SHOT_MAGIC_NUMBER) {
		mgrerr("shot magic number error(0x%08X) size(%zd)\n", device, group, frame,
			frame->shot->magicNumber, sizeof(struct camera2_shot_ext));
		ret = -EINVAL;
		goto p_err;
	}

#ifdef ENABLE_CLOCK_GATE
	{
		struct fimc_is_core *core = (struct fimc_is_core *)device->resourcemgr->private_data;

		/* HACK */
		/* dynamic clock on */
		if (sysfs_debug.en_clk_gate &&
				sysfs_debug.clk_gate_mode == CLOCK_GATE_MODE_HOST)
			fimc_is_clk_gate_set(core, group->id, true, false, true);
	}
#endif

	mgrdbgs(1, " SHOT(%d)\n", device, group, frame, frame->index);

#ifdef CONFIG_USE_SENSOR_GROUP
	head = GET_HEAD_GROUP_IN_DEVICE(FIMC_IS_DEVICE_ISCHAIN, group);
	if (head) {
		ret = fimc_is_itf_shot_wrap(device, group, frame);
	} else {
		framemgr = GET_HEAD_GROUP_FRAMEMGR(group);
		if (!framemgr) {
			merr("framemgr is NULL", group);
			ret = -EINVAL;
			goto p_err;
		}

		set_bit(group->leader.id, &frame->out_flag);
		framemgr_e_barrier_irqs(framemgr, FMGR_IDX_25, flags);
		trans_frame(framemgr, frame, FS_PROCESS);
		framemgr_x_barrier_irqr(framemgr, FMGR_IDX_25, flags);
	}
#else
	ret = fimc_is_itf_shot_wrap(device, group, frame);
#endif

p_err:
	return ret;
}

int fimc_is_ischain_runtime_suspend(struct device *dev)
{
	int ret = 0;
	struct platform_device *pdev = to_platform_device(dev);
	struct fimc_is_core *core = (struct fimc_is_core *)dev_get_drvdata(dev);
	struct fimc_is_mem *mem = &core->resourcemgr.mem;
	struct exynos_platform_fimc_is *pdata;

#if defined(CONFIG_PM_DEVFREQ)
#if defined(QOS_INTCAM)
	int int_cam_qos;
#endif
	int int_qos, mif_qos, cam_qos, hpg_qos;
	int refcount;
#endif
	pdata = dev_get_platdata(dev);

	FIMC_BUG(!pdata);
	FIMC_BUG(!pdata->clk_off);

	info("FIMC_IS runtime suspend in\n");

	CALL_MEMOP(mem, suspend, mem->default_ctx);

#if defined(CONFIG_FIMC_IS_BUS_DEVFREQ) && defined(CONFIG_EXYNOS_BTS)
#if defined(CONFIG_EXYNOS7870_BTS) || defined(CONFIG_EXYNOS7880_BTS)
	exynos_update_media_scenario(TYPE_CAM, false, 0);
#else
	exynos7_update_media_scenario(TYPE_CAM, false, 0);
#endif
#endif
#if defined(CONFIG_PCI_EXYNOS)
	exynos_pcie_l1ss_ctrl(1, PCIE_L1SS_CTRL_CAMERA);
#endif

	ret = pdata->clk_off(&pdev->dev);
	if (ret)
		err("clk_off is fail(%d)", ret);

#if defined(CONFIG_PM_DEVFREQ)
	refcount = atomic_dec_return(&core->resourcemgr.qos_refcount);
	if (refcount == 0) {
		/* DEVFREQ release */
		dbgd_resource("[RSC] %s: QoS UNLOCK\n", __func__);
#if defined(QOS_INTCAM)
		int_cam_qos = fimc_is_get_qos(core, FIMC_IS_DVFS_INT_CAM, FIMC_IS_SN_MAX);
#endif
		int_qos = fimc_is_get_qos(core, FIMC_IS_DVFS_INT, FIMC_IS_SN_MAX);
		mif_qos = fimc_is_get_qos(core, FIMC_IS_DVFS_MIF, FIMC_IS_SN_MAX);
		cam_qos = fimc_is_get_qos(core, FIMC_IS_DVFS_CAM, START_DVFS_LEVEL);
		hpg_qos = fimc_is_get_qos(core, FIMC_IS_DVFS_HPG, START_DVFS_LEVEL);

#if defined(QOS_INTCAM)
		if (int_cam_qos > 0)
			pm_qos_remove_request(&exynos_isp_qos_int_cam);
#endif
		if (int_qos > 0)
			pm_qos_remove_request(&exynos_isp_qos_int);
		if (mif_qos > 0)
			pm_qos_remove_request(&exynos_isp_qos_mem);
		if (cam_qos > 0)
			pm_qos_remove_request(&exynos_isp_qos_cam);
		if (hpg_qos > 0)
			pm_qos_remove_request(&exynos_isp_qos_hpg);
#if defined(CONFIG_HMP_VARIABLE_SCALE)
		if (core->resourcemgr.dvfs_ctrl.cur_hmp_bst)
			set_hmp_boost(0);
#elif defined(CONFIG_SCHED_EHMP) || defined(CONFIG_SCHED_EMS)
		if (core->resourcemgr.dvfs_ctrl.cur_hmp_bst)
			gb_qos_update_request(&gb_req, 0);
#endif
	}
#endif
#if defined (CONFIG_HOTPLUG_CPU) && defined (FIMC_IS_ONLINE_CPU_MIN)
	pm_qos_remove_request(&exynos_isp_qos_cpu_online_min);
#endif
#if !defined(ENABLE_IS_CORE)
	fimc_is_hardware_runtime_suspend(&core->hardware);
#endif
#ifdef CAMERA_HW_BIG_DATA_FILE_IO
	if (fimc_is_sec_need_update_to_file())
		fimc_is_sec_copy_err_cnt_to_file();
#endif
	info("FIMC_IS runtime suspend out\n");
	return 0;
}

int fimc_is_ischain_runtime_resume(struct device *dev)
{
	int ret = 0;
	struct platform_device *pdev = to_platform_device(dev);
	struct fimc_is_core *core = (struct fimc_is_core *)dev_get_drvdata(dev);
	struct fimc_is_mem *mem = &core->resourcemgr.mem;
	struct exynos_platform_fimc_is *pdata;

#if defined(CONFIG_PM_DEVFREQ)
#if defined(QOS_INTCAM)
	int int_cam_qos;
#endif
	int int_qos, mif_qos, cam_qos, hpg_qos;
	int refcount;
#endif
	pdata = dev_get_platdata(dev);

	FIMC_BUG(!pdata);
	FIMC_BUG(!pdata->clk_cfg);
	FIMC_BUG(!pdata->clk_on);

	info("FIMC_IS runtime resume in\n");

	ret = fimc_is_ischain_runtime_resume_pre(dev);
	if (ret) {
		err("fimc_is_runtime_resume_pre is fail(%d)", ret);
		goto p_err;
	}

	ret = pdata->clk_cfg(&pdev->dev);
	if (ret) {
		err("clk_cfg is fail(%d)", ret);
		goto p_err;
	}

	/* HACK: DVFS lock sequence is change.
	 * DVFS level should be locked after power on.
	 */
#if defined(CONFIG_PM_DEVFREQ)
	refcount = atomic_inc_return(&core->resourcemgr.qos_refcount);
	if (refcount == 1) {
#if defined(QOS_INTCAM)
		int_cam_qos = fimc_is_get_qos(core, FIMC_IS_DVFS_INT_CAM, START_DVFS_LEVEL);
#endif
		int_qos = fimc_is_get_qos(core, FIMC_IS_DVFS_INT, START_DVFS_LEVEL);
		mif_qos = fimc_is_get_qos(core, FIMC_IS_DVFS_MIF, START_DVFS_LEVEL);
		cam_qos = fimc_is_get_qos(core, FIMC_IS_DVFS_CAM, START_DVFS_LEVEL);
		hpg_qos = fimc_is_get_qos(core, FIMC_IS_DVFS_HPG, START_DVFS_LEVEL);

		/* DEVFREQ lock */
#if defined(QOS_INTCAM)
		if (int_cam_qos > 0 && !pm_qos_request_active(&exynos_isp_qos_int_cam))
			pm_qos_add_request(&exynos_isp_qos_int_cam, PM_QOS_INTCAM_THROUGHPUT, int_cam_qos);
#endif
		if (int_qos > 0 && !pm_qos_request_active(&exynos_isp_qos_int))
			pm_qos_add_request(&exynos_isp_qos_int, PM_QOS_DEVICE_THROUGHPUT, int_qos);
		if (mif_qos > 0 && !pm_qos_request_active(&exynos_isp_qos_mem))
			pm_qos_add_request(&exynos_isp_qos_mem, PM_QOS_BUS_THROUGHPUT, mif_qos);
		if (cam_qos > 0 && !pm_qos_request_active(&exynos_isp_qos_cam))
			pm_qos_add_request(&exynos_isp_qos_cam, PM_QOS_CAM_THROUGHPUT, cam_qos);
		if (hpg_qos > 0 && !pm_qos_request_active(&exynos_isp_qos_hpg))
			pm_qos_add_request(&exynos_isp_qos_hpg, PM_QOS_CPU_ONLINE_MIN, hpg_qos);
#if defined(QOS_INTCAM)
		info("[RSC] %s: QoS LOCK [INT_CAM(%d), INT(%d), MIF(%d), CAM(%d), HPG(%d)]\n",
			__func__, int_cam_qos, int_qos, mif_qos, cam_qos, hpg_qos);
#else
		info("[RSC] %s: QoS LOCK [INT(%d), MIF(%d), CAM(%d), HPG(%d)]\n",
			__func__, int_qos, mif_qos, cam_qos, hpg_qos);
#endif
	}
#endif
#if defined (CONFIG_HOTPLUG_CPU) && defined (FIMC_IS_ONLINE_CPU_MIN)
	pm_qos_add_request(&exynos_isp_qos_cpu_online_min, PM_QOS_CPU_ONLINE_MIN,
		FIMC_IS_ONLINE_CPU_MIN);
#endif
	/* Clock on */
	ret = pdata->clk_on(&pdev->dev);
	if (ret) {
		err("clk_on is fail(%d)", ret);
		goto p_err;
	}

	CALL_MEMOP(mem, resume, mem->default_ctx);

#if defined(CONFIG_FIMC_IS_BUS_DEVFREQ) && defined(CONFIG_EXYNOS_BTS)
#if defined(CONFIG_EXYNOS7870_BTS) || defined(CONFIG_EXYNOS7880_BTS)
	exynos_update_media_scenario(TYPE_CAM, true, 0);
#else
	exynos7_update_media_scenario(TYPE_CAM, true, 0);
#endif
#endif
#if defined(CONFIG_PCI_EXYNOS)
	exynos_pcie_l1ss_ctrl(0, PCIE_L1SS_CTRL_CAMERA);
#endif

#if !defined(ENABLE_IS_CORE)
	fimc_is_hardware_runtime_resume(&core->hardware);
#endif

p_err:
	info("FIMC-IS runtime resume out\n");
	return ret;
}

#ifdef ENABLE_IS_CORE
int fimc_is_ischain_power(struct fimc_is_device_ischain *device, int on)
{
	int ret = 0;
	int retry = 4;
#if defined(CONFIG_PM)
	int rpm_ret;
#endif
	u32 val;
	struct device *dev;
	struct fimc_is_core *core;
	struct fimc_is_vender *vender;
	struct fimc_is_interface *itf;
#ifdef CONFIG_VENDER_MCD
	struct fimc_is_vender_specific *specific;
#endif

	FIMC_BUG(!device);
	FIMC_BUG(!device->interface);

	dev = &device->pdev->dev;
	core = (struct fimc_is_core *)dev_get_drvdata(dev);
	vender = &core->vender;
	itf = device->interface;
#ifdef CONFIG_VENDER_MCD
	specific = core->vender.private_data;
#endif

	if (on) {
		/* runtime suspend callback can be called lately because of power relationship */
		while (test_bit(FIMC_IS_ISCHAIN_POWER_ON, &device->state) && (retry > 0)) {
			warn("sensor is not yet power off");
			msleep(500);
			--retry;
		}
		if (!retry) {
			ret = -EBUSY;
			goto p_err;
		}

		/* 2. FIMC-IS local power enable */
#if defined(CONFIG_PM)
		mdbgd_ischain("pm_runtime_suspended = %d\n", device, pm_runtime_suspended(dev));
		rpm_ret = pm_runtime_get_sync(dev);
		if (rpm_ret < 0)
			err("pm_runtime_get_sync() return error: %d", rpm_ret);
#else
		fimc_is_ischain_runtime_resume(dev);
		info("%s(%d) - fimc_is runtime resume complete\n", __func__, on);
#endif

		ret = fimc_is_vender_fw_sel(vender);
		if (ret) {
			err("fimc_is_vender_fw_sel is fail(%d)", ret);
			ret = -EINVAL;
			goto p_err;
		}

		if (core->current_position == SENSOR_POSITION_FRONT
#ifdef CONFIG_VENDER_MCD
		|| specific->suspend_resume_disable
#endif
		) {
			fimc_is_itf_fwboot_init(itf);
		}

		fimc_is_itf_set_fwboot(device, itf->fw_boot_mode);

		if (test_bit(IS_IF_RESUME, &itf->fw_boot)) {
#ifdef FW_SUSPEND_RESUME
			fimc_is_itf_restorefirm(device);
#endif
		} else {
			ret = fimc_is_ischain_loadfirm(device, vender);
			if (ret) {
				err("fimc_is_ischain_loadfirm is fail(%d)", ret);
				ret = -EINVAL;
				goto p_err;
			}
		}

		set_bit(FIMC_IS_ISCHAIN_LOADED, &device->state);

#ifdef ENABLE_FD_SW
		/* Load FD library */
		ret = fimc_is_ischain_loadfd(device);
		if (ret) {
			err("failed to fimc_is_lib_fd_load (%d)", ret);
			ret = -EINVAL;
			goto p_err;
		}
#endif

		/* 4. A5 start address setting */
		mdbgd_ischain("minfo.dvaddr: %llx\n", device, device->minfo->dvaddr);
		mdbgd_ischain("minfo.kvaddr: 0x%lX\n", device, device->minfo->kvaddr);

		if (!device->minfo->dvaddr) {
			err("firmware device virtual is NULL");
			ret = -ENOMEM;
			goto p_err;
		}

		writel(device->minfo->dvaddr, itf->regs + BBOAR);
		val = __raw_readl(itf->regs + BBOAR);
		if (device->minfo->dvaddr != val) {
			err("DVA(%llx), BBOAR(%x) is not conincedence", device->minfo->dvaddr, val);
			ret = -EINVAL;
			goto p_err;
		}

		ret = fimc_is_ischain_config_secure(device);
		if (ret) {
			err("fimc_is_ischain_config_secure is fail(%d)", ret);
			goto p_err;
		}

		ret = fimc_is_ischain_runtime_resume_post(dev);
		if (ret) {
			err("fimc_is_ischain_runtime_resume_post is fail(%d)", ret);
			goto p_err;
		}

#ifdef CONFIG_VENDER_MCD
		if (specific->need_cold_reset)
			specific->need_cold_reset = false;
#endif

		set_bit(FIMC_IS_ISCHAIN_POWER_ON, &device->state);
	} else {
#ifdef FW_SUSPEND_RESUME
		if (test_bit(IS_IF_SUSPEND, &itf->fw_boot)) {
			if (!test_bit(IS_IF_LAUNCH_SUCCESS, &itf->launch_state)) {
				fimc_is_itf_fwboot_init(itf);
			} else {
				fimc_is_itf_storefirm(device);
				clear_bit(IS_IF_LAUNCH_SUCCESS, &itf->launch_state);
			}
		}
#endif

		/* Check FW state for WFI of A5 */
		info("A5 state(0x%x)\n", readl(itf->regs + ISSR6));

		/* FIMC-IS local power down */
#if defined(CONFIG_PM)
		ret = pm_runtime_put_sync(dev);
		if (ret)
			err("pm_runtime_put_sync is fail(%d)", ret);
#else
		ret = fimc_is_ischain_runtime_suspend(dev);
		if (ret)
			err("fimc_is_runtime_suspend is fail(%d)", ret);
#endif

		ret = fimc_is_ischain_runtime_suspend_post(dev);
		if (ret)
			err("fimc_is_runtime_suspend_post is fail(%d)", ret);

		clear_bit(FIMC_IS_ISCHAIN_POWER_ON, &device->state);
	}

p_err:
	info("%s(%d):%d\n", __func__, on, ret);
	return ret;
}
#else
int fimc_is_ischain_power(struct fimc_is_device_ischain *device, int on)
{
	int ret = 0;
	int retry = 4;
#if defined(CONFIG_PM)
	int rpm_ret;
#endif
	struct device *dev;
	struct fimc_is_core *core;
	struct fimc_is_vender *vender;

	FIMC_BUG(!device);
	FIMC_BUG(!device->interface);

	dev = &device->pdev->dev;
	core = (struct fimc_is_core *)dev_get_drvdata(dev);
	vender = &core->vender;

	if (on) {
		/* runtime suspend callback can be called lately because of power relationship */
		while (test_bit(FIMC_IS_ISCHAIN_POWER_ON, &device->state) && (retry > 0)) {
			warn("sensor is not yet power off");
			msleep(500);
			--retry;
		}
		if (!retry) {
			ret = -EBUSY;
			goto p_err;
		}

		/* 2. FIMC-IS local power enable */
#if defined(CONFIG_PM)
		mdbgd_ischain("pm_runtime_suspended = %d\n", device, pm_runtime_suspended(dev));
		rpm_ret = pm_runtime_get_sync(dev);
		if (rpm_ret < 0)
			err("pm_runtime_get_sync() return error: %d", rpm_ret);
#else
		fimc_is_ischain_runtime_resume(dev);
		info("%s(%d) - fimc_is runtime resume complete\n", __func__, on);
#endif

		set_bit(FIMC_IS_ISCHAIN_LOADED, &device->state);

#ifdef ENABLE_FD_SW
		/* Load FD library */
		ret = fimc_is_ischain_loadfd(device);
		if (ret) {
			err("failed to fimc_is_lib_fd_load (%d)", ret);
			ret = -EINVAL;
			goto p_err;
		}
#endif
#ifdef ENABLE_VIRTUAL_OTF
		fimc_is_hw_cip_clk_enable(true);
		fimc_is_hw_c2sync_ring_clock(OOF, true);
		fimc_is_hw_c2sync_ring_clock(HBZ, true);
#endif

		set_bit(FIMC_IS_ISCHAIN_POWER_ON, &device->state);
	} else {
		/* FIMC-IS local power down */
#ifdef ENABLE_VIRTUAL_OTF
		fimc_is_hw_c2sync_ring_clock(OOF, false);
		fimc_is_hw_c2sync_ring_clock(HBZ, false);
		fimc_is_hw_cip_clk_enable(false);
#endif
#if defined(CONFIG_PM)
		ret = pm_runtime_put_sync(dev);
		if (ret)
			err("pm_runtime_put_sync is fail(%d)", ret);
#else
		ret = fimc_is_ischain_runtime_suspend(dev);
		if (ret)
			err("fimc_is_runtime_suspend is fail(%d)", ret);
#endif

		ret = fimc_is_ischain_runtime_suspend_post(dev);
		if (ret)
			err("fimc_is_runtime_suspend_post is fail(%d)", ret);

		clear_bit(FIMC_IS_ISCHAIN_POWER_ON, &device->state);
	}

p_err:

	info("%s(%d):%d\n", __func__, on, ret);
	return ret;
}
#endif

static int fimc_is_ischain_s_sensor_size(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame,
	u32 *lindex,
	u32 *hindex,
	u32 *indexes)
{
	int ret = 0;
	struct param_sensor_config *sensor_config;
	u32 binning, bns_binning;
	u32 sensor_width, sensor_height;
	u32 bns_width, bns_height;
	u32 framerate;
	enum fimc_is_ex_mode ex_mode;

	FIMC_BUG(!device->sensor);

	binning = fimc_is_sensor_g_bratio(device->sensor);
	sensor_width = fimc_is_sensor_g_width(device->sensor);
	sensor_height = fimc_is_sensor_g_height(device->sensor);
	bns_width = fimc_is_sensor_g_bns_width(device->sensor);
	bns_height = fimc_is_sensor_g_bns_height(device->sensor);
	framerate = fimc_is_sensor_g_framerate(device->sensor);
	ex_mode = fimc_is_sensor_g_ex_mode(device->sensor);

	if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
		bns_binning = 1000;
	else
		bns_binning = fimc_is_sensor_g_bns_ratio(device->sensor);

	sensor_config = fimc_is_itf_g_param(device, frame, PARAM_SENSOR_CONFIG);
	sensor_config->width = sensor_width;
	sensor_config->height = sensor_height;
	sensor_config->calibrated_width = sensor_width;
	sensor_config->calibrated_height = sensor_height;
	sensor_config->sensor_binning_ratio_x = binning;
	sensor_config->sensor_binning_ratio_y = binning;
	sensor_config->bns_binning_ratio_x = bns_binning;
	sensor_config->bns_binning_ratio_y = bns_binning;
	sensor_config->bns_margin_left = 0;
	sensor_config->bns_margin_top = 0;
	sensor_config->bns_output_width = bns_width;
	sensor_config->bns_output_height = bns_height;
	sensor_config->frametime = 10 * 1000 * 1000; /* max exposure time */
#ifdef FIXED_FPS_DEBUG
	sensor_config->min_target_fps = FIXED_FPS_VALUE;
	sensor_config->max_target_fps = FIXED_FPS_VALUE;
#else
	if (device->sensor->min_target_fps > 0)
		sensor_config->min_target_fps = device->sensor->min_target_fps;
	if (device->sensor->max_target_fps > 0)
		sensor_config->max_target_fps = device->sensor->max_target_fps;
#endif

	if (ex_mode == EX_DUALFPS_960 || ex_mode == EX_DUALFPS_480)
		sensor_config->early_config_lock = 1;
	else
		sensor_config->early_config_lock = 0;

	*lindex |= LOWBIT_OF(PARAM_SENSOR_CONFIG);
	*hindex |= HIGHBIT_OF(PARAM_SENSOR_CONFIG);
	(*indexes)++;

	return ret;
}

static int fimc_is_ischain_s_path(struct fimc_is_device_ischain *device,
	u32 *lindex, u32 *hindex, u32 *indexes)
{
	int ret = 0;
#ifdef SOC_SCC
	struct scc_param *scc_param;
#endif
#ifdef SOC_SCP
	struct scp_param *scp_param;
#endif

	FIMC_BUG(!device);
	FIMC_BUG(!lindex);
	FIMC_BUG(!hindex);
	FIMC_BUG(!indexes);

#ifdef SOC_SCC
	scc_param = &device->is_region->parameter.scalerc;
#endif
#ifdef SOC_SCP
	scp_param = &device->is_region->parameter.scalerp;
#endif

	if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state)) {
#ifdef SOC_SCC
		scc_param->otf_output.cmd = OTF_OUTPUT_COMMAND_DISABLE;
		*lindex |= LOWBIT_OF(PARAM_SCALERC_OTF_OUTPUT);
		*hindex |= HIGHBIT_OF(PARAM_SCALERC_OTF_OUTPUT);
		(*indexes)++;
#endif

		fimc_is_subdev_odc_stop(device, NULL, lindex, hindex, indexes);
		fimc_is_subdev_drc_stop(device, NULL, lindex, hindex, indexes);
		fimc_is_subdev_dnr_stop(device, NULL, lindex, hindex, indexes);

#ifdef SOC_SCP
		scp_param->control.cmd = CONTROL_COMMAND_STOP;
		*lindex |= LOWBIT_OF(PARAM_SCALERP_CONTROL);
		*hindex |= HIGHBIT_OF(PARAM_SCALERP_CONTROL);
		(*indexes)++;
#endif
	} else {
#ifdef SOC_SCC
		scc_param->otf_output.cmd = OTF_OUTPUT_COMMAND_ENABLE;
		*lindex |= LOWBIT_OF(PARAM_SCALERC_OTF_OUTPUT);
		*hindex |= HIGHBIT_OF(PARAM_SCALERC_OTF_OUTPUT);
		(*indexes)++;
#endif

		fimc_is_subdev_odc_bypass(device, NULL, lindex, hindex, indexes, true);
		fimc_is_subdev_drc_bypass(device, NULL, lindex, hindex, indexes, true);

#ifdef SOC_SCP
		scp_param->control.cmd = CONTROL_COMMAND_START;
		*lindex |= LOWBIT_OF(PARAM_SCALERP_CONTROL);
		*hindex |= HIGHBIT_OF(PARAM_SCALERP_CONTROL);
		(*indexes)++;
#endif
	}

	return ret;
}

int fimc_is_ischain_buf_tag(struct fimc_is_device_ischain *device,
	struct fimc_is_subdev *subdev,
	struct fimc_is_frame *ldr_frame,
	u32 pixelformat,
	u32 width,
	u32 height,
	u32 target_addr[])
{
	int ret = 0;
	int i, j;
	unsigned long flags;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	struct fimc_is_queue *queue;
	struct camera2_node *capture;

	framemgr = GET_SUBDEV_FRAMEMGR(subdev);
	FIMC_BUG(!framemgr);

	queue = GET_SUBDEV_QUEUE(subdev);
	if (!queue) {
		merr("queue is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_24, flags);

	frame = peek_frame(framemgr, FS_REQUEST);
	if (frame) {
		if (!frame->stream) {
			framemgr_x_barrier_irqr(framemgr, FMGR_IDX_24, flags);
			merr("frame->stream is NULL", device);
			BUG();
		}

		switch (pixelformat) {
		case V4L2_PIX_FMT_NV21:
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV16:
		case V4L2_PIX_FMT_NV61:
			for (i = 0; i < frame->planes; i++) {
				j = i * 2;
				target_addr[j] = (typeof(*target_addr))frame->dvaddr_buffer[i];
				target_addr[j + 1] = target_addr[j] + (width * height);
			}
			break;
		case V4L2_PIX_FMT_YVU420M:
			for (i = 0; i < frame->planes; i += 3) {
				target_addr[i] = (typeof(*target_addr))frame->dvaddr_buffer[i];
				target_addr[i + 1] = (typeof(*target_addr))frame->dvaddr_buffer[i + 2];
				target_addr[i + 2] = (typeof(*target_addr))frame->dvaddr_buffer[i + 1];
			}
			break;
		case V4L2_PIX_FMT_YUV420:
			for (i = 0; i < frame->planes; i++) {
				j = i * 3;
				target_addr[j] = (typeof(*target_addr))frame->dvaddr_buffer[i];
				target_addr[j + 1] = target_addr[j] + (width * height);
				target_addr[j + 2] = target_addr[j + 1] + (width * height / 4);
			}
			break;
		case V4L2_PIX_FMT_YVU420: /* AYV12 spec: The width should be aligned by 16 pixel. */
			for (i = 0; i < frame->planes; i++) {
				j = i * 3;
				target_addr[j] = (typeof(*target_addr))frame->dvaddr_buffer[i];
				target_addr[j + 2] = target_addr[j] + (ALIGN(width, 16) * height);
				target_addr[j + 1] = target_addr[j + 2] + (ALIGN(width / 2, 16) * height / 2);
			}
			break;
		case V4L2_PIX_FMT_YUV422P:
			for (i = 0; i < frame->planes; i++) {
				j = i * 3;
				target_addr[j] = (typeof(*target_addr))frame->dvaddr_buffer[i];
				target_addr[j + 1] = target_addr[j] + (width * height);
				target_addr[j + 2] = target_addr[j + 1] + (width * height / 2);
			}
			break;
		case V4L2_PIX_FMT_NV12M_S10B:
		case V4L2_PIX_FMT_NV21M_S10B:
			for (i = 0; i < frame->planes; i += 2) {
				j = i * 2;
				/* Y_ADDR, UV_ADDR, Y_2BIT_ADDR, UV_2BIT_ADDR */
				target_addr[j] = (typeof(*target_addr))frame->dvaddr_buffer[i];
				target_addr[j + 1] = (typeof(*target_addr))frame->dvaddr_buffer[i + 1];
				target_addr[j + 2] = target_addr[j] + NV12M_Y_SIZE(width, height);
				target_addr[j + 3] = target_addr[j + 1] + NV12M_CBCR_SIZE(width, height);
			}
			break;
		case V4L2_PIX_FMT_NV16M_S10B:
		case V4L2_PIX_FMT_NV61M_S10B:
			for (i = 0; i < frame->planes; i += 2) {
				j = i * 2;
				/* Y_ADDR, UV_ADDR, Y_2BIT_ADDR, UV_2BIT_ADDR */
				target_addr[j] = (typeof(*target_addr))frame->dvaddr_buffer[i];
				target_addr[j + 1] = (typeof(*target_addr))frame->dvaddr_buffer[i + 1];
				target_addr[j + 2] = target_addr[j] + NV16M_Y_SIZE(width, height);
				target_addr[j + 3] = target_addr[j + 1] + NV16M_CBCR_SIZE(width, height);
			}
			break;
		default:
			for (i = 0; i < frame->planes; i++)
				target_addr[i] = (typeof(*target_addr))frame->dvaddr_buffer[i];
			break;
		}

		capture = &ldr_frame->shot_ext->node_group.capture[subdev->cid];

		frame->fcount = ldr_frame->fcount;
		frame->stream->findex = ldr_frame->index;
		frame->stream->fcount = ldr_frame->fcount;

		if (likely(capture->vid == subdev->vid)) {
			frame->stream->input_crop_region[0] = capture->input.cropRegion[0];
			frame->stream->input_crop_region[1] = capture->input.cropRegion[1];
			frame->stream->input_crop_region[2] = capture->input.cropRegion[2];
			frame->stream->input_crop_region[3] = capture->input.cropRegion[3];
			frame->stream->output_crop_region[0] = capture->output.cropRegion[0];
			frame->stream->output_crop_region[1] = capture->output.cropRegion[1];
			frame->stream->output_crop_region[2] = capture->output.cropRegion[2];
			frame->stream->output_crop_region[3] = capture->output.cropRegion[3];
		} else {
			mserr("capture vid is changed(%d != %d)", subdev, subdev, subdev->vid, capture->vid);
			frame->stream->input_crop_region[0] = 0;
			frame->stream->input_crop_region[1] = 0;
			frame->stream->input_crop_region[2] = 0;
			frame->stream->input_crop_region[3] = 0;
			frame->stream->output_crop_region[0] = 0;
			frame->stream->output_crop_region[1] = 0;
			frame->stream->output_crop_region[2] = 0;
			frame->stream->output_crop_region[3] = 0;
		}

		set_bit(subdev->id, &ldr_frame->out_flag);
		trans_frame(framemgr, frame, FS_PROCESS);
	} else {
		for (i = 0; i < FIMC_IS_MAX_PLANES; i++)
			target_addr[i] = 0;

		ret = -EINVAL;
	}

#ifdef DBG_DRAW_DIGIT
	frame->width = width;
	frame->height = height;
#endif

	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_24, flags);

p_err:
	return ret;
}

int fimc_is_ischain_buf_tag_64bit(struct fimc_is_device_ischain *device,
	struct fimc_is_subdev *subdev,
	struct fimc_is_frame *ldr_frame,
	u32 pixelformat,
	u32 width,
	u32 height,
	uint64_t target_addr[])
{
	int ret = 0;
	int i;
	unsigned long flags;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	struct fimc_is_queue *queue;

	framemgr = GET_SUBDEV_FRAMEMGR(subdev);
	BUG_ON(!framemgr);

	queue = GET_SUBDEV_QUEUE(subdev);
	if (!queue) {
		merr("queue is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_24, flags);

	frame = peek_frame(framemgr, FS_REQUEST);
	if (frame) {
		if (!frame->stream) {
			framemgr_x_barrier_irqr(framemgr, FMGR_IDX_24, flags);
			merr("frame->stream is NULL", device);
			BUG();
		}

		switch (pixelformat) {
		case V4L2_PIX_FMT_Y12:	/* Only for ME : kernel virtual addr*/
			for (i = 0; i < frame->planes; i++)
				target_addr[i] = frame->kvaddr_buffer[i];
			break;
		default:
			for (i = 0; i < frame->planes; i++)
				target_addr[i] = (typeof(*target_addr))frame->dvaddr_buffer[i];
			break;
		}

		frame->fcount = ldr_frame->fcount;
		frame->stream->findex = ldr_frame->index;
		frame->stream->fcount = ldr_frame->fcount;
		set_bit(subdev->id, &ldr_frame->out_flag);
		trans_frame(framemgr, frame, FS_PROCESS);
	} else {
		for (i = 0; i < FIMC_IS_MAX_PLANES; i++)
			target_addr[i] = 0;

		ret = -EINVAL;
	}

#ifdef DBG_DRAW_DIGIT
	frame->width = width;
	frame->height = height;
#endif

	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_24, flags);

p_err:
	return ret;
}

static int fimc_is_ischain_chg_setfile(struct fimc_is_device_ischain *device)
{
	int ret = 0;
	u32 group = 0;
	struct fimc_is_path_info *path;
	u32 indexes, lindex, hindex;

	FIMC_BUG(!device);

	indexes = lindex = hindex = 0;
	path = &device->path;
	group = fimc_is_itf_g_group_info(device, path);

	ret = fimc_is_itf_process_stop(device, group);
	if (ret) {
		merr("fimc_is_itf_process_stop fail", device);
		goto p_err;
	}

	ret = fimc_is_itf_a_param(device, group);
	if (ret) {
		merr("fimc_is_itf_a_param is fail", device);
		ret = -EINVAL;
		goto p_err;
	}

	ret = fimc_is_itf_process_start(device, group);
	if (ret) {
		merr("fimc_is_itf_process_start fail", device);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	minfo("[ISC:D] %s(%d):%d\n", device, __func__,
		device->setfile & FIMC_IS_SETFILE_MASK, ret);
	return ret;
}

#ifdef ENABLE_DRC
static int fimc_is_ischain_drc_bypass(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame,
	bool bypass)
{
	int ret = 0;
	u32 lindex, hindex, indexes;
	struct fimc_is_group *group;
	struct fimc_is_subdev *leader, *subdev;

	FIMC_BUG(!device);
	FIMC_BUG(!device->drc.leader);

	subdev = &device->drc;
	leader = subdev->leader;
	group = container_of(leader, struct fimc_is_group, leader);
	lindex = hindex = indexes = 0;

	if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
		mserr("subdev is not start", device, subdev);
		ret = -EINVAL;
		goto p_err;
	}

	if (bypass)
		fimc_is_subdev_drc_stop(device, frame, &lindex, &hindex, &indexes);
	else
		fimc_is_subdev_drc_start(device, frame, &lindex, &hindex, &indexes);

	ret = fimc_is_itf_s_param(device, frame, lindex, hindex, indexes);
	if (ret) {
		mrerr("fimc_is_itf_s_param is fail(%d)", device, frame, ret);
		goto p_err;
	}

	if (bypass)
		clear_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);
	else
		set_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);

p_err:
	msrinfo("bypass : %d\n", device, subdev, frame, bypass);
	return ret;
}
#endif

#ifdef ENABLE_DIS
static int fimc_is_ischain_dis_bypass(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame,
	bool bypass)
{
	int ret = 0;
	u32 lindex, hindex, indexes;
	struct fimc_is_group *group;
	struct fimc_is_subdev *leader, *subdev;

	FIMC_BUG(!device);

	subdev = &device->group_dis.leader;
	leader = subdev->leader;
	group = container_of(leader, struct fimc_is_group, leader);
	lindex = hindex = indexes = 0;

	if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
		mserr("subdev is not start", device, subdev);
		ret = -EINVAL;
		goto p_err;
	}

	fimc_is_subdev_dis_bypass(device, frame, &lindex, &hindex, &indexes, bypass);

	 ret = fimc_is_itf_s_param(device, frame, lindex, hindex, indexes);
	if (ret) {
		mrerr("fimc_is_itf_s_param is fail(%d)", device, frame, ret);
		goto p_err;
	}

	if (bypass)
		clear_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);
	else
		set_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);

p_err:
	msrinfo("bypass : %d\n", device, subdev, frame, bypass);
	return ret;
}
#endif


#if defined(ENABLE_DNR_IN_TPU) || defined(ENABLE_DNR_IN_MCSC)
static int fimc_is_ischain_dnr_bypass(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame,
	bool bypass)
{
	int ret = 0;
	u32 lindex, hindex, indexes;
	struct fimc_is_group *group;
	struct fimc_is_subdev *leader, *subdev;

	FIMC_BUG(!device);
	FIMC_BUG(!device->dnr.leader);

	subdev = &device->dnr;
	leader = subdev->leader;
	group = container_of(leader, struct fimc_is_group, leader);
	lindex = hindex = indexes = 0;

	if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
		mserr("subdev is not start", device, subdev);
		ret = -EINVAL;
		goto p_err;
	}

	fimc_is_subdev_dnr_bypass(device, frame, &lindex, &hindex, &indexes, bypass);

	ret = fimc_is_itf_s_param(device, frame, lindex, hindex, indexes);
	if (ret) {
		mrerr("fimc_is_itf_s_param is fail(%d)", device, frame, ret);
		goto p_err;
	}

	if (bypass)
		clear_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);
	else
		set_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);

p_err:
	msrinfo("bypass : %d\n", device, subdev, frame, bypass);
	return ret;
}
#endif

#ifdef ENABLE_VRA
static int fimc_is_ischain_vra_bypass(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame,
	bool bypass)
{
	int ret = 0;
	u32 lindex, hindex, indexes;
	struct param_control *control;
	struct fimc_is_group *group;
	struct fimc_is_subdev *leader, *subdev;
#ifdef ENABLE_FD_SW
	struct param_fd_config *fd_config;
#endif

	FIMC_BUG(!device);

	subdev = &device->group_vra.leader;
	leader = subdev->leader;
	group = container_of(leader, struct fimc_is_group, leader);
	lindex = hindex = indexes = 0;

	if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
		mserr("subdev is not start", device, subdev);
		ret = -EINVAL;
		goto p_err;
	}

	control = fimc_is_itf_g_param(device, frame, PARAM_FD_CONTROL);
	if (bypass)
		control->cmd = CONTROL_COMMAND_STOP;
	else
		control->cmd = CONTROL_COMMAND_START;
	control->bypass = CONTROL_BYPASS_DISABLE;
	lindex |= LOWBIT_OF(PARAM_FD_CONTROL);
	hindex |= HIGHBIT_OF(PARAM_FD_CONTROL);
	indexes++;

#ifdef ENABLE_FD_SW
	fd_config = fimc_is_itf_g_param(device, frame, PARAM_FD_CONFIG);
	lindex |= LOWBIT_OF(PARAM_FD_CONFIG);
	hindex |= HIGHBIT_OF(PARAM_FD_CONFIG);
	indexes++;
#endif

	ret = fimc_is_itf_s_param(device, frame, lindex, hindex, indexes);
	if (ret) {
		merr("fimc_is_itf_s_param is fail(%d)", device, ret);
		goto p_err;
	}

	if (bypass) {
		clear_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);
	} else {
		set_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);
	}

p_err:
	msrinfo("bypass : %d\n", device, subdev, frame, bypass);
	return ret;
}
#endif

int fimc_is_ischain_g_capability(struct fimc_is_device_ischain *device,
	ulong user_ptr)
{
	int ret = 0;
	struct camera2_sm *capability;

	capability = vzalloc(sizeof(struct camera2_sm));
	if (!capability) {
		merr("capability is NULL", device);
		ret = -ENOMEM;
		goto p_err;
	}

	ret = fimc_is_itf_g_capability(device);
	if (ret) {
		merr("fimc_is_itf_g_capability is fail(%d)", device, ret);
		ret = -EINVAL;
		goto p_err;
	}

	ret = copy_to_user((void *)user_ptr, capability, sizeof(struct camera2_sm));

p_err:
	vfree(capability);
	return ret;
}

int fimc_is_ischain_probe(struct fimc_is_device_ischain *device,
	struct fimc_is_interface *interface,
	struct fimc_is_resourcemgr *resourcemgr,
	struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_devicemgr *devicemgr,
	struct fimc_is_mem *mem,
	struct platform_device *pdev,
	u32 instance)
{
	int ret = 0;

	FIMC_BUG(!interface);
	FIMC_BUG(!mem);
	FIMC_BUG(!pdev);
	FIMC_BUG(!device);

	device->interface	= interface;
	device->mem		= mem;
	device->pdev		= pdev;
	device->pdata		= pdev->dev.platform_data;
	device->instance	= instance;
	device->groupmgr	= groupmgr;
	device->resourcemgr	= resourcemgr;
	device->devicemgr	= devicemgr;
	device->sensor		= NULL;
	device->margin_left	= 0;
	device->margin_right	= 0;
	device->margin_width	= 0;
	device->margin_top	= 0;
	device->margin_bottom	= 0;
	device->margin_height	= 0;
	device->setfile		= 0;
	device->is_region	= NULL;
#ifdef ENABLE_FD_SW
	device->fd_lib	= NULL;
#endif

	atomic_set(&device->group_open_cnt, 0);
	atomic_set(&device->open_cnt, 0);
	atomic_set(&device->init_cnt, 0);

#ifdef ENABLE_BUFFER_HIDING
	fimc_is_pipe_probe(&device->pipe);
#endif

	fimc_is_group_probe(groupmgr, &device->group_paf, NULL, device,
		fimc_is_ischain_paf_shot,
		GROUP_SLOT_PAF, ENTRY_PAF, "PXS", &fimc_is_subdev_paf_ops);
	fimc_is_group_probe(groupmgr, &device->group_3aa, NULL, device,
		fimc_is_ischain_3aa_shot,
		GROUP_SLOT_3AA, ENTRY_3AA, "3XS", &fimc_is_subdev_3aa_ops);
	fimc_is_group_probe(groupmgr, &device->group_isp, NULL, device,
		fimc_is_ischain_isp_shot,
		GROUP_SLOT_ISP, ENTRY_ISP, "IXS", &fimc_is_subdev_isp_ops);
	fimc_is_group_probe(groupmgr, &device->group_dis, NULL, device,
		fimc_is_ischain_dis_shot,
		GROUP_SLOT_DIS, ENTRY_DIS, "DXS", &fimc_is_subdev_dis_ops);
	fimc_is_group_probe(groupmgr, &device->group_dcp, NULL, device,
		fimc_is_ischain_dcp_shot,
		GROUP_SLOT_DCP, ENTRY_DCP, "DCS", &fimc_is_subdev_dcp_ops);
	fimc_is_group_probe(groupmgr, &device->group_mcs, NULL, device,
		fimc_is_ischain_mcs_shot,
		GROUP_SLOT_MCS, ENTRY_MCS, "MXS", &fimc_is_subdev_mcs_ops);
	fimc_is_group_probe(groupmgr, &device->group_vra, NULL, device,
		fimc_is_ischain_vra_shot,
		GROUP_SLOT_VRA, ENTRY_VRA, "VXS", &fimc_is_subdev_vra_ops);

	fimc_is_subdev_probe(&device->txc, instance, ENTRY_3AC, "3XC", &fimc_is_subdev_3ac_ops);
	fimc_is_subdev_probe(&device->txp, instance, ENTRY_3AP, "3XP", &fimc_is_subdev_3ap_ops);
	fimc_is_subdev_probe(&device->txf, instance, ENTRY_3AF, "3XF", &fimc_is_subdev_3af_ops);
	fimc_is_subdev_probe(&device->txg, instance, ENTRY_3AG, "3XG", &fimc_is_subdev_3ag_ops);
	fimc_is_subdev_probe(&device->ixc, instance, ENTRY_IXC, "IXC", &fimc_is_subdev_ixc_ops);
	fimc_is_subdev_probe(&device->ixp, instance, ENTRY_IXP, "IXP", &fimc_is_subdev_ixp_ops);
	fimc_is_subdev_probe(&device->mexc, instance, ENTRY_MEXC, "MEX", &fimc_is_subdev_mexc_ops);
	fimc_is_subdev_probe(&device->dxc, instance, ENTRY_DXC, "DXC", &fimc_is_subdev_dxc_ops);
	fimc_is_subdev_probe(&device->drc, instance, ENTRY_DRC, "DRC", NULL);
	fimc_is_subdev_probe(&device->odc, instance, ENTRY_ODC, "ODC", NULL);
	fimc_is_subdev_probe(&device->dnr, instance, ENTRY_DNR, "DNR", NULL);
	fimc_is_subdev_probe(&device->dc1s, instance, ENTRY_DC1S, "D1S", &fimc_is_subdev_dcxs_ops);
	fimc_is_subdev_probe(&device->dc0c, instance, ENTRY_DC0C, "D0C", &fimc_is_subdev_dcxc_ops);
	fimc_is_subdev_probe(&device->dc1c, instance, ENTRY_DC1C, "D1C", &fimc_is_subdev_dcxc_ops);
	fimc_is_subdev_probe(&device->dc2c, instance, ENTRY_DC2C, "D2C", &fimc_is_subdev_dcxc_ops);
	fimc_is_subdev_probe(&device->dc3c, instance, ENTRY_DC3C, "D3C", &fimc_is_subdev_dcxc_ops);
	fimc_is_subdev_probe(&device->dc4c, instance, ENTRY_DC4C, "D4C", &fimc_is_subdev_dcxc_ops);

	fimc_is_subdev_probe(&device->m0p, instance, ENTRY_M0P, "M0P", &fimc_is_subdev_mcsp_ops);
	fimc_is_subdev_probe(&device->m1p, instance, ENTRY_M1P, "M1P", &fimc_is_subdev_mcsp_ops);
	fimc_is_subdev_probe(&device->m2p, instance, ENTRY_M2P, "M2P", &fimc_is_subdev_mcsp_ops);
	fimc_is_subdev_probe(&device->m3p, instance, ENTRY_M3P, "M3P", &fimc_is_subdev_mcsp_ops);
	fimc_is_subdev_probe(&device->m4p, instance, ENTRY_M4P, "M4P", &fimc_is_subdev_mcsp_ops);
	fimc_is_subdev_probe(&device->m5p, instance, ENTRY_M5P, "M5P", &fimc_is_subdev_mcsp_ops);

	clear_bit(FIMC_IS_ISCHAIN_OPEN, &device->state);
	clear_bit(FIMC_IS_ISCHAIN_LOADED, &device->state);
	clear_bit(FIMC_IS_ISCHAIN_POWER_ON, &device->state);
	clear_bit(FIMC_IS_ISCHAIN_OPEN_STREAM, &device->state);
	clear_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state);

	/* clear group open state */
	clear_bit(FIMC_IS_GROUP_OPEN, &device->group_3aa.state);
	clear_bit(FIMC_IS_GROUP_OPEN, &device->group_isp.state);
	clear_bit(FIMC_IS_GROUP_OPEN, &device->group_dis.state);
	clear_bit(FIMC_IS_GROUP_OPEN, &device->group_dcp.state);
	clear_bit(FIMC_IS_GROUP_OPEN, &device->group_mcs.state);
	clear_bit(FIMC_IS_GROUP_OPEN, &device->group_vra.state);

	/* clear subdevice state */
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->group_3aa.leader.state);
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->group_isp.leader.state);
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->group_dis.leader.state);
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->group_dcp.leader.state);
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->group_mcs.leader.state);
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->group_vra.leader.state);
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->txc.state);
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->txp.state);
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->txf.state);
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->txg.state);
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->ixc.state);
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->ixp.state);
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->mexc.state);
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->dxc.state);
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->drc.state);
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->odc.state);
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->dnr.state);
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->scc.state);
	clear_bit(FIMC_IS_SUBDEV_OPEN, &device->scp.state);

	clear_bit(FIMC_IS_SUBDEV_START, &device->group_3aa.leader.state);
	clear_bit(FIMC_IS_SUBDEV_START, &device->group_isp.leader.state);
	clear_bit(FIMC_IS_SUBDEV_START, &device->group_dis.leader.state);
	clear_bit(FIMC_IS_SUBDEV_START, &device->group_dcp.leader.state);
	clear_bit(FIMC_IS_SUBDEV_START, &device->group_mcs.leader.state);
	clear_bit(FIMC_IS_SUBDEV_START, &device->group_vra.leader.state);
	clear_bit(FIMC_IS_SUBDEV_START, &device->txc.state);
	clear_bit(FIMC_IS_SUBDEV_START, &device->txp.state);
	clear_bit(FIMC_IS_SUBDEV_START, &device->txf.state);
	clear_bit(FIMC_IS_SUBDEV_START, &device->txg.state);
	clear_bit(FIMC_IS_SUBDEV_START, &device->ixc.state);
	clear_bit(FIMC_IS_SUBDEV_START, &device->ixp.state);
	clear_bit(FIMC_IS_SUBDEV_START, &device->mexc.state);
	clear_bit(FIMC_IS_SUBDEV_START, &device->dxc.state);
	clear_bit(FIMC_IS_SUBDEV_START, &device->drc.state);
	clear_bit(FIMC_IS_SUBDEV_START, &device->odc.state);
	clear_bit(FIMC_IS_SUBDEV_START, &device->dnr.state);
	clear_bit(FIMC_IS_SUBDEV_START, &device->scc.state);
	clear_bit(FIMC_IS_SUBDEV_START, &device->scp.state);

	return ret;
}

static int fimc_is_ischain_open(struct fimc_is_device_ischain *device)
{
	struct fimc_is_minfo *minfo = NULL;
	u32 offset_region = 0;
	int ret = 0;
	bool has_vra_ch1_only = false;

	FIMC_BUG(!device);
	FIMC_BUG(!device->groupmgr);
	FIMC_BUG(!device->resourcemgr);

	device->minfo = &device->resourcemgr->minfo;
	minfo = device->minfo;

	clear_bit(FIMC_IS_ISCHAIN_INITING, &device->state);
	clear_bit(FIMC_IS_ISCHAIN_INIT, &device->state);
	clear_bit(FIMC_IS_ISCHAIN_START, &device->state);
	clear_bit(FIMC_IS_ISCHAIN_OPEN_STREAM, &device->state);
	clear_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state);

	/* 2. Init variables */
#if !defined(FAST_FDAE)
	memset(&device->fdUd, 0, sizeof(struct camera2_fd_uctl));
#endif
#ifdef ENABLE_SENSOR_DRIVER
	memset(&device->peri_ctls, 0, sizeof(struct camera2_uctl)*SENSOR_MAX_CTL);
#endif

	/* initial state, it's real apply to setting when opening */
	atomic_set(&device->init_cnt, 0);
	device->margin_left	= 0;
	device->margin_right	= 0;
	device->margin_width	= 0;
	device->margin_top	= 0;
	device->margin_bottom	= 0;
	device->margin_height	= 0;
	device->sensor		= NULL;
	device->module		= 0;

#ifdef ENABLE_IS_CORE
	offset_region = (FW_MEM_SIZE - ((device->instance + 1) * PARAM_REGION_SIZE));
	device->is_region	= (struct is_region *)(minfo->kvaddr + offset_region);
#else
	offset_region = device->instance * PARAM_REGION_SIZE;
	device->is_region	= (struct is_region *)(minfo->kvaddr_region + offset_region);
#endif
	device->kvaddr_shared	= device->is_region->shared[0];
	device->dvaddr_shared	= minfo->dvaddr +
				(u32)((ulong)&device->is_region->shared[0] - minfo->kvaddr);

#ifdef ENABLE_HYBRID_FD
	spin_lock_init(&device->is_region->fdae_info.slock);
#endif
#ifdef SOC_DRC
	fimc_is_subdev_open(&device->drc, NULL, (void *)&init_drc_param.control);
#endif

#ifdef SOC_ODC
	fimc_is_subdev_open(&device->odc, NULL, (void *)&init_odc_param.control);
#endif

#ifdef SOC_DNR
	fimc_is_subdev_open(&device->dnr, NULL, (void *)&init_tdnr_param.control);
#endif

#ifdef SOC_VRA
	ret = fimc_is_hw_g_ctrl(NULL, 0, HW_G_CTRL_HAS_VRA_CH1_ONLY, (void *)&has_vra_ch1_only);
	if(!has_vra_ch1_only)
		fimc_is_subdev_open(&device->group_vra.leader, NULL, (void *)&init_vra_param.control);
#endif

	ret = fimc_is_devicemgr_open(device->devicemgr, (void *)device, FIMC_IS_DEVICE_ISCHAIN);
	if (ret) {
		err("fimc_is_devicemgr_open is fail(%d)", ret);
		goto p_err;
	}

	/* for mediaserver force close */
	ret = fimc_is_resource_get(device->resourcemgr, RESOURCE_TYPE_ISCHAIN);
	if (ret) {
		merr("fimc_is_resource_get is fail", device);
		goto p_err;
	}

#ifdef ENABLE_CLOCK_GATE
	{
		struct fimc_is_core *core;
		if (sysfs_debug.en_clk_gate &&
				sysfs_debug.clk_gate_mode == CLOCK_GATE_MODE_HOST) {
			core = (struct fimc_is_core *)device->interface->core;
			fimc_is_clk_gate_lock_set(core, device->instance, true);
			fimc_is_wrap_clk_gate_set(core, (1 << GROUP_ID_MAX) - 1, true);
		}
	}
#endif

p_err:
	minfo("[ISC:D] %s():%d\n", device, __func__, ret);
	return ret;
}

int fimc_is_ischain_open_wrap(struct fimc_is_device_ischain *device, bool EOS)
{
	int ret = 0;

	FIMC_BUG(!device);

	if (test_bit(FIMC_IS_ISCHAIN_CLOSING, &device->state)) {
		merr("open is invalid on closing", device);
		ret = -EPERM;
		goto p_err;
	}

	if (test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
		merr("already open", device);
		ret = -EMFILE;
		goto p_err;
	}

	if (atomic_read(&device->open_cnt) > ENTRY_END) {
		merr("open count is invalid(%d)", device, atomic_read(&device->open_cnt));
		ret = -EMFILE;
		goto p_err;
	}

	if (EOS) {
		ret = fimc_is_ischain_open(device);
		if (ret) {
			merr("fimc_is_chain_open is fail(%d)", device, ret);
			goto p_err;
		}

		clear_bit(FIMC_IS_ISCHAIN_OPENING, &device->state);
		set_bit(FIMC_IS_ISCHAIN_OPEN, &device->state);
	} else {
		atomic_inc(&device->open_cnt);
		set_bit(FIMC_IS_ISCHAIN_OPENING, &device->state);
	}

p_err:
	return ret;
}

static int fimc_is_ischain_close(struct fimc_is_device_ischain *device)
{
	int ret = 0;
	bool has_vra_ch1_only = false;

	FIMC_BUG(!device);

	if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
		mwarn("this chain has not been opened", device);
		goto p_err;
	}

#ifdef ENABLE_CLOCK_GATE
	{
		struct fimc_is_core *core = (struct fimc_is_core *)device->resourcemgr->private_data;
		if (sysfs_debug.en_clk_gate && (sysfs_debug.clk_gate_mode == CLOCK_GATE_MODE_HOST)) {
			fimc_is_clk_gate_lock_set(core, device->instance, true);
			fimc_is_wrap_clk_gate_set(core, (1 << GROUP_ID_MAX) - 1, true);
		}
	}
#endif

#ifdef ENABLE_FD_SW
	if (device->fd_lib) {
		fimc_is_lib_fd_close(device->fd_lib);
		vfree(device->fd_lib);
		device->fd_lib = NULL;
	}
#endif

	/* subdev close */
#ifdef SOC_DRC
	fimc_is_subdev_close(&device->drc);
#endif
#ifdef SOC_ODC
	fimc_is_subdev_close(&device->odc);
#endif
#ifdef SOC_DNR
	fimc_is_subdev_close(&device->dnr);
#endif
#ifdef SOC_VRA
	ret = fimc_is_hw_g_ctrl(NULL, 0, HW_G_CTRL_HAS_VRA_CH1_ONLY, (void *)&has_vra_ch1_only);
	if(!has_vra_ch1_only)
		fimc_is_subdev_close(&device->group_vra.leader);
#endif

	ret = fimc_is_itf_close(device);
	if (ret)
		merr("fimc_is_itf_close is fail", device);

	/* for mediaserver force close */
	ret = fimc_is_resource_put(device->resourcemgr, RESOURCE_TYPE_ISCHAIN);
	if (ret)
		merr("fimc_is_resource_put is fail", device);

#ifdef ENABLE_CLOCK_GATE
	{
		struct fimc_is_core *core = (struct fimc_is_core *)device->resourcemgr->private_data;
		if (sysfs_debug.en_clk_gate && sysfs_debug.clk_gate_mode == CLOCK_GATE_MODE_HOST)
			fimc_is_clk_gate_lock_set(core, device->instance, false);
	}
#endif
	ret = fimc_is_devicemgr_close(device->devicemgr, (void *)device, FIMC_IS_DEVICE_ISCHAIN);
	if (ret) {
		err("fimc_is_devicemgr_close is fail(%d)", ret);
		goto p_err;
	}

	atomic_set(&device->open_cnt, 0);
	clear_bit(FIMC_IS_ISCHAIN_OPEN_STREAM, &device->state);

	minfo("[ISC:D] %s():%d\n", device, __func__, ret);

p_err:
	return ret;
}

int fimc_is_ischain_close_wrap(struct fimc_is_device_ischain *device)
{
	int ret = 0;

	FIMC_BUG(!device);

	if (test_bit(FIMC_IS_ISCHAIN_OPENING, &device->state)) {
		mwarn("close on opening", device);
		clear_bit(FIMC_IS_ISCHAIN_OPENING, &device->state);
	}

	if (!atomic_read(&device->open_cnt)) {
		merr("open count is invalid(%d)", device, atomic_read(&device->open_cnt));
		ret = -ENOENT;
		goto p_err;
	}

	atomic_dec(&device->open_cnt);
	set_bit(FIMC_IS_ISCHAIN_CLOSING, &device->state);

	if (!atomic_read(&device->open_cnt)) {
		ret = fimc_is_ischain_close(device);
		if (ret) {
			merr("fimc_is_chain_close is fail(%d)", device, ret);
			goto p_err;
		}

		clear_bit(FIMC_IS_ISCHAIN_CLOSING, &device->state);
		clear_bit(FIMC_IS_ISCHAIN_OPEN, &device->state);
	}

p_err:
	return ret;
}

static int fimc_is_ischain_init(struct fimc_is_device_ischain *device,
	u32 module_id)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_path_info *path;
	struct fimc_is_vender *vender;
	u32 flag;

	FIMC_BUG(!device);
	FIMC_BUG(!device->sensor);

	mdbgd_ischain("%s(module : %d)\n", device, __func__, module_id);

	sensor = device->sensor;
	path = &device->path;
	core = (struct fimc_is_core *)platform_get_drvdata(device->pdev);
	vender = &core->vender;

	if (test_bit(FIMC_IS_ISCHAIN_INIT, &device->state)) {
		minfo("stream is already initialized", device);
		goto p_err;
	}

	if (!test_bit(FIMC_IS_SENSOR_S_INPUT, &sensor->state)) {
		merr("I2C gpio is not yet set", device);
		ret = -EINVAL;
		goto p_err;
	}

	ret = fimc_is_sensor_g_module(sensor, &module);
	if (ret) {
		merr("fimc_is_sensor_g_module is fail(%d)", device, ret);
		goto p_err;
	}

	if (module->sensor_id != module_id) {
		merr("module id is invalid(%d != %d)", device, module->sensor_id, module_id);
		ret = -EINVAL;
		goto p_err;
	}

	if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state)) {
		/*
		 * Initiate cal address. If this value is 0, F/W does not load cal data.
		 * So vender must set this cal_address to let F/W load cal data.
		 */
		module->ext.sensor_con.cal_address = 0;
		ret = fimc_is_vender_cal_load(vender, module);
		if (ret) {
			merr("fimc_is_vender_cal_load is fail(%d)", device, ret);
			goto p_err;
		}
	}

	ret = fimc_is_vender_setfile_sel(vender, module->setfile_name, module->position);
	if (ret) {
		merr("fimc_is_vender_setfile_sel is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_itf_enum(device);
	if (ret) {
		merr("fimc_is_itf_enum is fail(%d)", device, ret);
		goto p_err;
	}

	flag = test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state) ? 1 : 0;

	flag |= (sensor->pdata->scenario == SENSOR_SCENARIO_STANDBY) ? (0x1 << 16) : (0x0 << 16);

	ret = fimc_is_itf_open(device, module_id, flag, path, &module->ext);
	if (ret) {
		merr("open fail", device);
		goto p_err;
	}

#ifdef ENABLE_FD_SW
	device->fd_lib = vzalloc(sizeof(struct fimc_is_lib));
		if (!device->fd_lib) {
		merr("fimc_is_library allocation is fail\n", device);
		ret = -ENOMEM;
		goto p_err;
	}

	ret = fimc_is_lib_fd_open(device->fd_lib);
	if (ret != FDS_OK) {
		merr("failed to fimc_is_lib_open (%d)\n", device, ret);
		vfree(device->fd_lib);
		device->fd_lib = NULL;
		ret = -EINVAL;
		goto p_err;
	}
#endif

	ret = fimc_is_itf_setfile(device, vender);
	if (ret) {
		merr("setfile fail", device);
		goto p_err;
	}

#ifdef ENABLE_IS_CORE
	if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state)) {
		ret = fimc_is_itf_stream_off(device);
		if (ret) {
			merr("streamoff fail", device);
			goto p_err;
		}
	}

	ret = fimc_is_itf_init_process_stop(device);
	if (ret) {
		merr("fimc_is_itf_init_process_stop is fail", device);
		goto p_err;
	}
#endif

	device->module = module_id;
	clear_bit(FIMC_IS_ISCHAIN_INITING, &device->state);
	set_bit(FIMC_IS_ISCHAIN_INIT, &device->state);

p_err:
	return ret;
}

static int fimc_is_ischain_init_wrap(struct fimc_is_device_ischain *device,
	u32 stream_type,
	u32 module_id)
{
	int ret = 0;
	u32 sindex;
	struct fimc_is_core *core;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_module_enum *module;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_vender_specific *priv;
	u32 sensor_id;

	FIMC_BUG(!device);
	FIMC_BUG(!device->groupmgr);

	if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
		merr("NOT yet open", device);
		ret = -EMFILE;
		goto p_err;
	}

	if (test_bit(FIMC_IS_ISCHAIN_START, &device->state)) {
		merr("already start", device);
		ret = -EINVAL;
		goto p_err;
	}

	if (atomic_read(&device->init_cnt) >= atomic_read(&device->group_open_cnt)) {
		merr("init count value(%d) is invalid", device, atomic_read(&device->init_cnt));
		ret = -EINVAL;
		goto p_err;
	}

	groupmgr = device->groupmgr;
	core = container_of(groupmgr, struct fimc_is_core, groupmgr);
	atomic_inc(&device->init_cnt);
	set_bit(FIMC_IS_ISCHAIN_INITING, &device->state);
	mdbgd_ischain("%s(%d, %d)\n", device, __func__,
		atomic_read(&device->init_cnt), atomic_read(&device->group_open_cnt));

	if (atomic_read(&device->init_cnt) == atomic_read(&device->group_open_cnt)) {
		if (test_bit(FIMC_IS_GROUP_OPEN, &device->group_3aa.state) &&
			!test_bit(FIMC_IS_GROUP_INIT, &device->group_3aa.state)) {
			merr("invalid 3aa group state", device);
			ret = -EINVAL;
			goto p_err;
		}

		if (test_bit(FIMC_IS_GROUP_OPEN, &device->group_isp.state) &&
			!test_bit(FIMC_IS_GROUP_INIT, &device->group_isp.state)) {
			merr("invalid isp group state", device);
			ret = -EINVAL;
			goto p_err;
		}

		if (test_bit(FIMC_IS_GROUP_OPEN, &device->group_dis.state) &&
			!test_bit(FIMC_IS_GROUP_INIT, &device->group_dis.state)) {
			merr("invalid dis group state", device);
			ret = -EINVAL;
			goto p_err;
		}

		if (test_bit(FIMC_IS_GROUP_OPEN, &device->group_dcp.state) &&
			!test_bit(FIMC_IS_GROUP_INIT, &device->group_dcp.state)) {
			merr("invalid dcp group state", device);
			ret = -EINVAL;
			goto p_err;
		}

		if (test_bit(FIMC_IS_GROUP_OPEN, &device->group_vra.state) &&
			!test_bit(FIMC_IS_GROUP_INIT, &device->group_vra.state)) {
			merr("invalid vra group state", device);
			ret = -EINVAL;
			goto p_err;
		}

		for (sindex = 0; sindex < FIMC_IS_SENSOR_COUNT; ++sindex) {
			sensor = &core->sensor[sindex];

			if (!test_bit(FIMC_IS_SENSOR_OPEN, &sensor->state))
				continue;

			if (!test_bit(FIMC_IS_SENSOR_S_INPUT, &sensor->state))
				continue;

			ret = fimc_is_sensor_g_module(sensor, &module);
			if (ret) {
				merr("fimc_is_sensor_g_module is fail(%d)", device, ret);
				goto p_err;
			}

			priv = core->vender.private_data;

			switch (module_id) {
			case SENSOR_POSITION_REAR:
				sensor_id = priv->rear_sensor_id;
				break;
			case SENSOR_POSITION_FRONT:
				sensor_id = priv->front_sensor_id;
				break;
			case SENSOR_POSITION_REAR2:
				sensor_id = priv->rear2_sensor_id;
				break;
			case SENSOR_POSITION_FRONT2:
				sensor_id = priv->front2_sensor_id;
				break;
			case SENSOR_POSITION_REAR3:
				sensor_id = priv->rear3_sensor_id;
				break;
			case SENSOR_POSITION_REAR_TOF:
				sensor_id = priv->rear_tof_sensor_id;
				break;
			case SENSOR_POSITION_FRONT_TOF:
				sensor_id = priv->front_tof_sensor_id;
				break;
#if defined(SECURE_CAMERA_IRIS)
			case SENSOR_POSITION_SECURE:
				sensor_id = priv->secure_sensor_id;
				break;
#endif
			default:
				merr("invalid module position(%d)", device, module->position);
				ret = -EINVAL;
				goto p_err;
			}

			if (module->sensor_id == sensor_id) {
				device->sensor = sensor;
				module_id = sensor_id;
				break;
			}
		}

		if (sindex >= FIMC_IS_SENSOR_COUNT) {
			merr("moduel id(%d) is invalid", device, module_id);
			ret = -EINVAL;
			goto p_err;
		}

		if (!sensor || !sensor->pdata) {
			merr("sensor is NULL", device);
			ret = -EINVAL;
			goto p_err;
		}

		device->path.sensor_name = module_id;
		device->path.mipi_csi = sensor->pdata->csi_ch;
		device->path.fimc_lite = sensor->pdata->flite_ch;

		if (stream_type) {
			set_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state);
		} else {
			clear_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state);
			sensor->ischain = device;
		}

		ret = fimc_is_groupmgr_init(device->groupmgr, device);
		if (ret) {
			merr("fimc_is_groupmgr_init is fail(%d)", device, ret);
			goto p_err;
		}

		/* register sensor(DDK, RTA)/preporcessor interface*/
		if (!test_bit(FIMC_IS_SENSOR_ITF_REGISTER, &sensor->state)) {
			ret = fimc_is_sensor_register_itf(sensor);
			if (ret) {
				merr("fimc_is_sensor_register_itf fail(%d)", device, ret);
				goto p_err;
			}

			set_bit(FIMC_IS_SENSOR_ITF_REGISTER, &sensor->state);
		}

		ret = fimc_is_ischain_init(device, module_id);
		if (ret) {
			merr("fimc_is_ischain_init is fail(%d)", device, ret);
			goto p_err;
		}

		atomic_set(&device->init_cnt, 0);

		ret = fimc_is_devicemgr_binding(device->devicemgr, sensor, device, FIMC_IS_DEVICE_ISCHAIN);
		if (ret) {
			merr("fimc_is_devicemgr_binding is fail", device);
			goto p_err;
		}
	}

p_err:
	return ret;
}

static int fimc_is_ischain_start(struct fimc_is_device_ischain *device)
{
	int ret = 0;
	u32 lindex = 0;
	u32 hindex = 0;
	u32 indexes = 0;

	FIMC_BUG(!device);
	FIMC_BUG(!device->sensor);

	ret = fimc_is_hw_ischain_cfg((void *)device);
	if (ret) {
		merr("hw init fail", device);
		goto p_err;
	}

	ret = fimc_is_ischain_s_sensor_size(device, NULL, &lindex, &hindex, &indexes);
	if (ret) {
		merr("fimc_is_ischain_s_sensor_size is fail(%d)", device, ret);
		goto p_err;
	}

#if !defined(FAST_FDAE)
	/* previous fd infomation should be clear */
	memset(&device->fdUd, 0x0, sizeof(struct camera2_fd_uctl));
#endif

#ifdef ENABLE_ULTRA_FAST_SHOT
	memset(&device->fastctlmgr, 0x0, sizeof(struct fast_control_mgr));
	memset(&device->is_region->fast_ctl, 0x0, sizeof(struct is_fast_control));
#endif

	/* NI information clear */
	memset(&device->cur_noise_idx, 0xFFFFFFFF, sizeof(device->cur_noise_idx));
	memset(&device->next_noise_idx, 0xFFFFFFFF, sizeof(device->next_noise_idx));

	ret = fimc_is_itf_sensor_mode(device);
	if (ret) {
		merr("fimc_is_itf_sensor_mode is fail(%d)", device, ret);
		goto p_err;
	}

	if (device->sensor->scene_mode >= AA_SCENE_MODE_DISABLED)
		device->is_region->parameter.taa.vdma1_input.scene_mode = device->sensor->scene_mode;

	ret = fimc_is_ischain_s_path(device, &lindex, &hindex, &indexes);
	if (ret) {
		merr("fimc_is_ischain_s_path is fail(%d)", device, ret);
		goto p_err;
	}

	lindex = 0xFFFFFFFF;
	hindex = 0xFFFFFFFF;
	indexes = 64;

	ret = fimc_is_itf_s_param(device , NULL, lindex, hindex, indexes);
	if (ret) {
		merr("fimc_is_itf_s_param is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_itf_f_param(device);
	if (ret) {
		merr("fimc_is_itf_f_param is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_itf_sys_ctl(device, IS_SYS_CLOCK_GATE, sysfs_debug.clk_gate_mode);
	if (ret) {
		merr("fimc_is_itf_sys_ctl is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_itf_init_process_start(device);
	if (ret) {
		merr("fimc_is_itf_init_process_start is fail(%d)", device, ret);
		goto p_err;
	}

#ifdef ENABLE_DVFS
	ret = fimc_is_dvfs_sel_table(device->resourcemgr);
	if (ret) {
		merr("fimc_is_dvfs_sel_table is fail(%d)", device, ret);
		goto p_err;
	}
#endif

	set_bit(FIMC_IS_ISCHAIN_START, &device->state);

p_err:
	minfo("[ISC:D] %s(%d):%d\n", device, __func__,
		device->setfile & FIMC_IS_SETFILE_MASK, ret);

	return ret;
}

int fimc_is_ischain_start_wrap(struct fimc_is_device_ischain *device,
	struct fimc_is_group *group)
{
	int ret = 0;
	struct fimc_is_group *leader;

	if (!test_bit(FIMC_IS_ISCHAIN_INIT, &device->state)) {
		merr("device is not yet init", device);
		ret = -EINVAL;
		goto p_err;
	}

	leader = device->groupmgr->leader[device->instance];
	if (leader != group)
		goto p_err;

	if (test_bit(FIMC_IS_ISCHAIN_START, &device->state)) {
		merr("already start", device);
		ret = -EINVAL;
		goto p_err;
	}

	/* param region init with default value */
	fimc_is_itf_param_init(device->is_region);

	ret = fimc_is_groupmgr_start(device->groupmgr, device);
	if (ret) {
		merr("fimc_is_groupmgr_start is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_start(device);
	if (ret) {
		merr("fimc_is_chain_start is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ischain_stop(struct fimc_is_device_ischain *device)
{
	int ret = 0;

	return ret;
}

int fimc_is_ischain_stop_wrap(struct fimc_is_device_ischain *device,
	struct fimc_is_group *group)
{
	int ret = 0;
	struct fimc_is_group *leader;

	leader = device->groupmgr->leader[device->instance];
	if (leader != group)
		goto p_err;

	if (!test_bit(FIMC_IS_ISCHAIN_START, &device->state)) {
		mwarn("already stop", device);
		goto p_err;
	}

	ret = fimc_is_groupmgr_stop(device->groupmgr, device);
	if (ret) {
		merr("fimc_is_groupmgr_stop is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_stop(device);
	if (ret) {
		merr("fimc_is_ischain_stop is fail(%d)", device, ret);
		goto p_err;
	}

	clear_bit(FIMC_IS_ISCHAIN_START, &device->state);

p_err:
	return ret;
}

int fimc_is_ischain_paf_open(struct fimc_is_device_ischain *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	int ret_err = 0;
	u32 group_id;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);
	FIMC_BUG(!vctx);
	FIMC_BUG(!GET_VIDEO(vctx));

	groupmgr = device->groupmgr;
	group = &device->group_paf;
	group_id = GROUP_ID_PAF0 + GET_PAFXS_ID(GET_VIDEO(vctx));

	ret = fimc_is_group_open(groupmgr,
		group,
		group_id,
		vctx);
	if (ret) {
		merr("fimc_is_group_open is fail(%d)", device, ret);
		goto err_group_open;
	}

	ret = fimc_is_ischain_open_wrap(device, false);
	if (ret) {
		merr("fimc_is_ischain_open_wrap is fail(%d)", device, ret);
		goto err_ischain_open;
	}

	atomic_inc(&device->group_open_cnt);

	return 0;

err_ischain_open:
	ret_err = fimc_is_group_close(groupmgr, group);
	if (ret_err)
		merr("fimc_is_group_close is fail(%d)", device, ret_err);
err_group_open:
	return ret;
}

int fimc_is_ischain_paf_close(struct fimc_is_device_ischain *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;
	struct fimc_is_queue *queue;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_paf;
	queue = GET_QUEUE(vctx);

	/* for mediaserver dead */
	if (test_bit(FIMC_IS_GROUP_START, &group->state)) {
		mgwarn("sudden group close", device, group);
		if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
			fimc_is_itf_sudden_stop_wrap(device, device->instance);
		set_bit(FIMC_IS_GROUP_REQUEST_FSTOP, &group->state);
		if (test_bit(FIMC_IS_HAL_DEBUG_SUDDEN_DEAD_DETECT, &sysfs_debug.hal_debug_mode)) {
			msleep(sysfs_debug.hal_debug_delay);
			panic("HAL sudden group close #1");
		}
	}

	if (group->head && test_bit(FIMC_IS_GROUP_START, &group->head->state)) {
		mgwarn("sudden group close", device, group);
		if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
			fimc_is_itf_sudden_stop_wrap(device, device->instance);
		set_bit(FIMC_IS_GROUP_REQUEST_FSTOP, &group->state);
		if (test_bit(FIMC_IS_HAL_DEBUG_SUDDEN_DEAD_DETECT, &sysfs_debug.hal_debug_mode)) {
			msleep(sysfs_debug.hal_debug_delay);
			panic("HAL sudden group close #2");
		}
	}

	ret = fimc_is_ischain_paf_stop(device, queue);
	if (ret)
		merr("fimc_is_ischain_paf_rdma_stop is fail", device);

	ret = fimc_is_group_close(groupmgr, group);
	if (ret)
		merr("fimc_is_group_close is fail", device);

	ret = fimc_is_ischain_close_wrap(device);
	if (ret)
		merr("fimc_is_ischain_close_wrap is fail(%d)", device, ret);

	atomic_dec(&device->group_open_cnt);

	return ret;
}

int fimc_is_ischain_paf_s_input(struct fimc_is_device_ischain *device,
	u32 stream_type,
	u32 module_id,
	u32 video_id,
	u32 input_type,
	u32 stream_leader)
{
	int ret = 0;
	struct fimc_is_group *group;
	struct fimc_is_groupmgr *groupmgr;

	FIMC_BUG(!device);
	FIMC_BUG(!device->groupmgr);

	groupmgr = device->groupmgr;
	group = &device->group_paf;

	mdbgd_ischain("%s()\n", device, __func__);

	ret = fimc_is_group_init(groupmgr, group, input_type, video_id, stream_leader);
	if (ret) {
		merr("fimc_is_group_init is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_init_wrap(device, stream_type, module_id);
	if (ret) {
		merr("fimc_is_ischain_init_wrap is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ischain_paf_start(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_paf;

	ret = fimc_is_group_start(groupmgr, group);
	if (ret) {
		merr("fimc_is_group_start is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_start_wrap(device, group);
	if (ret) {
		merr("fimc_is_ischain_start_wrap is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ischain_paf_stop(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_paf;

	ret = fimc_is_group_stop(groupmgr, group);
	if (ret) {
		merr("fimc_is_group_stop is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_stop_wrap(device, group);
	if (ret) {
		merr("fimc_is_ischain_stop_wrap is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	mginfo("%s(%d):%d\n", device, group,  __func__, atomic_read(&group->scount), ret);
	return ret;
}

static int fimc_is_ischain_paf_reqbufs(void *qdevice,
	struct fimc_is_queue *queue, u32 count)
{
#ifdef ENABLE_IS_CORE
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	group = &device->group_paf;

	if (!count) {
		ret = fimc_is_itf_unmap(device, GROUP_ID(group->id));
		if (ret)
			merr("fimc_is_itf_unmap is fail(%d)", device, ret);
	}

	return ret;
#else
	return 0;
#endif
}

static int fimc_is_ischain_paf_s_format(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_subdev *leader;

	FIMC_BUG(!device);
	FIMC_BUG(!queue);

	leader = &device->group_paf.leader;

	leader->input.width = queue->framecfg.width;
	leader->input.height = queue->framecfg.height;

	leader->input.crop.x = 0;
	leader->input.crop.y = 0;
	leader->input.crop.w = leader->input.width;
	leader->input.crop.h = leader->input.height;

	return ret;
}

int fimc_is_ischain_paf_buffer_queue(struct fimc_is_device_ischain *device,
	struct fimc_is_queue *queue,
	u32 index)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);
	FIMC_BUG(!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state));

	mdbgs_ischain(4, "%s\n", device, __func__);

	groupmgr = device->groupmgr;
	group = &device->group_paf;

	ret = fimc_is_group_buffer_queue(groupmgr, group, queue, index);
	if (ret)
		merr("fimc_is_group_buffer_queue is fail(%d)", device, ret);

	return ret;
}

int fimc_is_ischain_paf_buffer_finish(struct fimc_is_device_ischain *device,
	u32 index)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	mdbgs_ischain(4, "%s\n", device, __func__);

	groupmgr = device->groupmgr;
	group = &device->group_paf;

	ret = fimc_is_group_buffer_finish(groupmgr, group, index);
	if (ret)
		merr("fimc_is_group_buffer_finish is fail(%d)", device, ret);

	return ret;
}

const struct fimc_is_queue_ops fimc_is_ischain_paf_ops = {
	.start_streaming	= fimc_is_ischain_paf_start,
	.stop_streaming		= fimc_is_ischain_paf_stop,
	.s_format		= fimc_is_ischain_paf_s_format,
	.request_bufs		= fimc_is_ischain_paf_reqbufs
};

int fimc_is_ischain_3aa_open(struct fimc_is_device_ischain *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	int ret_err = 0;
	u32 group_id;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);
	FIMC_BUG(!vctx);
	FIMC_BUG(!GET_VIDEO(vctx));

	groupmgr = device->groupmgr;
	group = &device->group_3aa;
	group_id = GROUP_ID_3AA0 + GET_3XS_ID(GET_VIDEO(vctx));

	ret = fimc_is_group_open(groupmgr,
		group,
		group_id,
		vctx);
	if (ret) {
		merr("fimc_is_group_open is fail(%d)", device, ret);
		goto err_group_open;
	}

	ret = fimc_is_ischain_open_wrap(device, false);
	if (ret) {
		merr("fimc_is_ischain_open_wrap is fail(%d)", device, ret);
		goto err_ischain_open;
	}

	atomic_inc(&device->group_open_cnt);

	return 0;

err_ischain_open:
	ret_err = fimc_is_group_close(groupmgr, group);
	if (ret_err)
		merr("fimc_is_group_close is fail(%d)", device, ret_err);
err_group_open:
	return ret;
}

int fimc_is_ischain_3aa_close(struct fimc_is_device_ischain *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;
	struct fimc_is_queue *queue;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_3aa;
	queue = GET_QUEUE(vctx);

	/* for mediaserver dead */
	if (test_bit(FIMC_IS_GROUP_START, &group->state)) {
		mgwarn("sudden group close", device, group);
		if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
			fimc_is_itf_sudden_stop_wrap(device, device->instance);
		set_bit(FIMC_IS_GROUP_REQUEST_FSTOP, &group->state);
		if (test_bit(FIMC_IS_HAL_DEBUG_SUDDEN_DEAD_DETECT, &sysfs_debug.hal_debug_mode)) {
			msleep(sysfs_debug.hal_debug_delay);
			panic("HAL sudden group close #1");
		}
	}

	if (group->head && test_bit(FIMC_IS_GROUP_START, &group->head->state)) {
		mgwarn("sudden group close", device, group);
		if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
			fimc_is_itf_sudden_stop_wrap(device, device->instance);
		set_bit(FIMC_IS_GROUP_REQUEST_FSTOP, &group->state);
		if (test_bit(FIMC_IS_HAL_DEBUG_SUDDEN_DEAD_DETECT, &sysfs_debug.hal_debug_mode)) {
			msleep(sysfs_debug.hal_debug_delay);
			panic("HAL sudden group close #2");
		}
	}

	ret = fimc_is_ischain_3aa_stop(device, queue);
	if (ret)
		merr("fimc_is_ischain_3aa_stop is fail", device);

	ret = fimc_is_group_close(groupmgr, group);
	if (ret)
		merr("fimc_is_group_close is fail", device);

	ret = fimc_is_ischain_close_wrap(device);
	if (ret)
		merr("fimc_is_ischain_close_wrap is fail(%d)", device, ret);

	atomic_dec(&device->group_open_cnt);

	return ret;
}

int fimc_is_ischain_3aa_s_input(struct fimc_is_device_ischain *device,
	u32 stream_type,
	u32 module_id,
	u32 video_id,
	u32 input_type,
	u32 stream_leader)
{
	int ret = 0;
	struct fimc_is_group *group;
	struct fimc_is_groupmgr *groupmgr;

	FIMC_BUG(!device);
	FIMC_BUG(!device->groupmgr);

	groupmgr = device->groupmgr;
	group = &device->group_3aa;

	mdbgd_ischain("%s()\n", device, __func__);

	ret = fimc_is_group_init(groupmgr, group, input_type, video_id, stream_leader);
	if (ret) {
		merr("fimc_is_group_init is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_init_wrap(device, stream_type, module_id);
	if (ret) {
		merr("fimc_is_ischain_init_wrap is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ischain_3aa_start(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_3aa;

	ret = fimc_is_group_start(groupmgr, group);
	if (ret) {
		merr("fimc_is_group_start is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_start_wrap(device, group);
	if (ret) {
		merr("fimc_is_ischain_start_wrap is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ischain_3aa_stop(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_3aa;

	ret = fimc_is_group_stop(groupmgr, group);
	if (ret) {
		merr("fimc_is_group_stop is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_stop_wrap(device, group);
	if (ret) {
		merr("fimc_is_ischain_stop_wrap is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	mginfo("%s(%d):%d\n", device, group,  __func__, atomic_read(&group->scount), ret);
	return ret;
}

static int fimc_is_ischain_3aa_reqbufs(void *qdevice,
	struct fimc_is_queue *queue, u32 count)
{
#ifdef ENABLE_IS_CORE
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	group = &device->group_3aa;

	if (!count) {
		ret = fimc_is_itf_unmap(device, GROUP_ID(group->id));
		if (ret)
			merr("fimc_is_itf_unmap is fail(%d)", device, ret);
	}

	return ret;
#else
	return 0;
#endif
}

static int fimc_is_ischain_3aa_s_format(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_subdev *leader;

	FIMC_BUG(!device);
	FIMC_BUG(!queue);

	leader = &device->group_3aa.leader;

	leader->input.width = queue->framecfg.width;
	leader->input.height = queue->framecfg.height;

	leader->input.crop.x = 0;
	leader->input.crop.y = 0;
	leader->input.crop.w = leader->input.width;
	leader->input.crop.h = leader->input.height;

	return ret;
}

int fimc_is_ischain_3aa_buffer_queue(struct fimc_is_device_ischain *device,
	struct fimc_is_queue *queue,
	u32 index)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);
	FIMC_BUG(!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state));

	mdbgs_ischain(4, "%s\n", device, __func__);

	groupmgr = device->groupmgr;
	group = &device->group_3aa;

	ret = fimc_is_group_buffer_queue(groupmgr, group, queue, index);
	if (ret)
		merr("fimc_is_group_buffer_queue is fail(%d)", device, ret);

	return ret;
}

int fimc_is_ischain_3aa_buffer_finish(struct fimc_is_device_ischain *device,
	u32 index)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	mdbgs_ischain(4, "%s\n", device, __func__);

	groupmgr = device->groupmgr;
	group = &device->group_3aa;

	ret = fimc_is_group_buffer_finish(groupmgr, group, index);
	if (ret)
		merr("fimc_is_group_buffer_finish is fail(%d)", device, ret);

	return ret;
}

const struct fimc_is_queue_ops fimc_is_ischain_3aa_ops = {
	.start_streaming	= fimc_is_ischain_3aa_start,
	.stop_streaming		= fimc_is_ischain_3aa_stop,
	.s_format		= fimc_is_ischain_3aa_s_format,
	.request_bufs		= fimc_is_ischain_3aa_reqbufs
};

int fimc_is_ischain_isp_open(struct fimc_is_device_ischain *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	int ret_err = 0;
	u32 group_id;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);
	FIMC_BUG(!vctx);
	FIMC_BUG(!GET_VIDEO(vctx));

	groupmgr = device->groupmgr;
	group = &device->group_isp;
	group_id = GROUP_ID_ISP0 + GET_IXS_ID(GET_VIDEO(vctx));

	ret = fimc_is_group_open(groupmgr,
		group,
		group_id,
		vctx);
	if (ret) {
		merr("fimc_is_group_open is fail(%d)", device, ret);
		goto err_group_open;
	}

	ret = fimc_is_ischain_open_wrap(device, false);
	if (ret) {
		merr("fimc_is_ischain_open_wrap is fail(%d)", device, ret);
		goto err_ischain_open;
	}

	atomic_inc(&device->group_open_cnt);

	return 0;

err_ischain_open:
	ret_err = fimc_is_group_close(groupmgr, group);
	if (ret_err)
		merr("fimc_is_group_close is fail(%d)", device, ret_err);
err_group_open:
	return ret;
}

int fimc_is_ischain_isp_close(struct fimc_is_device_ischain *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;
	struct fimc_is_queue *queue;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_isp;
	queue = GET_QUEUE(vctx);

	/* for mediaserver dead */
	if (test_bit(FIMC_IS_GROUP_START, &group->state)) {
		mgwarn("sudden group close", device, group);
		if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
			fimc_is_itf_sudden_stop_wrap(device, device->instance);
		set_bit(FIMC_IS_GROUP_REQUEST_FSTOP, &group->state);
		if (test_bit(FIMC_IS_HAL_DEBUG_SUDDEN_DEAD_DETECT, &sysfs_debug.hal_debug_mode)) {
			msleep(sysfs_debug.hal_debug_delay);
			panic("HAL sudden group close #1");
		}
	}

	if (group->head && test_bit(FIMC_IS_GROUP_START, &group->head->state)) {
		mgwarn("sudden group close", device, group);
		if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
			fimc_is_itf_sudden_stop_wrap(device, device->instance);
		set_bit(FIMC_IS_GROUP_REQUEST_FSTOP, &group->state);
		if (test_bit(FIMC_IS_HAL_DEBUG_SUDDEN_DEAD_DETECT, &sysfs_debug.hal_debug_mode)) {
			msleep(sysfs_debug.hal_debug_delay);
			panic("HAL sudden group close #2");
		}
	}

	ret = fimc_is_ischain_isp_stop(device, queue);
	if (ret)
		merr("fimc_is_ischain_isp_stop is fail", device);

	ret = fimc_is_group_close(groupmgr, group);
	if (ret)
		merr("fimc_is_group_close is fail", device);

	ret = fimc_is_ischain_close_wrap(device);
	if (ret)
		merr("fimc_is_ischain_close_wrap is fail(%d)", device, ret);

	atomic_dec(&device->group_open_cnt);

	return ret;
}

int fimc_is_ischain_isp_s_input(struct fimc_is_device_ischain *device,
	u32 stream_type,
	u32 module_id,
	u32 video_id,
	u32 input_type,
	u32 stream_leader)
{
	int ret = 0;
	struct fimc_is_group *group;
	struct fimc_is_groupmgr *groupmgr;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_isp;

	mdbgd_ischain("%s()\n", device, __func__);

	ret = fimc_is_group_init(groupmgr, group, input_type, video_id, stream_leader);
	if (ret) {
		merr("fimc_is_group_init is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_init_wrap(device, stream_type, module_id);
	if (ret) {
		merr("fimc_is_ischain_init_wrap is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ischain_isp_start(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_isp;

	ret = fimc_is_group_start(groupmgr, group);
	if (ret) {
		merr("fimc_is_group_start is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_start_wrap(device, group);
	if (ret) {
		merr("fimc_is_group_start is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ischain_isp_stop(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_isp;

	ret = fimc_is_group_stop(groupmgr, group);
	if (ret) {
		merr("fimc_is_group_stop is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_stop_wrap(device, group);
	if (ret) {
		merr("fimc_is_ischain_stop_wrap is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	mginfo("%s(%d):%d\n", device, group,  __func__, atomic_read(&group->scount), ret);
	return ret;
}

static int fimc_is_ischain_isp_reqbufs(void *qdevice,
	struct fimc_is_queue *queue, u32 count)
{
#ifdef ENABLE_IS_CORE
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	group = &device->group_isp;

	if (!count) {
		ret = fimc_is_itf_unmap(device, GROUP_ID(group->id));
		if (ret)
			merr("fimc_is_itf_unmap is fail(%d)", device, ret);
	}

	return ret;
#else
	return 0;
#endif
}

static int fimc_is_ischain_isp_s_format(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_subdev *leader;

	FIMC_BUG(!device);
	FIMC_BUG(!queue);

	leader = &device->group_isp.leader;

	leader->input.width = queue->framecfg.width;
	leader->input.height = queue->framecfg.height;

	leader->input.crop.x = 0;
	leader->input.crop.y = 0;
	leader->input.crop.w = leader->input.width;
	leader->input.crop.h = leader->input.height;

	return ret;
}

int fimc_is_ischain_isp_buffer_queue(struct fimc_is_device_ischain *device,
	struct fimc_is_queue *queue,
	u32 index)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);
	FIMC_BUG(!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state));

	mdbgs_ischain(4, "%s\n", device, __func__);

	groupmgr = device->groupmgr;
	group = &device->group_isp;

	ret = fimc_is_group_buffer_queue(groupmgr, group, queue, index);
	if (ret) {
		merr("fimc_is_group_buffer_queue is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

int fimc_is_ischain_isp_buffer_finish(struct fimc_is_device_ischain *device,
	u32 index)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	mdbgs_ischain(4, "%s\n", device, __func__);

	groupmgr = device->groupmgr;
	group = &device->group_isp;

	ret = fimc_is_group_buffer_finish(groupmgr, group, index);
	if (ret)
		merr("fimc_is_group_buffer_finish is fail(%d)", device, ret);

	return ret;
}

const struct fimc_is_queue_ops fimc_is_ischain_isp_ops = {
	.start_streaming	= fimc_is_ischain_isp_start,
	.stop_streaming		= fimc_is_ischain_isp_stop,
	.s_format		= fimc_is_ischain_isp_s_format,
	.request_bufs		= fimc_is_ischain_isp_reqbufs
};

int fimc_is_ischain_dis_open(struct fimc_is_device_ischain *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	int ret_err = 0;
	u32 group_id;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);
	FIMC_BUG(!vctx);
	FIMC_BUG(!GET_VIDEO(vctx));

	groupmgr = device->groupmgr;
	group = &device->group_dis;
	group_id = GROUP_ID_DIS0 + GET_DXS_ID(GET_VIDEO(vctx));

	ret = fimc_is_group_open(groupmgr,
		group,
		group_id,
		vctx);
	if (ret) {
		merr("fimc_is_group_open is fail(%d)", device, ret);
		goto err_group_open;
	}

	ret = fimc_is_ischain_open_wrap(device, false);
	if (ret) {
		merr("fimc_is_ischain_open_wrap is fail(%d)", device, ret);
		goto err_ischain_open;
	}

	atomic_inc(&device->group_open_cnt);

	return 0;

err_ischain_open:
	ret_err = fimc_is_group_close(groupmgr, group);
	if (ret_err)
		merr("fimc_is_group_close is fail(%d)", device, ret_err);
err_group_open:
	return ret;
}

int fimc_is_ischain_dis_close(struct fimc_is_device_ischain *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;
	struct fimc_is_queue *queue;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_dis;
	queue = GET_QUEUE(vctx);

	/* for mediaserver dead */
	if (test_bit(FIMC_IS_GROUP_START, &group->state)) {
		mgwarn("sudden group close", device, group);
		if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
			fimc_is_itf_sudden_stop_wrap(device, device->instance);
		set_bit(FIMC_IS_GROUP_REQUEST_FSTOP, &group->state);
		if (test_bit(FIMC_IS_HAL_DEBUG_SUDDEN_DEAD_DETECT, &sysfs_debug.hal_debug_mode)) {
			msleep(sysfs_debug.hal_debug_delay);
			panic("HAL sudden group close #1");
		}
	}

	if (group->head && test_bit(FIMC_IS_GROUP_START, &group->head->state)) {
		mgwarn("sudden group close", device, group);
		if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
			fimc_is_itf_sudden_stop_wrap(device, device->instance);
		set_bit(FIMC_IS_GROUP_REQUEST_FSTOP, &group->state);
		if (test_bit(FIMC_IS_HAL_DEBUG_SUDDEN_DEAD_DETECT, &sysfs_debug.hal_debug_mode)) {
			msleep(sysfs_debug.hal_debug_delay);
			panic("HAL sudden group close #2");
		}
	}

	ret = fimc_is_ischain_dis_stop(device, queue);
	if (ret)
		merr("fimc_is_ischain_dis_stop is fail", device);

	ret = fimc_is_group_close(groupmgr, group);
	if (ret)
		merr("fimc_is_group_close is fail", device);

	ret = fimc_is_ischain_close_wrap(device);
	if (ret)
		merr("fimc_is_ischain_close_wrap is fail(%d)", device, ret);

	atomic_dec(&device->group_open_cnt);

	return ret;
}

int fimc_is_ischain_dis_s_input(struct fimc_is_device_ischain *device,
	u32 stream_type,
	u32 module_id,
	u32 video_id,
	u32 input_type,
	u32 stream_leader)
{
	int ret = 0;
	struct fimc_is_group *group;
	struct fimc_is_groupmgr *groupmgr;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_dis;

	mdbgd_ischain("%s()\n", device, __func__);

	ret = fimc_is_group_init(groupmgr, group, input_type, video_id, stream_leader);
	if (ret) {
		merr("fimc_is_group_init is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_init_wrap(device, stream_type, module_id);
	if (ret) {
		merr("fimc_is_ischain_init_wrap is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ischain_dis_start(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_dis;

	ret = fimc_is_group_start(groupmgr, group);
	if (ret) {
		merr("fimc_is_group_start is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_start_wrap(device, group);
	if (ret) {
		merr("fimc_is_group_start is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ischain_dis_stop(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_dis;

	ret = fimc_is_group_stop(groupmgr, group);
	if (ret) {
		merr("fimc_is_group_stop is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_stop_wrap(device, group);
	if (ret) {
		merr("fimc_is_ischain_stop_wrap is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	mginfo("%s(%d):%d\n", device, group,  __func__, atomic_read(&group->scount), ret);
	return ret;
}

static int fimc_is_ischain_dis_reqbufs(void *qdevice,
	struct fimc_is_queue *queue, u32 count)
{
#ifdef ENABLE_IS_CORE
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	group = &device->group_dis;

	if (!count) {
		ret = fimc_is_itf_unmap(device, GROUP_ID(group->id));
		if (ret)
			merr("fimc_is_itf_unmap is fail(%d)", device, ret);
	}

	return ret;
#else
	return 0;
#endif
}

static int fimc_is_ischain_dis_s_format(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_subdev *leader;

	FIMC_BUG(!device);
	FIMC_BUG(!queue);

	leader = &device->group_dis.leader;

	leader->input.width = queue->framecfg.width;
	leader->input.height = queue->framecfg.height;

	leader->input.crop.x = 0;
	leader->input.crop.y = 0;
	leader->input.crop.w = leader->input.width;
	leader->input.crop.h = leader->input.height;

	return ret;
}

int fimc_is_ischain_dis_buffer_queue(struct fimc_is_device_ischain *device,
	struct fimc_is_queue *queue,
	u32 index)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);
	FIMC_BUG(!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state));

	mdbgs_ischain(4, "%s\n", device, __func__);

	groupmgr = device->groupmgr;
	group = &device->group_dis;

	ret = fimc_is_group_buffer_queue(groupmgr, group, queue, index);
	if (ret) {
		merr("fimc_is_group_buffer_queue is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

int fimc_is_ischain_dis_buffer_finish(struct fimc_is_device_ischain *device,
	u32 index)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	mdbgs_ischain(4, "%s\n", device, __func__);

	groupmgr = device->groupmgr;
	group = &device->group_dis;

	ret = fimc_is_group_buffer_finish(groupmgr, group, index);
	if (ret)
		merr("fimc_is_group_buffer_finish is fail(%d)", device, ret);

	return ret;
}

const struct fimc_is_queue_ops fimc_is_ischain_dis_ops = {
	.start_streaming	= fimc_is_ischain_dis_start,
	.stop_streaming		= fimc_is_ischain_dis_stop,
	.s_format		= fimc_is_ischain_dis_s_format,
	.request_bufs		= fimc_is_ischain_dis_reqbufs
};

int fimc_is_ischain_dcp_open(struct fimc_is_device_ischain *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	int ret_err = 0;
	u32 group_id;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);
	FIMC_BUG(!vctx);
	FIMC_BUG(!GET_VIDEO(vctx));

	groupmgr = device->groupmgr;
	group = &device->group_dcp;
	group_id = GROUP_ID_DCP + GET_DCPXS_ID(GET_VIDEO(vctx));

	ret = fimc_is_group_open(groupmgr,
		group,
		group_id,
		vctx);
	if (ret) {
		merr("fimc_is_group_open is fail(%d)", device, ret);
		goto err_group_open;
	}

	ret = fimc_is_ischain_open_wrap(device, false);
	if (ret) {
		merr("fimc_is_ischain_open_wrap is fail(%d)", device, ret);
		goto err_ischain_open;
	}

	atomic_inc(&device->group_open_cnt);

	return 0;

err_ischain_open:
	ret_err = fimc_is_group_close(groupmgr, group);
	if (ret_err)
		merr("fimc_is_group_close is fail(%d)", device, ret_err);
err_group_open:
	return ret;
}

int fimc_is_ischain_dcp_close(struct fimc_is_device_ischain *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;
	struct fimc_is_queue *queue;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_dcp;
	queue = GET_QUEUE(vctx);

	/* for mediaserver dead */
	if (test_bit(FIMC_IS_GROUP_START, &group->state)) {
		mgwarn("sudden group close", device, group);
		if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
			fimc_is_itf_sudden_stop_wrap(device, device->instance);
		set_bit(FIMC_IS_GROUP_REQUEST_FSTOP, &group->state);
		if (test_bit(FIMC_IS_HAL_DEBUG_SUDDEN_DEAD_DETECT, &sysfs_debug.hal_debug_mode)) {
			msleep(sysfs_debug.hal_debug_delay);
			panic("HAL sudden group close #1");
		}
	}

	if (group->head && test_bit(FIMC_IS_GROUP_START, &group->head->state)) {
		mgwarn("sudden group close", device, group);
		if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
			fimc_is_itf_sudden_stop_wrap(device, device->instance);
		set_bit(FIMC_IS_GROUP_REQUEST_FSTOP, &group->state);
		if (test_bit(FIMC_IS_HAL_DEBUG_SUDDEN_DEAD_DETECT, &sysfs_debug.hal_debug_mode)) {
			msleep(sysfs_debug.hal_debug_delay);
			panic("HAL sudden group close #2");
		}
	}

	ret = fimc_is_ischain_dcp_stop(device, queue);
	if (ret)
		merr("fimc_is_ischain_dcp_stop is fail", device);

	ret = fimc_is_group_close(groupmgr, group);
	if (ret)
		merr("fimc_is_group_close is fail", device);

	ret = fimc_is_ischain_close_wrap(device);
	if (ret)
		merr("fimc_is_ischain_close_wrap is fail(%d)", device, ret);

	atomic_dec(&device->group_open_cnt);

	return ret;
}

int fimc_is_ischain_dcp_s_input(struct fimc_is_device_ischain *device,
	u32 stream_type,
	u32 module_id,
	u32 video_id,
	u32 input_type,
	u32 stream_leader)
{
	int ret = 0;
	struct fimc_is_group *group;
	struct fimc_is_groupmgr *groupmgr;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_dcp;

	mdbgd_ischain("%s()\n", device, __func__);

	ret = fimc_is_group_init(groupmgr, group, input_type, video_id, stream_leader);
	if (ret) {
		merr("fimc_is_group_init is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_init_wrap(device, stream_type, module_id);
	if (ret) {
		merr("fimc_is_ischain_init_wrap is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ischain_dcp_start(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_dcp;

	ret = fimc_is_group_start(groupmgr, group);
	if (ret) {
		merr("fimc_is_group_start is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_start_wrap(device, group);
	if (ret) {
		merr("fimc_is_group_start is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ischain_dcp_stop(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_dcp;

	ret = fimc_is_group_stop(groupmgr, group);
	if (ret) {
		merr("fimc_is_group_stop is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_stop_wrap(device, group);
	if (ret) {
		merr("fimc_is_ischain_stop_wrap is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	mginfo("%s(%d):%d\n", device, group,  __func__, atomic_read(&group->scount), ret);
	return ret;
}

static int fimc_is_ischain_dcp_reqbufs(void *qdevice,
	struct fimc_is_queue *queue, u32 count)
{
#ifdef ENABLE_IS_CORE
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	group = &device->group_dcp;

	if (!count) {
		ret = fimc_is_itf_unmap(device, GROUP_ID(group->id));
		if (ret)
			merr("fimc_is_itf_unmap is fail(%d)", device, ret);
	}

	return ret;
#else
	return 0;
#endif
}

static int fimc_is_ischain_dcp_s_format(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_subdev *leader;

	FIMC_BUG(!device);
	FIMC_BUG(!queue);

	leader = &device->group_dcp.leader;

	leader->input.width = queue->framecfg.width;
	leader->input.height = queue->framecfg.height;

	leader->input.crop.x = 0;
	leader->input.crop.y = 0;
	leader->input.crop.w = leader->input.width;
	leader->input.crop.h = leader->input.height;

	return ret;
}

int fimc_is_ischain_dcp_buffer_queue(struct fimc_is_device_ischain *device,
	struct fimc_is_queue *queue,
	u32 index)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);
	FIMC_BUG(!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state));

	mdbgs_ischain(4, "%s\n", device, __func__);

	groupmgr = device->groupmgr;
	group = &device->group_dcp;

	ret = fimc_is_group_buffer_queue(groupmgr, group, queue, index);
	if (ret) {
		merr("fimc_is_group_buffer_queue is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

int fimc_is_ischain_dcp_buffer_finish(struct fimc_is_device_ischain *device,
	u32 index)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	mdbgs_ischain(4, "%s\n", device, __func__);

	groupmgr = device->groupmgr;
	group = &device->group_dcp;

	ret = fimc_is_group_buffer_finish(groupmgr, group, index);
	if (ret)
		merr("fimc_is_group_buffer_finish is fail(%d)", device, ret);

	return ret;
}

const struct fimc_is_queue_ops fimc_is_ischain_dcp_ops = {
	.start_streaming	= fimc_is_ischain_dcp_start,
	.stop_streaming		= fimc_is_ischain_dcp_stop,
	.s_format		= fimc_is_ischain_dcp_s_format,
	.request_bufs		= fimc_is_ischain_dcp_reqbufs
};

int fimc_is_ischain_mcs_open(struct fimc_is_device_ischain *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	int ret_err = 0;
	u32 group_id;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);
	FIMC_BUG(!vctx);
	FIMC_BUG(!GET_VIDEO(vctx));

	groupmgr = device->groupmgr;
	group = &device->group_mcs;
	group_id = GROUP_ID_MCS0 + GET_MXS_ID(GET_VIDEO(vctx));

	ret = fimc_is_group_open(groupmgr,
		group,
		group_id,
		vctx);
	if (ret) {
		merr("fimc_is_group_open is fail(%d)", device, ret);
		goto err_group_open;
	}

	ret = fimc_is_ischain_open_wrap(device, false);
	if (ret) {
		merr("fimc_is_ischain_open_wrap is fail(%d)", device, ret);
		goto err_ischain_open;
	}

	atomic_inc(&device->group_open_cnt);

	return 0;

err_ischain_open:
	ret_err = fimc_is_group_close(groupmgr, group);
	if (ret_err)
		merr("fimc_is_group_close is fail(%d)", device, ret_err);
err_group_open:
	return ret;
}

int fimc_is_ischain_mcs_close(struct fimc_is_device_ischain *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;
	struct fimc_is_queue *queue;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_mcs;
	queue = GET_QUEUE(vctx);

	/* for mediaserver dead */
	if (test_bit(FIMC_IS_GROUP_START, &group->state)) {
		mgwarn("sudden group close", device, group);
		if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
			fimc_is_itf_sudden_stop_wrap(device, device->instance);
		set_bit(FIMC_IS_GROUP_REQUEST_FSTOP, &group->state);
		if (test_bit(FIMC_IS_HAL_DEBUG_SUDDEN_DEAD_DETECT, &sysfs_debug.hal_debug_mode)) {
			msleep(sysfs_debug.hal_debug_delay);
			panic("HAL sudden group close #1");
		}
	}

	if (group->head && test_bit(FIMC_IS_GROUP_START, &group->head->state)) {
		mgwarn("sudden group close", device, group);
		if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
			fimc_is_itf_sudden_stop_wrap(device, device->instance);
		set_bit(FIMC_IS_GROUP_REQUEST_FSTOP, &group->state);
		if (test_bit(FIMC_IS_HAL_DEBUG_SUDDEN_DEAD_DETECT, &sysfs_debug.hal_debug_mode)) {
			msleep(sysfs_debug.hal_debug_delay);
			panic("HAL sudden group close #2");
		}
	}

	ret = fimc_is_ischain_mcs_stop(device, queue);
	if (ret)
		merr("fimc_is_ischain_mcs_stop is fail", device);

	ret = fimc_is_group_close(groupmgr, group);
	if (ret)
		merr("fimc_is_group_close is fail", device);

	ret = fimc_is_ischain_close_wrap(device);
	if (ret)
		merr("fimc_is_ischain_close_wrap is fail(%d)", device, ret);

	atomic_dec(&device->group_open_cnt);

	return ret;
}

int fimc_is_ischain_mcs_s_input(struct fimc_is_device_ischain *device,
	u32 stream_type,
	u32 module_id,
	u32 video_id,
	u32 otf_input,
	u32 stream_leader)
{
	int ret = 0;
	struct fimc_is_group *group;
	struct fimc_is_groupmgr *groupmgr;

	FIMC_BUG(!device);
	FIMC_BUG(!device->groupmgr);

	groupmgr = device->groupmgr;
	group = &device->group_mcs;

	mdbgd_ischain("%s()\n", device, __func__);

	ret = fimc_is_group_init(groupmgr, group, otf_input, video_id, stream_leader);
	if (ret) {
		merr("fimc_is_group_init is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_init_wrap(device, stream_type, module_id);
	if (ret) {
		merr("fimc_is_ischain_init_wrap is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ischain_mcs_start(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_mcs;

	ret = fimc_is_group_start(groupmgr, group);
	if (ret) {
		merr("fimc_is_group_start is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_start_wrap(device, group);
	if (ret) {
		merr("fimc_is_ischain_start_wrap is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ischain_mcs_stop(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_mcs;

	ret = fimc_is_group_stop(groupmgr, group);
	if (ret) {
		merr("fimc_is_group_stop is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_stop_wrap(device, group);
	if (ret) {
		merr("fimc_is_ischain_stop_wrap is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	mginfo("%s(%d):%d\n", device, group,  __func__, atomic_read(&group->scount), ret);
	return ret;
}

static int fimc_is_ischain_mcs_reqbufs(void *qdevice,
	struct fimc_is_queue *queue, u32 count)
{
#ifdef ENABLE_IS_CORE
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	group = &device->group_mcs;

	if (!count) {
		ret = fimc_is_itf_unmap(device, GROUP_ID(group->id));
		if (ret)
			merr("fimc_is_itf_unmap is fail(%d)", device, ret);
	}

	return ret;
#else
	return 0;
#endif
}

static int fimc_is_ischain_mcs_s_format(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_subdev *leader;

	FIMC_BUG(!device);
	FIMC_BUG(!queue);

	leader = &device->group_mcs.leader;

	leader->input.width = queue->framecfg.width;
	leader->input.height = queue->framecfg.height;

	leader->input.crop.x = 0;
	leader->input.crop.y = 0;
	leader->input.crop.w = leader->input.width;
	leader->input.crop.h = leader->input.height;

	return ret;
}

int fimc_is_ischain_mcs_buffer_queue(struct fimc_is_device_ischain *device,
	struct fimc_is_queue *queue,
	u32 index)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);
	FIMC_BUG(!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state));

	mdbgs_ischain(4, "%s\n", device, __func__);

	groupmgr = device->groupmgr;
	group = &device->group_mcs;

	ret = fimc_is_group_buffer_queue(groupmgr, group, queue, index);
	if (ret)
		merr("fimc_is_group_buffer_queue is fail(%d)", device, ret);

	return ret;
}

int fimc_is_ischain_mcs_buffer_finish(struct fimc_is_device_ischain *device,
	u32 index)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	mdbgs_ischain(4, "%s\n", device, __func__);

	groupmgr = device->groupmgr;
	group = &device->group_mcs;

	ret = fimc_is_group_buffer_finish(groupmgr, group, index);
	if (ret)
		merr("fimc_is_group_buffer_finish is fail(%d)", device, ret);

	return ret;
}

const struct fimc_is_queue_ops fimc_is_ischain_mcs_ops = {
	.start_streaming	= fimc_is_ischain_mcs_start,
	.stop_streaming		= fimc_is_ischain_mcs_stop,
	.s_format		= fimc_is_ischain_mcs_s_format,
	.request_bufs		= fimc_is_ischain_mcs_reqbufs
};

int fimc_is_ischain_vra_open(struct fimc_is_device_ischain *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	int ret_err = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_vra;

	ret = fimc_is_group_open(groupmgr,
		group,
		GROUP_ID_VRA0,
		vctx);
	if (ret) {
		merr("fimc_is_group_open is fail(%d)", device, ret);
		goto err_group_open;
	}

	ret = fimc_is_ischain_open_wrap(device, false);
	if (ret) {
		merr("fimc_is_ischain_open_wrap is fail(%d)", device, ret);
		goto err_ischain_open;
	}

	atomic_inc(&device->group_open_cnt);

	return 0;

err_ischain_open:
	ret_err = fimc_is_group_close(groupmgr, group);
	if (ret_err)
		merr("fimc_is_group_close is fail(%d)", device, ret_err);
err_group_open:
	return ret;
}

int fimc_is_ischain_vra_close(struct fimc_is_device_ischain *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;
	struct fimc_is_queue *queue;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_vra;
	queue = GET_QUEUE(vctx);

	/* for mediaserver dead */
	if (test_bit(FIMC_IS_GROUP_START, &group->state)) {
		mgwarn("sudden group close", device, group);
		if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
			fimc_is_itf_sudden_stop_wrap(device, device->instance);
		set_bit(FIMC_IS_GROUP_REQUEST_FSTOP, &group->state);
		if (test_bit(FIMC_IS_HAL_DEBUG_SUDDEN_DEAD_DETECT, &sysfs_debug.hal_debug_mode)) {
			msleep(sysfs_debug.hal_debug_delay);
			panic("HAL sudden group close #1");
		}
	}

	if (group->head && test_bit(FIMC_IS_GROUP_START, &group->head->state)) {
		mgwarn("sudden group close", device, group);
		if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
			fimc_is_itf_sudden_stop_wrap(device, device->instance);
		set_bit(FIMC_IS_GROUP_REQUEST_FSTOP, &group->state);
		if (test_bit(FIMC_IS_HAL_DEBUG_SUDDEN_DEAD_DETECT, &sysfs_debug.hal_debug_mode)) {
			msleep(sysfs_debug.hal_debug_delay);
			panic("HAL sudden group close #2");
		}
	}

	ret = fimc_is_ischain_vra_stop(device, queue);
	if (ret)
		merr("fimc_is_ischain_vra_stop is fail", device);

	ret = fimc_is_group_close(groupmgr, group);
	if (ret)
		merr("fimc_is_group_close is fail", device);

	ret = fimc_is_ischain_close_wrap(device);
	if (ret)
		merr("fimc_is_ischain_close_wrap is fail(%d)", device, ret);

	atomic_dec(&device->group_open_cnt);

	return ret;
}

int fimc_is_ischain_vra_s_input(struct fimc_is_device_ischain *device,
	u32 stream_type,
	u32 module_id,
	u32 video_id,
	u32 otf_input,
	u32 stream_leader)
{
	int ret = 0;
	struct fimc_is_group *group;
	struct fimc_is_groupmgr *groupmgr;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_vra;

	mdbgd_ischain("%s()\n", device, __func__);

	ret = fimc_is_group_init(groupmgr, group, otf_input, video_id, stream_leader);
	if (ret) {
		merr("fimc_is_group_init is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_init_wrap(device, stream_type, module_id);
	if (ret) {
		merr("fimc_is_ischain_init_wrap is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ischain_vra_start(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_vra;

	ret = fimc_is_group_start(groupmgr, group);
	if (ret) {
		merr("fimc_is_group_start is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_start_wrap(device, group);
	if (ret) {
		merr("fimc_is_group_start is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ischain_vra_stop(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	groupmgr = device->groupmgr;
	group = &device->group_vra;

	ret = fimc_is_group_stop(groupmgr, group);
	if (ret) {
		merr("fimc_is_group_stop is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_stop_wrap(device, group);
	if (ret) {
		merr("fimc_is_ischain_stop_wrap is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	mginfo("%s(%d):%d\n", device, group,  __func__, atomic_read(&group->scount), ret);
	return ret;
}

static int fimc_is_ischain_vra_reqbufs(void *qdevice,
	struct fimc_is_queue *queue, u32 count)
{
#ifdef ENABLE_IS_CORE
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	group = &device->group_vra;

	if (!count) {
		ret = fimc_is_itf_unmap(device, GROUP_ID(group->id));
		if (ret)
			merr("fimc_is_itf_unmap is fail(%d)", device, ret);
	}

	return ret;
#else
	return 0;
#endif
}

static int fimc_is_ischain_vra_s_format(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_subdev *leader;

	FIMC_BUG(!device);
	FIMC_BUG(!queue);

	leader = &device->group_vra.leader;

	leader->input.width = queue->framecfg.width;
	leader->input.height = queue->framecfg.height;

	leader->input.crop.x = 0;
	leader->input.crop.y = 0;
	leader->input.crop.w = leader->input.width;
	leader->input.crop.h = leader->input.height;

	return ret;
}

int fimc_is_ischain_vra_buffer_queue(struct fimc_is_device_ischain *device,
	struct fimc_is_queue *queue,
	u32 index)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);
	FIMC_BUG(!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state));

	mdbgs_ischain(4, "%s\n", device, __func__);

	groupmgr = device->groupmgr;
	group = &device->group_vra;

	ret = fimc_is_group_buffer_queue(groupmgr, group, queue, index);
	if (ret) {
		merr("fimc_is_group_buffer_queue is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

int fimc_is_ischain_vra_buffer_finish(struct fimc_is_device_ischain *device,
	u32 index)
{
	int ret = 0;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;

	FIMC_BUG(!device);

	mdbgs_ischain(4, "%s\n", device, __func__);

	groupmgr = device->groupmgr;
	group = &device->group_vra;

	ret = fimc_is_group_buffer_finish(groupmgr, group, index);
	if (ret)
		merr("fimc_is_group_buffer_finish is fail(%d)", device, ret);

	return ret;
}

const struct fimc_is_queue_ops fimc_is_ischain_vra_ops = {
	.start_streaming	= fimc_is_ischain_vra_start,
	.stop_streaming		= fimc_is_ischain_vra_stop,
	.s_format		= fimc_is_ischain_vra_s_format,
	.request_bufs		= fimc_is_ischain_vra_reqbufs
};

static int fimc_is_ischain_paf_group_tag(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame,
	struct camera2_node *ldr_node)
{
	int ret = 0;
	struct fimc_is_group *group;

	group = &device->group_paf;

	ret = CALL_SOPS(&group->leader, tag, device, frame, ldr_node);
	if (ret) {
		merr("fimc_is_ischain_paf_tag is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ischain_3aa_group_tag(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame,
	struct camera2_node *ldr_node)
{
	int ret = 0;
	u32 capture_id;
	struct fimc_is_group *group;
	struct fimc_is_subdev *subdev;
	struct camera2_node_group *node_group;
	struct camera2_node *cap_node;

	group = &device->group_3aa;
	node_group = &frame->shot_ext->node_group;

	ret = CALL_SOPS(&group->leader, tag, device, frame, ldr_node);
	if (ret) {
		merr("fimc_is_ischain_3aa_tag is fail(%d)", device, ret);
		goto p_err;
	}

	for (capture_id = 0; capture_id < CAPTURE_NODE_MAX; ++capture_id) {
		cap_node = &node_group->capture[capture_id];
		subdev = NULL;

		switch (cap_node->vid) {
		case 0:
			break;
		case FIMC_IS_VIDEO_30C_NUM:
		case FIMC_IS_VIDEO_31C_NUM:
			subdev = group->subdev[ENTRY_3AC];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_3ac_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		case FIMC_IS_VIDEO_30P_NUM:
		case FIMC_IS_VIDEO_31P_NUM:
			subdev = group->subdev[ENTRY_3AP];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_3ap_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		case FIMC_IS_VIDEO_30F_NUM:
		case FIMC_IS_VIDEO_31F_NUM:
			subdev = group->subdev[ENTRY_3AF];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_3af_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		case FIMC_IS_VIDEO_30G_NUM:
		case FIMC_IS_VIDEO_31G_NUM:
			subdev = group->subdev[ENTRY_3AG];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_3ag_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		case FIMC_IS_VIDEO_ME0C_NUM:
		case FIMC_IS_VIDEO_ME1C_NUM:
			subdev = group->subdev[ENTRY_MEXC];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_mexc_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		default:
			break;
		}
	}

p_err:
	return ret;
}

static int fimc_is_ischain_isp_group_tag(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame,
	struct camera2_node *ldr_node)
{
	int ret = 0;
	u32 capture_id;
	struct fimc_is_group *group;
	struct fimc_is_subdev *subdev, *drc, *vra;
	struct camera2_node_group *node_group;
	struct camera2_node *cap_node;

	group = &device->group_isp;
	drc = group->subdev[ENTRY_DRC];
	vra = group->subdev[ENTRY_VRA];
	node_group = &frame->shot_ext->node_group;

#ifdef ENABLE_VRA
	if (vra) {
		if (frame->shot_ext->fd_bypass) {
			if (test_bit(FIMC_IS_SUBDEV_RUN, &vra->state)) {
				ret = fimc_is_ischain_vra_bypass(device, frame, true);
				if (ret) {
					merr("fd_bypass(1) is fail", device);
					goto p_err;
				}
			}
		} else {
			if (!test_bit(FIMC_IS_SUBDEV_RUN, &vra->state)) {
				ret = fimc_is_ischain_vra_bypass(device, frame, false);
				if (ret) {
					merr("fd_bypass(0) is fail", device);
					goto p_err;
				}
			}
		}
	}
#endif

#ifdef ENABLE_DRC
	if (drc) {
		if (frame->shot_ext->drc_bypass) {
			if (test_bit(FIMC_IS_SUBDEV_RUN, &drc->state)) {
				ret = fimc_is_ischain_drc_bypass(device, frame, true);
				if (ret) {
					err("fimc_is_ischain_drc_bypass(1) is fail");
					goto p_err;
				}
			}
		} else {
			if (!test_bit(FIMC_IS_SUBDEV_RUN, &drc->state)) {
				ret = fimc_is_ischain_drc_bypass(device, frame, false);
				if (ret) {
					err("fimc_is_ischain_drc_bypass(0) is fail");
					goto p_err;
				}
			}
		}
	}
#endif

	ret = CALL_SOPS(&group->leader, tag, device, frame, ldr_node);
	if (ret) {
		merr("fimc_is_ischain_isp_tag is fail(%d)", device, ret);
		goto p_err;
	}

	for (capture_id = 0; capture_id < CAPTURE_NODE_MAX; ++capture_id) {
		cap_node = &node_group->capture[capture_id];
		subdev = NULL;

		switch (cap_node->vid) {
		case 0:
			break;
		case FIMC_IS_VIDEO_I0C_NUM:
		case FIMC_IS_VIDEO_I1C_NUM:
			subdev = group->subdev[ENTRY_IXC];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_ixc_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		case FIMC_IS_VIDEO_I0P_NUM:
		case FIMC_IS_VIDEO_I1P_NUM:
			subdev = group->subdev[ENTRY_IXP];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_ixp_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		case FIMC_IS_VIDEO_ME0C_NUM:
		case FIMC_IS_VIDEO_ME1C_NUM:
			subdev = group->subdev[ENTRY_MEXC];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_mexc_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		default:
			break;
		}
	}

p_err:
	return ret;
}

static int fimc_is_ischain_dis_group_tag(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame,
	struct camera2_node *ldr_node)
{
	int ret = 0;
	u32 capture_id;
	struct fimc_is_group *group;
	struct fimc_is_subdev *subdev, *dis, *dnr, *vra;
	struct camera2_node_group *node_group;
	struct camera2_node *cap_node;

	group = &device->group_dis;
	dis = &group->leader;
	dnr = group->subdev[ENTRY_DNR];
	vra = group->subdev[ENTRY_VRA];
	node_group = &frame->shot_ext->node_group;

#ifdef ENABLE_VRA
	if (vra) {
		if (frame->shot_ext->fd_bypass) {
			if (test_bit(FIMC_IS_SUBDEV_RUN, &vra->state)) {
				ret = fimc_is_ischain_vra_bypass(device, frame, true);
				if (ret) {
					merr("fd_bypass(1) is fail", device);
					goto p_err;
				}
			}
		} else {
			if (!test_bit(FIMC_IS_SUBDEV_RUN, &vra->state)) {
				ret = fimc_is_ischain_vra_bypass(device, frame, false);
				if (ret) {
					merr("fd_bypass(0) is fail", device);
					goto p_err;
				}
			}
		}
	}
#endif

#ifdef ENABLE_DIS
	if (dis) {
		if (frame->shot_ext->dis_bypass) {
			if (test_bit(FIMC_IS_SUBDEV_RUN, &dis->state)) {
				ret = fimc_is_ischain_dis_bypass(device, frame, true);
				if (ret) {
					merr("dis_bypass(1) is fail", device);
					goto p_err;
				}
			}
		} else {
			if (!test_bit(FIMC_IS_SUBDEV_RUN, &dis->state)) {
				ret = fimc_is_ischain_dis_bypass(device, frame, false);
				if (ret) {
					merr("dis_bypass(0) is fail", device);
					goto p_err;
				}
			}
		}
	}
#endif

#ifdef ENABLE_DNR_IN_TPU
	if (dnr) {
		if (frame->shot_ext->dnr_bypass) {
			if (test_bit(FIMC_IS_SUBDEV_RUN, &dnr->state)) {
				ret = fimc_is_ischain_dnr_bypass(device, frame, true);
				if (ret) {
					merr("dnr_bypass(1) is fail", device);
					goto p_err;
				}
			}
		} else {
			if (!test_bit(FIMC_IS_SUBDEV_RUN, &dnr->state)) {
				ret = fimc_is_ischain_dnr_bypass(device, frame, false);
				if (ret) {
					merr("dnr_bypass(0) is fail", device);
					goto p_err;
				}
			}
		}
	}
#endif

	ret = CALL_SOPS(&group->leader, tag, device, frame, ldr_node);
	if (ret) {
		merr("fimc_is_ischain_dis_tag is fail(%d)", device, ret);
		goto p_err;
	}

	for (capture_id = 0; capture_id < CAPTURE_NODE_MAX; ++capture_id) {
		cap_node = &node_group->capture[capture_id];
		subdev = NULL;

		switch (cap_node->vid) {
		case 0:
			break;
		case FIMC_IS_VIDEO_D0C_NUM:
		case FIMC_IS_VIDEO_D1C_NUM:
			subdev = group->subdev[ENTRY_DXC];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_dxc_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		default:
			break;
		}
	}

p_err:
	return ret;
}

static int fimc_is_ischain_dcp_group_tag(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame,
	struct camera2_node *ldr_node)
{
	int ret = 0;
	u32 capture_id;
	struct fimc_is_group *group;
	struct fimc_is_subdev *subdev, *dcp;
	struct camera2_node_group *node_group;
	struct camera2_node *cap_node;

	group = &device->group_dcp;
	dcp = &group->leader;
	node_group = &frame->shot_ext->node_group;

	ret = CALL_SOPS(&group->leader, tag, device, frame, ldr_node);
	if (ret) {
		merr("fimc_is_ischain_dcp_tag is fail(%d)", device, ret);
		goto p_err;
	}

	for (capture_id = 0; capture_id < CAPTURE_NODE_MAX; ++capture_id) {
		cap_node = &node_group->capture[capture_id];
		subdev = NULL;

		switch (cap_node->vid) {
		case 0:
			break;
		case FIMC_IS_VIDEO_DCP1S_NUM:
			subdev = group->subdev[ENTRY_DC1S];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_dc1s_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		case FIMC_IS_VIDEO_DCP0C_NUM:
			subdev = group->subdev[ENTRY_DC0C];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_dc0c_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		case FIMC_IS_VIDEO_DCP1C_NUM:
			subdev = group->subdev[ENTRY_DC1C];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_dc1c_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		case FIMC_IS_VIDEO_DCP2C_NUM:
			subdev = group->subdev[ENTRY_DC2C];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_dc2c_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		case FIMC_IS_VIDEO_DCP3C_NUM:
			subdev = group->subdev[ENTRY_DC3C];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_dc2c_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		case FIMC_IS_VIDEO_DCP4C_NUM:
			subdev = group->subdev[ENTRY_DC4C];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_dc2c_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		default:
			break;
		}
	}

p_err:
	return ret;
}

static int fimc_is_ischain_mcs_group_tag(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame,
	struct camera2_node *ldr_node)
{
	int ret = 0;
	u32 capture_id;
	struct fimc_is_group *group;
	struct fimc_is_group *head;
	struct fimc_is_subdev *subdev, *vra, *dnr;
	struct camera2_node_group *node_group;
	struct camera2_node *cap_node;

	group = &device->group_mcs;
	head = GET_HEAD_GROUP_IN_DEVICE(FIMC_IS_DEVICE_ISCHAIN, group);
	vra = group->subdev[ENTRY_VRA];
	dnr = group->subdev[ENTRY_DNR];
	node_group = &frame->shot_ext->node_group;

#ifdef ENABLE_VRA
	if (vra) {
		if (frame->shot_ext->fd_bypass) {
			if (test_bit(FIMC_IS_SUBDEV_RUN, &vra->state)) {
				ret = CALL_SOPS(&group->leader, bypass, device, frame, true);
				if (ret) {
					merr("mcs_bypass(1) is fail(%d)", device, ret);
					goto p_err;
				}
				ret = fimc_is_ischain_vra_bypass(device, frame, true);
				if (ret) {
					merr("fd_bypass(1) is fail", device);
					goto p_err;
				}
			}
		} else {
			if (!test_bit(FIMC_IS_SUBDEV_RUN, &vra->state)) {
				ret = CALL_SOPS(&group->leader, bypass, device, frame, false);
				if (ret) {
					merr("mcs_bypass(0) is fail(%d)", device, ret);
					goto p_err;
				}
				ret = fimc_is_ischain_vra_bypass(device, frame, false);
				if (ret) {
					merr("fd_bypass(0) is fail", device);
					goto p_err;
				}
			}
		}
	}
#endif

	/* At Full-OTF case(Head group is OTF),
	 * set next_noise_idx for next frame applying.
	 *
	 * At DMA input case, set cur_noise_idx for current frame appling.
	 */
	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &head->state)) {
		frame->noise_idx = device->next_noise_idx[frame->fcount % NI_BACKUP_MAX];
		/* clear back up NI value */
		device->next_noise_idx[frame->fcount % NI_BACKUP_MAX] = 0xFFFFFFFF;
	} else {
		frame->noise_idx = device->cur_noise_idx[frame->fcount % NI_BACKUP_MAX];
		/* clear back up NI value */
		device->cur_noise_idx[frame->fcount % NI_BACKUP_MAX] = 0xFFFFFFFF;
	}

#ifdef ENABLE_DNR_IN_MCSC
	if (dnr) {
		if ((frame->shot_ext->dnr_bypass) || (device->hardware->hw_fro_en)) {
			if (test_bit(FIMC_IS_SUBDEV_RUN, &dnr->state)) {
				ret = fimc_is_ischain_dnr_bypass(device, frame, true);
				if (ret) {
					merr("dnr_bypass(1) is fail", device);
					goto p_err;
				}
			}
		} else {
			if (!test_bit(FIMC_IS_SUBDEV_RUN, &dnr->state)) {
				ret = fimc_is_ischain_dnr_bypass(device, frame, false);
				if (ret) {
					merr("dnr_bypass(0) is fail", device);
					goto p_err;
				}
			}
		}
	}
#endif

	ret = CALL_SOPS(&group->leader, tag, device, frame, ldr_node);
	if (ret) {
		merr("fimc_is_ischain_mcs_tag is fail(%d)", device, ret);
		goto p_err;
	}

	for (capture_id = 0; capture_id < CAPTURE_NODE_MAX; ++capture_id) {
		cap_node = &node_group->capture[capture_id];
		subdev = NULL;

		switch (cap_node->vid) {
		case 0:
			break;
		case FIMC_IS_VIDEO_M0P_NUM:
			subdev = group->subdev[ENTRY_M0P];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_mxp_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		case FIMC_IS_VIDEO_M1P_NUM:
			subdev = group->subdev[ENTRY_M1P];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_mxp_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		case FIMC_IS_VIDEO_M2P_NUM:
			subdev = group->subdev[ENTRY_M2P];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_mxp_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		case FIMC_IS_VIDEO_M3P_NUM:
			subdev = group->subdev[ENTRY_M3P];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_mxp_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		case FIMC_IS_VIDEO_M4P_NUM:
			subdev = group->subdev[ENTRY_M4P];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_mxp_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		case FIMC_IS_VIDEO_M5P_NUM:
			subdev = group->subdev[ENTRY_M5P];
			if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				ret = CALL_SOPS(subdev, tag, device, frame, cap_node);
				if (ret) {
					merr("fimc_is_ischain_mxp_tag is fail(%d)", device, ret);
					goto p_err;
				}
			}
			break;
		default:
			break;
		}
	}

p_err:
	return ret;
}

static int fimc_is_ischain_vra_group_tag(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame,
	struct camera2_node *ldr_node)
{
	int ret = 0;
	struct fimc_is_group *group;
	struct fimc_is_subdev *vra;

	group = &device->group_vra;
	vra = &group->leader;

#ifdef ENABLE_VRA
	if (vra) {
		if (frame->shot_ext->fd_bypass) {
			if (test_bit(FIMC_IS_SUBDEV_RUN, &vra->state)) {
				ret = fimc_is_ischain_vra_bypass(device, frame, true);
				if (ret) {
					merr("fd_bypass(1) is fail", device);
					goto p_err;
				}
			}
		} else {
			if (!test_bit(FIMC_IS_SUBDEV_RUN, &vra->state)) {
				ret = fimc_is_ischain_vra_bypass(device, frame, false);
				if (ret) {
					merr("fd_bypass(0) is fail", device);
					goto p_err;
				}
			}
		}
	}
#endif

	ret = CALL_SOPS(&group->leader, tag, device, frame, ldr_node);
	 if (ret) {
			 merr("fimc_is_ischain_vra_tag fail(%d)", device, ret);
			 goto p_err;
	 }

p_err:
	return ret;
}

static void fimc_is_ischain_update_shot(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame)
{
#ifdef ENABLE_ULTRA_FAST_SHOT
	if (device->fastctlmgr.fast_capture_count) {
		device->fastctlmgr.fast_capture_count--;
		frame->shot->ctl.aa.captureIntent = AA_CAPTURE_INTENT_PREVIEW;
		mrinfo("captureIntent update\n", device, frame);
	}
#endif
}

static int fimc_is_ischain_paf_shot(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *check_frame)
{
	int ret = 0;
	unsigned long flags;
	struct fimc_is_resourcemgr *resourcemgr;
	struct fimc_is_group *group, *child, *vra;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	struct camera2_node_group *node_group;
	struct camera2_node ldr_node = {0, };
	u32 setfile_save = 0;

	FIMC_BUG(!device);
	FIMC_BUG(!check_frame);

	mdbgs_ischain(4, "%s()\n", device, __func__);

	resourcemgr = device->resourcemgr;
	group = &device->group_paf;
	frame = NULL;

	framemgr = GET_HEAD_GROUP_FRAMEMGR(group);
	if (!framemgr) {
		merr("framemgr is NULL", device);
		ret = -EINVAL;
		goto framemgr_err;
	}

	frame = peek_frame(framemgr, FS_REQUEST);

	if (unlikely(!frame)) {
		merr("frame is NULL", device);
		ret = -EINVAL;
		goto frame_err;
	}

	if (unlikely(frame != check_frame)) {
		merr("frame checking is fail(%p != %p)", device, frame, check_frame);
		ret = -EINVAL;
		goto p_err;
	}

	if (unlikely(!frame->shot)) {
		merr("frame->shot is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

#ifdef ENABLE_IS_CORE
	if (unlikely(!test_bit(FRAME_MEM_MAPPED, &frame->mem_state))) {
		fimc_is_itf_map(device, GROUP_ID(group->id), frame->dvaddr_shot, frame->shot_size);
		set_bit(FRAME_MEM_MAPPED, &frame->mem_state);
	}
#endif

	frame->shot->ctl.vendor_entry.lowIndexParam = 0;
	frame->shot->ctl.vendor_entry.highIndexParam = 0;
	frame->shot->dm.vendor_entry.lowIndexParam = 0;
	frame->shot->dm.vendor_entry.highIndexParam = 0;
	node_group = &frame->shot_ext->node_group;

	PROGRAM_COUNT(8);

	if ((frame->shot_ext->setfile != device->setfile) &&
		(group->id == get_ischain_leader_group(device)->id)) {
			setfile_save = device->setfile;
			device->setfile = frame->shot_ext->setfile;

		mgrinfo(" setfile change at shot(%d -> %d)\n", device, group, frame,
			setfile_save, device->setfile & FIMC_IS_SETFILE_MASK);

		if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state)) {
			ret = fimc_is_ischain_chg_setfile(device);
			if (ret) {
				merr("fimc_is_ischain_chg_setfile is fail", device);
				device->setfile = setfile_save;
				goto p_err;
			}
		}
	}

	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state)) {
		enum aa_capture_intent captureIntent;
		captureIntent = group->intent_ctl.captureIntent;

		if (captureIntent != AA_CAPTURE_INTENT_CUSTOM) {
			frame->shot->ctl.aa.captureIntent = captureIntent;
			frame->shot->ctl.aa.vendor_captureCount = group->intent_ctl.vendor_captureCount;
			frame->shot->ctl.aa.vendor_captureExposureTime = group->intent_ctl.vendor_captureExposureTime;
			frame->shot->ctl.aa.vendor_captureEV = group->intent_ctl.vendor_captureEV;
			if (group->remainIntentCount > 0) {
				group->remainIntentCount--;
			} else {
				group->intent_ctl.captureIntent = AA_CAPTURE_INTENT_CUSTOM;
				group->intent_ctl.vendor_captureCount = 0;
				group->intent_ctl.vendor_captureExposureTime = 0;
				group->intent_ctl.vendor_captureEV = 0;
			}
			minfo("frame count(%d), intent(%d), count(%d), EV(%d), captureExposureTime(%d) remainIntentCount(%d)\n",
				device, frame->fcount,
				frame->shot->ctl.aa.captureIntent, frame->shot->ctl.aa.vendor_captureCount,
				frame->shot->ctl.aa.vendor_captureEV, frame->shot->ctl.aa.vendor_captureExposureTime,
				group->remainIntentCount);
		}

		/*
		 * Adjust the shot timeout value based on sensor exposure time control.
		 * Exposure Time >= (FIMC_IS_SHOT_TIMEOUT - 1sec): Increase shot timeout value.
		 */
		if (frame->shot->ctl.aa.vendor_captureExposureTime >= ((FIMC_IS_SHOT_TIMEOUT * 1000) - 1000000)) {
			resourcemgr->shot_timeout = (frame->shot->ctl.aa.vendor_captureExposureTime / 1000) + FIMC_IS_SHOT_TIMEOUT;
			resourcemgr->shot_timeout_tick = KEEP_FRAME_TICK_DEFAULT;
		} else if (resourcemgr->shot_timeout_tick > 0) {
			resourcemgr->shot_timeout_tick--;
		} else {
			resourcemgr->shot_timeout = FIMC_IS_SHOT_TIMEOUT;
		}
	}

	/* fd information copy */
#if !defined(ENABLE_SHARED_METADATA) && !defined(FAST_FDAE)
	memcpy(&frame->shot->uctl.fdUd, &device->fdUd, sizeof(struct camera2_fd_uctl));
#endif

	PROGRAM_COUNT(9);

	if (test_bit(FIMC_IS_SUBDEV_PARAM_ERR, &group->head->leader.state))
		set_bit(FIMC_IS_SUBDEV_FORCE_SET, &group->head->leader.state);
	else
		clear_bit(FIMC_IS_SUBDEV_FORCE_SET, &group->head->leader.state);

	child = group;
	while (child) {
		switch (child->slot) {
		case GROUP_SLOT_PAF:
			TRANS_CROP(ldr_node.input.cropRegion,
				node_group->leader.input.cropRegion);

			ret = fimc_is_ischain_paf_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_paf_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		case GROUP_SLOT_3AA:
			TRANS_CROP(ldr_node.input.cropRegion,
				node_group->leader.input.cropRegion);
			if (child->junction->cid < CAPTURE_NODE_MAX) {
				TRANS_CROP(ldr_node.output.cropRegion,
					node_group->capture[child->junction->cid].output.cropRegion);
			} else {
				mgerr("capture id(%d) is invalid", group, group, child->junction->cid);
			}

			ret = fimc_is_ischain_3aa_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_3aa_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		case GROUP_SLOT_ISP:
			TRANS_CROP(ldr_node.input.cropRegion,
				ldr_node.output.cropRegion);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_isp_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_isp_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		case GROUP_SLOT_DIS:
			TRANS_CROP(ldr_node.input.cropRegion,
				ldr_node.output.cropRegion);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_dis_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_dis_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		case GROUP_SLOT_MCS:
			TRANS_CROP(ldr_node.input.cropRegion,
				ldr_node.output.cropRegion);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_mcs_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_mcs_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		case GROUP_SLOT_VRA:
			vra = &device->group_vra;
			TRANS_CROP(ldr_node.input.cropRegion,
				(u32 *)&vra->prev->junction->output.crop);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_vra_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_vra_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		default:
			merr("group slot is invalid(%d)", device, child->slot);
			BUG();
		}

		child = child->child;
	}

	PROGRAM_COUNT(10);

p_err:
	fimc_is_ischain_update_shot(device, frame);

	if (ret) {
		mgrerr(" SKIP(%d) : %d\n", device, group, check_frame, check_frame->index, ret);
	} else {
		set_bit(group->leader.id, &frame->out_flag);
		framemgr_e_barrier_irqs(framemgr, FMGR_IDX_25, flags);
		trans_frame(framemgr, frame, FS_PROCESS);
		framemgr_x_barrier_irqr(framemgr, FMGR_IDX_25, flags);
	}

frame_err:
framemgr_err:
	return ret;
}

static int fimc_is_ischain_3aa_shot(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *check_frame)
{
	int ret = 0;
	unsigned long flags;
	struct fimc_is_resourcemgr *resourcemgr;
	struct fimc_is_group *group, *child, *vra;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	struct camera2_node_group *node_group;
	struct camera2_node ldr_node = {0, };
	u32 setfile_save = 0;

#ifdef ENABLE_FAST_SHOT
	uint32_t af_trigger_bk;
	enum aa_afstate vendor_afstate_bk;
	enum aa_capture_intent captureIntent_bk;
#endif

	mdbgs_ischain(4, "%s()\n", device, __func__);

	FIMC_BUG(!device);
	FIMC_BUG(!check_frame);

	resourcemgr = device->resourcemgr;
	group = &device->group_3aa;
	frame = NULL;

	framemgr = GET_HEAD_GROUP_FRAMEMGR(group);
	if (!framemgr) {
		merr("framemgr is NULL", device);
		ret = -EINVAL;
		goto framemgr_err;
	}

	frame = peek_frame(framemgr, FS_REQUEST);

	if (unlikely(!frame)) {
		merr("frame is NULL", device);
		ret = -EINVAL;
		goto frame_err;
	}

	if (unlikely(frame != check_frame)) {
		merr("frame checking is fail(%p != %p)", device, frame, check_frame);
		ret = -EINVAL;
		goto p_err;
	}

	if (unlikely(!frame->shot)) {
		merr("frame->shot is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

#ifdef ENABLE_IS_CORE
	if (unlikely(!test_bit(FRAME_MEM_MAPPED, &frame->mem_state))) {
		fimc_is_itf_map(device, GROUP_ID(group->id), frame->dvaddr_shot, frame->shot_size);
		set_bit(FRAME_MEM_MAPPED, &frame->mem_state);
	}
#endif

	frame->shot->ctl.vendor_entry.lowIndexParam = 0;
	frame->shot->ctl.vendor_entry.highIndexParam = 0;
	frame->shot->dm.vendor_entry.lowIndexParam = 0;
	frame->shot->dm.vendor_entry.highIndexParam = 0;
	node_group = &frame->shot_ext->node_group;

	PROGRAM_COUNT(8);

	if ((frame->shot_ext->setfile != device->setfile) &&
		(group->id == get_ischain_leader_group(device)->id)) {
			setfile_save = device->setfile;
			device->setfile = frame->shot_ext->setfile;

		mgrinfo(" setfile change at shot(%d -> %d)\n", device, group, frame,
			setfile_save, device->setfile & FIMC_IS_SETFILE_MASK);

		if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state)) {
			ret = fimc_is_ischain_chg_setfile(device);
			if (ret) {
				merr("fimc_is_ischain_chg_setfile is fail", device);
				device->setfile = setfile_save;
				goto p_err;
			}
		}
	}

#ifdef ENABLE_FAST_SHOT
	/* only fast shot can be enabled in case hal 1.0 */
	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state) &&
		(resourcemgr->hal_version == IS_HAL_VER_1_0)) {
		af_trigger_bk = frame->shot->ctl.aa.afTrigger;
		vendor_afstate_bk = frame->shot->ctl.aa.vendor_afState;
		captureIntent_bk = frame->shot->ctl.aa.captureIntent;

		memcpy(&frame->shot->ctl.aa, &group->fast_ctl.aa,
			sizeof(struct camera2_aa_ctl));
		memcpy(&frame->shot->ctl.scaler, &group->fast_ctl.scaler,
			sizeof(struct camera2_scaler_ctl));

		frame->shot->ctl.aa.afTrigger = af_trigger_bk;
		frame->shot->ctl.aa.vendor_afState = vendor_afstate_bk;
		frame->shot->ctl.aa.captureIntent = captureIntent_bk;
	}
#endif

	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state)) {
		enum aa_capture_intent captureIntent;
		captureIntent = group->intent_ctl.captureIntent;
		
		if (captureIntent != AA_CAPTURE_INTENT_CUSTOM) {
			frame->shot->ctl.aa.captureIntent = captureIntent;
			frame->shot->ctl.aa.vendor_captureCount = group->intent_ctl.vendor_captureCount;
			frame->shot->ctl.aa.vendor_captureExposureTime = group->intent_ctl.vendor_captureExposureTime;
			frame->shot->ctl.aa.vendor_captureEV = group->intent_ctl.vendor_captureEV;
			if (group->remainIntentCount > 0) {
				group->remainIntentCount--;
			} else {
				group->intent_ctl.captureIntent = AA_CAPTURE_INTENT_CUSTOM;
				group->intent_ctl.vendor_captureCount = 0;
				group->intent_ctl.vendor_captureExposureTime = 0;
				group->intent_ctl.vendor_captureEV = 0;
			}
			minfo("frame count(%d), intent(%d), count(%d), EV(%d), captureExposureTime(%d) remainIntentCount(%d)\n",
				device, frame->fcount,
				frame->shot->ctl.aa.captureIntent, frame->shot->ctl.aa.vendor_captureCount,
				frame->shot->ctl.aa.vendor_captureEV, frame->shot->ctl.aa.vendor_captureExposureTime,
				group->remainIntentCount);
		}

		if (group->lens_ctl.aperture != 0) {
			frame->shot->ctl.lens.aperture = group->lens_ctl.aperture;
			group->lens_ctl.aperture = 0;
		}

		/*
		 * Adjust the shot timeout value based on sensor exposure time control.
		 * Exposure Time >= (FIMC_IS_SHOT_TIMEOUT - 1sec): Increase shot timeout value.
		 */
		if (frame->shot->ctl.aa.vendor_captureExposureTime >= ((FIMC_IS_SHOT_TIMEOUT * 1000) - 1000000)) {
			resourcemgr->shot_timeout = (frame->shot->ctl.aa.vendor_captureExposureTime / 1000) + FIMC_IS_SHOT_TIMEOUT;
			resourcemgr->shot_timeout_tick = KEEP_FRAME_TICK_DEFAULT;
		} else if (resourcemgr->shot_timeout_tick > 0) {
			resourcemgr->shot_timeout_tick--;
		} else {
			resourcemgr->shot_timeout = FIMC_IS_SHOT_TIMEOUT;
		}
	}

	/* fd information copy */
#if !defined(ENABLE_SHARED_METADATA) && !defined(FAST_FDAE)
	memcpy(&frame->shot->uctl.fdUd, &device->fdUd, sizeof(struct camera2_fd_uctl));
#endif

	PROGRAM_COUNT(9);

	if (test_bit(FIMC_IS_SUBDEV_PARAM_ERR, &group->head->leader.state))
		set_bit(FIMC_IS_SUBDEV_FORCE_SET, &group->head->leader.state);
	else
		clear_bit(FIMC_IS_SUBDEV_FORCE_SET, &group->head->leader.state);

	child = group;
	while (child) {
		switch (child->slot) {
		case GROUP_SLOT_3AA:
			TRANS_CROP(ldr_node.input.cropRegion,
				node_group->leader.input.cropRegion);
			if (child->junction->cid < CAPTURE_NODE_MAX) {
				TRANS_CROP(ldr_node.output.cropRegion,
					node_group->capture[child->junction->cid].output.cropRegion);
			} else {
				mgerr("capture id(%d) is invalid", group, group, child->junction->cid);
			}

			ret = fimc_is_ischain_3aa_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_3aa_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		case GROUP_SLOT_ISP:
			TRANS_CROP(ldr_node.input.cropRegion,
				ldr_node.output.cropRegion);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_isp_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_isp_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		case GROUP_SLOT_DIS:
			TRANS_CROP(ldr_node.input.cropRegion,
				ldr_node.output.cropRegion);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_dis_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_dis_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		case GROUP_SLOT_MCS:
			TRANS_CROP(ldr_node.input.cropRegion,
				ldr_node.output.cropRegion);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_mcs_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_mcs_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		case GROUP_SLOT_VRA:
			vra = &device->group_vra;
			TRANS_CROP(ldr_node.input.cropRegion,
				(u32 *)&vra->prev->junction->output.crop);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_vra_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_vra_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		default:
			merr("group slot is invalid(%d)", device, child->slot);
			BUG();
		}

		child = child->child;
	}

	PROGRAM_COUNT(10);

p_err:
	fimc_is_ischain_update_shot(device, frame);

	if (ret) {
		mgrerr(" SKIP(%d) : %d\n", device, group, check_frame, check_frame->index, ret);
	} else {
		set_bit(group->leader.id, &frame->out_flag);
		framemgr_e_barrier_irqs(framemgr, FMGR_IDX_25, flags);
		trans_frame(framemgr, frame, FS_PROCESS);
#ifdef SENSOR_REQUEST_DELAY
		if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state) &&
			(frame->shot->uctl.opMode == CAMERA_OP_MODE_HAL3_GED
			|| frame->shot->uctl.opMode == CAMERA_OP_MODE_HAL3_SDK
			|| frame->shot->uctl.opMode == CAMERA_OP_MODE_HAL3_CAMERAX)) {
			if (framemgr->queued_count[FS_REQUEST] < SENSOR_REQUEST_DELAY)
				mgrwarn(" late sensor control shot", device, group, frame);
		}
#endif
		framemgr_x_barrier_irqr(framemgr, FMGR_IDX_25, flags);
	}

frame_err:
framemgr_err:
	return ret;
}

static int fimc_is_ischain_isp_shot(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *check_frame)
{
	int ret = 0;
	unsigned long flags;
	struct fimc_is_group *group, *child, *vra;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	struct camera2_node_group *node_group;
	struct camera2_node ldr_node = {0, };
	u32 setfile_save = 0;

	FIMC_BUG(!device);
	FIMC_BUG(!check_frame);
	FIMC_BUG(device->instance_sensor >= FIMC_IS_SENSOR_COUNT);

	mdbgs_ischain(4, "%s\n", device, __func__);

	frame = NULL;
	group = &device->group_isp;

	framemgr = GET_HEAD_GROUP_FRAMEMGR(group);
	if (!framemgr) {
		merr("framemgr is NULL", device);
		ret = -EINVAL;
		goto framemgr_err;
	}

	frame = peek_frame(framemgr, FS_REQUEST);

	if (unlikely(!frame)) {
		merr("frame is NULL", device);
		ret = -EINVAL;
		goto frame_err;
	}

	if (unlikely(frame != check_frame)) {
		merr("frame checking is fail(%p != %p)", device, frame, check_frame);
		ret = -EINVAL;
		goto p_err;
	}

	if (unlikely(!frame->shot)) {
		merr("frame->shot is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

#ifdef ENABLE_IS_CORE
	if (unlikely(!test_bit(FRAME_MEM_MAPPED, &frame->mem_state))) {
		fimc_is_itf_map(device, GROUP_ID(group->id), frame->dvaddr_shot, frame->shot_size);
		set_bit(FRAME_MEM_MAPPED, &frame->mem_state);
	}
#endif

	frame->shot->ctl.vendor_entry.lowIndexParam = 0;
	frame->shot->ctl.vendor_entry.highIndexParam = 0;
	frame->shot->dm.vendor_entry.lowIndexParam = 0;
	frame->shot->dm.vendor_entry.highIndexParam = 0;
	node_group = &frame->shot_ext->node_group;

	PROGRAM_COUNT(8);

	if ((frame->shot_ext->setfile != device->setfile) &&
		(group->id == get_ischain_leader_group(device)->id)) {
			setfile_save = device->setfile;
			device->setfile = frame->shot_ext->setfile;

		mgrinfo(" setfile change at shot(%d -> %d)\n", device, group, frame,
			setfile_save, device->setfile & FIMC_IS_SETFILE_MASK);

		if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state)) {
			ret = fimc_is_ischain_chg_setfile(device);
			if (ret) {
				merr("fimc_is_ischain_chg_setfile is fail", device);
				device->setfile = setfile_save;
				goto p_err;
			}
		}
	}

	PROGRAM_COUNT(9);

	if (test_bit(FIMC_IS_SUBDEV_PARAM_ERR, &group->leader.state))
		set_bit(FIMC_IS_SUBDEV_FORCE_SET, &group->leader.state);
	else
		clear_bit(FIMC_IS_SUBDEV_FORCE_SET, &group->leader.state);

	child = group;
	while (child) {
		switch (child->slot) {
		case GROUP_SLOT_ISP:
			TRANS_CROP(ldr_node.input.cropRegion,
				node_group->leader.input.cropRegion);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_isp_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_isp_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		case GROUP_SLOT_DIS:
			TRANS_CROP(ldr_node.input.cropRegion,
				ldr_node.output.cropRegion);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_dis_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_dis_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		case GROUP_SLOT_MCS:
			TRANS_CROP(ldr_node.input.cropRegion,
				ldr_node.output.cropRegion);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_mcs_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_mcs_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		case GROUP_SLOT_VRA:
			vra = &device->group_vra;
			TRANS_CROP(ldr_node.input.cropRegion,
				(u32 *)&vra->prev->junction->output.crop);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_vra_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_vra_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		default:
			merr("group slot is invalid(%d)", device, child->slot);
			BUG();
		}

		child = child->child;
	}

#ifdef PRINT_PARAM
	if (frame->fcount == 1) {
		fimc_is_hw_memdump(device->interface,
			(ulong) &device->is_region->parameter,
			(ulong) &device->is_region->parameter + sizeof(device->is_region->parameter));
	}
#endif

	PROGRAM_COUNT(10);

p_err:
	if (ret) {
		mgrerr(" SKIP(%d) : %d\n", device, group, check_frame, check_frame->index, ret);
	} else {
		set_bit(group->leader.id, &frame->out_flag);
		framemgr_e_barrier_irqs(framemgr, FMGR_IDX_26, flags);
		trans_frame(framemgr, frame, FS_PROCESS);
		framemgr_x_barrier_irqr(framemgr, FMGR_IDX_26, flags);
	}

frame_err:
framemgr_err:
	return ret;
}

static int fimc_is_ischain_dis_shot(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *check_frame)
{
	int ret = 0;
	unsigned long flags;
	struct fimc_is_group *group, *child, *vra;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	struct camera2_node_group *node_group;
	struct camera2_node ldr_node = {0, };

	mdbgs_ischain(4, "%s()\n", device, __func__);

	FIMC_BUG(!device);
	FIMC_BUG(!check_frame);

	frame = NULL;
	group = &device->group_dis;

	framemgr = GET_HEAD_GROUP_FRAMEMGR(group);
	if (!framemgr) {
		merr("framemgr is NULL", device);
		ret = -EINVAL;
		goto framemgr_err;
	}

	frame = peek_frame(framemgr, FS_REQUEST);

	if (unlikely(!frame)) {
		merr("frame is NULL", device);
		ret = -EINVAL;
		goto frame_err;
	}

	if (unlikely(frame != check_frame)) {
		merr("frame checking is fail(%p != %p)", device, frame, check_frame);
		ret = -EINVAL;
		goto p_err;
	}

	if (unlikely(!frame->shot)) {
		merr("frame->shot is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

#ifdef ENABLE_IS_CORE
	if (unlikely(!test_bit(FRAME_MEM_MAPPED, &frame->mem_state))) {
		fimc_is_itf_map(device, GROUP_ID(group->id), frame->dvaddr_shot, frame->shot_size);
		set_bit(FRAME_MEM_MAPPED, &frame->mem_state);
	}
#endif

	frame->shot->ctl.vendor_entry.lowIndexParam = 0;
	frame->shot->ctl.vendor_entry.highIndexParam = 0;
	frame->shot->dm.vendor_entry.lowIndexParam = 0;
	frame->shot->dm.vendor_entry.highIndexParam = 0;
	node_group = &frame->shot_ext->node_group;

	PROGRAM_COUNT(8);

	PROGRAM_COUNT(9);

	if (test_bit(FIMC_IS_SUBDEV_PARAM_ERR, &group->leader.state))
		set_bit(FIMC_IS_SUBDEV_FORCE_SET, &group->leader.state);
	else
		clear_bit(FIMC_IS_SUBDEV_FORCE_SET, &group->leader.state);

	child = group;
	while (child) {
		switch (child->slot) {
		case GROUP_SLOT_DIS:
			TRANS_CROP(ldr_node.input.cropRegion,
				node_group->leader.input.cropRegion);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_dis_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_dis_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		case GROUP_SLOT_MCS:
			TRANS_CROP(ldr_node.input.cropRegion,
				ldr_node.output.cropRegion);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_mcs_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_mcs_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		case GROUP_SLOT_VRA:
			vra = &device->group_vra;
			TRANS_CROP(ldr_node.input.cropRegion,
				(u32 *)&vra->prev->junction->output.crop);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_vra_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_vra_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		default:
			merr("group slot is invalid(%d)", device, child->slot);
			BUG();
		}

		child = child->child;
	}

	PROGRAM_COUNT(10);

p_err:
	if (ret) {
		mgrerr(" SKIP(%d) : %d\n", device, group, check_frame, check_frame->index, ret);
	} else {
		set_bit(group->leader.id, &frame->out_flag);
		framemgr_e_barrier_irqs(framemgr, FMGR_IDX_27, flags);
		trans_frame(framemgr, frame, FS_PROCESS);
		framemgr_x_barrier_irqr(framemgr, FMGR_IDX_27, flags);
	}

frame_err:
framemgr_err:
	return ret;
}

static int fimc_is_ischain_dcp_shot(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *check_frame)
{
	int ret = 0;
	unsigned long flags;
	struct fimc_is_group *group, *child;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	struct camera2_node_group *node_group;
	struct camera2_node ldr_node = {0, };

	mdbgs_ischain(4, "%s()\n", device, __func__);

	FIMC_BUG(!device);
	FIMC_BUG(!check_frame);

	frame = NULL;
	group = &device->group_dcp;

	framemgr = GET_HEAD_GROUP_FRAMEMGR(group);
	if (!framemgr) {
		merr("framemgr is NULL", device);
		ret = -EINVAL;
		goto framemgr_err;
	}

	frame = peek_frame(framemgr, FS_REQUEST);

	if (unlikely(!frame)) {
		merr("frame is NULL", device);
		ret = -EINVAL;
		goto frame_err;
	}

	if (unlikely(frame != check_frame)) {
		merr("frame checking is fail(%p != %p)", device, frame, check_frame);
		ret = -EINVAL;
		goto p_err;
	}

	if (unlikely(!frame->shot)) {
		merr("frame->shot is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

#ifdef ENABLE_IS_CORE
	if (unlikely(!test_bit(FRAME_MEM_MAPPED, &frame->mem_state))) {
		fimc_is_itf_map(device, GROUP_ID(group->id), frame->dvaddr_shot, frame->shot_size);
		set_bit(FRAME_MEM_MAPPED, &frame->mem_state);
	}
#endif

	frame->shot->ctl.vendor_entry.lowIndexParam = 0;
	frame->shot->ctl.vendor_entry.highIndexParam = 0;
	frame->shot->dm.vendor_entry.lowIndexParam = 0;
	frame->shot->dm.vendor_entry.highIndexParam = 0;
	node_group = &frame->shot_ext->node_group;

	PROGRAM_COUNT(8);

	if (test_bit(FIMC_IS_SUBDEV_PARAM_ERR, &group->leader.state))
		set_bit(FIMC_IS_SUBDEV_FORCE_SET, &group->leader.state);
	else
		clear_bit(FIMC_IS_SUBDEV_FORCE_SET, &group->leader.state);

	child = group;
	while (child) {
		switch (child->slot) {
		case GROUP_SLOT_DCP:
			TRANS_CROP(ldr_node.input.cropRegion,
				node_group->leader.input.cropRegion);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_dcp_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_dcp_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		case GROUP_SLOT_MCS:
			TRANS_CROP(ldr_node.input.cropRegion,
				ldr_node.output.cropRegion);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_mcs_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_mcs_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		default:
			merr("group slot is invalid(%d)", device, child->slot);
			BUG();
		}

		child = child->child;
	}

	PROGRAM_COUNT(10);

p_err:
	if (ret) {
		mgrerr(" SKIP(%d) : %d\n", device, group, check_frame, check_frame->index, ret);
	} else {
		set_bit(group->leader.id, &frame->out_flag);
		framemgr_e_barrier_irqs(framemgr, FMGR_IDX_28, flags);
		trans_frame(framemgr, frame, FS_PROCESS);
		framemgr_x_barrier_irqr(framemgr, FMGR_IDX_28, flags);
	}

frame_err:
framemgr_err:
	return ret;
}

static int fimc_is_ischain_mcs_shot(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *check_frame)
{
	int ret = 0;
	unsigned long flags;
	struct fimc_is_group *group, *child, *vra;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	struct camera2_node_group *node_group;
	struct camera2_node ldr_node = {0, };
	u32 setfile_save = 0;

	mdbgs_ischain(4, "%s()\n", device, __func__);

	FIMC_BUG(!device);
	FIMC_BUG(!check_frame);

	frame = NULL;
	group = &device->group_mcs;

	framemgr = GET_HEAD_GROUP_FRAMEMGR(group);
	if (!framemgr) {
		merr("framemgr is NULL", device);
		ret = -EINVAL;
		goto framemgr_err;
	}

	frame = peek_frame(framemgr, FS_REQUEST);

	if (unlikely(!frame)) {
		merr("frame is NULL", device);
		ret = -EINVAL;
		goto frame_err;
	}

	if (unlikely(frame != check_frame)) {
		merr("frame checking is fail(%p != %p)", device, frame, check_frame);
		ret = -EINVAL;
		goto p_err;
	}

	if (unlikely(!frame->shot)) {
		merr("frame->shot is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

#ifdef ENABLE_IS_CORE
	if (unlikely(!test_bit(FRAME_MEM_MAPPED, &frame->mem_state))) {
		fimc_is_itf_map(device, GROUP_ID(group->id), frame->dvaddr_shot, frame->shot_size);
		set_bit(FRAME_MEM_MAPPED, &frame->mem_state);
	}
#endif

	frame->shot->ctl.vendor_entry.lowIndexParam = 0;
	frame->shot->ctl.vendor_entry.highIndexParam = 0;
	frame->shot->dm.vendor_entry.lowIndexParam = 0;
	frame->shot->dm.vendor_entry.highIndexParam = 0;
	node_group = &frame->shot_ext->node_group;

	PROGRAM_COUNT(8);

	if ((frame->shot_ext->setfile != device->setfile) &&
		(group->id == get_ischain_leader_group(device)->id)) {
			setfile_save = device->setfile;
			device->setfile = frame->shot_ext->setfile;

		mgrinfo(" setfile change at shot(%d -> %d)\n", device, group, frame,
			setfile_save, device->setfile & FIMC_IS_SETFILE_MASK);

		if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state)) {
			ret = fimc_is_ischain_chg_setfile(device);
			if (ret) {
				merr("fimc_is_ischain_chg_setfile is fail", device);
				device->setfile = setfile_save;
				goto p_err;
			}
		}
	}

	PROGRAM_COUNT(9);

	if (test_bit(FIMC_IS_SUBDEV_PARAM_ERR, &group->leader.state))
		set_bit(FIMC_IS_SUBDEV_FORCE_SET, &group->leader.state);
	else
		clear_bit(FIMC_IS_SUBDEV_FORCE_SET, &group->leader.state);

	child = group;
	while (child) {
		switch (child->slot) {
		case GROUP_SLOT_MCS:
			TRANS_CROP(ldr_node.input.cropRegion,
				node_group->leader.input.cropRegion);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_mcs_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_mcs_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		case GROUP_SLOT_VRA:
			vra = &device->group_vra;
			TRANS_CROP(ldr_node.input.cropRegion,
				(u32 *)&vra->prev->junction->output.crop);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_vra_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_vra_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		default:
			merr("group slot is invalid(%d)", device, child->slot);
			BUG();
		}

		child = child->child;
	}

	PROGRAM_COUNT(10);

p_err:
	if (ret) {
		mgrerr(" SKIP(%d) : %d\n", device, group, check_frame, check_frame->index, ret);
	} else {
		set_bit(group->leader.id, &frame->out_flag);
		framemgr_e_barrier_irqs(framemgr, FMGR_IDX_29, flags);
		trans_frame(framemgr, frame, FS_PROCESS);
		framemgr_x_barrier_irqr(framemgr, FMGR_IDX_29, flags);
	}

frame_err:
framemgr_err:
	return ret;
}

static int fimc_is_ischain_vra_shot(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *check_frame)
{
	int ret = 0;
	unsigned long flags;
	struct fimc_is_group *group, *child;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	struct camera2_node_group *node_group;
	struct camera2_node ldr_node = {0, };
	u32 setfile_save = 0;

	mdbgs_ischain(4, "%s()\n", device, __func__);

	FIMC_BUG(!device);
	FIMC_BUG(!check_frame);

	frame = NULL;
	group = &device->group_vra;

	framemgr = GET_HEAD_GROUP_FRAMEMGR(group);
	if (!framemgr) {
		merr("framemgr is NULL", device);
		ret = -EINVAL;
		goto framemgr_err;
	}

	frame = peek_frame(framemgr, FS_REQUEST);

	if (unlikely(!frame)) {
		merr("frame is NULL", device);
		ret = -EINVAL;
		goto frame_err;
	}

	if (unlikely(frame != check_frame)) {
		merr("frame checking is fail(%p != %p)", device, frame, check_frame);
		ret = -EINVAL;
		goto p_err;
	}

	if (unlikely(!frame->shot)) {
		merr("frame->shot is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}


#ifdef ENABLE_IS_CORE
	if (unlikely(!test_bit(FRAME_MEM_MAPPED, &frame->mem_state))) {
		fimc_is_itf_map(device, GROUP_ID(group->id), frame->dvaddr_shot, frame->shot_size);
		set_bit(FRAME_MEM_MAPPED, &frame->mem_state);
	}
#endif

	frame->shot->ctl.vendor_entry.lowIndexParam = 0;
	frame->shot->ctl.vendor_entry.highIndexParam = 0;
	frame->shot->dm.vendor_entry.lowIndexParam = 0;
	frame->shot->dm.vendor_entry.highIndexParam = 0;
	node_group = &frame->shot_ext->node_group;

	PROGRAM_COUNT(8);

	if ((frame->shot_ext->setfile != device->setfile) &&
		(group->id == get_ischain_leader_group(device)->id)) {
			setfile_save = device->setfile;
			device->setfile = frame->shot_ext->setfile;

		mgrinfo(" setfile change at shot(%d -> %d)\n", device, group, frame,
			setfile_save, device->setfile & FIMC_IS_SETFILE_MASK);

		if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state)) {
			ret = fimc_is_ischain_chg_setfile(device);
			if (ret) {
				merr("fimc_is_ischain_chg_setfile is fail", device);
				device->setfile = setfile_save;
				goto p_err;
			}
		}
	}

	PROGRAM_COUNT(9);

	if (test_bit(FIMC_IS_SUBDEV_PARAM_ERR, &group->leader.state))
		set_bit(FIMC_IS_SUBDEV_FORCE_SET, &group->leader.state);
	else
		clear_bit(FIMC_IS_SUBDEV_FORCE_SET, &group->leader.state);

	child = group;
	while (child) {
		switch (child->slot) {
		case GROUP_SLOT_VRA:
			TRANS_CROP(ldr_node.input.cropRegion,
				node_group->leader.input.cropRegion);
			TRANS_CROP(ldr_node.output.cropRegion,
				ldr_node.input.cropRegion);
			ret = fimc_is_ischain_vra_group_tag(device, frame, &ldr_node);
			if (ret) {
				merr("fimc_is_ischain_vra_group_tag is fail(%d)", device, ret);
				goto p_err;
			}
			break;
		default:
			merr("group slot is invalid(%d)", device, child->slot);
			BUG();
		}

		child = child->child;
	}

	PROGRAM_COUNT(10);

p_err:
	if (ret) {
		mgrerr(" SKIP(%d) : %d\n", device, group, check_frame, check_frame->index, ret);
	} else {
		set_bit(group->leader.id, &frame->out_flag);
		framemgr_e_barrier_irqs(framemgr, FMGR_IDX_30, flags);
		trans_frame(framemgr, frame, FS_PROCESS);
		framemgr_x_barrier_irqr(framemgr, FMGR_IDX_30, flags);
	}

frame_err:
framemgr_err:
	return ret;
}

void fimc_is_bts_control(struct fimc_is_device_ischain *device)
{
#if defined(USE_BTS_SCEN)
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group_leader;

	FIMC_BUG_VOID(!device);
	FIMC_BUG_VOID(!device->groupmgr);

	groupmgr = device->groupmgr;
	group_leader = groupmgr->leader[device->instance];

	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group_leader->state))
		bts_scen_update(TYPE_CAM_BNS, false);
	else
		bts_scen_update(TYPE_CAM_BNS, true);
#endif
}
