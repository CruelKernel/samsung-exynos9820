/*
 * drivers/media/video/exynos/fimc-is-mc2/interface/fimc-is-interface-ishcain.c
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *       http://www.samsung.com
 *
 * The header file related to camera
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/workqueue.h>
#include <linux/bug.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>

#include "fimc-is-core.h"
#include "fimc-is-regs.h"
#include "fimc-is-err.h"
#include "fimc-is-groupmgr.h"

#include "fimc-is-interface.h"
#include "fimc-is-debug.h"
#include "fimc-is-clk-gate.h"

#include "fimc-is-interface-ischain.h"
#include "fimc-is-interface-library.h"
#include "../include/fimc-is-hw.h"

u32 __iomem *notify_fcount_sen0;
u32 __iomem *notify_fcount_sen1;
u32 __iomem *notify_fcount_sen2;
u32 __iomem *notify_fcount_sen3;

#define init_request_barrier(itf) mutex_init(&itf->request_barrier)
#define enter_request_barrier(itf) mutex_lock(&itf->request_barrier);
#define exit_request_barrier(itf) mutex_unlock(&itf->request_barrier);
#define init_process_barrier(itf) spin_lock_init(&itf->process_barrier);
#define enter_process_barrier(itf) spin_lock_irq(&itf->process_barrier);
#define exit_process_barrier(itf) spin_unlock_irq(&itf->process_barrier);

extern struct fimc_is_sysfs_debug sysfs_debug;
extern struct fimc_is_lib_support gPtr_lib_support;

#if defined(SOC_PAF0) || defined(SOC_PAF1)
static int fimc_is_interface_paf_probe(struct fimc_is_interface_ischain *itfc,
	int hw_id, struct platform_device *pdev)
{
	struct fimc_is_interface_hwip *itf_paf = NULL;
	int ret = 0;
	int hw_slot = -1;

	FIMC_BUG(!itfc);
	FIMC_BUG(!pdev);

	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d) ", hw_slot);
		return -EINVAL;
	}

	itf_paf = &itfc->itf_ip[hw_slot];
	itf_paf->id = hw_id;
	itf_paf->state = 0;

	ret = fimc_is_hw_get_address(itf_paf, pdev, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_get_address failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	ret = fimc_is_hw_get_irq(itf_paf, pdev, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_get_irq failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	ret = fimc_is_hw_request_irq(itf_paf, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_request_irq failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	set_bit(IS_CHAIN_IF_STATE_INIT, &itf_paf->state);

	dbg_itfc("[ID:%2d] probe done\n", hw_id);

	return ret;
}
#endif

#if defined(SOC_30S) || defined(SOC_31S)
static int fimc_is_interface_3aa_probe(struct fimc_is_interface_ischain *itfc,
	int hw_id, struct platform_device *pdev)
{
	struct fimc_is_interface_hwip *itf_3aa = NULL;
	int i, ret = 0;
	int hw_slot = -1;
	int handler_id = 0;

	FIMC_BUG(!itfc);
	FIMC_BUG(!pdev);

	switch (hw_id) {
	case DEV_HW_3AA0:
		handler_id = ID_3AA_0;
		break;
	case DEV_HW_3AA1:
		handler_id = ID_3AA_1;
		break;
	case DEV_HW_VPP:
		handler_id = ID_VPP;
		break;
	default:
		err_itfc("invalid hw_id(%d)", hw_id);
		return -EINVAL;
	}

	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d) ", hw_slot);
		return -EINVAL;
	}

	itf_3aa = &itfc->itf_ip[hw_slot];
	itf_3aa->id = hw_id;
	itf_3aa->state = 0;

	ret = fimc_is_hw_get_address(itf_3aa, pdev, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_get_address failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	ret = fimc_is_hw_get_irq(itf_3aa, pdev, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_get_irq failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	ret = fimc_is_hw_request_irq(itf_3aa, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_request_irq failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	for (i = 0; i < INTR_HWIP_MAX; i++) {
		itf_3aa->handler[i].valid = false;

		gPtr_lib_support.intr_handler_taaisp[handler_id][i]
			= (struct hwip_intr_handler *)&itf_3aa->handler[i];
	}

	/* library data settings */
	if (!gPtr_lib_support.minfo) {
		gPtr_lib_support.itfc		= itfc;
		gPtr_lib_support.minfo		= itfc->minfo;

		gPtr_lib_support.pdev		= pdev;

#if !defined(ENABLE_DYNAMIC_MEM)
		info_itfc("[ID:%2d] kvaddr for taaisp: 0x%lx\n", hw_id,
			CALL_BUFOP(gPtr_lib_support.minfo->pb_taaisp, kvaddr,
					gPtr_lib_support.minfo->pb_taaisp));
#endif
	}

	set_bit(IS_CHAIN_IF_STATE_INIT, &itf_3aa->state);

	dbg_itfc("[ID:%2d] probe done\n", hw_id);

	return ret;
}
#endif

#if (defined(SOC_I0S) || defined(SOC_I1S)) && !defined(SOC_3AAISP)
static int fimc_is_interface_isp_probe(struct fimc_is_interface_ischain *itfc,
	int hw_id, struct platform_device *pdev)
{
	struct fimc_is_interface_hwip *itf_isp = NULL;
	int i, ret = 0;
	int hw_slot = -1;
	int handler_id = 0;

	FIMC_BUG(!itfc);
	FIMC_BUG(!pdev);

	switch (hw_id) {
	case DEV_HW_ISP0:
		handler_id = ID_ISP_0;
		break;
	case DEV_HW_ISP1:
		handler_id = ID_ISP_1;
		break;
	default:
		err_itfc("invalid hw_id(%d)", hw_id);
		return -EINVAL;
	}

	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d) ", hw_slot);
		return -EINVAL;
	}

	itf_isp = &itfc->itf_ip[hw_slot];
	itf_isp->id = hw_id;
	itf_isp->state = 0;

	ret = fimc_is_hw_get_address(itf_isp, pdev, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_get_address failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	ret = fimc_is_hw_get_irq(itf_isp, pdev, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_get_irq failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	ret = fimc_is_hw_request_irq(itf_isp, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_request_irq failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	for (i = 0; i < INTR_HWIP_MAX; i++) {
		itf_isp->handler[i].valid = false;

		/* TODO: this is not cool */
		gPtr_lib_support.intr_handler_taaisp[handler_id][i]
			= (struct hwip_intr_handler *)&itf_isp->handler[i];
	}

	/* library data settings */
	if (!gPtr_lib_support.minfo) {
		gPtr_lib_support.itfc		= itfc;
		gPtr_lib_support.minfo		= itfc->minfo;

		gPtr_lib_support.pdev		= pdev;

#if !defined(ENABLE_DYNAMIC_MEM)
		info_itfc("[ID:%2d] kvaddr for taaisp: 0x%lx\n", hw_id,
			CALL_BUFOP(gPtr_lib_support.minfo->pb_taaisp, kvaddr,
					gPtr_lib_support.minfo->pb_taaisp));
#endif
	}

	set_bit(IS_CHAIN_IF_STATE_INIT, &itf_isp->state);

	dbg_itfc("[ID:%2d] probe done\n", hw_id);

	return ret;
}
#endif

#if defined(SOC_DIS) || defined(SOC_D0S) || defined(SOC_D1S)
static int fimc_is_interface_tpu_probe(struct fimc_is_interface_ischain *itfc,
	int hw_id, struct platform_device *pdev)
{
	struct fimc_is_interface_hwip *itf_tpu = NULL;
	int i, ret = 0;
	int hw_slot = -1;
	int handler_id = 0;

	FIMC_BUG(!itfc);
	FIMC_BUG(!pdev);

	switch (hw_id) {
	case DEV_HW_TPU0:
		handler_id = ID_TPU_0;
		break;
	case DEV_HW_TPU1:
		handler_id = ID_TPU_1;
		break;
	default:
		err_itfc("invalid hw_id(%d)", hw_id);
		return -EINVAL;
	}

	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d) ", hw_slot);
		return -EINVAL;
	}

	itf_tpu = &itfc->itf_ip[hw_slot];
	itf_tpu->id = hw_id;
	itf_tpu->state = 0;

	ret = fimc_is_hw_get_address(itf_tpu, pdev, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_get_address failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	ret = fimc_is_hw_get_irq(itf_tpu, pdev, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_get_irq failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	ret = fimc_is_hw_request_irq(itf_tpu, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_request_irq failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	for (i = 0; i < INTR_HWIP_MAX; i++) {
		itf_tpu->handler[i].valid = false;

		/* TODO: this is not cool */
		gPtr_lib_support.intr_handler_taaisp[handler_id][i]
			= (struct hwip_intr_handler *)&itf_tpu->handler[i];
	}

	/* library data settings */
	if (!gPtr_lib_support.minfo) {
		gPtr_lib_support.itfc		= itfc;
		gPtr_lib_support.minfo		= itfc->minfo;

		gPtr_lib_support.pdev		= pdev;

#if defined(ENABLE_ODC) || defined(ENABLE_VDIS) || defined(ENABLE_DNR)
		info_itfc("[ID:%2d] kvaddr for tpu: 0x%lx\n", hw_id,
			CALL_BUFOP(gPtr_lib_support.minfo->pb_tpu, kvaddr,
					gPtr_lib_support.minfo->pb_tpu));
	} else {
		info_itfc("[ID:%2d] kvaddr for tpu: 0x%lx\n", hw_id,
			CALL_BUFOP(gPtr_lib_support.minfo->pb_tpu, kvaddr,
					gPtr_lib_support.minfo->pb_tpu));
#endif
	}

	set_bit(IS_CHAIN_IF_STATE_INIT, &itf_tpu->state);

	dbg_itfc("[ID:%2d] probe done\n", hw_id);

	return ret;
}
#endif

#if defined(SOC_SCP) || (defined(SOC_MCS) && (defined(SOC_MCS0) || defined(SOC_MCS1)))
static int fimc_is_interface_scaler_probe(struct fimc_is_interface_ischain *itfc,
	int hw_id, struct platform_device *pdev)
{
	struct fimc_is_interface_hwip *itf_scaler = NULL;
	int ret = 0;
	int hw_slot = -1;

	FIMC_BUG(!itfc);
	FIMC_BUG(!pdev);

	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d) ", hw_slot);
		return -EINVAL;
	}

	itf_scaler = &itfc->itf_ip[hw_slot];
	itf_scaler->id = hw_id;
	itf_scaler->state = 0;

	ret = fimc_is_hw_get_address(itf_scaler, pdev, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_get_address failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	ret = fimc_is_hw_get_irq(itf_scaler, pdev, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_get_irq failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	ret = fimc_is_hw_request_irq(itf_scaler, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_request_irq failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	set_bit(IS_CHAIN_IF_STATE_INIT, &itf_scaler->state);

	dbg_itfc("[ID:%2d] probe done\n", hw_id);

	return ret;
}
#endif

#if defined(SOC_VRA) && defined(ENABLE_VRA)
static int fimc_is_interface_vra_probe(struct fimc_is_interface_ischain *itfc,
	int hw_id, struct platform_device *pdev)
{
	struct fimc_is_interface_hwip *itf_vra = NULL;
	int ret = 0;
	int hw_slot = -1;

	FIMC_BUG(!itfc);
	FIMC_BUG(!pdev);

	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d) ", hw_slot);
		return -EINVAL;
	}

	itf_vra = &itfc->itf_ip[hw_slot];
	itf_vra->id = hw_id;
	itf_vra->state = 0;

	ret = fimc_is_hw_get_address(itf_vra, pdev, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_get_address failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	ret = fimc_is_hw_get_irq(itf_vra, pdev, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_get_irq failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	ret = fimc_is_hw_request_irq(itf_vra, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_request_irq failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	/* library data settings */
	if (!gPtr_lib_support.minfo) {
		gPtr_lib_support.itfc		= itfc;
		gPtr_lib_support.minfo		= itfc->minfo;

		gPtr_lib_support.pdev		= pdev;

		info_itfc("[ID:%2d] kvaddr for vra: 0x%lx\n", hw_id,
			CALL_BUFOP(gPtr_lib_support.minfo->pb_vra, kvaddr,
					gPtr_lib_support.minfo->pb_vra));
	} else {
		info_itfc("[ID:%2d] kvaddr for vra: 0x%lx\n", hw_id,
			CALL_BUFOP(gPtr_lib_support.minfo->pb_vra, kvaddr,
					gPtr_lib_support.minfo->pb_vra));
	}

	set_bit(IS_CHAIN_IF_STATE_INIT, &itf_vra->state);

	dbg_itfc("[ID:%2d] probe done\n", hw_id);

	return ret;
}
#endif

#if defined(SOC_DCP) && defined(CONFIG_DCP_V1_0)
static int fimc_is_interface_dcp_probe(struct fimc_is_interface_ischain *itfc,
	int hw_id, struct platform_device *pdev)
{
	struct fimc_is_interface_hwip *itf_dcp = NULL;
	int i, ret = 0;
	int hw_slot = -1;
	int handler_id = 0;

	FIMC_BUG(!itfc);
	FIMC_BUG(!pdev);

	switch (hw_id) {
	case DEV_HW_DCP:
		handler_id = ID_DCP;
		break;
	default:
		err_itfc("invalid hw_id(%d)", hw_id);
		return -EINVAL;
	}

	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d) ", hw_slot);
		return -EINVAL;
	}

	itf_dcp = &itfc->itf_ip[hw_slot];
	itf_dcp->id = hw_id;
	itf_dcp->state = 0;

	ret = fimc_is_hw_get_address(itf_dcp, pdev, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_get_address failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	ret = fimc_is_hw_get_irq(itf_dcp, pdev, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_get_irq failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	ret = fimc_is_hw_request_irq(itf_dcp, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_request_irq failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	for (i = 0; i < INTR_HWIP_MAX; i++) {
		itf_dcp->handler[i].valid = false;

		/* TODO: this is not cool */
		gPtr_lib_support.intr_handler_taaisp[handler_id][i]
			= (struct hwip_intr_handler *)&itf_dcp->handler[i];
	}

#if 0 /* TODO */
	/* library data settings */
	if (!gPtr_lib_support.minfo) {
		gPtr_lib_support.itfc		= itfc;
		gPtr_lib_support.minfo		= itfc->minfo;

		gPtr_lib_support.pdev		= pdev;

		info_itfc("[ID:%2d] kvaddr for dcp: 0x%lx\n", hw_id,
			CALL_BUFOP(gPtr_lib_support.minfo->pb_dcp, kvaddr,
					gPtr_lib_support.minfo->pb_dcp));
	} else {
		info_itfc("[ID:%2d] kvaddr for dcp: 0x%lx\n", hw_id,
			CALL_BUFOP(gPtr_lib_support.minfo->pb_dcp, kvaddr,
					gPtr_lib_support.minfo->pb_dcp));
	}
#endif

	set_bit(IS_CHAIN_IF_STATE_INIT, &itf_dcp->state);

	dbg_itfc("[ID:%2d] probe done\n", hw_id);

	return ret;
}
#endif

int fimc_is_interface_ischain_probe(struct fimc_is_interface_ischain *this,
	struct fimc_is_hardware *hardware, struct fimc_is_resourcemgr *resourcemgr,
	struct platform_device *pdev, ulong core_regs)
{
	enum fimc_is_hardware_id hw_id = DEV_HW_END;
	int ret = 0;
	int hw_slot = -1;

	FIMC_BUG(!this);
	FIMC_BUG(!resourcemgr);

	this->state = 0;
	/* this->regs_mcuctl = fimc_is_hw_get_sysreg(core_regs); *//* deprecated */
	this->minfo = &resourcemgr->minfo;

#if defined(SOC_PAF0)
	hw_id = DEV_HW_PAF0;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d) ", hw_slot);
		return -EINVAL;
	}

	this->itf_ip[hw_slot].hw_ip = &(hardware->hw_ip[hw_slot]);
	ret = fimc_is_interface_paf_probe(this, hw_id, pdev);
	if (ret) {
		err_itfc("interface probe fail (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
#endif

#if defined(SOC_PAF1)
	hw_id = DEV_HW_PAF1;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d) ", hw_slot);
		return -EINVAL;
	}

	this->itf_ip[hw_slot].hw_ip = &(hardware->hw_ip[hw_slot]);
	ret = fimc_is_interface_paf_probe(this, hw_id, pdev);
	if (ret) {
		err_itfc("interface probe fail (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
#endif

#if defined(SOC_30S)
	hw_id = DEV_HW_3AA0;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d) ", hw_slot);
		return -EINVAL;
	}

	this->itf_ip[hw_slot].hw_ip = &(hardware->hw_ip[hw_slot]);
	ret = fimc_is_interface_3aa_probe(this, hw_id, pdev);
	if (ret) {
		err_itfc("interface probe fail (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
#endif

#if defined(SOC_31S)
	hw_id = DEV_HW_3AA1;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d) ", hw_slot);
		return -EINVAL;
	}

	this->itf_ip[hw_slot].hw_ip = &(hardware->hw_ip[hw_slot]);
	ret = fimc_is_interface_3aa_probe(this, hw_id, pdev);
	if (ret) {
		err_itfc("interface probe fail (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
#endif

#if (defined(SOC_I0S) && !defined(SOC_3AAISP))
	hw_id = DEV_HW_ISP0;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d) ", hw_slot);
		return -EINVAL;
	}

	this->itf_ip[hw_slot].hw_ip = &(hardware->hw_ip[hw_slot]);
	ret = fimc_is_interface_isp_probe(this, hw_id, pdev);
	if (ret) {
		err_itfc("interface probe fail (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
#endif

#if (defined(SOC_I1S) && !defined(SOC_3AAISP))
	hw_id = DEV_HW_ISP1;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d) ", hw_slot);
		return -EINVAL;
	}

	this->itf_ip[hw_slot].hw_ip = &(hardware->hw_ip[hw_slot]);
	ret = fimc_is_interface_isp_probe(this, hw_id, pdev);
	if (ret) {
		err_itfc("interface probe fail (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
#endif

#if defined(SOC_DIS) || defined(SOC_D0S)
	hw_id = DEV_HW_TPU0;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d)", hw_slot);
		return -EINVAL;
	}

	this->itf_ip[hw_slot].hw_ip = &(hardware->hw_ip[hw_slot]);

	ret = fimc_is_interface_tpu_probe(this, hw_id, pdev);
	if (ret) {
		err_itfc("interface probe fail (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
#endif

#if defined(SOC_D1S)
	hw_id = DEV_HW_TPU1;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d)", hw_slot);
		return -EINVAL;
	}

	this->itf_ip[hw_slot].hw_ip = &(hardware->hw_ip[hw_slot]);

	ret = fimc_is_interface_tpu_probe(this, hw_id, pdev);
	if (ret) {
		err_itfc("interface probe fail (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
#endif

#if (defined(SOC_SCP) && !defined(MCS_USE_SCP_PARAM))
	hw_id = DEV_HW_SCP;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d)", hw_slot);
		return -EINVAL;
	}

	this->itf_ip[hw_slot].hw_ip = &(hardware->hw_ip[hw_slot]);

	ret = fimc_is_interface_scaler_probe(this, hw_id, pdev);
	if (ret) {
		err_itfc("interface probe fail (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
#endif

#if (defined(SOC_SCP) && defined(MCS_USE_SCP_PARAM))
	hw_id = DEV_HW_MCSC0;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d)", hw_slot);
		return -EINVAL;
	}

	this->itf_ip[hw_slot].hw_ip = &(hardware->hw_ip[hw_slot]);

	ret = fimc_is_interface_scaler_probe(this, hw_id, pdev);
	if (ret) {
		err_itfc("interface probe fail (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
#endif

#if (defined(SOC_MCS) && defined(SOC_MCS0))
	hw_id = DEV_HW_MCSC0;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d)", hw_slot);
		return -EINVAL;
	}

	this->itf_ip[hw_slot].hw_ip = &(hardware->hw_ip[hw_slot]);

	ret = fimc_is_interface_scaler_probe(this, hw_id, pdev);
	if (ret) {
		err_itfc("interface probe fail (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
#endif

#if (defined(SOC_MCS) && defined(SOC_MCS1))
	hw_id = DEV_HW_MCSC1;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d)", hw_slot);
		return -EINVAL;
	}

	this->itf_ip[hw_slot].hw_ip = &(hardware->hw_ip[hw_slot]);

	ret = fimc_is_interface_scaler_probe(this, hw_id, pdev);
	if (ret) {
		err_itfc("interface probe fail (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
#endif

#if defined(SOC_VRA) && defined(ENABLE_VRA)
	hw_id = DEV_HW_VRA;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d)", hw_slot);
		return -EINVAL;
	}

	this->itf_ip[hw_slot].hw_ip = &(hardware->hw_ip[hw_slot]);

	ret = fimc_is_interface_vra_probe(this, hw_id, pdev);
	if (ret) {
		err_itfc("interface probe fail (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
#endif

#if defined(SOC_DCP) && defined(CONFIG_DCP_V1_0)
	hw_id = DEV_HW_DCP;
	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d)", hw_slot);
		return -EINVAL;
	}

	this->itf_ip[hw_slot].hw_ip = &(hardware->hw_ip[hw_slot]);

	ret = fimc_is_interface_dcp_probe(this, hw_id, pdev);
	if (ret) {
		err_itfc("interface probe fail (%d,%d)", hw_id, hw_slot);
		return -EINVAL;
	}
#endif

	set_bit(IS_CHAIN_IF_STATE_INIT, &this->state);
	dbg_itfc("interface ishchain probe done (%d,%d,%d)(%d)", hw_id, hw_slot, hw_slot, ret);

	return ret;
}

int print_fre_work_list(struct fimc_is_work_list *this)
{
	struct fimc_is_work *work, *temp;

	if (!(this->id & TRACE_WORK_ID_MASK))
		return 0;

	printk(KERN_ERR "[INF] fre(%02X, %02d) :", this->id, this->work_free_cnt);

	list_for_each_entry_safe(work, temp, &this->work_free_head, list) {
		printk(KERN_CONT "%X(%d)->", work->msg.command, work->fcount);
	}

	printk(KERN_CONT "X\n");

	return 0;
}

static int set_free_work(struct fimc_is_work_list *this, struct fimc_is_work *work)
{
	int ret = 0;
	ulong flags;

	if (work) {
		spin_lock_irqsave(&this->slock_free, flags);

		list_add_tail(&work->list, &this->work_free_head);
		this->work_free_cnt++;
#ifdef TRACE_WORK
		print_fre_work_list(this);
#endif

		spin_unlock_irqrestore(&this->slock_free, flags);
	} else {
		ret = -EFAULT;
		err("item is null ptr\n");
	}

	return ret;
}

int print_req_work_list(struct fimc_is_work_list *this)
{
	struct fimc_is_work *work, *temp;

	if (!(this->id & TRACE_WORK_ID_MASK))
		return 0;

	printk(KERN_ERR "[INF] req(%02X, %02d) :", this->id, this->work_request_cnt);

	list_for_each_entry_safe(work, temp, &this->work_request_head, list) {
		printk(KERN_CONT "%X([%d][G%X][F%d])->", work->msg.command,
				work->msg.instance, work->msg.group, work->fcount);
	}

	printk(KERN_CONT "X\n");

	return 0;
}

static int get_req_work(struct fimc_is_work_list *this,
	struct fimc_is_work **work)
{
	int ret = 0;
	ulong flags;

	if (work) {
		spin_lock_irqsave(&this->slock_request, flags);

		if (this->work_request_cnt) {
			*work = container_of(this->work_request_head.next,
					struct fimc_is_work, list);
			list_del(&(*work)->list);
			this->work_request_cnt--;
		} else
			*work = NULL;

		spin_unlock_irqrestore(&this->slock_request, flags);
	} else {
		ret = -EFAULT;
		err("item is null ptr\n");
	}

	return ret;
}

static void init_work_list(struct fimc_is_work_list *this, u32 id, u32 count)
{
	u32 i;

	this->id = id;
	this->work_free_cnt	= 0;
	this->work_request_cnt	= 0;
	INIT_LIST_HEAD(&this->work_free_head);
	INIT_LIST_HEAD(&this->work_request_head);
	spin_lock_init(&this->slock_free);
	spin_lock_init(&this->slock_request);
	for (i = 0; i < count; ++i)
		set_free_work(this, &this->work[i]);

	init_waitqueue_head(&this->wait_queue);
}

static inline void wq_func_schedule(struct fimc_is_interface *itf,
	struct work_struct *work_wq)
{
	if (itf->workqueue)
		queue_work(itf->workqueue, work_wq);
	else
		schedule_work(work_wq);
}

static void wq_func_subdev(struct fimc_is_subdev *leader,
	struct fimc_is_subdev *subdev,
	struct fimc_is_frame *sub_frame,
	u32 fcount, u32 rcount, u32 status)
{
	u32 done_state = VB2_BUF_STATE_DONE;
	u32 findex, mindex;
	struct fimc_is_video_ctx *ldr_vctx, *sub_vctx;
	struct fimc_is_framemgr *ldr_framemgr, *sub_framemgr;
	struct fimc_is_frame *ldr_frame;

	FIMC_BUG_VOID(!sub_frame);

	ldr_vctx = leader->vctx;
	sub_vctx = subdev->vctx;

	ldr_framemgr = GET_FRAMEMGR(ldr_vctx);
	sub_framemgr = GET_FRAMEMGR(sub_vctx);
	if (!ldr_framemgr) {
		mserr("ldr_framemgr is NULL", subdev, subdev);
		goto p_err;
	}
	if (!sub_framemgr) {
		mserr("sub_framemgr is NULL", subdev, subdev);
		goto p_err;
	}

	findex = sub_frame->stream->findex;
	mindex = ldr_vctx->queue.buf_maxcount;
	if (findex >= mindex) {
		mserr("findex(%d) is invalid(max %d)", subdev, subdev, findex, mindex);
		done_state = VB2_BUF_STATE_ERROR;
		sub_frame->stream->fvalid = 0;
		goto complete;
	}

	ldr_frame = &ldr_framemgr->frames[findex];
	if (ldr_frame->fcount != fcount) {
		mserr("frame mismatched(ldr%d, sub%d)", subdev, subdev, ldr_frame->fcount, fcount);
		done_state = VB2_BUF_STATE_ERROR;
		sub_frame->stream->fvalid = 0;
		goto complete;
	}

	if (status) {
		msrinfo("[ERR] NDONE(%d, E%X)\n", subdev, subdev, ldr_frame, sub_frame->index, status);
		done_state = VB2_BUF_STATE_ERROR;
		sub_frame->stream->fvalid = 0;
	} else {
		msrdbgs(1, " DONE(%d)\n", subdev, subdev, ldr_frame, sub_frame->index);
		sub_frame->stream->fvalid = 1;
	}

	clear_bit(subdev->id, &ldr_frame->out_flag);

complete:
	sub_frame->stream->fcount = fcount;
	sub_frame->stream->rcount = rcount;
	sub_frame->fcount = fcount;
	sub_frame->rcount = rcount;

	trans_frame(sub_framemgr, sub_frame, FS_COMPLETE);

	/* for debug */
	DBG_DIGIT_TAG((ldr_frame->group) ? ((struct fimc_is_group *)ldr_frame->group)->slot : 0,
			0, GET_QUEUE(sub_vctx), sub_frame, fcount - sub_frame->num_buffers + 1, 1);

	CALL_VOPS(sub_vctx, done, sub_frame->index, done_state);

p_err:
	return;

}

static void wq_func_frame(struct fimc_is_subdev *leader,
	struct fimc_is_subdev *subdev,
	u32 fcount, u32 rcount, u32 status)
{
	ulong flags;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;

	FIMC_BUG_VOID(!leader);
	FIMC_BUG_VOID(!subdev);
	FIMC_BUG_VOID(!leader->vctx);
	FIMC_BUG_VOID(!subdev->vctx);
	FIMC_BUG_VOID(!leader->vctx->video);
	FIMC_BUG_VOID(!subdev->vctx->video);

	framemgr = GET_FRAMEMGR(subdev->vctx);

	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_4, flags);

	frame = peek_frame(framemgr, FS_PROCESS);
	if (frame) {
		if (!frame->stream) {
			mserr("stream is NULL", subdev, subdev);
			BUG();
		}

		if (unlikely(frame->stream->fcount != fcount)) {
			while (frame) {
				if (fcount == frame->stream->fcount) {
					wq_func_subdev(leader, subdev, frame, fcount, rcount, status);
					break;
				} else if (fcount > frame->stream->fcount) {
					wq_func_subdev(leader, subdev, frame, frame->stream->fcount, rcount, 0xF);

					/* get next subdev frame */
					frame = peek_frame(framemgr, FS_PROCESS);
				} else {
					warn("%d frame done is ignored", frame->stream->fcount);
					break;
				}
			}
		} else {
			wq_func_subdev(leader, subdev, frame, fcount, rcount, status);
		}
	} else {
		mserr("frame done(%p) is occured without request", subdev, subdev, frame);
	}

	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_4, flags);
}

static void wq_func_3xc(struct work_struct *data, u32 wq_id)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[wq_id]);

	get_req_work(&itf->work_list[wq_id], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->txc;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[wq_id], work);
		get_req_work(&itf->work_list[wq_id], &work);
	}
}

static void wq_func_30c(struct work_struct *data)
{
	wq_func_3xc(data, WORK_30C_FDONE);
}

static void wq_func_31c(struct work_struct *data)
{
	wq_func_3xc(data, WORK_31C_FDONE);
}

static void wq_func_3xp(struct work_struct *data, u32 wq_id)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[wq_id]);

	get_req_work(&itf->work_list[wq_id], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->txp;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[wq_id], work);
		get_req_work(&itf->work_list[wq_id], &work);
	}
}

static void wq_func_30p(struct work_struct *data)
{
	wq_func_3xp(data, WORK_30P_FDONE);
}

static void wq_func_31p(struct work_struct *data)
{
	wq_func_3xp(data, WORK_31P_FDONE);
}

static void wq_func_32p(struct work_struct *data)
{
	wq_func_3xp(data, WORK_32P_FDONE);
}

static void wq_func_3xf(struct work_struct *data, u32 wq_id)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[wq_id]);

	get_req_work(&itf->work_list[wq_id], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->txf;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[wq_id], work);
		get_req_work(&itf->work_list[wq_id], &work);
	}
}

static void wq_func_30f(struct work_struct *data)
{
	wq_func_3xf(data, WORK_30F_FDONE);
}

static void wq_func_31f(struct work_struct *data)
{
	wq_func_3xf(data, WORK_31F_FDONE);
}

static void wq_func_3xg(struct work_struct *data, u32 wq_id)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[wq_id]);

	get_req_work(&itf->work_list[wq_id], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->txg;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[wq_id], work);
		get_req_work(&itf->work_list[wq_id], &work);
	}
}

static void wq_func_30g(struct work_struct *data)
{
	wq_func_3xg(data, WORK_30G_FDONE);
}

static void wq_func_31g(struct work_struct *data)
{
	wq_func_3xg(data, WORK_31G_FDONE);
}

static void wq_func_ixc(struct work_struct *data, u32 wq_id)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[wq_id]);

	get_req_work(&itf->work_list[wq_id], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->ixc;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[wq_id], work);
		get_req_work(&itf->work_list[wq_id], &work);
	}
}

static void wq_func_i0c(struct work_struct *data)
{
	wq_func_ixc(data, WORK_I0C_FDONE);
}

static void wq_func_i1c(struct work_struct *data)
{
	wq_func_ixc(data, WORK_I1C_FDONE);
}

static void wq_func_ixp(struct work_struct *data, u32 wq_id)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[wq_id]);

	get_req_work(&itf->work_list[wq_id], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->ixp;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[wq_id], work);
		get_req_work(&itf->work_list[wq_id], &work);
	}
}

static void wq_func_i0p(struct work_struct *data)
{
	wq_func_ixp(data, WORK_I0P_FDONE);
}

static void wq_func_i1p(struct work_struct *data)
{
	wq_func_ixp(data, WORK_I1P_FDONE);
}

static void wq_func_mexc(struct work_struct *data, u32 wq_id)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[wq_id]);

	get_req_work(&itf->work_list[wq_id], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->mexc;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[wq_id], work);
		get_req_work(&itf->work_list[wq_id], &work);
	}
}

static void wq_func_me0c(struct work_struct *data)
{
	wq_func_mexc(data, WORK_ME0C_FDONE);
}

static void wq_func_me1c(struct work_struct *data)
{
	wq_func_mexc(data, WORK_ME1C_FDONE);
}

static void wq_func_dxc(struct work_struct *data, u32 wq_id)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[wq_id]);

	get_req_work(&itf->work_list[wq_id], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->dxc;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[wq_id], work);
		get_req_work(&itf->work_list[wq_id], &work);
	}
}

static void wq_func_d0c(struct work_struct *data)
{
	wq_func_dxc(data, WORK_D0C_FDONE);
}

static void wq_func_d1c(struct work_struct *data)
{
	wq_func_dxc(data, WORK_D1C_FDONE);
}

static void wq_func_dc1s(struct work_struct *data)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[WORK_DC1S_FDONE]);

	get_req_work(&itf->work_list[WORK_DC1S_FDONE], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->dc1s;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[WORK_DC1S_FDONE], work);
		get_req_work(&itf->work_list[WORK_DC1S_FDONE], &work);
	}
}

static void wq_func_dc0c(struct work_struct *data)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[WORK_DC0C_FDONE]);

	get_req_work(&itf->work_list[WORK_DC0C_FDONE], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->dc0c;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[WORK_DC0C_FDONE], work);
		get_req_work(&itf->work_list[WORK_DC0C_FDONE], &work);
	}
}

static void wq_func_dc1c(struct work_struct *data)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[WORK_DC1C_FDONE]);

	get_req_work(&itf->work_list[WORK_DC1C_FDONE], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->dc1c;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[WORK_DC1C_FDONE], work);
		get_req_work(&itf->work_list[WORK_DC1C_FDONE], &work);
	}
}

static void wq_func_dc2c(struct work_struct *data)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[WORK_DC2C_FDONE]);

	get_req_work(&itf->work_list[WORK_DC2C_FDONE], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->dc2c;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[WORK_DC2C_FDONE], work);
		get_req_work(&itf->work_list[WORK_DC2C_FDONE], &work);
	}
}

static void wq_func_dc3c(struct work_struct *data)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[WORK_DC3C_FDONE]);

	get_req_work(&itf->work_list[WORK_DC3C_FDONE], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->dc3c;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[WORK_DC3C_FDONE], work);
		get_req_work(&itf->work_list[WORK_DC3C_FDONE], &work);
	}
}

static void wq_func_dc4c(struct work_struct *data)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[WORK_DC4C_FDONE]);

	get_req_work(&itf->work_list[WORK_DC4C_FDONE], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->dc4c;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[WORK_DC4C_FDONE], work);
		get_req_work(&itf->work_list[WORK_DC4C_FDONE], &work);
	}
}

static void wq_func_m0p(struct work_struct *data)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[WORK_M0P_FDONE]);

	get_req_work(&itf->work_list[WORK_M0P_FDONE], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->m0p;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		/* For supporting multi input to single output */
		if (subdev->vctx->video->try_smp) {
			subdev->vctx->video->try_smp = false;
			up(&subdev->vctx->video->smp_multi_input);
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[WORK_M0P_FDONE], work);
		get_req_work(&itf->work_list[WORK_M0P_FDONE], &work);
	}
}

static void wq_func_m1p(struct work_struct *data)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[WORK_M1P_FDONE]);

	get_req_work(&itf->work_list[WORK_M1P_FDONE], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->m1p;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		/* For supporting multi input to single output */
		if (subdev->vctx->video->try_smp) {
			subdev->vctx->video->try_smp = false;
			up(&subdev->vctx->video->smp_multi_input);
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[WORK_M1P_FDONE], work);
		get_req_work(&itf->work_list[WORK_M1P_FDONE], &work);
	}
}

static void wq_func_m2p(struct work_struct *data)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[WORK_M2P_FDONE]);

	get_req_work(&itf->work_list[WORK_M2P_FDONE], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->m2p;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		/* For supporting multi input to single output */
		if (subdev->vctx->video->try_smp) {
			subdev->vctx->video->try_smp = false;
			up(&subdev->vctx->video->smp_multi_input);
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[WORK_M2P_FDONE], work);
		get_req_work(&itf->work_list[WORK_M2P_FDONE], &work);
	}
}

static void wq_func_m3p(struct work_struct *data)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[WORK_M3P_FDONE]);

	get_req_work(&itf->work_list[WORK_M3P_FDONE], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->m3p;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		/* For supporting multi input to single output */
		if (subdev->vctx->video->try_smp) {
			subdev->vctx->video->try_smp = false;
			up(&subdev->vctx->video->smp_multi_input);
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[WORK_M3P_FDONE], work);
		get_req_work(&itf->work_list[WORK_M3P_FDONE], &work);
	}
}

static void wq_func_m4p(struct work_struct *data)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[WORK_M4P_FDONE]);

	get_req_work(&itf->work_list[WORK_M4P_FDONE], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->m4p;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		/* For supporting multi input to single output */
		if (subdev->vctx->video->try_smp) {
			subdev->vctx->video->try_smp = false;
			up(&subdev->vctx->video->smp_multi_input);
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[WORK_M4P_FDONE], work);
		get_req_work(&itf->work_list[WORK_M4P_FDONE], &work);
	}
}

static void wq_func_m5p(struct work_struct *data)
{
	u32 instance, fcount, rcount, status;
	struct fimc_is_interface *itf;
	struct fimc_is_device_ischain *device;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_work *work;
	struct fimc_is_msg *msg;

	itf = container_of(data, struct fimc_is_interface, work_wq[WORK_M5P_FDONE]);

	get_req_work(&itf->work_list[WORK_M5P_FDONE], &work);
	while (work) {
		msg = &work->msg;
		instance = msg->instance;
		fcount = msg->param1;
		rcount = msg->param2;
		status = msg->param3;

		if (instance >= FIMC_IS_STREAM_COUNT) {
			err("instance is invalid(%d)", instance);
			goto p_err;
		}

		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &device->state)) {
			merr("device is not open", device);
			goto p_err;
		}

		subdev = &device->m5p;
		if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
			merr("subdev is not start", device);
			goto p_err;
		}

		leader = subdev->leader;
		if (!leader) {
			merr("leader is NULL", device);
			goto p_err;
		}

		/* For supporting multi input to single output */
		if (subdev->vctx->video->try_smp) {
			subdev->vctx->video->try_smp = false;
			up(&subdev->vctx->video->smp_multi_input);
		}

		wq_func_frame(leader, subdev, fcount, rcount, status);

p_err:
		set_free_work(&itf->work_list[WORK_M5P_FDONE], work);
		get_req_work(&itf->work_list[WORK_M5P_FDONE], &work);
	}
}

static void wq_func_group_xxx(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group *group,
	struct fimc_is_framemgr *framemgr,
	struct fimc_is_frame *frame,
	struct fimc_is_video_ctx *vctx,
	u32 status, u32 lindex, u32 hindex)
{
	u32 done_state = VB2_BUF_STATE_DONE;

	FIMC_BUG_VOID(!vctx);
	FIMC_BUG_VOID(!framemgr);
	FIMC_BUG_VOID(!frame);

	/* perframe error control */
	if (test_bit(FIMC_IS_SUBDEV_PARAM_ERR, &group->leader.state)) {
		if (!status) {
			if (frame->lindex || frame->hindex)
				clear_bit(FIMC_IS_SUBDEV_PARAM_ERR, &group->leader.state);
			else
				status = SHOT_ERR_PERFRAME;
		}
	} else {
		if (status && (frame->lindex || frame->hindex))
			set_bit(FIMC_IS_SUBDEV_PARAM_ERR, &group->leader.state);
	}

	if (status) {
		mgrinfo("[ERR] NDONE(%d, E%X(L%X H%X))\n", group, group, frame, frame->index, status,
			lindex, hindex);
		done_state = VB2_BUF_STATE_ERROR;

		if (status == IS_SHOT_OVERFLOW) {
#ifdef OVERFLOW_PANIC_ENABLE_ISCHAIN
			panic("G%d overflow", group->id);
#else
			err("G%d overflow", group->id);
#endif
		}
	} else {
		mgrdbgs(1, " DONE(%d)\n", group, group, frame, frame->index);
	}

	/* Cache Invalidation */
	fimc_is_ischain_meta_invalid(frame);

	frame->result = status;
	clear_bit(group->leader.id, &frame->out_flag);
	fimc_is_group_done(groupmgr, group, frame, done_state);
	trans_frame(framemgr, frame, FS_COMPLETE);
	CALL_VOPS(vctx, done, frame->index, done_state);
}

void wq_func_group(struct fimc_is_device_ischain *device,
	struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group *group,
	struct fimc_is_framemgr *framemgr,
	struct fimc_is_frame *frame,
	struct fimc_is_video_ctx *vctx,
	u32 status, u32 fcount)
{
	u32 lindex = 0;
	u32 hindex = 0;

	FIMC_BUG_VOID(!groupmgr);
	FIMC_BUG_VOID(!group);
	FIMC_BUG_VOID(!framemgr);
	FIMC_BUG_VOID(!frame);
	FIMC_BUG_VOID(!vctx);

	TIME_SHOT(TMS_SDONE);
	/*
	 * complete count should be lower than 3 when
	 * buffer is queued or overflow can be occured
	 */
	if (framemgr->queued_count[FS_COMPLETE] >= DIV_ROUND_UP(framemgr->num_frames, 2))
		mgwarn(" complete bufs : %d", device, group, (framemgr->queued_count[FS_COMPLETE] + 1));

	if (status) {
		lindex = frame->shot->ctl.vendor_entry.lowIndexParam;
		lindex &= ~frame->shot->dm.vendor_entry.lowIndexParam;
		hindex = frame->shot->ctl.vendor_entry.highIndexParam;
		hindex &= ~frame->shot->dm.vendor_entry.highIndexParam;
	}

	if (unlikely(fcount != frame->fcount)) {
		if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state)) {
			while (frame) {
				if (fcount == frame->fcount) {
					wq_func_group_xxx(groupmgr, group, framemgr, frame, vctx,
						status, lindex, hindex);
					break;
				} else if (fcount > frame->fcount) {
					wq_func_group_xxx(groupmgr, group, framemgr, frame, vctx,
						SHOT_ERR_MISMATCH, lindex, hindex);

					/* get next leader frame */
					frame = peek_frame(framemgr, FS_PROCESS);
				} else {
					warn("%d shot done is ignored", fcount);
					break;
				}
			}
		} else {
			wq_func_group_xxx(groupmgr, group, framemgr, frame, vctx,
				SHOT_ERR_MISMATCH, lindex, hindex);
		}
	} else {
		wq_func_group_xxx(groupmgr, group, framemgr, frame, vctx,
			status, lindex, hindex);
	}
}

static void wq_func_shot(struct work_struct *data)
{
	struct fimc_is_device_ischain *device;
	struct fimc_is_interface *itf;
	struct fimc_is_msg *msg;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;
	struct fimc_is_group *head;
	struct fimc_is_work_list *work_list;
	struct fimc_is_work *work;
	struct fimc_is_video_ctx *vctx;
	ulong flags;
	u32 fcount, status;
	int instance;
	int group_id;
	struct fimc_is_core *core;

	FIMC_BUG_VOID(!data);

	itf = container_of(data, struct fimc_is_interface, work_wq[WORK_SHOT_DONE]);
	work_list = &itf->work_list[WORK_SHOT_DONE];
	group  = NULL;
	vctx = NULL;
	framemgr = NULL;

	get_req_work(work_list, &work);
	while (work) {
		core = (struct fimc_is_core *)itf->core;
		instance = work->msg.instance;
		group_id = work->msg.group;
		device = &((struct fimc_is_core *)itf->core)->ischain[instance];
		groupmgr = device->groupmgr;

		msg = &work->msg;
		fcount = msg->param1;
		status = msg->param2;

		switch (group_id) {
		case GROUP_ID(GROUP_ID_PAF0):
		case GROUP_ID(GROUP_ID_PAF1):
			group = &device->group_paf;
			break;
		case GROUP_ID(GROUP_ID_3AA0):
		case GROUP_ID(GROUP_ID_3AA1):
			group = &device->group_3aa;
			break;
		case GROUP_ID(GROUP_ID_ISP0):
		case GROUP_ID(GROUP_ID_ISP1):
			group = &device->group_isp;
			break;
		case GROUP_ID(GROUP_ID_DIS0):
		case GROUP_ID(GROUP_ID_DIS1):
			group = &device->group_dis;
			break;
		case GROUP_ID(GROUP_ID_DCP):
			group = &device->group_dcp;
			break;
		case GROUP_ID(GROUP_ID_MCS0):
		case GROUP_ID(GROUP_ID_MCS1):
			group = &device->group_mcs;
			break;
		case GROUP_ID(GROUP_ID_VRA0):
			group = &device->group_vra;
			break;
		default:
			merr("unresolved group id %d", device, group_id);
			group = NULL;
			vctx = NULL;
			framemgr = NULL;
			goto remain;
		}

		head = group->head;
		vctx = head->leader.vctx;
		if (!vctx) {
			merr("vctx is NULL", device);
			goto remain;
		}

		framemgr = GET_FRAMEMGR(vctx);
		if (!framemgr) {
			merr("framemgr is NULL", device);
			goto remain;
		}

		framemgr_e_barrier_irqs(framemgr, FMGR_IDX_5, flags);

		frame = peek_frame(framemgr, FS_PROCESS);

		if (frame) {
			/* clear bit for child group */
			if (group != head)
				clear_bit(group->leader.id, &frame->out_flag);
#ifdef MEASURE_TIME
#ifdef EXTERNAL_TIME
			do_gettimeofday(&frame->tzone[TM_SHOT_D]);
#endif
#endif

#ifdef ENABLE_CLOCK_GATE
			/* dynamic clock off */
			if (sysfs_debug.en_clk_gate &&
					sysfs_debug.clk_gate_mode == CLOCK_GATE_MODE_HOST)
				fimc_is_clk_gate_set(core, group->id, false, false, true);
#endif
			wq_func_group(device, groupmgr, head, framemgr, frame,
				vctx, status, fcount);
		} else {
			mgerr("invalid shot done(%d)", device, head, fcount);
			frame_manager_print_queues(framemgr);
		}

		framemgr_x_barrier_irqr(framemgr, FMGR_IDX_5, flags);
#ifdef ENABLE_CLOCK_GATE
		if (fcount == 1 &&
				sysfs_debug.en_clk_gate &&
				sysfs_debug.clk_gate_mode == CLOCK_GATE_MODE_HOST)
			fimc_is_clk_gate_lock_set(core, instance, false);
#endif
remain:
		set_free_work(work_list, work);
		get_req_work(work_list, &work);
	}
}

static inline void print_framemgr_spinlock_usage(struct fimc_is_core *core)
{
	u32 i;
	struct fimc_is_device_ischain *ischain;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_subdev *subdev;

	for (i = 0; i < FIMC_IS_SENSOR_COUNT; ++i) {
		sensor = &core->sensor[i];
		if (test_bit(FIMC_IS_SENSOR_OPEN, &sensor->state) && (framemgr = &sensor->vctx->queue.framemgr))
			info("[@] framemgr(%s) sindex : 0x%08lX\n", framemgr->name, framemgr->sindex);
	}

	for (i = 0; i < FIMC_IS_STREAM_COUNT; ++i) {
		ischain = &core->ischain[i];
		if (test_bit(FIMC_IS_ISCHAIN_OPEN, &ischain->state)) {
			/* 3AA GROUP */
			subdev = &ischain->group_3aa.leader;
			if (test_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state) && (framemgr = GET_SUBDEV_FRAMEMGR(subdev)))
				info("[@] framemgr(%s) sindex : 0x%08lX\n", framemgr->name, framemgr->sindex);

			subdev = &ischain->txc;
			if (test_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state) && (framemgr = GET_SUBDEV_FRAMEMGR(subdev)))
				info("[@] framemgr(%s) sindex : 0x%08lX\n", framemgr->name, framemgr->sindex);

			subdev = &ischain->txp;
			if (test_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state) && (framemgr = GET_SUBDEV_FRAMEMGR(subdev)))
				info("[@] framemgr(%s) sindex : 0x%08lX\n", framemgr->name, framemgr->sindex);

			subdev = &ischain->txf;
			if (test_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state) && (framemgr = GET_SUBDEV_FRAMEMGR(subdev)))
				info("[@] framemgr(%s) sindex : 0x%08lX\n", framemgr->name, framemgr->sindex);

			subdev = &ischain->txg;
			if (test_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state) && (framemgr = GET_SUBDEV_FRAMEMGR(subdev)))
				info("[@] framemgr(%s) sindex : 0x%08lX\n", framemgr->name, framemgr->sindex);

			/* ISP GROUP */
			subdev = &ischain->group_isp.leader;
			if (test_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state) && (framemgr = GET_SUBDEV_FRAMEMGR(subdev)))
				info("[@] framemgr(%s) sindex : 0x%08lX\n", framemgr->name, framemgr->sindex);

			subdev = &ischain->ixc;
			if (test_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state) && (framemgr = GET_SUBDEV_FRAMEMGR(subdev)))
				info("[@] framemgr(%s) sindex : 0x%08lX\n", framemgr->name, framemgr->sindex);

			subdev = &ischain->ixp;
			if (test_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state) && (framemgr = GET_SUBDEV_FRAMEMGR(subdev)))
				info("[@] framemgr(%s) sindex : 0x%08lX\n", framemgr->name, framemgr->sindex);

			subdev = &ischain->mexc;
			if (test_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state) && (framemgr = GET_SUBDEV_FRAMEMGR(subdev)))
				info("[@] framemgr(%s) sindex : 0x%08lX\n", framemgr->name, framemgr->sindex);

			/* DIS GROUP */
			subdev = &ischain->group_dis.leader;
			if (test_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state) && (framemgr = GET_SUBDEV_FRAMEMGR(subdev)))
				info("[@] framemgr(%s) sindex : 0x%08lX\n", framemgr->name, framemgr->sindex);

			subdev = &ischain->scc;
			if (test_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state) && (framemgr = GET_SUBDEV_FRAMEMGR(subdev)))
				info("[@] framemgr(%s) sindex : 0x%08lX\n", framemgr->name, framemgr->sindex);

			subdev = &ischain->scp;
			if (test_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state) && (framemgr = GET_SUBDEV_FRAMEMGR(subdev)))
				info("[@] framemgr(%s) sindex : 0x%08lX\n", framemgr->name, framemgr->sindex);
		}
	}
}

static void interface_timer(unsigned long data)
{
	u32 shot_count, scount_3ax, scount_isp;
	u32 fcount, i;
	unsigned long flags;
	struct fimc_is_interface *itf = (struct fimc_is_interface *)data;
	struct fimc_is_core *core;
	struct fimc_is_device_ischain *device;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_work_list *work_list;

	FIMC_BUG_VOID(!itf);
	FIMC_BUG_VOID(!itf->core);

	if (!test_bit(IS_IF_STATE_OPEN, &itf->state)) {
		pr_info("shot timer is terminated\n");
		return;
	}

	core = itf->core;

	for (i = 0; i < FIMC_IS_STREAM_COUNT; ++i) {
		device = &core->ischain[i];
		shot_count = 0;
		scount_3ax = 0;
		scount_isp = 0;

		sensor = device->sensor;
		if (!sensor)
			continue;

		if (!test_bit(FIMC_IS_SENSOR_FRONT_START, &sensor->state))
			continue;

		if (test_bit(FIMC_IS_ISCHAIN_OPEN_STREAM, &device->state)) {
			spin_lock_irqsave(&itf->shot_check_lock, flags);
			if (atomic_read(&itf->shot_check[i])) {
				atomic_set(&itf->shot_check[i], 0);
				atomic_set(&itf->shot_timeout[i], 0);
				spin_unlock_irqrestore(&itf->shot_check_lock, flags);
				continue;
			}
			spin_unlock_irqrestore(&itf->shot_check_lock, flags);

			if (test_bit(FIMC_IS_GROUP_START, &device->group_3aa.state)) {
				framemgr = GET_HEAD_GROUP_FRAMEMGR(&device->group_3aa);
				if (framemgr) {
					framemgr_e_barrier_irqs(framemgr, FMGR_IDX_6, flags);
					scount_3ax = framemgr->queued_count[FS_PROCESS];
					shot_count += scount_3ax;
					framemgr_x_barrier_irqr(framemgr, FMGR_IDX_6, flags);
				} else {
					minfo("\n### group_3aa framemgr is null ###\n", device);
				}
			}

			if (test_bit(FIMC_IS_GROUP_START, &device->group_isp.state)) {
				framemgr = GET_HEAD_GROUP_FRAMEMGR(&device->group_isp);
				if (framemgr) {
					framemgr_e_barrier_irqs(framemgr, FMGR_IDX_7, flags);
					scount_isp = framemgr->queued_count[FS_PROCESS];
					shot_count += scount_isp;
					framemgr_x_barrier_irqr(framemgr, FMGR_IDX_7, flags);
				} else {
					minfo("\n### group_isp framemgr is null ###\n", device);
				}
			}

			if (test_bit(FIMC_IS_GROUP_START, &device->group_dis.state)) {
				framemgr = GET_HEAD_GROUP_FRAMEMGR(&device->group_dis);
				if (framemgr) {
					framemgr_e_barrier_irqs(framemgr, FMGR_IDX_8, flags);
					shot_count += framemgr->queued_count[FS_PROCESS];
					framemgr_x_barrier_irqr(framemgr, FMGR_IDX_8, flags);
				} else {
					minfo("\n### group_dis framemgr is null ###\n", device);
				}
			}

			if (test_bit(FIMC_IS_GROUP_START, &device->group_vra.state)) {
				framemgr = GET_HEAD_GROUP_FRAMEMGR(&device->group_vra);
				if (framemgr) {
					framemgr_e_barrier_irqs(framemgr, FMGR_IDX_31, flags);
					shot_count += framemgr->queued_count[FS_PROCESS];
					framemgr_x_barrier_irqr(framemgr, FMGR_IDX_31, flags);
				} else {
					minfo("\n### group_vra framemgr is null ###\n", device);
				}
			}
		}

		if (shot_count) {
			atomic_inc(&itf->shot_timeout[i]);
			minfo("shot timer[%d] is increased to %d\n", device,
				i, atomic_read(&itf->shot_timeout[i]));
		}

		if (atomic_read(&itf->shot_timeout[i]) > TRY_TIMEOUT_COUNT) {
			merr("shot command is timeout(%d, %d(%d+%d))", device,
				atomic_read(&itf->shot_timeout[i]),
				shot_count, scount_3ax, scount_isp);

			minfo("\n### 3ax framemgr info ###\n", device);
			if (scount_3ax) {
				framemgr = GET_HEAD_GROUP_FRAMEMGR(&device->group_3aa);
				if (framemgr) {
					framemgr_e_barrier_irqs(framemgr, 0, flags);
					frame_manager_print_queues(framemgr);
					framemgr_x_barrier_irqr(framemgr, 0, flags);
				} else {
					minfo("\n### 3ax framemgr is null ###\n", device);
				}
			}

			minfo("\n### isp framemgr info ###\n", device);
			if (scount_isp) {
				framemgr = GET_HEAD_GROUP_FRAMEMGR(&device->group_isp);
				if (framemgr) {
					framemgr_e_barrier_irqs(framemgr, 0, flags);
					frame_manager_print_queues(framemgr);
					framemgr_x_barrier_irqr(framemgr, 0, flags);
				} else {
					minfo("\n### isp framemgr is null ###\n", device);
				}
			}

			minfo("\n### work list info ###\n", device);
			work_list = &itf->work_list[WORK_SHOT_DONE];
			print_fre_work_list(work_list);
			print_req_work_list(work_list);

			/* framemgr spinlock check */
			print_framemgr_spinlock_usage(core);
#ifdef FW_PANIC_ENABLE
			/* if panic happened, fw log dump should be happened by panic handler */
			mdelay(2000);
			panic("[@] camera firmware panic!!!");
#else
			fimc_is_resource_dump();
#endif
			return;
		}
	}

	for (i = 0; i < FIMC_IS_SENSOR_COUNT; ++i) {
		sensor = &core->sensor[i];

		if (!test_bit(FIMC_IS_SENSOR_BACK_START, &sensor->state))
			continue;

		if (!test_bit(FIMC_IS_SENSOR_FRONT_START, &sensor->state))
			continue;

		fcount = fimc_is_sensor_g_fcount(sensor);
		if (fcount == atomic_read(&itf->sensor_check[i])) {
			atomic_inc(&itf->sensor_timeout[i]);
			pr_err ("sensor timer[%d] is increased to %d(fcount : %d)\n", i,
				atomic_read(&itf->sensor_timeout[i]), fcount);
			fimc_is_sensor_dump(sensor);
		} else {
			atomic_set(&itf->sensor_timeout[i], 0);
			atomic_set(&itf->sensor_check[i], fcount);
		}

		if (atomic_read(&itf->sensor_timeout[i]) > SENSOR_TIMEOUT_COUNT) {
			merr("sensor is timeout(%d, %d)", sensor,
				atomic_read(&itf->sensor_timeout[i]),
				atomic_read(&itf->sensor_check[i]));

			/* framemgr spinlock check */
			print_framemgr_spinlock_usage(core);

#ifdef SENSOR_PANIC_ENABLE
			/* if panic happened, fw log dump should be happened by panic handler */
			mdelay(2000);
			panic("[@] camera sensor panic!!!");
#else
			fimc_is_resource_dump();
#endif
			return;
		}
	}

	mod_timer(&itf->timer, jiffies + (FIMC_IS_COMMAND_TIMEOUT/TRY_TIMEOUT_COUNT));
}

int fimc_is_interface_probe(struct fimc_is_interface *this,
	struct fimc_is_minfo *minfo,
	ulong regs,
	u32 irq,
	void *core_data)
{
	int ret = 0;
	struct fimc_is_core *core = (struct fimc_is_core *)core_data;

	dbg_interface(1, "%s\n", __func__);

	init_request_barrier(this);
	init_process_barrier(this);
	init_waitqueue_head(&this->lock_wait_queue);
	init_waitqueue_head(&this->init_wait_queue);
	init_waitqueue_head(&this->idle_wait_queue);
	spin_lock_init(&this->shot_check_lock);

	this->workqueue = alloc_workqueue("fimc-is/[H/U]", WQ_HIGHPRI | WQ_UNBOUND, 0);
	if (!this->workqueue)
		probe_warn("failed to alloc own workqueue, will be use global one");

	INIT_WORK(&this->work_wq[WORK_SHOT_DONE], wq_func_shot);
	INIT_WORK(&this->work_wq[WORK_30C_FDONE], wq_func_30c);
	INIT_WORK(&this->work_wq[WORK_30P_FDONE], wq_func_30p);
	INIT_WORK(&this->work_wq[WORK_30F_FDONE], wq_func_30f);
	INIT_WORK(&this->work_wq[WORK_30G_FDONE], wq_func_30g);
	INIT_WORK(&this->work_wq[WORK_31C_FDONE], wq_func_31c);
	INIT_WORK(&this->work_wq[WORK_31P_FDONE], wq_func_31p);
	INIT_WORK(&this->work_wq[WORK_31F_FDONE], wq_func_31f);
	INIT_WORK(&this->work_wq[WORK_31G_FDONE], wq_func_31g);
	INIT_WORK(&this->work_wq[WORK_32P_FDONE], wq_func_32p);
	INIT_WORK(&this->work_wq[WORK_I0C_FDONE], wq_func_i0c);
	INIT_WORK(&this->work_wq[WORK_I0P_FDONE], wq_func_i0p);
	INIT_WORK(&this->work_wq[WORK_I1C_FDONE], wq_func_i1c);
	INIT_WORK(&this->work_wq[WORK_I1P_FDONE], wq_func_i1p);
	INIT_WORK(&this->work_wq[WORK_ME0C_FDONE], wq_func_me0c);
	INIT_WORK(&this->work_wq[WORK_ME1C_FDONE], wq_func_me1c);
	INIT_WORK(&this->work_wq[WORK_D0C_FDONE], wq_func_d0c);
	INIT_WORK(&this->work_wq[WORK_D1C_FDONE], wq_func_d1c);

	INIT_WORK(&this->work_wq[WORK_DC1S_FDONE], wq_func_dc1s);
	INIT_WORK(&this->work_wq[WORK_DC0C_FDONE], wq_func_dc0c);
	INIT_WORK(&this->work_wq[WORK_DC1C_FDONE], wq_func_dc1c);
	INIT_WORK(&this->work_wq[WORK_DC2C_FDONE], wq_func_dc2c);
	INIT_WORK(&this->work_wq[WORK_DC3C_FDONE], wq_func_dc3c);
	INIT_WORK(&this->work_wq[WORK_DC4C_FDONE], wq_func_dc4c);

	INIT_WORK(&this->work_wq[WORK_M0P_FDONE], wq_func_m0p);
	INIT_WORK(&this->work_wq[WORK_M1P_FDONE], wq_func_m1p);
	INIT_WORK(&this->work_wq[WORK_M2P_FDONE], wq_func_m2p);
	INIT_WORK(&this->work_wq[WORK_M3P_FDONE], wq_func_m3p);
	INIT_WORK(&this->work_wq[WORK_M4P_FDONE], wq_func_m4p);
	INIT_WORK(&this->work_wq[WORK_M5P_FDONE], wq_func_m5p);

	this->core = (void *)core;
	this->regs = (void *)regs;

	clear_bit(IS_IF_STATE_OPEN, &this->state);
	clear_bit(IS_IF_STATE_START, &this->state);
	clear_bit(IS_IF_STATE_BUSY, &this->state);
	clear_bit(IS_IF_STATE_READY, &this->state);
	clear_bit(IS_IF_STATE_LOGGING, &this->state);

	init_work_list(&this->work_list[WORK_SHOT_DONE], TRACE_WORK_ID_SHOT, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_30C_FDONE], TRACE_WORK_ID_30C, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_30P_FDONE], TRACE_WORK_ID_30P, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_30F_FDONE], TRACE_WORK_ID_30F, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_30G_FDONE], TRACE_WORK_ID_30G, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_31C_FDONE], TRACE_WORK_ID_31C, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_31P_FDONE], TRACE_WORK_ID_31P, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_31F_FDONE], TRACE_WORK_ID_31F, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_31G_FDONE], TRACE_WORK_ID_31G, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_32P_FDONE], TRACE_WORK_ID_32P, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_I0C_FDONE], TRACE_WORK_ID_I0C, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_I0P_FDONE], TRACE_WORK_ID_I0P, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_I1C_FDONE], TRACE_WORK_ID_I1C, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_I1P_FDONE], TRACE_WORK_ID_I1P, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_ME0C_FDONE], TRACE_WORK_ID_ME0C, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_ME1C_FDONE], TRACE_WORK_ID_ME1C, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_D0C_FDONE], TRACE_WORK_ID_D0C, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_D1C_FDONE], TRACE_WORK_ID_D1C, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_DC1S_FDONE], TRACE_WORK_ID_DC1S, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_DC0C_FDONE], TRACE_WORK_ID_DC0C, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_DC1C_FDONE], TRACE_WORK_ID_DC1C, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_DC2C_FDONE], TRACE_WORK_ID_DC2C, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_DC3C_FDONE], TRACE_WORK_ID_DC3C, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_DC4C_FDONE], TRACE_WORK_ID_DC4C, MAX_WORK_COUNT);

	init_work_list(&this->work_list[WORK_M0P_FDONE], TRACE_WORK_ID_M0P, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_M1P_FDONE], TRACE_WORK_ID_M1P, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_M2P_FDONE], TRACE_WORK_ID_M2P, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_M3P_FDONE], TRACE_WORK_ID_M3P, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_M4P_FDONE], TRACE_WORK_ID_M4P, MAX_WORK_COUNT);
	init_work_list(&this->work_list[WORK_M5P_FDONE], TRACE_WORK_ID_M5P, MAX_WORK_COUNT);

	this->err_report_vendor = NULL;

	return ret;
}

int fimc_is_interface_open(struct fimc_is_interface *this)
{
	int i;
	int ret = 0;

	if (test_bit(IS_IF_STATE_OPEN, &this->state)) {
		err("already open");
		ret = -EMFILE;
		goto exit;
	}

	dbg_interface(1, "%s\n", __func__);

	for (i = 0; i < FIMC_IS_STREAM_COUNT; i++) {
		this->streaming[i] = IS_IF_STREAMING_INIT;
		this->processing[i] = IS_IF_PROCESSING_INIT;
		atomic_set(&this->shot_check[i], 0);
		atomic_set(&this->shot_timeout[i], 0);
	}
	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		atomic_set(&this->sensor_check[i], 0);
		atomic_set(&this->sensor_timeout[i], 0);
	}

	clear_bit(IS_IF_STATE_START, &this->state);
	clear_bit(IS_IF_STATE_BUSY, &this->state);
	clear_bit(IS_IF_STATE_READY, &this->state);
	clear_bit(IS_IF_STATE_LOGGING, &this->state);

	init_timer(&this->timer);
	this->timer.expires = jiffies + (FIMC_IS_COMMAND_TIMEOUT/TRY_TIMEOUT_COUNT);
	this->timer.data = (unsigned long)this;
	this->timer.function = interface_timer;
	add_timer(&this->timer);

	set_bit(IS_IF_STATE_OPEN, &this->state);

exit:
	return ret;
}

int fimc_is_interface_close(struct fimc_is_interface *this)
{
	int ret = 0;

	if (!test_bit(IS_IF_STATE_OPEN, &this->state)) {
		err("already close");
		ret = -EMFILE;
		goto exit;
	}

	if (this->timer.data)
		del_timer_sync(&this->timer);

	dbg_interface(1, "%s\n", __func__);

	clear_bit(IS_IF_STATE_OPEN, &this->state);

exit:
	return ret;
}

void fimc_is_interface_reset(struct fimc_is_interface *this)
{
	return;
}

int fimc_is_hw_logdump(struct fimc_is_interface *this)
{
	return 0;
}

int fimc_is_hw_regdump(struct fimc_is_interface *this)
{
	return 0;
}

int fimc_is_hw_memdump(struct fimc_is_interface *this,
	ulong start,
	ulong end)
{
	return 0;
}

int fimc_is_hw_i2c_lock(struct fimc_is_interface *this,
	u32 instance, int i2c_clk, bool lock)
{
	return 0;
}

void fimc_is_interface_lock(struct fimc_is_interface *this)
{
	return;
}

void fimc_is_interface_unlock(struct fimc_is_interface *this)
{
	return;
}

int fimc_is_hw_g_capability(struct fimc_is_interface *this,
	u32 instance, u32 address)
{
	return 0;
}

int fimc_is_hw_fault(struct fimc_is_interface *this)
{
	return 0;
}
