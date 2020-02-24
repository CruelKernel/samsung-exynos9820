/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
*/

#ifndef __DEFEX_RULES_H
#define __DEFEX_RULES_H

#define STATIC_RULES_MAX_STR 		32
#define INTEGRITY_LENGTH 		32
#define FEATURE_NAME_MAX_STR 		32

#define GET_ITEM_OFFSET(item_ptr)	(((char*)item_ptr) - ((char*)defex_packed_rules))
#define GET_ITEM_PTR(offset)		((struct rule_item_struct *)(((char*)defex_packed_rules) + (offset)))

enum feature_types {
	feature_is_file = 1,
	feature_for_recovery = 2,
	feature_ped_path = 4,
	feature_ped_exception = 8,
	feature_ped_status = 16,
	feature_safeplace_path = 32,
	feature_safeplace_status = 64,
	feature_immutable_path_open = 128,
	feature_immutable_path_write = 256,
	feature_immutable_src_exception = 512,
	feature_immutable_status = 1024
};

struct feature_match_entry {
	char feature_name[FEATURE_NAME_MAX_STR];
	int feature_num;
};

struct static_rule {
	unsigned int feature_type;
	char rule[STATIC_RULES_MAX_STR];
};

struct rule_item_struct {
	unsigned short int next_level;
        union {
		struct {
			unsigned short int next_file;
			unsigned short int feature_type;
		} __attribute__((packed));
		unsigned int data_size;
        } __attribute__((packed));
	unsigned char size;
#ifdef DEFEX_INTEGRITY_ENABLE
	unsigned char integrity[INTEGRITY_LENGTH];
#endif /* DEFEX_INTEGRITY_ENABLE */
	char name[0];
} __attribute__((packed));

extern const struct static_rule defex_static_rules[];
extern const int static_rule_count;

#endif /* __DEFEX_RULES_H */
