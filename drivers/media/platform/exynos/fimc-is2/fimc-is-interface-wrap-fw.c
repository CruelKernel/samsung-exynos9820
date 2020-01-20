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

int fimc_is_itf_s_param_wrap(struct fimc_is_device_ischain *device,
	u32 lindex, u32 hindex, u32 indexes)
{
	int ret = 0;

	if (lindex || hindex) {
		ret = fimc_is_hw_s_param(device->interface, device->instance,
			lindex, hindex, indexes);
	}

	return ret;
}

int fimc_is_itf_a_param_wrap(struct fimc_is_device_ischain *device, u32 group)
{
	int ret = 0;
	u32 setfile;

	setfile = (device->setfile & FIMC_IS_SETFILE_MASK);

	ret = fimc_is_hw_a_param(device->interface,
		device->instance, group, setfile);

	return ret;
}

int fimc_is_itf_f_param_wrap(struct fimc_is_device_ischain *device, u32 group)
{
	int ret = 0;
	u32 setfile;

	setfile = (device->setfile & FIMC_IS_SETFILE_MASK);

	ret = fimc_is_hw_a_param(device->interface,
		device->instance, (group & GROUP_ID_PARM_MASK), setfile);

	return ret;
}

int fimc_is_itf_enum_wrap(struct fimc_is_device_ischain *device)
{
	int ret = 0;

	mdbgd_ischain("%s()\n", device, __func__);

	ret = fimc_is_hw_enum(device->interface);
	if (ret) {
		merr("fimc_is_hw_enum is fail(%d)", device, ret);
		CALL_POPS(device, print_clk, device->pdev);
	}

	return ret;
}

void fimc_is_itf_storefirm_wrap(struct fimc_is_device_ischain *device)
{
	mdbgd_ischain("%s()\n", device, __func__);

	fimc_is_storefirm(device->interface);
}

void fimc_is_itf_restorefirm_wrap(struct fimc_is_device_ischain *device)
{
	mdbgd_ischain("%s()\n", device, __func__);

	fimc_is_restorefirm(device->interface);
}

int fimc_is_itf_set_fwboot_wrap(struct fimc_is_device_ischain *device, u32 val)
{
	int ret = 0;

	mdbgd_ischain("%s()\n", device, __func__);

	ret = fimc_is_set_fwboot(device->interface, val);
	if (ret) {
		merr("fimc_is_set_fwboot is fail(%d)", device, ret);
	}

	return ret;
}

int fimc_is_itf_open_wrap(struct fimc_is_device_ischain *device, u32 module_id,
	u32 flag, u32 offset_path)
{
	int ret = 0;

	ret = fimc_is_hw_open(device->interface,
		device->instance,
		module_id,
		device->dvaddr_shared,
		device->dvaddr_shared + (offset_path * 4),
		flag,
		&device->margin_width,
		&device->margin_height);
	if (ret) {
		merr("fimc_is_hw_open is fail", device);
		CALL_POPS(device, print_clk, device->pdev);
		fimc_is_sensor_gpio_dbg(device->sensor);
		ret = -EINVAL;
	}

	return ret;
}

int fimc_is_itf_close_wrap(struct fimc_is_device_ischain *device)
{
	int ret = 0;

	ret = fimc_is_hw_close(device->interface, device->instance);
	if (ret)
		merr("fimc_is_hw_close is fail", device);

	return ret;
}

int fimc_is_itf_setaddr_wrap(struct fimc_is_interface *itf,
	struct fimc_is_device_ischain *device, ulong *setfile_addr)
{
	int ret = 0;

	mdbgd_ischain("%s\n", device, __func__);

	ret = fimc_is_hw_saddr(itf, device->instance, (u32 *)setfile_addr);
	if (ret)
		merr("fimc_is_hw_saddr is fail(%d)", device, ret);

	return ret;
}

int fimc_is_itf_setfile_wrap(struct fimc_is_interface *itf, ulong setfile_addr,
	struct fimc_is_device_ischain *device)
{
	int ret = 0;

	mdbgd_ischain("%s\n", device, __func__);

	ret = fimc_is_hw_setfile(itf, device->instance);
	if (ret)
		merr("fimc_is_hw_setfile is fail(%d)", device, ret);

	return ret;
}

int fimc_is_itf_map_wrap(struct fimc_is_device_ischain *device,
	u32 group, u32 shot_addr, u32 shot_size)
{
	int ret = 0;

	mdbgd_ischain("%s()\n", device, __func__);

	ret = fimc_is_hw_map(device->interface, device->instance,
			group, shot_addr, shot_size);
	if (ret)
		merr("fimc_is_hw_map is fail(%d)", device, ret);

	return ret;
}

int fimc_is_itf_unmap_wrap(struct fimc_is_device_ischain *device, u32 group)
{
	int ret = 0;

	mdbgd_ischain("%s()\n", device, __func__);

	ret = fimc_is_hw_unmap(device->interface, device->instance, group);
	if (ret)
		merr("fimc_is_hw_unmap is fail(%d)", device, ret);

	return ret;
}

int fimc_is_itf_stream_on_wrap(struct fimc_is_device_ischain *device)
{
	int ret = 0;

	mdbgd_ischain("%s()\n", device, __func__);

	ret = fimc_is_hw_stream_on(device->interface, device->instance);
	if (ret) {
		merr("fimc_is_hw_stream_on is fail(%d)", device, ret);
		CALL_POPS(device, print_clk, device->pdev);
		fimc_is_sensor_gpio_dbg(device->sensor);
	}

	return ret;
}

int fimc_is_itf_stream_off_wrap(struct fimc_is_device_ischain *device)
{
	int ret = 0;

	mdbgd_ischain("%s()\n", device, __func__);

	ret = fimc_is_hw_stream_off(device->interface, device->instance);
	if (ret) {
		merr("fimc_is_hw_stream_off is fail(%d)", device, ret);
		CALL_POPS(device, print_clk, device->pdev);

		if (!device->sensor) {
			merr("sensor is NULL", device);
			ret = -ENODEV;
			goto p_err;
		}
		fimc_is_sensor_gpio_dbg(device->sensor);
	}

p_err:
	return ret;
}

int fimc_is_itf_process_on_wrap(struct fimc_is_device_ischain *device, u32 group)
{
	int ret = 0;

	ret = fimc_is_hw_process_on(device->interface, device->instance, group);

	return ret;
}

int fimc_is_itf_process_off_wrap(struct fimc_is_device_ischain *device, u32 group,
	u32 fstop)
{
	int ret = 0;

	ret = fimc_is_hw_process_off(device->interface, device->instance,
					group, fstop);

	return ret;
}

void fimc_is_itf_sudden_stop_wrap(struct fimc_is_device_ischain *device, u32 instance)
{
	return;
}

int fimc_is_itf_power_down_wrap(struct fimc_is_interface *interface, u32 instance)
{
	int ret = 0;

	ret = fimc_is_hw_power_down(interface, instance);

	return ret;
}

int fimc_is_itf_sys_ctl_wrap(struct fimc_is_device_ischain *device,
	int cmd, int val)
{
	int ret = 0;

	ret = fimc_is_hw_sys_ctl(device->interface, device->instance,
				cmd, val);

	return ret;
}

int fimc_is_itf_sensor_mode_wrap(struct fimc_is_device_ischain *ischain,
	struct fimc_is_sensor_cfg *cfg)
{
	int ret = 0;

	ret = fimc_is_hw_sensor_mode(ischain->interface, ischain->instance,
			((cfg->mode << 16) | (ischain->module & 0xFFFF)));
	if (ret)
		merr("fimc_is_hw_sensor_mode is fail(%d)", ischain, ret);

	return ret;
}

int fimc_is_itf_shot_wrap(struct fimc_is_device_ischain *device,
	struct fimc_is_group *group, struct fimc_is_frame *frame)
{
	int ret = 0;

	ret = fimc_is_hw_shot_nblk(device->interface,
		device->instance,
		GROUP_ID(group->id),
		frame->dvaddr_shot,
		frame->fcount,
		frame->rcount);

	return ret;
}
