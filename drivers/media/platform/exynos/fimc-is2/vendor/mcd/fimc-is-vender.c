/*
* Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is vender functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <exynos-fimc-is-module.h>
#include "fimc-is-vender.h"
#include "fimc-is-vender-specific.h"
#include "fimc-is-core.h"
#include "fimc-is-sec-define.h"
#include "fimc-is-dt.h"
#include "fimc-is-sysfs.h"

#include <linux/device.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <linux/firmware.h>

#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
#include <linux/bsearch.h>
#include <linux/dev_ril_bridge.h>
#endif

#include "fimc-is-binary.h"

#if defined (CONFIG_OIS_USE)
#include "fimc-is-device-ois.h"
#endif
#include "fimc-is-device-preprocessor.h"
#include "fimc-is-interface-sensor.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-interface-library.h"
#include "fimc-is-device-eeprom.h"

extern int fimc_is_create_sysfs(struct fimc_is_core *core);
extern bool is_dumped_fw_loading_needed;
extern bool force_caldata_dump;

static u32  rear_sensor_id;
static u32  rear2_sensor_id;
static u32  rear3_sensor_id;
static u32  front_sensor_id;
static u32  front2_sensor_id;
static u32  rear_tof_sensor_id;
static u32  front_tof_sensor_id;
static bool check_sensor_vendor;
static bool use_ois_hsi2c;
static bool use_ois;
static bool use_module_check;
static bool is_hw_init_running = false;
#if defined(CONFIG_CAMERA_FROM)
static FRomPowersource f_rom_power;
#endif
#ifdef SECURE_CAMERA_IRIS
static u32  secure_sensor_id;
#endif
static u32  ois_sensor_index;
static u32  mcu_sensor_index;
static u32  aperture_sensor_index;

#ifdef CAMERA_PARALLEL_RETENTION_SEQUENCE
struct workqueue_struct *sensor_pwr_ctrl_wq = 0;
#define CAMERA_WORKQUEUE_MAX_WAITING	1000
#endif

#ifdef USE_CAMERA_HW_BIG_DATA
static struct cam_hw_param_collector cam_hwparam_collector;
static bool mipi_err_check;
static bool need_update_to_file;
static int factory_aperture_value;

void fimc_is_sec_init_err_cnt(struct cam_hw_param *hw_param)
{
	if (hw_param) {
		memset(hw_param, 0, sizeof(struct cam_hw_param));
#ifdef CAMERA_HW_BIG_DATA_FILE_IO
		fimc_is_sec_copy_err_cnt_to_file();
#endif
	}
}

#ifdef CAMERA_HW_BIG_DATA_FILE_IO
bool fimc_is_sec_need_update_to_file(void)
{
	return need_update_to_file;
}
void fimc_is_sec_copy_err_cnt_to_file(void)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	long nwrite = 0;
	bool ret = false;
	int old_mask = 0;

	info("%s\n", __func__);

	if (current && current->fs) {
		old_fs = get_fs();
		set_fs(KERNEL_DS);

		ret = sys_access(CAM_HW_ERR_CNT_FILE_PATH, 0);

		if (ret != 0) {
			old_mask = sys_umask(7);
			fp = filp_open(CAM_HW_ERR_CNT_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0660);
			if (IS_ERR_OR_NULL(fp)) {
				warn("%s open failed", CAM_HW_ERR_CNT_FILE_PATH);
				sys_umask(old_mask);
				set_fs(old_fs);
				return;
			}

			filp_close(fp, current->files);
			sys_umask(old_mask);
		}

		fp = filp_open(CAM_HW_ERR_CNT_FILE_PATH, O_WRONLY | O_TRUNC | O_SYNC, 0660);
		if (IS_ERR_OR_NULL(fp)) {
			warn("%s open failed", CAM_HW_ERR_CNT_FILE_PATH);
			set_fs(old_fs);
			return;
		}

		nwrite = vfs_write(fp, (char *)&cam_hwparam_collector, sizeof(struct cam_hw_param_collector), &fp->f_pos);

		filp_close(fp, current->files);
		set_fs(old_fs);
		need_update_to_file = false;
	}
}

void fimc_is_sec_copy_err_cnt_from_file(void)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	long nread = 0;
	bool ret = false;

	info("%s\n", __func__);

	ret = fimc_is_sec_file_exist(CAM_HW_ERR_CNT_FILE_PATH);

	if (ret) {
		old_fs = get_fs();
		set_fs(KERNEL_DS);

		fp = filp_open(CAM_HW_ERR_CNT_FILE_PATH, O_RDONLY, 0660);
		if (IS_ERR_OR_NULL(fp)) {
			warn("%s open failed", CAM_HW_ERR_CNT_FILE_PATH);
			set_fs(old_fs);
			return;
		}

		nread = vfs_read(fp, (char *)&cam_hwparam_collector, sizeof(struct cam_hw_param_collector), &fp->f_pos);

		filp_close(fp, current->files);
		set_fs(old_fs);
	}
}
#endif /* CAMERA_HW_BIG_DATA_FILE_IO */

void fimc_is_sec_get_hw_param(struct cam_hw_param **hw_param, u32 position)
{
	switch (position) {
	case SENSOR_POSITION_REAR:
		*hw_param = &cam_hwparam_collector.rear_hwparam;
		break;
	case SENSOR_POSITION_REAR2:
		*hw_param = &cam_hwparam_collector.rear2_hwparam;
		break;
	case SENSOR_POSITION_REAR3:
		*hw_param = &cam_hwparam_collector.rear3_hwparam;
		break;
	case SENSOR_POSITION_FRONT:
		*hw_param = &cam_hwparam_collector.front_hwparam;
		break;
	case SENSOR_POSITION_FRONT2:
		*hw_param = &cam_hwparam_collector.front2_hwparam;
		break;
	case SENSOR_POSITION_SECURE:
		*hw_param = &cam_hwparam_collector.iris_hwparam;
		break;
	case SENSOR_POSITION_REAR_TOF:
		*hw_param = &cam_hwparam_collector.rear_tof_hwparam;
		break;
	case SENSOR_POSITION_FRONT_TOF:
		*hw_param = &cam_hwparam_collector.front_tof_hwparam;
		break;
	default:
		need_update_to_file = false;
		return;
	}
	need_update_to_file = true;
}
#endif

bool fimc_is_sec_is_valid_moduleid(char *moduleid)
{
	int i = 0;

	if (moduleid == NULL || strlen(moduleid) < 5) {
		goto err;
	}

	for (i = 0; i < 5; i++)
	{
		if (!((moduleid[i] > 47 && moduleid[i] < 58) || // 0 to 9
			(moduleid[i] > 64 && moduleid[i] < 91))) {  // A to Z
			goto err;
		}
	}

	return true;

err:
	warn("invalid moduleid\n");
	return false;
}

#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
static struct cam_cp_noti_info g_cp_noti_info;
static struct mutex g_mipi_mutex;
static bool g_init_notifier;

/* CP notity format (HEX raw format)
 * 10 00 AA BB 27 01 03 XX YY YY YY YY ZZ ZZ ZZ ZZ
 *
 * 00 10 (0x0010) - len
 * AA BB - not used
 * 27 - MAIN CMD (SYSTEM CMD : 0x27)
 * 01 - SUB CMD (CP Channel Info : 0x01)
 * 03 - NOTI CMD (0x03)
 * XX - RAT MODE
 * YY YY YY YY - BAND MODE
 * ZZ ZZ ZZ ZZ - FREQ INFO
 */

static int fimc_is_vendor_ril_notifier(struct notifier_block *nb,
				unsigned long size, void *buf)
{
	struct dev_ril_bridge_msg *msg;
	struct cam_cp_noti_info *cp_noti_info;

	if (!g_init_notifier) {
		warn("%s: not init ril notifier\n", __func__);
		return NOTIFY_DONE;
	}

	info("%s: ril notification size [%ld]\n", __func__, size);

	msg = (struct dev_ril_bridge_msg *)buf;

	if (size == sizeof(struct dev_ril_bridge_msg)
			&& msg->dev_id == IPC_SYSTEM_CP_CHANNEL_INFO
			&& msg->data_len == sizeof(struct cam_cp_noti_info)) {
		cp_noti_info = (struct cam_cp_noti_info *)msg->data;

		mutex_lock(&g_mipi_mutex);
		memcpy(&g_cp_noti_info, msg->data, sizeof(struct cam_cp_noti_info));
		mutex_unlock(&g_mipi_mutex);

		info("%s: update mipi channel [%d,%d,%d]\n",
			__func__, g_cp_noti_info.rat, g_cp_noti_info.band, g_cp_noti_info.channel);

		return NOTIFY_OK;
	}

	return NOTIFY_DONE;
}

static struct notifier_block g_ril_notifier_block = {
	.notifier_call = fimc_is_vendor_ril_notifier,
};

static void fimc_is_vendor_register_ril_notifier(void)
{
	if (!g_init_notifier) {
		info("%s: register ril notifier\n", __func__);

		mutex_init(&g_mipi_mutex);
		memset(&g_cp_noti_info, 0, sizeof(struct cam_cp_noti_info));

		register_dev_ril_bridge_event_notifier(&g_ril_notifier_block);
		g_init_notifier = true;
	}
}

static void fimc_is_vendor_get_rf_channel(struct cam_cp_noti_info *ch)
{
	if (!g_init_notifier) {
		warn("%s: not init ril notifier\n", __func__);
		memset(ch, 0, sizeof(struct cam_cp_noti_info));
		return;
	}

	mutex_lock(&g_mipi_mutex);
	memcpy(ch, &g_cp_noti_info, sizeof(struct cam_cp_noti_info));
	mutex_unlock(&g_mipi_mutex);
}

static int compare_rf_channel(const void *key, const void *element)
{
	struct cam_mipi_channel *k = ((struct cam_mipi_channel *)key);
	struct cam_mipi_channel *e = ((struct cam_mipi_channel *)element);

	if (k->rat_band < e->rat_band)
		return -1;
	else if (k->rat_band > e->rat_band)
		return 1;

	if (k->channel_max < e->channel_min)
		return -1;
	else if (k->channel_min > e->channel_max)
		return 1;

	return 0;
}

int fimc_is_vendor_select_mipi_by_rf_channel(const struct cam_mipi_channel *channel_list, const int size)
{
	struct cam_mipi_channel *result = NULL;
	struct cam_mipi_channel key;
	struct cam_cp_noti_info input_ch;

	fimc_is_vendor_get_rf_channel(&input_ch);

	key.rat_band = CAM_RAT_BAND(input_ch.rat, input_ch.band);
	key.channel_min = input_ch.channel;
	key.channel_max = input_ch.channel;

	info("%s: searching rf channel s [%d,%d,%d]\n",
		__func__, input_ch.rat, input_ch.band, input_ch.channel);

	result = bsearch(&key,
			channel_list,
			size,
			sizeof(struct cam_mipi_channel),
			compare_rf_channel);

	if (result == NULL) {
		info("%s: searching result : not found, use default mipi clock\n", __func__);
		return 0; /* return default mipi clock index = 0 */
	}

	info("%s: searching result : [0x%x,(%d-%d)]->(%d)\n", __func__,
		result->rat_band, result->channel_min, result->channel_max, result->setting_index);

	return result->setting_index;
}

int fimc_is_vendor_verify_mipi_channel(const struct cam_mipi_channel *channel_list, const int size)
{
	int i;
	u16 pre_rat;
	u16 pre_band;
	u32 pre_channel_min;
	u32 pre_channel_max;
	u16 cur_rat;
	u16 cur_band;
	u32 cur_channel_min;
	u32 cur_channel_max;

	if (channel_list == NULL || size < 2)
		return 0;

	pre_rat = CAM_GET_RAT(channel_list[0].rat_band);
	pre_band = CAM_GET_BAND(channel_list[0].rat_band);
	pre_channel_min = channel_list[0].channel_min;
	pre_channel_max = channel_list[0].channel_max;

	if (pre_channel_min > pre_channel_max) {
		panic("min is bigger than max : index=%d, min=%d, max=%d", 0, pre_channel_min, pre_channel_max);
		return -EINVAL;
	}

	for (i = 1; i < size; i++) {
		cur_rat = CAM_GET_RAT(channel_list[i].rat_band);
		cur_band = CAM_GET_BAND(channel_list[i].rat_band);
		cur_channel_min = channel_list[i].channel_min;
		cur_channel_max = channel_list[i].channel_max;

		if (cur_channel_min > cur_channel_max) {
			panic("min is bigger than max : index=%d, min=%d, max=%d", 0, cur_channel_min, cur_channel_max);
			return -EINVAL;
		}

		if (pre_rat > cur_rat) {
			panic("not sorted rat : index=%d, pre_rat=%d, cur_rat=%d", i, pre_rat, cur_rat);
			return -EINVAL;
		}

		if (pre_rat == cur_rat) {
			if (pre_band > cur_band) {
				panic("not sorted band : index=%d, pre_band=%d, cur_band=%d", i, pre_band, cur_band);
				return -EINVAL;
			}

			if (pre_band == cur_band) {
				if (pre_channel_max >= cur_channel_min) {
					panic("overlaped channel range : index=%d, pre_ch=[%d-%d], cur_ch=[%d-%d]",
						i, pre_channel_min, pre_channel_max, cur_channel_min, cur_channel_max);
					return -EINVAL;
				}
			}
		}

		pre_rat = cur_rat;
		pre_band = cur_band;
		pre_channel_min = cur_channel_min;
		pre_channel_max = cur_channel_max;
	}

	return 0;
}

#endif

void fimc_is_vendor_csi_stream_on(struct fimc_is_device_csi *csi)
{
#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
	struct fimc_is_device_sensor *device = NULL;
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct fimc_is_cis *cis = NULL;
	int ret;

	device = container_of(csi->subdev, struct fimc_is_device_sensor, subdev_csi);
	if (device == NULL) {
		warn("device is null");
		return;
	};

	ret = fimc_is_sensor_g_module(device, &module);
	if (ret) {
		warn("%s sensor_g_module failed(%d)", __func__, ret);
		return;
	}

	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;
	if (sensor_peri == NULL) {
		warn("sensor_peri is null");
		return;
	};

	if (sensor_peri->subdev_cis) {
		cis = (struct fimc_is_cis *)v4l2_get_subdevdata(sensor_peri->subdev_cis);
		CALL_CISOPS(cis, cis_update_mipi_info, sensor_peri->subdev_cis);
	}
#endif

#ifdef USE_CAMERA_HW_BIG_DATA
	mipi_err_check = false;
#endif
}

void fimc_is_vender_csi_err_handler(struct fimc_is_device_csi *csi)
{
#ifdef USE_CAMERA_HW_BIG_DATA
	struct fimc_is_device_sensor *device = NULL;
	struct cam_hw_param *hw_param = NULL;

	device = container_of(csi->subdev, struct fimc_is_device_sensor, subdev_csi);

	if (device && device->pdev && !mipi_err_check) {
		fimc_is_sec_get_hw_param(&hw_param, device->position);
		switch (device->pdev->id) {
#ifdef CSI_SCENARIO_COMP
			case CSI_SCENARIO_COMP:
				if (hw_param)
					hw_param->mipi_comp_err_cnt++;
				break;
#endif
			default:
				if (hw_param)
					hw_param->mipi_sensor_err_cnt++;
				break;
		}
		mipi_err_check = true;
	}
#endif
}

void fimc_is_vender_csi_err_print_debug_log(struct fimc_is_device_sensor *device)
{
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_cis *cis = NULL;
	int ret = 0;

	if (device && device->pdev) {
		ret = fimc_is_sensor_g_module(device, &module);
		if (ret) {
			warn("%s sensor_g_module failed(%d)", __func__, ret);
			return;
		}

		sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;

		if (sensor_peri->subdev_preprocessor) { /* for preprocessor */
			preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(sensor_peri->subdev_preprocessor);
			CALL_PREPROPOPS(preprocessor, preprocessor_debug, sensor_peri->subdev_preprocessor);
		}

		if (sensor_peri->subdev_cis) {
			cis = (struct fimc_is_cis *)v4l2_get_subdevdata(sensor_peri->subdev_cis);
			CALL_CISOPS(cis, cis_log_status, sensor_peri->subdev_cis);
		}
	}
}

int fimc_is_vender_probe(struct fimc_is_vender *vender)
{
	int i;
	int ret = 0;
	struct fimc_is_core *core;
	struct fimc_is_vender_specific *specific;

	BUG_ON(!vender);

	core = container_of(vender, struct fimc_is_core, vender);
	snprintf(vender->fw_path, sizeof(vender->fw_path), "%s", FIMC_IS_FW_SDCARD);
	snprintf(vender->request_fw_path, sizeof(vender->request_fw_path), "%s", FIMC_IS_FW);

	specific = devm_kzalloc(&core->pdev->dev,
			sizeof(struct fimc_is_vender_specific), GFP_KERNEL);
	if (!specific) {
		probe_err("failed to allocate vender specific");
		return -ENOMEM;
	}

	/* init mutex for rom read */
	mutex_init(&specific->rom_lock);
	mutex_init(&specific->hw_init_lock);

	if (fimc_is_create_sysfs(core)) {
		probe_err("fimc_is_create_sysfs is failed");
		ret = -EINVAL;
		goto p_err;
	}

	specific->rear_sensor_id = rear_sensor_id;
	specific->front_sensor_id = front_sensor_id;
	specific->rear2_sensor_id = rear2_sensor_id;
	specific->front2_sensor_id = front2_sensor_id;
	specific->rear3_sensor_id = rear3_sensor_id;
	specific->rear_tof_sensor_id = rear_tof_sensor_id;
	specific->front_tof_sensor_id = front_tof_sensor_id;
	specific->check_sensor_vendor = check_sensor_vendor;
	specific->use_ois = use_ois;
	specific->use_ois_hsi2c = use_ois_hsi2c;
	specific->use_module_check = use_module_check;
#if defined(CONFIG_CAMERA_FROM)
	specific->f_rom_power = f_rom_power;
#endif
	specific->suspend_resume_disable = false;
	specific->need_cold_reset = false;
#ifdef SECURE_CAMERA_IRIS
	specific->secure_sensor_id = secure_sensor_id;
#endif
	specific->ois_sensor_index = ois_sensor_index;
	specific->mcu_sensor_index = mcu_sensor_index;
	specific->aperture_sensor_index = aperture_sensor_index;
	specific->zoom_running = false;

	for (i = 0; i < ROM_ID_MAX; i++) {
		specific->eeprom_client[i] = NULL;
		specific->rom_valid[i] = false;
 	}

	vender->private_data = specific;

	fimc_is_load_ctrl_init();
#ifdef CAMERA_PARALLEL_RETENTION_SEQUENCE
	if (!sensor_pwr_ctrl_wq) {
		sensor_pwr_ctrl_wq = create_singlethread_workqueue("sensor_pwr_ctrl");
	}
#endif

p_err:
	return ret;
}

static int parse_sysfs_caminfo(struct device_node *np,
				struct fimc_is_cam_info *cam_infos, int camera_num)
{
	u32 temp;
	char *pprop;

	DT_READ_U32(np, "internal_id", cam_infos[camera_num].internal_id);
	DT_READ_U32(np, "isp", cam_infos[camera_num].isp);
	DT_READ_U32(np, "cal_memory", cam_infos[camera_num].cal_memory);
	DT_READ_U32(np, "read_version", cam_infos[camera_num].read_version);
	DT_READ_U32(np, "core_voltage", cam_infos[camera_num].core_voltage);
	DT_READ_U32(np, "upgrade", cam_infos[camera_num].upgrade);
	DT_READ_U32(np, "fw_write", cam_infos[camera_num].fw_write);
	DT_READ_U32(np, "fw_dump", cam_infos[camera_num].fw_dump);
	DT_READ_U32(np, "companion", cam_infos[camera_num].companion);
	DT_READ_U32(np, "ois", cam_infos[camera_num].ois);
	DT_READ_U32(np, "valid", cam_infos[camera_num].valid);
	DT_READ_U32(np, "dual_open", cam_infos[camera_num].dual_open);

	return 0;
}

int fimc_is_vender_dt(struct device_node *np)
{
	int ret = 0;
	struct device_node *camInfo_np;
	struct fimc_is_cam_info *camera_infos;
	struct fimc_is_common_cam_info *common_camera_infos = NULL;
	char camInfo_string[15];
	int camera_num;
	int max_camera_num;

	ret = of_property_read_u32(np, "rear_sensor_id", &rear_sensor_id);
	if (ret)
		probe_err("rear_sensor_id read is fail(%d)", ret);

	ret = of_property_read_u32(np, "front_sensor_id", &front_sensor_id);
	if (ret)
		probe_err("front_sensor_id read is fail(%d)", ret);

	ret = of_property_read_u32(np, "rear2_sensor_id", &rear2_sensor_id);
	if (ret)
		probe_err("rear2_sensor_id read is fail(%d)", ret);

	ret = of_property_read_u32(np, "front2_sensor_id", &front2_sensor_id);
	if (ret)
		probe_err("front2_sensor_id read is fail(%d)", ret);

	ret = of_property_read_u32(np, "rear3_sensor_id", &rear3_sensor_id);
	if (ret)
		probe_err("rear3_sensor_id read is fail(%d)", ret);

#ifdef SECURE_CAMERA_IRIS
	ret = of_property_read_u32(np, "secure_sensor_id", &secure_sensor_id);
	if (ret) {
		probe_err("secure_sensor_id read is fail(%d)", ret);
		secure_sensor_id = 0;
	}
#endif
	ret = of_property_read_u32(np, "rear_tof_sensor_id", &rear_tof_sensor_id);
	if (ret)
		probe_err("rear_tof_sensor_id read is fail(%d)", ret);

	ret = of_property_read_u32(np, "front_tof_sensor_id", &front_tof_sensor_id);
	if (ret)
		probe_err("front_tof_sensor_id read is fail(%d)", ret);

	ret = of_property_read_u32(np, "ois_sensor_index", &ois_sensor_index);
	if (ret)
		probe_err("ois_sensor_index read is fail(%d)", ret);

	ret = of_property_read_u32(np, "mcu_sensor_index", &mcu_sensor_index);
	if (ret)
		probe_err("mcu_sensor_index read is fail(%d)", ret);

	ret = of_property_read_u32(np, "aperture_sensor_index", &aperture_sensor_index);
	if (ret)
		probe_err("aperture_sensor_index read is fail(%d)", ret);

	check_sensor_vendor = of_property_read_bool(np, "check_sensor_vendor");
	if (!check_sensor_vendor) {
		probe_info("check_sensor_vendor not use(%d)\n", check_sensor_vendor);
	}
#ifdef CONFIG_OIS_USE
	use_ois = of_property_read_bool(np, "use_ois");
	if (!use_ois) {
		probe_err("use_ois not use(%d)", use_ois);
	}

	use_ois_hsi2c = of_property_read_bool(np, "use_ois_hsi2c");
	if (!use_ois_hsi2c) {
		probe_err("use_ois_hsi2c not use(%d)", use_ois_hsi2c);
	}
#endif

	use_module_check = of_property_read_bool(np, "use_module_check");
	if (!use_module_check) {
		probe_err("use_module_check not use(%d)", use_module_check);
	}

#if defined(CONFIG_CAMERA_FROM)
	f_rom_power = of_property_read_u32(np, "f_rom_power", &f_rom_power);
	if (!f_rom_power) {
		probe_info("f_rom_power not use(%d)\n", f_rom_power);
	}
#endif

	ret = of_property_read_u32(np, "max_camera_num", &max_camera_num);
	if (ret) {
		err("max_camera_num read is fail(%d)", ret);
		max_camera_num = 0;
	}

	fimc_is_get_cam_info(&camera_infos);

	for (camera_num = 0; camera_num < max_camera_num; camera_num++) {
		sprintf(camInfo_string, "%s%d", "camera_info", camera_num);

		camInfo_np = of_find_node_by_name(np, camInfo_string);
		if (!camInfo_np) {
			info("%s: camera_num = %d can't find camInfo_string node\n", __func__, camera_num);
			continue;
		}
		parse_sysfs_caminfo(camInfo_np, camera_infos, camera_num);
	}

	fimc_is_get_common_cam_info(&common_camera_infos);

	ret = of_property_read_u32(np, "max_supported_camera", &common_camera_infos->max_supported_camera);
	if (ret) {
		probe_err("supported_cameraId read is fail(%d)", ret);
	}

	ret = of_property_read_u32_array(np, "supported_cameraId",
		common_camera_infos->supported_camera_ids, common_camera_infos->max_supported_camera);
	if (ret) {
		probe_err("supported_cameraId read is fail(%d)", ret);
	}

	return ret;
}

int fimc_is_vendor_rom_parse_dt(struct device_node *dnode, int rom_id)
{
	const u32 *header_crc_check_list_spec;
	const u32 *crc_check_list_spec;
	const u32 *dual_crc_check_list_spec;
	const u32 *rom_dualcal_slave0_tilt_list_spec;
	const u32 *rom_dualcal_slave1_tilt_list_spec;
	const u32 *rom_dualcal_slave2_tilt_list_spec; /* wide(rear) - tof */
	const u32 *rom_dualcal_slave3_tilt_list_spec; /* ultra wide(rear2) - tof */
	const u32 *rom_ois_list_spec;
	const u32 *tof_cal_size_list_spec;
	const u32 *tof_cal_uid_list_spec;
	const u32 *tof_cal_valid_list_spec;
	const char *node_string;
	int ret = 0;
#ifdef FIMC_IS_DEVICE_ROM_DEBUG
	int i;
#endif
	u32 temp;
	char *pprop;
	bool skip_cal_loading;
	bool skip_crc_check;
	bool skip_header_loading;

	struct fimc_is_rom_info *finfo;

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);

	memset(finfo, 0, sizeof(struct fimc_is_rom_info));

	ret = of_property_read_u32(dnode, "rom_power_position", &finfo->rom_power_position);
	BUG_ON(ret);

	ret = of_property_read_u32(dnode, "rom_size", &finfo->rom_size);
	BUG_ON(ret);

	ret = of_property_read_u32(dnode, "cal_map_es_version", &finfo->cal_map_es_version);
	if (ret)
		finfo->cal_map_es_version = 0;

	ret = of_property_read_string(dnode, "camera_module_es_version", &node_string);
	if (ret)
		finfo->camera_module_es_version = 'A';
	else
		finfo->camera_module_es_version = node_string[0];

	skip_cal_loading = of_property_read_bool(dnode, "skip_cal_loading");
	if (skip_cal_loading) {
		set_bit(FIMC_IS_ROM_STATE_SKIP_CAL_LOADING, &finfo->rom_state);
	}

	skip_crc_check = of_property_read_bool(dnode, "skip_crc_check");
	if (skip_crc_check) {
		set_bit(FIMC_IS_ROM_STATE_SKIP_CRC_CHECK, &finfo->rom_state);
	}

	skip_header_loading = of_property_read_bool(dnode, "skip_header_loading");
	if (skip_header_loading) {
		set_bit(FIMC_IS_ROM_STATE_SKIP_HEADER_LOADING, &finfo->rom_state);
	}

	info("[rom%d] power_position:%d, rom_size:0x%X, skip_cal_loading:%d, calmap_es:%d, module_es:%c\n",
		rom_id, finfo->rom_power_position, finfo->rom_size, skip_cal_loading,
		finfo->cal_map_es_version, finfo->camera_module_es_version);

	header_crc_check_list_spec = of_get_property(dnode, "header_crc_check_list", &finfo->header_crc_check_list_len);
	if (header_crc_check_list_spec) {
		finfo->header_crc_check_list_len /= (unsigned int)sizeof(*header_crc_check_list_spec);

		BUG_ON(finfo->header_crc_check_list_len > FIMC_IS_ROM_CRC_MAX_LIST);

		ret = of_property_read_u32_array(dnode, "header_crc_check_list", finfo->header_crc_check_list, finfo->header_crc_check_list_len);
		if (ret)
			err("header_crc_check_list read is fail(%d)", ret);
#ifdef FIMC_IS_DEVICE_ROM_DEBUG
		else {
			info("header_crc_check_list :");
			for (i = 0; i < finfo->header_crc_check_list_len; i++)
				info(" %d ", finfo->header_crc_check_list[i]);
			info("\n");
		}
#endif
	}

	crc_check_list_spec = of_get_property(dnode, "crc_check_list", &finfo->crc_check_list_len);
	if (crc_check_list_spec) {
		finfo->crc_check_list_len /= (unsigned int)sizeof(*crc_check_list_spec);

		BUG_ON(finfo->crc_check_list_len > FIMC_IS_ROM_CRC_MAX_LIST);

		ret = of_property_read_u32_array(dnode, "crc_check_list", finfo->crc_check_list, finfo->crc_check_list_len);
		if (ret)
			err("crc_check_list read is fail(%d)", ret);
#ifdef FIMC_IS_DEVICE_ROM_DEBUG
		else {
			info("crc_check_list :");
			for (i = 0; i < finfo->crc_check_list_len; i++)
				info(" %d ", finfo->crc_check_list[i]);
			info("\n");
		}
#endif
	}

	dual_crc_check_list_spec = of_get_property(dnode, "dual_crc_check_list", &finfo->dual_crc_check_list_len);
	if (dual_crc_check_list_spec) {
		finfo->dual_crc_check_list_len /= (unsigned int)sizeof(*dual_crc_check_list_spec);

		ret = of_property_read_u32_array(dnode, "dual_crc_check_list", finfo->dual_crc_check_list, finfo->dual_crc_check_list_len);
		if (ret)
			info("dual_crc_check_list read is fail(%d)", ret);
#ifdef FIMC_IS_DEVICE_ROM_DEBUG
		else {
			info("dual_crc_check_list :");
			for (i = 0; i < finfo->dual_crc_check_list_len; i++)
				info(" %d ", finfo->dual_crc_check_list[i]);
			info("\n");
		}
#endif
	}

	rom_dualcal_slave0_tilt_list_spec
		= of_get_property(dnode, "rom_dualcal_slave0_tilt_list", &finfo->rom_dualcal_slave0_tilt_list_len);
	if (rom_dualcal_slave0_tilt_list_spec) {
		finfo->rom_dualcal_slave0_tilt_list_len /= (unsigned int)sizeof(*rom_dualcal_slave0_tilt_list_spec);

		ret = of_property_read_u32_array(dnode, "rom_dualcal_slave0_tilt_list",
			finfo->rom_dualcal_slave0_tilt_list, finfo->rom_dualcal_slave0_tilt_list_len);
		if (ret)
			info("rom_dualcal_slave0_tilt_list read is fail(%d)", ret);
#ifdef FIMC_IS_DEVICE_ROM_DEBUG
		else {
			info("rom_dualcal_slave0_tilt_list :");
			for (i = 0; i < finfo->rom_dualcal_slave0_tilt_list_len; i++)
				info(" %d ", finfo->rom_dualcal_slave0_tilt_list[i]);
			info("\n");
		}
#endif
	}

	rom_dualcal_slave1_tilt_list_spec
		= of_get_property(dnode, "rom_dualcal_slave1_tilt_list", &finfo->rom_dualcal_slave1_tilt_list_len);
	if (rom_dualcal_slave1_tilt_list_spec) {
		finfo->rom_dualcal_slave1_tilt_list_len /= (unsigned int)sizeof(*rom_dualcal_slave1_tilt_list_spec);

		ret = of_property_read_u32_array(dnode, "rom_dualcal_slave1_tilt_list",
			finfo->rom_dualcal_slave1_tilt_list, finfo->rom_dualcal_slave1_tilt_list_len);
		if (ret)
			info("rom_dualcal_slave1_tilt_list read is fail(%d)", ret);
#ifdef FIMC_IS_DEVICE_ROM_DEBUG
		else {
			info("rom_dualcal_slave1_tilt_list :");
			for (i = 0; i < finfo->rom_dualcal_slave1_tilt_list_len; i++)
				info(" %d ", finfo->rom_dualcal_slave1_tilt_list[i]);
			info("\n");
		}
#endif
	}

	rom_dualcal_slave2_tilt_list_spec
		= of_get_property(dnode, "rom_dualcal_slave2_tilt_list", &finfo->rom_dualcal_slave2_tilt_list_len);
	if (rom_dualcal_slave2_tilt_list_spec) {
		finfo->rom_dualcal_slave2_tilt_list_len /= (unsigned int)sizeof(*rom_dualcal_slave2_tilt_list_spec);

		ret = of_property_read_u32_array(dnode, "rom_dualcal_slave2_tilt_list",
			finfo->rom_dualcal_slave2_tilt_list, finfo->rom_dualcal_slave2_tilt_list_len);
		if (ret)
			info("rom_dualcal_slave2_tilt_list read is fail(%d)", ret);
#ifdef FIMC_IS_DEVICE_ROM_DEBUG
		else {
			info("rom_dualcal_slave2_tilt_list :");
			for (i = 0; i < finfo->rom_dualcal_slave2_tilt_list_len; i++)
				info(" %d ", finfo->rom_dualcal_slave2_tilt_list[i]);
			info("\n");
		}
#endif
	}

	rom_dualcal_slave3_tilt_list_spec
		= of_get_property(dnode, "rom_dualcal_slave3_tilt_list", &finfo->rom_dualcal_slave3_tilt_list_len);
	if (rom_dualcal_slave3_tilt_list_spec) {
		finfo->rom_dualcal_slave3_tilt_list_len /= (unsigned int)sizeof(*rom_dualcal_slave3_tilt_list_spec);

		ret = of_property_read_u32_array(dnode, "rom_dualcal_slave3_tilt_list",
			finfo->rom_dualcal_slave3_tilt_list, finfo->rom_dualcal_slave3_tilt_list_len);
		if (ret)
			info("rom_dualcal_slave3_tilt_list read is fail(%d)", ret);
#ifdef FIMC_IS_DEVICE_ROM_DEBUG
		else {
			info("rom_dualcal_slave3_tilt_list :");
			for (i = 0; i < finfo->rom_dualcal_slave3_tilt_list_len; i++)
				info(" %d ", finfo->rom_dualcal_slave3_tilt_list[i]);
			info("\n");
		}
#endif
	}

	rom_ois_list_spec = of_get_property(dnode, "rom_ois_list", &finfo->rom_ois_list_len);
	if (rom_ois_list_spec) {
		finfo->rom_ois_list_len /= (unsigned int)sizeof(*rom_ois_list_spec);

		ret = of_property_read_u32_array(dnode, "rom_ois_list",
			finfo->rom_ois_list, finfo->rom_ois_list_len);
		if (ret)
			info("rom_ois_list read is fail(%d)", ret);
#ifdef FIMC_IS_DEVICE_ROM_DEBUG
		else {
			info("rom_ois_list :");
			for (i = 0; i < finfo->rom_ois_list_len; i++)
				info(" %d ", finfo->rom_ois_list[i]);
			info("\n");
		}
#endif
	}

	DT_READ_U32_DEFAULT(dnode, "rom_header_cal_data_start_addr", finfo->rom_header_cal_data_start_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_header_version_start_addr", finfo->rom_header_version_start_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_header_sensor2_version_start_addr", finfo->rom_header_sensor2_version_start_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_header_cal_map_ver_start_addr", finfo->rom_header_cal_map_ver_start_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_header_isp_setfile_ver_start_addr", finfo->rom_header_isp_setfile_ver_start_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_header_project_name_start_addr", finfo->rom_header_project_name_start_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_header_module_id_addr", finfo->rom_header_module_id_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_header_sensor_id_addr", finfo->rom_header_sensor_id_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_header_sensor2_id_addr", finfo->rom_header_sensor2_id_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_header_mtf_data_addr", finfo->rom_header_mtf_data_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_header_f2_mtf_data_addr", finfo->rom_header_f2_mtf_data_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_header_f3_mtf_data_addr", finfo->rom_header_f3_mtf_data_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_header_sensor2_mtf_data_addr", finfo->rom_header_sensor2_mtf_data_addr, -1);

	DT_READ_U32_DEFAULT(dnode, "rom_awb_master_addr", finfo->rom_awb_master_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_awb_module_addr", finfo->rom_awb_module_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_sensor2_awb_master_addr", finfo->rom_sensor2_awb_master_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_sensor2_awb_module_addr", finfo->rom_sensor2_awb_module_addr, -1);

	DT_READ_U32_DEFAULT(dnode, "rom_af_cal_macro_addr", finfo->rom_af_cal_macro_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_af_cal_d50_addr", finfo->rom_af_cal_d50_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_af_cal_pan_addr", finfo->rom_af_cal_pan_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_sensor2_af_cal_macro_addr", finfo->rom_sensor2_af_cal_macro_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_sensor2_af_cal_d50_addr", finfo->rom_sensor2_af_cal_d50_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_sensor2_af_cal_pan_addr", finfo->rom_sensor2_af_cal_pan_addr, -1);

	DT_READ_U32_DEFAULT(dnode, "rom_dualcal_slave0_start_addr", finfo->rom_dualcal_slave0_start_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_dualcal_slave0_size", finfo->rom_dualcal_slave0_size, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_dualcal_slave1_start_addr", finfo->rom_dualcal_slave1_start_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_dualcal_slave1_size", finfo->rom_dualcal_slave1_size, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_dualcal_slave2_start_addr", finfo->rom_dualcal_slave2_start_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_dualcal_slave2_size", finfo->rom_dualcal_slave2_size, -1);

	tof_cal_size_list_spec = of_get_property(dnode, "rom_tof_cal_size_addr", &finfo->rom_tof_cal_size_addr_len);
	if (tof_cal_size_list_spec) {
		finfo->rom_tof_cal_size_addr_len /= (unsigned int)sizeof(*tof_cal_size_list_spec);

		BUG_ON(finfo->rom_tof_cal_size_addr_len > TOF_CAL_SIZE_MAX);

		ret = of_property_read_u32_array(dnode, "rom_tof_cal_size_addr", finfo->rom_tof_cal_size_addr, finfo->rom_tof_cal_size_addr_len);
		if (ret)
			err("rom_tof_cal_size_addr read is fail(%d)", ret);
#ifdef FIMC_IS_DEVICE_ROM_DEBUG
		else {
			info("rom_tof_cal_size_addr :");
			for (i = 0; i < finfo->rom_tof_cal_size_addr_len; i++)
				info(" %d ", finfo->rom_tof_cal_size_addr[i]);
			info("\n");
		}
#endif
	}

	tof_cal_uid_list_spec = of_get_property(dnode, "rom_tof_cal_uid_addr", &finfo->rom_tof_cal_uid_addr_len);
	if (tof_cal_uid_list_spec) {
		finfo->rom_tof_cal_uid_addr_len /= (unsigned int)sizeof(*tof_cal_uid_list_spec);

		BUG_ON(finfo->rom_tof_cal_uid_addr_len > TOF_CAL_UID_MAX);

		ret = of_property_read_u32_array(dnode, "rom_tof_cal_uid_addr", finfo->rom_tof_cal_uid_addr, finfo->rom_tof_cal_uid_addr_len);
		if (ret)
			err("rom_tof_cal_uid_addr read is fail(%d)", ret);
#ifdef FIMC_IS_DEVICE_ROM_DEBUG
		else {
			info("rom_tof_cal_uid_addr :");
			for (i = 0; i < finfo->rom_tof_cal_uid_addr_len; i++)
				info(" %d ", finfo->rom_tof_cal_uid_addr[i]);
			info("\n");
		}
#endif
	}
	tof_cal_valid_list_spec = of_get_property(dnode, "rom_tof_cal_validation_addr", &finfo->rom_tof_cal_validation_addr_len);
	if (tof_cal_valid_list_spec) {
		finfo->rom_tof_cal_validation_addr_len /= (unsigned int)sizeof(*tof_cal_valid_list_spec);
		ret = of_property_read_u32_array(dnode, "rom_tof_cal_validation_addr", finfo->rom_tof_cal_validation_addr, finfo->rom_tof_cal_validation_addr_len);
		if (ret)
			err("rom_tof_cal_validation_addr read is fail(%d)", ret);
#ifdef FIMC_IS_DEVICE_ROM_DEBUG
		else {
			info("rom_tof_cal_validation_addr :");
			for (i = 0; i < finfo->rom_tof_cal_uid_addr_len; i++)
				info(" %d ", finfo->rom_tof_cal_uid_addr[i]);
			info("\n");
		}
#endif
	}

	DT_READ_U32_DEFAULT(dnode, "rom_tof_cal_start_addr", finfo->rom_tof_cal_start_addr, -1);
	DT_READ_U32_DEFAULT(dnode, "rom_tof_cal_result_addr", finfo->rom_tof_cal_result_addr, -1);

	return 0;
}

bool fimc_is_vender_check_sensor(struct fimc_is_core *core)
{
	int i = 0;
	bool ret = false;
	int retry_count = 20;

	do {
		ret = false;
		for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
			if (!test_bit(FIMC_IS_SENSOR_PROBE, &core->sensor[i].state)) {
				ret = true;
				break;
			}
		}

		if (i == FIMC_IS_SENSOR_COUNT && ret == false) {
			info("Retry count = %d\n", retry_count);
			break;
		}

		mdelay(100);
		if (retry_count > 0) {
			--retry_count;
		} else {
			err("Could not get sensor before start ois fw update routine.\n");
			break;
		}
	} while (ret);

	return ret;
}

void fimc_is_vender_check_hw_init_running(void)
{
	int retry = 50;

	do {
		if (!is_hw_init_running) {
			break;
		}
		--retry;
		msleep(100);
	} while (retry > 0);

	if (retry <= 0) {
		err("HW init is not completed.");
	}

	return;
}

#if defined(CONFIG_SENSOR_RETENTION_USE) && defined(USE_CAMERA_PREPARE_RETENTION_ON_BOOT)
void fimc_is_vendor_prepare_retention(struct fimc_is_core *core, int sensor_id, int position)
{
	struct fimc_is_device_sensor *device;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct fimc_is_cis *cis;
	int ret = 0;
	u32 scenario = SENSOR_SCENARIO_NORMAL;
	u32 i2c_channel;

	device = &core->sensor[sensor_id];

#ifdef CONFIG_SENSORCORE_MCU_CONTROL
	scenario = SENSOR_SCENARIO_HW_INIT;
#endif

	info("%s: start %d %d\n", __func__, sensor_id, position);

	WARN_ON(!device);

	device->pdata->scenario = scenario;

	ret = fimc_is_search_sensor_module_with_position(device, position, &module);
	if (ret) {
		warn("%s fimc_is_search_sensor_module_with_position failed(%d)", __func__, ret);
		goto p_err;
	}

	if (module->ext.use_retention_mode == SENSOR_RETENTION_UNSUPPORTED) {
		warn("%s module doesn't support retention", __func__);
		goto p_err;
	}

	/* Sensor power on */
	ret = module->pdata->gpio_cfg(module, scenario, GPIO_SCENARIO_ON);
	if (ret) {
		warn("gpio on is fail(%d)", ret);
		goto p_power_off;
	}

	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;
	if (sensor_peri->subdev_cis) {
		i2c_channel = module->ext.sensor_con.peri_setting.i2c.channel;
		if (i2c_channel < SENSOR_CONTROL_I2C_MAX) {
			sensor_peri->cis.i2c_lock = &core->i2c_lock[i2c_channel];
			info("%s[%d]enable cis i2c client. position = %d\n",
					__func__, __LINE__, core->current_position);
		} else {
			warn("%s: wrong cis i2c_channel(%d)", __func__, i2c_channel);
			goto p_power_off;
		}

		cis = (struct fimc_is_cis *)v4l2_get_subdevdata(sensor_peri->subdev_cis);
		ret = CALL_CISOPS(cis, cis_check_rev_on_init, sensor_peri->subdev_cis);
		if (ret) {
			warn("v4l2_subdev_call(cis_check_rev_on_init) is fail(%d)", ret);
			goto p_power_off;
		}

		ret = CALL_CISOPS(cis, cis_init, sensor_peri->subdev_cis);
		if (ret) {
			warn("v4l2_subdev_call(init) is fail(%d)", ret);
			goto p_power_off;
		}

		ret = CALL_CISOPS(cis, cis_set_global_setting, sensor_peri->subdev_cis);
		if (ret) {
			warn("v4l2_subdev_call(cis_set_global_setting) is fail(%d)", ret);
			goto p_power_off;
		}

		ret = CALL_CISOPS(cis, cis_stream_on, sensor_peri->subdev_cis);
		if (ret) {
			warn("v4l2_subdev_call(cis_stream_on) is fail(%d)", ret);
			goto p_power_off;
		}
		CALL_CISOPS(cis, cis_wait_streamon, sensor_peri->subdev_cis);

		ret = CALL_CISOPS(cis, cis_stream_off, sensor_peri->subdev_cis);
		if (ret) {
			warn("v4l2_subdev_call(cis_stream_off) is fail(%d)", ret);
			goto p_power_off;
		}
		CALL_CISOPS(cis, cis_wait_streamoff, sensor_peri->subdev_cis);
	}

p_power_off:
	ret = module->pdata->gpio_cfg(module, scenario, GPIO_SCENARIO_SENSOR_RETENTION_ON);
	if (ret)
		warn("gpio off is fail(%d)", ret);

p_err:
	info("%s: end %d\n", __func__, ret);
}
#endif

int fimc_is_vender_share_i2c_client(struct fimc_is_core *core, u32 source, u32 target)
{
	int ret = 0;
	struct fimc_is_device_sensor *device = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri_source = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri_target = NULL;
	struct fimc_is_cis *cis_source = NULL;
	struct fimc_is_cis *cis_target = NULL;
	int i;

	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		device = &core->sensor[i];
		sensor_peri_source = find_peri_by_cis_id(device, source);
		if (!sensor_peri_source) {
			info("Not device for sensor_peri_source");
			continue;
		}

		cis_source = &sensor_peri_source->cis;
		if (!cis_source) {
			info("cis_source is NULL");
			continue;
		}

		sensor_peri_target = find_peri_by_cis_id(device, target);
		if (!sensor_peri_target) {
			info("Not device for sensor_peri_target");
			continue;
		}

		cis_target = &sensor_peri_target->cis;
		if (!cis_target) {
			info("cis_target is NULL");
			continue;
		}

		cis_target->client = cis_source->client;
		sensor_peri_target->module->client = cis_target->client;
		info("i2c client copy done(source: %d target: %d)\n", source, target);
		break;
	}

	return ret;
}

int fimc_is_vender_hw_init(struct fimc_is_vender *vender)
{
	int i;
	bool ret = false;
	struct device *dev  = NULL;
	struct fimc_is_core *core;
	struct fimc_is_vender_specific *specific = NULL;

	core = container_of(vender, struct fimc_is_core, vender);
	specific = core->vender.private_data;
	dev = &core->ischain[0].pdev->dev;

	info("hw init start\n");

	is_hw_init_running = true;
#ifdef CAMERA_HW_BIG_DATA_FILE_IO
	need_update_to_file = false;
	fimc_is_sec_copy_err_cnt_from_file();
#endif

	ret = fimc_is_vender_check_sensor(core);
	if (ret) {
		err("Do not init hw routine. Check sensor failed!\n");
		is_hw_init_running = false;
		fimc_is_load_ctrl_unlock();
		return -EINVAL;
	} else {
		info("Start hw init. Check sensor success!\n");
	}

#ifdef USE_SHARE_I2C_CLIENT_IMX516_IMX316
	ret = fimc_is_vender_share_i2c_client(core, SENSOR_NAME_IMX516, SENSOR_NAME_IMX316);
	if (ret) {
		err("i2c client copy failed!\n");
		return -EINVAL;
	}
#endif

#ifdef CONFIG_SENSORCORE_MCU_CONTROL
	mutex_lock(&specific->hw_init_lock);
#endif

	for (i = 0; i < ROM_ID_MAX; i++) {
		if (specific->rom_valid[i] == true) {
			ret = fimc_is_sec_run_fw_sel(dev, i);
			if (ret) {
				err("fimc_is_sec_run_fw_sel for ROM_ID(%d) is fail(%d)", i, ret);
			}
		}
	}

	ret = fimc_is_sec_fw_find(core);
	if (ret) {
		err("fimc_is_sec_fw_find is fail(%d)", ret);
	}

#if defined(CONFIG_CAMERA_FROM)
	ret = fimc_is_sec_check_bin_files(core);
	if (ret) {
		err("fimc_is_sec_check_bin_files is fail(%d)", ret);
	}
#endif

#ifndef CONFIG_SENSORCORE_MCU_CONTROL
#if defined (CONFIG_OIS_USE)
	fimc_is_ois_fw_update(core);
#endif
#endif

#if defined(CONFIG_SENSOR_RETENTION_USE) && defined(USE_CAMERA_PREPARE_RETENTION_ON_BOOT)
	fimc_is_vendor_prepare_retention(core, 0, SENSOR_POSITION_REAR);
#endif

#ifdef CONFIG_SENSORCORE_MCU_CONTROL
	mutex_unlock(&specific->hw_init_lock);
#endif

	ret = fimc_is_load_bin_on_boot();
	if (ret) {
		err("fimc_is_load_bin_on_boot is fail(%d)", ret);
	}

#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
	fimc_is_vendor_register_ril_notifier();
#endif
	is_hw_init_running = false;
	factory_aperture_value = 0;
	info("hw init done\n");
	return 0;
}

int fimc_is_vender_fw_prepare(struct fimc_is_vender *vender)
{
	int ret = 0;
	int rom_id;
	struct fimc_is_core *core;
	struct fimc_is_device_preproc *device;
	struct fimc_is_vender_specific *specific;

	WARN_ON(!vender);

	specific = vender->private_data;
	core = container_of(vender, struct fimc_is_core, vender);
	device = &core->preproc;

	rom_id = fimc_is_vendor_get_rom_id_from_position(core->current_position);

	if (rom_id != ROM_ID_REAR) {
		ret = fimc_is_sec_run_fw_sel(&device->pdev->dev, ROM_ID_REAR);
		if (ret < 0) {
			err("fimc_is_sec_run_fw_sel is fail, but continue fw sel");
		}
	}

	ret = fimc_is_sec_run_fw_sel(&device->pdev->dev, rom_id);
	if (ret < 0) {
		err("fimc_is_sec_run_fw_sel is fail1(%d)", ret);
		goto p_err;
	}

	ret = fimc_is_sec_fw_find(core);
	if (ret) {
		err("fimc_is_sec_fw_find is fail(%d)", ret);
	}

#if defined(CONFIG_CAMERA_FROM)
	ret = fimc_is_sec_check_bin_files(core);
	if (ret) {
		err("fimc_is_sec_check_bin_files is fail(%d)", ret);
	}
#endif

p_err:
	return ret;
}

/**
  * fimc_is_vender_request_binary: send loading request to the loader
  * @bin: pointer to fimc_is_binary structure
  * @path: path of binary file
  * @name: name of binary file
  * @device: device for which binary is being loaded
  **/
int fimc_is_vender_request_binary(struct fimc_is_binary *bin, const char *path1, const char *path2,
				const char *name, struct device *device)
{
	char *filename;
	unsigned int retry_cnt = 0;
	int retry_err = 0;
	int ret;

	bin->data = NULL;
	bin->size = 0;
	bin->fw = NULL;

	/* whether the loader is customized or not */
	if (bin->customized != (unsigned long)bin) {
		bin->alloc = &vmalloc;
		bin->free =  &vfree;
	} else {
		retry_cnt = bin->retry_cnt;
		retry_err = bin->retry_err;
	}

	/* read the requested binary from file system directly */
	if (path1) {
		filename = __getname();
		if (unlikely(!filename))
			return -ENOMEM;

		snprintf(filename, PATH_MAX, "%s%s", path1, name);
		ret = get_filesystem_binary(filename, bin);
		__putname(filename);
		/* read successfully or don't want to go further more */
		if (!ret || !device) {
			info("%s path1 load(%s) \n", __func__, name);
			return ret;
		}
	}

	/* read the requested binary from file system directly DUMP  */
	if (path2 && is_dumped_fw_loading_needed) {
		filename = __getname();
		if (unlikely(!filename))
			return -ENOMEM;

		snprintf(filename, PATH_MAX, "%s%s", path2, name);
		ret = get_filesystem_binary(filename, bin);
		__putname(filename);
		/* read successfully or don't want to go further more */
		if (!ret || !device) {
			info("%s path2 load(%s) \n", __func__, name);
			return ret;
		}
	}

	/* ask to 'request_firmware' */
	do {
		ret = request_firmware(&bin->fw, name, device);

		if (!ret && bin->fw) {
			bin->data = (void *)bin->fw->data;
			bin->size = bin->fw->size;

			info("%s path3 load(%s) \n", __func__, name);
			break;
		}

		/*
		 * if no specific error for retry is given;
		 * whatever any error is occurred, we should retry
		 */
		if (!bin->retry_err)
			retry_err = ret;
	} while (retry_cnt-- && (retry_err == ret));

	return ret;
}

int fimc_is_vender_fw_filp_open(struct fimc_is_vender *vender, struct file **fp, int bin_type)
{
	int ret = FW_SKIP;
	struct fimc_is_rom_info *sysfs_finfo;
	char fw_path[FIMC_IS_PATH_LEN];
	struct fimc_is_core *core;

	fimc_is_sec_get_sysfs_finfo(&sysfs_finfo, ROM_ID_REAR);
	core = container_of(vender, struct fimc_is_core, vender);
	memset(fw_path, 0x00, sizeof(fw_path));

	if (bin_type == IS_BIN_SETFILE) {
		if (is_dumped_fw_loading_needed) {
#ifdef CAMERA_MODULE_FRONT_SETF_DUMP
			if (core->current_position == SENSOR_POSITION_FRONT) {
				snprintf(fw_path, sizeof(fw_path),
					"%s%s", FIMC_IS_FW_DUMP_PATH, sysfs_finfo->load_front_setfile_name);
			} else
#endif
#ifdef CAMERA_MODULE_REAR2_SETF_DUMP
			if (core->current_position == SENSOR_POSITION_REAR2) {
				snprintf(fw_path, sizeof(fw_path),
					"%s%s", FIMC_IS_FW_DUMP_PATH, sysfs_finfo->load_setfile2_name);
			} else
#endif
			{
				snprintf(fw_path, sizeof(fw_path),
					"%s%s", FIMC_IS_FW_DUMP_PATH, sysfs_finfo->load_setfile_name);
			}
			*fp = filp_open(fw_path, O_RDONLY, 0);
			if (IS_ERR_OR_NULL(*fp)) {
				*fp = NULL;
				ret = FW_FAIL;
			} else {
				ret = FW_SUCCESS;
			}
		} else {
			ret = FW_SKIP;
		}
	}

	return ret;
}

static int fimc_is_ischain_loadcalb(struct fimc_is_core *core,
	struct fimc_is_module_enum *active_sensor, int position)
{
	int ret = 0;
	char *cal_ptr;
	char *cal_buf = NULL;
	u32 start_addr = 0;
	int cal_size = 0;
	int rom_id = ROM_ID_NOTHING;
	struct fimc_is_rom_info *finfo;
	struct fimc_is_rom_info *pinfo;
	char *loaded_fw_ver;

	rom_id = fimc_is_vendor_get_rom_id_from_position(position);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_cal_buf(&cal_buf, rom_id);

	fimc_is_sec_get_sysfs_pinfo(&pinfo, ROM_ID_REAR);

	cal_size = finfo->rom_size;

	pr_info("%s rom_id : %d, position : %d\n", __func__, rom_id, position);

	if (!fimc_is_sec_check_rom_ver(core, rom_id)) {
		err("Camera : Did not load cal data.");
		return 0;
	}

#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_FRONT)
	if (position == SENSOR_POSITION_FRONT
		|| position == SENSOR_POSITION_FRONT2
		|| position == SENSOR_POSITION_FRONT3) {
		start_addr = CAL_OFFSET1;
#ifdef ENABLE_IS_CORE
		cal_ptr = (char *)(core->resourcemgr.minfo.kvaddr + start_addr);
#else
		cal_ptr = (char *)(core->resourcemgr.minfo.kvaddr_cal[position]);
#endif
	} else
#endif
	{
		start_addr = CAL_OFFSET0;
#ifdef ENABLE_IS_CORE
		cal_ptr = (char *)(core->resourcemgr.minfo.kvaddr + start_addr);
#else
		cal_ptr = (char *)(core->resourcemgr.minfo.kvaddr_cal[position]);
#endif
	}

	fimc_is_sec_get_loaded_fw(&loaded_fw_ver);

	info("CAL DATA : MAP ver : %c%c%c%c\n",
		cal_buf[finfo->rom_header_cal_map_ver_start_addr],
		cal_buf[finfo->rom_header_cal_map_ver_start_addr + 1],
		cal_buf[finfo->rom_header_cal_map_ver_start_addr + 2],
		cal_buf[finfo->rom_header_cal_map_ver_start_addr + 3]);

	info("Camera : Sensor Version : 0x%x\n", cal_buf[finfo->rom_header_sensor_id_addr]);

	info("rom_fw_version = %s, phone_fw_version = %s, loaded_fw_version = %s\n",
		finfo->header_ver, pinfo->header_ver, loaded_fw_ver);

	/* CRC check */
	if (!test_bit(FIMC_IS_CRC_ERROR_ALL_SECTION, &finfo->crc_error)) {
		memcpy((void *)(cal_ptr) ,(void *)cal_buf, cal_size);
		info("Camera %d: the dumped Cal. data was applied successfully.\n", rom_id);
	} else {
		if (!test_bit(FIMC_IS_CRC_ERROR_HEADER, &finfo->crc_error)) {
			err("Camera %d: CRC32 error but only header section is no problem.", rom_id);
			memcpy((void *)(cal_ptr),
				(void *)cal_buf,
				finfo->rom_header_cal_data_start_addr);
			memset((void *)(cal_ptr + finfo->rom_header_cal_data_start_addr),
				0xFF,
				cal_size - finfo->rom_header_cal_data_start_addr);
		} else {
			err("Camera %d: CRC32 error for all section.", rom_id);
			memset((void *)(cal_ptr), 0xFF, cal_size);
			ret = -EIO;
		}
	}

#ifdef ENABLE_IS_CORE
	CALL_BUFOP(core->resourcemgr.minfo.pb_fw, sync_for_device,
		core->resourcemgr.minfo.pb_fw,
		start_addr, cal_size, DMA_TO_DEVICE);
#else
	if (rom_id < ROM_ID_MAX)
		CALL_BUFOP(core->resourcemgr.minfo.pb_cal[rom_id], sync_for_device,
			core->resourcemgr.minfo.pb_cal[rom_id],
			0, cal_size, DMA_TO_DEVICE);
#endif
	if (ret)
		warn("calibration loading is fail");
	else
		info("calibration loading is success\n");

	return ret;
}

int fimc_is_vender_cal_load(struct fimc_is_vender *vender, void *module_data)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct fimc_is_module_enum *module = module_data;

	core = container_of(vender, struct fimc_is_core, vender);

#ifdef ENABLE_IS_CORE
	/* Load calibration data from sensor */
	if (module->position >= SENSOR_POSITION_MAX) {
		module->ext.sensor_con.cal_address = 0;
	} else if (module->position == SENSOR_POSITION_FRONT
				|| module->position == SENSOR_POSITION_FRONT2
				|| module->position == SENSOR_POSITION_FRONT3) {
		module->ext.sensor_con.cal_address = CAL_OFFSET1;
	} else {
		module->ext.sensor_con.cal_address = CAL_OFFSET0;
	}
#endif

	ret = fimc_is_ischain_loadcalb(core, NULL, module->position);
	if (ret) {
		err("loadcalb fail, load default caldata\n");
	}

	return ret;
}

int fimc_is_vender_module_sel(struct fimc_is_vender *vender, void *module_data)
{
	int ret = 0;

	return ret;
}

int fimc_is_vender_module_del(struct fimc_is_vender *vender, void *module_data)
{
	int ret = 0;

	return ret;
}

int fimc_is_vender_fw_sel(struct fimc_is_vender *vender)
{
	int ret = 0;
	int rom_id;
	struct fimc_is_core *core;
	struct device *dev;
	struct fimc_is_rom_info *sysfs_finfo;

	WARN_ON(!vender);

	core = container_of(vender, struct fimc_is_core, vender);
	dev = &core->pdev->dev;

	if (!test_bit(FIMC_IS_PREPROC_S_INPUT, &core->preproc.state)) {
		rom_id = fimc_is_vendor_get_rom_id_from_position(core->current_position);
		ret = fimc_is_sec_run_fw_sel(dev, rom_id);
		if (ret) {
			err("fimc_is_sec_run_fw_sel is fail(%d)", ret);
			goto p_err;
		}
	}

	fimc_is_sec_get_sysfs_finfo(&sysfs_finfo, ROM_ID_REAR);
	snprintf(vender->request_fw_path, sizeof(vender->request_fw_path), "%s",
		sysfs_finfo->load_fw_name);

p_err:
	return ret;
}

int fimc_is_vender_setfile_sel(struct fimc_is_vender *vender,
	char *setfile_name,
	int position)
{
	int ret = 0;
	struct fimc_is_core *core;

	WARN_ON(!vender);
	WARN_ON(!setfile_name);

	core = container_of(vender, struct fimc_is_core, vender);

	snprintf(vender->setfile_path[position], sizeof(vender->setfile_path[position]),
		"%s%s", FIMC_IS_SETFILE_SDCARD_PATH, setfile_name);
	snprintf(vender->request_setfile_path[position],
		sizeof(vender->request_setfile_path[position]),
		"%s", setfile_name);

	return ret;
}

int fimc_is_vendor_get_module_from_position(int position, struct fimc_is_module_enum ** module)
{
	struct fimc_is_core *core;
	int i = 0;

	*module = NULL;

	if (!fimc_is_dev) {
		err("%s: fimc_is_dev is not yet probed", __func__);
		return -ENODEV;
	}

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("%s: core is NULL", __func__);
		return -EINVAL;
	}


	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		fimc_is_search_sensor_module_with_position(&core->sensor[i], position, module);
		if (*module)
			break;
	}

	if (*module == NULL) {
		err("%s: Could not find sensor id.", __func__);
		return -EINVAL;
	}

	return 0;
}

bool fimc_is_vendor_check_camera_running(int position)
{
	struct fimc_is_module_enum * module;
	fimc_is_vendor_get_module_from_position(position, &module);

	if (module) {
		return test_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
	}

	return false;
}

int fimc_is_vendor_get_rom_id_from_position(int position)
{
	struct fimc_is_module_enum * module;
	fimc_is_vendor_get_module_from_position(position, &module);

	if (module) {
		return module->pdata->rom_id;
	}

	return ROM_ID_NOTHING;
}

void fimc_is_vendor_get_rom_info_from_position(int position,
	int *rom_type, int *rom_id, int *rom_cal_index)
{
	struct fimc_is_module_enum * module;
	fimc_is_vendor_get_module_from_position(position, &module);

	if (module) {
		*rom_type = module->pdata->rom_type;
		*rom_id = module->pdata->rom_id;
		*rom_cal_index = module->pdata->rom_cal_index;
	} else {
		*rom_type = ROM_TYPE_NONE;
		*rom_id = ROM_ID_NOTHING;
		*rom_cal_index = 0;
	}
}

void fimc_is_vendor_get_rom_dualcal_info_from_position(int position,
	int *rom_type, int *rom_dualcal_id, int *rom_dualcal_index)
{
	struct fimc_is_module_enum * module;
	fimc_is_vendor_get_module_from_position(position, &module);

	if (module) {
		*rom_type = module->pdata->rom_type;
		*rom_dualcal_id = module->pdata->rom_dualcal_id;
		*rom_dualcal_index = module->pdata->rom_dualcal_index;
	} else {
		*rom_type = ROM_TYPE_NONE;
		*rom_dualcal_id = ROM_ID_NOTHING;
		*rom_dualcal_index = 0;
	}
}

#ifdef CAMERA_PARALLEL_RETENTION_SEQUENCE
void sensor_pwr_ctrl(struct work_struct *work)
{
	int ret = 0;
	struct exynos_platform_fimc_is_module *pdata;
	struct fimc_is_module_enum *g_module = NULL;
	struct fimc_is_core *core;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("core is NULL");
		return;
	}

	ret = fimc_is_preproc_g_module(&core->preproc, &g_module);
	if (ret) {
		err("fimc_is_sensor_g_module is fail(%d)", ret);
		return;
	}

	pdata = g_module->pdata;
	ret = pdata->gpio_cfg(g_module, SENSOR_SCENARIO_NORMAL,
		GPIO_SCENARIO_STANDBY_OFF_SENSOR);
	if (ret) {
		err("gpio_cfg(sensor) is fail(%d)", ret);
	}
}

static DECLARE_DELAYED_WORK(sensor_pwr_ctrl_work, sensor_pwr_ctrl);
#endif

int fimc_is_vender_sensor_gpio_on_sel(struct fimc_is_vender *vender, u32 scenario, u32 *gpio_scenario)
{
	int ret = 0;

	return ret;
}

int fimc_is_vender_sensor_gpio_on(struct fimc_is_vender *vender, u32 scenario, u32 gpio_scenario)
{
	int ret = 0;

	return ret;
}

int fimc_is_vender_sensor_gpio_off_sel(struct fimc_is_vender *vender, u32 scenario, u32 *gpio_scenario,
		void *module_data)
{
	int ret = 0;
#ifdef CONFIG_SENSOR_RETENTION_USE
	struct fimc_is_module_enum *module = module_data;
	struct sensor_open_extended *ext_info;
	struct fimc_is_core *core;

	core = container_of(vender, struct fimc_is_core, vender);
	ext_info = &(((struct fimc_is_module_enum *)module)->ext);

	if ((ext_info->use_retention_mode == SENSOR_RETENTION_ACTIVATED)
		&& (scenario == SENSOR_SCENARIO_NORMAL)) {
		*gpio_scenario = GPIO_SCENARIO_SENSOR_RETENTION_ON;
#if defined(CONFIG_OIS_USE) && defined(USE_OIS_SLEEP_MODE)
		/* Enable OIS gyro sleep */
		if (module->position == SENSOR_POSITION_REAR)
			fimc_is_ois_gyro_sleep(core);
#endif
		info("%s: use_retention_mode\n", __func__);
	}
#endif

	return ret;
}

int fimc_is_vender_sensor_gpio_off(struct fimc_is_vender *vender, u32 scenario, u32 gpio_scenario)
{
	int ret = 0;

	return ret;
}

#ifdef CONFIG_SENSOR_RETENTION_USE
void fimc_is_vender_check_retention(struct fimc_is_vender *vender, void *module_data)
{
	struct fimc_is_vender_specific *specific;
	struct fimc_is_rom_info *sysfs_finfo;
	struct fimc_is_module_enum *module = module_data;
	struct sensor_open_extended *ext_info;

	fimc_is_sec_get_sysfs_finfo(&sysfs_finfo, ROM_ID_REAR);
	specific = vender->private_data;
	ext_info = &(((struct fimc_is_module_enum *)module)->ext);

	if ((ext_info->use_retention_mode != SENSOR_RETENTION_UNSUPPORTED)
		&& (force_caldata_dump == false)) {
		info("Sensor[id = %d] use retention mode.\n", specific->rear_sensor_id);
	} else { /* force_caldata_dump == true */
		ext_info->use_retention_mode = SENSOR_RETENTION_UNSUPPORTED;
		info("Sensor[id = %d] does not support retention mode.\n", specific->rear_sensor_id);
	}

	return;
}
#endif

void fimc_is_vender_sensor_s_input(struct fimc_is_vender *vender, void *module)
{
	fimc_is_vender_fw_prepare(vender);

	return;
}

void fimc_is_vender_itf_open(struct fimc_is_vender *vender, struct sensor_open_extended *ext_info)
{
	struct fimc_is_vender_specific *specific;
	struct fimc_is_rom_info *sysfs_finfo;
	struct fimc_is_core *core;

	fimc_is_sec_get_sysfs_finfo(&sysfs_finfo, ROM_ID_REAR);
	specific = vender->private_data;
	core = container_of(vender, struct fimc_is_core, vender);

	specific->zoom_running = false;

	return;
}

/* Flash Mode Control */
#ifdef CONFIG_LEDS_LM3560
extern int lm3560_reg_update_export(u8 reg, u8 mask, u8 data);
#endif
#ifdef CONFIG_LEDS_SKY81296
extern int sky81296_torch_ctrl(int state);
#endif
#if defined(CONFIG_TORCH_CURRENT_CHANGE_SUPPORT) && defined(CONFIG_LEDS_S2MPB02)
extern int s2mpb02_set_torch_current(bool torch_mode, bool change_current, int intensity);
#endif

int fimc_is_vender_set_torch(struct camera2_shot *shot)
{
	u32 aeflashMode = shot->ctl.aa.vendor_aeflashMode;

	switch (aeflashMode) {
	case AA_FLASHMODE_ON_ALWAYS: /*TORCH mode*/
#ifdef CONFIG_LEDS_LM3560
		lm3560_reg_update_export(0xE0, 0xFF, 0xEF);
#elif defined(CONFIG_LEDS_SKY81296)
		sky81296_torch_ctrl(1);
#endif
#if defined(CONFIG_TORCH_CURRENT_CHANGE_SUPPORT) && defined(CONFIG_LEDS_S2MPB02)
#if defined(LEDS_S2MPB02_ADAPTIVE_MOVIE_CURRENT)
		info("s2mpb02 adaptive firingPower(%d)\n", shot->ctl.flash.firingPower);

		if (shot->ctl.flash.firingPower == 5)
			s2mpb02_set_torch_current(true, true, LEDS_S2MPB02_ADAPTIVE_MOVIE_CURRENT);
		else
#endif
			s2mpb02_set_torch_current(true, false, 0);
#endif
		break;
	case AA_FLASHMODE_START: /*Pre flash mode*/
#ifdef CONFIG_LEDS_LM3560
		lm3560_reg_update_export(0xE0, 0xFF, 0xEF);
#elif defined(CONFIG_LEDS_SKY81296)
		sky81296_torch_ctrl(1);
#endif
#if defined(CONFIG_TORCH_CURRENT_CHANGE_SUPPORT) && defined(CONFIG_LEDS_S2MPB02)
		s2mpb02_set_torch_current(false, false, 0);
#endif
		break;
	case AA_FLASHMODE_CAPTURE: /*Main flash mode*/
		break;
	case AA_FLASHMODE_OFF: /*OFF mode*/
#ifdef CONFIG_LEDS_SKY81296
		sky81296_torch_ctrl(0);
#endif
		break;
	default:
		break;
	}

	return 0;
}

int fimc_is_vender_video_s_ctrl(struct v4l2_control *ctrl,
	void *device_data)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = (struct fimc_is_device_ischain *)device_data;
	struct fimc_is_core *core;
	struct fimc_is_vender_specific *specific;
	unsigned int value = 0;
	unsigned int captureIntent = 0;
	unsigned int captureCount = 0;

	WARN_ON(!device);
	WARN_ON(!ctrl);

	core = (struct fimc_is_core *)platform_get_drvdata(device->pdev);
	specific = core->vender.private_data;

	switch (ctrl->id) {
	case V4L2_CID_IS_INTENT:
		ctrl->id = VENDER_S_CTRL;
		value = (unsigned int)ctrl->value;
		captureIntent = (value >> 16) & 0x0000FFFF;
		switch (captureIntent) {
		case AA_CAPTURE_INTENT_STILL_CAPTURE_DEBLUR_DYNAMIC_SHOT:
		case AA_CAPTURE_INTENT_STILL_CAPTURE_OIS_DYNAMIC_SHOT:
		case AA_CAPTURE_INTENT_STILL_CAPTURE_EXPOSURE_DYNAMIC_SHOT:
		case AA_CAPTURE_INTENT_STILL_CAPTURE_MFHDR_DYNAMIC_SHOT:
		case AA_CAPTURE_INTENT_STILL_CAPTURE_LLHDR_DYNAMIC_SHOT:
		case AA_CAPTURE_INTENT_STILL_CAPTURE_SUPER_NIGHT_SHOT_HANDHELD:
		case AA_CAPTURE_INTENT_STILL_CAPTURE_SUPER_NIGHT_SHOT_TRIPOD:
		case AA_CAPTURE_INTENT_STILL_CAPTURE_LLHDR_VEHDR_DYNAMIC_SHOT:
		case AA_CAPTURE_INTENT_STILL_CAPTURE_VENR_DYNAMIC_SHOT:
		case AA_CAPTURE_INTENT_STILL_CAPTURE_LLS_FLASH:
			captureCount = value & 0x0000FFFF;
			break;
		default:
			captureIntent = ctrl->value;
			captureCount = 0;
			break;
		}

		device->group_3aa.intent_ctl.captureIntent = captureIntent;
		device->group_3aa.intent_ctl.vendor_captureCount = captureCount;

                if (captureIntent == AA_CAPTURE_INTENT_STILL_CAPTURE_OIS_MULTI) {
                        device->group_3aa.remainIntentCount = 2;
                } else {
                        device->group_3aa.remainIntentCount = 0;
                }
		minfo("[VENDER] s_ctrl intent(%d) count(%d) remainIntentCount(%d)\n",
			device, captureIntent, captureCount, device->group_3aa.remainIntentCount);
		break;
	case V4L2_CID_IS_CAPTURE_EXPOSURETIME:
		ctrl->id = VENDER_S_CTRL;
		device->group_3aa.intent_ctl.vendor_captureExposureTime = ctrl->value;
		minfo("[VENDER] s_ctrl vendor_captureExposureTime(%d)\n", device, ctrl->value);
		break;
	case V4L2_CID_IS_TRANSIENT_ACTION:
		ctrl->id = VENDER_S_CTRL;

		minfo("[VENDOR] transient action(%d)\n", device, ctrl->value);
		if (ctrl->value == ACTION_ZOOMING)
			specific->zoom_running = true;
		else
			specific->zoom_running = false;
		break;
	case V4L2_CID_IS_FORCE_FLASH_MODE:
		if (device->sensor != NULL) {
			struct v4l2_subdev *subdev_flash;

			subdev_flash = device->sensor->subdev_flash;

			if (subdev_flash != NULL) {
				struct fimc_is_flash *flash = NULL;

				flash = (struct fimc_is_flash *)v4l2_get_subdevdata(subdev_flash);
				FIMC_BUG(!flash);

				minfo("[VENDOR] force flash mode\n", device);

				ctrl->id = V4L2_CID_FLASH_SET_FIRE;
				if (ctrl->value == CAM2_FLASH_MODE_OFF) {
					ctrl->value = 0; /* intensity */
					flash->flash_data.mode = CAM2_FLASH_MODE_OFF;
					flash->flash_data.flash_fired = false;
					ret = v4l2_subdev_call(subdev_flash, core, s_ctrl, ctrl);
				}
			}
		}
		break;
	case V4L2_CID_IS_FACTORY_APERTURE_CONTROL:
		ctrl->id = VENDER_S_CTRL;
		device->group_3aa.lens_ctl.aperture = ctrl->value;
		if (factory_aperture_value != ctrl->value) {
			minfo("[VENDER] s_ctrl aperture(%d)\n", device, ctrl->value);
			factory_aperture_value = ctrl->value;
		}
		break;
	case V4L2_CID_IS_CAMERA_TYPE:
		ctrl->id = VENDER_S_CTRL;
		switch (ctrl->value) {
		case IS_COLD_BOOT:
			/* change value to X when !TWIZ | front */
			fimc_is_itf_fwboot_init(device->interface);
			break;
		case IS_WARM_BOOT:
			if (specific ->need_cold_reset) {
				minfo("[VENDER] FW first launching mode for reset\n", device);
				device->interface->fw_boot_mode = FIRST_LAUNCHING;
			} else {
				/* change value to X when TWIZ & back | frist time back camera */
				if (!test_bit(IS_IF_LAUNCH_FIRST, &device->interface->launch_state))
					device->interface->fw_boot_mode = FIRST_LAUNCHING;
				else
					device->interface->fw_boot_mode = WARM_BOOT;
			}
			break;
		case IS_COLD_RESET:
			specific ->need_cold_reset = true;
			minfo("[VENDER] need cold reset!!!\n", device);

			/* Dump preprocessor and sensor debug info */
			fimc_is_vender_csi_err_print_debug_log(device->sensor);
			break;
		default:
			err("[VENDER]unsupported ioctl(0x%X)", ctrl->id);
			ret = -EINVAL;
			break;
		}
		break;
#ifdef CONFIG_SENSOR_RETENTION_USE
	case V4L2_CID_IS_PREVIEW_STATE:
		ctrl->id = VENDER_S_CTRL;
		break;
#endif
	}

	return ret;
}

int fimc_is_vender_ssx_video_s_ctrl(struct v4l2_control *ctrl,
	void *device_data)
{
	return 0;
}

int fimc_is_vender_ssx_video_s_ext_ctrl(struct v4l2_ext_controls *ctrls, void *device_data)
{
	int ret = 0;
	int i;
	struct fimc_is_device_sensor *device = (struct fimc_is_device_sensor *)device_data;
	struct fimc_is_core *core;
	struct fimc_is_vender_specific *specific;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct v4l2_ext_control *ext_ctrl;

	WARN_ON(!device);
	WARN_ON(!ctrls);

	core = (struct fimc_is_core *)platform_get_drvdata(device->pdev);
	specific = core->vender.private_data;

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(device->subdev_module);
	if (unlikely(!module)) {
		err("%s, module in is NULL", __func__);
		module = NULL;
		ret = -1;
		goto p_err;
	}
	WARN_ON(!module);

	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;

	WARN_ON(!sensor_peri);

	for (i = 0; i < ctrls->count; i++) {
		ext_ctrl = (ctrls->controls + i);
		switch (ext_ctrl->id) {
		default:
			break;
		}
	}

p_err:
	return ret;
}

int fimc_is_vender_ssx_video_g_ctrl(struct v4l2_control *ctrl,
	void *device_data)
{
	return 0;
}

void fimc_is_vender_resource_get(struct fimc_is_vender *vender)
{

}

void fimc_is_vender_resource_put(struct fimc_is_vender *vender)
{

}

bool fimc_is_vender_wdr_mode_on(void *cis_data)
{
#if defined(CONFIG_CAMERA_PDP)
	return (((cis_shared_data *)cis_data)->is_data.wdr_mode != CAMERA_WDR_OFF ? true : false);
#else
	return false;
#endif
}

bool fimc_is_vender_enable_wdr(void *cis_data)
{
	return false;
}

int fimc_is_vender_remove_dump_fw_file(void)
{
	info("%s\n", __func__);
	remove_dump_fw_file();

	return 0;
}
