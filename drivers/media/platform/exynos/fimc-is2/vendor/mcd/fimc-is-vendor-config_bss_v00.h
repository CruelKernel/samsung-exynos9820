#ifndef FIMC_IS_VENDOR_CONFIG_BSS_V00_H
#define FIMC_IS_VENDOR_CONFIG_BSS_V00_H

#include "fimc-is-eeprom-rear-2l4_v010.h"
#include "fimc-is-eeprom-front-3j1_v004.h"

#define VENDER_PATH

#define CAMERA_REAR_DUAL_CAL
#define CAMERA_REAR2
#define CAMERA_REAR2_TILT
#define CAMERA_REAR2_MODULEID

#define CAMERA_USE_OIS_VDD_1_8V

#define CAMERA_OIS_DOM_UPDATE_VERSION 'O'
#define CAMERA_OIS_SEC_UPDATE_VERSION 'P'

#define CAMERA_MODULE_DUAL_CAL_AVAILABLE_VERSION 'A'
#define FIMC_IS_MAX_FW_BUFFER_SIZE (4100 * 1024)

#define CAMERA_OIS_GYRO_OFFSET_SPEC 10000

#define RTA_CODE_AREA_SIZE (0x00180000)

#define USE_CAMERA_EMBEDDED_HEADER

#define USE_CAMERA_MIPI_CLOCK_VARIATION
#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
/*#define USE_CAMERA_MIPI_CLOCK_VARIATION_RUNTIME*/
#endif

#define USE_CAMERA_FACTORY_DRAM_TEST

#ifndef CONFIG_SEC_FACTORY
#define USE_CAMERA_PREPARE_RETENTION_ON_BOOT
#endif

#ifdef CONFIG_SEC_FACTORY
#define USE_CAMERA_CHECK_SENSOR_REV
#endif

#define USE_CAMERA_HW_BIG_DATA

#ifdef USE_CAMERA_HW_BIG_DATA
/* #define CAMERA_HW_BIG_DATA_FILE_IO */
/* #define CSI_SCENARIO_COMP		(0) This value follows dtsi */
#define CSI_SCENARIO_SEN_FRONT	(1)
#define CSI_SCENARIO_TELE		(2)
#define CSI_SCENARIO_SECURE		(3)
#define CSI_SCENARIO_SEN_REAR	(0)
#endif

#define USE_AF_SLEEP_MODE

/* It should be align with DDK and RTA side */
#define USE_NEW_PER_FRAME_CONTROL

/* define supported aperture level */
#define ROM_SUPPORT_APERTURE_F2	// Second step of aperture.

/* Tele sensor crop shift and OIS calibration will be applied instead of this feature in this project */
/* #define OIS_CENTERING_SHIFT_ENABLE */
#undef ENABLE_DYNAMIC_MEM

#define APERTURE_CLOSE_VALUE F1_5

#if defined(CONFIG_USE_CAMERA_LDU) || defined(CONFIG_SEC_FACTORY)
#define USE_OIS_SHIFT_FOR_APERTURE
#endif

#ifdef USE_OIS_SHIFT_FOR_APERTURE
#if defined(CONFIG_SEC_FACTORY)
#define OIS_SHIFT_OFFSET_VALUE_NORMAL 0
#else
#define OIS_SHIFT_OFFSET_VALUE_NORMAL 1000
#endif
#endif

#define USE_BUCK2_REGULATOR_CONTROL

#if ANDROID_VERSION >= 100000 /* Over Q */
#define USE_SENSOR_LONG_EXPOSURE_SHOT
#endif

#endif /* FIMC_IS_VENDOR_CONFIG_BSS_V00_H */
