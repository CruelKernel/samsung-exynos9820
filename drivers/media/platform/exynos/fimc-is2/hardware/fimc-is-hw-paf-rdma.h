/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_PAFRDMA_H
#define FIMC_IS_HW_PAFRDMA_H

#include "fimc-is-hw-control.h"
#include "fimc-is-interface-ddk.h"
#include "fimc-is-param.h"

struct fimc_is_hw_paf {
	struct paf_rdma_param		param[FIMC_IS_STREAM_COUNT];
	void __iomem	*paf_core_regs;
	void __iomem	*paf_ctx0_regs;
	void __iomem	*paf_ctx1_regs;
	void __iomem	*paf_rdma_core_regs;
	void __iomem	*paf_rdma0_regs;
	void __iomem	*paf_rdma1_regs;
	u32	input_dva[FIMC_IS_MAX_PLANES];
	u32	instance_id;
	u32	fcount;
};

int fimc_is_hw_paf_probe(struct fimc_is_hw_ip *hw_ip, struct fimc_is_interface *itf,
	struct fimc_is_interface_ischain *itfc, int id, const char *name);
int fimc_is_hw_paf_mode_change(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map);
int fimc_is_hw_paf_update_param(struct fimc_is_hw_ip *hw_ip, struct is_region *region,
	struct paf_rdma_param *param, u32 lindex, u32 hindex, u32 instance);
int fimc_is_hw_paf_reset(struct fimc_is_hw_ip *hw_ip);
#endif
