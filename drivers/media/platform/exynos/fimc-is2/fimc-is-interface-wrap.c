/*
 * Samsung Exynos SoC series FIMC-IS2 driver
 *
 * exynos fimc-is2 device interface functions
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-interface-wrap.h"
#include "fimc-is-interface-library.h"
#include "fimc-is-param.h"

int fimc_is_itf_s_param_wrap(struct fimc_is_device_ischain *device,
	u32 lindex, u32 hindex, u32 indexes)
{
	struct fimc_is_hardware *hardware = NULL;
	u32 instance = 0;
	int ret = 0;

	dbg_hw(2, "%s\n", __func__);

	hardware = device->hardware;
	instance = device->instance;

	ret = fimc_is_hardware_set_param(hardware, instance, device->is_region,
		lindex, hindex, hardware->hw_map[instance]);
	if (ret) {
		merr("fimc_is_hardware_set_param is fail(%d)", device, ret);
		return ret;
	}

	return ret;
}

int fimc_is_itf_a_param_wrap(struct fimc_is_device_ischain *device, u32 group)
{
	struct fimc_is_hardware *hardware = NULL;
	u32 instance = 0;
	int ret = 0;

	dbg_hw(2, "%s\n", __func__);

	hardware = device->hardware;
	instance = device->instance;

#if !defined(DISABLE_SETFILE)
	ret = fimc_is_hardware_apply_setfile(hardware, instance,
				device->setfile & FIMC_IS_SETFILE_MASK,
				hardware->hw_map[instance]);
	if (ret) {
		merr("fimc_is_hardware_apply_setfile is fail(%d)", device, ret);
		return ret;
	}
#endif
	return ret;
}

int fimc_is_itf_f_param_wrap(struct fimc_is_device_ischain *device, u32 group)
{
	struct fimc_is_hardware *hardware= NULL;
	u32 instance = 0;
	int ret = 0;

	dbg_hw(2, "%s\n", __func__);

	hardware = device->hardware;
	instance = device->instance;

	return ret;
}

int fimc_is_itf_enum_wrap(struct fimc_is_device_ischain *device)
{
	int ret = 0;

	dbg_hw(2, "%s\n", __func__);

	return ret;
}

void fimc_is_itf_storefirm_wrap(struct fimc_is_device_ischain *device)
{
	dbg_hw(2, "%s\n", __func__);

	return;
}

void fimc_is_itf_restorefirm_wrap(struct fimc_is_device_ischain *device)
{
	dbg_hw(2, "%s\n", __func__);

	return;
}

int fimc_is_itf_set_fwboot_wrap(struct fimc_is_device_ischain *device, u32 val)
{
	int ret = 0;

	dbg_hw(2, "%s\n", __func__);

	return ret;
}

int fimc_is_itf_open_wrap(struct fimc_is_device_ischain *device, u32 module_id,
	u32 flag, u32 offset_path)
{
	struct fimc_is_hardware *hardware;
	struct fimc_is_path_info *path;
	struct fimc_is_group *group;
	struct is_region *region;
	struct fimc_is_device_sensor *sensor;
	u32 instance = 0;
	u32 hw_id = 0;
	u32 group_id = -1;
	u32 group_slot, group_slot_c;
	int ret = 0, ret_c = 0;
	int hw_list[GROUP_HW_MAX];
	int hw_index;
	int hw_maxnum = 0;
	u32 rsccount;

	info_hw("%s: offset_path(0x%8x) flag(%d) sen(%d)\n", __func__, offset_path, flag, module_id);

	sensor   = device->sensor;
	instance = device->instance;
	hardware = device->hardware;
	path = (struct fimc_is_path_info *)&device->is_region->shared[offset_path];
	rsccount = atomic_read(&hardware->rsccount);

	if (rsccount == 0) {
		ret = fimc_is_init_ddk_thread();
		if (ret) {
			err("failed to create threads for DDK, ret %d", ret);
			return ret;
		}
	}

	region = device->is_region;
	region->shared[MAX_SHARED_COUNT-1] = MAGIC_NUMBER;

	for (hw_id = 0; hw_id < DEV_HW_END; hw_id++)
		clear_bit(hw_id, &hardware->hw_map[instance]);

	for (group_slot = GROUP_SLOT_PAF; group_slot < GROUP_SLOT_MAX; group_slot++) {
		switch (group_slot) {
		case GROUP_SLOT_PAF:
			group = &device->group_paf;
			break;
		case GROUP_SLOT_3AA:
			group = &device->group_3aa;
			break;
		case GROUP_SLOT_ISP:
			group = &device->group_isp;
			break;
		case GROUP_SLOT_DIS:
			group = &device->group_dis;
			break;
		case GROUP_SLOT_DCP:
			group = &device->group_dcp;
			break;
		case GROUP_SLOT_MCS:
			group = &device->group_mcs;
			break;
		case GROUP_SLOT_VRA:
			group = &device->group_vra;
			break;
		default:
			continue;
			break;
		}

		group_id = path->group[group_slot];
		dbg_hw(1, "itf_open_wrap: group[SLOT_%d]=[%x]\n", group_slot, group_id);
		hw_maxnum = fimc_is_get_hw_list(group_id, hw_list);
		for (hw_index = 0; hw_index < hw_maxnum; hw_index++) {
			hw_id = hw_list[hw_index];
			ret = fimc_is_hardware_open(hardware, hw_id, group, instance,
					flag, module_id);
			if (ret) {
				merr("fimc_is_hardware_open(%d) is fail", device, hw_id);
				goto hardware_close;
			}
		}
	}

	hardware->sensor_position[instance] = sensor->position;
	atomic_inc(&hardware->rsccount);

	info("%s: done: hw_map[0x%lx][RSC:%d][%d]\n", __func__,
		hardware->hw_map[instance], atomic_read(&hardware->rsccount),
		hardware->sensor_position[instance]);

	return ret;

hardware_close:
	group_slot_c = group_slot;

	for (group_slot = GROUP_SLOT_3AA; group_slot <= group_slot_c; group_slot++) {
		group_id = path->group[group_slot];
		hw_maxnum = fimc_is_get_hw_list(group_id, hw_list);
		for (hw_index = 0; hw_index < hw_maxnum; hw_index++) {
			hw_id = hw_list[hw_index];
			info_hw("[%d][ID:%d]itf_close_wrap: call hardware_close(), group_id(%d), ret(%d)\n",
				instance, hw_id, group_id, ret);
			ret_c = fimc_is_hardware_close(hardware, hw_id, instance);
			if (ret_c)
				merr("fimc_is_hardware_close(%d) is fail", device, hw_id);
		}
	}

	return ret;
}

int fimc_is_itf_close_wrap(struct fimc_is_device_ischain *device)
{
	struct fimc_is_hardware *hardware;
	struct fimc_is_path_info *path;
	u32 offset_path = 0;
	u32 instance = 0;
	u32 hw_id = 0;
	u32 group_id = -1;
	u32 group_slot;
	int ret = 0;
	int hw_list[GROUP_HW_MAX];
	int hw_index;
	int hw_maxnum = 0;
	u32 rsccount;

	dbg_hw(2, "%s\n", __func__);

	hardware = device->hardware;
	instance = device->instance;
	offset_path = (sizeof(struct sensor_open_extended) / 4) + 1;
	path = (struct fimc_is_path_info *)&device->is_region->shared[offset_path];
	rsccount = atomic_read(&hardware->rsccount);

	if (rsccount == 1)
		fimc_is_flush_ddk_thread();

#if !defined(DISABLE_SETFILE)
	ret = fimc_is_hardware_delete_setfile(hardware, instance,
			hardware->hw_map[instance]);
	if (ret) {
		merr("fimc_is_hardware_delete_setfile is fail(%d)", device, ret);
			return ret;
	}
#endif

	for (group_slot = GROUP_SLOT_PAF; group_slot < GROUP_SLOT_MAX; group_slot++) {
		group_id = path->group[group_slot];
		dbg_hw(1, "itf_close_wrap: group[SLOT_%d]=[%x]\n", group_slot, group_id);
		hw_maxnum = fimc_is_get_hw_list(group_id, hw_list);
		for (hw_index = 0; hw_index < hw_maxnum; hw_index++) {
			hw_id = hw_list[hw_index];
			ret = fimc_is_hardware_close(hardware, hw_id, instance);
			if (ret)
				merr("fimc_is_hardware_close(%d) is fail", device, hw_id);
		}
	}

	atomic_dec(&hardware->rsccount);

	if (rsccount == 1)
		check_lib_memory_leak();

	info("%s: done: hw_map[0x%lx][RSC:%d]\n", __func__,
		hardware->hw_map[instance], atomic_read(&hardware->rsccount));

	return ret;
}

int fimc_is_itf_setaddr_wrap(struct fimc_is_interface *itf,
	struct fimc_is_device_ischain *device, ulong *setfile_addr)
{
	int ret = 0;

	dbg_hw(2, "%s\n", __func__);

	*setfile_addr = device->resourcemgr->minfo.kvaddr_setfile;

	return ret;
}

int fimc_is_itf_setfile_wrap(struct fimc_is_interface *itf, ulong setfile_addr,
	struct fimc_is_device_ischain *device)
{
	struct fimc_is_hardware *hardware;
	u32 instance = 0;
	int ret = 0;

	dbg_hw(2, "%s\n", __func__);

	hardware = device->hardware;
	instance = device->instance;

#if !defined(DISABLE_SETFILE)
	ret = fimc_is_hardware_load_setfile(hardware, setfile_addr, instance,
				hardware->hw_map[instance]);
	if (ret) {
		merr("fimc_is_hardware_load_setfile is fail(%d)", device, ret);
		return ret;
	}
#endif

	return ret;
}

int fimc_is_itf_map_wrap(struct fimc_is_device_ischain *device,
	u32 group, u32 shot_addr, u32 shot_size)
{
	int ret = 0;

	dbg_hw(2, "%s\n", __func__);

	return ret;
}

int fimc_is_itf_unmap_wrap(struct fimc_is_device_ischain *device, u32 group)
{
	int ret = 0;

	dbg_hw(2, "%s\n", __func__);

	return ret;
}

int fimc_is_itf_stream_on_wrap(struct fimc_is_device_ischain *device)
{
	struct fimc_is_hardware *hardware;
	u32 instance;
	ulong hw_map;
	int ret = 0;

	dbg_hw(2, "%s\n", __func__);

	hardware = device->hardware;
	instance = device->instance;
	hw_map = hardware->hw_map[instance];

	ret = fimc_is_hardware_sensor_start(hardware, instance, hw_map);
	if (ret) {
		merr("fimc_is_hardware_stream_on is fail(%d)", device, ret);
		return ret;
	}

	return ret;
}

int fimc_is_itf_stream_off_wrap(struct fimc_is_device_ischain *device)
{
	struct fimc_is_hardware *hardware;
	u32 instance;
	ulong hw_map;
	int ret = 0;

	dbg_hw(2, "%s\n", __func__);

	hardware = device->hardware;
	instance = device->instance;
	hw_map = hardware->hw_map[instance];

	ret = fimc_is_hardware_sensor_stop(hardware, instance, hw_map);
	if (ret) {
		merr("fimc_is_hardware_stream_off is fail(%d)", device, ret);
		return ret;
	}

	return ret;
}

int fimc_is_itf_process_on_wrap(struct fimc_is_device_ischain *device, u32 group)
{
	struct fimc_is_hardware *hardware;
	u32 instance = 0;
	u32 group_id;
	int ret = 0;

	hardware = device->hardware;
	instance = device->instance;

	minfo_hw("itf_process_on_wrap: [G:0x%x]\n", instance, group);

	for (group_id = GROUP_ID_3AA0; group_id < GROUP_ID_MAX; group_id++) {
		if (((group) & GROUP_ID(group_id)) &&
				(GET_DEVICE_TYPE_BY_GRP(group_id) == FIMC_IS_DEVICE_ISCHAIN)) {
			ret = fimc_is_hardware_process_start(hardware, instance, group_id);
			if (ret) {
				merr("fimc_is_hardware_process_start is fail(%d)", device, ret);
				return ret;
			}
		}
	}

	return ret;
}

int fimc_is_itf_process_off_wrap(struct fimc_is_device_ischain *device, u32 group,
	u32 fstop)
{
	struct fimc_is_hardware *hardware;
	u32 instance = 0;
	u32 group_id;
	int ret = 0;

	hardware = device->hardware;
	instance = device->instance;

	minfo_hw("itf_process_off_wrap: [G:0x%x](%d)\n", instance, group, fstop);

	for (group_id = 0; group_id < GROUP_ID_MAX; group_id++) {
		if ((group) & GROUP_ID(group_id)) {
			if (GET_DEVICE_TYPE_BY_GRP(group_id) == FIMC_IS_DEVICE_ISCHAIN) {
				fimc_is_hardware_process_stop(hardware, instance, group_id, fstop);
			} else {
				/* in case of sensor group */
				if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
					fimc_is_sensor_group_force_stop(device->sensor, group_id);
			}
		}
	}

	return ret;
}

void fimc_is_itf_sudden_stop_wrap(struct fimc_is_device_ischain *device, u32 instance)
{
	int ret = 0;
	struct fimc_is_device_sensor *sensor;

	if (!device) {
		mwarn_hw("%s: device(null)\n", instance, __func__);
		return;
	}

	sensor = device->sensor;
	if (!sensor) {
		mwarn_hw("%s: sensor(null)\n", instance, __func__);
		return;
	}

	if (test_bit(FIMC_IS_SENSOR_FRONT_START, &sensor->state)) {
		minfo_hw("%s: sudden close, call sensor_front_stop()\n", instance, __func__);

		ret = fimc_is_sensor_front_stop(sensor);
		if (ret)
			merr("fimc_is_sensor_front_stop is fail(%d)", sensor, ret);
	}

	return;
}

int fimc_is_itf_power_down_wrap(struct fimc_is_interface *interface, u32 instance)
{
	int ret = 0;
	struct fimc_is_core *core;
#ifdef USE_DDK_SHUT_DOWN_FUNC
	void *data = NULL;
#endif

	dbg_hw(2, "%s\n", __func__);

	core = (struct fimc_is_core *)interface->core;
	if (!core) {
		mwarn_hw("%s: core(null)\n", instance, __func__);
		return ret;
	}

	fimc_is_itf_sudden_stop_wrap(&core->ischain[instance], instance);

#ifdef USE_DDK_SHUT_DOWN_FUNC
#ifdef ENABLE_FPSIMD_FOR_USER
	fpsimd_get();
	((ddk_shut_down_func_t)DDK_SHUT_DOWN_FUNC_ADDR)(data);
	fpsimd_put();
#else
	((ddk_shut_down_func_t)DDK_SHUT_DOWN_FUNC_ADDR)(data);
#endif
#endif

	return ret;
}

int fimc_is_itf_sys_ctl_wrap(struct fimc_is_device_ischain *device,
	int cmd, int val)
{
	int ret = 0;

	dbg_hw(2, "%s\n", __func__);

	return ret;
}

int fimc_is_itf_sensor_mode_wrap(struct fimc_is_device_ischain *device,
	struct fimc_is_sensor_cfg *cfg)
{
#ifdef USE_RTA_BINARY
	void *data = NULL;

	dbg_hw(2, "%s\n", __func__);

	if (cfg && cfg->mode == SENSOR_MODE_DEINIT) {
		info_hw("%s: call RTA_SHUT_DOWN\n", __func__);
#ifdef ENABLE_FPSIMD_FOR_USER
		fpsimd_get();
		((rta_shut_down_func_t)RTA_SHUT_DOWN_FUNC_ADDR)(data);
		fpsimd_put();
#else
		((rta_shut_down_func_t)RTA_SHUT_DOWN_FUNC_ADDR)(data);
#endif
	}
#else
	dbg_hw(2, "%s\n", __func__);
#endif

	return 0;
}

void fimc_is_itf_fwboot_init(struct fimc_is_interface *this)
{
	clear_bit(IS_IF_LAUNCH_FIRST, &this->launch_state);
	clear_bit(IS_IF_LAUNCH_SUCCESS, &this->launch_state);
	clear_bit(IS_IF_RESUME, &this->fw_boot);
	clear_bit(IS_IF_SUSPEND, &this->fw_boot);
	this->fw_boot_mode = COLD_BOOT;
}

bool check_setfile_change(struct fimc_is_group *group_leader,
	struct fimc_is_group *group, struct fimc_is_hardware *hardware,
	u32 instance, u32 scenario)
{
	struct fimc_is_group *group_ischain = group;
	struct fimc_is_hw_ip *hw_ip = NULL;
	int hw_slot = -1;
	u32 hw_id = DEV_HW_END;
	enum exynos_sensor_position sensor_position;

	if (group_leader->id != group->id)
		return false;

	if ((group->device_type == FIMC_IS_DEVICE_SENSOR)
		&& (group->next)) {
		group_ischain = group->next;
	}

	hw_id = get_hw_id_from_group(group_ischain->id);
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		merr_hw("[G:0x%x]: invalid slot (%d,%d)", instance,
			GROUP_ID(group->id), hw_id, hw_slot);
		return false;
	}

	hw_ip = &hardware->hw_ip[hw_slot];
	if (!hw_ip) {
		merr_hw("[G:0x%x]: hw_ip(null) (%d,%d)", instance,
			GROUP_ID(group->id), hw_id, hw_slot);
		return false;
	}

	sensor_position = hardware->sensor_position[instance];
	/* If the 3AA hardware is shared between front preview and reprocessing instance, (e.g. PIP)
	   apply_setfile funciton needs to be called for sensor control. There is two options to check
	   this, one is to check instance change and the other is to check scenario(setfile_index) change.
	   The scenario(setfile_index) is different front preview instance and reprocessing instance.
	   So, second option is more efficient way to support PIP scenario.
	 */
	if (scenario != hw_ip->applied_scenario) {
		msinfo_hw("[G:0x%x,0x%x,0x%x]%s: scenario(%d->%d), instance(%d->%d)\n", instance, hw_ip,
			GROUP_ID(group_leader->id), GROUP_ID(group_ischain->id),
			GROUP_ID(group->id), __func__,
			hw_ip->applied_scenario, scenario,
			atomic_read(&hw_ip->instance), instance);
		return true;
	}

	return false;
}

int fimc_is_itf_shot_wrap(struct fimc_is_device_ischain *device,
	struct fimc_is_group *group, struct fimc_is_frame *frame)
{
	struct fimc_is_hardware *hardware;
	struct fimc_is_interface *itf;
	struct fimc_is_group *group_leader;
	u32 instance = 0;
	int ret = 0;
	ulong flags;
	u32 scenario;
	bool apply_flag = false;

	scenario = device->setfile & FIMC_IS_SETFILE_MASK;
	hardware = device->hardware;
	instance = device->instance;
	itf = device->interface;
	group_leader = get_ischain_leader_group(device);

	apply_flag = check_setfile_change(group_leader, group, hardware, instance, scenario);

	if (!atomic_read(&group_leader->scount) || apply_flag) {
		mdbg_hw(1, "[G:0x%x]%s: call apply_setfile()\n",
			instance, GROUP_ID(group->id), __func__);
#if !defined(DISABLE_SETFILE)
		ret = fimc_is_hardware_apply_setfile(hardware, instance,
					scenario, hardware->hw_map[instance]);
		if (ret) {
			merr("fimc_is_hardware_apply_setfile is fail(%d)", device, ret);
			return ret;
		}
#endif
	}

	ret = fimc_is_hardware_grp_shot(hardware, instance, group, frame,
					hardware->hw_map[instance]);
	if (ret) {
		merr("fimc_is_hardware_grp_shot is fail(%d)", device, ret);
		return ret;
	}

	spin_lock_irqsave(&itf->shot_check_lock, flags);
	atomic_set(&itf->shot_check[instance], 1);
	spin_unlock_irqrestore(&itf->shot_check_lock, flags);

	return ret;
}

void fimc_is_itf_sfr_dump_wrap(struct fimc_is_device_ischain *device, u32 group, bool flag_print_log)
{
	u32 hw_maxnum;
	u32 hw_id;
	int hw_list[GROUP_HW_MAX];
	struct fimc_is_hardware *hardware;

	hardware = device->hardware;

	hw_maxnum = fimc_is_get_hw_list(group, hw_list);
	if (hw_maxnum > 0) {
		hw_id = hw_list[hw_maxnum - 1];
		fimc_is_hardware_sfr_dump(hardware, hw_id, flag_print_log);
	}
}
