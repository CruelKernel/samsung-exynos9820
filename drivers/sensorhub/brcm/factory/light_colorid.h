#ifndef __SSP_FACTORY_LIGHT_COLORID_H__

#define __SSP_FACTORY_LIGHT_COLORID_H__

#include "../ssp.h"

#define COLOR_ID_PATH		"/sys/class/lcd/panel/window_type"


/*
 *        file name	: version
 *
 *        contents	: YYMMDD.ColorIDs
 */


#define VERSION_FILE_NAME	"/efs/FactoryApp/version"

/*
 *        file name : predefine+ColorID
 *        contents  : dgf,r_coef,g_coef,b_coef,c_coef,cct_coef,cct_offset,threshhigh,threshlow,thresh_alert,crc
 */

#define PREDEFINED_FILE_NAME		"/efs/FactoryApp/predefine"
#define COLOR_NUM			2
#define COLOR_NUM_MAX		16

#define COLOR_ID_VERSION_FILE_LENGTH		35

#define COLOR_ID_PREDEFINED_FILE_LENGTH		65

#define COLOR_ID_IDS_LENGTH					20

#define COLOR_ID_PREDEFINED_FILENAME_LENGTH	65

#define WRITE_ALL_DATA_NUMBER				12

#define L_CUT_THRESHOLD_HIGH				3300
#define L_CUT_THRESHOLD_LOW					2200

#define L_CUT_THRESHOLD_HIGH_B1				7000
#define L_CUT_THRESHOLD_LOW_B1				5000

#define REV06A_THRESHOLD_HIGH				320
#define REV06A_THRESHOLD_LOW				250

#define B0		0
#define B1		1
#define B2		2
#define BX		3

ssize_t light_hiddendhole_version_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t light_hiddenhole_version_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);
ssize_t light_hiddendhole_hh_write_all_data_show(struct device *dev,	struct device_attribute *attr, char *buf);
ssize_t light_hiddenhole_write_all_data_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size);
ssize_t light_hiddendhole_is_exist_efs_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t light_hiddendhole_check_coef_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t light_hiddendhole_check_coef_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size);

#endif
