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

#ifndef _NPU_UTIL_STATEKEEPER_H_
#define _NPU_UTIL_STATEKEEPER_H_

#include <linux/atomic.h>
#include <linux/types.h>

#define STATE_KEEPER_MAX_STATE		16

struct npu_statekeeper {
	int		state_num;
	const char **state_names;
	const u8	(*transition_map)[STATE_KEEPER_MAX_STATE];
	atomic_t	state;
};

/* transition_map is a poiter to two dimensional array */

#define SKEEPER_IS_TRANSITABLE(keeper_ptr, TARGET) \
({										\
	struct npu_statekeeper *__keeper = (keeper_ptr);			\
	int __curr_state = atomic_read(&__keeper->state);			\
	int __target_state = (TARGET);						\
	u8 __ret;								\
	BUG_ON(__target_state < 0);						\
	BUG_ON(__target_state >= __keeper->state_num);				\
	__ret = __keeper->transition_map[__curr_state][__target_state];		\
	if (!__ret)								\
		npu_err("Invalid transition (%s) -> (%s)",			\
			__keeper->state_names[__curr_state],			\
			__keeper->state_names[__target_state]);			\
	__ret;									\
})

#define SKEEPER_EXPECT_STATE(keeper_ptr, STATE) \
({										\
	struct npu_statekeeper *__keeper = (keeper_ptr);			\
	int __curr_state = atomic_read(&__keeper->state);			\
	int __expect_state = (STATE);						\
	BUG_ON(__expect_state < 0);						\
	BUG_ON(__expect_state >= __keeper->state_num);				\
	if (__curr_state != __expect_state)					\
		npu_err("Requires state (%s), but current state is (%s)",	\
			__keeper->state_names[__expect_state],			\
			__keeper->state_names[__curr_state]);			\
	(__curr_state == __expect_state);					\
})

#define SKEEPER_COMPARE_STATE(keeper_ptr, STATE) \
({										\
	struct npu_statekeeper *__keeper = (keeper_ptr);			\
	int __curr_state = atomic_read(&__keeper->state);			\
	int __expect_state = (STATE);						\
	BUG_ON(__expect_state < 0);						\
	BUG_ON(__expect_state >= __keeper->state_num);				\
	(__curr_state == __expect_state);					\
})

int npu_statekeeper_transition(struct npu_statekeeper *keeper_ptr, int new_state);
void npu_statekeeper_initialize(struct npu_statekeeper *keeper_ptr,
	int param_state_num, const char *param_state_names[],
	const u8 param_transition_map[][STATE_KEEPER_MAX_STATE]);
const char *npu_statekeeper_state_name(struct npu_statekeeper *keeper_ptr);

#endif	/* _NPU_UTIL_STATEKEEPER_H_ */
