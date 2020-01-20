/*
 * drivers/media/platform/exynos/mfc/mfc_data_struct.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_DATA_STRUCT_H
#define __MFC_DATA_STRUCT_H __FILE__

#ifdef CONFIG_ARM_EXYNOS_DEVFREQ
#define CONFIG_MFC_USE_BUS_DEVFREQ
#endif

#ifdef CONFIG_MFC_USE_BUS_DEVFREQ
#include <linux/pm_qos.h>
#endif
#ifdef CONFIG_EXYNOS_BTS
#include <soc/samsung/bts.h>
#endif
#include <linux/videodev2.h>
#ifdef CONFIG_EXYNOS_ITMON
#include <soc/samsung/exynos-itmon.h>
#endif

#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>

#include "exynos_mfc_media.h"

#define MFC_NUM_CONTEXTS		32
#define MFC_MAX_PLANES			3
#define MFC_MAX_DPBS			32
#define MFC_MAX_BUFFERS			32
#define MFC_MAX_EXTRA_BUF		10
#define MFC_TIME_INDEX			15
#define MFC_SFR_LOGGING_COUNT_SET1	4
#define MFC_SFR_LOGGING_COUNT_SET2	23
#define MFC_LOGGING_DATA_SIZE		256
#define MFC_MAX_DEFAULT_PARAM		100

#define HWFC_MAX_BUF			10
#define OTF_MAX_BUF			30

#define HDR_MAX_WINDOWS			3
#define HDR_MAX_SCL			3
#define HDR_MAX_DISTRIBUTION		15
#define HDR_MAX_BEZIER_CURVES		15

/* Maximum number of temporal layers */
#define VIDEO_MAX_TEMPORAL_LAYERS	7

#define MAX_NUM_IMAGES_IN_VB		8
#define MAX_NUM_BUFCON_BUFS		32
#define MAX_NUM_CLUSTER			3

/*
 *  MFC region id for smc
 */
enum {
	FC_MFC_EXYNOS_ID_MFC_SH        = 0,
	FC_MFC_EXYNOS_ID_VIDEO         = 1,
	FC_MFC_EXYNOS_ID_MFC_FW        = 2,
	FC_MFC_EXYNOS_ID_SECTBL        = 3,
	FC_MFC_EXYNOS_ID_G2D_WFD       = 4,
	FC_MFC_EXYNOS_ID_MFC_NFW       = 5,
	FC_MFC_EXYNOS_ID_VIDEO_EXT     = 6,
};

/**
 * enum mfc_inst_type - The type of an MFC device node.
 */
enum mfc_node_type {
	MFCNODE_INVALID = -1,
	MFCNODE_DECODER = 0,
	MFCNODE_ENCODER = 1,
	MFCNODE_DECODER_DRM = 2,
	MFCNODE_ENCODER_DRM = 3,
	MFCNODE_ENCODER_OTF = 4,
	MFCNODE_ENCODER_OTF_DRM = 5,
};

/**
 * enum mfc_inst_type - The type of an MFC instance.
 */
enum mfc_inst_type {
	MFCINST_INVALID = 0,
	MFCINST_DECODER = 1,
	MFCINST_ENCODER = 2,
};

/**
 * enum mfc_inst_state - The state of an MFC instance.
 */
enum mfc_inst_state {
	MFCINST_FREE = 0,
	MFCINST_INIT = 100,
	MFCINST_GOT_INST,
	MFCINST_HEAD_PARSED,
	MFCINST_RUNNING_BUF_FULL,
	MFCINST_RUNNING,
	MFCINST_FINISHING,
	MFCINST_RETURN_INST,
	MFCINST_ERROR,
	MFCINST_ABORT,
	MFCINST_RES_CHANGE_INIT,
	MFCINST_RES_CHANGE_FLUSH,
	MFCINST_RES_CHANGE_END,
	MFCINST_FINISHED,
	MFCINST_ABORT_INST,
	MFCINST_DPB_FLUSHING,
	MFCINST_SPECIAL_PARSING,
	MFCINST_SPECIAL_PARSING_NAL,
};

/**
 * enum mfc_queue_state - The state of buffer queue.
 */
enum mfc_queue_state {
	QUEUE_FREE = 0,
	QUEUE_BUFS_REQUESTED,
	QUEUE_BUFS_QUERIED,
	QUEUE_BUFS_MMAPED,
};

enum mfc_dec_wait_state {
	WAIT_NONE	= 0,
	WAIT_G_FMT	= (1 << 0),
	WAIT_STOP	= (1 << 1),
};

/**
 * enum mfc_check_state - The state for user notification
 */
enum mfc_check_state {
	MFCSTATE_PROCESSING = 0,
	MFCSTATE_DEC_RES_DETECT,
	MFCSTATE_DEC_TERMINATING,
	MFCSTATE_ENC_NO_OUTPUT,
	MFCSTATE_DEC_S3D_REALLOC,
};

enum mfc_buf_usage_type {
	MFCBUF_INVALID = 0,
	MFCBUF_NORMAL,
	MFCBUF_DRM,
	MFCBUF_NORMAL_FW,
	MFCBUF_DRM_FW,
};

enum mfc_buf_process_type {
	MFCBUFPROC_DEFAULT		= 0x0,
	MFCBUFPROC_COPY			= (1 << 0),
	MFCBUFPROC_SHARE		= (1 << 1),
	MFCBUFPROC_META			= (1 << 2),
	MFCBUFPROC_ANBSHARE		= (1 << 3),
	MFCBUFPROC_ANBSHARE_NV12L	= (1 << 4),
};

enum mfc_ctrl_type {
	MFC_CTRL_TYPE_GET_SRC	= 0x1,
	MFC_CTRL_TYPE_GET_DST	= 0x2,
	MFC_CTRL_TYPE_SET	= 0x4,
};

enum mfc_ctrl_mode {
	MFC_CTRL_MODE_NONE	= 0x0,
	MFC_CTRL_MODE_SFR	= 0x1,
	MFC_CTRL_MODE_CST	= 0x2,
};

enum mfc_vb_flag {
	MFC_FLAG_HDR_CONTENT_LIGHT	= 0,
	MFC_FLAG_HDR_DISPLAY_COLOUR	= 1,
	MFC_FLAG_HDR_MAXTIX_COEFF	= 2,
	MFC_FLAG_HDR_COLOUR_DESC	= 3,
	MFC_FLAG_HDR_VIDEO_SIGNAL_TYPE	= 4,
	MFC_FLAG_BLACKBAR_DETECT	= 5,
	MFC_FLAG_HDR_PLUS		= 6,
	MFC_FLAG_CSD			= 29,
	MFC_FLAG_EMPTY_DATA		= 30,
	MFC_FLAG_LAST_FRAME		= 31,
};

struct mfc_ctx;

enum mfc_debug_cause {
	MFC_CAUSE_0WRITE_PAGE_FAULT		= 0,
	MFC_CAUSE_0READ_PAGE_FAULT		= 1,
	MFC_CAUSE_1WRITE_PAGE_FAULT		= 2,
	MFC_CAUSE_1READ_PAGE_FAULT		= 3,
	MFC_CAUSE_NO_INTERRUPT			= 4,
	MFC_CAUSE_NO_SCHEDULING			= 5,
	MFC_CAUSE_FAIL_STOP_NAL_Q		= 6,
	MFC_CAUSE_FAIL_STOP_NAL_Q_FOR_OTHER	= 7,
	MFC_CAUSE_FAIL_CLOSE_INST		= 8,
	MFC_CAUSE_FAIL_SLEEP			= 9,
	MFC_CAUSE_FAIL_WAKEUP			= 10,
	MFC_CAUSE_FAIL_RISC_ON			= 11,
	MFC_CAUSE_FAIL_DPB_FLUSH		= 12,
	MFC_CAUSE_FAIL_CHACHE_FLUSH		= 13,
};

struct mfc_debug {
	u32	cause;
	u8	fault_status;
	u32	fault_trans_info;
	u32	fault_addr;
	u8	SFRs_set1[MFC_SFR_LOGGING_COUNT_SET1];
	u32	SFRs_set2[MFC_SFR_LOGGING_COUNT_SET2];
	char	errorinfo[MFC_LOGGING_DATA_SIZE];
};

/**
 * struct mfc_buf - MFC buffer
 *
 */
struct mfc_buf {
	struct vb2_v4l2_buffer vb;
	struct list_head list;
	dma_addr_t addr[MAX_NUM_IMAGES_IN_VB][MFC_MAX_PLANES];
	struct dma_buf *dmabufs[MAX_NUM_IMAGES_IN_VB][MFC_MAX_PLANES];
	struct dma_buf_attachment *attachments[MAX_NUM_IMAGES_IN_VB][MFC_MAX_PLANES];
	int next_index;
	int done_index;
	int used;
	int num_bufs_in_batch;
	int num_valid_bufs;
	unsigned char *vir_addr;
};

struct mfc_buf_queue {
	struct list_head head;
	unsigned int count;
};

struct mfc_bits {
	unsigned long bits;
	spinlock_t lock;
};

struct mfc_hwlock {
	struct list_head waiting_list;
	unsigned int wl_count;
	unsigned long bits;
	unsigned long dev;
	unsigned int owned_by_irq;
	unsigned int transfer_owner;
	spinlock_t lock;
};

struct mfc_listable_wq {
	struct list_head list;
	wait_queue_head_t wait_queue;
	struct mutex wait_mutex;
	struct mfc_dev *dev;
	struct mfc_ctx *ctx;
};

struct mfc_pm {
	struct clk	*clock;
	atomic_t	pwr_ref;
	struct device	*device;
	spinlock_t	clklock;

	int clock_on_steps;
	int clock_off_steps;
	enum mfc_buf_usage_type base_type;
};

struct mfc_fw {
	int		date;
	int		fimv_info;
	size_t		size;
	int		status;
	int		drm_status;
};

struct mfc_ctx_buf_size {
	size_t dev_ctx;
	size_t h264_dec_ctx;
	size_t other_dec_ctx;
	size_t h264_enc_ctx;
	size_t hevc_enc_ctx;
	size_t other_enc_ctx;
	size_t shared_buf;
	size_t dbg_info_buf;
};

struct mfc_buf_size {
	size_t firmware_code;
	unsigned int cpb_buf;
	void *ctx_buf;
};

struct mfc_variant {
	struct mfc_buf_size *buf_size;
	int	num_entities;
};

enum mfc_sfr_dump_type {
	MFC_DUMP_NONE			= 0,
	MFC_DUMP_DEC_SEQ_START		= (1 << 0),
	MFC_DUMP_DEC_INIT_BUFS		= (1 << 1),
	MFC_DUMP_DEC_NAL_START		= (1 << 2),
	MFC_DUMP_ENC_SEQ_START		= (1 << 3),
	MFC_DUMP_ENC_INIT_BUFS		= (1 << 4),
	MFC_DUMP_ENC_NAL_START		= (1 << 5),
	MFC_DUMP_ERR_INT		= (1 << 6),
	MFC_DUMP_WARN_INT		= (1 << 7),
};

struct mfc_debugfs {
	struct dentry *root;
	struct dentry *mfc_info;
	struct dentry *debug_info;
#ifdef CONFIG_MFC_REG_TEST
	struct dentry *reg_info;
	struct dentry *reg_test;
#endif
	struct dentry *debug_level;
	struct dentry *debug_ts;
	struct dentry *dbg_enable;
	struct dentry *nal_q_dump;
	struct dentry *nal_q_disable;
	struct dentry *nal_q_parallel_disable;
	struct dentry *otf_dump;
	struct dentry *perf_measure_option;
	struct dentry *sfr_dump;
	struct dentry *mmcache_dump;
	struct dentry *mmcache_disable;
	struct dentry *perf_boost_mode;
};

/**
 * struct mfc_special_buf - represents internal used buffer
 * @daddr:		device virtual address
 * @virt:		kernel virtual address, only valid when the
 *			buffer accessed by driver
 */
struct mfc_special_buf {
	enum mfc_buf_usage_type	buftype;
	struct dma_buf			*dma_buf;
	struct dma_buf_attachment	*attachment;
	struct sg_table			*sgt;
	dma_addr_t			daddr;
	void				*vaddr;
	size_t				size;
};

#ifdef CONFIG_EXYNOS_BTS
struct mfc_bw_data {
	unsigned int	peak;
	unsigned int	read;
	unsigned int	write;
};

struct mfc_bw_info {
	struct mfc_bw_data bw_enc_h264;
	struct mfc_bw_data bw_enc_hevc;
	struct mfc_bw_data bw_enc_hevc_10bit;
	struct mfc_bw_data bw_enc_vp8;
	struct mfc_bw_data bw_enc_vp9;
	struct mfc_bw_data bw_enc_vp9_10bit;
	struct mfc_bw_data bw_enc_mpeg4;
	struct mfc_bw_data bw_dec_h264;
	struct mfc_bw_data bw_dec_hevc;
	struct mfc_bw_data bw_dec_hevc_10bit;
	struct mfc_bw_data bw_dec_vp8;
	struct mfc_bw_data bw_dec_vp9;
	struct mfc_bw_data bw_dec_vp9_10bit;
	struct mfc_bw_data bw_dec_mpeg4;
};
#endif

#ifdef CONFIG_MFC_USE_BUS_DEVFREQ
/*
 * threshold_mb - threshold of total MB(macroblock) count
 * Total MB count can be calculated by
 *	(MB of width) * (MB of height) * fps
 */
struct mfc_qos {
	unsigned int threshold_mb;
	unsigned int freq_mfc;
	unsigned int freq_int;
	unsigned int freq_mif;
	unsigned int mo_value;
	unsigned int mo_10bit_value;
	unsigned int mo_uhd_enc60_value;
	unsigned int time_fw;
};

struct mfc_qos_boost {
	unsigned int num_cluster;
	unsigned int freq_mfc;
	unsigned int freq_int;
	unsigned int freq_mif;
	unsigned int freq_cluster[MAX_NUM_CLUSTER];
};

struct mfc_qos_weight {
	unsigned int weight_h264_hevc;
	unsigned int weight_vp8_vp9;
	unsigned int weight_other_codec;
	unsigned int weight_3plane;
	unsigned int weight_10bit;
	unsigned int weight_422;
	unsigned int weight_bframe;
	unsigned int weight_num_of_ref;
	unsigned int weight_gpb;
	unsigned int weight_num_of_tile;
	unsigned int weight_super64_bframe;
};
#endif

struct mfc_feature {
	unsigned int support;
	unsigned int version;
};

struct mfc_platdata {
	/* MFC version */
	unsigned int ip_ver;
	/* Debug mode */
	unsigned int debug_mode;
	/* Sysmmu check */
	unsigned int share_sysmmu;
	unsigned int axid_mask;
	unsigned int mfc_fault_num;
	/* Default 10bit format for decoding */
	unsigned int P010_decoding;
	/* Formats */
	unsigned int support_10bit;
	unsigned int support_422;
	unsigned int support_rgb;
	/* HDR10+ */
	unsigned int max_hdr_win;
#ifdef CONFIG_MFC_USE_BUS_DEVFREQ
	/* QoS */
	unsigned int num_qos_steps;
	unsigned int max_qos_steps;
	unsigned int max_mb;
	unsigned int mfc_freq_control;
	unsigned int mo_control;
	unsigned int bw_control;
	struct mfc_qos *qos_table;
	struct mfc_qos_boost *qos_boost_table;
#endif
	/* NAL-Q size */
	unsigned int nal_q_entry_size;
	unsigned int nal_q_dump_size;
	/* Features */
	struct mfc_feature nal_q;
	struct mfc_feature skype;
	struct mfc_feature black_bar;
	struct mfc_feature color_aspect_dec;
	struct mfc_feature static_info_dec;
	struct mfc_feature color_aspect_enc;
	struct mfc_feature static_info_enc;
	struct mfc_feature hdr10_plus;

	/*
	 * new variables should be added above
	 * ============ boundary line ============
	 * The following variables are excluded from the MFC log dumps
	 */

	/* Encoder default parameter */
	unsigned int enc_param_num;
	unsigned int enc_param_addr[MFC_MAX_DEFAULT_PARAM];
	unsigned int enc_param_val[MFC_MAX_DEFAULT_PARAM];

#ifdef CONFIG_EXYNOS_BTS
	struct mfc_bw_info mfc_bw_info;
#endif
#ifdef CONFIG_MFC_USE_BUS_DEVFREQ
	struct mfc_qos_weight qos_weight;
#endif
};

/************************ NAL_Q data structure ************************/
#define NAL_Q_ENTRY_SIZE_FOR_HDR10	512

/* slot 4 * max instance 32 = 128 */
#define NAL_Q_QUEUE_SIZE		128

typedef struct __DecoderInputStr {
	int StartCode; /* = 0xAAAAAAAA; Decoder input structure marker */
	int CommandId;
	int InstanceId;
	int PictureTag;
	unsigned int CpbBufferAddr;
	int CpbBufferSize;
	int CpbBufferOffset;
	int StreamDataSize;
	int AvailableDpbFlagUpper;
	int AvailableDpbFlagLower;
	int DynamicDpbFlagUpper;
	int DynamicDpbFlagLower;
	unsigned int FrameAddr[3];
	int FrameSize[3];
	int NalStartOptions;
	int FrameStrideSize[3];
	int Frame2BitSize[2];
	int Frame2BitStrideSize[2];
	unsigned int ScratchBufAddr;
	int ScratchBufSize;
} DecoderInputStr; /* 28*4 = 112 bytes */

typedef struct __EncoderInputStr {
	int StartCode; /* 0xBBBBBBBB; Encoder input structure marker */
	int CommandId;
	int InstanceId;
	int PictureTag;
	unsigned int FrameAddr[3];
	unsigned int StreamBufferAddr;
	int StreamBufferSize;
	int StreamBufferOffset;
	int RcRoiCtrl;
	unsigned int RoiBufferAddr;
	int ParamChange;
	int IrSize;
	int GopConfig;
	int RcFrameRate;
	int RcBitRate;
	int MsliceMode;
	int MsliceSizeMb;
	int MsliceSizeBits;
	int FrameInsertion;
	int HierarchicalBitRateLayer[7];
	int H264RefreshPeriod;
	int HevcRefreshPeriod;
	int RcQpBound;
	int RcQpBoundPb;
	int FixedPictureQp;
	int PictureProfile;
	int BitCountEnable;
	int MaxBitCount;
	int MinBitCount;
	int NumTLayer;
	int H264NalControl;
	int HevcNalControl;
	int Vp8NalControl;
	int Vp9NalControl;
	int H264HDSvcExtension0;
	int H264HDSvcExtension1;
	int GopConfig2;
	int Frame2bitAddr[2];
	int Weight;
	int ExtCtbQpAddr;
	int WeightUpper;
	int RcMode;
	int St2094_40sei[30];
} EncoderInputStr; /* 81*4 = 324 bytes */

typedef struct __DecoderOutputStr {
	int StartCode; /* 0xAAAAAAAA; Decoder output structure marker */
	int CommandId;
	int InstanceId;
	int ErrorCode;
	int PictureTagTop;
	int PictureTimeTop;
	int DisplayFrameWidth;
	int DisplayFrameHeight;
	int DisplayStatus;
	unsigned int DisplayAddr[3];
	int DisplayFrameType;
	int DisplayCropInfo1;
	int DisplayCropInfo2;
	int DisplayPictureProfile;
	int DisplayAspectRatio;
	int DisplayExtendedAr;
	int DecodedNalSize;
	int UsedDpbFlagUpper;
	int UsedDpbFlagLower;
	int SeiAvail;
	int FramePackArrgmentId;
	int FramePackSeiInfo;
	int FramePackGridPos;
	int DisplayRecoverySeiInfo;
	int H264Info;
	int DisplayFirstCrc;
	int DisplaySecondCrc;
	int DisplayThirdCrc;
	int DisplayFirst2BitCrc;
	int DisplaySecond2BitCrc;
	int DecodedFrameWidth;
	int DecodedFrameHeight;
	int DecodedStatus;
	unsigned int DecodedAddr[3];
	int DecodedFrameType;
	int DecodedCropInfo1;
	int DecodedCropInfo2;
	int DecodedPictureProfile;
	int DecodedRecoverySeiInfo;
	int DecodedFirstCrc;
	int DecodedSecondCrc;
	int DecodedThirdCrc;
	int DecodedFirst2BitCrc;
	int DecodedSecond2BitCrc;
	int PictureTagBot;
	int PictureTimeBot;
	int ChromaFormat;
	int Mpeg4Info;
	int HevcInfo;
	int Vc1Info;
	int VideoSignalType;
	int ContentLightLevelInfoSei;
	int MasteringDisplayColourVolumeSei0;
	int MasteringDisplayColourVolumeSei1;
	int MasteringDisplayColourVolumeSei2;
	int MasteringDisplayColourVolumeSei3;
	int MasteringDisplayColourVolumeSei4;
	int MasteringDisplayColourVolumeSei5;
	int FirstPlaneDpbSize;
	int SecondPlaneDpbSize;
	int St2094_40sei[30];
} DecoderOutputStr; /* 94*4 =  376 bytes */

typedef struct __EncoderOutputStr {
	int StartCode; /* 0xBBBBBBBB; Encoder output structure marker */
	int CommandId;
	int InstanceId;
	int ErrorCode;
	int PictureTag;
	unsigned int EncodedFrameAddr[3];
	unsigned int StreamBufferAddr;
	int StreamBufferOffset;
	int StreamSize;
	int SliceType;
	int NalDoneInfo;
	unsigned int ReconLumaDpbAddr;
	unsigned int ReconChromaDpbAddr;
	int EncCnt;
} EncoderOutputStr; /* 16*4 = 64 bytes */

/**
 * enum nal_queue_state - The state for nal queue operation.
 */
typedef enum _nal_queue_state {
	NAL_Q_STATE_CREATED = 0,
	NAL_Q_STATE_STARTED, /* when mfc_nal_q_start() is called */
	NAL_Q_STATE_STOPPED, /* when mfc_nal_q_stop() is called */
} nal_queue_state;

struct _nal_queue_handle;
typedef struct _nal_queue_in_handle {
	struct _nal_queue_handle *nal_q_handle;
	struct mfc_special_buf in_buf;
	unsigned int in_exe_count;
	void *nal_q_in_addr;
} nal_queue_in_handle;

typedef struct _nal_queue_out_handle {
	struct _nal_queue_handle *nal_q_handle;
	struct mfc_special_buf out_buf;
	unsigned int out_exe_count;
	void *nal_q_out_addr;
	int nal_q_ctx;
} nal_queue_out_handle;

typedef struct _nal_queue_handle {
	nal_queue_in_handle *nal_q_in_handle;
	nal_queue_out_handle *nal_q_out_handle;
	nal_queue_state nal_q_state;
	unsigned int nal_q_clk_cnt;
	spinlock_t lock;
	int nal_q_exception;
} nal_queue_handle;

/************************ OTF data structure ************************/
struct _otf_buf_addr {
	dma_addr_t otf_daddr[HWFC_MAX_BUF][3];
	struct dma_buf_attachment *otf_buf_attach[HWFC_MAX_BUF];
};

struct _otf_buf_info {
	int pixel_format;
	int width;
	int height;
	int buffer_count;
	struct dma_buf *bufs[HWFC_MAX_BUF];
};

struct _otf_debug {
	struct mfc_special_buf stream_buf[OTF_MAX_BUF];
	unsigned int stream_size[OTF_MAX_BUF];
	unsigned char frame_cnt;
};

struct _otf_handle {
	int otf_work_bit;
	int otf_buf_index;
	int otf_job_id;
	u64 otf_time_stamp;
	struct _otf_buf_addr otf_buf_addr;
	struct _otf_buf_info otf_buf_info;
	struct _otf_debug otf_debug;
};
/********************************************************************/

struct mfc_perf {
	void __iomem *regs_base0;
	void __iomem *regs_base1;

	struct timeval begin;
	struct timeval end;

	int new_start;
	int count;
	int drv_margin;
};

extern struct mfc_dump_ops mfc_dump_ops;
struct mfc_dump_ops {
	void (*dump_regs)(struct mfc_dev *dev);
	void (*dump_info)(struct mfc_dev *dev);
	void (*dump_info_without_regs)(struct mfc_dev *dev);
	void (*dump_info_context)(struct mfc_dev *dev);
	void (*dump_and_stop_always)(struct mfc_dev *dev);
	void (*dump_and_stop_debug_mode)(struct mfc_dev *dev);
};

struct mfc_mmcache {
	void __iomem *base;
	int is_on_status;
};

/**
 * struct mfc_dev - The struct containing driver internal parameters.
 */
struct mfc_dev {
	struct v4l2_device	v4l2_dev;
	struct video_device	*vfd_dec;
	struct video_device	*vfd_enc;
	struct video_device	*vfd_dec_drm;
	struct video_device	*vfd_enc_drm;
	struct video_device	*vfd_enc_otf;
	struct video_device	*vfd_enc_otf_drm;
	struct device		*device;

	void __iomem		*regs_base;
	void __iomem		*sysmmu0_base;
	void __iomem		*sysmmu1_base;
	void __iomem		*hwfc_base;

	int			irq;
	struct resource		*mfc_mem;

	struct mfc_pm	pm;
	struct mfc_fw	fw;
	struct mfc_variant	*variant;
	struct mfc_platdata	*pdata;
	struct mfc_debug	*logging_data;

	int num_inst;
	int num_otf_inst;

	struct mutex mfc_mutex;

	int int_condition;
	int int_reason;
	unsigned int int_err;

	wait_queue_head_t cmd_wq;
	struct mfc_listable_wq hwlock_wq;

	bool has_2sysmmu;
	bool has_hwfc;
	bool has_mmcache;

	struct mfc_special_buf common_ctx_buf;
	struct mfc_special_buf drm_common_ctx_buf;

	struct mfc_ctx *ctx[MFC_NUM_CONTEXTS];
	int curr_ctx;
	int preempt_ctx;

	struct mfc_bits work_bits;

	struct mfc_hwlock hwlock;

	atomic_t sched_wait_cnt;
	atomic_t watchdog_tick_running;
	atomic_t watchdog_tick_cnt;
	atomic_t watchdog_run;
	struct timer_list watchdog_timer;
	struct workqueue_struct *watchdog_wq;
	struct work_struct watchdog_work;

	/* for DRM */
	int curr_ctx_is_drm;
	int num_drm_inst;
	struct mfc_special_buf fw_buf;
	struct mfc_special_buf drm_fw_buf;

	struct workqueue_struct *butler_wq;
	struct work_struct butler_work;

#ifdef CONFIG_MFC_USE_BUS_DEVFREQ
	struct list_head qos_queue;
	atomic_t qos_req_cur;
	struct pm_qos_request qos_req_mfc;
	struct pm_qos_request qos_req_int;
	struct pm_qos_request qos_req_mif;
	struct pm_qos_request qos_req_cluster[MAX_NUM_CLUSTER];
	int qos_has_enc_ctx;
	struct mutex qos_mutex;
#endif
	int id;
	atomic_t clk_ref;

	atomic_t trace_ref;
	struct _mfc_trace *mfc_trace;
	atomic_t trace_ref_longterm;

	struct _mfc_trace *mfc_trace_longterm;
	bool continue_clock_on;

	bool shutdown;
	bool sleep;

	nal_queue_handle *nal_q_handle;

	struct mfc_special_buf dbg_info_buf;

#ifdef CONFIG_EXYNOS_BTS
	struct bts_bw mfc_bw;
#endif

	struct mfc_debugfs debugfs;
	struct mfc_dump_ops *dump_ops;

	struct mfc_perf perf;

	struct mfc_mmcache mmcache;

#ifdef CONFIG_EXYNOS_ITMON
	struct notifier_block itmon_nb;
#endif
	int itmon_notified;

	char *reg_buf;
	unsigned int *reg_val;
	unsigned int reg_cnt;
};

/**
 *
 */
struct mfc_h264_enc_params {
	enum v4l2_mpeg_video_h264_profile profile;
	u8 level;
	u8 interlace;
	u16 open_gop_size;
	u8 open_gop;
	u8 _8x8_transform;
	s8 loop_filter_alpha;
	s8 loop_filter_beta;
	enum v4l2_mpeg_video_h264_loop_filter_mode loop_filter_mode;
	enum v4l2_mpeg_video_h264_entropy_mode entropy_mode;
	u8 rc_frame_qp;
	u8 rc_min_qp;
	u8 rc_max_qp;
	u8 rc_min_qp_p;
	u8 rc_max_qp_p;
	u8 rc_min_qp_b;
	u8 rc_max_qp_b;
	u8 rc_mb_dark;
	u8 rc_mb_smooth;
	u8 rc_mb_static;
	u8 rc_mb_activity;
	u8 rc_p_frame_qp;
	u8 rc_b_frame_qp;
	u8 ar_vui;
	u8 sei_gen_enable;
	u8 sei_fp_curr_frame_0;

	enum v4l2_mpeg_video_h264_vui_sar_idc ar_vui_idc;
	u16 ext_sar_width;
	u16 ext_sar_height;

	enum v4l2_mpeg_video_h264_hierarchical_coding_type hier_qp_type;
	u32 hier_bit_layer[7];
	u8 hier_qp_layer[7];
	u8 hier_qp_enable;
	u8 num_hier_layer;
	u8 hier_ref_type;
	u8 enable_ltr;
	u8 num_of_ltr;
	u32 set_priority;
	u32 base_priority;

	enum v4l2_mpeg_video_h264_sei_fp_arrangement_type sei_fp_arrangement_type;
	u32 fmo_enable;
	u32 fmo_slice_map_type;
	u32 fmo_slice_num_grp;
	u32 fmo_run_length[4];
	u32 fmo_sg_dir;
	u32 fmo_sg_rate;
	u32 aso_enable;
	u32 aso_slice_order[8];

	u32 prepend_sps_pps_to_idr;
	u32 vui_enable;
};

/**
 *
 */
struct mfc_mpeg4_enc_params {
	/* MPEG4 Only */
	enum v4l2_mpeg_video_mpeg4_profile profile;
	u8 level;
	u8 quarter_pixel;
	u8 rc_b_frame_qp;
	/* Common for MPEG4, H263 */
	u8 rc_frame_qp;
	u8 rc_min_qp;
	u8 rc_max_qp;
	u8 rc_min_qp_p;
	u8 rc_max_qp_p;
	u8 rc_min_qp_b;
	u8 rc_max_qp_b;
	u8 rc_p_frame_qp;
	u16 vop_time_res;
	u16 vop_frm_delta;
};

/**
 *
 */
struct mfc_vp9_enc_params {
	/* VP9 Only */
	u8 profile;
	u8 level;
	u8 rc_min_qp;
	u8 rc_max_qp;
	u8 rc_min_qp_p;
	u8 rc_max_qp_p;
	u8 rc_frame_qp;
	u8 rc_p_frame_qp;
	u16 vp9_gfrefreshperiod;
	u8 vp9_goldenframesel;
	u8 hier_qp_enable;
	u8 num_hier_layer;
	u8 hier_qp_layer[3];
	u32 hier_bit_layer[3];
	u8 max_partition_depth;
	u8 intra_pu_split_disable;
};

/**
 *
 */
struct mfc_vp8_enc_params {
	/* VP8 Only */
	u8 vp8_version;
	u8 rc_min_qp;
	u8 rc_max_qp;
	u8 rc_min_qp_p;
	u8 rc_max_qp_p;
	u8 rc_frame_qp;
	u8 rc_p_frame_qp;
	u8 vp8_numberofpartitions;
	u8 vp8_filterlevel;
	u8 vp8_filtersharpness;
	u16 vp8_gfrefreshperiod;
	u8 vp8_goldenframesel;
	u8 intra_4x4mode_disable;
	u8 num_hier_layer;
	u8 hier_qp_enable;
	u8 hier_qp_layer[3];
	u32 hier_bit_layer[3];
};

/**
 *
 */
struct mfc_hevc_enc_params {
	u8 profile;
	u8 level;
	u8 tier_flag;
	/* HEVC Only */
	u8 rc_min_qp;
	u8 rc_max_qp;
	u8 rc_min_qp_p;
	u8 rc_max_qp_p;
	u8 rc_min_qp_b;
	u8 rc_max_qp_b;
	u8 rc_lcu_dark;
	u8 rc_lcu_smooth;
	u8 rc_lcu_static;
	u8 rc_lcu_activity;
	u8 rc_frame_qp;
	u8 rc_p_frame_qp;
	u8 rc_b_frame_qp;
	u8 max_partition_depth;
	u8 refreshtype;
	u16 refreshperiod;
	s32 lf_beta_offset_div2;
	s32 lf_tc_offset_div2;
	u8 loopfilter_disable;
	u8 loopfilter_across;
	u8 nal_control_length_filed;
	u8 nal_control_user_ref;
	u8 nal_control_store_ref;
	u8 const_intra_period_enable;
	u8 lossless_cu_enable;
	u8 wavefront_enable;
	enum v4l2_mpeg_video_hevc_hierarchical_coding_type hier_qp_type;
	u8 enable_ltr;
	u8 hier_qp_enable;
	u8 hier_ref_type;
	u8 num_hier_layer;
	u32 hier_bit_layer[7];
	u8 hier_qp_layer[7];
	u8 general_pb_enable;
	u8 temporal_id_enable;
	u8 strong_intra_smooth;
	u8 intra_pu_split_disable;
	u8 tmv_prediction_disable;
	u8 max_num_merge_mv;
	u8 eco_mode_enable;
	u8 encoding_nostartcode_enable;
	u8 size_of_length_field;
	u8 user_ref;
	u8 store_ref;
	u8 prepend_sps_pps_to_idr;
};

/**
 *
 */
struct mfc_bpg_enc_params {
	u32 thumb_size;
	u32 exif_size;
};

/**
 *
 */
struct mfc_enc_params {
	enum v4l2_mpeg_video_multi_slice_mode slice_mode;
	u32 slice_mb;
	u32 slice_bit;
	u32 slice_mb_row;

	u32 gop_size;
	u32 intra_refresh_mb;
	u32 i_frm_ctrl_mode;
	u32 i_frm_ctrl;

	u8 pad;
	u8 pad_luma;
	u8 pad_cb;
	u8 pad_cr;

	u8 fixed_target_bit;
	u8 rc_mb;		/* H.264: MFCv5, MPEG4/H.263: MFCv6 */
	u8 rc_pvc;
	u8 rc_frame;
	u32 rc_bitrate;
	u32 rc_framerate;
	u16 rc_reaction_coeff;
	u16 rc_frame_delta;	/* MFC6.1 Only */

	u32 config_qp;
	u32 dynamic_qp;

	u8 frame_tag;
	u8 ratio_intra;
	u8 num_b_frame;		/* H.264, HEVC, MPEG4 */
	u8 num_refs_for_p;	/* H.264, HEVC, VP8, VP9 */
	enum v4l2_mpeg_video_header_mode seq_hdr_mode;
	enum v4l2_mpeg_mfc51_video_frame_skip_mode frame_skip_mode;
	u16 vbv_buf_size;
	u8 num_hier_max_layer;
	u8 hier_bitrate_ctrl;
	u8 weighted_enable;
	u8 roi_enable;
	u8 ivf_header_disable;	/* VP8, VP9 */

	u32 check_color_range;
	u32 color_range;
	u32 colour_primaries;
	u32 transfer_characteristics;
	u32 matrix_coefficients;

	u32 static_info_enable;
	u32 max_pic_average_light;
	u32 max_content_light;
	u32 max_display_luminance;
	u32 min_display_luminance;
	u32 white_point;
	u32 display_primaries_0;
	u32 display_primaries_1;
	u32 display_primaries_2;

	union {
		struct mfc_h264_enc_params h264;
		struct mfc_mpeg4_enc_params mpeg4;
		struct mfc_vp8_enc_params vp8;
		struct mfc_vp9_enc_params vp9;
		struct mfc_hevc_enc_params hevc;
		struct mfc_bpg_enc_params bpg;
	} codec;
};

struct mfc_ctx_ctrl {
	struct list_head list;
	enum mfc_ctrl_type type;
	unsigned int id;
	unsigned int addr;
	int has_new;
	int val;
};

struct mfc_buf_ctrl {
	struct list_head list;
	unsigned int id;
	enum mfc_ctrl_type type;
	int has_new;
	int val;
	unsigned int old_val;		/* only for MFC_CTRL_TYPE_SET */
	unsigned int old_val2;		/* only for MFC_CTRL_TYPE_SET */
	unsigned int is_volatile;	/* only for MFC_CTRL_TYPE_SET */
	unsigned int updated;
	unsigned int mode;
	unsigned int addr;
	unsigned int mask;
	unsigned int shft;
	unsigned int flag_mode;		/* only for MFC_CTRL_TYPE_SET */
	unsigned int flag_addr;		/* only for MFC_CTRL_TYPE_SET */
	unsigned int flag_shft;		/* only for MFC_CTRL_TYPE_SET */
	int (*read_cst) (struct mfc_ctx *ctx,
			struct mfc_buf_ctrl *buf_ctrl);
	void (*write_cst) (struct mfc_ctx *ctx,
			struct mfc_buf_ctrl *buf_ctrl);
};

struct mfc_ctrl_cfg {
	enum mfc_ctrl_type type;
	unsigned int id;
	unsigned int is_volatile;	/* only for MFC_CTRL_TYPE_SET */
	unsigned int mode;
	unsigned int addr;
	unsigned int mask;
	unsigned int shft;
	unsigned int flag_mode;		/* only for MFC_CTRL_TYPE_SET */
	unsigned int flag_addr;		/* only for MFC_CTRL_TYPE_SET */
	unsigned int flag_shft;		/* only for MFC_CTRL_TYPE_SET */
	int (*read_cst) (struct mfc_ctx *ctx,
			struct mfc_buf_ctrl *buf_ctrl);
	void (*write_cst) (struct mfc_ctx *ctx,
			struct mfc_buf_ctrl *buf_ctrl);
};

/* per buffer contol */
struct mfc_ctrls_ops {
	/* controls per buffer */
	int (*init_ctx_ctrls) (struct mfc_ctx *ctx);
	int (*cleanup_ctx_ctrls) (struct mfc_ctx *ctx);
	int (*init_buf_ctrls) (struct mfc_ctx *ctx,
			enum mfc_ctrl_type type, unsigned int index);
	void (*reset_buf_ctrls) (struct list_head *head);
	int (*cleanup_buf_ctrls) (struct mfc_ctx *ctx,
			enum mfc_ctrl_type type, unsigned int index);
	int (*to_buf_ctrls) (struct mfc_ctx *ctx, struct list_head *head);
	int (*to_ctx_ctrls) (struct mfc_ctx *ctx, struct list_head *head);
	int (*set_buf_ctrls_val) (struct mfc_ctx *ctx,
			struct list_head *head);
	int (*get_buf_ctrls_val) (struct mfc_ctx *ctx,
			struct list_head *head);
	int (*recover_buf_ctrls_val) (struct mfc_ctx *ctx,
			struct list_head *head);
	int (*get_buf_update_val) (struct mfc_ctx *ctx,
			struct list_head *head, unsigned int id, int value);
	int (*set_buf_ctrls_val_nal_q_dec) (struct mfc_ctx *ctx,
			struct list_head *head, DecoderInputStr *pInStr);
	int (*get_buf_ctrls_val_nal_q_dec) (struct mfc_ctx *ctx,
			struct list_head *head, DecoderOutputStr *pOutStr);
	int (*set_buf_ctrls_val_nal_q_enc) (struct mfc_ctx *ctx,
			struct list_head *head, EncoderInputStr *pInStr);
	int (*get_buf_ctrls_val_nal_q_enc) (struct mfc_ctx *ctx,
			struct list_head *head, EncoderOutputStr *pOutStr);
	int (*recover_buf_ctrls_nal_q) (struct mfc_ctx *ctx,
			struct list_head *head);
};

struct stored_dpb_info {
	int fd[MFC_MAX_PLANES];
};

struct dec_dpb_ref_info {
	int index;
	struct stored_dpb_info dpb[MFC_MAX_DPBS];
};

struct temporal_layer_info {
	unsigned int temporal_layer_count;
	unsigned int temporal_layer_bitrate[VIDEO_MAX_TEMPORAL_LAYERS];
};

struct mfc_enc_roi_info {
	char *addr;
	int size;
	int upper_qp;
	int lower_qp;
	bool enable;
};

struct mfc_user_shared_handle {
	int fd;
	struct dma_buf *dma_buf;
	void *vaddr;
};

struct mfc_raw_info {
	int num_planes;
	int stride[3];
	int plane_size[3];
	int stride_2bits[3];
	int plane_size_2bits[3];
	unsigned int total_plane_size;
};

/* HDR10+ ST 2094 40 Metadata HEVC SEI Message */
struct hdr10_plus_meta_per_win {
	unsigned int  maxscl[HDR_MAX_SCL];
	unsigned int  average_maxrgb;
	unsigned char num_distribution_maxrgb_percentiles;
	unsigned char distribution_maxrgb_percentages[HDR_MAX_DISTRIBUTION];
	unsigned int  distribution_maxrgb_percentiles[HDR_MAX_DISTRIBUTION];
	unsigned int  fraction_bright_pixels;

	unsigned short tone_mapping_flag;
	unsigned short knee_point_x;
	unsigned short knee_point_y;
	unsigned short num_bezier_curve_anchors;
	unsigned short bezier_curve_anchors[HDR_MAX_BEZIER_CURVES];

	unsigned char color_saturation_mapping_flag;
	unsigned char color_saturation_weight;

	/*
	 * This field is reserved for ST2094-40 SEI below or the others
	 * window_upper_left_corner_x
	 * window_upper_left_corner_y
	 * window_lower_right_corner_x
	 * window_lower_right_corner_y
	 * center_of_ellipse_x
	 * center_of_ellipse_y
	 * rotation_angle
	 * semimajor_axis_internal_ellipse
	 * semimajor_axis_external_ellipse
	 * semiminor_axis_external_ellipse
	 * overlap_process_option
	 */
	unsigned int reserved[11];
};

struct hdr10_plus_meta {
	unsigned int valid;

	unsigned char  t35_country_code;
	unsigned short t35_terminal_provider_code;
	unsigned short t35_terminal_provider_oriented_code;
	unsigned char  application_identifier;
	unsigned short application_version;
	unsigned char  num_windows;

	unsigned int  target_maximum_luminance;
	unsigned char target_actual_peak_luminance_flag;
	unsigned char num_rows_target_luminance;
	unsigned char num_cols_target_luminance;

	unsigned char mastering_actual_peak_luminance_flag;
	unsigned char num_rows_mastering_luminance;
	unsigned char num_cols_mastering_luminance;

	struct hdr10_plus_meta_per_win win_info[HDR_MAX_WINDOWS];

	/*
	 * This field is reserved for ST2094-40 SEI below or the others
	 * targeted_system_display_actual_peak_luminance[rows][cols]
	 * mastering_display_actual_peak_luminance[rows][cols]
	 */
	unsigned int reserved[11];
};

struct mfc_timestamp {
	struct list_head list;
	struct timeval timestamp;
	int index;
	int interval;
};

struct mfc_dec {
	int total_dpb_count;

	unsigned int src_buf_size;

	int loop_filter_mpeg4;
	int display_delay;
	int immediate_display;
	int slice_enable;
	int mv_count;
	int idr_decoding;
	int is_interlaced;
	int is_dts_mode;
	int stored_tag;

	int crc_enable;
	int crc_luma0;
	int crc_chroma0;
	int crc_luma1;
	int crc_chroma1;

	unsigned long consumed;
	unsigned long remained_size;
	dma_addr_t y_addr_for_pb;

	int sei_parse;

	int cr_left, cr_right, cr_top, cr_bot;

	int detect_black_bar;
	bool black_bar_updated;
	struct v4l2_rect black_bar;

	/* For dynamic DPB */
	int is_dynamic_dpb;
	int is_dpb_full;
	unsigned long available_dpb;
	unsigned int dynamic_set;
	unsigned int dynamic_used;

	struct dec_dpb_ref_info *ref_info;
	int assigned_fd[MFC_MAX_DPBS];
	struct mfc_user_shared_handle sh_handle_dpb;
	struct mfc_user_shared_handle sh_handle_hdr;
	struct hdr10_plus_meta *hdr10_plus_info;

	int has_multiframe;

	unsigned int err_reuse_flag;
	unsigned int dec_only_release_flag;

	unsigned int num_of_tile_over_4;
	unsigned int super64_bframe;

	unsigned int color_range;
	unsigned int color_space;

	unsigned int decoding_order;
	/*
	 * new variables should be added above
	 * ============ boundary line ============
	 * The following variables are excluded from the MFC log dumps
	 */

	/* for DRM ASP */
	struct mfc_buf *assigned_dpb[MFC_MAX_DPBS];

	/* for debugging about black bar detection */
	void *frame_vaddr[3][30];
	dma_addr_t frame_daddr[3][30];
	int index[3][30];
	int fd[3][30];
	unsigned int frame_size[3][30];
	unsigned char frame_cnt;
};

struct mfc_enc {
	unsigned int dst_buf_size;
	unsigned int header_size;

	enum v4l2_mpeg_mfc51_video_frame_type frame_type;
	enum v4l2_mpeg_mfc51_video_force_frame_type force_frame_type;

	size_t luma_dpb_size;
	size_t chroma_dpb_size;
	size_t me_buffer_size;
	size_t tmv_buffer_size;

	unsigned int slice_mode;
	unsigned int slice_size_mb;
	unsigned int slice_size_bits;
	unsigned int in_slice;
	unsigned int buf_full;

	int stored_tag;
	struct mfc_user_shared_handle sh_handle_svc;
	struct mfc_user_shared_handle sh_handle_roi;
	struct mfc_user_shared_handle sh_handle_hdr;
	int roi_index;
	struct mfc_special_buf roi_buf[MFC_MAX_EXTRA_BUF];
	struct mfc_enc_roi_info roi_info[MFC_MAX_EXTRA_BUF];

	/*
	 * new variables should be added above
	 * ============ boundary line ============
	 * The following variables are excluded from the MFC log dumps
	 */
	struct mfc_enc_params params;
};

struct mfc_fmt {
	char *name;
	u32 fourcc;
	u32 codec_mode;
	u32 type;
	u32 num_planes;
	u32 mem_planes;
};

/**
 * struct mfc_ctx - This struct contains the instance context
 */
struct mfc_ctx {
	struct mfc_dev *dev;
	struct mfc_dec *dec_priv;
	struct mfc_enc *enc_priv;
	struct _otf_handle *otf_handle;

	int num;
	int int_condition;
	int int_reason;
	unsigned int int_err;

	struct mfc_fmt *src_fmt;
	struct mfc_fmt *dst_fmt;

	struct mfc_buf_queue src_buf_queue;
	struct mfc_buf_queue dst_buf_queue;
	struct mfc_buf_queue src_buf_nal_queue;
	struct mfc_buf_queue dst_buf_nal_queue;
	struct mfc_buf_queue ref_buf_queue;
	spinlock_t buf_queue_lock;

	enum mfc_inst_type type;
	enum mfc_inst_state state;
	int inst_no;

	int img_width;
	int img_height;
	int crop_width;
	int crop_height;
	int crop_left;
	int crop_top;
	int dpb_count;
	int buf_stride;

	int min_dpb_size[3];

	struct mfc_raw_info raw_buf;

	enum mfc_queue_state capture_state;
	enum mfc_queue_state output_state;

	unsigned long src_ctrls_avail;
	unsigned long dst_ctrls_avail;

	unsigned int sequence;

	/* Control values */
	int codec_mode;
	__u32 pix_format;

	/* Extra Buffers */
	int codec_buffer_allocated;
	struct mfc_special_buf codec_buf;
	struct mfc_special_buf instance_ctx_buf;

	size_t mv_size;
	size_t scratch_buf_size;
	size_t loopfilter_luma_size;
	size_t loopfilter_chroma_size;

	/* Profile infomation */
	int is_10bit;
	int is_422;

	/* for DRM */
	int is_drm;

	int is_dpb_realloc;
	enum mfc_dec_wait_state wait_state;
	int clear_work_bit;

	unsigned long framerate;
	unsigned long last_framerate;
	unsigned int qos_ratio;

#ifdef CONFIG_MFC_USE_BUS_DEVFREQ
	int qos_req_step;
	struct list_head qos_list;
#endif

	struct mfc_timestamp ts_array[MFC_TIME_INDEX];
	struct list_head ts_list;
	int ts_count;
	int ts_is_full;

	int buf_process_type;

	unsigned long raw_protect_flag;
	unsigned long stream_protect_flag;

	int batch_mode;
	bool check_dump;
	bool mem_type_10bit;

	struct vb2_queue vq_src;
	struct vb2_queue vq_dst;

	/*
	 * new variables should be added above
	 * ============ boundary line ============
	 * The following variables are excluded from the MFC log dumps
	 */

	/* external structure */
	struct v4l2_fh fh;

	/* per buffer controls */
	struct mfc_ctrls_ops *c_ops;
	struct list_head ctrls;
	struct list_head src_ctrls[MFC_MAX_BUFFERS];
	struct list_head dst_ctrls[MFC_MAX_BUFFERS];

	/* wait queue */
	wait_queue_head_t cmd_wq;
	struct mfc_listable_wq hwlock_wq;
};

#endif /* __MFC_DATA_STRUCT_H */
