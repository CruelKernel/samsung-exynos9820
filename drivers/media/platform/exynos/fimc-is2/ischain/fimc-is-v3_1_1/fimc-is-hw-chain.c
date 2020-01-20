/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include "fimc-is-config.h"
#include "fimc-is-param.h"
#include "fimc-is-type.h"
#include "fimc-is-regs.h"
#include "fimc-is-groupmgr.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-ischain.h"

int fimc_is_hw_group_cfg(void *group_data)
{
	int ret = 0;
	struct fimc_is_group *group;
	struct fimc_is_device_ischain *device;

	FIMC_BUG(!group_data);

	group = group_data;
	device = group->device;

	if (!device) {
		err("device is NULL");
		BUG();
	}

	switch (group->slot) {
	case GROUP_SLOT_3AA:
		group->subdev[ENTRY_3AA] = NULL;
		group->subdev[ENTRY_3AC] = &device->txc;
		group->subdev[ENTRY_3AP] = &device->txp;
		group->subdev[ENTRY_ISP] = NULL;
		group->subdev[ENTRY_IXC] = NULL;
		group->subdev[ENTRY_IXP] = NULL;
		group->subdev[ENTRY_DRC] = NULL;
		group->subdev[ENTRY_DIS] = NULL;
		group->subdev[ENTRY_DXC] = NULL;
		group->subdev[ENTRY_ODC] = NULL;
		group->subdev[ENTRY_DNR] = NULL;
		group->subdev[ENTRY_SCC] = NULL;
		group->subdev[ENTRY_SCP] = NULL;
		group->subdev[ENTRY_VRA] = NULL;

		INIT_LIST_HEAD(&group->subdev_list);
		list_add_tail(&device->txc.list, &group->subdev_list);
		list_add_tail(&device->txp.list, &group->subdev_list);

		device->txc.param_dma_ot = PARAM_3AA_VDMA2_OUTPUT;
		device->txp.param_dma_ot = PARAM_3AA_VDMA4_OUTPUT;
		break;
	case GROUP_SLOT_ISP:
		group->subdev[ENTRY_3AA] = NULL;
		group->subdev[ENTRY_3AC] = NULL;
		group->subdev[ENTRY_3AP] = NULL;
		group->subdev[ENTRY_ISP] = NULL;
		group->subdev[ENTRY_IXC] = &device->ixc;
		group->subdev[ENTRY_IXP] = &device->ixp;
		group->subdev[ENTRY_DRC] = NULL;
		group->subdev[ENTRY_DIS] = NULL;
		group->subdev[ENTRY_DXC] = NULL;
		group->subdev[ENTRY_ODC] = NULL;
		group->subdev[ENTRY_DNR] = NULL;
		group->subdev[ENTRY_SCC] = NULL;
		group->subdev[ENTRY_SCP] = &device->scp;
		group->subdev[ENTRY_VRA] = NULL;

		INIT_LIST_HEAD(&group->subdev_list);
		list_add_tail(&device->ixc.list, &group->subdev_list);
		list_add_tail(&device->ixp.list, &group->subdev_list);
		list_add_tail(&device->scp.list, &group->subdev_list);
		break;
	case GROUP_SLOT_DIS:
		group->subdev[ENTRY_3AA] = NULL;
		group->subdev[ENTRY_3AC] = NULL;
		group->subdev[ENTRY_3AP] = NULL;
		group->subdev[ENTRY_ISP] = NULL;
		group->subdev[ENTRY_IXC] = NULL;
		group->subdev[ENTRY_IXP] = NULL;
		group->subdev[ENTRY_DRC] = NULL;
		group->subdev[ENTRY_DIS] = NULL;
		group->subdev[ENTRY_DXC] = NULL;
		group->subdev[ENTRY_ODC] = NULL;
		group->subdev[ENTRY_DNR] = NULL;
		group->subdev[ENTRY_SCC] = NULL;
		group->subdev[ENTRY_SCP] = NULL;
		group->subdev[ENTRY_VRA] = NULL;

		INIT_LIST_HEAD(&group->subdev_list);
		break;
	case GROUP_SLOT_MCS:
		group->subdev[ENTRY_3AA] = NULL;
		group->subdev[ENTRY_3AC] = NULL;
		group->subdev[ENTRY_3AP] = NULL;
		group->subdev[ENTRY_ISP] = NULL;
		group->subdev[ENTRY_IXC] = NULL;
		group->subdev[ENTRY_IXP] = NULL;
		group->subdev[ENTRY_DRC] = NULL;
		group->subdev[ENTRY_DIS] = NULL;
		group->subdev[ENTRY_DXC] = NULL;
		group->subdev[ENTRY_ODC] = NULL;
		group->subdev[ENTRY_DNR] = NULL;
		group->subdev[ENTRY_SCC] = NULL;
		group->subdev[ENTRY_SCP] = NULL;
		group->subdev[ENTRY_M0P] = NULL;
		group->subdev[ENTRY_M1P] = NULL;
		group->subdev[ENTRY_M2P] = NULL;
		group->subdev[ENTRY_M3P] = NULL;
		group->subdev[ENTRY_M4P] = NULL;
		group->subdev[ENTRY_VRA] = NULL;

		INIT_LIST_HEAD(&group->subdev_list);
		break;
	case GROUP_SLOT_VRA:
		group->subdev[ENTRY_3AA] = NULL;
		group->subdev[ENTRY_3AC] = NULL;
		group->subdev[ENTRY_3AP] = NULL;
		group->subdev[ENTRY_ISP] = NULL;
		group->subdev[ENTRY_IXC] = NULL;
		group->subdev[ENTRY_IXP] = NULL;
		group->subdev[ENTRY_DRC] = NULL;
		group->subdev[ENTRY_DIS] = NULL;
		group->subdev[ENTRY_DXC] = NULL;
		group->subdev[ENTRY_ODC] = NULL;
		group->subdev[ENTRY_DNR] = NULL;
		group->subdev[ENTRY_SCC] = NULL;
		group->subdev[ENTRY_SCP] = NULL;
		group->subdev[ENTRY_M0P] = NULL;
		group->subdev[ENTRY_M1P] = NULL;
		group->subdev[ENTRY_M2P] = NULL;
		group->subdev[ENTRY_M3P] = NULL;
		group->subdev[ENTRY_M4P] = NULL;
		group->subdev[ENTRY_VRA] = &device->group_vra.leader;

		INIT_LIST_HEAD(&group->subdev_list);
		list_add_tail(&device->group_vra.leader.list, &group->subdev_list);
		break;
	default:
		probe_err("group slot(%d) is invalid", group->slot);
		BUG();
		break;
	}

	return ret;
}

int fimc_is_hw_group_open(void *group_data)
{
	int ret = 0;
	u32 group_id;
	struct fimc_is_subdev *leader;
	struct fimc_is_group *group;
	struct fimc_is_device_ischain *device;

	FIMC_BUG(!group_data);

	group = group_data;
	leader = &group->leader;
	device = group->device;
	group_id = group->id;

	switch (group_id) {
	case GROUP_ID_3AA0:
		leader->constraints_width = 4144;
		leader->constraints_height = 3106;
		break;
	case GROUP_ID_ISP0:
		leader->constraints_width = 4144;
		leader->constraints_height = 3106;
		break;
	case GROUP_ID_VRA0:
		leader->constraints_width = 4144;
		leader->constraints_height = 3106;
		break;
	default:
		merr("group id is invalid(%d)", group, group_id);
		break;
	}

	return ret;
}

void fimc_is_hw_camif_init(void)
{
}

int fimc_is_hw_camif_cfg(void *sensor_data)
{
	return 0;
}

int fimc_is_hw_camif_open(void *sensor_data)
{
	return 0;
}

int fimc_is_hw_ischain_cfg(void *ischain_data)
{
	return 0;
}

void __iomem *fimc_is_hw_get_sysreg(ulong core_regs)
{
	void *sysregs;

	if (core_reg)
		sysregs = (void *)core_regs;
	else
		sysregs = NULL;

	return sysregs;
}
