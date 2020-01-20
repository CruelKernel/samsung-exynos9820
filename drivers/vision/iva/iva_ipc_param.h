/*
 *
 *  Copyright (C) 2016 Samsung Electrnoics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __IVA_IPC_PARAM_H__
#define __IVA_IPC_PARAM_H__

#include <stddef.h>
#define IVA_IPC_PARAM_ARCH64_SUPPORT
#undef CONFIG_TEST_PARAM

/* command parameter control switch */
#ifdef CONFIG_TEST_PARAM
#define SCL_IF_PARAM_TO_CMD_PARAM
#define PYRAMID_PARAM_TO_CMD_PARAM
#define WARP_MESH_PARAM_TO_CMD_PARAM
#define WARP_AFFINE_PARAM_TO_CMD_PARAM
#define NOISE_FILTER_PARAM_TO_CMD_PARAM
#define CORNER_COMP_PARAM_TO_CMD_PARAM
#define LOCAL_MINMAX_PARAM_TO_CMD_PARAM
#define LKT_IF_PARAM_TO_CMD_PARAM
#define MCH_LINEAR_PARAM_TO_CMD_PARAM
#define ORB_IN_PARAM_TO_CMD_PARAM
#define DSP_IPC_PARAM_TO_CMD_PARAM
#define GEN_IPC_PARAM_TO_CMD_PARAM
#define WIG_MFB_ENF_IPC_PARAM_TO_CMD_PARAM

#undef BAX_ICODE_IPC_PARAM_TO_CMD_PARAM
#undef BAX_JACOBIAN_IPC_PARAM_TO_CMD_PARAM

#define PEAKPOWER_IPC_PARAM_TO_CMD_PARAM

/* response parameter control switch */
#define MCH_OUT_PARAM_TO_RES_PARAM
#define LMD_OUT_PARAM_TO_RES_PARAM
#define LKT_OUT_PARAM_TO_RES_PARAM
#define ORB_OUT_PARAM_TO_RES_PARAM
#define TIME_TO_RES_PARAM
#endif

/* ipc parameter definitions */
struct ipc_msg_node {
	uint32_t	table_id;
	uint32_t	guid;
};

struct ipc_msg_schedule {
	uint32_t	rt_table;
	uint32_t	rt_table_size;
	uint32_t	rt_n_entries;
	uint32_t	dummy[1];	/* 64 byte aligned */
};

#define SCL_IF_MAKE_RECT(w, h)		((w << 16) | h)
#define SCL_IF_GET_HEIGHT(r)		(r & 0xFFFF)
#define SCL_IF_GET_WIDTH(r)		((r >> 16) & 0xFFFF)
#define SCL_IF_MAKE_CONFIG(in_bpp, out_bpp, inter_mode, bdr_mode)	\
		((in_bpp << 24) | (out_bpp << 16) | (inter_mode << 8) | bdr_mode)
#define SCL_IF_GET_IN_BPP(cfg)		((cfg >> 24) & 0xFF)
#define SCL_IF_GET_OUT_BPP(cfg)		((cfg >> 16) & 0xFF)
#define SCL_IF_GET_INTER_MODE(cfg)	((cfg >>  8) & 0xFF)
#define SCL_IF_GET_BORDER_MODE(cfg)	((cfg)       & 0xFF)

struct scale_if_param {
	uint32_t	in_buf_pa;
	uint32_t	in_rect;	/* width and height */
	uint32_t	in_stride;
	uint32_t	out_buf_pa;
	uint32_t	out_rect;
	uint32_t	out_stride;
	uint32_t	config;
	uint32_t	const_val;
};

/**
 * pyramid of scaled images.
 * Level-0 is the full-resolution input image itself
 */
struct pyr_level {
	uint32_t	img_pa;
	uint32_t	width;
	uint32_t	height;
};

struct pyramid_param {
	/* input file information */
	uint32_t	in_frame_pa;	/* physical address for input frame*/
	int32_t		in_width;
	int32_t		in_height;
	int32_t		in_stride;

	uint32_t	max_level;
	uint32_t	inter_mode;
	uint32_t	scale_n;
	uint32_t	scale_d;
	uint32_t	border_mode;
	uint32_t	border_const;

#define SCL_PYRAMID_MAX_LEVELS (8)
	struct pyr_level pyr_info[SCL_PYRAMID_MAX_LEVELS];
};

struct warp_mesh_param {
	int32_t		width;		/* Width of the input frame */
	int32_t		height;		/* Height of the input frame */
	int32_t		stride;		/* Stride of the input frame */
	uint32_t	input_bpp;
	uint32_t	p_buf_ptr;	/* Pointer to the input buffer */
	uint32_t	p_buf_mesh_ptr;	/* Pointer to the output buffer */
	uint32_t	p_buf_out_ptr;	/* Pointer to the output buffer */
	uint32_t	out_width;	/* Width of the output frame */
	uint32_t	out_height;	/* Height of the output frame */
	uint32_t	out_stride;	/* Stride of output frame */
	uint32_t	out_tile_width;	/* Output tile width */
	uint32_t	out_tile_height;/* Output tile height */
	uint32_t	inter_mode;	/* Interpolation mode */
	uint32_t	mesh_log_space;	/* Space parameter for mesh warp */
};

struct warp_affine_param {
	/* TODO: parameter for tiling */
	int32_t		width;
	int32_t		height;
	int32_t		stride;
	uint32_t	input_bpp;
	uint32_t	p_buf_ptr;
	uint32_t	p_buf_out_ptr;
	uint32_t	out_width;
	uint32_t	out_height;
	uint32_t	out_stride;
	uint32_t	inter_mode;
	float		aff_matrix[8];
	int32_t		in_tile_xoff;
	int32_t		in_tile_yoff;
	int32_t		out_tile_xoff;
	int32_t		out_tile_yoff;
};

struct noise_filter_param {
	uint32_t	i_frame_ptr;
	uint32_t	m_frame_ptr;
	uint32_t	o_frame_ptr;
	uint32_t	config_param_ptr;
	int32_t		width;
	int32_t		height;
	int32_t		stride;
	int32_t		m_stride;
	int32_t		o_stride;
	int32_t		border_mode;
	int32_t		border_const;
};

struct corner_compute_param {
	uint32_t	i_frame_ptr;
	uint32_t	o_frame_ptr;
	int32_t		width;
	int32_t		height;
	int32_t		stride;
	int32_t		o_stride;
	float		harris_k;
	uint32_t	block_size;
	uint32_t	use_harris;
	uint32_t	output_mode;
	int32_t		border_mode;
	int32_t		border_const;
};

struct local_minmax_param {
	uint32_t	i_frame_ptr;
	uint32_t	o_array_ptr;
	uint32_t	o_cnt_ptr;
	int32_t		width;
	int32_t		height;
	int32_t		stride;
	uint32_t	min_thresh;
	uint32_t	max_thresh;
	uint32_t	output_size;
	uint32_t	input_bpp;
	uint32_t	input_signed;
	uint32_t	min_enable;
	uint32_t	max_enable;
	uint32_t	kernel_size;
	uint32_t	strict_compare;
};

struct lkt_if_param {
	/* addr */
	uint32_t	ref_frm_ptr;
	uint32_t	trk_frm_ptr;
	uint32_t	ref_point_ptr;
	uint32_t	out_point_ptr;
	/* image & canvas info */
	uint32_t	ref_width;
	uint32_t	ref_height;
	uint32_t	trk_width;
	uint32_t	trk_height;
	uint32_t	ref_left_coord;
	uint32_t	ref_right_coord;
	uint32_t	ref_top_coord;
	uint32_t	ref_bot_coord;
	uint32_t	track_left_coord;
	uint32_t	track_right_coord;
	uint32_t	track_top_coord;
	uint32_t	track_bot_coord;
	/* tile info */
	uint32_t	ft_tiles;
	uint32_t	ref_tile_xoff;
	uint32_t	ref_tile_yoff;
	uint32_t	trk_tile_xoff;
	uint32_t	trk_tile_yoff;
	/* policy */
	uint32_t	ft_win_size;
	uint32_t	ft_min_step;
	uint32_t	ft_max_iter;
	uint32_t	enable_min_step;
	uint32_t	ft_use_scharr;
	uint32_t	ft_max_level;
	uint32_t	ft_output_mode;
	uint32_t	lkt_border_mode;
	uint32_t	lkt_border_constant;
	/* ftr info */
	uint32_t	num_point_list[16];
	uint32_t	ft_num_point;
};

struct mch_linear_param {
	uint32_t	frame_id;
	uint32_t	db_description_ptr;
	uint32_t	query_description_ptr;
	uint32_t	out_ptr;
	uint32_t	query_size;
	uint32_t	db_size;
	uint32_t	dist_mode;
	uint32_t	vector_size;
	uint32_t	num_nearest;
	uint32_t	output_mode;
	uint32_t	thresh_val;
	uint32_t	thresh_on;
};

struct orb_in_param {
	int32_t		frame_id;	/* Id of the frame.*/
	int32_t		width;		/* Width of the frame */
	int32_t		height;		/* Height of the frame */
	int32_t		stride;		/* Stride of the frame */

	uint32_t	tile_width;
	uint32_t	tile_height;
	uint32_t	p_buf_ptr;/* Pointer to the input buffer */
	uint32_t	p_chroma_buf_ptr;/* Pointer to the input chroma buffer */
	uint32_t	metadata;
	uint32_t	diameter;
	uint32_t	fast_th;
	uint32_t	har_th;
	uint32_t	har_on;
	uint32_t	x_scale;
	uint32_t	y_scale;
	uint32_t	nms_size;
	uint32_t	int_fast_th;
	uint32_t	iterations;

	uint32_t	p_keypoint_ptr;
	uint32_t	p_description_ptr;
	uint32_t	p_keypoint_sorted_ptr;
};

struct wig_mfb_enf_if_param {
#define NUM_FRAMES_WME	(5)
	uint32_t	p_buf_ptr[NUM_FRAMES_WME];
	uint32_t	p_buf_matrix_ptr;
	uint32_t	p_buf_out_ptr;
	uint32_t	p_tile_info_ptr;

	uint16_t        wig_i_stride;
	uint16_t        wig_i_width;
	uint16_t        wig_i_height;
	uint16_t        wig_o_width;
	uint16_t        wig_o_height;
	uint16_t        wig_o_tile_w;
	uint16_t        wig_o_tile_h;
	uint16_t        wig_o_tile_x;
	uint16_t        bld_o_width;
	uint16_t        bld_o_height;
	uint16_t        enf_o_stride;
	uint16_t        enf_o_width;
	uint16_t        enf_o_height;
	uint32_t        wig_i_seq_size;
	uint32_t        enf_o_img_size;
	uint32_t        num_tiles;
	uint32_t        frame;
};

struct peakpower_iva20_if_param {
#define NUM_FRAMES_PEAK	(5)
	/* in buffer*/
	uint32_t	p_buf_ptr[NUM_FRAMES_PEAK];
	uint32_t	p_scl_in_ptr;
	uint32_t	p_remap_in_ptr;
	uint32_t	p_buf_matrix_ptr;
	uint32_t	p_tile_info_ptr;
	uint32_t	p_mesh_y_ptr;
	uint32_t	p_mesh_c_ptr;
	uint32_t	p_delta_ptr;
	/* out buffer*/
	uint32_t	p_enf_out_ptr;
	uint32_t	p_scl_out_ptr;
	uint32_t	p_remap_out_ptr;
};

/* Generalized command parameters */
struct gen_cmd_param {
	uint32_t	param0;
	uint32_t	param1;
	uint32_t	param2;
	uint32_t	param3;
	uint32_t	param4;
	uint32_t	param5;
	uint32_t	param6;
	uint32_t	param7;
};

struct bax_icode_cmd_param {
	uint32_t	icode_bin_addr;
	uint32_t	in_addr_1;
	uint32_t	in_addr_2;
	uint32_t	out_addr_1;
	uint32_t	out_addr_2;
};

struct bax_jacobian_cmd_param {
	uint32_t	icode_bin_addr;

	uint32_t	c_data_input_addr;
	uint32_t	scalefactor_input_addr;
	uint32_t	p_data_input_addr;
	uint32_t	cfg_input1_addr;

	uint32_t	param_input1_addr;
	uint32_t	param_input2_addr;
	uint32_t	const_input1_addr;
	uint32_t	const_input2_addr;

	uint32_t	a_data_output_addr;
	uint32_t	b_data_output_addr;
	uint32_t	dbg_data_addr;
};

struct bax_hessian_cmd_param {
	uint32_t	icode_bin_addr;

	uint32_t	udata_input1_addr;
	uint32_t	vdata_input1_addr;

	uint32_t	a_sh_dram_addr;
	uint32_t	b_sh_dram_addr;

	uint32_t	const_input1_addr;
	uint32_t	const_input2_addr;

	uint32_t	damping1_addr;
	uint32_t	damping2_addr;

	uint32_t	cfg_input1_addr;
	uint32_t	cfg_input2_addr;

	uint32_t	param_input1_addr;
	uint32_t	param_input2_addr;

	uint32_t	dbg_data_addr;

	uint32_t	u_output_addr;
	uint32_t	v_output_addr;
	uint32_t	vinv_output_addr;
};

/* DSP_IPC input/out structure */
/* SCQ max request size (in word) */
#ifdef CONFIG_TEST_PARAM
#define MAX_REQUEST_SIZE_IN_WORD                   (26)
#else
#define MAX_REQUEST_SIZE_IN_WORD                   (512)
#endif

/*  SCQ max request size (in byte) */
#define MAX_REQUEST_SIZE                           (MAX_REQUEST_SIZE_IN_WORD<<2)
#define MAX_PAYLOAD_SIZE (MAX_REQUEST_SIZE_IN_WORD - (sizeof(struct _sc_packet_header_t) >> 2)) ///<(in words)

/* IPC packet header contains meta information for sending payload */
struct _sc_packet_header_t {
	unsigned int	next_type:	8;	///< Packet type of following payload
	unsigned int	src:		5;	///< Source device id
	unsigned int	dst:		5;	///< Destination device id
	unsigned int	version:	5;	///< Version of SCore
	unsigned int	reserved:	9;	///< RESERVED
};

struct _sc_kernel_packet_header_t {
	unsigned int	next_type:	4;	///< Packet type of following payload
	unsigned int	size:		11;	///< Size of sc_kernel_packet
	unsigned int	kernel_id:	9;	///< Packet type of current payload
	unsigned int	task_id:	8;	///< ID of the task requested
};

struct _sc_result_packet_header_t {
	unsigned int next_type:	4;	///< Packet type of following payload
	unsigned int size:	11;	///< Size of sc_result_packet
	unsigned int kernel_id:	9;	///< ID of ther kernel returned
	unsigned int task_id:	8;	///< ID of the task returned
	unsigned int ret_code;
};

/* IPC packet struct. it defines whole a packet structure */
struct dsp_ipc_params {
	struct _sc_packet_header_t		packet_header;
	struct _sc_kernel_packet_header_t	kernel_header;
	unsigned int				payload[MAX_PAYLOAD_SIZE];
};

#ifdef IVA_IPC_PARAM_ARCH64_SUPPORT
struct cmd_align_param {
#ifdef CONFIG_TEST_PARAM
	#define	MAX_CMD_PARAM_SIZE	(0xD0)		/* 208 B */
#else
	#define	MAX_CMD_PARAM_SIZE	(0x8)		/* 8 B */
#endif
	uint32_t	reserved[MAX_CMD_PARAM_SIZE/sizeof(uint32_t)];
};
#endif


/* definition for output params */
struct mch_out_param {
	uint32_t	result_size;
};

struct lkt_out_param {
	uint32_t	trk_point_ptr;
	int32_t		trk_num_point;
};

struct lmd_out_param {
	uint32_t	cnt_result;
};

struct orb_out_param {
	uint32_t	frame_id;
	uint32_t	num_keypoints;
};

#ifdef IVA_IPC_PARAM_ARCH64_SUPPORT
struct res_align_param {
	#define	MAX_RES_PARAM_SIZE	(0x8)		/* 8 B */
	uint32_t	reserved[MAX_RES_PARAM_SIZE/sizeof(uint32_t)];
};
#endif

/*
 * data for command parameters
 *  LIMITATION: do not use explicit byte- or word-typed member inside.
 */
union ipc_cmd_data {
	struct ipc_msg_schedule		p;
	struct ipc_msg_node		m;
#ifdef IVA_IPC_PARAM_ARCH64_SUPPORT
	struct cmd_align_param		align_param;	/* do not exceed */
#endif
#ifdef SCL_IF_PARAM_TO_CMD_PARAM
	struct scale_if_param		scale_param;
#endif
#ifdef PYRAMID_PARAM_TO_CMD_PARAM
	struct pyramid_param		pyrmd_param;
#endif
#ifdef WARP_MESH_PARAM_TO_CMD_PARAM
	struct warp_mesh_param		mesh_param;
#endif
#ifdef WARP_AFFINE_PARAM_TO_CMD_PARAM
	struct warp_affine_param	aff_param;
#endif
#ifdef NOISE_FILTER_PARAM_TO_CMD_PARAM
	struct noise_filter_param	nf_param;
#endif
#ifdef CORNER_COMP_PARAM_TO_CMD_PARAM
	struct corner_compute_param	cc_param;
#endif
#ifdef LOCAL_MINMAX_PARAM_TO_CMD_PARAM
	struct local_minmax_param	lm_param;
#endif
#ifdef LKT_IF_PARAM_TO_CMD_PARAM
	struct lkt_if_param		lkt_param;
#endif
#ifdef MCH_LINEAR_PARAM_TO_CMD_PARAM
	struct mch_linear_param		mch_param;
#endif
#ifdef ORB_IN_PARAM_TO_CMD_PARAM
	struct orb_in_param		orb_param;
#endif
#ifdef DSP_IPC_PARAM_TO_CMD_PARAM
	struct dsp_ipc_params		score_param;
#endif
#ifdef GEN_IPC_PARAM_TO_CMD_PARAM
	struct gen_cmd_param		cmd_param;
#endif
#ifdef WIG_MFB_ENF_IPC_PARAM_TO_CMD_PARAM
	struct wig_mfb_enf_if_param	wig_mfb_enf_param;
#endif
#ifdef BAX_ICODE_IPC_PARAM_TO_CMD_PARAM
	struct bax_icode_cmd_param	bax_icode_param;
#endif
#ifdef BAX_JACOBIAN_IPC_PARAM_TO_CMD_PARAM
	struct bax_jacobian_cmd_param	bax_jacobian_param;
	struct bax_hessian_cmd_param	bax_hessian_param;
#endif
#ifdef PEAKPOWER_IPC_PARAM_TO_CMD_PARAM
	struct peakpower_iva20_if_param	peakpower_param;
#endif
};

/*
 * data for response parameters
 *  LIMITATION: do not use explicit byte- or word-typed member inside.
 */
union ipc_res_data {
	struct ipc_msg_node	m;

#ifdef IVA_IPC_PARAM_ARCH64_SUPPORT
	struct res_align_param	align_param;	/* do not exceed */
#endif
	/* add from here */
#ifdef MCH_OUT_PARAM_TO_RES_PARAM
	struct mch_out_param	mch_param;
#endif
#ifdef LMD_OUT_PARAM_TO_RES_PARAM
	struct lmd_out_param	lmd_param;
#endif
#ifdef LKT_OUT_PARAM_TO_RES_PARAM
	struct lkt_out_param	lkt_out_param;
#endif
#ifdef ORB_OUT_PARAM_TO_RES_PARAM
	struct orb_out_param	orb_param;
#endif
#ifdef TIME_TO_RES_PARAM
	uint32_t		time;
#endif
};

struct ipc_cmd_param {
	uint32_t		header;
	uint16_t		ipc_cmd_type;
	uint16_t		extra;
	union {
		uint32_t	cmd_src;
		uint32_t	param_0;
	};
	union {
		uint32_t	cmd_id;
		uint32_t	param_1;
	};
	union ipc_cmd_data	data;
};

struct ipc_res_param {
	uint32_t		header;
	uint16_t		ipc_cmd_type;
	uint16_t		extra;
	union {
		uint32_t	rep_id;		/* match with cmd id */
		uint32_t	param_0;
	};
	union {
		uint32_t	ret;		/* success or failure */
		uint32_t	param_1;
	};
	union ipc_res_data	data;
};

#endif //__IVA_IPC_PARAM_H__
