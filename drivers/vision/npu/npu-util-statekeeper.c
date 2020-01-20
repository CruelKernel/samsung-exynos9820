/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "npu-util-statekeeper.h"
#include "npu-log.h"

/*
 * Sample state
 *
 * typedef enum {
 *	PROTO_DRV_STATE_UNLOADED = 0,
 *	PROTO_DRV_STATE_PROBED,
 *	PROTO_DRV_STATE_OPENED,
 *	PROTO_DRV_STATE_INVALID,
 * } proto_drv_state_e;
 * const char* PROTO_DRV_STATE_NAME[PROTO_DRV_STATE_INVALID + 1]
 *	= {"UNLOADED", "PROBED", "OPENED", "INVALID"};
 *
 * static const u8 proto_drv_thread_state_transition[][PROTO_DRV_STATE_INVALID+1] = {
 *	From    -   To		UNLOADED	PROBED		OPENED		INVALID
 *	UNLOADED	{	0,		1,		0,		0},
 *	PROBED		{	1,		0,		1,		0},
 *	OPENED		{	1,		1,		0,		0},
 *	INVALID		{	0,		0,		0,		0},
 * };
 */

/*
 * Make transition to new_state.
 * If the transition is not allowed in transition_map, BUG_ON(1) is fired.
 * Otherwise, old state is returned.
 */
int npu_statekeeper_transition(struct npu_statekeeper *keeper_ptr, int new_state)
{
	int old_state;

	BUG_ON(!keeper_ptr);
	BUG_ON(new_state < 0);
	BUG_ON(new_state >= keeper_ptr->state_num);

	old_state = atomic_xchg(&keeper_ptr->state, new_state);

	/* Check after transition is made - To ensure atomicity */
	if (!keeper_ptr->transition_map[old_state][new_state]) {
		npu_err("Invalid transition [%s] -> [%s]\n",
			keeper_ptr->state_names[old_state],
			keeper_ptr->state_names[new_state]);
		BUG_ON(1);
	}
	return old_state;
}

/*
 * Initialize npu_statekeeper object and
 * set its state to '0'
 */
void npu_statekeeper_initialize(struct npu_statekeeper *keeper_ptr,
			       int param_state_num, const char *param_state_names[],
			       const u8 param_transition_map[][STATE_KEEPER_MAX_STATE])
{
	BUG_ON(!keeper_ptr);
	BUG_ON(!param_state_names);
	BUG_ON(param_state_num <= 1);
	BUG_ON(!param_transition_map);

	atomic_set(&keeper_ptr->state, 0);
	keeper_ptr->state_num = param_state_num;
	keeper_ptr->state_names = param_state_names;
	keeper_ptr->transition_map = param_transition_map;
}

/* Return constant reference of current state name */
const char *npu_statekeeper_state_name(struct npu_statekeeper *keeper_ptr)
{
	int state;

	BUG_ON(!keeper_ptr);

	state = atomic_read(&keeper_ptr->state);
	BUG_ON(state < 0);
	BUG_ON(state >= keeper_ptr->state_num);

	return keeper_ptr->state_names[atomic_read(&keeper_ptr->state)];
}
