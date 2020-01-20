/*
 * Samsung Exynos SoC series FIMC-IS driver
 *
 * exynos fimc-is core functions
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-clk-gate.h"
#include "fimc-is-regs.h"

int fimc_is_clk_gate_init(struct fimc_is_core *core)
{
	struct fimc_is_clk_gate_ctrl *gate_ctrl;

	pr_info("%s\n",	__func__);

	if (!core) {
		err("core is NULL\n");
		return -EINVAL;
	}

	gate_ctrl = &core->resourcemgr.clk_gate_ctrl;
	memset(gate_ctrl, 0x0, sizeof(struct fimc_is_clk_gate_ctrl));

	/* init spin_lock for clock gating */
	spin_lock_init(&gate_ctrl->lock);
	core->resourcemgr.clk_gate_ctrl.gate_info = core->pdata->gate_info;

#if defined(ENABLE_IS_CORE)
	/* ISSR53 is clock gating debugging region.
	 * High means clock on state.
	 * To prevent telling A5 wrong clock off state,
	 * clock on state should be set before clock off is set.
	 */
	writel(0xFFFFFFFF, core->ischain[0].interface->regs + ISSR53);
#endif

	return 0;
}

int fimc_is_clk_gate_lock_set(struct fimc_is_core *core, u32 instance, u32 is_start)
{
	spin_lock(&core->resourcemgr.clk_gate_ctrl.lock);
	core->resourcemgr.clk_gate_ctrl.msk_lock_by_ischain[instance] = is_start;
	spin_unlock(&core->resourcemgr.clk_gate_ctrl.lock);
	return 0;
}

int fimc_is_wrap_clk_gate_set(struct fimc_is_core *core,
			int msk_group_id, bool is_on)
{
	int i;

	for (i = 0; i < FIMC_IS_GRP_MAX; i++) {
		if (msk_group_id & (1 << i))
			fimc_is_clk_gate_set(core, i, is_on, true, false);
	}

	return 0;
}

inline bool fimc_is_group_otf(struct fimc_is_device_ischain *device, int group_id)
{
	struct fimc_is_group *group;

	switch (group_id) {
	case GROUP_ID_PAF0:
	case GROUP_ID_PAF1:
		group = &device->group_paf;
		break;
	case GROUP_ID_3AA0:
	case GROUP_ID_3AA1:
		group = &device->group_3aa;
		break;
	case GROUP_ID_ISP0:
	case GROUP_ID_ISP1:
		group = &device->group_isp;
		break;
	case GROUP_ID_DIS0:
	case GROUP_ID_DIS1:
		group = &device->group_dis;
		break;
	default:
		group = NULL;
		pr_err("%s unresolved group id %d", __func__,  group_id);
		return false;
	}

	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state))
		return true;
	else
		return false;
}

int fimc_is_clk_gate_set(struct fimc_is_core *core,
			int group_id, bool is_on, bool skip_set_state, bool user_scenario)
{
	int ret = 0;
	int i;
	struct fimc_is_clk_gate_ctrl *gate_ctrl;
	struct exynos_fimc_is_clk_gate_info *gate_info;
#if defined(ENABLE_IS_CORE)
	int cfg = 0;
	u32 mask_on, mask_off, mask_cond_depend;
#endif

	gate_ctrl = &core->resourcemgr.clk_gate_ctrl;
	gate_info = gate_ctrl->gate_info;

	pr_debug("%s in\n", __func__);
	spin_lock(&gate_ctrl->lock);

	/* Set State */
	if (is_on) {
		if (skip_set_state == false) {
			(gate_ctrl->chk_on_off_cnt[group_id])++; /* for debuging */
			(gate_ctrl->msk_cnt[group_id])++;
			set_bit(group_id, &gate_ctrl->msk_state);
		}
		gate_info->groups[group_id].mask_clk_on_mod =
			gate_info->groups[group_id].mask_clk_on_org;
	} else {
		(gate_ctrl->chk_on_off_cnt[group_id])--; /* for debuging */
		(gate_ctrl->msk_cnt[group_id])--;
		if ((gate_ctrl->msk_cnt[group_id]) < 0) {
			pr_warn("%s msk_cnt[%d] is lower than zero !!\n", __func__, group_id);
			(gate_ctrl->msk_cnt[group_id]) = 0;
		}
		if ((gate_ctrl->msk_cnt[group_id]) == 0)
			clear_bit(group_id, &gate_ctrl->msk_state);
		/* if there's some processing group shot, don't clock off */
		if (test_bit_variables(group_id, &gate_ctrl->msk_state))
			goto exit;
		gate_info->groups[group_id].mask_clk_off_self_mod =
			gate_info->groups[group_id].mask_clk_off_self_org;
	}

	/* Don't off!! when other instance opening/closing */
	if (is_on == false) {
		for (i = 0; i < FIMC_IS_STREAM_COUNT; i++) {
			if ((!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &core->ischain[i].state)) &&
					(gate_ctrl->msk_lock_by_ischain[i])) {
				pr_info("%s lock(on) due to instance(%d)\n", __func__, i);
				goto exit;
			}
			/* don't off! if there is at least this group that is OTF */
			if (fimc_is_group_otf(&core->ischain[i], group_id)) {
				pr_debug("%s don't off!! this instance(%d) group(%d) is OTF\n",
					__func__, i, group_id);
				goto exit;
			}
		}
	}

	/* Check user scenario */
	if (user_scenario && gate_info->user_clk_gate) {
		if (fimc_is_set_user_clk_gate(group_id,
					core,
					is_on,
					gate_ctrl->msk_state,
					gate_info) < 0) {
			pr_debug("%s user scenario is skip!! [%d] !!\n", __func__, group_id);
			goto exit;
		}
	}

#if defined(ENABLE_IS_CORE)
	/* Get the region for clock gating debug */
	cfg = readl(core->ischain[0].interface->regs + ISSR53);

	/* Get Mask of self-on/off */
	if (is_on)
		mask_on = gate_info->groups[group_id].mask_clk_on_mod;
	else
		mask_off = gate_info->groups[group_id].mask_clk_off_self_mod;

	/* Clock on */
	if (is_on && ((gate_ctrl->msk_clk_on_off_state) !=  mask_on)) {
		cfg |= (mask_on << 16); /* shortly before clock on */
		writel(cfg, core->ischain[0].interface->regs + ISSR53);

		ret = gate_info->clk_on_off(mask_on, is_on);
		gate_ctrl->msk_clk_on_off_state |= mask_on;

		cfg |= (mask_on); /* after clock on */
		writel(cfg, core->ischain[0].interface->regs + ISSR53);
	}

	/* Clock off and check dependancy (it's for only clock-off) */
	if (is_on == false) {
		mask_cond_depend = gate_info->groups[group_id].mask_cond_for_depend;
		/* check dependancy */
		if (mask_cond_depend > 0 &&
				(mask_cond_depend & (gate_ctrl->msk_state))) {
			mask_off |= gate_info->groups[group_id].mask_clk_off_depend;
		}
		/* clock off */
		if (((gate_ctrl->msk_clk_on_off_state) & mask_off) > 0) {
			cfg &= ~(mask_off << 16); /* shortly before clock off */
			writel(cfg, core->ischain[0].interface->regs + ISSR53);

			ret = gate_info->clk_on_off(mask_off, is_on);
			gate_ctrl->msk_clk_on_off_state &= ~(mask_off);

			cfg &= ~(mask_off); /* after clock off */
			writel(cfg, core->ischain[0].interface->regs + ISSR53);
		}
	}
#endif
exit:
	spin_unlock(&gate_ctrl->lock);
	pr_debug("%s out\n", __func__);

	return ret;
}

int fimc_is_set_user_clk_gate(u32 group_id,
		struct fimc_is_core *core,
		bool is_on,
		unsigned long msk_state,
		struct exynos_fimc_is_clk_gate_info *gate_info)
{
	u32 user_scenario_id = 0;

	/* deside what user scenario is */
#if defined(ENABLE_FULL_BYPASS)
	if (group_id == GROUP_ID_ISP0)
		user_scenario_id = CLK_GATE_FULL_BYPASS_SN;
#else
	if (group_id == GROUP_ID_ISP0)
		user_scenario_id = CLK_GATE_NOT_FULL_BYPASS_SN;

#endif

	if (gate_info->user_clk_gate(group_id,
			is_on,
			user_scenario_id,
			msk_state,
			gate_info) < 0) {
		pr_err("%s user_clk_gate failed(%d) !!\n", __func__, group_id);
	}

	return 0;
}

