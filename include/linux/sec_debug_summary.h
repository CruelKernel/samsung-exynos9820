/*
 * include/linux/sec_debug_summary.h
 *
 * header file supporting debug functions for Samsung device
 *
 * COPYRIGHT(C) 2006-2016 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SEC_DEBUG_SUMMARY_H
#define SEC_DEBUG_SUMMARY_H

#include <linux/sec_debug.h>

#define SEC_DEBUG_SUMMARY_MAGIC0 0xFFFFFFFF
#define SEC_DEBUG_SUMMARY_MAGIC1 0x5ECDEB6
#define SEC_DEBUG_SUMMARY_MAGIC2 0x14F014F0
#define SEC_DEBUG_SUMMARY_MAGIC3 0x00010004

struct var_info {
	char name[32];
	int sizeof_type;
	uint64_t var_paddr;
}__attribute__((aligned(32)));

struct sec_debug_summary_simple_var_mon {
	int idx;
	struct var_info var[32];
};

struct sec_debug_summary_data_ap {
	char name[16];
	char state[16];
	char mdmerr_info[128];
	int nr_cpus;
	struct sec_debug_summary_simple_var_mon info_mon;

	unsigned long reserved_out_buf;
	unsigned long reserved_out_size;
};

struct sec_debug_summary_private {
	struct sec_debug_summary_data_ap apss;
};

struct sec_debug_summary {
	unsigned int magic[4];
	union {
		struct sec_debug_summary_data_ap *apss;
		uint64_t _apss;
	};

	struct sec_debug_summary_private priv;
};

extern int sec_debug_summary_add_infomon(char *name, unsigned int size,
	phys_addr_t addr);

#define ADD_VAR_TO_INFOMON(var) \
	sec_debug_summary_add_infomon(#var, sizeof(var), \
		(uint64_t)__pa(&var))
#define ADD_STR_TO_INFOMON(pstr) \
	sec_debug_summary_add_infomon(#pstr, -1, (uint64_t)__pa(pstr))

#define VAR_NAME_MAX	30

int __init sec_debug_summary_init(void);

extern char *sec_debug_arch_desc;

#endif /* SEC_DEBUG_SUMMARY_H */
