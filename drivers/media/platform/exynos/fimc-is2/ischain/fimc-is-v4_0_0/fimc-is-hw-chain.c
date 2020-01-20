/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
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

#include <mach/map.h>
#include <mach/regs-clock.h>
#include "fimc-is-config.h"
#include "fimc-is-param.h"
#include "fimc-is-type.h"
#include "fimc-is-regs.h"
#include "fimc-is-groupmgr.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-ischain.h"
#include "fimc-is-hw-chain.h"

#ifndef ENABLE_IS_CORE
#include "fimc-is-core.h"
#include "../../interface/fimc-is-interface-ischain.h"
#include "../../hardware/fimc-is-hw-control.h"
#endif

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

		device->txc.param_dma_ot = PARAM_3AA_VDMA4_OUTPUT;
		device->txp.param_dma_ot = PARAM_3AA_VDMA2_OUTPUT;
		break;
	case GROUP_SLOT_ISP:
		group->subdev[ENTRY_3AA] = NULL;
		group->subdev[ENTRY_3AC] = NULL;
		group->subdev[ENTRY_3AP] = NULL;
		group->subdev[ENTRY_ISP] = NULL;
		group->subdev[ENTRY_IXC] = &device->ixc;
		group->subdev[ENTRY_IXP] = &device->ixp;
		group->subdev[ENTRY_DRC] = &device->drc;
		group->subdev[ENTRY_DIS] = NULL;
		group->subdev[ENTRY_DXC] = NULL;
		group->subdev[ENTRY_ODC] = NULL;
		group->subdev[ENTRY_DNR] = NULL;
		group->subdev[ENTRY_SCC] = NULL;
		group->subdev[ENTRY_SCP] = NULL;
		group->subdev[ENTRY_VRA] = NULL;

		INIT_LIST_HEAD(&group->subdev_list);
		list_add_tail(&device->ixc.list, &group->subdev_list);
		list_add_tail(&device->ixp.list, &group->subdev_list);
		list_add_tail(&device->drc.list, &group->subdev_list);
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
		group->subdev[ENTRY_ODC] = &device->odc;
		group->subdev[ENTRY_DNR] = &device->dnr;
		group->subdev[ENTRY_SCC] = &device->scc;
		group->subdev[ENTRY_SCP] = &device->scp;
		group->subdev[ENTRY_VRA] = &device->group_vra.leader;

		INIT_LIST_HEAD(&group->subdev_list);
		list_add_tail(&device->odc.list, &group->subdev_list);
		list_add_tail(&device->dnr.list, &group->subdev_list);
		list_add_tail(&device->scc.list, &group->subdev_list);
		list_add_tail(&device->scp.list, &group->subdev_list);
		list_add_tail(&device->group_vra.leader.list, &group->subdev_list);
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
		group->subdev[ENTRY_VRA] = NULL;

		INIT_LIST_HEAD(&group->subdev_list);
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
		leader->constraints_width = 5984;
		leader->constraints_height = 3376;
		break;
	case GROUP_ID_3AA1:
		leader->constraints_width = 4000;
		leader->constraints_height = 2256;
		break;
	case GROUP_ID_ISP0:
		leader->constraints_width = 5120;
		leader->constraints_height = 2704;
		break;
	case GROUP_ID_ISP1:
		leader->constraints_width = 5984;
		leader->constraints_height = 3376;
		break;
	case GROUP_ID_DIS0:
		leader->constraints_width = 5120;
		leader->constraints_height = 2704;
		break;
	case GROUP_ID_VRA0:
		leader->constraints_width = 5120;
		leader->constraints_height = 2704;
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

#ifndef ENABLE_IS_CORE
static irqreturn_t interface_3aa_isr1(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_3aa = NULL;
	struct hwip_intr_handler *intr_3aa = NULL;

	itf_3aa = (struct fimc_is_interface_hwip *)data;
	intr_3aa = &itf_3aa->handler[INTR_HWIP1];

	if (intr_3aa->valid)
		intr_3aa->handler(intr_3aa->id, intr_3aa->ctx);
	else
		err_itfc("[%d]3aa(1) empty handler!!", itf_3aa->id);

	return IRQ_HANDLED;
}

static irqreturn_t interface_3aa_isr2(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_3aa = NULL;
	struct hwip_intr_handler *intr_3aa = NULL;

	itf_3aa = (struct fimc_is_interface_hwip *)data;
	intr_3aa = &itf_3aa->handler[INTR_HWIP2];

	if (intr_3aa->valid)
		intr_3aa->handler(intr_3aa->id, intr_3aa->ctx);
	else
		err_itfc("[%d]3aa(2) empty handler!!", itf_3aa->id);

	return IRQ_HANDLED;
}

static irqreturn_t interface_isp_isr1(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_isp = NULL;
	struct hwip_intr_handler *intr_isp = NULL;

	itf_isp = (struct fimc_is_interface_hwip *)data;
	intr_isp = &itf_isp->handler[INTR_HWIP1];

	if (intr_isp->valid)
		intr_isp->handler(intr_isp->id, intr_isp->ctx);
	else
		err_itfc("[%d]isp(1) empty handler!!", itf_isp->id);

	return IRQ_HANDLED;
}

static irqreturn_t interface_isp_isr2(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_isp = NULL;
	struct hwip_intr_handler *intr_isp = NULL;

	itf_isp = (struct fimc_is_interface_hwip *)data;
	intr_isp = &itf_isp->handler[INTR_HWIP2];

	if (intr_isp->valid)
		intr_isp->handler(intr_isp->id, intr_isp->ctx);
	else
		err_itfc("[%d]isp(2) empty handler!!", itf_isp->id);

	return IRQ_HANDLED;
}

static irqreturn_t interface_tpu_isr1(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_tpu = NULL;
	struct hwip_intr_handler *intr_tpu = NULL;

	itf_tpu = (struct fimc_is_interface_hwip *)data;
	intr_tpu = &itf_tpu->handler[INTR_HWIP1];

	if (intr_tpu->valid)
		intr_tpu->handler(intr_tpu->id, intr_tpu->ctx);
	else
		err_itfc("[%d]tpu(1) empty handler!!", itf_tpu->id);

	return IRQ_HANDLED;
}

static irqreturn_t interface_tpu_isr2(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_tpu = NULL;
	struct hwip_intr_handler *intr_tpu = NULL;

	itf_tpu = (struct fimc_is_interface_hwip *)data;
	intr_tpu = &itf_tpu->handler[INTR_HWIP2];

	if (intr_tpu->valid)
		intr_tpu->handler(intr_tpu->id, intr_tpu->ctx);
	else
		err_itfc("[%d]tpu(2) empty handler!!", itf_tpu->id);

	return IRQ_HANDLED;
}

static irqreturn_t interface_scaler_isr(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_scaler = NULL;
	struct hwip_intr_handler *intr_scaler = NULL;

	itf_scaler = (struct fimc_is_interface_hwip *)data;
	intr_scaler = &itf_scaler->handler[INTR_HWIP1];

	if (intr_scaler->valid)
		intr_scaler->handler(intr_scaler->id, (void *)itf_scaler->hw_ip);
	else
		err_itfc("[%d]scaler empty handler!!", itf_scaler->id);

	return IRQ_HANDLED;
}

static irqreturn_t interface_vra_isr1(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_vra = NULL;
	struct hwip_intr_handler *intr_vra = NULL;

	itf_vra = (struct fimc_is_interface_hwip *)data;
	intr_vra = &itf_vra->handler[INTR_HWIP1];

	if (intr_vra->valid)
		intr_vra->handler(intr_vra->id, (void *)itf_vra->hw_ip);
	else
		err_itfc("[%d]vra(1) empty handler!!", itf_vra->id);

	return IRQ_HANDLED;
}

static irqreturn_t interface_vra_isr2(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_vra = NULL;
	struct hwip_intr_handler *intr_vra = NULL;

	itf_vra = (struct fimc_is_interface_hwip *)data;
	intr_vra = &itf_vra->handler[INTR_HWIP2];

	if (intr_vra->valid) {
		intr_vra->handler(intr_vra->id, (void *)itf_vra->hw_ip);
	} else {
		err_itfc("[%d]vra(2) empty handler!!", itf_vra->id);
	}

	return IRQ_HANDLED;
}

inline int fimc_is_hw_slot_id(int hw_id)
{
	int slot_id = -1;

	switch (hw_id) {
	case DEV_HW_3AA0:
		slot_id = 0;
		break;
	case DEV_HW_3AA1:
		slot_id = 1;
		break;
	case DEV_HW_ISP0:
		slot_id = 2;
		break;
	case DEV_HW_ISP1:
		slot_id = 3;
		break;
	case DEV_HW_TPU0:
		slot_id = 4;
		break;
	case DEV_HW_SCP:
		slot_id = 5;
		break;
	case DEV_HW_VRA:
		slot_id = 6;
		break;
	default:
		break;
	}

	return slot_id;
}

int fimc_is_get_hw_list(int group_id, int *hw_list)
{
	int i;
	int hw_index = 0;

	/* initialization */
	for (i = 0; i < GROUP_HW_MAX; i++)
		hw_list[i] = -1;

	switch (group_id) {
	case GROUP_ID_3AA0:
		hw_list[hw_index] = DEV_HW_3AA0; hw_index++;
		break;
	case GROUP_ID_3AA1:
		hw_list[hw_index] = DEV_HW_3AA1; hw_index++;
		break;
	case GROUP_ID_ISP0:
		hw_list[hw_index] = DEV_HW_ISP0; hw_index++;
		break;
	case GROUP_ID_ISP1:
		hw_list[hw_index] = DEV_HW_ISP1; hw_index++;
		break;
	case GROUP_ID_DIS0:
		hw_list[hw_index] = DEV_HW_TPU0; hw_index++;
		hw_list[hw_index] = DEV_HW_SCP; hw_index++;
		hw_list[hw_index] = DEV_HW_VRA; hw_index++;
		break;
	default:
		break;
	}

	if (hw_index > GROUP_HW_MAX)
		warn_hw("hw_index(%d) > GROUP_HW_MAX(%d)\n", hw_index, GROUP_HW_MAX);

	return hw_index;
}

int fimc_is_hw_get_address(void *itfc_data, void *pdev_data, int hw_id)
{
	int ret = 0;
	struct resource *mem_res = NULL;
	struct platform_device *pdev = NULL;
	struct fimc_is_interface_hwip *itf_hwip = NULL;

	FIMC_BUG(!itfc_data);
	FIMC_BUG(!pdev_data);

	itf_hwip = (struct fimc_is_interface_hwip *)itfc_data;
	pdev = (struct platform_device *)pdev_data;

	switch (hw_id) {
	case DEV_HW_3AA0:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_3AA0);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] 3AA0 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_3AA1:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_3AA1);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] 3AA1 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_ISP0:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_ISP0);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] ISP0 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_ISP1:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_ISP1);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] ISP1 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_TPU0:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_TPU);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] TPU VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_SCP:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_SCP);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] SCP VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_VRA:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_VRA_CH0);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] VRA0 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);

		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_VRA_CH1);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_b = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs_b) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] VRA1 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs_b);
		break;
	default:
		probe_err("hw_id(%d) is invalid", hw_id);
		return -EINVAL;
		break;
	}

	return ret;
}

int fimc_is_hw_get_irq(void *itfc_data, void *pdev_data, int hw_id)
{
	struct fimc_is_interface_hwip *itf_hwip = NULL;
	struct platform_device *pdev = NULL;
	int ret = 0;

	FIMC_BUG(!itfc_data);

	itf_hwip = (struct fimc_is_interface_hwip *)itfc_data;
	pdev = (struct platform_device *)pdev_data;

	switch (hw_id) {
	case DEV_HW_3AA0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 2);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq 3aa0-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 3);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq 3aa0-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_3AA1:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 4);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq 3aa1-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 5);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq 3aa0-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_ISP0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 6);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq isp0-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 7);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq isp0-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_ISP1:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 8);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq isp1-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 9);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq isp1-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_TPU0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 10);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq tpu-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 11);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq tpu-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_SCP:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 12);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq scaler\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = 0;
		break;
	case DEV_HW_VRA:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 13);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq vra \n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 14);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq vra \n");
			return -EINVAL;
		}
		break;
	default:
		probe_err("hw_id(%d) is invalid", hw_id);
		return -EINVAL;
		break;
	}

	return ret;
}

int fimc_is_hw_request_irq(void *itfc_data, int hw_id)
{
	struct fimc_is_interface_hwip *itf_hwip = NULL;
	u32 name_len = 0;
	int ret = 0;

	FIMC_BUG(!itfc_data);

	dbg_itfc("%s: id(%d)\n", __func__, hw_id);

	itf_hwip = (struct fimc_is_interface_hwip *)itfc_data;

	switch (hw_id) {
	case DEV_HW_3AA0:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimc3a0-1");
		ret = request_irq(itf_hwip->irq[INTR_HWIP1], interface_3aa_isr1,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP1],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}

		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP2]);
		snprintf(itf_hwip->irq_name[INTR_HWIP2], name_len, "fimc3a0-2");
		ret = request_irq(itf_hwip->irq[INTR_HWIP2], interface_3aa_isr2,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP2],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [2] fail", hw_id);
			return -EINVAL;
		}
		break;
	case DEV_HW_3AA1:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimc3a1-1");
		ret = request_irq(itf_hwip->irq[INTR_HWIP1], interface_3aa_isr1,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP1],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}

		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP2]);
		snprintf(itf_hwip->irq_name[INTR_HWIP2], name_len, "fimc3a1-2");
		ret = request_irq(itf_hwip->irq[INTR_HWIP2], interface_3aa_isr2,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP2],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [2] fail", hw_id);
			return -EINVAL;
		}
		break;
	case DEV_HW_ISP0:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimcisp0-1");
		ret = request_irq(itf_hwip->irq[INTR_HWIP1], interface_isp_isr1,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP1],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}

		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP2]);
		snprintf(itf_hwip->irq_name[INTR_HWIP2], name_len, "fimcisp0-2");
		ret = request_irq(itf_hwip->irq[INTR_HWIP2], interface_isp_isr2,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP2],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [2] fail", hw_id);
			return -EINVAL;
		}
		break;
	case DEV_HW_ISP1:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimcisp1-1");
		ret = request_irq(itf_hwip->irq[INTR_HWIP1], interface_isp_isr1,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP1],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}

		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP2]);
		snprintf(itf_hwip->irq_name[INTR_HWIP2], name_len, "fimcisp1-2");
		ret = request_irq(itf_hwip->irq[INTR_HWIP2], interface_isp_isr2,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP2],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [2] fail", hw_id);
			return -EINVAL;
		}
		break;
	case DEV_HW_TPU0:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimctpu-1");
		ret = request_irq(itf_hwip->irq[INTR_HWIP1], interface_tpu_isr1,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP1],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}
		itf_hwip->handler[INTR_HWIP1].valid = true;

		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP2]);
		snprintf(itf_hwip->irq_name[INTR_HWIP2], name_len, "fimctpu-2");
		ret = request_irq(itf_hwip->irq[INTR_HWIP2], interface_tpu_isr2,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP2],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [2] fail", hw_id);
			return -EINVAL;
		}
		itf_hwip->handler[INTR_HWIP2].valid = true;
		break;
	case DEV_HW_SCP:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimcscp");
		ret = request_irq(itf_hwip->irq[INTR_HWIP1], interface_scaler_isr,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP1],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}
		itf_hwip->handler[INTR_HWIP1].valid = true;
		break;
	case DEV_HW_VRA:
		/* VRA CH0 */
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimcvra-0");
		ret = request_irq(itf_hwip->irq[INTR_HWIP1], interface_vra_isr1,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP1],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}
		itf_hwip->handler[INTR_HWIP1].id = INTR_HWIP1;
		itf_hwip->handler[INTR_HWIP1].valid = true;

		/* VRA CH1 */
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP2]);
		snprintf(itf_hwip->irq_name[INTR_HWIP2], name_len, "fimcvra-1");
		ret = request_irq(itf_hwip->irq[INTR_HWIP2], interface_vra_isr2,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP2],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [2] fail", hw_id);
			return -EINVAL;
		}
		itf_hwip->handler[INTR_HWIP2].id = INTR_HWIP2;
		itf_hwip->handler[INTR_HWIP2].valid = true;
		break;
	default:
		probe_err("hw_id(%d) is invalid", hw_id);
		return -EINVAL;
		break;
	}

	return ret;
}

int fimc_is_hw_s_ctrl(void *itfc_data, int hw_id, enum hw_s_ctrl_id id, void *val)
{
	int ret = 0;

	switch (id) {
	case HW_S_CTRL_FULL_BYPASS:
		{
			struct fimc_is_interface_ischain *itfc = NULL;
			unsigned long bypass = (unsigned long)val;
			u32 values = 0;

			FIMC_BUG(!itfc_data);

			itfc = (struct fimc_is_interface_ischain *)itfc_data;
			switch (hw_id) {
			case DEV_HW_TPU0:
				values = readl(itfc->regs_mcuctl + MCUCTLR);
				if (bypass)
					values |= MCUCTLR_TPU_HW_BYPASS(1);
				else
					values &= ~MCUCTLR_TPU_HW_BYPASS(1);
				writel(values, itfc->regs_mcuctl + MCUCTLR);
				info_itfc("[ID:%d] Full bypass set (%lu)", hw_id, bypass);
				break;
			default:
				err_itfc("[ID:%d] request_irq [2] fail", hw_id);
				ret = -EINVAL;
			}
		}
		break;
	case HW_S_CTRL_CHAIN_IRQ:
		{
			struct fimc_is_interface *itf = NULL;

			FIMC_BUG(!itfc_data);

			itf = (struct fimc_is_interface *)itfc_data;

			writel(0x7fff, itf->regs + INTMR2);
		}
		break;
	default:
		break;
	}

	return ret;
}

int fimc_is_hw_g_ctrl(void *itfc_data, int hw_id, enum hw_g_ctrl_id id, void *val)
{
	int ret = 0;

	switch (id) {
	case HW_G_CTRL_FRM_DONE_WITH_DMA:
		*(bool *)val = true;
		break;
	case HW_G_CTRL_HAS_MCSC:
		*(bool *)val = false;
		break;
	case HW_G_CTRL_HAS_VRA_CH1_ONLY:
		*(bool *)val = false;
		break;
	}

	return ret;
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
#endif
