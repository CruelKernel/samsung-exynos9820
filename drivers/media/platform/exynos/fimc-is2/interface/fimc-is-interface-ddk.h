/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_INTERFACE_LIBRARY_H
#define FIMC_IS_INTERFACE_LIBRARY_H

#include "fimc-is-core.h"
#include "fimc-is-interface-library.h"
#include "fimc-is-param.h"
#include "fimc-is-config.h"

#define CHAIN_ID_MASK		(0x0000000F)
#define CHAIN_ID_SHIFT		(0)
#define INSTANCE_ID_MASK	(0x000000F0)
#define INSTANCE_ID_SHIFT	(4)
#define REPROCESSING_FLAG_MASK	(0x00000F00)
#define REPROCESSING_FLAG_SHIFT	(8)
#define INPUT_TYPE_MASK		(0x0000F000)
#define INPUT_TYPE_SHIFT	(12)
#define MODULE_ID_MASK		(0x0FFF0000)
#define MODULE_ID_SHIFT		(16)

struct fimc_is_lib_isp {
	void				*object;
	struct lib_interface_func	*func;
	ulong				tune_count;
};

enum lib_cb_event_type {
	LIB_EVENT_CONFIG_LOCK			= 1,
	LIB_EVENT_FRAME_START			= 2,
	LIB_EVENT_FRAME_END			= 3,
	LIB_EVENT_DMA_A_OUT_DONE		= 4,
	LIB_EVENT_DMA_B_OUT_DONE		= 5,
	LIB_EVENT_FRAME_START_ISR		= 6,
	LIB_EVENT_ERROR_CIN_OVERFLOW		= 7,
	LIB_EVENT_ERROR_CIN_COLUMN		= 8,
	LIB_EVENT_ERROR_CIN_LINE		= 9,
	LIB_EVENT_ERROR_CIN_TOTAL_SIZE		= 10,
	LIB_EVENT_ERROR_COUT_OVERFLOW		= 11,
	LIB_EVENT_ERROR_COUT_COLUMN		= 12,
	LIB_EVENT_ERROR_COUT_LINE		= 13,
	LIB_EVENT_ERROR_COUT_TOTAL_SIZE		= 14,
	LIB_EVENT_ERROR_CONFIG_LOCK_DELAY	= 15,
	LIB_EVENT_ERROR_COMP_OVERFLOW		= 16,
	LIB_EVENT_END
};

enum dcp_dma_in_type {
	DCP_DMA_IN_GDC_MASTER,
	DCP_DMA_IN_GDC_SLAVE,
	DCP_DMA_IN_DISPARITY,
	DCP_DMA_IN_MAX
};

enum dcp_dma_out_type {
	DCP_DMA_OUT_MASTER,	/* plane order is [Y, C, Y2, C2] */
	DCP_DMA_OUT_SLAVE,	/* plane order is [Y, C] */
	DCP_DMA_OUT_MASTER_DS,
	DCP_DMA_OUT_SLAVE_DS,
	DCP_DMA_OUT_DISPARITY,
	DCP_DMA_OUT_MAX
};

#if 0 /* #ifdef SOC_DRC *//* DRC is controlled by library */
struct grid_rectangle {
	u32 width;
	u32 height;
};

struct param_grid_dma_input {
	u32			drc_buffer0_addr;
	u32			drc_buffer1_addr;
	struct grid_rectangle	drc_grid0_size;
	struct grid_rectangle	drc_grid1_size;
	bool			drc_enable0;
	bool			drc_enable1;
};
#endif

struct taa_param_set {
	struct param_sensor_config	sensor_config;
	struct param_otf_input		otf_input;
	struct param_dma_input		dma_input;
	struct param_dma_input		ddma_input;	/* deprecated */
	struct param_otf_output		otf_output;
	struct param_dma_output		dma_output_before_bds;
	struct param_dma_output		dma_output_after_bds;
	struct param_dma_output		dma_output_mrg;
	struct param_dma_output		dma_output_efd;
	struct param_dma_output		ddma_output;	/* deprecated */
#if 0 /* #ifdef SOC_DRC *//* DRC is controlled by library */
	bool				drc_en;
	u32				drc_grid0_dvaddr;
	u32				drc_grid1_dvaddr;
#endif
	u32				input_dva[FIMC_IS_MAX_PLANES];
	u32				output_dva_before_bds[FIMC_IS_MAX_PLANES];
	u32				output_dva_after_bds[FIMC_IS_MAX_PLANES];
	u32				output_dva_mrg[FIMC_IS_MAX_PLANES];
	u32				output_dva_efd[FIMC_IS_MAX_PLANES];
	uint64_t			output_kva_me[FIMC_IS_MAX_PLANES];	/* ME out */

	u32				instance_id;
	u32				fcount;
	bool				reprocessing;
};

struct isp_param_set {
	struct taa_param_set		*taa_param;
	struct param_otf_input		otf_input;
	struct param_dma_input		dma_input;
	struct param_dma_input		vdma3_input;	/* deprecated */
	struct param_otf_output		otf_output;
	struct param_dma_output		dma_output_chunk;
	struct param_dma_output		dma_output_yuv;
#if 0 /* #ifdef SOC_DRC *//* DRC is controlled by library */
	struct param_grid_dma_input	*drc_grid_input;
#endif
	u32				input_dva[FIMC_IS_MAX_PLANES];
	u32				output_dva_chunk[FIMC_IS_MAX_PLANES];
	u32				output_dva_yuv[FIMC_IS_MAX_PLANES];
	uint64_t			output_kva_me[FIMC_IS_MAX_PLANES];	/* ME out */

	u32				instance_id;
	u32				fcount;
	bool				reprocessing;
};

struct tpu_param_set {
	struct param_control		control;
	struct param_tpu_config		config;
	struct param_otf_input		otf_input;
	struct param_dma_input		dma_input;
	struct param_otf_output		otf_output;
	struct param_dma_output		dma_output;
	u32				input_dva[FIMC_IS_MAX_PLANES];
	u32				output_dva[FIMC_IS_MAX_PLANES];

	u32 				instance_id;
	u32				fcount;
	bool				reprocessing;
};

struct dcp_param_set {
	struct param_control		control;
	struct param_dcp_config		config;

	struct param_dma_input		dma_input_m;	/* master input */
	struct param_dma_input		dma_input_s;	/* slave input */
	struct param_dma_output		dma_output_m;	/* master output */
	struct param_dma_output		dma_output_s;	/* slave output */
	struct param_dma_output		dma_output_m_ds;/* master Down Scale output */
	struct param_dma_output		dma_output_s_ds;/* slave Down Scale output */

	struct param_dma_input		dma_input_disparity;
	struct param_dma_output		dma_output_disparity;

	u32				input_dva[DCP_DMA_IN_MAX][4];
	u32				output_dva[DCP_DMA_OUT_MAX][4];

	u32 				instance_id;
	u32				fcount;
	bool				reprocessing;
};

struct lib_callback_func {
	void (*camera_callback)(void *hw_ip, enum lib_cb_event_type event_id,
		u32 instance_id, void *data);
	void (*io_callback)(void *hw_ip, enum lib_cb_event_type event_id,
		u32 instance_id);
};

struct lib_tune_set {
	u32 index;
	ulong addr;
	u32 size;
	int decrypt_flag;
};

#define LIB_ISP_ADDR		(DDK_LIB_ADDR + LIB_ISP_OFFSET)
enum lib_func_type {
	LIB_FUNC_3AA = 1,
	LIB_FUNC_ISP,
	LIB_FUNC_TPU,
	LIB_FUNC_VRA,
	LIB_FUNC_DCP,
	LIB_FUNC_TYPE_END
};

struct lib_system_config {
	u32 overflow_recovery;	/* 0: do not execute recovery, 1: execute recovery */
	u32 reserved[9];
};

typedef u32 (*register_lib_isp)(u32 type, void **lib_func);
static const register_lib_isp get_lib_func = (register_lib_isp)LIB_ISP_ADDR;

struct lib_interface_func {
	int (*chain_create)(u32 chain_id, ulong addr, u32 offset,
		const struct lib_callback_func *cb);
	int (*object_create)(void **pobject, u32 obj_info, void *hw);
	int (*chain_destroy)(u32 chain_id);
	int (*object_destroy)(void *object, u32 sensor_id);
	int (*stop)(void *object, u32 instance_id);
	int (*recovery)(u32 instance_id);
	int (*set_param)(void *object, void *param_set);
	int (*set_ctrl)(void *object, u32 instance, u32 frame_number,
		struct camera2_shot *shot);
	int (*shot)(void *object, void *param_set, struct camera2_shot *shot, u32 num_buffers);
	int (*get_meta)(void *object, u32 instance, u32 frame_number,
		struct camera2_shot *shot);
	int (*create_tune_set)(void *isp_object, u32 instance, struct lib_tune_set *set);
	int (*apply_tune_set)(void *isp_object, u32 instance_id, u32 index);
	int (*delete_tune_set)(void *isp_object, u32 instance_id, u32 index);
	int (*set_parameter_flash)(void *object, struct param_isp_flash *p_flash);
	int (*set_parameter_af)(void *isp_object, struct param_isp_aa *param_aa,
		u32 frame_number, u32 instance);
	int (*load_cal_data)(void *isp_object, u32 instance_id, ulong kvaddr);
	int (*get_cal_data)(void *isp_object, u32 instance_id,
		struct cal_info *data, int type);
	int (*sensor_info_mode_chg)(void *isp_object, u32 instance_id,
		struct camera2_shot *shot);
	int (*sensor_update_ctl)(void *isp_object, u32 instance_id,
		u32 frame_count, struct camera2_shot *shot);
	int (*set_system_config)(struct lib_system_config *config);
};

int fimc_is_lib_isp_chain_create(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, u32 instance_id);
int fimc_is_lib_isp_object_create(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, u32 instance_id, u32 rep_flag, u32 module_id);
void fimc_is_lib_isp_chain_destroy(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, u32 instance_id);
void fimc_is_lib_isp_object_destroy(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, u32 instance_id);
int fimc_is_lib_isp_set_param(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, void *param);
int fimc_is_lib_isp_set_ctrl(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, struct fimc_is_frame *frame);
void fimc_is_lib_isp_shot(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, void *param_set, struct camera2_shot *shot);
int fimc_is_lib_isp_get_meta(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, struct fimc_is_frame *frame);
void fimc_is_lib_isp_stop(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, u32 instance_id);
int fimc_is_lib_isp_create_tune_set(struct fimc_is_lib_isp *this,
	ulong addr, u32 size, u32 index, int flag, u32 instance_id);
int fimc_is_lib_isp_apply_tune_set(struct fimc_is_lib_isp *this,
	u32 index, u32 instance_id);
int fimc_is_lib_isp_delete_tune_set(struct fimc_is_lib_isp *this,
	u32 index, u32 instance_id);
int fimc_is_lib_isp_load_cal_data(struct fimc_is_lib_isp *this,
	u32 index, ulong addr);
int fimc_is_lib_isp_get_cal_data(struct fimc_is_lib_isp *this,
	u32 instance_id, struct cal_info *c_info, int type);
int fimc_is_lib_isp_sensor_info_mode_chg(struct fimc_is_lib_isp *this,
	u32 instance_id, struct camera2_shot *shot);
int fimc_is_lib_isp_sensor_update_control(struct fimc_is_lib_isp *this,
	u32 instance_id, u32 frame_count, struct camera2_shot *shot);
int fimc_is_lib_isp_convert_face_map(struct fimc_is_hardware *hardware,
	struct taa_param_set *param_set, struct fimc_is_frame *frame);
void fimc_is_lib_isp_configure_algorithm(void);
void fimc_is_isp_get_bcrop1_size(void __iomem *base_addr, u32 *width, u32 *height);
int fimc_is_lib_isp_reset_recovery(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, u32 instance_id);

#ifdef ENABLE_FPSIMD_FOR_USER
#define CALL_LIBOP(lib, op, args...)					\
	({								\
		int ret_call_libop;					\
									\
		fpsimd_get();						\
		ret_call_libop = ((lib)->func->op ?			\
				(lib)->func->op(args) : -EINVAL);	\
		fpsimd_put();						\
									\
	ret_call_libop;})
#else
#define CALL_LIBOP(lib, op, args...)				\
	((lib)->func->op ? (lib)->func->op(args) : -EINVAL)
#endif

#endif
