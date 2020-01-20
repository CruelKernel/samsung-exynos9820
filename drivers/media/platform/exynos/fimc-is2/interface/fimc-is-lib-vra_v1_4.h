/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_LIB_VRA_H
#define FIMC_IS_LIB_VRA_H

#define VRA_DICO_API_VERSION			113
#define VRA_OTF_INPUT_FORMAT			VRA_YUV_FMT_422
#define VRA_TR_HW_RES_PER_FACE			100
#define VRA_FF_HW_RES_PER_LIST			150
#define VRA_IMAGE_SLOTS				4
/* default tune value */
#define VRA_TUNE_TRACKING_MODE			VRA_TRM_FULL_FR_TRACK
#define VRA_TUNE_FACE_PRIORITY			0
#define VRA_TUNE_DISABLE_FRONTAL_ROT_MASK	0x28
#define VRA_TUNE_WORKING_POINT			900
/* Hybrid FD */
#define MAX_HYBRID_FACES			16

typedef unsigned char vra_boolean;

/*
 * Assuming that int for all compilers using this code are 32 bits,
 * while long is translated to either 32 or 64 bits
 */
typedef unsigned int vra_uint32;

typedef signed int vra_int32;
typedef unsigned char *vra_dram_adr_type;

/*
 * Assuming that on systems where DramAdrType
 * is 64 bits unsigned long is defined as 8 bytes e.g. LINUX
 */
typedef unsigned long vra_ptr_as_int;

struct api_vra_tune_data {
	vra_uint32 tracking_mode;
	vra_uint32 enable_features;
	vra_uint32 full_frame_detection_freq;
	vra_uint32 min_face_size;
	vra_uint32 max_face_count;
	vra_uint32 face_priority;
	vra_uint32 disable_frontal_rot_mask;
	vra_uint32 disable_profile_rot_mask;
	vra_uint32 working_point;
	vra_uint32 tracking_smoothness;
	vra_uint32 selfie_working_point;
	vra_uint32 sensor_position;
	vra_uint32 use_post_detection_output;
};

enum api_vra_ctrl_task_set_event {	/* deprecated event id */
	CTRL_TASK_SET_CH0_INT,
	CTRL_TASK_SET_CH1_INT,
	CTRL_TASK_SET_NEWFR,
	CTRL_TASK_SET_ABORT,
	CTRL_TASK_SET_FWALGS
};

enum api_vra_fw_algs_task_set_event {
	FWALGS_TASK_SET_ABORT,
	FWALGS_TASK_SET_ALGS
};

enum api_vra_type {
	VRA_NO_ERROR				= 0,
	VRA_BUSY				= 1,
	/*
	 * Can't be currently performed -
	 * in the middle of overlapped operation
	 */

	/* General errors - Invalid data pointers */
	VRA_ERR_FRWORK_NOT_VALID		= 2,
	/*
	 * Framework isn't set, Different Framework is active,
	 * Framework data is overwritten
	 */
	VRA_ERR_SENSOR_NOT_VALID		= 3,
	/*
	 * Sensor isn't det, Sensor data is overwritten,
	 * Sensor points to invalid Framework
	 */

	/* Initialization errors */
	VRA_ERR_INIT_UNSUPPORTED_COMPILER	= 0x10,
	/*
	 * compiler with unsigned int that isn't 32 bits
	 * or unsigned long size < (Ptr - update vra_uint32 / vra_ptr_as_int typedef)
	 */
	VRA_ERR_INIT_UNSUPPORTED_API_VERSION	= 0x11,
	/*
	 * The API version set as Input is not supported anymore
	 * by this FW version
	 */
	VRA_ERR_INIT_UNKNOWN_API_VERSION	= 0x12,
	/* The API version set as Input is not known */
	VRA_ERR_INIT_HW_VERSION_NOT_MATCH_FW	= 0x13,
	/* The FW is compiled with different HW version */
	VRA_ERR_INIT_HW_VERSION_NOT_MATCH_HW	= 0x14,
	/* The HW that is used has different version */
	VRA_ERR_INIT_NOT_ENOUGH_MEMORY		= 0x15,
	/* Allocated memory for Framework or Sensor isn't big enough */
	VRA_ERR_INIT_NOT_ENOUGH_HW_MEMORY	= 0x16,
	/* Allocated memory for HW usage isn't big enough */
	VRA_ERR_INIT_FRWORK_ALREADY_EXIST	= 0x17,
	/* Single Framework is allowed */
	VRA_ERR_INIT_BAD_FEATURES_DATA		= 0x18,
	/* The data describing the features and the poses isn't valid */
	VRA_ERR_INIT_ONLY_SINGLE_SENSOR_ALLOWED	= 0x19,
	/*
	 * Only single sensor is allowed
	 * when VRA_ActivateSensor function is used instead of VRA_OnNewFrame
	 */
	VRA_ERR_INIT_MORE_THAN_MAX_SENSORS	= 0x1A,
	/*
	 * The total number of sensors is bigger
	 * than the maximal number of sensor allocated
	 */
	VRA_ERR_INIT_CLOCK_NOT_SET		= 0x1B,
	/* HwClockFreqMhz and SwClockFreqMhz must be set */
	VRA_ERR_ALLOC_MAX_DS_TOO_SMALL		= 0x1C,
	/* MaxDsImage.Width and MaxDsImage.Height must be at least 16 */
	VRA_ERR_ALLOC_DS_WIDTH_BIGGER_THAN_MAX	= 0x1D,
	/* MaxDsImage.Width must not be bigger than VRA_MAX_DS_WIDTH */
	VRA_ERR_ALLOC_CHAIN0_NOT_EXIST			= 0x1E,
	/*!< Ch0 not exist so can't allocate memory for Ch0 input.*/
	VRA_ERR_INIT_ONLY_DRAM_INPUT			= 0x1F,
	/*!< No CIN so input must be sent throgh DRAM */
	VRA_ERR_INIT_ZERO_MEMORY_SIZE			= 0x20,
	VRA_ERR_INIT_SENSOR_NOT_EXIST			= 0x21,
	/*!< Input errors. Some of them might happen also on initialization */
	VRA_ERR_BAD_INPUT_FORMAT		= 0x30,
	/*
	 * Not supported combination of input Planes number,
	 * DRAM input, YUV format and packing
	 */
	VRA_ERR_PADDING_NOT_ALLOWED		= 0x31,
	/*
	 * When UsePad isn't set on memory allocation
	 * on initalization padding can't be invoked
	 */
	VRA_ERR_WRONG_ZOOM_VALUE		= 0x32,
	/* The zoom value is less than 1000 stands for 1.0 */
	VRA_ERR_WRONG_TRACK_MODE		= 0x33,
	/* The track mode type is illegal */
	VRA_ERR_WRONG_MAX_FACES			= 0x34,
	/*
	 * The maximal track sizes is bigger
	 * than the memory allocated for the framework
	 */
	VRA_ERR_WRONG_SORT_TYPE			= 0x35,
	/* The required Sort type isn't valid */
	VRA_ERR_WRONG_MIN_FACE_SIZE		= 0x36,
	/* The required minimal face size isn't supported */
	VRA_ERR_WRONG_DETECT_FREQUENCY		= 0x37,
	/* The required Detection frequency isn't valid */
	VRA_ERR_SMILENOTALLOWED			= 0x38,
	/* Can't invoke smile - no appropriate const data */
	VRA_ERR_BLINKNOTALLOWED			= 0x39,
	/* Can't invoke blink - no appropriate const data */
	VRA_ERR_EYESDETNOTALLOWED		= 0x3A,
	/* Can't invoke eyes detection - no appropriate const data */
	VRA_ERR_MOUTH_DET_NOT_ALLOWED		= 0x3B,
	/* Can't invoke mouth detection - no appropriate const data */
	VRA_ERR_WRONG_ROT_ANGLES		= 0x3C,
	/* The value of the rotation angles or the yaw type is wrong */
	VRA_ERR_WRONG_WORKING_POINT		= 0x3D,
	/* The value of the working point isn't in 0-1000 range */
	VRA_ERR_WRONGSMOOTHLEVEL		= 0x3E,
	/* The value of the smooth level isn't in 0-20 range */
	VRA_ERR_WRONG_PADDING_VALUE		= 0x3F,
	/* The value of the padding size isn't in 0-8 range */
	VRA_ERR_WRONGLOADBALANCEVALUES		= 0x40,
	/*
	 * At least one of the requested indices is too big
	 * or BestInd > WorstInd
	 */
	VRA_ERR_WRONG_INPUT_SIZES_FOR_ALLOC		= 0x41,
	/*
	 * The input sizes for frame allocation are bigger than
	 * maximal input sizes
	 */
	VRA_ERR_WRONG_HYBRID_MAX_FACES			= 0x42,
	/*!< The actual Hybrid max faces must be <= allocated hybrid faces */
	VRA_ERR_WRONG_HYBRID_FREQUENCY		= 0x43,
	/*!< The Hybrid frequency must not be set to 0 */
	/* General errors */
	VRA_ERR_INTR_INDEX_NOT_VALID		= 0x51,
	/* The input interrupt index isn't valid */
	VRA_ERR_INTR_ALREADY_ACTIVE		= 0x52,
	/* Interrupt shouldn't be invoked while other interrupt is active */
	VRA_ERR_INTR_NOT_EXPECTED		= 0x53,
	/* Current Interrupt isn't expected - HW isn't active */
	VRA_ERR_INTR_NO_SOURCE			= 0x54,
	/* Current Interrupt source couldn't be found - HW mask is 0 */
	VRA_ERR_ALREADY_ABORTED			= 0x55,
	/* Framework is already aborting */
	VRA_ERR_TASKALREADYACTIVE		= 0x56,
	/*
	 * Task code is not re-entrant
	 * and anohter instance is currently active
	 */

	/* New Frame + Activate errors  + Alloc + Enable Clocks */
	VRA_ERR_FRWORK_ABORTING			= 0x61,
	/* Can't be set since Framework is aborting */
	VRA_ERR_NEW_FR_PREV_REQ_NOT_HANDLED	= 0x62,
	/* New frame can't be set since previous request wasn't handled yet */
	VRA_ERR_NEW_FR_NEXT_EXIST		= 0x63,
	/*
	 * Current sensor already has defined
	 * "next frame" waiting for process
	 */
	VRA_ERR_SENSOR_WRONG_EXPLICIT_NEW_FR	= 0x64,
	/* VRA_OnNewFrame isn't allowed after VRA_ActivateSensor */
	VRA_ERR_SENSOR_NEW_ALLOC_FR_REQUIRED	= 0x65,
	/*
	 * VRA_OnNewFrame isn't allowed after VRA_GetFrWorkFrameAllocation
	 * without VRA_OnNewAllocatedFrame
	 */
	VRA_ERR_NOT_ALLOWED_FOR_MULTI_SENSORS	= 0x66,
	/* VRA_ActivateSensor not allowed when more than 1 sensor is set */
	VRA_ERR_NOT_ALLOWED_FOR_DRAM_INPUT	= 0x67,
	/* VRA_ActivateSensor not allowed when DRAM input is used */
	VRA_ERR_NOT_ALLOWED_FOR_CH0_INPUT	= 0x68,
	/*!< VRA_GetFrWorkFrameAllocation not allowed when Ch0 input is used */
	VRA_ERR_NOT_ALLOWED_FOR_HW_CFGID	= 0x69,
	/*!< Operation not allowed for current VRA_HW_CFG_ID */
	VRA_ERR_NO_FREE_SLOT				= 0x6A,
	/*!< no free pyramid slot */
	VRA_ERR_SLOT_ALREADY_ALLOCATED		= 0x6B,
	/*!< A pyramid slot was already allocated and not used */
	VRA_ERR_SLOT_NOT_ALLOCATED			= 0x6C,
	/*!< No pyramid slot was allocated */
	VRA_ERR_WRONG_SLOT_INDEX			= 0x6D,
	/*!< The input slot index wasn't allocated for HOST */
	VRA_ERR_WRONG_INPUT_FORMAT_FOR_ALLOC	= 0x6E,
	/*!< Current frame input format isn't allowed for allocated frame */
	VRA_ERR_WRONG_ALLOC_LINE_OFS			= 0x6F,
	/*!< The allocated UV line offset or Y line offsert not match the sizes of current frame */
	VRA_ERR_WRONG_ALLOC_UV_ADR				= 0x70,
	/*!< The allocated UV address don't match to the allocated slot */
	VRA_ERR_WRONG_ALLOC_Y_ADR				= 0x71,
	/*!< The allocated Y address don't match to the allocated slot */
	VRA_ERR_ALLOC_PAD_TOP_PYR				= 0x72,
	/*!< Y top pyramid padding is required - not allowed for allocated input */
	VRA_ERR_TASK_ACTIVE					= 0x73,
	/*!< Can't be done while Framework task is active */
	VRA_ERR_HW_CLOCKS_IN_REQUIRED_EN_STATE	= 0x74,
	/*!< VRA_FrameWorkEnableHwClocks with EnableClocks that is already set */
	VRA_ERR_HW_CLOCKS_DISABLED			= 0x75,
	/*!< Must enable clocks before new input */
	VRA_ERR_HYBRID_DISABLED				= 0x76,
	/*!< Hybrid mode is disabled */
	VRA_ERR_HYBRID_RES_NOT_EXPECTED		= 0x77,
	/*!< Not waiting to Hybrid results */
	VRA_ERR_HYBRID_RES_TOO_SMALL_INDEX		= 0x78,
	/*!< The Hybrid results index is < expected index => old frame results */
	VRA_ERR_HYBRID_RES_TOO_BIG_INDEX		= 0x79,
	/*!< The Hybrid results index is > expected index => some results were lost */
	VRA_ERR_HYBRID_RES_WRONG_FACES_NUM		= 0x7A,
	/*!< The Hybrid results number of faces is not different from the expected */
	VRA_ERR_HYBRID_PREV_RES_NOT_HANDLED		= 0x7B,
	/*!< The Hybrid results can't be handled since previous results weren't handled yet */

	/* SetGet errors */
	VRA_ERR_INPUT_ACTIVE			= 0x81,
	/* Another Set/Get function is already active */
	VRA_ERR_TRANSACTION_ALREAY_ACTIVE	= 0x82,
	/* Transaction is already active */
	VRA_ERR_TRANSACTION_NOT_ACTIVE		= 0x83,
	/* Transaction is not active */

	/* Tuning errors */
	VRA_ERR_FRWORK_ACTIVE			= 0x91,
	/* Can't set tuning / override const data since Framework is active */
	VRA_ERR_WRONG_TUNING_GROUP		= 0x92,
	/* GrType excceds VRA_TUNE_GROUPS */
	VRA_ERR_WRONG_TUNING_VERSION		= 0x93,
	/* Wrong Group version */
	VRA_ERR_TUNING_PARAMETER_NOT_EXIST	= 0x94,
	/* Index of tuning parameter is bigger than the last in its group */
	VRA_ERR_TUNING_PARAMETER_NOT_INUSE	= 0x95,
	/* The requested tuning parmaeter is obsolete */
	VRA_ERR_TUNING_PARAMETER_TOO_SAMLL	= 0x96,
	/* Tuning parmaeter value is below minimal limitation */
	VRA_ERR_TUNING_PARAMETER_TOO_BIG	= 0x97,
	/* Tuning parmaeter value is above maximal limitation */
	VRA_Err_TuningParameterWrongHwCfgId	= 0x98
	/*!< Tuning parmaeter not relevant in current VRA_HW_CFG_ID */
};

/*
 * brief Following enums define explicit names
 * for booleans to make caller code more clear
 */

/* brief Whether next frame is processed or ignored */
enum api_vra_proc_type {
	VRA_IGNORE_FR = 0,
	VRA_PROCESS_FR = 1
};

/* brief Whether results list should be cleared or kept */
enum api_vra_update_tr_data_base {
	VRA_CLEAR_TR_DATA_BASE = 0,
	VRA_KEEP_TR_DATA_BASE = 1
};

/* brief Whether Current process should be finished or aborted */
enum api_vra_abort_type {
	VRA_FINISH_CRNT_PROC = 0,
	VRA_ABORT_CRNT_PROC = 1
};

/*
 * Bits for Mask of Eyes / Mouth / Smile / Blink process enabled
 * (bits that are not set are disabled)
 */
enum api_vra_facial_proc_type {
	VRA_FF_PROC_EYES = 0,
	VRA_FF_PROC_MOUTH = 1,
	VRA_FF_PROC_SMILE = 2,
	VRA_FF_PROC_BLINK = 3,
	VRA_FF_PROC_ALL = 4
};

#define VRA_FF_PROC_EN_EYES_BIT		(1 << VRA_FF_PROC_EYES)
#define VRA_FF_PROC_EN_MOUTH_BIT	(1 << VRA_FF_PROC_MOUTH)
#define VRA_FF_PROC_EN_SMILE_BIT	(1 << VRA_FF_PROC_SMILE)
#define VRA_FF_PROC_EN_BLINK_BIT	(1 << VRA_FF_PROC_BLINK)

/* brief The 3 facials locations for each face */
enum api_vra_facial_location_type {
	VRA_FF_LOCATION_LEFT_EYE = 0,
	VRA_FF_LOCATION_RIGHT_EYE = 1,
	VRA_FF_LOCATION_MOUTH = 2,
	VRA_FF_LOCATION_ALL = 3
};

/* brief The 6 facials that gets scores for each face */
enum api_vra_facial_scores_type {
	VRA_FF_SCORE_LEFT_EYE = 0,
	VRA_FF_SCORE_RIGHT_EYE = 1,
	VRA_FF_SCORE_MOUTH = 2,
	VRA_FF_SCORE_SMILE = 3,
	VRA_FF_SCORE_BLINK_LEFT = 4,
	VRA_FF_SCORE_BLINK_RIGHT = 5,
	VRA_FF_SCORE_ALL = 6
};

/* brief Whether Skin detection process is used for enhanced performance */
enum api_vra_skin_det_type {
	VRA_SKIN_DET_DISABLED = 0,
	VRA_SKIN_DET_ENABLED = 1
};

/* brief Whether Interrupts are set as "pulse" or "level" */
enum api_vra_int_type {
	VRA_INT_PULSE = 0,
	VRA_INT_LEVEL = 1
};

/* The Input YUV format */
enum api_vra_yuv_format {
	/* YUV per pixel - CIN or single plane or 3 planes */
	VRA_YUV_FMT_444 = 0,
	/* YUYV - CIN or DRAM single plane or DRAM 2 planes */
	VRA_YUV_FMT_422 = 1,
	/* U + V per each 2 * 2 Y pixels. Only on DRAM 2 planes */
	VRA_YUV_FMT_420 = 2,
	/* Y only data . Only on DRAM */
	VRA_YUV_FMT_400 = 3
};

/* Type of poses Yaw angles. */
enum api_vra_yaw_type {
	/* No YAW */
	VRA_YAW_FRONT = 0,
	/* for right profile => Yaw Angle = 90. 270 stands for left profile */
	VRA_YAW_RIGHT_PROFILE = 1,
	VRA_YAW_ALL = 2
};

/* brief The Image orientation */
enum api_vra_orientation {
	VRA_ORIENT_TOP_LEFT_TO_RIGHT = 0,	/* Normal mode */
	VRA_ORIENT_TOP_RIGHT_TO_LEFT = 1,	/* Mirror X */
	VRA_ORIENT_BTM_LEFT_TO_TOP = 2,		/* Rotate 90 CW */
	VRA_ORIENT_TOP_LEFT_TO_BTM = 3,		/* Rotate 90 CW + Mirror X */
	VRA_ORIENT_BTM_RIGHT_TO_LEFT = 4,	/* Rotate 180 CW */
	VRA_ORIENT_BTM_LEFT_TO_RIGHT = 5,	/* Rotate 180 CW + MirrorX */
	VRA_ORIENT_TOP_RIGHT_TO_BTM  = 6,	/* Rotate 270 CW */
	VRA_ORIENT_BTM_RIGHT_TO_TOP = 7		/* Rotate 270 CW + MirrorX */
};

/* brief Results sorting method */
enum api_vra_sort {
	VRA_SORT_NONE = 0,	/* No Preference. Keep results as is */
	VRA_SORT_SCORE = 1,	/* Prefer faces with higher score */
	VRA_SORT_SIZES = 2,	/* Prefer bigger faces */
	VRA_SORT_CENTER = 3,	/* Prefer faces closer to the center */
	VRA_SORT_ALL
};

/* brief Tracking mode */
enum api_vra_track_mode {
	/*
	 * Previous results are reset + full frame detection on each frame
	 * (and FF process + output after detection)
	 */
	VRA_TRM_SINGLE_FRAME = 0,

	/*
	 * Full frame detection on each frame
	 * (and FF process + output after detection)
	 */
	VRA_TRM_FULL_FR_TRACK = 1,

	/*
	 * Track + FF process is invoked according to previous frame result
	 * and output is sent after it is done and doesn't include new faces.
	 * Full frame detection is invoked
	 * according to SetFullFrameDetectionFrequency.
	 */
	VRA_TRM_ROI_TRACK = 2,

	VRA_TRM_ALL
};

/* brief FW errors */
enum api_vra_sen_err {
	/* Frame was lost due to process overload */
	VRA_ERR_FRAME_LOST = 0,
	/* Some of the faces lost during HW processing */
	VRA_ERR_HW_FACES_LOST = 1,
	/* Corrupted data cause Frame lost */
	VRA_ERR_CORRUPTED_DATA = 2,
	/*!< Hybrid request was lost due to process overload */
	VRA_ERR_HYB_REQ_LOST = 3,
};

/* brief HW errors */
enum api_vra_hw_err_bit_type {
	/* New frame recieved when HW wasn't prepared */
	VRA_HW_ERR_FR_LOST_ON_CLOSE_BIT = 0x1,
	/* New frame recieved while HW was still processing the prvious one */
	VRA_HW_ERR_FR_LOST_ON_ACTIVE_BIT = 0x2,
	/* AXI error detected on the input DMA to chain 0. */
	VRA_HW_ERR_AXI_IN_CH0_BIT = 0x4,
	/* AXI error detected on DMA writing chain0 pyramids to memory */
	VRA_HW_ERR_AXI_VDMA_OUT_BIT = 0x8,
	/* The CIN input sizes wasn't the expected sizes */
	VRA_HW_ERR_BAD_FR_SIZES_BIT = 0x10,
	/* Cin got inconsistent YUV format data */
	VRA_HW_ERR_YUV_FORMAT_BIT = 0x20,
	/* One of the CIN minmal intervals (horizontal or vertical) wasn't met */
	VRA_HW_ERR_INTERVALS_BIT = 0x40,
	/* The DMA writing chain0 data to memory had overflow */
	VRA_HW_ERR_OUT_VDMA_OVERFLOW_BIT = 0x80,
	/* The DMA writing chain1 results to memory had overflow */
	VRA_HW_ERR_RES_VDMA_OVERFLOW_BIT = 0x100,
	/* The DMA writing chain1 results to memory had more results than maximal allowed */
	VRA_HW_ERR_RES_VDMA_MAX_RES_BIT = 0x200,
	/* AXI error detected on the input DMA to chain 1 */
	VRA_HW_ERR_AXI_IN_CH1_BIT = 0x400,
	/* AXI error detected on DMA writing chain1 results to memory */
	VRA_HW_ERR_AXI_CH1_RES_BIT = 0x800,
	/* Chain1 Watchdog interrupt occurs - HW reset */
	VRA_HW_ERR_CH1_WATCHDOG_BIT = 0x1000,
	/* Chain0 interrupt called only once while 2 interrupts occured. */
	VRA_HW_ERR_CH0_INTR_LOST = 0x2000,
	/*!< Chain0 output self flush invoked - buffer is full and InvokeHwFlushOnBufferFull is set */
	VRA_HW_ERR_CH0_OUT_SELF_FLUSH = 0x4000,
};

/* Following masks defines the different HW errors types */
/* Errors that cause frame that is lost and not received by HW*/
#define VRA_MASK_ERR_FRAME_LOST		\
	(VRA_HW_ERR_FR_LOST_ON_CLOSE_BIT | VRA_HW_ERR_FR_LOST_ON_ACTIVE_BIT | VRA_HW_ERR_AXI_IN_CH0_BIT | VRA_HW_ERR_AXI_VDMA_OUT_BIT)
/* brief Errors that cause frame that is lost although received by HW since its input is not legal */
#define VRA_MASK_ERR_CORRUPTED_FRAME	\
	(VRA_HW_ERR_BAD_FR_SIZES_BIT | VRA_HW_ERR_YUV_FORMAT_BIT | VRA_HW_ERR_INTERVALS_BIT | VRA_HW_ERR_OUT_VDMA_OVERFLOW_BIT)
/* brief Errors that cause frame that is lost along Chain1 process */
#define VRA_MASK_ERR_FRAME_LOST_ON_CH1	\
	(VRA_HW_ERR_AXI_IN_CH1_BIT | VRA_HW_ERR_AXI_CH1_RES_BIT | VRA_HW_ERR_CH1_WATCHDOG_BIT)
/* Errors that stands for HW errors that were lost */
#define VRA_MASK_ERR_RESULTS_LOST	\
	(VRA_HW_ERR_RES_VDMA_OVERFLOW_BIT | VRA_HW_ERR_RES_VDMA_MAX_RES_BIT)

/* Each bit in mask stands for 45 degrees */
#define VRA_DIS_ROT_UNIT	45
#define VRA_DIS_ROT_0_BIT	(1 << 0)
#define VRA_DIS_ROT_45_BIT	(1 << 1)
#define VRA_DIS_ROT_90_BIT	(1 << 2)
#define VRA_DIS_ROT_135_BIT	(1 << 3)
#define VRA_DIS_ROT_180_BIT	(1 << 4)
#define VRA_DIS_ROT_225_BIT	(1 << 5)
#define VRA_DIS_ROT_270_BIT	(1 << 6)
#define VRA_DIS_ROT_315_BIT	(1 << 7)

struct api_vra_sizes {
	unsigned short width;
	unsigned short height;
};

/*
 * 5 stands for PYR current + Next output + Input for DET +
 * Input for POST_DET + Input to TR (+POST_TR + FF)
 *
 * Notice that on extreme invocation conditions
 * another DT slot + POST_DET slots may be also required
 */
#define VRA_DEF_MAPS_SLOTS	5

#define VRA_MAX_PAD_SIZE	8

/*
 * brief The required information for Sensor memory allocation -
 * both FW memory and HW memory
 */
struct api_vra_alloc_info {
	struct api_vra_sizes max_image;	/* HW memory */
	unsigned short track_faces;		/* SW memory */
	unsigned short dt_faces_hw_res;		/* Both HW and SW memory */

	/* used for HW tracking */
	unsigned short tr_hw_res_per_face;	/* HW memory */

	unsigned short ff_hw_res_per_face;	/* Both HW and SW memory */
	unsigned short ff_hw_res_per_list;	/* SW memory */

	/* The size in bytes of line of cache. Must be poewer of 2 */
	unsigned short cache_line_length;	/* HW memory */

	vra_boolean use_pad;			/* HW memory */

	/*
	 * HW memory - 0 stands for no support for 2 planes in Chain 0
	 * (Required only when DRAM input is sent throw chain0)
	 */
	vra_boolean allow_ch0_2planes;

	/*
	 * HW memory - 0 stands for input is sent throuhg DRAM directly to Ch1.
	 * 1 is required for either CIN input or "big" input images
	 */
	vra_boolean using_ch0_input;

	/* 0 stands for VRA_DEF_MAPS_SLOTS */
	unsigned char image_slots;		/* HW memory */

	/* required for maximal number of slots */
	unsigned char max_sensors;		/* Both HW and SW memory */

	/*
	 * Maximal delay frames between Track and Detection - 1
	 * (1 is single list - of current frame)
	 */
	unsigned char max_tr_res_frames;	/* Each Sensor memory */

	/* for Hybrid FD */
	unsigned char max_hybrid_faces;
};

typedef short int res_cord_type;	/* Includes RES_CORD_FRAC_BITS */
typedef short int res_score_type;	/* Includes RES_SCORE_FRAC_BITS */

/* Coordinates of the output results are represented without fractional bits */
#define RES_CORD_FRAC_BITS	0
#define RES_SCORE_FRAC_BITS	7
#define RES_SCORE_UNIT		(1 << RES_SCORE_FRAC_BITS)
#define MAX_SCORE		(15 * RES_SCORE_UNIT)

/* brief Description of rectangle - Top, Left + Sizes */
struct api_vra_rect_str {
	res_cord_type left;
	res_cord_type top;
	res_cord_type width;
	res_cord_type height;
};

/* brief Description of each Face in the output list */
struct api_vra_face_base_str {
	/* Start from 1. Might be reset on Reset Output list */
	vra_uint32			unique_id;
	/* The face coordinates where full image is the input image */
	struct api_vra_rect_str	rect;
	/* 0-1024 */
	res_score_type			score;
	/* Clockwise */
	unsigned short			rotation;
	/* 90 stands for right profile, 270 for left profile */
	unsigned short			yaw;
	/* Currently set to 0 */
	signed short			pitch;
};

/* brief Description of Point */

struct api_vra_point {
	res_cord_type left;
	res_cord_type top;
};

/* In case no valid facial or user ignores specific element */
#define VRA_FACIAL_DUMMY_SCORE		-32768
/* In case no valid facial or user ignores specific element */
#define VRA_FACIAL_DUMMY_LOCATION	0

/* brief Description of Facial data of each Face in the output list */
struct api_vra_facial_str {
	vra_boolean				is_valid_facial;
	/* In input image coordinates */
	struct api_vra_point	locations[VRA_FF_LOCATION_ALL];
	/* Eyes L&R, Mouth, Smile, Blink L&R. -1280 - 1280*/
	res_score_type				scores[VRA_FF_SCORE_ALL];
};

/*
 *  * brief Full description of each Face in the output list -
 *   * include Facial information
 */
struct api_vra_out_face {
	struct api_vra_face_base_str	base;
	struct api_vra_facial_str	facial;
};

/* for Hybrid FD */
struct api_vra_pdt_rect {
	res_cord_type	left;
	res_cord_type	top;
	unsigned short	size;
};

struct api_vra_pdt_face_extra {
	unsigned char	is_rot: 1;	/* assuming VRA_ROT_ALL == 2 */
	unsigned char	is_yaw: 1;	/* assuming VRA_YAW_ALL == 2 */
	unsigned char	rot: 2;
	unsigned char	mirror_x: 1;	/* Required for facial features + set ROI commands */
	unsigned char	hw_rot_and_mirror: 3;	/* Required for facial features + set ROI commands */
};

struct api_vra_pdt_out_face {
	struct api_vra_pdt_rect			rect;
	res_score_type					score;
	struct api_vra_pdt_face_extra	extra;
};

/* brief Header of output list */
struct api_vra_out_list_info {
	/* The unique frame index of the results that are reported */
	vra_uint32				fr_ind;

	/*
	 * The input sizes of this frame.
	 * The Rects are described related to these coordiantes
	 */
	struct api_vra_sizes		in_sizes;

	unsigned short				zoom_mult_1000;

	/*
	 * Whether the data reprted with this ListInfo ius valid
	 * (it is not in case of corrupted frame)
	 */
	vra_boolean				valid_output;
};

struct api_vra_face_hybrid_str {
	/*!< Start from 1. Might be reset on Reset Output list */
	vra_uint32					unique_id;
	/*!< The face coordinates where full image is the input image */
	struct api_vra_rect_str		rect;
	/*!< The HW rotation of the face - combination of face rotation and image orientation */
	unsigned short				hw_rotation;
	/*!< Whether face found by the HW is Frontal (0), Right profile (90) or left profile (270) */
	unsigned short				hw_yaw;
};

struct api_vra_hybrid_hdr_str {
	/*!< The input sizes of this frame. The Rects are described related to these coordiantes */
	struct api_vra_sizes		in_sizes;
	vra_dram_adr_type			y_baseaddr;
	vra_dram_adr_type			u_baseaddr;
	vra_dram_adr_type			v_baseaddr;
	/*!< The offset in bytes between each 2 couple of lines in Y Map */
	unsigned short				y_linebyte_offset;
	/*!< VRA_YUV_FMT_422 /  VRA_YUV_FMT_420 / VRA_YUV_FMT_400 */
	enum api_vra_yuv_format			yuv_format;
	vra_boolean					is_uv_interleaved;
};

/*
 * FrameWork callbacks.
 * 2 of them are generic and the rest includes Sensor Handle
 * that is set on initialization and describes the relevant sensor
 */

/*
 * The function that is called after Abort request
 * when HW and FW are aborted
 */
typedef void (*on_frw_abort_ptr)(void);

/*
 * The function that is called from interrupts
 * when HW errors are detected
 *
 * param ChainIndex - the index of the HW chain that creates the error - 0 or 1
 * param ErrMask - mask of VRA_HW_ERR_* bits describes the error
 *
 * ChainIndex is expected to be 0 or 1
 */
typedef void (*on_frw_hw_err_ptr)(unsigned int ch_idx, unsigned int err_mask);

/*
 * The function that is called when hybrid FD is used.
 * TODO: need to implement
 */
/*! \brief The function that is called when external hybrid process should be invoked. If function pointer is set to NULL
 * - Hybrid process is disabled
 * \param FrameIndex - The unique internal frame index that its input is sent to hybrid process
 * \param NumHybridFaces - The number of faces sent to the hybrid process
 * \param FacesPtr - Pointer to list of faces information (its length is NumHybridFaces)
 * \param HybridHdrPtr - Pointer to hybrid list header
 */
typedef void (*on_frw_invoke_hybrid_pr_ptr)(vra_uint32 frame_index,
		unsigned int num_hybrid_faces,
		const struct api_vra_face_hybrid_str *hybrid_faces_ptr,
		const struct api_vra_hybrid_hdr_str *hybrid_hdr_ptr);

/*! \brief The function that is called after Abort request in order to abort the Hybrid process in case previous requests exist */
typedef void (*on_frw_abort_hybrid_pr_ptr)(void);

/*
 * The function that is called when output results of current frame are ready.
 * Notice that for ROI tracking this function is called after the ROI process
 * and no callback is called on "full frame" process.
 *
 * param SesnorHandle - The current Sensor Handle that (set on initialization)
 * param NumAllFaces - Total number of faces found
 * param FacesPtr - Pointer to list of faces information
 *		(its length is NumAllFaces)
 * param OutListInfoPtr - Pointer to output list header
 */
typedef void (*on_sen_final_output_ready_ptr)(vra_uint32 sensor_handle,
		unsigned int num_all_faces,
		const struct api_vra_out_face *faces_ptr,
		const struct api_vra_out_list_info *out_list_info_ptr);

/*! \brief The function that is called when output results of current frame are ready
 * - without FF results, without smoothing with previous frame results and not weighted
 *  with Hybrid results.
 *  Notice that for ROI tracking this function is called after the ROI process
 *  and no callback is called on "full frame" process.
 *
 * \param SesnorHandle - The current Sensor Handle that (set on initialization)
 * \param NumAllFaces - Total number of faces found
 * \param FacesPtr - Pointer to list of faces information (its length is NumAllFaces)
 * \param OutListInfoPtr - Pointer to output list header
 */
typedef void (*on_sen_crnt_fr_output_ready_ptr) (vra_uint32 sensor_handle,
		unsigned int num_all_faces,
		const struct api_vra_face_base_str *faces_ptr,
		const struct api_vra_out_list_info *out_list_info_ptr);

/*
 * The function that is called when output results of current frame are ready.
 * Notice that for ROI tracking this function is called after the ROI process
 * and no callback is called on "full frame" process.
 * \param SesnorHandle - The current Sensor Handle that (set on initialization)
 * \param NumAllFaces - Total number of faces found
 * \param FacesPtr - Pointer to list of faces information (its length is NumAllFaces)
 * \param OutListInfoPtr - Pointer to output list header
 */
typedef void (*on_sen_post_detect_ready_ptr) (vra_uint32 sensor_handle,
		unsigned int num_pdt_faces,
		const struct api_vra_pdt_out_face *faces_ptr,
		const struct api_vra_out_list_info *out_list_info_ptr);

/*
 * The function that is called when the entire frame input is finished.
 * In case of DRAM input the caller may now free the area
 * of the input frame in DRAM
 *
 * param SesnorHandle - The current Sensor Handle that (set on initialization)
 * param FrameIndex - The unique frame index that its input is finished
 * param BaseAddress - In case of DRAM input -
 *		the address of the input frame in the DRAM
 */
typedef void (*on_sen_end_input_proc_ptr)(vra_uint32 sensor_handle,
		vra_uint32 frame_index,
		vra_dram_adr_type base_address);

/*
 * The function that is called
 * when one of the errors of api_vra_sen_err_type occurs.
 *
 * param SesnorHandle - The current Sensor Handle that (set on initialization)
 * param ErrorType - The error
 * param AdditonalInfo - Additional information -
 *		for VRA_ERR_FRAME_LOST the process type that lost the frame,
 *		for VRA_ERR_HW_FACES_LOST the number of faces that were lost,
 *		for VRA_ERR_CORRUPTED_DATA the unique index of the frame
 */
typedef void (*on_sen_error_ptr)(vra_uint32 sensor_handle,
		enum api_vra_sen_err error_type,
		unsigned int additonal_info);

/*
 * The function that is called when statistics is ready.
 * It is called only in case of explicit user request.
 *
 * param SesnorHandle - The current Sensor Handle that (set on initialization)
 * param StatPtr - Pointer to statistics vector.
 *		The pointer is valid only as long as the function is active.
 * param StatSize32Bits - The size of the statistics vector pointed by StatPtr.
 * param StatType - The type of the statistics.
 */
typedef void (*on_sen_stat_collected_ptr)(vra_uint32 sensor_handle,
		vra_uint32 *stat_ptr,
		unsigned int stat_num_of_longs,
		unsigned int stat_type);

/* Framework callbacks */
struct vra_call_backs_str {
	on_frw_abort_ptr		frw_abort_func_ptr;
	on_frw_hw_err_ptr		frw_hw_err_func_ptr;
	on_frw_invoke_hybrid_pr_ptr	frw_invoke_hybrid_pr_ptr;
	on_frw_abort_hybrid_pr_ptr	frw_abort_hybrid_pr_ptr;
	on_sen_final_output_ready_ptr	sen_final_output_ready_ptr; /* HFD off(legacy VRA only) */
	on_sen_crnt_fr_output_ready_ptr	sen_crnt_fr_output_ready_ptr; /* unfilterd data for FDAF */
	on_sen_post_detect_ready_ptr	sen_post_detect_ready_ptr; /* HFD on */
	on_sen_end_input_proc_ptr	sen_end_input_proc_ptr;
	on_sen_error_ptr		sen_error_ptr;
	on_sen_stat_collected_ptr	sen_stat_collected_ptr;
};

/* Framework initialization input */
struct api_vra_fr_work_init {
	/* The base address of VRA HW registers */
	uintptr_t			hw_regs_base_adr;
	/* The interrupt type in the system */
	enum api_vra_int_type	int_type;

	/*
	 * API ASSUMPTION :
	 * All sensors input are sent through either CIN or DRAM.
	 * The input type is set on FrameWork initialization
	 * and can't be changed
	 */

	/* Whether the input is received from DRAM or from CIN */
	vra_boolean			dram_input;
	/* The callbacks structure */
	struct vra_call_backs_str	call_backs;

	/*
	 * Required for translate HW clocks to time
	 * for Load Balancing calculations
	 */
	unsigned short			hw_clock_freq_mhz;

	/*
	 * Required for translate SW clocks to time
	 * for Load Balancing calculations
	 */
	unsigned short			sw_clock_freq_mhz;

	/* API expected behavior */
	/* If set - VRA_OnNewFrame return VRA_Busy when transaction is active */
	vra_boolean			block_new_fr_on_transaction;

	/*
	 * If set - VRA_OnNewFrame return VRA_Busy
	 * when Input function is active
	 */
	vra_boolean			block_new_fr_on_input_set;

	/*
	 * If set - Set function that occurs while Input is copied -
	 * waits to copy End, else return VRA_Busy
	 */
	vra_boolean			wait_on_lock;

	/*
	 * If set - on reset output list the unique id of
	 * results is also reset - else unique id is kept
	 */
	vra_boolean			reset_uniqu_id_on_reset_list;

	/*
	 * If set - the entire output faces are forced to be inside the image
	 * (output pixels are cropped)
	 */
	vra_boolean			crop_faces_out_of_image_pixels;
};

/* Description of the RAM section that will be used for HW DMAs output */
struct api_vra_dma_ram {
	/* The base address of the RAM */
	vra_dram_adr_type	base_adr;
	/* The total size of the RAM - for all DMAs */
	vra_uint32		total_size;
	/* Whether the RAM that is allocated for DMAs usage is cached */
	vra_boolean		is_cached;
};

/* Input description relevant only for input from DRAM */
struct api_vra_input_dram_desc {
	unsigned char	pix_component_store_bits;
	/* 8 / 10 / 12 / 14 / 16 - for each YUV component */

	unsigned char	pix_component_data_bits;
	/*
	 * 8 / 10 / 12 / 14 / 16. (CIN always uses the 8 MSBs).
	 * Equal to PixComponentStoreBits
	 * unless the actual data is written in LSBs
	 */

	unsigned char	planes_num;
	/*
	 * 1 / 2 / 3 are allowed.
	 * FMT_400 requires 1 plane, VRA_YUV_FMT_420 requires 2 planes,
	 * For FMT_444 1 plane or 3 planes are allowed, for FMT_422 - 1 or 2
	 */

	vra_boolean	un_pack_data;
	/*
	 * Allowed only for VRA_YUV_FMT_444.
	 * When set - stands for 32 / 64 total bits per pixel
	 * when per 8 / 16 PixComponentStoreBits.
	 * That is each pixel include Y + U + V + dummy
	 */

	unsigned short	line_ofs_fst_plane;
	/* In Bytes - between each 2 lines in the 1st plane */

	unsigned short	line_ofs_other_planes;
	/* In bytes - relevant only for 2 planes input */

	vra_int32	adr_ofs_bet_planes;
	/*
	 * In bytes -
	 * the offset of 2nd plane / 3rd plane base to the 1st / 2nd plane.
	 * Relevant only for 2 / 3 planes input.
	 * It is signed since Y should be always the 1st plane
	 * and U + V data might be stored in lower address
	 */
};

/* Input frame description */
struct api_vra_input_desc {
	struct api_vra_sizes		sizes;
	enum api_vra_yuv_format		yuv_format;

	vra_boolean			u_before_v;
	/* True for UV data, False for VU data */

	unsigned char			hdr_lines;
	/*
	 * should be cropped before the DS for CIN input.
	 * Not relevant for DRAM input
	 */

	struct api_vra_input_dram_desc	dram;
	/* Data relevant only for input from DRAM */
};
#endif
