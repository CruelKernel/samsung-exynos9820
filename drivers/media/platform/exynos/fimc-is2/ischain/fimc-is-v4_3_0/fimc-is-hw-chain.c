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

#include "fimc-is-config.h"
#include "fimc-is-param.h"
#include "fimc-is-type.h"
#include "fimc-is-regs.h"
#include "fimc-is-core.h"
#include "fimc-is-hw-chain.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-flite.h"
#include "fimc-is-device-csi.h"
#include "fimc-is-device-ischain.h"

#ifndef ENABLE_IS_CORE
#include "../../interface/fimc-is-interface-ischain.h"
#include "../../hardware/fimc-is-hw-control.h"
#include "../../hardware/fimc-is-hw-mcscaler-v2.h"
#endif

static struct fimc_is_reg mcuctl_regs[HW_MCUCTL_REG_CNT] = {
	{0x0000, "MCUCTRLR"},
	{0x0060, "MCUCTRLR2"},
};
static struct fimc_is_field mcuctl_fields[HW_MCUCTL_REG_FIELD_CNT] = {
	{"IN_PATH_SEL_MCS1"   , 31, 1, RW, 0},
	{"IN_PATH_SEL_TPU"    , 30, 1, RW, 0},
	{"OUT_PATH_SEL_TPU"   , 29, 1, RW, 0},
	{"ISPCPU_RT_INFO"     , 27, 1, RW, 0x1},
	{"FIMC_VRA_RT_INFO"   , 26, 1, RW, 0  },
	{"CSIS_3_RT_INFO"     , 25, 1, RW, 0x1},
	{"CSIS_2_RT_INFO"     , 24, 1, RW, 0x1},
	{"SHARED_BUFFER_SEL"  , 23, 1, RW, 0  },
	{"IN_PATH_SEL_BNS"    , 22, 1, RW, 0  },
	{"IN_PATH_SEL_3AAA0"  , 20, 2, RW, 0  },
	{"IN_PATH_SEL_3AA1"   , 18, 2, RW, 0  },
	{"IN_PATH_SEL_ISP0"   , 17, 1, RW, 0  },
	{"IN_PATH_SEL_ISP1"   , 16, 1, RW, 0  },
	{"FIMC_3AA0_RT_INFO"  , 13, 1, RW, 0  },
	{"FIMC_3AA1_RT_INFO"  , 12, 1, RW, 0  },
	{"FIMC_ISP0_RT_INFO"  , 11, 1, RW, 0  },
	{"FIMC_ISP1_RT_INFO"  , 10, 1, RW, 0  },
	{"MC_SCALER_RT_INFO"  , 9 , 1, RW, 0  },
	{"FIMC_TPU_RT_INFO"   , 8 , 1, RW, 0  },
	{"AXI_TREX_C_AWCACHE" , 4 , 4, RW, 0x2},
	{"AXI_TREX_C_ARCACHE" , 0 , 4, RW, 0x2},
};

/* Define default subdev ops if there are not used subdev IP */
const struct fimc_is_subdev_ops fimc_is_subdev_scc_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_scp_ops;

#ifndef ENABLE_IS_CORE
struct fimc_is_clk_gate clk_gate_3aa;
struct fimc_is_clk_gate clk_gate_isp;
struct fimc_is_clk_gate clk_gate_vra;
#endif

void __iomem *hwfc_rst;

void fimc_is_enter_lib_isr(void)
{
	kernel_neon_begin();
}

void fimc_is_exit_lib_isr(void)
{
	kernel_neon_end();
}

int fimc_is_hw_group_cfg(void *group_data)
{
	int ret = 0;
	struct fimc_is_group *group;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_ischain *device;

	FIMC_BUG(!group_data);

	group = group_data;
	device = group->device;
	sensor = group->sensor;

	if (!device && !sensor) {
		err("device is NULL");
		BUG();
	}

	switch (group->slot) {
#ifdef CONFIG_USE_SENSOR_GROUP
	case GROUP_SLOT_SENSOR:
		group->subdev[ENTRY_SENSOR] = &sensor->group_sensor.leader;
		group->subdev[ENTRY_SSVC0] = &sensor->ssvc0;
		group->subdev[ENTRY_SSVC1] = &sensor->ssvc1;
		group->subdev[ENTRY_SSVC2] = &sensor->ssvc2;
		group->subdev[ENTRY_SSVC3] = &sensor->ssvc3;
		group->subdev[ENTRY_BNS] = &sensor->bns;
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
		list_add_tail(&sensor->group_sensor.leader.list, &group->subdev_list);
		list_add_tail(&sensor->ssvc0.list, &group->subdev_list);
		list_add_tail(&sensor->ssvc1.list, &group->subdev_list);
		list_add_tail(&sensor->ssvc2.list, &group->subdev_list);
		list_add_tail(&sensor->ssvc3.list, &group->subdev_list);
		list_add_tail(&sensor->bns.list, &group->subdev_list);
		break;
#endif
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

		device->txc.param_dma_ot = PARAM_3AA_VDMA4_OUTPUT;
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
		group->subdev[ENTRY_DIS] = &device->group_dis.leader;
		group->subdev[ENTRY_DXC] = NULL;
		group->subdev[ENTRY_ODC] = &device->odc;
		group->subdev[ENTRY_DNR] = &device->dnr;
		group->subdev[ENTRY_SCC] = NULL;
		group->subdev[ENTRY_SCP] = NULL;
		group->subdev[ENTRY_VRA] = NULL;

		INIT_LIST_HEAD(&group->subdev_list);
		list_add_tail(&device->group_dis.leader.list, &group->subdev_list);
		list_add_tail(&device->odc.list, &group->subdev_list);
		list_add_tail(&device->dnr.list, &group->subdev_list);
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
		group->subdev[ENTRY_M1P] = &device->m1p;
		group->subdev[ENTRY_M2P] = &device->m2p;
		group->subdev[ENTRY_M3P] = &device->m3p;
		group->subdev[ENTRY_M4P] = &device->m4p;
		group->subdev[ENTRY_VRA] = &device->group_vra.leader;

		INIT_LIST_HEAD(&group->subdev_list);
		list_add_tail(&device->group_mcs.leader.list, &group->subdev_list);
		list_add_tail(&device->m0p.list, &group->subdev_list);
		list_add_tail(&device->m1p.list, &group->subdev_list);
		list_add_tail(&device->m2p.list, &group->subdev_list);
		list_add_tail(&device->m3p.list, &group->subdev_list);
		list_add_tail(&device->m4p.list, &group->subdev_list);
		list_add_tail(&device->group_vra.leader.list, &group->subdev_list);

		device->m0p.param_dma_ot = PARAM_MCS_OUTPUT0;
		device->m1p.param_dma_ot = PARAM_MCS_OUTPUT1;
		device->m2p.param_dma_ot = PARAM_MCS_OUTPUT2;
		device->m3p.param_dma_ot = PARAM_MCS_OUTPUT3;
		device->m4p.param_dma_ot = PARAM_MCS_OUTPUT4;
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

	/* for hwfc: reset all REGION_IDX registers and outputs */
	hwfc_rst = ioremap(0x14120850, SZ_4);

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
#ifdef CONFIG_USE_SENSOR_GROUP
	case GROUP_ID_SS0:
	case GROUP_ID_SS1:
	case GROUP_ID_SS2:
	case GROUP_ID_SS3:
	case GROUP_ID_SS4:
	case GROUP_ID_SS5:
#endif
	case GROUP_ID_3AA0:
	case GROUP_ID_3AA1:
		leader->constraints_width = 6532;
		leader->constraints_height = 3676;
		break;
	case GROUP_ID_ISP0:
		leader->constraints_width = 5120;
		leader->constraints_height = 2704;
		break;
	case GROUP_ID_ISP1:
		leader->constraints_width = 6532;
		leader->constraints_height = 3676;
		break;
	case GROUP_ID_DIS0:
		leader->constraints_width = 5120;
		leader->constraints_height = 2704;
		break;
	case GROUP_ID_MCS0:
		leader->constraints_width = 4096;
		leader->constraints_height = 2160;
		break;
	case GROUP_ID_MCS1:
		leader->constraints_width = 6532;
		leader->constraints_height = 3676;
		break;
	case GROUP_ID_VRA0:
		leader->constraints_width = 4096;
		leader->constraints_height = 2160;
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

	FIMC_BUG(!sensor_data);

	sensor = sensor_data;
	ischain = sensor->ischain;
	flite = (struct fimc_is_device_flite *)v4l2_get_subdevdata(sensor->subdev_flite);
	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);
	is_otf = (ischain && test_bit(FIMC_IS_GROUP_OTF_INPUT, &ischain->group_3aa.state));

	/* always clear csis dummy */
	clear_bit(CSIS_DUMMY, &csi->state);

	switch (csi->instance) {
	case 0:
		clear_bit(FLITE_DUMMY, &flite->state);

#ifdef SOC_SSVC0
		csi->dma_subdev[CSI_VIRTUAL_CH_0] = &sensor->ssvc0;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_0] = NULL;
#endif
#ifdef SOC_SSVC1
		csi->dma_subdev[CSI_VIRTUAL_CH_1] = &sensor->ssvc1;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_1] = NULL;
#endif
#ifdef SOC_SSVC2
		csi->dma_subdev[CSI_VIRTUAL_CH_2] = &sensor->ssvc2;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_2] = NULL;
#endif
#ifdef SOC_SSVC3
		csi->dma_subdev[CSI_VIRTUAL_CH_3] = &sensor->ssvc3;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_3] = NULL;
#endif
		break;
	case 2:
		if (is_otf)
			clear_bit(FLITE_DUMMY, &flite->state);
		else
			set_bit(FLITE_DUMMY, &flite->state);

#ifdef SOC_SSVC0
		csi->dma_subdev[CSI_VIRTUAL_CH_0] = &sensor->ssvc0;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_0] = NULL;
#endif
#ifdef SOC_SSVC1
		csi->dma_subdev[CSI_VIRTUAL_CH_1] = &sensor->ssvc1;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_1] = NULL;
#endif
#ifdef SOC_SSVC2
		csi->dma_subdev[CSI_VIRTUAL_CH_2] = &sensor->ssvc2;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_2] = NULL;
#endif
#ifdef SOC_SSVC3
		csi->dma_subdev[CSI_VIRTUAL_CH_3] = &sensor->ssvc3;
#else
		csi->dma_subdev[CSI_VIRTUAL_CH_3] = NULL;
#endif
		break;
	case 1:
	case 3:
		set_bit(FLITE_DUMMY, &flite->state);
		break;
	default:
		merr("sensor id is invalid(%d)", sensor, csi->instance);
		break;
	}

#ifdef CONFIG_CSIS_V4_0
	/* HACK: For dual scanario in EVT0, we should use CSI DMA out */
	if (csi->instance == 2 && !is_otf) {
		struct fimc_is_device_flite backup_flite;
		struct fimc_is_device_csi backup_csi;

		minfo("For dual scanario, reconfigure front camera", sensor);

		ret = fimc_is_csi_close(sensor->subdev_csi);
		if (ret)
			merr("fimc_is_csi_close is fail(%d)", sensor, ret);

		ret = fimc_is_flite_close(sensor->subdev_flite);
		if (ret)
			merr("fimc_is_flite_close is fail(%d)", sensor, ret);

		set_bit(CSIS_DMA_ENABLE, &csi->state);
		clear_bit(FLITE_DMA_ENABLE, &flite->state);
		memcpy(&backup_flite, flite, sizeof(struct fimc_is_device_flite));
		memcpy(&backup_csi, csi, sizeof(struct fimc_is_device_csi));

		ret = fimc_is_csi_open(sensor->subdev_csi, GET_FRAMEMGR(sensor->vctx));
		if (ret)
			merr("fimc_is_csi_open is fail(%d)", sensor, ret);

		ret = fimc_is_flite_open(sensor->subdev_flite, GET_FRAMEMGR(sensor->vctx));
		if (ret)
			merr("fimc_is_flite_open is fail(%d)", sensor, ret);

		backup_csi.framemgr = csi->framemgr;
		memcpy(flite, &backup_flite, sizeof(struct fimc_is_device_flite));
		memcpy(csi, &backup_csi, sizeof(struct fimc_is_device_csi));
	}
#endif
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

	switch (csi->instance) {
#ifdef CONFIG_CSIS_V4_0
	case 0:
		clear_bit(CSIS_DMA_ENABLE, &csi->state);
		set_bit(FLITE_DMA_ENABLE, &flite->state);
		break;
	case 2:
		clear_bit(CSIS_DMA_ENABLE, &csi->state);
		set_bit(FLITE_DMA_ENABLE, &flite->state);
		break;
#else
	case 0:
		set_bit(CSIS_DMA_ENABLE, &csi->state);
		clear_bit(FLITE_DMA_ENABLE, &flite->state);
		break;
	case 2:
		set_bit(CSIS_DMA_ENABLE, &csi->state);
		clear_bit(FLITE_DMA_ENABLE, &flite->state);
		break;
#endif
	case 1:
	case 3:
		set_bit(CSIS_DMA_ENABLE, &csi->state);
		clear_bit(FLITE_DMA_ENABLE, &flite->state);
		break;
	default:
		merr("sensor id is invalid(%d)", sensor, csi->instance);
		break;
	}

	return ret;
}

int fimc_is_hw_ischain_cfg(void *ischain_data)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct fimc_is_device_ischain *device;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_csi *csi;
	int i, sensor_cnt = 0;
	void __iomem *regs;
	u32 val;

	FIMC_BUG(!ischain_data);

	device = (struct fimc_is_device_ischain *)ischain_data;
	if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
		return ret;

	core = (struct fimc_is_core *)platform_get_drvdata(device->pdev);
	sensor = device->sensor;
	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);

	/* checked single/dual camera */
	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++)
		if (test_bit(FIMC_IS_SENSOR_OPEN, &(core->sensor[i].state)))
			sensor_cnt++;

	/* get MCUCTL base */
	regs = device->interface->regs;
	/* get MCUCTL controller reg2 value */
	val = fimc_is_hw_get_reg(regs, &mcuctl_regs[MCUCTRLR2]);

	/* NRT setting */
	val = fimc_is_hw_set_field_value(val,
			&mcuctl_fields[MCUCTRLR2_ISPCPU_RT_INFO], 0);
	/* 1) RT setting */
	val = fimc_is_hw_set_field_value(val,
			&mcuctl_fields[MCUCTRLR2_FIMC_3AA0_RT_INFO], 1);
	/* changed ISP0 NRT NRT from RT due to using 3AA0/ISP M2M */
	val = fimc_is_hw_set_field_value(val,
			&mcuctl_fields[MCUCTRLR2_FIMC_ISP0_RT_INFO], 0);

	if (sensor_cnt > 1) {
		/*
		 * DUAL scenario
		 *
		 * 2) Pixel Sync Buf, depending on Back Camera
		 *    BNS->3AA : 1 <= if Binning Scaling was applied
		 *    3AA->ISP : 0
		 */
		if (fimc_is_sensor_g_bns_ratio(&core->sensor[0]) > 1000)
			val = fimc_is_hw_set_field_value(val,
				&mcuctl_fields[MCUCTRLR2_SHARED_BUFFER_SEL], 1);
		else
			val = fimc_is_hw_set_field_value(val,
				&mcuctl_fields[MCUCTRLR2_SHARED_BUFFER_SEL], 0);

		/*
		 * 3) BNS Input Select
		 *    CSIS0 : 0 (BACK) <= Always
		 * 4) 3AA0 Input Select
		 *    CSIS0 : 0 (BNS) <= Always
		 */
		val = fimc_is_hw_set_field_value(val,
			&mcuctl_fields[MCUCTRLR2_IN_PATH_SEL_BNS], 0);
		val = fimc_is_hw_set_field_value(val,
			&mcuctl_fields[MCUCTRLR2_IN_PATH_SEL_3AA0], 0);
	} else {
		/*
		 * SINGLE scenario
		 *
		 * 2) Pixel Sync Buf
		 *    BNS->3AA : 1 <= if Binning Scaling was applied
		 *    3AA->ISP : 0
		 */
		if (fimc_is_sensor_g_bns_ratio(device->sensor) > 1000)
			val = fimc_is_hw_set_field_value(val,
				&mcuctl_fields[MCUCTRLR2_SHARED_BUFFER_SEL], 1);
		else
			val = fimc_is_hw_set_field_value(val,
				&mcuctl_fields[MCUCTRLR2_SHARED_BUFFER_SEL], 0);

		/*
		 * 3) BNS Input Select
		 *    CSIS0 : 0 (BACK)
		 *    CSIS2 : 1 (FRONT)
		 * 4) 3AA0 Input Select
		 *    CSIS0 : 0 (BNS) <= Always
		 */
		if (csi->instance == 0) {
			val = fimc_is_hw_set_field_value(val,
				&mcuctl_fields[MCUCTRLR2_IN_PATH_SEL_BNS], 0);
			val = fimc_is_hw_set_field_value(val,
				&mcuctl_fields[MCUCTRLR2_IN_PATH_SEL_3AA0], 0);
		} else {
			val = fimc_is_hw_set_field_value(val,
				&mcuctl_fields[MCUCTRLR2_IN_PATH_SEL_BNS], 1);
			val = fimc_is_hw_set_field_value(val,
				&mcuctl_fields[MCUCTRLR2_IN_PATH_SEL_3AA0], 0);
		}
	}

	/*
	 * 5) ISP1 Input Select
	 *    3AA1 : 1 <= Always
	 */
	val = fimc_is_hw_set_field_value(val,
		&mcuctl_fields[MCUCTRLR2_IN_PATH_SEL_ISP1], 1);

	/*
	 * 7) TPU Output Select
	 *    MCS0 : 0 <= Always
	 */
	val = fimc_is_hw_set_field_value(val,
		&mcuctl_fields[MCUCTRLR2_OUT_PATH_SEL_TPU], 0);

	/*
	 * 8) TPU Input Select
	 *    ISP0 : 0 <= Always
	 */
	val = fimc_is_hw_set_field_value(val,
		&mcuctl_fields[MCUCTRLR2_IN_PATH_SEL_TPU], 0);

	/*
	 * 9) MCS1 Input Select
	 *    ISP1 : 1 <= Always
	 */
	val = fimc_is_hw_set_field_value(val,
		&mcuctl_fields[MCUCTRLR2_IN_PATH_SEL_MCS1], 1);

	minfo("MCUCTL2 MUX 0x%08X", device, val);
	fimc_is_hw_set_reg(regs, &mcuctl_regs[MCUCTRLR2], val);

	return ret;
}

#ifndef ENABLE_IS_CORE
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

static irqreturn_t interface_isp_isr1(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_isp = NULL;
	struct hwip_intr_handler *intr_isp = NULL;

	itf_isp = (struct fimc_is_interface_hwip *)data;
	intr_isp = &itf_isp->handler[INTR_HWIP1];

	if (intr_isp->valid) {
		fimc_is_enter_lib_isr();
		intr_isp->handler(intr_isp->id, intr_isp->ctx);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[%d]isp(1) empty handler!!", itf_isp->id);
	}

	return IRQ_HANDLED;
}

static irqreturn_t interface_isp_isr2(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_isp = NULL;
	struct hwip_intr_handler *intr_isp = NULL;

	itf_isp = (struct fimc_is_interface_hwip *)data;
	intr_isp = &itf_isp->handler[INTR_HWIP2];

	if (intr_isp->valid) {
		fimc_is_enter_lib_isr();
		intr_isp->handler(intr_isp->id, intr_isp->ctx);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[%d]isp(2) empty handler!!", itf_isp->id);
	}

	return IRQ_HANDLED;
}

static irqreturn_t interface_tpu_isr1(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_tpu = NULL;
	struct hwip_intr_handler *intr_tpu = NULL;

	itf_tpu = (struct fimc_is_interface_hwip *)data;
	intr_tpu = &itf_tpu->handler[INTR_HWIP1];

	if (intr_tpu->valid) {
		fimc_is_enter_lib_isr();
		intr_tpu->handler(intr_tpu->id, intr_tpu->ctx);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[%d]tpu(1) empty handler!!", itf_tpu->id);
	}

	return IRQ_HANDLED;
}

static irqreturn_t interface_tpu_isr2(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_tpu = NULL;
	struct hwip_intr_handler *intr_tpu = NULL;

	itf_tpu = (struct fimc_is_interface_hwip *)data;
	intr_tpu = &itf_tpu->handler[INTR_HWIP2];

	if (intr_tpu->valid) {
		fimc_is_enter_lib_isr();
		intr_tpu->handler(intr_tpu->id, intr_tpu->ctx);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[%d]tpu(2) empty handler!!", itf_tpu->id);
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
	intr_vra = &itf_vra->handler[INTR_HWIP1];

	if (intr_vra->valid) {
		fimc_is_enter_lib_isr();
		intr_vra->handler(intr_vra->id, (void *)itf_vra->hw_ip);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[%d]vra(1) empty handler!!", itf_vra->id);
	}

	return IRQ_HANDLED;
}

static irqreturn_t interface_vra_isr2(int irq, void *data)
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
	case DEV_HW_MCSC0:
		slot_id = 5;
		break;
	case DEV_HW_MCSC1:
		slot_id = 6;
		break;
	case DEV_HW_VRA:
		slot_id = 7;
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
		break;
	case GROUP_ID_MCS0:
		hw_list[hw_index] = DEV_HW_MCSC0; hw_index++;
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

static int fimc_is_hw_get_clk_gate(struct fimc_is_hw_ip *hw_ip, int hw_id)
{
	int ret = 0;
	struct fimc_is_clk_gate *clk_gate = NULL;

	switch (hw_id) {
	case DEV_HW_3AA0:
		clk_gate = &clk_gate_3aa;
		clk_gate->regs = ioremap_nocache(0x144D081C, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
			goto p_err;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_ISP0:
		clk_gate = &clk_gate_isp;
		clk_gate->regs = ioremap_nocache(0x144D0824, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
			goto p_err;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_MCSC0:
		clk_gate = &clk_gate_isp;
		clk_gate->regs = ioremap_nocache(0x144D0824, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
			goto p_err;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_VRA:
		clk_gate = &clk_gate_vra;
		clk_gate->regs = ioremap_nocache(0x144D0810, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
			goto p_err;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	default:
		probe_err("hw_id(%d) is invalid", hw_id);
		ret = -EINVAL;
		goto p_err;
	}

	hw_ip->clk_gate = clk_gate;

p_err:
	return ret;
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

		itf_hwip->hw_ip->regs_start = mem_res->start;
		itf_hwip->hw_ip->regs_end = mem_res->end;
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res));
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] 3AA VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_3AA1:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_3AA1);
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

		info_itfc("[ID:%2d] 3AA VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_ISP0:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_ISP0);
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
		break;
	case DEV_HW_ISP1:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_ISP1);
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

		info_itfc("[ID:%2d] ISP1 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_TPU0:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_TPU);
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

		info_itfc("[ID:%2d] TPU VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_MCSC0:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_MCSC);
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

		info_itfc("[ID:%2d] MCSC0 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_MCSC1:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_MCSC);
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

		info_itfc("[ID:%2d] MCSC1 VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_VRA:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_VRA_CH0);
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

		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_VRA_CH1);
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
	default:
		probe_err("hw_id(%d) is invalid", hw_id);
		return -EINVAL;
		break;
	}

	ret = fimc_is_hw_get_clk_gate(itf_hwip->hw_ip, hw_id);;
	if (ret)
		dev_err(&pdev->dev, "fimc_is_hw_get_clk_gate is fail\n");

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
	case DEV_HW_MCSC0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 12);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq scaler\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_MCSC1:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 13);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq scaler\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_VRA:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 14);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq vra \n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 15);
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
	case DEV_HW_MCSC0:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimcmcs-0");
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
	case DEV_HW_MCSC1:
		name_len = sizeof(itf_hwip->irq_name[INTR_HWIP1]);
		snprintf(itf_hwip->irq_name[INTR_HWIP1], name_len, "fimcmcs-1");
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
#endif

int fimc_is_hw_s_ctrl(void *itfc_data, int hw_id, enum hw_s_ctrl_id id, void *val)
{
	int ret = 0;

	switch (id) {
	case HW_S_CTRL_FULL_BYPASS:
#ifndef ENABLE_IS_CORE
		{
			struct fimc_is_interface_ischain *itfc = NULL;
			unsigned long bypass = (unsigned long)val;
			u32 values = 0;
			u32 read_values = 0;

			FIMC_BUG(!itfc_data);

			itfc = (struct fimc_is_interface_ischain *)itfc_data;
			switch (hw_id) {
			case DEV_HW_TPU0:
				values = readl(itfc->regs_mcuctl + MCUCTLR);

				read_values = values & MCUCTLR_TPU_HW_BYPASS(1);
				if (bypass)
					values |= MCUCTLR_TPU_HW_BYPASS(1);
				else
					values &= ~MCUCTLR_TPU_HW_BYPASS(1);

				if  ((bypass && !read_values) || (!bypass && read_values)) {
					writel(values, itfc->regs_mcuctl + MCUCTLR);
					info_itfc("[ID:%d] Full bypass set (%lu)", hw_id, bypass);
				}
				break;
			default:
				err_itfc("[ID:%d] request_irq [2] fail", hw_id);
				ret = -EINVAL;
			}
		}
#endif
		break;
	case HW_S_CTRL_CHAIN_IRQ:
#ifndef ENABLE_IS_CORE
		{
			struct fimc_is_interface *itf = NULL;

			FIMC_BUG(!itfc_data);

			itf = (struct fimc_is_interface *)itfc_data;

			writel(0x7fff, itf->regs + INTMR2);
			info_itfc("hw_s_ctrl: 0x7fff, INTMR2");
		}
#endif
		break;
	case HW_S_CTRL_HWFC_IDX_RESET:
		if (hw_id == FIMC_IS_VIDEO_M3P_NUM) {
			struct fimc_is_video_ctx *vctx = (struct fimc_is_video_ctx *)itfc_data;
			struct fimc_is_device_ischain *device;
			unsigned long data = (unsigned long)val;

			FIMC_BUG(!vctx);
			FIMC_BUG(!GET_DEVICE(vctx));

			device = GET_DEVICE(vctx);

			/* reset if this instance is reprocessing */
			if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state)) {
				writel(data, hwfc_rst);
				minfo("HWFC reset\n", device);
			}
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
		*(bool *)val = true;
		break;
	case HW_G_CTRL_HAS_VRA_CH1_ONLY:
		*(bool *)val = false;
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
	case DEV_HW_MCSC1:
		{
			struct fimc_is_hw_mcsc_cap *cap = (struct fimc_is_hw_mcsc_cap *)cap_data;
			/* v2.10 */
			cap->hw_ver = HW_SET_VERSION(2, 0, 0, 0);
			cap->max_output = 5;
			cap->hwfc = MCSC_CAP_SUPPORT;
			cap->in_otf = MCSC_CAP_SUPPORT;
			cap->in_dma = MCSC_CAP_SUPPORT;
			cap->out_dma[0] = MCSC_CAP_SUPPORT;
			cap->out_dma[1] = MCSC_CAP_SUPPORT;
			cap->out_dma[2] = MCSC_CAP_SUPPORT;
			cap->out_dma[3] = MCSC_CAP_SUPPORT;
			cap->out_dma[4] = MCSC_CAP_SUPPORT;
			cap->out_otf[0] = MCSC_CAP_SUPPORT;
			cap->out_otf[1] = MCSC_CAP_SUPPORT;
			cap->out_otf[2] = MCSC_CAP_SUPPORT;
			cap->out_otf[3] = MCSC_CAP_SUPPORT;
			cap->out_otf[4] = MCSC_CAP_SUPPORT;
			cap->enable_shared_output = true;
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
