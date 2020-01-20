/* sec_nad.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef SEC_NAD_H
#define SEC_NAD_H

#if defined(CONFIG_SEC_FACTORY)
#if defined(CONFIG_SEC_NAD_LOG)
#define NAD_LOG_SIZE 0x100000
#define NAD_UFS_TEST_BLOCK "/dev/block/UFS_TEST" 
#define NAD_LOG_OFFSET 0xAC000
#endif   
#define NAD_PARAM_NAME "/dev/block/NAD_REFER"
#define NAD_OFFSET 8192
#define NAD_ENV_OFFSET 10240 * 2

#define NAD_PARAM_READ	0
#define NAD_PARAM_WRITE	1
#define NAD_PARAM_EMPTY	2

#define NAD_DRAM_TEST_NONE	0
#define NAD_DRAM_TEST_PASS	1
#define NAD_DRAM_TEST_FAIL	2

#define NAD_BUFF_SIZE		10
#define NAD_CMD_LIST		3

#define NAD_ACAT_FAIL_SKIP_FLAG	1001
#define UNLIMITED_LOOP_FLAG		1002
#define NAD_SUPPORT_FLAG		1003
#define NAD_ACAT_FLAG			5001
#define NAD_SECOND_FLAG			6001
#define NAD_ACAT_SECOND_FLAG	5002

#define NAD_RETRY_COUNT			30
#define NAD_FAIL_COUNT			10

#define NAD_HPM_BIG_LEVELCOUNT 9
#define NAD_HPM_LIT_LEVELCOUNT 6
#define NAD_HPM_G3D_LEVELCOUNT 4
#define NAD_HPM_MIF_LEVELCOUNT 8

/* MAGIC CODE for NAD API Success */
#define MAGIC_NAD_API_SUCCESS	6057

/* vst */
#define NAD_VST_MAGIC_NUM	0x95252A
#define NAD_VST_MAGIC_MASK	0xFFFFFF
#define NAD_VST_RESULT_MASK	0x3F
#define NAD_VST_ADJUST_MASK	0x3F

enum {
    EXYNOS8890 = 0,
    EXYNOS8895 = 1,
    EXYNOS7880 = 2,
    EXYNOS7870 = 3,
    EXYNOS7570 = 4,
    EXYNOS7885 = 5,
    EXYNOS9810 = 6,
    EXYNOS9110 = 7,
    EXYNOS9820 = 8,
    EXYNOS9610 = 9,
};


	//EXYNOS8890_JUNGFRAU[0], EXYNOS8895_KANGCHEN[1], EXYNOS7880_JOON[2], EXYNOS7870_JOSHUA[3], EXYNOS7570_JAVA[4], EXYNOS7885_LASSEN[5],EXYNOS7883_LASSENQ[5], EXYNOS7884_LASSENO[5],EXYNOS9810_LHOTSE[6],EXYNOS9610_RAMEN[7],EXYNOS9820_MAKALU[8]
	static  char nad_chipset_name[][12] = {"EXYNOS8890", "EXYNOS8895", "EXYNOS7880", "EXYNOS7870", "EXYNOS7570", "EXYNOS7885", "EXYNOS9810","EXYNOS9110","EXYNOS9820","EXYNOS9610","EXYNOSXXXX","EXYNOSXXXX"};
  static char nad_block_name[10][8] = {"DRAM", "BIG", "MIDD", "LITT", "MIF", "G3D", "INT", "CAM", "FUNC", "CP"};	

#if defined(CONFIG_SEC_SUPPORT_SECOND_NAD)
typedef struct{
	volatile u64 target_addr;	/* target address for test */
	volatile u64 expected_val;	/* written val */
	volatile u64 read_val;		/* read real value */

	unsigned int rank_id;		/* dram detail infomation*/
	unsigned int bank_id;
	unsigned int low_data;
	unsigned int col_data;

	unsigned int test_item;		/* mif or dram */
	unsigned int big_level;
	unsigned int lit_level;
	unsigned int mif_level;
	unsigned int int_level;
	unsigned int op_core_num;	/* operating CORE num */
} nad_dram_test_info;

typedef struct  {
	char magic[8];
	unsigned int error_count;
	nad_dram_test_info nad_dram_fail_info[20];
} nad_dram_information;

typedef struct  {
	unsigned int  das;
	char  das_string[10];
	unsigned int  block;
	char  block_string[10];
	unsigned int  level;
	unsigned int  vector;
	char  vector_string[10];
} nad_fail_information;

#if defined(CONFIG_SEC_SUPPORT_VST)
typedef struct  {
	char magic[10];
	u8 big_pass_fail;
	u8 mid_pass_fail;
	u8 lit_pass_fail;
	u8 g3d_pass_fail;
	u8 mif_pass_fail;
	u8 int_pass_fail;

	int nad_version;
	int vst_result;
	int vst_adjust;
	int vst_f_res;
	int vst_is_excuted;

	u8 big_up_margin;
	u8 mid_up_margin;
	u8 lit_up_margin;
	u8 g3d_up_margin;
	u8 mif_up_margin;
	u8 int_up_margin;
} vst_information;

typedef struct {
    unsigned int list_num;
    unsigned int block;
    unsigned int vector;
    unsigned int level;
    unsigned int operation_time;
    unsigned int vst_current;
	unsigned int spec_out;
    unsigned int rsvd;
} current_info;

typedef struct {
    char magic[8];
    int total_num;
    int rsvd;
    current_info current_list[100];
} nad_ave_current_information;
#endif

#if defined(CONFIG_SEC_NAD_HPM)
typedef struct  {
	u8 level;
	u8 fused_hpm;
	u8 pba_hpm[10];
	u8 pba_hpm_min;
	unsigned int pba_hpm_ave;
	unsigned int pba_hpm_var;
} hpm_level_information;

typedef struct  {
	hpm_level_information hpm_level_info[15];		//L0 ~ L15
	u8 level_count;
	u8 block_pass_fail;
} hpm_information;

typedef struct  {
	char magic[10];
	hpm_information  hpm_info[4];	//BIG, LITT, G3D, MIF
	u8 device_pass_fail;
	u8 hpm_invalid;
	char LotID[7];
	u8 asv_table_ver;
	u8 big_grp;
	u8 lit_grp;
	u8 g3d_grp;
	u8 mif_grp;
} nad_hpm;
#endif

typedef struct  {
	unsigned int nad_inform1;
	unsigned int nad_inform2;

	nad_fail_information nad_fail_info;

} nad_fail_backup_data;
#endif

#if defined(CONFIG_SEC_NAD_API)
static char nad_api_result_string[20*30] = {0,};

typedef struct {
	char name[20];
	unsigned int result;
} nad_api_results;
#endif

struct nad_env {
	char nad_factory[8];
	char nad_result[8];
	unsigned int nad_data;

	char nad_acat[8];
	char nad_acat_result[8];
	unsigned int nad_acat_data;
	unsigned int nad_acat_loop_count;
	unsigned int nad_acat_real_count;

	char nad_dram_test_need[8];
	unsigned int nad_dram_test_result;
	unsigned int nad_dram_fail_data;
	unsigned long nad_dram_fail_address;

	int current_nad_status;
	unsigned int nad_acat_skip_fail;
	unsigned int unlimited_loop;
	unsigned int nad_support;
	unsigned int max_temperature;
	unsigned int nad_acat_max_temperature;

#if defined(CONFIG_SEC_SUPPORT_VST)
	vst_information vst_info;
	int vst_big_fail_count;
	int vst_mid_fail_count;
	int vst_lit_fail_count;
	int vst_g3d_fail_count;
	int vst_mif_fail_count;
	int vst_int_fail_count;
	int vst_big_fail_inform4_2;
	int vst_mid_fail_inform4_2;
	int vst_lit_fail_inform4_2;
	int vst_g3d_fail_inform4_2;
	int vst_mif_fail_inform4_2;
	int vst_int_fail_inform4_2;

	nad_ave_current_information nad_ave_current_info;
	unsigned char nAsv_TABLE;
#endif

#if defined(CONFIG_SEC_SUPPORT_SECOND_NAD)
	unsigned int  nad_inform2_data;
	unsigned int  nad_inform3_data;
	unsigned int  nad_init_temperature;

	unsigned int  nad_acat_init_temperature;
	unsigned int  nad_acat_inform2_data;
	unsigned int  nad_acat_inform3_data;

	char nad_second[8];
	char nad_second_result[8];
	unsigned int  nad_second_data;
	unsigned int  nad_second_inform2_data;
	unsigned int  nad_second_inform3_data;
	unsigned int  nad_second_max_temperature;
	unsigned int  nad_second_init_temperature;

	char nad_acat_second[8];
	char nad_acat_second_result[8];
	unsigned int  nad_acat_second_data;
	unsigned int  nad_acat_second_inform2_data;
	unsigned int  nad_acat_second_inform3_data;
	unsigned int  nad_acat_second_max_temperature;
	unsigned int  nad_acat_second_init_temperature;

	nad_fail_information nad_fail_info;				/* 1st NAD */
	nad_fail_information nad_second_fail_info;		/* 2nd NAD */
	nad_fail_information nad_acat_fail_info;		/* ACAT 1st NAD */
	nad_fail_information nad_acat_second_fail_info;	/* ACAT 2nd NAD */

	nad_dram_information nad_dram_fail_information;
	nad_dram_information nad_second_dram_fail_information;
	nad_dram_information nad_acat_dram_fail_information;
	nad_dram_information nad_acat_second_dram_fail_information;

	unsigned int  nad_acat_second_running_count;
	unsigned int acat_fail_retry_count;
	nad_fail_backup_data acat_fail_backup[NAD_FAIL_COUNT];
#endif
#if defined(CONFIG_SEC_NAD_HPM)
	nad_hpm nad_hpm_info;
	int hpm_min_diff_level_big[NAD_HPM_BIG_LEVELCOUNT];
	int hpm_min_diff_level_lit[NAD_HPM_LIT_LEVELCOUNT];
	int hpm_min_diff_level_g3d[NAD_HPM_G3D_LEVELCOUNT];
	int hpm_min_diff_level_mif[NAD_HPM_MIF_LEVELCOUNT];
#endif
#if defined(CONFIG_SEC_NAD_API)
	int nad_api_status;
	int nad_api_magic;
	int nad_api_total_count;
	nad_api_results nad_api_info[30];
#endif

	int nad_enter_status;
	
#if defined(CONFIG_SEC_NAD_X)
	char nad_extend[8];
	char nad_extend_result[8];
	unsigned int nadx_is_excuted;
	unsigned int nad_extend_data;
	unsigned int nad_extend_inform2_data;
	unsigned int nad_extend_inform3_data;
	unsigned int nad_extend_max_temperature;
	unsigned int nad_extend_init_temperature;
	unsigned int nad_extend_loop_count;
	unsigned int nad_extend_real_count;
	unsigned int nad_extend_skip_fail;
	nad_fail_information nad_extend_fail_info;
	nad_dram_information nad_extend_dram_fail_information;
	unsigned int nad_X_Pre_Domain_Level[7]; //{cl2_lv, cl1_lv, cl0_lv, g3d_lv, mif_lv, int_lv, disp_lv},
	unsigned int nad_X_Margin;

	unsigned int nadx_fail_count;
	nad_fail_backup_data nadx_fail_data_backup;
	
	char nad_extend_second[8];
	char nad_extend_second_result[8];
	unsigned int nad_extend_second_data;
	unsigned int nad_extend_second_inform2_data;
	unsigned int nad_extend_second_inform3_data;
	unsigned int nad_extend_second_max_temperature;
	unsigned int nad_extend_second_init_temperature;
	nad_fail_information nad_extend_second_fail_info;
	nad_dram_information nad_extend_second_dram_fail_information;
	unsigned int nad_X_second_Pre_Domain_Level[7]; //{cl2_lv, cl1_lv, cl0_lv, g3d_lv, mif_lv, int_lv, disp_lv},
	unsigned int nad_X_second_Margin;
#endif

	nad_fail_backup_data last_fail_data_backup;
	nad_dram_information nad_last_dram_fail_information;
	int last_nad_fail_status;
};


struct sec_nad_param {
	struct work_struct sec_nad_work;
	struct delayed_work sec_nad_delay_work;
	unsigned long offset;
	int state;
	int retry_cnt;
	int curr_cnt;
};

static struct sec_nad_param sec_nad_param_data;
static struct nad_env sec_nad_env;
extern unsigned int lpcharge;
static DEFINE_MUTEX(sec_nad_param_lock);
#endif

#endif
