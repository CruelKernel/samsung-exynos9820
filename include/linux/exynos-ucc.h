/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd
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

struct ucc_req {
	struct plist_node node;
	int active;
	char name[20];
};

extern int filter_cstate(int cpu, int index);
extern void __init init_exynos_ucc(void);

#ifdef CONFIG_ARM64_EXYNOS_CPUIDLE
extern int filter_cstate(int cpu, int index);
extern void ucc_add_request(struct ucc_req *req, int value);
extern void ucc_update_request(struct ucc_req *req, int value);
extern void ucc_remove_request(struct ucc_req *req);
#else
static inline int filter_cstate(int cpu, int index) { return index; }
static inline void ucc_add_request(struct ucc_req *req, int value) { }
static inline void ucc_update_request(struct ucc_req *req, int value) { }
static inline void ucc_remove_request(struct ucc_req *req) { }
#endif