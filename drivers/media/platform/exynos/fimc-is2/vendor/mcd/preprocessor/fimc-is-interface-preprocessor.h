/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_PREPROCESSOR_INTERFACE_H
#define FIMC_IS_PREPROCESSOR_INTERFACE_H

#include "fimc-is-mem.h"
#include "fimc-is-config.h"
#include "exynos-fimc-is-sensor.h"
#include "fimc-is-metadata.h"
#include "fimc-is-binary.h"
#include "fimc-is-device-sensor.h"

#define PREPROCESSOR_INTERFACE_MAGIC 0x9FEDCBA8

typedef struct {
	bool bEnableLsc;
} SENCMD_CompanionEnableLscStr;

typedef struct {
	unsigned short usActuatorPos;
	unsigned short usOutdoorWeight;
	unsigned short usAwbRedGain;
	unsigned short usAwbGreenGain;
	unsigned short usAwbBlueGain;
	unsigned short usBv;
	unsigned short usOisCapture;
	unsigned short usLscPower;
	unsigned short usPGain;
	bool bHwSetEveryFrame;
}SENCMD_CompanionSetLsc3aaInfoStr;

typedef struct 
	{
	unsigned int enableWdr;
} SENCMD_CompanionEnableWdrStr;

typedef struct {
	unsigned int uiLongExposureCoarse;
	unsigned int uiLongExposureFine;
	unsigned int uiLongExposureAnalogGain;
	unsigned int uiLongExposureDigitalGain;
	unsigned int uiLongExposureCompanionDigitalGain;
	unsigned int uiShortExposureCoarse;
	unsigned int uiShortExposureFine;
	unsigned int uiShortExposureAnalogGain;
	unsigned int uiShortExposureDigitalGain;
	unsigned int uiShortExposureCompanionDigitalGain;
	bool bHwSetEveryFrame;
} SENCMD_CompanionSetWdrAeInfoStr;

typedef struct {
	unsigned int uiMode;
} SENCMD_CompanionChangeModeStr;

typedef struct
{
	unsigned int *pAfSetting;	/* pAfSetting[NUM_OF_73C3_AF_PARAMETERS] */
	unsigned int *pAfUpdate;	/* pAfUpdate[NUM_OF_73C3_AF_PARAMETERS] */
	unsigned int buf_size;		/* ex. buf_size = NUM_OF_73C3_AF_PARAMETERS */
	bool bHwSetEveryFrame;
}SENCMD_CompanionSetAfInfoStr_73C3;

typedef struct
{
	unsigned short readAddrOffset;
	unsigned short *pAddrArray;
	unsigned short *pBufArray;
	unsigned short bufSize;
}SENCMD_CompanionReadAfReg;

typedef struct
{
	u32 uUpdate;
	u32 uOnOff;
	u32 uMode;
}SENCMD_CompanionEnableBlockStr;

typedef struct
{
	u16 motion;
	bool bHwSetEveryFrame;
}SENCMD_CompanionSetMotionInfoStr;

typedef struct
{
	u16 uBypass;
	bool bHwSetEveryFrame;
}SENCMD_CompanionSetBypassInfoStr;

typedef struct
{
	u16 uLeMode;
	bool bHwSetEveryFrame;
}SENCMD_CompanionSetLeModeInfoStr;

typedef struct
{
	u16 uSeamlessMode;
	bool bHwSetEveryFrame;
}SENCMD_CompanionSetSeamlessModeInfoStr;

typedef struct
{
	u32 uUpdate;
	u32 uDrcGain;
	bool bHwSetEveryFrame;
}SENCMD_CompanionSetDrcInfoStr;

typedef struct
{
	u16 uIrStrength;
	bool bHwSetEveryFrame;
}SENCMD_CompanionSetIrInfoStr;

typedef struct
{
	u32 uRoiScale;
	bool bHwSetEveryFrame;
}SENCMD_CompanionSetRoiInfoStr;

typedef struct
{
	u8 *pAfData;
        u32 pAfDataSize;
	bool crcResult;
	bool isValid;	
}SENCMD_CompanionReadAfStatFromSpiStr;

typedef struct
{
	unsigned int cfgUpdateDelay;
}SENCMD_CompanionChangeCfgStr;

typedef struct
{
	u16 uPowerMode;
	bool bHwSetEveryFrame;
}SENCMD_CompanionPowerModeCfgStr;

typedef struct {
	void *param;
	unsigned int return_value;
} preproc_setting_info;

struct fimc_is_preprocessor_interface;

struct fimc_is_preprocessor_interface_ops {
	int (*set_af)(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionSetAfInfoStr_73C3 *param);
	int (*enable_block)(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionEnableBlockStr *param); /* C73C3_BlockOnOff() */
	int (*set_lsc_3aa)(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionSetLsc3aaInfoStr *param);
	int (*set_wdr_ae)(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionSetWdrAeInfoStr *param);
	int (*set_motion)(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionSetMotionInfoStr *param);
	int (*set_bypass)(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionSetBypassInfoStr *param);
	int (*set_seamless_mode)(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionSetSeamlessModeInfoStr *param);
	int (*set_drc)(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionSetDrcInfoStr *param);
	int (*set_ir)(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionSetIrInfoStr *param);
	int (*set_roi)(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionSetRoiInfoStr *param);
	int (*set_power_mode)(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionPowerModeCfgStr *param);
	int (*change_config)(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionChangeCfgStr *param);
	int (*read_af_stat)(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionReadAfStatFromSpiStr *param);
	int (*read_af_reg)(struct fimc_is_preprocessor_interface *itf, SENCMD_CompanionReadAfReg *param);
	int (*get_ae_gains)(struct fimc_is_preprocessor_interface *itf, u32 **preproc_auto_exposure);
};

struct fimc_is_preprocessor_interface {
	u32						magic;
	struct fimc_is_preprocessor_interface_ops	preproc_itf_ops;
};

#ifdef CONFIG_VENDER_MCD
typedef int (*preprocessor_func_type)(struct v4l2_subdev *subdev, void *param);
#endif

struct fimc_is_preprocessor_ops {
#ifdef CONFIG_VENDER_MCD
	int (*preprocessor_init)(struct v4l2_subdev *subdev);
	int (*preprocessor_log_status)(struct v4l2_subdev *subdev);
#endif
	int (*preprocessor_stream_on)(struct v4l2_subdev *subdev);
	int (*preprocessor_stream_off)(struct v4l2_subdev *subdev);
	int (*preprocessor_mode_change)(struct v4l2_subdev *subdev, struct fimc_is_device_sensor *device);
	int (*preprocessor_debug)(struct v4l2_subdev *subdev);
	int (*preprocessor_wait_s_input)(struct v4l2_subdev *subdev);
	int (*preprocessor_deinit)(struct v4l2_subdev *subdev);
	int (*preprocessor_s_format)(struct v4l2_subdev *subdev, struct fimc_is_device_sensor *device);
	int (*preprocessor_set_le_mode)(struct v4l2_subdev *subdev, void *param);
#ifdef CONFIG_VENDER_MCD
	/*int (*preprocessor_set_size)(struct v4l2_subdev *subdev, preprocessor_shared_data *preprocessor_data);*/
	preprocessor_func_type preprocessor_set_af;
	preprocessor_func_type preprocessor_enable_block;
	preprocessor_func_type preprocessor_set_lsc_3aa;
	preprocessor_func_type preprocessor_wdr_ae;
	preprocessor_func_type preprocessor_set_motion;
	preprocessor_func_type preprocessor_set_bypass;
	preprocessor_func_type preprocessor_set_seamless_mode;
	preprocessor_func_type preprocessor_set_drc;
	preprocessor_func_type preprocessor_set_ir;
	preprocessor_func_type preprocessor_set_roi;
	preprocessor_func_type preprocessor_set_power_mode;
	preprocessor_func_type preprocessor_change_config;
	preprocessor_func_type preprocessor_read_af_stat;
	preprocessor_func_type preprocessor_read_af_reg;
#endif
};

int init_preprocessor_interface(struct fimc_is_preprocessor_interface *itf);
#endif
