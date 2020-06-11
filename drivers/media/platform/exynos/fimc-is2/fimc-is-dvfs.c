/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is core functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#if defined(CONFIG_SCHED_EHMP)
#include <linux/ehmp.h>
#elif defined(CONFIG_SCHED_EMS)
#include <linux/ems.h>
#endif
#include "fimc-is-core.h"
#include "fimc-is-dvfs.h"
#include "fimc-is-hw-dvfs.h"
#include <linux/videodev2_exynos_camera.h>

#ifdef CONFIG_PM_DEVFREQ

#if defined(QOS_INTCAM)
extern struct pm_qos_request exynos_isp_qos_int_cam;
#endif
extern struct pm_qos_request exynos_isp_qos_int;
extern struct pm_qos_request exynos_isp_qos_mem;
extern struct pm_qos_request exynos_isp_qos_cam;
extern struct pm_qos_request exynos_isp_qos_hpg;

#if defined(CONFIG_SCHED_EHMP) || defined(CONFIG_SCHED_EMS)
extern struct gb_qos_request gb_req;
#endif

static inline int fimc_is_get_start_sensor_cnt(struct fimc_is_core *core)
{
	int i, sensor_cnt = 0;

	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++)
		if (test_bit(FIMC_IS_SENSOR_OPEN, &(core->sensor[i].state)))
			sensor_cnt++;

	return sensor_cnt;
}

#ifdef BDS_DVFS
static int fimc_is_get_bds_size(struct fimc_is_device_ischain *device)
{
	int resol = 0;
	struct fimc_is_group *group;

	group = &device->group_3aa;

	if (!test_bit(FIMC_IS_GROUP_INIT, &group->state))
		return resol;

	resol = device->txp.output.width * device->txp.output.height;

	return resol;
}
#else
static int fimc_is_get_target_resol(struct fimc_is_device_ischain *device)
{
	int resol = 0;
#ifdef SOC_MCS
	int i = 0;
	struct fimc_is_group *group;

	group = &device->group_mcs;

	if (!test_bit(FIMC_IS_GROUP_INIT, &group->state))
		return resol;

	for (i = ENTRY_M0P; i <= ENTRY_M4P; i++)
		if (group->subdev[i] && test_bit(FIMC_IS_SUBDEV_START, &(group->subdev[i]->state)))
			resol = max_t(int, resol, group->subdev[i]->output.width * group->subdev[i]->output.height);
#else
	resol = device->scp.output.width * device->scp.output.height;
#endif
	return resol;
}
#endif

int fimc_is_dvfs_init(struct fimc_is_resourcemgr *resourcemgr)
{
	int ret = 0;
	struct fimc_is_dvfs_ctrl *dvfs_ctrl;

	FIMC_BUG(!resourcemgr);

	dvfs_ctrl = &resourcemgr->dvfs_ctrl;

#if defined(QOS_INTCAM)
	dvfs_ctrl->cur_int_cam_qos = 0;
#endif
	dvfs_ctrl->cur_int_qos = 0;
	dvfs_ctrl->cur_mif_qos = 0;
	dvfs_ctrl->cur_cam_qos = 0;
	dvfs_ctrl->cur_i2c_qos = 0;
	dvfs_ctrl->cur_disp_qos = 0;
	dvfs_ctrl->cur_hpg_qos = 0;
	dvfs_ctrl->cur_hmp_bst = 0;

	/* init spin_lock for clock gating */
	mutex_init(&dvfs_ctrl->lock);

	if (!(dvfs_ctrl->static_ctrl))
		dvfs_ctrl->static_ctrl =
			kzalloc(sizeof(struct fimc_is_dvfs_scenario_ctrl), GFP_KERNEL);
	if (!(dvfs_ctrl->dynamic_ctrl))
		dvfs_ctrl->dynamic_ctrl =
			kzalloc(sizeof(struct fimc_is_dvfs_scenario_ctrl), GFP_KERNEL);
	if (!(dvfs_ctrl->external_ctrl))
		dvfs_ctrl->external_ctrl =
			kzalloc(sizeof(struct fimc_is_dvfs_scenario_ctrl), GFP_KERNEL);

	if (!dvfs_ctrl->static_ctrl || !dvfs_ctrl->dynamic_ctrl || !dvfs_ctrl->external_ctrl) {
		err("dvfs_ctrl alloc is failed!!\n");
		return -ENOMEM;
	}

	/* assign static / dynamic scenario check logic data */
	ret = fimc_is_hw_dvfs_init((void *)dvfs_ctrl);
	if (ret) {
		err("fimc_is_hw_dvfs_init is failed(%d)\n", ret);
		return -EINVAL;
	}

	/* default value is 0 */
	dvfs_ctrl->dvfs_table_idx = 0;
	clear_bit(FIMC_IS_DVFS_SEL_TABLE, &dvfs_ctrl->state);

	return 0;
}

int fimc_is_dvfs_sel_table(struct fimc_is_resourcemgr *resourcemgr)
{
	int ret = 0;
	struct fimc_is_dvfs_ctrl *dvfs_ctrl;
	u32 dvfs_table_idx = 0;

	FIMC_BUG(!resourcemgr);

	dvfs_ctrl = &resourcemgr->dvfs_ctrl;

	if (test_bit(FIMC_IS_DVFS_SEL_TABLE, &dvfs_ctrl->state))
		return 0;

#if defined(EXPANSION_DVFS_TABLE)
	switch(resourcemgr->hal_version) {
	case IS_HAL_VER_1_0:
		dvfs_table_idx = 0;
		break;
	case IS_HAL_VER_3_2:
		dvfs_table_idx = 1;
		break;
	default:
		err("hal version is unknown");
		dvfs_table_idx = 0;
		ret = -EINVAL;
		break;
	}
#endif

	if (dvfs_table_idx >= dvfs_ctrl->dvfs_table_max) {
		err("dvfs index(%d) is invalid", dvfs_table_idx);
		ret = -EINVAL;
		goto p_err;
	}

	resourcemgr->dvfs_ctrl.dvfs_table_idx = dvfs_table_idx;
	set_bit(FIMC_IS_DVFS_SEL_TABLE, &dvfs_ctrl->state);

p_err:
	info("[RSC] %s(%d):%d\n", __func__, dvfs_table_idx, ret);
	return ret;
}

int fimc_is_dvfs_sel_static(struct fimc_is_device_ischain *device)
{
	struct fimc_is_core *core;
	struct fimc_is_dvfs_ctrl *dvfs_ctrl;
	struct fimc_is_dvfs_scenario_ctrl *static_ctrl;
	struct fimc_is_dvfs_scenario *scenarios;
	struct fimc_is_resourcemgr *resourcemgr;
	struct fimc_is_dual_info *dual_info;
	int i, scenario_id, scenario_cnt;
	int position, resol, fps, stream_cnt, streaming_cnt;
	unsigned long sensor_map;

	FIMC_BUG(!device);
	FIMC_BUG(!device->interface);

	core = (struct fimc_is_core *)device->interface->core;
	resourcemgr = device->resourcemgr;
	dvfs_ctrl = &(resourcemgr->dvfs_ctrl);
	static_ctrl = dvfs_ctrl->static_ctrl;

	if (!test_bit(FIMC_IS_DVFS_SEL_TABLE, &dvfs_ctrl->state)) {
		err("dvfs table is NOT selected");
		return -EINVAL;
	}

	/* static scenario */
	if (!static_ctrl) {
		err("static_dvfs_ctrl is NULL");
		return -EINVAL;
	}

	if (static_ctrl->scenario_cnt == 0) {
		pr_debug("static_scenario's count is zero");
		return -EINVAL;
	}

	scenarios = static_ctrl->scenarios;
	scenario_cnt = static_ctrl->scenario_cnt;
	position = fimc_is_sensor_g_position(device->sensor);
#ifdef BDS_DVFS
	resol = fimc_is_get_bds_size(device);
#else
	resol = fimc_is_get_target_resol(device);
#endif
	fps = fimc_is_sensor_g_framerate(device->sensor);

	/*
	 * stream_cnt : number of open sensors
	 * streaming_cnt : number of sensors in operation
	 */
	stream_cnt = fimc_is_get_start_sensor_cnt(core);
	streaming_cnt = resourcemgr->streaming_cnt;
	sensor_map = core->sensor_map;
	dual_info = &core->dual_info;

	for (i = 0; i < scenario_cnt; i++) {
		if (!scenarios[i].check_func) {
			warn("check_func[%d] is NULL\n", i);
			continue;
		}

		if ((scenarios[i].check_func(device, NULL, position, resol, fps,
			stream_cnt, streaming_cnt, sensor_map, dual_info)) > 0) {
			scenario_id = scenarios[i].scenario_id;
			static_ctrl->cur_scenario_id = scenario_id;
			static_ctrl->cur_scenario_idx = i;
			static_ctrl->cur_frame_tick = scenarios[i].keep_frame_tick;
			return scenario_id;
		}
	}

	warn("couldn't find static dvfs scenario [sensor:(%d/%d/%d)/fps:%d/setfile:%d/resol:(%d)]\n",
		fimc_is_get_start_sensor_cnt(core), streaming_cnt,
		device->sensor->pdev->id,
		fps, (device->setfile & FIMC_IS_SETFILE_MASK), resol);

	static_ctrl->cur_scenario_id = FIMC_IS_SN_DEFAULT;
	static_ctrl->cur_scenario_idx = -1;
	static_ctrl->cur_frame_tick = -1;

	return FIMC_IS_SN_DEFAULT;
}

int fimc_is_dvfs_sel_dynamic(struct fimc_is_device_ischain *device, struct fimc_is_group *group)
{
	int ret;
	struct fimc_is_core *core;
	struct fimc_is_dvfs_ctrl *dvfs_ctrl;
	struct fimc_is_dvfs_scenario_ctrl *dynamic_ctrl;
	struct fimc_is_dvfs_scenario *scenarios;
	struct fimc_is_resourcemgr *resourcemgr;
	struct fimc_is_dual_info *dual_info;
	int i, scenario_id, scenario_cnt;
	int position, resol, fps;
	unsigned long sensor_map;

	FIMC_BUG(!device);

	core = (struct fimc_is_core *)device->interface->core;
	resourcemgr = device->resourcemgr;
	dvfs_ctrl = &(resourcemgr->dvfs_ctrl);
	dynamic_ctrl = dvfs_ctrl->dynamic_ctrl;

	if (!test_bit(FIMC_IS_DVFS_SEL_TABLE, &dvfs_ctrl->state)) {
		err("dvfs table is NOT selected");
		return -EINVAL;
	}

	/* dynamic scenario */
	if (!dynamic_ctrl) {
		err("dynamic_dvfs_ctrl is NULL");
		return -EINVAL;
	}

	if (dynamic_ctrl->scenario_cnt == 0) {
		pr_debug("dynamic_scenario's count is zero");
		return -EINVAL;
	}

	scenarios = dynamic_ctrl->scenarios;
	scenario_cnt = dynamic_ctrl->scenario_cnt;

	if (dynamic_ctrl->cur_frame_tick >= 0) {
		(dynamic_ctrl->cur_frame_tick)--;
		/*
		 * when cur_frame_tick is lower than 0, clear current scenario.
		 * This means that current frame tick to keep dynamic scenario
		 * was expired.
		 */
		if (dynamic_ctrl->cur_frame_tick < 0) {
			dynamic_ctrl->cur_scenario_id = -1;
			dynamic_ctrl->cur_scenario_idx = -1;
		}
	}

	if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state) || group->id == GROUP_ID_VRA0)
		return -EAGAIN;

	position = fimc_is_sensor_g_position(device->sensor);
#ifdef BDS_DVFS
	resol = fimc_is_get_bds_size(device);
#else
	resol = fimc_is_get_target_resol(device);
#endif
	fps = fimc_is_sensor_g_framerate(device->sensor);
	sensor_map = core->sensor_map;
	dual_info = &core->dual_info;

	for (i = 0; i < scenario_cnt; i++) {
		if (!scenarios[i].check_func) {
			warn("check_func[%d] is NULL\n", i);
			continue;
		}

		ret = scenarios[i].check_func(device, group, position, resol, fps, 0, 0, sensor_map, dual_info);
		switch (ret) {
		case DVFS_MATCHED:
			scenario_id = scenarios[i].scenario_id;
			dynamic_ctrl->cur_scenario_id = scenario_id;
			dynamic_ctrl->cur_scenario_idx = i;
			dynamic_ctrl->cur_frame_tick = scenarios[i].keep_frame_tick;

			return scenario_id;
		case DVFS_SKIP:
			goto p_again;
		case DVFS_NOT_MATCHED:
		default:
			continue;
		}
	}

p_again:
	return  -EAGAIN;
}

int fimc_is_dvfs_sel_external(struct fimc_is_device_sensor *device)
{
	int ret;
	struct fimc_is_core *core;
	struct fimc_is_dvfs_ctrl *dvfs_ctrl;
	struct fimc_is_dvfs_scenario_ctrl *external_ctrl;
	struct fimc_is_dvfs_scenario *scenarios;
	struct fimc_is_resourcemgr *resourcemgr;
	struct fimc_is_dual_info *dual_info;
	int i, scenario_id, scenario_cnt;
	int position, resol, fps, stream_cnt, streaming_cnt;
	unsigned long sensor_map;

	FIMC_BUG(!device);

	core = device->private_data;
	resourcemgr = device->resourcemgr;
	dvfs_ctrl = &(resourcemgr->dvfs_ctrl);
	external_ctrl = dvfs_ctrl->external_ctrl;

	if (!test_bit(FIMC_IS_DVFS_SEL_TABLE, &dvfs_ctrl->state)) {
		err("dvfs table is NOT selected");
		return -EINVAL;
	}

	/* external scenario */
	if (!external_ctrl) {
		warn("external_dvfs_ctrl is NULL, default max dvfs lv");
		return FIMC_IS_SN_MAX;
	}

	scenarios = external_ctrl->scenarios;
	scenario_cnt = external_ctrl->scenario_cnt;
	position = fimc_is_sensor_g_position(device);
	resol = fimc_is_sensor_g_width(device) * fimc_is_sensor_g_height(device);
	fps = fimc_is_sensor_g_framerate(device);

	/*
	 * stream_cnt : number of open sensors
	 * streaming_cnt : number of sensors in operation
	 */
	stream_cnt = fimc_is_get_start_sensor_cnt(core);
	streaming_cnt = resourcemgr->streaming_cnt;
	sensor_map = core->sensor_map;
	dual_info = &core->dual_info;

	for (i = 0; i < scenario_cnt; i++) {
		if (!scenarios[i].ext_check_func) {
			warn("check_func[%d] is NULL\n", i);
			continue;
		}

		ret = scenarios[i].ext_check_func(device, position, resol, fps,
				stream_cnt, streaming_cnt, sensor_map, dual_info);
		switch (ret) {
		case DVFS_MATCHED:
			scenario_id = scenarios[i].scenario_id;
			external_ctrl->cur_scenario_id = scenario_id;
			external_ctrl->cur_scenario_idx = i;
			external_ctrl->cur_frame_tick = scenarios[i].keep_frame_tick;
			return scenario_id;
		case DVFS_SKIP:
			return -EAGAIN;
		case DVFS_NOT_MATCHED:
		default:
			continue;
		}
	}

	warn("couldn't find external dvfs scenario [sensor:(%d/%d/%d)/fps:%d/resol:(%d)]\n",
		stream_cnt, streaming_cnt, position, fps, resol);

	external_ctrl->cur_scenario_id = FIMC_IS_SN_MAX;
	external_ctrl->cur_scenario_idx = -1;
	external_ctrl->cur_frame_tick = -1;

	return FIMC_IS_SN_MAX;
}


int fimc_is_get_qos(struct fimc_is_core *core, u32 type, u32 scenario_id)
{
	struct exynos_platform_fimc_is	*pdata = NULL;
	int qos = 0;
	u32 dvfs_idx = core->resourcemgr.dvfs_ctrl.dvfs_table_idx;

	pdata = core->pdata;
	if (pdata == NULL) {
		err("pdata is NULL\n");
		return -EINVAL;
	}

	if (type >= FIMC_IS_DVFS_END) {
		err("Cannot find DVFS value");
		return -EINVAL;
	}

	if (dvfs_idx >= FIMC_IS_DVFS_TABLE_IDX_MAX) {
		err("invalid dvfs index(%d)", dvfs_idx);
		dvfs_idx = 0;
	}

	qos = pdata->dvfs_data[dvfs_idx][scenario_id][type];

	return qos;
}

int fimc_is_set_dvfs(struct fimc_is_core *core, struct fimc_is_device_ischain *device, u32 scenario_id)
{
	int ret = 0;
#if defined(QOS_INTCAM)
	int int_cam_qos, int_qos, mif_qos, i2c_qos, cam_qos, disp_qos, hpg_qos;
#else
	int int_qos, mif_qos, i2c_qos, cam_qos, disp_qos, hpg_qos;
#endif
	struct fimc_is_resourcemgr *resourcemgr;
	struct fimc_is_dvfs_ctrl *dvfs_ctrl;

	if (core == NULL) {
		err("core is NULL\n");
		return -EINVAL;
	}

	resourcemgr = &core->resourcemgr;
	dvfs_ctrl = &(resourcemgr->dvfs_ctrl);

#if defined(QOS_INTCAM)
	int_cam_qos = fimc_is_get_qos(core, FIMC_IS_DVFS_INT_CAM, scenario_id);
#endif
	int_qos = fimc_is_get_qos(core, FIMC_IS_DVFS_INT, scenario_id);
	mif_qos = fimc_is_get_qos(core, FIMC_IS_DVFS_MIF, scenario_id);
	i2c_qos = fimc_is_get_qos(core, FIMC_IS_DVFS_I2C, scenario_id);
	cam_qos = fimc_is_get_qos(core, FIMC_IS_DVFS_CAM, scenario_id);
	disp_qos = fimc_is_get_qos(core, FIMC_IS_DVFS_DISP, scenario_id);
	hpg_qos = fimc_is_get_qos(core, FIMC_IS_DVFS_HPG, scenario_id);

#if defined(QOS_INTCAM)
	if ((int_cam_qos < 0) || (int_qos < 0) || (mif_qos < 0)
	|| (i2c_qos < 0) || (cam_qos < 0) || (disp_qos < 0)) {
		err("getting qos value is failed!!\n");
		return -EINVAL;
	}
#else
	if ((int_qos < 0) || (mif_qos < 0) || (i2c_qos < 0)
	|| (cam_qos < 0) || (disp_qos < 0)) {
		err("getting qos value is failed!!\n");
		return -EINVAL;
	}
#endif

#if defined(QOS_INTCAM)
	/* check current qos */
	if (int_cam_qos && dvfs_ctrl->cur_int_cam_qos != int_cam_qos) {
		if (i2c_qos && device) {
			ret = fimc_is_itf_i2c_lock(device, i2c_qos, true);
			if (ret) {
				err("fimc_is_itf_i2_clock fail\n");
				goto exit;
			}
		}

		pm_qos_update_request(&exynos_isp_qos_int_cam, int_cam_qos);
		dvfs_ctrl->cur_int_cam_qos = int_cam_qos;

		if (i2c_qos && device) {
			/* i2c unlock */
			ret = fimc_is_itf_i2c_lock(device, i2c_qos, false);
			if (ret) {
				err("fimc_is_itf_i2c_unlock fail\n");
				goto exit;
			}
		}
	}

	if (int_qos && dvfs_ctrl->cur_int_qos != int_qos) {
		pm_qos_update_request(&exynos_isp_qos_int, int_qos);
		dvfs_ctrl->cur_int_qos = int_qos;
	}
#else
	/* check current qos */
	if (int_qos && dvfs_ctrl->cur_int_qos != int_qos) {
		if (i2c_qos && device) {
			ret = fimc_is_itf_i2c_lock(device, i2c_qos, true);
			if (ret) {
				err("fimc_is_itf_i2_clock fail\n");
				goto exit;
			}
		}

		pm_qos_update_request(&exynos_isp_qos_int, int_qos);
		dvfs_ctrl->cur_int_qos = int_qos;

		if (i2c_qos && device) {
			/* i2c unlock */
			ret = fimc_is_itf_i2c_lock(device, i2c_qos, false);
			if (ret) {
				err("fimc_is_itf_i2c_unlock fail\n");
				goto exit;
			}
		}
	}
#endif

	if (mif_qos && dvfs_ctrl->cur_mif_qos != mif_qos) {
		pm_qos_update_request(&exynos_isp_qos_mem, mif_qos);
		dvfs_ctrl->cur_mif_qos = mif_qos;
	}

	if (cam_qos && dvfs_ctrl->cur_cam_qos != cam_qos) {
		pm_qos_update_request(&exynos_isp_qos_cam, cam_qos);
		dvfs_ctrl->cur_cam_qos = cam_qos;
	}

#if defined(ENABLE_HMP_BOOST)
	/* hpg_qos : number of minimum online CPU */
	if (hpg_qos && dvfs_ctrl->cur_hpg_qos != hpg_qos) {
		pm_qos_update_request(&exynos_isp_qos_hpg, hpg_qos);
		dvfs_ctrl->cur_hpg_qos = hpg_qos;

#if defined(CONFIG_HMP_VARIABLE_SCALE)
		/* for migration to big core */
		if (hpg_qos > 4) {
			if (!dvfs_ctrl->cur_hmp_bst) {
				set_hmp_boost(1);
				dvfs_ctrl->cur_hmp_bst = 1;
			}
		} else {
			if (dvfs_ctrl->cur_hmp_bst) {
				set_hmp_boost(0);
				dvfs_ctrl->cur_hmp_bst = 0;
			}
		}
#elif defined(CONFIG_SCHED_EHMP) || defined(CONFIG_SCHED_EMS)
		/* for migration to big core */
		if (hpg_qos > 4) {
			if (!dvfs_ctrl->cur_hmp_bst) {
				gb_qos_update_request(&gb_req, 100);
				dvfs_ctrl->cur_hmp_bst = 1;
			}
		} else {
			if (dvfs_ctrl->cur_hmp_bst) {
				gb_qos_update_request(&gb_req, 0);
				dvfs_ctrl->cur_hmp_bst = 0;
			}
		}
#endif
	}
#endif

#if defined(QOS_INTCAM)
	info("[RSC:%d]: New QoS [INT_CAM(%d), INT(%d), MIF(%d), CAM(%d), DISP(%d), I2C(%d), HPG(%d, %d)]\n",
			device ? device->instance : 0, int_cam_qos, int_qos, mif_qos,
			cam_qos, disp_qos, i2c_qos, hpg_qos, dvfs_ctrl->cur_hmp_bst);
#else
	info("[RSC:%d]: New QoS [INT(%d), MIF(%d), CAM(%d), DISP(%d), I2C(%d), HPG(%d, %d)]\n",
			device ? device->instance : 0, int_qos, mif_qos,
			cam_qos, disp_qos, i2c_qos, hpg_qos, dvfs_ctrl->cur_hmp_bst);
#endif
exit:
	return ret;
}

void fimc_is_dual_mode_update(struct fimc_is_device_ischain *device,
	struct fimc_is_group *group,
	struct fimc_is_frame *frame)
{
	struct fimc_is_core *core = (struct fimc_is_core *)device->interface->core;
	struct fimc_is_dual_info *dual_info = &core->dual_info;
	struct fimc_is_device_sensor *sensor = device->sensor;
	struct fimc_is_resourcemgr *resourcemgr;
	int i, streaming_cnt = 0;

	resourcemgr = sensor->resourcemgr;

	if (group->head->device_type != FIMC_IS_DEVICE_SENSOR)
		return;

	dual_info->max_fps[sensor->position] = frame->shot->ctl.aa.aeTargetFpsRange[1];

	for (i = 0; i < SENSOR_POSITION_MAX; i++) {
		if (dual_info->max_fps[i] >= 24)
			streaming_cnt++;
	}
	resourcemgr->streaming_cnt = streaming_cnt;

	/* Continue if wide and tele/s-wide complete fimc_is_sensor_s_input(). */
	if (!(test_bit(SENSOR_POSITION_REAR, &core->sensor_map) &&
		(test_bit(SENSOR_POSITION_REAR2, &core->sensor_map) ||
		 test_bit(SENSOR_POSITION_REAR3, &core->sensor_map))))
		return;


	/* Update max fps of dual sensor device with reference to shot meta. */
	switch (sensor->position) {
	case SENSOR_POSITION_REAR:
		dual_info->max_fps_master = frame->shot->ctl.aa.aeTargetFpsRange[1];
		break;
	case SENSOR_POSITION_REAR2:
	case SENSOR_POSITION_REAR3:
		dual_info->max_fps_slave = frame->shot->ctl.aa.aeTargetFpsRange[1];
		break;
	default:
		return;
	}

	/*
	 * bypass - master_max_fps : 30fps, slave_max_fps : 0fps (sensor standby)
	 * sync - master_max_fps : 30fps, slave_max_fps : 30fps (fusion)
	 * switch - master_max_fps : 5ps, slave_max_fps : 30fps (post standby)
	 * nothing - invalid mode
	 */
	if (dual_info->max_fps_master >= 24 && dual_info->max_fps_slave == 0)
		dual_info->mode = FIMC_IS_DUAL_MODE_BYPASS;
	else if (dual_info->max_fps_master >= 24 && dual_info->max_fps_slave >= 24)
		dual_info->mode = FIMC_IS_DUAL_MODE_SYNC;
	else if (dual_info->max_fps_master <= 5 && dual_info->max_fps_slave >= 24)
		dual_info->mode = FIMC_IS_DUAL_MODE_SWITCH;
	else
		dual_info->mode = FIMC_IS_DUAL_MODE_NOTHING;
}

void fimc_is_dual_dvfs_update(struct fimc_is_device_ischain *device,
	struct fimc_is_group *group,
	struct fimc_is_frame *frame)
{
	struct fimc_is_core *core = (struct fimc_is_core *)device->interface->core;
	struct fimc_is_dual_info *dual_info = &core->dual_info;
	struct fimc_is_resourcemgr *resourcemgr = device->resourcemgr;
	struct fimc_is_dvfs_scenario_ctrl *static_ctrl
				= resourcemgr->dvfs_ctrl.static_ctrl;
	int scenario_id, pre_scenario_id;

	/* Continue if wide and tele/s-wide complete fimc_is_sensor_s_input(). */
	if (!(test_bit(SENSOR_POSITION_REAR, &core->sensor_map) &&
		(test_bit(SENSOR_POSITION_REAR2, &core->sensor_map) ||
		 test_bit(SENSOR_POSITION_REAR3, &core->sensor_map))))
		return;

	if (group->head->device_type != FIMC_IS_DEVICE_SENSOR)
		return;

	/*
	 * tick_count : Add dvfs update margin for dvfs update when mode is changed
	 * from fusion(sync) to standby(bypass, switch) because H/W does not apply
	 * immediately even if mode is dropped from hal.
	 * tick_count == 0 : dvfs update
	 * tick_count > 0 : tick count decrease
	 * tick count < 0 : ignore
	 */
	if (dual_info->tick_count >= 0)
		dual_info->tick_count--;

	/* If pre_mode and mode are different, tick_count setup. */
	if (dual_info->pre_mode != dual_info->mode) {

		/* If current state is FIMC_IS_DUAL_NOTHING, do not do DVFS update. */
		if (dual_info->mode == FIMC_IS_DUAL_MODE_NOTHING)
			dual_info->tick_count = -1;

		switch (dual_info->pre_mode) {
		case FIMC_IS_DUAL_MODE_BYPASS:
		case FIMC_IS_DUAL_MODE_SWITCH:
		case FIMC_IS_DUAL_MODE_NOTHING:
			dual_info->tick_count = 0;
			break;
		case FIMC_IS_DUAL_MODE_SYNC:
			dual_info->tick_count = FIMC_IS_DVFS_DUAL_TICK;
			break;
		default:
			err("invalid dual mode %d -> %d\n", dual_info->pre_mode, dual_info->mode);
			dual_info->tick_count = -1;
			dual_info->pre_mode = FIMC_IS_DUAL_MODE_NOTHING;
			dual_info->mode = FIMC_IS_DUAL_MODE_NOTHING;
			break;
		}
	}

	/* Only if tick_count is 0 dvfs update. */
	if (dual_info->tick_count == 0) {
		pre_scenario_id = static_ctrl->cur_scenario_id;
		scenario_id = fimc_is_dvfs_sel_static(device);
		if (scenario_id >= 0 && scenario_id != pre_scenario_id) {
			struct fimc_is_dvfs_scenario_ctrl *static_ctrl = resourcemgr->dvfs_ctrl.static_ctrl;

			mgrinfo("tbl[%d] dual static scenario(%d)-[%s]\n", device, group, frame,
				resourcemgr->dvfs_ctrl.dvfs_table_idx,
				static_ctrl->cur_scenario_id,
				static_ctrl->scenarios[static_ctrl->cur_scenario_idx].scenario_nm);
			fimc_is_set_dvfs((struct fimc_is_core *)device->interface->core, device, scenario_id);
		} else {
			mgrinfo("tbl[%d] dual DVFS update skip %d -> %d\n", device, group, frame,
				resourcemgr->dvfs_ctrl.dvfs_table_idx,
				pre_scenario_id, scenario_id);
		}
	}

	/* Update current mode to pre_mode. */
	dual_info->pre_mode = dual_info->mode;
}

unsigned int fimc_is_get_bit_count(unsigned long bits)
{
	unsigned int count = 0;

	while (bits) {
		bits &= (bits - 1);
		count++;
	}

	return count;
}
#endif
