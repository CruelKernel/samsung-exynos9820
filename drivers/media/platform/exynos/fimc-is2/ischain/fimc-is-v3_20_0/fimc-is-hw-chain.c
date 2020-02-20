/*
 * Samsung Exynos SoC series FIMC-IS2 driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
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

#include <asm/neon.h>

#include "fimc-is-config.h"
#include "fimc-is-param.h"
#include "fimc-is-type.h"
#include "fimc-is-regs.h"
#include "fimc-is-core.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-ischain.h"
#include "fimc-is-device-flite.h"
#include "fimc-is-device-csi.h"
#include "fimc-is-hw-chain.h"

#include "../../interface/fimc-is-interface-ischain.h"
#include "../../hardware/fimc-is-hw-control.h"
#include "../../hardware/fimc-is-hw-mcscaler-v2.h"

/* for access sysreg_isp register */
#define SYSREG_ISP_MIPIPHY_CON	0x144F1040

/* Define default subdev ops if there are not used subdev IP */
const struct fimc_is_subdev_ops fimc_is_subdev_dis_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_scc_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_scp_ops;

void fimc_is_enter_lib_isr(void)
{
	kernel_neon_begin();
}

void fimc_is_exit_lib_isr(void)
{
	kernel_neon_end();
}

void __iomem *hwfc_rst;
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
		group->subdev[ENTRY_3AA] = &device->group_3aa.leader;
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
		list_add_tail(&device->group_3aa.leader.list, &group->subdev_list);
		list_add_tail(&device->txc.list, &group->subdev_list);
		list_add_tail(&device->txp.list, &group->subdev_list);

		/* 3AA-ISP ISP uses vdma2 as 3AC DMA */
		device->txc.param_dma_ot = PARAM_3AA_VDMA2_OUTPUT;
		device->txp.param_dma_ot = PARAM_3AA_VDMA2_OUTPUT;
		break;
	case GROUP_SLOT_ISP:
		group->subdev[ENTRY_3AA] = NULL;
		group->subdev[ENTRY_3AC] = NULL;
		group->subdev[ENTRY_3AP] = NULL;
		group->subdev[ENTRY_ISP] = &device->group_isp.leader;
		group->subdev[ENTRY_IXC] = &device->ixc;
		group->subdev[ENTRY_IXP] = &device->ixp;
		group->subdev[ENTRY_DRC] = NULL;
		group->subdev[ENTRY_DIS] = NULL;
		group->subdev[ENTRY_DXC] = NULL;
		group->subdev[ENTRY_ODC] = NULL;
		group->subdev[ENTRY_DNR] = NULL;
		group->subdev[ENTRY_SCC] = NULL;
		group->subdev[ENTRY_SCP] = NULL;
		group->subdev[ENTRY_VRA] = NULL;

		INIT_LIST_HEAD(&group->subdev_list);
		list_add_tail(&device->group_isp.leader.list, &group->subdev_list);
		list_add_tail(&device->ixc.list, &group->subdev_list);
		list_add_tail(&device->ixp.list, &group->subdev_list);
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
		group->subdev[ENTRY_MCS] = &device->group_mcs.leader;
		group->subdev[ENTRY_M0P] = &device->m0p;
		group->subdev[ENTRY_M1P] = NULL;
		group->subdev[ENTRY_M2P] = NULL;
		group->subdev[ENTRY_M3P] = NULL;
		group->subdev[ENTRY_M4P] = NULL;
		group->subdev[ENTRY_VRA] = NULL;

		INIT_LIST_HEAD(&group->subdev_list);
		list_add_tail(&device->group_mcs.leader.list, &group->subdev_list);
		list_add_tail(&device->m0p.list, &group->subdev_list);
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

	/* for hwfc */
	hwfc_rst = ioremap(0x12C00850, SZ_4);

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
		leader->constraints_width = 4144; /* 13MP(16:9) max width */
		leader->constraints_height = 3106; /* 13MP(4:3) max height */
		break;
	case GROUP_ID_ISP0:
		leader->constraints_width = 4240; /* 13MP(16:9) max width */
		leader->constraints_height = 3152; /* 13MP(4:3) max height */
		break;
	case GROUP_ID_MCS0:
	case GROUP_ID_MCS1:
		leader->constraints_width = 4240; /* 13MP(4:3) max width */
		leader->constraints_height = 3152; /* 13MP(4:3) max height */
		break;
	case GROUP_ID_VRA0:
		leader->constraints_width = 640; /* VGA max width */
		leader->constraints_height = 480; /* VGA max height */
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
	int ret = 0;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_flite *flite;
	struct fimc_is_device_csi *csi;
	struct fimc_is_device_ischain *ischain;
	bool is_otf = false;
	void __iomem *isp_mipiphy_con;
	u32 val;

	FIMC_BUG(!sensor_data);

	sensor = sensor_data;
	ischain = sensor->ischain;
	flite = (struct fimc_is_device_flite *)v4l2_get_subdevdata(sensor->subdev_flite);
	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);
	is_otf = (ischain && test_bit(FIMC_IS_GROUP_OTF_INPUT, &ischain->group_3aa.state));
	isp_mipiphy_con = ioremap(SYSREG_ISP_MIPIPHY_CON, SZ_32);

	/* always clear csis dummy */
	clear_bit(CSIS_DUMMY, &csi->state);

	switch (sensor->instance) {
	case 0:
		clear_bit(FLITE_DUMMY, &flite->state);

		/* MIPI CSIS select PHY0 */
		val = readl(isp_mipiphy_con);
		val &= ~(1 << 2);
		writel(val, isp_mipiphy_con);
		break;
	case 1:
		if (is_otf)
			clear_bit(FLITE_DUMMY, &flite->state);
		else
			set_bit(FLITE_DUMMY, &flite->state);

		/* MIPI CSIS select PHY1 */
		val = readl(isp_mipiphy_con);
		val |= (1 << 2);
		writel(val, isp_mipiphy_con);
		break;
	default:
		merr("sensor id is invalid(%d)", sensor, sensor->instance);
		break;
	}

	iounmap(isp_mipiphy_con);

	return ret;
}

int fimc_is_hw_camif_open(void *sensor_data)
{
	int ret = 0;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_flite *flite;
	struct fimc_is_device_csi *csi;

	FIMC_BUG(!sensor_data);

	sensor = sensor_data;
	flite = (struct fimc_is_device_flite *)v4l2_get_subdevdata(sensor->subdev_flite);
	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);

	switch (sensor->instance) {
	case 0:
		set_bit(CSIS_DMA_ENABLE, &csi->state);
		clear_bit(FLITE_DMA_ENABLE, &flite->state);
		break;
	case 1:
		set_bit(CSIS_DMA_ENABLE, &csi->state);
		clear_bit(FLITE_DMA_ENABLE, &flite->state);
		break;
	default:
		merr("sensor id is invalid(%d)", sensor, sensor->instance);
		break;
	}

	return ret;
}

int fimc_is_hw_ischain_cfg(void *ischain_data)
{
#ifdef ENABLE_SHARED_METADATA
	struct fimc_is_device_ischain *device = (struct fimc_is_device_ischain *)ischain_data;

	/* shared meta initialize */
	fimc_is_hw_shared_meta_update(device, NULL, NULL, SHARED_META_INIT);
#endif

	return 0;
}

static irqreturn_t interface_3aa_isr1(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_3aa = NULL;
	struct hwip_intr_handler *intr_3aa = NULL;

	itf_3aa = (struct fimc_is_interface_hwip *)data;
	intr_3aa = &itf_3aa->handler[INTR_HWIP1];

	if (intr_3aa->valid) {
		fimc_is_enter_lib_isr();
		intr_3aa->handler(intr_3aa->id, intr_3aa->ctx);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[%d]3aa(1) empty handler!!", itf_3aa->id);
	}

	return IRQ_HANDLED;
}

static irqreturn_t interface_3aa_isr2(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_3aa = NULL;
	struct hwip_intr_handler *intr_3aa = NULL;

	itf_3aa = (struct fimc_is_interface_hwip *)data;
	intr_3aa = &itf_3aa->handler[INTR_HWIP2];

	if (intr_3aa->valid) {
		fimc_is_enter_lib_isr();
		intr_3aa->handler(intr_3aa->id, intr_3aa->ctx);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[%d]3aa(2) empty handler!!", itf_3aa->id);
	}

	return IRQ_HANDLED;
}

static irqreturn_t interface_3aa_isr3(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_3aa = NULL;
	struct hwip_intr_handler *intr_3aa = NULL;

	itf_3aa = (struct fimc_is_interface_hwip *)data;
	intr_3aa = &itf_3aa->handler[INTR_HWIP3];

	if (intr_3aa->valid) {
		fimc_is_enter_lib_isr();
		intr_3aa->handler(intr_3aa->id, intr_3aa->ctx);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[%d]3aa(3) empty handler!!", itf_3aa->id);
	}

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
	intr_vra = &itf_vra->handler[INTR_HWIP2];

	if (intr_vra->valid) {
		fimc_is_enter_lib_isr();
		intr_vra->handler(intr_vra->id, (void *)itf_vra->hw_ip);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[%d]vra(1) empty handler!!", itf_vra->id);
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
	case DEV_HW_MCSC0:
		slot_id = 1;
		break;
	case DEV_HW_VRA:
		slot_id = 2;
		break;
	case DEV_HW_MCSC1:
		slot_id = 3;
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
	case GROUP_ID_MCS0:
		hw_list[hw_index] = DEV_HW_MCSC0; hw_index++;
		break;
	case GROUP_ID_VRA0:
		hw_list[hw_index] = DEV_HW_VRA; hw_index++;
		break;
	case GROUP_ID_MCS1:
		hw_list[hw_index] = DEV_HW_MCSC1; hw_index++;
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
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_ISP);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] ISP VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);

		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_ISP_SET_B);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_b_start = mem_res->start;
		itf_hwip->hw_ip->regs_b_end = mem_res->end;
		itf_hwip->hw_ip->regs_b = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] ISP(SETB) VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs_b);

		break;
	case DEV_HW_MCSC0:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_MCSC0);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] MCSC VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_VRA:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_VRA_SET_A);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] VRA0 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);

		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_VRA_SET_B);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_b_start = mem_res->start;
		itf_hwip->hw_ip->regs_b_end = mem_res->end;
		itf_hwip->hw_ip->regs_b = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs_b) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] VRA1 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs_b);
		break;
	case DEV_HW_MCSC1:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_MCSC1);
		if (!mem_res) {
			dev_err(&pdev->dev, "Failed to get io memory region\n");
			return -EINVAL;
		}

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] MCSC VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
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
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 0);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq 3aa0-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 1);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq 3aa0-2\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP3] = platform_get_irq(pdev, 2);
		if (itf_hwip->irq[INTR_HWIP3] < 0) {
			err("Failed to get irq 3aa0-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_MCSC0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 3);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq scaler\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = 0;
		itf_hwip->irq[INTR_HWIP3] = 0;
		break;
	case DEV_HW_VRA:
		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 4);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq vra \n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP1] = 0;
		itf_hwip->irq[INTR_HWIP3] = 0;
		break;
	case DEV_HW_MCSC1:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 5);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq scaler\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = 0;
		itf_hwip->irq[INTR_HWIP3] = 0;
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

		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP3]);
		snprintf(itf_hwip->irq_name[INTR_HWIP3], name_len, "fimc3a0-3");
		ret = request_irq(itf_hwip->irq[INTR_HWIP3], interface_3aa_isr3,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP3],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [3] fail", hw_id);
			return -EINVAL;
		}
		break;
	case DEV_HW_MCSC0:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimcmcsc-0");
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
		/* VRA */
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP2]);
		snprintf(itf_hwip->irq_name[INTR_HWIP2], name_len, "fimcvra-0");
		ret = request_irq(itf_hwip->irq[INTR_HWIP2], interface_vra_isr1,
			FIMC_IS_HW_IRQ_FLAG,
			itf_hwip->irq_name[INTR_HWIP2],
			itf_hwip);
		if (ret) {
			err_itfc("[ID:%d] request_irq [1] fail", hw_id);
			return -EINVAL;
		}
		itf_hwip->handler[INTR_HWIP2].id = INTR_HWIP2;
		itf_hwip->handler[INTR_HWIP2].valid = true;
		break;
	case DEV_HW_MCSC1:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimcmcsc-1");
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
		/* nothing */
		break;
	case HW_S_CTRL_CHAIN_IRQ:
		/* nothing */
		break;
	case HW_S_CTRL_HWFC_IDX_RESET:
		{
			struct fimc_is_video_ctx *vctx = (struct fimc_is_video_ctx *)itfc_data;
			struct fimc_is_device_ischain *device;
			unsigned long data = (unsigned long)val;

			FIMC_BUG(!vctx);
			FIMC_BUG(!GET_DEVICE(vctx));

			device = GET_DEVICE(vctx);

			/* reset if this instance is reprocessing */
			if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
				writel(data, hwfc_rst);
		}
		break;
	}

	return ret;
}

int fimc_is_hw_g_ctrl(void *itfc_data, int hw_id, enum hw_g_ctrl_id id, void *val)
{
	int ret = 0;

	switch (id) {
	case HW_G_CTRL_FRM_DONE_WITH_DMA:
		*(bool *)val = false;
		break;
	case HW_G_CTRL_HAS_MCSC:
		*(bool *)val = true;
		break;
	case HW_G_CTRL_HAS_VRA_CH1_ONLY:
		*(bool *)val = true;
		break;
	}

	return ret;
}

int fimc_is_hw_query_cap(void *cap_data, int hw_id)
{
	int ret = 0;

	FIMC_BUG(!cap_data);

	switch (hw_id) {
	case DEV_HW_MCSC0:
		{
			struct fimc_is_hw_mcsc_cap *cap = (struct fimc_is_hw_mcsc_cap *)cap_data;
			/* v1.22 */
			cap->hw_ver = HW_SET_VERSION(1, 22, 0, 0);
			cap->max_output = 1;
			cap->in_otf = MCSC_CAP_SUPPORT;
			cap->out_dma[0] = MCSC_CAP_SUPPORT;
			cap->enable_shared_output = false;
		}
		break;
	case DEV_HW_MCSC1:
		{
			struct fimc_is_hw_mcsc_cap *cap = (struct fimc_is_hw_mcsc_cap *)cap_data;
			/* v1.23 */
			cap->hw_ver = HW_SET_VERSION(1, 23, 0, 0);
			cap->max_output = 1;
			cap->hwfc = MCSC_CAP_SUPPORT;
			cap->in_dma = MCSC_CAP_SUPPORT;
			cap->out_hwfc[0] = MCSC_CAP_SUPPORT;
			cap->out_dma[0] = MCSC_CAP_SUPPORT;
			cap->enable_shared_output = false;
		}
		break;
	default:
		break;
	}

	return ret;
}

#ifdef ENABLE_SHARED_METADATA
int fimc_is_hw_shared_meta_update(struct fimc_is_device_ischain *device,
		struct fimc_is_group *group, struct fimc_is_frame *frame, int shot_done_flag)
{
	int ret = 0;
	struct fimc_is_resourcemgr *resourcemgr;

	resourcemgr = device->resourcemgr;

	switch (shot_done_flag) {
	case SHARED_META_INIT:
		if (!(device->instance)) {
			spin_lock(&resourcemgr->shared_meta_lock);
			memset(&resourcemgr->shared_shot.uctl.fdUd, 0x0, sizeof(struct camera2_fd_uctl));
			spin_unlock(&resourcemgr->shared_meta_lock);
		}
		break;
	case SHARED_META_SHOT:
		if (group->id == GROUP_ID_3AA0) {
			/* shot */
			spin_lock(&resourcemgr->shared_meta_lock);
			memcpy(&frame->shot->uctl.fdUd, &resourcemgr->shared_shot.uctl.fdUd,
				sizeof(struct camera2_fd_uctl));
			spin_unlock(&resourcemgr->shared_meta_lock);
		}
		break;
	case SHARED_META_SHOT_DONE:
		if (group->id == GROUP_ID_VRA0) {
			/* shot done */
			spin_lock(&resourcemgr->shared_meta_lock);
			memcpy(&resourcemgr->shared_shot.uctl.fdUd, &frame->shot->dm.stats,
				sizeof(struct camera2_fd_uctl));
			spin_unlock(&resourcemgr->shared_meta_lock);
		}
		break;
	default:
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

void fimc_is_hw_djag_get_input(struct mcsc_scenario_info *info, u32 *djag_in)
{
	/* Do nothing. */
}

void fimc_is_hw_djag_adjust_out_size(struct fimc_is_device_ischain *ischain,
					u32 in_width, u32 in_height,
					u32 *out_width, u32 *out_height)
{
	/* Do nothing. */
}
