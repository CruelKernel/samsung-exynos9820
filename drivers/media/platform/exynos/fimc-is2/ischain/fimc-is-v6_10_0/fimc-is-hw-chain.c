/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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
#include <linux/smc.h>

#include <asm/neon.h>

#include "fimc-is-config.h"
#include "fimc-is-param.h"
#include "fimc-is-type.h"
#include "fimc-is-regs.h"
#include "fimc-is-core.h"
#include "fimc-is-hw-chain.h"
#include "fimc-is-hw-settle-10nm-lpe.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-csi.h"
#include "fimc-is-device-ischain.h"

#include "../../interface/fimc-is-interface-ischain.h"
#include "../../hardware/fimc-is-hw-control.h"
#include "../../hardware/fimc-is-hw-mcscaler-v2.h"
#ifdef ENABLE_FULLCHAIN_OVERFLOW_RECOVERY
#include "../../sensor/module_framework/fimc-is-device-sensor-peri.h"
#include "../../sensor/module_framework/pafstat/fimc-is-pafstat.h"
#endif

#if defined(CONFIG_SECURE_CAMERA_USE)
#define itfc_to_core(x) \
	container_of(x, struct fimc_is_core, interface_ischain)
#endif

enum sysreg_cam_reg_field {
	SYSREG_CAM_F_GLUEMUX_PAF_DMA3_OTF_SEL,
	SYSREG_CAM_F_GLUEMUX_PAF_DMA2_OTF_SEL,
	SYSREG_CAM_F_GLUEMUX_PAF_DMA1_OTF_SEL,
	SYSREG_CAM_F_GLUEMUX_PAF_DMA0_OTF_SEL,
	SYSREG_CAM_F_MUX_3AA1_VAL,
	SYSREG_CAM_F_MUX_3AA0_VAL,
	SYSREG_CAM_F_CSIS2_DPHY_S_MUXSEL,
	SYSREG_CAM_F_CSIS1_DPHY_S_MUXSEL,
	SYSREG_CAM_F_CSIS0_DPHY_S_MUXSEL,
	SYSREG_CAM_F_PAFSTAT_CORE1_IN_CSIS3_EN,
	SYSREG_CAM_F_PAFSTAT_CORE1_IN_CSIS2_EN,
	SYSREG_CAM_F_PAFSTAT_CORE1_IN_CSIS1_EN,
	SYSREG_CAM_F_PAFSTAT_CORE1_IN_CSIS0_EN,
	SYSREG_CAM_F_PAFSTAT_CORE0_IN_CSIS3_EN,
	SYSREG_CAM_F_PAFSTAT_CORE0_IN_CSIS2_EN,
	SYSREG_CAM_F_PAFSTAT_CORE0_IN_CSIS1_EN,
	SYSREG_CAM_F_PAFSTAT_CORE0_IN_CSIS0_EN,
	SYSREG_CAM_REG_FIELD_CNT
};

static struct fimc_is_field sysreg_cam_fields[SYSREG_CAM_REG_FIELD_CNT] = {
	{"GLUEMUX_PAF_DMA3_OTF_SEL",	21,	2,	RW,	0x0},
	{"GLUEMUX_PAF_DMA2_OTF_SEL",	19,	2,	RW,	0x2},
	{"GLUEMUX_PAF_DMA1_OTF_SEL",	17,	2,	RW,	0x3},
	{"GLUEMUX_PAF_DMA0_OTF_SEL",	15,	2,	RW,	0x2},
	{"MUX_3AA1_VA",			13,	2,	RW,	0x1},
	{"MUX_3AA0_VA",			11,	2,	RW,	0x0},
	{"CSIS2_DPHY_S_MUXSEL",		10,	1,	RW,	0x0},
	{"CSIS1_DPHY_S_MUXSEL",		9,	1,	RW,	0x0},
	{"CSIS0_DPHY_S_MUXSEL",		8,	1,	RW,	0x0},
	{"PAFSTAT_CORE1_IN_CSIS3_EN",	7,	1,	RW,	0x0},
	{"PAFSTAT_CORE1_IN_CSIS2_EN",	6,	1,	RW,	0x1},
	{"PAFSTAT_CORE1_IN_CSIS1_EN",	5,	1,	RW,	0x1},
	{"PAFSTAT_CORE1_IN_CSIS0_EN",	4,	1,	RW,	0x1},
	{"PAFSTAT_CORE1_IN_CSIS3_EN",	3,	1,	RW,	0x0},
	{"PAFSTAT_CORE1_IN_CSIS2_EN",	2,	1,	RW,	0x1},
	{"PAFSTAT_CORE1_IN_CSIS1_EN",	1,	1,	RW,	0x1},
	{"PAFSTAT_CORE1_IN_CSIS0_EN",	0,	1,	RW,	0x1},
};

/*
 * [22:21] GLUEMUX_PAF_DMA3_OTF_SEL[1:0] :  0 = DMA0, 1 = DMA1, 2 = DMA2, 3 = DMA3
 * [20:19] GLUEMUX_PAF_DMA2_OTF_SEL[1:0] :  0 = DMA0, 1 = DMA1, 2 = DMA2, 3 = DMA3
 * [18:17] GLUEMUX_PAF_DMA1_OTF_SEL[1:0] :  0 = DMA0, 1 = DMA1, 2 = DMA2, 3 = DMA3
 * [16:15] GLUEMUX_PAF_DMA0_OTF_SEL[1:0] :  0 = DMA0, 1 = DMA1, 2 = DMA2, 3 = DMA3
 * [14:13] MUX_3AA1_VAL[1:0] :  0 = CSIS0, 1 = CSIS1, 2 = CSIS2, 3 = CSIS3
 * [12:11] MUX_3AA0_VAL[1:0] :  0 = CSIS0, 1 = CSIS1, 2 = CSIS2, 3 = CSIS3
 * [10] CSIS2_DPHY_S_MUXSEL :  0 : S4(2) of M4S4S4, 1: S4(2) of M2S4S4S2
 * [9] CSIS1_DPHY_S_MUXSEL :  0 : S4(2) of M4S4S4, 1: S4(2) of M2S4S4S2
 * [8] CSIS0_DPHY_S_MUXSEL :  0 : S4(2) of M4S4S4, 1: S4(2) of M2S4S4S2
 * [7] I_PAFSTAT_CORE1_IN_CSIS3_EN
 * [6] I_PAFSTAT_CORE1_IN_CSIS2_EN
 * [5] I_PAFSTAT_CORE1_IN_CSIS1_EN
 * [4] I_PAFSTAT_CORE1_IN_CSIS0_EN
 * [3] I_PAFSTAT_CORE0_IN_CSIS3_EN
 * [2] I_PAFSTAT_CORE0_IN_CSIS2_EN
 * [1] I_PAFSTAT_CORE0_IN_CSIS1_EN
 * [0] I_PAFSTAT_CORE0_IN_CSIS0_EN
 */
#define MUX_SET_VAL_DEFAULT		(0x00402077)
#define MUX_CLR_VAL_DEFAULT		(0x007FFFFF)

/* Define default subdev ops if there are not used subdev IP */
const struct fimc_is_subdev_ops fimc_is_subdev_scc_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_scp_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_dis_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_dxc_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_dcp_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_dcxs_ops;
const struct fimc_is_subdev_ops fimc_is_subdev_dcxc_ops;

struct fimc_is_clk_gate clk_gate_3aa0;
struct fimc_is_clk_gate clk_gate_3aa1;
struct fimc_is_clk_gate clk_gate_isp0;
struct fimc_is_clk_gate clk_gate_mcsc;
struct fimc_is_clk_gate clk_gate_vra;

void __iomem *hwfc_rst;
void __iomem *isp_lhm_atb_glue_rst;

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
	int entry = 0;

	for (entry = 0; entry < ENTRY_END; entry++)
		group->subdev[entry] = NULL;

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

		list_add_tail(&device->group_3aa.leader.list, &group->subdev_list);
		list_add_tail(&device->txc.list, &group->subdev_list);
		list_add_tail(&device->txp.list, &group->subdev_list);

		device->txc.param_dma_ot = PARAM_3AA_VDMA4_OUTPUT;
		device->txp.param_dma_ot = PARAM_3AA_VDMA2_OUTPUT;
		break;
	case GROUP_SLOT_ISP:
		fimc_is_hw_group_init(group);
		group->subdev[ENTRY_ISP] = &device->group_isp.leader;
		group->subdev[ENTRY_IXC] = &device->ixc;
		group->subdev[ENTRY_IXP] = &device->ixp;
		group->subdev[ENTRY_MEXC] = &device->mexc;

		list_add_tail(&device->group_isp.leader.list, &group->subdev_list);
		list_add_tail(&device->ixc.list, &group->subdev_list);
		list_add_tail(&device->ixp.list, &group->subdev_list);
		list_add_tail(&device->mexc.list, &group->subdev_list);
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
		group->subdev[ENTRY_DNR] = &device->dnr;

		list_add_tail(&device->group_mcs.leader.list, &group->subdev_list);
		list_add_tail(&device->m0p.list, &group->subdev_list);
		list_add_tail(&device->m1p.list, &group->subdev_list);
		list_add_tail(&device->m2p.list, &group->subdev_list);
		list_add_tail(&device->m3p.list, &group->subdev_list);
		list_add_tail(&device->dnr.list, &group->subdev_list);

		device->m0p.param_dma_ot = PARAM_MCS_OUTPUT0;
		device->m1p.param_dma_ot = PARAM_MCS_OUTPUT1;
		device->m2p.param_dma_ot = PARAM_MCS_OUTPUT2;
		device->m3p.param_dma_ot = PARAM_MCS_OUTPUT3;
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

	/* for overflow recovery: reset ISP Block */
	isp_lhm_atb_glue_rst = ioremap(SYSREG_ISP_LHM_ATB_GLUE_ADDR, SZ_4);

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
		leader->constraints_height = 4320;
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
}

int fimc_is_hw_camif_cfg(void *sensor_data)
{
	int ret = 0;
	unsigned long value = 0;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_csi *csi;
	struct fimc_is_device_ischain *ischain;
	u32 paf_ch = 0;
	u32 csi_ch = 0;
	u32 mux_set_val = MUX_SET_VAL_DEFAULT;
	u32 mux_clr_val = MUX_CLR_VAL_DEFAULT;
	unsigned long mux_backup_val = 0;

	FIMC_BUG(!sensor_data);

	sensor = (struct fimc_is_device_sensor *)sensor_data;

	ischain = sensor->ischain;
	if (!ischain)
		goto p_err;

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);
	if (!csi) {
		merr("csi is null\n", sensor);
		ret = -ENODEV;
		goto p_err;
	}

	csi_ch = csi->instance;
	if (csi_ch > CSI_ID_MAX) {
		merr("CSI channel is invalid(%d)\n", sensor, csi_ch);
		ret = -ERANGE;
		goto p_err;
	}

	mutex_lock(&ischain->resourcemgr->sysreg_lock);

	/* read previous value */
	exynos_smc_readsfr((TZPC_CAM + TZPC_DECPROT1STAT_OFFSET), &mux_backup_val);

	/* CSIS to PDP MUX */
	paf_ch = (ischain->group_3aa.id == GROUP_ID_3AA1) ? 1 : 0;

	switch (paf_ch) {
	case 0:
		mux_set_val = fimc_is_hw_set_field_value(mux_set_val,
				&sysreg_cam_fields[SYSREG_CAM_F_MUX_3AA0_VAL], csi_ch);
		mux_set_val = fimc_is_hw_set_field_value(mux_set_val,
				&sysreg_cam_fields[SYSREG_CAM_F_MUX_3AA1_VAL], ((csi_ch == 0) ? 2 : 0));
		break;
	case 1:
		mux_set_val = fimc_is_hw_set_field_value(mux_set_val,
				&sysreg_cam_fields[SYSREG_CAM_F_MUX_3AA0_VAL], ((csi_ch == 0) ? 1 : 0));
		mux_set_val = fimc_is_hw_set_field_value(mux_set_val,
				&sysreg_cam_fields[SYSREG_CAM_F_MUX_3AA1_VAL], csi_ch);
		break;
	default:
		merr("PDP channel is invalid(%d)", sensor, paf_ch);
		ret = -ERANGE;
		goto p_err_lock;
	}
	minfo("CSI(%d) --> PAFSTAT(%d), mux(0x%08X), backup(0x%08lX)\n",
		sensor, csi_ch, paf_ch, mux_set_val, mux_backup_val);

	if (mux_backup_val != (unsigned long)mux_set_val) {
		/* SMC call for PAFSTAT MUX */
		ret = exynos_smc(SMC_CMD_REG, SMC_REG_ID_SFR_W(TZPC_CAM + TZPC_DECPROT1CLR_OFFSET), mux_clr_val, 0);
		if (ret)
			err("CAMIF mux clear is Error(%x)\n", ret);

		ret = exynos_smc(SMC_CMD_REG, SMC_REG_ID_SFR_W(TZPC_CAM + TZPC_DECPROT1SET_OFFSET), mux_set_val, 0);
		if (ret)
			err("CAMIF mux set is Error(%x)\n", ret);

		exynos_smc_readsfr((TZPC_CAM + TZPC_DECPROT1STAT_OFFSET), &value);
		info_hw("CAMIF mux status(0x%x) is (0x%lx)\n", (TZPC_CAM + TZPC_DECPROT1STAT_OFFSET), value);
	}

p_err_lock:
	mutex_unlock(&ischain->resourcemgr->sysreg_lock);

p_err:
	return ret;
}

int fimc_is_hw_camif_open(void *sensor_data)
{
	int ret = 0;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_csi *csi;

	FIMC_BUG(!sensor_data);

	sensor = sensor_data;
	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);

	switch (csi->instance) {
	case 0:
	case 1:
	case 2:
	case 3:
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
		break;
	default:
		merr("sensor id is invalid(%d)", sensor, csi->instance);
		break;
	}

	return ret;
}

void fimc_is_hw_ischain_qe_cfg(void)
{
	dbg_hw(2, "%s() \n", __func__);
}

int fimc_is_hw_ischain_cfg(void *ischain_data)
{
	int ret = 0;
	/* TBD */

	return ret;
}

#ifdef ENABLE_HWACG_CONTROL
void fimc_is_hw_csi_qchannel_enable_all(bool enable)
{
	void __iomem *csi0_regs;
	void __iomem *csi1_regs;
	void __iomem *csi2_regs;
#if !defined(CONFIG_SECURE_CAMERA_USE)
	void __iomem *csi3_regs;
#endif
	u32 reg_val;

	csi0_regs = ioremap_nocache(CSIS0_QCH_EN_ADDR, SZ_4);
	csi1_regs = ioremap_nocache(CSIS1_QCH_EN_ADDR, SZ_4);
	csi2_regs = ioremap_nocache(CSIS2_QCH_EN_ADDR, SZ_4);
#if !defined(CONFIG_SECURE_CAMERA_USE)
	csi3_regs = ioremap_nocache(CSIS3_QCH_EN_ADDR, SZ_4);
#endif

	reg_val = readl(csi0_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi0_regs);

	reg_val = readl(csi1_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi1_regs);

	reg_val = readl(csi2_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi2_regs);

#if !defined(CONFIG_SECURE_CAMERA_USE)
	reg_val = readl(csi3_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi3_regs);
#endif

	iounmap(csi0_regs);
	iounmap(csi1_regs);
	iounmap(csi2_regs);
#if !defined(CONFIG_SECURE_CAMERA_USE)
	iounmap(csi3_regs);
#endif
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

static irqreturn_t fimc_is_isr1_isp(int irq, void *data)
{
	fimc_is_isr1_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr2_isp(int irq, void *data)
{
	fimc_is_isr2_ddk(data);
	return IRQ_HANDLED;
}

static irqreturn_t fimc_is_isr1_mcs(int irq, void *data)
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
	case DEV_HW_ISP0:
		slot_id = 2;
		break;
	case DEV_HW_MCSC0:
		slot_id = 3;
		break;
	case DEV_HW_VRA:
		slot_id = 4;
		break;
	case DEV_HW_PAF0:
		slot_id = 5;
		break;
	case DEV_HW_PAF1:
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
	case GROUP_ID_MCS0:
		hw_list[hw_index] = DEV_HW_MCSC0; hw_index++;
		break;
	case GROUP_ID_VRA0:
		hw_list[hw_index] = DEV_HW_VRA; hw_index++;
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
		clk_gate = &clk_gate_3aa0;
		clk_gate->regs = ioremap_nocache(0x144B0084, 0x4);	/*Q-channel protocol related register*/
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_3AA1:
		clk_gate = &clk_gate_3aa1;
		clk_gate->regs = ioremap_nocache(0x144B4084, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_ISP0:
		clk_gate = &clk_gate_isp0;
		clk_gate->regs = ioremap_nocache(0x14600094, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_MCSC0:
		clk_gate = &clk_gate_mcsc;
		clk_gate->regs = ioremap_nocache(0x14640004, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
		}
		/* TODO : check bit :QACTIVE_ENABLE bit is [1] */

		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_VRA:
		clk_gate = &clk_gate_vra;
		clk_gate->regs = ioremap_nocache(0x1461b0dc, 0x4);
		if (!clk_gate->regs) {
			probe_err("Failed to remap clk_gate regs\n");
			ret = -ENOMEM;
		}
		hw_ip->clk_gate_idx = 0;
		clk_gate->bit[hw_ip->clk_gate_idx] = 0;
		clk_gate->refcnt[hw_ip->clk_gate_idx] = 0;

		spin_lock_init(&clk_gate->slock);
		break;
	case DEV_HW_PAF0:
	case DEV_HW_PAF1:
		break;
	default:
		probe_err("hw_id(%d) is invalid", hw_id);
		ret = -EINVAL;
	}

	hw_ip->clk_gate = clk_gate;

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
		itf_hwip->hw_ip->regs = ioremap_nocache(mem_res->start, resource_size(mem_res)) + LIC_3AA1_OFFSET_ADDR;
		if (!itf_hwip->hw_ip->regs) {
			dev_err(&pdev->dev, "Failed to remap io region\n");
			return -EINVAL;
		}

		info_itfc("[ID:%2d] 3AA VA(0x%p)\n", hw_id, itf_hwip->hw_ip->regs);
		break;
	case DEV_HW_ISP0:
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
	case DEV_HW_3AA0: /* 3AAW */
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
			err("Failed to get irq 3aa1-2\n");
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
	case DEV_HW_MCSC0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 6);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq scaler\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_VRA:
		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 7);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq vra \n");
			return -EINVAL;
		}
		break;
	case DEV_HW_PAF0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 8);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq PAF0\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_PAF1:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 9);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq PAF1\n");
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

//#define DECLARE_FUNC_NAME(NUM, NAME) fimc_is_isr##NUM_##NAME
static inline int __fimc_is_hw_request_irq(struct fimc_is_interface_hwip *itf_hwip,
	const char *name,
	int isr_num,
	unsigned int added_irq_flags,
	irqreturn_t (*func)(int, void *))
{
	size_t name_len = 0;
	int ret = 0;

	name_len = sizeof(itf_hwip->irq_name[isr_num]);
	snprintf(itf_hwip->irq_name[isr_num], name_len, "fimc%s-%d", name, isr_num);
	ret = request_irq(itf_hwip->irq[isr_num], func,
		FIMC_IS_HW_IRQ_FLAG | added_irq_flags,
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
		ret = __fimc_is_hw_request_irq(itf_hwip, "3aa0", INTR_HWIP1, IRQF_TRIGGER_NONE, fimc_is_isr1_3aa0);
		ret = __fimc_is_hw_request_irq(itf_hwip, "3aa0", INTR_HWIP2, IRQF_TRIGGER_NONE, fimc_is_isr2_3aa0);
		break;
	case DEV_HW_3AA1:
		ret = __fimc_is_hw_request_irq(itf_hwip, "3aa1", INTR_HWIP1, IRQF_TRIGGER_NONE, fimc_is_isr1_3aa1);
		ret = __fimc_is_hw_request_irq(itf_hwip, "3aa1", INTR_HWIP2, IRQF_TRIGGER_NONE, fimc_is_isr2_3aa1);
		break;
	case DEV_HW_ISP0:
		ret = __fimc_is_hw_request_irq(itf_hwip, "isp0", INTR_HWIP1, IRQF_TRIGGER_NONE, fimc_is_isr1_isp);
		ret = __fimc_is_hw_request_irq(itf_hwip, "isp0", INTR_HWIP2, IRQF_TRIGGER_NONE, fimc_is_isr2_isp);
		break;
	case DEV_HW_MCSC0:
		ret = __fimc_is_hw_request_irq(itf_hwip, "mcs0", INTR_HWIP1, IRQF_TRIGGER_NONE, fimc_is_isr1_mcs);
		break;
	case DEV_HW_VRA:
		/* VRA CH1 */
		ret = __fimc_is_hw_request_irq(itf_hwip, "vra", INTR_HWIP2, IRQF_TRIGGER_NONE, fimc_is_isr1_vra);
		break;
	case DEV_HW_PAF0:
		ret = __fimc_is_hw_request_irq(itf_hwip, "paf0", INTR_HWIP1, IRQF_SHARED, fimc_is_isr1_paf0);
		break;
	case DEV_HW_PAF1:
		ret = __fimc_is_hw_request_irq(itf_hwip, "paf1", INTR_HWIP1, IRQF_SHARED, fimc_is_isr1_paf1);
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
		if (hw_id == FIMC_IS_VIDEO_M1P_NUM) {
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
		{
			struct fimc_is_hw_mcsc_cap *cap = (struct fimc_is_hw_mcsc_cap *)cap_data;
			/* v4.10 */
			cap->hw_ver = HW_SET_VERSION(4, 10, 0, 0);
			cap->max_output = 4;
			cap->hwfc = MCSC_CAP_SUPPORT;
			cap->in_otf = MCSC_CAP_SUPPORT;
			cap->in_dma = MCSC_CAP_SUPPORT;
			cap->out_dma[0] = MCSC_CAP_SUPPORT;
			cap->out_dma[1] = MCSC_CAP_SUPPORT;
			cap->out_dma[2] = MCSC_CAP_SUPPORT;
			cap->out_dma[3] = MCSC_CAP_SUPPORT;
			cap->out_dma[4] = MCSC_CAP_NOT_SUPPORT;
			cap->out_dma[5] = MCSC_CAP_NOT_SUPPORT;
			cap->out_otf[0] = MCSC_CAP_NOT_SUPPORT;
			cap->out_otf[1] = MCSC_CAP_NOT_SUPPORT;
			cap->out_otf[2] = MCSC_CAP_NOT_SUPPORT;
			cap->out_otf[3] = MCSC_CAP_NOT_SUPPORT;
			cap->out_otf[4] = MCSC_CAP_NOT_SUPPORT;
			cap->out_otf[5] = MCSC_CAP_NOT_SUPPORT;
			cap->out_hwfc[1] = MCSC_CAP_SUPPORT;
			cap->out_hwfc[2] = MCSC_CAP_SUPPORT;
			cap->enable_shared_output = false;
			cap->tdnr = MCSC_CAP_SUPPORT;
			cap->djag = MCSC_CAP_NOT_SUPPORT;
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

	return ioremap_nocache(SYSREG_CAM_BASE_ADDR, SZ_4K);
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

#ifdef ENABLE_FULLCHAIN_OVERFLOW_RECOVERY
int fimc_is_hw_overflow_recovery(void)
{
	int ret = 0;
	struct fimc_is_core *core = NULL;
	struct fimc_is_hardware *hardware = NULL;

	struct fimc_is_device_ischain *device = NULL;
	struct fimc_is_group *group;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_groupmgr *groupmgr;
	u32 backup_frame_numbuffers = 0;
	u32 recov_fcount = 0;

	struct fimc_is_device_sensor *sensor;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct v4l2_subdev *subdev_paf;

	u32 instance = 0;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core)
		goto exit;

	device = &core->ischain[0];
	hardware = &core->hardware;
	instance = device->instance;
	backup_frame_numbuffers = hardware->recovery_numbuffers;

	set_bit(HW_OVERFLOW_RECOVERY, &hardware->hw_recovery_flag);

	sensor = device->sensor;
	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(sensor->subdev_module);
	WARN_ON(!module);
	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;
	WARN_ON(!sensor_peri);
	subdev_paf = sensor_peri->subdev_paf;

	groupmgr = device->groupmgr;
	group = groupmgr->group[instance][GROUP_SLOT_3AA];
	framemgr = GET_HEAD_GROUP_FRAMEMGR(group);
	if (!framemgr) {
		merr("framemgr is NULL for recovery", device);
		goto exit;
	}

	/* 0. frame flush */
	ret = fimc_is_hardware_flush_frame_by_group(hardware, group, instance);
	if (ret) {
		merr("hw flush frame fail = %x", device, ret);
		goto exit;
	}

	/* 1. PAFSTAT sw reset */
	ret = fimc_is_pafstat_reset_recovery(subdev_paf, 0, PD_NONE);
	if (ret) {
		merr("pafstat reset fail = %x", device, ret);
		goto exit;
	}

	/* 2. Call DDK 3AA reset seq. */
	group = groupmgr->group[instance][GROUP_SLOT_3AA];
	ret = fimc_is_hardware_restore_by_group(hardware, group, instance);
	if (ret) {
		merr("3AA reset restore fail = %x", device, ret);
		goto exit;
	}

	/* 3. BLK_ISP LHM_ATB_GLUE sw reset */
	writel(0x0, isp_lhm_atb_glue_rst);
	writel(0x100, isp_lhm_atb_glue_rst);

	/* 4. Call DDK ISP reset seq. */
	group = groupmgr->group[instance][GROUP_SLOT_ISP];
	ret = fimc_is_hardware_restore_by_group(hardware, group, instance);
	if (ret) {
		merr("ISP reset restore fail = %x", device, ret);
		goto exit;
	}

	/* 5. MCSC sw reset  & sfr restore*/
	group = groupmgr->group[instance][GROUP_SLOT_MCS];
	ret = fimc_is_hardware_restore_by_group(hardware, group, instance);
	if (ret) {
		merr("MCSC reset restore fail = %x", device, ret);
		goto exit;
	}

	/* 6. for Next Shot */
	if (backup_frame_numbuffers) {
		u32 sensor_fcount = atomic_read(&group->sensor_fcount);
		u32 backup_fcount = atomic_read(&group->backup_fcount);

		if (sensor_fcount <= (backup_fcount + backup_frame_numbuffers)) {
			recov_fcount = backup_fcount + backup_frame_numbuffers;
		} else {
#if defined(ENABLE_HW_FAST_READ_OUT)
			recov_fcount = backup_fcount + backup_frame_numbuffers;
			recov_fcount = (int)DIV_ROUND_UP((sensor_fcount - recov_fcount), backup_frame_numbuffers);
			recov_fcount = backup_fcount + (backup_frame_numbuffers * (1 + recov_fcount));
			mgwarn(" frame count(%d->%d) reset by sensor focunt(%d)", group, group,
				backup_fcount + backup_frame_numbuffers,
				recov_fcount, sensor_fcount);
#else
			recov_fcount = sensor_fcount;
			mgwarn(" frame count reset by sensor focunt(%d->%d)", group, group,
				backup_fcount + backup_frame_numbuffers,
				recov_fcount);
#endif
		}
	}

	group = groupmgr->group[instance][GROUP_SLOT_3AA];
	ret = fimc_is_hardware_recovery_shot(hardware, instance,
			group, recov_fcount, framemgr);
	if (ret) {
		merr("Hardware recovery shot fail = %x", device, ret);
		goto exit;
	}

	/* 7. PAFSTAT sfr restore */
	ret = fimc_is_pafstat_reset_recovery(subdev_paf, 1, PD_MOD2);
	if (ret) {
		merr("pafstat restore fail = %x", device, ret);
		goto exit;
	}

	clear_bit(HW_OVERFLOW_RECOVERY, &hardware->hw_recovery_flag);

exit:
	return ret;
}
#endif

void fimc_is_hw_djag_get_input(struct fimc_is_device_ischain *ischain, u32 *djag_in)
{
	struct fimc_is_global_param *g_param;

	if (!ischain) {
		err_hw("device is NULL");
		return;
	}

	g_param = &ischain->resourcemgr->global_param;

	dbg_hw(2, "%s:video_mode %d\n", __func__, g_param->video_mode);

	if (g_param->video_mode)
		*djag_in = MCSC_DJAG_IN_VIDEO_MODE;
	else
		*djag_in = MCSC_DJAG_IN_CAPTURE_MODE;
}

void fimc_is_hw_djag_adjust_out_size(struct fimc_is_device_ischain *ischain,
					u32 in_width, u32 in_height,
					u32 *out_width, u32 *out_height)
{
	/* Do nothing. */
}
