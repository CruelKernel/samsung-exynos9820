/* arch/arm64/mach-exynos/include/mach/apm-exynos.h
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *               http://www.samsung.com
 *
 * AP Parameter definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ECT_PARSER_H
#define __ECT_PARSER_H __FILE__

#include <linux/slab.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/debugfs.h>

#define BLOCK_HEADER		"HEADER"

#define BLOCK_AP_THERMAL	"THERMAL"
#define BLOCK_MIF_THERMAL	"MR4"
#define BLOCK_DVFS		"DVFS"
#define BLOCK_ASV		"ASV"
#define BLOCK_TIMING_PARAM	"TIMING"
#define BLOCK_RCC		"RCC"
#define BLOCK_PLL		"PLL"
#define BLOCK_MARGIN		"MARGIN"
#define BLOCK_MINLOCK		"MINLOCK"
#define BLOCK_GEN_PARAM		"GEN"
#define BLOCK_BIN		"BIN"
#define BLOCK_NEW_TIMING_PARAM	"NEWTIME"
#define BLOCK_PIDTM		"PIDTM"

#define SYSFS_NODE_HEADER		"header"
#define SYSFS_NODE_AP_THERMAL		"ap_thermal"
#define SYSFS_NODE_MIF_THERMAL		"mr4"
#define SYSFS_NODE_DVFS			"dvfs_table"
#define SYSFS_NODE_ASV			"asv_table"
#define SYSFS_NODE_TIMING_PARAM		"mif_timing_parameter"
#define SYSFS_NODE_RCC			"rcc_table"
#define SYSFS_NODE_PLL			"pll_list"
#define SYSFS_NODE_MARGIN		"margin_table"
#define SYSFS_NODE_MINLOCK		"minlock_table"
#define SYSFS_NODE_GEN_PARAM		"general_parameter"
#define SYSFS_NODE_BIN			"binary"
#define SYSFS_NODE_NEW_TIMING_PARAM	"new_timing_parameter"
#define SYSFS_NODE_PIDTM		"pidtm"

#define PMIC_VOLTAGE_STEP	(6250)

#define MINMAX_ASV_FUSING		(0)
#define MINMAX_ASV_TABLE		(1)
#define MINMAX_MIN_FREQ			(2)
#define MINMAX_MAX_FREQ			(3)
#define MINMAX_BOOT_FREQ		(4)
#define MINMAX_RESUME_FREQ		(5)
#define MINMAX_COL			(6)

struct ect_header
{
	char sign[4];
	char version[4];
	unsigned int total_size;
	int num_of_header;
};

enum e_dvfs_mode_flag {
	e_dvfs_mode_clock_name = 0x1,
	e_dvfs_mode_sfr_address = 0x2,
};

struct ect_dvfs_level
{
	unsigned int level;
	int level_en;
};

struct ect_dvfs_domain
{
	char *domain_name;
	unsigned int domain_offset;

	unsigned int max_frequency;
	unsigned int min_frequency;
	int boot_level_idx;
	int resume_level_idx;
	int num_of_clock;
	int num_of_level;
	enum e_dvfs_mode_flag mode;
	char **list_clock;
	unsigned int *list_sfr;
	struct ect_dvfs_level  *list_level;
	unsigned int *list_dvfs_value;
};

struct ect_dvfs_header
{
	int parser_version;
	char version[4];
	int num_of_domain;

	struct ect_dvfs_domain *domain_list;
};

struct ect_pll_frequency
{
	unsigned int frequency;
	unsigned int p;
	unsigned int m;
	unsigned int s;
	unsigned int k;
};

struct ect_pll
{
	char *pll_name;
	unsigned int pll_offset;

	unsigned int type_pll;
	int num_of_frequency;
	struct ect_pll_frequency *frequency_list;
};

struct ect_pll_header
{
	int parser_version;
	char version[4];
	int num_of_pll;

	struct ect_pll *pll_list;
};

struct ect_voltage_table
{
	int table_version;
	int boot_level_idx;
	int resume_level_idx;
	int *level_en;
	unsigned int *voltages;
	unsigned char *voltages_step;
	unsigned int volt_step;
};

struct ect_voltage_domain
{
	char *domain_name;
	unsigned int domain_offset;

	int num_of_group;
	int num_of_level;
	int num_of_table;
	unsigned int *level_list;
	struct ect_voltage_table *table_list;
};

struct ect_voltage_header
{
	int parser_version;
	char version[4];
	int num_of_domain;

	struct ect_voltage_domain *domain_list;
};

struct ect_rcc_table
{
	int table_version;

	unsigned int *rcc;
	unsigned char *rcc_compact;
};

struct ect_rcc_domain
{
	char *domain_name;
	unsigned int domain_offset;

	int num_of_group;
	int num_of_level;
	int num_of_table;
	unsigned int *level_list;
	struct ect_rcc_table *table_list;
};

struct ect_rcc_header
{
	int parser_version;
	char version[4];
	int num_of_domain;

	struct ect_rcc_domain *domain_list;
};

struct ect_mif_thermal_level
{
	int mr4_level;
	unsigned int max_frequency;
	unsigned int min_frequency;
	unsigned int refresh_rate_value;
	unsigned int polling_period;
	unsigned int sw_trip;
};

struct ect_mif_thermal_header
{
	int parser_version;
	char version[4];
	int num_of_level;

	struct ect_mif_thermal_level *level;
};

struct ect_ap_thermal_range
{
	unsigned int lower_bound_temperature;
	unsigned int upper_bound_temperature;
	unsigned int max_frequency;
	unsigned int sw_trip;
	unsigned int flag;
};

struct ect_ap_thermal_function
{
	char *function_name;
	unsigned int function_offset;

	int num_of_range;
	struct ect_ap_thermal_range *range_list;
};

struct ect_ap_thermal_header
{
	int parser_version;
	char version[4];
	int num_of_function;

	struct ect_ap_thermal_function *function_list;
};

struct ect_margin_domain
{
	char *domain_name;
	unsigned int domain_offset;

	int num_of_group;
	int num_of_level;
	unsigned int *offset;
	unsigned char *offset_compact;
	unsigned int volt_step;
};

struct ect_margin_header
{
	int parser_version;
	char version[4];
	int num_of_domain;

	struct ect_margin_domain *domain_list;
};

struct ect_timing_param_size
{
	unsigned int memory_size;
	unsigned long long parameter_key;
	unsigned int offset;

	int num_of_timing_param;
	int num_of_level;
	unsigned int *timing_parameter;
};

struct ect_timing_param_header
{
	int parser_version;
	char version[4];
	int num_of_size;

	struct ect_timing_param_size *size_list;
};

struct ect_minlock_frequency
{
	unsigned int main_frequencies;
	unsigned int sub_frequencies;
};

struct ect_minlock_domain
{
	char *domain_name;
	unsigned int domain_offset;

	int num_of_level;
	struct ect_minlock_frequency *level;
};

struct ect_minlock_header
{
	int parser_version;
	char version[4];
	int num_of_domain;

	struct ect_minlock_domain *domain_list;
};

struct ect_gen_param_table
{
	char *table_name;
	unsigned int offset;

	int num_of_col;
	int num_of_row;
	unsigned int *parameter;
};

struct ect_gen_param_header
{
	int parser_version;
	char version[4];
	int num_of_table;

	struct ect_gen_param_table *table_list;
};

struct ect_bin
{
	char *binary_name;
	unsigned int offset;

	int binary_size;
	void *ptr;
};

struct ect_bin_header
{
	int parser_version;
	char version[4];
	int num_of_binary;

	struct ect_bin *binary_list;
};

enum e_new_timing_parameter_mode
{
	e_mode_normal_value = 0x1,
	e_mode_extend_value = 0x2,
};

struct ect_new_timing_param_size
{
	unsigned long long parameter_key;
	unsigned int offset;

	enum e_new_timing_parameter_mode mode;
	int num_of_timing_param;
	int num_of_level;

	unsigned int *timing_parameter;
};

struct ect_new_timing_param_header
{
	int parser_version;
	char version[4];
	int num_of_size;

	struct ect_new_timing_param_size *size_list;
};

struct ect_pidtm_block
{
	char *block_name;
	unsigned int offset;

	int num_of_temperature;
	int *temperature_list;

	int num_of_parameter;
	char **param_name_list;
	int *param_value_list;
};

struct ect_pidtm_header
{
	int parser_version;
	char version[4];
	int num_of_block;

	struct ect_pidtm_block *block_list;
};

struct ect_info
{
	char *block_name;
	int block_name_length;
	int (*parser)(void *address, struct ect_info *info);
	int (*dump)(struct seq_file *s, void *data);
	struct file_operations dump_ops;
	char *dump_node_name;
	void *block_handle;
	int block_precedence;
};

#if defined(CONFIG_ECT)
void ect_init(phys_addr_t address, phys_addr_t size);
int ect_parse_binary_header(void);
void* ect_get_block(char *block_name);
struct ect_dvfs_domain *ect_dvfs_get_domain(void *block, char *domain_name);
struct ect_pll *ect_pll_get_pll(void *block, char *pll_name);
struct ect_voltage_domain *ect_asv_get_domain(void *block, char *domain_name);
struct ect_rcc_domain *ect_rcc_get_domain(void *block, char *domain_name);
struct ect_mif_thermal_level *ect_mif_thermal_get_level(void *block, int mr4_level);
struct ect_ap_thermal_function *ect_ap_thermal_get_function(void *block, char *function_name);
struct ect_margin_domain *ect_margin_get_domain(void *block, char *domain_name);
struct ect_timing_param_size *ect_timing_param_get_size(void *block, int size);
struct ect_timing_param_size *ect_timing_param_get_key(void *block, unsigned long long key);
struct ect_minlock_domain *ect_minlock_get_domain(void *block, char *domain_name);
struct ect_gen_param_table *ect_gen_param_get_table(void *block, char *table_name);
struct ect_bin *ect_binary_get_bin(void *block, char *binary_name);
struct ect_new_timing_param_size *ect_new_timing_param_get_key(void *block, unsigned long long key);
struct ect_pidtm_block *ect_pidtm_get_block(void *block, char *block_name);

void ect_init_map_io(void);

int ect_strcmp(char *src1, char *src2);
int ect_strncmp(char *src1, char *src2, int length);

unsigned long long ect_read_value64(unsigned int *address, int index);

#else

static inline void ect_init(phys_addr_t address, phys_addr_t size) {}
static inline int ect_parse_binary_header(void) { return 0; }
static inline void* ect_get_block(char *block_name) { return NULL; }
static inline struct ect_dvfs_domain *ect_dvfs_get_domain(void *block, char *domain_name) { return NULL; }
static inline struct ect_pll *ect_pll_get_pll(void *block, char *pll_name) { return NULL; }
static inline struct ect_voltage_domain *ect_asv_get_domain(void *block, char *domain_name) { return NULL; }
static inline struct ect_rcc_domain *ect_rcc_get_domain(void *block, char *domain_name) { return NULL; }
static inline struct ect_mif_thermal_level *ect_mif_thermal_get_level(void *block, int mr4_level) { return NULL; }
static inline struct ect_ap_thermal_function *ect_ap_thermal_get_function(void *block, char *function_name) { return NULL; }
static inline struct ect_margin_domain *ect_margin_get_domain(void *block, char *domain_name) { return NULL; }
static inline struct ect_timing_param_size *ect_timing_param_get_size(void *block, int size) { return NULL; }
static inline struct ect_timing_param_size *ect_timing_param_get_key(void *block, unsigned long long key) { return NULL; }
static inline struct ect_minlock_domain *ect_minlock_get_domain(void *block, char *domain_name) { return NULL; }
static inline struct ect_gen_param_table *ect_gen_param_get_table(void *block, char *table_name) { return NULL; }
static inline struct ect_bin *ect_binary_get_bin(void *block, char *binary_name) { return NULL; }
static inline struct ect_new_timing_param_size *ect_new_timing_param_get_key(void *block, unsigned long long key) { return NULL; }
static inline struct ect_pidtm_block *ect_pidtm_get_block(void *block, char *block_name) { return NULL; }

static inline void ect_init_map_io(void) {}

static inline int ect_strcmp(char *src1, char *src2) { return -1; }
static inline int ect_strncmp(char *src1, char *src2, int length) { return -1; }

static inline unsigned long long ect_read_value64(unsigned int *address, int index) { return 0; }

#endif

#endif
