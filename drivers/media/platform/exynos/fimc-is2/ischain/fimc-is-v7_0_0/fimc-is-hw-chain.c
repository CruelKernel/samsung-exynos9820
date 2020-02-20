/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd
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
#include "fimc-is-hw-chain.h"
#include "fimc-is-hw-settle-8nm-lpp.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-csi.h"
#include "fimc-is-device-ischain.h"

#include "../../interface/fimc-is-interface-ischain.h"
#include "../../hardware/fimc-is-hw-control.h"
#include "../../hardware/fimc-is-hw-mcscaler-v2.h"

DEFINE_MUTEX(sysreg_isppre_lock);
static struct fimc_is_reg sysreg_isppre_regs[SYSREG_ISPPRE_REG_CNT] = {
	{0x0400, "ISPPRE_USER_CON"},
	{0x0404, "ISPPRE_SC_CON0"},
	{0x0408, "ISPPRE_SC_CON1"},
	{0x040C, "ISPPRE_SC_CON2"},
	{0x0410, "ISPPRE_SC_CON3"},
	{0x0414, "ISPPRE_SC_CON4"},
	{0x0418, "ISPPRE_SC_CON5"},
	{0x041C, "ISPPRE_SC_CON6"},
	{0x0420, "ISPPRE_SC_CON7"},
	{0x0424, "ISPPRE_SC_CON8"},
	{0x0428, "ISPPRE_SC_CON9"},
	{0x042C, "ISPPRE_SC_CON10"},
	{0x0430, "ISPPRE_SC_CON11"},
	{0x0434, "ISPPRE_SC_CON12"},
	{0x0438, "ISPPRE_SC_CON13"},
	{0x043C, "ISPPRE_SC_CON14"},
	{0x0440, "ISPPRE_SC_CON15"},
	{0x0444, "ISPPRE_SC_CON16"},
	{0x0448, "ISPPRE_SC_PDP_IN_EN"},
	{0x044C, "ISPPRE_SC_CON17"},
};

static struct fimc_is_reg sysreg_isplp_regs[SYSREG_ISPLP_REG_CNT] = {
	{0x0404, "ISPLP_USER_CON"},
	{0x0408, "ISPLP_USER_CON2"},
};

static struct fimc_is_reg sysreg_isphq_regs[SYSREG_ISPHQ_REG_CNT] = {
	{0x0404, "ISPHQ_USER_CON"},
};

static struct fimc_is_field sysreg_isppre_fields[SYSREG_ISPPRE_REG_FIELD_CNT] = {
	{"ENABLE_OTF_IN_DSPMISPPRE", 19, 1, RW, 0x1}, /* ISPPRE_USER_CON */
	{"ENABLE_OTF_OUT_ISPPREDSPM", 18, 1, RW, 0x1}, /* ISPPRE_USER_CON */
	{"ENABLE_OTF_OUT_ISPPREISPHQ", 17, 1, RW, 0x1}, /* ISPPRE_USER_CON */
	{"ENABLE_OTF_OUT_ISPPREISPLP", 16, 1, RW, 0x1}, /* ISPPRE_USER_CON */
	{"SW_RESETN_LHM_AST_GLUE_DSPMISPPRE", 15, 1, RW, 0x1}, /* ISPPRE_USER_CON */
	{"SW_RESETN_LHS_AST_GLUE_ISPPREDSPM", 14, 1, RW, 0x1}, /* ISPPRE_USER_CON */
	{"SW_RESETN_LHS_ATB_GLUE_ISPPREISPHQ", 13, 1, RW, 0x1}, /* ISPPRE_USER_CON */
	{"SW_RESETN_LHS_ATB_GLUE_ISPPREISPLP", 12, 1, RW, 0x1}, /* ISPPRE_USER_CON */
	{"TYPE_LHM_AST_GLUE_DSPMISPPRE", 11, 1, RW, 0x1}, /* ISPPRE_USER_CON */
	{"TYPE_LHS_AST_GLUE_ISPPREDSPM", 10, 1, RW, 0x1}, /* ISPPRE_USER_CON */
	{"TYPE_LHS_ATB_GLUE_ISPPREISPHQ", 9, 1, RW, 0x1}, /* ISPPRE_USER_CON */
	{"TYPE_LHS_ATB_GLUE_ISPPREISPLP", 8, 1, RW, 0x1}, /* ISPPRE_USER_CON */
	{"GLUEMUX_PDP0_VAL", 0, 2, RW, 0x3}, /* ISPPRE_SC_CON0 */
	{"GLUEMUX_PDP1_VAL", 0, 2, RW, 0x3}, /* ISPPRE_SC_CON1 */
	{"GLUEMUX_CSISX4_PDP_DMA0_OTF_SEL", 0, 3, RW, 0x7}, /* ISPPRE_SC_CON2 */
	{"GLUEMUX_CSISX4_PDP_DMA1_OTF_SEL", 0, 3, RW, 0x7}, /* ISPPRE_SC_CON3 */
	{"GLUEMUX_CSISX4_PDP_DMA2_OTF_SEL", 0, 3, RW, 0x7}, /* ISPPRE_SC_CON4 */
	{"GLUEMUX_CSISX4_PDP_DMA3_OTF_SEL", 0, 3, RW, 0x7}, /* ISPPRE_SC_CON5 */
	{"GLUEMUX_PDP_TOP_DMA0_OTF_SEL", 0, 3, RW, 0x7}, /* ISPPRE_SC_CON6 */
	{"GLUEMUX_PDP_TOP_DMA1_OTF_SEL", 0, 3, RW, 0x7}, /* ISPPRE_SC_CON7 */
	{"GLUEMUX_PDP_TOP_DMA2_OTF_SEL", 0, 3, RW, 0x7}, /* ISPPRE_SC_CON8 */
	{"GLUEMUX_PDP_TOP_DMA3_OTF_SEL", 0, 3, RW, 0x7}, /* ISPPRE_SC_CON9 */
	{"GLUEMUX_SCORE_VAL", 0, 3, RW, 0x7}, /* ISPPRE_SC_CON10 */
	{"GLUEMUX_PDP1_STALL_VAL", 0, 2, RW, 0x3}, /* ISPPRE_SC_CON11 */
	{"GLUEMUX_PDP0_STALL_VAL", 0, 2, RW, 0x3}, /* ISPPRE_SC_CON12 */
	{"GLUEMUX_3AA1_VAL", 0, 1, RW, 0x1}, /* ISPPRE_SC_CON13 */
	{"GLUEMUX_3AA0_VAL", 0, 1, RW, 0x1}, /* ISPPRE_SC_CON14 */
	{"GLUEMUX_ISPHQ_VAL", 0, 1, RW, 0x1}, /* ISPPRE_SC_CON15 */
	{"GLUEMUX_ISPLP_VAL", 0, 1, RW, 0x1}, /* ISPPRE_SC_CON16 */
	{"PDP_CORE1_IN_CSIS3_EN", 7, 1, RW, 0x1}, /* ISPPRE_SC_PDP_IN_EN */
	{"PDP_CORE1_IN_CSIS2_EN", 6, 1, RW, 0x1}, /* ISPPRE_SC_PDP_IN_EN */
	{"PDP_CORE1_IN_CSIS1_EN", 5, 1, RW, 0x1}, /* ISPPRE_SC_PDP_IN_EN */
	{"PDP_CORE1_IN_CSIS0_EN", 4, 1, RW, 0x1}, /* ISPPRE_SC_PDP_IN_EN */
	{"PDP_CORE0_IN_CSIS3_EN", 3, 1, RW, 0x1}, /* ISPPRE_SC_PDP_IN_EN */
	{"PDP_CORE0_IN_CSIS2_EN", 2, 1, RW, 0x1}, /* ISPPRE_SC_PDP_IN_EN */
	{"PDP_CORE0_IN_CSIS1_EN", 1, 1, RW, 0x1}, /* ISPPRE_SC_PDP_IN_EN */
	{"PDP_CORE0_IN_CSIS0_EN", 0, 1, RW, 0x1}, /* ISPPRE_SC_PDP_IN_EN */
	{"GLUEMUX_PAFSTAT_SEL", 0, 1, RW, 0x1}, /* ISPPRE_SC_CON17 */
};

static struct fimc_is_field sysreg_isplp_fields[SYSREG_ISPLP_REG_FIELD_CNT] = {
	{"GLUEMUX_MC_SCALER_SC1_VAL", 18, 2, RW, 0x3}, /* ISPLP_USER_CON */
	{"GLUEMUX_MC_SCALER_SC0_VAL", 16, 2, RW, 0x3}, /* ISPLP_USER_CON */
	{"AWCACHE_ISPLP1", 12, 4, RW, 0xf}, /* ISPLP_USER_CON */
	{"ARCACHE_ISPLP1", 8, 4, RW, 0xf}, /* ISPLP_USER_CON */
	{"AWCACHE_ISPLP0", 4, 4, RW, 0xf}, /* ISPLP_USER_CON */
	{"ARCACHE_ISPLP0", 0, 4, RW, 0xf}, /* ISPLP_USER_CON */
	{"ENABLE_OTF_IN_DSPMISPLP", 15, 1, RW, 0x1}, /* ISPLP_USER_CON2 */
	{"ENABLE_OTF_IN_ISPPREISPLP", 14, 1, RW, 0x1}, /* ISPLP_USER_CON2 */
	{"ENABLE_OTF_IN_ISPHQISPLP", 13, 1, RW, 0x1}, /* ISPLP_USER_CON2 */
	{"ENABLE_OTF_IN_CIPISPLP", 12, 1, RW, 0x1}, /* ISPLP_USER_CON2 */
	{"ENABLE_OTF_OUT_ISPLPDSPM", 11, 1, RW, 0x1}, /* ISPLP_USER_CON2 */
	{"SW_RESETN_LHM_AST_GLUE_DSPMISPLP", 10, 1, RW, 0x1}, /* ISPLP_USER_CON2 */
	{"SW_RESETN_LHS_AST_GLUE_ISPLPDSPM", 9, 1, RW, 0x1}, /* ISPLP_USER_CON2 */
	{"SW_RESETN_LHM_ATB_GLUE_ISPPREISPLP", 8, 1, RW, 0x1}, /* ISPLP_USER_CON2 */
	{"SW_RESETN_LHM_ATB_GLUE_ISPHQISPLP", 7, 1, RW, 0x1}, /* ISPLP_USER_CON2 */
	{"SW_RESETN_LHM_ATB_GLUE_CIPISPLP", 6, 1, RW, 0x1}, /* ISPLP_USER_CON2 */
	{"TYPE_LHM_AST_GLUE_DSPMISPLP", 5, 1, RW, 0x1}, /* ISPLP_USER_CON2 */
	{"TYPE_LHS_AST_GLUE_ISPLPDSPM", 4, 1, RW, 0x1}, /* ISPLP_USER_CON2 */
	{"TYPE_LHM_ATB_GLUE_ISPPREISPLP", 3, 1, RW, 0x1}, /* ISPLP_USER_CON2 */
	{"TYPE_LHM_ATB_GLUE_ISPHQISPLP", 2, 1, RW, 0x1}, /* ISPLP_USER_CON2 */
	{"TYPE_LHM_ATB_GLUE_CIPISPLP", 1, 1, RW, 0x1}, /* ISPLP_USER_CON2 */
	{"C2COM_SW_RESET", 0, 1, RW, 0x1}, /* ISPLP_USER_CON2 */
};

static struct fimc_is_field sysreg_isphq_fields[SYSREG_ISPHQ_REG_FIELD_CNT] = {
	{"ENABLE_OTF_IN_ISPPREISPHQ", 17, 1, RW, 0x1}, /* ISPHQ_USER_CON */
	{"ENABLE_OTF_OUT_ISPHQDSPM", 16, 1, RW, 0x1}, /* ISPHQ_USER_CON */
	{"ENABLE_OTF_OUT_ISPHQISPLP", 15, 1, RW, 0x1}, /* ISPHQ_USER_CON */
	{"SW_RESETN_LHS_AST_GLUE_ISPHQDSPM", 14, 1, RW, 0x1}, /* ISPHQ_USER_CON */
	{"SW_RESETN_LHS_ATB_GLUE_ISPHQISPLP", 13, 1, RW, 0x1}, /* ISPHQ_USER_CON */
	{"SW_RESETN_LHM_ATB_GLUE_ISPPREISPHQ", 12, 1, RW, 0x1}, /* ISPHQ_USER_CON */
	{"TYPE_LHS_AST_GLUE_ISPHQDSPM", 11, 1, RW, 0x1}, /* ISPHQ_USER_CON */
	{"TYPE_LHS_ATB_GLUE_ISPHQISPLP", 10, 1, RW, 0x1}, /* ISPHQ_USER_CON */
	{"TYPE_LHM_ATB_GLUE_ISPPREISPHQ", 9, 1, RW, 0x1}, /* ISPHQ_USER_CON */
	{"ISPHQ_SW_RESETN_C2COM", 8, 1, RW, 0x1}, /* ISPHQ_USER_CON */
	{"AWCACHE_ISPHQ", 4, 4, RW, 0xf}, /* ISPHQ_USER_CON */
	{"ARCACHE_ISPHQ", 0, 4, RW, 0xf}, /* ISPHQ_USER_CON */
};

/* Define default subdev ops if there are not used subdev IP */
const struct fimc_is_subdev_ops fimc_is_subdev_scc_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_scp_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_dis_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_dxc_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_dcp_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_dcxs_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_dcxc_ops;

void __iomem *hwfc_rst;

void fimc_is_enter_lib_isr(void)
{
	kernel_neon_begin();
}

void fimc_is_exit_lib_isr(void)
{
	kernel_neon_end();
}

void fimc_is_hw_group_init(struct fimc_is_group *group)
{
	int i;

	for (i = ENTRY_SENSOR; i < ENTRY_END; i++)
		group->subdev[i] = NULL;

	INIT_LIST_HEAD(&group->subdev_list);
}

int fimc_is_hw_group_cfg(void *group_data)
{
	int ret = 0;
	struct fimc_is_group *group;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_ischain *device;

	FIMC_BUG(!group_data);

	group = (struct fimc_is_group *)group_data;

#ifdef CONFIG_USE_SENSOR_GROUP
	if (group->slot == GROUP_SLOT_SENSOR) {
		sensor = group->sensor;
		if (!sensor) {
			err("device is NULL");
			BUG();
		}

		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_SENSOR] = &sensor->group_sensor.leader;
		group->subdev[ENTRY_SSVC0] = &sensor->ssvc0;
		group->subdev[ENTRY_SSVC1] = &sensor->ssvc1;
		group->subdev[ENTRY_SSVC2] = &sensor->ssvc2;
		group->subdev[ENTRY_SSVC3] = &sensor->ssvc3;

		list_add_tail(&sensor->group_sensor.leader.list, &group->subdev_list);
		list_add_tail(&sensor->ssvc0.list, &group->subdev_list);
		list_add_tail(&sensor->ssvc1.list, &group->subdev_list);
		list_add_tail(&sensor->ssvc2.list, &group->subdev_list);
		list_add_tail(&sensor->ssvc3.list, &group->subdev_list);
		return ret;
	}
#endif

	device = group->device;
	if (!device) {
		err("device is NULL");
		BUG();
	}

	switch (group->slot) {
	case GROUP_SLOT_PAF:
		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_PAF] = &device->group_paf.leader;

		list_add_tail(&device->group_paf.leader.list, &group->subdev_list);
		break;
	case GROUP_SLOT_3AA:
		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_3AA] = &device->group_3aa.leader;
		group->subdev[ENTRY_3AC] = &device->txc;
		group->subdev[ENTRY_3AP] = &device->txp;
		group->subdev[ENTRY_3AF] = &device->txf;
		group->subdev[ENTRY_3AG] = &device->txg;

		list_add_tail(&device->group_3aa.leader.list, &group->subdev_list);
		list_add_tail(&device->txc.list, &group->subdev_list);
		list_add_tail(&device->txp.list, &group->subdev_list);
		list_add_tail(&device->txf.list, &group->subdev_list);
		list_add_tail(&device->txg.list, &group->subdev_list);

		device->txc.param_dma_ot = PARAM_3AA_VDMA4_OUTPUT;
		device->txp.param_dma_ot = PARAM_3AA_VDMA2_OUTPUT;
		device->txf.param_dma_ot = PARAM_3AA_FDDMA_OUTPUT;
		device->txg.param_dma_ot = PARAM_3AA_MRGDMA_OUTPUT;
		break;
	case GROUP_SLOT_ISP:
		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_ISP] = &device->group_isp.leader;
		group->subdev[ENTRY_IXC] = &device->ixc;
		group->subdev[ENTRY_IXP] = &device->ixp;

		list_add_tail(&device->group_isp.leader.list, &group->subdev_list);
		list_add_tail(&device->ixc.list, &group->subdev_list);
		list_add_tail(&device->ixp.list, &group->subdev_list);
		break;
	case GROUP_SLOT_DIS:
		break;
	case GROUP_SLOT_DCP:
		break;
	case GROUP_SLOT_MCS:
		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_MCS] = &device->group_mcs.leader;
		group->subdev[ENTRY_M0P] = &device->m0p;
		group->subdev[ENTRY_M1P] = &device->m1p;
		group->subdev[ENTRY_M2P] = &device->m2p;
		group->subdev[ENTRY_M3P] = &device->m3p;
		group->subdev[ENTRY_M4P] = &device->m4p;
		group->subdev[ENTRY_M5P] = &device->m5p;

		list_add_tail(&device->group_mcs.leader.list, &group->subdev_list);
		list_add_tail(&device->m0p.list, &group->subdev_list);
		list_add_tail(&device->m1p.list, &group->subdev_list);
		list_add_tail(&device->m2p.list, &group->subdev_list);
		list_add_tail(&device->m3p.list, &group->subdev_list);
		list_add_tail(&device->m4p.list, &group->subdev_list);
		list_add_tail(&device->m5p.list, &group->subdev_list);

		device->m0p.param_dma_ot = PARAM_MCS_OUTPUT0;
		device->m1p.param_dma_ot = PARAM_MCS_OUTPUT1;
		device->m2p.param_dma_ot = PARAM_MCS_OUTPUT2;
		device->m3p.param_dma_ot = PARAM_MCS_OUTPUT3;
		device->m4p.param_dma_ot = PARAM_MCS_OUTPUT4;
		device->m5p.param_dma_ot = PARAM_MCS_OUTPUT5;
		break;
	case GROUP_SLOT_VRA:
		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_VRA] = &device->group_vra.leader;

		list_add_tail(&device->group_vra.leader.list, &group->subdev_list);
		break;
	default:
		probe_err("group slot(%d) is invalid", group->slot);
		BUG();
		break;
	}

	/* for hwfc: reset all REGION_IDX registers and outputs */
	hwfc_rst = ioremap(HWFC_INDEX_RESET_ADDR, SZ_4);

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
	case GROUP_ID_PAF0:
	case GROUP_ID_PAF1:
	case GROUP_ID_3AA0:
	case GROUP_ID_3AA1:
	case GROUP_ID_ISP0:
	case GROUP_ID_ISP1:
	case GROUP_ID_MCS0:
	case GROUP_ID_MCS1:
		leader->constraints_width = 5376;
		leader->constraints_height = 4032;
		break;
	case GROUP_ID_VRA0:
		leader->constraints_width = 640;
		leader->constraints_height = 480;
		break;
	default:
		merr("group id is invalid(%d)", group, group_id);
		break;
	}

	return ret;
}

void fimc_is_hw_camif_init(void)
{
	void __iomem *sysreg_isppre_base
		= ioremap_nocache(SYSREG_ISPPRE_BASE_ADDR, 0x4F0);

	fimc_is_hw_set_reg(sysreg_isppre_base,
			&sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON0], 0);
	fimc_is_hw_set_reg(sysreg_isppre_base,
			&sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON1], 1);

	iounmap(sysreg_isppre_base);
}

#ifdef USE_CAMIF_FIX_UP
int fimc_is_hw_camif_fix_up(struct fimc_is_device_sensor *sensor)
{
	struct fimc_is_device_csi *csi;
	u32 csi_ch, pdp_ch, actual, expected;
	enum sysreg_isppre_reg_name pdp_mux;
#if defined(SECURE_CAMERA_FACE)
	struct fimc_is_core *core;
#endif
	int ret = 0;
	void __iomem *sysreg_isppre_base = ioremap_nocache(SYSREG_ISPPRE_BASE_ADDR, 0x4F0);

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);
	if (!csi) {
		merr("failed to get CSI", sensor);
		ret = -ENODEV;
		goto err_get_csi;
	}

	csi_ch = csi->instance;
	if (csi_ch >= CSI_ID_MAX) {
		merr("invalid CSI channel(%d)", sensor, csi_ch);
		ret = -ERANGE;
		goto err_invalid_csi_ch;
	}

	if (test_bit(FIMC_IS_SENSOR_STAND_ALONE, &sensor->state))
		goto stand_alone_sensor;

	if (!sensor->ischain) {
		merr("no binded IS chain", sensor);
		ret = -ENODEV;
		goto err_no_binded_ischain;
	}

	/* check PDP mux configuration */
#if defined(SECURE_CAMERA_FACE)
	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		merr("failed to get core", sensor);
		ret = -ENODEV;
		goto err_get_core;
	}

	mutex_lock(&core->secure_state_lock);
	if (core->scenario == FIMC_IS_SCENARIO_SECURE)
		goto secure_scenario;
#endif

	pdp_ch = (sensor->ischain->group_3aa.id == GROUP_ID_3AA1) ? 1 : 0;
	pdp_mux = pdp_ch ? SYSREG_R_ISPPRE_SC_CON1 : SYSREG_R_ISPPRE_SC_CON0;

	mutex_lock(&sysreg_isppre_lock);
	actual = fimc_is_hw_get_reg(sysreg_isppre_base, &sysreg_isppre_regs[pdp_mux]);
	expected = csi_ch;
	if (csi_ch == CSI_ID_E)
		expected = 3;

	if (csi_ch != CSI_ID_D) {
		if (actual != expected) {
			fimc_is_hw_set_reg(sysreg_isppre_base, &sysreg_isppre_regs[pdp_mux], expected);

			minfo("[FIXUP]CSI(%d) --> PDP(%d)\n", sensor, csi_ch, pdp_ch);
		} else {
			minfo("[NS]CSI(%d) --> PDP(%d)\n", sensor, csi_ch, pdp_ch);
		}
	}
	mutex_unlock(&sysreg_isppre_lock);

#if defined(SECURE_CAMERA_FACE)
secure_scenario:
	mutex_unlock(&core->secure_state_lock);
err_get_core:
#endif
err_no_binded_ischain:
stand_alone_sensor:
err_invalid_csi_ch:
err_get_csi:
	iounmap(sysreg_isppre_base);

	return ret;
}

int fimc_is_hw_camif_pdp_in_enable(struct fimc_is_device_sensor *sensor, bool enable)
{
	int ret = 0;
	struct fimc_is_device_csi *csi;
	void __iomem *sysreg_isppre_base = ioremap_nocache(SYSREG_ISPPRE_BASE_ADDR, 0x4F0);
	u32 csi_ch, pdp_ch, bit_pos;
	enum sysreg_isppre_reg_field field_id;

	csi = (struct fimc_is_device_csi *) v4l2_get_subdevdata(sensor->subdev_csi);
	if (!csi) {
		merr("failed to get CSI", sensor);
		ret = -ENODEV;
		goto exit;
	}

	csi_ch = csi->instance;
	if (csi_ch >= CSI_ID_MAX) {
		merr("invalid CSI channel(%d)", sensor, csi_ch);
		ret = -ERANGE;
		goto exit;
	}

	if (test_bit(FIMC_IS_SENSOR_STAND_ALONE, &sensor->state))
		goto exit;

	if (!sensor->ischain) {
		merr("no binded IS chain", sensor);
		ret = -ENODEV;
		goto exit;
	}

	pdp_ch = (sensor->ischain->group_3aa.id == GROUP_ID_3AA1) ? 1 : 0;
	bit_pos = csi_ch;
	if (csi_ch == CSI_ID_E)
		bit_pos = 3;

	field_id = pdp_ch ? SYSREG_F_PDP_CORE1_IN_CSIS0_EN : SYSREG_F_PDP_CORE0_IN_CSIS0_EN;
	field_id -= bit_pos;

	mutex_lock(&sysreg_isppre_lock);
	fimc_is_hw_set_field(sysreg_isppre_base, &sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_PDP_IN_EN],
			&sysreg_isppre_fields[field_id], enable);
	mutex_unlock(&sysreg_isppre_lock);

	minfo("[%s]CSI(%d) --> PDP(%d)\n", sensor, enable ? "ENABLE" : "DISABLE", csi_ch, pdp_ch);

exit:
	iounmap(sysreg_isppre_base);
	return ret;
}
#endif

int fimc_is_hw_camif_cfg(void *sensor_data)
{
	int ret = 0;
	int vc;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_csi *csi;
	struct fimc_is_device_ischain *ischain;
	struct fimc_is_subdev *dma_subdev;
	struct fimc_is_sensor_cfg *sensor_cfg;
	void __iomem *isppre_reg = ioremap_nocache(SYSREG_ISPPRE_BASE_ADDR, 0x4F0);
	int pdp_ch = -1;
	u32 csi_ch;
	u32 dma_ch;
	u32 mux_val;
	enum subdev_ch_mode scm = SCM_WO_PAF_HW;
	struct fimc_is_core *core;
	enum sysreg_isppre_reg_name pdp_mux;

	FIMC_BUG(!sensor_data);

	sensor = sensor_data;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		merr("core is null\n", sensor);
		ret = -ENODEV;
		goto err_get_core;
	}

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);
	if (!csi) {
		merr("csi is null\n", sensor);
		ret = -ENODEV;
		goto err_get_csi;
	}

	csi_ch = csi->instance;
	if (csi_ch >= CSI_ID_MAX) {
		merr("CSI channel is invalid(%d)\n", sensor, csi_ch);
		ret = -ERANGE;
		goto err_invalid_csi_ch;
	}

	if (!IS_ENABLED(USE_CAMIF_FIX_UP))
		fimc_is_hw_set_reg(isppre_reg, &sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_PDP_IN_EN], 0xff);

	/* CSIS to PDP MUX */
	ischain = sensor->ischain;
	if (ischain) {
#if defined(SECURE_CAMERA_FACE)
		mutex_lock(&core->secure_state_lock);
		if (core->secure_state == FIMC_IS_STATE_UNSECURE) {
			if (core->scenario == FIMC_IS_SCENARIO_SECURE) {
				fimc_is_hw_set_reg(isppre_reg,
					&sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON0], 2);
				fimc_is_hw_set_reg(isppre_reg,
					&sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON1], 3);
				fimc_is_hw_set_reg(isppre_reg,
					&sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON9], 0);

				minfo("[S]CSI(%d) --> PDP(%d)\n", sensor, CSI_ID_C, 0);
				minfo("[S]CSI(%d) --> PDP(%d)\n", sensor, CSI_ID_E, 1);
			} else
#endif
			{
				pdp_ch = (ischain->group_3aa.id == GROUP_ID_3AA1) ? 1 : 0;
				if (!IS_ENABLED(USE_CAMIF_FIX_UP)) {
					pdp_mux = pdp_ch ? SYSREG_R_ISPPRE_SC_CON1 : SYSREG_R_ISPPRE_SC_CON0;

					if (csi_ch == CSI_ID_E) {
						fimc_is_hw_set_reg(isppre_reg,
								&sysreg_isppre_regs[pdp_mux], 3);
					} else if (csi_ch != CSI_ID_D) {
						fimc_is_hw_set_reg(isppre_reg,
								&sysreg_isppre_regs[pdp_mux], csi_ch);
					}

					minfo("[NS]CSI(%d) --> PDP(%d)\n", sensor, csi_ch, pdp_ch);
				}
			}

#if defined(SECURE_CAMERA_FACE)
		}
		mutex_unlock(&core->secure_state_lock);
#endif

	}

	sensor_cfg = sensor->cfg;
	if (!sensor_cfg) {
		merr("sensor_cfg is null\n", sensor);
		ret = -ENODEV;
		goto err_get_sensor_cfg;
	}

	if (sensor_cfg->pd_mode != PD_NONE) {
		if (pdp_ch < 0) {
			merr("PDP channel was not determined with PD mode(%d)",
						sensor, sensor_cfg->pd_mode);
			goto err_get_pdp_ch;
		}

		scm = SCM_W_PAF_HW;
	}

	/* DMA input MUX */
	for (vc = CSI_VIRTUAL_CH_0; vc < CSI_VIRTUAL_CH_MAX; vc++) {
		dma_subdev = csi->dma_subdev[vc];
		if (!dma_subdev)
			continue;

		if (!test_bit(FIMC_IS_SUBDEV_OPEN, &dma_subdev->state))
			continue;

		if (scm == SCM_W_PAF_HW) {
			if (vc == 0)
				mux_val = pdp_ch + 1;
			else
				mux_val = pdp_ch + 3;
		} else {
			mux_val = 0;
		}

		dma_ch = dma_subdev->dma_ch[scm];
		switch (dma_ch) {
		case 0:
			if (scm == SCM_WO_PAF_HW && csi_ch != CSI_ID_A) {
				merr("check your DMA channel for CSI #%d, cannot use DMA #%d",
						sensor, csi_ch, dma_ch);
				ret = -EINVAL;
				goto err_invalid_dma_ch;
			} else {
				fimc_is_hw_set_reg(isppre_reg,
					&sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON2], mux_val);
			}
			break;
		case 1:
			if (scm == SCM_WO_PAF_HW && csi_ch != CSI_ID_B) {
				merr("check your DMA channel for CSI #%d, cannot use DMA #%d",
						sensor, csi_ch, dma_ch);
				ret = -EINVAL;
				goto err_invalid_dma_ch;
			} else {
				fimc_is_hw_set_reg(isppre_reg,
					&sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON3], mux_val);
			}
			break;
		case 2:
			if (scm == SCM_WO_PAF_HW && csi_ch != CSI_ID_C) {
				merr("check your DMA channel for CSI #%d, cannot use DMA #%d",
						sensor, csi_ch, dma_ch);
				ret = -EINVAL;
				goto err_invalid_dma_ch;
			} else {
#if defined(SECURE_CAMERA_FACE)
				mutex_lock(&core->secure_state_lock);
				if (core->secure_state == FIMC_IS_STATE_UNSECURE) {
					if (core->scenario == FIMC_IS_SCENARIO_SECURE) {
						fimc_is_hw_set_reg(isppre_reg,
							&sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON5], 1);
						minfo("[S][SC_CON4](%d) --> (%d)\n", sensor, mux_val, 1);
						mux_val = 1;
					}
#endif
					fimc_is_hw_set_reg(isppre_reg,
						&sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON4], mux_val);
#if defined(SECURE_CAMERA_FACE)
				}
				mutex_unlock(&core->secure_state_lock);
#endif
			}
			break;
		case 3:
			if (scm == SCM_WO_PAF_HW && csi_ch != CSI_ID_E) {
				merr("check your DMA channel for CSI #%d, cannot use DMA #%d",
						sensor, csi_ch, dma_ch);
				ret = -EINVAL;
				goto err_invalid_dma_ch;
			} else {
#if defined(SECURE_CAMERA_FACE)
				mutex_lock(&core->secure_state_lock);
				if (core->secure_state == FIMC_IS_STATE_UNSECURE) {
					if (core->scenario == FIMC_IS_SCENARIO_SECURE) {
						fimc_is_hw_set_reg(isppre_reg,
							&sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON4], 1);
						minfo("[S][SC_CON5](%d) --> (%d)\n", sensor, mux_val, 1);
						mux_val = 1;
					}
#endif
					fimc_is_hw_set_reg(isppre_reg,
						&sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON5], mux_val);
#if defined(SECURE_CAMERA_FACE)
				}
				mutex_unlock(&core->secure_state_lock);
#endif
			}
			break;
		case 4:
			if (scm == SCM_WO_PAF_HW) {
				merr("check your DMA channel for CSI #%d, cannot use DMA #%d without PDP",
						sensor, csi_ch, dma_ch);
				ret = -EINVAL;
				goto err_invalid_dma_ch;
			} else {
				fimc_is_hw_set_reg(isppre_reg,
					&sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON6], mux_val);
			}
			break;
		case 5:
			if (scm == SCM_WO_PAF_HW) {
				merr("check your DMA channel for CSI #%d, cannot use DMA #%d without PDP",
						sensor, csi_ch, dma_ch);
				ret = -EINVAL;
				goto err_invalid_dma_ch;
			} else {
				fimc_is_hw_set_reg(isppre_reg,
					&sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON7], mux_val);
			}
			break;
		case 6:
			if (scm == SCM_WO_PAF_HW) {
				merr("check your DMA channel for CSI #%d, cannot use DMA #%d without PDP",
						sensor, csi_ch, dma_ch);
				ret = -EINVAL;
				goto err_invalid_dma_ch;
			} else {
				fimc_is_hw_set_reg(isppre_reg,
					&sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON8], mux_val);
			}
			break;
		case 7:
#if defined(SECURE_CAMERA_FACE)
			mutex_lock(&core->secure_state_lock);
			if (core->secure_state == FIMC_IS_STATE_UNSECURE) {
#endif
				if (scm == SCM_WO_PAF_HW && csi_ch != CSI_ID_D) {
					merr("check your DMA channel for CSI #%d, cannot use DMA #%d",
							sensor, csi_ch, dma_ch);
					ret = -EINVAL;
				} else {
					fimc_is_hw_set_reg(isppre_reg,
						&sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON9], mux_val);
				}
#if defined(SECURE_CAMERA_FACE)
			}
			mutex_unlock(&core->secure_state_lock);
#endif
			break;
		default:
			merr("DMA channel is invalid(%d)", sensor, dma_ch);
			ret = -ERANGE;
			goto err_invalid_dma_ch;
		}

		if (!ret)
			minfo("DMA #%d VC #%d input(%d)\n", sensor, dma_ch, vc, mux_val);
	}

err_invalid_dma_ch:
err_get_pdp_ch:
err_get_sensor_cfg:
err_invalid_csi_ch:
err_get_csi:
err_get_core:
	iounmap(isppre_reg);
	return ret;
}

int fimc_is_hw_camif_open(void *sensor_data)
{
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_csi *csi;

	FIMC_BUG(!sensor_data);

	sensor = sensor_data;
	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);

	if (csi->instance >= CSI_ID_MAX) {
		merr("CSI channel is invalid(%d)\n", sensor, csi->instance);
		return -EINVAL;
	}

	set_bit(CSIS_DMA_ENABLE, &csi->state);

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

	return 0;
}

void fimc_is_hw_ischain_qe_cfg(void)
{
	dbg_hw(2, "%s() \n", __func__);
}

int fimc_is_hw_ischain_enable(struct fimc_is_device_ischain *device)
{
	void __iomem *isppre_regs;
	void __iomem *isplp_regs;
	void __iomem *isphq_regs;
	u32 isphq_val = 0, isplp_val = 0, isppre_val = 0;
	u32 isphq_backup = 0, isplp_backup = 0, isppre_backup = 0;

	FIMC_BUG(!device);

	isppre_regs = ioremap_nocache(SYSREG_ISPPRE_BASE_ADDR, 0x1000);
	isplp_regs = ioremap_nocache(SYSREG_ISPLP_BASE_ADDR, 0x1000);
	isphq_regs = ioremap_nocache(SYSREG_ISPHQ_BASE_ADDR, 0x1000);

	isppre_val = fimc_is_hw_get_reg(isppre_regs, &sysreg_isppre_regs[SYSREG_R_ISPPRE_USER_CON]);
	isplp_val = fimc_is_hw_get_reg(isplp_regs, &sysreg_isplp_regs[SYSREG_R_ISPLP_USER_CON2]);
	isphq_val = fimc_is_hw_get_reg(isphq_regs, &sysreg_isplp_regs[SYSREG_R_ISPHQ_USER_CON]);

	isppre_backup = isppre_val;
	isplp_backup = isplp_val;
	isphq_backup = isphq_val;

	isppre_val = fimc_is_hw_set_field_value(isppre_val, &sysreg_isppre_fields[SYSREG_F_ENABLE_OTF_IN_DSPMISPPRE], 0);
	isppre_val = fimc_is_hw_set_field_value(isppre_val, &sysreg_isppre_fields[SYSREG_F_ENABLE_OTF_OUT_ISPPREDSPM], 0);
#ifdef CONFIG_SOC_EXYNOS9820_EVT0
	isppre_val = fimc_is_hw_set_field_value(isppre_val, &sysreg_isppre_fields[SYSREG_F_ENABLE_OTF_OUT_ISPPREISPLP], 1);
	isppre_val = fimc_is_hw_set_field_value(isppre_val, &sysreg_isppre_fields[SYSREG_F_ENABLE_OTF_OUT_ISPPREISPHQ], 1);

	isplp_val = fimc_is_hw_set_field_value(isplp_val, &sysreg_isplp_fields[SYSREG_F_ENABLE_OTF_IN_ISPPREISPLP], 1);
	isplp_val = fimc_is_hw_set_field_value(isplp_val, &sysreg_isplp_fields[SYSREG_F_ENABLE_OTF_IN_ISPHQISPLP], 1);
	isplp_val = fimc_is_hw_set_field_value(isplp_val, &sysreg_isplp_fields[SYSREG_F_ENABLE_OTF_IN_CIPISPLP], 0);
#endif
	isplp_val = fimc_is_hw_set_field_value(isplp_val, &sysreg_isplp_fields[SYSREG_F_ENABLE_OTF_IN_DSPMISPLP], 0);
	isplp_val = fimc_is_hw_set_field_value(isplp_val, &sysreg_isplp_fields[SYSREG_F_ENABLE_OTF_OUT_ISPLPDSPM], 0);

#ifdef CONFIG_SOC_EXYNOS9820_EVT0
	isphq_val = fimc_is_hw_set_field_value(isphq_val, &sysreg_isphq_fields[SYSREG_F_ENABLE_OTF_IN_ISPPREISPHQ], 1);
	isphq_val = fimc_is_hw_set_field_value(isphq_val, &sysreg_isphq_fields[SYSREG_F_ENABLE_OTF_OUT_ISPHQISPLP], 1);
#endif
	isphq_val = fimc_is_hw_set_field_value(isphq_val, &sysreg_isphq_fields[SYSREG_F_ENABLE_OTF_OUT_ISPHQDSPM], 0);

	if (isppre_backup != isppre_val) {
		minfo("SYSREG_R_ISPPRE_USER_CON:(0x%08X)->(0x%08X)\n", device, isppre_backup, isppre_val);
		fimc_is_hw_set_reg(isppre_regs, &sysreg_isppre_regs[SYSREG_R_ISPPRE_USER_CON], isppre_val);
	}

	if (isplp_backup != isplp_val) {
		minfo("SYSREG_R_ISPLP_USER_CON2:(0x%08X)->(0x%08X)\n", device, isplp_backup, isplp_val);
		fimc_is_hw_set_reg(isplp_regs, &sysreg_isplp_regs[SYSREG_R_ISPLP_USER_CON2], isplp_val);
	}

	if (isphq_backup != isphq_val) {
		minfo("SYSREG_R_ISPHQ_USER_CON:(0x%08X)->(0x%08X)\n", device, isphq_backup, isphq_val);
		fimc_is_hw_set_reg(isphq_regs, &sysreg_isphq_regs[SYSREG_R_ISPHQ_USER_CON], isphq_val);
	}

	iounmap(isppre_regs);
	iounmap(isplp_regs);
	iounmap(isphq_regs);

	return 0;
}

int fimc_is_hw_ischain_cfg(void *ischain_data)
{
	int ret = 0;
	struct fimc_is_device_ischain *device;

	void __iomem *isppre_regs;
	void __iomem *isplp_regs;

	u32 isplp_val = 0, isppre_val0 = 0, isppre_val1 = 0;
	u32 isplp_backup = 0, isppre_backup0 = 0, isppre_backup1 = 0;

	u32 input_isp0 = 0, input_isp1 = 1;
	u32 input_mcsc_src0 = 0, input_mcsc_src1 = 0;

	FIMC_BUG(!ischain_data);

	device = (struct fimc_is_device_ischain *)ischain_data;
	if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
		return ret;

	isppre_regs   = ioremap_nocache(SYSREG_ISPPRE_BASE_ADDR, 0x1000);
	isplp_regs = ioremap_nocache(SYSREG_ISPLP_BASE_ADDR, 0x1000);

	/*
	 * 1-1) Select 3AA0 input
	 *    PDP0 : 0 <= Default
	 *    SCORE : 1

	 * 1-2) Select 3AA1 input
	 *    PDP1 : 0 <= Default
	 *    SCORE : 1
	 */
	isppre_val0 = fimc_is_hw_get_reg(isppre_regs, &sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON14]);
	isppre_val1 = fimc_is_hw_get_reg(isppre_regs, &sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON13]);
	isppre_backup0 = isppre_val0;
	isppre_backup1 = isppre_val1;

	isppre_val0 = fimc_is_hw_set_field_value(isppre_val0, &sysreg_isppre_fields[SYSREG_F_GLUEMUX_3AA0_VAL], 0);
	isppre_val1 = fimc_is_hw_set_field_value(isppre_val1, &sysreg_isppre_fields[SYSREG_F_GLUEMUX_3AA1_VAL], 0);

	if (isppre_backup0 != isppre_val0 || isppre_backup1 != isppre_val1) {
		minfo("SYSREG_R_ISPPRE_SC_CON14:(0x%08X)->(0x%08X)\n", device, isppre_backup0, isppre_val0);
		fimc_is_hw_set_reg(isppre_regs, &sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON14], isppre_val0);

		minfo("SYSREG_R_ISPPRE_SC_CON13:(0x%08X)->(0x%08X)\n", device, isppre_backup1, isppre_val1);
		fimc_is_hw_set_reg(isppre_regs, &sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON13], isppre_val1);
	}

	/*
	 * 2-1) Select ISPLP(ISP0) input
	 *    3AA0 : 0 <= Default
	 *    3AA1 : 1

	 * 2-2) Select ISPHQ(ISP1) input
	 *    3AA0 : 0
	 *    3AA1 : 1 <= Default
	 */
	if ((device->group_3aa.id == GROUP_ID_3AA0 && device->group_isp.id == GROUP_ID_ISP0)
		|| (device->group_3aa.id == GROUP_ID_3AA1 && device->group_isp.id == GROUP_ID_ISP1)) {
		input_isp0 = 0;
		input_isp1 = 1;
	} else {
		input_isp0 = 1;
		input_isp1 = 0;
	}

	isppre_val0 = fimc_is_hw_get_reg(isppre_regs, &sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON16]);
	isppre_val1 = fimc_is_hw_get_reg(isppre_regs, &sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON15]);
	isppre_backup0 = isppre_val0;
	isppre_backup1 = isppre_val1;

	isppre_val0 = fimc_is_hw_set_field_value(isppre_val0, &sysreg_isppre_fields[SYSREG_F_GLUEMUX_ISPLP_VAL], input_isp0);
	isppre_val1 = fimc_is_hw_set_field_value(isppre_val1, &sysreg_isppre_fields[SYSREG_F_GLUEMUX_ISPHQ_VAL], input_isp1);

	if (isppre_backup0 != isppre_val0 || isppre_backup1 != isppre_val1) {
		minfo("SYSREG_R_ISPPRE_SC_CON16:(0x%08X)->(0x%08X)\n", device, isppre_backup0, isppre_val0);
		fimc_is_hw_set_reg(isppre_regs, &sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON16], isppre_val0);

		minfo("SYSREG_R_ISPPRE_SC_CON15:(0x%08X)->(0x%08X)\n", device, isppre_backup1, isppre_val1);
		fimc_is_hw_set_reg(isppre_regs, &sysreg_isppre_regs[SYSREG_R_ISPPRE_SC_CON15], isppre_val1);
	}


	/*
	 * 3-1) Select MC-Scaler SRC0 input
	 *    ISPLP -> MCSC0: 0  <= basic scenario
	 *    Score -> MCSC0 : 3
	 * 3-2) Select MC-Scaler SRC1 input
	 *    ISPHQ -> MCSC1: 0  <= basic scenario
	 *    Score -> MCSC1 : 3
	 */
	isplp_val = fimc_is_hw_get_reg(isplp_regs, &sysreg_isplp_regs[SYSREG_R_ISPLP_USER_CON]);
	isplp_backup = isplp_val;

	input_mcsc_src0 = 0;
	input_mcsc_src1 = 0;
	/* It will be changed in fimc_is_hardware_shot */
	isplp_val = fimc_is_hw_set_field_value(isplp_val,
				&sysreg_isplp_fields[SYSREG_F_GLUEMUX_MC_SCALER_SC0_VAL], input_mcsc_src0);
	isplp_val = fimc_is_hw_set_field_value(isplp_val,
				&sysreg_isplp_fields[SYSREG_F_GLUEMUX_MC_SCALER_SC1_VAL], input_mcsc_src1);

	/* 5) Write result to register */
	if (isplp_backup != isplp_val) {
		minfo("SYSREG_R_ISPLP_USER_CON:(0x%08X)->(0x%08X)\n", device, isplp_backup, isplp_val);
		fimc_is_hw_set_reg(isplp_regs, &sysreg_isplp_regs[SYSREG_R_ISPLP_USER_CON], isplp_val);	/* MCSC input */
	}

	/* 6) Write enable register: EVT0 */
	ret = fimc_is_hw_ischain_enable(device);

	iounmap(isppre_regs);
	iounmap(isplp_regs);

	return ret;
}

#ifdef ENABLE_HWACG_CONTROL
void fimc_is_hw_csi_qchannel_enable_all(bool enable)
{
	void __iomem *csi0_regs;
	void __iomem *csi1_regs;
	void __iomem *csi2_regs;
	void __iomem *csi3_regs;
	void __iomem *csi3_1_regs;

	u32 reg_val;

	csi0_regs = ioremap_nocache(CSIS0_QCH_EN_ADDR, SZ_4);
	csi1_regs = ioremap_nocache(CSIS1_QCH_EN_ADDR, SZ_4);
	csi2_regs = ioremap_nocache(CSIS2_QCH_EN_ADDR, SZ_4);
	csi3_regs = ioremap_nocache(CSIS3_QCH_EN_ADDR, SZ_4);
	csi3_1_regs = ioremap_nocache(CSIS3_1_QCH_EN_ADDR, SZ_4);

	reg_val = readl(csi0_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi0_regs);

	reg_val = readl(csi1_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi1_regs);

	reg_val = readl(csi2_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi2_regs);

	reg_val = readl(csi3_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi3_regs);

	reg_val = readl(csi3_1_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi3_1_regs);

	iounmap(csi0_regs);
	iounmap(csi1_regs);
	iounmap(csi2_regs);
	iounmap(csi3_regs);
	iounmap(csi3_1_regs);
}
#endif

static inline void fimc_is_isr1_ddk(void *data)
{
	struct fimc_is_interface_hwip *itf_hw = NULL;
	struct hwip_intr_handler *intr_hw = NULL;

	itf_hw = (struct fimc_is_interface_hwip *)data;
	intr_hw = &itf_hw->handler[INTR_HWIP1];

	if (intr_hw->valid) {
		fimc_is_enter_lib_isr();
		intr_hw->handler(intr_hw->id, intr_hw->ctx);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[ID:%d](1) empty handler!!", itf_hw->id);
	}
}

static inline void fimc_is_isr2_ddk(void *data)
{
	struct fimc_is_interface_hwip *itf_hw = NULL;
	struct hwip_intr_handler *intr_hw = NULL;

	itf_hw = (struct fimc_is_interface_hwip *)data;
	intr_hw = &itf_hw->handler[INTR_HWIP2];

	if (intr_hw->valid) {
		fimc_is_enter_lib_isr();
		intr_hw->handler(intr_hw->id, intr_hw->ctx);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[ID:%d](2) empty handler!!", itf_hw->id);
	}
}

static inline void fimc_is_isr1_host(void *data)
{
	struct fimc_is_interface_hwip *itf_hw = NULL;
	struct hwip_intr_handler *intr_hw = NULL;

	itf_hw = (struct fimc_is_interface_hwip *)data;
	intr_hw = &itf_hw->handler[INTR_HWIP1];

	if (intr_hw->valid)
		intr_hw->handler(intr_hw->id, (void *)itf_hw->hw_ip);
	else
		err_itfc("[ID:%d] empty handler!!", itf_hw->id);

}

static irqreturn_t fimc_is_isr1_3aa0(int irq, void *data)
{
	fimc_is_isr1_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr2_3aa0(int irq, void *data)
{
	fimc_is_isr2_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr1_3aa1(int irq, void *data)
{
	fimc_is_isr1_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr2_3aa1(int irq, void *data)
{
	fimc_is_isr2_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr1_vpp(int irq, void *data)
{
	fimc_is_isr1_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr2_vpp(int irq, void *data)
{
	fimc_is_isr2_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr1_isp0(int irq, void *data)
{
	fimc_is_isr1_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr2_isp0(int irq, void *data)
{
	fimc_is_isr2_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr1_isp1(int irq, void *data)
{
	fimc_is_isr1_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr2_isp1(int irq, void *data)
{
	fimc_is_isr2_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr1_mcs0(int irq, void *data)
{
	fimc_is_isr1_host(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr1_mcs1(int irq, void *data)
{
	fimc_is_isr1_host(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr1_vra(int irq, void *data)
{
	struct fimc_is_interface_hwip *itf_hw = NULL;
	struct hwip_intr_handler *intr_hw = NULL;

	itf_hw = (struct fimc_is_interface_hwip *)data;
	intr_hw = &itf_hw->handler[INTR_HWIP2];

	if (intr_hw->valid) {
		fimc_is_enter_lib_isr();
		intr_hw->handler(intr_hw->id, itf_hw->hw_ip);
		fimc_is_exit_lib_isr();
	} else {
		err_itfc("[ID:%d](2) empty handler!!", itf_hw->id);
	}

	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr1_paf0(int irq, void *data)
{
	fimc_is_isr1_host(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr1_paf1(int irq, void *data)
{
	fimc_is_isr1_host(data);
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
	case DEV_HW_ISP0: /* ISPLP */
		slot_id = 2;
		break;
	case DEV_HW_ISP1: /* ISPHQ */
		slot_id = 3;
		break;
	case DEV_HW_MCSC0:
		slot_id = 4;
		break;
	case DEV_HW_MCSC1:
		slot_id = 5;
		break;
	case DEV_HW_VRA:
		slot_id = 6;
		break;
	case DEV_HW_PAF0:
		slot_id = 7;
		break;
	case DEV_HW_PAF1:
		slot_id = 8;
		break;
	case DEV_HW_VPP:
		slot_id = 9;
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
	case GROUP_ID_PAF0:
		hw_list[hw_index] = DEV_HW_PAF0; hw_index++;
		break;
	case GROUP_ID_PAF1:
		hw_list[hw_index] = DEV_HW_PAF1; hw_index++;
		break;
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
	case GROUP_ID_MCS0:
		hw_list[hw_index] = DEV_HW_MCSC0; hw_index++;
		break;
	case GROUP_ID_MCS1:
		hw_list[hw_index] = DEV_HW_MCSC1; hw_index++;
		break;
	case GROUP_ID_VRA0:
		hw_list[hw_index] = DEV_HW_VRA; hw_index++;
		break;
	case GROUP_ID_3AA2:
		hw_list[hw_index] = DEV_HW_VPP; hw_index++;
		break;
	default:
		break;
	}

	return hw_index;
}

static int fimc_is_hw_get_clk_gate(struct fimc_is_hw_ip *hw_ip, int hw_id)
{
	if (!hw_ip) {
		probe_err("hw_id(%d) hw_ip(NULL)", hw_id);
		return -EINVAL;
	}

	hw_ip->clk_gate_idx = 0;
	hw_ip->clk_gate = NULL;

	return 0;
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
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_ISPLP);
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
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_ISPHQ);
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
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_VRA);
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

		info_itfc("[ID:%2d] VRA VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);

		break;
	case DEV_HW_PAF0:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_PAF_RDMA);
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

		info_itfc("[ID:%2d] PAF0 RD VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);

		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_PAF_CORE);
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

		info_itfc("[ID:%2d] PAF0 COMMON VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs_b);
		break;
	case DEV_HW_PAF1:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_PAF_RDMA);
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

		info_itfc("[ID:%2d] PAF1 RD VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);

		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_PAF_CORE);
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

		info_itfc("[ID:%2d] PAF1 COMMON VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs_b);
		break;
	case DEV_HW_VPP:
		mem_res = platform_get_resource(pdev, IORESOURCE_MEM, IORESOURCE_3AA2);
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

		info_itfc("[ID:%2d] VPP VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
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
		break;
	case DEV_HW_3AA1:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 2);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq 3aa1-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 3);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq 3aa0-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_ISP0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 4);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq isp0-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 5);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq isp0-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_ISP1:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 6);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq isp1-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 7);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq isp1-2\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_MCSC0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 8);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq scaler\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_MCSC1:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 9);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq scaler\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_VRA:
		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 10);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq vra \n");
			return -EINVAL;
		}
		break;
	case DEV_HW_PAF0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 11);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq PAF0\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_PAF1:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 12);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq PAF1\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_VPP:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 13);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq 3aa1-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 14);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq 3aa0-2\n");
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

static inline int __fimc_is_hw_request_irq(struct fimc_is_interface_hwip *itf_hwip,
	const char *name, int isr_num, irqreturn_t (*func)(int, void *))
{
	size_t name_len = 0;
	int ret = 0;

	name_len = sizeof(itf_hwip->irq_name[isr_num]);
	snprintf(itf_hwip->irq_name[isr_num], name_len, "fimc%s-%d", name, isr_num);
	ret = request_irq(itf_hwip->irq[isr_num], func,
		FIMC_IS_HW_IRQ_FLAG,
		itf_hwip->irq_name[isr_num],
		itf_hwip);
	if (ret) {
		err_itfc("[HW:%s] request_irq [%d] fail", name, isr_num);
		return -EINVAL;
	}
	itf_hwip->handler[isr_num].id = isr_num;
	itf_hwip->handler[isr_num].valid = true;

	return ret;
}

int fimc_is_hw_request_irq(void *itfc_data, int hw_id)
{
	struct fimc_is_interface_hwip *itf_hwip = NULL;
	int ret = 0;

	FIMC_BUG(!itfc_data);


	itf_hwip = (struct fimc_is_interface_hwip *)itfc_data;

	switch (hw_id) {
	case DEV_HW_3AA0:
		ret = __fimc_is_hw_request_irq(itf_hwip, "3aa0", INTR_HWIP1, fimc_is_isr1_3aa0);
		ret = __fimc_is_hw_request_irq(itf_hwip, "3aa0", INTR_HWIP2, fimc_is_isr2_3aa0);
		break;
	case DEV_HW_3AA1:
		ret = __fimc_is_hw_request_irq(itf_hwip, "3aa1", INTR_HWIP1, fimc_is_isr1_3aa1);
		ret = __fimc_is_hw_request_irq(itf_hwip, "3aa1", INTR_HWIP2, fimc_is_isr2_3aa1);
		break;
	case DEV_HW_ISP0:
		ret = __fimc_is_hw_request_irq(itf_hwip, "isp0", INTR_HWIP1, fimc_is_isr1_isp0);
		ret = __fimc_is_hw_request_irq(itf_hwip, "isp0", INTR_HWIP2, fimc_is_isr2_isp0);
		break;
	case DEV_HW_ISP1:
		ret = __fimc_is_hw_request_irq(itf_hwip, "isp1", INTR_HWIP1, fimc_is_isr1_isp1);
		ret = __fimc_is_hw_request_irq(itf_hwip, "isp1", INTR_HWIP2, fimc_is_isr2_isp1);
		break;
	case DEV_HW_MCSC0:
		ret = __fimc_is_hw_request_irq(itf_hwip, "mcs0", INTR_HWIP1, fimc_is_isr1_mcs0);
		break;
	case DEV_HW_MCSC1:
		ret = __fimc_is_hw_request_irq(itf_hwip, "mcs1", INTR_HWIP1, fimc_is_isr1_mcs1);
		break;
	case DEV_HW_VRA:
		ret = __fimc_is_hw_request_irq(itf_hwip, "vra", INTR_HWIP2, fimc_is_isr1_vra);
		break;
	case DEV_HW_PAF0:
		ret = __fimc_is_hw_request_irq(itf_hwip, "paf0", INTR_HWIP1, fimc_is_isr1_paf0);
		break;
	case DEV_HW_PAF1:
		ret = __fimc_is_hw_request_irq(itf_hwip, "paf1", INTR_HWIP1, fimc_is_isr1_paf1);
		break;
	case DEV_HW_VPP:
		ret = __fimc_is_hw_request_irq(itf_hwip, "vpp", INTR_HWIP1, fimc_is_isr1_vpp);
		ret = __fimc_is_hw_request_irq(itf_hwip, "vpp", INTR_HWIP2, fimc_is_isr2_vpp);
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
		break;
	case HW_S_CTRL_CHAIN_IRQ:
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
			if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
				writel(data, hwfc_rst);
		}
		break;
	case HW_S_CTRL_MCSC_SET_INPUT:
		{
			unsigned long mode = (unsigned long)val;

			info_itfc("%s: mode(%lu)\n", __func__, mode);
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
	case DEV_HW_MCSC1:
		{
			struct fimc_is_hw_mcsc_cap *cap = (struct fimc_is_hw_mcsc_cap *)cap_data;

			cap->hw_ver = HW_SET_VERSION(5, 0, 0, 0);
			cap->max_output = 6;
			cap->max_djag = 1;
			cap->max_cac = 1;
			cap->max_uvsp = 2;
			cap->hwfc = MCSC_CAP_SUPPORT;
			cap->in_otf = MCSC_CAP_SUPPORT;
			cap->in_dma = MCSC_CAP_SUPPORT;
			cap->out_dma[0] = MCSC_CAP_SUPPORT;
			cap->out_dma[1] = MCSC_CAP_SUPPORT;
			cap->out_dma[2] = MCSC_CAP_SUPPORT;
			cap->out_dma[3] = MCSC_CAP_SUPPORT;
			cap->out_dma[4] = MCSC_CAP_SUPPORT;
			cap->out_dma[5] = MCSC_CAP_SUPPORT;
			cap->out_otf[0] = MCSC_CAP_NOT_SUPPORT;
			cap->out_otf[1] = MCSC_CAP_NOT_SUPPORT;
			cap->out_otf[2] = MCSC_CAP_NOT_SUPPORT;
			cap->out_otf[3] = MCSC_CAP_NOT_SUPPORT;
			cap->out_otf[4] = MCSC_CAP_NOT_SUPPORT;
			cap->out_otf[5] = MCSC_CAP_NOT_SUPPORT;
			cap->out_hwfc[3] = MCSC_CAP_SUPPORT;
			cap->out_hwfc[4] = MCSC_CAP_SUPPORT;
			cap->enable_shared_output = true;
			cap->tdnr = MCSC_CAP_NOT_SUPPORT;
			cap->djag = MCSC_CAP_SUPPORT;
#ifdef CONFIG_SOC_EXYNOS9820_EVT0
			cap->cac = MCSC_CAP_NOT_SUPPORT;
			cap->uvsp = MCSC_CAP_NOT_SUPPORT;
#else
			cap->cac = MCSC_CAP_SUPPORT;
			cap->uvsp = MCSC_CAP_NOT_SUPPORT;       /* UVSP was substitued by active pedestal @2018.10.11 */
#endif
			cap->ysum = MCSC_CAP_NOT_SUPPORT;
			cap->ds_vra = MCSC_CAP_SUPPORT;
		}
		break;
	default:
		break;
	}

	return ret;
}

void __iomem *fimc_is_hw_get_sysreg(ulong core_regs)
{
	if (core_regs)
		err_itfc("%s: core_regs(%p)\n", __func__, (void *)core_regs);

	/* deprecated */

	return NULL;
}

u32 fimc_is_hw_find_settle(u32 mipi_speed)
{
	u32 align_mipi_speed;
	u32 find_mipi_speed;
	size_t max;
	int s, e, m;

	align_mipi_speed = ALIGN(mipi_speed, 10);
	max = sizeof(fimc_is_csi_settle_table) / sizeof(u32);

	s = 0;
	e = max - 2;

	if (fimc_is_csi_settle_table[s] < align_mipi_speed)
		return fimc_is_csi_settle_table[s + 1];

	if (fimc_is_csi_settle_table[e] > align_mipi_speed)
		return fimc_is_csi_settle_table[e + 1];

	/* Binary search */
	while (s <= e) {
		m = ALIGN((s + e) / 2, 2);
		find_mipi_speed = fimc_is_csi_settle_table[m];

		if (find_mipi_speed == align_mipi_speed)
			break;
		else if (find_mipi_speed > align_mipi_speed)
			s = m + 2;
		else
			e = m - 2;
	}

	return fimc_is_csi_settle_table[m + 1];
}

unsigned int get_dma(struct fimc_is_device_sensor *device, u32 *dma_ch)
{
	*dma_ch = 0;

	return 0;
}

void fimc_is_hw_djag_get_input(struct fimc_is_device_ischain *ischain, u32 *djag_in)
{
	struct fimc_is_global_param *g_param;
	bool reprocessing, change_in;
	u32 new_in = *djag_in;

	if (!ischain) {
		err_hw("device is NULL");
		return;
	}

	g_param = &ischain->resourcemgr->global_param;
	reprocessing = test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &ischain->state);

	dbg_hw(2, "%s:video_mode %d reprocessing %d\n", __func__,
			g_param->video_mode, reprocessing);

	/* When it is video mode, DJAG should work for preview(video) stream. */
	if (g_param->video_mode)
		change_in = reprocessing;
	/* Otherwise, put DJAG into reprocessing stream. */
	else
		change_in = !reprocessing;

	if (change_in)
		new_in = (*djag_in == DEV_HW_MCSC1) ? DEV_HW_MCSC0 : DEV_HW_MCSC1;

	*djag_in = new_in;
}

void fimc_is_hw_djag_adjust_out_size(struct fimc_is_device_ischain *ischain,
					u32 in_width, u32 in_height,
					u32 *out_width, u32 *out_height)
{
	struct fimc_is_global_param *g_param;
	int bratio;
	bool is_down_scale;

	if (!ischain) {
		err_hw("device is NULL");
		return;
	}

	g_param = &ischain->resourcemgr->global_param;
	is_down_scale = (*out_width < in_width) || (*out_height < in_height);
	bratio = fimc_is_sensor_g_bratio(ischain->sensor);
	if (bratio < 0) {
		err_hw("failed to get sensor_bratio");
		return;
	}

	dbg_hw(2, "%s:video_mode %d is_down_scale %d bratio %d\n", __func__,
			g_param->video_mode, is_down_scale, bratio);

	if (g_param->video_mode
		&& is_down_scale
		&& bratio >= MCSC_DJAG_ENABLE_SENSOR_BRATIO) {
		dbg_hw(2, "%s:%dx%d -> %dx%d\n", __func__,
				*out_width, *out_height, in_width, in_height);

		*out_width = in_width;
		*out_height = in_height;
	}
}
