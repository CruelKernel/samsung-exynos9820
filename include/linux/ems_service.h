/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

enum stune_group {
	STUNE_ROOT,
	STUNE_FOREGROUND,
	STUNE_BACKGROUND,
	STUNE_TOPAPP,
	STUNE_RT,
	STUNE_GROUP_COUNT,
};

struct kpp {
	struct plist_node node;
	int grp_idx;
	bool active;
};


#ifdef CONFIG_SCHED_EMS
/* prefer perf */
extern int kpp_status(int grp_idx);
extern void kpp_request(int grp_idx, struct kpp *req, int value);
#else
static inline int kpp_status(int grp_idx) { return 0; }
static inline void kpp_request(int grp_idx, struct kpp *req, int value) { }
#endif
