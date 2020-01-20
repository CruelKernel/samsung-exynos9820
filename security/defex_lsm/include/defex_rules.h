/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
*/

#ifndef __DEFEX_RULES_H
#define __DEFEX_RULES_H

#define STATIC_RULES_MAX_STR 32
#ifdef DEFEX_INTEGRITY_ENABLE
#define INTEGRITY_LENGTH 32
#endif /* DEFEX_INTEGRITY_ENABLE */

#define GET_ITEM_OFFSET(item_ptr)	(((char*)item_ptr) - ((char*)defex_packed_rules))
#define GET_ITEM_PTR(offset)		((struct rule_item_struct *)(((char*)defex_packed_rules) + (offset)))

enum feature_types {
	feature_is_file = 1,
	feature_ped_path = 2,
	feature_ped_exception = 4,
	feature_ped_status = 8,
	feature_safeplace_path = 16,
	feature_safeplace_status = 32
};

struct static_rule {
	unsigned int feature_type;
	char rule[STATIC_RULES_MAX_STR];
};

struct rule_item_struct {
	unsigned short int next_file;
	unsigned short int next_level;
	unsigned short int feature_type;
	unsigned char size;
#ifdef DEFEX_INTEGRITY_ENABLE
	unsigned char integrity[INTEGRITY_LENGTH];
#endif /* DEFEX_INTEGRITY_ENABLE */
	char name[0];
} __attribute__((packed));

extern const struct static_rule defex_static_rules[];
extern const int static_rule_count;

#endif /* __DEFEX_RULES_H */
