/*
 *  Copyright (C) 2016, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */
#include "light_colorid.h"

enum {
	COLOR_ID_UTYPE = 0,
	COLOR_ID_BLACK = 1,
	COLOR_ID_WHITE = 2,
	COLOR_ID_GOLD = 3,
	COLOR_ID_SILVER = 4,
	COLOR_ID_GREEN = 5,
	COLOR_ID_BLUE = 6,
	COLOR_ID_PINKGOLD = 7,
	COLOR_ID_DEFAULT,
} COLOR_ID_INDEX;

struct light_coef_predefine_item {
	int version;
	int color_id;
	int coef[7];	/* r_coef,g_coef,b_coef,c_coef,dgf,cct_coef,cct_offset */
	unsigned int thresh[2];	/* high,low*/
	unsigned int thresh_alert;	/* high*/
};

#ifdef CONFIG_SENSORS_SSP_GREAT
struct light_coef_predefine_item light_coef_predefine_table[COLOR_NUM+1] = {
	{170609,	COLOR_ID_DEFAULT,	{-170, 80, -290, 1000, 2266, 1389, 1372},	{2300, 1000}, 550},
	{170609,	COLOR_ID_UTYPE,		{-170, 80, -290, 1000, 2266, 1389, 1372},	{2300, 1000}, 550},
	{170609,	COLOR_ID_BLACK,		{-170, 80, -290, 1000, 2266, 1389, 1372},	{2300, 1000}, 550},
};
#else
struct light_coef_predefine_item light_coef_predefine_table[COLOR_NUM+1] = {
	{190610,	COLOR_ID_DEFAULT,	{-888, 217, -444, 1115, 3673, 1112, 1370},	{0, 0}, 550},
	{190610,	COLOR_ID_UTYPE,		{-888, 217, -444, 1115, 3673, 1112, 1370},	{0, 0}, 550},
	{190610,	COLOR_ID_BLACK,		{-888, 217, -444, 1115, 3673, 1112, 1370},	{0, 0}, 550},
};
#endif

/* Color ID functions */

int light_corloid_read_colorid(void)
{
	int iRet = 0;
	mm_segment_t old_fs;
	struct file *colorid_filp = NULL;
	int color_id;
	char buf[8] = {0, };
	char temp;
	char *token, *str;

	//read current colorid
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	colorid_filp = filp_open(COLOR_ID_PATH, O_RDONLY, 0660);
	if (IS_ERR(colorid_filp)) {
		iRet = PTR_ERR(colorid_filp);
		pr_err("[SSP] %s - Can't open colorid file %d\n",
				__func__, iRet);
		set_fs(old_fs);
		return iRet;
	}

	iRet = vfs_read(colorid_filp,
		(char *)buf, sizeof(char)*8, &colorid_filp->f_pos);
	if (iRet < 0) {
		pr_err("[SSP] %s - Can't read the colorid data\n", __func__);
		filp_close(colorid_filp, current->files);
		set_fs(old_fs);
		return iRet;
	}

	filp_close(colorid_filp, current->files);
	set_fs(old_fs);

	// 00 00 00  or ff ff ff is not connected
	if ((strcmp("00 00 00", buf) == 0) || (strcmp("ff ff ff", buf) == 0)) {
		pr_err("[SSP] %s : lcd is not connected", __func__);
		return ERROR;
	}

	str = buf;
	token = strsep(&str, " ");
	iRet = kstrtou8(token, 16, &temp);
	if (iRet < 0)
		pr_err("[SSP] %s - colorid kstrtou8 error %d", __func__, iRet);

	color_id = (int)(temp&0x0F);

	pr_info("[SSP] %s - colorid = %d\n", __func__, color_id);

	return color_id;
}

int light_colorid_read_efs_version(char buf[], int buf_length)
{
	int iRet = 0;
	mm_segment_t old_fs;
	struct file *version_filp = NULL;

	memset(buf, 0, sizeof(char)*buf_length);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	version_filp = filp_open(VERSION_FILE_NAME, O_RDONLY, 0660);

	if (IS_ERR(version_filp)) {
		iRet = PTR_ERR(version_filp);
		pr_err("[SSP] %s - Can't open version file %d\n", __func__, iRet);
		set_fs(old_fs);
		return iRet;
	}

	iRet = vfs_read(version_filp,
		(char *)buf, buf_length * sizeof(char), &version_filp->f_pos);


	if (iRet < 0)
		pr_err("[SSP] %s - Can't read the version data %d\n", __func__, iRet);

	filp_close(version_filp, current->files);
	set_fs(old_fs);

	pr_info("[SSP] %s - %s\n", __func__, buf);

	return iRet;
}

int light_colorid_write_efs_version(int version, char *color_ids)
{
	int iRet = 0;
	mm_segment_t old_fs;
	struct file *version_filp = NULL;
	char file_contents[COLOR_ID_VERSION_FILE_LENGTH] = {0, };

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	snprintf(file_contents, sizeof(file_contents), "%d.%s", version, color_ids);

	version_filp = filp_open(VERSION_FILE_NAME,
			O_CREAT | O_TRUNC | O_WRONLY | O_SYNC, 0660);

	if (IS_ERR(version_filp)) {
		pr_err("[SSP] %s - Can't open version file\n", __func__);
		set_fs(old_fs);
		iRet = PTR_ERR(version_filp);
		return iRet;
	}

	iRet = vfs_write(version_filp, (char *)&file_contents,
		sizeof(char)*COLOR_ID_VERSION_FILE_LENGTH, &version_filp->f_pos);
	if (iRet < 0) {
		pr_err("[SSP] %s - Can't write the version data to file\n", __func__);
		iRet = -EIO;
	}

	filp_close(version_filp, current->files);
	set_fs(old_fs);

	return iRet;
}

bool light_colorid_check_efs_version(void)
{
	int iRet = 0, i;
	int efs_version;
	char str_efs_version[COLOR_ID_VERSION_FILE_LENGTH];

	pr_info("[SSP] %s ", __func__);

	iRet = light_colorid_read_efs_version(str_efs_version, COLOR_ID_VERSION_FILE_LENGTH);
	if (iRet < 0) {
		pr_err("[SSP] %s - version read failed %d\n", __func__, iRet);
		return false;
	}

	/* version parsing */
	{
		char *token;
		char *str = str_efs_version;

		token = strsep(&str, ".");
		iRet = kstrtos32(token, 10, &efs_version);
		if (iRet < 0)
			pr_err("[SSP] %s - version kstros32 error %d\n", __func__, iRet);

		pr_info("[SSP] %s - version %d\n", __func__, efs_version);
	}

	for (i = 1; i <= COLOR_NUM; i++) {
		if (efs_version < light_coef_predefine_table[i].version) {
			pr_err("[SSP] %s - %d false\n", __func__, i);
			return false;
		}
	}

	pr_info("[SSP] %s - success\n", __func__);

	return true;
}

int light_colorid_write_predefined_file(struct ssp_data *data, int color_id, int coef[], int thresh[],  int thresh_alert)
{
	int iRet = 0, crc;
	mm_segment_t old_fs;
	struct file *coef_filp = NULL;
	char file_name[COLOR_ID_PREDEFINED_FILENAME_LENGTH] = {0, };
	char file_contents[COLOR_ID_PREDEFINED_FILE_LENGTH] = {0, };

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	snprintf(file_name, sizeof(file_name), "%s%d", PREDEFINED_FILE_NAME, color_id);

	// this value is temporary. Origainal value is in light_coef_predefine_table array.
	// we normally use light_coef_predefine_table array.
	if(data->ap_type == B0) {
		if(data->ap_rev < 23) {
			thresh[0]= REV06A_THRESHOLD_HIGH;
			thresh[1] = REV06A_THRESHOLD_LOW;
		}
	}
	else if(data->ap_type == B1) {
		if (data->ap_rev < 20) {
			thresh[0] = L_CUT_THRESHOLD_HIGH_B1;
			thresh[1] = L_CUT_THRESHOLD_LOW_B1;
		}
		else if((data->ap_rev >= 20 && data->ap_rev < 23) || 
				(data->ap_rev == 24)) {
			thresh[0]= REV06A_THRESHOLD_HIGH;
			thresh[1]= REV06A_THRESHOLD_LOW;
		}
	}
	else if (data->ap_type == B2) {
		if (data->ap_rev <= 1) {
			thresh[0] = L_CUT_THRESHOLD_HIGH;
			thresh[1] = L_CUT_THRESHOLD_LOW;
		}
		else if((data->ap_rev > 1 && data->ap_rev < 23) || 
				(data->ap_rev == 24)) {
			thresh[0]= REV06A_THRESHOLD_HIGH;
			thresh[1]= REV06A_THRESHOLD_LOW;
		}
	}

	crc = coef[0] + coef[1] + coef[2] + coef[3] + coef[4] + coef[5] + coef[6]
		+ thresh[0] + thresh[1] + thresh_alert;

	snprintf(file_contents, sizeof(file_contents), "%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%d",
		coef[4], coef[0], coef[1], coef[2], coef[3], coef[5], coef[6], thresh[0], thresh[1], thresh_alert, crc);

	coef_filp = filp_open(file_name,
			O_CREAT | O_TRUNC | O_WRONLY | O_SYNC, 0660);

	if (IS_ERR(coef_filp)) {
		pr_err("[SSP] %s - Can't open coef file %d\n", __func__, color_id);
		set_fs(old_fs);
		iRet = PTR_ERR(coef_filp);
		return iRet;
	}

	iRet = vfs_write(coef_filp, (char *)file_contents,
			sizeof(char)*COLOR_ID_PREDEFINED_FILE_LENGTH, &coef_filp->f_pos);

	if (iRet < 0) {
		pr_err("[SSP] %s - Can't write the coef data to file\n", __func__);
		iRet = -EIO;
	}

	filp_close(coef_filp, current->files);
	set_fs(old_fs);

	return iRet;
}

int light_colorid_read_predefined_file(int color_id, int coef[], unsigned int thresh[], unsigned int *thresh_alert)
{
	int iRet = 0, crc, i;
	mm_segment_t old_fs;
	struct file *coef_filp = NULL;
	char file_name[COLOR_ID_PREDEFINED_FILENAME_LENGTH] = {0, };
	char file_contents[COLOR_ID_PREDEFINED_FILE_LENGTH] = {0, };
	char *token, *str;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	snprintf(file_name, sizeof(file_name), "%s%d", PREDEFINED_FILE_NAME, color_id);

	coef_filp = filp_open(file_name, O_RDONLY, 0660);

	if (IS_ERR(coef_filp)) {
		pr_err("[SSP] %s - Can't open coef file %d\n", __func__, color_id);
		set_fs(old_fs);
		iRet = PTR_ERR(coef_filp);
		memcpy(coef, light_coef_predefine_table[0].coef, sizeof(int)*7);
		memcpy(thresh, light_coef_predefine_table[0].thresh, sizeof(unsigned int)*2);
		memcpy(thresh_alert, &light_coef_predefine_table[0].thresh_alert, sizeof(unsigned int));
		return iRet;
	}

	iRet = vfs_read(coef_filp,
		(char *)file_contents, sizeof(char)*COLOR_ID_PREDEFINED_FILE_LENGTH, &coef_filp->f_pos);

	if (iRet != sizeof(char)*COLOR_ID_PREDEFINED_FILE_LENGTH) {
		pr_err("[SSP] %s - Can't read the coef data to file\n", __func__);
		memcpy(coef, light_coef_predefine_table[0].coef, sizeof(int)*7);
		memcpy(thresh, light_coef_predefine_table[0].thresh, sizeof(unsigned int)*2);
		memcpy(thresh_alert, &light_coef_predefine_table[0].thresh_alert, sizeof(unsigned int));
		filp_close(coef_filp, current->files);
		set_fs(old_fs);
		return -EIO;
	}

	filp_close(coef_filp, current->files);
	set_fs(old_fs);

	//parsing
	str = file_contents;

	for (i = 0 ; i < 7; i++) {
		int temp;

		token = strsep(&str, ",\n");
		if (token == NULL) {
			pr_err("[SSP] %s - too few arguments %d(8 needed)\n", __func__, i);
			memcpy(coef, light_coef_predefine_table[0].coef, sizeof(int)*7);
			memcpy(thresh, light_coef_predefine_table[0].thresh, sizeof(unsigned int)*2);
			memcpy(thresh_alert, &light_coef_predefine_table[0].thresh_alert, sizeof(unsigned int));
			return -EINVAL;
		}

		iRet = kstrtos32(token, 10, &temp);

		/*
		 *change order of coef array
		 *        file	: dgf,r_coef,g_coef,b_coef,c_coef,cct_coef,cct_offset
		 *        array	: r_coef,g_coef,b_coef,c_coef,dgf,cct_coef,cct_offset
		 */
		if (i == 0)		/* dgf */
			coef[4] = temp;
		else if (i <= 4)	/* r_coef,g_coef,b_coef,c_coef */
			coef[i-1] = temp;
		else			/* cct_coef,cct_offset */
			coef[i] = temp;

		if (iRet < 0) {
			pr_err("[SSP] %s - kstrtos32 error %d\n", __func__, iRet);
			memcpy(coef, light_coef_predefine_table[0].coef, sizeof(int)*7);
			memcpy(thresh, light_coef_predefine_table[0].thresh, sizeof(unsigned int)*2);
			memcpy(thresh_alert, &light_coef_predefine_table[0].thresh_alert, sizeof(unsigned int));
			return iRet;
		}
	}

	for (i = 0 ; i < 2; i++) {
		token = strsep(&str, ",\n");
		if (token == NULL) {
			pr_err("[SSP] %s - too few arguments %d(2 needed)\n", __func__, i);
			memcpy(coef, light_coef_predefine_table[0].coef, sizeof(int)*7);
			memcpy(thresh, light_coef_predefine_table[0].thresh, sizeof(unsigned int)*2);
			memcpy(thresh_alert, &light_coef_predefine_table[0].thresh_alert, sizeof(unsigned int));
			return -EINVAL;
		}

		iRet = kstrtou32(token, 10, &thresh[i]);

		if (iRet < 0) {
			pr_err("[SSP] %s - kstrtou32 error %d\n", __func__, iRet);
			memcpy(coef, light_coef_predefine_table[0].coef, sizeof(int)*7);
			memcpy(thresh, light_coef_predefine_table[0].thresh, sizeof(unsigned int)*2);
			memcpy(thresh_alert, &light_coef_predefine_table[0].thresh_alert, sizeof(unsigned int));
			return iRet;
		}
	}

	// for prox alert th
	token = strsep(&str, ",\n");
	if (token == NULL) {
		pr_err("[SSP] %s - too few arguments (1 needed)\n", __func__);
		memcpy(coef, light_coef_predefine_table[0].coef, sizeof(int)*7);
		memcpy(thresh, light_coef_predefine_table[0].thresh, sizeof(unsigned int)*2);
		memcpy(thresh_alert, &light_coef_predefine_table[0].thresh_alert, sizeof(unsigned int));
		return -EINVAL;
	}

	iRet = kstrtou32(token, 10, thresh_alert);

	if (iRet < 0) {
		pr_err("[SSP] %s - kstrtou32 error %d\n", __func__, iRet);
		memcpy(coef, light_coef_predefine_table[0].coef, sizeof(int)*7);
		memcpy(thresh, light_coef_predefine_table[0].thresh, sizeof(unsigned int)*2);
		memcpy(thresh_alert, &light_coef_predefine_table[0].thresh_alert, sizeof(unsigned int));
		return iRet;
	}

	token = strsep(&str, ",\n");
	iRet = kstrtos32(token, 10, &crc);

	if (crc != (coef[0] + coef[1] + coef[2] + coef[3] + coef[4] + coef[5] + coef[6]
				+ thresh[0] + thresh[1] + (*thresh_alert))) {
		pr_err("[SSP] %s - crc error %d(%d %d %d %d %d %d %d %u %u %u)\n", __func__,
			crc, coef[0], coef[1], coef[2], coef[3], coef[4], coef[5], coef[6],
			thresh[0], thresh[1], (*thresh_alert));

		//use default coef
		memcpy(coef, light_coef_predefine_table[0].coef, sizeof(int)*7);
		memcpy(thresh, light_coef_predefine_table[0].thresh, sizeof(unsigned int)*2);
		memcpy(thresh_alert, &light_coef_predefine_table[0].thresh_alert, sizeof(unsigned int));
		return -EINVAL;
	}

	return iRet;
}



int light_colorid_write_all_predefined_data(struct ssp_data *data)
{
	int i, version = -1, iRet = 0;
	char str_ids[COLOR_ID_IDS_LENGTH] = "";

	pr_info("[SSP] %s\n", __func__);

	for (i = 1 ; i <= COLOR_NUM ; i++) {
		char buf[5];

		iRet = light_colorid_write_predefined_file(
			data,
			light_coef_predefine_table[i].color_id,
			light_coef_predefine_table[i].coef,
			light_coef_predefine_table[i].thresh,
			light_coef_predefine_table[i].thresh_alert);
		if (iRet < 0) {
			pr_err("[SSP] %s - %d write file err %d\n", __func__, i, iRet);
			break;
		}
		if (version < light_coef_predefine_table[i].version)
			version = light_coef_predefine_table[i].version;

		snprintf(buf, sizeof(buf), "%x", light_coef_predefine_table[i].color_id);
		strcat(str_ids, buf);
	}

	if (iRet >= 0) {
		iRet = light_colorid_write_efs_version(version, str_ids);
		if (iRet < 0)
			pr_err("[SSP] %s - write version file err %d\n", __func__, iRet);
	}

	return iRet;
}

void initialize_light_colorid_do_task(struct work_struct *work)
{
	struct ssp_data *data = container_of((struct delayed_work *)work,
	                                     struct ssp_data, work_ssp_light_efs_file_init);

	pr_info("[SSP] %s, is called efs file status %d \n", __func__, data->light_efs_file_status);

	if (data->light_efs_file_status < 0) {
		if (initialize_light_colorid(data) < 0)
			pr_err("[SSP]: %s - initialize light colorid failed\n", __func__);
	}
	
}

int initialize_light_colorid(struct ssp_data *data)
{
	int color_id, ret;
	unsigned int thresh[2];
	unsigned int thresh_alert = 0;
	data->light_efs_file_status = 0;

	if (!light_colorid_check_efs_version())
		light_colorid_write_all_predefined_data(data);

	color_id = light_corloid_read_colorid();

	ret = light_colorid_read_predefined_file(color_id, data->light_coef, thresh, &thresh_alert);
	if (ret < 0) {
		data->light_efs_file_status = ret;
		pr_err("[SSP] %s - read predefined file failed : ret %d\n", __func__, data->light_efs_file_status);
	}

#if defined(CONFIG_SENSORS_SSP_BEYOND)
	// if efs file can't open and read
	if (data->light_efs_file_status < 0) {

		if(data->ap_type == B0) {
			if(data->ap_rev < 23) {
				data->uProxHiThresh = REV06A_THRESHOLD_HIGH;
				data->uProxLoThresh = REV06A_THRESHOLD_LOW;
			}
			else {
				data->uProxHiThresh = thresh[0];
				data->uProxLoThresh = thresh[1];
			}
		}
		else if(data->ap_type == B1) {
			if (data->ap_rev < 20) {
				data->uProxHiThresh = L_CUT_THRESHOLD_HIGH_B1;
				data->uProxLoThresh = L_CUT_THRESHOLD_LOW_B1;
			}
			else if((data->ap_rev >= 20 && data->ap_rev < 23) || 
				(data->ap_rev == 24)) {
				data->uProxHiThresh = REV06A_THRESHOLD_HIGH;
				data->uProxLoThresh = REV06A_THRESHOLD_LOW;
			}
			else {
				data->uProxHiThresh = thresh[0];
				data->uProxLoThresh = thresh[1];
			}
		}
		else if (data->ap_type == B2) {
			if (data->ap_rev <= 1) {
				data->uProxHiThresh = L_CUT_THRESHOLD_HIGH;
				data->uProxLoThresh = L_CUT_THRESHOLD_LOW;
			}
			else if((data->ap_rev > 1 && data->ap_rev < 23) || 
					(data->ap_rev == 24)) {
				data->uProxHiThresh = REV06A_THRESHOLD_HIGH;
				data->uProxLoThresh = REV06A_THRESHOLD_LOW;
			}
			else {
				data->uProxHiThresh = thresh[0];
				data->uProxLoThresh = thresh[1];
			}
		}
		else {//if(data->ap_type == BX) {
			if (data->ap_rev < 4) {
				data->uProxHiThresh = REV06A_THRESHOLD_HIGH;
				data->uProxLoThresh = REV06A_THRESHOLD_LOW;
			}
			else {
				data->uProxHiThresh = thresh[0];
				data->uProxLoThresh = thresh[1];
			}
		}
	}
	else {
		data->uProxHiThresh = thresh[0];
		data->uProxLoThresh = thresh[1];
	}


	if(data->ap_type == B0) {
		data->uProxHiThresh_detect = DEFAULT_DETECT_HIGH_THRESHOLD;
		data->uProxLoThresh_detect = data->uProxLoThresh;
	}
	else if(data->ap_type == B1) {
		if (data->ap_rev < 20) {
			data->uProxHiThresh_detect = DEFAULT_DETECT_HIGH_THRESHOLD;
			data->uProxLoThresh_detect = L_CUT_THRESHOLD_LOW_B1;
		}
		else if((data->ap_rev >= 20 && data->ap_rev < 23) || 
				(data->ap_rev == 24)) {
			data->uProxHiThresh_detect = DEFAULT_DETECT_HIGH_THRESHOLD;
			data->uProxLoThresh_detect = REV06A_THRESHOLD_LOW;
		}
		else {
			data->uProxHiThresh_detect = DEFAULT_DETECT_HIGH_THRESHOLD;
			data->uProxLoThresh_detect = data->uProxLoThresh;
		}
	}
	else if(data->ap_type == B2) {
		if(data->ap_rev <= 1) {
			data->uProxHiThresh_detect = DEFAULT_DETECT_HIGH_THRESHOLD;
			data->uProxLoThresh_detect = DEFAULT_DETECT_LOW_THRESHOLD_FOR_LCUT;
		}
		else if((data->ap_rev > 1 && data->ap_rev < 23) || 
				(data->ap_rev == 24)) {
			data->uProxHiThresh_detect = DEFAULT_DETECT_HIGH_THRESHOLD;
			data->uProxLoThresh_detect = REV06A_THRESHOLD_LOW;
		}
		else {
			data->uProxHiThresh_detect = DEFAULT_DETECT_HIGH_THRESHOLD;
			data->uProxLoThresh_detect = data->uProxLoThresh;
		}
	} else {
		if(data->ap_rev < 4) {
			data->uProxHiThresh_detect = DEFAULT_DETECT_HIGH_THRESHOLD;
			data->uProxLoThresh_detect = REV06A_THRESHOLD_LOW;
		}
		else {
			data->uProxHiThresh_detect = DEFAULT_DETECT_HIGH_THRESHOLD;
			data->uProxLoThresh_detect = data->uProxLoThresh;
		}
	}
#else
		data->uProxHiThresh = thresh[0];
		data->uProxLoThresh = thresh[1];
		data->uProxHiThresh_detect = DEFAULT_DETECT_HIGH_THRESHOLD;
		data->uProxLoThresh_detect = DEFAULT_DETECT_LOW_THRESHOLD;
#endif

	data->uProxAlertHiThresh = thresh_alert;

	set_light_coef(data);

	pr_info("[SSP] %s - finished id %d, ap-type %d, ap-rev %d, efs file status %d, threshold %d %d %d %d \n"
		, __func__, color_id, data->ap_type, data->ap_rev, data->light_efs_file_status, 
		data->uProxHiThresh, data->uProxLoThresh, data->uProxHiThresh_detect, data->uProxLoThresh_detect);

	return 0;
}


/*************************************************************************/
/* Color ID HiddenHole Sysfs                                             */
/*************************************************************************/
ssize_t light_hiddendhole_version_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char efs_version[COLOR_ID_VERSION_FILE_LENGTH] = "";
	char str_ids[COLOR_ID_IDS_LENGTH] = "";
	char idsbuf[5];
	int i, version = -1, iRet = 0;
	char *str, temp[2];
	int coef[7], err = 0, length;
	u8 colorid;
	unsigned int thresh[2];
	unsigned int thresh_alert = 0;

	for (i = 1; i <= COLOR_NUM; i++) {
		if (version < light_coef_predefine_table[i].version)
			version = light_coef_predefine_table[i].version;

		snprintf(idsbuf, sizeof(idsbuf), "%x", light_coef_predefine_table[i].color_id);
		strcat(str_ids, idsbuf);
	}

	iRet = light_colorid_read_efs_version(efs_version, COLOR_ID_VERSION_FILE_LENGTH);
	if (iRet < 0) {
		pr_err("[SSP]: %s - version read failed %d\n", __func__, iRet);
		return -EIO;
	}

	str = efs_version;
	strsep(&str, ".");
	length = strlen(str);

	for (i = 0; i < length; i++) {
		temp[0] = str[i];
		temp[1] = '\0';
		iRet = kstrtou8(temp, 16, &colorid);
		if (iRet < 0) {
			pr_err("[SSP] %s - colorid %c kstrtou8 error %d\n", __func__, str[i], iRet);
			err = 1;
			break;
		}

		iRet = light_colorid_read_predefined_file((int)colorid, coef, thresh, &thresh_alert);
		if (iRet < 0) {
			err = 1;
			pr_err("[SSP] %s - read coef fail, err = %d\n", __func__, err);
			break;
		}
	}

	if (err == 1)
		return snprintf(buf, PAGE_SIZE, "%d,%d\n", 0, 0);
	else
		return snprintf(buf, PAGE_SIZE, "P%s.%s,P%d.%s\n", efs_version, str, version, str_ids);
}

ssize_t light_hiddenhole_version_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	char temp;
	int version = -1, iRet = 0;
	char str_ids[COLOR_ID_IDS_LENGTH] = "";
	char *str;

	pr_info("[SSP] %s - %s\n", __func__, buf);

	iRet = sscanf(buf, "%1c%6d", &temp, &version);
	if (iRet < 0)
		return iRet;

	str = (char *)buf;
	strsep(&str, ".");

	if (str == NULL) {
		pr_err("[SSP] %s - ids str is nullw\n", __func__);
		return -EINVAL;
	}

	if (strlen(str) < COLOR_ID_IDS_LENGTH) {
		iRet = sscanf(str, "%20s", str_ids);
		if (iRet < 0)
			return iRet;
	} else {
		pr_err("[SSP] %s - ids overflow\n", __func__);
		return -EINVAL;
	}

	iRet = light_colorid_write_efs_version(version, str_ids);
	if (iRet < 0)
		pr_err("[SSP] %s - write version file err %d\n", __func__, iRet);
	else
		pr_info("[SSP] %s - Success\n", __func__);

	return size;
}

ssize_t light_hiddendhole_hh_write_all_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int ret = 0, err = 0;

	ret = light_colorid_write_all_predefined_data(data);

	if (ret >= 0) {
		int coef[7], i;
		unsigned int thresh[2];
		unsigned int thresh_alert;

		for (i = 1; i <= COLOR_NUM; i++) {
			ret = light_colorid_read_predefined_file(light_coef_predefine_table[i].color_id,
					coef, thresh, &thresh_alert);

			if (ret < 0) {
				err = 1;
				pr_err("[SSP] %s - read coef fail, err = %d\n", __func__, err);
				break;
			}
		}
	} else {
		err = 1;
		pr_err("[SSP] %s - write fail\n", __func__);
	}

	return snprintf(buf, PAGE_SIZE, "%s\n", (err == 0) ? "TRUE" : "FALSE");
}

ssize_t light_hiddenhole_write_all_data_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int color_id = 0;
	int coef[7] = {0, };	/* r_coef,g_coef,b_coef,c_coef,dgf,cct_coef,cct_offset */
	unsigned int thresh[2] = {0, };	/* high,low*/
	unsigned int thresh_alert = 0;
	int crc = 0;
	int iRet = 0;
	int temp_ret = 0;

	mm_segment_t old_fs;
	struct file *coef_filp = NULL;
	char file_name[COLOR_ID_PREDEFINED_FILENAME_LENGTH] = {0, };
	char file_contents[COLOR_ID_PREDEFINED_FILE_LENGTH] = {0, };

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pr_info("[SSP] %s - %s\n", __func__, buf);

	temp_ret = sscanf(buf, "%2d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%7d",
		&color_id, &coef[4], &coef[0], &coef[1],
		&coef[2], &coef[3], &coef[5], &coef[6],
		&thresh[0], &thresh[1], &thresh_alert, &crc);

	if ((coef[0]+coef[1]+coef[2]+coef[3]+coef[4]+coef[5]+coef[6]+thresh[0]+thresh[1]+thresh_alert != crc) ||
		(temp_ret != WRITE_ALL_DATA_NUMBER)) {
		set_fs(old_fs);
		pr_info("[SSP] %s - crc is wrong\n", __func__);
		return -EINVAL;
	}

	pr_info("[SSP] %s - %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", __func__,
		color_id, coef[4], coef[0], coef[1],
		coef[2], coef[3], coef[5], coef[6],
		thresh[0], thresh[1], thresh_alert, crc);

	snprintf(file_name, sizeof(file_name), "%s%d", PREDEFINED_FILE_NAME, color_id);
	snprintf(file_contents, sizeof(file_contents), "%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%d",
		coef[4], coef[0], coef[1], coef[2], coef[3], coef[5], coef[6], thresh[0], thresh[1], thresh_alert, crc);

	coef_filp = filp_open(file_name,
			O_CREAT | O_TRUNC | O_WRONLY | O_SYNC, 0660);

	if (IS_ERR(coef_filp)) {
		pr_err("[SSP] %s - Can't open coef file %d\n", __func__, color_id);
		set_fs(old_fs);
		iRet = PTR_ERR(coef_filp);
		return iRet;
	}

	iRet = vfs_write(coef_filp, (char *)file_contents,
		sizeof(char)*COLOR_ID_PREDEFINED_FILE_LENGTH, &coef_filp->f_pos);
	if (iRet < 0) {
		pr_err("[SSP] %s - Can't write the coef data to file\n", __func__);
		iRet = -EIO;
	}

	filp_close(coef_filp, current->files);
	set_fs(old_fs);

	pr_info("[SSP] %s - Success %d\n", __func__, iRet);
	return size;
}

ssize_t  light_hiddendhole_is_exist_efs_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int is_exist = 1, colorid;
	mm_segment_t old_fs;
	struct file *coef_filp = NULL;
	char file_name[COLOR_ID_PREDEFINED_FILENAME_LENGTH];

	pr_info("[SSP] %s ", __func__);

	colorid = light_corloid_read_colorid();
	if (colorid < 0) {
		pr_err("[SSP] %s - read colorid failed - %d\n", __func__, colorid);
		is_exist = 0;
	} else {
		old_fs = get_fs();
		set_fs(KERNEL_DS);

		snprintf(file_name, sizeof(file_name), "%s%d", PREDEFINED_FILE_NAME, colorid);

		coef_filp = filp_open(file_name, O_RDONLY, 0660);

		if (IS_ERR(coef_filp)) {
			pr_err("[SSP] %s - Can't open coef file %d\n", __func__, colorid);
			set_fs(old_fs);
			is_exist = 0;
		} else {
			filp_close(coef_filp, current->files);
			set_fs(old_fs);
		}
	}
	return snprintf(buf, PAGE_SIZE, "%s\n", (is_exist == 1) ? "TRUE" : "FALSE");
}

ssize_t light_hiddendhole_check_coef_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	pr_info("[SSP] %s ", __func__);
	return snprintf(buf, PAGE_SIZE, "%d", data->light_efs_file_status);
}
ssize_t light_hiddendhole_check_coef_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	pr_info("[SSP] %s ", __func__);

	if (initialize_light_colorid(data) < 0)
		pr_err("[SSP]: %s - initialize light colorid failed\n", __func__);

	return size;
}

ssize_t light_hiddendhole_hh_ext_prox_th_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	ssp_dbg("[SSP]: %s - uProxAlertHiThresh = %u\n", __func__, data->uProxAlertHiThresh);
	return snprintf(buf, PAGE_SIZE, "%d", data->uProxAlertHiThresh);
}

ssize_t light_hiddendhole_check_hh_ext_prox_th_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	u16 uNewThresh = 0;
	int iRet = 0;

	pr_info("[SSP] %s ", __func__);

	iRet = kstrtou16(buf, 10, &uNewThresh);
	if (iRet < 0)
		pr_err("[SSP]: %s - kstrto16 failed.(%d)\n", __func__, iRet);

	data->uProxAlertHiThresh = uNewThresh;
	ssp_dbg("[SSP]: %s - uProxAlertHiThresh = %u\n", __func__, data->uProxAlertHiThresh);

	set_proximity_alert_threshold(data);

	return size;
}

static DEVICE_ATTR(hh_ver, 0664,
	light_hiddendhole_version_show, light_hiddenhole_version_store);
static DEVICE_ATTR(hh_write_all_data, 0664,
	light_hiddendhole_hh_write_all_data_show, light_hiddenhole_write_all_data_store);
static DEVICE_ATTR(hh_is_exist_efs, 0444, light_hiddendhole_is_exist_efs_show, NULL);
static DEVICE_ATTR(hh_check_coef, 0664,
	light_hiddendhole_check_coef_show, light_hiddendhole_check_coef_store);
static DEVICE_ATTR(hh_ext_prox_th, 0664,
	light_hiddendhole_hh_ext_prox_th_show, light_hiddendhole_check_hh_ext_prox_th_store);

static struct device_attribute *hiddenhole_attrs[] = {
	&dev_attr_hh_ver,
	&dev_attr_hh_write_all_data,
	&dev_attr_hh_is_exist_efs,
	&dev_attr_hh_check_coef,
	&dev_attr_hh_ext_prox_th,
	NULL,
};

void initialize_hiddenhole_factorytest(struct ssp_data *data)
{
	sensors_register(data->hiddenhole_device, data, hiddenhole_attrs, "hidden_hole");
}

void remove_hiddenhole_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->hiddenhole_device, hiddenhole_attrs);
}
