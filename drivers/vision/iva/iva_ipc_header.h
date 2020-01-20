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

#ifndef __IVA_IPC_HEADER_H__
#define __IVA_IPC_HEADER_H__


/* TO DO: do redefine for future use. as of now, this is used for temporarily */

/*
 *  31        24         16          8
 *  +----------+----------+----------+----------+
 *  |   func   | sub-func |        extra        |
 *  +----------+----------+----------+----------+
 */

#define IPC_HDR_FUNC_MSK	(0xFF)
#define IPC_HDR_FUNC_SFT	(24)
#define IPC_HDR_SUBFUNC_MSK	(0xFF)
#define IPC_HDR_SUBFUNC_SFT	(16)
#define IPC_HDR_EXTRA_MSK	(0xFFFF)
#define IPC_HDR_EXTRA_SFT	(0)

/* function definition */
enum ipc_func {
	ipc_func_test_mode	= 0,
	ipc_func_kernel,
	/* add any additional kernel test here */
	ipc_func_bringup,
	ipc_func_sched_table,
	ipc_func_iva_ctrl,
	ipc_func_max
};

enum ipc_test_mode_id {
	test_subf_nop		= 0,	/* nop */
	test_subf_stop,			/* make the system stop */
	test_subf_response,		/* make dummy ipc response*/
	test_subf_fatal,		/* make the system dead */
	test_subf_score,		/* make score ipc test */

	test_subf_systick_freq	= 5,	/* get systick frequency */
	test_subf_mem_cpy,		/* copy memory */
	test_subf_max
};

enum ipc_kern_id {
	ipc_kernel_none			= -1,
	ipc_kernel_scale		= 0,
	ipc_kernel_pyramid,
	ipc_kernel_noisefilter,
	ipc_kernel_postproc_luma,
	ipc_kernel_affine,
	ipc_kernel_lmd_frame		= 5,
	ipc_kernel_corner_calculate,
	ipc_kernel_optflow_lk,
	ipc_kernel_remap_frame,
	ipc_kernel_matching,

	ipc_kernel_orb			= 10,
	ipc_kernel_affine_frame,
	ipc_kernel_affine_tile,
	ipc_kernel_corner_to_track,
	ipc_kernel_lmd_tile,
	ipc_kernel_feature_track_vcm	= 15,
	ipc_kernel_mcu_lktracker_tile,
	ipc_kernel_mcu_lktracker_win,
	ipc_kernel_remap_tile,
	ipc_kernel_wig_mfb_enf,

	ipc_kernel_bax_icode 		= 20,
	ipc_kernel_bax_jacobian,
	ipc_kernel_bax_hessian,

	ipc_kernel_iva_to_score 	= 30,
	ipc_kernel_iva_to_score_ddr,
	ipc_kernel_iva_to_score_scq,
	ipc_kernel_iva_to_score_cbci,
	ipc_kernel_iva_to_score_scl_cbci,

	ipc_kernel_vcm20_simple 	= 40,

	ipc_kernel_max,
	/* others */
};

enum ipc_kern_score_id {
	ipc_kernel_score_and	= 100,
	ipc_kernel_score_mbox,
	ipc_kernel_score_rt,
	ipc_kernel_score_sif,
	ipc_kernel_score_max
};

enum ipc_bringup_id {
	ipc_bringup_iva_clock		= 0,
	ipc_bringup_perform_vdma,
	ipc_bringup_perf_hwa_enf,
	ipc_bringup_si_hwa_wig_vcm,
	ipc_bringup_gp_int,
	ipc_bringup_si_peak_power,
	ipc_bringup_perf_streaming_if,
	ipc_bringup_iva_score_if,
	ipc_bringup_iva_scaler_asb,
#if defined(TARGET_SOC_VER_20) || defined(TARGET_SOC_VER_30)
	ipc_bringup_wig_mfb_enf,
	ipc_bringup_perf_hwa_enf20,
	ipc_bringup_peakpower_iva20,
	ipc_bringup_mmr_rw,
	ipc_bringup_mmr_memmap,
#endif
	ipc_bringup_max
};

/* this is for internal communication with iva drirver */
enum ipc_iva_ctrl_id {
	ipc_iva_ctrl_release_wait	= 0,
	ipc_iva_ctrl_dump_sfrs,
	ipc_iva_ctrl_max
};

enum iva_optflow_pyrlk_queue_chain {
	ipc_csc_lmd_lkt		= 3,
	ipc_scl_csc_lmd_lkt,
	ipc_scl_csc_lmd_dsp_lkt,
	/* others */
};

/* It has to be synchronized with corresponding enum in iva_fw. */
enum ipc_cmd_type {
	IPC_CMD_NONE		= 0,
	IPC_CMD_INIT,
	IPC_CMD_DEINIT,
	IPC_CMD_PROFILE_START,

	IPC_CMD_SEND_TABLE	= 10,
	IPC_CMD_SEND_TABLE_ACK,
	IPC_CMD_EVICT_TABLE,
	IPC_CMD_EVICT_TABLE_ACK,

	IPC_CMD_FRAME_START	= 20,	/* Used for cross-target communication */
	IPC_CMD_NODE_START,		/* Used for cross-target communication */
	IPC_CMD_FRAME_DONE,		/* Used for cross-target communication */
	IPC_CMD_NODE_DONE,		/* Used for cross-target communication */
	IPC_CMD_DSP_DONE,
	IPC_CMD_ABANDON_NOTIFY,		/* Abandon Notify */

	IPC_CMD_IVA_MAX,

	/* for error handling */
	IPC_CMD_ERR_BASE	= 30,
	IPC_CMD_TABLE_ERR_NOTIFY= IPC_CMD_ERR_BASE,/* iva rt error on tsst */
	IPC_CMD_SYS_ERR_NOTIFY,		/* iva crash */
	IPC_CMD_TIMEOUT_NOTIFY,		/* ipc timeout */
	IPC_CMD_SIGNAL_NOTIFY,		/* received kill signal */
	IPC_CMD_UNKNOWN_NOTIFY,		/* unknown error */
	IPC_CMD_SYSMMU_FAULT_NOTIFY,	/* sysmmu fault */
	IPC_CMD_ERR_MAX,

	IPC_CMD_MAX
};

#define IPC_MK_HDR(func, subf, extra)	\
		(((func & IPC_HDR_FUNC_MSK) << IPC_HDR_FUNC_SFT) |\
		((subf & IPC_HDR_SUBFUNC_MSK) << IPC_HDR_SUBFUNC_SFT) |\
		((extra & IPC_HDR_EXTRA_MSK) << IPC_HDR_EXTRA_SFT))

#define IPC_GET_FUNC(header)	\
		((header >> IPC_HDR_FUNC_SFT) & IPC_HDR_FUNC_MSK)

#define IPC_GET_SUB_FUNC(header)	\
		((header >> IPC_HDR_SUBFUNC_SFT) & IPC_HDR_SUBFUNC_MSK)

#define IPC_GET_EXTRA(header)	\
		((header >> IPC_HDR_EXTRA_SFT) & IPC_HDR_EXTRA_MSK)

#endif // __IVA_IPC_HEADER_H__
